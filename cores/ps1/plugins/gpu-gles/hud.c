/***************************************************************************
                           menu.c  -  description
                             -------------------
    begin                : Sun Mar 08 2009
    copyright            : (C) 1999-2009 by Pete Bernert
    web                  : www.pbernert.com   

    PCSX rearmed adjustments (c) notaz, 2012
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

#include "gpuStdafx.h"

#define _IN_MENU

#include "gpuExternals.h"

////////////////////////////////////////////////////////////////////////
// field with menu chars... like good old C64 time :)
////////////////////////////////////////////////////////////////////////

static const GLubyte texrasters[40][12]= {

// 0,0 FPS
{0x00,0x60,0x60,0x60,0x60,0x60,0x7e,0x60,0x60,0x60,0x60,0x7f},
{0x00,0x18,0x18,0x18,0x18,0x18,0x1f,0x18,0x18,0x18,0x18,0x1f},
{0x00,0x03,0x06,0x00,0x00,0x00,0xc3,0x66,0x66,0x66,0x66,0xc3},
{0x00,0xf0,0x18,0x18,0x18,0x18,0xf0,0x00,0x00,0x00,0x18,0xf0},
// 4,0 0
{0x00,0x3c,0x66,0xc3,0xe3,0xf3,0xdb,0xcf,0xc7,0xc3,0x66,0x3c},
// 5,0 1
{0x00,0x7e,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x78,0x38,0x18},
// 6,0 2
{0x00,0xff,0xc0,0xc0,0x60,0x30,0x18,0x0c,0x06,0x03,0xe7,0x7e},
// 7,0 3

{0x00,0x7e,0xe7,0x03,0x03,0x07,0x7e,0x07,0x03,0x03,0xe7,0x7e},
// 0,1 4
{0x00,0x0c,0x0c,0x0c,0x0c,0x0c,0xff,0xcc,0x6c,0x3c,0x1c,0x0c},
// 1,1 5
{0x00,0x7e,0xe7,0x03,0x03,0x07,0xfe,0xc0,0xc0,0xc0,0xc0,0xff},
// 2,1 6
{0x00,0x7e,0xe7,0xc3,0xc3,0xc7,0xfe,0xc0,0xc0,0xc0,0xe7,0x7e},
// 3,1 7
{0x00,0x30,0x30,0x30,0x30,0x18,0x0c,0x06,0x03,0x03,0x03,0xff},
// 4,1 8
{0x00,0x7e,0xe7,0xc3,0xc3,0xe7,0x7e,0xe7,0xc3,0xc3,0xe7,0x7e},
// 5,1 9
{0x00,0x7e,0xe7,0x03,0x03,0x03,0x7f,0xe7,0xc3,0xc3,0xe7,0x7e},
// 6,1 smiley
{0x00,0x3c,0x42,0x99,0xa5,0x81,0xa5,0x81,0x42,0x3c,0x00,0x00},
// 7,1 sun
{0x00,0x08,0x49,0x2a,0x1c,0x7f,0x1c,0x2a,0x49,0x08,0x00,0x00},

// 0,2 fl + empty  box
{0xff,0x81,0x81,0x81,0xff,0x00,0x87,0x84,0x84,0xf4,0x84,0xf8},
// 1,2 fs + grey box
{0xff,0xab,0xd5,0xab,0xff,0x00,0x87,0x81,0x87,0xf4,0x87,0xf8},
// 2,2 od + filled box
{0xff,0xff,0xff,0xff,0xff,0x00,0x66,0x95,0x95,0x95,0x96,0x60},
// 3,2 fi + half grey box
{0xff,0xa1,0xd1,0xa1,0xff,0x00,0x82,0x82,0x82,0xe2,0x82,0xf8},
// 4,2 di + half filled box
{0xff,0xf1,0xf1,0xf1,0xff,0x00,0xe2,0x92,0x92,0x92,0x92,0xe0},
// 5,2 am  + grey box
{0xff,0xab,0xd5,0xab,0xff,0x00,0x95,0x95,0x95,0xf7,0x95,0x60},
// 6,2 ab  + filled box
{0xff,0xff,0xff,0xff,0xff,0x00,0x97,0x95,0x96,0xf5,0x96,0x60},
// 7,2 fa
{0x00,0x00,0x00,0x00,0x00,0x00,0x85,0x85,0x87,0xf5,0x82,0xf8},

// 0,3 fb
{0xff,0x8b,0x85,0x8b,0xff,0x00,0x82,0x82,0x82,0xe2,0x87,0xf8},
// 1,3 gf
{0xff,0x8f,0x8f,0x8f,0xff,0x00,0x74,0x94,0x96,0xb4,0x87,0x70},
// 2,3 D
{0xff,0x00,0xfc,0xc6,0xc3,0xc3,0xc3,0xc3,0xc6,0xfc,0x00,0xff}, 
// 3,3 G
{0xff,0x00,0x3e,0x63,0xc3,0xc7,0xc0,0xc0,0x63,0x3e,0x00,0xff},
// 4,3
{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
// 5,3
{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
// 6,3 tex pal smiley
{0x00,0x3c,0x7e,0xe7,0xdb,0xff,0xdb,0xff,0x7e,0x3c,0x00,0x00},
// 7,3
{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},

// 0,4 subtract blending (moon)
{0x00,0x06,0x1c,0x38,0x78,0x78,0x78,0x38,0x1c,0x06,0x00,0x00},
// 1,4 blurring
{0x00,0x7e,0x93,0xa5,0x93,0xc9,0x93,0xa5,0x93,0x7e,0x00,0x00},
// 2,4 (M)
{0xff,0x00,0xc3,0xc3,0xc3,0xdb,0xff,0xe7,0xc3,0x81,0x00,0xff},
// 3,4 (A)
{0xff,0x00,0xc3,0xc3,0xff,0xc3,0xc3,0x66,0x3c,0x18,0x00,0xff},
// 4,4 blank
{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
// 5,4
{0x00,0xfe,0xc5,0x62,0x35,0x18,0x0c,0xc6,0xc6,0x7c,0x00,0x00},
// 6,4  <-
{0x00,0x00,0x00,0x00,0x00,0x10,0x30,0x7f,0xff,0x7f,0x30,0x10},
// 7,4 .
{0x00,0x38,0x38,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}
};

////////////////////////////////////////////////////////////////////////
// create lists/stuff for fonts 
// (as a matter of fact: no more display list used, just a texture)
////////////////////////////////////////////////////////////////////////

GLuint gTexFontName=0;

void MakeDisplayLists(void)                            // MAKE FONT 
{
 GLubyte TexBytes[64][64][3];                          // we use a 64x64 texture
 int x,y,i,j,n=0; GLubyte col,IB;

 glPixelStorei(GL_UNPACK_ALIGNMENT,1);
 
 memset(TexBytes,0,64*64*3);

 for(y=0;y<5;y++)                                      // create texture out of raster infos
  {
   for(x=0;x<8;x++,n++)
    {
     for(i=0;i<12;i++)
      {
       IB=texrasters[n][i];
       for(j=0;j<8;j++)
        {
         if(IB&(1<<(7-j))) col=255; else col=0;
         TexBytes[y*12+i][x*8+j][0]=col;
         TexBytes[y*12+i][x*8+j][1]=col;
         TexBytes[y*12+i][x*8+j][2]=col;
        }
      }
    }
  }

 glGenTextures(1, &gTexFontName);                      // set tex params for font texture
 glBindTexture(GL_TEXTURE_2D, gTexFontName);
 glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
 glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
 glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
 glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
 glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 64, 64, 0, GL_RGB,
              GL_UNSIGNED_BYTE,TexBytes);
 glError();
}

////////////////////////////////////////////////////////////////////////
// kill existing font stuff
////////////////////////////////////////////////////////////////////////

void KillDisplayLists(void)
{
#ifdef _WINDOWS
 if(hGFont) DeleteObject(hGFont);                      // windows: kill info font
 hGFont=NULL;
#endif

 if(gTexFontName)                                      // del font/info textures
  {glDeleteTextures(1,&gTexFontName);gTexFontName=0;}
}

////////////////////////////////////////////////////////////////////////
// display text/infos in gpu menu
////////////////////////////////////////////////////////////////////////

#define TEXCHAR_VERTEX(t0,t1,v0,v1,v2) \
 *pta++ = t0; *pta++ = t1; *pva++ = v0; *pva++ = v1; *pva++ = v2

#ifdef OWNSCALE
#define DRAWTEXCHAR \
 pta = text_tex_array, pva = text_vertex_array; \
 TEXCHAR_VERTEX(fX1/256.0f,fY2/256.0f,fXS1,fYS2,1.0f); \
 TEXCHAR_VERTEX(fX1/256.0f,fY1/256.0f,fXS1,fYS1,1.0f); \
 TEXCHAR_VERTEX(fX2/256.0f,fY1/256.0f,fXS2,fYS1,1.0f); \
 TEXCHAR_VERTEX(fX2/256.0f,fY2/256.0f,fXS2,fYS2,1.0f); \
 glDrawArrays(GL_TRIANGLE_FAN,0,4)

#else
#define DRAWTEXCHAR glTexCoord2f(fX1,fY2);glVertex3f(fXS1,fYS2,1.0f);glTexCoord2f(fX1,fY1);glVertex3f(fXS1,fYS1,1.0f);glTexCoord2f(fX2,fY1);glVertex3f(fXS2,fYS1,1.0f);glTexCoord2f(fX2,fY2);glVertex3f(fXS2,fYS2,1.0f);
#endif

static GLfloat text_tex_array[2*4];
static GLfloat text_vertex_array[3*4];

void DisplayText(const char *ltext, int right_aligned)
{
 int iX,iY,i;
 GLfloat fX1,fY1,fX2,fY2,fYS1,fYS2,fXS1,fXS2,fXS,fXSC,fYSC,fYD;
 GLfloat *pta, *pva;

 glDisable(GL_SCISSOR_TEST);                           // disable unwanted ogl states
 glDisable(GL_ALPHA_TEST);
 if(bOldSmoothShaded) {glShadeModel(GL_FLAT);bOldSmoothShaded=FALSE;}
 if(bBlendEnable)     {glDisable(GL_BLEND);bBlendEnable=FALSE;}
 if(!bTexEnabled)     {glEnable(GL_TEXTURE_2D);bTexEnabled=TRUE;}

 gTexName=gTexFontName;
 glBindTexture(GL_TEXTURE_2D,gTexFontName);            // now set font texture

 fYD=fYSC=(GLfloat)PSXDisplay.DisplayMode.y/(GLfloat)iResY; // some pre-calculations
 fYS1=(GLfloat)PSXDisplay.DisplayMode.y-1.0f*fYSC;
 fYS2=(GLfloat)PSXDisplay.DisplayMode.y-13.0f*fYSC;
 fYSC*=13.0f;
 fXS= (GLfloat)PSXDisplay.DisplayMode.x/(GLfloat)iResX;
 fXSC= 8.0f*fXS;fXS*=10.0f;
 fXS1=0.0f;
 fXS2=50.0f*fXS;                                       // 3 is one option

 vertex[0].c.lcol=0xffffffff;                          // set menu text color
 SETCOL(vertex[0]); 

 //glBegin(GL_QUADS);
 glEnableClientState(GL_VERTEX_ARRAY);
 glEnableClientState(GL_TEXTURE_COORD_ARRAY);
 glDisableClientState(GL_COLOR_ARRAY);
 glVertexPointer(3,GL_FLOAT,0,text_vertex_array);
 glTexCoordPointer(2,GL_FLOAT,0,text_tex_array);
 glError();

 if(right_aligned)
  fYSC=fXS1=(GLfloat)PSXDisplay.DisplayMode.x-strlen(ltext)*8.0f;
 else
  fYSC=fXS1=1.0f*fXS;                                  // start pos of numbers

 i=0;do                                                // paint fps numbers
  {
   iX=4;iY=4;
   if(ltext[i]>='0' && ltext[i]<='3')
    {iX=4+ltext[i]-'0';iY=0;}
   else
   if(ltext[i]>='4' && ltext[i]<='9')
    {iX=ltext[i]-'4';iY=1;}
   else
   if(ltext[i]=='.')
    {iX=7;iY=4;}
   else
   if(ltext[i]==0) break;

   fX1=(GLfloat)iX*32.0f; fX2=fX1+32.0f;
   fY1=(GLfloat)iY*48.0f; fY2=fY1+48.0f;
   fXS1+=fXS;
   fXS2=fXS1+fXSC;

   DRAWTEXCHAR;

   i++;
  }
 while(i);

 //glEnd();
 glDisableClientState(GL_VERTEX_ARRAY);
 glDisableClientState(GL_TEXTURE_COORD_ARRAY);
 CSTEXTURE = CSVERTEX = CSCOLOR = 0;

 glEnable(GL_ALPHA_TEST);                              // repair needed states
 glEnable(GL_SCISSOR_TEST);                       
 glError();
}
 
////////////////////////////////////////////////////////////////////////

void HideText(void)
{
 GLfloat fYS1,fYS2,fXS1,fXS2,fXS,fXSC,fYSC;

 glDisable(GL_SCISSOR_TEST);                           // turn off unneeded ogl states
 glDisable(GL_ALPHA_TEST);
 if(bOldSmoothShaded) {glShadeModel(GL_FLAT);bOldSmoothShaded=FALSE;}
 if(bBlendEnable)     {glDisable(GL_BLEND);bBlendEnable=FALSE;}
 if(bTexEnabled)      {glDisable(GL_TEXTURE_2D);bTexEnabled=FALSE;}

 fYSC=(GLfloat)PSXDisplay.DisplayMode.y/(GLfloat)iResY;
 fYS1=12.0f*fYSC;fYSC*=13.0f;
 fYS2=0.0f;
 fXS= (GLfloat)PSXDisplay.DisplayMode.x/(GLfloat)iResX;
 fXSC= 8.0f*fXS;fXS*=10.0f;
 fXS1=0.0f;
 fXS2=50.0f*fXS;                                       

 vertex[0].c.lcol=0xff000000;                          // black color
 SETCOL(vertex[0]); 

 //glBegin(GL_QUADS);                                  // make one quad

 {
  GLfloat vertex_array[3*4] = {
   fXS1,fYS2,0.99996f,
   fXS1,fYSC,0.99996f,
   fXS2,fYSC,0.99996f,
   fXS2,fYS2,0.99996f,
  };
  glDisableClientState(GL_TEXTURE_COORD_ARRAY);
  glEnableClientState(GL_VERTEX_ARRAY);
  glVertexPointer(3,GL_FLOAT,0,vertex_array);

  glDrawArrays(GL_TRIANGLE_STRIP,0,4);
  glDisableClientState(GL_VERTEX_ARRAY);
 }

 //glEnd();
 glEnable(GL_ALPHA_TEST);                              // enable needed ogl states
 glEnable(GL_SCISSOR_TEST);                       
}

