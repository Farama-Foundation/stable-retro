#ifndef GDP_H
#define GDP_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define CHANGED_RENDERMODE      0x0001
#define CHANGED_CYCLETYPE       0x0002
#define CHANGED_SCISSOR         0x0004
#define CHANGED_TMEM            0x0008
#define CHANGED_TILE            0x0010
#define CHANGED_COMBINE_COLORS  0x020
#define CHANGED_COMBINE			  0x040
#define CHANGED_ALPHACOMPARE	  0x080
#define CHANGED_FOGCOLOR		  0x100

#define CHANGED_FB_TEXTURE      0x0200

#define TEXTUREMODE_NORMAL          0
#define TEXTUREMODE_TEXRECT         1
#define TEXTUREMODE_BGIMAGE         2
#define TEXTUREMODE_FRAMEBUFFER     3
/* New GLiden64 define */
#define TEXTUREMODE_FRAMEBUFFER_BG	4

#define LOADTYPE_BLOCK          0
#define LOADTYPE_TILE           1

void gln64gDPSetOtherMode( uint32_t mode0, uint32_t mode1 );
void gln64gDPSetPrimDepth( uint16_t z, uint16_t dz );
void gln64gDPPipelineMode( uint32_t mode );
void gln64gDPSetCycleType( uint32_t type );
void gln64gDPSetTexturePersp( uint32_t enable );
void gln64gDPSetTextureLUT( uint32_t mode );
void gln64gDPSetCombineKey( uint32_t type );
void gln64gDPSetAlphaCompare( uint32_t mode );
void gln64gDPSetCombine( int32_t muxs0, int32_t muxs1 );
void gln64gDPSetColorImage( uint32_t format, uint32_t size, uint32_t width, uint32_t address );
void gDPSetColorImage( uint32_t format, uint32_t size, uint32_t width, uint32_t address );
void gln64gDPSetTextureImage( uint32_t format, uint32_t size, uint32_t width, uint32_t address );
void gDPSetTextureImage( uint32_t format, uint32_t size, uint32_t width, uint32_t address );
void gln64gDPSetDepthSource(uint32_t source);
void gln64gDPSetDepthImage( uint32_t address );
void gDPSetDepthImage( uint32_t address );
void gln64gDPSetEnvColor( uint32_t r, uint32_t g, uint32_t b, uint32_t a );
void gDPSetEnvColor( uint32_t r, uint32_t g, uint32_t b, uint32_t a );
void gln64gDPSetBlendColor( uint32_t r, uint32_t g, uint32_t b, uint32_t a );
void gDPSetBlendColor( uint32_t r, uint32_t g, uint32_t b, uint32_t a );
void gln64gDPSetFogColor( uint32_t r, uint32_t g, uint32_t b, uint32_t a );
void gDPSetFogColor( uint32_t r, uint32_t g, uint32_t b, uint32_t a );
void gln64gDPSetRenderMode(uint32_t mode1, uint32_t mode2);
void gln64gDPSetFillColor( uint32_t c );
void gln64gDPSetAlphaDither(uint32_t type);
void gDPSetFillColor( uint32_t c );
void gln64gDPSetTextureFilter(uint32_t type);
void gln64gDPSetTextureLOD(uint32_t mode);
void gln64gDPSetPrimColor( uint32_t m, uint32_t l, uint32_t r, uint32_t g, uint32_t b, uint32_t a );
void gDPSetPrimColor( uint32_t m, uint32_t l, uint32_t r, uint32_t g, uint32_t b, uint32_t a );
void gln64gDPSetTile(
    uint32_t format, uint32_t size, uint32_t line, uint32_t tmem, uint32_t tile, uint32_t palette, uint32_t cmt,
    uint32_t cms, uint32_t maskt, uint32_t masks, uint32_t shiftt, uint32_t shifts );
void gDPSetTile(
    uint32_t format, uint32_t size, uint32_t line, uint32_t tmem, uint32_t tile, uint32_t palette, uint32_t cmt,
    uint32_t cms, uint32_t maskt, uint32_t masks, uint32_t shiftt, uint32_t shifts );
void gln64gDPSetTileSize( uint32_t tile, uint32_t uls, uint32_t ult, uint32_t lrs, uint32_t lrt );
void gln64gDPLoadTile( uint32_t tile, uint32_t uls, uint32_t ult, uint32_t lrs, uint32_t lrt );
void gln64gDPLoadTLUT( uint32_t tile, uint32_t uls, uint32_t ult, uint32_t lrs, uint32_t lrt );
void gln64gDPSetScissor( uint32_t mode, float ulx, float uly, float lrx, float lry );
void gln64gDPFillRectangle( int32_t ulx, int32_t uly, int32_t lrx, int32_t lry );
void gln64gDPSetConvert( int32_t k0, int32_t k1, int32_t k2, int32_t k3, int32_t k4, int32_t k5 );
void gln64gDPSetKeyR( uint32_t cR, uint32_t sR, uint32_t wR );
void gln64gDPSetKeyGB(uint32_t cG, uint32_t sG, uint32_t wG, uint32_t cB, uint32_t sB, uint32_t wB );
void gln64gDPTextureRectangle( float ulx, float uly, float lrx, float lry, int32_t tile, float s, float t, float dsdx, float dtdy );
void gln64gDPTextureRectangleFlip( float ulx, float uly, float lrx, float lry, int32_t tile, float s, float t, float dsdx, float dtdy );
void gln64gDPFullSync(void);
void gln64gDPTileSync(void);
void gln64gDPPipeSync(void);
void gln64gDPLoadSync(void);
void gln64gDPNoOp(void);

void gln64gDPTriFill(uint32_t w0, uint32_t w1);
void gln64gDPTriFillZ(uint32_t w0, uint32_t w1);
void gln64gDPTriShadeZ(uint32_t w0, uint32_t w1);
void gln64gDPTriTxtrZ(uint32_t w0, uint32_t w1);
void gln64gDPTriTxtr(uint32_t w0, uint32_t w1);
void gln64gDPTriShadeTxtrZ(uint32_t w0, uint32_t w1);
void gln64gDPTriShadeTxtr(uint32_t w0, uint32_t w1);

void gln64gDPLoadBlock(uint32_t tile, uint32_t uls, uint32_t ult, uint32_t lrs, uint32_t dxt);

#ifdef __cplusplus
}
#endif

#endif

