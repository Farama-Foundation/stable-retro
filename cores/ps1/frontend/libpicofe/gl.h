#ifndef LIBPICOFE_GL_H
#define LIBPICOFE_GL_H

#ifdef HAVE_GLES

int  gl_init(void *display, int *quirks);
int  gl_create(void *window, int *quirks, int w, int h);
void gl_announce(void);
int  gl_flip_v(const void *fb, int w, int h, const float *vertices);
int  gl_flip(const void *fb, int w, int h);
void gl_clear(void);
void gl_destroy(void);
void gl_shutdown(void);

/* for external flips */
extern void *gl_es_display;
extern void *gl_es_surface;

#else

static __inline int gl_init(void *display, int *quirks)
{
  return -1;
}

static __inline int gl_create(void *window, int *quirks, int w, int h)
{
  return -1;
}
static __inline void gl_announce(void)
{
}
static __inline int gl_flip_v(const void *fb, int w, int h, const float *vertices)
{
  return -1;
}
static __inline int gl_flip(const void *fb, int w, int h)
{
  return -1;
}
static __inline void gl_clear(void) {}
static __inline void gl_destroy(void) {}
static __inline void gl_shutdown(void) {}

#define gl_es_display (void *)0
#define gl_es_surface (void *)0

#endif

#define GL_QUIRK_ACTIVATE_RECREATE 1
#define GL_QUIRK_SCALING_NEAREST 2
#define GL_QUIRK_VSYNC_ON 4

#endif // LIBPICOFE_GL_H
