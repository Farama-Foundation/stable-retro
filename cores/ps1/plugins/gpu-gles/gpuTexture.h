/***************************************************************************
                          texture.h  -  description
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

#ifndef _GPU_TEXTURE_H_
#define _GPU_TEXTURE_H_

#ifdef __cplusplus
extern "C" {
#endif


#define TEXTUREPAGESIZE 256*256

void           InitializeTextureStore();
void           CleanupTextureStore();
GLuint         LoadTextureWnd(int pageid,int TextureMode,unsigned int GivenClutId);
GLuint         LoadTextureMovie(void);
void           InvalidateTextureArea(int imageX0,int imageY0,int imageX1,int imageY1);
void           InvalidateTextureAreaEx(void);
void           LoadTexturePage(int pageid, int mode, short cx, short cy);
void           ResetTextureArea(BOOL bDelTex);
GLuint         SelectSubTextureS(int TextureMode, unsigned int GivenClutId);
void           CheckTextureMemory(void);


void           LoadSubTexturePage(int pageid, int mode, short cx, short cy);
void           LoadSubTexturePageSort(int pageid, int mode, short cx, short cy);
void           LoadPackedSubTexturePage(int pageid, int mode, short cx, short cy);
void           LoadPackedSubTexturePageSort(int pageid, int mode, short cx, short cy);
unsigned int   XP8RGBA(unsigned int BGR);
unsigned int   XP8RGBAEx(unsigned int BGR);
unsigned int   XP8RGBA_0(unsigned int BGR);
unsigned int   XP8RGBAEx_0(unsigned int BGR);
unsigned int   XP8BGRA_0(unsigned int BGR);
unsigned int   XP8BGRAEx_0(unsigned int BGR);
unsigned int   XP8RGBA_1(unsigned int BGR);
unsigned int   XP8RGBAEx_1(unsigned int BGR);
unsigned int   XP8BGRA_1(unsigned int BGR);
unsigned int   XP8BGRAEx_1(unsigned int BGR);
unsigned int   P8RGBA(unsigned int BGR);
unsigned int   P8BGRA(unsigned int BGR);
unsigned int   CP8RGBA_0(unsigned int BGR);
unsigned int   CP8RGBAEx_0(unsigned int BGR);
unsigned int   CP8BGRA_0(unsigned int BGR);
unsigned int   CP8BGRAEx_0(unsigned int BGR);
unsigned int   CP8RGBA(unsigned int BGR);
unsigned int   CP8RGBAEx(unsigned int BGR);
unsigned short XP5RGBA (unsigned short BGR);
unsigned short XP5RGBA_0 (unsigned short BGR);
unsigned short XP5RGBA_1 (unsigned short BGR);
unsigned short P5RGBA (unsigned short BGR);
unsigned short CP5RGBA_0 (unsigned short BGR);
unsigned short XP4RGBA (unsigned short BGR);
unsigned short XP4RGBA_0 (unsigned short BGR);
unsigned short XP4RGBA_1 (unsigned short BGR);
unsigned short P4RGBA (unsigned short BGR);
unsigned short CP4RGBA_0 (unsigned short BGR);

#ifdef __cplusplus
}
#endif


#endif // _TEXTURE_H_
