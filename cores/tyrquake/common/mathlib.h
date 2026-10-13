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

#ifndef MATHLIB_H
#define MATHLIB_H

#include <limits.h>
#include <math.h>
#include <stdint.h>

#include "qtypes.h"

/* mathlib.h */

typedef float vec_t;
typedef vec_t vec3_t[3];
typedef vec_t vec5_t[5];

typedef int fixed4_t;
typedef int fixed8_t;
typedef int fixed16_t;
#define FIXED16_MAX INT_MAX;

/* min and max macros with type checking */
#define qmax(a,b) ((a > b) ? a : b)
#define qmin(a,b) ((a < b) ? a : b)

/* clamp macro with type checking */
#define qclamp(var,min,max) qmax(qmin(var,max),min)

#ifndef M_PI
#define M_PI 3.14159265358979323846	/* matches value in gcc v2 math.h */
#endif

#ifndef FLT_MAX
#define FLT_MAX 3.402823466e+38f 
#endif

extern vec3_t vec3_origin;

/* IS_NAN — true if x is NaN or Infinity. The original implementation
 * type-punned a float through an int pointer and ANDed against
 * nanmask (the IEEE-754 single-precision exponent bits 0x7f800000),
 * which evaluates true for both NaN and Infinity. */
#define IS_NAN(x) (!((x) >= -FLT_MAX && (x) <= FLT_MAX))

extern int32_t nanmask;

#define DotProduct(x,y) (x[0]*y[0]+x[1]*y[1]+x[2]*y[2])
#define VectorSubtract(a,b,c) do {c[0]=a[0]-b[0];c[1]=a[1]-b[1];c[2]=a[2]-b[2];} while (0)
#define VectorAdd(a,b,c) do {c[0]=a[0]+b[0];c[1]=a[1]+b[1];c[2]=a[2]+b[2];} while (0)
#define VectorCopy(a,b) do {b[0]=a[0];b[1]=a[1];b[2]=a[2];} while (0)

void VectorMA(const vec3_t veca, const float scale, const vec3_t vecb,
	      vec3_t vecc);

vec_t _DotProduct(vec3_t v1, vec3_t v2);
void _VectorSubtract(vec3_t veca, vec3_t vecb, vec3_t out);
void _VectorAdd(vec3_t veca, vec3_t vecb, vec3_t out);
void _VectorCopy(vec3_t in, vec3_t out);

int VectorCompare(vec3_t v1, vec3_t v2);
vec_t Length(vec3_t v);
void CrossProduct(const vec3_t v1, const vec3_t v2, vec3_t cross);
float VectorNormalize(vec3_t v);	/* returns vector length */
void VectorInverse(vec3_t v);
void VectorScale(const vec3_t in, const vec_t scale, vec3_t out);
int Q_log2(int val);

void R_ConcatRotations(float in1[3][3], float in2[3][3], float out[3][3]);
void R_ConcatTransforms(float in1[3][4], float in2[3][4], float out[3][4]);

void FloorDivMod(double numer, double denom, int *quotient, int *rem);
fixed16_t Invert24To16(fixed16_t val);
int GreatestCommonDivisor(int i1, int i2);

void AngleVectors(const vec3_t angles, vec3_t forward, vec3_t right, vec3_t up);
float anglemod(float a);

/* plane_t structure */
/* !!! if this is changed, it must be changed in asm_i386.h too !!! */
typedef struct mplane_s {
    vec3_t normal;
    float dist;
    byte type;		/* for texture axis selection and fast side tests */
    byte signbits;	/* signx + signy<<1 + signz<<1 */
    byte pad[2];
} mplane_t;

int SignbitsForPlane(const mplane_t *plane); /* sign bits for BOPS test */

#define PSIDE_FRONT 1
#define PSIDE_BACK  2
#define PSIDE_BOTH  (PSIDE_FRONT | PSIDE_BACK)

int BoxOnPlaneSide(const vec3_t mins, const vec3_t maxs, const mplane_t *plane);
#define BOX_ON_PLANE_SIDE(mins, maxs, p)			\
	(((p)->type < 3)?					\
	(							\
		((p)->dist <= (mins)[(p)->type])?		\
			PSIDE_FRONT				\
		:						\
		(						\
			((p)->dist >= (maxs)[(p)->type])?	\
				PSIDE_BACK			\
			:					\
				PSIDE_BOTH			\
		)						\
	)							\
	:							\
		BoxOnPlaneSide( (mins), (maxs), (p)))

void RotatePointAroundVector(vec3_t dst, const vec3_t dir,
			     const vec3_t point, float degrees);



#endif /* MATHLIB_H */
