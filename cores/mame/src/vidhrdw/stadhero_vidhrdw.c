/***************************************************************************

  stadhero video emulation - Bryan McPhail, mish@tendril.co.uk

*********************************************************************

	MXC-06 chip to produce sprites, see dec0.c
	BAC-06 chip for background?

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

data16_t *stadhero_pf1_data,*stadhero_pf2_data;
static struct tilemap *pf1_tilemap,*pf2_tilemap;
static int flipscreen;

static data16_t stadhero_pf2_control_0[8];
static data16_t stadhero_pf2_control_1[8];

/******************************************************************************/

static void stadhero_drawsprites(struct mame_bitmap *bitmap,const struct rectangle *cliprect,int pri_mask,int pri_val)
{
	int offs;

	for (offs = 0;offs < 0x400;offs += 4)
	{
		int x,y,sprite,colour,multi,fx,fy,inc,flash,mult;

		y = spriteram16[offs];
		if ((y&0x8000) == 0) continue;

		x = spriteram16[offs+2];
		colour = x >> 12;
		if ((colour & pri_mask) != pri_val) continue;

		flash=x&0x800;
		if (flash && (cpu_getcurrentframe() & 1)) continue;

		fx = y & 0x2000;
		fy = y & 0x4000;
		multi = (1 << ((y & 0x1800) >> 11)) - 1;	/* 1x, 2x, 4x, 8x height */
											/* multi = 0   1   3   7 */

		sprite = spriteram16[offs+1] & 0x0fff;

		x = x & 0x01ff;
		y = y & 0x01ff;
		if (x >= 256) x -= 512;
		if (y >= 256) y -= 512;
		x = 240 - x;
		y = 240 - y;

		sprite &= ~multi;
		if (fy)
			inc = -1;
		else
		{
			sprite += multi;
			inc = 1;
		}

		if (flip_screen) {
			y=240-y;
			x=240-x;
			if (fx) fx=0; else fx=1;
			if (fy) fy=0; else fy=1;
			mult=16;
		}
		else mult=-16;

		while (multi >= 0)
		{
			drawgfx(bitmap,Machine->gfx[2],
					sprite - multi * inc,
					colour,
					fx,fy,
					x,y + mult * multi,
					cliprect,TRANSPARENCY_PEN,0);
			multi--;
		}
	}
}

/******************************************************************************/

VIDEO_UPDATE( stadhero )
{
	flipscreen=stadhero_pf2_control_0[0]&0x80;
	tilemap_set_flip(ALL_TILEMAPS,flipscreen ? (TILEMAP_FLIPY | TILEMAP_FLIPX) : 0);
	tilemap_set_scrollx( pf2_tilemap,0, stadhero_pf2_control_1[0] );
	tilemap_set_scrolly( pf2_tilemap,0, stadhero_pf2_control_1[1] );

	tilemap_draw(bitmap,cliprect,pf2_tilemap,0,0);
	stadhero_drawsprites(bitmap,cliprect,0x00,0x00);
	tilemap_draw(bitmap,cliprect,pf1_tilemap,0,0);
}

/******************************************************************************/

WRITE16_HANDLER( stadhero_pf1_data_w )
{
	data16_t oldword=stadhero_pf1_data[offset];
	COMBINE_DATA(&stadhero_pf1_data[offset]);
	if (oldword!=stadhero_pf1_data[offset])
		tilemap_mark_tile_dirty(pf1_tilemap,offset);
}

WRITE16_HANDLER( stadhero_pf2_data_w )
{
	data16_t oldword=stadhero_pf2_data[offset];
	COMBINE_DATA(&stadhero_pf2_data[offset]);
	if (oldword!=stadhero_pf2_data[offset])
		tilemap_mark_tile_dirty(pf2_tilemap,offset);
}

WRITE16_HANDLER( stadhero_pf2_control_0_w )
{
	COMBINE_DATA(&stadhero_pf2_control_0[offset]);
}

WRITE16_HANDLER( stadhero_pf2_control_1_w )
{
	COMBINE_DATA(&stadhero_pf2_control_1[offset]);
}

/******************************************************************************/

static UINT32 stadhero_scan(UINT32 col,UINT32 row,UINT32 num_cols,UINT32 num_rows)
{
	/* logical (col,row) -> memory offset */
	return (col & 0xf) + ((row & 0xf) << 4) + ((row & 0x30) << 4) + ((col & 0x30) << 6);
}

static void get_pf2_tile_info(int tile_index)
{
	int tile,color;

	tile=stadhero_pf2_data[tile_index];
	color=tile >> 12;
	tile=tile&0xfff;

	SET_TILE_INFO(
			1,
			tile,
			color,
			0)
}

static void get_pf1_tile_info(int tile_index)
{
	int tile=stadhero_pf1_data[tile_index];
	int color=tile >> 12;

	tile=tile&0xfff;
	SET_TILE_INFO(
			0,
			tile,
			color,
			0)
}

VIDEO_START( stadhero )
{
	pf1_tilemap =     tilemap_create(get_pf1_tile_info,tilemap_scan_rows,TILEMAP_TRANSPARENT, 8, 8,32,32);
	pf2_tilemap =     tilemap_create(get_pf2_tile_info,stadhero_scan,TILEMAP_OPAQUE,     16,16,64,64);

	if (!pf1_tilemap || !pf2_tilemap)
		return 1;

	tilemap_set_transparent_pen(pf1_tilemap,0);

	return 0;
}

/******************************************************************************/
