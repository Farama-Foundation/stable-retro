/***************************************************************************

	Bally/Sente SAC-1 system

    driver by Aaron Giles

***************************************************************************/

#include "driver.h"


/*----------- defined in machine/balsente.c -----------*/

extern UINT8 balsente_shooter;
extern UINT8 balsente_shooter_x;
extern UINT8 balsente_shooter_y;
extern UINT8 balsente_adc_shift;
extern data16_t *shrike_shared;
extern data16_t *shrike_io;

MACHINE_INIT( balsente );

void balsente_noise_gen(int chip, int count, short *buffer);

WRITE_HANDLER( balsente_random_reset_w );
READ_HANDLER( balsente_random_num_r );

WRITE_HANDLER( balsente_rombank_select_w );
WRITE_HANDLER( balsente_rombank2_select_w );

WRITE_HANDLER( balsente_misc_output_w );

READ_HANDLER( balsente_m6850_r );
WRITE_HANDLER( balsente_m6850_w );

READ_HANDLER( balsente_m6850_sound_r );
WRITE_HANDLER( balsente_m6850_sound_w );

INTERRUPT_GEN( balsente_update_analog_inputs );
READ_HANDLER( balsente_adc_data_r );
WRITE_HANDLER( balsente_adc_select_w );

READ_HANDLER( balsente_counter_8253_r );
WRITE_HANDLER( balsente_counter_8253_w );

READ_HANDLER( balsente_counter_state_r );
WRITE_HANDLER( balsente_counter_control_w );

WRITE_HANDLER( balsente_chip_select_w );
WRITE_HANDLER( balsente_dac_data_w );
WRITE_HANDLER( balsente_register_addr_w );

READ_HANDLER( nstocker_port2_r );
WRITE_HANDLER( spiker_expand_w );
READ_HANDLER( spiker_expand_r );
READ_HANDLER( grudge_steering_r );

READ_HANDLER( shrike_shared_6809_r );
WRITE_HANDLER( shrike_shared_6809_w );
READ16_HANDLER( shrike_io_68k_r );
WRITE16_HANDLER( shrike_io_68k_w );


/*----------- defined in vidhrdw/balsente.c -----------*/

VIDEO_START( balsente );
VIDEO_UPDATE( balsente );

WRITE_HANDLER( balsente_videoram_w );
WRITE_HANDLER( balsente_paletteram_w );
WRITE_HANDLER( balsente_palette_select_w );
WRITE_HANDLER( shrike_sprite_select_w );
