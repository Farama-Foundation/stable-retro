#ifndef _GSP_FUNCS_C_H
#define _GSP_FUNCS_C_H

#include <stdint.h>

#include "gSP_funcs_prot.h"

#ifdef __cplusplus
extern "C" {
#endif

#define gSPCombineMatrices() GSPCombineMatricesC()
#define gSPClipVertex(v)     GSPClipVertexC(v)
#define gSPLookAt(l, n)      GSPLookAtC(l, n)
#define gSPLight(l, n)       GSPLightC(l, n)
#define gSPLightColor(l, c)  GSPLightColorC(l, c)
#define gSPViewport(v)       GSPViewportC(v)
#define gSPForceMatrix(mptr) GSPForceMatrixC(mptr)
#define gSPEndDisplayList()  GSPEndDisplayListC()

void GSPCombineMatricesC(void);
void GSPClipVertexC(uint32_t v);
void GSPLookAtC(uint32_t l, uint32_t n);
void GSPLightC(uint32_t l, int32_t n);
void GSPLightColorC(uint32_t lightNum, uint32_t packedColor );
void GSPViewportC(uint32_t v);
void GSPForceMatrixC(uint32_t mptr);
void GSPEndDisplayListC(void);

#ifdef __cplusplus
}
#endif

#endif
