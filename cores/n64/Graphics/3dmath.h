#ifndef _GRAPHICS_3DMATH_H
#define _GRAPHICS_3DMATH_H

#include <math.h>
#include <string.h>

#include <retro_inline.h>
#include <retro_miscellaneous.h>

#ifdef __cplusplus
extern "C" {
#endif

static INLINE float DotProduct(const float *v0, const float *v1)
{
   return v0[0] * v1[0] + v0[1] * v1[1] + v0[2] * v1[2];
}

static INLINE void CopyMatrix( float m0[4][4], float m1[4][4] )
{
    memcpy( m0, m1, 16 * sizeof( float ) );
}

static INLINE void NormalizeVector(float *v)
{
   float len = v[0]*v[0] + v[1]*v[1] + v[2]*v[2];
   if (len == 0.0f)
      return;
   len = sqrtf( len );
   v[0] /= len;
   v[1] /= len;
   v[2] /= len;
}

void TransformVectorNormalize(float vec[3], float mtx[4][4]);

#ifdef __cplusplus
}
#endif

#endif
