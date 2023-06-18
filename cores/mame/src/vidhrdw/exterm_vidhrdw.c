/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "cpu/tms34010/tms34010.h"


data16_t *exterm_master_videoram, *exterm_slave_videoram;



/*************************************
 *
 *	Palette setup
 *
 *************************************/

PALETTE_INIT( exterm )
{
	int i;

	/* initialize 555 RGB lookup */
	for (i = 0; i < 32768; i++)
	{
		int r,g,b;

		r = (i >> 10) & 0x1f;
		g = (i >>  5) & 0x1f;
		b = (i >>  0) & 0x1f;

		r = (r << 3) | (r >> 2);
		g = (g << 3) | (g >> 2);
		b = (b << 3) | (b >> 2);
		
		palette_set_color(i+4096,r,g,b);
	}
}



/*************************************
 *
 *	Master shift register
 *
 *************************************/

void exterm_to_shiftreg_master(unsigned int address, UINT16* shiftreg)
{
	memcpy(shiftreg, &exterm_master_videoram[TOWORD(address)], 256 * sizeof(UINT16));
}

void exterm_from_shiftreg_master(unsigned int address, UINT16* shiftreg)
{
	memcpy(&exterm_master_videoram[TOWORD(address)], shiftreg, 256 * sizeof(UINT16));
}

void exterm_to_shiftreg_slave(unsigned int address, UINT16* shiftreg)
{
	memcpy(shiftreg, &exterm_slave_videoram[TOWORD(address)], 256 * 2 * sizeof(UINT8));
}

void exterm_from_shiftreg_slave(unsigned int address, UINT16* shiftreg)
{
	memcpy(&exterm_slave_videoram[TOWORD(address)], shiftreg, 256 * 2 * sizeof(UINT8));
}



/*************************************
 *
 *	Video startup/shutdown
 *
 *************************************/

VIDEO_START( exterm )
{
	return 0;
}



/*************************************
 *
 *	Main video refresh
 *
 *************************************/

VIDEO_UPDATE( exterm )
{
	int x, y;

	/* if the display is blanked, fill with black */
	if (tms34010_io_display_blanked(0))
	{
		fillbitmap(bitmap, Machine->pens[0], cliprect);
		return;
	}

	/* 16-bit case */
	for (y = cliprect->min_y; y <= cliprect->max_y; y++)
	{
		data16_t *bgsrc = &exterm_master_videoram[256 * y];
		UINT16 scanline[256];

		/* on the top/bottom of the screen, it's all background */
		if (y < 40 || y > 238)
			for (x = 0; x < 256; x++)
			{
				UINT16 bgdata = *bgsrc++;
				scanline[x] = (bgdata & 0x8000) ? (bgdata & 0xfff) : (bgdata + 0x1000);
			}

		/* elsewhere, we have to blend foreground and background */
		else
		{
			data16_t *fgsrc = (tms34010_get_DPYSTRT(1) & 0x800) ? &exterm_slave_videoram[y*128] : &exterm_slave_videoram[(256+y)*128];

			for (x = 0; x < 256; x += 2)
			{
				UINT16 fgdata = *fgsrc++;
				UINT16 bgdata;

				if (fgdata & 0x00ff)
					scanline[x] = fgdata & 0x00ff;
				else
				{
					bgdata = bgsrc[0];
					scanline[x] = (bgdata & 0x8000) ? (bgdata & 0xfff) : (bgdata + 0x1000);
				}

				if (fgdata & 0xff00)
					scanline[x+1] = fgdata >> 8;
				else
				{
					bgdata = bgsrc[1];
					scanline[x+1] = (bgdata & 0x8000) ? (bgdata & 0xfff) : (bgdata + 0x1000);
				}

				bgsrc += 2;
			}
		}

		/* draw the scanline */
		draw_scanline16(bitmap, cliprect->min_x, y, cliprect->max_x - cliprect->min_x, &scanline[cliprect->min_x], Machine->pens, -1);
	}
}
