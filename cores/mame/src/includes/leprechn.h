/*************************************************************************

	Leprechaun/Pot of Gold

*************************************************************************/

/*----------- defined in machine/leprechn.c -----------*/

DRIVER_INIT( leprechn );
READ_HANDLER( leprechn_sh_0805_r );


/*----------- defined in vidhrdw/leprechn.c -----------*/

PALETTE_INIT( leprechn );
VIDEO_START( leprechn );

WRITE_HANDLER( leprechn_graphics_command_w );
READ_HANDLER( leprechn_videoram_r );
WRITE_HANDLER( leprechn_videoram_w );
