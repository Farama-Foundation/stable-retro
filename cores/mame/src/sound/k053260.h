/*********************************************************

	Konami 053260 PCM/ADPCM Sound Chip

*********************************************************/
#ifndef __K053260_H__
#define __K053260_H__

#define MAX_053260 2

struct K053260_interface {
	int	num;								/* number of chips */
	int clock[MAX_053260];					/* clock */
	int region[MAX_053260];					/* memory region of sample ROM(s) */
	int mixing_level[MAX_053260][2];		/* volume */
	void (*irq[MAX_053260])( int param );	/* called on SH1 complete cycle ( clock / 32 ) */
};


int K053260_sh_start( const struct MachineSound *msound );
void K053260_sh_stop( void );

WRITE_HANDLER( K053260_0_w );
WRITE_HANDLER( K053260_1_w );
READ_HANDLER( K053260_0_r );
READ_HANDLER( K053260_1_r );
WRITE16_HANDLER( K053260_0_lsb_w );
READ16_HANDLER( K053260_0_lsb_r );
WRITE16_HANDLER( K053260_1_lsb_w );
READ16_HANDLER( K053260_1_lsb_r );

#endif /* __K053260_H__ */
