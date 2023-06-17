/***************************************************************************

Rock'n'Rage(GX620) (c) 1986 Konami

Driver by Manuel Abadia <manu@teleline.es>

***************************************************************************/

#include "driver.h"
#include "cpu/m6809/m6809.h"
#include "cpu/hd6309/hd6309.h"
#include "vidhrdw/generic.h"
#include "vidhrdw/konamiic.h"

extern int rockrage_irq_enable;

/* from vidhrdw */
VIDEO_START( rockrage );
VIDEO_UPDATE( rockrage );
WRITE_HANDLER( rockrage_vreg_w );
PALETTE_INIT( rockrage );

static INTERRUPT_GEN( rockrage_interrupt )
{
	if (K007342_is_INT_enabled())
        cpu_set_irq_line(0, HD6309_IRQ_LINE, HOLD_LINE);
}

static WRITE_HANDLER( rockrage_bankswitch_w )
{
	int bankaddress;
	unsigned char *RAM = memory_region(REGION_CPU1);

	/* bits 4-6 = bank number */
	bankaddress = 0x10000 + ((data & 0x70) >> 4) * 0x2000;
	cpu_setbank(1,&RAM[bankaddress]);

	/* bits 0 & 1 = coin counters */
	coin_counter_w(0,data & 0x01);
	coin_counter_w(1,data & 0x02);

	/* other bits unknown */
}

static WRITE_HANDLER( rockrage_sh_irqtrigger_w )
{
	soundlatch_w(offset, data);
	cpu_set_irq_line(1,M6809_IRQ_LINE,HOLD_LINE);
}

static READ_HANDLER( rockrage_VLM5030_busy_r ) {
	return ( VLM5030_BSY() ? 1 : 0 );
}

static WRITE_HANDLER( rockrage_speech_w ) {
	/* bit2 = data bus enable */
	VLM5030_RST( ( data >> 1 ) & 0x01 );
	VLM5030_ST(  ( data >> 0 ) & 0x01 );
}

static MEMORY_READ_START( rockrage_readmem )
	{ 0x0000, 0x1fff, K007342_r },			/* Color RAM + Video RAM */
	{ 0x2000, 0x21ff, K007420_r },			/* Sprite RAM */
	{ 0x2200, 0x23ff, K007342_scroll_r },	/* Scroll RAM */
	{ 0x2400, 0x247f, paletteram_r },		/* Palette */
	{ 0x2e01, 0x2e01, input_port_3_r },		/* 1P controls */
	{ 0x2e02, 0x2e02, input_port_4_r },		/* 2P controls */
	{ 0x2e03, 0x2e03, input_port_1_r },		/* DISPW #2 */
	{ 0x2e40, 0x2e40, input_port_0_r },		/* DIPSW #1 */
	{ 0x2e00, 0x2e00, input_port_2_r },		/* coinsw, testsw, startsw */
	{ 0x4000, 0x5fff, MRA_RAM },			/* RAM */
	{ 0x6000, 0x7fff, MRA_BANK1 },			/* banked ROM */
	{ 0x8000, 0xffff, MRA_ROM },			/* ROM */
MEMORY_END

static MEMORY_WRITE_START( rockrage_writemem )
	{ 0x0000, 0x1fff, K007342_w },				/* Color RAM + Video RAM */
	{ 0x2000, 0x21ff, K007420_w },				/* Sprite RAM */
	{ 0x2200, 0x23ff, K007342_scroll_w },		/* Scroll RAM */
	{ 0x2400, 0x247f, paletteram_xBBBBBGGGGGRRRRR_w, &paletteram },/* palette */
	{ 0x2600, 0x2607, K007342_vreg_w },			/* Video Registers */
	{ 0x2e80, 0x2e80, rockrage_sh_irqtrigger_w },/* cause interrupt on audio CPU */
	{ 0x2ec0, 0x2ec0, watchdog_reset_w },		/* watchdog reset */
	{ 0x2f00, 0x2f00, rockrage_vreg_w },		/* ??? */
	{ 0x2f40, 0x2f40, rockrage_bankswitch_w },	/* bankswitch control */
	{ 0x4000, 0x5fff, MWA_RAM },				/* RAM */
	{ 0x6000, 0x7fff, MWA_RAM },				/* banked ROM */
	{ 0x8000, 0xffff, MWA_ROM },				/* ROM */
MEMORY_END

static MEMORY_READ_START( rockrage_readmem_sound )
	{ 0x3000, 0x3000, rockrage_VLM5030_busy_r },/* VLM5030 */
	{ 0x5000, 0x5000, soundlatch_r },			/* soundlatch_r */
	{ 0x6001, 0x6001, YM2151_status_port_0_r },	/* YM 2151 */
	{ 0x7000, 0x77ff, MRA_RAM },				/* RAM */
	{ 0x8000, 0xffff, MRA_ROM },				/* ROM */
MEMORY_END

static MEMORY_WRITE_START( rockrage_writemem_sound )
	{ 0x2000, 0x2000, VLM5030_data_w }, 			/* VLM5030 */
	{ 0x4000, 0x4000, rockrage_speech_w },			/* VLM5030 */
	{ 0x6000, 0x6000, YM2151_register_port_0_w },	/* YM 2151 */
	{ 0x6001, 0x6001, YM2151_data_port_0_w },		/* YM 2151 */
	{ 0x7000, 0x77ff, MWA_RAM },					/* RAM */
	{ 0x8000, 0xffff, MWA_ROM },					/* ROM */
MEMORY_END

/***************************************************************************

	Input Ports

***************************************************************************/

INPUT_PORTS_START( rockrage )
	PORT_START	/* DSW #1 */
	PORT_DIPNAME( 0x0f, 0x0f, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x05, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 3C_2C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 4C_3C ) )
	PORT_DIPSETTING(    0x0f, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 3C_4C ) )
	PORT_DIPSETTING(    0x07, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x0e, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 2C_5C ) )
	PORT_DIPSETTING(    0x0d, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x0b, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0x0a, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(    0x09, DEF_STR( 1C_7C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0xf0, 0xf0, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x50, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 3C_2C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 4C_3C ) )
	PORT_DIPSETTING(    0xf0, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 3C_4C ) )
	PORT_DIPSETTING(    0x70, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0xe0, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x60, DEF_STR( 2C_5C ) )
	PORT_DIPSETTING(    0xd0, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0xb0, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0xa0, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(    0x90, DEF_STR( 1C_7C ) )
/*	PORT_DIPSETTING(    0x00, "Invalid" )*/

	PORT_START	/* DSW #2 */
	PORT_DIPNAME( 0x03, 0x01, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x03, "1" )
	PORT_DIPSETTING(    0x02, "2" )
	PORT_DIPSETTING(    0x01, "3" )
	PORT_DIPSETTING(    0x00, "5" )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x08, "30k and every 70k" )
	PORT_DIPSETTING(    0x00, "40k and every 80k" )
	PORT_DIPNAME( 0x10, 0x10, "Freeze Screen" )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START	/* COINSW */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_COIN3 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On ) )
	PORT_SERVICE( 0x80, IP_ACTIVE_LOW )

	PORT_START	/* PLAYER 1 INPUTS */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START	/* PLAYER 2 INPUTS */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

INPUT_PORTS_END

static struct GfxLayout charlayout =
{
	8,8,			/* 8*8 characters */
	0x20000/16,		/* 8192 characters */
	4,				/* 4 bpp */
	{ 0, 1, 2, 3 },	/* the four bitplanes are packed in one nibble */
	{ 0*4, 1*4, 0x20000*8+0*4, 0x20000*8+1*4, 2*4, 3*4, 0x20000*8+2*4, 0x20000*8+3*4 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16 },
	16*8		/* every char takes 16 consecutive bytes */
};

static struct GfxLayout spritelayout =
{
	8,8,			/* 8*8 sprites */
	0x40000/32,	/* 8192 sprites */
	4,				/* 4 bpp */
	{ 0, 1, 2, 3 },	/* the four bitplanes are packed in one nibble */
	{ 0*4, 1*4, 2*4, 3*4, 4*4, 5*4, 6*4, 7*4 },
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32 },
	32*8			/* every sprite takes 32 consecutive bytes */
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &charlayout,   64, 32 },	/* colors 00..31, but using 2 lookup tables */
	{ REGION_GFX2, 0, &spritelayout, 32,  1 },	/* colors 32..63 */
	{ -1 } /* end of array */
};

/***************************************************************************

	Machine Driver

***************************************************************************/

static struct YM2151interface ym2151_interface =
{
	1,			/* 1 chip */
	3579545,	/* 3.579545 MHz */
	{ YM3012_VOL(60,MIXER_PAN_LEFT,60,MIXER_PAN_RIGHT) },
	{ 0 },
	{ 0 }
};

static struct VLM5030interface vlm5030_interface =
{
	3579545,	/* 3.579545 MHz */
	60,			/* volume */
	REGION_SOUND1,	/* memory region of speech rom */
	0
};

static MACHINE_DRIVER_START( rockrage )

	/* basic machine hardware */
	MDRV_CPU_ADD(HD6309, 3000000)		/* 24MHz/8 (?) */
	MDRV_CPU_MEMORY(rockrage_readmem,rockrage_writemem)
	MDRV_CPU_VBLANK_INT(rockrage_interrupt,1)

	MDRV_CPU_ADD(M6809, 2000000)
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)		/* 24MHz/12 (?) */
	MDRV_CPU_MEMORY(rockrage_readmem_sound,rockrage_writemem_sound)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 2*8, 30*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(64)
	MDRV_COLORTABLE_LENGTH(64 + 2*16*16)

	MDRV_PALETTE_INIT(rockrage)
	MDRV_VIDEO_START(rockrage)
	MDRV_VIDEO_UPDATE(rockrage)

	/* sound hardware */
	MDRV_SOUND_ADD(YM2151, ym2151_interface)
	MDRV_SOUND_ADD(VLM5030, vlm5030_interface)
MACHINE_DRIVER_END


/***************************************************************************

  Game ROMs

***************************************************************************/

ROM_START( rockrage )
	ROM_REGION( 0x20000, REGION_CPU1, 0 ) /* code + banked roms */
	ROM_LOAD( "rr-q01.rom", 0x08000, 0x08000, CRC(0ddb5ef5) SHA1(71b38c9f957858371f0ac95720d3c6d07339e5c5) )	/* fixed ROM */
	ROM_LOAD( "rr-q02.rom", 0x10000, 0x10000, CRC(b4f6e346) SHA1(43fded4484836ff315dd6e40991f909dad73f1ed) )	/* banked ROM */

	ROM_REGION(  0x10000 , REGION_CPU2, 0 ) /* 64k for the sound CPU */
	ROM_LOAD( "620k03.11c", 0x08000, 0x08000, CRC(9fbefe82) SHA1(ab42b7e519a0dd08f2249dad0819edea0976f39a) )

	ROM_REGION( 0x040000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "620k06.15g",	0x000000, 0x20000, BAD_DUMP CRC(c0e2b35c) SHA1(fb37a151188f27f883fed5fdfb0094c3efa9470d)  )	/* tiles */
	ROM_LOAD( "620k05.16g",	0x020000, 0x20000, BAD_DUMP CRC(ca9d9346) SHA1(fee8d98def802f312c6cd0ec751c67aa18acfacd)  )

	ROM_REGION( 0x040000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "rr-k11.rom",	0x000000, 0x20000, CRC(70449239) SHA1(07653ea3bfe0063c9d2b2102ac52a1b50fc2971e) )	/* sprites */
	ROM_LOAD( "rr-l10.rom",	0x020000, 0x20000, CRC(06d108e0) SHA1(cae8c5f2fc4e84bc7adbf27f71a18a74968c4296) )

	ROM_REGION( 0x0300, REGION_PROMS, 0 )
	ROM_LOAD( "620k09.11g", 0x00000, 0x00100, CRC(9f0e0608) SHA1(c95bdb370e4a91f27afbd5ff3b39b2e0ad87da73) )	/* layer 0 lookup table */
	ROM_LOAD( "620k08.12g", 0x00100, 0x00100, CRC(b499800c) SHA1(46fa4e071ebceed12027de109be1e16dde5e846e) )	/* layer 1 lookup table */
	ROM_LOAD( "620k07.13g", 0x00200, 0x00100, CRC(b6135ee0) SHA1(248a978987cff86c2bbad10ef332f63a6abd5bee) )	/* sprite lookup table, but its not used */
															/* because it's always 0 1 2 ... f */
	ROM_REGION( 0x08000, REGION_SOUND1, 0 ) /* VLM3050 data */
	ROM_LOAD( "620k04.6e", 0x00000, 0x08000, CRC(8be969f3) SHA1(9856b4c13fac77b645aed67a08cb4965b4966492) )
ROM_END

ROM_START( rockragj )
	ROM_REGION( 0x20000, REGION_CPU1, 0 ) /* code + banked roms */
	ROM_LOAD( "620k01.16c", 0x08000, 0x08000, CRC(4f5171f7) SHA1(5bce9e3f9d01c113c697853763cd891b91297eb2) )	/* fixed ROM */
	ROM_LOAD( "620k02.15c", 0x10000, 0x10000, CRC(04c4d8f7) SHA1(2a1a024fc38bb934c454092b0aed74d0f1d1c4af) )	/* banked ROM */

	ROM_REGION(  0x10000 , REGION_CPU2, 0 ) /* 64k for the sound CPU */
	ROM_LOAD( "620k03.11c", 0x08000, 0x08000, CRC(9fbefe82) SHA1(ab42b7e519a0dd08f2249dad0819edea0976f39a) )

	ROM_REGION( 0x040000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "620k06.15g",	0x000000, 0x20000, CRC(c0e2b35c) SHA1(fb37a151188f27f883fed5fdfb0094c3efa9470d) )	/* tiles */
	ROM_LOAD( "620k05.16g",	0x020000, 0x20000, CRC(ca9d9346) SHA1(fee8d98def802f312c6cd0ec751c67aa18acfacd) )

	ROM_REGION( 0x040000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "620k11.7g",	0x000000, 0x20000, CRC(7430f6e9) SHA1(5d488c7b7b0eb4e502b3e566ac102cd3267e8568) )	/* sprites */
	ROM_LOAD( "620k10.8g",	0x020000, 0x20000, CRC(0d1a95ab) SHA1(be565424f17af31dcd07004c6be03bbb00aef514) )

	ROM_REGION( 0x0300, REGION_PROMS, 0 )
	ROM_LOAD( "620k09.11g", 0x00000, 0x00100, CRC(9f0e0608) SHA1(c95bdb370e4a91f27afbd5ff3b39b2e0ad87da73) )	/* layer 0 lookup table */
	ROM_LOAD( "620k08.12g", 0x00100, 0x00100, CRC(b499800c) SHA1(46fa4e071ebceed12027de109be1e16dde5e846e) )	/* layer 1 lookup table */
	ROM_LOAD( "620k07.13g", 0x00200, 0x00100, CRC(b6135ee0) SHA1(248a978987cff86c2bbad10ef332f63a6abd5bee) )	/* sprite lookup table, but its not used */
															/* because it's always 0 1 2 ... f */
	ROM_REGION( 0x08000, REGION_SOUND1, 0 ) /* VLM3050 data */
	ROM_LOAD( "620k04.6e", 0x00000, 0x08000, CRC(8be969f3) SHA1(9856b4c13fac77b645aed67a08cb4965b4966492) )
ROM_END

/***************************************************************************

  Game driver(s)

***************************************************************************/

GAME( 1986, rockrage, 0,        rockrage, rockrage, 0, ROT0, "Konami", "Rock 'n Rage (World[Q])" )
GAME( 1986, rockragj, rockrage, rockrage, rockrage, 0, ROT0, "Konami", "Koi no Hotrock (Japan)" )
