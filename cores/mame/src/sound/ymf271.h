#ifndef _YMF271_H_
#define _YMF271_H_

#define MAX_YMF271	(2)

struct YMF271interface 
{
	int num;
	int region[MAX_YMF271];			/* memory region of sample ROMs */
	int mixing_level[MAX_YMF271];			/* volume */
	read8_handler ext_read[MAX_YMF271];		/* external memory read */
	write8_handler ext_write[MAX_YMF271];	/* external memory write */
	void (*irq_callback[MAX_YMF271])(int state);	/* irq callback */
};

int YMF271_sh_start( const struct MachineSound *msound );
void YMF271_sh_stop( void );

READ_HANDLER( YMF271_0_r );
WRITE_HANDLER( YMF271_0_w );
READ_HANDLER( YMF271_1_r );
WRITE_HANDLER( YMF271_1_w );

#endif
