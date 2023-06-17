/*******************************************************************

Rolling Thunder Video Hardware

*******************************************************************/

#include "driver.h"


#define GFX_TILES1	0
#define GFX_TILES2	1
#define GFX_SPRITES	2

unsigned char *rthunder_videoram1,*rthunder_videoram2;
extern unsigned char *spriteram;

static int tilebank;
static int xscroll[4], yscroll[4];	/* scroll + priority */

static struct tilemap *tilemap[4];

static int backcolor;
static int flipscreen;
static const unsigned char *tile_address_prom;


/***************************************************************************

  Convert the color PROMs into a more useable format.

  Rolling Thunder has two palette PROMs (512x8 and 512x4) and two 2048x8
  lookup table PROMs.
  The palette PROMs are connected to the RGB output this way:

  bit 3 -- 220 ohm resistor  -- BLUE
        -- 470 ohm resistor  -- BLUE
        -- 1  kohm resistor  -- BLUE
  bit 0 -- 2.2kohm resistor  -- BLUE

  bit 7 -- 220 ohm resistor  -- GREEN
        -- 470 ohm resistor  -- GREEN
        -- 1  kohm resistor  -- GREEN
        -- 2.2kohm resistor  -- GREEN
        -- 220 ohm resistor  -- RED
        -- 470 ohm resistor  -- RED
        -- 1  kohm resistor  -- RED
  bit 0 -- 2.2kohm resistor  -- RED

***************************************************************************/

PALETTE_INIT( namcos86 )
{
	int i;
	int totcolors,totlookup;


	totcolors = Machine->drv->total_colors;
	totlookup = Machine->drv->color_table_len;

	for (i = 0;i < totcolors;i++)
	{
		int bit0,bit1,bit2,bit3,r,g,b;


		bit0 = (color_prom[0] >> 0) & 0x01;
		bit1 = (color_prom[0] >> 1) & 0x01;
		bit2 = (color_prom[0] >> 2) & 0x01;
		bit3 = (color_prom[0] >> 3) & 0x01;
		r = 0x0e * bit0 + 0x1f * bit1 + 0x43 * bit2 + 0x8f * bit3;
		bit0 = (color_prom[0] >> 4) & 0x01;
		bit1 = (color_prom[0] >> 5) & 0x01;
		bit2 = (color_prom[0] >> 6) & 0x01;
		bit3 = (color_prom[0] >> 7) & 0x01;
		g = 0x0e * bit0 + 0x1f * bit1 + 0x43 * bit2 + 0x8f * bit3;
		bit0 = (color_prom[totcolors] >> 0) & 0x01;
		bit1 = (color_prom[totcolors] >> 1) & 0x01;
		bit2 = (color_prom[totcolors] >> 2) & 0x01;
		bit3 = (color_prom[totcolors] >> 3) & 0x01;
		b = 0x0e * bit0 + 0x1f * bit1 + 0x43 * bit2 + 0x8f * bit3;

		palette_set_color(i,r,g,b);
		color_prom++;
	}

	color_prom += totcolors;
	/* color_prom now points to the beginning of the lookup table */

	/* tiles lookup table */
	for (i = 0;i < totlookup/2;i++)
		*(colortable++) = *color_prom++;

	/* sprites lookup table */
	for (i = 0;i < totlookup/2;i++)
		*(colortable++) = *(color_prom++) + totcolors/2;

	/* color_prom now points to the beginning of the tile address decode PROM */

	tile_address_prom = color_prom;	/* we'll need this at run time */
}




/***************************************************************************

  Callbacks for the TileMap code

***************************************************************************/

static INLINE void get_tile_info(int tile_index,int layer,data8_t *vram)
{
	unsigned char attr = vram[2*tile_index + 1];
	int tile_offs;
	if (layer & 2)
		tile_offs = ((tile_address_prom[((layer & 1) << 4) + (attr & 0x03)] & 0xe0) >> 5) * 0x100;
	else
		tile_offs = ((tile_address_prom[((layer & 1) << 4) + ((attr & 0x03) << 2)] & 0x0e) >> 1) * 0x100 + tilebank * 0x800;

	SET_TILE_INFO(
			(layer & 2) ? GFX_TILES2 : GFX_TILES1,
			vram[2*tile_index] + tile_offs,
			attr,
			0)
}

static void get_tile_info0(int tile_index) { get_tile_info(tile_index,0,&rthunder_videoram1[0x0000]); }
static void get_tile_info1(int tile_index) { get_tile_info(tile_index,1,&rthunder_videoram1[0x1000]); }
static void get_tile_info2(int tile_index) { get_tile_info(tile_index,2,&rthunder_videoram2[0x0000]); }
static void get_tile_info3(int tile_index) { get_tile_info(tile_index,3,&rthunder_videoram2[0x1000]); }


/***************************************************************************

  Start the video hardware emulation.

***************************************************************************/

VIDEO_START( namcos86 )
{
	tilemap[0] = tilemap_create(get_tile_info0,tilemap_scan_rows,TILEMAP_TRANSPARENT,8,8,64,32);
	tilemap[1] = tilemap_create(get_tile_info1,tilemap_scan_rows,TILEMAP_TRANSPARENT,8,8,64,32);
	tilemap[2] = tilemap_create(get_tile_info2,tilemap_scan_rows,TILEMAP_TRANSPARENT,8,8,64,32);
	tilemap[3] = tilemap_create(get_tile_info3,tilemap_scan_rows,TILEMAP_TRANSPARENT,8,8,64,32);

	if (!tilemap[0] || !tilemap[1] || !tilemap[2] || !tilemap[3])
		return 1;

	tilemap_set_transparent_pen(tilemap[0],7);
	tilemap_set_transparent_pen(tilemap[1],7);
	tilemap_set_transparent_pen(tilemap[2],7);
	tilemap_set_transparent_pen(tilemap[3],7);

	return 0;
}



/***************************************************************************

  Memory handlers

***************************************************************************/

READ_HANDLER( rthunder_videoram1_r )
{
	return rthunder_videoram1[offset];
}

WRITE_HANDLER( rthunder_videoram1_w )
{
	if (rthunder_videoram1[offset] != data)
	{
		rthunder_videoram1[offset] = data;
		tilemap_mark_tile_dirty(tilemap[offset/0x1000],(offset & 0xfff)/2);
	}
}

READ_HANDLER( rthunder_videoram2_r )
{
	return rthunder_videoram2[offset];
}

WRITE_HANDLER( rthunder_videoram2_w )
{
	if (rthunder_videoram2[offset] != data)
	{
		rthunder_videoram2[offset] = data;
		tilemap_mark_tile_dirty(tilemap[2+offset/0x1000],(offset & 0xfff)/2);
	}
}

WRITE_HANDLER( rthunder_tilebank_select_0_w )
{
	if (tilebank != 0)
	{
		tilebank = 0;
		tilemap_mark_all_tiles_dirty(tilemap[0]);
		tilemap_mark_all_tiles_dirty(tilemap[1]);
	}
}

WRITE_HANDLER( rthunder_tilebank_select_1_w )
{
	if (tilebank != 1)
	{
		tilebank = 1;
		tilemap_mark_all_tiles_dirty(tilemap[0]);
		tilemap_mark_all_tiles_dirty(tilemap[1]);
	}
}

static void scroll_w(int layer,int offset,int data)
{
	int xdisp[4] = { 36,34,37,35 };
	int ydisp = 9;
	int scrollx,scrolly;


	switch (offset)
	{
		case 0:
			xscroll[layer] = (xscroll[layer]&0xff)|(data<<8);
			break;
		case 1:
			xscroll[layer] = (xscroll[layer]&0xff00)|data;
			break;
		case 2:
			yscroll[layer] = data;
			break;
	}

	scrollx = xscroll[layer]+xdisp[layer];
	scrolly = yscroll[layer]+ydisp;
	if (flipscreen)
	{
		scrollx = -scrollx+256;
		scrolly = -scrolly;
	}
	tilemap_set_scrollx(tilemap[layer],0,scrollx-16);
	tilemap_set_scrolly(tilemap[layer],0,scrolly+16);
}

WRITE_HANDLER( rthunder_scroll0_w )
{
	scroll_w(0,offset,data);
}
WRITE_HANDLER( rthunder_scroll1_w )
{
	scroll_w(1,offset,data);
}
WRITE_HANDLER( rthunder_scroll2_w )
{
	scroll_w(2,offset,data);
}
WRITE_HANDLER( rthunder_scroll3_w )
{
	scroll_w(3,offset,data);
}


WRITE_HANDLER( rthunder_backcolor_w )
{
	backcolor = data;
}


/***************************************************************************

  Display refresh

***************************************************************************/

static void draw_sprites( struct mame_bitmap *bitmap, const struct rectangle *cliprect, int sprite_priority )
{
	/* note: sprites don't yet clip at the top of the screen properly */
	const unsigned char *source = &spriteram[0x1400];
	const unsigned char *finish = &spriteram[0x1c00-16];	/* the last is NOT a sprite */

	int sprite_xoffs = spriteram[0x1bf5] - 256 * (spriteram[0x1bf4] & 1);
	int sprite_yoffs = spriteram[0x1bf7] - 256 * (spriteram[0x1bf6] & 1);

	while( source<finish )
	{
/*
	source[4]	S-FT -BBB
	source[5]	TTTT TTTT
	source[6]   CCCC CCCX
	source[7]	XXXX XXXX
	source[8]	PPPT -S-F
	source[9]   YYYY YYYY
*/
		unsigned char priority = source[8];
		if( priority>>5 == sprite_priority )
		{
			unsigned char attrs = source[4];
			unsigned char color = source[6];
			int sx = source[7] + (color&1)*256; /* need adjust for left clip */
			int sy = -source[9];
			int flipx = attrs&0x20;
			int flipy = priority & 0x01;
			int tall = (priority&0x04)?1:0;
			int wide = (attrs&0x80)?1:0;
			int sprite_bank = attrs&7;
			int sprite_number = (source[5]&0xff)*4;
			int row,col;

			if ((attrs & 0x10) && !wide) sprite_number += 1;
			if ((priority & 0x10) && !tall) sprite_number += 2;
			color = color>>1;

			if (sx>512-32) sx -= 512;
			if (sy < -209-16) sy += 256;

			if (flipx && !wide) sx-=16;
			if (!tall) sy+=16;
/*			if (flipy && !tall) sy+=16;*/

			sx += sprite_xoffs;
			sy -= sprite_yoffs;

			for( row=0; row<=tall; row++ )
			{
				for( col=0; col<=wide; col++ )
				{
					if (flipscreen)
					{
						drawgfx( bitmap, Machine->gfx[GFX_SPRITES+sprite_bank],
							sprite_number+2*row+col,
							color,
							!flipx,!flipy,
							512-16-67 - (sx+16*(flipx?1-col:col)),
							64-16+209 - (sy+16*(flipy?1-row:row)),
							cliprect,
							TRANSPARENCY_PEN, 0xf );
					}
					else
					{
						drawgfx( bitmap, Machine->gfx[GFX_SPRITES+sprite_bank],
							sprite_number+2*row+col,
							color,
							flipx,flipy,
							-67 + (sx+16*(flipx?1-col:col)),
							209 + (sy+16*(flipy?1-row:row)),
							cliprect,
							TRANSPARENCY_PEN, 0xf );
					}
				}
			}
		}
		source+=16;
	}
}



VIDEO_UPDATE( namcos86 )
{
	int layer;

	/* this is the global sprite Y offset, actually */
	flipscreen = spriteram[0x1bf6] & 1;

	tilemap_set_flip(ALL_TILEMAPS,flipscreen ? (TILEMAP_FLIPY | TILEMAP_FLIPX) : 0);

	fillbitmap(bitmap,Machine->gfx[0]->colortable[8*backcolor+7],cliprect);

	for (layer = 0;layer < 8;layer++)
	{
		int i;

		for (i = 3;i >= 0;i--)
		{
			if (((xscroll[i] & 0x0e00) >> 9) == layer)
				tilemap_draw(bitmap,cliprect,tilemap[i],0,0);
		}

		draw_sprites(bitmap,cliprect,layer);
	}
#if 0
{
	char buf[80];
int b=keyboard_pressed(KEYCODE_Y)?8:0;
	sprintf(buf,"%02x %02x %02x %02x %02x %02x %02x %02x",
			spriteram[0x1bf0+b],
			spriteram[0x1bf1+b],
			spriteram[0x1bf2+b],
			spriteram[0x1bf3+b],
			spriteram[0x1bf4+b],
			spriteram[0x1bf5+b],
			spriteram[0x1bf6+b],
			spriteram[0x1bf7+b]);
	usrintf_showmessage(buf);
}
#endif
}
