/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/
#include "driver.h"
#include "vidhrdw/generic.h"


static struct tilemap *tilemap;
static int char_bank,palette_bank,flipscreen,gfxctrl;

UINT8 *flstory_scrlram;


static void get_tile_info(int tile_index)
{
	int code = videoram[tile_index*2];
	int attr = videoram[tile_index*2+1];
	int tile_number = code + ((attr & 0xc0) << 2) + 0x400 + 0x800 * char_bank;
    int flags = TILE_FLIPYX((attr & 0x18) >> 3) | TILE_SPLIT((attr & 0x20) >> 5);
	tile_info.priority = (attr & 0x20) >> 5;
	SET_TILE_INFO(
			0,
			tile_number,
			attr & 0x0f,
			flags)
}

static void rumba_get_tile_info(int tile_index)
{
	int code = videoram[tile_index*2];
	int attr = videoram[tile_index*2+1];
	int tile_number = code + ((attr & 0xc0) << 2) + 0x400 + 0x800 * char_bank;
	int col = (attr & 0x0f);
/*	tile_info.priority = (attr & 0x20) >> 5; */
	SET_TILE_INFO(
			0,
			tile_number,
			col,
			0)
}

VIDEO_START( flstory )
{
    tilemap = tilemap_create( get_tile_info,tilemap_scan_rows,TILEMAP_SPLIT,8,8,32,32 );
/*	tilemap_set_transparent_pen( tilemap,15 ); */
	tilemap_set_transmask(tilemap,0,0x3fff,0xc000); /* split type 0 has pens 0-13 transparent in front half */
	tilemap_set_transmask(tilemap,1,0x8000,0x7fff); /* split type 1 has pen 15 transparent in front half */
	tilemap_set_scroll_cols(tilemap,32);

	paletteram = auto_malloc(0x200);
	paletteram_2 = auto_malloc(0x200);
	return video_start_generic();
}

VIDEO_START( rumba )
{
	tilemap = tilemap_create( rumba_get_tile_info,tilemap_scan_rows,TILEMAP_SPLIT,8,8,32,32 );
	tilemap_set_transmask(tilemap,0,0x3fff,0xc000);
	tilemap_set_scroll_cols(tilemap, 32);

	paletteram = auto_malloc(0x200);
	paletteram_2 = auto_malloc(0x200);
	return video_start_generic();
}

WRITE_HANDLER( flstory_videoram_w )
{
	videoram[offset] = data;
	tilemap_mark_tile_dirty(tilemap,offset/2);
}

WRITE_HANDLER( flstory_palette_w )
{
	if (offset & 0x100)
		paletteram_xxxxBBBBGGGGRRRR_split2_w((offset & 0xff) + (palette_bank << 8),data);
	else
		paletteram_xxxxBBBBGGGGRRRR_split1_w((offset & 0xff) + (palette_bank << 8),data);
}

READ_HANDLER( flstory_palette_r )
{
	if (offset & 0x100)
		return paletteram_2[ (offset & 0xff) + (palette_bank << 8) ];
	else
		return paletteram  [ (offset & 0xff) + (palette_bank << 8) ];
}

WRITE_HANDLER( flstory_gfxctrl_w )
{
	if (gfxctrl == data)
		return;
	gfxctrl = data;

	flipscreen = (~data & 0x01);
	if (char_bank != ((data & 0x10) >> 4))
	{
		char_bank = (data & 0x10) >> 4;
		tilemap_mark_all_tiles_dirty(tilemap);
	}
	palette_bank = (data & 0x20) >> 5;

	flip_screen_set(flipscreen);

/*usrintf_showmessage("%04x: gfxctrl = %02x\n",activecpu_get_pc(),data);*/

}

READ_HANDLER( flstory_scrlram_r )
{
	return flstory_scrlram[offset];
}

WRITE_HANDLER( flstory_scrlram_w )
{
	flstory_scrlram[offset] = data;
	tilemap_set_scrolly(tilemap, offset, data );
}

WRITE_HANDLER( rumba_gfxctrl_w )
{
	if (gfxctrl == data)
		return;
	gfxctrl = data;

	palette_bank = (data & 0x20) >> 5;

	if (data & 0x04)
	{
		flipscreen = (data & 0x01);
		flip_screen_set(flipscreen);
	}

/* usrintf_showmessage("%04x: gfxctrl = %02x\n",activecpu_get_pc(),data); */

}

/***************************************************************************

  Draw the game screen in the given mame_bitmap.
  Do NOT call osd_update_display() from this function, it will be called by
  the main emulation engine.

***************************************************************************/

void flstory_draw_sprites(struct mame_bitmap *bitmap, const struct rectangle *cliprect, int pri)
{
	int i;

	for (i = 0; i < 0x20; i++)
	{
		int pr = spriteram[spriteram_size-1 -i];
		int offs = (pr & 0x1f) * 4;

		if ((pr & 0x80) == pri)
		{
			int code,sx,sy,flipx,flipy;

			code = spriteram[offs+2] + ((spriteram[offs+1] & 0x30) << 4);
			sx = spriteram[offs+3];
			sy = spriteram[offs+0];

			if (flipscreen)
			{
				sx = (240 - sx) & 0xff ;
				sy = sy - 1 ;
			}
			else
				sy = 240 - sy - 1 ;

			flipx = ((spriteram[offs+1]&0x40)>>6)^flipscreen;
			flipy = ((spriteram[offs+1]&0x80)>>7)^flipscreen;

			drawgfx(bitmap,Machine->gfx[1],
					code,
					spriteram[offs+1] & 0x0f,
					flipx,flipy,
					sx,sy,
					cliprect,TRANSPARENCY_PEN,15);
			/* wrap around */
			if (sx > 240)
				drawgfx(bitmap,Machine->gfx[1],
						code,
						spriteram[offs+1] & 0x0f,
						flipx,flipy,
						sx-256,sy,
						cliprect,TRANSPARENCY_PEN,15);
		}
	}
}

VIDEO_UPDATE( flstory )
{
	tilemap_draw(bitmap,cliprect,tilemap,0|TILEMAP_BACK,0);
	tilemap_draw(bitmap,cliprect,tilemap,1|TILEMAP_BACK,0);
	flstory_draw_sprites(bitmap,cliprect,0x00);
	tilemap_draw(bitmap,cliprect,tilemap,0|TILEMAP_FRONT,0);
	flstory_draw_sprites(bitmap,cliprect,0x80);
    tilemap_draw(bitmap,cliprect,tilemap,1|TILEMAP_FRONT,0);
}

void rumba_draw_sprites(struct mame_bitmap *bitmap, const struct rectangle *cliprect)
{
	int i;

	for (i = 0; i < 0x20; i++)
	{
		int pr = spriteram[spriteram_size-1 -i];
		int offs = (pr & 0x1f) * 4;

		/* if ((pr & 0x80) == pri) */
		{
			int code,sx,sy,flipx,flipy;

			code = spriteram[offs+2] + ((spriteram[offs+1] & 0x20) << 3);
			sx = spriteram[offs+3];
			sy = spriteram[offs+0];

			if (flipscreen)
			{
				sx = (240 - sx + 1) & 0xff ;
				sy = sy + 1 ;
			}
			else
				sy = 240 - sy + 1 ;

			flipx = ((spriteram[offs+1]&0x40)>>6)^flipscreen;
			flipy = ((spriteram[offs+1]&0x80)>>7)^flipscreen;

			drawgfx(bitmap,Machine->gfx[1],
					code,
					spriteram[offs+1] & 0x0f,
					flipx,flipy,
					sx,sy,
					cliprect,TRANSPARENCY_PEN,15);
			/* wrap around */
			if (sx > 240)
				drawgfx(bitmap,Machine->gfx[1],
						code,
						spriteram[offs+1] & 0x0f,
						flipx,flipy,
						sx-256,sy,
						cliprect,TRANSPARENCY_PEN,15);
		}
	}
}


VIDEO_UPDATE( rumba )
{
	tilemap_draw(bitmap,cliprect,tilemap,TILEMAP_BACK,0);
	rumba_draw_sprites(bitmap,cliprect);
	tilemap_draw(bitmap,cliprect,tilemap,TILEMAP_FRONT,0);
	rumba_draw_sprites(bitmap,cliprect);

}
