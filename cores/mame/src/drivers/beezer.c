/*

  Beezer - (c) 1982 Tong Electronic

  Written by Mathis Rosenhauer

*/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "machine/6522via.h"
#include "cpu/m6809/m6809.h"

/* from vidhrdw/beezer.c */
extern UINT8 *videoram;
INTERRUPT_GEN( beezer_interrupt );
VIDEO_UPDATE( beezer );
WRITE_HANDLER( beezer_ram_w );

/* from machine/beezer.c */
DRIVER_INIT( beezer );
WRITE_HANDLER( beezer_bankswitch_w );

static MEMORY_READ_START( readmem )
	{ 0x0000, 0xbfff, MRA_RAM },
	{ 0xc000, 0xcfff, MRA_BANK1 },
	{ 0xd000, 0xffff, MRA_ROM },
MEMORY_END

static MEMORY_WRITE_START( writemem )
	{ 0x0000, 0xbfff, beezer_ram_w, &videoram, &videoram_size },
	{ 0xc000, 0xcfff, MWA_BANK1 },
	{ 0xd000, 0xffff, beezer_bankswitch_w },
MEMORY_END

static MEMORY_READ_START( readmem_sound )
	{ 0x0000, 0x07ff, MRA_RAM },
/*	{ 0x1000, 0x10ff, beezer_6840_r },*/
	{ 0x1800, 0x18ff, via_1_r },
	{ 0xe000, 0xffff, MRA_ROM },
MEMORY_END

static MEMORY_WRITE_START( writemem_sound )
	{ 0x0000, 0x07ff, MWA_RAM },
/*	{ 0x1000, 0x10ff, beezer_6840_w },*/
	{ 0x1800, 0x18ff, via_1_w },
/*	{ 0x8000, 0x9fff, beezer_dac_w },*/
	{ 0xe000, 0xffff, MWA_ROM },
MEMORY_END



INPUT_PORTS_START( beezer )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_TILT )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* IN1 */
	PORT_ANALOG( 0x0f, 0x00, IPT_TRACKBALL_X | IPF_REVERSE, 20, 10, 0, 0)
	PORT_START	/* IN2 */
	PORT_ANALOG( 0x0f, 0x00, IPT_TRACKBALL_Y | IPF_REVERSE, 20, 10, 0, 0)

	PORT_START	/* IN3 */
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0x04, 0x00, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x04, "3" )
	PORT_DIPSETTING(    0x00, "4" )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	PORT_DIPNAME( 0x30, 0x10, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x20, "30000" )
	PORT_DIPSETTING(    0x10, "60000" )
	PORT_DIPSETTING(    0x00, "90000" )
	PORT_DIPSETTING(    0x30, DEF_STR( No ) )
	PORT_DIPNAME( 0xc0, 0xc0, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0xc0, "Easy" )
	PORT_DIPSETTING(    0x80, "Medium" )
	PORT_DIPSETTING(    0x40, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
INPUT_PORTS_END

static struct DACinterface dac_interface =
{
	1,
	{ 100 }
};

static MACHINE_DRIVER_START( beezer )

	/* basic machine hardware */
	MDRV_CPU_ADD(M6809, 1000000)        /* 1 MHz */
	MDRV_CPU_MEMORY(readmem,writemem)
	MDRV_CPU_VBLANK_INT(beezer_interrupt,128)

	MDRV_CPU_ADD(M6809, 1000000)        /* 1 MHz */
	MDRV_CPU_MEMORY(readmem_sound,writemem_sound)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(256, 384)
	MDRV_VISIBLE_AREA(0, 256-1, 16, 303)
	MDRV_PALETTE_LENGTH(16)

	MDRV_VIDEO_START(generic)
	MDRV_VIDEO_UPDATE(beezer)

	/* sound hardware */
	MDRV_SOUND_ADD(DAC, dac_interface)
MACHINE_DRIVER_END

/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( beezer )
	ROM_REGION( 0x20000, REGION_CPU1, 0 )     /* 64k for main CPU */
	ROM_LOAD( "g1",   0x0d000, 0x1000, CRC(3467a0ec) SHA1(0b094a9bf772b101acd26cf09009c67dd4785ed2) )
	ROM_LOAD( "g3",   0x0e000, 0x1000, CRC(9950cdf2) SHA1(b2b59cc1080357de6ba297392881d626157df809) )
	ROM_LOAD( "g5",   0x0f000, 0x1000, CRC(a4b09879) SHA1(69739dd1d3c88ee6ab310ca3c71b3b50d8ec618f) )

	ROM_LOAD( "f1",   0x12000, 0x2000, CRC(ce1b0b8b) SHA1(8ed1d793928bb7afa041a4f61e0c2f78b4442f2f) )
	ROM_LOAD( "f3",   0x14000, 0x2000, CRC(6a11072a) SHA1(9700beaec669849da4d0e39d6dbf0b872d7f1b7f) )
	ROM_LOAD( "e1",   0x16000, 0x1000, CRC(21e4ca9b) SHA1(4024678a4006614051675858ba65db655931a539) )
	ROM_LOAD( "e3",   0x18000, 0x1000, CRC(a4f735d7) SHA1(110061d1c63a331384729951f93a31e62744d0d7) )
	ROM_LOAD( "e5",   0x1a000, 0x1000, CRC(0485575b) SHA1(c3be070541459fad4da4a71604883b2f3043374a) )
	ROM_LOAD( "f5",   0x1c000, 0x1000, CRC(4b11f572) SHA1(4f283c98a7f1bcf534921b4a54cf564335c53e37) )
	ROM_LOAD( "f7",   0x1e000, 0x1000, CRC(bef67473) SHA1(5759ceeca0bb677cee97b74f1a1087d53c25463a) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )     /* 64k for sound CPU */
	ROM_LOAD( "d7",   0xf000, 0x1000, CRC(23b0782e) SHA1(7751327b84235a2e2700e4bdd21adec205c54f0e) )

	ROM_REGION( 0x0300, REGION_PROMS, 0 )
	ROM_LOAD( "d1.cpu", 0x000, 0x0100, CRC(8db17a40) SHA1(0e04a4f5f99b302dbbbfda438808d549f8680fe2) )
	ROM_LOAD( "e1.cpu", 0x100, 0x0100, CRC(3c775c5e) SHA1(ac86f45938c0c9d5fec1245bf86718442baf445b) )
ROM_END

ROM_START( beezer1 )
	ROM_REGION( 0x20000, REGION_CPU1, 0 )     /* 64k for main CPU */
	ROM_LOAD( "g1.32",   0x0d000, 0x1000, CRC(3134cb93) SHA1(7d4a484378b66ccf2fded31885d6dfb2abae9317) )
	ROM_LOAD( "g3.32",   0x0e000, 0x1000, CRC(a3cb2c2d) SHA1(1e17eb0eaf02f86865845a065a5f714fc51aa7d6) )
	ROM_LOAD( "g5.32",   0x0f000, 0x1000, CRC(5e559bf9) SHA1(cd3713f3ed1215ea5c5640474ba6f005242cd093) )

	ROM_LOAD( "f1.64",   0x12000, 0x2000, CRC(b8a78cca) SHA1(4218ef8c4c8e10d7cc47d6de4c4d189ef3c0f0a1) )
	ROM_LOAD( "f3.32",   0x14000, 0x1000, CRC(bfa023f5) SHA1(56fb15e2db61197e1aec5a5825beff7c788a4ba3) )
	ROM_LOAD( "e1",      0x16000, 0x1000, CRC(21e4ca9b) SHA1(4024678a4006614051675858ba65db655931a539) )
	ROM_LOAD( "e3",      0x18000, 0x1000, CRC(a4f735d7) SHA1(110061d1c63a331384729951f93a31e62744d0d7) )
	ROM_LOAD( "e5",      0x1a000, 0x1000, CRC(0485575b) SHA1(c3be070541459fad4da4a71604883b2f3043374a) )
	ROM_LOAD( "f5",      0x1c000, 0x1000, CRC(4b11f572) SHA1(4f283c98a7f1bcf534921b4a54cf564335c53e37) )
	ROM_LOAD( "f7",      0x1e000, 0x1000, CRC(bef67473) SHA1(5759ceeca0bb677cee97b74f1a1087d53c25463a) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )     /* 64k for sound CPU */
	ROM_LOAD( "d7.32",   0xf000, 0x1000, CRC(b11028b5) SHA1(db8958f0bb12e333ce056da3338f1a824dda36e0) )

	ROM_REGION( 0x0300, REGION_PROMS, 0 )
	ROM_LOAD( "d1.cpu", 0x000, 0x0100, CRC(8db17a40) SHA1(0e04a4f5f99b302dbbbfda438808d549f8680fe2) )
	ROM_LOAD( "e1.cpu", 0x100, 0x0100, CRC(3c775c5e) SHA1(ac86f45938c0c9d5fec1245bf86718442baf445b) )
ROM_END

GAMEX( 1982, beezer,  0,       beezer, beezer, beezer, ORIENTATION_FLIP_X, "Tong Electronic", "Beezer (set 1)", GAME_IMPERFECT_SOUND )
GAMEX( 1982, beezer1,  beezer, beezer, beezer, beezer, ORIENTATION_FLIP_X, "Tong Electronic", "Beezer (set 2)", GAME_IMPERFECT_SOUND )
