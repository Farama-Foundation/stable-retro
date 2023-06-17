/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"


#define TOP_MONITOR_ROWS 28
#define BOTTOM_MONITOR_ROWS 28

#define BIGSPRITE_WIDTH 128
#define BIGSPRITE_HEIGHT 256
#define ARMWREST_BIGSPRITE_WIDTH 256
#define ARMWREST_BIGSPRITE_HEIGHT 128

unsigned char *punchout_videoram2;
size_t punchout_videoram2_size;
unsigned char *punchout_bigsprite1ram;
size_t punchout_bigsprite1ram_size;
unsigned char *punchout_bigsprite2ram;
size_t punchout_bigsprite2ram_size;
unsigned char *punchout_scroll;
unsigned char *punchout_bigsprite1;
unsigned char *punchout_bigsprite2;
unsigned char *punchout_palettebank;
static unsigned char *dirtybuffer2,*bs1dirtybuffer,*bs2dirtybuffer;
static struct mame_bitmap *bs1tmpbitmap,*bs2tmpbitmap;

static int top_palette_bank,bottom_palette_bank;

static struct rectangle topvisiblearea =
{
	0*8, 32*8-1,
	0*8, TOP_MONITOR_ROWS*8-1
};
static struct rectangle bottomvisiblearea =
{
	0*8, 32*8-1,
	TOP_MONITOR_ROWS*8, (TOP_MONITOR_ROWS+BOTTOM_MONITOR_ROWS)*8-1
};
static struct rectangle backgroundvisiblearea =
{
	0*8, 64*8-1,
	TOP_MONITOR_ROWS*8, (TOP_MONITOR_ROWS+BOTTOM_MONITOR_ROWS)*8-1
};



/***************************************************************************

  Convert the color PROMs into a more useable format.

  Punch Out has a six 512x4 palette PROMs (one per gun; three for the top
  monitor chars, three for everything else).
  The PROMs are connected to the RGB output this way:

  bit 3 -- 240 ohm resistor -- inverter  -- RED/GREEN/BLUE
        -- 470 ohm resistor -- inverter  -- RED/GREEN/BLUE
        -- 1  kohm resistor -- inverter  -- RED/GREEN/BLUE
  bit 0 -- 2  kohm resistor -- inverter  -- RED/GREEN/BLUE

***************************************************************************/
static void convert_palette(const unsigned char *color_prom)
{
	int i;


	for (i = 0;i < 1024;i++)
	{
		int bit0,bit1,bit2,bit3,r,g,b;


		bit0 = (color_prom[0] >> 0) & 0x01;
		bit1 = (color_prom[0] >> 1) & 0x01;
		bit2 = (color_prom[0] >> 2) & 0x01;
		bit3 = (color_prom[0] >> 3) & 0x01;
		r = 255 - (0x10 * bit0 + 0x21 * bit1 + 0x46 * bit2 + 0x88 * bit3);
		bit0 = (color_prom[1024] >> 0) & 0x01;
		bit1 = (color_prom[1024] >> 1) & 0x01;
		bit2 = (color_prom[1024] >> 2) & 0x01;
		bit3 = (color_prom[1024] >> 3) & 0x01;
		g = 255 - (0x10 * bit0 + 0x21 * bit1 + 0x46 * bit2 + 0x88 * bit3);
		bit0 = (color_prom[2*1024] >> 0) & 0x01;
		bit1 = (color_prom[2*1024] >> 1) & 0x01;
		bit2 = (color_prom[2*1024] >> 2) & 0x01;
		bit3 = (color_prom[2*1024] >> 3) & 0x01;
		b = 255 - (0x10 * bit0 + 0x21 * bit1 + 0x46 * bit2 + 0x88 * bit3);

		palette_set_color(i,r,g,b);
		color_prom++;
	}

	/* reserve the last color for the transparent pen (none of the game colors has */
	/* these RGB components) */
	palette_set_color(1024,240,240,240);
}


/* these depend on jumpers on the board and change from game to game */
static int gfx0inv,gfx1inv,gfx2inv,gfx3inv;

PALETTE_INIT( punchout )
{
	int i;
	#define TOTAL_COLORS(gfxn) (Machine->gfx[gfxn]->total_colors * Machine->gfx[gfxn]->color_granularity)
	#define COLOR(gfxn,offs) (colortable[Machine->drv->gfxdecodeinfo[gfxn].color_codes_start + (offs)])


	convert_palette(color_prom);


	/* top monitor chars */
	for (i = 0;i < TOTAL_COLORS(0);i++)
		COLOR(0,i ^ gfx0inv) = i;

	/* bottom monitor chars */
	for (i = 0;i < TOTAL_COLORS(1);i++)
		COLOR(1,i ^ gfx1inv) = i + 512;

	/* big sprite #1 */
	for (i = 0;i < TOTAL_COLORS(2);i++)
	{
		if (i % 8 == 0) COLOR(2,i ^ gfx2inv) = 1024;	/* transparent */
		else COLOR(2,i ^ gfx2inv) = i + 512;
	}

	/* big sprite #2 */
	for (i = 0;i < TOTAL_COLORS(3);i++)
	{
		if (i % 4 == 0) COLOR(3,i ^ gfx3inv) = 1024;	/* transparent */
		else COLOR(3,i ^ gfx3inv) = i + 512;
	}
}

PALETTE_INIT( armwrest )
{
	int i;
	#define TOTAL_COLORS(gfxn) (Machine->gfx[gfxn]->total_colors * Machine->gfx[gfxn]->color_granularity)
	#define COLOR(gfxn,offs) (colortable[Machine->drv->gfxdecodeinfo[gfxn].color_codes_start + (offs)])


	convert_palette(color_prom);


	/* top monitor / bottom monitor backround chars */
	for (i = 0;i < TOTAL_COLORS(0);i++)
		COLOR(0,i) = i;

	/* bottom monitor foreground chars */
	for (i = 0;i < TOTAL_COLORS(1);i++)
		COLOR(1,i) = i + 512;

	/* big sprite #1 */
	for (i = 0;i < TOTAL_COLORS(2);i++)
	{
		if (i % 8 == 7) COLOR(2,i) = 1024;	/* transparent */
		else COLOR(2,i) = i + 512;
	}

	/* big sprite #2 - pen order is inverted */
	for (i = 0;i < TOTAL_COLORS(3);i++)
	{
		if (i % 4 == 3) COLOR(3,i ^ 3) = 1024;	/* transparent */
		else COLOR(3,i ^ 3) = i + 512;
	}
}



static void gfx_fix(void)
{
	/* one graphics ROM (4v) doesn't */
	/* exist but must be seen as a 0xff fill for colors to come out properly */
	memset(memory_region(REGION_GFX3) + 0x2c000,0xff,0x4000);
}

DRIVER_INIT( punchout )
{
	gfx_fix();

	gfx0inv = 0x03;
	gfx1inv = 0xfc;
	gfx2inv = 0xff;
	gfx3inv = 0xfc;
}

DRIVER_INIT( spnchout )
{
	gfx_fix();

	gfx0inv = 0x00;
	gfx1inv = 0xff;
	gfx2inv = 0xff;
	gfx3inv = 0xff;
}

DRIVER_INIT( spnchotj )
{
	gfx_fix();

	gfx0inv = 0xfc;
	gfx1inv = 0xff;
	gfx2inv = 0xff;
	gfx3inv = 0xff;
}

DRIVER_INIT( armwrest )
{
	gfx_fix();

	/* also, ROM 2k is enabled only when its top half is accessed. The other half must */
	/* be seen as a 0xff fill for colors to come out properly */
	memset(memory_region(REGION_GFX2) + 0x08000,0xff,0x2000);
}




/***************************************************************************

  Start the video hardware emulation.

***************************************************************************/
VIDEO_START( punchout )
{
	if ((dirtybuffer = auto_malloc(videoram_size)) == 0)
		return 1;
	memset(dirtybuffer,1,videoram_size);

	if ((dirtybuffer2 = auto_malloc(punchout_videoram2_size)) == 0)
		return 1;
	memset(dirtybuffer2,1,punchout_videoram2_size);

	if ((tmpbitmap = auto_bitmap_alloc(512,480)) == 0)
		return 1;

	if ((bs1dirtybuffer = auto_malloc(punchout_bigsprite1ram_size)) == 0)
		return 1;
	memset(bs1dirtybuffer,1,punchout_bigsprite1ram_size);

	if ((bs1tmpbitmap = auto_bitmap_alloc(BIGSPRITE_WIDTH,BIGSPRITE_HEIGHT)) == 0)
		return 1;

	if ((bs2dirtybuffer = auto_malloc(punchout_bigsprite2ram_size)) == 0)
		return 1;
	memset(bs2dirtybuffer,1,punchout_bigsprite2ram_size);

	if ((bs2tmpbitmap = auto_bitmap_alloc(BIGSPRITE_WIDTH,BIGSPRITE_HEIGHT)) == 0)
		return 1;

	return 0;
}

VIDEO_START( armwrest )
{
	if ((dirtybuffer = auto_malloc(videoram_size)) == 0)
		return 1;
	memset(dirtybuffer,1,videoram_size);

	if ((dirtybuffer2 = auto_malloc(punchout_videoram2_size)) == 0)
		return 1;
	memset(dirtybuffer2,1,punchout_videoram2_size);

	if ((tmpbitmap = auto_bitmap_alloc(512,480)) == 0)
		return 1;

	if ((bs1dirtybuffer = auto_malloc(punchout_bigsprite1ram_size)) == 0)
		return 1;
	memset(bs1dirtybuffer,1,punchout_bigsprite1ram_size);

	if ((bs1tmpbitmap = auto_bitmap_alloc(ARMWREST_BIGSPRITE_WIDTH,ARMWREST_BIGSPRITE_HEIGHT)) == 0)
		return 1;

	if ((bs2dirtybuffer = auto_malloc(punchout_bigsprite2ram_size)) == 0)
		return 1;
	memset(bs2dirtybuffer,1,punchout_bigsprite2ram_size);

	if ((bs2tmpbitmap = auto_bitmap_alloc(BIGSPRITE_WIDTH,BIGSPRITE_HEIGHT)) == 0)
		return 1;

	return 0;
}



WRITE_HANDLER( punchout_videoram2_w )
{
	if (punchout_videoram2[offset] != data)
	{
		dirtybuffer2[offset] = 1;

		punchout_videoram2[offset] = data;
	}
}

WRITE_HANDLER( punchout_bigsprite1ram_w )
{
	if (punchout_bigsprite1ram[offset] != data)
	{
		bs1dirtybuffer[offset] = 1;

		punchout_bigsprite1ram[offset] = data;
	}
}

WRITE_HANDLER( punchout_bigsprite2ram_w )
{
	if (punchout_bigsprite2ram[offset] != data)
	{
		bs2dirtybuffer[offset] = 1;

		punchout_bigsprite2ram[offset] = data;
	}
}



WRITE_HANDLER( punchout_palettebank_w )
{
	*punchout_palettebank = data;

	if (top_palette_bank != ((data >> 1) & 0x01))
	{
		top_palette_bank = (data >> 1) & 0x01;
		memset(dirtybuffer,1,videoram_size);
	}
	if (bottom_palette_bank != ((data >> 0) & 0x01))
	{
		bottom_palette_bank = (data >> 0) & 0x01;
		memset(dirtybuffer2,1,punchout_videoram2_size);
		memset(bs1dirtybuffer,1,punchout_bigsprite1ram_size);
		memset(bs2dirtybuffer,1,punchout_bigsprite2ram_size);
	}
}



/***************************************************************************

  Draw the game screen in the given mame_bitmap.
  Do NOT call osd_update_display() from this function, it will be called by
  the main emulation engine.

***************************************************************************/
VIDEO_UPDATE( punchout )
{
	int offs;


	/* for every character in the Video RAM, check if it has been modified */
	/* since last time and update it accordingly. */
	for (offs = videoram_size - 2;offs >= 0;offs -= 2)
	{
		if (dirtybuffer[offs] || dirtybuffer[offs + 1])
		{
			int sx,sy;


			dirtybuffer[offs] = 0;
			dirtybuffer[offs + 1] = 0;

			sx = offs/2 % 32;
			sy = offs/2 / 32;

			drawgfx(tmpbitmap,Machine->gfx[0],
					videoram[offs] + 256 * (videoram[offs + 1] & 0x03),
					((videoram[offs + 1] & 0x7c) >> 2) + 64 * top_palette_bank,
					videoram[offs + 1] & 0x80,0,
					8*sx,8*sy - 16,
					&topvisiblearea,TRANSPARENCY_NONE,0);
		}
	}

	for (offs = punchout_videoram2_size - 2;offs >= 0;offs -= 2)
	{
		if (dirtybuffer2[offs] | dirtybuffer2[offs + 1])
		{
			int sx,sy;


			dirtybuffer2[offs] = 0;
			dirtybuffer2[offs + 1] = 0;

			sx = offs/2 % 64;
			sy = offs/2 / 64;

			drawgfx(tmpbitmap,Machine->gfx[1],
					punchout_videoram2[offs] + 256 * (punchout_videoram2[offs + 1] & 0x03),
					((punchout_videoram2[offs + 1] & 0x7c) >> 2) + 64 * bottom_palette_bank,
					punchout_videoram2[offs + 1] & 0x80,0,
					8*sx,8*sy + 8*TOP_MONITOR_ROWS - 16,
					&backgroundvisiblearea,TRANSPARENCY_NONE,0);
		}
	}

	for (offs = punchout_bigsprite1ram_size - 4;offs >= 0;offs -= 4)
	{
		if (bs1dirtybuffer[offs] | bs1dirtybuffer[offs + 1] | bs1dirtybuffer[offs + 3])
		{
			int sx,sy;


			bs1dirtybuffer[offs] = 0;
			bs1dirtybuffer[offs + 1] = 0;
			bs1dirtybuffer[offs + 3] = 0;

			sx = offs/4 % 16;
			sy = offs/4 / 16;

			drawgfx(bs1tmpbitmap,Machine->gfx[2],
					punchout_bigsprite1ram[offs] + 256 * (punchout_bigsprite1ram[offs + 1] & 0x1f),
					(punchout_bigsprite1ram[offs + 3] & 0x1f) + 32 * bottom_palette_bank,
					punchout_bigsprite1ram[offs + 3] & 0x80,0,
					8*sx,8*sy,
					0,TRANSPARENCY_NONE,0);
		}
	}

	for (offs = punchout_bigsprite2ram_size - 4;offs >= 0;offs -= 4)
	{
		if (bs2dirtybuffer[offs] | bs2dirtybuffer[offs + 1] | bs2dirtybuffer[offs + 3])
		{
			int sx,sy;


			bs2dirtybuffer[offs] = 0;
			bs2dirtybuffer[offs + 1] = 0;
			bs2dirtybuffer[offs + 3] = 0;

			sx = offs/4 % 16;
			sy = offs/4 / 16;

			drawgfx(bs2tmpbitmap,Machine->gfx[3],
					punchout_bigsprite2ram[offs] + 256 * (punchout_bigsprite2ram[offs + 1] & 0x0f),
					(punchout_bigsprite2ram[offs + 3] & 0x3f) + 64 * bottom_palette_bank,
					punchout_bigsprite2ram[offs + 3] & 0x80,0,
					8*sx,8*sy,
					0,TRANSPARENCY_NONE,0);
		}
	}


	/* copy the character mapped graphics */
	{
		int scroll[64];


		for (offs = 0;offs < TOP_MONITOR_ROWS;offs++)
			scroll[offs] = 0;
		for (offs = 0;offs < BOTTOM_MONITOR_ROWS;offs++)
			scroll[TOP_MONITOR_ROWS + offs] = -(58 + punchout_scroll[2*(offs+2)] + 256 * (punchout_scroll[2*(offs+2) + 1] & 0x01));

		copyscrollbitmap(bitmap,tmpbitmap,TOP_MONITOR_ROWS + BOTTOM_MONITOR_ROWS,scroll,0,0,&Machine->visible_area,TRANSPARENCY_NONE,0);
	}

	/* copy the two big sprites */
	{
		int zoom;

		zoom = punchout_bigsprite1[0] + 256 * (punchout_bigsprite1[1] & 0x0f);
		if (zoom)
		{
			int sx,sy;
			UINT32 startx,starty;
			int incxx,incyy;

			sx = 4096 - (punchout_bigsprite1[2] + 256 * (punchout_bigsprite1[3] & 0x0f));
			if (sx > 4096-4*127) sx -= 4096;

			sy = -(punchout_bigsprite1[4] + 256 * (punchout_bigsprite1[5] & 1));
			if (sy <= -256 + zoom/0x40) sy += 512;

			incxx = zoom << 6;
			incyy = zoom << 6;

			startx = -sx * 0x4000;
			starty = -sy * 0x10000;
			startx += 3740 * zoom;	/* adjustment to match the screen shots */
			starty -= 178 * zoom;	/* and make the hall of fame picture nice */

			if (punchout_bigsprite1[6] & 1)	/* flip x */
			{
				startx = (bs1tmpbitmap->width << 16) - startx - 1;
				incxx = -incxx;
			}

			if (punchout_bigsprite1[7] & 1)	/* display in top monitor */
			{
				copyrozbitmap(bitmap,bs1tmpbitmap,
					startx,starty + 0x200*(2) * zoom,
					incxx,0,0,incyy,	/* zoom, no rotation */
					0,	/* no wraparound */
					&topvisiblearea,TRANSPARENCY_COLOR,1024,0);
			}
			if (punchout_bigsprite1[7] & 2)	/* display in bottom monitor */
			{
				copyrozbitmap(bitmap,bs1tmpbitmap,
					startx,starty - 0x200*TOP_MONITOR_ROWS * zoom,
					incxx,0,0,incyy,	/* zoom, no rotation */
					0,	/* no wraparound */
					&bottomvisiblearea,TRANSPARENCY_COLOR,1024,0);
			}
		}
	}
	{
		int sx,sy;


		sx = 512 - (punchout_bigsprite2[0] + 256 * (punchout_bigsprite2[1] & 1));
		if (sx > 512-127) sx -= 512;
		sx -= 55;	/* adjustment to match the screen shots */

		sy = -punchout_bigsprite2[2] + 256 * (punchout_bigsprite2[3] & 1);
		sy += 3;	/* adjustment to match the screen shots */

		copybitmap(bitmap,bs2tmpbitmap,
				punchout_bigsprite2[4] & 1,0,
				sx,sy + 8*TOP_MONITOR_ROWS - 16,
				&bottomvisiblearea,TRANSPARENCY_COLOR,1024);
	}
}


VIDEO_UPDATE( armwrest )
{
	int offs;


	/* for every character in the Video RAM, check if it has been modified */
	/* since last time and update it accordingly. */
	for (offs = punchout_videoram2_size - 2;offs >= 0;offs -= 2)
	{
		if (dirtybuffer2[offs] | dirtybuffer2[offs + 1])
		{
			int sx,sy;


			dirtybuffer2[offs] = 0;
			dirtybuffer2[offs + 1] = 0;

			sx = offs/2 % 32;
			sy = offs/2 / 32;

			if (sy >= 32)
			{
				/* top screen */
				sy -= 32;
				drawgfx(tmpbitmap,Machine->gfx[0],
						punchout_videoram2[offs] + 256 * (punchout_videoram2[offs + 1] & 0x03) +
								8 * (punchout_videoram2[offs + 1] & 0x80),
						((punchout_videoram2[offs + 1] & 0x7c) >> 2) + 64 * top_palette_bank,
						0,0,
						8*sx,8*sy - 16,
						&topvisiblearea,TRANSPARENCY_NONE,0);
			}
			else
				/* bottom screen background */
				drawgfx(tmpbitmap,Machine->gfx[0],
						punchout_videoram2[offs] + 256 * (punchout_videoram2[offs + 1] & 0x03),
						128 + ((punchout_videoram2[offs + 1] & 0x7c) >> 2) + 64 * bottom_palette_bank,
						punchout_videoram2[offs + 1] & 0x80,0,
						8*sx,8*sy + 8*TOP_MONITOR_ROWS - 16,
						&backgroundvisiblearea,TRANSPARENCY_NONE,0);
		}
	}

	for (offs = punchout_bigsprite1ram_size - 4;offs >= 0;offs -= 4)
	{
		if (bs1dirtybuffer[offs] | bs1dirtybuffer[offs + 1] | bs1dirtybuffer[offs + 3])
		{
			int sx,sy;


			bs1dirtybuffer[offs] = 0;
			bs1dirtybuffer[offs + 1] = 0;
			bs1dirtybuffer[offs + 3] = 0;

			sx = offs/4 % 16;
			sy = offs/4 / 16;
			if (sy >= 16)
			{
				sy -= 16;
				sx += 16;
			}

			drawgfx(bs1tmpbitmap,Machine->gfx[2],
					punchout_bigsprite1ram[offs] + 256 * (punchout_bigsprite1ram[offs + 1] & 0x1f),
					(punchout_bigsprite1ram[offs + 3] & 0x1f) + 32 * bottom_palette_bank,
					punchout_bigsprite1ram[offs + 3] & 0x80,0,
					8*sx,8*sy,
					0,TRANSPARENCY_NONE,0);
		}
	}

	for (offs = punchout_bigsprite2ram_size - 4;offs >= 0;offs -= 4)
	{
		if (bs2dirtybuffer[offs] | bs2dirtybuffer[offs + 1] | bs2dirtybuffer[offs + 3])
		{
			int sx,sy;


			bs2dirtybuffer[offs] = 0;
			bs2dirtybuffer[offs + 1] = 0;
			bs2dirtybuffer[offs + 3] = 0;

			sx = offs/4 % 16;
			sy = offs/4 / 16;

			drawgfx(bs2tmpbitmap,Machine->gfx[3],
					punchout_bigsprite2ram[offs] + 256 * (punchout_bigsprite2ram[offs + 1] & 0x0f),
					(punchout_bigsprite2ram[offs + 3] & 0x3f) + 64 * bottom_palette_bank,
					punchout_bigsprite2ram[offs + 3] & 0x80,0,
					8*sx,8*sy,
					0,TRANSPARENCY_NONE,0);
		}
	}


	/* copy the character mapped graphics */
	copybitmap(bitmap,tmpbitmap,0,0,0,0,&Machine->visible_area,TRANSPARENCY_NONE,0);


	/* copy the two big sprites */
	{
		int zoom;

		zoom = punchout_bigsprite1[0] + 256 * (punchout_bigsprite1[1] & 0x0f);
		if (zoom)
		{
			int sx,sy;
			UINT32 startx,starty;
			int incxx,incyy;

			sx = 4096 - (punchout_bigsprite1[2] + 256 * (punchout_bigsprite1[3] & 0x0f));
			if (sx > 4096-4*127) sx -= 4096;

			sy = -(punchout_bigsprite1[4] + 256 * (punchout_bigsprite1[5] & 1));
			if (sy <= -256 + zoom/0x40) sy += 512;

			incxx = zoom << 6;
			incyy = zoom << 6;

			startx = -sx * 0x4000;
			starty = -sy * 0x10000;
			startx += 3740 * zoom;	/* adjustment to match the screen shots */
			starty -= 178 * zoom;	/* and make the hall of fame picture nice */

			if (punchout_bigsprite1[6] & 1)	/* flip x */
			{
				startx = (bs1tmpbitmap->width << 16) - startx - 1;
				incxx = -incxx;
			}

			if (punchout_bigsprite1[7] & 1)	/* display in top monitor */
			{
				copyrozbitmap(bitmap,bs1tmpbitmap,
					startx,starty + 0x200*(2) * zoom,
					incxx,0,0,incyy,	/* zoom, no rotation */
					0,	/* no wraparound */
					&topvisiblearea,TRANSPARENCY_COLOR,1024,0);
			}
			if (punchout_bigsprite1[7] & 2)	/* display in bottom monitor */
			{
				copyrozbitmap(bitmap,bs1tmpbitmap,
					startx,starty - 0x200*(TOP_MONITOR_ROWS-2) * zoom,
					incxx,0,0,incyy,	/* zoom, no rotation */
					0,	/* no wraparound */
					&bottomvisiblearea,TRANSPARENCY_COLOR,1024,0);
			}
		}
	}
	{
		int sx,sy;


		sx = 512 - (punchout_bigsprite2[0] + 256 * (punchout_bigsprite2[1] & 1));
		if (sx > 512-127) sx -= 512;
		sx -= 55;	/* adjustment to match the screen shots */

		sy = -punchout_bigsprite2[2] + 256 * (punchout_bigsprite2[3] & 1);
		sy += 3;	/* adjustment to match the screen shots */

		copybitmap(bitmap,bs2tmpbitmap,
				punchout_bigsprite2[4] & 1,0,
				sx,sy + 8*TOP_MONITOR_ROWS - 16,
				&bottomvisiblearea,TRANSPARENCY_COLOR,1024);
	}


	/* draw the foregound chars */
	for (offs = videoram_size - 2;offs >= 0;offs -= 2)
	{
		int sx,sy;


		dirtybuffer[offs] = 0;
		dirtybuffer[offs + 1] = 0;

		sx = offs/2 % 32;
		sy = offs/2 / 32;

		drawgfx(bitmap,Machine->gfx[1],
				videoram[offs] + 256 * (videoram[offs + 1] & 0x07),
				((videoram[offs + 1] & 0xf8) >> 3) + 32 * bottom_palette_bank,
				videoram[offs + 1] & 0x80,0,
				8*sx,8*sy + 8*TOP_MONITOR_ROWS - 16,
				&backgroundvisiblearea,TRANSPARENCY_PEN,7);
	}
}
