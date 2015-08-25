#include "plugin.h"
#include <windows.h>
#include <stdio.h>
#include <fstream>
#include <string.h>
#include <map>
#include "Mesh.h"
#include "BoxGrid.h"
#include "Handles.h"
#include "config.h"
#include "ARAPDeformer.h"
#include "PCFShadow.h"
#include "trackball.h"
#include "Support.h"
#include "rendering_functions.h"
#include "embree_intersector.h"
#include "timer.h"
#include "SOIL.h"

MISPlugin MISPluginInstance;
void TW_CALL SetMeshFileCB(const void *value, void *clientData)
{
	MISPluginInstance.cfg_meshfile = *(const int *)value;
	MISPluginInstance.close();
	MIS::m_config = new Config(MISPluginInstance.getMeshfile());
	MIS::m_mesh = new Mesh(MIS::m_config->getMeshfile());
	MIS::m_support = new Support(DEFAULT_FLATTEN_THRESHOLD,DEFAULT_ANGLE_OBJ);
	MIS::m_embree = new EmbreeIntersector(MIS::m_mesh->getV(),MIS::m_mesh->getF());
	MISPluginInstance.updateAfterConfigLoading();
}
void TW_CALL VoxCB(void *clientData)
{
	MIS::voxelize();
	MIS::m_support->clear();
	MIS::m_support->updateStandingZone(*MIS::m_mesh);
	MIS::m_mesh->computeAnchor(*MIS::m_support);
	MIS::m_voxels->computeAnchors(*MIS::m_mesh);
	MIS::m_mesh->computeEmbedWeight(*MIS::m_voxels);
	MIS::initialize();
	MIS::init_ARAP();
	MIS::init_RBF();
	MISPluginInstance.updateAfterVoxelLoading();
}
void TW_CALL SetVoxResCB(const void *value, void *clientData)
{ 
	MIS::m_config->setVoxRes(*(const int *)value);
}
void TW_CALL GetVoxResCB(void *value, void *clientData)
{ 
	*(int *)value = MIS::m_config->getVoxRes();
}
void TW_CALL GetMeshFileCB(void *value, void *clientData)
{ 
	*(int *)value = MISPluginInstance.cfg_meshfile;
}
void TW_CALL SaveCB(void *clientData)
{
	if(!MIS::isConfigLoaded()) return;
	if(MIS::isVoxelized())
	{
		const string voxfile = MIS::m_config->getVoxfile();
		MIS::m_voxels->saveToFile(voxfile);
	}
	MIS::m_config->saveConfig(MIS::m_config->getConfigfile());
}
void TW_CALL SetCornerThresCB(const void *value, void *clientData)
{ 
	if(!MIS::isConfigLoaded()) return;

	MIS::m_mesh->corner_threshold = *(const ScalarType *)value;
	MIS::m_mesh->ComputeCornerNormals();
}
void TW_CALL GetCornerThresCB(void *value, void *clientData)
{ 
	if(MIS::isConfigLoaded())
		*(ScalarType *)value = MIS::m_mesh->corner_threshold;
	else
		*(ScalarType *)value = 20.0;
}
void TW_CALL CopyStdStringToClient(std::string& destinationClientString, const std::string& sourceLibraryString)
{
  destinationClientString = sourceLibraryString;
}
	static bool pick_id_handle(int mouse_x, int mouse_y,const Config & cfg,
		double *modelview_matrix, double *projection_matrix, int *viewport, 
		int & idHandle)
	{
		GLfloat winX, winY;               // Holds Our X, Y and Z Coordinates
		winX = (float)mouse_x;
		winY = (float)viewport[3] - mouse_y;
		idHandle = -1;
		ScalarType best = 50.0; double dist;
		double posX, posY, posZ;
		for(int j = 0 ; j < cfg.getNbHandles() ; ++j) {
			RowVector3 p;
		    p = cfg.getHandle(j);
			gluProject(p[0],p[1],p[2],modelview_matrix,projection_matrix,viewport,&posX,&posY,&posZ);
			dist = sqrt((posX-winX)*(posX-winX) + (posY-winY)*(posY-winY));
			if(dist < best) {
				idHandle = j;
				best = dist;
			}
		}
		return idHandle != -1;
	}
	static bool pick_id_handle(int mouse_x, int mouse_y,const Handles & handle,
		double *modelview_matrix, double *projection_matrix, int *viewport, 
		int & idHandle)
	{
		GLfloat winX, winY;               // Holds Our X, Y and Z Coordinates
		winX = (float)mouse_x;
		winY = (float)viewport[3] - mouse_y;
		idHandle = -1;
		ScalarType best = 50.0; double dist;
		double posX, posY, posZ;
		for(int j = 0 ; j < handle.getNumHandles();  ++j) {
			RowVector3 p;
			p = handle.getHandlePose(j);
			gluProject(p[0],p[1],p[2],modelview_matrix,projection_matrix,viewport,&posX,&posY,&posZ);
			dist = sqrt((posX-winX)*(posX-winX) + (posY-winY)*(posY-winY));
			if(dist < best) {
				idHandle = j;
				best = dist;
			}
		}
		return idHandle != -1;
	}
	static bool embree_create_handle(int mouse_x, int mouse_y, const EmbreeIntersector *emb,
		double *modelview_matrix, double *projection_matrix, int *viewport, 
		RowVector3 & P)
	{
		if (!emb) 
			return -1;
		GLfloat winX, winY;               // Holds Our X, Y and Z Coordinates
		winX = (float)mouse_x;
		winY = (float)viewport[3] - mouse_y;	
		double direction[3];
		gluUnProject(winX, winY, 1, modelview_matrix,projection_matrix,viewport, &(direction[0]),&(direction[1]),&(direction[2]));
		Vector3 dir(direction[0],direction[1],direction[2]);
		Vector3 origin;
		Matrix44 modelViewMat;
		for (int i = 0; i<4; ++i)
			for (int j = 0; j<4; ++j)
				modelViewMat(i,j) = modelview_matrix[4*j+i];
		origin = modelViewMat.inverse().block(0,3,3,1);
		dir -= origin;
		dir.normalize();
		embree::Ray hit1(toVec3f(origin),toVec3f(dir));
		int fi = -1;
		RowVector3 bc;
		if (emb->intersectRay(hit1))
		{
			fi = hit1.id0;
			bc << 1 - hit1.u - hit1.v, hit1.u, hit1.v;
			RowVector3 P1 = MIS::m_mesh->V(fi,bc);
			origin = origin + 50.0f*dir;
			dir = -dir;
			embree::Ray hit2(toVec3f(origin),toVec3f(dir));
			if (emb->intersectRay(hit2))
			{
				fi = hit2.id0;
				bc << 1 - hit2.u - hit2.v, hit2.u, hit2.v;
				RowVector3 P2 = MIS::m_mesh->V(fi,bc);
				P = 0.5*(P1+P2);
				return true;
			}
		}
		return false;
	}
	void MISPlugin::drawPlane() {
		RowVector3 cn(0,-1,0);
		RowVector3 ground = MIS::m_mesh->getLowestVertex(RowVector3(0,-1,0));
		RowVector3 ref = ground- plane_offset*cn;
		RowVector3 c1 = ref + RowVector3(-1000,0,-1000);
		RowVector3 c2 = ref + RowVector3(-1000,0,1000);
		RowVector3 c3 = ref + RowVector3(1000,0,1000);
		RowVector3 c4 = ref + RowVector3(1000,0,-1000);
		glBegin(GL_QUADS);
		glNormal3f(cn[0],cn[1],cn[2]);
		glVertex3f(c1[0],c1[1],c1[2]);
		glVertex3f(c2[0],c2[1],c2[2]);
		glVertex3f(c3[0],c3[1],c3[2]);
		glVertex3f(c4[0],c4[1],c4[2]);
		glEnd();
	}
	void MISPlugin::init(Preview3D *preview)
	{
		cout << "LOADING MIS PLUGIN..." << endl;
		findFiles(MESH_DIR,OBJ_EXT,objFiles);
		PreviewPlugin::init(preview);
		mybar = TwNewBar("VoxARAP");
		TwDefine(" VoxARAP size='280 200' text=light alpha=255 color='40 40 40' position='3 3'");
		TwCopyStdStringToClientFunc(CopyStdStringToClient);

		std::stringstream ssEnumMeshFile;
		ssEnumMeshFile << objFiles[0];
		for(unsigned int i = 1 ; i < objFiles.size() ; ++i)
			ssEnumMeshFile << "," << objFiles[i];
		TwType meshFileType = TwDefineEnumFromString("MeshFileType", ssEnumMeshFile.str().c_str());
        TwAddVarCB(mybar, "meshFile", meshFileType, SetMeshFileCB, GetMeshFileCB, this, " visible=true label='mesh file :' ");	
		TwAddVarCB(mybar, "voxRes", TW_TYPE_UINT32, SetVoxResCB, GetVoxResCB, this, " visible=false label='Voxel grid resolution' max=256 step=2 ");
		TwAddButton(mybar, "voxelCfg", VoxCB, this, "visible=false label='voxelize' ");
		TwAddButton(mybar, "save", SaveCB, this, "visible=false label='Save' ");

		TwAddSeparator(mybar, "sep1", " ");
		TwAddVarRW(mybar, "display_volume", TW_TYPE_BOOLCPP, &display_volume, "visible=true");
		TwAddVarRW(mybar, "display_mesh", TW_TYPE_BOOLCPP, &display_mesh, "visible=true");
		TwAddVarRW(mybar, "display_texture", TW_TYPE_BOOLCPP, &display_texture, "visible=true");
		TwAddVarRW(mybar, "display_handles", TW_TYPE_BOOLCPP, &display_handles, "visible=true");
		TwAddVarRW(mybar, "display_shadow", TW_TYPE_BOOLCPP, &display_shadow, "visible=true");
		TwAddVarRW(mybar, "display_wire", TW_TYPE_BOOLCPP, &MIS::wire, "visible=true");
		TwAddVarRW(mybar, "display_floor", TW_TYPE_BOOLCPP, &display_floor, "visible=true");
		TwAddVarRW(mybar, "display_text", TW_TYPE_BOOLCPP, &display_text, "visible=true");
		TwAddVarRW(mybar, "Plane offset", TW_TYPE_FLOAT, &plane_offset, "visible=true min=-1000.7 max=1000.7 step=0.01 ");
		TwAddVarRW(mybar, "gap", TW_TYPE_FLOAT, &offset, "visible=true min=-10.7 max=10.7 step=0.01 ");
		

		stats = TwNewBar("Statistics");
		TwDefine(" Statistics size='280 100' text=light alpha=255 color='40 40 40' position='3 200' valueswidth=100");
		TwAddVarRO(stats, "nbN", TW_TYPE_UINT32, &log_nbN, " label='Nodes :' ");		
		TwAddVarRO(stats, "nbC", TW_TYPE_UINT32, &log_nbC, " label='Voxels :' ");		
		TwAddVarRO(stats, "nbV", TW_TYPE_UINT32, &log_nbV, " label='Vertices :' ");
		TwAddVarRO(stats, "nbF", TW_TYPE_UINT32, &log_nbF, " label='Triangles :' ");		
		TwType mouseModeType = TwDefineEnum("MouseModeType", NULL, 0);
		TwAddVarRO(stats, "mouseMode", mouseModeType, &log_mouse_mode, " label='Mode :' enum='0 {NOTHING}, 1 {TRANSLATE}, 2 {CREATE}'  ");

		media_exponent = 0.3;
		media_density = 0.7;
		media_color[0] = 158.0/255.0;
		media_color[1] = 189.0/255.0;
		media_color[2] = 248.0/255.0;
		mesh_color[0] = 158.0/255.0;
		mesh_color[1] = 189.0/255.0;
		mesh_color[2] = 248.0/255.0;
		mesh_color[3] = 74.0/255.0;
		imesh_color[0] = 238.0/255.0;
		imesh_color[1] = 250.0/255.0;
		imesh_color[2] = 0.0/255.0;
		imesh_color[3] = 138.0/255.0;

		TwAddVarRW(m_preview->bar, "Media exponent", TW_TYPE_FLOAT, &media_exponent, " group='Transparency' min=0.0 max=2.0 step=0.02 ");
		TwAddVarRW(m_preview->bar, "Media density", TW_TYPE_FLOAT, &media_density, " group='Transparency' min=0.0 max=5.0 step=0.1 ");
		TwAddVarRW(m_preview->bar, "Media color", TW_TYPE_COLOR3F, media_color, " group='Transparency' colormode=hls");
		TwAddVarRW(m_preview->bar, "Surface color", TW_TYPE_COLOR4F, mesh_color, " group='Transparency'");
		TwAddVarRW(m_preview->bar, "Body color", TW_TYPE_COLOR4F, imesh_color, " group='Transparency'");
		TwAddVarCB(m_preview->bar, "Corner threshold", TW_TYPE_SCALARTYPE, SetCornerThresCB, GetCornerThresCB, this, " min=0 max=180 step=2 ");

		cout << "\t [done]" << endl;
	}

	void MISPlugin::updateAfterConfigLoading() {
		log_nbV = MIS::m_mesh->nbV;
		log_nbF = MIS::m_mesh->nbF;
		get_scale_and_shift_to_fit_mesh(MIS::m_mesh->vertices,m_preview->zoom,m_preview->mesh_center);
		m_preview->radius = 1/m_preview->zoom;
		m_preview->shift = RowVector3(0,-0.1,0);
		resetCamera();
		TwDefine(" VoxARAP/voxelCfg visible=true ");
		TwDefine(" VoxARAP/voxRes visible=true ");
		MIS::m_mesh->computeRestVolume();
		MIS::m_mesh->computeCurrentVolume();
	}
	void MISPlugin::updateAfterVoxelLoading()
	{
		log_nbN = MIS::m_voxels->getNumNodes();
		log_nbC = MIS::m_voxels->getNumBoxes();
		TwDefine(" VoxARAP/save visible=true ");
	}
	bool MISPlugin::keyDownEvent(unsigned char key, int modifiers, int mouse_x, int mouse_y) {

		if(key == 2) m_preview->SetAutoRotate(!m_preview->g_AutoRotate);
		if(key == 3) {resetCamera();}
		if(key==4){MIS::embed_mode++;MIS::embed_mode %=3;MIS::updateSurface();}
		if(key==5){MIS::m_handles->reset();MIS::updateVoxelGrid();}
		if(!MIS::isConfigLoaded()) return false;	
		if(key == 100) {
			m_preview->g_RotationAngle -= 0.1;
		}
		if(key == 102) {
			m_preview->g_RotationAngle += 0.1;
		}
		if(MIS::isReady())
		{
			if( modifiers == (Preview3D::ALT) ) {
				translation = RowVector3::Zero();
				if(key == 't')
				{
					m_mouse_mode = TRANSLATE_HANDLE;
					return true;
				}
				if(key == 'e')
				{
					m_mouse_mode = HANDLE_CREATION;
					return true;
				}
			}
		}
		if(key=='p')
			takeScreenshot();
		if(key=='r')
			recording = !recording;
		if(key=='0')
			m_mouse_mode = NOTHING;
		return false;
	}
	
	bool MISPlugin::keyUpEvent(unsigned char key, int modifiers, int mouse_x, int mouse_y) {
		return false;
	}

	bool MISPlugin::mouseDownEvent(int mouse_x, int mouse_y, int button, int modifiers) { 
		if(!MIS::isConfigLoaded()) return false;
		mouse_down = true;
		from_x = mouse_x;
		from_y = mouse_y;
		if(button==0)
		{
			if(m_mouse_mode == TRANSLATE_HANDLE)
			{
				deforming = (tmp_id_handle >= 0);
				if(deforming){
					translation_down= MIS::m_handles->getHandlePose(tmp_id_handle);
				}
			}
			if(m_mouse_mode == HANDLE_CREATION) {
				deforming = (tmp_id_handle >= 0);
			}
		}
		if(button == 2) {
			if(m_mouse_mode == HANDLE_CREATION) {
				if(tmp_id_handle >= 0) {
					MIS::m_config->deleteHandle(tmp_id_handle);
					tmp_id_handle = -1;
					MIS::m_handles->update(*MIS::m_config, *MIS::m_voxels);
					MIS::update_ARAP();
				}
				else {
					MIS::m_config->addHandle(tmp_handle);
					MIS::m_handles->update(*MIS::m_config, *MIS::m_voxels);
					MIS::update_ARAP();
				}
				return true;
			}
		}
		return false;
	}
	
	bool MISPlugin::mouseUpEvent(int mouse_x, int mouse_y, int button, int modifiers) {
		if(!MIS::isConfigLoaded()) return false;
		mouse_down = false;
		if(deforming) {
			deforming = false;
			translation = RowVector3::Zero();
			return true;
		}
		return false;
	};
	
	bool MISPlugin::mouseMoveEvent(int mouse_x, int mouse_y) {
		if(!MIS::isConfigLoaded()) return false;
		if(mouse_down) {
			if(deforming) {
				if(m_mouse_mode == TRANSLATE_HANDLE) {
					RowVector3 p = MIS::m_handles->getHandlePose(tmp_id_handle);
					get_translation(p,
						mouse_x,mouse_y, from_x, from_y, translation);
					MIS::m_handles->setT(tmp_id_handle, translation_down+translation);
					MIS::updateVoxelGrid();
					return true;
				}
				if(m_mouse_mode == HANDLE_CREATION) {
					get_translation(
						MIS::m_config->getHandle(tmp_id_handle),
						mouse_x,mouse_y, from_x, from_y, translation);
					MIS::m_config->moveHandle(tmp_id_handle,translation);	
					MIS::m_handles->update(*MIS::m_config, *MIS::m_voxels);
					MIS::update_ARAP();
					from_x = mouse_x;
					from_y = mouse_y;
				}
				return true;
			}
		if(m_mouse_mode == TRANSLATE_HANDLE)
			return true;
		else
			return false;
		}
		else
		{
			if(m_mouse_mode == HANDLE_CREATION) {
				if(pick_id_handle(mouse_x,mouse_y,*MIS::m_config,m_fullViewMatrix,m_projMatrix,m_viewport,tmp_id_handle)) {
					tmp_ha_set = false;
				}
				else {
					tmp_ha_set = embree_create_handle(mouse_x,mouse_y,MIS::m_embree,m_fullViewMatrix,m_projMatrix,m_viewport,tmp_handle);
				}
			}
			if(m_mouse_mode == TRANSLATE_HANDLE) 
			{
				if(pick_id_handle(mouse_x,mouse_y,*MIS::m_handles,m_fullViewMatrix,m_projMatrix,m_viewport,tmp_id_handle))
				{

				}
			}
		}
		if(m_mouse_mode == TRANSLATE_HANDLE)
			return true;
		else
			return false;
	}
	
	bool MISPlugin::mouseScrollEvent(int mouse_x, int mouse_y, float delta) {
		return false;
	}

	const static RowVector4 green(0,1,0,1);
	const static RowVector4 red(1,0,0,1);
	const static RowVector4 blue(0,0,1,1);
	const static RowVector4 yellow(1,1,0,1);
	const static RowVector4 white(1,1,1,1);
	const static RowVector4 black(0,0,0,1);

	void MISPlugin::setOpenglLight() {
		float v[4]; 
		glEnable(GL_LIGHT0);
		v[0] = v[1] = v[2] = m_preview->g_LightMultiplier*0.4f; v[3] = 1.0f;
		glLightfv(GL_LIGHT0, GL_AMBIENT, v);
		v[0] = v[1] = v[2] = m_preview->g_LightMultiplier*1.0f; v[3] = 1.0f;
		glLightfv(GL_LIGHT0, GL_DIFFUSE, v);	
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		glMultMatrixd(m_cameraViewMatrix);
		v[0] = - m_preview->g_LightDistance*m_preview->g_LightDirection[0];
		v[1] = - m_preview->g_LightDistance*m_preview->g_LightDirection[1];
		v[2] = - m_preview->g_LightDistance*m_preview->g_LightDirection[2];
		v[3] = 1.0f;
		glLightfv(GL_LIGHT0, GL_POSITION, v);
	}

	void MISPlugin::setOpenglMatrices() {		
		double mat[16];
		glViewport(300, 0, m_preview->width-300, m_preview->height);
		glGetIntegerv(GL_VIEWPORT, m_viewport);
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		double fH = tan( m_preview->view_angle / 360.0 * M_PI ) * m_preview->dnear;
		double fW = fH * (double)(m_preview->width-300)/(double)m_preview->height;
		glFrustum( -fW, fW, -fH, fH, m_preview->dnear, m_preview->dfar);
		glGetDoublev(GL_PROJECTION_MATRIX, m_projMatrix);
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		glFrustum( -fH, fH, -fH, fH, m_preview->dnear, m_preview->dfar);
		glGetDoublev(GL_PROJECTION_MATRIX, m_projSquaredMatrix);
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		gluLookAt(m_preview->eye[0], m_preview->eye[1], m_preview->eye[2], m_preview->center[0], m_preview->center[1], m_preview->center[2], m_preview->up[0], m_preview->up[1], m_preview->up[2]);	
		glScaled(m_preview->g_Zoom, m_preview->g_Zoom, m_preview->g_Zoom);
		glScaled(m_preview->zoom, m_preview->zoom, m_preview->zoom);	
		glTranslated(m_preview->shift[0],m_preview->shift[1],m_preview->shift[2]);
		QuaternionToMatrix44(m_preview->g_Rotation,mat);
		glMultMatrixd(mat);
		glGetDoublev(GL_MODELVIEW_MATRIX, m_cameraViewMatrix);
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		Quaternion q0 = MIS::m_config->getMeshRotation();
		QuaternionToMatrix44(q0,mat);
		glMultMatrixd(mat);
		Matrix33 mat2 = q0.conjugate().toRotationMatrix();
		RowVector3 g = (mat2 * Vector3(0,-1,0));	
		Quaternion q(AngleAxis(m_preview->g_RotationAngle,g.normalized()));
		QuaternionToMatrix44(q,mat);
		glMultMatrixd(mat);
		glTranslated(-m_preview->mesh_center[0],-m_preview->mesh_center[1],-m_preview->mesh_center[2]);
		glGetDoublev(GL_MODELVIEW_MATRIX, m_modelViewMatrix);
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		RowVector3 l_LookAt = m_preview->mesh_center;
		RowVector3 l_Pos = l_LookAt - m_preview->g_LightDistance*RowVector3(m_preview->g_LightDirection[0],m_preview->g_LightDirection[1],m_preview->g_LightDirection[2]);
		gluLookAt(l_Pos[0], l_Pos[1], l_Pos[2], l_LookAt[0], l_LookAt[1], l_LookAt[2], 1, 0, 0);
		glGetDoublev(GL_MODELVIEW_MATRIX, m_lightViewMatrix);
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		glMultMatrixd(m_cameraViewMatrix);
		glMultMatrixd(m_modelViewMatrix);
		glGetDoublev(GL_MODELVIEW_MATRIX, m_fullViewMatrix);
	}

	void MISPlugin::initOpenglMatricesForLight() {
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		glMultMatrixd(m_projMatrix);
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		glMultMatrixd(m_lightViewMatrix);
		glMultMatrixd(m_modelViewMatrix);
	}

	void MISPlugin::initOpenglMatricesForCamera(bool forceSquared) {
		glViewport(300,0,m_preview->width-300,m_preview->height);
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		if(forceSquared) {
			glMultMatrixd(m_projSquaredMatrix);
		}
		else {
			glMultMatrixd(m_projMatrix);
		}
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		glMultMatrixd(m_fullViewMatrix);
	}

	void MISPlugin::initOpenglMatricesForWorld() {
		glViewport(300,0,m_preview->width-300,m_preview->height);
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		glMultMatrixd(m_projMatrix);
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		glMultMatrixd(m_cameraViewMatrix);
	}

	void MISPlugin::resize(int w, int h) {}

	void MISPlugin::preDraw(int currentTime) { 
		if(!MIS::isConfigLoaded()) return;
		log_mouse_mode = m_mouse_mode;	
		if( m_preview->g_AutoRotate ) 
		{
			ScalarType delta_angle = (currentTime-previousTime)/1000.0f;
			m_preview->g_RotationAngle += delta_angle;
			if(m_preview->g_RotationAngle > 2.0*M_PI)
				m_preview->g_RotationAngle -= 2.0*M_PI;		
			Quaternion q(AngleAxis(m_preview->g_RotationAngle,Vector3(0,-1,0)));
		}
		previousTime = currentTime;
		setOpenglMatrices();
		initOpenglMatricesForWorld();
		glGetIntegerv(GL_VIEWPORT, m_preview->m_viewport);
		glGetDoublev(GL_PROJECTION_MATRIX, m_preview->m_projection_matrix);
		glGetDoublev(GL_MODELVIEW_MATRIX, m_preview->m_modelview_matrix);
		glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, m_preview->g_MatAmbient);
		glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, m_preview->g_MatDiffuse);
		glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, m_preview->g_MatSpecular);
		glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, m_preview->g_MatShininess);
		return;
	}


	void MISPlugin::postDraw(int currentTime) {
		if(!MIS::isConfigLoaded()) return;
		if(display_shadow) {
			PCFShadow::beforeShadow();
			initOpenglMatricesForLight();
			if(display_mesh)
			{
				vector<int> idhandles;idhandles.push_back(0);
				glPushMatrix();
				glTranslated(offset,0,0);
				MIS::m_mesh->render(MIS::wire);
	     		glPopMatrix();
			}
			if(MIS::isReady() && display_volume)
			{
				glColor4f(imesh_color[0],imesh_color[1],imesh_color[2],imesh_color[3]);
				glPushMatrix();
				glTranslated(-offset,0,0);
				MIS::m_voxels->renderWire();
				glPopMatrix();
			}
			PCFShadow::afterShadow();
		}
		glEnable(GL_DEPTH_TEST);
		glEnable(GL_NORMALIZE);
		glEnable(GL_LIGHTING);
		glDisable(GL_CULL_FACE);
		glDisable(GL_COLOR_MATERIAL);
		glDepthMask( GL_TRUE );
		glEnable(GL_TEXTURE_2D);
		glPushAttrib(GL_ENABLE_BIT);	
		setOpenglLight();
		initOpenglMatricesForCamera();
		if(display_shadow)
			PCFShadow::beforeRender();
		else 
			glUseProgram(m_preview->shader_id);
		if(display_floor)
		{
			glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, Preview3D::WHITE);
			glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, Preview3D::WHITE);
			glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, Preview3D::BLACK);
			glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, m_preview->g_MatShininess);
			glColor3f(Preview3D::WHITE[0],Preview3D::WHITE[1],Preview3D::WHITE[2]);
			drawPlane();
		}
		glUseProgram(0);
		glEnable(GL_LIGHTING);
		if(MIS::m_mesh->textured() && display_texture)
			glUseProgram(m_preview->shader_txture_id); 
		else
			glUseProgram(m_preview->shader_id);
		glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, m_preview->g_MatAmbient);
		glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, m_preview->g_MatDiffuse);
		glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, m_preview->g_MatSpecular);
		glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, m_preview->g_MatShininess);
		glColor4f(mesh_color[0],mesh_color[1],mesh_color[2],mesh_color[3]);
		if(display_mesh)
		{
			vector<int> idhandles;
			for(int i=0;i<MIS::m_config->getNbHandles();i++)
				idhandles.push_back(i);
			glPushMatrix();
			glTranslated(offset,0,0);
			MIS::m_mesh->render(MIS::wire);
	     	glPopMatrix();
		}
		glUseProgram(0);
		glUseProgram(m_preview->shader_id);
		glColor4f(imesh_color[0],imesh_color[1],imesh_color[2],imesh_color[3]);
		if(MIS::isReady() && display_volume)
		{
			glPushMatrix();
			glTranslated(-offset,0,0);
			MIS::m_voxels->renderWire();
			//MIS::m_voxels->renderLattice();
			MIS::m_voxels->renderAnchor();
			glPopMatrix();
		}
		if(MIS::isReady() && display_handles && tmp_id_handle!=-1)
		{
			const RowVector3 & a = MIS::m_handles->getHandlePose(tmp_id_handle);
			glPushMatrix();
				glTranslated(a[0]-offset,a[1],a[2]);
				double mat[16];
				QuaternionToMatrix44(MIS::m_config->getMeshRotation().inverse(),mat);
				glMultMatrixd(mat);
				if(m_mouse_mode == TRANSLATE_HANDLE)
					paint3DFrame(0.15);
			glPopMatrix();
		}
		if(display_text)
		{
			glUseProgram(0);
			drawText();
		}
		glUseProgram(0);
		glPopAttrib();
		if(recording) takeScreenshot();
		return;
	}
	
	void MISPlugin::resetCamera() {
		m_preview->g_Rotation.setIdentity();
	}

void MISPlugin::close() {
	if(!MIS::isConfigLoaded()) return;
	TwDefine(" VoxARAP/meshFile readwrite ");;
	resetVars();
	MIS::uninitialize();
}
	void MISPlugin::get_translation(const RowVector3 & pos0, int mouse_x, int mouse_y, int from_x, int from_y, RowVector3 & translation)
	{
		GLdouble winX, winY, winZ;
		gluProject(pos0[0],pos0[1],pos0[2],m_fullViewMatrix,m_projMatrix,m_viewport, &winX, &winY, &winZ);

		double x,y,z;
		RowVector3 p1, p2;
		winX = from_x;
		winY = m_viewport[3] - from_y;
		gluUnProject(winX, winY, winZ, m_fullViewMatrix,m_projMatrix,m_viewport, &x,&y,&z);
		p1 << x,y,z;
		winX = mouse_x;
		winY = m_viewport[3] - mouse_y;
		gluUnProject(winX, winY, winZ, m_fullViewMatrix,m_projMatrix,m_viewport, &x,&y,&z);
		p2 << x,y,z;

		translation = p2-p1;
	}
void MISPlugin::takeScreenshot()
{
	static int imageID = 0;
	std::stringstream s;
	s  << "screenshot/"<<imageID << ".bmp";
	SOIL_save_screenshot
	(
		s.str().c_str(),
		SOIL_SAVE_TYPE_BMP,
		300, 0, glutGet(GLUT_WINDOW_WIDTH)-300, glutGet(GLUT_WINDOW_HEIGHT)
	);
	cout <<"saving a screenshot ... "<<"screenshot/"<<imageID<<".bmp"<<endl;
	imageID++;
}
