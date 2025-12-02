#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <EGL/egl.h>
#include <GLES/gl.h>
#include "gl_loader.h"
#include "gl_platform.h"
#include "gl.h"

static EGLDisplay edpy = EGL_NO_DISPLAY;
static EGLSurface esfc = EGL_NO_SURFACE;
static EGLContext ectxt = EGL_NO_CONTEXT;

static GLuint texture_name;

/* for external flips */
void *gl_es_display;
void *gl_es_surface;

static int tex_w, tex_h;
static void *tex_mem;
static int flip_old_w, flip_old_h;

static int gl_have_error(const char *name)
{
	GLenum e = glGetError();
	if (e != GL_NO_ERROR) {
		fprintf(stderr, "GL error: %s %x\n", name, e);
		return 1;
	}
	return 0;
}

static int gles_have_error(const char *name)
{
	EGLint e = eglGetError();
	if (e != EGL_SUCCESS) {
		fprintf(stderr, "%s %x\n", name, e);
		return 1;
	}
	return 0;
}

int gl_init(void *display, int *quirks)
{
	int retval = -1;
	int ret;

	ret = gl_platform_init(&display, quirks);
	if (ret != 0) {
		fprintf(stderr, "gl_platform_init failed with %d\n", ret);
		return retval;
	}

	edpy = eglGetDisplay((EGLNativeDisplayType)display);
	if (edpy == EGL_NO_DISPLAY) {
		fprintf(stderr, "Failed to get EGL display\n");
		goto out;
	}

	if (!eglInitialize(edpy, NULL, NULL)) {
		fprintf(stderr, "Failed to initialize EGL\n");
		goto out;
	}
	retval = 0;

out:
	if (retval && edpy != EGL_NO_DISPLAY)
		gl_shutdown();
	return retval;
}

int gl_create(void *window, int *quirks, int w, int h)
{
	EGLConfig ecfg = NULL;
	EGLint num_config;
	int retval = -1;
	int ret;
	EGLint config_attr[] =
	{
		EGL_NONE
	};

	ret = gl_platform_create(&window, quirks);
	if (ret != 0) {
		fprintf(stderr, "gl_platform_init failed with %d\n", ret);
		return retval;
	}

	flip_old_w = flip_old_h = 0;
	for (tex_w = 1; tex_w < w; tex_w *= 2)
		;
	for (tex_h = 1; tex_h < h; tex_h *= 2)
		;
	tex_mem = realloc(tex_mem, tex_w * tex_h * 2);
	if (tex_mem == NULL) {
		fprintf(stderr, "OOM\n");
		goto out;
	}
	memset(tex_mem, 0, tex_w * tex_h * 2);

	if (!eglChooseConfig(edpy, config_attr, &ecfg, 1, &num_config)) {
		fprintf(stderr, "Failed to choose config (%x)\n", eglGetError());
		goto out;
	}

	if (ecfg == NULL || num_config == 0) {
		fprintf(stderr, "No EGL configs available\n");
		goto out;
	}

	esfc = eglCreateWindowSurface(edpy, ecfg,
		(EGLNativeWindowType)window, NULL);
	if (esfc == EGL_NO_SURFACE) {
		fprintf(stderr, "Unable to create EGL surface (%x)\n",
			eglGetError());
		goto out;
	}

	ectxt = eglCreateContext(edpy, ecfg, EGL_NO_CONTEXT, NULL);
	if (ectxt == EGL_NO_CONTEXT) {
		fprintf(stderr, "Unable to create EGL context (%x)\n",
			eglGetError());
		// on mesa, some distros disable ES1.x but compat GL still works
		ret = eglBindAPI(EGL_OPENGL_API);
		if (!ret) {
			fprintf(stderr, "eglBindAPI: %x\n", eglGetError());
			goto out;
		}
		ectxt = eglCreateContext(edpy, ecfg, EGL_NO_CONTEXT, NULL);
		if (ectxt == EGL_NO_CONTEXT) {
			fprintf(stderr, "giving up on EGL context (%x)\n",
				eglGetError());
			goto out;
		}
	}

	ret = eglMakeCurrent(edpy, esfc, esfc, ectxt);
	if (!ret) {
		fprintf(stderr, "eglMakeCurrent: %x\n", eglGetError());
		goto out;
	}
	
	ret = *quirks & GL_QUIRK_VSYNC_ON ? 1 : 0;
	eglSwapInterval(edpy, ret);

	// 1.x (fixed-function) only
	glEnable(GL_TEXTURE_2D);
	glGetError();

	assert(!texture_name);

	glGenTextures(1, &texture_name);
	if (gl_have_error("glGenTextures"))
		goto out;

	glBindTexture(GL_TEXTURE_2D, texture_name);
	if (gl_have_error("glBindTexture"))
		goto out;

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, tex_w, tex_h, 0, GL_RGB,
		GL_UNSIGNED_SHORT_5_6_5, tex_mem);
	if (gl_have_error("glTexImage2D"))
		goto out;

	// no mipmaps
	ret = *quirks & GL_QUIRK_SCALING_NEAREST ? GL_NEAREST : GL_LINEAR;
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, ret);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, ret);

	//glViewport(0, 0, 512, 512);
	glLoadIdentity();
	glFrontFace(GL_CW);
	glEnable(GL_CULL_FACE);

	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glEnableClientState(GL_VERTEX_ARRAY);

	if (gl_have_error("init"))
		goto out;

	gl_es_display = (void *)edpy;
	gl_es_surface = (void *)esfc;
	retval = 0;
out:
	if (retval && edpy != EGL_NO_DISPLAY)
		gl_destroy();
	return retval;
}

void gl_announce(void)
{
	printf("GL_RENDERER: %s\n", (char *)glGetString(GL_RENDERER));
	printf("GL_VERSION: %s\n", (char *)glGetString(GL_VERSION));
}

static const float default_vertices[] = {
	-1.0f,  1.0f,  0.0f, // 0    0  1
	 1.0f,  1.0f,  0.0f, // 1  ^
	-1.0f, -1.0f,  0.0f, // 2  | 2  3
	 1.0f, -1.0f,  0.0f, // 3  +-->
};

static float texture[] = {
	0.0f, 0.0f, // we flip this:
	1.0f, 0.0f, // v^
	0.0f, 1.0f, //  |  u
	1.0f, 1.0f, //  +-->
};

int gl_flip_v(const void *fb, int w, int h, const float *vertices)
{
	gl_have_error("pre-flip unknown");

	glBindTexture(GL_TEXTURE_2D, texture_name);
	if (gl_have_error("glBindTexture"))
		return -1;

	if (fb != NULL) {
		if (w != flip_old_w || h != flip_old_h) {
			float f_w = (float)w / tex_w;
			float f_h = (float)h / tex_h;
			texture[1*2 + 0] = f_w;
			texture[2*2 + 1] = f_h;
			texture[3*2 + 0] = f_w;
			texture[3*2 + 1] = f_h;
			flip_old_w = w;
			flip_old_h = h;
		}

		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, w, h,
			GL_RGB, GL_UNSIGNED_SHORT_5_6_5, fb);
		if (gl_have_error("glTexSubImage2D")) {
			fprintf(stderr, "  %dx%d t: %dx%d %p\n", w, h, tex_w, tex_h, fb);
			return -1;
		}
	}

	glVertexPointer(3, GL_FLOAT, 0, vertices ? vertices : default_vertices);
	glTexCoordPointer(2, GL_FLOAT, 0, texture);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	if (gl_have_error("glDrawArrays"))
		return -1;

	eglSwapBuffers(edpy, esfc);
	if (gles_have_error("eglSwapBuffers"))
		return -1;

	return 0;
}

int gl_flip(const void *fb, int w, int h)
{
	return gl_flip_v(fb, w, h, NULL);
}

// to be used once after exiting menu, etc
void gl_clear(void)
{
	glClearColor(0, 0, 0, 0);
	glClear(GL_COLOR_BUFFER_BIT);
	eglSwapBuffers(edpy, esfc);
	glClear(GL_COLOR_BUFFER_BIT);
	eglSwapBuffers(edpy, esfc);
	glClear(GL_COLOR_BUFFER_BIT);
	gl_have_error("glClear");
}

void gl_destroy(void)
{
	if (edpy == EGL_NO_DISPLAY)
		return; // nothing to do

	// sometimes there is an error... from somewhere?
	//gl_have_error("finish");
	glGetError();

	if (texture_name)
	{
		glDeleteTextures(1, &texture_name);
		gl_have_error("glDeleteTextures");
		texture_name = 0;
	}

	eglMakeCurrent(edpy, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
	if (esfc != EGL_NO_SURFACE) {
		eglDestroySurface(edpy, esfc);
		esfc = EGL_NO_SURFACE;
	}
	if (ectxt != EGL_NO_CONTEXT) {
		eglDestroyContext(edpy, ectxt);
		ectxt = EGL_NO_CONTEXT;
	}

	gl_es_surface = (void *)esfc;

	if (tex_mem) free(tex_mem);
	tex_mem = NULL;

	gl_platform_destroy();
}

void gl_shutdown(void)
{
	if (edpy != EGL_NO_DISPLAY) {
		eglTerminate(edpy);
		edpy = EGL_NO_DISPLAY;
	}

	gl_es_display = (void *)edpy;

	gl_platform_shutdown();
}
