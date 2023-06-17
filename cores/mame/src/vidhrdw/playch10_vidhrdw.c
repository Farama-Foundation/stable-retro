#include "driver.h"
#include "vidhrdw/generic.h"
#include "vidhrdw/ppu2c03b.h"

/* from machine */
extern int pc10_sdcs;			/* ShareD Chip Select */
extern int pc10_dispmask;		/* Display Mask */
extern int pc10_gun_controller;	/* wether we need to draw a crosshair or not */
extern int pc10_int_detect;

static struct tilemap *bg_tilemap;

WRITE_HANDLER( playch10_videoram_w )
{
	if (pc10_sdcs)
	{
		if (videoram[offset] != data)
		{
			videoram[offset] = data;
			tilemap_mark_tile_dirty(bg_tilemap, offset / 2);
		}
	}
}

PALETTE_INIT( playch10 )
{
	int i;

	for (i = 0; i < 256; i++)
	{
		int bit0, bit1, bit2, bit3, r, g, b;

		/* red component */

		bit0 = ~(color_prom[0] >> 0) & 0x01;
		bit1 = ~(color_prom[0] >> 1) & 0x01;
		bit2 = ~(color_prom[0] >> 2) & 0x01;
		bit3 = ~(color_prom[0] >> 3) & 0x01;

		r = 0x0e * bit0 + 0x1f * bit1 + 0x43 * bit2 + 0x8f * bit3;

		/* green component */
		bit0 = ~(color_prom[256] >> 0) & 0x01;
		bit1 = ~(color_prom[256] >> 1) & 0x01;
		bit2 = ~(color_prom[256] >> 2) & 0x01;
		bit3 = ~(color_prom[256] >> 3) & 0x01;

		g = 0x0e * bit0 + 0x1f * bit1 + 0x43 * bit2 + 0x8f * bit3;

		/* blue component */

		bit0 = ~(color_prom[2*256] >> 0) & 0x01;
		bit1 = ~(color_prom[2*256] >> 1) & 0x01;
		bit2 = ~(color_prom[2*256] >> 2) & 0x01;
		bit3 = ~(color_prom[2*256] >> 3) & 0x01;

		b = 0x0e * bit0 + 0x1f * bit1 + 0x43 * bit2 + 0x8f * bit3;

		palette_set_color(i,r,g,b);

		color_prom++;
	}

	ppu2c03b_init_palette( 256 );
}

static void ppu_irq( int num, int *ppu_regs )
{
	cpu_set_nmi_line( 1, PULSE_LINE );
	pc10_int_detect = 1;
}

/* our ppu interface											*/
/* things like mirroring and wether to use vrom or vram			*/
/* can be set by calling 'ppu2c03b_override_hardware_options'	*/

static struct ppu2c03b_interface ppu_interface =
{
	1,						/* num */
	{ REGION_GFX2 },		/* vrom gfx region */
	{ 1 },					/* gfxlayout num */
	{ 256 },				/* color base */
	{ PPU_MIRROR_NONE },	/* mirroring */
	{ ppu_irq }				/* irq */
};

static void get_bg_tile_info(int tile_index)
{
	int offs = tile_index * 2;
	int code = videoram[offs] + ((videoram[offs + 1] & 0x07) << 8);
	int color = (videoram[offs + 1] >> 3) & 0x1f;

	SET_TILE_INFO(0, code, color, 0)
}

VIDEO_START( playch10 )
{
	bg_tilemap = tilemap_create(get_bg_tile_info, tilemap_scan_rows, 
		TILEMAP_OPAQUE, 8, 8, 32, 32);

	if ( !bg_tilemap )
		return 1;

	if ( ppu2c03b_init( &ppu_interface ) )
		return 1;

	return 0;
}

/***************************************************************************

  Display refresh

***************************************************************************/

VIDEO_UPDATE( playch10 )
{
	struct rectangle top_monitor = Machine->visible_area;
	struct rectangle bottom_monitor = Machine->visible_area;

	top_monitor.max_y = ( top_monitor.max_y - top_monitor.min_y ) / 2;
	bottom_monitor.min_y = ( bottom_monitor.max_y - bottom_monitor.min_y ) / 2;

	/* On Playchoice 10 single monitor, this bit toggles	*/
	/* between PPU and BIOS display.						*/
	/* We support the multi-monitor layout. In this case,	*/
	/* if the bit is not set, then we should display		*/
	/* the PPU portion.										*/

	if ( !pc10_dispmask )
	{
		/* render the ppu */
		ppu2c03b_render( 0, bitmap, 0, 0, 0, 30*8 );

		/* if this is a gun game, draw a simple crosshair */
		if ( pc10_gun_controller )
		{
			int x_center = readinputport( 5 );
			int y_center = readinputport( 6 ) + 30*8;

			draw_crosshair(1, bitmap, x_center, y_center, &bottom_monitor);
		}
	}

	/* When the bios is accessing vram, the video circuitry cant access it */

	if ( !pc10_sdcs )
	{
		tilemap_draw(bitmap, &top_monitor, bg_tilemap, 0, 0);
	}
}
