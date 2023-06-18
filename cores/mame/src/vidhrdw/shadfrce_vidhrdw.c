#include "driver.h"
#include "vidhrdw/generic.h"

static struct tilemap *shadfrce_fgtilemap, *shadfrce_bg0tilemap,  *shadfrce_bg1tilemap;
extern data16_t *shadfrce_fgvideoram, *shadfrce_bg0videoram,  *shadfrce_bg1videoram,   *shadfrce_spvideoram;
/* extern data16_t *shadfrce_videoregs; */

static data16_t *shadfrce_spvideoram_old; /* I *think* the sprites need to be delayed anyway */


static void get_shadfrce_fgtile_info(int tile_index)
{

	/* ---- ----  tttt tttt  ---- ----  pppp TTTT */
	int tileno, colour;

	tileno = (shadfrce_fgvideoram[tile_index *2] & 0x00ff) | ((shadfrce_fgvideoram[tile_index *2+1] & 0x000f) << 8);
	colour = (shadfrce_fgvideoram[tile_index *2+1] & 0x00f0) >>4;

	SET_TILE_INFO(0,tileno,colour*4,0)
}

WRITE16_HANDLER( shadfrce_fgvideoram_w )
{
	if (shadfrce_fgvideoram[offset] != data)
	{
		shadfrce_fgvideoram[offset] = data;
		tilemap_mark_tile_dirty(shadfrce_fgtilemap,offset/2);
	}
}

static void get_shadfrce_bg0tile_info(int tile_index)
{

	/* ---- ----  ---- cccc  --TT TTTT TTTT TTTT */
	int tileno, colour,fyx;

	tileno = (shadfrce_bg0videoram[tile_index *2+1] & 0x3fff);
	colour = shadfrce_bg0videoram[tile_index *2] & 0x001f;
	if (colour & 0x10) colour ^= 0x30;	/* skip hole */
	fyx = (shadfrce_bg0videoram[tile_index *2] & 0x00c0) >>6;

	SET_TILE_INFO(2,tileno,colour,TILE_FLIPYX(fyx))
}

WRITE16_HANDLER( shadfrce_bg0videoram_w )
{
	if (shadfrce_bg0videoram[offset] != data)
	{
		shadfrce_bg0videoram[offset] = data;
		tilemap_mark_tile_dirty(shadfrce_bg0tilemap,offset/2);
	}
}

static void get_shadfrce_bg1tile_info(int tile_index)
{
	int tileno, colour;

	tileno = (shadfrce_bg1videoram[tile_index] & 0x0fff);
	colour = (shadfrce_bg1videoram[tile_index] & 0xf000) >> 12;

	SET_TILE_INFO(2,tileno,colour+64,0)
}

WRITE16_HANDLER( shadfrce_bg1videoram_w )
{
	if (shadfrce_bg1videoram[offset] != data)
	{
		shadfrce_bg1videoram[offset] = data;
		tilemap_mark_tile_dirty(shadfrce_bg1tilemap,offset);
	}
}




VIDEO_START( shadfrce )
{
	shadfrce_fgtilemap = tilemap_create(get_shadfrce_fgtile_info,tilemap_scan_rows,TILEMAP_TRANSPARENT, 8, 8,64,32);
		tilemap_set_transparent_pen(shadfrce_fgtilemap,0);

	shadfrce_bg0tilemap = tilemap_create(get_shadfrce_bg0tile_info,tilemap_scan_rows,TILEMAP_TRANSPARENT, 16, 16,32,32);
		tilemap_set_transparent_pen(shadfrce_bg0tilemap,0);

	shadfrce_bg1tilemap = tilemap_create(get_shadfrce_bg1tile_info,tilemap_scan_rows,TILEMAP_OPAQUE, 16, 16,32,32);

	shadfrce_spvideoram_old = auto_malloc(spriteram_size);

	return 0;
}

WRITE16_HANDLER ( shadfrce_bg0scrollx_w )
{
	tilemap_set_scrollx( shadfrce_bg0tilemap, 0, data & 0x1ff );
}

WRITE16_HANDLER ( shadfrce_bg0scrolly_w )
{
	tilemap_set_scrolly( shadfrce_bg0tilemap, 0, data  & 0x1ff );
}

WRITE16_HANDLER ( shadfrce_bg1scrollx_w )
{
	tilemap_set_scrollx( shadfrce_bg1tilemap, 0, data  & 0x1ff );
}

WRITE16_HANDLER ( shadfrce_bg1scrolly_w )
{
	tilemap_set_scrolly( shadfrce_bg1tilemap, 0, data & 0x1ff );
}




static void draw_sprites( struct mame_bitmap *bitmap, const struct rectangle *cliprect )
{

	/* | ---- ---- hhhf Fe-Y | ---- ---- yyyy yyyy | ---- ---- TTTT TTTT | ---- ---- tttt tttt |
	   | ---- ---- -pCc cccX | ---- ---- xxxx xxxx | ---- ---- ---- ---- | ---- ---- ---- ---- | */

	/* h  = height
	   f  = flipx
	   F  = flipy
	   e  = enable
	   Yy = Y Position
	   Tt = Tile No.
	   Xx = X Position
	   Cc = color
	   P = priority
	*/

	const struct GfxElement *gfx = Machine->gfx[1];
	data16_t *finish = shadfrce_spvideoram_old;
	data16_t *source = finish + 0x2000/2 - 8;
	int hcount;
	while( source>=finish )
	{
		int ypos = 0x100 - (((source[0] & 0x0003) << 8) | (source[1] & 0x00ff));
		int xpos = (((source[4] & 0x0001) << 8) | (source[5] & 0x00ff)) + 1;
		int tile = ((source[2] & 0x00ff) << 8) | (source[3] & 0x00ff);
		int height = (source[0] & 0x00e0) >> 5;
		int enable = ((source[0] & 0x0004));
		int flipx = ((source[0] & 0x0010) >> 4);
		int flipy = ((source[0] & 0x0008) >> 3);
		int pal = ((source[4] & 0x003e));
		int pri_mask = (source[4] & 0x0040) ? 0x02 : 0x00;

		if (pal & 0x20) pal ^= 0x60;	/* skip hole */

		height++;
		if (enable)	{
			for (hcount=0;hcount<height;hcount++) {
				pdrawgfx(bitmap,gfx,tile+hcount,pal,flipx,flipy,xpos,ypos-hcount*16-16,cliprect,TRANSPARENCY_PEN,0,pri_mask);
				pdrawgfx(bitmap,gfx,tile+hcount,pal,flipx,flipy,xpos-0x200,ypos-hcount*16-16,cliprect,TRANSPARENCY_PEN,0,pri_mask);
				pdrawgfx(bitmap,gfx,tile+hcount,pal,flipx,flipy,xpos,ypos-hcount*16-16+0x200,cliprect,TRANSPARENCY_PEN,0,pri_mask);
				pdrawgfx(bitmap,gfx,tile+hcount,pal,flipx,flipy,xpos-0x200,ypos-hcount*16-16+0x200,cliprect,TRANSPARENCY_PEN,0,pri_mask);
			}
		}
		source-=8;
	}
}

VIDEO_UPDATE( shadfrce )
{
	fillbitmap(priority_bitmap,0,cliprect);

	tilemap_draw(bitmap,cliprect,shadfrce_bg1tilemap,0,0);
	tilemap_draw(bitmap,cliprect,shadfrce_bg0tilemap,0,1);

	draw_sprites(bitmap,cliprect);

	tilemap_draw(bitmap,cliprect,shadfrce_fgtilemap, 0,0);

/*
	usrintf_showmessage	("Regs %04x %04x %04x %04x %04x %04x %04x",
	shadfrce_videoregs[0],
	shadfrce_videoregs[1],
	shadfrce_videoregs[2],
	shadfrce_videoregs[3],
	shadfrce_videoregs[4],
	shadfrce_videoregs[5],
	shadfrce_videoregs[6]);
*/

}

VIDEO_EOF( shadfrce )
{
	/* looks like sprites are *two* frames ahead */
	memcpy(shadfrce_spvideoram_old,shadfrce_spvideoram,spriteram_size);
}
