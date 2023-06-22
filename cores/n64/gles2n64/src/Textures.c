#include <stdlib.h>
#include <memory.h>
#include <time.h>

#include <retro_miscellaneous.h>

#include "Common.h"
#include "Config.h"
#include "OpenGL.h"
#include "Textures.h"
#include "GBI.h"
#include "RSP.h"
#include "gDP.h"
#include "gSP.h"
#include "N64.h"
#include "CRC.h"
#include "convert.h"
#include "FrameBuffer.h"

#include "../../Graphics/RDP/gDP_state.h"
#include "../../Graphics/image_convert.h"

#define FORMAT_NONE     0
#define FORMAT_I8       1
#define FORMAT_IA88     2
#define FORMAT_RGBA4444 3
#define FORMAT_RGBA5551 4
#define FORMAT_RGBA8888 5

#ifdef __GNUC__
# define likely(x) __builtin_expect((x),1)
# define unlikely(x) __builtin_expect((x),0)
# define prefetch(x, y) __builtin_prefetch((x),(y))
#else
# define likely(x) (x)
# define unlikely(x) (x)
# define prefetch(x, y)
#endif

TextureCache    cache;

static INLINE uint32_t GetNone( uint64_t *src, uint16_t x, uint16_t i, uint8_t palette )
{
	return 0x00000000;
}

static INLINE uint32_t GetCI4IA_RGBA4444( uint64_t *src, uint16_t x, uint16_t i, uint8_t palette )
{
	uint8_t color4B = ((uint8_t*)src)[(x>>1)^(i<<1)];

	if (x & 1)
		return IA88_RGBA4444( *(uint16_t*)&TMEM[256 + (palette << 4) + (color4B & 0x0F)] );
   return IA88_RGBA4444( *(uint16_t*)&TMEM[256 + (palette << 4) + (color4B >> 4)] );
}

static INLINE uint32_t GetCI4IA_RGBA8888( uint64_t *src, uint16_t x, uint16_t i, uint8_t palette )
{
	uint8_t color4B = ((uint8_t*)src)[(x>>1)^(i<<1)];

	if (x & 1)
		return IA88_RGBA8888( *(uint16_t*)&TMEM[256 + (palette << 4) + (color4B & 0x0F)] );
   return IA88_RGBA8888( *(uint16_t*)&TMEM[256 + (palette << 4) + (color4B >> 4)] );
}

static INLINE uint32_t GetCI4RGBA_RGBA5551( uint64_t *src, uint16_t x, uint16_t i, uint8_t palette )
{
	uint8_t color4B = ((uint8_t*)src)[(x>>1)^(i<<1)];

	if (x & 1)
		return RGBA5551_RGBA5551( *(uint16_t*)&TMEM[256 + (palette << 4) + (color4B & 0x0F)] );
   return RGBA5551_RGBA5551( *(uint16_t*)&TMEM[256 + (palette << 4) + (color4B >> 4)] );
}

static INLINE uint32_t GetCI4RGBA_RGBA8888( uint64_t *src, uint16_t x, uint16_t i, uint8_t palette )
{
	uint8_t color4B = ((uint8_t*)src)[(x>>1)^(i<<1)];

	if (x & 1)
		return RGBA5551_RGBA8888( *(uint16_t*)&TMEM[256 + (palette << 4) + (color4B & 0x0F)] );
   return RGBA5551_RGBA8888( *(uint16_t*)&TMEM[256 + (palette << 4) + (color4B >> 4)] );
}

static INLINE uint32_t GetIA31_RGBA8888( uint64_t *src, uint16_t x, uint16_t i, uint8_t palette )
{
	uint8_t color4B = ((uint8_t*)src)[(x>>1)^(i<<1)];

	return IA31_RGBA8888( (x & 1) ? (color4B & 0x0F) : (color4B >> 4) );
}

static INLINE uint32_t GetIA31_RGBA4444( uint64_t *src, uint16_t x, uint16_t i, uint8_t palette )
{
	uint8_t color4B = ((uint8_t*)src)[(x>>1)^(i<<1)];

	return IA31_RGBA4444( (x & 1) ? (color4B & 0x0F) : (color4B >> 4) );
}

static INLINE uint32_t GetI4_RGBA8888( uint64_t *src, uint16_t x, uint16_t i, uint8_t palette )
{
	uint8_t color4B = ((uint8_t*)src)[(x>>1)^(i<<1)];

	return I4_RGBA8888( (x & 1) ? (color4B & 0x0F) : (color4B >> 4) );
}

static INLINE uint32_t GetI4_RGBA4444( uint64_t *src, uint16_t x, uint16_t i, uint8_t palette )
{
	uint8_t color4B = ((uint8_t*)src)[(x>>1)^(i<<1)];

	return I4_RGBA4444( (x & 1) ? (color4B & 0x0F) : (color4B >> 4) );
}

static INLINE uint32_t GetCI8IA_RGBA4444( uint64_t *src, uint16_t x, uint16_t i, uint8_t palette )
{
	return IA88_RGBA4444( *(uint16_t*)&TMEM[256 + ((uint8_t*)src)[x^(i<<1)]] );
}

static INLINE uint32_t GetCI8IA_RGBA8888( uint64_t *src, uint16_t x, uint16_t i, uint8_t palette )
{
	return IA88_RGBA8888( *(uint16_t*)&TMEM[256 + ((uint8_t*)src)[x^(i<<1)]] );
}

static INLINE uint32_t GetCI8RGBA_RGBA5551( uint64_t *src, uint16_t x, uint16_t i, uint8_t palette )
{
	return RGBA5551_RGBA5551( *(uint16_t*)&TMEM[256 + ((uint8_t*)src)[x^(i<<1)]] );
}

static INLINE uint32_t GetCI8RGBA_RGBA8888( uint64_t *src, uint16_t x, uint16_t i, uint8_t palette )
{
	return RGBA5551_RGBA8888( *(uint16_t*)&TMEM[256 + ((uint8_t*)src)[x^(i<<1)]] );
}

static INLINE uint32_t GetIA44_RGBA8888( uint64_t *src, uint16_t x, uint16_t i, uint8_t palette )
{
	return IA44_RGBA8888(((uint8_t*)src)[x^(i<<1)]);
}

static INLINE uint32_t GetIA44_RGBA4444( uint64_t *src, uint16_t x, uint16_t i, uint8_t palette )
{
	return IA44_RGBA4444(((uint8_t*)src)[x^(i<<1)]);
}

static INLINE uint32_t GetI8_RGBA8888( uint64_t *src, uint16_t x, uint16_t i, uint8_t palette )
{
	return I8_RGBA8888(((uint8_t*)src)[x^(i<<1)]);
}

static INLINE uint32_t GetI8_RGBA4444( uint64_t *src, uint16_t x, uint16_t i, uint8_t palette )
{
	return I8_RGBA4444(((uint8_t*)src)[x^(i<<1)]);
}

static INLINE uint32_t GetCI16IA_RGBA8888(uint64_t *src, uint16_t x, uint16_t i, uint8_t palette)
{
	const uint16_t tex = ((uint16_t*)src)[x^i];
	const uint16_t col = (*(uint16_t*)&TMEM[256 + (tex >> 8)]);
	const uint16_t c = col >> 8;
	const uint16_t a = col & 0xFF;
	return (a << 24) | (c << 16) | (c << 8) | c;
}

static INLINE uint32_t GetCI16IA_RGBA4444(uint64_t *src, uint16_t x, uint16_t i, uint8_t palette)
{
	const uint16_t tex = ((uint16_t*)src)[x^i];
	const uint16_t col = (*(uint16_t*)&TMEM[256 + (tex >> 8)]);
	const uint16_t c = col >> 12;
	const uint16_t a = col & 0x0F;
	return (a << 12) | (c << 8) | (c << 4) | c;
}

static INLINE uint32_t GetCI16RGBA_RGBA8888(uint64_t *src, uint16_t x, uint16_t i, uint8_t palette)
{
	const uint16_t tex = ((uint16_t*)src)[x^i];
	return RGBA5551_RGBA8888(*(uint16_t*)&TMEM[256 + (tex >> 8)]);
}

static INLINE uint32_t GetCI16RGBA_RGBA5551(uint64_t *src, uint16_t x, uint16_t i, uint8_t palette)
{
	const uint16_t tex = ((uint16_t*)src)[x^i];
	return RGBA5551_RGBA5551(*(uint16_t*)&TMEM[256 + (tex >> 8)]);
}

static INLINE uint32_t GetRGBA5551_RGBA8888(uint64_t *src, uint16_t x, uint16_t i, uint8_t palette)
{
	uint16_t tex = ((uint16_t*)src)[x^i];
	return RGBA5551_RGBA8888(tex);
}

static INLINE uint32_t GetRGBA5551_RGBA5551( uint64_t *src, uint16_t x, uint16_t i, uint8_t palette )
{
	uint16_t tex = ((uint16_t*)src)[x^i];
	return RGBA5551_RGBA5551(tex);
}

static INLINE uint32_t GetIA88_RGBA8888( uint64_t *src, uint16_t x, uint16_t i, uint8_t palette )
{
	return IA88_RGBA8888(((uint16_t*)src)[x^i]);
}

static INLINE uint32_t GetIA88_RGBA4444( uint64_t *src, uint16_t x, uint16_t i, uint8_t palette )
{
	return IA88_RGBA4444(((uint16_t*)src)[x^i]);
}

static INLINE uint32_t GetRGBA8888_RGBA8888( uint64_t *src, uint16_t x, uint16_t i, uint8_t palette )
{
	return ((uint32_t*)src)[x^i];
}

static INLINE uint32_t GetRGBA8888_RGBA4444( uint64_t *src, uint16_t x, uint16_t i, uint8_t palette )
{
	return RGBA8888_RGBA4444(((uint32_t*)src)[x^i]);
}


uint16_t YUV_RGBA4444(uint8_t y, uint8_t u, uint8_t v)
{
	return RGBA8888_RGBA4444(YUVtoRGBA8888(y, u, v));
}

static INLINE void GetYUV_RGBA8888(uint64_t * src, uint32_t * dst, uint16_t x)
{
   const uint32_t t = (((uint32_t*)src)[x]);
   uint8_t y1       = (uint8_t)t & 0xFF;
   uint8_t v        = (uint8_t)(t >> 8) & 0xFF;
   uint8_t y0       = (uint8_t)(t >> 16) & 0xFF;
   uint8_t u        = (uint8_t)(t >> 24) & 0xFF;
   uint32_t c       = YUVtoRGBA8888(y0, u, v);
   *(dst++)         = c;
   c                = YUVtoRGBA8888(y1, u, v);
   *(dst++)         = c;
}

static INLINE void GetYUV_RGBA4444(uint64_t * src, uint16_t * dst, uint16_t x)
{
	const uint32_t t = (((uint32_t*)src)[x]);
	uint8_t y1 = (uint8_t)t & 0xFF;
	uint8_t v = (uint8_t)(t >> 8) & 0xFF;
	uint8_t y0 = (uint8_t)(t >> 16) & 0xFF;
	uint8_t u = (uint8_t)(t >> 24) & 0xFF;
	uint16_t c = YUV_RGBA4444(y0, u, v);
	*(dst++) = c;
	c = YUV_RGBA4444(y1, u, v);
	*(dst++) = c;
}

const struct TextureLoadParameters
{
	GetTexelFunc	Get16;
	GLenum			glType16;
	GLint			   glInternalFormat16;
	GetTexelFunc	Get32;
	GLenum			glType32;
	GLint			   glInternalFormat32;
	uint32_t				autoFormat, lineShift, maxTexels;
} imageFormat[4][4][5] =
{ // G_TT_NONE
	{ //		Get16					glType16						glInternalFormat16	Get32					glType32						glInternalFormat32	autoFormat
		{ /* 4-bit */
			{ GetI4_RGBA4444, GL_UNSIGNED_SHORT_4_4_4_4, GL_RGBA4, GetI4_RGBA8888, GL_UNSIGNED_BYTE, GL_RGBA, GL_RGBA4, 4, 8192 }, // RGBA as I
			{ GetNone, GL_UNSIGNED_SHORT_4_4_4_4, GL_RGBA4, GetNone, GL_UNSIGNED_BYTE, GL_RGBA, GL_RGBA4, 4, 8192 }, // YUV
			{ GetI4_RGBA4444, GL_UNSIGNED_SHORT_4_4_4_4, GL_RGBA4, GetI4_RGBA8888, GL_UNSIGNED_BYTE, GL_RGBA, GL_RGBA4, 4, 8192 }, // CI without palette
			{ GetIA31_RGBA4444, GL_UNSIGNED_SHORT_4_4_4_4, GL_RGBA4, GetIA31_RGBA8888, GL_UNSIGNED_BYTE, GL_RGBA, GL_RGBA4, 4, 8192 }, // IA
			{ GetI4_RGBA4444, GL_UNSIGNED_SHORT_4_4_4_4, GL_RGBA4, GetI4_RGBA8888, GL_UNSIGNED_BYTE, GL_RGBA, GL_RGBA4, 4, 8192 }, // I
		},
		{ /* 8-bit */
			{ GetI8_RGBA4444, GL_UNSIGNED_SHORT_4_4_4_4, GL_RGBA4, GetI8_RGBA8888, GL_UNSIGNED_BYTE, GL_RGBA, GL_RGBA, 3, 4096 }, // RGBA as I
			{ GetNone, GL_UNSIGNED_SHORT_4_4_4_4, GL_RGBA4, GetNone, GL_UNSIGNED_BYTE, GL_RGBA, GL_RGBA4, 0, 4096 }, // YUV
			{ GetI8_RGBA4444, GL_UNSIGNED_SHORT_4_4_4_4, GL_RGBA4, GetI8_RGBA8888, GL_UNSIGNED_BYTE, GL_RGBA, GL_RGBA, 3, 4096 }, // CI without palette
			{ GetIA44_RGBA4444, GL_UNSIGNED_SHORT_4_4_4_4, GL_RGBA4, GetIA44_RGBA8888, GL_UNSIGNED_BYTE, GL_RGBA, GL_RGBA4, 3, 4096 }, // IA
			{ GetI8_RGBA4444, GL_UNSIGNED_SHORT_4_4_4_4, GL_RGBA4, GetI8_RGBA8888, GL_UNSIGNED_BYTE, GL_RGBA, GL_RGBA, 3, 4096 }, // I
		},
		{ /* 16-bit */
			{ GetRGBA5551_RGBA5551, GL_UNSIGNED_SHORT_5_5_5_1, GL_RGB5_A1, GetRGBA5551_RGBA8888, GL_UNSIGNED_BYTE, GL_RGBA, GL_RGB5_A1, 2, 2048 }, // RGBA
			{ GetNone, GL_UNSIGNED_SHORT_4_4_4_4, GL_RGBA4, GetNone, GL_UNSIGNED_BYTE, GL_RGBA, GL_RGBA4, 2, 2048 }, // YUV
			{ GetIA88_RGBA4444, GL_UNSIGNED_SHORT_4_4_4_4, GL_RGBA4, GetIA88_RGBA8888, GL_UNSIGNED_BYTE, GL_RGBA, GL_RGBA, 2, 2048 }, // CI as IA
			{ GetIA88_RGBA4444, GL_UNSIGNED_SHORT_4_4_4_4, GL_RGBA4, GetIA88_RGBA8888, GL_UNSIGNED_BYTE, GL_RGBA, GL_RGBA, 2, 2048 }, // IA
			{ GetNone, GL_UNSIGNED_SHORT_4_4_4_4, GL_RGBA4, GetNone, GL_UNSIGNED_BYTE, GL_RGBA, GL_RGBA4, 0, 2048 }, // I
		},
		{ /* 32-bit */
			{ GetRGBA8888_RGBA4444, GL_UNSIGNED_SHORT_4_4_4_4, GL_RGBA4, GetRGBA8888_RGBA8888, GL_UNSIGNED_BYTE, GL_RGBA, GL_RGBA, 2, 1024 }, // RGBA
			{ GetNone, GL_UNSIGNED_SHORT_4_4_4_4, GL_RGBA4, GetNone, GL_UNSIGNED_BYTE, GL_RGBA, GL_RGBA4, 0, 1024 }, // YUV
			{ GetNone, GL_UNSIGNED_SHORT_4_4_4_4, GL_RGBA4, GetNone, GL_UNSIGNED_BYTE, GL_RGBA, GL_RGBA4, 0, 1024 }, // CI
			{ GetNone, GL_UNSIGNED_SHORT_4_4_4_4, GL_RGBA4, GetNone, GL_UNSIGNED_BYTE, GL_RGBA, GL_RGBA4, 0, 1024 }, // IA
			{ GetNone, GL_UNSIGNED_SHORT_4_4_4_4, GL_RGBA4, GetNone, GL_UNSIGNED_BYTE, GL_RGBA, GL_RGBA4, 0, 1024 }, // I
		}
	},
	// DUMMY
	{ //		Get16					glType16						glInternalFormat16	Get32					glType32						glInternalFormat32	autoFormat
		{ // 4-bit
			{ GetCI4RGBA_RGBA5551, GL_UNSIGNED_SHORT_5_5_5_1, GL_RGB5_A1, GetCI4RGBA_RGBA8888, GL_UNSIGNED_BYTE, GL_RGBA, GL_RGB5_A1, 4, 4096 }, // CI (Banjo-Kazooie uses this, doesn't make sense, but it works...)
			{ GetNone, GL_UNSIGNED_SHORT_4_4_4_4, GL_RGBA4, GetNone, GL_UNSIGNED_BYTE, GL_RGBA, GL_RGBA4, 4, 8192 }, // YUV
			{ GetCI4RGBA_RGBA5551, GL_UNSIGNED_SHORT_5_5_5_1, GL_RGB5_A1, GetCI4RGBA_RGBA8888, GL_UNSIGNED_BYTE, GL_RGBA, GL_RGB5_A1, 4, 4096 }, // CI
			{ GetCI4RGBA_RGBA5551, GL_UNSIGNED_SHORT_5_5_5_1, GL_RGB5_A1, GetCI4RGBA_RGBA8888, GL_UNSIGNED_BYTE, GL_RGBA, GL_RGB5_A1, 4, 4096 }, // IA as CI
			{ GetCI4RGBA_RGBA5551, GL_UNSIGNED_SHORT_5_5_5_1, GL_RGB5_A1, GetCI4RGBA_RGBA8888, GL_UNSIGNED_BYTE, GL_RGBA, GL_RGB5_A1, 4, 4096 }, // I as CI
		},
		{ // 8-bit
			{ GetCI8RGBA_RGBA5551, GL_UNSIGNED_SHORT_5_5_5_1, GL_RGB5_A1, GetCI8RGBA_RGBA8888, GL_UNSIGNED_BYTE, GL_RGBA, GL_RGB5_A1, 3, 2048 }, // RGBA
			{ GetNone, GL_UNSIGNED_SHORT_4_4_4_4, GL_RGBA4, GetNone, GL_UNSIGNED_BYTE, GL_RGBA, GL_RGBA4, 0, 4096 }, // YUV
			{ GetCI8RGBA_RGBA5551, GL_UNSIGNED_SHORT_5_5_5_1, GL_RGB5_A1, GetCI8RGBA_RGBA8888, GL_UNSIGNED_BYTE, GL_RGBA, GL_RGB5_A1, 3, 2048 }, // CI
			{ GetCI8RGBA_RGBA5551, GL_UNSIGNED_SHORT_5_5_5_1, GL_RGB5_A1, GetCI8RGBA_RGBA8888, GL_UNSIGNED_BYTE, GL_RGBA, GL_RGB5_A1, 3, 2048 }, // IA as CI
			{ GetCI8RGBA_RGBA5551, GL_UNSIGNED_SHORT_5_5_5_1, GL_RGB5_A1, GetCI8RGBA_RGBA8888, GL_UNSIGNED_BYTE, GL_RGBA, GL_RGB5_A1, 3, 2048 }, // I as CI
		},
		{ // 16-bit
			{ GetCI16RGBA_RGBA5551, GL_UNSIGNED_SHORT_5_5_5_1, GL_RGB5_A1, GetRGBA5551_RGBA8888, GL_UNSIGNED_BYTE, GL_RGBA, GL_RGB5_A1, 2, 2048 }, // RGBA
			{ GetNone, GL_UNSIGNED_SHORT_4_4_4_4, GL_RGBA4, GetNone, GL_UNSIGNED_BYTE, GL_RGBA, GL_RGBA4, 2, 2048 }, // YUV
			{ GetNone, GL_UNSIGNED_SHORT_4_4_4_4, GL_RGBA4, GetNone, GL_UNSIGNED_BYTE, GL_RGBA, GL_RGBA4, 0, 2048 }, // CI
			{ GetCI16RGBA_RGBA5551, GL_UNSIGNED_SHORT_5_5_5_1, GL_RGB5_A1, GetCI16RGBA_RGBA8888, GL_UNSIGNED_BYTE, GL_RGBA, GL_RGB5_A1, 2, 2048 }, // IA as CI
			{ GetNone, GL_UNSIGNED_SHORT_4_4_4_4, GL_RGBA4, GetNone, GL_UNSIGNED_BYTE, GL_RGBA, GL_RGBA4, 0, 2048 }, // I
		},
		{ // 32-bit
			{ GetRGBA8888_RGBA4444, GL_UNSIGNED_SHORT_4_4_4_4, GL_RGBA4, GetRGBA8888_RGBA8888, GL_UNSIGNED_BYTE, GL_RGBA, GL_RGBA, 2, 1024 }, // RGBA
			{ GetNone, GL_UNSIGNED_SHORT_4_4_4_4, GL_RGBA4, GetNone, GL_UNSIGNED_BYTE, GL_RGBA, GL_RGBA4, 0, 1024 }, // YUV
			{ GetNone, GL_UNSIGNED_SHORT_4_4_4_4, GL_RGBA4, GetNone, GL_UNSIGNED_BYTE, GL_RGBA, GL_RGBA4, 0, 1024 }, // CI
			{ GetNone, GL_UNSIGNED_SHORT_4_4_4_4, GL_RGBA4, GetNone, GL_UNSIGNED_BYTE, GL_RGBA, GL_RGBA4, 0, 1024 }, // IA
			{ GetNone, GL_UNSIGNED_SHORT_4_4_4_4, GL_RGBA4, GetNone, GL_UNSIGNED_BYTE, GL_RGBA, GL_RGBA4, 0, 1024 }, // I
		}
	},
	// G_TT_RGBA16
	{ //		Get16					glType16						glInternalFormat16	Get32					glType32						glInternalFormat32	autoFormat
		{ // 4-bit
			{ GetCI4RGBA_RGBA5551, GL_UNSIGNED_SHORT_5_5_5_1, GL_RGB5_A1, GetCI4RGBA_RGBA8888, GL_UNSIGNED_BYTE, GL_RGBA, GL_RGB5_A1, 4, 4096 }, // CI (Banjo-Kazooie uses this, doesn't make sense, but it works...)
			{ GetNone, GL_UNSIGNED_SHORT_4_4_4_4, GL_RGBA4, GetNone, GL_UNSIGNED_BYTE, GL_RGBA, GL_RGBA4, 4, 8192 }, // YUV
			{ GetCI4RGBA_RGBA5551, GL_UNSIGNED_SHORT_5_5_5_1, GL_RGB5_A1, GetCI4RGBA_RGBA8888, GL_UNSIGNED_BYTE, GL_RGBA, GL_RGB5_A1, 4, 4096 }, // CI
			{ GetCI4RGBA_RGBA5551, GL_UNSIGNED_SHORT_5_5_5_1, GL_RGB5_A1, GetCI4RGBA_RGBA8888, GL_UNSIGNED_BYTE, GL_RGBA, GL_RGB5_A1, 4, 4096 }, // IA as CI
			{ GetCI4RGBA_RGBA5551, GL_UNSIGNED_SHORT_5_5_5_1, GL_RGB5_A1, GetCI4RGBA_RGBA8888, GL_UNSIGNED_BYTE, GL_RGBA, GL_RGB5_A1, 4, 4096 }, // I as CI
		},
		{ // 8-bit
			{ GetCI8RGBA_RGBA5551, GL_UNSIGNED_SHORT_5_5_5_1, GL_RGB5_A1, GetCI8RGBA_RGBA8888, GL_UNSIGNED_BYTE, GL_RGBA, GL_RGB5_A1, 3, 2048 }, // RGBA
			{ GetNone, GL_UNSIGNED_SHORT_4_4_4_4, GL_RGBA4, GetNone, GL_UNSIGNED_BYTE, GL_RGBA, GL_RGBA4, 0, 4096 }, // YUV
			{ GetCI8RGBA_RGBA5551, GL_UNSIGNED_SHORT_5_5_5_1, GL_RGB5_A1, GetCI8RGBA_RGBA8888, GL_UNSIGNED_BYTE, GL_RGBA, GL_RGB5_A1, 3, 2048 }, // CI
			{ GetCI8RGBA_RGBA5551, GL_UNSIGNED_SHORT_5_5_5_1, GL_RGB5_A1, GetCI8RGBA_RGBA8888, GL_UNSIGNED_BYTE, GL_RGBA, GL_RGB5_A1, 3, 2048 }, // IA as CI
			{ GetCI8RGBA_RGBA5551, GL_UNSIGNED_SHORT_5_5_5_1, GL_RGB5_A1, GetCI8RGBA_RGBA8888, GL_UNSIGNED_BYTE, GL_RGBA, GL_RGB5_A1, 3, 2048 }, // I as CI
		},
		{ // 16-bit
			{ GetCI16RGBA_RGBA5551, GL_UNSIGNED_SHORT_5_5_5_1, GL_RGB5_A1, GetRGBA5551_RGBA8888, GL_UNSIGNED_BYTE, GL_RGBA, GL_RGB5_A1, 2, 2048 }, // RGBA
			{ GetNone, GL_UNSIGNED_SHORT_4_4_4_4, GL_RGBA4, GetNone, GL_UNSIGNED_BYTE, GL_RGBA, GL_RGBA4, 2, 2048 }, // YUV
			{ GetNone, GL_UNSIGNED_SHORT_4_4_4_4, GL_RGBA4, GetNone, GL_UNSIGNED_BYTE, GL_RGBA, GL_RGBA4, 0, 2048 }, // CI
			{ GetCI16RGBA_RGBA5551, GL_UNSIGNED_SHORT_5_5_5_1, GL_RGB5_A1, GetCI16RGBA_RGBA8888, GL_UNSIGNED_BYTE, GL_RGBA, GL_RGB5_A1, 2, 2048 }, // IA as CI
			{ GetNone, GL_UNSIGNED_SHORT_4_4_4_4, GL_RGBA4, GetNone, GL_UNSIGNED_BYTE, GL_RGBA, GL_RGBA4, 0, 2048 }, // I
		},
		{ // 32-bit
			{ GetRGBA8888_RGBA4444, GL_UNSIGNED_SHORT_4_4_4_4, GL_RGBA4, GetRGBA8888_RGBA8888, GL_UNSIGNED_BYTE, GL_RGBA, GL_RGBA, 2, 1024 }, // RGBA
			{ GetNone, GL_UNSIGNED_SHORT_4_4_4_4, GL_RGBA4, GetNone, GL_UNSIGNED_BYTE, GL_RGBA, GL_RGBA4, 0, 1024 }, // YUV
			{ GetNone, GL_UNSIGNED_SHORT_4_4_4_4, GL_RGBA4, GetNone, GL_UNSIGNED_BYTE, GL_RGBA, GL_RGBA4, 0, 1024 }, // CI
			{ GetNone, GL_UNSIGNED_SHORT_4_4_4_4, GL_RGBA4, GetNone, GL_UNSIGNED_BYTE, GL_RGBA, GL_RGBA4, 0, 1024 }, // IA
			{ GetNone, GL_UNSIGNED_SHORT_4_4_4_4, GL_RGBA4, GetNone, GL_UNSIGNED_BYTE, GL_RGBA, GL_RGBA4, 0, 1024 }, // I
		}
	},
	// G_TT_IA16
	{ //		Get16					glType16						glInternalFormat16	Get32					glType32						glInternalFormat32	autoFormat
		{ // 4-bit
			{ GetCI4IA_RGBA4444, GL_UNSIGNED_SHORT_4_4_4_4, GL_RGBA4, GetCI4IA_RGBA8888, GL_UNSIGNED_BYTE, GL_RGBA, GL_RGBA4, 4, 4096 }, // IA
			{ GetNone, GL_UNSIGNED_SHORT_4_4_4_4, GL_RGBA4, GetNone, GL_UNSIGNED_BYTE, GL_RGBA, GL_RGBA4, 4, 8192 }, // YUV
			{ GetCI4IA_RGBA4444, GL_UNSIGNED_SHORT_4_4_4_4, GL_RGBA4, GetCI4IA_RGBA8888, GL_UNSIGNED_BYTE, GL_RGBA, GL_RGBA4, 4, 4096 }, // CI
			{ GetCI4IA_RGBA4444, GL_UNSIGNED_SHORT_4_4_4_4, GL_RGBA4, GetCI4IA_RGBA8888, GL_UNSIGNED_BYTE, GL_RGBA, GL_RGBA4, 4, 4096 }, // IA as CI
			{ GetCI4IA_RGBA4444, GL_UNSIGNED_SHORT_4_4_4_4, GL_RGBA4, GetCI4IA_RGBA8888, GL_UNSIGNED_BYTE, GL_RGBA, GL_RGBA4, 4, 4096 }, // I as CI
		},
		{ // 8-bit
			{ GetCI8IA_RGBA4444, GL_UNSIGNED_SHORT_4_4_4_4, GL_RGBA4, GetCI8IA_RGBA8888, GL_UNSIGNED_BYTE, GL_RGBA, GL_RGBA4, 3, 2048 }, // RGBA
			{ GetNone, GL_UNSIGNED_SHORT_4_4_4_4, GL_RGBA4, GetNone, GL_UNSIGNED_BYTE, GL_RGBA, GL_RGBA4, 0, 4096 }, // YUV
			{ GetCI8IA_RGBA4444, GL_UNSIGNED_SHORT_4_4_4_4, GL_RGBA4, GetCI8IA_RGBA8888, GL_UNSIGNED_BYTE, GL_RGBA, GL_RGBA4, 3, 2048 }, // CI
			{ GetCI8IA_RGBA4444, GL_UNSIGNED_SHORT_4_4_4_4, GL_RGBA4, GetCI8IA_RGBA8888, GL_UNSIGNED_BYTE, GL_RGBA, GL_RGBA4, 3, 2048 }, // IA as CI
			{ GetCI8IA_RGBA4444, GL_UNSIGNED_SHORT_4_4_4_4, GL_RGBA4, GetCI8IA_RGBA8888, GL_UNSIGNED_BYTE, GL_RGBA, GL_RGBA4, 3, 2048 }, // I as CI
		},
		{ // 16-bit
			{ GetCI16IA_RGBA4444, GL_UNSIGNED_SHORT_4_4_4_4, GL_RGBA4, GetCI16IA_RGBA8888, GL_UNSIGNED_BYTE, GL_RGBA, GL_RGBA, 2, 2048 }, // RGBA
			{ GetNone, GL_UNSIGNED_SHORT_4_4_4_4, GL_RGBA4, GetNone, GL_UNSIGNED_BYTE, GL_RGBA, GL_RGBA4, 2, 2048 }, // YUV
			{ GetNone, GL_UNSIGNED_SHORT_4_4_4_4, GL_RGBA4, GetNone, GL_UNSIGNED_BYTE, GL_RGBA, GL_RGBA4, 0, 2048 }, // CI
			{ GetCI16IA_RGBA4444, GL_UNSIGNED_SHORT_4_4_4_4, GL_RGBA4, GetCI16IA_RGBA8888, GL_UNSIGNED_BYTE, GL_RGBA, GL_RGBA, 2, 2048 }, // IA
			{ GetNone, GL_UNSIGNED_SHORT_4_4_4_4, GL_RGBA4, GetNone, GL_UNSIGNED_BYTE, GL_RGBA, GL_RGBA4, 0, 2048 }, // I
		},
		{ // 32-bit
			{ GetRGBA8888_RGBA4444, GL_UNSIGNED_SHORT_4_4_4_4, GL_RGBA4, GetRGBA8888_RGBA8888, GL_UNSIGNED_BYTE, GL_RGBA, GL_RGBA, 2, 1024 }, // RGBA
			{ GetNone, GL_UNSIGNED_SHORT_4_4_4_4, GL_RGBA4, GetNone, GL_UNSIGNED_BYTE, GL_RGBA, GL_RGBA4, 0, 1024 }, // YUV
			{ GetNone, GL_UNSIGNED_SHORT_4_4_4_4, GL_RGBA4, GetNone, GL_UNSIGNED_BYTE, GL_RGBA, GL_RGBA4, 0, 1024 }, // CI
			{ GetNone, GL_UNSIGNED_SHORT_4_4_4_4, GL_RGBA4, GetNone, GL_UNSIGNED_BYTE, GL_RGBA, GL_RGBA4, 0, 1024 }, // IA
			{ GetNone, GL_UNSIGNED_SHORT_4_4_4_4, GL_RGBA4, GetNone, GL_UNSIGNED_BYTE, GL_RGBA, GL_RGBA4, 0, 1024 }, // I
		}
	}
};

int isTexCacheInit = 0;

void TextureCache_Init(void)
{
   int x, y, i;
   uint8_t noise[64*64*2];
   uint32_t dummyTexture[16] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

   isTexCacheInit = 1;
   cache.current[0] = NULL;
   cache.current[1] = NULL;
   cache.top = NULL;
   cache.bottom = NULL;
   cache.numCached = 0;
   cache.cachedBytes = 0;

   glPixelStorei(GL_PACK_ALIGNMENT, 1);
   glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
   glGenTextures( 32, cache.glNoiseNames );

   srand((unsigned)time(NULL));
   for (i = 0; i < 32; i++)
   {
      glBindTexture( GL_TEXTURE_2D, cache.glNoiseNames[i] );
      for (y = 0; y < 64; y++)
      {
         for (x = 0; x < 64; x++)
         {
            uint32_t r = rand() & 0xFF;
            noise[y*64*2+x*2] = r;
            noise[y*64*2+x*2+1] = r;
         }
      }
      glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE_ALPHA, 64, 64, 0, GL_LUMINANCE_ALPHA, GL_UNSIGNED_BYTE, noise);
   }

   cache.dummy = TextureCache_AddTop();
   cache.dummy->address = 0;
   cache.dummy->clampS = 1;
   cache.dummy->clampT = 1;
   cache.dummy->clampWidth = 2;
   cache.dummy->clampHeight = 2;
   cache.dummy->crc = 0;
   cache.dummy->format = 0;
   cache.dummy->size = 0;
   cache.dummy->frameBufferTexture = false;
   cache.dummy->width = 2;
   cache.dummy->height = 2;
   cache.dummy->realWidth = 2;
   cache.dummy->realHeight = 2;
   cache.dummy->maskS = 0;
   cache.dummy->maskT = 0;
   cache.dummy->scaleS = 0.5f;
   cache.dummy->scaleT = 0.5f;
   cache.dummy->shiftScaleS = 1.0f;
   cache.dummy->shiftScaleT = 1.0f;
   cache.dummy->textureBytes = 2 * 2 * 4;
   cache.dummy->tMem = 0;

   glBindTexture( GL_TEXTURE_2D, cache.dummy->glName );
   glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA, 2, 2, 0, GL_RGBA, GL_UNSIGNED_BYTE, dummyTexture);

   cache.cachedBytes = cache.dummy->textureBytes;
   TextureCache_ActivateDummy(0);
   TextureCache_ActivateDummy(1);
   CRC_BuildTable();
}

bool TextureCache_Verify(void)
{
   uint16_t i;
   CachedTexture *current;

   i = 0;
   current = cache.top;

   while (current)
   {
      i++;
      current = current->lower;
   }

   if (i != cache.numCached)
      return false;

   i = 0;
   current = cache.bottom;
   while (current)
   {
      i++;
      current = current->higher;
   }

   if (i != cache.numCached)
      return false;

   return true;
}

void TextureCache_RemoveBottom(void)
{
   CachedTexture *newBottom = cache.bottom->higher;

   glDeleteTextures( 1, &cache.bottom->glName );
   cache.cachedBytes -= cache.bottom->textureBytes;

   if (cache.bottom == cache.top)
      cache.top = NULL;

   free( cache.bottom );

   cache.bottom = newBottom;

   if (cache.bottom)
      cache.bottom->lower = NULL;

   cache.numCached--;
}

void TextureCache_Remove( CachedTexture *texture )
{
   if ((texture == cache.bottom) && (texture == cache.top))
   {
      cache.top = NULL;
      cache.bottom = NULL;
   }
   else if (texture == cache.bottom)
   {
      cache.bottom = texture->higher;

      if (cache.bottom)
         cache.bottom->lower = NULL;
   }
   else if (texture == cache.top)
   {
      cache.top = texture->lower;

      if (cache.top)
         cache.top->higher = NULL;
   }
   else
   {
      texture->higher->lower = texture->lower;
      texture->lower->higher = texture->higher;
   }

   glDeleteTextures( 1, &texture->glName );
   cache.cachedBytes -= texture->textureBytes;
   free( texture );

   cache.numCached--;
}

CachedTexture *TextureCache_AddTop(void)
{
   CachedTexture *newtop;
   while (cache.cachedBytes > TEXTURECACHE_MAX)
   {
      if (cache.bottom != cache.dummy)
         TextureCache_RemoveBottom();
      else if (cache.dummy->higher)
         TextureCache_Remove( cache.dummy->higher );
   }

   newtop = (CachedTexture*)malloc( sizeof( CachedTexture ) );

   glGenTextures( 1, &newtop->glName );

   newtop->lower = cache.top;
   newtop->higher = NULL;

   if (cache.top)
      cache.top->higher = newtop;

   if (!cache.bottom)
      cache.bottom = newtop;

   cache.top = newtop;

   cache.numCached++;

   return newtop;
}

void TextureCache_MoveToTop( CachedTexture *newtop )
{
   if (newtop == cache.top)
      return;

   if (newtop == cache.bottom)
   {
      cache.bottom = newtop->higher;
      cache.bottom->lower = NULL;
   }
   else
   {
      newtop->higher->lower = newtop->lower;
      newtop->lower->higher = newtop->higher;
   }

   newtop->higher = NULL;
   newtop->lower = cache.top;
   cache.top->higher = newtop;
   cache.top = newtop;
}

void TextureCache_Destroy(void)
{
   while (cache.bottom)
      TextureCache_RemoveBottom();

   glDeleteTextures( 32, cache.glNoiseNames );
   glDeleteTextures( 1, &cache.dummy->glName  );

   cache.top = NULL;
   cache.bottom = NULL;
}

struct TileSizes
{
	uint32_t maskWidth, clampWidth, width, realWidth;
	uint32_t maskHeight, clampHeight, height, realHeight;
};

static void _calcTileSizes(uint32_t _t, struct TileSizes *_sizes, struct gDPTile * _pLoadTile)
{
   uint32_t lineWidth, lineHeight, maskWidth, maskHeight, width, height;
   struct gDPLoadTileInfo *info;
   uint32_t loadWidth = 0, loadHeight = 0;
   struct gDPTile * pTile = _t < 2 ? gSP.textureTile[_t] : &gDP.tiles[_t];
   const struct TextureLoadParameters *loadParams = (const struct TextureLoadParameters*)
      &imageFormat[gDP.otherMode.textureLUT][pTile->size][pTile->format];
   const uint32_t maxTexels = loadParams->maxTexels;
   const uint32_t tileWidth = ((pTile->lrs - pTile->uls) & 0x03FF) + 1;
   const uint32_t tileHeight = ((pTile->lrt - pTile->ult) & 0x03FF) + 1;

   const bool bUseLoadSizes = _pLoadTile != NULL && _pLoadTile->loadType == LOADTYPE_TILE &&
      (pTile->tmem == _pLoadTile->tmem);

   if (bUseLoadSizes) {
      loadWidth = ((_pLoadTile->lrs - _pLoadTile->uls) & 0x03FF) + 1;
      loadHeight = ((_pLoadTile->lrt - _pLoadTile->ult) & 0x03FF) + 1;
   }



   lineWidth = pTile->line << loadParams->lineShift;
   lineHeight = lineWidth != 0 ? MIN(maxTexels / lineWidth, tileHeight) : 0;

   maskWidth = 1 << pTile->masks;
   maskHeight = 1 << pTile->maskt;

   info = (struct gDPLoadTileInfo*)&gDP.loadInfo[pTile->tmem];
   if (info->loadType == LOADTYPE_TILE) {
      if (pTile->masks && ((maskWidth * maskHeight) <= maxTexels))
         width = maskWidth; // Use mask width if set and valid
      else {
         width = MIN(info->width, info->texWidth);
         if (info->size > pTile->size)
            width <<= info->size - pTile->size;
      }
      if (pTile->maskt && ((maskWidth * maskHeight) <= maxTexels))
         height = maskHeight;
      else
         height = info->height;
   } else {
      if (pTile->masks && ((maskWidth * maskHeight) <= maxTexels))
         width = maskWidth; // Use mask width if set and valid
      else if ((tileWidth * tileHeight) <= maxTexels)
         width = tileWidth; // else use tile width if valid
      else
         width = lineWidth; // else use line-based width

      if (pTile->maskt && ((maskWidth * maskHeight) <= maxTexels))
         height = maskHeight;
      else if ((tileWidth * tileHeight) <= maxTexels)
         height = tileHeight;
      else
         height = lineHeight;
   }

   _sizes->clampWidth = (pTile->clamps && gDP.otherMode.cycleType != G_CYC_COPY) ? tileWidth : width;
   _sizes->clampHeight = (pTile->clampt && gDP.otherMode.cycleType != G_CYC_COPY) ? tileHeight : height;

   if (_sizes->clampWidth > 256)
      pTile->clamps = 0;
   if (_sizes->clampHeight > 256)
      pTile->clampt = 0;

   // Make sure masking is valid
   if (maskWidth > width) {
      pTile->masks = powof(width);
      maskWidth = 1 << pTile->masks;
   }

   if (maskHeight > height) {
      pTile->maskt = powof(height);
      maskHeight = 1 << pTile->maskt;
   }

   _sizes->maskWidth = maskWidth;
   _sizes->maskHeight = maskHeight;
   _sizes->width = width;
   _sizes->height = height;

   if (pTile->clamps != 0)
      _sizes->realWidth = _sizes->clampWidth;
   else if (pTile->masks != 0)
      _sizes->realWidth = _sizes->maskWidth;
   else
      _sizes->realWidth = _sizes->width;

   if (pTile->clampt != 0)
      _sizes->realHeight = _sizes->clampHeight;
   else if (pTile->maskt != 0)
      _sizes->realHeight = _sizes->maskHeight;
   else
      _sizes->realHeight = _sizes->height;

   if (gSP.texture.level > gSP.texture.tile)
   {
      _sizes->realWidth = pow2(_sizes->realWidth);
      _sizes->realHeight = pow2(_sizes->realHeight);
   }
}

static void _loadBackground( CachedTexture *pTexture )
{
	uint32_t *pDest;
	uint8_t *pSwapped, *pSrc;
	uint32_t numBytes, bpl;
	uint32_t x, y, j, tx, ty;
	uint16_t clampSClamp;
	uint16_t clampTClamp;
	GetTexelFunc GetTexel;
	GLuint glInternalFormat;
	GLenum glType;
	bool bLoaded = false;
	const struct TextureLoadParameters *loadParams = (const struct TextureLoadParameters*)
      &imageFormat[pTexture->format == 2 ? G_TT_RGBA16 : G_TT_NONE][pTexture->size][pTexture->format];
	if (loadParams->autoFormat == GL_RGBA) {
		pTexture->textureBytes = (pTexture->realWidth * pTexture->realHeight) << 2;
		GetTexel = loadParams->Get32;
		glInternalFormat = loadParams->glInternalFormat32;
		glType = loadParams->glType32;
	} else {
		pTexture->textureBytes = (pTexture->realWidth * pTexture->realHeight) << 1;
		GetTexel = loadParams->Get16;
		glInternalFormat = loadParams->glInternalFormat16;
		glType = loadParams->glType16;
	}

	bpl = gSP.bgImage.width << gSP.bgImage.size >> 1;
	numBytes = bpl * gSP.bgImage.height;
	pSwapped = (uint8_t*)malloc(numBytes);
	//assert(pSwapped != NULL);
	UnswapCopyWrap(gfx_info.RDRAM, gSP.bgImage.address, pSwapped, 0, RDRAMSize, numBytes);
	pDest = (uint32_t*)malloc(pTexture->textureBytes);
	//assert(pDest != NULL);

	clampSClamp = pTexture->width - 1;
	clampTClamp = pTexture->height - 1;

	j = 0;
	for (y = 0; y < pTexture->realHeight; y++) {
		ty = MIN(y, (uint32_t)clampTClamp);

		pSrc = &pSwapped[bpl * ty];

		for (x = 0; x < pTexture->realWidth; x++) {
			tx = MIN(x, (uint32_t)clampSClamp);

			if (glInternalFormat == GL_RGBA)
				((uint32_t*)pDest)[j++] = GetTexel((uint64_t*)pSrc, tx, 0, pTexture->palette);
			else
				((uint16_t*)pDest)[j++] = GetTexel((uint64_t*)pSrc, tx, 0, pTexture->palette);
		}
	}

#if 0
	if ((config.textureFilter.txEnhancementMode | config.textureFilter.txFilterMode) != 0 && config.textureFilter.txFilterIgnoreBG == 0 && TFH.isInited())
   {
		GHQTexInfo ghqTexInfo;
		if (txfilter_filter((uint8_t*)pDest, pTexture->realWidth, pTexture->realHeight, glInternalFormat, (uint64)pTexture->crc, &ghqTexInfo) != 0 && ghqTexInfo.data != NULL) {
			if (ghqTexInfo.width % 2 != 0 && ghqTexInfo.format != GL_RGBA)
				glPixelStorei(GL_UNPACK_ALIGNMENT, 2);
			glTexImage2D(GL_TEXTURE_2D, 0, ghqTexInfo.format, ghqTexInfo.width, ghqTexInfo.height, 0, ghqTexInfo.texture_format, ghqTexInfo.pixel_type, ghqTexInfo.data);
			_updateCachedTexture(ghqTexInfo, pTexture);
			bLoaded = true;
		}
	}
#endif
	if (!bLoaded)
   {
      if (pTexture->realWidth % 2 != 0 && glInternalFormat != GL_RGBA)
         glPixelStorei(GL_UNPACK_ALIGNMENT, 2);
      glTexImage2D(GL_TEXTURE_2D, 0, glInternalFormat, pTexture->realWidth, pTexture->realHeight, 0, GL_RGBA, glType, pDest);
   }
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	free(pDest);
}

static INLINE void TextureCache_getTextureDestData(CachedTexture *tmptex, uint32_t* pDest, GLuint glInternalFormat, GetTexelFunc GetTexel, uint16_t* pLine)
{
	uint64_t *pSrc;
	uint16_t x, y, i, j, tx, ty;
	uint16_t mirrorSBit, maskSMask, clampSClamp;
	uint16_t mirrorTBit, maskTMask, clampTClamp;

   if (tmptex->maskS > 0) {
      clampSClamp = tmptex->clampS ? tmptex->clampWidth - 1 : (tmptex->mirrorS ? (tmptex->width << 1) - 1 : tmptex->width - 1);
      maskSMask = (1 << tmptex->maskS) - 1;
      mirrorSBit = (tmptex->mirrorS != 0 || tmptex->realWidth/ tmptex->width == 2) ? 1 << tmptex->maskS : 0;
   } else {
      clampSClamp = MIN(tmptex->clampWidth, tmptex->width) - 1;
      maskSMask = 0xFFFF;
      mirrorSBit = 0x0000;
   }

   if (tmptex->maskT > 0) {
      clampTClamp = tmptex->clampT ? tmptex->clampHeight - 1 : (tmptex->mirrorT ? (tmptex->height << 1) - 1 : tmptex->height - 1);
      maskTMask = (1 << tmptex->maskT) - 1;
      mirrorTBit = (tmptex->mirrorT != 0 || tmptex->realHeight / tmptex->height == 2) ? 1 << tmptex->maskT : 0;
   } else {
      clampTClamp = MIN(tmptex->clampHeight, tmptex->height) - 1;
      maskTMask = 0xFFFF;
      mirrorTBit = 0x0000;
   }

   if (tmptex->size == G_IM_SIZ_32b) {
	   int line32;
	   int width;
	   uint16_t gr, ab;
      const uint16_t * tmem16 = (uint16_t*)TMEM;
      const uint32_t tbase = tmptex->tMem << 2;

      int wid_64 = (tmptex->clampWidth) << 2;
      if (wid_64 & 15) wid_64 += 16;
      wid_64 &= 0xFFFFFFF0;
      wid_64 >>= 3;
      line32 = tmptex->line << 1;
      line32 = (line32 - wid_64) << 3;
      if (wid_64 < 1) wid_64 = 1;
      width = wid_64 << 1;
      line32 = width + (line32 >> 2);

      j = 0;
      for (y = 0; y < tmptex->realHeight; ++y) {
		  uint32_t tline, xorval;
         ty = MIN(y, clampTClamp) & maskTMask;
         if (y & mirrorTBit)
            ty ^= maskTMask;

         tline = tbase + line32 * ty;
         xorval = (ty & 1) ? 3 : 1;

         for (x = 0; x < tmptex->realWidth; ++x) {
			 uint32_t taddr;
            tx = MIN(x, clampSClamp) & maskSMask;
            if (x & mirrorSBit)
               tx ^= maskSMask;

            taddr = ((tline + tx) ^ xorval) & 0x3ff;
            gr = swapword(tmem16[taddr]);
            ab = swapword(tmem16[taddr | 0x400]);
            pDest[j++] = (ab << 16) | gr;
         }
      }
   } else if (tmptex->format == G_IM_FMT_YUV) {
      j = 0;
      *pLine <<= 1;
      for (y = 0; y < tmptex->realHeight; ++y) {
         pSrc = &TMEM[tmptex->tMem] + *pLine * y;
         for (x = 0; x < tmptex->realWidth / 2; x++) {
            if (glInternalFormat == GL_RGBA)
               GetYUV_RGBA8888(pSrc, pDest + j, x);
            else
               GetYUV_RGBA4444(pSrc, (uint16_t*)pDest + j, x);
            j += 2;
         }
      }
   }
   else
   {
      uint32_t tMemMask;

      j = 0;
      tMemMask = gDP.otherMode.textureLUT == G_TT_NONE ? 0x1FF : 0xFF;

      for (y = 0; y < tmptex->realHeight; ++y)
      {
         ty = MIN(y, clampTClamp) & maskTMask;

         if (y & mirrorTBit)
            ty ^= maskTMask;

         pSrc = &TMEM[(tmptex->tMem + *pLine * ty) & tMemMask];

         i = (ty & 1) << 1;
         for (x = 0; x < tmptex->realWidth; ++x) {
            tx = MIN(x, clampSClamp) & maskSMask;

            if (x & mirrorSBit)
               tx ^= maskSMask;

            if (glInternalFormat == GL_RGBA)
               pDest[j++] = GetTexel(pSrc, tx, i, tmptex->palette);
            else
               ((uint16_t*)pDest)[j++] = GetTexel(pSrc, tx, i, tmptex->palette);
         }
      }
   }
}

static void _load(uint32_t _tile, CachedTexture *_pTexture)
{
	uint32_t *pDest;
   uint16_t line;
	GetTexelFunc GetTexel;
	GLuint glInternalFormat;
	GLenum glType;
	uint32_t sizeShift;
	uint64_t ricecrc = 0;
	GLint mipLevel = 0, maxLevel = 0;
	CachedTexture tmptex = {0};

	const struct TextureLoadParameters *loadParams = (const struct TextureLoadParameters*)&imageFormat[gDP.otherMode.textureLUT][_pTexture->size][_pTexture->format];
	if (loadParams->autoFormat == GL_RGBA)
   {
		sizeShift = 2;
		_pTexture->textureBytes = (_pTexture->realWidth * _pTexture->realHeight) << sizeShift;
		GetTexel = loadParams->Get32;
		glInternalFormat = loadParams->glInternalFormat32;
		glType = loadParams->glType32;
	}
   else
   {
		sizeShift = 1;
		_pTexture->textureBytes = (_pTexture->realWidth * _pTexture->realHeight) << sizeShift;
		GetTexel = loadParams->Get16;
		glInternalFormat = loadParams->glInternalFormat16;
		glType = loadParams->glType16;
	}

	pDest = (uint32_t*)malloc(_pTexture->textureBytes);


	if (config.generalEmulation.enableLOD != 0 && gSP.texture.level > gSP.texture.tile + 1)
		maxLevel = _tile == 0 ? 0 : gSP.texture.level - gSP.texture.tile - 1;

	_pTexture->max_level = maxLevel;

	memcpy(&tmptex, _pTexture, sizeof(CachedTexture));

	line = tmptex.line;

	while (true)
   {
      struct TileSizes sizes;
      uint32_t tileMipLevel;
      struct gDPTile *mipTile;
      bool bLoaded = false;
      TextureCache_getTextureDestData(&tmptex, pDest, glInternalFormat, GetTexel, &line);

#if 0
      if (m_toggleDumpTex && config.textureFilter.txHiresEnable != 0 && config.textureFilter.txDump != 0) {
         txfilter_dmptx((uint8_t*)pDest, tmptex.realWidth, tmptex.realHeight, tmptex.realWidth, glInternalFormat, (unsigned short)(_pTexture->format << 8 | _pTexture->size), ricecrc);
      }
#endif
#if 0
      else if ((config.textureFilter.txEnhancementMode | config.textureFilter.txFilterMode) != 0 && maxLevel == 0 && (config.textureFilter.txFilterIgnoreBG == 0 || (RSP.cmd != G_TEXRECT && RSP.cmd != G_TEXRECTFLIP)) && TFH.isInited())
      {
         GHQTexInfo ghqTexInfo;
         if (txfilter_filter((uint8_t*)pDest, tmptex.realWidth, tmptex.realHeight, glInternalFormat, (uint64)_pTexture->crc, &ghqTexInfo) != 0 && ghqTexInfo.data != NULL) {
            glTexImage2D(GL_TEXTURE_2D, 0, ghqTexInfo.format, ghqTexInfo.width, ghqTexInfo.height, 0, ghqTexInfo.texture_format, ghqTexInfo.pixel_type, ghqTexInfo.data);
            _updateCachedTexture(ghqTexInfo, _pTexture);
            bLoaded = true;
         }
      }
#endif
      if (!bLoaded)
      {
         if (tmptex.realWidth % 2 != 0 && glInternalFormat != GL_RGBA)
            glPixelStorei(GL_UNPACK_ALIGNMENT, 2);
         glTexImage2D(GL_TEXTURE_2D, mipLevel, glInternalFormat, tmptex.realWidth, tmptex.realHeight, 0, GL_RGBA, glType, pDest);
      }
      if (mipLevel == maxLevel)
         break;
      ++mipLevel;


      tileMipLevel = gSP.texture.tile + mipLevel + 1;
      mipTile = (struct gDPTile*)&gDP.tiles[tileMipLevel];
      line           = mipTile->line;
      tmptex.tMem    = mipTile->tmem;
      tmptex.palette = mipTile->palette;
      tmptex.maskS   = mipTile->masks;
      tmptex.maskT   = mipTile->maskt;
      _calcTileSizes(tileMipLevel, &sizes, NULL);
      tmptex.width = sizes.width;
      tmptex.clampWidth = sizes.clampWidth;
      tmptex.height = sizes.height;
      tmptex.clampHeight = sizes.clampHeight;
      // Ensure mip-map levels size consistency.
      if (tmptex.realWidth > 1)
         tmptex.realWidth >>= 1;
      if (tmptex.realHeight > 1)
         tmptex.realHeight >>= 1;
      _pTexture->textureBytes += (tmptex.realWidth * tmptex.realHeight) << sizeShift;
   }
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	free(pDest);
}

struct TextureParams
{
	uint16_t width;
	uint16_t height;
	uint16_t clampWidth;
	uint16_t clampHeight;
	uint8_t maskS;
	uint8_t maskT;
	uint8_t mirrorS;
	uint8_t mirrorT;
	uint8_t clampS;
	uint8_t clampT;
	uint8_t format;
	uint8_t size;
};

static uint32_t _calculateCRC(uint32_t t, const struct TextureParams *_params)
{
   const uint32_t line = gSP.textureTile[t]->line;
   const uint32_t lineBytes = line << 3;

   const uint64_t *src = (uint64_t*)&TMEM[gSP.textureTile[t]->tmem];
   uint32_t crc = 0xFFFFFFFF;
   crc = Hash_Calculate(crc, src, _params->height*lineBytes);

   if (gSP.textureTile[t]->size == G_IM_SIZ_32b)
   {
      src = (uint64_t*)&TMEM[gSP.textureTile[t]->tmem + 256];
      crc = Hash_Calculate(crc, src, _params->height*lineBytes);
   }

   if (gDP.otherMode.textureLUT != G_TT_NONE || gSP.textureTile[t]->format == G_IM_FMT_CI) {
      if (gSP.textureTile[t]->size == G_IM_SIZ_4b)
         crc = Hash_Calculate( crc, &gDP.paletteCRC16[gSP.textureTile[t]->palette], 4 );
      else if (gSP.textureTile[t]->size == G_IM_SIZ_8b)
         crc = Hash_Calculate( crc, &gDP.paletteCRC256, 4 );
   }

   crc = Hash_Calculate(crc, _params, sizeof(_params));

   return crc;
}

void TextureCache_ActivateTexture( uint32_t t, CachedTexture *_pTexture )
{
   bool bUseBilinear;
   glActiveTexture( GL_TEXTURE0 + t );
   glBindTexture( GL_TEXTURE_2D, _pTexture->glName );

   bUseBilinear = (gDP.otherMode.textureFilter | (gSP.objRendermode&G_OBJRM_BILERP)) != 0;

   if (config.texture.bilinearMode == BILINEAR_STANDARD)
   {
      if (bUseBilinear) {
         if (_pTexture->max_level > 0)
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
         else
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
         glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
      } else {
         if (_pTexture->max_level > 0)
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
         else
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
         glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
      }
   }
   else
   { // 3 point filter
      if (_pTexture->max_level > 0) { // Apply standard bilinear to mipmap textures
         if (bUseBilinear) {
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
         } else {
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
         }
      } else if (bUseBilinear && config.generalEmulation.enableLOD != 0 && gSP.texture.level > gSP.texture.tile) { // Apply standard bilinear to first tile of mipmap texture
         glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
         glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
      } else { // Don't use texture filter. Texture will be filtered by 3 point filter shader
         glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
         glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
      }
   }

#ifndef HAVE_OPENGLES
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, _pTexture->max_level);
#endif

   /* Set clamping modes */
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, _pTexture->clampS ? GL_CLAMP_TO_EDGE : _pTexture->mirrorS ? GL_MIRRORED_REPEAT : GL_REPEAT);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, _pTexture->clampT ? GL_CLAMP_TO_EDGE : _pTexture->mirrorT ? GL_MIRRORED_REPEAT : GL_REPEAT);

   if (OGL.renderState == RS_TRIANGLE && config.texture.maxAnisotropy > 0)
   {
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, config.texture.maxAnisotropy);
   }

   _pTexture->lastDList = __RSP.DList;
   TextureCache_MoveToTop( _pTexture );
   cache.current[t] = _pTexture;
}

void TextureCache_ActivateDummy( uint32_t t)
{
   glActiveTexture(GL_TEXTURE0 + t);
   glBindTexture(GL_TEXTURE_2D, cache.dummy->glName );
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
}

int _background_compare(CachedTexture *current, uint32_t crc)
{
   if ((current != NULL) &&
         (current->crc == crc) &&
         (current->width == gSP.bgImage.width) &&
         (current->height == gSP.bgImage.height) &&
         (current->format == gSP.bgImage.format) &&
         (current->size == gSP.bgImage.size))
      return 1;
   else
      return 0;
}

void _updateBackground(void)
{
   uint32_t numBytes, crc;
   CachedTexture *current;
   CachedTexture *pCurrent;

   numBytes = gSP.bgImage.width * gSP.bgImage.height << gSP.bgImage.size >> 1;
   crc = Hash_Calculate( 0xFFFFFFFF, &gfx_info.RDRAM[gSP.bgImage.address], numBytes );

   if (gDP.otherMode.textureLUT != G_TT_NONE || gSP.bgImage.format == G_IM_FMT_CI)
   {
      if (gSP.bgImage.size == G_IM_SIZ_4b)
         crc = Hash_Calculate( crc, &gDP.paletteCRC16[gSP.bgImage.palette], 4 );
      else if (gSP.bgImage.size == G_IM_SIZ_8b)
         crc = Hash_Calculate( crc, &gDP.paletteCRC256, 4 );
   }

   //before we traverse cache, check to see if texture is already bound:
   if (_background_compare(cache.current[0], crc))
      return;

   current = (CachedTexture*)cache.top;
   while (current)
   {
      if (_background_compare(current, crc))
      {
         TextureCache_ActivateTexture( 0, current );
         cache.hits++;
         return;
      }
      current = current->lower;
   }
   cache.misses++;

   glActiveTexture(GL_TEXTURE0);

   pCurrent = TextureCache_AddTop();

   glBindTexture( GL_TEXTURE_2D, pCurrent->glName );

   pCurrent->address = gSP.bgImage.address;
   pCurrent->crc = crc;

   pCurrent->format = gSP.bgImage.format;
   pCurrent->size = gSP.bgImage.size;

   pCurrent->width = gSP.bgImage.width;
   pCurrent->height = gSP.bgImage.height;

   pCurrent->clampWidth = gSP.bgImage.width;
   pCurrent->clampHeight = gSP.bgImage.height;
   pCurrent->palette = gSP.bgImage.palette;
   pCurrent->maskS = 0;
   pCurrent->maskT = 0;
   pCurrent->mirrorS = 0;
   pCurrent->mirrorT = 0;
   pCurrent->clampS = 0;
   pCurrent->clampT = 0;
   pCurrent->line = 0;
   pCurrent->tMem = 0;
   pCurrent->lastDList = __RSP.DList;
   pCurrent->frameBufferTexture = false;

   pCurrent->realWidth = gSP.bgImage.width;
   pCurrent->realHeight = gSP.bgImage.height;

   pCurrent->scaleS = 1.0f / (float)(pCurrent->realWidth);
   pCurrent->scaleT = 1.0f / (float)(pCurrent->realHeight);

   pCurrent->shiftScaleS = 1.0f;
   pCurrent->shiftScaleT = 1.0f;

   pCurrent->offsetS = 0.5f;
   pCurrent->offsetT = 0.5f;

   _loadBackground( pCurrent );
   TextureCache_ActivateTexture( 0, pCurrent );

   cache.cachedBytes += pCurrent->textureBytes;
   cache.current[0] = pCurrent;
}

int _texture_compare(uint32_t t, CachedTexture *current, uint32_t crc,  uint32_t width, uint32_t height, uint32_t clampWidth, uint32_t clampHeight)
{
   return  ((current != NULL) &&
         (current->crc == crc) &&
         (current->width == width) &&
         (current->height == height) &&
         (current->clampWidth == clampWidth) &&
         (current->clampHeight == clampHeight) &&
         (current->maskS == gSP.textureTile[t]->masks) &&
         (current->maskT == gSP.textureTile[t]->maskt) &&
         (current->mirrorS == gSP.textureTile[t]->mirrors) &&
         (current->mirrorT == gSP.textureTile[t]->mirrort) &&
         (current->clampS == gSP.textureTile[t]->clamps) &&
         (current->clampT == gSP.textureTile[t]->clampt) &&
         (current->format == gSP.textureTile[t]->format) &&
         (current->size == gSP.textureTile[t]->size));
}

static
void _updateShiftScale(uint32_t _t, CachedTexture *_pTexture)
{
	_pTexture->shiftScaleS = 1.0f;
	_pTexture->shiftScaleT = 1.0f;

	if (gSP.textureTile[_t]->shifts > 10)
		_pTexture->shiftScaleS = (float)(1 << (16 - gSP.textureTile[_t]->shifts));
	else if (gSP.textureTile[_t]->shifts > 0)
		_pTexture->shiftScaleS /= (float)(1 << gSP.textureTile[_t]->shifts);

	if (gSP.textureTile[_t]->shiftt > 10)
		_pTexture->shiftScaleT = (float)(1 << (16 - gSP.textureTile[_t]->shiftt));
	else if (gSP.textureTile[_t]->shiftt > 0)
		_pTexture->shiftScaleT /= (float)(1 << gSP.textureTile[_t]->shiftt);
}

void TextureCache_Update( uint32_t _t )
{
   CachedTexture *current;
   CachedTexture *pCurrent;
   uint32_t crc;
   struct TextureParams params;
   struct TileSizes sizes;

   switch (gSP.textureTile[_t]->textureMode)
   {
      case TEXTUREMODE_BGIMAGE:
         _updateBackground();
         return;
      case TEXTUREMODE_FRAMEBUFFER:
         FrameBuffer_ActivateBufferTexture( _t, gSP.textureTile[_t]->frameBuffer );
         return;
      case TEXTUREMODE_FRAMEBUFFER_BG:
         FrameBuffer_ActivateBufferTextureBG( _t, gSP.textureTile[_t]->frameBuffer );
         return;
   }

	if (gDP.otherMode.textureLOD == G_TL_LOD && gSP.texture.level == gSP.texture.tile && _t == 1)
   {
		cache.current[1] = cache.current[0];
		TextureCache_ActivateTexture(_t, cache.current[_t]);
		return;
	}

   if (gSP.texture.tile == 7 && _t == 0 && gSP.textureTile[0] == gDP.loadTile && gDP.loadTile->loadType == LOADTYPE_BLOCK && gSP.textureTile[0]->tmem == gSP.textureTile[1]->tmem)
		gSP.textureTile[0] = gSP.textureTile[1];

   _calcTileSizes(_t, &sizes, gDP.loadTile);

   params.width = sizes.width;
   params.height = sizes.height;
   params.clampWidth = sizes.clampWidth;
   params.clampHeight = sizes.clampHeight;
   params.maskS = gSP.textureTile[_t]->masks;
   params.maskT = gSP.textureTile[_t]->maskt;
   params.mirrorS = gSP.textureTile[_t]->mirrors;
   params.mirrorT = gSP.textureTile[_t]->mirrort;
   params.clampS = gSP.textureTile[_t]->clamps;
   params.clampT = gSP.textureTile[_t]->clampt;
   params.format = gSP.textureTile[_t]->format;
   params.size = gSP.textureTile[_t]->size;

   crc = _calculateCRC(_t, &params );

   //before we traverse cache, check to see if texture is already bound:
   if (_texture_compare(_t, cache.current[_t], crc, params.width, params.height, params.clampWidth, params.clampHeight))
   {
      _updateShiftScale(_t, cache.current[_t]);
      TextureCache_ActivateTexture(_t, cache.current[_t]);
      return;
   }

   current = cache.top;
   while (current)
   {
      if (_texture_compare(_t, current, crc, params.width, params.height, params.clampWidth, params.clampHeight))
      {
         _updateShiftScale(_t, current);
         TextureCache_ActivateTexture(_t, current );
         cache.hits++;
         return;
      }
      current = current->lower;
   }
   cache.misses++;

   glActiveTexture( GL_TEXTURE0 + _t);

   pCurrent = TextureCache_AddTop();

   glBindTexture( GL_TEXTURE_2D, pCurrent->glName );

   pCurrent->address = gDP.textureImage.address;

   pCurrent->crc = crc;

   pCurrent->format = gSP.textureTile[_t]->format;
   pCurrent->size   = gSP.textureTile[_t]->size;

   pCurrent->width = sizes.width;
   pCurrent->height = sizes.height;

   pCurrent->clampWidth = sizes.clampWidth;
   pCurrent->clampHeight = sizes.clampHeight;

   pCurrent->palette = gSP.textureTile[_t]->palette;

   pCurrent->maskS = gSP.textureTile[_t]->masks;
   pCurrent->maskT = gSP.textureTile[_t]->maskt;
   pCurrent->mirrorS = gSP.textureTile[_t]->mirrors;
   pCurrent->mirrorT = gSP.textureTile[_t]->mirrort;
   pCurrent->clampS = gSP.textureTile[_t]->clamps;
   pCurrent->clampT = gSP.textureTile[_t]->clampt;
   pCurrent->line = gSP.textureTile[_t]->line;
   pCurrent->tMem = gSP.textureTile[_t]->tmem;
   pCurrent->lastDList = __RSP.DList;

   pCurrent->realWidth = sizes.realWidth;
   pCurrent->realHeight = sizes.realHeight;

   pCurrent->scaleS = 1.0f / (float)(pCurrent->realWidth);
   pCurrent->scaleT = 1.0f / (float)(pCurrent->realHeight);

   pCurrent->offsetS = 0.5f;
   pCurrent->offsetT = 0.5f;

   pCurrent->shiftScaleS = 1.0f;
   pCurrent->shiftScaleT = 1.0f;

	_updateShiftScale(_t, pCurrent);

   _load(_t, pCurrent );
   TextureCache_ActivateTexture(_t, pCurrent );

   cache.cachedBytes += pCurrent->textureBytes;

   cache.current[_t] = pCurrent;
}

void TextureCache_ActivateNoise(uint32_t t)
{
   glActiveTexture(GL_TEXTURE0 + t);
   glBindTexture(GL_TEXTURE_2D, cache.glNoiseNames[__RSP.DList & 0x1F]);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT );
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT );
}
