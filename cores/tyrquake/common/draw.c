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

/* draw.c -- this is the only file outside the refresh that touches the */
/* vid buffer */

#include "compat/strl.h"
#include "common.h"
#include "console.h"
#include "d_iface.h"
#include "draw.h"
#include "quakedef.h"
#include "rhi.h"
#include "sys.h"
#include "vid.h"
#include "view.h"
#include "wad.h"
#include "zone.h"

#include "sound.h"

typedef struct {
    vrect_t rect;
    int width;
    int height;
    const byte *ptexbytes;
    int rowbytes;
} rectdesc_t;

static rectdesc_t r_rectdesc;

byte *draw_chars;		/* 8*8 graphic characters */
const qpic_t *draw_disc;
extern byte *host_basepal;
static const qpic_t *draw_backtile;


/* ============================================================================= */
/* Support Routines */

typedef struct cachepic_s {
    char name[MAX_QPATH];
    cache_user_t cache;
} cachepic_t;

#define	MAX_CACHED_PICS		128
static cachepic_t menu_cachepics[MAX_CACHED_PICS];
static int menu_numcachepics;


void *Draw_PicFromWad(const char *name)
{
    return (qpic_t*)W_GetLumpName(name);
}

/*
================
Draw_CachePic
================
*/
qpic_t *
Draw_CachePic(const char *path)
{
    cachepic_t *pic;
    int i;
    qpic_t *dat;

    for (pic = menu_cachepics, i = 0; i < menu_numcachepics; pic++, i++)
	if (!strcmp(path, pic->name))
	    break;

    if (i == menu_numcachepics) {
	if (menu_numcachepics == MAX_CACHED_PICS)
	    Sys_Error("menu_numcachepics == MAX_CACHED_PICS");
	menu_numcachepics++;
	strlcpy(pic->name, path, sizeof(pic->name));
    }

    dat = (qpic_t*)Cache_Check(&pic->cache);

    if (dat)
	return dat;

/**/
/* load the pic from disk */
/**/
    COM_LoadCacheFile(path, &pic->cache);

    dat = (qpic_t *)pic->cache.data;
    if (!dat) {
	Sys_Error("%s: failed to load %s", __func__, path);
    }

    SwapPic(dat);

    /* Defensive: the on-disk .lmp format is just `int width;
     * int height; byte data[width*height]`.  Both fields are
     * file-controlled.  Without bounds, dat->width = -1 (or any
     * negative) makes Draw_Pic's `memcpy(dest, source,
     * pic->width)` interpret the size as size_t = SIZE_MAX --
     * catastrophic write.  dat->height huge with a tiny
     * pic->width walks pic->data[] far past the cache extent
     * (com_filesize bytes loaded) on every redraw.  Cap each
     * dimension to vid.maxwidth/maxheight (qpic was never
     * intended to exceed screen size; the largest legitimate
     * Quake .lmp is 320x200 conback) and require the implied
     * total fits within the loaded file. */
    if (dat->width <= 0 || dat->height <= 0
        || dat->width > 4096 || dat->height > 4096
        || (size_t)com_filesize < sizeof(qpic_t)
        || (size_t)dat->width * (size_t)dat->height
             > (size_t)com_filesize - offsetof(qpic_t, data))
	Sys_Error("%s: %s has bad dimensions %dx%d (filesize %d)",
                  __func__, path, dat->width, dat->height,
                  com_filesize);

    return dat;
}



/*
===============
Draw_Init
===============
*/

extern void VID_SetPalette2(unsigned char *palette);

/* Colored Lighting lookup tables */
byte palmap2[64][64][64];	

/*
===============
BestColor
===============
*/
static byte BestColor(int r, int g, int b, int start, int stop)
{
	int	i;
	/* let any color go to 0 as a last resort */
	int bestdistortion = 256*256*4;
	int bestcolor      = 0;
	byte *pal          = host_basepal + start*3;
	for (i=start ; i<= stop ; i++)
	{
		int dr          = r - (int)pal[0];
		int dg          = g - (int)pal[1];
		int db          = b - (int)pal[2];
		int distortion  = dr * dr + dg * dg + db * db;

		pal            += 3;

		if (distortion < bestdistortion)
		{
			if (!distortion)
				return i; /* perfect match */

			bestdistortion = distortion;
			bestcolor      = i;
		}
	}

	return bestcolor;
}

static void Draw_Generate18BPPTable (void)
{
	int		r, g, b;

	/* Make the 18-bit lookup table here */
	for (r=0 ; r<256 ; r+=4)
	{
		for (g=0 ; g<256 ; g+=4)
		{
			for (b=0 ; b<256 ; b+=4)
			{
				int beastcolor = BestColor (r, g, b, 0, 254);
				palmap2[r>>2][g>>2][b>>2] = beastcolor;
			}
		}
	}
}

void Draw_Init(void)
{
    draw_chars = (byte*)W_GetLumpName("conchars");
    draw_disc = (const qpic_t*)W_GetLumpName("disc");
    draw_backtile = (const qpic_t*)W_GetLumpName("backtile");

    r_rectdesc.width = draw_backtile->width;
    r_rectdesc.height = draw_backtile->height;
    r_rectdesc.ptexbytes = draw_backtile->data;
    r_rectdesc.rowbytes = draw_backtile->width;

    /* Always build the 18bpp palette LUT and 64x64x64 palmap2.
     * These are needed by D_PolysetDrawSpansRGB (and the Phong-RGB
     * variant) whenever r_coloredlight is enabled at runtime, and
     * having them built unconditionally lets the user toggle the
     * cvar without a restart.  Cost is one-time at startup. */
    VID_SetPalette2(host_basepal);
    Draw_Generate18BPPTable();
}

/*
================
Draw_Character

Draws one 8*8 graphics character with 0 being transparent.
It can be clipped to the top of the screen to allow the console to be
smoothly scrolled off.
================
*/
void Draw_Character(int x, int y, int num)
{
    byte *dest;
    byte *source;
    int drawline;
    int row, col;

    num &= 255;

    if (y <= -8)
	return;

    if (y > vid.height - 8 || x < 0 || x > vid.width - 8)
	return;
    if (num < 0 || num > 255)
	return;

    /* Phase 4o: if the active backend implements a
     * Draw_Character intercept (Vulkan: queue_2d_char
     * pushes one quad sampling the conchars atlas at
     * the right sub-UV), forward and skip the SW
     * memcpy-into-vid.buffer below.  When no backend
     * implements the intercept (SW-only builds, SW
     * backend selected), the field stays NULL and
     * Draw_Character falls through to its original
     * behaviour. */
    if (g_rhi && g_rhi->queue_2d_char) {
	g_rhi->queue_2d_char(x, y, num, 1);
	return;
    }

    row = num >> 4;
    col = num & 15;
    source = draw_chars + (row << 10) + (col << 3);

    if (y < 0) {		/* clipped */
	drawline = 8 + y;
	source -= 128 * y;
	y = 0;
    } else
	drawline = 8;

    {
	dest = vid.conbuffer + y * vid.conrowbytes + x;

	while (drawline--)
	{
	    if (source[0])
		dest[0] = source[0];
	    if (source[1])
		dest[1] = source[1];
	    if (source[2])
		dest[2] = source[2];
	    if (source[3])
		dest[3] = source[3];
	    if (source[4])
		dest[4] = source[4];
	    if (source[5])
		dest[5] = source[5];
	    if (source[6])
		dest[6] = source[6];
	    if (source[7])
		dest[7] = source[7];
	    source += 128;
	    dest += vid.conrowbytes;
	}
    }
}

/*
================
Draw_String
================
*/
void Draw_String(int x, int y, char *str)
{
   while (*str)
   {
      Draw_Character(x, y, *str);
      str++;
      x += 8;
   }
}

static void Draw_Pixel(int x, int y, byte color)
{
      uint8_t *dest;
      /* Defensive: x and y come from float arithmetic that
       * folds in user cvars (cl_crossx, cl_crossy).  A NaN
       * cvar value (e.g. "cl_crossx nan", reachable via
       * svc_stufftext from a hostile NQ server) propagates
       * to (int)NaN at the implicit cast and yields INT_MIN
       * on x86 -- a wild address single-byte write through
       * vid.conbuffer otherwise.  Bound to the visible
       * framebuffer rectangle. */
      if (x < 0 || y < 0
          || (unsigned)x >= vid.width
          || (unsigned)y >= vid.height)
         return;
      dest = vid.conbuffer + y * vid.conrowbytes + x;
      *dest = color;
}

void
Draw_Crosshair(void)
{
   int x, y;
   float cx = cl_crossx.value;
   float cy = cl_crossy.value;
   byte c;

   /* Cvars below feed an int conversion via Draw_Pixel /
    * Draw_Character.  NaN/Inf cvar values (reachable via
    * svc_stufftext "cl_crossx nan" from a hostile server)
    * make the cast UB.  Draw_Pixel clamps the resulting
    * coordinates defensively but treat the cvars at the
    * source too -- silently coerces wild values to 0 so
    * the crosshair stays visible at center. */
   if (IS_NAN(cx))
      cx = 0;
   if (IS_NAN(cy))
      cy = 0;
   if (IS_NAN(crosshaircolor.value))
      c = 0;
   else
      c = (byte)((int)crosshaircolor.value & 0xFF);

   if (crosshair.value == 2)
   {
      x = scr_vrect.x + scr_vrect.width / 2 + (int)cx;
      y = scr_vrect.y + scr_vrect.height / 2 + (int)cy;
      Draw_Pixel(x - 1, y, c);
      Draw_Pixel(x - 3, y, c);
      Draw_Pixel(x + 1, y, c);
      Draw_Pixel(x + 3, y, c);
      Draw_Pixel(x, y - 1, c);
      Draw_Pixel(x, y - 3, c);
      Draw_Pixel(x, y + 1, c);
      Draw_Pixel(x, y + 3, c);
   }
   else if (crosshair.value)
      Draw_Character(scr_vrect.x + scr_vrect.width / 2 - 4 +
            (int)cx,
            scr_vrect.y + scr_vrect.height / 2 - 4 +
            (int)cy, '+');
}


/*
=============
Draw_Pic
=============
*/
void Draw_Pic(int x, int y, const qpic_t *pic)
{
   int v;
   uint8_t *dest;
   const byte *source;

   /* Defensive: pic pointers can come from long-lived globals
    * (sb_nums[][], menu_cachepics[], etc) and have been
    * observed holding stale values across content reloads.
    * Skip gracefully rather than crashing the renderer. */
   if (!Hunk_PointerInHunk(pic))
      return;

   if (x < 0 || x + pic->width > vid.width ||
         y < 0 || y + pic->height > vid.height)
      Sys_Error("%s: bad coordinates", __func__);

   /* Phase 4l: forward to the active RHI backend's 2D
    * intercept if it implements one, and skip the SW
    * memcpy below when it does.  The Vulkan overlay
    * renders the pic at native resolution; the compute
    * upload no longer needs the SW-rasterised copy in
    * vid.buffer.  When no backend implements the
    * intercept (SW-only builds, SW backend selected),
    * the field stays NULL and Draw_Pic falls through to
    * its original memcpy-into-vid.buffer behaviour. */
   if (g_rhi && g_rhi->queue_2d_pic) {
      g_rhi->queue_2d_pic(x, y, pic);
      return;
   }

   source = pic->data;

   dest = vid.buffer + y * vid.rowbytes + x;

   for (v = 0; v < pic->height; v++)
   {
      memcpy(dest, source, pic->width);
      dest   += vid.rowbytes;
      source += pic->width;
   }
}


/*
=============
Draw_TransPic
=============
*/
void Draw_TransPic(int x, int y, const qpic_t *pic)
{
   byte *dest, tbyte;
   const byte *source;
   int v, u;

   /* Defensive: see Draw_Pic. */
   if (!Hunk_PointerInHunk(pic))
      return;

   if (x < 0 || (unsigned)(x + pic->width) > vid.width ||
         y < 0 || (unsigned)(y + pic->height) > vid.height)
      Sys_Error("%s: bad coordinates", __func__);

   /* Phase 4l: same intercept as Draw_Pic.  The overlay
    * FS gained a discard on palette index 255
    * (TRANSPARENT_COLOR in d_iface.h) at this phase, so
    * the Vulkan-rendered TransPic gets the same per-
    * pixel transparency the SW loop below provides.
    * On intercept, skip the SW path entirely. */
   if (g_rhi && g_rhi->queue_2d_pic) {
      g_rhi->queue_2d_pic(x, y, pic);
      return;
   }

   source = pic->data;

   {
      dest = vid.buffer + y * vid.rowbytes + x;

      if (pic->width & 7) {	/* general */
         for (v = 0; v < pic->height; v++) {
            for (u = 0; u < pic->width; u++)
               if ((tbyte = source[u]) != TRANSPARENT_COLOR)
                  dest[u] = tbyte;

            dest += vid.rowbytes;
            source += pic->width;
         }
      } else {		/* unwound */
         for (v = 0; v < pic->height; v++) {
            for (u = 0; u < pic->width; u += 8) {
               if ((tbyte = source[u]) != TRANSPARENT_COLOR)
                  dest[u] = tbyte;
               if ((tbyte = source[u + 1]) != TRANSPARENT_COLOR)
                  dest[u + 1] = tbyte;
               if ((tbyte = source[u + 2]) != TRANSPARENT_COLOR)
                  dest[u + 2] = tbyte;
               if ((tbyte = source[u + 3]) != TRANSPARENT_COLOR)
                  dest[u + 3] = tbyte;
               if ((tbyte = source[u + 4]) != TRANSPARENT_COLOR)
                  dest[u + 4] = tbyte;
               if ((tbyte = source[u + 5]) != TRANSPARENT_COLOR)
                  dest[u + 5] = tbyte;
               if ((tbyte = source[u + 6]) != TRANSPARENT_COLOR)
                  dest[u + 6] = tbyte;
               if ((tbyte = source[u + 7]) != TRANSPARENT_COLOR)
                  dest[u + 7] = tbyte;
            }
            dest += vid.rowbytes;
            source += pic->width;
         }
      }
   }
}


/*
=============
Draw_TransPicTranslate
=============
*/
void Draw_TransPicTranslate(int x, int y, const qpic_t *pic, byte *translation)
{
   byte *dest, tbyte;
   const byte *source;
   int v, u;

   /* Defensive: see Draw_Pic. */
   if (!Hunk_PointerInHunk(pic))
      return;

   if (x < 0 || (unsigned)(x + pic->width) > vid.width ||
         y < 0 || (unsigned)(y + pic->height) > vid.height)
      Sys_Error("%s: bad coordinates", __func__);

   source = pic->data;

   {
      dest = vid.buffer + y * vid.rowbytes + x;

      if (pic->width & 7) {	/* general */
         for (v = 0; v < pic->height; v++) {
            for (u = 0; u < pic->width; u++)
               if ((tbyte = source[u]) != TRANSPARENT_COLOR)
                  dest[u] = translation[tbyte];

            dest += vid.rowbytes;
            source += pic->width;
         }
      } else {		/* unwound */
         for (v = 0; v < pic->height; v++) {
            for (u = 0; u < pic->width; u += 8) {
               if ((tbyte = source[u]) != TRANSPARENT_COLOR)
                  dest[u] = translation[tbyte];
               if ((tbyte = source[u + 1]) != TRANSPARENT_COLOR)
                  dest[u + 1] = translation[tbyte];
               if ((tbyte = source[u + 2]) != TRANSPARENT_COLOR)
                  dest[u + 2] = translation[tbyte];
               if ((tbyte = source[u + 3]) != TRANSPARENT_COLOR)
                  dest[u + 3] = translation[tbyte];
               if ((tbyte = source[u + 4]) != TRANSPARENT_COLOR)
                  dest[u + 4] = translation[tbyte];
               if ((tbyte = source[u + 5]) != TRANSPARENT_COLOR)
                  dest[u + 5] = translation[tbyte];
               if ((tbyte = source[u + 6]) != TRANSPARENT_COLOR)
                  dest[u + 6] = translation[tbyte];
               if ((tbyte = source[u + 7]) != TRANSPARENT_COLOR)
                  dest[u + 7] = translation[tbyte];
            }
            dest += vid.rowbytes;
            source += pic->width;
         }
      }
   }
}


/* =============================================================================
 *
 *   Scaled (pixel-doubled) draw entry points
 *
 *   The original Draw_* functions blit native-resolution UI assets at
 *   1:1 pixel size. At modern render resolutions (e.g. 1920x1200) the
 *   menu and status bar end up as a tiny strip in the middle of the
 *   screen because the assets themselves are 320x200-native and the
 *   draw routines don't scale.
 *
 *   The Scaled variants take an integer scale factor (typically the
 *   value returned by SCR_GetUIScale()) and pixel-double the source
 *   asset by 'scale' in both dimensions on the way out. The (x, y)
 *   destination coordinates are in *physical screen pixels*, not
 *   logical 320x200-space pixels, so callers are responsible for
 *   multiplying their own logical coordinates by 'scale' before
 *   passing them in (and for adjusting any 320-based centering math
 *   to use 320*scale).
 *
 *   Bounds-clipping uses the destination extents (pic->width*scale,
 *   pic->height*scale) and silently clips off-screen rather than
 *   Sys_Error()ing; off-by-one centering arithmetic shouldn't be
 *   fatal.
 * ============================================================================= */

void
Draw_PicScaled(int x, int y, const qpic_t *pic, int scale)
{
    byte *dest;
    const byte *source;
    int v, u, sx, sy;
    int dw, dh;

    /* Defensive: see Draw_Pic. */
    if (!Hunk_PointerInHunk(pic))
	return;

    if (scale < 1)
	scale = 1;

    if (scale == 1) {
	Draw_Pic(x, y, pic);
	return;
    }

    dw = pic->width  * scale;
    dh = pic->height * scale;

    if (x < 0 || y < 0 || x + dw > (int)vid.width || y + dh > (int)vid.height)
	return;

    /* Phase 4q: forward the scale > 1 path to the
     * overlay queue when a backend implements it,
     * skipping the stretched-memcpy SW write below.
     * Backends without the entry (SW backend, NULL
     * field) keep the original behaviour.  scale == 1
     * was already handled by the Draw_Pic branch
     * above, which routes through queue_2d_pic. */
    if (g_rhi && g_rhi->queue_2d_pic_scaled) {
	g_rhi->queue_2d_pic_scaled(x, y, pic, scale);
	return;
    }

    source = pic->data;
    dest = vid.buffer + y * vid.rowbytes + x;

    for (v = 0; v < pic->height; v++) {
	for (sy = 0; sy < scale; sy++) {
	    byte *d = dest + sy * vid.rowbytes;
	    for (u = 0; u < pic->width; u++) {
		byte b = source[u];
		for (sx = 0; sx < scale; sx++)
		    d[u * scale + sx] = b;
	    }
	}
	dest   += scale * vid.rowbytes;
	source += pic->width;
    }
}

void
Draw_TransPicScaled(int x, int y, const qpic_t *pic, int scale)
{
    byte *dest;
    const byte *source;
    int v, u, sx, sy;
    int dw, dh;

    /* Defensive: see Draw_Pic. */
    if (!Hunk_PointerInHunk(pic))
	return;

    if (scale < 1)
	scale = 1;

    if (scale == 1) {
	Draw_TransPic(x, y, pic);
	return;
    }

    dw = pic->width  * scale;
    dh = pic->height * scale;

    if (x < 0 || y < 0 || x + dw > (int)vid.width || y + dh > (int)vid.height)
	return;

    /* Phase 4q: same intercept as Draw_PicScaled above.
     * The overlay FS byte-255 discard reproduces the
     * `if (b != TRANSPARENT_COLOR) ...` SW skip for
     * free (TRANSPARENT_COLOR == 255). */
    if (g_rhi && g_rhi->queue_2d_pic_scaled) {
	g_rhi->queue_2d_pic_scaled(x, y, pic, scale);
	return;
    }

    source = pic->data;
    dest = vid.buffer + y * vid.rowbytes + x;

    for (v = 0; v < pic->height; v++) {
	for (sy = 0; sy < scale; sy++) {
	    byte *d = dest + sy * vid.rowbytes;
	    for (u = 0; u < pic->width; u++) {
		byte b = source[u];
		if (b != TRANSPARENT_COLOR) {
		    for (sx = 0; sx < scale; sx++)
			d[u * scale + sx] = b;
		}
	    }
	}
	dest   += scale * vid.rowbytes;
	source += pic->width;
    }
}

void
Draw_TransPicTranslateScaled(int x, int y, const qpic_t *pic,
			     byte *translation, int scale)
{
    byte *dest;
    const byte *source;
    int v, u, sx, sy;
    int dw, dh;

    /* Defensive: see Draw_Pic. */
    if (!Hunk_PointerInHunk(pic))
	return;

    if (scale < 1)
	scale = 1;

    if (scale == 1) {
	Draw_TransPicTranslate(x, y, pic, translation);
	return;
    }

    dw = pic->width  * scale;
    dh = pic->height * scale;

    if (x < 0 || y < 0 || x + dw > (int)vid.width || y + dh > (int)vid.height)
	return;

    /* Phase 4s: forward the scale > 1 translated path to
     * the overlay queue when a backend implements it.
     * The intercept handles the per-pixel translation
     * (CPU-side, into a scratch buffer) before queuing.
     * Skip the SW stretched-memcpy-with-translation
     * loop below.  Backends without the entry (SW
     * backend, NULL field) fall through. */
    if (g_rhi && g_rhi->queue_2d_pic_translate_scaled) {
	g_rhi->queue_2d_pic_translate_scaled(x, y, pic, translation, scale);
	return;
    }

    source = pic->data;
    dest = vid.buffer + y * vid.rowbytes + x;

    for (v = 0; v < pic->height; v++) {
	for (sy = 0; sy < scale; sy++) {
	    byte *d = dest + sy * vid.rowbytes;
	    for (u = 0; u < pic->width; u++) {
		byte b = source[u];
		if (b != TRANSPARENT_COLOR) {
		    byte t = translation[b];
		    for (sx = 0; sx < scale; sx++)
			d[u * scale + sx] = t;
		}
	    }
	}
	dest   += scale * vid.rowbytes;
	source += pic->width;
    }
}

void
Draw_CharacterScaled(int x, int y, int num, int scale)
{
    byte *dest;
    byte *source;
    int drawline;
    int row, col;
    int u, sx, sy;

    if (scale < 1)
	scale = 1;

    if (scale == 1) {
	Draw_Character(x, y, num);
	return;
    }

    num &= 255;
    if (num < 0 || num > 255)
	return;

    if (x < 0 || y < 0 ||
	x + 8 * scale > (int)vid.width ||
	y + 8 * scale > (int)vid.height)
	return;

    /* Phase 4o: see Draw_Character intercept above. */
    if (g_rhi && g_rhi->queue_2d_char) {
	g_rhi->queue_2d_char(x, y, num, scale);
	return;
    }

    row = num >> 4;
    col = num & 15;
    source = draw_chars + (row << 10) + (col << 3);

    drawline = 8;
    dest = vid.conbuffer + y * vid.conrowbytes + x;

    while (drawline--) {
	for (sy = 0; sy < scale; sy++) {
	    byte *d = dest + sy * vid.conrowbytes;
	    for (u = 0; u < 8; u++) {
		byte b = source[u];
		if (b) {
		    for (sx = 0; sx < scale; sx++)
			d[u * scale + sx] = b;
		}
	    }
	}
	source += 128;
	dest += scale * vid.conrowbytes;
    }
}

void
Draw_StringScaled(int x, int y, char *str, int scale)
{
    if (scale < 1)
	scale = 1;
    while (*str) {
	Draw_CharacterScaled(x, y, *str, scale);
	str++;
	x += 8 * scale;
    }
}


#define CONBACK_CHAR_W	8
#define CONBACK_CHAR_H	8

static void
Draw_ScaledCharToConback(const qpic_t *conback, int num, byte *dest)
{
    int y;
    int drawlines = conback->height * CONBACK_CHAR_H / 200;
    int drawwidth = conback->width * CONBACK_CHAR_W / 320;
    int row       = num >> 4;
    int col       = num & 15;
    byte *source  = draw_chars + (row << 10) + (col << 3);
    int fstep     = 320 * 0x10000 / conback->width;

    for (y = 0; y < drawlines; y++, dest += conback->width)
    {
	int x;
	int f = 0;
	byte *src = source + (y * CONBACK_CHAR_H / drawlines) * 128;
	for (x = 0; x < drawwidth; x++, f += fstep)
	{
	    if (src[f >> 16])
		dest[x] = 0x60 + src[f >> 16];
	}
    }
}

/*
 * Draw_ConbackString
 *
 * This function draws a string to a very specific location on the console
 * background. The position is such that for a 320x200 background, the text
 * will be 6 pixels from the bottom and 11 pixels from the right. For other
 * sizes, the positioning is scaled so as to make it appear the same size and
 * at the same location.
 */
static void Draw_ConbackString(qpic_t *cb, const char *str)
{
    int x;
    size_t len = strlen(str);
    int row    = cb->height - ((CONBACK_CHAR_H + 6) * cb->height / 200);
    int col    = cb->width - ((11 + CONBACK_CHAR_W * len) * cb->width / 320);
    byte *dest = cb->data + cb->width * row + col;
    for (x = 0; x < len; x++)
	Draw_ScaledCharToConback(cb, str[x], dest + (x * CONBACK_CHAR_W *
						     cb->width / 320));
}


/*
================
Draw_ConsoleBackground

================
*/
void
Draw_ConsoleBackground(int lines)
{
    int x, y, v;
    const byte *src;
    byte *dest;
    int f, fstep;
    qpic_t *conback = Draw_CachePic("gfx/conback.lmp");

    /* hack the version number directly into the pic */
    Draw_ConbackString(conback, stringify(TYR_VERSION));

    /* Phase 4r (re-attempt of 67c8f47 / Phase 4p, see
     * 02c3181 for the revert and 8a9268d / Phase 4q for
     * the unblocking scale > 1 pic intercept): forward
     * to the backend's overlay-queue intercept when
     * non-NULL.  The intercept queues a stretched-
     * bottom-portion quad after any prior 2D pushes so
     * earlier overlay entries (Sbar HUD pics / digits,
     * etc.) are covered, and later overlay pushes
     * (console-text characters from Con_DrawConsole's
     * character loop, menu pics from M_Draw) correctly
     * draw on top.  Must be called *after*
     * Draw_ConbackString above so the cached upload
     * captures the version-stamped pixels.  When the
     * field is NULL (SW backend, SW-only build),
     * Draw_ConsoleBackground falls through to its
     * original memcpy-into-vid.buffer behaviour. */
    if (g_rhi && g_rhi->queue_2d_console_background) {
	g_rhi->queue_2d_console_background(lines, conback);
	return;
    }

    /* draw the pic */
    {
	dest = vid.conbuffer;

	for (y = 0; y < lines; y++, dest += vid.conrowbytes) {
	    v = (vid.conheight - lines + y) * conback->height / vid.conheight;
	    src = conback->data + v * conback->width;
	    if (vid.conwidth == conback->width)
		memcpy(dest, src, vid.conwidth);
	    else {
		f = 0;
		fstep = conback->width * 0x10000 / vid.conwidth;
		for (x = 0; x < vid.conwidth; x += 4) {
		    dest[x] = src[f >> 16];
		    f += fstep;
		    dest[x + 1] = src[f >> 16];
		    f += fstep;
		    dest[x + 2] = src[f >> 16];
		    f += fstep;
		    dest[x + 3] = src[f >> 16];
		    f += fstep;
		}
	    }
	}
    }
}


/*
==============
R_DrawRect8
==============
*/
static void R_DrawRect8(vrect_t *prect, int rowbytes, const byte *psrc, int transparent)
{
    byte t;
    int i, j;
    byte *pdest = vid.buffer + (prect->y * vid.rowbytes) + prect->x;
    int srcdelta = rowbytes - prect->width;
    int destdelta = vid.rowbytes - prect->width;

    if (transparent)
    {
	for (i = 0; i < prect->height; i++)
	{
	    for (j = 0; j < prect->width; j++)
	    {
		t = *psrc;
		if (t != TRANSPARENT_COLOR)
		    *pdest = t;

		psrc++;
		pdest++;
	    }

	    psrc += srcdelta;
	    pdest += destdelta;
	}
    }
    else
    {
	for (i = 0; i < prect->height; i++)
	{
	    memcpy(pdest, psrc, prect->width);
	    psrc += rowbytes;
	    pdest += vid.rowbytes;
	}
    }
}

/*
=============
Draw_TileClear

This repeats a 64*64 tile graphic to fill the screen around a sized down
refresh window.
=============
*/
void
Draw_TileClear(int x, int y, int w, int h)
{
    int width, height, tileoffsetx, tileoffsety;
    const byte *psrc;
    vrect_t vr;

    if (x < 0 || (unsigned)(x + w) > vid.width ||
	y < 0 || (unsigned)(y + h) > vid.height)
	Sys_Error("%s: bad coordinates", __func__);

    r_rectdesc.rect.x = x;
    r_rectdesc.rect.y = y;
    r_rectdesc.rect.width = w;
    r_rectdesc.rect.height = h;

    vr.y = r_rectdesc.rect.y;
    height = r_rectdesc.rect.height;

    tileoffsety = vr.y % r_rectdesc.height;

    while (height > 0) {
	vr.x = r_rectdesc.rect.x;
	width = r_rectdesc.rect.width;

	if (tileoffsety != 0)
	    vr.height = r_rectdesc.height - tileoffsety;
	else
	    vr.height = r_rectdesc.height;

	if (vr.height > height)
	    vr.height = height;

	tileoffsetx = vr.x % r_rectdesc.width;

	while (width > 0) {
	    if (tileoffsetx != 0)
		vr.width = r_rectdesc.width - tileoffsetx;
	    else
		vr.width = r_rectdesc.width;

	    if (vr.width > width)
		vr.width = width;

	    psrc = r_rectdesc.ptexbytes +
		(tileoffsety * r_rectdesc.rowbytes) + tileoffsetx;

	    R_DrawRect8(&vr, r_rectdesc.rowbytes, psrc, 0);

	    vr.x += vr.width;
	    width -= vr.width;
	    tileoffsetx = 0;	/* only the left tile can be left-clipped */
	}

	vr.y += vr.height;
	height -= vr.height;
	tileoffsety = 0;	/* only the top tile can be top-clipped */
    }
}


/*
=============
Draw_Fill

Fills a box of pixels with a single color
=============
*/
void Draw_Fill(int x, int y, int w, int h, int c)
{
    byte *dest;
    int u, v;

    if (x < 0 || x + w > vid.width || y < 0 || y + h > vid.height)
    {
	Con_Printf("Bad Draw_Fill(%d, %d, %d, %d, %c)\n", x, y, w, h, c);
	return;
    }

    /* Phase 4t: forward to the backend's solid-color
     * intercept when non-NULL.  See queue_2d_fill in
     * rhi.h for semantics.  When NULL (SW backend),
     * fall through to the original vid.buffer loop. */
    if (g_rhi && g_rhi->queue_2d_fill) {
	g_rhi->queue_2d_fill(x, y, w, h, c);
	return;
    }

    {
	dest = vid.buffer + y * vid.rowbytes + x;
	for (v = 0; v < h; v++, dest += vid.rowbytes)
	    for (u = 0; u < w; u++)
		dest[u] = c;
    }
}

/* ============================================================================= */

/*
================
Draw_FadeScreen

================
*/
void Draw_FadeScreen(void)
{
   int x, y;

   /* Phase 4t: forward to the backend's full-screen
    * checkerboard-fade intercept when non-NULL.  See
    * queue_2d_fade_screen in rhi.h.  When NULL (SW
    * backend), fall through to the original vid.buffer
    * checkerboard write. */
   if (g_rhi && g_rhi->queue_2d_fade_screen) {
      g_rhi->queue_2d_fade_screen();
      return;
   }

   for (y = 0; y < vid.height; y++)
   {
      byte *pbuf = (byte *)(vid.buffer + vid.rowbytes * y);
      int t      = (y & 1) << 1;

      for (x = 0; x < vid.width; x++) {
         if ((x & 3) != t)
            pbuf[x] = 0;
      }
   }
}

/* ============================================================================= */

/*
================
Draw_BeginDisc

Draws the little blue disc in the corner of the screen.
Call before beginning any disc IO.
================
*/
void Draw_BeginDisc(void)
{
    D_BeginDirectRect(vid.width - 24, 0, draw_disc->data, 24, 24);
}

/*
================
Draw_EndDisc

Erases the disc icon.
Call after completing any disc IO
================
*/
void Draw_EndDisc(void)
{
    D_EndDirectRect(vid.width - 24, 0, 24, 24);
}
