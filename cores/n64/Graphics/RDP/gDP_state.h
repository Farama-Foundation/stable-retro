#ifndef _GDP_STATE_H
#define _GDP_STATE_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

struct gDPCombine
{
	union
	{
		struct
		{
			// muxs1
			unsigned	aA1		: 3;
			unsigned	sbA1	: 3;
			unsigned	aRGB1	: 3;
			unsigned	aA0		: 3;
			unsigned	sbA0	: 3;
			unsigned	aRGB0	: 3;
			unsigned	mA1		: 3;
			unsigned	saA1	: 3;
			unsigned	sbRGB1	: 4;
			unsigned	sbRGB0	: 4;

			// muxs0
			unsigned	mRGB1	: 5;
			unsigned	saRGB1	: 4;
			unsigned	mA0		: 3;
			unsigned	saA0	: 3;
			unsigned	mRGB0	: 5;
			unsigned	saRGB0	: 4;
		};

		struct
		{
			uint32_t			muxs1, muxs0;
		};

		uint64_t				mux;
	};
};

struct FrameBuffer;
struct gDPTile
{
   uint32_t   width;
   uint32_t  height;

	uint32_t  format;               /* e.g. RGBA, YUV, etc. */
   uint32_t    size;               /* e.g. 4/8/16/32bpp    */
   uint32_t    line;             
   uint32_t palette;               /* 0..15 - palette index */
   uint32_t    tmem;               /* Texture memory location */

	union
	{
		struct
		{
			unsigned	mirrort	: 1;
			unsigned	clampt	: 1;
			unsigned	pad0	: 30;

			unsigned	mirrors	: 1;
			unsigned	clamps	: 1;
			unsigned	pad1	: 30;
		};

		struct
		{
			uint32_t cmt, cms;
		};
	};

	uint32_t maskt, masks;
	uint32_t shiftt, shifts;
	float fuls, fult, flrs, flrt;
	uint32_t uls;                 /* Upper left S - 8:3 */
   uint32_t ult;                 /* Upper Left T - 8:3 */
   uint32_t lrs;                 /* Lower Right S      */
   uint32_t lrt;                 /* Lower Right T      */

	uint32_t textureMode;
	uint32_t loadType;
	uint32_t imageAddress;
	struct FrameBuffer *frameBuffer;
};

struct gDPLoadTileInfo
{
   uint8_t size;
   uint8_t loadType;
   uint16_t uls;
   uint16_t ult;
   uint16_t width;
   uint16_t height;
   uint16_t texWidth;
   uint32_t texAddress;
   uint32_t dxt;
};

struct gDPOtherMode
{
   union
   {
      struct
      {
         unsigned int alphaCompare    : 2;      /* 0..1   - alpha_compare */
         unsigned int depthSource     : 1;      /* 2..2   - depth_source  */

         //				struct
         //				{
         unsigned int AAEnable     : 1;      /* 3      - aa_en      */
         unsigned int depthCompare : 1;      /* 4      - z_cmp      */
         unsigned int depthUpdate  : 1;      /* 5      - z_upd      */
         unsigned int imageRead    : 1;      /* 6      - im_rd      */
         unsigned int clearOnCvg   : 1;      /* 7      - clr_on_cvg */

         unsigned int cvgDest      : 2;      /* 8..9   - cvg_dst    */
         unsigned int depthMode    : 2;      /* 10..11 - zmode      */

         unsigned int cvgXAlpha    : 1;      /* 12     - cvg_x_alpha */
         unsigned int alphaCvgSel  : 1;      /* 13     - alpha_cvg_sel */
         unsigned int forceBlender : 1;      /* 14     - force_bl   */
         unsigned int textureEdge  : 1;      /* 15     - tex_edge - not used */
         //				} renderMode;

         //				struct
         //				{
         unsigned int c2_m2b : 2;
         unsigned int c1_m2b : 2;
         unsigned int c2_m2a : 2;
         unsigned int c1_m2a : 2;
         unsigned int c2_m1b : 2;
         unsigned int c1_m1b : 2;
         unsigned int c2_m1a : 2;
         unsigned int c1_m1a : 2;
         //				} blender;                             /* 16..31 - blender */

         unsigned int blendMask   : 4;          /* 0..3   - blend_mask */
         unsigned int alphaDither : 2;          /* 4..5   - alpha_dither */
         unsigned int colorDither : 2;          /* 6..7   - rgb_dither */

         unsigned int combineKey     : 1;       /* 8..8   - key_en */
         unsigned int textureConvert : 3;       /* 9..11  - text_conv */
         unsigned int textureFilter  : 2;       /* 12..13 - text_filt */
         unsigned int textureLUT     : 2;       /* 14..15 - text_tlut */

         unsigned int textureLOD        : 1;    /* 16..16 - text_lod */
         unsigned int textureSharpen    : 1;    /* 17..18 - text_sharpen */
         unsigned int textureDetail     : 1;    /* 17..18 - text_sharpen */
         unsigned int texturePersp      : 1;    /* 19..19 - text_persp */
         unsigned int cycleType         : 2;    /* 20..21 - cycle_type */
         unsigned int unusedColorDither : 1;    /* 22..22 - unsupported */
         unsigned int pipelineMode      : 1;    /* 23..23 - atomic_prim */

         unsigned int pad : 8;                  /* 24..31 - padding */

      };

      uint64_t			_u64;
      uint32_t       _u32[2];

      struct
      {
         uint32_t			l, h;
      };
   };
};

struct gDPInfo
{
   struct gDPOtherMode otherMode;
	struct gDPCombine combine;
	struct gDPTile tiles[8], *loadTile;

	struct Color
	{
		float r, g, b, a;
	} fogColor,  blendColor, envColor;

	struct
	{
		float z, dz;
		uint32_t color;
	} fillColor;

	struct PrimColor
	{
      float r, g, b, a;
		float l, m;
	} primColor;

	struct
	{
		float z, deltaZ;
	} primDepth;

	struct
	{
		uint32_t format, size, width, bpl;
		uint32_t address;
	} textureImage;

	struct
	{
		uint32_t format, size, width, height, bpl;
		uint32_t address, changed;
		uint32_t depthImage;
	} colorImage;

	uint32_t	depthImageAddress;

	struct
	{
		uint32_t mode;
		float ulx, uly, lrx, lry;
	} scissor;

	struct
	{
		int32_t k0, k1, k2, k3, k4, k5;
	} convert;

	struct
	{
		struct Color center, scale, width;
	} key;

   struct
   {
      uint32_t width, height;
   }  texRect;

	uint32_t changed;

	uint16_t TexFilterPalette[512];
	uint32_t paletteCRC16[16];
	uint32_t paletteCRC256;
	uint32_t half_1, half_2;

   struct gDPLoadTileInfo loadInfo[512];
};

extern struct gDPInfo gDP;

#ifdef __cplusplus
}
#endif

#endif
