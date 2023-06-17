/***************************************************************************

Functions to emulate the video hardware of the machine.

This hardware of the machine is similar with that of Mutant Night.
The difference between the two machines is that there are three BG
layers as against just one BG layer in Mutant Night.


Foreground RAM format ( Foreground RAM format is same as Mutant Night )
--------------------------------------------------------------
 +0         +1
 xxxx xxxx  ---- ----		= bottom 8 bits of tile number
 ---- ----  xx-- ----		= top 2 bits of tile number
 ---- ----  --x- ----		= flip X
 ---- ----  ---x ----		= flip Y
 ---- ----  ---- xxxx		= color ( 00h - 0fh )


Background RAM format
--------------------------------------------------------------
 +0         +1
 xxxx xxxx  ---- ----		= bottom 8 bits of tile number
 ---- ----  x--- ----		= bit 9 of tile number
 ---- ----  -x-- ----		= bit 8 of tile number
 ---- ----  --x- ----		= bit 10 of tile number
 ---- ----  ---x ----		= bit 11 of tile number (the most top bit)
 ---- ----  ---- xxxx		= color ( 00h - 1fh )


Sprite RAM format	( Sprite format is same as Mutant Night )
--------------------------------------------------------------
 +0         +1         +2         +3         +4
 xxxx xxxx  ---- ----  ---- ----  ---- ----  ---- ----	= sprite Y position
 ---- ----  xxxx xxxx  ---- ----  ---- ----  ---- ----  = bottom 8 bits of sprite X position
 ---- ----  ---- ----  xx-- ----  ---- ----  ---- ----	= middle 2 bits of sprite number
 ---- ----  ---- ----  --x- ----  ---- ----  ---- ----	= flip X
 ---- ----  ---- ----  ---x ----  ---- ----  ---- ----  = flip Y
 ---- ----  ---- ----  ---- x---  ---- ----  ---- ----	= top bit of sprite number
 ---- ----  ---- ----  ---- -x--  ---- ----  ---- ----	= 0:normal size (16x16)  1:big size (32x32)
 ---- ----  ---- ----  ---- --x-  ---- ----  ---- ----	= sprite on / off
 ---- ----  ---- ----  ---- ---x  ---- ----  ---- ----	= top bit of sprite X position
 ---- ----  ---- ----  ---- ----  xxxx xxxx  ---- ----  = bottom 8 bits of sprite number
 ---- ----  ---- ----  ---- ----  ---- ----  xxxx xxxx	= color


Scroll RAM format (Omega Fighter)
--------------------------------------------------------------
    +0         +1
 X  ???? -xxx  xxxx xxxx		= scroll X (0 - 0x3ff)
 Y  ???? ---x  xxxx xxxx        = scroll Y (0 - 0x1ff)

Scroll RAM format (Atomic Robokid)
--------------------------------------------------------------
    +0         +1
 X  ???? ---x  xxxx xxxx		= scroll X (0 - 0x1ff)
 Y  ???? ---x  xxxx xxxx        = scroll Y (0 - 0x1ff)


Real screen resolution and virtual one
--------------------------------------------------------------
                Real        Virtual
Omega Fighter   256x192(H)  2048x512
Atomic Robokid  256x192(H)  512x512


***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "tilemap.h"



/**************************************************************************
  Variables
**************************************************************************/

unsigned char *omegaf_fg_videoram;
unsigned char *omegaf_bg0_videoram;
unsigned char *omegaf_bg1_videoram;
unsigned char *omegaf_bg2_videoram;
size_t omegaf_fgvideoram_size;

static int omegaf_bg0_bank = 0;
static int omegaf_bg1_bank = 0;
static int omegaf_bg2_bank = 0;

unsigned char *omegaf_bg0_scroll_x;
unsigned char *omegaf_bg1_scroll_x;
unsigned char *omegaf_bg2_scroll_x;
unsigned char *omegaf_bg0_scroll_y;
unsigned char *omegaf_bg1_scroll_y;
unsigned char *omegaf_bg2_scroll_y;

static struct tilemap *fg_tilemap;
static struct tilemap *bg0_tilemap;
static struct tilemap *bg1_tilemap;
static struct tilemap *bg2_tilemap;

static int bg0_enabled = 1;
static int bg1_enabled = 1;
static int bg2_enabled = 1;

static struct mame_bitmap *bitmap_sp;	/* for sprite overdraw */
static int sprite_overdraw_enabled = 0;

static int scrollx_mask = 0x07ff;
static int bank_mask = 1;


/***************************************************************************
  Callbacks for the tilemap code
***************************************************************************/

static void get_bg0_tile_info(int tile_index)
{
	int color, tile, hi, lo;

	int attrib = ( (tile_index & 0x00f) | ( (tile_index & 0x070) << 5 ) |
	                                      ( (tile_index & 0xf80) >> 3 ) ) << 1;
	lo  = omegaf_bg0_videoram[ attrib ];
	hi  = omegaf_bg0_videoram[ attrib | 1 ];
	color = hi & 0x0f;
	tile = ( ((hi & 0x80) << 2) | ((hi & 0x40) << 2) |
	         ((hi & 0x20) << 5) | ((hi & 0x10) << 7) ) | lo;
	SET_TILE_INFO(
			0,
			tile,
			color,
			0)
	tile_info.priority = 0;
}

static void get_bg1_tile_info(int tile_index)
{
	int color, tile, hi, lo;

	int attrib = ( (tile_index & 0x00f) | ( (tile_index & 0x070) << 5 ) |
	                                      ( (tile_index & 0xf80) >> 3 ) ) << 1;
	lo  = omegaf_bg1_videoram[ attrib ];
	hi  = omegaf_bg1_videoram[ attrib | 1 ];
	color = hi & 0x0f;
	tile = ( ((hi & 0x80) << 2) | ((hi & 0x40) << 2) |
	         ((hi & 0x20) << 5) | ((hi & 0x10) << 7) ) | lo;
	SET_TILE_INFO(
			1,
			tile,
			color,
			0)
	tile_info.priority = 0;
}

static void get_bg2_tile_info(int tile_index)
{
	int color, tile, hi, lo;

	int attrib = ( (tile_index & 0x00f) | ( (tile_index & 0x070) << 5 ) |
	                                      ( (tile_index & 0xf80) >> 3 ) ) << 1;
	lo  = omegaf_bg2_videoram[ attrib ];
	hi  = omegaf_bg2_videoram[ attrib | 1 ];
	color = hi & 0x0f;
	tile = ( ((hi & 0x80) << 2) | ((hi & 0x40) << 2) |
	         ((hi & 0x20) << 5) | ((hi & 0x10) << 7) ) | lo;
	SET_TILE_INFO(
			2,
			tile,
			color,
			0)
	tile_info.priority = 0;
}

static void robokid_get_bg0_tile_info(int tile_index)
{
	int color, tile, hi, lo;

	int attrib = ( (tile_index & 0x00f) | ( (tile_index & 0x010) << 5 ) |
	                                      ( (tile_index & 0x3e0) >> 1 ) ) << 1;
	lo  = omegaf_bg0_videoram[ attrib ];
	hi  = omegaf_bg0_videoram[ attrib | 1 ];
	color = hi & 0x0f;
	tile = ( ((hi & 0x80) << 2) | ((hi & 0x40) << 2) |
	         ((hi & 0x20) << 5) | ((hi & 0x10) << 7) ) | lo;
	SET_TILE_INFO(
			0,
			tile,
			color,
			0)
	tile_info.priority = 0;
}

static void robokid_get_bg1_tile_info(int tile_index)
{
	int color, tile, hi, lo;

	int attrib = ( (tile_index & 0x00f) | ( (tile_index & 0x010) << 5 ) |
	                                      ( (tile_index & 0x3e0) >> 1 ) ) << 1;
	lo  = omegaf_bg1_videoram[ attrib ];
	hi  = omegaf_bg1_videoram[ attrib | 1 ];
	color = hi & 0x0f;
	tile = ( ((hi & 0x80) << 2) | ((hi & 0x40) << 2) |
	         ((hi & 0x20) << 5) | ((hi & 0x10) << 7) ) | lo;
	SET_TILE_INFO(
			1,
			tile,
			color,
			0)
	tile_info.priority = 0;
}

static void robokid_get_bg2_tile_info(int tile_index)
{
	int color, tile, hi, lo;

	int attrib = ( (tile_index & 0x00f) | ( (tile_index & 0x010) << 5 ) |
	                                      ( (tile_index & 0x3e0) >> 1 ) ) << 1;
	lo  = omegaf_bg2_videoram[ attrib ];
	hi  = omegaf_bg2_videoram[ attrib | 1 ];
	color = hi & 0x0f;
	tile = ( ((hi & 0x80) << 2) | ((hi & 0x40) << 2) |
	         ((hi & 0x20) << 5) | ((hi & 0x10) << 7) ) | lo;
	SET_TILE_INFO(
			2,
			tile,
			color,
			0)
	tile_info.priority = 0;
}

static void get_fg_tile_info(int tile_index)
{
	int color, tile, hi, lo;

	lo  = omegaf_fg_videoram[ tile_index << 1 ];
	hi = omegaf_fg_videoram[ (tile_index << 1) + 1 ];
	color  = hi & 0x0f;
	tile = ( ((hi & 0x80) << 2) | ((hi & 0x40) << 2) |
	         ((hi & 0x20) << 5) | ((hi & 0x10) << 7) ) | lo;
	SET_TILE_INFO(
			5,
			tile,
			color,
			0)
	tile_info.priority = 0;
}


/***************************************************************************
  Initialize and destroy video hardware emulation
***************************************************************************/

static int videoram_alloc(int size)
{
	/* create video ram */
	if ( (omegaf_bg0_videoram = auto_malloc(size)) == NULL )
		return 1;
	memset( omegaf_bg0_videoram, 0x00, size );

	if ( (omegaf_bg1_videoram = auto_malloc(size)) == NULL )
		return 1;
	memset( omegaf_bg1_videoram, 0x00, size );

	if ( (omegaf_bg2_videoram = auto_malloc(size)) == NULL )
		return 1;
	memset( omegaf_bg2_videoram, 0x00, size );

	if ( (bitmap_sp = auto_bitmap_alloc (Machine -> drv -> screen_width, Machine -> drv -> screen_height ) ) == NULL )
		return 1;

	return 0;
}

VIDEO_START( omegaf )
{
	scrollx_mask = 0x07ff;
	bank_mask = 7;

	if ( videoram_alloc(0x2000) )
		return 1;

	/*                           Info               Offset             Type                 w   h  col  row */
	fg_tilemap  = tilemap_create(get_fg_tile_info,  tilemap_scan_rows, TILEMAP_TRANSPARENT, 8,  8,  32, 32);
	bg0_tilemap = tilemap_create(get_bg0_tile_info, tilemap_scan_rows, TILEMAP_TRANSPARENT, 16, 16, 128, 32);
	bg1_tilemap = tilemap_create(get_bg1_tile_info, tilemap_scan_rows, TILEMAP_TRANSPARENT, 16, 16, 128, 32);
	bg2_tilemap = tilemap_create(get_bg2_tile_info, tilemap_scan_rows, TILEMAP_TRANSPARENT, 16, 16, 128, 32);

	if ( !fg_tilemap || !bg0_tilemap || !bg1_tilemap || !bg2_tilemap )
		return 1;

	tilemap_set_transparent_pen( fg_tilemap,  15 );
	tilemap_set_transparent_pen( bg0_tilemap, 15 );
	tilemap_set_transparent_pen( bg1_tilemap, 15 );
	tilemap_set_transparent_pen( bg2_tilemap, 15 );

	return 0;
}

VIDEO_START( robokid )
{
	scrollx_mask = 0x01ff;
	bank_mask = 1;

	if ( videoram_alloc(0x0800) )
		return 1;

	/*                           Info               Offset             Type                         w   h  col  row */
	fg_tilemap  = tilemap_create(        get_fg_tile_info,  tilemap_scan_rows, TILEMAP_TRANSPARENT, 8,  8,  32, 32);
	bg0_tilemap = tilemap_create(robokid_get_bg0_tile_info, tilemap_scan_rows, TILEMAP_TRANSPARENT, 16, 16, 32, 32);
	bg1_tilemap = tilemap_create(robokid_get_bg1_tile_info, tilemap_scan_rows, TILEMAP_TRANSPARENT, 16, 16, 32, 32);
	bg2_tilemap = tilemap_create(robokid_get_bg2_tile_info, tilemap_scan_rows, TILEMAP_TRANSPARENT, 16, 16, 32, 32);

	if ( !fg_tilemap || !bg0_tilemap || !bg1_tilemap || !bg2_tilemap )
		return 1;

	tilemap_set_transparent_pen( fg_tilemap,  15 );
	tilemap_set_transparent_pen( bg0_tilemap, 15 );
	tilemap_set_transparent_pen( bg1_tilemap, 15 );
	tilemap_set_transparent_pen( bg2_tilemap, 15 );

	return 0;
}


/***************************************************************************
  Memory handler
***************************************************************************/

WRITE_HANDLER( omegaf_bg0_bank_w )
{
	omegaf_bg0_bank = data & bank_mask;
}

WRITE_HANDLER( omegaf_bg1_bank_w )
{
	omegaf_bg1_bank = data & bank_mask;
}

WRITE_HANDLER( omegaf_bg2_bank_w )
{
	omegaf_bg2_bank = data & bank_mask;
}

READ_HANDLER( omegaf_bg0_videoram_r )
{
	return omegaf_bg0_videoram[ (omegaf_bg0_bank << 10) | offset ];
}

READ_HANDLER( omegaf_bg1_videoram_r )
{
	return omegaf_bg1_videoram[ (omegaf_bg1_bank << 10) | offset ];
}

READ_HANDLER( omegaf_bg2_videoram_r )
{
	return omegaf_bg2_videoram[ (omegaf_bg2_bank << 10) | offset ];
}

WRITE_HANDLER( omegaf_bg0_videoram_w )
{
	int address;
	int tile_index;

	address = (omegaf_bg0_bank << 10 ) | offset;
	if ( omegaf_bg0_videoram[address] != data )
	{
		omegaf_bg0_videoram[ address ] = data;
		tile_index = ( (address & 0x001e) >> 1 ) | ( (address & 0x1c00) >> 6 ) |
		             ( (address & 0x03e0) << 2 );
		tilemap_mark_tile_dirty( bg0_tilemap, tile_index );
	}
}

WRITE_HANDLER( omegaf_bg1_videoram_w )
{
	int address;
	int tile_index;

	address = (omegaf_bg1_bank << 10 ) | offset;
	if ( omegaf_bg1_videoram[address] != data )
	{
		omegaf_bg1_videoram[ address ] = data;
		tile_index = ( (address & 0x001e) >> 1 ) | ( (address & 0x1c00) >> 6 ) |
		             ( (address & 0x03e0) << 2 );
		tilemap_mark_tile_dirty( bg1_tilemap, tile_index );
	}
}

WRITE_HANDLER( omegaf_bg2_videoram_w )
{
	int address;
	int tile_index;

	address = (omegaf_bg2_bank << 10 ) | offset;
	if ( omegaf_bg2_videoram[address] != data )
	{
		omegaf_bg2_videoram[ address ] = data;
		tile_index = ( (address & 0x001e) >> 1 ) | ( (address & 0x1c00) >> 6 ) |
		             ( (address & 0x03e0) << 2 );
		tilemap_mark_tile_dirty( bg2_tilemap, tile_index );
	}
}

WRITE_HANDLER( robokid_bg0_videoram_w )
{
	int address;
	int tile_index;

	address = (omegaf_bg0_bank << 10 ) | offset;
	if ( omegaf_bg0_videoram[address] != data )
	{
		omegaf_bg0_videoram[ address ] = data;
		tile_index = ( (address & 0x001e) >> 1 ) | ( (address & 0x0400) >> 6 ) |
		               (address & 0x03e0);
		tilemap_mark_tile_dirty( bg0_tilemap, tile_index );
	}
}

WRITE_HANDLER( robokid_bg1_videoram_w )
{
	int address;
	int tile_index;

	address = (omegaf_bg1_bank << 10 ) | offset;
	if ( omegaf_bg1_videoram[address] != data )
	{
		omegaf_bg1_videoram[ address ] = data;
		tile_index = ( (address & 0x001e) >> 1 ) | ( (address & 0x0400) >> 6 ) |
		               (address & 0x03e0);
		tilemap_mark_tile_dirty( bg1_tilemap, tile_index );
	}
}

WRITE_HANDLER( robokid_bg2_videoram_w )
{
	int address;
	int tile_index;

	address = (omegaf_bg2_bank << 10 ) | offset;
	if ( omegaf_bg2_videoram[address] != data )
	{
		omegaf_bg2_videoram[ address ] = data;
		tile_index = ( (address & 0x001e) >> 1 ) | ( (address & 0x0400) >> 6 ) |
		               (address & 0x03e0);
		tilemap_mark_tile_dirty( bg2_tilemap, tile_index );
	}
}

WRITE_HANDLER( omegaf_bg0_scrollx_w )
{
	int scrollx;

	omegaf_bg0_scroll_x[offset] = data;

	scrollx = (omegaf_bg0_scroll_x[1] << 8) | omegaf_bg0_scroll_x[0];
	scrollx &= scrollx_mask;
	tilemap_set_scrollx( bg0_tilemap, 0, scrollx );
}

WRITE_HANDLER( omegaf_bg0_scrolly_w )
{
	int scrolly;

	omegaf_bg0_scroll_y[offset] = data;

	scrolly = (omegaf_bg0_scroll_y[1] << 8) | omegaf_bg0_scroll_y[0];
	scrolly &= 0x01ff;
	tilemap_set_scrolly( bg0_tilemap, 0, scrolly );
}

WRITE_HANDLER( omegaf_bg1_scrollx_w )
{
	int scrollx;

	omegaf_bg1_scroll_x[offset] = data;

	scrollx = (omegaf_bg1_scroll_x[1] << 8) | omegaf_bg1_scroll_x[0];
	scrollx &= scrollx_mask;
	tilemap_set_scrollx( bg1_tilemap, 0, scrollx );
}

WRITE_HANDLER( omegaf_bg1_scrolly_w )
{
	int scrolly;

	omegaf_bg1_scroll_y[offset] = data;

	scrolly = (omegaf_bg1_scroll_y[1] << 8) | omegaf_bg1_scroll_y[0];
	scrolly &= 0x01ff;
	tilemap_set_scrolly( bg1_tilemap, 0, scrolly );
}

WRITE_HANDLER( omegaf_bg2_scrollx_w )
{
	int scrollx;

	omegaf_bg2_scroll_x[offset] = data;

	scrollx = (omegaf_bg2_scroll_x[1] << 8) | omegaf_bg2_scroll_x[0];
	scrollx &= scrollx_mask;
	tilemap_set_scrollx( bg2_tilemap, 0, scrollx );
}

WRITE_HANDLER( omegaf_bg2_scrolly_w )
{
	int scrolly;

	omegaf_bg2_scroll_y[offset] = data;

	scrolly = (omegaf_bg2_scroll_y[1] << 8) | omegaf_bg2_scroll_y[0];
	scrolly &= 0x01ff;
	tilemap_set_scrolly( bg2_tilemap, 0, scrolly );
}

WRITE_HANDLER( omegaf_fgvideoram_w )
{
	if (omegaf_fg_videoram[offset] != data)
	{
		omegaf_fg_videoram[offset] = data;
		tilemap_mark_tile_dirty(fg_tilemap, offset >> 1);
	}
}

WRITE_HANDLER( omegaf_bg0_enabled_w )
{
	if (bg0_enabled != data)
		bg0_enabled = data;
}

WRITE_HANDLER( omegaf_bg1_enabled_w )
{
	if (bg1_enabled != data)
		bg1_enabled = data;
}

WRITE_HANDLER( omegaf_bg2_enabled_w )
{
	if (bg2_enabled != data)
		bg2_enabled = data;
}

WRITE_HANDLER( omegaf_sprite_overdraw_w )
{
	log_cb(RETRO_LOG_DEBUG, LOGPRE  "sprite overdraw flag : %02x\n", data );
	if (sprite_overdraw_enabled != (data & 1))
	{
		sprite_overdraw_enabled = data & 1;
		fillbitmap(bitmap_sp, 15, &Machine -> visible_area);
	}
}

WRITE_HANDLER( omegaf_flipscreen_w )
{
	flip_screen_set(data & 0x80);
}

/***************************************************************************
  Screen refresh
***************************************************************************/

static void draw_sprites(struct mame_bitmap *bitmap,const struct rectangle *cliprect)
{
	int offs;

	/* Draw the sprites */
	for (offs = 11 ;offs < spriteram_size; offs += 16)
	{
		int sx, sy, tile, color, flipx, flipy, big;

		if (spriteram[offs + 2] & 2)
		{
			sx = spriteram[offs + 1];
			sy = spriteram[offs];
			if (spriteram[offs + 2] & 1)
				sx -= 256;
			tile = spriteram[offs + 3] |
					((spriteram[offs + 2] & 0xc0) << 2) |
					((spriteram[offs + 2] & 0x08) << 7);

			big  = spriteram[offs + 2] & 4;
			if (big)
				tile >>= 2;
			flipx = spriteram[offs + 2] & 0x10;
			flipy = spriteram[offs + 2] & 0x20;
			color = spriteram[offs + 4] & 0x0f;

			drawgfx(bitmap,Machine->gfx[(big) ? 4 : 3],
					tile,
					color,
					flipx,flipy,
					sx,sy,
					&Machine->visible_area,
					TRANSPARENCY_PEN, 15);
		}
	}
}

VIDEO_UPDATE( omegaf )
{
	fillbitmap(bitmap,Machine->pens[15],cliprect);	/* ??*/

	if (bg0_enabled)	tilemap_draw(bitmap,cliprect, bg0_tilemap, 0, 0);
	if (bg1_enabled)	tilemap_draw(bitmap,cliprect, bg1_tilemap, 0, 0);
	if (bg2_enabled)	tilemap_draw(bitmap,cliprect, bg2_tilemap, 0, 0);
	if ( sprite_overdraw_enabled )				/* overdraw sprite mode */
	{
		draw_sprites(bitmap_sp,cliprect);
		copybitmap(bitmap, bitmap_sp, 0, 0, 0, 0,
		           cliprect, TRANSPARENCY_PEN, 15);
	}
	else										/* normal sprite mode */
		draw_sprites(bitmap,cliprect);
	tilemap_draw(bitmap,cliprect, fg_tilemap, 0, 0);
}
