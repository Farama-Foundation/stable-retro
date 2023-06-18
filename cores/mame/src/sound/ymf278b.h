#ifndef __YMF278B_H__
#define __YMF278B_H__

#define MAX_YMF278B	(2)

#define YMF278B_STD_CLOCK (33868800)			/* standard clock for OPL4 */

struct YMF278B_interface {
	int num;        				/* Number of chips */
	int clock[MAX_YMF278B];				/* clock input, normally 33.8688 MHz */
	int region[MAX_YMF278B];			/* memory region of sample ROMs */
	int mixing_level[MAX_YMF278B];			/* volume */
	void (*irq_callback[MAX_YMF278B])(int state);	/* irq callback */
};

int  YMF278B_sh_start( const struct MachineSound *msound );
void YMF278B_sh_stop( void );

READ_HANDLER( YMF278B_status_port_0_r );
READ_HANDLER( YMF278B_data_port_0_r );
WRITE_HANDLER( YMF278B_control_port_0_A_w );
WRITE_HANDLER( YMF278B_data_port_0_A_w );
WRITE_HANDLER( YMF278B_control_port_0_B_w );
WRITE_HANDLER( YMF278B_data_port_0_B_w );
WRITE_HANDLER( YMF278B_control_port_0_C_w );
WRITE_HANDLER( YMF278B_data_port_0_C_w );

READ_HANDLER( YMF278B_status_port_1_r );
READ_HANDLER( YMF278B_data_port_1_r );
WRITE_HANDLER( YMF278B_control_port_1_A_w );
WRITE_HANDLER( YMF278B_data_port_1_A_w );
WRITE_HANDLER( YMF278B_control_port_1_B_w );
WRITE_HANDLER( YMF278B_data_port_1_B_w );
WRITE_HANDLER( YMF278B_control_port_1_C_w );
WRITE_HANDLER( YMF278B_data_port_1_C_w );

#endif
