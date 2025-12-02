/***************************************************************************
                          prim.c  -  description
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

#define _IN_PRIMDRAW

#include "gpuStdafx.h"
#include "gpuExternals.h"
#include "gpuPlugin.h"
#include "gpuDraw.h"
#include "gpuTexture.h"
#include "gpuPrim.h"

////////////////////////////////////////////////////////////////////////                                          
// defines
////////////////////////////////////////////////////////////////////////

#define DEFOPAQUEON  glAlphaFunc(GL_EQUAL,0.0f);bBlendEnable=FALSE;glDisable(GL_BLEND);                                
#define DEFOPAQUEOFF glAlphaFunc(GL_GREATER,0.49f);
#define fpoint(x) x
////////////////////////////////////////////////////////////////////////                                          
// globals
////////////////////////////////////////////////////////////////////////


BOOL           bDrawTextured;                          // current active drawing states
BOOL           bDrawSmoothShaded;
BOOL           bOldSmoothShaded;
BOOL           bDrawNonShaded;
BOOL           bDrawMultiPass;
int            iOffscreenDrawing;
int            iDrawnSomething=0;

BOOL           bRenderFrontBuffer=FALSE;               // flag for front buffer rendering

GLubyte        ubGloAlpha;                             // texture alpha
GLubyte        ubGloColAlpha;                          // color alpha
int            iFilterType;                            // type of filter
BOOL           bFullVRam=FALSE;                        // sign for tex win
BOOL           bDrawDither;                            // sign for dither
BOOL           bUseMultiPass;                          // sign for multi pass
GLuint         gTexName;                               // binded texture
BOOL           bTexEnabled;                            // texture enable flag
BOOL           bBlendEnable;                           // blend enable flag
PSXRect_t      xrUploadArea;                           // rect to upload
PSXRect_t      xrUploadAreaIL;                         // rect to upload
PSXRect_t      xrUploadAreaRGB24;                      // rect to upload rgb24
int            iSpriteTex=0;                           // flag for "hey, it's a sprite"
unsigned short usMirror;                               // mirror, mirror on the wall

BOOL           bNeedUploadAfter=FALSE;                 // sign for uploading in next frame
BOOL           bNeedUploadTest=FALSE;                  // sign for upload test
BOOL           bUsingTWin=FALSE;                       // tex win active flag
BOOL           bUsingMovie=FALSE;                      // movie active flag
PSXRect_t      xrMovieArea;                            // rect for movie upload
short          sSprite_ux2;                            // needed for sprire adjust
short          sSprite_vy2;                            // 
unsigned int   ulOLDCOL=0;                             // active color
unsigned int   ulClutID;                               // clut

unsigned int  dwCfgFixes;                              // game fixes
unsigned int  dwActFixes=0;
unsigned int  dwEmuFixes=0;
BOOL          bUseFixes;

int           drawX,drawY,drawW,drawH;                 // offscreen drawing checkers
short         sxmin,sxmax,symin,symax;
unsigned int CSVERTEX=0,CSCOLOR=0,CSTEXTURE=0;

void offsetPSX4(void)
{
 lx0 += PSXDisplay.DrawOffset.x;
 ly0 += PSXDisplay.DrawOffset.y;
 lx1 += PSXDisplay.DrawOffset.x;
 ly1 += PSXDisplay.DrawOffset.y;
 lx2 += PSXDisplay.DrawOffset.x;
 ly2 += PSXDisplay.DrawOffset.y;
 lx3 += PSXDisplay.DrawOffset.x;
 ly3 += PSXDisplay.DrawOffset.y;
}

////////////////////////////////////////////////////////////////////////                                          
// Update global TP infos
////////////////////////////////////////////////////////////////////////

void UpdateGlobalTP(unsigned short gdata)
{
 GlobalTextAddrX = (gdata << 6) & 0x3c0;

 if(iGPUHeight==1024)                                  // ZN mode
  {
   if(dwGPUVersion==2)                                 // very special zn gpu
    {
     GlobalTextAddrY =((gdata & 0x60 ) << 3);
     GlobalTextIL    =(gdata & 0x2000) >> 13;
     GlobalTextABR = (unsigned short)((gdata >> 7) & 0x3);
     GlobalTextTP = (gdata >> 9) & 0x3;
     if(GlobalTextTP==3) GlobalTextTP=2;             
     GlobalTexturePage = (GlobalTextAddrX>>6)+(GlobalTextAddrY>>4);
     usMirror =0;
     STATUSREG = (STATUSREG & 0xffffe000 ) | (gdata & 0x1fff );
     return;
    }
   else                                                // "enhanced" psx gpu
    {
     GlobalTextAddrY = (unsigned short)(((gdata << 4) & 0x100) | ((gdata >> 2) & 0x200));
    }
  }
 else GlobalTextAddrY = (gdata << 4) & 0x100;          // "normal" psx gpu

 usMirror=gdata&0x3000;
 
 GlobalTextTP = (gdata >> 7) & 0x3;                    // tex mode (4,8,15)
 if(GlobalTextTP==3) GlobalTextTP=2;                   // seen in Wild9 :(
 GlobalTextABR = (gdata >> 5) & 0x3;                   // blend mode

 GlobalTexturePage = (GlobalTextAddrX>>6)+(GlobalTextAddrY>>4);

 STATUSREG&=~0x07ff;                                   // Clear the necessary bits
 STATUSREG|=(gdata & 0x07ff);                          // set the necessary bits
}

////////////////////////////////////////////////////////////////////////                                          
// Some ASM color convertion... Lewpy's special...
////////////////////////////////////////////////////////////////////////


unsigned int DoubleBGR2RGB (unsigned int BGR)
{
 unsigned int ebx,eax,edx;

 ebx=(BGR&0x000000ff)<<1;
 if(ebx&0x00000100) ebx=0x000000ff;

 eax=(BGR&0x0000ff00)<<1;
 if(eax&0x00010000) eax=0x0000ff00;

 edx=(BGR&0x00ff0000)<<1;
 if(edx&0x01000000) edx=0x00ff0000;

 return (ebx|eax|edx);
}

unsigned short BGR24to16 (unsigned int BGR)
{
 return ((BGR>>3)&0x1f)|((BGR&0xf80000)>>9)|((BGR&0xf800)>>6);
}


////////////////////////////////////////////////////////////////////////
// OpenGL primitive drawing commands
////////////////////////////////////////////////////////////////////////

void PRIMdrawTexturedQuad(OGLVertex* vertex1, OGLVertex* vertex2,
                                   OGLVertex* vertex3, OGLVertex* vertex4) 
{


Vertex v[4];

v[0].xyz.x = fpoint(vertex1->x);
v[0].xyz.y = fpoint(vertex1->y);
v[0].xyz.z = fpoint(vertex1->z);
v[0].st.x = fpoint(vertex1->sow);
v[0].st.y = fpoint(vertex1->tow);

v[1].xyz.x = fpoint(vertex2->x);
v[1].xyz.y = fpoint(vertex2->y);
v[1].xyz.z = fpoint(vertex2->z);
v[1].st.x = fpoint(vertex2->sow);
v[1].st.y = fpoint(vertex2->tow);

v[2].xyz.x = fpoint(vertex4->x);
v[2].xyz.y = fpoint(vertex4->y);
v[2].xyz.z = fpoint(vertex4->z);
v[2].st.x = fpoint(vertex4->sow);
v[2].st.y = fpoint(vertex4->tow);

v[3].xyz.x = fpoint(vertex3->x);
v[3].xyz.y = fpoint(vertex3->y);
v[3].xyz.z = fpoint(vertex3->z);
v[3].st.x = fpoint(vertex3->sow);
v[3].st.y = fpoint(vertex3->tow);
if (CSCOLOR==1) glDisableClientState(GL_COLOR_ARRAY);glError();
if (CSTEXTURE==0) glEnableClientState(GL_TEXTURE_COORD_ARRAY);glError();
if (CSVERTEX==0) glEnableClientState(GL_VERTEX_ARRAY);glError();
glTexCoordPointer(2, GL_FLOAT, sizeof(v[0]), &v[0].st);glError();
glVertexPointer(3, GL_FLOAT, sizeof(v[0]), &v[0].xyz);glError();
glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);glError();
CSTEXTURE=CSVERTEX=1;
CSCOLOR=0;
}

///////////////////////////////////////////////////////// 

void PRIMdrawTexturedTri(OGLVertex* vertex1, OGLVertex* vertex2,
                                  OGLVertex* vertex3) 
{
Vertex v[3];
if (vertex1->x==0&&vertex1->y==0&&vertex2->x==0&&vertex2->y==0&&vertex3->x==0&&vertex3->y==0) return;

v[0].xyz.x = fpoint(vertex1->x);
v[0].xyz.y = fpoint(vertex1->y);
v[0].xyz.z = fpoint(vertex1->z);
v[0].st.x = fpoint(vertex1->sow);
v[0].st.y = fpoint(vertex1->tow);

v[1].xyz.x = fpoint(vertex2->x);
v[1].xyz.y = fpoint(vertex2->y);
v[1].xyz.z = fpoint(vertex2->z);
v[1].st.x = fpoint(vertex2->sow);
v[1].st.y = fpoint(vertex2->tow);

v[2].xyz.x = fpoint(vertex3->x);
v[2].xyz.y = fpoint(vertex3->y);
v[2].xyz.z = fpoint(vertex3->z);
v[2].st.x = fpoint(vertex3->sow);
v[2].st.y = fpoint(vertex3->tow);
if (CSCOLOR==1) glDisableClientState(GL_COLOR_ARRAY);glError();
if (CSTEXTURE==0) glEnableClientState(GL_TEXTURE_COORD_ARRAY);glError();
if (CSVERTEX==0) glEnableClientState(GL_VERTEX_ARRAY);glError();
glTexCoordPointer(2, GL_FLOAT, sizeof(v[0]), &v[0].st);glError();
glVertexPointer(3, GL_FLOAT, sizeof(v[0]), &v[0].xyz);glError();
glDrawArrays(GL_TRIANGLES, 0, 3);glError();
CSTEXTURE=CSVERTEX=1;
CSCOLOR=0;

}

///////////////////////////////////////////////////////// 

void PRIMdrawTexGouraudTriColor(OGLVertex* vertex1, OGLVertex* vertex2,
                                         OGLVertex* vertex3) 
{

Vertex2 v[3];
if (vertex1->x==0&&vertex1->y==0&&vertex2->x==0&&vertex2->y==0&&vertex3->x==0&&vertex3->y==0) return;

v[0].xyz.x = fpoint(vertex1->x);
v[0].xyz.y = fpoint(vertex1->y);
v[0].xyz.z = fpoint(vertex1->z);
v[0].st.x = fpoint(vertex1->sow);
v[0].st.y = fpoint(vertex1->tow);
v[0].rgba.r = vertex1->c.col[0];
v[0].rgba.g = vertex1->c.col[1];
v[0].rgba.b = vertex1->c.col[2];
v[0].rgba.a = vertex1->c.col[3];

v[1].xyz.x = fpoint(vertex2->x);
v[1].xyz.y = fpoint(vertex2->y);
v[1].xyz.z = fpoint(vertex2->z);
v[1].st.x = fpoint(vertex2->sow);
v[1].st.y = fpoint(vertex2->tow);
v[1].rgba.r = vertex2->c.col[0];
v[1].rgba.g = vertex2->c.col[1];
v[1].rgba.b = vertex2->c.col[2];
v[1].rgba.a = vertex2->c.col[3];

v[2].xyz.x = fpoint(vertex3->x);
v[2].xyz.y = fpoint(vertex3->y);
v[2].xyz.z = fpoint(vertex3->z);
v[2].st.x = fpoint(vertex3->sow);
v[2].st.y = fpoint(vertex3->tow);
v[2].rgba.r = vertex3->c.col[0];
v[2].rgba.g = vertex3->c.col[1];
v[2].rgba.b = vertex3->c.col[2];
v[2].rgba.a = vertex3->c.col[3];

if (CSTEXTURE==0) glEnableClientState(GL_TEXTURE_COORD_ARRAY);glError();
if (CSVERTEX==0) glEnableClientState(GL_VERTEX_ARRAY);glError();
if (CSCOLOR==0) glEnableClientState(GL_COLOR_ARRAY);glError();

glTexCoordPointer(2, GL_FLOAT, sizeof(v[0]), &v[0].st);glError();
glVertexPointer(3, GL_FLOAT, sizeof(v[0]), &v[0].xyz);glError();
glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(v[0]), &v[0].rgba);glError();

glDrawArrays(GL_TRIANGLES, 0, 3);glError();
CSTEXTURE=CSVERTEX=CSCOLOR=1;
}

///////////////////////////////////////////////////////// 

void PRIMdrawTexGouraudTriColorQuad(OGLVertex* vertex1, OGLVertex* vertex2,
                                             OGLVertex* vertex3, OGLVertex* vertex4) 
{
Vertex2 v[4];
if (vertex1->x==0&&vertex1->y==0&&vertex2->x==0&&vertex2->y==0&&vertex3->x==0&&vertex3->y==0&&vertex4->x==0&&vertex4->y==0) return;

v[0].xyz.x = fpoint(vertex1->x);
v[0].xyz.y = fpoint(vertex1->y);
v[0].xyz.z = fpoint(vertex1->z);
v[0].st.x = fpoint(vertex1->sow);
v[0].st.y = fpoint(vertex1->tow);
v[0].rgba.r = vertex1->c.col[0];
v[0].rgba.g = vertex1->c.col[1];
v[0].rgba.b = vertex1->c.col[2];
v[0].rgba.a = vertex1->c.col[3];

v[1].xyz.x = fpoint(vertex2->x);
v[1].xyz.y = fpoint(vertex2->y);
v[1].xyz.z = fpoint(vertex2->z);
v[1].st.x = fpoint(vertex2->sow);
v[1].st.y = fpoint(vertex2->tow);
v[1].rgba.r = vertex2->c.col[0];
v[1].rgba.g = vertex2->c.col[1];
v[1].rgba.b = vertex2->c.col[2];
v[1].rgba.a = vertex2->c.col[3];

v[2].xyz.x = fpoint(vertex4->x);
v[2].xyz.y = fpoint(vertex4->y);
v[2].xyz.z = fpoint(vertex4->z);
v[2].st.x = fpoint(vertex4->sow);
v[2].st.y = fpoint(vertex4->tow);
v[2].rgba.r = vertex4->c.col[0];
v[2].rgba.g = vertex4->c.col[1];
v[2].rgba.b = vertex4->c.col[2];
v[2].rgba.a = vertex4->c.col[3];

v[3].xyz.x = fpoint(vertex3->x);
v[3].xyz.y = fpoint(vertex3->y);
v[3].xyz.z = fpoint(vertex3->z);
v[3].st.x = fpoint(vertex3->sow);
v[3].st.y = fpoint(vertex3->tow);
v[3].rgba.r = vertex3->c.col[0];
v[3].rgba.g = vertex3->c.col[1];
v[3].rgba.b = vertex3->c.col[2];
v[3].rgba.a = vertex3->c.col[3];

if (CSTEXTURE==0) glEnableClientState(GL_TEXTURE_COORD_ARRAY);glError();
if (CSVERTEX==0) glEnableClientState(GL_VERTEX_ARRAY);glError();
if (CSCOLOR==0) glEnableClientState(GL_COLOR_ARRAY);glError();

glTexCoordPointer(2, GL_FLOAT, sizeof(v[0]), &v[0].st);glError();
glVertexPointer(3, GL_FLOAT, sizeof(v[0]), &v[0].xyz);glError();
glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(v[0]), &v[0].rgba);glError();

glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);glError();
CSTEXTURE=CSVERTEX=CSCOLOR=1;
}

///////////////////////////////////////////////////////// 

void PRIMdrawTri(OGLVertex* vertex1, OGLVertex* vertex2, OGLVertex* vertex3)
{
Vec3f v[3];
if (vertex1->x==0&&vertex1->y==0&&vertex2->x==0&&vertex2->y==0&&vertex3->x==0&&vertex3->y==0) return;

v[0].x = fpoint(vertex1->x);
v[0].y = fpoint(vertex1->y);
v[0].z = fpoint(vertex1->z);

v[1].x = fpoint(vertex2->x);
v[1].y = fpoint(vertex2->y);
v[1].z = fpoint(vertex2->z);

v[2].x = fpoint(vertex3->x);
v[2].y = fpoint(vertex3->y);
v[2].z = fpoint(vertex3->z);

if (CSVERTEX==0) glEnableClientState(GL_VERTEX_ARRAY);glError();
if (CSTEXTURE==1) glDisableClientState(GL_TEXTURE_COORD_ARRAY);glError();
if (CSCOLOR==1) glDisableClientState(GL_COLOR_ARRAY);glError();

glVertexPointer(3, GL_FLOAT, sizeof(v[0]), &v[0]);glError();
glDrawArrays(GL_TRIANGLES, 0, 3);glError();
CSVERTEX=1;
CSTEXTURE=CSCOLOR=0;

}

///////////////////////////////////////////////////////// 

void PRIMdrawTri2(OGLVertex* vertex1, OGLVertex* vertex2,
                           OGLVertex* vertex3, OGLVertex* vertex4) 
{
Vec3f v[4];
if (vertex1->x==0&&vertex1->y==0&&vertex2->x==0&&vertex2->y==0&&vertex3->x==0&&vertex3->y==0&&vertex4->x==0&&vertex4->y==0) return;

v[0].x = fpoint(vertex1->x);
v[0].y = fpoint(vertex1->y);
v[0].z = fpoint(vertex1->z);

v[1].x = fpoint(vertex3->x);
v[1].y = fpoint(vertex3->y);
v[1].z = fpoint(vertex3->z);

v[2].x = fpoint(vertex2->x);
v[2].y = fpoint(vertex2->y);
v[2].z = fpoint(vertex2->z);

v[3].x = fpoint(vertex4->x);
v[3].y = fpoint(vertex4->y);
v[3].z = fpoint(vertex4->z);

if (CSVERTEX==0) glEnableClientState(GL_VERTEX_ARRAY);glError();
if (CSTEXTURE==1) glDisableClientState(GL_TEXTURE_COORD_ARRAY);glError();
if (CSCOLOR==1) glDisableClientState(GL_COLOR_ARRAY);glError();

glVertexPointer(3, GL_FLOAT, sizeof(v[0]), &v[0]);glError();
glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);glError();
CSVERTEX=1;
CSTEXTURE=CSCOLOR=0;
}

///////////////////////////////////////////////////////// 

void PRIMdrawGouraudTriColor(OGLVertex* vertex1, OGLVertex* vertex2,
                                      OGLVertex* vertex3) 
{
Vertex2 v[3];
if (vertex1->x==0&&vertex1->y==0&&vertex2->x==0&&vertex2->y==0&&vertex3->x==0&&vertex3->y==0) return;

v[0].xyz.x = fpoint(vertex1->x);
v[0].xyz.y = fpoint(vertex1->y);
v[0].xyz.z = fpoint(vertex1->z);
v[0].rgba.r = vertex1->c.col[0];
v[0].rgba.g = vertex1->c.col[1];
v[0].rgba.b = vertex1->c.col[2];
v[0].rgba.a = vertex1->c.col[3];

v[1].xyz.x = fpoint(vertex2->x);
v[1].xyz.y = fpoint(vertex2->y);
v[1].xyz.z = fpoint(vertex2->z);
v[1].rgba.r = vertex2->c.col[0];
v[1].rgba.g = vertex2->c.col[1];
v[1].rgba.b = vertex2->c.col[2];
v[1].rgba.a = vertex2->c.col[3];

v[2].xyz.x = fpoint(vertex3->x);
v[2].xyz.y = fpoint(vertex3->y);
v[2].xyz.z = fpoint(vertex3->z);
v[2].rgba.r = vertex3->c.col[0];
v[2].rgba.g = vertex3->c.col[1];
v[2].rgba.b = vertex3->c.col[2];
v[2].rgba.a = vertex3->c.col[3];

if (CSVERTEX==0) glEnableClientState(GL_VERTEX_ARRAY);glError();
if (CSCOLOR==0) glEnableClientState(GL_COLOR_ARRAY);glError();
if (CSTEXTURE==1) glDisableClientState(GL_TEXTURE_COORD_ARRAY);glError();

glVertexPointer(3, GL_FLOAT, sizeof(v[0]), &v[0].xyz);glError();
glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(v[0]), &v[0].rgba);glError();

glDrawArrays(GL_TRIANGLES, 0, 3);glError();
CSVERTEX=CSCOLOR=1;
CSTEXTURE=0;
}

///////////////////////////////////////////////////////// 

void PRIMdrawGouraudTri2Color(OGLVertex* vertex1, OGLVertex* vertex2,
                                       OGLVertex* vertex3, OGLVertex* vertex4) 
{
Vertex2 v[4];
if (vertex1->x==0&&vertex1->y==0&&vertex2->x==0&&vertex2->y==0&&vertex3->x==0&&vertex3->y==0&&vertex4->x==0&&vertex4->y==0) return;

v[0].xyz.x = fpoint(vertex1->x);
v[0].xyz.y = fpoint(vertex1->y);
v[0].xyz.z = fpoint(vertex1->z);
v[0].rgba.r = vertex1->c.col[0];
v[0].rgba.g = vertex1->c.col[1];
v[0].rgba.b = vertex1->c.col[2];
v[0].rgba.a = vertex1->c.col[3];

v[1].xyz.x = fpoint(vertex2->x);
v[1].xyz.y = fpoint(vertex2->y);
v[1].xyz.z = fpoint(vertex2->z);
v[1].rgba.r = vertex2->c.col[0];
v[1].rgba.g = vertex2->c.col[1];
v[1].rgba.b = vertex2->c.col[2];
v[1].rgba.a = vertex2->c.col[3];

v[2].xyz.x = fpoint(vertex3->x);
v[2].xyz.y = fpoint(vertex3->y);
v[2].xyz.z = fpoint(vertex3->z);
v[2].rgba.r = vertex3->c.col[0];
v[2].rgba.g = vertex3->c.col[1];
v[2].rgba.b = vertex3->c.col[2];
v[2].rgba.a = vertex3->c.col[3];

v[3].xyz.x = fpoint(vertex4->x);
v[3].xyz.y = fpoint(vertex4->y);
v[3].xyz.z = fpoint(vertex4->z);
v[3].rgba.r = vertex4->c.col[0];
v[3].rgba.g = vertex4->c.col[1];
v[3].rgba.b = vertex4->c.col[2];
v[3].rgba.a = vertex4->c.col[3];

if (CSTEXTURE==1) glDisableClientState(GL_TEXTURE_COORD_ARRAY);glError();
if (CSVERTEX==0) glEnableClientState(GL_VERTEX_ARRAY);glError();
if (CSCOLOR==0) glEnableClientState(GL_COLOR_ARRAY);glError();

glVertexPointer(3, GL_FLOAT, sizeof(v[0]), &v[0].xyz);glError();
glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(v[0]), &v[0].rgba);glError();

glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);glError();
CSTEXTURE=0;
CSVERTEX=CSCOLOR=1;
}

///////////////////////////////////////////////////////// 

void PRIMdrawFlatLine(OGLVertex* vertex1, OGLVertex* vertex2,OGLVertex* vertex3, OGLVertex* vertex4)
{
Vertex2 v[4];
if (vertex1->x==0&&vertex1->y==0&&vertex2->x==0&&vertex2->y==0&&vertex3->x==0&&vertex3->y==0&&vertex4->x==0&&vertex4->y==0) return;

v[0].xyz.x = fpoint(vertex1->x);
v[0].xyz.y = fpoint(vertex1->y);
v[0].xyz.z = fpoint(vertex1->z);
v[0].rgba.r = vertex1->c.col[0];
v[0].rgba.g = vertex1->c.col[1];
v[0].rgba.b = vertex1->c.col[2];
v[0].rgba.a = vertex1->c.col[3];

v[1].xyz.x = fpoint(vertex2->x);
v[1].xyz.y = fpoint(vertex2->y);
v[1].xyz.z = fpoint(vertex2->z);
v[1].rgba.r = vertex1->c.col[0];
v[1].rgba.g = vertex1->c.col[1];
v[1].rgba.b = vertex1->c.col[2];
v[1].rgba.a = vertex1->c.col[3];

v[2].xyz.x = fpoint(vertex4->x);
v[2].xyz.y = fpoint(vertex4->y);
v[2].xyz.z = fpoint(vertex4->z);
v[2].rgba.r = vertex1->c.col[0];
v[2].rgba.g = vertex1->c.col[1];
v[2].rgba.b = vertex1->c.col[2];
v[2].rgba.a = vertex1->c.col[3];

v[3].xyz.x = fpoint(vertex3->x);
v[3].xyz.y = fpoint(vertex3->y);
v[3].xyz.z = fpoint(vertex3->z);
v[3].rgba.r = vertex1->c.col[0];
v[3].rgba.g = vertex1->c.col[1];
v[3].rgba.b = vertex1->c.col[2];
v[3].rgba.a = vertex1->c.col[3];

if (CSTEXTURE==1) glDisableClientState(GL_TEXTURE_COORD_ARRAY);glError();
if (CSVERTEX==0) glEnableClientState(GL_VERTEX_ARRAY);glError();
if (CSCOLOR==0) glEnableClientState(GL_COLOR_ARRAY);glError();

glVertexPointer(3, GL_FLOAT, sizeof(v[0]), &v[0].xyz);glError();
glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(v[0]), &v[0].rgba);glError();

glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);glError();

CSTEXTURE=0;
CSVERTEX=CSCOLOR=1;


}

///////////////////////////////////////////////////////// 
     
void PRIMdrawGouraudLine(OGLVertex* vertex1, OGLVertex* vertex2,OGLVertex* vertex3, OGLVertex* vertex4)
{
	Vertex2 v[4];
if (vertex1->x==0&&vertex1->y==0&&vertex2->x==0&&vertex2->y==0&&vertex3->x==0&&vertex3->y==0&&vertex4->x==0&&vertex4->y==0) return;

v[0].xyz.x = fpoint(vertex1->x);
v[0].xyz.y = fpoint(vertex1->y);
v[0].xyz.z = fpoint(vertex1->z);
v[0].rgba.r = vertex1->c.col[0];
v[0].rgba.g = vertex1->c.col[1];
v[0].rgba.b = vertex1->c.col[2];
v[0].rgba.a = vertex1->c.col[3];

v[1].xyz.x = fpoint(vertex2->x);
v[1].xyz.y = fpoint(vertex2->y);
v[1].xyz.z = fpoint(vertex2->z);
v[1].rgba.r = vertex2->c.col[0];
v[1].rgba.g = vertex2->c.col[1];
v[1].rgba.b = vertex2->c.col[2];
v[1].rgba.a = vertex2->c.col[3];

v[3].xyz.x = fpoint(vertex3->x);
v[3].xyz.y = fpoint(vertex3->y);
v[3].xyz.z = fpoint(vertex3->z);
v[3].rgba.r = vertex3->c.col[0];
v[3].rgba.g = vertex3->c.col[1];
v[3].rgba.b = vertex3->c.col[2];
v[3].rgba.a = vertex3->c.col[3];

v[2].xyz.x = fpoint(vertex4->x);
v[2].xyz.y = fpoint(vertex4->y);
v[2].xyz.z = fpoint(vertex4->z);
v[2].rgba.r = vertex4->c.col[0];
v[2].rgba.g = vertex4->c.col[1];
v[2].rgba.b = vertex4->c.col[2];
v[2].rgba.a = vertex4->c.col[3];

if (CSTEXTURE==1) glDisableClientState(GL_TEXTURE_COORD_ARRAY);glError();
if (CSVERTEX==0) glEnableClientState(GL_VERTEX_ARRAY);glError();
if (CSCOLOR==0) glEnableClientState(GL_COLOR_ARRAY);glError();

glVertexPointer(3, GL_FLOAT, sizeof(v[0]), &v[0].xyz);glError();
glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(v[0]), &v[0].rgba);glError();

glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);glError();
CSTEXTURE=0;
CSVERTEX=CSCOLOR=1;
}

///////////////////////////////////////////////////////// 
             
void PRIMdrawQuad(OGLVertex* vertex1, OGLVertex* vertex2,
                           OGLVertex* vertex3, OGLVertex* vertex4) 
{
Vec3f v[4];
if (vertex1->x==0&&vertex1->y==0&&vertex2->x==0&&vertex2->y==0&&vertex3->x==0&&vertex3->y==0&&vertex4->x==0&&vertex4->y==0) return;

v[0].x = fpoint(vertex1->x);
v[0].y = fpoint(vertex1->y);
v[0].z = fpoint(vertex1->z);

v[1].x = fpoint(vertex2->x);
v[1].y = fpoint(vertex2->y);
v[1].z = fpoint(vertex2->z);

v[2].x = fpoint(vertex4->x);
v[2].y = fpoint(vertex4->y);
v[2].z = fpoint(vertex4->z);

v[3].x = fpoint(vertex3->x);
v[3].y = fpoint(vertex3->y);
v[3].z = fpoint(vertex3->z);

if (CSTEXTURE==1) glDisableClientState(GL_TEXTURE_COORD_ARRAY);glError();
if (CSVERTEX==0) glEnableClientState(GL_VERTEX_ARRAY);glError();
if (CSCOLOR==1) glDisableClientState(GL_COLOR_ARRAY);glError();

glVertexPointer(3, GL_FLOAT, sizeof(v[0]), &v[0]);glError();
glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);glError();
CSTEXTURE=0;
CSVERTEX=1;
CSCOLOR=0;
}

////////////////////////////////////////////////////////////////////////                                          
// Transparent blending settings
////////////////////////////////////////////////////////////////////////

static GLenum obm1=GL_ZERO;
static GLenum obm2=GL_ZERO;

typedef struct SEMITRANSTAG
{
 GLenum  srcFac;
 GLenum  dstFac;
 GLubyte alpha;
} SemiTransParams;

SemiTransParams TransSets[4]=
{
 {GL_SRC_ALPHA,GL_SRC_ALPHA,          127},
 {GL_ONE,      GL_ONE,                255},
 {GL_ZERO,     GL_ONE_MINUS_SRC_COLOR,255},
 {GL_ONE_MINUS_SRC_ALPHA,GL_ONE,      192}
}; 

////////////////////////////////////////////////////////////////////////

void SetSemiTrans(void)
{
/*
* 0.5 x B + 0.5 x F
* 1.0 x B + 1.0 x F
* 1.0 x B - 1.0 x F
* 1.0 x B +0.25 x F
*/

 if(!DrawSemiTrans)                                    // no semi trans at all?
  {
   if(bBlendEnable)
    {glDisable(GL_BLEND);glError();bBlendEnable=FALSE;}// -> don't wanna blend
   ubGloAlpha=ubGloColAlpha=255;                       // -> full alpha
   return;                                             // -> and bye
  }

 ubGloAlpha=ubGloColAlpha=TransSets[GlobalTextABR].alpha;

 if(!bBlendEnable)
  {glEnable(GL_BLEND);glError();bBlendEnable=TRUE;}    // wanna blend

 if(TransSets[GlobalTextABR].srcFac!=obm1 || 
    TransSets[GlobalTextABR].dstFac!=obm2)
  {
   //if(glBlendEquationEXTEx==NULL)
    {
     obm1=TransSets[GlobalTextABR].srcFac;
     obm2=TransSets[GlobalTextABR].dstFac;
     glBlendFunc(obm1,obm2); glError();                // set blend func
    }
   /*else
   if(TransSets[GlobalTextABR].dstFac !=GL_ONE_MINUS_SRC_COLOR)
    {
     if(obm2==GL_ONE_MINUS_SRC_COLOR)
      glBlendEquationEXTEx(FUNC_ADD_EXT);
     obm1=TransSets[GlobalTextABR].srcFac;
     obm2=TransSets[GlobalTextABR].dstFac;
     glBlendFunc(obm1,obm2);                           // set blend func
    }
   else
    {
     glBlendEquationEXTEx(FUNC_REVERSESUBTRACT_EXT);
     obm1=TransSets[GlobalTextABR].srcFac;
     obm2=TransSets[GlobalTextABR].dstFac;
     glBlendFunc(GL_ONE,GL_ONE);                       // set blend func
    }*/
  }
}

void SetScanTrans(void)                                // blending for scan lines
{
/* if(glBlendEquationEXTEx!=NULL)
  {
   if(obm2==GL_ONE_MINUS_SRC_COLOR)
    glBlendEquationEXTEx(FUNC_ADD_EXT);
  }
*/
 obm1=TransSets[0].srcFac;
 obm2=TransSets[0].dstFac;
 glBlendFunc(obm1,obm2); glError();                    // set blend func
}

void SetScanTexTrans(void)                             // blending for scan mask texture
{
/* if(glBlendEquationEXTEx!=NULL)
  {
   if(obm2==GL_ONE_MINUS_SRC_COLOR)
    glBlendEquationEXTEx(FUNC_ADD_EXT);
  }
*/
 obm1=TransSets[2].srcFac;
 obm2=TransSets[2].dstFac;
 glBlendFunc(obm1,obm2); glError();                    // set blend func
}

////////////////////////////////////////////////////////////////////////                                          
// multi pass in old 'Advanced blending' mode... got it from Lewpy :)
////////////////////////////////////////////////////////////////////////                                          

SemiTransParams MultiTexTransSets[4][2]=
{
 {
 {GL_ONE      ,GL_SRC_ALPHA,          127},
 {GL_SRC_ALPHA,GL_ONE,                127}
 },
 {
 {GL_ONE,      GL_SRC_ALPHA,          255},
 {GL_SRC_ALPHA,GL_ONE,                255}
 },
 {
 {GL_ZERO,     GL_ONE_MINUS_SRC_COLOR,255},
 {GL_ZERO,     GL_ONE_MINUS_SRC_COLOR,255}
 },
 {
 {GL_SRC_ALPHA,GL_ONE,                127},
 {GL_ONE_MINUS_SRC_ALPHA,GL_ONE,      255}
 }
}; 

////////////////////////////////////////////////////////////////////////                                          

SemiTransParams MultiColTransSets[4]=
{
 {GL_ONE_MINUS_SRC_ALPHA,GL_SRC_ALPHA,127},
 {GL_ONE,      GL_ONE,                255},
 {GL_ZERO,     GL_ONE_MINUS_SRC_COLOR,255},
 {GL_SRC_ALPHA,GL_ONE,                127}
}; 

////////////////////////////////////////////////////////////////////////                                          

void SetSemiTransMulti(int Pass)
{
 static GLenum bm1=GL_ZERO;
 static GLenum bm2=GL_ONE;

 ubGloAlpha=255;
 ubGloColAlpha=255;
 
 // are we enabling SemiTransparent mode?
 if(DrawSemiTrans)
  {
   if(bDrawTextured)
    {
     bm1=MultiTexTransSets[GlobalTextABR][Pass].srcFac;
     bm2=MultiTexTransSets[GlobalTextABR][Pass].dstFac;
     ubGloAlpha=MultiTexTransSets[GlobalTextABR][Pass].alpha;
    }
   // no texture
   else
    {
     bm1=MultiColTransSets[GlobalTextABR].srcFac;
     bm2=MultiColTransSets[GlobalTextABR].dstFac;
     ubGloColAlpha=MultiColTransSets[GlobalTextABR].alpha;
    }
  }
 // no shading
 else
  {
   if(Pass==0)
    {
     // disable blending
     bm1=GL_ONE;bm2=GL_ZERO;
    }
   else
    {
     // disable blending, but add src col a second time
     bm1=GL_ONE;bm2=GL_ONE;
    }
  }

 if(!bBlendEnable)
  {glEnable(GL_BLEND);glError();bBlendEnable=TRUE;}    // wanna blend

 if(bm1!=obm1 || bm2!=obm2)
  {
   glBlendFunc(bm1,bm2); glError();                    // set blend func
   obm1=bm1;obm2=bm2;
  }
}

////////////////////////////////////////////////////////////////////////                                          
// Set several rendering stuff including blending 
////////////////////////////////////////////////////////////////////////

void SetZMask3O(void)
{
 if(iUseMask && DrawSemiTrans && !iSetMask)
  {
   vertex[0].z=vertex[1].z=vertex[2].z=gl_z;
   gl_z+=0.00004f;
  }
}

void SetZMask3(void)
{
 if(iUseMask)
  {
   if(iSetMask || DrawSemiTrans)
    {vertex[0].z=vertex[1].z=vertex[2].z=0.95f;}
   else
    {
     vertex[0].z=vertex[1].z=vertex[2].z=gl_z;
     gl_z+=0.00004f;
    }
  }
}

void SetZMask3NT(void)
{
 if(iUseMask)
  {
   if(iSetMask)
    {vertex[0].z=vertex[1].z=vertex[2].z=0.95f;}
   else
    {
     vertex[0].z=vertex[1].z=vertex[2].z=gl_z;
     gl_z+=0.00004f;
    }
  }
}

////////////////////////////////////////////////////////////////////////

 void SetZMask4O(void)
{
 if(iUseMask && DrawSemiTrans && !iSetMask)
  {
   vertex[0].z=vertex[1].z=vertex[2].z=vertex[3].z=gl_z;
   gl_z+=0.00004f;
  }
}

 void SetZMask4(void)
{
 if(iUseMask)
  {
   if(iSetMask || DrawSemiTrans)
    {vertex[0].z=vertex[1].z=vertex[2].z=vertex[3].z=0.95f;}
   else
    {
     vertex[0].z=vertex[1].z=vertex[2].z=vertex[3].z=gl_z;
     gl_z+=0.00004f;
    }
  }
}

 void SetZMask4NT(void)
{
 if(iUseMask)
  {
   if(iSetMask==1)
    {vertex[0].z=vertex[1].z=vertex[2].z=vertex[3].z=0.95f;}
   else
    {
     vertex[0].z=vertex[1].z=vertex[2].z=vertex[3].z=gl_z;
     gl_z+=0.00004f;
    }
  }
}

 void SetZMask4SP(void)
{
 if(iUseMask)
  {
   if(iSetMask==1)
    {vertex[0].z=vertex[1].z=vertex[2].z=vertex[3].z=0.95f;}
   else
    {
     if(bCheckMask)
      {
       vertex[0].z=vertex[1].z=vertex[2].z=vertex[3].z=gl_z;
       gl_z+=0.00004f;
      }
     else
      {vertex[0].z=vertex[1].z=vertex[2].z=vertex[3].z=0.95f;}
    }
  }
}

////////////////////////////////////////////////////////////////////////

 void SetRenderState(unsigned int DrawAttributes)
{
 bDrawNonShaded = (SHADETEXBIT(DrawAttributes)) ? TRUE : FALSE;
 DrawSemiTrans = (SEMITRANSBIT(DrawAttributes)) ? TRUE : FALSE;
}                         

////////////////////////////////////////////////////////////////////////                                          

 void SetRenderColor(unsigned int DrawAttributes)
{
 if(bDrawNonShaded) {g_m1=g_m2=g_m3=128;}
 else
  {
   g_m1=DrawAttributes&0xff;
   g_m2=(DrawAttributes>>8)&0xff;
   g_m3=(DrawAttributes>>16)&0xff;
  }
}

////////////////////////////////////////////////////////////////////////                                          
                               
void SetRenderMode(unsigned int DrawAttributes,BOOL bSCol)
{
 if((bUseMultiPass) && (bDrawTextured) && !(bDrawNonShaded))
      {bDrawMultiPass = TRUE; SetSemiTransMulti(0);}
 else {bDrawMultiPass = FALSE;SetSemiTrans();}

 if(bDrawTextured)                                     // texture ? build it/get it from cache
  {
   GLuint currTex;
   if(bUsingTWin)       currTex=LoadTextureWnd(GlobalTexturePage,GlobalTextTP, ulClutID);
   else if(bUsingMovie) currTex=LoadTextureMovie();
   else                 currTex=SelectSubTextureS(GlobalTextTP,ulClutID);

   if(gTexName!=currTex)
    {gTexName=currTex;glBindTexture(GL_TEXTURE_2D,currTex); glError();}

   if(!bTexEnabled)                                    // -> turn texturing on
    {bTexEnabled=TRUE;glEnable(GL_TEXTURE_2D); glError();}
  }
 else                                                  // no texture ?
 if(bTexEnabled) 
  {bTexEnabled=FALSE;glDisable(GL_TEXTURE_2D); glError();} // -> turn texturing off

 if(bSCol)                                             // also set color ?
  {
   if((dwActFixes&4) && ((DrawAttributes&0x00ffffff)==0))
     DrawAttributes|=0x007f7f7f;

   if(bDrawNonShaded)                                  // -> non shaded?
    {
/*     if(bGLBlend)  vertex[0].c.lcol=0x7f7f7f;          // --> solid color...
     else          */vertex[0].c.lcol=0xffffff;
    }
   else                                                // -> shaded?
    {
//     if(!bUseMultiPass && !bGLBlend)                   // --> given color...
          vertex[0].c.lcol=DoubleBGR2RGB(DrawAttributes);
//     else vertex[0].c.lcol=DrawAttributes;
    }
   vertex[0].c.col[3]=ubGloAlpha;                      // -> set color with
   SETCOL(vertex[0]);                                  //    texture alpha
  }
 
 if(bDrawSmoothShaded!=bOldSmoothShaded)               // shading changed?
  {
   if(bDrawSmoothShaded) glShadeModel(GL_SMOOTH);      // -> set actual shading
   else                  glShadeModel(GL_FLAT);
   glError();
   bOldSmoothShaded=bDrawSmoothShaded;
  }
}

////////////////////////////////////////////////////////////////////////                                          
// Set Opaque multipass color
////////////////////////////////////////////////////////////////////////

void SetOpaqueColor(unsigned int DrawAttributes)
{
 if(bDrawNonShaded) return;                            // no shading? bye
  
 DrawAttributes=DoubleBGR2RGB(DrawAttributes);         // multipass is just half color, so double it on opaque pass
 vertex[0].c.lcol=DrawAttributes|0xff000000;
 SETCOL(vertex[0]);                                    // set color
}

////////////////////////////////////////////////////////////////////////                                          
// Fucking stupid screen coord checking
////////////////////////////////////////////////////////////////////////

BOOL ClipVertexListScreen(void)
{
 if (lx0 >= PSXDisplay.DisplayEnd.x)      goto NEXTSCRTEST;
 if (ly0 >= PSXDisplay.DisplayEnd.y)      goto NEXTSCRTEST;
 if (lx2 <  PSXDisplay.DisplayPosition.x) goto NEXTSCRTEST;
 if (ly2 <  PSXDisplay.DisplayPosition.y) goto NEXTSCRTEST;

 return TRUE;

NEXTSCRTEST:
 if(PSXDisplay.InterlacedTest) return FALSE;

 if (lx0 >= PreviousPSXDisplay.DisplayEnd.x)      return FALSE;
 if (ly0 >= PreviousPSXDisplay.DisplayEnd.y)      return FALSE;
 if (lx2 <  PreviousPSXDisplay.DisplayPosition.x) return FALSE;
 if (ly2 <  PreviousPSXDisplay.DisplayPosition.y) return FALSE;

 return TRUE;
}

////////////////////////////////////////////////////////////////////////

BOOL bDrawOffscreenFront(void)
{
 if(sxmin < PSXDisplay.DisplayPosition.x) return FALSE;   // must be complete in front
 if(symin < PSXDisplay.DisplayPosition.y) return FALSE;
 if(sxmax > PSXDisplay.DisplayEnd.x)      return FALSE;
 if(symax > PSXDisplay.DisplayEnd.y)      return FALSE;
 return TRUE;
}

BOOL bOnePointInFront(void)
{
 if(sxmax< PSXDisplay.DisplayPosition.x)
  return FALSE;

 if(symax< PSXDisplay.DisplayPosition.y)
  return FALSE;

 if(sxmin>=PSXDisplay.DisplayEnd.x)
  return FALSE;

 if(symin>=PSXDisplay.DisplayEnd.y)
  return FALSE;

 return TRUE;
}
 

BOOL bOnePointInBack(void)
{
 if(sxmax< PreviousPSXDisplay.DisplayPosition.x)
  return FALSE;

 if(symax< PreviousPSXDisplay.DisplayPosition.y)
  return FALSE;

 if(sxmin>=PreviousPSXDisplay.DisplayEnd.x)
  return FALSE;

 if(symin>=PreviousPSXDisplay.DisplayEnd.y)
  return FALSE;

 return TRUE;
}
 
BOOL bDrawOffscreen4(void)
{
 BOOL bFront;short sW,sH;

 sxmax=max(lx0,max(lx1,max(lx2,lx3)));
 if(sxmax<drawX) return FALSE;
 sxmin=min(lx0,min(lx1,min(lx2,lx3)));
 if(sxmin>drawW) return FALSE;
 symax=max(ly0,max(ly1,max(ly2,ly3)));
 if(symax<drawY) return FALSE;
 symin=min(ly0,min(ly1,min(ly2,ly3)));
 if(symin>drawH) return FALSE;

 if(PSXDisplay.Disabled) return TRUE;                  // disabled? ever

 if(iOffscreenDrawing==1) return bFullVRam;

 if(dwActFixes&1 && iOffscreenDrawing==4)
  {
   if(PreviousPSXDisplay.DisplayPosition.x==PSXDisplay.DisplayPosition.x &&
      PreviousPSXDisplay.DisplayPosition.y==PSXDisplay.DisplayPosition.y &&
      PreviousPSXDisplay.DisplayEnd.x==PSXDisplay.DisplayEnd.x &&
      PreviousPSXDisplay.DisplayEnd.y==PSXDisplay.DisplayEnd.y)
    {
     bRenderFrontBuffer=TRUE;
     return FALSE;
    }
  }

 sW=drawW-1;sH=drawH-1;
 
 sxmin=min(sW,max(sxmin,drawX));
 sxmax=max(drawX,min(sxmax,sW));
 symin=min(sH,max(symin,drawY));
 symax=max(drawY,min(symax,sH));

 if(bOnePointInBack()) return bFullVRam;

 if(iOffscreenDrawing==2) 
      bFront=bDrawOffscreenFront();
 else bFront=bOnePointInFront();

 if(bFront)
  {
   if(PSXDisplay.InterlacedTest) return bFullVRam;      // -> ok, no need for adjust
                               
   vertex[0].x=lx0 - PSXDisplay.DisplayPosition.x+PreviousPSXDisplay.Range.x0;
   vertex[1].x=lx1 - PSXDisplay.DisplayPosition.x+PreviousPSXDisplay.Range.x0;
   vertex[2].x=lx2 - PSXDisplay.DisplayPosition.x+PreviousPSXDisplay.Range.x0;
   vertex[3].x=lx3 - PSXDisplay.DisplayPosition.x+PreviousPSXDisplay.Range.x0;
   vertex[0].y=ly0 - PSXDisplay.DisplayPosition.y+PreviousPSXDisplay.Range.y0;
   vertex[1].y=ly1 - PSXDisplay.DisplayPosition.y+PreviousPSXDisplay.Range.y0;
   vertex[2].y=ly2 - PSXDisplay.DisplayPosition.y+PreviousPSXDisplay.Range.y0;
   vertex[3].y=ly3 - PSXDisplay.DisplayPosition.y+PreviousPSXDisplay.Range.y0;

   if(iOffscreenDrawing==4 && !(dwActFixes&1))         // -> frontbuffer wanted
    {
     bRenderFrontBuffer=TRUE;
     //return TRUE;
    }
   return bFullVRam;                                   // -> but no od
  }

 return TRUE;
}

////////////////////////////////////////////////////////////////////////
 
BOOL bDrawOffscreen3(void)
{
 BOOL bFront;short sW,sH;

 sxmax=max(lx0,max(lx1,lx2));
 if(sxmax<drawX) return FALSE;
 sxmin=min(lx0,min(lx1,lx2));
 if(sxmin>drawW) return FALSE;
 symax=max(ly0,max(ly1,ly2));
 if(symax<drawY) return FALSE;
 symin=min(ly0,min(ly1,ly2));
 if(symin>drawH) return FALSE;

 if(PSXDisplay.Disabled) return TRUE;                  // disabled? ever

 if(iOffscreenDrawing==1) return bFullVRam;

 sW=drawW-1;sH=drawH-1;
 sxmin=min(sW,max(sxmin,drawX));
 sxmax=max(drawX,min(sxmax,sW));
 symin=min(sH,max(symin,drawY));
 symax=max(drawY,min(symax,sH));

 if(bOnePointInBack()) return bFullVRam;

 if(iOffscreenDrawing==2) 
      bFront=bDrawOffscreenFront();
 else bFront=bOnePointInFront();

 if(bFront)
  {
   if(PSXDisplay.InterlacedTest) return bFullVRam;     // -> ok, no need for adjust

   vertex[0].x=lx0 - PSXDisplay.DisplayPosition.x+PreviousPSXDisplay.Range.x0;
   vertex[1].x=lx1 - PSXDisplay.DisplayPosition.x+PreviousPSXDisplay.Range.x0;
   vertex[2].x=lx2 - PSXDisplay.DisplayPosition.x+PreviousPSXDisplay.Range.x0;
   vertex[0].y=ly0 - PSXDisplay.DisplayPosition.y+PreviousPSXDisplay.Range.y0;
   vertex[1].y=ly1 - PSXDisplay.DisplayPosition.y+PreviousPSXDisplay.Range.y0;
   vertex[2].y=ly2 - PSXDisplay.DisplayPosition.y+PreviousPSXDisplay.Range.y0;

   if(iOffscreenDrawing==4)                            // -> frontbuffer wanted
    {
     bRenderFrontBuffer=TRUE;
   //  return TRUE;
    }

   return bFullVRam;                                   // -> but no od
  }

 return TRUE;
}

////////////////////////////////////////////////////////////////////////

BOOL FastCheckAgainstScreen(short imageX0,short imageY0,short imageX1,short imageY1)
{
 PSXRect_t xUploadArea;

 imageX1 += imageX0;
 imageY1 += imageY0;

 if (imageX0 < PreviousPSXDisplay.DisplayPosition.x)
   xUploadArea.x0 = PreviousPSXDisplay.DisplayPosition.x;
 else
 if (imageX0 > PreviousPSXDisplay.DisplayEnd.x)
   xUploadArea.x0 = PreviousPSXDisplay.DisplayEnd.x;
 else
   xUploadArea.x0 = imageX0;

 if(imageX1 < PreviousPSXDisplay.DisplayPosition.x)
   xUploadArea.x1 = PreviousPSXDisplay.DisplayPosition.x;
 else
 if (imageX1 > PreviousPSXDisplay.DisplayEnd.x)
   xUploadArea.x1 = PreviousPSXDisplay.DisplayEnd.x;
 else
   xUploadArea.x1 = imageX1;

 if (imageY0 < PreviousPSXDisplay.DisplayPosition.y)
   xUploadArea.y0 = PreviousPSXDisplay.DisplayPosition.y;
 else
 if (imageY0 > PreviousPSXDisplay.DisplayEnd.y)
   xUploadArea.y0 = PreviousPSXDisplay.DisplayEnd.y;
 else
   xUploadArea.y0 = imageY0;

 if (imageY1 < PreviousPSXDisplay.DisplayPosition.y)
   xUploadArea.y1 = PreviousPSXDisplay.DisplayPosition.y;
 else
 if (imageY1 > PreviousPSXDisplay.DisplayEnd.y)
   xUploadArea.y1 = PreviousPSXDisplay.DisplayEnd.y;
 else
   xUploadArea.y1 = imageY1;

 if ((xUploadArea.x0 != xUploadArea.x1) && (xUploadArea.y0 != xUploadArea.y1))
      return TRUE;
 else return FALSE;
}

BOOL CheckAgainstScreen(short imageX0,short imageY0,short imageX1,short imageY1)
{
 imageX1 += imageX0;
 imageY1 += imageY0;

 if (imageX0 < PreviousPSXDisplay.DisplayPosition.x)
   xrUploadArea.x0 = PreviousPSXDisplay.DisplayPosition.x;
 else
 if (imageX0 > PreviousPSXDisplay.DisplayEnd.x)
   xrUploadArea.x0 = PreviousPSXDisplay.DisplayEnd.x;
 else
   xrUploadArea.x0 = imageX0;

 if(imageX1 < PreviousPSXDisplay.DisplayPosition.x)
   xrUploadArea.x1 = PreviousPSXDisplay.DisplayPosition.x;
 else
 if (imageX1 > PreviousPSXDisplay.DisplayEnd.x)
   xrUploadArea.x1 = PreviousPSXDisplay.DisplayEnd.x;
 else
   xrUploadArea.x1 = imageX1;

 if (imageY0 < PreviousPSXDisplay.DisplayPosition.y)
   xrUploadArea.y0 = PreviousPSXDisplay.DisplayPosition.y;
 else
 if (imageY0 > PreviousPSXDisplay.DisplayEnd.y)
   xrUploadArea.y0 = PreviousPSXDisplay.DisplayEnd.y;
 else
   xrUploadArea.y0 = imageY0;

 if (imageY1 < PreviousPSXDisplay.DisplayPosition.y)
   xrUploadArea.y1 = PreviousPSXDisplay.DisplayPosition.y;
 else
 if (imageY1 > PreviousPSXDisplay.DisplayEnd.y)
   xrUploadArea.y1 = PreviousPSXDisplay.DisplayEnd.y;
 else
   xrUploadArea.y1 = imageY1;

 if ((xrUploadArea.x0 != xrUploadArea.x1) && (xrUploadArea.y0 != xrUploadArea.y1))
      return TRUE;
 else return FALSE;
}

BOOL FastCheckAgainstFrontScreen(short imageX0,short imageY0,short imageX1,short imageY1)
{
 PSXRect_t xUploadArea;

 imageX1 += imageX0;
 imageY1 += imageY0;

 if (imageX0 < PSXDisplay.DisplayPosition.x)
   xUploadArea.x0 = PSXDisplay.DisplayPosition.x;
 else
 if (imageX0 > PSXDisplay.DisplayEnd.x)
   xUploadArea.x0 = PSXDisplay.DisplayEnd.x;
 else
   xUploadArea.x0 = imageX0;

 if(imageX1 < PSXDisplay.DisplayPosition.x)
   xUploadArea.x1 = PSXDisplay.DisplayPosition.x;
 else
 if (imageX1 > PSXDisplay.DisplayEnd.x)
   xUploadArea.x1 = PSXDisplay.DisplayEnd.x;
 else
   xUploadArea.x1 = imageX1;

 if (imageY0 < PSXDisplay.DisplayPosition.y)
   xUploadArea.y0 = PSXDisplay.DisplayPosition.y;
 else
 if (imageY0 > PSXDisplay.DisplayEnd.y)
   xUploadArea.y0 = PSXDisplay.DisplayEnd.y;
 else
   xUploadArea.y0 = imageY0;

 if (imageY1 < PSXDisplay.DisplayPosition.y)
   xUploadArea.y1 = PSXDisplay.DisplayPosition.y;
 else
 if (imageY1 > PSXDisplay.DisplayEnd.y)
   xUploadArea.y1 = PSXDisplay.DisplayEnd.y;
 else
   xUploadArea.y1 = imageY1;

 if ((xUploadArea.x0 != xUploadArea.x1) && (xUploadArea.y0 != xUploadArea.y1))
      return TRUE; 
 else return FALSE;
}

BOOL CheckAgainstFrontScreen(short imageX0,short imageY0,short imageX1,short imageY1)
{
 imageX1 += imageX0;
 imageY1 += imageY0;

 if (imageX0 < PSXDisplay.DisplayPosition.x)
   xrUploadArea.x0 = PSXDisplay.DisplayPosition.x;
 else
 if (imageX0 > PSXDisplay.DisplayEnd.x)
   xrUploadArea.x0 = PSXDisplay.DisplayEnd.x;
 else
   xrUploadArea.x0 = imageX0;

 if(imageX1 < PSXDisplay.DisplayPosition.x)
   xrUploadArea.x1 = PSXDisplay.DisplayPosition.x;
 else
 if (imageX1 > PSXDisplay.DisplayEnd.x)
   xrUploadArea.x1 = PSXDisplay.DisplayEnd.x;
 else
   xrUploadArea.x1 = imageX1;

 if (imageY0 < PSXDisplay.DisplayPosition.y)
   xrUploadArea.y0 = PSXDisplay.DisplayPosition.y;
 else
 if (imageY0 > PSXDisplay.DisplayEnd.y)
   xrUploadArea.y0 = PSXDisplay.DisplayEnd.y;
 else
   xrUploadArea.y0 = imageY0;

 if (imageY1 < PSXDisplay.DisplayPosition.y)
   xrUploadArea.y1 = PSXDisplay.DisplayPosition.y;
 else
 if (imageY1 > PSXDisplay.DisplayEnd.y)
   xrUploadArea.y1 = PSXDisplay.DisplayEnd.y;
 else
   xrUploadArea.y1 = imageY1;

 if ((xrUploadArea.x0 != xrUploadArea.x1) && (xrUploadArea.y0 != xrUploadArea.y1))
      return TRUE; 
 else return FALSE;
}

////////////////////////////////////////////////////////////////////////

void PrepareFullScreenUpload (int Position)
{
 if (Position==-1)                                     // rgb24
  {
   if(PSXDisplay.Interlaced)
    {
     xrUploadArea.x0 = PSXDisplay.DisplayPosition.x;
     xrUploadArea.x1 = PSXDisplay.DisplayEnd.x;
     xrUploadArea.y0 = PSXDisplay.DisplayPosition.y;
     xrUploadArea.y1 = PSXDisplay.DisplayEnd.y;
    }
   else
    {
     xrUploadArea.x0 = PreviousPSXDisplay.DisplayPosition.x;
     xrUploadArea.x1 = PreviousPSXDisplay.DisplayEnd.x;
     xrUploadArea.y0 = PreviousPSXDisplay.DisplayPosition.y;
     xrUploadArea.y1 = PreviousPSXDisplay.DisplayEnd.y;
    }

   if(bNeedRGB24Update)
    {
     if(lClearOnSwap) 
      {
//       lClearOnSwap=0;
      }
     else    
     if(PSXDisplay.Interlaced && PreviousPSXDisplay.RGB24<2) // in interlaced mode we upload at least two full frames (GT1 menu)
      {
       PreviousPSXDisplay.RGB24++;
      }
     else
      {
       xrUploadArea.y1 = min(xrUploadArea.y0+xrUploadAreaRGB24.y1,xrUploadArea.y1);
       xrUploadArea.y0+=xrUploadAreaRGB24.y0;
      }
    }
  }
 else
 if (Position)
  {
   xrUploadArea.x0 = PSXDisplay.DisplayPosition.x;
   xrUploadArea.x1 = PSXDisplay.DisplayEnd.x;
   xrUploadArea.y0 = PSXDisplay.DisplayPosition.y;
   xrUploadArea.y1 = PSXDisplay.DisplayEnd.y;
  }
 else
  {
   xrUploadArea.x0 = PreviousPSXDisplay.DisplayPosition.x;
   xrUploadArea.x1 = PreviousPSXDisplay.DisplayEnd.x;
   xrUploadArea.y0 = PreviousPSXDisplay.DisplayPosition.y;
   xrUploadArea.y1 = PreviousPSXDisplay.DisplayEnd.y;
  }

 if (xrUploadArea.x0 < 0)               xrUploadArea.x0 = 0;
 else
 if (xrUploadArea.x0 > 1023)            xrUploadArea.x0 = 1023;

 if (xrUploadArea.x1 < 0)               xrUploadArea.x1 = 0;
 else
 if (xrUploadArea.x1 > 1024)            xrUploadArea.x1 = 1024;

 if (xrUploadArea.y0 < 0)               xrUploadArea.y0 = 0;
 else
 if (xrUploadArea.y0 > iGPUHeightMask)  xrUploadArea.y0 = iGPUHeightMask;

 if (xrUploadArea.y1 < 0)               xrUploadArea.y1 = 0;
 else
 if (xrUploadArea.y1 > iGPUHeight)      xrUploadArea.y1 = iGPUHeight;

 if (PSXDisplay.RGB24)
  {
   InvalidateTextureArea(xrUploadArea.x0,xrUploadArea.y0,xrUploadArea.x1-xrUploadArea.x0,xrUploadArea.y1-xrUploadArea.y0);
  }
}

////////////////////////////////////////////////////////////////////////
// Upload screen (MDEC and such)
////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////

unsigned char * LoadDirectMovieFast(void);

void UploadScreenEx(int Position)
{
 short ya,yb,xa,xb,x, y, YStep, XStep, U, UStep,ux[4],vy[4];

 if(!PSXDisplay.DisplayMode.x) return;
 if(!PSXDisplay.DisplayMode.y) return;

 glDisable(GL_SCISSOR_TEST); glError();
 glShadeModel(GL_FLAT); glError();
 bOldSmoothShaded=FALSE;
 glDisable(GL_BLEND); glError();
 bBlendEnable=FALSE;
 glDisable(GL_TEXTURE_2D); glError();
 bTexEnabled=FALSE;
 glDisable(GL_ALPHA_TEST); glError();

 //glPixelZoom(((float)rRatioRect.right)/((float)PSXDisplay.DisplayMode.x),
 //            -1.0f*(((float)rRatioRect.bottom)/((float)PSXDisplay.DisplayMode.y)));
                                                      
 //----------------------------------------------------//

 YStep = 256;                                          // max texture size
 XStep = 256;
 UStep = (PSXDisplay.RGB24 ? 128 : 0);
 ya    = xrUploadArea.y0;
 yb    = xrUploadArea.y1;
 xa    = xrUploadArea.x0;
 xb    = xrUploadArea.x1;
 
 for(y=ya;y<=yb;y+=YStep)                              // loop y
  {
   U = 0;
   for(x=xa;x<=xb;x+=XStep)                            // loop x
    {
     ly0 = ly1 = y;                                    // -> get y coords
     ly2 = y + YStep;
     if (ly2 > yb) ly2 = yb;
     ly3 = ly2;

     lx0 = lx3 = x;                                    // -> get x coords
     lx1 = x + XStep;
     if (lx1 > xb) lx1 = xb;

     lx2 = lx1;

     ux[0]=ux[3]=(xa - x);                             // -> set tex x coords
     if (ux[0] < 0) ux[0]=ux[3]=0;
     ux[2]=ux[1]=(xb - x);
     if (ux[2] > 256) ux[2]=ux[1]=256;

     vy[0]=vy[1]=(ya - y);                             // -> set tex y coords
     if (vy[0] < 0) vy[0]=vy[1]=0;
     vy[2]=vy[3]=(yb - y);
     if (vy[2] > 256) vy[2]=vy[3]=256;

     if ((ux[0] >= ux[2]) ||                           // -> cheaters never win...
         (vy[0] >= vy[2])) continue;                   //    (but winners always cheat...)
                
     xrMovieArea.x0=lx0+U; xrMovieArea.y0=ly0;
     xrMovieArea.x1=lx2+U; xrMovieArea.y1=ly2;
     
     offsetScreenUpload(Position);

     //glRasterPos2f(vertex[0].x,vertex[0].y);

     //glDrawPixels(xrMovieArea.x1-xrMovieArea.x0,
     //             xrMovieArea.y1-xrMovieArea.y0,
     //             GL_RGBA,GL_UNSIGNED_BYTE,
                  LoadDirectMovieFast();//);

     U+=UStep;
    }
  }

 //----------------------------------------------------//

// glPixelZoom(1.0F,1.0F);

 glEnable(GL_ALPHA_TEST); glError();
 glEnable(GL_SCISSOR_TEST); glError();
}

////////////////////////////////////////////////////////////////////////

void UploadScreen(int Position)
{
 short x, y, YStep, XStep, U, s, UStep,ux[4],vy[4];
 short xa,xb,ya,yb;

 if(xrUploadArea.x0>1023) xrUploadArea.x0=1023;
 if(xrUploadArea.x1>1024) xrUploadArea.x1=1024;
 if(xrUploadArea.y0>iGPUHeightMask)  xrUploadArea.y0=iGPUHeightMask;
 if(xrUploadArea.y1>iGPUHeight)      xrUploadArea.y1=iGPUHeight;

 if(xrUploadArea.x0==xrUploadArea.x1) return;
 if(xrUploadArea.y0==xrUploadArea.y1) return;

 if(PSXDisplay.Disabled && iOffscreenDrawing<4) return;

 iDrawnSomething   = 2;
 iLastRGB24=PSXDisplay.RGB24+1;

 if(bSkipNextFrame) return;

 if(dwActFixes & 2) {UploadScreenEx(Position);return;}

 bUsingMovie       = TRUE;
 bDrawTextured     = TRUE;                             // just doing textures
 bDrawSmoothShaded = FALSE;

/* if(bGLBlend) vertex[0].c.lcol=0xff7f7f7f;             // set solid col
 else          */vertex[0].c.lcol=0xffffffff;
 SETCOL(vertex[0]); 

 SetOGLDisplaySettings(0);

 YStep = 256;                                          // max texture size
 XStep = 256;

 UStep = (PSXDisplay.RGB24 ? 128 : 0);
 
 ya=xrUploadArea.y0;
 yb=xrUploadArea.y1;
 xa=xrUploadArea.x0;
 xb=xrUploadArea.x1;

 for(y=ya;y<=yb;y+=YStep)                              // loop y
  {
   U = 0;
   for(x=xa;x<=xb;x+=XStep)                            // loop x
    {
     ly0 = ly1 = y;                                    // -> get y coords
     ly2 = y + YStep;
     if (ly2 > yb) ly2 = yb;
     ly3 = ly2;

     lx0 = lx3 = x;                                    // -> get x coords
     lx1 = x + XStep;
     if (lx1 > xb) lx1 = xb;

     lx2 = lx1;

     ux[0]=ux[3]=(xa - x);                             // -> set tex x coords
     if (ux[0] < 0) ux[0]=ux[3]=0;
     ux[2]=ux[1]=(xb - x);
     if (ux[2] > 256) ux[2]=ux[1]=256;

     vy[0]=vy[1]=(ya - y);                             // -> set tex y coords
     if (vy[0] < 0) vy[0]=vy[1]=0;
     vy[2]=vy[3]=(yb - y);
     if (vy[2] > 256) vy[2]=vy[3]=256;

     if ((ux[0] >= ux[2]) ||                           // -> cheaters never win...
         (vy[0] >= vy[2])) continue;                   //    (but winners always cheat...)
                
     xrMovieArea.x0=lx0+U; xrMovieArea.y0=ly0;
     xrMovieArea.x1=lx2+U; xrMovieArea.y1=ly2;

     s=ux[2] - ux[0]; if(s>255) s=255;

     gl_ux[2] = gl_ux[1] = s;
     s=vy[2] - vy[0]; if(s>255) s=255;
     gl_vy[2] = gl_vy[3] = s;
     gl_ux[0] = gl_ux[3] = gl_vy[0] = gl_vy[1] = 0;

     SetRenderState((unsigned int)0x01000000);
     SetRenderMode((unsigned int)0x01000000, FALSE);  // upload texture data
     offsetScreenUpload(Position);
     assignTextureVRAMWrite();

     PRIMdrawTexturedQuad(&vertex[0], &vertex[1], &vertex[2], &vertex[3]);

     U+=UStep;
    }
  }

 bUsingMovie=FALSE;                                    // done...
 bDisplayNotSet = TRUE;
}

////////////////////////////////////////////////////////////////////////
// Detect next screen
////////////////////////////////////////////////////////////////////////

BOOL IsCompleteInsideNextScreen(short x, short y, short xoff, short yoff)
{        
 if (x > PSXDisplay.DisplayPosition.x+1)     return FALSE;
 if ((x + xoff) < PSXDisplay.DisplayEnd.x-1) return FALSE;
 yoff+=y;
 if (y >= PSXDisplay.DisplayPosition.y &&
     y <= PSXDisplay.DisplayEnd.y )
  {
   if ((yoff) >= PSXDisplay.DisplayPosition.y &&
       (yoff) <= PSXDisplay.DisplayEnd.y ) return TRUE;
  }   
 if (y > PSXDisplay.DisplayPosition.y+1) return FALSE;
 if (yoff < PSXDisplay.DisplayEnd.y-1)   return FALSE;
 return TRUE;
}

BOOL IsPrimCompleteInsideNextScreen(short x, short y, short xoff, short yoff)
{
 x+=PSXDisplay.DrawOffset.x;
 if (x > PSXDisplay.DisplayPosition.x+1) return FALSE;
 y+=PSXDisplay.DrawOffset.y;
 if (y > PSXDisplay.DisplayPosition.y+1) return FALSE;
 xoff+=PSXDisplay.DrawOffset.x;
 if (xoff < PSXDisplay.DisplayEnd.x-1)   return FALSE;
 yoff+=PSXDisplay.DrawOffset.y;
 if (yoff < PSXDisplay.DisplayEnd.y-1)   return FALSE;
 return TRUE;
}

BOOL IsInsideNextScreen(short x, short y, short xoff, short yoff)
{                    
 if (x > PSXDisplay.DisplayEnd.x) return FALSE;
 if (y > PSXDisplay.DisplayEnd.y) return FALSE;
 if ((x + xoff) < PSXDisplay.DisplayPosition.x) return FALSE;
 if ((y + yoff) < PSXDisplay.DisplayPosition.y) return FALSE;
 return TRUE;
}

////////////////////////////////////////////////////////////////////////
// mask stuff...
////////////////////////////////////////////////////////////////////////

//Mask1    Set mask bit while drawing. 1 = on
//Mask2    Do not draw to mask areas. 1= on

void cmdSTP(unsigned char * baseAddr)
{
 unsigned int gdata = ((unsigned int*)baseAddr)[0];

 STATUSREG&=~0x1800;                                   // clear the necessary bits
 STATUSREG|=((gdata & 0x03) << 11);                    // set the current bits

 if(!iUseMask) return;

 if(gdata&1) {sSetMask=0x8000;lSetMask=0x80008000;iSetMask=1;}
 else        {sSetMask=0;     lSetMask=0;         iSetMask=0;}

 if(gdata&2) 
  {
   if(!(gdata&1)) iSetMask=2;
   bCheckMask=TRUE;
   if(iDepthFunc==0) return;
   iDepthFunc=0;
   glDepthFunc(GL_LESS); glError();
  }
 else
  {
   bCheckMask=FALSE;
   if(iDepthFunc==1) return;
   glDepthFunc(GL_ALWAYS); glError();
   iDepthFunc=1;
  }
}

////////////////////////////////////////////////////////////////////////
// cmd: Set texture page infos
////////////////////////////////////////////////////////////////////////

void cmdTexturePage(unsigned char * baseAddr)
{
 unsigned int gdata = ((unsigned int*)baseAddr)[0];
 UpdateGlobalTP((unsigned short)gdata);
 GlobalTextREST = (gdata&0x00ffffff)>>9;
}

////////////////////////////////////////////////////////////////////////
// cmd: turn on/off texture window
////////////////////////////////////////////////////////////////////////

void cmdTextureWindow(unsigned char *baseAddr)
{
 unsigned int gdata = ((unsigned int*)baseAddr)[0];

 unsigned int YAlign,XAlign;

 ulGPUInfoVals[INFO_TW]=gdata&0xFFFFF;

 if(gdata & 0x020)
  TWin.Position.y1 = 8;    // xxxx1
 else if (gdata & 0x040)
  TWin.Position.y1 = 16;   // xxx10
 else if (gdata & 0x080)
  TWin.Position.y1 = 32;   // xx100
 else if (gdata & 0x100)
  TWin.Position.y1 = 64;   // x1000
 else if (gdata & 0x200)
  TWin.Position.y1 = 128;  // 10000
 else
  TWin.Position.y1 = 256;  // 00000

  // Texture window size is determined by the least bit set of the relevant 5 bits

 if (gdata & 0x001)
  TWin.Position.x1 = 8;    // xxxx1
 else if (gdata & 0x002)
  TWin.Position.x1 = 16;   // xxx10
 else if (gdata & 0x004)
  TWin.Position.x1 = 32;   // xx100
 else if (gdata & 0x008)
  TWin.Position.x1 = 64;   // x1000
 else if (gdata & 0x010)
  TWin.Position.x1 = 128;  // 10000
 else
  TWin.Position.x1 = 256;  // 00000

 // Re-calculate the bit field, because we can't trust what is passed in the data

 YAlign = (unsigned int)(32 - (TWin.Position.y1 >> 3));
 XAlign = (unsigned int)(32 - (TWin.Position.x1 >> 3));

 // Absolute position of the start of the texture window

 TWin.Position.y0 = (short)(((gdata >> 15) & YAlign) << 3);
 TWin.Position.x0 = (short)(((gdata >> 10) & XAlign) << 3);

 if((TWin.Position.x0 == 0 &&                          // tw turned off
     TWin.Position.y0 == 0 &&
     TWin.Position.x1 == 0 &&
     TWin.Position.y1 == 0) ||
     (TWin.Position.x1 == 256 &&
      TWin.Position.y1 == 256))
  {
   bUsingTWin = FALSE;                                 // -> just do it

#ifdef OWNSCALE
   TWin.UScaleFactor = 1.0f;
   TWin.VScaleFactor = 1.0f;
#else
   TWin.UScaleFactor = 
   TWin.VScaleFactor = 1.0f/256.0f;
#endif
  }
 else                                                  // tw turned on
  {
   bUsingTWin = TRUE;

   TWin.OPosition.y1 = TWin.Position.y1;               // -> get psx sizes
   TWin.OPosition.x1 = TWin.Position.x1;              

   if(TWin.Position.x1<=2)   TWin.Position.x1=2;       // -> set OGL sizes
   else
   if(TWin.Position.x1<=4)   TWin.Position.x1=4;
   else
   if(TWin.Position.x1<=8)   TWin.Position.x1=8;
   else
   if(TWin.Position.x1<=16)  TWin.Position.x1=16;
   else
   if(TWin.Position.x1<=32)  TWin.Position.x1=32;
   else
   if(TWin.Position.x1<=64)  TWin.Position.x1=64;
   else
   if(TWin.Position.x1<=128) TWin.Position.x1=128;
   else
   if(TWin.Position.x1<=256) TWin.Position.x1=256;
   
   if(TWin.Position.y1<=2)   TWin.Position.y1=2;
   else
   if(TWin.Position.y1<=4)   TWin.Position.y1=4;
   else
   if(TWin.Position.y1<=8)   TWin.Position.y1=8;
   else
   if(TWin.Position.y1<=16)  TWin.Position.y1=16;
   else
   if(TWin.Position.y1<=32)  TWin.Position.y1=32;
   else
   if(TWin.Position.y1<=64)  TWin.Position.y1=64;
   else
   if(TWin.Position.y1<=128) TWin.Position.y1=128;
   else
   if(TWin.Position.y1<=256) TWin.Position.y1=256;

#ifdef OWNSCALE
   TWin.UScaleFactor = (float)TWin.Position.x1;
   TWin.VScaleFactor = (float)TWin.Position.y1;
#else
   TWin.UScaleFactor = ((float)TWin.Position.x1)/256.0f; // -> set scale factor
   TWin.VScaleFactor = ((float)TWin.Position.y1)/256.0f;
#endif
  }
}

////////////////////////////////////////////////////////////////////////
// mmm, Lewpy uses that in TileS ... I don't ;)
////////////////////////////////////////////////////////////////////////

/*
void ClampToPSXDrawAreaOffset(short *x0, short *y0, short *x1, short *y1)
{
 if (*x0 < PSXDisplay.DrawArea.x0)
  {
   *x1 -= (PSXDisplay.DrawArea.x0 - *x0);
   *x0 = PSXDisplay.DrawArea.x0;
  }
 else
 if (*x0 > PSXDisplay.DrawArea.x1)
  {
   *x0 = PSXDisplay.DrawArea.x1;
   *x1 = 0;
  }

 if (*y0 < PSXDisplay.DrawArea.y0)
  {
   *y1 -= (PSXDisplay.DrawArea.y0 - *y0);
   *y0 = PSXDisplay.DrawArea.y0;
  }
 else
 if (*y0 > PSXDisplay.DrawArea.y1)
  {
   *y0 = PSXDisplay.DrawArea.y1;
   *y1 = 0;
  }

 if (*x1 < 0) *x1 = 0;

 if ((*x1 + *x0) > PSXDisplay.DrawArea.x1)
  *x1 = (PSXDisplay.DrawArea.x1 -  *x0 + 1);

 if (*y1 < 0) *y1 = 0;

 if ((*y1 + *y0) > PSXDisplay.DrawArea.y1)
  *y1 = (PSXDisplay.DrawArea.y1 -  *y0 + 1);
}
*/

////////////////////////////////////////////////////////////////////////
// Check draw area dimensions
////////////////////////////////////////////////////////////////////////

void ClampToPSXScreen(short *x0, short *y0, short *x1, short *y1)
{
 if (*x0 < 0)               *x0 = 0;
 else
 if (*x0 > 1023)            *x0 = 1023;
            
 if (*x1 < 0)               *x1 = 0;
 else
 if (*x1 > 1023)            *x1 = 1023;

 if (*y0 < 0)               *y0 = 0;
 else
 if (*y0 > iGPUHeightMask)  *y0 = iGPUHeightMask;
            
 if (*y1 < 0)               *y1 = 0;
 else
 if (*y1 > iGPUHeightMask)  *y1 = iGPUHeightMask;
}

////////////////////////////////////////////////////////////////////////
// Used in Load Image and Blk Fill
////////////////////////////////////////////////////////////////////////

void ClampToPSXScreenOffset(short *x0, short *y0, short *x1, short *y1)
{
 if (*x0 < 0)
  { *x1 += *x0;  *x0 = 0; }
 else
 if (*x0 > 1023)
  { *x0 = 1023;  *x1 = 0; }

 if (*y0 < 0)
  { *y1 += *y0;  *y0 = 0; }
 else
 if (*y0 > iGPUHeightMask)
  { *y0 = iGPUHeightMask;   *y1 = 0; }

 if (*x1 < 0) *x1 = 0;

 if ((*x1 + *x0) > 1024) *x1 = (1024 -  *x0);

 if (*y1 < 0) *y1 = 0;

 if ((*y1 + *y0) > iGPUHeight)  *y1 = (iGPUHeight -  *y0);
}

////////////////////////////////////////////////////////////////////////
// cmd: start of drawing area... primitives will be clipped inside
////////////////////////////////////////////////////////////////////////

void cmdDrawAreaStart(unsigned char * baseAddr)
{
 unsigned int gdata = ((unsigned int*)baseAddr)[0];

 drawX = gdata & 0x3ff;                                // for soft drawing
 if(drawX>=1024) drawX=1023;

 if(dwGPUVersion==2)
  {
   ulGPUInfoVals[INFO_DRAWSTART]=gdata&0x3FFFFF;
   drawY  = (gdata>>12)&0x3ff;
  }
 else
  {
   ulGPUInfoVals[INFO_DRAWSTART]=gdata&0xFFFFF;
   drawY  = (gdata>>10)&0x3ff;
  }

 if(drawY>=iGPUHeight) drawY=iGPUHeightMask;

 PreviousPSXDisplay.DrawArea.y0=PSXDisplay.DrawArea.y0;
 PreviousPSXDisplay.DrawArea.x0=PSXDisplay.DrawArea.x0;

 PSXDisplay.DrawArea.y0 = (short)drawY;                // for OGL drawing
 PSXDisplay.DrawArea.x0 = (short)drawX;
}

////////////////////////////////////////////////////////////////////////
// cmd: end of drawing area... primitives will be clipped inside
////////////////////////////////////////////////////////////////////////

void cmdDrawAreaEnd(unsigned char * baseAddr)
{
 unsigned int gdata = ((unsigned int*)baseAddr)[0];

 drawW = gdata & 0x3ff;                                // for soft drawing
 if(drawW>=1024) drawW=1023;

 if(dwGPUVersion==2)
  {
   ulGPUInfoVals[INFO_DRAWEND]=gdata&0x3FFFFF;
   drawH  = (gdata>>12)&0x3ff;
  }
 else
  {
   ulGPUInfoVals[INFO_DRAWEND]=gdata&0xFFFFF;
   drawH  = (gdata>>10)&0x3ff;
  }
 
 if(drawH>=iGPUHeight) drawH=iGPUHeightMask;

 PSXDisplay.DrawArea.y1 = (short)drawH;                // for OGL drawing
 PSXDisplay.DrawArea.x1 = (short)drawW;

 ClampToPSXScreen(&PSXDisplay.DrawArea.x0,             // clamp
                  &PSXDisplay.DrawArea.y0,
                  &PSXDisplay.DrawArea.x1,
                  &PSXDisplay.DrawArea.y1);

 bDisplayNotSet = TRUE;
}

////////////////////////////////////////////////////////////////////////
// cmd: draw offset... will be added to prim coords
////////////////////////////////////////////////////////////////////////

void cmdDrawOffset(unsigned char * baseAddr)
{
 unsigned int gdata = ((unsigned int*)baseAddr)[0];

 PreviousPSXDisplay.DrawOffset.x = 
  PSXDisplay.DrawOffset.x = (short)(gdata & 0x7ff);

 if(dwGPUVersion==2)
  {
   ulGPUInfoVals[INFO_DRAWOFF]=gdata&0x7FFFFF;
   PSXDisplay.DrawOffset.y = (short)((gdata>>12) & 0x7ff);
  }
 else
  {
   ulGPUInfoVals[INFO_DRAWOFF]=gdata&0x3FFFFF;
   PSXDisplay.DrawOffset.y = (short)((gdata>>11) & 0x7ff);
  }
 
 PSXDisplay.DrawOffset.x=(short)(((int)PSXDisplay.DrawOffset.x<<21)>>21);
 PSXDisplay.DrawOffset.y=(short)(((int)PSXDisplay.DrawOffset.y<<21)>>21);

 PSXDisplay.CumulOffset.x =                            // new OGL prim offsets
  PSXDisplay.DrawOffset.x - PSXDisplay.GDrawOffset.x + PreviousPSXDisplay.Range.x0;
 PSXDisplay.CumulOffset.y = 
  PSXDisplay.DrawOffset.y - PSXDisplay.GDrawOffset.y + PreviousPSXDisplay.Range.y0;
}

////////////////////////////////////////////////////////////////////////
// cmd: load image to vram
////////////////////////////////////////////////////////////////////////

void primLoadImage(unsigned char * baseAddr)
{
 unsigned short *sgpuData = ((unsigned short *) baseAddr);

 VRAMWrite.x      = sgpuData[2]&0x03ff;
 VRAMWrite.y      = sgpuData[3]&iGPUHeightMask;
 VRAMWrite.Width  = sgpuData[4];
 VRAMWrite.Height = sgpuData[5];

 iDataWriteMode = DR_VRAMTRANSFER;
 VRAMWrite.ImagePtr = psxVuw + (VRAMWrite.y<<10) + VRAMWrite.x;
 VRAMWrite.RowsRemaining = VRAMWrite.Width;
 VRAMWrite.ColsRemaining = VRAMWrite.Height;

 bNeedWriteUpload=TRUE;
}

////////////////////////////////////////////////////////////////////////

void PrepareRGB24Upload(void)
{
 VRAMWrite.x=(VRAMWrite.x*2)/3;
 VRAMWrite.Width=(VRAMWrite.Width*2)/3;

 if(!PSXDisplay.InterlacedTest && // NEW
    CheckAgainstScreen(VRAMWrite.x, VRAMWrite.y, VRAMWrite.Width, VRAMWrite.Height))
  {
   xrUploadArea.x0-=PreviousPSXDisplay.DisplayPosition.x;
   xrUploadArea.x1-=PreviousPSXDisplay.DisplayPosition.x;
   xrUploadArea.y0-=PreviousPSXDisplay.DisplayPosition.y;
   xrUploadArea.y1-=PreviousPSXDisplay.DisplayPosition.y;
  }  
 else
 if(CheckAgainstFrontScreen(VRAMWrite.x, VRAMWrite.y, VRAMWrite.Width, VRAMWrite.Height))
  {
   xrUploadArea.x0-=PSXDisplay.DisplayPosition.x;
   xrUploadArea.x1-=PSXDisplay.DisplayPosition.x;
   xrUploadArea.y0-=PSXDisplay.DisplayPosition.y;
   xrUploadArea.y1-=PSXDisplay.DisplayPosition.y;
  }  
 else return;

 if(bRenderFrontBuffer) 
  {
   updateFrontDisplay();
  }

 if(bNeedRGB24Update==FALSE)
  {
   xrUploadAreaRGB24=xrUploadArea;
   bNeedRGB24Update=TRUE;
  }
 else
  {
   xrUploadAreaRGB24.x0=min(xrUploadAreaRGB24.x0,xrUploadArea.x0);
   xrUploadAreaRGB24.x1=max(xrUploadAreaRGB24.x1,xrUploadArea.x1);
   xrUploadAreaRGB24.y0=min(xrUploadAreaRGB24.y0,xrUploadArea.y0);
   xrUploadAreaRGB24.y1=max(xrUploadAreaRGB24.y1,xrUploadArea.y1);
  }
}

////////////////////////////////////////////////////////////////////////

void CheckWriteUpdate()
{
 int iX=0,iY=0;

 if(VRAMWrite.Width)   iX=1;
 if(VRAMWrite.Height)  iY=1;

 InvalidateTextureArea(VRAMWrite.x, VRAMWrite.y, VRAMWrite.Width-iX, VRAMWrite.Height-iY);

 if(PSXDisplay.Interlaced && !iOffscreenDrawing) return;

 if(PSXDisplay.RGB24) {PrepareRGB24Upload();return;}

 if(!PSXDisplay.InterlacedTest &&
    CheckAgainstScreen(VRAMWrite.x, VRAMWrite.y, VRAMWrite.Width, VRAMWrite.Height)) 
  {
   if(dwActFixes&0x800) return;

   if(bRenderFrontBuffer) 
    {
     updateFrontDisplay();
    }

   UploadScreen(FALSE);

   bNeedUploadTest=TRUE;
  }
 else 
 if(iOffscreenDrawing)
  {
   if (CheckAgainstFrontScreen(VRAMWrite.x, VRAMWrite.y, VRAMWrite.Width, VRAMWrite.Height)) 
    {
     if(PSXDisplay.InterlacedTest)
      {
       if(PreviousPSXDisplay.InterlacedNew) 
        {
         PreviousPSXDisplay.InterlacedNew=FALSE;
         bNeedInterlaceUpdate=TRUE;
         xrUploadAreaIL.x0=PSXDisplay.DisplayPosition.x;
         xrUploadAreaIL.y0=PSXDisplay.DisplayPosition.y;
         xrUploadAreaIL.x1=PSXDisplay.DisplayPosition.x+PSXDisplay.DisplayModeNew.x;
         xrUploadAreaIL.y1=PSXDisplay.DisplayPosition.y+PSXDisplay.DisplayModeNew.y;
         if(xrUploadAreaIL.x1>1023) xrUploadAreaIL.x1=1023;
         if(xrUploadAreaIL.y1>511)  xrUploadAreaIL.y1=511;
        }

       if(bNeedInterlaceUpdate==FALSE)
        {
         xrUploadAreaIL=xrUploadArea;
         bNeedInterlaceUpdate=TRUE;
        }
       else
        {
         xrUploadAreaIL.x0=min(xrUploadAreaIL.x0,xrUploadArea.x0);
         xrUploadAreaIL.x1=max(xrUploadAreaIL.x1,xrUploadArea.x1);
         xrUploadAreaIL.y0=min(xrUploadAreaIL.y0,xrUploadArea.y0);
         xrUploadAreaIL.y1=max(xrUploadAreaIL.y1,xrUploadArea.y1);
        }
       return;
      }

     if(!bNeedUploadAfter)
      {
       bNeedUploadAfter = TRUE;
       xrUploadArea.x0=VRAMWrite.x;
       xrUploadArea.x1=VRAMWrite.x+VRAMWrite.Width;
       xrUploadArea.y0=VRAMWrite.y;
       xrUploadArea.y1=VRAMWrite.y+VRAMWrite.Height;
      }
     else
      {
       xrUploadArea.x0=min(xrUploadArea.x0,VRAMWrite.x);
       xrUploadArea.x1=max(xrUploadArea.x1,VRAMWrite.x+VRAMWrite.Width);
       xrUploadArea.y0=min(xrUploadArea.y0,VRAMWrite.y);
       xrUploadArea.y1=max(xrUploadArea.y1,VRAMWrite.y+VRAMWrite.Height);
      }

     if(dwActFixes&0x8000)
      {
       if((xrUploadArea.x1-xrUploadArea.x0)>=(PSXDisplay.DisplayMode.x-32) &&
          (xrUploadArea.y1-xrUploadArea.y0)>=(PSXDisplay.DisplayMode.y-32))
        {
         UploadScreen(-1);
         updateFrontDisplay();
        }
      }
    }
  }
}

////////////////////////////////////////////////////////////////////////
// cmd: vram -> psx mem
////////////////////////////////////////////////////////////////////////

void primStoreImage(unsigned char * baseAddr)
{
 unsigned short *sgpuData = ((unsigned short *) baseAddr);

 VRAMRead.x      = sgpuData[2]&0x03ff;
 VRAMRead.y      = sgpuData[3]&iGPUHeightMask;
 VRAMRead.Width  = sgpuData[4];
 VRAMRead.Height = sgpuData[5];

 VRAMRead.ImagePtr = psxVuw + (VRAMRead.y<<10) + VRAMRead.x;
 VRAMRead.RowsRemaining = VRAMRead.Width;
 VRAMRead.ColsRemaining = VRAMRead.Height;

 iDataReadMode = DR_VRAMTRANSFER;

 STATUSREG |= GPUSTATUS_READYFORVRAM;
}

////////////////////////////////////////////////////////////////////////
// cmd: blkfill - NO primitive! Doesn't care about draw areas...
////////////////////////////////////////////////////////////////////////

void primBlkFill(unsigned char * baseAddr)
{
 unsigned int *gpuData = ((unsigned int *) baseAddr);
 short *sgpuData = ((short *) baseAddr);

 iDrawnSomething=1;

 sprtX = sgpuData[2];
 sprtY = sgpuData[3];
 sprtW = sgpuData[4] & 0x3ff;
 sprtH = sgpuData[5] & iGPUHeightMask;

 sprtW = (sprtW+15) & ~15;

 // Increase H & W if they are one short of full values, because they never can be full values
 if (sprtH == iGPUHeightMask)  sprtH=iGPUHeight;
 if (sprtW == 1023)            sprtW=1024; 
        
 // x and y of start
 ly0 = ly1 = sprtY;
 ly2 = ly3 = (sprtY+sprtH);
 lx0 = lx3 = sprtX;
 lx1 = lx2 = (sprtX+sprtW);

 offsetBlk();

 if(ClipVertexListScreen())                           
  {
   PSXDisplay_t * pd;
   if(PSXDisplay.InterlacedTest) pd=&PSXDisplay;
   else                          pd=&PreviousPSXDisplay;

   if ((lx0 <= pd->DisplayPosition.x+16) &&
       (ly0 <= pd->DisplayPosition.y+16) &&
       (lx2 >= pd->DisplayEnd.x-16) &&
       (ly2 >= pd->DisplayEnd.y-16))
    {
     GLclampf g,b,r;
     g=((GLclampf)GREEN(gpuData[0]))/255.0f;
     b=((GLclampf)BLUE(gpuData[0]))/255.0f;
     r=((GLclampf)RED(gpuData[0]))/255.0f;
     
     //glDisable(GL_SCISSOR_TEST); glError();
     glClearColor(r,g,b,1.0f); glError();
     glClear(uiBufferBits); glError();
     gl_z=0.0f;

     if(gpuData[0]!=0x02000000 &&
        (ly0>pd->DisplayPosition.y ||
         ly2<pd->DisplayEnd.y))
      {
       bDrawTextured     = FALSE;
       bDrawSmoothShaded = FALSE;
       SetRenderState((unsigned int)0x01000000);
       SetRenderMode((unsigned int)0x01000000, FALSE);
       vertex[0].c.lcol=0xff000000;
       SETCOL(vertex[0]); 
       if(ly0>pd->DisplayPosition.y)
        {
         vertex[0].x=0;vertex[0].y=0;
         vertex[1].x=pd->DisplayEnd.x-pd->DisplayPosition.x;vertex[1].y=0;
         vertex[2].x=vertex[1].x;vertex[2].y=ly0-pd->DisplayPosition.y;
         vertex[3].x=0;vertex[3].y=vertex[2].y;
         PRIMdrawQuad(&vertex[0], &vertex[1], &vertex[2], &vertex[3]);
        }
       if(ly2<pd->DisplayEnd.y)
        {
         vertex[0].x=0;vertex[0].y=(pd->DisplayEnd.y-pd->DisplayPosition.y)-(pd->DisplayEnd.y-ly2);
         vertex[1].x=pd->DisplayEnd.x-pd->DisplayPosition.x;vertex[1].y=vertex[0].y;
         vertex[2].x=vertex[1].x;vertex[2].y=pd->DisplayEnd.y;
         vertex[3].x=0;vertex[3].y=vertex[2].y;
         PRIMdrawQuad(&vertex[0], &vertex[1], &vertex[2], &vertex[3]);
        }
      }

     //glEnable(GL_SCISSOR_TEST); glError();
    }
   else
    {
     bDrawTextured     = FALSE;
     bDrawSmoothShaded = FALSE;
     SetRenderState((unsigned int)0x01000000);
     SetRenderMode((unsigned int)0x01000000, FALSE);
     vertex[0].c.lcol=gpuData[0]|0xff000000;
     SETCOL(vertex[0]); 
     //glDisable(GL_SCISSOR_TEST); glError();
     PRIMdrawQuad(&vertex[0], &vertex[1], &vertex[2], &vertex[3]);
     //glEnable(GL_SCISSOR_TEST); glError();
    }
  }

 //mmm... will clean all stuff, also if not all _should_ be cleaned...
 //if (IsInsideNextScreen(sprtX, sprtY, sprtW, sprtH))
 // try this:
 if (IsCompleteInsideNextScreen(sprtX, sprtY, sprtW, sprtH))
  {
   lClearOnSwapColor = COLOR(gpuData[0]);
   lClearOnSwap = 1;
  }

/* if(iOffscreenDrawing)
  {
   ClampToPSXScreenOffset( &sprtX, &sprtY, &sprtW, &sprtH);
   if ((sprtW == 0) || (sprtH == 0)) return;
   InvalidateTextureArea(sprtX, sprtY, sprtW-1, sprtH-1);   

   sprtW+=sprtX;
   sprtH+=sprtY;

   FillSoftwareArea(sprtX, sprtY, sprtW, sprtH, BGR24to16(gpuData[0]));
  }*/
}
  
////////////////////////////////////////////////////////////////////////
// cmd: move image vram -> vram
////////////////////////////////////////////////////////////////////////

void MoveImageWrapped(short imageX0,short imageY0,
                      short imageX1,short imageY1,
                      short imageSX,short imageSY)
{
 int i,j,imageXE,imageYE;

 if(iFrameReadType&2)
  {
   imageXE=imageX0+imageSX;
   imageYE=imageY0+imageSY;

   if(imageYE>iGPUHeight && imageXE>1024) 
    {
     CheckVRamRead(0,0,
                   (imageXE&0x3ff),
                   (imageY0&iGPUHeightMask),
                   FALSE);
    }

   if(imageXE>1024) 
    {
     CheckVRamRead(0,imageY0, 
                   (imageXE&0x3ff),
                   (imageYE>iGPUHeight)?iGPUHeight:imageYE,
                   FALSE);
    }

   if(imageYE>iGPUHeight) 
    {
     CheckVRamRead(imageX0,0, 
                   (imageXE>1024)?1024:imageXE,
                   imageYE&iGPUHeightMask,
                   FALSE);
    }

   CheckVRamRead(imageX0,imageY0, 
                 (imageXE>1024)?1024:imageXE,
                 (imageYE>iGPUHeight)?iGPUHeight:imageYE,
                 FALSE);
  }

 for(j=0;j<imageSY;j++)
  for(i=0;i<imageSX;i++)
   psxVuw [(1024*((imageY1+j)&iGPUHeightMask))+((imageX1+i)&0x3ff)]=
    psxVuw[(1024*((imageY0+j)&iGPUHeightMask))+((imageX0+i)&0x3ff)];

 if(!PSXDisplay.RGB24)
  {
   imageXE=imageX1+imageSX;
   imageYE=imageY1+imageSY;

   if(imageYE>iGPUHeight && imageXE>1024) 
    {
     InvalidateTextureArea(0,0,
                           (imageXE&0x3ff)-1,
                           (imageYE&iGPUHeightMask)-1);
    }

   if(imageXE>1024) 
    {
     InvalidateTextureArea(0,imageY1,
                           (imageXE&0x3ff)-1,
                           ((imageYE>iGPUHeight)?iGPUHeight:imageYE)-imageY1-1);
    }

   if(imageYE>iGPUHeight) 
    {
     InvalidateTextureArea(imageX1,0,
                           ((imageXE>1024)?1024:imageXE)-imageX1-1,
                           (imageYE&iGPUHeightMask)-1);
    }

   InvalidateTextureArea(imageX1,imageY1,
                         ((imageXE>1024)?1024:imageXE)-imageX1-1,
                         ((imageYE>iGPUHeight)?iGPUHeight:imageYE)-imageY1-1);
  }
}

////////////////////////////////////////////////////////////////////////

void primMoveImage(unsigned char * baseAddr)
{
 short *sgpuData = ((short *) baseAddr);
 short imageY0,imageX0,imageY1,imageX1,imageSX,imageSY,i,j;

 imageX0 = sgpuData[2]&0x03ff;
 imageY0 = sgpuData[3]&iGPUHeightMask;
 imageX1 = sgpuData[4]&0x03ff;
 imageY1 = sgpuData[5]&iGPUHeightMask;
 imageSX = sgpuData[6];
 imageSY = sgpuData[7];

 if((imageX0 == imageX1) && (imageY0 == imageY1)) return;  
 if(imageSX<=0) return;
 if(imageSY<=0) return;

 if(iGPUHeight==1024 && sgpuData[7]>1024) return;

 if((imageY0+imageSY)>iGPUHeight ||
    (imageX0+imageSX)>1024       ||
    (imageY1+imageSY)>iGPUHeight ||
    (imageX1+imageSX)>1024)
  {
   MoveImageWrapped(imageX0,imageY0,imageX1,imageY1,imageSX,imageSY);
   if((imageY0+imageSY)>iGPUHeight) imageSY=iGPUHeight-imageY0;
   if((imageX0+imageSX)>1024)       imageSX=1024-imageX0;
   if((imageY1+imageSY)>iGPUHeight) imageSY=iGPUHeight-imageY1;
   if((imageX1+imageSX)>1024)       imageSX=1024-imageX1;
  }

 if(iFrameReadType&2)
  CheckVRamRead(imageX0,imageY0, 
                imageX0+imageSX,
                imageY0+imageSY,
                FALSE);

 if(imageSX&1)
  {
   unsigned short *SRCPtr, *DSTPtr;
   unsigned short LineOffset;

   SRCPtr = psxVuw + (1024*imageY0) + imageX0;
   DSTPtr = psxVuw + (1024*imageY1) + imageX1;

   LineOffset = 1024 - imageSX;

   for(j=0;j<imageSY;j++)
    {
     for(i=0;i<imageSX;i++) *DSTPtr++ = *SRCPtr++;
     SRCPtr += LineOffset;
     DSTPtr += LineOffset;
    }
  }
 else
  {
   unsigned int *SRCPtr, *DSTPtr;
   unsigned short LineOffset;
   int dx=imageSX>>1;

   SRCPtr = (unsigned int *)(psxVuw + (1024*imageY0) + imageX0);
   DSTPtr = (unsigned int *)(psxVuw + (1024*imageY1) + imageX1);

   LineOffset = 512 - dx;

   for(j=0;j<imageSY;j++)
    {
     for(i=0;i<dx;i++) *DSTPtr++ = *SRCPtr++;
     SRCPtr += LineOffset;
     DSTPtr += LineOffset;
    }
  }

 if (!PSXDisplay.RGB24)
  {
   InvalidateTextureArea(imageX1,imageY1,imageSX-1,imageSY-1);

   if (CheckAgainstScreen(imageX1,imageY1,imageSX,imageSY)) 
    {
     if(imageX1>=PreviousPSXDisplay.DisplayPosition.x &&
        imageX1<PreviousPSXDisplay.DisplayEnd.x &&
        imageY1>=PreviousPSXDisplay.DisplayPosition.y &&
        imageY1<PreviousPSXDisplay.DisplayEnd.y)
      {
       imageX1 += imageSX;
       imageY1 += imageSY;

       if(imageX1>=PreviousPSXDisplay.DisplayPosition.x &&
          imageX1<=PreviousPSXDisplay.DisplayEnd.x &&
          imageY1>=PreviousPSXDisplay.DisplayPosition.y &&
          imageY1<=PreviousPSXDisplay.DisplayEnd.y)
        {
         if(!(
               imageX0>=PSXDisplay.DisplayPosition.x &&
               imageX0<PSXDisplay.DisplayEnd.x &&
               imageY0>=PSXDisplay.DisplayPosition.y &&
               imageY0<PSXDisplay.DisplayEnd.y 
              ))
          {
           if(bRenderFrontBuffer) 
            {
             updateFrontDisplay();
            }
 
           UploadScreen(FALSE);
          }
         else bFakeFrontBuffer=TRUE;
        }
      }

     bNeedUploadTest=TRUE;
    }
   else
   if(iOffscreenDrawing)
    {
     if (CheckAgainstFrontScreen(imageX1,imageY1,imageSX,imageSY)) 
      {
       if(!PSXDisplay.InterlacedTest &&
//          !bFullVRam &&
          ((
            imageX0>=PreviousPSXDisplay.DisplayPosition.x &&
            imageX0<PreviousPSXDisplay.DisplayEnd.x &&
            imageY0>=PreviousPSXDisplay.DisplayPosition.y &&
            imageY0<PreviousPSXDisplay.DisplayEnd.y
           ) ||
           (
            imageX0>=PSXDisplay.DisplayPosition.x &&
            imageX0<PSXDisplay.DisplayEnd.x &&
            imageY0>=PSXDisplay.DisplayPosition.y &&
            imageY0<PSXDisplay.DisplayEnd.y
           )))
        return;

       bNeedUploadTest=TRUE;

       if(!bNeedUploadAfter)
        {
         bNeedUploadAfter = TRUE;
         xrUploadArea.x0=imageX0;
         xrUploadArea.x1=imageX0+imageSX;
         xrUploadArea.y0=imageY0;
         xrUploadArea.y1=imageY0+imageSY;
        }
       else
        {
         xrUploadArea.x0=min(xrUploadArea.x0,imageX0);
         xrUploadArea.x1=max(xrUploadArea.x1,imageX0+imageSX);
         xrUploadArea.y0=min(xrUploadArea.y0,imageY0);
         xrUploadArea.y1=max(xrUploadArea.y1,imageY0+imageSY);
        }
      }
    }
  }
}


////////////////////////////////////////////////////////////////////////
// cmd: draw free-size Tile 
////////////////////////////////////////////////////////////////////////

void primTileS(unsigned char * baseAddr)
{
 unsigned int *gpuData = ((unsigned int*)baseAddr);
 short *sgpuData = ((short *) baseAddr);

 sprtX = sgpuData[2];
 sprtY = sgpuData[3];
 sprtW = sgpuData[4] & 0x3ff;
 sprtH = sgpuData[5] & iGPUHeightMask;

 // x and y of start

 lx0 = sprtX;
 ly0 = sprtY;

 offsetST();

 if((dwActFixes&1) &&                                  // FF7 special game gix (battle cursor)
    sprtX==0 && sprtY==0 && sprtW==24 && sprtH==16) 
  return;

 bDrawTextured = FALSE;
 bDrawSmoothShaded = FALSE;

 SetRenderState(gpuData[0]);

/* if(iOffscreenDrawing)
  {
   if(IsPrimCompleteInsideNextScreen(lx0,ly0,lx2,ly2) ||
      (ly0==-6 && ly2==10))                            // OH MY GOD... I DIDN'T WANT TO DO IT... BUT I'VE FOUND NO OTHER WAY... HACK FOR GRADIUS SHOOTER :(
    {
     lClearOnSwapColor = COLOR(gpuData[0]);
     lClearOnSwap = 1;
    }

   offsetPSX4();
   if(bDrawOffscreen4())
    {
     if(!(iTileCheat && sprtH==32 && gpuData[0]==0x60ffffff)) // special cheat for certain ZiNc games
      {
       InvalidateTextureAreaEx();   
       FillSoftwareAreaTrans(lx0,ly0,lx2,ly2,
                             BGR24to16(gpuData[0]));  
      }
    }
  }*/

 SetRenderMode(gpuData[0], FALSE);
 SetZMask4NT();

 if(bIgnoreNextTile) {bIgnoreNextTile=FALSE;return;}

 vertex[0].c.lcol=gpuData[0];
 vertex[0].c.col[3]=ubGloColAlpha;
 SETCOL(vertex[0]); 
 
 PRIMdrawQuad(&vertex[0], &vertex[1], &vertex[2], &vertex[3]);

 iDrawnSomething=1;
}

////////////////////////////////////////////////////////////////////////
// cmd: draw 1 dot Tile (point)
////////////////////////////////////////////////////////////////////////

void primTile1(unsigned char * baseAddr)
{
 unsigned int *gpuData = ((unsigned int*)baseAddr);
 short *sgpuData = ((short *) baseAddr);

 sprtX = sgpuData[2];
 sprtY = sgpuData[3];
 sprtW = 1;
 sprtH = 1;

 lx0 = sprtX;
 ly0 = sprtY;

 offsetST();

 bDrawTextured = FALSE;
 bDrawSmoothShaded = FALSE;

 SetRenderState(gpuData[0]);

/* if(iOffscreenDrawing)
  {
   offsetPSX4();

   if(bDrawOffscreen4())
    {
     InvalidateTextureAreaEx();   
     FillSoftwareAreaTrans(lx0,ly0,lx2,ly2,
                           BGR24to16(gpuData[0]));          
    }
  }
*/
 SetRenderMode(gpuData[0], FALSE);
 SetZMask4NT();

 vertex[0].c.lcol=gpuData[0];vertex[0].c.col[3]=ubGloColAlpha;
 SETCOL(vertex[0]); 

 PRIMdrawQuad(&vertex[0], &vertex[1], &vertex[2], &vertex[3]);

 iDrawnSomething=1;
}

////////////////////////////////////////////////////////////////////////
// cmd: draw 8 dot Tile (small rect)
////////////////////////////////////////////////////////////////////////

void primTile8(unsigned char * baseAddr)
{
 unsigned int *gpuData = ((unsigned int*)baseAddr);
 short *sgpuData = ((short *) baseAddr);

 sprtX = sgpuData[2];
 sprtY = sgpuData[3];
 sprtW = 8;
 sprtH = 8;

 lx0 = sprtX;
 ly0 = sprtY;

 offsetST();

 bDrawTextured = FALSE;
 bDrawSmoothShaded = FALSE;
 SetRenderState(gpuData[0]);

/* if(iOffscreenDrawing)
  {
   offsetPSX4();

   if(bDrawOffscreen4())
    {
     InvalidateTextureAreaEx();   
     FillSoftwareAreaTrans(lx0,ly0,lx2,ly2,
                           BGR24to16(gpuData[0]));    
    }
  }
*/
 SetRenderMode(gpuData[0], FALSE);
 SetZMask4NT();

 vertex[0].c.lcol=gpuData[0];
 vertex[0].c.col[3]=ubGloColAlpha;
 SETCOL(vertex[0]); 

 PRIMdrawQuad(&vertex[0], &vertex[1], &vertex[2], &vertex[3]);

 iDrawnSomething=1;
}

////////////////////////////////////////////////////////////////////////
// cmd: draw 16 dot Tile (medium rect)
////////////////////////////////////////////////////////////////////////

void primTile16(unsigned char * baseAddr)
{
 unsigned int *gpuData = ((unsigned int*)baseAddr);
 short *sgpuData = ((short *) baseAddr);

 sprtX = sgpuData[2];
 sprtY = sgpuData[3];
 sprtW = 16;
 sprtH = 16;
 // x and y of start
 lx0 = sprtX;
 ly0 = sprtY;

 offsetST();

 bDrawTextured = FALSE;
 bDrawSmoothShaded = FALSE;
 SetRenderState(gpuData[0]);

/* if(iOffscreenDrawing)
  {
   offsetPSX4();

   if(bDrawOffscreen4())
    {
     InvalidateTextureAreaEx();   
     FillSoftwareAreaTrans(lx0,ly0,lx2,ly2,
                           BGR24to16(gpuData[0]));    
    }
  }
*/
 SetRenderMode(gpuData[0], FALSE);
 SetZMask4NT();

 vertex[0].c.lcol=gpuData[0];
 vertex[0].c.col[3]=ubGloColAlpha;
 SETCOL(vertex[0]); 

 PRIMdrawQuad(&vertex[0], &vertex[1], &vertex[2], &vertex[3]);

 iDrawnSomething=1;
}

////////////////////////////////////////////////////////////////////////
// helper: filter effect by multipass rendering
////////////////////////////////////////////////////////////////////////

/*void DrawMultiBlur(void)
{
 int lABR,lDST;float fx,fy;

 lABR=GlobalTextABR;
 lDST=DrawSemiTrans;

 fx=(float)PSXDisplay.DisplayMode.x/(float)(iResX); 
 fy=(float)PSXDisplay.DisplayMode.y/(float)(iResY);

 vertex[0].x+=fx;vertex[1].x+=fx;
 vertex[2].x+=fx;vertex[3].x+=fx;

 GlobalTextABR=0;
 DrawSemiTrans=1;
 SetSemiTrans();

 PRIMdrawTexturedQuad(&vertex[0], &vertex[1], &vertex[2], &vertex[3]);

 vertex[0].y+=fy;vertex[1].y+=fy;
 vertex[2].y+=fy;vertex[3].y+=fy;
 PRIMdrawTexturedQuad(&vertex[0], &vertex[1], &vertex[2], &vertex[3]);

 if(bDrawMultiPass) {obm1=obm2=GL_SRC_ALPHA;}

 GlobalTextABR=lABR;
 DrawSemiTrans=lDST;
}
*/
////////////////////////////////////////////////////////////////////////

#define   POFF 0.375f

void DrawMultiFilterSprite(void)
{
 int lABR,lDST;

 if(bUseMultiPass || DrawSemiTrans || ubOpaqueDraw) 
  {
   PRIMdrawTexturedQuad(&vertex[0], &vertex[1], &vertex[2], &vertex[3]);
   return;
  }

 lABR=GlobalTextABR;
 lDST=DrawSemiTrans;
 vertex[0].c.col[3]=ubGloAlpha/2;                      // -> set color with
 SETCOL(vertex[0]);                                    //    texture alpha
 PRIMdrawTexturedQuad(&vertex[0], &vertex[1], &vertex[2], &vertex[3]);
 vertex[0].x+=POFF;vertex[1].x+=POFF;
 vertex[2].x+=POFF;vertex[3].x+=POFF;
 vertex[0].y+=POFF;vertex[1].y+=POFF;
 vertex[2].y+=POFF;vertex[3].y+=POFF;
 GlobalTextABR=0;
 DrawSemiTrans=1;
 SetSemiTrans();
 PRIMdrawTexturedQuad(&vertex[0], &vertex[1], &vertex[2], &vertex[3]);
 GlobalTextABR=lABR;
 DrawSemiTrans=lDST;
}

////////////////////////////////////////////////////////////////////////
// cmd: small sprite (textured rect)
////////////////////////////////////////////////////////////////////////

void primSprt8(unsigned char * baseAddr)
{
 unsigned int *gpuData = ((unsigned int *) baseAddr);
 short *sgpuData = ((short *) baseAddr);
 short s;

 iSpriteTex=1;

 sprtX = sgpuData[2];
 sprtY = sgpuData[3];
 sprtW = 8;
 sprtH = 8;

 lx0 = sprtX;
 ly0 = sprtY;

 offsetST();

 // do texture stuff
 gl_ux[0]=gl_ux[3]=baseAddr[8];//gpuData[2]&0xff;

 if(usMirror & 0x1000) 
  {
   s=gl_ux[0];
   s-=sprtW-1;
   if(s<0) {s=0;}
   gl_ux[0]=gl_ux[3]=s;
  }

 sSprite_ux2=s=gl_ux[0]+sprtW; 
 if(s)     s--;
 if(s>255) s=255;
 gl_ux[1]=gl_ux[2]=s;
 // Y coords
 gl_vy[0]=gl_vy[1]=baseAddr[9];//(gpuData[2]>>8)&0xff;

 if(usMirror & 0x2000) 
  {
   s=gl_vy[0];
   s-=sprtH-1;
   if(s<0) {s=0;}
   gl_vy[0]=gl_vy[1]=s;
  }

 sSprite_vy2=s=gl_vy[0]+sprtH; 
 if(s)     s--;
 if(s>255) s=255;
 gl_vy[2]=gl_vy[3]=s;

 ulClutID=(gpuData[2]>>16);

 bDrawTextured = TRUE;
 bDrawSmoothShaded = FALSE;
 SetRenderState(gpuData[0]);

/* if(iOffscreenDrawing)      
  {
   offsetPSX4();

   if(bDrawOffscreen4())
    {
     InvalidateTextureAreaEx();   
     SetRenderColor(gpuData[0]);
     lx0-=PSXDisplay.DrawOffset.x;
     ly0-=PSXDisplay.DrawOffset.y;

     if(bUsingTWin) DrawSoftwareSpriteTWin(baseAddr,8,8);
     else
     if(usMirror)   DrawSoftwareSpriteMirror(baseAddr,8,8);
     else
     DrawSoftwareSprite(baseAddr,8,8,baseAddr[8],baseAddr[9]);
    }
  }
*/
 SetRenderMode(gpuData[0], TRUE);
 SetZMask4SP();

 sSprite_ux2=gl_ux[0]+sprtW;
 sSprite_vy2=gl_vy[0]+sprtH;

 assignTextureSprite();

 if(iFilterType>4) 
  DrawMultiFilterSprite();
 else
  PRIMdrawTexturedQuad(&vertex[0], &vertex[1], &vertex[2], &vertex[3]);

 if(bDrawMultiPass)
  {
   SetSemiTransMulti(1);
   PRIMdrawTexturedQuad(&vertex[0], &vertex[1], &vertex[2], &vertex[3]);
  }

 if(ubOpaqueDraw)
  {
   SetZMask4O();
   if(bUseMultiPass) SetOpaqueColor(gpuData[0]);
   DEFOPAQUEON

/*   if(bSmallAlpha && iFilterType<=2)
    {
     glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
     glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
     PRIMdrawTexturedQuad(&vertex[0], &vertex[1], &vertex[2], &vertex[3]);
     glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
     glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
     SetZMask4O();
    }
*/
   PRIMdrawTexturedQuad(&vertex[0], &vertex[1], &vertex[2], &vertex[3]);
   DEFOPAQUEOFF
  }

 iSpriteTex=0;
 iDrawnSomething=1;
}

////////////////////////////////////////////////////////////////////////
// cmd: medium sprite (textured rect)
////////////////////////////////////////////////////////////////////////

void primSprt16(unsigned char * baseAddr)
{
 unsigned int *gpuData = ((unsigned int *) baseAddr);
 short *sgpuData = ((short *) baseAddr);
 short s;

 iSpriteTex=1;

 sprtX = sgpuData[2];
 sprtY = sgpuData[3];
 sprtW = 16;
 sprtH = 16;

 lx0 = sprtX;
 ly0 = sprtY;

 offsetST();

 // do texture stuff
 gl_ux[0]=gl_ux[3]=baseAddr[8];//gpuData[2]&0xff;

 if(usMirror & 0x1000) 
  {
   s=gl_ux[0];
   s-=sprtW-1;
   if(s<0) {s=0;}
   gl_ux[0]=gl_ux[3]=s;
  }

 sSprite_ux2=s=gl_ux[0]+sprtW; 
 if(s)     s--;
 if(s>255) s=255;
 gl_ux[1]=gl_ux[2]=s; 
 // Y coords
 gl_vy[0]=gl_vy[1]=baseAddr[9];//(gpuData[2]>>8)&0xff;

 if(usMirror & 0x2000) 
  {
   s=gl_vy[0];
   s-=sprtH-1;
   if(s<0) {s=0;}
   gl_vy[0]=gl_vy[1]=s;
  }

 sSprite_vy2=s=gl_vy[0]+sprtH; 
 if(s)     s--;
 if(s>255) s=255;
 gl_vy[2]=gl_vy[3]=s;

 ulClutID=(gpuData[2]>>16);

 bDrawTextured = TRUE;
 bDrawSmoothShaded = FALSE;
 SetRenderState(gpuData[0]);

/* if(iOffscreenDrawing)  
  {
   offsetPSX4();

   if(bDrawOffscreen4())
    {
     InvalidateTextureAreaEx();   
     SetRenderColor(gpuData[0]);
     lx0-=PSXDisplay.DrawOffset.x;
     ly0-=PSXDisplay.DrawOffset.y;
     if(bUsingTWin) DrawSoftwareSpriteTWin(baseAddr,16,16);
     else
     if(usMirror)   DrawSoftwareSpriteMirror(baseAddr,16,16);
     else
     DrawSoftwareSprite(baseAddr,16,16,baseAddr[8],baseAddr[9]);
    }
  }
*/
 SetRenderMode(gpuData[0], TRUE);
 SetZMask4SP();

 sSprite_ux2=gl_ux[0]+sprtW;
 sSprite_vy2=gl_vy[0]+sprtH;

 assignTextureSprite();

 if(iFilterType>4) 
  DrawMultiFilterSprite();
 else
  PRIMdrawTexturedQuad(&vertex[0], &vertex[1], &vertex[2], &vertex[3]);

 if(bDrawMultiPass)
  {
   SetSemiTransMulti(1);
   PRIMdrawTexturedQuad(&vertex[0], &vertex[1], &vertex[2], &vertex[3]);
  }

 if(ubOpaqueDraw)
  {
   SetZMask4O();
   if(bUseMultiPass) SetOpaqueColor(gpuData[0]);
   DEFOPAQUEON

/*   if(bSmallAlpha && iFilterType<=2)
    {
     glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
     glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
     PRIMdrawTexturedQuad(&vertex[0], &vertex[1], &vertex[2], &vertex[3]);
     glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
     glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
     SetZMask4O();
    }
*/
   PRIMdrawTexturedQuad(&vertex[0], &vertex[1], &vertex[2], &vertex[3]);
   DEFOPAQUEOFF
  }

 iSpriteTex=0;
 iDrawnSomething=1;
}

////////////////////////////////////////////////////////////////////////
// cmd: free-size sprite (textured rect)
////////////////////////////////////////////////////////////////////////
 
void primSprtSRest(unsigned char * baseAddr,unsigned short type)
{
 unsigned int *gpuData = ((unsigned int *) baseAddr);
 short *sgpuData = ((short *) baseAddr);
 short s;unsigned short sTypeRest=0;

 sprtX = sgpuData[2];
 sprtY = sgpuData[3];
 sprtW = sgpuData[6] & 0x3ff;
 sprtH = sgpuData[7] & 0x1ff;


 // do texture stuff
 switch(type)
  {
   case 1:
    gl_vy[0]=gl_vy[1]=baseAddr[9];
    s=256-baseAddr[8];
    sprtW-=s;
    sprtX+=s;
    gl_ux[0]=gl_ux[3]=0;
    break;
   case 2:
    gl_ux[0]=gl_ux[3]=baseAddr[8];
    s=256-baseAddr[9];
    sprtH-=s;
    sprtY+=s;
    gl_vy[0]=gl_vy[1]=0;
    break;
   case 3:
    s=256-baseAddr[8];
    sprtW-=s;
    sprtX+=s;
    gl_ux[0]=gl_ux[3]=0;
    s=256-baseAddr[9];
    sprtH-=s;
    sprtY+=s;
    gl_vy[0]=gl_vy[1]=0;
    break;

   case 4:
    gl_vy[0]=gl_vy[1]=baseAddr[9];
    s=512-baseAddr[8];
    sprtW-=s;
    sprtX+=s;
    gl_ux[0]=gl_ux[3]=0;
    break;
   case 5:
    gl_ux[0]=gl_ux[3]=baseAddr[8];
    s=512-baseAddr[9];
    sprtH-=s;
    sprtY+=s;
    gl_vy[0]=gl_vy[1]=0;
    break;
   case 6:
    s=512-baseAddr[8];
    sprtW-=s;
    sprtX+=s;
    gl_ux[0]=gl_ux[3]=0;
    s=512-baseAddr[9];
    sprtH-=s;
    sprtY+=s;
    gl_vy[0]=gl_vy[1]=0;
    break;

  }

 if(usMirror & 0x1000) 
  {
   s=gl_ux[0];
   s-=sprtW-1;if(s<0) s=0;
   gl_ux[0]=gl_ux[3]=s;
  }
 if(usMirror & 0x2000) 
  {
   s=gl_vy[0];
   s-=sprtH-1;if(s<0) {s=0;}
   gl_vy[0]=gl_vy[1]=s;
  }

 sSprite_ux2=s=gl_ux[0]+sprtW; 
 if(s>255) s=255;
 gl_ux[1]=gl_ux[2]=s;
 sSprite_vy2=s=gl_vy[0]+sprtH; 
 if(s>255) s=255;
 gl_vy[2]=gl_vy[3]=s;

 if(!bUsingTWin)
  {
   if(sSprite_ux2>256) 
    {sprtW=256-gl_ux[0];sSprite_ux2=256;sTypeRest+=1;}
   if(sSprite_vy2>256) 
    {sprtH=256-gl_vy[0];sSprite_vy2=256;sTypeRest+=2;}
  }
 
 lx0 = sprtX;
 ly0 = sprtY;

 offsetST();

 ulClutID=(gpuData[2]>>16);

 bDrawTextured = TRUE;
 bDrawSmoothShaded = FALSE;
 SetRenderState(gpuData[0]);

/* if(iOffscreenDrawing)
  {
   offsetPSX4();

   if(bDrawOffscreen4())
    {
     InvalidateTextureAreaEx();   
     SetRenderColor(gpuData[0]);
     lx0-=PSXDisplay.DrawOffset.x;
     ly0-=PSXDisplay.DrawOffset.y;
     if(bUsingTWin) DrawSoftwareSpriteTWin(baseAddr,sprtW,sprtH);
     else
     if(usMirror)   DrawSoftwareSpriteMirror(baseAddr,sprtW,sprtH);
     else
     DrawSoftwareSprite(baseAddr,sprtW,sprtH,baseAddr[8],baseAddr[9]);
    }
  }
*/
 SetRenderMode(gpuData[0], TRUE);
 SetZMask4SP();

 sSprite_ux2=gl_ux[0]+sprtW;
 sSprite_vy2=gl_vy[0]+sprtH;

 assignTextureSprite();

 if(iFilterType>4) 
  DrawMultiFilterSprite();
 else
  PRIMdrawTexturedQuad(&vertex[0], &vertex[1], &vertex[2], &vertex[3]);
 
 if(bDrawMultiPass)
  {
   SetSemiTransMulti(1);
   PRIMdrawTexturedQuad(&vertex[0], &vertex[1], &vertex[2], &vertex[3]);
  }

 if(ubOpaqueDraw)
  {
   SetZMask4O();
   if(bUseMultiPass) SetOpaqueColor(gpuData[0]);
   DEFOPAQUEON

/*   if(bSmallAlpha && iFilterType<=2)
    {
     glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
     glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
     PRIMdrawTexturedQuad(&vertex[0], &vertex[1], &vertex[2], &vertex[3]);
     glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
     glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
     SetZMask4O();
    }
*/
   PRIMdrawTexturedQuad(&vertex[0], &vertex[1], &vertex[2], &vertex[3]);
   DEFOPAQUEOFF
  }

 if(sTypeRest && type<4) 
  {
   if(sTypeRest&1  && type==1) primSprtSRest(baseAddr,4);
   if(sTypeRest&2  && type==2) primSprtSRest(baseAddr,5);
   if(sTypeRest==3 && type==3) primSprtSRest(baseAddr,6);
  }
}

void primSprtS(unsigned char * baseAddr)
{
 unsigned int *gpuData = ((unsigned int *) baseAddr);
 short *sgpuData = ((short *) baseAddr);

 short s;unsigned short sTypeRest=0;

 sprtX = sgpuData[2];
 sprtY = sgpuData[3];
 sprtW = sgpuData[6] & 0x3ff;
 sprtH = sgpuData[7] & 0x1ff;

 if(!sprtH) return;
 if(!sprtW) return;

 iSpriteTex=1;

 // do texture stuff
 gl_ux[0]=gl_ux[3]=baseAddr[8];//gpuData[2]&0xff;
 gl_vy[0]=gl_vy[1]=baseAddr[9];//(gpuData[2]>>8)&0xff;

 if(usMirror & 0x1000) 
  {
   s=gl_ux[0];
   s-=sprtW-1;
   if(s<0) {s=0;}
   gl_ux[0]=gl_ux[3]=s;
  }
 if(usMirror & 0x2000) 
  {
   s=gl_vy[0];
   s-=sprtH-1;
   if(s<0) {s=0;}
   gl_vy[0]=gl_vy[1]=s;
  }

 sSprite_ux2=s=gl_ux[0]+sprtW; 
 if(s)     s--;
 if(s>255) s=255;
 gl_ux[1]=gl_ux[2]=s;
 sSprite_vy2=s=gl_vy[0]+sprtH; 
 if(s)     s--;
 if(s>255) s=255;
 gl_vy[2]=gl_vy[3]=s;

 if(!bUsingTWin)
  {
   if(sSprite_ux2>256) 
    {sprtW=256-gl_ux[0];sSprite_ux2=256;sTypeRest+=1;}
   if(sSprite_vy2>256) 
    {sprtH=256-gl_vy[0];sSprite_vy2=256;sTypeRest+=2;}
  }

 lx0 = sprtX;
 ly0 = sprtY;

 offsetST();

 ulClutID=(gpuData[2]>>16);

 bDrawTextured = TRUE;
 bDrawSmoothShaded = FALSE;
 SetRenderState(gpuData[0]);

/* if(iOffscreenDrawing)
  {
   offsetPSX4();

   if(bDrawOffscreen4())
    {
     InvalidateTextureAreaEx();   
     SetRenderColor(gpuData[0]);
     lx0-=PSXDisplay.DrawOffset.x;
     ly0-=PSXDisplay.DrawOffset.y;
     if(bUsingTWin) DrawSoftwareSpriteTWin(baseAddr,sprtW,sprtH);
     else
     if(usMirror)   DrawSoftwareSpriteMirror(baseAddr,sprtW,sprtH);
     else
     DrawSoftwareSprite(baseAddr,sprtW,sprtH,baseAddr[8],baseAddr[9]);
    }
  }
*/
 SetRenderMode(gpuData[0], TRUE);
 SetZMask4SP();

 if((dwActFixes&1) && gTexFrameName && gTexName==gTexFrameName) 
  {iSpriteTex=0;return;}

 sSprite_ux2=gl_ux[0]+sprtW;
 sSprite_vy2=gl_vy[0]+sprtH;

 assignTextureSprite();

 if(iFilterType>4) 
  DrawMultiFilterSprite();
 else
  PRIMdrawTexturedQuad(&vertex[0], &vertex[1], &vertex[2], &vertex[3]);
 
 if(bDrawMultiPass)
  {
   SetSemiTransMulti(1);
   PRIMdrawTexturedQuad(&vertex[0], &vertex[1], &vertex[2], &vertex[3]);
  }

 if(ubOpaqueDraw)
  {
   SetZMask4O();
   if(bUseMultiPass) SetOpaqueColor(gpuData[0]);
   DEFOPAQUEON

/*   if(bSmallAlpha && iFilterType<=2)
    {
     glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
     glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
     PRIMdrawTexturedQuad(&vertex[0], &vertex[1], &vertex[2], &vertex[3]);
     glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
     glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
     SetZMask4O();
    }
*/
   PRIMdrawTexturedQuad(&vertex[0], &vertex[1], &vertex[2], &vertex[3]);
   DEFOPAQUEOFF
  }

 if(sTypeRest) 
  {
   if(sTypeRest&1)  primSprtSRest(baseAddr,1);
   if(sTypeRest&2)  primSprtSRest(baseAddr,2);
   if(sTypeRest==3) primSprtSRest(baseAddr,3);
  }

 iSpriteTex=0;
 iDrawnSomething=1;
}

////////////////////////////////////////////////////////////////////////
// cmd: flat shaded Poly4
////////////////////////////////////////////////////////////////////////

void primPolyF4(unsigned char *baseAddr)
{
 unsigned int *gpuData = ((unsigned int *) baseAddr);
 short *sgpuData = ((short *) baseAddr);

 lx0 = sgpuData[2];
 ly0 = sgpuData[3];
 lx1 = sgpuData[4];
 ly1 = sgpuData[5];
 lx2 = sgpuData[6];
 ly2 = sgpuData[7];
 lx3 = sgpuData[8];
 ly3 = sgpuData[9];

 if(offset4()) return;

 bDrawTextured = FALSE;
 bDrawSmoothShaded = FALSE;
 SetRenderState(gpuData[0]);

/* if(iOffscreenDrawing)
  {
   offsetPSX4();
   if(bDrawOffscreen4())
    {
     InvalidateTextureAreaEx();   
     drawPoly4F(gpuData[0]);
    }
  }
*/
 SetRenderMode(gpuData[0], FALSE);
 SetZMask4NT();

 vertex[0].c.lcol=gpuData[0];vertex[0].c.col[3]=ubGloColAlpha;
 SETCOL(vertex[0]); 

 PRIMdrawTri2(&vertex[0], &vertex[1], &vertex[2],&vertex[3]);

 iDrawnSomething=1;
}

////////////////////////////////////////////////////////////////////////
// cmd: smooth shaded Poly4
////////////////////////////////////////////////////////////////////////

void primPolyG4(unsigned char * baseAddr);

BOOL bDrawOffscreenFrontFF9G4(void)
{
 if(lx0< PSXDisplay.DisplayPosition.x) return FALSE;   // must be complete in front
 if(lx0> PSXDisplay.DisplayEnd.x)      return FALSE;
 if(ly0< PSXDisplay.DisplayPosition.y) return FALSE;
 if(ly0> PSXDisplay.DisplayEnd.y)      return FALSE;
 if(lx1< PSXDisplay.DisplayPosition.x) return FALSE;
 if(lx1> PSXDisplay.DisplayEnd.x)      return FALSE;
 if(ly1< PSXDisplay.DisplayPosition.y) return FALSE;
 if(ly1> PSXDisplay.DisplayEnd.y)      return FALSE;
 if(lx2< PSXDisplay.DisplayPosition.x) return FALSE;
 if(lx2> PSXDisplay.DisplayEnd.x)      return FALSE;
 if(ly2< PSXDisplay.DisplayPosition.y) return FALSE;
 if(ly2> PSXDisplay.DisplayEnd.y)      return FALSE;
 if(lx3< PSXDisplay.DisplayPosition.x) return FALSE;
 if(lx3> PSXDisplay.DisplayEnd.x)      return FALSE;
 if(ly3< PSXDisplay.DisplayPosition.y) return FALSE;
 if(ly3> PSXDisplay.DisplayEnd.y)      return FALSE;
 return TRUE;
}

BOOL bCheckFF9G4(unsigned char * baseAddr)
{
 static unsigned char pFF9G4Cache[32];
 static int iFF9Fix=0;

 if(baseAddr)
  {
   if(iFF9Fix==0)
    {
     if(bDrawOffscreenFrontFF9G4())
      {
       short *sgpuData = ((short *) pFF9G4Cache);
       iFF9Fix=2;
       memcpy(pFF9G4Cache,baseAddr,32);

       if(sgpuData[2]==142)
        {
         sgpuData[2] +=65;
         sgpuData[10]+=65;
        }
       return TRUE;
      }
     else iFF9Fix=1;
    }
   return FALSE;
  }

 if(iFF9Fix==2)
  {
   int labr=GlobalTextABR;
   GlobalTextABR=1;
   primPolyG4(pFF9G4Cache);
   GlobalTextABR=labr;
  }
 iFF9Fix=0;

 return FALSE;
}

////////////////////////////////////////////////////////////////////////

void primPolyG4(unsigned char * baseAddr)
{
 unsigned int *gpuData = (unsigned int *)baseAddr;
 short *sgpuData = ((short *) baseAddr);

 lx0 = sgpuData[2];
 ly0 = sgpuData[3];
 lx1 = sgpuData[6];
 ly1 = sgpuData[7];
 lx2 = sgpuData[10];
 ly2 = sgpuData[11];
 lx3 = sgpuData[14];
 ly3 = sgpuData[15];

 if(offset4()) return;

 bDrawTextured = FALSE;
 bDrawSmoothShaded = TRUE;
 SetRenderState(gpuData[0]);

/* if(iOffscreenDrawing)
  {
   offsetPSX4();

   if((dwActFixes&512) && bCheckFF9G4(baseAddr)) return;

   if(bDrawOffscreen4())
    {
     InvalidateTextureAreaEx();   
     drawPoly4G(gpuData[0], gpuData[2], gpuData[4], gpuData[6]);
    }     
  }
*/
 SetRenderMode(gpuData[0], FALSE);
 SetZMask4NT();

 vertex[0].c.lcol=gpuData[0];
 vertex[1].c.lcol=gpuData[2];
 vertex[2].c.lcol=gpuData[4];
 vertex[3].c.lcol=gpuData[6];

 vertex[0].c.col[3]=vertex[1].c.col[3]=vertex[2].c.col[3]=vertex[3].c.col[3]=ubGloAlpha;


 PRIMdrawGouraudTri2Color(&vertex[0],&vertex[1], &vertex[2], &vertex[3]);

 iDrawnSomething=1;
}

////////////////////////////////////////////////////////////////////////
// cmd: flat shaded Texture3
////////////////////////////////////////////////////////////////////////

BOOL DoLineCheck(unsigned int * gpuData)
{
 BOOL bQuad=FALSE;short dx,dy;

 if(lx0==lx1)
  {
   dx=lx0-lx2;if(dx<0) dx=-dx;

   if(ly1==ly2) 
    {
     dy=ly1-ly0;if(dy<0) dy=-dy;
     if(dx<=1)
      {
       vertex[3]=vertex[2];
       vertex[2]=vertex[0];
       vertex[2].x=vertex[3].x;
      }
     else
     if(dy<=1)
      {
       vertex[3]=vertex[2];
       vertex[2].y=vertex[0].y;
      }
     else return FALSE;

     bQuad=TRUE;
    }
   else
   if(ly0==ly2) 
    {
     dy=ly0-ly1;if(dy<0) dy=-dy;
     if(dx<=1)
      {
       vertex[3]=vertex[1];
       vertex[3].x=vertex[2].x;
      }
     else
     if(dy<=1)
      {
       vertex[3]=vertex[2];
       vertex[3].y=vertex[1].y;
      }
     else return FALSE;

     bQuad=TRUE;
    }
  }

 if(lx0==lx2)
  {
   dx=lx0-lx1;if(dx<0) dx=-dx;

   if(ly2==ly1) 
    {
     dy=ly2-ly0;if(dy<0) dy=-dy;
     if(dx<=1)
      {
       vertex[3]=vertex[1];
       vertex[1]=vertex[0];
       vertex[1].x=vertex[3].x;
      }
     else
     if(dy<=1)
      {
       vertex[3]=vertex[1];
       vertex[1].y=vertex[0].y;
      }
     else return FALSE;

     bQuad=TRUE;
    }
   else
   if(ly0==ly1)
    {
     dy=ly2-ly0;if(dy<0) dy=-dy;
     if(dx<=1)
      {
       vertex[3]=vertex[2];
       vertex[3].x=vertex[1].x;
      }
     else
     if(dy<=1)
      {
       vertex[3]=vertex[1];
       vertex[3].y=vertex[2].y;
      }
     else return FALSE;

     bQuad=TRUE;
    }
  }

 if(lx1==lx2)
  {
   dx=lx1-lx0;if(dx<0) dx=-dx;

   if(ly1==ly0)
    {
     dy=ly1-ly2;if(dy<0) dy=-dy;

     if(dx<=1)
      {
       vertex[3]=vertex[2];
       vertex[2].x=vertex[0].x;
      }
     else
     if(dy<=1)
      {
       vertex[3]=vertex[2];
       vertex[2]=vertex[0];
       vertex[2].y=vertex[3].y;
      }
     else return FALSE;

     bQuad=TRUE;
    }
   else
   if(ly2==ly0)
    {
     dy=ly2-ly1;if(dy<0) dy=-dy;

     if(dx<=1)
      {
       vertex[3]=vertex[1];
       vertex[1].x=vertex[0].x;
      }
     else
     if(dy<=1)
      {
       vertex[3]=vertex[1];
       vertex[1]=vertex[0];
       vertex[1].y=vertex[3].y;
      }
     else return FALSE;

     bQuad=TRUE;
    }
  }

 if(!bQuad) return FALSE;

 PRIMdrawTexturedQuad(&vertex[0], &vertex[1], &vertex[3], &vertex[2]);

 if(bDrawMultiPass)
  {
   SetSemiTransMulti(1);
   PRIMdrawTexturedQuad(&vertex[0], &vertex[1], &vertex[3], &vertex[2]);
  }

 if(ubOpaqueDraw)
  {
   SetZMask4O();
   if(bUseMultiPass) SetOpaqueColor(gpuData[0]);
   DEFOPAQUEON
   PRIMdrawTexturedQuad(&vertex[0], &vertex[1], &vertex[3], &vertex[2]);
   DEFOPAQUEOFF
  }

 iDrawnSomething=1;

 return TRUE;
}

////////////////////////////////////////////////////////////////////////

void primPolyFT3(unsigned char * baseAddr)
{
 unsigned int *gpuData = ((unsigned int *) baseAddr);
 short *sgpuData = ((short *) baseAddr);

 lx0 = sgpuData[2];
 ly0 = sgpuData[3];
 lx1 = sgpuData[6];
 ly1 = sgpuData[7];
 lx2 = sgpuData[10];
 ly2 = sgpuData[11];

 if(offset3()) return;
    
 // do texture UV coordinates stuff
 gl_ux[0]=gl_ux[3]=baseAddr[8];//gpuData[2]&0xff;
 gl_vy[0]=gl_vy[3]=baseAddr[9];//(gpuData[2]>>8)&0xff;
 gl_ux[1]=baseAddr[16];//gpuData[4]&0xff;
 gl_vy[1]=baseAddr[17];//(gpuData[4]>>8)&0xff;
 gl_ux[2]=baseAddr[24];//gpuData[6]&0xff;
 gl_vy[2]=baseAddr[25];//(gpuData[6]>>8)&0xff;

 UpdateGlobalTP((unsigned short)(gpuData[4]>>16));
 ulClutID=gpuData[2]>>16;

 bDrawTextured = TRUE;
 bDrawSmoothShaded = FALSE;
 SetRenderState(gpuData[0]);

/* if(iOffscreenDrawing)
  {
   offsetPSX3();
   if(bDrawOffscreen3())
    {
     InvalidateTextureAreaEx();   
     SetRenderColor(gpuData[0]);
     drawPoly3FT(baseAddr);
    }
  }
*/
 SetRenderMode(gpuData[0], TRUE);
 SetZMask3();

 assignTexture3();

 if(!(dwActFixes&0x10))
  {
   if(DoLineCheck(gpuData)) return;
  }

 PRIMdrawTexturedTri(&vertex[0], &vertex[1], &vertex[2]);

 if(bDrawMultiPass)
  {
   SetSemiTransMulti(1);
   PRIMdrawTexturedTri(&vertex[0], &vertex[1], &vertex[2]);
  }

 if(ubOpaqueDraw)
  {
   SetZMask3O();
   if(bUseMultiPass) SetOpaqueColor(gpuData[0]);
   DEFOPAQUEON
   PRIMdrawTexturedTri(&vertex[0], &vertex[1], &vertex[2]);
   DEFOPAQUEOFF
  }

 iDrawnSomething=1;
}

////////////////////////////////////////////////////////////////////////
// cmd: flat shaded Texture4
////////////////////////////////////////////////////////////////////////

#define ST_FAC             255.99f

void RectTexAlign(void)
{
 int UFlipped = FALSE;
 int VFlipped = FALSE;

 if(gTexName==gTexFrameName) return;

 if(ly0==ly1)
  {
   if(!((lx1==lx3 && ly3==ly2 && lx2==lx0) ||
        (lx1==lx2 && ly2==ly3 && lx3==lx0)))
    return;

   if(ly0<ly2) 
    {
     if (vertex[0].tow > vertex[2].tow)
      VFlipped = 1;
    }
   else
    {
     if (vertex[0].tow < vertex[2].tow)
      VFlipped = 2;
    }
  }
 else
 if(ly0==ly2)
  {
   if(!((lx2==lx3 && ly3==ly1 && lx1==lx0) ||
        (lx2==lx1 && ly1==ly3 && lx3==lx0)))
    return;

   if(ly0<ly1) 
    {
     if (vertex[0].tow > vertex[1].tow)
      VFlipped = 3;
    }
   else
    {
     if (vertex[0].tow < vertex[1].tow)
      VFlipped = 4;
    }
  }
 else
 if(ly0==ly3)
  {
   if(!((lx3==lx2 && ly2==ly1 && lx1==lx0) ||
        (lx3==lx1 && ly1==ly2 && lx2==lx0)))
    return;

   if(ly0<ly1) 
    {
     if (vertex[0].tow > vertex[1].tow)
      VFlipped = 5;
    }
   else
    {
     if (vertex[0].tow < vertex[1].tow)
      VFlipped = 6;
    }
  }
 else return;

 if(lx0==lx1)
  {
   if(lx0<lx2) 
    {
     if (vertex[0].sow > vertex[2].sow)
      UFlipped = 1;
    }
   else
    {
     if (vertex[0].sow < vertex[2].sow)
      UFlipped = 2;
    }
  }
 else
 if(lx0==lx2)
  {
   if(lx0<lx1) 
    {
     if (vertex[0].sow > vertex[1].sow)
      UFlipped = 3;
    }
   else
    {
     if (vertex[0].sow < vertex[1].sow)
      UFlipped = 4;
    }
  }
 else
 if(lx0==lx3)
  {
   if(lx0<lx1) 
    {
     if (vertex[0].sow > vertex[1].sow)
      UFlipped = 5;
    }
   else
    {
     if (vertex[0].sow < vertex[1].sow)
      UFlipped = 6;
    }
  }

 if (UFlipped)
  {
#ifdef OWNSCALE
   if(bUsingTWin)
    {
     switch(UFlipped)
      {
       case 1:
        vertex[2].sow+=0.95f/TWin.UScaleFactor; 
        vertex[3].sow+=0.95f/TWin.UScaleFactor;
        break;
       case 2:
        vertex[0].sow+=0.95f/TWin.UScaleFactor; 
        vertex[1].sow+=0.95f/TWin.UScaleFactor;
        break;
       case 3:
        vertex[1].sow+=0.95f/TWin.UScaleFactor; 
        vertex[3].sow+=0.95f/TWin.UScaleFactor;
        break;
       case 4:
        vertex[0].sow+=0.95f/TWin.UScaleFactor; 
        vertex[2].sow+=0.95f/TWin.UScaleFactor;
        break;
       case 5:
        vertex[1].sow+=0.95f/TWin.UScaleFactor; 
        vertex[2].sow+=0.95f/TWin.UScaleFactor;
        break;
       case 6:
        vertex[0].sow+=0.95f/TWin.UScaleFactor; 
        vertex[3].sow+=0.95f/TWin.UScaleFactor;
        break;
      }
    }
   else
    {
     switch(UFlipped)
      {
       case 1:
        vertex[2].sow+=1.0f/ST_FAC; 
        vertex[3].sow+=1.0f/ST_FAC;
        break;
       case 2:
        vertex[0].sow+=1.0f/ST_FAC; 
        vertex[1].sow+=1.0f/ST_FAC;
        break;
       case 3:
        vertex[1].sow+=1.0f/ST_FAC; 
        vertex[3].sow+=1.0f/ST_FAC;
        break;
       case 4:
        vertex[0].sow+=1.0f/ST_FAC; 
        vertex[2].sow+=1.0f/ST_FAC;
        break;
       case 5:
        vertex[1].sow+=1.0f/ST_FAC; 
        vertex[2].sow+=1.0f/ST_FAC;
        break;
       case 6:
        vertex[0].sow+=1.0f/ST_FAC; 
        vertex[3].sow+=1.0f/ST_FAC;
        break;
      }
    }
#else
   if(bUsingTWin)
    {
     switch(UFlipped)
      {
       case 1:
        vertex[2].sow+=1.0f/TWin.UScaleFactor; 
        vertex[3].sow+=1.0f/TWin.UScaleFactor;
        break;
       case 2:
        vertex[0].sow+=1.0f/TWin.UScaleFactor; 
        vertex[1].sow+=1.0f/TWin.UScaleFactor;
        break;
       case 3:
        vertex[1].sow+=1.0f/TWin.UScaleFactor; 
        vertex[3].sow+=1.0f/TWin.UScaleFactor;
        break;
       case 4:
        vertex[0].sow+=1.0f/TWin.UScaleFactor; 
        vertex[2].sow+=1.0f/TWin.UScaleFactor;
        break;
       case 5:
        vertex[1].sow+=1.0f/TWin.UScaleFactor; 
        vertex[2].sow+=1.0f/TWin.UScaleFactor;
        break;
       case 6:
        vertex[0].sow+=1.0f/TWin.UScaleFactor; 
        vertex[3].sow+=1.0f/TWin.UScaleFactor;
        break;
      }
    }
   else
    {
     switch(UFlipped)
      {
       case 1:
        vertex[2].sow+=1.0f; 
        vertex[3].sow+=1.0f;
        break;
       case 2:
        vertex[0].sow+=1.0f; 
        vertex[1].sow+=1.0f;
        break;
       case 3:
        vertex[1].sow+=1.0f; 
        vertex[3].sow+=1.0f;
        break;
       case 4:
        vertex[0].sow+=1.0f; 
        vertex[2].sow+=1.0f;
        break;
       case 5:
        vertex[1].sow+=1.0f; 
        vertex[2].sow+=1.0f;
        break;
       case 6:
        vertex[0].sow+=1.0f; 
        vertex[3].sow+=1.0f;
        break;
      }
    }
#endif
  }

 if (VFlipped)
  {
#ifdef OWNSCALE
   if(bUsingTWin)
    {
     switch(VFlipped)
      {
       case 1:
        vertex[2].tow+=0.95f/TWin.VScaleFactor; 
        vertex[3].tow+=0.95f/TWin.VScaleFactor;
        break;
       case 2:
        vertex[0].tow+=0.95f/TWin.VScaleFactor; 
        vertex[1].tow+=0.95f/TWin.VScaleFactor;
        break;
       case 3:
        vertex[1].tow+=0.95f/TWin.VScaleFactor; 
        vertex[3].tow+=0.95f/TWin.VScaleFactor;
        break;
       case 4:
        vertex[0].tow+=0.95f/TWin.VScaleFactor; 
        vertex[2].tow+=0.95f/TWin.VScaleFactor;
        break;
       case 5:
        vertex[1].tow+=0.95f/TWin.VScaleFactor; 
        vertex[2].tow+=0.95f/TWin.VScaleFactor;
        break;
       case 6:
        vertex[0].tow+=0.95f/TWin.VScaleFactor; 
        vertex[3].tow+=0.95f/TWin.VScaleFactor;
        break;
      }
    }
   else
    {
     switch(VFlipped)
      {
       case 1:
        vertex[2].tow+=1.0f/ST_FAC; 
        vertex[3].tow+=1.0f/ST_FAC;
        break;
       case 2:
        vertex[0].tow+=1.0f/ST_FAC; 
        vertex[1].tow+=1.0f/ST_FAC;
        break;
       case 3:
        vertex[1].tow+=1.0f/ST_FAC;
        vertex[3].tow+=1.0f/ST_FAC;
        break;
       case 4:
        vertex[0].tow+=1.0f/ST_FAC; 
        vertex[2].tow+=1.0f/ST_FAC;
        break;
       case 5:
        vertex[1].tow+=1.0f/ST_FAC;
        vertex[2].tow+=1.0f/ST_FAC;
        break;
       case 6:
        vertex[0].tow+=1.0f/ST_FAC;
        vertex[3].tow+=1.0f/ST_FAC;
        break;
      }
    }
#else
   if(bUsingTWin)
    {
     switch(VFlipped)
      {
       case 1:
        vertex[2].tow+=1.0f/TWin.VScaleFactor; 
        vertex[3].tow+=1.0f/TWin.VScaleFactor;
        break;
       case 2:
        vertex[0].tow+=1.0f/TWin.VScaleFactor; 
        vertex[1].tow+=1.0f/TWin.VScaleFactor;
        break;
       case 3:
        vertex[1].tow+=1.0f/TWin.VScaleFactor; 
        vertex[3].tow+=1.0f/TWin.VScaleFactor;
        break;
       case 4:
        vertex[0].tow+=1.0f/TWin.VScaleFactor; 
        vertex[2].tow+=1.0f/TWin.VScaleFactor;
        break;
       case 5:
        vertex[1].tow+=1.0f/TWin.VScaleFactor; 
        vertex[2].tow+=1.0f/TWin.VScaleFactor;
        break;
       case 6:
        vertex[0].tow+=1.0f/TWin.VScaleFactor; 
        vertex[3].tow+=1.0f/TWin.VScaleFactor;
        break;
      }
    }
   else
    {
     switch(VFlipped)
      {
       case 1:
        vertex[2].tow+=1.0f; 
        vertex[3].tow+=1.0f;
        break;
       case 2:
        vertex[0].tow+=1.0f; 
        vertex[1].tow+=1.0f;
        break;
       case 3:
        vertex[1].tow+=1.0f; 
        vertex[3].tow+=1.0f;
        break;
       case 4:
        vertex[0].tow+=1.0f; 
        vertex[2].tow+=1.0f;
        break;
       case 5:
        vertex[1].tow+=1.0f; 
        vertex[2].tow+=1.0f;
        break;
       case 6:
        vertex[0].tow+=1.0f; 
        vertex[3].tow+=1.0f;
        break;
      }
    }
#endif
  }

}

void primPolyFT4(unsigned char * baseAddr)
{
 unsigned int *gpuData = ((unsigned int *) baseAddr);
 short *sgpuData = ((short *) baseAddr);

 lx0 = sgpuData[2];
 ly0 = sgpuData[3];
 lx1 = sgpuData[6];
 ly1 = sgpuData[7];
 lx2 = sgpuData[10];
 ly2 = sgpuData[11];
 lx3 = sgpuData[14];
 ly3 = sgpuData[15];

 if(offset4()) return;

 gl_vy[0]=baseAddr[9];//((gpuData[2]>>8)&0xff);
 gl_vy[1]=baseAddr[17];//((gpuData[4]>>8)&0xff);
 gl_vy[2]=baseAddr[25];//((gpuData[6]>>8)&0xff);
 gl_vy[3]=baseAddr[33];//((gpuData[8]>>8)&0xff);
 
 gl_ux[0]=baseAddr[8];//(gpuData[2]&0xff);
 gl_ux[1]=baseAddr[16];//(gpuData[4]&0xff);
 gl_ux[2]=baseAddr[24];//(gpuData[6]&0xff);
 gl_ux[3]=baseAddr[32];//(gpuData[8]&0xff);

 UpdateGlobalTP((unsigned short)(gpuData[4]>>16));
 ulClutID=(gpuData[2]>>16);

 bDrawTextured = TRUE;
 bDrawSmoothShaded = FALSE;
 SetRenderState(gpuData[0]);

/* if(iOffscreenDrawing)
  {
   offsetPSX4();
   if(bDrawOffscreen4())
    {
     InvalidateTextureAreaEx();   
     SetRenderColor(gpuData[0]);
     drawPoly4FT(baseAddr);
    }
  }
*/
 SetRenderMode(gpuData[0], TRUE);

 SetZMask4();

 assignTexture4();

 RectTexAlign();

 PRIMdrawTexturedQuad(&vertex[0], &vertex[1], &vertex[3], &vertex[2]);

 if(bDrawMultiPass)
  {
   SetSemiTransMulti(1);
   PRIMdrawTexturedQuad(&vertex[0], &vertex[1], &vertex[3], &vertex[2]);
  }

 if(ubOpaqueDraw)
  {
   SetZMask4O();
   if(bUseMultiPass) SetOpaqueColor(gpuData[0]);
   DEFOPAQUEON

/*   if(bSmallAlpha && iFilterType<=2)
    {
     glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
     glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
     PRIMdrawTexturedQuad(&vertex[0], &vertex[1], &vertex[3], &vertex[2]);
     glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
     glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
     SetZMask4O();
    }
*/
   
   PRIMdrawTexturedQuad(&vertex[0], &vertex[1], &vertex[3], &vertex[2]);
   DEFOPAQUEOFF
  }

 iDrawnSomething=1;
}

////////////////////////////////////////////////////////////////////////
// cmd: smooth shaded Texture3
////////////////////////////////////////////////////////////////////////

void primPolyGT3(unsigned char *baseAddr)
{    
 unsigned int *gpuData = ((unsigned int *) baseAddr);
 short *sgpuData = ((short *) baseAddr);

 lx0 = sgpuData[2];
 ly0 = sgpuData[3];
 lx1 = sgpuData[8];
 ly1 = sgpuData[9];
 lx2 = sgpuData[14];
 ly2 = sgpuData[15];

 if(offset3()) return;

 // do texture stuff
 gl_ux[0]=gl_ux[3]=baseAddr[8];//gpuData[2]&0xff;
 gl_vy[0]=gl_vy[3]=baseAddr[9];//(gpuData[2]>>8)&0xff;
 gl_ux[1]=baseAddr[20];//gpuData[5]&0xff;
 gl_vy[1]=baseAddr[21];//(gpuData[5]>>8)&0xff;
 gl_ux[2]=baseAddr[32];//gpuData[8]&0xff;
 gl_vy[2]=baseAddr[33];//(gpuData[8]>>8)&0xff;

 UpdateGlobalTP((unsigned short)(gpuData[5]>>16));
 ulClutID=(gpuData[2]>>16);
           
 bDrawTextured = TRUE;
 bDrawSmoothShaded = TRUE;
 SetRenderState(gpuData[0]);

/* if(iOffscreenDrawing)
  {
   offsetPSX3();
   if(bDrawOffscreen3())
    {
     InvalidateTextureAreaEx();   
     drawPoly3GT(baseAddr);
    }
  }
*/
 SetRenderMode(gpuData[0], FALSE);
 SetZMask3();

 assignTexture3();

 if(bDrawNonShaded)
  {
   //if(!bUseMultiPass) vertex[0].lcol=DoubleBGR2RGB(gpuData[0]); else vertex[0].lcol=gpuData[0];
   // eat this...
/*   if(bGLBlend) vertex[0].c.lcol=0x7f7f7f;
   else         */vertex[0].c.lcol=0xffffff;
   vertex[0].c.col[3]=ubGloAlpha;
   SETCOL(vertex[0]); 

   PRIMdrawTexturedTri(&vertex[0], &vertex[1], &vertex[2]);

   if(ubOpaqueDraw)
    {
     SetZMask3O();
     DEFOPAQUEON
     PRIMdrawTexturedTri(&vertex[0], &vertex[1], &vertex[2]);
     DEFOPAQUEOFF
    }
   return; 
  }

/* if(!bUseMultiPass  && !bGLBlend)
  {
  */ vertex[0].c.lcol=DoubleBGR2RGB(gpuData[0]); 
   vertex[1].c.lcol=DoubleBGR2RGB(gpuData[3]); 
   vertex[2].c.lcol=DoubleBGR2RGB(gpuData[6]);
  /*}
 else
  {
   vertex[0].c.lcol=gpuData[0];
   vertex[1].c.lcol=gpuData[3];
   vertex[2].c.lcol=gpuData[6];
  }*/
 vertex[0].c.col[3]=vertex[1].c.col[3]=vertex[2].c.col[3]=ubGloAlpha;

 PRIMdrawTexGouraudTriColor(&vertex[0], &vertex[1], &vertex[2]);

 if(bDrawMultiPass)
  {
   SetSemiTransMulti(1);
   PRIMdrawTexGouraudTriColor(&vertex[0], &vertex[1], &vertex[2]);
  }

 if(ubOpaqueDraw)
  {
   SetZMask3O();
   if(bUseMultiPass) 
    {
     vertex[0].c.lcol=DoubleBGR2RGB(gpuData[0]);
     vertex[1].c.lcol=DoubleBGR2RGB(gpuData[3]);
     vertex[2].c.lcol=DoubleBGR2RGB(gpuData[6]);
     vertex[0].c.col[3]=vertex[1].c.col[3]=vertex[2].c.col[3]=ubGloAlpha;
    }
   DEFOPAQUEON
   PRIMdrawTexGouraudTriColor(&vertex[0], &vertex[1], &vertex[2]);
   DEFOPAQUEOFF
  }

 iDrawnSomething=1;
}

////////////////////////////////////////////////////////////////////////
// cmd: smooth shaded Poly3
////////////////////////////////////////////////////////////////////////

void primPolyG3(unsigned char *baseAddr)
{    
 unsigned int *gpuData = ((unsigned int *) baseAddr);
 short *sgpuData = ((short *) baseAddr);

 lx0 = sgpuData[2];
 ly0 = sgpuData[3];
 lx1 = sgpuData[6];
 ly1 = sgpuData[7];
 lx2 = sgpuData[10];
 ly2 = sgpuData[11];

 if(offset3()) return;

 bDrawTextured = FALSE;
 bDrawSmoothShaded = TRUE;
 SetRenderState(gpuData[0]);

/* if(iOffscreenDrawing) 
  {
   offsetPSX3();
   if(bDrawOffscreen3())
    {
     InvalidateTextureAreaEx();   
     drawPoly3G(gpuData[0], gpuData[2], gpuData[4]);
    }
  }
*/
 SetRenderMode(gpuData[0], FALSE);
 SetZMask3NT();

 vertex[0].c.lcol=gpuData[0];
 vertex[1].c.lcol=gpuData[2];
 vertex[2].c.lcol=gpuData[4];
 vertex[0].c.col[3]=vertex[1].c.col[3]=vertex[2].c.col[3]=ubGloColAlpha; 

 PRIMdrawGouraudTriColor(&vertex[0], &vertex[1], &vertex[2]);

 iDrawnSomething=1;
}

////////////////////////////////////////////////////////////////////////
// cmd: smooth shaded Texture4
////////////////////////////////////////////////////////////////////////

void primPolyGT4(unsigned char *baseAddr)
{ 
 unsigned int *gpuData = ((unsigned int *) baseAddr);
 short *sgpuData = ((short *) baseAddr);

 lx0 = sgpuData[2];
 ly0 = sgpuData[3];
 lx1 = sgpuData[8];
 ly1 = sgpuData[9];
 lx2 = sgpuData[14];
 ly2 = sgpuData[15];
 lx3 = sgpuData[20];
 ly3 = sgpuData[21];

 if(offset4()) return;

 // do texture stuff
 gl_ux[0]=baseAddr[8];//gpuData[2]&0xff;
 gl_vy[0]=baseAddr[9];//(gpuData[2]>>8)&0xff;
 gl_ux[1]=baseAddr[20];//gpuData[5]&0xff;
 gl_vy[1]=baseAddr[21];//(gpuData[5]>>8)&0xff;
 gl_ux[2]=baseAddr[32];//gpuData[8]&0xff;
 gl_vy[2]=baseAddr[33];//(gpuData[8]>>8)&0xff;
 gl_ux[3]=baseAddr[44];//gpuData[11]&0xff;
 gl_vy[3]=baseAddr[45];//(gpuData[11]>>8)&0xff;

 UpdateGlobalTP((unsigned short)(gpuData[5]>>16));
 ulClutID=(gpuData[2]>>16);

 bDrawTextured     = TRUE;
 bDrawSmoothShaded = TRUE;
 SetRenderState(gpuData[0]);

/* if(iOffscreenDrawing)
  {
   offsetPSX4();
   if(bDrawOffscreen4())
    {
     InvalidateTextureAreaEx();   
     drawPoly4GT(baseAddr);
    }     
  }
*/
 SetRenderMode(gpuData[0], FALSE);
 SetZMask4();

 assignTexture4();

 RectTexAlign();

 if(bDrawNonShaded)
  {
   //if(!bUseMultiPass) vertex[0].lcol=DoubleBGR2RGB(gpuData[0]); else vertex[0].lcol=gpuData[0];
/*   if(bGLBlend) vertex[0].c.lcol=0x7f7f7f;
   else          */vertex[0].c.lcol=0xffffff;
   vertex[0].c.col[3]=ubGloAlpha;
   SETCOL(vertex[0]); 

   PRIMdrawTexturedQuad(&vertex[0], &vertex[1], &vertex[3], &vertex[2]);
  
   if(ubOpaqueDraw)
    {
     SetZMask4O();
     ubGloAlpha=ubGloColAlpha=0xff;   
     DEFOPAQUEON
     PRIMdrawTexturedQuad(&vertex[0], &vertex[1], &vertex[3], &vertex[2]);
     DEFOPAQUEOFF
    }
   return;
  }

// if(!bUseMultiPass  && !bGLBlend) 
  {
   vertex[0].c.lcol=DoubleBGR2RGB(gpuData[0]);
   vertex[1].c.lcol=DoubleBGR2RGB(gpuData[3]);
   vertex[2].c.lcol=DoubleBGR2RGB(gpuData[6]);
   vertex[3].c.lcol=DoubleBGR2RGB(gpuData[9]);
  }
 /*else
  {
   vertex[0].c.lcol=gpuData[0];
   vertex[1].c.lcol=gpuData[3];
   vertex[2].c.lcol=gpuData[6];
   vertex[3].c.lcol=gpuData[9];
  }*/

 vertex[0].c.col[3]=vertex[1].c.col[3]=vertex[2].c.col[3]=vertex[3].c.col[3]=ubGloAlpha; 

 PRIMdrawTexGouraudTriColorQuad(&vertex[0], &vertex[1], &vertex[3],&vertex[2]);
 
 if(bDrawMultiPass)
  {
   SetSemiTransMulti(1);
   PRIMdrawTexGouraudTriColorQuad(&vertex[0], &vertex[1], &vertex[3],&vertex[2]);
  }

 if(ubOpaqueDraw)
  {
   SetZMask4O();
   if(bUseMultiPass) 
    {
     vertex[0].c.lcol=DoubleBGR2RGB(gpuData[0]);
     vertex[1].c.lcol=DoubleBGR2RGB(gpuData[3]);
     vertex[2].c.lcol=DoubleBGR2RGB(gpuData[6]);
     vertex[3].c.lcol=DoubleBGR2RGB(gpuData[9]);
     vertex[0].c.col[3]=vertex[1].c.col[3]=vertex[2].c.col[3]=vertex[3].c.col[3]=ubGloAlpha; 
    }
   ubGloAlpha=ubGloColAlpha=0xff;   
   DEFOPAQUEON
   PRIMdrawTexGouraudTriColorQuad(&vertex[0], &vertex[1], &vertex[3],&vertex[2]);
   DEFOPAQUEOFF
  }

 iDrawnSomething=1;
}

////////////////////////////////////////////////////////////////////////
// cmd: smooth shaded Poly3
////////////////////////////////////////////////////////////////////////

void primPolyF3(unsigned char *baseAddr)
{    
 unsigned int *gpuData = ((unsigned int *) baseAddr);
 short *sgpuData = ((short *) baseAddr);

 lx0 = sgpuData[2];
 ly0 = sgpuData[3];
 lx1 = sgpuData[4];
 ly1 = sgpuData[5];
 lx2 = sgpuData[6];
 ly2 = sgpuData[7];

 if(offset3()) return;

 bDrawTextured     = FALSE;
 bDrawSmoothShaded = FALSE;
 SetRenderState(gpuData[0]);

/* if(iOffscreenDrawing)
  {
   offsetPSX3();
   if(bDrawOffscreen3())
    {
     InvalidateTextureAreaEx();   
     drawPoly3F(gpuData[0]);
    }
  }
*/
 SetRenderMode(gpuData[0], FALSE);
 SetZMask3NT();

 vertex[0].c.lcol=gpuData[0];
 vertex[0].c.col[3]=ubGloColAlpha;
 SETCOL(vertex[0]); 

 PRIMdrawTri(&vertex[0], &vertex[1], &vertex[2]);

 iDrawnSomething=1;
}

////////////////////////////////////////////////////////////////////////
// cmd: skipping shaded polylines
////////////////////////////////////////////////////////////////////////

void primLineGSkip(unsigned char *baseAddr)
{    
 unsigned int *gpuData = ((unsigned int *) baseAddr);
 short *sgpuData = ((short *) baseAddr);
 int iMax=255;
 int i=2;

 lx1 = sgpuData[2];
 ly1 = sgpuData[3];

 while(!(((gpuData[i] & 0xF000F000) == 0x50005000) && i>=4))
  {
   i++;

   ly1 = (short)((gpuData[i]>>16) & 0xffff);
   lx1 = (short)(gpuData[i] & 0xffff);

   i++;if(i>iMax) break;
  }
}

////////////////////////////////////////////////////////////////////////
// cmd: shaded polylines
////////////////////////////////////////////////////////////////////////

void primLineGEx(unsigned char *baseAddr)
{    
 unsigned int *gpuData = ((unsigned int *) baseAddr);
 int iMax=255;
 short cx0,cx1,cy0,cy1;int i;BOOL bDraw=TRUE;

 bDrawTextured = FALSE;
 bDrawSmoothShaded = TRUE;
 SetRenderState(gpuData[0]);
 SetRenderMode(gpuData[0], FALSE);
 SetZMask4NT();

 vertex[0].c.lcol=vertex[3].c.lcol=gpuData[0];
 vertex[0].c.col[3]=vertex[3].c.col[3]=ubGloColAlpha; 
 ly1 = (short)((gpuData[1]>>16) & 0xffff);
 lx1 = (short)(gpuData[1] & 0xffff);

 i=2;

 //while((gpuData[i]>>24)!=0x55)
 //while((gpuData[i]&0x50000000)!=0x50000000) 
 // currently best way to check for poly line end:
 while(!(((gpuData[i] & 0xF000F000) == 0x50005000) && i>=4))
  {
   ly0 = ly1;lx0=lx1;
   vertex[1].c.lcol=vertex[2].c.lcol=vertex[0].c.lcol;
   vertex[0].c.lcol=vertex[3].c.lcol=gpuData[i];
   vertex[0].c.col[3]=vertex[3].c.col[3]=ubGloColAlpha; 

   i++;

   ly1 = (short)((gpuData[i]>>16) & 0xffff);
   lx1 = (short)(gpuData[i] & 0xffff);

   if(offsetline()) bDraw=FALSE; else bDraw=TRUE;
  
   if (bDraw && ((lx0 != lx1) || (ly0 != ly1)))
    {
/*     if(iOffscreenDrawing)
      {
       cx0=lx0;cx1=lx1;cy0=ly0;cy1=ly1;
       offsetPSXLine();
       if(bDrawOffscreen4())
        {
         InvalidateTextureAreaEx();   
         drawPoly4G(gpuData[i-3],gpuData[i-1],gpuData[i-3],gpuData[i-1]);
        }
       lx0=cx0;lx1=cx1;ly0=cy0;ly1=cy1;
      }*/

     PRIMdrawGouraudLine(&vertex[0], &vertex[1], &vertex[2], &vertex[3]);
    }
   i++;  

   if(i>iMax) break;
  }

 iDrawnSomething=1;
}

////////////////////////////////////////////////////////////////////////
// cmd: shaded polyline2
////////////////////////////////////////////////////////////////////////

void primLineG2(unsigned char *baseAddr)
{    
 unsigned int *gpuData = ((unsigned int *) baseAddr);
 short *sgpuData = ((short *) baseAddr);

 lx0 = sgpuData[2];
 ly0 = sgpuData[3];
 lx1 = sgpuData[6];
 ly1 = sgpuData[7];

 vertex[0].c.lcol=vertex[3].c.lcol=gpuData[0];
 vertex[1].c.lcol=vertex[2].c.lcol=gpuData[2];
 vertex[0].c.col[3]=vertex[1].c.col[3]=vertex[2].c.col[3]=vertex[3].c.col[3]=ubGloColAlpha; 

 bDrawTextured = FALSE;
 bDrawSmoothShaded = TRUE;

 if((lx0 == lx1) && (ly0 == ly1)) return;
    
 if(offsetline()) return;
    
 SetRenderState(gpuData[0]);
 SetRenderMode(gpuData[0], FALSE);
 SetZMask4NT();

/* if(iOffscreenDrawing)
  {
   offsetPSXLine();
   if(bDrawOffscreen4())
    {
     InvalidateTextureAreaEx();   
     drawPoly4G(gpuData[0],gpuData[2],gpuData[0],gpuData[2]);
    }
  }
*/
 //if(ClipVertexList4())
 PRIMdrawGouraudLine(&vertex[0], &vertex[1], &vertex[2], &vertex[3]);

 iDrawnSomething=1;
}

////////////////////////////////////////////////////////////////////////
// cmd: skipping flat polylines
////////////////////////////////////////////////////////////////////////

void primLineFSkip(unsigned char *baseAddr)
{
 unsigned int *gpuData = ((unsigned int *) baseAddr);
 int i=2,iMax=255;

 ly1 = (short)((gpuData[1]>>16) & 0xffff);
 lx1 = (short)(gpuData[1] & 0xffff);

 while(!(((gpuData[i] & 0xF000F000) == 0x50005000) && i>=3))
  {
   ly1 = (short)((gpuData[i]>>16) & 0xffff);
   lx1 = (short)(gpuData[i] & 0xffff);
   i++;if(i>iMax) break;
  }             
}

////////////////////////////////////////////////////////////////////////
// cmd: drawing flat polylines
////////////////////////////////////////////////////////////////////////

void primLineFEx(unsigned char *baseAddr)
{
 unsigned int *gpuData = ((unsigned int *) baseAddr);
 int iMax;
 short cx0,cx1,cy0,cy1;int i;

 iMax=255;

 bDrawTextured = FALSE;
 bDrawSmoothShaded = FALSE;
 SetRenderState(gpuData[0]);
 SetRenderMode(gpuData[0], FALSE);
 SetZMask4NT();

 vertex[0].c.lcol=gpuData[0];
 vertex[0].c.col[3]=ubGloColAlpha; 

 ly1 = (short)((gpuData[1]>>16) & 0xffff);
 lx1 = (short)(gpuData[1] & 0xffff);

 i=2;

// while(!(gpuData[i]&0x40000000)) 
// while((gpuData[i]>>24)!=0x55)
// while((gpuData[i]&0x50000000)!=0x50000000) 
// currently best way to check for poly line end:
 while(!(((gpuData[i] & 0xF000F000) == 0x50005000) && i>=3))
  {
   ly0 = ly1;lx0=lx1;
   ly1 = (short)((gpuData[i]>>16) & 0xffff);
   lx1 = (short)(gpuData[i] & 0xffff);

   if(!offsetline())
    {
/*     if(iOffscreenDrawing)
      {
       cx0=lx0;cx1=lx1;cy0=ly0;cy1=ly1;
       offsetPSXLine();
       if(bDrawOffscreen4())
        {
         InvalidateTextureAreaEx();   
         drawPoly4F(gpuData[0]);
        }
       lx0=cx0;lx1=cx1;ly0=cy0;ly1=cy1;
      }*/
     PRIMdrawFlatLine(&vertex[0], &vertex[1], &vertex[2], &vertex[3]);
    }
                  
   i++;if(i>iMax) break;
  }

 iDrawnSomething=1;
}

////////////////////////////////////////////////////////////////////////
// cmd: drawing flat polyline2
////////////////////////////////////////////////////////////////////////

void primLineF2(unsigned char *baseAddr)
{
 unsigned int *gpuData = ((unsigned int *) baseAddr);
 short *sgpuData = ((short *) baseAddr);

 lx0 = sgpuData[2];
 ly0 = sgpuData[3];
 lx1 = sgpuData[4];
 ly1 = sgpuData[5];

 if(offsetline()) return;

 bDrawTextured = FALSE;
 bDrawSmoothShaded = FALSE;
 SetRenderState(gpuData[0]);
 SetRenderMode(gpuData[0], FALSE);
 SetZMask4NT();

 vertex[0].c.lcol=gpuData[0];
 vertex[0].c.col[3]=ubGloColAlpha; 

/* if(iOffscreenDrawing)
  {
   offsetPSXLine();
   if(bDrawOffscreen4())
    {
     InvalidateTextureAreaEx();   
     drawPoly4F(gpuData[0]);
    }
  }
*/
 //if(ClipVertexList4()) 
 PRIMdrawFlatLine(&vertex[0], &vertex[1], &vertex[2], &vertex[3]);

 iDrawnSomething=1;
}

////////////////////////////////////////////////////////////////////////
// cmd: well, easiest command... not implemented
////////////////////////////////////////////////////////////////////////

void primNI(unsigned char *bA)
{
}

////////////////////////////////////////////////////////////////////////
// cmd func ptr table
////////////////////////////////////////////////////////////////////////

void (*primTableJ[256])(unsigned char *) = 
{
    // 00
    primNI,primNI,primBlkFill,primNI,primNI,primNI,primNI,primNI,
    // 08
    primNI,primNI,primNI,primNI,primNI,primNI,primNI,primNI,
    // 10
    primNI,primNI,primNI,primNI,primNI,primNI,primNI,primNI,
    // 18
    primNI,primNI,primNI,primNI,primNI,primNI,primNI,primNI,
    // 20
    primPolyF3,primPolyF3,primPolyF3,primPolyF3,primPolyFT3,primPolyFT3,primPolyFT3,primPolyFT3,
    // 28
    primPolyF4,primPolyF4,primPolyF4,primPolyF4,primPolyFT4,primPolyFT4,primPolyFT4,primPolyFT4,
    // 30
    primPolyG3,primPolyG3,primPolyG3,primPolyG3,primPolyGT3,primPolyGT3,primPolyGT3,primPolyGT3,
    // 38
    primPolyG4,primPolyG4,primPolyG4,primPolyG4,primPolyGT4,primPolyGT4,primPolyGT4,primPolyGT4,
    // 40
    primLineF2,primLineF2,primLineF2,primLineF2,primNI,primNI,primNI,primNI,
    // 48
    primLineFEx,primLineFEx,primLineFEx,primLineFEx,primLineFEx,primLineFEx,primLineFEx,primLineFEx,
    // 50
    primLineG2,primLineG2,primLineG2,primLineG2,primNI,primNI,primNI,primNI,
    // 58
    primLineGEx,primLineGEx,primLineGEx,primLineGEx,primLineGEx,primLineGEx,primLineGEx,primLineGEx,
    // 60
    primTileS,primTileS,primTileS,primTileS,primSprtS,primSprtS,primSprtS,primSprtS,
    // 68
    primTile1,primTile1,primTile1,primTile1,primNI,primNI,primNI,primNI,
    // 70
    primTile8,primTile8,primTile8,primTile8,primSprt8,primSprt8,primSprt8,primSprt8,
    // 78
    primTile16,primTile16,primTile16,primTile16,primSprt16,primSprt16,primSprt16,primSprt16,
    // 80
    primMoveImage,primNI,primNI,primNI,primNI,primNI,primNI,primNI,
    // 88
    primNI,primNI,primNI,primNI,primNI,primNI,primNI,primNI,
    // 90
    primNI,primNI,primNI,primNI,primNI,primNI,primNI,primNI,
    // 98
    primNI,primNI,primNI,primNI,primNI,primNI,primNI,primNI,
    // a0
    primLoadImage,primNI,primNI,primNI,primNI,primNI,primNI,primNI,
    // a8
    primNI,primNI,primNI,primNI,primNI,primNI,primNI,primNI,
    // b0
    primNI,primNI,primNI,primNI,primNI,primNI,primNI,primNI,
    // b8
    primNI,primNI,primNI,primNI,primNI,primNI,primNI,primNI,
    // c0
    primStoreImage,primNI,primNI,primNI,primNI,primNI,primNI,primNI,
    // c8
    primNI,primNI,primNI,primNI,primNI,primNI,primNI,primNI,
    // d0
    primNI,primNI,primNI,primNI,primNI,primNI,primNI,primNI,
    // d8
    primNI,primNI,primNI,primNI,primNI,primNI,primNI,primNI,
    // e0
    primNI,cmdTexturePage,cmdTextureWindow,cmdDrawAreaStart,cmdDrawAreaEnd,cmdDrawOffset,cmdSTP,primNI,
    // e8
    primNI,primNI,primNI,primNI,primNI,primNI,primNI,primNI,
    // f0
    primNI,primNI,primNI,primNI,primNI,primNI,primNI,primNI,
    // f8
    primNI,primNI,primNI,primNI,primNI,primNI,primNI,primNI
};

////////////////////////////////////////////////////////////////////////
// cmd func ptr table for skipping
////////////////////////////////////////////////////////////////////////

void (*primTableSkip[256])(unsigned char *) = 
{
    // 00
    primNI,primNI,primBlkFill,primNI,primNI,primNI,primNI,primNI,
    // 08
    primNI,primNI,primNI,primNI,primNI,primNI,primNI,primNI,
    // 10
    primNI,primNI,primNI,primNI,primNI,primNI,primNI,primNI,
    // 18
    primNI,primNI,primNI,primNI,primNI,primNI,primNI,primNI,
    // 20
    primNI,primNI,primNI,primNI,primNI,primNI,primNI,primNI,
    // 28
    primNI,primNI,primNI,primNI,primNI,primNI,primNI,primNI,
    // 30
    primNI,primNI,primNI,primNI,primNI,primNI,primNI,primNI,
    // 38
    primNI,primNI,primNI,primNI,primNI,primNI,primNI,primNI,
    // 40
    primNI,primNI,primNI,primNI,primNI,primNI,primNI,primNI,
    // 48
    primLineFSkip,primLineFSkip,primLineFSkip,primLineFSkip,primLineFSkip,primLineFSkip,primLineFSkip,primLineFSkip,
    // 50
    primNI,primNI,primNI,primNI,primNI,primNI,primNI,primNI,
    // 58
    primLineGSkip,primLineGSkip,primLineGSkip,primLineGSkip,primLineGSkip,primLineGSkip,primLineGSkip,primLineGSkip,
    // 60
    primNI,primNI,primNI,primNI,primNI,primNI,primNI,primNI,
    // 68
    primNI,primNI,primNI,primNI,primNI,primNI,primNI,primNI,
    // 70
    primNI,primNI,primNI,primNI,primNI,primNI,primNI,primNI,
    // 78
    primNI,primNI,primNI,primNI,primNI,primNI,primNI,primNI,
    // 80
    primMoveImage,primNI,primNI,primNI,primNI,primNI,primNI,primNI,
    // 88
    primNI,primNI,primNI,primNI,primNI,primNI,primNI,primNI,
    // 90
    primNI,primNI,primNI,primNI,primNI,primNI,primNI,primNI,
    // 98
    primNI,primNI,primNI,primNI,primNI,primNI,primNI,primNI,
    // a0
    primLoadImage,primNI,primNI,primNI,primNI,primNI,primNI,primNI,
    // a8
    primNI,primNI,primNI,primNI,primNI,primNI,primNI,primNI,
    // b0
    primNI,primNI,primNI,primNI,primNI,primNI,primNI,primNI,
    // b8
    primNI,primNI,primNI,primNI,primNI,primNI,primNI,primNI,
    // c0
    primStoreImage,primNI,primNI,primNI,primNI,primNI,primNI,primNI,
    // c8
    primNI,primNI,primNI,primNI,primNI,primNI,primNI,primNI,
    // d0
    primNI,primNI,primNI,primNI,primNI,primNI,primNI,primNI,
    // d8
    primNI,primNI,primNI,primNI,primNI,primNI,primNI,primNI,
    // e0
    primNI,cmdTexturePage,cmdTextureWindow,cmdDrawAreaStart,cmdDrawAreaEnd,cmdDrawOffset,cmdSTP,primNI,
    // e8
    primNI,primNI,primNI,primNI,primNI,primNI,primNI,primNI,
    // f0
    primNI,primNI,primNI,primNI,primNI,primNI,primNI,primNI,
    // f8
    primNI,primNI,primNI,primNI,primNI,primNI,primNI,primNI
};
