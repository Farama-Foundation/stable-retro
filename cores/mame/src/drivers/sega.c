/***************************************************************************

	Sega vector hardware

	Games supported:
		* Space Fury
		* Zektor
		* Tac/Scan
		* Eliminator
		* Star Trek

	Known bugs:
		* none at this time

****************************************************************************

	4/25/99 - Tac-Scan sound call for coins now works. (Jim Hernandez)
	2/5/98 - Added input ports support for Tac Scan. Bonus Ships now work.
			 Zektor now uses it's own input port section. (Jim Hernandez)

	Sega Vector memory map (preliminary)

	Most of the info here comes from the wiretap archive at:
	http://www.spies.com/arcade/simulation/gameHardware/

	 * Sega G80 Vector Simulation

	ROM Address Map
	---------------
		   Eliminator Elim4Player Space Fury  Zektor  TAC/SCAN	Star Trk
	-----+-----------+-----------+-----------+-------+---------+---------+
	0000 | 969		 | 1390 	 | 969		 | 1611  | 1711    | 1873	 | CPU u25
	-----+-----------+-----------+-----------+-------+---------+---------+
	0800 | 1333 	 | 1347 	 | 960		 | 1586  | 1670    | 1848	 | ROM u1
	-----+-----------+-----------+-----------+-------+---------+---------+
	1000 | 1334 	 | 1348 	 | 961		 | 1587  | 1671    | 1849	 | ROM u2
	-----+-----------+-----------+-----------+-------+---------+---------+
	1800 | 1335 	 | 1349 	 | 962		 | 1588  | 1672    | 1850	 | ROM u3
	-----+-----------+-----------+-----------+-------+---------+---------+
	2000 | 1336 	 | 1350 	 | 963		 | 1589  | 1673    | 1851	 | ROM u4
	-----+-----------+-----------+-----------+-------+---------+---------+
	2800 | 1337 	 | 1351 	 | 964		 | 1590  | 1674    | 1852	 | ROM u5
	-----+-----------+-----------+-----------+-------+---------+---------+
	3000 | 1338 	 | 1352 	 | 965		 | 1591  | 1675    | 1853	 | ROM u6
	-----+-----------+-----------+-----------+-------+---------+---------+
	3800 | 1339 	 | 1353 	 | 966		 | 1592  | 1676    | 1854	 | ROM u7
	-----+-----------+-----------+-----------+-------+---------+---------+
	4000 | 1340 	 | 1354 	 | 967		 | 1593  | 1677    | 1855	 | ROM u8
	-----+-----------+-----------+-----------+-------+---------+---------+
	4800 | 1341 	 | 1355 	 | 968		 | 1594  | 1678    | 1856	 | ROM u9
	-----+-----------+-----------+-----------+-------+---------+---------+
	5000 | 1342 	 | 1356 	 |			 | 1595  | 1679    | 1857	 | ROM u10
	-----+-----------+-----------+-----------+-------+---------+---------+
	5800 | 1343 	 | 1357 	 |			 | 1596  | 1680    | 1858	 | ROM u11
	-----+-----------+-----------+-----------+-------+---------+---------+
	6000 | 1344 	 | 1358 	 |			 | 1597  | 1681    | 1859	 | ROM u12
	-----+-----------+-----------+-----------+-------+---------+---------+
	6800 | 1345 	 | 1359 	 |			 | 1598  | 1682    | 1860	 | ROM u13
	-----+-----------+-----------+-----------+-------+---------+---------+
	7000 |			 | 1360 	 |			 | 1599  | 1683    | 1861	 | ROM u14
	-----+-----------+-----------+-----------+-------+---------+---------+
	7800 |									 | 1600  | 1684    | 1862	 | ROM u15
	-----+-----------+-----------+-----------+-------+---------+---------+
	8000 |									 | 1601  | 1685    | 1863	 | ROM u16
	-----+-----------+-----------+-----------+-------+---------+---------+
	8800 |									 | 1602  | 1686    | 1864	 | ROM u17
	-----+-----------+-----------+-----------+-------+---------+---------+
	9000 |									 | 1603  | 1687    | 1865	 | ROM u18
	-----+-----------+-----------+-----------+-------+---------+---------+
	9800 |									 | 1604  | 1688    | 1866	 | ROM u19
	-----+-----------+-----------+-----------+-------+---------+---------+
	A000 |									 | 1605  | 1709    | 1867	 | ROM u20
	-----+-----------+-----------+-----------+-------+---------+---------+
	A800 |									 | 1606  | 1710    | 1868	 | ROM u21
	-----+-----------+-----------+-----------+-------+---------+---------+
	B000 |													   | 1869	 | ROM u22
	-----+-----------+-----------+-----------+-------+---------+---------+
	B800 |													   | 1870	 | ROM u23
	-----+-----------+-----------+-----------+-------+---------+---------+

	I/O ports:
	read:

	write:

	These games all have dipswitches, but they are mapped in such a way as to make
	using them with MAME extremely difficult. I might try to implement them in the
	future.

	SWITCH MAPPINGS
	---------------

	+------+------+------+------+------+------+------+------+
	|SW1-8 |SW1-7 |SW1-6 |SW1-5 |SW1-4 |SW1-3 |SW1-2 |SW1-1 |
	+------+------+------+------+------+------+------+------+
	 F8:08 |F9:08 |FA:08 |FB:08 |F8:04 |F9:04  FA:04  FB:04    Zektor &
		   |	  | 	 |		|	   |	  | 			   Space Fury
		   |	  | 	 |		|	   |	  |
	   1  -|------|------|------|------|------|--------------- upright
	   0  -|------|------|------|------|------|--------------- cocktail
		   |	  | 	 |		|	   |	  |
		   |  1  -|------|------|------|------|--------------- voice
		   |  0  -|------|------|------|------|--------------- no voice
				  | 	 |		|	   |	  |
				  |  1	 |	1  -|------|------|--------------- 5 ships
				  |  0	 |	1  -|------|------|--------------- 4 ships
				  |  1	 |	0  -|------|------|--------------- 3 ships
				  |  0	 |	0  -|------|------|--------------- 2 ships
								|	   |	  |
								|  1   |  1  -|--------------- hardest
								|  0   |  1  -|--------------- hard
	1 = Open					|  1   |  0  -|--------------- medium
	0 = Closed					|  0   |  0  -|--------------- easy

	+------+------+------+------+------+------+------+------+
	|SW2-8 |SW2-7 |SW2-6 |SW2-5 |SW2-4 |SW2-3 |SW2-2 |SW2-1 |
	+------+------+------+------+------+------+------+------+
	|F8:02 |F9:02 |FA:02 |FB:02 |F8:01 |F9:01 |FA:01 |FB:01 |
	|	   |	  | 	 |		|	   |	  | 	 |		|
	|  1   |  1   |  0	 |	0	|  1   | 1	  | 0	 |	0	| 1 coin/ 1 play
	+------+------+------+------+------+------+------+------+

	Known problems:

	1 The games seem to run too fast. This is most noticable
	  with the speech samples in Zektor - they don't match the mouth.
	  Slowing down the Z80 doesn't help and in fact hurts performance.

	2 Cocktail mode isn't implemented.

	Is 1) still valid?

***************************************************************************/

#include "driver.h"
#include "vidhrdw/vector.h"
#include "sndhrdw/segasnd.h"
#include "sega.h"



/*************************************
 *
 *	Main CPU memory handlers
 *
 *************************************/

static MEMORY_READ_START( readmem )
	{ 0x0000, 0xbfff, MRA_ROM },
	{ 0xc800, 0xcfff, MRA_RAM },
	{ 0xe000, 0xefff, MRA_RAM },
	{ 0xd000, 0xdfff, MRA_RAM },			/* sound ram */
MEMORY_END


static MEMORY_WRITE_START( writemem )
	{ 0x0000, 0xffff, sega_w, &sega_mem },
	{ 0xe000, 0xefff, MWA_RAM, &vectorram, &vectorram_size },	/* handled by the above, */
												/* here only to initialize the pointer */
MEMORY_END

static READ_HANDLER( sega_sh_r )
{
	/* 0x80 = universal sound board ready */
	/* 0x01 = speech ready, theorically, but the schematics show it unconnected */

	return 0x80;
}

static PORT_READ_START( readport )
	{ 0x3f, 0x3f, sega_sh_r },
	{ 0xbe, 0xbe, sega_mult_r },
	{ 0xf8, 0xfb, sega_ports_r },
PORT_END


static PORT_WRITE_START( writeport )
	{ 0xbd, 0xbd, sega_mult1_w },
	{ 0xbe, 0xbe, sega_mult2_w },
	{ 0xf8, 0xf8, sega_switch_w },
	{ 0xf9, 0xf9, sega_coin_counter_w }, /* 0x80 = enable, 0x00 = disable */
PORT_END




static MEMORY_READ_START( speech_readmem )
	{ 0x0000, 0x07ff, MRA_ROM },
MEMORY_END


static MEMORY_WRITE_START( speech_writemem )
	{ 0x0000, 0x07ff, MWA_ROM },
MEMORY_END


/*************************************
 *
 *	Port definitions
 *
 *************************************/

/* This fake input port is used for DIP Switch 2
   for all games except Eliminato 4 players */
#define COINAGE PORT_START \
		PORT_DIPNAME( 0x0f, 0x0c, DEF_STR ( Coin_B ) ) \
		PORT_DIPSETTING(	0x00, DEF_STR ( 4C_1C ) ) \
		PORT_DIPSETTING(	0x08, DEF_STR ( 3C_1C ) ) \
		PORT_DIPSETTING(	0x09, "2 Coins/1 Credit 5/3 6/4" ) \
		PORT_DIPSETTING(	0x05, "2 Coins/1 Credit 4/3" ) \
		PORT_DIPSETTING(	0x04, DEF_STR ( 2C_1C ) ) \
		PORT_DIPSETTING(	0x0c, DEF_STR ( 1C_1C ) ) \
		PORT_DIPSETTING(	0x0d, "1 Coin/1 Credit 5/6" ) \
		PORT_DIPSETTING(	0x03, "1 Coin/1 Credit 4/5" ) \
		PORT_DIPSETTING(	0x0b, "1 Coin/1 Credit 2/3" ) \
		PORT_DIPSETTING(	0x02, DEF_STR ( 1C_2C ) ) \
		PORT_DIPSETTING(	0x0f, "1 Coin/2 Credits 4/9" ) \
		PORT_DIPSETTING(	0x07, "1 Coin/2 Credits 5/11" ) \
		PORT_DIPSETTING(	0x0a, DEF_STR ( 1C_3C ) ) \
		PORT_DIPSETTING(	0x06, DEF_STR ( 1C_4C ) ) \
		PORT_DIPSETTING(	0x0e, DEF_STR ( 1C_5C ) ) \
		PORT_DIPSETTING(	0x01, DEF_STR ( 1C_6C ) ) \
		PORT_DIPNAME( 0xf0, 0xc0, DEF_STR ( Coin_A ) ) \
		PORT_DIPSETTING(	0x00, DEF_STR ( 4C_1C ) ) \
		PORT_DIPSETTING(	0x80, DEF_STR ( 3C_1C ) ) \
		PORT_DIPSETTING(	0x90, "2 Coins/1 Credit 5/3 6/4" ) \
		PORT_DIPSETTING(	0x50, "2 Coins/1 Credit 4/3" ) \
		PORT_DIPSETTING(	0x40, DEF_STR ( 2C_1C ) ) \
		PORT_DIPSETTING(	0xc0, DEF_STR ( 1C_1C ) ) \
		PORT_DIPSETTING(	0xd0, "1 Coin/1 Credit 5/6" ) \
		PORT_DIPSETTING(	0x30, "1 Coin/1 Credit 4/5" ) \
		PORT_DIPSETTING(	0xb0, "1 Coin/1 Credit 2/3" ) \
		PORT_DIPSETTING(	0x20, DEF_STR ( 1C_2C ) ) \
		PORT_DIPSETTING(	0xf0, "1 Coin/2 Credits 4/9" ) \
		PORT_DIPSETTING(	0x70, "1 Coin/2 Credits 5/11" ) \
		PORT_DIPSETTING(	0xa0, DEF_STR ( 1C_3C ) ) \
		PORT_DIPSETTING(	0x60, DEF_STR ( 1C_4C ) ) \
		PORT_DIPSETTING(	0xe0, DEF_STR ( 1C_5C ) ) \
		PORT_DIPSETTING(	0x10, DEF_STR ( 1C_6C ) )


INPUT_PORTS_START( spacfury )
	PORT_START	/* IN0 - port 0xf8 */
	/* The next bit is referred to as the Service switch in the self test - it just adds a credit */
	PORT_BIT_IMPULSE( 0x20, IP_ACTIVE_LOW, IPT_COIN3, 3 )
	PORT_BIT_IMPULSE( 0x40, IP_ACTIVE_LOW, IPT_COIN2, 3 )
	PORT_BIT_IMPULSE( 0x80, IP_ACTIVE_LOW, IPT_COIN1, 3 )

	PORT_START	/* IN1 - port 0xf9 */
	PORT_BIT ( 0x10, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT ( 0x20, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT ( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* IN2 - port 0xfa */
	PORT_BIT ( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT )
	PORT_BIT ( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT )
	PORT_BIT ( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* IN3 - port 0xfb */
	PORT_BIT ( 0x10, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT ( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT ( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* IN4 - FAKE - lazy way to move the self-test fake input port to 5 */
	PORT_BIT ( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* IN5 - FAKE */
	/* This fake input port is used to get the status of the F2 key, */
	/* and activate the test mode, which is triggered by a NMI */
	PORT_BITX(0x01, IP_ACTIVE_HIGH, IPT_SERVICE, DEF_STR( Service_Mode ), KEYCODE_F2, IP_JOY_NONE )

	PORT_START	/* FAKE */
		/* This fake input port is used for DIP Switch 1 */
		PORT_DIPNAME( 0x03, 0x01, DEF_STR ( Bonus_Life ) )
		PORT_DIPSETTING(	0x00, "10000" )
		PORT_DIPSETTING(	0x02, "20000" )
		PORT_DIPSETTING(	0x01, "30000" )
		PORT_DIPSETTING(	0x03, "40000" )
		PORT_DIPNAME( 0x0c, 0x00, DEF_STR ( Difficulty ) )
		PORT_DIPSETTING(	0x00, "Easy" )
		PORT_DIPSETTING(	0x08, "Normal" )
		PORT_DIPSETTING(	0x04, "Hard" )
		PORT_DIPSETTING(	0x0c, "Very Hard" )
		PORT_DIPNAME( 0x30, 0x30, DEF_STR ( Lives ) )
		PORT_DIPSETTING(	0x00, "2" )
		PORT_DIPSETTING(	0x20, "3" )
		PORT_DIPSETTING(	0x10, "4" )
		PORT_DIPSETTING(	0x30, "5" )
		PORT_DIPNAME( 0x40, 0x00, DEF_STR ( Demo_Sounds) )
		PORT_DIPSETTING(	0x40, DEF_STR ( Off ) )
		PORT_DIPSETTING(	0x00, DEF_STR ( On ) )
		PORT_DIPNAME( 0x80, 0x80, DEF_STR ( Cabinet ) )
		PORT_DIPSETTING(	0x80, DEF_STR ( Upright ) )
		PORT_DIPSETTING(	0x00, DEF_STR ( Cocktail ) )

		COINAGE

	PORT_START	/* IN8 - port 0xfc */
	PORT_BIT ( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT ( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT ( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_PLAYER2 )
	PORT_BIT ( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER2 )
INPUT_PORTS_END


INPUT_PORTS_START( zektor )
	PORT_START	/* IN0 - port 0xf8 */
	/* The next bit is referred to as the Service switch in the self test - it just adds a credit */
	PORT_BIT_IMPULSE( 0x20, IP_ACTIVE_LOW, IPT_COIN3, 3 )
	PORT_BIT_IMPULSE( 0x40, IP_ACTIVE_LOW, IPT_COIN2, 3 )
	PORT_BIT_IMPULSE( 0x80, IP_ACTIVE_LOW, IPT_COIN1, 3 )

	PORT_START	/* IN1 - port 0xf9 */
	PORT_BIT ( 0xf0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* IN2 - port 0xfa */
	PORT_BIT ( 0xf0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* IN3 - port 0xfb */
	PORT_BIT ( 0xf0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* IN4 - port 0xfc - read in machine/sega.c */
	PORT_BIT ( 0x01, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT ( 0x02, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT ( 0x04, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT ( 0x08, IP_ACTIVE_HIGH, IPT_BUTTON2 )
	PORT_BIT ( 0xf0, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START	/* IN5 - FAKE */
	/* This fake input port is used to get the status of the F2 key, */
	/* and activate the test mode, which is triggered by a NMI */
	PORT_BITX(0x01, IP_ACTIVE_HIGH, IPT_SERVICE, DEF_STR( Service_Mode ), KEYCODE_F2, IP_JOY_NONE )

	PORT_START	/* FAKE */
	/* This fake input port is used for DIP Switch 1 */
	PORT_DIPNAME( 0x03, 0x01, DEF_STR ( Bonus_Life ) )
	PORT_DIPSETTING(	0x03, "10000" )
	PORT_DIPSETTING(	0x01, "20000" )
	PORT_DIPSETTING(	0x02, "30000" )
	PORT_DIPSETTING(	0x00, "None" )
	PORT_DIPNAME( 0x0c, 0x00, DEF_STR ( Difficulty ) )
	PORT_DIPSETTING(	0x00, "Easy" )
	PORT_DIPSETTING(	0x08, "Normal" )
	PORT_DIPSETTING(	0x04, "Hard" )
	PORT_DIPSETTING(	0x0c, "Very Hard" )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR ( Lives ) )
	PORT_DIPSETTING(	0x00, "2" )
	PORT_DIPSETTING(	0x20, "3" )
	PORT_DIPSETTING(	0x10, "4" )
	PORT_DIPSETTING(	0x30, "5" )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR ( Demo_Sounds ) )
	PORT_DIPSETTING(	0x40, DEF_STR ( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR ( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR ( Cabinet ) )
	PORT_DIPSETTING(	0x80, DEF_STR ( Upright ) )
	PORT_DIPSETTING(	0x00, DEF_STR ( Cocktail ) )

	COINAGE

	PORT_START		/* IN8 - FAKE port for the dial */
	PORT_ANALOG( 0xff, 0x00, IPT_DIAL | IPF_CENTER, 100, 10, 0, 0 )
INPUT_PORTS_END


INPUT_PORTS_START( startrek )
	PORT_START	/* IN0 - port 0xf8 */
	/* The next bit is referred to as the Service switch in the self test - it just adds a credit */
	PORT_BIT_IMPULSE( 0x20, IP_ACTIVE_LOW, IPT_COIN3, 3 )
	PORT_BIT_IMPULSE( 0x40, IP_ACTIVE_LOW, IPT_COIN2, 3 )
	PORT_BIT_IMPULSE( 0x80, IP_ACTIVE_LOW, IPT_COIN1, 3 )

	PORT_START	/* IN1 - port 0xf9 */
	PORT_BIT ( 0xf0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* IN2 - port 0xfa */
	PORT_BIT ( 0xf0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* IN3 - port 0xfb */
	PORT_BIT ( 0xf0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* IN4 - port 0xfc - read in machine/sega.c */
	PORT_BIT ( 0x01, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT ( 0x02, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT ( 0x04, IP_ACTIVE_HIGH, IPT_BUTTON2 )
	PORT_BIT ( 0x08, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT ( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON3 )
	PORT_BIT ( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON4 )
	PORT_BIT ( 0xc0, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START	/* IN5 - FAKE */
	/* This fake input port is used to get the status of the F2 key, */
	/* and activate the test mode, which is triggered by a NMI */
	PORT_BITX(0x01, IP_ACTIVE_HIGH, IPT_SERVICE, DEF_STR( Service_Mode ), KEYCODE_F2, IP_JOY_NONE )

	PORT_START	/* FAKE */
	/* This fake input port is used for DIP Switch 1 */
	PORT_DIPNAME( 0x03, 0x01, DEF_STR ( Bonus_Life ) )
	PORT_DIPSETTING(	0x00, "10000" )
	PORT_DIPSETTING(	0x02, "20000" )
	PORT_DIPSETTING(	0x01, "30000" )
	PORT_DIPSETTING(	0x03, "40000" )
	PORT_DIPNAME( 0x0c, 0x00, DEF_STR ( Difficulty ) )
	PORT_DIPSETTING(	0x00, "Easy" )
	PORT_DIPSETTING(	0x08, "Medium" )
	PORT_DIPSETTING(	0x04, "Hard" )
	PORT_DIPSETTING(	0x0c, "Tournament" )
	PORT_DIPNAME( 0x30, 0x30, "Photon Torpedoes" )
	PORT_DIPSETTING(	0x00, "1" )
	PORT_DIPSETTING(	0x20, "2" )
	PORT_DIPSETTING(	0x10, "3" )
	PORT_DIPSETTING(	0x30, "4" )
	PORT_DIPNAME( 0x40, 0x00, "Demo Sounds?" )
	PORT_DIPSETTING(	0x40, DEF_STR ( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR ( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR ( Cabinet ) )
	PORT_DIPSETTING(	0x80, DEF_STR ( Upright ) )
	PORT_DIPSETTING(	0x00, DEF_STR ( Cocktail ) )

	COINAGE

	PORT_START		/* IN8 - dummy port for the dial */
	PORT_ANALOG( 0xff, 0x00, IPT_DIAL | IPF_CENTER, 100, 10, 0, 0 )
INPUT_PORTS_END


INPUT_PORTS_START( tacscan )
	PORT_START	/* IN0 - port 0xf8 */
	/* The next bit is referred to as the Service switch in the self test - it just adds a credit */
	PORT_BIT_IMPULSE( 0x20, IP_ACTIVE_LOW, IPT_COIN3, 3 )
	PORT_BIT_IMPULSE( 0x40, IP_ACTIVE_LOW, IPT_COIN2, 3 )
	PORT_BIT_IMPULSE( 0x80, IP_ACTIVE_LOW, IPT_COIN1, 3 )

	PORT_START	/* IN1 - port 0xf9 */
	PORT_BIT ( 0xf0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* IN2 - port 0xfa */
	PORT_BIT ( 0xf0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* IN3 - port 0xfb */
	PORT_BIT ( 0xf0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* IN4 - port 0xfc - read in machine/sega.c */
	PORT_BIT ( 0x01, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT ( 0x02, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT ( 0x04, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT ( 0x08, IP_ACTIVE_HIGH, IPT_BUTTON2 )
	PORT_BIT ( 0xf0, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START	/* IN5 - FAKE */
	/* This fake input port is used to get the status of the F2 key, */
	/* and activate the test mode, which is triggered by a NMI */
	PORT_BITX(0x01, IP_ACTIVE_HIGH, IPT_SERVICE, DEF_STR( Service_Mode ), KEYCODE_F2, IP_JOY_NONE )

	PORT_START	/* FAKE */
	/* This fake input port is used for DIP Switch 1 */
	PORT_DIPNAME( 0x03, 0x01, DEF_STR ( Bonus_Life ) )
	PORT_DIPSETTING(	0x03, "10000" )
	PORT_DIPSETTING(	0x01, "20000" )
	PORT_DIPSETTING(	0x02, "30000" )
	PORT_DIPSETTING(	0x00, "None" )
	PORT_DIPNAME( 0x0c, 0x00, DEF_STR ( Difficulty ) )
	PORT_DIPSETTING(	0x00, "Easy" )
	PORT_DIPSETTING(	0x08, "Normal" )
	PORT_DIPSETTING(	0x04, "Hard" )
	PORT_DIPSETTING(	0x0c, "Very Hard" )
	PORT_DIPNAME( 0x30, 0x30, "Number of Ships" )
	PORT_DIPSETTING(	0x00, "2" )
	PORT_DIPSETTING(	0x20, "4" )
	PORT_DIPSETTING(	0x10, "6" )
	PORT_DIPSETTING(	0x30, "8" )
	PORT_DIPNAME( 0x40, 0x00, "Demo Sounds?" )
	PORT_DIPSETTING(	0x40, DEF_STR ( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR ( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR ( Cabinet ) )
	PORT_DIPSETTING(	0x80, DEF_STR ( Upright ) )
	PORT_DIPSETTING(	0x00, DEF_STR ( Cocktail ) )

	COINAGE

	PORT_START		/* IN8 - FAKE port for the dial */
	PORT_ANALOG( 0xff, 0x00, IPT_DIAL | IPF_CENTER, 100, 10, 0, 0 )
INPUT_PORTS_END


INPUT_PORTS_START( elim2 )
	PORT_START	/* IN0 - port 0xf8 */
	/* The next bit is referred to as the Service switch in the self test - it just adds a credit */
	PORT_BIT_IMPULSE( 0x20, IP_ACTIVE_LOW, IPT_COIN3, 3 )
	PORT_BIT_IMPULSE( 0x40, IP_ACTIVE_LOW, IPT_COIN2, 3 )
	PORT_BIT_IMPULSE( 0x80, IP_ACTIVE_LOW, IPT_COIN1, 3 )

	PORT_START	/* IN1 - port 0xf9 */
	PORT_BIT ( 0x10, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT ( 0x20, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT ( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* IN2 - port 0xfa */
	PORT_BIT ( 0x10, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT ( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT )
	PORT_BIT ( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT )
	PORT_BIT ( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* IN3 - port 0xfb */
	PORT_BIT ( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT	| IPF_PLAYER2 )
	PORT_BIT ( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT ( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* IN4 - port 0xfc - read in machine/sega.c */
	PORT_BIT ( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON1		| IPF_PLAYER2 )
	PORT_BIT ( 0x02, IP_ACTIVE_HIGH, IPT_BUTTON2		| IPF_PLAYER2 )
	PORT_BIT ( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_PLAYER2 )
	PORT_BIT ( 0xf8, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START	/* IN5 - FAKE */
	/* This fake input port is used to get the status of the F2 key, */
	/* and activate the test mode, which is triggered by a NMI */
	PORT_BITX(0x01, IP_ACTIVE_HIGH, IPT_SERVICE, DEF_STR( Service_Mode ), KEYCODE_F2, IP_JOY_NONE )

	PORT_START	/* FAKE */
		/* This fake input port is used for DIP Switch 1 */
		PORT_DIPNAME( 0x03, 0x02, DEF_STR ( Bonus_Life ) )
		PORT_DIPSETTING(	0x01, "10000" )
		PORT_DIPSETTING(	0x02, "20000" )
		PORT_DIPSETTING(	0x00, "30000" )
		PORT_DIPSETTING(	0x03, "None" )
		PORT_DIPNAME( 0x0c, 0x00, DEF_STR ( Difficulty ) )
		PORT_DIPSETTING(	0x00, "Easy" )
		PORT_DIPSETTING(	0x08, "Normal" )
		PORT_DIPSETTING(	0x04, "Hard" )
		PORT_DIPSETTING(	0x0c, "Very Hard" )
		PORT_DIPNAME( 0x30, 0x20, DEF_STR ( Lives ) )
		PORT_DIPSETTING(	0x20, "3" )
		PORT_DIPSETTING(	0x10, "4" )
		PORT_DIPSETTING(	0x00, "5" )
		/* 0x30 gives 5 Lives */
		PORT_DIPNAME( 0x80, 0x80, DEF_STR ( Cabinet ) )
		PORT_DIPSETTING(	0x80, DEF_STR ( Upright ) )
		PORT_DIPSETTING(	0x00, DEF_STR ( Cocktail ) )

		COINAGE
INPUT_PORTS_END


INPUT_PORTS_START( elim4 )
	PORT_START	/* IN0 - port 0xf8 */
	PORT_BIT ( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	/* The next bit is referred to as the Service switch in the self test - it just adds a credit */
	PORT_BIT_IMPULSE( 0x20, IP_ACTIVE_LOW, IPT_COIN1, 3 )
	PORT_BIT_IMPULSE( 0x40, IP_ACTIVE_LOW, IPT_COIN2, 3 )
	PORT_BIT_IMPULSE( 0x80, IP_ACTIVE_LOW, IPT_COIN3, 3 )

	PORT_START	/* IN1 - port 0xf9 */
	PORT_BIT ( 0x10, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT ( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 		| IPF_PLAYER2 )
	PORT_BIT ( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* IN2 - port 0xfa */
	PORT_BIT ( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT )
	PORT_BIT ( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT	| IPF_PLAYER2 )
	PORT_BIT ( 0x40, IP_ACTIVE_LOW, IPT_BUTTON1 		| IPF_PLAYER2 )
	PORT_BIT ( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* IN3 - port 0xfb */
	PORT_BIT ( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT )
	PORT_BIT ( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT	| IPF_PLAYER2 )
	PORT_BIT ( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* IN4 - port 0xfc - read in machine/sega.c */
	PORT_BIT ( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON1		| IPF_PLAYER3 )
	PORT_BIT ( 0x02, IP_ACTIVE_HIGH, IPT_BUTTON2		| IPF_PLAYER3 )
	PORT_BIT ( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_PLAYER3 )
	PORT_BIT ( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT	| IPF_PLAYER3 )
	PORT_BIT ( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1		| IPF_PLAYER4 )
	PORT_BIT ( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON2		| IPF_PLAYER4 )
	PORT_BIT ( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_PLAYER4 )
	PORT_BIT ( 0x80, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT	| IPF_PLAYER4 )

	PORT_START	/* IN5 - FAKE */
	/* This fake input port is used to get the status of the F2 key, */
	/* and activate the test mode, which is triggered by a NMI */
	PORT_BITX(0x01, IP_ACTIVE_HIGH, IPT_SERVICE, DEF_STR( Service_Mode ), KEYCODE_F2, IP_JOY_NONE )

	PORT_START	/* FAKE */
		/* This fake input port is used for DIP Switch 1 */
		PORT_DIPNAME( 0x03, 0x02, DEF_STR ( Bonus_Life ) )
		PORT_DIPSETTING(	0x01, "10000" )
		PORT_DIPSETTING(	0x02, "20000" )
		PORT_DIPSETTING(	0x00, "30000" )
		PORT_DIPSETTING(	0x03, "None" )
		PORT_DIPNAME( 0x0c, 0x00, DEF_STR ( Difficulty ) )
		PORT_DIPSETTING(	0x00, "Easy" )
		PORT_DIPSETTING(	0x08, "Normal" )
		PORT_DIPSETTING(	0x04, "Hard" )
		PORT_DIPSETTING(	0x0c, "Very Hard" )
		PORT_DIPNAME( 0x30, 0x30, DEF_STR ( Lives ) )
		PORT_DIPSETTING(	0x20, "3" )
		PORT_DIPSETTING(	0x10, "4" )
		PORT_DIPSETTING(	0x00, "5" )
		/* 0x30 gives 5 Lives */
		PORT_DIPNAME( 0x80, 0x80, DEF_STR ( Cabinet ) )
		PORT_DIPSETTING(	0x80, DEF_STR ( Upright ) )
		PORT_DIPSETTING(	0x00, DEF_STR ( Cocktail ) )

		PORT_START /* That is the coinage port in all the other games */
		PORT_BIT ( 0xff, IP_ACTIVE_LOW, IPT_UNKNOWN )

		PORT_START		/* IN8 - FAKE - port 0xfc - read in machine/sega.c */
	PORT_BIT_IMPULSE( 0x01, IP_ACTIVE_HIGH, IPT_COIN1, 3 )
	PORT_BIT_IMPULSE( 0x02, IP_ACTIVE_HIGH, IPT_COIN2, 3 )
	PORT_BIT_IMPULSE( 0x04, IP_ACTIVE_HIGH, IPT_COIN3, 3 )
	PORT_BIT_IMPULSE( 0x08, IP_ACTIVE_HIGH, IPT_COIN4, 3 )
	PORT_BIT ( 0xf0, IP_ACTIVE_HIGH, IPT_UNUSED )
INPUT_PORTS_END



/*************************************
 *
 *	Space Fury sound interfaces
 *
 *************************************/

static const char *spacfury_sample_names[] =
{
	"*spacfury",
	/* Sound samples */
	"sfury1.wav",
	"sfury2.wav",
	"sfury3.wav",
	"sfury4.wav",
	"sfury5.wav",
	"sfury6.wav",
	"sfury7.wav",
	"sfury8.wav",
	"sfury9.wav",
	"sfury10.wav",
	0	/* end of array */
};


static struct Samplesinterface spacfury_samples_interface =
{
	8,	/* 8 channels */
	10, /* volume */
	spacfury_sample_names
};


/*************************************
 *
 *	Zektor sound interfaces
 *
 *************************************/

static const char *zektor_sample_names[] =
{
	"*zektor",
	"elim1.wav",  /*  0 fireball */
	"elim2.wav",  /*  1 bounce */
	"elim3.wav",  /*  2 Skitter */
	"elim4.wav",  /*  3 Eliminator */
	"elim5.wav",  /*  4 Electron */
	"elim6.wav",  /*  5 fire */
	"elim7.wav",  /*  6 thrust */
	"elim8.wav",  /*  7 Electron */
	"elim9.wav",  /*  8 small explosion */
	"elim10.wav", /*  9 med explosion */
	"elim11.wav", /* 10 big explosion */
				  /* Missing Zizzer */
				  /* Missing City fly by */
				  /* Missing Rotation Rings */


	0	/* end of array */
};


static struct Samplesinterface zektor_samples_interface =
{
	8,
	10, /* volume */
	zektor_sample_names
};



/*************************************
 *
 *	Tac/Scan sound interfaces
 *
 *************************************/

static const char *tacscan_sample_names[] =
{
	"*tacscan",
	/* Player ship thrust sounds */
	"01.wav",
	"02.wav",
	"03.wav",
	"plaser.wav",
	"pexpl.wav",
	"pship.wav",
	"tunnelh.wav",
	"sthrust.wav",
	"slaser.wav",
	"sexpl.wav",
	"eshot.wav",
	"eexpl.wav",
	"tunnelw.wav",
	"flight1.wav",
	"flight2.wav",
	"flight3.wav",
	"flight4.wav",
	"flight5.wav",
	"formatn.wav",
	"warp.wav",
	"credit.wav",
	"1up.wav",

	0	/* end of array */
};


static struct Samplesinterface tacscan_samples_interface =
{
	12, /* 12 channels */
	100, /* volume */
	tacscan_sample_names
};


static struct CustomSound_interface tacscan_custom_interface =
{
	tacscan_sh_start,
	0,
	tacscan_sh_update
};



/*************************************
 *
 *	Eliminator sound interfaces
 *
 *************************************/

static const char *elim_sample_names[] =
{
	"*elim2",
	"elim1.wav",
	"elim2.wav",
	"elim3.wav",
	"elim4.wav",
	"elim5.wav",
	"elim6.wav",
	"elim7.wav",
	"elim8.wav",
	"elim9.wav",
	"elim10.wav",
	"elim11.wav",
	"elim12.wav",
	0	/* end of array */
};

static struct Samplesinterface elim2_samples_interface =
{
	8,	/* 8 channels */
	100, /* volume */
	elim_sample_names
};



/*************************************
 *
 *	Star Trek sound interfaces
 *
 *************************************/

static const char *startrek_sample_names[] =
{
	"*startrek",
	/* Sound samples */
	"trek1.wav",
	"trek2.wav",
	"trek3.wav",
	"trek4.wav",
	"trek5.wav",
	"trek6.wav",
	"trek7.wav",
	"trek8.wav",
	"trek9.wav",
	"trek10.wav",
	"trek11.wav",
	"trek12.wav",
	"trek13.wav",
	"trek14.wav",
	"trek15.wav",
	"trek16.wav",
	"trek17.wav",
	"trek18.wav",
	"trek19.wav",
	"trek20.wav",
	"trek21.wav",
	"trek22.wav",
	"trek23.wav",
	"trek24.wav",
	"trek25.wav",
	"trek26.wav",
	"trek27.wav",
	"trek28.wav",
	0	/* end of array */
};


static struct Samplesinterface startrek_samples_interface =
{
	5, /* 5 channels */
	10, /* volume */
	startrek_sample_names
};



/*************************************
 *
 *	Machine drivers
 *
 *************************************/

static MACHINE_DRIVER_START( elim2 )

	/* basic machine hardware */
	MDRV_CPU_ADD(Z80, 3867120)
	MDRV_CPU_MEMORY(readmem,writemem)
	MDRV_CPU_PORTS(readport,writeport)
	MDRV_CPU_PERIODIC_INT(sega_interrupt,40)

	MDRV_FRAMES_PER_SECOND(40)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_VECTOR | VIDEO_RGB_DIRECT)
	MDRV_SCREEN_SIZE(400, 300)
	MDRV_VISIBLE_AREA(512, 1536, 600, 1440)
	MDRV_PALETTE_LENGTH(256)

	MDRV_VIDEO_START(sega)
	MDRV_VIDEO_UPDATE(sega)

	/* sound hardware */
	MDRV_SOUND_ADD_TAG("samples", SAMPLES, elim2_samples_interface)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( zektor )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(elim2)

	MDRV_CPU_ADD(I8035, 3120000)
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
	MDRV_CPU_MEMORY(sega_speechboard_readmem, sega_speechboard_writemem)
	MDRV_CPU_PORTS (sega_speechboard_readport,sega_speechboard_writeport)
	MDRV_SOUND_ADD(SP0250, sega_sp0250_interface)

	/* video hardware */
	MDRV_VISIBLE_AREA(512, 1536, 624, 1432)

	/* sound hardware */
	MDRV_SOUND_REPLACE("samples", SAMPLES, zektor_samples_interface)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( tacscan )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(elim2)

	/* video hardware */
	MDRV_VISIBLE_AREA(496, 1552, 592, 1456)

	/* sound hardware */
	MDRV_SOUND_REPLACE("samples", SAMPLES, tacscan_samples_interface)
	MDRV_SOUND_ADD_TAG("custom",  CUSTOM,  tacscan_custom_interface)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( spacfury )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(elim2)

	MDRV_CPU_ADD(I8035, 3120000)
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
	MDRV_CPU_MEMORY(sega_speechboard_readmem, sega_speechboard_writemem)
	MDRV_CPU_PORTS (sega_speechboard_readport,sega_speechboard_writeport)
	MDRV_SOUND_ADD(SP0250, sega_sp0250_interface)

	/* video hardware */
	MDRV_VISIBLE_AREA(512, 1536, 552, 1464)

	/* sound hardware */
	MDRV_SOUND_REPLACE("samples", SAMPLES, spacfury_samples_interface)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( startrek )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(elim2)

	MDRV_CPU_ADD(I8035, 3120000)
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
	MDRV_CPU_MEMORY(sega_speechboard_readmem, sega_speechboard_writemem)
	MDRV_CPU_PORTS (sega_speechboard_readport,sega_speechboard_writeport)
	MDRV_SOUND_ADD(SP0250, sega_sp0250_interface)

	/* video hardware */
	MDRV_VISIBLE_AREA(512, 1536, 616, 1464)

	/* sound hardware */
	MDRV_SOUND_REPLACE("samples", SAMPLES, startrek_samples_interface)
MACHINE_DRIVER_END



/*************************************
 *
 *	ROM definitions
 *
 *************************************/

ROM_START( spacfury ) /* Revision C */
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "969c.u25",     0x0000, 0x0800, CRC(411207f2) SHA1(2a082be4052b5d8f365abd0a51ea805d270d1189) )
	ROM_LOAD( "960c.u1",      0x0800, 0x0800, CRC(d071ab7e) SHA1(c7d2429e4fa77988d7ac62bc68f876ffb7467838) )
	ROM_LOAD( "961c.u2",      0x1000, 0x0800, CRC(aebc7b97) SHA1(d0a0328ed34de9bd2c83da4ddc2d017e2b5a8bdc) )
	ROM_LOAD( "962c.u3",      0x1800, 0x0800, CRC(dbbba35e) SHA1(0400d1ba09199d19f5b8aa5bb1a85ed27930822d) )
	ROM_LOAD( "963c.u4",      0x2000, 0x0800, CRC(d9e9eadc) SHA1(1ad228d65dca48d084bbac358af80882886e7a40) )
	ROM_LOAD( "964c.u5",      0x2800, 0x0800, CRC(7ed947b6) SHA1(c0fd7ed74a87cc422a42e2a4f9eb947f5d5d9fed) )
	ROM_LOAD( "965c.u6",      0x3000, 0x0800, CRC(d2443a22) SHA1(45e5d43eae89e25370bb8e8db2b664642a238eb9) )
	ROM_LOAD( "966c.u7",      0x3800, 0x0800, CRC(1985ccfc) SHA1(8c5931519b976c82a94d17279cc919b4baad5bb7) )
	ROM_LOAD( "967c.u8",      0x4000, 0x0800, CRC(330f0751) SHA1(07ae52fdbfa2cc326f88dc76c3dc8e145b592863) )
	ROM_LOAD( "968c.u9",      0x4800, 0x0800, CRC(8366eadb) SHA1(8e4cb30a730237da2e933370faf5eaa1a41cacbf) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )     /* 64k for speech code and data */
	ROM_LOAD( "808c.u7",     0x0000, 0x0800, CRC(b779884b) SHA1(ac07e99717a1f51b79f3e43a5d873ebfa0559320) )
	ROM_LOAD( "970c.u6",     0x0800, 0x1000, CRC(979d8535) SHA1(1ed097e563319ca6d2b7df9875ce7ee921eae468) )
	ROM_LOAD( "971c.u5",     0x1800, 0x1000, CRC(022dbd32) SHA1(4e0504b5ccc28094078912673c49571cf83804ab) )
	ROM_LOAD( "972c.u4",     0x2800, 0x1000, CRC(fad9346d) SHA1(784e5ab0fb00235cfd733c502baf23960923504f) )
ROM_END


ROM_START( spacfura ) /* Revision A */
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "969a.u25",     0x0000, 0x0800, CRC(896a615c) SHA1(542386196eca9fd822e36508e173201ee8a962ed) )
	ROM_LOAD( "960a.u1",      0x0800, 0x0800, CRC(e1ea7964) SHA1(9c84c525973fcf1437b062d98195272723249d02) )
	ROM_LOAD( "961a.u2",      0x1000, 0x0800, CRC(cdb04233) SHA1(6f8d2fe6d46d04ebe94b7943006d63b24c88ed5a) )
	ROM_LOAD( "962a.u3",      0x1800, 0x0800, CRC(5f03e632) SHA1(c6e8d72ba13ab05ec01a78502948a73c21e0fd69) )
	ROM_LOAD( "963a.u4",      0x2000, 0x0800, CRC(45a77b44) SHA1(91f4822b89ec9c16c67c781a11fabfa4b9914660) )
	ROM_LOAD( "964a.u5",      0x2800, 0x0800, CRC(ba008f8b) SHA1(24f5bef240ae2bcfd5b1d95f51b3599f79518b56) )
	ROM_LOAD( "965a.u6",      0x3000, 0x0800, CRC(78677d31) SHA1(ed5810aa4bddbfe36a6ff9992dd0cb58cce66836) )
	ROM_LOAD( "966a.u7",      0x3800, 0x0800, CRC(a8a51105) SHA1(f5e0fa662552f50fa6905f579d4c678b790ffa96) )
	ROM_LOAD( "967a.u8",      0x4000, 0x0800, CRC(d60f667d) SHA1(821271ec1918e22ed29a5b1f4b0182765ef5ba10) )
	ROM_LOAD( "968a.u9",      0x4800, 0x0800, CRC(aea85b6a) SHA1(8778ff0be34cd4fd5b8f6f76c64bfca68d4d240e) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )     /* 64k for speech code and data */
	ROM_LOAD( "808a.u7",     0x0000, 0x0800, CRC(5988c767) SHA1(3b91a8cd46aa7e714028cc40f700fea32287afb1) )
	ROM_LOAD( "970.u6",      0x0800, 0x1000, CRC(f3b47b36) SHA1(6ae0b627349664140a7f70799645b368e452d69c) )
	ROM_LOAD( "971.u5",      0x1800, 0x1000, CRC(e72bbe88) SHA1(efadf8aa448c289cf4d0cf1831255b9ac60820f2) )
	ROM_LOAD( "972.u4",      0x2800, 0x1000, CRC(8b3da539) SHA1(3a0c4af96a2116fc668a340534582776b2018663) )
ROM_END


ROM_START( zektor )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "1611.cpu",     0x0000, 0x0800, CRC(6245aa23) SHA1(815f3c7edad9c290b719a60964085e90e7268112) )
	ROM_LOAD( "1586.rom",     0x0800, 0x0800, CRC(efeb4fb5) SHA1(b337179c01870c953b8d38c20263802e9a7936d3) )
	ROM_LOAD( "1587.rom",     0x1000, 0x0800, CRC(daa6c25c) SHA1(061e390775b6dd24f85d51951267bca4339a3845) )
	ROM_LOAD( "1588.rom",     0x1800, 0x0800, CRC(62b67dde) SHA1(831bad0f5a601d6859f69c70d0962c970d92db0e) )
	ROM_LOAD( "1589.rom",     0x2000, 0x0800, CRC(c2db0ba4) SHA1(658773f2b56ea805d7d678e300f9bbc896fbf176) )
	ROM_LOAD( "1590.rom",     0x2800, 0x0800, CRC(4d948414) SHA1(f60d295b0f8f798126dbfdc197943d8511238390) )
	ROM_LOAD( "1591.rom",     0x3000, 0x0800, CRC(b0556a6c) SHA1(84b481cc60dc3df3a1cf18b1ece4c70bcc7bb5a1) )
	ROM_LOAD( "1592.rom",     0x3800, 0x0800, CRC(750ecadf) SHA1(83ddd482230fbf6cf78a054fb4abd5bc8aec3ec8) )
	ROM_LOAD( "1593.rom",     0x4000, 0x0800, CRC(34f8850f) SHA1(d93594e529aca8d847c9f1e9055f1840f6069fb2) )
	ROM_LOAD( "1594.rom",     0x4800, 0x0800, CRC(52b22ab2) SHA1(c8f822a1a54081cfc88149c97b4dc19aa745a8d5) )
	ROM_LOAD( "1595.rom",     0x5000, 0x0800, CRC(a704d142) SHA1(95c1249a8efd1a69972ffd7a4da76a0bca5095d9) )
	ROM_LOAD( "1596.rom",     0x5800, 0x0800, CRC(6975e33d) SHA1(3f12037edd6f1b803b5f864789f4b88958ac9578) )
	ROM_LOAD( "1597.rom",     0x6000, 0x0800, CRC(d48ab5c2) SHA1(3f4faf4b131b120b30cd4e73ff34d5cd7ef6c47a) )
	ROM_LOAD( "1598.rom",     0x6800, 0x0800, CRC(ab54a94c) SHA1(9dd57b4b6e46d46922933128d9786df011c6133d) )
	ROM_LOAD( "1599.rom",     0x7000, 0x0800, CRC(c9d4f3a5) SHA1(8516914b49fad85222cbdd9a43609834f5d0f13d) )
	ROM_LOAD( "1600.rom",     0x7800, 0x0800, CRC(893b7dbc) SHA1(136135f0be2e8dddfa0d21a5f4119ee4685c4866) )
	ROM_LOAD( "1601.rom",     0x8000, 0x0800, CRC(867bdf4f) SHA1(5974d32d878206abd113f74ba20fa5276cf21a6f) )
	ROM_LOAD( "1602.rom",     0x8800, 0x0800, CRC(bd447623) SHA1(b8d255aeb32096891379330c5b8adf1d151d70c2) )
	ROM_LOAD( "1603.rom",     0x9000, 0x0800, CRC(9f8f10e8) SHA1(ffe9d872d9011b3233cb06d966852319f9e4cd01) )
	ROM_LOAD( "1604.rom",     0x9800, 0x0800, CRC(ad2f0f6c) SHA1(494a224905b1dac58b3b50f65a8be986b68b06f2) )
	ROM_LOAD( "1605.rom",     0xa000, 0x0800, CRC(e27d7144) SHA1(5b82fda797d86e11882d1f9738a59092c5e3e7d8) )
	ROM_LOAD( "1606.rom",     0xa800, 0x0800, CRC(7965f636) SHA1(5c8720beedab4979a813ce7f0e8961c863973ff7) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )     /* 64k for speech code and data */
	ROM_LOAD( "1607.spc",    0x0000, 0x0800, CRC(b779884b) SHA1(ac07e99717a1f51b79f3e43a5d873ebfa0559320) )
	ROM_LOAD( "1608.spc",    0x0800, 0x1000, CRC(637e2b13) SHA1(8a470f9a8a722f7ced340c4d32b4cf6f05b3e848) )
	ROM_LOAD( "1609.spc",    0x1800, 0x1000, CRC(675ee8e5) SHA1(e314482028b8925ad02e833a1d22224533d0a683) )
	ROM_LOAD( "1610.spc",    0x2800, 0x1000, CRC(2915c7bd) SHA1(3ed98747b5237aa1b3bab6866292370dc2c7655a) )
ROM_END


ROM_START( tacscan )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "1711a",        0x0000, 0x0800, CRC(0da13158) SHA1(256c5441a4841441501c9b7bcf09e0e99e8dd671) )
	ROM_LOAD( "1670c",        0x0800, 0x0800, CRC(98de6fd5) SHA1(f22c215d7558e00366fec5092abb51c670468f8c) )
	ROM_LOAD( "1671a",        0x1000, 0x0800, CRC(dc400074) SHA1(70093ef56e0784173a06da1ac781bb9d8c4e7fc5) )
	ROM_LOAD( "1672a",        0x1800, 0x0800, CRC(2caf6f7e) SHA1(200119260f78bb1c5389707b3ceedfbc1ae43549) )
	ROM_LOAD( "1673a",        0x2000, 0x0800, CRC(1495ce3d) SHA1(3189f8061961d90a52339c855c06e81f4537fb2b) )
	ROM_LOAD( "1674a",        0x2800, 0x0800, CRC(ab7fc5d9) SHA1(b2d9241d83d175ead4da36d7311a41a5f972e06a) )
	ROM_LOAD( "1675a",        0x3000, 0x0800, CRC(cf5e5016) SHA1(78a3f1e4a905515330d4737ac38576ac6e0d8611) )
	ROM_LOAD( "1676a",        0x3800, 0x0800, CRC(b61a3ab3) SHA1(0f4ef5c7fe299ad20fa4637260282a733f1cf461) )
	ROM_LOAD( "1677a",        0x4000, 0x0800, CRC(bc0273b1) SHA1(8e8d8830f17b9fa6d45d98108ca02d90c29de574) )
	ROM_LOAD( "1678b",        0x4800, 0x0800, CRC(7894da98) SHA1(2de7c121ad847e51a10cb1b81aec84cc44a3d04c) )
	ROM_LOAD( "1679a",        0x5000, 0x0800, CRC(db865654) SHA1(db4d5675b53ff2bbaf70090fd064e98862f4ad33) )
	ROM_LOAD( "1680a",        0x5800, 0x0800, CRC(2c2454de) SHA1(74101806439c9faeba88ffe573fa4f93ffa0ba3c) )
	ROM_LOAD( "1681a",        0x6000, 0x0800, CRC(77028885) SHA1(bc981620ebbfbe4e32b3b4d00504475634454c57) )
	ROM_LOAD( "1682a",        0x6800, 0x0800, CRC(babe5cf1) SHA1(26219b7a26f818fee2fe579ec6fb0b16c6bf056f) )
	ROM_LOAD( "1683a",        0x7000, 0x0800, CRC(1b98b618) SHA1(19854cb2741ba37c11ae6d429fa6c17ff930f5e5) )
	ROM_LOAD( "1684a",        0x7800, 0x0800, CRC(cb3ded3b) SHA1(f1e886f4f71b0f6f2c11fb8b4921c3452fc9b2c0) )
	ROM_LOAD( "1685a",        0x8000, 0x0800, CRC(43016a79) SHA1(ee22c1fe0c8df90d9215175104f8a796c3d2aed3) )
	ROM_LOAD( "1686a",        0x8800, 0x0800, CRC(a4397772) SHA1(cadc95b869f5bf5dba7f03dfe5ae64a50899cced) )
	ROM_LOAD( "1687a",        0x9000, 0x0800, CRC(002f3bc4) SHA1(7f3795a05d5651c90cdcd4d00c46d05178b433ea) )
	ROM_LOAD( "1688a",        0x9800, 0x0800, CRC(0326d87a) SHA1(3a5ea4526db417b9e00b24b019c1c6016773c9e7) )
	ROM_LOAD( "1709a",        0xa000, 0x0800, CRC(f35ed1ec) SHA1(dce95a862af0c6b67fb76b99fee0523d53b7551c) )
	ROM_LOAD( "1710a",        0xa800, 0x0800, CRC(6203be22) SHA1(89731c7c88d0125a11368d707f566eb53c783266) )
ROM_END


ROM_START( elim2 )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "cpu_u25.969",  0x0000, 0x0800, CRC(411207f2) SHA1(2a082be4052b5d8f365abd0a51ea805d270d1189) )
	ROM_LOAD( "1333",         0x0800, 0x0800, CRC(fd2a2916) SHA1(431d340c0c9257d66f5851a591861bcefb600cec) )
	ROM_LOAD( "1334",         0x1000, 0x0800, CRC(79eb5548) SHA1(d951de5c0ab94fdb6e58207ee9a147674dd74220) )
	ROM_LOAD( "1335",         0x1800, 0x0800, CRC(3944972e) SHA1(59c84cf23898adb7e434c5802dbb821c79099890) )
	ROM_LOAD( "1336",         0x2000, 0x0800, CRC(852f7b4d) SHA1(6db45b9d11374f4cadf185aec81f33c0040bc001) )
	ROM_LOAD( "1337",         0x2800, 0x0800, CRC(cf932b08) SHA1(f0b61ca8266fd3de7522244c9b1587eecd24a4f1) )
	ROM_LOAD( "1338",         0x3000, 0x0800, CRC(99a3f3c9) SHA1(aa7d4805c70311ebe24ff70fcc32c0e2a7c4488a) )
	ROM_LOAD( "1339",         0x3800, 0x0800, CRC(d35f0fa3) SHA1(752f14b298604a9b91e94cd6d5d291ef33f27ec0) )
	ROM_LOAD( "1340",         0x4000, 0x0800, CRC(8fd4da21) SHA1(f30627dd1fbcc12bb587742a9072bbf38ba48401) )
	ROM_LOAD( "1341",         0x4800, 0x0800, CRC(629c9a28) SHA1(cb7df14ea1bb2d7997bfae1ca70b47763c73298a) )
	ROM_LOAD( "1342",         0x5000, 0x0800, CRC(643df651) SHA1(80c5da44b5d2a7d97c7ba0067f773eb645a9d432) )
	ROM_LOAD( "1343",         0x5800, 0x0800, CRC(d29d70d2) SHA1(ee2cd752b99ebd522eccf5e71d02c31479acfdf5) )
	ROM_LOAD( "1344",         0x6000, 0x0800, CRC(c5e153a3) SHA1(7e805573aeed01e3d4ed477870800dd7ecad7a1b) )
	ROM_LOAD( "1345",         0x6800, 0x0800, CRC(40597a92) SHA1(ee1ae2b424c38b40d2cbeda4aba3328e6d3f9c81) )

	ROM_REGION( 0x0400, REGION_USER1, 0 )
	ROM_LOAD ("s-c.u39",      0x0000, 0x0400, CRC(56484d19) SHA1(61f43126fdcfc230638ed47085ae037a098e6781) )	/* unknown */
ROM_END


ROM_START( elim2a )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "cpu_u25.969",  0x0000, 0x0800, CRC(411207f2) SHA1(2a082be4052b5d8f365abd0a51ea805d270d1189) )
	ROM_LOAD( "1158",         0x0800, 0x0800, CRC(a40ac3a5) SHA1(9cf707e3439def17390ae16b49552fb1996a6335) )
	ROM_LOAD( "1159",         0x1000, 0x0800, CRC(ff100604) SHA1(1636337c702473b5a567832a622b0c09bd1e2aba) )
	ROM_LOAD( "1160a",        0x1800, 0x0800, CRC(ebfe33bd) SHA1(226da36becd278d34030f564fef61851819d2324) )
	ROM_LOAD( "1161a",        0x2000, 0x0800, CRC(03d41db3) SHA1(da9e618314c01b56b9d66abd14f1e5bf928fff54) )
	ROM_LOAD( "1162a",        0x2800, 0x0800, CRC(f2c7ece3) SHA1(86a9099ce97439cd849dc32ed2c164a1be14e4e7) )
	ROM_LOAD( "1163a",        0x3000, 0x0800, CRC(1fc58b00) SHA1(732c57781cd45cd301b2337b6879ff811d9692f3) )
	ROM_LOAD( "1164a",        0x3800, 0x0800, CRC(f37480d1) SHA1(3d7fac05d60083ddcd51c0190078c89a39f79a91) )
	ROM_LOAD( "1165a",        0x4000, 0x0800, CRC(328819f8) SHA1(ed5e3488399b4481e69f623404a28515524af60a) )
	ROM_LOAD( "1166a",        0x4800, 0x0800, CRC(1b8e8380) SHA1(d56ccc4fac9c8149ebef4939ba401372d69bf022) )
	ROM_LOAD( "1167a",        0x5000, 0x0800, CRC(16aa3156) SHA1(652a547ff1cb4ede507418b392e28f30a3cc179c) )
	ROM_LOAD( "1168a",        0x5800, 0x0800, CRC(3c7c893a) SHA1(73d2835833a6d40f6a9b0a87364af48a449d9674) )
	ROM_LOAD( "1169a",        0x6000, 0x0800, CRC(5cee23b1) SHA1(66f6fc6163148608296e3d25abb194559a2f5179) )
	ROM_LOAD( "1170a",        0x6800, 0x0800, CRC(8cdacd35) SHA1(f24f8a74cb4b8452ddbd42e61d3b0366bbee7f98) )

	ROM_REGION( 0x0400, REGION_USER1, 0 )
	ROM_LOAD ("s-c.u39",      0x0000, 0x0400, CRC(56484d19) SHA1(61f43126fdcfc230638ed47085ae037a098e6781) )	/* unknown */
ROM_END


ROM_START( elim4 )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "1390_cpu.u25", 0x0000, 0x0800, CRC(97010c3e) SHA1(b07db05abf48461b633bbabe359a973a5bc6da13) )
	ROM_LOAD( "1347",         0x0800, 0x0800, CRC(657d7320) SHA1(ef8a637d94dfa8b9dfa600269d914d635e597a9c) )
	ROM_LOAD( "1348",         0x1000, 0x0800, CRC(b15fe578) SHA1(d53773a5f7ec3c130d4ff75a5348a9f37c82c7c8) )
	ROM_LOAD( "1349",         0x1800, 0x0800, CRC(0702b586) SHA1(9847172872419c475d474ff09612c38b867e15af) )
	ROM_LOAD( "1350",         0x2000, 0x0800, CRC(4168dd3b) SHA1(1f26877c63cd7983dfa9a869e0442e8a213f382f) )
	ROM_LOAD( "1351",         0x2800, 0x0800, CRC(c950f24c) SHA1(497a9aa7b9d040a4ff7b3f938093edec2218120d) )
	ROM_LOAD( "1352",         0x3000, 0x0800, CRC(dc8c91cc) SHA1(c99224c7e57dfce9440771f78ce90ea576feed2a) )
	ROM_LOAD( "1353",         0x3800, 0x0800, CRC(11eda631) SHA1(8ba926268762d491d28d5629d5a310b1accca47d) )
	ROM_LOAD( "1354",         0x4000, 0x0800, CRC(b9dd6e7a) SHA1(ab6780f0abe99a5ef76746d45384e80399c6d611) )
	ROM_LOAD( "1355",         0x4800, 0x0800, CRC(c92c7237) SHA1(18aad6618df51a1980775a3aaa4447205453a8af) )
	ROM_LOAD( "1356",         0x5000, 0x0800, CRC(889b98e3) SHA1(23661149e7ffbdbc2c95920d13e9b8b24f86cd9a) )
	ROM_LOAD( "1357",         0x5800, 0x0800, CRC(d79248a5) SHA1(e58062d5c4e5f6fe8d70dd9b55d46a57137c9a64) )
	ROM_LOAD( "1358",         0x6000, 0x0800, CRC(c5dabc77) SHA1(2dc59e627f40fefefc206f2e9d070a62606e44fc) )
	ROM_LOAD( "1359",         0x6800, 0x0800, CRC(24c8e5d8) SHA1(d0ae6e1dfd05d170c49837760369f04df4eaa14f) )
	ROM_LOAD( "1360",         0x7000, 0x0800, CRC(96d48238) SHA1(76a7b49081cd2d0dd1976077aa66b6d5ae5b2b43) )

	ROM_REGION( 0x0400, REGION_USER1, 0 )
	ROM_LOAD ("s-c.u39",      0x0000, 0x0400, CRC(56484d19) SHA1(61f43126fdcfc230638ed47085ae037a098e6781) )	/* unknown */
ROM_END


ROM_START( startrek )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "cpu1873",      0x0000, 0x0800, CRC(be46f5d9) SHA1(fadf13042d31b0dacf02a3166545c946f6fd3f33) )
	ROM_LOAD( "1848",         0x0800, 0x0800, CRC(65e3baf3) SHA1(0c081ed6c8be0bb5eb3d5769ac1f0b8fe4735d11) )
	ROM_LOAD( "1849",         0x1000, 0x0800, CRC(8169fd3d) SHA1(439d4b857083ae40df7d7f53c36ec13b05d86a86) )
	ROM_LOAD( "1850",         0x1800, 0x0800, CRC(78fd68dc) SHA1(fb56567458807d9becaacac11091931af9889620) )
	ROM_LOAD( "1851",         0x2000, 0x0800, CRC(3f55ab86) SHA1(f75ce0c56e22e8758dd1f5ce9ac00f5f41b13465) )
	ROM_LOAD( "1852",         0x2800, 0x0800, CRC(2542ecfb) SHA1(7cacee44670768e9fae1024f172b867193d2ea4a) )
	ROM_LOAD( "1853",         0x3000, 0x0800, CRC(75c2526a) SHA1(6e86b30fcdbe7622ab873092e7a7a46d8bad790f) )
	ROM_LOAD( "1854",         0x3800, 0x0800, CRC(096d75d0) SHA1(26e90c296b00239a6cde4ec5e80cccd7bb36bcbd) )
	ROM_LOAD( "1855",         0x4000, 0x0800, CRC(bc7b9a12) SHA1(6dc60e380dc5790cd345b06c064ea7d69570aadb) )
	ROM_LOAD( "1856",         0x4800, 0x0800, CRC(ed9fe2fb) SHA1(5d56e8499cb4f54c5e76a9231c53d95777777e05) )
	ROM_LOAD( "1857",         0x5000, 0x0800, CRC(28699d45) SHA1(c133eb4fc13987e634d3789bfeaf9e03196f8fd3) )
	ROM_LOAD( "1858",         0x5800, 0x0800, CRC(3a7593cb) SHA1(7504f960507579d043b7ee20fb8fd2610399ff4b) )
	ROM_LOAD( "1859",         0x6000, 0x0800, CRC(5b11886b) SHA1(b0fb6e912953822242501943f7214e4af6ab7891) )
	ROM_LOAD( "1860",         0x6800, 0x0800, CRC(62eb96e6) SHA1(51d1f5e48e3e21147584ace61b8832ad892cb6e2) )
	ROM_LOAD( "1861",         0x7000, 0x0800, CRC(99852d1d) SHA1(eaea6a99f0a7f0292db3ea19649b5c1be45b9507) )
	ROM_LOAD( "1862",         0x7800, 0x0800, CRC(76ce27b2) SHA1(8fa8d73aa4dcf3709ecd057bad3278fac605988c) )
	ROM_LOAD( "1863",         0x8000, 0x0800, CRC(dd92d187) SHA1(5a11cdc91bb7b36ea98503892847d8dbcedfe95a) )
	ROM_LOAD( "1864",         0x8800, 0x0800, CRC(e37d3a1e) SHA1(15d949989431dcf1e0406f1e3745f3ee91012ff5) )
	ROM_LOAD( "1865",         0x9000, 0x0800, CRC(b2ec8125) SHA1(70982c614471614f6b490ae2d65faec0eff2ac37) )
	ROM_LOAD( "1866",         0x9800, 0x0800, CRC(6f188354) SHA1(e99946467090b68559c2b54ad2e85204b71a459f) )
	ROM_LOAD( "1867",         0xa000, 0x0800, CRC(b0a3eae8) SHA1(51a0855753dc2d4fe1a05bd54fa958beeab35299) )
	ROM_LOAD( "1868",         0xa800, 0x0800, CRC(8b4e2e07) SHA1(11f7de6327abf88012854417224b38a2352a9dc7) )
	ROM_LOAD( "1869",         0xb000, 0x0800, CRC(e5663070) SHA1(735944c2b924964f72f3bb3d251a35ea2aef3d15) )
	ROM_LOAD( "1870",         0xb800, 0x0800, CRC(4340616d) SHA1(e93686a29377933332523425532d102e30211111) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )     /* 64k for speech code and data */
	ROM_LOAD ("1670",         0x0000, 0x0800, CRC(b779884b) SHA1(ac07e99717a1f51b79f3e43a5d873ebfa0559320) )
	ROM_LOAD ("1871",         0x0800, 0x1000, CRC(03713920) SHA1(25a0158cab9983248e91133f96d1849c9e9bcbd2) )
	ROM_LOAD ("1872",         0x1800, 0x1000, CRC(ebb5c3a9) SHA1(533b6f0499b311f561cf7aba14a7f48ca7c47321) )

	ROM_REGION( 0x0400, REGION_USER1, 0 )
	ROM_LOAD ("s-c.u39",      0x0000, 0x0400, CRC(56484d19) SHA1(61f43126fdcfc230638ed47085ae037a098e6781) )	/* unknown */
ROM_END



/*************************************
 *
 *	Driver initialization
 *
 *************************************/

DRIVER_INIT( spacfury )
{
	/* This game uses the 315-0064 security chip */
	sega_security(64);

	install_port_read_handler(0, 0xfc, 0xfc, input_port_8_r);

	install_port_write_handler(0, 0x38, 0x38, sega_sh_speechboard_w);
	install_port_write_handler(0, 0x3e, 0x3e, spacfury1_sh_w);
	install_port_write_handler(0, 0x3f, 0x3f, spacfury2_sh_w);
	install_port_write_handler(0, 0xf8, 0xf8, IOWP_NOP);
}


DRIVER_INIT( zektor )
{
	/* This game uses the 315-0082 security chip */
	sega_security(82);

	install_port_read_handler(0, 0xfc, 0xfc, sega_IN4_r);

	install_port_write_handler(0, 0x38, 0x38, sega_sh_speechboard_w);
	install_port_write_handler(0, 0x3e, 0x3e, zektor1_sh_w);
	install_port_write_handler(0, 0x3f, 0x3f, zektor2_sh_w);
}


DRIVER_INIT( elim2 )
{
	/* This game uses the 315-0070 security chip */
	sega_security(70);

	install_port_read_handler(0, 0xfc, 0xfc, input_port_4_r);

	install_port_write_handler(0, 0x3e, 0x3e, elim1_sh_w);
	install_port_write_handler(0, 0x3f, 0x3f, elim2_sh_w);
}


DRIVER_INIT( elim4 )
{
	/* This game uses the 315-0076 security chip */
	sega_security(76);

	install_port_read_handler(0, 0xfc, 0xfc, elim4_IN4_r);

	install_port_write_handler(0, 0x3e, 0x3e, elim1_sh_w);
	install_port_write_handler(0, 0x3f, 0x3f, elim2_sh_w);
}


DRIVER_INIT( startrek )
{
	/* This game uses the 315-0064 security chip */
	sega_security(64);

	install_port_read_handler(0, 0xfc, 0xfc, sega_IN4_r);

	install_port_write_handler(0, 0x38, 0x38, sega_sh_speechboard_w);
	install_port_write_handler(0, 0x3f, 0x3f, startrek_sh_w);
}


DRIVER_INIT( tacscan )
{
	/* This game uses the 315-0076 security chip */
	sega_security(76);

	install_port_read_handler(0, 0xfc, 0xfc, sega_IN4_r);

	install_port_write_handler(0, 0x3f, 0x3f, tacscan_sh_w);
}



/*************************************
 *
 *	Game drivers
 *
 *************************************/

GAME( 1981, spacfury, 0,        spacfury, spacfury, spacfury, ROT0,   "Sega", "Space Fury (revision C)" )
GAME( 1981, spacfura, spacfury, spacfury, spacfury, spacfury, ROT0,   "Sega", "Space Fury (revision A)" )
GAME( 1982, zektor,   0,        zektor,   zektor,   zektor,   ROT0,   "Sega", "Zektor (revision B)" )
GAME( 1982, tacscan,  0,        tacscan,  tacscan,  tacscan,  ROT270, "Sega", "Tac-Scan" )
GAME( 1981, elim2,	  0,        elim2,    elim2,    elim2,    ROT0,   "Gremlin", "Eliminator (2 Players, set 1)" )
GAME( 1981, elim2a,   elim2,    elim2,    elim2,    elim2,    ROT0,   "Gremlin", "Eliminator (2 Players, set 2)" )
GAME( 1981, elim4,	  elim2,    elim2,    elim4,    elim4,    ROT0,   "Gremlin", "Eliminator (4 Players)" )
GAME( 1982, startrek, 0,        startrek, startrek, startrek, ROT0,   "Sega", "Star Trek" )
