/*************************************************************************

	Coors Light Bowling/Bowl-O-Rama hardware

*************************************************************************/

/*----------- defined in vidhrdw/capbowl.c -----------*/

MACHINE_INIT( capbowl );

VIDEO_START( capbowl );
VIDEO_UPDATE( capbowl );

extern unsigned char *capbowl_rowaddress;

WRITE_HANDLER( capbowl_rom_select_w );

READ_HANDLER( capbowl_pagedrom_r );

WRITE_HANDLER( bowlrama_turbo_w );
READ_HANDLER( bowlrama_turbo_r );

WRITE_HANDLER( capbowl_tms34061_w );
READ_HANDLER( capbowl_tms34061_r );
