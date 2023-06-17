/*************************************************************************

	Atari Super Breakout hardware

*************************************************************************/

/*----------- defined in machine/sbrkout.c -----------*/

WRITE_HANDLER( sbrkout_serve_led_w );
WRITE_HANDLER( sbrkout_start_1_led_w );
WRITE_HANDLER( sbrkout_start_2_led_w );
READ_HANDLER( sbrkout_read_DIPs_r );
INTERRUPT_GEN( sbrkout_interrupt );
READ_HANDLER( sbrkout_select1_r );
READ_HANDLER( sbrkout_select2_r );


/*----------- defined in vidhrdw/sbrkout.c -----------*/

extern unsigned char *sbrkout_horiz_ram;
extern unsigned char *sbrkout_vert_ram;

VIDEO_UPDATE( sbrkout );
