/*************************************************************************

	Cinematronics Cosmic Chasm hardware

*************************************************************************/

/*----------- defined in machine/cchasm.c -----------*/

READ16_HANDLER( cchasm_6840_r );
WRITE16_HANDLER( cchasm_6840_w );

WRITE16_HANDLER( cchasm_led_w );


/*----------- defined in sndhrdw/cchasm.c -----------*/

READ_HANDLER( cchasm_snd_io_r );
WRITE_HANDLER( cchasm_snd_io_w );

WRITE16_HANDLER( cchasm_io_w );
READ16_HANDLER( cchasm_io_r );

int cchasm_sh_start(const struct MachineSound *msound);
void cchasm_sh_update(void);


/*----------- defined in vidhrdw/cchasm.c -----------*/

extern data16_t *cchasm_ram;

WRITE16_HANDLER( cchasm_refresh_control_w );
VIDEO_START( cchasm );
