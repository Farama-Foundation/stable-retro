/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

extern int ddrible_int_enable_0;
extern int ddrible_int_enable_1;

unsigned char *ddrible_fg_videoram;
unsigned char *ddrible_bg_videoram;
unsigned char *ddrible_spriteram_1;
unsigned char *ddrible_spriteram_2;

static int ddribble_vregs[2][5];
static int charbank[2];

static struct tilemap *fg_tilemap,*bg_tilemap;


PALETTE_INIT( ddrible )
{
	int i;
	#define TOTAL_COLORS(gfxn) (Machine->gfx[gfxn]->total_colors * Machine->gfx[gfxn]->color_granularity)
	#define COLOR(gfxn,offs) (colortable[Machine->drv->gfxdecodeinfo[gfxn].color_codes_start + offs])


	/* build the lookup table for sprites. Palette is dynamic. */
	for (i = 0;i < TOTAL_COLORS(3);i++)
		COLOR(3,i) = (*(color_prom++) & 0x0f);
}

WRITE_HANDLER( K005885_0_w )
{
	switch (offset){
		case 0x03:	/* char bank selection for set 1 */
			if ((data & 0x03) != charbank[0])
			{
				charbank[0] = data & 0x03;
				tilemap_mark_all_tiles_dirty( fg_tilemap );
			}
			break;
		case 0x04:	/* IRQ control, flipscreen */
			ddrible_int_enable_0 = data & 0x02;
			break;
	}
	ddribble_vregs[0][offset] = data;
}

WRITE_HANDLER( K005885_1_w )
{
	switch (offset){
		case 0x03:	/* char bank selection for set 2 */
			if ((data & 0x03) != charbank[1])
			{
				charbank[1] = data & 0x03;
				tilemap_mark_all_tiles_dirty( bg_tilemap );
			}
			break;
		case 0x04:	/* IRQ control, flipscreen */
			ddrible_int_enable_1 = data & 0x02;
			break;
	}
	ddribble_vregs[1][offset] = data;
}

/***************************************************************************

	Callbacks for the TileMap code

***************************************************************************/

static UINT32 tilemap_scan(UINT32 col,UINT32 row,UINT32 num_cols,UINT32 num_rows)
{
	/* logical (col,row) -> memory offset */
	return (col & 0x1f) + ((row & 0x1f) << 5) + ((col & 0x20) << 6);	/* skip 0x400 */
}

static void get_fg_tile_info(int tile_index)
{
	unsigned char attr = ddrible_fg_videoram[tile_index];
	int num = ddrible_fg_videoram[tile_index + 0x400] +
			((attr & 0xc0) << 2) + ((attr & 0x20) << 5) + ((charbank[0] & 2) << 10);
	SET_TILE_INFO(
			0,
			num,
			0,
			TILE_FLIPYX((attr & 0x30) >> 4))
}

static void get_bg_tile_info(int tile_index)
{
	unsigned char attr = ddrible_bg_videoram[tile_index];
	int num = ddrible_bg_videoram[tile_index + 0x400] +
			((attr & 0xc0) << 2) + ((attr & 0x20) << 5) + (charbank[1] << 11);
	SET_TILE_INFO(
			1,
			num,
			0,
			TILE_FLIPYX((attr & 0x30) >> 4))
}

/***************************************************************************

	Start the video hardware emulation.

***************************************************************************/

VIDEO_START( ddrible )
{
	fg_tilemap = tilemap_create(get_fg_tile_info,tilemap_scan,TILEMAP_TRANSPARENT,8,8,64,32);
	bg_tilemap = tilemap_create(get_bg_tile_info,tilemap_scan,TILEMAP_OPAQUE,     8,8,64,32);

	if (!fg_tilemap || !bg_tilemap)
		return 1;

	tilemap_set_transparent_pen(fg_tilemap,0);

	return 0;
}

/***************************************************************************

	Memory handlers

***************************************************************************/

WRITE_HANDLER( ddrible_fg_videoram_w )
{
	if (ddrible_fg_videoram[offset] != data)
	{
		ddrible_fg_videoram[offset] = data;
		tilemap_mark_tile_dirty(fg_tilemap,offset & 0xbff);
	}
}

WRITE_HANDLER( ddrible_bg_videoram_w )
{
	if (ddrible_bg_videoram[offset] != data)
	{
		ddrible_bg_videoram[offset] = data;
		tilemap_mark_tile_dirty(bg_tilemap,offset & 0xbff);
	}
}

/***************************************************************************

	Double Dribble sprites

Each sprite has 5 bytes:
byte #0:	sprite number
byte #1:
	bits 0..2:	sprite bank #
	bit 3:		not used?
	bits 4..7:	sprite color
byte #2:	y position
byte #3:	x position
byte #4:	attributes
	bit 0:		x position (high bit)
	bit 1:		???
	bits 2..4:	sprite size
	bit 5:		flip x
	bit 6:		flip y
	bit 7:		unused?

***************************************************************************/

static void ddribble_draw_sprites( struct mame_bitmap *bitmap, const struct rectangle *cliprect, unsigned char* source, int lenght, int gfxset, int flipscreen )
{
	struct GfxElement *gfx = Machine->gfx[gfxset];
	const unsigned char *finish = source + lenght;

	while( source < finish )
	{
		int number = source[0] | ((source[1] & 0x07) << 8);	/* sprite number */
		int attr = source[4];								/* attributes */
		int sx = source[3] | ((attr & 0x01) << 8);			/* vertical position */
		int sy = source[2];									/* horizontal position */
		int flipx = attr & 0x20;							/* flip x */
		int flipy = attr & 0x40;							/* flip y */
		int color = (source[1] & 0xf0) >> 4;				/* color */
		int width,height;

		if (flipscreen){
				flipx = !flipx;
				flipy = !flipy;
				sx = 240 - sx;
				sy = 240 - sy;

				if ((attr & 0x1c) == 0x10){	/* ???. needed for some sprites in flipped mode */
					sx -= 0x10;
					sy -= 0x10;
				}
		}

		switch (attr & 0x1c){
			case 0x10:	/* 32x32 */
				width = height = 2; number &= (~3); break;
			case 0x08:	/* 16x32 */
				width = 1; height = 2; number &= (~2); break;
			case 0x04:	/* 32x16 */
				width = 2; height = 1; number &= (~1); break;
			/* the hardware allow more sprite sizes, but ddribble doesn't use them */
			default:	/* 16x16 */
				width = height = 1; break;
		}

		{
			static int x_offset[2] = { 0x00, 0x01 };
			static int y_offset[2] = { 0x00, 0x02 };
			int x,y, ex, ey;

			for( y=0; y < height; y++ ){
				for( x=0; x < width; x++ ){
					ex = flipx ? (width-1-x) : x;
					ey = flipy ? (height-1-y) : y;

					drawgfx(bitmap,gfx,
						(number)+x_offset[ex]+y_offset[ey],
						color,
						flipx, flipy,
						sx+x*16,sy+y*16,
						cliprect,
						TRANSPARENCY_PEN, 0);
				}
			}
		}
		source += 5;
	}
}

/***************************************************************************

	Display Refresh

***************************************************************************/

VIDEO_UPDATE( ddrible )
{
	tilemap_set_flip(fg_tilemap, (ddribble_vregs[0][4] & 0x08) ? (TILEMAP_FLIPY | TILEMAP_FLIPX) : 0);
	tilemap_set_flip(bg_tilemap, (ddribble_vregs[1][4] & 0x08) ? (TILEMAP_FLIPY | TILEMAP_FLIPX) : 0);

	/* set scroll registers */
	tilemap_set_scrollx(fg_tilemap,0,ddribble_vregs[0][1] | ((ddribble_vregs[0][2] & 0x01) << 8));
	tilemap_set_scrollx(bg_tilemap,0,ddribble_vregs[1][1] | ((ddribble_vregs[1][2] & 0x01) << 8));
	tilemap_set_scrolly(fg_tilemap,0,ddribble_vregs[0][0]);
	tilemap_set_scrolly(bg_tilemap,0,ddribble_vregs[1][0]);

	tilemap_draw(bitmap,cliprect,bg_tilemap,0,0);
	ddribble_draw_sprites(bitmap,cliprect,ddrible_spriteram_1,0x07d,2,ddribble_vregs[0][4] & 0x08);
	ddribble_draw_sprites(bitmap,cliprect,ddrible_spriteram_2,0x140,3,ddribble_vregs[1][4] & 0x08);
	tilemap_draw(bitmap,cliprect,fg_tilemap,0,0);
}
