#include "driver.h"
#include "vidhrdw/ppu2c03b.h"

/* from machine */
extern int vsnes_gun_controller;


PALETTE_INIT( vsnes )
{
	ppu2c03b_init_palette( 0 );
}

PALETTE_INIT( vsdual )
{
	ppu2c03b_init_palette( 0 );
	ppu2c03b_init_palette( 64 );
}

static void ppu_irq( int num, int *ppu_regs )
{
	cpu_set_nmi_line( num, PULSE_LINE );
}

/* our ppu interface											*/
static struct ppu2c03b_interface ppu_interface =
{
	1,						/* num */
	{ REGION_GFX1 },		/* vrom gfx region */
	{ 0 },					/* gfxlayout num */
	{ 0 },					/* color base */
	{ PPU_MIRROR_NONE },	/* mirroring */
	{ ppu_irq }				/* irq */
};

/* our ppu interface for dual games								*/
static struct ppu2c03b_interface ppu_dual_interface =
{
	2,										/* num */
	{ REGION_GFX1, REGION_GFX2 },			/* vrom gfx region */
	{ 0, 1 },								/* gfxlayout num */
	{ 0, 64 },								/* color base */
	{ PPU_MIRROR_NONE, PPU_MIRROR_NONE },	/* mirroring */
	{ ppu_irq, ppu_irq }					/* irq */
};

VIDEO_START( vsnes )
{
	return ppu2c03b_init( &ppu_interface );
}

VIDEO_START( vsdual )
{
	return ppu2c03b_init( &ppu_dual_interface );
}

/***************************************************************************

  Display refresh

***************************************************************************/
VIDEO_UPDATE( vsnes )
{
	/* render the ppu */
	ppu2c03b_render( 0, bitmap, 0, 0, 0, 0 );

		/* if this is a gun game, draw a simple crosshair */
		if ( vsnes_gun_controller )
		{
			int x_center = readinputport( 4 );
			int y_center = readinputport( 5 );

			draw_crosshair(1, bitmap,x_center,y_center,&Machine->visible_area);

		}

	}


VIDEO_UPDATE( vsdual )
{
	/* render the ppu's */
	ppu2c03b_render( 0, bitmap, 0, 0, 0, 0 );
	ppu2c03b_render( 1, bitmap, 0, 0, 32*8, 0 );
}
