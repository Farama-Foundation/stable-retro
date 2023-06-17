/***************************************************************************

Eggs & Dommy

Very similar to Burger Time hardware (and uses its video driver)

driver by Nicola Salmoria

To Do:
Sprite Priorities in Dommy

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"


/* from vidhrdw/btime.c */
PALETTE_INIT( btime );
VIDEO_START( btime );
VIDEO_UPDATE( eggs );

READ_HANDLER( btime_mirrorvideoram_r );
WRITE_HANDLER( btime_mirrorvideoram_w );
READ_HANDLER( btime_mirrorcolorram_r );
WRITE_HANDLER( btime_mirrorcolorram_w );
WRITE_HANDLER( btime_video_control_w );

static MEMORY_READ_START( dommy_readmem )
	{ 0x0000, 0x07ff, MRA_RAM },
	{ 0x2000, 0x27ff, MRA_RAM },
	{ 0x2800, 0x2bff, btime_mirrorvideoram_r },
	{ 0x4000, 0x4000, input_port_2_r },     /* DSW1 */
	{ 0x4001, 0x4001, input_port_3_r },     /* DSW2 */
/*	{ 0x4004, 0x4004, },  */ /* this is read */
	{ 0x4002, 0x4002, input_port_0_r },     /* IN0 */
	{ 0x4003, 0x4003, input_port_1_r },     /* IN1 */
	{ 0xa000, 0xffff, MRA_ROM },
MEMORY_END

static MEMORY_WRITE_START( dommy_writemem )
	{ 0x0000, 0x07ff, MWA_RAM },
	{ 0x2000, 0x23ff, videoram_w, &videoram, &videoram_size },
	{ 0x2400, 0x27ff, colorram_w, &colorram },
	{ 0x2800, 0x2bff, btime_mirrorvideoram_w },
	{ 0x4000, 0x4000, MWA_NOP },
	{ 0x4001, 0x4001, btime_video_control_w },
	{ 0x4004, 0x4004, AY8910_control_port_0_w },
	{ 0x4005, 0x4005, AY8910_write_port_0_w },
	{ 0x4006, 0x4006, AY8910_control_port_1_w },
	{ 0x4007, 0x4007, AY8910_write_port_1_w },
	{ 0xa000, 0xffff, MWA_ROM },
MEMORY_END

static MEMORY_READ_START( eggs_readmem )
	{ 0x0000, 0x07ff, MRA_RAM },
	{ 0x1000, 0x17ff, MRA_RAM },
	{ 0x1800, 0x1bff, btime_mirrorvideoram_r },
	{ 0x1c00, 0x1fff, btime_mirrorcolorram_r },
	{ 0x2000, 0x2000, input_port_2_r },     /* DSW1 */
	{ 0x2001, 0x2001, input_port_3_r },     /* DSW2 */
	{ 0x2002, 0x2002, input_port_0_r },     /* IN0 */
	{ 0x2003, 0x2003, input_port_1_r },     /* IN1 */
	{ 0x3000, 0x7fff, MRA_ROM },
	{ 0xf000, 0xffff, MRA_ROM },    /* reset/interrupt vectors */
MEMORY_END

static MEMORY_WRITE_START( eggs_writemem )
	{ 0x0000, 0x07ff, MWA_RAM },
	{ 0x1000, 0x13ff, videoram_w, &videoram, &videoram_size },
	{ 0x1400, 0x17ff, colorram_w, &colorram },
	{ 0x1800, 0x1bff, btime_mirrorvideoram_w },
	{ 0x1c00, 0x1fff, btime_mirrorcolorram_w },
	{ 0x2000, 0x2000, btime_video_control_w },
	{ 0x2001, 0x2001, MWA_NOP },
	{ 0x2004, 0x2004, AY8910_control_port_0_w },
	{ 0x2005, 0x2005, AY8910_write_port_0_w },
	{ 0x2006, 0x2006, AY8910_control_port_1_w },
	{ 0x2007, 0x2007, AY8910_write_port_1_w },
	{ 0x3000, 0x7fff, MWA_ROM },
MEMORY_END



INPUT_PORTS_START( scregg )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_4WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_4WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_4WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN2 )

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START2 )

	PORT_START      /* DSW1 */
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 1C_3C ) )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 1C_3C ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Cocktail ) )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_VBLANK  )

	PORT_START      /* DSW2 */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x01, "3" )
	PORT_DIPSETTING(    0x00, "5" )
	PORT_DIPNAME( 0x06, 0x04, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x04, "30000" )
	PORT_DIPSETTING(    0x02, "50000" )
	PORT_DIPSETTING(    0x00, "70000"  )
	PORT_DIPSETTING(    0x06, "Never"  )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x00, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x10, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x00, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x20, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x80, "Easy" )
	PORT_DIPSETTING(    0x00, "Hard" )
INPUT_PORTS_END




static struct GfxLayout charlayout =
{
	8,8,    /* 8*8 characters */
	1024,   /* 1024 characters */
	3,      /* 3 bits per pixel */
	{ 2*1024*8*8, 1024*8*8, 0 },    /* the bitplanes are separated */
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8     /* every char takes 8 consecutive bytes */
};

static struct GfxLayout spritelayout =
{
	16,16,  /* 16*16 sprites */
	256,    /* 256 sprites */
	3,      /* 3 bits per pixel */
	{ 2*256*16*16, 256*16*16, 0 },  /* the bitplanes are separated */
	{ 16*8+0, 16*8+1, 16*8+2, 16*8+3, 16*8+4, 16*8+5, 16*8+6, 16*8+7,
	  0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
	  8*8, 9*8, 10*8, 11*8, 12*8, 13*8, 14*8, 15*8 },
	32*8    /* every sprite takes 32 consecutive bytes */
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &charlayout,          0, 1 },     /* char set #1 */
	{ REGION_GFX1, 0, &spritelayout,        0, 1 },     /* sprites */
	{ -1 } /* end of array */
};



static struct AY8910interface ay8910_interface =
{
	2,      /* 2 chips */
	1500000,        /* 1.5 MHz ? (hand tuned) */
	{ 23, 23 },
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 }
};


static MACHINE_DRIVER_START( dommy )

	/* basic machine hardware */
	MDRV_CPU_ADD(M6502, 1500000)
	MDRV_CPU_MEMORY(dommy_readmem,dommy_writemem)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,16)

	MDRV_FRAMES_PER_SECOND(57)
	MDRV_VBLANK_DURATION(3072)        /* frames per second, vblank duration taken from Burger Time */

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 31*8-1, 1*8, 31*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(8)
	MDRV_COLORTABLE_LENGTH(8)

	MDRV_PALETTE_INIT(btime)
	MDRV_VIDEO_START(btime)
	MDRV_VIDEO_UPDATE(eggs)

	/* sound hardware */
	MDRV_SOUND_ADD(AY8910, ay8910_interface)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( scregg )

	/* basic machine hardware */
	MDRV_CPU_ADD(M6502, 1500000)
	MDRV_CPU_MEMORY(eggs_readmem,eggs_writemem)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,16)

	MDRV_FRAMES_PER_SECOND(57)
	MDRV_VBLANK_DURATION(3072)        /* frames per second, vblank duration taken from Burger Time */

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(1*8, 31*8-1, 1*8, 31*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(8)
	MDRV_COLORTABLE_LENGTH(8)

	MDRV_PALETTE_INIT(btime)
	MDRV_VIDEO_START(btime)
	MDRV_VIDEO_UPDATE(eggs)

	/* sound hardware */
	MDRV_SOUND_ADD(AY8910, ay8910_interface)
MACHINE_DRIVER_END


ROM_START( dommy )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )     /* 64k for code */
	ROM_LOAD( "dommy.e01",  0xa000, 0x2000, CRC(9ae064ed) SHA1(73082e5254d54d8386f580cc82a74242a6debd84) )
	ROM_LOAD( "dommy.e11",  0xc000, 0x2000, CRC(7c4fad5c) SHA1(fb733ac979092a6fc278836b82d8ed3fae7a20d9) )
	ROM_LOAD( "dommy.e21",  0xe000, 0x2000, CRC(cd1a4d55) SHA1(f7f4f5ef2e89519652e8401e75dc4e2b8edf4bae) )

	ROM_REGION( 0x6000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "dommy.e50",  0x0000, 0x2000, CRC(5e9db0a4) SHA1(82e60d6b65a4d901102d7e195e66b278f18586f7) )
	ROM_LOAD( "dommy.e40",  0x2000, 0x2000, CRC(4d1c36fb) SHA1(5904421e8e2f727dd6292045871429a1373085e9) )
	ROM_LOAD( "dommy.e30",  0x4000, 0x2000, CRC(4e68bb12) SHA1(de26d278e43882deffad4d5b19d785f8824cf05a) )

	ROM_REGION( 0x0040, REGION_PROMS, 0 ) /* palette decoding is probably wrong */
	ROM_LOAD( "dommy.e70",  0x0018, 0x0008, CRC(50c1d86e) SHA1(990a87a7f7e6a2af67dc6890e2326c7403e46520) )	/* palette */
	ROM_CONTINUE(			  0x0000, 0x0018 )
	ROM_LOAD( "dommy.e60",  0x0020, 0x0020, CRC(24da2b63) SHA1(4db7e1ff1b9fd5ae4098cd7ca66cf1fa2574501a) )	/* unknown */
ROM_END

ROM_START( scregg )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )     /* 64k for code */
	ROM_LOAD( "scregg.e14",   0x3000, 0x1000, CRC(29226d77) SHA1(e1a329a4452eeb90801d001140ce865bf1ea7716) )
	ROM_LOAD( "scregg.d14",   0x4000, 0x1000, CRC(eb143880) SHA1(73b3ca6e0d72cd0db951ae9ed1552cf8b7d91e68) )
	ROM_LOAD( "scregg.c14",   0x5000, 0x1000, CRC(4455f262) SHA1(fc7b2d9094fa5e25c1bf4b68386f640f4502e0c0) )
	ROM_LOAD( "scregg.b14",   0x6000, 0x1000, CRC(044ac5d2) SHA1(f2d2fe2236de1b3b2614cc95f61a90571638cd69) )
	ROM_LOAD( "scregg.a14",   0x7000, 0x1000, CRC(b5a0814a) SHA1(192cdc506fb0bbfed8ae687f2699397ace3bef30) )
	ROM_RELOAD(               0xf000, 0x1000 )        /* for reset/interrupt vectors */

	ROM_REGION( 0x6000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "scregg.j12",   0x0000, 0x1000, CRC(a485c10c) SHA1(88edd35479ceb58244f644a7e0520d225df3bf65) )
	ROM_LOAD( "scregg.j10",   0x1000, 0x1000, CRC(1fd4e539) SHA1(3067bbd9493614e80d8d3982fe80ef25688d256c) )
	ROM_LOAD( "scregg.h12",   0x2000, 0x1000, CRC(8454f4b2) SHA1(6a8d257a3fec901453c7216ad894badf96188ebf) )
	ROM_LOAD( "scregg.h10",   0x3000, 0x1000, CRC(72bd89ee) SHA1(2e38c27b546eeef0fe42340777c8687f4c65ee97) )
	ROM_LOAD( "scregg.g12",   0x4000, 0x1000, CRC(ff3c2894) SHA1(0da866db6a79f658de3efc609b9ca8520b4d22d0) )
	ROM_LOAD( "scregg.g10",   0x5000, 0x1000, CRC(9c20214a) SHA1(e01b72501a01ffc0370cf19c9a379a54800cccc6) )

	ROM_REGION( 0x0040, REGION_PROMS, 0 )
	ROM_LOAD( "screggco.c6",  0x0000, 0x0020, CRC(ff23bdd6) SHA1(d09738915da456449bb4e8d9eefb8e6378f0edea) )	/* palette */
	ROM_LOAD( "screggco.b4",  0x0020, 0x0020, CRC(7cc4824b) SHA1(2a283fc17fac32e63385948bfe180d05f1fb8727) )	/* unknown */
ROM_END

ROM_START( eggs )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )     /* 64k for code */
	ROM_LOAD( "e14.bin",      0x3000, 0x1000, CRC(4e216f9d) SHA1(7b9d984481c8227e417dae4a1adbb5dec5f959b7) )
	ROM_LOAD( "d14.bin",      0x4000, 0x1000, CRC(4edb267f) SHA1(f5d1a79b13d6fbb92561b4e4cfb78465114497d1) )
	ROM_LOAD( "c14.bin",      0x5000, 0x1000, CRC(15a5c48c) SHA1(70141c739a8c019554a6c5257ad12606a1542b1f) )
	ROM_LOAD( "b14.bin",      0x6000, 0x1000, CRC(5c11c00e) SHA1(4a9295086bf935a1c9b1b01f83d1ff6242d74907) )
	ROM_LOAD( "a14.bin",      0x7000, 0x1000, CRC(953faf07) SHA1(ee3181e9ee664053d6e6899fe38e136a9ea6abe1) )
	ROM_RELOAD(               0xf000, 0x1000 )   /* for reset/interrupt vectors */

	ROM_REGION( 0x6000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "j12.bin",      0x0000, 0x1000, CRC(ce4a2e46) SHA1(6b31c481ca038834ae295d015054f852baa6330f) )
	ROM_LOAD( "j10.bin",      0x1000, 0x1000, CRC(a1bcaffc) SHA1(74f6df3136826822bbc22b027700fb3ddfceaa97) )
	ROM_LOAD( "h12.bin",      0x2000, 0x1000, CRC(9562836d) SHA1(c5d5d6ceede6105975c87ff8e1f7e5312b992b92) )
	ROM_LOAD( "h10.bin",      0x3000, 0x1000, CRC(3cfb3a8e) SHA1(e60c9da1a7841c3bb5351a109d99c8df34747212) )
	ROM_LOAD( "g12.bin",      0x4000, 0x1000, CRC(679f8af7) SHA1(f69302ff0125d96fbfdd914d7ecefd7130a24616) )
	ROM_LOAD( "g10.bin",      0x5000, 0x1000, CRC(5b58d3b5) SHA1(f138b7294dd20d050bb8a2191e87b0c3815f6148) )

	ROM_REGION( 0x0040, REGION_PROMS, 0 )
	ROM_LOAD( "eggs.c6",      0x0000, 0x0020, CRC(e8408c81) SHA1(549b9948a4a73e7a704731b942565183cef05d52) )	/* palette */
	ROM_LOAD( "screggco.b4",  0x0020, 0x0020, CRC(7cc4824b) SHA1(2a283fc17fac32e63385948bfe180d05f1fb8727) )	/* unknown */
ROM_END


GAME( 198?, dommy,  0,      dommy,  scregg, 0, ROT270, "Technos", "Dommy" )
GAME( 1983, scregg, 0,      scregg, scregg, 0, ROT270, "Technos", "Scrambled Egg" )
GAME( 1983, eggs,   scregg, scregg, scregg, 0, ROT270, "[Technos] Universal USA", "Eggs" )
