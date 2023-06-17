/*******************************************************************************

     Nova 2001 - Video Description:
     --------------------------------------------------------------------------

     Foreground Playfield Chars (static)
        character code index from first set of chars
     Foreground Playfield color modifier RAM
        Char colors are normally taken from the first set of 16,
        This is the pen for "color 1" for this tile (from the first 16 pens)

     Background Playfield Chars (scrolling)
        character code index from second set of chars
     Foreground Playfield color modifier RAM
        Char colors are normally taken from the second set of 16,
        This is the pen for "color 1" for this tile (from the second 16 pens)
     (Scrolling in controlled via the 8910 A and B port outputs)

     Sprite memory is made of 32 byte records:

        Sprite+0, 0x80 = Sprite Bank
        Sprite+0, 0x7f = Sprite Character Code
        Sprite+1, 0xff = X location
        Sprite+2, 0xff = Y location
        Sprite+3, 0x20 = Y Flip
        Sprite+3, 0x10 = X Flip
        Sprite+3, 0x0f = pen for "color 1" taken from the first 16 colors
	Sprite+3, 0x80
	Sprite+3, 0x40

        All the rest are unknown and/or uneccessary.

*******************************************************************************/

#include "vidhrdw/generic.h"

UINT8 *nova2001_videoram2, *nova2001_colorram2;

static struct tilemap *bg_tilemap, *fg_tilemap;

PALETTE_INIT( nova2001 )
{
	int i,j;


	for (i = 0;i < Machine->drv->total_colors;i++)
	{
		int intensity,r,g,b;


		intensity = (*color_prom >> 0) & 0x03;
		/* red component */
		r = (((*color_prom >> 0) & 0x0c) | intensity) * 0x11;
		/* green component */
		g = (((*color_prom >> 2) & 0x0c) | intensity) * 0x11;
		/* blue component */
		b = (((*color_prom >> 4) & 0x0c) | intensity) * 0x11;

		palette_set_color(i,r,g,b);
		color_prom++;
	}

	/* Color #1 is used for palette animation.          */

	/* To handle this, color entries 0-15 are based on  */
	/* the primary 16 colors, while color entries 16-31 */
	/* are based on the secondary set.                  */

	/* The only difference among 0-15 and 16-31 is that */
	/* color #1 changes each time */

	for (i = 0;i < 16;i++)
	{
		for (j = 0;j < 16;j++)
		{
			if (j == 1)
			{
				colortable[16*i+1] = i;
				colortable[16*i+16*16+1] = i+16;
			}
			else
			{
				colortable[16*i+j] = j;
				colortable[16*i+16*16+j] = j+16;
			}
		}
	}
}

WRITE_HANDLER( nova2001_videoram_w )
{
	if (videoram[offset] != data)
	{
		videoram[offset] = data;
		tilemap_mark_tile_dirty(bg_tilemap, offset);
	}
}

WRITE_HANDLER( nova2001_colorram_w )
{
	if (colorram[offset] != data)
	{
		colorram[offset] = data;
		tilemap_mark_tile_dirty(bg_tilemap, offset);
	}
}

WRITE_HANDLER( nova2001_videoram2_w )
{
	if (nova2001_videoram2[offset] != data)
	{
		nova2001_videoram2[offset] = data;
		tilemap_mark_tile_dirty(fg_tilemap, offset);
	}
}

WRITE_HANDLER( nova2001_colorram2_w )
{
	if (nova2001_colorram2[offset] != data)
	{
		nova2001_colorram2[offset] = data;
		tilemap_mark_tile_dirty(fg_tilemap, offset);
	}
}

WRITE_HANDLER( nova2001_scroll_x_w )
{
	tilemap_set_scrollx(bg_tilemap, 0, data - (flip_screen ? 0 : 7));
}

WRITE_HANDLER( nova2001_scroll_y_w )
{
	tilemap_set_scrolly(bg_tilemap, 0, data);
}

WRITE_HANDLER( nova2001_flipscreen_w )
{
	if (flip_screen != (~data & 0x01))
	{
		flip_screen_set(~data & 0x01);
		tilemap_mark_all_tiles_dirty(ALL_TILEMAPS);
	}
}

static void get_bg_tile_info(int tile_index)
{
	int code = videoram[tile_index];
	int color = colorram[tile_index] & 0x0f;

	SET_TILE_INFO(1, code, color, 0)
}

static void get_fg_tile_info(int tile_index)
{
	int code = nova2001_videoram2[tile_index];
	int color = nova2001_colorram2[tile_index] & 0x0f;

	SET_TILE_INFO(0, code, color, 0)
}

VIDEO_START( nova2001 )
{
	bg_tilemap = tilemap_create(get_bg_tile_info, tilemap_scan_rows, 
		TILEMAP_OPAQUE, 8, 8, 32, 32);

	if ( !bg_tilemap )
		return 1;

	fg_tilemap = tilemap_create(get_fg_tile_info, tilemap_scan_rows, 
		TILEMAP_TRANSPARENT, 8, 8, 32, 32);

	if ( !fg_tilemap )
		return 1;

	tilemap_set_transparent_pen(fg_tilemap, 0);

	return 0;
}

static void nova2001_draw_sprites( struct mame_bitmap *bitmap )
{
	int offs;

	for (offs = 0;offs < spriteram_size;offs += 32)
	{
		int flipx = spriteram[offs+3] & 0x10;
		int flipy = spriteram[offs+3] & 0x20;
		int sx = spriteram[offs+1];
		int sy = spriteram[offs+2];

		if (flip_screen)
		{
			sx = 240 - sx;
			sy = 240 - sy;
			flipx = !flipx;
			flipy = !flipy;
		}

		drawgfx(bitmap,Machine->gfx[2 + ((spriteram[offs+0] & 0x80) >> 7)],
			spriteram[offs+0] & 0x7f,
			spriteram[offs+3] & 0x0f,
			flipx,flipy,
			sx,sy,
			&Machine->visible_area,TRANSPARENCY_PEN,0);
	}
}

VIDEO_UPDATE( nova2001 )
{
	tilemap_draw(bitmap, &Machine->visible_area, bg_tilemap, 0, 0);
	nova2001_draw_sprites(bitmap);
	tilemap_draw(bitmap, &Machine->visible_area, fg_tilemap, 0, 0);
}
