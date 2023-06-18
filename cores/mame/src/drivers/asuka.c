/***************************************************************************

Asuka & Asuka  (+ Taito/Visco games on similar hardware)
=============

David Graves, Brian Troha

Made out of:	Rastan driver by Jarek Burczynski
				MAME Taito F2 driver
				Raine source - very special thanks to
				  Richard Bush and the Raine Team.
				two different drivers for Bonze Adventure that were
				  written at the same time by Yochizo and Frotz

	Bonze Adventure (c) 1988 Taito Corporation
	Asuka & Asuka   (c) 1988 Taito Corporation
	Maze of Flott   (c) 1989 Taito Corporation
	Galmedes        (c) 1992 Visco Corporation
	Earth Joker     (c) 1993 Visco Corporation
	Kokontouzai Eto Monogatari (c) 1994 Visco Corporation

Main CPU: MC68000 uses irq 5 (4 in bonze, 4&5 in cadash).
Sound   : Z80 & YM2151 + MSM5205 (YM2610 in bonze)
Chips   : TC0100SCN + TC0002OBJ + TC0110PCR (+ C-Chip in bonze)

Memory map for Asuka & Asuka
----------------------------

The other games seem identical but Eto is slightly different.

0x000000 - 0x0fffff : ROM (not all used for each game)
0x100000 - 0x103fff : 16k of RAM
0x200000 - 0x20000f : palette generator
0x400000 - 0x40000f : input ports and dipswitches
0x3a0000 - 0x3a0003 : sprite control
0x3e0000 - 0x3e0003 : communication with sound CPU
0xc00000 - 0xc2000f : TC0100SCN (see taitoic.c)
0xd00000 - 0xd007ff : sprite RAM


Cadashu Info (Malcor)
---------------------

Main PCB (JAMMA) K1100528A
Main processor  - 68000 12MHz
                - HD64180RP8 8MHz (8 bit processor, dual channel DMAC,
                             memory mapped I/O, used for multigame link)
Misc custom ICs including three PQFPs, one PGA, and one SIP


From "garmedes.txt"
-------------------

The following cord is written, on PCB:  K1100388A   J1100169A   M6100708A
There are the parts that were written as B68 on this PCB.
The original title of the game called B68 is unknown.
This PCB is the same as the one that is used with EARTH-JOKER.
<I think B68 is the Taito ROM id# for Asuka & Asuka - B.Troha>


Use of TC0100SCN
----------------

Asuka & Asuka: $e6a init code clearing TC0100SCN areas is erroneous.
It only clears 1/8 of the BG layers; then it clears too much of the
rowscroll areas [0xc000, 0xc400] causing overrun into next 64K block.

Asuka is one of the early Taito games using the TC0100SCN. (Ninja
Warriors was probably the first.) They didn't bother using its FG (text)
layer facility, instead placing text in the BG / sprite layers.

Maze of Flott [(c) one year later] and most other games with the
TC0100SCN do use the FG layer for text (Driftout is an exception).


Notes on Bonze DIPs by Stephane Humbert
---------------------------------------

The 2nd bonus life is awarded at the wrong score because of a bug in
the game code at $961E; and the unused DIP switch enables the built-in
map editor if the branch at $7572 is skipped.


TODO
----

DIPs

Mofflot: $14c46 sub inits sound system: in a pause loop during this
it reads a dummy address.

Earthjkr: Wrong screen size? Left edge of green blueprints in
attract looks like it's incorrectly off screen.

Cadash: Hooks for twin arcade machine setup: will involve emulating an extra
microcontroller, the 07 rom might be the program for it.

Galmedes: Test mode has select1/2 stuck at on.

Eto: $76d0 might be a protection check? It reads to and writes from
the prog rom. Doesn't seem to cause problems though.

***************************************************************************/

#include "driver.h"
#include "state.h"
#include "vidhrdw/taitoic.h"
#include "sndhrdw/taitosnd.h"

WRITE16_HANDLER( asuka_spritectrl_w );

INTERRUPT_GEN( rastan_s_interrupt );

VIDEO_START( asuka );
VIDEO_START( galmedes );
VIDEO_START( cadash );
VIDEO_UPDATE( asuka );
VIDEO_UPDATE( bonzeadv );

WRITE_HANDLER( rastan_adpcm_trigger_w );
WRITE_HANDLER( rastan_c000_w );
WRITE_HANDLER( rastan_d000_w );

WRITE16_HANDLER( bonzeadv_c_chip_w );
READ16_HANDLER( bonzeadv_c_chip_r );


/***********************************************************
				INTERRUPTS
***********************************************************/

void cadash_irq_handler(int irq);

static void cadash_interrupt5(int param)
{
	cpu_set_irq_line(0, 5, HOLD_LINE);
}

INTERRUPT_GEN( cadash_interrupt )
{
	timer_set(TIME_IN_CYCLES(500,0),0, cadash_interrupt5);
	cpu_set_irq_line(0, 4, HOLD_LINE);  /* interrupt vector 4 */
}


/************************************************
			SOUND
************************************************/

static WRITE_HANDLER( sound_bankswitch_w )
{
	cpu_setbank( 1, memory_region(REGION_CPU2) + ((data-1) & 0x03) * 0x4000 + 0x10000 );
}


/***********************************************************
			 MEMORY STRUCTURES
***********************************************************/

static MEMORY_READ16_START( bonzeadv_readmem )
	{ 0x000000, 0x03ffff, MRA16_ROM },
	{ 0x080000, 0x0fffff, MRA16_ROM },
	{ 0x10c000, 0x10ffff, MRA16_RAM },	/* main RAM */
	{ 0x200000, 0x200007, TC0110PCR_word_r },
	{ 0x390000, 0x390001, input_port_0_word_r },
	{ 0x3b0000, 0x3b0001, input_port_1_word_r },
	{ 0x3d0000, 0x3d0001, MRA16_NOP },
	{ 0x3e0002, 0x3e0003, taitosound_comm16_lsb_r },
	{ 0x800000, 0x800803, bonzeadv_c_chip_r },
	{ 0xc00000, 0xc0ffff, TC0100SCN_word_0_r },	/* tilemaps */
	{ 0xc20000, 0xc2000f, TC0100SCN_ctrl_word_0_r },
	{ 0xd00000, 0xd03fff, PC090OJ_word_0_r },	/* sprite ram */
MEMORY_END

static MEMORY_WRITE16_START( bonzeadv_writemem )
	{ 0x000000, 0x03ffff, MWA16_ROM },
	{ 0x10c000, 0x10ffff, MWA16_RAM },
	{ 0x200000, 0x200007, TC0110PCR_step1_word_w },
	{ 0x3a0000, 0x3a0001, asuka_spritectrl_w },
	{ 0x3c0000, 0x3c0001, watchdog_reset16_w },
	{ 0x3e0000, 0x3e0001, taitosound_port16_lsb_w },
	{ 0x3e0002, 0x3e0003, taitosound_comm16_lsb_w },
	{ 0x800000, 0x800c01, bonzeadv_c_chip_w },
	{ 0xc00000, 0xc0ffff, TC0100SCN_word_0_w },	/* tilemaps */
	{ 0xc20000, 0xc2000f, TC0100SCN_ctrl_word_0_w },
	{ 0xd00000, 0xd03fff, PC090OJ_word_0_w },	/* sprite ram */
MEMORY_END

static MEMORY_READ16_START( asuka_readmem )
	{ 0x000000, 0x0fffff, MRA16_ROM },
	{ 0x100000, 0x103fff, MRA16_RAM },	/* RAM */
	{ 0x1076f0, 0x1076f1, MRA16_NOP },	/* Mofflott init does dummy reads here */
	{ 0x200000, 0x20000f, TC0110PCR_word_r },
	{ 0x3e0000, 0x3e0001, MRA16_NOP },
	{ 0x3e0002, 0x3e0003, taitosound_comm16_lsb_r },
	{ 0x400000, 0x40000f, TC0220IOC_halfword_r },
	{ 0xc00000, 0xc0ffff, TC0100SCN_word_0_r },	/* tilemaps */
	{ 0xc20000, 0xc2000f, TC0100SCN_ctrl_word_0_r },
	{ 0xd00000, 0xd03fff, PC090OJ_word_0_r },	/* sprite ram */
MEMORY_END

static MEMORY_WRITE16_START( asuka_writemem )
	{ 0x000000, 0x0fffff, MWA16_ROM },
	{ 0x100000, 0x103fff, MWA16_RAM },
	{ 0x200000, 0x20000f, TC0110PCR_step1_word_w },
	{ 0x3a0000, 0x3a0003, asuka_spritectrl_w },
	{ 0x3e0000, 0x3e0001, taitosound_port16_lsb_w },
	{ 0x3e0002, 0x3e0003, taitosound_comm16_lsb_w },
	{ 0x400000, 0x40000f, TC0220IOC_halfword_w },
	{ 0xc00000, 0xc0ffff, TC0100SCN_word_0_w },	/* tilemaps */
	{ 0xc10000, 0xc103ff, MWA16_NOP },	/* error in Asuka init code */
	{ 0xc20000, 0xc2000f, TC0100SCN_ctrl_word_0_w },
	{ 0xd00000, 0xd03fff, PC090OJ_word_0_w },	/* sprite ram */
MEMORY_END

static MEMORY_READ16_START( cadash_readmem )
	{ 0x000000, 0x07ffff, MRA16_ROM },
	{ 0x0c0000, 0x0c0001, MRA16_NOP },
	{ 0x0c0002, 0x0c0003, taitosound_comm16_lsb_r },
	{ 0x100000, 0x107fff, MRA16_RAM },	/* RAM */
	{ 0x800000, 0x800fff, MRA16_RAM },	/* network ram */
	{ 0x900000, 0x90000f, TC0220IOC_halfword_r },
	{ 0xa00000, 0xa0000f, TC0110PCR_word_r },
	{ 0xb00000, 0xb03fff, PC090OJ_word_0_r },	/* sprite ram */
	{ 0xc00000, 0xc0ffff, TC0100SCN_word_0_r },	/* tilemaps */
	{ 0xc20000, 0xc2000f, TC0100SCN_ctrl_word_0_r },
MEMORY_END

static MEMORY_WRITE16_START( cadash_writemem )
	{ 0x000000, 0x07ffff, MWA16_ROM },
	{ 0x080000, 0x080003, asuka_spritectrl_w },
	{ 0x0c0000, 0x0c0001, taitosound_port16_lsb_w },
	{ 0x0c0002, 0x0c0003, taitosound_comm16_lsb_w },
	{ 0x100000, 0x107fff, MWA16_RAM },
	{ 0x800000, 0x800fff, MWA16_RAM },	/* network ram */
	{ 0x900000, 0x90000f, TC0220IOC_halfword_w },
	{ 0xa00000, 0xa0000f, TC0110PCR_step1_4bpg_word_w },
	{ 0xb00000, 0xb03fff, PC090OJ_word_0_w },	/* sprite ram */
	{ 0xc00000, 0xc0ffff, TC0100SCN_word_0_w },	/* tilemaps */
	{ 0xc20000, 0xc2000f, TC0100SCN_ctrl_word_0_w },
MEMORY_END

static MEMORY_READ16_START( eto_readmem )
	{ 0x000000, 0x0fffff, MRA16_ROM },
	{ 0x100000, 0x10000f, TC0110PCR_word_r },
	{ 0x200000, 0x203fff, MRA16_RAM },	/* RAM */
	{ 0x300000, 0x30000f, TC0220IOC_halfword_r },
	{ 0x400000, 0x40000f, TC0220IOC_halfword_r },	/* service mode mirror */
	{ 0x4e0000, 0x4e0001, MRA16_NOP },
	{ 0x4e0002, 0x4e0003, taitosound_comm16_lsb_r },
	{ 0xc00000, 0xc03fff, PC090OJ_word_0_r },	/* sprite ram */
	{ 0xd00000, 0xd0ffff, TC0100SCN_word_0_r },	/* tilemaps */
	{ 0xd20000, 0xd2000f, TC0100SCN_ctrl_word_0_r },
MEMORY_END

static MEMORY_WRITE16_START( eto_writemem )	/* N.B. tc100scn mirror overlaps spriteram */
	{ 0x000000, 0x0fffff, MWA16_ROM },
	{ 0x100000, 0x10000f, TC0110PCR_step1_word_w },
	{ 0x200000, 0x203fff, MWA16_RAM },
	{ 0x300000, 0x30000f, TC0220IOC_halfword_w },
	{ 0x4a0000, 0x4a0003, asuka_spritectrl_w },
	{ 0x4e0000, 0x4e0001, taitosound_port16_lsb_w },
	{ 0x4e0002, 0x4e0003, taitosound_comm16_lsb_w },
	{ 0xc00000, 0xc03fff, PC090OJ_word_0_w },	/* sprite ram */
	{ 0xc00000, 0xc0ffff, TC0100SCN_word_0_w },	/* service mode mirror */
	{ 0xd00000, 0xd0ffff, TC0100SCN_word_0_w },	/* tilemaps */
	{ 0xd20000, 0xd2000f, TC0100SCN_ctrl_word_0_w },
MEMORY_END


/***************************************************************************/

static MEMORY_READ_START( bonzeadv_z80_readmem )
	{ 0x0000, 0x3fff, MRA_ROM },
	{ 0x4000, 0x7fff, MRA_BANK1 },
	{ 0xc000, 0xdfff, MRA_RAM },
	{ 0xe000, 0xe000, YM2610_status_port_0_A_r },
	{ 0xe001, 0xe001, YM2610_read_port_0_r },
	{ 0xe002, 0xe002, YM2610_status_port_0_B_r },
	{ 0xe201, 0xe201, taitosound_slave_comm_r },
MEMORY_END

static MEMORY_WRITE_START( bonzeadv_z80_writemem )
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0xc000, 0xdfff, MWA_RAM },
	{ 0xe000, 0xe000, YM2610_control_port_0_A_w },
	{ 0xe001, 0xe001, YM2610_data_port_0_A_w },
	{ 0xe002, 0xe002, YM2610_control_port_0_B_w },
	{ 0xe003, 0xe003, YM2610_data_port_0_B_w },
	{ 0xe200, 0xe200, taitosound_slave_port_w },
	{ 0xe201, 0xe201, taitosound_slave_comm_w },
	{ 0xe400, 0xe403, MWA_NOP }, /* pan */
	{ 0xe600, 0xe600, MWA_NOP },
	{ 0xee00, 0xee00, MWA_NOP },
	{ 0xf000, 0xf000, MWA_NOP },
	{ 0xf200, 0xf200, sound_bankswitch_w },
MEMORY_END

static MEMORY_READ_START( z80_readmem )
	{ 0x0000, 0x3fff, MRA_ROM },
	{ 0x4000, 0x7fff, MRA_BANK1 },
	{ 0x8000, 0x8fff, MRA_RAM },
	{ 0x9001, 0x9001, YM2151_status_port_0_r },
	{ 0x9002, 0x9100, MRA_RAM },
	{ 0xa001, 0xa001, taitosound_slave_comm_r },
MEMORY_END

static MEMORY_WRITE_START( z80_writemem )
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0x8000, 0x8fff, MWA_RAM },
	{ 0x9000, 0x9000, YM2151_register_port_0_w },
	{ 0x9001, 0x9001, YM2151_data_port_0_w },
	{ 0xa000, 0xa000, taitosound_slave_port_w },
	{ 0xa001, 0xa001, taitosound_slave_comm_w },
	{ 0xb000, 0xb000, rastan_adpcm_trigger_w },
	{ 0xc000, 0xc000, rastan_c000_w },
	{ 0xd000, 0xd000, rastan_d000_w },
MEMORY_END


/***********************************************************
			 INPUT PORTS, DIPs
***********************************************************/


#define TAITO_COINAGE_JAPAN_8 \
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Coin_A ) ) \
	PORT_DIPSETTING(    0x10, DEF_STR( 2C_1C ) ) \
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_1C ) ) \
	PORT_DIPSETTING(    0x00, DEF_STR( 2C_3C ) ) \
	PORT_DIPSETTING(    0x20, DEF_STR( 1C_2C ) ) \
	PORT_DIPNAME( 0xc0, 0xc0, DEF_STR( Coin_B ) ) \
	PORT_DIPSETTING(    0x40, DEF_STR( 2C_1C ) ) \
	PORT_DIPSETTING(    0xc0, DEF_STR( 1C_1C ) ) \
	PORT_DIPSETTING(    0x00, DEF_STR( 2C_3C ) ) \
	PORT_DIPSETTING(    0x80, DEF_STR( 1C_2C ) )

#define TAITO_COINAGE_WORLD_8 \
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Coin_A ) ) \
	PORT_DIPSETTING(    0x00, DEF_STR( 4C_1C ) ) \
	PORT_DIPSETTING(    0x10, DEF_STR( 3C_1C ) ) \
	PORT_DIPSETTING(    0x20, DEF_STR( 2C_1C ) ) \
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_1C ) ) \
	PORT_DIPNAME( 0xc0, 0xc0, DEF_STR( Coin_B ) ) \
	PORT_DIPSETTING(    0xc0, DEF_STR( 1C_2C ) ) \
	PORT_DIPSETTING(    0x80, DEF_STR( 1C_3C ) ) \
	PORT_DIPSETTING(    0x40, DEF_STR( 1C_4C ) ) \
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_6C ) )

#define TAITO_COINAGE_US_8 \
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Coinage ) ) \
	PORT_DIPSETTING(    0x00, DEF_STR( 4C_1C ) ) \
	PORT_DIPSETTING(    0x10, DEF_STR( 3C_1C ) ) \
	PORT_DIPSETTING(    0x20, DEF_STR( 2C_1C ) ) \
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_1C ) ) \
	PORT_DIPNAME( 0xc0, 0xc0, "Price to Continue" ) \
	PORT_DIPSETTING(    0x00, DEF_STR( 3C_1C ) ) \
	PORT_DIPSETTING(    0x40, DEF_STR( 2C_1C ) ) \
	PORT_DIPSETTING(    0x80, DEF_STR( 1C_1C ) ) \
	PORT_DIPSETTING(    0xc0, "Same as Start" )

#define TAITO_DIFFICULTY_8 \
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Difficulty ) ) \
	PORT_DIPSETTING(    0x02, "Easy" ) \
	PORT_DIPSETTING(    0x03, "Medium" ) \
	PORT_DIPSETTING(    0x01, "Hard" ) \
	PORT_DIPSETTING(    0x00, "Hardest" )

#define ASUKA_PLAYERS_INPUT( player ) \
	PORT_START \
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | player ) \
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | player ) \
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | player ) \
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | player ) \
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | player ) \
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | player ) \
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN ) \
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

#define ASUKA_SYSTEM_INPUT \
	PORT_START \
	PORT_BIT( 0x01, IP_ACTIVE_LOW,  IPT_TILT ) \
	PORT_BIT( 0x02, IP_ACTIVE_LOW,  IPT_SERVICE1 ) \
	PORT_BIT( 0x04, IP_ACTIVE_LOW,  IPT_COIN1 ) \
	PORT_BIT( 0x08, IP_ACTIVE_LOW,  IPT_COIN2 ) \
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNKNOWN ) \
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN ) \
	PORT_BIT( 0x40, IP_ACTIVE_LOW,  IPT_START1 ) \
	PORT_BIT( 0x80, IP_ACTIVE_LOW,  IPT_START2 )

#define CADASH_PLAYERS_INPUT( player ) \
	PORT_START \
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN ) \
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN ) \
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON2 | player ) \
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON1 | player ) \
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | player ) \
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | player ) \
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | player ) \
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | player )

#define CADASH_SYSTEM_INPUT \
	PORT_START \
	PORT_BIT( 0x01, IP_ACTIVE_LOW,  IPT_COIN1 ) \
	PORT_BIT( 0x02, IP_ACTIVE_LOW,  IPT_COIN2 ) \
	PORT_BIT( 0x04, IP_ACTIVE_LOW,  IPT_START2 ) \
	PORT_BIT( 0x08, IP_ACTIVE_LOW,  IPT_START1 ) \
	PORT_BIT( 0x10, IP_ACTIVE_LOW,  IPT_SERVICE1 ) \
	PORT_BIT( 0x20, IP_ACTIVE_LOW,  IPT_TILT ) \
	PORT_BIT( 0x40, IP_ACTIVE_LOW,  IPT_UNKNOWN ) \
	PORT_BIT( 0x80, IP_ACTIVE_LOW,  IPT_UNKNOWN )


INPUT_PORTS_START( bonzeadv )
	PORT_START	/* DSWA */
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_SERVICE( 0x04, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_1C ) )
	PORT_DIPNAME( 0xc0, 0xc0, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_6C ) )

	PORT_START	/* DSWB */
	TAITO_DIFFICULTY_8
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x08, "40k,100k" )
	PORT_DIPSETTING(    0x0c, "50k,150k" )
	PORT_DIPSETTING(    0x04, "60k,200k" )
	PORT_DIPSETTING(    0x00, "80k,250k" )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x20, "2" )
	PORT_DIPSETTING(    0x30, "3" )
	PORT_DIPSETTING(    0x10, "4" )
	PORT_DIPSETTING(    0x00, "5" )
	PORT_DIPNAME( 0x40, 0x00, "Allow Continue" )
	PORT_DIPSETTING(    0x40, DEF_STR( No ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START	/* 800007 */
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_SERVICE1 )

	PORT_START /* 800009 */
	PORT_BIT_IMPULSE( 0x01, IP_ACTIVE_HIGH, IPT_COIN1, 1 )
	PORT_BIT_IMPULSE( 0x02, IP_ACTIVE_HIGH, IPT_COIN2, 1 )

	PORT_START	/* 80000B */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_TILT )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON2 )

	PORT_START	/* 80000d */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END

INPUT_PORTS_START( jigkmgri ) /* coinage DIPs differ from bonzeadv */
	PORT_START	/* DSWA */
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_SERVICE( 0x04, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	TAITO_COINAGE_JAPAN_8

	PORT_START	/* DSWB */
	TAITO_DIFFICULTY_8
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x08, "40k,100k" )
	PORT_DIPSETTING(    0x0c, "50k,150k" )
	PORT_DIPSETTING(    0x04, "60k,200k" )
	PORT_DIPSETTING(    0x00, "80k,250k" )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x20, "2" )
	PORT_DIPSETTING(    0x30, "3" )
	PORT_DIPSETTING(    0x10, "4" )
	PORT_DIPSETTING(    0x00, "5" )
	PORT_DIPNAME( 0x40, 0x00, "Allow Continue" )
	PORT_DIPSETTING(    0x40, DEF_STR( No ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START	/* 800007 */
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_SERVICE1 )

	PORT_START /* 800009 */
	PORT_BIT_IMPULSE( 0x01, IP_ACTIVE_HIGH, IPT_COIN1, 1 )
	PORT_BIT_IMPULSE( 0x02, IP_ACTIVE_HIGH, IPT_COIN2, 1 )

	PORT_START	/* 80000B */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_TILT )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON2 )

	PORT_START	/* 80000d */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END

INPUT_PORTS_START( asuka )
	PORT_START	/* DSWA */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_SERVICE( 0x04, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	TAITO_COINAGE_JAPAN_8

	PORT_START	/* DSWB */
	TAITO_DIFFICULTY_8
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x10, "1" )
	PORT_DIPSETTING(    0x20, "2" )
	PORT_DIPSETTING(    0x30, "3" )
	PORT_DIPSETTING(    0x00, "4" )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0xc0, 0x40, "Allow Continue" )
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0xc0, "Up to Level 2" )
	PORT_DIPSETTING(    0x80, "Up to Level 3" )
	PORT_DIPSETTING(    0x40, DEF_STR( Yes ) )

	/* IN0 */
	ASUKA_PLAYERS_INPUT( IPF_PLAYER1 )

	/* IN1 */
	ASUKA_PLAYERS_INPUT( IPF_PLAYER2 )

	/* IN2 */
	ASUKA_SYSTEM_INPUT
INPUT_PORTS_END

INPUT_PORTS_START( mofflott )
	PORT_START	/* DSWA */
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_SERVICE( 0x04, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	TAITO_COINAGE_JAPAN_8

	PORT_START	/* DSWB */
	TAITO_DIFFICULTY_8
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x0c, "20k and every 50k" )
	PORT_DIPSETTING(    0x08, "50k and every 100k" )
	PORT_DIPSETTING(    0x04, "100k only" )
	PORT_DIPSETTING(    0x00, "None" )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "2" )
	PORT_DIPSETTING(    0x30, "3" )
	PORT_DIPSETTING(    0x10, "4" )
	PORT_DIPSETTING(    0x20, "5" )
	PORT_BITX(    0x40, 0x40, IPT_DIPSWITCH_NAME | IPF_CHEAT, "Invulnerability", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	/* IN0 */
	ASUKA_PLAYERS_INPUT( IPF_PLAYER1 )

	/* IN1 */
	ASUKA_PLAYERS_INPUT( IPF_PLAYER2 )

	/* IN2 */
	ASUKA_SYSTEM_INPUT
INPUT_PORTS_END

INPUT_PORTS_START( cadash )
	PORT_START	/* DSWA */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unused ) )	/* Manual says leave it off*/
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_SERVICE( 0x04, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	TAITO_COINAGE_WORLD_8

	PORT_START	/* DSWB */
	TAITO_DIFFICULTY_8
	PORT_DIPNAME( 0x0c, 0x0c, "Starting Time" )
	PORT_DIPSETTING(    0x00, "5:00" )
	PORT_DIPSETTING(    0x04, "6:00" )
	PORT_DIPSETTING(    0x0c, "7:00" )
	PORT_DIPSETTING(    0x08, "8:00" )
	/* Round cleared   Added time	*/
	/*       1            8:00	*/
	/*       2           10:00	*/
	/*       3            8:00	*/
	/*       4            7:00	*/
	/*       5            9:00	*/
	PORT_DIPNAME( 0x30, 0x30, "Added Time (after round clear)" )
	PORT_DIPSETTING(    0x00, "Default - 2:00" )
	PORT_DIPSETTING(    0x10, "Default - 1:00" )
	PORT_DIPSETTING(    0x30, "Default" )
	PORT_DIPSETTING(    0x20, "Default + 1:00" )
	PORT_DIPNAME( 0xc0, 0xc0, "Communication Mode" )
	PORT_DIPSETTING(    0xc0, "Stand alone" )
	PORT_DIPSETTING(    0x80, "Master" )
	PORT_DIPSETTING(    0x00, "Slave" )
/*	PORT_DIPSETTING(    0x40, "Stand alone" )*/

	/* IN0 */
	CADASH_PLAYERS_INPUT( IPF_PLAYER1 )

	/* IN1 */
	CADASH_PLAYERS_INPUT( IPF_PLAYER2 )

	/* IN2 */
	CADASH_SYSTEM_INPUT
INPUT_PORTS_END

INPUT_PORTS_START( cadashj )
	PORT_START	/* DSWA */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unused ) )	/* Manual says leave it off*/
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_SERVICE( 0x04, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	TAITO_COINAGE_JAPAN_8

	PORT_START	/* DSWB */
	TAITO_DIFFICULTY_8
	PORT_DIPNAME( 0x0c, 0x0c, "Starting Time" )
	PORT_DIPSETTING(    0x00, "5:00" )
	PORT_DIPSETTING(    0x04, "6:00" )
	PORT_DIPSETTING(    0x0c, "7:00" )
	PORT_DIPSETTING(    0x08, "8:00" )
	PORT_DIPNAME( 0x30, 0x30, "Added Time (after round clear)" )
	PORT_DIPSETTING(    0x00, "Default - 2:00" )
	PORT_DIPSETTING(    0x10, "Default - 1:00" )
	PORT_DIPSETTING(    0x30, "Default" )
	PORT_DIPSETTING(    0x20, "Default + 1:00" )
	PORT_DIPNAME( 0xc0, 0xc0, "Communication Mode" )
	PORT_DIPSETTING(    0xc0, "Stand alone" )
	PORT_DIPSETTING(    0x80, "Master" )
	PORT_DIPSETTING(    0x00, "Slave" )
/*	PORT_DIPSETTING(    0x40, "Stand alone" )*/

	/* IN0 */
	CADASH_PLAYERS_INPUT( IPF_PLAYER1 )

	/* IN1 */
	CADASH_PLAYERS_INPUT( IPF_PLAYER2 )

	/* IN2 */
	CADASH_SYSTEM_INPUT
INPUT_PORTS_END

INPUT_PORTS_START( cadashu )
	PORT_START	/* DSWA */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unused ) )	/* Manual says leave it off*/
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_SERVICE( 0x04, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	TAITO_COINAGE_US_8

	PORT_START	/* DSWB */
	TAITO_DIFFICULTY_8
	PORT_DIPNAME( 0x0c, 0x0c, "Starting Time" )
	PORT_DIPSETTING(    0x00, "5:00" )
	PORT_DIPSETTING(    0x04, "6:00" )
	PORT_DIPSETTING(    0x0c, "7:00" )
	PORT_DIPSETTING(    0x08, "8:00" )
	PORT_DIPNAME( 0x30, 0x30, "Added Time (after round clear)" )
	PORT_DIPSETTING(    0x00, "Default - 2:00" )
	PORT_DIPSETTING(    0x10, "Default - 1:00" )
	PORT_DIPSETTING(    0x30, "Default" )
	PORT_DIPSETTING(    0x20, "Default + 1:00" )
	PORT_DIPNAME( 0xc0, 0xc0, "Communication Mode" )
	PORT_DIPSETTING(    0xc0, "Stand alone" )
	PORT_DIPSETTING(    0x80, "Master" )
	PORT_DIPSETTING(    0x00, "Slave" )
/*	PORT_DIPSETTING(    0x40, "Stand alone" )*/

	/* IN0 */
	CADASH_PLAYERS_INPUT( IPF_PLAYER1 )

	/* IN1 */
	CADASH_PLAYERS_INPUT( IPF_PLAYER2 )

	/* IN2 */
	CADASH_SYSTEM_INPUT
INPUT_PORTS_END

INPUT_PORTS_START( galmedes )
	PORT_START	/* DSWA */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_SERVICE( 0x04, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	TAITO_COINAGE_JAPAN_8

	PORT_START	/* DSWB */
	TAITO_DIFFICULTY_8
	PORT_DIPNAME( 0x0c, 0x08, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x08, "every 100k" )
	PORT_DIPSETTING(    0x0c, "100k and every 200k" )
	PORT_DIPSETTING(    0x04, "150k and every 200k" )
	PORT_DIPSETTING(    0x00, "every 200k" )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x20, "1" )
	PORT_DIPSETTING(    0x10, "2" )
	PORT_DIPSETTING(    0x30, "3" )
	PORT_DIPSETTING(    0x00, "4" )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	/* IN0 */
	ASUKA_PLAYERS_INPUT( IPF_PLAYER1 )

	/* IN1 */
	ASUKA_PLAYERS_INPUT( IPF_PLAYER2 )

	/* IN2 */
	ASUKA_SYSTEM_INPUT
INPUT_PORTS_END

INPUT_PORTS_START( earthjkr )
	PORT_START	/* DSWA */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_SERVICE( 0x04, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	TAITO_COINAGE_JAPAN_8

	PORT_START	/* DSWB */
	TAITO_DIFFICULTY_8
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "1" )
	PORT_DIPSETTING(    0x30, "2" )
	PORT_DIPSETTING(    0x20, "3" )
	PORT_DIPSETTING(    0x10, "4" )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	/* IN0 */
	ASUKA_PLAYERS_INPUT( IPF_PLAYER1 )

	/* IN1 */
	ASUKA_PLAYERS_INPUT( IPF_PLAYER2 )

	/* IN2 */
	ASUKA_SYSTEM_INPUT
INPUT_PORTS_END

INPUT_PORTS_START( eto )
	PORT_START	/* DSWA */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_SERVICE( 0x04, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	TAITO_COINAGE_JAPAN_8

	PORT_START	/* DSWB */
	TAITO_DIFFICULTY_8
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	/* IN0 */
	ASUKA_PLAYERS_INPUT( IPF_PLAYER1 )

	/* IN1 */
	ASUKA_PLAYERS_INPUT( IPF_PLAYER2 )

	/* IN2 */
	ASUKA_SYSTEM_INPUT
INPUT_PORTS_END


/**************************************************************
				GFX DECODING
**************************************************************/

static struct GfxLayout charlayout =
{
	8,8,
	RGN_FRAC(1,1),
	4,
	{ 0, 1, 2, 3 },
	{ 2*4, 3*4, 0*4, 1*4, 6*4, 7*4, 4*4, 5*4 },
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32 },
	32*8
};

static struct GfxLayout tilelayout =
{
	16,16,
	RGN_FRAC(1,1),
	4,
	{ 0, 1, 2, 3 },
	{ 2*4, 3*4, 0*4, 1*4, 6*4, 7*4, 4*4, 5*4,
	  10*4, 11*4, 8*4, 9*4, 14*4, 15*4, 12*4, 13*4 },
	{ 0*64, 1*64, 2*64, 3*64, 4*64, 5*64, 6*64, 7*64,
	  8*64, 9*64, 10*64, 11*64, 12*64, 13*64, 14*64, 15*64 },
	128*8
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX2, 0, &tilelayout,  0, 256 },	/* OBJ */
	{ REGION_GFX1, 0, &charlayout,  0, 256 },	/* SCR */
	{ -1 } /* end of array */
};



/**************************************************************
				SOUND
**************************************************************/

static void irq_handler(int irq)
{
	cpu_set_irq_line(1,0,irq ? ASSERT_LINE : CLEAR_LINE);
}

static struct YM2610interface ym2610_interface =
{
	1,	/* 1 chip */
	8000000,	/* 8 MHz */
	{ 25 },
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 },
	{ irq_handler },
	{ REGION_SOUND1 },	/* Delta-T */
	{ REGION_SOUND1 },	/* ADPCM */
	{ YM3012_VOL(100,MIXER_PAN_LEFT,100,MIXER_PAN_RIGHT) }
};


static struct YM2151interface ym2151_interface =
{
	1,			/* 1 chip */
	4000000,	/* 4 MHz ? */
	{ YM3012_VOL(50,MIXER_PAN_CENTER,50,MIXER_PAN_CENTER) },
	{ irq_handler },
	{ sound_bankswitch_w }
};


static struct ADPCMinterface adpcm_interface =
{
	1,			/* 1 chip */
	8000,       /* 8000Hz playback */
	REGION_SOUND1,	/* memory region */
	{ 60 }
};


/***********************************************************
			     MACHINE DRIVERS
***********************************************************/

VIDEO_EOF( asuka )
{
	PC090OJ_eof_callback();
}


static MACHINE_DRIVER_START( bonzeadv )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 8000000)    /* checked on PCB */
	MDRV_CPU_MEMORY(bonzeadv_readmem,bonzeadv_writemem)
	MDRV_CPU_VBLANK_INT(irq4_line_hold,1)

	MDRV_CPU_ADD(Z80,4000000)    /* sound CPU, also required for test mode */
	MDRV_CPU_MEMORY(bonzeadv_z80_readmem,bonzeadv_z80_writemem)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(10)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_BUFFERS_SPRITERAM)
	MDRV_SCREEN_SIZE(40*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 40*8-1, 3*8, 31*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(4096)

	MDRV_VIDEO_START(asuka)
	MDRV_VIDEO_EOF(asuka)
	MDRV_VIDEO_UPDATE(bonzeadv)

	/* sound hardware */
	MDRV_SOUND_ADD(YM2610, ym2610_interface)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( asuka )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 8000000)	/* 8 MHz ??? */
	MDRV_CPU_MEMORY(asuka_readmem,asuka_writemem)
	MDRV_CPU_VBLANK_INT(irq5_line_hold,1)

	MDRV_CPU_ADD(Z80, 4000000)	/* 4 MHz ??? */
	MDRV_CPU_MEMORY(z80_readmem,z80_writemem)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(10)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_BUFFERS_SPRITERAM)
	MDRV_SCREEN_SIZE(40*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 40*8-1, 2*8, 32*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(4096)

	MDRV_VIDEO_START(asuka)
	MDRV_VIDEO_EOF(asuka)
	MDRV_VIDEO_UPDATE(asuka)

	/* sound hardware */
	MDRV_SOUND_ADD(YM2151, ym2151_interface)
	MDRV_SOUND_ADD(ADPCM, adpcm_interface)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( cadash )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 12000000)	/* 12 MHz ??? */
	MDRV_CPU_MEMORY(cadash_readmem,cadash_writemem)
	MDRV_CPU_VBLANK_INT(cadash_interrupt,1)

	MDRV_CPU_ADD(Z80, 4000000)	/* 4 MHz ??? */
	MDRV_CPU_MEMORY(z80_readmem,z80_writemem)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(10)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_BUFFERS_SPRITERAM)
	MDRV_SCREEN_SIZE(40*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 40*8-1, 2*8, 32*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(4096)

	MDRV_VIDEO_START(cadash)
	MDRV_VIDEO_EOF(asuka)
	MDRV_VIDEO_UPDATE(bonzeadv)

	/* sound hardware */
	MDRV_SOUND_ADD(YM2151, ym2151_interface)
	MDRV_SOUND_ADD(ADPCM, adpcm_interface)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( galmedes )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 8000000)	/* 8 MHz ??? */
	MDRV_CPU_MEMORY(asuka_readmem,asuka_writemem)
	MDRV_CPU_VBLANK_INT(irq5_line_hold,1)

	MDRV_CPU_ADD(Z80, 4000000)	/* 4 MHz ??? */
	MDRV_CPU_MEMORY(z80_readmem,z80_writemem)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(10)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_BUFFERS_SPRITERAM)
	MDRV_SCREEN_SIZE(40*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 40*8-1, 2*8, 32*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(4096)	/* only Mofflott uses full palette space */

	MDRV_VIDEO_START(galmedes)
	MDRV_VIDEO_EOF(asuka)
	MDRV_VIDEO_UPDATE(asuka)

	/* sound hardware */
	MDRV_SOUND_ADD(YM2151, ym2151_interface)
	MDRV_SOUND_ADD(ADPCM, adpcm_interface)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( eto )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 8000000)	/* 8 MHz ??? */
	MDRV_CPU_MEMORY(eto_readmem,eto_writemem)
	MDRV_CPU_VBLANK_INT(irq5_line_hold,1)

	MDRV_CPU_ADD(Z80, 4000000)	/* 4 MHz ??? */
	MDRV_CPU_MEMORY(z80_readmem,z80_writemem)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(10)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_BUFFERS_SPRITERAM)
	MDRV_SCREEN_SIZE(40*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 40*8-1, 2*8, 32*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(4096)

	MDRV_VIDEO_START(galmedes)
	MDRV_VIDEO_EOF(asuka)
	MDRV_VIDEO_UPDATE(asuka)

	/* sound hardware */
	MDRV_SOUND_ADD(YM2151, ym2151_interface)
	MDRV_SOUND_ADD(ADPCM, adpcm_interface)
MACHINE_DRIVER_END


/***************************************************************************
					DRIVERS
***************************************************************************/

ROM_START( bonzeadv )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )     /* 68000 code */
	ROM_LOAD16_BYTE( "b41-09-1.17", 0x00000, 0x10000, CRC(af821fbc) SHA1(55bc13742033a31c92d6268d6b8344062ca78633) )
	ROM_LOAD16_BYTE( "b41-11-1.26", 0x00001, 0x10000, CRC(823fff00) SHA1(b8b8cafbe860136c202d8d9f3ed5a54e2f4df363) )
	ROM_LOAD16_BYTE( "b41-10.16",   0x20000, 0x10000, CRC(4ca94d77) SHA1(69a9f6bcb6d5e4132eed50860bdfe8d6b6d914cd) )
	ROM_LOAD16_BYTE( "b41-15.25",   0x20001, 0x10000, CRC(aed7a0d0) SHA1(99ffc0b0e88b81231756610bf48df5365e12603b) )
	/* 0x040000 - 0x7ffff is intentionally empty */
	ROM_LOAD16_WORD_SWAP( "b41-01.15", 0x80000, 0x80000, CRC(5d072fa4) SHA1(6ffe1b8531381eb6dd3f1fec18c91294a6aca9f6) )

	ROM_REGION( 0x80000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "b41-03.1",  0x00000, 0x80000, CRC(736d35d0) SHA1(7d41a7d71e117714bbd2cdda2953589cda6e763a) )	/* SCR tiles (8 x 8) */

	ROM_REGION( 0x80000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "b41-02.7",  0x00000, 0x80000, CRC(29f205d9) SHA1(9e9f0c2755a9aa5acfe2601911bfa07d8d61164c) )	/* Sprites (16 x 16) */

	ROM_REGION( 0x1c000, REGION_CPU2, 0 )     /* sound cpu */
	ROM_LOAD( "b41-13.20",  0x00000, 0x04000, CRC(9e464254) SHA1(b6f6126b54c15320ecaa652d0eeabaa4cd94bd26) )
	ROM_CONTINUE(        0x10000, 0x0c000 )   /* banked stuff */

	ROM_REGION( 0x80000, REGION_SOUND1, ROMREGION_SOUNDONLY )	  /* ADPCM samples */
	ROM_LOAD( "b41-04.48",  0x00000, 0x80000, CRC(c668638f) SHA1(07238a6cb4d93ffaf6351657163b5d80f0dbf688) )
ROM_END

ROM_START( bonzeadu )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )     /* 68000 code */
	ROM_LOAD16_BYTE( "b41-09-1.17", 0x00000, 0x10000, CRC(af821fbc) SHA1(55bc13742033a31c92d6268d6b8344062ca78633) )
	ROM_LOAD16_BYTE( "b41-11-1.26", 0x00001, 0x10000, CRC(823fff00) SHA1(b8b8cafbe860136c202d8d9f3ed5a54e2f4df363) )
	ROM_LOAD16_BYTE( "b41-10.16",   0x20000, 0x10000, CRC(4ca94d77) SHA1(69a9f6bcb6d5e4132eed50860bdfe8d6b6d914cd) )
	ROM_LOAD16_BYTE( "b41-14.25",   0x20001, 0x10000, CRC(37def16a) SHA1(b0a3b7206db55e29454672fffadf4e2a64eed873) )
	/* 0x040000 - 0x7ffff is intentionally empty */
	ROM_LOAD16_WORD_SWAP( "b41-01.15", 0x80000, 0x80000, CRC(5d072fa4) SHA1(6ffe1b8531381eb6dd3f1fec18c91294a6aca9f6) )

	ROM_REGION( 0x80000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "b41-03.1",  0x00000, 0x80000, CRC(736d35d0) SHA1(7d41a7d71e117714bbd2cdda2953589cda6e763a) )	/* SCR tiles (8 x 8) */

	ROM_REGION( 0x80000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "b41-02.7",  0x00000, 0x80000, CRC(29f205d9) SHA1(9e9f0c2755a9aa5acfe2601911bfa07d8d61164c) )	/* Sprites (16 x 16) */

	ROM_REGION( 0x1c000, REGION_CPU2, 0 )     /* sound cpu */
	ROM_LOAD( "b41-13.20",  0x00000, 0x04000, CRC(9e464254) SHA1(b6f6126b54c15320ecaa652d0eeabaa4cd94bd26) )
	ROM_CONTINUE(        0x10000, 0x0c000 )   /* banked stuff */

	ROM_REGION( 0x80000, REGION_SOUND1, ROMREGION_SOUNDONLY )	  /* ADPCM samples */
	ROM_LOAD( "b41-04.48",  0x00000, 0x80000, CRC(c668638f) SHA1(07238a6cb4d93ffaf6351657163b5d80f0dbf688) )
ROM_END

ROM_START( jigkmgri )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )     /* 68000 code */
	ROM_LOAD16_BYTE( "b41-09-1.17", 0x00000, 0x10000, CRC(af821fbc) SHA1(55bc13742033a31c92d6268d6b8344062ca78633) )
	ROM_LOAD16_BYTE( "b41-11-1.26", 0x00001, 0x10000, CRC(823fff00) SHA1(b8b8cafbe860136c202d8d9f3ed5a54e2f4df363) )
	ROM_LOAD16_BYTE( "b41-10.16",   0x20000, 0x10000, CRC(4ca94d77) SHA1(69a9f6bcb6d5e4132eed50860bdfe8d6b6d914cd) )
	ROM_LOAD16_BYTE( "b41-12.25",   0x20001, 0x10000, CRC(40d9c1fc) SHA1(6f03d263e10559988aaa2be00d9bbf55f2fb864e) )
	/* 0x040000 - 0x7ffff is intentionally empty */
	ROM_LOAD16_WORD_SWAP( "b41-01.15", 0x80000, 0x80000, CRC(5d072fa4) SHA1(6ffe1b8531381eb6dd3f1fec18c91294a6aca9f6) )

	ROM_REGION( 0x80000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "b41-03.1",  0x00000, 0x80000, CRC(736d35d0) SHA1(7d41a7d71e117714bbd2cdda2953589cda6e763a) )	/* Tiles (8 x 8) */

	ROM_REGION( 0x80000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "b41-02.7",  0x00000, 0x80000, CRC(29f205d9) SHA1(9e9f0c2755a9aa5acfe2601911bfa07d8d61164c) )	/* Sprites (16 x 16) */

	ROM_REGION( 0x1c000, REGION_CPU2, 0 )     /* sound cpu */
	ROM_LOAD( "b41-13.20",  0x00000, 0x04000, CRC(9e464254) SHA1(b6f6126b54c15320ecaa652d0eeabaa4cd94bd26) )
	ROM_CONTINUE(        0x10000, 0x0c000 )   /* banked stuff */

	ROM_REGION( 0x80000, REGION_SOUND1, ROMREGION_SOUNDONLY )	  /* ADPCM samples */
	ROM_LOAD( "b41-04.48",  0x00000, 0x80000, CRC(c668638f) SHA1(07238a6cb4d93ffaf6351657163b5d80f0dbf688) )
ROM_END

ROM_START( asuka )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )     /* 1024k for 68000 code */
	ROM_LOAD16_BYTE( "asuka_13.rom",  0x00000, 0x20000, CRC(855efb3e) SHA1(644e02e207adeaec7839c824688d88ab8d046418) )
	ROM_LOAD16_BYTE( "asuka_12.rom",  0x00001, 0x20000, CRC(271eeee9) SHA1(c08e347be4aae929c0ab95ff7618edaa1a7d6da9) )
	/* 0x040000 - 0x7ffff is intentionally empty */
	ROM_LOAD16_WORD( "asuka_03.rom",  0x80000, 0x80000, CRC(d3a59b10) SHA1(35a2ff18b64e73ac5e17484354c0cc58bc2cd7fc) )	/* Fix ROM */

	ROM_REGION( 0x80000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "asuka_01.rom",  0x00000, 0x80000, CRC(89f32c94) SHA1(74fbb699e05e2336509cb5ac06ed94335ff870d5) )	/* SCR tiles (8 x 8) */

	ROM_REGION( 0xa0000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD       ( "asuka_02.rom", 0x00000, 0x80000, CRC(f5018cd3) SHA1(860ce140ae369556d03d5d78987b87c0d6070df5) )	/* Sprites (16 x 16) */
	ROM_LOAD16_BYTE( "asuka_07.rom", 0x80000, 0x10000, CRC(c113acc8) SHA1(613c61a78df73dcb0b9c9018ae829e865baac772) )
	ROM_LOAD16_BYTE( "asuka_06.rom", 0x80001, 0x10000, CRC(f517e64d) SHA1(8be491bfe0f7eed58521de9d31da677acf635c23) )

	ROM_REGION( 0x1c000, REGION_CPU2, 0 )	/* sound cpu */
	ROM_LOAD( "asuka_11.rom", 0x00000, 0x04000, CRC(c378b508) SHA1(1b145fe736b924f298e02532cf9f26cc18b42ca7) )
	ROM_CONTINUE(             0x10000, 0x0c000 )	/* banked stuff */

	ROM_REGION( 0x10000, REGION_SOUND1, ROMREGION_SOUNDONLY )	/* ADPCM samples */
	ROM_LOAD( "asuka_10.rom", 0x00000, 0x10000, CRC(387aaf40) SHA1(47c583564ef1d49ece15f97221b2e073e8fb0544) )
ROM_END

ROM_START( mofflott )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )     /* 1024k for 68000 code */
	ROM_LOAD16_BYTE( "c17-09.bin",  0x00000, 0x20000, CRC(05ee110f) SHA1(8cedd911d3fdcca1e409260d12dd03a2fb35ef86) )
	ROM_LOAD16_BYTE( "c17-08.bin",  0x00001, 0x20000, CRC(d0aacffd) SHA1(2c5ec4020aad2c1cd3a004dc70a12e0d77eb6aa7) )
	/* 0x40000 - 0x7ffff is intentionally empty */
	ROM_LOAD16_WORD( "c17-03.bin",  0x80000, 0x80000, CRC(27047fc3) SHA1(1f88a7a42a94bac0e164a69896ae168ab821fbb3) )	/* Fix ROM */

	ROM_REGION( 0x80000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "c17-01.bin",  0x00000, 0x80000, CRC(e9466d42) SHA1(93d533a9a992e3ff537e914577ede41729235826) )	/* SCR tiles (8 x 8) */

	ROM_REGION( 0xa0000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD       ( "c17-02.bin", 0x00000, 0x80000, CRC(8860a8db) SHA1(372adea8835a9524ece30ab71181ef9d05b120e9) )	/* Sprites (16 x 16) */
	ROM_LOAD16_BYTE( "c17-05.bin", 0x80000, 0x10000, CRC(57ac4741) SHA1(3188ff0866324c68fba8e9745a0cb186784cb53d) )
	ROM_LOAD16_BYTE( "c17-04.bin", 0x80001, 0x10000, CRC(f4250410) SHA1(1f5f6baca4aa695ce2ae5c65adcb460da872a239) )

	ROM_REGION( 0x1c000, REGION_CPU2, 0 )	/* sound cpu */
	ROM_LOAD( "c17-07.bin", 0x00000, 0x04000, CRC(cdb7bc2c) SHA1(5113055c954a39918436db75cc06b53c29c60728) )
	ROM_CONTINUE(           0x10000, 0x0c000 )	/* banked stuff */

	ROM_REGION( 0x10000, REGION_SOUND1, ROMREGION_SOUNDONLY )	/* ADPCM samples */
	ROM_LOAD( "c17-06.bin", 0x00000, 0x10000, CRC(5c332125) SHA1(408f42df18b38347c8a4e177a9484162a66877e1) )
ROM_END

ROM_START( cadash )
	ROM_REGION( 0x80000, REGION_CPU1, 0 )     /* 512k for 68000 code */
	ROM_LOAD16_BYTE( "c21-14",  0x00000, 0x20000, CRC(5daf13fb) SHA1(c2be42b2cdc90b6463ce87211cf711c951b17fab) )
	ROM_LOAD16_BYTE( "c21-16",  0x00001, 0x20000, CRC(cbaa2e75) SHA1(c41ea71f2b0e72bf993dfcfd30f1994cae9f52a0) )
	ROM_LOAD16_BYTE( "c21-13",  0x40000, 0x20000, CRC(6b9e0ee9) SHA1(06314b9c0be19314e6b6ecb5274a63eb36b642f5) )
	ROM_LOAD16_BYTE( "c21-17",  0x40001, 0x20000, CRC(bf9a578a) SHA1(42bde46081db6be2f61eaf171438ecc9264d18be) )

	ROM_REGION( 0x80000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "c21-02.9",  0x00000, 0x80000, CRC(205883b9) SHA1(5aafee8cab3f949a7db91bcc26912f331041b51e) )	/* SCR tiles (8 x 8) */

	ROM_REGION( 0x80000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "c21-01.1",  0x00000, 0x80000, CRC(1ff6f39c) SHA1(742f296efc8073fafa73da2c8d7d26ca9514b6bf) )	/* Sprites (16 x 16) */

	ROM_REGION( 0x1c000, REGION_CPU2, 0 )	/* sound cpu */
	ROM_LOAD( "c21-08.38",   0x00000, 0x04000, CRC(dca495a0) SHA1(4e0f401f1b967da75f33fd7294860ad0b4bf2dce) )
	ROM_CONTINUE(            0x10000, 0x0c000 )	/* banked stuff */

	ROM_REGION( 0x10000, REGION_SOUND1, 0 )
	/* empty region */

	ROM_REGION( 0x08000, REGION_USER1, 0 )	/* 2 machine interface mcu rom ? */
	ROM_LOAD( "c21-07.57",   0x00000, 0x08000, CRC(f02292bd) SHA1(0a5c06a048ad67f90e0d766b504582e9eef035f7) )

	ROM_REGION( 0x01000, REGION_USER2, 0 )	/* pals ? */
/*	ROM_LOAD( "c21-09",   0x00000, 0x00ada, CRC(f02292bd) SHA1(0a5c06a048ad67f90e0d766b504582e9eef035f7) )*/
/*	ROM_LOAD( "c21-10",   0x00000, 0x00ada, CRC(f02292bd) SHA1(0a5c06a048ad67f90e0d766b504582e9eef035f7) )*/
/*	ROM_LOAD( "c21-11-1", 0x00000, 0x00ada, CRC(f02292bd) SHA1(0a5c06a048ad67f90e0d766b504582e9eef035f7) )*/
/*	ROM_LOAD( "c21-12",   0x00000, 0x00cd5, CRC(f02292bd) SHA1(0a5c06a048ad67f90e0d766b504582e9eef035f7) )*/
ROM_END

ROM_START( cadashj )
	ROM_REGION( 0x80000, REGION_CPU1, 0 )     /* 512k for 68000 code */
	ROM_LOAD16_BYTE( "c21-04.11",  0x00000, 0x20000, CRC(cc22ebe5) SHA1(170787e7ab2055af593f3f2596cab44feb53b060) )
	ROM_LOAD16_BYTE( "c21-06.15",  0x00001, 0x20000, CRC(26e03304) SHA1(c8b271e455dde312c8871dc8dd4d3f0f063fa894) )
	ROM_LOAD16_BYTE( "c21-03.10",  0x40000, 0x20000, CRC(c54888ed) SHA1(8a58da25eb8986a1c6496290e82344840badef0a) )
	ROM_LOAD16_BYTE( "c21-05.14",  0x40001, 0x20000, CRC(834018d2) SHA1(0b1a29316f90a98478b47d7fa3f05c68e5ddd9b3) )

	ROM_REGION( 0x80000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "c21-02.9",  0x00000, 0x80000, CRC(205883b9) SHA1(5aafee8cab3f949a7db91bcc26912f331041b51e) )	/* SCR tiles (8 x 8) */

	ROM_REGION( 0x80000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "c21-01.1",  0x00000, 0x80000, CRC(1ff6f39c) SHA1(742f296efc8073fafa73da2c8d7d26ca9514b6bf) )	/* Sprites (16 x 16) */

	ROM_REGION( 0x1c000, REGION_CPU2, 0 )	/* sound cpu */
	ROM_LOAD( "c21-08.38",   0x00000, 0x04000, CRC(dca495a0) SHA1(4e0f401f1b967da75f33fd7294860ad0b4bf2dce) )
	ROM_CONTINUE(            0x10000, 0x0c000 )	/* banked stuff */

	ROM_REGION( 0x10000, REGION_SOUND1, 0 )
	/* empty region */

	ROM_REGION( 0x08000, REGION_USER1, 0 )	/* 2 machine interface mcu rom ? */
	ROM_LOAD( "c21-07.57",   0x00000, 0x08000, CRC(f02292bd) SHA1(0a5c06a048ad67f90e0d766b504582e9eef035f7) )
ROM_END

ROM_START( cadashu )
	ROM_REGION( 0x80000, REGION_CPU1, 0 )     /* 512k for 68000 code */
	ROM_LOAD16_BYTE( "c21-14-2.11",  0x00000, 0x20000, CRC(f823d418) SHA1(5b4a0b42fb5a2e1ba1e25465762cdc24c41b33f8) )
	ROM_LOAD16_BYTE( "c21-16-2.15",  0x00001, 0x20000, CRC(90165577) SHA1(b8e163cf60933aaaa53873fbc866d8d1750240ab) )
	ROM_LOAD16_BYTE( "c21-13-2.10",  0x40000, 0x20000, CRC(92dcc3ae) SHA1(7d11c6d8b54468f0c56b4f58adc176e4d46a62eb) )
	ROM_LOAD16_BYTE( "c21-15-2.14",  0x40001, 0x20000, CRC(f915d26a) SHA1(cdc7e6a35077ebff937350aee1eee332352e9383) )

	ROM_REGION( 0x80000, REGION_GFX1, ROMREGION_DISPOSE )
	/* bad dump so used checksum from other sets */ /**/
	ROM_LOAD( "c21-02.9",  0x00000, 0x80000, CRC(205883b9) SHA1(5aafee8cab3f949a7db91bcc26912f331041b51e) )	/* SCR tiles (8 x 8) */

	ROM_REGION( 0x80000, REGION_GFX2, ROMREGION_DISPOSE )
	/* bad dump so used checksum from other sets */ /**/
	ROM_LOAD( "c21-01.1",  0x00000, 0x80000, CRC(1ff6f39c) SHA1(742f296efc8073fafa73da2c8d7d26ca9514b6bf) )	/* Sprites (16 x 16) */

	ROM_REGION( 0x1c000, REGION_CPU2, 0 )	/* sound cpu */
	ROM_LOAD( "c21-08.38",   0x00000, 0x04000, CRC(dca495a0) SHA1(4e0f401f1b967da75f33fd7294860ad0b4bf2dce) )
	ROM_CONTINUE(            0x10000, 0x0c000 )	/* banked stuff */

	ROM_REGION( 0x10000, REGION_SOUND1, 0 )
	/* empty region */

	ROM_REGION( 0x08000, REGION_USER1, 0 )	/* 2 machine interface mcu rom ? */
	ROM_LOAD( "c21-07.57",   0x00000, 0x08000, CRC(f02292bd) SHA1(0a5c06a048ad67f90e0d766b504582e9eef035f7) )
ROM_END

ROM_START( cadashi )
	ROM_REGION( 0x80000, REGION_CPU1, 0 )     /* 512k for 68000 code */
	ROM_LOAD16_BYTE( "c21-14it",  0x00000, 0x20000, CRC(d1d9e613) SHA1(296c188daec962bdb4e78e20f1cc4c7d1f4dda09) )
	ROM_LOAD16_BYTE( "c21-16it",  0x00001, 0x20000, CRC(142256ef) SHA1(9ffc64d7c900bfa0300de9e6d18c7458f4c76ed7) )
	ROM_LOAD16_BYTE( "c21-13it",  0x40000, 0x20000, CRC(c9cf6e30) SHA1(872c871cd60e0aa7149660277f67f90748d82743) )
	ROM_LOAD16_BYTE( "c21-17it",  0x40001, 0x20000, CRC(641fc9dd) SHA1(1497e39f6b250de39ef2785aaca7e68a803612fa) )

	ROM_REGION( 0x80000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "c21-02.9",  0x00000, 0x80000, CRC(205883b9) SHA1(5aafee8cab3f949a7db91bcc26912f331041b51e) )	/* SCR tiles (8 x 8) */

	ROM_REGION( 0x80000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "c21-01.1",  0x00000, 0x80000, CRC(1ff6f39c) SHA1(742f296efc8073fafa73da2c8d7d26ca9514b6bf) )	/* Sprites (16 x 16) */

	ROM_REGION( 0x1c000, REGION_CPU2, 0 )	/* sound cpu */
	ROM_LOAD( "c21-08.38",   0x00000, 0x04000, CRC(dca495a0) SHA1(4e0f401f1b967da75f33fd7294860ad0b4bf2dce) )
	ROM_CONTINUE(            0x10000, 0x0c000 )	/* banked stuff */

	ROM_REGION( 0x10000, REGION_SOUND1, 0 )
	/* empty region */

	ROM_REGION( 0x08000, REGION_USER1, 0 )	/* 2 machine interface mcu rom ? */
	ROM_LOAD( "c21-07.57",   0x00000, 0x08000, CRC(f02292bd) SHA1(0a5c06a048ad67f90e0d766b504582e9eef035f7) )
ROM_END

ROM_START( cadashf )
	ROM_REGION( 0x80000, REGION_CPU1, 0 )     /* 512k for 68000 code */
	ROM_LOAD16_BYTE( "c21-19",  0x00000, 0x20000, CRC(4d70543b) SHA1(4fc8d4a9f978232a484af3d91bf8eea2afc839a7) )
	ROM_LOAD16_BYTE( "c21-21",  0x00001, 0x20000, CRC(0e5b9950) SHA1(872919bab057fc9e5baffe5dfe35b1b8c1ed0105) )
	ROM_LOAD16_BYTE( "c21-18",  0x40000, 0x20000, CRC(8a19e59b) SHA1(b42a0c8273ca6f202a5dc6e33965423da3b074d8) )
	ROM_LOAD16_BYTE( "c21-20",  0x40001, 0x20000, CRC(b96acfd9) SHA1(d05b55fd5bbf8fd0e5a7272d1951f27a4900371f) )

	ROM_REGION( 0x80000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "c21-02.9",  0x00000, 0x80000, CRC(205883b9) SHA1(5aafee8cab3f949a7db91bcc26912f331041b51e) )	/* SCR tiles (8 x 8) */

	ROM_REGION( 0x80000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "c21-01.1",  0x00000, 0x80000, CRC(1ff6f39c) SHA1(742f296efc8073fafa73da2c8d7d26ca9514b6bf) )	/* Sprites (16 x 16) */

	ROM_REGION( 0x1c000, REGION_CPU2, 0 )	/* sound cpu */
	ROM_LOAD( "c21-08.38",   0x00000, 0x04000, CRC(dca495a0) SHA1(4e0f401f1b967da75f33fd7294860ad0b4bf2dce) )
	ROM_CONTINUE(            0x10000, 0x0c000 )	/* banked stuff */

	ROM_REGION( 0x10000, REGION_SOUND1, 0 )
	/* empty region */

	ROM_REGION( 0x08000, REGION_USER1, 0 )	/* 2 machine interface mcu rom ? */
	ROM_LOAD( "c21-07.57",   0x00000, 0x08000, CRC(f02292bd) SHA1(0a5c06a048ad67f90e0d766b504582e9eef035f7) )
ROM_END

ROM_START( galmedes )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )     /* 1024k for 68000 code */
	ROM_LOAD16_BYTE( "gm-prg1.bin",  0x00000, 0x20000, CRC(32a70753) SHA1(3bd094b7ae600dbc87ba74e8b2d6b86a68346f4f) )
	ROM_LOAD16_BYTE( "gm-prg0.bin",  0x00001, 0x20000, CRC(fae546a4) SHA1(484cad5287daa495b347f6b5b065f3b3d02d8f0e) )
	/* 0x40000 - 0x7ffff is intentionally empty */
	ROM_LOAD16_WORD( "gm-30.rom",    0x80000, 0x80000, CRC(4da2a407) SHA1(7bd0eb629dd7022a16e328612c786c544267f7bc) )	/* Fix ROM */

	ROM_REGION( 0x80000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "gm-scn.bin", 0x00000, 0x80000, CRC(3bab0581) SHA1(56b79a4ffd9f4880a63450b7d1b79f029de75e20) )	/* SCR tiles (8 x 8) */

	ROM_REGION( 0x80000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "gm-obj.bin", 0x00000, 0x80000, CRC(7a4a1315) SHA1(e2010ee4222415fd55ba3102003be4151d29e39b) )	/* Sprites (16 x 16) */

	ROM_REGION( 0x1c000, REGION_CPU2, 0 )	/* sound cpu */
	ROM_LOAD( "gm-snd.bin", 0x00000, 0x04000, CRC(d6f56c21) SHA1(ff9743448ac8ce57a2f8c33a26145e7b92cbe3c3) )
	ROM_CONTINUE(           0x10000, 0x0c000 )	/* banked stuff */

	ROM_REGION( 0x10000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	/* empty region */
ROM_END

ROM_START( earthjkr )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )     /* 1024k for 68000 code */
	ROM_LOAD16_BYTE( "ej_3b.rom",  0x00000, 0x20000, CRC(bdd86fc2) SHA1(96578860ed03718f8a68847b367eac6c81b79ca2) )
	ROM_LOAD16_BYTE( "ej_3a.rom",  0x00001, 0x20000, CRC(9c8050c6) SHA1(076c882f75787e8120de66ff0dcd2cb820513c45) )
	/* 0x40000 - 0x7ffff is intentionally empty */
	ROM_LOAD16_WORD( "ej_30e.rom", 0x80000, 0x80000, CRC(49d1f77f) SHA1(f6c9b2fc88b77cc9baa5be48da5c3eb72310e471) )	/* Fix ROM */

	ROM_REGION( 0x80000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "ej_chr.rom", 0x00000, 0x80000, CRC(ac675297) SHA1(2a34e1eae3a4be84dbf709053f5e8a781b1073fc) )	/* SCR tiles (8 x 8) */

	ROM_REGION( 0xa0000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD       ( "ej_obj.rom", 0x00000, 0x80000, CRC(5f21ac47) SHA1(45c94ffb53ee9b822b0676f6fb151fed4ce6d967) )	/* Sprites (16 x 16) */
	ROM_LOAD16_BYTE( "ej_1.rom",   0x80000, 0x10000, CRC(cb4891db) SHA1(af1112608cdd897ef6028ef617f5ca69d7964861) )
	ROM_LOAD16_BYTE( "ej_0.rom",   0x80001, 0x10000, CRC(b612086f) SHA1(625748fcb698ec57b7b3ce46019cf85de99aaaa1) )

	ROM_REGION( 0x1c000, REGION_CPU2, 0 )	/* sound cpu */
	ROM_LOAD( "ej_2.rom", 0x00000, 0x04000, CRC(42ba2566) SHA1(c437388684b565c7504d6bad6accd73aa000faca) )
	ROM_CONTINUE(         0x10000, 0x0c000 )	/* banked stuff */

	ROM_REGION( 0x10000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	/* empty region */
ROM_END

ROM_START( eto )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )     /* 1024k for 68000 code */
	ROM_LOAD16_BYTE( "eto-1.23",  0x00000, 0x20000, CRC(44286597) SHA1(ac37e5edbf9d187f60232adc5e9ebed45b3d2fe2) )
	ROM_LOAD16_BYTE( "eto-0.8",   0x00001, 0x20000, CRC(57b79370) SHA1(25f83eada982ef654260fe92016d42a90005a05c) )
	/* 0x40000 - 0x7ffff is intentionally empty */
	ROM_LOAD16_WORD( "eto-2.30",    0x80000, 0x80000, CRC(12f46fb5) SHA1(04db8b6ccd0051668bd2930275efa0265c0cfd2b) )	/* Fix ROM */

	ROM_REGION( 0x80000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "eto-4.3", 0x00000, 0x80000, CRC(a8768939) SHA1(a2cbbd3e10ed48ba32a680b2e40ea03900cf33fa) )	/* Sprites (16 x 16) */

	ROM_REGION( 0x80000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "eto-3.6", 0x00000, 0x80000, CRC(dd247397) SHA1(53a7bf877fd7e5f3daf295a698f4012447b6f113) )	/* SCR tiles (8 x 8) */

	ROM_REGION( 0x1c000, REGION_CPU2, 0 )	/* sound cpu */
	ROM_LOAD( "eto-5.27", 0x00000, 0x04000, CRC(b3689da0) SHA1(812d2e0a794403df9f0a5035784f14cd070ea080) )
	ROM_CONTINUE(         0x10000, 0x0c000 )	/* banked stuff */

	ROM_REGION( 0x10000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	/* empty region */
ROM_END

DRIVER_INIT(earthjkr)
{
	data16_t *rom = memory_region(REGION_CPU1);
	/* 357c -> 317c, I think this is bitrot, see ROM loading for which ROM needs redumping, causes rowscroll to be broken on final stage (writes to ROM area instead)
	   code is correct in the 'prototype?' set */
	rom[0x7aaa/2] = 0x317c; 
}


GAME( 1988, bonzeadv, 0,        bonzeadv, bonzeadv, 0,        ROT0,   "Taito Corporation Japan", "Bonze Adventure (World)" )
GAME( 1988, bonzeadu, bonzeadv, bonzeadv, jigkmgri, 0,        ROT0,   "Taito America Corporation", "Bonze Adventure (US)" )
GAME( 1988, jigkmgri, bonzeadv, bonzeadv, jigkmgri, 0,        ROT0,   "Taito Corporation", "Jigoku Meguri (Japan)" )
GAME( 1988, asuka,    0,        asuka,    asuka,    0,        ROT270, "Taito Corporation", "Asuka and Asuka (Japan)" )
GAME( 1989, mofflott, 0,        galmedes, mofflott, 0,        ROT270, "Taito Corporation", "Maze of Flott (Japan)" )
GAME( 1989, cadash,   0,        cadash,   cadash,   0,        ROT0,   "Taito Corporation Japan", "Cadash (World)" )
GAME( 1989, cadashj,  cadash,   cadash,   cadashj,  0,        ROT0,   "Taito Corporation", "Cadash (Japan)" )
GAME( 1989, cadashu,  cadash,   cadash,   cadashu,  0,        ROT0,   "Taito America Corporation", "Cadash (US)" )
GAME( 1989, cadashi,  cadash,   cadash,   cadash,   0,        ROT0,   "Taito Corporation Japan", "Cadash (Italy)" )
GAME( 1989, cadashf,  cadash,   cadash,   cadash,   0,        ROT0,   "Taito Corporation Japan", "Cadash (France)" )
GAME( 1992, galmedes, 0,        galmedes, galmedes, 0,        ROT270, "Visco", "Galmedes (Japan)" )
GAME( 1993, earthjkr, 0,        galmedes, earthjkr, earthjkr, ROT270, "Visco", "U.N. Defense Force - Earth Joker (Japan)" )
GAME( 1994, eto,      0,        eto,      eto,      0,        ROT0,   "Visco", "Kokontouzai Eto Monogatari (Japan)" )
