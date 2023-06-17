/*************************************************************************

	Atari CoJag hardware

*************************************************************************/

/*----------- defined in vidhrdw/cojag.c -----------*/

READ32_HANDLER( cojag_blitter_r );
WRITE32_HANDLER( cojag_blitter_w );

READ16_HANDLER( cojag_tom_regs_r );
WRITE16_HANDLER( cojag_tom_regs_w );

VIDEO_START( cojag );
VIDEO_UPDATE( cojag );
