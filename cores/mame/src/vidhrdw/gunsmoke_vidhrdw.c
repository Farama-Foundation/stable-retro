/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"



unsigned char *gunsmoke_bg_scrolly;
unsigned char *gunsmoke_bg_scrollx;
static int chon,objon,bgon;
static int sprite3bank;

static struct mame_bitmap * bgbitmap;
static unsigned char bgmap[9][9][2];


/***************************************************************************

  Convert the color PROMs into a more useable format.

  Gunsmoke has three 256x4 palette PROMs (one per gun) and a lot ;-) of
  256x4 lookup table PROMs.
  The palette PROMs are connected to the RGB output this way:

  bit 3 -- 220 ohm resistor  -- RED/GREEN/BLUE
        -- 470 ohm resistor  -- RED/GREEN/BLUE
        -- 1  kohm resistor  -- RED/GREEN/BLUE
  bit 0 -- 2.2kohm resistor  -- RED/GREEN/BLUE

***************************************************************************/
PALETTE_INIT( gunsmoke )
{
	int i;
	#define TOTAL_COLORS(gfxn) (Machine->gfx[gfxn]->total_colors * Machine->gfx[gfxn]->color_granularity)
	#define COLOR(gfxn,offs) (colortable[Machine->drv->gfxdecodeinfo[gfxn].color_codes_start + offs])


	for (i = 0;i < Machine->drv->total_colors;i++)
	{
		int bit0,bit1,bit2,bit3,r,g,b;


		bit0 = (color_prom[0] >> 0) & 0x01;
		bit1 = (color_prom[0] >> 1) & 0x01;
		bit2 = (color_prom[0] >> 2) & 0x01;
		bit3 = (color_prom[0] >> 3) & 0x01;
		r = 0x0e * bit0 + 0x1f * bit1 + 0x43 * bit2 + 0x8f * bit3;
		bit0 = (color_prom[Machine->drv->total_colors] >> 0) & 0x01;
		bit1 = (color_prom[Machine->drv->total_colors] >> 1) & 0x01;
		bit2 = (color_prom[Machine->drv->total_colors] >> 2) & 0x01;
		bit3 = (color_prom[Machine->drv->total_colors] >> 3) & 0x01;
		g = 0x0e * bit0 + 0x1f * bit1 + 0x43 * bit2 + 0x8f * bit3;
		bit0 = (color_prom[2*Machine->drv->total_colors] >> 0) & 0x01;
		bit1 = (color_prom[2*Machine->drv->total_colors] >> 1) & 0x01;
		bit2 = (color_prom[2*Machine->drv->total_colors] >> 2) & 0x01;
		bit3 = (color_prom[2*Machine->drv->total_colors] >> 3) & 0x01;
		b = 0x0e * bit0 + 0x1f * bit1 + 0x43 * bit2 + 0x8f * bit3;

		palette_set_color(i,r,g,b);
		color_prom++;
	}

	color_prom += 2*Machine->drv->total_colors;
	/* color_prom now points to the beginning of the lookup table */

	/* characters use colors 64-79 */
	for (i = 0;i < TOTAL_COLORS(0);i++)
		COLOR(0,i) = *(color_prom++) + 64;
	color_prom += 128;	/* skip the bottom half of the PROM - not used */

	/* background tiles use colors 0-63 */
	for (i = 0;i < TOTAL_COLORS(1);i++)
	{
		COLOR(1,i) = color_prom[0] + 16 * (color_prom[256] & 0x03);
		color_prom++;
	}
	color_prom += TOTAL_COLORS(1);

	/* sprites use colors 128-255 */
	for (i = 0;i < TOTAL_COLORS(2);i++)
	{
		COLOR(2,i) = color_prom[0] + 16 * (color_prom[256] & 0x07) + 128;
		color_prom++;
	}
	color_prom += TOTAL_COLORS(2);
}



VIDEO_START( gunsmoke )
{
	if ((bgbitmap = auto_bitmap_alloc(9*32,9*32)) == 0)
		return 1;

	if (video_start_generic() == 1)
		return 1;

	memset (bgmap, 0xff, sizeof (bgmap));

	return 0;
}



WRITE_HANDLER( gunsmoke_c804_w )
{
	int bankaddress;
	unsigned char *RAM = memory_region(REGION_CPU1);


	/* bits 0 and 1 are for coin counters */
	coin_counter_w(1,data & 1);
	coin_counter_w(0,data & 2);

	/* bits 2 and 3 select the ROM bank */
	bankaddress = 0x10000 + (data & 0x0c) * 0x1000;
	cpu_setbank(1,&RAM[bankaddress]);

	/* bit 5 resets the sound CPU? - we ignore it */

	/* bit 6 flips screen */
	flip_screen_set(data & 0x40);

	/* bit 7 enables characters? */
	chon = data & 0x80;
}



WRITE_HANDLER( gunsmoke_d806_w )
{
	/* bits 0-2 select the sprite 3 bank */
	sprite3bank = data & 0x07;

	/* bit 4 enables bg 1? */
	bgon = data & 0x10;

	/* bit 5 enables sprites? */
	objon = data & 0x20;
}



/***************************************************************************

  Draw the game screen in the given mame_bitmap.
  Do NOT call osd_update_display() from this function, it will be called by
  the main emulation engine.

***************************************************************************/
VIDEO_UPDATE( gunsmoke )
{
	int offs,sx,sy;
	int bg_scrolly, bg_scrollx;
	unsigned char *p=memory_region(REGION_GFX4);
	int top,left;


	if (get_vh_global_attribute_changed())
		memset (bgmap, 0xff, sizeof (bgmap));


	if (bgon)
	{
		bg_scrolly = gunsmoke_bg_scrolly[0] + 256 * gunsmoke_bg_scrolly[1];
		bg_scrollx = gunsmoke_bg_scrollx[0];
		offs = 16 * ((bg_scrolly>>5)+8)+2*(bg_scrollx>>5) ;
		if (bg_scrollx & 0x80) offs -= 0x10;

		top = 8 - (bg_scrolly>>5) % 9;
		left = (bg_scrollx>>5) % 9;

		bg_scrolly&=0x1f;
		bg_scrollx&=0x1f;

		for (sy = 0;sy <9;sy++)
		{
			int ty = (sy + top) % 9;
			offs &= 0x7fff; /* Enforce limits (for top of scroll) */

			for (sx = 0;sx < 9;sx++)
			{
				int offset;
				int tx = (sx + left) % 9;
				unsigned char *map = &bgmap[ty][tx][0];
				offset=offs+(sx*2);

				if (p[offset] != map[0] || p[offset+1] != map[1])
				{
					int code,col,flipx,flipy;

					map[0] = p[offset];
					map[1] = p[offset+1];

					code = p[offset] + 256*(p[offset+1] & 1);
					col = (p[offset+1] & 0x3c) >> 2;

					flipx = p[offset+1] & 0x40;
					flipy = p[offset+1] & 0x80;

					if (flip_screen)
					{
						tx = 8 - tx;
						ty = 8 - ty;
						flipx = !flipx;
						flipy = !flipy;
					}

					drawgfx(bgbitmap,Machine->gfx[1],
							code,col,
							flipx,flipy,
							(8-ty)*32, tx*32,
							0,TRANSPARENCY_NONE,0);
				}
			}
			offs-=0x10;
		}

		{
			int xscroll,yscroll;

			xscroll = (top*32-bg_scrolly);
			yscroll = -(left*32+bg_scrollx);

			if (flip_screen)
			{
				xscroll = 256 - xscroll;
				yscroll = 256 - yscroll;
			}

			copyscrollbitmap(bitmap,bgbitmap,1,&xscroll,1,&yscroll,&Machine->visible_area,TRANSPARENCY_NONE,0);
		}
	}
	else
		fillbitmap(bitmap,Machine->pens[0],&Machine->visible_area);



	if (objon)
	{
		/* Draw the sprites. */
		for (offs = spriteram_size - 32;offs >= 0;offs -= 32)
		{
			int bank,flipx,flipy;


			bank = (spriteram[offs + 1] & 0xc0) >> 6;
			if (bank == 3) bank += sprite3bank;

			sx = spriteram[offs + 3] - ((spriteram[offs + 1] & 0x20) << 3);
 			sy = spriteram[offs + 2];
			flipx = 0;
			flipy = spriteram[offs + 1] & 0x10;
			if (flip_screen)
			{
				sx = 240 - sx;
				sy = 240 - sy;
				flipx = !flipx;
				flipy = !flipy;
			}

			drawgfx(bitmap,Machine->gfx[2],
					spriteram[offs] + 256 * bank,
					spriteram[offs + 1] & 0x0f,
					flipx,flipy,
					sx,sy,
					&Machine->visible_area,TRANSPARENCY_PEN,0);
		}
	}


	if (chon)
	{
		/* draw the frontmost playfield. They are characters, but draw them as sprites */
		for (offs = videoram_size - 1;offs >= 0;offs--)
		{
			sx = offs % 32;
			sy = offs / 32;
			if (flip_screen)
			{
				sx = 31 - sx;
				sy = 31 - sy;
			}

			drawgfx(bitmap,Machine->gfx[0],
					videoram[offs] + ((colorram[offs] & 0xc0) << 2),
					colorram[offs] & 0x1f,
					!flip_screen,!flip_screen,
					8*sx,8*sy,
					&Machine->visible_area,TRANSPARENCY_COLOR,79);
		}
	}
}
