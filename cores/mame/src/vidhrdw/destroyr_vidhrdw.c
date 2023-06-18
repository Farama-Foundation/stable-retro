/***************************************************************************

Atari Destroyer video emulation

***************************************************************************/

#include "driver.h"

int destroyr_wavemod;
int destroyr_cursor;

UINT8* destroyr_major_obj_ram;
UINT8* destroyr_minor_obj_ram;
UINT8* destroyr_alpha_num_ram;


VIDEO_UPDATE( destroyr )
{
	int i;
	int j;

	fillbitmap(bitmap, Machine->pens[0], &Machine->visible_area);

	/* draw major objects */

	for (i = 0; i < 16; i++)
	{
		int attr = destroyr_major_obj_ram[2 * i + 0] ^ 0xff;
		int horz = destroyr_major_obj_ram[2 * i + 1];

		int num = attr & 3;
		int scan = attr & 4;
		int flipx = attr & 8;

		if (scan == 0)
		{
			if (horz >= 192)
				horz -= 256;
		}
		else
		{
			if (horz < 192)
				continue;
		}

		drawgfx(bitmap, Machine->gfx[2], num, 0, flipx, 0,
			horz, 16 * i, &Machine->visible_area, TRANSPARENCY_PEN, 0);
	}

	/* draw alpha numerics */

	for (i = 0; i < 8; i++)
	{
		for (j = 0; j < 32; j++)
		{
			int num = destroyr_alpha_num_ram[32 * i + j];

			drawgfx(bitmap, Machine->gfx[0], num, 0, 0, 0,
				8 * j, 8 * i, &Machine->visible_area, TRANSPARENCY_PEN, 0);
		}
	}

	/* draw minor objects */

	for (i = 0; i < 2; i++)
	{
		int horz = 256 - destroyr_minor_obj_ram[i + 2];
		int vert = 256 - destroyr_minor_obj_ram[i + 4];

		drawgfx(bitmap, Machine->gfx[1], destroyr_minor_obj_ram[i + 0], 0, 0, 0,
			horz, vert, &Machine->visible_area, TRANSPARENCY_PEN, 0);
	}

	/* draw waves */

	for (i = 0; i < 4; i++)
	{
		drawgfx(bitmap, Machine->gfx[3], destroyr_wavemod ? 1 : 0, 0, 0, 0,
			64 * i, 0x4e, &Machine->visible_area, TRANSPARENCY_PEN, 0);
	}

	/* draw cursor */

	for (i = 0; i < 256; i++)
	{
		if (i & 4)
			plot_pixel(bitmap, i, destroyr_cursor ^ 0xff, Machine->pens[7]);
	}
}
