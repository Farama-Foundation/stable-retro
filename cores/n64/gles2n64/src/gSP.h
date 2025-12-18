#ifndef GSP_H
#define GSP_H

#include <stdint.h>
#include <boolean.h>
#include "GBI.h"
#include "gDP.h"

#include "../../Graphics/RSP/RSP_state.h"
#include "../../Graphics/RSP/gSP_funcs_C.h"

#define CHANGED_VIEWPORT        0x01
#define CHANGED_MATRIX          0x02
#define CHANGED_COLORBUFFER     0x04
#define CHANGED_GEOMETRYMODE    0x08
#define CHANGED_TEXTURE         0x10
#define CHANGED_FOGPOSITION     0x20
#define CHANGED_LIGHT			  0x40
#define CHANGED_CPU_FB_WRITE	  0x80

#define CLIP_X      0x03
#define CLIP_NEGX   0x01
#define CLIP_POSX   0x02

#define CLIP_Y      0x0C
#define CLIP_NEGY   0x04
#define CLIP_POSY   0x08

#define CLIP_Z      0x10

#define CLIP_ALL    0x1F // CLIP_NEGX|CLIP_POSX|CLIP_NEGY|CLIP_POSY|CLIP_Z

void gln64gSPLoadUcodeEx( uint32_t uc_start, uint32_t uc_dstart, uint16_t uc_dsize );
void gln64gSPNoOp();
void gln64gSPMatrix( uint32_t matrix, uint8_t param );
void gln64gSPDMAMatrix( uint32_t matrix, uint8_t index, uint8_t multiply );
void gln64gSPLight( uint32_t l, int32_t n );
void gln64gSPLightCBFD( uint32_t l, int32_t n );
void gln64gSPVertex( uint32_t v, uint32_t n, uint32_t v0 );
void gln64gSPCIVertex( uint32_t v, uint32_t n, uint32_t v0 );
void gln64gSPDMAVertex( uint32_t v, uint32_t n, uint32_t v0 );
void gln64gSPCBFDVertex( uint32_t v, uint32_t n, uint32_t v0 );
void gln64gSPDisplayList( uint32_t dl );
void gln64gSPDMADisplayList( uint32_t dl, uint32_t n );
void gln64gSPBranchList( uint32_t dl );
void gln64gSPBranchLessZ( uint32_t branchdl, uint32_t vtx, float zval );
void gln64gSPDlistCount(uint32_t count, uint32_t v);
void gln64gSPSprite2DBase( uint32_t base );
void gln64gSPDMATriangles( uint32_t tris, uint32_t n );
void gln64gSP1Quadrangle( int32_t v0, int32_t v1, int32_t v2, int32_t v3 );
void gln64gSPCullDisplayList( uint32_t v0, uint32_t vn );
void gln64gSPPopMatrix( uint32_t param );
void gln64gSPPopMatrixN( uint32_t param, uint32_t num );
void gln64gSPSegment( int32_t seg, int32_t base );
void gln64gSPClipRatio( uint32_t r );
void gln64gSPInsertMatrix( uint32_t where, uint32_t num );
void gln64gSPModifyVertex( uint32_t vtx, uint32_t where, uint32_t val );
void gln64gSPNumLights( int32_t n );
void gln64gSPFogFactor( int16_t fm, int16_t fo );
void gln64gSPPerspNormalize( uint16_t scale );
void gln64gSPTexture( float sc, float tc, int32_t level, int32_t tile, int32_t on );
void gln64gSPEndDisplayList();
void gln64gSPGeometryMode( uint32_t clear, uint32_t set );
void gln64gSPSetGeometryMode( uint32_t mode );
void gln64gSPClearGeometryMode( uint32_t mode );
void gln64gSPSetOtherMode_H(uint32_t _length, uint32_t _shift, uint32_t _data);
void gln64gSPSetOtherMode_L(uint32_t _length, uint32_t _shift, uint32_t _data);
void gln64gSPLine3D( int32_t v0, int32_t v1, int32_t flag );
void gln64gSPLineW3D( int32_t v0, int32_t v1, int32_t wd, int32_t flag );
void gln64gSPObjRectangle( uint32_t sp );
void gln64gSPObjRectangleR(uint32_t _sp);
void gln64gSPObjSprite( uint32_t sp );
void gln64gSPObjLoadTxtr( uint32_t tx );
void gln64gSPObjLoadTxSprite( uint32_t txsp );
void gln64gSPObjLoadTxRectR( uint32_t txsp );
void gln64gSPBgRect1Cyc( uint32_t bg );
void gln64gSPBgRectCopy( uint32_t bg );
void gln64gSPObjMatrix( uint32_t mtx );
void gln64gSPObjSubMatrix( uint32_t mtx );
void gln64gSPObjRendermode(uint32_t _mode);
void gln64gSPSetDMAOffsets( uint32_t mtxoffset, uint32_t vtxoffset );
void gln64gSPSetVertexColorBase( uint32_t base );

void gln64gSPSetVertexNormaleBase( uint32_t base );
void gln64gSPProcessVertex(uint32_t v);
void gln64gSPCoordMod(uint32_t _w0, uint32_t _w1);

void gln64gSPTriangleUnknown(void);

void gln64gSPTriangle(int32_t v0, int32_t v1, int32_t v2);
void gln64gSP2Triangles(const int32_t v00, const int32_t v01, const int32_t v02, const int32_t flag0,
                    const int32_t v10, const int32_t v11, const int32_t v12, const int32_t flag1 );
void gln64gSP4Triangles(const int32_t v00, const int32_t v01, const int32_t v02,
                    const int32_t v10, const int32_t v11, const int32_t v12,
                    const int32_t v20, const int32_t v21, const int32_t v22,
                    const int32_t v30, const int32_t v31, const int32_t v32 );


extern void (*gln64gSPTransformVertex)(float vtx[4], float mtx[4][4]);
extern void (*gln64gSPPointLightVertex)(void *_vtx, float * _vPos);
extern void (*gln64gSPBillboardVertex)(uint32_t v, uint32_t i);
void gln64gSPSetupFunctions(void);
void gSPSetupFunctions(void);
void gln64gSPSetDMATexOffset(uint32_t _addr);

#endif

