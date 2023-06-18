/***************************************************************************

	Videa Gridlee hardware

    driver by Aaron Giles

***************************************************************************/

#include "driver.h"


/*----------- defined in sndhrdw/gridlee.c -----------*/

WRITE_HANDLER( gridlee_sound_w );
int gridlee_sh_start(const struct MachineSound *msound);


/*----------- defined in vidhrdw/gridlee.c -----------*/

/* video driver data & functions */
extern UINT8 gridlee_cocktail_flip;

PALETTE_INIT( gridlee );
VIDEO_START( gridlee );
VIDEO_UPDATE( gridlee );

WRITE_HANDLER( gridlee_cocktail_flip_w );
WRITE_HANDLER( gridlee_videoram_w );
WRITE_HANDLER( gridlee_palette_select_w );
