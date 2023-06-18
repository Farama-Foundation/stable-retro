#ifndef YMF262INTF_H
#define YMF262INTF_H


#define MAX_262 2

#ifndef YAC512_VOL
/* YMF262interface->mixing_levelXX macro */
#define YAC512_VOL(LVol,LPan,RVol,RPan) (MIXER(LVol,LPan)|(MIXER(RVol,RPan) << 16))
#endif

/* note: YMF262 outputs 4 (four) separate channels.
BOTH mixing level fields have to be filled using YAC512_VOL() macro */

/* most PC sound cards (all ?) don't use channels C and D */

struct YMF262interface
{
	int num;
	int baseclock;					/* in Hz, typical clock is 14318180 Hz */
	int mixing_levelAB[MAX_262];	/* channels A,B output from DOAB pin (#21 on YMF262-M) */
	int mixing_levelCD[MAX_262];	/* channels C,D output from DOCD pin (#22 on YMF262-M) */
	void (*handler[MAX_262])(int irq);
};



/* YMF262 */
READ_HANDLER ( YMF262_status_0_r );
WRITE_HANDLER( YMF262_register_A_0_w );
WRITE_HANDLER( YMF262_register_B_0_w );
WRITE_HANDLER( YMF262_data_A_0_w );
WRITE_HANDLER( YMF262_data_B_0_w );


READ_HANDLER ( YMF262_status_1_r );
WRITE_HANDLER( YMF262_register_A_1_w );
WRITE_HANDLER( YMF262_register_B_1_w );
WRITE_HANDLER( YMF262_data_A_1_w );
WRITE_HANDLER( YMF262_data_B_1_w );

int YMF262_sh_start(const struct MachineSound *msound);
void YMF262_sh_stop(void);
void YMF262_sh_reset(void);


#endif
