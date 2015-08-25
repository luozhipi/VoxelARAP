#ifndef __MIS_PLUGIN_H
#define __MIS_PLUGIN_H
#include "PreviewPlugin.h"
#include "PluginManager.h"
#include "common.h"

enum MouseMode { NOTHING, TRANSLATE_HANDLE, HANDLE_CREATION};

class MISPlugin : public PreviewPlugin
{
public:
	MISPlugin()
	{
		CreatePluginManager();
		PluginManager().register_plugin(this);
		cfg_meshfile = 0;
		resetVars();
	}

	void resetVars() {
		display_mesh = true;
		recording = false;
		display_texture = false;
		display_volume = true;
		display_handles = true;
		m_mouse_mode = NOTHING;
		mouse_down = false;
		deforming = false;
	    tmp_id_handle = -1;
		display_floor = false;
		display_shadow = false;
		display_text = true;
		plane_offset = 0.0;
		offset = 0.0;
	}

	~MISPlugin() {
	}
	void init(Preview3D *preview);
	void updateAfterConfigLoading();
	void updateAfterVoxelLoading();

	bool keyDownEvent(unsigned char key, int modifiers, int mouse_x, int mouse_y);
	
	bool keyUpEvent(unsigned char key, int modifiers, int mouse_x, int mouse_y);

	bool mouseDownEvent(int mouse_x, int mouse_y, int button, int modifiers);
	
	bool mouseUpEvent(int mouse_x, int mouse_y, int button, int modifiers);
	
	bool mouseMoveEvent(int mouse_x, int mouse_y);
	
	bool mouseScrollEvent(int mouse_x, int mouse_y, float delta);

	void get_translation(const RowVector3 & pos0, int mouse_x, int mouse_y, int from_x, int from_y, RowVector3 & translation);

	void preDraw(int currentTime);
	void postDraw(int currentTime);
	void resize(int w, int h);
	void drawPlane();
	void resetCamera();
	void close();
	void takeScreenshot();

	void setOpenglLight();
	void setOpenglMatrices();
	void initOpenglMatricesForLight();
	void initOpenglMatricesForCamera(bool forceSquared = false);
	void initOpenglMatricesForWorld();

	string getMeshfile() const { return objFiles[cfg_meshfile]; }
	int cfg_meshfile;
	int log_nbV;
	int log_nbF;
	int log_nbN;
	int log_nbC;
	int log_mouse_mode;
	int previousTime;
	float offset;
protected:

	MouseMode m_mouse_mode;
	bool mouse_down;
	bool deforming;
	int tmp_id_handle;RowVector3 tmp_handle;bool tmp_ha_set;
	int from_x, from_y;

	RowVector3 translation;
	RowVector3 translation_down;

	TwBar *mybar;
	TwBar *stats;
	Preview3D::KeyModifier m_key_modifier;
	float media_exponent;
	float media_density;
	float media_color[3];
	float imesh_color[4];
	float mesh_color[4];
	int m_viewport[4];
	double m_projMatrix[16];
	double m_projSquaredMatrix[16];
	double m_cameraViewMatrix[16];
	double m_lightViewMatrix[16];
	double m_modelViewMatrix[16];
	double m_fullViewMatrix[16];
	vector<string> objFiles;

	float plane_offset;
	bool display_mesh;
	bool recording;
	bool display_texture;
	bool display_volume;
	bool display_handles;
	bool display_floor;
	bool display_shadow;
	bool display_text;



	void drawPerText(const char * text, int len, int x, int y)
	{
		glMatrixMode(GL_PROJECTION); 
		 double matrix[16]; 
		 glGetDoublev(GL_PROJECTION_MATRIX, matrix); 
		 glLoadIdentity(); 
		 glOrtho(-300, 300, -300, 300, -5, 5); 
		 glMatrixMode(GL_MODELVIEW); 
		 glLoadIdentity();
		 glPushMatrix(); 
		 glLoadIdentity(); 
		 glRasterPos2i(x, y); 
		 for(int i=0; i<len; i++){
			 glutBitmapCharacter(GLUT_BITMAP_TIMES_ROMAN_24, (int)text[i]);
		 }
		 glPopMatrix(); 
		 glMatrixMode(GL_PROJECTION); 
		 glLoadMatrixd(matrix);
		 glMatrixMode(GL_MODELVIEW);
	}
	void drawText()
	{
		static int timeBase = 0;
		int t = glutGet(GLUT_ELAPSED_TIME);
		if (t - timeBase > 500) {
			timeBase = t;		
		}
		glDisable(GL_LIGHTING);
		glDisable(GL_DEPTH_TEST);
		glDisable(GL_TEXTURE_2D);
		glColor3f(1, 0, 0);
		string voxelmethodstr = "volumetric ARAP";
		string skinmethodstr;
		switch(MIS::embed_mode)
		{
		case 0:
			skinmethodstr = "trilinear";
			break;
		case 1:
			skinmethodstr = "linear blending";
			break;
		case 2:
			skinmethodstr = "rbf";
			break;
		}
		stringstream ss_voxel;
		ss_voxel <<voxelmethodstr << " " << MIS::time_voxel<< " s";
		string str_voxel = ss_voxel.str();
		stringstream ss_skin;
		ss_skin <<ss_voxel.str()<<" vertex blending: "<<skinmethodstr << " " << MIS::time_skin<< " s  ";
		string str_skin = ss_skin.str();
		drawPerText(str_skin.data(), str_skin.size(), -250, 280);
		glColor3f(0, 1, 0);
		stringstream ss_volume;
		ss_volume << "relative volume " << MIS::volume<< "%";//<<" loss of volume "<<MIS::loss;
		string str_volume = ss_volume.str();
		drawPerText(str_volume.data(), str_volume.size(), -250, 250);
		glEnable(GL_LIGHTING);
		glEnable(GL_DEPTH_TEST);
		glEnable(GL_TEXTURE_2D);
	}
};

#endif
