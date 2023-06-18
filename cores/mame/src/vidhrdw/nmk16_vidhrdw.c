#include "driver.h"
#include "vidhrdw/generic.h"

data16_t *nmk_bgvideoram,*nmk_fgvideoram,*nmk_txvideoram;
data16_t *gunnail_scrollram, *gunnail_scrollramy;
data16_t tharrier_scroll;

static int redraw_bitmap;

static data16_t *spriteram_old,*spriteram_old2;
static int bgbank;
static int videoshift;
static int bioship_background_bank;
static UINT8 bioship_scroll[4];

extern data16_t *nmk16_mainram;

static struct tilemap *bg_tilemap,*fg_tilemap,*tx_tilemap;
static struct mame_bitmap *background_bitmap;

/***************************************************************************

  Callbacks for the TileMap code

***************************************************************************/

static UINT32 bg_scan(UINT32 col,UINT32 row,UINT32 num_cols,UINT32 num_rows)
{
	/* logical (col,row) -> memory offset */
	return (row & 0x0f) + ((col & 0xff) << 4) + ((row & 0x70) << 8);
}

static UINT32 bg_scan_td2(UINT32 col,UINT32 row,UINT32 num_cols,UINT32 num_rows)
{
	/* logical (col,row) -> memory offset */
	return (row & 0x0f) + ((col & 0x3ff) << 4) + ((row & 0x70) << 10);
}


static void macross_get_bg_tile_info(int tile_index)
{
	int code = nmk_bgvideoram[tile_index];
	SET_TILE_INFO(
			1,
			(code & 0xfff) + (bgbank << 12),
			code >> 12,
			0)
}

static void strahl_get_fg_tile_info(int tile_index)
{
	int code = nmk_fgvideoram[tile_index];
	SET_TILE_INFO(
			3,
			(code & 0xfff),
			code >> 12,
			0)
}

static void macross_get_tx_tile_info(int tile_index)
{
	int code = nmk_txvideoram[tile_index];
	SET_TILE_INFO(
			0,
			code & 0xfff,
			code >> 12,
			0)
}

static void bjtwin_get_bg_tile_info(int tile_index)
{
	int code = nmk_bgvideoram[tile_index];
	int bank = (code & 0x800) ? 1 : 0;
	SET_TILE_INFO(
			bank,
			(code & 0x7ff) + ((bank) ? (bgbank << 11) : 0),
			code >> 12,
			0)
}

static void get_tile_info_0_8bit(int tile_index)
{
	int code = nmk_bgvideoram[tile_index];
	SET_TILE_INFO(
			1,
			code,
			0,
			0);
}


/***************************************************************************

  Start the video hardware emulation.

***************************************************************************/

VIDEO_START( bioship )
{
	bg_tilemap = tilemap_create(macross_get_bg_tile_info,bg_scan,TILEMAP_TRANSPARENT,16,16,256,32);
	tx_tilemap = tilemap_create(macross_get_tx_tile_info,tilemap_scan_cols,TILEMAP_TRANSPARENT,8,8,32,32);
	spriteram_old = auto_malloc(0x1000);
	spriteram_old2 = auto_malloc(0x1000);
	background_bitmap = auto_bitmap_alloc(8192,512);

	if (!bg_tilemap || !spriteram_old || !spriteram_old2 || !background_bitmap)
		return 1;

	tilemap_set_transparent_pen(bg_tilemap,15);
	tilemap_set_transparent_pen(tx_tilemap,15);
	bioship_background_bank=0;
	redraw_bitmap = 1;

	memset(spriteram_old,0,0x1000);
	memset(spriteram_old2,0,0x1000);

	videoshift =  0;	/* 256x224 screen, no shift */

	return 0;
}

VIDEO_START( strahl )
{
	bg_tilemap = tilemap_create(macross_get_bg_tile_info,bg_scan,TILEMAP_OPAQUE,16,16,256,32);
	fg_tilemap = tilemap_create(strahl_get_fg_tile_info, bg_scan,TILEMAP_TRANSPARENT,16,16,256,32);
	tx_tilemap = tilemap_create(macross_get_tx_tile_info,tilemap_scan_cols,TILEMAP_TRANSPARENT,8,8,32,32);
	spriteram_old = auto_malloc(0x1000);
	spriteram_old2 = auto_malloc(0x1000);

	if (!bg_tilemap || !fg_tilemap || !spriteram_old || !spriteram_old2)
		return 1;

	tilemap_set_transparent_pen(fg_tilemap,15);
	tilemap_set_transparent_pen(tx_tilemap,15);

	memset(spriteram_old,0,0x1000);
	memset(spriteram_old2,0,0x1000);

	videoshift =  0;	/* 256x224 screen, no shift */
	background_bitmap = NULL;
	return 0;
}

VIDEO_START( macross )
{
	bg_tilemap = tilemap_create(macross_get_bg_tile_info,bg_scan,TILEMAP_OPAQUE,16,16,256,32);
	tx_tilemap = tilemap_create(macross_get_tx_tile_info,tilemap_scan_cols,TILEMAP_TRANSPARENT,8,8,32,32);
	spriteram_old = auto_malloc(0x1000);
	spriteram_old2 = auto_malloc(0x1000);

	if (!bg_tilemap || !spriteram_old || !spriteram_old2)
		return 1;

	tilemap_set_transparent_pen(tx_tilemap,15);

	memset(spriteram_old,0,0x1000);
	memset(spriteram_old2,0,0x1000);

	videoshift =  0;	/* 256x224 screen, no shift */
	background_bitmap = NULL;

	return 0;
}

VIDEO_START( gunnail )
{
	bg_tilemap = tilemap_create(macross_get_bg_tile_info,bg_scan,TILEMAP_OPAQUE,16,16,256,32);
	tx_tilemap = tilemap_create(macross_get_tx_tile_info,tilemap_scan_cols,TILEMAP_TRANSPARENT,8,8,64,32);
	spriteram_old = auto_malloc(0x1000);
	spriteram_old2 = auto_malloc(0x1000);

	if (!bg_tilemap || !spriteram_old || !spriteram_old2)
		return 1;

	tilemap_set_transparent_pen(tx_tilemap,15);
	tilemap_set_scroll_rows(bg_tilemap,512);

	memset(spriteram_old,0,0x1000);
	memset(spriteram_old2,0,0x1000);

	videoshift = 64;	/* 384x224 screen, leftmost 64 pixels have to be retrieved */
						/* from the other side of the tilemap (!) */
	background_bitmap = NULL;

	return 0;
}

VIDEO_START( macross2 )
{
	bg_tilemap = tilemap_create(macross_get_bg_tile_info,bg_scan,TILEMAP_OPAQUE,16,16,256,128);
	tx_tilemap = tilemap_create(macross_get_tx_tile_info,tilemap_scan_cols,TILEMAP_TRANSPARENT,8,8,64,32);
	spriteram_old = auto_malloc(0x1000);
	spriteram_old2 = auto_malloc(0x1000);

	if (!bg_tilemap || !spriteram_old || !spriteram_old2)
		return 1;

	tilemap_set_transparent_pen(tx_tilemap,15);

	memset(spriteram_old,0,0x1000);
	memset(spriteram_old2,0,0x1000);

	videoshift = 64;	/* 384x224 screen, leftmost 64 pixels have to be retrieved */
						/* from the other side of the tilemap (!) */
	background_bitmap = NULL;
	return 0;
}

VIDEO_START( tdragon2 )
{
	bg_tilemap = tilemap_create(macross_get_bg_tile_info,bg_scan_td2,TILEMAP_OPAQUE,16,16,1024,32);
	tx_tilemap = tilemap_create(macross_get_tx_tile_info,tilemap_scan_cols,TILEMAP_TRANSPARENT,8,8,64,32);
	spriteram_old = auto_malloc(0x1000);
	spriteram_old2 = auto_malloc(0x1000);

	if (!bg_tilemap || !spriteram_old || !spriteram_old2)
		return 1;

	tilemap_set_transparent_pen(tx_tilemap,15);

	memset(spriteram_old,0,0x1000);
	memset(spriteram_old2,0,0x1000);

	videoshift = 64;	/* 384x224 screen, leftmost 64 pixels have to be retrieved */
						/* from the other side of the tilemap (!) */
	background_bitmap = NULL;
	return 0;
}

VIDEO_START( bjtwin )
{
	bg_tilemap = tilemap_create(bjtwin_get_bg_tile_info,tilemap_scan_cols,TILEMAP_OPAQUE,8,8,64,32);
	spriteram_old = auto_malloc(0x1000);
	spriteram_old2 = auto_malloc(0x1000);

	if (!bg_tilemap || !spriteram_old || !spriteram_old2)
		return 1;

	memset(spriteram_old,0,0x1000);
	memset(spriteram_old2,0,0x1000);

	videoshift = 64;	/* 384x224 screen, leftmost 64 pixels have to be retrieved */
						/* from the other side of the tilemap (!) */
	background_bitmap = NULL;
	return 0;
}



/***************************************************************************

  Memory handlers

***************************************************************************/

READ16_HANDLER( nmk_bgvideoram_r )
{
	return nmk_bgvideoram[offset];
}

WRITE16_HANDLER( nmk_bgvideoram_w )
{
	int oldword = nmk_bgvideoram[offset];
	int newword = oldword;
	COMBINE_DATA(&newword);

	if (oldword != newword)
	{
		nmk_bgvideoram[offset] = newword;
		tilemap_mark_tile_dirty(bg_tilemap,offset);
	}
}

READ16_HANDLER( nmk_fgvideoram_r )
{
	return nmk_fgvideoram[offset];
}

WRITE16_HANDLER( nmk_fgvideoram_w )
{
	int oldword = nmk_fgvideoram[offset];
	int newword = oldword;
	COMBINE_DATA(&newword);

	if (oldword != newword)
	{
		nmk_fgvideoram[offset] = newword;
		tilemap_mark_tile_dirty(fg_tilemap,offset);
	}
}

READ16_HANDLER( nmk_txvideoram_r )
{
	return nmk_txvideoram[offset];
}

WRITE16_HANDLER( nmk_txvideoram_w )
{
	int oldword = nmk_txvideoram[offset];
	int newword = oldword;
	COMBINE_DATA(&newword);

	if (oldword != newword)
	{
		nmk_txvideoram[offset] = newword;
		tilemap_mark_tile_dirty(tx_tilemap,offset);
	}
}

WRITE16_HANDLER( mustang_scroll_w )
{
	static UINT8 scroll[4];

	if (ACCESSING_MSB)
	{
		scroll[offset] = (data >> 8) & 0xff;

		if (offset & 2)
			tilemap_set_scrolly(bg_tilemap,0,scroll[2] * 256 + scroll[3]);
		else
			tilemap_set_scrollx(bg_tilemap,0,scroll[0] * 256 + scroll[1] - videoshift);
	}
}

WRITE16_HANDLER( nmk_scroll_w )
{
	if (ACCESSING_LSB)
	{
		static UINT8 scroll[4];

		scroll[offset] = data & 0xff;

		if (offset & 2)
			tilemap_set_scrolly(bg_tilemap,0,scroll[2] * 256 + scroll[3]);
		else
			tilemap_set_scrollx(bg_tilemap,0,scroll[0] * 256 + scroll[1] - videoshift);
	}
}

WRITE16_HANDLER( nmk_scroll_2_w )
{
	if (ACCESSING_LSB)
	{
		static UINT8 scroll[4];

		scroll[offset] = data & 0xff;

		if (offset & 2)
			tilemap_set_scrolly(fg_tilemap,0,scroll[2] * 256 + scroll[3]);
		else
			tilemap_set_scrollx(fg_tilemap,0,scroll[0] * 256 + scroll[1] - videoshift);
	}
}

WRITE16_HANDLER( nmk_scroll_3_w )
{

	COMBINE_DATA(&gunnail_scrollram[offset]);

/*	usrintf_showmessage( "scroll %04x, %04x", gunnail_scrollram[0], gunnail_scrollram[0x100]);*/

	tilemap_set_scrollx(bg_tilemap,0,gunnail_scrollram[0]-videoshift);
	tilemap_set_scrolly(bg_tilemap,0,gunnail_scrollram[0x100]);
}


WRITE16_HANDLER( vandyke_scroll_w )
{
	static UINT16 scroll[4];

	scroll[offset] = data;

	tilemap_set_scrollx(bg_tilemap,0,scroll[0] * 256 + (scroll[1] >> 8));
	tilemap_set_scrolly(bg_tilemap,0,scroll[2] * 256 + (scroll[3] >> 8));
}

static int mustang_bg_xscroll;

WRITE16_HANDLER( twinactn_scroll_w )
{
/*  mame_printf_debug("mustang %04x %04x %04x\n",offset,data,mem_mask); */

	switch (data & 0xff00)
	{
		case 0x0000:
			mustang_bg_xscroll = (mustang_bg_xscroll & 0x00ff) | ((data & 0x00ff)<<8);
			break;

		case 0x0100:
			mustang_bg_xscroll = (mustang_bg_xscroll & 0xff00) | (data & 0x00ff);
			break;

		case 0x0200:
			break;

		case 0x0300:
			break;

		default:
			break;
	}

	tilemap_set_scrollx(bg_tilemap,0,mustang_bg_xscroll - videoshift);
}

WRITE16_HANDLER( nmk_flipscreen_w )
{
	if (ACCESSING_LSB)
		flip_screen_set(data & 0x01);
}

WRITE16_HANDLER( nmk_tilebank_w )
{
	if (ACCESSING_LSB)
	{
		if (bgbank != (data & 0xff))
		{
			bgbank = data & 0xff;
			tilemap_mark_all_tiles_dirty(bg_tilemap);
		}
	}
}

WRITE16_HANDLER( bioship_scroll_w )
{
	if (ACCESSING_MSB)
		bioship_scroll[offset]=data>>8;
}

WRITE16_HANDLER( bioship_bank_w )
{
	if (ACCESSING_LSB)
	{
		if (bioship_background_bank != data)
		{
			bioship_background_bank = data;
			redraw_bitmap=1;
		}
	}
}

/***************************************************************************

  Display refresh

***************************************************************************/

extern int is_blkheart;

/* manybloc uses extra flip bits on the sprites, but these break other games*/

static void nmk16_draw_sprites(struct mame_bitmap *bitmap, const struct rectangle *cliprect, int priority)
{
	int offs;

	for (offs = 0;offs < 0x1000/2;offs += 8)
	{
		if (spriteram_old2[offs] & 0x0001)
		{
			int sx = (spriteram_old2[offs+4] & 0x1ff) + videoshift;
			int sy = (spriteram_old2[offs+6] & 0x1ff);
			int code = spriteram_old2[offs+3];
			int color = spriteram_old2[offs+7];
			int w = (spriteram_old2[offs+1] & 0x0f);
			int h = ((spriteram_old2[offs+1] & 0xf0) >> 4);
			int pri = (spriteram_old2[offs] & 0xc0) >> 6;
			int xx,yy,x;
			int delta = 16;

			if (pri!=priority) continue;

			if (flip_screen)
			{
				sx = 368 - sx;
				sy = 240 - sy;
				delta = -16;
			}

			yy = h;
			do
			{
				x = sx;
				xx = w;
				do
				{
					drawgfx(bitmap,Machine->gfx[2],
							code,
							color,
							flip_screen, flip_screen,
							((x + 16) & 0x1ff) - 16,sy & 0x1ff,
							cliprect,TRANSPARENCY_PEN,15);

					code++;
					x += delta;
				} while (--xx >= 0);

				sy += delta;
			} while (--yy >= 0);
		}
	}
}

static void nmk16_draw_sprites_flipsupported(struct mame_bitmap *bitmap, const struct rectangle *cliprect, int priority)
{
	int offs;

	for (offs = 0;offs < 0x1000/2;offs += 8)
	{
		if (spriteram_old2[offs] & 0x0001)
		{
			int sx = (spriteram_old2[offs+4] & 0x1ff) + videoshift;
			int sy = (spriteram_old2[offs+6] & 0x1ff);
			int code = spriteram_old2[offs+3];
			int color = spriteram_old2[offs+7];
			int w = (spriteram_old2[offs+1] & 0x0f);
			int h = ((spriteram_old2[offs+1] & 0xf0) >> 4);
			int pri = (spriteram_old2[offs] & 0xc0) >> 6;
			int flipy= ((spriteram_old2[offs+1] & 0x0200) >> 9);
			int flipx = ((spriteram_old2[offs+1] & 0x0100) >> 8);

			int xx,yy,x;
			int delta = 16;

			if(pri != priority)
				continue;

			if (flip_screen)
			{
				flipx ^= 1;
				flipy ^= 1;
				sx = 368 - sx;
				sy = 240 - sy;
				delta = -16;
			}

			yy = h;
			sy+=flipy?(delta*h):0;
			do
			{
				x = sx+(flipx?(delta*w):0);


				xx = w;
				do
				{
					drawgfx(bitmap,Machine->gfx[2],
							code,
							color,
							flipx, flipy,
							((x + 16) & 0x1ff) - 16,sy & 0x1ff,
							cliprect,TRANSPARENCY_PEN,15);
					code++;
					x +=delta * ( flipx?-1:1 );


				} while (--xx >= 0);
				sy += delta * ( flipy?-1:1);

			} while (--yy >= 0);
		}
	}
}

VIDEO_UPDATE( macross )
{
	tilemap_set_scrollx(tx_tilemap,0,-videoshift);

	tilemap_draw(bitmap,cliprect,bg_tilemap,0,0);

	nmk16_draw_sprites(bitmap,cliprect,3);
	nmk16_draw_sprites(bitmap,cliprect,2);
	nmk16_draw_sprites(bitmap,cliprect,1);
	nmk16_draw_sprites(bitmap,cliprect,0);

	tilemap_draw(bitmap,cliprect,tx_tilemap,0,0);
}

VIDEO_UPDATE( manybloc )
{
	tilemap_set_scrollx(tx_tilemap,0,-videoshift);

	tilemap_draw(bitmap,cliprect,bg_tilemap,0,0);

	nmk16_draw_sprites_flipsupported(bitmap,cliprect,3);
	nmk16_draw_sprites_flipsupported(bitmap,cliprect,2);
	nmk16_draw_sprites_flipsupported(bitmap,cliprect,1);
	nmk16_draw_sprites_flipsupported(bitmap,cliprect,0);

	tilemap_draw(bitmap,cliprect,tx_tilemap,0,0);
}

VIDEO_UPDATE( tharrier )
{
	tharrier_scroll = nmk16_mainram[0x9f00/2];

	tilemap_set_scrollx(tx_tilemap,0,-videoshift);
	tilemap_set_scrollx(bg_tilemap,0,tharrier_scroll);

	tilemap_draw(bitmap,cliprect,bg_tilemap,0,0);

	nmk16_draw_sprites_flipsupported(bitmap,cliprect,3);
	nmk16_draw_sprites_flipsupported(bitmap,cliprect,2);
	nmk16_draw_sprites_flipsupported(bitmap,cliprect,1);
	nmk16_draw_sprites_flipsupported(bitmap,cliprect,0);

	tilemap_draw(bitmap,cliprect,tx_tilemap,0,0);
}

VIDEO_UPDATE( gunnail )
{
	int y1, i;
    struct	rectangle bgclip = *cliprect;

	/* the hardware supports per-scanline X *and* Y scroll which isn't
	   supported by tilemaps so we have to draw the tilemap one line at a time */
	y1 = cliprect->min_y;
	while (y1 <= cliprect->max_y)
	{
		int const yscroll = gunnail_scrollramy[0] + gunnail_scrollramy[y1];
		int y2;

		/* group all consecutive lines with the same y scroll to reduce overhead */
		y2 = y1+1;
		while (y2 <= cliprect->max_y && gunnail_scrollramy[y2] == gunnail_scrollramy[y1])
			y2++;

		bgclip.min_y = y1;
		bgclip.max_y = y2-1;

		tilemap_set_scrolly(bg_tilemap, 0, yscroll);
		for (i = y1; i < y2; i++)
			tilemap_set_scrollx(bg_tilemap,(i + yscroll) & 0x1ff, gunnail_scrollram[0] + gunnail_scrollram[i] - videoshift);

		tilemap_draw(bitmap,&bgclip,bg_tilemap,0,0);

		y1 = y2;
	}

	nmk16_draw_sprites(bitmap,cliprect,3);
	nmk16_draw_sprites(bitmap,cliprect,2);
	nmk16_draw_sprites(bitmap,cliprect,1);
	nmk16_draw_sprites(bitmap,cliprect,0);

	tilemap_set_scrollx(tx_tilemap,0,-videoshift);
	tilemap_draw(bitmap,cliprect,tx_tilemap,0,0);
}

VIDEO_UPDATE( bioship )
{
	data16_t *tilerom = (data16_t *)memory_region(REGION_GFX5);
	int scrollx=-(bioship_scroll[1] + bioship_scroll[0]*256);
	int scrolly=-(bioship_scroll[3] + bioship_scroll[2]*256);

	tilemap_set_scrollx(tx_tilemap,0,-videoshift);

	if (redraw_bitmap)
	{
		int bank = bioship_background_bank * 0x2000;
		int sx=0, sy=0, offs;
		redraw_bitmap=0;

		/* Draw background from tile rom */
		for (offs = 0;offs <0x1000;offs++) {
				data16_t data = tilerom[offs+bank];
				int numtile = data&0xfff;
				int color = (data&0xf000)>>12;

				drawgfx(background_bitmap,Machine->gfx[3],
						numtile,
						color,
						0,0,   /* no flip */
						16*sx,16*sy,
						0,TRANSPARENCY_NONE,0);

				data = tilerom[offs+0x1000+bank];
				numtile = data&0xfff;
				color = (data&0xf000)>>12;
				drawgfx(background_bitmap,Machine->gfx[3],
						numtile,
						color,
						0,0,   /* no flip */
						16*sx,(16*sy)+256,
						0,TRANSPARENCY_NONE,0);

				sy++;
				if (sy==16) {sy=0; sx++;}
		}
	}

	copyscrollbitmap(bitmap,background_bitmap,1,&scrollx,1,&scrolly,cliprect,TRANSPARENCY_NONE,0);
	tilemap_draw(bitmap,cliprect,bg_tilemap,0,0);

	nmk16_draw_sprites(bitmap,cliprect,3);
	nmk16_draw_sprites(bitmap,cliprect,2);
	nmk16_draw_sprites(bitmap,cliprect,1);
	nmk16_draw_sprites(bitmap,cliprect,0);

	tilemap_draw(bitmap,cliprect,tx_tilemap,0,0);
}

VIDEO_UPDATE( strahl )
{
	tilemap_set_scrollx(tx_tilemap,0,-videoshift);

	tilemap_draw(bitmap,cliprect,bg_tilemap,0,0);
	tilemap_draw(bitmap,cliprect,fg_tilemap,0,0);

	nmk16_draw_sprites(bitmap,cliprect,3);
	nmk16_draw_sprites(bitmap,cliprect,2);
	nmk16_draw_sprites(bitmap,cliprect,1);
	nmk16_draw_sprites(bitmap,cliprect,0);

	tilemap_draw(bitmap,cliprect,tx_tilemap,0,0);
}

VIDEO_UPDATE( bjtwin )
{
	tilemap_set_scrollx(bg_tilemap,0,-videoshift);

	tilemap_draw(bitmap,cliprect,bg_tilemap,0,0);

	nmk16_draw_sprites(bitmap,cliprect,3);
	nmk16_draw_sprites(bitmap,cliprect,2);
	nmk16_draw_sprites(bitmap,cliprect,1);
	nmk16_draw_sprites(bitmap,cliprect,0);
}

VIDEO_UPDATE( hachamf )
{
	tilemap_set_scrollx(tx_tilemap,0,-videoshift);

	tilemap_draw(bitmap,cliprect,bg_tilemap,0,0);

	nmk16_draw_sprites(bitmap,cliprect,3);
	nmk16_draw_sprites(bitmap,cliprect,2);
	nmk16_draw_sprites(bitmap,cliprect,1);
	nmk16_draw_sprites(bitmap,cliprect,0);

	tilemap_draw(bitmap,cliprect,tx_tilemap,0,0);
}

VIDEO_EOF( nmk )
{
	/* looks like sprites are *two* frames ahead */
	memcpy(spriteram_old2,nmk16_mainram+0x8000/2,0x1000);
}

VIDEO_EOF( strahl )
{
	/* looks like sprites are *two* frames ahead */
	memcpy(spriteram_old2,nmk16_mainram+0xf000/2,0x1000);
}
