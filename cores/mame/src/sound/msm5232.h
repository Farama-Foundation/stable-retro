#ifndef _H_MSM5232_
#define _H_MSM5232_

#define MAX_MSM5232 2

struct MSM5232interface
{
	int		num;						/* total number of chips */
	int		baseclock;					/* in Hz, master clock 1 and 2 (default = 2119040 Hz) */
	double	capacity[MAX_MSM5232][8];	/* in Farads, capacitors connected to pins: 24,25,26,27 and 37,38,39,40 */
	int		mixing_level[MAX_MSM5232];	/* master volume */
};

int  MSM5232_sh_start (const struct MachineSound *msound);
void MSM5232_sh_stop  (void);
void MSM5232_sh_reset (void);

WRITE_HANDLER( MSM5232_0_w );
WRITE_HANDLER( MSM5232_1_w );

#endif
