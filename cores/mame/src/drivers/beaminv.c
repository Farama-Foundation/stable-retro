/***************************************************************************

	Tekunon Kougyou Beam Invader hardware

	driver by Zsolt Vasvari

	Games supported:
		* Beam Invader

	Known issues:
		* Port 0 might be a analog port select

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "beaminv.h"


/****************************************************************
 *
 *	Special port handler - doesn't warrant its own 'machine file
 *
 ****************************************************************/

static READ_HANDLER( beaminv_input_port_3_r )
{
	return (input_port_3_r(offset) & 0xfe) | ((cpu_getscanline() >> 7) & 0x01);
}


/*************************************
 *
 *	Memory handlers
 *
 *************************************/

static MEMORY_READ_START( readmem )
	{ 0x0000, 0x17ff, MRA_ROM },
	{ 0x1800, 0x1fff, MRA_RAM },
	{ 0x2400, 0x2400, input_port_0_r },
	{ 0x2800, 0x28ff, input_port_1_r },
	{ 0x3400, 0x3400, input_port_2_r },
	{ 0x3800, 0x3800, beaminv_input_port_3_r },
	{ 0x4000, 0x5fff, MRA_RAM },
MEMORY_END

static MEMORY_WRITE_START( writemem )
	{ 0x0000, 0x17ff, MWA_ROM },
	{ 0x1800, 0x1fff, MWA_RAM },
	{ 0x4000, 0x5fff, beaminv_videoram_w, &videoram },
MEMORY_END


/*************************************
 *
 *	Port handlers
 *
 *************************************/

static PORT_WRITE_START( writeport )
PORT_END


/*************************************
 *
 *	Port definitions
 *
 *************************************/

INPUT_PORTS_START( beaminv )
	PORT_START      /* IN0 */
	PORT_DIPNAME( 0x03, 0x00, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x01, "4" )
	PORT_DIPSETTING(    0x02, "5" )
	PORT_DIPSETTING(    0x03, "6" )
	PORT_DIPNAME( 0x0c, 0x04, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x00, "1000" )
	PORT_DIPSETTING(    0x04, "2000" )
	PORT_DIPSETTING(    0x08, "3000" )
	PORT_DIPSETTING(    0x0c, "4000" )
	PORT_DIPNAME( 0x10, 0x00, DEF_STR( Unknown ) )		/* probably unused */
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x60, 0x40, "Faster Bombs At" )
	PORT_DIPSETTING(    0x00, "49 Enemies" )
	PORT_DIPSETTING(    0x20, "39 Enemies" )
	PORT_DIPSETTING(    0x40, "29 Enemies" )
	PORT_DIPSETTING(    0x60, "Never" )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Unknown ) )		/* probably unused */
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0xe0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START      /* IN2 */
	PORT_ANALOG( 0xff, 0x00, IPT_PADDLE, 20, 10, 0x00, 0xff)

	PORT_START      /* IN3 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_SPECIAL )  /* should be V128, using VBLANK slows game down */
	PORT_BIT( 0xfe, IP_ACTIVE_LOW, IPT_UNUSED )

INPUT_PORTS_END


/*************************************
 *
 *	Machine drivers
 *
 *************************************/

static MACHINE_DRIVER_START( beaminv )

	/* basic machine hardware */
	MDRV_CPU_ADD(Z80, 2000000)	/* 2 MHz ? */
	MDRV_CPU_MEMORY(readmem,writemem)
	MDRV_CPU_PORTS(0,writeport)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,2)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(0)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(256, 256)
	MDRV_VISIBLE_AREA(16, 223, 16, 247)
	MDRV_PALETTE_LENGTH(2)
	MDRV_PALETTE_INIT(black_and_white)
	MDRV_VIDEO_START(generic_bitmapped)
	MDRV_VIDEO_UPDATE(generic_bitmapped)
MACHINE_DRIVER_END


/*************************************
 *
 *	ROM definitions
 *
 *************************************/

ROM_START( beaminv )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "0a", 0x0000, 0x0400, CRC(17503086) SHA1(18c789216e5c4330dba3eeb24919dae636bf803d) )
	ROM_LOAD( "1a", 0x0400, 0x0400, CRC(aa9e1666) SHA1(050e2bd169f1502f49b7e6f5f2df9dac0d8107aa) )
	ROM_LOAD( "2a", 0x0800, 0x0400, CRC(ebaa2fc8) SHA1(b4ff1e1bdfe9efdc08873bba2f0a30d24678f9d8) )
	ROM_LOAD( "3a", 0x0c00, 0x0400, CRC(4f62c2e6) SHA1(4bd7d5e4f18d250003c7d771f1cdab08d699a765) )
	ROM_LOAD( "4a", 0x1000, 0x0400, CRC(3eebf757) SHA1(990eebda80ec52b7e3a36912c6e9230cd97f9f25) )
	ROM_LOAD( "5a", 0x1400, 0x0400, CRC(ec08bc1f) SHA1(e1df6704298e470a77158740c275fdca105e8f69) )
ROM_END


/*************************************
 *
 *	Game drivers
 *
 *************************************/

GAMEX(19??, beaminv, 0, beaminv, beaminv, 0, ROT0, "Tekunon Kougyou", "Beam Invader", GAME_NO_SOUND)
