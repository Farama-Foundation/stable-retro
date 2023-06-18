/***************************************************************************

	The Game Room Lethal Justice hardware

***************************************************************************/

#include "driver.h"
#include "cpu/tms34010/tms34010.h"
#include "lethalj.h"


#define BLITTER_SOURCE_WIDTH		1024
#define BLITTER_DEST_WIDTH			512
#define BLITTER_DEST_HEIGHT			512


static data16_t blitter_data[8];

static UINT16 *screenram;
static UINT16 *blitter_base;
static int blitter_rows;

static UINT16 gunx, guny;
static UINT8 blank_palette;



/*************************************
 *
 *	Compute X/Y coordinates
 *
 *************************************/

static INLINE void get_crosshair_xy(int player, int *x, int *y)
{
	*x = ((readinputport(2 + player * 2) & 0xff) * Machine->drv->screen_width) / 255;
	*y = ((readinputport(3 + player * 2) & 0xff) * Machine->drv->screen_height) / 255;
}



/*************************************
 *
 *	Gun input handling
 *
 *************************************/

READ16_HANDLER( lethalj_gun_r )
{
	data16_t result = 0;
	int beamx, beamy;
	
	switch (offset)
	{
		case 4:
		case 5:
			/* latch the crosshair position */
			get_crosshair_xy(offset - 4, &beamx, &beamy);
			gunx = beamx;
			guny = beamy;
			blank_palette = 1;
			break;
		
		case 6:
			result = gunx/2;
			break;
		
		case 7:
			result = guny + 4;
			break;
	}
	log_cb(RETRO_LOG_DEBUG, LOGPRE "%08X:lethalj_gun_r(%d) = %04X\n", activecpu_get_pc(), offset, result);
	return result;
}



/*************************************
 *
 *	Palette init (standard 5-5-5 RGB)
 *
 *************************************/

PALETTE_INIT( lethalj )
{
	int i, r, g, b;
	for (r = i = 0; r < 32; r++)
		for (g = 0; g < 32; g++)
			for (b = 0; b < 32; b++, i++)
				palette_set_color(i, (r << 3) | (r >> 2), (g << 3) | (g >> 2), (b << 3) | (b >> 2));
}



/*************************************
 *
 *	video startup
 *
 *************************************/

VIDEO_START( lethalj )
{
	/* allocate video RAM for screen */
	screenram = auto_malloc(BLITTER_DEST_WIDTH * BLITTER_DEST_HEIGHT * sizeof(screenram[0]));
	if (!screenram)
		return 1;

	/* predetermine blitter info */
	blitter_base = (UINT16 *)memory_region(REGION_GFX1);
	blitter_rows = memory_region_length(REGION_GFX1) / (2*BLITTER_SOURCE_WIDTH);
	return 0;
}



/*************************************
 *
 *	Memory maps
 *
 *************************************/

static void gen_ext1_int(int param)
{
	cpu_set_irq_line(0, 0, ASSERT_LINE);
}



static void do_blit(void)
{
	int dsty = (INT16)blitter_data[1];
	int srcx = (UINT16)blitter_data[2];
	int srcy = (UINT16)blitter_data[3];
	int width = (UINT16)blitter_data[5];
	int dstx = (INT16)blitter_data[6];
	int height = (UINT16)blitter_data[7];
	int y;

/*	logerror("blitter data = %04X %04X %04X %04X %04X %04X %04X %04X\n",
			blitter_data[0], blitter_data[1], blitter_data[2], blitter_data[3],
			blitter_data[4], blitter_data[5], blitter_data[6], blitter_data[7]);*/

	/* loop over Y coordinates */
	for (y = 0; y <= height; y++, srcy++, dsty++)
	{
		UINT16 *source = blitter_base + srcy * BLITTER_SOURCE_WIDTH;
		UINT16 *dest = screenram + dsty * BLITTER_DEST_WIDTH;
		
		/* clip in Y */
		if (dsty >= 0 && dsty < BLITTER_DEST_HEIGHT)
		{
			int sx = srcx;
			int dx = dstx;
			int x;
			
			/* loop over X coordinates */
			for (x = 0; x <= width; x++, sx++, dx++)
				if (dx >= 0 && dx < BLITTER_DEST_WIDTH)
				{
					int pix = source[sx % BLITTER_SOURCE_WIDTH];
					if (pix)
						dest[dx] = pix;
				}
		}
	}
}



WRITE16_HANDLER( lethalj_blitter_w )
{
	/* combine the data */
	COMBINE_DATA(&blitter_data[offset]);

	/* blit on a write to offset 7, and signal an IRQ */
	if (offset == 7)
	{
		do_blit();
		timer_set(TIME_IN_USEC(10), 0, gen_ext1_int);
	}

	/* clear the IRQ on offset 0 */
	else if (offset == 0)
		cpu_set_irq_line(0, 0, CLEAR_LINE);
}



/*************************************
 *
 *	video update
 *
 *************************************/

VIDEO_UPDATE( lethalj )
{
	int beamx, beamy;
	
	/* blank palette: fill with white */
	if (blank_palette)
		fillbitmap(bitmap, 0x7fff, cliprect);
	
	/* otherwise, blit from screenram */
	else
	{
		int x, y;
		for (y = cliprect->min_y; y <= cliprect->max_y; y++)
		{
			UINT16 *source = screenram + y * BLITTER_DEST_WIDTH + cliprect->min_x;
			UINT16 *dest = (UINT16 *)bitmap->base + y * bitmap->rowpixels + cliprect->min_x;
			for (x = cliprect->min_x; x <= cliprect->max_x; x++)
				*dest++ = *source++ & 0x7fff;
		}
	}
	
	/* draw player 1's crosshair */
	get_crosshair_xy(0, &beamx, &beamy);
	draw_crosshair(1, bitmap, beamx, beamy, cliprect);

	/* draw player 2's crosshair */
	get_crosshair_xy(1, &beamx, &beamy);
	draw_crosshair(2, bitmap, beamx, beamy, cliprect);

	if (cliprect->max_y == Machine->visible_area.max_y)
		blank_palette = 0;
}
