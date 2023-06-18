/**********************************************************************************************
 *
 *   Yamaha YMZ280B driver
 *   by Aaron Giles
 *
 **********************************************************************************************/

#ifndef YMZ280B_H
#define YMZ280B_H

#define MAX_YMZ280B 			2

#ifndef VOL_YM3012
#define YM3012_VOL(LVol,LPan,RVol,RPan) (MIXER(LVol,LPan)|(MIXER(RVol,RPan) << 16))
#endif

struct YMZ280Binterface
{
	int num;                  						/* total number of chips */
	int baseclock[MAX_YMZ280B];						/* input clock */
	int region[MAX_YMZ280B];						/* memory region where the sample ROM lives */
	int mixing_level[MAX_YMZ280B];					/* master volume */
	void (*irq_callback[MAX_YMZ280B])(int state);	/* irq callback */
};

int YMZ280B_sh_start(const struct MachineSound *msound);
void YMZ280B_sh_stop(void);
void YMZ280B_set_sample_base(int which, void *base);

READ_HANDLER ( YMZ280B_status_0_r );
WRITE_HANDLER( YMZ280B_register_0_w );
WRITE_HANDLER( YMZ280B_data_0_w );

READ16_HANDLER ( YMZ280B_status_0_lsb_r );
READ16_HANDLER ( YMZ280B_status_0_msb_r );
WRITE16_HANDLER( YMZ280B_register_0_lsb_w );
WRITE16_HANDLER( YMZ280B_register_0_msb_w );
WRITE16_HANDLER( YMZ280B_data_0_lsb_w );
WRITE16_HANDLER( YMZ280B_data_0_msb_w );

READ_HANDLER ( YMZ280B_status_1_r );
WRITE_HANDLER( YMZ280B_register_1_w );
WRITE_HANDLER( YMZ280B_data_1_w );

READ16_HANDLER ( YMZ280B_status_1_lsb_r );
READ16_HANDLER ( YMZ280B_status_1_msb_r );
WRITE16_HANDLER( YMZ280B_register_1_lsb_w );
WRITE16_HANDLER( YMZ280B_register_1_msb_w );
WRITE16_HANDLER( YMZ280B_data_1_lsb_w );
WRITE16_HANDLER( YMZ280B_data_1_msb_w );

#endif
