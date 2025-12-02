/***************************************************************************
                            draw.h  -  description
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

#ifndef _GL_DRAW_H_
#define _GL_DRAW_H_


#ifdef __cplusplus

extern "C" {
#endif
	


// internally used defines

#define GPUCOMMAND(x) ((x>>24) & 0xff)
#define RED(x) (x & 0xff)
#define BLUE(x) ((x>>16) & 0xff)
#define GREEN(x) ((x>>8) & 0xff)
#define COLOR(x) (x & 0xffffff)

// prototypes

#ifdef _WINDOWS
BOOL bSetupPixelFormat(HDC hDC);
#endif

int  GLinitialize(void *ext_gles_display, void *ext_gles_surface);
void GLcleanup();
#ifdef _WINDOWS
BOOL offset2(void);
BOOL offset3(void);
BOOL offset4(void);
BOOL offsetline(void);
#else
unsigned short offset2(void);
unsigned short offset3(void);
unsigned short offset4(void);
unsigned short offsetline(void);
#endif
void offsetST(void);
void offsetBlk(void);
void offsetScreenUpload(int Position);
void assignTexture3(void);
void assignTexture4(void);
void assignTextureSprite(void);
void assignTextureVRAMWrite(void);
void SetOGLDisplaySettings (unsigned short DisplaySet);
void ReadConfig(void);
void WriteConfig(void);
void SetExtGLFuncs(void);
///////////////////////////////////////////////////////////////////////

#ifdef __cplusplus
}
#endif

#endif // _GL_DRAW_H_
