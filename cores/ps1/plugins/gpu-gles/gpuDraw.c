/***************************************************************************
                           draw.c  -  description
                             -------------------
    begin                : Sun Mar 08 2009
    copyright            : (C) 1999-2009 by Pete Bernert
    web                  : www.pbernert.com   
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version. See also the license.txt file for *
 *   additional informations.                                              *
 *                                                                         *
 ***************************************************************************/

//*************************************************************************// 
// History of changes:
//
// 2009/03/08 - Pete  
// - generic cleanup for the Peops release
//
//*************************************************************************// 


#define _IN_DRAW


#include "gpuExternals.h"
#include "gpuPlugin.h"
#include "gpuDraw.h"
#include "gpuPrim.h"
#include "gpuTexture.h"
#include "gpuStdafx.h"

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
//#include "menu.h"
            
////////////////////////////////////////////////////////////////////////////////////
// defines

#define SIGNBIT 0x800
#define S_MASK  0xf000
#define L_MASK  0xfffff000

// ownscale: some ogl drivers have buggy texture matrix funcs, so it
//           is safer to calc sow/tow ourselves

#ifdef OWNSCALE

///////////////////////////////////////////////////////////////

#define ST_FACSPRITE       255.99f
#define ST_BFFACSPRITE     0.5f/256.0f
#define ST_BFFACSPRITESORT 0.333f/256.0f

#define ST_OFFSET          0.5f/256.0f;

#define ST_FAC             255.99f
#define ST_BFFAC           0.5f/256.0f
#define ST_BFFACSORT       0.333f/256.0f

#define ST_FACTRI          255.99f
#define ST_BFFACTRI        0.5f/256.0f
#define ST_BFFACTRISORT    0.333f/256.0f

#define ST_FACVRAMX        255.0f
#define ST_FACVRAM         256.0f

///////////////////////////////////////////////////////////////

#else

#define ST_BFFACSPRITE     0.5f
#define ST_BFFACSPRITESORT 0.333f

#define ST_BFFAC           0.5f
#define ST_BFFACSORT       0.333f

#define ST_BFFACTRI        0.5f
#define ST_BFFACTRISORT    0.333f

#define ST_OFFSET          0.5f;
                
#endif

////////////////////////////////////////////////////////////////////////////////////
// draw globals

void  glBlendEquationEXT(GLenum mode);
void  glColorTableEXT(GLenum target, GLenum internalFormat, GLsizei width, GLenum format,GLenum type, const GLvoid *data);

// draw globals; most will be initialized again later (by config or checks) 

BOOL           bIsFirstFrame=TRUE;

// resolution/ratio vars

int            iResX;
int            iResY;
BOOL           bKeepRatio=FALSE;
RECT           rRatioRect;

// psx mask related vars

BOOL           bCheckMask=FALSE;
int            iUseMask=0;
int            iSetMask=0;
unsigned short sSetMask=0;
unsigned int   lSetMask=0;

// drawing/coord vars

OGLVertex      vertex[4];
GLubyte        gl_ux[8];
GLubyte        gl_vy[8];
short          sprtY,sprtX,sprtH,sprtW;

// drawing options

BOOL           bOpaquePass;
BOOL           bAdvancedBlend;

// OGL extension support


// gfx card buffer infos

int            iDepthFunc=0;
int            iZBufferDepth=0;
GLbitfield     uiBufferBits=GL_COLOR_BUFFER_BIT;

////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
// Set OGL pixel format
////////////////////////////////////////////////////////////////////////
 

////////////////////////////////////////////////////////////////////////
// Get extension infos (f.e. pal textures / packed pixels)
////////////////////////////////////////////////////////////////////////

void GetExtInfos(void)                              
{
 BOOL bPacked=FALSE;                                   // default: no packed pixel support

 if(strstr((char *)glGetString(GL_EXTENSIONS),         // packed pixels available?
    "GL_EXT_packed_pixels"))                          
  bPacked=TRUE;                                        // -> ok

 
 iClampType=GL_CLAMP_TO_EDGE;
}

////////////////////////////////////////////////////////////////////////
// Setup some stuff depending on user settings or in-game toggle
////////////////////////////////////////////////////////////////////////

void SetExtGLFuncs(void)
{
 //----------------------------------------------------//

 SetFixes();                                           // update fix infos

 //----------------------------------------------------//

  {
   if(bAdvancedBlend) bUseMultiPass=TRUE;              // -> pseudo-advanced with 2 passes
   else               bUseMultiPass=FALSE;             // -> or simple 'bright color' mode
//   bGLBlend=FALSE;                                     // -> no ext blending!
   glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE); glError();
  }

 if(bOpaquePass)                                        // opaque mode?
  {
   if(dwActFixes&32) 
    {
     TCF[0]=CP8RGBA_0;
     PalTexturedColourFn=CP8RGBA;                      // -> init col func
    }
   else
    {
     TCF[0]=XP8RGBA_0;
     PalTexturedColourFn=XP8RGBA;                      // -> init col func
    }

   TCF[1]=XP8RGBA_1;
   glAlphaFuncx(GL_GREATER,0.49f); glError();

  }
 else                                                  // no opaque mode?
  {
   TCF[0]=TCF[1]=P8RGBA;
   PalTexturedColourFn=P8RGBA;                         // -> init col func
   glAlphaFuncx(GL_NOTEQUAL,0); glError();             // --> set alpha func

  }

 //----------------------------------------------------//

 LoadSubTexFn=LoadSubTexturePageSort;                  // init load tex ptr

 bBlendEnable=FALSE;                                   // init blending: off
 glDisable(GL_BLEND); glError();


 SetScanTrans();                                       // init scan lines (if wanted)
}

////////////////////////////////////////////////////////////////////////
// setup scan lines
////////////////////////////////////////////////////////////////////////

#define R_TSP 0x00,0x45,0x00,0xff
#define G_TSP 0x00,0x00,0x45,0xff
#define B_TSP 0x45,0x00,0x00,0xff
#define O_TSP 0x45,0x45,0x45,0xff
#define N_TSP 0x00,0x00,0x00,0xff

GLuint  gTexScanName=0;

GLubyte texscan[4][16]= 
{
{R_TSP, G_TSP, B_TSP, N_TSP},
{O_TSP, N_TSP, O_TSP, N_TSP},
{B_TSP, N_TSP, R_TSP, G_TSP},
{O_TSP, N_TSP, O_TSP, N_TSP}
};

void CreateScanLines(void)
{
}

////////////////////////////////////////////////////////////////////////
// Initialize OGL
////////////////////////////////////////////////////////////////////////

#define MODE_RAW 0
#define MODE_X11 1
#define MODE_SDL 2
int use_fsaa = 0;

EGLDisplay display;
EGLSurface surface;
static EGLConfig  config;
static EGLContext context;

#if defined(USE_X11)
#include "X11/Xlib.h"
#include "X11/Xutil.h"
#include "X11/Xatom.h"

Window			x11Window	= 0;
Display*		x11Display	= 0;
long			x11Screen	= 0;
XVisualInfo		x11Visual;
XVisualInfo*	px11Visual	= 0;
Colormap        x11Colormap	= 0;
#endif

EGLint attrib_list_fsaa[] =
{
	EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
	EGL_BUFFER_SIZE,    0,
	EGL_DEPTH_SIZE,     16,
	EGL_SAMPLE_BUFFERS, 1,
	EGL_SAMPLES,        4,
	EGL_NONE
};

EGLint attrib_list[] =
{
//	EGL_DEPTH_SIZE,   16,
	EGL_NONE
};

bool TestEGLError(const char* pszLocation)
{
	/*
		eglGetError returns the last error that has happened using egl,
		not the status of the last called function. The user has to
		check after every single egl call or at least once every frame.
	*/
	EGLint iErr = eglGetError();
	if (iErr != EGL_SUCCESS)
	{
		printf("%s failed (0x%x).\n", pszLocation, (int)iErr);
		return FALSE;
	}

	return TRUE;
}

static int initEGL(void)
{
	NativeWindowType window = 0;

	printf ("GL init\n");

	EGLint numConfigs;
	EGLint majorVersion;
	EGLint minorVersion;
#if defined(USE_X11)
	enum
	{
	_NET_WM_STATE_REMOVE =0,
	_NET_WM_STATE_ADD = 1,
	_NET_WM_STATE_TOGGLE =2
	};
	
	Window			        sRootWindow;
	XSetWindowAttributes	sWA;
	unsigned int		    ui32Mask;
	int			            i32Depth;
#endif
	
	EGLint *attribList = NULL;
	if (use_fsaa)
	{
		printf( "GLES: Using Full Scene Antialiasing\n" );
		attribList = attrib_list_fsaa;
	}
	else
	{
		attribList = attrib_list;
	}

#if defined(USE_X11)
            // Initializes the display and screen
            x11Display = XOpenDisplay( ":0" );
            if (!x11Display)
            {
                printf("GLES Error: Unable to open X display\n");
                return -1;
            }
            x11Screen = XDefaultScreen( x11Display );

            // Gets the display parameters so we can pass the same parameters to the window to be created.
            sRootWindow	= RootWindow(x11Display, x11Screen);
            i32Depth	= DefaultDepth(x11Display, x11Screen);
            px11Visual	= &x11Visual;
            XMatchVisualInfo( x11Display, x11Screen, i32Depth, TrueColor, px11Visual);
            if (!px11Visual)
            {
                printf("GLES Error: Unable to acquire visual\n");
                return -1;
            }
            // Colormap of the specified visual type for the display.
            x11Colormap = XCreateColormap( x11Display, sRootWindow, px11Visual->visual, AllocNone );
            sWA.colormap = x11Colormap;

            // List of events to be handled by the application. Add to these for handling other events.
            sWA.event_mask = StructureNotifyMask | ExposureMask | ButtonPressMask | ButtonReleaseMask | KeyPressMask | KeyReleaseMask;

            // Display capabilities list.
            ui32Mask = CWBackPixel | CWBorderPixel | CWEventMask | CWColormap;

            // Creates the X11 window
            x11Window = XCreateWindow( x11Display, RootWindow(x11Display, x11Screen), 0, 0, iResX, iResY,
                                        0, CopyFromParent, InputOutput, CopyFromParent, ui32Mask, &sWA);

            // Make the window viewable and flush the output buffer.
            XMapWindow(x11Display, x11Window);
            XFlush(x11Display);

            // Make the window fullscreen
            unsigned char fullScreen = 1;
            Atom wmState = XInternAtom(x11Display, "_NET_WM_STATE", False);
            Atom wmFullScreen = XInternAtom(x11Display,"_NET_WM_STATE_FULLSCREEN", False);

            XEvent xev;
            xev.xclient.type		    = ClientMessage;
            xev.xclient.serial		    = 0;
            xev.xclient.send_event      = True;
            xev.xclient.window		    = x11Window;
            xev.xclient.message_type    = wmState;
            xev.xclient.format		    = 32;
            xev.xclient.data.l[0]		= (fullScreen ? _NET_WM_STATE_ADD : _NET_WM_STATE_REMOVE);
            xev.xclient.data.l[1]		= wmFullScreen;
            xev.xclient.data.l[2]		= 0;

            XSendEvent(x11Display, DefaultRootWindow(x11Display), False, SubstructureRedirectMask | SubstructureNotifyMask, &xev);

            display = eglGetDisplay( (EGLNativeDisplayType)x11Display );
            window = x11Window;
#else
            display = eglGetDisplay( (EGLNativeDisplayType)0 );
#endif

	if( display == EGL_NO_DISPLAY )
	{
		printf( "GLES EGL Error: GL No Display\n" );
		return -1;
	}

	if( !eglInitialize( display, &majorVersion, &minorVersion ) )
	{
		printf( "GLES EGL Error: eglInitialize failed\n" );
		return -1;
	}

	if( !eglChooseConfig( display, attribList, &config, 1, &numConfigs ) )
	{
		printf( "GLES EGL Error: eglChooseConfig failed\n" );
		return -1;
	}

	context = eglCreateContext( display, config, NULL, NULL );
	if( context==0 )
	{
		printf( "GLES EGL Error: eglCreateContext failed\n" );
		return -1;
	}

#ifdef FAKE_WINDOW
	// broken Caanoo libs won't accept NULL window
	window = (NativeWindowType)1;
#endif
	surface = eglCreateWindowSurface( display, config, window, NULL );
	if (!TestEGLError("eglCreateWindowSurface"))
		return -1;

	eglMakeCurrent( display, surface, surface, context );
	if (!TestEGLError("eglMakeCurrent"))
		return -1;

	printf("GLES init ok\n");
	return 0;
}

static int created_gles_context;

int GLinitialize(void *ext_gles_display, void *ext_gles_surface)
{
 if(ext_gles_display != NULL && ext_gles_surface != NULL) { 
  display = (EGLDisplay)ext_gles_display;
  surface = (EGLSurface)ext_gles_surface;
 }
 else {
  if(initEGL()!=0)
   return -1;
  created_gles_context=1;
 }

 //----------------------------------------------------// 

 glDepthRangef(0.0f, 1.0f);glError();

 glViewport(rRatioRect.left,                           // init viewport by ratio rect
            iResY-(rRatioRect.top+rRatioRect.bottom),
            rRatioRect.right, 
            rRatioRect.bottom); glError();

 glScissor(0, 0, iResX, iResY); glError();             // init clipping (fullscreen)
 glEnable(GL_SCISSOR_TEST); glError();

#ifndef OWNSCALE
 glMatrixMode(GL_TEXTURE);                             // init psx tex sow and tow if not "ownscale"
 glLoadIdentity();
 glScalef(1.0f/255.99f,1.0f/255.99f,1.0f);             // geforce precision hack
#endif 
 
 //glPolygonOffset( -0.2f, -0.2f );glError();

 glMatrixMode(GL_PROJECTION); glError();               // init projection with psx resolution
 glLoadIdentity(); glError();

 glOrtho(0,PSXDisplay.DisplayMode.x,
         PSXDisplay.DisplayMode.y, 0, -1, 1); glError();

 if(iZBufferDepth)                                     // zbuffer?
  {
   uiBufferBits=GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT;
   glEnable(GL_DEPTH_TEST); glError();
   glDepthFunc(GL_ALWAYS); glError();
   iDepthFunc=1;
  }
 else                                                  // no zbuffer?
  {
   uiBufferBits=GL_COLOR_BUFFER_BIT;
   glDisable(GL_DEPTH_TEST); glError();
  }

 glClearColor(0.0f, 0.0f, 0.0f, 0.0f); glError();      // first buffer clear
 glClear(uiBufferBits); glError();

 GetExtInfos();                                        // get ext infos
 SetExtGLFuncs();                                      // init all kind of stuff (tex function pointers)
 
 glEnable(GL_ALPHA_TEST); glError();                   // wanna alpha test

  {
   glDisable(GL_LINE_SMOOTH); glError();
   glDisable(GL_POINT_SMOOTH); glError();
  }

 ubGloAlpha=127;                                       // init some drawing vars
 ubGloColAlpha=127;
 TWin.UScaleFactor = 1;
 TWin.VScaleFactor = 1;
 bDrawMultiPass=FALSE;
 bTexEnabled=FALSE;
 bUsingTWin=FALSE;
      
 if(bDrawDither)  glEnable(GL_DITHER);                 // dither mode
 else             glDisable(GL_DITHER); 
 glError(); 
 glDisable(GL_FOG); glError();                          // turn all (currently) unused modes off
 glDisable(GL_LIGHTING); glError();  
 glDisable(GL_STENCIL_TEST); glError();  
 glDisable(GL_TEXTURE_2D); glError();
 glDisable(GL_CULL_FACE);

 glFlush(); glError();                                 // we are done...
 glFinish(); glError();                           

 CreateScanLines();                                    // setup scanline stuff (if wanted)

 CheckTextureMemory();                                 // check available tex memory

 if(bKeepRatio) SetAspectRatio();                      // set ratio

 
 bIsFirstFrame = FALSE;                                // we have survived the first frame :)

 return 0;
}

////////////////////////////////////////////////////////////////////////
// clean up OGL stuff
////////////////////////////////////////////////////////////////////////

void GLcleanup() 
{                                                     
 CleanupTextureStore();                                // bye textures

 if(created_gles_context) {
  eglMakeCurrent( display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT );
  eglDestroySurface( display, surface );
  eglDestroyContext( display, context );
  eglTerminate( display );

#if defined(USE_X11)
		if (x11Window) XDestroyWindow(x11Display, x11Window);
		if (x11Colormap) XFreeColormap( x11Display, x11Colormap );
		if (x11Display) XCloseDisplay(x11Display);
#endif
  created_gles_context=0;
 }
}

////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
// Offset stuff
////////////////////////////////////////////////////////////////////////

// please note: it is hardly do-able in a hw/accel plugin to get the 
//              real psx polygon coord mapping right... the following
//              works not to bad with many games, though

static __inline BOOL CheckCoord4()
{
 if(lx0<0)
  {
   if(((lx1-lx0)>CHKMAX_X) ||
      ((lx2-lx0)>CHKMAX_X)) 
    {
     if(lx3<0)
      {
       if((lx1-lx3)>CHKMAX_X) return TRUE;
       if((lx2-lx3)>CHKMAX_X) return TRUE;
      }
    }
  }
 if(lx1<0)
  {
   if((lx0-lx1)>CHKMAX_X) return TRUE;
   if((lx2-lx1)>CHKMAX_X) return TRUE;
   if((lx3-lx1)>CHKMAX_X) return TRUE;
  }
 if(lx2<0)
  {
   if((lx0-lx2)>CHKMAX_X) return TRUE;
   if((lx1-lx2)>CHKMAX_X) return TRUE;
   if((lx3-lx2)>CHKMAX_X) return TRUE;
  }
 if(lx3<0)
  {
   if(((lx1-lx3)>CHKMAX_X) ||
      ((lx2-lx3)>CHKMAX_X))
    {
     if(lx0<0)
      {
       if((lx1-lx0)>CHKMAX_X) return TRUE;
       if((lx2-lx0)>CHKMAX_X) return TRUE;
      }
    }
  }
 

 if(ly0<0)
  {
   if((ly1-ly0)>CHKMAX_Y) return TRUE;
   if((ly2-ly0)>CHKMAX_Y) return TRUE;
  }
 if(ly1<0)
  {
   if((ly0-ly1)>CHKMAX_Y) return TRUE;
   if((ly2-ly1)>CHKMAX_Y) return TRUE;
   if((ly3-ly1)>CHKMAX_Y) return TRUE;
  }
 if(ly2<0)
  {
   if((ly0-ly2)>CHKMAX_Y) return TRUE;
   if((ly1-ly2)>CHKMAX_Y) return TRUE;
   if((ly3-ly2)>CHKMAX_Y) return TRUE;
  }
 if(ly3<0)
  {
   if((ly1-ly3)>CHKMAX_Y) return TRUE;
   if((ly2-ly3)>CHKMAX_Y) return TRUE;
  }

 return FALSE;
}

static __inline BOOL CheckCoord3()
{
 if(lx0<0)
  {
   if((lx1-lx0)>CHKMAX_X) return TRUE;
   if((lx2-lx0)>CHKMAX_X) return TRUE;
  }
 if(lx1<0)
  {
   if((lx0-lx1)>CHKMAX_X) return TRUE;
   if((lx2-lx1)>CHKMAX_X) return TRUE;
  }
 if(lx2<0)
  {
   if((lx0-lx2)>CHKMAX_X) return TRUE;
   if((lx1-lx2)>CHKMAX_X) return TRUE;
  }
 if(ly0<0)
  {
   if((ly1-ly0)>CHKMAX_Y) return TRUE;
   if((ly2-ly0)>CHKMAX_Y) return TRUE;
  }
 if(ly1<0)
  {
   if((ly0-ly1)>CHKMAX_Y) return TRUE;
   if((ly2-ly1)>CHKMAX_Y) return TRUE;
  }
 if(ly2<0)
  {
   if((ly0-ly2)>CHKMAX_Y) return TRUE;
   if((ly1-ly2)>CHKMAX_Y) return TRUE;
  }

 return FALSE;
}


static __inline BOOL CheckCoord2()
{
 if(lx0<0)
  {
   if((lx1-lx0)>CHKMAX_X) return TRUE;
  }
 if(lx1<0)
  {
   if((lx0-lx1)>CHKMAX_X) return TRUE;
  }
 if(ly0<0)
  {
   if((ly1-ly0)>CHKMAX_Y) return TRUE;
  }
 if(ly1<0)
  {
   if((ly0-ly1)>CHKMAX_Y) return TRUE;
  }

 return FALSE;
}

// Pete's way: a very easy (and hopefully fast) approach for lines
// without sqrt... using a small float -> short cast trick :)

#define VERTEX_OFFX 0.2f
#define VERTEX_OFFY 0.2f

BOOL offsetline(void)           
{
 short x0,x1,y0,y1,dx,dy;float px,py;

 if(bDisplayNotSet)
  SetOGLDisplaySettings(1);

 if(!(dwActFixes&16))
  {
   lx0=(short)(((int)lx0<<SIGNSHIFT)>>SIGNSHIFT);
   lx1=(short)(((int)lx1<<SIGNSHIFT)>>SIGNSHIFT);
   ly0=(short)(((int)ly0<<SIGNSHIFT)>>SIGNSHIFT);
   ly1=(short)(((int)ly1<<SIGNSHIFT)>>SIGNSHIFT);

   if(CheckCoord2()) return TRUE;
  }

 x0 = (lx0 + PSXDisplay.CumulOffset.x)+1;
 x1 = (lx1 + PSXDisplay.CumulOffset.x)+1;
 y0 = (ly0 + PSXDisplay.CumulOffset.y)+1;
 y1 = (ly1 + PSXDisplay.CumulOffset.y)+1;
 
 dx=x1-x0;
 dy=y1-y0;
 
 if(dx>=0)
  {
   if(dy>=0)
    {
     px=0.5f;
          if(dx>dy) py=-0.5f;
     else if(dx<dy) py= 0.5f;
     else           py= 0.0f;
    }
   else
    {
     py=-0.5f;
     dy=-dy;
          if(dx>dy) px= 0.5f;
     else if(dx<dy) px=-0.5f;
     else           px= 0.0f;
    }
  }
 else
  {
   if(dy>=0)
    {
     py=0.5f;
     dx=-dx;
          if(dx>dy) px=-0.5f;
     else if(dx<dy) px= 0.5f;
     else           px= 0.0f;
    }
   else
    {
     px=-0.5f;
          if(dx>dy) py=-0.5f;
     else if(dx<dy) py= 0.5f;
     else           py= 0.0f;
    }
  } 
 
 vertex[0].x=(short)((float)x0-px);
 vertex[3].x=(short)((float)x0+py);
 
 vertex[0].y=(short)((float)y0-py);
 vertex[3].y=(short)((float)y0-px);
 
 vertex[1].x=(short)((float)x1-py);
 vertex[2].x=(short)((float)x1+px);

 vertex[1].y=(short)((float)y1+px);
 vertex[2].y=(short)((float)y1+py);

 if(vertex[0].x==vertex[3].x &&                        // ortho rect? done
    vertex[1].x==vertex[2].x &&
    vertex[0].y==vertex[1].y &&
    vertex[2].y==vertex[3].y) return FALSE;
 if(vertex[0].x==vertex[1].x &&
    vertex[2].x==vertex[3].x &&
    vertex[0].y==vertex[3].y &&
    vertex[1].y==vertex[2].y) return FALSE;

 vertex[0].x-=VERTEX_OFFX;                             // otherwise a small offset
 vertex[0].y-=VERTEX_OFFY;                             // to get better accuracy
 vertex[1].x-=VERTEX_OFFX;
 vertex[1].y-=VERTEX_OFFY;
 vertex[2].x-=VERTEX_OFFX;
 vertex[2].y-=VERTEX_OFFY;
 vertex[3].x-=VERTEX_OFFX;
 vertex[3].y-=VERTEX_OFFY;

 return FALSE;
}

///////////////////////////////////////////////////////// 

BOOL offset2(void)
{
 if(bDisplayNotSet)
  SetOGLDisplaySettings(1);

 if(!(dwActFixes&16))
  {
   lx0=(short)(((int)lx0<<SIGNSHIFT)>>SIGNSHIFT);
   lx1=(short)(((int)lx1<<SIGNSHIFT)>>SIGNSHIFT);
   ly0=(short)(((int)ly0<<SIGNSHIFT)>>SIGNSHIFT);
   ly1=(short)(((int)ly1<<SIGNSHIFT)>>SIGNSHIFT);

   if(CheckCoord2()) return TRUE;
  }

 vertex[0].x=lx0+PSXDisplay.CumulOffset.x;
 vertex[1].x=lx1+PSXDisplay.CumulOffset.x;
 vertex[0].y=ly0+PSXDisplay.CumulOffset.y;
 vertex[1].y=ly1+PSXDisplay.CumulOffset.y;

 return FALSE;
}

///////////////////////////////////////////////////////// 

BOOL offset3(void)
{
 if(bDisplayNotSet)
  SetOGLDisplaySettings(1);

 if(!(dwActFixes&16))
  {
   lx0=(short)(((int)lx0<<SIGNSHIFT)>>SIGNSHIFT);
   lx1=(short)(((int)lx1<<SIGNSHIFT)>>SIGNSHIFT);
   lx2=(short)(((int)lx2<<SIGNSHIFT)>>SIGNSHIFT);
   ly0=(short)(((int)ly0<<SIGNSHIFT)>>SIGNSHIFT);
   ly1=(short)(((int)ly1<<SIGNSHIFT)>>SIGNSHIFT);
   ly2=(short)(((int)ly2<<SIGNSHIFT)>>SIGNSHIFT);

   if(CheckCoord3()) return TRUE;
  }

 vertex[0].x=lx0+PSXDisplay.CumulOffset.x;
 vertex[1].x=lx1+PSXDisplay.CumulOffset.x;
 vertex[2].x=lx2+PSXDisplay.CumulOffset.x;
 vertex[0].y=ly0+PSXDisplay.CumulOffset.y;
 vertex[1].y=ly1+PSXDisplay.CumulOffset.y;
 vertex[2].y=ly2+PSXDisplay.CumulOffset.y;

 return FALSE;
}

///////////////////////////////////////////////////////// 

BOOL offset4(void)
{
 if(bDisplayNotSet)
  SetOGLDisplaySettings(1);

 if(!(dwActFixes&16))
  {
   lx0=(short)(((int)lx0<<SIGNSHIFT)>>SIGNSHIFT);
   lx1=(short)(((int)lx1<<SIGNSHIFT)>>SIGNSHIFT);
   lx2=(short)(((int)lx2<<SIGNSHIFT)>>SIGNSHIFT);
   lx3=(short)(((int)lx3<<SIGNSHIFT)>>SIGNSHIFT);
   ly0=(short)(((int)ly0<<SIGNSHIFT)>>SIGNSHIFT);
   ly1=(short)(((int)ly1<<SIGNSHIFT)>>SIGNSHIFT);
   ly2=(short)(((int)ly2<<SIGNSHIFT)>>SIGNSHIFT);
   ly3=(short)(((int)ly3<<SIGNSHIFT)>>SIGNSHIFT);

   if(CheckCoord4()) return TRUE;
  }

 vertex[0].x=lx0+PSXDisplay.CumulOffset.x;
 vertex[1].x=lx1+PSXDisplay.CumulOffset.x;
 vertex[2].x=lx2+PSXDisplay.CumulOffset.x;
 vertex[3].x=lx3+PSXDisplay.CumulOffset.x;
 vertex[0].y=ly0+PSXDisplay.CumulOffset.y;
 vertex[1].y=ly1+PSXDisplay.CumulOffset.y;
 vertex[2].y=ly2+PSXDisplay.CumulOffset.y;
 vertex[3].y=ly3+PSXDisplay.CumulOffset.y;

 return FALSE;
}

///////////////////////////////////////////////////////// 

void offsetST(void)
{
 if(bDisplayNotSet)
  SetOGLDisplaySettings(1);

 if(!(dwActFixes&16))
  {
   lx0=(short)(((int)lx0<<SIGNSHIFT)>>SIGNSHIFT);
   ly0=(short)(((int)ly0<<SIGNSHIFT)>>SIGNSHIFT);

   if(lx0<-512 && PSXDisplay.DrawOffset.x<=-512)
    lx0+=2048;

   if(ly0<-512 && PSXDisplay.DrawOffset.y<=-512)
    ly0+=2048;
  }

 ly1 = ly0;
 ly2 = ly3 = ly0+sprtH;
 lx3 = lx0;
 lx1 = lx2 = lx0+sprtW;

 vertex[0].x=lx0+PSXDisplay.CumulOffset.x;
 vertex[1].x=lx1+PSXDisplay.CumulOffset.x;
 vertex[2].x=lx2+PSXDisplay.CumulOffset.x;
 vertex[3].x=lx3+PSXDisplay.CumulOffset.x;
 vertex[0].y=ly0+PSXDisplay.CumulOffset.y;
 vertex[1].y=ly1+PSXDisplay.CumulOffset.y;
 vertex[2].y=ly2+PSXDisplay.CumulOffset.y;
 vertex[3].y=ly3+PSXDisplay.CumulOffset.y;
}

///////////////////////////////////////////////////////// 

void offsetScreenUpload(int Position)
{
 if(bDisplayNotSet)
  SetOGLDisplaySettings(1);

 if(Position==-1)
  {
   int lmdx,lmdy;

   lmdx=xrUploadArea.x0;
   lmdy=xrUploadArea.y0;

   lx0-=lmdx;
   ly0-=lmdy;
   lx1-=lmdx;
   ly1-=lmdy;
   lx2-=lmdx;
   ly2-=lmdy;
   lx3-=lmdx;
   ly3-=lmdy;
  }
 else
 if(Position)
  {
   lx0-=PSXDisplay.DisplayPosition.x;
   ly0-=PSXDisplay.DisplayPosition.y;
   lx1-=PSXDisplay.DisplayPosition.x;
   ly1-=PSXDisplay.DisplayPosition.y;
   lx2-=PSXDisplay.DisplayPosition.x;
   ly2-=PSXDisplay.DisplayPosition.y;
   lx3-=PSXDisplay.DisplayPosition.x;
   ly3-=PSXDisplay.DisplayPosition.y;
  }
 else
  {
   lx0-=PreviousPSXDisplay.DisplayPosition.x;
   ly0-=PreviousPSXDisplay.DisplayPosition.y;
   lx1-=PreviousPSXDisplay.DisplayPosition.x;
   ly1-=PreviousPSXDisplay.DisplayPosition.y;
   lx2-=PreviousPSXDisplay.DisplayPosition.x;
   ly2-=PreviousPSXDisplay.DisplayPosition.y;
   lx3-=PreviousPSXDisplay.DisplayPosition.x;
   ly3-=PreviousPSXDisplay.DisplayPosition.y;
  }

 vertex[0].x=lx0 + PreviousPSXDisplay.Range.x0;
 vertex[1].x=lx1 + PreviousPSXDisplay.Range.x0;
 vertex[2].x=lx2 + PreviousPSXDisplay.Range.x0;
 vertex[3].x=lx3 + PreviousPSXDisplay.Range.x0;
 vertex[0].y=ly0 + PreviousPSXDisplay.Range.y0;
 vertex[1].y=ly1 + PreviousPSXDisplay.Range.y0;
 vertex[2].y=ly2 + PreviousPSXDisplay.Range.y0;
 vertex[3].y=ly3 + PreviousPSXDisplay.Range.y0;

 if(iUseMask)
  {
   vertex[0].z=vertex[1].z=vertex[2].z=vertex[3].z=gl_z;
   gl_z+=0.00004f;
  }
}
 
///////////////////////////////////////////////////////// 

void offsetBlk(void)
{
 if(bDisplayNotSet)
  SetOGLDisplaySettings(1);
                                            
 vertex[0].x=lx0-PSXDisplay.GDrawOffset.x + PreviousPSXDisplay.Range.x0;
 vertex[1].x=lx1-PSXDisplay.GDrawOffset.x + PreviousPSXDisplay.Range.x0;
 vertex[2].x=lx2-PSXDisplay.GDrawOffset.x + PreviousPSXDisplay.Range.x0;
 vertex[3].x=lx3-PSXDisplay.GDrawOffset.x + PreviousPSXDisplay.Range.x0;
 vertex[0].y=ly0-PSXDisplay.GDrawOffset.y + PreviousPSXDisplay.Range.y0;
 vertex[1].y=ly1-PSXDisplay.GDrawOffset.y + PreviousPSXDisplay.Range.y0;
 vertex[2].y=ly2-PSXDisplay.GDrawOffset.y + PreviousPSXDisplay.Range.y0;
 vertex[3].y=ly3-PSXDisplay.GDrawOffset.y + PreviousPSXDisplay.Range.y0;

 if(iUseMask)
  {
   vertex[0].z=vertex[1].z=vertex[2].z=vertex[3].z=gl_z;
   gl_z+=0.00004f;
  }
}

////////////////////////////////////////////////////////////////////////
// texture sow/tow calculations
////////////////////////////////////////////////////////////////////////

void assignTextureVRAMWrite(void)
{
#ifdef OWNSCALE

 vertex[0].sow=0.5f/ ST_FACVRAMX;
 vertex[0].tow=0.5f/ ST_FACVRAM;

 vertex[1].sow=(float)gl_ux[1]/ ST_FACVRAMX;
 vertex[1].tow=0.5f/ ST_FACVRAM;

 vertex[2].sow=(float)gl_ux[2]/ ST_FACVRAMX;
 vertex[2].tow=(float)gl_vy[2]/ ST_FACVRAM;

 vertex[3].sow=0.5f/ ST_FACVRAMX;
 vertex[3].tow=(float)gl_vy[3]/ ST_FACVRAM;

#else

 if(gl_ux[1]==255)
  {
   vertex[0].sow=(gl_ux[0]*255.99f)/255.0f;
   vertex[1].sow=(gl_ux[1]*255.99f)/255.0f;
   vertex[2].sow=(gl_ux[2]*255.99f)/255.0f;
   vertex[3].sow=(gl_ux[3]*255.99f)/255.0f;
  }
 else
  {
   vertex[0].sow=gl_ux[0];
   vertex[1].sow=gl_ux[1];
   vertex[2].sow=gl_ux[2];
   vertex[3].sow=gl_ux[3];
  }

 vertex[0].tow=gl_vy[0];
 vertex[1].tow=gl_vy[1];
 vertex[2].tow=gl_vy[2];
 vertex[3].tow=gl_vy[3];

#endif
}

GLuint  gLastTex=0;
GLuint  gLastFMode=(GLuint)-1;

///////////////////////////////////////////////////////// 

void assignTextureSprite(void)
{
 if(bUsingTWin)
  {
   vertex[0].sow=vertex[3].sow=(float)gl_ux[0]/TWin.UScaleFactor;
   vertex[1].sow=vertex[2].sow=(float)sSprite_ux2/TWin.UScaleFactor;
   vertex[0].tow=vertex[1].tow=(float)gl_vy[0]/TWin.VScaleFactor;
   vertex[2].tow=vertex[3].tow=(float)sSprite_vy2/TWin.VScaleFactor;
   gLastTex=gTexName;
  }
 else
  {
#ifdef OWNSCALE

   vertex[0].sow=vertex[3].sow=(float)gl_ux[0]     / ST_FACSPRITE;
   vertex[1].sow=vertex[2].sow=(float)sSprite_ux2  / ST_FACSPRITE;
   vertex[0].tow=vertex[1].tow=(float)gl_vy[0]     / ST_FACSPRITE;
   vertex[2].tow=vertex[3].tow=(float)sSprite_vy2  / ST_FACSPRITE;

#else
 
   vertex[0].sow=vertex[3].sow=gl_ux[0];
   vertex[1].sow=vertex[2].sow=sSprite_ux2;
   vertex[0].tow=vertex[1].tow=gl_vy[0];
   vertex[2].tow=vertex[3].tow=sSprite_vy2;

#endif

   if(iFilterType>2) 
    {
     if(gLastTex!=gTexName || gLastFMode!=0)
      {
       glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR); glError();
       glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR); glError();
       gLastTex=gTexName;gLastFMode=0;
      }
    }
  }

 if(usMirror & 0x1000) 
  {
   vertex[0].sow=vertex[1].sow;
   vertex[1].sow=vertex[2].sow=vertex[3].sow;
   vertex[3].sow=vertex[0].sow;
  }

 if(usMirror & 0x2000) 
  {
   vertex[0].tow=vertex[3].tow;
   vertex[2].tow=vertex[3].tow=vertex[1].tow;
   vertex[1].tow=vertex[0].tow;
  }

}

///////////////////////////////////////////////////////// 

void assignTexture3(void)
{
 if(bUsingTWin)
  {
   vertex[0].sow=(float)gl_ux[0]/TWin.UScaleFactor;
   vertex[0].tow=(float)gl_vy[0]/TWin.VScaleFactor;
   vertex[1].sow=(float)gl_ux[1]/TWin.UScaleFactor;
   vertex[1].tow=(float)gl_vy[1]/TWin.VScaleFactor;
   vertex[2].sow=(float)gl_ux[2]/TWin.UScaleFactor;
   vertex[2].tow=(float)gl_vy[2]/TWin.VScaleFactor;
   gLastTex=gTexName;
  }
 else
  {
#ifdef OWNSCALE
   vertex[0].sow=(float)gl_ux[0] / ST_FACTRI;
   vertex[0].tow=(float)gl_vy[0] / ST_FACTRI;
   vertex[1].sow=(float)gl_ux[1] / ST_FACTRI;

   vertex[1].tow=(float)gl_vy[1] / ST_FACTRI;
   vertex[2].sow=(float)gl_ux[2] / ST_FACTRI;
   vertex[2].tow=(float)gl_vy[2] / ST_FACTRI;
#else
   vertex[0].sow=gl_ux[0];
   vertex[0].tow=gl_vy[0];
   vertex[1].sow=gl_ux[1];
   vertex[1].tow=gl_vy[1];
   vertex[2].sow=gl_ux[2];
   vertex[2].tow=gl_vy[2];
#endif

   if(iFilterType>2) 
    {
     if(gLastTex!=gTexName || gLastFMode!=1)
      {
       glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR); glError();
       glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR); glError();
       gLastTex=gTexName;gLastFMode=1;
      }
    }

   if(iFilterType) 
    {
     float fxmin=256.0f,fxmax=0.0f,fymin=256.0f,fymax=0.0f;int i;
     for(i=0;i<3;i++)
      {
       if(vertex[i].sow<fxmin) fxmin=vertex[i].sow;
       if(vertex[i].tow<fymin) fymin=vertex[i].tow;
       if(vertex[i].sow>fxmax) fxmax=vertex[i].sow;
       if(vertex[i].tow>fymax) fymax=vertex[i].tow; 
      }

     for(i=0;i<3;i++)
      {
       if(vertex[i].sow==fxmin) vertex[i].sow+=ST_BFFACSORT;
       if(vertex[i].sow==fxmax) vertex[i].sow-=ST_BFFACSORT;
       if(vertex[i].tow==fymin) vertex[i].tow+=ST_BFFACSORT;
       if(vertex[i].tow==fymax) vertex[i].tow-=ST_BFFACSORT;
      }
    }
  }
}

///////////////////////////////////////////////////////// 

void assignTexture4(void)
{
 if(bUsingTWin)
  {
   vertex[0].sow=(float)gl_ux[0]/TWin.UScaleFactor;
   vertex[0].tow=(float)gl_vy[0]/TWin.VScaleFactor;
   vertex[1].sow=(float)gl_ux[1]/TWin.UScaleFactor;
   vertex[1].tow=(float)gl_vy[1]/TWin.VScaleFactor;
   vertex[2].sow=(float)gl_ux[2]/TWin.UScaleFactor;
   vertex[2].tow=(float)gl_vy[2]/TWin.VScaleFactor;
   vertex[3].sow=(float)gl_ux[3]/TWin.UScaleFactor;
   vertex[3].tow=(float)gl_vy[3]/TWin.VScaleFactor;
   gLastTex=gTexName;
  }
 else
  {
#ifdef OWNSCALE
   vertex[0].sow=(float)gl_ux[0] / ST_FAC;
   vertex[0].tow=(float)gl_vy[0] / ST_FAC;
   vertex[1].sow=(float)gl_ux[1] / ST_FAC;
   vertex[1].tow=(float)gl_vy[1] / ST_FAC;
   vertex[2].sow=(float)gl_ux[2] / ST_FAC;
   vertex[2].tow=(float)gl_vy[2] / ST_FAC;
   vertex[3].sow=(float)gl_ux[3] / ST_FAC;
   vertex[3].tow=(float)gl_vy[3] / ST_FAC;
#else
   vertex[0].sow=gl_ux[0];
   vertex[0].tow=gl_vy[0];
   vertex[1].sow=gl_ux[1];
   vertex[1].tow=gl_vy[1];
   vertex[2].sow=gl_ux[2];
   vertex[2].tow=gl_vy[2];
   vertex[3].sow=gl_ux[3];
   vertex[3].tow=gl_vy[3];
#endif

   if(iFilterType>2) 
    {
     if(gLastTex!=gTexName || gLastFMode!=1)
      {
       glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR); glError();
       glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR); glError();
       gLastTex=gTexName;gLastFMode=1;
      }
    }

   if(iFilterType) 
    {
     float fxmin=256.0f,fxmax=0.0f,fymin=256.0f,fymax=0.0f;int i;
     for(i=0;i<4;i++)
      {
       if(vertex[i].sow<fxmin) fxmin=vertex[i].sow;
       if(vertex[i].tow<fymin) fymin=vertex[i].tow;
       if(vertex[i].sow>fxmax) fxmax=vertex[i].sow;
       if(vertex[i].tow>fymax) fymax=vertex[i].tow; 
      }

     for(i=0;i<4;i++)
      {
       if(vertex[i].sow==fxmin) vertex[i].sow+=ST_BFFACSORT;
       if(vertex[i].sow==fxmax) vertex[i].sow-=ST_BFFACSORT;
       if(vertex[i].tow==fymin) vertex[i].tow+=ST_BFFACSORT;
       if(vertex[i].tow==fymax) vertex[i].tow-=ST_BFFACSORT;
      }
    }
  }
}

////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
// render pos / buffers
////////////////////////////////////////////////////////////////////////

#define EqualRect(pr1,pr2) ((pr1)->left==(pr2)->left && (pr1)->top==(pr2)->top && (pr1)->right==(pr2)->right && (pr1)->bottom==(pr2)->bottom)

////////////////////////////////////////////////////////////////////////
// SetDisplaySettings: "simply" calcs the new drawing area and updates
//                     the ogl clipping (scissor) 

BOOL bSetClip=FALSE;

void SetOGLDisplaySettings(BOOL DisplaySet)
{
 static RECT rprev={0,0,0,0};
 static RECT rC   ={0,0,0,0};
 static int iOldX=0;
 static int iOldY=0;
 RECT r;float XS,YS;

 bDisplayNotSet = FALSE;

 //----------------------------------------------------// that's a whole screen upload
 if(!DisplaySet)
  {
   RECT rX;
   PSXDisplay.GDrawOffset.x=0;
   PSXDisplay.GDrawOffset.y=0;

   PSXDisplay.CumulOffset.x = PSXDisplay.DrawOffset.x+PreviousPSXDisplay.Range.x0;
   PSXDisplay.CumulOffset.y = PSXDisplay.DrawOffset.y+PreviousPSXDisplay.Range.y0;

   rprev.left=rprev.left+1;

   rX=rRatioRect;
   rX.top=iResY-(rRatioRect.top+rRatioRect.bottom);

   if(bSetClip || !EqualRect(&rC,&rX))
    {
     rC=rX;
     glScissor(rC.left,rC.top,rC.right,rC.bottom); glError();
     //LOGE("glscissor:%d %d %d %d",rC.left,rC.top,rC.right,rC.bottom);
     bSetClip=FALSE; 
    }
   return;
  }
 //----------------------------------------------------// 

 PSXDisplay.GDrawOffset.y = PreviousPSXDisplay.DisplayPosition.y;
 PSXDisplay.GDrawOffset.x = PreviousPSXDisplay.DisplayPosition.x;
 PSXDisplay.CumulOffset.x = PSXDisplay.DrawOffset.x - PSXDisplay.GDrawOffset.x+PreviousPSXDisplay.Range.x0;
 PSXDisplay.CumulOffset.y = PSXDisplay.DrawOffset.y - PSXDisplay.GDrawOffset.y+PreviousPSXDisplay.Range.y0;

 r.top   =PSXDisplay.DrawArea.y0 - PreviousPSXDisplay.DisplayPosition.y;
 r.bottom=PSXDisplay.DrawArea.y1 - PreviousPSXDisplay.DisplayPosition.y;

 if(r.bottom<0 || r.top>=PSXDisplay.DisplayMode.y)
  {
   r.top   =PSXDisplay.DrawArea.y0 - PSXDisplay.DisplayPosition.y;
   r.bottom=PSXDisplay.DrawArea.y1 - PSXDisplay.DisplayPosition.y;
  }

 r.left  =PSXDisplay.DrawArea.x0 - PreviousPSXDisplay.DisplayPosition.x;
 r.right =PSXDisplay.DrawArea.x1 - PreviousPSXDisplay.DisplayPosition.x;

 if(r.right<0 || r.left>=PSXDisplay.DisplayMode.x)
  {
   r.left  =PSXDisplay.DrawArea.x0 - PSXDisplay.DisplayPosition.x;
   r.right =PSXDisplay.DrawArea.x1 - PSXDisplay.DisplayPosition.x;
  }

 if(!bSetClip && EqualRect(&r,&rprev) &&
    iOldX == PSXDisplay.DisplayMode.x &&
    iOldY == PSXDisplay.DisplayMode.y)
  return;

 rprev = r;
 iOldX = PSXDisplay.DisplayMode.x;
 iOldY = PSXDisplay.DisplayMode.y;

 XS=(float)rRatioRect.right/(float)PSXDisplay.DisplayMode.x;
 YS=(float)rRatioRect.bottom/(float)PSXDisplay.DisplayMode.y;

 if(PreviousPSXDisplay.Range.x0)
  {
   short s=PreviousPSXDisplay.Range.x0+PreviousPSXDisplay.Range.x1;

   r.left+=PreviousPSXDisplay.Range.x0+1;

   r.right+=PreviousPSXDisplay.Range.x0;

   if(r.left>s)  r.left=s;
   if(r.right>s) r.right=s;
  }

 if(PreviousPSXDisplay.Range.y0)
  {
   short s=PreviousPSXDisplay.Range.y0+PreviousPSXDisplay.Range.y1;

   r.top+=PreviousPSXDisplay.Range.y0+1;
   r.bottom+=PreviousPSXDisplay.Range.y0;

   if(r.top>s)    r.top=s;
   if(r.bottom>s) r.bottom=s;
  }

 // Set the ClipArea variables to reflect the new screen,
 // offset from zero (since it is a new display buffer)
 r.left   = (int)(((float)(r.left))      *XS);
 r.top    = (int)(((float)(r.top))       *YS);
 r.right  = (int)(((float)(r.right  + 1))*XS);
 r.bottom = (int)(((float)(r.bottom + 1))*YS);

 // Limit clip area to the screen size
 if (r.left   > iResX)   r.left   = iResX;
 if (r.left   < 0)       r.left   = 0;
 if (r.top    > iResY)   r.top    = iResY;
 if (r.top    < 0)       r.top    = 0;
 if (r.right  > iResX)   r.right  = iResX;
 if (r.right  < 0)       r.right  = 0;
 if (r.bottom > iResY)   r.bottom = iResY;
 if (r.bottom < 0)       r.bottom = 0;

 r.right -=r.left;
 r.bottom-=r.top;
 r.top=iResY-(r.top+r.bottom);

 r.left+=rRatioRect.left;
 r.top -=rRatioRect.top;

 if(bSetClip || !EqualRect(&r,&rC))
  {
   glScissor(r.left,r.top,r.right,r.bottom); glError();

   rC=r;
   bSetClip=FALSE;
  }
}

////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////

