#include "driver.h"
#include "vidhrdw/generic.h"
#include "tilemap.h"

unsigned char *skykid_textram, *skykid_videoram;

static struct tilemap *background;
static int priority;
static int flipscreen;


/***************************************************************************

	Convert the color PROMs into a more useable format.

	The palette PROMs are connected to the RGB output this way:

	bit 3	-- 220 ohm resistor  -- RED/GREEN/BLUE
			-- 470 ohm resistor  -- RED/GREEN/BLUE
			-- 1  kohm resistor  -- RED/GREEN/BLUE
	bit 0	-- 2.2kohm resistor  -- RED/GREEN/BLUE

***************************************************************************/

PALETTE_INIT( skykid )
{
	int i;
	int bit0,bit1,bit2,bit3,r,g,b;
	int totcolors = Machine->drv->total_colors;

	for (i = 0; i < totcolors; i++)
	{
		/* red component */
		bit0 = (color_prom[totcolors*0] >> 0) & 0x01;
		bit1 = (color_prom[totcolors*0] >> 1) & 0x01;
		bit2 = (color_prom[totcolors*0] >> 2) & 0x01;
		bit3 = (color_prom[totcolors*0] >> 3) & 0x01;
		r = 0x0e*bit0 + 0x1f*bit1 + 0x43*bit2 + 0x8f*bit3;

		/* green component */
		bit0 = (color_prom[totcolors*1] >> 0) & 0x01;
		bit1 = (color_prom[totcolors*1] >> 1) & 0x01;
		bit2 = (color_prom[totcolors*1] >> 2) & 0x01;
		bit3 = (color_prom[totcolors*1] >> 3) & 0x01;
		g = 0x0e*bit0 + 0x1f*bit1 + 0x43*bit2 + 0x8f*bit3;

		/* blue component */
		bit0 = (color_prom[totcolors*2] >> 0) & 0x01;
		bit1 = (color_prom[totcolors*2] >> 1) & 0x01;
		bit2 = (color_prom[totcolors*2] >> 2) & 0x01;
		bit3 = (color_prom[totcolors*2] >> 3) & 0x01;
		b = 0x0e*bit0 + 0x1f*bit1 + 0x43*bit2 + 0x8f*bit3;

		palette_set_color(i,r,g,b);
		color_prom++;
	}

	/* text palette */
	for (i = 0; i < 64*4; i++)
		*(colortable++) = i;

	color_prom += 2*totcolors;
	/* color_prom now points to the beginning of the lookup table */

	/* tiles lookup table */
	for (i = 0; i < 128*4; i++)
		*(colortable++) = *(color_prom++);

	/* sprites lookup table */
	for (i = 0;i < 64*8;i++)
		*(colortable++) = *(color_prom++);

}

/***************************************************************************

	Callbacks for the TileMap code

***************************************************************************/

static void get_tile_info_bg(int tile_index)
{
	unsigned char code = skykid_videoram[tile_index];
	unsigned char attr = skykid_videoram[tile_index+0x800];

	SET_TILE_INFO(
			1,
			code + 256*(attr & 0x01),
			((attr & 0x7e) >> 1) | ((attr & 0x01) << 6),
			0)
}

/***************************************************************************

	Start the video hardware emulation.

***************************************************************************/

VIDEO_START( skykid )
{
	background = tilemap_create(get_tile_info_bg,tilemap_scan_rows,TILEMAP_OPAQUE,8,8,64,32);

	if (!background)
		return 1;

	{
		unsigned char *RAM = memory_region(REGION_CPU1);

		spriteram	= &RAM[0x4f80];
		spriteram_2	= &RAM[0x4f80+0x0800];
		spriteram_3	= &RAM[0x4f80+0x0800+0x0800];
		spriteram_size = 0x80;

		return 0;
	}
}

/***************************************************************************

	Memory handlers

***************************************************************************/

READ_HANDLER( skykid_videoram_r )
{
	return skykid_videoram[offset];
}

WRITE_HANDLER( skykid_videoram_w )
{
	if (skykid_videoram[offset] != data){
		skykid_videoram[offset] = data;
		tilemap_mark_tile_dirty(background,offset & 0x7ff);
	}
}

WRITE_HANDLER( skykid_scroll_x_w )
{
	if (flipscreen)
		tilemap_set_scrollx(background, 0, (189 - (offset ^ 1)) & 0x1ff);
	else
		tilemap_set_scrollx(background, 0, ((offset) + 35) & 0x1ff);
}

WRITE_HANDLER( skykid_scroll_y_w )
{
	if (flipscreen)
		tilemap_set_scrolly(background, 0, (261 - offset) & 0xff);
	else
		tilemap_set_scrolly(background, 0, (offset + 27) & 0xff);
}

WRITE_HANDLER( skykid_flipscreen_w )
{
	priority = data;
	flipscreen = offset;
	tilemap_set_flip(background,flipscreen ? (TILEMAP_FLIPY | TILEMAP_FLIPX) : 0);
}

/***************************************************************************

	Display Refresh

***************************************************************************/

static void skykid_draw_sprites(struct mame_bitmap *bitmap,const struct rectangle *cliprect)
{
	int offs;

	for (offs = 0; offs < spriteram_size; offs += 2){
		int number = spriteram[offs] | ((spriteram_3[offs] & 0x80) << 1);
		int color = (spriteram[offs+1] & 0x3f);
		int sx = (spriteram_2[offs+1]) + 0x100*(spriteram_3[offs+1] & 1) - 72;
		int sy = 256 - spriteram_2[offs] - 57;
		int flipy = spriteram_3[offs] & 0x02;
		int flipx = spriteram_3[offs] & 0x01;
		int width, height;

		if (flipscreen){
				flipx = !flipx;
				flipy = !flipy;
		}

		if (number >= 128*3) continue;

		switch (spriteram_3[offs] & 0x0c){
			case 0x0c:	/* 2x both ways */
				width = height = 2; number &= (~3); break;
			case 0x08:	/* 2x vertical */
				width = 1; height = 2; number &= (~2); break;
			case 0x04:	/* 2x horizontal */
				width = 2; height = 1; number &= (~1); sy += 16; break;
			default:	/* normal sprite */
				width = height = 1; sy += 16; break;
		}

		{
			static int x_offset[2] = { 0x00, 0x01 };
			static int y_offset[2] = { 0x00, 0x02 };
			int x,y, ex, ey;

			for( y=0; y < height; y++ ){
				for( x=0; x < width; x++ ){
					ex = flipx ? (width-1-x) : x;
					ey = flipy ? (height-1-y) : y;

					drawgfx(bitmap,Machine->gfx[2+(number >> 7)],
						(number)+x_offset[ex]+y_offset[ey],
						color,
						flipx, flipy,
						sx+x*16,sy+y*16,
						cliprect,
						TRANSPARENCY_COLOR,255);
				}
			}
		}
	}
}

VIDEO_UPDATE( skykid )
{
	int offs;

	tilemap_draw(bitmap,cliprect,background,0,0);
	if ((priority & 0xf0) != 0x50)
		skykid_draw_sprites(bitmap,cliprect);

	for (offs = 0x400 - 1; offs > 0; offs--){
		{
			int mx,my,sx,sy;

            mx = offs % 32;
			my = offs / 32;

			if (my < 2)	{
				if (mx < 2 || mx >= 30) continue; /* not visible */
				sx = my + 34;
				sy = mx - 2;
			}
			else if (my >= 30){
				if (mx < 2 || mx >= 30) continue; /* not visible */
				sx = my - 30;
				sy = mx - 2;
			}
			else{
				sx = mx + 2;
				sy = my - 2;
			}
			if (flipscreen){
				sx = 35 - sx;
				sy = 27 - sy;
			}

			drawgfx(bitmap,Machine->gfx[0],	skykid_textram[offs] + (flipscreen << 8),
					skykid_textram[offs+0x400] & 0x3f,
					0,0,sx*8,sy*8,
					cliprect,TRANSPARENCY_PEN,0);
        }
	}
	if ((priority & 0xf0) == 0x50)
		skykid_draw_sprites(bitmap,cliprect);
}
