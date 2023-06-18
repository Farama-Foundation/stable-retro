#include "driver.h"
#include "vidhrdw/generic.h"

static struct tilemap *bg_layer,*fg_layer,*tx_layer;
unsigned char *dynduke_back_data,*dynduke_fore_data,*dynduke_scroll_ram,*dynduke_control_ram;

static int flipscreen,back_bankbase,fore_bankbase;
static int back_enable,fore_enable,sprite_enable,txt_enable;

/******************************************************************************/

WRITE_HANDLER( dynduke_paletteram_w )
{
	int r,g,b;
	int color;

	paletteram[offset]=data;
	color=paletteram[offset&0xffe]|(paletteram[offset|1]<<8);

	r = (color >> 0) & 0x0f;
	g = (color >> 4) & 0x0f;
	b = (color >> 8) & 0x0f;

	r = (r << 4) | r;
	g = (g << 4) | g;
	b = (b << 4) | b;

	palette_set_color(offset/2,r,g,b);

}

READ_HANDLER( dynduke_background_r )
{
	return dynduke_back_data[offset];
}

READ_HANDLER( dynduke_foreground_r )
{
	return dynduke_fore_data[offset];
}

WRITE_HANDLER( dynduke_background_w )
{
	dynduke_back_data[offset]=data;
	tilemap_mark_tile_dirty(bg_layer,offset/2);
}

WRITE_HANDLER( dynduke_foreground_w )
{
	dynduke_fore_data[offset]=data;
	tilemap_mark_tile_dirty(fg_layer,offset/2);
}

WRITE_HANDLER( dynduke_text_w )
{
	videoram[offset]=data;
	tilemap_mark_tile_dirty(tx_layer,offset/2);
}

static void get_bg_tile_info(int tile_index)
{
	int tile=dynduke_back_data[2*tile_index]+(dynduke_back_data[2*tile_index+1]<<8);
	int color=tile >> 12;

	tile=tile&0xfff;

	SET_TILE_INFO(
			1,
			tile+back_bankbase,
			color,
			0)
}

static void get_fg_tile_info(int tile_index)
{
	int tile=dynduke_fore_data[2*tile_index]+(dynduke_fore_data[2*tile_index+1]<<8);
	int color=tile >> 12;

	tile=tile&0xfff;

	SET_TILE_INFO(
			2,
			tile+fore_bankbase,
			color,
			0)
}

static void get_tx_tile_info(int tile_index)
{
	int tile=videoram[2*tile_index]+((videoram[2*tile_index+1]&0xc0)<<2);
	int color=videoram[2*tile_index+1]&0xf;

	SET_TILE_INFO(
			0,
			tile,
			color,
			0)
}

VIDEO_START( dynduke )
{
	bg_layer = tilemap_create(get_bg_tile_info,tilemap_scan_cols,TILEMAP_SPLIT,      16,16,32,32);
	fg_layer = tilemap_create(get_fg_tile_info,tilemap_scan_cols,TILEMAP_TRANSPARENT,16,16,32,32);
	tx_layer = tilemap_create(get_tx_tile_info,tilemap_scan_rows,TILEMAP_TRANSPARENT, 8, 8,32,32);


	tilemap_set_transparent_pen(fg_layer,15);
	tilemap_set_transparent_pen(tx_layer,15);

	return 0;
}

WRITE_HANDLER( dynduke_gfxbank_w )
{
	static int old_back,old_fore;

	if (data&0x01) back_bankbase=0x1000; else back_bankbase=0;
	if (data&0x10) fore_bankbase=0x1000; else fore_bankbase=0;

	if (back_bankbase!=old_back)
		tilemap_mark_all_tiles_dirty(bg_layer);
	if (fore_bankbase!=old_fore)
		tilemap_mark_all_tiles_dirty(fg_layer);

	old_back=back_bankbase;
	old_fore=fore_bankbase;
}

WRITE_HANDLER( dynduke_control_w )
{

	dynduke_control_ram[offset]=data;

	if (offset!=6) return;

	/* bit 0x80 toggles, maybe sprite buffering?
	 bit 0x40 is flipscreen
	 bit 0x20 not used?
	 bit 0x10 not used?
	 bit 0x08 is set on the title screen (sprite disable?)
	 bit 0x04 unused? txt disable?
	 bit 0x02 is used on the map screen (fore disable?)
	 bit 0x01 set when inserting coin.. bg disable? */

	if (data&0x1) back_enable = 0; else back_enable = 1;
	if (data&0x2) fore_enable=0; else fore_enable=1;
	if (data&0x4) txt_enable = 0; else txt_enable = 1;
	if (data&0x8) sprite_enable=0; else sprite_enable=1;

	flipscreen=data&0x40;
	tilemap_set_flip(ALL_TILEMAPS,flipscreen ? (TILEMAP_FLIPY | TILEMAP_FLIPX) : 0);
}

static void draw_sprites(struct mame_bitmap *bitmap,const struct rectangle *cliprect,int pri)
{
	int offs,fx,fy,x,y,color,sprite;

	if (!sprite_enable) return;

	for (offs = 0x1000-8;offs >= 0;offs -= 8)
	{
		/* Don't draw empty sprite table entries */
		if (buffered_spriteram[offs+7]!=0xf) continue;
		if (buffered_spriteram[offs+0]==0xf0f) continue;
		if (((buffered_spriteram[offs+5]>>5)&3)!=pri) continue;

		fx= buffered_spriteram[offs+1]&0x20;
		fy= buffered_spriteram[offs+1]&0x40;
		y = buffered_spriteram[offs+0];
		x = buffered_spriteram[offs+4];

		if (buffered_spriteram[offs+5]&1) x=0-(0x100-x);

		color = buffered_spriteram[offs+1]&0x1f;
		sprite = buffered_spriteram[offs+2]+(buffered_spriteram[offs+3]<<8);
		sprite &= 0x3fff;

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

static void draw_background(struct mame_bitmap *bitmap, const struct rectangle *cliprect, int pri )
{
	/* The transparency / palette handling on the background layer is very strange */
	const struct mame_bitmap *bm = tilemap_get_pixmap(bg_layer);
	int scrolly, scrollx;
	int x,y;

	/* if we're disabled, don't draw */
	if (!back_enable)
	{
		fillbitmap(bitmap,get_black_pen(),cliprect);
		return;
	}

	scrolly = ((dynduke_scroll_ram[0x02]&0x30)<<4)+((dynduke_scroll_ram[0x04]&0x7f)<<1)+((dynduke_scroll_ram[0x04]&0x80)>>7);
	scrollx = ((dynduke_scroll_ram[0x12]&0x30)<<4)+((dynduke_scroll_ram[0x14]&0x7f)<<1)+((dynduke_scroll_ram[0x14]&0x80)>>7);
	
	for (y=0;y<256;y++)
	{
		int realy = (y + scrolly) & 0x1ff;
		UINT16 *src = (UINT16*)bm->line[realy];
	    UINT16 *dst = (UINT16*)bitmap->line[y];


		for (x=0;x<256;x++)
		{
			int realx = (x + scrollx) & 0x1ff;
			UINT16 srcdat = src[realx];

			/* 0x01 - data bits
               0x02
               0x04
               0x08
               0x10 - extra colour bit? (first boss)
               0x20 - priority over sprites
               the old driver also had 'bg_palbase' but I don't see what it's for?
            */

			if ((srcdat & 0x20) == pri)
			{
				if (srcdat & 0x10) srcdat += 0x400;
				/*if (srcdat & 0x10) srcdat += mame_rand(machine)&0x1f; */

				srcdat = (srcdat & 0x000f) | ((srcdat & 0xffc0) >> 2);
				dst[x] = srcdat;
			}


		}
	}
}



VIDEO_UPDATE( dynduke )
{
	/* Setup the tilemaps */
	tilemap_set_scrolly( fg_layer,0, ((dynduke_scroll_ram[0x22]&0x30)<<4)+((dynduke_scroll_ram[0x24]&0x7f)<<1)+((dynduke_scroll_ram[0x24]&0x80)>>7) );
	tilemap_set_scrollx( fg_layer,0, ((dynduke_scroll_ram[0x32]&0x30)<<4)+((dynduke_scroll_ram[0x34]&0x7f)<<1)+((dynduke_scroll_ram[0x34]&0x80)>>7) );
	tilemap_set_enable( fg_layer,fore_enable);
    tilemap_set_enable( tx_layer,txt_enable);
	

	draw_background(bitmap, cliprect,0x00);
	draw_sprites(bitmap,cliprect,0); /* Untested: does anything use it? Could be behind background */
	draw_sprites(bitmap,cliprect,1);
	draw_background(bitmap, cliprect,0x20);
	draw_sprites(bitmap,cliprect,2);
	tilemap_draw(bitmap,cliprect,fg_layer,0,0);
	draw_sprites(bitmap,cliprect,3);
	tilemap_draw(bitmap,cliprect,tx_layer,0,0);
}
