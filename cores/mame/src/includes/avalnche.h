/*************************************************************************

	Atari Avalanche hardware

*************************************************************************/

/*----------- defined in machine/avalnche.c -----------*/

READ_HANDLER( avalnche_input_r );
WRITE_HANDLER( avalnche_output_w );
WRITE_HANDLER( avalnche_noise_amplitude_w );
INTERRUPT_GEN( avalnche_interrupt );


/*----------- defined in vidhrdw/avalnche.c -----------*/

WRITE_HANDLER( avalnche_videoram_w );
VIDEO_UPDATE( avalnche );
