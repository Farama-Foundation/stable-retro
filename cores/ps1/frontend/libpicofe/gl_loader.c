#include <EGL/egl.h>
#include <GLES/gl.h>
//#include "gl_loader.h"

#include <dlfcn.h>
#include <stdio.h>

#if defined(__linux__)

static void *libegl;
static void *libgles;

#define FPTR(f) typeof(f) * p##f
FPTR(eglGetError);
FPTR(eglBindAPI);
FPTR(eglGetDisplay);
FPTR(eglInitialize);
FPTR(eglChooseConfig);
FPTR(eglCreateWindowSurface);
FPTR(eglSwapInterval);
FPTR(eglCreateContext);
FPTR(eglMakeCurrent);
FPTR(eglSwapBuffers);
FPTR(eglDestroyContext);
FPTR(eglDestroySurface);
FPTR(eglTerminate);

FPTR(glGetError);
FPTR(glGetString);
FPTR(glEnableClientState);
FPTR(glEnable);
FPTR(glGenTextures);
FPTR(glDeleteTextures);
FPTR(glBindTexture);
FPTR(glTexImage2D);
FPTR(glTexParameterf);
FPTR(glTexSubImage2D);
FPTR(glTexCoordPointer);
FPTR(glVertexPointer);
FPTR(glDrawArrays);
FPTR(glLoadIdentity);
FPTR(glClearColor);
FPTR(glClear);
FPTR(glFrontFace);


void gl_unload(void)
{
	if (libegl)
		dlclose(libegl);
	libegl = NULL;
	if (libgles)
		dlclose(libgles);
	libgles = NULL;
}

#define LOADSYM(l,n)	p##n = dlsym(l,#n); if (!p##n) goto err;

int gl_load(void)
{
	// possible library file name. Some systems have them versioned, others
	// don't, old-style brcm naming on Raspberry Pi systems.
	char *egl[] = { "libEGL.so.1", "libEGL.so", "libbrcmEGL.so", NULL };
	char *gles[] = { "libGLESv1_CM.so.1", "libGLESv1_CM.so", "libGLES_CM.so.1",
			"libGLES_CM.so", "libbrcmGLESv2.so", "libGLESv2.so", NULL };
	int i;

	for (i = 0, libegl = NULL; egl[i] && !libegl; i++)
		libegl = dlopen(egl[i], RTLD_LAZY);
	if (!libegl)
		goto err;

	LOADSYM(libegl, eglGetError);
	LOADSYM(libegl, eglBindAPI);
	LOADSYM(libegl, eglGetDisplay);
	LOADSYM(libegl, eglInitialize);
	LOADSYM(libegl, eglChooseConfig);
	LOADSYM(libegl, eglCreateWindowSurface);
	LOADSYM(libegl, eglSwapInterval);
	LOADSYM(libegl, eglCreateContext);
	LOADSYM(libegl, eglMakeCurrent);
	LOADSYM(libegl, eglSwapBuffers);
	LOADSYM(libegl, eglDestroyContext);
	LOADSYM(libegl, eglDestroySurface);
	LOADSYM(libegl, eglTerminate);

	for (i = 0, libgles = NULL; gles[i] && !libgles; i++)
		libgles = dlopen(gles[i], RTLD_LAZY);
	if (!libgles)
		goto err;

	LOADSYM(libgles, glGetError);
	LOADSYM(libgles, glGetString);
	LOADSYM(libgles, glEnableClientState);
	LOADSYM(libgles, glEnable);
	LOADSYM(libgles, glGenTextures);
	LOADSYM(libgles, glDeleteTextures);
	LOADSYM(libgles, glBindTexture);
	LOADSYM(libgles, glTexImage2D);
	LOADSYM(libgles, glTexParameterf);
	LOADSYM(libgles, glTexSubImage2D);
	LOADSYM(libgles, glTexCoordPointer);
	LOADSYM(libgles, glVertexPointer);
	LOADSYM(libgles, glDrawArrays);
	LOADSYM(libgles, glLoadIdentity);
	LOADSYM(libgles, glClearColor);
	LOADSYM(libgles, glClear);
	LOADSYM(libgles, glFrontFace);

	return 0;

err:
	fprintf(stderr, "warning: OpenGLES libraries are not available\n");
	gl_unload();
	return 1;
}

#else

void gl_unload(void)
{
}

int gl_load(void)
{
	return 0;
}

#endif
