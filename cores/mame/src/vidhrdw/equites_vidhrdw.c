/*******************************************************************************

Equites           (c) 1984 Alpha Denshi Co./Sega
Bull Fighter      (c) 1984 Alpha Denshi Co./Sega
The Koukouyakyuh  (c) 1985 Alpha Denshi Co.
Splendor Blast    (c) 1985 Alpha Denshi Co.
High Voltage      (c) 1985 Alpha Denshi Co.

drivers by Acho A. Tang

*******************************************************************************/
/* Directives*/

#include "driver.h"
#include "vidhrdw/generic.h"

#define BMPAD 8
#define BMW_l2 9
#define FP_PRECISION 20
#define FP_HALF ((1<<(FP_PRECISION-1))-1)

/******************************************************************************/
/* Imports*/

extern int equites_id, equites_flip;

/******************************************************************************/
/* Locals*/

static struct tilemap *charmap0, *charmap1, *activecharmap, *inactivecharmap;
static data16_t *defcharram, *charram0, *charram1, *activecharram, *inactivecharram;
static unsigned char *dirtybuf;
static int maskwidth, maskheight, maskcolor;
static int scrollx, scrolly;
static int bgcolor[4];
static struct rectangle halfclip;
static struct PRESTEP_TYPE { unsigned sy, fdx; } *prestep;

/******************************************************************************/
/* Exports*/

data16_t *splndrbt_scrollx, *splndrbt_scrolly;

/******************************************************************************/
/* Initializations*/

static void video_init_common(void)
{
	pen_t *colortable;
	int i;

	colortable = Machine->remapped_colortable;

	/* set defaults*/
	maskwidth = 8;
	maskheight = Machine->visible_area.max_y - Machine->visible_area.min_y + 1;
	maskcolor = get_black_pen();
	scrollx = scrolly = 0;
	for (i=0; i<4; i++) bgcolor[i] = 0;

	/* set uniques*/
	switch (equites_id)
	{
		case 0x8401:
			maskwidth = 16;
		break;
		case 0x8510:
			scrollx = 128;
			scrolly = 8;
		break;
		case 0x8511:
			scrollx = 128;
			scrolly = 8;
		break;
	}
}

/* Equites Hardware*/
PALETTE_INIT( equites )
{
	UINT8 *clut_ptr;
	int i, r, g, b;

	for (i=0; i<256; i++)
	{
		r = color_prom[i] & 0xf;
		r = (r << 4) + r;
		g = color_prom[i+0x100] & 0xf;
		g = (g << 4) + g;
		b = color_prom[i+0x200] & 0xf;
		b = (b << 4) + b;

		palette_set_color(i, r, g, b);
		colortable[i] = i;
	}

	clut_ptr = memory_region(REGION_USER1) + 0x80;
	for (i=0; i<128; i++)
		colortable[i+0x100] = clut_ptr[i];
}

static void equites_charinfo(int offset)
{
	int tile, color;

	offset <<= 1;
	tile = videoram16[offset];
	color = videoram16[offset+1];
	tile &= 0xff;
	color &= 0x1f;

	SET_TILE_INFO(0, tile, color, 0);
}

VIDEO_START( equites )
{
	charmap0 = tilemap_create(equites_charinfo, tilemap_scan_cols, TILEMAP_TRANSPARENT, 8, 8, 32, 32);
	tilemap_set_transparent_pen(charmap0, 0);
	tilemap_set_scrolldx(charmap0, BMPAD, BMPAD);
	tilemap_set_scrolldy(charmap0, BMPAD, BMPAD);

	video_init_common();

	return (0);
}

/* Splendor Blast Hardware*/
PALETTE_INIT( splndrbt )
{
	UINT8 *prom_ptr;
	int i, r, g, b;

	for (i=0; i<0x100; i++)
	{
		r = color_prom[i] & 0xf;
		r = (r << 4) + r;
		g = color_prom[i+0x100] & 0xf;
		g = (g << 4) + g;
		b = color_prom[i+0x200] & 0xf;
		b = (b << 4) + b;

		palette_set_color(i, r, g, b);
		colortable[i] = (i & 3 || (i > 0x3f && i < 0x80) || i > 0xbf) ? i : 0;

	}
	prom_ptr = memory_region(REGION_USER1);
	colortable += 0x100;
	for (i=0; i<0x80; i++) { colortable[i] = prom_ptr[i]+0x10; colortable[i+0x80] = prom_ptr[i]; }

	prom_ptr += 0x100;
	colortable += 0x100;
	for (i=0; i<0x400; i++) colortable[i] = prom_ptr[i];
}

static void splndrbt_char0info(int offset)
{
	int tile, color;

	offset <<= 1;
	tile = charram0[offset];
	color = charram0[offset+1];
	tile &= 0xff;
	color &= 0x3f;

	SET_TILE_INFO(0, tile, color, 0);
}

static void splndrbt_char1info(int offset)
{
	int tile, color;

	offset <<= 1;
	tile = charram1[offset];
	color = charram1[offset+1];
	tile &= 0xff;
	color &= 0x3f;
	tile += 0x100;

	SET_TILE_INFO(0, tile, color, 0);
}

static void splndrbt_video_reset(void)
{
	memset(spriteram16, 0, spriteram_size);

	activecharram = charram0;
	inactivecharram = charram1;

	activecharmap = charmap0;
	inactivecharmap = charmap1;
}

static void splndrbt_prestep(
	struct PRESTEP_TYPE *ps,
	const struct rectangle *dst_clip,
	int src_w, int src_h,
	int dst_startw, int dst_endw)
{
	double DA, DB, DC, D0, D1, Dsum;
	int i, dst_vish;

	dst_vish = dst_clip->max_y - dst_clip->min_y;

	DA = (double)(src_w << FP_PRECISION) * dst_vish;
	DB = dst_endw - dst_startw;
	DC = dst_startw * dst_vish;
	D1 = 0;

	for (i=0; i<=dst_vish; i++)
	{
		D0 = DA / (DB * i + DC);
		D1 += D0;
		ps[i].fdx = (unsigned)D0;
	}

	DA = src_w * dst_vish;
	D0 = src_h;
	D1 /= 1 << FP_PRECISION;
	Dsum = 0.5;

	for (i=0; i<=dst_vish; i++)
	{
		ps[i].sy = (unsigned)Dsum;
		Dsum += (DA * D0) / ((DB * i + DC) * D1);
		/*logerror("dst_y=%3u src_y=%3u\n", i, ps[i].sy);*/
	}
}

VIDEO_START( splndrbt )
{
#define BMW (1<<BMW_l2)

	unsigned char *buf8ptr;
	int i;

	if (Machine->color_depth > 16) return(-1);

	halfclip = Machine->visible_area;
	i = halfclip.max_y - halfclip.min_y + 1;
	halfclip.max_y = halfclip.min_y + (i >> 1) - 1;

	if ((tmpbitmap = auto_bitmap_alloc(BMW, BMW)) == NULL) return(-1);

	charmap0 = tilemap_create(splndrbt_char0info, tilemap_scan_cols, TILEMAP_TRANSPARENT_COLOR, 8, 8, 32, 32);
	tilemap_set_transparent_pen(charmap0, 0);
	tilemap_set_scrolldx(charmap0, 8, 8);
	tilemap_set_scrolldy(charmap0, 32, 32);

	charmap1 = tilemap_create(splndrbt_char1info, tilemap_scan_cols, TILEMAP_TRANSPARENT_COLOR, 8, 8, 32, 32);
	tilemap_set_transparent_pen(charmap1, 0);
	tilemap_set_scrolldx(charmap1, 8, 8);
	tilemap_set_scrolldy(charmap1, 32, 32);

	if ((buf8ptr = auto_malloc(videoram_size * 2)) == NULL) return(-1);
	charram0 = (data16_t*)buf8ptr;
	charram1 = (data16_t*)(buf8ptr + videoram_size);

	if ((dirtybuf = auto_malloc(0x800)) == NULL) return(-1);
	memset(dirtybuf, 1, 0x800);

	if ((prestep = (struct PRESTEP_TYPE *)auto_malloc(i * sizeof(struct PRESTEP_TYPE))) == NULL) return(-1);
	splndrbt_prestep(prestep, &Machine->visible_area, BMW, 434, 96, 480);

	defcharram = videoram16 + videoram_size / 2;

	video_init_common();
	splndrbt_video_reset();

	return (0);
}

MACHINE_INIT( splndrbt )
{
	splndrbt_video_reset();
}

/******************************************************************************/
/* Realtime Functions*/

/* Equites Hardware*/
static void equites_update_clut(void)
{
	pen_t *colortable;
	int i, c;

	colortable = Machine->remapped_colortable;
	c = *bgcolor;

	for (i=0x80; i<0x100; i+=0x08) colortable[i] = c;
}

static void equites_draw_scroll(struct mame_bitmap *bitmap)
{
#define TILE_BANKBASE 1

	int i, offsx, offsy, skipx, skipy, dispx, dispy, encode, bank, tile, color, fy, fx, fxy, x, y, flipadjx;

	flipadjx = (equites_flip) ? 10 : 0;

	dispy = scrolly & 0x0f;
	offsy = scrolly & 0xf0;
	dispx = (scrollx + flipadjx) & 0x0f;
	offsx = (scrollx + flipadjx)>>4 & 0xf;
	if (dispy > 7)
	{
		offsy += 0x10;
		dispy -= 0x10;
	}
	if (dispx > 7)
	{
		offsx++;
		dispx -= 0x10;
	}

	for (i=0; i<255; i++)
	{
		skipx = i & 0x0f;
		skipy = i & 0xf0;
		encode = spriteram16_2[((offsy + skipy) & 0xf0) + ((offsx + skipx) & 0x0f)];
		bank = (encode>>8 & 0x01) + TILE_BANKBASE;
		tile = encode & 0xff;
		fxy = encode & 0x0800;
		color = encode>>12 & 0x0f;
		fx = (encode & 0x0400) | fxy;
		fy = (encode & 0x0200) | fxy;
		x = (skipx<<4) - dispx + BMPAD;
		y = skipy - dispy + BMPAD;

		drawgfx( bitmap,
			 Machine->gfx[bank],
			 tile, color,
			 fx, fy,
			 x, y,
			 0, TRANSPARENCY_NONE, 0);
	}

#undef TILE_BANKBASE
}

static void equites_draw_sprites(struct mame_bitmap *bitmap)
{
#define SPRITE_BANKBASE 3
#define SHIFTX -4
#define SHIFTY 1

	struct GfxElement *gfx;
	data16_t *sptr, *eptr;
	int encode, bank, tile, color, fy, fx, fxy, absx, absy, sx, sy, flipadjx;

	flipadjx = (equites_flip) ? 8 : 0;

	sptr = spriteram16;
	eptr = sptr + 0x100;
	for (; sptr<eptr; sptr+=2)
	{
		encode = *(sptr + 1);
		if (encode)
		{
			bank = (encode>>8 & 0x01) + SPRITE_BANKBASE;
			gfx = Machine->gfx[bank];
			tile = encode & 0xff;
			fxy = encode & 0x800;
			encode = ~encode & 0xf600;
			fx = (encode & 0x400) | fxy;
			fy = (encode & 0x200) | fxy;
			color = encode>>12 & 0x0f;
			encode = *sptr;
			sx = encode>>8 & 0xff;
			sy = encode & 0xff;
			absx = (sx + flipadjx + SHIFTX) & 0xff;
			absy = (sy + SHIFTY) & 0xff;
			if (absx >= 248) absx -= 256;
			if (absy >= 248) absy -= 256;

			drawgfx(bitmap,
				gfx,
				tile, color,
				fx, fy,
				absx + BMPAD, absy + BMPAD,
				0, TRANSPARENCY_PEN, 0);
		}
	}

#undef SHIFTY
#undef SHIFTX
#undef SPRITE_BANKBASE
}

VIDEO_UPDATE( equites )
{
	equites_update_clut();
	equites_draw_scroll(bitmap);
	equites_draw_sprites(bitmap);
	plot_box(bitmap, cliprect->min_x, cliprect->min_y, maskwidth, maskheight, maskcolor);
	plot_box(bitmap, cliprect->max_x-maskwidth+1, cliprect->min_y, maskwidth, maskheight, maskcolor);
	tilemap_draw(bitmap, cliprect, charmap0, 0, 0);
}

/* Splendor Blast Hardware*/
static void splndrbt_update_clut(void)
{
	pen_t *colortable;
	int c;

	colortable = Machine->remapped_colortable;
	c = *bgcolor;

	switch(equites_id)
	{
		case 0x8511:
			colortable[0x114] = c;
		break;
	}
}

static void splndrbt_draw_scroll(struct mame_bitmap *bitmap)
{
#define TILE_BANKBASE 1

	int data, bank, tile, color, fx, fy, x, y, i;

	for (i=0; i<0x400; i++)
	if (dirtybuf[i])
	{
		dirtybuf[i] = 0;

		x = (i & 0x1f) << 4;
		y = (i & ~0x1f) >> 1;
		if (!(data = spriteram16_2[i])) { plot_box(bitmap, x, y, 16, 16, *bgcolor); continue; }
		tile = data & 0xff;
		data >>= 8;
		bank = TILE_BANKBASE;
		color = data>>3 & 0x1f;
		bank += data & 1;
		fx = data & 4;
		fy = data & 2;

		drawgfx(bitmap,
			Machine->gfx[bank],
			tile, color,
			fx, fy,
			x, y,
			0, TRANSPARENCY_NONE, 0);
	}

#undef TILE_BANKBASE
}

static void splndrbt_slantcopy(
	struct mame_bitmap *src_bitmap,
	struct mame_bitmap *dst_bitmap,
	const struct rectangle *dst_clip,
	int src_x, int src_y,
	unsigned src_w, unsigned src_h,
	unsigned dst_startw, unsigned dst_endw,
	struct PRESTEP_TYPE *ps)
{
#define WARP ((1<<BMW_l2)-1)

	data16_t *src_base, *src_ptr, *dst_ptr;
	int src_pitch, dst_pitch, dst_wdiff, dst_visw, dst_vish;
	int dst_curline, dst_xend, src_fsx, src_fdx, eax, ebx, ecx, edx;

	src_base = (data16_t*)src_bitmap->base;
	src_pitch = src_bitmap->rowpixels;
	ebx = ((src_x + (src_w>>1)) & WARP) << FP_PRECISION;

	eax = dst_clip->min_x;
	edx = dst_clip->min_y;
	dst_visw = dst_clip->max_x - eax + 1;
	dst_vish = dst_clip->max_y - edx;
	dst_pitch = dst_bitmap->rowpixels;
	dst_ptr = (data16_t*)dst_bitmap->base + edx * dst_pitch + eax + (dst_visw >> 1);

	dst_wdiff = dst_endw - dst_startw;
	dst_curline = 0;

	do
	{
		dst_xend = dst_startw + (dst_wdiff * dst_curline) / dst_vish;
		eax = (src_y + ps[dst_curline].sy) & WARP;
		src_fdx = ps[dst_curline].fdx;
		src_ptr = src_base + src_pitch * eax;
		if (dst_xend > dst_visw) dst_xend = dst_visw;
		src_fsx = FP_HALF;
		dst_xend >>= 1;
		ecx = 0;

		do
		{
			eax = ebx - src_fsx;
			edx = ebx + src_fsx;
			src_fsx += src_fdx;
			eax >>= FP_PRECISION;
			edx >>= FP_PRECISION;
			eax &= WARP;
			edx &= WARP;
			eax = *(src_ptr + eax);
			edx = *(src_ptr + edx);
			*(dst_ptr - ecx - 1) = eax;
			*(dst_ptr + ecx) = edx;
		}
		while(++ecx < dst_xend);

		dst_ptr += dst_pitch;
	}
	while(++dst_curline <= dst_vish);

#undef WARP
}

static void splndrbt_draw_sprites(struct mame_bitmap *bitmap, const struct rectangle *clip)
{
#define SPRITE_BANKBASE 3
#define FP_ONE 0x10000
#define SHIFTX 0
#define SHIFTY 0

	struct GfxElement *gfx;
	data16_t *data_ptr;
	int data, sprite, fx, fy, absx, absy, sx, sy, adjy, scalex, scaley, color, i;

	gfx = Machine->gfx[SPRITE_BANKBASE];
	data_ptr = spriteram16 + 1;

	for (i=0; i<0x7e; i+=2)
	{
		data = data_ptr[i];
		if (!data) continue;

		fx = data & 0x2000;
		fy = data & 0x1000;
		sprite = data & 0x7f;
		scaley = (data>>8 & 0x0f) + 1;
		adjy = 0x10 - scaley;
		scaley = (scaley << 12) + (scaley << 8) - 1;
		if (scaley > FP_ONE) scaley = FP_ONE;

		data = data_ptr[i+1];
		sx = data & 0xff;
		absx = (sx + SHIFTX) & 0xff;
		if (absx >= 252) absx -= 256;

		color = data>>8 & 0x1f;

		data = data_ptr[i+0x80];
		sy = data & 0xff;
		absy = (-sy + adjy + SHIFTY) & 0xff;

		data = data_ptr[i+0x81];
		scalex = (data & 0x0f) + 1;
		scalex <<= 12;

		drawgfxzoom(bitmap,
			gfx,
			sprite, color,
			fx, fy,
			absx, absy, clip,
			TRANSPARENCY_PEN, 0,
			scalex, scaley);
	}

#undef SHIFTY
#undef SHIFTX
#undef FP_ONE
#undef SPRITE_BANKBASE
}

VIDEO_UPDATE( splndrbt )
{
	splndrbt_update_clut();
	fillbitmap(bitmap, *bgcolor, &halfclip);
	splndrbt_draw_scroll(tmpbitmap);

	splndrbt_slantcopy(
		tmpbitmap, bitmap, cliprect,
		*splndrbt_scrollx + scrollx, *splndrbt_scrolly + scrolly,
		BMW, 434, 96, 480, prestep);

	tilemap_draw(bitmap, cliprect, charmap1, 0, 0);
	splndrbt_draw_sprites(bitmap, cliprect);
	tilemap_draw(bitmap, cliprect, charmap0, 0, 0);
}

/******************************************************************************/
/* Memory Handlers*/

/* Equites Hardware*/
READ16_HANDLER(equites_spriteram_r)
{
	if (*spriteram16 == 0x5555) return (0);
	return (spriteram16[offset]);
}

WRITE16_HANDLER(equites_charram_w)
{
	COMBINE_DATA(videoram16 + offset);

	tilemap_mark_tile_dirty(charmap0, offset>>1);
}

WRITE16_HANDLER(equites_bgcolor_w)
{
	if (!ACCESSING_MSB) return;

	data >>= 8;

	switch (equites_id)
	{
		case 0x8400:
			if (!data) bgcolor[0] = 0;
			else if (data==0x0e) bgcolor[0] = bgcolor[2];
			else
			{
				bgcolor[2] = bgcolor[1];
				bgcolor[1] = bgcolor[0] = data;
			}
		break;
		default:
			*bgcolor = data;
	}
}

WRITE16_HANDLER(equites_scrollreg_w)
{
	if (ACCESSING_LSB) scrolly = data & 0xff;
	if (ACCESSING_MSB) scrollx = data >> 8;
}

/* Splendor Blast Hardware*/
WRITE16_HANDLER(splndrbt_selchar0_w)
{
	activecharram = charram0;
	inactivecharram = charram1;

	activecharmap = charmap0;
	inactivecharmap = charmap1;
}

WRITE16_HANDLER(splndrbt_selchar1_w)
{
	activecharram = charram1;
	inactivecharram = charram0;

	activecharmap = charmap1;
	inactivecharmap = charmap0;
}

WRITE16_HANDLER(splndrbt_charram_w)
{
	int oddoffs = offset | 1;

	COMBINE_DATA(videoram16 + offset);
	COMBINE_DATA(defcharram + offset);

	if (data==0x20 && !(offset&1))
	{
		activecharram[offset] = inactivecharram[offset] = 0x20;
		activecharram[oddoffs] = inactivecharram[oddoffs] = 0x08;
		offset >>= 1;
		tilemap_mark_tile_dirty(activecharmap, offset);
		tilemap_mark_tile_dirty(activecharmap, oddoffs);
		tilemap_mark_tile_dirty(inactivecharmap, offset);
		tilemap_mark_tile_dirty(inactivecharmap, oddoffs);
	}
	else
	{
		COMBINE_DATA(activecharram + offset);
		tilemap_mark_tile_dirty(activecharmap, offset>>1);
	}
}

READ16_HANDLER(splndrbt_bankedchar_r)
{
	if (defcharram[offset|1]==0x3f) return(0);
	return(defcharram[offset]);
}

WRITE16_HANDLER(splndrbt_bankedchar_w)
{
	COMBINE_DATA(defcharram + offset);
	COMBINE_DATA(activecharram + offset);
	inactivecharram[offset&~1] = 0x20;
	inactivecharram[offset|1] = 0x08;
	offset >>= 1;
	tilemap_mark_tile_dirty(activecharmap, offset);
	tilemap_mark_tile_dirty(inactivecharmap, offset);
}

WRITE16_HANDLER(splndrbt_scrollram_w)
{
	COMBINE_DATA(spriteram16_2 + offset);
	dirtybuf[offset] = 1;
}

WRITE16_HANDLER(splndrbt_bgcolor_w)
{
	data >>= 8;

	if (ACCESSING_MSB && *bgcolor != data)
	{
		*bgcolor = data;
		memset(dirtybuf, 1, 0x400);
	}
}

/******************************************************************************/
