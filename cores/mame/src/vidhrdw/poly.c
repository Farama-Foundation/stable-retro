/***************************************************************************

	poly.c

	Helper routines for polygon rendering.

***************************************************************************/

#ifndef RECURSIVE_INCLUDE

#include "poly.h"

/***************************************************************************

	Global variables

***************************************************************************/

static struct poly_scanline_data scanlines;



/*------------------------------------------------------------------
	setup_triangle_n
------------------------------------------------------------------*/

#define RECURSIVE_INCLUDE

#define FUNC_NAME	setup_triangle_0
#define NUM_PARAMS	0
#include "poly.c"
#undef NUM_PARAMS
#undef FUNC_NAME

#define FUNC_NAME	setup_triangle_1
#define NUM_PARAMS	1
#include "poly.c"
#undef NUM_PARAMS
#undef FUNC_NAME

#define FUNC_NAME	setup_triangle_2
#define NUM_PARAMS	2
#include "poly.c"
#undef NUM_PARAMS
#undef FUNC_NAME

#define FUNC_NAME	setup_triangle_3
#define NUM_PARAMS	3
#include "poly.c"
#undef NUM_PARAMS
#undef FUNC_NAME

#define FUNC_NAME	setup_triangle_4
#define NUM_PARAMS	4
#include "poly.c"
#undef NUM_PARAMS
#undef FUNC_NAME

#define FUNC_NAME	setup_triangle_5
#define NUM_PARAMS	5
#include "poly.c"
#undef NUM_PARAMS
#undef FUNC_NAME

#define FUNC_NAME	setup_triangle_6
#define NUM_PARAMS	6
#include "poly.c"
#undef NUM_PARAMS
#undef FUNC_NAME


#else

const struct poly_scanline_data *FUNC_NAME(const struct poly_vertex *v1, const struct poly_vertex *v2, const struct poly_vertex *v3, const struct rectangle *cliprect)
{
	INT32 i, y, dy, ey, sx, sdx, ex, edx, temp, longest_scanline;
	INT64 pstart[MAX_VERTEX_PARAMS], pdelta[MAX_VERTEX_PARAMS];
	const struct poly_vertex *tv;
	struct poly_scanline *scan;
	struct shiftclip;

	/* first sort by Y */
	if (v2->y < v1->y) { tv = v1; v1 = v2; v2 = tv; }
	if (v3->y < v2->y)
	{
		tv = v2; v2 = v3; v3 = tv;
		if (v2->y < v1->y) { tv = v1; v1 = v2; v2 = tv; }
	}

	/* see if we're on-screen */
	if (v1->y > cliprect->max_y || v3->y < cliprect->min_y)
		return NULL;
	if ((v1->x < cliprect->min_x && v2->x < cliprect->min_x && v3->x < cliprect->min_x) ||
		(v1->x > cliprect->max_x && v2->x > cliprect->max_x && v3->x > cliprect->max_x))
		return NULL;

	/* set the final start/end Y values */
	scanlines.sy = (v1->y < cliprect->min_y) ? cliprect->min_y : v1->y;
	scanlines.ey = (v3->y > cliprect->max_y) ? cliprect->max_y : v3->y - 1;

	/* imagine the triangle divided into two parts vertically, splitting at v2:

				* (v1)			------- part 1 starts here

		(v2) *					------- part 1 ends here, part 2 starts here

					* (v3)		------- part 2 ends here
	*/

	/* determine what portion of the triangle lives in the upper part (16.16) */
	temp = v3->y - v1->y;
	if (temp <= 0)
		return NULL;
	temp = ((v2->y - v1->y) << 16) / temp;

	/* compute the length of the longest scanline; this is the dx from v1 to v2 */
	/* plus the fractional portion of the dx from v1 to v3 at the boundary between */
	/* the two parts (16.16) */
	longest_scanline = ((v1->x - v2->x) << 16) + temp * (v3->x - v1->x);

	/* if the longest scanline is 0, early out */
	if (longest_scanline == 0)
		return NULL;

	/* the per-pixel deltas for the parameters will be constant across the entire */
	/* surface of the triangle, so compute those up front (16.16) */
	if (NUM_PARAMS >= 0)
	{
		INT64 value = ((INT64)(v1->p[0] - v2->p[0]) << 32) + (INT64)temp * ((INT64)(v3->p[0] - v1->p[0]) << 16);
		scanlines.dp[0] = value / longest_scanline;
	}
	if (NUM_PARAMS >= 1)
	{
		INT64 value = ((INT64)(v1->p[1] - v2->p[1]) << 32) + (INT64)temp * ((INT64)(v3->p[1] - v1->p[1]) << 16);
		scanlines.dp[1] = value / longest_scanline;
	}
	if (NUM_PARAMS >= 2)
	{
		INT64 value = ((INT64)(v1->p[2] - v2->p[2]) << 32) + (INT64)temp * ((INT64)(v3->p[2] - v1->p[2]) << 16);
		scanlines.dp[2] = value / longest_scanline;
	}
	if (NUM_PARAMS >= 3)
	{
		INT64 value = ((INT64)(v1->p[3] - v2->p[3]) << 32) + (INT64)temp * ((INT64)(v3->p[3] - v1->p[3]) << 16);
		scanlines.dp[3] = value / longest_scanline;
	}
	if (NUM_PARAMS >= 4)
	{
		INT64 value = ((INT64)(v1->p[4] - v2->p[4]) << 32) + (INT64)temp * ((INT64)(v3->p[4] - v1->p[4]) << 16);
		scanlines.dp[4] = value / longest_scanline;
	}
	if (NUM_PARAMS >= 5)
	{
		INT64 value = ((INT64)(v1->p[5] - v2->p[5]) << 32) + (INT64)temp * ((INT64)(v3->p[5] - v1->p[5]) << 16);
		scanlines.dp[5] = value / longest_scanline;
	}

	/* if the longest scanline was negative, the middle vertex is to the right */
	if (longest_scanline < 0)
	{
		/* the target vertex for the left side is v3 */
		tv = v3;

		/* compute the height of the first part of the triangle */
		dy = v2->y - v1->y;

		/* start the right hand counter at v1's X */
		ex = v1->x;

		/* if there's nothing to draw in part 1, move on to part 2 */
		if (dy <= 0)
		{
			/* the new height is the height of part 2 */
			dy = v3->y - v2->y;

			/* start the right hand counter at v2's X */
			ex = v2->x;

			/* and match up the last two vertices so that we come out all right in the end */
			v2 = v3;
		}

		/* compute the final parameters for the right hand slope */
		edx = ((v2->x - ex) << 16) / dy;
		ex = (ex << 16) | 0xffff;
	}

	/* if the longest scanline was positive, the middle vertex is to the left */
	else
	{
		/* start the right hand counter at v1's X */
		ex = v1->x;

		/* compute the final parameters for the right hand slope */
		/* which is guaranteed to be v1->v3, unlike the case above */
		edx = ((v3->x - ex) << 16) / (v3->y - v1->y);
		ex = (ex << 16) | 0xffff;

		/* compute the height of the first part of the triangle */
		dy = v2->y - v1->y;

		/* if there's nothing to draw in part 1, move on to part 2 */
		if (dy <= 0)
		{
			/* the new height is the height of part 2 */
			dy = v3->y - v2->y;

			/* shuffle the remaining vertices to make it come out all right in the end */
			v1 = v2;
			v2 = v3;
		}

		/* the target vertex for the left side is v2 */
		tv = v2;
	}

	/* compute the parameters for the left hand slope */
	sx = v1->x;
	temp = tv->y - v1->y;
	sdx = ((tv->x - sx) << 16) / temp;
	sx = (sx << 16) | 0xffff;

	/* set up the starting values and per-scanline deltas for the parameters */
	if (NUM_PARAMS >= 0)
	{
		pstart[0] = ((INT64)v1->p[0] << 16) | 0x8000;
		pdelta[0] = ((INT64)(tv->p[0] - v1->p[0]) << 16) / temp;
	}
	if (NUM_PARAMS >= 1)
	{
		pstart[1] = ((INT64)v1->p[1] << 16) | 0x8000;
		pdelta[1] = ((INT64)(tv->p[1] - v1->p[1]) << 16) / temp;
	}
	if (NUM_PARAMS >= 2)
	{
		pstart[2] = ((INT64)v1->p[2] << 16) | 0x8000;
		pdelta[2] = ((INT64)(tv->p[2] - v1->p[2]) << 16) / temp;
	}
	if (NUM_PARAMS >= 3)
	{
		pstart[3] = ((INT64)v1->p[3] << 16) | 0x8000;
		pdelta[3] = ((INT64)(tv->p[3] - v1->p[3]) << 16) / temp;
	}
	if (NUM_PARAMS >= 4)
	{
		pstart[4] = ((INT64)v1->p[4] << 16) | 0x8000;
		pdelta[4] = ((INT64)(tv->p[4] - v1->p[4]) << 16) / temp;
	}
	if (NUM_PARAMS >= 5)
	{
		pstart[5] = ((INT64)v1->p[5] << 16) | 0x8000;
		pdelta[5] = ((INT64)(tv->p[5] - v1->p[5]) << 16) / temp;
	}

	/* set up everything for the big loop */
	scan = &scanlines.scanline[0];
	y = v1->y;
	ey = (v3->y < cliprect->max_y) ? v3->y : cliprect->max_y;

	/* loop until finished */
	while (1)
	{
		/* handle top clipping */
		if (y < cliprect->min_y)
		{
			/* get the delta */
			temp = cliprect->min_y - y;

			/* limit to the number of scanlines */
			if (temp > dy)
				temp = dy;
			dy -= temp;

			/* adjust x,y and all the parameters */
			y += temp;
			sx += sdx * temp;
			ex += edx * temp;
			if (NUM_PARAMS >= 0) pstart[0] += pdelta[0] * temp;
			if (NUM_PARAMS >= 1) pstart[1] += pdelta[1] * temp;
			if (NUM_PARAMS >= 2) pstart[2] += pdelta[2] * temp;
			if (NUM_PARAMS >= 3) pstart[3] += pdelta[3] * temp;
			if (NUM_PARAMS >= 4) pstart[4] += pdelta[4] * temp;
			if (NUM_PARAMS >= 5) pstart[5] += pdelta[5] * temp;
		}

		/* loop over scanlines */
		for (i = 0; i < dy; i++)
		{
			/* out at the bottom */
			if (y > ey)
				break;

			/* set the X start/end points */
			scan->sx = sx >> 16;
			scan->ex = (ex >> 16) - 1;

			/* compute the parameters only for non-zero scanlines */
			temp = (ex >> 16) - (sx >> 16);
			if (temp > 0)
			{
				if (NUM_PARAMS >= 0)
				{
					/* in order to get accurate texturing, we need to account for the fractional */
					/* pixel we just chopped off */
					temp = ~sx & 0xffff;
					scan->p[0] = pstart[0] + ((temp * scanlines.dp[0]) >> 16);
				}
				if (NUM_PARAMS >= 1) scan->p[1] = pstart[1] + ((temp * scanlines.dp[1]) >> 16);
				if (NUM_PARAMS >= 2) scan->p[2] = pstart[2] + ((temp * scanlines.dp[2]) >> 16);
				if (NUM_PARAMS >= 3) scan->p[3] = pstart[3] + ((temp * scanlines.dp[3]) >> 16);
				if (NUM_PARAMS >= 4) scan->p[4] = pstart[4] + ((temp * scanlines.dp[4]) >> 16);
				if (NUM_PARAMS >= 5) scan->p[5] = pstart[5] + ((temp * scanlines.dp[5]) >> 16);
			}
			scan++;

			/* update the parameters */
			y += 1;
			sx += sdx;
			ex += edx;
			if (NUM_PARAMS >= 0) pstart[0] += pdelta[0];
			if (NUM_PARAMS >= 1) pstart[1] += pdelta[1];
			if (NUM_PARAMS >= 2) pstart[2] += pdelta[2];
			if (NUM_PARAMS >= 3) pstart[3] += pdelta[3];
			if (NUM_PARAMS >= 4) pstart[4] += pdelta[4];
			if (NUM_PARAMS >= 5) pstart[5] += pdelta[5];
		}

		/* out at the bottom */
		if (y > ey)
			break;

		/* compute the height of part 2 */
		dy = v3->y - y;
		if (dy <= 0)
			break;

		/* if the longest scanline was negative, the middle vertex is to the right */
		if (longest_scanline < 0)
		{
			/* compute the new right slope parameters */
			ex = v2->x;
			edx = ((v3->x - ex) << 16) / dy;
			ex = (ex << 16) | 0xffff;
		}

		/* if the longest scanline was positive, the middle vertex is to the left */
		else
		{
			/* compute the new left slope parameters */
			sx = v2->x;
			sdx = ((v3->x - sx) << 16) / dy;
			sx = (sx << 16) | 0xffff;

			/* set up the starting values and per-scanline deltas for the parameters */
			if (NUM_PARAMS >= 0)
			{
				pstart[0] = ((INT64)v2->p[0] << 16) | 0x8000;
				pdelta[0] = ((INT64)(v3->p[0] - v2->p[0]) << 16) / dy;
			}
			if (NUM_PARAMS >= 1)
			{
				pstart[1] = ((INT64)v2->p[1] << 16) | 0x8000;
				pdelta[1] = ((INT64)(v3->p[1] - v2->p[1]) << 16) / dy;
			}
			if (NUM_PARAMS >= 2)
			{
				pstart[2] = ((INT64)v2->p[2] << 16) | 0x8000;
				pdelta[2] = ((INT64)(v3->p[2] - v2->p[2]) << 16) / dy;
			}
			if (NUM_PARAMS >= 3)
			{
				pstart[3] = ((INT64)v2->p[3] << 16) | 0x8000;
				pdelta[3] = ((INT64)(v3->p[3] - v2->p[3]) << 16) / dy;
			}
			if (NUM_PARAMS >= 4)
			{
				pstart[4] = ((INT64)v2->p[4] << 16) | 0x8000;
				pdelta[4] = ((INT64)(v3->p[4] - v2->p[4]) << 16) / dy;
			}
			if (NUM_PARAMS >= 5)
			{
				pstart[5] = ((INT64)v2->p[5] << 16) | 0x8000;
				pdelta[5] = ((INT64)(v3->p[5] - v2->p[5]) << 16) / dy;
			}
		}
	}

	/* apply clipping */
	while (--scan >= &scanlines.scanline[0])
	{
		/* left clip */
		if (scan->sx < cliprect->min_x)
		{
			temp = cliprect->min_x - scan->sx;
			scan->sx += temp;
			if (NUM_PARAMS >= 0) scan->p[0] += scanlines.dp[0] * temp;
			if (NUM_PARAMS >= 1) scan->p[1] += scanlines.dp[1] * temp;
			if (NUM_PARAMS >= 2) scan->p[2] += scanlines.dp[2] * temp;
			if (NUM_PARAMS >= 3) scan->p[3] += scanlines.dp[3] * temp;
			if (NUM_PARAMS >= 4) scan->p[4] += scanlines.dp[4] * temp;
			if (NUM_PARAMS >= 5) scan->p[5] += scanlines.dp[5] * temp;
		}

		/* right clip */
		if (scan->ex > cliprect->max_x)
			scan->ex = cliprect->max_x;
	}

	return &scanlines;
}

#endif
