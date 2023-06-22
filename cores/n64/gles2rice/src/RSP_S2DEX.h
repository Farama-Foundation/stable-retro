/*
Copyright (C) 2002 Rice1964

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

#ifndef _RSP_S2DEX_H_
#define _RSP_S2DEX_H_

// GBI commands for S2DEX microcode.
#define S2DEX_BG_1CYC           0x01
#define S2DEX_BG_COPY           0x02
#define S2DEX_OBJ_RECTANGLE     0x03
#define S2DEX_OBJ_SPRITE        0x04
#define S2DEX_OBJ_MOVEMEM       0x05
#define S2DEX_SELECT_DL         0xb0
#define S2DEX_OBJ_RENDERMODE    0xb1
#define S2DEX_OBJ_RECTANGLE_R   0xb2
#define S2DEX_OBJ_LOADTXTR      0xc1
#define S2DEX_OBJ_LDTX_SPRITE   0xc2
#define S2DEX_OBJ_LDTX_RECT     0xc3
#define S2DEX_OBJ_LDTX_RECT_R   0xc4
#define S2DEX_RDPHALF_0         0xe4

// Struct types
#define S2DEX_OBJLT_TXTRBLOCK   0x00001033
#define S2DEX_OBJLT_TXTRTILE    0x00fc1034
#define S2DEX_OBJLT_TLUT        0x00000030

// Loaded types
#define S2DEX_BGLT_LOADBLOCK    0x0033
#define S2DEX_BGLT_LOADTILE     0xfff4

#ifdef MSB_FIRST

typedef struct
{
  uint32_t    type;  // Struct classification type. Should always be S2DEX_OBJLT_TXTRBLOCK (0x1033)
  uint32_t    image; // Texture address within the DRAM.

  uint16_t    tmem;  // TMEM load address
  uint16_t    tsize; // Texture size

  uint16_t    tline; // Texture line width for one line.
  uint16_t    sid;   // State ID

  uint32_t    flag;  // State flag
  uint32_t    mask;  // State mask
} uObjTxtrBlock;

typedef struct
{
  uint32_t    type;    // Struct classification type. Should always be S2DEX_OBJLT_TXTRTILE (0xfc1034)
  uint32_t    image;   // Texture address within the DRAM.

  uint16_t    tmem;    // TMEM load address
  uint16_t    twidth;  // Texture width

  uint16_t    theight; // Texture height
  uint16_t    sid;     // State ID. Always a multiple of 4. Can be 0, 4, 8, or 12. 

  uint32_t    flag;    // State flag
  uint32_t    mask;    // State mask
} uObjTxtrTile;      // 24 bytes

typedef struct
{
  uint32_t    type;  // Struct classification type, should always be S2DEX_OBJLT_TLUT (0x30)
  uint32_t    image; // Texture address within the DRAM.

  uint16_t    phead; // Number indicating the first loaded palette (Between 256 - 511).
  uint16_t    pnum;  // Loaded palette number - 1

  uint16_t    zero;  // Always 0
  uint16_t    sid;   // State ID. Always a multiple of 4. Can be 0, 4, 8, or 12.

  uint32_t    flag;  // State flag
  uint32_t    mask;  // State mask
} uObjTxtrTLUT;    // 24 bytes

typedef union
{
  uObjTxtrBlock  block;
  uObjTxtrTile   tile;
  uObjTxtrTLUT   tlut;
} uObjTxtr;

typedef struct
{
  short   objX;        // X-coordinate of the upper-left position of the sprite.
  uint16_t  scaleW;      // X-axis scale factor.

  uint16_t  imageW;      // Texture width
  uint16_t  paddingX;    // Unused, always 0.

  short   objY;        // Y-coordinate of the upper-left position of the sprite.
  uint16_t  scaleH;      // Y-axis scale factor.
  
  uint16_t  imageH;      // Texture height
  uint16_t  paddingY;    // Unused, always 0.

  uint16_t  imageStride; // Number of bytes from one row of pixels in memory to the next row of pixels (ie. Stride)
  uint16_t  imageAdrs;   // Texture header position within the TMEM.

  uint8_t   imageFmt;    // Texel format
  uint8_t   imageSiz;    // Texel size
  uint8_t   imagePal;    // Palette number. Can be from 0 - 7
  uint8_t   imageFlags;  // Flag used for image manipulations
} uObjSprite;


typedef struct 
{
  uObjTxtr      txtr;
  uObjSprite    sprite;
} uObjTxSprite;     /* 48 bytes */

typedef struct
{
  int32_t       A, B, C, D;

  short     X;
  short     Y;

  uint16_t   BaseScaleX;
  uint16_t   BaseScaleY;
} uObjMtx;

typedef struct
{
  float   A, B, C, D;
  float   X;
  float   Y;
  float   BaseScaleX;
  float   BaseScaleY;
} uObjMtxReal;

typedef struct
{
  short    X;
  short    Y;
  uint16_t   BaseScaleX;
  uint16_t   BaseScaleY;
} uObjSubMtx;


typedef struct
{
  uint16_t    imageX;     // X-coordinate of the upper-left position of the texture.
  uint16_t    imageW;     // Texture width

  short     frameX;     // X-coordinate of the upper-left position of the frame to be transferred.
  uint16_t    frameW;     // Width of the frame to be transferred.

  uint16_t    imageY;     // Y-coordinate of the upper-left position of the texture.
  uint16_t    imageH;     // Texture height

  short     frameY;     // Y-coordinate of the upper-left position of the frame to be transferred.  
  uint16_t    frameH;     // Height of the frame to be transferred.

  uint32_t    imagePtr;   // Texture address within the DRAM.

  uint16_t    imageLoad;  // Can either be S2DEX_BGLT_LOADBLOCK (0x0033) or #define S2DEX_BGLT_LOADTILE (0xfff4).
  uint8_t     imageFmt;   // Texel format
  uint8_t     imageSiz;   // Texel size
  
  uint16_t    imagePal;   // Palette number
  uint16_t    imageFlip;  // Inverts the image if 0x01 is set.

  // Set within initialization routines, should not need to be directly set.
  uint16_t    tmemW;
  uint16_t    tmemH;
  uint16_t    tmemLoadSH;
  uint16_t    tmemLoadTH;
  uint16_t    tmemSizeW;
  uint16_t    tmemSize;
} uObjBg;

typedef struct
{
  uint16_t    imageX;     // X-coordinate of the upper-left position of the texture.
  uint16_t    imageW;     // Texture width

  short     frameX;     // X-coordinate of the upper-left position of the frame to be transferred.
  uint16_t    frameW;     // Width of the frame to be transferred.

  uint16_t    imageY;     // Y-coordinate of the upper-left position of the texture.
  uint16_t    imageH;     // Texture height

  short     frameY;     // Y-coordinate of the upper-left position of the frame to be transferred.   
  uint16_t    frameH;     // Height of the frame to be transferred.

  uint32_t    imagePtr;   // Texture address within the DRAM.

  uint16_t    imageLoad;  // Can either be S2DEX_BGLT_LOADBLOCK (0x0033) or S2DEX_BGLT_LOADTILE (0xfff4)
  uint8_t     imageFmt;   // Texel format
  uint8_t     imageSiz;   // Texel size

  uint16_t    imagePal;   // Palette number
  uint16_t    imageFlip;  // Inverts the image if 0x01 is set.

  uint16_t    scaleW;     // X-axis scale factor
  uint16_t    scaleH;     // Y-axis scale factor

  int32_t     imageYorig; // Starting point in the original image.
  uint8_t     padding[4];
} uObjScaleBg;

#else

typedef struct //Intel Format
{
  uint32_t    type;  // Struct classification type. Should always be S2DEX_OBJLT_TXTRBLOCK (0x1033)
  uint32_t    image; // Texture address within the DRAM.

  uint16_t    tsize; // Texture size
  uint16_t    tmem;  // TMEM load address

  uint16_t    sid;   // State ID
  uint16_t    tline; // Texture line width for one line.

  uint32_t    flag;  // State flag
  uint32_t    mask;  // State mask
} uObjTxtrBlock;

typedef struct //Intel Format
{
  uint32_t    type;    // Struct classification type. Should always be S2DEX_OBJLT_TXTRTILE (0xfc1034)
  uint32_t    image;   // Texture address within the DRAM.

  uint16_t    twidth;  // Texture width
  uint16_t    tmem;    // TMEM load address

  uint16_t    sid;     // State ID. Always a multiple of 4. Can be 0, 4, 8, or 12. 
  uint16_t    theight; // Texture height

  uint32_t    flag;    // State flag
  uint32_t    mask;    // State mask
} uObjTxtrTile;      // 24 bytes

typedef struct // Intel Format
{
  uint32_t    type;  // Struct classification type, should always be S2DEX_OBJLT_TLUT (0x30)
  uint32_t    image; // Texture address within the DRAM.

  uint16_t    pnum;  // Loaded palette number - 1
  uint16_t    phead; // Number indicating the first loaded palette (Between 256 - 511).

  uint16_t    sid;   // State ID. Always a multiple of 4. Can be 0, 4, 8, or 12.
  uint16_t    zero;  // Always 0

  uint32_t    flag;  // State flag
  uint32_t    mask;  // State mask
} uObjTxtrTLUT;    // 24 bytes

typedef union
{
  uObjTxtrBlock  block;
  uObjTxtrTile   tile;
  uObjTxtrTLUT   tlut;
} uObjTxtr;

typedef struct // Intel format
{
  uint16_t  scaleW;      // X-axis scale factor.
  short   objX;        // X-coordinate of the upper-left position of the sprite.
  
  uint16_t  paddingX;    // Unused, always 0.
  uint16_t  imageW;      // Texture width
  
  uint16_t  scaleH;      // Y-axis scale factor.
  short   objY;        // Y-coordinate of the upper-left position of the sprite.
  
  uint16_t  paddingY;    // Unused, always 0.
  uint16_t  imageH;      // Texture height
  
  uint16_t  imageAdrs;   // Texture header position within the TMEM.
  uint16_t  imageStride; // Number of bytes from one row of pixels in memory to the next row of pixels (ie. Stride)

  uint8_t   imageFlags;  // Flag used for image manipulations
  uint8_t   imagePal;    // Palette number. Can be from 0 - 7
  uint8_t   imageSiz;    // Texel size
  uint8_t   imageFmt;    // Texel format
} uObjSprite;


typedef struct 
{
  uObjTxtr      txtr;
  uObjSprite    sprite;
} uObjTxSprite;     /* 48 bytes */

typedef struct // Intel format
{
  int32_t       A, B, C, D;

  short     Y;
  short     X;

  uint16_t   BaseScaleY;
  uint16_t   BaseScaleX;
} uObjMtx;

typedef struct  //Intel format
{
  float   A, B, C, D;
  float   X;
  float   Y;
  float   BaseScaleX;
  float   BaseScaleY;
} uObjMtxReal;

typedef struct
{
  short    Y;
  short    X;
  uint16_t   BaseScaleY;
  uint16_t   BaseScaleX;
} uObjSubMtx;


typedef struct // Intel Format
{
  uint16_t    imageW;     // Texture width
  uint16_t    imageX;     // X-coordinate of the upper-left position of the texture.

  uint16_t    frameW;     // Width of the frame to be transferred.
  short     frameX;     // X-coordinate of the upper-left position of the frame to be transferred.

  uint16_t    imageH;     // Texture height
  uint16_t    imageY;     // Y-coordinate of the upper-left position of the texture.

  uint16_t    frameH;     // Height of the frame to be transferred.
  short     frameY;     // Y-coordinate of the upper-left position of the frame to be transferred.  

  uint32_t    imagePtr;   // Texture address within the DRAM.

  uint8_t     imageSiz;   // Texel size
  uint8_t     imageFmt;   // Texel format
  uint16_t    imageLoad;  // Can either be S2DEX_BGLT_LOADBLOCK (0x0033) or #define S2DEX_BGLT_LOADTILE (0xfff4).

  uint16_t    imageFlip;  // Inverts the image if 0x01 is set.
  uint16_t    imagePal;   // Palette number

  // Set within initialization routines, should not need to be directly set.
  uint16_t    tmemH;
  uint16_t    tmemW;
  uint16_t    tmemLoadTH;
  uint16_t    tmemLoadSH;
  uint16_t    tmemSize;
  uint16_t    tmemSizeW;
} uObjBg;

// Intel Format
typedef struct
{
  uint16_t    imageW;     // Texture width
  uint16_t    imageX;     // X-coordinate of the upper-left position of the texture.

  uint16_t    frameW;     // Width of the frame to be transferred.
  short     frameX;     // X-coordinate of the upper-left position of the frame to be transferred.

  uint16_t    imageH;     // Texture height
  uint16_t    imageY;     // Y-coordinate of the upper-left position of the texture.

  uint16_t    frameH;     // Height of the frame to be transferred.
  short     frameY;     // Y-coordinate of the upper-left position of the frame to be transferred.   

  uint32_t    imagePtr;   // Texture address within the DRAM.

  uint8_t     imageSiz;   // Texel size
  uint8_t     imageFmt;   // Texel format
  uint16_t    imageLoad;  // Can either be S2DEX_BGLT_LOADBLOCK (0x0033) or S2DEX_BGLT_LOADTILE (0xfff4)

  uint16_t    imageFlip;  // Inverts the image if 0x01 is set.
  uint16_t    imagePal;   // Palette number

  uint16_t    scaleH;     // Y-axis scale factor
  uint16_t    scaleW;     // X-axis scale factor

  int32_t       imageYorig; // Starting point in the original image.
  uint8_t     padding[4];
} uObjScaleBg;

#endif

#endif

