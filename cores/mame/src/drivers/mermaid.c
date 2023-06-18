/***************************************************************************

Mermaid

Driver by Zsolt Vasvari

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"


extern unsigned char* mermaid_background_videoram;
extern unsigned char* mermaid_foreground_videoram;
extern unsigned char* mermaid_foreground_colorram;
extern unsigned char* mermaid_background_scrollram;
extern unsigned char* mermaid_foreground_scrollram;


PALETTE_INIT( mermaid );
VIDEO_UPDATE( mermaid );
WRITE_HANDLER( mermaid_flip_screen_x_w );
WRITE_HANDLER( mermaid_flip_screen_y_w );


static unsigned char *mermaid_AY8910_enable;

static WRITE_HANDLER( mermaid_AY8910_write_port_w )
{
	if (mermaid_AY8910_enable[0])  AY8910_write_port_0_w(offset, data);
	if (mermaid_AY8910_enable[1])  AY8910_write_port_1_w(offset, data);
}

static WRITE_HANDLER( mermaid_AY8910_control_port_w )
{
	if (mermaid_AY8910_enable[0])  AY8910_control_port_0_w(offset, data);
	if (mermaid_AY8910_enable[1])  AY8910_control_port_1_w(offset, data);
}

#if 0
static READ_HANDLER( mermaid_f800_r )
{
	/* collision register active LO*/
	/* Bit 0*/
	/* Bit 1 - Sprite - Foreground*/
	/* Bit 2 - Sprite - Stream*/
	/* Bit 3*/
	/* Bit 4*/
	/* Bit 5*/
	/* Bit 6*/
	/* Bit 7*/
	/*return rand() & 0xff;*/
	return 0x00;
}
#endif

static MEMORY_READ_START( readmem )
	{ 0x0000, 0x9fff, MRA_ROM },
	{ 0xc000, 0xcbff, MRA_RAM },
	{ 0xd000, 0xd3ff, MRA_RAM },
	{ 0xd800, 0xd81f, MRA_RAM },
	{ 0xd840, 0xd8bf, MRA_RAM },
	{ 0xdc00, 0xdfff, MRA_RAM },
	{ 0xe000, 0xe000, input_port_0_r },
	{ 0xe800, 0xe800, input_port_1_r },
	{ 0xf000, 0xf000, input_port_2_r },
	{ 0xf800, 0xf800, input_port_3_r },
/*	{ 0xf800, 0xf800, mermaid_f800_r },*/
MEMORY_END

static MEMORY_WRITE_START( writemem )
	{ 0x0000, 0x9fff, MWA_ROM },
	{ 0xc000, 0xc7ff, MWA_RAM },
	{ 0xc800, 0xcbff, MWA_RAM, &mermaid_background_videoram, &videoram_size },
	{ 0xd000, 0xd3ff, MWA_RAM, &mermaid_foreground_videoram },
	{ 0xd800, 0xd81f, MWA_RAM, &mermaid_background_scrollram },
	{ 0xd840, 0xd85f, MWA_RAM, &mermaid_foreground_scrollram },
	{ 0xd880, 0xd8bf, MWA_RAM, &spriteram, &spriteram_size },
	{ 0xdc00, 0xdfff, MWA_RAM, &mermaid_foreground_colorram },
	{ 0xe000, 0xe001, MWA_RAM, &mermaid_AY8910_enable },
	{ 0xe005, 0xe005, mermaid_flip_screen_x_w },
	{ 0xe006, 0xe006, mermaid_flip_screen_y_w },
	{ 0xe007, 0xe007, interrupt_enable_w },
	{ 0xe807, 0xe807, MWA_NOP },	/* watchdog? */
	{ 0xf802, 0xf802, MWA_NOP },	/* ??? see memory map */
	{ 0xf806, 0xf806, mermaid_AY8910_write_port_w },
	{ 0xf807, 0xf807, mermaid_AY8910_control_port_w },
MEMORY_END


INPUT_PORTS_START( mermaid )
	PORT_START      /* DSW */
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x02, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x00, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x00, "20000" )
	PORT_DIPSETTING(    0x04, "30000" )
	PORT_DIPNAME( 0x08, 0x08, "Allow Continue" )
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x30, 0x00, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x10, "4" )
	PORT_DIPSETTING(    0x20, "5" )
	PORT_DIPSETTING(    0x30, "6" )
	PORT_DIPNAME( 0xc0, 0x00, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 1C_2C ))
	PORT_DIPSETTING(    0x80, DEF_STR( 1C_3C ) )

	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP    | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN  | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_COIN2 )

	PORT_START		/* Fake IN for debug features (0xf800 -> 0xc01a, cpl) */
	PORT_DIPNAME( 0x01, 0x00, "0xf800 bit 0" )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x00, "0xf800 bit 1" )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x00, "0xf800 bit 2" )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x00, "0xf800 bit 3" )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x00, "0xf800 bit 4" )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x00, "0xf800 bit 5" )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x00, "0xf800 bit 6" )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x00, "0xf800 bit 7" )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
INPUT_PORTS_END


static struct GfxLayout background_charlayout =
{
	8,8,    /* 8*8 chars */
	RGN_FRAC(1,1),    /* 256 characters */
	1,      /* 1 bit per pixel */
	{ 0 },  /* single bitplane */
	{ 0, 1, 2, 3, 4, 5, 6, 7},
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8},
	8*8     /* every char takes 8 consecutive bytes */
};

static struct GfxLayout foreground_charlayout =
{
	8,8,    /* 8*8 chars */
	RGN_FRAC(1,2),   /* 1024 characters */
	2,      /* 2 bits per pixel */
	{ 0, RGN_FRAC(1,2) },  /* the two bitplanes are separated */
	{ 0, 1, 2, 3, 4, 5, 6, 7},
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8},
	8*8     /* every char takes 8 consecutive bytes */
};

static struct GfxLayout spritelayout =
{
	16,16,	/* 16*16 sprites */
	RGN_FRAC(1,2),	/* 256 sprites */
	2,		/* 2 bits per pixel */
	{ 0, RGN_FRAC(1,2) },	/* the two bitplanes are separated */
	{ 0, 1, 2, 3, 4, 5, 6, 7,
	  8*8+0, 8*8+1, 8*8+2, 8*8+3, 8*8+4, 8*8+5, 8*8+6, 8*8+7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
	  16*8, 17*8, 18*8, 19*8, 20*8, 21*8, 22*8, 23*8 },
	32*8	/* every sprite takes 32 consecutive bytes */
};


static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &foreground_charlayout,     0, 16 },
	{ REGION_GFX1, 0, &spritelayout,              0, 16 },
	{ REGION_GFX2, 0, &background_charlayout,  4*16, 2  },
	{ -1 } /* end of array */
};


static struct AY8910interface ay8910_interface =
{
	2,	/* 2 chips */
	1500000,	/* 1.5 MHz ? */
	{ 25, 25 },
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 }
};


static MACHINE_DRIVER_START( mermaid )

	/* basic machine hardware */
	MDRV_CPU_ADD(Z80, 4000000)        /* 4.00 MHz??? */
	MDRV_CPU_MEMORY(readmem,writemem)
	MDRV_CPU_VBLANK_INT(nmi_line_pulse,1)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 2*8, 30*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(4*16+1)
	MDRV_COLORTABLE_LENGTH(4*16+2*2)

	MDRV_PALETTE_INIT(mermaid)
	MDRV_VIDEO_START(generic)
	MDRV_VIDEO_UPDATE(mermaid)

	/* sound hardware */
	MDRV_SOUND_ADD(AY8910, ay8910_interface)
MACHINE_DRIVER_END


/***************************************************************************

  Game driver(s)

***************************************************************************/
ROM_START( mermaid )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )       /* 64k for code */
	ROM_LOAD( "g960_32.15",	  0x0000, 0x1000, CRC(8311f090) SHA1(c59485a712cf1cd384f03874c693b58e972fe4da) )
	ROM_LOAD( "g960_33.16",	  0x1000, 0x1000, CRC(9f274fc4) SHA1(4098e98c9d95f7e621de061925374154a23c5d35) )
	ROM_LOAD( "g960_34.17",	  0x2000, 0x1000, CRC(5f910179) SHA1(bcf1e24b7584d18f9e85a8b4aec6f03bb1034150) )
	ROM_LOAD( "g960_35.18",	  0x3000, 0x1000, CRC(db1868a1) SHA1(f5bb0b9895c5e2facc5ae9db9f1bed44e14d308a) )
	ROM_LOAD( "g960_36.19",	  0x4000, 0x1000, CRC(178a3567) SHA1(993479d9fadf1c4d3f44ce030f2d6197ecfceb9d) )
	ROM_LOAD( "g960_37.20",	  0x5000, 0x1000, CRC(7d602527) SHA1(1a888bd1829b9f12dd820c49785bea6bc8edab04) )
	ROM_LOAD( "g960_38.21",	  0x6000, 0x1000, CRC(bf9f623c) SHA1(48d3aebb01c01c51acaccd1a4582ab21e6ed1104) )
	ROM_LOAD( "g960_39.22",	  0x7000, 0x1000, CRC(df0db390) SHA1(b466cf1abbf0703d6fbacc86c65d254ef310ba27) )
	ROM_LOAD( "g960_40.23",	  0x8000, 0x1000, CRC(fb7aba3f) SHA1(fe6903c11363ed4c34b29226df58e833150cc525) )
	ROM_LOAD( "g960_41.24",	  0x9000, 0x1000, CRC(d022981d) SHA1(ab1659a933af4d49daeacd70072f6c1197181c20) )

	ROM_REGION( 0x4000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "g960_45.77",	  0x0000, 0x1000, CRC(1f6b735e) SHA1(dd7ea4ef674f0495a87fc1929ea14852e8d8d338) )
	ROM_LOAD( "g960_44.76",	  0x1000, 0x1000, CRC(fd76074e) SHA1(673a214fc41b923191b4136c0cf39fc5efa970ba) )
	ROM_LOAD( "g960_47.79",	  0x2000, 0x1000, CRC(3b7d4ad0) SHA1(722483989c611b6396538dd3b357589262f366e3) )
	ROM_LOAD( "g960_46.78",	  0x3000, 0x1000, CRC(50c117cd) SHA1(45b4055497c785218e2aaaffa86d732912555821) )

	ROM_REGION( 0x1000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "g960_43.26",	  0x0000, 0x1000, CRC(6f077417) SHA1(f2c20e03427a2f5a113c6a4cf95875b77a0ec418) )

	ROM_REGION( 0x0040, REGION_PROMS, 0 )
	ROM_LOAD( "col_a", 	      0x0000, 0x0020, CRC(ef87bcd6) SHA1(00a5888ad028fabeb7369eed33be5cd49b6b7bb0) )
	ROM_LOAD( "col_b", 	      0x0020, 0x0020, CRC(ca48abdd) SHA1(a864612c2c33acddfa9993ed10a1d63d2e3f145d) )

	ROM_REGION( 0x1000, REGION_USER1, 0 )	/* unknown */
	ROM_LOAD( "g960_42.39",	  0x0000, 0x1000, CRC(287840bb) SHA1(9a1836f39f328b0c9672976d95a9ece45bb9e89f) )
ROM_END

ROM_START( rougien )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )       /* 64k for code */
	ROM_LOAD( "rou-00.bin", 0x0000,  0x1000,  CRC(14cd1108) SHA1(46657fa4d900936e2a71ad43702d2c43fef09efe) )
	ROM_LOAD( "rou-01.bin", 0x1000,  0x1000,  CRC(ee40670d) SHA1(7c31c3b693999bc1ae42b9f2de1a9883d3db535d) )
	ROM_LOAD( "rou-02.bin", 0x2000,  0x1000,  CRC(5e528f46) SHA1(6bad10bb72eab423b6478e8d5a41e92f0b72793c) )
	ROM_LOAD( "rou-03.bin", 0x3000,  0x1000,  CRC(d235a7e8) SHA1(0c0cbf81d3b379dc5d82c3541dd3b32ccad73046) )
	ROM_LOAD( "rou-04.bin", 0x4000,  0x1000,  CRC(7352bb66) SHA1(0bc41faf67d5305258909c9523461cdf8d10d9dc) )
	ROM_LOAD( "rou-05.bin", 0x5000,  0x1000,  CRC(af77444d) SHA1(d7c2b59fbaa7be813ed5dcd9d7babc19e0fec9fd) )
	ROM_LOAD( "rou-06.bin", 0x6000,  0x1000,  CRC(2c16c857) SHA1(f31189cf73635a1542a18193da85ed898c297768) )
	ROM_LOAD( "rou-07.bin", 0x7000,  0x1000,  CRC(fbf32f2e) SHA1(ab8a2bc8ee10e308e887b2bef2b92e1669aab888) )
	ROM_LOAD( "rou-08.bin", 0x8000,  0x1000,  CRC(bfac531c) SHA1(63e6bdd1ca2709ae733c84311df5833546f08663) )
	ROM_LOAD( "rou-09.bin", 0x9000,  0x1000,  CRC(af854340) SHA1(f2d5e1bb6b25d87ee03c21975f9c9976ae3652b1) )

	ROM_REGION( 0xc000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "rou-20.bin",  0x1000,  0x1000,  CRC(c5dc1258) SHA1(20034c77f205684f9c868747988ab391456a2189) )
	ROM_LOAD( "rou-21.bin",  0x0000,  0x1000,  CRC(36e4ba8c) SHA1(6c39de7d983019b280c54e03d4ca0fe2cef4ea90) )
	ROM_LOAD( "rou-22.bin",  0x3000,  0x1000,  CRC(35811443) SHA1(3e0ec254a94730664a3d13dd10d87d2040c9c5e6) )
	ROM_LOAD( "rou-23.bin",  0x2000,  0x1000,  CRC(5974c848) SHA1(a3e5408aaee87afadea521115f78686f84832ab9) )
	ROM_LOAD( "rou-24.bin",  0x5000,  0x1000,  CRC(56ceb0be) SHA1(a5475ce7d66e9f97da373d3fb694b536a257e78d) )
	ROM_LOAD( "rou-25.bin",  0x4000,  0x1000,  CRC(706d9864) SHA1(26d7e803670f791938a7e93bf3b68a94525c0458) )
	ROM_LOAD( "rou-26.bin",  0x7000,  0x1000,  CRC(fbbc6339) SHA1(a4c7035fe61a267b53372a0504e7932e52ac4119) )
	ROM_LOAD( "rou-27.bin",  0x6000,  0x1000,  CRC(522fa2e0) SHA1(ce20c5e447f27cb147a62c1dd176d9dfa60f4c33) )
	ROM_LOAD( "rou-28.bin",  0x9000,  0x1000,  CRC(33f160dc) SHA1(3fc3a31a37cc724c692080edc2e4fd8678e9a8c9) )
	ROM_LOAD( "rou-29.bin",  0x8000,  0x1000,  CRC(bf018a7e) SHA1(e608630ead16f01f9b186f622644ce2567f29057) )
	ROM_LOAD( "rou-30.bin",  0xb000,  0x1000,  CRC(c75be223) SHA1(ca3fa46d9132a31b46e6e29a8b91acf7a380fd74) )
	ROM_LOAD( "rou-31.bin",  0xa000,  0x1000,  CRC(b2a6f058) SHA1(faba09b6fd80e1e20e79435b60ce89e7110ec98a) )

	ROM_REGION( 0x1000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "rou-43.bin",  0x0000,  0x1000, CRC(ee4b9de4) SHA1(878a86113435536545353f68864c3a034566c616) )

	ROM_REGION( 0x0040, REGION_PROMS, 0 )
	ROM_LOAD( "prom_a.bin", 	      0x0000, 0x0020, CRC(49f619b9) SHA1(c936aaf79822628a2ffff169d236389bc2eef6a5) )
	ROM_LOAD( "prom_b.bin", 	      0x0020, 0x0020, CRC(41ad4fc8) SHA1(a9d24586130f00cd350459635de5f4f7629e00b4) )

	ROM_REGION( 0x3000, REGION_USER1, 0 )	/* unknown */
	ROM_LOAD( "rou-40.bin",  0x0000,  0x1000, CRC(ab38b942) SHA1(9575f67e002c68d384122e05a12c6c0f21335825) )
	ROM_LOAD( "rou-41.bin",  0x1000,  0x1000, CRC(59ed0d88) SHA1(7faf6ab01fa3c1c04c38d2ea27b27c47450876de) )
	ROM_LOAD( "rou-42.bin",  0x2000,  0x1000, CRC(5ce13444) SHA1(e6da83190b26b094159a3a97deffd31d0d20a061) )
ROM_END

GAME ( 1982, mermaid, 0, mermaid, mermaid, 0, ROT0, "[Sanritsu] Rock-ola", "Mermaid" )
GAME ( 1982, rougien, 0, mermaid, mermaid, 0, ROT0, "Sanritsu", "Rougien" )
