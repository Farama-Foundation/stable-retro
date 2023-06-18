/***************************************************************************

	sndhrdw/mcr.c

	Functions to emulate general the various MCR sound cards.

***************************************************************************/

#include "machine/6821pia.h"



/************ Generic MCR routines ***************/

void mcr_sound_init(void);

WRITE_HANDLER( ssio_data_w );
READ_HANDLER( ssio_status_r );
void ssio_reset_w(int state);

WRITE_HANDLER( csdeluxe_data_w );
READ_HANDLER( csdeluxe_status_r );
void csdeluxe_reset_w(int state);

WRITE_HANDLER( turbocs_data_w );
READ_HANDLER( turbocs_status_r );
void turbocs_reset_w(int state);

WRITE_HANDLER( soundsgood_data_w );
READ_HANDLER( soundsgood_status_r );
void soundsgood_reset_w(int state);

WRITE_HANDLER( squawkntalk_data_w );
void squawkntalk_reset_w(int state);



/************ Sound Configuration ***************/

extern UINT8 mcr_sound_config;

#define MCR_SSIO				0x01
#define MCR_CHIP_SQUEAK_DELUXE	0x02
#define MCR_SOUNDS_GOOD			0x04
#define MCR_TURBO_CHIP_SQUEAK	0x08
#define MCR_SQUAWK_N_TALK		0x10
#define MCR_WILLIAMS_SOUND		0x20

#define MCR_CONFIGURE_SOUND(x) \
	mcr_sound_config = x



/************ External definitions ***************/

MACHINE_DRIVER_EXTERN( mcr_ssio );
MACHINE_DRIVER_EXTERN( chip_squeak_deluxe );
MACHINE_DRIVER_EXTERN( sounds_good );
MACHINE_DRIVER_EXTERN( turbo_chip_squeak );
MACHINE_DRIVER_EXTERN( turbo_chip_squeak_plus_sounds_good );
MACHINE_DRIVER_EXTERN( squawk_n_talk );
