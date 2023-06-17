/***************************************************************************

	Atari Night Driver hardware

	driver by Mike Balfour

	Games supported:
		* Night Driver

	Known issues:
		* none at this time

****************************************************************************

	Memory Map:
		0000-01FF	R/W 	SCRAM (Scratchpad RAM)
		0200-03FF	 W		PFW (Playfield Write)
		0400-05FF	 W		HVC (Horiz/Vert/Char for Roadway)
		0600-07FF	 R		IN0
		0800-09FF	 R		IN1
		0A00-0BFF	 W		OUT0
		0C00-0DFF	 W		OUT1
		0E00-0FFF	 -		OUT2 (Not used)
		8000-83FF	 R		PFR (Playfield Read)
		8400-87FF			Steering Reset
		8800-8FFF	 -		Spare (Not used)
		9000-97FF	 R		Program ROM1
		9800-9FFF	 R		Program ROM2
		(F800-FFFF)	 R		Program ROM2 - only needed for the 6502 vectors

	If you have any questions about how this driver works, don't hesitate to
	ask.  - Mike Balfour (mab22@po.cwru.edu)

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

extern UINT8 *nitedrvr_ram;

extern int nitedrvr_gear;
extern int nitedrvr_track;

extern READ_HANDLER( nitedrvr_in0_r );
extern READ_HANDLER( nitedrvr_in1_r );
extern READ_HANDLER( nitedrvr_ram_r );
extern READ_HANDLER( nitedrvr_steering_reset_r );
extern WRITE_HANDLER( nitedrvr_steering_reset_w );
extern WRITE_HANDLER( nitedrvr_out0_w );
extern WRITE_HANDLER( nitedrvr_out1_w );
extern WRITE_HANDLER( nitedrvr_ram_w );
extern void nitedrvr_crash_toggle(int dummy);

extern UINT8 *nitedrvr_hvc;

extern WRITE_HANDLER( nitedrvr_videoram_w );
extern WRITE_HANDLER( nitedrvr_hvc_w );

extern VIDEO_START( nitedrvr );
extern VIDEO_UPDATE( nitedrvr );


/*************************************
 *
 *	Palette generation
 *
 *************************************/

static unsigned short colortable_source[] =
{
	0x00, 0x01,
	0x01, 0x00,
};

static PALETTE_INIT( nitedrvr )
{
	palette_set_color(0,0x00,0x00,0x00); /* BLACK */
	palette_set_color(1,0xff,0xff,0xff); /* WHITE */
	palette_set_color(2,0x55,0x55,0x55); /* DK GREY - for MAME text only */
	palette_set_color(3,0x80,0x80,0x80); /* LT GREY - for MAME text only */
	memcpy(colortable,colortable_source,sizeof(colortable_source));
}


static MACHINE_INIT( nitedrvr )
{
	timer_pulse(TIME_IN_SEC(0.693 * (180000 + (2 * 330)) * 1e-6), 0, nitedrvr_crash_toggle);
}



/*************************************
 *
 *	Main CPU memory handlers
 *
 *************************************/

static MEMORY_READ_START( readmem )
	{ 0x0000, 0x00ff, nitedrvr_ram_r }, /* SCRAM */
	{ 0x0100, 0x01ff, nitedrvr_ram_r }, /* SCRAM */
	{ 0x0600, 0x07ff, nitedrvr_in0_r },
	{ 0x0800, 0x09ff, nitedrvr_in1_r },
	{ 0x8000, 0x807f, videoram_r }, /* PFR */
	{ 0x8080, 0x80ff, videoram_r }, /* PFR */
	{ 0x8100, 0x817f, videoram_r }, /* PFR */
	{ 0x8180, 0x81ff, videoram_r }, /* PFR */
	{ 0x8200, 0x827f, videoram_r }, /* PFR */
	{ 0x8280, 0x82ff, videoram_r }, /* PFR */
	{ 0x8300, 0x837f, videoram_r }, /* PFR */
	{ 0x8380, 0x83ff, videoram_r }, /* PFR */
	{ 0x8400, 0x87ff, nitedrvr_steering_reset_r },
	{ 0x9000, 0x9fff, MRA_ROM }, /* ROM1-ROM2 */
	{ 0xfff0, 0xffff, MRA_ROM }, /* ROM2 for 6502 vectors */
MEMORY_END


static MEMORY_WRITE_START( writemem )
	{ 0x0000, 0x00ff, nitedrvr_ram_w, &nitedrvr_ram }, /* SCRAM */
	{ 0x0100, 0x01ff, nitedrvr_ram_w }, /* SCRAM */
	{ 0x0200, 0x027f, nitedrvr_videoram_w, &videoram }, /* PFW */
	{ 0x0280, 0x02ff, nitedrvr_videoram_w }, /* PFW */
	{ 0x0300, 0x037f, nitedrvr_videoram_w }, /* PFW */
	{ 0x0380, 0x03ff, nitedrvr_videoram_w }, /* PFW */
	{ 0x0400, 0x05ff, nitedrvr_hvc_w, &nitedrvr_hvc }, /* POSH, POSV, CHAR, Watchdog */
	{ 0x0a00, 0x0bff, nitedrvr_out0_w },
	{ 0x0c00, 0x0dff, nitedrvr_out1_w },
	{ 0x8400, 0x87ff, nitedrvr_steering_reset_w },
	{ 0x9000, 0x9fff, MWA_ROM }, /* ROM1-ROM2 */
MEMORY_END



/*************************************
 *
 *	Port definitions
 *
 *************************************/

INPUT_PORTS_START( nitedrvr )
	PORT_START		/* fake port, gets mapped to Night Driver ports */
	PORT_DIPNAME( 0x30, 0x10, DEF_STR( Coinage ) )
	PORT_DIPSETTING(	0x30, DEF_STR( 2C_1C ) )
	/*PORT_DIPSETTING(	0x20, DEF_STR( 1C_1C ) )  // not a typo */
	PORT_DIPSETTING(	0x10, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(	0x00, DEF_STR( 1C_2C ) )
	PORT_DIPNAME( 0xC0, 0x80, "Playing Time" )
	PORT_DIPSETTING(	0x00, "50" )
	PORT_DIPSETTING(	0x40, "75" )
	PORT_DIPSETTING(	0x80, "100" )
	PORT_DIPSETTING(	0xC0, "125" )

	PORT_START		/* fake port, gets mapped to Night Driver ports */
	PORT_DIPNAME( 0x10, 0x00, "Track Set" )
	PORT_DIPSETTING(	0x00, "Normal" )
	PORT_DIPSETTING(	0x10, "Reverse" )
	PORT_DIPNAME( 0x20, 0x20, "Bonus Time" )
	PORT_DIPSETTING(	0x00, DEF_STR ( No ) )
	PORT_DIPSETTING(	0x20, "Score = 350" )
	PORT_BIT (0x40, IP_ACTIVE_HIGH, IPT_VBLANK )
	PORT_BITX(0x80, IP_ACTIVE_LOW, IPT_SERVICE | IPF_TOGGLE, "Self Test", KEYCODE_F2, IP_JOY_NONE )

	PORT_START		/* fake port, gets mapped to Night Driver ports */
	PORT_BITX(0x10, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN | IPF_PLAYER2, "1st gear", IP_KEY_DEFAULT, IP_JOY_DEFAULT )
	PORT_BITX(0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_PLAYER2, "2nd gear", IP_KEY_DEFAULT, IP_JOY_DEFAULT )
	PORT_BITX(0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_PLAYER2, "3rd gear", IP_KEY_DEFAULT, IP_JOY_DEFAULT )
	PORT_BITX(0x80, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP | IPF_PLAYER2, "4th gear", IP_KEY_DEFAULT, IP_JOY_DEFAULT )

	PORT_START		/* fake port, gets mapped to Night Driver ports */
	PORT_BIT (0x10, IP_ACTIVE_HIGH, IPT_UNKNOWN )	/* Spare */
	PORT_DIPNAME( 0x20, 0x00, "Difficult Bonus" )
	PORT_DIPSETTING(	0x00, "Normal" )
	PORT_DIPSETTING(	0x20, "Difficult" )
	PORT_BIT (0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT (0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START		/* fake port, gets mapped to Night Driver ports */
	PORT_BIT (0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT (0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT (0x04, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_BUTTON1, "Gas", IP_KEY_DEFAULT, IP_JOY_DEFAULT )
	PORT_BITX(0x10, IP_ACTIVE_HIGH, IPT_BUTTON2, "Novice Track", IP_KEY_DEFAULT, IP_JOY_DEFAULT )
	PORT_BITX(0x20, IP_ACTIVE_HIGH, IPT_BUTTON3, "Expert Track", IP_KEY_DEFAULT, IP_JOY_DEFAULT )
	PORT_BITX(0x40, IP_ACTIVE_HIGH, IPT_BUTTON4, "Pro Track", IP_KEY_DEFAULT, IP_JOY_DEFAULT )
	PORT_BIT (0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )	/* Alternating signal? */

	PORT_START		/* fake port used for steering */
	PORT_ANALOG( 0xff, 0x00, IPT_DIAL, 100, 10, 0, 0 )

	PORT_START
	PORT_ADJUSTER( 60, "Motor RPM" )
INPUT_PORTS_END



/*************************************
 *
 *	Graphics definitions
 *
 *************************************/

static struct GfxLayout charlayout =
{
	8,8,
	64,
	1,
	{ 0 },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8
};


static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &charlayout, 0x00, 0x02 }, /* offset into colors, # of colors */
	{ -1 } /* end of array */
};


/************************************************************************/
/* nitedrvr Sound System Analog emulation                                  */
/************************************************************************/

const struct discrete_lfsr_desc nitedrvr_lfsr = {
	16,			/* Bit Length */
	0,			/* Reset Value */
	0,			/* Use Bit 0 as XOR input 0 */
	14,			/* Use Bit 14 as XOR input 1 */
	DISC_LFSR_XNOR,		/* Feedback stage1 is XNOR */
	DISC_LFSR_OR,		/* Feedback stage2 is just stage 1 output OR with external feed */
	DISC_LFSR_REPLACE,	/* Feedback stage3 replaces the shifted register contents */
	0x000001,		/* Everything is shifted into the first bit only */
	0,			/* Output is already inverted by XNOR */
	15			/* Output bit */
};

/* Nodes - Inputs */
#define NITEDRVR_BANG_DATA	NODE_01
#define NITEDRVR_SKID1_EN	NODE_02
#define NITEDRVR_SKID2_EN	NODE_03
#define NITEDRVR_MOTOR_DATA	NODE_04
#define NITEDRVR_CRASH_EN	NODE_05
#define NITEDRVR_ATTRACT_EN	NODE_06
/* Nodes - Sounds */
#define NITEDRVR_NOISE		NODE_10
#define NITEDRVR_BANG_SND	NODE_11
#define NITEDRVR_MOTOR_SND	NODE_12
#define NITEDRVR_SCREECH1_SND	NODE_13
#define NITEDRVR_SCREECH2_SND	NODE_14

static DISCRETE_SOUND_START(nitedrvr_sound_interface)
	/************************************************/
	/* nitedrvr  Effects Relataive Gain Table       */
	/*                                              */
	/* Effect    V-ampIn  Gain ratio      Relative  */
	/* Bang       3.8     5/(5+6.8)        1000.0   */
	/* Motor      3.8     5/(5+10)          786.7   */
	/* Screech1   3.8     5/(5+47)          226.9   */
	/* Screech2   3.8     5/(5+47)          226.9   */
	/************************************************/

	/************************************************/
	/* Input register mapping for nitedrvr          */
	/************************************************/
	/*              NODE                 ADDR  MASK    GAIN      OFFSET  INIT */
	DISCRETE_INPUTX(NITEDRVR_BANG_DATA,  0x00, 0x000f, 1000.0/15, 0.0,   0.0)
	DISCRETE_INPUT (NITEDRVR_SKID1_EN,   0x01, 0x000f,                   0.0)
	DISCRETE_INPUT (NITEDRVR_SKID2_EN,   0x02, 0x000f,                   0.0)
	DISCRETE_INPUT (NITEDRVR_MOTOR_DATA, 0x03, 0x000f,                   0.0)
	DISCRETE_INPUT (NITEDRVR_CRASH_EN,   0x04, 0x000f,                   0.0)
	DISCRETE_INPUT (NITEDRVR_ATTRACT_EN, 0x05, 0x000f,                   0.0)

	/************************************************/
	/* NOTE: Motor circuit is a rip from Sprint for */
	/* now. This will have to be converted to 566.  */
	/*                                              */
	/* Motor sound circuit is based on a 556 VCO    */
	/* with the input frequency set by the MotorSND */
	/* latch (4 bit). This freqency is then used to */
	/* driver a modulo 12 counter, with div6, 4 & 3 */
	/* summed as the output of the circuit.         */
	/* VCO Output is Sq wave = 27-382Hz             */
	/*  F1 freq - (Div6)                            */
	/*  F2 freq = (Div4)                            */
	/*  F3 freq = (Div3) 33.3% duty, 33.3 deg phase */
	/* To generate the frequency we take the freq.  */
	/* diff. and /15 to get all the steps between   */
	/* 0 - 15.  Then add the low frequency and send */
	/* that value to a squarewave generator.        */
	/* Also as the frequency changes, it ramps due  */
	/* to a 2.2uf capacitor on the R-ladder.        */
	/* Note the VCO freq. is controlled by a 250k   */
	/* pot.  The freq. used here is for the pot set */
	/* to 125k.  The low freq is allways the same.  */
	/* This adjusts the high end.                   */
	/* 0k = 214Hz.   250k = 4416Hz                  */
	/************************************************/
	DISCRETE_RCFILTER(NODE_20, 1, NITEDRVR_MOTOR_DATA, 123037, 2.2e-6)
	DISCRETE_ADJUSTMENT(NODE_21, 1, (214.0-27.0)/12/31, (4416.0-27.0)/12/31, DISC_LOGADJ, 6)
	DISCRETE_MULTIPLY(NODE_22, 1, NODE_20, NODE_21)

	DISCRETE_MULTADD(NODE_23, 1, NODE_22, 2, 27.0/6)	/* F1 = /12*2 = /6 */
	DISCRETE_SQUAREWAVE(NODE_24, 1, NODE_23, (786.7/3), 50.0, 0, 0)
	DISCRETE_RCFILTER(NODE_25, 1, NODE_24, 10000, 1e-7)

	DISCRETE_MULTADD(NODE_26, 1, NODE_22, 3, 27.0/4)	/* F2 = /12*3 = /4 */
	DISCRETE_SQUAREWAVE(NODE_27, 1, NODE_26, (786.7/3), 50.0, 0, 0)
	DISCRETE_RCFILTER(NODE_28, 1, NODE_27, 10000, 1e-7)

	DISCRETE_MULTADD(NODE_29, 1, NODE_22, 4, 27.0/3)	/* F3 = /12*4 = /3 */
	DISCRETE_SQUAREWAVE(NODE_30, 1, NODE_29, (786.7/3), 100.0/3, 0, 360.0/3)
	DISCRETE_RCFILTER(NODE_31, 1, NODE_30, 10000, 1e-7)

	DISCRETE_ADDER3(NITEDRVR_MOTOR_SND, NITEDRVR_ATTRACT_EN, NODE_25, NODE_28, NODE_31)

	/************************************************/
	/* Bang circuit is built around a noise         */
	/* generator built from 2 shift registers that  */
	/* are clocked by the 4V signal.                */
	/* 4V = HSYNC/2/4                               */
	/*    = 15750/2/4                               */
	/* Output is integrated to apply decay.         */
	/************************************************/
	DISCRETE_LFSR_NOISE(NITEDRVR_NOISE, NITEDRVR_CRASH_EN, NITEDRVR_CRASH_EN, 15750.0/2/4, 1.0, 0, 0, &nitedrvr_lfsr)

	DISCRETE_MULTIPLY(NODE_62, 1, NITEDRVR_NOISE, NITEDRVR_BANG_DATA)
	DISCRETE_RCFILTER(NITEDRVR_BANG_SND, 1, NODE_62, 545.6, 3.3e-7)

	/************************************************/
	/* Skid circuits ripped from other drivers      */
	/* for now.                                     */
	/*                                              */
	/* Skid circuits takes the noise output from    */
	/* the crash circuit and applies +ve feedback   */
	/* to cause oscillation. There is also an RC    */
	/* filter on the input to the feedback cct.     */
	/* RC is 1K & 10uF                              */
	/* Feedback cct is modelled by using the RC out */
	/* as the frequency input on a VCO,             */
	/* breadboarded freq range as:                  */
	/*  0 = 940Hz, 34% duty                         */
	/*  1 = 630Hz, 29% duty                         */
	/*  the duty variance is so small we ignore it  */
	/************************************************/
	DISCRETE_INVERT(NODE_70, NITEDRVR_NOISE)
	DISCRETE_RCFILTER(NODE_71, 1, NODE_70, 1000, 1e-5)
	DISCRETE_MULTADD(NODE_72, 1, NODE_71, 940.0-630.0, ((940.0-630.0)/2)+630.0)
	DISCRETE_SQUAREWAVE(NITEDRVR_SCREECH1_SND, NITEDRVR_SKID1_EN, NODE_72, 226.9, 31.5, 0, 0.0)

	DISCRETE_MULTADD(NODE_75, 1, NODE_71, 1380.0-626.0, 626.0+((1380.0-626.0)/2))	/* Frequency */
	DISCRETE_MULTADD(NODE_76, 1, NODE_71, 32.0-13.0, 13.0+((32.0-13.0)/2))		/* Duty */
	DISCRETE_SQUAREWAVE(NITEDRVR_SCREECH2_SND, NITEDRVR_SKID2_EN, NODE_75, 226.9, NODE_76, 0, 0.0)

	/************************************************/
	/* Combine all sound sources.                   */
	/* Add some final gain to get to a good sound   */
	/* level.                                       */
	/************************************************/
	DISCRETE_ADDER4(NODE_90, 1, NITEDRVR_MOTOR_SND, NITEDRVR_BANG_SND, NITEDRVR_SCREECH1_SND, NITEDRVR_SCREECH2_SND)
	DISCRETE_GAIN(NODE_91, NODE_90, 65534.0/(786.7+1000.0+226.9))

	DISCRETE_OUTPUT(NODE_91, 100)
DISCRETE_SOUND_END



/*************************************
 *
 *	Machine driver
 *
 *************************************/

static MACHINE_DRIVER_START( nitedrvr )

	/* basic machine hardware */
	MDRV_CPU_ADD(M6502,1000000)	   /* 1 MHz ???? */
	MDRV_CPU_MEMORY(readmem,writemem)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,1)

	MDRV_FRAMES_PER_SECOND(57)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)

	MDRV_MACHINE_INIT(nitedrvr)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 0*8, 32*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(4)
	MDRV_COLORTABLE_LENGTH(sizeof(colortable_source) / sizeof(colortable_source[0]))
	
	MDRV_PALETTE_INIT(nitedrvr)
	MDRV_VIDEO_START(nitedrvr)
	MDRV_VIDEO_UPDATE(nitedrvr)

	/* sound hardware */
	MDRV_SOUND_ADD_TAG("discrete", DISCRETE, nitedrvr_sound_interface)
MACHINE_DRIVER_END



/*************************************
 *
 *	ROM definitions
 *
 *************************************/

ROM_START( nitedrvr )
	ROM_REGION( 0x10000, REGION_CPU1, 0 ) /* 64k for code */
	ROM_LOAD( "6569-01.d2",   0x9000, 0x0800, CRC(7afa7542) SHA1(81018e25ebdeae1daf1308676661063b6fd7fd22) )
	ROM_LOAD( "6570-01.f2",   0x9800, 0x0800, CRC(bf5d77b1) SHA1(6f603f8b0973bd89e0e721b66944aac8e9f904d9) )
	ROM_RELOAD( 			  0xf800, 0x0800 )

	ROM_REGION( 0x0200, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "6568-01.p2",   0x0000, 0x0200, CRC(f80d8889) SHA1(ca573543dcce1221459d5693c476cef14bfac4f4) )

	ROM_REGION( 0x0100, REGION_PROMS, 0 )
	ROM_LOAD( "6559-01.h7",   0x0000, 0x0100, CRC(5a8d0e42) SHA1(772220c4c24f18769696ddba26db2bc2e5b0909d) )	/* unknown */
ROM_END



/*************************************
 *
 *	Game drivers
 *
 *************************************/

GAMEX( 1976, nitedrvr, 0, nitedrvr, nitedrvr, 0, ROT0, "Atari", "Night Driver", GAME_IMPERFECT_SOUND )
