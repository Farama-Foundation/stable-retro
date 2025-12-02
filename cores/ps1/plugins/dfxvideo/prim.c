/***************************************************************************
                          prim.c  -  description
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

////////////////////////////////////////////////////////////////////////
// globals
////////////////////////////////////////////////////////////////////////

BOOL           bUsingTWin=FALSE;
TWin_t         TWin;
//unsigned long  clutid;                                 // global clut
unsigned short usMirror=0;                             // sprite mirror
int            iDither=0;
int32_t        drawX;
int32_t        drawY;
int32_t        drawW;
int32_t        drawH;
uint32_t       dwCfgFixes;
uint32_t       dwActFixes=0;
int            iUseFixes;
int            iUseDither=0;
BOOL           bDoVSyncUpdate=FALSE;

// USE_NASM
static inline unsigned short BGR24to16 (uint32_t BGR)
{
 return (unsigned short)(((BGR>>3)&0x1f)|((BGR&0xf80000)>>9)|((BGR&0xf800)>>6));
}

////////////////////////////////////////////////////////////////////////
// Update global TP infos
////////////////////////////////////////////////////////////////////////

static inline void UpdateGlobalTP(unsigned short gdata)
{
 GlobalTextAddrX = (gdata << 6) & 0x3c0;               // texture addr

 GlobalTextAddrY = (gdata << 4) & 0x100;

 GlobalTextTP = (gdata >> 7) & 0x3;                    // tex mode (4,8,15)

 if(GlobalTextTP==3) GlobalTextTP=2;                   // seen in Wild9 :(

 GlobalTextABR = (gdata >> 5) & 0x3;                   // blend mode

 lGPUstatusRet&=~0x000001ff;                           // Clear the necessary bits
 lGPUstatusRet|=(gdata & 0x01ff);                      // set the necessary bits

 switch(iUseDither)
 {
  case 0:
   iDither=0;
  break;
  case 1:
   if(lGPUstatusRet&0x0200) iDither=2;
   else iDither=0;
  break;
  case 2:
   iDither=2;
  break;
 }
}

////////////////////////////////////////////////////////////////////////

static inline void SetRenderMode(uint32_t DrawAttributes)
{
 DrawSemiTrans = (SEMITRANSBIT(DrawAttributes)) ? TRUE : FALSE;

 if(SHADETEXBIT(DrawAttributes)) 
  {g_m1=g_m2=g_m3=128;}
 else
  {
   if((dwActFixes&4) && ((DrawAttributes&0x00ffffff)==0))
    DrawAttributes|=0x007f7f7f;

   g_m1=(short)(DrawAttributes&0xff);
   g_m2=(short)((DrawAttributes>>8)&0xff);
   g_m3=(short)((DrawAttributes>>16)&0xff);
  }
}

////////////////////////////////////////////////////////////////////////

// oki, here are the psx gpu coord rules: poly coords are
// 11 bit signed values (-1024...1023). If the x or y distance 
// exceeds 1024, the polygon will not be drawn. 
// Since quads are treated as two triangles by the real gpu,
// this 'discard rule' applies to each of the quad's triangle 
// (so one triangle can be drawn, the other one discarded). 
// Also, y drawing is wrapped at 512 one time,
// then it will get negative (and therefore not drawn). The
// 'CheckCoord' funcs are a simple (not comlete!) approach to
// do things right, I will add a better detection soon... the 
// current approach will be easier to do in hw/accel plugins, imho

// 11 bit signed
#define SIGNSHIFT 21
#define CHKMAX_X 1024
#define CHKMAX_Y 512

static inline void AdjustCoord4(void)
{
 lx0=(short)(((int)lx0<<SIGNSHIFT)>>SIGNSHIFT);
 lx1=(short)(((int)lx1<<SIGNSHIFT)>>SIGNSHIFT);
 lx2=(short)(((int)lx2<<SIGNSHIFT)>>SIGNSHIFT);
 lx3=(short)(((int)lx3<<SIGNSHIFT)>>SIGNSHIFT);
 ly0=(short)(((int)ly0<<SIGNSHIFT)>>SIGNSHIFT);
 ly1=(short)(((int)ly1<<SIGNSHIFT)>>SIGNSHIFT);
 ly2=(short)(((int)ly2<<SIGNSHIFT)>>SIGNSHIFT);
 ly3=(short)(((int)ly3<<SIGNSHIFT)>>SIGNSHIFT);
}

static inline void AdjustCoord3(void)
{
 lx0=(short)(((int)lx0<<SIGNSHIFT)>>SIGNSHIFT);
 lx1=(short)(((int)lx1<<SIGNSHIFT)>>SIGNSHIFT);
 lx2=(short)(((int)lx2<<SIGNSHIFT)>>SIGNSHIFT);
 ly0=(short)(((int)ly0<<SIGNSHIFT)>>SIGNSHIFT);
 ly1=(short)(((int)ly1<<SIGNSHIFT)>>SIGNSHIFT);
 ly2=(short)(((int)ly2<<SIGNSHIFT)>>SIGNSHIFT);
}

static inline void AdjustCoord2(void)
{
 lx0=(short)(((int)lx0<<SIGNSHIFT)>>SIGNSHIFT);
 lx1=(short)(((int)lx1<<SIGNSHIFT)>>SIGNSHIFT);
 ly0=(short)(((int)ly0<<SIGNSHIFT)>>SIGNSHIFT);
 ly1=(short)(((int)ly1<<SIGNSHIFT)>>SIGNSHIFT);
}

static inline void AdjustCoord1(void)
{
 lx0=(short)(((int)lx0<<SIGNSHIFT)>>SIGNSHIFT);
 ly0=(short)(((int)ly0<<SIGNSHIFT)>>SIGNSHIFT);

 if(lx0<-512 && PSXDisplay.DrawOffset.x<=-512)
  lx0+=2048;

 if(ly0<-512 && PSXDisplay.DrawOffset.y<=-512)
  ly0+=2048;
}

////////////////////////////////////////////////////////////////////////
// special checks... nascar, syphon filter 2, mgs
////////////////////////////////////////////////////////////////////////

// xenogears FT4: not removed correctly right now... the tri 0,1,2
// should get removed, the tri 1,2,3 should stay... pfff

// x -466 1023 180 1023
// y   20 -228 222 -100

// 0 __1
//  \ / \    .
//   2___3

static inline BOOL CheckCoord4(void)
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

static inline BOOL CheckCoord3(void)
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


static inline BOOL CheckCoord2(void)
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

static inline BOOL CheckCoordL(short slx0,short sly0,short slx1,short sly1)
{
 if(slx0<0)
  {
   if((slx1-slx0)>CHKMAX_X) return TRUE;
  }
 if(slx1<0)
  {
   if((slx0-slx1)>CHKMAX_X) return TRUE;
  }
 if(sly0<0)
  {
   if((sly1-sly0)>CHKMAX_Y) return TRUE;
  }
 if(sly1<0)
  {
   if((sly0-sly1)>CHKMAX_Y) return TRUE;
  }

 return FALSE;
}


////////////////////////////////////////////////////////////////////////
// mask stuff... used in silent hill
////////////////////////////////////////////////////////////////////////

static inline void cmdSTP(unsigned char * baseAddr)
{
 uint32_t gdata = GETLE32(&((uint32_t*)baseAddr)[0]);

 lGPUstatusRet&=~0x1800;                                   // Clear the necessary bits
 lGPUstatusRet|=((gdata & 0x03) << 11);                    // Set the necessary bits

 if(gdata&1) {sSetMask=0x8000;lSetMask=0x80008000;}
 else        {sSetMask=0;     lSetMask=0;         }

 if(gdata&2) bCheckMask=TRUE;
 else        bCheckMask=FALSE;
}
 
////////////////////////////////////////////////////////////////////////
// cmd: Set texture page infos
////////////////////////////////////////////////////////////////////////

static void cmdTexturePage(unsigned char * baseAddr)
{
 uint32_t gdata = GETLE32(&((uint32_t*)baseAddr)[0]);

 lGPUstatusRet&=~0x000007ff;
 lGPUstatusRet|=(gdata & 0x07ff);
 
 usMirror=gdata&0x3000;
 
 UpdateGlobalTP((unsigned short)gdata);
}

////////////////////////////////////////////////////////////////////////
// cmd: turn on/off texture window
////////////////////////////////////////////////////////////////////////

static void cmdTextureWindow(unsigned char *baseAddr)
{
 uint32_t gdata = GETLE32(&((uint32_t*)baseAddr)[0]);

 uint32_t YAlign,XAlign;

 lGPUInfoVals[INFO_TW]=gdata&0xFFFFF;

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
 TWin.ymask = TWin.Position.y1 - 1;

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
 TWin.xmask = TWin.Position.x1 - 1;

 // Re-calculate the bit field, because we can't trust what is passed in the data


 YAlign = (uint32_t)(32 - (TWin.Position.y1 >> 3));
 XAlign = (uint32_t)(32 - (TWin.Position.x1 >> 3));

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
  }                                                    
 else                                                  // otherwise
  {
   bUsingTWin = TRUE;                                  // -> tw turned on
  }
}

////////////////////////////////////////////////////////////////////////
// cmd: start of drawing area... primitives will be clipped inside
////////////////////////////////////////////////////////////////////////



static void cmdDrawAreaStart(unsigned char * baseAddr)
{
 uint32_t gdata = GETLE32(&((uint32_t*)baseAddr)[0]);

 drawX  = gdata & 0x3ff;                               // for soft drawing

   lGPUInfoVals[INFO_DRAWSTART]=gdata&0xFFFFF;
   drawY  = (gdata>>10)&0x3ff;
   if(drawY>=512) drawY=511;                           // some security
}

////////////////////////////////////////////////////////////////////////
// cmd: end of drawing area... primitives will be clipped inside
////////////////////////////////////////////////////////////////////////

static void cmdDrawAreaEnd(unsigned char * baseAddr)
{
 uint32_t gdata = GETLE32(&((uint32_t*)baseAddr)[0]);

 drawW  = gdata & 0x3ff;                               // for soft drawing

   lGPUInfoVals[INFO_DRAWEND]=gdata&0xFFFFF;
   drawH  = (gdata>>10)&0x3ff;
   if(drawH>=512) drawH=511;                           // some security
}

////////////////////////////////////////////////////////////////////////
// cmd: draw offset... will be added to prim coords
////////////////////////////////////////////////////////////////////////

static void cmdDrawOffset(unsigned char * baseAddr)
{
 uint32_t gdata = GETLE32(&((uint32_t*)baseAddr)[0]);

 PSXDisplay.DrawOffset.x = (short)(gdata & 0x7ff);

   lGPUInfoVals[INFO_DRAWOFF]=gdata&0x3FFFFF;
   PSXDisplay.DrawOffset.y = (short)((gdata>>11) & 0x7ff);
 
 PSXDisplay.DrawOffset.y=(short)(((int)PSXDisplay.DrawOffset.y<<21)>>21);
 PSXDisplay.DrawOffset.x=(short)(((int)PSXDisplay.DrawOffset.x<<21)>>21);
}
 
////////////////////////////////////////////////////////////////////////
// cmd: load image to vram
////////////////////////////////////////////////////////////////////////

static void primLoadImage(unsigned char * baseAddr)
{
 unsigned short *sgpuData = ((unsigned short *) baseAddr);

 VRAMWrite.x      = GETLEs16(&sgpuData[2])&0x3ff;
 VRAMWrite.y      = GETLEs16(&sgpuData[3])&511;
 VRAMWrite.Width  = GETLEs16(&sgpuData[4]);
 VRAMWrite.Height = GETLEs16(&sgpuData[5]);

 DataWriteMode = DR_VRAMTRANSFER;

 VRAMWrite.ImagePtr = psxVuw + (VRAMWrite.y<<10) + VRAMWrite.x;
 VRAMWrite.RowsRemaining = VRAMWrite.Width;
 VRAMWrite.ColsRemaining = VRAMWrite.Height;
}

////////////////////////////////////////////////////////////////////////
// cmd: vram -> psx mem
////////////////////////////////////////////////////////////////////////

static void primStoreImage(unsigned char * baseAddr)
{
 unsigned short *sgpuData = ((unsigned short *) baseAddr);

 VRAMRead.x      = GETLEs16(&sgpuData[2])&0x03ff;
 VRAMRead.y      = GETLEs16(&sgpuData[3])&511;
 VRAMRead.Width  = GETLEs16(&sgpuData[4]);
 VRAMRead.Height = GETLEs16(&sgpuData[5]);

 VRAMRead.ImagePtr = psxVuw + (VRAMRead.y<<10) + VRAMRead.x;
 VRAMRead.RowsRemaining = VRAMRead.Width;
 VRAMRead.ColsRemaining = VRAMRead.Height;

 DataReadMode = DR_VRAMTRANSFER;

 lGPUstatusRet |= GPUSTATUS_READYFORVRAM;
}

////////////////////////////////////////////////////////////////////////
// cmd: blkfill - NO primitive! Doesn't care about draw areas...
////////////////////////////////////////////////////////////////////////

static void primBlkFill(unsigned char * baseAddr)
{
 uint32_t *gpuData = ((uint32_t *) baseAddr);
 short *sgpuData = ((short *) baseAddr);

 short sX = GETLEs16(&sgpuData[2]);
 short sY = GETLEs16(&sgpuData[3]);
 short sW = GETLEs16(&sgpuData[4]) & 0x3ff;
 short sH = GETLEs16(&sgpuData[5]) & 0x3ff;

 sW = (sW+15) & ~15;

 // Increase H & W if they are one short of full values, because they never can be full values
 if (sH >= 1023) sH=1024;
 if (sW >= 1023) sW=1024; 

 // x and y of end pos
 sW+=sX;
 sH+=sY;

 FillSoftwareArea(sX, sY, sW, sH, BGR24to16(GETLE32(&gpuData[0])));

 bDoVSyncUpdate=TRUE;
}
 
////////////////////////////////////////////////////////////////////////
// cmd: move image vram -> vram
////////////////////////////////////////////////////////////////////////

static void primMoveImage(unsigned char * baseAddr)
{
 short *sgpuData = ((short *) baseAddr);

 short imageY0,imageX0,imageY1,imageX1,imageSX,imageSY,i,j;

 imageX0 = GETLEs16(&sgpuData[2])&0x03ff;
 imageY0 = GETLEs16(&sgpuData[3])&511;
 imageX1 = GETLEs16(&sgpuData[4])&0x03ff;
 imageY1 = GETLEs16(&sgpuData[5])&511;
 imageSX = GETLEs16(&sgpuData[6]);
 imageSY = GETLEs16(&sgpuData[7]);

 if((imageX0 == imageX1) && (imageY0 == imageY1)) return; 
 if(imageSX<=0)  return;
 if(imageSY<=0)  return;

 if((imageY0+imageSY)>512 ||
    (imageX0+imageSX)>1024       ||
    (imageY1+imageSY)>512 ||
    (imageX1+imageSX)>1024)
  {
   int i,j;
   for(j=0;j<imageSY;j++)
    for(i=0;i<imageSX;i++)
     psxVuw [(1024*((imageY1+j)&511))+((imageX1+i)&0x3ff)]=
      psxVuw[(1024*((imageY0+j)&511))+((imageX0+i)&0x3ff)];

   bDoVSyncUpdate=TRUE;
 
   return;
  }
 
 if((imageSX|imageX0|imageX1)&1)                       // not dword aligned? slower func
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
 else                                                  // dword aligned
  {
   uint32_t *SRCPtr, *DSTPtr;
   unsigned short LineOffset;
   int dx=imageSX>>1;

   SRCPtr = (uint32_t *)(psxVuw + (1024*imageY0) + imageX0);
   DSTPtr = (uint32_t *)(psxVuw + (1024*imageY1) + imageX1);

   LineOffset = 512 - dx;

   for(j=0;j<imageSY;j++)
    {
     for(i=0;i<dx;i++) *DSTPtr++ = *SRCPtr++;
     SRCPtr += LineOffset;
     DSTPtr += LineOffset;
    }
  }

 imageSX+=imageX1;
 imageSY+=imageY1;

 bDoVSyncUpdate=TRUE;
}

////////////////////////////////////////////////////////////////////////
// cmd: draw free-size Tile 
////////////////////////////////////////////////////////////////////////

static void primTileS(unsigned char * baseAddr)
{
 uint32_t *gpuData = ((uint32_t*)baseAddr);
 short *sgpuData = ((short *) baseAddr);
 short sW = GETLEs16(&sgpuData[4]) & 0x3ff;
 short sH = GETLEs16(&sgpuData[5]) & 511;              // mmm... limit tiles to 0x1ff or height?

 lx0 = GETLEs16(&sgpuData[2]);
 ly0 = GETLEs16(&sgpuData[3]);

 if(!(dwActFixes&8)) AdjustCoord1();
                      
 // x and y of start
 ly2 = ly3 = ly0+sH +PSXDisplay.DrawOffset.y;
 ly0 = ly1 = ly0    +PSXDisplay.DrawOffset.y;
 lx1 = lx2 = lx0+sW +PSXDisplay.DrawOffset.x;
 lx0 = lx3 = lx0    +PSXDisplay.DrawOffset.x;

 DrawSemiTrans = (SEMITRANSBIT(GETLE32(&gpuData[0]))) ? TRUE : FALSE;

  FillSoftwareAreaTrans(lx0,ly0,lx2,ly2,
                        BGR24to16(GETLE32(&gpuData[0])));          

 bDoVSyncUpdate=TRUE;
}

////////////////////////////////////////////////////////////////////////
// cmd: draw 1 dot Tile (point)
////////////////////////////////////////////////////////////////////////

static void primTile1(unsigned char * baseAddr)
{
 uint32_t *gpuData = ((uint32_t*)baseAddr);
 short *sgpuData = ((short *) baseAddr);
 short sH = 1;
 short sW = 1;

 lx0 = GETLEs16(&sgpuData[2]);
 ly0 = GETLEs16(&sgpuData[3]);

 if(!(dwActFixes&8)) AdjustCoord1();

 // x and y of start
 ly2 = ly3 = ly0+sH +PSXDisplay.DrawOffset.y;
 ly0 = ly1 = ly0    +PSXDisplay.DrawOffset.y;
 lx1 = lx2 = lx0+sW +PSXDisplay.DrawOffset.x;
 lx0 = lx3 = lx0    +PSXDisplay.DrawOffset.x;

 DrawSemiTrans = (SEMITRANSBIT(GETLE32(&gpuData[0]))) ? TRUE : FALSE;

 FillSoftwareAreaTrans(lx0,ly0,lx2,ly2,
                       BGR24to16(GETLE32(&gpuData[0])));          // Takes Start and Offset

 bDoVSyncUpdate=TRUE;
}

////////////////////////////////////////////////////////////////////////
// cmd: draw 8 dot Tile (small rect)
////////////////////////////////////////////////////////////////////////

static void primTile8(unsigned char * baseAddr)
{
 uint32_t *gpuData = ((uint32_t*)baseAddr);
 short *sgpuData = ((short *) baseAddr);
 short sH = 8;
 short sW = 8;

 lx0 = GETLEs16(&sgpuData[2]);
 ly0 = GETLEs16(&sgpuData[3]);

 if(!(dwActFixes&8)) AdjustCoord1();

 // x and y of start
 ly2 = ly3 = ly0+sH +PSXDisplay.DrawOffset.y;
 ly0 = ly1 = ly0    +PSXDisplay.DrawOffset.y;
 lx1 = lx2 = lx0+sW +PSXDisplay.DrawOffset.x;
 lx0 = lx3 = lx0    +PSXDisplay.DrawOffset.x;

 DrawSemiTrans = (SEMITRANSBIT(GETLE32(&gpuData[0]))) ? TRUE : FALSE;

 FillSoftwareAreaTrans(lx0,ly0,lx2,ly2,
                       BGR24to16(GETLE32(&gpuData[0])));          // Takes Start and Offset

 bDoVSyncUpdate=TRUE;
}

////////////////////////////////////////////////////////////////////////
// cmd: draw 16 dot Tile (medium rect)
////////////////////////////////////////////////////////////////////////

static void primTile16(unsigned char * baseAddr)
{
 uint32_t *gpuData = ((uint32_t*)baseAddr);
 short *sgpuData = ((short *) baseAddr);
 short sH = 16;
 short sW = 16;

 lx0 = GETLEs16(&sgpuData[2]);
 ly0 = GETLEs16(&sgpuData[3]);

 if(!(dwActFixes&8)) AdjustCoord1();

 // x and y of start
 ly2 = ly3 = ly0+sH +PSXDisplay.DrawOffset.y;
 ly0 = ly1 = ly0    +PSXDisplay.DrawOffset.y;
 lx1 = lx2 = lx0+sW +PSXDisplay.DrawOffset.x;
 lx0 = lx3 = lx0    +PSXDisplay.DrawOffset.x;

 DrawSemiTrans = (SEMITRANSBIT(GETLE32(&gpuData[0]))) ? TRUE : FALSE;

 FillSoftwareAreaTrans(lx0,ly0,lx2,ly2,
                       BGR24to16(GETLE32(&gpuData[0])));          // Takes Start and Offset

 bDoVSyncUpdate=TRUE;
}

////////////////////////////////////////////////////////////////////////
// cmd: small sprite (textured rect)
////////////////////////////////////////////////////////////////////////

static void primSprt8(unsigned char * baseAddr)
{
 uint32_t *gpuData = ((uint32_t *) baseAddr);
 short *sgpuData = ((short *) baseAddr);

 lx0 = GETLEs16(&sgpuData[2]);
 ly0 = GETLEs16(&sgpuData[3]);

 if(!(dwActFixes&8)) AdjustCoord1();

 SetRenderMode(GETLE32(&gpuData[0]));

 if(bUsingTWin) DrawSoftwareSpriteTWin(baseAddr,8,8);
 else
 if(usMirror)   DrawSoftwareSpriteMirror(baseAddr,8,8);
 else           DrawSoftwareSprite(baseAddr,8,8,
                                   baseAddr[8],
                                   baseAddr[9]);

 bDoVSyncUpdate=TRUE;
}

////////////////////////////////////////////////////////////////////////
// cmd: medium sprite (textured rect)
////////////////////////////////////////////////////////////////////////

static void primSprt16(unsigned char * baseAddr)
{
 uint32_t *gpuData = ((uint32_t *) baseAddr);
 short *sgpuData = ((short *) baseAddr);

 lx0 = GETLEs16(&sgpuData[2]);
 ly0 = GETLEs16(&sgpuData[3]);

 if(!(dwActFixes&8)) AdjustCoord1();

 SetRenderMode(GETLE32(&gpuData[0]));

 if(bUsingTWin) DrawSoftwareSpriteTWin(baseAddr,16,16);
 else
 if(usMirror)   DrawSoftwareSpriteMirror(baseAddr,16,16);
 else           DrawSoftwareSprite(baseAddr,16,16,
                                   baseAddr[8],
                                   baseAddr[9]);

 bDoVSyncUpdate=TRUE;
}

////////////////////////////////////////////////////////////////////////
// cmd: free-size sprite (textured rect)
////////////////////////////////////////////////////////////////////////

// func used on texture coord wrap
static void primSprtSRest(unsigned char * baseAddr,unsigned short type)
{
 uint32_t *gpuData = ((uint32_t *) baseAddr);
 short *sgpuData = ((short *) baseAddr);
 unsigned short sTypeRest=0;

 short s;
 short sX = GETLEs16(&sgpuData[2]);
 short sY = GETLEs16(&sgpuData[3]);
 short sW = GETLEs16(&sgpuData[6]) & 0x3ff;
 short sH = GETLEs16(&sgpuData[7]) & 0x1ff;
 short tX = baseAddr[8];
 short tY = baseAddr[9];

 switch(type)
  {
   case 1:
    s=256-baseAddr[8];
    sW-=s;
    sX+=s;
    tX=0;
    break;
   case 2:
    s=256-baseAddr[9];
    sH-=s;
    sY+=s;
    tY=0;
    break;
   case 3:
    s=256-baseAddr[8];
    sW-=s;
    sX+=s;
    tX=0;
    s=256-baseAddr[9];
    sH-=s;
    sY+=s;
    tY=0;
    break;
   case 4:
    s=512-baseAddr[8];
    sW-=s;
    sX+=s;
    tX=0;
    break;
   case 5:
    s=512-baseAddr[9];
    sH-=s;
    sY+=s;
    tY=0;
    break;
   case 6:
    s=512-baseAddr[8];
    sW-=s;
    sX+=s;
    tX=0;
    s=512-baseAddr[9];
    sH-=s;
    sY+=s;
    tY=0;
    break;
  }

 SetRenderMode(GETLE32(&gpuData[0]));

 if(tX+sW>256) {sW=256-tX;sTypeRest+=1;}
 if(tY+sH>256) {sH=256-tY;sTypeRest+=2;}

 lx0 = sX;
 ly0 = sY;

 if(!(dwActFixes&8)) AdjustCoord1();

 DrawSoftwareSprite(baseAddr,sW,sH,tX,tY);

 if(sTypeRest && type<4)  
  {
   if(sTypeRest&1  && type==1)  primSprtSRest(baseAddr,4);
   if(sTypeRest&2  && type==2)  primSprtSRest(baseAddr,5);
   if(sTypeRest==3 && type==3)  primSprtSRest(baseAddr,6);
  }

}
                                     
////////////////////////////////////////////////////////////////////////

static void primSprtS(unsigned char * baseAddr)
{
 uint32_t *gpuData = ((uint32_t *) baseAddr);
 short *sgpuData = ((short *) baseAddr);
 short sW,sH;

 lx0 = GETLEs16(&sgpuData[2]);
 ly0 = GETLEs16(&sgpuData[3]);

 if(!(dwActFixes&8)) AdjustCoord1();

 sW = GETLEs16(&sgpuData[6]) & 0x3ff;
 sH = GETLEs16(&sgpuData[7]) & 0x1ff;

 SetRenderMode(GETLE32(&gpuData[0]));

 if(bUsingTWin) DrawSoftwareSpriteTWin(baseAddr,sW,sH);
 else
 if(usMirror)   DrawSoftwareSpriteMirror(baseAddr,sW,sH);
 else          
  {
   unsigned short sTypeRest=0;
   short tX=baseAddr[8];
   short tY=baseAddr[9];

   if(tX+sW>256) {sW=256-tX;sTypeRest+=1;}
   if(tY+sH>256) {sH=256-tY;sTypeRest+=2;}

   DrawSoftwareSprite(baseAddr,sW,sH,tX,tY);

   if(sTypeRest) 
    {
     if(sTypeRest&1)  primSprtSRest(baseAddr,1);
     if(sTypeRest&2)  primSprtSRest(baseAddr,2);
     if(sTypeRest==3) primSprtSRest(baseAddr,3);
    }

  }

 bDoVSyncUpdate=TRUE;
}

////////////////////////////////////////////////////////////////////////
// cmd: flat shaded Poly4
////////////////////////////////////////////////////////////////////////

static void primPolyF4(unsigned char *baseAddr)
{
 uint32_t *gpuData = ((uint32_t *) baseAddr);
 short *sgpuData = ((short *) baseAddr);

 lx0 = GETLEs16(&sgpuData[2]);
 ly0 = GETLEs16(&sgpuData[3]);
 lx1 = GETLEs16(&sgpuData[4]);
 ly1 = GETLEs16(&sgpuData[5]);
 lx2 = GETLEs16(&sgpuData[6]);
 ly2 = GETLEs16(&sgpuData[7]);
 lx3 = GETLEs16(&sgpuData[8]);
 ly3 = GETLEs16(&sgpuData[9]);

 if(!(dwActFixes&8)) 
  {
   AdjustCoord4();
   if(CheckCoord4()) return;
  }

 offsetPSX4();
 DrawSemiTrans = (SEMITRANSBIT(GETLE32(&gpuData[0]))) ? TRUE : FALSE;

 drawPoly4F(GETLE32(&gpuData[0]));

 bDoVSyncUpdate=TRUE;
}

////////////////////////////////////////////////////////////////////////
// cmd: smooth shaded Poly4
////////////////////////////////////////////////////////////////////////

static void primPolyG4(unsigned char * baseAddr)
{
 uint32_t *gpuData = (uint32_t *)baseAddr;
 short *sgpuData = ((short *) baseAddr);

 lx0 = GETLEs16(&sgpuData[2]);
 ly0 = GETLEs16(&sgpuData[3]);
 lx1 = GETLEs16(&sgpuData[6]);
 ly1 = GETLEs16(&sgpuData[7]);
 lx2 = GETLEs16(&sgpuData[10]);
 ly2 = GETLEs16(&sgpuData[11]);
 lx3 = GETLEs16(&sgpuData[14]);
 ly3 = GETLEs16(&sgpuData[15]);

 if(!(dwActFixes&8))
  {
   AdjustCoord4();
   if(CheckCoord4()) return;
  }

 offsetPSX4();
 DrawSemiTrans = (SEMITRANSBIT(GETLE32(&gpuData[0]))) ? TRUE : FALSE;

 drawPoly4G(GETLE32(&gpuData[0]), GETLE32(&gpuData[2]), 
            GETLE32(&gpuData[4]), GETLE32(&gpuData[6]));

 bDoVSyncUpdate=TRUE;
}

////////////////////////////////////////////////////////////////////////
// cmd: flat shaded Texture3
////////////////////////////////////////////////////////////////////////

static void primPolyFT3(unsigned char * baseAddr)
{
 uint32_t *gpuData = ((uint32_t *) baseAddr);
 short *sgpuData = ((short *) baseAddr);

 lx0 = GETLEs16(&sgpuData[2]);
 ly0 = GETLEs16(&sgpuData[3]);
 lx1 = GETLEs16(&sgpuData[6]);
 ly1 = GETLEs16(&sgpuData[7]);
 lx2 = GETLEs16(&sgpuData[10]);
 ly2 = GETLEs16(&sgpuData[11]);

 lLowerpart=GETLE32(&gpuData[4])>>16;
 UpdateGlobalTP((unsigned short)lLowerpart);

 if(!(dwActFixes&8))
  {
   AdjustCoord3();
   if(CheckCoord3()) return;
  }

 offsetPSX3();
 SetRenderMode(GETLE32(&gpuData[0]));

 drawPoly3FT(baseAddr);

 bDoVSyncUpdate=TRUE;
}

////////////////////////////////////////////////////////////////////////
// cmd: flat shaded Texture4
////////////////////////////////////////////////////////////////////////

static void primPolyFT4(unsigned char * baseAddr)
{
 uint32_t *gpuData = ((uint32_t *) baseAddr);
 short *sgpuData = ((short *) baseAddr);

 lx0 = GETLEs16(&sgpuData[2]);
 ly0 = GETLEs16(&sgpuData[3]);
 lx1 = GETLEs16(&sgpuData[6]);
 ly1 = GETLEs16(&sgpuData[7]);
 lx2 = GETLEs16(&sgpuData[10]);
 ly2 = GETLEs16(&sgpuData[11]);
 lx3 = GETLEs16(&sgpuData[14]);
 ly3 = GETLEs16(&sgpuData[15]);

 lLowerpart=GETLE32(&gpuData[4])>>16;
 UpdateGlobalTP((unsigned short)lLowerpart);

 if(!(dwActFixes&8))
  {
   AdjustCoord4();
   if(CheckCoord4()) return;
  }

 offsetPSX4();

 SetRenderMode(GETLE32(&gpuData[0]));

 drawPoly4FT(baseAddr);

 bDoVSyncUpdate=TRUE;
}

////////////////////////////////////////////////////////////////////////
// cmd: smooth shaded Texture3
////////////////////////////////////////////////////////////////////////

static void primPolyGT3(unsigned char *baseAddr)
{    
 uint32_t *gpuData = ((uint32_t *) baseAddr);
 short *sgpuData = ((short *) baseAddr);

 lx0 = GETLEs16(&sgpuData[2]);
 ly0 = GETLEs16(&sgpuData[3]);
 lx1 = GETLEs16(&sgpuData[8]);
 ly1 = GETLEs16(&sgpuData[9]);
 lx2 = GETLEs16(&sgpuData[14]);
 ly2 = GETLEs16(&sgpuData[15]);

 lLowerpart=GETLE32(&gpuData[5])>>16;
 UpdateGlobalTP((unsigned short)lLowerpart);

 if(!(dwActFixes&8))
  {
   AdjustCoord3();
   if(CheckCoord3()) return;
  }
           
 offsetPSX3();
 DrawSemiTrans = (SEMITRANSBIT(GETLE32(&gpuData[0]))) ? TRUE : FALSE;

 if(SHADETEXBIT(GETLE32(&gpuData[0])))
  {
   gpuData[0] = (gpuData[0]&HOST2LE32(0xff000000))|HOST2LE32(0x00808080);
   gpuData[3] = (gpuData[3]&HOST2LE32(0xff000000))|HOST2LE32(0x00808080);
   gpuData[6] = (gpuData[6]&HOST2LE32(0xff000000))|HOST2LE32(0x00808080);
  }

 drawPoly3GT(baseAddr);

 bDoVSyncUpdate=TRUE;
}

////////////////////////////////////////////////////////////////////////
// cmd: smooth shaded Poly3
////////////////////////////////////////////////////////////////////////

static void primPolyG3(unsigned char *baseAddr)
{    
 uint32_t *gpuData = ((uint32_t *) baseAddr);
 short *sgpuData = ((short *) baseAddr);

 lx0 = GETLEs16(&sgpuData[2]);
 ly0 = GETLEs16(&sgpuData[3]);
 lx1 = GETLEs16(&sgpuData[6]);
 ly1 = GETLEs16(&sgpuData[7]);
 lx2 = GETLEs16(&sgpuData[10]);
 ly2 = GETLEs16(&sgpuData[11]);

 if(!(dwActFixes&8))
  {
   AdjustCoord3();
   if(CheckCoord3()) return;
  }

 offsetPSX3();
 DrawSemiTrans = (SEMITRANSBIT(GETLE32(&gpuData[0]))) ? TRUE : FALSE;

 drawPoly3G(GETLE32(&gpuData[0]), GETLE32(&gpuData[2]), GETLE32(&gpuData[4]));

 bDoVSyncUpdate=TRUE;
}

////////////////////////////////////////////////////////////////////////
// cmd: smooth shaded Texture4
////////////////////////////////////////////////////////////////////////

static void primPolyGT4(unsigned char *baseAddr)
{ 
 uint32_t *gpuData = ((uint32_t *) baseAddr);
 short *sgpuData = ((short *) baseAddr);

 lx0 = GETLEs16(&sgpuData[2]);
 ly0 = GETLEs16(&sgpuData[3]);
 lx1 = GETLEs16(&sgpuData[8]);
 ly1 = GETLEs16(&sgpuData[9]);
 lx2 = GETLEs16(&sgpuData[14]);
 ly2 = GETLEs16(&sgpuData[15]);
 lx3 = GETLEs16(&sgpuData[20]);
 ly3 = GETLEs16(&sgpuData[21]);

 lLowerpart=GETLE32(&gpuData[5])>>16;
 UpdateGlobalTP((unsigned short)lLowerpart);

 if(!(dwActFixes&8))
  {
   AdjustCoord4();
   if(CheckCoord4()) return;
  }

 offsetPSX4();
 DrawSemiTrans = (SEMITRANSBIT(GETLE32(&gpuData[0]))) ? TRUE : FALSE;

 if(SHADETEXBIT(GETLE32(&gpuData[0])))
  {
   gpuData[0] = (gpuData[0]&HOST2LE32(0xff000000))|HOST2LE32(0x00808080);
   gpuData[3] = (gpuData[3]&HOST2LE32(0xff000000))|HOST2LE32(0x00808080);
   gpuData[6] = (gpuData[6]&HOST2LE32(0xff000000))|HOST2LE32(0x00808080);
   gpuData[9] = (gpuData[9]&HOST2LE32(0xff000000))|HOST2LE32(0x00808080);
  }

 drawPoly4GT(baseAddr);

 bDoVSyncUpdate=TRUE;
}

////////////////////////////////////////////////////////////////////////
// cmd: smooth shaded Poly3
////////////////////////////////////////////////////////////////////////

static void primPolyF3(unsigned char *baseAddr)
{    
 uint32_t *gpuData = ((uint32_t *) baseAddr);
 short *sgpuData = ((short *) baseAddr);

 lx0 = GETLEs16(&sgpuData[2]);
 ly0 = GETLEs16(&sgpuData[3]);
 lx1 = GETLEs16(&sgpuData[4]);
 ly1 = GETLEs16(&sgpuData[5]);
 lx2 = GETLEs16(&sgpuData[6]);
 ly2 = GETLEs16(&sgpuData[7]);

 if(!(dwActFixes&8))
  {
   AdjustCoord3();
   if(CheckCoord3()) return;
  }

 offsetPSX3();
 SetRenderMode(GETLE32(&gpuData[0]));

 drawPoly3F(GETLE32(&gpuData[0]));

 bDoVSyncUpdate=TRUE;
}

////////////////////////////////////////////////////////////////////////
// cmd: skipping shaded polylines
////////////////////////////////////////////////////////////////////////

static void primLineGSkip(unsigned char *baseAddr)
{    
 uint32_t *gpuData = ((uint32_t *) baseAddr);
 int iMax=255;
 int i=2;

 ly1 = (short)((GETLE32(&gpuData[1])>>16) & 0xffff);
 lx1 = (short)(GETLE32(&gpuData[1]) & 0xffff);

 while(!(((GETLE32(&gpuData[i]) & 0xF000F000) == 0x50005000) && i>=4))
  {
   i++;
   ly1 = (short)((GETLE32(&gpuData[i])>>16) & 0xffff);
   lx1 = (short)(GETLE32(&gpuData[i]) & 0xffff);
   i++;if(i>iMax) break;
  }
}

////////////////////////////////////////////////////////////////////////
// cmd: shaded polylines
////////////////////////////////////////////////////////////////////////

static void primLineGEx(unsigned char *baseAddr)
{    
 uint32_t *gpuData = ((uint32_t *) baseAddr);
 int iMax=255;
 uint32_t lc0,lc1;
 short slx0,slx1,sly0,sly1;int i=2;BOOL bDraw=TRUE;

 sly1 = (short)((GETLE32(&gpuData[1])>>16) & 0xffff);
 slx1 = (short)(GETLE32(&gpuData[1]) & 0xffff);

 if(!(dwActFixes&8)) 
  {
   slx1=(short)(((int)slx1<<SIGNSHIFT)>>SIGNSHIFT);
   sly1=(short)(((int)sly1<<SIGNSHIFT)>>SIGNSHIFT);
  }

 lc1 = GETLE32(&gpuData[0]) & 0xffffff;

 DrawSemiTrans = (SEMITRANSBIT(GETLE32(&gpuData[0]))) ? TRUE : FALSE;

 while(!(((GETLE32(&gpuData[i]) & 0xF000F000) == 0x50005000) && i>=4))
  {
   sly0=sly1; slx0=slx1; lc0=lc1;
   lc1=GETLE32(&gpuData[i]) & 0xffffff;

   i++;

   // no check needed on gshaded polyline positions
   // if((gpuData[i] & 0xF000F000) == 0x50005000) break;

   sly1 = (short)((GETLE32(&gpuData[i])>>16) & 0xffff);
   slx1 = (short)(GETLE32(&gpuData[i]) & 0xffff);

   if(!(dwActFixes&8))
    {
     slx1=(short)(((int)slx1<<SIGNSHIFT)>>SIGNSHIFT);
     sly1=(short)(((int)sly1<<SIGNSHIFT)>>SIGNSHIFT);
     if(CheckCoordL(slx0,sly0,slx1,sly1)) bDraw=FALSE; else bDraw=TRUE;
    }

   if ((lx0 != lx1) || (ly0 != ly1))
    {
     ly0=sly0;
     lx0=slx0;
     ly1=sly1;
     lx1=slx1;
              
     offsetPSX2();
     if(bDraw) DrawSoftwareLineShade(lc0, lc1);
    }
   i++;  
   if(i>iMax) break;
  }

 bDoVSyncUpdate=TRUE;
}

////////////////////////////////////////////////////////////////////////
// cmd: shaded polyline2
////////////////////////////////////////////////////////////////////////

static void primLineG2(unsigned char *baseAddr)
{    
 uint32_t *gpuData = ((uint32_t *) baseAddr);
 short *sgpuData = ((short *) baseAddr);

 lx0 = GETLEs16(&sgpuData[2]);
 ly0 = GETLEs16(&sgpuData[3]);
 lx1 = GETLEs16(&sgpuData[6]);
 ly1 = GETLEs16(&sgpuData[7]);

 if(!(dwActFixes&8))
  {
   AdjustCoord2();
   if(CheckCoord2()) return;
  }

 if((lx0 == lx1) && (ly0 == ly1)) {lx1++;ly1++;}

 DrawSemiTrans = (SEMITRANSBIT(GETLE32(&gpuData[0]))) ? TRUE : FALSE;
 offsetPSX2();
 DrawSoftwareLineShade(GETLE32(&gpuData[0]),GETLE32(&gpuData[2]));

 bDoVSyncUpdate=TRUE;
}

////////////////////////////////////////////////////////////////////////
// cmd: skipping flat polylines
////////////////////////////////////////////////////////////////////////

static void primLineFSkip(unsigned char *baseAddr)
{
 uint32_t *gpuData = ((uint32_t *) baseAddr);
 int i=2,iMax=255;

 ly1 = (short)((GETLE32(&gpuData[1])>>16) & 0xffff);
 lx1 = (short)(GETLE32(&gpuData[1]) & 0xffff);

 while(!(((GETLE32(&gpuData[i]) & 0xF000F000) == 0x50005000) && i>=3))
  {
   ly1 = (short)((GETLE32(&gpuData[i])>>16) & 0xffff);
   lx1 = (short)(GETLE32(&gpuData[i]) & 0xffff);
   i++;if(i>iMax) break;
  }             
}

////////////////////////////////////////////////////////////////////////
// cmd: drawing flat polylines
////////////////////////////////////////////////////////////////////////

static void primLineFEx(unsigned char *baseAddr)
{
 uint32_t *gpuData = ((uint32_t *) baseAddr);
 int iMax;
 short slx0,slx1,sly0,sly1;int i=2;BOOL bDraw=TRUE;

 iMax=255;

 sly1 = (short)((GETLE32(&gpuData[1])>>16) & 0xffff);
 slx1 = (short)(GETLE32(&gpuData[1]) & 0xffff);
 if(!(dwActFixes&8))
  {
   slx1=(short)(((int)slx1<<SIGNSHIFT)>>SIGNSHIFT);
   sly1=(short)(((int)sly1<<SIGNSHIFT)>>SIGNSHIFT);
  }

 SetRenderMode(GETLE32(&gpuData[0]));

 while(!(((GETLE32(&gpuData[i]) & 0xF000F000) == 0x50005000) && i>=3))
  {
   sly0 = sly1;slx0=slx1;
   sly1 = (short)((GETLE32(&gpuData[i])>>16) & 0xffff);
   slx1 = (short)(GETLE32(&gpuData[i]) & 0xffff);
   if(!(dwActFixes&8))
    {
     slx1=(short)(((int)slx1<<SIGNSHIFT)>>SIGNSHIFT);
     sly1=(short)(((int)sly1<<SIGNSHIFT)>>SIGNSHIFT);

     if(CheckCoordL(slx0,sly0,slx1,sly1)) bDraw=FALSE; else bDraw=TRUE;
    }

   ly0=sly0;
   lx0=slx0;
   ly1=sly1;
   lx1=slx1;

   offsetPSX2();
   if(bDraw) DrawSoftwareLineFlat(GETLE32(&gpuData[0]));

   i++;if(i>iMax) break;
  }

 bDoVSyncUpdate=TRUE;
}

////////////////////////////////////////////////////////////////////////
// cmd: drawing flat polyline2
////////////////////////////////////////////////////////////////////////

static void primLineF2(unsigned char *baseAddr)
{
 uint32_t *gpuData = ((uint32_t *) baseAddr);
 short *sgpuData = ((short *) baseAddr);

 lx0 = GETLEs16(&sgpuData[2]);
 ly0 = GETLEs16(&sgpuData[3]);
 lx1 = GETLEs16(&sgpuData[4]);
 ly1 = GETLEs16(&sgpuData[5]);

 if(!(dwActFixes&8))
  {
   AdjustCoord2();
   if(CheckCoord2()) return;
  }

 if((lx0 == lx1) && (ly0 == ly1)) {lx1++;ly1++;}
                    
 offsetPSX2();
 SetRenderMode(GETLE32(&gpuData[0]));

 DrawSoftwareLineFlat(GETLE32(&gpuData[0]));

 bDoVSyncUpdate=TRUE;
}

////////////////////////////////////////////////////////////////////////
// cmd: well, easiest command... not implemented
////////////////////////////////////////////////////////////////////////

static void primNI(unsigned char *bA)
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
