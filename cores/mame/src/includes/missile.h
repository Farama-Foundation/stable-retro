/*************************************************************************

	Atari Missile Command hardware

*************************************************************************/

/*----------- defined in machine/missile.c -----------*/

MACHINE_INIT( missile );
READ_HANDLER( missile_r );
WRITE_HANDLER( missile_w );


/*----------- defined in vidhrdw/missile.c -----------*/

extern unsigned char *missile_video2ram;

VIDEO_START( missile );
VIDEO_UPDATE( missile );

WRITE_HANDLER( missile_video_3rd_bit_w );
WRITE_HANDLER( missile_video2_w );

READ_HANDLER( missile_video_r );
WRITE_HANDLER( missile_video_w );
WRITE_HANDLER( missile_video_mult_w );
WRITE_HANDLER( missile_palette_w );
