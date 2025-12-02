/***************************************************************************
*   Copyright (C) 2010 PCSX4ALL Team                                      *
*   Copyright (C) 2010 Unai                                               *
*   Copyright (C) 2016 Senquack (dansilsby <AT> gmail <DOT> com)          *
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 2 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
*   This program is distributed in the hope that it will be useful,       *
*   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
*   GNU General Public License for more details.                          *
*                                                                         *
*   You should have received a copy of the GNU General Public License     *
*   along with this program; if not, write to the                         *
*   Free Software Foundation, Inc.,                                       *
*   51 Franklin Street, Fifth Floor, Boston, MA 02111-1307 USA.           *
***************************************************************************/

#ifndef __GPU_UNAI_GPU_RASTER_LINE_H__
#define __GPU_UNAI_GPU_RASTER_LINE_H__

///////////////////////////////////////////////////////////////////////////////
//  GPU internal line drawing functions
//
// Rewritten October 2016 by senquack:
//  Instead of one pixel at a time, lines are now drawn in runs of pixels,
//  whether vertical, horizontal, or diagonal. A new inner driver
//  'gpuPixelSpanFn' is used, as well as an enhanced Bresenham run-slice
//  algorithm. For more information, see the following:
//
//  Michael Abrash - Graphics Programming Black Book
//  Chapters 35 - 36 (does not implement diagonal runs)
//  http://www.drdobbs.com/parallel/graphics-programming-black-book/184404919
//  http://www.jagregory.com/abrash-black-book/
//
//  Article by Andrew Delong (does not implement diagonal runs)
//  http://timetraces.ca/nw/drawline.htm
//
//  'Run-Based Multi-Point Line Drawing' by Eun Jae Lee & Larry F. Hodges
//  https://smartech.gatech.edu/bitstream/handle/1853/3632/93-22.pdf
//  Provided the idea of doing a half-octant transform allowing lines with
//  slopes between 0.5 and 2.0 (diagonal runs of pixels) to be handled
//  identically to the traditional horizontal/vertical run-slice method.

// Use 16.16 fixed point precision for line math.
// NOTE: Gouraud colors used by gpuPixelSpanFn can use a different precision.
#define GPU_LINE_FIXED_BITS 16

// If defined, Gouraud lines will use fixed-point multiply-by-inverse to
// do most divisions. With enough accuracy, this should be OK.
#define USE_LINES_ALL_FIXED_PT_MATH

//////////////////////
// Flat-shaded line //
//////////////////////
void gpuDrawLineF(PtrUnion packet, const PSD gpuPixelSpanDriver)
{
	int x0, y0, x1, y1;
	int dx, dy;

	// All three of these variables should be signed (so multiplication works)
	ptrdiff_t sx;  // Sign of x delta, positive when x0 < x1
	const ptrdiff_t dst_depth  = FRAME_BYTES_PER_PIXEL; // PSX: 2 bytes per pixel
	const ptrdiff_t dst_stride = FRAME_BYTE_STRIDE;     // PSX: 2048 bytes per framebuffer line

	// Clip region: xmax/ymax seem to normally be one *past* the rightmost/
	//  bottommost pixels of the draw area. Since we render every pixel between
	//  and including both line endpoints, subtract one from xmax/ymax.
	const int xmin = gpu_unai.DrawingArea[0];
	const int ymin = gpu_unai.DrawingArea[1];
	const int xmax = gpu_unai.DrawingArea[2] - 1;
	const int ymax = gpu_unai.DrawingArea[3] - 1;

	x0 = GPU_EXPANDSIGN(le16_to_s16(packet.U2[2])) + gpu_unai.DrawingOffset[0];
	y0 = GPU_EXPANDSIGN(le16_to_s16(packet.U2[3])) + gpu_unai.DrawingOffset[1];
	x1 = GPU_EXPANDSIGN(le16_to_s16(packet.U2[4])) + gpu_unai.DrawingOffset[0];
	y1 = GPU_EXPANDSIGN(le16_to_s16(packet.U2[5])) + gpu_unai.DrawingOffset[1];

	// Always draw top to bottom, so ensure y0 <= y1
	if (y0 > y1) {
		SwapValues(y0, y1);
		SwapValues(x0, x1);
	}

	// Is line totally outside Y clipping range?
	if (y0 > ymax || y1 < ymin) return;

	dx = x1 - x0;
	dy = y1 - y0;

	// X-axis range check : max distance between any two X coords is 1023
	// (PSX hardware will not render anything violating this rule)
	// NOTE: We'll check y coord range further below
	if (dx >= CHKMAX_X || dx <= -CHKMAX_X)
		return;

	// Y-axis range check and clipping
	if (dy) {
		// Y-axis range check : max distance between any two Y coords is 511
		// (PSX hardware will not render anything violating this rule)
		if (dy >= CHKMAX_Y)
			return;

		// We already know y0 < y1
		if (y0 < ymin) {
			x0 += GPU_FAST_DIV(((ymin - y0) * dx), dy);
			y0 = ymin;
		}
		if (y1 > ymax) {
			x1 += GPU_FAST_DIV(((ymax - y1) * dx), dy);
			y1 = ymax;
		}

		// Recompute in case clipping occurred:
		dx = x1 - x0;
		dy = y1 - y0;
	}

	// Check X clipping range, set 'sx' x-direction variable
	if (dx == 0) {
		// Is vertical line totally outside X clipping range?
		if (x0 < xmin || x0 > xmax)
			return;
		sx = 0;
	} else {
		if (dx > 0) {
			// x0 is leftmost coordinate
			if (x0 > xmax) return; // Both points outside X clip range

			if (x0 < xmin) {
				if (x1 < xmin) return; // Both points outside X clip range
				y0 += GPU_FAST_DIV(((xmin - x0) * dy), dx);
				x0 = xmin;
			}

			if (x1 > xmax) {
				y1 += GPU_FAST_DIV(((xmax - x1) * dy), dx);
				x1 = xmax;
			}

			sx = +1;
			dx = x1 - x0; // Get final value, which should also be absolute value
		} else {
			// x1 is leftmost coordinate
			if (x1 > xmax) return; // Both points outside X clip range

			if (x1 < xmin) {
				if (x0 < xmin) return; // Both points outside X clip range

				y1 += GPU_FAST_DIV(((xmin - x1) * dy), dx);
				x1 = xmin;
			}

			if (x0 > xmax) {
				y0 += GPU_FAST_DIV(((xmax - x0) * dy), dx);
				x0 = xmax;
			}

			sx = -1;
			dx = x0 - x1; // Get final value, which should also be absolute value
		}

		// Recompute in case clipping occurred:
		dy = y1 - y0;
	}

	// IMPORTANT: dx,dy should now contain their absolute values

	int min_length,    // Minimum length of a pixel run
	    start_length,  // Length of first run
	    end_length,    // Length of last run
	    err_term,      // Cumulative error to determine when to draw longer run
	    err_adjup,     // Increment to err_term for each run drawn
	    err_adjdown;   // Subract this from err_term after drawing longer run

	// Color to draw with (16 bits, highest of which is unset mask bit)
	uintptr_t col16 = GPU_RGB16(le32_to_u32(packet.U4[0]));

	le16_t *dst = gpu_unai.vram + (y0 * dst_stride + x0 * dst_depth) / FRAME_BYTES_PER_PIXEL;

	// SPECIAL CASE: Vertical line
	if (dx == 0) {
		gpuPixelSpanDriver(dst, col16, dst_stride, dy+1);
		return;
	}

	// SPECIAL CASE: Horizontal line
	if (dy == 0) {
		gpuPixelSpanDriver(dst, col16, sx * dst_depth, dx+1);
		return;
	}

	// SPECIAL CASE: Diagonal line
	if (dx == dy) {
		gpuPixelSpanDriver(dst, col16, dst_stride + (sx * dst_depth), dy+1);
		return;
	}

	int       major, minor;             // Major axis, minor axis
	ptrdiff_t incr_major, incr_minor;   // Ptr increment for each step along axis

	if (dx > dy) {
		major = dx;
		minor = dy;
	} else {
		major = dy;
		minor = dx;
	}

	// Determine if diagonal or horizontal runs
	if (major < (2 * minor)) {
		// Diagonal runs, so perform half-octant transformation
		minor = major - minor;

		// Advance diagonally when drawing runs
		incr_major = dst_stride + (sx * dst_depth);

		// After drawing each run, correct for over-advance along minor axis
		if (dx > dy)
			incr_minor = -dst_stride;
		else
			incr_minor = -sx * dst_depth;
	} else {
		// Horizontal or vertical runs
		if (dx > dy) {
			incr_major = sx * dst_depth;
			incr_minor = dst_stride;
		} else {
			incr_major = dst_stride;
			incr_minor = sx * dst_depth;
		}
	}

	if (minor > 1) {
		// Minimum number of pixels each run
		min_length = major / minor;

		// Initial error term; reflects an initial step of 0.5 along minor axis
		err_term = (major % minor) - (minor * 2);

		// Increment err_term this much each step along minor axis; when
		//  err_term crosses zero, draw longer pixel run.
		err_adjup = (major % minor) * 2;
	} else {
		min_length = major;
		err_term = 0;
		err_adjup = 0;
	}

	// Error term adjustment when err_term turns over; used to factor
	//  out the major-axis step made at that time
	err_adjdown = minor * 2;

	// The initial and last runs are partial, because minor axis advances
	//  only 0.5 for these runs, rather than 1. Each is half a full run,
	//  plus the initial pixel.
	start_length = end_length = (min_length / 2) + 1;

	if (min_length & 1) {
		// If there're an odd number of pixels per run, we have 1 pixel that
		//  can't be allocated to either the initial or last partial run, so
		//  we'll add 0.5 to err_term so that this pixel will be handled
		//  by the normal full-run loop
		err_term += minor;
	} else {
		// If the minimum run length is even and there's no fractional advance,
		// we have one pixel that could go to either the initial or last
		// partial run, which we arbitrarily allocate to the last run
		if (err_adjup == 0)
			start_length--; // Leave out the extra pixel at the start
	}

	// First run of pixels
	dst = gpuPixelSpanDriver(dst, col16, incr_major, start_length);
	dst += incr_minor / 2;

	// Middle runs of pixels
	while (--minor > 0) {
		int run_length = min_length;
		err_term += err_adjup;

		// If err_term passed 0, reset it and draw longer run
		if (err_term > 0) {
			err_term -= err_adjdown;
			run_length++;
		}

		dst = gpuPixelSpanDriver(dst, col16, incr_major, run_length);
		dst += incr_minor / 2;
	}

	// Final run of pixels
	gpuPixelSpanDriver(dst, col16, incr_major, end_length);
}

/////////////////////////
// Gouraud-shaded line //
/////////////////////////
void gpuDrawLineG(PtrUnion packet, const PSD gpuPixelSpanDriver)
{
	int x0, y0, x1, y1;
	int dx, dy, dr, dg, db;
	u32 r0, g0, b0, r1, g1, b1;

	// All three of these variables should be signed (so multiplication works)
	ptrdiff_t sx;  // Sign of x delta, positive when x0 < x1
	const ptrdiff_t dst_depth  = FRAME_BYTES_PER_PIXEL; // PSX: 2 bytes per pixel
	const ptrdiff_t dst_stride = FRAME_BYTE_STRIDE;     // PSX: 2048 bytes per framebuffer line

	// Clip region: xmax/ymax seem to normally be one *past* the rightmost/
	//  bottommost pixels of the draw area. We'll render every pixel between
	//  and including both line endpoints, so subtract one from xmax/ymax.
	const int xmin = gpu_unai.DrawingArea[0];
	const int ymin = gpu_unai.DrawingArea[1];
	const int xmax = gpu_unai.DrawingArea[2] - 1;
	const int ymax = gpu_unai.DrawingArea[3] - 1;

	x0 = GPU_EXPANDSIGN(le16_to_s16(packet.U2[2])) + gpu_unai.DrawingOffset[0];
	y0 = GPU_EXPANDSIGN(le16_to_s16(packet.U2[3])) + gpu_unai.DrawingOffset[1];
	x1 = GPU_EXPANDSIGN(le16_to_s16(packet.U2[6])) + gpu_unai.DrawingOffset[0];
	y1 = GPU_EXPANDSIGN(le16_to_s16(packet.U2[7])) + gpu_unai.DrawingOffset[1];

	u32 col0 = le32_to_u32(packet.U4[0]);
	u32 col1 = le32_to_u32(packet.U4[2]);

	// Always draw top to bottom, so ensure y0 <= y1
	if (y0 > y1) {
		SwapValues(y0, y1);
		SwapValues(x0, x1);
		SwapValues(col0, col1);
	}

	// Is line totally outside Y clipping range?
	if (y0 > ymax || y1 < ymin) return;

	// If defined, Gouraud colors are fixed-point 5.11, otherwise they are 8.16
	// (This is only beneficial if using SIMD-optimized pixel driver)
#ifdef GPU_GOURAUD_LOW_PRECISION
	r0 = (col0 >> 3) & 0x1f;  g0 = (col0 >> 11) & 0x1f;  b0 = (col0 >> 19) & 0x1f;
	r1 = (col1 >> 3) & 0x1f;  g1 = (col1 >> 11) & 0x1f;  b1 = (col1 >> 19) & 0x1f;
#else
	r0 = col0 & 0xff;  g0 = (col0 >> 8) & 0xff;  b0 = (col0 >> 16) & 0xff;
	r1 = col1 & 0xff;  g1 = (col1 >> 8) & 0xff;  b1 = (col1 >> 16) & 0xff;
#endif

	dx = x1 - x0;
	dy = y1 - y0;
	dr = r1 - r0;
	dg = g1 - g0;
	db = b1 - b0;

	// X-axis range check : max distance between any two X coords is 1023
	// (PSX hardware will not render anything violating this rule)
	// NOTE: We'll check y coord range further below
	if (dx >= CHKMAX_X || dx <= -CHKMAX_X)
		return;

	// Y-axis range check and clipping
	if (dy) {
		// Y-axis range check : max distance between any two Y coords is 511
		// (PSX hardware will not render anything violating this rule)
		if (dy >= CHKMAX_Y)
			return;

		// We already know y0 < y1
		if (y0 < ymin) {
#ifdef USE_LINES_ALL_FIXED_PT_MATH
			s32 factor = GPU_FAST_DIV(((ymin - y0) << GPU_LINE_FIXED_BITS), dy);
			x0 += (dx * factor) >> GPU_LINE_FIXED_BITS;
			r0 += (dr * factor) >> GPU_LINE_FIXED_BITS;
			g0 += (dg * factor) >> GPU_LINE_FIXED_BITS;
			b0 += (db * factor) >> GPU_LINE_FIXED_BITS;
#else
			x0 += (ymin - y0) * dx / dy;
			r0 += (ymin - y0) * dr / dy;
			g0 += (ymin - y0) * dg / dy;
			b0 += (ymin - y0) * db / dy;
#endif
			y0 = ymin;
		}

		if (y1 > ymax) {
#ifdef USE_LINES_ALL_FIXED_PT_MATH
			s32 factor = GPU_FAST_DIV(((ymax - y1) << GPU_LINE_FIXED_BITS), dy);
			x1 += (dx * factor) >> GPU_LINE_FIXED_BITS;
			r1 += (dr * factor) >> GPU_LINE_FIXED_BITS;
			g1 += (dg * factor) >> GPU_LINE_FIXED_BITS;
			b1 += (db * factor) >> GPU_LINE_FIXED_BITS;
#else
			x1 += (ymax - y1) * dx / dy;
			r1 += (ymax - y1) * dr / dy;
			g1 += (ymax - y1) * dg / dy;
			b1 += (ymax - y1) * db / dy;
#endif
			y1 = ymax;
		}

		// Recompute in case clipping occurred:
		dx = x1 - x0;
		dy = y1 - y0;
		dr = r1 - r0;
		dg = g1 - g0;
		db = b1 - b0;
	}

	// Check X clipping range, set 'sx' x-direction variable
	if (dx == 0) {
		// Is vertical line totally outside X clipping range?
		if (x0 < xmin || x0 > xmax)
			return;
		sx = 0;
	} else {
		if (dx > 0) {
			// x0 is leftmost coordinate
			if (x0 > xmax) return; // Both points outside X clip range

			if (x0 < xmin) {
				if (x1 < xmin) return; // Both points outside X clip range

#ifdef USE_LINES_ALL_FIXED_PT_MATH
				s32 factor = GPU_FAST_DIV(((xmin - x0) << GPU_LINE_FIXED_BITS), dx);
				y0 += (dy * factor) >> GPU_LINE_FIXED_BITS;
				r0 += (dr * factor) >> GPU_LINE_FIXED_BITS;
				g0 += (dg * factor) >> GPU_LINE_FIXED_BITS;
				b0 += (db * factor) >> GPU_LINE_FIXED_BITS;
#else
				y0 += (xmin - x0) * dy / dx;
				r0 += (xmin - x0) * dr / dx;
				g0 += (xmin - x0) * dg / dx;
				b0 += (xmin - x0) * db / dx;
#endif
				x0 = xmin;
			}

			if (x1 > xmax) {
#ifdef USE_LINES_ALL_FIXED_PT_MATH
				s32 factor = GPU_FAST_DIV(((xmax - x1) << GPU_LINE_FIXED_BITS), dx);
				y1 += (dy * factor) >> GPU_LINE_FIXED_BITS;
				r1 += (dr * factor) >> GPU_LINE_FIXED_BITS;
				g1 += (dg * factor) >> GPU_LINE_FIXED_BITS;
				b1 += (db * factor) >> GPU_LINE_FIXED_BITS;
#else
				y1 += (xmax - x1) * dy / dx;
				r1 += (xmax - x1) * dr / dx;
				g1 += (xmax - x1) * dg / dx;
				b1 += (xmax - x1) * db / dx;
#endif
				x1 = xmax;
			}

			sx = +1;
			dx = x1 - x0; // Get final value, which should also be absolute value
		} else {
			// x1 is leftmost coordinate
			if (x1 > xmax) return; // Both points outside X clip range

			if (x1 < xmin) {
				if (x0 < xmin) return; // Both points outside X clip range

#ifdef USE_LINES_ALL_FIXED_PT_MATH
				s32 factor = GPU_FAST_DIV(((xmin - x1) << GPU_LINE_FIXED_BITS), dx);
				y1 += (dy * factor) >> GPU_LINE_FIXED_BITS;
				r1 += (dr * factor) >> GPU_LINE_FIXED_BITS;
				g1 += (dg * factor) >> GPU_LINE_FIXED_BITS;
				b1 += (db * factor) >> GPU_LINE_FIXED_BITS;
#else
				y1 += (xmin - x1) * dy / dx;
				r1 += (xmin - x1) * dr / dx;
				g1 += (xmin - x1) * dg / dx;
				b1 += (xmin - x1) * db / dx;
#endif
				x1 = xmin;
			}

			if (x0 > xmax) {
#ifdef USE_LINES_ALL_FIXED_PT_MATH
				s32 factor = GPU_FAST_DIV(((xmax - x0) << GPU_LINE_FIXED_BITS), dx);
				y0 += (dy * factor) >> GPU_LINE_FIXED_BITS;
				r0 += (dr * factor) >> GPU_LINE_FIXED_BITS;
				g0 += (dg * factor) >> GPU_LINE_FIXED_BITS;
				b0 += (db * factor) >> GPU_LINE_FIXED_BITS;
#else
				y0 += (xmax - x0) * dy / dx;
				r0 += (xmax - x0) * dr / dx;
				g0 += (xmax - x0) * dg / dx;
				b0 += (xmax - x0) * db / dx;
#endif
				x0 = xmax;
			}

			sx = -1;
			dx = x0 - x1; // Get final value, which should also be absolute value
		}

		// Recompute in case clipping occurred:
		dy = y1 - y0;
		dr = r1 - r0;
		dg = g1 - g0;
		db = b1 - b0;
	}

	// IMPORTANT: dx,dy should now contain their absolute values

	int min_length,    // Minimum length of a pixel run
	    start_length,  // Length of first run
	    end_length,    // Length of last run
	    err_term,      // Cumulative error to determine when to draw longer run
	    err_adjup,     // Increment to err_term for each run drawn
	    err_adjdown;   // Subract this from err_term after drawing longer run

	GouraudColor gcol;
	gcol.r = r0 << GPU_GOURAUD_FIXED_BITS;
	gcol.g = g0 << GPU_GOURAUD_FIXED_BITS;
	gcol.b = b0 << GPU_GOURAUD_FIXED_BITS;

	le16_t *dst = gpu_unai.vram + (y0 * dst_stride + x0 * dst_depth) / FRAME_BYTES_PER_PIXEL;

	// SPECIAL CASE: Vertical line
	if (dx == 0) {
#ifdef USE_LINES_ALL_FIXED_PT_MATH
		// Get dy fixed-point inverse
		s32 inv_factor = 1 << GPU_GOURAUD_FIXED_BITS;
		if (dy > 1) inv_factor = GPU_FAST_DIV(inv_factor, dy);

		// Simultaneously divide and convert integer to Gouraud fixed point:
		gcol.r_incr = dr * inv_factor;
		gcol.g_incr = dg * inv_factor;
		gcol.b_incr = db * inv_factor;
#else
		// First, convert to Gouraud fixed point
		gcol.r_incr = dr << GPU_GOURAUD_FIXED_BITS;
		gcol.g_incr = dg << GPU_GOURAUD_FIXED_BITS;
		gcol.b_incr = db << GPU_GOURAUD_FIXED_BITS;

		if (dy > 1) {
			if (dr) gcol.r_incr /= dy;
			if (dg) gcol.g_incr /= dy;
			if (db) gcol.b_incr /= dy;
		}
#endif

		gpuPixelSpanDriver(dst, (uintptr_t)&gcol, dst_stride, dy+1);
		return;
	}

	// SPECIAL CASE: Horizontal line
	if (dy == 0) {
#ifdef USE_LINES_ALL_FIXED_PT_MATH
		// Get dx fixed-point inverse
		s32 inv_factor = (1 << GPU_GOURAUD_FIXED_BITS);
		if (dx > 1) inv_factor = GPU_FAST_DIV(inv_factor, dx);

		// Simultaneously divide and convert integer to Gouraud fixed point:
		gcol.r_incr = dr * inv_factor;
		gcol.g_incr = dg * inv_factor;
		gcol.b_incr = db * inv_factor;
#else
		gcol.r_incr = dr << GPU_GOURAUD_FIXED_BITS;
		gcol.g_incr = dg << GPU_GOURAUD_FIXED_BITS;
		gcol.b_incr = db << GPU_GOURAUD_FIXED_BITS;

		if (dx > 1) {
			if (dr) gcol.r_incr /= dx;
			if (dg) gcol.g_incr /= dx;
			if (db) gcol.b_incr /= dx;
		}
#endif

		gpuPixelSpanDriver(dst, (uintptr_t)&gcol, sx * dst_depth, dx+1);
		return;
	}

	// SPECIAL CASE: Diagonal line
	if (dx == dy) {
#ifdef USE_LINES_ALL_FIXED_PT_MATH
		// Get dx fixed-point inverse
		s32 inv_factor = (1 << GPU_GOURAUD_FIXED_BITS);
		if (dx > 1) inv_factor = GPU_FAST_DIV(inv_factor, dx);

		// Simultaneously divide and convert integer to Gouraud fixed point:
		gcol.r_incr = dr * inv_factor;
		gcol.g_incr = dg * inv_factor;
		gcol.b_incr = db * inv_factor;
#else
		// First, convert to Gouraud fixed point
		gcol.r_incr = dr << GPU_GOURAUD_FIXED_BITS;
		gcol.g_incr = dg << GPU_GOURAUD_FIXED_BITS;
		gcol.b_incr = db << GPU_GOURAUD_FIXED_BITS;

		if (dx > 1) {
			if (dr) gcol.r_incr /= dx;
			if (dg) gcol.g_incr /= dx;
			if (db) gcol.b_incr /= dx;
		}
#endif

		gpuPixelSpanDriver(dst, (uintptr_t)&gcol, dst_stride + (sx * dst_depth), dy+1);
		return;
	}

	int       major, minor;             // Absolute val of major,minor axis delta
	ptrdiff_t incr_major, incr_minor;   // Ptr increment for each step along axis

	if (dx > dy) {
		major = dx;
		minor = dy;
	} else {
		major = dy;
		minor = dx;
	}

	// Determine if diagonal or horizontal runs
	if (major < (2 * minor)) {
		// Diagonal runs, so perform half-octant transformation
		minor = major - minor;

		// Advance diagonally when drawing runs
		incr_major = dst_stride + (sx * dst_depth);

		// After drawing each run, correct for over-advance along minor axis
		if (dx > dy)
			incr_minor = -dst_stride;
		else
			incr_minor = -sx * dst_depth;
	} else {
		// Horizontal or vertical runs
		if (dx > dy) {
			incr_major = sx * dst_depth;
			incr_minor = dst_stride;
		} else {
			incr_major = dst_stride;
			incr_minor = sx * dst_depth;
		}
	}

#ifdef USE_LINES_ALL_FIXED_PT_MATH
	s32 major_inv = GPU_FAST_DIV((1 << GPU_GOURAUD_FIXED_BITS), major);

	// Simultaneously divide and convert from integer to Gouraud fixed point:
	gcol.r_incr = dr * major_inv;
	gcol.g_incr = dg * major_inv;
	gcol.b_incr = db * major_inv;
#else
	gcol.r_incr = dr ? ((dr << GPU_GOURAUD_FIXED_BITS) / major) : 0;
	gcol.g_incr = dg ? ((dg << GPU_GOURAUD_FIXED_BITS) / major) : 0;
	gcol.b_incr = db ? ((db << GPU_GOURAUD_FIXED_BITS) / major) : 0;
#endif

	if (minor > 1) {
		// Minimum number of pixels each run
		min_length = major / minor;

		// Initial error term; reflects an initial step of 0.5 along minor axis
		err_term = (major % minor) - (minor * 2);

		// Increment err_term this much each step along minor axis; when
		//  err_term crosses zero, draw longer pixel run.
		err_adjup = (major % minor) * 2;
	} else {
		min_length = major;
		err_term = 0;
		err_adjup = 0;
	}

	// Error term adjustment when err_term turns over; used to factor
	//  out the major-axis step made at that time
	err_adjdown = minor * 2;

	// The initial and last runs are partial, because minor axis advances
	//  only 0.5 for these runs, rather than 1. Each is half a full run,
	//  plus the initial pixel.
	start_length = end_length = (min_length / 2) + 1;

	if (min_length & 1) {
		// If there're an odd number of pixels per run, we have 1 pixel that
		//  can't be allocated to either the initial or last partial run, so
		//  we'll add 0.5 to err_term so that this pixel will be handled
		//  by the normal full-run loop
		err_term += minor;
	} else {
		// If the minimum run length is even and there's no fractional advance,
		// we have one pixel that could go to either the initial or last
		// partial run, which we'll arbitrarily allocate to the last run
		if (err_adjup == 0)
			start_length--; // Leave out the extra pixel at the start
	}

	// First run of pixels
	dst = gpuPixelSpanDriver(dst, (uintptr_t)&gcol, incr_major, start_length);
	dst += incr_minor / 2;

	// Middle runs of pixels
	while (--minor > 0) {
		int run_length = min_length;
		err_term += err_adjup;

		// If err_term passed 0, reset it and draw longer run
		if (err_term > 0) {
			err_term -= err_adjdown;
			run_length++;
		}

		dst = gpuPixelSpanDriver(dst, (uintptr_t)&gcol, incr_major, run_length);
		dst += incr_minor / 2;
	}

	// Final run of pixels
	gpuPixelSpanDriver(dst, (uintptr_t)&gcol, incr_major, end_length);
}

#endif /* __GPU_UNAI_GPU_RASTER_LINE_H__ */
