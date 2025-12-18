#include <stdint.h>
#include <math.h>

#include "3dmath.h"

void TransformVectorNormalize(float vec[3], float mtx[4][4])
{
   float len;
   float x   = vec[0];
   float y   = vec[1];
   float z   = vec[2];

   vec[0] = mtx[0][0] * x 
   + mtx[1][0] * y 
   + mtx[2][0] * z;

   vec[1] = mtx[0][1] *x 
   + mtx[1][1] * y 
   + mtx[2][1] * z;

   vec[2] = mtx[0][2] * x 
   + mtx[1][2] * y 
   + mtx[2][2] * z;

   len = vec[0]*vec[0] + vec[1]*vec[1] + vec[2]*vec[2];

   if (len != 0.0)
   {
      len = sqrtf(len);
      vec[0] /= len;
      vec[1] /= len;
      vec[2] /= len;
   }
}
