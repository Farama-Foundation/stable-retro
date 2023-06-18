/***************************************************************************

	M72 audio interface

****************************************************************************/

MACHINE_INIT( m72_sound );
void m72_ym2151_irq_handler(int irq);
WRITE_HANDLER( m72_sound_command_w );
WRITE_HANDLER( m72_sound_irq_ack_w );
READ_HANDLER( m72_sample_r );
WRITE_HANDLER( m72_sample_w );

/* the port goes to different address bits depending on the game */
void m72_set_sample_start(int start);
WRITE_HANDLER( vigilant_sample_addr_w );
WRITE_HANDLER( shisen_sample_addr_w );
WRITE_HANDLER( rtype2_sample_addr_w );
WRITE_HANDLER( poundfor_sample_addr_w );
