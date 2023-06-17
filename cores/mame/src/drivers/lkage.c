/***************************************************************************

Legend of Kage
(C)1985 Taito
CPU: Z80 (x2), MC68705
Sound: YM2203 (x2)

Phil Stroffolino
ptroffo@yahoo.com

TODO:

- The high score display uses a video attribute flag whose pupose isn't known.

- purpose of the 0x200 byte prom, "a54-10.2" is unknown.  It contains values in range 0x0..0xf.

- SOUND: lots of unknown writes to the YM2203 I/O ports

- Note that all the bootlegs are derived from a different version of the
  original which hasn't been found yet.

- lkage is verfied to be an original set, but it seems to work regardless of what
  the mcu does. Moreover, the mcu returns a checksum which is different from what
  is expected - the MCU computes 0x89, but the main CPU expects 0x5d.
  The game works anyway, it never gives the usual Taito "BAD HW" message.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "cpu/z80/z80.h"
#include "cpu/m6805/m6805.h"


extern unsigned char *lkage_scroll, *lkage_vreg;
WRITE_HANDLER( lkage_videoram_w );
VIDEO_START( lkage );
VIDEO_UPDATE( lkage );

READ_HANDLER( lkage_68705_portA_r );
WRITE_HANDLER( lkage_68705_portA_w );
READ_HANDLER( lkage_68705_portB_r );
WRITE_HANDLER( lkage_68705_portB_w );
READ_HANDLER( lkage_68705_portC_r );
WRITE_HANDLER( lkage_68705_portC_w );
WRITE_HANDLER( lkage_68705_ddrA_w );
WRITE_HANDLER( lkage_68705_ddrB_w );
WRITE_HANDLER( lkage_68705_ddrC_w );
WRITE_HANDLER( lkage_mcu_w );
READ_HANDLER( lkage_mcu_r );
READ_HANDLER( lkage_mcu_status_r );


static int sound_nmi_enable,pending_nmi;

static void nmi_callback(int param)
{
	if (sound_nmi_enable) cpu_set_irq_line(1,IRQ_LINE_NMI,PULSE_LINE);
	else pending_nmi = 1;
}

static WRITE_HANDLER( lkage_sound_command_w )
{
	soundlatch_w(offset,data);
	timer_set(TIME_NOW,data,nmi_callback);
}

static WRITE_HANDLER( lkage_sh_nmi_disable_w )
{
	sound_nmi_enable = 0;
}

static WRITE_HANDLER( lkage_sh_nmi_enable_w )
{
	sound_nmi_enable = 1;
	if (pending_nmi)
	{ /* probably wrong but commands may go lost otherwise */
		cpu_set_irq_line(1,IRQ_LINE_NMI,PULSE_LINE);
		pending_nmi = 0;
	}
}

static MEMORY_READ_START( readmem )
    { 0x0000, 0xdfff, MRA_ROM },
	{ 0xe000, 0xe7ff, MRA_RAM },      /* work ram */
	{ 0xe800, 0xefff, paletteram_r },
	{ 0xf000, 0xf003, MRA_RAM },       /* video registers */
	{ 0xf062, 0xf062, lkage_mcu_r },
	{ 0xf080, 0xf080, input_port_0_r }, /* DSW1 */
	{ 0xf081, 0xf081, input_port_1_r }, /* DSW2 (coinage) */
	{ 0xf082, 0xf082, input_port_2_r }, /* DSW3 */
	{ 0xf083, 0xf083, input_port_3_r }, /* start, insert coin, tilt */
	{ 0xf084, 0xf084, input_port_4_r }, /* P1 controls */
	{ 0xf086, 0xf086, input_port_5_r }, /* P2 controls */
	{ 0xf087, 0xf087, lkage_mcu_status_r },
	{ 0xf0a0, 0xf0a3, MRA_RAM }, /* unknown */
	{ 0xf0c0, 0xf0c5, MRA_RAM },
	{ 0xf100, 0xf15f, MRA_RAM },
	{ 0xf400, 0xffff, MRA_RAM },  /* videoram */
MEMORY_END

static MEMORY_WRITE_START( writemem)
    { 0x0000, 0xdfff, MWA_ROM },
	{ 0xe000, 0xe7ff, MWA_RAM }, /* work ram */
	{ 0xe800, 0xefff, paletteram_xxxxRRRRGGGGBBBB_w, &paletteram },
	{ 0xf000, 0xf003, MWA_RAM, &lkage_vreg }, /* video registers */
	{ 0xf060, 0xf060, lkage_sound_command_w },
	{ 0xf061, 0xf061, MWA_NOP },
	{ 0xf062, 0xf062, lkage_mcu_w },
	{ 0xf063, 0xf063, MWA_NOP }, /* pulsed; nmi on sound cpu? */
	{ 0xf0a0, 0xf0a3, MWA_RAM }, /* unknown */
	{ 0xf0c0, 0xf0c5, MWA_RAM, &lkage_scroll },
	{ 0xf0e1, 0xf0e1, MWA_NOP }, /* pulsed */
	{ 0xf100, 0xf15f, MWA_RAM, &spriteram },
	{ 0xf400, 0xffff, lkage_videoram_w, &videoram }, /* videoram */
MEMORY_END

static READ_HANDLER( port_fetch_r )
{
	return memory_region(REGION_USER1)[offset];
}

static PORT_READ_START( readport )
	{ 0x4000, 0x7fff, port_fetch_r },
PORT_END


static MEMORY_READ_START( m68705_readmem )
	{ 0x0000, 0x0000, lkage_68705_portA_r },
	{ 0x0001, 0x0001, lkage_68705_portB_r },
	{ 0x0002, 0x0002, lkage_68705_portC_r },
	{ 0x0010, 0x007f, MRA_RAM },
	{ 0x0080, 0x07ff, MRA_ROM },
MEMORY_END

static MEMORY_WRITE_START( m68705_writemem )
	{ 0x0000, 0x0000, lkage_68705_portA_w },
	{ 0x0001, 0x0001, lkage_68705_portB_w },
	{ 0x0002, 0x0002, lkage_68705_portC_w },
	{ 0x0004, 0x0004, lkage_68705_ddrA_w },
	{ 0x0005, 0x0005, lkage_68705_ddrB_w },
	{ 0x0006, 0x0006, lkage_68705_ddrC_w },
	{ 0x0010, 0x007f, MWA_RAM },
	{ 0x0080, 0x07ff, MWA_ROM },
MEMORY_END



/***************************************************************************/

/* sound section is almost identical to Bubble Bobble, YM2203 instead of YM3526 */

static MEMORY_READ_START( readmem_sound )
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0x87ff, MRA_RAM },
	{ 0x9000, 0x9000, YM2203_status_port_0_r },
	{ 0xa000, 0xa000, YM2203_status_port_1_r },
	{ 0xb000, 0xb000, soundlatch_r },
	{ 0xb001, 0xb001, MRA_NOP },	/* ??? */
	{ 0xe000, 0xefff, MRA_ROM },	/* space for diagnostic ROM? */
MEMORY_END

static MEMORY_WRITE_START( writemem_sound )
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0x8000, 0x87ff, MWA_RAM },
	{ 0x9000, 0x9000, YM2203_control_port_0_w },
	{ 0x9001, 0x9001, YM2203_write_port_0_w },
	{ 0xa000, 0xa000, YM2203_control_port_1_w },
	{ 0xa001, 0xa001, YM2203_write_port_1_w },
	{ 0xb000, 0xb000, MWA_NOP },	/* ??? */
	{ 0xb001, 0xb001, lkage_sh_nmi_enable_w },
	{ 0xb002, 0xb002, lkage_sh_nmi_disable_w },
	{ 0xe000, 0xefff, MWA_ROM },	/* space for diagnostic ROM? */
MEMORY_END

/***************************************************************************/

INPUT_PORTS_START( lkage )
	PORT_START      /* DSW1 */
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x03, "30000 100000" ) /* unverified */
	PORT_DIPSETTING(    0x02, "30000 70000" ) /* unverified */
	PORT_DIPSETTING(    0x01, "20000 70000" ) /* unverified */
	PORT_DIPSETTING(    0x00, "20000 50000" ) /* unverified */
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Free_Play ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x18, 0x18, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x18, "3" )
	PORT_DIPSETTING(    0x10, "4" )
	PORT_DIPSETTING(    0x08, "5" )
	PORT_BITX(0,  0x00, IPT_DIPSWITCH_SETTING | IPF_CHEAT, "255", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Cocktail ) )

	PORT_START      /* DSW2 */
	PORT_DIPNAME( 0x0f, 0x00, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x0f, DEF_STR( 9C_1C ) )
	PORT_DIPSETTING(    0x0e, DEF_STR( 8C_1C ) )
	PORT_DIPSETTING(    0x0d, DEF_STR( 7C_1C ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( 6C_1C ) )
	PORT_DIPSETTING(    0x0b, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(    0x0a, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x09, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0x05, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 1C_7C ) )
	PORT_DIPSETTING(    0x07, DEF_STR( 1C_8C ) )
	PORT_DIPNAME( 0xf0, 0x00, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0xf0, DEF_STR( 9C_1C ) )
	PORT_DIPSETTING(    0xe0, DEF_STR( 8C_1C ) )
	PORT_DIPSETTING(    0xd0, DEF_STR( 7C_1C ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 6C_1C ) )
	PORT_DIPSETTING(    0xb0, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(    0xa0, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x90, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0x50, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(    0x60, DEF_STR( 1C_7C ) )
	PORT_DIPSETTING(    0x70, DEF_STR( 1C_8C ) )

	PORT_START
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Demo_Sounds) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, "Initial Season" )
	PORT_DIPSETTING(    0x02, "Spring" )
	PORT_DIPSETTING(    0x00, "Winter" )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x0c, "Easiest" )
	PORT_DIPSETTING(    0x08, "Easy" )
	PORT_DIPSETTING(    0x04, "Normal" )
	PORT_DIPSETTING(    0x00, "Hard" )
	PORT_DIPNAME( 0x10, 0x10, "Coinage Display" )
	PORT_DIPSETTING(    0x10, "Coins/Credits" )
	PORT_DIPSETTING(    0x00, "Insert Coin" )
	PORT_DIPNAME( 0x20, 0x20, "Year Display" )
	PORT_DIPSETTING(    0x00, "1985" )
	PORT_DIPSETTING(    0x20, "MCMLXXXIV" )
	PORT_BITX(    0x40, 0x40, IPT_DIPSWITCH_NAME | IPF_CHEAT, "Invulnerability", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, "Coin Slots" )
	PORT_DIPSETTING(    0x80, "A and B" )
	PORT_DIPSETTING(    0x00, "A only" )

	PORT_START      /* Service */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_COIN3 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_TILT )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END



static struct GfxLayout tile_layout =
{
	8,8,
	RGN_FRAC(1,4),
	4,
	{ RGN_FRAC(1,4),RGN_FRAC(0,4),RGN_FRAC(3,4),RGN_FRAC(2,4) },
	{ 7, 6, 5, 4, 3, 2, 1, 0 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8
};

static struct GfxLayout sprite_layout =
{
	16,16,
	RGN_FRAC(1,4),
	4,
	{ RGN_FRAC(1,4),RGN_FRAC(0,4),RGN_FRAC(3,4),RGN_FRAC(2,4) },
	{ 7, 6, 5, 4, 3, 2, 1, 0,
			64+7, 64+6, 64+5, 64+4, 64+3, 64+2, 64+1, 64+0 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
			128+0*8, 128+1*8, 128+2*8, 128+3*8, 128+4*8, 128+5*8, 128+6*8, 128+7*8 },
	32*8
};


static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0x0000, &tile_layout,  /*128*/0, 64 },
	{ REGION_GFX1, 0x0000, &sprite_layout,  0, 16 },
	{ -1 }
};



static void irqhandler(int irq)
{
	cpu_set_irq_line(1,0,irq ? ASSERT_LINE : CLEAR_LINE);
}

static struct YM2203interface ym2203_interface =
{
	2,          /* 2 chips */
	4000000,    /* 4 MHz ? (hand tuned) */
	{ YM2203_VOL(40,15), YM2203_VOL(40,15) },
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 },
	{ irqhandler }
};



static MACHINE_DRIVER_START( lkage )

	/* basic machine hardware */
	MDRV_CPU_ADD(Z80,6000000)
	MDRV_CPU_FLAGS(CPU_16BIT_PORT)	/* ??? */
	MDRV_CPU_MEMORY(readmem,writemem)
	MDRV_CPU_PORTS(readport,0)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,1)

	MDRV_CPU_ADD(Z80, 6000000)
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)	/* ??? */
	MDRV_CPU_MEMORY(readmem_sound,writemem_sound)
								/* IRQs are triggered by the YM2203 */
	MDRV_CPU_ADD(M68705,4000000/2)	/* ??? */
	MDRV_CPU_MEMORY(m68705_readmem,m68705_writemem)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(2*8, 32*8-1, 2*8, 30*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(1024)

	MDRV_VIDEO_START(lkage)
	MDRV_VIDEO_UPDATE(lkage)

	/* sound hardware */
	MDRV_SOUND_ADD(YM2203, ym2203_interface)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( lkageb )

	/* basic machine hardware */
	MDRV_CPU_ADD(Z80,6000000)
	MDRV_CPU_FLAGS(CPU_16BIT_PORT)	/* ??? */
	MDRV_CPU_MEMORY(readmem,writemem)
	MDRV_CPU_PORTS(readport,0)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,1)

	MDRV_CPU_ADD(Z80, 6000000)
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)	/* ??? */
	MDRV_CPU_MEMORY(readmem_sound,writemem_sound)
								/* IRQs are triggered by the YM2203 */
	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(2*8, 31*8-1, 2*8, 30*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(1024)

	MDRV_VIDEO_START(lkage)
	MDRV_VIDEO_UPDATE(lkage)

	/* sound hardware */
	MDRV_SOUND_ADD(YM2203, ym2203_interface)
MACHINE_DRIVER_END



ROM_START( lkage )
	ROM_REGION( 0x14000, REGION_CPU1, 0 ) /* Z80 code (main CPU) */
	ROM_LOAD( "a54-01-1.37", 0x0000, 0x8000, CRC(973da9c5) SHA1(ad3b5d6a329b784e47be563c6f8dc628f32ba0a5) )
	ROM_LOAD( "a54-02-1.38", 0x8000, 0x8000, CRC(27b509da) SHA1(c623950bd7dd2b5699ca948e3731455964106b89) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* Z80 code (sound CPU) */
	ROM_LOAD( "a54-04.54",   0x0000, 0x8000, CRC(541faf9a) SHA1(b142ff3bd198f700697ec06ea92db3109ab5818e) )

	ROM_REGION( 0x10000, REGION_CPU3, 0 ) /* 68705 MCU code */
	ROM_LOAD( "a54-09.53",   0x0000, 0x0800, CRC(0e8b8846) SHA1(a4a105462b0127229bb7edfadd2e581c7e40f1cc) )

	ROM_REGION( 0x4000, REGION_USER1, 0 ) /* data */
	ROM_LOAD( "a54-03.51",   0x0000, 0x4000, CRC(493e76d8) SHA1(13c6160edd94ba2801fd89bb33bcae3a1e3454ff) )

	ROM_REGION( 0x10000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "a54-05-1.84", 0x0000, 0x4000, CRC(0033c06a) SHA1(89964503fc338817c6511fd15942741996b7037a) )
	ROM_LOAD( "a54-06-1.85", 0x4000, 0x4000, CRC(9f04d9ad) SHA1(3b9a4d30348fd02e5c8ae94655548bd4a02dd65d) )
	ROM_LOAD( "a54-07-1.86", 0x8000, 0x4000, CRC(b20561a4) SHA1(0d6d83dfae79ea133e37704ca47426b4c978fb36) )
	ROM_LOAD( "a54-08-1.87", 0xc000, 0x4000, CRC(3ff3b230) SHA1(ffcd964efb0af32b5d7a70305dfda615ea95acbe) )

	ROM_REGION( 0x0200, REGION_PROMS, 0 )
	ROM_LOAD( "a54-10.2",    0x0000, 0x0200, CRC(17dfbd14) SHA1(f8f0b6dfedd4ba108dad43ccc7697ef4ab9cbf86) )	/* unknown */
ROM_END

ROM_START( lkageb )
	ROM_REGION( 0x10000, REGION_CPU1, 0 ) /* Z80 code (main CPU) */
	ROM_LOAD( "ic37_1",      0x0000, 0x8000, CRC(05694f7b) SHA1(08a3796d6cf04d64db52ed8208a51084c420e10a) )
	ROM_LOAD( "ic38_2",      0x8000, 0x8000, CRC(22efe29e) SHA1(f7a29d54081ca7509e822ad8823ec977bccc4a40) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* Z80 code (sound CPU) */
	ROM_LOAD( "a54-04.54",   0x0000, 0x8000, CRC(541faf9a) SHA1(b142ff3bd198f700697ec06ea92db3109ab5818e) )

	ROM_REGION( 0x10000, REGION_CPU3, 0 ) /* 68705 MCU code */
	ROM_LOAD( "mcu",   0x0000, 0x0800, NO_DUMP )

	ROM_REGION( 0x4000, REGION_USER1, 0 ) /* data */
	ROM_LOAD( "a54-03.51",   0x0000, 0x4000, CRC(493e76d8) SHA1(13c6160edd94ba2801fd89bb33bcae3a1e3454ff) )

	ROM_REGION( 0x10000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "ic93_5",      0x0000, 0x4000, CRC(76753e52) SHA1(13f61969d59b055a5ab40237148e091d7cabe190) )
	ROM_LOAD( "ic94_6",      0x4000, 0x4000, CRC(f33c015c) SHA1(756326daab255d3a36d97e51ee141b9f7157f12e) )
	ROM_LOAD( "ic95_7",      0x8000, 0x4000, CRC(0e02c2e8) SHA1(1d8a817ba66cf26a4fe51ae00874c0fe6e7cebe3) )
	ROM_LOAD( "ic96_8",      0xc000, 0x4000, CRC(4ef5f073) SHA1(dfd234542b28cff74692a1c381772da01e8bb4a7) )

	ROM_REGION( 0x0200, REGION_PROMS, 0 )
	ROM_LOAD( "a54-10.2",    0x0000, 0x0200, CRC(17dfbd14) SHA1(f8f0b6dfedd4ba108dad43ccc7697ef4ab9cbf86) )	/* unknown */
ROM_END

ROM_START( lkageb2 )
	ROM_REGION( 0x10000, REGION_CPU1, 0 ) /* Z80 code (main CPU) */
	ROM_LOAD( "lok.a",       0x0000, 0x8000, CRC(866df793) SHA1(44a9a773d7bbfc5f9d53f56682438ef8b23ecbd6) )
	ROM_LOAD( "lok.b",       0x8000, 0x8000, CRC(fba9400f) SHA1(fedcb9b717feaeec31afda098f0ac2744df6c7be) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* Z80 code (sound CPU) */
	ROM_LOAD( "a54-04.54",   0x0000, 0x8000, CRC(541faf9a) SHA1(b142ff3bd198f700697ec06ea92db3109ab5818e) )

	ROM_REGION( 0x4000, REGION_USER1, 0 ) /* data */
	ROM_LOAD( "a54-03.51",   0x0000, 0x4000, CRC(493e76d8) SHA1(13c6160edd94ba2801fd89bb33bcae3a1e3454ff) )

	ROM_REGION( 0x10000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "ic93_5",      0x0000, 0x4000, CRC(76753e52) SHA1(13f61969d59b055a5ab40237148e091d7cabe190) )
	ROM_LOAD( "ic94_6",      0x4000, 0x4000, CRC(f33c015c) SHA1(756326daab255d3a36d97e51ee141b9f7157f12e) )
	ROM_LOAD( "ic95_7",      0x8000, 0x4000, CRC(0e02c2e8) SHA1(1d8a817ba66cf26a4fe51ae00874c0fe6e7cebe3) )
	ROM_LOAD( "ic96_8",      0xc000, 0x4000, CRC(4ef5f073) SHA1(dfd234542b28cff74692a1c381772da01e8bb4a7) )

	ROM_REGION( 0x0200, REGION_PROMS, 0 )
	ROM_LOAD( "a54-10.2",    0x0000, 0x0200, CRC(17dfbd14) SHA1(f8f0b6dfedd4ba108dad43ccc7697ef4ab9cbf86) )	/* unknown */
ROM_END

ROM_START( lkageb3 )
	ROM_REGION( 0x10000, REGION_CPU1, 0 ) /* Z80 code (main CPU) */
	ROM_LOAD( "z1.bin",      0x0000, 0x8000, CRC(60cac488) SHA1(b61df14159f37143b1faed22d77fc7be31602022) )
	ROM_LOAD( "z2.bin",      0x8000, 0x8000, CRC(22c95f17) SHA1(8ca438d508a36918778651adf599cf45a7c4a5d7) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* Z80 code (sound CPU) */
	ROM_LOAD( "a54-04.54",   0x0000, 0x8000, CRC(541faf9a) SHA1(b142ff3bd198f700697ec06ea92db3109ab5818e) )

	ROM_REGION( 0x4000, REGION_USER1, 0 ) /* data */
	ROM_LOAD( "a54-03.51",   0x0000, 0x4000, CRC(493e76d8) SHA1(13c6160edd94ba2801fd89bb33bcae3a1e3454ff) )

	ROM_REGION( 0x10000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "ic93_5",      0x0000, 0x4000, CRC(76753e52) SHA1(13f61969d59b055a5ab40237148e091d7cabe190) )
	ROM_LOAD( "ic94_6",      0x4000, 0x4000, CRC(f33c015c) SHA1(756326daab255d3a36d97e51ee141b9f7157f12e) )
	ROM_LOAD( "ic95_7",      0x8000, 0x4000, CRC(0e02c2e8) SHA1(1d8a817ba66cf26a4fe51ae00874c0fe6e7cebe3) )
	ROM_LOAD( "ic96_8",      0xc000, 0x4000, CRC(4ef5f073) SHA1(dfd234542b28cff74692a1c381772da01e8bb4a7) )

	ROM_REGION( 0x0200, REGION_PROMS, 0 )
	ROM_LOAD( "a54-10.2",    0x0000, 0x0200, CRC(17dfbd14) SHA1(f8f0b6dfedd4ba108dad43ccc7697ef4ab9cbf86) )	/* unknown */
ROM_END

static unsigned char mcu_val;

/*Note: This probably uses another MCU dump,which is undumped.*/

static READ_HANDLER( fake_mcu_r )
{
	int result = 0;

	switch(mcu_val)
	{
		/*These are for the attract mode*/
		case 0x01:
			result = mcu_val-1;
			break;

		case 0x90:
			result = mcu_val+0x43;
			break;

		/*Gameplay Protection,checked in this order at a start of a play*/
		case 0xa6:
			result = mcu_val+0x27;
			break;

		case 0x34:
			result = mcu_val+0x7f;
			break;

		case 0x48:
			result = mcu_val+0xb7;
			break;

		default:
			result = mcu_val;
			break;
	}
	return result;
}

static WRITE_HANDLER( fake_mcu_w )
{
	mcu_val = data;
}

static READ_HANDLER( fake_status_r )
{
	static int res = 3; /* cpu data/mcu ready status */
	return res;
}

DRIVER_INIT( lkageb )
{
	install_mem_read_handler (0,0xf062,0xf062,fake_mcu_r);
	install_mem_read_handler (0,0xf087,0xf087,fake_status_r);
	install_mem_write_handler(0,0xf062,0xf062,fake_mcu_w );
}


GAME( 1984, lkage,   0,     lkage,  lkage, 0,       ROT0, "Taito Corporation", "The Legend of Kage" )
GAME( 1984, lkageb,  lkage, lkageb, lkage, lkageb,  ROT0, "bootleg", "The Legend of Kage (bootleg set 1)" )
GAME( 1984, lkageb2, lkage, lkageb, lkage, lkageb,  ROT0, "bootleg", "The Legend of Kage (bootleg set 2)" )
GAME( 1984, lkageb3, lkage, lkageb, lkage, lkageb,  ROT0, "bootleg", "The Legend of Kage (bootleg set 3)" )
