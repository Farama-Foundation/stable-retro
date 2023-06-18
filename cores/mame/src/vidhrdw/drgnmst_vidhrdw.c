/* remaining gfx glitches*/

/* layer priority register not fully understood*/

#include "driver.h"
#include "vidhrdw/generic.h"

extern data16_t *drgnmst_vidregs;

extern data16_t *drgnmst_fg_videoram;
static struct tilemap *drgnmst_fg_tilemap;
extern data16_t *drgnmst_bg_videoram;
static struct tilemap *drgnmst_bg_tilemap;
extern data16_t *drgnmst_md_videoram;
static struct tilemap *drgnmst_md_tilemap;

extern data16_t *drgnmst_rowscrollram;
extern data16_t *drgnmst_vidregs2;


static void get_drgnmst_fg_tile_info(int tile_index)
{
	int tileno,colour, flipyx;
	tileno = drgnmst_fg_videoram[tile_index*2] & 0xfff;
	colour = drgnmst_fg_videoram[tile_index*2+1] & 0x1f;
	flipyx = (drgnmst_fg_videoram[tile_index*2+1] & 0x60)>>5;
	
	if ((drgnmst_fg_videoram[tile_index * 2] & 0x4000))
		tileno |= 0x10000;

	/* tileno |= (BIT(tile_index, 5)) << 15; */ /* 8x8 tile bank seems like cps1 (not on mastfury? or because bad dump) */
	if ((drgnmst_fg_videoram[tile_index * 2] & 0x8000))
		tileno |= 0x8000;

	tileno ^= 0x18000;

	SET_TILE_INFO(1,tileno,colour,TILE_FLIPYX(flipyx))
}

WRITE16_HANDLER( drgnmst_fg_videoram_w )
{
	COMBINE_DATA(&drgnmst_fg_videoram[offset]);
	tilemap_mark_tile_dirty(drgnmst_fg_tilemap,offset/2);
}



static void get_drgnmst_bg_tile_info(int tile_index)
{
	int tileno,colour,flipyx;
	tileno = (drgnmst_bg_videoram[tile_index*2]& 0x3ff) + 0xc00;
	colour = drgnmst_bg_videoram[tile_index*2+1] & 0x1f;
	flipyx = (drgnmst_bg_videoram[tile_index*2+1] & 0x60)>>5;
	
	
	if ((drgnmst_bg_videoram[tile_index * 2] & 0x1000))
		tileno |= 0x1000;

	tileno ^= 0x1000;

	SET_TILE_INFO(3,tileno,colour,TILE_FLIPYX(flipyx))
}

WRITE16_HANDLER( drgnmst_bg_videoram_w )
{
	COMBINE_DATA(&drgnmst_bg_videoram[offset]);
	tilemap_mark_tile_dirty(drgnmst_bg_tilemap,offset/2);
}

static void get_drgnmst_md_tile_info(int tile_index)
{
	int tileno,colour,flipyx;
	tileno = (drgnmst_md_videoram[tile_index*2]& 0x3fff)-0x2000;

	colour = drgnmst_md_videoram[tile_index*2+1] & 0x1f;
	flipyx = (drgnmst_md_videoram[tile_index*2+1] & 0x60)>>5;
	
	if ((drgnmst_md_videoram[tile_index * 2] & 0x4000))
		tileno |= 0x4000;

	tileno ^= 0x4000;



	SET_TILE_INFO(2,tileno,colour,TILE_FLIPYX(flipyx))
}

WRITE16_HANDLER( drgnmst_md_videoram_w )
{
	COMBINE_DATA(&drgnmst_md_videoram[offset]);
	tilemap_mark_tile_dirty(drgnmst_md_tilemap,offset/2);
}

static void drgnmst_draw_sprites(struct mame_bitmap *bitmap,const struct rectangle *cliprect)
{
	const struct GfxElement *gfx = Machine->gfx[0];
	data16_t *source = spriteram16;
	data16_t *finish = source + 0x800/2;

	while( source<finish )
	{
		int xpos, ypos, number, flipx,flipy,wide,high;
		int x,y;
		int incx,incy;
		int colr;

		number = source[2];
		xpos = source[0];
		ypos = source[1];
		flipx = source[3] & 0x0020;
		flipy = source[3] & 0x0040;
		wide = (source[3] & 0x0f00)>>8;
		high = (source[3] & 0xf000)>>12;
		colr = (source[3] & 0x001f);

		if ((source[3] & 0xff00) == 0xff00) break;


		if (!flipx) { incx = 16;} else { incx = -16; xpos += 16*wide; }
		if (!flipy) { incy = 16;} else { incy = -16; ypos += 16*high; }


		for (y=0;y<=high;y++) {
			for (x=0;x<=wide;x++) {

				int realx, realy, realnumber;

				realx = xpos+incx*x;
				realy = ypos+incy*y;
				realnumber = number+x+y*16;

				drawgfx(bitmap,gfx,realnumber,colr,flipx,flipy,realx,realy,cliprect,TRANSPARENCY_PEN,15);

			}
		}

		source+=4;
	}
}


UINT32 drgnmst_fg_tilemap_scan_cols( UINT32 col, UINT32 row, UINT32 num_cols, UINT32 num_rows )
{
	return (col*32)+(row&0x1f)+((row&0xe0)>>5)*2048;
}

UINT32 drgnmst_md_tilemap_scan_cols( UINT32 col, UINT32 row, UINT32 num_cols, UINT32 num_rows )
{
	return (col*16)+(row&0x0f)+((row&0xf0)>>4)*1024;
}

UINT32 drgnmst_bg_tilemap_scan_cols( UINT32 col, UINT32 row, UINT32 num_cols, UINT32 num_rows )
{
	return (col*8)+(row&0x07)+((row&0xf8)>>3)*512;
}

VIDEO_START(drgnmst)
{
	drgnmst_fg_tilemap = tilemap_create(get_drgnmst_fg_tile_info,drgnmst_fg_tilemap_scan_cols,TILEMAP_TRANSPARENT,      8, 8, 64,64);
	tilemap_set_transparent_pen(drgnmst_fg_tilemap,15);

	drgnmst_md_tilemap = tilemap_create(get_drgnmst_md_tile_info,drgnmst_md_tilemap_scan_cols,TILEMAP_TRANSPARENT,      16, 16, 64,64);
	tilemap_set_transparent_pen(drgnmst_md_tilemap,15);

	drgnmst_bg_tilemap = tilemap_create(get_drgnmst_bg_tile_info,drgnmst_bg_tilemap_scan_cols,TILEMAP_TRANSPARENT,      32, 32, 64,64);
	tilemap_set_transparent_pen(drgnmst_bg_tilemap,15);

	/* do the other tilemaps have rowscroll too? probably not ..*/
	tilemap_set_scroll_rows(drgnmst_md_tilemap,1024);

	return 0;
}

VIDEO_UPDATE(drgnmst)
{
	int y, rowscroll_bank;
	
	rowscroll_bank = (drgnmst_vidregs[4] & 0x30) >> 4;
	
	/* drgnmst scrolling */
	
	tilemap_set_scrollx(drgnmst_fg_tilemap,0, drgnmst_vidregs[0x6]-18); /* verify (test mode colour test needs it) */
	tilemap_set_scrolly(drgnmst_fg_tilemap,0, drgnmst_vidregs[0x7]); /* verify */

	tilemap_set_scrolly(drgnmst_md_tilemap,0, drgnmst_vidregs[0x9]); /* verify */

	for (y = 0; y < 1024; y++)
         tilemap_set_scrollx(drgnmst_md_tilemap,y, drgnmst_vidregs[0x8]-16+drgnmst_rowscrollram[y+0x800*rowscroll_bank]);
	
	tilemap_set_scrollx(drgnmst_bg_tilemap,0, drgnmst_vidregs[0xa]-18); /* verify */
	tilemap_set_scrolly(drgnmst_bg_tilemap,0, drgnmst_vidregs[0xb]); /* verify */

	/* TODO: figure out which bits relate to the order, like cps1?
	23da mastfury before attract portraits, ending
    12c8 mastfury power on */
	switch (drgnmst_vidregs2[0])
	{
		case 0x23c0: /* all ok */
			tilemap_draw(bitmap,cliprect,drgnmst_bg_tilemap,TILEMAP_IGNORE_TRANSPARENCY,0);
			tilemap_draw(bitmap,cliprect,drgnmst_fg_tilemap,0,0);
			tilemap_draw(bitmap,cliprect,drgnmst_md_tilemap,0,0);
			break;
		case 0x2cc0: /* mastfury mr daeth stage all ok */
			tilemap_draw(bitmap,cliprect,drgnmst_bg_tilemap,TILEMAP_IGNORE_TRANSPARENCY,0);
			tilemap_draw(bitmap,cliprect,drgnmst_md_tilemap,0,0);
			tilemap_draw(bitmap,cliprect,drgnmst_fg_tilemap,0,0);
			break;
		case 0x38c0: /* mastfury Sakamoto stage, Sya Ki stage same as above? but see note
			should fg also go above sprites? (it partially obscures 'time over' and bonus stage items on Sakamoto stage
		    but explicitly changes from 2cc0 to display scores, which indicates there is maybe a difference) */
			tilemap_draw(bitmap,cliprect,drgnmst_bg_tilemap,TILEMAP_IGNORE_TRANSPARENCY,0);
			tilemap_draw(bitmap,cliprect,drgnmst_md_tilemap,0,0);
			tilemap_draw(bitmap,cliprect,drgnmst_fg_tilemap,0,0);
			break;
		case 0x2780: /* mastfury skyscraper lift stage all ok */
		case 0x279a: /* mastfury continue screen all ok */
			tilemap_draw(bitmap,cliprect,drgnmst_md_tilemap,TILEMAP_IGNORE_TRANSPARENCY,0);
			tilemap_draw(bitmap,cliprect,drgnmst_fg_tilemap,0,0);
			tilemap_draw(bitmap,cliprect,drgnmst_bg_tilemap,0,0);
			break;
		case 0x2d80: /* all ok */
		case 0x2cda: /* mastfury win quotes all ok */
			tilemap_draw(bitmap,cliprect,drgnmst_md_tilemap,TILEMAP_IGNORE_TRANSPARENCY,0);
			tilemap_draw(bitmap,cliprect,drgnmst_bg_tilemap,0,0);
			tilemap_draw(bitmap,cliprect,drgnmst_fg_tilemap,0,0);
			break;
		case 0x38da: /* fg unsure */
		case 0x215a: /* fg unsure (mastfury title) */
		case 0x2140: /* all ok */
			tilemap_draw(bitmap,cliprect,drgnmst_fg_tilemap,TILEMAP_IGNORE_TRANSPARENCY,0);
			tilemap_draw(bitmap,cliprect,drgnmst_bg_tilemap,0,0);
			tilemap_draw(bitmap,cliprect,drgnmst_md_tilemap,0,0);
			break;
		case 0x2451: /* fg unsure */
		case 0x2d9a: /* fg unsure */
		case 0x2440: /* all ok */
		case 0x245a: /* fg unsure, title screen */
			tilemap_draw(bitmap,cliprect,drgnmst_fg_tilemap,TILEMAP_IGNORE_TRANSPARENCY,0);
			tilemap_draw(bitmap,cliprect,drgnmst_md_tilemap,0,0);
			tilemap_draw(bitmap,cliprect,drgnmst_bg_tilemap,0,0);
			break;
		default:
			tilemap_draw(bitmap,cliprect,drgnmst_bg_tilemap,TILEMAP_IGNORE_TRANSPARENCY,0);
			tilemap_draw(bitmap,cliprect,drgnmst_fg_tilemap,0,0);
			tilemap_draw(bitmap,cliprect,drgnmst_md_tilemap,0,0);
            break;

	}

	drgnmst_draw_sprites(bitmap,cliprect);

}

VIDEO_UPDATE(mastfury)
{
	int y, rowscroll_bank;
	
	rowscroll_bank = (drgnmst_vidregs[4] & 0x30) >> 4;
	
	/* mastfury scrolling
	 does layer order change scroll offsets? */
	
	tilemap_set_scrollx(drgnmst_fg_tilemap,0, drgnmst_vidregs[0x6]-14); /* 14 = continue screen background */
	tilemap_set_scrolly(drgnmst_fg_tilemap,0, drgnmst_vidregs[0x7]); /* verify */

	tilemap_set_scrolly(drgnmst_md_tilemap,0, drgnmst_vidregs[0x8]); /* skyscraper lift stage confirms this reg? */

	for (y = 0; y < 1024; y++)
         tilemap_set_scrollx(drgnmst_md_tilemap,y, drgnmst_vidregs[0x9]-14+drgnmst_rowscrollram[y+0x800*rowscroll_bank]);
	
	tilemap_set_scrollx(drgnmst_bg_tilemap,0, drgnmst_vidregs[0xa]-18); /* 14 = char select backround, but vs screen is 16? */
	tilemap_set_scrolly(drgnmst_bg_tilemap,0, drgnmst_vidregs[0x10]); /* skyscraper lift stage confirms this reg? */

	/* TODO: figure out which bits relate to the order, like cps1?
	23da mastfury before attract portraits, ending
    12c8 mastfury power on */
	switch (drgnmst_vidregs2[0])
	{
		case 0x23c0: /* all ok */
			tilemap_draw(bitmap,cliprect,drgnmst_bg_tilemap,TILEMAP_IGNORE_TRANSPARENCY,0);
			tilemap_draw(bitmap,cliprect,drgnmst_fg_tilemap,0,0);
			tilemap_draw(bitmap,cliprect,drgnmst_md_tilemap,0,0);
			break;
		case 0x2cc0: /* mastfury mr daeth stage all ok */
			tilemap_draw(bitmap,cliprect,drgnmst_bg_tilemap,TILEMAP_IGNORE_TRANSPARENCY,0);
			tilemap_draw(bitmap,cliprect,drgnmst_md_tilemap,0,0);
			tilemap_draw(bitmap,cliprect,drgnmst_fg_tilemap,0,0);
			break;
		case 0x38c0: /* mastfury Sakamoto stage, Sya Ki stage same as above? but see note
			should fg also go above sprites? (it partially obscures 'time over' and bonus stage items on Sakamoto stage
		    but explicitly changes from 2cc0 to display scores, which indicates there is maybe a difference) */
			tilemap_draw(bitmap,cliprect,drgnmst_bg_tilemap,TILEMAP_IGNORE_TRANSPARENCY,0);
			tilemap_draw(bitmap,cliprect,drgnmst_md_tilemap,0,0);
			tilemap_draw(bitmap,cliprect,drgnmst_fg_tilemap,0,0);
			break;
		case 0x2780: /* mastfury skyscraper lift stage all ok */
		case 0x279a: /* mastfury continue screen all ok */
			tilemap_draw(bitmap,cliprect,drgnmst_md_tilemap,TILEMAP_IGNORE_TRANSPARENCY,0);
			tilemap_draw(bitmap,cliprect,drgnmst_fg_tilemap,0,0);
			tilemap_draw(bitmap,cliprect,drgnmst_bg_tilemap,0,0);
			break;
		case 0x2d80: /* all ok */
		case 0x2cda: /* mastfury win quotes all ok */
			tilemap_draw(bitmap,cliprect,drgnmst_md_tilemap,TILEMAP_IGNORE_TRANSPARENCY,0);
			tilemap_draw(bitmap,cliprect,drgnmst_bg_tilemap,0,0);
			tilemap_draw(bitmap,cliprect,drgnmst_fg_tilemap,0,0);
			break;
		case 0x38da: /* fg unsure */
		case 0x215a: /* fg unsure (mastfury title) */
		case 0x2140: /* all ok */
			tilemap_draw(bitmap,cliprect,drgnmst_fg_tilemap,TILEMAP_IGNORE_TRANSPARENCY,0);
			tilemap_draw(bitmap,cliprect,drgnmst_bg_tilemap,0,0);
			tilemap_draw(bitmap,cliprect,drgnmst_md_tilemap,0,0);
			break;
		case 0x2451: /* fg unsure */
		case 0x2d9a: /* fg unsure */
		case 0x2440: /* all ok */
		case 0x245a: /* fg unsure, title screen */
			tilemap_draw(bitmap,cliprect,drgnmst_fg_tilemap,TILEMAP_IGNORE_TRANSPARENCY,0);
			tilemap_draw(bitmap,cliprect,drgnmst_md_tilemap,0,0);
			tilemap_draw(bitmap,cliprect,drgnmst_bg_tilemap,0,0);
			break;
		default:
			tilemap_draw(bitmap,cliprect,drgnmst_bg_tilemap,TILEMAP_IGNORE_TRANSPARENCY,0);
			tilemap_draw(bitmap,cliprect,drgnmst_fg_tilemap,0,0);
			tilemap_draw(bitmap,cliprect,drgnmst_md_tilemap,0,0);
            break;

	}

	drgnmst_draw_sprites(bitmap,cliprect);

}
