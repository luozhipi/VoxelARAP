/*
 *  shaders.c
 *  GLUTviewer
 *
 *  Created by Denis Kovacs on 4/10/08.
 *  Copyright 2008 New York University. All rights reserved.
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include "shaders.h"

char *textFileRead(const char *fn) 
{
	
	FILE *fp;
	char *content = NULL;
	
	size_t count;
	
	fp = fopen(fn, "r");
	
	if (fp != NULL) 
	{
		fseek(fp, 0, SEEK_END); 
		count = ftell(fp);
		fseek(fp, 0, SEEK_SET); 
		
		if (count > 0) 
		{
			content = (char *) malloc(sizeof(char) * (count+1));
			count = fread(content, sizeof(char), count, fp);
			content[count] = '\0';
		}
		fclose(fp);
	}
	else {
		printf("File '%s' not found",fn);
	}

	return content;
} 


void printShaderInfoLog(GLuint obj)
{
	GLint infologLength = 0;
	GLint charsWritten  = 0;
	char *infoLog;
	
	glGetShaderiv(obj, GL_INFO_LOG_LENGTH,&infologLength);
	
	if (infologLength > 0)
	{
		infoLog = (char *)malloc(infologLength);
		glGetShaderInfoLog(obj, infologLength, &charsWritten, infoLog);
		printf("%s\n",infoLog);
		free(infoLog);
	}
}



void printProgramInfoLog(GLuint obj)
{
	GLint infologLength = 0;
	GLint charsWritten  = 0;
	char *infoLog;
	
	glGetProgramiv(obj, GL_INFO_LOG_LENGTH,&infologLength);
	
	if (infologLength > 0)
	{
		infoLog = (char *)malloc(infologLength);
		glGetProgramInfoLog(obj, infologLength, &charsWritten, infoLog);
		printf("%s\n",infoLog);
		free(infoLog);
	}
}


GLuint loadShader(const char *src, GLenum shaderType)
{
	GLuint s = glCreateShader(shaderType);
	
	glShaderSource(s, 1, &src, NULL);
	
	glCompileShader(s);

	//printShaderInfoLog(s);

	return s;
}

struct GLSL_Program initShaderProgram(GLuint v, GLuint f)
{
	struct GLSL_Program p;
	p.v = v;
	p.f = f;
	
	p.p = glCreateProgram();
	
	glAttachShader(p.p, p.v);
	glAttachShader(p.p, p.f);
	
	glLinkProgram(p.p);
	// now you can use it with " glUseProgram(p.p); "
	
	//printProgramInfoLog(p.p);
	
	return p;
}

struct GLSL_Program initGShaderProgram(GLuint v, GLuint g, GLuint f, GLenum primIn, GLenum primOut, GLint maxVerticesOut)
{
	struct GLSL_Program p;
	p.v = v;
	p.f = f;
	p.g = g;
	
	p.p = glCreateProgram();
	
	glAttachShader(p.p, p.v);
	glAttachShader(p.p, p.g);
	glAttachShader(p.p, p.f);
	
	glProgramParameteriEXT(p.p, GL_GEOMETRY_INPUT_TYPE_EXT, primIn); 
	glProgramParameteriEXT(p.p, GL_GEOMETRY_OUTPUT_TYPE_EXT, primOut); 
	glProgramParameteriEXT(p.p, GL_GEOMETRY_VERTICES_OUT_EXT, maxVerticesOut);
	
	glLinkProgram(p.p);
	// now you can use it with " glUseProgram(p.p); "
	
	//printProgramInfoLog(p.p);
	
	return p;
}


struct GLSL_Program loadShaderProgram(const char *vertFilename, const char *fragFilename) 
{
	char *vertShader = textFileRead(vertFilename);
	char *fragShader = textFileRead(fragFilename);
	
	GLuint v = loadShader(vertShader, GL_VERTEX_SHADER);
	GLuint f = loadShader(fragShader, GL_FRAGMENT_SHADER);
	
	free((char *) vertShader); 
	free((char *) fragShader); 
	
	return initShaderProgram(v, f);
}


struct GLSL_Program loadShaderProgramStr(const char *vertShader, const char *fragShader) 
{
	GLuint v = loadShader(vertShader, GL_VERTEX_SHADER);
	GLuint f = loadShader(fragShader, GL_FRAGMENT_SHADER);
	
	return initShaderProgram(v, f);
}


struct GLSL_Program loadGShaderProgram(const char *vertFilename, const char *geomFilename, const char *fragFilename, GLenum primIn, GLenum primOut, GLint maxVerticesOut) 
{
	GLuint v = loadShader(vertFilename, GL_VERTEX_SHADER);
	GLuint g = loadShader(geomFilename, GL_GEOMETRY_SHADER_EXT);
	GLuint f = loadShader(fragFilename, GL_FRAGMENT_SHADER);
	
	return initGShaderProgram(v, g, f, primIn, primOut, maxVerticesOut);
}


void deinitShaderProgram(struct GLSL_Program p)
{
	glDetachShader(p.p, p.v);
	glDetachShader(p.p, p.f);
	
	glDeleteShader(p.v); 
	glDeleteShader(p.v); 

	glDeleteProgram(p.p);
}
