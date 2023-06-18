/***************************************************************************

	Atari Subs hardware

	driver by Mike Balfour

	Games supported:
		* Subs
		
	Known issues:
		* none at this time

****************************************************************************

	If you have any questions about how this driver works, don't hesitate to
	ask.  - Mike Balfour (mab22@po.cwru.edu)

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "subs.h"



/*************************************
 *
 *	Palette generation
 *
 *************************************/

static unsigned short colortable_source[] =
{
	0x00, 0x01,		/* Right screen */
	0x02, 0x03		/* Left screen */
};

static PALETTE_INIT( subs )
{
	palette_set_color(0,0x00,0x00,0x00); /* BLACK - modified on video invert */
	palette_set_color(1,0xff,0xff,0xff); /* WHITE - modified on video invert */
	palette_set_color(2,0x00,0x00,0x00); /* BLACK - modified on video invert */
	palette_set_color(3,0xff,0xff,0xff); /* WHITE - modified on video invert*/
	memcpy(colortable,colortable_source,sizeof(colortable_source));
}



/*************************************
 *
 *	Main CPU memory handlers
 *
 *************************************/

static MEMORY_READ_START( readmem )
	{ 0x0000, 0x0007, subs_control_r },
	{ 0x0020, 0x0027, subs_coin_r },
	{ 0x0060, 0x0063, subs_options_r },
	{ 0x0000, 0x01ff, MRA_RAM },
	{ 0x0800, 0x0bff, MRA_RAM },
	{ 0x2000, 0x3fff, MRA_ROM },
	{ 0xf000, 0xffff, MRA_ROM }, /* A14/A15 unused, so mirror ROM */
MEMORY_END


static MEMORY_WRITE_START( writemem )
	{ 0x0000, 0x0000, subs_noise_reset_w },
	{ 0x0020, 0x0020, subs_steer_reset_w },
/*	{ 0x0040, 0x0040, subs_timer_reset_w },*/
	{ 0x0060, 0x0061, subs_lamp1_w },
	{ 0x0062, 0x0063, subs_lamp2_w },
	{ 0x0064, 0x0065, subs_sonar2_w },
	{ 0x0066, 0x0067, subs_sonar1_w },
/* Schematics show crash and explode reversed.  But this is proper.*/
	{ 0x0068, 0x0069, subs_explode_w },
	{ 0x006a, 0x006b, subs_crash_w },
	{ 0x006c, 0x006d, subs_invert1_w },
	{ 0x006e, 0x006f, subs_invert2_w },
	{ 0x0090, 0x009f, spriteram_w, &spriteram },
	{ 0x0000, 0x07ff, MWA_RAM },
	{ 0x0800, 0x0bff, videoram_w, &videoram, &videoram_size },
	{ 0x2000, 0x3fff, MWA_ROM },
MEMORY_END



/*************************************
 *
 *	Port definitions
 *
 *************************************/

INPUT_PORTS_START( subs )
	PORT_START /* OPTIONS */
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x00, "Credit/Time" )
	PORT_DIPSETTING(    0x00, "Each Coin Buys Time" )
	PORT_DIPSETTING(    0x02, "Fixed Time" )
	PORT_DIPNAME( 0x0c, 0x00, "Game Language" )
	PORT_DIPSETTING(    0x00, "English" )
	PORT_DIPSETTING(    0x04, "Spanish" )
	PORT_DIPSETTING(    0x08, "French" )
	PORT_DIPSETTING(    0x0c, "German" )
	PORT_DIPNAME( 0x10, 0x00, DEF_STR( Free_Play ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x10, DEF_STR( On ) )
	PORT_DIPNAME( 0xe0, 0x40, "Game Length" )
	PORT_DIPSETTING(    0x00, "0:30 Minutes" )
	PORT_DIPSETTING(    0x20, "1:00 Minutes" )
	PORT_DIPSETTING(    0x40, "1:30 Minutes" )
	PORT_DIPSETTING(    0x60, "2:00 Minutes" )
	PORT_DIPSETTING(    0x80, "2:30 Minutes" )
	PORT_DIPSETTING(    0xa0, "3:00 Minutes" )
	PORT_DIPSETTING(    0xc0, "3:30 Minutes" )
	PORT_DIPSETTING(    0xe0, "4:00 Minutes" )

	PORT_START /* IN1 */
	PORT_BIT ( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN ) /* Diag Step */
	PORT_BIT ( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN ) /* Diag Hold */
	PORT_BIT ( 0x04, IP_ACTIVE_LOW, IPT_TILT )    /* Slam */
	PORT_BIT ( 0x08, IP_ACTIVE_LOW, IPT_UNUSED )  /* Spare */
	PORT_BIT ( 0xf0, IP_ACTIVE_HIGH, IPT_UNUSED ) /* Filled in with steering information */

	PORT_START /* IN2 */
	PORT_BIT ( 0x01, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT ( 0x02, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT ( 0x04, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT ( 0x08, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT ( 0x10, IP_ACTIVE_LOW, IPT_VBLANK )
	PORT_BIT ( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BITX( 0x40, IP_ACTIVE_LOW, IPT_SERVICE | IPF_TOGGLE, "Self Test", KEYCODE_F2, IP_JOY_NONE )
	PORT_BIT ( 0x80, IP_ACTIVE_LOW, IPT_BUTTON1 )

	PORT_START      /* IN3 */
	PORT_ANALOG( 0xff, 0x00, IPT_DIAL | IPF_PLAYER2, 100, 20, 0, 0 )

	PORT_START      /* IN4 */
	PORT_ANALOG( 0xff, 0x00, IPT_DIAL, 100, 20, 0, 0 )

INPUT_PORTS_END



/*************************************
 *
 *	Graphics definitions
 *
 *************************************/

static struct GfxLayout playfield_layout =
{
	8,8,
	256,
	1,
	{ 0 },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8
};


static struct GfxLayout motion_layout =
{
	16,16,
	64,
	1,
	{ 0 },
	{ 3 + 0x400*8, 2 + 0x400*8, 1 + 0x400*8, 0 + 0x400*8,
	  7 + 0x400*8, 6 + 0x400*8, 5 + 0x400*8, 4 + 0x400*8,
	  3, 2, 1, 0, 7, 6, 5, 4 },
	{ 0, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
	  8*8, 9*8, 10*8, 11*8, 12*8, 13*8, 14*8, 15*8 },
	16*8
};


static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &playfield_layout, 0, 2 }, 	/* playfield graphics */
	{ REGION_GFX2, 0, &motion_layout,    0, 2 }, 	/* motion graphics */
	{ -1 }
};


/************************************************************************/
/* subs Sound System Analog emulation                               */
/************************************************************************/

const struct discrete_lfsr_desc subs_lfsr={
	16,			/* Bit Length */
	0,			/* Reset Value */
	13,			/* Use Bit 13 as XOR input 0 */
	14,			/* Use Bit 14 as XOR input 1 */
	DISC_LFSR_XNOR,		/* Feedback stage1 is XNOR */
	DISC_LFSR_OR,		/* Feedback stage2 is just stage 1 output OR with external feed */
	DISC_LFSR_REPLACE,	/* Feedback stage3 replaces the shifted register contents */
	0x000001,		/* Everything is shifted into the first bit only */
	0,			/* Output is already inverted by XNOR */
	13			/* Output bit */
};

/* Nodes - Inputs */
#define SUBS_SONAR1_EN			NODE_01
#define SUBS_SONAR2_EN			NODE_02
#define SUBS_LAUNCH_DATA		NODE_03
#define SUBS_CRASH_DATA			NODE_04
#define SUBS_CRASH_EN			NODE_05
#define SUBS_EXPLODE_EN			NODE_06
#define SUBS_NOISE_RESET		NODE_07
/* Nodes - Sounds */
#define SUBS_NOISE			NODE_10
#define SUBS_SONAR1_SND			NODE_11
#define SUBS_SONAR2_SND			NODE_12
#define SUBS_LAUNCH_SND			NODE_13
#define SUBS_CRASH_SND			NODE_14
#define SUBS_EXPLODE_SND		NODE_15

static DISCRETE_SOUND_START(subs_sound_interface)
	/************************************************/
	/* subs  Effects Relataive Gain Table           */
	/*                                              */
	/* NOTE: The schematic does not show the amp    */
	/*  stage so I will assume a 5K volume control. */
	/*                                              */
	/* Effect       V-ampIn   Gain ratio  Relative  */
	/* Sonar         2.33     5/(5+2.7)     320.8   */
	/* Launch        3.8      5/(5+47)       77.5   */
	/* Crash         3.8      5/(5+22)      149.2   */
	/* Explosion     10.0     5/(5+5.6)    1000.0   */
	/************************************************/

	/************************************************/
	/* Input register mapping for subs              */
	/************************************************/
	/*                   NODE              ADDR  MASK    GAIN      OFFSET  INIT */
	DISCRETE_INPUTX     (SUBS_SONAR1_EN,   0x00, 0x000f, 400.0,     0,     0.0)
	DISCRETE_INPUTX     (SUBS_SONAR2_EN,   0x01, 0x000f, 400.0,     0,     0.0)
	DISCRETE_INPUTX     (SUBS_LAUNCH_DATA, 0x02, 0x000f,  77.5/15,  0,     0.0)
	DISCRETE_INPUTX     (SUBS_CRASH_DATA,  0x03, 0x000f, 149.2/15,  0,     0.0)
	DISCRETE_INPUT      (SUBS_CRASH_EN,    0x04, 0x000f,                   0.0)
	DISCRETE_INPUT      (SUBS_EXPLODE_EN,  0x05, 0x000f,                   0.0)
	DISCRETE_INPUT_PULSE(SUBS_NOISE_RESET, 0x06, 0x000f,                   1.0)

	/************************************************/
	/* Noise source                                 */
	/* LFSR clk = 256H = 15750.0Hz                  */
	/************************************************/
	DISCRETE_LFSR_NOISE(SUBS_NOISE, SUBS_NOISE_RESET, SUBS_NOISE_RESET, 15750.0, 1.0, 0, 0, &subs_lfsr)

	/************************************************/
	/* Launch is just amplitude contolled noise     */
	/************************************************/
	DISCRETE_MULTIPLY(SUBS_LAUNCH_SND, 1, SUBS_NOISE, SUBS_LAUNCH_DATA)

	/************************************************/
	/* Crash resamples the noise at 8V and then     */
	/* controls the amplitude.                      */
	/* 8V = Hsync/2/8 = 15750/2/8                   */
	/************************************************/
	DISCRETE_SQUAREWFIX(NODE_20, 1, 15750.0/2/8, 1.0, 50, 1.0/2, 0)	/* Resample freq. */
	DISCRETE_SAMPLHOLD(NODE_21, 1, SUBS_NOISE, NODE_20, DISC_SAMPHOLD_REDGE)
	DISCRETE_MULTIPLY(SUBS_CRASH_SND, SUBS_CRASH_EN, NODE_21, SUBS_CRASH_DATA)

	/************************************************/
	/* Explode filters the crash sound.             */
	/* I'm not sure of the exact filter freq.       */
	/************************************************/
	DISCRETE_TRANSFORM3(NODE_30, 1, NODE_21, SUBS_CRASH_DATA, 1000.0 / 149.2, "01*2*")
	DISCRETE_FILTER2(SUBS_EXPLODE_SND, SUBS_EXPLODE_EN, NODE_30, 100.0, (1.0 / 7.6), DISC_FILTER_BANDPASS)

	/************************************************/
	/* Not sure how the sonar works yet.            */
	/************************************************/
	DISCRETE_RCDISC2(NODE_40, SUBS_SONAR1_EN, SUBS_SONAR1_EN, 680000.0, SUBS_SONAR1_EN, 1000.0, 1e-6)	/* Decay envelope */
	DISCRETE_ADDER2(NODE_41, 1, NODE_40, 800)
	DISCRETE_LOGIC_AND(NODE_42, 1, SUBS_SONAR1_EN, SUBS_NOISE)
	DISCRETE_TRIANGLEWAVE(SUBS_SONAR1_SND, NODE_42, NODE_41, 320.8, 0.0, 0)
	
	DISCRETE_RCDISC2(NODE_50, SUBS_SONAR2_EN, SUBS_SONAR2_EN, 18600.0, SUBS_SONAR2_EN, 20.0, 4.7e-6)	/* Decay envelope */
	DISCRETE_ADDER2(NODE_51, 1, NODE_50, 800)
	DISCRETE_LOGIC_AND(NODE_52, 1, SUBS_SONAR2_EN, SUBS_NOISE)
	DISCRETE_TRIANGLEWAVE(SUBS_SONAR2_SND, NODE_52, NODE_51, 320.8, 0.0, 0)

	/************************************************/
	/* Combine all sound sources.                   */
	/* Add some final gain to get to a good sound   */
	/* level.                                       */
	/************************************************/

	DISCRETE_ADDER4(NODE_90, 1, 0, SUBS_LAUNCH_SND, SUBS_CRASH_SND, SUBS_EXPLODE_SND)
	DISCRETE_ADDER4(NODE_91, 1, 0, SUBS_LAUNCH_SND, SUBS_CRASH_SND, SUBS_EXPLODE_SND)
	DISCRETE_GAIN(NODE_92, NODE_90, 65534.0/(320.8+77.5+149.2+1000.0))
	DISCRETE_GAIN(NODE_93, NODE_91, 65534.0/(320.8+77.5+149.2+1000.0))
	DISCRETE_OUTPUT(NODE_92, MIXER(100,MIXER_PAN_LEFT))
	DISCRETE_OUTPUT(NODE_93, MIXER(100,MIXER_PAN_RIGHT))
DISCRETE_SOUND_END


/*************************************
 *
 *	Machine driver
 *
 *************************************/

static MACHINE_DRIVER_START( subs )

	/* basic machine hardware */
	MDRV_CPU_ADD(M6502,12096000/16)		/* clock input is the "4H" signal */
	MDRV_CPU_MEMORY(readmem,writemem)
	MDRV_CPU_VBLANK_INT(subs_interrupt,4)

	MDRV_FRAMES_PER_SECOND(57)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)
	
	MDRV_MACHINE_INIT(subs)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_DUAL_MONITOR)
	MDRV_ASPECT_RATIO(8,3)
	MDRV_SCREEN_SIZE(64*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 64*8-1, 0*8, 28*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(4)
	MDRV_COLORTABLE_LENGTH(sizeof(colortable_source) / sizeof(colortable_source[0]))
	
	MDRV_PALETTE_INIT(subs)
	MDRV_VIDEO_START(generic)
	MDRV_VIDEO_UPDATE(subs)

	/* sound hardware */
	MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
	MDRV_SOUND_ADD_TAG("discrete", DISCRETE, subs_sound_interface)
MACHINE_DRIVER_END



/*************************************
 *
 *	ROM definitions
 *
 *************************************/

ROM_START( subs )
	ROM_REGION( 0x10000, REGION_CPU1, 0 ) /* 64k for code */
	ROM_LOAD( "34190.p1",     0x2800, 0x0800, CRC(a88aef21) SHA1(3811c137041ca43a6e49fbaf7d9d8ef37ba190a2) )
	ROM_LOAD( "34191.p2",     0x3000, 0x0800, CRC(2c652e72) SHA1(097b665e803cbc57b5a828403a8d9a258c19e97f) )
	ROM_LOAD( "34192.n2",     0x3800, 0x0800, CRC(3ce63d33) SHA1(a413cb3e0d03dc40a50f5b03b76a4edbe7906f3e) )
	ROM_RELOAD(               0xf800, 0x0800 )
	/* Note: These are being loaded into a bogus location, */
	/*		 They are nibble wide rom images which will be */
	/*		 merged and loaded into the proper place by    */
	/*		 subs_rom_init()							   */
	ROM_LOAD( "34196.e2",     0x8000, 0x0100, CRC(7c7a04c3) SHA1(269d9f7573cc5da4412f53d647127c4884435353) )	/* ROM 0 D4-D7 */
	ROM_LOAD( "34194.e1",     0x9000, 0x0100, CRC(6b1c4acc) SHA1(3a743b721d9e7e9bdc4533aeeab294eb0ea27500) )	/* ROM 0 D0-D3 */

	ROM_REGION( 0x0800, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "34211.m4",     0x0000, 0x0800, CRC(fa8d4409) SHA1(a83b7a835212d31fe421d537fa0d78f234c26f5b) )	/* Playfield */

	ROM_REGION( 0x0800, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "34216.d7",     0x0000, 0x0200, CRC(941d28b4) SHA1(89388ec06546dc567aa5dbc6a7898974f2871ecc) )	/* Motion */
	ROM_LOAD( "34218.e7",     0x0200, 0x0200, CRC(f4f4d874) SHA1(d99ad9a74611f9851f6bfa6000ebd70e1a364f5d) )	/* Motion */
	ROM_LOAD( "34217.d8",     0x0400, 0x0200, CRC(a7a60da3) SHA1(34fc21cc1ca69d58d3907094dc0a3faaf6f461b3) )	/* Motion */
	ROM_LOAD( "34219.e8",     0x0600, 0x0200, CRC(99a5a49b) SHA1(2cb429f8de73c7d78dc83e47f1448ea4340c333d) )	/* Motion */
ROM_END



/*************************************
 *
 *	Driver initialization
 *
 *************************************/

static DRIVER_INIT( subs )
{
	unsigned char *rom = memory_region(REGION_CPU1);
	int i;

	/* Merge nibble-wide roms together,
	   and load them into 0x2000-0x20ff */

	for(i=0;i<0x100;i++)
		rom[0x2000+i] = (rom[0x8000+i]<<4)+rom[0x9000+i];
}



/*************************************
 *
 *	Game drivers
 *
 *************************************/

GAMEX( 1977, subs, 0, subs, subs, subs, ROT0, "Atari", "Subs", GAME_IMPERFECT_SOUND )
