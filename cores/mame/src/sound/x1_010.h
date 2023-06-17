struct x1_010_interface
{
	int clock;	/* clock */
	int volume;	/* volume */
	int adr;	/* address */
};


READ_HANDLER ( seta_sound_r );
WRITE_HANDLER( seta_sound_w );

READ16_HANDLER ( seta_sound_word_r );
WRITE16_HANDLER( seta_sound_word_w );

void seta_sound_enable_w(int);

int seta_sh_start( const struct MachineSound *msound );
void seta_sh_stop( void );
