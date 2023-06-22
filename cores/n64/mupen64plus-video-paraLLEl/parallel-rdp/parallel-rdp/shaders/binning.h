/* Copyright (c) 2020 Themaister
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef BINNING_H_
#define BINNING_H_

// There are 4 critical Y coordinates to test when binning. Top, bottom, mid, and mid - 1.

const int SUBPIXELS_Y = 4;

ivec4 quantize_x(ivec4 x)
{
	return x >> 15;
}

int minimum4(ivec4 v)
{
	ivec2 minimum2 = min(v.xy, v.zw);
	return min(minimum2.x, minimum2.y);
}

int maximum4(ivec4 v)
{
	ivec2 maximum2 = max(v.xy, v.zw);
	return max(maximum2.x, maximum2.y);
}

ivec2 interpolate_xs(TriangleSetup setup, ivec4 ys, bool flip, int scaling)
{
	int yh_interpolation_base = setup.yh & ~(SUBPIXELS_Y - 1);
	int ym_interpolation_base = setup.ym;

	yh_interpolation_base *= scaling;
	ym_interpolation_base *= scaling;

	ivec4 xh = scaling * setup.xh + (ys - yh_interpolation_base) * setup.dxhdy;
	ivec4 xm = scaling * setup.xm + (ys - yh_interpolation_base) * setup.dxmdy;
	ivec4 xl = scaling * setup.xl + (ys - ym_interpolation_base) * setup.dxldy;
	xl = mix(xl, xm, lessThan(ys, ivec4(scaling * setup.ym)));

	ivec4 xh_shifted = quantize_x(xh);
	ivec4 xl_shifted = quantize_x(xl);

	ivec4 xleft, xright;
	if (flip)
	{
		xleft = xh_shifted;
		xright = xl_shifted;
	}
	else
	{
		xleft = xl_shifted;
		xright = xh_shifted;
	}

	return ivec2(minimum4(xleft), maximum4(xright));
}

bool bin_primitive(TriangleSetup setup, ivec2 lo, ivec2 hi, int scaling)
{
    int start_y = lo.y * SUBPIXELS_Y;
    int end_y = (hi.y * SUBPIXELS_Y) + (SUBPIXELS_Y - 1);

    // First, we clip start/end against y_lo, y_hi.
    start_y = max(start_y, scaling * int(setup.yh));
    end_y = min(end_y, scaling * int(setup.yl) - 1);

    // Y is clipped out, exit early.
    if (end_y < start_y)
        return false;

    bool flip = (setup.flags & TRIANGLE_SETUP_FLIP_BIT) != 0;

    // Sample the X ranges for min and max Y, and potentially the mid-point as well.
    ivec4 ys = ivec4(start_y, end_y, clamp(setup.ym * scaling + ivec2(-1, 0), ivec2(start_y), ivec2(end_y)));
    ivec2 x_range = interpolate_xs(setup, ys, flip, scaling);
    x_range.x = max(x_range.x, lo.x);
	x_range.y = min(x_range.y, hi.x);
	return x_range.x <= x_range.y;
}

#endif