#ifndef _GSP_FUNCS_PROT_H
#define _GSP_FUNCS_PROT_H

#include <stdint.h>

#include <boolean.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Glide64 prototypes */
void glide64gSPMatrix( uint32_t matrix, uint8_t param );
void glide64gSPClearGeometryMode (uint32_t mode);
void glide64gSPSetGeometryMode (uint32_t mode);
void glide64gSPTexture(int32_t sc, int32_t tc, int32_t level, 
      int32_t tile, int32_t on);
void glide64gSPClipRatio(uint32_t r);
void glide64gSPDisplayList(uint32_t dl);
void glide64gSPBranchList(uint32_t dl);
void glide64gSPSetVertexColorBase(uint32_t base);
void glide64gSPSegment(int32_t seg, int32_t base);
void glide64gSPClipVertex(uint32_t v);
void glide64gSPLightColor( uint32_t lightNum, uint32_t packedColor );
void glide64gSPCombineMatrices(void);
void glide64gSPLookAt(uint32_t l, uint32_t n);
void glide64gSP1Triangle( int32_t v0, int32_t v1, int32_t v2, int32_t flag );
void glide64gSP4Triangles( int32_t v00, int32_t v01, int32_t v02,
                    int32_t v10, int32_t v11, int32_t v12,
                    int32_t v20, int32_t v21, int32_t v22,
                    int32_t v30, int32_t v31, int32_t v32 );
void glide64gSPLight(uint32_t l, int32_t n);
void glide64gSPViewport(uint32_t v);
void glide64gSPForceMatrix( uint32_t mptr );
void glide64gSPObjMatrix( uint32_t mtx );
void glide64gSPObjSubMatrix( uint32_t mtx );
void glide64gSPBranchLessZ(uint32_t branchdl, uint32_t vtx, float zval);

void glide64gSP2Triangles(
      const int32_t v00,
      const int32_t v01,
      const int32_t v02,
      const int32_t flag0,
      const int32_t v10,
      const int32_t v11,
      const int32_t v12,
      const int32_t flag1 );

void glide64gSPVertex(uint32_t v, uint32_t n, uint32_t v0);
void glide64gSPFogFactor(int16_t fm, int16_t fo );
void glide64gSPNumLights(int32_t n);
void glide64gSPPopMatrixN(uint32_t param, uint32_t num );
void glide64gSPPopMatrix(uint32_t param);
void glide64gSPDlistCount(uint32_t count, uint32_t v);
void glide64gSPModifyVertex( uint32_t vtx, uint32_t where, uint32_t val );
bool glide64gSPCullVertices( uint32_t v0, uint32_t vn );
void glide64gSPCullDisplayList( uint32_t v0, uint32_t vn );
void glide64gSP1Quadrangle(int32_t v0, int32_t v1, int32_t v2, int32_t v3);
void glide64gSPCIVertex(uint32_t v, uint32_t n, uint32_t v0);
void glide64gSPSetVertexColorBase(uint32_t base);
void glide64gSPSetDMATexOffset(uint32_t addr);

void gln64gSPDlistCount(uint32_t count, uint32_t v);

#ifdef __cplusplus
}
#endif


#ifndef GLIDEN64
#ifdef __cplusplus
extern "C" {
#endif
#endif

/* GLN64 prototypes */
extern void (*gln64gSPLightVertex)(void *data);
void gln64gSPSegment(int32_t seg, int32_t base);
void gln64gSPClipVertex(uint32_t v);
void gln64gSPLightColor(uint32_t lightNum, uint32_t packedColor);
void gln64gSPLight(uint32_t l, int32_t n);
void gln64gSPCombineMatrices(void);
void gln64gSPLookAt(uint32_t l, uint32_t n);
void gln64gSP1Triangle( int32_t v0, int32_t v1, int32_t v2, int32_t flag );
void gln64gSP4Triangles( int32_t v00, int32_t v01, int32_t v02,
                    int32_t v10, int32_t v11, int32_t v12,
                    int32_t v20, int32_t v21, int32_t v22,
                    int32_t v30, int32_t v31, int32_t v32 );
void gln64gSPViewport(uint32_t v);
void gln64gSPForceMatrix( uint32_t mptr );

#ifndef GLIDEN64
#ifdef __cplusplus
}
#endif
#endif

#ifdef __cplusplus

/* Rice prototypes */

void ricegSPModifyVertex(uint32_t vtx, uint32_t where, uint32_t val);
void ricegSPCIVertex(uint32_t v, uint32_t n, uint32_t v0);
void ricegSPLightColor(uint32_t lightNum, uint32_t packedColor);
void ricegSPNumLights(int32_t n);
void ricegSPDMATriangles( uint32_t tris, uint32_t n );
void ricegSPLight(uint32_t dwAddr, uint32_t dwLight);
void ricegSPViewport(uint32_t v);

#endif

#endif
