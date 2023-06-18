#include "vidhrdw/generic.h"

static struct tilemap *pf3_layer,*pf2_layer,*pf1_layer,*text_layer;
static int deadangle_tilebank, deadangle_oldtilebank;
data8_t *deadang_video_data,*deadang_scroll_ram;

/******************************************************************************/

WRITE_HANDLER( deadang_foreground_w )
{
	deadang_video_data[offset]=data;
	tilemap_mark_tile_dirty( pf1_layer, offset/2 );
}

WRITE_HANDLER( deadang_text_w )
{
	videoram[offset]=data;
	tilemap_mark_tile_dirty( text_layer, offset/2 );
}

WRITE_HANDLER( deadang_bank_w )
{
	deadangle_tilebank = data&1;
	if (deadangle_tilebank!=deadangle_oldtilebank) {
		deadangle_oldtilebank = deadangle_tilebank;
		tilemap_mark_all_tiles_dirty (pf1_layer);
	}
}

/******************************************************************************/

static UINT32 bg_scan(UINT32 col,UINT32 row,UINT32 num_cols,UINT32 num_rows)
{
	return (col&0xf) | ((row&0xf)<<4) | ((col&0x70)<<4) | ((row&0xf0)<<7);
}

static void get_pf3_tile_info( int tile_index )
{
	const data8_t *bgMap = memory_region(REGION_GFX6);
	int code=(bgMap[tile_index*2]<<8) | bgMap[tile_index*2+1];
	SET_TILE_INFO(4,code&0x7ff,code>>12,0);
}

static void get_pf2_tile_info( int tile_index )
{
	const data8_t *bgMap = memory_region(REGION_GFX7);
	int code=(bgMap[tile_index*2]<<8) | bgMap[tile_index*2+1];
	SET_TILE_INFO(3,code&0x7ff,code>>12,0);
}

static void get_pf1_tile_info( int tile_index )
{
	int offs=tile_index*2;
	int tile=deadang_video_data[offs]+(deadang_video_data[offs+1]<<8);
	int color=tile >> 12;
	tile=tile&0xfff;

	SET_TILE_INFO(2,tile+deadangle_tilebank*0x1000,color,0)
}

static void get_text_tile_info( int tile_index )
{
	int offs=tile_index*2;
	int tile=videoram[offs]+((videoram[offs+1]&0xc0)<<2);
	int color=videoram[offs+1]&0xf;

	SET_TILE_INFO(0,tile,color,0)
}

VIDEO_START( deadang )
{
	pf3_layer = tilemap_create(get_pf3_tile_info,bg_scan,          TILEMAP_OPAQUE,     16,16,128,256);
	pf2_layer = tilemap_create(get_pf2_tile_info,bg_scan,          TILEMAP_TRANSPARENT,16,16,128,256);
	pf1_layer = tilemap_create(get_pf1_tile_info,tilemap_scan_cols,TILEMAP_TRANSPARENT,16,16, 32, 32);
	text_layer = tilemap_create(get_text_tile_info,tilemap_scan_rows,TILEMAP_TRANSPARENT, 8, 8, 32, 32);

	if (!text_layer || !pf1_layer || !pf2_layer || !pf3_layer)
		return 1;

	tilemap_set_transparent_pen(pf2_layer, 15);
	tilemap_set_transparent_pen(pf1_layer, 15);
	tilemap_set_transparent_pen(text_layer, 15);

	return 0;
}

static void draw_sprites(struct mame_bitmap *bitmap)
{
	int offs,fx,fy,x,y,color,sprite,pri;

	for (offs = 0; offs<0x800; offs+=8)
	{
		/* Don't draw empty sprite table entries */
		if (spriteram[offs+7]!=0xf) continue;

		switch (spriteram[offs+5]&0xc0) {
		default:
		case 0xc0: pri=0; break; /* Unknown */
		case 0x80: pri=0; break; /* Over all playfields */
		case 0x40: pri=0xf0; break; /* Under top playfield */
		case 0x00: pri=0xf0|0xcc; break; /* Under middle playfield */
		}

		fx= spriteram[offs+1]&0x20;
		fy= spriteram[offs+1]&0x40;
		y = spriteram[offs+0];
		x = spriteram[offs+4];
		if (fy) fy=0; else fy=1;
		if (spriteram[offs+5]&1) x=0-(0xff-x);

		color = (spriteram[offs+3]>>4)&0xf;
		sprite = (spriteram[offs+2]+(spriteram[offs+3]<<8))&0xfff;

		if (flip_screen) {
			x=240-x;
			y=240-y;
			if (fx) fx=0; else fx=1;
			if (fy) fy=0; else fy=1;
		}

		pdrawgfx(bitmap,Machine->gfx[1],
				sprite,
				color,fx,fy,x,y,
				&Machine->visible_area,TRANSPARENCY_PEN,15,pri);
	}
}

VIDEO_UPDATE( deadang )
{
	/* Setup the tilemaps */
	tilemap_set_scrolly( pf3_layer,0, ((deadang_scroll_ram[0x02]&0xf0)<<4)+((deadang_scroll_ram[0x04]&0x7f)<<1)+((deadang_scroll_ram[0x04]&0x80)>>7) );
	tilemap_set_scrollx( pf3_layer,0, ((deadang_scroll_ram[0x12]&0xf0)<<4)+((deadang_scroll_ram[0x14]&0x7f)<<1)+((deadang_scroll_ram[0x14]&0x80)>>7) );
	tilemap_set_scrolly( pf1_layer,0, ((deadang_scroll_ram[0x22]&0x10)<<4)+((deadang_scroll_ram[0x24]&0x7f)<<1)+((deadang_scroll_ram[0x24]&0x80)>>7) );
	tilemap_set_scrollx( pf1_layer,0, ((deadang_scroll_ram[0x32]&0x10)<<4)+((deadang_scroll_ram[0x34]&0x7f)<<1)+((deadang_scroll_ram[0x34]&0x80)>>7) );
	tilemap_set_scrolly( pf2_layer,0, ((deadang_scroll_ram[0x42]&0xf0)<<4)+((deadang_scroll_ram[0x44]&0x7f)<<1)+((deadang_scroll_ram[0x44]&0x80)>>7) );
	tilemap_set_scrollx( pf2_layer,0, ((deadang_scroll_ram[0x52]&0xf0)<<4)+((deadang_scroll_ram[0x54]&0x7f)<<1)+((deadang_scroll_ram[0x54]&0x80)>>7) );

	/* Control byte:
		0x01: Background playfield disable
		0x02: Middle playfield disable
		0x04: Top playfield disable
		0x08: ?  Toggles at start of game
		0x10: Sprite disable
		0x20: Unused?
		0x40: Flipscreen
		0x80: Always set?
	*/
	tilemap_set_enable(pf3_layer,!(deadang_scroll_ram[0x68]&1));
	tilemap_set_enable(pf1_layer,!(deadang_scroll_ram[0x68]&2));
	tilemap_set_enable(pf2_layer,!(deadang_scroll_ram[0x68]&4));
	flip_screen_set( deadang_scroll_ram[0x68]&0x40 );

	fillbitmap(bitmap,get_black_pen(),cliprect);
	fillbitmap(priority_bitmap,0,cliprect);
	tilemap_draw(bitmap,cliprect,pf3_layer,0,1);
	tilemap_draw(bitmap,cliprect,pf1_layer,0,2);
	tilemap_draw(bitmap,cliprect,pf2_layer,0,4);
	if (!(deadang_scroll_ram[0x68]&0x10)) draw_sprites(bitmap);
	tilemap_draw(bitmap,cliprect,text_layer,0,0);
}
