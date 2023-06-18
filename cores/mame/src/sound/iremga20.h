/*********************************************************

	Irem GA20 PCM Sound Chip

*********************************************************/
#ifndef __IREMGA20_H__
#define __IREMGA20_H__

struct IremGA20_interface {
	int clock;					/* clock */
	int region;					/* memory region of sample ROM(s) */
	int mixing_level[2];		/* volume */
};

int IremGA20_sh_start( const struct MachineSound *msound );
void IremGA20_sh_stop( void );
WRITE_HANDLER( IremGA20_w );
READ_HANDLER( IremGA20_r );

#endif /* __IREMGA20_H__ */
