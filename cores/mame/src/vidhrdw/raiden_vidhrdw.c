
#include "driver.h"
#include "vidhrdw/generic.h"

static struct tilemap *bg_layer,*fg_layer,*tx_layer;
unsigned char *raiden_back_data,*raiden_fore_data,*raiden_scroll_ram;

static int flipscreen,ALTERNATE;

/******************************************************************************/

READ_HANDLER( raiden_background_r )
{
	return raiden_back_data[offset];
}

READ_HANDLER( raiden_foreground_r )
{
	return raiden_fore_data[offset];
}

WRITE_HANDLER( raiden_background_w )
{
	raiden_back_data[offset]=data;
	tilemap_mark_tile_dirty( bg_layer,offset/2);
}

WRITE_HANDLER( raiden_foreground_w )
{
	raiden_fore_data[offset]=data;
	tilemap_mark_tile_dirty( fg_layer,offset/2);
}

WRITE_HANDLER( raiden_text_w )
{
	videoram[offset]=data;
	tilemap_mark_tile_dirty( tx_layer,offset/2);
}

WRITE_HANDLER( raidena_text_w )
{
	videoram[offset]=data;
	tilemap_mark_tile_dirty( tx_layer,offset/2);
}

static void get_back_tile_info(int tile_index)
{
	int tile=raiden_back_data[2*tile_index]+(raiden_back_data[2*tile_index+1]<<8);
	int color=tile >> 12;

	tile=tile&0xfff;

	SET_TILE_INFO(
			1,
			tile,
			color,
			0)
}

static void get_fore_tile_info(int tile_index)
{
	int tile=raiden_fore_data[2*tile_index]+(raiden_fore_data[2*tile_index+1]<<8);
	int color=tile >> 12;

	tile=tile&0xfff;

	SET_TILE_INFO(
			2,
			tile,
			color,
			0)
}

static void get_text_tile_info(int tile_index)
{
	int tile=videoram[2*tile_index]+((videoram[2*tile_index+1]&0xc0)<<2);
	int color=videoram[2*tile_index+1]&0xf;

	SET_TILE_INFO(
			0,
			tile,
			color,
			0)
}

VIDEO_START( raiden )
{
	bg_layer = tilemap_create(get_back_tile_info,tilemap_scan_cols,TILEMAP_OPAQUE,     16,16,32,32);
	fg_layer = tilemap_create(get_fore_tile_info,tilemap_scan_cols,TILEMAP_TRANSPARENT,16,16,32,32);
	tx_layer = tilemap_create(get_text_tile_info,tilemap_scan_cols,TILEMAP_TRANSPARENT,8,8,32,32);
	ALTERNATE=0;

	if (!bg_layer || !fg_layer || !tx_layer)
		return 1;

	tilemap_set_transparent_pen(fg_layer,15);
	tilemap_set_transparent_pen(tx_layer,15);

	return 0;
}

VIDEO_START( raidena )
{
	bg_layer = tilemap_create(get_back_tile_info,tilemap_scan_cols,TILEMAP_OPAQUE,     16,16,32,32);
	fg_layer = tilemap_create(get_fore_tile_info,tilemap_scan_cols,TILEMAP_TRANSPARENT,16,16,32,32);
	tx_layer = tilemap_create(get_text_tile_info,tilemap_scan_rows,TILEMAP_TRANSPARENT,8,8,32,32);
	ALTERNATE=1;

	if (!bg_layer || !fg_layer || !tx_layer)
		return 1;

	tilemap_set_transparent_pen(fg_layer,15);
	tilemap_set_transparent_pen(tx_layer,15);

	return 0;
}

WRITE_HANDLER( raiden_control_w )
{
	/* All other bits unknown - could be playfield enables */

	/* Flipscreen */
	if (offset==6) {
		flipscreen=data&0x2;
		tilemap_set_flip(ALL_TILEMAPS,flipscreen ? (TILEMAP_FLIPY | TILEMAP_FLIPX) : 0);
	}
}

static void draw_sprites(struct mame_bitmap *bitmap,const struct rectangle *cliprect,int pri_mask)
{
	int offs,fx,fy,x,y,color,sprite;

	for (offs = 0x1000-8;offs >= 0;offs -= 8)
	{
		/* Don't draw empty sprite table entries */
		if (buffered_spriteram[offs+7]!=0xf) continue;
		if (buffered_spriteram[offs+0]==0xf0f) continue;
		if (!(pri_mask&buffered_spriteram[offs+5])) continue;

		fx= buffered_spriteram[offs+1]&0x20;
		fy= buffered_spriteram[offs+1]&0x40;
		y = buffered_spriteram[offs+0];
		x = buffered_spriteram[offs+4];

		if (buffered_spriteram[offs+5]&1) x=0-(0x100-x);

		color = buffered_spriteram[offs+1]&0xf;
		sprite = buffered_spriteram[offs+2]+(buffered_spriteram[offs+3]<<8);
		sprite &= 0x0fff;

		if (flipscreen) {
			x=240-x;
			y=240-y;
			if (fx) fx=0; else fx=1;
			if (fy) fy=0; else fy=1;
		}

		drawgfx(bitmap,Machine->gfx[3],
				sprite,
				color,fx,fy,x,y,
				cliprect,TRANSPARENCY_PEN,15);
	}
}

VIDEO_UPDATE( raiden )
{
	/* Setup the tilemaps, alternate version has different scroll positions */
	if (!ALTERNATE) {
		tilemap_set_scrollx( bg_layer,0, ((raiden_scroll_ram[1]<<8)+raiden_scroll_ram[0]) );
		tilemap_set_scrolly( bg_layer,0, ((raiden_scroll_ram[3]<<8)+raiden_scroll_ram[2]) );
		tilemap_set_scrollx( fg_layer,0, ((raiden_scroll_ram[5]<<8)+raiden_scroll_ram[4]) );
		tilemap_set_scrolly( fg_layer,0, ((raiden_scroll_ram[7]<<8)+raiden_scroll_ram[6]) );
	}
	else {
		tilemap_set_scrolly( bg_layer,0, ((raiden_scroll_ram[0x02]&0x30)<<4)+((raiden_scroll_ram[0x04]&0x7f)<<1)+((raiden_scroll_ram[0x04]&0x80)>>7) );
		tilemap_set_scrollx( bg_layer,0, ((raiden_scroll_ram[0x12]&0x30)<<4)+((raiden_scroll_ram[0x14]&0x7f)<<1)+((raiden_scroll_ram[0x14]&0x80)>>7) );
		tilemap_set_scrolly( fg_layer,0, ((raiden_scroll_ram[0x22]&0x30)<<4)+((raiden_scroll_ram[0x24]&0x7f)<<1)+((raiden_scroll_ram[0x24]&0x80)>>7) );
		tilemap_set_scrollx( fg_layer,0, ((raiden_scroll_ram[0x32]&0x30)<<4)+((raiden_scroll_ram[0x34]&0x7f)<<1)+((raiden_scroll_ram[0x34]&0x80)>>7) );
	}

	tilemap_draw(bitmap,cliprect,bg_layer,0,0);

	/* Draw sprites underneath foreground */
	draw_sprites(bitmap,cliprect,0x40);
	tilemap_draw(bitmap,cliprect,fg_layer,0,0);

	/* Rest of sprites */
	draw_sprites(bitmap,cliprect,0x80);

	/* Text layer */
	tilemap_draw(bitmap,cliprect,tx_layer,0,0);
}
