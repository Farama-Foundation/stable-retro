/* Fit of Fighting Video Hardware */

#include "driver.h"

extern data16_t *fitfight_spriteram;
extern data16_t *fof_100000, *fof_600000, *fof_700000, *fof_800000, *fof_900000, *fof_a00000;

extern data16_t *fof_bak_tileram;
static struct tilemap *fof_bak_tilemap;
extern data16_t *fof_mid_tileram;
static struct tilemap *fof_mid_tilemap;
extern data16_t *fof_txt_tileram;
static struct tilemap *fof_txt_tilemap;

extern char bbprot_kludge;

static void fitfight_drawsprites( struct mame_bitmap *bitmap, const struct rectangle *cliprect )
{
	const struct GfxElement *gfx = Machine->gfx[3];
	data16_t *source = fitfight_spriteram;
	data16_t *finish = source + 0x800/2;

	while( source<finish )
	{
		int xpos, ypos, number,xflip,yflip, end, colr;

		ypos = source[0];
		xpos = source[3];
		number = source[2];
		xflip = (source[1] & 0x0001) ^ 0x0001;
		yflip = (source[1] & 0x0002);
		colr = (source[1] & 0x00fc) >> 2;
		if (bbprot_kludge==1) colr = (source[1] & 0x00f8) >> 3;
		end = source[0] & 0x8000;

		ypos = 0xff-ypos;

		xpos -=48;
		ypos -=16;

		if (end) break;

		 drawgfx(bitmap,gfx,number,colr,xflip,yflip,xpos,ypos,cliprect,TRANSPARENCY_PEN,0);

		source+=4;
	}
}

static void get_fof_bak_tile_info(int tile_index)
{
	int code = fof_bak_tileram[tile_index*2+1];
	int colr = fof_bak_tileram[tile_index*2] & 0x1f;
	int xflip = (fof_bak_tileram[tile_index*2] & 0x0020)>>5;
	xflip ^= 1;

	SET_TILE_INFO(2,code,colr,TILE_FLIPYX(xflip))
}

WRITE16_HANDLER(  fof_bak_tileram_w )
{
	COMBINE_DATA(&fof_bak_tileram[offset]);
	tilemap_mark_tile_dirty(fof_bak_tilemap,offset/2);
}


static void get_fof_mid_tile_info(int tile_index)
{
	int code = fof_mid_tileram[tile_index*2+1];
	int colr = fof_mid_tileram[tile_index*2] & 0x1f;
	int xflip = (fof_mid_tileram[tile_index*2] & 0x0020)>>5;
	xflip ^= 1;

	SET_TILE_INFO(1,code,colr,TILE_FLIPYX(xflip))
}

WRITE16_HANDLER(  fof_mid_tileram_w )
{
	COMBINE_DATA(&fof_mid_tileram[offset]);

	tilemap_mark_tile_dirty(fof_mid_tilemap,offset/2);
}

static void get_fof_txt_tile_info(int tile_index)
{
	int code = fof_txt_tileram[tile_index*2+1];
	int colr = fof_txt_tileram[tile_index*2] & 0x1f;
	int xflip = (fof_txt_tileram[tile_index*2] & 0x0020)>>5;
	xflip ^= 1;

	SET_TILE_INFO(0,code,colr,TILE_FLIPYX(xflip))
}

WRITE16_HANDLER(  fof_txt_tileram_w )
{
	COMBINE_DATA(&fof_txt_tileram[offset]);
	tilemap_mark_tile_dirty(fof_txt_tilemap,offset/2);
}

/* video start / update */

VIDEO_START(fitfight)
{
	fof_bak_tilemap = tilemap_create(get_fof_bak_tile_info,tilemap_scan_cols,TILEMAP_OPAQUE,8,8,128, 32);
	/* opaque */

	fof_mid_tilemap = tilemap_create(get_fof_mid_tile_info,tilemap_scan_cols,TILEMAP_TRANSPARENT,8,8,128, 32);
	tilemap_set_transparent_pen(fof_mid_tilemap,0);

	fof_txt_tilemap = tilemap_create(get_fof_txt_tile_info,tilemap_scan_cols,TILEMAP_TRANSPARENT,8,8,128, 32);
	tilemap_set_transparent_pen(fof_txt_tilemap,0);


	return 0;
}

VIDEO_UPDATE(fitfight)
{
	/* scroll isn't right */

	int scrollval;

	int scroll2;

	scroll2 = (fof_700000[0] & 0x01f8) >> 6;
	scroll2 = 0;

	scrollval = (fof_a00000[0]&0xff00) >> 5;



	tilemap_set_scrollx(fof_bak_tilemap,0, scrollval - scroll2 );
	tilemap_set_scrolly(fof_bak_tilemap,0, fof_a00000[0]&0xff);

	tilemap_draw(bitmap,cliprect,fof_bak_tilemap,0,0);


	scrollval = (fof_900000[0]&0xff00) >> 5;

	tilemap_set_scrollx(fof_mid_tilemap,0, scrollval - scroll2 );
	tilemap_set_scrolly(fof_mid_tilemap,0, fof_900000[0]&0xff);

	tilemap_draw(bitmap,cliprect,fof_mid_tilemap,0,0);

	fitfight_drawsprites(bitmap,cliprect);

	tilemap_draw(bitmap,cliprect,fof_txt_tilemap,0,0);

/*	usrintf_showmessage	("Regs %04x %04x %04x %04x %04x %04x",
			fof_100000[0], fof_600000[0], fof_700000[0],
			fof_800000[0], fof_900000[0],
			fof_a00000[0] );
*/
}
