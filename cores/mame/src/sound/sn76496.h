#ifndef SN76496_H
#define SN76496_H

#define MAX_76496 5

struct SN76496interface
{
	int num;	/* total number of 76496 in the machine */
	int baseclock[MAX_76496];
	int volume[MAX_76496];
};

int SN76496_sh_start(const struct MachineSound *msound);
WRITE_HANDLER( SN76496_0_w );
WRITE_HANDLER( SN76496_1_w );
WRITE_HANDLER( SN76496_2_w );
WRITE_HANDLER( SN76496_3_w );
WRITE_HANDLER( SN76496_4_w );
#endif
