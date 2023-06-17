 /*
 * @file vidhrdw/djboy.c
 *
 * video hardware for DJ Boy
 */
#include "driver.h"
#include "vidhrdw/generic.h"

static data8_t djboy_videoreg, djboy_scrollx, djboy_scrolly, flipscreen;
static struct tilemap *background;
int scroll = 0;

void djboy_set_videoreg( data8_t data )
{
	djboy_videoreg = data;
}

WRITE_HANDLER( djboy_scrollx_w )
{
	djboy_scrollx = data;
}

WRITE_HANDLER( djboy_scrolly_w )
{
	djboy_scrolly = data;
}

static void get_bg_tile_info(int tile_index)
{
	UINT8 attr = videoram[tile_index + 0x800];
	int code = videoram[tile_index] + (attr&0xf)*256;
	int color = attr>>4;
	if( color&8 )
	{
		code |= 0x1000;
	}
	SET_TILE_INFO(1, code, color, 0);	/* no flip */
}

WRITE_HANDLER( djboy_videoram_w )
{
	if( videoram[offset] != data)
	{
		videoram[offset] = data;
		tilemap_mark_tile_dirty( background, offset & 0x7ff);
	}
}

VIDEO_START( djboy )
{
	background = tilemap_create(get_bg_tile_info,tilemap_scan_rows,TILEMAP_OPAQUE,16,16,64,32);
	buffered_spriteram   = auto_malloc(spriteram_size);
	if( background )
	{
		return 0;
	}

	return -1;
}

static void 
draw_sprites( struct mame_bitmap *bitmap,const struct rectangle *cliprect ) /* pandora_draw*/
{

	int sx=0, sy=0, x=0, y=0, offs;

	/*
     * Sprite Tile Format
     * ------------------
     *
     * Byte | Bit(s)   | Use
     * -----+-76543210-+----------------
     *  0-2 | -------- | unused
     *  3   | xxxx.... | Palette Bank
     *  3   | .......x | XPos - Sign Bit
     *  3   | ......x. | YPos - Sign Bit
     *  3   | .....x.. | Use Relative offsets
     *  4   | xxxxxxxx | XPos
     *  5   | xxxxxxxx | YPos
     *  6   | xxxxxxxx | Sprite Number (low 8 bits)
     *  7   | ....xxxx | Sprite Number (high 4 bits)
     *  7   | x....... | Flip Sprite Y-Axis
     *  7   | .x...... | Flip Sprite X-Axis
     */

	for (offs = 0;offs < 0x1000;offs += 8)
	{
		int dx = buffered_spriteram[offs+4];
		int dy = buffered_spriteram[offs+5];
		int tilecolour = buffered_spriteram[offs+3];
		int attr = buffered_spriteram[offs+7];
		int flipx =   attr & 0x80;
		int flipy =  (attr & 0x40) << 1;
		int tile  = ((attr & 0x3f) << 8) + (buffered_spriteram[offs+6] & 0xff);

		if (tilecolour & 1) dx |= 0x100;
		if (tilecolour & 2) dy |= 0x100;

		if (tilecolour & 4)
		{
			x += dx;
			y += dy;
		}
		else
		{
			x = dx;
			y = dy;
		}

		if (flipscreen)
		{
			sx = 240 - x;
			sy = 240 - y;
			flipx = !flipx;
			flipy = !flipy;
		}
		else
		{
			sx = x;
			sy = y;
		}

		sx &=0x1ff;
		sy &=0x1ff;

		if (sx&0x100) sx-=0x200;
		if (sy&0x100) sy-=0x200;

		drawgfx(
			bitmap,Machine->gfx[0],
			tile,
			(tilecolour & 0xf0) >> 4,
			flipx, flipy,
			sx,sy,
			cliprect,TRANSPARENCY_PEN,0);
	}
}


WRITE_HANDLER( djboy_paletteram_w )
{
	int val;

	paletteram[offset] = data;
	offset &= ~1;
	val = (paletteram[offset]<<8) | paletteram[offset+1];

	palette_set_color(offset/2,pal4bit(val >> 8),pal4bit(val >> 4),pal4bit(val >> 0));
}

VIDEO_UPDATE( djboy )
{
	/**
	 * xx------ msb x
	 * --x----- msb y
	 * ---x---- flipscreen?
	 * ----xxxx ROM bank
	 */
	flipscreen = 0; /*djboy_vidreg & 0x10;*/
    
	scroll = djboy_scrollx | ((djboy_videoreg&0xc0)<<2);
	tilemap_set_scrollx( background, 0, scroll-0x391 );
	scroll = djboy_scrolly | ((djboy_videoreg&0x20)<<3);
	tilemap_set_scrolly( background, 0, scroll );	
	tilemap_draw( bitmap, cliprect,background,0,0 );
	draw_sprites( bitmap, cliprect );
}

VIDEO_EOF( djboy )
{
	memcpy(buffered_spriteram,spriteram,spriteram_size);
}
