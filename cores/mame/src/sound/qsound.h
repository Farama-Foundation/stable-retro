/*********************************************************

    Capcom Q-Sound system

*********************************************************/

#ifndef __QSOUND_H__
#define __QSOUND_H__

#define QSOUND_CLOCK    4000000   /* default 4MHz clock */

struct QSound_interface {
	int clock;					/* clock */
	int region;					/* memory region of sample ROM(s) */
	int mixing_level[2];		/* volume */
};

int  qsound_sh_start( const struct MachineSound *msound );
void qsound_sh_stop( void );

WRITE_HANDLER( qsound_data_h_w );
WRITE_HANDLER( qsound_data_l_w );
WRITE_HANDLER( qsound_cmd_w );
READ_HANDLER( qsound_status_r );

#endif /* __QSOUND_H__ */
