/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

static int spritectrl[3];

int crbaloon_collision;

static struct tilemap *bg_tilemap;

/***************************************************************************

  Convert the color PROMs into a more useable format.

  Crazy Balloon has no PROMs, the color code directly maps to a color:
  all bits are inverted
  bit 3 HALF (intensity)
  bit 2 BLUE
  bit 1 GREEN
  bit 0 RED

***************************************************************************/
PALETTE_INIT( crbaloon )
{
	int i;
	#define TOTAL_COLORS(gfxn) (Machine->gfx[gfxn]->total_colors * Machine->gfx[gfxn]->color_granularity)
	#define COLOR(gfxn,offs) (colortable[Machine->drv->gfxdecodeinfo[gfxn].color_codes_start + offs])


	for (i = 0;i < Machine->drv->total_colors;i++)
	{
		int intensity,r,g,b;


		intensity = (~i & 0x08) ? 0xff : 0x55;

		/* red component */
		r = intensity * ((~i >> 0) & 1);
		/* green component */
		g = intensity * ((~i >> 1) & 1);
		/* blue component */
		b = intensity * ((~i >> 2) & 1);
		palette_set_color(i,r,g,b);
	}

	for (i = 0;i < TOTAL_COLORS(0);i += 2)
	{
		COLOR(0,i) = 15;		/* black background */
		COLOR(0,i + 1) = i / 2;	/* colored foreground */
	}
}

WRITE_HANDLER( crbaloon_videoram_w )
{
	if (videoram[offset] != data)
	{
		videoram[offset] = data;
		tilemap_mark_tile_dirty(bg_tilemap, offset);
	}
}

WRITE_HANDLER( crbaloon_colorram_w )
{
	if (colorram[offset] != data)
	{
		colorram[offset] = data;
		tilemap_mark_tile_dirty(bg_tilemap, offset);
	}
}

WRITE_HANDLER( crbaloon_spritectrl_w )
{
	spritectrl[offset] = data;
}

WRITE_HANDLER( crbaloon_flipscreen_w )
{
	if (flip_screen != (data & 0x01))
	{
		flip_screen_set(data & 0x01);
		tilemap_mark_all_tiles_dirty(ALL_TILEMAPS);
	}
}

static void get_bg_tile_info(int tile_index)
{
	int code = videoram[tile_index];
	int color = colorram[tile_index] & 0x0f;

	SET_TILE_INFO(0, code, color, 0)
}

VIDEO_START( crbaloon )
{
	bg_tilemap = tilemap_create(get_bg_tile_info, tilemap_scan_rows_flip_xy, 
		TILEMAP_OPAQUE, 8, 8, 32, 32);

	if ( !bg_tilemap )
		return 1;

	if ((tmpbitmap = auto_bitmap_alloc(Machine->drv->screen_width,Machine->drv->screen_height)) == 0)
		return 1;

	return 0;
}

static void crbaloon_draw_sprites( struct mame_bitmap *bitmap )
{
	int x,y;

	/* Check Collision - Draw balloon in background colour, if no */
    /* collision occured, bitmap will be same as tmpbitmap        */

	int bx = spritectrl[1];
	int by = spritectrl[2] - 32;

	tilemap_draw(tmpbitmap, 0, bg_tilemap, 0, 0);

	if (flip_screen)
	{
		by += 32;
	}

	drawgfx(bitmap,Machine->gfx[1],
			spritectrl[0] & 0x0f,
			15,
			0,0,
			bx,by,
			&Machine->visible_area,TRANSPARENCY_PEN,0);

    crbaloon_collision = 0;

	for (x = bx; x < bx + Machine->gfx[1]->width; x++)
	{
		for (y = by; y < by + Machine->gfx[1]->height; y++)
        {
			if ((x < Machine->visible_area.min_x) ||
			    (x > Machine->visible_area.max_x) ||
			    (y < Machine->visible_area.min_y) ||
			    (y > Machine->visible_area.max_y))
			{
				continue;
			}

        	if (read_pixel(bitmap, x, y) != read_pixel(tmpbitmap, x, y))
        	{
				crbaloon_collision = -1;
				break;
			}
        }
	}


	/* actually draw the balloon */

	drawgfx(bitmap,Machine->gfx[1],
			spritectrl[0] & 0x0f,
			(spritectrl[0] & 0xf0) >> 4,
			0,0,
			bx,by,
			&Machine->visible_area,TRANSPARENCY_PEN,0);
}

VIDEO_UPDATE( crbaloon )
{
	tilemap_draw(bitmap, &Machine->visible_area, bg_tilemap, 0, 0);
	crbaloon_draw_sprites(bitmap);
}
