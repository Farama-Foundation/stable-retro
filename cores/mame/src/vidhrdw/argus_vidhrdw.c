/***************************************************************************

Functions to emulate the video hardware of the machine.


BG RAM format [Argus and Butasan]
-----------------------------------------------------------------------------
 +0         +1
 xxxx xxxx  ---- ---- = 1st - 8th bits of tile number
 ---- ----  xx-- ---- = 9th and 10th bit of tile number
 ---- ----  --x- ---- = flip y
 ---- ----  ---x ---- = flip x
 ---- ----  ---- xxxx = color

BG RAM format [Valtric]
-----------------------------------------------------------------------------
 +0         +1
 xxxx xxxx  ---- ---- = 1st - 8th bits of tile number
 ---- ----  xx-- ---- = 9th and 10th bit of tile number
 ---- ----  --x- ---- = 11th bit of tile number
 ---- ----  ---- xxxx = color


Text RAM format [Argus, Valtric and Butasan]
-----------------------------------------------------------------------------
 +0         +1
 xxxx xxxx  ---- ---- = low bits of tile number
 ---- ----  xx-- ---- = high bits of tile number
 ---- ----  --x- ---- = flip y
 ---- ----  ---x ---- = flip x
 ---- ----  ---- xxxx = color


Sprite RAM format [Argus]
-----------------------------------------------------------------------------
 +11        +12        +13        +14        +15
 xxxx xxxx  ---- ----  ---- ----  ---- ----  ---- ---- = sprite y
 ---- ----  xxxx xxxx  ---- ----  ---- ----  ---- ---- = low bits of sprite x
 ---- ----  ---- ----  xx-- ----  ---- ----  ---- ---- = high bits of tile number
 ---- ----  ---- ----  --x- ----  ---- ----  ---- ---- = flip y
 ---- ----  ---- ----  ---x ----  ---- ----  ---- ---- = flip x
 ---- ----  ---- ----  ---- --x-  ---- ----  ---- ---- = high bit of sprite y
 ---- ----  ---- ----  ---- ---x  ---- ----  ---- ---- = high bit of sprite x
 ---- ----  ---- ----  ---- ----  xxxx xxxx  ---- ---- = low bits of tile number
 ---- ----  ---- ----  ---- ----  ---- ----  ---- x--- = BG1 / sprite priority (Argus only)
 ---- ----  ---- ----  ---- ----  ---- ----  ---- -xxx = color

Sprite RAM format [Valtric]
-----------------------------------------------------------------------------
 +11        +12        +13        +14        +15
 xxxx xxxx  ---- ----  ---- ----  ---- ----  ---- ---- = sprite y
 ---- ----  xxxx xxxx  ---- ----  ---- ----  ---- ---- = low bits of sprite x
 ---- ----  ---- ----  xx-- ----  ---- ----  ---- ---- = high bits of tile number
 ---- ----  ---- ----  --x- ----  ---- ----  ---- ---- = flip y
 ---- ----  ---- ----  ---x ----  ---- ----  ---- ---- = flip x
 ---- ----  ---- ----  ---- --x-  ---- ----  ---- ---- = high bit of sprite y
 ---- ----  ---- ----  ---- ---x  ---- ----  ---- ---- = high bit of sprite x
 ---- ----  ---- ----  ---- ----  xxxx xxxx  ---- ---- = low bits of tile number
 ---- ----  ---- ----  ---- ----  ---- ----  ---- xxxx = color

Sprite RAM format [Butasan]
-----------------------------------------------------------------------------
 +8         +9         +10        +11        +12
 ---- -x--  ---- ----  ---- ----  ---- ----  ---- ---- = flip y
 ---- ---x  ---- ----  ---- ----  ---- ----  ---- ---- = flip x
 ---- ----  ---- xxxx  ---- ----  ---- ----  ---- ---- = color ($00 - $0B)
 ---- ----  ---- ----  xxxx xxxx  ---- ----  ---- ---- = low bits of sprite x
 ---- ----  ---- ----  ---- ----  ---- ---x  ---- ---- = top bit of sprite x
 ---- ----  ---- ----  ---- ----  ---- ----  xxxx xxxx = low bits of sprite y
 +13        +14        +15
 ---- ---x  ---- ----  ---- ---- = top bit of sprite y
 ---- ----  xxxx xxxx  ---- ---- = low bits of tile number
 ---- ----  ---- ----  ---- xxxx = top bits of tile number

(*) Sprite size is defined by its offset.
    $F000 - $F0FF : 16x32    $F100 - $F2FF : 16x16
    $F300 - $F3FF : 16x32    $F400 - $F57F : 16x16
    $F580 - $F61F : 32x32    $F620 - $F67F : 64x64


Scroll RAM of X and Y coordinates [Argus, Valtric and Butasan]
-----------------------------------------------------------------------------
 +0         +1
 xxxx xxxx  ---- ---- = scroll value
 ---- ----  ---- ---x = top bit of scroll value


Video effect RAM ( $C30C )
-----------------------------------------------------------------------------
 +0
 ---- ---x  = BG enable bit
 ---- --x-  = gray scale effect or tile bank select.


Flip screen controller
-----------------------------------------------------------------------------
 +0
 x--- ----  = flip screen


BG0 palette intensity ( $C47F, $C4FF )
-----------------------------------------------------------------------------
 +0 (c47f)  +1 (c4ff)
 xxxx ----  ---- ---- = red intensity
 ---- xxxx  ---- ---- = green intensity
 ---- ----  xxxx ---- = blue intensity


(*) Things which are not emulated.
 - Color $000 - 00f, $01e, $02e ... are half transparent color.
 - Maybe, BG0 scroll value of Valtric is used for mosaic effect.
 - Sprite priority bit may be present in Butasan. But I don't know
   what is happened when it is set.

***************************************************************************/

#include "driver.h"
#include "tilemap.h"
#include "vidhrdw/generic.h"


#define BUTASAN_TEXT_RAMSIZE		0x0800
#define BUTASAN_BG0_RAMSIZE			0x0800
#define BUTASAN_TXBACK_RAMSIZE		0x0800
#define BUTASAN_BG0BACK_RAMSIZE		0x0800


data8_t *argus_paletteram;
data8_t *argus_txram;
data8_t *argus_bg0_scrollx;
data8_t *argus_bg0_scrolly;
data8_t *argus_bg1ram;
data8_t *argus_bg1_scrollx;
data8_t *argus_bg1_scrolly;
data8_t *butasan_bg1ram;

static data8_t *argus_dummy_bg0ram;
static data8_t *butasan_txram;
static data8_t *butasan_bg0ram;
static data8_t *butasan_bg0backram;
static data8_t *butasan_txbackram;

static struct tilemap *tx_tilemap  = NULL;
static struct tilemap *bg0_tilemap = NULL;
static struct tilemap *bg1_tilemap = NULL;

static data8_t argus_bg_status    = 0x01;
static data8_t butasan_bg1_status = 0x01;
static data8_t argus_bg_purple  = 0;
static data8_t argus_flipscreen = 0;

static int argus_palette_intensity = 0;

/* VROM scroll related for Argus */
static int lowbitscroll = 0;
static int prvscrollx = 0;


/***************************************************************************
  Callbacks for the tilemap code
***************************************************************************/

static void argus_get_tx_tile_info(int tile_index)
{
	int hi, lo;

	lo = argus_txram[  tile_index << 1  ];
	hi = argus_txram[ (tile_index << 1) + 1 ];

	SET_TILE_INFO(
			3,
			((hi & 0xc0) << 2) | lo,
			hi & 0x0f,
			TILE_FLIPYX((hi & 0x30) >> 4))
}

static void argus_get_bg0_tile_info(int tile_index)
{
	int hi, lo;

	lo = argus_dummy_bg0ram[  tile_index << 1  ];
	hi = argus_dummy_bg0ram[ (tile_index << 1) + 1 ];

	SET_TILE_INFO(
			1,
			((hi & 0xc0) << 2) | lo,
			hi & 0x0f,
			TILE_FLIPYX((hi & 0x30) >> 4))
}

static void argus_get_bg1_tile_info(int tile_index)
{
	int hi, lo;

	lo = argus_bg1ram[  tile_index << 1  ];
	hi = argus_bg1ram[ (tile_index << 1) + 1 ];

	SET_TILE_INFO(
			2,
			lo,
			hi & 0x0f,
			TILE_FLIPYX((hi & 0x30) >> 4))
}

static void valtric_get_tx_tile_info(int tile_index)
{
	int hi, lo;

	lo = argus_txram[  tile_index << 1  ];
	hi = argus_txram[ (tile_index << 1) + 1 ];

	SET_TILE_INFO(
			2,
			((hi & 0xc0) << 2) | lo,
			hi & 0x0f,
			TILE_FLIPYX((hi & 0x30) >> 4))
}

static void valtric_get_bg_tile_info(int tile_index)
{
	int hi, lo, color, tile;

	lo = argus_bg1ram[  tile_index << 1  ];
	hi = argus_bg1ram[ (tile_index << 1) + 1 ];

	tile = ((hi & 0xc0) << 2) | ((hi & 0x20) << 5) | lo;
	color = hi & 0x0f;

	SET_TILE_INFO(
			1,
			tile,
			color,
			0)
}

static void butasan_get_tx_tile_info(int tile_index)
{
	int hi, lo;

	tile_index ^= 0x3e0;

	lo = butasan_txram[  tile_index << 1  ];
	hi = butasan_txram[ (tile_index << 1) + 1 ];

	SET_TILE_INFO(
			3,
			((hi & 0xc0) << 2) | lo,
			hi & 0x0f,
			TILE_FLIPYX((hi & 0x30) >> 4))
}

static void butasan_get_bg0_tile_info(int tile_index)
{
	int hi, lo;
	int attrib;

	attrib = (tile_index & 0x00f) | ((tile_index & 0x1e0) >> 1);
	attrib |= ((tile_index & 0x200) >> 1) | ((tile_index & 0x010) << 5);
	attrib ^= 0x0f0;

	lo = butasan_bg0ram[  attrib << 1  ];
	hi = butasan_bg0ram[ (attrib << 1) + 1 ];

	SET_TILE_INFO(
			1,
			((hi & 0xc0) << 2) | lo,
			hi & 0x0f,
			TILE_FLIPYX((hi & 0x30) >> 4))
}

static void butasan_get_bg1_tile_info(int tile_index)
{
	int bank, tile, attrib, color;

	attrib = (tile_index & 0x00f) | ((tile_index & 0x3e0) >> 1)
				| ((tile_index & 0x010) << 5);
	attrib ^= 0x0f0;

	bank = (butasan_bg1_status & 0x02) << 7;
	tile = butasan_bg1ram[ attrib ] | bank;
	color = (tile & 0x80) >> 7;

	SET_TILE_INFO(
			2,
			tile,
			color,
			0)
}


/***************************************************************************
  Initialize and destroy video hardware emulation
***************************************************************************/

VIDEO_START( argus )
{
	lowbitscroll = 0;
	/*                           info                      offset             type                  w   h  col  row */
	bg0_tilemap = tilemap_create(argus_get_bg0_tile_info,  tilemap_scan_cols, TILEMAP_OPAQUE,      16, 16, 32, 32);
	bg1_tilemap = tilemap_create(argus_get_bg1_tile_info,  tilemap_scan_cols, TILEMAP_TRANSPARENT, 16, 16, 32, 32);
	tx_tilemap  = tilemap_create(argus_get_tx_tile_info,   tilemap_scan_cols, TILEMAP_TRANSPARENT,  8,  8, 32, 32);

	if ( !tx_tilemap || !bg0_tilemap || !bg1_tilemap )
		return 1;

	/* dummy RAM for back ground */
	argus_dummy_bg0ram = auto_malloc( 0x800 );
	if ( argus_dummy_bg0ram == NULL )
		return 1;
	memset( argus_dummy_bg0ram, 0, 0x800 );

	memset( argus_bg0_scrollx, 0x00, 2 );

	tilemap_set_transparent_pen( bg0_tilemap, 15 );
	tilemap_set_transparent_pen( bg1_tilemap, 15 );
	tilemap_set_transparent_pen( tx_tilemap,  15 );

	return 0;
}

VIDEO_START( valtric )
{
	/*                           info                       offset             type                 w   h  col  row */
	bg1_tilemap = tilemap_create(valtric_get_bg_tile_info,  tilemap_scan_cols, TILEMAP_OPAQUE,      16, 16, 32, 32);
	tx_tilemap  = tilemap_create(valtric_get_tx_tile_info,  tilemap_scan_cols, TILEMAP_TRANSPARENT,  8,  8, 32, 32);

	if ( !tx_tilemap || !bg1_tilemap )
		return 1;

	tilemap_set_transparent_pen( bg1_tilemap, 15 );
	tilemap_set_transparent_pen( tx_tilemap,  15 );
	return 0;
}

VIDEO_START( butasan )
{
	/*                           info                       offset             type                 w   h  col  row */
	bg0_tilemap = tilemap_create(butasan_get_bg0_tile_info, tilemap_scan_rows, TILEMAP_OPAQUE,      16, 16, 32, 32);
	bg1_tilemap = tilemap_create(butasan_get_bg1_tile_info, tilemap_scan_rows, TILEMAP_OPAQUE,      16, 16, 32, 32);
	tx_tilemap  = tilemap_create(butasan_get_tx_tile_info,  tilemap_scan_rows, TILEMAP_TRANSPARENT,  8,  8, 32, 32);

	if ( !tx_tilemap || !bg0_tilemap || !bg1_tilemap )
		return 1;

	butasan_txram = auto_malloc( BUTASAN_TEXT_RAMSIZE );
	if (butasan_txram == NULL)
		return 1;

	butasan_bg0ram = auto_malloc( BUTASAN_BG0_RAMSIZE );
	if (butasan_bg0ram == NULL)
		return 1;

	butasan_txbackram = auto_malloc( BUTASAN_TXBACK_RAMSIZE );
	if (butasan_txbackram == NULL)
		return 1;

	butasan_bg0backram = auto_malloc( BUTASAN_BG0BACK_RAMSIZE );
	if (butasan_bg0backram == NULL)
		return 1;

	memset( butasan_txram,      0x00, BUTASAN_TEXT_RAMSIZE );
	memset( butasan_bg0ram,     0x00, BUTASAN_BG0_RAMSIZE );
	memset( butasan_txbackram,  0x00, BUTASAN_TXBACK_RAMSIZE );
	memset( butasan_bg0backram, 0x00, BUTASAN_BG0BACK_RAMSIZE );

	tilemap_set_transparent_pen( tx_tilemap,  15 );

	return 0;
}


/***************************************************************************
  Functions for handler of MAP roms in Argus and palette color
***************************************************************************/

/* Write bg0 pattern data to dummy bg0 ram */
static void argus_write_dummy_rams( int dramoffs, int vromoffs )
{
	int i;
	int voffs;
	int offs;

	data8_t *VROM1 = memory_region( REGION_USER1 );		/* "ag_15.bin" */
	data8_t *VROM2 = memory_region( REGION_USER2 );		/* "ag_16.bin" */

	/* offset in pattern data */
	offs = VROM1[ vromoffs ] | ( VROM1[ vromoffs + 1 ] << 8 );
	offs &= 0x7ff;

	voffs = offs * 16;
	for (i = 0 ; i < 8 ; i ++)
	{
		argus_dummy_bg0ram[ dramoffs ] = VROM2[ voffs ];
		argus_dummy_bg0ram[ dramoffs + 1 ] = VROM2[ voffs + 1 ];
		tilemap_mark_tile_dirty( bg0_tilemap, dramoffs >> 1 );
		dramoffs += 2;
		voffs += 2;
	}
}

static void argus_change_palette(int color, int data)
{
	int r, g, b;

	r = (data >> 12) & 0x0f;
	g = (data >>  8) & 0x0f;
	b = (data >>  4) & 0x0f;

	r = (r << 4) | r;
	g = (g << 4) | g;
	b = (b << 4) | b;

	palette_set_color(color, r, g, b);
}

static void argus_change_bg_palette(int color, int data)
{
	int r, g, b;
	int ir, ig, ib;

	r = (data >> 12) & 0x0f;
	g = (data >>  8) & 0x0f;
	b = (data >>  4) & 0x0f;

	ir = (argus_palette_intensity >> 12) & 0x0f;
	ig = (argus_palette_intensity >>  8) & 0x0f;
	ib = (argus_palette_intensity >>  4) & 0x0f;

	r = (r - ir > 0) ? r - ir : 0;
	g = (g - ig > 0) ? g - ig : 0;
	b = (b - ib > 0) ? b - ib : 0;

	if (argus_bg_status & 2)			/* Gray / purple scale */
	{
		r = (r + g + b) / 3;
		g = b = r;
		if (argus_bg_purple == 2)		/* Purple */
			g = 0;
	}

	r = (r << 4) | r;
	g = (g << 4) | g;
	b = (b << 4) | b;

	palette_set_color(color, r, g, b);
}


/***************************************************************************
  Memory handler
***************************************************************************/

READ_HANDLER( argus_txram_r )
{
	return argus_txram[ offset ];
}

WRITE_HANDLER( argus_txram_w )
{
	if (argus_txram[ offset ] != data)
	{
		argus_txram[ offset ] = data;
		tilemap_mark_tile_dirty(tx_tilemap, offset >> 1);
	}
}

READ_HANDLER( butasan_txram_r )
{
	return butasan_txram[ offset ];
}

WRITE_HANDLER( butasan_txram_w )
{
	if (butasan_txram[ offset ] != data)
	{
		butasan_txram[ offset ] = data;
		tilemap_mark_tile_dirty(tx_tilemap, (offset ^ 0x7c0) >> 1);
	}
}

READ_HANDLER( argus_bg1ram_r )
{
	return argus_bg1ram[ offset ];
}

WRITE_HANDLER( argus_bg1ram_w )
{
	if (argus_bg1ram[ offset ] != data)
	{
		argus_bg1ram[ offset ] = data;
		tilemap_mark_tile_dirty(bg1_tilemap, offset >> 1);
	}
}

READ_HANDLER( butasan_bg0ram_r )
{
	return butasan_bg0ram[ offset ];
}

WRITE_HANDLER( butasan_bg0ram_w )
{
	if (butasan_bg0ram[ offset ] != data)
	{
		int idx;

		butasan_bg0ram[ offset ] = data;

		idx = ((offset & 0x01f) >> 1) | ((offset & 0x400) >> 6);
		idx |= (offset & 0x3e0) ^ 0x1e0;

		tilemap_mark_tile_dirty(bg0_tilemap, idx);
	}
}

READ_HANDLER( butasan_bg1ram_r )
{
	return butasan_bg1ram[ offset ];
}

WRITE_HANDLER( butasan_bg1ram_w )
{
	if (butasan_bg1ram[ offset ] != data)
	{
		int idx;

		butasan_bg1ram[ offset ] = data;

		idx = (offset & 0x00f) | ((offset & 0x200) >> 5) | ((offset & 0x1f0) << 1);
		idx ^= 0x0f0;

		tilemap_mark_tile_dirty(bg1_tilemap, idx);
	}
}

WRITE_HANDLER ( argus_bg0_scrollx_w )
{
	if (argus_bg0_scrollx[ offset ] != data)
	{
		argus_bg0_scrollx[ offset ] = data;
	}
}

WRITE_HANDLER( argus_bg0_scrolly_w )
{
	if (argus_bg0_scrolly[ offset ] != data)
	{
		int scrolly;
		argus_bg0_scrolly[ offset ] = data;
		scrolly = argus_bg0_scrolly[0] | ( (argus_bg0_scrolly[1] & 0x01) << 8);
		if (!argus_flipscreen)
			tilemap_set_scrolly( bg0_tilemap, 0, scrolly );
		else
			tilemap_set_scrolly( bg0_tilemap, 0, (scrolly + 256) & 0x1ff );
	}
}

WRITE_HANDLER( butasan_bg0_scrollx_w )
{
	if (argus_bg0_scrollx[ offset ] != data)
	{
		int scrollx;
		argus_bg0_scrollx[ offset ] = data;
		scrollx = argus_bg0_scrollx[0] | ( (argus_bg0_scrollx[1] & 0x01) << 8);
		if (!argus_flipscreen)
			tilemap_set_scrollx( bg0_tilemap, 0, scrollx );
		else
			tilemap_set_scrollx( bg0_tilemap, 0, (scrollx + 256) & 0x1ff );
	}
}

WRITE_HANDLER( argus_bg1_scrollx_w )
{
	if (argus_bg1_scrollx[ offset ] != data)
	{
		int scrollx;
		argus_bg1_scrollx[ offset ] = data;
		scrollx = argus_bg1_scrollx[0] | ( (argus_bg1_scrollx[1] & 0x01) << 8);
		if (!argus_flipscreen)
			tilemap_set_scrollx( bg1_tilemap, 0, scrollx );
		else
			tilemap_set_scrollx( bg1_tilemap, 0, (scrollx + 256) & 0x1ff );
	}
}

WRITE_HANDLER( argus_bg1_scrolly_w )
{
	if (argus_bg1_scrolly[ offset ] != data)
	{
		int scrolly;
		argus_bg1_scrolly[ offset ] = data;
		scrolly = argus_bg1_scrolly[0] | ( (argus_bg1_scrolly[1] & 0x01) << 8);
		if (!argus_flipscreen)
			tilemap_set_scrolly( bg1_tilemap, 0, scrolly );
		else
			tilemap_set_scrolly( bg1_tilemap, 0, (scrolly + 256) & 0x1ff );
	}
}

WRITE_HANDLER( argus_bg_status_w )
{
	if (argus_bg_status != data)
	{
		argus_bg_status = data;

		/* Backgound enable */
		tilemap_set_enable(bg1_tilemap, argus_bg_status & 1);

		/* Gray / purple scale */
		if (argus_bg_status & 2)
		{
			int offs;

			for (offs = 0x400 ; offs < 0x500 ; offs ++)
			{
				argus_change_bg_palette( (offs - 0x0400) + 128,
					(argus_paletteram[offs] << 8) | argus_paletteram[offs + 0x0400] );
			}
		}
	}
}

WRITE_HANDLER( valtric_bg_status_w )
{
	if (argus_bg_status != data)
	{
		argus_bg_status = data;

		/* Backgound enable */
		tilemap_set_enable(bg1_tilemap, argus_bg_status & 1);

		/* Gray / purple scale */
		if (argus_bg_status & 2)
		{
			int offs;

			for (offs = 0x400 ; offs < 0x600 ; offs += 2)
			{
				argus_change_bg_palette( ((offs - 0x0400) >> 1) + 256,
					argus_paletteram[offs | 1] | (argus_paletteram[offs & ~1] << 8));
			}
		}
	}
}

WRITE_HANDLER( butasan_bg0_status_w )
{
	if (argus_bg_status != data)
	{
		argus_bg_status = data;

		/* Backgound enable */
		tilemap_set_enable(bg0_tilemap, argus_bg_status & 1);
	}
}

WRITE_HANDLER( argus_flipscreen_w )
{
	if (argus_flipscreen != (data >> 7))
	{
		argus_flipscreen = data >> 7;
		tilemap_set_flip( ALL_TILEMAPS, (argus_flipscreen) ? TILEMAP_FLIPY | TILEMAP_FLIPX : 0);
		if (!argus_flipscreen)
		{
			int scrollx, scrolly;

			if (bg0_tilemap != NULL)
			{
				scrollx = argus_bg0_scrollx[0] | ( (argus_bg0_scrollx[1] & 0x01) << 8);
				tilemap_set_scrollx(bg0_tilemap, 0, scrollx & 0x1ff);

				scrolly = argus_bg0_scrolly[0] | ( (argus_bg0_scrolly[1] & 0x01) << 8);
				tilemap_set_scrolly(bg0_tilemap, 0, scrolly);
			}
			scrollx = argus_bg1_scrollx[0] | ( (argus_bg1_scrollx[1] & 0x01) << 8);
			tilemap_set_scrollx(bg1_tilemap, 0, scrollx);

			scrolly = argus_bg1_scrolly[0] | ( (argus_bg1_scrolly[1] & 0x01) << 8);
			tilemap_set_scrolly(bg1_tilemap, 0, scrolly);
		}
		else
		{
			int scrollx, scrolly;

			if (bg0_tilemap != NULL)
			{
				scrollx = argus_bg0_scrollx[0] | ( (argus_bg0_scrollx[1] & 0x01) << 8);
				tilemap_set_scrollx(bg0_tilemap, 0, (scrollx + 256) & 0x1ff);

				scrolly = argus_bg0_scrolly[0] | ( (argus_bg0_scrolly[1] & 0x01) << 8);
				tilemap_set_scrolly(bg0_tilemap, 0, (scrolly + 256) & 0x1ff);
			}
			scrollx = argus_bg1_scrollx[0] | ( (argus_bg1_scrollx[1] & 0x01) << 8);
			tilemap_set_scrollx(bg1_tilemap, 0, (scrollx + 256) & 0x1ff);

			scrolly = argus_bg1_scrolly[0] | ( (argus_bg1_scrolly[1] & 0x01) << 8);
			tilemap_set_scrolly(bg1_tilemap, 0, (scrolly + 256) & 0x1ff);
		}
	}
}

READ_HANDLER( argus_paletteram_r )
{
	return argus_paletteram[ offset ];
}

WRITE_HANDLER( argus_paletteram_w )
{
	int offs;

	argus_paletteram[ offset ] = data;

	if (offset != 0x007f && offset != 0x00ff)
	{
		if (offset >= 0x0000 && offset <= 0x00ff)				/* sprite color */
		{
			if (offset & 0x80)
				offset -= 0x80;

			argus_change_palette( offset,
				(argus_paletteram[offset] << 8) | argus_paletteram[offset + 0x80] );
		}

		else if ( (offset >= 0x0400 && offset <= 0x04ff) ||
				  (offset >= 0x0800 && offset <= 0x08ff) )		/* BG0 color */
		{
			if (offset >= 0x0800)
				offset -= 0x0400;

			argus_change_bg_palette( (offset - 0x0400) + 128,
				(argus_paletteram[offset] << 8) | argus_paletteram[offset + 0x0400] );
		}

		else if ( (offset >= 0x0500 && offset <= 0x05ff) ||
				  (offset >= 0x0900 && offset <= 0x09ff) )		/* BG1 color */
		{
			if (offset >= 0x0900)
				offset -= 0x0400;

			argus_change_palette( (offset - 0x0500) + 384,
				(argus_paletteram[offset] << 8) | argus_paletteram[offset + 0x0400] );
		}

		else if ( (offset >= 0x0700 && offset <= 0x07ff) ||
				  (offset >= 0x0b00 && offset <= 0x0bff) )		/* text color */
		{
			if (offset >= 0x0b00)
				offset -= 0x0400;

			argus_change_palette( (offset - 0x0700) + 640,
				(argus_paletteram[offset] << 8) | argus_paletteram[offset + 0x0400] );
		}
	}
	else
	{
		argus_palette_intensity = (argus_paletteram[0x007f] << 8) | argus_paletteram[0x00ff];

		for (offs = 0x400 ; offs < 0x500 ; offs ++)
		{
			argus_change_bg_palette( (offs - 0x0400) + 128,
				(argus_paletteram[offs] << 8) | argus_paletteram[offs + 0x0400] );
		}

		argus_bg_purple = argus_paletteram[0x0ff] & 0x0f;
	}
}

WRITE_HANDLER( valtric_paletteram_w )
{
	int offs;

	argus_paletteram[ offset ] = data;

	if (offset != 0x01fe && offset != 0x01ff)
	{
		if (offset >= 0x0000 && offset <= 0x01ff)
		{
			argus_change_palette( offset >> 1,
				argus_paletteram[offset | 1] | (argus_paletteram[offset & ~1] << 8));
		}
		else if (offset >= 0x0400 && offset <= 0x05ff )
		{
			argus_change_bg_palette( ((offset - 0x0400) >> 1) + 256,
				argus_paletteram[offset | 1] | (argus_paletteram[offset & ~1] << 8));
		}
		else if (offset >= 0x0600 && offset <= 0x07ff )
		{
			argus_change_palette( ((offset - 0x0600) >> 1) + 512,
				argus_paletteram[offset | 1] | (argus_paletteram[offset & ~1] << 8));
		}
	}
	else
	{
		argus_palette_intensity = (argus_paletteram[0x01fe] << 8) | argus_paletteram[0x01ff];

		for (offs = 0x400 ; offs < 0x600 ; offs += 2)
		{
			argus_change_bg_palette( ((offs - 0x0400) >> 1) + 256,
				argus_paletteram[offs | 1] | (argus_paletteram[offs & ~1] << 8));
		}

		argus_bg_purple = argus_paletteram[0x01ff] & 0x0f;
	}
}

WRITE_HANDLER( butasan_paletteram_w )
{
	argus_paletteram[ offset ] = data;

	if (offset < 0x0200 )							/* BG1 color */
	{
		argus_change_palette( ((offset - 0x0000) >> 1) + 256,
			argus_paletteram[offset | 1] | (argus_paletteram[offset & ~1] << 8));
	}
	else if (offset < 0x0240 )						/* BG0 color */
	{
		argus_change_palette( ((offset - 0x0200) >> 1) + 192,
			argus_paletteram[offset | 1] | (argus_paletteram[offset & ~1] << 8));
	}
	else if (offset >= 0x0400 && offset <= 0x04ff )	/* Sprite color */
	{
		if (offset < 0x0480)			/* 16 colors */
			argus_change_palette( ((offset - 0x0400) >> 1) + 0,
				argus_paletteram[offset | 1] | (argus_paletteram[offset & ~1] << 8));
		else							/* 8  colors */
		{
			argus_change_palette( (offset & 0x70) + ((offset & 0x00f) >> 1) + 64,
				argus_paletteram[offset | 1] | (argus_paletteram[offset & ~1] << 8));
			argus_change_palette( (offset & 0x70) + ((offset & 0x00f) >> 1) + 72,
				argus_paletteram[offset | 1] | (argus_paletteram[offset & ~1] << 8));
		}
	}
	else if (offset >= 0x0600 && offset <= 0x07ff )	/* Text color */
	{
		argus_change_palette( ((offset - 0x0600) >> 1) + 512,
			argus_paletteram[offset | 1] | (argus_paletteram[offset & ~1] << 8));
	}
}

READ_HANDLER( butasan_txbackram_r )
{
	return butasan_txbackram[ offset ];
}

WRITE_HANDLER( butasan_txbackram_w )
{
	if (butasan_txbackram[ offset ] != data)
	{
		butasan_txbackram[ offset ] = data;
	}
}

READ_HANDLER( butasan_bg0backram_r )
{
	return butasan_bg0backram[ offset ];
}

WRITE_HANDLER( butasan_bg0backram_w )
{
	if (butasan_bg0backram[ offset ] != data)
	{
		butasan_bg0backram[ offset ] = data;
	}
}

WRITE_HANDLER( butasan_bg1_status_w )
{
	if (butasan_bg1_status != data)
	{
		butasan_bg1_status = data;

		tilemap_set_enable(bg1_tilemap, butasan_bg1_status & 0x01);	/* Set enable flag */
		tilemap_mark_all_tiles_dirty( bg1_tilemap );				/* Bank changed */
	}
}


/***************************************************************************
  Screen refresh
***************************************************************************/

static void argus_bg0_scroll_handle( void )
{
	int delta;
	int scrollx;
	int dcolumn;

	/* Deficit between previous and current scroll value */
	scrollx = argus_bg0_scrollx[0] | (argus_bg0_scrollx[1] << 8);
	delta = scrollx - prvscrollx;
	prvscrollx = scrollx;

	if ( delta == 0 )
		return;

	if (delta > 0)
	{
		lowbitscroll += delta % 16;
		dcolumn = delta / 16;

		if (lowbitscroll >= 16)
		{
			dcolumn ++;
			lowbitscroll -= 16;
		}

		if (dcolumn != 0)
		{
			int i, j;
			int col, woffs, roffs;

			col = ( (scrollx / 16) + 16 ) % 32;
			woffs = 32 * 2 * col;
			roffs = ( ( (scrollx / 16) + 16 ) * 8 ) % 0x8000;

			if ( dcolumn >= 18 )
				dcolumn = 18;

			for ( i = 0 ; i < dcolumn ; i ++ )
			{
				for ( j = 0 ; j < 4 ; j ++ )
				{
					argus_write_dummy_rams( woffs, roffs );
					woffs += 16;
					roffs += 2;
				}
				woffs -= 128;
				roffs -= 16;
				if (woffs < 0)
					woffs += 0x800;
				if (roffs < 0)
					roffs += 0x8000;
			}
		}
	}
	else
	{
		lowbitscroll += (delta % 16);
		dcolumn = -(delta / 16);

		if (lowbitscroll <= 0)
		{
			dcolumn ++;
			lowbitscroll += 16;
		}

		if (dcolumn != 0)
		{
			int i, j;
			int col, woffs, roffs;

			col = ( (scrollx / 16) + 31 ) % 32;
			woffs = 32 * 2 * col;
			roffs = ( (scrollx / 16) - 1 ) * 8;
			if (roffs < 0)
				roffs += 0x08000;

			if (dcolumn >= 18)
				dcolumn = 18;

			for ( i = 0 ; i < dcolumn ; i ++ )
			{
				for ( j = 0 ; j < 4 ; j ++ )
				{
					argus_write_dummy_rams( woffs, roffs );
					woffs += 16;
					roffs += 2;
				}
				if (woffs >= 0x800)
					woffs -= 0x800;
				if (roffs >= 0x8000)
					roffs -= 0x8000;
			}
		}
	}

	if (!argus_flipscreen)
		tilemap_set_scrollx(bg0_tilemap, 0, scrollx & 0x1ff);
	else
		tilemap_set_scrollx(bg0_tilemap, 0, (scrollx + 256) & 0x1ff);

}

static void argus_draw_sprites(struct mame_bitmap *bitmap, const struct rectangle *cliprect, int priority)
{
	int offs;

	/* Draw the sprites */
	for (offs = 11 ; offs < spriteram_size ; offs += 16)
	{
		if ( !(spriteram[offs+4] == 0 && spriteram[offs] == 0xf0) )
		{
			int sx, sy, tile, flipx, flipy, color, pri;

			sx = spriteram[offs + 1];
			sy = spriteram[offs];

			if (argus_flipscreen)
			{
				sx = 240 - sx;
				sy = 240 - sy;
			}

			if (!argus_flipscreen)
			{
				if (  spriteram[offs+2] & 0x01)  sx -= 256;
				if (!(spriteram[offs+2] & 0x02)) sy -= 256;
			}
			else
			{
				if (  spriteram[offs+2] & 0x01)  sx += 256;
				if (!(spriteram[offs+2] & 0x02)) sy += 256;
			}

			tile	 = spriteram[offs+3] + ((spriteram[offs+2] & 0xc0) << 2);
			flipx	 = spriteram[offs+2] & 0x10;
			flipy	 = spriteram[offs+2] & 0x20;
			color	 = spriteram[offs+4] & 0x07;
			pri      = (spriteram[offs+4] & 0x08) >> 3;

			if (argus_flipscreen)
			{
				flipx ^= 0x10;
				flipy ^= 0x20;
			}

			if (priority != pri)
				drawgfx(bitmap,Machine->gfx[0],
							tile,
							color,
							flipx, flipy,
							sx, sy,
							cliprect,
							TRANSPARENCY_PEN, 15
				);
		}
	}
}

static void valtric_draw_sprites(struct mame_bitmap *bitmap, const struct rectangle *cliprect)
{
	int offs;

	/* Draw the sprites */
	for (offs = 11 ; offs < spriteram_size ; offs += 16)
	{
		if ( !(spriteram[offs+4] == 0 && spriteram[offs] == 0xf0) )
		{
			int sx, sy, tile, flipx, flipy, color;

			sx = spriteram[offs + 1];
			sy = spriteram[offs];

			if (argus_flipscreen)
			{
				sx = 240 - sx;
				sy = 240 - sy;
			}

			if (!argus_flipscreen)
			{
				if (  spriteram[offs+2] & 0x01)  sx -= 256;
				if (!(spriteram[offs+2] & 0x02)) sy -= 256;
			}
			else
			{
				if (  spriteram[offs+2] & 0x01)  sx += 256;
				if (!(spriteram[offs+2] & 0x02)) sy += 256;
			}

			tile	 = spriteram[offs+3] + ((spriteram[offs+2] & 0xc0) << 2);
			flipx	 = spriteram[offs+2] & 0x10;
			flipy	 = spriteram[offs+2] & 0x20;
			color	 = spriteram[offs+4] & 0x0f;

			if (argus_flipscreen)
			{
				flipx ^= 0x10;
				flipy ^= 0x20;
			}

			drawgfx(bitmap,Machine->gfx[0],
						tile,
						color,
						flipx, flipy,
						sx, sy,
						cliprect,
						TRANSPARENCY_PEN, 15);
		}
	}
}

void butasan_draw_sprites(struct mame_bitmap *bitmap, const struct rectangle *cliprect)
{
	int offs;

	/* Draw the sprites */
	for (offs = 8 ; offs < spriteram_size ; offs += 16)
	{
		int sx, sy, tile, flipx, flipy, color;

		sx = spriteram[offs + 2];
		sy = 240 - spriteram[offs + 4];

		if (spriteram[offs + 3] & 0x01) sx -= 256;
		if (spriteram[offs + 5] & 0x01) sy += 256;

		tile	 = spriteram[offs + 6] + ((spriteram[offs + 7] & 0x0f) << 8);
		flipx	 = spriteram[offs + 0] & 0x01;
		flipy	 = spriteram[offs + 0] & 0x04;
		color	 = spriteram[offs + 1] & 0x0f;

		if (!argus_flipscreen)
		{
			if ( (offs >= 0x100 && offs < 0x300) || (offs >= 0x400 && offs < 0x580) )
			{
				drawgfx(bitmap,Machine->gfx[0],
							tile,
							color,
							flipx, flipy,
							sx, sy,
							cliprect,
							TRANSPARENCY_PEN, 7);
			}
			else if ( (offs >= 0x000 && offs < 0x100) || (offs >= 0x300 && offs < 0x400) )
			{
				int i;

				for ( i = 0 ; i <= 1 ; i ++ )
				{
					int td;

					td = (flipx) ? (1 - i) : i;

					drawgfx(bitmap,Machine->gfx[0],
								tile + td,
								color,
								flipx, flipy,
								sx + i * 16, sy,
								cliprect,
								TRANSPARENCY_PEN, 7);
				}
			}
			else if ( offs >= 0x580 && offs < 0x620 )
			{
				int i, j;

				for ( i = 0 ; i <= 1 ; i ++ )
				{
					for ( j = 0 ; j <= 1 ; j ++ )
					{
						int td;

						if ( !flipy )
							td = (flipx) ? (i * 2) + 1 - j : i * 2 + j;
						else
							td = (flipx) ? ( (1 - i) * 2 ) + 1 - j : (1 - i) * 2 + j;

						drawgfx(bitmap,Machine->gfx[0],
									tile + td,
									color,
									flipx, flipy,
									sx + j * 16, sy - i * 16,
									cliprect,
									TRANSPARENCY_PEN, 7);
					}
				}
			}
			else if ( offs >= 0x620 && offs < 0x680 )
			{
				int i, j;

				for ( i = 0 ; i <= 3 ; i ++ )
				{
					for ( j = 0 ; j <= 3 ; j ++ )
					{
						int td;

						if ( !flipy )
							td = (flipx) ? (i * 4) + 3 - j : i * 4 + j;
						else
							td = (flipx) ? ( (3 - i) * 4 ) + 3 - j : (3 - i) * 4 + j;

						drawgfx(bitmap,Machine->gfx[0],
									tile + td,
									color,
									flipx, flipy,
									sx + j * 16, sy - i * 16,									
									cliprect,
									TRANSPARENCY_PEN, 7);
					}
				}
			}
		}
		else
		{
			sx = 240 - sx;
			sy = 240 - sy;
			flipx ^= 0x01;
			flipy ^= 0x04;

			if ( (offs >= 0x100 && offs < 0x300) || (offs >= 0x400 && offs < 0x580) )
			{
				drawgfx(bitmap,Machine->gfx[0],
							tile,
							color,
							flipx, flipy,
							sx, sy,
							cliprect,
							TRANSPARENCY_PEN, 7);
			}
			else if ( (offs >= 0x000 && offs < 0x100) || (offs >= 0x300 && offs < 0x400) )
			{
				int i;

				for ( i = 0 ; i <= 1 ; i ++ )
				{
					int td;

					td = (flipx) ? i : (1 - i);

					drawgfx(bitmap,Machine->gfx[0],
								tile + td,
								color,
								flipx, flipy,
								sx - i * 16, sy,
								cliprect,
								TRANSPARENCY_PEN, 7);
				}
			}
			else if ( offs >= 0x580 && offs < 0x620 )
			{
				int i, j;

				for ( i = 0 ; i <= 1 ; i ++ )
				{
					for ( j = 0 ; j <= 1 ; j ++ )
					{
						int td;

						if ( !flipy )
							td = (flipx) ? (1 - i) * 2 + j : ( (1 - i) * 2 ) + 1 - j;
						else
							td = (flipx) ? i * 2 + j : (i * 2) + 1 - j;

						drawgfx(bitmap,Machine->gfx[0],
									tile + td,
									color,
									flipx, flipy,
									sx - j * 16, sy + i * 16,
									cliprect,
									TRANSPARENCY_PEN, 7);
					}
				}
			}
			else if ( offs >= 0x620 && offs < 0x680 )
			{
				int i, j;

				for ( i = 0 ; i <= 3 ; i ++ )
				{
					for ( j = 0 ; j <= 3 ; j ++ )
					{
						int td;

						if ( !flipy )
							td = (flipx) ? (3 - i) * 4 + j : ( (3 - i) * 4 ) + 3 - j;
						else
							td = (flipx) ? i * 4 + j : (i * 4) + 3 - j;

						drawgfx(bitmap,Machine->gfx[0],
									tile + td,
									color,
									flipx, flipy,
									sx - j * 16, sy + i * 16,
									cliprect,
									TRANSPARENCY_PEN, 7);
					}
				}
			}
		}
	}
}


#ifdef MAME_DEBUG
static void butasan_log_vram(void)
{
	int offs;

	if ( keyboard_pressed(KEYCODE_M) )
	{
		int i;
		log_cb(RETRO_LOG_DEBUG, LOGPRE "\nSprite RAM\n");
		log_cb(RETRO_LOG_DEBUG, LOGPRE "---------------------------------------\n");
		log_cb(RETRO_LOG_DEBUG, LOGPRE "       +0 +1 +2 +3 +4 +5 +6 +7  +8 +9 +a +b +c +d +e +f\n");
		for (offs = 0 ; offs < spriteram_size ; offs += 16)
		{
			for (i = 0 ; i < 16 ; i ++)
			{
				if (i == 0)
				{
					log_cb(RETRO_LOG_DEBUG, LOGPRE "%04x : ", offs + 0xf000);
					log_cb(RETRO_LOG_DEBUG, LOGPRE "%02x ", spriteram[offs]);
				}
				else if (i == 7)
					log_cb(RETRO_LOG_DEBUG, LOGPRE "%02x  ", spriteram[offs + 7]);
				else if (i == 15)
					log_cb(RETRO_LOG_DEBUG, LOGPRE "%02x\n", spriteram[offs + 15]);
				else
					log_cb(RETRO_LOG_DEBUG, LOGPRE "%02x ", spriteram[offs + i]);
			}
		}
		log_cb(RETRO_LOG_DEBUG, LOGPRE "\nColor RAM\n");
		log_cb(RETRO_LOG_DEBUG, LOGPRE "---------------------------------------\n");
		log_cb(RETRO_LOG_DEBUG, LOGPRE "       +0 +1 +2 +3 +4 +5 +6 +7  +8 +9 +a +b +c +d +e +f\n");
		for (offs = 0 ; offs < 0xbf0 ; offs += 16)
		{
			for (i = 0 ; i < 16 ; i ++)
			{
				if (i == 0)
				{
					log_cb(RETRO_LOG_DEBUG, LOGPRE "%04x : ", offs + 0xc400);
					log_cb(RETRO_LOG_DEBUG, LOGPRE "%02x ", argus_paletteram[offs]);
				}
				else if (i == 7)
					log_cb(RETRO_LOG_DEBUG, LOGPRE "%02x  ", argus_paletteram[offs + 7]);
				else if (i == 15)
					log_cb(RETRO_LOG_DEBUG, LOGPRE "%02x\n", argus_paletteram[offs + 15]);
				else
					log_cb(RETRO_LOG_DEBUG, LOGPRE "%02x ", argus_paletteram[offs + i]);
			}
		}
	}
}
#endif

VIDEO_UPDATE( argus )
{
	/* scroll BG0 and render tile at proper position */
	argus_bg0_scroll_handle();

	fillbitmap(bitmap, Machine->pens[0], cliprect);

	tilemap_draw(bitmap, cliprect, bg0_tilemap, 0, 0);
	argus_draw_sprites(bitmap, cliprect, 0);
	tilemap_draw(bitmap, cliprect, bg1_tilemap, 0, 0);
	argus_draw_sprites(bitmap, cliprect, 1);
	tilemap_draw(bitmap, cliprect, tx_tilemap,  0, 0);
}

VIDEO_UPDATE( valtric )
{
	fillbitmap(bitmap, Machine->pens[0], cliprect);

	tilemap_draw(bitmap, cliprect, bg1_tilemap, 0, 0);
	valtric_draw_sprites(bitmap, cliprect);
	tilemap_draw(bitmap, cliprect, tx_tilemap,  0, 0);
}

VIDEO_UPDATE( butasan )
{
	fillbitmap(bitmap, Machine->pens[0], cliprect);

	tilemap_draw(bitmap, cliprect, bg1_tilemap, 0, 0);
	tilemap_draw(bitmap, cliprect, bg0_tilemap, 0, 0);
	butasan_draw_sprites(bitmap, cliprect);
	tilemap_draw(bitmap, cliprect, tx_tilemap,  0, 0);

#ifdef MAME_DEBUG
	butasan_log_vram();
#endif
}
