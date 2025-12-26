#ifndef _OPENGL_H
#define _OPENGL_H

extern bool enable_opengl;
extern bool using_opengl;
extern bool refresh_opengl;

bool initialize_opengl();
void deinitialize_opengl_renderer();
void render_opengl_frame(bool sw);

#endif
