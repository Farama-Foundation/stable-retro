#include "driver.h"
#include "vidhrdw/konamiic.h"
#include "f1gp.h"


data16_t *f1gp_spr1vram,*f1gp_spr2vram,*f1gp_spr1cgram,*f1gp_spr2cgram;
data16_t *f1gp_fgvideoram,*f1gp_rozvideoram;
data16_t *f1gp2_sprcgram,*f1gp2_spritelist;
size_t f1gp_spr1cgram_size,f1gp_spr2cgram_size;

static data16_t *zoomdata;
static int dirtygfx,flipscreen,gfxctrl;
static unsigned char *dirtychar;

#define TOTAL_CHARS 0x800

static struct tilemap *fg_tilemap,*roz_tilemap;
static int f1gp2_roz_bank;


/***************************************************************************

  Callbacks for the TileMap code

***************************************************************************/

static void f1gp_get_roz_tile_info(int tile_index)
{
	int code = f1gp_rozvideoram[tile_index];

	SET_TILE_INFO(3,code & 0x7ff,code >> 12,0)
}

static void f1gp2_get_roz_tile_info(int tile_index)
{
	int code = f1gp_rozvideoram[tile_index];

	SET_TILE_INFO(2,(code & 0x7ff) + (f1gp2_roz_bank << 11),code >> 12,0)
}

static void get_fg_tile_info(int tile_index)
{
	int code = f1gp_fgvideoram[tile_index];

	SET_TILE_INFO(0,code & 0x7fff,0,(code & 0x8000)?TILE_FLIPY:0)
}


/***************************************************************************

  Start the video hardware emulation.

***************************************************************************/

VIDEO_START( f1gp )
{
	roz_tilemap = tilemap_create(f1gp_get_roz_tile_info,tilemap_scan_rows,TILEMAP_OPAQUE,     16,16,64,64);
	fg_tilemap =  tilemap_create(get_fg_tile_info,      tilemap_scan_rows,TILEMAP_TRANSPARENT, 8, 8,64,32);

	if (!fg_tilemap || !roz_tilemap)
		return 1;

	K053936_wraparound_enable(0, 1);
	K053936_set_offset(0, -58, -2);

	tilemap_set_transparent_pen(fg_tilemap,0xff);

	if (!(dirtychar = auto_malloc(TOTAL_CHARS)))
		return 1;
	memset(dirtychar,1,TOTAL_CHARS);

	zoomdata = (data16_t *)memory_region(REGION_GFX4);

	return 0;
}

VIDEO_START( f1gp2 )
{
	roz_tilemap = tilemap_create(f1gp2_get_roz_tile_info,tilemap_scan_rows,TILEMAP_TRANSPARENT,16,16,64,64);
	fg_tilemap =  tilemap_create(get_fg_tile_info,       tilemap_scan_rows,TILEMAP_TRANSPARENT, 8, 8,64,32);

	if (!fg_tilemap || !roz_tilemap)
		return 1;

	K053936_wraparound_enable(0, 1);
	K053936_set_offset(0, -48, -21);

	tilemap_set_transparent_pen(fg_tilemap,0xff);
	tilemap_set_transparent_pen(roz_tilemap,0x0f);

	tilemap_set_scrolldx(fg_tilemap,-80,0);
	tilemap_set_scrolldy(fg_tilemap,-26,0);

	if (!(dirtychar = auto_malloc(TOTAL_CHARS)))
		return 1;
	memset(dirtychar,1,TOTAL_CHARS);

	zoomdata = (data16_t *)memory_region(REGION_GFX4);

	return 0;
}


/***************************************************************************

  Memory handlers

***************************************************************************/

READ16_HANDLER( f1gp_zoomdata_r )
{
	return zoomdata[offset];
}

WRITE16_HANDLER( f1gp_zoomdata_w )
{
	int oldword = zoomdata[offset];
	COMBINE_DATA(&zoomdata[offset]);
	if (oldword != zoomdata[offset])
	{
		dirtygfx = 1;
		dirtychar[offset / 64] = 1;
	}
}

READ16_HANDLER( f1gp_rozvideoram_r )
{
	return f1gp_rozvideoram[offset];
}

WRITE16_HANDLER( f1gp_rozvideoram_w )
{
	int oldword = f1gp_rozvideoram[offset];
	COMBINE_DATA(&f1gp_rozvideoram[offset]);
	if (oldword != f1gp_rozvideoram[offset])
		tilemap_mark_tile_dirty(roz_tilemap,offset);
}

WRITE16_HANDLER( f1gp_fgvideoram_w )
{
	int oldword = f1gp_fgvideoram[offset];
	COMBINE_DATA(&f1gp_fgvideoram[offset]);
	if (oldword != f1gp_fgvideoram[offset])
		tilemap_mark_tile_dirty(fg_tilemap,offset);
}

WRITE16_HANDLER( f1gp_fgscroll_w )
{
	static int scroll[2];

	COMBINE_DATA(&scroll[offset]);

	tilemap_set_scrollx(fg_tilemap,0,scroll[0]);
	tilemap_set_scrolly(fg_tilemap,0,scroll[1]);
}

WRITE16_HANDLER( f1gp_gfxctrl_w )
{
	if (ACCESSING_LSB)
	{
		flipscreen = data & 0x20;
		gfxctrl = data & 0xdf;
	}
}

WRITE16_HANDLER( f1gp2_gfxctrl_w )
{
	if (ACCESSING_LSB)
	{
		flipscreen = data & 0x20;

		/* bit 0/1 = fg/sprite/roz priority */
		/* bit 2 = blank screen */

		gfxctrl = data & 0xdf;
	}

	if (ACCESSING_MSB)
	{
		if (f1gp2_roz_bank != (data >> 8))
		{
			f1gp2_roz_bank = (data >> 8);
			tilemap_mark_all_tiles_dirty(roz_tilemap);
		}
	}
}


/***************************************************************************

  Display refresh

***************************************************************************/

static void f1gp_drawsprites(struct mame_bitmap *bitmap,const struct rectangle *cliprect,int chip,int primask)
{
	int attr_start,first;
	data16_t *spram = chip ? f1gp_spr2vram : f1gp_spr1vram;

	first = 4 * spram[0x1fe];

	for (attr_start = 0x0200-8;attr_start >= first;attr_start -= 4)
	{
		int map_start;
		int ox,oy,x,y,xsize,ysize,zoomx,zoomy,flipx,flipy,color,pri;
		/* table hand made by looking at the ship explosion in attract mode */
		/* it's almost a logarithmic scale but not exactly */
		int zoomtable[16] = { 0,7,14,20,25,30,34,38,42,46,49,52,54,57,59,61 };

		if (!(spram[attr_start + 2] & 0x0080)) continue;

		ox = spram[attr_start + 1] & 0x01ff;
		xsize = (spram[attr_start + 2] & 0x0700) >> 8;
		zoomx = (spram[attr_start + 1] & 0xf000) >> 12;
		oy = spram[attr_start + 0] & 0x01ff;
		ysize = (spram[attr_start + 2] & 0x7000) >> 12;
		zoomy = (spram[attr_start + 0] & 0xf000) >> 12;
		flipx = spram[attr_start + 2] & 0x0800;
		flipy = spram[attr_start + 2] & 0x8000;
		color = (spram[attr_start + 2] & 0x000f);/* + 16 * spritepalettebank;*/
		pri = 0;/*spram[attr_start + 2] & 0x0010;*/
		map_start = spram[attr_start + 3];

		zoomx = 16 - zoomtable[zoomx]/8;
		zoomy = 16 - zoomtable[zoomy]/8;

		for (y = 0;y <= ysize;y++)
		{
			int sx,sy;

			if (flipy) sy = ((oy + zoomy * (ysize - y) + 16) & 0x1ff) - 16;
			else sy = ((oy + zoomy * y + 16) & 0x1ff) - 16;

			for (x = 0;x <= xsize;x++)
			{
				int code;

				if (flipx) sx = ((ox + zoomx * (xsize - x) + 16) & 0x1ff) - 16;
				else sx = ((ox + zoomx * x + 16) & 0x1ff) - 16;

				if (chip == 0)
					code = f1gp_spr1cgram[map_start % (f1gp_spr1cgram_size/2)];
				else
					code = f1gp_spr2cgram[map_start % (f1gp_spr2cgram_size/2)];

				pdrawgfxzoom(bitmap,Machine->gfx[1 + chip],
						code,
						color,
						flipx,flipy,
						sx,sy,
						cliprect,TRANSPARENCY_PEN,15,
						0x1000 * zoomx,0x1000 * zoomy,
/*						pri ? 0 : 0x2);*/
						primask);
				map_start++;
			}

			if (xsize == 2) map_start += 1;
			if (xsize == 4) map_start += 3;
			if (xsize == 5) map_start += 2;
			if (xsize == 6) map_start += 1;
		}
	}
}


VIDEO_UPDATE( f1gp )
{
	static struct GfxLayout tilelayout =
	{
		16,16,
		TOTAL_CHARS,
		4,
		{ 0, 1, 2, 3 },
#ifdef MSB_FIRST
		{ 0*4, 1*4, 2*4, 3*4, 4*4, 5*4, 6*4, 7*4,
				8*4, 9*4, 10*4, 11*4, 12*4, 13*4, 14*4, 15*4 },
#else
		{ 2*4, 3*4, 0*4, 1*4, 6*4, 7*4, 4*4, 5*4,
				10*4, 11*4, 8*4, 9*4, 14*4, 15*4, 12*4, 13*4 },
#endif
		{ 0*64, 1*64, 2*64, 3*64, 4*64, 5*64, 6*64, 7*64,
				8*64, 9*64, 10*64, 11*64, 12*64, 13*64, 14*64, 15*64 },
		64*16,
	};


	if (dirtygfx)
	{
		int i;

		dirtygfx = 0;

		for (i = 0;i < TOTAL_CHARS;i++)
		{
			if (dirtychar[i])
			{
				dirtychar[i] = 0;
				decodechar(Machine->gfx[3],i,(UINT8 *)zoomdata,&tilelayout);
			}
		}

		tilemap_mark_all_tiles_dirty(roz_tilemap);
	}



	fillbitmap(priority_bitmap, 0, cliprect);

	K053936_0_zoom_draw(bitmap,cliprect,roz_tilemap,0,0);

	tilemap_draw(bitmap,cliprect,fg_tilemap,0,1);

	/* quick kludge for "continue" screen priority */
	if (gfxctrl == 0x00)
	{
		f1gp_drawsprites(bitmap,cliprect,0,0x02);
		f1gp_drawsprites(bitmap,cliprect,1,0x02);
	}
	else
	{
		f1gp_drawsprites(bitmap,cliprect,0,0x00);
		f1gp_drawsprites(bitmap,cliprect,1,0x02);
	}
}



static void f1gp2_drawsprites(struct mame_bitmap *bitmap,const struct rectangle *cliprect)
{
	int offs;


	offs = 0;
	while (offs < 0x0400 && (f1gp2_spritelist[offs] & 0x4000) == 0)
	{
		int attr_start;
		int map_start;
		int ox,oy,x,y,xsize,ysize,zoomx,zoomy,flipx,flipy,color;

		attr_start = 4 * (f1gp2_spritelist[offs++] & 0x01ff);

		ox = f1gp2_spritelist[attr_start + 1] & 0x01ff;
		xsize = (f1gp2_spritelist[attr_start + 1] & 0x0e00) >> 9;
		zoomx = (f1gp2_spritelist[attr_start + 1] & 0xf000) >> 12;
		oy = f1gp2_spritelist[attr_start + 0] & 0x01ff;
		ysize = (f1gp2_spritelist[attr_start + 0] & 0x0e00) >> 9;
		zoomy = (f1gp2_spritelist[attr_start + 0] & 0xf000) >> 12;
		flipx = f1gp2_spritelist[attr_start + 2] & 0x4000;
		flipy = f1gp2_spritelist[attr_start + 2] & 0x8000;
		color = (f1gp2_spritelist[attr_start + 2] & 0x1f00) >> 8;
		map_start = f1gp2_spritelist[attr_start + 3] & 0x7fff;

/* aerofgt has the following adjustment, but doing it here would break the title screen*/
/*		ox += (xsize*zoomx+2)/4;*/
/*		oy += (ysize*zoomy+2)/4;*/

		zoomx = 32 - zoomx;
		zoomy = 32 - zoomy;

		if (f1gp2_spritelist[attr_start + 2] & 0x20ff) color = rand();

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

				code = f1gp2_sprcgram[map_start & 0x3fff];
				map_start++;

				if (flipscreen)
					drawgfxzoom(bitmap,Machine->gfx[1],
							code,
							color,
							!flipx,!flipy,
							304-sx,208-sy,
							cliprect,TRANSPARENCY_PEN,15,
							zoomx << 11,zoomy << 11);
				else
					drawgfxzoom(bitmap,Machine->gfx[1],
							code,
							color,
							flipx,flipy,
							sx,sy,
							cliprect,TRANSPARENCY_PEN,15,
							zoomx << 11,zoomy << 11);
			}
		}
	}
}


VIDEO_UPDATE( f1gp2 )
{
	if (gfxctrl & 4)	/* blank screen */
	{
		fillbitmap(bitmap, get_black_pen(), cliprect);
	}
	else
	{
		switch (gfxctrl & 3)
		{
			case 0:
				K053936_0_zoom_draw(bitmap,cliprect,roz_tilemap,TILEMAP_IGNORE_TRANSPARENCY,0);
				f1gp2_drawsprites(bitmap,cliprect);
				tilemap_draw(bitmap,cliprect,fg_tilemap,0,0);
				break;
			case 1:
				K053936_0_zoom_draw(bitmap,cliprect,roz_tilemap,TILEMAP_IGNORE_TRANSPARENCY,0);
				tilemap_draw(bitmap,cliprect,fg_tilemap,0,0);
				f1gp2_drawsprites(bitmap,cliprect);
				break;
			case 2:
				tilemap_draw(bitmap,cliprect,fg_tilemap,TILEMAP_IGNORE_TRANSPARENCY,0);
				K053936_0_zoom_draw(bitmap,cliprect,roz_tilemap,0,0);
				f1gp2_drawsprites(bitmap,cliprect);
				break;
#ifdef MAME_DEBUG
			case 3:
				usrintf_showmessage("unsupported priority 3\n");
#endif
		}
	}
}
