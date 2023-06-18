/***************************************************************************

	Renegade Video Hardware

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

UINT8 *renegade_videoram2;
int renegade_scrollx;
static struct tilemap *bg_tilemap;
static struct tilemap *fg_tilemap;

WRITE_HANDLER( renegade_videoram_w )
{
	if( videoram[offset]!=data )
	{
		videoram[offset] = data;
		offset = offset%(64*16);
		tilemap_mark_tile_dirty(bg_tilemap,offset);
	}
}

WRITE_HANDLER( renegade_videoram2_w )
{
	if( renegade_videoram2[offset]!=data )
	{
		renegade_videoram2[offset] = data;
		offset = offset%(32*32);
		tilemap_mark_tile_dirty(fg_tilemap,offset);
	}
}

WRITE_HANDLER( renegade_flipscreen_w )
{
	flip_screen_set(~data & 0x01);
}

WRITE_HANDLER( renegade_scroll0_w )
{
	renegade_scrollx = (renegade_scrollx&0xff00)|data;
}

WRITE_HANDLER( renegade_scroll1_w )
{
	renegade_scrollx = (renegade_scrollx&0xFF)|(data<<8);
}

static void get_bg_tilemap_info(int tile_index)
{
	const UINT8 *source = &videoram[tile_index];
	UINT8 attributes = source[0x400]; /* CCC??BBB */
	SET_TILE_INFO(
			1+(attributes&0x7),
			source[0],
			attributes>>5,
			0)
}

static void get_fg_tilemap_info(int tile_index)
{
	const UINT8 *source = &renegade_videoram2[tile_index];
	UINT8 attributes = source[0x400];
	SET_TILE_INFO(
			0,
			(attributes&3)*256 + source[0],
			attributes>>6,
			0)
}

VIDEO_START( renegade )
{
	bg_tilemap = tilemap_create(get_bg_tilemap_info,tilemap_scan_rows,TILEMAP_OPAQUE,   16,16,64,16);
	fg_tilemap = tilemap_create(get_fg_tilemap_info,tilemap_scan_rows,TILEMAP_TRANSPARENT,8,8,32,32);

	if (!bg_tilemap || !fg_tilemap)
		return 1;

	tilemap_set_transparent_pen(fg_tilemap,0);
	tilemap_set_scrolldx( bg_tilemap, 256, 0 );
	return 0;
}

static void draw_sprites( struct mame_bitmap *bitmap, const struct rectangle *cliprect )
{
	UINT8 *source = spriteram;
	UINT8 *finish = source+96*4;

	while( source<finish )
	{
		int sy = 240-source[0];
		if( sy>=16 )
		{
		    int attributes = source[1]; /* SFCCBBBB */
		    int sx = source[3];
		    int sprite_number = source[2];
		    int sprite_bank = 9 + (attributes&0xF);
		    int color = (attributes>>4)&0x3;
		    int xflip = attributes&0x40;

		    if( sx>248 ) sx -= 256;

			if (flip_screen)
			{
				sx = 240 - sx;
				sy = 240 - sy;
				xflip = !xflip;
			}

		    if( attributes&0x80 ){ /* big sprite */
		        drawgfx(bitmap,Machine->gfx[sprite_bank],
		            sprite_number+1,
		            color,
		            xflip,flip_screen,
		            sx,sy + (flip_screen ? -16 : 16),
		            cliprect,TRANSPARENCY_PEN,0);
		    }
		    else
			{
				sy += (flip_screen ? -16 : 16);
		    }
		    drawgfx(bitmap,Machine->gfx[sprite_bank],
		        sprite_number,
		        color,
		        xflip,flip_screen,
		        sx,sy,
		        cliprect,TRANSPARENCY_PEN,0);
		}
		source+=4;
	}
}

VIDEO_UPDATE( renegade )
{
	tilemap_set_scrollx( bg_tilemap, 0, renegade_scrollx );
	tilemap_draw( bitmap,cliprect,bg_tilemap,0 ,0);
	draw_sprites( bitmap,cliprect );
	tilemap_draw( bitmap,cliprect,fg_tilemap,0 ,0);
}
