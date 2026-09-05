/*
Copyright (C) 1996-1997 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
/* mathlib.c -- math primitives */

#include <math.h>

#include "mathlib.h"
#include "model.h"
#include "sys.h"

#include "quakedef.h"

vec3_t vec3_origin = { 0, 0, 0 };
int32_t nanmask = 255 << 23;

/*-----------------------------------------------------------------*/

#define DEG2RAD( a ) ( a * M_PI ) / 180.0F

void
ProjectPointOnPlane(vec3_t dst, const vec3_t p, const vec3_t normal)
{
   vec3_t n;
   float inv_denom = 1.0F / DotProduct(normal, normal);
   float d = DotProduct(normal, p) * inv_denom;

   n[0] = normal[0] * inv_denom;
   n[1] = normal[1] * inv_denom;
   n[2] = normal[2] * inv_denom;

   dst[0] = p[0] - d * n[0];
   dst[1] = p[1] - d * n[1];
   dst[2] = p[2] - d * n[2];
}

/*
 * assumes "src" is normalized
 */
void
PerpendicularVector(vec3_t dst, const vec3_t src)
{
   int pos;
   int i;
   float minelem = 1.0F;
   vec3_t tempvec;

   /*
    ** find the smallest magnitude axially aligned vector
    */
   for (pos = 0, i = 0; i < 3; i++) {
      if (fabsf(src[i]) < minelem) {
         pos = i;
         minelem = fabsf(src[i]);
      }
   }
   tempvec[0] = tempvec[1] = tempvec[2] = 0.0F;
   tempvec[pos] = 1.0F;

   /*
    ** project the point onto the plane defined by src
    */
   ProjectPointOnPlane(dst, tempvec, src);

   /*
    ** normalize the result
    */
   VectorNormalize(dst);
}


void
RotatePointAroundVector(vec3_t dst, const vec3_t dir, const vec3_t point,
			float degrees)
{
   float m[3][3];
   float im[3][3];
   float zrot[3][3];
   float tmpmat[3][3];
   float rot[3][3];
   int i;
   vec3_t vr, vup, vf;

   vf[0] = dir[0];
   vf[1] = dir[1];
   vf[2] = dir[2];

   PerpendicularVector(vr, dir);
   CrossProduct(vr, vf, vup);

   m[0][0] = vr[0];
   m[1][0] = vr[1];
   m[2][0] = vr[2];

   m[0][1] = vup[0];
   m[1][1] = vup[1];
   m[2][1] = vup[2];

   m[0][2] = vf[0];
   m[1][2] = vf[1];
   m[2][2] = vf[2];

   memcpy(im, m, sizeof(im));

   im[0][1] = m[1][0];
   im[0][2] = m[2][0];
   im[1][0] = m[0][1];
   im[1][2] = m[2][1];
   im[2][0] = m[0][2];
   im[2][1] = m[1][2];

   memset(zrot, 0, sizeof(zrot));
   zrot[0][0] = zrot[1][1] = zrot[2][2] = 1.0F;

   zrot[0][0] = cosf(DEG2RAD(degrees));
   zrot[0][1] = sinf(DEG2RAD(degrees));
   zrot[1][0] = -sinf(DEG2RAD(degrees));
   zrot[1][1] = cosf(DEG2RAD(degrees));

   R_ConcatRotations(m, zrot, tmpmat);
   R_ConcatRotations(tmpmat, im, rot);

   for (i = 0; i < 3; i++) {
      dst[i] =
         rot[i][0] * point[0] + rot[i][1] * point[1] +
         rot[i][2] * point[2];
   }
}

/*-----------------------------------------------------------------*/

float
anglemod(float a)
{
   a = (360.0 / 65536) * ((int)(a * (65536 / 360.0)) & 65535);
   return a;
}

int
SignbitsForPlane(const mplane_t *plane)
{
   int i;
   /* for fast box on planeside test */
   int bits = 0;
   for (i = 0; i < 3; i++)
   {
      if (plane->normal[i] < 0)
         bits |= 1 << i;
   }
   return bits;
}

/*
==================
BOPS_Error

Split out like this for ASM to call.
==================
*/
void BOPS_Error(void)
{
    Sys_Error("%s:  Bad signbits", __func__);
}

/*
==================
BoxOnPlaneSide

Returns PSIDE_FRONT, PSIDE_BACK, or PSIDE_BOTH (PSIDE_FRONT | PSIDE_BACK)
==================
*/
int
BoxOnPlaneSide(const vec3_t mins, const vec3_t maxs, const mplane_t *p)
{
   float dist1 = 0.0f, dist2 = 0.0f;
   int sides;

   /* general case */
   switch (p->signbits)
   {
      case 0:
         dist1 =
            p->normal[0] * maxs[0] + p->normal[1] * maxs[1] +
            p->normal[2] * maxs[2];
         dist2 =
            p->normal[0] * mins[0] + p->normal[1] * mins[1] +
            p->normal[2] * mins[2];
         break;
      case 1:
         dist1 =
            p->normal[0] * mins[0] + p->normal[1] * maxs[1] +
            p->normal[2] * maxs[2];
         dist2 =
            p->normal[0] * maxs[0] + p->normal[1] * mins[1] +
            p->normal[2] * mins[2];
         break;
      case 2:
         dist1 =
            p->normal[0] * maxs[0] + p->normal[1] * mins[1] +
            p->normal[2] * maxs[2];
         dist2 =
            p->normal[0] * mins[0] + p->normal[1] * maxs[1] +
            p->normal[2] * mins[2];
         break;
      case 3:
         dist1 =
            p->normal[0] * mins[0] + p->normal[1] * mins[1] +
            p->normal[2] * maxs[2];
         dist2 =
            p->normal[0] * maxs[0] + p->normal[1] * maxs[1] +
            p->normal[2] * mins[2];
         break;
      case 4:
         dist1 =
            p->normal[0] * maxs[0] + p->normal[1] * maxs[1] +
            p->normal[2] * mins[2];
         dist2 =
            p->normal[0] * mins[0] + p->normal[1] * mins[1] +
            p->normal[2] * maxs[2];
         break;
      case 5:
         dist1 =
            p->normal[0] * mins[0] + p->normal[1] * maxs[1] +
            p->normal[2] * mins[2];
         dist2 =
            p->normal[0] * maxs[0] + p->normal[1] * mins[1] +
            p->normal[2] * maxs[2];
         break;
      case 6:
         dist1 =
            p->normal[0] * maxs[0] + p->normal[1] * mins[1] +
            p->normal[2] * mins[2];
         dist2 =
            p->normal[0] * mins[0] + p->normal[1] * maxs[1] +
            p->normal[2] * maxs[2];
         break;
      case 7:
         dist1 =
            p->normal[0] * mins[0] + p->normal[1] * mins[1] +
            p->normal[2] * mins[2];
         dist2 =
            p->normal[0] * maxs[0] + p->normal[1] * maxs[1] +
            p->normal[2] * maxs[2];
         break;
      default:
         BOPS_Error();
         break;
   }

   sides = 0;
   if (dist1 >= p->dist)
      sides = PSIDE_FRONT;
   if (dist2 < p->dist)
      sides |= PSIDE_BACK;

   return sides;
}

void
AngleVectors(const vec3_t angles, vec3_t forward, vec3_t right, vec3_t up)
{
   float sr, sp, cr, cp;

   float angle = angles[YAW] * (float)(M_PI * 2 / 360);
   float sy = sinf(angle);
   float cy = cosf(angle);
   angle = angles[PITCH] * (float)(M_PI * 2 / 360);
   sp = sinf(angle);
   cp = cosf(angle);
   angle = angles[ROLL] * (float)(M_PI * 2 / 360);
   sr = sinf(angle);
   cr = cosf(angle);

   forward[0] = cp * cy;
   forward[1] = cp * sy;
   forward[2] = -sp;
   right[0] = (-1 * sr * sp * cy + -1 * cr * -sy);
   right[1] = (-1 * sr * sp * sy + -1 * cr * cy);
   right[2] = -1 * sr * cp;
   up[0] = (cr * sp * cy + -sr * -sy);
   up[1] = (cr * sp * sy + -sr * cy);
   up[2] = cr * cp;
}

int
VectorCompare(vec3_t v1, vec3_t v2)
{
   int i;

   for (i = 0; i < 3; i++)
      if (v1[i] != v2[i])
         return 0;

   return 1;
}

void VectorMA(const vec3_t veca, const float scale, const vec3_t vecb, vec3_t vecc)
{
   vecc[0] = veca[0] + scale * vecb[0];
   vecc[1] = veca[1] + scale * vecb[1];
   vecc[2] = veca[2] + scale * vecb[2];
}


vec_t _DotProduct(vec3_t v1, vec3_t v2)
{
   return v1[0] * v2[0] + v1[1] * v2[1] + v1[2] * v2[2];
}

void _VectorSubtract(vec3_t veca, vec3_t vecb, vec3_t out)
{
   out[0] = veca[0] - vecb[0];
   out[1] = veca[1] - vecb[1];
   out[2] = veca[2] - vecb[2];
}

void _VectorAdd(vec3_t veca, vec3_t vecb, vec3_t out)
{
   out[0] = veca[0] + vecb[0];
   out[1] = veca[1] + vecb[1];
   out[2] = veca[2] + vecb[2];
}

void _VectorCopy(vec3_t in, vec3_t out)
{
   out[0] = in[0];
   out[1] = in[1];
   out[2] = in[2];
}

void CrossProduct(const vec3_t v1, const vec3_t v2, vec3_t cross)
{
#if defined(__ARM_NEON__) && !defined(__APPLE__)
   asm volatile (
         "flds s3, [%0] \n\t" /* d1[1]={x0} */
         "add %0, %0, #4 \n\t" /**/
         "vld1.32 {d0}, [%0] \n\t" /* d0={y0,z0} */
         "vmov.f32 s2, s1 \n\t" /* d1[0]={z0} */

         "flds s5, [%1] \n\t" /* d2[1]={x1} */
         "add %1, %1, #4 \n\t" /**/
         "vld1.32 {d3}, [%1] \n\t" /* d3={y1,z1} */
         "vmov.f32 s4, s7 \n\t" /* d2[0]=d3[1] */

         "vmul.f32 d4, d0, d2 \n\t" /* d4=d0*d2 */
         "vmls.f32 d4, d1, d3 \n\t" /* d4-=d1*d3 */

         "vmul.f32 d5, d3, d1[1]                 \n\t" /* d5=d3*d1[1] */
         "vmls.f32 d5, d0, d2[1]                          \n\t" /* d5-=d0*d2[1] */

         "vst1.32 d4, [%2] \n\t" /**/
         "fsts s10, [%2, #8] \n\t" /**/

         : "+&r"(v1), "+&r"(v2), "+&r"(cross):
            : "d0", "d1", "d2", "d3", "d4", "d5", "memory"
               ); 
#else
         cross[0] = v1[1] * v2[2] - v1[2] * v2[1];
         cross[1] = v1[2] * v2[0] - v1[0] * v2[2];
         cross[2] = v1[0] * v2[1] - v1[1] * v2[0];
#endif
}

vec_t Length(vec3_t v)
{
   float length;

   length  = v[0] * v[0];
   length += v[1] * v[1];
   length += v[2] * v[2];
   length  = sqrtf(length);

   return length;
}

float VectorNormalize(vec3_t v)
{
   float length = v[0] * v[0] + v[1] * v[1] + v[2] * v[2];
   length = sqrtf(length);

   if (length)
   {
      float ilength = 1.0f / length;
      v[0] *= ilength;
      v[1] *= ilength;
      v[2] *= ilength;
   }

   return length;
}

void VectorInverse(vec3_t v)
{
    v[0] = -v[0];
    v[1] = -v[1];
    v[2] = -v[2];
}

void VectorScale(const vec3_t in, const vec_t scale, vec3_t out)
{
    out[0] = in[0] * scale;
    out[1] = in[1] * scale;
    out[2] = in[2] * scale;
}


int Q_log2(int val)
{
   int answer = 0;

   while ((val >>= 1) != 0)
      answer++;
   return answer;
}


/*
================
R_ConcatRotations
================
*/
void R_ConcatRotations(float in1[3][3], float in2[3][3], float out[3][3])
{
   out[0][0] = in1[0][0] * in2[0][0] + in1[0][1] * in2[1][0] +
      in1[0][2] * in2[2][0];
   out[0][1] = in1[0][0] * in2[0][1] + in1[0][1] * in2[1][1] +
      in1[0][2] * in2[2][1];
   out[0][2] = in1[0][0] * in2[0][2] + in1[0][1] * in2[1][2] +
      in1[0][2] * in2[2][2];
   out[1][0] = in1[1][0] * in2[0][0] + in1[1][1] * in2[1][0] +
      in1[1][2] * in2[2][0];
   out[1][1] = in1[1][0] * in2[0][1] + in1[1][1] * in2[1][1] +
      in1[1][2] * in2[2][1];
   out[1][2] = in1[1][0] * in2[0][2] + in1[1][1] * in2[1][2] +
      in1[1][2] * in2[2][2];
   out[2][0] = in1[2][0] * in2[0][0] + in1[2][1] * in2[1][0] +
      in1[2][2] * in2[2][0];
   out[2][1] = in1[2][0] * in2[0][1] + in1[2][1] * in2[1][1] +
      in1[2][2] * in2[2][1];
   out[2][2] = in1[2][0] * in2[0][2] + in1[2][1] * in2[1][2] +
      in1[2][2] * in2[2][2];
}


/*
================
R_ConcatTransforms
================
*/
void
R_ConcatTransforms(float in1[3][4], float in2[3][4], float out[3][4])
{
    out[0][0] = in1[0][0] * in2[0][0] + in1[0][1] * in2[1][0] +
	in1[0][2] * in2[2][0];
    out[0][1] = in1[0][0] * in2[0][1] + in1[0][1] * in2[1][1] +
	in1[0][2] * in2[2][1];
    out[0][2] = in1[0][0] * in2[0][2] + in1[0][1] * in2[1][2] +
	in1[0][2] * in2[2][2];
    out[0][3] = in1[0][0] * in2[0][3] + in1[0][1] * in2[1][3] +
	in1[0][2] * in2[2][3] + in1[0][3];
    out[1][0] = in1[1][0] * in2[0][0] + in1[1][1] * in2[1][0] +
	in1[1][2] * in2[2][0];
    out[1][1] = in1[1][0] * in2[0][1] + in1[1][1] * in2[1][1] +
	in1[1][2] * in2[2][1];
    out[1][2] = in1[1][0] * in2[0][2] + in1[1][1] * in2[1][2] +
	in1[1][2] * in2[2][2];
    out[1][3] = in1[1][0] * in2[0][3] + in1[1][1] * in2[1][3] +
	in1[1][2] * in2[2][3] + in1[1][3];
    out[2][0] = in1[2][0] * in2[0][0] + in1[2][1] * in2[1][0] +
	in1[2][2] * in2[2][0];
    out[2][1] = in1[2][0] * in2[0][1] + in1[2][1] * in2[1][1] +
	in1[2][2] * in2[2][1];
    out[2][2] = in1[2][0] * in2[0][2] + in1[2][1] * in2[1][2] +
	in1[2][2] * in2[2][2];
    out[2][3] = in1[2][0] * in2[0][3] + in1[2][1] * in2[1][3] +
	in1[2][2] * in2[2][3] + in1[2][3];
}


/*
===================
FloorDivMod

Returns mathematically correct (floor-based) quotient and remainder for
numer and denom, both of which should contain no fractional part. The
quotient must fit in 32 bits.
====================
*/

void
FloorDivMod(double numer, double denom, int *quotient, int *rem)
{
   int q, r;
   double x;

   if (denom <= 0.0)
      Sys_Error("%s: bad denominator %lf", __func__, denom);

   if (numer >= 0.0)
   {
      x = floor(numer / denom);
      q = (int)x;
      r = (int)floor(numer - (x * denom));
   }
   else
   {
      /* perform operations with positive values, 
       * and fix mod to make floor-based */
      x = floor(-numer / denom);
      q = -(int)x;
      r = (int)floor(-numer - (x * denom));
      if (r != 0)
      {
         q--;
         r = (int)denom - r;
      }
   }

   *quotient = q;
   *rem = r;
}


/*
===================
GreatestCommonDivisor
====================
*/
int
GreatestCommonDivisor(int i1, int i2)
{
   if (i1 > i2)
   {
      if (i2 == 0)
         return (i1);
      return GreatestCommonDivisor(i2, i1 % i2);
   }
   else
   {
      if (i1 == 0)
         return (i2);
      return GreatestCommonDivisor(i1, i2 % i1);
   }
}


/*
===================
Invert24To16

Inverts an 8.24 value to a 16.16 value
====================
*/

fixed16_t Invert24To16(fixed16_t val)
{
   if (val < 256)
      return (0xFFFFFFFF);

   return (fixed16_t)
      (((double)0x10000 * (double)0x1000000 / (double)val) + 0.5);
}
