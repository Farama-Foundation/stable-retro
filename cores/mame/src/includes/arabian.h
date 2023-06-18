/***************************************************************************

	Sun Electronics Arabian hardware

	driver by Dan Boris

***************************************************************************/

#include "driver.h"


/*----------- defined in vidhrdw/arabian.c -----------*/

extern UINT8 arabian_video_control;
extern UINT8 arabian_flip_screen;

PALETTE_INIT( arabian );
VIDEO_START( arabian );
VIDEO_UPDATE( arabian );

WRITE_HANDLER( arabian_blitter_w );
WRITE_HANDLER( arabian_videoram_w );
