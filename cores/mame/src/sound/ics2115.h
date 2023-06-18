#ifndef __ICS2115_H__
#define __ICS2115_H__

struct ics2115_interface {
	int mixing_level[2];
	int region;
	void (*irq_cb)(int);
};

int  ics2115_sh_start( const struct MachineSound *msound );
void ics2115_sh_stop( void );
void ics2115_reset( void );

READ_HANDLER( ics2115_r );
WRITE_HANDLER( ics2115_w );

#endif
