/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

struct tilemap *pb_tilemap;
extern data8_t *pb_videoram;
extern int is_pbillian;

WRITE_HANDLER( pb_videoram_w )
{
	pb_videoram[offset] = data;
	tilemap_mark_tile_dirty(pb_tilemap,offset&0x3ff);
}

static void get_pb_tile_info(int tile_index)
{
	int tileno,pal;
	tileno = pb_videoram[tile_index]+((pb_videoram[tile_index+0x400]&0x7)<<8);
	pal=(pb_videoram[tile_index+0x400]&0xf0)>>4;
	SET_TILE_INFO(0,tileno,pal+0x10,0)
}

VIDEO_START(pbillian)
{
	pb_tilemap = tilemap_create(get_pb_tile_info,tilemap_scan_rows,TILEMAP_OPAQUE, 8, 8,32,32);
	paletteram = auto_malloc(0x200);
	return 0;
}


static void draw_sprites( struct mame_bitmap *bitmap, const struct rectangle *cliprect )
{
/*
	 Sprite format :
	  
		byte 0:  765432-- tile 
     				 ------10 ?? tile number ... maybe protection related
    byte 1:  sprite Y
    byte 2:  sprite X
    byte 3:  7654---- palette
             ----3210 tile (highest bits)
  
  Two lower bits of byte 0 are used both in prebillian 
  and hot smash. In hot smash bit 0 is set when ball 
  collide with screen edge. 
  (code @ $2ab5 , $2a0c (bit set))

  If there's no tile number,  it's possible to convert
  code to use 16x16 sprites.
  
  Maybe it's just simple sprite flipping (h/v)

*/

	const UINT8 *source = spriteram;
	const UINT8 *finish = source+40*4;

	int x,y,num,col;
	
	while(source<finish  )
	{
		x = source[1];
		y = source[2];
		col =(source[3]&0xf0)>>4;
	  num=source[0]|((source[3]&0x0f)<<8); 
	  
	  drawgfx( bitmap,Machine->gfx[0],num++,col,flip_screen,flip_screen,flip_screen?240-x+8:x,flip_screen?240-y+8:y,cliprect,TRANSPARENCY_PEN,0);
		drawgfx( bitmap,Machine->gfx[0],num++,col,flip_screen,flip_screen,flip_screen?240-x:x+8,flip_screen?240-y+8:y,cliprect,TRANSPARENCY_PEN,0);
		drawgfx( bitmap,Machine->gfx[0],num++,col,flip_screen,flip_screen,flip_screen?240-x+8:x,flip_screen?240-y:y+8,cliprect,TRANSPARENCY_PEN,0);
		drawgfx( bitmap,Machine->gfx[0],num++,col,flip_screen,flip_screen,flip_screen?240-x:x+8,flip_screen?240-y:y+8,cliprect,TRANSPARENCY_PEN,0);

		source += 0x4;
	}
}

VIDEO_UPDATE(pbillian)
{
	tilemap_draw(bitmap,cliprect,pb_tilemap,0,0);
	draw_sprites(bitmap,cliprect);
	if(is_pbillian)usrintf_showmessage	("Power %d%%", ((input_port_3_r(0)&0x3f)*100)/0x3f);
}
