#include "driver.h"
#include "vidhrdw/generic.h"
#include "tilemap.h"

unsigned char *baraduke_textram, *baraduke_videoram;

static struct tilemap *tilemap[2];	/* backgrounds */
static int xscroll[2], yscroll[2];	/* scroll registers */
static int flipscreen;

/***************************************************************************

	Convert the color PROMs into a more useable format.

	The palette PROMs are connected to the RGB output this way:

	bit 3	-- 220 ohm resistor  -- RED/GREEN/BLUE
			-- 470 ohm resistor  -- RED/GREEN/BLUE
			-- 1  kohm resistor  -- RED/GREEN/BLUE
	bit 0	-- 2.2kohm resistor  -- RED/GREEN/BLUE

***************************************************************************/

PALETTE_INIT( baraduke )
{
	int i;
	int bit0,bit1,bit2,bit3,r,g,b;

	for (i = 0; i < 2048; i++)
	{
		/* red component */
		bit0 = (color_prom[2048] >> 0) & 0x01;
		bit1 = (color_prom[2048] >> 1) & 0x01;
		bit2 = (color_prom[2048] >> 2) & 0x01;
		bit3 = (color_prom[2048] >> 3) & 0x01;
		r = 0x0e*bit0 + 0x1f*bit1 + 0x43*bit2 + 0x8f*bit3;

		/* green component */
		bit0 = (color_prom[0] >> 0) & 0x01;
		bit1 = (color_prom[0] >> 1) & 0x01;
		bit2 = (color_prom[0] >> 2) & 0x01;
		bit3 = (color_prom[0] >> 3) & 0x01;
		g = 0x0e*bit0 + 0x1f*bit1 + 0x43*bit2 + 0x8f*bit3;

		/* blue component */
		bit0 = (color_prom[0] >> 4) & 0x01;
		bit1 = (color_prom[0] >> 5) & 0x01;
		bit2 = (color_prom[0] >> 6) & 0x01;
		bit3 = (color_prom[0] >> 7) & 0x01;
		b = 0x0e*bit0 + 0x1f*bit1 + 0x43*bit2 + 0x8f*bit3;

		palette_set_color(i,r,g,b);
		color_prom++;
	}
}

/***************************************************************************

	Callbacks for the TileMap code

***************************************************************************/

static void get_tile_info0(int tile_index)
{
	unsigned char attr = baraduke_videoram[2*tile_index + 1];
	unsigned char code = baraduke_videoram[2*tile_index];

	SET_TILE_INFO(
			1 + ((attr & 0x02) >> 1),
			code | ((attr & 0x01) << 8),
			attr,
			0)
}

static void get_tile_info1(int tile_index)
{
	unsigned char attr = baraduke_videoram[0x1000 + 2*tile_index + 1];
	unsigned char code = baraduke_videoram[0x1000 + 2*tile_index];

	SET_TILE_INFO(
			3 + ((attr & 0x02) >> 1),
			code | ((attr & 0x01) << 8),
			attr,
			0)
}

/***************************************************************************

	Start the video hardware emulation.

***************************************************************************/

VIDEO_START( baraduke )
{
	tilemap[0] = tilemap_create(get_tile_info0,tilemap_scan_rows,TILEMAP_TRANSPARENT,8,8,64,32);
	tilemap[1] = tilemap_create(get_tile_info1,tilemap_scan_rows,TILEMAP_TRANSPARENT,8,8,64,32);

	if (!tilemap[0] || !tilemap[1])
		return 1;

	tilemap_set_transparent_pen(tilemap[0],7);
	tilemap_set_transparent_pen(tilemap[1],7);

	return 0;
}

/***************************************************************************

	Memory handlers

***************************************************************************/

READ_HANDLER( baraduke_videoram_r )
{
	return baraduke_videoram[offset];
}

WRITE_HANDLER( baraduke_videoram_w )
{
	if (baraduke_videoram[offset] != data)
	{
		baraduke_videoram[offset] = data;
		tilemap_mark_tile_dirty(tilemap[offset/0x1000],(offset&0xfff)/2);
	}
}

static void scroll_w(int layer,int offset,int data)
{
	int xdisp[2] = { 26, 24 };
	int scrollx, scrolly;

	switch (offset)
	{
		case 0:	/* high scroll x */
			xscroll[layer] = (xscroll[layer] & 0xff) | (data << 8);
			break;
		case 1:	/* low scroll x */
			xscroll[layer] = (xscroll[layer] & 0xff00) | data;
			break;
		case 2:	/* scroll y */
			yscroll[layer] = data;
			break;
	}

	scrollx = xscroll[layer] + xdisp[layer];
	scrolly = yscroll[layer] + 25;
	if (flipscreen)
	{
		scrollx = -scrollx + 227;
		scrolly = -scrolly + 32;
	}

	tilemap_set_scrollx(tilemap[layer], 0, scrollx);
	tilemap_set_scrolly(tilemap[layer], 0, scrolly);
}

WRITE_HANDLER( baraduke_scroll0_w )
{
	scroll_w(0, offset, data);
}
WRITE_HANDLER( baraduke_scroll1_w )
{
	scroll_w(1, offset, data);
}

/***************************************************************************

	Display Refresh

***************************************************************************/

static void draw_sprites(struct mame_bitmap *bitmap, const struct rectangle *cliprect, int priority)
{
	const unsigned char *source = &spriteram[0];
	const unsigned char *finish = &spriteram[0x0800-16];/* the last is NOT a sprite */

	int sprite_xoffs = spriteram[0x07f5] - 256 * (spriteram[0x07f4] & 1) + 16;
	int sprite_yoffs = spriteram[0x07f7] - 256 * (spriteram[0x07f6] & 1);

	while( source<finish )
	{
/*
	source[4]	S-FT ---P
	source[5]	TTTT TTTT
	source[6]   CCCC CCCX
	source[7]	XXXX XXXX
	source[8]	---T -S-F
	source[9]   YYYY YYYY
*/
		{
			unsigned char attrs = source[4];
			unsigned char attr2 = source[8];
			unsigned char color = source[6];
			int sx = source[7] + (color & 0x01)*256; /* need adjust for left clip */
			int sy = -source[9];
			int flipx = attrs & 0x20;
			int flipy = attr2 & 0x01;
			int tall = (attr2 & 0x04) ? 1 : 0;
			int wide = (attrs & 0x80) ? 1 : 0;
			int pri = attrs & 0x01;
			int sprite_number = (source[5] & 0xff)*4;
			int row,col;

			if (pri == priority)
			{
				if ((attrs & 0x10) && !wide) sprite_number += 1;
				if ((attr2 & 0x10) && !tall) sprite_number += 2;
				color = color >> 1;

				if( sx > 512 - 32 ) sx -= 512;

				if( flipx && !wide ) sx -= 16;
				if( !tall ) sy += 16;
				if( !tall && (attr2 & 0x10) && flipy ) sy -= 16;

				sx += sprite_xoffs;
				sy -= sprite_yoffs;

				for( row=0; row<=tall; row++ )
				{
					for( col=0; col<=wide; col++ )
					{
						if (flipscreen)
						{
							drawgfx( bitmap, Machine->gfx[5],
								sprite_number+2*row+col,
								color,
								!flipx,!flipy,
								512-67 - (sx+16*(flipx ? 1-col : col)),
								64-16-209 - (sy+16*(flipy ? 1-row : row)),
								cliprect,
								TRANSPARENCY_PEN, 0xf );
						}
						else
						{
							drawgfx( bitmap, Machine->gfx[5],
								sprite_number+2*row+col,
								color,
								flipx,flipy,
								-87 + (sx+16*(flipx ? 1-col : col)),
								209 + (sy+16*(flipy ? 1-row : row)),
								cliprect,
								TRANSPARENCY_PEN, 0x0f );
						}
					}
				}
			}
		}
		source+=16;
	}
}

VIDEO_UPDATE( baraduke )
{
	int offs;

	/* this is the global sprite Y offset, actually */
	flipscreen = spriteram[0x07f6] & 0x01;
	tilemap_set_flip(ALL_TILEMAPS,flipscreen ? (TILEMAP_FLIPY | TILEMAP_FLIPX) : 0);

	tilemap_draw(bitmap,cliprect,tilemap[1],TILEMAP_IGNORE_TRANSPARENCY,0);
	draw_sprites(bitmap,cliprect,0);
	tilemap_draw(bitmap,cliprect,tilemap[0],0,0);
	draw_sprites(bitmap,cliprect,1);

	for (offs = 0x400 - 1; offs > 0; offs--)
	{
		int mx,my,sx,sy;

        mx = offs % 32;
		my = offs / 32;

		if (my < 2)
		{
			if (mx < 2 || mx >= 30) continue; /* not visible */
			sx = my + 34; sy = mx - 2;
		}
		else if (my >= 30)
		{
			if (mx < 2 || mx >= 30) continue; /* not visible */
			sx = my - 30; sy = mx - 2;
		}
		else
		{
			sx = mx + 2; sy = my - 2;
		}
		if (flipscreen)
		{
				sx = 35 - sx; sy = 27 - sy;
		}

		drawgfx(bitmap,Machine->gfx[0],	baraduke_textram[offs],
				(baraduke_textram[offs+0x400] << 2) & 0x1ff,
				flipscreen,flipscreen,sx*8,sy*8,
				cliprect,TRANSPARENCY_PEN,3);
	}
}

VIDEO_UPDATE( metrocrs )
{
	int offs;

	/* this is the global sprite Y offset, actually */
	flipscreen = spriteram[0x07f6] & 0x01;
	tilemap_set_flip(ALL_TILEMAPS,flipscreen ? (TILEMAP_FLIPY | TILEMAP_FLIPX) : 0);

	tilemap_draw(bitmap,cliprect,tilemap[0],TILEMAP_IGNORE_TRANSPARENCY,0);
	draw_sprites(bitmap,cliprect,0);
	tilemap_draw(bitmap,cliprect,tilemap[1],0,0);
	draw_sprites(bitmap,cliprect,1);
	for (offs = 0x400 - 1; offs > 0; offs--)
	{
		int mx,my,sx,sy;

        mx = offs % 32;
		my = offs / 32;

		if (my < 2)
		{
			if (mx < 2 || mx >= 30) continue; /* not visible */
			sx = my + 34; sy = mx - 2;
		}
		else if (my >= 30)
		{
			if (mx < 2 || mx >= 30) continue; /* not visible */
			sx = my - 30; sy = mx - 2;
		}
		else
		{
			sx = mx + 2; sy = my - 2;
		}
		if (flipscreen)
		{
				sx = 35 - sx; sy = 27 - sy;
		}
		drawgfx(bitmap,Machine->gfx[0],	baraduke_textram[offs],
				(baraduke_textram[offs+0x400] << 2) & 0x1ff,
				flipscreen,flipscreen,sx*8,sy*8,
				cliprect,TRANSPARENCY_PEN,3);
	}
}
