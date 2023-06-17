/*************************************************************************

	Cinematronics vector hardware

*************************************************************************/

/*----------- defined in sndhrdw/cinemat.c -----------*/

typedef void (*cinemat_sound_handler_proc)(UINT8, UINT8);
extern cinemat_sound_handler_proc cinemat_sound_handler;

MACHINE_INIT( cinemat_sound );

READ16_HANDLER( cinemat_output_port_r );
WRITE16_HANDLER( cinemat_output_port_w );

extern struct Samplesinterface starcas_samples_interface;
extern struct Samplesinterface warrior_samples_interface;
extern struct Samplesinterface ripoff_samples_interface;
extern struct Samplesinterface solarq_samples_interface;
extern struct Samplesinterface spacewar_samples_interface;
extern struct Samplesinterface armora_samples_interface;
extern struct Samplesinterface sundance_samples_interface;
extern struct Samplesinterface tailg_samples_interface;

void tailg_sound_w(UINT8 sound_val, UINT8 bits_changed);
void starcas_sound_w(UINT8 sound_val, UINT8 bits_changed);
void warrior_sound_w(UINT8 sound_val, UINT8 bits_changed);
void armora_sound_w(UINT8 sound_val, UINT8 bits_changed);
void ripoff_sound_w(UINT8 sound_val, UINT8 bits_changed);
void solarq_sound_w(UINT8 sound_val, UINT8 bits_changed);
void spacewar_sound_w(UINT8 sound_val, UINT8 bits_changed);
void demon_sound_w(UINT8 sound_val, UINT8 bits_changed);
void armora_sound_w(UINT8 sound_val, UINT8 bits_changed);
void sundance_sound_w(UINT8 sound_val, UINT8 bits_changed);

MACHINE_DRIVER_EXTERN( demon_sound );


/*----------- defined in vidhrdw/cinemat.c -----------*/

void CinemaVectorData(int fromx, int fromy, int tox, int toy, int color);

PALETTE_INIT( cinemat );
PALETTE_INIT( cinemat_color );
VIDEO_START( cinemat );
VIDEO_EOF( cinemat );

VIDEO_UPDATE( spacewar );
