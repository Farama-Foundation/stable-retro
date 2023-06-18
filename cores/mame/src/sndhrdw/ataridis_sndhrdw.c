/************************************************************************/
/* Discrete sound systems for Atari games                               */
/************************************************************************/

#include "driver.h"


/************************************************************************/
/* firetrk Sound System Analog emulation by K.Wilkins Feb 2001          */
/* Questions/Suggestions to mame@dysfunction.demon.co.uk                */
/* Modified and added superbug/montecar sounds.  Jan 2003 D.R.          */
/* Complete re-write Feb 2004, D. Renaud                                */
/************************************************************************/
const struct discrete_lfsr_desc firetrk_lfsr={
	16,			/* Bit Length */
	0,			/* Reset Value */
	0,			/* Use Bit 0 as XOR input 0 */
	14,			/* Use Bit 14 as XOR input 1 */
	DISC_LFSR_XNOR,		/* Feedback stage1 is XNOR */
	DISC_LFSR_OR,		/* Feedback stage2 is just stage 1 output OR with external feed */
	DISC_LFSR_REPLACE,	/* Feedback stage3 replaces the shifted register contents */
	0x000001,		/* Everything is shifted into the first bit only */
	0,			/* Output is not inverted */
	15			/* Output bit */
};

const struct discrete_dac_r1_ladder firetrk_motor_v_dac =
{
	4,		/* size of ladder */
	{2200000, 1000000, 470000, 220000, 0,0,0,0},	/* R24, R23, R22, R21 */
	4.4,		/* 5V - diode junction (0.6V) */
	68000,		/* R25 */
	0,		/* no rGnd */
	1e-5		/* C24 */
};

const struct discrete_555_cc_desc firetrk_motor_vco =
{
	DISC_555_OUT_DC | DISC_555_OUT_SQW,
	5,		/* B+ voltage of 555 */
	5.0 - 1.7,	/* High output voltage of 555 (Usually v555 - 1.7) */
	5.0 * 2 / 3,	/* threshold */
	5.0 /3,		/* trigger */
	5,		/* B+ voltage of the Constant Current source */
	0.7		/* Q2 junction voltage */
};

const struct discrete_dac_r1_ladder firetrk_motor_out_dac =
{
	2,		/* size of ladder */
	{10000, 10000, 0,0,0,0,0,0},	/* R74, R73 */
	3.4,		/* TTL high voltage */
	0,		/* no rBias */
	0,		/* no rGnd */
	1e-7		/* C43 */
};

const struct discrete_dac_r1_ladder firetrk_siren_cv_dac =
{
	4,		/* size of ladder */
	{2200000, 1000000, 470000, 220000, 0,0,0,0},	/* R46, R47, R45, R48 */
	5,		/* 5V */
	100000,		/* R49 */
	390000,		/* R44 */
	1e-5		/* C30 */
};

const struct discrete_555_astbl_desc firetrk_siren_555 =
{
	DISC_555_OUT_SQW | DISC_555_OUT_DC,
	5,		/* B+ voltage of 555 */
	5.0 - 1.7,	/* High output voltage of 555 (Usually v555 - 1.7) */
	5.0 * 2.0 /3.0,	/* normally 2/3 of v555 */
	5.0 / 3.0	/* normally 1/3 of v555 */
};

const struct discrete_dac_r1_ladder firetrk_bang_dac =
{
	4,		/* size of ladder */
	{8200, 3900, 2200, 1000, 0,0,0,0},	/* R37, R35, R36, R34 */
	0,		/* no vBias */
	0,		/* no rBias */
	0,		/* no rGnd */
	0		/* no smoothing cap */
};

const struct discrete_schmitt_osc_desc firetrk_screech_osc =
{
	2200,	/* R29 */
	330,	/* R16 */
	2.2e-6,	/* C8 */
	1.6,	/* Rise Threshold of 74LS14 */
	0.8,	/* Fall Threshold of 74LS14 */
	3.4,	/* Output high voltage of 74LS14 */
	DISC_SCHMITT_OSC_IN_IS_LOGIC | DISC_SCHMITT_OSC_ENAB_IS_NOR
};

const struct discrete_mixer_desc firetrk_mixer =
{
	DISC_MIXER_IS_OP_AMP,
	7,					/* 7 inputs */
	{4700, 22000, 31333, 33000, 10545.6, 32000, 150000, 0}, /* R54, R55, R72||(R70+R71), R53, R56 + R37||R35||R36||R34, R58 + R73||R74, R52 */
	{0,0,0,0,0,0,0,0},			/* No variable resistor nodes */
	{2.2e-7, 2.2e-7, 1e-8, 2.2e-7, 2.2e-7, 0, 2.2e-7, 0}, /* C34, C32, C44, C35, C33, NA, C31 */
	0,					/* No rI */
	22000,					/* R43 */
	0,					/* No Filter */
	1e-5,					/* C12 */
	5.0 * 820 / (270 + 820),		/* vBias = 5V * R51/(R41+R51) */
	3400
};

/* Nodes - Inputs */
#define FIRETRUCK_MOTOR_DATA	NODE_01
#define FIRETRUCK_HORN_EN	NODE_02
#define FIRETRUCK_SIREN_DATA	NODE_03
#define FIRETRUCK_CRASH_DATA	NODE_04
#define FIRETRUCK_SKID_EN	NODE_05
#define FIRETRUCK_BELL_EN	NODE_06
#define FIRETRUCK_XTNDPLY_EN	NODE_07
#define FIRETRUCK_ATTRACT_EN	NODE_08
#define FIRETRUCK_ATTRACT_INV	NODE_09
/* Nodes - Sounds */
#define FIRETRUCK_NOISE		NODE_11
#define FIRETRUCK_MOTORSND	NODE_12
#define FIRETRUCK_HORNSND	NODE_13
#define FIRETRUCK_SIRENSND	NODE_14
#define FIRETRUCK_BANGSND	NODE_15
#define FIRETRUCK_SCREECHSND	NODE_16
#define FIRETRUCK_BELLSND	NODE_17
#define FIRETRUCK_XTNDPLYSND	NODE_18

DISCRETE_SOUND_START(firetrk_sound_interface)
	/************************************************/
	/* Input register mapping for firetruck         */
	/************************************************/
	/*                  NODE             ADDR  MASK    GAIN    OFFSET  INIT */
	DISCRETE_INPUT(FIRETRUCK_MOTOR_DATA ,0x00,0x000f,                  0.0)
	DISCRETE_INPUT(FIRETRUCK_HORN_EN    ,0x01,0x000f,                  0.0)
	DISCRETE_INPUT(FIRETRUCK_SIREN_DATA ,0x02,0x000f,                  0.0)
	DISCRETE_INPUT(FIRETRUCK_CRASH_DATA ,0x03,0x000f,                 15.0)
	DISCRETE_INPUT(FIRETRUCK_SKID_EN    ,0x04,0x000f,                  0.0)
	DISCRETE_INPUT(FIRETRUCK_BELL_EN    ,0x05,0x000f,                  0.0)
	DISCRETE_INPUT(FIRETRUCK_ATTRACT_EN ,0x06,0x000f,                  0.0)
	DISCRETE_INPUT(FIRETRUCK_XTNDPLY_EN ,0x07,0x000f,                  0.0)

	DISCRETE_LOGIC_INVERT(FIRETRUCK_ATTRACT_INV, 1, FIRETRUCK_ATTRACT_EN)

	/************************************************/
	/* Motor sound circuit is based on a 556 VCO    */
	/* with the input frequency set by the MotorSND */
	/* latch (4 bit). This freqency is then used to */
	/* drive a modulo 12 counter, with div6 & div12 */
	/* summed as the output of the circuit.         */
	/************************************************/
	DISCRETE_ADJUSTMENT(NODE_20, 1,
				10000,	/* R26 + R27 @ min */
				260000,	/* R26 + R27 @ max */
				DISC_LOGADJ, 8)
	DISCRETE_DAC_R1(NODE_21, 1,		/* base of Q1 */
			FIRETRUCK_MOTOR_DATA,	/* IC F8, pins 2,5,6,9 */
			3.4,			/* TTL ON level */
			&firetrk_motor_v_dac)
	DISCRETE_555_CC(NODE_22, 1,	/* IC B9 pin 3, always enabled */
			NODE_21,	/* vIn */
			NODE_20,	/* current adjust */
			1.e-8,		/* C25 */
			1000000, 0, 0,	/* R28, no rGnd, no rDis */
			&firetrk_motor_vco)
	DISCRETE_COUNTER(NODE_23, 1, FIRETRUCK_ATTRACT_EN,	/* IC A9, QB-QD */
			NODE_22,				/* from IC B9, pin 3 */
			5, 1, 0, 1)				/* /6 counter on rising edge */
	DISCRETE_TRANSFORM2(NODE_24, 1, NODE_23, 2, "01>")	/* IC A9, pin 8 */
	DISCRETE_COUNTER(NODE_25, 1, FIRETRUCK_ATTRACT_EN,	/* IC A9, pin 12 */
			NODE_24,					/* from IC A9, pin 8 */
			1, 1, 0, 1)					/* /2 counter on rising edge */
	DISCRETE_TRANSFORM3(NODE_26, 1, NODE_24, NODE_25, 2, "12*0+")	/* Mix the mess together in binary */
	DISCRETE_DAC_R1(FIRETRUCK_MOTORSND, 1, NODE_26,
			3.4,				/* TTL ON level */
			&firetrk_motor_out_dac)

	/************************************************/
	/* Horn, this is taken from the 64V signal.     */
	/*  64V = HSYNC/128                             */
	/*      = 15750/128                             */
	/************************************************/
	DISCRETE_SQUAREWFIX(NODE_30, FIRETRUCK_ATTRACT_INV, 15750.0/128, 3.4, 50.0, 3.4/2, 0)
	DISCRETE_ONOFF(FIRETRUCK_HORNSND, FIRETRUCK_HORN_EN, NODE_30)

	/************************************************/
	/* Siren is built around a 556 based VCO, the   */
	/* 4 bit input value is smoothed between trans- */
	/* itions by a 10u capacitor.                   */
	/* 0000 = 666Hz with 35% duty cycle             */
	/* 1111 = 526Hz with 63% duty cycle             */
	/************************************************/
	DISCRETE_DAC_R1(NODE_40, 1,		/* IC E9, pin 7 */
			FIRETRUCK_SIREN_DATA,	/* IC F8, pins 15,16,12,19 */
			3.4,			/* TTL ON level */
			&firetrk_siren_cv_dac)
	DISCRETE_555_ASTABLE(FIRETRUCK_SIRENSND, FIRETRUCK_ATTRACT_INV,
				2200,	/* R60 */
				10000,	/* R59 */
				1e-7,	/* C37 */
				NODE_40,	/* CV is straight from DAC because op-amp E9 is just x1 buffer */
				&firetrk_siren_555)

	/************************************************/
	/* Crash circuit is built around a noise        */
	/* generator built from 2 shift registers that  */
	/* are clocked by the 2V signal.                */
	/* 2V = HSYNC/4                                 */
	/*    = 15750/4                                 */
	/* Output is binary weighted with 4 bits of     */
	/* crash volume.                                */
	/************************************************/
	DISCRETE_LFSR_NOISE(FIRETRUCK_NOISE, FIRETRUCK_ATTRACT_INV, FIRETRUCK_ATTRACT_INV, 15750.0/4, 1.0, 0, 0.5, &firetrk_lfsr)

	DISCRETE_SWITCH(NODE_50, 1, FIRETRUCK_NOISE, 0,	/* Enable gate K9 */
			FIRETRUCK_CRASH_DATA)		/* IC K9, pins 3,6,11,8 */
	DISCRETE_DAC_R1(FIRETRUCK_BANGSND, 1,	/* Bang */
			NODE_50,		/* from enable gates K9 */
			3.4,			/* TTL ON level */
			&firetrk_bang_dac)

	/************************************************/
	/* Screech is just the noise modulating a       */
	/* Schmitt VCO.                                 */
	/************************************************/
	DISCRETE_SCHMITT_OSCILLATOR(FIRETRUCK_SCREECHSND, FIRETRUCK_SKID_EN, FIRETRUCK_NOISE, 3.4, &firetrk_screech_osc)

	/************************************************/
	/* Bell circuit -                               */
	/* The Hsync signal is put into a div 16        */
	/* counter.                                     */
	/************************************************/
	DISCRETE_RCDISC2(NODE_70, FIRETRUCK_BELL_EN,
				4.4, 10,	/* Q3 instantally charges C42 */
				0, 33000,	/* discharges thru R66 */
				1e-5)		/* C42 */
	DISCRETE_TRANSFORM2(NODE_71, 1, NODE_70,
				5.0 * 680 / (10000 + 680),	/* vRef = 5V * R64 / (R65 + R64) */
				"01<")				/* Output is low until vIn drops below vRef */
	DISCRETE_COUNTER_FIX(NODE_72, 1, NODE_71, 15750, 15, 1, 0)	/* IC B10 */
	DISCRETE_TRANSFORM4(FIRETRUCK_BELLSND, 1, NODE_72,
				8,	/* count 0-7 allow cap voltage to output.  8-15 ground output. */
				NODE_70,	/* scale logic to cap voltage */
				47000.0 / (47000 + 47000 + 47000),	/* to drop voltage per divider R72 / (R70+R71+R72) */
				"01<2*3*")

	/************************************************/
	/* Extended play circuit is just the 8V signal  */
	/* 8V = HSYNC/16                                */
	/*    = 15750/16                                */
	/************************************************/
	DISCRETE_SQUAREWFIX(FIRETRUCK_XTNDPLYSND, FIRETRUCK_XTNDPLY_EN, 15750.0/16, 3.4, 50.0, 3.4/2, 0.0)

	/************************************************/
	/* Combine all 7 sound sources.                 */
	/************************************************/
	DISCRETE_MIXER7(NODE_90, 1, FIRETRUCK_XTNDPLYSND, FIRETRUCK_HORNSND, FIRETRUCK_BELLSND, FIRETRUCK_SCREECHSND, FIRETRUCK_BANGSND, FIRETRUCK_MOTORSND, FIRETRUCK_SIRENSND, &firetrk_mixer)
	DISCRETE_OUTPUT(NODE_90, 100)
DISCRETE_SOUND_END


/************************************************************************/
/* superbug Sound System Analog emulation                               */
/* Complete re-write Feb 2004, D. Renaud                                */
/************************************************************************/

const struct discrete_dac_r1_ladder superbug_motor_v_dac =
{
	4,		/* size of ladder */
	{2200000, 1000000, 470000, 220000, 0,0,0,0},	/* R8, R9, R6, R7 */
	4.4,	/* 5V - diode junction (0.6V) */
	68000,	/* R10 */
	0,		/* no rGnd */
	1e-5	/* C20 */
};

const struct discrete_555_cc_desc superbug_motor_vco =
{
	DISC_555_OUT_DC | DISC_555_OUT_SQW,
	5,				/* B+ voltage of 555 */
	5.0 - 1.7,		/* High output voltage of 555 (Usually v555 - 1.7) */
	5.0 * 2 / 3,	/* threshold */
	5.0 /3,			/* trigger */
	5,				/* B+ voltage of the Constant Current source */
	0.7				/* Q1 junction voltage */
};

const struct discrete_dac_r1_ladder superbug_motor_out_dac =
{
	2,		/* size of ladder */
	{10000, 10000, 0,0,0,0,0,0},	/* R32, R34 */
	3.4,	/* TTL high voltage */
	0,		/* no rBias */
	0,		/* no rGnd */
	1e-7	/* C26 */
};

const struct discrete_dac_r1_ladder superbug_bang_dac =
{
	4,		/* size of ladder */
	{8200, 3900, 2200, 1000, 0,0,0,0},	/* R28, R28, R27, R26 */
	0,		/* no vBias */
	0,		/* no rBias */
	0,		/* no rGnd */
	1e-7	/* C25 */
};

const struct discrete_schmitt_osc_desc superbug_screech_osc =
{
	2200,	/* R30 */
	330,	/* R52 */
	2.2e-6,	/* C27 */
	1.7,	/* Rise Threshold of 7414 */
	0.9,	/* Fall Threshold of 7414 */
	3.4,	/* Output high voltage of 7414 */
	DISC_SCHMITT_OSC_IN_IS_LOGIC | DISC_SCHMITT_OSC_ENAB_IS_NOR
};

const struct discrete_mixer_desc superbug_mixer =
{
	DISC_MIXER_IS_RESISTOR,
	4,							/* 4 inputs */
	{15000, 10545.6, 33000, 47000, 0,0,0,0}, /* R54, R55, R72||(R70+R71), R53, R56 + R37||R35||R36||R34, R58 + R73||R74, R52 */
	{0,0,0,0,0,0,0,0},			/* No variable resistor nodes */
	{0, 0, 0, 0, 0,0,0,0},		/* No caps */
	0,							/* No rI */
	5000,						/* R63 */
	0,							/* No Filter */
	1e-7,						/* C35 */
	0,							/* not used in resistor network */
	33000
};

/* Nodes - Inputs */
#define SUPERBUG_SPEED_DATA		NODE_01
#define SUPERBUG_CRASH_DATA		NODE_02
#define SUPERBUG_SKID_EN		NODE_03
#define SUPERBUG_ASR_EN			NODE_04
#define SUPERBUG_ATTRACT_EN		NODE_05
#define SUPERBUG_ATTRACT_INV	NODE_06
/* Nodes - Sounds */
#define SUPERBUG_NOISE			NODE_10
#define SUPERBUG_MOTORSND		NODE_11
#define SUPERBUG_BANGSND		NODE_12
#define SUPERBUG_SCREECHSND		NODE_13
#define SUPERBUG_ASRSND			NODE_14

DISCRETE_SOUND_START(superbug_sound_interface)
	/************************************************/
	/* Input register mapping for superbug          */
	/************************************************/
	/*                   NODE             ADDR   MASK   GAIN    OFFSET  INIT */
	DISCRETE_INPUT(SUPERBUG_SPEED_DATA  , 0x00, 0x000f,                 15.0)
	DISCRETE_INPUT(SUPERBUG_CRASH_DATA  , 0x03, 0x000f,                 15.0)
	DISCRETE_INPUT(SUPERBUG_SKID_EN     , 0x04, 0x000f,                  0.0)
	DISCRETE_INPUT(SUPERBUG_ATTRACT_EN  , 0x06, 0x000f,                  0.0)
	DISCRETE_INPUT(SUPERBUG_ASR_EN      , 0x07, 0x000f,                  0.0)

	DISCRETE_LOGIC_INVERT(SUPERBUG_ATTRACT_INV, 1, SUPERBUG_ATTRACT_EN)

	/************************************************/
	/* Motor sound circuit is based on a 556 VCO    */
	/* with the input frequency set by the MotorSND */
	/* latch (4 bit). This freqency is then used to */
	/* drive a modulo 12 counter.                   */
	/************************************************/
	DISCRETE_ADJUSTMENT(NODE_20, 1,
				10000,	/* R12 + R62 @ min */
				260000,	/* R12 + R62 @ max */
				DISC_LOGADJ, 8)
	DISCRETE_DAC_R1(NODE_21, 1,		/* base of Q1 */
			SUPERBUG_SPEED_DATA,	/* IC B5, pins 3, 14, 6, 11 */
			3.4,			/* TTL ON level */
			&superbug_motor_v_dac)
	DISCRETE_555_CC(NODE_22, 1,	/* IC A6 pin 3, always enabled */
			NODE_21,	/* vIn */
			NODE_20,	/* current adjust */
			1.e-8,		/* C21 */
			3300000, 0, 0,	/* R11, no rGnd, no rDis */
			&superbug_motor_vco)
	DISCRETE_COUNTER(NODE_23, 1, SUPERBUG_ATTRACT_EN,	/* IC A7, QB-QD */
			NODE_22,				/* from IC A6, pin 3 */
			5, 1, 0, 1)				/* /6 counter on rising edge */
	DISCRETE_TRANSFORM2(NODE_24, 1, NODE_23, 2, "01>")	/* IC A7, pin 8-QD */
	DISCRETE_TRANSFORM3(NODE_25, 1, NODE_23, 1, 4, "01=02=|")	/* IC A7, pin 11-QB */
	DISCRETE_LOGIC_XOR(NODE_26, 1, NODE_24, NODE_25)	/* Gate A9, pin 8 */
	DISCRETE_COUNTER(NODE_27, 1, SUPERBUG_ATTRACT_EN,	/* IC A7, pin 12-QA */
			NODE_26,					/* from IC A9, pin 8 */
			1, 1, 0, 1)					/* /2 counter on rising edge */
	DISCRETE_TRANSFORM3(NODE_28, 1, NODE_24, NODE_27, 2, "12*0+")	/* Mix the mess together in binary */
	DISCRETE_DAC_R1(SUPERBUG_MOTORSND, 1, NODE_28,
			3.4,				/* TTL ON level */
			&superbug_motor_out_dac)

	/************************************************/
	/* Crash circuit is built around a noise        */
	/* generator built from 2 shift registers that  */
	/* are clocked by the 2V signal.                */
	/* 2V = HSYNC/4                                 */
	/*    = 15750/4                                 */
	/* Output is binary weighted with 4 bits of     */
	/* crash volume.                                */
	/************************************************/
	DISCRETE_LFSR_NOISE(SUPERBUG_NOISE, SUPERBUG_ATTRACT_INV, SUPERBUG_ATTRACT_INV, 15750.0/4, 1.0, 0, 0.5, &firetrk_lfsr)	/* Same as firetrk */

	DISCRETE_SWITCH(NODE_40, 1, SUPERBUG_NOISE, 0,	/* Enable gate C8 */
			SUPERBUG_CRASH_DATA)		/* IC D8, pins 3,14,6,11 */
	DISCRETE_DAC_R1(SUPERBUG_BANGSND, 1,	/* Bang */
			NODE_40,		/* from enable gates C8 */
			3.4,			/* TTL ON level */
			&superbug_bang_dac)

	/************************************************/
	/* Screech is just the noise modulating a       */
	/* Schmitt VCO.                                 */
	/************************************************/
	DISCRETE_SCHMITT_OSCILLATOR(SUPERBUG_SCREECHSND, SUPERBUG_SKID_EN, SUPERBUG_NOISE, 3.4, &superbug_screech_osc)

	/************************************************/
	/* ASR circuit is just the 8V signal            */
	/* 8V = HSYNC/16                                */
	/*    = 15750/16                                */
	/************************************************/
	DISCRETE_SQUAREWFIX(SUPERBUG_ASRSND, SUPERBUG_ASR_EN, 15750.0/16, 3.4, 50.0, 3.4/2, 0.0)

	/************************************************/
	/* Combine all 4 sound sources.                 */
	/************************************************/
	DISCRETE_MIXER4(NODE_90, 1, SUPERBUG_MOTORSND, SUPERBUG_BANGSND, SUPERBUG_SCREECHSND, SUPERBUG_ASRSND, &superbug_mixer)
	DISCRETE_OUTPUT(NODE_90, 100)
DISCRETE_SOUND_END


/************************************************************************/
/* montecar Sound System Analog emulation                               */
/* Complete re-write Mar 2004, D. Renaud                                */
/************************************************************************/

/* Both cars use same parts for v_dac, vco and out_dac */
const struct discrete_dac_r1_ladder montecar_motor_v_dac =
{
	4,		/* size of ladder */
	{2200000, 1000000, 470000, 220000, 0,0,0,0},	/* R44, R43, R46, R45 */
	4.4,		/* 5V - diode junction (0.6V) */
	68000,		/* R80 */
	0,		/* no rGnd */
	4.7e-6		/* C77 */
};

const struct discrete_555_cc_desc montecar_motor_vco =
{
	DISC_555_OUT_DC | DISC_555_OUT_SQW,
	5,		/* B+ voltage of 555 */
	5.0 - 1.7,	/* High output voltage of 555 (Usually v555 - 1.7) */
	5.0 * 2 / 3,	/* threshold */
	5.0 /3,		/* trigger */
	5,		/* B+ voltage of the Constant Current source */
	0.7		/* Q1 junction voltage */
};

const struct discrete_dac_r1_ladder montecar_motor_out_dac =
{
	3,		/* size of ladder */
	{10000, 10000, 10000, 0,0,0,0,0},	/* R31, R30, R29 */
	3.4,		/* TTL high voltage */
	0,		/* no rBias */
	0,		/* no rGnd */
	1e-7		/* C53 */
};

const struct discrete_comp_adder_table montecar_drone_vol_res =
{
	DISC_COMP_P_RESISTOR,
	0,					/* no default */
	4,					/* # of resistors */
	{100250, 47250, 22250, 10250, 0,0,0,0}	/* R105, R104, R103, R102 (250 added to all for 4066 resistance) */
};

const struct discrete_dac_r1_ladder montecar_bang_dac =
{
	4,		/* size of ladder */
	{8200, 3900, 2200, 1000, 0,0,0,0},	/* R39, R42, R41, R40 */
	0,		/* no vBias */
	0,		/* no rBias */
	0,		/* no rGnd */
	0		/* no cSmoothing */
};

const struct discrete_schmitt_osc_desc montecar_screech_osc =
{
	2200,	/* R54 */
	330,	/* R53 */
	2.2e-6,	/* C57 */
	1.6,	/* Rise Threshold of 74LS14 */
	0.8,	/* Fall Threshold of 74LS14 */
	3.4,	/* Output high voltage of 74LS14 */
	DISC_SCHMITT_OSC_IN_IS_LOGIC | DISC_SCHMITT_OSC_ENAB_IS_NOR
};

const struct discrete_mixer_desc montecar_mixer =
{
	DISC_MIXER_IS_OP_AMP,
	5,					/* 5 inputs */
	{15000, 33000, 10000, 10000, 13333.3, 0,0,0}, /* R93, R97, R96, variable , R95 + R31||R30||R29 */
	{0,0,0,NODE_30,0,0,0,0},			/* Only drone has variable node */
	{2.2e-7, 2.2e-7, 2.2e-7, 1e-6, 1e-6, 0,0,0},	/* C83, C84, C85, C88, C47 */
	27000,					/* R92 */
	82000,					/* R98 */
	0,					/* No Filter */
	2.2e-7,					/* C6 */
	5,					/* vRef */
	8000
};

/* Nodes - Inputs */
#define MONTECAR_MOTOR_DATA		NODE_01	/* Motor1 - Motor4 */
#define MONTECAR_DRONE_MOTOR_DATA	NODE_02	/* Motor5 - Motor8 */
#define MONTECAR_CRASH_DATA		NODE_03
#define MONTECAR_SKID_EN		NODE_04
#define MONTECAR_DRONE_LOUD_DATA	NODE_05
#define MONTECAR_BEEPER_EN		NODE_06
#define MONTECAR_ATTRACT_EN		NODE_07
#define MONTECAR_ATTRACT_INV		NODE_08
/* Nodes - Sounds */
#define MONTECAR_NOISE			NODE_15
#define MONTECAR_MOTORSND		NODE_10	/* MotorAud1 */
#define MONTECAR_BEEPSND		NODE_11
#define MONTECAR_DRONE_MOTORSND		NODE_12	/* MotorAud2 */
#define MONTECAR_BANGSND		NODE_13
#define MONTECAR_SCREECHSND		NODE_14

DISCRETE_SOUND_START(montecar_sound_interface)
	/************************************************/
	/* Input register mapping for montecar          */
	/************************************************/
	/*                   NODE                   ADDR   MASK   GAIN    OFFSET  INIT */
	DISCRETE_INPUT(MONTECAR_MOTOR_DATA        , 0x00, 0x000f,                  0.0)
	DISCRETE_INPUT(MONTECAR_DRONE_MOTOR_DATA  , 0x02, 0x000f,                  0.0)
	DISCRETE_INPUT(MONTECAR_CRASH_DATA        , 0x03, 0x000f,                 15.0)
	DISCRETE_INPUT(MONTECAR_SKID_EN           , 0x04, 0x000f,                  0.0)
	DISCRETE_INPUT(MONTECAR_DRONE_LOUD_DATA   , 0x05, 0x000f,                  0.0)
	DISCRETE_INPUT(MONTECAR_ATTRACT_INV       , 0x06, 0x000f,                  0.0)
	DISCRETE_INPUT(MONTECAR_BEEPER_EN         , 0x07, 0x000f,                  0.0)

	DISCRETE_LOGIC_INVERT(MONTECAR_ATTRACT_EN, 1, MONTECAR_ATTRACT_INV)

	/************************************************/
	/* Motor sound circuit is based on a 556 VCO    */
	/* with the input frequency set by the MotorSND */
	/* latch (4 bit). This freqency is then used to */
	/* driver a modulo 12 counter, with div6, 4 & 3 */
	/* summed as the output of the circuit.         */
	/************************************************/
	DISCRETE_ADJUSTMENT(NODE_20, 1,
				10000,	/* R87 + R89 @ min */
				260000,	/* R87 + R89 @ max */
				DISC_LOGADJ, 8)
	DISCRETE_DAC_R1(NODE_21, 1,		/* base of Q7 */
			MONTECAR_MOTOR_DATA,	/* IC H8, pins 5, 2, 9, 6 */
			3.4,			/* TTL ON level */
			&montecar_motor_v_dac)
	DISCRETE_555_CC(NODE_22, 1,	/* IC C9 pin 9 */
			NODE_21,	/* vIn */
			NODE_20,	/* current adjust */
			1.e-8,		/* C81 */
			1000000, 0, 0,	/* R86, no rGnd, no rDis */
			&montecar_motor_vco)
	DISCRETE_COUNTER(NODE_23, 1, MONTECAR_ATTRACT_EN,	/* IC B/C9, QB-QD */
			NODE_22,				/* from IC C9, pin 9 */
			5, 1, 0, 1)				/* /6 counter on rising edge */
	DISCRETE_TRANSFORM2(NODE_24, 1, NODE_23, 2, "01>")	/* IC B/C9, pin 8-QD */
	DISCRETE_TRANSFORM3(NODE_25, 1, NODE_23, 1, 4, "01=02=|")	/* IC B/C9, pin 11-QB */
	DISCRETE_LOGIC_XOR(NODE_26, 1, NODE_24, NODE_25)	/* Gate A9, pin 11 */
	DISCRETE_COUNTER(NODE_27, 1, MONTECAR_ATTRACT_EN,	/* IC B/C9, pin 12-QA */
			NODE_26,					/* from IC A9, pin 11 */
			1, 1, 0, 1)					/* /2 counter on rising edge */
	DISCRETE_TRANSFORM5(NODE_28, 1, NODE_27, NODE_25, NODE_24, 2, 4, "13*24*+0+")	/* Mix the mess together in binary */
	DISCRETE_DAC_R1(MONTECAR_MOTORSND, 1, NODE_28,
			3.4,				/* TTL ON level */
			&montecar_motor_out_dac)

	/************************************************/
	/* Drone motor sound is basically the same as   */
	/* the regular car but with a volume control.   */
	/* Also I shifted the frequencies up for it to  */
	/* sound different from the player's car.       */
	/************************************************/
	DISCRETE_COMP_ADDER(NODE_30, 1, MONTECAR_DRONE_LOUD_DATA, &montecar_drone_vol_res)	/* make sure to change the node value in the mixer table if you change this node number */

	DISCRETE_ADJUSTMENT(NODE_40, 1,
				10000,	/* R85 + R88 @ min */
				260000,	/* R85 + R88 @ max */
				DISC_LOGADJ, 9)
	DISCRETE_DAC_R1(NODE_41, 1,		/* base of Q7 */
			MONTECAR_DRONE_MOTOR_DATA,	/* IC H8, pins 19, 16, 12, 15 */
			3.4,			/* TTL ON level */
			&montecar_motor_v_dac)
	DISCRETE_555_CC(NODE_42, 1,	/* IC C9 pin 5 */
			NODE_41,	/* vIn */
			NODE_40,	/* current adjust */
			1.e-8,		/* C80 */
			1000000, 0, 0,	/* R81, no rGnd, no rDis */
			&montecar_motor_vco)
	DISCRETE_COUNTER(NODE_43, 1, MONTECAR_ATTRACT_EN,	/* IC A/B9, QB-QD */
			NODE_42,				/* from IC C9, pin 5 */
			5, 1, 0, 1)				/* /6 counter on rising edge */
	DISCRETE_TRANSFORM2(NODE_44, 1, NODE_43, 2, "01>")	/* IC A/B9, pin 8-QD */
	DISCRETE_TRANSFORM3(NODE_45, 1, NODE_43, 1, 4, "01=02=|")	/* IC A/B9, pin 11-QB */
	DISCRETE_LOGIC_XOR(NODE_46, 1, NODE_44, NODE_45)	/* Gate A9, pin 6 */
	DISCRETE_COUNTER(NODE_47, 1, MONTECAR_ATTRACT_EN,	/* IC A/B9, pin 12-QA */
			NODE_46,				/* from IC A9, pin 6 */
			1, 1, 0, 1)				/* /2 counter on rising edge */
	DISCRETE_TRANSFORM5(NODE_48, 1, NODE_47, NODE_45, NODE_44, 2, 4, "13*24*+0+")	/* Mix the mess together in binary */
	DISCRETE_DAC_R1(MONTECAR_DRONE_MOTORSND, 1, NODE_48,
			3.4,				/* TTL ON level */
			&montecar_motor_out_dac)

	/************************************************/
	/* Crash circuit is built around a noise        */
	/* generator built from 2 shift registers that  */
	/* are clocked by the 2V signal.                */
	/* 2V = HSYNC/4                                 */
	/*    = 15750/4                                 */
	/* Output is binary weighted with 4 bits of     */
	/* crash volume.                                */
	/*                                              */
	/* TO DO: add filtering !!!!!!!!!!!!!!!!!!!!!!! */
	/************************************************/
	DISCRETE_LFSR_NOISE(MONTECAR_NOISE, MONTECAR_ATTRACT_INV, MONTECAR_ATTRACT_INV, 15750.0/4, 1.0, 0, 0.5, &firetrk_lfsr)	/* Same as firetrk */

	DISCRETE_SWITCH(NODE_50, 1, MONTECAR_NOISE, 0,	/* Enable gate A9 */
			MONTECAR_CRASH_DATA)		/* IC J8, pins 3,6,11,14 */
	DISCRETE_DAC_R1(MONTECAR_BANGSND, 1,	/* Bang */
			NODE_50,	/* from enable gates A9 */
			3.4,		/* TTL ON level */
			&montecar_bang_dac)

	/************************************************/
	/* Screech is just the noise modulating a       */
	/* Schmitt VCO.                                 */
	/************************************************/
	DISCRETE_SCHMITT_OSCILLATOR(NODE_60, MONTECAR_SKID_EN, MONTECAR_NOISE, 3.4, &montecar_screech_osc)
	DISCRETE_SWITCH(MONTECAR_SCREECHSND, 1, MONTECAR_ATTRACT_INV, 0, NODE_60)

	/************************************************/
	/* Beep circuit is just the 8V signal           */
	/* 8V = HSYNC/16                                */
	/*    = 15750/16                                */
	/************************************************/
	DISCRETE_SQUAREWFIX(MONTECAR_BEEPSND, MONTECAR_BEEPER_EN, 15750.0/16, 3.4, 50.0, 3.4/2, 0.0)

	/************************************************/
	/* Combine all 5 sound sources.                 */
	/************************************************/
	DISCRETE_MIXER5(NODE_90, 1, MONTECAR_BEEPSND, MONTECAR_SCREECHSND, MONTECAR_BANGSND, MONTECAR_DRONE_MOTORSND, MONTECAR_MOTORSND, &montecar_mixer)
	DISCRETE_OUTPUT(NODE_90, 100)
DISCRETE_SOUND_END
