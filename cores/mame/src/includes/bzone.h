/*************************************************************************

	Atari Battle Zone hardware

*************************************************************************/

/*----------- defined in drivers/bzone.c -----------*/

READ_HANDLER( bzone_IN0_r );


/*----------- defined in sndhrdw/bzone.c -----------*/

WRITE_HANDLER( bzone_sounds_w );

int bzone_sh_start(const struct MachineSound *msound);
void bzone_sh_stop(void);
void bzone_sh_update(void);


/*----------- defined in sndhrdw/redbaron.c -----------*/

WRITE_HANDLER( redbaron_sounds_w );
WRITE_HANDLER( redbaron_pokey_w );

int redbaron_sh_start(const struct MachineSound *msound);
void redbaron_sh_stop(void);
void redbaron_sh_update(void);
