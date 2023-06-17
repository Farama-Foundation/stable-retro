#include "driver.h"
#include "vidhrdw/generic.h"


UINT8 *ninjakd2_bg_videoram, *ninjakd2_fg_videoram;

static UINT8  ninjakd2_bg_enable = 1, sprite_overdraw_enabled = 0;
static UINT16 ninjakd2_scrollx, ninjakd2_scrolly;
static struct tilemap *fg_tilemap, *bg_tilemap;
static struct mame_bitmap *bitmap_sp;	/* for sprite overdraw */

static void get_bg_tile_info(int tile_index)
{
	int code = ((ninjakd2_bg_videoram[tile_index*2 + 1] & 0xc0) << 2) | ninjakd2_bg_videoram[tile_index*2];
	int color = ninjakd2_bg_videoram[tile_index*2 + 1] & 0xf;
	SET_TILE_INFO(0, code, color, TILE_FLIPYX((ninjakd2_bg_videoram[tile_index*2 + 1] & 0x30) >> 4))
}

static void get_fg_tile_info(int tile_index)
{
	int code = ((ninjakd2_fg_videoram[tile_index*2 + 1] & 0xc0) << 2) | ninjakd2_fg_videoram[tile_index*2];
	int color = ninjakd2_fg_videoram[tile_index*2 + 1] & 0xf;
	SET_TILE_INFO(2, code, color, TILE_FLIPYX((ninjakd2_fg_videoram[tile_index*2 + 1] & 0x30) >> 4))
}

VIDEO_START( ninjakd2 )
{
	bg_tilemap = tilemap_create(get_bg_tile_info, tilemap_scan_rows, TILEMAP_OPAQUE,      16, 16, 32, 32);
	fg_tilemap = tilemap_create(get_fg_tile_info, tilemap_scan_rows, TILEMAP_TRANSPARENT,  8,  8,  32, 32);

	tilemap_set_transparent_pen(fg_tilemap, 15);

	if ((bitmap_sp = auto_bitmap_alloc(Machine->drv->screen_width,Machine->drv->screen_height)) == 0)
		return 1;

	return 0;
}

WRITE_HANDLER( ninjakd2_bgvideoram_w )
{
	ninjakd2_bg_videoram[offset] = data;
	tilemap_mark_tile_dirty(bg_tilemap, offset >> 1);
}

WRITE_HANDLER( ninjakd2_fgvideoram_w )
{
	ninjakd2_fg_videoram[offset] = data;
	tilemap_mark_tile_dirty(fg_tilemap, offset >> 1);
}

WRITE_HANDLER( ninjakd2_scrollx_w )
{
	if(offset)
		ninjakd2_scrollx = ((ninjakd2_scrollx & 0x0ff) | data*256) & 0x1ff;
	else
		ninjakd2_scrollx = ((ninjakd2_scrollx & 0x100) | data) & 0x1ff;
}

WRITE_HANDLER( ninjakd2_scrolly_w )
{
	if(offset)
		ninjakd2_scrolly = ((ninjakd2_scrolly & 0x0ff) | data*256) & 0x1ff;
	else
		ninjakd2_scrolly = ((ninjakd2_scrolly & 0x100) | data) & 0x1ff;
}

WRITE_HANDLER( ninjakd2_background_enable_w )
{
	ninjakd2_bg_enable = data & 1;
}

WRITE_HANDLER( ninjakd2_sprite_overdraw_w )
{
	sprite_overdraw_enabled = data & 1;

	if(sprite_overdraw_enabled)
		fillbitmap(bitmap_sp,15,&Machine->visible_area);
}

void ninjakd2_draw_sprites(struct mame_bitmap *bitmap, const struct rectangle *cliprect)
{
	int offs;

	/* Draw the sprites */

	for (offs = 11 ;offs < spriteram_size; offs+=16)
	{
		int sx,sy,tile,color,flipx,flipy;

		if (spriteram[offs+2] & 2)
		{
			sx = spriteram[offs+1];
			sy = spriteram[offs];
			if (spriteram[offs+2] & 1) sx-=256;
			tile = spriteram[offs+3]+((spriteram[offs+2] & 0xc0)<<2);
			flipx = spriteram[offs+2] & 0x10;
			flipy = spriteram[offs+2] & 0x20;
			color = spriteram[offs+4] & 0x0f;

			if(sprite_overdraw_enabled && (color >= 0x0c && color <= 0x0e) )
			{
				/* "static" sprites */
				drawgfx(bitmap_sp,Machine->gfx[1],
						tile,
						color,
						flipx,flipy,
						sx,sy,
						cliprect,
						TRANSPARENCY_PEN, 15);
			}
			else
			{
				drawgfx(bitmap,Machine->gfx[1],
						tile,
						color,
						flipx,flipy,
						sx,sy,
						cliprect,
						TRANSPARENCY_PEN, 15);

				/* "normal" sprites with color = 0x0f clear the "static" ones */
				if(sprite_overdraw_enabled && color == 0x0f)
				{
					int x,y,offset = 0;
					const struct GfxElement *gfx = Machine->gfx[1];
					UINT8 *srcgfx = gfx->gfxdata + tile * gfx->char_modulo;

					for(y = 0; y < gfx->height; y++)
					{
						for(x = 0; x < gfx->width; x++)
						{
							if(srcgfx[offset] != 15)
							{
								plot_pixel(bitmap_sp, sx + x, sy + y, 15);
							}

							offset++;
						}
					}
				}
			}
		}
	}

	if(sprite_overdraw_enabled)
		copybitmap(bitmap, bitmap_sp, 0, 0, 0, 0, cliprect, TRANSPARENCY_PEN, 15);
}

VIDEO_UPDATE( ninjakd2 )
{
	fillbitmap(bitmap, Machine->pens[0],0);

	tilemap_set_scrollx(bg_tilemap, 0, ninjakd2_scrollx);
	tilemap_set_scrolly(bg_tilemap, 0, ninjakd2_scrolly);

	if (ninjakd2_bg_enable)
		tilemap_draw(bitmap, cliprect, bg_tilemap, 0, 0);

	ninjakd2_draw_sprites(bitmap, cliprect);
	tilemap_draw(bitmap, cliprect, fg_tilemap, 0, 0);

}

