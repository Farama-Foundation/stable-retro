/***************************************************************************

Lady Bug memory map (preliminary)

driver by Nicola Salmoria

0000-5fff ROM
6000-6fff RAM
d000-d3ff video RAM
          d000-d007/d020-d027/d040-d047/d060-d067 contain the column scroll
          registers (not used by Lady Bug)
d400-d7ff color RAM (4 bits wide)

memory mapped ports:

read:
9000      IN0
9001      IN1
9002      DSW0
9003      DSW1
e000      IN2 (not used by Lady Bug)
8000      interrupt enable? (toggle)?
see the input_ports definition below for details on the input bits

write:
7000-73ff sprites
a000      flip screen
b000      sound port 1
c000      sound port 2

interrupts:
There is no vblank interrupt. The vblank status is read from IN1.
Coin insertion in left slot generates a NMI, in right slot an IRQ.

TODO:
- Coin lockouts are missing. The game only accepts 9 coins, so there has to be
  a lockout call somewhere.

***************************************************************************/

/*
 * Space Raider todo list:
 *
 * decode cpu#2 writes to port 0x30 and 0x38 - resistors for sound
 * decode cpu#2 writes to port 0x28-0x2f - ???
 * examine other bits from cpu#2 write to 0xe800
 * one unknown dip
 */

#include "driver.h"
#include "vidhrdw/generic.h"


extern READ_HANDLER( ladybug_IN0_r );
extern READ_HANDLER( ladybug_IN1_r );
extern INTERRUPT_GEN( ladybug_interrupt );

extern WRITE_HANDLER( ladybug_videoram_w );
extern WRITE_HANDLER( ladybug_colorram_w );
extern WRITE_HANDLER( ladybug_flipscreen_w );

extern PALETTE_INIT( ladybug );
extern VIDEO_START( ladybug );
extern VIDEO_UPDATE( ladybug );

extern PALETTE_INIT( sraider );
extern VIDEO_START( sraider );
extern VIDEO_UPDATE( sraider );
extern VIDEO_EOF( sraider );
extern WRITE_HANDLER( sraider_grid_color_w );
extern WRITE_HANDLER( sraider_grid_data_w );

extern READ_HANDLER( sraider_8005_r );
extern WRITE_HANDLER( sraider_sound_low_w );
extern WRITE_HANDLER( sraider_sound_high_w );
extern READ_HANDLER( sraider_sound_low_r );
extern READ_HANDLER( sraider_sound_high_r );
extern WRITE_HANDLER( sraider_io_w );
extern WRITE_HANDLER( sraider_misc_w );

extern PALETTE_INIT( mrsdyna );
extern VIDEO_UPDATE( mrsdyna );
extern WRITE_HANDLER( mrsdyna_io_w );



READ_HANDLER( mrsdyna_rnd_r )
{
	return rand() % 4;
}

static MEMORY_READ_START( readmem )
	{ 0x0000, 0x5fff, MRA_ROM },
	{ 0x6000, 0x6fff, MRA_RAM },
	{ 0x8000, 0x8fff, MRA_NOP },
	{ 0x9000, 0x9000, input_port_0_r },	/* IN0 */
	{ 0x9001, 0x9001, input_port_1_r },	/* IN1 */
	{ 0x9002, 0x9002, input_port_3_r },	/* DSW0 */
	{ 0x9003, 0x9003, input_port_4_r },	/* DSW1 */
	{ 0xd000, 0xd7ff, MRA_RAM },	/* video and color RAM */
	{ 0xe000, 0xe000, input_port_2_r },	/* IN2 */
MEMORY_END

static MEMORY_WRITE_START( writemem )
	{ 0x0000, 0x5fff, MWA_ROM },
	{ 0x6000, 0x6fff, MWA_RAM },
	{ 0x7000, 0x73ff, MWA_RAM, &spriteram, &spriteram_size },
	{ 0xa000, 0xa000, ladybug_flipscreen_w },
	{ 0xb000, 0xbfff, SN76496_0_w },
	{ 0xc000, 0xcfff, SN76496_1_w },
	{ 0xd000, 0xd3ff, ladybug_videoram_w, &videoram },
	{ 0xd400, 0xd7ff, ladybug_colorram_w, &colorram },
MEMORY_END

/***************************************************************************

  Lady Bug doesn't have VBlank interrupts.
  Interrupts are still used by the game: but they are related to coin
  slots. Left slot generates a NMI, Right slot an IRQ.

***************************************************************************/
INTERRUPT_GEN( ladybug_interrupt )
{
	if (readinputport(5) & 1)	/* Left Coin */
		cpu_set_irq_line(0, IRQ_LINE_NMI, PULSE_LINE);
	else if (readinputport(5) & 2)	/* Right Coin */
		cpu_set_irq_line(0, 0, HOLD_LINE);
}

INPUT_PORTS_START( ladybug )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_4WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_4WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_4WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_UNUSED )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_TILT )

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	/* This should be connected to the 4V clock. I don't think the game uses it. */
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	/* Note that there are TWO VBlank inputs, one is active low, the other active */
	/* high. There are probably other differencies in the hardware, but emulating */
	/* them this way is enough to get the game running. */
	PORT_BIT( 0xc0, 0x40, IPT_VBLANK )

	PORT_START	/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_UNUSED )
	PORT_BIT( 0x0e, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL )
	PORT_BIT( 0xe0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* DSW0 */
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x03, "Easy" )
	PORT_DIPSETTING(    0x02, "Medium" )
	PORT_DIPSETTING(    0x01, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_DIPNAME( 0x04, 0x04, "High Score Names" )
	PORT_DIPSETTING(    0x00, "3 Letters" )
	PORT_DIPSETTING(    0x04, "10 Letters" )
	PORT_BITX(    0x08, 0x08, IPT_DIPSWITCH_NAME | IPF_CHEAT, "Rack Test", KEYCODE_F1, IP_JOY_NONE )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, "Freeze" )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Free_Play ) )
	PORT_DIPSETTING(    0x40, DEF_STR( No ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x80, "3" )
	PORT_DIPSETTING(    0x00, "5" )

	PORT_START	/* DSW1 */
	PORT_DIPNAME( 0x0f, 0x0f, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x0a, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x07, DEF_STR( 3C_2C ) )
	PORT_DIPSETTING(    0x0f, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x09, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x0e, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x0d, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x0b, DEF_STR( 1C_5C ) )
	/* settings 0x00 thru 0x05 all give 1 Coin/1 Credit */
	PORT_DIPNAME( 0xf0, 0xf0, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x60, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0xa0, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x70, DEF_STR( 3C_2C ) )
	PORT_DIPSETTING(    0xf0, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x90, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0xe0, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0xd0, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0xb0, DEF_STR( 1C_5C ) )
	/* settings 0x00 thru 0x50 all give 1 Coin/1 Credit */

	PORT_START	/* FAKE */
	/* The coin slots are not memory mapped. Coin Left causes a NMI, */
	/* Coin Right an IRQ. This fake input port is used by the interrupt */
	/* handler to be notified of coin insertions. We use IMPULSE to */
	/* trigger exactly one interrupt, without having to check when the */
	/* user releases the key. */
	PORT_BIT_IMPULSE( 0x01, IP_ACTIVE_HIGH, IPT_COIN1, 1 )
	PORT_BIT_IMPULSE( 0x02, IP_ACTIVE_HIGH, IPT_COIN2, 1 )
INPUT_PORTS_END

INPUT_PORTS_START( snapjack )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_UNUSED )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_TILT )

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	/* This should be connected to the 4V clock. I don't think the game uses it. */
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	/* Note that there are TWO VBlank inputs, one is active low, the other active */
	/* high. There are probably other differencies in the hardware, but emulating */
	/* them this way is enough to get the game running. */
	PORT_BIT( 0xc0, 0x40, IPT_VBLANK )

	PORT_START	/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_UNUSED  )
	PORT_BIT( 0x0e, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL )
	PORT_BIT( 0xe0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* DSW0 */
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x03, "Easy" )
	PORT_DIPSETTING(    0x02, "Medium" )
	PORT_DIPSETTING(    0x01, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_DIPNAME( 0x04, 0x04, "High Score Names" )
	PORT_DIPSETTING(    0x00, "3 Letters" )
	PORT_DIPSETTING(    0x04, "10 Letters" )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x10, 0x00, "unused1?" )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x00, "unused2?" )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0xc0, 0xc0, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "2" )
	PORT_DIPSETTING(    0xc0, "3" )
	PORT_DIPSETTING(    0x80, "4" )
	PORT_DIPSETTING(    0x40, "5" )

	PORT_START	/* DSW1 */
	/* coinage is slightly different from Lady Bug and Cosmic Avenger */
	PORT_DIPNAME( 0x0f, 0x0f, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x05, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x07, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x0a, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 3C_2C ) )
	PORT_DIPSETTING(    0x09, DEF_STR( 2C_2C ) )
	PORT_DIPSETTING(    0x0f, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x0e, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x0d, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x0b, DEF_STR( 1C_5C ) )
	/* settings 0x00 thru 0x04 all give 1 Coin/1 Credit */
	PORT_DIPNAME( 0xf0, 0xf0, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x50, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x70, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0xa0, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x60, DEF_STR( 3C_2C ) )
	PORT_DIPSETTING(    0x90, DEF_STR( 2C_2C ) )
	PORT_DIPSETTING(    0xf0, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0xe0, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0xd0, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0xb0, DEF_STR( 1C_5C ) )
	/* settings 0x00 thru 0x04 all give 1 Coin/1 Credit */

	PORT_START	/* FAKE */
	/* The coin slots are not memory mapped. Coin Left causes a NMI, */
	/* Coin Right an IRQ. This fake input port is used by the interrupt */
	/* handler to be notified of coin insertions. We use IMPULSE to */
	/* trigger exactly one interrupt, without having to check when the */
	/* user releases the key. */
	PORT_BIT_IMPULSE( 0x01, IP_ACTIVE_HIGH, IPT_COIN1, 1 )
	PORT_BIT_IMPULSE( 0x02, IP_ACTIVE_HIGH, IPT_COIN2, 1 )
INPUT_PORTS_END

INPUT_PORTS_START( cavenger )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_TILT )

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	/* This should be connected to the 4V clock. I don't think the game uses it. */
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	/* Note that there are TWO VBlank inputs, one is active low, the other active */
	/* high. There are probably other differencies in the hardware, but emulating */
	/* them this way is enough to get the game running. */
	PORT_BIT( 0xc0, 0x40, IPT_VBLANK )

	PORT_START	/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x0e, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL )
	PORT_BIT( 0xe0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* DSW0 */
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x03, "Easy" )
	PORT_DIPSETTING(    0x02, "Medium" )
	PORT_DIPSETTING(    0x01, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_DIPNAME( 0x04, 0x04, "High Score Names" )
	PORT_DIPSETTING(    0x00, "3 Letters" )
	PORT_DIPSETTING(    0x04, "10 Letters" )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x30, 0x00, "Initial High Score" )
	PORT_DIPSETTING(    0x00, "0" )
	PORT_DIPSETTING(    0x30, "5000" )
	PORT_DIPSETTING(    0x20, "8000" )
	PORT_DIPSETTING(    0x10, "10000" )
	PORT_DIPNAME( 0xc0, 0xc0, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "2" )
	PORT_DIPSETTING(    0xc0, "3" )
	PORT_DIPSETTING(    0x80, "4" )
	PORT_DIPSETTING(    0x40, "5" )

	PORT_START	/* DSW1 */
	PORT_DIPNAME( 0x0f, 0x0f, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x0a, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x07, DEF_STR( 3C_2C ) )
	PORT_DIPSETTING(    0x0f, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x09, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x0e, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x0d, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x0b, DEF_STR( 1C_5C ) )
	/* settings 0x00 thru 0x05 all give 1 Coin/1 Credit */
	PORT_DIPNAME( 0xf0, 0xf0, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x60, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0xa0, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x70, DEF_STR( 3C_2C ) )
	PORT_DIPSETTING(    0xf0, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x90, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0xe0, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0xd0, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0xb0, DEF_STR( 1C_5C ) )
	/* settings 0x00 thru 0x50 all give 1 Coin/1 Credit */

	PORT_START	/* FAKE */
	/* The coin slots are not memory mapped. Coin Left causes a NMI, */
	/* Coin Right an IRQ. This fake input port is used by the interrupt */
	/* handler to be notified of coin insertions. We use IMPULSE to */
	/* trigger exactly one interrupt, without having to check when the */
	/* user releases the key. */
	PORT_BIT_IMPULSE( 0x01, IP_ACTIVE_HIGH, IPT_COIN1, 1 )
	PORT_BIT_IMPULSE( 0x02, IP_ACTIVE_HIGH, IPT_COIN2, 1 )
INPUT_PORTS_END

INPUT_PORTS_START( dorodon )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_4WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_4WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_4WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_UNUSED )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_TILT )

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	/* This should be connected to the 4V clock. I don't think the game uses it. */
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	/* Note that there are TWO VBlank inputs, one is active low, the other active */
	/* high. There are probably other differencies in the hardware, but emulating */
	/* them this way is enough to get the game running. */
	PORT_BIT( 0xc0, 0x40, IPT_VBLANK )

	PORT_START	/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_UNUSED )
	PORT_BIT( 0x0e, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL )
	PORT_BIT( 0xe0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* DSW0 */
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x03, "Easy" )
	PORT_DIPSETTING(    0x02, "Medium" )
	PORT_DIPSETTING(    0x01, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x04, "20000" )
	PORT_DIPSETTING(    0x00, "30000" )
	PORT_BITX(    0x08, 0x08, IPT_DIPSWITCH_NAME | IPF_CHEAT, "Rack Test", KEYCODE_F1, IP_JOY_NONE )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, "Freeze" )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Free_Play ) )
	PORT_DIPSETTING(    0x40, DEF_STR( No ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x80, "3" )
	PORT_DIPSETTING(    0x00, "5" )

	PORT_START	/* DSW1 */
	PORT_DIPNAME( 0x0f, 0x0f, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x0a, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x07, DEF_STR( 3C_2C ) )
	PORT_DIPSETTING(    0x0f, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x09, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x0e, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x0d, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x0b, DEF_STR( 1C_5C ) )
	/* settings 0x00 thru 0x05 all give 1 Coin/1 Credit */
	PORT_DIPNAME( 0xf0, 0xf0, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x60, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0xa0, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x70, DEF_STR( 3C_2C ) )
	PORT_DIPSETTING(    0xf0, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x90, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0xe0, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0xd0, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0xb0, DEF_STR( 1C_5C ) )
	/* settings 0x00 thru 0x50 all give 1 Coin/1 Credit */

	PORT_START	/* FAKE */
	/* The coin slots are not memory mapped. Coin Left causes a NMI, */
	/* Coin Right an IRQ. This fake input port is used by the interrupt */
	/* handler to be notified of coin insertions. We use IMPULSE to */
	/* trigger exactly one interrupt, without having to check when the */
	/* user releases the key. */
	PORT_BIT_IMPULSE( 0x01, IP_ACTIVE_HIGH, IPT_COIN1, 1 )
	PORT_BIT_IMPULSE( 0x02, IP_ACTIVE_HIGH, IPT_COIN2, 1 )
INPUT_PORTS_END

INPUT_PORTS_START( sraider )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN2 )

	PORT_START	/* DSW0 */
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x03, "Easy" )
	PORT_DIPSETTING(    0x02, "Medium" )
	PORT_DIPSETTING(    0x01, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_DIPNAME( 0x04, 0x04, "High Score Names" )
	PORT_DIPSETTING(    0x00, "3 Letters" )
	PORT_DIPSETTING(    0x04, "10 Letters" )
	PORT_DIPNAME( 0x08, 0x08, "Allow_Continue" )
	PORT_DIPSETTING(    0x08, DEF_STR( No ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0xc0, 0xc0, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "2" )
	PORT_DIPSETTING(    0xc0, "3" )
	PORT_DIPSETTING(    0x80, "4" )
	PORT_DIPSETTING(    0x40, "5" )

	/* Free Play setting works when it's set for both */
	PORT_START	/* DSW1 */
	PORT_DIPNAME( 0x0f, 0x0f, DEF_STR( Coin_A ) )
	/* settings 0x00 thru 0x05 all give 1 Coin/1 Credit */
	PORT_DIPSETTING(    0x06, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x0a, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x07, DEF_STR( 3C_2C ) )
	PORT_DIPSETTING(    0x09, DEF_STR( 2C_2C ) )
	PORT_DIPSETTING(    0x0f, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x0e, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x0d, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x0b, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0xf0, 0xf0, DEF_STR( Coin_B ) )
	/* settings 0x00 thru 0x50 all give 1 Coin/1 Credit */
	PORT_DIPSETTING(    0x60, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0xa0, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x70, DEF_STR( 3C_2C ) )
	PORT_DIPSETTING(    0x90, DEF_STR( 2C_2C ) )
	PORT_DIPSETTING(    0xf0, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0xe0, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0xd0, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0xb0, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Free_Play ) )
INPUT_PORTS_END

INPUT_PORTS_START( mrsdyna )
	PORT_START   /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_4WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_4WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_4WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN ) /* JAMMA PIN 12 */

	PORT_START   /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN ) /* VBLANK???? */
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN2 )

	PORT_START  /* DSW0 @ R3 via '244 @ R2 */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, "High Score Names" )
	PORT_DIPSETTING(    0x00, "3 Letters" )
	PORT_DIPSETTING(    0x04, "12 Letters" )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0xc0, 0xc0, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "2" )
	PORT_DIPSETTING(    0xc0, "3" )
	PORT_DIPSETTING(    0x80, "4" )
	PORT_DIPSETTING(    0x40, "5" )

	/* Free Play setting works when it's set for both */
	PORT_START  /* DSW1 @ P3 via '244 @ P2 */
	PORT_DIPNAME( 0x0f, 0x0f, DEF_STR( Coin_A ) )
	/* settings 0x00 through 0x05 all give 1 Coin/1 Credit */
	PORT_DIPSETTING(    0x06, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x0a, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x07, DEF_STR( 3C_2C ) )
	PORT_DIPSETTING(    0x09, DEF_STR( 2C_2C ) )
	PORT_DIPSETTING(    0x0f, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x0e, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x0d, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x0b, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0xf0, 0xf0, DEF_STR( Coin_B ) )
	/* settings 0x00 through 0x50 all give 1 Coin/1 Credit */
	PORT_DIPSETTING(    0x60, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0xa0, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x70, DEF_STR( 3C_2C ) )
	PORT_DIPSETTING(    0x90, DEF_STR( 2C_2C ) )
	PORT_DIPSETTING(    0xf0, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0xe0, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0xd0, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0xb0, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Free_Play ) )
INPUT_PORTS_END

static struct GfxLayout charlayout =
{
	8,8,	/* 8*8 characters */
	512,	/* 512 characters */
	2,	/* 2 bits per pixel */
	{ 0, 512*8*8 },	/* the two bitplanes are separated */
	{ 7, 6, 5, 4, 3, 2, 1, 0 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8	/* every char takes 8 consecutive bytes */
};
static struct GfxLayout spritelayout =
{
	16,16,	/* 16*16 sprites */
	128,	/* 128 sprites */
	2,	/* 2 bits per pixel */
	{ 1, 0 },	/* the two bitplanes are packed in two consecutive bits */
	{ 0, 2, 4, 6, 8, 10, 12, 14,
			8*16+0, 8*16+2, 8*16+4, 8*16+6, 8*16+8, 8*16+10, 8*16+12, 8*16+14 },
	{ 23*16, 22*16, 21*16, 20*16, 19*16, 18*16, 17*16, 16*16,
			7*16, 6*16, 5*16, 4*16, 3*16, 2*16, 1*16, 0*16 },
	64*8	/* every sprite takes 64 consecutive bytes */
};
static struct GfxLayout spritelayout2 =
{
	8,8,	/* 8*8 sprites */
	512,	/* 512 sprites */
	2,	/* 2 bits per pixel */
	{ 1, 0 },	/* the two bitplanes are packed in two consecutive bits */
	{ 0, 2, 4, 6, 8, 10, 12, 14 },
	{ 7*16, 6*16, 5*16, 4*16, 3*16, 2*16, 1*16, 0*16 },
	16*8	/* every sprite takes 16 consecutive bytes */
};

static struct GfxLayout gridlayout =
{
	8,8,	/* 8*8 characters */
	512,	/* 512 characters */
	1,	/* 1 bit per pixel */
	{ 0 },
	{ 7, 6, 5, 4, 3, 2, 1, 0 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8},
	8*8	/* every char takes 8 consecutive bytes */
};

static struct GfxLayout gridlayout2 =
{
	8,8,	/* 8*8 characters */
	512,	/* 512 characters */
	1,	/* 2 bits per pixel */
	{ 0 },
	{ 7, 6, 5, 4, 3, 2, 1, 0 },
	{ 7*8, 6*8, 5*8, 4*8, 3*8, 2*8, 1*8, 0*8 },
	8*8	/* every char takes 8 consecutive bytes */
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &charlayout,      0,  8 },
	{ REGION_GFX2, 0, &spritelayout,  4*8, 16 },
	{ REGION_GFX2, 0, &spritelayout2, 4*8, 16 },
	{ -1 } /* end of array */
};

static struct GfxDecodeInfo sraider_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &charlayout,      0,  8 },
	{ REGION_GFX2, 0, &spritelayout,  4*8, 16 },
	{ REGION_GFX2, 0, &spritelayout2, 4*8, 16 },
	{ REGION_USER1, 0, &gridlayout, 32+64, 1 },
	{ REGION_USER1, 0, &gridlayout2, 32+64, 1 },
	{ -1 } /* end of array */
};

static struct SN76496interface sn76496_interface =
{
	2,	/* 2 chips */
	{ 4000000, 4000000 },	/* 4 MHz */
	{ 100, 100 }
};

static struct SN76496interface sraider_sn76496_interface =
{
	5,	/* 5 chips */
	{ 4000000, 4000000, 4000000, 4000000, 4000000 },	/* 4 MHz */
	{ 100, 100, 100, 100, 100 }
};


static MACHINE_DRIVER_START( ladybug )

	/* basic machine hardware */
	MDRV_CPU_ADD(Z80, 4000000)	/* 4 MHz */
	MDRV_CPU_MEMORY(readmem,writemem)
	MDRV_CPU_VBLANK_INT(ladybug_interrupt,1)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(1*8, 31*8-1, 4*8, 28*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(32)
	MDRV_COLORTABLE_LENGTH(4*24)

	MDRV_PALETTE_INIT(ladybug)
	MDRV_VIDEO_START(ladybug)
	MDRV_VIDEO_UPDATE(ladybug)

	/* sound hardware */
	MDRV_SOUND_ADD(SN76496, sn76496_interface)
MACHINE_DRIVER_END



static MEMORY_READ_START( sraider_cpu1_readmem )
    { 0x0000, 0x5fff, MRA_ROM },
	{ 0x6000, 0x6fff, MRA_RAM },
	{ 0x8005, 0x8005, sraider_8005_r },  /* protection check? */
	{ 0x9000, 0x9000, input_port_0_r },	/* IN0 */
	{ 0x9001, 0x9001, input_port_1_r },	/* IN1 */
	{ 0x9002, 0x9002, input_port_2_r },	/* DSW0 */
	{ 0x9003, 0x9003, input_port_3_r },	/* DSW1 */
MEMORY_END

static MEMORY_WRITE_START( sraider_cpu1_writemem )
    { 0x0000, 0x5fff, MWA_ROM },
	{ 0x6000, 0x6fff, MWA_RAM },
	{ 0x7000, 0x73ff, MWA_RAM, &spriteram, &spriteram_size },
	{ 0x8006, 0x8006, sraider_sound_low_w },
	{ 0x8007, 0x8007, sraider_sound_high_w },
	{ 0xd000, 0xd3ff, ladybug_videoram_w, &videoram },
	{ 0xd400, 0xd7ff, ladybug_colorram_w, &colorram },
	{ 0xe000, 0xe000, MWA_NOP },  /* unknown */
	/* 0x10 when in attract, 0x20 when coined/playing */
MEMORY_END

static MEMORY_READ_START( sraider_cpu2_readmem )
    { 0x0000, 0x5fff, MRA_ROM },
	{ 0x6000, 0x63ff, MRA_RAM },
	{ 0x8000, 0x8000, sraider_sound_low_r },
	{ 0xa000, 0xa000, sraider_sound_high_r },
    { 0xc000, 0xc000, mrsdyna_rnd_r }, /* some kind of sync */
MEMORY_END

static MEMORY_WRITE_START( sraider_cpu2_writemem )
    { 0x0000, 0x5fff, MWA_ROM },
	{ 0x6000, 0x63ff, MWA_RAM },
	{ 0xe000, 0xe0ff, sraider_grid_data_w },
	{ 0xe800, 0xe800, sraider_io_w },
MEMORY_END

static MEMORY_WRITE_START( mrsdyna_cpu2_writemem )
    { 0x0000, 0x5fff, MWA_ROM },
	{ 0x6000, 0x63ff, MWA_RAM },
	{ 0xe000, 0xe0ff, sraider_grid_data_w },
	{ 0xe800, 0xe800, mrsdyna_io_w },
MEMORY_END

static PORT_WRITE_START( sraider_cpu2_writeport )
PORT_ADDRESS_BITS(8)
	{ 0x00, 0x00, SN76496_0_w },
	{ 0x08, 0x08, SN76496_1_w },
	{ 0x10, 0x10, SN76496_2_w },
	{ 0x18, 0x18, SN76496_3_w },
	{ 0x20, 0x20, SN76496_4_w },
	{ 0x28, 0x3f, sraider_misc_w },  /* lots unknown */
PORT_END


static MACHINE_DRIVER_START( sraider )

	/* basic machine hardware */
	MDRV_CPU_ADD(Z80, 4000000)	/* 4 MHz */
	MDRV_CPU_MEMORY(sraider_cpu1_readmem,sraider_cpu1_writemem)
	MDRV_CPU_VBLANK_INT(irq0_line_pulse,1)

	MDRV_CPU_ADD(Z80, 4000000)	/* 4 MHz */
	MDRV_CPU_MEMORY(sraider_cpu2_readmem,sraider_cpu2_writemem)
	MDRV_CPU_PORTS(0,sraider_cpu2_writeport)
	MDRV_CPU_VBLANK_INT(irq0_line_pulse,1)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(1*8, 31*8-1, 4*8, 28*8-1)
	MDRV_GFXDECODE(sraider_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(65)
	MDRV_COLORTABLE_LENGTH(32+64+2)

	MDRV_PALETTE_INIT(sraider)
	MDRV_VIDEO_START(sraider)
	MDRV_VIDEO_UPDATE(sraider)
	MDRV_VIDEO_EOF(sraider)

	/* sound hardware */
	MDRV_SOUND_ADD(SN76496, sraider_sn76496_interface)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( mrsdyna )

	/* basic machine hardware */
	MDRV_CPU_ADD(Z80, 4000000)	/* 4 MHz */
	MDRV_CPU_MEMORY(sraider_cpu1_readmem,sraider_cpu1_writemem)
	MDRV_CPU_VBLANK_INT(irq0_line_pulse,1)

	MDRV_CPU_ADD(Z80, 4000000)	/* 4 MHz */
	MDRV_CPU_MEMORY(sraider_cpu2_readmem,mrsdyna_cpu2_writemem)
	MDRV_CPU_PORTS(0,sraider_cpu2_writeport)
	MDRV_CPU_VBLANK_INT(irq0_line_pulse,1)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(1*8, 31*8-1, 4*8, 28*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(65)
	MDRV_COLORTABLE_LENGTH(32+64+2)

	MDRV_PALETTE_INIT(mrsdyna)
	MDRV_VIDEO_START(sraider)
	MDRV_VIDEO_UPDATE(mrsdyna)

	/* sound hardware */
	MDRV_SOUND_ADD(SN76496, sraider_sn76496_interface)
MACHINE_DRIVER_END

/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( ladybug )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "lb1.cpu",      0x0000, 0x1000, CRC(d09e0adb) SHA1(ddc1f849cbcefb64b70a26c2a4c993f0516af814) )
	ROM_LOAD( "lb2.cpu",      0x1000, 0x1000, CRC(88bc4a0a) SHA1(193c9f90b7550020c0923cb158dff7d5faa53bc6) )
	ROM_LOAD( "lb3.cpu",      0x2000, 0x1000, CRC(53e9efce) SHA1(1960e9cd896b6a65197aefc3f10348103552b598) )
	ROM_LOAD( "lb4.cpu",      0x3000, 0x1000, CRC(ffc424d7) SHA1(2a4b9533e61e265bdd38c126add8c26d5bc048d5) )
	ROM_LOAD( "lb5.cpu",      0x4000, 0x1000, CRC(ad6af809) SHA1(276275d56c725b9d90eeb44c317ceb06bac27ae7) )
	ROM_LOAD( "lb6.cpu",      0x5000, 0x1000, CRC(cf1acca4) SHA1(c05de7de4bd05d5c2af6aa752e057a9286f3effc) )

	ROM_REGION( 0x2000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "lb9.vid",      0x0000, 0x1000, CRC(77b1da1e) SHA1(58cb82417396a3d96acfc864f091b1a5988f228d) )
	ROM_LOAD( "lb10.vid",     0x1000, 0x1000, CRC(aa82e00b) SHA1(83a5b745e58844b6dd7d05dfe9dbb5959aaf5c40) )

	ROM_REGION( 0x2000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "lb8.cpu",      0x0000, 0x1000, CRC(8b99910b) SHA1(0bc812cf872f04eacedb50feed53f1aa8a1f24b9) )
	ROM_LOAD( "lb7.cpu",      0x1000, 0x1000, CRC(86a5b448) SHA1(f8585a6fcf921e3e21f112dd2de474cb53cef290) )

	ROM_REGION( 0x0060, REGION_PROMS, 0 )
	ROM_LOAD( "10-2.vid",     0x0000, 0x0020, CRC(df091e52) SHA1(4d7fea6d9ab31e5f280b1dc198a325f00c3826ef) ) /* palette */
	ROM_LOAD( "10-1.vid",     0x0020, 0x0020, CRC(40640d8f) SHA1(85d13a9b78c47174cff7c869f52b30263bae575e) ) /* sprite color lookup table */
	ROM_LOAD( "10-3.vid",     0x0040, 0x0020, CRC(27fa3a50) SHA1(7cf59b7a37c156640d6ea91554d1c4276c1780e0) ) /* ?? */
ROM_END

ROM_START( ladybugb )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "lb1a.cpu",     0x0000, 0x1000, CRC(ec135e54) SHA1(69fc6db04b28c25eda329fc88c235267ca93a09f) )
	ROM_LOAD( "lb2a.cpu",     0x1000, 0x1000, CRC(3049c5c6) SHA1(51ceb70fa4789ff91c9bb1e157be5b6c09ff3c8e) )
	ROM_LOAD( "lb3a.cpu",     0x2000, 0x1000, CRC(b0fef837) SHA1(37e9d8d157c3af12cd97534a42dd21f621ac501b) )
	ROM_LOAD( "lb4.cpu",      0x3000, 0x1000, CRC(ffc424d7) SHA1(2a4b9533e61e265bdd38c126add8c26d5bc048d5) )
	ROM_LOAD( "lb5.cpu",      0x4000, 0x1000, CRC(ad6af809) SHA1(276275d56c725b9d90eeb44c317ceb06bac27ae7) )
	ROM_LOAD( "lb6a.cpu",     0x5000, 0x1000, CRC(88c8002a) SHA1(ffff1b8d4c1521710c988eee12081d28ed491ccf) )

	ROM_REGION( 0x2000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "lb9.vid",      0x0000, 0x1000, CRC(77b1da1e) SHA1(58cb82417396a3d96acfc864f091b1a5988f228d) )
	ROM_LOAD( "lb10.vid",     0x1000, 0x1000, CRC(aa82e00b) SHA1(83a5b745e58844b6dd7d05dfe9dbb5959aaf5c40) )

	ROM_REGION( 0x2000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "lb8.cpu",      0x0000, 0x1000, CRC(8b99910b) SHA1(0bc812cf872f04eacedb50feed53f1aa8a1f24b9) )
	ROM_LOAD( "lb7.cpu",      0x1000, 0x1000, CRC(86a5b448) SHA1(f8585a6fcf921e3e21f112dd2de474cb53cef290) )

	ROM_REGION( 0x0060, REGION_PROMS, 0 )
	ROM_LOAD( "10-2.vid",     0x0000, 0x0020, CRC(df091e52) SHA1(4d7fea6d9ab31e5f280b1dc198a325f00c3826ef) ) /* palette */
	ROM_LOAD( "10-1.vid",     0x0020, 0x0020, CRC(40640d8f) SHA1(85d13a9b78c47174cff7c869f52b30263bae575e) ) /* sprite color lookup table */
	ROM_LOAD( "10-3.vid",     0x0040, 0x0020, CRC(27fa3a50) SHA1(7cf59b7a37c156640d6ea91554d1c4276c1780e0) ) /* ?? */
ROM_END

ROM_START( snapjack )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "sj2a.bin",     0x0000, 0x1000, CRC(6b30fcda) SHA1(85e4ebbbe8e8d6c79a14387d7a6818abc9430037) )
	ROM_LOAD( "sj2b.bin",     0x1000, 0x1000, CRC(1f1088d1) SHA1(0fd5204ea27e9bdd811e9ea21e9bbab84b916f4a) )
	ROM_LOAD( "sj2c.bin",     0x2000, 0x1000, CRC(edd65f3a) SHA1(763d588f0755a22c0f24269e6f38979fd516693f) )
	ROM_LOAD( "sj2d.bin",     0x3000, 0x1000, CRC(f4481192) SHA1(514bb124a1d75a622e2ca4c2175d819092d4638d) )
	ROM_LOAD( "sj2e.bin",     0x4000, 0x1000, CRC(1bff7d05) SHA1(47246095313ebba30f42d715a9fb5fc1abb68ea6) )
	ROM_LOAD( "sj2f.bin",     0x5000, 0x1000, CRC(21793edf) SHA1(11e259161bab3a32a8b52f7baa4fec17be6d4302) )

	ROM_REGION( 0x2000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "sj2i.bin",     0x0000, 0x1000, CRC(ff2011c7) SHA1(38409e2318dee3cc0678d4ee9e93d9b895883df6) )
	ROM_LOAD( "sj2j.bin",     0x1000, 0x1000, CRC(f097babb) SHA1(461662719bc7f1cf21c41759f4832a92b0fdb4f2) )

	ROM_REGION( 0x2000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "sj2h.bin",     0x0000, 0x1000, CRC(b7f105b6) SHA1(1135c3188b41cb0ccb24079c613188209b624683) )
	ROM_LOAD( "sj2g.bin",     0x1000, 0x1000, CRC(1cdb03a8) SHA1(5f390a672f3adf6392f8060bf7f0bcabc2eba139) )

	ROM_REGION( 0x0060, REGION_PROMS, 0 )
	ROM_LOAD( "sj8t.bin",     0x0000, 0x0020, CRC(cbbd9dd1) SHA1(e267726ba59e9a42ac89dd22eb1508ad21fd32ac) ) /* palette */
	ROM_LOAD( "sj9k.bin",     0x0020, 0x0020, CRC(5b16fbd2) SHA1(0a776aeca3947a6f29d527018f5182e758b50c5d) ) /* sprite color lookup table */
	ROM_LOAD( "sj9h.bin",     0x0040, 0x0020, CRC(27fa3a50) SHA1(7cf59b7a37c156640d6ea91554d1c4276c1780e0) ) /* ?? */
ROM_END

ROM_START( cavenger )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "1",            0x0000, 0x1000, CRC(9e0cc781) SHA1(f23bd6b9f427c26ac996a5c8ba29f356cf45c78a) )
	ROM_LOAD( "2",            0x1000, 0x1000, CRC(5ce5b950) SHA1(170e3f8be592dcccb8868474f40f8f2223e8a8b5) )
	ROM_LOAD( "3",            0x2000, 0x1000, CRC(bc28218d) SHA1(4b0f1b38a5837b7ffc9aec6c28c6eb72cfa46226) )
	ROM_LOAD( "4",            0x3000, 0x1000, CRC(2b32e9f5) SHA1(f8a7ea799d8ff9b4f830d064bb2f34a76729c336) )
	ROM_LOAD( "5",            0x4000, 0x1000, CRC(d117153e) SHA1(622c90a6c3f0adc24fe8a1d4969075cbd55add4e) )
	ROM_LOAD( "6",            0x5000, 0x1000, CRC(c7d366cb) SHA1(ec4981fe34abf992acbd6325b2c756c58ff80b04) )

	ROM_REGION( 0x2000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "9",            0x0000, 0x1000, CRC(63357785) SHA1(20eaa866b7700535312fd415edaea94408ff3e3d) )
	ROM_LOAD( "0",            0x1000, 0x1000, CRC(52ad1133) SHA1(bc8c52c6ba919287773ff6a4ec793ebd95176130) )

	ROM_REGION( 0x2000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "8",            0x0000, 0x1000, CRC(b022bf2d) SHA1(85f78d5a1e5782587bb66ad101a94fd0d62fb790) )
	/* 1000-1fff empty */

	ROM_REGION( 0x0060, REGION_PROMS, 0 )
	ROM_LOAD( "t8.bpr",       0x0000, 0x0020, CRC(42a24dd5) SHA1(03175ee7f8e11896a89d7cc0d614a78a49923627) ) /* palette */
	ROM_LOAD( "k9.bpr",       0x0020, 0x0020, CRC(d736b8de) SHA1(4c9c76826f3a2a631d01fd2531d55318172b0c12) ) /* sprite color lookup table */
	ROM_LOAD( "h9.bpr",       0x0040, 0x0020, CRC(27fa3a50) SHA1(7cf59b7a37c156640d6ea91554d1c4276c1780e0) ) /* ?? */
ROM_END

ROM_START( dorodon )
	ROM_REGION( 0x20000, REGION_CPU1, 0 ) /* 64K for data, 64K for encrypted opcodes */
	ROM_LOAD( "dorodon.0",   0x0000, 0x2000, CRC(460aaf26) SHA1(c4ea41cba4ac2d93fedec3c117a4470fee2a910f) )
	ROM_LOAD( "dorodon.1",   0x2000, 0x2000, CRC(d2451eb6) SHA1(4154bfe50b7f75444d3f0c9be6bd2475fdba1938) )
	ROM_LOAD( "dorodon.2",   0x4000, 0x2000, CRC(d3c6ee6c) SHA1(6971ecdc968810c19f8601efc3d389450156bb22) )

	/* Characters */
	ROM_REGION( 0x2000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "dorodon.5",   0x0000, 0x1000, CRC(5eee2b85) SHA1(55ac9566e805d103b6916f51c764e2601cc1f715) )
	ROM_LOAD( "dorodon.6",   0x1000, 0x1000, CRC(395ac25a) SHA1(d8a55e42b8c5d957c2e6a3181d7ac10c6a448f46) )

	/* Sprites */
	ROM_REGION( 0x2000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "dorodon.4",   0x0000, 0x1000, CRC(d70bb50a) SHA1(b9d46862f288c49bb8b660da87b63bd4ecb36379) )
	ROM_LOAD( "dorodon.3",   0x1000, 0x1000, CRC(e44e59e6) SHA1(ff730152804d75ddb9fb19e8ec33cc764d8a50e8) )

	/* Opcode Decryption PROMS */
	ROM_REGION( 0x0100, REGION_USER1, ROMREGION_DISPOSE )
	ROM_LOAD_NIB_HIGH( "dorodon.bp4",0x0000,0x0100,CRC(f865c135) SHA1(1202f83bfa50afa5a5d24401efa8bf058e7e30b5) )
	ROM_LOAD_NIB_LOW(  "dorodon.bp3",0x0000,0x0100,CRC(47b2f0bb) SHA1(640720aa5c1119080c6da928f6d1b0e76b989742) )

	ROM_REGION( 0x0060, REGION_PROMS, 0 )
	ROM_LOAD( "dorodon.bp0", 0x0000, 0x0020, CRC(8fcf0bc8) SHA1(392d22731b3e4bc663d6e4385f6069ee2b4ee029) ) /* palette */
	ROM_LOAD( "dorodon.bp1", 0x0020, 0x0020, CRC(3f209be4) SHA1(f924494eed357a15ffc11331c163af24585d4ab9) ) /* sprite color lookup table */
	ROM_LOAD( "dorodon.bp2", 0x0040, 0x0020, CRC(27fa3a50) SHA1(7cf59b7a37c156640d6ea91554d1c4276c1780e0) ) /* timing?? */
ROM_END

ROM_START( dorodon2 )
	ROM_REGION( 0x20000, REGION_CPU1, 0 ) /* 64K for data, 64K for encrypted opcodes */
	ROM_LOAD( "1.3fg",        0x0000, 0x2000, CRC(4d05d6f8) SHA1(db12ad04295f0ce112b6e90fde94a53ed1d6c3b9) )
	ROM_LOAD( "2.3h",         0x2000, 0x2000, CRC(27b43b09) SHA1(12a8a6b8665bb9d1967ec631a794aab564a50570) )
	ROM_LOAD( "3.3k",         0x4000, 0x2000, CRC(38d2f295) SHA1(b4d2cfd6e9f03c3ef18dcf67326f4106749b62b1) )

	/* Characters */
	ROM_REGION( 0x2000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "6.6a",        0x0000, 0x1000, CRC(2a2d8b9c) SHA1(ba3ce8ed6cafa711bf4c6ed260dd15b38adbd6cc) )
	ROM_LOAD( "7.6bc",       0x1000, 0x1000, CRC(d14f95fa) SHA1(e9ba87602d779d833b8152c077c692e67ef696cc) )

	/* Sprites */
	ROM_REGION( 0x2000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "5.3t",        0x0000, 0x1000, CRC(54c04f58) SHA1(342ef914e6f8bf37472d146bb5e9fb67056d7fc5) )
	ROM_LOAD( "4.3r",        0x1000, 0x1000, CRC(1ebb6493) SHA1(30367d7594118e0fa8620e5d20c66a650ca82c86) )

	/* Opcode Decryption PROMS (from other romset) */
	ROM_REGION( 0x0100, REGION_USER1, ROMREGION_DISPOSE )
	ROM_LOAD_NIB_HIGH( "dorodon.bp4",0x0000,0x0100,CRC(f865c135) SHA1(1202f83bfa50afa5a5d24401efa8bf058e7e30b5) )
	ROM_LOAD_NIB_LOW(  "dorodon.bp3",0x0000,0x0100,CRC(47b2f0bb) SHA1(640720aa5c1119080c6da928f6d1b0e76b989742) )

	/* (from other romset - I think these are correct, they match the Starcade video) */
	ROM_REGION( 0x0060, REGION_PROMS, 0 )
	ROM_LOAD( "dorodon.bp0", 0x0000, 0x0020, CRC(8fcf0bc8) SHA1(392d22731b3e4bc663d6e4385f6069ee2b4ee029) ) /* palette */
	ROM_LOAD( "dorodon.bp1", 0x0020, 0x0020, CRC(3f209be4) SHA1(f924494eed357a15ffc11331c163af24585d4ab9) ) /* sprite color lookup table */
	ROM_LOAD( "dorodon.bp2", 0x0040, 0x0020, CRC(27fa3a50) SHA1(7cf59b7a37c156640d6ea91554d1c4276c1780e0) ) /* timing?? */
ROM_END

ROM_START( sraider )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "sraid3.r4",    0x0000, 0x2000, CRC(0f389774) SHA1(c67596e6bf00175ff0a241506cd2f88114d05933) )
	ROM_LOAD( "sraid2.n4",    0x2000, 0x2000, CRC(38a48db0) SHA1(6f4f384d702fb8ee4bb2ef579638239d57e32ddd) )
	ROM_LOAD( "sraid1.m4",    0x4000, 0x2000, CRC(2f302a4e) SHA1(3a902ce6858f38df88b60830bef4b1d45b09b2df) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for code */
	ROM_LOAD( "sraid-s4.h6",  0x0000, 0x2000, CRC(57173a12) SHA1(6cb8fd4826e499f9a4e63621d58bc4b596cc261e) )
	ROM_LOAD( "sraid-s5.j6",  0x2000, 0x2000, CRC(5a459179) SHA1(a261c8f3c7c4cd4587c003bbbe815d2c4e01ffbc) )
	ROM_LOAD( "sraid-s6.l6",  0x4000, 0x2000, CRC(ea3aa25d) SHA1(353c0d075d5e0a3bc25a65e2748f5eb5212a844d) )

	ROM_REGION( 0x2000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "sraid-s0.k6",  0x0000, 0x1000, CRC(a0373909) SHA1(00e3bd5dd90769d670fc3c51edd1cd4b69e6132d) )
	ROM_LOAD( "sraids11.l6",  0x1000, 0x1000, CRC(ba22d949) SHA1(83762ced1df92ff594887e44d5b783826bbfb0c9) )

	ROM_REGION( 0x2000, REGION_GFX2, ROMREGION_DISPOSE )
    ROM_LOAD( "sraid-s7.m2",  0x0000, 0x1000, CRC(299f8e07) SHA1(1de71f251286088487da7285d6f8070147002af5) )
	ROM_LOAD( "sraid-s8.n2",  0x1000, 0x1000, CRC(57ba8888) SHA1(2aa1a5f682d146a55a96e471bb78e5c60da02bf9) )

	ROM_REGION( 0x0060, REGION_PROMS, 0 )
	ROM_LOAD( "srpr10-1.a2",  0x0000, 0x0020, CRC(121fdb99) SHA1(3bc092da40beb129a4df3db2f55d22bbbcf7bad8) )
	ROM_LOAD( "srpr10-2.l3",  0x0020, 0x0020, CRC(88b67e70) SHA1(e21ee2939e96dffee101bd92c62ed975b6b64001) )
	ROM_LOAD( "srpr10-3.c1",  0x0040, 0x0020, CRC(27fa3a50) SHA1(7cf59b7a37c156640d6ea91554d1c4276c1780e0) )

	/* fixed portion of the grid */
	ROM_REGION( 0x1000, REGION_USER1, 0 )
	ROM_LOAD( "sraid-s9.f6",  0x0000, 0x1000, CRC(2380b90f) SHA1(0310554e3f2ec973c2bb6e816d04e5c0c1e0a0b9) )
ROM_END

ROM_START( mrsdyna )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "mrsd-8203a-r4.f3", 0x0000, 0x2000, CRC(c944062c) SHA1(c61fc327d67595e601f6a7e5e337646f5f9d351b) )
	ROM_LOAD( "mrsd-8203a-n4.f2", 0x2000, 0x2000, CRC(d1b9c7bb) SHA1(c139c8ae5b14924eb04a265095a7ab95ac5370af) )
	ROM_LOAD( "mrsd-8203a-m4.f1", 0x4000, 0x2000, CRC(d25b1dfe) SHA1(f68c6fb2cda37fcffbe7c3c2a3cc5cb372c4101b) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )
	ROM_LOAD( "mrsd-8203a-h6.f4", 0x0000, 0x2000, CRC(04f8617b) SHA1(64deef2269790d8460d0ad510548e178f0f61607) )
	ROM_LOAD( "mrsd-8203a-j6.f5", 0x2000, 0x2000, CRC(1ffb5fc3) SHA1(e8fc7b95663a396ef7d46ba6ce24973a3c343381) )
	ROM_LOAD( "mrsd-8203a-l6.f6", 0x4000, 0x2000, CRC(5a0f5030) SHA1(d1530230fe6c666f7920cb82cb47f5fcc7e1ecc8) )

	ROM_REGION( 0x2000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "mrsd-8203b-k6.f10", 0x0000, 0x1000, CRC(e33cb26e) SHA1(207fa986754f8d7cd0bb3e56fd271ee0c1990269) )
	ROM_LOAD( "mrsd-8203b-l6.f11", 0x1000, 0x1000, CRC(a327ba05) SHA1(5eac27b48d14fec179919fe0902a6c7ada95f2b2) )

	ROM_REGION( 0x2000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "mrsd-8203b-m2.f7", 0x0000, 0x1000, CRC(a00ae797) SHA1(adff7f38870b7e8fa114886792a3acbb7a5726ab) )
	ROM_LOAD( "mrsd-8203b-n2.f8", 0x1000, 0x1000, CRC(81f2bdbd) SHA1(45ee1d62462cfadf7d2c46767f03ccfb3c876c08) )

	ROM_REGION( 0x0060, REGION_PROMS, 0 )
	ROM_LOAD( "mrsd-10-1.a2",  0x0000, 0x0020, CRC(4a819ad4) SHA1(d9072af7e52b506c1bcf8a327242d470eb240857) )
	ROM_LOAD( "mrsd-10-2.l3",  0x0020, 0x0020, CRC(2d926a3a) SHA1(129fb60ce3df67614e39dcaac9c93f0652addbbb) )
	ROM_LOAD( "mrsd-10-3.c1",  0x0040, 0x0020, CRC(27fa3a50) SHA1(7cf59b7a37c156640d6ea91554d1c4276c1780e0) ) /* ?? */
ROM_END


DRIVER_INIT( dorodon )
{
	/* Decode the opcodes */

	offs_t i;
	UINT8 *rom = memory_region(REGION_CPU1);
	offs_t diff = memory_region_length(REGION_CPU1) / 2;
	UINT8 *table = memory_region(REGION_USER1);

	memory_set_opcode_base(0,rom+diff);

	for (i = 0;i < diff;i++)
	{
		rom[i + diff] = table[rom[i]];
	}
}

DRIVER_INIT( sraider )
{
}

GAME( 1981, cavenger, 0,       ladybug, cavenger, 0,       ROT0,   "Universal", "Cosmic Avenger" )
GAME( 1981, ladybug,  0,       ladybug, ladybug,  0,       ROT270, "Universal", "Lady Bug" )
GAME( 1981, ladybugb, ladybug, ladybug, ladybug,  0,       ROT270, "bootleg",   "Lady Bug (bootleg)" )
GAME( 1982, dorodon,  0,       ladybug, dorodon,  dorodon, ROT270, "Falcon",    "Dorodon (set 1)" )
GAME( 1982, dorodon2, dorodon, ladybug, dorodon,  dorodon, ROT270, "Falcon",    "Dorodon (set 2)" )
GAME( 1982, snapjack, 0,       ladybug, snapjack, 0,       ROT0,   "Universal", "Snap Jack" )
GAME( 1982, sraider,  0,       sraider, sraider,  sraider, ROT270, "Universal", "Space Raider" )
GAME( 1982, mrsdyna,  0,       mrsdyna, mrsdyna,  0,       ROT270, "Universal", "Mrs. Dynamite" )
