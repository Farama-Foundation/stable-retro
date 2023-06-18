/*************************************************************************

	Atari Basketball hardware

*************************************************************************/

/*----------- defined in machine/bsktball.c -----------*/

WRITE_HANDLER( bsktball_nmion_w );
INTERRUPT_GEN( bsktball_interrupt );
WRITE_HANDLER( bsktball_ld1_w );
WRITE_HANDLER( bsktball_ld2_w );
READ_HANDLER( bsktball_in0_r );
WRITE_HANDLER( bsktball_led1_w );
WRITE_HANDLER( bsktball_led2_w );
WRITE_HANDLER( bsktball_bounce_w );
WRITE_HANDLER( bsktball_note_w );
WRITE_HANDLER( bsktball_noise_reset_w );


/*----------- defined in vidhrdw/bsktball.c -----------*/

extern unsigned char *bsktball_motion;
VIDEO_UPDATE( bsktball );
