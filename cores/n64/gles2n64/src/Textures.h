#ifndef TEXTURES_H
#define TEXTURES_H

#include <retro_inline.h>

#include <glsm/glsmsym.h>

#include "convert.h"

#ifdef __cplusplus
extern "C" {
#endif

struct gDPTile;

typedef uint32_t (*GetTexelFunc)( uint64_t *src, uint16_t x, uint16_t i, uint8_t palette );

typedef struct CachedTexture
{
   GLuint  glName;
   uint32_t     address;
   uint32_t     crc;
   float   offsetS, offsetT;
   uint32_t     maskS, maskT;
   uint32_t     clampS, clampT;
   uint32_t     mirrorS, mirrorT;
   uint32_t     line;
   uint32_t     size;
   uint32_t     format;
   uint32_t     tMem;
   uint32_t     palette;
   uint32_t     width, height;            // N64 width and height
   uint32_t     clampWidth, clampHeight;  // Size to clamp to
   uint32_t     realWidth, realHeight;    // Actual texture size
   float     scaleS, scaleT;           // Scale to map to 0.0-1.0
   float     shiftScaleS, shiftScaleT; // Scale to shift
   uint32_t     textureBytes;

   struct CachedTexture   *lower, *higher;
   uint32_t     lastDList;
   uint8_t      max_level;
   uint8_t		frameBufferTexture;
} CachedTexture;

#define TEXTURECACHE_MAX (8 * 1024 * 1024)
#define TEXTUREBUFFER_SIZE (512 * 1024)

typedef struct TextureCache
{
    CachedTexture   *current[2];
    CachedTexture   *bottom, *top;
    CachedTexture   *dummy;

    uint32_t             cachedBytes;
    uint32_t             numCached;
    uint32_t             hits, misses;
    GLuint          glNoiseNames[32];
} TextureCache;

extern TextureCache cache;

static INLINE uint8_t TextureCache_SizeToBPP(uint8_t size)
{
   switch (size)
   {
      case 0:
         return 4;
      case 1:
         return 8;
      case 2:
         return 16;
      default:
         break;
   }

   return 32;
}

static INLINE uint32_t pow2( uint32_t dim )
{
    uint32_t i = 1;

    while (i < dim) i <<= 1;

    return i;
}

static INLINE uint32_t powof( uint32_t dim )
{
    uint32_t num, i;
    num = 1;
    i = 0;

    while (num < dim)
    {
        num <<= 1;
        i++;
    }

    return i;
}

CachedTexture *TextureCache_AddTop();
void TextureCache_MoveToTop( CachedTexture *newtop );
void TextureCache_Remove( CachedTexture *texture );
void TextureCache_RemoveBottom();
void TextureCache_Init();
void TextureCache_Destroy();
void TextureCache_Update( uint32_t t );
void TextureCache_ActivateTexture( uint32_t t, CachedTexture *texture );
void TextureCache_ActivateNoise( uint32_t t );
void TextureCache_ActivateDummy( uint32_t t );
bool TextureCache_Verify();

#ifdef __cplusplus
}
#endif

#endif

