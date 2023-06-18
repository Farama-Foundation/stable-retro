/*************************************************************

    Namco ND-1 Driver - Mark McDougall

        With contributions from:
            James Jenkins
            Walter Fath

    Currently Supported Games:
        Namco Classics Vol #1
        Namco Classics Vol #2

    T.B.D.
        Sound

 *************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "cpu/m6809/m6809.h"
#include "vidhrdw/ygv608.h"
#include "namcond1.h"


/*************************************************************/

static MEMORY_READ16_START( readmem )
	{ 0x000000, 0x0fffff, MRA16_ROM },
	{ 0x400000, 0x40ffff, namcond1_shared_ram_r },  /* shared ram*/
	{ 0x800000, 0x80000f, ygv608_r },
	{ 0xA00000, 0xA03FFF, MRA16_RAM },              /* EEPROM*/
#ifdef MAME_DEBUG
	{ 0xB00000, 0xB00001, debug_trigger },
#endif
	{ 0xc3ff00, 0xc3ffff, namcond1_cuskey_r },
MEMORY_END

static MEMORY_WRITE16_START( writemem )
	{ 0x000000, 0x0fffff, MWA16_NOP },
	{ 0x400000, 0x40ffff, namcond1_shared_ram_w, &namcond1_shared_ram },        /* shared ram?*/
	{ 0x800000, 0x80000f, ygv608_w },
	{ 0xA00000, 0xA03FFF, MWA16_RAM, &namcond1_eeprom },
	{ 0xc3ff00, 0xc3ff0f, namcond1_cuskey_w },
MEMORY_END

/*************************************************************/

INPUT_PORTS_START( namcond1 )
	PORT_START      /* player 1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START      /* player 2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START2 )

	PORT_START  	/* dipswitches */
	PORT_DIPNAME( 0x01, 0x00, "Freeze" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x01, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x00, "Test" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x02, DEF_STR( On ) )
	PORT_BITX( 0x04, IP_ACTIVE_HIGH, IPF_TOGGLE, "Test", KEYCODE_F2, IP_JOY_NONE )
	PORT_BIT_IMPULSE( 0x10, IP_ACTIVE_HIGH, IPT_SERVICE1, 1 )
	PORT_BIT( 0xE8, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START    /* coin mech inputs - a hack */
	PORT_BIT( 0xf0, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_COIN4 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_COIN3 )
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN1 )
INPUT_PORTS_END


/* text-layer characters */

static struct GfxLayout pts_8x8_4bits_layout =
{
	8,8,	      /* 8*8 pixels */
	RGN_FRAC(1,1),        /* 65536 patterns */
	4,	          /* 4 bits per pixel */
	{ 0, 1, 2, 3 },
    { STEP8( 0*256, 4 ) },
    { STEP8( 0*256, 8*4 ) },
	8*8*4
};

static struct GfxLayout pts_16x16_4bits_layout =
{
	16,16,        /* 16*16 pixels */
	RGN_FRAC(1,1),        /* 16384 patterns */
	4,	          /* 4 bits per pixel */
	{ 0, 1, 2, 3 },
    { STEP8( 0*256, 4 ), STEP8( 1*256, 4 ) },
    { STEP8( 0*256, 8*4 ), STEP8( 2*256, 8*4 ) },
	16*16*4
};

static struct GfxLayout pts_32x32_4bits_layout =
{
	32,32,        /* 32*32 pixels */
	RGN_FRAC(1,1),         /* 4096 patterns */
	4,	          /* 4 bits per pixel */
	{ 0, 1, 2, 3 },
    { STEP8( 0*256, 4 ), STEP8( 1*256, 4 ), STEP8( 4*256, 4 ), STEP8( 5*256, 4 ) },
    { STEP8( 0*256, 8*4 ), STEP8( 2*256, 8*4 ), STEP8( 8*256, 8*4 ), STEP8( 10*256, 8*4 ) },
	32*32*4
};

static struct GfxLayout pts_64x64_4bits_layout =
{
	64,64,        /* 32*32 pixels */
	RGN_FRAC(1,1),         /* 1024 patterns */
	4,	          /* 4 bits per pixel */
	{ 0, 1, 2, 3 },
    { STEP8( 0*256, 4 ), STEP8( 1*256, 4 ), STEP8( 4*256, 4 ), STEP8( 5*256, 4 ),
      STEP8( 16*256, 4 ), STEP8( 17*256, 4 ), STEP8( 20*256, 4 ), STEP8( 21*256, 4 ) },
    { STEP8( 0*256, 8*4 ), STEP8( 2*256, 8*4 ), STEP8( 8*256, 8*4 ), STEP8( 10*256, 8*4 ),
      STEP8( 32*256, 8*4 ), STEP8( 34*256, 8*4 ), STEP8( 40*256, 8*4 ), STEP8( 42*256, 8*4 ) },
	64*64*4
};

static struct GfxLayout pts_8x8_8bits_layout =
{
	8,8,	      /* 8*8 pixels */
	RGN_FRAC(1,1),        /* 32768 patterns */
	8,	          /* 8 bits per pixel */
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
    { STEP8( 0*512, 8 ) },
    { STEP8( 0*512, 8*8 ) },
	8*8*8
};

static struct GfxLayout pts_16x16_8bits_layout =
{
	16,16,        /* 16*16 pixels */
	RGN_FRAC(1,1),         /* 8192 patterns */
	8,	          /* 8 bits per pixel */
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
    { STEP8( 0*512, 8 ), STEP8( 1*512, 8 ) },
    { STEP8( 0*512, 8*8 ), STEP8( 2*512, 8*8 ) },
	16*16*8
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0x00000000, &pts_8x8_4bits_layout,    0,  16 },
	{ REGION_GFX1, 0x00000000, &pts_16x16_4bits_layout,  0,  16 },
	{ REGION_GFX1, 0x00000000, &pts_32x32_4bits_layout,  0,  16 },
	{ REGION_GFX1, 0x00000000, &pts_64x64_4bits_layout,  0,  16 },
	{ REGION_GFX1, 0x00000000, &pts_8x8_8bits_layout,    0, 256 },
	{ REGION_GFX1, 0x00000000, &pts_16x16_8bits_layout,  0, 256 },
	{ -1 }
};

/******************************************
  ND-1 Master clock = 49.152MHz
  - 680000  = 12288000 (CLK/4)
  - H8/3002 = 16666667 (CLK/3) ??? huh?
  - H8/3002 = 16384000 (CLK/3)
  - The level 1 interrupt to the 68k has been measured at 60Hz.
*******************************************/

static MACHINE_DRIVER_START( namcond1 )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 12288000)
	MDRV_CPU_MEMORY(readmem,writemem)
	MDRV_CPU_VBLANK_INT(irq1_line_hold, 1)
	MDRV_CPU_PERIODIC_INT(ygv608_timed_interrupt, 1000)

	MDRV_FRAMES_PER_SECOND(60.0)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(100)

	MDRV_MACHINE_INIT(namcond1)
	MDRV_NVRAM_HANDLER(namcond1)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_NEEDS_6BITS_PER_GUN)
	MDRV_SCREEN_SIZE(288, 224)   /* maximum display resolution (512x512 in theory)*/
	MDRV_VISIBLE_AREA(0, 287, 0, 223)   /* default visible area*/
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(256)

	MDRV_VIDEO_START(ygv608)
	MDRV_VIDEO_UPDATE(ygv608)
	MDRV_VIDEO_STOP(ygv608)
MACHINE_DRIVER_END


ROM_START( ncv1 )
	ROM_REGION( 0x100000,REGION_CPU1, 0 )		/* 16MB for Main CPU */
	ROM_LOAD16_WORD( "n1main0b.14d", 0x00000, 0x80000, CRC(4ffc530b) SHA1(23d622d0261a3584236a77b2cefa522a0f46490e) )
	ROM_LOAD16_WORD( "n1main1b.13d", 0x80000, 0x80000, CRC(26499a4e) SHA1(4af0c365713b4a51da684a3423b07cbb70d9599b) )

	ROM_REGION( 0x80000,REGION_CPU2, 0 )		/* sub CPU */
	ROM_LOAD( "nc1sub.1c",          0x00000, 0x80000, CRC(48ea0de2) SHA1(33e57c8d084a960ccbda462d18e355de44ec7ad9) )

	ROM_REGION( 0x200000,REGION_GFX1, ROMREGION_DISPOSE )	/* 2MB character generator */
	ROM_LOAD( "nc1cg0.10c",         0x000000, 0x200000, CRC(355e7f29) SHA1(47d92c4e28c3610a620d3c9b3be558199477f6d8) )

	ROM_REGION( 0x200000,REGION_SOUND1, 0 ) 	/* 2MB sound data */
    ROM_LOAD( "nc1voice.7b",     0x000000, 0x200000, CRC(91c85bd6) SHA1(c2af8b1518b2b601f2b14c3f327e7e3eae9e29fc) )
ROM_END

ROM_START( ncv1j )
	ROM_REGION( 0x100000,REGION_CPU1, 0 )		/* 16MB for Main CPU */
	ROM_LOAD16_WORD( "n1main0j.14d",  0x00000, 0x80000, CRC(48ce0b2b) SHA1(07dfca8ba935ee0151211f9eb4d453f2da1d4bd7) )
	ROM_LOAD16_WORD( "n1main1j.13d",  0x80000, 0x80000, CRC(49f99235) SHA1(97afde7f7dddd8538de78a74325d0038cb1217f7) )

	ROM_REGION( 0x80000,REGION_CPU2, 0 )		/* sub CPU */
	ROM_LOAD( "nc1sub.1c",          0x00000, 0x80000, CRC(48ea0de2) SHA1(33e57c8d084a960ccbda462d18e355de44ec7ad9) )

	ROM_REGION( 0x200000,REGION_GFX1, ROMREGION_DISPOSE )	/* 2MB character generator */
	ROM_LOAD( "nc1cg0.10c",         0x000000, 0x200000, CRC(355e7f29) SHA1(47d92c4e28c3610a620d3c9b3be558199477f6d8) )

	ROM_REGION( 0x200000,REGION_SOUND1, 0 ) 	/* 2MB sound data */
    ROM_LOAD( "nc1voice.7b",     0x000000, 0x200000, CRC(91c85bd6) SHA1(c2af8b1518b2b601f2b14c3f327e7e3eae9e29fc) )
ROM_END

ROM_START( ncv1j2 )
	ROM_REGION( 0x100000,REGION_CPU1, 0 )		/* 16MB for Main CPU */
	ROM_LOAD16_WORD( "1main0ja.14d", 0x00000, 0x80000, CRC(7207469d) SHA1(73faf1973a57c1bc2163e9ee3fe2febd3b8763a4) )
	ROM_LOAD16_WORD( "1main1ja.13d", 0x80000, 0x80000, CRC(52401b17) SHA1(60c9f20831d0101c02dafbc0bd15422f71f3ad81) )

	ROM_REGION( 0x80000,REGION_CPU2, 0 )		/* sub CPU */
	ROM_LOAD( "nc1sub.1c",          0x00000, 0x80000, CRC(48ea0de2) SHA1(33e57c8d084a960ccbda462d18e355de44ec7ad9) )

	ROM_REGION( 0x200000,REGION_GFX1, ROMREGION_DISPOSE )	/* 2MB character generator */
	ROM_LOAD( "nc1cg0.10c",         0x000000, 0x200000, CRC(355e7f29) SHA1(47d92c4e28c3610a620d3c9b3be558199477f6d8) )

	ROM_REGION( 0x200000,REGION_SOUND1, 0 ) 	/* 2MB sound data */
    ROM_LOAD( "nc1voice.7b",     0x000000, 0x200000, CRC(91c85bd6) SHA1(c2af8b1518b2b601f2b14c3f327e7e3eae9e29fc) )
ROM_END

ROM_START( ncv2 )
	ROM_REGION( 0x100000,REGION_CPU1, 0 )		/* 16MB for Main CPU */
	ROM_LOAD16_WORD( "ncs1mn0.14e", 0x00000, 0x80000, CRC(fb8a4123) SHA1(47acdfe9b5441d0e3649aaa9780e676f760c4e42) )
	ROM_LOAD16_WORD( "ncs1mn1.13e", 0x80000, 0x80000, CRC(7a5ef23b) SHA1(0408742424a6abad512b5baff63409fe44353e10) )


	ROM_REGION( 0x80000,REGION_CPU2, 0 )		/* sub CPU */
	ROM_LOAD( "ncs1sub.1d",          0x00000, 0x80000, CRC(365cadbf) SHA1(7263220e1630239e3e88b828c00389d02628bd7d) )

	ROM_REGION( 0x400000,REGION_GFX1, ROMREGION_DISPOSE )	/* 4MB character generator */
	ROM_LOAD( "ncs1cg0.10e",         0x000000, 0x200000, CRC(fdd24dbe) SHA1(4dceaae3d853075f58a7408be879afc91d80292e) )
	ROM_LOAD( "ncs1cg1.10e",         0x200000, 0x200000, CRC(007b19de) SHA1(d3c093543511ec1dd2f8be6db45f33820123cabc) )

	ROM_REGION( 0x200000,REGION_SOUND1, 0 ) 	/* 2MB sound data */
    ROM_LOAD( "ncs1voic.7c",     0x000000, 0x200000, CRC(ed05fd88) SHA1(ad88632c89a9946708fc6b4c9247e1bae9b2944b) )
ROM_END

ROM_START( ncv2j )
	ROM_REGION( 0x100000,REGION_CPU1, 0 )		/* 16MB for Main CPU */
	ROM_LOAD16_WORD( "ncs1mn0j.14e", 0x00000, 0x80000, CRC(99991192) SHA1(e0b0e15ae23560b77119b3d3e4b2d2bb9d8b36c9) )
	ROM_LOAD16_WORD( "ncs1mn1j.13e", 0x80000, 0x80000, CRC(af4ba4f6) SHA1(ff5adfdd462cfd3f17fbe2401dfc88ff8c71b6f8) )

	ROM_REGION( 0x80000,REGION_CPU2, 0 )		/* sub CPU */
	ROM_LOAD( "ncs1sub.1d",          0x00000, 0x80000, CRC(365cadbf) SHA1(7263220e1630239e3e88b828c00389d02628bd7d) )

	ROM_REGION( 0x400000,REGION_GFX1, ROMREGION_DISPOSE )	/* 4MB character generator */
	ROM_LOAD( "ncs1cg0.10e",         0x000000, 0x200000, CRC(fdd24dbe) SHA1(4dceaae3d853075f58a7408be879afc91d80292e) )
	ROM_LOAD( "ncs1cg1.10e",         0x200000, 0x200000, CRC(007b19de) SHA1(d3c093543511ec1dd2f8be6db45f33820123cabc) )

	ROM_REGION( 0x200000,REGION_SOUND1, 0 ) 	/* 2MB sound data */
    ROM_LOAD( "ncs1voic.7c",     0x000000, 0x200000, CRC(ed05fd88) SHA1(ad88632c89a9946708fc6b4c9247e1bae9b2944b) )
ROM_END

#if 0

static void namcond1_patch( int *addr )
{
    unsigned char *ROM = memory_region(REGION_CPU1);
    int             i;

    for( i=0; addr[i]; i++ )
      /* insert a NOP instruction*/
      WRITE_WORD( &ROM[addr[i]], 0x4e71 );
}

/*
 *  These are the patch locations to skip coldboot check
 *  - they were required before the MCU simulation was
 *    sufficiently advanced.
 *  - please do not delete these comments *just in case*!
 *
 *  ncv1    0x15038
 *  ncv1j   0x151c0
 *  ncv1j2  0x152e4
 *  ncv2    0x17974
 *  ncv2j   0x17afc
 */

#endif


GAMEX( 1995, ncv1,      0, namcond1, namcond1, 0, ROT90, "Namco", "Namco Classics Collection Vol.1", GAME_NO_SOUND | GAME_IMPERFECT_GRAPHICS )
GAMEX( 1995, ncv1j,  ncv1, namcond1, namcond1, 0, ROT90, "Namco", "Namco Classics Collection Vol.1 (Japan set 1)", GAME_NO_SOUND | GAME_IMPERFECT_GRAPHICS )
GAMEX( 1995, ncv1j2, ncv1, namcond1, namcond1, 0, ROT90, "Namco", "Namco Classics Collection Vol.1 (Japan set 2)", GAME_NO_SOUND | GAME_IMPERFECT_GRAPHICS )
GAMEX( 1996, ncv2,      0, namcond1, namcond1, 0, ROT90, "Namco", "Namco Classics Collection Vol.2", GAME_NOT_WORKING | GAME_NO_SOUND | GAME_IMPERFECT_GRAPHICS )
GAMEX( 1996, ncv2j,  ncv2, namcond1, namcond1, 0, ROT90, "Namco", "Namco Classics Collection Vol.2 (Japan)", GAME_NOT_WORKING | GAME_NO_SOUND | GAME_IMPERFECT_GRAPHICS )
