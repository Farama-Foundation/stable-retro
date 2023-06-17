/*************************************************************************

	Atari Major Havoc hardware

*************************************************************************/

#define MHAVOC_CLOCK		10000000
#define MHAVOC_CLOCK_5M		(MHAVOC_CLOCK/2)
#define MHAVOC_CLOCK_2_5M	(MHAVOC_CLOCK/4)
#define MHAVOC_CLOCK_1_25M	(MHAVOC_CLOCK/8)
#define MHAVOC_CLOCK_625K	(MHAVOC_CLOCK/16)

#define MHAVOC_CLOCK_156K	(MHAVOC_CLOCK_625K/4)
#define MHAVOC_CLOCK_5K		(MHAVOC_CLOCK_625K/16/8)
#define MHAVOC_CLOCK_2_4K	(MHAVOC_CLOCK_625K/16/16)


/*----------- defined in machine/mhavoc.c -----------*/

WRITE_HANDLER( mhavoc_alpha_irq_ack_w );
WRITE_HANDLER( mhavoc_gamma_irq_ack_w );

MACHINE_INIT( mhavoc );

WRITE_HANDLER( mhavoc_gamma_w );
READ_HANDLER( mhavoc_alpha_r );

WRITE_HANDLER( mhavoc_alpha_w );
READ_HANDLER( mhavoc_gamma_r );

WRITE_HANDLER( mhavoc_ram_banksel_w );
WRITE_HANDLER( mhavoc_rom_banksel_w );

READ_HANDLER( mhavoc_port_0_r );
READ_HANDLER( alphaone_port_0_r );
READ_HANDLER( mhavoc_port_1_r );

WRITE_HANDLER( mhavoc_out_0_w );
WRITE_HANDLER( alphaone_out_0_w );
WRITE_HANDLER( mhavoc_out_1_w );
