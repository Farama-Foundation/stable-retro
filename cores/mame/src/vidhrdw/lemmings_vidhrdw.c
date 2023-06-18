/***************************************************************************

	Lemmings video emulation - Bryan McPhail, mish@tendril.co.uk

*********************************************************************

	There are two sets of sprites, the combination of custom chips 52 & 71.
	There is a background pixel layer implemented with discrete logic
	rather than a custom chip and a foreground VRAM tilemap layer that the
	game mostly uses as a pixel layer (the vram format is arranged as
	sequential pixels, rather than sequential characters).

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

data16_t *lemmings_pixel_0_data,*lemmings_pixel_1_data,*lemmings_vram_data,*lemmings_control_data;
static data16_t *sprite_triple_buffer_0,*sprite_triple_buffer_1;
static data8_t *vram_buffer, *vram_dirty;
struct mame_bitmap *bitmap0;
static struct tilemap *vram_tilemap;

/******************************************************************************/

static void lemmings_drawsprites(struct mame_bitmap *bitmap, data16_t *spritedata, int gfxbank, data16_t pri)
{
	int offs;

	for (offs = 0;offs < 0x400;offs += 4)
	{
		int x,y,sprite,colour,multi,fx,fy,inc,flash,mult;

		sprite = spritedata[offs+1] & 0x3fff;

		if ((spritedata[offs+2]&0x2000)!=pri)
			continue;

		y = spritedata[offs];
		flash=y&0x1000;
		if (flash && (cpu_getcurrentframe() & 1)) continue;

		x = spritedata[offs+2];
		colour = (x >>9) & 0xf;

		fx = y & 0x2000;
		fy = y & 0x4000;
		multi = (1 << ((y & 0x0600) >> 9)) - 1;	/* 1x, 2x, 4x, 8x height */

		x = x & 0x01ff;
		y = y & 0x01ff;
		if (x >= 320) x -= 512;
		if (y >= 256) y -= 512;

		if (x>320 || x<-16) continue;

		sprite &= ~multi;
		if (fy)
			inc = 1;
		else
		{
			sprite += multi;
			inc = -1;
		}
		mult=-16;

		while (multi >= 0)
		{
			drawgfx(bitmap,Machine->gfx[gfxbank],
					sprite - multi * inc,
					colour,
					fx,fy,
					x,y + mult * multi,
					&Machine->visible_area,TRANSPARENCY_PEN,0);

			multi--;
		}
	}
}

/******************************************************************************/

static void get_tile_info(int tile_index)
{
	data16_t tile=lemmings_vram_data[tile_index];

	SET_TILE_INFO(
			2,
			tile&0x7ff,
			(tile>>12)&0xf,
			0)
}

VIDEO_START( lemmings )
{
	bitmap0 = bitmap_alloc(2048,256);
	vram_tilemap = tilemap_create(get_tile_info,tilemap_scan_cols,TILEMAP_TRANSPARENT,8,8,64,32);

	vram_buffer = (data8_t*)malloc(2048*64); /* 64 bytes per VRAM character */
	vram_dirty = (data8_t*)malloc(2048);
	sprite_triple_buffer_0 = (data16_t*)malloc(0x800);
	sprite_triple_buffer_1 = (data16_t*)malloc(0x800);

	if (!bitmap0 || !vram_tilemap || !vram_buffer || !vram_dirty || !sprite_triple_buffer_0 || !sprite_triple_buffer_1)
		return 1;

	tilemap_set_transparent_pen(vram_tilemap,0);
	fillbitmap(bitmap0,0x100,0);

	return 0;
}

VIDEO_STOP( lemmings )
{
	if (bitmap0)
		bitmap_free(bitmap0);
	if (vram_buffer)
		free(vram_buffer);
	if (vram_dirty)
		free(vram_dirty);
	if (sprite_triple_buffer_0)
		free(sprite_triple_buffer_0);
	if (sprite_triple_buffer_1)
		free(sprite_triple_buffer_1);
}

VIDEO_EOF( lemmings )
{
	memcpy(sprite_triple_buffer_0,buffered_spriteram16,0x800);
	memcpy(sprite_triple_buffer_1,buffered_spriteram16_2,0x800);
}

/******************************************************************************/

WRITE16_HANDLER( lemmings_pixel_0_w )
{
	int sx,sy,src,old;

	old=lemmings_pixel_0_data[offset];
	COMBINE_DATA(&lemmings_pixel_0_data[offset]);
	src=lemmings_pixel_0_data[offset];
	if (old==src)
		return;

	sy=(offset<<1)/0x800;
	sx=(offset<<1)&0x7ff;

	if (sx>2047 || sy>255)
		return;

	plot_pixel(bitmap0,sx+0,sy,Machine->pens[((src>>8)&0xf)|0x100]);
	plot_pixel(bitmap0,sx+1,sy,Machine->pens[((src>>0)&0xf)|0x100]);
}

WRITE16_HANDLER( lemmings_pixel_1_w )
{
	int sx,sy,src,old,tile;

	old=lemmings_pixel_1_data[offset];
	COMBINE_DATA(&lemmings_pixel_1_data[offset]);
	src=lemmings_pixel_1_data[offset];
/*	if (old==src)*/
/*		return;*/

	sy=((offset<<1)/0x200);
	sx=((offset<<1)&0x1ff);

	/* Copy pixel to buffer for easier decoding later */
	tile=((sx/8)*32)+(sy/8);
	vram_dirty[tile]=1;
	vram_buffer[(tile*64) + ((sx&7)) + ((sy&7)*8)]=(src>>8)&0xf;

	sx+=1; /* Update both pixels in the word */
	vram_buffer[(tile*64) + ((sx&7)) + ((sy&7)*8)]=(src>>0)&0xf;
}

WRITE16_HANDLER( lemmings_vram_w )
{
	COMBINE_DATA(&lemmings_vram_data[offset]);
	tilemap_mark_tile_dirty(vram_tilemap,offset);
}

VIDEO_UPDATE( lemmings )
{
	int x1=-lemmings_control_data[0],x0=-lemmings_control_data[2],i,y=0;
	struct rectangle rect;
	rect.max_y=cliprect->max_y;
	rect.min_y=cliprect->min_y;

	/* Decode any characters that have changed in vram */
	for (i=0; i<2048; i++) {
		if (vram_dirty[i]) {
			decodechar(Machine->gfx[2],i,vram_buffer,Machine->drv->gfxdecodeinfo[2].gfxlayout);
			tilemap_mark_tile_dirty(vram_tilemap,i);
			vram_dirty[i]=0;
		}
	}

	fillbitmap(bitmap,get_black_pen(),cliprect);
	lemmings_drawsprites(bitmap,sprite_triple_buffer_1,1,0x0000);

	/* Pixel layer can be windowed in hardware (two player mode) */
	if ((lemmings_control_data[6]&2)==0) {
		copyscrollbitmap(bitmap,bitmap0,1,&x1,1,&y,&Machine->visible_area,TRANSPARENCY_PEN,0x100);
	} else {
		rect.max_x=159;
		rect.min_x=0;
		copyscrollbitmap(bitmap,bitmap0,1,&x0,1,&y,&rect,TRANSPARENCY_PEN,0x100);
		rect.max_x=319;
		rect.min_x=160;
		copyscrollbitmap(bitmap,bitmap0,1,&x1,1,&y,&rect,TRANSPARENCY_PEN,0x100);
	}
	lemmings_drawsprites(bitmap,sprite_triple_buffer_0,0,0x0000);
	lemmings_drawsprites(bitmap,sprite_triple_buffer_1,1,0x2000);
	tilemap_draw(bitmap,cliprect,vram_tilemap,0,0);
	lemmings_drawsprites(bitmap,sprite_triple_buffer_0,0,0x2000);
}
