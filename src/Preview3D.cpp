//
//  plugin.cpp
// Created by Olga Diamanti on 9/21/11.
//  modified by Zhiping Luo on 3/3/14.
//  Copyright 2011 ETH Zurich. All rights reserved.

#include "Preview3D.h"
#include "trackball.h"
#include "shaders.h"

using namespace std;

// Undef Visual Studio macros...
#undef max
#undef min

#include <omp.h>

#include "PluginManager.h"
#include "utils_functions.h"
#include "plugin.h"
#include "PCFShadow.h"

float Preview3D::ZERO[] = {0.0f,0.0f,0.0f,0.0f};

float Preview3D::DEFAULT_LIGHT_DIRECTION[] = {0.04,0.16,-0.99};//{ 0.0f, -0.80f, -0.60f};

float Preview3D::MIS_AMBIENT[4] = { 102.0f/255.0f,61.0f/255.0f,199.0f/255.0f,1.0f };
float Preview3D::MIS_DIFFUSE[4] = { 255.0f/255.0f,228.0f/255.0f,58.0f/255.0f,0.5f };
float Preview3D::MIS_SPECULAR[4] = { 106.0f/255.0f,94.0f/255.0f,0.0f/255.0f,1.0f };
float Preview3D::MIS_SHININESS = 16.0f;

float Preview3D::BLACK[4] = {0.0f,0.0f,0.0f,1.0f};
float Preview3D::WHITE[4] = {1.0f,1.0f,1.0f,1.0f};

Preview3D::Preview3D(int start_time)
{
	display_bars = true;
	corner_threshold = 20;
	frustum_shift_x = 0.125;
	width_percentage = 1.;
	height_percentage = 1.;
	view_angle = 45.0;
	dnear = 0.01;
	dfar = 1000.0;
	g_Zoom = 1.0f;
	zoom = 1.0f;
	shift.setZero();
	mesh_center.setZero();
	g_Rotation.setIdentity();
	g_AutoRotate = 0;
	g_RotateTime = 0;
	g_RotationAngle = 0;
	Preview3D::CopyArray4(MIS_AMBIENT,g_MatAmbient);
	Preview3D::CopyArray4(MIS_DIFFUSE,g_MatDiffuse);
	Preview3D::CopyArray4(MIS_SPECULAR,g_MatSpecular);
	g_MatShininess = MIS_SHININESS;
	g_LightMultiplier = 1.0f;
	g_LightDistance = 5.0f;
	Preview3D::CopyArray3(DEFAULT_LIGHT_DIRECTION,g_LightDirection);
	cout << "LOADING MESH SHADER..." << endl;
	s_directionalPerPixelProgram = loadShaderProgram("shaders/directionalperpixel.vert", "shaders/directionalperpixel.frag");
	shader_id = s_directionalPerPixelProgram.p;
	s_directionalPerPixelTextureProgram = loadShaderProgram("shaders/textureddirectionalperpixel.vert", "shaders/textureddirectionalperpixel.frag");
	shader_txture_id = s_directionalPerPixelTextureProgram.p;
	cout<< "\t [done]" << endl;
	cout << "LOADING SHADOW SHADER..." << endl;
	PCFShadow::init();
	cout<< "\t [done]" << endl;
	// initialize scroll position to 0
	scroll_position = 0.0f;
	down = false;
	// Initialize AntTweakBar
	if( !TwInit(TW_OPENGL, NULL) )
	{
		// A fatal error occured    
		fprintf(stderr, "AntTweakBar initialization failed: %s\n", TwGetLastError());
	}

	// Create a tweak bar
	bar = TwNewBar("TweakBar");
	TwDefine(" TweakBar size='280 400' text=light alpha=255 color='40 40 40' position='3 300' valueswidth=100 visible=true "); // change default tweak bar size and color

	// Add 'g_Zoom' to 'bar': this is a modifable (RW) variable of type TW_TYPE_FLOAT. Its key shortcuts are [z] and [Z].
	TwAddVarRW(bar, "Zoom", TW_TYPE_FLOAT, &g_Zoom, 
		" min=0.1 max=50 step=0.1 keyIncr=+ keyDecr=- help='Scale the object (1=original size).' ");
	// Add 'g_Rotation' to 'bar': this is a variable of type TW_TYPE_QUAT4F which defines the object's orientation
	TwAddVarCB(bar, "ObjRotation", TW_TYPE_QUAT4F, SetObjRotationCB, GetObjRotationCB, this, 
		" label='Object rotation' help='Change the object orientation.' ");
	// Add callback to toggle auto-rotate mode (callback functions are defined above).
	TwAddVarCB(bar, "AutoRotate", TW_TYPE_BOOLCPP, SetAutoRotateCB, GetAutoRotateCB,this,
		" label='Auto-rotate' help='Toggle auto-rotate mode.' ");
	// Add 'g_LightMultiplier' to 'bar': this is a variable of type TW_TYPE_FLOAT. Its key shortcuts are [+] and [-].
	TwAddVarRW(bar, "Multiplier", TW_TYPE_FLOAT, &g_LightMultiplier, 
	" label='Light booster' min=0.1 max=100 step=0.02 help='Increase/decrease the light power.' ");
	TwAddVarRW(bar, "LDistance", TW_TYPE_FLOAT, &g_LightDistance, 
	" label='Light distance' min=0.1 max=1000 step=0.1 help='Increase/decrease the light distance.' ");
	// Add 'g_LightDirection' to 'bar': this is a variable of type TW_TYPE_DIR3F which defines the light direction
	TwAddVarRW(bar, "LightDir", TW_TYPE_DIR3F, &g_LightDirection, 
	" label='Light direction' open help='Change the light direction.' ");

	TwAddVarRW(bar, "Ambient", TW_TYPE_COLOR4F, &g_MatAmbient, " group='Material' ");
	TwAddVarRW(bar, "Diffuse", TW_TYPE_COLOR4F, &g_MatDiffuse, " group='Material' ");
	TwAddVarRW(bar, "Specular", TW_TYPE_COLOR4F, &g_MatSpecular, " group='Material' ");	
	TwAddVarRW(bar,"Shininess",TW_TYPE_FLOAT,&g_MatShininess," group='Material' min=0 max=128");

	g_RotateTime = 0;
	g_Rotation.setIdentity();
	g_RotationAngle = 0;
	for (unsigned int i = 0; i<PluginManager().plugin_list_.size(); ++i)
		PluginManager().plugin_list_[i]->init(this);
}

Preview3D::~Preview3D()
{
	DeletePluginManager();
}

bool Preview3D::key_down(unsigned char key, int modifiers, int mouse_x, int mouse_y)
{
	for (unsigned int i = 0; i <PluginManager().plugin_list_.size(); ++i)
		if (PluginManager().plugin_list_[i]->keyDownEvent(key, modifiers, mouse_x, mouse_y))
			return true;

	return false;
}

bool Preview3D::key_up(unsigned char /*key*/, int /*modifiers*/, int /*mouse_x*/, int /*mouse_y*/)
{
	return false;
}

bool Preview3D::mouse_down(int mouse_x, 
	int mouse_y,
	int button,
	int modifiers)
{

	down = true;

	switch(button) 
	{
	case GLUT_LEFT_BUTTON :
		{
			TwMouseMotion(mouse_x,mouse_y);

			// First pass to AntTweakBar
			if( !TwMouseButton(TW_MOUSE_PRESSED, TW_MOUSE_LEFT) )
			{
				TwMouseButton(TW_MOUSE_RELEASED, TW_MOUSE_LEFT);

				for (unsigned int i = 0; i <PluginManager().plugin_list_.size(); ++i)
					if (PluginManager().plugin_list_[i]->mouseDownEvent(mouse_x, mouse_y, button, modifiers))
						return true;


				if (modifiers == NO_KEY)
				{
					//init track ball
					down_x = mouse_x;
					down_y = mouse_y;
					down_rotation = g_Rotation;
					mouse_mode = ROTATION;
				}

			}
			break;
		}    
	case GLUT_RIGHT_BUTTON :
		{
			TwMouseMotion(mouse_x,mouse_y);

			// First pass to AntTweakBar
			if( !TwMouseButton(TW_MOUSE_PRESSED, TW_MOUSE_RIGHT) )
			{
				TwMouseButton(TW_MOUSE_RELEASED, TW_MOUSE_RIGHT);
				for (unsigned int i = 0; i <PluginManager().plugin_list_.size(); ++i)
					if (PluginManager().plugin_list_[i]->mouseDownEvent(mouse_x, mouse_y, button, modifiers))
						return true;

				if (modifiers == NO_KEY)
				{
					down_x = mouse_x;
					down_y = mouse_y;
					down_translation[0] = shift[0];
					down_translation[1] = shift[1];
					down_translation[2] = shift[2];
					mouse_mode = TRANSLATE;
				}
			}
			break;
		}


	}  
	return true;
}

bool Preview3D::mouse_up(int mouse_x, 
	int mouse_y,
	int button,
	int modifiers)
{
	down = false;
	for (unsigned int i = 0; i <PluginManager().plugin_list_.size(); ++i)
		if (PluginManager().plugin_list_[i]->mouseUpEvent(mouse_x, mouse_y, button, modifiers))
			return true;

	mouse_mode = NOTHING;

	switch(button) {
	case GLUT_LEFT_BUTTON :
		// First pass to AntTweakBar
		if( !TwMouseButton(TW_MOUSE_RELEASED, TW_MOUSE_LEFT) )
		{
		}
		break;
	case GLUT_RIGHT_BUTTON :
		// First pass to AntTweakBar
		if( !TwMouseButton(TW_MOUSE_RELEASED, TW_MOUSE_RIGHT) )
		{
		}
		break;
	}
	return true;
}

bool Preview3D::mouse_move(int mouse_x, int mouse_y)
{

	for (unsigned int i = 0; i <PluginManager().plugin_list_.size(); ++i)
		if (PluginManager().plugin_list_[i]->mouseMoveEvent(mouse_x, mouse_y))
			return true;

	if(down)
	{

		switch(mouse_mode) {
		case ROTATION :
			{
				
				float q1[4];
				q1[0] = down_rotation.x();
				q1[1] = down_rotation.y();
				q1[2] = down_rotation.z();
				q1[3] = down_rotation.w();

				double origin_x, origin_y, origin_z;
				gluProject(
					0,0,0,
					m_modelview_matrix, m_projection_matrix,
					m_viewport, &origin_x, &origin_y, &origin_z);

				float q2[4];
				float center_x=0., center_y=0., half_width=0., half_height=0.;
				// Trackball centered at object's centroid
				center_x = (float)origin_x;
				center_y = (float)origin_y;

				half_width =  ((float)(m_viewport[2]))/4.f;
				half_height = ((float)(m_viewport[3]))/4.f;

				trackball(q2,
					(center_x-down_x)/half_width,
					(down_y-center_y)/half_height,
					(center_x-mouse_x)/half_width,
					(mouse_y-center_y)/half_height);
				// I think we need to do this because we have z pointing out of the
				// screen rather than into the screen
				q2[2] = -q2[2];
				
				float q3[4];
				add_quats(q1,q2,q3);
				g_Rotation = Quaternion(q3[3],q3[0],q3[1],q3[2]);

				break;
			}

		case TRANSLATE:
			{
				glMatrixMode(GL_MODELVIEW);
				glPushMatrix();

				glLoadIdentity();
	
				gluLookAt(eye[0], eye[1], eye[2], center[0], center[1], center[2], up[0], up[1], up[2]);
				glScaled(g_Zoom, g_Zoom, g_Zoom);
				glScaled(zoom, zoom, zoom);
				
				double mat[16];
				glGetDoublev(GL_MODELVIEW_MATRIX,mat);

				glPopMatrix();

				//translation
				GLfloat winX, winY;               // Holds Our X and Y Coordinates
				winX = (float)mouse_x;                  // Holds The Mouse X Coordinate
				winY = (float)mouse_y;
				winY = (float)m_viewport[3] - winY;
				double pos1[3];
				gluUnProject(winX, winY, 0.5, mat, m_projection_matrix, m_viewport, &(pos1[0]),&(pos1[1]),&(pos1[2]));


				winX = (float)down_x;                  // Holds The Mouse X Coordinate
				winY = (float)down_y;
				winY = (float)m_viewport[3] - winY;
				double pos0[3];
				gluUnProject(winX, winY, 0.5, mat, m_projection_matrix, m_viewport, &(pos0[0]),&(pos0[1]),&(pos0[2]));

				shift[0] = down_translation[0] + (float)(pos1[0] - pos0[0]);
				shift[1] = down_translation[1] + (float)(pos1[1] - pos0[1]);
				shift[2] = down_translation[2] + (float)(pos1[2] - pos0[2]);


				break;
			}

		default:
			break;
		}
	}
	if(!TwMouseMotion(mouse_x,mouse_y))
	{
	}
	return true;
}

bool Preview3D::mouse_scroll(int mouse_x, int mouse_y, float delta_y)
{
	scroll_position += delta_y;
	bool in_bar = (TwMouseMotion(mouse_x,mouse_y) != 0);
	//cout << delta_y << endl;
	if(!TwMouseWheel((int)scroll_position) && !in_bar)
	{
		//cout << delta_y << endl;
		for (unsigned int i = 0; i <PluginManager().plugin_list_.size(); ++i)
			if (PluginManager().plugin_list_[i]->mouseScrollEvent(mouse_x, mouse_y, delta_y))
				return true;
		float factor = 0.05f;
		g_Zoom = (g_Zoom + delta_y*factor > 0.1f ? g_Zoom + delta_y*factor : 0.1f);
		return true;
	}
	return true;
}

void Preview3D::draw(int current_time)
{
	// Clear frame buffer
	glClearColor(1.0f,1.0f,1.0f,0.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	for (unsigned int i = 0; i <PluginManager().plugin_list_.size(); ++i)
		PluginManager().plugin_list_[i]->preDraw(current_time);

	for (unsigned int i = 0; i <PluginManager().plugin_list_.size(); ++i)
		PluginManager().plugin_list_[i]->postDraw(current_time);

	if(display_bars) TwDraw();
}

void Preview3D::resize(int w, int h)
{
	width = (int)(width_percentage*w);
	height = (int)(height_percentage*h);
	m_viewport[2] = width;
	m_viewport[3] = height;

	eye[0] = 0;
	eye[1] = 0;
	eye[2] = 5;

	center[0] = 0;
	center[1] = 0;
	center[2] = 0;

	up[0] = 0;
	up[1] = 1;
	up[2] = 0;
	
	sight_distance = (center-eye).norm();
	line_of_sight = (center-eye).normalized();
	// Send the new window size to AntTweakBar
	TwWindowSize(width, height);

	
	for (unsigned int i = 0; i <PluginManager().plugin_list_.size(); ++i)
		PluginManager().plugin_list_[i]->resize(w,h);
}


bool Preview3D::GetAutoRotate() {
	return g_AutoRotate;
}
void Preview3D::SetAutoRotate(bool value)
{
	g_AutoRotate = value; // copy value to g_AutoRotate
	if( g_AutoRotate!=0 ) 
	{
		g_RotateTime = glutGet(GLUT_ELAPSED_TIME);
	}
}


//  Callback function called when the 'AutoRotate' variable value of the tweak bar has changed
void TW_CALL Preview3D::SetAutoRotateCB(const void *value, void *clientData)
{
	static_cast<Preview3D *>(clientData)->SetAutoRotate(*static_cast<const bool *>(value));
}
void TW_CALL Preview3D::GetAutoRotateCB(void *value, void *clientData)
{
	*static_cast<bool *>(value) = static_cast<Preview3D*>(clientData)->GetAutoRotate();
}

void TW_CALL Preview3D::SetObjRotationCB(const void *value, void *clientData)
{ 
    const float* fvalue = (const float *)value;
	static_cast<Preview3D *>(clientData)->setObjRotation(Quaternion(fvalue[3],fvalue[0],fvalue[1],fvalue[2]));
}
void TW_CALL Preview3D::GetObjRotationCB(void *value, void *clientData)
{
	const Quaternion & quat = static_cast<Preview3D *>(clientData)->getObjRotation();
	((float *)value)[0] = quat.x();
    ((float *)value)[1] = quat.y();
    ((float *)value)[2] = quat.z();
    ((float *)value)[3] = quat.w();
}
