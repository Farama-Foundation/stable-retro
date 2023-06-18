/*************************************************************************

	sndhrdw\triplhnt.c

*************************************************************************/
#include "driver.h"
#include "triplhnt.h"


static const char *triplhnt_sample_names[] =
{
	"*triplhnt",
	"bear_rac.wav",
	"witch.wav",
	0
};

struct Samplesinterface triplhnt_samples_interface =
{
	2,	/* 2 channels */
	20,	/* volume */
	triplhnt_sample_names
};


/************************************************************************/
/* triplhnt Sound System Analog emulation                               */
/* Feb 2004, Derrick Renaud                                             */
/************************************************************************/
const struct discrete_lfsr_desc triplhnt_lfsr={
	16,			/* Bit Length */
	0,			/* Reset Value */
	0,			/* Use Bit 0 as XOR input 0 */
	14,			/* Use Bit 14 as XOR input 1 */
	DISC_LFSR_XNOR,		/* Feedback stage1 is XNOR */
	DISC_LFSR_OR,		/* Feedback stage2 is just stage 1 output OR with external feed */
	DISC_LFSR_REPLACE,	/* Feedback stage3 replaces the shifted register contents */
	0x000001,		/* Everything is shifted into the first bit only */
	0,			/* Output not inverted */
	15			/* Output bit */
};

const struct discrete_dac_r1_ladder triplhnt_bear_roar_v_dac =
{
	4,		/* size of ladder */
	{1000000, 470000, 220000, 2200000, 0,0,0,0},	/* R47, R50, R48, R51 */
	5,		/* vBias */
	68000,	/* R44 */
	0,		/* no rGnd */
	0		/* no smoothing cap */
};

const struct discrete_dac_r1_ladder triplhnt_bear_roar_out_dac =
{
	3,		/* size of ladder */
	{100000, 33000, 100000, 0,0,0,0,0},	/* R56, R58, R57 */
	0,		/* no vBias */
	0,		/* no rBias */
	0,		/* no rGnd */
	0		/* no smoothing cap */
};

const struct discrete_dac_r1_ladder triplhnt_shot_dac =
{
	4,		/* size of ladder */
	{8200, 3900, 2200, 1000, 0,0,0,0},	/* R53, R54, R55, R52 */
	0,		/* no vBias */
	0,		/* no rBias */
	0,		/* no rGnd */
	0		/* no smoothing cap */
};

const struct discrete_555_cc_desc triplhnt_bear_roar_vco =
{
	DISC_555_OUT_DC | DISC_555_OUT_SQW,
	5,		/* B+ voltage of 555 */
	5.0 - 1.7,	/* High output voltage of 555 (Usually v555 - 1.7) */
	5.0 * 2 / 3,	/* threshold */
	5.0 /3,	/* trigger */
	5,		/* B+ voltage of the Constant Current source */
	0.7		/* Q2 junction voltage */
};

const struct discrete_schmitt_osc_desc triplhnt_screech_osc =
{
	2200,	/* R84 */
	330,	/* R85 */
	1.e-6,	/* C59 */
	1.7,	/* Rise Threshold of 7414 */
	0.9,	/* Fall Threshold of 7414 */
	3.4,	/* Output high voltage of 7414 */
	1		/* invert output using 7400 gate E7 */
};

const struct discrete_mixer_desc triplhnt_mixer =
{
	DISC_MIXER_IS_RESISTOR,
	3,					/* 3 inputs */
	{3300 + 19879.5, 47000, 27000 + 545.6, 0,0,0,0,0},	/* R59 + (R56||R57||R58), R60 + (R52||R53||R54||R55), R61 */
	{0,0,0,0,0,0,0,0},		/* No variable resistor nodes */
	{0, 0, 0, 0, 0,0,0,0},	/* No caps */
	0,						/* No rI */
	1000,					/* R78 */
	1e-7,					/* C72 */
	1e-7,					/* C44 */
	0,						/* vBias not used for resistor network */
	150000
};

/* Nodes - Sounds */
#define TRIPLHNT_NOISE			NODE_10
#define TRIPLHNT_BEAR_ROAR_SND	NODE_11
#define TRIPLHNT_SHOT_SND		NODE_12
#define TRIPLHNT_SCREECH_SND	NODE_13
#define POOLSHRK_SCORE_SND		NODE_14

DISCRETE_SOUND_START(triplhnt_discrete_interface)
	/************************************************/
	/* Input register mapping for triplhnt          */
	/************************************************/
	/*              NODE                    ADDR  MASK     GAIN      OFFSET  INIT */
	DISCRETE_INPUT(TRIPLHNT_BEAR_ROAR_DATA, 0x00, 0x000f,                    15.0)
	DISCRETE_INPUT(TRIPLHNT_BEAR_EN,        0x01, 0x000f,                     1.0)
	DISCRETE_INPUT(TRIPLHNT_SHOT_DATA,      0x02, 0x000f,                     0.0)	/* should init to 15 when hooked up */
	DISCRETE_INPUT(TRIPLHNT_SCREECH_EN,     0x03, 0x000f,                     0.0)
	DISCRETE_INPUT(TRIPLHNT_LAMP_EN,        0x04, 0x000f,                     1.0)
	/************************************************/

	DISCRETE_LFSR_NOISE(TRIPLHNT_NOISE,			/* Output A7 pin 13 */
				TRIPLHNT_LAMP_EN, TRIPLHNT_LAMP_EN,	/* From gate A8 pin 10 */
				12096000.0/2/256,		/* 256H signal */
				3.4, 0, 3.4/2, &triplhnt_lfsr)	/* TTL voltage level */

	/************************************************/
	/* Bear Roar is a VCO with noise mixed in.      */
	/************************************************/
	DISCRETE_ADJUSTMENT(NODE_20, 1,
				10000,	/* R86 + R88 @ 0 */
				260000,	/* R86 + R88 @ max */
				DISC_LOGADJ, 10)
	DISCRETE_DAC_R1(NODE_21, 1,			/* base of Q2 */
			TRIPLHNT_BEAR_ROAR_DATA,	/* IC B10, Q0-Q3 */
			3.4,				/* TTL ON level */
			&triplhnt_bear_roar_v_dac)
	DISCRETE_555_CC(NODE_22, 1,	/* IC C11 pin 3, always enabled */
			NODE_21,	/* vIn */
			NODE_20,	/* current adjust */
			1.e-8,		/* C58 */
			0, 390000, 0,	/* no rBias, R87, no rDis */
			&triplhnt_bear_roar_vco)
	DISCRETE_COUNTER(NODE_23, 1, TRIPLHNT_BEAR_EN,	/* IC B6, QB-QD */
			NODE_22,			/* from IC C11, pin 3 */
			5, 1, 0, 1)			/* /6 counter on rising edge */
	DISCRETE_TRANSFORM2(NODE_24, 1, NODE_23, 2, "01>")	/* IC B6, pin 8 */
	DISCRETE_LOGIC_INVERT(NODE_25, 1, NODE_22)	/* IC D9, pin 3 */
	DISCRETE_LOGIC_NAND(NODE_26, 1, NODE_25, TRIPLHNT_NOISE)	/* IC D9, pin 11 */
	DISCRETE_LOGIC_XOR(NODE_27, 1, NODE_24, NODE_26)	/* IC B8, pin 6 */
	DISCRETE_COUNTER(NODE_28, 1, TRIPLHNT_BEAR_EN,	/* IC B6, pin 12 */
			NODE_27,	/* from IC B8, pin 6 */
			1, 1, 0, 1)	/* /2 counter on rising edge */
	DISCRETE_TRANSFORM5(NODE_29, 1, NODE_24, NODE_28, NODE_26, 2, 4, "13*24*+0+")	/* Mix the mess together in binary */
	DISCRETE_DAC_R1(TRIPLHNT_BEAR_ROAR_SND, 1, NODE_29,
			3.4,				/* TTL ON level */
			&triplhnt_bear_roar_out_dac)

	/************************************************/
	/* Shot is just the noise amplitude modulated   */
	/* by an R1 DAC.                                */
	/************************************************/
	DISCRETE_SWITCH(NODE_40, 1,	/* Gate A9, pins 6, 8, 11, 3 */
			TRIPLHNT_NOISE,	/* noise enables the data which is then inverted */
			1, TRIPLHNT_SHOT_DATA)	/* the data has been previously inverted for ease of use */
	DISCRETE_DAC_R1(TRIPLHNT_SHOT_SND, 1,
			NODE_40,
			3.4,			/* TTL ON level */
			&triplhnt_shot_dac)

	/************************************************/
	/* Screech is just the noise modulating a       */
	/* Schmitt VCO.                                 */
	/************************************************/
	DISCRETE_SCHMITT_OSCILLATOR(TRIPLHNT_SCREECH_SND, TRIPLHNT_SCREECH_EN, TRIPLHNT_NOISE, 3.4, &triplhnt_screech_osc)

	/************************************************/
	/* Final gain and ouput.                        */
	/************************************************/
	DISCRETE_MIXER3(NODE_90, 1, TRIPLHNT_BEAR_ROAR_SND, TRIPLHNT_SHOT_SND, TRIPLHNT_SCREECH_SND, &triplhnt_mixer)
	DISCRETE_OUTPUT(NODE_90, 100)
DISCRETE_SOUND_END
