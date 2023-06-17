/***************************************************************************

	Atari Cloud 9 (prototype) hardware

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "cloud9.h"


unsigned char *cloud9_vram2;
unsigned char *cloud9_bitmap_regs;
unsigned char *cloud9_auto_inc_x;
unsigned char *cloud9_auto_inc_y;
unsigned char *cloud9_both_banks;
unsigned char *cloud9_vram_bank;
unsigned char *cloud9_color_bank;


/***************************************************************************
  cloud9_paletteram_w

  Cloud 9 uses 9-bit color, in the form RRRGGGBB, with the LSB of B stored
  in the $40 bit of the address.
***************************************************************************/
WRITE_HANDLER( cloud9_paletteram_w )
{
	int bit0,bit1,bit2;
	int r,g,b;
	int blue;


	paletteram[(offset & 0x3f)] = data;
	blue = (offset & 0x40);

	/* red component */
	bit0 = (~data >> 5) & 0x01;
	bit1 = (~data >> 6) & 0x01;
	bit2 = (~data >> 7) & 0x01;
	r = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;

	/* green component */
	bit0 = (~data >> 2) & 0x01;
	bit1 = (~data >> 3) & 0x01;
	bit2 = (~data >> 4) & 0x01;
	g = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;

	/* blue component */
	bit0 = (~blue >> 6) & 0x01;
	bit1 = (~data >> 0) & 0x01;
	bit2 = (~data >> 1) & 0x01;
	b = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;

	palette_set_color((offset & 0x3f),r,g,b);
}

/***************************************************************************
 The video system is a little bit odd.

 It consists of a 256x240 bitmap, that can apparently be indexed both
 directly and indirectly.

 Indirect Indexing:
 This is accomplished through memory locations 0x0000, 0x0001, 0x00002.
 0 = The x index on the bitmap
 1 = The y index on the bitmap
 2 = The pixel value for that (x,y) location (this will be 0x00-0x0F).

 Direct Indexing:
 Somehow, the bitmap is mapped to locations 0x600-0x3FFF.  Since a 256x256
 bitmap would theoretically need 0x10000 bytes, we need a way to compress
 it.  We can reduce it to 0x8000 bytes by storing two pixels to a byte.
 To drop it down to 0x4000 bytes, we're going to assume there is two banks
 of video memory.  Since Cloud9 doesn't provide many examples for how these
 bytes are accessed, the following are guesses that seem to work.  If other
 games use this hardware, feel free to try modifying the following
 assumptions.  We'll assume that:
 X & 0x03 == 0 - store at bank0, lo nibble
 X & 0x03 == 1 - store at bank0, hi nibble
 X & 0x03 == 2 - store at bank1, lo nibble
 X & 0x03 == 3 - store at bank1, hi nibble
 For Cloud9 to work, it's only important that writing 0's to 0x0600-0x3FFF
 clears the bitmap.  Oddly enough, to clear the bitmap, it only writes to
 0x0600-0x3FFF once, which leads me to believe there's a register that allows
 both banks to be written to at once, and a register to allow a specific bank
 to be selected.

***************************************************************************/


/***************************************************************************
  ConvertPoint

  This is used to take an x,y location and convert it to the proper memory
  address, bank, and nibble.  This is pure speculation.
***************************************************************************/
static void convert_point(unsigned int x, unsigned int y, unsigned char **vptr, int *vpixel)
{
	unsigned int voff;

	voff = (y << 6) + (x >> 2) - 0x600;

	switch (x & 0x02)
	{
	case 0x00:
		*vptr = &(videoram[voff]);
		break;
	case 0x02:
		*vptr = &(cloud9_vram2[voff]);
		break;
	}

	switch (x & 0x01)
	{
	case 0x00:
		*vpixel = (**vptr & 0x0f) >> 0;
		break;
	case 0x01:
		*vpixel = (**vptr & 0xf0) >> 4;
		break;
	}
}

/***************************************************************************
  cloud9_bitmap_regs_r
***************************************************************************/
READ_HANDLER( cloud9_bitmap_regs_r )
{
	unsigned char *vptr;
	int vpixel;
	unsigned int x, y;

	x = cloud9_bitmap_regs[0];
	y = cloud9_bitmap_regs[1];

	switch (offset)
	{
	case 0:
		/* Indirect Addressing - X register */
		return x;
	case 1:
		/* Indirect Addressing - Y register */
		return y;
	case 2:
		/* Indirect Addressing - pixel value at (X,Y) */
		if (y < 0x0c)
		{
			log_cb(RETRO_LOG_DEBUG, LOGPRE "Unexpected read from top of bitmap!\n");
			return 0;
		}

		convert_point(x,y,&vptr,&vpixel);
		return vpixel;
	}

	return 0;
}

/***************************************************************************
  cloud9_bitmap_regs_w
***************************************************************************/
WRITE_HANDLER( cloud9_bitmap_regs_w )
{
	unsigned int x, y;

	cloud9_bitmap_regs[offset] = data;

	x = cloud9_bitmap_regs[0];
	y = cloud9_bitmap_regs[1];

	if (offset == 2)
	{
		/* Not quite sure how these writes map to VRAM
		   (Cloud9 only directly writes 00 to VRAM and doesn't read it) */

		/* Don't allow writes to memory at less than 0x600 */
		if (y >= 0x0c)
		{
			unsigned char *vptr;
			int vpixel;

			convert_point(x,y,&vptr,&vpixel);

			/* This is a guess */
			switch (x & 0x01)
			{
			case 0x00:
				*vptr = (*vptr & 0xf0) | ((data & 0x0f) << 0);
				break;
			case 0x01:
				*vptr = (*vptr & 0x0f) | ((data & 0x0f) << 4);
				break;
			}

		}

		/* If color_bank is set, add 0x20 to the color */
		plot_pixel(tmpbitmap, x, y, Machine->pens[(data & 0x0f) + ((*cloud9_color_bank & 0x80) >> 2)]);

		if ((*cloud9_auto_inc_x) < 0x80)
			cloud9_bitmap_regs[0]++;

		if ((*cloud9_auto_inc_y) < 0x80)
			cloud9_bitmap_regs[1]++;
	}
}

/***************************************************************************
  cloud9_bitmap_w
***************************************************************************/
WRITE_HANDLER( cloud9_bitmap_w )
{
	UINT8 x, y;

	y = ((offset + 0x600) >> 6);
	x = ((offset + 0x600) & 0x3f) << 2;

	if (*cloud9_both_banks & 0x80)
	{
		videoram[offset] = data;
		cloud9_vram2[offset] = data;

		plot_pixel(tmpbitmap, x,   y, Machine->pens[((data & 0x0f) >> 0) + ((*cloud9_color_bank & 0x80) >> 2)]);
		plot_pixel(tmpbitmap, x+1, y, Machine->pens[((data & 0xf0) >> 4) + ((*cloud9_color_bank & 0x80) >> 2)]);
		plot_pixel(tmpbitmap, x+2, y, Machine->pens[((data & 0x0f) >> 0) + ((*cloud9_color_bank & 0x80) >> 2)]);
		plot_pixel(tmpbitmap, x+3, y, Machine->pens[((data & 0xf0) >> 4) + ((*cloud9_color_bank & 0x80) >> 2)]);
	}
	else if (*cloud9_vram_bank & 0x80)
	{
		cloud9_vram2[offset] = data;

		plot_pixel(tmpbitmap, x+2, y, Machine->pens[((data & 0x0f) >> 0) + ((*cloud9_color_bank & 0x80) >> 2)]);
		plot_pixel(tmpbitmap, x+3, y, Machine->pens[((data & 0xf0) >> 4) + ((*cloud9_color_bank & 0x80) >> 2)]);
	}
	else
	{
		videoram[offset] = data;

		plot_pixel(tmpbitmap, x  , y, Machine->pens[((data & 0x0f) >> 0) + ((*cloud9_color_bank & 0x80) >> 2)]);
		plot_pixel(tmpbitmap, x+1, y, Machine->pens[((data & 0xf0) >> 4) + ((*cloud9_color_bank & 0x80) >> 2)]);
	}
}

/***************************************************************************

  Draw the game screen in the given mame_bitmap.
  Do NOT call osd_update_display() from this function, it will be called by
  the main emulation engine.

***************************************************************************/
VIDEO_UPDATE( cloud9 )
{
	int offs;

	copybitmap(bitmap,tmpbitmap,0,0,0,0,&Machine->visible_area,TRANSPARENCY_NONE,0);

	/* draw the sprites */
	for (offs = 0;offs < 20;offs++)
	{
		int spritenum;
		int xflip,yflip,rblank,lblank;
		int x, y;

		spritenum = spriteram[offs + 0x20];

		xflip  = (spriteram[offs + 0x40] & 0x80);
		yflip  = (spriteram[offs + 0x40] & 0x40);
		rblank = (spriteram[offs + 0x40] & 0x20);
		lblank = (spriteram[offs + 0x40] & 0x10);
		x = spriteram[offs + 0x60];
		y = 240 - spriteram[offs];

		drawgfx(bitmap,Machine->gfx[2],
				spritenum,
				1 + ((*cloud9_color_bank & 0x80) >> 6),
				xflip,yflip,
				x,y,
				&Machine->visible_area,TRANSPARENCY_PEN,0);
	}
}

VIDEO_START( cloud9 )
{
	tmpbitmap = auto_bitmap_alloc(Machine->drv->screen_width,Machine->drv->screen_height);
	cloud9_vram2 = auto_malloc(videoram_size);

	if (!tmpbitmap || !cloud9_vram2)
		return 1;

	memset(cloud9_vram2, 0, videoram_size);

	return 0;
}
