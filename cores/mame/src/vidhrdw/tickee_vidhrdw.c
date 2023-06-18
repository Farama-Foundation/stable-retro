/***************************************************************************

	Raster Elite Tickee Tickats hardware

***************************************************************************/

#include "driver.h"
#include "cpu/tms34010/tms34010.h"
#include "tickee.h"


data16_t *tickee_vram;


/* local variables */
static void *setup_gun_timer;



/*************************************
 *
 *	Compute X/Y coordinates
 *
 *************************************/

static INLINE void get_crosshair_xy(int player, int *x, int *y)
{
	*x = ((readinputport(4 + player * 2) & 0xff) * Machine->drv->screen_width) >> 8;
	*y = ((readinputport(5 + player * 2) & 0xff) * Machine->drv->screen_height) >> 8;
}



/*************************************
 *
 *	Light gun interrupts
 *
 *************************************/

static void trigger_gun_interrupt(int which)
{
	/* fire the IRQ at the correct moment */
	cpu_set_irq_line(0, which, ASSERT_LINE);
}


static void clear_gun_interrupt(int which)
{
	/* clear the IRQ on the next scanline? */
	cpu_set_irq_line(0, which, CLEAR_LINE);
}


static void setup_gun_interrupts(int param)
{
	int beamx, beamy;
	double time;

	/* set a timer to do this again next frame */
	timer_adjust(setup_gun_timer, cpu_getscanlinetime(0), 0, 0);

	/* only do work if the palette is flashed */
	if (!tickee_control[2])
		return;

	/* generate interrupts for player 1's gun */
	get_crosshair_xy(0, &beamx, &beamy);
	time = cpu_getscanlinetime(beamy);
	time += cpu_getscanlineperiod() * (double)(beamx + 175) / (double)450;
	timer_set(time, 0, trigger_gun_interrupt);
	timer_set(time + cpu_getscanlineperiod(), 0, clear_gun_interrupt);

	/* generate interrupts for player 2's gun */
	get_crosshair_xy(1, &beamx, &beamy);
	time = cpu_getscanlinetime(beamy);
	time += cpu_getscanlineperiod() * (double)(beamx + 175) / (double)450;
	timer_set(time, 1, trigger_gun_interrupt);
	timer_set(time + cpu_getscanlineperiod(), 1, clear_gun_interrupt);
}



/*************************************
 *
 *	Video startup
 *
 *************************************/

VIDEO_START( tickee )
{
	/* start a timer going on the first scanline of every frame */
	setup_gun_timer = timer_alloc(setup_gun_interrupts);
	timer_adjust(setup_gun_timer, cpu_getscanlinetime(0), 0, 0);

	return 0;
}



/*************************************
 *
 *	Video update
 *
 *************************************/

VIDEO_UPDATE( tickee )
{
	int v, h, width, xoffs, beamx, beamy;
	UINT8 *base1 = (UINT8 *)tickee_vram;
	pen_t pen_lookup[256];
	UINT32 offset;

	/* fill out the pen array based on the palette bank */
	for (h = 0; h < 256; h++)
		pen_lookup[h] = tickee_control[2] ? 255 : h;

	/* determine the base of the videoram */
	offset = (~tms34010_get_DPYSTRT(0) & 0xfff0) << 5;
	offset += TOBYTE(0x1000) * (cliprect->min_y - Machine->visible_area.min_y);

	/* determine how many pixels to copy */
	xoffs = cliprect->min_x;
	width = cliprect->max_x - xoffs + 1;
	offset += xoffs;

	/* loop over rows */
	for (v = cliprect->min_y; v <= cliprect->max_y; v++)
	{
		UINT8 scanline[512];

		/* extract the scanline to account for endianness */
		for (h = 0; h < width; h++)
			scanline[h] = base1[BYTE_XOR_LE(offset + h) & 0x7ffff];

		/* draw it and advance */
		draw_scanline8(bitmap, xoffs, v, width, scanline, pen_lookup, -1);
		offset += TOBYTE(0x1000);
	}

	/* draw player 1's crosshair */
	get_crosshair_xy(0, &beamx, &beamy);
	draw_crosshair(1, bitmap, beamx, beamy, cliprect);

	/* draw player 2's crosshair */
	get_crosshair_xy(1, &beamx, &beamy);
	draw_crosshair(2, bitmap, beamx, beamy, cliprect);
}
