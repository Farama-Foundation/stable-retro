/***************************************************************************

  Nintendo 8080 hardware

	- Space Fever
	- Space Fever High Splitter
	- Space Launcher
	- Sheriff / Bandido
	- Helifire

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "cpu/i8039/i8039.h"
#include <math.h>

#define HARDWARE_IS_SPACE_FEVER     ( n8080_hardware == 1 )
#define HARDWARE_IS_SHERIFF         ( n8080_hardware == 2 )
#define HARDWARE_IS_HELIFIRE        ( n8080_hardware == 3 )

static int n8080_hardware;

static mame_timer* sound_timer[3];

static int mono_flop[3];

static int sheriff_color_mode;
static int sheriff_color_data;

static int helifire_decay;

static UINT8 helifire_LSFR[63];

static int spacefev_ufo_frame;
static int spacefev_ufo_cycle;
static int spacefev_red_screen;
static int spacefev_red_cannon;

int helifire_flash;

static mame_timer* spacefev_red_cannon_timer;

static unsigned shift_data;
static unsigned shift_bits;

static UINT16 prev_sound_pins;
static UINT16 curr_sound_pins;

static unsigned helifire_mv;
static unsigned helifire_sc; /* IC56 */


/* following data is based on screen shots */

static const UINT8 sheriff_color_PROM[] =
{
	0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8,
	0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x0, 0x0, 0x0, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8,
	0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8,
	0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x0, 0x0, 0x0, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8,
	0xe, 0xe, 0xe, 0xe, 0xe, 0xe, 0xe, 0xe, 0xe, 0xe, 0xe, 0xe, 0xe, 0xe, 0xe, 0xe,
	0xe, 0xe, 0xe, 0xe, 0xe, 0xe, 0xe, 0x6, 0x6, 0x6, 0xe, 0xe, 0xd, 0xf, 0xf, 0xf,
	0xe, 0xe, 0xe, 0xe, 0xe, 0xe, 0xe, 0xe, 0xe, 0xe, 0xe, 0xe, 0xe, 0xe, 0xe, 0xe,
	0xe, 0xe, 0xe, 0xe, 0xe, 0xe, 0xe, 0x6, 0x6, 0x6, 0xe, 0xe, 0xd, 0xf, 0xf, 0xf,
	0xe, 0xe, 0xe, 0xe, 0xe, 0xe, 0xe, 0xe, 0xe, 0xe, 0xe, 0xe, 0xe, 0xe, 0xe, 0xe,
	0xe, 0xe, 0xe, 0xe, 0xe, 0xe, 0xe, 0x6, 0x6, 0x6, 0xe, 0xe, 0xd, 0xf, 0xf, 0xf,
	0xe, 0xe, 0xe, 0xe, 0xe, 0xb, 0xb, 0xb, 0xb, 0xb, 0xb, 0xb, 0xb, 0x9, 0x9, 0xb,
	0xb, 0xb, 0xb, 0xb, 0xb, 0xb, 0xb, 0x6, 0x6, 0x6, 0xe, 0xe, 0xd, 0xf, 0xe, 0xb,
	0xe, 0xe, 0xe, 0xe, 0xe, 0xb, 0xb, 0xb, 0xb, 0xb, 0xb, 0xb, 0xb, 0xb, 0xb, 0xb,
	0xb, 0xb, 0xb, 0xb, 0xb, 0xb, 0xb, 0x6, 0x6, 0x6, 0xe, 0xe, 0xd, 0xf, 0xe, 0xb,
	0xe, 0xe, 0xe, 0xe, 0xe, 0xb, 0xb, 0xb, 0xb, 0xb, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3,
	0x3, 0x3, 0x3, 0x3, 0x3, 0xb, 0xb, 0x6, 0x6, 0x6, 0xe, 0xe, 0xd, 0xf, 0xe, 0xb,
	0xe, 0xe, 0xe, 0xe, 0xe, 0xb, 0xb, 0xb, 0xb, 0xb, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3,
	0x3, 0x3, 0x3, 0x3, 0x3, 0xb, 0xb, 0x6, 0x6, 0x6, 0xe, 0xe, 0xd, 0xf, 0xf, 0xb,
	0xe, 0xe, 0xe, 0xe, 0xe, 0xb, 0xb, 0xb, 0xb, 0xb, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3,
	0x3, 0x3, 0x3, 0x3, 0x3, 0xb, 0xb, 0x6, 0x6, 0x6, 0xe, 0xe, 0xd, 0xf, 0xf, 0xb,
	0xe, 0xe, 0xe, 0xe, 0xe, 0xb, 0xb, 0xb, 0xb, 0xb, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3,
	0x3, 0x3, 0x3, 0x3, 0x3, 0xb, 0xb, 0x6, 0x6, 0x6, 0xe, 0xe, 0xd, 0xf, 0xf, 0xb,
	0xe, 0xe, 0xe, 0xe, 0xe, 0xb, 0xb, 0xb, 0xb, 0xb, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3,
	0x3, 0x3, 0x3, 0x3, 0x3, 0xb, 0xb, 0x6, 0x6, 0x6, 0xe, 0xe, 0xd, 0xf, 0xf, 0xb,
	0xe, 0xe, 0xe, 0xe, 0xe, 0xb, 0xb, 0xb, 0xb, 0xb, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3,
	0x3, 0x3, 0x3, 0x3, 0x3, 0xb, 0xb, 0x6, 0x6, 0x6, 0xe, 0xe, 0xd, 0xf, 0x9, 0xb,
	0xe, 0xe, 0xe, 0xe, 0xe, 0xb, 0xb, 0xb, 0xb, 0xb, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3,
	0x3, 0x3, 0x3, 0x3, 0x3, 0xb, 0xb, 0x6, 0x6, 0x6, 0xe, 0xe, 0xd, 0xf, 0x9, 0xb,
	0xe, 0xe, 0xe, 0xe, 0xe, 0xb, 0xb, 0xb, 0xb, 0xb, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3,
	0x3, 0x3, 0x3, 0x3, 0x3, 0xb, 0xb, 0x6, 0x6, 0x6, 0xe, 0xe, 0xd, 0xf, 0x9, 0xb,
	0xe, 0xe, 0xe, 0xa, 0xa, 0xb, 0xb, 0xb, 0xb, 0xb, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3,
	0x3, 0x3, 0x3, 0x3, 0x3, 0xb, 0xb, 0x2, 0x2, 0x6, 0xe, 0xe, 0xd, 0xf, 0x9, 0xb,
	0xe, 0xe, 0xe, 0xa, 0xa, 0xb, 0xb, 0xb, 0xb, 0xb, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3,
	0x3, 0x3, 0x3, 0x3, 0x3, 0xb, 0xb, 0x2, 0x2, 0x6, 0xe, 0xe, 0xd, 0xf, 0x9, 0xb,
	0xe, 0xe, 0xe, 0xe, 0xe, 0xb, 0xb, 0xb, 0xb, 0xb, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3,
	0x3, 0x3, 0x3, 0x3, 0x3, 0xb, 0xb, 0x6, 0x6, 0x6, 0xe, 0xe, 0xd, 0xf, 0x9, 0xb,
	0xe, 0xe, 0xe, 0xe, 0xe, 0xb, 0xb, 0xb, 0xb, 0xb, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3,
	0x3, 0x3, 0x3, 0x3, 0x3, 0xb, 0xb, 0x6, 0x6, 0x6, 0xe, 0xe, 0xd, 0xf, 0x9, 0xb,
	0xe, 0xe, 0xe, 0xe, 0xe, 0xb, 0xb, 0xb, 0xb, 0xb, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3,
	0x3, 0x3, 0x3, 0x3, 0x3, 0xb, 0xb, 0x6, 0x6, 0x6, 0xe, 0xe, 0xd, 0xf, 0x9, 0xb,
	0xe, 0xe, 0xe, 0xe, 0xe, 0xb, 0xb, 0xb, 0xb, 0xb, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3,
	0x3, 0x3, 0x3, 0x3, 0x3, 0xb, 0xb, 0x6, 0x6, 0x6, 0xe, 0xe, 0xd, 0xf, 0xf, 0xb,
	0xe, 0xe, 0xe, 0xe, 0xe, 0xb, 0xb, 0xb, 0xb, 0xb, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3,
	0x3, 0x3, 0x3, 0x3, 0x3, 0xb, 0xb, 0x6, 0x6, 0x6, 0xe, 0xe, 0xd, 0xf, 0xf, 0xb,
	0xe, 0xe, 0xe, 0xe, 0xe, 0xb, 0xb, 0xb, 0xb, 0xb, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3,
	0x3, 0x3, 0x3, 0x3, 0x3, 0xb, 0xb, 0x6, 0x6, 0x6, 0xe, 0xe, 0xd, 0xf, 0xf, 0xb,
	0xe, 0xe, 0xe, 0xe, 0xe, 0xb, 0xb, 0xb, 0xb, 0xb, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3,
	0x3, 0x3, 0x3, 0x3, 0x3, 0xb, 0xb, 0x6, 0x6, 0x6, 0xe, 0xe, 0xd, 0xf, 0xa, 0xb,
	0xe, 0xe, 0xe, 0xe, 0xe, 0xb, 0xb, 0xb, 0xb, 0xb, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3,
	0x3, 0x3, 0x3, 0x3, 0x3, 0xb, 0xb, 0x6, 0x6, 0x6, 0xe, 0xe, 0xd, 0xf, 0xa, 0xb,
	0xe, 0xe, 0xe, 0xe, 0xe, 0xb, 0xb, 0xb, 0xb, 0xb, 0xb, 0xb, 0xb, 0xb, 0xb, 0xb,
	0xb, 0xb, 0xb, 0xb, 0xb, 0xb, 0xb, 0x6, 0x6, 0x6, 0xe, 0xe, 0xd, 0xf, 0xa, 0xf,
	0xe, 0xe, 0xe, 0xe, 0xe, 0xb, 0xb, 0xb, 0xb, 0xb, 0xb, 0xb, 0xb, 0x9, 0x9, 0xb,
	0xb, 0xb, 0xb, 0xb, 0xb, 0xb, 0xb, 0x6, 0x6, 0x6, 0xe, 0xe, 0xd, 0xf, 0xf, 0xf,
	0xe, 0xe, 0xe, 0xe, 0xe, 0xe, 0xe, 0xe, 0xe, 0xe, 0xe, 0xe, 0xe, 0xe, 0xe, 0xe,
	0xe, 0xe, 0xe, 0xe, 0xe, 0xe, 0xe, 0x6, 0x6, 0x6, 0xe, 0xe, 0xd, 0xf, 0xf, 0xf,
	0xe, 0xe, 0xe, 0xe, 0xe, 0xe, 0xe, 0xe, 0xe, 0xe, 0xe, 0xe, 0xe, 0xe, 0xe, 0xe,
	0xe, 0xe, 0xe, 0xe, 0xe, 0xe, 0xe, 0x6, 0x6, 0x6, 0xe, 0xe, 0xd, 0xf, 0xf, 0xf,
	0xe, 0xe, 0xe, 0xe, 0xe, 0xe, 0xe, 0xe, 0xe, 0xe, 0xe, 0xe, 0xe, 0xe, 0xe, 0xe,
	0xe, 0xe, 0xe, 0xe, 0xe, 0xe, 0xe, 0x6, 0x6, 0x6, 0xe, 0xe, 0xd, 0xf, 0xf, 0xf,
	0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8,
	0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x0, 0x0, 0x0, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8,
	0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8,
	0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x0, 0x0, 0x0, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8,
};


static struct DACinterface n8080_dac_interface =
{
	1, { 30 }
};


struct SN76477interface sheriff_sn76477_interface =
{
	1,
	{ 35 },
	{ RES_K(36)  },  /* 04 */
	{ RES_K(100) },  /* 05 */
	{ CAP_N(1)   },  /* 06 */
	{ RES_K(620) },  /* 07 */
	{ CAP_U(1)   },  /* 08 */
	{ RES_K(20)  },  /* 10 */
	{ RES_K(150) },  /* 11 */
	{ RES_K(47)  },  /* 12 */
	{ 0          },  /* 16 */
	{ CAP_N(1)   },  /* 17 */
	{ RES_M(1.5) },  /* 18 */
	{ 0          },  /* 19 */
	{ RES_M(1.5) },  /* 20 */
	{ CAP_N(47)  },  /* 21 */
	{ CAP_N(47)  },  /* 23 */
	{ RES_K(560) },  /* 24 */
};


struct SN76477interface spacefev_sn76477_interface =
{
	1,
	{ 35 },
	{ RES_K(36)  },  /* 04 */
	{ RES_K(150) },  /* 05 */
	{ CAP_N(1)   },  /* 06 */
	{ RES_M(1)   },  /* 07 */
	{ CAP_U(1)   },  /* 08 */
	{ RES_K(20)  },  /* 10 */
	{ RES_K(150) },  /* 11 */
	{ RES_K(47)  },  /* 12 */
	{ 0          },  /* 16 */
	{ CAP_N(1)   },  /* 17 */
	{ RES_M(1.5) },  /* 18 */
	{ 0          },  /* 19 */
	{ RES_M(1)   },  /* 20 */
	{ CAP_N(47)  },  /* 21 */
	{ CAP_N(47)  },  /* 23 */
	{ RES_K(820) },  /* 24 */
};


static WRITE_HANDLER( n8080_shift_bits_w )
{
	shift_bits = data & 7;
}


static WRITE_HANDLER( n8080_shift_data_w )
{
	shift_data = (shift_data >> 8) | (data << 8);
}


static READ_HANDLER( n8080_shift_r )
{
	return shift_data >> (8 - shift_bits);
}


static PALETTE_INIT( n8080 )
{
	int i;

	for (i = 0; i < 8; i++)
		palette_set_color(i, pal1bit(i >> 0), pal1bit(i >> 1), pal1bit(i >> 2));
}


static PALETTE_INIT( helifire )
{
	int i;

	palette_init_n8080(NULL, NULL);

	for (i = 0; i < 0x100; i++)
	{
		int level = 0xff * exp(-3 * i / 255.); /* capacitor discharge */

		palette_set_color(0x000 + 8 + i, 0x00, 0x00, level);   /* shades of blue */
		palette_set_color(0x100 + 8 + i, 0x00, 0xC0, level);   /* shades of blue w/ green star */

		palette_set_color(0x200 + 8 + i, level, 0x00, 0x00);   /* shades of red */
		palette_set_color(0x300 + 8 + i, level, 0xC0, 0x00);   /* shades of red w/ green star */
	}
}


static void helifire_next_line(void)
{
	helifire_mv++;

	if (helifire_sc % 4 == 2)
	{
		helifire_mv %= 256;
	}
	else
	{
		if (flip_screen)
		{
			helifire_mv %= 255;
		}
		else
		{
			helifire_mv %= 257;
		}
	}

	if (helifire_mv == 128)
	{
		helifire_sc++;
	}
}


static VIDEO_START( spacefev )
{
	spacefev_ufo_frame = 0;
	spacefev_ufo_cycle = 0;

	spacefev_red_screen = 0;
	spacefev_red_cannon = 0;

	return 0;
}


static VIDEO_START( helifire )
{
	UINT8 data = 0;

	int i;

	helifire_mv = 0;
	helifire_sc = 0;

	for (i = 0; i < 63; i++)
	{
		int bit =
			(data >> 6) ^
			(data >> 7) ^ 1;

		data = (data << 1) | (bit & 1);

		helifire_LSFR[i] = data;
	}

	flip_screen = 0;

	helifire_flash = 0;

	return 0;
}


static VIDEO_UPDATE( spacefev )
{
	UINT8 mask = flip_screen ? 0xff : 0x00;

	int x;
	int y;

	const UINT8* pRAM = videoram;

	/* Fake dip switch for cocktail mode */
	if (readinputport(4) == 0x01) mask = 0;

	for (y = 0; y < 256; y++)
	{
		UINT16* pLine = bitmap->line[y ^ mask];

		for (x = 0; x < 256; x += 8)
		{
			int n;

			UINT8 color = 0;

			if (spacefev_red_screen)
			{
				color = 1;
			}
			else
			{
				UINT8 val = memory_region(REGION_PROMS)[x >> 3];

				if ((x >> 3) == 0x06)
				{
					color = spacefev_red_cannon ? 1 : 7;
				}

				if ((x >> 3) == 0x1b)
				{
					static const UINT8 ufo_color[] =
					{
						1, /* red     */
						2, /* green   */
						7, /* white   */
						3, /* yellow  */
						5, /* magenta */
						6, /* cyan    */
					};

					color = ufo_color[spacefev_ufo_cycle];
				}

				for (n = color + 1; n < 8; n++)
				{
					if (~val & (1 << n))
					{
						color = n;
					}
				}
			}

			for (n = 0; n < 8; n++)
			{
				pLine[(x + n) ^ mask] = (pRAM[x >> 3] & (1 << n)) ? color : 0;
			}
		}

		pRAM += 32;
	}
}


static VIDEO_UPDATE( sheriff )
{
	UINT8 mask = flip_screen ? 0xff : 0x00;

	int x;
	int y;

	const UINT8* pRAM = videoram;

	for (y = 0; y < 256; y++)
	{
		UINT16* pLine = bitmap->line[y ^ mask];

		for (x = 0; x < 256; x += 8)
		{
			int n;

			UINT8 color = sheriff_color_PROM[32 * (y >> 3) + (x >> 3)];

			if (sheriff_color_mode == 1 && !(color & 8))
			{
				color = sheriff_color_data ^ 7;
			}

			if (sheriff_color_mode == 2)
			{
				color = sheriff_color_data ^ 7;
			}

			if (sheriff_color_mode == 3)
			{
				color = 7;
			}

			for (n = 0; n < 8; n++)
			{
				pLine[(x + n) ^ mask] = (pRAM[x >> 3] & (1 << n)) ? (color & 7) : 0;
			}
		}

		pRAM += 32;
	}
}


static VIDEO_UPDATE( helifire )
{
	int SUN_BRIGHTNESS = readinputport(4);
	int SEA_BRIGHTNESS = readinputport(5);

	static const int wave[8] = { 0, 1, 2, 2, 2, 1, 0, 0 };

	unsigned saved_mv = helifire_mv;
	unsigned saved_sc = helifire_sc;

	int x;
	int y;

	for (y = 0; y < 256; y++)
	{
		UINT16* pLine = bitmap->line[y];

		int level = 120 + wave[helifire_mv & 7];

		/* draw sky */

		for (x = level; x < 256; x++)
		{
			pLine[x] = 0x200 + 8 + SUN_BRIGHTNESS + x - level;
		}

		/* draw stars */

		if (helifire_mv % 8 == 4) /* upper half */
		{
			int step = (320 * (helifire_mv - 0)) % sizeof helifire_LSFR;

			int data =
				((helifire_LSFR[step] & 1) << 6) |
				((helifire_LSFR[step] & 2) << 4) |
				((helifire_LSFR[step] & 4) << 2) |
				((helifire_LSFR[step] & 8) << 0);

			pLine[0x80 + data] |= 0x100;
		}

		if (helifire_mv % 8 == 5) /* lower half */
		{
			int step = (320 * (helifire_mv - 1)) % sizeof helifire_LSFR;

			int data =
				((helifire_LSFR[step] & 1) << 6) |
				((helifire_LSFR[step] & 2) << 4) |
				((helifire_LSFR[step] & 4) << 2) |
				((helifire_LSFR[step] & 8) << 0);

			pLine[0x00 + data] |= 0x100;
		}

		/* draw sea */

		for (x = 0; x < level; x++)
		{
			pLine[x] = 8 + SEA_BRIGHTNESS + x;
		}

		/* draw foreground */

		for (x = 0; x < 256; x += 8)
		{
			int offset = 32 * y + (x >> 3);

			int n;

			for (n = 0; n < 8; n++)
			{
				if (flip_screen)
				{
					if ((videoram[offset ^ 0x1fff] << n) & 0x80)
					{
						pLine[x + n] = colorram[offset ^ 0x1fff] & 7;
					}
				}
				else
				{
					if ((videoram[offset] >> n) & 1)
					{
						pLine[x + n] = colorram[offset] & 7;
					}
				}
			}
		}

		/* next line */

		helifire_next_line();
	}

	helifire_mv = saved_mv;
	helifire_sc = saved_sc;
}


static VIDEO_EOF( helifire )
{
	int n = (cpu_getcurrentframe() >> 1) % sizeof helifire_LSFR;

	int i;

	for (i = 0; i < 8; i++)
	{
		int R = (i & 1);
		int G = (i & 2);
		int B = (i & 4);

		if (helifire_flash)
		{
			if (helifire_LSFR[n] & 0x20)
			{
				G |= B;
			}

			if (cpu_getcurrentframe() & 0x04)
			{
				R |= G;
			}
		}

		palette_set_color(i,
			R ? 255 : 0,
			G ? 255 : 0,
			B ? 255 : 0);
	}

	for (i = 0; i < 256; i++)
	{
		helifire_next_line();
	}
}


static VIDEO_EOF( spacefev )
{
	spacefev_ufo_frame = (spacefev_ufo_frame + 1) % 32;

	if (spacefev_ufo_frame == 0)
	{
		spacefev_ufo_cycle = (spacefev_ufo_cycle + 1) % 6;
	}
}


static INTERRUPT_GEN( interrupt )
{
	if (cpu_getvblank())
	{
		cpu_set_irq_line_and_vector(0, 0, PULSE_LINE, 0xcf);  /* RST $08 */
	}
	else
	{
		cpu_set_irq_line_and_vector(0, 0, PULSE_LINE, 0xd7);  /* RST $10 */
	}
}


static void spacefev_vco_voltage_timer(int dummy)
{
	double voltage = 0;

	if (mono_flop[2])
	{
		voltage = 5 * (1 - exp(- timer_timeelapsed(sound_timer[2]) / 0.22));
	}

	SN76477_set_vco_voltage(0, voltage);
}


static void helifire_decay_timer(int dummy)
{
	/* ... */
}


static void spacefev_update_SN76477_status(void)
{
	double dblR0 = RES_M(1.0);
	double dblR1 = RES_M(1.5);

	if (!mono_flop[0])
	{
		dblR0 = 1 / (1 / RES_K(150) + 1 / dblR0); /* ? */
	}
	if (!mono_flop[1])
	{
		dblR1 = 1 / (1 / RES_K(620) + 1 / dblR1); /* ? */
	}

	SN76477_set_decay_res(0, dblR0);

	SN76477_set_vco_res(0, dblR1);

	SN76477_enable_w(0,
		!mono_flop[0] &&
		!mono_flop[1] &&
		!mono_flop[2]);

	SN76477_vco_w(0, mono_flop[1]);

	SN76477_mixer_b_w(0, mono_flop[0]);
}


static void sheriff_update_SN76477_status(void)
{
	if (mono_flop[1])
	{
		SN76477_set_vco_voltage(0, 5);
	}
	else
	{
		SN76477_set_vco_voltage(0, 0);
	}

	SN76477_enable_w(0,
		!mono_flop[0] &&
		!mono_flop[1]);

	SN76477_vco_w(0, mono_flop[0]);

	SN76477_mixer_b_w(0, !mono_flop[0]);
}


static void update_SN76477_status(void)
{
	if (HARDWARE_IS_SPACE_FEVER)
	{
		spacefev_update_SN76477_status();
	}
	if (HARDWARE_IS_SHERIFF)
	{
		sheriff_update_SN76477_status();
	}
}


static void start_mono_flop(int n, double expire)
{
	mono_flop[n] = 1;

	update_SN76477_status();

	timer_adjust(sound_timer[n], expire, n, 0);
}


static void stop_mono_flop(int n)
{
	mono_flop[n] = 0;

	update_SN76477_status();

	timer_adjust(sound_timer[n], TIME_NEVER, n, 0);
}


static void start_red_cannon(double expire)
{
	spacefev_red_cannon = 1;

	timer_adjust(spacefev_red_cannon_timer, expire, 0, 0);
}


static void stop_red_cannon(int dummy)
{
	spacefev_red_cannon = 0;

	timer_adjust(spacefev_red_cannon_timer, TIME_NEVER, 0, 0);
}


static void spacefev_sound_pins_changed(void)
{
	UINT16 changes = ~curr_sound_pins & prev_sound_pins;

	bool irq_active;

	if (changes & (1 << 0x3))
	{
		stop_mono_flop(1);
	}
	if (changes & ((1 << 0x3) | (1 << 0x6)))
	{
		stop_mono_flop(2);
	}
	if (changes & (1 << 0x3))
	{
		start_mono_flop(0, TIME_IN_MSEC(0.55 * 36 * 100));
	}
	if (changes & (1 << 0x6))
	{
		start_mono_flop(1, TIME_IN_MSEC(0.55 * 22 * 33));
	}
	if (changes & (1 << 0x4))
	{
		start_mono_flop(2, TIME_IN_MSEC(0.55 * 22 * 33));
	}

	irq_active = (~curr_sound_pins & ((1 << 0x2) | (1 << 0x3) | (1 << 0x5))) != 0;
	cpu_set_irq_line(1, 0, irq_active ? ASSERT_LINE : CLEAR_LINE);
}


static void sheriff_sound_pins_changed(void)
{
	UINT16 changes = ~curr_sound_pins & prev_sound_pins;

	bool irq_active;

	if (changes & (1 << 0x6))
	{
		stop_mono_flop(1);
	}
	if (changes & (1 << 0x6))
	{
		start_mono_flop(0, TIME_IN_MSEC(0.55 * 33 * 33));
	}
	if (changes & (1 << 0x4))
	{
		start_mono_flop(1, TIME_IN_MSEC(0.55 * 33 * 33));
	}

	irq_active = (~curr_sound_pins & ((1 << 0x2) | (1 << 0x3) | (1 << 0x5))) != 0;
	cpu_set_irq_line(1, 0, irq_active ? ASSERT_LINE : CLEAR_LINE);
}


static void helifire_sound_pins_changed(void)
{
	/*UINT16 changes = ~curr_sound_pins & prev_sound_pins;*/

	bool irq_active;

	/* lacking emulation of sound bits 10, 11, 12 and 4 */

	irq_active = (~curr_sound_pins & (1 << 6)) != 0;
	cpu_set_irq_line(1, 0, irq_active ? ASSERT_LINE : CLEAR_LINE);
}


static void sound_pins_changed(void)
{
	if (HARDWARE_IS_SPACE_FEVER)
	{
		spacefev_sound_pins_changed();
	}
	if (HARDWARE_IS_SHERIFF)
	{
		sheriff_sound_pins_changed();
	}
	if (HARDWARE_IS_HELIFIRE)
	{
		helifire_sound_pins_changed();
	}

	prev_sound_pins = curr_sound_pins;
}


static void delayed_sound_1(int data)
{
	static UINT8 prev_data = 0;

	curr_sound_pins &= ~(
		(1 << 0x7) |
		(1 << 0x5) |
		(1 << 0x6) |
		(1 << 0x3) |
		(1 << 0x4) |
		(1 << 0x1));

	if (~data & 0x01) curr_sound_pins |= 1 << 0x7;
	if (~data & 0x02) curr_sound_pins |= 1 << 0x5; /* pulse */
	if (~data & 0x04) curr_sound_pins |= 1 << 0x6; /* pulse */
	if (~data & 0x08) curr_sound_pins |= 1 << 0x3; /* pulse (except in Helifire) */
	if (~data & 0x10) curr_sound_pins |= 1 << 0x4; /* pulse (except in Helifire) */
	if (~data & 0x20) curr_sound_pins |= 1 << 0x1;

	if (HARDWARE_IS_SPACE_FEVER)
	{
		if (data & ~prev_data & 0x10)
		{
			start_red_cannon(TIME_IN_MSEC(0.55 * 68 * 10));
		}

		spacefev_red_screen = data & 0x08;
	}

	sound_pins_changed();

	prev_data = data;
}


static void delayed_sound_2(int data)
{
	curr_sound_pins &= ~(
		(1 << 0x8) |
		(1 << 0x9) |
		(1 << 0xA) |
		(1 << 0xB) |
		(1 << 0x2) |
		(1 << 0xC));

	if (~data & 0x01) curr_sound_pins |= 1 << 0x8;
	if (~data & 0x02) curr_sound_pins |= 1 << 0x9;
	if (~data & 0x04) curr_sound_pins |= 1 << 0xA;
	if (~data & 0x08) curr_sound_pins |= 1 << 0xB;
	if (~data & 0x10) curr_sound_pins |= 1 << 0x2; /* pulse */
	if (~data & 0x20) curr_sound_pins |= 1 << 0xC;

	if (HARDWARE_IS_SPACE_FEVER)
	{
		flip_screen = data & 0x20;
	}

	if (HARDWARE_IS_HELIFIRE)
	{
		helifire_flash = data & 0x20;
	}

	sound_pins_changed();
}


static WRITE_HANDLER( n8080_sound_1_w )
{
	timer_set(TIME_NOW, data, delayed_sound_1); /* force CPUs to sync */
}


static WRITE_HANDLER( n8080_sound_2_w )
{
	timer_set(TIME_NOW, data, delayed_sound_2); /* force CPUs to sync */
}


static READ_HANDLER( n8080_8035_p1_r )
{
	UINT8 val = 0;

	if (curr_sound_pins & (1 << 0xB)) val |= 0x01;
	if (curr_sound_pins & (1 << 0xA)) val |= 0x02;
	if (curr_sound_pins & (1 << 0x9)) val |= 0x04;
	if (curr_sound_pins & (1 << 0x8)) val |= 0x08;
	if (curr_sound_pins & (1 << 0x5)) val |= 0x10;
	if (curr_sound_pins & (1 << 0x3)) val |= 0x20;
	if (curr_sound_pins & (1 << 0x2)) val |= 0x40;
	if (curr_sound_pins & (1 << 0x1)) val |= 0x80;

	return val;
}


static READ_HANDLER( helifire_8035_extended_ram_r )
{
	UINT8 val = 0;

	if (curr_sound_pins & (1 << 0x7)) val |= 0x01;
	if (curr_sound_pins & (1 << 0x8)) val |= 0x02;
	if (curr_sound_pins & (1 << 0x9)) val |= 0x04;
	if (curr_sound_pins & (1 << 0x1)) val |= 0x08;

	return val;
}


static READ_HANDLER( n8080_8035_t0_r )
{
	return (curr_sound_pins & (1 << 0x7)) ? 1 : 0;
}


static READ_HANDLER( n8080_8035_t1_r )
{
	return (curr_sound_pins & (1 << 0xC)) ? 1 : 0;
}


static READ_HANDLER( helifire_8035_t0_r )
{
	return (curr_sound_pins & (1 << 0x3)) ? 1 : 0;
}


static READ_HANDLER( helifire_8035_t1_r )
{
	return (curr_sound_pins & (1 << 0x4)) ? 1 : 0;
}


static WRITE_HANDLER( n8080_dac_w )
{
	DAC_data_w(0, data & 0x80);
}


static WRITE_HANDLER( helifire_dac_data_w )
{
	DAC_data_w(0, data);
}


static WRITE_HANDLER( helifire_dac_vref_w )
{
	helifire_decay = ~data & 0x80;
}


static WRITE_HANDLER( n8080_video_control_w )
{
	sheriff_color_mode = (data >> 3) & 3;
	sheriff_color_data = (data >> 0) & 7;

	flip_screen = data & 0x20;
}


static MEMORY_READ_START( main_cpu_readmem )
	MEMORY_ADDRESS_BITS(15)
	{ 0x0000, 0x3fff, MRA_ROM },
	{ 0x4000, 0x7fff, MRA_RAM },
MEMORY_END


static MEMORY_WRITE_START( main_cpu_writemem )
	MEMORY_ADDRESS_BITS(15)
	{ 0x0000, 0x3fff, MWA_ROM },
	{ 0x4000, 0x7fff, MWA_RAM, &videoram },
MEMORY_END


static MEMORY_READ_START( helifire_main_cpu_readmem )
	{ 0x0000, 0x3fff, MRA_ROM },
	{ 0x4000, 0x7fff, MRA_RAM },
	{ 0xc000, 0xdfff, MRA_RAM },
MEMORY_END

static MEMORY_WRITE_START( helifire_main_cpu_writemem )
	{ 0x0000, 0x3fff, MWA_ROM },
	{ 0x4000, 0x7fff, MWA_RAM, &videoram },
	{ 0xc000, 0xdfff, MWA_RAM, &colorram },
MEMORY_END


static PORT_READ_START( n8080_main_io_readport )
	PORT_ADDRESS_BITS(3)
	{ 0x00, 0x00, input_port_0_r },
	{ 0x01, 0x01, input_port_1_r },
	{ 0x02, 0x02, input_port_2_r },
	{ 0x03, 0x03, n8080_shift_r },
	{ 0x04, 0x04, input_port_3_r },
PORT_END


static PORT_WRITE_START( n8080_main_io_writeport )
	PORT_ADDRESS_BITS(3)
	{ 0x02, 0x02, n8080_shift_bits_w },
	{ 0x03, 0x03, n8080_shift_data_w },
	{ 0x04, 0x04, n8080_sound_1_w },
	{ 0x05, 0x05, n8080_sound_2_w },
	{ 0x06, 0x06, n8080_video_control_w },
PORT_END


static MEMORY_READ_START( sound_cpu_readmem )
	MEMORY_ADDRESS_BITS(10)
	{ 0x0000, 0x03ff, MRA_ROM },
MEMORY_END


static MEMORY_WRITE_START( sound_cpu_writemem )
	MEMORY_ADDRESS_BITS(10)
	{ 0x0000, 0x03ff, MWA_ROM },
MEMORY_END


static PORT_READ_START( n8080_sound_io_readport )
	{ I8039_t0, I8039_t0, n8080_8035_t0_r },
	{ I8039_t1, I8039_t1, n8080_8035_t1_r },
	{ I8039_p1, I8039_p1, n8080_8035_p1_r },
PORT_END


static PORT_WRITE_START( n8080_sound_io_writeport )
	{ I8039_p2, I8039_p2, n8080_dac_w },
PORT_END


static PORT_READ_START( helifire_sound_io_readport )
	{ I8039_t0, I8039_t0, helifire_8035_t0_r },
	{ I8039_t1, I8039_t1, helifire_8035_t1_r },
	{ 0x00, 0x7f, helifire_8035_extended_ram_r },
PORT_END


static PORT_WRITE_START( helifire_sound_io_writeport )
	{ I8039_p1, I8039_p1, helifire_dac_data_w },
	{ I8039_p2, I8039_p2, helifire_dac_vref_w },
PORT_END


static MACHINE_INIT( spacefev )
{
	n8080_hardware = 1;

	timer_pulse(TIME_IN_HZ(1000), 0, spacefev_vco_voltage_timer);

	sound_timer[0] = timer_alloc(stop_mono_flop);
	sound_timer[1] = timer_alloc(stop_mono_flop);
	sound_timer[2] = timer_alloc(stop_mono_flop);

	spacefev_red_cannon_timer = timer_alloc(stop_red_cannon);

	SN76477_envelope_1_w(0, 1);
	SN76477_envelope_2_w(0, 0);
	SN76477_mixer_a_w(0, 0);
	SN76477_mixer_b_w(0, 0);
	SN76477_mixer_c_w(0, 0);
	SN76477_noise_clock_w(0, 0);

	mono_flop[0] = 0;
	mono_flop[1] = 0;
	mono_flop[2] = 0;

	delayed_sound_1(0);
	delayed_sound_2(0);
}


static MACHINE_INIT( sheriff )
{
	n8080_hardware = 2;

	sound_timer[0] = timer_alloc(stop_mono_flop);
	sound_timer[1] = timer_alloc(stop_mono_flop);

	SN76477_envelope_1_w(0, 1);
	SN76477_envelope_2_w(0, 0);
	SN76477_mixer_a_w(0, 0);
	SN76477_mixer_b_w(0, 0);
	SN76477_mixer_c_w(0, 0);
	SN76477_noise_clock_w(0, 0);

	mono_flop[0] = 0;
	mono_flop[1] = 0;

	delayed_sound_1(0);
	delayed_sound_2(0);

	n8080_video_control_w(0, 0);
}


static MACHINE_INIT( helifire )
{
	n8080_hardware = 3;

	timer_pulse(TIME_IN_HZ(1000), 0, helifire_decay_timer);

	delayed_sound_1(0);
	delayed_sound_2(0);

	n8080_video_control_w(0, 0);

	helifire_decay = 0;
}


static MACHINE_DRIVER_START( n8080 )

	/* basic machine hardware */
	MDRV_CPU_ADD_TAG("main", 8080, 20160000 / 10)
	MDRV_CPU_MEMORY(main_cpu_readmem, main_cpu_writemem)
	MDRV_CPU_PORTS(n8080_main_io_readport, n8080_main_io_writeport)
	MDRV_CPU_VBLANK_INT(interrupt, 2)

	MDRV_CPU_ADD_TAG("sound", I8035, 6000000 / I8039_CLOCK_DIVIDER)
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
	MDRV_CPU_MEMORY(sound_cpu_readmem, sound_cpu_writemem)
	MDRV_CPU_PORTS(n8080_sound_io_readport, n8080_sound_io_writeport)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(256, 256)
	MDRV_VISIBLE_AREA(0, 255, 16, 239)
	MDRV_PALETTE_LENGTH(8)
	MDRV_PALETTE_INIT(n8080)

	/* sound hardware */
	MDRV_SOUND_ADD(DAC, n8080_dac_interface)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( spacefev )

	MDRV_IMPORT_FROM(n8080)

	/* basic machine hardware */
	MDRV_MACHINE_INIT(spacefev)

	/* video hardware */
	MDRV_VIDEO_START(spacefev)
	MDRV_VIDEO_UPDATE(spacefev)
	MDRV_VIDEO_EOF(spacefev)

	/* sound hardware */
	MDRV_SOUND_ADD(SN76477, spacefev_sn76477_interface)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( sheriff )

	MDRV_IMPORT_FROM(n8080)

	/* basic machine hardware */
	MDRV_MACHINE_INIT(sheriff)

	/* video hardware */
	MDRV_VIDEO_UPDATE(sheriff)

	/* sound hardware */
	MDRV_SOUND_ADD(SN76477, sheriff_sn76477_interface)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( helifire )

	MDRV_IMPORT_FROM(n8080)

	/* basic machine hardware */
	MDRV_MACHINE_INIT(helifire)

	MDRV_CPU_MODIFY("main")
	MDRV_CPU_MEMORY(helifire_main_cpu_readmem, helifire_main_cpu_writemem)
	MDRV_CPU_PORTS(n8080_main_io_readport, n8080_main_io_writeport)

	MDRV_CPU_MODIFY("sound")
	MDRV_CPU_MEMORY(sound_cpu_readmem, sound_cpu_writemem)
	MDRV_CPU_PORTS(helifire_sound_io_readport, helifire_sound_io_writeport)

	/* video hardware */
	MDRV_PALETTE_LENGTH(0x400 + 8)
	MDRV_PALETTE_INIT(helifire)
	MDRV_VIDEO_START(helifire)
	MDRV_VIDEO_UPDATE(helifire)
	MDRV_VIDEO_EOF(helifire)
MACHINE_DRIVER_END


INPUT_PORTS_START( spacefev )
	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_2WAY )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_2WAY )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_COIN1 )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_2WAY | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_2WAY | IPF_PLAYER2 )
	PORT_BITX(0x08, 0x00, 0, "Game A", KEYCODE_Q, IP_JOY_NONE )
	PORT_BITX(0x10, 0x00, 0, "Game B", KEYCODE_W, IP_JOY_NONE )
	PORT_BITX(0x20, 0x00, 0, "Game C", KEYCODE_E, IP_JOY_NONE )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNUSED ) /* enables diagnostic ROM at $1c00 */

	PORT_START
	PORT_DIPNAME( 0x03, 0x00, DEF_STR( Lives ))
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x01, "4" )
	PORT_DIPSETTING(    0x02, "5" )
	PORT_DIPSETTING(    0x03, "6" )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unused ))
	PORT_DIPSETTING(    0x04, DEF_STR( Off ))
	PORT_DIPSETTING(    0x00, DEF_STR( On ))
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unused ))
	PORT_DIPSETTING(    0x08, DEF_STR( Off ))
	PORT_DIPSETTING(    0x00, DEF_STR( On ))
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unused ))
	PORT_DIPSETTING(    0x10, DEF_STR( Off ))
	PORT_DIPSETTING(    0x00, DEF_STR( On ))
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unused ))
	PORT_DIPSETTING(    0x20, DEF_STR( Off ))
	PORT_DIPSETTING(    0x00, DEF_STR( On ))
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unused ))
	PORT_DIPSETTING(    0x40, DEF_STR( Off ))
	PORT_DIPSETTING(    0x00, DEF_STR( On ))
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unused ))
	PORT_DIPSETTING(    0x80, DEF_STR( Off ))
	PORT_DIPSETTING(    0x00, DEF_STR( On ))

	PORT_START

	PORT_START	/* fake port for cocktail mode */
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Cabinet ))
	PORT_DIPSETTING(    0x00, DEF_STR( Cocktail ))
	PORT_DIPSETTING(    0x01, DEF_STR( Upright ))
INPUT_PORTS_END


INPUT_PORTS_START( highsplt )
	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_2WAY )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_2WAY )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_COIN1 )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_2WAY | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_2WAY | IPF_PLAYER2 )
	PORT_BITX(0x08, 0x00, 0, "Game A", KEYCODE_Q, IP_JOY_NONE )
	PORT_BITX(0x10, 0x00, 0, "Game B", KEYCODE_W, IP_JOY_NONE )
	PORT_BITX(0x20, 0x00, 0, "Game C", KEYCODE_E, IP_JOY_NONE )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNUSED ) /* enables diagnostic ROM at $2000 */

	PORT_START
	PORT_DIPNAME( 0x03, 0x00, DEF_STR( Lives ))
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x01, "4" )
	PORT_DIPSETTING(    0x02, "5" )
	PORT_DIPSETTING(    0x03, "6" )
	PORT_DIPNAME( 0x0c, 0x00, DEF_STR( Bonus_Life ))
	PORT_DIPSETTING(    0x00, "1500" )
	PORT_DIPSETTING(    0x04, "2000" )
	PORT_DIPSETTING(    0x08, "3000" )
	PORT_DIPSETTING(    0x0c, "4000" )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unused ))
	PORT_DIPSETTING(    0x10, DEF_STR( Off ))
	PORT_DIPSETTING(    0x00, DEF_STR( On ))
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unused ))
	PORT_DIPSETTING(    0x20, DEF_STR( Off ))
	PORT_DIPSETTING(    0x00, DEF_STR( On ))
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unused ))
	PORT_DIPSETTING(    0x40, DEF_STR( Off ))
	PORT_DIPSETTING(    0x00, DEF_STR( On ))
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unused ))
	PORT_DIPSETTING(    0x80, DEF_STR( Off ))
	PORT_DIPSETTING(    0x00, DEF_STR( On ))

	PORT_START

	PORT_START	/* fake port for cocktail mode */
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Cabinet ))
	PORT_DIPSETTING(    0x00, DEF_STR( Cocktail ))
	PORT_DIPSETTING(    0x01, DEF_STR( Upright ))
INPUT_PORTS_END


INPUT_PORTS_START( spacelnc )
	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_2WAY )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_2WAY )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_COIN1 )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_2WAY | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_2WAY | IPF_PLAYER2 )
	PORT_BITX(0x08, 0x00, 0, "Game A", KEYCODE_Q, IP_JOY_NONE )
	PORT_BITX(0x10, 0x00, 0, "Game B", KEYCODE_W, IP_JOY_NONE )
	PORT_BITX(0x20, 0x00, 0, "Game C", KEYCODE_E, IP_JOY_NONE )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNUSED ) /* enables diagnostic ROM at $2000 */

	PORT_START
	PORT_DIPNAME( 0x03, 0x00, DEF_STR( Lives ))
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x01, "4" )
	PORT_DIPSETTING(    0x02, "5" )
	PORT_DIPSETTING(    0x03, "6" )
	PORT_DIPNAME( 0x0c, 0x00, DEF_STR( Bonus_Life ))
	PORT_DIPSETTING(    0x00, "1000" )
	PORT_DIPSETTING(    0x04, "3000" )
	PORT_DIPSETTING(    0x08, "5000" )
	PORT_DIPSETTING(    0x0c, "8000" )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unused ))
	PORT_DIPSETTING(    0x10, DEF_STR( Off ))
	PORT_DIPSETTING(    0x00, DEF_STR( On ))
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unused ))
	PORT_DIPSETTING(    0x20, DEF_STR( Off ))
	PORT_DIPSETTING(    0x00, DEF_STR( On ))
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unused ))
	PORT_DIPSETTING(    0x40, DEF_STR( Off ))
	PORT_DIPSETTING(    0x00, DEF_STR( On ))
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unused ))
	PORT_DIPSETTING(    0x80, DEF_STR( Off ))
	PORT_DIPSETTING(    0x00, DEF_STR( On ))

	PORT_START

	PORT_START	/* fake port for cocktail mode */
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Cabinet ))
	PORT_DIPSETTING(    0x00, DEF_STR( Cocktail ))
	PORT_DIPSETTING(    0x01, DEF_STR( Upright ))
INPUT_PORTS_END


INPUT_PORTS_START( sheriff )
	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICKLEFT_RIGHT )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICKLEFT_LEFT )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICKLEFT_UP )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICKLEFT_DOWN )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_JOYSTICKRIGHT_RIGHT )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICKRIGHT_LEFT )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICKRIGHT_UP )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_JOYSTICKRIGHT_DOWN )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICKLEFT_RIGHT  | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICKLEFT_LEFT   | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICKLEFT_UP     | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICKLEFT_DOWN   | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_JOYSTICKRIGHT_RIGHT | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICKRIGHT_LEFT  | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICKRIGHT_UP    | IPF_COCKTAIL )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_JOYSTICKRIGHT_DOWN  | IPF_COCKTAIL )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNUSED ) /* EXP1 */
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNUSED ) /* EXP2 */
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNUSED ) /* EXP3 enables diagnostic ROM at $2400 */
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_COIN1 )

	PORT_START
	PORT_DIPNAME( 0x03, 0x00, DEF_STR( Lives ))
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x01, "4" )
	PORT_DIPSETTING(    0x02, "5" )
	PORT_DIPSETTING(    0x03, "6" )
	PORT_DIPNAME( 0x04, 0x00, DEF_STR( Unused ))
	PORT_DIPSETTING(    0x04, DEF_STR( Off ))
	PORT_DIPSETTING(    0x00, DEF_STR( On ))
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Unused ))
	PORT_DIPSETTING(    0x08, DEF_STR( Off ))
	PORT_DIPSETTING(    0x00, DEF_STR( On ))
	PORT_DIPNAME( 0x10, 0x00, DEF_STR( Unused ))
	PORT_DIPSETTING(    0x10, DEF_STR( Off ))
	PORT_DIPSETTING(    0x00, DEF_STR( On ))
	PORT_DIPNAME( 0x20, 0x00, DEF_STR( Unused ))
	PORT_DIPSETTING(    0x20, DEF_STR( Off ))
	PORT_DIPSETTING(    0x00, DEF_STR( On ))
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Unused ))
	PORT_DIPSETTING(    0x40, DEF_STR( Off ))
	PORT_DIPSETTING(    0x00, DEF_STR( On ))
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Cabinet ))
	PORT_DIPSETTING(    0x80, DEF_STR( Upright ))
	PORT_DIPSETTING(    0x00, DEF_STR( Cocktail ))
INPUT_PORTS_END


INPUT_PORTS_START( bandido )
	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICKLEFT_RIGHT )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICKLEFT_LEFT )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICKLEFT_UP )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICKLEFT_DOWN )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_JOYSTICKRIGHT_RIGHT )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICKRIGHT_LEFT )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICKRIGHT_UP )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_JOYSTICKRIGHT_DOWN )

	PORT_START

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNUSED ) /* EXP1 */
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNUSED ) /* EXP2 */
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNUSED ) /* EXP3 enables diagnostic ROM at $2400 */
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_COIN1 )

	PORT_START
	PORT_DIPNAME( 0x03, 0x00, DEF_STR( Lives ))
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x01, "4" )
	PORT_DIPSETTING(    0x02, "5" )
	PORT_DIPSETTING(    0x03, "6" )
	PORT_DIPNAME( 0x04, 0x00, DEF_STR( Coinage ))
	PORT_DIPSETTING(    0x04, DEF_STR( 2C_1C ))
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ))
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Unused ))
	PORT_DIPSETTING(    0x08, DEF_STR( Off ))
	PORT_DIPSETTING(    0x00, DEF_STR( On ))
	PORT_DIPNAME( 0x10, 0x00, DEF_STR( Unused ))
	PORT_DIPSETTING(    0x10, DEF_STR( Off ))
	PORT_DIPSETTING(    0x00, DEF_STR( On ))
	PORT_DIPNAME( 0x20, 0x00, DEF_STR( Unused ))
	PORT_DIPSETTING(    0x20, DEF_STR( Off ))
	PORT_DIPSETTING(    0x00, DEF_STR( On ))
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Unused ))
	PORT_DIPSETTING(    0x40, DEF_STR( Off ))
	PORT_DIPSETTING(    0x00, DEF_STR( On ))
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Cabinet ))
	PORT_DIPSETTING(    0x80, DEF_STR( Upright ))
	PORT_DIPSETTING(    0x00, DEF_STR( Cocktail ))
INPUT_PORTS_END


INPUT_PORTS_START( helifire )
	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP    | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN  | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNUSED ) /* EXP1 */
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNUSED ) /* EXP2 */
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNUSED ) /* EXP3 */
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_COIN1 )

	PORT_START
	PORT_DIPNAME( 0x03, 0x00, DEF_STR( Lives ))
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x01, "4" )
	PORT_DIPSETTING(    0x02, "5" )
	PORT_DIPSETTING(    0x03, "6" )
	PORT_DIPNAME( 0x0c, 0x00, DEF_STR( Bonus_Life ))
	PORT_DIPSETTING(    0x00, "5000" )
	PORT_DIPSETTING(    0x04, "6000" )
	PORT_DIPSETTING(    0x08, "8000" )
	PORT_DIPSETTING(    0x0c, "10000" )
	PORT_DIPNAME( 0x10, 0x00, DEF_STR( Coinage ))
	PORT_DIPSETTING(    0x10, DEF_STR( 2C_1C ))
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ))
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Cabinet ))
	PORT_DIPSETTING(    0x80, DEF_STR( Upright ))
	PORT_DIPSETTING(    0x00, DEF_STR( Cocktail ))

	/* potentiometers */

	PORT_START	/* 04 */
	PORT_DIPNAME( 0xff, 0x50, "VR1 sun brightness" )
	PORT_DIPSETTING(    0x00, "00" )
	PORT_DIPSETTING(    0x10, "10" )
	PORT_DIPSETTING(    0x20, "20" )
	PORT_DIPSETTING(    0x30, "30" )
	PORT_DIPSETTING(    0x40, "40" )
	PORT_DIPSETTING(    0x50, "50" )
	PORT_DIPSETTING(    0x60, "60" )
	PORT_DIPSETTING(    0x70, "70" )

	PORT_START	/* 05 */
	PORT_DIPNAME( 0xff, 0x00, "VR2 sea brightness" )
	PORT_DIPSETTING(    0x00, "00" )
	PORT_DIPSETTING(    0x10, "10" )
	PORT_DIPSETTING(    0x20, "20" )
	PORT_DIPSETTING(    0x30, "30" )
	PORT_DIPSETTING(    0x40, "40" )
	PORT_DIPSETTING(    0x50, "50" )
	PORT_DIPSETTING(    0x60, "60" )
	PORT_DIPSETTING(    0x70, "70" )
INPUT_PORTS_END


ROM_START( spacefev )
	ROM_REGION( 0x8000, REGION_CPU1, 0 )
	ROM_LOAD( "f1-ro-.bin",  0x0000, 0x0400, CRC(35f295bd) SHA1(34d1df25fcdea598ca1191cecc2125e6f63dbce3) )
	ROM_LOAD( "f2-ro-.bin",  0x0400, 0x0400, CRC(0c633f4c) SHA1(a551ddbf21670fb1f000404b92da87a97f7ba157) )
	ROM_LOAD( "g1-ro-.bin",  0x0800, 0x0400, CRC(f3d851cb) SHA1(535c52a56e54a064aa3d1c48a129f714234a1007) )
	ROM_LOAD( "g2-ro-.bin",  0x0c00, 0x0400, CRC(1faef63a) SHA1(68e1bfc45587bfb1ee2eb477b60efd4f69dffd2c) )
	ROM_LOAD( "h1-ro-.bin",  0x1000, 0x0400, CRC(b365389d) SHA1(e681f2c5e37cc07912915ef74184ff9336309de3) )
	ROM_LOAD( "h2-ro-.bin",  0x1400, 0x0400, CRC(a163e800) SHA1(e8817f3e17f099a0dc66213d2d3d3fdeb117b10e) )
	ROM_LOAD( "i1-ro-p.bin", 0x1800, 0x0400, CRC(756b5582) SHA1(b7f3d218b7f4267ce6128624306396bcacb9b44e) )

	ROM_REGION( 0x0400, REGION_CPU2, 0 )
	ROM_LOAD( "ss3.ic2",     0x0000, 0x0400, CRC(95c2c1ee) SHA1(42a3a382fc7d2782052372d71f6d0e8a153e74d0) )

	ROM_REGION( 0x0020, REGION_PROMS, 0 )
	ROM_LOAD( "f5-i-.bin",   0x0000, 0x0020, CRC(c5914ec1) SHA1(198875fcab36d09c8726bb21e2fdff9882f6721a) )
ROM_END

ROM_START( spacefevo )
	ROM_REGION( 0x8000, REGION_CPU1, 0 )
	ROM_LOAD( "f1-ro-.bin",  0x0000, 0x0400, CRC(35f295bd) SHA1(34d1df25fcdea598ca1191cecc2125e6f63dbce3) )
	ROM_LOAD( "f2-ro-.bin",  0x0400, 0x0400, CRC(0c633f4c) SHA1(a551ddbf21670fb1f000404b92da87a97f7ba157) )
	ROM_LOAD( "g1-ro-.bin",  0x0800, 0x0400, CRC(f3d851cb) SHA1(535c52a56e54a064aa3d1c48a129f714234a1007) )
	ROM_LOAD( "g2-ro-.bin",  0x0c00, 0x0400, CRC(1faef63a) SHA1(68e1bfc45587bfb1ee2eb477b60efd4f69dffd2c) )
	ROM_LOAD( "h1-ro-.bin",  0x1000, 0x0400, CRC(b365389d) SHA1(e681f2c5e37cc07912915ef74184ff9336309de3) )
	ROM_LOAD( "h2-ro-.bin",  0x1400, 0x0400, CRC(a163e800) SHA1(e8817f3e17f099a0dc66213d2d3d3fdeb117b10e) )
	ROM_LOAD( "i1-ro-.bin",  0x1800, 0x0400, CRC(00027be2) SHA1(551a779a2e5a6455b7a348d246731c094e0ec709) )

	ROM_REGION( 0x0400, REGION_CPU2, 0 )
	ROM_LOAD( "ss3.ic2",     0x0000, 0x0400, CRC(95c2c1ee) SHA1(42a3a382fc7d2782052372d71f6d0e8a153e74d0) )

	ROM_REGION( 0x0020, REGION_PROMS, 0 )
	ROM_LOAD( "f5-i-.bin",   0x0000, 0x0020, CRC(c5914ec1) SHA1(198875fcab36d09c8726bb21e2fdff9882f6721a) )
ROM_END

ROM_START( highsplt )
	ROM_REGION( 0x8000, REGION_CPU1, 0 )
	ROM_LOAD( "f1-ha-.bin",  0x0000, 0x0400, CRC(b8887351) SHA1(ccd49937f1cbd7a157b3715474ccc3e8fdcea2b2) )
	ROM_LOAD( "f2-ha-.bin",  0x0400, 0x0400, CRC(cda933a7) SHA1(a0447c8c98e24674081c9bf4b1ef07dc186c6e2b) )
	ROM_LOAD( "g1-ha-.bin",  0x0800, 0x0400, CRC(de17578a) SHA1(d9d5dbf38331f212d2a566c60756a788e169104d) )
	ROM_LOAD( "g2-ha-.bin",  0x0c00, 0x0400, CRC(f1a90948) SHA1(850f27b42ca12bcba4aa95a1ad3e66206fa63554) )
	ROM_LOAD( "hs.h1",       0x1000, 0x0400, CRC(eefb4273) SHA1(853a62976a406516f10ac68dc2859399b8b7aae8) )
	ROM_LOAD( "h2-ha-.bin",  0x1400, 0x0400, CRC(e91703e8) SHA1(f58606b0c7d945e94c3fccc7ebe17ca25675e6a0) )
	ROM_LOAD( "hs.i1",       0x1800, 0x0400, CRC(41e18df9) SHA1(2212c836313775e7c507a875672c0b3635825e02) )
	ROM_LOAD( "i2-ha-.bin",  0x1c00, 0x0400, CRC(eff9f82d) SHA1(5004e52dfa652ceefca9ed4210c0fa8f0591dc08) )

	ROM_REGION( 0x0400, REGION_CPU2, 0 )
	ROM_LOAD( "ss4.bin",     0x0000, 0x0400, CRC(939e01d4) SHA1(7c9ccd24e5da03831cd0aa821da17e3b81cd8381) )

	ROM_REGION( 0x0020, REGION_PROMS, 0 )
	ROM_LOAD( "f5-i-.bin",   0x0000, 0x0020, CRC(c5914ec1) SHA1(198875fcab36d09c8726bb21e2fdff9882f6721a) )
ROM_END

ROM_START( highspla )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "f1-ha-.bin",  0x0000, 0x0400, CRC(b8887351) SHA1(ccd49937f1cbd7a157b3715474ccc3e8fdcea2b2) )
	ROM_LOAD( "f2-ha-.bin",  0x0400, 0x0400, CRC(cda933a7) SHA1(a0447c8c98e24674081c9bf4b1ef07dc186c6e2b) )
	ROM_LOAD( "g1-ha-.bin",  0x0800, 0x0400, CRC(de17578a) SHA1(d9d5dbf38331f212d2a566c60756a788e169104d) )
	ROM_LOAD( "g2-ha-.bin",  0x0c00, 0x0400, CRC(f1a90948) SHA1(850f27b42ca12bcba4aa95a1ad3e66206fa63554) )
	ROM_LOAD( "h1-ha-.bin",  0x1000, 0x0400, CRC(b0505da3) SHA1(f7b1f3a6dd06ff0cdeb6b13c948b7a262592514a) )
	ROM_LOAD( "h2-ha-.bin",  0x1400, 0x0400, CRC(e91703e8) SHA1(f58606b0c7d945e94c3fccc7ebe17ca25675e6a0) )
	ROM_LOAD( "i1-ha-.bin",  0x1800, 0x0400, CRC(aa36b25d) SHA1(28f555aab27b206a8c6f550b6caa938cece6e204) )
	ROM_LOAD( "i2-ha-.bin",  0x1c00, 0x0400, CRC(eff9f82d) SHA1(5004e52dfa652ceefca9ed4210c0fa8f0591dc08) )

	ROM_REGION( 0x0400, REGION_CPU2, 0 )
	ROM_LOAD( "ss4.bin",     0x0000, 0x0400, CRC(939e01d4) SHA1(7c9ccd24e5da03831cd0aa821da17e3b81cd8381) )

	ROM_REGION( 0x0020, REGION_PROMS, 0 )
	ROM_LOAD( "f5-i-.bin",   0x0000, 0x0020, CRC(c5914ec1) SHA1(198875fcab36d09c8726bb21e2fdff9882f6721a) )
ROM_END

ROM_START( spacelnc )
	ROM_REGION( 0x8000, REGION_CPU1, 0 )
	ROM_LOAD( "sl.f1",    0x0000, 0x0400, CRC(6ad59e40) SHA1(d416f7e6f5f55178df5c390548cd299650853022) )
	ROM_LOAD( "sl.f2",    0x0400, 0x0400, CRC(2de568e2) SHA1(f13740d3d9bf7434b7760e9286ef6e2ede40845f) )
	ROM_LOAD( "sl.g1",    0x0800, 0x0400, CRC(06d0ab36) SHA1(bf063100b065dbf511d6f32da169fb461568d15d) )
	ROM_LOAD( "sl.g2",    0x0c00, 0x0400, CRC(73ac4fe6) SHA1(7fa8c09692446bdf804900158e040f0b875a2e32) )
	ROM_LOAD( "sl.h1",    0x1000, 0x0400, CRC(7f42a94b) SHA1(ad85706de5e3f952b12756275be1ea1276a10666) )
	ROM_LOAD( "sl.h2",    0x1400, 0x0400, CRC(04b7a5f9) SHA1(589b0a0c8dcb1300623fe8478f1d7173b2bc575f) )
	ROM_LOAD( "sl.i1",    0x1800, 0x0400, CRC(d30007a3) SHA1(9e5905df8f7822385daef159a07f0e8257cb862a) )
	ROM_LOAD( "sl.i2",    0x1c00, 0x0400, CRC(640ffd2f) SHA1(65c21396c39dc99ec263f66f400a8e4c7712b20a) )

	ROM_REGION( 0x0400, REGION_CPU2, 0 )
	ROM_LOAD( "sl.snd",   0x0000, 0x0400, CRC(8e1ff929) SHA1(5c7da97b05fb8fff242158978199f5d35b234426) )

	ROM_REGION( 0x0020, REGION_PROMS, 0 )
	ROM_LOAD( "sf.prm",   0x0000, 0x0020, CRC(c5914ec1) SHA1(198875fcab36d09c8726bb21e2fdff9882f6721a) )
ROM_END

ROM_START( sheriff )
	ROM_REGION( 0x8000, REGION_CPU1, 0 )
	ROM_LOAD( "sh.f1",    0x0000, 0x0400, CRC(e79df6e8) SHA1(908176de9bfc3d48e2da9af6ba7ebdee698ec2de) )
	ROM_LOAD( "sh.f2",    0x0400, 0x0400, CRC(da67721a) SHA1(ee6a5fb98da1d1fcfad0ef27af300473a637f578) )
	ROM_LOAD( "sh.g1",    0x0800, 0x0400, CRC(3fb7888e) SHA1(2c2d6b27d577d5ccf759e451e53c2e3314af40f6) )
	ROM_LOAD( "sh.g2",    0x0c00, 0x0400, CRC(585fcfee) SHA1(82f2abc14f893c092b80da45fc297fa5fb0890b5) )
	ROM_LOAD( "sh.h1",    0x1000, 0x0400, CRC(e59eab52) SHA1(aa87710237dd48d1831f1b307d547b1b0707cd4e) )
	ROM_LOAD( "sh.h2",    0x1400, 0x0400, CRC(79e69a6a) SHA1(1780ce77d7d9ddbf4aceabe0fcf079339837bbe1) )
	ROM_LOAD( "sh.i1",    0x1800, 0x0400, CRC(dda7d1e8) SHA1(bd2a7388e81c71922b2e97d68be71359a75e8d37) )
	ROM_LOAD( "sh.i2",    0x1c00, 0x0400, CRC(5c5f3f86) SHA1(25c64ccb7d0e136f67d6e1da7927ae6d89e0ceb9) )
	ROM_LOAD( "sh.j1",    0x2000, 0x0400, CRC(0aa8b79a) SHA1(aed139e8c8ba912823c57fe4cc7231b2d638f479) )

	ROM_REGION( 0x0400, REGION_CPU2, 0 )
	ROM_LOAD( "sh.snd",   0x0000, 0x0400, CRC(75731745) SHA1(538a63c9c60f1886fca4caf3eb1e0bada2d3f162) )

	ROM_REGION( 0x0400, REGION_PROMS, 0 )
	ROM_LOAD( "82s137.3l", 0x0000, 0x0400, CRC(820f8cdd) SHA1(197eeb008c140558e7c1ab2b2bd0f6a27096877c) )
ROM_END

ROM_START( bandido )
	ROM_REGION( 0x8000, REGION_CPU1, 0 )
	ROM_LOAD( "sh-a.f1",  0x0000, 0x0400, CRC(aec94829) SHA1(aa6d241670ea061bac4a71dff82dfa832095eae6) )
	ROM_LOAD( "sh.f2",    0x0400, 0x0400, CRC(da67721a) SHA1(ee6a5fb98da1d1fcfad0ef27af300473a637f578) )
	ROM_LOAD( "sh.g1",    0x0800, 0x0400, CRC(3fb7888e) SHA1(2c2d6b27d577d5ccf759e451e53c2e3314af40f6) )
	ROM_LOAD( "sh.g2",    0x0c00, 0x0400, CRC(585fcfee) SHA1(82f2abc14f893c092b80da45fc297fa5fb0890b5) )
	ROM_LOAD( "sh-a.h1",  0x1000, 0x0400, CRC(5cb63677) SHA1(59a8e5f8b134bf44d3e5a1105a9346f0c5f9378e) )
	ROM_LOAD( "sh.h2",    0x1400, 0x0400, CRC(79e69a6a) SHA1(1780ce77d7d9ddbf4aceabe0fcf079339837bbe1) )
	ROM_LOAD( "sh.i1",    0x1800, 0x0400, CRC(dda7d1e8) SHA1(bd2a7388e81c71922b2e97d68be71359a75e8d37) )
	ROM_LOAD( "sh.i2",    0x1c00, 0x0400, CRC(5c5f3f86) SHA1(25c64ccb7d0e136f67d6e1da7927ae6d89e0ceb9) )
	ROM_LOAD( "sh.j1",    0x2000, 0x0400, CRC(0aa8b79a) SHA1(aed139e8c8ba912823c57fe4cc7231b2d638f479) )
	ROM_LOAD( "sh-a.j2",  0x2400, 0x0400, CRC(a10b848a) SHA1(c045f1f6a11cbf49a1bae06c701b659d587292a3) )

	ROM_REGION( 0x0400, REGION_CPU2, 0 )
	ROM_LOAD( "sh.snd",   0x0000, 0x0400, CRC(75731745) SHA1(538a63c9c60f1886fca4caf3eb1e0bada2d3f162) )

	ROM_REGION( 0x0400, REGION_PROMS, 0 )
	ROM_LOAD( "82s137.3l", 0x0000, 0x0400, CRC(820f8cdd) SHA1(197eeb008c140558e7c1ab2b2bd0f6a27096877c) )
ROM_END

ROM_START( helifire )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "tub_f1_b",  0x0000, 0x0400, CRC(032f89ca) SHA1(63b0310875ed78a6385e44eea781ddcc4a63557c) )
	ROM_LOAD( "tub_f2_b",  0x0400, 0x0400, CRC(2774e70f) SHA1(98d845e80db61799493dbebe8db801567277432c) )
	ROM_LOAD( "tub_g1_b",  0x0800, 0x0400, CRC(b5ad6e8a) SHA1(1eb4931e85bd6a559e85a2b978d383216d3988a7) )
	ROM_LOAD( "tub_g2_b",  0x0c00, 0x0400, CRC(5e015bf4) SHA1(60f5a9707c8655e54a8381afd764856fb25c29f1) )
	ROM_LOAD( "tub_h1_b",  0x1000, 0x0400, CRC(23bb4e5a) SHA1(b59bc0adff3635aca1def2b1997f7edc6ca7e8ee) )
	ROM_LOAD( "tub_h2_b",  0x1400, 0x0400, CRC(358227c6) SHA1(d7bd678ef1737edc6aa609e43e3ae96a8d61dc15) )
	ROM_LOAD( "tub_i1_b",  0x1800, 0x0400, CRC(0c679f44) SHA1(cbe31dbe5f2c5f11a637cb3bde4e059c310d0e76) )
	ROM_LOAD( "tub_i2_b",  0x1c00, 0x0400, CRC(d8b7a398) SHA1(3ddfeac39147d5df6096f525f7ef67abef32a28b) )
	ROM_LOAD( "tub_j1_b",  0x2000, 0x0400, CRC(98ef24db) SHA1(70ad8dd6e1e8f4bf4ce431737ca1856eecc03d53) )
	ROM_LOAD( "tub_j2_b",  0x2400, 0x0400, CRC(5e2b5877) SHA1(f7c747e8a1d9fe2dda71ee6304636cf3cdf727a7) )

	ROM_REGION( 0x1000, REGION_CPU2, 0 )
	ROM_LOAD( "tub-e_ic5-a", 0x0000, 0x0400, CRC(9d77a31f) SHA1(36db9b5087b6661de88042854874bc247c92d985) )
ROM_END

ROM_START( helifira )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "hf-a.f1",  0x0000, 0x0400, CRC(92c9d6c1) SHA1(860a7b3980e9e11d48769fad347c965e04ed3f89) )
	ROM_LOAD( "hf-a.f2",  0x0400, 0x0400, CRC(a264dde8) SHA1(48f972ad5af6c2ab61117f60d9244df6df6d313c) )
	ROM_LOAD( "hf.g1",    0x0800, 0x0400, CRC(b5ad6e8a) SHA1(1eb4931e85bd6a559e85a2b978d383216d3988a7) )
	ROM_LOAD( "hf-a.g2",  0x0c00, 0x0400, CRC(a987ebcd) SHA1(46726293c308c18b28941809419ba4c2ffc8084f) )
	ROM_LOAD( "hf-a.h1",  0x1000, 0x0400, CRC(25abcaf0) SHA1(a14c795de1fc283405f71bb83f4ac5c98fd406cb) )
	ROM_LOAD( "hf.h2",    0x1400, 0x0400, CRC(358227c6) SHA1(d7bd678ef1737edc6aa609e43e3ae96a8d61dc15) )
	ROM_LOAD( "hf.i1",    0x1800, 0x0400, CRC(0c679f44) SHA1(cbe31dbe5f2c5f11a637cb3bde4e059c310d0e76) )
	ROM_LOAD( "hf-a.i2",  0x1c00, 0x0400, CRC(296610fd) SHA1(f1ab379983e45f3cd718dd82962c609297b4dcb8) )
	ROM_LOAD( "hf.j1",    0x2000, 0x0400, CRC(98ef24db) SHA1(70ad8dd6e1e8f4bf4ce431737ca1856eecc03d53) )
	ROM_LOAD( "hf.j2",    0x2400, 0x0400, CRC(5e2b5877) SHA1(f7c747e8a1d9fe2dda71ee6304636cf3cdf727a7) )

	ROM_REGION( 0x1000, REGION_CPU2, 0 )
	ROM_LOAD( "hf.snd",   0x0000, 0x0400, CRC(9d77a31f) SHA1(36db9b5087b6661de88042854874bc247c92d985) )
ROM_END


GAME (1979, spacefev, 0,        spacefev, spacefev, 0, ROT270, "Nintendo", "Space Fever (New Ver)" )
GAME (1979, spacefevo,spacefev, spacefev, spacefev, 0, ROT270, "Nintendo", "Space Fever (Old Ver)" )
GAME (1979, highsplt, 0,        spacefev, highsplt, 0, ROT270, "Nintendo", "Space Fever High Splitter (set 1)" )
GAME (1979, highspla, highsplt, spacefev, highsplt, 0, ROT270, "Nintendo", "Space Fever High Splitter (set 2)" )
GAME (1979, spacelnc, 0,        spacefev, spacelnc, 0, ROT270, "Nintendo", "Space Launcher" )
GAME (1979, sheriff,  0,        sheriff,  sheriff,  0, ROT270, "Nintendo", "Sheriff" )
GAME (1980, bandido,  sheriff,  sheriff,  bandido,  0, ROT270, "Exidy",    "Bandido" )
GAMEX(1980, helifire, 0,        helifire, helifire, 0, ROT270, "Nintendo", "HeliFire (set 1)", GAME_IMPERFECT_SOUND | GAME_IMPERFECT_GRAPHICS )
GAMEX(1980, helifira, helifire, helifire, helifire, 0, ROT270, "Nintendo", "HeliFire (set 2)", GAME_IMPERFECT_SOUND | GAME_IMPERFECT_GRAPHICS )
