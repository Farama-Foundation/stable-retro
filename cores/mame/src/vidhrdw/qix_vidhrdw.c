/***************************************************************************

	Taito Qix hardware

	driver by John Butler, Ed Mueller, Aaron Giles

***************************************************************************/

#include "driver.h"
#include "qix.h"
#include "vidhrdw/generic.h"


/* Constants */
#define SCANLINE_INCREMENT	4


/* Globals */
UINT8 *qix_videoaddress;
UINT8 qix_cocktail_flip;


/* Local variables */
static UINT8 vram_mask;
static UINT8 qix_palettebank;



/*************************************
 *
 *	Video startup
 *
 *************************************/

VIDEO_START( qix )
{
	/* allocate memory for the full video RAM */
	videoram = auto_malloc(256 * 256);
	if (!videoram)
		return 1;

	/* initialize the mask for games that don't use it */
	vram_mask = 0xff;
	return 0;
}



/*************************************
 *
 *	Scanline caching
 *
 *************************************/

void qix_scanline_callback(int scanline)
{
	/* force a partial update */
	force_partial_update(scanline - 1);

	/* set a timer for the next increment */
	scanline += SCANLINE_INCREMENT;
	if (scanline > 256)
		scanline = SCANLINE_INCREMENT;
	timer_set(cpu_getscanlinetime(scanline), scanline, qix_scanline_callback);
}



/*************************************
 *
 *	Current scanline read
 *
 *************************************/

READ_HANDLER( qix_scanline_r )
{
	int scanline = cpu_getscanline();
	return (scanline <= 0xff) ? scanline : 0;
}



/*************************************
 *
 *	Video RAM mask
 *
 *************************************/

WRITE_HANDLER( slither_vram_mask_w )
{
	/* Slither appears to extend the basic hardware by providing */
	/* a mask register which controls which data bits get written */
	/* to video RAM */
	vram_mask = data;
}



/*************************************
 *
 *	Direct video RAM read/write
 *
 *	The screen is 256x256 with eight
 *	bit pixels (64K).  The screen is
 *	divided into two halves each half
 *	mapped by the video CPU at
 *	$0000-$7FFF.  The high order bit
 *	of the address latch at $9402
 *	specifies which half of the screen
 *	is being accessed.
 *
 *************************************/

READ_HANDLER( qix_videoram_r )
{
	/* add in the upper bit of the address latch */
	offset += (qix_videoaddress[0] & 0x80) << 8;
	return videoram[offset];
}


WRITE_HANDLER( qix_videoram_w )
{
	/* add in the upper bit of the address latch */
	offset += (qix_videoaddress[0] & 0x80) << 8;

	/* blend the data */
	videoram[offset] = (videoram[offset] & ~vram_mask) | (data & vram_mask);
}



/*************************************
 *
 *	Latched video RAM read/write
 *
 *	The address latch works as follows.
 *	When the video CPU accesses $9400,
 *	the screen address is computed by
 *	using the values at $9402 (high
 *	byte) and $9403 (low byte) to get
 *	a value between $0000-$FFFF.  The
 *	value at that location is either
 *	returned or written.
 *
 *************************************/

READ_HANDLER( qix_addresslatch_r )
{
	/* compute the value at the address latch */
	offset = (qix_videoaddress[0] << 8) | qix_videoaddress[1];
	return videoram[offset];
}



WRITE_HANDLER( qix_addresslatch_w )
{
	/* compute the value at the address latch */
	offset = (qix_videoaddress[0] << 8) | qix_videoaddress[1];

	/* blend the data */
	videoram[offset] = (videoram[offset] & ~vram_mask) | (data & vram_mask);
}



/*************************************
 *
 *	Palette RAM
 *
 *************************************/

WRITE_HANDLER( qix_paletteram_w )
{
	/* this conversion table should be about right. It gives a reasonable */
	/* gray scale in the test screen, and the red, green and blue squares */
	/* in the same screen are barely visible, as the manual requires. */
	static UINT8 table[16] =
	{
		0x00,	/* value = 0, intensity = 0 */
		0x12,	/* value = 0, intensity = 1 */
		0x24,	/* value = 0, intensity = 2 */
		0x49,	/* value = 0, intensity = 3 */
		0x12,	/* value = 1, intensity = 0 */
		0x24,	/* value = 1, intensity = 1 */
		0x49,	/* value = 1, intensity = 2 */
		0x92,	/* value = 1, intensity = 3 */
		0x5b,	/* value = 2, intensity = 0 */
		0x6d,	/* value = 2, intensity = 1 */
		0x92,	/* value = 2, intensity = 2 */
		0xdb,	/* value = 2, intensity = 3 */
		0x7f,	/* value = 3, intensity = 0 */
		0x91,	/* value = 3, intensity = 1 */
		0xb6,	/* value = 3, intensity = 2 */
		0xff	/* value = 3, intensity = 3 */
	};
	int bits, intensity, red, green, blue;

	/* set the palette RAM value */
	paletteram[offset] = data;

	/* compute R, G, B from the table */
	intensity = (data >> 0) & 0x03;
	bits = (data >> 6) & 0x03;
	red = table[(bits << 2) | intensity];
	bits = (data >> 4) & 0x03;
	green = table[(bits << 2) | intensity];
	bits = (data >> 2) & 0x03;
	blue = table[(bits << 2) | intensity];

	/* update the palette */
	palette_set_color(offset, red, green, blue);
}


WRITE_HANDLER( qix_palettebank_w )
{
	/* set the bank value */
	if (qix_palettebank != (data & 3))
	{
		force_partial_update(cpu_getscanline() - 1);
		qix_palettebank = data & 3;
	}

	/* LEDs are in the upper 6 bits */
}



/*************************************
 *
 *	Core video refresh
 *
 *************************************/

VIDEO_UPDATE( qix )
{
	pen_t *pens = &Machine->pens[qix_palettebank * 256];
	int y;

	/* draw the bitmap */
	for (y = cliprect->min_y; y <= cliprect->max_y; y++)
		draw_scanline8(bitmap, 0, y, 256, &videoram[y * 256], pens, -1);
}
