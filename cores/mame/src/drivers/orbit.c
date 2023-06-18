/***************************************************************************

Atari Orbit Driver

  game 0 = beginner slow
  game 1 = beginner medium
  game 2 = beginner fast
  game 3 = intermediate slow
  game 4 = intermediate fast
  game 5 = expert fast shells only
  game 6 = expert slow
  game 7 = expert medium
  game 8 = expert fast
  game 9 = super expert

  Flip screen DIP doesn't work because it's not supported by the game.

***************************************************************************/

#include "driver.h"

extern VIDEO_START( orbit );
extern VIDEO_UPDATE( orbit );

extern UINT8* orbit_playfield_ram;
extern UINT8* orbit_sprite_ram;

extern WRITE_HANDLER( orbit_playfield_w );
extern WRITE_HANDLER( orbit_sprite_w );

static int orbit_nmi_enable;

static UINT8 orbit_misc_flags;


static INTERRUPT_GEN( orbit_interrupt )
{
	if (orbit_nmi_enable)
	{
		cpu_set_nmi_line(0, PULSE_LINE);
	}
}


static void update_misc_flags(UINT8 val)
{
	orbit_misc_flags = val;

	/* BIT0 => UNUSED       */
	/* BIT1 => LOCKOUT      */
	/* BIT2 => NMI ENABLE   */
	/* BIT3 => HEAT RST LED */
	/* BIT4 => PANEL BUS OC */
	/* BIT5 => PANEL STROBE */
	/* BIT6 => HYPER LED    */
	/* BIT7 => WARNING SND  */

	orbit_nmi_enable = (orbit_misc_flags >> 2) & 1;
	discrete_sound_w(5, (orbit_misc_flags >> 7) & 1);

	set_led_status(0, orbit_misc_flags & 0x08);
	set_led_status(1, orbit_misc_flags & 0x40);

	coin_lockout_w(0, !(orbit_misc_flags & 0x02));
	coin_lockout_w(1, !(orbit_misc_flags & 0x02));
}


static MACHINE_INIT( orbit )
{
	update_misc_flags(0);
}


WRITE_HANDLER( orbit_note_w )
{
	discrete_sound_w(0, (~data) & 0xff);
}

WRITE_HANDLER( orbit_note_amp_w )
{
	discrete_sound_w(1, data & 0x0f);
	discrete_sound_w(2, (data >> 4) & 0x0f);
}

WRITE_HANDLER( orbit_noise_amp_w )
{
	discrete_sound_w(3, data & 0x0f);
	discrete_sound_w(4, (data & 0xf0) >> 4);
}

WRITE_HANDLER( orbit_noise_rst_w )
{
	discrete_sound_w(6, 0);
}


WRITE_HANDLER( orbit_misc_w )
{
	UINT8 bit = offset >> 1;

	if (offset & 1)
	{
		update_misc_flags(orbit_misc_flags | (1 << bit));
	}
	else
	{
		update_misc_flags(orbit_misc_flags & ~(1 << bit));
	}
}


WRITE_HANDLER( orbit_zeropage_w )
{
	memory_region(REGION_CPU1)[offset & 0xff] = data;
}


READ_HANDLER( orbit_zeropage_r )
{
	return memory_region(REGION_CPU1)[offset & 0xff];
}


static MEMORY_READ_START( orbit_readmem )
	{ 0x0000, 0x00ff, MRA_RAM },
	{ 0x0100, 0x07ff, orbit_zeropage_r },
	{ 0x0800, 0x0800, input_port_0_r },
	{ 0x1000, 0x1000, input_port_1_r },
	{ 0x1800, 0x1800, input_port_2_r },
	{ 0x2000, 0x2000, input_port_3_r },
	{ 0x2800, 0x2800, input_port_4_r },
	{ 0x3000, 0x33ff, MRA_RAM },
	{ 0x6800, 0x7fff, MRA_ROM }, /* program */
	{ 0xfc00, 0xffff, MRA_ROM }, /* program mirror */
MEMORY_END

static MEMORY_WRITE_START( orbit_writemem )
	{ 0x0000, 0x07ff, orbit_zeropage_w },
	{ 0x3000, 0x33bf, orbit_playfield_w, &orbit_playfield_ram },
	{ 0x33c0, 0x33ff, orbit_sprite_w, &orbit_sprite_ram },
	{ 0x3400, 0x37bf, orbit_playfield_w },
	{ 0x37c0, 0x37ff, orbit_sprite_w },
	{ 0x3800, 0x3800, orbit_note_w },
	{ 0x3900, 0x3900, orbit_noise_amp_w },
	{ 0x3a00, 0x3a00, orbit_note_amp_w },
	{ 0x3c00, 0x3c0f, orbit_misc_w },
	{ 0x3e00, 0x3e00, orbit_noise_rst_w },
	{ 0x3f00, 0x3f00, watchdog_reset_w },
	{ 0x6800, 0x7fff, MWA_ROM }, /* program */
	{ 0xfc00, 0xffff, MWA_ROM }, /* program mirror */
MEMORY_END


INPUT_PORTS_START( orbit )
	PORT_START /* 0800 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_PLAYER1 ) /* actually buttons */
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_TILT )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN2 )

	PORT_START /* 1000 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_PLAYER2 ) /* actually buttons */
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_SERVICE( 0x40, IP_ACTIVE_LOW )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN1 )

	PORT_START /* 1800 */
	PORT_DIPNAME( 0x07, 0x00, "Play Time Per Credit" )
	PORT_DIPSETTING( 0x00, "0:30" )
	PORT_DIPSETTING( 0x01, "1:00" )
	PORT_DIPSETTING( 0x02, "1:30" )
	PORT_DIPSETTING( 0x03, "2:00" )
	PORT_DIPSETTING( 0x04, "2:30" )
	PORT_DIPSETTING( 0x05, "3:00" )
	PORT_DIPSETTING( 0x06, "3:30" )
	PORT_DIPSETTING( 0x07, "4:00" )
	PORT_DIPNAME( 0x18, 0x00, "Language" )
	PORT_DIPSETTING( 0x00, "English" )
	PORT_DIPSETTING( 0x08, "Spanish" )
	PORT_DIPSETTING( 0x10, "French" )
	PORT_DIPSETTING( 0x18, "German" )
	PORT_DIPNAME( 0x20, 0x00, DEF_STR( Free_Play ))
	PORT_DIPSETTING( 0x00, DEF_STR( Off ))
	PORT_DIPSETTING( 0x20, DEF_STR( On ))
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Unknown )) /* probably unused */
	PORT_DIPSETTING( 0x00, DEF_STR( Off ))
	PORT_DIPSETTING( 0x40, DEF_STR( On ))
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Unknown )) /* probably unused */
	PORT_DIPSETTING( 0x00, DEF_STR( Off ))
	PORT_DIPSETTING( 0x80, DEF_STR( On ))

	PORT_START /* 2000 */
	PORT_BITX( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1, "Game Reset", KEYCODE_PLUS_PAD, IP_JOY_NONE )
	PORT_BITX( 0x02, IP_ACTIVE_LOW, IPT_BUTTON1, "Game 9", KEYCODE_9_PAD, IP_JOY_NONE )
	PORT_BITX( 0x04, IP_ACTIVE_LOW, IPT_BUTTON1, "Game 8", KEYCODE_8_PAD, IP_JOY_NONE )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Flip_Screen ))
	PORT_DIPSETTING( 0x00, DEF_STR( Off ))
	PORT_DIPSETTING( 0x08, DEF_STR( On ))
	PORT_BITX( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1, "Heat Reset", KEYCODE_ENTER_PAD, IP_JOY_NONE )
	PORT_DIPNAME( 0x20, 0x20, "NEXT TEST" ) /* should be off */
	PORT_DIPSETTING( 0x20, DEF_STR( Off ))
	PORT_DIPSETTING( 0x00, DEF_STR( On ))
	PORT_DIPNAME( 0x40, 0x40, "DIAG TEST" ) /* should be off */
	PORT_DIPSETTING( 0x40, DEF_STR( Off ))
	PORT_DIPSETTING( 0x00, DEF_STR( On ))
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_VBLANK )

	PORT_START /* 2800 */
	PORT_BITX( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1, "Game 7 / Strong Gravity",  KEYCODE_7_PAD, IP_JOY_NONE )
	PORT_BITX( 0x02, IP_ACTIVE_LOW, IPT_BUTTON1, "Game 6 / Stars",  KEYCODE_6_PAD, IP_JOY_NONE )
	PORT_BITX( 0x04, IP_ACTIVE_LOW, IPT_BUTTON1, "Game 5 / Unlimited Supplies",  KEYCODE_5_PAD, IP_JOY_NONE )
	PORT_BITX( 0x08, IP_ACTIVE_LOW, IPT_BUTTON1, "Game 4 / Space Stations",  KEYCODE_4_PAD, IP_JOY_NONE )
	PORT_BITX( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1, "Game 3 / Black Hole",  KEYCODE_3_PAD, IP_JOY_NONE )
	PORT_BITX( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1, "Game 2 / Zero Gravity",  KEYCODE_2_PAD, IP_JOY_NONE )
	PORT_BITX( 0x40, IP_ACTIVE_LOW, IPT_BUTTON1, "Game 1 / Negative Gravity",  KEYCODE_1_PAD, IP_JOY_NONE )
	PORT_BITX( 0x80, IP_ACTIVE_LOW, IPT_BUTTON1, "Game 0 / Bounce Back",  KEYCODE_0_PAD, IP_JOY_NONE )
INPUT_PORTS_END


static struct GfxLayout orbit_full_sprite_layout =
{
	8, 32,    /* width, height */
	128,      /* total         */
	1,        /* planes        */
	{ 0 },    /* plane offsets */
	{
		0, 1, 2, 3, 4, 5, 6, 7
	},
	{
		0x00, 0x08, 0x10, 0x18, 0x20, 0x28, 0x30, 0x38,
		0x40, 0x48, 0x50, 0x58, 0x60, 0x68, 0x70, 0x78,
		0x80, 0x88, 0x90, 0x98, 0xA0, 0xA8, 0xB0, 0xB8,
		0xC0, 0xC8, 0xD0, 0xD8, 0xE0, 0xE8, 0xF0, 0xF8
	},
	0x100     /* increment */
};


static struct GfxLayout orbit_upper_sprite_layout =
{
	8, 16,    /* width, height */
	128,      /* total         */
	1,        /* planes        */
	{ 0 },    /* plane offsets */
	{
		0, 1, 2, 3, 4, 5, 6, 7
	},
	{
		0x00, 0x08, 0x10, 0x18, 0x20, 0x28, 0x30, 0x38,
		0x40, 0x48, 0x50, 0x58, 0x60, 0x68, 0x70, 0x78
	},
	0x100     /* increment */
};


static struct GfxLayout orbit_lower_sprite_layout =
{
	8, 16,    /* width, height */
	128,      /* total         */
	1,        /* planes        */
	{ 0 },    /* plane offsets */
	{
		0, 1, 2, 3, 4, 5, 6, 7
	},
	{
		0x80, 0x88, 0x90, 0x98, 0xA0, 0xA8, 0xB0, 0xB8,
		0xC0, 0xC8, 0xD0, 0xD8, 0xE0, 0xE8, 0xF0, 0xF8
	},
	0x100     /* increment */
};


static struct GfxLayout orbit_tile_layout =
{
	16, 16,   /* width, height */
	64,       /* total         */
	1,        /* planes        */
	{ 0 },    /* plane offsets */
	{
		0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7
	},
	{
		0x00, 0x00, 0x08, 0x08, 0x10, 0x10, 0x18, 0x18,
		0x20, 0x20, 0x28, 0x28, 0x30, 0x30, 0x38, 0x38
	},
	0x40      /* increment */
};


static struct GfxDecodeInfo orbit_gfx_decode_info[] =
{
	{ REGION_GFX1, 0, &orbit_full_sprite_layout, 0, 1 },
	{ REGION_GFX1, 0, &orbit_upper_sprite_layout, 0, 1 },
	{ REGION_GFX1, 0, &orbit_lower_sprite_layout, 0, 1 },
	{ REGION_GFX2, 0, &orbit_tile_layout, 0, 1 },
	{ -1 } /* end of array */
};


static PALETTE_INIT( orbit )
{
	palette_set_color(0, 0x00, 0x00, 0x00);
	palette_set_color(1, 0xFF, 0xFF, 0xFF);
}


/************************************************************************/
/* orbit Sound System Analog emulation                                  */
/************************************************************************/

const struct discrete_lfsr_desc orbit_lfsr = {
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
#define ORBIT_NOTE_FREQ		NODE_01
#define ORBIT_ANOTE1_AMP	NODE_02
#define ORBIT_ANOTE2_AMP	NODE_03
#define ORBIT_NOISE1_AMP	NODE_04
#define ORBIT_NOISE2_AMP	NODE_05
#define ORBIT_WARNING_EN	NODE_06
#define ORBIT_NOISE_EN		NODE_07
/* Nodes - Sounds */
#define ORBIT_NOISE		NODE_10
#define ORBIT_NOISE1_SND	NODE_11
#define ORBIT_NOISE2_SND	NODE_12
#define ORBIT_ANOTE1_SND	NODE_13
#define ORBIT_ANOTE2_SND	NODE_14
#define ORBIT_WARNING_SND	NODE_15

static DISCRETE_SOUND_START(orbit_sound_interface)
	/************************************************/
	/* orbit  Effects Relataive Gain Table          */
	/*                                              */
	/* Effect  V-ampIn  Gain ratio        Relative  */
	/* Note     3.8     10/(20.1+.47+.55)   962.1   */
	/* Warning  3.8     10/(20.1+6.8)       755.4   */
	/* Noise    3.8     10/(20.1+.22)      1000.0   */
	/************************************************/

	/************************************************/
	/* Input register mapping for orbit             */
	/************************************************/
	/*                   NODE              ADDR  MASK    GAIN      OFFSET  INIT */
	DISCRETE_INPUT      (ORBIT_NOTE_FREQ,  0x00, 0x000f,                   0.0)
	DISCRETE_INPUTX     (ORBIT_ANOTE1_AMP, 0x01, 0x000f, 962.1/15,  0,     0.0)
	DISCRETE_INPUTX     (ORBIT_ANOTE2_AMP, 0x02, 0x000f, 962.1/15,  0,     0.0)
	DISCRETE_INPUTX     (ORBIT_NOISE1_AMP, 0x03, 0x000f, 1000.0/15, 0,     0.0)
	DISCRETE_INPUTX     (ORBIT_NOISE2_AMP, 0x04, 0x000f, 1000.0/15, 0,     0.0)
	DISCRETE_INPUT      (ORBIT_WARNING_EN, 0x05, 0x000f,                   0.0)
	DISCRETE_INPUT_PULSE(ORBIT_NOISE_EN,   0x06, 0x000f,                   1.0)

	/************************************************/
	/* Warning is just a triggered 2V signal        */
	/************************************************/
	DISCRETE_SQUAREWFIX(ORBIT_WARNING_SND, ORBIT_WARNING_EN, 15750.0/4, 755.4, 50.0, 0, 0.0)

	/************************************************/
	/* Noise effect is variable amplitude, filtered */
	/* random noise.                                */
	/* LFSR clk = 32V= 15750.0Hz/2/32               */
	/************************************************/
	DISCRETE_LFSR_NOISE(ORBIT_NOISE, ORBIT_NOISE_EN, ORBIT_NOISE_EN, 15750.0/2/32, 1, 0, 0, &orbit_lfsr)
	DISCRETE_MULTIPLY(NODE_20, 1, ORBIT_NOISE, ORBIT_NOISE1_AMP)
	DISCRETE_MULTIPLY(NODE_21, 1, ORBIT_NOISE, ORBIT_NOISE2_AMP)
	DISCRETE_FILTER2(ORBIT_NOISE1_SND, 1, NODE_20, 160.0, (1.0 / 10.0), DISC_FILTER_BANDPASS)
	DISCRETE_FILTER2(ORBIT_NOISE2_SND, 1, NODE_21, 160.0, (1.0 / 10.0), DISC_FILTER_BANDPASS)

	/************************************************/
	/* Note sound is created by a divider circuit.  */
	/* The master clock is the 64H signal, which is */
	/* 12.096MHz/256.  This is then sent to a       */
	/* preloadable 8 bit counter, which loads the   */
	/* value from NOTELD when overflowing from 0xFF */
	/* to 0x00.  Therefore it divides by 2 (NOTELD  */
	/* = FE) to 256 (NOTELD = 00).                  */
	/* There is also a final /2 stage.              */
	/* Note that there is no music disable line.    */
	/* When there is no music, the game sets the    */
	/* oscillator to 0Hz.  (NOTELD = FF)            */
	/************************************************/
	DISCRETE_ADDER2(NODE_30, 1, ORBIT_NOTE_FREQ, 1)	/* To get values of 1 - 256 */
	DISCRETE_DIVIDE(NODE_31, 1, 12096000.0/256/2, NODE_30)
	DISCRETE_SQUAREWAVE(ORBIT_ANOTE1_SND, ORBIT_NOTE_FREQ, NODE_31, ORBIT_ANOTE1_AMP, 50.0, 0, 0.0)	/* NOTE=FF Disables audio */
	DISCRETE_SQUAREWAVE(ORBIT_ANOTE2_SND, ORBIT_NOTE_FREQ, NODE_31, ORBIT_ANOTE2_AMP, 50.0, 0, 0.0)	/* NOTE=FF Disables audio */

	/************************************************/
	/* Final gain and ouput.                        */
	/************************************************/
	DISCRETE_ADDER3(NODE_90, 1, ORBIT_WARNING_SND, ORBIT_NOISE1_SND, ORBIT_ANOTE1_SND)
	DISCRETE_ADDER3(NODE_91, 1, ORBIT_WARNING_SND, ORBIT_NOISE2_SND, ORBIT_ANOTE2_SND)
	DISCRETE_GAIN(NODE_92, NODE_90, 65534.0/(962.1+755.4+1000.0))
	DISCRETE_GAIN(NODE_93, NODE_91, 65534.0/(962.1+755.4+1000.0))
	DISCRETE_OUTPUT(NODE_93, MIXER(100,MIXER_PAN_LEFT))
	DISCRETE_OUTPUT(NODE_92, MIXER(100,MIXER_PAN_RIGHT))
DISCRETE_SOUND_END


static MACHINE_DRIVER_START( orbit )

	/* basic machine hardware */
	MDRV_CPU_ADD(M6800, 12096000 / 16)
	MDRV_CPU_MEMORY(orbit_readmem, orbit_writemem)
	MDRV_CPU_VBLANK_INT(irq0_line_pulse, 1)
	MDRV_CPU_PERIODIC_INT(orbit_interrupt, 240)

	MDRV_FRAMES_PER_SECOND(60) /* interlaced */
	MDRV_VBLANK_DURATION((int) ((22. * 1000000) / (262. * 60) + 0.5))
	MDRV_MACHINE_INIT(orbit)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(512, 524)
	MDRV_VISIBLE_AREA(0, 511, 0, 479)
	MDRV_GFXDECODE(orbit_gfx_decode_info)
	MDRV_PALETTE_LENGTH(2)
	MDRV_PALETTE_INIT(orbit)
	MDRV_VIDEO_START(orbit)
	MDRV_VIDEO_UPDATE(orbit)

	/* sound hardware */
	MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
	MDRV_SOUND_ADD_TAG("discrete", DISCRETE, orbit_sound_interface)
MACHINE_DRIVER_END


ROM_START( orbit )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD_NIB_LOW ( "033701.h2", 0x6800, 0x400, CRC(6de43b85) SHA1(1643972f45d3a0dd6540158c575cd84cee2b0c9a) )
	ROM_LOAD_NIB_HIGH( "033693.l2", 0x6800, 0x400, CRC(8878409e) SHA1(a14e0161705bbc230f0aec1837ebc41d62178368) )
	ROM_LOAD_NIB_LOW ( "033702.h1", 0x6C00, 0x400, CRC(8166bdcb) SHA1(b7ae6cd46b4aff6e1e1ec9273cf068dec4a8cd46) )
	ROM_LOAD_NIB_HIGH( "033694.l1", 0x6C00, 0x400, CRC(5337a8ee) SHA1(1606bfa652bb5253c387f11c96d77d7a84983344) )
	ROM_LOAD_NIB_LOW ( "033699.f2", 0x7000, 0x400, CRC(b498b36f) SHA1(5d150af193196fccd7c20ba731a020a9ae75e516) )
	ROM_LOAD_NIB_HIGH( "033691.m2", 0x7000, 0x400, CRC(6cbabb21) SHA1(fffb3f7be73c72b4775d8cdfe174c75ae4389cba) )
	ROM_LOAD_NIB_LOW ( "033700.f1", 0x7400, 0x400, CRC(9807c922) SHA1(b6b62530b24d967104f632540ef98f2b4780c3ed) )
	ROM_LOAD_NIB_HIGH( "033692.m1", 0x7400, 0x400, CRC(96167d1b) SHA1(6f272b2f1b30aa94f51ea5710f4114bfdea19f2c) )
	ROM_LOAD_NIB_LOW ( "033697.e2", 0x7800, 0x400, CRC(19ccf0dc) SHA1(7d12c4985bd0a25ef518246faf2849e5a0cf600b) )
	ROM_LOAD_NIB_HIGH( "033689.n2", 0x7800, 0x400, CRC(ea3b70c1) SHA1(5e985fed057f362deaeb5e4049c4e8c1d449d6e1) )
	ROM_LOAD_NIB_LOW ( "033698.e1", 0x7C00, 0x400, CRC(356a7c32) SHA1(a3496c0f9d9f3e2e0b452cdc0e908dc93d179990) )
	ROM_RELOAD(                     0xFC00, 0x400 )
	ROM_LOAD_NIB_HIGH( "033690.n1", 0x7C00, 0x400, CRC(f756ebd4) SHA1(4e473541b712078c6a81901714a6243de348e543) )
	ROM_RELOAD(                     0xFC00, 0x400 )

	ROM_REGION( 0x1000, REGION_GFX1, ROMREGION_DISPOSE )  /* sprites */
	ROM_LOAD( "033712.b7", 0x0000, 0x800, CRC(cfd43bf2) SHA1(dbca0da6ed355aac921bae5adeef2f384f5fa2c3) )
	ROM_LOAD( "033713.d7", 0x0800, 0x800, CRC(5ac89f4d) SHA1(747889b33cd83510a640e68fb4581a3e881c43a3) )

	ROM_REGION( 0x200, REGION_GFX2, ROMREGION_DISPOSE )   /* tiles */
	ROM_LOAD( "033711.a7", 0x0000, 0x200, CRC(9987174a) SHA1(d2117b6e6d64c29aef8ad8c94256baea493bce5c) )

	ROM_REGION( 0x100, REGION_PROMS, 0 )                  /* sync, unused */
	ROM_LOAD( "033688.p6", 0x0000, 0x100, CRC(ee66ddba) SHA1(5b9ae4cbf019375c8d54528b69280413c641c4f2) )
ROM_END


GAME( 1978, orbit, 0, orbit, orbit, 0, 0, "Atari", "Orbit" )
