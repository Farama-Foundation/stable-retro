/***************************************************************************

	Emulation by Bryan McPhail, mish@tendril.co.uk

	Like NeoGeo sprite scale Y line selection is from an external rom.

	For 16 high sprites scale data starts at 0x3800 (9 scale levels)
	For 32 high sprites scale data starts at 0x7000 (17 scale levels)
	For 64 high sprites scale data starts at 0xa000 (33 scale levels)
	For 128 pixel high sprites scale data starts 0xc000 (65 scale levels)

	0xe000 and up - possibly X scale data?  unconfirmed

	Sprites are also double buffered, and this seems to be performed
	by having two complete sprite chips that are toggled per frame, rather
	than just ram.  Beast Busters has 4 sprite chips as it has two sprite
	banks.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

static struct tilemap *fix_tilemap,*pf1_tilemap,*pf2_tilemap;
static const data8_t *scale_table_ptr;
static data8_t scale_line_count;

data16_t *bbuster_pf1_data,*bbuster_pf2_data,*bbuster_pf1_scroll_data,*bbuster_pf2_scroll_data;

/******************************************************************************/

static void get_bbuster_tile_info( int tile_index )
{
	data16_t tile=videoram16[tile_index];
	SET_TILE_INFO(0,tile&0xfff,tile>>12,0)
}

static void get_pf1_tile_info( int tile_index )
{
	data16_t tile=bbuster_pf1_data[tile_index];
	SET_TILE_INFO(3,tile&0xfff,tile>>12,0)
}

static void get_pf2_tile_info( int tile_index )
{
	data16_t tile=bbuster_pf2_data[tile_index];
	SET_TILE_INFO(4,tile&0xfff,tile>>12,0)
}

WRITE16_HANDLER( bbuster_video_w )
{
	COMBINE_DATA(&videoram16[offset]);
	tilemap_mark_tile_dirty(fix_tilemap,offset);
}

WRITE16_HANDLER( bbuster_pf1_w )
{
	COMBINE_DATA(&bbuster_pf1_data[offset]);
	tilemap_mark_tile_dirty(pf1_tilemap,offset);
}

WRITE16_HANDLER( bbuster_pf2_w )
{
	COMBINE_DATA(&bbuster_pf2_data[offset]);
	tilemap_mark_tile_dirty(pf2_tilemap,offset);
}

/******************************************************************************/

VIDEO_START( bbuster )
{
	fix_tilemap = tilemap_create(get_bbuster_tile_info,tilemap_scan_rows,TILEMAP_TRANSPARENT,8,8,32,32);
	pf1_tilemap = tilemap_create(get_pf1_tile_info,tilemap_scan_cols,TILEMAP_TRANSPARENT,16,16,128,32);
	pf2_tilemap = tilemap_create(get_pf2_tile_info,tilemap_scan_cols,TILEMAP_TRANSPARENT,16,16,128,32);

	if (!fix_tilemap || !pf1_tilemap || !pf2_tilemap)
		return 1;

	tilemap_set_transparent_pen(pf1_tilemap, 15);
	tilemap_set_transparent_pen(fix_tilemap, 15);

	return 0;
}

VIDEO_START( mechatt )
{
	fix_tilemap = tilemap_create(get_bbuster_tile_info,tilemap_scan_rows,TILEMAP_TRANSPARENT,8,8,32,32);
	pf1_tilemap = tilemap_create(get_pf1_tile_info,tilemap_scan_cols,TILEMAP_TRANSPARENT,16,16,256,32);
	pf2_tilemap = tilemap_create(get_pf2_tile_info,tilemap_scan_cols,TILEMAP_TRANSPARENT,16,16,256,32);

	if (!fix_tilemap || !pf1_tilemap || !pf2_tilemap)
		return 1;

	tilemap_set_transparent_pen(pf1_tilemap, 15);
	tilemap_set_transparent_pen(fix_tilemap, 15);

	return 0;
}

/******************************************************************************/

#define ADJUST_4x4 \
		if ((dx&0x10) && (dy&0x10)) code+=3;	\
		else if (dy&0x10) code+=2;				\
		else if (dx&0x10) code+=1

#define ADJUST_8x8 \
		if ((dx&0x20) && (dy&0x20)) code+=12;	\
		else if (dy&0x20) code+=8;				\
		else if (dx&0x20) code+=4

#define ADJUST_16x16 \
		if ((dx&0x40) && (dy&0x40)) code+=48;	\
		else if (dy&0x40) code+=32;				\
		else if (dx&0x40) code+=16

static INLINE const data8_t *get_source_ptr(unsigned int sprite, int dx, int dy, int block, const struct GfxElement *gfx )
{
	int source_base,code=0;

	/* Get a tile index from the x,y position in the block */
	switch (block)
	{
	case 0: /* 16 x 16 sprite */
		break;

	case 1: /* 32 x 32 block
				0 1
		        2 3
			*/
		ADJUST_4x4;
		break;

	case 2: /* 64 by 64 block
				0  1	4  5
				2  3	6  7

				8  9	12 13
				10 11	14 15
			*/
		ADJUST_4x4;
		ADJUST_8x8;
		break;

	case 3: /* 128 by 128 block */
		ADJUST_4x4;
		ADJUST_8x8;
		ADJUST_16x16;
		break;
	}

	source_base=((sprite+code) % gfx->total_elements) * 16;
	return gfx->gfxdata + ((source_base+(dy%16)) * gfx->line_modulo);
}

static void bbusters_draw_block(struct mame_bitmap *dest,int x,int y,int size,int flipx,int flipy,unsigned int sprite,int color,int bank,int block)
{
	const struct GfxElement *gfx=Machine->gfx[bank];
	const pen_t *pal = gfx->colortable + gfx->color_granularity * (color % gfx->total_colors);
	unsigned int xinc=(scale_line_count * 0x10000 ) / size;
	data8_t pixel;
	int x_index;
	int dy=y;
	int sx,ex=scale_line_count;

	while (scale_line_count) {

		if (dy>=16 && dy<240) {
			UINT16 *destline = (UINT16 *)dest->line[dy];
			data8_t srcline=*scale_table_ptr;
			const data8_t *srcptr=0;

			if (!flipy)
				srcline=size-srcline-1;

			if (flipx)
				x_index=(ex-1)*0x10000;
			else
				x_index=0;

			for (sx=0; sx<size; sx++) {
				if ((sx%16)==0)
					srcptr=get_source_ptr(sprite,sx,srcline,block,gfx);

				pixel=*srcptr++;
				if (pixel!=15)
					destline[(x+(x_index>>16)) & 0x1ff]=pal[pixel];

				if (flipx)
					x_index-=xinc;
				else
					x_index+=xinc;
			}
		}

		dy++;
		scale_table_ptr--;
		scale_line_count--;
	}
}

static void draw_sprites(struct mame_bitmap *bitmap, const data16_t *source, int bank, int pass)
{
	const data8_t *scale_table=memory_region(REGION_USER1);
	int offs;

	for (offs = 0;offs <0x800 ;offs += 4) {
		int x,sprite,colour,fx,fy,scale;
		INT16 y;
		int block;

	    sprite=source[offs+1];
	    colour=source[offs+0];

	    if ((colour==0xf7 || colour==0xffff || colour == 0x43f9) && (sprite==0x3fff || sprite==0xffff || sprite==0x0001))
			continue; /* sprite 1, color 0x43f9 is the dead sprite in the top-right of the screen in Mechanized Attack's High Score table. */

	    y=source[offs+3];
		/* if (y>254) continue;  Speedup */
	    x=source[offs+2];
		if (x&0x200) x=-(0x100-(x&0xff));
		if (y > 320 || y < -256) y &= 0x1ff; /* fix for bbusters ending & "Zing!" attract-mode fullscreen zombie & Helicopter on the 3rd rotation of the attractmode sequence */
		/* if (x>256) continue;  Speedup */

		/*
			Source[0]:
				0xf000:	Colour
				0x0800: FX
				0x0400: FY?
				0x0300: Block control
				0x0080: ?
				0x007f: scale

			Scale varies according to block size.
			Block type 0: 0x70 = no scale, 0x7f == half size - 16 pixel sprite
			Block type 1: 0x60 = no scale, 0x6f == half size - 32 pixel sprite
			Block type 2: 0x40 = no scale, 0x5f == half size - 64 pixel sprite
			Block type 3: 0x00 = no scale, 0x3f == half size - 128 pixel sprite

		*/
		colour=colour>>12;
		block=(source[offs+0]>>8)&0x3;
		fy=source[offs+0]&0x400;
		fx=source[offs+0]&0x800;
	    sprite=sprite&0x3fff;

		/* Palettes 0xc-0xf confirmed to be behind tilemap on Beast Busters */
		if (pass==1 && (colour&0xc)!=0xc)
			continue;
		
		if (pass==0 && (colour&0xc)==0xc)
			continue;

		switch ((source[offs+0]>>8)&0x3) {
			case 0:
				scale=source[offs+0]&0x7;
				scale_table_ptr=scale_table+0x387f+(0x80*scale);
				scale_line_count=0x10-scale;
				bbusters_draw_block(bitmap,x,y,16,fx,fy,sprite,colour,bank,block);
				break;
			case 1: /* 2 x 2 */
				scale=source[offs+0]&0xf;
				scale_table_ptr=scale_table+0x707f+(0x80*scale);
				scale_line_count=0x20-scale;
				bbusters_draw_block(bitmap,x,y,32,fx,fy,sprite,colour,bank,block);
				break;
			case 2: /* 64 by 64 block (2 x 2) x 2 */
				scale=source[offs+0]&0x1f;
				scale_table_ptr=scale_table+0xa07f+(0x80*scale);
				scale_line_count=0x40-scale;
				bbusters_draw_block(bitmap,x,y,64,fx,fy,sprite,colour,bank,block);
				break;
			case 3: /* 2 x 2 x 2 x 2 */
				scale=source[offs+0]&0x3f;
				scale_table_ptr=scale_table+0xc07f+(0x80*scale);
				scale_line_count=0x80-scale;
				bbusters_draw_block(bitmap,x,y,128,fx,fy,sprite,colour,bank,block);
				break;
		}
	}
}

/******************************************************************************/

VIDEO_UPDATE( bbuster )
{
	tilemap_set_scrollx( pf1_tilemap,0, bbuster_pf1_scroll_data[0] );
	tilemap_set_scrolly( pf1_tilemap,0, bbuster_pf1_scroll_data[1] );
	tilemap_set_scrollx( pf2_tilemap,0, bbuster_pf2_scroll_data[0] );
	tilemap_set_scrolly( pf2_tilemap,0, bbuster_pf2_scroll_data[1] );

	tilemap_draw(bitmap,cliprect,pf2_tilemap,0,0);
	draw_sprites(bitmap,buffered_spriteram16_2,2,1);
	tilemap_draw(bitmap,cliprect,pf1_tilemap,0,0);
	draw_sprites(bitmap,buffered_spriteram16_2,2,0);
	draw_sprites(bitmap,buffered_spriteram16,1,-1);
	tilemap_draw(bitmap,cliprect,fix_tilemap,0,0);

	draw_crosshair(1, bitmap,readinputport(6),readinputport(5),cliprect);
	draw_crosshair(2, bitmap,readinputport(8),readinputport(7),cliprect);
	draw_crosshair(3, bitmap,readinputport(10),readinputport(9),cliprect);
}

VIDEO_UPDATE( mechatt )
{
	tilemap_set_scrollx( pf1_tilemap,0, bbuster_pf1_scroll_data[0] );
	tilemap_set_scrolly( pf1_tilemap,0, bbuster_pf1_scroll_data[1] );
	tilemap_set_scrollx( pf2_tilemap,0, bbuster_pf2_scroll_data[0] );
	tilemap_set_scrolly( pf2_tilemap,0, bbuster_pf2_scroll_data[1] );

	tilemap_draw(bitmap,cliprect,pf2_tilemap,0,0);
	tilemap_draw(bitmap,cliprect,pf1_tilemap,0,0);
	draw_sprites(bitmap,buffered_spriteram16,1,-1);
	tilemap_draw(bitmap,cliprect,fix_tilemap,0,0);

	draw_crosshair(1, bitmap,readinputport(2),readinputport(3),cliprect);
	draw_crosshair(2, bitmap,readinputport(4),readinputport(5),cliprect);
}
