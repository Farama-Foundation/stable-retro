#ifndef __SP0250_H__
#define __SP0250_H__

struct sp0250_interface {
	int volume;
	void (*drq_callback)(int state);
};

int  sp0250_sh_start( const struct MachineSound *msound );
void sp0250_sh_stop( void );

WRITE_HANDLER( sp0250_w );

#endif
