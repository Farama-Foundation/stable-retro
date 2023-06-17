#ifndef __2610INTF_H__
#define __2610INTF_H__

#include "fm.h"
#ifdef BUILD_YM2610
  void YM2610UpdateRequest(int chip);
#endif

#define   MAX_2610    (2)

#ifndef VOL_YM3012
/* #define YM3014_VOL(Vol,Pan) VOL_YM3012((Vol)/2,Pan,(Vol)/2,Pan) */
#define YM3012_VOL(LVol,LPan,RVol,RPan) (MIXER(LVol,LPan)|(MIXER(RVol,RPan) << 16))
#endif

struct YM2610interface
{
	int num;	/* total number of 8910 in the machine */
	int baseclock;
	int volumeSSG[MAX_8910]; /* for SSG sound */
	mem_read_handler portAread[MAX_8910];
	mem_read_handler portBread[MAX_8910];
	mem_write_handler portAwrite[MAX_8910];
	mem_write_handler portBwrite[MAX_8910];
	void ( *handler[MAX_8910] )( int irq );	/* IRQ handler for the YM2610 */
	int pcmromb[MAX_2610];		/* Delta-T rom region */
	int pcmroma[MAX_2610];		/* ADPCM   rom region */
	int volumeFM[MAX_2610];		/* use YM3012_VOL macro */
};

/************************************************/
/* Sound Hardware Start							*/
/************************************************/
int YM2610_sh_start(const struct MachineSound *msound);
int YM2610B_sh_start(const struct MachineSound *msound);

/************************************************/
/* Sound Hardware Stop							*/
/************************************************/
void YM2610_sh_stop(void);

void YM2610_sh_reset(void);

/************************************************/
/* Chip 0 functions								*/
/************************************************/
READ_HANDLER( YM2610_status_port_0_A_r );
READ16_HANDLER( YM2610_status_port_0_A_lsb_r );
READ_HANDLER( YM2610_status_port_0_B_r );
READ16_HANDLER( YM2610_status_port_0_B_lsb_r );
READ_HANDLER( YM2610_read_port_0_r );
READ16_HANDLER( YM2610_read_port_0_lsb_r );
WRITE_HANDLER( YM2610_control_port_0_A_w );
WRITE16_HANDLER( YM2610_control_port_0_A_lsb_w );
WRITE_HANDLER( YM2610_control_port_0_B_w );
WRITE16_HANDLER( YM2610_control_port_0_B_lsb_w );
WRITE_HANDLER( YM2610_data_port_0_A_w );
WRITE16_HANDLER( YM2610_data_port_0_A_lsb_w );
WRITE_HANDLER( YM2610_data_port_0_B_w );
WRITE16_HANDLER( YM2610_data_port_0_B_lsb_w );

/************************************************/
/* Chip 1 functions								*/
/************************************************/
READ_HANDLER( YM2610_status_port_1_A_r );
READ16_HANDLER( YM2610_status_port_1_A_lsb_r );
READ_HANDLER( YM2610_status_port_1_B_r );
READ16_HANDLER( YM2610_status_port_1_B_lsb_r );
READ_HANDLER( YM2610_read_port_1_r );
READ16_HANDLER( YM2610_read_port_1_lsb_r );
WRITE_HANDLER( YM2610_control_port_1_A_w );
WRITE16_HANDLER( YM2610_control_port_1_A_lsb_w );
WRITE_HANDLER( YM2610_control_port_1_B_w );
WRITE16_HANDLER( YM2610_control_port_1_B_lsb_w );
WRITE_HANDLER( YM2610_data_port_1_A_w );
WRITE16_HANDLER( YM2610_data_port_1_A_lsb_w );
WRITE_HANDLER( YM2610_data_port_1_B_w );
WRITE16_HANDLER( YM2610_data_port_1_B_lsb_w );

#endif
/**************** end of file ****************/
