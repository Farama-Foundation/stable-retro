/**********************************************************************************************
 *
 *   Data East BSMT2000 driver
 *   by Aaron Giles
 *
 **********************************************************************************************/

#ifndef BSMT2000_H
#define BSMT2000_H

#define MAX_BSMT2000 			1

struct BSMT2000interface
{
	int num;                  						/* total number of chips */
	int baseclock[MAX_BSMT2000];					/* input clock */
	int voices[MAX_BSMT2000];						/* number of voices (11 or 12) */
	int region[MAX_BSMT2000];						/* memory region where the sample ROM lives */
	int mixing_level[MAX_BSMT2000];					/* master volume */
};

int BSMT2000_sh_start(const struct MachineSound *msound);
void BSMT2000_sh_stop(void);
void BSMT2000_sh_reset(void);

WRITE16_HANDLER( BSMT2000_data_0_w );

#endif
