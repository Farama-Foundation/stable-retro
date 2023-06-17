/*************************************************************************

	8080bw.h

*************************************************************************/

/*----------- defined in machine/8080bw.c -----------*/

WRITE_HANDLER( c8080bw_shift_amount_w );
WRITE_HANDLER( c8080bw_shift_data_w );
READ_HANDLER( c8080bw_shift_data_r );
READ_HANDLER( c8080bw_shift_data_rev_r );
READ_HANDLER( c8080bw_shift_data_comp_r );
INTERRUPT_GEN( c8080bw_interrupt );

READ_HANDLER( boothill_shift_data_r );

READ_HANDLER( spcenctr_port_0_r );
READ_HANDLER( spcenctr_port_1_r );

READ_HANDLER( boothill_port_0_r );
READ_HANDLER( boothill_port_1_r );

READ_HANDLER( gunfight_port_0_r );
READ_HANDLER( gunfight_port_1_r );

READ_HANDLER( seawolf_port_1_r );

WRITE_HANDLER( desertgu_controller_select_w );
READ_HANDLER( desertgu_port_1_r );

/*----------- defined in sndhrdw/8080bw.c -----------*/

MACHINE_INIT( invaders );
MACHINE_INIT( sstrangr );
MACHINE_INIT( invad2ct );
MACHINE_INIT( gunfight );
MACHINE_INIT( boothill );
MACHINE_INIT( phantom2 );
MACHINE_INIT( bowler );
MACHINE_INIT( ballbomb );
MACHINE_INIT( seawolf );
MACHINE_INIT( desertgu );
MACHINE_INIT( schaser );
MACHINE_INIT( polaris );
MACHINE_INIT( clowns );
MACHINE_INIT( indianbt );
MACHINE_INIT( lupin3 );
MACHINE_INIT( yosakdon );
MACHINE_INIT( lrescue );
MACHINE_INIT( invrvnge );
MACHINE_INIT( rollingc );
MACHINE_INIT( astropal );
MACHINE_INIT( galactic );


extern struct SN76477interface invaders_sn76477_interface;
extern struct Samplesinterface invaders_samples_interface;
extern struct SN76477interface lrescue_sn76477_interface;
extern struct Samplesinterface lrescue_samples_interface;
extern struct SN76477interface invad2ct_sn76477_interface;
extern struct Samplesinterface invad2ct_samples_interface;
extern struct Samplesinterface boothill_samples_interface;
extern struct DACinterface schaser_dac_interface;
extern struct CustomSound_interface schaser_custom_interface;
extern struct SN76477interface schaser_sn76477_interface;
extern struct Samplesinterface seawolf_samples_interface;
extern struct discrete_sound_block polaris_sound_interface[];


/*----------- defined in vidhrdw/8080bw.c -----------*/

DRIVER_INIT( 8080bw );
DRIVER_INIT( invaders );
DRIVER_INIT( invadpt2 );
DRIVER_INIT( cosmicmo );
DRIVER_INIT( yosakdon );
DRIVER_INIT( clowns );
DRIVER_INIT( cosmo );
DRIVER_INIT( sstrangr );
DRIVER_INIT( ozmawars );
DRIVER_INIT( sstrngr2 );
DRIVER_INIT( invaddlx );
DRIVER_INIT( invrvnge );
DRIVER_INIT( invad2ct );
DRIVER_INIT( schaser );
DRIVER_INIT( rollingc );
DRIVER_INIT( polaris );
DRIVER_INIT( lupin3 );
DRIVER_INIT( seawolf );
DRIVER_INIT( blueshrk );
DRIVER_INIT( desertgu );
DRIVER_INIT( phantom2 );
DRIVER_INIT( maze );
DRIVER_INIT( bowler );
DRIVER_INIT( gunfight );
DRIVER_INIT( bandido );
DRIVER_INIT( lrescue );
DRIVER_INIT( spclaser );
DRIVER_INIT( galxwars );
DRIVER_INIT( indianbt );
DRIVER_INIT( 280zzzap );
DRIVER_INIT( astropal );
DRIVER_INIT( galactic );

void c8080bw_flip_screen_w(int data);
void c8080bw_screen_red_w(int data);

INTERRUPT_GEN( polaris_interrupt );
INTERRUPT_GEN( phantom2_interrupt );

WRITE_HANDLER( c8080bw_videoram_w );
WRITE_HANDLER( schaser_colorram_w );
READ_HANDLER( schaser_colorram_r );
WRITE_HANDLER( spaceint_color_w );
WRITE_HANDLER( cosmo_colorram_w );

VIDEO_UPDATE( 8080bw );


PALETTE_INIT( invadpt2 );
PALETTE_INIT( lrescue );
PALETTE_INIT( sflush );
PALETTE_INIT( cosmo );
PALETTE_INIT( indianbt );

WRITE_HANDLER( bowler_bonus_display_w );
