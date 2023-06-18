/***************************************************************************

	Atari Star Wars hardware

***************************************************************************/

#include "driver.h"

/*----------- defined in drivers/starwars.c -----------*/

extern UINT8 starwars_is_esb;


/*----------- defined in machine/starwars.c -----------*/

WRITE_HANDLER( starwars_out_w );
READ_HANDLER( starwars_input_1_r );

READ_HANDLER( starwars_adc_r );
WRITE_HANDLER( starwars_adc_select_w );

void swmathbox_init(void);
void swmathbox_reset(void);

READ_HANDLER( swmathbx_prng_r );
READ_HANDLER( swmathbx_reh_r );
READ_HANDLER( swmathbx_rel_r );

WRITE_HANDLER( swmathbx_w );


/*----------- defined in sndhrdw/starwars.c -----------*/

READ_HANDLER( starwars_main_read_r );
READ_HANDLER( starwars_main_ready_flag_r );
WRITE_HANDLER( starwars_main_wr_w );
WRITE_HANDLER( starwars_soundrst_w );

READ_HANDLER( starwars_sin_r );
READ_HANDLER( starwars_m6532_r );

WRITE_HANDLER( starwars_sout_w );
WRITE_HANDLER( starwars_m6532_w );
