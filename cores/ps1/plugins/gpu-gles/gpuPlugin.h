/***************************************************************************
                            gpu.h  -  description
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

#ifndef _GPU_PLUGIN_H
#define _GPU_PLUGIN_H

/////////////////////////////////////////////////////////////////////////////

#ifdef __cplusplus
extern "C" {
#endif 

#if !defined(_WINDOWS) && !defined(__NANOGL__)
#define glOrtho(x,y,z,xx,yy,zz) glOrthof(x,y,z,xx,yy,zz)
#endif

#define PRED(x)   ((x << 3) & 0xF8)
#define PBLUE(x)  ((x >> 2) & 0xF8)
#define PGREEN(x) ((x >> 7) & 0xF8)

#define RED(x) (x & 0xff)
#define BLUE(x) ((x>>16) & 0xff)
#define GREEN(x) ((x>>8) & 0xff)
#define COLOR(x) (x & 0xffffff)


#include "gpuExternals.h"

/////////////////////////////////////////////////////////////////////////////

#define CALLBACK

#define bool unsigned short

#if 0
typedef struct {
	unsigned int ulFreezeVersion;
	unsigned int ulStatus;
	unsigned int ulControl[256];
	unsigned char psxVRam[1024*1024*2];
} GPUFreeze_t;

long CALLBACK GPUinit();
long CALLBACK GPUshutdown();
long CALLBACK GPUopen(unsigned long *disp, char *cap, char *cfg);
long CALLBACK GPUclose();
unsigned long CALLBACK GPUreadData(void);
void CALLBACK GPUreadDataMem(unsigned long * pMem, int iSize);
unsigned long CALLBACK GPUreadStatus(void);
void CALLBACK GPUwriteData(unsigned long gdata);
void CALLBACK GPUwriteDataMem(unsigned long * pMem, int iSize);
void CALLBACK GPUwriteStatus(unsigned long gdata);
long CALLBACK GPUdmaChain(unsigned long * baseAddrL, unsigned long addr);
void CALLBACK GPUupdateLace(void);
void CALLBACK GPUmakeSnapshot(void);
long CALLBACK GPUfreeze(unsigned long ulGetFreezeData,GPUFreeze_t * pF);
long CALLBACK GPUgetScreenPic(unsigned char * pMem);
long CALLBACK GPUshowScreenPic(unsigned char * pMem);
//void CALLBACK GPUkeypressed(int keycode);
//void CALLBACK GPUdisplayText(s8 * pText);
//void CALLBACK GPUclearDynarec(void (CALLBACK *callback)(void));
long CALLBACK GPUconfigure(void);
long CALLBACK GPUtest(void);
void CALLBACK GPUabout(void);
#endif

void           DoSnapShot(void);
void		   GPUvSinc(void);
void           updateDisplay(void);
void           updateFrontDisplay(void);
void           SetAutoFrameCap(void);
void           SetAspectRatio(void);
void           CheckVRamRead(int x, int y, int dx, int dy, bool bFront);
void           CheckVRamReadEx(int x, int y, int dx, int dy);
void           SetFixes(void);

void PaintPicDot(unsigned char * p,unsigned char c);
//void DrawNumBorPic(unsigned char *pMem, int lSelectedSlot);
void ResizeWindow();

////////////////////////////////////////////////////////////////////////////
#ifdef __cplusplus
}
#endif 


#endif // _GPU_INTERNALS_H
