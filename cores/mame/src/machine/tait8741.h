#ifndef __TAITO8741__
#define __TAITO8741__

#define MAX_TAITO8741 4

/* NEC 8741 program mode */
#define TAITO8741_MASTER 0
#define TAITO8741_SLAVE  1
#define TAITO8741_PORT   2

struct TAITO8741interface
{
	int num;
	int mode[MAX_TAITO8741];            /* program select */
	int serial_connect[MAX_TAITO8741];	/* serial port connection */
	mem_read_handler portHandler_r[MAX_TAITO8741]; /* parallel port handler */
};

int  TAITO8741_start(const struct TAITO8741interface *taito8741intf);
void TAITO8741_stop(void);

void TAITO8741_reset(int num);

/* write handler */
WRITE_HANDLER( TAITO8741_0_w );
WRITE_HANDLER( TAITO8741_1_w );
WRITE_HANDLER( TAITO8741_2_w );
WRITE_HANDLER( TAITO8741_3_w );
/* read handler */
READ_HANDLER( TAITO8741_0_r );
READ_HANDLER( TAITO8741_1_r );
READ_HANDLER( TAITO8741_2_r );
READ_HANDLER( TAITO8741_3_r );

/****************************************************************************
  joshi Vollyball set.
****************************************************************************/

extern int josvolly_nmi_enable;

void josvolly_8741_reset(void);
WRITE_HANDLER( josvolly_8741_0_w );
WRITE_HANDLER( josvolly_8741_1_w );
READ_HANDLER( josvolly_8741_0_r );
READ_HANDLER( josvolly_8741_1_r );

#endif
