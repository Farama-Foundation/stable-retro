/**************************************************************************

Blood Bros, West Story [+Sky Smasher]
TAD Corporation 1990
68000 + Z80 + YM3931 + YM3812

driver by Carlos A. Lozano Baides

Coin inputs are handled by the sound CPU, so they don't work with sound
disabled. Use the service switch instead.

TODO:
West Story:
- sound
- some bad sprites, probably bad ROMs.
- tilemap scroll


Sky Smasher  (c) 1990 Nihon System [Seibu hardware]
-----------

Like some other Seibu hardware games, hold P1 right at boot to
view DIP descriptions.

Game does not appear to have cocktail mode. The screen hardware
is undoubtedly capable of flipscreen and layer priority flipping
however.

Dumpers Notes
=============

PCB is made by Seibu

Sound           - Z80
                - YM3931 SEI0100BU    (64 pin DIP)
                - YM3812
                - M6295

GFX             - SEI0210   custom    (128 pin PQFP)
                - SEI0220BP custom    (80 pin PQFP)
                - SEI0200   custom    (100 pin PQFP)
                - SEI0160   custom    (60 pin PQFP)


Stephh's notes (based on the games M68000 code and some tests) :

1) 'bloodbro'

  - When "Starting Coin" Dip Switch is set to "x2", you need 2 coins to start
    a game (but 1 coin to join), then 1 coin to continue.
    However, it you insert 2 coins, and press START2, you start a 2 players game.
    Also note that when "Starting Coin" Dip Switch is set to "x2", SERVICE1
    adds 2 credits instead of 1.

  - Bits 6 and 7 of DSW are told to be unused, but they are tested before
    entering the initials (code at 0x004a1e, 0x014bb0 and 0x014c84)


2) 'weststry'

  - This bootleg has been realised by Datsu in 1991. This "company" also
    bootlegged "Toki" (another TAD game) in 1990.

  - When "Starting Coin" Dip Switch is set to "x2", you need 2 coins to start
    a game (but 1 coin to join), then 1 coin to continue.
    However, it you insert 2 coins, and press START2, you start a 2 players game.

  - Bits 6 and 7 of DSW are told to be unused, but bit 7 is tested when
    entering the initials (code at 0x00497e, 0x014cf2 and 0x014dc4
    Note that bit 8 (!) is tested at 0x0049a2, so this has no effect.
  - Bit 7 of DSW is also "merged" with bit 6 (code at 0x0002f6 and 0x000326).


3) 'skysmash'

  - This game only has 2 buttons : bits 6 and 14 of players inputs are NEVER tested !

  - When "Starting Coin" Dip Switch is set to "x2", you need 2 coins to start
    a game (but 1 coin to join), then 1 coin to continue.
    However, it you insert 2 coins, and press START2, you start a 2 players game.
    Also note that when "Starting Coin" Dip Switch is set to "x2", SERVICE1
    adds 2 credits instead of 1.

  - Bit 6 of DSW was previouly used as a "Cabinet" Dip Switch (OFF = Upright
    and ON = Cocktail), but it isn't tested outside of the "test mode".
    Check code from 0x021abe to 0x021afc (and the "rts" instruction at 0x021adc)
  - Bit 7 of DSW is only tested at 0x02035e and writes a value to 0x0c0100.
    I don't know what it is supposed to do, but it could be a "Flip Screen"
    Dip Switch as in 'toki' (same manufacturer and similar hardware).


**************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "cpu/z80/z80.h"
#include "sndhrdw/seibu.h"

extern VIDEO_UPDATE( bloodbro );
extern VIDEO_UPDATE( weststry );
extern VIDEO_UPDATE( skysmash );
extern VIDEO_START( bloodbro );

WRITE16_HANDLER( bloodbro_bgvideoram_w );
WRITE16_HANDLER( bloodbro_fgvideoram_w );
WRITE16_HANDLER( bloodbro_txvideoram_w );

extern data16_t *bloodbro_bgvideoram,*bloodbro_fgvideoram;
extern data16_t *bloodbro_txvideoram;
extern data16_t *bloodbro_scroll;

/***************************************************************************/

static MEMORY_READ16_START( readmem_cpu )
	{ 0x000000, 0x07ffff, MRA16_ROM },
	{ 0x080000, 0x08afff, MRA16_RAM },
	{ 0x08b000, 0x08bfff, MRA16_RAM },
	{ 0x08c000, 0x08c3ff, MRA16_RAM },
	{ 0x08c400, 0x08cfff, MRA16_RAM },
	{ 0x08d000, 0x08d3ff, MRA16_RAM },
	{ 0x08d400, 0x08d7ff, MRA16_RAM },
	{ 0x08d800, 0x08dfff, MRA16_RAM },
	{ 0x08e000, 0x08e7ff, MRA16_RAM },
	{ 0x08e800, 0x08f7ff, MRA16_RAM },
	{ 0x08f800, 0x08ffff, MRA16_RAM },
	{ 0x0a0000, 0x0a000d, seibu_main_word_r },
	{ 0x0c0000, 0x0c007f, MRA16_RAM },
	{ 0x0e0000, 0x0e0001, input_port_1_word_r },
	{ 0x0e0002, 0x0e0003, input_port_2_word_r },
	{ 0x0e0004, 0x0e0005, input_port_3_word_r },
MEMORY_END

static MEMORY_WRITE16_START( writemem_cpu )
	{ 0x000000, 0x07ffff, MWA16_ROM },
	{ 0x080000, 0x08afff, MWA16_RAM },
	{ 0x08b000, 0x08bfff, MWA16_RAM, &spriteram16, &spriteram_size },
	{ 0x08c000, 0x08c3ff, bloodbro_bgvideoram_w, &bloodbro_bgvideoram },
	{ 0x08c400, 0x08cfff, MWA16_RAM },
	{ 0x08d000, 0x08d3ff, bloodbro_fgvideoram_w, &bloodbro_fgvideoram },
	{ 0x08d400, 0x08d7ff, MWA16_RAM },
	{ 0x08d800, 0x08dfff, bloodbro_txvideoram_w, &bloodbro_txvideoram },
	{ 0x08e000, 0x08e7ff, MWA16_RAM },
	{ 0x08e800, 0x08f7ff, paletteram16_xxxxBBBBGGGGRRRR_word_w, &paletteram16 },
	{ 0x08f800, 0x08ffff, MWA16_RAM },
	{ 0x0a0000, 0x0a000d, seibu_main_word_w },
	{ 0x0c0000, 0x0c007f, MWA16_RAM, &bloodbro_scroll },
	{ 0x0c0080, 0x0c0081, MWA16_NOP }, /* IRQ Ack VBL? */
	{ 0x0c00c0, 0x0c00c1, MWA16_NOP }, /* watchdog? */
/*	{ 0x0c0100, 0x0c0100, MWA16_NOP },  // ?? Written 1 time /*/
MEMORY_END

/**** West Story Memory Map ********************************************/

static MEMORY_READ16_START( weststry_readmem_cpu )
	{ 0x000000, 0x07ffff, MRA16_ROM },
	{ 0x080000, 0x08afff, MRA16_RAM },
	{ 0x08b000, 0x08bfff, MRA16_RAM },
	{ 0x08c000, 0x08c3ff, MRA16_RAM },
	{ 0x08c400, 0x08cfff, MRA16_RAM },
	{ 0x08d000, 0x08d3ff, MRA16_RAM },
	{ 0x08d400, 0x08dfff, MRA16_RAM },
	{ 0x08d800, 0x08dfff, MRA16_RAM },
	{ 0x08e000, 0x08ffff, MRA16_RAM },
	{ 0x0c1000, 0x0c1001, input_port_0_word_r },
	{ 0x0c1002, 0x0c1003, input_port_1_word_r },
	{ 0x0c1004, 0x0c1005, input_port_2_word_r },
	{ 0x0c1000, 0x0c17ff, MRA16_RAM },
	{ 0x128000, 0x1287ff, MRA16_RAM },
	{ 0x120000, 0x128fff, MRA16_RAM },
MEMORY_END

static MEMORY_WRITE16_START( weststry_writemem_cpu )
	{ 0x000000, 0x07ffff, MWA16_ROM },
	{ 0x080000, 0x08afff, MWA16_RAM },
	{ 0x08b000, 0x08bfff, MWA16_RAM, &spriteram16, &spriteram_size },
	{ 0x08c000, 0x08c3ff, bloodbro_bgvideoram_w, &bloodbro_bgvideoram },
	{ 0x08c400, 0x08cfff, MWA16_RAM },
	{ 0x08d000, 0x08d3ff, bloodbro_fgvideoram_w, &bloodbro_fgvideoram },
	{ 0x08d400, 0x08d7ff, MWA16_RAM },
	{ 0x08d800, 0x08dfff, bloodbro_txvideoram_w, &bloodbro_txvideoram },
	{ 0x08e000, 0x08ffff, MWA16_RAM },
	{ 0x0c1000, 0x0c17ff, MWA16_RAM },
	{ 0x128000, 0x1287ff, paletteram16_xxxxBBBBGGGGRRRR_word_w, &paletteram16 },
	{ 0x120000, 0x128fff, MWA16_RAM },
MEMORY_END

/******************************************************************************/

INPUT_PORTS_START( bloodbro )
	SEIBU_COIN_INPUTS	/* Must be port 0: coin inputs read through sound cpu */

	PORT_START
	PORT_DIPNAME( 0x0001, 0x0001, "Coin Mode" )
	PORT_DIPSETTING(      0x0001, "Mode 1" )
	PORT_DIPSETTING(      0x0000, "Mode 2" )
	/* Coin Mode 1 */
	PORT_DIPNAME( 0x001e, 0x001e, DEF_STR( Coinage ) )
	PORT_DIPSETTING(      0x0014, DEF_STR( 6C_1C ) )
	PORT_DIPSETTING(      0x0016, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(      0x0018, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(      0x001a, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( 8C_3C ) )
	PORT_DIPSETTING(      0x001c, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0004, DEF_STR( 5C_3C ) )
	PORT_DIPSETTING(      0x0006, DEF_STR( 3C_2C ) )
	PORT_DIPSETTING(      0x001e, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0008, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(      0x0012, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x0010, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x000e, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(      0x000c, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(      0x000a, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Free_Play ) )
	/* Coin Mode 2
	PORT_DIPNAME( 0x0006, 0x0006, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0004, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0006, DEF_STR( 1C_1C ) )
	PORT_DIPNAME( 0x0018, 0x0018, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(      0x0018, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x0010, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x0008, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 1C_6C ) )
	*/
	PORT_DIPNAME( 0x0020, 0x0020, "Starting Coin" )
	PORT_DIPSETTING(      0x0020, "Normal" )
	PORT_DIPSETTING(      0x0000, "x2" )
	PORT_DIPNAME( 0x0040, 0x0040, DEF_STR( Unknown ) )	/* see notes*/
	PORT_DIPSETTING(      0x0040, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0080, 0x0080, DEF_STR( Unknown ) )	/* see notes*/
	PORT_DIPSETTING(      0x0080, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0300, 0x0300, DEF_STR( Lives ) )
	PORT_DIPSETTING(      0x0000, "1" )
	PORT_DIPSETTING(      0x0200, "2" )
	PORT_DIPSETTING(      0x0300, "3" )
	PORT_DIPSETTING(      0x0100, "5" )
	PORT_DIPNAME( 0x0c00, 0x0c00, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(      0x0c00, "300k then every 500k" )
	PORT_DIPSETTING(      0x0800, "Every 500k" )
	PORT_DIPSETTING(      0x0400, "500k only" )
	PORT_DIPSETTING(      0x0000, "None" )
	PORT_DIPNAME( 0x3000, 0x3000, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(      0x2000, "Easy" )
	PORT_DIPSETTING(      0x3000, "Normal" )
	PORT_DIPSETTING(      0x1000, "Hard" )
	PORT_DIPSETTING(      0x0000, "Hardest" )
	PORT_DIPNAME( 0x4000, 0x4000, "Allow Continue" )
	PORT_DIPSETTING(      0x0000, DEF_STR( No ) )
	PORT_DIPSETTING(      0x4000, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x8000, 0x8000, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x8000, DEF_STR( On ) )

	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )	/* "Fire"*/
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )	/* "Roll"*/
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER1 )	/* "Dynamite"*/
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* tested when "continue" - check code at 0x000598 */
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )	/* "Fire"*/
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )	/* "Roll"*/
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )	/* "Dynamite"*/
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* tested - check code at 0x0005fe - VBKANK ? */

	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x000e, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x00e0, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* tested - check code at 0x000800 */
	PORT_BIT( 0x0e00, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0xe000, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END


INPUT_PORTS_START( weststry )
	PORT_START
	PORT_DIPNAME( 0x0001, 0x0001, "Coin Mode" )
	PORT_DIPSETTING(      0x0001, "Mode 1" )
	PORT_DIPSETTING(      0x0000, "Mode 2" )
	/* Coin Mode 1 */
	PORT_DIPNAME( 0x001e, 0x001e, DEF_STR( Coinage ) )
	PORT_DIPSETTING(      0x0014, DEF_STR( 6C_1C ) )
	PORT_DIPSETTING(      0x0016, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(      0x0018, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(      0x001a, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( 8C_3C ) )
	PORT_DIPSETTING(      0x001c, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0004, DEF_STR( 5C_3C ) )
	PORT_DIPSETTING(      0x0006, DEF_STR( 3C_2C ) )
	PORT_DIPSETTING(      0x001e, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0008, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(      0x0012, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x0010, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x000e, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(      0x000c, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(      0x000a, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Free_Play ) )
	/* Coin Mode 2
	PORT_DIPNAME( 0x0006, 0x0006, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0004, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0006, DEF_STR( 1C_1C ) )
	PORT_DIPNAME( 0x0018, 0x0018, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(      0x0018, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x0010, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x0008, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 1C_6C ) )
	*/
	PORT_DIPNAME( 0x0020, 0x0020, "Starting Coin" )
	PORT_DIPSETTING(      0x0020, "Normal" )
	PORT_DIPSETTING(      0x0000, "x2" )
	PORT_DIPNAME( 0x0040, 0x0040, DEF_STR( Unknown ) )	/* see notes*/
	PORT_DIPSETTING(      0x0040, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0080, 0x0080, DEF_STR( Unknown ) )	/* see notes*/
	PORT_DIPSETTING(      0x0080, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0300, 0x0300, DEF_STR( Lives ) )
	PORT_DIPSETTING(      0x0000, "1" )
	PORT_DIPSETTING(      0x0200, "2" )
	PORT_DIPSETTING(      0x0300, "3" )
	PORT_DIPSETTING(      0x0100, "5" )
	PORT_DIPNAME( 0x0c00, 0x0c00, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(      0x0c00, "300k then every 500k" )
	PORT_DIPSETTING(      0x0800, "Every 500k" )
	PORT_DIPSETTING(      0x0400, "500k only" )
	PORT_DIPSETTING(      0x0000, "None" )
	PORT_DIPNAME( 0x3000, 0x3000, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(      0x2000, "Easy" )
	PORT_DIPSETTING(      0x3000, "Normal" )
	PORT_DIPSETTING(      0x1000, "Hard" )
	PORT_DIPSETTING(      0x0000, "Hardest" )
	PORT_DIPNAME( 0x4000, 0x4000, "Allow Continue" )
	PORT_DIPSETTING(      0x0000, DEF_STR( No ) )
	PORT_DIPSETTING(      0x4000, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x8000, 0x8000, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x8000, DEF_STR( On ) )

	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )	/* "Fire"*/
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )	/* "Roll"*/
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER1 )	/* "Dynamite"*/
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )	/* "Fire"*/
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )	/* "Roll"*/
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )	/* "Dynamite"*/
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x00e0, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0xff00, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END

INPUT_PORTS_START( skysmash )
	SEIBU_COIN_INPUTS	/* Must be port 0: coin inputs read through sound cpu */

	PORT_START
	PORT_DIPNAME( 0x0001, 0x0001, "Coin Mode" )
	PORT_DIPSETTING(      0x0001, "Mode 1" )
	PORT_DIPSETTING(      0x0000, "Mode 2" )
	/* Coin Mode 1 */
	PORT_DIPNAME( 0x001e, 0x001e, DEF_STR( Coinage ) )
	PORT_DIPSETTING(      0x0014, DEF_STR( 6C_1C ) )
	PORT_DIPSETTING(      0x0016, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(      0x0018, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(      0x001a, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( 8C_3C ) )
	PORT_DIPSETTING(      0x001c, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0004, DEF_STR( 5C_3C ) )
	PORT_DIPSETTING(      0x0006, DEF_STR( 3C_2C ) )
	PORT_DIPSETTING(      0x001e, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0008, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(      0x0012, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x0010, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x000e, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(      0x000c, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(      0x000a, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Free_Play ) )
	/* Coin Mode 2
	PORT_DIPNAME( 0x0006, 0x0006, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0004, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0006, DEF_STR( 1C_1C ) )
	PORT_DIPNAME( 0x0018, 0x0018, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(      0x0018, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x0010, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x0008, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 1C_6C ) )
	*/
	PORT_DIPNAME( 0x0020, 0x0020, "Starting Coin" )
	PORT_DIPSETTING(      0x0020, "Normal" )
	PORT_DIPSETTING(      0x0000, "x2" )
	PORT_DIPNAME( 0x0040, 0x0040, DEF_STR( Unused ) )	/* see notes*/
	PORT_DIPSETTING(      0x0040, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0080, 0x0080, DEF_STR( Unknown ) )	/* see notes*/
	PORT_DIPSETTING(      0x0080, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0300, 0x0300, DEF_STR( Lives ) )
	PORT_DIPSETTING(      0x0200, "2" )
	PORT_DIPSETTING(      0x0300, "3" )
	PORT_DIPSETTING(      0x0100, "5" )
	PORT_BITX( 0,         0x0000, IPT_DIPSWITCH_SETTING | IPF_CHEAT, "Infinite", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPNAME( 0x0c00, 0x0c00, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(      0x0c00, "120k then every 200k" )
	PORT_DIPSETTING(      0x0800, "Every 200k" )
	PORT_DIPSETTING(      0x0400, "Every 250k" )
	PORT_DIPSETTING(      0x0000, "200k only" )
	PORT_DIPNAME( 0x3000, 0x3000, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(      0x0000, "Easy" )
	PORT_DIPSETTING(      0x3000, "Normal" )
	PORT_DIPSETTING(      0x2000, "Hard" )
	PORT_DIPSETTING(      0x1000, "Hardest" )
	PORT_DIPNAME( 0x4000, 0x4000, "Allow Continue" )
	PORT_DIPSETTING(      0x0000, DEF_STR( No ) )
	PORT_DIPSETTING(      0x4000, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x8000, 0x8000, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x8000, DEF_STR( On ) )

	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )	/* "Fire"*/
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )	/* "Bomb"*/
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )	/* "Fire"*/
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )	/* "Bomb"*/
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x000e, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x00e0, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0e00, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0xe000, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END


/**** Blood Bros, Skysmash gfx decode ************************************/

static struct GfxLayout textlayout =
{
	8,8,	/* 8*8 characters */
	4096,	/* 4096 characters */
	4,	/* 4 bits per pixel */
	{ 0, 4, 0x10000*8, 0x10000*8+4 },
	{ 3, 2, 1, 0, 8+3, 8+2, 8+1, 8+0},
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16 },
	16*8	/* every char takes 16 consecutive bytes */
};

static struct GfxLayout backlayout =
{
	16,16,	/* 16*16 sprites  */
	4096,	/* 4096 sprites */
	4,	/* 4 bits per pixel */
	{ 8, 12, 0, 4 },
	{ 3, 2, 1, 0, 16+3, 16+2, 16+1, 16+0,
             3+32*16, 2+32*16, 1+32*16, 0+32*16, 16+3+32*16, 16+2+32*16, 16+1+32*16, 16+0+32*16 },
	{ 0*16, 2*16, 4*16, 6*16, 8*16, 10*16, 12*16, 14*16,
			16*16, 18*16, 20*16, 22*16, 24*16, 26*16, 28*16, 30*16 },
	128*8	/* every sprite takes 128 consecutive bytes */
};

static struct GfxLayout spritelayout =
{
	16,16,	/* 16*16 sprites  */
	RGN_FRAC(1,1),
	4,	/* 4 bits per pixel */
	{ 8, 12, 0, 4 },
	{ 3, 2, 1, 0, 16+3, 16+2, 16+1, 16+0,
             3+32*16, 2+32*16, 1+32*16, 0+32*16, 16+3+32*16, 16+2+32*16, 16+1+32*16, 16+0+32*16 },
	{ 0*16, 2*16, 4*16, 6*16, 8*16, 10*16, 12*16, 14*16,
			16*16, 18*16, 20*16, 22*16, 24*16, 26*16, 28*16, 30*16 },
	128*8	/* every sprite takes 128 consecutive bytes */
};

static struct GfxDecodeInfo bloodbro_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0x00000, &textlayout,   0x70*16,  0x10 }, /* Text */
	{ REGION_GFX2, 0x00000, &backlayout,   0x40*16,  0x10 }, /* Background */
	{ REGION_GFX2, 0x80000, &backlayout,   0x50*16,  0x10 }, /* Foreground */
	{ REGION_GFX3, 0x00000, &spritelayout, 0x00*16,  0x10 }, /* Sprites */
	{ -1 }
};

/**** West Story gfx decode *********************************************/

static struct GfxLayout weststry_textlayout =
{
	8,8,	/* 8*8 sprites */
	4096,	/* 4096 sprites */
	4,	/* 4 bits per pixel */
	{ 0, 0x8000*8, 2*0x8000*8, 3*0x8000*8 },
        { 0, 1, 2, 3, 4, 5, 6, 7 },
        { 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8	/* every sprite takes 8 consecutive bytes */
};

static struct GfxLayout weststry_backlayout =
{
	16,16,	/* 16*16 sprites */
	4096,	/* 4096 sprites */
	4,	/* 4 bits per pixel */
	{ 0*0x20000*8, 1*0x20000*8, 2*0x20000*8, 3*0x20000*8 },
	{ 0, 1, 2, 3, 4, 5, 6, 7,
         	16*8+0, 16*8+1, 16*8+2, 16*8+3, 16*8+4, 16*8+5, 16*8+6, 16*8+7},
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
			8*8, 9*8, 10*8, 11*8, 12*8, 13*8, 14*8, 15*8 },
	32*8	/* every sprite takes 32 consecutive bytes */
};

static struct GfxLayout weststry_spritelayout =
{
	16,16,	/* 16*16 sprites */
	8192,	/* 8192 sprites */
	4,	/* 4 bits per pixel */
	{ 0*0x40000*8, 1*0x40000*8, 2*0x40000*8, 3*0x40000*8 },
	{ 0, 1, 2, 3, 4, 5, 6, 7,
         	16*8+0, 16*8+1, 16*8+2, 16*8+3, 16*8+4, 16*8+5, 16*8+6, 16*8+7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
			8*8, 9*8, 10*8, 11*8, 12*8, 13*8, 14*8, 15*8 },
	32*8	/* every sprite takes 32 consecutive bytes */
};

static struct GfxDecodeInfo weststry_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0x00000, &weststry_textlayout,     16*16,  0x10 },
	{ REGION_GFX2, 0x00000, &weststry_backlayout,     48*16,  0x10 },
	{ REGION_GFX2, 0x80000, &weststry_backlayout,     32*16,  0x10 },
	{ REGION_GFX3, 0x00000, &weststry_spritelayout,    0*16,  0x10 },
	{ -1 }
};

/**** Blood Bros Interrupt & Driver Machine  ****************************/

/* Parameters: YM3812 frequency, Oki frequency, Oki memory region */
SEIBU_SOUND_SYSTEM_YM3812_HARDWARE(14318180/4,8000,REGION_SOUND1);

static MACHINE_DRIVER_START( bloodbro )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 10000000) /* 10 MHz */
	MDRV_CPU_MEMORY(readmem_cpu,writemem_cpu)
	MDRV_CPU_VBLANK_INT(irq4_line_hold,1)

	SEIBU_SOUND_SYSTEM_CPU(14318180/4)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)
	MDRV_MACHINE_INIT(seibu_sound_1)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 2*8, 30*8-1)
	MDRV_GFXDECODE(bloodbro_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(2048)

	MDRV_VIDEO_START(bloodbro)
	MDRV_VIDEO_UPDATE(bloodbro)

	/* sound hardware */
	SEIBU_SOUND_SYSTEM_YM3812_INTERFACE
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( skysmash )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 10000000) /* 10 MHz */
	MDRV_CPU_MEMORY(readmem_cpu,writemem_cpu)
	MDRV_CPU_VBLANK_INT(irq2_line_hold,1)

	SEIBU_SOUND_SYSTEM_CPU(14318180/4)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)
	MDRV_MACHINE_INIT(seibu_sound_1)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 2*8, 30*8-1)
	MDRV_GFXDECODE(bloodbro_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(2048)

	MDRV_VIDEO_START(bloodbro)
	MDRV_VIDEO_UPDATE(skysmash)

	/* sound hardware */
	SEIBU_SOUND_SYSTEM_YM3812_INTERFACE
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( weststry )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 10000000) /* 10 MHz */
	MDRV_CPU_MEMORY(weststry_readmem_cpu,weststry_writemem_cpu)
	MDRV_CPU_VBLANK_INT(irq6_line_hold,1)

	SEIBU_SOUND_SYSTEM_CPU(14318180/4)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)
	MDRV_MACHINE_INIT(seibu_sound_1)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(256, 256)
	MDRV_VISIBLE_AREA(0, 255, 16, 239)
	MDRV_GFXDECODE(weststry_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(1024)

	MDRV_VIDEO_START(bloodbro)
	MDRV_VIDEO_UPDATE(weststry)

	/* sound hardware */
	SEIBU_SOUND_SYSTEM_YM3812_INTERFACE
MACHINE_DRIVER_END



ROM_START( bloodbro )
	ROM_REGION( 0x80000, REGION_CPU1, 0 )
	ROM_LOAD16_BYTE( "bb_02.bin",    0x00001, 0x20000, CRC(c0fdc3e4) SHA1(31968f693de2054a0c8ba50a8d44a371dd9c2848) )
	ROM_LOAD16_BYTE( "bb_01.bin",    0x00000, 0x20000, CRC(2d7e0fdf) SHA1(8fe22d8a1ef7d562a475a5b6c98303b0cb1af561) )
	ROM_LOAD16_BYTE( "bb_04.bin",    0x40001, 0x20000, CRC(fd951c2c) SHA1(f4031bf303c67c82f2f78f7456f78382d8c1ac85) )
	ROM_LOAD16_BYTE( "bb_03.bin",    0x40000, 0x20000, CRC(18d3c460) SHA1(93b86af1199f0fedeaf1fe64d27ffede4b819e42) )

	ROM_REGION( 0x20000, REGION_CPU2, 0 )
	ROM_LOAD( "bb_07.bin",    0x000000, 0x08000, CRC(411b94e8) SHA1(6968441f64212c0935afeca68f07deaadf86d614) )
	ROM_CONTINUE(             0x010000, 0x08000 )
	ROM_COPY( REGION_CPU2, 0, 0x018000, 0x08000 )

	ROM_REGION( 0x20000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "bb_05.bin",    0x00000, 0x10000, CRC(04ba6d19) SHA1(7333075c3323756d51917418b5234d785a9bee00) )	/* characters */
	ROM_LOAD( "bb_06.bin",    0x10000, 0x10000, CRC(7092e35b) SHA1(659d30b2e2fd9ffa34a47e98193c8f0a87ac1315) )

	ROM_REGION( 0x100000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "bloodb.bk",   0x00000, 0x100000, CRC(1aa87ee6) SHA1(e7843c1e8a0f3a685f0b5d6e3a2eb3176c410847) )	/* Background+Foreground */

	ROM_REGION( 0x100000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "bloodb.obj",   0x00000, 0x100000, CRC(d27c3952) SHA1(de7306432b682f238b911507ad7aa2fa8acbee80) )	/* sprites */

	ROM_REGION( 0x20000, REGION_SOUND1, 0 )	/* ADPCM samples */
	ROM_LOAD( "bb_08.bin",    0x00000, 0x20000, CRC(deb1b975) SHA1(08f2e9a0a23171201b71d381d091edcd3787c287) )
ROM_END

ROM_START( weststry )
	ROM_REGION( 0x80000, REGION_CPU1, 0 )	/* 64k for cpu code */
	ROM_LOAD16_BYTE( "ws13.bin",    0x00001, 0x20000, CRC(158e302a) SHA1(52cc1bf526424ff025a6b79f3fc7bba4b9bbfcbb) )
	ROM_LOAD16_BYTE( "ws15.bin",    0x00000, 0x20000, CRC(672e9027) SHA1(71cb9fcef04edb972ba88de45d605dcff539ea2d) )
	ROM_LOAD16_BYTE( "bb_04.bin",   0x40001, 0x20000, CRC(fd951c2c) SHA1(f4031bf303c67c82f2f78f7456f78382d8c1ac85) )
	ROM_LOAD16_BYTE( "bb_03.bin",   0x40000, 0x20000, CRC(18d3c460) SHA1(93b86af1199f0fedeaf1fe64d27ffede4b819e42) )

	ROM_REGION( 0x20000, REGION_CPU2, 0 )	/* 64k for sound cpu code */
	ROM_LOAD( "ws17.bin",    0x000000, 0x08000, CRC(e00a8f09) SHA1(e7247ce0ab99d0726f31dee5de5ba33f4ebd183e) )
	ROM_CONTINUE(            0x010000, 0x08000 )
	ROM_COPY( REGION_CPU2, 0, 0x018000, 0x08000 )

	ROM_REGION( 0x20000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "ws09.bin",    0x00000, 0x08000, CRC(f05b2b3e) SHA1(6570d795d68655ace9668f32dc0bf5c2d2372411) )	/* characters */
	ROM_CONTINUE(            0x00000, 0x8000 )
	ROM_LOAD( "ws11.bin",    0x08000, 0x08000, CRC(2b10e3d2) SHA1(0f5045615b44e2300745fd3afac7f1441352cca5) )
	ROM_CONTINUE(            0x08000, 0x8000 )
	ROM_LOAD( "ws10.bin",    0x10000, 0x08000, CRC(efdf7c82) SHA1(65392697f56473cfe90d9733b9c49f2da6f9b7e6) )
	ROM_CONTINUE(            0x10000, 0x8000 )
	ROM_LOAD( "ws12.bin",    0x18000, 0x08000, CRC(af993578) SHA1(b250b562deeab3bb2c79002e5e1f0b6e17986848) )
	ROM_CONTINUE(            0x18000, 0x8000 )

	ROM_REGION( 0x100000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "ws05.bin",    0x00000, 0x20000, CRC(007c8dc0) SHA1(f44576da3b89d6a889fdb564825ac6ce3bb4cffe) )	/* Background */
	ROM_LOAD( "ws07.bin",    0x20000, 0x20000, CRC(0f0c8d9a) SHA1(f5fe9b5ee4c8ffd7caf5313d13fb5f6e181ed9b6) )
	ROM_LOAD( "ws06.bin",    0x40000, 0x20000, CRC(459d075e) SHA1(24cd0bffe7c5bbccf653ced0b73579059603d187) )
	ROM_LOAD( "ws08.bin",    0x60000, 0x20000, CRC(4d6783b3) SHA1(9870fe9570afeff179b6080581fd6bb187898ff0) )
	ROM_LOAD( "ws01.bin",    0x80000, 0x20000, CRC(32bda4bc) SHA1(ed0c0740c7af513b341b2b7ff3e0bf6045e930e9) )	/* Foreground */
	ROM_LOAD( "ws03.bin",    0xa0000, 0x20000, CRC(046b51f8) SHA1(25af752caebdec762582fc0130cf14546110bb54) )
	ROM_LOAD( "ws02.bin",    0xc0000, 0x20000, CRC(ed9d682e) SHA1(0f79ea09a7af367d175081f72f2bc94f6caad463) )
	ROM_LOAD( "ws04.bin",    0xe0000, 0x20000, CRC(75f082e5) SHA1(b29f09a3cc9a0ac3f982be3981f5e895050c49e8) )

	ROM_REGION( 0x100000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "ws25.bin",    0x00000, 0x20000, CRC(8092e8e9) SHA1(eabe58ac0f88234b0dddf361f56aad509a83012e) )	/* sprites */
	ROM_LOAD( "ws26.bin",    0x20000, 0x20000, CRC(f6a1f42c) SHA1(6d5503e1a9b00104970292d22301ed28893c5223) )
	ROM_LOAD( "ws23.bin",    0x40000, 0x20000, CRC(43d58e24) SHA1(99e255faa9716d9102a1223419084fc209ab4024) )
	ROM_LOAD( "ws24.bin",    0x60000, 0x20000, CRC(20a867ea) SHA1(d3985002931fd4180fc541d61a94371871f3709d) )
	ROM_LOAD( "ws21.bin",    0x80000, 0x20000, CRC(e23d7296) SHA1(33bbced960be22efc7d2681e06a27feba09e0fc0) )
	ROM_LOAD( "ws22.bin",    0xa0000, 0x20000, CRC(7150a060) SHA1(73bdd7d6752f7fe9e23073d835dbc468d57865fa) )
	ROM_LOAD( "ws19.bin",    0xc0000, 0x20000, CRC(c5dd0a96) SHA1(4696ab1b02d40c54a7dacf0bdf90b624b7d6812e) )
	ROM_LOAD( "ws20.bin",    0xe0000, 0x20000, CRC(f1245c16) SHA1(f3941bf5830995f65a5378326fdb72687fbbddcf) )

	ROM_REGION( 0x20000, REGION_SOUND1, 0 )	/* ADPCM samples */
	ROM_LOAD( "bb_08.bin",    0x00000, 0x20000, CRC(deb1b975) SHA1(08f2e9a0a23171201b71d381d091edcd3787c287) )
ROM_END

ROM_START( skysmash )
	ROM_REGION( 0x80000, REGION_CPU1, 0 )
	ROM_LOAD16_BYTE( "rom5",    0x00000, 0x20000, CRC(867f9897) SHA1(7751f9d03d71bd5db0b82bda6e4d5231a30c1ad0) )
	ROM_LOAD16_BYTE( "rom6",    0x00001, 0x20000, CRC(e9c1d308) SHA1(d7032345b91f87de64ad09ffea49e39b755cac44) )
	ROM_LOAD16_BYTE( "rom7",    0x40000, 0x20000, CRC(d209db4d) SHA1(1cf85d39d12e92c1b97f7e5a148f3ad56cdca963) )
	ROM_LOAD16_BYTE( "rom8",    0x40001, 0x20000, CRC(d3646728) SHA1(898606be662214d2ba99e9a3e3cc0c7e7609a719) )

	ROM_REGION( 0x20000, REGION_CPU2, 0 )
	ROM_LOAD( "rom2",    0x000000, 0x08000, CRC(75b194cf) SHA1(6aaf36cdab06c0aa5328f5176557387a5d3f7d26) )
	ROM_CONTINUE(        0x010000, 0x08000 )
	ROM_COPY( REGION_CPU2, 0, 0x018000, 0x08000 )

	ROM_REGION( 0x20000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "rom3",    0x00000, 0x10000, CRC(fbb241be) SHA1(cd94c328891538bbd8c062d90a47ddf3d7d05bb0) )	/* characters */
	ROM_LOAD( "rom4",    0x10000, 0x10000, CRC(ad3cde81) SHA1(2bd0c707e5b67d3699a743d989cb5384cbe37ff7) )

	ROM_REGION( 0x100000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "rom9",    0x00000, 0x100000, CRC(b0a5eecf) SHA1(9e8191c7ae4a32dc16aebc37fa942afc531eddd4) )	/* Background + Foreground */

	ROM_REGION( 0x80000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "rom10",   0x00000, 0x080000, CRC(1bbcda5d) SHA1(63915221f70a7dfda6a4d8ac7f5c663c9316610a) )	/* sprites */

	ROM_REGION( 0x20000, REGION_SOUND1, 0 )	/* ADPCM samples */
	ROM_LOAD( "rom1",    0x00000, 0x20000, CRC(e69986f6) SHA1(de38bf2d5638cb40740882e1abccf7928e43a5a6) )
ROM_END


/***************************************************************************/

static DRIVER_INIT( weststry )
{
	UINT8 *gfx = memory_region(REGION_GFX3);
	int i;

	/* invert sprite data */
	for (i = 0;i < memory_region_length(REGION_GFX3);i++)
		gfx[i] = ~gfx[i];
}

/***************************************************************************/

GAMEX(1990, bloodbro, 0,        bloodbro, bloodbro, 0,        ROT0,   "Tad", "Blood Bros.", GAME_NO_COCKTAIL )
GAMEX(1990, weststry, bloodbro, weststry, weststry, weststry, ROT0,   "bootleg", "West Story", GAME_NO_COCKTAIL | GAME_NO_SOUND )
GAME( 1990, skysmash, 0,        skysmash, skysmash, 0,        ROT270, "Nihon System", "Sky Smasher" )
