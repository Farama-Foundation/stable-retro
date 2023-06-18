/*************************************************************************

	Incredible Technologies/Strata system
	(8-bit blitter variant)

**************************************************************************/

/*----------- defined in drivers/itech8.c -----------*/

void itech8_update_interrupts(int periodic, int tms34061, int blitter);


/*----------- defined in machine/slikshot.c -----------*/

READ_HANDLER( slikz80_port_r );
WRITE_HANDLER( slikz80_port_w );

READ_HANDLER( slikshot_z80_r );
READ_HANDLER( slikshot_z80_control_r );
WRITE_HANDLER( slikshot_z80_control_w );

void slikshot_extra_draw(struct mame_bitmap *bitmap, const struct rectangle *cliprect);


/*----------- defined in vidhrdw/itech8.c -----------*/

extern UINT8 *itech8_grom_bank;
extern UINT8 *itech8_display_page;

VIDEO_START( itech8 );
VIDEO_START( slikshot );

WRITE_HANDLER( itech8_palette_w );

READ_HANDLER( itech8_blitter_r );
WRITE_HANDLER( itech8_blitter_w );

WRITE_HANDLER( itech8_tms34061_w );
READ_HANDLER( itech8_tms34061_r );

VIDEO_UPDATE( itech8 );
