/***************************************************************************

  Thunder Zone       (c) 1991 Data East Corporation (World 2 players)
  Desert Assault     (c) 1991 Data East Corporation (USA 4 players)
  Desert Assault     (c) 1991 Data East Corporation (USA 2 players)

  I'm not sure if one of the alpha blending effects is correct (mode 0x8000,
  the usual mode 0x4000 should be correct).  It may be some kind of orthogonal
  priority effect where it should cut a hole in other higher priority sprites
  to reveal a non-alpha'd hole, or alpha against a further back tilemap.

  Emulation by Bryan McPhail, mish@tendril.co.uk


Stephh's notes (based on the games M68000 code and some tests) :

0) all games

  - If the "Flip Screen" Dip Switch is correct, screen flipping is handled by
    a write to 0x220000.w (dassault_control_1[0]) which isn't supported yet.
  - I can't confirm the "Demo Sounds" Dip Switch, but the routine is the same
    in all games (but different addresses) and it writes something to 0x180000.w
    (see 'WRITE16_HANDLER( dassault_sound_w )').


1) 'thndzone'

  - "Max Players" Dip Switch set to "2" :

      * COIN1    : adds coin(s)/credit(s) depending on "Coin A" Dip Switch
      * COIN2    : adds coin(s)/credit(s) depending on "Coin B" Dip Switch
      * COIN3    : adds coin(s)/credit(s) depending on "Coin A" Dip Switch
      * COIN4    : adds coin(s)/credit(s) depending on "Coin A" Dip Switch
      * SERVICE1 : adds 4 coins/credits

      * START1   : starts a game for player 1
      * START2   : starts a game for player 2

      * BUTTON1n : "fire"
      * BUTTON2n : "nuke"

  - "Max Players" Dip Switch set to "4" :

      * COIN1    : adds coin(s)/credit(s) depending on "Coin A" Dip Switch
      * COIN2    : adds coin(s)/credit(s) depending on "Coin B" Dip Switch
      * COIN3    : adds coin(s)/credit(s) depending on "Coin A" Dip Switch
      * COIN4    : adds coin(s)/credit(s) depending on "Coin A" Dip Switch
      * SERVICE1 : adds 4 coins/credits

      * START1   : starts a game for player 1
      * START2   : starts a game for player 2

      * BUTTON1n : "fire" + starts a game for player n
      * BUTTON2n : "nuke"


2) 'dassault'

  - "Max Players" Dip Switch set to "2" :

      * COIN1    : adds coin(s)/credit(s) depending on "Coin A" Dip Switch
      * COIN2    : adds coin(s)/credit(s) depending on "Coin B" Dip Switch
      * COIN3    : NO EFFECT !
      * COIN4    : NO EFFECT !
      * SERVICE1 : adds 1 coin/credit

      * START1   : starts a game for player 1
      * START2   : starts a game for player 2

      * BUTTON1n : "fire"
      * BUTTON2n : "nuke"

  - "Max Players" Dip Switch set to "3" :

      * COIN1    : adds coin(s)/credit(s) for player 1 depending on "Coin A" Dip Switch
      * COIN2    : adds coin(s)/credit(s) for player 2 depending on "Coin A" Dip Switch
      * COIN3    : adds coin(s)/credit(s) for player 3 depending on "Coin A" Dip Switch
      * COIN4    : adds coin(s)/credit(s) for FAKE player 4 depending on "Coin A" Dip Switch
      * SERVICE1 : adds 1 coin/credit for all players (including FAKE player 4 !)

      * START1   : NO EFFECT !
      * START2   : NO EFFECT !

      * BUTTON1n : "fire" + starts a game for player n
      * BUTTON2n : "nuke"

  - "Max Players" Dip Switch set to "4" :

      * COIN1    : adds coin(s)/credit(s) for player 1 depending on "Coin A" Dip Switch
      * COIN2    : adds coin(s)/credit(s) for player 2 depending on "Coin A" Dip Switch
      * COIN3    : adds coin(s)/credit(s) for player 3 depending on "Coin A" Dip Switch
      * COIN4    : adds coin(s)/credit(s) for player 4 depending on "Coin A" Dip Switch
      * SERVICE1 : adds 1 coin/credit for all players

      * START1   : NO EFFECT !
      * START2   : NO EFFECT !

      * BUTTON1n : "fire" + starts a game for player n
      * BUTTON2n : "nuke"


3) 'dassaul4'

  - always 4 players :

      * COIN1    : adds coin(s)/credit(s) for player 1 depending on "Coinage" Dip Switch
      * COIN2    : adds coin(s)/credit(s) for player 2 depending on "Coinage" Dip Switch
      * COIN3    : adds coin(s)/credit(s) for player 3 depending on "Coinage" Dip Switch
      * COIN4    : adds coin(s)/credit(s) for player 4 depending on "Coinage" Dip Switch
      * SERVICE1 : adds 1 coin/credit

      * NO START1 !
      * NO START2 !

      * BUTTON1n : "fire" + starts a game for player n
      * BUTTON2n : "nuke"


***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "cpu/h6280/h6280.h"
#include "deco16ic.h"

VIDEO_START( dassault );
VIDEO_UPDATE( dassault );

static data16_t *dassault_ram,*shared_ram,*dassault_ram2;

/**********************************************************************************/

static READ16_HANDLER( dassault_control_r )
{
	switch (offset<<1)
	{
		case 0: /* Player 1 & Player 2 joysticks & fire buttons */
			return (readinputport(0) + (readinputport(1) << 8));

		case 2: /* Player 3 & Player 4 joysticks & fire buttons */
			return (readinputport(6) + (readinputport(7) << 8));

		case 4: /* Dip 1 (stored at 0x3f8035) */
			return readinputport(3);

		case 6: /* Dip 2 (stored at 0x3f8034) */
			return readinputport(4);

		case 8: /* VBL, Credits */
			return readinputport(2);
	}

	return 0xffff;
}

static WRITE16_HANDLER( dassault_control_w )
{
	coin_counter_w(0,data&1);
	if (data&0xfffe)
		log_cb(RETRO_LOG_DEBUG, LOGPRE "Coin cointrol %04x\n",data);
}

static READ16_HANDLER( dassault_sub_control_r )
{
	return readinputport(5);
}

static WRITE16_HANDLER( dassault_sound_w )
{
	soundlatch_w(0,data&0xff);
	cpu_set_irq_line(2,0,HOLD_LINE); /* IRQ1 */
}

/* The CPU-CPU irq controller is overlaid onto the end of the shared memory */
static READ16_HANDLER( dassault_irq_r )
{
	switch (offset) {
		case 0: cpu_set_irq_line(0, 5, CLEAR_LINE); break;
		case 1: cpu_set_irq_line(1, 6, CLEAR_LINE); break;
	}
	return shared_ram[(0xffc/2)+offset]; /* The values probably don't matter */
}

static WRITE16_HANDLER( dassault_irq_w )
{
	switch (offset) {
		case 0: cpu_set_irq_line(0, 5, ASSERT_LINE); break;
		case 1: cpu_set_irq_line(1, 6, ASSERT_LINE); break;
	}

	COMBINE_DATA(&shared_ram[(0xffc/2)+offset]); /* The values probably don't matter */
}

static WRITE16_HANDLER( shared_ram_w )
{
	COMBINE_DATA(&shared_ram[offset]);
}

static READ16_HANDLER( shared_ram_r )
{
	return shared_ram[offset];
}

/**********************************************************************************/

static MEMORY_READ16_START( dassault_readmem )
	{ 0x000000, 0x07ffff, MRA16_ROM },
	{ 0x100000, 0x103fff, MRA16_RAM },
	{ 0x1c0000, 0x1c000f, dassault_control_r },
	{ 0x200000, 0x201fff, MRA16_RAM },
	{ 0x202000, 0x203fff, MRA16_RAM },
	{ 0x240000, 0x240fff, MRA16_RAM },
	{ 0x242000, 0x242fff, MRA16_RAM },
	{ 0x3f8000, 0x3fbfff, MRA16_RAM }, /* Main ram */
	{ 0x3fc000, 0x3fcfff, MRA16_RAM }, /* Spriteram (2nd) */
	{ 0x3feffc, 0x3fefff, dassault_irq_r },
	{ 0x3fe000, 0x3fefff, shared_ram_r }, /* Shared ram */
MEMORY_END

static MEMORY_WRITE16_START( dassault_writemem )
	{ 0x000000, 0x07ffff, MWA16_ROM },
	{ 0x100000, 0x103fff, deco16_nonbuffered_palette_w, &paletteram16 },
	{ 0x140004, 0x140007, MWA16_NOP }, /* ? */
	{ 0x180000, 0x180001, dassault_sound_w },
	{ 0x1c000a, 0x1c000b, deco16_priority_w },
	{ 0x1c000c, 0x1c000d, buffer_spriteram16_2_w },
	{ 0x1c000e, 0x1c000f, dassault_control_w },

	{ 0x200000, 0x201fff, deco16_pf1_data_w, &deco16_pf1_data },
	{ 0x202000, 0x203fff, deco16_pf2_data_w, &deco16_pf2_data },
	{ 0x212000, 0x212fff, MWA16_RAM, &deco16_pf2_rowscroll },
	{ 0x220000, 0x22000f, MWA16_RAM, &deco16_pf12_control },

	{ 0x240000, 0x240fff, deco16_pf3_data_w, &deco16_pf3_data },
	{ 0x242000, 0x242fff, deco16_pf4_data_w, &deco16_pf4_data },
	{ 0x252000, 0x252fff, MWA16_RAM, &deco16_pf4_rowscroll },
	{ 0x260000, 0x26000f, MWA16_RAM, &deco16_pf34_control },

	{ 0x3f8000, 0x3fbfff, MWA16_RAM, &dassault_ram },
	{ 0x3fc000, 0x3fcfff, MWA16_RAM, &spriteram16_2, &spriteram_2_size },
	{ 0x3feffc, 0x3fefff, dassault_irq_w },
	{ 0x3fe000, 0x3fefff, shared_ram_w, &shared_ram },
MEMORY_END

static MEMORY_READ16_START( dassault_sub_readmem )
	{ 0x000000, 0x07ffff, MRA16_ROM },
	{ 0x100004, 0x100005, dassault_sub_control_r },
	{ 0x3f8000, 0x3fbfff, MRA16_RAM }, /* Sub cpu ram */
	{ 0x3fc000, 0x3fcfff, MRA16_RAM }, /* Sprite ram */
	{ 0x3feffc, 0x3fefff, dassault_irq_r },
	{ 0x3fe000, 0x3fefff, shared_ram_r },
MEMORY_END

static MEMORY_WRITE16_START( dassault_sub_writemem )
	{ 0x000000, 0x07ffff, MWA16_ROM },
	{ 0x100000, 0x100001, buffer_spriteram16_w },
	{ 0x100002, 0x100007, MWA16_NOP }, /* ? */
	{ 0x3f8000, 0x3fbfff, MWA16_RAM, &dassault_ram2 },
	{ 0x3fc000, 0x3fcfff, MWA16_RAM, &spriteram16, &spriteram_size },
	{ 0x3feffc, 0x3fefff, dassault_irq_w },
	{ 0x3fe000, 0x3fefff, shared_ram_w },
MEMORY_END

/******************************************************************************/

static MEMORY_READ_START( sound_readmem )
	{ 0x000000, 0x00ffff, MRA_ROM },
	{ 0x100000, 0x100001, YM2203_status_port_0_r },
	{ 0x110000, 0x110001, YM2151_status_port_0_r },
	{ 0x120000, 0x120001, OKIM6295_status_0_r },
	{ 0x130000, 0x130001, OKIM6295_status_1_r },
	{ 0x140000, 0x140001, soundlatch_r },
	{ 0x1f0000, 0x1f1fff, MRA_BANK8 },
MEMORY_END

static MEMORY_WRITE_START( sound_writemem )
	{ 0x000000, 0x00ffff, MWA_ROM },
	{ 0x100000, 0x100001, YM2203_word_0_w },
	{ 0x110000, 0x110001, YM2151_word_0_w },
	{ 0x120000, 0x120001, OKIM6295_data_0_w },
	{ 0x130000, 0x130001, OKIM6295_data_1_w },
	{ 0x1f0000, 0x1f1fff, MWA_BANK8 },
	{ 0x1fec00, 0x1fec01, H6280_timer_w },
	{ 0x1ff402, 0x1ff403, H6280_irq_status_w },
MEMORY_END

/**********************************************************************************/

#define DASSAULT_PLAYER_INPUT( player, start ) \
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | player ) \
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | player ) \
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | player ) \
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | player ) \
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | player ) \
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | player ) \
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED ) \
	PORT_BIT( 0x80, IP_ACTIVE_LOW, start )


INPUT_PORTS_START( thndzone )
	PORT_START
	DASSAULT_PLAYER_INPUT( IPF_PLAYER1, IPT_START1 )

	PORT_START
	DASSAULT_PLAYER_INPUT( IPF_PLAYER2, IPT_START2 )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE1 )			/* Adds 4 credits/coins !*/
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_VBLANK )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* Dip switch bank 1 */
	PORT_DIPNAME( 0x07, 0x07, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x07, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x05, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 1C_6C ) )
	PORT_DIPNAME( 0x38, 0x38, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x38, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x28, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x18, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 1C_6C ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, "2 Coins to Start, 1 to Continue" )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START	/* Dip switch bank 2 */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x08, "Easy" )
	PORT_DIPSETTING(    0x0c, "Normal" )
	PORT_DIPSETTING(    0x04, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x00, "Max Players" )
	PORT_DIPSETTING(    0x20, "2" )
	PORT_DIPSETTING(    0x00, "4" )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Demo_Sounds ) )		/* Check code at 0x001490*/
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START /* Cpu 1 vblank */
	PORT_BIT( 0xff, IP_ACTIVE_HIGH, IPT_VBLANK )

	PORT_START
	DASSAULT_PLAYER_INPUT( IPF_PLAYER3, IPT_COIN3 )

	PORT_START
	DASSAULT_PLAYER_INPUT( IPF_PLAYER4, IPT_COIN4 )
INPUT_PORTS_END

INPUT_PORTS_START( dassault )
	PORT_START
	DASSAULT_PLAYER_INPUT( IPF_PLAYER1, IPT_START1 )

	PORT_START
	DASSAULT_PLAYER_INPUT( IPF_PLAYER2, IPT_START2 )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_VBLANK )

	PORT_START	/* Dip switch bank 1 */
	PORT_DIPNAME( 0x07, 0x07, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x07, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x05, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 1C_6C ) )
	PORT_DIPNAME( 0x38, 0x38, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x38, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x28, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x18, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 1C_6C ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, "2 Coins to Start, 1 to Continue" )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START	/* Dip switch bank 2 */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x08, "Easy" )
	PORT_DIPSETTING(    0x0c, "Normal" )
	PORT_DIPSETTING(    0x04, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_DIPNAME( 0x30, 0x10, "Max Players" )
	PORT_DIPSETTING(    0x30, "2" )
	PORT_DIPSETTING(    0x20, "3" )
	PORT_DIPSETTING(    0x10, "4" )
/*	PORT_DIPSETTING(    0x00, "4 (buggy)" )*/
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Demo_Sounds ) )		/* Check code at 0x0014bc*/
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START /* Cpu 1 vblank */
	PORT_BIT( 0xff, IP_ACTIVE_HIGH, IPT_VBLANK )

	PORT_START
	DASSAULT_PLAYER_INPUT( IPF_PLAYER3, IPT_COIN3 )

	PORT_START
	DASSAULT_PLAYER_INPUT( IPF_PLAYER4, IPT_COIN4 )
INPUT_PORTS_END

INPUT_PORTS_START( dassaul4 )
	PORT_START
	DASSAULT_PLAYER_INPUT( IPF_PLAYER1, IPT_UNUSED )

	PORT_START
	DASSAULT_PLAYER_INPUT( IPF_PLAYER2, IPT_UNUSED )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_VBLANK )

	PORT_START	/* Dip switch bank 1 */
	PORT_DIPNAME( 0x07, 0x07, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x07, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x05, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 1C_6C ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, "2 Coins to Start, 1 to Continue" )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START	/* Dip switch bank 2 */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x08, "Easy" )
	PORT_DIPSETTING(    0x0c, "Normal" )
	PORT_DIPSETTING(    0x04, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Demo_Sounds ) )		/* Check code at 0x0014a4*/
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START /* Cpu 1 vblank */
	PORT_BIT( 0xff, IP_ACTIVE_HIGH, IPT_VBLANK )

	PORT_START
	DASSAULT_PLAYER_INPUT( IPF_PLAYER3, IPT_COIN3 )

	PORT_START
	DASSAULT_PLAYER_INPUT( IPF_PLAYER4, IPT_COIN4 )
INPUT_PORTS_END

/**********************************************************************************/

static struct GfxLayout charlayout =
{
	8,8,
	RGN_FRAC(1,2),
	4,
	{ RGN_FRAC(1,2)+8, RGN_FRAC(1,2), 8, 0 },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16 },
	16*8
};

static struct GfxLayout tilelayout =
{
	16,16,
	RGN_FRAC(1,2),
	4,
	{ RGN_FRAC(1,2)+8, RGN_FRAC(1,2), 8, 0 },
	{ 32*8+0, 32*8+1, 32*8+2, 32*8+3, 32*8+4, 32*8+5, 32*8+6, 32*8+7,
		0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16,
			8*16, 9*16, 10*16, 11*16, 12*16, 13*16, 14*16, 15*16 },
	64*8
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	/* REGION_GFX1 is copied to REGION_GFX2 at runtime */
	{ REGION_GFX2, 0, &charlayout,     0,  32 },	/* Characters 8x8 */
	{ REGION_GFX2, 0, &tilelayout,     0,  32 },	/* Tiles 16x16 */
	{ REGION_GFX3, 0, &tilelayout,   512,  32 },	/* Tiles 16x16 */
	{ REGION_GFX4, 0, &tilelayout,  1024,  64 },	/* Sprites 16x16 */
	{ REGION_GFX5, 0, &tilelayout,  2048,  64 },	/* Sprites 16x16 */
	{ -1 } /* end of array */
};

/**********************************************************************************/

static struct YM2203interface ym2203_interface =
{
	1,
	32220000/8,	/* Accurate, audio section crystal is 32.220 MHz */
	{ YM2203_VOL(40,40) },
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 }
};

static void sound_irq(int state)
{
	cpu_set_irq_line(2,1,state);
}

WRITE_HANDLER( sound_bankswitch_w )
{
	/* the second OKIM6295 ROM is bank switched */
	OKIM6295_set_bank_base(1, (data & 1) * 0x40000);
}

static struct YM2151interface ym2151_interface =
{
	1,
	32220000/9, /* Accurate, audio section crystal is 32.220 MHz */
	{ YM3012_VOL(45,MIXER_PAN_LEFT,45,MIXER_PAN_RIGHT) },
	{ sound_irq },
	{ sound_bankswitch_w }
};

static struct OKIM6295interface okim6295_interface =
{
	2,              /* 2 chips */
	{ 7757, 15514 },/* Frequency */
	{ REGION_SOUND1, REGION_SOUND2 },
	{ 50, 25 }		/* Note!  Keep chip 1 louder than chip 2 */
};

/**********************************************************************************/

static MACHINE_DRIVER_START( dassault )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 14000000) /* Accurate */
	MDRV_CPU_MEMORY(dassault_readmem,dassault_writemem)
	MDRV_CPU_VBLANK_INT(irq4_line_hold,1)

	MDRV_CPU_ADD(M68000, 14000000) /* Accurate */
	MDRV_CPU_MEMORY(dassault_sub_readmem,dassault_sub_writemem)
	MDRV_CPU_VBLANK_INT(irq5_line_hold,1)

	MDRV_CPU_ADD(H6280,32220000/8)
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)	/* Accurate */
	MDRV_CPU_MEMORY(sound_readmem,sound_writemem)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(529)
	MDRV_INTERLEAVE(140) /* 140 CPU slices per frame */

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_RGB_DIRECT | VIDEO_NEEDS_6BITS_PER_GUN | VIDEO_BUFFERS_SPRITERAM)
	MDRV_SCREEN_SIZE(40*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 40*8-1, 1*8, 31*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(4096)

	MDRV_VIDEO_START(dassault)
	MDRV_VIDEO_UPDATE(dassault)

	/* sound hardware */
	MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
	MDRV_SOUND_ADD(YM2203, ym2203_interface)
	MDRV_SOUND_ADD(YM2151, ym2151_interface)
	MDRV_SOUND_ADD(OKIM6295, okim6295_interface)
MACHINE_DRIVER_END

/**********************************************************************************/

ROM_START( dassault )
	ROM_REGION(0x80000, REGION_CPU1, 0 ) /* 68000 code */
	ROM_LOAD16_BYTE("01.bin", 0x00000, 0x20000, CRC(14f17ea7) SHA1(0bb8b7dba05f1ea42e68838861f0d4c263eac6b3) )
	ROM_LOAD16_BYTE("03.bin", 0x00001, 0x20000, CRC(bed1b90c) SHA1(c100f89b69025e2ff885b35a733abc627da98a07) )
	ROM_LOAD16_BYTE("gs00",   0x40000, 0x20000, CRC(b7277175) SHA1(ffb19c4dd12e0391f01de57c46a7998885fe22bf) )
	ROM_LOAD16_BYTE("gs02",   0x40001, 0x20000, CRC(cde31e35) SHA1(0219845308c9f46e73b0504bd2aefa2fa74f388e) )

	ROM_REGION(0x80000, REGION_CPU2, 0 ) /* 68000 code (Sub cpu) */
	ROM_LOAD16_BYTE("hc10-1.bin", 0x00000, 0x20000, CRC(ac5ac770) SHA1(bf6640900c2f9c8091168bf106edf85350c34652) )
	ROM_LOAD16_BYTE("hc08-1.bin", 0x00001, 0x20000, CRC(864dca56) SHA1(0967f613684b539d10b67e4f6033c890e2134ea2) )
	ROM_LOAD16_BYTE("gs11",       0x40000, 0x20000, CRC(80cb23de) SHA1(d52426460eea2285c57cfc3fe37aa6dc79990e25) )
	ROM_LOAD16_BYTE("gs09",       0x40001, 0x20000, CRC(0a8fa7e1) SHA1(330ae9602b5f56b5dc4961a41991b64412a59880) )

	ROM_REGION(0x10000, REGION_CPU3, 0 ) /* Sound CPU */
	ROM_LOAD( "gs04",    0x00000, 0x10000, CRC(81c29ebf) SHA1(1b241277a8e35cdeaeb120970d14a09d33032459) )

	ROM_REGION(0x020000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD16_BYTE( "gs05", 0x000000, 0x10000, CRC(0aae996a) SHA1(d37a12b057e9934212362d7eafa575c961819a27) )
	ROM_LOAD16_BYTE( "gs06", 0x000001, 0x10000, CRC(4efdf03d) SHA1(835d22829c6d0f4efc76801b449f9a779f460f1c) )

	ROM_REGION(0x120000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "maj-02", 0x000000, 0x100000, CRC(383bbc37) SHA1(c537ab147a2770ce28ee185b08dd62d35249bfa9) )
	/* Other 0x20000 filled in later */

	ROM_REGION(0x200000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "maj-01", 0x000000, 0x100000, CRC(9840a204) SHA1(096c351769da5184c3d9a05495370134acc9507a) )
	ROM_LOAD( "maj-00", 0x100000, 0x100000, CRC(87ea8d16) SHA1(db47123aa2ebbb800cfc5cfcf50309bc39cadbcd) )

	ROM_REGION( 0x400000, REGION_GFX4, ROMREGION_DISPOSE ) /* sprites chip 1 */
	ROM_LOAD( "maj-04", 0x000000, 0x80000, CRC(36e49b19) SHA1(bfbc45b635bf3d46ff8b8a514a3f352bf3a95535) )
	ROM_LOAD( "maj-05", 0x080000, 0x80000, CRC(80fc71cc) SHA1(65b15afbe5d628051b012777d486b6ce92a3795c) )
	ROM_LOAD( "maj-06", 0x100000, 0x80000, CRC(2e7a684b) SHA1(cffeda1a816dad30d6b1cb12458661188d625d40) )
	ROM_LOAD( "maj-07", 0x180000, 0x80000, CRC(3acc1f78) SHA1(87ec65b4f54a66370754534d03f4c9217531b42f) )
	ROM_LOAD( "maj-08", 0x200000, 0x80000, CRC(1958a36d) SHA1(466a30dcd2ea13028272ed2187f890ee20d6636b) )
	ROM_LOAD( "maj-09", 0x280000, 0x80000, CRC(c21087a1) SHA1(b769c5f2f9b9c525d121902fe9557a6bfc077b99) )
	ROM_LOAD( "maj-10", 0x300000, 0x80000, CRC(a02fa641) SHA1(14b999a441964e612700bf21945a948eaebb253e) )
	ROM_LOAD( "maj-11", 0x380000, 0x80000, CRC(dabe9305) SHA1(44d69fe55e674de7f4c610d295d4528d4b2eb150) )

	ROM_REGION( 0x80000, REGION_GFX5, ROMREGION_DISPOSE ) /* sprites chip 2 */
	ROM_LOAD16_BYTE( "gs12",   0x000000, 0x20000, CRC(9a86a015) SHA1(968576b8422393ab9a93d98c15428b1c11417b3d) )
	ROM_LOAD16_BYTE( "gs13",   0x000001, 0x20000, CRC(f4709905) SHA1(697842a3d7bc2588c77833c3af8938e6f0b1238d) )
	ROM_LOAD16_BYTE( "gs14",   0x040000, 0x20000, CRC(750fc523) SHA1(ef8794359ff3a44a97ab402821fbe205a0be8f6a) )
	ROM_LOAD16_BYTE( "gs15",   0x040001, 0x20000, CRC(f14edd3d) SHA1(802d576df6dac2c9bf99f963f1955fc3a7ffdac0) )

	ROM_REGION(0x20000, REGION_SOUND1, 0 ) /* Oki samples */
	ROM_LOAD( "gs07",  0x00000,  0x20000,  CRC(750b7e5d) SHA1(d33b17a1d8c9b05d5c1daf0c80fed6381e04b167) )

	ROM_REGION(0x80000, REGION_SOUND2, 0 ) /* Extra Oki samples */
	ROM_LOAD( "maj-03", 0x00000, 0x80000,  CRC(31dcfac3) SHA1(88c7fc139f871991defbc8dc2c9c66b150dd6f6f) )	/* banked */

	ROM_REGION( 0x1000, REGION_PROMS, 0 )
	ROM_LOAD( "mb7128y.10m", 0x00000,  0x800,  CRC(bde780a2) SHA1(94ea9fe6c3a421e976d077e67f564ca5c37a5e88) )	/* Priority?  Unused */
	ROM_LOAD( "mb7128y.16p", 0x00800,  0x800,  CRC(c44d2751) SHA1(7c195650689d5cbbdccba696e0e7d3dc5bb7c506) )	/* Timing??  Unused */
	/* Above prom also at 16s and 17s */
ROM_END

ROM_START( dassaul4 )
	ROM_REGION(0x80000, REGION_CPU1, 0 ) /* 68000 code */
	ROM_LOAD16_BYTE("gs01", 0x00000, 0x20000, CRC(8613634d) SHA1(69b64e54fde3b5f1ee3435d7327b84e7a7d43f6d) )
	ROM_LOAD16_BYTE("gs03", 0x00001, 0x20000, CRC(ea860bd4) SHA1(6e4e2d004433ad5842b4bc895eaa8f55bd1ee168) )
	ROM_LOAD16_BYTE("gs00", 0x40000, 0x20000, CRC(b7277175) SHA1(ffb19c4dd12e0391f01de57c46a7998885fe22bf) )
	ROM_LOAD16_BYTE("gs02", 0x40001, 0x20000, CRC(cde31e35) SHA1(0219845308c9f46e73b0504bd2aefa2fa74f388e) )

	ROM_REGION(0x80000, REGION_CPU2, 0 ) /* 68000 code (Sub cpu) */
	ROM_LOAD16_BYTE("gs10",   0x00000, 0x20000, CRC(285f72a3) SHA1(d01972aec500805ca1abed14983064cd14e942d4) )
	ROM_LOAD16_BYTE("gs08",   0x00001, 0x20000, CRC(16691ede) SHA1(dc481dfc6104833a6fd18be6275e77ecc0510165) )
	ROM_LOAD16_BYTE("gs11",   0x40000, 0x20000, CRC(80cb23de) SHA1(d52426460eea2285c57cfc3fe37aa6dc79990e25) )
	ROM_LOAD16_BYTE("gs09",   0x40001, 0x20000, CRC(0a8fa7e1) SHA1(330ae9602b5f56b5dc4961a41991b64412a59880) )

	ROM_REGION(0x10000, REGION_CPU3, 0 ) /* Sound CPU */
	ROM_LOAD( "gs04",    0x00000, 0x10000, CRC(81c29ebf) SHA1(1b241277a8e35cdeaeb120970d14a09d33032459) )

	ROM_REGION(0x020000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD16_BYTE( "gs05", 0x000000, 0x10000, CRC(0aae996a) SHA1(d37a12b057e9934212362d7eafa575c961819a27) )
	ROM_LOAD16_BYTE( "gs06", 0x000001, 0x10000, CRC(4efdf03d) SHA1(835d22829c6d0f4efc76801b449f9a779f460f1c) )

	ROM_REGION(0x120000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "maj-02", 0x000000, 0x100000, CRC(383bbc37) SHA1(c537ab147a2770ce28ee185b08dd62d35249bfa9) )
	/* Other 0x20000 filled in later */

	ROM_REGION(0x200000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "maj-01", 0x000000, 0x100000, CRC(9840a204) SHA1(096c351769da5184c3d9a05495370134acc9507a) )
	ROM_LOAD( "maj-00", 0x100000, 0x100000, CRC(87ea8d16) SHA1(db47123aa2ebbb800cfc5cfcf50309bc39cadbcd) )

	ROM_REGION( 0x400000, REGION_GFX4, ROMREGION_DISPOSE ) /* sprites chip 1 */
	ROM_LOAD( "maj-04", 0x000000, 0x80000, CRC(36e49b19) SHA1(bfbc45b635bf3d46ff8b8a514a3f352bf3a95535) )
	ROM_LOAD( "maj-05", 0x080000, 0x80000, CRC(80fc71cc) SHA1(65b15afbe5d628051b012777d486b6ce92a3795c) )
	ROM_LOAD( "maj-06", 0x100000, 0x80000, CRC(2e7a684b) SHA1(cffeda1a816dad30d6b1cb12458661188d625d40) )
	ROM_LOAD( "maj-07", 0x180000, 0x80000, CRC(3acc1f78) SHA1(87ec65b4f54a66370754534d03f4c9217531b42f) )
	ROM_LOAD( "maj-08", 0x200000, 0x80000, CRC(1958a36d) SHA1(466a30dcd2ea13028272ed2187f890ee20d6636b) )
	ROM_LOAD( "maj-09", 0x280000, 0x80000, CRC(c21087a1) SHA1(b769c5f2f9b9c525d121902fe9557a6bfc077b99) )
	ROM_LOAD( "maj-10", 0x300000, 0x80000, CRC(a02fa641) SHA1(14b999a441964e612700bf21945a948eaebb253e) )
	ROM_LOAD( "maj-11", 0x380000, 0x80000, CRC(dabe9305) SHA1(44d69fe55e674de7f4c610d295d4528d4b2eb150) )

	ROM_REGION( 0x80000, REGION_GFX5, ROMREGION_DISPOSE ) /* sprites chip 2 */
	ROM_LOAD16_BYTE( "gs12",   0x000000, 0x20000, CRC(9a86a015) SHA1(968576b8422393ab9a93d98c15428b1c11417b3d) )
	ROM_LOAD16_BYTE( "gs13",   0x000001, 0x20000, CRC(f4709905) SHA1(697842a3d7bc2588c77833c3af8938e6f0b1238d) )
	ROM_LOAD16_BYTE( "gs14",   0x040000, 0x20000, CRC(750fc523) SHA1(ef8794359ff3a44a97ab402821fbe205a0be8f6a) )
	ROM_LOAD16_BYTE( "gs15",   0x040001, 0x20000, CRC(f14edd3d) SHA1(802d576df6dac2c9bf99f963f1955fc3a7ffdac0) )

	ROM_REGION(0x20000, REGION_SOUND1, 0 ) /* Oki samples */
	ROM_LOAD( "gs07",  0x00000,  0x20000,  CRC(750b7e5d) SHA1(d33b17a1d8c9b05d5c1daf0c80fed6381e04b167) )

	ROM_REGION(0x80000, REGION_SOUND2, 0 ) /* Extra Oki samples */
	ROM_LOAD( "maj-03", 0x00000, 0x80000,  CRC(31dcfac3) SHA1(88c7fc139f871991defbc8dc2c9c66b150dd6f6f) )	/* banked */

	ROM_REGION( 0x1000, REGION_PROMS, 0 )
	ROM_LOAD( "mb7128y.10m", 0x00000,  0x800,  CRC(bde780a2) SHA1(94ea9fe6c3a421e976d077e67f564ca5c37a5e88) )	/* Priority?  Unused */
	ROM_LOAD( "mb7128y.16p", 0x00800,  0x800,  CRC(c44d2751) SHA1(7c195650689d5cbbdccba696e0e7d3dc5bb7c506) )	/* Timing??  Unused */
	/* Above prom also at 16s and 17s */
ROM_END

ROM_START( thndzone )
	ROM_REGION(0x80000, REGION_CPU1, 0 ) /* 68000 code */
	ROM_LOAD16_BYTE("gz_01.bin", 0x00000, 0x20000, CRC(15e8c328) SHA1(8876b5fde77604c2fe4654271ceb341a8fa460c1) )
	ROM_LOAD16_BYTE("gz_03.bin", 0x00001, 0x20000, CRC(aab5c86e) SHA1(c3560b15360ddf14e8444d9f70724e698b2bd42f) )
	ROM_LOAD16_BYTE("gs00",   0x40000, 0x20000, CRC(b7277175) SHA1(ffb19c4dd12e0391f01de57c46a7998885fe22bf) ) /* Aka GT00 */
	ROM_LOAD16_BYTE("gs02",   0x40001, 0x20000, CRC(cde31e35) SHA1(0219845308c9f46e73b0504bd2aefa2fa74f388e) ) /* Aka GT02 etc */

	ROM_REGION(0x80000, REGION_CPU2, 0 ) /* 68000 code (Sub cpu) */
	ROM_LOAD16_BYTE("gz_10.bin", 0x00000, 0x20000, CRC(79f919e9) SHA1(b6793173e310b1df07cf3e9209da1fbec3a8a05b) )
	ROM_LOAD16_BYTE("gz_08.bin", 0x00001, 0x20000, CRC(d47d7836) SHA1(8a5d3e8b89f5dfd6bac83f7b093ddb03d5ecef73) )
	ROM_LOAD16_BYTE("gs11",      0x40000, 0x20000, CRC(80cb23de) SHA1(d52426460eea2285c57cfc3fe37aa6dc79990e25) )
	ROM_LOAD16_BYTE("gs09",      0x40001, 0x20000, CRC(0a8fa7e1) SHA1(330ae9602b5f56b5dc4961a41991b64412a59880) )

	ROM_REGION(0x10000, REGION_CPU3, 0 ) /* Sound CPU */
	ROM_LOAD( "gs04",    0x00000, 0x10000, CRC(81c29ebf) SHA1(1b241277a8e35cdeaeb120970d14a09d33032459) )

	ROM_REGION(0x020000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD16_BYTE( "gs05", 0x000000, 0x10000, CRC(0aae996a) SHA1(d37a12b057e9934212362d7eafa575c961819a27) )
	ROM_LOAD16_BYTE( "gs06", 0x000001, 0x10000, CRC(4efdf03d) SHA1(835d22829c6d0f4efc76801b449f9a779f460f1c) )

	ROM_REGION(0x120000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "maj-02", 0x000000, 0x100000, CRC(383bbc37) SHA1(c537ab147a2770ce28ee185b08dd62d35249bfa9) )
	/* Other 0x20000 filled in later */

	ROM_REGION(0x200000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "maj-01", 0x000000, 0x100000, CRC(9840a204) SHA1(096c351769da5184c3d9a05495370134acc9507a) )
	ROM_LOAD( "maj-00", 0x100000, 0x100000, CRC(87ea8d16) SHA1(db47123aa2ebbb800cfc5cfcf50309bc39cadbcd) )

	ROM_REGION( 0x400000, REGION_GFX4, ROMREGION_DISPOSE ) /* sprites chip 1 */
	ROM_LOAD( "maj-04", 0x000000, 0x80000, CRC(36e49b19) SHA1(bfbc45b635bf3d46ff8b8a514a3f352bf3a95535) )
	ROM_LOAD( "maj-05", 0x080000, 0x80000, CRC(80fc71cc) SHA1(65b15afbe5d628051b012777d486b6ce92a3795c) )
	ROM_LOAD( "maj-06", 0x100000, 0x80000, CRC(2e7a684b) SHA1(cffeda1a816dad30d6b1cb12458661188d625d40) )
	ROM_LOAD( "maj-07", 0x180000, 0x80000, CRC(3acc1f78) SHA1(87ec65b4f54a66370754534d03f4c9217531b42f) )
	ROM_LOAD( "maj-08", 0x200000, 0x80000, CRC(1958a36d) SHA1(466a30dcd2ea13028272ed2187f890ee20d6636b) )
	ROM_LOAD( "maj-09", 0x280000, 0x80000, CRC(c21087a1) SHA1(b769c5f2f9b9c525d121902fe9557a6bfc077b99) )
	ROM_LOAD( "maj-10", 0x300000, 0x80000, CRC(a02fa641) SHA1(14b999a441964e612700bf21945a948eaebb253e) )
	ROM_LOAD( "maj-11", 0x380000, 0x80000, CRC(dabe9305) SHA1(44d69fe55e674de7f4c610d295d4528d4b2eb150) )

	ROM_REGION( 0x80000, REGION_GFX5, ROMREGION_DISPOSE ) /* sprites chip 2 */
	ROM_LOAD16_BYTE( "gs12",   0x000000, 0x20000, CRC(9a86a015) SHA1(968576b8422393ab9a93d98c15428b1c11417b3d) )
	ROM_LOAD16_BYTE( "gs13",   0x000001, 0x20000, CRC(f4709905) SHA1(697842a3d7bc2588c77833c3af8938e6f0b1238d) )
	ROM_LOAD16_BYTE( "gs14",   0x040000, 0x20000, CRC(750fc523) SHA1(ef8794359ff3a44a97ab402821fbe205a0be8f6a) )
	ROM_LOAD16_BYTE( "gs15",   0x040001, 0x20000, CRC(f14edd3d) SHA1(802d576df6dac2c9bf99f963f1955fc3a7ffdac0) )

	ROM_REGION(0x20000, REGION_SOUND1, 0 ) /* Oki samples */
	ROM_LOAD( "gs07",  0x00000,  0x20000,  CRC(750b7e5d) SHA1(d33b17a1d8c9b05d5c1daf0c80fed6381e04b167) )

	ROM_REGION(0x80000, REGION_SOUND2, 0 ) /* Extra Oki samples */
	ROM_LOAD( "maj-03", 0x00000, 0x80000,  CRC(31dcfac3) SHA1(88c7fc139f871991defbc8dc2c9c66b150dd6f6f) )	/* banked */

	ROM_REGION( 0x1000, REGION_PROMS, 0 )
	ROM_LOAD( "mb7128y.10m", 0x00000,  0x800,  CRC(bde780a2) SHA1(94ea9fe6c3a421e976d077e67f564ca5c37a5e88) )	/* Priority?  Unused */
	ROM_LOAD( "mb7128y.16p", 0x00800,  0x800,  CRC(c44d2751) SHA1(7c195650689d5cbbdccba696e0e7d3dc5bb7c506) )	/* Timing??  Unused */
	/* Above prom also at 16s and 17s */
ROM_END

/**********************************************************************************/

static READ16_HANDLER( dassault_main_skip )
{
	int ret=dassault_ram[0];

	if (activecpu_get_previouspc()==0x1170 && ret&0x8000)
		cpu_spinuntil_int();

	return ret;
}

static READ16_HANDLER( thndzone_main_skip )
{
	int ret=dassault_ram[0];

	if (activecpu_get_pc()==0x114c && ret&0x8000)
		cpu_spinuntil_int();

	return ret;
}

static void init_dassault(void)
{
	const data8_t *src = memory_region(REGION_GFX1);
	data8_t *dst = memory_region(REGION_GFX2);
	data8_t *tmp = (data8_t *)malloc(0x80000);

	/* Playfield 4 also has access to the char graphics, make things easier
	by just copying the chars to both banks (if I just used a different gfx
	bank then the colours would be wrong). */
	memcpy(tmp+0x000000,dst+0x80000,0x80000);
	memcpy(dst+0x090000,tmp+0x00000,0x80000);
	memcpy(dst+0x080000,src+0x00000,0x10000);
	memcpy(dst+0x110000,src+0x10000,0x10000);

	free(tmp);

	/* Save time waiting on vblank bit */
	install_mem_read16_handler(0, 0x3f8000, 0x3f8001, dassault_main_skip);
}

static void init_thndzone(void)
{
	const data8_t *src = memory_region(REGION_GFX1);
	data8_t *dst = memory_region(REGION_GFX2);
	data8_t *tmp = (data8_t *)malloc(0x80000);

	/* Playfield 4 also has access to the char graphics, make things easier
	by just copying the chars to both banks (if I just used a different gfx
	bank then the colours would be wrong). */
	memcpy(tmp+0x000000,dst+0x80000,0x80000);
	memcpy(dst+0x090000,tmp+0x00000,0x80000);
	memcpy(dst+0x080000,src+0x00000,0x10000);
	memcpy(dst+0x110000,src+0x10000,0x10000);

	free(tmp);

	/* Save time waiting on vblank bit */
	install_mem_read16_handler(0, 0x3f8000, 0x3f8001, thndzone_main_skip);
}

/**********************************************************************************/

GAME(1991, thndzone, 0,        dassault, thndzone, thndzone, ROT0, "Data East Corporation", "Thunder Zone (World)" )
GAME(1991, dassault, thndzone, dassault, dassault, dassault, ROT0, "Data East Corporation", "Desert Assault (US)" )
GAME(1991, dassaul4, thndzone, dassault, dassaul4, dassault, ROT0, "Data East Corporation", "Desert Assault (US 4 Players)" )
