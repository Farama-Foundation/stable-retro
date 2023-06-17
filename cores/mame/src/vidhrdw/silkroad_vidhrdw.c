#include "driver.h"

/* Sprites probably need to be delayed */
/* Some scroll layers may need to be offset slightly? */
/* Check Sprite Colours after redump */
/* Clean Up */
/* is theres a bg colour register? */

static struct tilemap *fg_tilemap,*fg2_tilemap,*fg3_tilemap;
extern data32_t *silkroad_vidram,*silkroad_vidram2,*silkroad_vidram3, *silkroad_sprram, *silkroad_regs;

static void silkroad_drawsprites( struct mame_bitmap *bitmap, const struct rectangle *cliprect, int pri )
{
	const struct GfxElement *gfx = Machine->gfx[0];
	data32_t *source = silkroad_sprram;
	data32_t *finish = source + 0x1000/4;
	data32_t *maxspr = source;

	while( maxspr<finish )
	{
		int attr = (maxspr[1] & 0x0000ffff);

		if ( (attr & 0xff00) == 0xff00 ) break;
		maxspr+=2;
	}

	finish = maxspr-2;

	while( finish>=source )
	{

		int xpos = (finish[0] & 0x01ff0000) >> 16;
		int ypos = (finish[0] & 0x0000ffff);
		int tileno = (finish[1] & 0xffff0000) >> 16;
		int attr = (finish[1] & 0x0000ffff);

		int flipx = (attr & 0x0080);

		int width = (attr & 0x0f00) >> 8;
		int wcount;
		int color = (attr & 0x003f) ;

		int priority = (attr & 0x1000) >> 12;

		if ( (attr & 0x8000) == 0x8000 ) tileno+=0x10000;

		width += 1;


		if (priority == pri){

		if (!flipx) {
		for (wcount=0;wcount<width;wcount++) {
		drawgfx(bitmap,gfx,tileno+wcount,color,0,0,xpos+wcount*16+8,ypos,cliprect,TRANSPARENCY_PEN,0);
		}

		} else {

		for (wcount=width;wcount>0;wcount--) {
		drawgfx(bitmap,gfx,tileno+(width-wcount),color,1,0,xpos+wcount*16-16+8,ypos,cliprect,TRANSPARENCY_PEN,0);
		}

	}
		}
	finish-=2;
	}
}


static void get_fg_tile_info(int tile_index)
{

	int code = ((silkroad_vidram[tile_index] & 0xffff0000) >> 16 );
	int color = ((silkroad_vidram[tile_index] & 0x000001f));
	int flipx =  ((silkroad_vidram[tile_index] & 0x0000080) >> 7);

	code += 0x18000;

	SET_TILE_INFO(
			0,
			code,
			color,
			TILE_FLIPYX(flipx))
}



WRITE32_HANDLER( silkroad_fgram_w )
{
	COMBINE_DATA(&silkroad_vidram[offset]);

	tilemap_mark_tile_dirty(fg_tilemap,offset);
}

static void get_fg2_tile_info(int tile_index)
{
	int code = ((silkroad_vidram2[tile_index] & 0xffff0000) >> 16 );
	int color = ((silkroad_vidram2[tile_index] & 0x000001f));
	int flipx =  ((silkroad_vidram2[tile_index] & 0x0000080) >> 7);
	code += 0x18000;
	SET_TILE_INFO(
			0,
			code,
			color,
			TILE_FLIPYX(flipx))
}



WRITE32_HANDLER( silkroad_fgram2_w )
{
	COMBINE_DATA(&silkroad_vidram2[offset]);
	tilemap_mark_tile_dirty(fg2_tilemap,offset);
}

static void get_fg3_tile_info(int tile_index)
{
	int code = ((silkroad_vidram3[tile_index] & 0xffff0000) >> 16 );
	int color = ((silkroad_vidram3[tile_index] & 0x000001f));
	int flipx =  ((silkroad_vidram3[tile_index] & 0x0000080) >> 7);
	code += 0x18000;
	SET_TILE_INFO(
			0,
			code,
			color,
			TILE_FLIPYX(flipx))
}



WRITE32_HANDLER( silkroad_fgram3_w )
{
	COMBINE_DATA(&silkroad_vidram3[offset]);
	tilemap_mark_tile_dirty(fg3_tilemap,offset);
}



VIDEO_START(silkroad)
{
	fg_tilemap = tilemap_create(get_fg_tile_info,tilemap_scan_rows,TILEMAP_TRANSPARENT,16,16,64, 64);
		tilemap_set_transparent_pen(fg_tilemap,0);

	fg2_tilemap = tilemap_create(get_fg2_tile_info,tilemap_scan_rows,TILEMAP_TRANSPARENT,16,16,64, 64);
		tilemap_set_transparent_pen(fg2_tilemap,0);

	fg3_tilemap = tilemap_create(get_fg3_tile_info,tilemap_scan_rows,TILEMAP_TRANSPARENT,16,16,64, 64);
		tilemap_set_transparent_pen(fg3_tilemap,0);

	return 0;
}

int enable1=1,enable2=1,enable3=1;

VIDEO_UPDATE(silkroad)
{
	fillbitmap(bitmap,0x7c0,&Machine->visible_area);

	tilemap_set_scrollx( fg_tilemap, 0, ((silkroad_regs[0] & 0xffff0000) >> 16) );
	tilemap_set_scrolly( fg_tilemap, 0, (silkroad_regs[0] & 0x0000ffff) >> 0 );

	tilemap_set_scrolly( fg3_tilemap, 0, (silkroad_regs[1] & 0xffff0000) >> 16 );
	tilemap_set_scrollx( fg3_tilemap, 0, (silkroad_regs[2] & 0xffff0000) >> 16 );

	tilemap_set_scrolly( fg2_tilemap, 0, ((silkroad_regs[5] & 0xffff0000) >> 16));
	tilemap_set_scrollx( fg2_tilemap, 0, (silkroad_regs[2] & 0x0000ffff) >> 0 );

/*
	if (keyboard_pressed(KEYCODE_A)) enable1 ^= 1;
	if (keyboard_pressed(KEYCODE_S)) enable2 ^= 1;
	if (keyboard_pressed(KEYCODE_D)) enable3 ^= 1;
*/

	if(enable1)	tilemap_draw(bitmap,cliprect,fg_tilemap,0,0);

	silkroad_drawsprites(bitmap,cliprect,0);
	if(enable2)	tilemap_draw(bitmap,cliprect,fg2_tilemap,0,0);

	silkroad_drawsprites(bitmap,cliprect,1);
	if(enable3)	tilemap_draw(bitmap,cliprect,fg3_tilemap,0,0);

/*
	usrintf_showmessage	("Regs %08x %08x %08x %08x %08x",
	silkroad_regs[0],
	silkroad_regs[1],
	silkroad_regs[2],
	silkroad_regs[4],
	silkroad_regs[5]
	);
*/
}
