/***************************************************************************

	Atari Basketball hardware

	driver by Mike Balfour

	Games supported:
		* Basketball

	Known issues:
		* none at this time

****************************************************************************

	Note:  The original hardware uses the Player 1 and Player 2 Start buttons
	as the Jump/Shoot buttons.	I've taken button 1 and mapped it to the Start
	buttons to keep people from getting confused.

	If you have any questions about how this driver works, don't hesitate to
	ask.  - Mike Balfour (mab22@po.cwru.edu)

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

extern UINT8 *bsktball_motion;

extern WRITE_HANDLER( bsktball_videoram_w );

extern VIDEO_START( bsktball );
extern VIDEO_UPDATE( bsktball );

extern WRITE_HANDLER( bsktball_nmion_w );
extern INTERRUPT_GEN( bsktball_interrupt );
extern WRITE_HANDLER( bsktball_ld1_w );
extern WRITE_HANDLER( bsktball_ld2_w );
extern READ_HANDLER( bsktball_in0_r );
extern WRITE_HANDLER( bsktball_led1_w );
extern WRITE_HANDLER( bsktball_led2_w );
extern WRITE_HANDLER( bsktball_bounce_w );
extern WRITE_HANDLER( bsktball_note_w );
extern WRITE_HANDLER( bsktball_noise_reset_w );

/*************************************
 *
 *	Palette generation
 *
 *************************************/

static unsigned short colortable_source[] =
{
	/* Playfield */
	0x01, 0x00, 0x00, 0x00,
	0x01, 0x03, 0x03, 0x03,

	/* Motion */
	0x01, 0x00, 0x00, 0x00,
	0x01, 0x00, 0x01, 0x00,
	0x01, 0x00, 0x02, 0x00,
	0x01, 0x00, 0x03, 0x00,

	0x01, 0x01, 0x00, 0x00,
	0x01, 0x01, 0x01, 0x00,
	0x01, 0x01, 0x02, 0x00,
	0x01, 0x01, 0x03, 0x00,

	0x01, 0x02, 0x00, 0x00,
	0x01, 0x02, 0x01, 0x00,
	0x01, 0x02, 0x02, 0x00,
	0x01, 0x02, 0x03, 0x00,

	0x01, 0x03, 0x00, 0x00,
	0x01, 0x03, 0x01, 0x00,
	0x01, 0x03, 0x02, 0x00,
	0x01, 0x03, 0x03, 0x00,

	0x01, 0x00, 0x00, 0x01,
	0x01, 0x00, 0x01, 0x01,
	0x01, 0x00, 0x02, 0x01,
	0x01, 0x00, 0x03, 0x01,

	0x01, 0x01, 0x00, 0x01,
	0x01, 0x01, 0x01, 0x01,
	0x01, 0x01, 0x02, 0x01,
	0x01, 0x01, 0x03, 0x01,

	0x01, 0x02, 0x00, 0x01,
	0x01, 0x02, 0x01, 0x01,
	0x01, 0x02, 0x02, 0x01,
	0x01, 0x02, 0x03, 0x01,

	0x01, 0x03, 0x00, 0x01,
	0x01, 0x03, 0x01, 0x01,
	0x01, 0x03, 0x02, 0x01,
	0x01, 0x03, 0x03, 0x01,

	0x01, 0x00, 0x00, 0x02,
	0x01, 0x00, 0x01, 0x02,
	0x01, 0x00, 0x02, 0x02,
	0x01, 0x00, 0x03, 0x02,

	0x01, 0x01, 0x00, 0x02,
	0x01, 0x01, 0x01, 0x02,
	0x01, 0x01, 0x02, 0x02,
	0x01, 0x01, 0x03, 0x02,

	0x01, 0x02, 0x00, 0x02,
	0x01, 0x02, 0x01, 0x02,
	0x01, 0x02, 0x02, 0x02,
	0x01, 0x02, 0x03, 0x02,

	0x01, 0x03, 0x00, 0x02,
	0x01, 0x03, 0x01, 0x02,
	0x01, 0x03, 0x02, 0x02,
	0x01, 0x03, 0x03, 0x02,

	0x01, 0x00, 0x00, 0x03,
	0x01, 0x00, 0x01, 0x03,
	0x01, 0x00, 0x02, 0x03,
	0x01, 0x00, 0x03, 0x03,

	0x01, 0x01, 0x00, 0x03,
	0x01, 0x01, 0x01, 0x03,
	0x01, 0x01, 0x02, 0x03,
	0x01, 0x01, 0x03, 0x03,

	0x01, 0x02, 0x00, 0x03,
	0x01, 0x02, 0x01, 0x03,
	0x01, 0x02, 0x02, 0x03,
	0x01, 0x02, 0x03, 0x03,

	0x01, 0x03, 0x00, 0x03,
	0x01, 0x03, 0x01, 0x03,
	0x01, 0x03, 0x02, 0x03,
	0x01, 0x03, 0x03, 0x03,
};

static PALETTE_INIT( bsktball )
{
	palette_set_color(0,0x00,0x00,0x00); /* BLACK */
	palette_set_color(1,0x80,0x80,0x80); /* LIGHT GREY */
	palette_set_color(2,0x50,0x50,0x50); /* DARK GREY */
	palette_set_color(3,0xff,0xff,0xff); /* WHITE */
	memcpy(colortable,colortable_source,sizeof(colortable_source));
}



/*************************************
 *
 *	Main CPU memory handlers
 *
 *************************************/

static MEMORY_READ_START( readmem )
	{ 0x0000, 0x01ff, MRA_RAM }, /* Zero Page RAM */
	{ 0x0800, 0x0800, bsktball_in0_r },
	{ 0x0802, 0x0802, input_port_5_r },
	{ 0x0803, 0x0803, input_port_6_r },
	{ 0x1800, 0x1cff, MRA_RAM }, /* video ram */
	{ 0x2000, 0x3fff, MRA_ROM }, /* PROGRAM */
	{ 0xfff0, 0xffff, MRA_ROM }, /* PROM8 for 6502 vectors */
MEMORY_END


static MEMORY_WRITE_START( writemem )
	{ 0x0000, 0x01ff, MWA_RAM }, /* WRAM */
	{ 0x1000, 0x1000, MWA_NOP }, /* Timer Reset */
	{ 0x1010, 0x1010, bsktball_bounce_w }, /* Crowd Amp / Bounce */
	{ 0x1022, 0x1023, MWA_NOP }, /* Coin Counter */
	{ 0x1024, 0x1025, bsktball_led1_w }, /* LED 1 */
	{ 0x1026, 0x1027, bsktball_led2_w }, /* LED 2 */
	{ 0x1028, 0x1029, bsktball_ld1_w }, /* LD 1 */
	{ 0x102a, 0x102b, bsktball_ld2_w }, /* LD 2 */
	{ 0x102c, 0x102d, bsktball_noise_reset_w }, /* Noise Reset */
	{ 0x102e, 0x102f, bsktball_nmion_w }, /* NMI On */
	{ 0x1030, 0x1030, bsktball_note_w }, /* Music Ckt Note Dvsr */
	{ 0x1800, 0x1bbf, bsktball_videoram_w, &videoram }, /* DISPLAY */
	{ 0x1bc0, 0x1bff, MWA_RAM, &bsktball_motion },
	{ 0x2000, 0x3fff, MWA_ROM }, /* PROM1-PROM8 */
MEMORY_END



/*************************************
 *
 *	Port definitions
 *
 *************************************/

INPUT_PORTS_START( bsktball )
	PORT_START	/* IN0 */
	PORT_ANALOG( 0xFF, 0x00, IPT_TRACKBALL_X, 100, 10, 0, 0 ) /* Sensitivity, clip, min, max */

	PORT_START	/* IN0 */
	PORT_ANALOG( 0xFF, 0x00, IPT_TRACKBALL_Y, 100, 10, 0, 0 )

	PORT_START	/* IN0 */
	PORT_ANALOG( 0xFF, 0x00, IPT_TRACKBALL_X | IPF_PLAYER2, 100, 10, 0, 0 ) /* Sensitivity, clip, min, max */

	PORT_START	/* IN0 */
	PORT_ANALOG( 0xFF, 0x00, IPT_TRACKBALL_Y | IPF_PLAYER2, 100, 10, 0, 0 )

	PORT_START		/* IN0 */
	PORT_BIT ( 0x01, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT ( 0x02, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT ( 0x04, IP_ACTIVE_LOW, IPT_BUTTON1 ) /* SPARE */
	PORT_BIT ( 0x08, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 ) /* SPARE */
	/* 0x10 - DR0 = PL2 H DIR */
	/* 0x20 - DR1 = PL2 V DIR */
	/* 0x40 - DR2 = PL1 H DIR */
	/* 0x80 - DR3 = PL1 V DIR */

	PORT_START		/* IN2 */
	PORT_BIT ( 0x01, IP_ACTIVE_HIGH, IPT_VBLANK )
	PORT_BIT ( 0x02, IP_ACTIVE_LOW, IPT_TILT )
	PORT_BIT ( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN ) /* SPARE */
	PORT_BIT ( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN ) /* TEST STEP */
	PORT_SERVICE( 0x10, IP_ACTIVE_LOW )
	PORT_BIT ( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN ) /* COIN 0 */
	PORT_BIT ( 0x40, IP_ACTIVE_LOW, IPT_COIN2 ) /* COIN 1 */
	PORT_BIT ( 0x80, IP_ACTIVE_LOW, IPT_COIN1 ) /* COIN 2 */

	PORT_START		/* DSW */
	PORT_DIPNAME( 0x07, 0x00, "Play Time per Credit" )
	PORT_DIPSETTING(	0x07, DEF_STR( Free_Play ) )
	PORT_DIPSETTING(	0x06, "2:30" )
	PORT_DIPSETTING(	0x05, "2:00" )
	PORT_DIPSETTING(	0x04, "1:30" )
	PORT_DIPSETTING(	0x03, "1:15" )
	PORT_DIPSETTING(	0x02, "0:45" )
	PORT_DIPSETTING(	0x01, "0:30" )
	PORT_DIPSETTING(	0x00, "1:00" )
	PORT_DIPNAME( 0x18, 0x00, DEF_STR( Coinage ) )
	PORT_DIPSETTING(	0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(	0x10, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(	0x08, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(	0x18, DEF_STR( 1C_6C ) )
	PORT_DIPNAME( 0x20, 0x00, "Cost" )
	PORT_DIPSETTING(	0x20, "Two Coin Minimum" )
	PORT_DIPSETTING(	0x00, "One Coin Minimum" )
	PORT_DIPNAME( 0xC0, 0x00, "Language" )
	PORT_DIPSETTING(	0xC0, "German" )
	PORT_DIPSETTING(	0x80, "French" )
	PORT_DIPSETTING(	0x40, "Spanish" )
	PORT_DIPSETTING(	0x00, "English" )
INPUT_PORTS_END



/*************************************
 *
 *	Graphics layouts
 *
 *************************************/

static struct GfxLayout charlayout =
{
	8,8,
	64,
	2,
	{ 0, 8*0x800 },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8
};


static struct GfxLayout motionlayout =
{
	8,32,
	64,
	2,
	{ 0, 8*0x800 },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{	0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
		8*8, 9*8, 10*8, 11*8, 12*8, 13*8, 14*8, 15*8,
		16*8, 17*8, 18*8, 19*8, 20*8, 21*8, 22*8, 23*8,
		24*8, 25*8, 26*8, 27*8, 28*8, 29*8, 30*8, 31*8 },
	32*8
};


static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0x0600, &charlayout,   0x00, 0x02 },
	{ REGION_GFX1, 0x0000, &motionlayout, 0x08, 0x40 },
	{ -1 }
};


/************************************************************************/
/* bsktball Sound System Analog emulation                               */
/************************************************************************/

const struct discrete_lfsr_desc bsktball_lfsr={
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
#define BSKTBALL_NOTE_DATA		NODE_01
#define BSKTBALL_CROWD_DATA		NODE_02
#define BSKTBALL_NOISE_EN		NODE_03
#define BSKTBALL_BOUNCE_EN		NODE_04
/* Nodes - Sounds */
#define BSKTBALL_NOISE			NODE_10
#define BSKTBALL_BOUNCE_SND		NODE_11
#define BSKTBALL_NOTE_SND		NODE_12
#define BSKTBALL_CROWD_SND		NODE_13

static DISCRETE_SOUND_START(bsktball_sound_interface)
	/************************************************/
	/* bsktball  Effects Relataive Gain Table       */
	/*                                              */
	/* Effect       V-ampIn   Gain ratio  Relative  */
	/* Note          3.8      47/47        1000.0   */
	/* Bounce        3.8      47/47        1000.0   */
	/* Crowd         3.8      47/220        213.6   */
	/************************************************/

	/************************************************/
	/* Input register mapping for bsktball          */
	/************************************************/
	/*              NODE                 ADDR  MASK    GAIN     OFFSET  INIT */
	DISCRETE_INPUT (BSKTBALL_NOTE_DATA,  0x00, 0x000f,                  0.0)
	DISCRETE_INPUTX(BSKTBALL_CROWD_DATA, 0x01, 0x000f, 213.6/15, 0,     0.0)
	DISCRETE_INPUTX(BSKTBALL_BOUNCE_EN,  0x02, 0x000f, 1000.0/2, 0,     0.0)
	DISCRETE_INPUT (BSKTBALL_NOISE_EN,   0x03, 0x000f,                  0.0)

	/************************************************/
	/* Bounce is a trigger fed directly to the amp  */
	/************************************************/
	DISCRETE_FILTER2(BSKTBALL_BOUNCE_SND, 1, BSKTBALL_BOUNCE_EN, 10.0, 5, DISC_FILTER_HIGHPASS)	/* remove DC*/

	/************************************************/
	/* Crowd effect is variable amplitude, filtered */
	/* random noise.                                */
	/* LFSR clk = 256H = 15750.0Hz                  */
	/************************************************/
	DISCRETE_LFSR_NOISE(BSKTBALL_NOISE, BSKTBALL_NOISE_EN, BSKTBALL_NOISE_EN, 15750.0, BSKTBALL_CROWD_DATA, 0, 0, &bsktball_lfsr)
	DISCRETE_FILTER2(BSKTBALL_CROWD_SND, 1, BSKTBALL_NOISE, 330.0, (1.0 / 7.6), DISC_FILTER_BANDPASS)

	/************************************************/
	/* Note sound is created by a divider circuit.  */
	/* The master clock is the 32H signal, which is */
	/* 12.096MHz/128.  This is then sent to a       */
	/* preloadable 8 bit counter, which loads the   */
	/* value from OUT30 when overflowing from 0xFF  */
	/* to 0x00.  Therefore it divides by 2 (OUT30   */
	/* = FE) to 256 (OUT30 = 00).                   */
	/* There is also a final /2 stage.              */
	/* Note that there is no music disable line.    */
	/* When there is no music, the game sets the    */
	/* oscillator to 0Hz.  (OUT30 = FF)             */
	/************************************************/
	DISCRETE_ADDER2(NODE_20, 1, BSKTBALL_NOTE_DATA, 1)	/* To get values of 1 - 256 */
	DISCRETE_DIVIDE(NODE_21, 1, 12096000.0/128/2, NODE_20)
	DISCRETE_SQUAREWAVE(BSKTBALL_NOTE_SND, BSKTBALL_NOTE_DATA, NODE_21, 1000, 50.0, 0, 0.0)	/* NOTE=FF Disables audio */

	DISCRETE_ADDER3(NODE_90, 1, BSKTBALL_BOUNCE_SND, BSKTBALL_NOTE_SND, BSKTBALL_CROWD_SND)
	DISCRETE_GAIN(NODE_91, NODE_90, 65534.0/(1000.0+1000.0+213.6))
	DISCRETE_OUTPUT(NODE_91, 100)
DISCRETE_SOUND_END


/*************************************
 *
 *	Machine driver
 *
 *************************************/

static MACHINE_DRIVER_START( bsktball )

	/* basic machine hardware */
	MDRV_CPU_ADD(M6502,750000)
	MDRV_CPU_MEMORY(readmem,writemem)
	MDRV_CPU_VBLANK_INT(bsktball_interrupt,8)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(32*8, 28*8)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 0*8, 28*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(4)
	MDRV_COLORTABLE_LENGTH(sizeof(colortable_source) / sizeof(colortable_source[0]))
	
	MDRV_PALETTE_INIT(bsktball)
	MDRV_VIDEO_START(bsktball)
	MDRV_VIDEO_UPDATE(bsktball)

	/* sound hardware */
	MDRV_SOUND_ADD_TAG("discrete", DISCRETE, bsktball_sound_interface)
MACHINE_DRIVER_END



/*************************************
 *
 *	ROM definitions
 *
 *************************************/

ROM_START( bsktball )
	ROM_REGION( 0x10000, REGION_CPU1, 0 ) /* 64k for code */
	ROM_LOAD( "034765.d1",    0x2000, 0x0800, CRC(798cea39) SHA1(b1b709a74258b01b21d7c2038a3b6abe879944c5) )
	ROM_LOAD( "034764.c1",    0x2800, 0x0800, CRC(a087109e) SHA1(f5d6dcccc4a54db35be3d8997bc51e73892747fb) )
	ROM_LOAD( "034766.f1",    0x3000, 0x0800, CRC(a82e9a9f) SHA1(9aca236c5145c04a8aaebb316179482bbdc9ddfc) )
	ROM_LOAD( "034763.b1",    0x3800, 0x0800, CRC(1fc69359) SHA1(a215ba3bb18ea2c57c443dfc4c4a0a3846bbedfe) )
	ROM_RELOAD(               0xf800, 0x0800 )

	ROM_REGION( 0x1000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "034757.a6",    0x0000, 0x0800, CRC(010e8ad3) SHA1(43ce2c2089ec3011e2d28e8257a35efeed0e71c5) )
	ROM_LOAD( "034758.b6",    0x0800, 0x0800, CRC(f7bea344) SHA1(df544bff67bb0334f77cef11792199d9c3f5fdf4) )
ROM_END



/*************************************
 *
 *	Game drivers
 *
 *************************************/

GAME( 1979, bsktball, 0, bsktball, bsktball, 0, ROT0, "Atari", "Basketball" )
