#include "PCFShadow.h"

#ifdef _WIN32
#include "windows.h"
#endif

#include "GL/glew.h"
#include "GL/glut.h"

#include <cstdio>


// Expressed as float so gluPerspective division returns a float and not 0 (640/480 != 640.0/480.0).
#define RENDER_WIDTH 1280.0
#define RENDER_HEIGHT 720.0
#define SHADOW_MAP_RATIO 4

// Hold id of the framebuffer for light POV rendering
static GLuint fboId;

// Z values will be rendered to this texture when using fboId framebuffer
static GLuint depthTextureId;

// Use to activate/disable shadowShader
static GLhandleARB shadowShaderId;

static GLuint shadowMapUniform;
static GLuint shadowMapStepXUniform;
static GLuint shadowMapStepYUniform;


// Loading shader function
static GLhandleARB loadShader(char* filename, unsigned int type)
{
	FILE *pfile;
	GLhandleARB handle;
	const GLcharARB* files[1];
	
	// shader Compilation variable
	GLint result;				// Compilation code result
	GLint errorLoglength ;
	char* errorLogText;
	GLsizei actualErrorLogLength;
	
	char buffer[400000];
	memset(buffer,0,400000);
	
	// This will raise a warning on MS compiler
	pfile = fopen(filename, "rb");
	if(!pfile)
	{
		printf("Sorry, can't open file: '%s'.\n", filename);
		exit(0);
	}
	
	fread(buffer,sizeof(char),400000,pfile);
	//printf("%s\n",buffer);
	
	
	fclose(pfile);
	
	handle = glCreateShaderObjectARB(type);
	if (!handle)
	{
		//We have failed creating the vertex shader object.
		printf("Failed creating vertex shader object from file: %s.",filename);
		exit(0);
	}
	
	files[0] = (const GLcharARB*)buffer;
	glShaderSourceARB(
					  handle, //The handle to our shader
					  1, //The number of files.
					  files, //An array of const char * data, which represents the source code of theshaders
					  NULL);
	
	glCompileShaderARB(handle);
	
	//Compilation checking.
	glGetObjectParameterivARB(handle, GL_OBJECT_COMPILE_STATUS_ARB, &result);
	
	// If an error was detected.
	if (!result)
	{
		//We failed to compile.
		printf("Shader '%s' failed compilation.\n",filename);
		
		//Attempt to get the length of our error log.
		glGetObjectParameterivARB(handle, GL_OBJECT_INFO_LOG_LENGTH_ARB, &errorLoglength);
		
		//Create a buffer to read compilation error message
		errorLogText = (char*)malloc(sizeof(char) * errorLoglength);
		
		//Used to get the final length of the log.
		glGetInfoLogARB(handle, errorLoglength, &actualErrorLogLength, errorLogText);
		
		// Display errors.
		printf("%s\n",errorLogText);
		
		// Free the buffer malloced earlier
		free(errorLogText);
	}
	
	return handle;
}

static void loadShadowShader()
{
	GLhandleARB vertexShaderHandle;
	GLhandleARB fragmentShaderHandle;
	
	vertexShaderHandle   = loadShader("shaders/PCFShadow.vert",GL_VERTEX_SHADER);
	fragmentShaderHandle = loadShader("shaders/PCFShadow.frag",GL_FRAGMENT_SHADER);
	
	shadowShaderId = glCreateProgramObjectARB();
	
	glAttachObjectARB(shadowShaderId,vertexShaderHandle);
	glAttachObjectARB(shadowShaderId,fragmentShaderHandle);
	glLinkProgramARB(shadowShaderId);
	
	shadowMapUniform = glGetUniformLocationARB(shadowShaderId,"ShadowMap");
	shadowMapStepXUniform = glGetUniformLocationARB(shadowShaderId,"xPixelOffset");
	shadowMapStepYUniform = glGetUniformLocationARB(shadowShaderId,"yPixelOffset");
}

static void generateShadowFBO()
{
	int shadowMapWidth = RENDER_WIDTH * SHADOW_MAP_RATIO;
	int shadowMapHeight = RENDER_HEIGHT * SHADOW_MAP_RATIO;
	
	//GLfloat borderColor[4] = {0,0,0,0};
	
	GLenum FBOstatus;
	
	// Try to use a texture depth component
	glGenTextures(1, &depthTextureId);
	glBindTexture(GL_TEXTURE_2D, depthTextureId);
	
	// GL_LINEAR does not make sense for depth texture. However, next tutorial shows usage of GL_LINEAR and PCF. Using GL_NEAREST
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	
	// Remove artefact on the edges of the shadowmap
	glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP );
	glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP );
	
	// This is to allow usage of shadow2DProj function in the shader
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_R_TO_TEXTURE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
	glTexParameteri(GL_TEXTURE_2D, GL_DEPTH_TEXTURE_MODE, GL_INTENSITY); 
	
	
	
	// No need to force GL_DEPTH_COMPONENT24, drivers usually give you the max precision if available 
	glTexImage2D( GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, shadowMapWidth, shadowMapHeight, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_BYTE, 0);
	glBindTexture(GL_TEXTURE_2D, 0);
	
	// create a framebuffer object
	glGenFramebuffersEXT(1, &fboId);
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, fboId);
	
	// Instruct openGL that we won't bind a color texture with the currently binded FBO
	glDrawBuffer(GL_NONE);
	glReadBuffer(GL_NONE);
	
	// attach the texture to FBO depth attachment point
	glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT,GL_TEXTURE_2D, depthTextureId, 0);
	
	// check FBO status
	FBOstatus = glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT);
	if(FBOstatus != GL_FRAMEBUFFER_COMPLETE_EXT)
		printf("GL_FRAMEBUFFER_COMPLETE_EXT failed, CANNOT use FBO\n");
	
	// switch back to window-system-provided framebuffer
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
}

static void setTextureMatrix(void)
{
	static double modelView[16];
	static double projection[16];
	
	// This is matrix transform every coordinate x,y,z
	// x = x* 0.5 + 0.5 
	// y = y* 0.5 + 0.5 
	// z = z* 0.5 + 0.5 
	// Moving from unit cube [-1,1] to [0,1]  
	const GLdouble bias[16] = {	
		0.5, 0.0, 0.0, 0, 
		0.0, 0.5, 0.0, 0,
		0.0, 0.0, 0.5, 0,
		0.5, 0.5, 0.5, 1.0};
	
	
	// Grab modelview and transformation matrices
	glGetDoublev(GL_MODELVIEW_MATRIX, modelView);
	glGetDoublev(GL_PROJECTION_MATRIX, projection);
	
	
	glMatrixMode(GL_TEXTURE);
	glActiveTextureARB(GL_TEXTURE7);
	
	glLoadIdentity();	
	glLoadMatrixd(bias);
	
	// concatating all matrice into one.
	glMultMatrixd (projection);
	glMultMatrixd (modelView);
	
	// Go back to normal matrix mode
	glMatrixMode(GL_MODELVIEW);
}


void PCFShadow::beforeShadow() 
{
	//First step: Render from the light POV to a FBO, story depth values only
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT,fboId);	//Rendering offscreen
	
	//Using the fixed pipeline to render to the depthbuffer
	glUseProgramObjectARB(0);
	
	// In the case we render the shadowmap to a higher resolution, the viewport must be modified accordingly.
	glViewport(0,0,RENDER_WIDTH * SHADOW_MAP_RATIO,RENDER_HEIGHT* SHADOW_MAP_RATIO);
	
	// Clear previous frame values
	glClear( GL_DEPTH_BUFFER_BIT);
	
	//Disable color rendering, we only want to write to the Z-Buffer
	glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE); 
	
	// Culling switching, rendering only backface, this is done to avoid self-shadowing
	glCullFace(GL_FRONT);
}

void PCFShadow::afterShadow()
{	
	//Save modelview/projection matrice into texture7, also add a biais
	setTextureMatrix();
}

void PCFShadow::beforeRender()
{	
	// Now rendering from the camera POV, using the FBO to generate shadows
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT,0);
	
	//glViewport(0,0,RENDER_WIDTH,RENDER_HEIGHT);
	
	//Enabling color write (previously disabled for light POV z-buffer rendering)
	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE); 
	
	// Clear previous frame values
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	//Using the shadow shader
	glUseProgramObjectARB(shadowShaderId);
	glUniform1iARB(shadowMapUniform,7);
	glUniform1fARB(shadowMapStepXUniform,1.0/ (RENDER_WIDTH * SHADOW_MAP_RATIO));
	glUniform1fARB(shadowMapStepYUniform,1.0/ (RENDER_HEIGHT * SHADOW_MAP_RATIO));
	glActiveTextureARB(GL_TEXTURE7);
	glBindTexture(GL_TEXTURE_2D,depthTextureId);
	
	glDisable(GL_CULL_FACE);

}

void PCFShadow::init()
{
	generateShadowFBO();
	loadShadowShader();
	
	// This is important, if not here, FBO's depthbuffer won't be populated.
	glEnable(GL_DEPTH_TEST);
	glClearColor(0,0,0,1.0f);
	
	glEnable(GL_CULL_FACE);
	
	//glHint(GL_PERSPECTIVE_CORRECTION_HINT,GL_NICEST);
}