//
//  Plugin.h
//  Preview3D
//
//  Created by Olga Diamanti on 9/21/11.
//  Copyright 2011 ETH Zurich. All rights reserved.
//
//

#ifndef PreviewPlugin_h
#define PreviewPlugin_h

#include "Preview3D.h"

// Abstract class for plugins
// All plugins MUST have this class as their parent and implement all the callbacks
// For an example of a basic plugins see plugins/skeleton.h
//
// Return value of callbacks: returning true to any of the callbacks tells Preview3D that the event has been 
// handled and that it should not be passed to other plugins or to other internal functions of Preview3D

class PreviewPlugin
{
public:
  PreviewPlugin(){}
  ~PreviewPlugin(){};
  // Runs immediately after a new mesh had been loaded. 
  // Note: this callback is also called on startup if no mesh has been loaded, in this case the vertices and faces
  //       matrices of Preview3D will be empty
  virtual void init(Preview3D *preview)
  {
    m_preview = preview;
  }
  
  // It is called before the draw procedure of Preview3D
  virtual void preDraw(int currentTime) = 0;
  
  // It is called after the draw procedure of Preview3D
  virtual void postDraw(int currentTime)=0;
  
  // It is called when the mouse button is pressed
  // - button can be GLUT_LEFT_BUTTON, GLUT_MIDDLE_BUTTON or GLUT_RIGHT_BUTTON
  // - modifiers is a bitfield that might one or more of the following bits Preview3D::NO_KEY, Preview3D::SHIFT, Preview3D::CTRL, Preview3D::ALT;
  virtual bool mouseDownEvent(int mouse_x, int mouse_y, int button, int modifiers)=0;
  
  // It is called when the mouse button is released
  // - button can be GLUT_LEFT_BUTTON, GLUT_MIDDLE_BUTTON or GLUT_RIGHT_BUTTON
  // - modifiers is a bitfield that might one or more of the following bits Preview3D::NO_KEY, Preview3D::SHIFT, Preview3D::CTRL, Preview3D::ALT;
  virtual bool mouseUpEvent(int mouse_x, int mouse_y, int button, int modifiers)=0;
  
  // It is called every time the mouse is moved
  // - mouse_x and mouse_y are the new coordinates of the mouse pointer in screen coordinates
  virtual bool mouseMoveEvent(int mouse_x, int mouse_y)=0;

  // It is called every time the scroll wheel is moved
  // Note: this callback is not working with every glut implementation
  virtual bool mouseScrollEvent(int mouse_x, int mouse_y,  float delta_y)=0;
  
  // It is called when a keyboard key is pressed
  // - modifiers is a bitfield that might one or more of the following bits Preview3D::NO_KEY, Preview3D::SHIFT, Preview3D::CTRL, Preview3D::ALT;
  // - mouse_x and mouse_y are the current coordinates of the mouse pointer in screen coordinates 
  virtual bool keyDownEvent(unsigned char key, int modifiers, int mouse_x, int mouse_y)=0;


  virtual void resize(int w, int h)=0;
  
protected:
  // Pointer to the main Preview3D class
  Preview3D *m_preview;
  
};

#endif
