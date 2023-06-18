/***************************************************************************

	Atari Canyon Bomber hardware

	driver by Mike Balfour

	Games supported:
		* Canyon Bomber

	Known issues:
		* none at this time

****************************************************************************

	Memory Map:
        0000-01FF       WRAM
        0400-04FF       W A0=0:MOTOR1, A0=1:MOTOR2
        0500-05FF       W A0=0:EXPLODE, A0=1:TIMER RESET
        0600-067F       W A0=0:WHISTLE1, A0=1:WHISTLE2
        0680-06FF       W A0=0:LED1, A0=1:LED2
        0700-077F       W A0=0:ATTRACT1, A0=1:ATTRACT2
        0800-0FFF       DISPLAY / RAM
        1000-17FF       SWITCHES
        1800-1FFF       OPTIONS
        2000-27FF       ROM1
        2800-2FFF       ROM2
        3000-37FF       ROM3 (Language ROM)
        3800-3FFF       ROM4 (Program ROM)

	If you have any questions about how this driver works, don't hesitate to
	ask.  - Mike Balfour (mab22@po.cwru.edu)

***************************************************************************/

#include "driver.h"

extern WRITE_HANDLER( canyon_videoram_w );

extern VIDEO_START( canyon );
extern VIDEO_UPDATE( canyon );

extern UINT8* canyon_videoram;


/*************************************
 *
 *	Palette generation
 *
 *************************************/

static PALETTE_INIT( canyon )
{
	palette_set_color(0, 0x00, 0x00, 0x00); /* BLACK */
	palette_set_color(1, 0xff, 0xff, 0xff); /* WHITE */
	palette_set_color(2, 0x80, 0x80, 0x80); /* GREY  */

	colortable[0] = 2;
	colortable[1] = 0;
	colortable[2] = 2;
	colortable[3] = 1;
}


/*************************************
 *
 *	Read handlers
 *
 *************************************/

static READ_HANDLER( canyon_switches_r )
{
	UINT8 val = 0;

	if ((readinputport(2) >> (offset & 7)) & 1)
	{
		val |= 0x80;
	}
	if ((readinputport(1) >> (offset & 3)) & 1)
	{
		val |= 0x01;
	}

	return val;
}


static READ_HANDLER( canyon_options_r )
{
	return (readinputport(0) >> (2 * (~offset & 3))) & 3;
}


static READ_HANDLER( canyon_wram_r )
{
	return memory_region(REGION_CPU1)[offset];
}



/*************************************
 *
 *	Write handlers
 *
 *************************************/

static WRITE_HANDLER( canyon_led_w )
{
	set_led_status(offset & 0x01, offset & 0x02);
}


static WRITE_HANDLER( canyon_motor_w )
{
	discrete_sound_w(offset & 0x01, data & 0x0f);
}


static WRITE_HANDLER( canyon_explode_w )
{
	discrete_sound_w(6, data / 16);
}


static WRITE_HANDLER( canyon_attract_w )
{
	discrete_sound_w(4 + (offset & 0x01), !(offset & 0x02));
}


static WRITE_HANDLER( canyon_whistle_w )
{
	discrete_sound_w(2 + (offset & 0x01), (offset & 0x02) >> 1);
}


static WRITE_HANDLER( canyon_wram_w )
{
	memory_region(REGION_CPU1)[offset] = data;
}



/*************************************
 *
 *	Main CPU memory handlers
 *
 *************************************/

static MEMORY_READ_START( readmem )
	{ 0x0000, 0x00ff, MRA_RAM },
	{ 0x0100, 0x01ff, canyon_wram_r },
	{ 0x0800, 0x0bff, MRA_RAM },
	{ 0x1000, 0x17ff, canyon_switches_r },
	{ 0x1800, 0x1fff, canyon_options_r },
	{ 0x2000, 0x3fff, MRA_ROM },
	{ 0xe000, 0xffff, MRA_ROM }, /* mirror for 6502 vectors */
MEMORY_END


static MEMORY_WRITE_START( writemem )
	{ 0x0000, 0x00ff, MWA_RAM },
	{ 0x0100, 0x01ff, canyon_wram_w },
	{ 0x0400, 0x0401, canyon_motor_w },
	{ 0x0500, 0x0500, canyon_explode_w },
	{ 0x0501, 0x0501, MWA_NOP }, /* watchdog, disabled in service mode */
	{ 0x0600, 0x0603, canyon_whistle_w },
	{ 0x0680, 0x0683, canyon_led_w },
	{ 0x0700, 0x0703, canyon_attract_w },
	{ 0x0800, 0x0bff, canyon_videoram_w, &canyon_videoram },
	{ 0x1000, 0x17ff, MWA_NOP }, /* sloppy code writes here */
	{ 0x2000, 0x3fff, MWA_ROM },
MEMORY_END



/*************************************
 *
 *	Port definitions
 *
 *************************************/

INPUT_PORTS_START( canyon )
	PORT_START      /* DSW */
	PORT_DIPNAME( 0x03, 0x00, "Language" )
	PORT_DIPSETTING(    0x00, "English" )
	PORT_DIPSETTING(    0x01, "Spanish" )
	PORT_DIPSETTING(    0x02, "French" )
	PORT_DIPSETTING(    0x03, "German" )
	PORT_DIPNAME( 0x30, 0x00, "Misses Per Play" )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x10, "4" )
	PORT_DIPSETTING(    0x20, "5" )
	PORT_DIPSETTING(    0x30, "6" )
	PORT_DIPNAME( 0xC0, 0x80, DEF_STR( Coinage ))
	PORT_DIPSETTING(    0xC0, DEF_STR( 2C_1C ))
	PORT_DIPSETTING(    0x80, DEF_STR( 1C_1C ))
	PORT_DIPSETTING(    0x40, DEF_STR( 1C_2C ))
	PORT_DIPSETTING(    0x00, DEF_STR( Free_Play ) )

	PORT_START      /* IN1 */
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_PLAYER2 )

	PORT_START      /* IN2 */
	PORT_BIT ( 0x01, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT ( 0x02, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT ( 0x04, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT ( 0x08, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_SERVICE( 0x10, IP_ACTIVE_HIGH )
	PORT_BIT ( 0x20, IP_ACTIVE_LOW, IPT_VBLANK )
	PORT_BITX( 0x40, IP_ACTIVE_HIGH, IPT_BUTTON7, "Hiscore Reset", KEYCODE_H, IP_JOY_DEFAULT )
	PORT_BIT ( 0x80, IP_ACTIVE_HIGH, IPT_TILT ) /* SLAM */

	PORT_START
	PORT_ADJUSTER( 20, "Motor 1 RPM" )

	PORT_START
	PORT_ADJUSTER( 30, "Motor 2 RPM" )

	PORT_START
	PORT_ADJUSTER( 70, "Whistle 1 Freq" )

	PORT_START
	PORT_ADJUSTER( 80, "Whistle 2 Freq" )
INPUT_PORTS_END



/*************************************
 *
 *	Graphics definitions
 *
 *************************************/

static struct GfxLayout tile_layout =
{
	8, 8,
    64,
    1,
    { 0 },
    {
		0x4, 0x5, 0x6, 0x7, 0xC, 0xD, 0xE, 0xF
	},
	{
		0x00, 0x10, 0x20, 0x30, 0x40, 0x50, 0x60, 0x70
	},
	0x80
};


static struct GfxLayout sprite_layout =
{
	32, 16,
	4,
	1,
	{ 0 },
	{
		0x007, 0x006, 0x005, 0x004, 0x003, 0x002, 0x001, 0x000,
		0x00F, 0x00E, 0x00D, 0x00C, 0x00B, 0x00A, 0x009, 0x008,
		0x107, 0x106, 0x105, 0x104, 0x103, 0x102, 0x101, 0x100,
		0x10F, 0x10E, 0x10D, 0x10C, 0x10B, 0x10A, 0x109, 0x108
	},
	{
		0x00, 0x10, 0x20, 0x30, 0x40, 0x50, 0x60, 0x70,
		0x80, 0x90, 0xA0, 0xB0, 0xC0, 0xD0, 0xE0, 0xF0
	},
	0x200
};


static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &tile_layout,   0, 2 },
	{ REGION_GFX2, 0, &sprite_layout, 0, 2 },
	{ -1 }
};



/************************************************************************/
/* canyon Sound System Analog emulation                               */
/************************************************************************/

const struct discrete_555_astbl_desc canyonWhistl555 =
{
	DISC_555_OUT_CAP | DISC_555_OUT_AC,
	5,		/* B+ voltage of 555 */
	5.0 - 1.7,	/* High output voltage of 555 (Usually v555 - 1.7) */
	5.0 * 2.0 /3.0,	/* normally 2/3 of v555 */
	5.0 / 3.0	/* normally 1/3 of v555 */
};

const struct discrete_lfsr_desc canyon_lfsr={
	16,                 /* Bit Length */
	0,                  /* Reset Value */
	6,                  /* Use Bit 6 as XOR input 0 */
	8,                  /* Use Bit 8 as XOR input 1 */
	DISC_LFSR_XNOR,     /* Feedback stage1 is XNOR */
	DISC_LFSR_OR,       /* Feedback stage2 is just stage 1 output OR with external feed */
	DISC_LFSR_REPLACE,  /* Feedback stage3 replaces the shifted register contents */
	0x000001,           /* Everything is shifted into the first bit only */
	0,                  /* Output is not inverted, Active Low Reset */
	15                  /* Output bit */
};

/* Nodes - Inputs */
#define CANYON_MOTORSND1_DATA		NODE_01
#define CANYON_MOTORSND2_DATA		NODE_02
#define CANYON_EXPLODESND_DATA		NODE_03
#define CANYON_WHISTLESND1_EN		NODE_04
#define CANYON_WHISTLESND2_EN		NODE_05
#define CANYON_ATTRACT1_EN		NODE_06
#define CANYON_ATTRACT2_EN		NODE_07
/* Nodes - Sounds */
#define CANYON_MOTORSND1		NODE_10
#define CANYON_MOTORSND2		NODE_11
#define CANYON_EXPLODESND		NODE_12
#define CANYON_WHISTLESND1		NODE_13
#define CANYON_WHISTLESND2		NODE_14
#define CANYON_NOISE			NODE_15
#define CANYON_FINAL_MIX1		NODE_16
#define CANYON_FINAL_MIX2		NODE_17

static DISCRETE_SOUND_START(canyon_sound_interface)
	/************************************************/
	/* Canyon sound system: 5 Sound Sources         */
	/*                     Relative Volume          */
	/*    1/2) Motor           14.29%               */
	/*      3) Explode        100.00%               */
	/*    4/5) Whistle         51.94%               */
	/* Relative volumes calculated from resitor     */
	/* network in combiner circuit taking voltages  */
	/* into account                                 */
	/*                                              */
	/* Motor   3.8V * 5/(5+100) = 0.1810            */
	/* Explode 3.8V * 5/(5+10)  = 1.2667            */
	/* Whistle 5.0V * 5/(5+33)  = 0.6579            */
	/*                                              */
	/*  Discrete sound mapping via:                 */
	/*     discrete_sound_w($register,value)        */
	/*  $00 - Motorsound frequency 1                */
	/*  $01 - Motorsound frequency 2                */
	/*  $02 - Whistle enable 1                      */
	/*  $03 - Whistle enable 2                      */
	/*  $04 - Attract enable 1                      */
	/*  $05 - Attract enable 2                      */
	/*  $06 - Explode volume                        */
	/*                                              */
	/************************************************/

	/************************************************/
	/* Input register mapping for canyon            */
	/************************************************/
	/*                   NODE                  ADDR  MASK    GAIN    OFFSET  INIT */
	DISCRETE_INPUTX(CANYON_MOTORSND1_DATA    , 0x00, 0x000f, -1.0   , 15.0,   0.0)
	DISCRETE_INPUTX(CANYON_MOTORSND2_DATA    , 0x01, 0x000f, -1.0   , 15.0,   0.0)
	DISCRETE_INPUT (CANYON_WHISTLESND1_EN    , 0x02, 0x000f,                  0.0)
	DISCRETE_INPUT (CANYON_WHISTLESND2_EN    , 0x03, 0x000f,                  0.0)
	DISCRETE_INPUT (CANYON_ATTRACT1_EN       , 0x04, 0x000f,                  0.0)
	DISCRETE_INPUT (CANYON_ATTRACT2_EN       , 0x05, 0x000f,                  0.0)
	DISCRETE_INPUTX(CANYON_EXPLODESND_DATA   , 0x06, 0x000f, 1000.0/15.0, 0,  0.0)

	/************************************************/
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
	/* to a 1uf capacitor on the R-ladder.          */
	/* Note the VCO freq. is controlled by a 250k   */
	/* pot.  The freq. used here is for the pot set */
	/* to 125k.  The low freq is allways the same.  */
	/* This adjusts the high end.                   */
	/* 0k = 214Hz.   250k = 4416Hz                  */
	/************************************************/
	DISCRETE_RCFILTER(NODE_20, 1, CANYON_MOTORSND1_DATA, 123000, 1e-6)
	DISCRETE_ADJUSTMENT(NODE_21, 1, (214.0-27.0)/12/15, (4416.0-27.0)/12/15, DISC_LOGADJ, 3)
	DISCRETE_MULTIPLY(NODE_22, 1, NODE_20, NODE_21)

	DISCRETE_MULTADD(NODE_23, 1, NODE_22, 2, 27.0/6)	/* F1 = /12*2 = /6 */
	DISCRETE_SQUAREWAVE(NODE_24, 1, NODE_23, (142.9/3), 50.0, 0, 0)
	DISCRETE_RCFILTER(NODE_25, 1, NODE_24, 10000, 1e-7)

	DISCRETE_MULTADD(NODE_26, 1, NODE_22, 3, 27.0/4)	/* F2 = /12*3 = /4 */
	DISCRETE_SQUAREWAVE(NODE_27, 1, NODE_26, (142.9/3), 50.0, 0, 0)
	DISCRETE_RCFILTER(NODE_28, 1, NODE_27, 10000, 1e-7)

	DISCRETE_MULTADD(NODE_29, 1, NODE_22, 4, 27.0/3)	/* F3 = /12*4 = /3 */
	DISCRETE_SQUAREWAVE(NODE_30, 1, NODE_29, (142.9/3), 100.0/3, 0, 360.0/3)
	DISCRETE_RCFILTER(NODE_31, 1, NODE_30, 10000, 1e-7)

	DISCRETE_ADDER3(CANYON_MOTORSND1, CANYON_ATTRACT1_EN, NODE_25, NODE_28, NODE_31)

	/************************************************/
	/* The motor2 sound is basically the same as    */
	/* for 1.  But I shifted the frequencies up for */
	/* it to sound different from motor 1.          */
	/************************************************/
	DISCRETE_RCFILTER(NODE_40, 1, CANYON_MOTORSND2_DATA, 123000, 1e-6)
	DISCRETE_ADJUSTMENT(NODE_41, 1, (214.0-27.0)/12/15, (4416.0-27.0)/12/15, DISC_LOGADJ, 4)
	DISCRETE_MULTIPLY(NODE_42, 1, NODE_40, NODE_41)

	DISCRETE_MULTADD(NODE_43, 1, NODE_42, 2, 27.0/6)	/* F1 = /12*2 = /6 */
	DISCRETE_SQUAREWAVE(NODE_44, 1, NODE_43, (142.9/3), 50.0, 0, 0)
	DISCRETE_RCFILTER(NODE_45, 1, NODE_44, 10000, 1e-7)

	DISCRETE_MULTADD(NODE_46, 1, NODE_42, 3, 27.0/4)	/* F2 = /12*3 = /4 */
	DISCRETE_SQUAREWAVE(NODE_47, 1, NODE_46, (142.9/3), 50.0, 0, 0)
	DISCRETE_RCFILTER(NODE_48, 1, NODE_47, 10000, 1e-7)

	DISCRETE_MULTADD(NODE_49, 1, NODE_42, 4, 27.0/3)	/* F3 = /12*4 = /3 */
	DISCRETE_SQUAREWAVE(NODE_50, 1, NODE_49, (142.9/3), 100.0/3, 0, 360.0/3)
	DISCRETE_RCFILTER(NODE_51, 1, NODE_50, 10000, 1e-7)

	DISCRETE_ADDER3(CANYON_MOTORSND2, CANYON_ATTRACT2_EN, NODE_45, NODE_48, NODE_51)

	/************************************************/
	/* Explode circuit is built around a noise      */
	/* generator built from 2 shift registers that  */
	/* are clocked by the 2V signal.                */
	/* 2V = HSYNC/4                                 */
	/*    = 15750/4                                 */
	/* Output is binary weighted with 4 bits of     */
	/* crash volume.                                */
	/************************************************/
	DISCRETE_LOGIC_OR(NODE_60, 1, CANYON_ATTRACT1_EN, CANYON_ATTRACT2_EN)
	DISCRETE_LFSR_NOISE(CANYON_NOISE, NODE_60, NODE_60, 15750.0/4, 1.0, 0, 0, &canyon_lfsr)

	DISCRETE_MULTIPLY(NODE_61, 1, CANYON_NOISE, CANYON_EXPLODESND_DATA)
	DISCRETE_RCFILTER(CANYON_EXPLODESND, 1, NODE_61, 545, 5e-6)

	/************************************************/
	/* Whistle circuit is a 555 capacitor charge    */
	/* waveform.  The original game pot varies from */
	/* 0-100k, but we are going to limit it because */
	/* below 50k the frequency is too high.         */
	/* When triggered it starts at it's highest     */
	/* frequency, then decays at the rate set by    */
	/* a 68k resistor and 22uf capacitor.           */
	/************************************************/
	DISCRETE_ADJUSTMENT(NODE_70, 1, 50000, 100000, DISC_LINADJ, 5)	/* R59 */
	DISCRETE_MULTADD(NODE_71, 1, CANYON_WHISTLESND1_EN, 3.05-0.33, 0.33)
	DISCRETE_RCDISC2(NODE_72, CANYON_WHISTLESND1_EN, NODE_71, 1.0, NODE_71, 68000.0, 2.2e-5)	/* CV */
	DISCRETE_555_ASTABLE(NODE_73, CANYON_WHISTLESND1_EN, 33000, NODE_70, 1e-8, NODE_72, &canyonWhistl555)
	DISCRETE_MULTIPLY(CANYON_WHISTLESND1, CANYON_WHISTLESND1_EN, NODE_73, 519.4/3.3)

	DISCRETE_ADJUSTMENT(NODE_75, 1, 50000, 100000, DISC_LINADJ, 6)	/* R69 */
	DISCRETE_MULTADD(NODE_76, 1, CANYON_WHISTLESND2_EN, 3.05-0.33, 0.33)
	DISCRETE_RCDISC2(NODE_77, CANYON_WHISTLESND2_EN, NODE_76, 1.0, NODE_76, 68000.0, 2.2e-5)	/* CV */
	DISCRETE_555_ASTABLE(NODE_78, CANYON_WHISTLESND2_EN, 33000, NODE_75, 1e-8, NODE_77, &canyonWhistl555)
	DISCRETE_MULTIPLY(CANYON_WHISTLESND2, CANYON_WHISTLESND2_EN, NODE_78, 519.4/3.3)

	/************************************************/
	/* Combine all 5 sound sources.                 */
	/* Add some final gain to get to a good sound   */
	/* level.                                       */
	/************************************************/
	DISCRETE_ADDER3(NODE_90, 1, CANYON_MOTORSND1, CANYON_EXPLODESND, CANYON_WHISTLESND1)
	DISCRETE_ADDER3(NODE_91, 1, CANYON_MOTORSND2, CANYON_EXPLODESND, CANYON_WHISTLESND2)
	DISCRETE_GAIN(CANYON_FINAL_MIX1, NODE_90, 77)
	DISCRETE_GAIN(CANYON_FINAL_MIX2, NODE_91, 77)

	DISCRETE_OUTPUT(CANYON_FINAL_MIX1, MIXER(100,MIXER_PAN_LEFT))
	DISCRETE_OUTPUT(CANYON_FINAL_MIX2, MIXER(100,MIXER_PAN_RIGHT))
DISCRETE_SOUND_END



/*************************************
 *
 *	Machine driver
 *
 *************************************/

static MACHINE_DRIVER_START( canyon )

	/* basic machine hardware */
	MDRV_CPU_ADD(M6502, 12096000 / 16)
	MDRV_CPU_MEMORY(readmem, writemem)
	MDRV_CPU_VBLANK_INT(nmi_line_pulse, 1)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(22 * 1000000 / 15750)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(256, 240)
	MDRV_VISIBLE_AREA(0, 255, 0, 239)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(3)
	MDRV_COLORTABLE_LENGTH(4)

	MDRV_PALETTE_INIT(canyon)
	MDRV_VIDEO_START(canyon)
	MDRV_VIDEO_UPDATE(canyon)

	/* sound hardware */
	MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
	MDRV_SOUND_ADD_TAG("discrete", DISCRETE, canyon_sound_interface)
MACHINE_DRIVER_END



/*************************************
 *
 *	ROM definitions
 *
 *************************************/

ROM_START( canyon )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD_NIB_LOW ( "9499-01.j1", 0x3000, 0x0400, CRC(31800767) SHA1(d4aebe12d3c45a2a8a361dc6f63e1a6230a78c17) )
	ROM_LOAD_NIB_HIGH( "9503-01.p1", 0x3000, 0x0400, CRC(1eddbe28) SHA1(7d30280bf9edff743c16386d7cdec78094477996) )
	ROM_LOAD         ( "9496-01.d1", 0x3800, 0x0800, CRC(8be15080) SHA1(095c15e9ac91623b2d514858dca2e4c261d36fd0) )
	ROM_RELOAD(                      0xF800, 0x0800 ) /* for 6502 vectors */

	ROM_REGION( 0x0400, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "9492-01.n8", 0x0000, 0x0400, CRC(7449f754) SHA1(a8ffc39e1a86c94487551f5026eedbbd066b12c9) )

	ROM_REGION( 0x0100, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD_NIB_LOW ( "9506-01.m5", 0x0000, 0x0100, CRC(0d63396a) SHA1(147fae3b02a86310c8d022a7e7cfbf71ea511616) )
	ROM_LOAD_NIB_HIGH( "9505-01.n5", 0x0000, 0x0100, CRC(60507c07) SHA1(fcb76890cbaa37e02392bf8b97f7be9a6fe6a721) )

	ROM_REGION( 0x0100, REGION_PROMS, 0 )
	ROM_LOAD( "9491-01.j6", 0x0000, 0x0100, CRC(b8094b4c) SHA1(82dc6799a19984f3b204ee3aeeb007e55afc8be3) )	/* sync (not used) */
ROM_END


ROM_START( canyonp )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD_NIB_LOW ( "cbp3000l.j1", 0x3000, 0x0800, CRC(49cf29a0) SHA1(b58f024f45f85e5c2a48a95c60e80fd1be60eaac) )
	ROM_LOAD_NIB_HIGH( "cbp3000m.p1", 0x3000, 0x0800, CRC(b4385c23) SHA1(b550dfe9182f2b29aedba160a0917ca78b82f0e7) )
	ROM_LOAD_NIB_LOW ( "cbp3800l.h1", 0x3800, 0x0800, CRC(c7ee4431) SHA1(7a0f4454a981c4e9ee27e273e9a8379458e660e5) )
	ROM_RELOAD(                       0xf800, 0x0800 ) /* for 6502 vectors */
	ROM_LOAD_NIB_HIGH( "cbp3800m.r1", 0x3800, 0x0800, CRC(94246a9a) SHA1(5ff8b69fb744a5f62d4cf291e8f25e3620b479e7) )
	ROM_RELOAD(                       0xf800, 0x0800 ) /* for 6502 vectors */

	ROM_REGION( 0x0400, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "9492-01.n8", 0x0000, 0x0400, CRC(7449f754) SHA1(a8ffc39e1a86c94487551f5026eedbbd066b12c9) )

	ROM_REGION( 0x0100, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD_NIB_LOW ( "9506-01.m5", 0x0000, 0x0100, CRC(0d63396a) SHA1(147fae3b02a86310c8d022a7e7cfbf71ea511616) )
	ROM_LOAD_NIB_HIGH( "9505-01.n5", 0x0000, 0x0100, CRC(60507c07) SHA1(fcb76890cbaa37e02392bf8b97f7be9a6fe6a721) )

	ROM_REGION( 0x0100, REGION_PROMS, 0 )
	ROM_LOAD( "9491-01.j6", 0x0000, 0x0100, CRC(b8094b4c) SHA1(82dc6799a19984f3b204ee3aeeb007e55afc8be3) )	/* sync (not used) */
ROM_END



/*************************************
 *
 *	Game drivers
 *
 *************************************/

GAME( 1977, canyon,  0,      canyon, canyon, 0, ROT0, "Atari", "Canyon Bomber" )
GAME( 1977, canyonp, canyon, canyon, canyon, 0, ROT0, "Atari", "Canyon Bomber (prototype)" )
