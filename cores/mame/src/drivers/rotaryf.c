/* Rotary Fighter

driver by Barry Rodewald
 based on Initial work by David Haywood

 todo:

 sound
 verify game speed if possible (related to # of interrupts)

*/

#include "driver.h"
#include "8080bw.h"
#include "vidhrdw/generic.h"
#include "cpu/i8085/i8085.h"


static PALETTE_INIT( rotaryf )
{
	palette_set_color(0,0x00,0x00,0x00); /* black */
	palette_set_color(1,0xff,0xff,0xff); /* white */
}

INTERRUPT_GEN( rotaryf_interrupt )
{
	if(cpu_getvblank())
		cpu_set_irq_line(0,I8085_RST55_LINE,HOLD_LINE);
	else
		cpu_set_irq_line(0,I8085_RST75_LINE,HOLD_LINE);

}

static MEMORY_READ_START( rotaryf_readmem )
	{ 0x0000, 0x03ff, MRA_ROM },
	{ 0x4000, 0x57ff, MRA_ROM },
/*	{ 0x6ffb, 0x6ffb, random_r }, ??*/
/*	{ 0x6ffd, 0x6ffd, random_r }, ??*/
/*	{ 0x6fff, 0x6fff, random_r }, ??*/
	{ 0x7000, 0x73ff, MRA_RAM },
	{ 0x8000, 0x9fff, MRA_RAM },
	{ 0xa000, 0xa1ff, MRA_RAM },
MEMORY_END

static MEMORY_WRITE_START( rotaryf_writemem )
	{ 0x0000, 0x03ff, MWA_ROM },
	{ 0x4000, 0x57ff, MWA_ROM },
	{ 0x7000, 0x73ff, MWA_RAM }, /* clears to 1ff ?*/
	{ 0x8000, 0x9fff, c8080bw_videoram_w, &videoram, &videoram_size },
	{ 0xa000, 0xa1ff, MWA_RAM },
MEMORY_END

static PORT_READ_START( rotaryf_readport )
/*	{ 0x00, 0x00, input_port_0_r },*/
	{ 0x21, 0x21, input_port_1_r },
	{ 0x29, 0x29, input_port_2_r },
	{ 0x26, 0x26, input_port_3_r },
/*	{ 0x28, 0x28, c8080bw_shift_data_r },*/
PORT_END

static PORT_WRITE_START( rotaryf_writeport )
/*	{ 0x21, 0x21, c8080bw_shift_amount_w },*/
/*	{ 0x28, 0x28, c8080bw_shift_data_w },*/
PORT_END

INPUT_PORTS_START( rotaryf )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START      /* IN1 */
	PORT_BIT_IMPULSE( 0x20, IP_ACTIVE_HIGH, IPT_COIN1, 1 )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_2WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_2WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_2WAY | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_2WAY | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START2 )

	PORT_START      /* DSW0 */
	PORT_DIPNAME( 0x81, 0x00, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x81, "3" )
	PORT_DIPSETTING(    0x01, "4" )
	PORT_DIPSETTING(    0x80, "5" )
	PORT_DIPSETTING(    0x00, "6" )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING( 0x00, DEF_STR( Off ) )
	PORT_DIPSETTING( 0x02, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x00, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING( 0x04, "1000" )
	PORT_DIPSETTING( 0x00, "1500" )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPNAME( 0x10, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING( 0x00, DEF_STR( Off ) )
	PORT_DIPSETTING( 0x10, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING( 0x00, DEF_STR( Off ) )
	PORT_DIPSETTING( 0x20, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING( 0x00, DEF_STR( Off ) )
	PORT_DIPSETTING( 0x40, DEF_STR( On ) )
/*	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_PLAYER2 )*/
/*	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_2WAY | IPF_PLAYER2 )*/
/*	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_2WAY | IPF_PLAYER2 )*/

	PORT_START		/* Dummy port for cocktail mode */
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Cocktail ) )
INPUT_PORTS_END


static MACHINE_DRIVER_START( rotaryf )

	/* basic machine hardware */
	MDRV_CPU_ADD_TAG("main",8085A,2000000) /* 8080? */ /* 2 MHz? */
	MDRV_CPU_MEMORY(rotaryf_readmem,rotaryf_writemem)
	MDRV_CPU_PORTS(rotaryf_readport,rotaryf_writeport)
	MDRV_CPU_VBLANK_INT(rotaryf_interrupt,5)
	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 0*8, 32*8-1)
	MDRV_PALETTE_LENGTH(2)
	MDRV_PALETTE_INIT(rotaryf)
	MDRV_VIDEO_START(generic_bitmapped)
	MDRV_VIDEO_UPDATE(8080bw)

	/* sound hardware */
	MDRV_SOUND_ADD(SAMPLES, invaders_samples_interface)
	MDRV_SOUND_ADD(SN76477, invaders_sn76477_interface)

MACHINE_DRIVER_END

ROM_START( rotaryf )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )     /* 64k for code */
	ROM_LOAD( "krf-1.bin",            0x0000, 0x0400, CRC(f7b2d3e6) SHA1(be7afc1a14be60cb895fc4180167353c7156fc4c) )
	ROM_RELOAD (                      0x4000, 0x0400             )
	ROM_LOAD( "krf-2.bin",			  0x4400, 0x0400, CRC(be9f047a) SHA1(e5dd2b5b4fda7f178e7f1137592ba49fbc9cc82e) )
	ROM_LOAD( "krf-3.bin",            0x4800, 0x0400, CRC(c7629eb6) SHA1(03aae964783ce4b1de77737e83fd2094483fbda4) )
	ROM_LOAD( "krf-4.bin",            0x4c00, 0x0400, CRC(b4703093) SHA1(9239d6da818049bc98a631c3bf5b962b5df5b2ea) )
	ROM_LOAD( "krf-5.bin",            0x5000, 0x0400, CRC(ae233f07) SHA1(a7bbd2ee4477ee041d170e2fc4e94c99c3b564fc) )
	ROM_LOAD( "krf-6.bin",            0x5400, 0x0400, CRC(e28b3713) SHA1(428f73891125f80c722357f1029b18fa9416bcfd) )
ROM_END

GAMEX(19??, rotaryf, 0,        rotaryf, rotaryf, 8080bw,	ROT270,   "<unknown>", "Rotary Fighter", GAME_NO_SOUND )
