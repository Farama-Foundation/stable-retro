/***************************************************************************

	Atari Missile Command hardware

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "missile.h"

unsigned char *missile_videoram;


/***************************************************************************

  Start the video hardware emulation.

***************************************************************************/

VIDEO_START( missile )
{
	/* force video ram to be $0000-$FFFF even though only $1900-$FFFF is used */
	if ((missile_videoram = auto_malloc (256 * 256)) == 0)
		return 1;

	if ((tmpbitmap = auto_bitmap_alloc(256, 256)) == 0)
		return 1;

	memset (missile_videoram, 0, 256 * 256);
	return 0;
}



/********************************************************************************************/
READ_HANDLER( missile_video_r )
{
	return (missile_videoram[offset] & 0xe0);
}

/********************************************************************************************/
static void missile_blit_w (offs_t offset)
{
	int x, y;
	int bottom;
	int color;

	/* The top 25 lines ($0000 -> $18ff) aren't used or drawn */
	y = (offset >> 8) - 25;
	x = offset & 0xff;
	if( y < 231 - 32)
		bottom = 1;
	else
		bottom = 0;

	/* cocktail mode */
	if (flip_screen)
	{
		y = tmpbitmap->height - 1 - y;
	}

	color = (missile_videoram[offset] >> 5);

	if (bottom) color &= 0x06;

	plot_pixel(tmpbitmap, x, y, Machine->pens[color]);
}

/********************************************************************************************/
WRITE_HANDLER( missile_video_w )
{
	/* $0640 - $4fff */
	int wbyte, wbit;
	unsigned char *RAM = memory_region(REGION_CPU1);


	if (offset < 0xf800)
	{
		missile_videoram[offset] = data;
		missile_blit_w (offset);
	}
	else
	{
		missile_videoram[offset] = (missile_videoram[offset] & 0x20) | data;
		missile_blit_w (offset);
		wbyte = ((offset - 0xf800) >> 2) & 0xfffe;
		wbit = (offset - 0xf800) % 8;
		if(data & 0x20)
			RAM[0x401 + wbyte] |= (1 << wbit);
		else
			RAM[0x401 + wbyte] &= ((1 << wbit) ^ 0xff);
	}
}

WRITE_HANDLER( missile_video2_w )
{
	/* $5000 - $ffff */
	offset += 0x5000;
	missile_video_w (offset, data);
}

/********************************************************************************************/
WRITE_HANDLER( missile_video_mult_w )
{
	/*
		$1900 - $3fff

		2-bit color writes in 4-byte blocks.
		The 2 color bits are in x000x000.

		Note that the address range is smaller because 1 byte covers 4 screen pixels.
	*/

	data = (data & 0x80) + ((data & 8) << 3);
	offset = offset << 2;

	/* If this is the bottom 8 lines of the screen, set the 3rd color bit */
	if (offset >= 0xf800) data |= 0x20;

	missile_videoram[offset]     = data;
	missile_videoram[offset + 1] = data;
	missile_videoram[offset + 2] = data;
	missile_videoram[offset + 3] = data;

	missile_blit_w (offset);
	missile_blit_w (offset + 1);
	missile_blit_w (offset + 2);
	missile_blit_w (offset + 3);
}


/********************************************************************************************/
WRITE_HANDLER( missile_video_3rd_bit_w )
{
	int i;
	unsigned char *RAM = memory_region(REGION_CPU1);
	offset = offset + 0x400;

	/* This is needed to make the scrolling text work properly */
	RAM[offset] = data;

	offset = ((offset - 0x401) << 2) + 0xf800;
	for (i=0; i<8; i++)
	{
		if (data & (1 << i))
			missile_videoram[offset + i] |= 0x20;
		else
			missile_videoram[offset + i] &= 0xc0;
		missile_blit_w (offset + i);
	}
}


/********************************************************************************************/
VIDEO_UPDATE( missile )
{
	if (get_vh_global_attribute_changed())
	{
		int offs;

		for (offs = 0x1900; offs <= 0xffff; offs++)
			missile_blit_w (offs);
	}
	copybitmap(bitmap,tmpbitmap,0,0,0,0,&Machine->visible_area,TRANSPARENCY_NONE,0);
}
