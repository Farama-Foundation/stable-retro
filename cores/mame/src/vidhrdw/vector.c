/******************************************************************************
 *
 * vector.c
 *
 *
 * Copyright 1997,1998 by the M.A.M.E. Project
 *
 *        anti-alias code by Andrew Caldwell
 *        (still more to add)
 *
 * 980611 use translucent vectors. Thanks to Peter Hirschberg
 *        and Neil Bradley for the inspiration. BW
 * 980307 added cleverer dirty handling. BW, ASG
 *        fixed antialias table .ac
 * 980221 rewrote anti-alias line draw routine
 *        added inline assembly multiply fuction for 8086 based machines
 *        beam diameter added to draw routine
 *        beam diameter is accurate in anti-alias line draw (Tcosin)
 *        flicker added .ac
 * 980203 moved LBO's routines for drawing into a buffer of vertices
 *        from avgdvg.c to this location. Scaling is now initialized
 *        by calling vector_init(...). BW
 * 980202 moved out of msdos.c ASG
 * 980124 added anti-alias line draw routine
 *        modified avgdvg.c and sega.c to support new line draw routine
 *        added two new tables Tinten and Tmerge (for 256 color support)
 *        added find_color routine to build above tables .ac
 * 010903 added support for direct RGB modes MLR
 *
 **************************************************************************** */

/* GLmame and FXmame provide their own vector implementations */
#if !(defined xgl)

#include <math.h>
#include "osd_cpu.h"
#include "driver.h"
#include "vector.h"
#include "artwork.h"

#define MAX_DIRTY_PIXELS (2*MAX_PIXELS)

unsigned char *vectorram;
size_t vectorram_size;

static int int_beam;                      /* size of vector beam    */
static int beam_diameter_is_one;		      /* flag that beam is one pixel wide */

static float vector_scale_x;              /* scaling to screen */
static float vector_scale_y;              /* scaling to screen */

static float gamma_correction = 1.2f;

static int (*vector_aux_renderer)(point *start, int num_points) = NULL;

static point *new_list;
static point *old_list;
static int new_index;
static int old_index;

/* coordinates of pixels are stored here for faster removal */
vector_pixel_t *vector_dirty_list;
static int dirty_index;

static vector_pixel_t *pixel;
static int p_index=0;

static UINT32 *pTcosin;            /* adjust line width */

#define Tcosin(x)   pTcosin[(x)]          /* adjust line width */

#define ANTIALIAS_GUNBIT  6             /* 6 bits per gun in vga (1-8 valid) */
#define ANTIALIAS_GUNNUM  (1<<ANTIALIAS_GUNBIT)

static UINT8 Tgamma[256];         /* quick gamma anti-alias table  */
static UINT8 Tgammar[256];        /* same as above, reversed order */

static struct mame_bitmap *vecbitmap;
static int vecwidth, vecheight;
static int xmin, ymin, xmax, ymax; /* clipping area */

static int vector_runs;	/* vector runs per refresh */

static void (*vector_draw_aa_pixel)(int x, int y, rgb_t col, int dirty);

static void vector_draw_aa_pixel_15 (int x, int y, rgb_t col, int dirty);
static void vector_draw_aa_pixel_32 (int x, int y, rgb_t col, int dirty);

void vector_register_aux_renderer(int (*aux_renderer)(point *start, int num_points))
{
	vector_aux_renderer = aux_renderer;
}

/*
 * multiply and divide routines for drawing lines
 * can be be replaced by an assembly routine in osd_cpu.h
 */
#ifndef vec_mult
static INLINE int vec_mult(int parm1, int parm2)
{
	int temp,result;

	temp     = abs(parm1);
	result   = (temp&0x0000ffff) * (parm2&0x0000ffff);
	result >>= 16;
	result  += (temp&0x0000ffff) * (parm2>>16       );
	result  += (temp>>16       ) * (parm2&0x0000ffff);
	result >>= 16;
	result  += (temp>>16       ) * (parm2>>16       );

	if( parm1 < 0 )
		return(-result);
	else
		return( result);
}
#endif

/* can be be replaced by an assembly routine in osd_cpu.h */
#ifndef vec_div
static INLINE int vec_div(int parm1, int parm2)
{
	if( (parm2>>12) )
	{
		parm1 = (parm1<<4) / (parm2>>12);
		if( parm1 > 0x00010000 )
			return( 0x00010000 );
		if( parm1 < -0x00010000 )
			return( -0x00010000 );
		return( parm1 );
	}
	return( 0x00010000 );
}
#endif

/* MLR 990316 new gamma handling added */
void vector_set_gamma(float _gamma)
{
	int i, h;

	gamma_correction = _gamma;

	for (i = 0; i < 256; i++)
	{
		h = 255.0*pow(i/255.0, 1.0/gamma_correction);
		if( h > 255) h = 255;
		Tgamma[i] = Tgammar[255-i] = h;
	}
}

float vector_get_gamma(void)
{
	return gamma_correction;
}

static void update_options(void)
{

	/* Beam width is encoded as fixed point */
	int_beam = (int)(options.beam * 0x00010000);
	int_beam = int_beam > 0x00100000 ? 0x00100000 : int_beam;
	int_beam = int_beam < 0x00010000 ? 0x00010000 : int_beam;
	beam_diameter_is_one = int_beam == 0x00010000;
}

/*
 * Initializes vector game video emulation
 */

VIDEO_START( vector )
{
	int i;

  /* Set initial rendering options */
  update_options();

	p_index = 0;

	new_index = 0;
	old_index = 0;
	vector_runs = 0;

	switch(Machine->color_depth)
	{
	case 15:
		vector_draw_aa_pixel = vector_draw_aa_pixel_15;
		break;
	case 32:
		vector_draw_aa_pixel = vector_draw_aa_pixel_32;
		break;
	default:
		logerror ("Vector games have to use direct RGB modes!\n");
		return 1;
		break;
	}

	/* allocate memory for tables */
	pTcosin = auto_malloc ( (2048+1) * sizeof(pTcosin[0]));   /* yes! 2049 is correct */
	pixel = auto_malloc (MAX_PIXELS * sizeof (pixel[0]));
	vector_dirty_list = auto_malloc (MAX_DIRTY_PIXELS * sizeof (vector_dirty_list[0]));
	old_list = auto_malloc (MAX_POINTS * sizeof (old_list[0]));
	new_list = auto_malloc (MAX_POINTS * sizeof (new_list[0]));

	/* did we get the requested memory? */
	if (!(pTcosin && pixel && old_list && new_list && vector_dirty_list))
		return 1;

	/* build cosine table for fixing line width in antialias */
	for (i=0; i<=2048; i++)
	{
		Tcosin(i) = (int)((double)(1.0/cos(atan((double)(i)/2048.0)))*0x10000000 + 0.5);
	}

	/* build gamma correction table */
	vector_set_gamma (gamma_correction);

	/* make sure we reset the list */
	vector_dirty_list[0] = VECTOR_PIXEL_END;
	
	return 0;
}


/*
 * Clear the old bitmap. Delete pixel for pixel, this is faster than memset.
 */
static void vector_clear_pixels (void)
{
	vector_pixel_t coords;
	int i;

	if (Machine->color_depth == 32)
	{
		for (i=p_index-1; i>=0; i--)
		{
			coords = pixel[i];
			((UINT32 *)vecbitmap->line[VECTOR_PIXEL_Y(coords)])[VECTOR_PIXEL_X(coords)] = 0;
		}
	}
	else
	{
		for (i=p_index-1; i>=0; i--)
		{
			coords = pixel[i];
			((UINT16 *)vecbitmap->line[VECTOR_PIXEL_Y(coords)])[VECTOR_PIXEL_X(coords)] = 0;
		}
	}
	p_index=0;
}

/*
 * draws an anti-aliased pixel (blends pixel with background)
 */
#define LIMIT5(x) ((x < 0x1f)? x : 0x1f)
#define LIMIT8(x) ((x < 0xff)? x : 0xff)

static void vector_draw_aa_pixel_15 (int x, int y, rgb_t col, int dirty)
{
	vector_pixel_t coords;
	UINT32 dst;

	if (x < xmin || x >= xmax)
		return;
	if (y < ymin || y >= ymax)
		return;

	dst = ((UINT16 *)vecbitmap->line[y])[x];
	((UINT16 *)vecbitmap->line[y])[x] = LIMIT5((RGB_BLUE(col) >> 3) + (dst & 0x1f))
		| (LIMIT5((RGB_GREEN(col) >> 3) + ((dst >> 5) & 0x1f)) << 5)
		| (LIMIT5((RGB_RED(col) >> 3) + (dst >> 10)) << 10);

	coords = VECTOR_PIXEL(x,y);
	if (p_index<MAX_PIXELS)
		pixel[p_index++] = coords;

	/* Mark this pixel as dirty */
	if (dirty_index<MAX_DIRTY_PIXELS)
		vector_dirty_list[dirty_index++] = coords;
}

static void vector_draw_aa_pixel_32 (int x, int y, rgb_t col, int dirty)
{
	vector_pixel_t coords;
	UINT32 dst;

	if (x < xmin || x >= xmax)
		return;
	if (y < ymin || y >= ymax)
		return;

	dst = ((UINT32 *)vecbitmap->line[y])[x];
	((UINT32 *)vecbitmap->line[y])[x] = LIMIT8(RGB_BLUE(col) + (dst & 0xff))
		| (LIMIT8(RGB_GREEN(col) + ((dst >> 8) & 0xff)) << 8)
		| (LIMIT8(RGB_RED(col) + (dst >> 16)) << 16);

	coords = VECTOR_PIXEL(x,y);
	if (p_index<MAX_PIXELS)
		pixel[p_index++] = coords;

	/* Mark this pixel as dirty */
	if (dirty_index<MAX_DIRTY_PIXELS)
		vector_dirty_list[dirty_index++] = coords;
}


/*
 * draws a line
 *
 * input:   x2  16.16 fixed point
 *          y2  16.16 fixed point
 *         col  0-255 indexed color (8 bit)
 *   intensity  0-255 intensity
 *       dirty  bool  mark the pixels as dirty while plotting them
 *
 * written by Andrew Caldwell
 */

void vector_draw_to(int x2, int y2, rgb_t col, int intensity, int dirty, rgb_t (*color_callback)(void))
{
	unsigned char a1;
	int dx,dy,sx,sy,cx,cy,width;
	static int x1,yy1;
	int xx,yy;

	x2 = (int)(vector_scale_x*x2);
	y2 = (int)(vector_scale_y*y2);

	/* [2] adjust cords if needed */

	if (options.antialias)
	{
		if(beam_diameter_is_one)
		{
			x2 = (x2+0x8000)&0xffff0000;
			y2 = (y2+0x8000)&0xffff0000;
		}
	}
	else /* noantialiasing */
	{
		x2 = (x2 + 0x8000) >> 16;
		y2 = (y2 + 0x8000) >> 16;
	}

	/* [3] handle color and intensity */

	if (intensity == 0) goto end_draw;

	col = Tinten(intensity, col);

	/* [4] draw line */

	if (options.antialias)
	{
		/* draw an anti-aliased line */
		dx = abs(x1 - x2);
		dy = abs(yy1 - y2);

		if (dx >= dy)
		{
			sx = ((x1 <= x2) ? 1 : -1);
			sy = vec_div(y2 - yy1, dx);
			if (sy < 0)
				dy--;
			x1 >>= 16;
			xx = x2 >> 16;
			width = vec_mult(int_beam << 4, Tcosin(abs(sy) >> 5));
			if (!beam_diameter_is_one)
				yy1 -= width >> 1; /* start back half the diameter */
			for (;;)
			{
				if (color_callback) col = Tinten(intensity, (*color_callback)());
				dx = width;    /* init diameter of beam */
				dy = yy1 >> 16;
				vector_draw_aa_pixel(x1, dy++, Tinten(Tgammar[0xff & (yy1 >> 8)], col), dirty);
				dx -= 0x10000 - (0xffff & yy1); /* take off amount plotted */
				a1 = Tgamma[(dx >> 8) & 0xff];   /* calc remainder pixel */
				dx >>= 16;                   /* adjust to pixel (solid) count */
				while (dx--)                 /* plot rest of pixels */
					vector_draw_aa_pixel(x1, dy++, col, dirty);
				vector_draw_aa_pixel(x1, dy, Tinten(a1,col), dirty);
				if (x1 == xx) break;
				x1 += sx;
				yy1 += sy;
			}
		}
		else
		{
			sy = ((yy1 <= y2) ? 1: -1);
			sx = vec_div(x2 - x1, dy);
			if (sx < 0)
				dx--;
			yy1 >>= 16;
			yy = y2 >> 16;
			width = vec_mult(int_beam << 4,Tcosin(abs(sx) >> 5));
			if (!beam_diameter_is_one)
				x1 -= width >> 1; /* start back half the width */
			for (;;)
			{
				if (color_callback) col = Tinten(intensity, (*color_callback)());
				dy = width;    /* calc diameter of beam */
				dx = x1 >> 16;
				vector_draw_aa_pixel(dx++, yy1, Tinten(Tgammar[0xff & (x1 >> 8)], col), dirty);
				dy -= 0x10000 - (0xffff & x1); /* take off amount plotted */
				a1 = Tgamma[(dy >> 8) & 0xff];   /* remainder pixel */
				dy >>= 16;                   /* adjust to pixel (solid) count */
				while (dy--)                 /* plot rest of pixels */
					vector_draw_aa_pixel(dx++, yy1, col, dirty);
				vector_draw_aa_pixel(dx, yy1, Tinten(a1, col), dirty);
				if (yy1 == yy) break;
				yy1 += sy;
				x1 += sx;
			}
		}
	}
	else /* use good old Bresenham for non-antialiasing 980317 BW */
	{
		dx = abs(x1 - x2);
		dy = abs(yy1 - y2);
		sx = (x1 <= x2) ? 1 : -1;
		sy = (yy1 <= y2) ? 1 : -1;
		cx = dx / 2;
		cy = dy / 2;

		if (dx >= dy)
		{
			for (;;)
			{
				if (color_callback) col = Tinten(intensity, (*color_callback)());
				vector_draw_aa_pixel(x1, yy1, col, dirty);
				if (x1 == x2) break;
				x1 += sx;
				cx -= dy;
				if (cx < 0)
				{
					yy1 += sy;
					cx += dx;
				}
			}
		}
		else
		{
			for (;;)
			{
				if (color_callback) col = Tinten(intensity, (*color_callback)());
				vector_draw_aa_pixel(x1, yy1, col, dirty);
				if (yy1 == y2) break;
				yy1 += sy;
				cy -= dx;
				if (cy < 0)
				{
					x1 += sx;
					cy += dy;
				}
			}
		}
	}

end_draw:

	x1 = x2;
	yy1 = y2;
}

int vector_logging = 0;

/*
 * Adds a line end point to the vertices list. The vector processor emulation
 * needs to call this.
 */
void vector_add_point (int x, int y, rgb_t color, int intensity)
{
	point *newpoint;

	intensity *= options.vector_intensity_correction;
	if (intensity > 0xff)
		intensity = 0xff;

	if (options.vector_flicker && (intensity > 0))
	{
		intensity += (intensity * (0x80-(rand()&0xff)) * options.vector_flicker)>>16;
		if (intensity < 0)
			intensity = 0;
		if (intensity > 0xff)
			intensity = 0xff;
	}
	newpoint = &new_list[new_index];
	newpoint->x = x;
	newpoint->y = y;
	newpoint->col = color;
	newpoint->intensity = intensity;
	newpoint->callback = 0;
	newpoint->status = VDIRTY; /* mark identical lines as clean later */

	new_index++;
	if (new_index >= MAX_POINTS)
	{
		new_index--;
		log_cb(RETRO_LOG_DEBUG, LOGPRE "*** Warning! Vector list overflow!\n");
	}
}

void vector_add_point_callback (int x, int y, rgb_t (*color_callback)(void), int intensity)
{
	point *newpoint;

	intensity *= options.vector_intensity_correction;
	if (intensity > 0xff)
		intensity = 0xff;

	if (options.vector_flicker && (intensity > 0))
	{
		intensity += (intensity * (0x80-(rand()&0xff)) * options.vector_flicker)>>16;
		if (intensity < 0)
			intensity = 0;
		if (intensity > 0xff)
			intensity = 0xff;
	}
	newpoint = &new_list[new_index];
	newpoint->x = x;
	newpoint->y = y;
	newpoint->col = 1;
	newpoint->intensity = intensity;
	newpoint->callback = color_callback;
	newpoint->status = VDIRTY; /* mark identical lines as clean later */

	new_index++;
	if (new_index >= MAX_POINTS)
	{
		new_index--;
		log_cb(RETRO_LOG_DEBUG, LOGPRE "*** Warning! Vector list overflow!\n");
	}
}

/*
 * Add new clipping info to the list
 */
void vector_add_clip (int x1, int yy1, int x2, int y2)
{
	point *newpoint;

	newpoint = &new_list[new_index];
	newpoint->x = x1;
	newpoint->y = yy1;
	newpoint->arg1 = x2;
	newpoint->arg2 = y2;
	newpoint->status = VCLIP;

	new_index++;
	if (new_index >= MAX_POINTS)
	{
		new_index--;
		log_cb(RETRO_LOG_DEBUG, LOGPRE "*** Warning! Vector list overflow!\n");
	}
}


/*
 * Set the clipping area
 */
void vector_set_clip (int x1, int yy1, int x2, int y2)
{
	/* failsafe */
	if ((x1 >= x2) || (yy1 >= y2))
	{
		log_cb(RETRO_LOG_DEBUG, LOGPRE "Error in clipping parameters.\n");
		xmin = 0;
		ymin = 0;
		xmax = vecwidth;
		ymax = vecheight;
		return;
	}

	/* scale coordinates to display */
		x1 = (int)(vector_scale_x*x1);
	yy1= (int)(vector_scale_y*yy1);
	x2 = (int)(vector_scale_x*x2);
	y2 = (int)(vector_scale_y*y2);

	xmin = (x1 + 0x8000) >> 16;
	ymin = (yy1 + 0x8000) >> 16;
	xmax = (x2 + 0x8000) >> 16;
	ymax = (y2 + 0x8000) >> 16;

	/* Make it foolproof by trapping rounding errors */
	if (xmin < 0) xmin = 0;
	if (ymin < 0) ymin = 0;
	if (xmax > vecwidth) xmax = vecwidth;
	if (ymax > vecheight) ymax = vecheight;
}


/*
 * The vector CPU creates a new display list. We save the old display list,
 * but only once per refresh.
 */
void vector_clear_list (void)
{
	point *tmp;

	if (vector_runs == 0)
	{
		old_index = new_index;
		tmp = old_list; old_list = new_list; new_list = tmp;
	}

	new_index = 0;
	vector_runs++;
}


/*
 * By comparing with the last drawn list, we can prevent that identical
 * vectors are marked dirty which appeared at the same list index in the
 * previous frame. BW 19980307
 */
static void clever_mark_dirty (void)
{
	int i, min_index, last_match = 0;
	point *new, *old;
	point newclip, oldclip;
	int clips_match = 1;

	if (old_index < new_index)
		min_index = old_index;
	else
		min_index = new_index;

	/* Reset the active clips to invalid values */
	memset (&newclip, 0, sizeof (newclip));
	memset (&oldclip, 0, sizeof (oldclip));

	/* Mark vectors which are not the same in both lists as dirty */
	new = new_list;
	old = old_list;

	for (i = min_index; i > 0; i--, old++, new++)
	{
		/* If this is a clip, we need to determine if the clip regions still match */
		if (old->status == VCLIP || new->status == VCLIP)
		{
			if (old->status == VCLIP)
				oldclip = *old;
			if (new->status == VCLIP)
				newclip = *new;
			clips_match = (newclip.x == oldclip.x) && (newclip.y == oldclip.y) && (newclip.arg1 == oldclip.arg1) && (newclip.arg2 == oldclip.arg2);
			if (!clips_match)
				last_match = 0;

			/* fall through to erase the old line if this is not a clip */
			if (old->status == VCLIP)
				continue;
		}

		/* If the clips match and the vectors match, update */
		else if (clips_match && (new->x == old->x) && (new->y == old->y) &&
			(new->col == old->col) && (new->intensity == old->intensity) &&
			(!new->callback && !old->callback))
		{
			if (last_match)
			{
				new->status = VCLEAN;
				continue;
			}
			last_match = 1;
		}

		/* If nothing matches, remember it */
		else
			last_match = 0;

		/* mark the pixels of the old vector dirty */
		if (dirty_index + old->arg2 - old->arg1 < MAX_DIRTY_PIXELS)
		{
			memcpy(&vector_dirty_list[dirty_index], &pixel[old->arg1], (old->arg2 - old->arg1) * sizeof(pixel[0]));
			dirty_index += old->arg2 - old->arg1;
		}
	}

	/* all old vector with index greater new_index are dirty */
	/* old = &old_list[min_index] here! */
	for (i = (old_index-min_index); i > 0; i--, old++)
	{
		/* skip old clips */
		if (old->status == VCLIP)
			continue;

		/* mark the pixels of the old vector dirty */
		if (dirty_index + old->arg2 - old->arg1 < MAX_DIRTY_PIXELS)
		{
			memcpy(&vector_dirty_list[dirty_index], &pixel[old->arg1], (old->arg2 - old->arg1) * sizeof(pixel[0]));
			dirty_index += old->arg2 - old->arg1;
		}
	}
}

VIDEO_UPDATE( vector )
{
	int i;
	point *curpoint;

	int rv = 1;

	/* if there is an auxiliary renderer set, let it run */
	if (vector_aux_renderer)
		rv = vector_aux_renderer(new_list, new_index);

	/* if the aux renderer chooses, it can override the bitmap */
	if (!rv)
	{
		/* This prevents a crash in the artwork system */
		vector_dirty_list[0] = VECTOR_PIXEL_END;
		return;
	}

	/* copy parameters */
	vecbitmap = bitmap;
	vecwidth  = bitmap->width;
	vecheight = bitmap->height;

	/* reset clipping area */
	xmin = 0;
	xmax = vecwidth;
	ymin = 0;
	ymax = vecheight;

	/* setup scaling */
	vector_scale_x = ((float)vecwidth)/(Machine->visible_area.max_x - Machine->visible_area.min_x);
	vector_scale_y = ((float)vecheight)/(Machine->visible_area.max_y - Machine->visible_area.min_y);

	/* next call to vector_clear_list() is allowed to swap the lists */
	vector_runs = 0;

	/* mark pixels which are not idential in newlist and oldlist dirty */
	/* the old pixels which get removed are marked dirty immediately,  */
	/* new pixels are recognized by setting new->dirty */
	dirty_index = 0;
	clever_mark_dirty();

	/* clear ALL pixels in the hidden map */
	vector_clear_pixels();

  /* Update rendering options once the screen is clear */
	update_options();

	/* Draw ALL lines into the hidden map. Mark only those lines with */
	/* new->dirty = 1 as dirty. Remember the pixel start/end indices  */
	curpoint = new_list;

	for (i = 0; i < new_index; i++)
	{
		if (curpoint->status == VCLIP)
		{
			vector_set_clip(curpoint->x, curpoint->y, curpoint->arg1, curpoint->arg2);
		}
		else
		{
			curpoint->arg1 = p_index;
			vector_draw_to(curpoint->x, curpoint->y, curpoint->col, Tgamma[curpoint->intensity], curpoint->status, curpoint->callback);

			curpoint->arg2 = p_index;
		}
		curpoint++;
	}

	vector_dirty_list[dirty_index] = VECTOR_PIXEL_END;
}

#endif /* if !(defined xgl) */
