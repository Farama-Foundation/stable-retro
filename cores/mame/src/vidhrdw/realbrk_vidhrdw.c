/***************************************************************************

					  -= Billiard Academy Real Break =-

					driver by	Luca Elia (l.elia@tin.it)

	This hardware provides for:

		-	2 scrolling background layers, 1024 x 512 in size
			made of 16 x 16 tiles with 256 colors

		-	1 text layer (fixed?), 512 x 256 in size
			made of 8 x 8 tiles with 16 colors

		-	0x300 sprites made of 16x16 tiles, both 256 or 16 colors
			per tile and from 1 to 32x32 (more?) tiles per sprite.
			Sprites can zoom / shrink


***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "realbrk.h"

data16_t *realbrk_vram_0, *realbrk_vram_1, *realbrk_vram_2, *realbrk_vregs;

static int disable_video;

WRITE16_HANDLER( realbrk_flipscreen_w )
{
	if (ACCESSING_LSB)
	{
		coin_counter_w(0,	data & 0x0001);
		coin_counter_w(1,	data & 0x0004);

		flip_screen_set(	data & 0x0080);
	}

	if (ACCESSING_MSB)
	{
		disable_video	=	data & 0x8000;
	}
}

/***************************************************************************

								Tilemaps


***************************************************************************/

static struct tilemap	*tilemap_0,*tilemap_1,	/* Backgrounds*/
						*tilemap_2;				/* Text*/


/***************************************************************************

							Background Tilemaps

	Offset: 	Bits:					Value:

		0.w		f--- ---- ---- ----		Flip Y
				-e-- ---- ---- ----		Flip X
				--dc ba98 7--- ----
				---- ---- -654 3210		Color

		2.w								Code

***************************************************************************/

static void get_tile_info_0(int tile_index)
{
	data16_t attr = realbrk_vram_0[tile_index * 2 + 0];
	data16_t code = realbrk_vram_0[tile_index * 2 + 1];
	SET_TILE_INFO(
			0,
			code,
			attr & 0x7f,
			TILE_FLIPYX( attr >> 14 ))
}

static void get_tile_info_1(int tile_index)
{
	data16_t attr = realbrk_vram_1[tile_index * 2 + 0];
	data16_t code = realbrk_vram_1[tile_index * 2 + 1];
	SET_TILE_INFO(
			0,
			code,
			attr & 0x7f,
			TILE_FLIPYX( attr >> 14 ))
}

WRITE16_HANDLER( realbrk_vram_0_w )
{
	data16_t old_data	=	realbrk_vram_0[offset];
	data16_t new_data	=	COMBINE_DATA(&realbrk_vram_0[offset]);
	if (old_data != new_data)	tilemap_mark_tile_dirty(tilemap_0,offset/2);
}

WRITE16_HANDLER( realbrk_vram_1_w )
{
	data16_t old_data	=	realbrk_vram_1[offset];
	data16_t new_data	=	COMBINE_DATA(&realbrk_vram_1[offset]);
	if (old_data != new_data)	tilemap_mark_tile_dirty(tilemap_1,offset/2);
}

/***************************************************************************

								Text Tilemap

	Offset: 	Bits:					Value:

		0.w		fedc ---- ---- ----		Color
				---- ba98 7654 3210		Code

	The full palette of 0x8000 colors can be used by this tilemap since
	a video register selects the higher bits of the color code.

***************************************************************************/

static void get_tile_info_2(int tile_index)
{
	data16_t code = realbrk_vram_2[tile_index];
	SET_TILE_INFO(
			1,
			code & 0x0fff,
			((code & 0xf000) >> 12) | ((realbrk_vregs[0xa/2] & 0x7f) << 4),
			0)
}

WRITE16_HANDLER( realbrk_vram_2_w )
{
	data16_t old_data	=	realbrk_vram_2[offset];
	data16_t new_data	=	COMBINE_DATA(&realbrk_vram_2[offset]);
	if (old_data != new_data)	tilemap_mark_tile_dirty(tilemap_2,offset);
}



/***************************************************************************


							Video Hardware Init


***************************************************************************/

VIDEO_START(realbrk)
{
	/* Backgrounds */
	tilemap_0 = tilemap_create(	get_tile_info_0, tilemap_scan_rows,
								TILEMAP_TRANSPARENT,
								16,16,
								0x40, 0x20);

	tilemap_1 = tilemap_create(	get_tile_info_1, tilemap_scan_rows,
								TILEMAP_TRANSPARENT,
								16,16,
								0x40, 0x20);

	/* Text */
	tilemap_2 = tilemap_create(	get_tile_info_2, tilemap_scan_rows,
								TILEMAP_TRANSPARENT,
								8,8,
								0x40, 0x20);

	if ( !tilemap_0 || !tilemap_1 || !tilemap_2 )
		return 1;

	tilemap_set_transparent_pen(tilemap_0,0);
	tilemap_set_transparent_pen(tilemap_1,0);
	tilemap_set_transparent_pen(tilemap_2,0);

	return 0;
}

/***************************************************************************

								Sprites Drawing

	Sprites RAM is 0x4000 bytes long with each sprite needing 16 bytes.

	Not all sprites must be displayed: there is a list of words at offset
	0x3000. If the high bit of a word is 0 the low bits form the index
	of a sprite to be drawn. 0x300 items of the list seem to be used.

	Each sprite is made of several 16x16 tiles (from 1 to 32x32) and
	can be zoomed / shrinked in size.

	There are two set of tiles: with 256 or 16 colors.

	Offset: 	Bits:					Value:

		0.w								Y

		2.w								X

		4.w		fedc ba98 ---- ----		Number of tiles along Y, minus 1 (5 bits or more ?)
				---- ---- 7654 3210		Number of tiles along X, minus 1 (5 bits or more ?)

		6.w		fedc ba98 ---- ----		Zoom factor along Y (0x40 = no zoom)
				---- ---- 7654 3210		Zoom factor along X (0x40 = no zoom)

		8.w		fe-- ---- ---- ----
				--d- ---- ---- ----		Flip Y
				---c ---- ---- ----		Flip X
				---- ba98 7654 3210		Priority?

		A.w		fedc b--- ---- ----
				---- -a98 7654 3210		Color

		C.w		fedc ba9- ---- ----
				---- ---8 ---- ----
				---- ---- 7654 321-
				---- ---- ---- ---0		1 = Use 16 color sprites, 0 = Use 256 color sprites 

		E.w								Code

***************************************************************************/

static void realbrk_draw_sprites(struct mame_bitmap *bitmap,const struct rectangle *cliprect)
{
	int offs;

	int max_x		=	Machine->drv->screen_width;
	int max_y		=	Machine->drv->screen_height;

	for ( offs = 0x3000/2; offs < 0x3600/2; offs += 2/2 )
	{
		int sx, sy, dim, zoom, flip, color, attr, code, flipx, flipy, gfx;

		int x, xdim, xnum, xstart, xend, xinc;
		int y, ydim, ynum, ystart, yend, yinc;

		data16_t *s;

		if (spriteram16[offs] & 0x8000)	continue;

		s		=		&spriteram16[(spriteram16[offs] & 0x3ff) * 16/2];

		sy		=		s[ 0 ];
		sx		=		s[ 1 ];
		dim		=		s[ 2 ];
		zoom	=		s[ 3 ];
		flip	=		s[ 4 ];
		color	=		s[ 5 ];
		attr	=		s[ 6 ];
		code	=		s[ 7 ];

		xnum	=		((dim >> 0) & 0x1f) + 1;
		ynum	=		((dim >> 8) & 0x1f) + 1;

		flipx	=		flip & 0x0100;
		flipy	=		flip & 0x0200;

		gfx		=		(attr & 0x0001) + 2;

		sx		=		((sx & 0x1ff) - (sx & 0x200)) << 16;
		sy		=		((sy & 0x0ff) - (sy & 0x100)) << 16;

		xdim	=		((zoom & 0x00ff) >> 0) << (16-6+4);
		ydim	=		((zoom & 0xff00) >> 8) << (16-6+4);

		if (flip_screen_x)	{	flipx = !flipx;		sx = (max_x << 16) - sx - xnum * xdim;	}
		if (flip_screen_y)	{	flipy = !flipy;		sy = (max_y << 16) - sy - ynum * ydim;	}

		if (flipx)	{ xstart = xnum-1;  xend = -1;    xinc = -1; }
		else		{ xstart = 0;       xend = xnum;  xinc = +1; }

		if (flipy)	{ ystart = ynum-1;  yend = -1;    yinc = -1; }
		else		{ ystart = 0;       yend = ynum;  yinc = +1; }

		for (y = ystart; y != yend; y += yinc)
		{
			for (x = xstart; x != xend; x += xinc)
			{
				int currx = (sx + x * xdim) / 0x10000;
				int curry = (sy + y * ydim) / 0x10000;

				int scalex = (sx + (x + 1) * xdim) / 0x10000 - currx;
				int scaley = (sy + (y + 1) * ydim) / 0x10000 - curry;

				drawgfxzoom(	bitmap,Machine->gfx[gfx],
								code++,
								color,
								flipx, flipy,
								currx, curry,
								cliprect,TRANSPARENCY_PEN,0,
								scalex << 12, scaley << 12);
			}
		}
	}
}


/***************************************************************************

								Screen Drawing

	Video Registers:

	Offset: 	Bits:					Value:

	0.w									Background 0 Scroll Y

	2.w									Background 0 Scroll X

	4.w									Background 1 Scroll Y

	6.w									Background 1 Scroll X

	8.w			fedc ba98 ---- ----		? bit f = flip
				---- ---- 7654 3210

	A.w			fedc ba98 7--- ----
				---- ---- -654 3210		Color codes high bits for the text tilemap

	C.w			f--- ---- ---- ----
				-edc ba98 7654 3210		Index of the background color

***************************************************************************/

WRITE16_HANDLER( realbrk_vregs_w )
{
	data16_t old_data = realbrk_vregs[offset];
	data16_t new_data = COMBINE_DATA(&realbrk_vregs[offset]);
	if (new_data != old_data)
	{
		if (offset == 0xa/2)
			tilemap_mark_all_tiles_dirty(tilemap_0);
	}
}

VIDEO_UPDATE(realbrk)
{
	int layers_ctrl = -1;

	tilemap_set_scrolly(tilemap_0, 0, realbrk_vregs[0x0/2]);
	tilemap_set_scrollx(tilemap_0, 0, realbrk_vregs[0x2/2]);

	tilemap_set_scrolly(tilemap_1, 0, realbrk_vregs[0x4/2]);
	tilemap_set_scrollx(tilemap_1, 0, realbrk_vregs[0x6/2]);

#ifdef MAME_DEBUG
if ( keyboard_pressed(KEYCODE_Z) )
{	int msk = 0;
	if (keyboard_pressed(KEYCODE_Q))	msk |= 1;
	if (keyboard_pressed(KEYCODE_W))	msk |= 2;
	if (keyboard_pressed(KEYCODE_E))	msk |= 4;
	if (keyboard_pressed(KEYCODE_A))	msk |= 8;
	if (msk != 0) layers_ctrl &= msk;	}
#endif

	if (disable_video)
	{
		fillbitmap(bitmap,get_black_pen(),cliprect);
		return;
	}
	else
		fillbitmap(bitmap,Machine->pens[realbrk_vregs[0xc/2] & 0x7fff],cliprect);

	if (layers_ctrl & 2)	tilemap_draw(bitmap,cliprect,tilemap_1,0,0);
	if (layers_ctrl & 1)	tilemap_draw(bitmap,cliprect,tilemap_0,0,0);

	if (layers_ctrl & 8)	realbrk_draw_sprites(bitmap,cliprect);

	if (layers_ctrl & 4)	tilemap_draw(bitmap,cliprect,tilemap_2,0,0);

/*	usrintf_showmessage("%04x",realbrk_vregs[0x8/2]);*/
}
