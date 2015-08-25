// Main class of the Viewer

#ifndef Preview3D_h
#define Preview3D_h

#include "EIGEN_inc.h"
#include "GL_inc.h"
#include "shaders.h"

#include <AntTweakBar.h>

////////////////////////////////////////////////////////////////////////////////
// Preview3D Class
////////////////////////////////////////////////////////////////////////////////

class Preview3D
{
	// The plugin class must be friend to be able to access the private fields of Preview3D
	friend class PreviewPlugin;

public:
	/********* Display Options *********/
	
	TwBar *bar; // Pointer to the tweak bar

	bool display_bars;
	double corner_threshold;
	bool g_AutoRotate;
	float radius;
	// Light parameter
	float g_LightMultiplier;
	float g_LightDistance;
	float g_LightDirection[3];
	// Shapes material
	float g_MatAmbient[4];
	float g_MatDiffuse[4];
	float g_MatSpecular[4];
	float g_MatShininess;
	// Shaders
	struct GLSL_Program s_directionalPerPixelProgram;
	struct GLSL_Program s_directionalPerPixelTextureProgram;
	struct GLSL_Program s_directionalPerPixelColorProgram;
	int shader_id;
	int shader_txture_id;
	/***********************************/


	/********* Scene/Camera Options *********/
	RowVector3 eye;
	RowVector3 up;
	RowVector3 center;
	RowVector3 line_of_sight;
	float sight_distance;
	// Scene frustum shift
	double frustum_shift_x;
	// Scene scale
	float g_Zoom;
	// zoom and shift are set once when mesh is loaded to center and normalize
	// shape to fit unit box
	float zoom;
	RowVector3 shift; 
	RowVector3 mesh_center;
	double view_angle;
	double dnear;
	double dfar;
	double m_projection_matrix[16];
	double m_modelview_matrix[16];
	GLint m_viewport[4];
	Quaternion g_Rotation;
	/***********************************/


	/********* Window Options *********/
	double width_percentage;
	double height_percentage;
	int width;
	int height;
	/***********************************/


	/********* Other Variables *********/
	// number of frames per second
	double fps;
	float down_translation[3];

	enum MouseMode { NOTHING, ROTATION, ZOOM, PAN, TRANSLATE, SELECT } mouse_mode;
	enum KeyModifier { NO_KEY = TW_KMOD_NONE, SHIFT = TW_KMOD_SHIFT, CTRL =TW_KMOD_CTRL, ALT = TW_KMOD_ALT } key_modifier;

	// Auto rotate
	int g_RotateTime;
	ScalarType g_RotationAngle;
	bool down;
	int last_x;
	int last_y;
	int down_x;
	int down_y;
	float scroll_position;
	Quaternion down_rotation;
	/***********************************/


	/********* Static Variables ***************/
	static float GOLD_DIFFUSE[4];
	static float BLACK[4];
	static float WHITE[4];
	
	static float MIS_AMBIENT[4];
	static float MIS_DIFFUSE[4];
	static float MIS_SPECULAR[4];
	static float MIS_SHININESS;
	
	static float ZERO[];

	static float DEFAULT_LIGHT_DIRECTION[];

	/******************************************/


	////////////////////////////////////////////////////////////////////////////
	// Functions
	////////////////////////////////////////////////////////////////////////////

	/********* Initialization *********/
	Preview3D(int current_time);
	~Preview3D();


	/********* Handle key/mouse events*********/
	bool key_down(unsigned char key, int modifier, int mouse_x, int mouse_y);
	bool key_up(unsigned char key, int modifier, int mouse_x, int mouse_y);
	bool mouse_down(int mouse_x, int mouse_y, int button, int key_pressed);
	bool mouse_up(int mouse_x, int mouse_y, int button, int key_pressed);
	bool mouse_move(int mouse_x, int mouse_y);
	bool mouse_scroll(int mouse_x, int mouse_y, float delta_y);

	/********* Compiling / Drawing *********/
	void draw(int current_time);
	// OpenGL context resize
	void resize(int w, int h);

	/********* Setters/Getters *********/
	void SetAutoRotate(bool value);
	bool GetAutoRotate();
	
	const Quaternion & getObjRotation() const { return g_Rotation; }
	void setObjRotation(const Quaternion & q) { g_Rotation = q; }

	/********* Math Helpers (used to convert anttweakbar trackball to opengl matrices)*********/
	static void CopyArray3(float a[], float b[3])
	{
		b[0] = a[0];
		b[1] = a[1];
		b[2] = a[2];
	};
	static void CopyArray4(float a[], float b[4])
	{
		b[0] = a[0];
		b[1] = a[1];
		b[2] = a[2];
		b[3] = a[3];
	};
	
	/********* AntTweakBar callbacks *********/
	static void TW_CALL SetAutoRotateCB(const void *value, void *clientData);
	static void TW_CALL GetAutoRotateCB(void *value, void *clientData);
	
	static void TW_CALL SetObjRotationCB(const void *value, void *clientData);
	static void TW_CALL GetObjRotationCB(void *value, void *clientData);

};

#endif