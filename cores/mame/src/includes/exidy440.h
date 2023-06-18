/*************************************************************************

	Exidy 440 hardware

*************************************************************************/

/*----------- defined in drivers/exidy440.c -----------*/

extern UINT8 exidy440_bank;
extern UINT8 exidy440_topsecret;


/*----------- defined in sndhrdw/exidy440.c -----------*/

extern UINT8 exidy440_sound_command;
extern UINT8 exidy440_sound_command_ack;
extern UINT8 *exidy440_m6844_data;
extern UINT8 *exidy440_sound_banks;
extern UINT8 *exidy440_sound_volume;

int exidy440_sh_start(const struct MachineSound *msound);
void exidy440_sh_stop(void);
void exidy440_sh_update(void);

READ_HANDLER( exidy440_m6844_r );
WRITE_HANDLER( exidy440_m6844_w );
READ_HANDLER( exidy440_sound_command_r );
WRITE_HANDLER( exidy440_sound_volume_w );
WRITE_HANDLER( exidy440_sound_interrupt_clear_w );


/*----------- defined in vidhrdw/exidy440.c -----------*/

extern UINT8 *spriteram;
extern UINT8 *exidy440_imageram;
extern UINT8 *exidy440_scanline;
extern UINT8 exidy440_firq_vblank;
extern UINT8 exidy440_firq_beam;
extern UINT8 topsecex_yscroll;

INTERRUPT_GEN( exidy440_vblank_interrupt );

VIDEO_START( exidy440 );
VIDEO_EOF( exidy440 );
VIDEO_UPDATE( exidy440 );

READ_HANDLER( exidy440_videoram_r );
WRITE_HANDLER( exidy440_videoram_w );
READ_HANDLER( exidy440_paletteram_r );
WRITE_HANDLER( exidy440_paletteram_w );
WRITE_HANDLER( exidy440_spriteram_w );
WRITE_HANDLER( exidy440_control_w );
READ_HANDLER( exidy440_vertical_pos_r );
READ_HANDLER( exidy440_horizontal_pos_r );
WRITE_HANDLER( exidy440_interrupt_clear_w );
