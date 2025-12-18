#ifndef _GDP_FUNCS_PROT_H
#define _GDP_FUNCS_PROT_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Glide64 prototypes */
void glide64gDPTriangle(uint32_t w0, uint32_t w1, int shade, int texture, int zbuffer);
void glide64gSPClipRatio(uint32_t r);
void glide64gDPSetScissor( uint32_t mode, float ulx, float uly, float lrx, float lry );
void glide64gDPLoadBlock( uint32_t tile, uint32_t ul_s, uint32_t ul_t,
      uint32_t lr_s, uint32_t dxt );
void glide64gDPSetTextureImage(
      int32_t fmt,
      int32_t siz,
      int32_t width,
      int32_t addr);
void glide64gDPSetTile(
      uint32_t fmt,
      uint32_t siz,
      uint32_t line,
      uint32_t tmem,
      uint32_t tile,
      uint32_t palette,
      uint32_t cmt,
      uint32_t maskt,
      uint32_t shiftt,
      uint32_t cms,
      uint32_t masks,
      uint32_t shifts );
void glide64gDPLoadTile(uint32_t tile, uint32_t ul_s, uint32_t ul_t,
      uint32_t lr_s, uint32_t lr_t);
void glide64gDPSetTileSize(uint32_t tile, uint32_t uls, uint32_t ult,
      uint32_t lrs, uint32_t lrt);
void glide64gDPSetTextureImage(int32_t fmt, int32_t siz,
   int32_t width, int32_t addr);
void glide64gDPFillRectangle(uint32_t ul_x, uint32_t ul_y, uint32_t lr_x, uint32_t lr_y);

#ifdef __cplusplus
}
#endif


/* GLN64 prototypes */
#ifndef GLIDEN64
#ifdef __cplusplus
extern "C" {
#endif
#endif

void gln64gDPSetScissor( uint32_t mode, float ulx, float uly, float lrx, float lry );
void gln64gDPLoadBlock( uint32_t tile, uint32_t ul_s, uint32_t ul_t,
      uint32_t lr_s, uint32_t dxt );

#ifndef GLIDEN64
#ifdef __cplusplus
}
#endif
#endif

#ifdef __cplusplus

/* Rice prototypes */
void ricegDPSetScissor(void *data, 
      uint32_t mode, float ulx, float uly, float lrx, float lry );
void ricegDPSetFillColor(uint32_t c);
void ricegDPSetFogColor(uint32_t r, uint32_t g, uint32_t b, uint32_t a);
void ricegDPFillRect(int32_t ulx, int32_t uly, int32_t lrx, int32_t lry );
void ricegDPSetEnvColor(uint32_t r, uint32_t g, uint32_t b, uint32_t a);
void ricegDPLoadBlock( uint32_t tileno, uint32_t uls, uint32_t ult,
      uint32_t lrs, uint32_t dxt );
void ricegDPLoadTile(uint32_t tileno, uint32_t uls, uint32_t ult,
      uint32_t lrs, uint32_t lrt);
void ricegDPLoadTLUT(uint16_t count, uint32_t tileno,
      uint32_t uls, uint32_t ult, uint32_t lrs, uint32_t lrt);

#endif

#endif
