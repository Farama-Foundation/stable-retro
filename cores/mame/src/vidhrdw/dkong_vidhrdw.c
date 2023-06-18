/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/
#include "driver.h"
#include "dkong.h"
#include "res_net.h"
#include "vidhrdw/generic.h"
#include <math.h>
#include "machine/random.h"

#define RADARSCP_BCK_COL_OFFSET			256
#define RADARSCP_GRID_COL_OFFSET		(RADARSCP_BCK_COL_OFFSET + 256)
#define RADARSCP_STAR_COL				(RADARSCP_GRID_COL_OFFSET + 8)

/*use scanline timer for backfround */

static int gfx_bank, palette_bank;

	/* radar scope */
	UINT8           sig30Hz = 0;
	UINT8           lfsr_5I = 0;
	UINT8           grid_sig = 0;
	UINT8           rflip_sig = 0;
	UINT8           star_ff = 0;
	UINT8           blue_level = 0;
	UINT8           grid_on = 0;
	UINT16          grid_col = 0;
	UINT8           snd02_enable=0;
	UINT8           p02_b5_enable;
	double          cd4049_a = 0.0;
	double          cd4049_b = 0.0;

	/* radarscp_step */
	double            cv1 = 0.0;
	double            cv2 = 0.0;
	double            vg1 = 0.0;
	double            vg2 = 0.0;
	double            vg3 = 0.0;
	double            cv3 = 0.0;
	double            cv4 = 0.0;
	double            vc17 = 0.0;
	int               pixelcnt = 0;
	int               counter = 0;

static const double cd4049_vl = 1.5/5.0;
static const double cd4049_vh = 3.5/5.0;
static const double cd4049_al = 0.01;

static const UINT8 *color_codes;

static struct mame_bitmap *bg_bits;
static struct tilemap *bg_tilemap;

WRITE_HANDLER( dkong_videoram_w )
{
	if (videoram[offset] != data)
	{
		videoram[offset] = data;
		tilemap_mark_tile_dirty(bg_tilemap, offset);
	}
}

void set_var(void)
{
	/* radar scope */
	sig30Hz = 0;
	lfsr_5I = 0;
	grid_sig = 0;
	rflip_sig = 0;
	star_ff = 0;
	blue_level = 0;
	grid_on = 0;
	grid_col = 0;
	snd02_enable=0;
	counter = 0;

	/* radarscp_step */
	cv1 = 0.0;
	cv2 = 0.0;
	vg1 = 0.0;
	vg2 = 0.0;
	vg3 = 0.0;
	cv3 = 0.0;
	cv4 = 0.0;
	vc17 = 0.0;
	pixelcnt = 0;
}

/***************************************************************************

  Convert the color PROMs into a more useable format.

  Donkey Kong has two 256x4 palette PROMs and one 256x4 PROM which contains
  the color codes to use for characters on a per row/column basis (groups of
  of 4 characters in the same column - actually row, since the display is
  rotated)
  The palette PROMs are connected to the RGB output this way:

    5V  -- 470 ohm resistor -- inverter  -- RED
  bit 3 -- 220 ohm resistor -- inverter  -- RED
        -- 470 ohm resistor -- inverter  -- RED
        -- 1  kohm resistor -- inverter  -- RED

   5V  -- 470 ohm resistor -- inverter  -- GREEN
  bit 0 -- 220 ohm resistor -- inverter  -- GREEN
  bit 3 -- 470 ohm resistor -- inverter  -- GREEN
        -- 1  kohm resistor -- inverter  -- GREEN
    5V  -- 680 ohm resistor -- inverter  -- BLUE
        -- 220 ohm resistor -- inverter  -- BLUE
  bit 0 -- 470 ohm resistor -- inverter  -- BLUE

  After the mixing stage there is a darlington circuit with approx. linear
  transfer function for red and green. Minimum output voltage is 0.9 Volt.
  Blue is transferred through just one transistor. Here we have to
  substract 0.7 Volts. This signal (0..5V) is inverted by a amplifier
  circuit with an emitter lowpass RC. The R is a variable resistor.
  So any calculations done here may be void in reality ...
***************************************************************************/

/*
    dkong color interface:

    All outputs are open-collector and pullup resistors are connected to 5V.
    Red and Green outputs are routed through a complementary darlington
    whereas blue is routed through a 1:1 stage leading to a 0.7V cutoff.
*/

static const res_net_decode_info dkong_decode_info =
{
	2,      /*  there may be two proms needed to construct color */
	0,      /*  start at 0 */
	255,    /*  end at 255 */
	/*  R,   G,   B,   R,   G,   B */
	{ 256, 256,   0,   0,   0,   0},        /*  offsets */
	{   1,  -2,   0,   0,   2,   0},        /*  shifts */
	{0x07,0x04,0x03,0x00,0x03,0x00}         /*  masks */
};

static const res_net_info dkong_net_info =
{
	RES_NET_VCC_5V | RES_NET_VBIAS_5V | RES_NET_VIN_MB7052 |  RES_NET_MONITOR_SANYO_EZV20,
	{
		{ RES_NET_AMP_DARLINGTON, 470, 0, 3, { 1000, 470, 220 } },
		{ RES_NET_AMP_DARLINGTON, 470, 0, 3, { 1000, 470, 220 } },
		{ RES_NET_AMP_EMITTER,    680, 0, 2, {  470, 220,   0 } }  /*  dkong */
	}
};

static const res_net_info dkong_net_bck_info =
{
	RES_NET_VCC_5V | RES_NET_VBIAS_5V | RES_NET_VIN_MB7052 |  RES_NET_MONITOR_SANYO_EZV20,
	{
		{ RES_NET_AMP_DARLINGTON, 470, 0, 0, { 0 } },
		{ RES_NET_AMP_DARLINGTON, 470, 0, 0, { 0 } },
		{ RES_NET_AMP_EMITTER,    680, 0, 0, { 0 } }
	}
};

static const res_net_decode_info dkong3_decode_info =
{
	1,      /*  one prom needed to contruct color */
	0,      /*  start at 0 */
	255,    /*  end at 255 */
	/*   R,   G,   B */
	{   0,   0, 512 },      /*  offsets */
	{   4,   0,   0 },      /*  shifts */
	{0x0F,0x0F,0x0F }       /*  masks */
};

static const res_net_info dkong3_net_info =
{
	RES_NET_VCC_5V | RES_NET_VBIAS_5V | RES_NET_VIN_MB7052 |  RES_NET_MONITOR_SANYO_EZV20,
	{
		{ RES_NET_AMP_DARLINGTON, 470,      0, 4, { 2200, 1000, 470, 220 } },
		{ RES_NET_AMP_DARLINGTON, 470,      0, 4, { 2200, 1000, 470, 220 } },
		{ RES_NET_AMP_DARLINGTON, 470,      0, 4, { 2200, 1000, 470, 220 } }
	}
};

/*
    radarscp interface

    All outputs are open-collector. The pullup resistors are connected to
    inverters (TTL). All outputs are routed through a complimentary
    darlington. The blue channel has a pulldown resistor (R8, 0M15) as well.
*/

#define TRS_J1  (1)         /* (1) = Closed (0) = Open */

static const res_net_info radarscp_net_info =
{
	RES_NET_VCC_5V | RES_NET_VBIAS_TTL | RES_NET_VIN_MB7052 |  RES_NET_MONITOR_SANYO_EZV20,
	{
		{ RES_NET_AMP_DARLINGTON, 470 * TRS_J1, 470*(1-TRS_J1), 3, { 1000, 470, 220 } },
		{ RES_NET_AMP_DARLINGTON, 470 * TRS_J1, 470*(1-TRS_J1), 3, { 1000, 470, 220 } },
		{ RES_NET_AMP_EMITTER,    680 * TRS_J1, 680*(1-TRS_J1), 2, {  470, 220,   0 } }    /*  radarscp */
	}
};

static const res_net_info radarscp_net_bck_info =
{
	RES_NET_VCC_5V | RES_NET_VBIAS_TTL | RES_NET_VIN_MB7052 |  RES_NET_MONITOR_SANYO_EZV20,
	{
		{ RES_NET_AMP_DARLINGTON, 470, 4700, 0, { 0 } },
		{ RES_NET_AMP_DARLINGTON, 470, 4700, 0, { 0 } },
		{ RES_NET_AMP_EMITTER,    470, 4700, 0, { 0 } }    /*  radarscp */
	}
};

/*
    radarscp1 interface

    All outputs are open-collector. They are followed by inverters which
    drive the resistor network. All outputs are routed through a complimentary
    darlington.
*/

static const res_net_info radarscp1_net_info =
{
	RES_NET_VCC_5V | RES_NET_VBIAS_5V | RES_NET_VIN_TTL_OUT | RES_NET_MONITOR_SANYO_EZV20,
	{
		{ RES_NET_AMP_DARLINGTON, 0,      0, 4, { 39000, 20000, 10000, 4990 } },
		{ RES_NET_AMP_DARLINGTON, 0,      0, 4, { 39000, 20000, 10000, 4990 } },
		{ RES_NET_AMP_EMITTER,    0,      0, 4, { 39000, 20000, 10000, 4990 } }
	}
};

/* Radarscp star color */

static const res_net_info radarscp_stars_net_info =
{
	RES_NET_VCC_5V | RES_NET_VBIAS_5V | RES_NET_VIN_TTL_OUT | RES_NET_MONITOR_SANYO_EZV20,
	{
		{ RES_NET_AMP_DARLINGTON, 4700, 470, 0, { 0 } },
		{ RES_NET_AMP_DARLINGTON,    1,   0, 0, { 0 } },    /*  dummy */
		{ RES_NET_AMP_EMITTER,       1,   0, 0, { 0 } },    /*  dummy */
	}
};

/* Dummy struct to generate background palette entries */

static const res_net_info radarscp_blue_net_info =
{
	RES_NET_VCC_5V | RES_NET_VBIAS_5V | RES_NET_VIN_VCC | RES_NET_MONITOR_SANYO_EZV20,
	{
		{ RES_NET_AMP_DARLINGTON,  470, 4700, 0, { 0 } },   /*  bias/gnd exist in schematics, readable in TKG3 schematics */
		{ RES_NET_AMP_DARLINGTON,  470, 4700, 0, { 0 } },   /*  bias/gnd exist in schematics, readable in TKG3 schematics */
		{ RES_NET_AMP_EMITTER,       0,    0, 8, { 128,64,32,16,8,4,2,1 } },    /*  dummy */
	}
};

/* Dummy struct to generate grid palette entries */

static const res_net_info radarscp_grid_net_info =
{
	RES_NET_VCC_5V | RES_NET_VBIAS_5V | RES_NET_VIN_TTL_OUT | RES_NET_MONITOR_SANYO_EZV20,
	{
		{ RES_NET_AMP_DARLINGTON,    0,   0, 1, { 1 } },    /*  dummy */
		{ RES_NET_AMP_DARLINGTON,    0,   0, 1, { 1 } },    /*  dummy */
		{ RES_NET_AMP_EMITTER,       0,   0, 1, { 1 } },    /*  dummy */
	}
};

PALETTE_INIT( dkong)
{
	int i;
	rgb_t   *rgb;
	rgb = compute_res_net_all(color_prom, &dkong_decode_info, &dkong_net_info);
	res_palette_set_colors(0, rgb, 256);
	free(rgb);

	/* Now treat tri-state black background generation */

	for (i=0;i<256;i++)
		if ( (i & 0x03) == 0x00 )  /*  NOR => CS=1 => Tristate => real black */
		{
			int r,g,b;
			r = compute_res_net( 1, 0, &dkong_net_bck_info );
			g = compute_res_net( 1, 1, &dkong_net_bck_info );
			b = compute_res_net( 1, 2, &dkong_net_bck_info );
			palette_set_color( i, r, g, b );
		}
	palette_normalize_range(0, 255, 0, 255);
	color_prom += 512;
	/* color_prom now points to the beginning of the character color codes */
	color_codes = color_prom;	/* we'll need it later */
}

PALETTE_INIT( radarscp )
{
	int i;
	int r,g,b;

	for (i = 0; i < 256; i++)
	{
		r = compute_res_net((color_prom[256] >> 1) & 0x07, 0, &radarscp_net_info);
		g = compute_res_net(((color_prom[256] << 2) & 0x04) | ((color_prom[0] >> 2) & 0x03), 1, &radarscp_net_info);
		b = compute_res_net((color_prom[0] >> 0) & 0x03, 2, &radarscp_net_info);

		palette_set_color(i,r,g,b);
		color_prom++;
	}

	/* Now treat tri-state black background generation */
	for (i = 0; i < 256; i++)
		if ((/*m_vidhw != DKONG_RADARSCP_CONVERSION*/ 1) && ((i & 0x03) == 0x00)) /* NOR => CS=1 => Tristate => real black */
		{
			r = compute_res_net(1, 0, &radarscp_net_bck_info);
			g = compute_res_net(1, 1, &radarscp_net_bck_info);
			b = compute_res_net(1, 2, &radarscp_net_bck_info);
			palette_set_color(i,r,g,b);
		}

	/* Star color */
	palette_set_color(RADARSCP_STAR_COL,
			compute_res_net(1, 0, &radarscp_stars_net_info),
			compute_res_net(0, 1, &radarscp_stars_net_info),
			compute_res_net(0, 2, &radarscp_stars_net_info));

	/* Oscillating background */
	for (i = 0; i < 256; i++)
	{
		r = compute_res_net(0, 0, &radarscp_blue_net_info);
		g = compute_res_net(0, 1, &radarscp_blue_net_info);
		b = compute_res_net(i, 2, &radarscp_blue_net_info);

		palette_set_color(RADARSCP_BCK_COL_OFFSET + i, r, g, b);
	}

	/* Grid */
	for (i = 0; i < 8; i++)
	{
		r = compute_res_net(BIT(i, 0), 0, &radarscp_grid_net_info);
		g = compute_res_net(BIT(i, 1), 1, &radarscp_grid_net_info);
		b = compute_res_net(BIT(i, 2), 2, &radarscp_grid_net_info);

		palette_set_color(RADARSCP_GRID_COL_OFFSET + i, r, g, b);
	}

	palette_normalize_range(0, RADARSCP_GRID_COL_OFFSET+7, 0, 255);

	color_prom += 256;
	/* color_prom now points to the beginning of the character color codes */
	color_codes = color_prom; /* we'll need it later */
}

/***************************************************************************

  Convert the color PROMs into a more useable format.

  Donkey Kong 3 has two 512x8 palette PROMs and one 256x4 PROM which contains
  the color codes to use for characters on a per row/column basis (groups of
  of 4 characters in the same column - actually row, since the display is
  rotated)
  Interstingly, bytes 0-255 of the palette PROMs contain an inverted palette,
  as other Nintendo games like Donkey Kong, while bytes 256-511 contain a non
  inverted palette. This was probably done to allow connection to both the
  special Nintendo and a standard monitor.
  I don't know the exact values of the resistors between the PROMs and the
  RGB output, but they are probably the usual:

  bit 7 -- 220 ohm resistor -- inverter  -- RED
        -- 470 ohm resistor -- inverter  -- RED
        -- 1  kohm resistor -- inverter  -- RED
        -- 2.2kohm resistor -- inverter  -- RED
        -- 220 ohm resistor -- inverter  -- GREEN
        -- 470 ohm resistor -- inverter  -- GREEN
        -- 1  kohm resistor -- inverter  -- GREEN
  bit 0 -- 2.2kohm resistor -- inverter  -- GREEN

  bit 3 -- 220 ohm resistor -- inverter  -- BLUE
        -- 470 ohm resistor -- inverter  -- BLUE
        -- 1  kohm resistor -- inverter  -- BLUE
  bit 0 -- 2.2kohm resistor -- inverter  -- BLUE

***************************************************************************/

PALETTE_INIT( dkong3 )
{
	rgb_t	*rgb;

	rgb = compute_res_net_all(color_prom, &dkong3_decode_info, &dkong3_net_info);
	res_palette_set_colors(0, rgb, 256);
	palette_normalize_range(0, 255, 0, 255);
	free(rgb);

	color_prom += 1024;
	/* color_prom now points to the beginning of the character color codes */
	color_codes = color_prom;	/* we'll need it later */
}

static void get_bg_tile_info(int tile_index)
{
	int code = videoram[tile_index] + 256 * gfx_bank;
	int color = (color_codes[tile_index % 32 + 32 * (tile_index / 32 / 4)] & 0x0f) + 0x10 * palette_bank;

	SET_TILE_INFO(0, code, color, 0)
}

VIDEO_START( dkong )
{
	gfx_bank = 0;
	palette_bank = 0;

	bg_tilemap = tilemap_create(get_bg_tile_info, tilemap_scan_rows,
		TILEMAP_OPAQUE, 8, 8, 32, 32);

	if ( !bg_tilemap )
		return 1;

	return 0;
}

VIDEO_START( radarscp )
{
	cd4049_b = (log(0.0 - log(cd4049_al)) - log(0.0 - log((1.0-cd4049_al))) ) / log(cd4049_vh/cd4049_vl);
	cd4049_a = log(0.0 - log(cd4049_al)) - cd4049_b * log(cd4049_vh);
	gfx_bank = 0;
	palette_bank = 0;

	bg_tilemap = tilemap_create(get_bg_tile_info, tilemap_scan_rows,
		TILEMAP_OPAQUE, 8, 8, 32, 32);

	if ( !bg_tilemap )
		return 1;

	bg_bits = auto_bitmap_alloc_depth(Machine->drv->screen_width,Machine->drv->screen_height,16);

	return 0;
}

WRITE_HANDLER( dkongjr_gfxbank_w )
{
	if (gfx_bank != (data & 0x01))
	{
		gfx_bank = data & 0x01;
		tilemap_mark_all_tiles_dirty(bg_tilemap);
	}
}

WRITE_HANDLER( dkong3_gfxbank_w )
{
	if (gfx_bank != (~data & 0x01))
	{
		gfx_bank = ~data & 0x01;
		tilemap_mark_all_tiles_dirty(bg_tilemap);
	}
}

WRITE_HANDLER( dkong_palettebank_w )
{
	int newbank;

	newbank = palette_bank;

	if (data & 1)
		newbank |= 1 << offset;
	else
		newbank &= ~(1 << offset);

	if (palette_bank != newbank)
	{
		palette_bank = newbank;
		tilemap_mark_all_tiles_dirty(bg_tilemap);
	}
}

WRITE_HANDLER( radarscp_grid_enable_w )
{
	grid_on = data & 0x01;
}

WRITE_HANDLER( radarscp_grid_color_w )
{
	grid_col = (data & 0x07) ^ 0x07;
}

WRITE_HANDLER( dkong_flipscreen_w )
{
	flip_screen_set(data & 0x01);
}

/***************************************************************************
  Draw the game screen in the given mame_bitmap.
  Do NOT call osd_update_display() from this function, it will be called by
  the main emulation engine.
***************************************************************************/

static void draw_sprites(struct mame_bitmap *bitmap, unsigned int mask_bank, unsigned int shift_bits)
{
	int offs;

	/* Draw the sprites. */
	for (offs = 0;offs < spriteram_size;offs += 4)
	{
		if (spriteram[offs])
		{
			/* spriteram[offs + 2] & 0x40 is used by Donkey Kong 3 only */
			/* spriteram[offs + 2] & 0x30 don't seem to be used (they are */
			/* probably not part of the color code, since Mario Bros, which */
			/* has similar hardware, uses a memory mapped port to change */
			/* palette bank, so it's limited to 16 color codes) */

			int x,y;

			x = spriteram[offs + 3] - 8;
			y = 240 - spriteram[offs] + 7;

			if (flip_screen)
			{
				x = 240 - x;
				y = 240 - y;

				drawgfx(bitmap,Machine->gfx[1],
						(spriteram[offs + 1] & 0x7f) + ((spriteram[offs + 2] & mask_bank) << shift_bits),
						(spriteram[offs + 2] & 0x0f) + 16 * palette_bank,
						!(spriteram[offs + 2] & 0x80),!(spriteram[offs + 1] & 0x80),
						x,y,
						&Machine->visible_area,TRANSPARENCY_PEN,0);

				/* draw with wrap around - this fixes the 'beheading' bug */
				drawgfx(bitmap,Machine->gfx[1],
						(spriteram[offs + 1] & 0x7f) + ((spriteram[offs + 2] & mask_bank) << shift_bits),
						(spriteram[offs + 2] & 0x0f) + 16 * palette_bank,
						(spriteram[offs + 2] & 0x80),(spriteram[offs + 1] & 0x80),
						x-256,y,
						&Machine->visible_area,TRANSPARENCY_PEN,0);
			}
			else
			{
				drawgfx(bitmap,Machine->gfx[1],
						(spriteram[offs + 1] & 0x7f) + ((spriteram[offs + 2] & mask_bank) << shift_bits),
						(spriteram[offs + 2] & 0x0f) + 16 * palette_bank,
						(spriteram[offs + 2] & 0x80),(spriteram[offs + 1] & 0x80),
						x,y,
						&Machine->visible_area,TRANSPARENCY_PEN,0);

				/* draw with wrap around - this fixes the 'beheading' bug */
				drawgfx(bitmap,Machine->gfx[1],
						(spriteram[offs + 1] & 0x7f) + ((spriteram[offs + 2] & mask_bank) << shift_bits),
						(spriteram[offs + 2] & 0x0f) + 16 * palette_bank,
						(spriteram[offs + 2] & 0x80),(spriteram[offs + 1] & 0x80),
						x+256,y,
						&Machine->visible_area,TRANSPARENCY_PEN,0);
			}
		}
	}
}

/* The hardware designer must have been a real CD4049 fan
 * The blue signal is created by "abusing" the 4049
 * as an amplifier by operating it in the undefined area
 * between roughly 1.5 and 3.5V. The transfer function
 * below is an approximation but datasheets state
 * a wide range for the transfer function.
 *
 * SwitcherCad was not a real help since it can not
 * adequately model the 4049
 *
 * Sound02 will mix in noise into the periodic fluctuation
 * of the background: The 30Hz signal is used
 * to generate noise with an LS164. This signal is going
 * through a NAND (Signal RFLIP to video) and then XOR with 128V.
 * This should really be emulated using the discrete sound interface.
 * TODO: This should be part of the vblank routine
 */

static INLINE double CD4049(double x)
{
	if (x>0)
	 	return exp(-cd4049_a * pow(x,cd4049_b));
	else
		return 1.0;
}

/* Actually the sound noise is a multivibrator with
 * a period of roughly 4.4 ms
 */

#define RC1		(2.2e3 * 22e-6)
#define RC2		(10e3 * 33e-6)
#define RC31	(18e3 * 33e-6)
#define RC32	((18e3 + 68e3) * 33e-6)
#define RC4		(90e3 * 0.47e-6)
#define dt		(1./60./(double) VTOTAL)
#define period2 (((int64_t)(PIXEL_CLOCK) * ( 33L * 68L )) / (int32_t)10000000L / 3)  /*  period/2 in pixel ... */

/* re checked the ported code is good
 * todo confirm pixel clock speed
 *  */

void radarscp_step(int line_cnt)
{
	/* Condensator is illegible in schematics for TRS2 board.
	 * TRS1 board states 3.3u.
	 */

	double vg3i, vo;
	double diff;
	int sig, j;

	/* vsync is divided by 2 by a LS161
	 * The resulting 30 Hz signal clocks a LFSR (LS164) operating as a
	 * random number generator.
	 */

	if ( line_cnt == 0)
	{
		sig30Hz = (1-sig30Hz);
		if (sig30Hz)
			lfsr_5I = (mame_rand() > RAND_MAX/2);
	}

	/* sound2 mixes in a 30Hz noise signal.
	 * With the current model this has no real effect
	 * Included for completeness
	 */

	/* Now mix with SND02 (sound 2) line - on 74ls259, bit2 */
	rflip_sig = snd02_enable & lfsr_5I;

	/* blue background generation */

	line_cnt += (256 - 8) + 1; /* offset 8 needed to match monitor pictures */
	if (line_cnt>511)
		line_cnt -= VTOTAL;

	sig = rflip_sig ^ ((line_cnt & 0x80)>>7);

	/*if (hardware_type == HARDWARE_TRS01)
	 *	rflip_sig = !rflip_sig; */

	if  (sig) /*  128VF */
		diff = (0.0 - cv1);
	else
		diff = (4.8 - cv1);
	diff = diff - diff*exp(0.0 - (1.0/RC1 * dt) );
	cv1 += diff;

	diff = (cv1 - cv2 - vg1);
	diff = diff - diff*exp(0.0 - (1.0/RC2 * dt) );
	cv2 += diff;

	/* FIXME: use the inverse function
	 * Solve the amplifier by iteration*/
	for (j=1; j<=11; j++)/* 11% = 1/75 / (1/75+1/10)*/
	{
		double f = (double) j / 100.0;
		vg1 = (cv1 - cv2)*(1-f) + f * vg2;
		vg2 = 5*CD4049(vg1/5);
	}
	/* FIXME: use the inverse function
	 * Solve the amplifier by iteration 50% = both resistors equal*/
	for (j=10; j<=20; j++)
	{
		double f = (double) j / 40.0;
		vg3i = (1.0-f) * vg2 + f * vg3;
		vg3 = 5*CD4049(vg3i/5);
	}

#define RC17 (33e-6 * 1e3 * (0*4.7+1.0/(1.0/10.0+1.0/20.0+0.0/0.3)))
	diff = (vg3 - vc17);
	diff = diff - diff*exp(0.0 - (1.0/RC17 * dt) );
	vc17 += diff;

	vo = (vg3 - vc17);
	vo = vo + 20.0 / (20.0+10.0) * 5;

	/* Transistor is marked as OMIT in TRS-02 schems. */
	/*vo = vo - 0.7; */


	/*double vo = (vg3o - vg3)/4.7 + 5.0/16.0; */
	/*vo = vo / (1.0 / 4.7 + 1.0 / 16.0 + 1.0 / 30.0 ); */
	/*printf("%f %f\n", vg3, vc17); */

	blue_level = (int)(vo/5.0*255);
	/* printf("%d\n", m_blue_level); */

	/*
	 * Grid signal
	 *
	 * Mixed with ANS line (bit 5) from Port B of 8039
	 */
	if (grid_on && p02_b5_enable)
	{
		diff = (0.0 - cv3);
		diff = diff - diff*exp(0.0 - (1.0/RC32 * dt) );
	}
	else
	{
		diff = (5.0 - cv3);
		diff = diff - diff*exp(0.0 - (1.0/RC31 * dt) );
	}
	cv3 += diff;

	diff = (vg2 - 0.8 * cv3 - cv4);
	diff = diff - diff*exp(0.0 - (1.0/RC4 * dt) );
	cv4 += diff;

	if (CD4049(CD4049((vg2 - cv4)/5.0))>2.4/5.0) /* TTL - Level */
		grid_sig = 0;
	else
		grid_sig = 1;

	/* stars */
	pixelcnt += HTOTAL;
	if (pixelcnt > period2 )
	{
		star_ff = !star_ff;
		pixelcnt = pixelcnt - period2;
	}
}

/* re checked the ported code is good */
void radarscp_scanline(int scanline)
{
	const UINT8 *table = memory_region(REGION_GFX3);
	int 		table_len = memory_region_length(REGION_GFX3);
	int 			x,y,offset;
	UINT16 			*pixel;

	y = scanline;
	radarscp_step(y);
	if (y <= Machine->visible_area.min_y  || y > Machine->visible_area.max_y )
		counter = 0;
	/*change this temp to match the driver */
		offset = (~flip_screen ^ rflip_sig) ? 0x400 : 0x000;
	x = 0;
	while (x < Machine->drv->screen_width)
	{
		pixel = BITMAP_ADDR16(bg_bits, y, x);
		if ((counter < table_len) && (x == 4 * (table[counter|offset] & 0x7f)))
		{
			if ( star_ff && (table[counter|offset] & 0x80) )    /* star */
				*pixel = RADARSCP_STAR_COL;
			else if (grid_sig && !(table[counter|offset] & 0x80))    /* radar */
				*pixel = RADARSCP_GRID_COL_OFFSET+grid_col;
			else
				*pixel = RADARSCP_BCK_COL_OFFSET + blue_level;
			counter++;
		}
		else
			*pixel = RADARSCP_BCK_COL_OFFSET + blue_level;
		x++;
	}
	while ((counter < table_len) && (x < 4 * (table[counter|offset] & 0x7f)))
		counter++;
}

void radarscp_draw_background(struct mame_bitmap *bitmap, const struct rectangle *cliprect)
{
	/*UINT8     *htable = nullptr;*/
	UINT8 draw_ok;
	int y,x;
	UINT16 *pixel;
/*	if (m_hardware_type == HARDWARE_TRS01)
		htable = m_gfx4; */

	y = cliprect->min_y;
	while (y <= cliprect->max_y)
	{
		x = cliprect->min_x;
		while (x <= cliprect->max_x)
		{
			pixel = BITMAP_ADDR16(bitmap, y, x );
			draw_ok = !(*pixel & 0x01) && !(*pixel & 0x02);
/*			if (m_hardware_type == HARDWARE_TRS01)   Check again from schematics 
				draw_ok = draw_ok  && !((htable[ (!rflip_sig<<7) | (x>>2)] >>2) & 0x01);*/
			if (draw_ok)
				*pixel = *(BITMAP_ADDR16(bg_bits, y, x));
			x++;
		}
		y++;
	}
}

static void draw_grid(struct mame_bitmap *bitmap)
{
	const UINT8 *table = memory_region(REGION_GFX3);
	int x,y;

	counter = flip_screen ? 0x000 : 0x400;

	x = Machine->visible_area.min_x;
	y = Machine->visible_area.min_y;

	while (y <= Machine->visible_area.max_y)
	{
		x = 4 * (table[counter] & 0x7f);
		if (x >= Machine->visible_area.min_x &&
				x <= Machine->visible_area.max_x)
		{
			if (table[counter] & 0x80)	/* star */
			{
				if (rand() & 1)	/* noise coming from sound board */
					plot_pixel(bitmap,x,y,Machine->pens[RADARSCP_STAR_COL]);

			}
			else if (grid_on)			/* radar */
				plot_pixel(bitmap,x,y,Machine->pens[RADARSCP_GRID_COL_OFFSET+grid_col]);
		}

		counter++;

		if (x >= 4 * (table[counter] & 0x7f))
			y++;
	}
}

VIDEO_UPDATE( radarscp )
{
	int i;
	tilemap_draw(bitmap, &Machine->visible_area, bg_tilemap, 0, 0);
	draw_sprites(bitmap, 0x40, 1);
	
	/*for (i=VTOTAL; i > 0; i--) */
	for (i=0; i < VTOTAL; i++)
		radarscp_scanline(i);

	radarscp_draw_background(bitmap, cliprect);
}

VIDEO_UPDATE( dkong )
{
	tilemap_draw(bitmap, &Machine->visible_area, bg_tilemap, 0, 0);
	draw_sprites(bitmap, 0x40, 1);
  
  if (!strcmp(Machine->gamedrv->name, "shootgal"))
	{
		draw_crosshair(1, bitmap, readinputport(0) + 8, readinputport(1) + 3, cliprect );
	}
  
}

VIDEO_UPDATE( pestplce )
{
	int offs;

	tilemap_draw(bitmap, &Machine->visible_area, bg_tilemap, 0, 0);

	/* Draw the sprites. */
	for (offs = 0;offs < spriteram_size;offs += 4)
	{
		if (spriteram[offs])
		{
			drawgfx(bitmap,Machine->gfx[1],
					spriteram[offs + 2],
					(spriteram[offs + 1] & 0x0f) + 16 * palette_bank,
					spriteram[offs + 1] & 0x80,spriteram[offs + 1] & 0x40,
					spriteram[offs + 3] - 8,240 - spriteram[offs] + 8,
					&Machine->visible_area,TRANSPARENCY_PEN,0);
		}
	}
}

VIDEO_UPDATE( spclforc )
{
	tilemap_draw(bitmap, &Machine->visible_area, bg_tilemap, 0, 0);

	/* it uses spriteram[offs + 2] & 0x10 for sprite bank */
	draw_sprites(bitmap, 0x10, 3);
}
