/* 8080bw.c *********************************
 updated: 1997-04-09 08:46 TT
 updated  20-3-1998 LT Added color changes on base explosion
 *
 * Author      : Tormod Tjaberg
 * Created     : 1997-04-09
 * Description : Sound routines for the 'invaders' games
 *
 * Note:
 * The samples were taken from Michael Strutt's (mstrutt@pixie.co.za)
 * excellent space invader emulator and converted to signed samples so
 * they would work under SEAL. The port info was also gleaned from
 * his emulator. These sounds should also work on all the invader games.
 *
 * The sounds are generated using output port 3 and 5
 *
 * Port 3:
 * bit 0=UFO  (repeats)       emulated
 * bit 1=Shot                 1.raw
 * bit 2=Base hit             2.raw
 * bit 3=Invader hit          3.raw
 * bit 4=Bonus base           9.raw
 *
 * Port 5:
 * bit 0=Fleet movement 1     4.raw
 * bit 1=Fleet movement 2     5.raw
 * bit 2=Fleet movement 3     6.raw
 * bit 3=Fleet movement 4     7.raw
 * bit 4=UFO 2                8.raw
 */
#include "driver.h"
#include "cpu/i8039/i8039.h"
#include "machine/74123.h"
#include "vidhrdw/generic.h"
#include "8080bw.h"

static WRITE_HANDLER( invad2ct_sh_port1_w );
static WRITE_HANDLER( invaders_sh_port3_w );
static WRITE_HANDLER( invaders_sh_port5_w );
static WRITE_HANDLER( invad2ct_sh_port7_w );

static WRITE_HANDLER( lrescue_sh_port3_w );
static WRITE_HANDLER( lrescue_sh_port5_w );

static WRITE_HANDLER( indianbt_sh_port3_w );
static WRITE_HANDLER( indianbt_sh_port5_w );

static WRITE_HANDLER( lupin3_sh_port3_w );
static WRITE_HANDLER( lupin3_sh_port5_w );

static WRITE_HANDLER( invrvnge_sh_port3_w );
static WRITE_HANDLER( invrvnge_sh_port5_w );

static WRITE_HANDLER( yosakdon_sh_port3_w );
static WRITE_HANDLER( yosakdon_sh_port5_w );

static WRITE_HANDLER( rollingc_sh_port0_w );

static WRITE_HANDLER( astropal_sh_port3_w );

static WRITE_HANDLER( galactic_sh_port3_w );
static WRITE_HANDLER( galactic_sh_port5_w );

static WRITE_HANDLER( ballbomb_sh_port3_w );
static WRITE_HANDLER( ballbomb_sh_port5_w );

static WRITE_HANDLER( boothill_sh_port3_w );
static WRITE_HANDLER( boothill_sh_port5_w );

static WRITE_HANDLER( clowns_sh_port7_w );
extern struct Samplesinterface circus_samples_interface;

static WRITE_HANDLER( seawolf_sh_port5_w );

static WRITE_HANDLER( schaser_sh_port3_w );
static WRITE_HANDLER( schaser_sh_port5_w );
static int  schaser_sh_start(const struct MachineSound *msound);
static void schaser_sh_stop(void);
static void schaser_sh_update(void);

static WRITE_HANDLER( polaris_sh_port2_w );
static WRITE_HANDLER( polaris_sh_port4_w );
static WRITE_HANDLER( polaris_sh_port6_w );


struct SN76477interface invaders_sn76477_interface =
{
  1,                   /* 1 chip                         */
  { 25 },              /* mixing level pin description   */
  { 0   /* N/C */},    /* 4   noise_res                  */
  { 0   /* N/C */},    /* 5   filter_res                 */
  { 0   /* N/C */},    /* 6   filter_cap                 */
  { 0   /* N/C */},    /* 7   decay_res	                 */
  { 0   /* N/C */},    /* 8   attack_decay_cap           */
  { RES_K(100)   },    /* 10  attack_res                 */
  { RES_K(56)    },    /* 11  amplitude_res              */
  { RES_K(10)    },    /* 12  feedback_res               */
  { 0   /* N/C */},    /* 16  vco_voltage                */
  { CAP_U(0.1)   },    /* 17  vco_cap                    */
  { RES_K(8.2)   },    /* 18  vco_res                    */
  { 5.0	         },    /* 19  pitch_voltage              */
  { RES_K(120)   },    /* 20  slf_res                    */
  { CAP_U(1.0)   },    /* 21  slf_cap                    */
  { 0   /* N/C */},    /* 23  oneshot_cap                */
  { 0   /* N/C */}     /* 24  oneshot_res                */
};

static const char *invaders_sample_names[] =
{
	"*invaders",
	"1.wav",	/* Shot/Missle */
	"2.wav",	/* Base Hit/Explosion */
	"3.wav",	/* Invader Hit */
	"4.wav",	/* Fleet move 1 */
	"5.wav",	/* Fleet move 2 */
	"6.wav",	/* Fleet move 3 */
	"7.wav",	/* Fleet move 4 */
	"8.wav",	/* UFO/Saucer Hit */
	"9.wav",	/* Bonus Base */
	0       /* end of array */
};

struct Samplesinterface invaders_samples_interface =
{
	4,	/* 4 channels */
	25,	/* volume */
	invaders_sample_names
};


struct SN76477interface invad2ct_sn76477_interface =
{
	2,	/* 2 chips */
	{ 25,         25 },  /* mixing level   pin description		 */
	{ 0,          0	/* N/C */  },	/*	4  noise_res		 */
	{ 0,          0	/* N/C */  },	/*	5  filter_res		 */
	{ 0,          0	/* N/C */  },	/*	6  filter_cap		 */
	{ 0,          0	/* N/C */  },	/*	7  decay_res		 */
	{ 0,          0	/* N/C */  },	/*	8  attack_decay_cap  */
	{ RES_K(100), RES_K(100)   },	/* 10  attack_res		 */
	{ RES_K(56),  RES_K(56)    },	/* 11  amplitude_res	 */
	{ RES_K(10),  RES_K(10)    },	/* 12  feedback_res 	 */
	{ 0,          0	/* N/C */  },	/* 16  vco_voltage		 */
	{ CAP_U(0.1), CAP_U(0.047) },	/* 17  vco_cap			 */
	{ RES_K(8.2), RES_K(39)    },	/* 18  vco_res			 */
	{ 5.0,        5.0          },	/* 19  pitch_voltage	 */
	{ RES_K(120), RES_K(120)   },	/* 20  slf_res			 */
	{ CAP_U(1.0), CAP_U(1.0)   },	/* 21  slf_cap			 */
	{ 0,          0	/* N/C */  },	/* 23  oneshot_cap		 */
	{ 0,          0	/* N/C */  }	/* 24  oneshot_res		 */
};

static const char *invad2ct_sample_names[] =
{
	"*invaders",
	"1.wav",	/* Shot/Missle - Player 1 */
	"2.wav",	/* Base Hit/Explosion - Player 1 */
	"3.wav",	/* Invader Hit - Player 1 */
	"4.wav",	/* Fleet move 1 - Player 1 */
	"5.wav",	/* Fleet move 2 - Player 1 */
	"6.wav",	/* Fleet move 3 - Player 1 */
	"7.wav",	/* Fleet move 4 - Player 1 */
	"8.wav",	/* UFO/Saucer Hit - Player 1 */
	"9.wav",	/* Bonus Base - Player 1 */
	"11.wav",	/* Shot/Missle - Player 2 */
	"12.wav",	/* Base Hit/Explosion - Player 2 */
	"13.wav",	/* Invader Hit - Player 2 */
	"14.wav",	/* Fleet move 1 - Player 2 */
	"15.wav",	/* Fleet move 2 - Player 2 */
	"16.wav",	/* Fleet move 3 - Player 2 */
	"17.wav",	/* Fleet move 4 - Player 2 */
	"18.wav",	/* UFO/Saucer Hit - Player 2 */
	0       /* end of array */
};

struct Samplesinterface invad2ct_samples_interface =
{
	8,	/* 8 channels */
	25,	/* volume */
	invad2ct_sample_names
};


MACHINE_INIT( invaders )
{
	install_port_write_handler(0, 0x03, 0x03, invaders_sh_port3_w);
	install_port_write_handler(0, 0x05, 0x05, invaders_sh_port5_w);

	SN76477_envelope_1_w(0, 1);
	SN76477_envelope_2_w(0, 0);
	SN76477_mixer_a_w(0, 0);
	SN76477_mixer_b_w(0, 0);
	SN76477_mixer_c_w(0, 0);
	SN76477_vco_w(0, 1);
}

MACHINE_INIT( sstrangr )
{
	install_port_write_handler(0, 0x42, 0x42, invaders_sh_port3_w);
	install_port_write_handler(0, 0x44, 0x44, invaders_sh_port5_w);

	SN76477_envelope_1_w(0, 1);
	SN76477_envelope_2_w(0, 0);
	SN76477_mixer_a_w(0, 0);
	SN76477_mixer_b_w(0, 0);
	SN76477_mixer_c_w(0, 0);
	SN76477_vco_w(0, 1);
}

MACHINE_INIT( invad2ct )
{
	machine_init_invaders();

	install_port_write_handler(0, 0x01, 0x01, invad2ct_sh_port1_w);
	install_port_write_handler(0, 0x07, 0x07, invad2ct_sh_port7_w);

	SN76477_envelope_1_w(1, 1);
	SN76477_envelope_2_w(1, 0);
	SN76477_mixer_a_w(1, 0);
	SN76477_mixer_b_w(1, 0);
	SN76477_mixer_c_w(1, 0);
	SN76477_vco_w(1, 1);
}


/*
   Note: For invad2ct, the Player 1 sounds are the same as for the
         original and deluxe versions.  Player 2 sounds are all
         different, and are triggered by writes to port 1 and port 7.

*/

static void invaders_sh_1_w(int board, int data, unsigned char *last)
{
	int base_channel, base_sample;

	base_channel = 4 * board;
	base_sample  = 9 * board;

	SN76477_enable_w(board, !(data & 0x01));				/* Saucer Sound */

	if (data & 0x02 && ~*last & 0x02)
		sample_start (base_channel+0, base_sample+0, 0);	/* Shot Sound */

	if (data & 0x04 && ~*last & 0x04)
		sample_start (base_channel+1, base_sample+1, 0);	/* Base Hit */

	if (~data & 0x04 && *last & 0x04)
		sample_stop (base_channel+1);

	if (data & 0x08 && ~*last & 0x08)
		sample_start (base_channel+0, base_sample+2, 0);	/* Invader Hit */

	if (data & 0x10 && ~*last & 0x10)
		sample_start (base_channel+2, 8, 0);				/* Bonus Missle Base */

	c8080bw_screen_red_w(data & 0x04);

	*last = data;
}

static void invaders_sh_2_w(int board, int data, unsigned char *last)
{
	int base_channel, base_sample;

	base_channel = 4 * board;
	base_sample  = 9 * board;

	if (data & 0x01 && ~*last & 0x01)
		sample_start (base_channel+1, base_sample+3, 0);	/* Fleet 1 */

	if (data & 0x02 && ~*last & 0x02)
		sample_start (base_channel+1, base_sample+4, 0);	/* Fleet 2 */

	if (data & 0x04 && ~*last & 0x04)
		sample_start (base_channel+1, base_sample+5, 0);	/* Fleet 3 */

	if (data & 0x08 && ~*last & 0x08)
		sample_start (base_channel+1, base_sample+6, 0);	/* Fleet 4 */

	if (data & 0x10 && ~*last & 0x10)
		sample_start (base_channel+3, base_sample+7, 0);	/* Saucer Hit */

	c8080bw_flip_screen_w(data & 0x20);

	*last = data;
}


static WRITE_HANDLER( invad2ct_sh_port1_w )
{
	static unsigned char last = 0;

	invaders_sh_1_w(1, data, &last);
}

static WRITE_HANDLER( invaders_sh_port3_w )
{
	static unsigned char last = 0;

	invaders_sh_1_w(0, data, &last);
}

static WRITE_HANDLER( invaders_sh_port5_w )
{
	static unsigned char last = 0;

	invaders_sh_2_w(0, data, &last);
}

static WRITE_HANDLER( invad2ct_sh_port7_w )
{
	static unsigned char last = 0;

	invaders_sh_2_w(1, data, &last);
}


/*******************************************************/
/*                                                     */
/* Lunar Rescue                                        */
/*                                                     */
/*******************************************************/

struct SN76477interface lrescue_sn76477_interface =
{
  1,                   /* 1 chip                         */
  { 25 },              /* mixing level pin description   */
  { 0   /* N/C */},    /* 4   noise_res                  */
  { 0   /* N/C */},    /* 5   filter_res                 */
  { 0   /* N/C */},    /* 6   filter_cap                 */
  { 0   /* N/C */},    /* 7   decay_res	                 */
  { 0   /* N/C */},    /* 8   attack_decay_cap           */
  { RES_K(100)   },    /* 10  attack_res                 */
  { RES_K(56)    },    /* 11  amplitude_res              */
  { RES_K(10)    },    /* 12  feedback_res               */
  { 0   /* N/C */},    /* 16  vco_voltage                */
  { CAP_U(0.1)   },    /* 17  vco_cap                    */
  { RES_K(8.2)   },    /* 18  vco_res                    */
  { 5.0	         },    /* 19  pitch_voltage              */
  { RES_K(120)   },    /* 20  slf_res                    */
  { CAP_U(1.0)   },    /* 21  slf_cap                    */
  { 0   /* N/C */},    /* 23  oneshot_cap                */
  { 0   /* N/C */}     /* 24  oneshot_res                */
};

static const char *lrescue_sample_names[] =
{
  "*lrescue",
  "alienexplosion",
  "rescueshipexplosion",
  "beamgun",
  "thrust",
  "bonus2",
  "bonus3",
  "shootingstar",
  "stepl",
  "steph",
  0
};

struct Samplesinterface lrescue_samples_interface =
{
  4,	/* 4 channels */
  25,	/* volume */
  lrescue_sample_names
};

MACHINE_INIT( lrescue )
{
  install_port_write_handler(0, 0x03, 0x03, lrescue_sh_port3_w);
  install_port_write_handler(0, 0x05, 0x05, lrescue_sh_port5_w);

  SN76477_envelope_1_w(0, 1);
  SN76477_envelope_2_w(0, 0);
  SN76477_mixer_a_w(0, 0);
  SN76477_mixer_b_w(0, 0);
  SN76477_mixer_c_w(0, 0);
  SN76477_vco_w(0, 1);
}

static void lrescue_sh_1_w(int board, int data, unsigned char *last)
{
  int base_channel, base_sample;

  base_channel = 4 * board;
  base_sample  = 9 * board;

  if (data & 0x01 && ~*last & 0x01) sample_start (base_channel+0, base_sample+3, 0);    /* thrust.wav  */
  if (data & 0x02 && ~*last & 0x02) sample_start (base_channel+1, base_sample+2, 0);    /* beamgun.wav  */
  if (data & 0x04 && ~*last & 0x04) sample_start (base_channel+0, base_sample+1, 0);    /* rescueshipexplosion.wav  */
  if (data & 0x08 && ~*last & 0x08) sample_start (base_channel+1, base_sample+0, 0);    /* alienexplosion.wav  */
  if (data & 0x10 && ~*last & 0x10) sample_start (base_channel+2, base_sample+5, 0);    /* bonus3.wav - bonus life */

  c8080bw_screen_red_w(data & 0x04);

  *last = data;
}

static void lrescue_sh_2_w(int board, int data, unsigned char *last)
{
  int base_channel, base_sample;

  base_channel = 4 * board;
  base_sample  = 9 * board;

  if (data & 0x01 && ~*last & 0x01) sample_start (base_channel+1, base_sample+8, 0);    /* steph.wav - foot step high  */
  if (data & 0x02 && ~*last & 0x02) sample_start (base_channel+1, base_sample+7, 0);    /* stepl.wav - foot step low  */
  if (data & 0x04 && ~*last & 0x04) sample_start (base_channel+1, base_sample+4, 0);    /* bonus2.wav - count men saved  */
  if (data & 0x08 && ~*last & 0x08) sample_start (base_channel+3, base_sample+4, 0);    /* bitstream tunes - end level and bonus 1  */
  if (data & 0x10 && ~*last & 0x10) sample_start (base_channel+3, base_sample+6, 0);    /* shootingstar.wav  */
  if (~data & 0x10 && *last & 0x10) sample_stop  (base_channel+3);                      /* This makes the rescue ship sound beep on and off  */

  c8080bw_flip_screen_w(data & 0x20);

  *last = data;
}

static WRITE_HANDLER( lrescue_sh_port3_w )
{
  static unsigned char last = 0;

  lrescue_sh_1_w(0, data, &last);
}

static WRITE_HANDLER( lrescue_sh_port5_w )
{
  static unsigned char last = 0;

  lrescue_sh_2_w(0, data, &last);
}

/*******************************************************/
/*                                                     */
/* Midway "Gun Fight"                                  */
/*                                                     */
/*******************************************************/

MACHINE_INIT( gunfight )
{
	install_port_read_handler(0, 0x00, 0x00, gunfight_port_0_r);
	install_port_read_handler(0, 0x01, 0x01, gunfight_port_1_r);
}


/*******************************************************/
/*                                                     */
/* Midway "Boot Hill"                                  */
/*                                                     */
/*******************************************************/

static const char *boothill_sample_names[] =
{
	"*boothill", /* in case we ever find any bootlegs hehehe */
	"addcoin.wav",
	"endgame.wav",
	"gunshot.wav",
	"killed.wav",
	0       /* end of array */
};

struct Samplesinterface boothill_samples_interface =
{
	9,	/* 9 channels */
	25,	/* volume */
	boothill_sample_names
};


/* HC 4/14/98 NOTE: *I* THINK there are sounds missing...
i dont know for sure... but that is my guess....... */

MACHINE_INIT( boothill )
{
	install_port_read_handler (0, 0x00, 0x00, boothill_port_0_r);
	install_port_read_handler (0, 0x01, 0x01, boothill_port_1_r);

	install_port_write_handler(0, 0x03, 0x03, boothill_sh_port3_w);
	install_port_write_handler(0, 0x05, 0x05, boothill_sh_port5_w);
}

static WRITE_HANDLER( boothill_sh_port3_w )
{
	switch (data)
	{
		case 0x0c:
			sample_start (0, 0, 0);
			break;

		case 0x18:
		case 0x28:
			sample_start (1, 2, 0);
			break;

		case 0x48:
		case 0x88:
			sample_start (2, 3, 0);
			break;
	}
}

/* HC 4/14/98 */
static WRITE_HANDLER( boothill_sh_port5_w )
{
	switch (data)
	{
		case 0x3b:
			sample_start (2, 1, 0);
			break;
	}
}


/*******************************************************/
/*                                                     */
/* Taito "Balloon Bomber"                              */
/*                                                     */
/*******************************************************/

/* This only does the color swap for the explosion */
/* We do not have correct samples so sound not done */

MACHINE_INIT( ballbomb )
{
	install_port_write_handler(0, 0x03, 0x03, ballbomb_sh_port3_w);
	install_port_write_handler(0, 0x05, 0x05, ballbomb_sh_port5_w);
}

static WRITE_HANDLER( ballbomb_sh_port3_w )
{
	static unsigned char last = 0;
	UINT8 rising_bits = data & ~last;

	if (rising_bits & 0x01) sample_start(1, 2, 0);		/* Hit a balloon */
	if (rising_bits & 0x02) sample_start(2, 0, 0);		/* Shot */
	if (rising_bits & 0x04) sample_start(2, 1, 0);		/* Base hit */
	if (rising_bits & 0x08) sample_start(1, 7, 0);		/* Hit a bomb */
	if (rising_bits & 0x10) sample_start(3, 8, 0);		/* Bonus base at 1500 points */

	mixer_sound_enable_global_w(data & 0x20);
	c8080bw_screen_red_w(data & 0x04);

	last = data;
}

static WRITE_HANDLER( ballbomb_sh_port5_w )
{
	static unsigned char last = 0;
	UINT8 rising_bits = data & ~last;

	if (data & 0x01) sample_start(0, 7, 0);		/* Indicates plane will drop bombs */
	if (data & 0x04) sample_start(0, 4, 0);		/* Plane is dropping new balloons at start of level */
	if (rising_bits & 0x10) sample_start(2, 2, 0);		/* Balloon hit and bomb drops */

	c8080bw_flip_screen_w(data & 0x20);

	last = data;
}


/*******************************************************/
/*                                                     */
/* Taito "Polaris"		                       */
/*                                                     */
/*******************************************************/

const struct discrete_lfsr_desc polaris_lfsr={
	17,			/* Bit Length */
	0,			/* Reset Value */
	4,			/* Use Bit 4 as XOR input 0 */
	16,			/* Use Bit 16 as XOR input 1 */
	DISC_LFSR_XOR,		/* Feedback stage1 is XOR */
	DISC_LFSR_OR,		/* Feedback stage2 is just stage 1 output OR with external feed */
	DISC_LFSR_REPLACE,	/* Feedback stage3 replaces the shifted register contents */
	0x000001,		/* Everything is shifted into the first bit only */
	0,			/* Output is not inverted */
	12			/* Output bit */
};

DISCRETE_SOUND_START(polaris_sound_interface)

/* Nodes - Inputs */
#define POLARIS_MUSIC_DATA	NODE_01
#define POLARIS_SX0_EN		NODE_02
#define POLARIS_SX1_EN		NODE_03
#define POLARIS_SX2_EN		NODE_04
#define POLARIS_SX3_EN		NODE_05
#define POLARIS_SX5_EN		NODE_06
#define POLARIS_SX6_EN		NODE_07
#define POLARIS_SX7_EN		NODE_08
#define POLARIS_SX9_EN		NODE_09
#define POLARIS_SX10_EN		NODE_10
/* Nodes - Sounds */
#define POLARIS_MUSIC		NODE_11
#define POLARIS_NOISE_LO	NODE_12
#define POLARIS_NOISE_LO_FILT	NODE_13
#define POLARIS_NOISE_HI_FILT	NODE_14
#define POLARIS_SHOTSND		NODE_15
#define POLARIS_SHIP_HITSND	NODE_16
#define POLARIS_SHIPSND		NODE_17
#define POLARIS_EXPLOSIONSND	NODE_18
#define POLARIS_PLANESND	NODE_19
#define POLARIS_HITSND		NODE_20
#define POLARIS_SONARSND	NODE_21
#define POLARIS_FINAL_MIX	NODE_22
/* Nodes - Adjust */
#define POLARIS_ADJ_VR1		NODE_97
#define POLARIS_ADJ_VR2		NODE_98
#define POLARIS_ADJ_VR3		NODE_99

	/************************************************/
	/* Polaris sound system: 8 Sound Sources        */
	/*                                              */
	/* Relative volumes are adjustable              */
	/*                                              */
	/*  Discrete sound mapping via:                 */
	/*     discrete_sound_w($register,value)        */
	/*  $00 - Music Data                            */
	/*  $01 - SX0                                   */
	/*  $02 - SX1                                   */
	/*  $03 - SX2                                   */
	/*  $04 - SX3                                   */
	/*  $05 - SX5                                   */
	/*  $06 - SX6                                   */
	/*  $07 - SX7                                   */
	/*  $08 - SX9                                   */
	/*  $09 - SX10                                  */
	/*                                              */
	/************************************************/

	/************************************************/
	/* Input register mapping for polaris           */
	/************************************************/
	/*               NODE                ADDR  MASK    INIT */
	DISCRETE_INPUT(POLARIS_MUSIC_DATA  , 0x00, 0x000f, 0.0)
	DISCRETE_INPUT(POLARIS_SX0_EN      , 0x01, 0x000f, 0.0)
	DISCRETE_INPUT(POLARIS_SX1_EN      , 0x02, 0x000f, 0.0)
	DISCRETE_INPUT(POLARIS_SX2_EN      , 0x03, 0x000f, 0.0)
	DISCRETE_INPUT(POLARIS_SX3_EN      , 0x04, 0x000f, 0.0)
	DISCRETE_INPUT(POLARIS_SX5_EN      , 0x05, 0x000f, 0.0)
	DISCRETE_INPUT(POLARIS_SX6_EN      , 0x06, 0x000f, 0.0)
	DISCRETE_INPUT(POLARIS_SX7_EN      , 0x07, 0x000f, 0.0)
	DISCRETE_INPUT(POLARIS_SX9_EN      , 0x08, 0x000f, 0.0)
	DISCRETE_INPUT(POLARIS_SX10_EN     , 0x09, 0x000f, 0.0)

	DISCRETE_ADJUSTMENT(POLARIS_ADJ_VR1, 1, 0, 1000, DISC_LINADJ, 4)
	DISCRETE_ADJUSTMENT(POLARIS_ADJ_VR2, 1, 0, 1000, DISC_LINADJ, 5)
	DISCRETE_ADJUSTMENT(POLARIS_ADJ_VR3, 1, 0, 1000, DISC_LINADJ, 6)

/******************************************************************************
 *
 * Music Generator
 *
 * This is a simulation of the following circuit:
 * 555 Timer (Ra = 1k, Rb = 1k, C =.01uF) running at 48kHz.  Connected to a
 * 1 bit counter (/2) for 24kHz.  But I will use the breadboarded frequency
 * of 44140Hz/2.
 * This is then sent to a preloadable 8 bit counter, which loads the value
 * from Port 2 when overflowing from 0xFF to 0x00.  Therefore it divides by
 * 2 (Port 2 = FE) to 256 (Port 2 = 00).
 * This goes to a 2 bit counter which has a 47k resistor on Q0 and a 12k on Q1.
 * This creates a sawtooth ramp of: 0%, 12/59%, 47/59%, 100% then back to 0%
 *
 * Note that there is no music disable line.  When there is no music, the game
 * sets the oscillator to 0Hz.  (Port 2 = FF)
 *
 ******************************************************************************/
	DISCRETE_ADDER2(NODE_23, 1, POLARIS_MUSIC_DATA, 1)	/* To get values of 1 - 256 */
	DISCRETE_DIVIDE(NODE_24, 1, 44140.0/2/2, NODE_23)	/* /2 counter frequency */
	DISCRETE_DIVIDE(NODE_25, 1, 44140.0/2/4, NODE_23)	/* /4 counter frequency */
	DISCRETE_SQUAREWAVE(NODE_26, 1, NODE_24, 12.0/59.0*0.7527, 50.0, 0, 0.0)	/* /2 */
	DISCRETE_SQUAREWAVE(NODE_27, 1, NODE_25, 47.0/59.0*0.7527, 50.0, 0, 0.0)	/* /4 */
	DISCRETE_ADDER2(NODE_28, POLARIS_MUSIC_DATA, NODE_26, NODE_27)	/* Port2=FF Disables audio */
	DISCRETE_MULTIPLY(NODE_29, 1, NODE_28, POLARIS_ADJ_VR3)	/* Basic Volume Adj */
	DISCRETE_RCFILTER(POLARIS_MUSIC, 1, NODE_29, 9559.0, 1.8e-10)

/******************************************************************************
 *
 * Noise sources
 *
 * The frequencies for the noise sources were breadboarded to
 * get an accurate value.
 *
 * Also we are going to cheat and reduce the gain of the HI noise a little to
 * compensate for the computer being too accurate.
 *
 ******************************************************************************/
	DISCRETE_SQUAREWFIX(NODE_30, 1, (60.0/512.0), 1, 0.01, 1.0/2, 359.0)	/* feed frequency to keep the noise going */

	DISCRETE_LFSR_NOISE(POLARIS_NOISE_LO, 1, 1, 830.0, 1.0, NODE_30, 0, &polaris_lfsr)  /* Unfiltered Lo noise */

	DISCRETE_RCFILTER(NODE_31, 1, POLARIS_NOISE_LO, 560.0, 2.2e-7)
	DISCRETE_RCFILTER(POLARIS_NOISE_LO_FILT, 1, NODE_31, 6800.0, 2.2e-7)	/* Filtered Lo noise */

	DISCRETE_LFSR_NOISE(NODE_32, 1, 1, 44140.0/2, 0.8, NODE_30, 0, &polaris_lfsr)
	DISCRETE_RCFILTER(NODE_33, 1, NODE_32, 560.0, 1.e-8)
	DISCRETE_RCFILTER(POLARIS_NOISE_HI_FILT, 1, NODE_33, 6800.0, 1.e-8)	/* Filtered Hi noise */

/******************************************************************************
 *
 * Background Sonar Sound
 *
 * This is a background sonar that plays at all times during the game.
 * It is a VCO triangle wave genarator, that uses the Low frequency filtered
 * noise source for the frequency.  It goes from 1 to 1/1.12 of it's VCO range.
 * This is then amplitude modulated, by some fancy clocking scheme.
 * It is disabled during SX3.  (No sonar when you die.)
 *
 ******************************************************************************/
	DISCRETE_MULTIPLY(NODE_74, 1, POLARIS_ADJ_VR1, 0.7527)	/* Basic Volume Adj * Relative Gain */

	DISCRETE_ADDER2(NODE_80, 1, POLARIS_NOISE_LO_FILT, 1.0/2)	/* Shift back to DC */
	DISCRETE_SQUAREWAVE(NODE_81, 1, 60.0/16.0, 1, 50.0, 1.0/2, 0)	/* Clock */
	DISCRETE_SQUAREWAVE(NODE_82, 1, 60.0/16.0/8.0, 1, 50.0, 1.0/2, 0)	/* Data */
	DISCRETE_LOGIC_OR(NODE_83, 1, NODE_82, POLARIS_SX3_EN)	/* OR for later enable */
	DISCRETE_SWITCH(NODE_84, 1, NODE_83, 0, NODE_74)	/* Overall gain */
	DISCRETE_SAMPLHOLD(NODE_85, 1, NODE_84, NODE_81, DISC_SAMPHOLD_REDGE)
	DISCRETE_SWITCH(NODE_86, 1, NODE_83, NODE_85, 0)/* Invert */
	DISCRETE_RCDISC2(NODE_87, NODE_85, NODE_86, 680000.0, NODE_86, 1000.0, 1e-6)	/* Decay envelope */
	DISCRETE_GAIN(NODE_88, NODE_80, 1800.0-(1800.0/1.12))
	DISCRETE_ADDER2(NODE_89, 1, NODE_88, 1800.0/1.12)
	DISCRETE_TRIANGLEWAVE(POLARIS_SONARSND, NODE_86, NODE_89, NODE_87, 0, 0)


/******************************************************************************
 *
 * Shot - SX0
 *
 * When Enabled it makes a low frequency RC filtered noise.  As soon as it
 * disables, it switches to a high frequency RC filtered noise with the volume
 * decaying based on the RC values of 680k and 1uF.
 *
 ******************************************************************************/
	DISCRETE_MULTIPLY(NODE_34, 1, POLARIS_SX0_EN, POLARIS_ADJ_VR1)	/* Basic Volume Adj */
	DISCRETE_MULTIPLY(NODE_35, 1, NODE_34, 0.6034)	/* Relative Gain */
	DISCRETE_RCDISC2(NODE_36, POLARIS_SX0_EN, NODE_35, 680000.0, NODE_35, 1000.0, 1e-6)	/* Decay envelope */

	DISCRETE_MULTIPLY(NODE_37, 1, POLARIS_NOISE_LO_FILT, NODE_36)	/* Amplify noise using envelope */
	DISCRETE_MULTIPLY(NODE_38, 1, POLARIS_NOISE_HI_FILT, NODE_36)	/* Amplify noise using envelope */

	DISCRETE_SWITCH(POLARIS_SHOTSND, 1, POLARIS_SX0_EN, NODE_38, NODE_37)

/******************************************************************************
 *
 * Ship Hit (Sub) - SX1
 *
 * When Enabled it makes a low frequency RC filtered noise.  As soon as it
 * disables, it's  volume starts decaying based on the RC values of 680k and
 * 1uF.  Also as it decays, a decaying high frequency RC filtered noise is
 * mixed in.
 *
 ******************************************************************************/
	DISCRETE_MULTIPLY(NODE_40, 1, POLARIS_SX1_EN, POLARIS_ADJ_VR2)	/* Basic Volume Adj */
	DISCRETE_MULTIPLY(NODE_41, 1, NODE_40, 0.4375)	/* Relative Gain */
	DISCRETE_RCDISC2(NODE_42, POLARIS_SX1_EN, NODE_41, 680000.0, NODE_41, 1000.0, 1e-6)	/* Decay envelope */

	DISCRETE_MULTIPLY(NODE_43, 1, POLARIS_NOISE_LO_FILT, NODE_42)	/* Amplify noise using envelope */
	DISCRETE_MULTIPLY(NODE_44, 1, POLARIS_NOISE_HI_FILT, NODE_42)	/* Amplify noise using envelope */

	DISCRETE_SWITCH(NODE_45, 1, POLARIS_SX1_EN, 0, NODE_44)
	DISCRETE_ADDER2(POLARIS_SHIP_HITSND, 1, NODE_43, NODE_45)

/******************************************************************************
 *
 * Ship - SX2
 *
 * This uses a 5.75Hz |\|\ sawtooth the modulate the frequency of a
 * voltage controlled triangle wave oscillator.  The voltage varies 10.5V - 1.5V
 * causing the frequency to change from 2040Hz to 245Hz with a rise percentage
 * changing from 60% to 68%.  Close enough to a 50% triangle to most ears.
 *
 ******************************************************************************/
	DISCRETE_SAWTOOTHWAVE(NODE_50, POLARIS_SX2_EN, 5.75, 2040.0-245.0, (2040.0-245.0)/2, 1, 0)	/* Modulation Wave */
	DISCRETE_ADDER2(NODE_51, 1, NODE_50, 245.0)
	DISCRETE_MULTIPLY(NODE_52, 1, POLARIS_ADJ_VR2, 0.6034)	/* Basic Volume Adj * Relative Gain */
	DISCRETE_TRIANGLEWAVE(POLARIS_SHIPSND, POLARIS_SX2_EN, NODE_51, NODE_52, 0, 0)

/******************************************************************************
 *
 * Explosion - SX3
 *
 * When Enabled it makes a low frequency noise.  As soon as it disables, it's
 * volume starts decaying based on the RC values of 680k and 0.33uF.  The
 * final output is RC filtered.
 *
 * Note that when this is triggered, the sonar sound clock is disabled.
 *
 ******************************************************************************/
	DISCRETE_GAIN(NODE_55, POLARIS_SX3_EN, 2000.0)
	DISCRETE_RCDISC2(NODE_56, POLARIS_SX3_EN, NODE_55, 680000.0, NODE_55, 1000.0, 3.3e-7)	/* Decay envelope */

	DISCRETE_MULTIPLY(NODE_57, 1, POLARIS_NOISE_LO, NODE_56)	/* Amplify noise using envelope */

	DISCRETE_RCFILTER(NODE_58, 1, NODE_57, 560.0, 2.2e-7)
	DISCRETE_RCFILTER(POLARIS_EXPLOSIONSND, 1, NODE_58, 6800.0, 2.2e-7)

/******************************************************************************
 *
 * Plane Down - SX6
 * Plane Up   - SX7
 *
 * The oscillator is enabled when SX7 goes high. When SX6 is enabled the
 * frequency lowers.  When SX6 is disabled the frequency ramps back up.
 * Also some NOISE_HI_FILT is mixed in so the frequency varies some.
 *
 ******************************************************************************/
	DISCRETE_RAMP(NODE_70, POLARIS_SX7_EN, POLARIS_SX6_EN, (3240.0-410.0)/1.2, 3240.0, 410.0, 0)
	DISCRETE_GAIN(NODE_71, POLARIS_NOISE_HI_FILT, 0.1)
	DISCRETE_MULTIPLY(NODE_72, 1, NODE_70, NODE_71)
/*	DISCRETE_MULTIPLY(NODE_72, 1, POLARIS_NOISE_LO, (3240.0-410.0)/10.0/2.0)*/
	DISCRETE_ADDER2(NODE_73, 1, NODE_70, NODE_72)	/* Add in 10% hi freq shift to final plane freq. */
	DISCRETE_TRIANGLEWAVE(POLARIS_PLANESND, POLARIS_SX7_EN, NODE_73, NODE_74, 0, 0)

/******************************************************************************
 *
 * HIT - SX9 & SX10
 *
 * SX9 seems to be never used, so I am not sure if the schematic is correct.
 * Following the schematic, 3 different sounds are produced.
 * SX10  SX9  Effect
 *  0     0   no sound
 *  0     1   NOISE_HI_FILT while enabled
 *  1     0   NOISE_LO_FILT while enabled
 *  1     1   NOISE_HI_FILT & NOISE_LO_FILT decaying imediately @ 680k, 0.22uF
 *
 ******************************************************************************/
	DISCRETE_LOGIC_NAND(NODE_60, 1, POLARIS_SX9_EN, POLARIS_SX10_EN)
	DISCRETE_SWITCH(NODE_61, 1, NODE_60, 0, POLARIS_ADJ_VR1)	/* Always on except when SX9 and SX10 enabled */
	DISCRETE_RCDISC2(NODE_62, NODE_61, NODE_61, 680000.0, NODE_61, 1000.0, 2.2e-7)
	DISCRETE_MULTIPLY(NODE_63, POLARIS_SX9_EN, POLARIS_NOISE_HI_FILT, NODE_62)
	DISCRETE_MULTIPLY(NODE_64, POLARIS_SX10_EN, POLARIS_NOISE_LO_FILT, NODE_62)
	DISCRETE_ADDER2(POLARIS_HITSND, 1, NODE_63, NODE_64)

/******************************************************************************
 *
 * Final Mixing and Output
 *
 ******************************************************************************/

	DISCRETE_ADDER4(NODE_90, 1, POLARIS_SHOTSND, POLARIS_HITSND, POLARIS_PLANESND, POLARIS_SONARSND)	/* VR1 */
	DISCRETE_ADDER2(NODE_91, 1, POLARIS_SHIP_HITSND, POLARIS_SHIPSND)					/* VR2 */
	DISCRETE_ADDER4(NODE_92, POLARIS_SX5_EN, NODE_90, NODE_91, POLARIS_MUSIC, POLARIS_EXPLOSIONSND)
	DISCRETE_GAIN(POLARIS_FINAL_MIX, NODE_92, 65534.0/3000.0)
	DISCRETE_OUTPUT(POLARIS_FINAL_MIX, 100)

DISCRETE_SOUND_END

MACHINE_INIT( polaris )
{
	install_port_write_handler(0, 0x02, 0x02, polaris_sh_port2_w);
	install_port_write_handler(0, 0x04, 0x04, polaris_sh_port4_w);
	install_port_write_handler(0, 0x06, 0x06, polaris_sh_port6_w);
}

static WRITE_HANDLER( polaris_sh_port2_w )
{
	discrete_sound_w(0, (~data) & 0xff);
}

static WRITE_HANDLER( polaris_sh_port4_w )
{
	/* 0x01 - SX0 - Shot */
	discrete_sound_w(1, data & 0x01);

	/* 0x02 - SX1 - Ship Hit (Sub) */
	discrete_sound_w(2, (data & 0x02) >> 1);

	/* 0x04 - SX2 - Ship */
	discrete_sound_w(3, (data & 0x04) >> 2 );

	/* 0x08 - SX3 - Explosion */
	discrete_sound_w(4, (data & 0x08) >> 3);

	/* 0x10 - SX4 */

	/* 0x20 - SX5 - Sound Enable */
	discrete_sound_w(5, (data & 0x20) >> 5);
}

static WRITE_HANDLER( polaris_sh_port6_w )
{
	coin_lockout_global_w(data & 0x04);  /* SX8 */

	c8080bw_flip_screen_w(data & 0x20);  /* SX11 */

	/* 0x01 - SX6 - Plane Down */
	discrete_sound_w(6, (data & 0x01) );

	/* 0x02 - SX7 - Plane Up */
	discrete_sound_w(7, (data & 0x02) >> 1 );

	/* 0x08 - SX9 - Hit */
	discrete_sound_w(9, (data & 0x08) >> 3);

	/* 0x10 - SX10 - Hit */
	discrete_sound_w(9, (data & 0x10) >> 4);
}


/*******************************************************/
/*                                                     */
/* Midway "Phantom II"		                           */
/*                                                     */
/*******************************************************/

MACHINE_INIT( phantom2 )
{
	install_port_write_handler(0, 0x04, 0x04, watchdog_reset_w);
}


/*******************************************************/
/*                                                     */
/* Midway "4 Player Bowling Alley"					   */
/*                                                     */
/*******************************************************/

MACHINE_INIT( bowler )
{
	install_port_write_handler(0, 0x04, 0x04, watchdog_reset_w);
	install_port_write_handler(0, 0x07, 0x07, bowler_bonus_display_w);
}


/*******************************************************/
/*                                                     */
/* Midway "Sea Wolf"                                   */
/*                                                     */
/*******************************************************/

static const char *seawolf_sample_names[] =
{
	"*seawolf",
	"shiphit.wav",
	"torpedo.wav",
	"dive.wav",
	"sonar.wav",
	"minehit.wav",
	0       /* end of array */
};

struct Samplesinterface seawolf_samples_interface =
{
	5,	/* 5 channels */
	25,	/* volume */
	seawolf_sample_names
};

MACHINE_INIT( seawolf )
{
/*  Lamp Display Output (write) Ports are as follows:

Port 1:
  Basically D0-D3 are column drivers and D4-D7 are row drivers.
  The folowing table shows values that light up individual lamps.

	D7 D6 D5 D4 D3 D2 D1 D0   Function
	--------------------------------------------------------------------------------------
	 0  0  0  1  1  0  0  0   Explosion Lamp 0
	 0  0  0  1  0  1  0  0   Explosion Lamp 1
	 0  0  0  1  0  0  1  0   Explosion Lamp 2
	 0  0  0  1  0  0  0  1   Explosion Lamp 3
	 0  0  1  0  1  0  0  0   Explosion Lamp 4
	 0  0  1  0  0  1  0  0   Explosion Lamp 5
	 0  0  1  0  0  0  1  0   Explosion Lamp 6
	 0  0  1  0  0  0  0  1   Explosion Lamp 7
	 0  1  0  0  1  0  0  0   Explosion Lamp 8
	 0  1  0  0  0  1  0  0   Explosion Lamp 9
	 0  1  0  0  0  0  1  0   Explosion Lamp A
	 0  1  0  0  0  0  0  1   Explosion Lamp B
	 1  0  0  0  1  0  0  0   Explosion Lamp C
	 1  0  0  0  0  1  0  0   Explosion Lamp D
	 1  0  0  0  0  0  1  0   Explosion Lamp E
	 1  0  0  0  0  0  0  1   Explosion Lamp F

Port 2:
	D7 D6 D5 D4 D3 D2 D1 D0   Function
	--------------------------------------------------------------------------------------
	 x  x  x  x  x  x  x  1   Torpedo 1
	 x  x  x  x  x  x  1  x   Torpedo 2
	 x  x  x  x  x  1  x  x   Torpedo 3
	 x  x  x  x  1  x  x  x   Torpedo 4
	 x  x  x  1  x  x  x  x   Ready
	 x  x  1  x  x  x  x  x   Reload

*/

	install_port_read_handler (0, 0x01, 0x01, seawolf_port_1_r);

	install_port_write_handler(0, 0x05, 0x05, seawolf_sh_port5_w);

}

static WRITE_HANDLER( seawolf_sh_port5_w )
{
	if (data & 0x01)
		sample_start (0, 0, 0);  /* Ship Hit */
	if (data & 0x02)
		sample_start (1, 1, 0);  /* Torpedo */
	if (data & 0x04)
		sample_start (2, 2, 0);  /* Dive */
	if (data & 0x08)
		sample_start (3, 3, 0);  /* Sonar */
	if (data & 0x10)
		sample_start (4, 4, 0);  /* Mine Hit */

	coin_counter_w(0, (data & 0x20) >> 5);    /* Coin Counter */
}


/*******************************************************/
/*                                                     */
/* Midway "Desert Gun"                                 */
/*                                                     */
/*******************************************************/

MACHINE_INIT( desertgu )
{
	install_port_read_handler (0, 0x01, 0x01, desertgu_port_1_r);

	install_port_write_handler(0, 0x07, 0x07, desertgu_controller_select_w);
}


/*******************************************************/
/*                                                     */
/* Taito "Space Chaser" 							   */
/*                                                     */
/*******************************************************/

/*
 *  The dot sound is a square wave clocked by either the
 *  the 8V or 4V signals
 *
 *  The frequencies are (for the 8V signal):
 *
 *  19.968 MHz crystal / 2 (Qa of 74160 #10) -> 9.984MHz
 *					   / 2 (7474 #14) -> 4.992MHz
 *					   / 256+16 (74161 #5 and #8) -> 18352.94Hz
 *					   / 8 (8V) -> 2294.12 Hz
 * 					   / 2 the final freq. is 2 toggles -> 1147.06Hz
 *
 *  for 4V, it's double at 2294.12Hz
 */

static int channel_dot;

struct SN76477interface schaser_sn76477_interface =
{
	1,	/* 1 chip */
	{ 50 },  /* mixing level   pin description		 */
	{ RES_K( 47)   },		/*	4  noise_res		 */
	{ RES_K(330)   },		/*	5  filter_res		 */
	{ CAP_P(470)   },		/*	6  filter_cap		 */
	{ RES_M(2.2)   },		/*	7  decay_res		 */
	{ CAP_U(1.0)   },		/*	8  attack_decay_cap  */
	{ RES_K(4.7)   },		/* 10  attack_res		 */
	{ 0			   },		/* 11  amplitude_res (variable)	 */
	{ RES_K(33)    },		/* 12  feedback_res 	 */
	{ 0            },		/* 16  vco_voltage		 */
	{ CAP_U(0.1)   },		/* 17  vco_cap			 */
	{ RES_K(39)    },		/* 18  vco_res			 */
	{ 5.0		   },		/* 19  pitch_voltage	 */
	{ RES_K(120)   },		/* 20  slf_res			 */
	{ CAP_U(1.0)   },		/* 21  slf_cap			 */
	{ 0 		   },		/* 23  oneshot_cap (variable) */
	{ RES_K(220)   }		/* 24  oneshot_res		 */
};

struct DACinterface schaser_dac_interface =
{
	1,
	{ 50 }
};

struct CustomSound_interface schaser_custom_interface =
{
	schaser_sh_start,
	schaser_sh_stop,
	schaser_sh_update
};

static INT16 backgroundwave[32] =
{
    0x7fff, 0x7fff, 0x7fff, 0x7fff, 0x7fff, 0x7fff, 0x7fff, 0x7fff,
    0x7fff, 0x7fff, 0x7fff, 0x7fff, 0x7fff, 0x7fff, 0x7fff, 0x7fff,
   -0x8000,-0x8000,-0x8000,-0x8000,-0x8000,-0x8000,-0x8000,-0x8000,
   -0x8000,-0x8000,-0x8000,-0x8000,-0x8000,-0x8000,-0x8000,-0x8000,
};

MACHINE_INIT( schaser )
{
	install_port_write_handler(0, 0x03, 0x03, schaser_sh_port3_w);
	install_port_write_handler(0, 0x05, 0x05, schaser_sh_port5_w);

	SN76477_mixer_a_w(0, 0);
	SN76477_mixer_c_w(0, 0);

	SN76477_envelope_1_w(0, 1);
	SN76477_envelope_2_w(0, 0);
}

static WRITE_HANDLER( schaser_sh_port3_w )
{
	int explosion;

	/* bit 0 - Dot Sound Enable (SX0)
	   bit 1 - Dot Sound Pitch (SX1)
	   bit 2 - Effect Sound A (SX2)
	   bit 3 - Effect Sound B (SX3)
	   bit 4 - Effect Sound C (SX4)
	   bit 5 - Explosion (SX5) */

	if (channel_dot)
	{
		int freq;

		mixer_set_volume(channel_dot, (data & 0x01) ? 100 : 0);

		freq = 19968000 / 2 / 2 / (256+16) / ((data & 0x02) ? 8 : 4) / 2;
		mixer_set_sample_frequency(channel_dot, freq);
	}

	explosion = (data >> 5) & 0x01;
	if (explosion)
	{
		SN76477_set_amplitude_res(0, RES_K(200));
		SN76477_set_oneshot_cap(0, CAP_U(0.1));		/* ???? */
	}
	else
	{
		/* 68k and 200k resistors in parallel */
		SN76477_set_amplitude_res(0, RES_K(1.0/((1.0/200.0)+(1.0/68.0))));
		SN76477_set_oneshot_cap(0, CAP_U(0.1));		/* ???? */
	}
	SN76477_enable_w(0, !explosion);
	SN76477_mixer_b_w(0, explosion);
}

static WRITE_HANDLER( schaser_sh_port5_w )
{
	/* bit 0 - Music (DAC) (SX6)
	   bit 1 - Sound Enable (SX7)
	   bit 2 - Coin Lockout (SX8)
	   bit 3 - Field Control A (SX9)
	   bit 4 - Field Control B (SX10)
	   bit 5 - Flip Screen */

	DAC_data_w(0, data & 0x01 ? 0xff : 0x00);

	mixer_sound_enable_global_w(data & 0x02);

	coin_lockout_global_w(data & 0x04);

	c8080bw_flip_screen_w(data & 0x20);
}

static int schaser_sh_start(const struct MachineSound *msound)
{
	channel_dot = mixer_allocate_channel(50);
	mixer_set_name(channel_dot,"Dot Sound");

	mixer_set_volume(channel_dot,0);
	mixer_play_sample_16(channel_dot,backgroundwave,sizeof(backgroundwave),1000,1);

	return 0;
}

static void schaser_sh_stop(void)
{
	mixer_stop_sample(channel_dot);
}

static void schaser_sh_update(void)
{
}


static WRITE_HANDLER( clowns_sh_port7_w )
{
/* bit 0x08 seems to always be enabled.  Possibly sound enable? */
/* A new sample set needs to be made with 3 different balloon sounds,
   and the code modified to suit. */

	if (data & 0x01)
		sample_start (0, 0, 0);  /* Bottom Balloon Pop */

	if (data & 0x02)
		sample_start (0, 0, 0);  /* Middle Balloon Pop */

	if (data & 0x04)
		sample_start (0, 0, 0);  /* Top Balloon Pop */

	if (data & 0x10)
		sample_start (2, 2, 0);  /* Bounce */

	if (data & 0x20)
		sample_start (1, 1, 0);  /* Splat */
}

MACHINE_INIT( clowns )
{
	/* Ports 5 & 6 are probably the music channels. They change value when
	 * a bonus is made. */

	install_port_write_handler (0, 0x07, 0x07, clowns_sh_port7_w);
}


/*******************************************************/
/*                                                     */
/* Taito "Indian Battle"	                       */
/* *Missing music*                                     */
/* Based on invader settings and sample choices from   */
/* https://github.com/mamedev/mame                     */
/*                                                     */
/*******************************************************/

MACHINE_INIT( indianbt )
{
	install_port_write_handler(0, 0x03, 0x03, indianbt_sh_port3_w);
	install_port_write_handler(0, 0x05, 0x05, indianbt_sh_port5_w);

	SN76477_envelope_1_w(0, 1);
	SN76477_envelope_2_w(0, 0);
	SN76477_mixer_a_w(0, 0);
	SN76477_mixer_b_w(0, 0);
	SN76477_mixer_c_w(0, 0);
	SN76477_vco_w(0, 1);
}

static WRITE_HANDLER( indianbt_sh_port3_w )
{
	static unsigned char last = 0;
	UINT8 rising_bits = data & ~last;

	if (rising_bits & 0x01) sample_start(1, 7, 0);     /* Death */
	if (rising_bits & 0x02) sample_start(0, 1, 0);     /* Shot Sound */
	if (rising_bits & 0x04) sample_start(2, 3, 0);     /* Move */
	if (rising_bits & 0x08) sample_start(3, 2, 0);     /* Hit */

	mixer_sound_enable_global_w(data & 0x20);
	c8080bw_screen_red_w(data & 0x01);

	last = data;
}

static WRITE_HANDLER( indianbt_sh_port5_w )
{
	static unsigned char last = 0;
	UINT8 rising_bits = data & ~last;

    	if (rising_bits & 0x01) sample_start(4, 0, 0);     /* Bird dropped an egg, Lasso used */
	if (rising_bits & 0x02) sample_start(4, 2, 0);     /* Egg hatches, egg shot */
	if (rising_bits & 0x08) sample_start(5, 0, 0);     /* Grabber, Lasso caught something */
	if (rising_bits & 0x10) sample_start(3, 7, 0);     /* Lasso sound */

	last = data;
}


/*******************************************************/
/*                                                     */
/* Taito "Lupin  III"	                               */
/* Based on invader settings and sample choices from   */
/* https://github.com/mamedev/mame                     */
/*                                                     */
/*******************************************************/

MACHINE_INIT( lupin3 )
{
	install_port_write_handler(0, 0x03, 0x03, lupin3_sh_port3_w);
	install_port_write_handler(0, 0x05, 0x05, lupin3_sh_port5_w);

	SN76477_envelope_1_w(0, 1);
	SN76477_envelope_2_w(0, 0);
	SN76477_mixer_a_w(0, 0);
	SN76477_mixer_b_w(0, 0);
	SN76477_mixer_c_w(0, 0);
	SN76477_vco_w(0, 1);
}

static WRITE_HANDLER( lupin3_sh_port3_w )
{
	static unsigned char last = 0;
	UINT8 rising_bits = data & ~last;

	if (rising_bits & 0x01) sample_start(0, 6, 0);		/* Walking, get money */
	SN76477_enable_w(0, !(data & 0x02));			/* Helicopter */
	if (rising_bits & 0x04) sample_start(0, 7, 0);		/* Translocate */
	if (rising_bits & 0x08) sample_start(0, 1, 0);		/* Jail */
	if (rising_bits & 0x10) sample_start(3, 8, 0);		/* Bonus man */

	last = data;
}

static WRITE_HANDLER( lupin3_sh_port5_w )
{
	static unsigned char last = 0;
	UINT8 rising_bits = data & ~last;

    	if (rising_bits & 0x01) sample_start(0, 3, 0);		/* Land on building, wife kicks man */
	if (rising_bits & 0x02) sample_start(1, 2, 0);		/* Deposit money, start intermission, end game */
	if (rising_bits & 0x04) sample_start(2, 5, 0);		/* Deposit money, start intermission, Slides down rope */
	if (rising_bits & 0x08) sample_start(3, 0, 0);		/* Start intermission, end game */
	/*if (rising_bits & 0x10) sample_start(3, 9, 0);	/* Dog barking */ 

	c8080bw_flip_screen_w(data & 0x20);

	last = data;
}


/*******************************************************/
/*                                                     */
/* Zenitone Microsec "Invader's Revenge"               */
/* Based on invader settings and sample choices from   */
/* https://github.com/mamedev/mame                     */
/*                                                     */
/* Sounds are not accurate.                            */
/*                                                     */
/*******************************************************/

MACHINE_INIT( invrvnge )
{
	install_port_write_handler(0, 0x03, 0x03, invrvnge_sh_port3_w);
	install_port_write_handler(0, 0x05, 0x05, invrvnge_sh_port5_w);
}

static WRITE_HANDLER( invrvnge_sh_port3_w )
{
        switch (data)
	{
	        case 0x06:				/* Shoot */
                        sample_start(1, 0, 0);
                        break;
                case 0x14:				/* Hit alien */
                        sample_start(2, 2, 0); 
                        break;
                case 0x16:				/* Hit asteroid */
                        sample_start(2, 5, 0);
                        break;
                case 0x1e:				/* Death (followed by 0x0a byte), also bit 4 of port 5 */
                        sample_start(3, 1, 0);
                        break;
                case 0x18:				/* Fuel low */
                case 0x30:				/* Fuel bar filling up */
                        sample_start(3, 7, 0);
                        break;
                case 0x02:				/* Coin */
                case 0x24:				/* Alien dropping to steal fuel */
                case 0x26:				/* Alien lifting with fuel */
                        break;
                case 0x32:				/* UFO drops bomb */
                        sample_start(2, 5, 0);
                        break;
                case 0x3a:				/* Thrust, docking, extra ship? */
                        sample_start(0, 8, 0);
                        break;
	}
}

static WRITE_HANDLER( invrvnge_sh_port5_w )
{
    /* No operations */
}


/*******************************************************/
/*                                                     */
/* Worldwing "Yosaku To Donbee"	                       */
/* Based on invader settings and sample choices from   */
/* https://github.com/mamedev/mame                     */
/*                                                     */
/* Sounds are unknown so some from invaders are used.  */
/*                                                     */
/*******************************************************/

MACHINE_INIT( yosakdon )
{
	install_port_write_handler(0, 0x03, 0x03, yosakdon_sh_port3_w);
	install_port_write_handler(0, 0x05, 0x05, yosakdon_sh_port5_w);
}

static WRITE_HANDLER( yosakdon_sh_port3_w )
{
	static unsigned char last = 0;
	UINT8 rising_bits = data & ~last;

	if (rising_bits & 0x01) sample_start(0, 3, 0);     /* Game over */
	if (rising_bits & 0x02) sample_start(2, 0, 0);     /* Bird dead */
	if (rising_bits & 0x04) sample_start(0, 1, 0);     /* Rifle shot */
	if (rising_bits & 0x08) sample_start(1, 2, 0);     /* Man dead */
	if (rising_bits & 0x10) sample_start(5, 8, 0);     /* Bonus man? */

	mixer_sound_enable_global_w(data & 0x20);

	last = data;
}

static WRITE_HANDLER( yosakdon_sh_port5_w )
{
	static unsigned char last = 0;
	UINT8 rising_bits = data & ~last;

    	if (rising_bits & 0x01) sample_start(1, 6, 0);	/* Ready? */
	if (rising_bits & 0x04) sample_start(3, 7, 0);	/* Big bird dead */
	SN76477_enable_w(0, !(data & 0x08));		/* Big bird */
	if (rising_bits & 0x10) sample_start(2, 7, 0);	/* Game over */

	c8080bw_flip_screen_w(data & 0x20);

	last = data;
}

/*******************************************************/
/*                                                     */
/* Nichibutsu's "Rolling Crash / Moon Base"            */
/* Based on invader settings and sample choices from   */
/* https://github.com/mamedev/mame                     */
/*                                                     */
/* Sounds are not accurate. */
/*                                                     */
/*******************************************************/

MACHINE_INIT( rollingc )
{
	install_port_write_handler(0, 0x00, 0x00, rollingc_sh_port0_w); /* Rolling Crash */
	/* Note Rolling Crash also uses some of the sounds from Moon Base. */
	/* Though we basically use the Invaders sounds for Moon Base. */
	install_port_write_handler(0, 0x03, 0x03, invaders_sh_port3_w); /* Moon Base */
	install_port_write_handler(0, 0x05, 0x05, invaders_sh_port5_w); /* Moon Base */

	SN76477_envelope_1_w(0, 1);
	SN76477_envelope_2_w(0, 0);
	SN76477_mixer_a_w(0, 0);
	SN76477_mixer_b_w(0, 0);
	SN76477_mixer_c_w(0, 0);
	SN76477_vco_w(0, 1);
}

static WRITE_HANDLER( rollingc_sh_port0_w )
{
	static unsigned char last = 0;
	UINT8 rising_bits = data & ~last;

	if (rising_bits & 0x02) sample_start(2, 0, 0);	/* Steering */
	if (rising_bits & 0x04) sample_start(0, 1, 0);	/* Collision */
	if (rising_bits & 0x10) sample_start(1, 8, 0);	/* Computer car is starting to move */

	/* Note sound of hitting a dot seem to come from Moon Base port handlers. */

	last = data;
}

/*******************************************************/
/*                                                     */
/* Sidam's "Astropal"                                  */
/* Based on invader samples                            */
/*                                                     */
/* Sounds are not accurate.                            */
/*                                                     */
/*******************************************************/

MACHINE_INIT( astropal )
{
	install_port_write_handler(0, 0x03, 0x03, astropal_sh_port3_w);
	/* Can use invaders handler for port 0x05 */
	install_port_write_handler(0, 0x05, 0x05, invaders_sh_port5_w);

	SN76477_envelope_1_w(0, 1);
	SN76477_envelope_2_w(0, 0);
	SN76477_mixer_a_w(0, 0);
	SN76477_mixer_b_w(0, 0);
	SN76477_mixer_c_w(0, 0);
	SN76477_vco_w(0, 1);
}

static WRITE_HANDLER( astropal_sh_port3_w )
{
	static unsigned char last = 0;
	static int exploding = 0; /* Work around to stop exploding sound repeating */
	UINT8 rising_bits = data & ~last;

	if (rising_bits & 0x02) {
		sample_start (0, 0, 0);		/* Shot Sound */
		exploding = 0; /* Use this to reset exploding sound */
	}

	if (rising_bits & 0x04 && !exploding)  {
		sample_start (1, 1, 0);		/* Your Ship Hit */
		exploding = 1;
	}

	if (rising_bits & 0x08)
		sample_start (0, 2, 0);		/* Asteroid Hit */

	if (rising_bits & 0x10)
		sample_start (2, 8, 0);		/* Bonus Man? */

	c8080bw_screen_red_w(data & 0x04);

	last = data;
}


/*******************************************************/
/* Taito do Brasil's "Galactica - Batalha Espacial"    */
/* Based on invader samples                            */
/*                                                     */
/* Sounds are not accurate. Actual sounds can be       */
/* heard in:                                           */
/*   https://youtu.be/XRA5h--ikW8                      */
/*   https://youtu.be/z60ewMRyqSs                      */
/*   https://youtu.be/d1-Uoyhg5fI                      */
/*   https://youtu.be/6CWoN31NUsg                      */
/*                                                     */
/* Not sure these sounds can be reproduced from        */
/* Invader samples. Also, did not figure out what      */
/* port triggers a dive sound.                         */
/*                                                     */
/*******************************************************/

MACHINE_INIT( galactic )
{
	install_port_write_handler(0, 0x03, 0x03, galactic_sh_port3_w);
	install_port_write_handler(0, 0x05, 0x05, galactic_sh_port5_w);
}

static WRITE_HANDLER( galactic_sh_port3_w )
{
	static unsigned char last = 0;
	UINT8 rising_bits = data & ~last;

	if (rising_bits & 0x02)
		sample_start (0, 2, 0);		/* Enemy hit */

	if (rising_bits & 0x04)
		sample_start (1, 1, 0);		/* Your ship hit */

	/*if (data & 0x08) */
		/*sample_start (1, 6, 0);	/* Background continual? */

	if (rising_bits & 0x10)
		sample_start (2, 8, 0);		/* Bonus Man? */

	c8080bw_screen_red_w(data & 0x04);

	last = data;
}

static WRITE_HANDLER( galactic_sh_port5_w )
{
	static unsigned char last = 0;
	UINT8 rising_bits = data & ~last;

	/* These are meant to be a deep base background. */
	/* Can simulate by changing "rising_bits" to "data" */
	/* But the result sounds rough, and your explosion */
	/* cuts out. */
	if (rising_bits & 0x01)
		sample_start (1, 3, 0);		/* Background beat */

	if (rising_bits & 0x02)
		sample_start (1, 4, 0);		/* Background beat */

	if (rising_bits & 0x04)
		sample_start (1, 5, 0);		/* Background beat */

	if (rising_bits & 0x08)
		sample_start (1, 6, 0);		/* Background beat */

	if (data & 0x10)
		sample_start (0, 0, 0);		/* Your shot */

	c8080bw_flip_screen_w(data & 0x20);

	last = data;
}
