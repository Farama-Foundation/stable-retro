/***************************************************************************

   Crude Buster Video emulation - Bryan McPhail, mish@tendril.co.uk

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

data16_t *twocrude_pf1_data,*twocrude_pf2_data,*twocrude_pf3_data,*twocrude_pf4_data;

static struct tilemap *pf1_tilemap,*pf2_tilemap,*pf3_tilemap,*pf4_tilemap;
static int twocrude_pri,flipscreen;

static data16_t twocrude_control_0[8];
static data16_t twocrude_control_1[8];

data16_t *twocrude_pf1_rowscroll,*twocrude_pf2_rowscroll;
data16_t *twocrude_pf3_rowscroll,*twocrude_pf4_rowscroll;

/* Function for all 16x16 1024 by 512 layers */
static UINT32 back_scan(UINT32 col,UINT32 row,UINT32 num_cols,UINT32 num_rows)
{
	/* logical (col,row) -> memory offset */
	return (col & 0x1f) + ((row & 0x1f) << 5) + ((col & 0x20) << 5);
}

static INLINE void get_back_tile_info(int tile_index,int gfx_bank,data16_t *gfx_base)
{
	int tile,color;

	tile=gfx_base[tile_index];
	color=tile >> 12;
	tile=tile&0xfff;

	SET_TILE_INFO(
			gfx_bank,
			tile,
			color,
			0)
}

static void get_back_tile_info2(int tile_index) { get_back_tile_info(tile_index,1,twocrude_pf2_data); }
static void get_back_tile_info3(int tile_index) { get_back_tile_info(tile_index,2,twocrude_pf3_data); }
static void get_back_tile_info4(int tile_index) { get_back_tile_info(tile_index,3,twocrude_pf4_data); }


/* 8x8 top layer */
static void get_fore_tile_info(int tile_index)
{
	int tile=twocrude_pf1_data[tile_index];
	int color=tile >> 12;

	tile=tile&0xfff;

	SET_TILE_INFO(
			0,
			tile,
			color,
			0)
}

/******************************************************************************/

VIDEO_START( twocrude )
{
	pf2_tilemap = tilemap_create(get_back_tile_info2,back_scan,        TILEMAP_OPAQUE,16,16,64,32);
	pf3_tilemap = tilemap_create(get_back_tile_info3,back_scan,        TILEMAP_TRANSPARENT,16,16,64,32);
	pf4_tilemap = tilemap_create(get_back_tile_info4,back_scan,        TILEMAP_TRANSPARENT,16,16,64,32);
	pf1_tilemap = tilemap_create(get_fore_tile_info, tilemap_scan_rows,TILEMAP_TRANSPARENT,8,8,64,32);

	if (!pf1_tilemap || !pf2_tilemap || !pf3_tilemap || !pf4_tilemap)
		return 1;

	tilemap_set_transparent_pen(pf1_tilemap,0);
	tilemap_set_transparent_pen(pf3_tilemap,0);
	tilemap_set_transparent_pen(pf4_tilemap,0);

	return 0;
}

/******************************************************************************/

static void update_24bitcol(int offset)
{
	UINT8 r,g,b; /* The highest palette value seems to be 0x8e */

	r = (UINT8)((float)((paletteram16[offset] >> 0) & 0xff)*1.75);
	g = (UINT8)((float)((paletteram16[offset] >> 8) & 0xff)*1.75);
	b = (UINT8)((float)((paletteram16_2[offset] >> 0) & 0xff)*1.75);

	palette_set_color(offset,r,g,b);
}

WRITE16_HANDLER( twocrude_palette_24bit_rg_w )
{
	COMBINE_DATA(&paletteram16[offset]);
	update_24bitcol(offset);
}

WRITE16_HANDLER( twocrude_palette_24bit_b_w )
{
	COMBINE_DATA(&paletteram16_2[offset]);
	update_24bitcol(offset);
}

/******************************************************************************/

void twocrude_pri_w(int pri)
{
	twocrude_pri=pri;
}

WRITE16_HANDLER( twocrude_pf1_data_w )
{
	data16_t oldword=twocrude_pf1_data[offset];
	COMBINE_DATA(&twocrude_pf1_data[offset]);
	if (oldword!=twocrude_pf1_data[offset])
		tilemap_mark_tile_dirty(pf1_tilemap,offset);
}

WRITE16_HANDLER( twocrude_pf2_data_w )
{
	data16_t oldword=twocrude_pf2_data[offset];
	COMBINE_DATA(&twocrude_pf2_data[offset]);
	if (oldword!=twocrude_pf2_data[offset])
		tilemap_mark_tile_dirty(pf2_tilemap,offset);
}

WRITE16_HANDLER( twocrude_pf3_data_w )
{
	data16_t oldword=twocrude_pf3_data[offset];
	COMBINE_DATA(&twocrude_pf3_data[offset]);
	if (oldword!=twocrude_pf3_data[offset])
		tilemap_mark_tile_dirty(pf3_tilemap,offset);
}

WRITE16_HANDLER( twocrude_pf4_data_w )
{
	data16_t oldword=twocrude_pf4_data[offset];
	COMBINE_DATA(&twocrude_pf4_data[offset]);
	if (oldword!=twocrude_pf4_data[offset])
		tilemap_mark_tile_dirty(pf4_tilemap,offset);
}

WRITE16_HANDLER( twocrude_control_0_w )
{
	COMBINE_DATA(&twocrude_control_0[offset]);
}

WRITE16_HANDLER( twocrude_control_1_w )
{
	COMBINE_DATA(&twocrude_control_1[offset]);
}

/******************************************************************************/

static void twocrude_drawsprites(struct mame_bitmap *bitmap, const struct rectangle *cliprect, int pri)
{
	int offs;

	for (offs = 0;offs < 0x400;offs += 4)
	{
		int x,y,sprite,colour,multi,fx,fy,inc,flash,mult;

		sprite = buffered_spriteram16[offs+1] & 0x7fff;
		if (!sprite) continue;

		y = buffered_spriteram16[offs];
		x = buffered_spriteram16[offs+2];

		if ((y&0x8000) && pri==1) continue;
		if (!(y&0x8000) && pri==0) continue;

		colour = (x >> 9) &0xf;
		if (x&0x2000) colour+=64;

		flash=y&0x1000;
		if (flash && (cpu_getcurrentframe() & 1)) continue;

		fx = y & 0x2000;
		fy = y & 0x4000;
		multi = (1 << ((y & 0x0600) >> 9)) - 1;	/* 1x, 2x, 4x, 8x height */

		x = x & 0x01ff;
		y = y & 0x01ff;
		if (x >= 256) x -= 512;
		if (y >= 256) y -= 512;
		x = 240 - x;
		y = 240 - y;

		if (x>256) continue; /* Speedup */

		sprite &= ~multi;
		if (fy)
			inc = -1;
		else
		{
			sprite += multi;
			inc = 1;
		}

		if (flipscreen) {
			y=240-y;
			x=240-x;
			if (fx) fx=0; else fx=1;
			if (fy) fy=0; else fy=1;
			mult=16;
		}
		else mult=-16;

		while (multi >= 0)
		{
			drawgfx(bitmap,Machine->gfx[4],
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

VIDEO_UPDATE( twocrude )
{
	int offs;
	int pf23_control,pf14_control;

	/* Update flipscreen */
	if (twocrude_control_1[0]&0x80)
		flipscreen=0;
	else
		flipscreen=1;
	tilemap_set_flip(ALL_TILEMAPS,flipscreen ? (TILEMAP_FLIPY | TILEMAP_FLIPX) : 0);

	pf23_control=twocrude_control_0[6];
	pf14_control=twocrude_control_1[6];

	/* Background - Rowscroll enable */
	if (pf23_control&0x4000) {
		int scrollx=twocrude_control_0[3],rows;
		tilemap_set_scroll_cols(pf2_tilemap,1);
		tilemap_set_scrolly( pf2_tilemap,0, twocrude_control_0[4] );

		/* Several different rowscroll styles! */
		switch ((twocrude_control_0[5]>>11)&7) {
			case 0: rows=512; break;/* Every line of 512 height bitmap */
			case 1: rows=256; break;
			case 2: rows=128; break;
			case 3: rows=64; break;
			case 4: rows=32; break;
			case 5: rows=16; break;
			case 6: rows=8; break;
			case 7: rows=4; break;
			default: rows=1; break;
		}

		tilemap_set_scroll_rows(pf2_tilemap,rows);
		for (offs = 0;offs < rows;offs++)
			tilemap_set_scrollx( pf2_tilemap,offs, scrollx + twocrude_pf2_rowscroll[offs] );
	}
	else {
		tilemap_set_scroll_rows(pf2_tilemap,1);
		tilemap_set_scroll_cols(pf2_tilemap,1);
		tilemap_set_scrollx( pf2_tilemap,0, twocrude_control_0[3] );
		tilemap_set_scrolly( pf2_tilemap,0, twocrude_control_0[4] );
	}

	/* Playfield 3 */
	if (pf23_control&0x40) { /* Rowscroll */
		int scrollx=twocrude_control_0[1],rows;
		tilemap_set_scroll_cols(pf3_tilemap,1);
		tilemap_set_scrolly( pf3_tilemap,0, twocrude_control_0[2] );

		/* Several different rowscroll styles! */
		switch ((twocrude_control_0[5]>>3)&7) {
			case 0: rows=512; break;/* Every line of 512 height bitmap */
			case 1: rows=256; break;
			case 2: rows=128; break;
			case 3: rows=64; break;
			case 4: rows=32; break;
			case 5: rows=16; break;
			case 6: rows=8; break;
			case 7: rows=4; break;
			default: rows=1; break;
		}

		tilemap_set_scroll_rows(pf3_tilemap,rows);
		for (offs = 0;offs < rows;offs++)
			tilemap_set_scrollx( pf3_tilemap,offs, scrollx + twocrude_pf3_rowscroll[offs] );
	}
	else if (pf23_control&0x20) { /* Colscroll */
		int scrolly=twocrude_control_0[2],cols;
		tilemap_set_scroll_rows(pf3_tilemap,1);
		tilemap_set_scrollx( pf3_tilemap,0, twocrude_control_0[1] );

		/* Several different colscroll styles! */
		switch ((twocrude_control_0[5]>>0)&7) {
			case 0: cols=64; break;
			case 1: cols=32; break;
			case 2: cols=16; break;
			case 3: cols=8; break;
			case 4: cols=4; break;
			case 5: cols=2; break;
			case 6: cols=1; break;
			case 7: cols=1; break;
			default: cols=1; break;
		}

		tilemap_set_scroll_cols(pf3_tilemap,cols);
		for (offs = 0;offs < cols;offs++)
			tilemap_set_scrolly( pf3_tilemap,offs,scrolly + twocrude_pf3_rowscroll[offs+0x200] );
	}
	else {
		tilemap_set_scroll_rows(pf3_tilemap,1);
		tilemap_set_scroll_cols(pf3_tilemap,1);
		tilemap_set_scrollx( pf3_tilemap,0, twocrude_control_0[1] );
		tilemap_set_scrolly( pf3_tilemap,0, twocrude_control_0[2] );
	}

	/* Playfield 4 - Rowscroll enable */
	if (pf14_control&0x4000) {
		int scrollx=twocrude_control_1[3],rows;
		tilemap_set_scroll_cols(pf4_tilemap,1);
		tilemap_set_scrolly( pf4_tilemap,0, twocrude_control_1[4] );

		/* Several different rowscroll styles! */
		switch ((twocrude_control_1[5]>>11)&7) {
			case 0: rows=512; break;/* Every line of 512 height bitmap */
			case 1: rows=256; break;
			case 2: rows=128; break;
			case 3: rows=64; break;
			case 4: rows=32; break;
			case 5: rows=16; break;
			case 6: rows=8; break;
			case 7: rows=4; break;
			default: rows=1; break;
		}

		tilemap_set_scroll_rows(pf4_tilemap,rows);
		for (offs = 0;offs < rows;offs++)
			tilemap_set_scrollx( pf4_tilemap,offs, scrollx + twocrude_pf4_rowscroll[offs] );
	}
	else {
		tilemap_set_scroll_rows(pf4_tilemap,1);
		tilemap_set_scroll_cols(pf4_tilemap,1);
		tilemap_set_scrollx( pf4_tilemap,0, twocrude_control_1[3] );
		tilemap_set_scrolly( pf4_tilemap,0, twocrude_control_1[4] );
	}

	/* Playfield 1 */
	if (pf14_control&0x40) { /* Rowscroll */
		int scrollx=twocrude_control_1[1],rows;
		tilemap_set_scroll_cols(pf1_tilemap,1);
		tilemap_set_scrolly( pf1_tilemap,0, twocrude_control_1[2] );

		/* Several different rowscroll styles! */
		switch ((twocrude_control_1[5]>>3)&7) {
			case 0: rows=256; break;/* Every line of 256 height bitmap */
			case 1: rows=128; break;
			case 2: rows=64; break;
			case 3: rows=32; break;
			case 4: rows=16; break;
			case 5: rows=8; break;
			case 6: rows=4; break;
			case 7: rows=2; break;
			default: rows=1; break;
		}

		tilemap_set_scroll_rows(pf1_tilemap,rows);
		for (offs = 0;offs < rows;offs++)
			tilemap_set_scrollx( pf1_tilemap,offs, scrollx + twocrude_pf1_rowscroll[offs] );
	}
	else {
		tilemap_set_scroll_rows(pf1_tilemap,1);
		tilemap_set_scroll_cols(pf1_tilemap,1);
		tilemap_set_scrollx( pf1_tilemap,0, twocrude_control_1[1] );
		tilemap_set_scrolly( pf1_tilemap,0, twocrude_control_1[2] );
	}

	/* Draw playfields & sprites */
	tilemap_draw(bitmap,cliprect,pf2_tilemap,0,0);
	twocrude_drawsprites(bitmap,cliprect,0);

	if (twocrude_pri) {
		tilemap_draw(bitmap,cliprect,pf4_tilemap,0,0);
		tilemap_draw(bitmap,cliprect,pf3_tilemap,0,0);
	}
	else {
		tilemap_draw(bitmap,cliprect,pf3_tilemap,0,0);
		tilemap_draw(bitmap,cliprect,pf4_tilemap,0,0);
	}

	twocrude_drawsprites(bitmap,cliprect,1);
	tilemap_draw(bitmap,cliprect,pf1_tilemap,0,0);
}
