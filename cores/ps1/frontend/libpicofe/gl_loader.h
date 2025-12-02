#if defined(__linux__)

int gl_load(void);
void gl_unload(void);


#define FPTR(f) typeof(f) * p##f
extern FPTR(eglGetError);
extern FPTR(eglBindAPI);
extern FPTR(eglGetDisplay);
extern FPTR(eglInitialize);
extern FPTR(eglChooseConfig);
extern FPTR(eglCreateWindowSurface);
extern FPTR(eglSwapInterval);
extern FPTR(eglCreateContext);
extern FPTR(eglMakeCurrent);
extern FPTR(eglSwapBuffers);
extern FPTR(eglDestroyContext);
extern FPTR(eglDestroySurface);
extern FPTR(eglTerminate);

extern FPTR(glGetError);
extern FPTR(glGetString);
extern FPTR(glEnableClientState);
extern FPTR(glEnable);
extern FPTR(glGenTextures);
extern FPTR(glDeleteTextures);
extern FPTR(glBindTexture);
extern FPTR(glTexImage2D);
extern FPTR(glTexParameterf);
extern FPTR(glTexSubImage2D);
extern FPTR(glTexCoordPointer);
extern FPTR(glVertexPointer);
extern FPTR(glDrawArrays);
extern FPTR(glLoadIdentity);
extern FPTR(glClearColor);
extern FPTR(glClear);
extern FPTR(glFrontFace);
#undef FPTR


#define eglGetError peglGetError
#define eglBindAPI peglBindAPI
#define eglGetDisplay peglGetDisplay
#define eglInitialize peglInitialize
#define eglChooseConfig peglChooseConfig
#define eglCreateWindowSurface peglCreateWindowSurface
#define eglSwapInterval peglSwapInterval
#define eglCreateContext peglCreateContext
#define eglMakeCurrent peglMakeCurrent
#define eglSwapBuffers peglSwapBuffers
#define eglDestroyContext peglDestroyContext
#define eglDestroySurface peglDestroySurface
#define eglTerminate peglTerminate

#define glGetError pglGetError
#define glGetString pglGetString
#define glLoadIdentity pglLoadIdentity
#define glEnableClientState pglEnableClientState
#define glEnable pglEnable
#define glGenTextures pglGenTextures
#define glDeleteTextures pglDeleteTextures
#define glBindTexture pglBindTexture
#define glTexImage2D pglTexImage2D
#define glTexParameterf pglTexParameterf
#define glTexSubImage2D pglTexSubImage2D
#define glTexCoordPointer pglTexCoordPointer
#define glVertexPointer pglVertexPointer
#define glDrawArrays pglDrawArrays
#define glLoadIdentity pglLoadIdentity
#define glClearColor pglClearColor
#define glClear pglClear
#define glFrontFace pglFrontFace

#endif
