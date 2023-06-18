/***************************************************************************

	poly.h

	Helper routines for polygon rendering.

***************************************************************************/

#include "driver.h"


/* up to 6 params (Z,U,V,R,G,B), plus it makes the vertex struct a nice size */
#define MAX_VERTEX_PARAMS		6
#define MAX_POLY_SCANLINES		512

struct poly_vertex
{
	INT32	x;							/* 16.0 screen X coordinate */
	INT32	y;							/* 16.0 screen Y coordinate */
	INT32	p[MAX_VERTEX_PARAMS];		/* 32.0 interpolated parameter values */
};

struct poly_scanline
{
	INT32	sx, ex;						/* 16.0 starting and ending X coordinates */
	INT64	p[MAX_VERTEX_PARAMS];		/* 32.16 starting parameter values (at left) */
};

struct poly_scanline_data
{
	INT32	sy, ey;						/* 16.0 starting and ending Y coordinates */
	INT64	dp[MAX_VERTEX_PARAMS];		/* 32.16 per-pixel deltas for each parameter */
	struct poly_scanline scanline[MAX_POLY_SCANLINES];
};

const struct poly_scanline_data *setup_triangle_0(const struct poly_vertex *v1, const struct poly_vertex *v2, const struct poly_vertex *v3, const struct rectangle *cliprect);
const struct poly_scanline_data *setup_triangle_1(const struct poly_vertex *v1, const struct poly_vertex *v2, const struct poly_vertex *v3, const struct rectangle *cliprect);
const struct poly_scanline_data *setup_triangle_2(const struct poly_vertex *v1, const struct poly_vertex *v2, const struct poly_vertex *v3, const struct rectangle *cliprect);
const struct poly_scanline_data *setup_triangle_3(const struct poly_vertex *v1, const struct poly_vertex *v2, const struct poly_vertex *v3, const struct rectangle *cliprect);
const struct poly_scanline_data *setup_triangle_4(const struct poly_vertex *v1, const struct poly_vertex *v2, const struct poly_vertex *v3, const struct rectangle *cliprect);
const struct poly_scanline_data *setup_triangle_5(const struct poly_vertex *v1, const struct poly_vertex *v2, const struct poly_vertex *v3, const struct rectangle *cliprect);
const struct poly_scanline_data *setup_triangle_6(const struct poly_vertex *v1, const struct poly_vertex *v2, const struct poly_vertex *v3, const struct rectangle *cliprect);
