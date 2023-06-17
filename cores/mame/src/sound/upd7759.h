#ifndef UPD7759S_H
#define UPD7759S_H

#define MAX_UPD7759 2

/* There are two modes for the uPD7759, selected through the !MD pin.
   This is the mode select input.  High is stand alone, low is slave.
   We're making the assumption that nobody switches modes through
   software. */

#define UPD7759_STANDARD_CLOCK		640000

struct upd7759_interface
{
	int num;					/* number of chips */
	int clock[MAX_UPD7759];		/* clock (per chip) */
	int volume[MAX_UPD7759];	/* volume (per chip) */
	int region[MAX_UPD7759]; 	/* memory region (per chip, standalone mode only) */
	void (*drqcallback[MAX_UPD7759])(int param);	/* drq callback (per chip, slave mode only) */
};

int upd7759_sh_start(const struct MachineSound *msound);
void upd7759_sh_stop(void);

void upd7759_set_bank_base(int which, offs_t base);

void upd7759_reset_w(int num, UINT8 data);

void upd7759_w(int num, UINT8 data);
void upd7759_port_w(int num, UINT8 data);
void upd7759_start_w(int num, UINT8 data);
int upd7759_busy_r(int num);
void upd7759_reset(int which);
WRITE_HANDLER( upd7759_0_reset_w );
WRITE_HANDLER( upd7759_0_port_w );
WRITE_HANDLER( upd7759_0_start_w );
READ_HANDLER( upd7759_0_busy_r );

#endif

