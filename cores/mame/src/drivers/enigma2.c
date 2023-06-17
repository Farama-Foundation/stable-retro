/********************************************************************

Enigma 2 (c) Zilec Electronics

driver by Pierpaolo Prazzoli and Tomasz Slanina

Two sets:

Enigma2 (1981)
 2xZ80 + AY8910

Enigma2a (1984?)
 Conversion applied to a Taito Space Invaders Part II board set. Bootleg ?

TODO:
 	- Add sound once there is a good dump of the sound program
 	- What is the other Prom used for?
 	- Why does it write into ROM area?
*********************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

static int cmap;

WRITE_HANDLER( enigma2_videoram_w )
{
	if (videoram[offset] != data)
	{
		int i,x,y,col;

		videoram[offset] = data;

		y = offset / 32;
		col = 8 * (offset % 32);
		x = 255 - col;

		for (i = 0; i < 8; i++)
		{
			if( data & 0x80 )
				plot_pixel(tmpbitmap, x, 255 - y, cmap?(memory_region(REGION_PROMS)[((y+16) >> 3 << 5) | ((col+i) >> 3)] & 0x07):Machine->pens[7]);
			else
				plot_pixel(tmpbitmap, x, 255 - y, Machine->pens[0]);
			x++;
			data <<= 1;
		}
	}
}

static READ_HANDLER( fake_r )
{
	static int cnt;
	/* HACK! to get dip-switches working, since they are read by the sound board
							  enigma						 enigma2a */
	if( activecpu_get_pc() == 0x156 || activecpu_get_pc() == 0x284 )
	{
		cnt = 0;
		return readinputport(2);
	}
	else
	{
		cnt ^= 1;
		return cnt ? 0x7d : 0xf4; /*expected values*/
	}
}

static READ_HANDLER( fake_r2 )
{
	if( activecpu_get_pc() == 0x7e5 ) /* needed by enigma2a*/
		return 0xaa;
	else
		return 0xf4;
}

static READ_HANDLER( fake_r3 )
{
	return 0x38;
}

static MEMORY_READ_START( readmem )
	{ 0x0000, 0x1fff, MRA_ROM },
	{ 0x2000, 0x21ff, MRA_RAM },
  { 0x2200, 0x3fff, videoram_r },
	{ 0x4000, 0x4fff, MRA_ROM },
	{ 0x5001, 0x5001, fake_r }, /* sub cpu communication */
	{ 0x5002, 0x5002, fake_r2 },
	{ 0x5035, 0x5035, fake_r3 },		/* only enigma2a (pc:1282) */
	{ 0x5801, 0x5801, input_port_0_r }, /* only enigma2a, used instead of ports */
	{ 0x5802, 0x5802, input_port_1_r }, /* only enigma2a, used instead of ports */
MEMORY_END

static MEMORY_WRITE_START( writemem )
	{ 0x0000, 0x1fff, MWA_ROM },
	{ 0x2000, 0x21ff, MWA_RAM },
	{ 0x2200, 0x3fff, enigma2_videoram_w, &videoram },
	{ 0x4000, 0x4fff, MWA_ROM },
	{ 0x5015, 0x53fb, MWA_RAM }, /* every 0x20 */
	{ 0x5415, 0x541b, MWA_RAM }, /* always zero ? */
	{ 0x5803, 0x5803, MWA_RAM }, /* only enigma2a, used instead of ports */
	{ 0x5805, 0x5805, MWA_RAM }, /* only enigma2a, used instead of ports */
	{ 0x5806, 0x5806, MWA_RAM }, /* only enigma2a, used instead of ports */
MEMORY_END

static PORT_READ_START( readport )
	{ 0x01, 0x01, input_port_0_r },
	{ 0x02, 0x02, input_port_1_r },
PORT_END

static PORT_WRITE_START( writeport )
	{ 0x03, 0x03, MWA_NOP },
	{ 0x05, 0x05, MWA_NOP },
	{ 0x06, 0x06, MWA_NOP },
PORT_END

INPUT_PORTS_START( enigma2a )
	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT   | IPF_2WAY | IPF_PLAYER1 )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT  | IPF_2WAY | IPF_PLAYER1 )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT   | IPF_2WAY | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT  | IPF_2WAY | IPF_PLAYER2 )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START
	PORT_DIPNAME( 0x03, 0x00, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x01, "4" )
	PORT_DIPSETTING(    0x02, "5" )
	PORT_DIPSETTING(    0x03, "6" )
	PORT_DIPNAME( 0x1c, 0x00, "Skill level" )
	PORT_DIPSETTING(    0x00, "1" )
	PORT_DIPSETTING(    0x04, "2" )
	PORT_DIPSETTING(    0x0c, "3" )
	PORT_DIPSETTING(    0x08, "4" )
	PORT_DIPSETTING(    0x10, "5" )
	PORT_DIPSETTING(    0x14, "6" )
/*	PORT_DIPSETTING(    0x18, "?" )
	PORT_DIPSETTING(    0x1c, "?" ) */
	PORT_DIPNAME( 0x20, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x20, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
INPUT_PORTS_END

INPUT_PORTS_START( enigma2 )
	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT   | IPF_2WAY | IPF_PLAYER1 )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT  | IPF_2WAY | IPF_PLAYER1 )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT   | IPF_2WAY | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT  | IPF_2WAY | IPF_PLAYER2 )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START
	PORT_DIPNAME( 0x03, 0x00, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x01, "4" )
	PORT_DIPSETTING(    0x02, "5" )
	PORT_DIPSETTING(    0x03, "6" )
	PORT_DIPNAME( 0x1c, 0x00, "Skill level" ) /* need check */
	PORT_DIPSETTING(    0x00, "1" )
	PORT_DIPSETTING(    0x04, "2" )
	PORT_DIPSETTING(    0x0c, "3" )
	PORT_DIPSETTING(    0x08, "4" )
	PORT_DIPSETTING(    0x10, "5" )
	PORT_DIPSETTING(    0x14, "6" )
/*	PORT_DIPSETTING(    0x18, "?" )
	PORT_DIPSETTING(    0x1c, "?" )*/
	PORT_DIPNAME( 0x20, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x20, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, "Number of invaders" )
	PORT_DIPSETTING(    0x40, "16" )
	PORT_DIPSETTING(    0x00, "32" )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
INPUT_PORTS_END

INTERRUPT_GEN( enigma2_interrupt )
{
	int vector = cpu_getvblank() ? 0xcf : 0xd7;
    cpu_set_irq_line_and_vector(0, 0, HOLD_LINE, vector);
}

PALETTE_INIT( enigma2 )
{
/*
	Washed colors were hand-tuned from :
	http://www.arcadeflyers.com/?page=flyerdb&subpage=thumbs&id=1756&PHPSESSID=9c8361a00f26b15f9b49bdd7cca9d47f
*/
	palette_set_color(0,0,0,0);
	palette_set_color(1,85,113,178);
	palette_set_color(2,225,198,83);
	palette_set_color(3,194,218,156);
	palette_set_color(4,234,57,47);
	palette_set_color(5,238,187,186);
	/* 6 */
	palette_set_color(7,0xff,0xff,0xff);
}

static MACHINE_DRIVER_START( enigma2 )
	MDRV_CPU_ADD_TAG("main",Z80, 2500000)
	MDRV_CPU_MEMORY(readmem,writemem)
	MDRV_CPU_PORTS(readport,writeport)
	MDRV_CPU_VBLANK_INT(enigma2_interrupt,2)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER )
	MDRV_SCREEN_SIZE(256, 256)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 2*8, 30*8-1)
	MDRV_PALETTE_LENGTH(8)
	MDRV_PALETTE_INIT(enigma2)

	MDRV_VIDEO_START(generic_bitmapped)
	MDRV_VIDEO_UPDATE(generic_bitmapped)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( enigma2a )
	MDRV_IMPORT_FROM(enigma2)
	MDRV_CPU_REPLACE("main", 8080, 2000000)
MACHINE_DRIVER_END

ROM_START( enigma2a )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "36_en1.bin",   0x0000, 0x0800, CRC(15f44806) SHA1(4a2f7bc91d4edf7a069e0865d964371c97af0a0a) )
	ROM_LOAD( "35_en2.bin",   0x0800, 0x0800, CRC(e841072f) SHA1(6ab02fd9fdeac5ab887cd25eee3d6b70ab01f849) )
	ROM_LOAD( "34_en3.bin",   0x1000, 0x0800, CRC(43d06cf4) SHA1(495af05d54c0325efb67347f691e64d194645d85) )
	ROM_LOAD( "33_en4.bin",   0x1800, 0x0800, CRC(8879a430) SHA1(c97f44bef3741eef74e137d2459e79f1b3a90457) )
	ROM_LOAD( "5.11d",        0x4000, 0x0800, CRC(098ac15b) SHA1(cce28a2540a9eabb473391fff92895129ae41751) )
	ROM_LOAD( "6.13d",   	  0x4800, 0x0800, CRC(240a9d4b) SHA1(ca1c69fafec0471141ce1254ddfaef54fecfcbf0) )

	/* the length of the correct rom should be 0x1000 */
	ROM_REGION( 0x10000, REGION_CPU2, 0 )
	ROM_LOAD( "sound.bin",    0x0000, 0x0800, BAD_DUMP CRC(5f092d3c) SHA1(17c70f6af1b5560a45e6b1bdb330a98b27570fe9) )

ROM_END

ROM_START( enigma2 )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "1.5d",         0x0000, 0x0800, CRC(499749de) SHA1(401928ff41d3b4cbb68e6ad3bf3be4a10ae1781f) )
	ROM_LOAD( "2.7d",         0x0800, 0x0800, CRC(173c1329) SHA1(3f1ad46d0e58ab236e4ff2b385d09fbf113627da) )
	ROM_LOAD( "3.8d",         0x1000, 0x0800, CRC(c7d3e6b1) SHA1(43f7c3a02b46747998260d5469248f21714fe12b) )
	ROM_LOAD( "4.10d",        0x1800, 0x0800, CRC(c6a7428c) SHA1(3503f09856655c5973fb89f60d1045fe41012aa9) )
	ROM_LOAD( "5.11d",   	  0x4000, 0x0800, CRC(098ac15b) SHA1(cce28a2540a9eabb473391fff92895129ae41751) )
	ROM_LOAD( "6.13d",   	  0x4800, 0x0800, CRC(240a9d4b) SHA1(ca1c69fafec0471141ce1254ddfaef54fecfcbf0) )

	/* the length of the correct rom should be 0x1000*/
	ROM_REGION( 0x10000, REGION_CPU2, 0 )
	ROM_LOAD( "s.2f",         0x0000, 0x0800, BAD_DUMP CRC(9bceb714) SHA1(c3d9301cc93d073d7b2694346c26eddd36f94aae) )

 /* Color Map */
	ROM_REGION( 0x800, REGION_PROMS, 0 )
	ROM_LOAD( "7.11f",        0x0000, 0x0800, CRC(409b5aad) SHA1(1b774a70f725637458ed68df9ed42476291b0e43) )

	/* Unknown */
	ROM_REGION( 0x10000, REGION_USER1, 0 )
	ROM_LOAD( "8.13f",        0x0000, 0x0800, CRC(e9cb116d) SHA1(41da4f46c5614ec3345c233467ebad022c6b0bf5) )
ROM_END

static DRIVER_INIT(enigma2) {	cmap=1;}
static DRIVER_INIT(enigma2a){	cmap=0;}

GAMEX( 1981, enigma2,  0,		enigma2, enigma2,  enigma2, ROT90, "GamePlan (Zilec Electronics license)", "Enigma 2", GAME_NO_SOUND | GAME_WRONG_COLORS )
GAMEX( 1984, enigma2a, enigma2, enigma2a, enigma2a, enigma2a, ROT90, "Zilec Electronics", "Enigma 2 (Space Invaders Hardware)", GAME_NO_SOUND | GAME_WRONG_COLORS )
