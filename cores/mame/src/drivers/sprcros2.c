/*
Super Cross II (JPN Ver.)
(c)1986 GM Shoji

C2-00172-D
CPU  :Z80B
Sound:SN76489 x3

SCS-24.4E
SCS-25.4C
SCS-26.4B
SCS-27.5K
SCS-28.5J
SCS-29.5H
SCS-30.5F

SC-62.3A
SC-63.3B
SC-64.6A

C2-00171-D
CPU  :Z80B
OSC  :10.000MHz

SCM-00.10L
SCM-01.10K
SCM-02.10J
SCM-03.10G
SCM-20.5K
SCM-21.5G
SCM-22.5E
SCM-23.5B

SC-60.4K
SC-61.5A

Notes:

- sprites pop in at the wrong place sometimes before entering the screen

- correct drawing/animation of bg is very sensitive to cpu speed/interrupts/
  interleave, current settings aren't correct but don't think there's any
  visible problems

- engine rev sound may not be completely correct

- bg not using second half of prom, of interest is this half is identical to
  the second half of a bankp/appoooh prom, hardware is similar to bankp/appoooh
  in a few ways, there's also an unused SEGA logo in the bg graphics

- fg not using odd colours, shouldn't matter as the colours are duplicated

- sprite priorities are wrong when bikes are jumping as they are ordered on
  vertical position only, assume this is original behaviour
*/

#include "driver.h"
#include "state.h"

extern data8_t *sprcros2_fgvideoram, *sprcros2_spriteram, *sprcros2_bgvideoram;
extern size_t sprcros2_spriteram_size;

WRITE_HANDLER( sprcros2_fgvideoram_w );
WRITE_HANDLER( sprcros2_bgvideoram_w );
WRITE_HANDLER( sprcros2_bgscrollx_w );
WRITE_HANDLER( sprcros2_bgscrolly_w );

PALETTE_INIT( sprcros2 );
VIDEO_START( sprcros2 );
VIDEO_UPDATE( sprcros2 );
static data8_t *sprcros2_sharedram;
int sprcros2_m_port7 = 0;
static int sprcros2_s_port3 = 0;

static READ_HANDLER( sprcros2_sharedram_r )
{
	return sprcros2_sharedram[offset];
}

static WRITE_HANDLER( sprcros2_sharedram_w )
{
	sprcros2_sharedram[offset]=data;
}

static WRITE_HANDLER( sprcros2_m_port7_w )
{
	unsigned char *RAM = memory_region(REGION_CPU1);

	/*76543210*/
	/*x------- unused*/
	/*-x------ bankswitch halves of scm-01.10k into c000-dfff*/
	/*--xx---- unused*/
    /*----x--- irq enable*/
	/*-----x-- ?? off with title flash and screen clears, possibly layer/sprite enable*/
	/*------x- flip screen*/
	/*-------x nmi enable*/

	if((sprcros2_m_port7^data)&0x40)
		cpu_setbank(1,&RAM[0x10000+((data&0x40)<<7)]);

	tilemap_set_flip( ALL_TILEMAPS,data&0x02?(TILEMAP_FLIPX|TILEMAP_FLIPY):0 );

	sprcros2_m_port7 = data;
}

static WRITE_HANDLER( sprcros2_s_port3_w )
{
	unsigned char *RAM = memory_region(REGION_CPU2);

	/*76543210*/
	/*xxxx---- unused*/
	/*----x--- bankswitch halves of scs-27.5k into c000-dfff*/
	/*-----xx- unused*/
	/*-------x nmi enable*/

	if((sprcros2_s_port3^data)&0x08)
		cpu_setbank(2,&RAM[0x10000+((data&0x08)<<10)]);

	sprcros2_s_port3 = data;
}

static MEMORY_READ_START( sprcros2_m_readmem )
	{ 0x0000, 0xbfff, MRA_ROM },
	{ 0xc000, 0xdfff, MRA_BANK1 },
	{ 0xe000, 0xf7ff, MRA_RAM },
	{ 0xf800, 0xffff, MRA_RAM },						/*shared with slave cpu*/
MEMORY_END

static MEMORY_WRITE_START( sprcros2_m_writemem )
	{ 0x0000, 0xbfff, MWA_ROM },
	{ 0xc000, 0xdfff, MWA_BANK1 },
	{ 0xe000, 0xe7ff, sprcros2_fgvideoram_w, &sprcros2_fgvideoram },
	{ 0xe800, 0xe817, MWA_RAM },						/*always zero*/
	{ 0xe818, 0xe83f, MWA_RAM, &sprcros2_spriteram, &sprcros2_spriteram_size },
	{ 0xe840, 0xefff, MWA_RAM },						/*always zero*/
	{ 0xf000, 0xf7ff, MWA_RAM },
	{ 0xf800, 0xffff, MWA_RAM, &sprcros2_sharedram },	/*shared with slave cpu*/
MEMORY_END

static PORT_READ_START( sprcros2_m_readport )
	{ 0x00, 0x00, input_port_0_r },
	{ 0x01, 0x01, input_port_1_r },
	{ 0x02, 0x02, input_port_2_r },
	{ 0x04, 0x04, input_port_3_r },
	{ 0x05, 0x05, input_port_4_r },
PORT_END

static PORT_WRITE_START( sprcros2_m_writeport )
	{ 0x00, 0x00, SN76496_0_w },
	{ 0x01, 0x01, SN76496_1_w },
	{ 0x02, 0x02, SN76496_2_w },
	{ 0x07, 0x07, sprcros2_m_port7_w },
PORT_END

static MEMORY_READ_START( sprcros2_s_readmem )
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0xbfff, MRA_ROM },
	{ 0xc000, 0xdfff, MRA_BANK2 },
	{ 0xe000, 0xf7ff, MRA_RAM },
	{ 0xf800, 0xffff, sprcros2_sharedram_r },
MEMORY_END

static MEMORY_WRITE_START( sprcros2_s_writemem )
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0x8000, 0xbfff, MWA_ROM },
	{ 0xc000, 0xdfff, MWA_BANK2 },
	{ 0xe000, 0xe7ff, sprcros2_bgvideoram_w, &sprcros2_bgvideoram },
	{ 0xe800, 0xefff, MWA_RAM },						/*always zero*/
	{ 0xf000, 0xf7ff, MWA_RAM },
	{ 0xf800, 0xffff, sprcros2_sharedram_w },
MEMORY_END

static PORT_WRITE_START( sprcros2_s_writeport )
	{ 0x00, 0x00, sprcros2_bgscrollx_w },
	{ 0x01, 0x01, sprcros2_bgscrolly_w },
	{ 0x03, 0x03, sprcros2_s_port3_w },
PORT_END

INPUT_PORTS_START( sprcros2 )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START	/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_COIN3 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON2 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON2 | IPF_COCKTAIL )
	PORT_BIT( 0xc0, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START	/* IN3 */
	PORT_DIPNAME( 0x07, 0x00, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x07, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0x05, DEF_STR( 1C_6C ) )
	PORT_BIT( 0xf8, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START	/* IN4 */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Cocktail ) )
	PORT_BIT( 0x0e, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x70, IP_ACTIVE_HIGH, IPT_UNUSED )			/*unused coinage bits*/
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x80, DEF_STR( On ) )
INPUT_PORTS_END

static struct GfxLayout sprcros2_bglayout =
{
	8,8,
	RGN_FRAC(1,3),
	3,
	{ RGN_FRAC(0,3), RGN_FRAC(1,3), RGN_FRAC(2,3) },
	{ STEP8(0,1) },
	{ STEP8(0,8) },
	8*8
};

static struct GfxLayout sprcros2_spritelayout =
{
	32,32,
	RGN_FRAC(1,3),
	3,
	{ RGN_FRAC(0,3), RGN_FRAC(1,3), RGN_FRAC(2,3) },
	{ STEP8(0,1), STEP8(256,1), STEP8(512,1), STEP8(768,1) },
	{ STEP16(0,8), STEP16(128,8) },
	32*32
};

static struct GfxLayout sprcros2_fglayout =
{
	8,8,
	RGN_FRAC(1,1),
	2,
	{ 0, 4 },
	{ STEP4(64,1), STEP4(0,1) },
	{ STEP8(0,8) },
	8*8*2
};

static struct GfxDecodeInfo sprcros2_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &sprcros2_bglayout,     0,   16 },
	{ REGION_GFX2, 0, &sprcros2_spritelayout, 256, 6  },
	{ REGION_GFX3, 0, &sprcros2_fglayout,     512, 64 },
	{ -1 } /* end of array */
};

static struct SN76496interface sprcros2_sn76496_interface =
{
	3,
	{ 10000000/4, 10000000/4, 10000000/4 },
	{ 50, 50, 50 }
};

static INTERRUPT_GEN( sprcros2_m_interrupt )
{
	if (cpu_getiloops() == 0)
	{
		if(sprcros2_m_port7&0x01)
			cpu_set_irq_line(0, IRQ_LINE_NMI, PULSE_LINE);
	}
	else
	{
		if(sprcros2_m_port7&0x08)
			cpu_set_irq_line(0, 0, HOLD_LINE);
	}
}

static INTERRUPT_GEN( sprcros2_s_interrupt )
{
	if(sprcros2_s_port3&0x01)
		cpu_set_irq_line(1, IRQ_LINE_NMI, PULSE_LINE);
}

static MACHINE_DRIVER_START( sprcros2 )

	/* basic machine hardware */
	MDRV_CPU_ADD(Z80,10000000/2)
	MDRV_CPU_MEMORY(sprcros2_m_readmem,sprcros2_m_writemem)
	MDRV_CPU_PORTS(sprcros2_m_readport,sprcros2_m_writeport)
	MDRV_CPU_VBLANK_INT(sprcros2_m_interrupt,2)	/*1 nmi + 1 irq*/

	MDRV_CPU_ADD(Z80,10000000/2)
	MDRV_CPU_MEMORY(sprcros2_s_readmem,sprcros2_s_writemem)
	MDRV_CPU_PORTS(0,sprcros2_s_writeport)
	MDRV_CPU_VBLANK_INT(sprcros2_s_interrupt,2)	/*2 nmis*/

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(1*8, 31*8-1, 2*8, 30*8-1)
	MDRV_GFXDECODE(sprcros2_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(18)
	MDRV_COLORTABLE_LENGTH(768)

	MDRV_PALETTE_INIT(sprcros2)
	MDRV_VIDEO_START(sprcros2)
	MDRV_VIDEO_UPDATE(sprcros2)

	/* sound hardware */
	MDRV_SOUND_ADD(SN76496, sprcros2_sn76496_interface)
MACHINE_DRIVER_END

static DRIVER_INIT( sprcros2 )
{
	state_save_register_int("main", 0, "m_cpu_port7", &sprcros2_m_port7);
	state_save_register_int("main", 0, "s_cpu_port3", &sprcros2_s_port3);
}

ROM_START( sprcros2 )
	ROM_REGION( 0x14000, REGION_CPU1, 0 )
	ROM_LOAD( "scm-03.10g", 0x00000, 0x4000, CRC(b9757908) SHA1(d59cb2aac1b6268fc766306850f5711d4a12d897) )
	ROM_LOAD( "scm-02.10j", 0x04000, 0x4000, CRC(849c5c87) SHA1(0e02c4990e371d6a290efa53301818e769648945) )
	ROM_LOAD( "scm-01.10k", 0x08000, 0x4000, CRC(385a62de) SHA1(847bf9d97ab3fa8949d9198e4e509948a940d6aa) )

	ROM_LOAD( "scm-00.10l", 0x10000, 0x4000, CRC(13fa3684) SHA1(611b7a237e394f285dcc5beb027dacdbdd58a7a0) )	/*banked into c000-dfff*/

	ROM_REGION( 0x14000, REGION_CPU2, 0 )
	ROM_LOAD( "scs-30.5f",  0x00000, 0x4000, CRC(c0a40e41) SHA1(e74131b353855749258dffa45091c825ccdbf05a) )
	ROM_LOAD( "scs-29.5h",  0x04000, 0x4000, CRC(83d49fa5) SHA1(7112110df2f382bbc0e651adcec975054a485b9b) )
	ROM_LOAD( "scs-28.5j",  0x08000, 0x4000, CRC(480d351f) SHA1(d1b86f441ae0e58b30e0f089ab25de219d5f30e3) )

	ROM_LOAD( "scs-27.5k",  0x10000, 0x4000, CRC(2cf720cb) SHA1(a95c5b8c88371cf597bb7d80afeca6a48c7b74e6) )	/*banked into c000-dfff*/

	ROM_REGION( 0xc000, REGION_GFX1, ROMREGION_DISPOSE )	/*bg*/
	ROM_LOAD( "scs-26.4b",   0x0000, 0x4000, CRC(f958b56d) SHA1(a1973179d336d2ba57294155550515f2b8a33a09) )
	ROM_LOAD( "scs-25.4c",   0x4000, 0x4000, CRC(d6fd7ba5) SHA1(1c26c4c1655b2be9cb6103e75386cc2f0cf27fc5) )
	ROM_LOAD( "scs-24.4e",   0x8000, 0x4000, CRC(87783c36) SHA1(7102be795afcddd76b4d41823e95c65fe1ffbca0) )

	ROM_REGION( 0xc000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "scm-23.5b",   0x0000, 0x4000, CRC(ab42f8e3) SHA1(8c2213b7c47a48e223fc3f7d323d16c0e4cd0457) )	/*sprites*/
	ROM_LOAD( "scm-22.5e",   0x4000, 0x4000, CRC(0cad254c) SHA1(36e30e30b652b3a388a3c4a82251196f79368f59) )
	ROM_LOAD( "scm-21.5g",   0x8000, 0x4000, CRC(b6b68998) SHA1(cc3c6d996beeedcc7e5199f10d65c5b1d3c6e666) )

	ROM_REGION( 0x4000, REGION_GFX3, ROMREGION_DISPOSE )	/*fg*/
	ROM_LOAD( "scm-20.5k",   0x0000, 0x4000, CRC(67a099a6) SHA1(43981abdcaa0ff36183027a3c691ce2df7f06ec7) )

	ROM_REGION( 0x0420, REGION_PROMS, 0 )
	ROM_LOAD( "sc-64.6a",    0x0000, 0x0020, CRC(336dd1c0) SHA1(f0a0d2c13617fd84ee55c0cb96643761a8735147) )	/*palette*/
	ROM_LOAD( "sc-63.3b",    0x0020, 0x0100, CRC(9034a059) SHA1(1801965b4f0f3e04ca4b3faf0ba3a27dbb008474) )	/*bg clut lo nibble*/
	ROM_LOAD( "sc-62.3a",    0x0120, 0x0100, CRC(3c78a14f) SHA1(8f9c196a3e18bdce2d4855bc285bd5bde534bf09) )	/*bg clut hi nibble*/
	ROM_LOAD( "sc-61.5a",    0x0220, 0x0100, CRC(2f71185d) SHA1(974fbb52285f01f4353e9acb1992dcd6fdefedcb) )	/*sprite clut*/
	ROM_LOAD( "sc-60.4k",    0x0320, 0x0100, CRC(d7a4e57d) SHA1(6db02ec6aa55b05422cb505e63c71e36b4b11b4a) )	/*fg clut*/
ROM_END

GAME( 1986, sprcros2, 0, sprcros2, sprcros2, sprcros2, ROT0, "GM Shoji", "Super Cross 2 (Japan)" )
