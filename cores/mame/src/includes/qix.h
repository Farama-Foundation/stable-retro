/***************************************************************************

	Taito Qix hardware

	driver by John Butler, Ed Mueller, Aaron Giles

***************************************************************************/

#include "driver.h"


/*----------- defined in machine/qix.c -----------*/

extern UINT8 *qix_sharedram;
extern UINT8 *qix_68705_port_out;
extern UINT8 *qix_68705_ddr;

MACHINE_INIT( qix );
MACHINE_INIT( qixmcu );
MACHINE_INIT( slither );

READ_HANDLER( qix_sharedram_r );
WRITE_HANDLER( qix_sharedram_w );

WRITE_HANDLER( zoo_bankswitch_w );

READ_HANDLER( qix_data_firq_r );
READ_HANDLER( qix_data_firq_ack_r );
WRITE_HANDLER( qix_data_firq_w );
WRITE_HANDLER( qix_data_firq_ack_w );

READ_HANDLER( qix_video_firq_r );
READ_HANDLER( qix_video_firq_ack_r );
WRITE_HANDLER( qix_video_firq_w );
WRITE_HANDLER( qix_video_firq_ack_w );

READ_HANDLER( qix_68705_portA_r );
READ_HANDLER( qix_68705_portB_r );
READ_HANDLER( qix_68705_portC_r );
WRITE_HANDLER( qix_68705_portA_w );
WRITE_HANDLER( qix_68705_portB_w );
WRITE_HANDLER( qix_68705_portC_w );

WRITE_HANDLER( qix_pia_0_w );
WRITE_HANDLER( zookeep_pia_0_w );
WRITE_HANDLER( zookeep_pia_2_w );


/*----------- defined in vidhrdw/qix.c -----------*/

extern UINT8 *qix_videoaddress;
extern UINT8 qix_cocktail_flip;

VIDEO_START( qix );
VIDEO_UPDATE( qix );

INTERRUPT_GEN( qix_vblank_start );
void qix_scanline_callback(int scanline);

READ_HANDLER( qix_scanline_r );
READ_HANDLER( qix_videoram_r );
WRITE_HANDLER( qix_videoram_w );
READ_HANDLER( qix_addresslatch_r );
WRITE_HANDLER( qix_addresslatch_w );
WRITE_HANDLER( slither_vram_mask_w );
WRITE_HANDLER( qix_paletteram_w );
WRITE_HANDLER( qix_palettebank_w );

READ_HANDLER( qix_data_io_r );
READ_HANDLER( qix_sound_io_r );
WRITE_HANDLER( qix_data_io_w );
WRITE_HANDLER( qix_sound_io_w );
