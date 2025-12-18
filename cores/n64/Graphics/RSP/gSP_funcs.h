#ifndef _GSP_FUNCS_H
#define _GSP_FUNCS_H

#include <stdint.h>

#include "gSP_funcs_prot.h"

#define gSPCombineMatrices() GSPCombineMatrices()
#define gSPClipVertex(v)     GSPClipVertex(v)
#define gSPLookAt(l, n)      GSPLookAt(l, n)
#define gSPLight(l, n)       GSPLight(l, n)
#define gSPLightColor(l, c)  GSPLightColor(l, c)
#define gSPViewport(v)       GSPViewport(v)
#define gSPForceMatrix(mptr) GSPForceMatrix(mptr)
#define gSPEndDisplayList()  GSPEndDisplayList()

void GSPCombineMatrices(void);
void GSPClipVertex(uint32_t v);
void GSPLookAt(uint32_t l, uint32_t n);
void GSPLight(uint32_t l, int32_t n);
void GSPLightColor(uint32_t lightNum, uint32_t packedColor );
void GSPViewport(uint32_t v);
void GSPForceMatrix(uint32_t mptr);
void GSPEndDisplayList(void);


#endif
