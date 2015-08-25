// Sylvain Lefebvre 2013-01-06
#pragma once

// Call at init time   WARNING: expects a square rendering port
void slcrystal_init(int screen_sz,float znear,float zfar);

void slcrystal_terminate();

void slcrystal_set_perspective(const float *m);
void slcrystal_set_view(const float *m);
void slcrystal_set_lightpos(const float *d);

void slcrystal_set_media_color(float r,float g,float b);
void slcrystal_set_media_exponent(float e);
void slcrystal_set_media_density (float d);

// start frame, give background color
void slcrystal_frame_begin(float r,float g,float b);

void slcrystal_begin(float r,float g,float b,float a);

void slcrystal_end();

// set on offset for the viewport
void slcrystal_set_viewport_offset(int x_viewport,int y_viewport);

void slcrystal_set_model_matrix( const float *modelMatrix );

// end frame (draw on screen)
void slcrystal_frame_end();

// draw a simple box
void slcrystal_draw_box();
