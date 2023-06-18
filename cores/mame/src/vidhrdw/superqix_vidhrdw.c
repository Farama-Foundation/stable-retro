/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

static int gfxbank;
static UINT8 *superqix_bitmapram,*superqix_bitmapram2,*superqix_bitmapram_dirty,*superqix_bitmapram2_dirty;
static struct mame_bitmap *tmpbitmap2;
int sqix_minx,sqix_maxx,sqix_miny,sqix_maxy;
int sqix_last_bitmap;
int sqix_current_bitmap;

static struct tilemap *bg_tilemap;

WRITE_HANDLER( superqix_videoram_w )
{
	if (videoram[offset] != data)
	{
		videoram[offset] = data;
		tilemap_mark_tile_dirty(bg_tilemap, offset);
	}
}

WRITE_HANDLER( superqix_colorram_w )
{
	if (colorram[offset] != data)
	{
		colorram[offset] = data;
		tilemap_mark_tile_dirty(bg_tilemap, offset);
	}
}

READ_HANDLER( superqix_bitmapram_r )
{
	return superqix_bitmapram[offset];
}

WRITE_HANDLER( superqix_bitmapram_w )
{
	if (data != superqix_bitmapram[offset])
	{
		int x,y;
		superqix_bitmapram[offset] = data;
		superqix_bitmapram_dirty[offset] = 1;
		x=offset%128;
		y=offset/128;
		if(x<sqix_minx) sqix_minx=x;
		if(x>sqix_maxx) sqix_maxx=x;
		if(y<sqix_miny) sqix_miny=y;
		if(y>sqix_maxy) sqix_maxy=y;
	}
}

READ_HANDLER( superqix_bitmapram2_r )
{
	return superqix_bitmapram2[offset];
}

WRITE_HANDLER( superqix_bitmapram2_w )
{
	if (data != superqix_bitmapram2[offset])
	{
		int x,y;
		superqix_bitmapram2[offset] = data;
		superqix_bitmapram2_dirty[offset] = 1;
		x=offset%128;
		y=offset/128;
		if(x<sqix_minx) sqix_minx=x;
		if(x>sqix_maxx) sqix_maxx=x;
		if(y<sqix_miny) sqix_miny=y;
		if(y>sqix_maxy) sqix_maxy=y;
	}
}

WRITE_HANDLER( superqix_0410_w )
{
	int bankaddress;
	UINT8 *RAM = memory_region(REGION_CPU1);

	/* bits 0-1 select the tile bank */
	if (gfxbank != (data & 0x03))
	{
		gfxbank = data & 0x03;
		tilemap_mark_all_tiles_dirty (bg_tilemap);
	}

	/* bit 2 controls bitmap 1/2 */
	sqix_current_bitmap=data&4;
	if(sqix_current_bitmap !=sqix_last_bitmap)
	{
		sqix_last_bitmap=sqix_current_bitmap;
		memset(superqix_bitmapram_dirty,1,0x7000);
		memset(superqix_bitmapram2_dirty,1,0x7000);
		sqix_minx=0;sqix_maxx=127;sqix_miny=0;sqix_maxy=223;
	}

	/* bit 3 enables NMI */
	interrupt_enable_w(offset,data & 0x08);

	/* bits 4-5 control ROM bank */
	bankaddress = 0x10000 + ((data & 0x30) >> 4) * 0x4000;
	cpu_setbank(1,&RAM[bankaddress]);
}

WRITE_HANDLER( superqix_flipscreen_w )
{
	flip_screen_set(!data);
}

static void get_bg_tile_info(int tile_index)
{
	int attr = colorram[tile_index];
	int bank = (attr & 0x04) ? 0 : (1 + gfxbank);
	int code = videoram[tile_index] + 256 * (attr & 0x03);
	int color = (attr & 0xf0) >> 4;

	tile_info.priority = (attr & 0x08) >> 3;

	SET_TILE_INFO(bank, code, color, 0)
}

VIDEO_START( superqix )
{
	/* palette RAM is accessed thorough I/O ports, so we have to */
	/* allocate it ourselves */
	if ((paletteram = auto_malloc(256 * sizeof(UINT8))) == 0)
		return 1;

	if ((superqix_bitmapram = auto_malloc(0x7000 * sizeof(UINT8))) == 0)
		return 1;

	if ((superqix_bitmapram2 = auto_malloc(0x7000 * sizeof(UINT8))) == 0)
		return 1;

	if ((superqix_bitmapram_dirty = auto_malloc(0x7000 * sizeof(UINT8))) == 0)
		return 1;

	memset(superqix_bitmapram_dirty,1,0x7000);

	if ((superqix_bitmapram2_dirty = auto_malloc(0x7000 * sizeof(UINT8))) == 0)
		return 1;

	memset(superqix_bitmapram2_dirty,1,0x7000);

	if ((tmpbitmap2 = auto_bitmap_alloc(256, 256)) == 0)
		return 1;

	sqix_minx=0;sqix_maxx=127;sqix_miny=0;sqix_maxy=223;
	sqix_last_bitmap=0;

	bg_tilemap = tilemap_create(get_bg_tile_info, tilemap_scan_rows, 
		TILEMAP_TRANSPARENT, 8, 8, 32, 32);
	
	if (!bg_tilemap)
		return 1;

	tilemap_set_transparent_pen(bg_tilemap, 0);

	return 0;
}

static void superqix_draw_bitmap( struct mame_bitmap *bitmap )
{
	int i;
	UINT8 pens[16];

	pens[0]=0;

	for (i=1; i<16; i++)
		pens[i]=Machine->pens[i];

	if (sqix_current_bitmap==0)		/* Bitmap 1 */
	{
		int x,y;

		for (y = sqix_miny;y <= sqix_maxy;y++)
		{
			for (x = sqix_minx;x <= sqix_maxx;x++)
			{
				int sx,sy,d;

				if (superqix_bitmapram_dirty[y*128+x])
				{
					superqix_bitmapram_dirty[y*128+x]=0;
					d = superqix_bitmapram[y*128+x];

					sx = 2*x;
					sy = y+16;

					plot_pixel(tmpbitmap2, sx    , sy, pens[d >> 4]);
					plot_pixel(tmpbitmap2, sx + 1, sy, pens[d & 0x0f]);
				}
			}
		}
	}
	else		/* Bitmap 2 */
	{
		int x,y;

		for (y = sqix_miny;y <= sqix_maxy;y++)
		{
			for (x = sqix_minx;x <= sqix_maxx;x++)
			{
				int sx,sy,d;

				if (superqix_bitmapram2_dirty[y*128+x])
				{
					superqix_bitmapram2_dirty[y*128+x]=0;
					d = superqix_bitmapram2[y*128+x];

					sx = 2*x;
					sy = y+16;

					plot_pixel(tmpbitmap2, sx    , sy, pens[d >> 4]);
					plot_pixel(tmpbitmap2, sx + 1, sy, pens[d & 0x0f]);
				}
			}
		}
	}

	copybitmap(bitmap,tmpbitmap2,flip_screen,flip_screen,0,0,&Machine->visible_area,TRANSPARENCY_PEN,0);
}

static void superqix_draw_sprites( struct mame_bitmap *bitmap )
{
	int offs;

	for (offs = 0; offs < spriteram_size; offs += 4)
	{
		int attr = spriteram[offs + 3];
		int code = spriteram[offs] + 256 * (attr & 0x01);
		int color = (attr & 0xf0) >> 4;
		int flipx = attr & 0x04;
		int flipy = attr & 0x08;
		int sx = spriteram[offs + 1];
		int sy = spriteram[offs + 2];

		if (flip_screen)
		{
			sx = 240 - sx;
			sy = 240 - sy;
			flipx = !flipx;
			flipy = !flipy;
		}

		drawgfx(bitmap,Machine->gfx[5],	code, color, flipx, flipy, sx, sy,
				&Machine->visible_area, TRANSPARENCY_PEN, 0);
	}
}

VIDEO_UPDATE( superqix )
{
	fillbitmap(bitmap, get_black_pen(), cliprect);
	tilemap_draw(bitmap, cliprect, bg_tilemap, 0, 0);
	superqix_draw_bitmap(bitmap);
	superqix_draw_sprites(bitmap);
	tilemap_draw(bitmap, cliprect, bg_tilemap, 1, 0);

	sqix_minx=1000;sqix_maxx=-1;sqix_miny=1000;sqix_maxy=-1;
}
