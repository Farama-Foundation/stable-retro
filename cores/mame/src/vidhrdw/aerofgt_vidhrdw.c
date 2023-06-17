#include "driver.h"


data16_t *aerofgt_rasterram;
data16_t *aerofgt_bg1videoram,*aerofgt_bg2videoram;
data16_t *aerofgt_spriteram1,*aerofgt_spriteram2,*aerofgt_spriteram3;
size_t aerofgt_spriteram1_size,aerofgt_spriteram2_size,aerofgt_spriteram3_size;

static unsigned char gfxbank[8];
static data16_t bg1scrollx,bg1scrolly,bg2scrollx,bg2scrolly;

static int charpalettebank,spritepalettebank;

static struct tilemap *bg1_tilemap,*bg2_tilemap;
static int sprite_gfx;


/***************************************************************************

  Callbacks for the TileMap code

***************************************************************************/

static void get_pspikes_tile_info(int tile_index)
{
	UINT16 code = aerofgt_bg1videoram[tile_index];
	int bank = (code & 0x1000) >> 12;
	SET_TILE_INFO(
			0,
			(code & 0x0fff) + (gfxbank[bank] << 12),
			((code & 0xe000) >> 13) + 8 * charpalettebank,
			0)
}

static void karatblz_bg1_tile_info(int tile_index)
{
	UINT16 code = aerofgt_bg1videoram[tile_index];
	SET_TILE_INFO(
			0,
			(code & 0x1fff) + (gfxbank[0] << 13),
			(code & 0xe000) >> 13,
			0)
}

/* also spinlbrk */
static void karatblz_bg2_tile_info(int tile_index)
{
	UINT16 code = aerofgt_bg2videoram[tile_index];
	SET_TILE_INFO(
			1,
			(code & 0x1fff) + (gfxbank[1] << 13),
			(code & 0xe000) >> 13,
			0)
}

static void spinlbrk_bg1_tile_info(int tile_index)
{
	UINT16 code = aerofgt_bg1videoram[tile_index];
	SET_TILE_INFO(
			0,
			(code & 0x0fff) + (gfxbank[0] << 12),
			(code & 0xf000) >> 12,
			0)
}

static void get_bg1_tile_info(int tile_index)
{
	UINT16 code = aerofgt_bg1videoram[tile_index];
	int bank = (code & 0x1800) >> 11;
	SET_TILE_INFO(
			0,
			(code & 0x07ff) + (gfxbank[bank] << 11),
			(code & 0xe000) >> 13,
			0)
}

static void get_bg2_tile_info(int tile_index)
{
	UINT16 code = aerofgt_bg2videoram[tile_index];
	int bank = 4 + ((code & 0x1800) >> 11);
	SET_TILE_INFO(
			1,
			(code & 0x07ff) + (gfxbank[bank] << 11),
			(code & 0xe000) >> 13,
			0)
}


/***************************************************************************

  Start the video hardware emulation.

***************************************************************************/

VIDEO_START( pspikes )
{
	bg1_tilemap = tilemap_create(get_pspikes_tile_info,tilemap_scan_rows,TILEMAP_OPAQUE,8,8,64,32);
	/* no bg2 in this game */

	if (!bg1_tilemap)
		return 1;

	sprite_gfx = 1;

	return 0;
}

VIDEO_START( karatblz )
{
	bg1_tilemap = tilemap_create(karatblz_bg1_tile_info,tilemap_scan_rows,TILEMAP_OPAQUE,     8,8,64,64);
	bg2_tilemap = tilemap_create(karatblz_bg2_tile_info,tilemap_scan_rows,TILEMAP_TRANSPARENT,8,8,64,64);

	if (!bg1_tilemap || !bg2_tilemap)
		return 1;

	tilemap_set_transparent_pen(bg2_tilemap,15);

	spritepalettebank = 0;

	sprite_gfx = 2;

	return 0;
}

VIDEO_START( spinlbrk )
{
	int i;

	bg1_tilemap = tilemap_create(spinlbrk_bg1_tile_info,tilemap_scan_rows,TILEMAP_OPAQUE,     8,8,64,64);
	bg2_tilemap = tilemap_create(karatblz_bg2_tile_info,tilemap_scan_rows,TILEMAP_TRANSPARENT,8,8,64,64);

	if (!bg1_tilemap || !bg2_tilemap)
		return 1;

	tilemap_set_transparent_pen(bg2_tilemap,15);

	spritepalettebank = 0;

	sprite_gfx = 2;


	/* sprite maps are hardcoded in this game */

	/* enemy sprites use ROM instead of RAM */
	aerofgt_spriteram2 = (data16_t *)memory_region(REGION_GFX5);
	aerofgt_spriteram2_size = 0x20000;

	/* front sprites are direct maps */
	aerofgt_spriteram1 = aerofgt_spriteram2 + aerofgt_spriteram2_size/2;
	aerofgt_spriteram1_size = 0x4000;
	for (i = 0;i < aerofgt_spriteram1_size/2;i++)
		aerofgt_spriteram1[i] = i;

	return 0;
}

VIDEO_START( turbofrc )
{
	bg1_tilemap = tilemap_create(get_bg1_tile_info,tilemap_scan_rows,TILEMAP_OPAQUE,     8,8,64,64);
	bg2_tilemap = tilemap_create(get_bg2_tile_info,tilemap_scan_rows,TILEMAP_TRANSPARENT,8,8,64,64);

	if (!bg1_tilemap || !bg2_tilemap)
		return 1;

	tilemap_set_transparent_pen(bg2_tilemap,15);

	spritepalettebank = 0;

	sprite_gfx = 2;

	return 0;
}



/***************************************************************************

  Memory handlers

***************************************************************************/

WRITE16_HANDLER( aerofgt_bg1videoram_w )
{
	int oldword = aerofgt_bg1videoram[offset];
	COMBINE_DATA(&aerofgt_bg1videoram[offset]);
	if (oldword != aerofgt_bg1videoram[offset])
		tilemap_mark_tile_dirty(bg1_tilemap,offset);
}

WRITE16_HANDLER( aerofgt_bg2videoram_w )
{
	int oldword = aerofgt_bg2videoram[offset];
	COMBINE_DATA(&aerofgt_bg2videoram[offset]);
	if (oldword != aerofgt_bg2videoram[offset])
		tilemap_mark_tile_dirty(bg2_tilemap,offset);
}


static void setbank(struct tilemap *tmap,int num,int bank)
{
	if (gfxbank[num] != bank)
	{
		gfxbank[num] = bank;
		tilemap_mark_all_tiles_dirty(tmap);
	}
}

WRITE16_HANDLER( pspikes_gfxbank_w )
{
	if (ACCESSING_LSB)
	{
		setbank(bg1_tilemap,0,(data & 0xf0) >> 4);
		setbank(bg1_tilemap,1,data & 0x0f);
	}
}

WRITE16_HANDLER( karatblz_gfxbank_w )
{
	if (ACCESSING_MSB)
	{
		setbank(bg1_tilemap,0,(data & 0x0100) >> 8);
		setbank(bg2_tilemap,1,(data & 0x0800) >> 11);
	}
}

WRITE16_HANDLER( spinlbrk_gfxbank_w )
{
	if (ACCESSING_LSB)
	{
		setbank(bg1_tilemap,0,(data & 0x07));
		setbank(bg2_tilemap,1,(data & 0x38) >> 3);
	}
}

WRITE16_HANDLER( turbofrc_gfxbank_w )
{
	static data16_t bank[2];
	struct tilemap *tmap = (offset == 0) ? bg1_tilemap : bg2_tilemap;

	data = COMBINE_DATA(&bank[offset]);

	setbank(tmap,4*offset + 0,(data >>  0) & 0x0f);
	setbank(tmap,4*offset + 1,(data >>  4) & 0x0f);
	setbank(tmap,4*offset + 2,(data >>  8) & 0x0f);
	setbank(tmap,4*offset + 3,(data >> 12) & 0x0f);
}

WRITE16_HANDLER( aerofgt_gfxbank_w )
{
	static data16_t bank[4];
	struct tilemap *tmap = (offset < 2) ? bg1_tilemap : bg2_tilemap;

	data = COMBINE_DATA(&bank[offset]);

	setbank(tmap,2*offset + 0,(data >> 8) & 0xff);
	setbank(tmap,2*offset + 1,(data >> 0) & 0xff);
}

WRITE16_HANDLER( aerofgt_bg1scrollx_w )
{
	COMBINE_DATA(&bg1scrollx);
}

WRITE16_HANDLER( aerofgt_bg1scrolly_w )
{
	COMBINE_DATA(&bg1scrolly);
}

WRITE16_HANDLER( aerofgt_bg2scrollx_w )
{
	COMBINE_DATA(&bg2scrollx);
}

WRITE16_HANDLER( aerofgt_bg2scrolly_w )
{
	COMBINE_DATA(&bg2scrolly);
}

WRITE16_HANDLER( pspikes_palette_bank_w )
{
	if (ACCESSING_LSB)
	{
		spritepalettebank = data & 0x03;
		if (charpalettebank != (data & 0x1c) >> 2)
		{
			charpalettebank = (data & 0x1c) >> 2;
			tilemap_mark_all_tiles_dirty(bg1_tilemap);
		}
	}
}



/***************************************************************************

  Display refresh

***************************************************************************/

static void aerofgt_drawsprites(struct mame_bitmap *bitmap,const struct rectangle *cliprect,int priority)
{
	int offs;


	priority <<= 12;

	offs = 0;
	while (offs < 0x0400 && (aerofgt_spriteram3[offs] & 0x8000) == 0)
	{
		int attr_start;

		attr_start = 4 * (aerofgt_spriteram3[offs] & 0x03ff);

		/* is the way I handle priority correct? Or should I just check bit 13? */
		if ((aerofgt_spriteram3[attr_start + 2] & 0x3000) == priority)
		{
			int map_start;
			int ox,oy,x,y,xsize,ysize,zoomx,zoomy,flipx,flipy,color;

			ox = aerofgt_spriteram3[attr_start + 1] & 0x01ff;
			xsize = (aerofgt_spriteram3[attr_start + 1] & 0x0e00) >> 9;
			zoomx = (aerofgt_spriteram3[attr_start + 1] & 0xf000) >> 12;
			oy = aerofgt_spriteram3[attr_start + 0] & 0x01ff;
			ysize = (aerofgt_spriteram3[attr_start + 0] & 0x0e00) >> 9;
			zoomy = (aerofgt_spriteram3[attr_start + 0] & 0xf000) >> 12;
			flipx = aerofgt_spriteram3[attr_start + 2] & 0x4000;
			flipy = aerofgt_spriteram3[attr_start + 2] & 0x8000;
			color = (aerofgt_spriteram3[attr_start + 2] & 0x0f00) >> 8;
			map_start = aerofgt_spriteram3[attr_start + 3] & 0x3fff;

			ox += (xsize*zoomx+2)/4;
			oy += (ysize*zoomy+2)/4;

			zoomx = 32 - zoomx;
			zoomy = 32 - zoomy;

			for (y = 0;y <= ysize;y++)
			{
				int sx,sy;

				if (flipy) sy = ((oy + zoomy * (ysize - y)/2 + 16) & 0x1ff) - 16;
				else sy = ((oy + zoomy * y / 2 + 16) & 0x1ff) - 16;

				for (x = 0;x <= xsize;x++)
				{
					int code;

					if (flipx) sx = ((ox + zoomx * (xsize - x) / 2 + 16) & 0x1ff) - 16;
					else sx = ((ox + zoomx * x / 2 + 16) & 0x1ff) - 16;

					if (map_start < 0x2000)
						code = aerofgt_spriteram1[map_start & 0x1fff] & 0x1fff;
					else
						code = aerofgt_spriteram2[map_start & 0x1fff] & 0x1fff;

					drawgfxzoom(bitmap,Machine->gfx[sprite_gfx + (map_start >= 0x2000 ? 1 : 0)],
							code,
							color,
							flipx,flipy,
							sx,sy,
							cliprect,TRANSPARENCY_PEN,15,
							zoomx << 11, zoomy << 11);
					map_start++;
				}
			}
		}

		offs++;
	}
}

static void turbofrc_drawsprites(struct mame_bitmap *bitmap,const struct rectangle *cliprect,int chip, int chip_disabled_pri)
{
	int attr_start,base,first;


	base = chip * 0x0200;
	first = 4 * aerofgt_spriteram3[0x1fe + base];

	for (attr_start = base + 0x0200-8;attr_start >= first + base;attr_start -= 4)
	{
		int map_start;
		int ox,oy,x,y,xsize,ysize,zoomx,zoomy,flipx,flipy,color,pri;
/* some other drivers still use this wrong table, they have to be upgraded*/
/*		int zoomtable[16] = { 0,7,14,20,25,30,34,38,42,46,49,52,54,57,59,61 };*/

		if (!(aerofgt_spriteram3[attr_start + 2] & 0x0080)) continue;
		pri = aerofgt_spriteram3[attr_start + 2] & 0x0010;
		if ( chip_disabled_pri & !pri) continue;
		if (!chip_disabled_pri & (pri>>4)) continue;
		ox = aerofgt_spriteram3[attr_start + 1] & 0x01ff;
		xsize = (aerofgt_spriteram3[attr_start + 2] & 0x0700) >> 8;
		zoomx = (aerofgt_spriteram3[attr_start + 1] & 0xf000) >> 12;
		oy = aerofgt_spriteram3[attr_start + 0] & 0x01ff;
		ysize = (aerofgt_spriteram3[attr_start + 2] & 0x7000) >> 12;
		zoomy = (aerofgt_spriteram3[attr_start + 0] & 0xf000) >> 12;
		flipx = aerofgt_spriteram3[attr_start + 2] & 0x0800;
		flipy = aerofgt_spriteram3[attr_start + 2] & 0x8000;
		color = (aerofgt_spriteram3[attr_start + 2] & 0x000f) + 16 * spritepalettebank;

		map_start = aerofgt_spriteram3[attr_start + 3];

/* aerofgt has this adjustment, but doing it here would break turbo force title screen*/
/*		ox += (xsize*zoomx+2)/4;*/
/*		oy += (ysize*zoomy+2)/4;*/

		zoomx = 32 - zoomx;
		zoomy = 32 - zoomy;

		for (y = 0;y <= ysize;y++)
		{
			int sx,sy;

			if (flipy) sy = ((oy + zoomy * (ysize - y)/2 + 16) & 0x1ff) - 16;
			else sy = ((oy + zoomy * y / 2 + 16) & 0x1ff) - 16;

			for (x = 0;x <= xsize;x++)
			{
				int code;

				if (flipx) sx = ((ox + zoomx * (xsize - x) / 2 + 16) & 0x1ff) - 16;
				else sx = ((ox + zoomx * x / 2 + 16) & 0x1ff) - 16;

				if (chip == 0)
					code = aerofgt_spriteram1[map_start % (aerofgt_spriteram1_size/2)];
				else
					code = aerofgt_spriteram2[map_start % (aerofgt_spriteram2_size/2)];

				pdrawgfxzoom(bitmap,Machine->gfx[sprite_gfx + chip],
						code,
						color,
						flipx,flipy,
						sx,sy,
						cliprect,TRANSPARENCY_PEN,15,
						zoomx << 11,zoomy << 11,
						pri ? 0 : 2);
				map_start++;
			}

			if (xsize == 2) map_start += 1;
			if (xsize == 4) map_start += 3;
			if (xsize == 5) map_start += 2;
			if (xsize == 6) map_start += 1;
		}
	}
}

static void spinlbrk_draw_sprites(struct mame_bitmap *bitmap,const struct rectangle *cliprect,int chip, int chip_disabled_pri)
{
	int attr_start,base,first;


	base = chip * 0x0200;
	first = 4 * aerofgt_spriteram3[0x1fe + base];

	for (attr_start = base + 0x0200-8;attr_start >= first + base;attr_start -= 4)
	{
		int map_start;
		int ox,oy,x,y,xsize,ysize,zoomx,zoomy,flipx,flipy,color,pri;
/* some other drivers still use this wrong table, they have to be upgraded*/
/*		int zoomtable[16] = { 0,7,14,20,25,30,34,38,42,46,49,52,54,57,59,61 };*/

		if (!(aerofgt_spriteram3[attr_start + 2] & 0x0080)) continue;
		pri = aerofgt_spriteram3[attr_start + 2] & 0x0010;
		if ( chip_disabled_pri & !pri) continue;
		if (!chip_disabled_pri & (pri>>4)) continue;
		ox = aerofgt_spriteram3[attr_start + 1] & 0x01ff;
		xsize = (aerofgt_spriteram3[attr_start + 2] & 0x0700) >> 8;
		zoomx = (aerofgt_spriteram3[attr_start + 1] & 0xf000) >> 12;
		oy = aerofgt_spriteram3[attr_start + 0] & 0x01ff;
		ysize = (aerofgt_spriteram3[attr_start + 2] & 0x7000) >> 12;
		zoomy = (aerofgt_spriteram3[attr_start + 0] & 0xf000) >> 12;
		flipx = aerofgt_spriteram3[attr_start + 2] & 0x0800;
		flipy = aerofgt_spriteram3[attr_start + 2] & 0x8000;
		color = (aerofgt_spriteram3[attr_start + 2] & 0x000f) + 16 * spritepalettebank;

		map_start = aerofgt_spriteram3[attr_start + 3];

/* aerofgt has this adjustment, but doing it here would break turbo force title screen*/
/*		ox += (xsize*zoomx+2)/4;*/
/*		oy += (ysize*zoomy+2)/4;*/

		zoomx = 32 - zoomx;
		zoomy = 32 - zoomy;

		for (y = 0;y <= ysize;y++)
		{
			int sx,sy;

			if (flipy) sy = ((oy + zoomy * (ysize - y)/2 + 16) & 0x1ff) - 16;
			else sy = ((oy + zoomy * y / 2 + 16) & 0x1ff) - 16;

			for (x = 0;x <= xsize;x++)
			{
				int code;

				if (flipx) sx = ((ox + zoomx * (xsize - x) / 2 + 16) & 0x1ff) - 16;
				else sx = ((ox + zoomx * x / 2 + 16) & 0x1ff) - 16;

				if (chip == 0)
					code = aerofgt_spriteram1[map_start % (aerofgt_spriteram1_size/2)];
				else
					code = aerofgt_spriteram2[map_start % (aerofgt_spriteram2_size/2)];

				pdrawgfxzoom(bitmap,Machine->gfx[sprite_gfx + chip],
						code,
						color,
						flipx,flipy,
						sx,sy,
						cliprect,TRANSPARENCY_PEN,15,
						zoomx << 11,zoomy << 11,
						pri ? 2 : 0);
				map_start++;
			}

			if (xsize == 2) map_start += 1;
			if (xsize == 4) map_start += 3;
			if (xsize == 5) map_start += 2;
			if (xsize == 6) map_start += 1;
		}
	}
}

VIDEO_UPDATE( pspikes )
{
	int i,scrolly;

	tilemap_set_scroll_rows(bg1_tilemap,256);
	scrolly = bg1scrolly;
	for (i = 0;i < 256;i++)
		tilemap_set_scrollx(bg1_tilemap,(i + scrolly) & 0xff,aerofgt_rasterram[i]);
	tilemap_set_scrolly(bg1_tilemap,0,scrolly);

	fillbitmap(priority_bitmap,0,cliprect);

	tilemap_draw(bitmap,cliprect,bg1_tilemap,0,0);
	turbofrc_drawsprites(bitmap,cliprect,0,-1);
	turbofrc_drawsprites(bitmap,cliprect,0, 0);
}

VIDEO_UPDATE( karatblz )
{
	tilemap_set_scrollx(bg1_tilemap,0,bg1scrollx-8);
	tilemap_set_scrolly(bg1_tilemap,0,bg1scrolly);
	tilemap_set_scrollx(bg2_tilemap,0,bg2scrollx-4);
	tilemap_set_scrolly(bg2_tilemap,0,bg2scrolly);

	fillbitmap(priority_bitmap,0,cliprect);

	tilemap_draw(bitmap,cliprect,bg1_tilemap,0,0);
	tilemap_draw(bitmap,cliprect,bg2_tilemap,0,0);

	/* we use the priority buffer so sprites are drawn front to back */
	turbofrc_drawsprites(bitmap,cliprect,1,-1);
	turbofrc_drawsprites(bitmap,cliprect,1, 0);
	turbofrc_drawsprites(bitmap,cliprect,0,-1);
	turbofrc_drawsprites(bitmap,cliprect,0, 0);
}

VIDEO_UPDATE( spinlbrk )
{
	int i,scrolly;

	tilemap_set_scroll_rows(bg1_tilemap,512);
	scrolly = 0;
	for (i = 0;i < 256;i++)
		tilemap_set_scrollx(bg1_tilemap,(i + scrolly) & 0x1ff,aerofgt_rasterram[i]-8);
/*	tilemap_set_scrolly(bg1_tilemap,0,bg1scrolly);*/
	tilemap_set_scrollx(bg2_tilemap,0,bg2scrollx-4);
/*	tilemap_set_scrolly(bg2_tilemap,0,bg2scrolly);*/

	fillbitmap(priority_bitmap,0,cliprect);

	tilemap_draw(bitmap,cliprect,bg1_tilemap,0,0);
	tilemap_draw(bitmap,cliprect,bg2_tilemap,0,1);

	/* we use the priority buffer so sprites are drawn front to back */
	spinlbrk_draw_sprites(bitmap,cliprect,0, 0);
	spinlbrk_draw_sprites(bitmap,cliprect,0,-1);
	spinlbrk_draw_sprites(bitmap,cliprect,1, 0);
	spinlbrk_draw_sprites(bitmap,cliprect,1,-1);
}

VIDEO_UPDATE( turbofrc )
{
	int i,scrolly;

	tilemap_set_scroll_rows(bg1_tilemap,512);
	scrolly = bg1scrolly+2;
	for (i = 0;i < 256;i++)
/*		tilemap_set_scrollx(bg1_tilemap,(i + scrolly) & 0x1ff,aerofgt_rasterram[i]-11);*/
		tilemap_set_scrollx(bg1_tilemap,(i + scrolly) & 0x1ff,aerofgt_rasterram[7]-11);
	tilemap_set_scrolly(bg1_tilemap,0,scrolly);
	tilemap_set_scrollx(bg2_tilemap,0,bg2scrollx-7);
	tilemap_set_scrolly(bg2_tilemap,0,bg2scrolly+2);

	fillbitmap(priority_bitmap,0,cliprect);

	tilemap_draw(bitmap,cliprect,bg1_tilemap,0,0);
	tilemap_draw(bitmap,cliprect,bg2_tilemap,0,1);

	/* we use the priority buffer so sprites are drawn front to back */
	turbofrc_drawsprites(bitmap,cliprect,1,-1); /*ship*/
	turbofrc_drawsprites(bitmap,cliprect,1, 0); /*intro*/
	turbofrc_drawsprites(bitmap,cliprect,0,-1); /*enemy*/
	turbofrc_drawsprites(bitmap,cliprect,0, 0); /*enemy*/
}

VIDEO_UPDATE( aerofgt )
{
	tilemap_set_scrollx(bg1_tilemap,0,aerofgt_rasterram[0x0000]-18);
	tilemap_set_scrolly(bg1_tilemap,0,bg1scrolly);
	tilemap_set_scrollx(bg2_tilemap,0,aerofgt_rasterram[0x0200]-20);
	tilemap_set_scrolly(bg2_tilemap,0,bg2scrolly);

	fillbitmap(priority_bitmap,0,cliprect);

	tilemap_draw(bitmap,cliprect,bg1_tilemap,0,0);

	aerofgt_drawsprites(bitmap,cliprect,0);
	aerofgt_drawsprites(bitmap,cliprect,1);

	tilemap_draw(bitmap,cliprect,bg2_tilemap,0,0);

	aerofgt_drawsprites(bitmap,cliprect,2);
	aerofgt_drawsprites(bitmap,cliprect,3);
}
