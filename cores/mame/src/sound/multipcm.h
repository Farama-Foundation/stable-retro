#ifndef __MultiPCM_H__
#define __MultiPCM_H__

#define MAX_MULTIPCM	(2)	/* max # of multipcm chips*/

/* banking types (done via an external PAL, so can vary per game)*/
#define MULTIPCM_MODE_MODEL1	(0)	/* Model 1 banking method*/
#define MULTIPCM_MODE_MULTI32	(1)	/* Multi32 banking method*/
#define MULTIPCM_MODE_STADCROSS (2)	/* Stadium Cross banking method*/

#ifndef VOL_YM3012
#define YM3012_VOL(LVol,LPan,RVol,RPan) (MIXER(LVol,LPan)|(MIXER(RVol,RPan) << 16))
#endif

struct MultiPCM_interface
{
	int chips;
	int clock[MAX_MULTIPCM];
	int type[MAX_MULTIPCM];
	int banksize[MAX_MULTIPCM];
	int region[MAX_MULTIPCM];
	int mixing_level[MAX_MULTIPCM];
};


int MultiPCM_sh_start(const struct MachineSound *msound);
void MultiPCM_sh_stop(void);

WRITE_HANDLER( MultiPCM_reg_0_w );
READ_HANDLER( MultiPCM_reg_0_r);
WRITE_HANDLER( MultiPCM_reg_1_w );
READ_HANDLER( MultiPCM_reg_1_r);
WRITE_HANDLER( MultiPCM_bank_0_w );
WRITE_HANDLER( MultiPCM_bank_1_w );

#endif
