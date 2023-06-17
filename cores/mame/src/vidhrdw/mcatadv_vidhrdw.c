/* Magical Cat Adventure / Nostradamus Video Hardware */

/*
Notes:
Tilemap drawing is a killer on the first level of Nost due to the whole tilemap being dirty every frame.
Sprite drawing is quite fast (See USER1 in the profiler), but requires blit-time rotation support

Nost final boss, the priority of the arms is under the tilemaps, everything else is above. Should it be blended? i.e. Shadow.
There is an area of vid ram for linescroll, nost tests it but doesn't appear to use it.

ToDo: Fix Sprites for Cocktail
*/

#include "driver.h"
#include "vidhrdw/generic.h"

/* Defined in driver */
extern data16_t *mcatadv_videoram1, *mcatadv_videoram2;
extern data16_t *mcatadv_scroll, *mcatadv_scroll2;
extern data16_t *mcatadv_vidregs;

static struct tilemap *mcatadv_tilemap1,  *mcatadv_tilemap2;
static data16_t *spriteram_old, *vidregs_old;
static int palette_bank1, palette_bank2;


static void get_mcatadv_tile_info1(int tile_index)
{
	int tileno, colour, pri;

	tileno = mcatadv_videoram1[tile_index*2+1];
	colour = (mcatadv_videoram1[tile_index*2] & 0x3f00)>>8;
	pri = (mcatadv_videoram1[tile_index*2] & 0xc000)>>14;

	SET_TILE_INFO(0,tileno,colour + palette_bank1*0x40,0)
	tile_info.priority = pri;
}

WRITE16_HANDLER( mcatadv_videoram1_w )
{
	if (mcatadv_videoram1[offset] != data)
	{
		COMBINE_DATA(&mcatadv_videoram1[offset]);
		tilemap_mark_tile_dirty(mcatadv_tilemap1,offset/2);
	}
}

static void get_mcatadv_tile_info2(int tile_index)
{
	int tileno, colour, pri;

	tileno = mcatadv_videoram2[tile_index*2+1];
	colour = (mcatadv_videoram2[tile_index*2] & 0x3f00)>>8;
	pri = (mcatadv_videoram2[tile_index*2] & 0xc000)>>14;

	SET_TILE_INFO(1,tileno,colour + palette_bank2*0x40,0)
	tile_info.priority = pri;
}

WRITE16_HANDLER( mcatadv_videoram2_w )
{
	if (mcatadv_videoram2[offset] != data)
	{
		COMBINE_DATA(&mcatadv_videoram2[offset]);
		tilemap_mark_tile_dirty(mcatadv_tilemap2,offset/2);
	}
}


static void mcatadv_drawsprites ( struct mame_bitmap *bitmap, const struct rectangle *cliprect )
{
	data16_t *source = spriteram_old;
	data16_t *finish = source + (spriteram_size/2)/2;
	int global_x = mcatadv_vidregs[0]-0x184;
	int global_y = mcatadv_vidregs[1]-0x1f1;

	UINT16 *destline;
	UINT8 *priline;

	int xstart, xend, xinc;
	int ystart, yend, yinc;

	if( vidregs_old[2] == 0x0001 ) /* Double Buffered */
	{
		source += (spriteram_size/2)/2;
		finish += (spriteram_size/2)/2;
	}
	else if( vidregs_old[2] ) /* I suppose it's possible that there is 4 banks, haven't seen it used though */
	{
		log_cb(RETRO_LOG_DEBUG, LOGPRE "Spritebank != 0/1\n");
	}

	while ( source<finish )
	{
		int pen = (source[0]&0x3f00)>>8;
		int tileno = source[1]&0xffff;
		int pri = (source[0]&0xc000)>>14;
		int x = source[2]&0x3ff;
		int y = source[3]&0x3ff;
		int flipy = source[0] & 0x0040;
		int flipx = source[0] & 0x0080;

		int height = ((source[3]&0xf000)>>12)*16;
		int width = ((source[2]&0xf000)>>12)*16;
		int offset = tileno * 256;

		data8_t *sprdata = memory_region ( REGION_GFX1 );

		int drawxpos, drawypos;
		int xcnt,ycnt;
		int pix;

		if (x & 0x200) x-=0x400;
		if (y & 0x200) y-=0x400;

#if 0 /* For Flipscreen/Cocktail*/
		if(mcatadv_vidregs[0]&0x8000)
		{
			flipx = !flipx;
		}
		if(mcatadv_vidregs[1]&0x8000)
		{
			flipy = !flipy;
		}
#endif

		if (source[3] != source[0]) /* 'hack' don't draw sprites while its testing the ram!*/
		{
			if(!flipx) { xstart = 0;        xend = width;  xinc = 1; }
			else       { xstart = width-1;  xend = -1;     xinc = -1; }
			if(!flipy) { ystart = 0;        yend = height; yinc = 1; }
			else       { ystart = height-1; yend = -1;     yinc = -1; }

			for (ycnt = ystart; ycnt != yend; ycnt += yinc) {
				drawypos = y+ycnt-global_y;

				if ((drawypos >= cliprect->min_y) && (drawypos <= cliprect->max_y)) {
					destline = (UINT16 *)(bitmap->line[drawypos]);
					priline = (UINT8 *)(priority_bitmap->line[drawypos]);

					for (xcnt = xstart; xcnt != xend; xcnt += xinc) {
						drawxpos = x+xcnt-global_x;

						if (offset >= 0x500000*2) offset = 0;
						pix = sprdata[offset/2];

						if (offset & 1)  pix = pix >> 4;
						pix &= 0x0f;

						if ((drawxpos >= cliprect->min_x) && (drawxpos <= cliprect->max_x) && pix)
							if((priline[drawxpos] < pri))
								destline[drawxpos] = (pix + (pen<<4));

						offset++;
					}
				}
				else  {
					offset += width;
				}
			}
		}
		source+=4;
	}
}

VIDEO_UPDATE( mcatadv )
{
	int i, scrollx, scrolly, flip;

	fillbitmap(bitmap, get_black_pen(), cliprect);
	fillbitmap(priority_bitmap, 0, cliprect);

	if(mcatadv_scroll[2] != palette_bank1) {
		palette_bank1 = mcatadv_scroll[2];
		tilemap_mark_all_tiles_dirty(mcatadv_tilemap1);
	}

	if(mcatadv_scroll2[2] != palette_bank2) {
		palette_bank2 = mcatadv_scroll2[2];
		tilemap_mark_all_tiles_dirty(mcatadv_tilemap2);
	}

	scrollx = (mcatadv_scroll[0]&0x1ff)-0x194;
	scrolly = (mcatadv_scroll[1]&0x1ff)-0x1df;

	/* Global Flip */
	if(!(mcatadv_scroll[0]&0x8000)) scrollx -= 0x19;
	if(!(mcatadv_scroll[1]&0x8000)) scrolly -= 0x141;
	flip = ((mcatadv_scroll[0]&0x8000)?0:TILEMAP_FLIPX) | ((mcatadv_scroll[1]&0x8000)?0:TILEMAP_FLIPY);

	tilemap_set_scrollx(mcatadv_tilemap1, 0, scrollx);
	tilemap_set_scrolly(mcatadv_tilemap1, 0, scrolly);
	tilemap_set_flip(mcatadv_tilemap1, flip);

	scrollx = (mcatadv_scroll2[0]&0x1ff)-0x194;
	scrolly = (mcatadv_scroll2[1]&0x1ff)-0x1df;

	/* Global Flip */
	if(!(mcatadv_scroll2[0]&0x8000)) scrollx -= 0x19;
	if(!(mcatadv_scroll2[1]&0x8000)) scrolly -= 0x141;
	flip = ((mcatadv_scroll2[0]&0x8000)?0:TILEMAP_FLIPX) | ((mcatadv_scroll2[1]&0x8000)?0:TILEMAP_FLIPY);

	tilemap_set_scrollx(mcatadv_tilemap2, 0, scrollx);
	tilemap_set_scrolly(mcatadv_tilemap2, 0, scrolly);
	tilemap_set_flip(mcatadv_tilemap2, flip);

	for (i=0; i<=3; i++)
	{
#ifdef MAME_DEBUG
		if (!keyboard_pressed(KEYCODE_Q))
#endif
			tilemap_draw(bitmap, cliprect, mcatadv_tilemap1, i, i);
#ifdef MAME_DEBUG
		if (!keyboard_pressed(KEYCODE_W))
#endif
			tilemap_draw(bitmap, cliprect, mcatadv_tilemap2, i, i);
	}

	profiler_mark(PROFILER_USER1);
#ifdef MAME_DEBUG
	if (!keyboard_pressed(KEYCODE_E))
#endif
		mcatadv_drawsprites (bitmap, cliprect);
	profiler_mark(PROFILER_END);
}

VIDEO_START( mcatadv )
{
	mcatadv_tilemap1 = tilemap_create(get_mcatadv_tile_info1,tilemap_scan_rows,TILEMAP_TRANSPARENT, 16, 16,32,32);
	tilemap_set_transparent_pen(mcatadv_tilemap1,0);

	mcatadv_tilemap2 = tilemap_create(get_mcatadv_tile_info2,tilemap_scan_rows,TILEMAP_TRANSPARENT, 16, 16,32,32);
	tilemap_set_transparent_pen(mcatadv_tilemap2,0);

	spriteram_old = auto_malloc(spriteram_size);
	vidregs_old = auto_malloc(0xf);

	if(!mcatadv_tilemap1 || !mcatadv_tilemap2 || !spriteram_old || !vidregs_old)
		return 1;

	memset(spriteram_old,0,spriteram_size);

	palette_bank1 = 0;
	palette_bank2 = 0;

	return 0;
}

VIDEO_EOF( mcatadv )
{
	memcpy(spriteram_old,spriteram16,spriteram_size);
	memcpy(vidregs_old,mcatadv_vidregs,0xf);
}
