/*************************************************************************

	Sega Z80-3D system

	driver by Alex Pasadyn, Howie Cohen, Frank Palazzolo, Ernesto Corvi,
	and Aaron Giles

	Games supported:
		* Turbo
		* Subroc 3D
		* Buck Rogers: Planet of Zoom

**************************************************************************
	TURBO
**************************************************************************

	Memory Map:  ( * not complete * )

	Address Range:	R/W:	 Function:
	--------------------------------------------------------------------------
	0000 - 5fff		R		 Program ROM
	a000 - a0ff		W		 Sprite RAM
	a800 - a803		W		 Lamps / Coin Meters
	b000 - b1ff		R/W		 Collision RAM
	e000 - e7ff		R/W		 character RAM
	f000 - f7ff		R/W		 RAM
	f202					 coinage 2
	f205					 coinage 1
	f800 - f803		R/W		 road drawing
	f900 - f903		R/W		 road drawing
	fa00 - fa03		R/W		 sound
	fb00 - fb03		R/W		 x,DS2,x,x
	fc00 - fc01		R		 DS1,x
	fc00 - fc01		W		 score
	fd00			R		 Coin Inputs, etc.
	fe00			R		 DS3,x

	Switch settings:
	Notes:
		1) Facing the CPU board, with the two large IDC connectors at
		   the top of the board, and the large and small IDC
		   connectors at the bottom, DIP switch #1 is upper right DIP
		   switch, DIP switch #2 is the DIP switch to the right of it.

		2) Facing the Sound board, with the IDC connector at the
		   bottom of the board, DIP switch #3 (4 bank) can be seen.
	----------------------------------------------------------------------------

	Option	   (DIP Swtich #1) | SW1 | SW2 | SW3 | SW4 | SW5 | SW6 | SW7 | SW8 |
	 --------------------------|-----|-----|-----|-----|-----|-----|-----|-----|
	1 Car On Extended Play	   | ON	 | ON  |	 |	   |	 |	   |	 |	   |
	2 Car On Extended Play	   | OFF | ON  |	 |	   |	 |	   |	 |	   |
	3 Car On Extended Play	   | ON	 | OFF |	 |	   |	 |	   |	 |	   |
	4 Car On Extended Play	   | OFF | OFF |	 |	   |	 |	   |	 |	   |
	 --------------------------|-----|-----|-----|-----|-----|-----|-----|-----|
	Game Time Adjustable	   |	 |	   | ON	 |	   |	 |	   |	 |	   |
	Game Time Fixed (55 Sec.)  |	 |	   | OFF |	   |	 |	   |	 |	   |
	 --------------------------|-----|-----|-----|-----|-----|-----|-----|-----|
	Hard Game Difficulty	   |	 |	   |	 | ON  |	 |	   |	 |	   |
	Easy Game Difficulty	   |	 |	   |	 | OFF |	 |	   |	 |	   |
	 --------------------------|-----|-----|-----|-----|-----|-----|-----|-----|
	Normal Game Mode		   |	 |	   |	 |	   | ON	 |	   |	 |	   |
	No Collisions (cheat)	   |	 |	   |	 |	   | OFF |	   |	 |	   |
	 --------------------------|-----|-----|-----|-----|-----|-----|-----|-----|
	Initial Entry Off (?)	   |	 |	   |	 |	   |	 | ON  |	 |	   |
	Initial Entry On  (?)	   |	 |	   |	 |	   |	 | OFF |	 |	   |
	 --------------------------|-----|-----|-----|-----|-----|-----|-----|-----|
	Not Used				   |	 |	   |	 |	   |	 |	   |  X	 |	X  |
	---------------------------------------------------------------------------

	Option	   (DIP Swtich #2) | SW1 | SW2 | SW3 | SW4 | SW5 | SW6 | SW7 | SW8 |
	--------------------------|-----|-----|-----|-----|-----|-----|-----|-----|
	60 Seconds Game Time	   | ON	 | ON  |	 |	   |	 |	   |	 |	   |
	70 Seconds Game Time	   | OFF | ON  |	 |	   |	 |	   |	 |	   |
	80 Seconds Game Time	   | ON	 | OFF |	 |	   |	 |	   |	 |	   |
	90 Seconds Game Time	   | OFF | OFF |	 |	   |	 |	   |	 |	   |
	 --------------------------|-----|-----|-----|-----|-----|-----|-----|-----|
	Slot 1	 1 Coin	 1 Credit  |	 |	   | ON	 | ON  | ON	 |	   |	 |	   |
	Slot 1	 1 Coin	 2 Credits |	 |	   | OFF | ON  | ON	 |	   |	 |	   |
	Slot 1	 1 Coin	 3 Credits |	 |	   | ON	 | OFF | ON	 |	   |	 |	   |
	Slot 1	 1 Coin	 6 Credits |	 |	   | OFF | OFF | ON	 |	   |	 |	   |
	Slot 1	 2 Coins 1 Credit  |	 |	   | ON	 | ON  | OFF |	   |	 |	   |
	Slot 1	 3 Coins 1 Credit  |	 |	   | OFF | ON  | OFF |	   |	 |	   |
	Slot 1	 4 Coins 1 Credit  |	 |	   | ON	 | OFF | OFF |	   |	 |	   |
	Slot 1	 1 Coin	 1 Credit  |	 |	   | OFF | OFF | OFF |	   |	 |	   |
	 --------------------------|-----|-----|-----|-----|-----|-----|-----|-----|
	Slot 2	 1 Coin	 1 Credit  |	 |	   |	 |	   |	 | ON  | ON	 | ON  |
	Slot 2	 1 Coin	 2 Credits |	 |	   |	 |	   |	 | OFF | ON	 | ON  |
	Slot 2	 1 Coin	 3 Credits |	 |	   |	 |	   |	 | ON  | OFF | ON  |
	Slot 2	 1 Coin	 6 Credits |	 |	   |	 |	   |	 | OFF | OFF | ON  |
	Slot 2	 2 Coins 1 Credit  |	 |	   |	 |	   |	 | ON  | ON	 | OFF |
	Slot 2	 3 Coins 1 Credit  |	 |	   |	 |	   |	 | OFF | ON	 | OFF |
	Slot 2	 4 Coins 1 Credit  |	 |	   |	 |	   |	 | ON  | OFF | OFF |
	Slot 2	 1 Coins 1 Credit  |	 |	   |	 |	   |	 | OFF | OFF | OFF |
	---------------------------------------------------------------------------

	Option	   (DIP Swtich #3) | SW1 | SW2 | SW3 | SW4 |
	 --------------------------|-----|-----|-----|-----|
	Not Used				   |  X	 |	X  |	 |	   |
	 --------------------------|-----|-----|-----|-----|
	Digital (LED) Tachometer   |	 |	   | ON	 |	   |
	Analog (Meter) Tachometer  |	 |	   | OFF |	   |
	 --------------------------|-----|-----|-----|-----|
	Cockpit Sound System	   |	 |	   |	 | ON  |
	Upright Sound System	   |	 |	   |	 | OFF |
	---------------------------------------------------

	Here is a complete list of the ROMs:

	Turbo ROMLIST - Frank Palazzolo
	Name	 Loc			 Function
	-----------------------------------------------------------------------------
	Images Acquired:
	EPR1262,3,4	 IC76, IC89, IC103
	EPR1363,4,5
	EPR15xx				Program ROMS
	EPR1244				Character Data 1
	EPR1245				Character Data 2
	EPR-1125			Road ROMS
	EPR-1126
	EPR-1127
	EPR-1238
	EPR-1239
	EPR-1240
	EPR-1241
	EPR-1242
	EPR-1243
	EPR1246-1258		Sprite ROMS
	EPR1288-1300

	PR-1114		IC13	Color 1 (road, etc.)
	PR-1115		IC18	Road gfx
	PR-1116		IC20	Crash (collision detection?)
	PR-1117		IC21	Color 2 (road, etc.)
	PR-1118		IC99	256x4 Character Color PROM
	PR-1119		IC50	512x8 Vertical Timing PROM
	PR-1120		IC62	Horizontal Timing PROM
	PR-1121		IC29	Color PROM
	PR-1122		IC11	Pattern 1
	PR-1123		IC21	Pattern 2

	PA-06R		IC22	Mathbox Timing PAL
	PA-06L		IC90	Address Decode PAL

**************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "machine/8255ppi.h"
#include "turbo.h"
#include "machine/segacrpt.h"


/*************************************
 *
 *	Turbo CPU memory handlers
 *
 *************************************/

static MEMORY_READ_START( turbo_readmem )
	{ 0x0000, 0x5fff, MRA_ROM },
	{ 0xb000, 0xb1ff, MRA_RAM },
	{ 0xe000, 0xe7ff, MRA_RAM },
	{ 0xf000, 0xf7ff, MRA_RAM },
	{ 0xf800, 0xf803, ppi8255_0_r },
	{ 0xf900, 0xf903, ppi8255_1_r },
	{ 0xfa00, 0xfa03, ppi8255_2_r },
	{ 0xfb00, 0xfb03, ppi8255_3_r },
	{ 0xfc00, 0xfcff, turbo_8279_r },
	{ 0xfd00, 0xfdff, input_port_0_r },
	{ 0xfe00, 0xfeff, turbo_collision_r },
MEMORY_END


static MEMORY_WRITE_START( turbo_writemem )
	{ 0x0000, 0x5fff, MWA_ROM },
	{ 0xa000, 0xa0ff, MWA_RAM, &spriteram, &spriteram_size },
	{ 0xa800, 0xa807, turbo_coin_and_lamp_w },
	{ 0xb000, 0xb1ff, MWA_RAM, &sega_sprite_position },
	{ 0xb800, 0xb800, MWA_NOP },	/* resets the analog wheel value */
	{ 0xe000, 0xe7ff, MWA_RAM, &videoram, &videoram_size },
	{ 0xe800, 0xe800, turbo_collision_clear_w },
	{ 0xf000, 0xf7ff, MWA_RAM },
	{ 0xf800, 0xf803, ppi8255_0_w },
	{ 0xf900, 0xf903, ppi8255_1_w },
	{ 0xfa00, 0xfa03, ppi8255_2_w },
	{ 0xfb00, 0xfb03, ppi8255_3_w },
	{ 0xfc00, 0xfcff, turbo_8279_w },
MEMORY_END



/*************************************
 *
 *	Subroc3D CPU memory handlers
 *
 *************************************/

static MEMORY_READ_START( subroc3d_readmem )
	{ 0x0000, 0x5fff, MRA_ROM },
	{ 0xa000, 0xa7ff, MRA_RAM },
	{ 0xa800, 0xa800, input_port_0_r },
	{ 0xa801, 0xa801, input_port_1_r },
	{ 0xa802, 0xa802, input_port_2_r },
	{ 0xa803, 0xa803, input_port_3_r },
	{ 0xb000, 0xb7ff, MRA_RAM },
	{ 0xe000, 0xe7ff, MRA_RAM },
	{ 0xe800, 0xe803, ppi8255_0_r },
	{ 0xf000, 0xf003, ppi8255_1_r },
MEMORY_END


static MEMORY_WRITE_START( subroc3d_writemem )
	{ 0x0000, 0x5fff, MWA_ROM },
	{ 0xa000, 0xa3ff, MWA_RAM, &sega_sprite_position },
	{ 0xa400, 0xa7ff, MWA_RAM, &spriteram, &spriteram_size },
	{ 0xb000, 0xb7ff, MWA_RAM },
	{ 0xe000, 0xe7ff, MWA_RAM, &videoram, &videoram_size },
	{ 0xe800, 0xe803, ppi8255_0_w },
	{ 0xf000, 0xf003, ppi8255_1_w },
	{ 0xf800, 0xf801, turbo_8279_w },
MEMORY_END



/*************************************
 *
 *	Buck Rogers CPU memory handlers
 *
 *************************************/

static MEMORY_READ_START( buckrog_readmem )
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0xc000, 0xc7ff, MRA_RAM },
	{ 0xc800, 0xc803, ppi8255_0_r },
	{ 0xd000, 0xd003, ppi8255_1_r },
	{ 0xe000, 0xe1ff, MRA_RAM },
	{ 0xe800, 0xe800, input_port_0_r },
	{ 0xe801, 0xe801, input_port_1_r },
	{ 0xe802, 0xe802, buckrog_port_2_r },
	{ 0xe803, 0xe803, buckrog_port_3_r },
	{ 0xf800, 0xffff, MRA_RAM },
MEMORY_END


static MEMORY_WRITE_START( buckrog_writemem )
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0xc000, 0xc7ff, MWA_RAM, &videoram, &videoram_size },
	{ 0xc800, 0xc803, ppi8255_0_w },
	{ 0xd000, 0xd003, ppi8255_1_w },
	{ 0xd800, 0xd801, turbo_8279_w },
	{ 0xe000, 0xe1ff, MWA_RAM, &sega_sprite_position },
	{ 0xe400, 0xe4ff, MWA_RAM, &spriteram, &spriteram_size },
	{ 0xf800, 0xffff, MWA_RAM },
MEMORY_END


static MEMORY_READ_START( buckrog_readmem2 )
	{ 0x0000, 0x0fff, MRA_ROM },
	{ 0xf000, 0xf7ff, MRA_RAM },
MEMORY_END


static PORT_READ_START( buckrog_readport2 )
	{ 0x00, 0x00, buckrog_cpu2_command_r },
PORT_END


static MEMORY_WRITE_START( buckrog_writemem2 )
	{ 0x0000, 0xdfff, buckrog_bitmap_w },
	{ 0xf000, 0xf7ff, MWA_RAM },
MEMORY_END




/*************************************
 *
 *	Port definitions
 *
 *************************************/

INPUT_PORTS_START( turbo )
	PORT_START		/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON2 )				/* ACCEL B */
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON1 )				/* ACCEL A */
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_TOGGLE )	/* SHIFT */
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START1 )
	PORT_SERVICE_NO_TOGGLE( 0x10, IP_ACTIVE_LOW )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN1 )

	PORT_START	/* DSW 1 */
	PORT_DIPNAME( 0x03, 0x03, "Car On Extended Play" )
	PORT_DIPSETTING( 0x03, "1" )
	PORT_DIPSETTING( 0x02, "2" )
	PORT_DIPSETTING( 0x01, "3" )
	PORT_DIPSETTING( 0x00, "4" )
	PORT_DIPNAME( 0x04, 0x04, "Game Time" )
	PORT_DIPSETTING( 0x00, "Fixed (55 sec)" )
	PORT_DIPSETTING( 0x04, "Adjustable" )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Difficulty ) )
	PORT_DIPSETTING( 0x00, "Easy")
	PORT_DIPSETTING( 0x08, "Hard")
	PORT_DIPNAME( 0x10, 0x00, "Game Mode" )
	PORT_DIPSETTING( 0x10, "No Collisions (cheat)" )
	PORT_DIPSETTING( 0x00, "Normal" )
	PORT_DIPNAME( 0x20, 0x00, "Initial Entry" )
	PORT_DIPSETTING( 0x20, DEF_STR( Off ))
	PORT_DIPSETTING( 0x00, DEF_STR( On ))
	PORT_BIT( 0xc0, 0xc0, IPT_UNUSED )

	PORT_START	/* DSW 2 */
	PORT_DIPNAME( 0x03, 0x03, "Game Time" )
	PORT_DIPSETTING( 0x00, "60 seconds" )
	PORT_DIPSETTING( 0x01, "70 seconds" )
	PORT_DIPSETTING( 0x02, "80 seconds" )
	PORT_DIPSETTING( 0x03, "90 seconds" )
	PORT_DIPNAME( 0x1c, 0x1c, DEF_STR( Coin_B ))
	PORT_DIPSETTING(	0x18, DEF_STR( 4C_1C ))
	PORT_DIPSETTING(	0x14, DEF_STR( 3C_1C ))
	PORT_DIPSETTING(	0x10, DEF_STR( 2C_1C ))
/*	PORT_DIPSETTING(	0x00, DEF_STR( 1C_1C ))*/
	PORT_DIPSETTING(	0x1c, DEF_STR( 1C_1C ))
	PORT_DIPSETTING(	0x04, DEF_STR( 1C_2C ))
	PORT_DIPSETTING(	0x08, DEF_STR( 1C_3C ))
	PORT_DIPSETTING(	0x0c, DEF_STR( 1C_6C ))
	PORT_DIPNAME( 0xe0, 0xe0, DEF_STR( Coin_A ))
	PORT_DIPSETTING(	0xc0, DEF_STR( 4C_1C ))
	PORT_DIPSETTING(	0xa0, DEF_STR( 3C_1C ))
	PORT_DIPSETTING(	0x80, DEF_STR( 2C_1C ))
/*	PORT_DIPSETTING(	0x00, DEF_STR( 1C_1C ))*/
	PORT_DIPSETTING(	0xe0, DEF_STR( 1C_1C ))
	PORT_DIPSETTING(	0x20, DEF_STR( 1C_2C ))
	PORT_DIPSETTING(	0x40, DEF_STR( 1C_3C ))
	PORT_DIPSETTING(	0x60, DEF_STR( 1C_6C ))

	PORT_START	/* DSW 3 */
	PORT_BIT( 0x0f, 0x00, IPT_UNUSED )		/* Merged with collision bits */
	PORT_BIT( 0x30, 0x00, IPT_UNUSED )
	PORT_DIPNAME( 0x40, 0x40, "Tachometer" )
	PORT_DIPSETTING(	0x40, "Analog (Meter)")
	PORT_DIPSETTING(	0x00, "Digital (led)")
	PORT_DIPNAME( 0x80, 0x80, "Sound System" )
	PORT_DIPSETTING(	0x80, DEF_STR( Upright ) )
	PORT_DIPSETTING(	0x00, "Cockpit")

	PORT_START		/* IN0 */
	PORT_ANALOG( 0xff, 0, IPT_DIAL | IPF_CENTER, 10, 30, 0, 0 )
INPUT_PORTS_END


INPUT_PORTS_START( subroc3d )
	PORT_START
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP )

	PORT_START
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT )
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START1 )
	PORT_SERVICE_NO_TOGGLE( 0x10, 0x10 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN1 )

	PORT_START  /* DSW 2 */
	PORT_DIPNAME( 0x07, 0x00, DEF_STR( Coin_A ))
	PORT_DIPSETTING(    0x07, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x05, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_6C ) )
	PORT_DIPNAME( 0x38, 0x00, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x38, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x28, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x18, DEF_STR( 1C_6C ) )
	PORT_DIPNAME( 0xc0, 0x40, "Ships" )
	PORT_DIPSETTING(    0x00, "2" )
	PORT_DIPSETTING(    0x40, "3" )
	PORT_DIPSETTING(    0x80, "4" )
	PORT_DIPSETTING(    0xc0, "5" )

	PORT_START  /* DSW 3 */
	PORT_DIPNAME( 0x03, 0x03, "Extra Ship" )
	PORT_DIPSETTING(    0x00, "20000" )
	PORT_DIPSETTING(    0x01, "40000" )
	PORT_DIPSETTING(    0x02, "60000" )
	PORT_DIPSETTING(    0x03, "80000" )
	PORT_DIPNAME( 0x04, 0x04, "Initial Input" )
	PORT_DIPSETTING(    0x00, "Disable" )
	PORT_DIPSETTING(    0x04, "Enable" )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x00, "Difficult" )
	PORT_DIPSETTING(    0x08, "Normal" )
	PORT_DIPNAME( 0x10, 0x10, "Play" )
	PORT_DIPSETTING(    0x00, "Free" )
	PORT_DIPSETTING(    0x10, "Normal" )
	PORT_DIPNAME( 0x20, 0x20, "Motion" )
	PORT_DIPSETTING(    0x00, "Stop" )
	PORT_DIPSETTING(    0x20, "Normal" )
	PORT_DIPNAME( 0x40, 0x00, "Screen" )
	PORT_DIPSETTING(    0x00, "Mono" )
	PORT_DIPSETTING(    0x40, "Stereo" )
	PORT_DIPNAME( 0x80, 0x80, "Game" )
	PORT_DIPSETTING(    0x00, "Endless" )
	PORT_DIPSETTING(    0x80, "Normal" )

	PORT_START  /* DSW 1 */					/* Unused */
INPUT_PORTS_END


INPUT_PORTS_START( buckrog )
	PORT_START
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON3 ) /* Accel Hi*/
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 ) /* Accel Lo*/
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_UP )

	PORT_START /* Inputs */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START1 )
	PORT_SERVICE_NO_TOGGLE( 0x10, 0x10 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN1 )

	PORT_START  /* DSW 1 */
	PORT_DIPNAME( 0x07, 0x00, DEF_STR( Coin_A ))
	PORT_DIPSETTING(    0x07, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x05, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_6C ) )
	PORT_DIPNAME( 0x38, 0x00, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x38, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x28, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x18, DEF_STR( 1C_6C ) )

	PORT_START  /* DSW 2 */
	PORT_DIPNAME( 0x01, 0x00, "Collisions" )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, "Accel by" )
	PORT_DIPSETTING(    0x00, "Pedal" )
	PORT_DIPSETTING(    0x02, "Button" )
	PORT_DIPNAME( 0x04, 0x00, "Best 5 Scores" )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x00, "Score Display" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x00, "Difficult" )
	PORT_DIPSETTING(    0x10, "Normal" )
	PORT_DIPNAME( 0x60, 0x00, "Extra Ships" )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x20, "4" )
	PORT_DIPSETTING(    0x40, "5" )
	PORT_DIPSETTING(    0x60, "6" )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x80, "Cockpit" )
INPUT_PORTS_END



/*************************************
 *
 *	Sound interfaces
 *
 *************************************/

static const char *turbo_sample_names[]=
{
	"*turbo",
	"01.wav",		/* Trig1 */
	"02.wav",		/* Trig2 */
	"03.wav",		/* Trig3 */
	"04.wav",		/* Trig4 */
	"05.wav",		/* Screech */
	"06.wav",		/* Crash */
	"skidding.wav",	/* Spin */
	"idle.wav",		/* Idle */
	"ambulanc.wav",	/* Ambulance */
	0
};

static struct Samplesinterface turbo_samples_interface =
{
	8,			/* eight channels */
	25,			/* volume */
	turbo_sample_names
};


static const char *buckrog_sample_names[]=
{
	"*buckrog",
	"alarm0.wav",
	"alarm1.wav",
	"alarm2.wav",
	"alarm3.wav",
	"exp.wav",
	"fire.wav",
	"rebound.wav",
	"hit.wav",
	"shipsnd1.wav",
	"shipsnd2.wav",
	"shipsnd3.wav",
	0
};

static struct Samplesinterface buckrog_samples_interface =
{
	6,          /* 6 channels */
	25,         /* volume */
	buckrog_sample_names
};


static const char *subroc3d_sample_names[] =
{
	"*subroc3d",
	"01.wav",   /* enemy missile */
	"02.wav",   /* enemy torpedo */
	"03.wav",   /* enemy fighter */
	"04.wav",   /* explosion in sky */
	"05.wav",   /* explosion on sea */
	"06.wav",   /* missile shoot */
	"07.wav",   /* torpedo shoot */
	"08.wav",   /* my ship expl */
	"09.wav",   /* prolog sound */
	"11.wav",   /* alarm 0 */
	"12.wav",   /* alarm 1 */
	0
};

static struct Samplesinterface subroc3d_samples_interface =
{
	8,          /* eight channels */
	50,         /* volume */
	subroc3d_sample_names
};


/*************************************
 *
 *	Machine drivers
 *
 *************************************/

static MACHINE_DRIVER_START( turbo )

	/* basic machine hardware */
	MDRV_CPU_ADD(Z80, 5000000)
	MDRV_CPU_MEMORY(turbo_readmem,turbo_writemem)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,1)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)
	MDRV_MACHINE_INIT(turbo)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(1*8, 32*8-1, 1*8, 27*8-1)
	MDRV_PALETTE_LENGTH(512)

	MDRV_PALETTE_INIT(turbo)
	MDRV_VIDEO_START(turbo)
	MDRV_VIDEO_EOF(turbo)
	MDRV_VIDEO_UPDATE(turbo)

	/* sound hardware */
	MDRV_SOUND_ADD(SAMPLES, turbo_samples_interface)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( subroc3d )

	/* basic machine hardware */
	MDRV_CPU_ADD(Z80, 5000000)
	MDRV_CPU_MEMORY(subroc3d_readmem,subroc3d_writemem)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,1)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)
	MDRV_MACHINE_INIT(subroc3d)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_UPDATE_AFTER_VBLANK)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 30*8-1, 0*8, 28*8-1)
	MDRV_PALETTE_LENGTH(512)

	MDRV_PALETTE_INIT(subroc3d)
	MDRV_VIDEO_START(subroc3d)
	MDRV_VIDEO_UPDATE(subroc3d)

	/* sound hardware */
	MDRV_SOUND_ADD(SAMPLES, subroc3d_samples_interface)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( buckrog )

	/* basic machine hardware */
	MDRV_CPU_ADD(Z80, 5000000)
	MDRV_CPU_MEMORY(buckrog_readmem,buckrog_writemem)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,1)

	MDRV_CPU_ADD(Z80, 5000000)
	MDRV_CPU_MEMORY(buckrog_readmem2,buckrog_writemem2)
	MDRV_CPU_PORTS(buckrog_readport2,0)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(10)
	MDRV_MACHINE_INIT(buckrog)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 0*8, 28*8-1)
	MDRV_PALETTE_LENGTH(1024+512+256)

	MDRV_PALETTE_INIT(buckrog)
	MDRV_VIDEO_START(buckrog)
	MDRV_VIDEO_UPDATE(buckrog)

	/* sound hardware */
	MDRV_SOUND_ADD(SAMPLES, buckrog_samples_interface)
MACHINE_DRIVER_END



/*************************************
 *
 *	ROM definitions
 *
 *************************************/

ROM_START( turbo )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "epr1513.bin",  0x0000, 0x2000, CRC(0326adfc) SHA1(d9f06f0bc78667fa58c4b8ab3a3897d0dd0bdfbf) )
	ROM_LOAD( "epr1514.bin",  0x2000, 0x2000, CRC(25af63b0) SHA1(9af4b3da83a4cef79b7dd0e9061132c499872c1c) )
	ROM_LOAD( "epr1515.bin",  0x4000, 0x2000, CRC(059c1c36) SHA1(ba870e6f45ff15aa148b2c2f213c879144aaacf0) )

	ROM_REGION( 0x20000, REGION_GFX1, 0 )	/* sprite data */
	ROM_LOAD( "epr1246.rom", 0x00000, 0x2000, CRC(555bfe9a) SHA1(1e56385475eeff044dcd9b44a154991d3efe995e) )	/* level 0 */
	ROM_RELOAD(				 0x02000, 0x2000 )
	ROM_LOAD( "epr1247.rom", 0x04000, 0x2000, CRC(c8c5e4d5) SHA1(da70297340ddea0cd7fe04f2d94ea65f8202d0e5) )	/* level 1 */
	ROM_RELOAD(				 0x06000, 0x2000 )
	ROM_LOAD( "epr1248.rom", 0x08000, 0x2000, CRC(82fe5b94) SHA1(b96688ca0cfd90fdc4ee7c2e6c0b66726cc5713c) )	/* level 2 */
	ROM_RELOAD(				 0x0a000, 0x2000 )
	ROM_LOAD( "epr1249.rom", 0x0c000, 0x2000, CRC(e258e009) SHA1(598d382db0f789ea2fde749b7467abed545de25a) )	/* level 3 */
	ROM_LOAD( "epr1250.rom", 0x0e000, 0x2000, CRC(aee6e05e) SHA1(99b9b1ec996746ddf713ed38192f350f1f32a847) )
	ROM_LOAD( "epr1251.rom", 0x10000, 0x2000, CRC(292573de) SHA1(3ddc980d11478a6a6e4082c2f76c1ab82ffe2f36) )	/* level 4 */
	ROM_LOAD( "epr1252.rom", 0x12000, 0x2000, CRC(aee6e05e) SHA1(99b9b1ec996746ddf713ed38192f350f1f32a847) )
	ROM_LOAD( "epr1253.rom", 0x14000, 0x2000, CRC(92783626) SHA1(13979eb964112436182d2a92f21803bcc28f4a4a) )	/* level 5 */
	ROM_LOAD( "epr1254.rom", 0x16000, 0x2000, CRC(aee6e05e) SHA1(99b9b1ec996746ddf713ed38192f350f1f32a847) )
	ROM_LOAD( "epr1255.rom", 0x18000, 0x2000, CRC(485dcef9) SHA1(0f760ebb42cc2580a29758c72428a41d74477ce6) )	/* level 6 */
	ROM_LOAD( "epr1256.rom", 0x1a000, 0x2000, CRC(aee6e05e) SHA1(99b9b1ec996746ddf713ed38192f350f1f32a847) )
	ROM_LOAD( "epr1257.rom", 0x1c000, 0x2000, CRC(4ca984ce) SHA1(99f294fb203f23929b44baa2dd1825c67dde08a1) )	/* level 7 */
	ROM_LOAD( "epr1258.rom", 0x1e000, 0x2000, CRC(aee6e05e) SHA1(99b9b1ec996746ddf713ed38192f350f1f32a847) )

	ROM_REGION( 0x1000, REGION_GFX2, 0 )	/* foreground data */
	ROM_LOAD( "epr1244.rom", 0x0000, 0x0800, CRC(17f67424) SHA1(6126562510f1509f3487faaa3b9d7470ab600a2c) )
	ROM_LOAD( "epr1245.rom", 0x0800, 0x0800, CRC(2ba0b46b) SHA1(5d4d4f19ad7a911c7b37db190a420faf665546b4) )

	ROM_REGION( 0x4800, REGION_GFX3, 0 )	/* road data */
	ROM_LOAD( "epr1125.rom", 0x0000, 0x0800, CRC(65b5d44b) SHA1(bbdd5db013c9d876e9666f17c48569c7531bfc08) )
	ROM_LOAD( "epr1126.rom", 0x0800, 0x0800, CRC(685ace1b) SHA1(99c8d36ac910169b27676d18c894433c2ba44853) )
	ROM_LOAD( "epr1127.rom", 0x1000, 0x0800, CRC(9233c9ca) SHA1(cbf9a0f564d8ace1ccd701c1769dbc001d465851) )
	ROM_LOAD( "epr1238.rom", 0x1800, 0x0800, CRC(d94fd83f) SHA1(1e3a68259d2ede623d5a7306fdf693a4eab301f0) )
	ROM_LOAD( "epr1239.rom", 0x2000, 0x0800, CRC(4c41124f) SHA1(d73a9441552c77fb3078553195794311a950d589) )
	ROM_LOAD( "epr1240.rom", 0x2800, 0x0800, CRC(371d6282) SHA1(f5902b357d976822d46aa6404b7bd30855d435a9) )
	ROM_LOAD( "epr1241.rom", 0x3000, 0x0800, CRC(1109358a) SHA1(27a5351a4e87309671e72115299420315a93dba6) )
	ROM_LOAD( "epr1242.rom", 0x3800, 0x0800, CRC(04866769) SHA1(1f9c0d53766fdaf8de57d3df05f291c2ca3dc5fb) )
	ROM_LOAD( "epr1243.rom", 0x4000, 0x0800, CRC(29854c48) SHA1(cab89bc30f83d9746931ddf6f95a6d0c8a517e5d) )

	ROM_REGION( 0x200, REGION_GFX4, 0 )		/* number data (copied at init time) */

	ROM_REGION( 0x1000, REGION_PROMS, 0 )	/* various PROMs */
	ROM_LOAD( "pr1121.bin",	 0x0000, 0x0200, CRC(7692f497) SHA1(42468c0705df9928e15ff8deb7e793a6c0c04353) )	/* palette */
	ROM_LOAD( "pr1122.bin",	 0x0200, 0x0400, CRC(1a86ce70) SHA1(cab708b9a089b2e28f2298c1e4fae6e200923527) )	/* sprite priorities */
	ROM_LOAD( "pr1123.bin",	 0x0600, 0x0400, CRC(02d2cb52) SHA1(c34d6b60355747ce20fcb8d322df0e188d187f10) )	/* sprite/road/background priorities */
	ROM_LOAD( "pr-1118.bin", 0x0a00, 0x0100, CRC(07324cfd) SHA1(844abc2042d6810fa34d84ff1ed57744886c6ea6) )	/* background color table */
	ROM_LOAD( "pr1114.bin",	 0x0b00, 0x0020, CRC(78aded46) SHA1(c78afe804f8b8e837b0c502de5b8715a41fb92b9) )	/* road red/green color table */
	ROM_LOAD( "pr1117.bin",	 0x0b20, 0x0020, CRC(f06d9907) SHA1(f11db7800f41b03e79f5eef8d7ef3ae0a6277518) )	/* road green/blue color table */
	ROM_LOAD( "pr1115.bin",	 0x0b40, 0x0020, CRC(5394092c) SHA1(129ff61104979ff6a3c3af8bf81c04ae9b133c9e) )	/* road collision/enable */
	ROM_LOAD( "pr1116.bin",	 0x0b60, 0x0020, CRC(3956767d) SHA1(073aaf57175526660fcf7af2e16e7f1d1aaba9a9) )	/* collision detection */
	ROM_LOAD( "sndprom.bin", 0x0b80, 0x0020, CRC(b369a6ae) SHA1(dda7c6cf58ce5173f29a3084c85393c0c4587086) )
	ROM_LOAD( "pr-1119.bin", 0x0c00, 0x0200, CRC(628d3f1d) SHA1(570f8999603d02e52106ec2df203f63edfb883dd) )	/* timing - not used */
	ROM_LOAD( "pr-1120.bin", 0x0e00, 0x0200, CRC(591b6a68) SHA1(85de39d9e14a1f0c65c1308c27a106e3c2dd9b5b) )	/* timing - not used */
ROM_END


ROM_START( turboa )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "epr1262.rom",  0x0000, 0x2000, CRC(1951b83a) SHA1(31933676140db66281b7ca016a1b42cb985f44dd) )
	ROM_LOAD( "epr1263.rom",  0x2000, 0x2000, CRC(45e01608) SHA1(0a9812714c41904bef7a8777b4aae63b5a1dd633) )
	ROM_LOAD( "epr1264.rom",  0x4000, 0x2000, CRC(1802f6c7) SHA1(5c575821d849d955059868b3dd3167b4bef9a8c4) )

	ROM_REGION( 0x20000, REGION_GFX1, 0 )	/* sprite data */
	ROM_LOAD( "epr1246.rom", 0x00000, 0x2000, CRC(555bfe9a) SHA1(1e56385475eeff044dcd9b44a154991d3efe995e) )	/* level 0 */
	ROM_RELOAD(				 0x02000, 0x2000 )
	ROM_LOAD( "epr1247.rom", 0x04000, 0x2000, CRC(c8c5e4d5) SHA1(da70297340ddea0cd7fe04f2d94ea65f8202d0e5) )	/* level 1 */
	ROM_RELOAD(				 0x06000, 0x2000 )
	ROM_LOAD( "epr1248.rom", 0x08000, 0x2000, CRC(82fe5b94) SHA1(b96688ca0cfd90fdc4ee7c2e6c0b66726cc5713c) )	/* level 2 */
	ROM_RELOAD(				 0x0a000, 0x2000 )
	ROM_LOAD( "epr1249.rom", 0x0c000, 0x2000, CRC(e258e009) SHA1(598d382db0f789ea2fde749b7467abed545de25a) )	/* level 3 */
	ROM_LOAD( "epr1250.rom", 0x0e000, 0x2000, CRC(aee6e05e) SHA1(99b9b1ec996746ddf713ed38192f350f1f32a847) )
	ROM_LOAD( "epr1251.rom", 0x10000, 0x2000, CRC(292573de) SHA1(3ddc980d11478a6a6e4082c2f76c1ab82ffe2f36) )	/* level 4 */
	ROM_LOAD( "epr1252.rom", 0x12000, 0x2000, CRC(aee6e05e) SHA1(99b9b1ec996746ddf713ed38192f350f1f32a847) )
	ROM_LOAD( "epr1253.rom", 0x14000, 0x2000, CRC(92783626) SHA1(13979eb964112436182d2a92f21803bcc28f4a4a) )	/* level 5 */
	ROM_LOAD( "epr1254.rom", 0x16000, 0x2000, CRC(aee6e05e) SHA1(99b9b1ec996746ddf713ed38192f350f1f32a847) )
	ROM_LOAD( "epr1255.rom", 0x18000, 0x2000, CRC(485dcef9) SHA1(0f760ebb42cc2580a29758c72428a41d74477ce6) )	/* level 6 */
	ROM_LOAD( "epr1256.rom", 0x1a000, 0x2000, CRC(aee6e05e) SHA1(99b9b1ec996746ddf713ed38192f350f1f32a847) )
	ROM_LOAD( "epr1257.rom", 0x1c000, 0x2000, CRC(4ca984ce) SHA1(99f294fb203f23929b44baa2dd1825c67dde08a1) )	/* level 7 */
	ROM_LOAD( "epr1258.rom", 0x1e000, 0x2000, CRC(aee6e05e) SHA1(99b9b1ec996746ddf713ed38192f350f1f32a847) )

	ROM_REGION( 0x1000, REGION_GFX2, 0 )	/* foreground data */
	ROM_LOAD( "epr1244.rom", 0x0000, 0x0800, CRC(17f67424) SHA1(6126562510f1509f3487faaa3b9d7470ab600a2c) )
	ROM_LOAD( "epr1245.rom", 0x0800, 0x0800, CRC(2ba0b46b) SHA1(5d4d4f19ad7a911c7b37db190a420faf665546b4) )

	ROM_REGION( 0x4800, REGION_GFX3, 0 )	/* road data */
	ROM_LOAD( "epr1125.rom", 0x0000, 0x0800, CRC(65b5d44b) SHA1(bbdd5db013c9d876e9666f17c48569c7531bfc08) )
	ROM_LOAD( "epr1126.rom", 0x0800, 0x0800, CRC(685ace1b) SHA1(99c8d36ac910169b27676d18c894433c2ba44853) )
	ROM_LOAD( "epr1127.rom", 0x1000, 0x0800, CRC(9233c9ca) SHA1(cbf9a0f564d8ace1ccd701c1769dbc001d465851) )
	ROM_LOAD( "epr1238.rom", 0x1800, 0x0800, CRC(d94fd83f) SHA1(1e3a68259d2ede623d5a7306fdf693a4eab301f0) )
	ROM_LOAD( "epr1239.rom", 0x2000, 0x0800, CRC(4c41124f) SHA1(d73a9441552c77fb3078553195794311a950d589) )
	ROM_LOAD( "epr1240.rom", 0x2800, 0x0800, CRC(371d6282) SHA1(f5902b357d976822d46aa6404b7bd30855d435a9) )
	ROM_LOAD( "epr1241.rom", 0x3000, 0x0800, CRC(1109358a) SHA1(27a5351a4e87309671e72115299420315a93dba6) )
	ROM_LOAD( "epr1242.rom", 0x3800, 0x0800, CRC(04866769) SHA1(1f9c0d53766fdaf8de57d3df05f291c2ca3dc5fb) )
	ROM_LOAD( "epr1243.rom", 0x4000, 0x0800, CRC(29854c48) SHA1(cab89bc30f83d9746931ddf6f95a6d0c8a517e5d) )

	ROM_REGION( 0x200, REGION_GFX4, 0 )		/* number data (copied at init time) */

	ROM_REGION( 0x1000, REGION_PROMS, 0 )	/* various PROMs */
	ROM_LOAD( "pr1121.bin",	 0x0000, 0x0200, CRC(7692f497) SHA1(42468c0705df9928e15ff8deb7e793a6c0c04353) )	/* palette */
	ROM_LOAD( "pr1122.bin",	 0x0200, 0x0400, CRC(1a86ce70) SHA1(cab708b9a089b2e28f2298c1e4fae6e200923527) )	/* sprite priorities */
	ROM_LOAD( "pr1123.bin",	 0x0600, 0x0400, CRC(02d2cb52) SHA1(c34d6b60355747ce20fcb8d322df0e188d187f10) )	/* sprite/road/background priorities */
	ROM_LOAD( "pr-1118.bin", 0x0a00, 0x0100, CRC(07324cfd) SHA1(844abc2042d6810fa34d84ff1ed57744886c6ea6) )	/* background color table */
	ROM_LOAD( "pr1114.bin",	 0x0b00, 0x0020, CRC(78aded46) SHA1(c78afe804f8b8e837b0c502de5b8715a41fb92b9) )	/* road red/green color table */
	ROM_LOAD( "pr1117.bin",	 0x0b20, 0x0020, CRC(f06d9907) SHA1(f11db7800f41b03e79f5eef8d7ef3ae0a6277518) )	/* road green/blue color table */
	ROM_LOAD( "pr1115.bin",	 0x0b40, 0x0020, CRC(5394092c) SHA1(129ff61104979ff6a3c3af8bf81c04ae9b133c9e) )	/* road collision/enable */
	ROM_LOAD( "pr1116.bin",	 0x0b60, 0x0020, CRC(3956767d) SHA1(073aaf57175526660fcf7af2e16e7f1d1aaba9a9) )	/* collision detection */
	ROM_LOAD( "sndprom.bin", 0x0b80, 0x0020, CRC(b369a6ae) SHA1(dda7c6cf58ce5173f29a3084c85393c0c4587086) )
	ROM_LOAD( "pr-1119.bin", 0x0c00, 0x0200, CRC(628d3f1d) SHA1(570f8999603d02e52106ec2df203f63edfb883dd) )	/* timing - not used */
	ROM_LOAD( "pr-1120.bin", 0x0e00, 0x0200, CRC(591b6a68) SHA1(85de39d9e14a1f0c65c1308c27a106e3c2dd9b5b) )	/* timing - not used */
ROM_END


ROM_START( turbob )
	ROM_REGION( 0x10000, REGION_CPU1, 0 ) /* 64k for code */
	ROM_LOAD( "epr-1363.cpu",  0x0000, 0x2000, CRC(5c110fb6) SHA1(fdcdf488bd112db12aa22c4b7e9f34004185d4ce) )
	ROM_LOAD( "epr-1364.cpu",  0x2000, 0x2000, CRC(6a341693) SHA1(428927c4a14bf82225875012c255d25dcffaf2ab) )
	ROM_LOAD( "epr-1365.cpu",  0x4000, 0x2000, CRC(3b6b0dc8) SHA1(3ebfa3f9fabd444ee105591acb6984b6b3523725) )

	ROM_REGION( 0x20000, REGION_GFX1, 0 ) /* sprite data */
	ROM_LOAD( "epr1246.rom", 0x00000, 0x2000, CRC(555bfe9a) SHA1(1e56385475eeff044dcd9b44a154991d3efe995e) )	/* level 0 */
	ROM_RELOAD(				 0x02000, 0x2000 )
	ROM_LOAD( "mpr1290.rom", 0x04000, 0x2000, CRC(95182020) SHA1(cd392a311da222727ce92801cb9d926ccdb08797) )	/* is this good? */
	ROM_RELOAD(				 0x06000, 0x2000 )
	ROM_LOAD( "epr1248.rom", 0x08000, 0x2000, CRC(82fe5b94) SHA1(b96688ca0cfd90fdc4ee7c2e6c0b66726cc5713c) )	/* level 2 */
	ROM_RELOAD(				 0x0a000, 0x2000 )
	ROM_LOAD( "mpr1291.rom", 0x0c000, 0x2000, CRC(0e857f82) SHA1(fbf0dcd11fd4fa09235c3f05d8e284b7dcc8f303) )	/* is this good? */
	ROM_LOAD( "epr1250.rom", 0x0e000, 0x2000, CRC(aee6e05e) SHA1(99b9b1ec996746ddf713ed38192f350f1f32a847) )
	ROM_LOAD( "epr1251.rom", 0x10000, 0x2000, CRC(292573de) SHA1(3ddc980d11478a6a6e4082c2f76c1ab82ffe2f36) )	/* level 4 */
	ROM_LOAD( "epr1252.rom", 0x12000, 0x2000, CRC(aee6e05e) SHA1(99b9b1ec996746ddf713ed38192f350f1f32a847) )
	ROM_LOAD( "epr1253.rom", 0x14000, 0x2000, CRC(92783626) SHA1(13979eb964112436182d2a92f21803bcc28f4a4a) )	/* level 5 */
	ROM_LOAD( "epr1254.rom", 0x16000, 0x2000, CRC(aee6e05e) SHA1(99b9b1ec996746ddf713ed38192f350f1f32a847) )
	ROM_LOAD( "epr1255.rom", 0x18000, 0x2000, CRC(485dcef9) SHA1(0f760ebb42cc2580a29758c72428a41d74477ce6) )	/* level 6 */
	ROM_LOAD( "epr1256.rom", 0x1a000, 0x2000, CRC(aee6e05e) SHA1(99b9b1ec996746ddf713ed38192f350f1f32a847) )
	ROM_LOAD( "epr1257.rom", 0x1c000, 0x2000, CRC(4ca984ce) SHA1(99f294fb203f23929b44baa2dd1825c67dde08a1) )	/* level 7 */
	ROM_LOAD( "epr1258.rom", 0x1e000, 0x2000, CRC(aee6e05e) SHA1(99b9b1ec996746ddf713ed38192f350f1f32a847) )

	ROM_REGION( 0x1000, REGION_GFX2, 0 )	/* foreground data */
	ROM_LOAD( "epr1244.rom", 0x0000, 0x0800, CRC(17f67424) SHA1(6126562510f1509f3487faaa3b9d7470ab600a2c) )
	ROM_LOAD( "epr1245.rom", 0x0800, 0x0800, CRC(2ba0b46b) SHA1(5d4d4f19ad7a911c7b37db190a420faf665546b4) )

	ROM_REGION( 0x4800, REGION_GFX3, 0 )	/* road data */
	ROM_LOAD( "epr1125.rom", 0x0000, 0x0800, CRC(65b5d44b) SHA1(bbdd5db013c9d876e9666f17c48569c7531bfc08) )
	ROM_LOAD( "epr1126.rom", 0x0800, 0x0800, CRC(685ace1b) SHA1(99c8d36ac910169b27676d18c894433c2ba44853) )
	ROM_LOAD( "epr1127.rom", 0x1000, 0x0800, CRC(9233c9ca) SHA1(cbf9a0f564d8ace1ccd701c1769dbc001d465851) )
	ROM_LOAD( "epr1238.rom", 0x1800, 0x0800, CRC(d94fd83f) SHA1(1e3a68259d2ede623d5a7306fdf693a4eab301f0) )
	ROM_LOAD( "epr1239.rom", 0x2000, 0x0800, CRC(4c41124f) SHA1(d73a9441552c77fb3078553195794311a950d589) )
	ROM_LOAD( "epr1240.rom", 0x2800, 0x0800, CRC(371d6282) SHA1(f5902b357d976822d46aa6404b7bd30855d435a9) )
	ROM_LOAD( "epr1241.rom", 0x3000, 0x0800, CRC(1109358a) SHA1(27a5351a4e87309671e72115299420315a93dba6) )
	ROM_LOAD( "epr1242.rom", 0x3800, 0x0800, CRC(04866769) SHA1(1f9c0d53766fdaf8de57d3df05f291c2ca3dc5fb) )
	ROM_LOAD( "epr1243.rom", 0x4000, 0x0800, CRC(29854c48) SHA1(cab89bc30f83d9746931ddf6f95a6d0c8a517e5d) )

	ROM_REGION( 0x200, REGION_GFX4, 0 )		/* number data (copied at init time) */

	ROM_REGION( 0x1000, REGION_PROMS, 0 )	/* various PROMs */
	ROM_LOAD( "pr1121.bin",	 0x0000, 0x0200, CRC(7692f497) SHA1(42468c0705df9928e15ff8deb7e793a6c0c04353) )	/* palette */
	ROM_LOAD( "pr1122.bin",	 0x0200, 0x0400, CRC(1a86ce70) SHA1(cab708b9a089b2e28f2298c1e4fae6e200923527) )	/* sprite priorities */
	ROM_LOAD( "pr1123.bin",	 0x0600, 0x0400, CRC(02d2cb52) SHA1(c34d6b60355747ce20fcb8d322df0e188d187f10) )	/* sprite/road/background priorities */
	ROM_LOAD( "pr-1118.bin", 0x0a00, 0x0100, CRC(07324cfd) SHA1(844abc2042d6810fa34d84ff1ed57744886c6ea6) )	/* background color table */
	ROM_LOAD( "pr1114.bin",	 0x0b00, 0x0020, CRC(78aded46) SHA1(c78afe804f8b8e837b0c502de5b8715a41fb92b9) )	/* road red/green color table */
	ROM_LOAD( "pr1117.bin",	 0x0b20, 0x0020, CRC(f06d9907) SHA1(f11db7800f41b03e79f5eef8d7ef3ae0a6277518) )	/* road green/blue color table */
	ROM_LOAD( "pr1115.bin",	 0x0b40, 0x0020, CRC(5394092c) SHA1(129ff61104979ff6a3c3af8bf81c04ae9b133c9e) )	/* road collision/enable */
	ROM_LOAD( "pr1116.bin",	 0x0b60, 0x0020, CRC(3956767d) SHA1(073aaf57175526660fcf7af2e16e7f1d1aaba9a9) )	/* collision detection */
	ROM_LOAD( "sndprom.bin", 0x0b80, 0x0020, CRC(b369a6ae) SHA1(dda7c6cf58ce5173f29a3084c85393c0c4587086) )
	ROM_LOAD( "pr-1119.bin", 0x0c00, 0x0200, CRC(628d3f1d) SHA1(570f8999603d02e52106ec2df203f63edfb883dd) )	/* timing - not used */
	ROM_LOAD( "pr-1120.bin", 0x0e00, 0x0200, CRC(591b6a68) SHA1(85de39d9e14a1f0c65c1308c27a106e3c2dd9b5b) )	/* timing - not used */
ROM_END


ROM_START( subroc3d )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "epr1614a.88", 0x0000, 0x2000, CRC(0ed856b4) SHA1(c2f48170365a53bff312ca20df5b74466de6349a) )
	ROM_LOAD( "epr1615.87",  0x2000, 0x2000, CRC(6281eb2e) SHA1(591d7f184f51f33fb583c916eddacf4581d612d7) )
	ROM_LOAD( "epr1616.86",  0x4000, 0x2000, CRC(cc7b0c9b) SHA1(0b44c9a2421a51bdc16a2b590f24fbbfb47ef86f) )

	ROM_REGION( 0x40000, REGION_GFX1, 0 )	/* sprite data */
	ROM_LOAD( "epr1417.29",  0x00000, 0x2000, CRC(2aaff4e0) SHA1(4b4e4f65d63fb9648108c5f01248ffcb3b4bc54f) )	/* level 0 */
	ROM_LOAD( "epr1418.30",  0x02000, 0x2000, CRC(41ff0f15) SHA1(c441c5368a3faf2544d617e1ceb5cb8eac23017d) )
	ROM_LOAD( "epr1419.55",  0x08000, 0x2000, CRC(37ac818c) SHA1(26b15f410c6a6dcde498e20cece973d5ba23b0de) )	/* level 1 */
	ROM_LOAD( "epr1420.56",  0x0a000, 0x2000, CRC(41ff0f15) SHA1(c441c5368a3faf2544d617e1ceb5cb8eac23017d) )
	ROM_LOAD( "epr1422.81",  0x10000, 0x2000, CRC(0221db58) SHA1(8a157168610bf867a038229ad345de8f95741d04) )	/* level 2 */
	ROM_LOAD( "epr1423.82",  0x12000, 0x2000, CRC(08b1a4b8) SHA1(8e64228911863bf93fdf8a17a2ddca739fb20cd6) )
	ROM_LOAD( "epr1421.80",  0x16000, 0x2000, CRC(1db33c09) SHA1(1b2ec0c15fb178bed7cd2c877a6679ac6c59955c) )
	ROM_LOAD( "epr1425.107", 0x18000, 0x2000, CRC(0221db58) SHA1(8a157168610bf867a038229ad345de8f95741d04) )	/* level 3 */
	ROM_LOAD( "epr1426.108", 0x1a000, 0x2000, CRC(08b1a4b8) SHA1(8e64228911863bf93fdf8a17a2ddca739fb20cd6) )
	ROM_LOAD( "epr1424.106", 0x1e000, 0x2000, CRC(1db33c09) SHA1(1b2ec0c15fb178bed7cd2c877a6679ac6c59955c) )
	ROM_LOAD( "epr1664.116", 0x20000, 0x2000, CRC(6c93ece7) SHA1(b6523f08862f70743422283d7d46e226994add8c) )	/* level 4 */
	ROM_LOAD( "epr1427.115", 0x22000, 0x2000, CRC(2f8cfc2d) SHA1(1ee1b57cf7133aee5c12d654112883af36dff2fa) )
	ROM_LOAD( "epr1429.117", 0x26000, 0x2000, CRC(80e649c7) SHA1(433c847e05a072af8fd7a4d1f50ad856f569c0a6) )
	ROM_LOAD( "epr1665.90",  0x28000, 0x2000, CRC(6c93ece7) SHA1(b6523f08862f70743422283d7d46e226994add8c) )	/* level 5 */
	ROM_LOAD( "epr1430.89",  0x2a000, 0x2000, CRC(2f8cfc2d) SHA1(1ee1b57cf7133aee5c12d654112883af36dff2fa) )
	ROM_LOAD( "epr1432.91",  0x2e000, 0x2000, CRC(d9cd98d0) SHA1(4e1c135ea19375c6a97aac3d134572a45972c56a) )
	ROM_LOAD( "epr1666.64",  0x30000, 0x2000, CRC(6c93ece7) SHA1(b6523f08862f70743422283d7d46e226994add8c) )	/* level 6 */
	ROM_LOAD( "epr1433.63",  0x32000, 0x2000, CRC(2f8cfc2d) SHA1(1ee1b57cf7133aee5c12d654112883af36dff2fa) )
	ROM_LOAD( "epr1436.66",  0x34000, 0x2000, CRC(fc4ad926) SHA1(bf6659ac9eaf5e85bc73848ab4e0c6c7413b55a8) )
	ROM_LOAD( "epr1435.65",  0x36000, 0x2000, CRC(40662eef) SHA1(23bf268ea93288af90bd0e8d6f506a5b92490829) )
	ROM_LOAD( "epr1438.38",  0x38000, 0x2000, CRC(d563d4c1) SHA1(81ebb65c3c0a44aaddf6895a80533436b87a15c7) )	/* level 7 */
	ROM_LOAD( "epr1437.37",  0x3a000, 0x2000, CRC(18ba6aad) SHA1(b959f09739909b835d790928f35b7f7e6bd52c31) )
	ROM_LOAD( "epr1440.40",  0x3c000, 0x2000, CRC(3a0e659c) SHA1(51e64b2417cf3b599aa9ecc84457462a5dca2a61) )
	ROM_LOAD( "epr1439.39",  0x3e000, 0x2000, CRC(3d051668) SHA1(aa4f6152235f07ad39019c46dfacf69d70a7fdcc) )

	ROM_REGION( 0x01000, REGION_GFX2, 0 )	/* foreground data */
	ROM_LOAD( "epr1618.82",  0x0000, 0x0800, CRC(a25fea71) SHA1(283efee3951d081119d756114f9f49c2996de5f2) )
	ROM_LOAD( "epr1617.83",  0x0800, 0x0800, CRC(f70c678e) SHA1(1fabf0011fa4fefd29daf18d4ed6b2cbec14e7b7) )

	ROM_REGION( 0x01000, REGION_PROMS, 0 )	/* various PROMs */
	ROM_LOAD( "pr1419.108", 0x00000, 0x0200, CRC(2cfa2a3f) SHA1(7e2ed2f4ef3324c41da153828c7976e7ba91af7c) )  /* color prom */
	ROM_LOAD( "pr1620.62",  0x00200, 0x0100, CRC(0ab7ef09) SHA1(b89f8889e2c1220b381e1d6ecc4105cb4152e350) )  /* char color palette */
	ROM_LOAD( "pr1449.14",  0x00300, 0x0200, CRC(5eb9ff47) SHA1(b8b1e7cfb8aa380663684df6090c48c7c57a6d50) )  /* ??? */
	ROM_LOAD( "pr1450.21",  0x00500, 0x0200, CRC(66bdb00c) SHA1(3956647b27a73770bd163eb7ad29fcd9243dac83) )  /* sprite priority */
	ROM_LOAD( "pr1451.58",  0x00700, 0x0200, CRC(6a575261) SHA1(79f690db671e471153cbdf1939e733da74fcdc08) )  /* ??? */
	ROM_LOAD( "pr1453.39",  0x00900, 0x0020, CRC(181c6d23) SHA1(4749b205cbaa513ee65a644946235d2cfe275648) )  /* ??? */
	ROM_LOAD( "pr1454.67",  0x00920, 0x0020, CRC(dc683440) SHA1(8469914d364dc8f9d0839cae3c864de3b2f3c8df) )  /* ??? */
ROM_END


ROM_START( buckrog )
	ROM_REGION( 2*0x10000, REGION_CPU1, 0 ) /* 64k for code + 64k for decrypted opcodes */
	ROM_LOAD( "br-3.bin", 0x0000, 0x4000, CRC(f0055e97) SHA1(f6ee2afd6fef710949087d1cb04cbc242d1fa9f5) )	/* encrypted */
	ROM_LOAD( "br-4.bin", 0x4000, 0x4000, CRC(7d084c39) SHA1(ef2c0a2a59e14d9e196fd3837139fc5acf0f63be) )	/* encrypted */

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for code */
	ROM_LOAD( "5200.66", 0x0000, 0x1000, CRC(0d58b154) SHA1(9f3951eb7ea1fa9ff914738462e4b4f755d60802) )

	ROM_REGION( 0x40000, REGION_GFX1, 0 ) /* sprite data */
	ROM_LOAD( "5216.100",  0x00000, 0x2000, CRC(8155bd73) SHA1(b6814f03eafe16457655598685b4827456b86335) )	/* level 0 */
	ROM_LOAD( "5213.84",   0x08000, 0x2000, CRC(fd78dda4) SHA1(4328b5782cbe692765eac43a8eba40bdf2e41921) )	/* level 1 */
	ROM_LOAD( "ic68",      0x10000, 0x4000, CRC(2a194270) SHA1(8d4e444bd8a4e2fa32099787849e6c02cffe49b0) )	/* level 2 */
	ROM_LOAD( "ic52",      0x18000, 0x4000, CRC(b31a120f) SHA1(036cdf56cb43b892609a8f793d5ca66940bf128e) )	/* level 3 */
	ROM_LOAD( "ic43",      0x20000, 0x4000, CRC(d3584926) SHA1(7ad410ad84447a3edba2c51c4ec4314a117fffe7) )	/* level 4 */
	ROM_LOAD( "ic59",      0x28000, 0x4000, CRC(d83c7fcf) SHA1(4c4a590762ef87a3057a12e8d4310decbeb8613c) )	/* level 5 */
	ROM_LOAD( "5208.58",   0x2c000, 0x2000, CRC(d181fed2) SHA1(fd46e609b7e04d0661c84ad0faa616d75b8ba89f) )
	ROM_LOAD( "ic75",      0x30000, 0x4000, CRC(1bd6e453) SHA1(472fbc7add05b96e368b961c5ef7ef27f3896216) )	/* level 6 */
	ROM_LOAD( "5239.74",   0x34000, 0x2000, CRC(c34e9b82) SHA1(9e69fe9dcc631783e43abe356657f3c6a6a533d8) )
	ROM_LOAD( "ic91",      0x38000, 0x4000, CRC(221f4ced) SHA1(07498c9105c4c4589b19c2bc36abafb176de7bda) )	/* level 7 */
	ROM_LOAD( "5238.90",   0x3c000, 0x2000, CRC(7aff0886) SHA1(09ed9fa973257bb23b488e02ef9e02d867e4c366) )

	ROM_REGION( 0x01000, REGION_GFX2, 0 )	/* foreground data */
	ROM_LOAD( "5201.102",  0x0000, 0x0800, CRC(7f21b0a4) SHA1(b6d784031ffecb36863ae1d81eeaaf8f76ab83df) )
	ROM_LOAD( "5202.103",  0x0800, 0x0800, CRC(43f3e5a7) SHA1(2714943b6720311c5d226db3b6fe95d072677793) )

	ROM_REGION( 0x2000, REGION_GFX3, 0 )	/* background color data */
	ROM_LOAD( "5203.91", 0x0000, 0x2000, CRC(631f5b65) SHA1(ce8b23cf97f7e08a13f426964ef140a20a884335) )

	ROM_REGION( 0x0600, REGION_PROMS, 0 )	/* various PROMs */
	ROM_LOAD( "ic95",    0x0000, 0x0400, CRC(45e997a8) SHA1(023703b90b503310351b12157b1e732e61430fa5) ) /* sprite colortable */
	ROM_LOAD( "5198.93", 0x0400, 0x0200, CRC(32e74bc8) SHA1(dd2c812efd7b8f6b31a45e698d6453ea6bec132e) ) /* char colortable */
ROM_END


ROM_START( buckrogn )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "ic3", 0x0000, 0x4000, CRC(7f1910af) SHA1(22d37750282676d8fd1f602e928c174f823245c9) )
	ROM_LOAD( "ic4", 0x4000, 0x4000, CRC(5ecd393b) SHA1(d069f12326644f2c685e516d91d33b97ec162c56) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for code */
	ROM_LOAD( "5200.66", 0x0000, 0x1000, CRC(0d58b154) SHA1(9f3951eb7ea1fa9ff914738462e4b4f755d60802) )

	ROM_REGION( 0x40000, REGION_GFX1, 0 ) /* sprite data */
	ROM_LOAD( "5216.100",  0x00000, 0x2000, CRC(8155bd73) SHA1(b6814f03eafe16457655598685b4827456b86335) )	/* level 0 */
	ROM_LOAD( "5213.84",   0x08000, 0x2000, CRC(fd78dda4) SHA1(4328b5782cbe692765eac43a8eba40bdf2e41921) )	/* level 1 */
	ROM_LOAD( "ic68",      0x10000, 0x4000, CRC(2a194270) SHA1(8d4e444bd8a4e2fa32099787849e6c02cffe49b0) )	/* level 2 */
	ROM_LOAD( "ic52",      0x18000, 0x4000, CRC(b31a120f) SHA1(036cdf56cb43b892609a8f793d5ca66940bf128e) )	/* level 3 */
	ROM_LOAD( "ic43",      0x20000, 0x4000, CRC(d3584926) SHA1(7ad410ad84447a3edba2c51c4ec4314a117fffe7) )	/* level 4 */
	ROM_LOAD( "ic59",      0x28000, 0x4000, CRC(d83c7fcf) SHA1(4c4a590762ef87a3057a12e8d4310decbeb8613c) )	/* level 5 */
	ROM_LOAD( "5208.58",   0x2c000, 0x2000, CRC(d181fed2) SHA1(fd46e609b7e04d0661c84ad0faa616d75b8ba89f) )
	ROM_LOAD( "ic75",      0x30000, 0x4000, CRC(1bd6e453) SHA1(472fbc7add05b96e368b961c5ef7ef27f3896216) )	/* level 6 */
	ROM_LOAD( "5239.74",   0x34000, 0x2000, CRC(c34e9b82) SHA1(9e69fe9dcc631783e43abe356657f3c6a6a533d8) )
	ROM_LOAD( "ic91",      0x38000, 0x4000, CRC(221f4ced) SHA1(07498c9105c4c4589b19c2bc36abafb176de7bda) )	/* level 7 */
	ROM_LOAD( "5238.90",   0x3c000, 0x2000, CRC(7aff0886) SHA1(09ed9fa973257bb23b488e02ef9e02d867e4c366) )

	ROM_REGION( 0x01000, REGION_GFX2, 0 )	/* foreground data */
	ROM_LOAD( "5201.102",  0x0000, 0x0800, CRC(7f21b0a4) SHA1(b6d784031ffecb36863ae1d81eeaaf8f76ab83df) )
	ROM_LOAD( "5202.103",  0x0800, 0x0800, CRC(43f3e5a7) SHA1(2714943b6720311c5d226db3b6fe95d072677793) )

	ROM_REGION( 0x2000, REGION_GFX3, 0 )	/* background color data */
	ROM_LOAD( "5203.91", 0x0000, 0x2000, CRC(631f5b65) SHA1(ce8b23cf97f7e08a13f426964ef140a20a884335) )

	ROM_REGION( 0x0600, REGION_PROMS, 0 )	/* various PROMs */
	ROM_LOAD( "ic95",    0x0000, 0x0400, CRC(45e997a8) SHA1(023703b90b503310351b12157b1e732e61430fa5) ) /* sprite colortable */
	ROM_LOAD( "5198.93", 0x0400, 0x0200, CRC(32e74bc8) SHA1(dd2c812efd7b8f6b31a45e698d6453ea6bec132e) ) /* char colortable */
ROM_END



/*************************************
 *
 *	Driver init
 *
 *************************************/

static DRIVER_INIT( decode_turbo )
{
	turbo_rom_decode();
}


static DRIVER_INIT( decode_buckrog )
{
	buckrog_decode();
}



/*************************************
 *
 *	Game drivers
 *
 *************************************/

GAMEX( 1981, turbo,    0,       turbo,    turbo,    0,              ROT270,             "Sega", "Turbo", GAME_NO_COCKTAIL )
GAMEX( 1981, turboa,   turbo,   turbo,    turbo,    decode_turbo,   ROT270,             "Sega", "Turbo (encrypted set 1)", GAME_NO_COCKTAIL )
GAMEX( 1981, turbob,   turbo,   turbo,    turbo,    decode_turbo,   ROT270,             "Sega", "Turbo (encrypted set 2)", GAME_NO_COCKTAIL )
GAME ( 1982, subroc3d, 0,       subroc3d, subroc3d, 0,              ORIENTATION_FLIP_X, "Sega", "Subroc-3D" )
GAME ( 1982, buckrog,  0,       buckrog,  buckrog,  decode_buckrog, ROT0,               "Sega", "Buck Rogers - Planet of Zoom" )
GAME ( 1982, buckrogn, buckrog, buckrog,  buckrog,  0,              ROT0,               "Sega", "Buck Rogers - Planet of Zoom (not encrypted)" )
