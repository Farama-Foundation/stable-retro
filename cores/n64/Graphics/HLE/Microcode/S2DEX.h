#ifndef _GRAPHICS_HLE_MICROCODE_S2DEX_H
#define _GRAPHICS_HLE_MICROCODE_S2DEX_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define G_BGLT_LOADBLOCK         0x0033
#define G_BGLT_LOADTILE          0xfff4

#define G_BG_FLAG_FLIPS          0x01
#define G_BG_FLAG_FLIPT          0x10

#define	G_OBJRM_NOTXCLAMP		   0x01
#define	G_OBJRM_XLU				   0x02
#define	G_OBJRM_ANTIALIAS		   0x04
#define	G_OBJRM_BILERP			   0x08
#define	G_OBJRM_SHRINKSIZE_1	   0x10
#define	G_OBJRM_SHRINKSIZE_2	   0x20
#define	G_OBJRM_WIDEN			   0x40

#define S2DEX_MV_MATRIX			   0
#define S2DEX_MV_SUBMATRIX		   2
#define S2DEX_MV_VIEWPORT		   8

#define S2DEX_BG_1CYC            0x01
#define S2DEX_BG_COPY            0x02
#define S2DEX_OBJ_RECTANGLE      0x03
#define S2DEX_OBJ_SPRITE         0x04
#define S2DEX_OBJ_MOVEMEM        0x05
#define S2DEX_LOAD_UCODE         0xAF
#define S2DEX_SELECT_DL          0xB0
#define S2DEX_OBJ_RENDERMODE     0xB1
#define S2DEX_OBJ_RECTANGLE_R    0xB2
#define S2DEX_OBJ_LOADTXTR       0xC1
#define S2DEX_OBJ_LDTX_SPRITE    0xC2
#define S2DEX_OBJ_LDTX_RECT      0xC3
#define S2DEX_OBJ_LDTX_RECT_R    0xC4
#define S2DEX_RDPHALF_0          0xE4

struct uObjScaleBg
{
  uint16_t imageW;     /* Texture width (8-byte alignment, u10.2) */
  uint16_t imageX;     /* x-coordinate of upper-left
				              position of texture (u10.5) */
  uint16_t frameW;     /* Transfer destination frame width (u10.2) */
  int16_t frameX;      /* x-coordinate of upper-left
				              position of transfer destination frame (s10.2) */

  uint16_t imageH;     /* Texture height (u10.2) */
  uint16_t imageY;     /* y-coordinate of upper-left position of
				              texture (u10.5) */
  uint16_t frameH;     /* Transfer destination frame height (u10.2) */
  int16_t frameY;      /* y-coordinate of upper-left position of transfer
				             destination  frame (s10.2) */

  uint32_t imagePtr;   /* Address of texture source in DRAM*/
  uint8_t  imageSiz;   /* Texel size
					           G_IM_SIZ_4b (4 bits/texel)
					           G_IM_SIZ_8b (8 bits/texel)
					           G_IM_SIZ_16b (16 bits/texel)
					           G_IM_SIZ_32b (32 bits/texel) */
  uint8_t  imageFmt;   /* Texel format
					           G_IM_FMT_RGBA (RGBA format)
					           G_IM_FMT_YUV (YUV format)
					           G_IM_FMT_CI (CI format)
					           G_IM_FMT_IA (IA format)
					           G_IM_FMT_I (I format)  */
  uint16_t imageLoad;  /* Method for loading the BG image texture
					           G_BGLT_LOADBLOCK (use LoadBlock)
					           G_BGLT_LOADTILE (use LoadTile) */
  uint16_t imageFlip;  /* Image inversion on/off (horizontal
					           direction only)

					           0 (normal display (no inversion))
					           G_BG_FLAG_FLIPS (horizontal inversion 
                          of texture image) */
  uint16_t imagePal;   /* Position of palette for 4-bit color
				              index texture (4-bit precision, 0~15) */

  uint16_t scaleH;     /* y-direction scale value (u5.10) */
  uint16_t scaleW;     /* x-direction scale value (u5.10) */
  int32_t imageYorig;  /* image drawing origin (s20.5)*/

  uint8_t  padding[4]; /* Padding */
};                     /* 40 bytes */

typedef struct
{
    uint16_t imageW;     /* Texture width (8-byte alignment, u10.2) */
    uint16_t imageX;     /* x-coordinate of upper-left position of texture (u10.5) */ 
    uint16_t frameW;     /* Transfer destination frame width (u10.2) */
    int16_t frameX;      /* X-coordinate of upper-left position of 
                            transfer destination frame (s10.2) */
    uint16_t imageH;     /* Texture height (u10.2) */
    uint16_t imageY;     /* Y-coordinate of upper-left position of 
                            texture (u10.5) */ 
    uint16_t frameH;     /* Transfer destination frame height (u10.2) */
    int16_t frameY;      /* Y-coordinate of upper-left position of 
                           transfer destination frame (s10.2) */

    uint32_t imagePtr;   /* Address of texture source in DRAM*/
    uint8_t  imageSiz;   /* Texel size
                            G_IM_SIZ_4b (4 bits/texel)
                            G_IM_SIZ_8b (8 bits/texel)
                            G_IM_SIZ_16b (16 bits/texel)
                            G_IM_SIZ_32b (32 bits/texel) */
    uint8_t  imageFmt;   /* Texel format
                           G_IM_FMT_RGBA (RGBA format)
                           G_IM_FMT_YUV (YUV format)
                           G_IM_FMT_CI (CI format)
                           G_IM_FMT_IA (IA format)
                           G_IM_FMT_I (I format)  */
    uint16_t imageLoad;  /* Method for loading the BG image texture
                            G_BGLT_LOADBLOCK (use LoadBlock)
                            G_BGLT_LOADTILE (use LoadTile)
                         */
    uint16_t imageFlip;  /* Image inversion on/off (horizontal direction only)
                            0 (normal display (no inversion))
                            G_BG_FLAG_FLIPS (horizontal inversion of 
                            texture image) */
    uint16_t imagePal;   /* Position of palette for 4-bit color 
                            index texture (4-bit precision, 0~15) */

    /* The following is set in the initialization routine guS2DInitBg */

    uint16_t tmemH;      /* TMEM height for a single load (quadruple 
                            value, s13.2) */
    uint16_t tmemW;      /* TMEM width for one frame line (word size) */
    uint16_t tmemLoadTH; /* TH value or Stride value */
    uint16_t tmemLoadSH; /* SH value */
    uint16_t tmemSize;   /* imagePtr skip value for a single load  */
    uint16_t tmemSizeW;  /* imagePtr skip value for one image line */
} uObjBg;                /* 40 bytes */

struct uSprite
{
	uint32_t imagePtr;
	uint32_t tlutPtr;
	int16_t	imageW;
	int16_t	stride;
	int8_t	imageSiz;
	int8_t	imageFmt;
	int16_t	imageH;
	int16_t	imageY;
	int16_t	imageX;
	int8_t	dummy[4];
};                       /* 24 bytes */

struct uObjSprite
{
	uint16_t scaleW;      /* Width-direction scaling (u5.10) */
	int16_t  objX;        /* x-coordinate of upper-left corner of OBJ (s10.2) */
	uint16_t paddingX;    /* Unused (always 0) */
	uint16_t imageW;      /* Texture width (length in s direction, u10.5)  */
	uint16_t scaleH;      /* Height-direction scaling (u5.10) */
	int16_t  objY;        /* y-coordinate of upper-left corner of OBJ (s10.2) */
	uint16_t paddingY;    /* Unused (always 0) */
	uint16_t imageH;      /* Texture height (length in t direction, u10.5)  */
	uint16_t imageAdrs;   /* Texture starting position in TMEM (In units of 64-bit words) */
	uint16_t imageStride; /* Texel wrapping width (In units of 64-bit words) */
	uint8_t  imageFlags;  /* Display flag
				                (*) More than one of the following flags 
                            can be specified as the bit sum of the flags:

					             0 (Normal display (no inversion))
					             G_OBJ_FLAG_FLIPS (s-direction (x) inversion)
					             G_OBJ_FLAG_FLIPT (t-direction (y) inversion)  */
	uint8_t  imagePal;    /* Position of palette for 4-bit 
                            color index texture  (4-bit precision, 0~7)  */
	uint8_t  imageSiz;    /* Texel size
					             G_IM_SIZ_4b (4 bits/texel)
					             G_IM_SIZ_8b (8 bits/texel)
					             G_IM_SIZ_16b (16 bits/texel)
					             G_IM_SIZ_32b (32 bits/texel) */
	uint8_t  imageFmt;    /* Texel format
					             G_IM_FMT_RGBA (RGBA format)
					             G_IM_FMT_YUV (YUV format)
					             G_IM_FMT_CI (CI format)
					             G_IM_FMT_IA (IA format)
					             G_IM_FMT_I  (I format) */

};                       /* 24 bytes */

typedef struct
{
    uint32_t   type;   /* Structure identifier (G_OBJLT_TXTRBLOCK) */
    uint32_t   image;  /* Texture source address in DRAM (8-byte alignment) */
    uint16_t   tsize;  /* Texture size (specified by GS_TB_TSIZE) */
    uint16_t   tmem;   /* TMEM word address where texture will be loaded (8-byte word) */
    uint16_t   sid;    /* Status ID (multiple of 4: either 0, 4, 8, or 12) */
    uint16_t   tline;  /* Texture line width (specified by GS_TB_TLINE) */
    uint32_t   flag;   /* Status flag */
    uint32_t   mask;   /* Status mask */
} uObjTxtrBlock;       /* 24 bytes */

typedef struct
{
   uint32_t   type;   /* Structure identifier (G_OBJLT_TXTRTILE) */
   uint32_t   image;  /* Texture source address in DRAM (8-byte alignment) */
   uint16_t   twidth; /* Texture width (specified by GS_TT_TWIDTH) */
   uint16_t   tmem;   /* TMEM word address where texture will be loaded (8-byte word) */
   uint16_t   sid;    /* Status ID (multiple of 4: either 0, 4, 8, or 12) */
   uint16_t   theight;/* Texture height (specified by GS_TT_THEIGHT) */
   uint32_t   flag;   /* Status flag */
   uint32_t   mask;   /* Status mask  */
} uObjTxtrTile;       /* 24 bytes */

typedef struct
{
   uint32_t   type;   /* Structure identifier (G_OBJLT_TLUT) */
   uint32_t   image;  /* Texture source address in DRAM */
   uint16_t   pnum;   /* Number of palettes to load - 1 */
   uint16_t   phead;  /* Palette position at start of load (256~511) */
   uint16_t   sid;    /* Status ID (multiple of 4: either 0, 4, 8, or 12) */
   uint16_t   zero;   /* Always assign 0 */
   uint32_t   flag;   /* Status flag */
   uint32_t   mask;   /* Status mask */
} uObjTxtrTLUT;       /* 24 bytes */

typedef union 
{
   uObjTxtrBlock      block;
   uObjTxtrTile       tile;
   uObjTxtrTLUT       tlut;
} uObjTxtr;

struct uObjTxSprite 
{
   uObjTxtr             txtr;
   struct uObjSprite    sprite;
};

typedef struct
{
   int32_t A, B, C, D;  /* s15.16 */
   int16_t Y, X;        /* s10.2 */
   uint16_t BaseScaleY; /* u5.10 */
   uint16_t BaseScaleX; /* u5.10 */
} uObjMtx;

typedef struct
{
   int16_t Y, X;		   /* s10.2  */
   uint16_t BaseScaleY;	/* u5.10  */
   uint16_t BaseScaleX;	/* u5.10  */
} uObjSubMtx;

#ifdef __cplusplus
}
#endif

#endif
