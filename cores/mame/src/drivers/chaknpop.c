/*
 *	Chack'n Pop
 *	(C) 1983 TAITO Corp.
 *
 * driver by BUT
 */

#include "driver.h"
#include "cpu/z80/z80.h"
#include "cpu/m6805/m6805.h"

/* machine/chaknpop.c */
READ_HANDLER( chaknpop_68705_portA_r );
WRITE_HANDLER( chaknpop_68705_portA_w );
READ_HANDLER( chaknpop_68705_portB_r );
WRITE_HANDLER( chaknpop_68705_portB_w );
READ_HANDLER( chaknpop_68705_portC_r );
WRITE_HANDLER( chaknpop_68705_portC_w );
WRITE_HANDLER( chaknpop_68705_ddrA_w );
WRITE_HANDLER( chaknpop_68705_ddrB_w );
WRITE_HANDLER( chaknpop_68705_ddrC_w );
WRITE_HANDLER( chaknpop_mcu_w );
READ_HANDLER( chaknpop_mcu_r );
READ_HANDLER( chaknpop_mcu_status_r );


/* vidhrdw/chaknpop.c */
extern data8_t *chaknpop_txram;
extern data8_t *chaknpop_sprram;
extern size_t chaknpop_sprram_size;
extern data8_t *chaknpop_attrram;


PALETTE_INIT( chaknpop );
VIDEO_START( chaknpop );
VIDEO_UPDATE( chaknpop );

READ_HANDLER( chaknpop_gfxmode_r );
WRITE_HANDLER( chaknpop_gfxmode_w );

WRITE_HANDLER( chaknpop_txram_w );

WRITE_HANDLER( chaknpop_attrram_w );


/***************************************************************************

  Memory Handler(s)

***************************************************************************/

static WRITE_HANDLER ( unknown_port_1_w )
{
	/*logerror("%04x: write to unknow port 1: 0x%02x\n", activecpu_get_pc(), data);*/
}

static WRITE_HANDLER ( unknown_port_2_w )
{
	/*logerror("%04x: write to unknow port 2: 0x%02x\n", activecpu_get_pc(), data);*/
}

static WRITE_HANDLER ( coinlock_w )
{
	log_cb(RETRO_LOG_DEBUG, LOGPRE "%04x: coin lock %sable\n", activecpu_get_pc(), data ? "dis" : "en");
}


/***************************************************************************

  Memory Map(s)

***************************************************************************/

static MEMORY_READ_START( readmem )
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0x87ff, MRA_RAM },
	{ 0x8800, 0x8800, chaknpop_mcu_r },
	{ 0x8801, 0x8801, chaknpop_mcu_status_r },
	{ 0x8802, 0x8802, MRA_NOP },            /* watchdog? */
	{ 0x8805, 0x8805, AY8910_read_port_0_r },
	{ 0x8807, 0x8807, AY8910_read_port_1_r },
	{ 0x8808, 0x8808, input_port_3_r },		/* DSW C*/
	{ 0x8809, 0x8809, input_port_1_r },		/* IN1*/
	{ 0x880a, 0x880a, input_port_0_r },		/* IN0*/
	{ 0x880b, 0x880b, input_port_2_r },		/* IN2*/
	{ 0x880c, 0x880c, chaknpop_gfxmode_r },
	{ 0x9000, 0x93ff, MRA_RAM },			/* TX tilemap*/
	{ 0x9800, 0x983f, MRA_RAM },			/* Color attribute*/
	{ 0x9840, 0x98ff, MRA_RAM },			/* sprite*/
	{ 0xa000, 0xbfff, MRA_ROM },
	{ 0xc000, 0xffff, MRA_BANK1 },			/* bitmap plane 1-4*/
MEMORY_END

static MEMORY_WRITE_START( writemem )
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0x8000, 0x87ff, MWA_RAM },
	{ 0x8800, 0x8800, chaknpop_mcu_w },
	{ 0x8802, 0x8802, MWA_NOP }, 
	{ 0x8804, 0x8804, AY8910_control_port_0_w },
	{ 0x8805, 0x8805, AY8910_write_port_0_w },
	{ 0x8806, 0x8806, AY8910_control_port_1_w },
	{ 0x8807, 0x8807, AY8910_write_port_1_w },
	{ 0x880c, 0x880c, chaknpop_gfxmode_w },
	{ 0x880D, 0x880D, coinlock_w },			/* coin lock out*/
	{ 0x9000, 0x93ff, chaknpop_txram_w, &chaknpop_txram },
	{ 0x9800, 0x983f, chaknpop_attrram_w, &chaknpop_attrram },
	{ 0x9840, 0x98ff, MWA_RAM, &chaknpop_sprram, &chaknpop_sprram_size },
	{ 0xa000, 0xbfff, MWA_ROM },
	{ 0xc000, 0xffff, MWA_BANK1 },			/* bitmap plane 1-4 */
MEMORY_END

static MEMORY_READ_START( chaknpop_m68705_readmem )
	{ 0x0000, 0x0000, chaknpop_68705_portA_r },
	{ 0x0001, 0x0001, chaknpop_68705_portB_r },
	{ 0x0002, 0x0002, chaknpop_68705_portC_r },
	{ 0x0010, 0x007f, MRA_RAM },
	{ 0x0080, 0x07ff, MRA_ROM },
MEMORY_END

static MEMORY_WRITE_START( chaknpop_m68705_writemem )
	{ 0x0000, 0x0000, chaknpop_68705_portA_w },
	{ 0x0001, 0x0001, chaknpop_68705_portB_w },
	{ 0x0002, 0x0002, chaknpop_68705_portC_w },
	{ 0x0004, 0x0004, chaknpop_68705_ddrA_w },
	{ 0x0005, 0x0005, chaknpop_68705_ddrB_w },
	{ 0x0006, 0x0006, chaknpop_68705_ddrC_w },
	{ 0x0010, 0x007f, MWA_RAM },
	{ 0x0080, 0x07ff, MWA_ROM },
MEMORY_END

static struct AY8910interface ay8910_interface =
{
	2,		/* 2 chips */
	18000000 / 12,	/* Verified on PCB */
	{ 15, 10 },
	{ input_port_5_r, 0 },		/* DSW A*/
	{ input_port_4_r, 0 },		/* DSW B*/
	{ 0, unknown_port_1_w },	/* ??*/
	{ 0, unknown_port_2_w }		/* ??*/
};


/***************************************************************************

  Input Port(s)

***************************************************************************/

INPUT_PORTS_START( chaknpop )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN1 )	/* LEFT COIN*/
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN2 )	/* RIGHT COIN*/
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_TILT )

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* IN3: DSW C */
	PORT_DIPNAME( 0x01, 0x01, "Language" )
	PORT_DIPSETTING(    0x00, "English" )
	PORT_DIPSETTING(    0x01, "Japanese" )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, "Super Chack'n" )
	PORT_DIPSETTING(    0x04, "pi" )
	PORT_DIPSETTING(    0x00, "1st Chance" )
	PORT_BITX(    0x08, 0x08, IPT_DIPSWITCH_NAME | IPF_CHEAT, "Endless", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, "Credit Info" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x10, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, "Show Year" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x20, DEF_STR( On ) )
	PORT_BITX(    0x40, 0x40, IPT_DIPSWITCH_NAME | IPF_CHEAT, "Infinite", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x00, "1 Way" )
	PORT_DIPSETTING(    0x80, "2 Way" )

	PORT_START      /* IN4: DSW B */
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x00, "80k and every 100k" )
	PORT_DIPSETTING(    0x01, "60k and every 100k" )
	PORT_DIPSETTING(    0x02, "40k and every 100k" )
	PORT_DIPSETTING(    0x03, "20k and every 100k" )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Free_Play ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x18, 0x08, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "6" )
	PORT_DIPSETTING(    0x08, "3" )
	PORT_DIPSETTING(    0x10, "2" )
	PORT_DIPSETTING(    0x18, "1" )
	PORT_DIPNAME( 0x20, 0x00, "Training/Difficulty" )
	PORT_DIPSETTING(    0x20, "Off/Every 10 Min." )
	PORT_DIPSETTING(    0x00, "On/Every 7 Min." )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Cocktail ) )

	PORT_START      /* IN5: DSW A */
	PORT_DIPNAME(0x0f,  0x00, DEF_STR( Coin_A ) )
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
	PORT_DIPNAME(0xf0,  0x00, DEF_STR( Coin_B ) )
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
INPUT_PORTS_END


/***************************************************************************

  Machine Driver(s)

***************************************************************************/

static struct GfxLayout spritelayout =
{
	16,16,	/* 16*16 characters */
	256,	/* 256 characters */
	2,	/* 2 bits per pixel */
	{ 0, 0x2000*8 },	/* the two bitplanes are separated */
	{ 0, 1, 2, 3, 4, 5, 6, 7 ,
			8*8+0, 8*8+1, 8*8+2, 8*8+3, 8*8+4, 8*8+5, 8*8+6, 8*8+7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
			16*8, 17*8, 18*8, 19*8, 20*8, 21*8, 22*8, 23*8 },
	32*8	/* every sprite takes 32 consecutive bytes */
};

static struct GfxLayout charlayout =
{
	8,8,	/* 8*8 characters */
	1024,	/* 1024 characters */
	2,	/* 2 bits per pixel */
	{ 0, 0x2000*8 },	/* the two bitplanes are separated */
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8	/* every char takes 8 consecutive bytes */
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &spritelayout, 0,  8 },
	{ REGION_GFX2, 0, &charlayout,   32, 8 },
	{ -1 } /* end of array */
};

static MACHINE_DRIVER_START( chaknpop )

	/* basic machine hardware */
	MDRV_CPU_ADD(Z80, 18000000 / 6)	/* Verified on PCB */
	MDRV_CPU_MEMORY(readmem,writemem)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,1)
	
	MDRV_CPU_ADD(M68705, 18000000 / 6)	/* Verified on PCB */
	MDRV_CPU_MEMORY(chaknpop_m68705_readmem,chaknpop_m68705_writemem)

	MDRV_FRAMES_PER_SECOND(59.1828)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(100)	/* 100 CPU slices per frame - an high value to ensure proper */
							/* synchronization of the CPUs */

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 2*8, 30*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(1024)

	MDRV_PALETTE_INIT(chaknpop)
	MDRV_VIDEO_START(chaknpop)
	MDRV_VIDEO_UPDATE(chaknpop)

	/* sound hardware */
	MDRV_SOUND_ADD(AY8910, ay8910_interface)
MACHINE_DRIVER_END


/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( chaknpop )
	ROM_REGION( 0x18000, REGION_CPU1, 0 )	/* Main CPU */
	ROM_LOAD( "a04-01.28",    0x00000, 0x2000, CRC(386fe1c8) SHA1(cca24abfb8a7f439251e7936036475c694002561) )
	ROM_LOAD( "a04-02.27",    0x02000, 0x2000, CRC(5562a6a7) SHA1(0c5d81f9aaf858f88007a6bca7f83dc3ef59c5b5) )
	ROM_LOAD( "a04-03.26",    0x04000, 0x2000, CRC(3e2f0a9c) SHA1(f1cf87a4cb07f77104d4a4d369807dac522e052c) )
	ROM_LOAD( "a04-04.25",    0x06000, 0x2000, CRC(5209c7d4) SHA1(dcba785a697df55d84d65735de38365869a1da9d) )
	ROM_LOAD( "a04-05.3",     0x0a000, 0x2000, CRC(8720e024) SHA1(99e445c117d1501a245f9eb8d014abc4712b4963) )

	ROM_REGION( 0x0800,  REGION_CPU2, 0 )	/* 2k for the microcontroller */
    ROM_LOAD( "ao4_06.ic23", 0x0000, 0x0800, CRC(9c78c24c) SHA1(f74c7f3ee106e5c45c907e590ec09614a2bc6751) )

	ROM_REGION( 0x4000, REGION_GFX1, ROMREGION_DISPOSE )	/* Sprite */
	ROM_LOAD( "a04-08.14",     0x0000, 0x2000, CRC(5575a021) SHA1(c2fad53fe6a12c19cec69d27c13fce6aea2502f2) )
	ROM_LOAD( "a04-07.15",     0x2000, 0x2000, CRC(ae687c18) SHA1(65b25263da88d30cbc0dad94511869596e5c975a) )

	ROM_REGION( 0x4000, REGION_GFX2, ROMREGION_DISPOSE )	/* Text */
	ROM_LOAD( "a04-09.98",     0x0000, 0x2000, CRC(757a723a) SHA1(62ab84d2aaa9bc1ea5aa9df8155aa3b5a1e93889) )
	ROM_LOAD( "a04-10.97",     0x2000, 0x2000, CRC(3e3fd608) SHA1(053a8fbdb35bf1c142349f78a63e8cd1adb41ef6) )

	ROM_REGION( 0x0800, REGION_PROMS, 0 )			/* Palette */
	ROM_LOAD( "a04-11.bin",    0x0000, 0x0400, CRC(9bf0e85f) SHA1(44f0a4712c99a715dec54060afb0b27dc48998b4) )
	ROM_LOAD( "a04-12.bin",    0x0400, 0x0400, CRC(954ce8fc) SHA1(e187f9e2cb754264d149c2896ca949dea3bcf2eb) )
ROM_END


/*  ( YEAR  NAME      PARENT    MACHINE   INPUT     INIT      MONITOR  COMPANY              FULLNAME ) */
GAME( 1983, chaknpop, 0,        chaknpop, chaknpop, 0,        ROT0,    "Taito Corporation", "Chack'n Pop")
