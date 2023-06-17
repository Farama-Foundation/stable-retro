/*************************************************************************

	Centuri Aztarac hardware

*************************************************************************/

/*----------- defined in sndhrdw/aztarac.c -----------*/

READ16_HANDLER( aztarac_sound_r );
WRITE16_HANDLER( aztarac_sound_w );

READ_HANDLER( aztarac_snd_command_r );
READ_HANDLER( aztarac_snd_status_r );
WRITE_HANDLER( aztarac_snd_status_w );

INTERRUPT_GEN( aztarac_snd_timed_irq );


/*----------- defined in vidhrdw/aztarac.c -----------*/

extern data16_t *aztarac_vectorram;

WRITE16_HANDLER( aztarac_ubr_w );

VIDEO_START( aztarac );
