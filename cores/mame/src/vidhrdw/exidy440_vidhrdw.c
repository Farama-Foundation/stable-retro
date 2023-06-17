/***************************************************************************

	Exidy 440 video system

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "exidy440.h"

#define SPRITE_COUNT		40
#define SPRITERAM_SIZE		(SPRITE_COUNT * 4)
#define MAX_SCANLINE		240


/* globals */
UINT8 *exidy440_scanline;
UINT8 *exidy440_imageram;
UINT8 exidy440_firq_vblank;
UINT8 exidy440_firq_beam;
UINT8 topsecex_yscroll;

/* local allocated storage */
static UINT8 exidy440_latched_x;
static UINT8 *local_videoram;
static UINT8 *local_paletteram;
static UINT8 *scanline_dirty;

/* local variables */
static UINT8 firq_enable;
static UINT8 firq_select;
static UINT8 palettebank_io;
static UINT8 palettebank_vis;
static UINT8 topsecex_last_yscroll;

/* function prototypes */
void exidy440_update_firq(void);



/*************************************
 *
 *	Initialize the video system
 *
 *************************************/

VIDEO_START( exidy440 )
{
	/* reset the system */
	firq_enable = 0;
	firq_select = 0;
	palettebank_io = 0;
	palettebank_vis = 0;
	exidy440_firq_vblank = 0;
	exidy440_firq_beam = 0;

	/* reset Top Secret variables */
	topsecex_yscroll = 0;
	topsecex_last_yscroll = 0;

	/* allocate a bitmap */
	tmpbitmap = auto_bitmap_alloc(Machine->drv->screen_width, Machine->drv->screen_height);
	if (!tmpbitmap)
		return 1;

	/* allocate a buffer for VRAM */
	local_videoram = auto_malloc(256 * 256 * 2);
	if (!local_videoram)
		return 1;

	/* clear it */
	memset(local_videoram, 0, 256 * 256 * 2);

	/* allocate a buffer for palette RAM */
	local_paletteram = auto_malloc(512 * 2);
	if (!local_paletteram)
		return 1;

	/* clear it */
	memset(local_paletteram, 0, 512 * 2);

	/* allocate a scanline dirty array */
	scanline_dirty = auto_malloc(256);
	if (!scanline_dirty)
		return 1;

	/* mark everything dirty to start */
	memset(scanline_dirty, 1, 256);
	return 0;
}



/*************************************
 *
 *	Video RAM read/write
 *
 *************************************/

READ_HANDLER( exidy440_videoram_r )
{
	UINT8 *base = &local_videoram[(*exidy440_scanline * 256 + offset) * 2];

	/* combine the two pixel values into one byte */
	return (base[0] << 4) | base[1];
}


WRITE_HANDLER( exidy440_videoram_w )
{
	UINT8 *base = &local_videoram[(*exidy440_scanline * 256 + offset) * 2];

	/* expand the two pixel values into two bytes */
	base[0] = (data >> 4) & 15;
	base[1] = data & 15;

	/* mark the scanline dirty */
	scanline_dirty[*exidy440_scanline] = 1;
}



/*************************************
 *
 *	Palette RAM read/write
 *
 *************************************/

READ_HANDLER( exidy440_paletteram_r )
{
	return local_paletteram[palettebank_io * 512 + offset];
}


WRITE_HANDLER( exidy440_paletteram_w )
{
	/* update palette ram in the I/O bank */
	local_paletteram[palettebank_io * 512 + offset] = data;

	/* if we're modifying the active palette, change the color immediately */
	if (palettebank_io == palettebank_vis)
	{
		int word;

		/* combine two bytes into a word */
		offset = palettebank_vis * 512 + (offset & 0x1fe);
		word = (local_paletteram[offset] << 8) + local_paletteram[offset + 1];

		/* extract the 5-5-5 RGB colors */
		palette_set_color(offset / 2, ((word >> 10) & 31) << 3, ((word >> 5) & 31) << 3, (word & 31) << 3);
	}
}



/*************************************
 *
 *	Horizontal/vertical positions
 *
 *************************************/

READ_HANDLER( exidy440_horizontal_pos_r )
{
	/* clear the FIRQ on a read here */
	exidy440_firq_beam = 0;
	exidy440_update_firq();

	/* according to the schems, this value is only latched on an FIRQ
	 * caused by collision or beam */
	return exidy440_latched_x;
}


READ_HANDLER( exidy440_vertical_pos_r )
{
	int result;

	/* according to the schems, this value is latched on any FIRQ
	 * caused by collision or beam, ORed together with CHRCLK,
	 * which probably goes off once per scanline; for now, we just
	 * always return the current scanline */
	result = cpu_getscanline();
	return (result < 255) ? result : 255;
}



/*************************************
 *
 *	Sprite RAM handler
 *
 *************************************/

WRITE_HANDLER( exidy440_spriteram_w )
{
	force_partial_update(cpu_getscanline());
	spriteram[offset] = data;
}



/*************************************
 *
 *	Interrupt and I/O control regs
 *
 *************************************/

WRITE_HANDLER( exidy440_control_w )
{
	int oldvis = palettebank_vis;

	/* extract the various bits */
	exidy440_bank = data >> 4;
	firq_enable = (data >> 3) & 1;
	firq_select = (data >> 2) & 1;
	palettebank_io = (data >> 1) & 1;
	palettebank_vis = data & 1;

	/* set the memory bank for the main CPU from the upper 4 bits */
	cpu_setbank(1, &memory_region(REGION_CPU1)[0x10000 + exidy440_bank * 0x4000]);

	/* update the FIRQ in case we enabled something */
	exidy440_update_firq();

	/* if we're swapping palettes, change all the colors */
	if (oldvis != palettebank_vis)
	{
		int i;

		/* pick colors from the visible bank */
		offset = palettebank_vis * 512;
		for (i = 0; i < 256; i++, offset += 2)
		{
			/* extract a word and the 5-5-5 RGB components */
			int word = (local_paletteram[offset] << 8) + local_paletteram[offset + 1];
			palette_set_color(i, ((word >> 10) & 31) << 3, ((word >> 5) & 31) << 3, (word & 31) << 3);
		}
	}
}


WRITE_HANDLER( exidy440_interrupt_clear_w )
{
	/* clear the VBLANK FIRQ on a write here */
	exidy440_firq_vblank = 0;
	exidy440_update_firq();
}



/*************************************
 *
 *	Interrupt generators
 *
 *************************************/

void exidy440_update_firq(void)
{
	if (exidy440_firq_vblank || (firq_enable && exidy440_firq_beam))
		cpu_set_irq_line(0, 1, ASSERT_LINE);
	else
		cpu_set_irq_line(0, 1, CLEAR_LINE);
}


INTERRUPT_GEN( exidy440_vblank_interrupt )
{
	/* set the FIRQ line on a VBLANK */
	exidy440_firq_vblank = 1;
	exidy440_update_firq();
}



/*************************************
 *
 *	IRQ callback handlers
 *
 *************************************/

void beam_firq_callback(int param)
{
	/* generate the interrupt, if we're selected */
	if (firq_select && firq_enable)
	{
		exidy440_firq_beam = 1;
		exidy440_update_firq();
	}

	/* round the x value to the nearest byte */
	param = (param + 1) / 2;

	/* latch the x value; this convolution comes from the read routine */
	exidy440_latched_x = (param + 3) ^ 2;
}


void collide_firq_callback(int param)
{
	/* generate the interrupt, if we're selected */
	if (!firq_select && firq_enable)
	{
		exidy440_firq_beam = 1;
		exidy440_update_firq();
	}

	/* round the x value to the nearest byte */
	param = (param + 1) / 2;

	/* latch the x value; this convolution comes from the read routine */
	exidy440_latched_x = (param + 3) ^ 2;
}



/*************************************
 *
 *	Determine the time when the beam
 *	will intersect a given pixel
 *
 *************************************/

double compute_pixel_time(int x, int y)
{
	/* assuming this is called at refresh time, compute how long until we
	 * hit the given x,y position */
	return cpu_getscanlinetime(y) + (cpu_getscanlineperiod() * (double)x * (1.0 / 320.0));
}



/*************************************
 *
 *	Sprite drawing
 *
 *************************************/

static void draw_sprites(struct mame_bitmap *bitmap, const struct rectangle *cliprect, int scroll_offset)
{
	int i;

	/* get a pointer to the palette to look for collision flags */
	UINT8 *palette = &local_paletteram[palettebank_vis * 512];

	/* start the count high for topsecret, which doesn't use collision flags */
	int count = exidy440_topsecret ? 128 : 0;

	/* draw the sprite images, checking for collisions along the way */
	UINT8 *sprite = spriteram + (SPRITE_COUNT - 1) * 4;

	for (i = 0; i < SPRITE_COUNT; i++, sprite -= 4)
	{
		int image = (~sprite[3] & 0x3f);
		int xoffs = (~((sprite[1] << 8) | sprite[2]) & 0x1ff);
		int yoffs = (~sprite[0] & 0xff) + 1;
		int x, y, sy;
		UINT8 *src;

		/* skip if out of range */
		if (yoffs < cliprect->min_y || yoffs >= cliprect->max_y + 16)
			continue;

		/* get a pointer to the source image */
		src = &exidy440_imageram[image * 128];

		/* account for large positive offsets meaning small negative values */
		if (xoffs >= 0x1ff - 16)
			xoffs -= 0x1ff;

		/* loop over y */
		sy = yoffs + scroll_offset;
		for (y = 0; y < 16; y++, yoffs--, sy--)
		{
			/* wrap at the top and bottom of the screen */
			if (sy >= 240)
				sy -= 240;
			else if (sy < 0)
				sy += 240;

			/* stop if we get before the current scanline */
			if (yoffs < cliprect->min_y)
				break;

			/* only draw scanlines that are in this cliprect */
			if (yoffs <= cliprect->max_y)
			{
				UINT8 *old = &local_videoram[sy * 512 + xoffs];
				int currx = xoffs;

				/* loop over x */
				for (x = 0; x < 8; x++, old += 2)
				{
					int ipixel = *src++;
					int left = ipixel & 0xf0;
					int right = (ipixel << 4) & 0xf0;
					int pen;

					/* left pixel */
					if (left && currx >= 0 && currx < 320)
					{
						/* combine with the background */
						pen = left | old[0];
						plot_pixel(bitmap, currx, yoffs, Machine->pens[pen]);

						/* check the collisions bit */
						if ((palette[2 * pen] & 0x80) && count++ < 128)
							timer_set(compute_pixel_time(currx, yoffs), currx, collide_firq_callback);
					}
					currx++;

					/* right pixel */
					if (right && currx >= 0 && currx < 320)
					{
						/* combine with the background */
						pen = right | old[1];
						plot_pixel(bitmap, currx, yoffs, Machine->pens[pen]);

						/* check the collisions bit */
						if ((palette[2 * pen] & 0x80) && count++ < 128)
							timer_set(compute_pixel_time(currx, yoffs), currx, collide_firq_callback);
					}
					currx++;
				}
			}
			else
				src += 8;
		}
	}
}



/*************************************
 *
 *	Core refresh routine
 *
 *************************************/

static void update_screen(struct mame_bitmap *bitmap, const struct rectangle *cliprect, int scroll_offset)
{
	int y, sy;
	int beamx, beamy;

	/* draw any dirty scanlines from the VRAM directly */
	sy = scroll_offset + cliprect->min_y;
	for (y = cliprect->min_y; y <= cliprect->max_y; y++, sy++)
	{
		/* wrap at the bottom of the screen */
		if (sy >= 240)
			sy -= 240;

		/* only redraw if dirty */
		if (scanline_dirty[sy])
		{
			draw_scanline8(tmpbitmap, 0, y, 320, &local_videoram[sy * 512], Machine->pens, -1);
			scanline_dirty[sy] = 0;
		}
	}
	copybitmap(bitmap,tmpbitmap,0,0,0,0,cliprect,TRANSPARENCY_NONE,0);

	/* draw the sprites */
	draw_sprites(bitmap, cliprect, scroll_offset);

	/* draw the crosshair (but not for topsecret) */
	if (!exidy440_topsecret)
	{
		beamx = ((input_port_4_r(0) & 0xff) * 320) >> 8;
		beamy = ((input_port_5_r(0) & 0xff) * 240) >> 8;

		draw_crosshair(1, bitmap, beamx, beamy, cliprect);
	}
}



/*************************************
 *
 *	Standard screen refresh callback
 *
 *************************************/

VIDEO_UPDATE( exidy440 )
{
	/* if we need a full refresh, mark all scanlines dirty */
	if (get_vh_global_attribute_changed())
		memset(scanline_dirty, 1, 256);

	/* if we're Top Secret, do our refresh here; others are done in the update function above */
	/* unless we're doing a full refresh (eg. when the driver is paused) */
	if (exidy440_topsecret)
	{
		/* if the scroll changed, mark everything dirty */
		if (topsecex_yscroll != topsecex_last_yscroll)
		{
			topsecex_last_yscroll = topsecex_yscroll;
			memset(scanline_dirty, 1, 256);
		}

		/* redraw the screen */
		update_screen(bitmap, cliprect, topsecex_yscroll);
	}
	else
	{
		/* redraw the screen */
		update_screen(bitmap, cliprect, 0);
	}
}



/*************************************
 *
 *	Standard screen refresh callback
 *
 *************************************/

VIDEO_EOF( exidy440 )
{
	/* generate an interrupt once/frame for the beam */
	if (!exidy440_topsecret)
	{
		double time, increment;
		int beamx, beamy;
		int i;

		beamx = ((input_port_4_r(0) & 0xff) * 320) >> 8;
		beamy = ((input_port_5_r(0) & 0xff) * 240) >> 8;

		/* The timing of this FIRQ is very important. The games look for an FIRQ
			and then wait about 650 cycles, clear the old FIRQ, and wait a
			very short period of time (~130 cycles) for another one to come in.
			From this, it appears that they are expecting to get beams over
			a 12 scanline period, and trying to pick roughly the middle one.
			This is how it is implemented. */
		increment = cpu_getscanlineperiod();
		time = compute_pixel_time(beamx, beamy) - increment * 6;
		for (i = 0; i <= 12; i++, time += increment)
			timer_set(time, beamx, beam_firq_callback);
	}
}
