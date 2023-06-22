#ifndef _GLES2N64_3DMATH_H
#define _GLES2N64_3DMATH_H

#include <math.h>
#include <memory.h>
#include <string.h>

#include <retro_inline.h>

#include "../../Graphics/3dmath.h"

#ifdef __cplusplus
extern "C" {
#endif

void MultMatrix( float m0[4][4], float m1[4][4], float dest[4][4]);

static INLINE void MultMatrix2( float m0[4][4], float m1[4][4] )
{
    float dst[4][4];
    MultMatrix(m0, m1, dst);
    memcpy( m0, dst, sizeof(float) * 16 );
}

static INLINE void Transpose3x3Matrix( float mtx[4][4] )
{
    float tmp = mtx[0][1];
    mtx[0][1] = mtx[1][0];
    mtx[1][0] = tmp;

    tmp = mtx[0][2];
    mtx[0][2] = mtx[2][0];
    mtx[2][0] = tmp;

    tmp = mtx[1][2];
    mtx[1][2] = mtx[2][1];
    mtx[2][1] = tmp;
}

#ifdef __cplusplus
}
#endif

#endif

