/************************************************************************

	Quiz Panicuru Fantasy video hardware

************************************************************************/

#include "driver.h"

data16_t *quizpani_bg_videoram, *quizpani_txt_videoram;
data16_t *quizpani_scrollreg;

static struct tilemap *bg_tilemap, *txt_tilemap;

static int quizpani_bgbank = 0, quizpani_txtbank = 0;

static UINT32 bg_scan(UINT32 col,UINT32 row,UINT32 num_cols,UINT32 num_rows)
{
	/* logical (col,row) -> memory offset */
	return (row & 0x0f) + ((col & 0xff) << 4) + ((row & 0x70) << 8);
}

static void bg_tile_info(int tile_index)
{
	int code = quizpani_bg_videoram[tile_index];

	SET_TILE_INFO(
			1,
			(code & 0xfff) + (0x1000 * quizpani_bgbank),
			code >> 12,
			0)
}

static void txt_tile_info(int tile_index)
{
	int code = quizpani_txt_videoram[tile_index];

	SET_TILE_INFO(
			0,
			(code & 0xfff) + (0x1000 * quizpani_txtbank),
			code >> 12,
			0)
}

WRITE16_HANDLER( quizpani_bg_videoram_w )
{
	quizpani_bg_videoram[offset] = data;
	tilemap_mark_tile_dirty(bg_tilemap, offset);
}

WRITE16_HANDLER( quizpani_txt_videoram_w )
{
	quizpani_txt_videoram[offset] = data;
	tilemap_mark_tile_dirty(txt_tilemap, offset);
}

WRITE16_HANDLER( quizpani_tilesbank_w )
{
	if (ACCESSING_LSB)
	{
		if(quizpani_txtbank != (data & 0x30)>>4)
		{
			quizpani_txtbank = (data & 0x30)>>4;
			tilemap_mark_all_tiles_dirty(txt_tilemap);
		}

		if(quizpani_bgbank != (data & 3))
		{
			quizpani_bgbank = data & 3;
			tilemap_mark_all_tiles_dirty(bg_tilemap);
		}
	}
}

VIDEO_START( quizpani )
{
	bg_tilemap  = tilemap_create(bg_tile_info, bg_scan,TILEMAP_OPAQUE,16,16,256,32);
	txt_tilemap = tilemap_create(txt_tile_info,bg_scan,TILEMAP_TRANSPARENT,16,16,256,32);
	tilemap_set_transparent_pen(txt_tilemap,15);

	if( !bg_tilemap || !txt_tilemap )
		return 1;

	return 0;
}

VIDEO_UPDATE( quizpani )
{
	tilemap_set_scrollx(bg_tilemap, 0, quizpani_scrollreg[0] - 64);
	tilemap_set_scrolly(bg_tilemap, 0, quizpani_scrollreg[1] + 16);
	tilemap_set_scrollx(txt_tilemap, 0, quizpani_scrollreg[2] - 64);
	tilemap_set_scrolly(txt_tilemap, 0, quizpani_scrollreg[3] + 16);

	tilemap_draw(bitmap,cliprect,bg_tilemap,0,0);
	tilemap_draw(bitmap,cliprect,txt_tilemap,0,0);
}
