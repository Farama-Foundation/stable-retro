#include "driver.h"
#include "vidhrdw/generic.h"



extern data16_t *welltris_spriteram;
extern size_t welltris_spriteram_size;
extern data16_t *welltris_pixelram;
extern data16_t *welltris_charvideoram;

static struct tilemap *char_tilemap;
static unsigned char gfxbank[8];
static UINT16 charpalettebank;
static UINT16 spritepalettebank;
static UINT16 pixelpalettebank;
static int welltris_scrollx, welltris_scrolly;



READ16_HANDLER( welltris_spriteram_r )
{
	return welltris_spriteram[offset];
}

WRITE16_HANDLER( welltris_spriteram_w )
{
	int offs;

	COMBINE_DATA(&welltris_spriteram[offset]);

	/* hack... sprite doesn't work otherwise (quiz18kn) */
	if ((offset == 0x1fe) &&
		(welltris_spriteram[0x01fc] == 0x0000) &&
		(welltris_spriteram[0x01fd] == 0x0000) &&
		(welltris_spriteram[0x01ff] == 0x0000)) {
		for (offs = 0; offs < 0x1fc; offs++) welltris_spriteram[offs] = 0x0000;
	}
}


/* Sprite Drawing is pretty much the same as fromance.c */
static void welltris_draw_sprites(struct mame_bitmap *bitmap,const struct rectangle *cliprect)
{
	UINT8 zoomtable[16] = { 0,7,14,20,25,30,34,38,42,46,49,52,54,57,59,61 };
	int offs;

	/* draw the sprites */
	for (offs = 0; offs < 0x200 - 4; offs += 4) {
		int data0 = welltris_spriteram[offs + 0];
		int data1 = welltris_spriteram[offs + 1];
		int data2 = welltris_spriteram[offs + 2];
		int data3 = welltris_spriteram[offs + 3];
		int code = data3 & 0x1fff;
		int color = (data2 & 0x0f) + (0x10 * spritepalettebank);
		int y = (data0 & 0x1ff) + 1;
		int x = (data1 & 0x1ff) + 6;
		int yzoom = (data0 >> 12) & 15;
		int xzoom = (data1 >> 12) & 15;
		int zoomed = (xzoom | yzoom);
		int ytiles = ((data2 >> 12) & 7) + 1;
		int xtiles = ((data2 >>  8) & 7) + 1;
		int yflip = (data2 >> 15) & 1;
		int xflip = (data2 >> 11) & 1;
		int xt, yt;

		if (!(welltris_spriteram[offs + 2] & 0x0080)) continue;

		/* compute the zoom factor -- stolen from aerofgt.c */
		xzoom = 16 - zoomtable[xzoom] / 8;
		yzoom = 16 - zoomtable[yzoom] / 8;

		/* wrap around */
		if (x > Machine->visible_area.max_x) x -= 0x200;
		if (y > Machine->visible_area.max_y) y -= 0x200;

		/* normal case */
		if (!xflip && !yflip) {
			for (yt = 0; yt < ytiles; yt++) {
				for (xt = 0; xt < xtiles; xt++, code++) {
					if (!zoomed)
						drawgfx(bitmap, Machine->gfx[1], code, color, 0, 0,
								x + xt * 16, y + yt * 16, cliprect, TRANSPARENCY_PEN, 15);
					else
						drawgfxzoom(bitmap, Machine->gfx[1], code, color, 0, 0,
								x + xt * xzoom, y + yt * yzoom, cliprect, TRANSPARENCY_PEN, 15,
								0x1000 * xzoom, 0x1000 * yzoom);
				}
				if (xtiles == 3) code += 1;
				if (xtiles == 5) code += 3;
				if (xtiles == 6) code += 2;
				if (xtiles == 7) code += 1;
			}
		}

		/* xflipped case */
		else if (xflip && !yflip) {
			for (yt = 0; yt < ytiles; yt++) {
				for (xt = 0; xt < xtiles; xt++, code++) {
					if (!zoomed)
						drawgfx(bitmap, Machine->gfx[1], code, color, 1, 0,
								x + (xtiles - 1 - xt) * 16, y + yt * 16, cliprect, TRANSPARENCY_PEN, 15);
					else
						drawgfxzoom(bitmap, Machine->gfx[1], code, color, 1, 0,
								x + (xtiles - 1 - xt) * xzoom, y + yt * yzoom, cliprect, TRANSPARENCY_PEN, 15,
								0x1000 * xzoom, 0x1000 * yzoom);
				}
				if (xtiles == 3) code += 1;
				if (xtiles == 5) code += 3;
				if (xtiles == 6) code += 2;
				if (xtiles == 7) code += 1;
			}
		}

		/* yflipped case */
		else if (!xflip && yflip) {
			for (yt = 0; yt < ytiles; yt++) {
				for (xt = 0; xt < xtiles; xt++, code++) {
					if (!zoomed)
						drawgfx(bitmap, Machine->gfx[1], code, color, 0, 1,
								x + xt * 16, y + (ytiles - 1 - yt) * 16, cliprect, TRANSPARENCY_PEN, 15);
					else
						drawgfxzoom(bitmap, Machine->gfx[1], code, color, 0, 1,
								x + xt * xzoom, y + (ytiles - 1 - yt) * yzoom, cliprect, TRANSPARENCY_PEN, 15,
								0x1000 * xzoom, 0x1000 * yzoom);
				}
				if (xtiles == 3) code += 1;
				if (xtiles == 5) code += 3;
				if (xtiles == 6) code += 2;
				if (xtiles == 7) code += 1;
			}
		}

		/* x & yflipped case */
		else {
			for (yt = 0; yt < ytiles; yt++) {
				for (xt = 0; xt < xtiles; xt++, code++) {
					if (!zoomed)
						drawgfx(bitmap, Machine->gfx[1], code, color, 1, 1,
								x + (xtiles - 1 - xt) * 16, y + (ytiles - 1 - yt) * 16, cliprect, TRANSPARENCY_PEN, 15);
					else
						drawgfxzoom(bitmap, Machine->gfx[1], code, color, 1, 1,
								x + (xtiles - 1 - xt) * xzoom, y + (ytiles - 1 - yt) * yzoom, cliprect, TRANSPARENCY_PEN, 15,
								0x1000 * xzoom, 0x1000 * yzoom);
				}
				if (xtiles == 3) code += 1;
				if (xtiles == 5) code += 3;
				if (xtiles == 6) code += 2;
				if (xtiles == 7) code += 1;
			}
		}
	}
}

static void setbank(struct tilemap *tmap, int num, int bank)
{
	if (gfxbank[num] != bank)
	{
		gfxbank[num] = bank;
		tilemap_mark_all_tiles_dirty(tmap);
	}
}


/* Not really enough evidence here */

WRITE16_HANDLER( welltris_palette_bank_w )
{
	if (ACCESSING_LSB)
	{
		if (charpalettebank != (data & 0x03))
		{
			charpalettebank = (data & 0x03);
			tilemap_mark_all_tiles_dirty(char_tilemap);
		}

		flip_screen_set(data & 0x80);

		spritepalettebank = (data & 0x20) >> 5;
		pixelpalettebank = (data & 0x08) >> 3;
	}
}

WRITE16_HANDLER( welltris_gfxbank_w )
{
	if (ACCESSING_LSB)
	{
		setbank(char_tilemap, 0, (data & 0xf0) >> 4);
		setbank(char_tilemap, 1, data & 0x0f);
	}
}

WRITE16_HANDLER( welltris_scrollreg_w )
{
	switch (offset) {
		case 0: welltris_scrollx = data - 14; break;
		case 1: welltris_scrolly = data +  0; break;
	}
}

static void get_welltris_tile_info(int tile_index)
{
	UINT16 code = welltris_charvideoram[tile_index];
	int bank = (code & 0x1000) >> 12;

	SET_TILE_INFO(
			0,
			(code & 0x0fff) + (gfxbank[bank] << 12),
			((code & 0xe000) >> 13) + (8 * charpalettebank),
			0)
}

WRITE16_HANDLER( welltris_charvideoram_w )
{
	int oldword = welltris_charvideoram[offset];
	COMBINE_DATA(&welltris_charvideoram[offset]);
	if (oldword != welltris_charvideoram[offset])
		tilemap_mark_tile_dirty(char_tilemap, offset);
}

VIDEO_START( welltris )
{
	char_tilemap = tilemap_create(get_welltris_tile_info, tilemap_scan_rows, TILEMAP_TRANSPARENT, 8, 8, 64, 32);
	if (!char_tilemap) return 1;

	tilemap_set_transparent_pen(char_tilemap, 15);

	return 0;
}

static void welltris_drawbackground(struct mame_bitmap *bitmap, const struct rectangle *cliprect)
{
	int x, y;
	int pixdata;

	for (y = 0; y < 256; y++) {
		for (x = 0; x < 512 / 2; x++) {
			pixdata = welltris_pixelram[(x & 0xff) + (y & 0xff) * 256];

			plot_pixel(bitmap, x*2,   y, (pixdata >> 8) + (0x100 * pixelpalettebank) + 0x400);
			plot_pixel(bitmap, x*2+1, y, (pixdata & 0xff) + (0x100 * pixelpalettebank) + 0x400);
		}
	}
}

VIDEO_UPDATE( welltris )
{
	tilemap_set_scrollx(char_tilemap, 0, welltris_scrollx);
	tilemap_set_scrolly(char_tilemap, 0, welltris_scrolly);

	welltris_drawbackground(bitmap, cliprect);
	tilemap_draw(bitmap, cliprect, char_tilemap, 0, 0);
	welltris_draw_sprites(bitmap, cliprect);
}
