/***************************************************************************
                          draw.c  -  description
                             -------------------
    begin                : Sun Oct 28 2001
    copyright            : (C) 2001 by Pete Bernert
    email                : BlackDove@addcom.de
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

#define _IN_DRAW

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/cursorfont.h>

#include "gpu.h"

// misc globals
int            iResX=640;
int            iResY=480;
long           lLowerpart;
BOOL           bIsFirstFrame = TRUE;
BOOL           bCheckMask = FALSE;
unsigned short sSetMask = 0;
unsigned long  lSetMask = 0;
int            iDesktopCol = 16;
int            iShowFPS = 0;
int            iWinSize; 
int            iMaintainAspect = 0;
int            iUseNoStretchBlt = 0;
int            iFastFwd = 0;
//int            iDebugMode = 0;
int            iFVDisplay = 0;
PSXPoint_t     ptCursorPoint[8];
unsigned short usCursorActive = 0;
unsigned long  ulKeybits;

int            iWindowMode=1;
int            iColDepth=32;
char           szDispBuf[64];
char           szMenuBuf[36];
char           szDebugText[512];
void InitMenu(void) {}
void CloseMenu(void) {}

//unsigned int   LUT16to32[65536];
//unsigned int   RGBtoYUV[65536];

#include <sys/ipc.h>
#include <sys/shm.h>
#include <X11/extensions/Xv.h>
#include <X11/extensions/Xvlib.h>
#include <X11/extensions/XShm.h>
int xv_port = -1;
int xv_id = -1;
int xv_depth = 0;
int yuv_port = -1;
int yuv_id = -1;
int use_yuv = 0;
int xv_vsync = 0;

XShmSegmentInfo shminfo;
int finalw,finalh;

typedef struct {
#define MWM_HINTS_DECORATIONS   2
  long flags;
  long functions;
  long decorations;
  long input_mode;
} MotifWmHints;

extern XvImage  *XvShmCreateImage(Display*, XvPortID, int, char*, int, int, XShmSegmentInfo*);

#include <time.h>

// prototypes
void hq2x_32( unsigned char * srcPtr, DWORD srcPitch, unsigned char * dstPtr, int width, int height);
void hq3x_32( unsigned char * srcPtr,  DWORD srcPitch, unsigned char * dstPtr, int width, int height);

////////////////////////////////////////////////////////////////////////
// generic 2xSaI helpers
////////////////////////////////////////////////////////////////////////

void *         pSaISmallBuff=NULL;
void *         pSaIBigBuff=NULL;

#define GET_RESULT(A, B, C, D) ((A != C || A != D) - (B != C || B != D))

static __inline int GetResult1(DWORD A, DWORD B, DWORD C, DWORD D, DWORD E)
{
 int x = 0;
 int y = 0;
 int r = 0;
 if (A == C) x+=1; else if (B == C) y+=1;
 if (A == D) x+=1; else if (B == D) y+=1;
 if (x <= 1) r+=1; 
 if (y <= 1) r-=1;
 return r;
}

static __inline int GetResult2(DWORD A, DWORD B, DWORD C, DWORD D, DWORD E) 
{
 int x = 0; 
 int y = 0;
 int r = 0;
 if (A == C) x+=1; else if (B == C) y+=1;
 if (A == D) x+=1; else if (B == D) y+=1;
 if (x <= 1) r-=1; 
 if (y <= 1) r+=1;
 return r;
}

#define colorMask8     0x00FEFEFE
#define lowPixelMask8  0x00010101
#define qcolorMask8    0x00FCFCFC
#define qlowpixelMask8 0x00030303

#define INTERPOLATE8(A, B) ((((A & colorMask8) >> 1) + ((B & colorMask8) >> 1) + (A & B & lowPixelMask8)))
#define Q_INTERPOLATE8(A, B, C, D) (((((A & qcolorMask8) >> 2) + ((B & qcolorMask8) >> 2) + ((C & qcolorMask8) >> 2) + ((D & qcolorMask8) >> 2) \
	+ ((((A & qlowpixelMask8) + (B & qlowpixelMask8) + (C & qlowpixelMask8) + (D & qlowpixelMask8)) >> 2) & qlowpixelMask8))))


void Super2xSaI_ex8(unsigned char *srcPtr, DWORD srcPitch,
	            unsigned char  *dstBitmap, int width, int height)
{
 DWORD dstPitch        = srcPitch<<1;
 DWORD srcPitchHalf    = srcPitch>>1;
 int   finWidth        = srcPitch>>2;
 DWORD line;
 DWORD *dP;
 DWORD *bP;
 int iXA,iXB,iXC,iYA,iYB,iYC,finish;
 DWORD color4, color5, color6;
 DWORD color1, color2, color3;
 DWORD colorA0, colorA1, colorA2, colorA3,
       colorB0, colorB1, colorB2, colorB3,
       colorS1, colorS2;
 DWORD product1a, product1b,
       product2a, product2b;

 finalw=width<<1;
 finalh=height<<1;

 line = 0;

  {
   for (; height; height-=1)
	{
     bP = (DWORD *)srcPtr;
	 dP = (DWORD *)(dstBitmap + line*dstPitch);
     for (finish = width; finish; finish -= 1 )
      {
//---------------------------------------    B1 B2
//                                         4  5  6 S2
//                                         1  2  3 S1
//                                           A1 A2
       if(finish==finWidth) iXA=0;
       else                 iXA=1;
       if(finish>4) {iXB=1;iXC=2;}
       else
       if(finish>3) {iXB=1;iXC=1;}
       else         {iXB=0;iXC=0;}
       if(line==0)  {iYA=0;}
       else         {iYA=finWidth;}
       if(height>4) {iYB=finWidth;iYC=srcPitchHalf;}
       else
       if(height>3) {iYB=finWidth;iYC=finWidth;}
       else         {iYB=0;iYC=0;}

       colorB0 = *(bP- iYA - iXA);
       colorB1 = *(bP- iYA);
       colorB2 = *(bP- iYA + iXB);
       colorB3 = *(bP- iYA + iXC);

       color4 = *(bP  - iXA);
       color5 = *(bP);
       color6 = *(bP  + iXB);
       colorS2 = *(bP + iXC);

       color1 = *(bP  + iYB  - iXA);
       color2 = *(bP  + iYB);
       color3 = *(bP  + iYB  + iXB);
       colorS1= *(bP  + iYB  + iXC);

       colorA0 = *(bP + iYC - iXA);
       colorA1 = *(bP + iYC);
       colorA2 = *(bP + iYC + iXB);
       colorA3 = *(bP + iYC + iXC);

       if (color2 == color6 && color5 != color3)
        {
         product2b = product1b = color2;
        }
       else
       if (color5 == color3 && color2 != color6)
        {
         product2b = product1b = color5;
        }
       else
       if (color5 == color3 && color2 == color6)
        {
         register int r = 0;

         r += GET_RESULT ((color6&0x00ffffff), (color5&0x00ffffff), (color1&0x00ffffff),  (colorA1&0x00ffffff));
         r += GET_RESULT ((color6&0x00ffffff), (color5&0x00ffffff), (color4&0x00ffffff),  (colorB1&0x00ffffff));
         r += GET_RESULT ((color6&0x00ffffff), (color5&0x00ffffff), (colorA2&0x00ffffff), (colorS1&0x00ffffff));
         r += GET_RESULT ((color6&0x00ffffff), (color5&0x00ffffff), (colorB2&0x00ffffff), (colorS2&0x00ffffff));

         if (r > 0)
          product2b = product1b = color6;
         else
         if (r < 0)
          product2b = product1b = color5;
         else
          {
           product2b = product1b = INTERPOLATE8(color5, color6);
          }
        }
       else
        {
         if (color6 == color3 && color3 == colorA1 && color2 != colorA2 && color3 != colorA0)
             product2b = Q_INTERPOLATE8 (color3, color3, color3, color2);
         else
         if (color5 == color2 && color2 == colorA2 && colorA1 != color3 && color2 != colorA3)
             product2b = Q_INTERPOLATE8 (color2, color2, color2, color3);
         else
             product2b = INTERPOLATE8 (color2, color3);

         if (color6 == color3 && color6 == colorB1 && color5 != colorB2 && color6 != colorB0)
             product1b = Q_INTERPOLATE8 (color6, color6, color6, color5);
         else
         if (color5 == color2 && color5 == colorB2 && colorB1 != color6 && color5 != colorB3)
             product1b = Q_INTERPOLATE8 (color6, color5, color5, color5);
         else
             product1b = INTERPOLATE8 (color5, color6);
        }

       if (color5 == color3 && color2 != color6 && color4 == color5 && color5 != colorA2)
        product2a = INTERPOLATE8(color2, color5);
       else
       if (color5 == color1 && color6 == color5 && color4 != color2 && color5 != colorA0)
        product2a = INTERPOLATE8(color2, color5);
       else
        product2a = color2;

       if (color2 == color6 && color5 != color3 && color1 == color2 && color2 != colorB2)
        product1a = INTERPOLATE8(color2, color5);
       else
       if (color4 == color2 && color3 == color2 && color1 != color5 && color2 != colorB0)
        product1a = INTERPOLATE8(color2, color5);
       else
        product1a = color5;

       *dP=product1a;
       *(dP+1)=product1b;
       *(dP+(srcPitchHalf))=product2a;
       *(dP+1+(srcPitchHalf))=product2b;

       bP += 1;
       dP += 2;
      }//end of for ( finish= width etc..)

     line += 2;
     srcPtr += srcPitch;
	}; //endof: for (; height; height--)
  }
}

////////////////////////////////////////////////////////////////////////

void Std2xSaI_ex8(unsigned char *srcPtr, DWORD srcPitch,
                  unsigned char *dstBitmap, int width, int height)
{
 DWORD dstPitch        = srcPitch<<1;
 DWORD srcPitchHalf    = srcPitch>>1;
 int   finWidth        = srcPitch>>2;
 DWORD line;
 DWORD *dP;
 DWORD *bP;
 int iXA,iXB,iXC,iYA,iYB,iYC,finish;

 finalw=width<<1;
 finalh=height<<1;

 DWORD colorA, colorB;
 DWORD colorC, colorD,
       colorE, colorF, colorG, colorH,
       colorI, colorJ, colorK, colorL,
       colorM, colorN, colorO, colorP;
 DWORD product, product1, product2;

 line = 0;

  {
   for (; height; height-=1)
	{
     bP = (DWORD *)srcPtr;
	 dP = (DWORD *)(dstBitmap + line*dstPitch);
     for (finish = width; finish; finish -= 1 )
      {
//---------------------------------------
// Map of the pixels:                    I|E F|J
//                                       G|A B|K
//                                       H|C D|L
//                                       M|N O|P
       if(finish==finWidth) iXA=0;
       else                 iXA=1;
       if(finish>4) {iXB=1;iXC=2;}
       else
       if(finish>3) {iXB=1;iXC=1;}
       else         {iXB=0;iXC=0;}
       if(line==0)  {iYA=0;}
       else         {iYA=finWidth;}
       if(height>4) {iYB=finWidth;iYC=srcPitchHalf;}
       else
       if(height>3) {iYB=finWidth;iYC=finWidth;}
       else         {iYB=0;iYC=0;}

       colorI = *(bP- iYA - iXA);
       colorE = *(bP- iYA);
       colorF = *(bP- iYA + iXB);
       colorJ = *(bP- iYA + iXC);

       colorG = *(bP  - iXA);
       colorA = *(bP);
       colorB = *(bP  + iXB);
       colorK = *(bP + iXC);

       colorH = *(bP  + iYB  - iXA);
       colorC = *(bP  + iYB);
       colorD = *(bP  + iYB  + iXB);
       colorL = *(bP  + iYB  + iXC);

       colorM = *(bP + iYC - iXA);
       colorN = *(bP + iYC);
       colorO = *(bP + iYC + iXB);
       colorP = *(bP + iYC + iXC);


       if((colorA == colorD) && (colorB != colorC))
        {
         if(((colorA == colorE) && (colorB == colorL)) ||
            ((colorA == colorC) && (colorA == colorF) && 
             (colorB != colorE) && (colorB == colorJ)))
          {
           product = colorA;
          }
         else
          {
           product = INTERPOLATE8(colorA, colorB);
          }

         if(((colorA == colorG) && (colorC == colorO)) ||
            ((colorA == colorB) && (colorA == colorH) && 
             (colorG != colorC) && (colorC == colorM)))
          {
           product1 = colorA;
          }
         else
          {
           product1 = INTERPOLATE8(colorA, colorC);
          }
         product2 = colorA;
        }
       else
       if((colorB == colorC) && (colorA != colorD))
        {
         if(((colorB == colorF) && (colorA == colorH)) ||
            ((colorB == colorE) && (colorB == colorD) && 
             (colorA != colorF) && (colorA == colorI)))
          {
           product = colorB;
          }
         else
          {
           product = INTERPOLATE8(colorA, colorB);
          }

         if(((colorC == colorH) && (colorA == colorF)) ||
            ((colorC == colorG) && (colorC == colorD) && 
             (colorA != colorH) && (colorA == colorI)))
          {
           product1 = colorC;
          }
         else
          {
           product1=INTERPOLATE8(colorA, colorC);
          }
         product2 = colorB;
        }
       else
       if((colorA == colorD) && (colorB == colorC))
        {
         if (colorA == colorB)
          {
           product = colorA;
           product1 = colorA;
           product2 = colorA;
          }
         else
          {
           register int r = 0;
           product1 = INTERPOLATE8(colorA, colorC);
           product = INTERPOLATE8(colorA, colorB);

           r += GetResult1 (colorA&0x00FFFFFF, colorB&0x00FFFFFF, colorG&0x00FFFFFF, colorE&0x00FFFFFF, colorI&0x00FFFFFF);
           r += GetResult2 (colorB&0x00FFFFFF, colorA&0x00FFFFFF, colorK&0x00FFFFFF, colorF&0x00FFFFFF, colorJ&0x00FFFFFF);
           r += GetResult2 (colorB&0x00FFFFFF, colorA&0x00FFFFFF, colorH&0x00FFFFFF, colorN&0x00FFFFFF, colorM&0x00FFFFFF);
           r += GetResult1 (colorA&0x00FFFFFF, colorB&0x00FFFFFF, colorL&0x00FFFFFF, colorO&0x00FFFFFF, colorP&0x00FFFFFF);

           if (r > 0)
            product2 = colorA;
           else
           if (r < 0)
            product2 = colorB;
           else
            {
             product2 = Q_INTERPOLATE8(colorA, colorB, colorC, colorD);
            }
          }
        }
       else
        {
         product2 = Q_INTERPOLATE8(colorA, colorB, colorC, colorD);

         if ((colorA == colorC) && (colorA == colorF) && 
             (colorB != colorE) && (colorB == colorJ))
          {
           product = colorA;
          }
         else
         if ((colorB == colorE) && (colorB == colorD) && (colorA != colorF) && (colorA == colorI))
          {
           product = colorB;
          }
         else
          {
           product = INTERPOLATE8(colorA, colorB);
          }

         if ((colorA == colorB) && (colorA == colorH) && 
             (colorG != colorC) && (colorC == colorM))
          {
           product1 = colorA;
          }
         else
         if ((colorC == colorG) && (colorC == colorD) && 
             (colorA != colorH) && (colorA == colorI))
          {
           product1 = colorC;
          }
         else
          {
           product1 = INTERPOLATE8(colorA, colorC);
          }
        }

//////////////////////////

       *dP=colorA;
       *(dP+1)=product;
       *(dP+(srcPitchHalf))=product1;
       *(dP+1+(srcPitchHalf))=product2;

       bP += 1;
       dP += 2;
      }//end of for ( finish= width etc..)

     line += 2;
     srcPtr += srcPitch;
	}; //endof: for (; height; height--)
  }
}

////////////////////////////////////////////////////////////////////////

void SuperEagle_ex8(unsigned char *srcPtr, DWORD srcPitch,
	                unsigned char  *dstBitmap, int width, int height)
{
 DWORD dstPitch        = srcPitch<<1;
 DWORD srcPitchHalf    = srcPitch>>1;
 int   finWidth        = srcPitch>>2;
 DWORD line;
 DWORD *dP;
 DWORD *bP;
 int iXA,iXB,iXC,iYA,iYB,iYC,finish;
 DWORD color4, color5, color6;
 DWORD color1, color2, color3;
 DWORD colorA1, colorA2, 
       colorB1, colorB2,
       colorS1, colorS2;
 DWORD product1a, product1b,
       product2a, product2b;

 finalw=width<<1;
 finalh=height<<1;

 line = 0;

  {
   for (; height; height-=1)
	{
     bP = (DWORD *)srcPtr;
	 dP = (DWORD *)(dstBitmap + line*dstPitch);
     for (finish = width; finish; finish -= 1 )
      {
       if(finish==finWidth) iXA=0;
       else                 iXA=1;
       if(finish>4) {iXB=1;iXC=2;}
       else
       if(finish>3) {iXB=1;iXC=1;}
       else         {iXB=0;iXC=0;}
       if(line==0)  {iYA=0;}
       else         {iYA=finWidth;}
       if(height>4) {iYB=finWidth;iYC=srcPitchHalf;}
       else
       if(height>3) {iYB=finWidth;iYC=finWidth;}
       else         {iYB=0;iYC=0;}

       colorB1 = *(bP- iYA);
       colorB2 = *(bP- iYA + iXB);

       color4 = *(bP  - iXA);
       color5 = *(bP);
       color6 = *(bP  + iXB);
       colorS2 = *(bP + iXC);

       color1 = *(bP  + iYB  - iXA);
       color2 = *(bP  + iYB);
       color3 = *(bP  + iYB  + iXB);
       colorS1= *(bP  + iYB  + iXC);

       colorA1 = *(bP + iYC);
       colorA2 = *(bP + iYC + iXB);

       if(color2 == color6 && color5 != color3)
        {
         product1b = product2a = color2;
         if((color1 == color2) ||
            (color6 == colorB2))
          {
           product1a = INTERPOLATE8(color2, color5);
           product1a = INTERPOLATE8(color2, product1a);
          }
         else
          {
           product1a = INTERPOLATE8(color5, color6);
          }
 
         if((color6 == colorS2) ||
            (color2 == colorA1))
          {
           product2b = INTERPOLATE8(color2, color3);
           product2b = INTERPOLATE8(color2, product2b);
          }
         else
          {
           product2b = INTERPOLATE8(color2, color3);
          }
        }
       else
       if (color5 == color3 && color2 != color6)
        {
         product2b = product1a = color5;

         if ((colorB1 == color5) ||
             (color3 == colorS1))
          {
           product1b = INTERPOLATE8(color5, color6);
           product1b = INTERPOLATE8(color5, product1b);
          }
         else
          {
           product1b = INTERPOLATE8(color5, color6);
          }

         if ((color3 == colorA2) ||
             (color4 == color5))
          {
           product2a = INTERPOLATE8(color5, color2);
           product2a = INTERPOLATE8(color5, product2a);
          }
         else
          {
           product2a = INTERPOLATE8(color2, color3);
          }
        }
       else
       if (color5 == color3 && color2 == color6)
        {
         register int r = 0;

         r += GET_RESULT ((color6&0x00ffffff), (color5&0x00ffffff), (color1&0x00ffffff),  (colorA1&0x00ffffff));
         r += GET_RESULT ((color6&0x00ffffff), (color5&0x00ffffff), (color4&0x00ffffff),  (colorB1&0x00ffffff));
         r += GET_RESULT ((color6&0x00ffffff), (color5&0x00ffffff), (colorA2&0x00ffffff), (colorS1&0x00ffffff));
         r += GET_RESULT ((color6&0x00ffffff), (color5&0x00ffffff), (colorB2&0x00ffffff), (colorS2&0x00ffffff));

         if (r > 0)
          {
           product1b = product2a = color2;
           product1a = product2b = INTERPOLATE8(color5, color6);
          }
         else
         if (r < 0)
          {
           product2b = product1a = color5;
           product1b = product2a = INTERPOLATE8(color5, color6);
          }
         else
          {
           product2b = product1a = color5;
           product1b = product2a = color2;
          }
        }
       else
        {
         product2b = product1a = INTERPOLATE8(color2, color6);
         product2b = Q_INTERPOLATE8(color3, color3, color3, product2b);
         product1a = Q_INTERPOLATE8(color5, color5, color5, product1a);

         product2a = product1b = INTERPOLATE8(color5, color3);
         product2a = Q_INTERPOLATE8(color2, color2, color2, product2a);
         product1b = Q_INTERPOLATE8(color6, color6, color6, product1b);
        }

////////////////////////////////

       *dP=product1a;
       *(dP+1)=product1b;
       *(dP+(srcPitchHalf))=product2a;
       *(dP+1+(srcPitchHalf))=product2b;

       bP += 1;
       dP += 2;
      }//end of for ( finish= width etc..)

     line += 2;
     srcPtr += srcPitch;
	}; //endof: for (; height; height--)
  }
}

/////////////////////////

//#include <assert.h>

static __inline void scale2x_32_def_whole(uint32_t*  dst0, uint32_t* dst1, const uint32_t* src0, const uint32_t* src1, const uint32_t* src2, unsigned count)
{

	//assert(count >= 2);

	// first pixel
	if (src0[0] != src2[0] && src1[0] != src1[1]) {
		dst0[0] = src1[0] == src0[0] ? src0[0] : src1[0];
		dst0[1] = src1[1] == src0[0] ? src0[0] : src1[0];
		dst1[0] = src1[0] == src2[0] ? src2[0] : src1[0];
		dst1[1] = src1[1] == src2[0] ? src2[0] : src1[0];
	} else {
		dst0[0] = src1[0];
		dst0[1] = src1[0];
		dst1[0] = src1[0];
		dst1[1] = src1[0];
	}
	++src0;
	++src1;
	++src2;
	dst0 += 2;
	dst1 += 2;

	// central pixels
	count -= 2;
	while (count) {
		if (src0[0] != src2[0] && src1[-1] != src1[1]) {
			dst0[0] = src1[-1] == src0[0] ? src0[0] : src1[0];
			dst0[1] = src1[1] == src0[0] ? src0[0] : src1[0];
			dst1[0] = src1[-1] == src2[0] ? src2[0] : src1[0];
			dst1[1] = src1[1] == src2[0] ? src2[0] : src1[0];
		} else {
			dst0[0] = src1[0];
			dst0[1] = src1[0];
			dst1[0] = src1[0];
			dst1[1] = src1[0];
		}

		++src0;
		++src1;
		++src2;
		dst0 += 2;
		dst1 += 2;
		--count;
	}

	// last pixel
	if (src0[0] != src2[0] && src1[-1] != src1[0]) {
		dst0[0] = src1[-1] == src0[0] ? src0[0] : src1[0];
		dst0[1] = src1[0] == src0[0] ? src0[0] : src1[0];
		dst1[0] = src1[-1] == src2[0] ? src2[0] : src1[0];
		dst1[1] = src1[0] == src2[0] ? src2[0] : src1[0];
	} else {
		dst0[0] = src1[0];
		dst0[1] = src1[0];
		dst1[0] = src1[0];
		dst1[1] = src1[0];
	}
}

void Scale2x_ex8(unsigned char *srcPtr, DWORD srcPitch,
				 unsigned char  *dstPtr, int width, int height)
{
	//const int srcpitch = srcPitch;
	const int dstPitch = srcPitch<<1;

	int count = height;

	finalw=width<<1;
	finalh=height<<1;

	uint32_t  *dst0 = (uint32_t  *)dstPtr;
	uint32_t  *dst1 = dst0 + (dstPitch >> 2);

	uint32_t  *src0 = (uint32_t  *)srcPtr;
	uint32_t  *src1 = src0 + (srcPitch >> 2);
	uint32_t  *src2 = src1 + (srcPitch >> 2);
	scale2x_32_def_whole(dst0, dst1, src0, src0, src1, width);

	count -= 2;
	while(count) {
		dst0 += dstPitch >> 1;
		dst1 += dstPitch >> 1;
		scale2x_32_def_whole(dst0, dst1, src0, src0, src1, width);
		src0 = src1;
		src1 = src2;
		src2 += srcPitch >> 2;
		--count;
	}
	dst0 += dstPitch >> 1;
	dst1 += dstPitch >> 1;
	scale2x_32_def_whole(dst0, dst1, src0, src1, src1, width);

}

////////////////////////////////////////////////////////////////////////

static __inline void scale3x_32_def_whole(uint32_t* dst0, uint32_t* dst1, uint32_t* dst2, const uint32_t* src0, const uint32_t* src1, const uint32_t* src2, unsigned count)
{
	//assert(count >= 2);

	//first pixel
	if (src0[0] != src2[0] && src1[0] != src1[1]) {
		dst0[0] = src1[0];
		dst0[1] = (src1[0] == src0[0] && src1[0] != src0[1]) || (src1[1] == src0[0] && src1[0] != src0[0]) ? src0[0] : src1[0];
		dst0[2] = src1[1] == src0[0] ? src1[1] : src1[0];
		dst1[0] = (src1[0] == src0[0] && src1[0] != src2[0]) || (src1[0] == src2[0] && src1[0] != src0[0]) ? src1[0] : src1[0];
		dst1[1] = src1[0];
		dst1[2] = (src1[1] == src0[0] && src1[0] != src2[1]) || (src1[1] == src2[0] && src1[0] != src0[1]) ? src1[1] : src1[0];
		dst2[0] = src1[0];
		dst2[1] = (src1[0] == src2[0] && src1[0] != src2[1]) || (src1[1] == src2[0] && src1[0] != src2[0]) ? src2[0] : src1[0];
		dst2[2] = src1[1] == src2[0] ? src1[1] : src1[0];
	} else {
		dst0[0] = src1[0];
		dst0[1] = src1[0];
		dst0[2] = src1[0];
		dst1[0] = src1[0];
		dst1[1] = src1[0];
		dst1[2] = src1[0];
		dst2[0] = src1[0];
		dst2[1] = src1[0];
		dst2[2] = src1[0];
	}
	++src0;
	++src1;
	++src2;
	dst0 += 3;
	dst1 += 3;
	dst2 += 3;

	//central pixels
	count -= 2;
	while (count) {
		if (src0[0] != src2[0] && src1[-1] != src1[1]) {
			dst0[0] = src1[-1] == src0[0] ? src1[-1] : src1[0];
			dst0[1] = (src1[-1] == src0[0] && src1[0] != src0[1]) || (src1[1] == src0[0] && src1[0] != src0[-1]) ? src0[0] : src1[0];
			dst0[2] = src1[1] == src0[0] ? src1[1] : src1[0];
			dst1[0] = (src1[-1] == src0[0] && src1[0] != src2[-1]) || (src1[-1] == src2[0] && src1[0] != src0[-1]) ? src1[-1] : src1[0];
			dst1[1] = src1[0];
			dst1[2] = (src1[1] == src0[0] && src1[0] != src2[1]) || (src1[1] == src2[0] && src1[0] != src0[1]) ? src1[1] : src1[0];
			dst2[0] = src1[-1] == src2[0] ? src1[-1] : src1[0];
			dst2[1] = (src1[-1] == src2[0] && src1[0] != src2[1]) || (src1[1] == src2[0] && src1[0] != src2[-1]) ? src2[0] : src1[0];
			dst2[2] = src1[1] == src2[0] ? src1[1] : src1[0];
		} else {
			dst0[0] = src1[0];
			dst0[1] = src1[0];
			dst0[2] = src1[0];
			dst1[0] = src1[0];
			dst1[1] = src1[0];
			dst1[2] = src1[0];
			dst2[0] = src1[0];
			dst2[1] = src1[0];
			dst2[2] = src1[0];
		}

		++src0;
		++src1;
		++src2;
		dst0 += 3;
		dst1 += 3;
		dst2 += 3;
		--count;
	}

	// last pixel
	if (src0[0] != src2[0] && src1[-1] != src1[0]) {
		dst0[0] = src1[-1] == src0[0] ? src1[-1] : src1[0];
		dst0[1] = (src1[-1] == src0[0] && src1[0] != src0[0]) || (src1[0] == src0[0] && src1[0] != src0[-1]) ? src0[0] : src1[0];
		dst0[2] = src1[0];
		dst1[0] = (src1[-1] == src0[0] && src1[0] != src2[-1]) || (src1[-1] == src2[0] && src1[0] != src0[-1]) ? src1[-1] : src1[0];
		dst1[1] = src1[0];
		dst1[2] = (src1[0] == src0[0] && src1[0] != src2[0]) || (src1[0] == src2[0] && src1[0] != src0[0]) ? src1[0] : src1[0];
		dst2[0] = src1[-1] == src2[0] ? src1[-1] : src1[0];
		dst2[1] = (src1[-1] == src2[0] && src1[0] != src2[0]) || (src1[0] == src2[0] && src1[0] != src2[-1]) ? src2[0] : src1[0];
		dst2[2] = src1[0];
	} else {
		dst0[0] = src1[0];
		dst0[1] = src1[0];
		dst0[2] = src1[0];
		dst1[0] = src1[0];
		dst1[1] = src1[0];
		dst1[2] = src1[0];
		dst2[0] = src1[0];
		dst2[1] = src1[0];
		dst2[2] = src1[0];
	}
}


void Scale3x_ex8(unsigned char *srcPtr, DWORD srcPitch,
				 unsigned char  *dstPtr, int width, int height)
{
	int count = height;

	int dstPitch = srcPitch*3;
	int dstRowPixels = dstPitch>>2;	

	finalw=width*3;
	finalh=height*3;

	uint32_t  *dst0 = (uint32_t  *)dstPtr;
	uint32_t  *dst1 = dst0 + dstRowPixels;
	uint32_t  *dst2 = dst1 + dstRowPixels;

	uint32_t  *src0 = (uint32_t  *)srcPtr;
	uint32_t  *src1 = src0 + (srcPitch >> 2);
	uint32_t  *src2 = src1 + (srcPitch >> 2);
	scale3x_32_def_whole(dst0, dst1, dst2, src0, src0, src2, width);

	count -= 2;
	while(count) {
		dst0 += dstRowPixels*3;
		dst1 += dstRowPixels*3;
		dst2 += dstRowPixels*3;

		scale3x_32_def_whole(dst0, dst1, dst2, src0, src1, src2, width);
		src0 = src1;
		src1 = src2;
		src2 += srcPitch >> 2;
		--count;
	}

	dst0 += dstRowPixels*3;
	dst1 += dstRowPixels*3;
	dst2 += dstRowPixels*3;

	scale3x_32_def_whole(dst0, dst1, dst2, src0, src1, src1, width);
}


////////////////////////////////////////////////////////////////////////

#ifndef MAX
#define MAX(a,b)    (((a) > (b)) ? (a) : (b))
#define MIN(a,b)    (((a) < (b)) ? (a) : (b))
#endif


////////////////////////////////////////////////////////////////////////
// X STUFF :)
////////////////////////////////////////////////////////////////////////


static Cursor        cursor;
XVisualInfo          vi;
static XVisualInfo   *myvisual;
Display              *display;
static Colormap      colormap;
Window        window;
static GC            hGC;
static XImage      * Ximage;
static XvImage     * XCimage; 
static XImage      * XFimage; 
static XImage      * XPimage=0 ; 
char *               Xpixels;
char *               pCaptionText;

static int fx=0;


static Atom xv_intern_atom_if_exists( Display *display, char const * atom_name )
{
  XvAttribute * attributes;
  int attrib_count,i;
  Atom xv_atom = None;

  attributes = XvQueryPortAttributes( display, xv_port, &attrib_count );
  if( attributes!=NULL )
  {
    for ( i = 0; i < attrib_count; ++i )
    {
      if ( strcmp(attributes[i].name, atom_name ) == 0 )
      {
        xv_atom = XInternAtom( display, atom_name, False );
        break; // found what we want, break out
      }
    }
    XFree( attributes );
  }

  return xv_atom;
}



// close display

void DestroyDisplay(void)
{
 if(display)
  {
   XFreeColormap(display, colormap);
   if(hGC) 
    {
     XFreeGC(display,hGC);
     hGC = 0;
    }
   if(Ximage)
    {
     XDestroyImage(Ximage);
     Ximage=0;
    }
   if(XCimage) 
    { 
     XFree(XCimage); 
     XCimage=0; 
    } 
   if(XFimage) 
    { 
     XDestroyImage(XFimage);
     XFimage=0; 
    } 

	XShmDetach(display,&shminfo);
	shmdt(shminfo.shmaddr);
	shmctl(shminfo.shmid,IPC_RMID,NULL);

  Atom atom_vsync = xv_intern_atom_if_exists(display, "XV_SYNC_TO_VBLANK");
  if (atom_vsync != None) {
	XvSetPortAttribute(display, xv_port, atom_vsync, xv_vsync);
  }

   XSync(display,False);

   XCloseDisplay(display);
  }
}

static int depth=0;
int root_window_id=0;


// Create display

void CreateDisplay(void)
{
 XSetWindowAttributes winattr;
 int                  myscreen;
 Screen *             screen;
 XEvent               event;
 XSizeHints           hints;
 XWMHints             wm_hints;
 MotifWmHints         mwmhints;
 Atom                 mwmatom;

 Atom			delwindow;

 XGCValues            gcv;
 int i;

 int ret, j, p;
 int formats;
 unsigned int p_num_adaptors=0, p_num_ports=0;

 XvAdaptorInfo		*ai;
 XvImageFormatValues	*fo;

 // Open display
 display = XOpenDisplay(NULL);

 if (!display)
  {
   fprintf (stderr,"Failed to open display!!!\n");
   DestroyDisplay();
   return;
  }

 myscreen=DefaultScreen(display);

 // desktop fullscreen switch
 if (!iWindowMode) fx = 1;

 screen=DefaultScreenOfDisplay(display);

 root_window_id=RootWindow(display,DefaultScreen(display));

  //Look for an Xvideo RGB port
  ret = XvQueryAdaptors(display, root_window_id, &p_num_adaptors, &ai);
  if (ret != Success) {
    if (ret == XvBadExtension)
      printf("XvBadExtension returned at XvQueryExtension.\n");
    else
      if (ret == XvBadAlloc)
	printf("XvBadAlloc returned at XvQueryExtension.\n");
      else
	printf("other error happaned at XvQueryAdaptors.\n");

    exit(-1);
  }

  depth = DefaultDepth(display, myscreen);

  for (i = 0; i < p_num_adaptors; i++) {
	p_num_ports = ai[i].base_id + ai[i].num_ports;
	for (p = ai[i].base_id; p < p_num_ports; p++) {
		fo = XvListImageFormats(display, p, &formats);
		for (j = 0; j < formats; j++) {
			//backup YUV mode
			//hmm, should I bother check guid == 55595659-0000-0010-8000-00aa00389b71?
			//and check byte order?   fo[j].byte_order == LSBFirst
#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
			if ( fo[j].type == XvYUV && fo[j].bits_per_pixel == 16 && fo[j].format == XvPacked && strncmp("YUYV", fo[j].component_order, 5) == 0 )
#else
			if ( fo[j].type == XvYUV && fo[j].bits_per_pixel == 16 && fo[j].format == XvPacked && strncmp("UYVY", fo[j].component_order, 5) == 0 )
#endif
			{
				yuv_port = p;
				yuv_id = fo[j].id;
			}
			if (fo[j].type == XvRGB && fo[j].bits_per_pixel == 32)
			{
				xv_port = p;
				xv_id = fo[j].id;
				xv_depth = fo[j].depth;
				printf("RGB mode found.  id: %x, depth: %d\n", xv_id, xv_depth);

				if (xv_depth != depth) {
					printf("Warning: Depth does not match screen depth (%d)\n", depth);
				}
				else {
					//break out of loops
					j = formats;
					p = p_num_ports;
					i = p_num_adaptors;
				}
			}
		}
		if (fo)
			XFree(fo);
	}
  }
  if (p_num_adaptors > 0)
    XvFreeAdaptorInfo(ai);
  if (xv_port == -1 && yuv_port == -1)
  {
	printf("RGB & YUV not found.  Quitting.\n");
	exit(-1);
  }
  else if (xv_port == -1 && yuv_port != -1)
  {
	use_yuv = 1;
	printf("RGB not found.  Using YUV.\n");
	xv_port = yuv_port;
	xv_id = yuv_id;
  }
  else if (xv_depth && xv_depth != depth && yuv_port != -1)
  {
	use_yuv = 1;
	printf("Acceptable RGB mode not found.  Using YUV.\n");
	xv_port = yuv_port;
	xv_id = yuv_id;
  }

  Atom atom_vsync = xv_intern_atom_if_exists(display, "XV_SYNC_TO_VBLANK");
  if (atom_vsync != None) {
	XvGetPortAttribute(display, xv_port, atom_vsync, &xv_vsync);
	XvSetPortAttribute(display, xv_port, atom_vsync, 0);
  }

myvisual = 0;

if(XMatchVisualInfo(display,myscreen, depth, TrueColor, &vi))
	myvisual = &vi;

if (!myvisual)
{
	fprintf(stderr,"Failed to obtain visual!\n");
	DestroyDisplay();
	return;
}

 if(myvisual->red_mask==0x00007c00 &&
    myvisual->green_mask==0x000003e0 &&
    myvisual->blue_mask==0x0000001f)
     {iColDepth=15;}
 else
 if(myvisual->red_mask==0x0000f800 &&
    myvisual->green_mask==0x000007e0 &&
    myvisual->blue_mask==0x0000001f)
     {iColDepth=16;}
 else
 if(myvisual->red_mask==0x00ff0000 &&
    myvisual->green_mask==0x0000ff00 &&
    myvisual->blue_mask==0x000000ff)
     {iColDepth=32;}
 else
  {
   iColDepth=0;
/*   fprintf(stderr,"COLOR DEPTH NOT SUPPORTED!\n");
   fprintf(stderr,"r: %08lx\n",myvisual->red_mask);
   fprintf(stderr,"g: %08lx\n",myvisual->green_mask);
   fprintf(stderr,"b: %08lx\n",myvisual->blue_mask);
   DestroyDisplay();
   return;*/
  }

 // pffff... much work for a simple blank cursor... oh, well...
 if(iWindowMode) cursor=XCreateFontCursor(display,XC_trek);
 else
  {
   Pixmap p1,p2;
   XImage * img;
   XColor b,w;
   char * idata;
   XGCValues GCv;
   GC        GCc;

   memset(&b,0,sizeof(XColor));
   memset(&w,0,sizeof(XColor));
   idata=(char *)malloc(8);
   memset(idata,0,8);

   p1=XCreatePixmap(display,RootWindow(display,myvisual->screen),8,8,1);
   p2=XCreatePixmap(display,RootWindow(display,myvisual->screen),8,8,1);

   img = XCreateImage(display,myvisual->visual,
                      1,XYBitmap,0,idata,8,8,8,1);

   GCv.function   = GXcopy;
   GCv.foreground = ~0;
   GCv.background =  0;
   GCv.plane_mask = AllPlanes;
   GCc = XCreateGC(display,p1,
                   (GCFunction|GCForeground|GCBackground|GCPlaneMask),&GCv);

   XPutImage(display, p1,GCc,img,0,0,0,0,8,8);
   XPutImage(display, p2,GCc,img,0,0,0,0,8,8);
   XFreeGC(display, GCc);

   cursor = XCreatePixmapCursor(display,p1,p2,&b,&w,0,0);

   XFreePixmap(display,p1);
   XFreePixmap(display,p2);
   XDestroyImage(img); // will free idata as well
  }

 colormap=XCreateColormap(display,root_window_id,
                          myvisual->visual,AllocNone);

 winattr.background_pixel=0;
 winattr.border_pixel=WhitePixelOfScreen(screen);
 winattr.bit_gravity=ForgetGravity;
 winattr.win_gravity=NorthWestGravity;
 winattr.backing_store=NotUseful;

 winattr.override_redirect=False;
 winattr.save_under=False;
 winattr.event_mask=0;
 winattr.do_not_propagate_mask=0;
 winattr.colormap=colormap;
 winattr.cursor=None;

 window=XCreateWindow(display,root_window_id,
                      0,0,iResX,iResY,
                      0,myvisual->depth,
                      InputOutput,myvisual->visual,
                      CWBorderPixel | CWBackPixel |
                      CWEventMask | CWDontPropagate |
                      CWColormap | CWCursor,
                      &winattr);

 if(!window)
  {
   fprintf(stderr,"Failed in XCreateWindow()!!!\n");
   DestroyDisplay();
   return;
  }

 delwindow = XInternAtom(display,"WM_DELETE_WINDOW",0);
 XSetWMProtocols(display, window, &delwindow, 1);

 hints.flags=USPosition|USSize;
 hints.base_width = iResX;
 hints.base_height = iResY;

 wm_hints.input=1;
 wm_hints.flags=InputHint;

 XSetWMHints(display,window,&wm_hints);
 XSetWMNormalHints(display,window,&hints);
 if(pCaptionText)
      XStoreName(display,window,pCaptionText);
 else XStoreName(display,window,"P.E.Op.S SoftX PSX Gpu");

 XDefineCursor(display,window,cursor);

 // hack to get rid of window title bar 
 if (fx)
  {
   mwmhints.flags=MWM_HINTS_DECORATIONS;
   mwmhints.decorations=0;
   mwmatom=XInternAtom(display,"_MOTIF_WM_HINTS",0);
   XChangeProperty(display,window,mwmatom,mwmatom,32,
                   PropModeReplace,(unsigned char *)&mwmhints,4);
  }

 // key stuff
 XSelectInput(display,
              window,
              FocusChangeMask | ExposureMask |
              KeyPressMask | KeyReleaseMask
             );

 XMapRaised(display,window);
 XClearWindow(display,window);
 XWindowEvent(display,window,ExposureMask,&event);

 if (fx) // fullscreen
  {
   XResizeWindow(display,window,screen->width,screen->height);

   hints.min_width   = hints.max_width = hints.base_width = screen->width;
   hints.min_height= hints.max_height = hints.base_height = screen->height;

   XSetWMNormalHints(display,window,&hints);

   // set the window layer for GNOME
   {
    XEvent xev;

    memset(&xev, 0, sizeof(xev));
    xev.xclient.type = ClientMessage;
    xev.xclient.serial = 0;
    xev.xclient.send_event = 1;
    xev.xclient.message_type = XInternAtom(display, "_NET_WM_STATE", 0);
    xev.xclient.window = window;
    xev.xclient.format = 32;
    xev.xclient.data.l[0] = 1;
    xev.xclient.data.l[1] = XInternAtom(display, "_NET_WM_STATE_FULLSCREEN", 0);
    xev.xclient.data.l[2] = 0;
    xev.xclient.data.l[3] = 0;
    xev.xclient.data.l[4] = 0;

    XSendEvent(display, root_window_id, 0,
      SubstructureRedirectMask | SubstructureNotifyMask, &xev);
   }
  }

 gcv.graphics_exposures = False;
 hGC = XCreateGC(display,window,
                 GCGraphicsExposures, &gcv);
 if(!hGC) 
  {
   fprintf(stderr,"No gfx context!!!\n");
   DestroyDisplay();
  }



 Xpixels = (char *)malloc(220*15*4);
 memset(Xpixels,255,220*15*4);
 XFimage = XCreateImage(display,myvisual->visual, 
                      depth, ZPixmap, 0, 
                      (char *)Xpixels,  
                      220, 15, 
                      depth>16 ? 32 : 16,
                      0); 

 Xpixels = (char *)malloc(8*8*4);
 memset(Xpixels,0,8*8*4);
 XCimage = XvCreateImage(display,xv_port,xv_id,
                      (char *)Xpixels, 8, 8);


/*
Allocate max that could be needed:
Big(est?) PSX res: 640x512
32bpp (times 4)
2xsai func= 3xwidth,3xheight
= approx 11.8mb
*/
shminfo.shmid = shmget(IPC_PRIVATE, 640*512*4*3*3, IPC_CREAT | 0777);
shminfo.shmaddr = shmat(shminfo.shmid, 0, 0);
shminfo.readOnly = 0;
 
 if (!XShmAttach(display, &shminfo)) {
    printf("XShmAttach failed !\n");
    exit (-1);
 }
}

void (*p2XSaIFunc) (unsigned char *, DWORD, unsigned char *, int, int);
unsigned char *pBackBuffer = 0;

void BlitScreen32(unsigned char *surf, int32_t x, int32_t y)
{
 unsigned char *pD;
 unsigned int startxy;
 uint32_t lu;
 unsigned short s;
 unsigned short row, column;
 unsigned short dx = PreviousPSXDisplay.Range.x1;
 unsigned short dy = PreviousPSXDisplay.DisplayMode.y;

 int32_t lPitch = PSXDisplay.DisplayMode.x << 2;

 uint32_t *destpix;

 if (PreviousPSXDisplay.Range.y0) // centering needed?
  {
   memset(surf, 0, (PreviousPSXDisplay.Range.y0 >> 1) * lPitch);

   dy -= PreviousPSXDisplay.Range.y0;
   surf += (PreviousPSXDisplay.Range.y0 >> 1) * lPitch;

   memset(surf + dy * lPitch,
          0, ((PreviousPSXDisplay.Range.y0 + 1) >> 1) * lPitch);
  }

 if (PreviousPSXDisplay.Range.x0)
  {
   for (column = 0; column < dy; column++)
    {
     destpix = (uint32_t *)(surf + (column * lPitch));
     memset(destpix, 0, PreviousPSXDisplay.Range.x0 << 2);
    }
   surf += PreviousPSXDisplay.Range.x0 << 2;
  }

 if (PSXDisplay.RGB24)
  {
   for (column = 0; column < dy; column++)
    {
     startxy = ((1024) * (column + y)) + x;
     pD = (unsigned char *)&psxVuw[startxy];
     destpix = (uint32_t *)(surf + (column * lPitch));
     for (row = 0; row < dx; row++)
      {
       lu = *((uint32_t *)pD);
       destpix[row] = 
          0xff000000 | (RED(lu) << 16) | (GREEN(lu) << 8) | (BLUE(lu));
       pD += 3;
      }
    }
  }
 else
  {
   for (column = 0;column<dy;column++)
    {
     startxy = (1024 * (column + y)) + x;
     destpix = (uint32_t *)(surf + (column * lPitch));
     for (row = 0; row < dx; row++)
      {
       s = GETLE16(&psxVuw[startxy++]);
       destpix[row] = 
          (((s << 19) & 0xf80000) | ((s << 6) & 0xf800) | ((s >> 7) & 0xf8)) | 0xff000000;
      }
    }
  }
}

void BlitToYUV(unsigned char * surf,int32_t x,int32_t y)
{
 unsigned char * pD;
 unsigned int startxy;
 uint32_t lu;unsigned short s;
 unsigned short row,column;
 unsigned short dx = PreviousPSXDisplay.Range.x1;
 unsigned short dy = PreviousPSXDisplay.DisplayMode.y;
 int Y,U,V, R,G,B;

 int32_t lPitch = PSXDisplay.DisplayMode.x << 2;
 uint32_t *destpix;

 if (PreviousPSXDisplay.Range.y0) // centering needed?
  {
   for (column = 0; column < (PreviousPSXDisplay.Range.y0 >> 1); column++)
    {
     destpix = (uint32_t *)(surf + column * lPitch);
     for (row = 0; row < dx; row++)
     {
      destpix[row] = (4 << 24) | (128 << 16) | (4 << 8) | 128;
     }
    }

   dy -= PreviousPSXDisplay.Range.y0;
   surf += (PreviousPSXDisplay.Range.y0 >> 1) * lPitch;

   for (column = 0; column < (PreviousPSXDisplay.Range.y0 + 1) >> 1; column++)
    {
     destpix = (uint32_t *)(surf + (dy + column) * lPitch);
     for (row = 0; row < dx; row++)
     {
      destpix[row] = (4 << 24) | (128 << 16) | (4 << 8) | 128;
     }
    }
  }

 if (PreviousPSXDisplay.Range.x0)
  {
   for (column = 0; column < dy; column++)
    {
     destpix = (uint32_t *)(surf + (column * lPitch));
     for (row = 0; row < PreviousPSXDisplay.Range.x0; row++)
      {
       destpix[row] = (4 << 24) | (128 << 16) | (4 << 8) | 128;
      }
    }
   surf += PreviousPSXDisplay.Range.x0 << 2;
  }

 if (PSXDisplay.RGB24)
  {
   for (column = 0; column < dy; column++)
    {
     startxy = (1024 * (column + y)) + x;
     pD = (unsigned char *)&psxVuw[startxy];
     destpix = (uint32_t *)(surf + (column * lPitch));
     for (row = 0; row < dx; row++)
      {
       lu = *((uint32_t *)pD);

       R = RED(lu);
       G = GREEN(lu);
       B = BLUE(lu);

       Y = min(abs(R * 2104 + G * 4130 + B * 802 + 4096 + 131072) >> 13, 235);
       U = min(abs(R * -1214 + G * -2384 + B * 3598 + 4096 + 1048576) >> 13, 240);
       V = min(abs(R * 3598 + G * -3013 + B * -585 + 4096 + 1048576) >> 13, 240);

#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
       destpix[row] = Y << 24 | U << 16 | Y << 8 | V;
#else
       destpix[row] = Y << 24 | V << 16 | Y << 8 | U;
#endif
       pD += 3;
      }
    }
  }
 else
  {
   for (column = 0; column < dy; column++)
    {
     startxy = (1024 * (column + y)) + x;
     destpix = (uint32_t *)(surf + (column * lPitch));
     for (row = 0; row < dx; row++)
      {
       s = GETLE16(&psxVuw[startxy++]);

       R = (s << 3) &0xf8;
       G = (s >> 2) &0xf8;
       B = (s >> 7) &0xf8;

       Y = min(abs(R * 2104 + G * 4130 + B * 802 + 4096 + 131072) >> 13, 235);
       U = min(abs(R * -1214 + G * -2384 + B * 3598 + 4096 + 1048576) >> 13, 240);
       V = min(abs(R * 3598 + G * -3013 + B * -585 + 4096 + 1048576) >> 13, 240);

#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
       destpix[row] = Y << 24 | U << 16 | Y << 8 | V;
#else
       destpix[row] = Y << 24 | V << 16 | Y << 8 | U;
#endif
      }
    }
  }
}

//dst will have half the pitch (32bit to 16bit)
void RGB2YUV(uint32_t *s, int width, int height, uint32_t *d)
{
	int x,y;
	int R,G,B, Y1,Y2,U,V;

	for (y=0; y<height; y++) {
		for(x=0; x<width>>1; x++) {
			R = (*s >> 16) & 0xff;
			G = (*s >> 8) & 0xff;
			B = *s & 0xff;
			s++;

			Y1 = min(abs(R * 2104 + G * 4130 + B * 802 + 4096 + 131072) >> 13, 235);
			U = min(abs(R * -1214 + G * -2384 + B * 3598 + 4096 + 1048576) >> 13, 240);
			V = min(abs(R * 3598 + G * -3013 + B * -585 + 4096 + 1048576) >> 13, 240);

			R = (*s >> 16) & 0xff;
			G = (*s >> 8) & 0xff;
			B = *s & 0xff;
			s++;

			Y2 = min(abs(R * 2104 + G * 4130 + B * 802 + 4096 + 131072) >> 13, 235);

#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
			*d = V | Y2 << 8 | U << 16 | Y1 << 24;
#else
			*d = U | Y1 << 8 | V << 16 | Y2 << 24;
#endif
			d++;
		}
	}
}

time_t tStart;

//Note: dest x,y,w,h are both input and output variables
inline void MaintainAspect(unsigned int *dx,unsigned int *dy,unsigned int *dw,unsigned int *dh)
{
	//Currently just 4/3 aspect ratio
	int t;

	if (*dw * 3 > *dh * 4) {
		t = *dh * 4.0f / 3;	//new width aspect
		*dx = (*dw - t) / 2;	//centering
		*dw = t;
	} else {
		t = *dw * 3.0f / 4;
		*dy = (*dh - t) / 2;
		*dh = t;
	}
}

void DoBufferSwap(void)
{
	Screen *screen;
	Window _dw;
	XvImage *xvi;
	unsigned int dstx, dsty, srcy = 0;
	unsigned int _d, _w, _h;	//don't care about _d

	finalw = PSXDisplay.DisplayMode.x;
	finalh = PSXDisplay.DisplayMode.y;

	if (finalw == 0 || finalh == 0)
		return;

	XSync(display,False);

	if(use_yuv) {
		if (iUseNoStretchBlt==0 || finalw > 320 || finalh > 256) {
			BlitToYUV((unsigned char *)shminfo.shmaddr, PSXDisplay.DisplayPosition.x, PSXDisplay.DisplayPosition.y);
			finalw <<= 1;
		} else {
			BlitScreen32((unsigned char *)pBackBuffer, PSXDisplay.DisplayPosition.x, PSXDisplay.DisplayPosition.y);
			p2XSaIFunc(pBackBuffer, finalw<<2, (unsigned char *)pSaIBigBuff,finalw,finalh);
			RGB2YUV( (uint32_t*)pSaIBigBuff, finalw, finalh, (uint32_t*)shminfo.shmaddr);
		}
	} else if(iUseNoStretchBlt==0 || finalw > 320 || finalh > 256) {
		BlitScreen32((unsigned char *)shminfo.shmaddr, PSXDisplay.DisplayPosition.x, PSXDisplay.DisplayPosition.y);
	} else {
		BlitScreen32((unsigned char *)pBackBuffer, PSXDisplay.DisplayPosition.x, PSXDisplay.DisplayPosition.y);
		p2XSaIFunc(pBackBuffer, finalw<<2, (unsigned char *)shminfo.shmaddr,finalw,finalh);
	}

	XGetGeometry(display, window, &_dw, (int *)&_d, (int *)&_d, &_w, &_h, &_d, &_d);
	if (use_yuv) {
		xvi = XvShmCreateImage(display, yuv_port, yuv_id, 0, finalw, finalh, &shminfo);
	} else
		xvi = XvShmCreateImage(display, xv_port, xv_id, 0, finalw, finalh, &shminfo);

	xvi->data = shminfo.shmaddr;

	screen=DefaultScreenOfDisplay(display);	
	//screennum = DefaultScreen(display);

	if (!iWindowMode) {
		_w = screen->width;
		_h = screen->height;
	}

	dstx = 0;
	dsty = 0;

	if (iMaintainAspect)
		MaintainAspect(&dstx, &dsty, &_w, &_h);

	if (ulKeybits&KEY_SHOWFPS)	//to avoid flicker, don't paint overtop FPS bar
	{
		srcy = 15 * finalh / _h;
		dsty += 15;
	}

	XvShmPutImage(display, xv_port, window, hGC, xvi, 
		0,srcy,		//src x,y
		finalw,finalh,	//src w,h
		dstx,dsty,	//dst x,y
		_w,_h,		//dst w,h
		1
		);

	if(ulKeybits&KEY_SHOWFPS) //DisplayText();               // paint menu text 
	{
		if(szDebugText[0] && ((time(NULL) - tStart) < 2))
		{
			strcpy(szDispBuf,szDebugText);
		}
		else 
		{
			szDebugText[0]=0;
			strcat(szDispBuf,szMenuBuf);
		}

		//XPutImage(display,window,hGC, XFimage, 
		//          0, 0, 0, 0, 220,15);
		XFree(xvi);
		xvi = XvCreateImage(display, xv_port, xv_id, XFimage->data, 220, 15);
		XvPutImage(display, xv_port, window, hGC, xvi, 
			0,0,		//src x,y
			220,15,		//src w,h
			0,0,		//dst x,y
			220,15		//dst w,h
			);

		XDrawString(display,window,hGC,2,13,szDispBuf,strlen(szDispBuf));
	}
	
	//if(XPimage) DisplayPic();
	

	XFree(xvi);
}

void DoClearScreenBuffer(void)                         // CLEAR DX BUFFER
{
 Window _dw;
 unsigned int _d, _w, _h;	//don't care about _d

 XGetGeometry(display, window, &_dw, (int *)&_d, (int *)&_d, &_w, &_h, &_d, &_d);

 XvPutImage(display, xv_port, window, hGC, XCimage,
           0, 0, 8, 8, 0, 0, _w, _h);
 //XSync(display,False);
}

void DoClearFrontBuffer(void)                          // CLEAR DX BUFFER
{/*
 XPutImage(display,window,hGC, XCimage,
           0, 0, 0, 0, iResX, iResY);
 XSync(display,False);*/
}

int Xinitialize()
{
   iDesktopCol=32;


 if(iUseNoStretchBlt>0)
  {
   pBackBuffer=(unsigned char *)malloc(640*512*sizeof(uint32_t));
   memset(pBackBuffer,0,640*512*sizeof(uint32_t));
   if (use_yuv) {
    pSaIBigBuff=malloc(640*512*4*3*3);
    memset(pSaIBigBuff,0,640*512*4*3*3);
   }
  }

 p2XSaIFunc=NULL;

#if 0
 if(iUseNoStretchBlt==1)
  {
   p2XSaIFunc=Std2xSaI_ex8;
  }

 if(iUseNoStretchBlt==2)
  {
   p2XSaIFunc=Super2xSaI_ex8;
  }

 if(iUseNoStretchBlt==3)
  {
   p2XSaIFunc=SuperEagle_ex8;
  }

 if(iUseNoStretchBlt==4)
  {
   p2XSaIFunc=Scale2x_ex8;
  }
 if(iUseNoStretchBlt==5)
  {
   p2XSaIFunc=Scale3x_ex8;
  }
 if(iUseNoStretchBlt==6)
  {
   p2XSaIFunc=hq2x_32;
  }
 if(iUseNoStretchBlt==7)
  {
   p2XSaIFunc=hq3x_32;
  }
#endif

 bUsingTWin=FALSE;

 InitMenu();

 bIsFirstFrame = FALSE;                                // done

 if(iShowFPS)
  {
   iShowFPS=0;
   ulKeybits|=KEY_SHOWFPS;
   szDispBuf[0]=0;
   //BuildDispMenu(0);
  }

 return 0;
}

void Xcleanup()                                        // X CLEANUP
{
 CloseMenu();

 if(iUseNoStretchBlt>0) 
  {
   if(pBackBuffer)  free(pBackBuffer);
   pBackBuffer=0;
   if(pSaIBigBuff) free(pSaIBigBuff);
   pSaIBigBuff=0;
  }
}

unsigned long ulInitDisplay(void)
{
 CreateDisplay();                                      // x stuff
 Xinitialize();                                        // init x
 return (unsigned long)display;
}

void CloseDisplay(void)
{
 Xcleanup();                                           // cleanup dx
 DestroyDisplay();
}

void CreatePic(unsigned char * pMem)
{
 unsigned char * p=(unsigned char *)malloc(128*96*4);
 unsigned char * ps; int x,y;

 ps=p;

 if(iDesktopCol==16)
  {
   unsigned short s;
   for(y=0;y<96;y++)
    {
     for(x=0;x<128;x++)
      {
       s=(*(pMem+0))>>3;
       s|=((*(pMem+1))&0xfc)<<3;
       s|=((*(pMem+2))&0xf8)<<8;
       pMem+=3;
       *((unsigned short *)(ps+y*256+x*2))=s;
      }
    }
  }
 else
 if(iDesktopCol==15)
  {
   unsigned short s;
   for(y=0;y<96;y++)
    {
     for(x=0;x<128;x++)
      {
       s=(*(pMem+0))>>3;
       s|=((*(pMem+1))&0xfc)<<2;
       s|=((*(pMem+2))&0xf8)<<7;
       pMem+=3;
       *((unsigned short *)(ps+y*256+x*2))=s;
      }
    }
  }
 else
 if(iDesktopCol==32)
  {
   uint32_t l;
   for(y=0;y<96;y++)
    {
     for(x=0;x<128;x++)
      {
       l=  *(pMem+0);
       l|=(*(pMem+1))<<8;
       l|=(*(pMem+2))<<16;
       pMem+=3;
       *((uint32_t *)(ps+y*512+x*4))=l;
      }
    }
  }

 XPimage = XCreateImage(display,myvisual->visual,
                        depth, ZPixmap, 0,
                        (char *)p, 
                        128, 96,
                        depth>16 ? 32 : 16,
                        0);
}

void DestroyPic(void)
{
 if(XPimage) 
  { /*
   XPutImage(display,window,hGC, XCimage,
	  0, 0, 0, 0, iResX, iResY);*/
   XDestroyImage(XPimage);
   XPimage=0; 
  } 
}

void DisplayPic(void)
{
 XPutImage(display,window,hGC, XPimage,
           0, 0, iResX-128, 0,128,96);
}

void ShowGpuPic(void)
{
}

void ShowTextGpuPic(void)
{
}

#if 0
static void hq2x_32_def(uint32_t * dst0, uint32_t * dst1, const uint32_t * src0, const uint32_t * src1, const uint32_t * src2, unsigned count)
{
	static unsigned char cache_vert_mask[640];
	unsigned char cache_horiz_mask = 0;
	
	unsigned i;
	unsigned char mask;
	uint32_t  c[9];

	if (src0 == src1)	//processing first row
		memset(cache_vert_mask, 0, count);	
	
	for(i=0;i<count;++i) {
		c[1] = src0[0];
		c[4] = src1[0];
		c[7] = src2[0];

		if (i>0) {
			c[0] = src0[-1];
			c[3] = src1[-1];
			c[6] = src2[-1];
		} else {
			c[0] = c[1];
			c[3] = c[4];
			c[6] = c[7];
		}

		if (i<count-1) {
			c[2] = src0[1];
			c[5] = src1[1];
			c[8] = src2[1];
		} else {
			c[2] = c[1];
			c[5] = c[4];
			c[8] = c[7];
		}

		mask = 0;

		mask |= interp_32_diff(c[0], c[4]) << 0;
		mask |= cache_vert_mask[i];
		mask |= interp_32_diff(c[2], c[4]) << 2;
		mask |= cache_horiz_mask;
		cache_horiz_mask = interp_32_diff(c[5], c[4]) << 3;
		mask |= cache_horiz_mask << 1;	// << 3 << 1 == << 4
		mask |= interp_32_diff(c[6], c[4]) << 5;
		cache_vert_mask[i] = interp_32_diff(c[7], c[4]) << 1;
		mask |= cache_vert_mask[i] << 5; // << 1 << 5 == << 6
		mask |= interp_32_diff(c[8], c[4]) << 7;


		switch (mask) {
#include "hq2x.h"
		}


		src0 += 1;
		src1 += 1;
		src2 += 1;
		dst0 += 2;
		dst1 += 2;
	}
}

void hq2x_32( unsigned char * srcPtr,  DWORD srcPitch, unsigned char * dstPtr, int width, int height)
{
	const int dstPitch = srcPitch<<1;

	int count = height;

	finalw=width*2;
	finalh=height*2;

	uint32_t  *dst0 = (uint32_t  *)dstPtr;
	uint32_t  *dst1 = dst0 + (dstPitch >> 2);

	uint32_t  *src0 = (uint32_t  *)srcPtr;
	uint32_t  *src1 = src0 + (srcPitch >> 2);
	uint32_t  *src2 = src1 + (srcPitch >> 2);
	hq2x_32_def(dst0, dst1, src0, src0, src1, width);


	count -= 2;
	while(count) {
		dst0 += dstPitch >> 1;		//next 2 lines (dstPitch / 4 char per int * 2)
		dst1 += dstPitch >> 1;
		hq2x_32_def(dst0, dst1, src0, src1, src2, width);
		src0 = src1;
		src1 = src2;
		src2 += srcPitch >> 2;
		--count;
	}
	dst0 += dstPitch >> 1;
	dst1 += dstPitch >> 1;
	hq2x_32_def(dst0, dst1, src0, src1, src1, width);
}

static void hq3x_32_def(uint32_t*  dst0, uint32_t*  dst1, uint32_t*  dst2, const uint32_t* src0, const uint32_t* src1, const uint32_t* src2, unsigned count)
{
	static unsigned char cache_vert_mask[640];
	unsigned char cache_horiz_mask = 0;

	unsigned i;
	unsigned char mask;
	uint32_t  c[9];

	if (src0 == src1)	//processing first row
		memset(cache_vert_mask, 0, count);	

	for(i=0;i<count;++i) {
		c[1] = src0[0];
		c[4] = src1[0];
		c[7] = src2[0];

		if (i>0) {
			c[0] = src0[-1];
			c[3] = src1[-1];
			c[6] = src2[-1];
		} else {
			c[0] = c[1];
			c[3] = c[4];
			c[6] = c[7];
		}

		if (i<count-1) {
			c[2] = src0[1];
			c[5] = src1[1];
			c[8] = src2[1];
		} else {
			c[2] = c[1];
			c[5] = c[4];
			c[8] = c[7];
		}

		mask = 0;
		
		mask |= interp_32_diff(c[0], c[4]) << 0;
		mask |= cache_vert_mask[i];
		mask |= interp_32_diff(c[2], c[4]) << 2;
		mask |= cache_horiz_mask;
		cache_horiz_mask = interp_32_diff(c[5], c[4]) << 3;
		mask |= cache_horiz_mask << 1;	// << 3 << 1 == << 4
		mask |= interp_32_diff(c[6], c[4]) << 5;
		cache_vert_mask[i] = interp_32_diff(c[7], c[4]) << 1;
		mask |= cache_vert_mask[i] << 5; // << 1 << 5 == << 6
		mask |= interp_32_diff(c[8], c[4]) << 7;

		switch (mask) {
#include "hq3x.h"
		}

		src0 += 1;
		src1 += 1;
		src2 += 1;
		dst0 += 3;
		dst1 += 3;
		dst2 += 3;
	}
}

void hq3x_32( unsigned char * srcPtr,  DWORD srcPitch, unsigned char * dstPtr, int width, int height)
{
	int count = height;

	int dstPitch = srcPitch*3;
	int dstRowPixels = dstPitch>>2;

	finalw=width*3;
	finalh=height*3;

	uint32_t  *dst0 = (uint32_t  *)dstPtr;
	uint32_t  *dst1 = dst0 + dstRowPixels;
	uint32_t  *dst2 = dst1 + dstRowPixels;

	uint32_t  *src0 = (uint32_t  *)srcPtr;
	uint32_t  *src1 = src0 + (srcPitch >> 2);
	uint32_t  *src2 = src1 + (srcPitch >> 2);
	hq3x_32_def(dst0, dst1, dst2, src0, src0, src2, width);

	count -= 2;
	while(count) {
		dst0 += dstRowPixels * 3;
		dst1 += dstRowPixels * 3;
		dst2 += dstRowPixels * 3;

		hq3x_32_def(dst0, dst1, dst2, src0, src1, src2, width);
		src0 = src1;
		src1 = src2;
		src2 += srcPitch >> 2;
		--count;
	}
	dst0 += dstRowPixels * 3;
	dst1 += dstRowPixels * 3;
	dst2 += dstRowPixels * 3;

	hq3x_32_def(dst0, dst1, dst2, src0, src1, src1, width);

}
#endif
