#include "driver.h"
#include "vidhrdw/generic.h"

static int gfxbank;

static struct tilemap *bg_tilemap;

WRITE_HANDLER( sichuan2_videoram_w )
{
	if (videoram[offset] != data)
	{
		videoram[offset] = data;
		tilemap_mark_tile_dirty(bg_tilemap, offset / 2);
	}
}

WRITE_HANDLER( sichuan2_bankswitch_w )
{
	int bankaddress;
	int bank;
	UINT8 *RAM = memory_region(REGION_CPU1);

	if (data & 0xc0) log_cb(RETRO_LOG_DEBUG, LOGPRE "bank switch %02x\n",data);

	/* bits 0-2 select ROM bank */
	bankaddress = 0x10000 + (data & 0x07) * 0x4000;
	cpu_setbank(1, &RAM[bankaddress]);

	/* bits 3-5 select gfx bank */
	bank = (data & 0x38) >> 3;

	if (gfxbank != bank)
	{
		gfxbank = bank;
		tilemap_mark_all_tiles_dirty(ALL_TILEMAPS);
	}

	/* bits 6-7 unknown */
}

WRITE_HANDLER( sichuan2_paletteram_w )
{
	int r, g, b;

	paletteram[offset] = data;

	offset &= 0xff;

	r = paletteram[offset + 0x000] & 0x1f;
	g = paletteram[offset + 0x100] & 0x1f;
	b = paletteram[offset + 0x200] & 0x1f;
	r = (r << 3) | (r >> 2);
	g = (g << 3) | (g >> 2);
	b = (b << 3) | (b >> 2);

	palette_set_color(offset, r, g, b);
}

static void get_bg_tile_info(int tile_index)
{
	int offs = tile_index * 2;
	int code = videoram[offs] + ((videoram[offs + 1] & 0x0f) << 8) + (gfxbank << 12);
	int color = (videoram[offs + 1] & 0xf0) >> 4;

	SET_TILE_INFO(0, code, color, 0)
}

VIDEO_START( sichuan2 )
{
	bg_tilemap = tilemap_create(get_bg_tile_info, tilemap_scan_rows, 
		TILEMAP_OPAQUE, 8, 8, 64, 32);

	if ( !bg_tilemap )
		return 1;

	return 0;
}

VIDEO_UPDATE( sichuan2 )
{
	tilemap_draw(bitmap, &Machine->visible_area, bg_tilemap, 0, 0);
}
