/*************************************************************************

	Sega G-80 raster hardware

*************************************************************************/

/*----------- defined in machine/segar.c -----------*/

extern UINT8 *segar_mem;
extern void (*sega_decrypt)(int,unsigned int *);

void sega_security(int chip);
WRITE_HANDLER( segar_w );


/*----------- defined in sndhrdw/segar.c -----------*/

WRITE_HANDLER( astrob_speech_port_w );
WRITE_HANDLER( astrob_audio_ports_w );
WRITE_HANDLER( spaceod_audio_ports_w );
WRITE_HANDLER( monsterb_audio_8255_w );
 READ_HANDLER( monsterb_audio_8255_r );

 READ_HANDLER( monsterb_sh_rom_r );
 READ_HANDLER( monsterb_sh_t1_r );
 READ_HANDLER( monsterb_sh_command_r );
WRITE_HANDLER( monsterb_sh_dac_w );
WRITE_HANDLER( monsterb_sh_busy_w );
WRITE_HANDLER( monsterb_sh_offset_a0_a3_w );
WRITE_HANDLER( monsterb_sh_offset_a4_a7_w );
WRITE_HANDLER( monsterb_sh_offset_a8_a11_w );
WRITE_HANDLER( monsterb_sh_rom_select_w );

/* temporary speech handling through samples */
int astrob_speech_sh_start(const struct MachineSound *msound);
void astrob_speech_sh_update(void);

/* sample names */
extern const char *astrob_sample_names[];
extern const char *s005_sample_names[];
extern const char *monsterb_sample_names[];
extern const char *spaceod_sample_names[];


/*----------- defined in vidhrdw/segar.c -----------*/

extern UINT8 *segar_characterram;
extern UINT8 *segar_characterram2;
extern UINT8 *segar_mem_colortable;
extern UINT8 *segar_mem_bcolortable;

WRITE_HANDLER( segar_characterram_w );
WRITE_HANDLER( segar_characterram2_w );
WRITE_HANDLER( segar_colortable_w );
WRITE_HANDLER( segar_bcolortable_w );

WRITE_HANDLER( segar_video_port_w );

PALETTE_INIT( segar );
VIDEO_START( segar );
VIDEO_UPDATE( segar );

WRITE_HANDLER( monsterb_back_port_w );

VIDEO_START( monsterb );
VIDEO_UPDATE( monsterb );

WRITE_HANDLER( spaceod_back_port_w );
WRITE_HANDLER( spaceod_backshift_w );
WRITE_HANDLER( spaceod_backshift_clear_w );
WRITE_HANDLER( spaceod_backfill_w );
WRITE_HANDLER( spaceod_nobackfill_w );

VIDEO_START( spaceod );
VIDEO_UPDATE( spaceod );

WRITE_HANDLER( pignewt_back_color_w );
WRITE_HANDLER( pignewt_back_ports_w );

WRITE_HANDLER( sindbadm_back_port_w );

VIDEO_UPDATE( sindbadm );
