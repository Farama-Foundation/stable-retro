/***************************************************************************

  The Main Event, (c) 1988 Konami
  Devastators, (c) 1988 Konami

Emulation by Bryan McPhail, mish@tendril.co.uk

Notes:
- Schematics show a palette/work RAM bank selector, but this doesn't seem
  to be used?

- In Devastators, shadows don't work. Bit 7 of the sprite attribute is always 0,
  could there be a global enable flag in the 051960?
  This is particularly evident in level 2 where plane shadows cover other sprites.
  The priority/shadow encoder PROM is quite complex, however bits 5-7 of the sprite
  attribute don't seem to be used, at least not in the first two levels, so the
  PROM just maps to the fixed priority order currently implemented.

- In Devastators, sprite zooming for the planes in level 2 is particularly bad.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "vidhrdw/konamiic.h"
#include "cpu/m6809/m6809.h"


VIDEO_UPDATE( mainevt );
VIDEO_UPDATE( dv );
VIDEO_START( mainevt );
VIDEO_START( dv );



static INTERRUPT_GEN( mainevt_interrupt )
{
	if (K052109_is_IRQ_enabled())
		irq0_line_hold();
}


static int nmi_enable;

static WRITE_HANDLER( dv_nmienable_w )
{
	nmi_enable = data;
}

static INTERRUPT_GEN( dv_interrupt )
{
	if (nmi_enable)
		nmi_line_pulse();
}


WRITE_HANDLER( mainevt_bankswitch_w )
{
	unsigned char *RAM = memory_region(REGION_CPU1);
	int bankaddress;

	/* bit 0-1 ROM bank select */
	bankaddress = 0x10000 + (data & 0x03) * 0x2000;
	cpu_setbank(1,&RAM[bankaddress]);

	/* TODO: bit 5 = select work RAM or palette? */
/*	palette_selected = data & 0x20;*/

	/* bit 6 = enable char ROM reading through the video RAM */
	K052109_set_RMRD_line((data & 0x40) ? ASSERT_LINE : CLEAR_LINE);

	/* bit 7 = NINITSET (unknown) */

	/* other bits unused */
}

WRITE_HANDLER( mainevt_coin_w )
{
	coin_counter_w(0,data & 0x10);
	coin_counter_w(1,data & 0x20);
	set_led_status(0,data & 0x01);
	set_led_status(1,data & 0x02);
	set_led_status(2,data & 0x04);
	set_led_status(3,data & 0x08);
}

WRITE_HANDLER( mainevt_sh_irqtrigger_w )
{
	cpu_set_irq_line_and_vector(1,0,HOLD_LINE,0xff);
}

WRITE_HANDLER( mainevt_sh_irqcontrol_w )
{
	upd7759_reset_w(0, data & 2);
	upd7759_start_w(0, data & 1);

	interrupt_enable_w(0,data & 4);
}

WRITE_HANDLER( devstor_sh_irqcontrol_w )
{
interrupt_enable_w(0,data & 4);
}

WRITE_HANDLER( mainevt_sh_bankswitch_w )
{
	int bank_A,bank_B;

/*logerror("CPU #1 PC: %04x bank switch = %02x\n",activecpu_get_pc(),data);*/

	/* bits 0-3 select the 007232 banks */
	bank_A=(data&0x3);
	bank_B=((data>>2)&0x3);
	K007232_set_bank( 0, bank_A, bank_B );

	/* bits 4-5 select the UPD7759 bank */
	upd7759_set_bank_base(0, ((data >> 4) & 0x03) * 0x20000);
}

WRITE_HANDLER( dv_sh_bankswitch_w )
{
	int bank_A,bank_B;

/*logerror("CPU #1 PC: %04x bank switch = %02x\n",activecpu_get_pc(),data);*/

	/* bits 0-3 select the 007232 banks */
	bank_A=(data&0x3);
	bank_B=((data>>2)&0x3);
	K007232_set_bank( 0, bank_A, bank_B );
}


static MEMORY_READ_START( readmem )
	{ 0x1f94, 0x1f94, input_port_0_r }, /* Coins */
	{ 0x1f95, 0x1f95, input_port_1_r }, /* Player 1 */
	{ 0x1f96, 0x1f96, input_port_2_r }, /* Player 2 */
	{ 0x1f97, 0x1f97, input_port_5_r }, /* Dip 1 */
	{ 0x1f98, 0x1f98, input_port_7_r }, /* Dip 3 */
	{ 0x1f99, 0x1f99, input_port_3_r }, /* Player 3 */
	{ 0x1f9a, 0x1f9a, input_port_4_r }, /* Player 4 */
	{ 0x1f9b, 0x1f9b, input_port_6_r }, /* Dip 2 */

	{ 0x0000, 0x3fff, K052109_051960_r },
	{ 0x4000, 0x5fff, MRA_RAM },
	{ 0x6000, 0x7fff, MRA_BANK1 },
	{ 0x8000, 0xffff, MRA_ROM },
MEMORY_END

static MEMORY_WRITE_START( writemem )
	{ 0x1f80, 0x1f80, mainevt_bankswitch_w },
	{ 0x1f84, 0x1f84, soundlatch_w },				/* probably */
	{ 0x1f88, 0x1f88, mainevt_sh_irqtrigger_w },	/* probably */
	{ 0x1f8c, 0x1f8d, MWA_NOP },	/* ??? */
	{ 0x1f90, 0x1f90, mainevt_coin_w },	/* coin counters + lamps */

	{ 0x0000, 0x3fff, K052109_051960_w },
	{ 0x4000, 0x5dff, MWA_RAM },
	{ 0x5e00, 0x5fff, paletteram_xBBBBBGGGGGRRRRR_swap_w, &paletteram },
 	{ 0x6000, 0xffff, MWA_ROM },
MEMORY_END


static MEMORY_READ_START( dv_readmem )
	{ 0x1f94, 0x1f94, input_port_0_r }, /* Coins */
	{ 0x1f95, 0x1f95, input_port_1_r }, /* Player 1 */
	{ 0x1f96, 0x1f96, input_port_2_r }, /* Player 2 */
	{ 0x1f97, 0x1f97, input_port_5_r }, /* Dip 1 */
	{ 0x1f98, 0x1f98, input_port_7_r }, /* Dip 3 */
	{ 0x1f9b, 0x1f9b, input_port_6_r }, /* Dip 2 */
	{ 0x1fa0, 0x1fbf, K051733_r },

	{ 0x0000, 0x3fff, K052109_051960_r },
	{ 0x4000, 0x5fff, MRA_RAM },
	{ 0x6000, 0x7fff, MRA_BANK1 },
	{ 0x8000, 0xffff, MRA_ROM },
MEMORY_END

static MEMORY_WRITE_START( dv_writemem )
	{ 0x1f80, 0x1f80, mainevt_bankswitch_w },
	{ 0x1f84, 0x1f84, soundlatch_w },				/* probably */
	{ 0x1f88, 0x1f88, mainevt_sh_irqtrigger_w },	/* probably */
	{ 0x1f90, 0x1f90, mainevt_coin_w },	/* coin counters + lamps */
	{ 0x1fb2, 0x1fb2, dv_nmienable_w },
	{ 0x1fa0, 0x1fbf, K051733_w },

	{ 0x0000, 0x3fff, K052109_051960_w },
	{ 0x4000, 0x5dff, MWA_RAM },
	{ 0x5e00, 0x5fff, paletteram_xBBBBBGGGGGRRRRR_swap_w, &paletteram },
	{ 0x6000, 0xffff, MWA_ROM },
MEMORY_END


static MEMORY_READ_START( sound_readmem )
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0x83ff, MRA_RAM },
	{ 0xa000, 0xa000, soundlatch_r },
	{ 0xb000, 0xb00d, K007232_read_port_0_r },
	{ 0xd000, 0xd000, upd7759_0_busy_r },
MEMORY_END

static MEMORY_WRITE_START( sound_writemem )
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0x8000, 0x83ff, MWA_RAM },
	{ 0xb000, 0xb00d, K007232_write_port_0_w },
	{ 0x9000, 0x9000, upd7759_0_port_w },
	{ 0xe000, 0xe000, mainevt_sh_irqcontrol_w },
	{ 0xf000, 0xf000, mainevt_sh_bankswitch_w },
MEMORY_END

static MEMORY_READ_START( dv_sound_readmem )
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0x83ff, MRA_RAM },
	{ 0xa000, 0xa000, soundlatch_r },
	{ 0xb000, 0xb00d, K007232_read_port_0_r },
	{ 0xc001, 0xc001, YM2151_status_port_0_r },
MEMORY_END

static MEMORY_WRITE_START( dv_sound_writemem )
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0x8000, 0x83ff, MWA_RAM },
	{ 0xb000, 0xb00d, K007232_write_port_0_w },
	{ 0xc000, 0xc000, YM2151_register_port_0_w },
	{ 0xc001, 0xc001, YM2151_data_port_0_w },
	{ 0xe000, 0xe000, devstor_sh_irqcontrol_w },
	{ 0xf000, 0xf000, dv_sh_bankswitch_w },
MEMORY_END



/*****************************************************************************/

INPUT_PORTS_START( mainevt )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN4 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_SERVICE2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_SERVICE3 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_SERVICE4 )

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER3 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER3 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER3 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER3 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER3 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER3 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER4 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER4 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER4 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER4 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER4 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER4 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START
	PORT_DIPNAME( 0x0f, 0x0f, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x05, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 3C_2C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 4C_3C ) )
	PORT_DIPSETTING(    0x0f, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 4C_5C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 3C_4C ) )
	PORT_DIPSETTING(    0x07, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x0e, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 2C_5C ) )
	PORT_DIPSETTING(    0x0d, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x0b, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0x0a, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(    0x09, DEF_STR( 1C_7C ) )
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

 	PORT_START
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x18, 0x10, "Bonus Energy" )
	PORT_DIPSETTING(    0x00, "60" )
	PORT_DIPSETTING(    0x08, "70" )
	PORT_DIPSETTING(    0x10, "80" )
	PORT_DIPSETTING(    0x18, "90" )
	PORT_DIPNAME( 0x60, 0x40, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x60, "Easy" )
	PORT_DIPSETTING(    0x40, "Normal" )
	PORT_DIPSETTING(    0x20, "Difficult" )
	PORT_DIPSETTING(    0x00, "Very Difficult" )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_SERVICE( 0x04, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_BIT( 0xf0, IP_ACTIVE_LOW, IPT_UNUSED )
INPUT_PORTS_END

INPUT_PORTS_START( mainev2p )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_SERVICE2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START	/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START2 )

	PORT_START	/* IN1 */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* IN2 */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START
	PORT_DIPNAME( 0x0f, 0x0f, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x05, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 3C_2C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 4C_3C ) )
	PORT_DIPSETTING(    0x0f, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 4C_5C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 3C_4C ) )
	PORT_DIPSETTING(    0x07, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x0e, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 2C_5C ) )
	PORT_DIPSETTING(    0x0d, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x0b, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0x0a, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(    0x09, DEF_STR( 1C_7C ) )
	PORT_DIPNAME( 0xf0, 0xf0, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x50, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 3C_2C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 4C_3C ) )
	PORT_DIPSETTING(    0xf0, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 4C_5C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 3C_4C ) )
	PORT_DIPSETTING(    0x70, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0xe0, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x60, DEF_STR( 2C_5C ) )
	PORT_DIPSETTING(    0xd0, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0xb0, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0xa0, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(    0x90, DEF_STR( 1C_7C ) )

 	PORT_START
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x18, 0x10, "Bonus Energy" )
	PORT_DIPSETTING(    0x00, "60" )
	PORT_DIPSETTING(    0x08, "70" )
	PORT_DIPSETTING(    0x10, "80" )
	PORT_DIPSETTING(    0x18, "90" )
	PORT_DIPNAME( 0x60, 0x40, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x60, "Easy" )
	PORT_DIPSETTING(    0x40, "Normal" )
	PORT_DIPSETTING(    0x20, "Difficult" )
	PORT_DIPSETTING(    0x00, "Very Difficult" )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_SERVICE( 0x04, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_BIT( 0xf0, IP_ACTIVE_LOW, IPT_UNUSED )
INPUT_PORTS_END

INPUT_PORTS_START( devstors )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START	/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START2 )

	PORT_START
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START
	PORT_DIPNAME( 0x0f, 0x0f, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x05, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 3C_2C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 4C_3C ) )
	PORT_DIPSETTING(    0x0f, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 3C_4C ) )
	PORT_DIPSETTING(    0x07, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x0e, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 2C_5C ) )
	PORT_DIPSETTING(    0x0d, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x0b, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0x0a, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(    0x09, DEF_STR( 1C_7C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0xf0, 0xf0, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x50, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 3C_2C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 4C_3C ) )
	PORT_DIPSETTING(    0xf0, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 3C_4C ) )
	PORT_DIPSETTING(    0x70, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0xe0, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x60, DEF_STR( 2C_5C ) )
	PORT_DIPSETTING(    0xd0, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0xb0, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0xa0, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(    0x90, DEF_STR( 1C_7C ) )
/*	PORT_DIPSETTING(    0x00, "Invalid" )*/

 	PORT_START
	PORT_DIPNAME( 0x03, 0x02, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x03, "2" )
	PORT_DIPSETTING(    0x02, "3" )
	PORT_DIPSETTING(    0x01, "5" )
	PORT_DIPSETTING(    0x00, "7" )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x18, 0x18, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x18, "150 and every 200" )
	PORT_DIPSETTING(    0x10, "150 and every 250" )
	PORT_DIPSETTING(    0x08, "150" )
	PORT_DIPSETTING(    0x00, "200" )
	PORT_DIPNAME( 0x60, 0x40, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x60, "Easy" )
	PORT_DIPSETTING(    0x40, "Normal" )
	PORT_DIPSETTING(    0x20, "Difficult" )
	PORT_DIPSETTING(    0x00, "Very Difficult" )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_SERVICE( 0x04, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_BIT( 0xf0, IP_ACTIVE_LOW, IPT_UNUSED )
INPUT_PORTS_END

/* Same as 'devstors', but additional "Cocktail" Dip Switch (even if I don't see the use) */
INPUT_PORTS_START( devstor2 )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START	/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START2 )

	PORT_START
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START
	PORT_DIPNAME( 0x0f, 0x0f, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x05, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 3C_2C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 4C_3C ) )
	PORT_DIPSETTING(    0x0f, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 3C_4C ) )
	PORT_DIPSETTING(    0x07, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x0e, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 2C_5C ) )
	PORT_DIPSETTING(    0x0d, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x0b, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0x0a, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(    0x09, DEF_STR( 1C_7C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0xf0, 0xf0, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x50, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 3C_2C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 4C_3C ) )
	PORT_DIPSETTING(    0xf0, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 3C_4C ) )
	PORT_DIPSETTING(    0x70, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0xe0, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x60, DEF_STR( 2C_5C ) )
	PORT_DIPSETTING(    0xd0, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0xb0, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0xa0, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(    0x90, DEF_STR( 1C_7C ) )
/*	PORT_DIPSETTING(    0x00, "Invalid" )*/

 	PORT_START
	PORT_DIPNAME( 0x03, 0x02, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x03, "2" )
	PORT_DIPSETTING(    0x02, "3" )
	PORT_DIPSETTING(    0x01, "5" )
	PORT_DIPSETTING(    0x00, "7" )
	PORT_DIPNAME( 0x04, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x18, 0x18, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x18, "150 and every 200" )
	PORT_DIPSETTING(    0x10, "150 and every 250" )
	PORT_DIPSETTING(    0x08, "150" )
	PORT_DIPSETTING(    0x00, "200" )
	PORT_DIPNAME( 0x60, 0x40, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x60, "Easy" )
	PORT_DIPSETTING(    0x40, "Normal" )
	PORT_DIPSETTING(    0x20, "Difficult" )
	PORT_DIPSETTING(    0x00, "Very Difficult" )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_SERVICE( 0x04, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_BIT( 0xf0, IP_ACTIVE_LOW, IPT_UNUSED )
INPUT_PORTS_END

/*****************************************************************************/

static void volume_callback(int v)
{
	K007232_set_volume(0,0,(v >> 4) * 0x11,0);
	K007232_set_volume(0,1,0,(v & 0x0f) * 0x11);
}

static struct K007232_interface k007232_interface =
{
	1,		/* number of chips */
	3579545,	/* clock */
	{ REGION_SOUND1 },	/* memory regions */
	{ K007232_VOL(20,MIXER_PAN_CENTER,20,MIXER_PAN_CENTER) },	/* volume */
	{ volume_callback }	/* external port callback */
};

static struct upd7759_interface upd7759_interface =
{
	1,		/* number of chips */
	{ UPD7759_STANDARD_CLOCK },
	{ 50 }, /* volume */
	{ REGION_SOUND2 },		/* memory region */
	{0}
};

static struct YM2151interface ym2151_interface =
{
	1,			/* 1 chip */
	3579545,	/* 3.579545 MHz */
	{ YM3012_VOL(30,MIXER_PAN_CENTER,30,MIXER_PAN_CENTER) },
	{ 0 }
};

static MACHINE_DRIVER_START( mainevt )

	/* basic machine hardware */
	MDRV_CPU_ADD(HD6309, 3000000)	/* ?? */
	MDRV_CPU_MEMORY(readmem,writemem)
	MDRV_CPU_VBLANK_INT(mainevt_interrupt,1)

	MDRV_CPU_ADD(Z80, 3579545)
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)	/* 3.579545 MHz */
	MDRV_CPU_MEMORY(sound_readmem,sound_writemem)
	MDRV_CPU_VBLANK_INT(nmi_line_pulse,8)	/* ??? */

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_HAS_SHADOWS)
	MDRV_SCREEN_SIZE(64*8, 32*8)
	MDRV_VISIBLE_AREA(14*8, (64-14)*8-1, 2*8, 30*8-1 )
	MDRV_PALETTE_LENGTH(256)

	MDRV_VIDEO_START(mainevt)
	MDRV_VIDEO_UPDATE(mainevt)

	/* sound hardware */
	MDRV_SOUND_ADD(K007232, k007232_interface)
	MDRV_SOUND_ADD(UPD7759, upd7759_interface)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( devstors )

	/* basic machine hardware */
	MDRV_CPU_ADD(HD6309, 3000000)	/* ?? */
	MDRV_CPU_MEMORY(dv_readmem,dv_writemem)
	MDRV_CPU_VBLANK_INT(dv_interrupt,1)

	MDRV_CPU_ADD(Z80, 3579545)
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)	/* 3.579545 MHz */
	MDRV_CPU_MEMORY(dv_sound_readmem,dv_sound_writemem)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,4)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_HAS_SHADOWS)
	MDRV_SCREEN_SIZE(64*8, 32*8)
	MDRV_VISIBLE_AREA(13*8, (64-13)*8-1, 2*8, 30*8-1 )
	MDRV_PALETTE_LENGTH(256)

	MDRV_VIDEO_START(dv)
	MDRV_VIDEO_UPDATE(dv)

	/* sound hardware */
	MDRV_SOUND_ADD(YM2151, ym2151_interface)
	MDRV_SOUND_ADD(K007232, k007232_interface)
MACHINE_DRIVER_END



/***************************************************************************

  Game driver(s)

***************************************************************************/

/* 4 players - English title screen - No "Warning" message in the ROM */

ROM_START( mainevt )
	ROM_REGION( 0x40000, REGION_CPU1, 0 )
	ROM_LOAD( "799c02.k11",   0x10000, 0x08000, CRC(e2e7dbd5) SHA1(80314cd42a9f47f7bb82a2160fb5ef2ddc6dff30) )
	ROM_CONTINUE(             0x08000, 0x08000 )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for the audio CPU */
	ROM_LOAD( "799c01.f7",    0x00000, 0x08000, CRC(447c4c5c) SHA1(86e42132793c59cc6feece143516f7ecd4ed14e8) )

	ROM_REGION( 0x20000, REGION_GFX1, 0 )	/* graphics (addressable by the main CPU) */
	ROM_LOAD16_BYTE( "799c06.f22",   0x00000, 0x08000, CRC(f839cb58) SHA1(b36202ca2b68b6249c3f972ad09501e28a0162f7) )
	ROM_LOAD16_BYTE( "799c07.h22",   0x00001, 0x08000, CRC(176df538) SHA1(379e1de81afb85b1559de170cd2ab9f4af2b137e) )
	ROM_LOAD16_BYTE( "799c08.j22",   0x10000, 0x08000, CRC(d01e0078) SHA1(7ac242eb24271ac2783ec4d9e97ae051f1f3363a) )
	ROM_LOAD16_BYTE( "799c09.k22",   0x10001, 0x08000, CRC(9baec75e) SHA1(a8f6102c8fd46f18678f336bc44be31458ca9256) )

	ROM_REGION( 0x100000, REGION_GFX2, 0 )	/* graphics (addressable by the main CPU) */
	ROM_LOAD( "799b04.h4",    0x00000, 0x80000, CRC(323e0c2b) SHA1(c108d656b6ceff13c910739e4ca760acbb640de3) )
	ROM_LOAD( "799b05.k4",    0x80000, 0x80000, CRC(571c5831) SHA1(2a18f0bcf6946ada6e0bde7edbd11afd4db1c170) )

	ROM_REGION( 0x0100, REGION_PROMS, 0 )
	ROM_LOAD( "63s141n.bin",  0x0000, 0x0100, CRC(61f6c8d1) SHA1(c70f1f8e434aaaffb89e30e2230a08374ef324ad) )	/* priority encoder (not used) */

	ROM_REGION( 0x80000, REGION_SOUND1, 0 )	/* 512k for 007232 samples */
	ROM_LOAD( "799b03.d4",    0x00000, 0x80000, CRC(f1cfd342) SHA1(079afc5c631de7f5b652d0ce6fd44b3aacd14a1b) )

	ROM_REGION( 0x80000, REGION_SOUND2, 0 )	/* 512k for the UPD7759C samples */
	ROM_LOAD( "799b06.c22",   0x00000, 0x80000, CRC(2c8c47d7) SHA1(18a899767177ddfd870df9ed156d8bbc04b58a19) )
ROM_END


/* 4 players - English title screen - No "Warning" message in the ROM */

ROM_START( mainevto )
	ROM_REGION( 0x40000, REGION_CPU1, 0 )
	ROM_LOAD( "799_02.k11",   0x10000, 0x08000, CRC(c143596b) SHA1(5da7efaf0f7c7a493cc242eae115f278bc9c134b) )
	ROM_CONTINUE(             0x08000, 0x08000 )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for the audio CPU */
	ROM_LOAD( "799c01.f7",    0x00000, 0x08000, CRC(447c4c5c) SHA1(86e42132793c59cc6feece143516f7ecd4ed14e8) )

	ROM_REGION( 0x20000, REGION_GFX1, 0 )	/* graphics (addressable by the main CPU) */
	ROM_LOAD16_BYTE( "799c06.f22",   0x00000, 0x08000, CRC(f839cb58) SHA1(b36202ca2b68b6249c3f972ad09501e28a0162f7) )
	ROM_LOAD16_BYTE( "799c07.h22",   0x00001, 0x08000, CRC(176df538) SHA1(379e1de81afb85b1559de170cd2ab9f4af2b137e) )
	ROM_LOAD16_BYTE( "799c08.j22",   0x10000, 0x08000, CRC(d01e0078) SHA1(7ac242eb24271ac2783ec4d9e97ae051f1f3363a) )
	ROM_LOAD16_BYTE( "799c09.k22",   0x10001, 0x08000, CRC(9baec75e) SHA1(a8f6102c8fd46f18678f336bc44be31458ca9256) )

	ROM_REGION( 0x100000, REGION_GFX2, 0 )	/* graphics (addressable by the main CPU) */
	ROM_LOAD( "799b04.h4",    0x00000, 0x80000, CRC(323e0c2b) SHA1(c108d656b6ceff13c910739e4ca760acbb640de3) )
	ROM_LOAD( "799b05.k4",    0x80000, 0x80000, CRC(571c5831) SHA1(2a18f0bcf6946ada6e0bde7edbd11afd4db1c170) )

	ROM_REGION( 0x0100, REGION_PROMS, 0 )
	ROM_LOAD( "63s141n.bin",  0x0000, 0x0100, CRC(61f6c8d1) SHA1(c70f1f8e434aaaffb89e30e2230a08374ef324ad) )	/* priority encoder (not used) */

	ROM_REGION( 0x80000, REGION_SOUND1, 0 )	/* 512k for 007232 samples */
	ROM_LOAD( "799b03.d4",    0x00000, 0x80000, CRC(f1cfd342) SHA1(079afc5c631de7f5b652d0ce6fd44b3aacd14a1b) )

	ROM_REGION( 0x80000, REGION_SOUND2, 0 )	/* 512k for the UPD7759C samples */
	ROM_LOAD( "799b06.c22",   0x00000, 0x80000, CRC(2c8c47d7) SHA1(18a899767177ddfd870df9ed156d8bbc04b58a19) )
ROM_END


/* 2 players - English title screen - "Warning" message in the ROM (not displayed) */

ROM_START( mainev2p )
	ROM_REGION( 0x40000, REGION_CPU1, 0 )
	ROM_LOAD( "799x02.k11",   0x10000, 0x08000, CRC(42cfc650) SHA1(2d1918ebc0d93a2356ad995a6854dbde7c3b8daf) )
	ROM_CONTINUE(             0x08000, 0x08000 )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for the audio CPU */
	ROM_LOAD( "799c01.f7",    0x00000, 0x08000, CRC(447c4c5c) SHA1(86e42132793c59cc6feece143516f7ecd4ed14e8) )

	ROM_REGION( 0x20000, REGION_GFX1, 0 )	/* graphics (addressable by the main CPU) */
	ROM_LOAD16_BYTE( "799c06.f22",   0x00000, 0x08000, CRC(f839cb58) SHA1(b36202ca2b68b6249c3f972ad09501e28a0162f7) )
	ROM_LOAD16_BYTE( "799c07.h22",   0x00001, 0x08000, CRC(176df538) SHA1(379e1de81afb85b1559de170cd2ab9f4af2b137e) )
	ROM_LOAD16_BYTE( "799c08.j22",   0x10000, 0x08000, CRC(d01e0078) SHA1(7ac242eb24271ac2783ec4d9e97ae051f1f3363a) )
	ROM_LOAD16_BYTE( "799c09.k22",   0x10001, 0x08000, CRC(9baec75e) SHA1(a8f6102c8fd46f18678f336bc44be31458ca9256) )

	ROM_REGION( 0x100000, REGION_GFX2, 0 )	/* graphics (addressable by the main CPU) */
	ROM_LOAD( "799b04.h4",    0x00000, 0x80000, CRC(323e0c2b) SHA1(c108d656b6ceff13c910739e4ca760acbb640de3) )
	ROM_LOAD( "799b05.k4",    0x80000, 0x80000, CRC(571c5831) SHA1(2a18f0bcf6946ada6e0bde7edbd11afd4db1c170) )

	ROM_REGION( 0x0100, REGION_PROMS, 0 )
	ROM_LOAD( "63s141n.bin",  0x0000, 0x0100, CRC(61f6c8d1) SHA1(c70f1f8e434aaaffb89e30e2230a08374ef324ad) )	/* priority encoder (not used) */

	ROM_REGION( 0x80000, REGION_SOUND1, 0 )	/* 512k for 007232 samples */
	ROM_LOAD( "799b03.d4",    0x00000, 0x80000, CRC(f1cfd342) SHA1(079afc5c631de7f5b652d0ce6fd44b3aacd14a1b) )

	ROM_REGION( 0x80000, REGION_SOUND2, 0 )	/* 512k for the UPD7759C samples */
	ROM_LOAD( "799b06.c22",   0x00000, 0x80000, CRC(2c8c47d7) SHA1(18a899767177ddfd870df9ed156d8bbc04b58a19) )
ROM_END


/* 2 players - Japan title screen - "Warning" message in the ROM (displayed) */

ROM_START( ringohja )
	ROM_REGION( 0x40000, REGION_CPU1, 0 )
	ROM_LOAD( "799n02.k11",   0x10000, 0x08000, CRC(f9305dd0) SHA1(7135053be9d46ac9c09ab63eca1eb71825a71a13) )
	ROM_CONTINUE(             0x08000, 0x08000 )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for the audio CPU */
	ROM_LOAD( "799c01.f7",    0x00000, 0x08000, CRC(447c4c5c) SHA1(86e42132793c59cc6feece143516f7ecd4ed14e8) )

	ROM_REGION( 0x20000, REGION_GFX1, 0 )	/* graphics (addressable by the main CPU) */
	ROM_LOAD16_BYTE( "799c06.f22",   0x00000, 0x08000, CRC(f839cb58) SHA1(b36202ca2b68b6249c3f972ad09501e28a0162f7) )
	ROM_LOAD16_BYTE( "799c07.h22",   0x00001, 0x08000, CRC(176df538) SHA1(379e1de81afb85b1559de170cd2ab9f4af2b137e) )
	ROM_LOAD16_BYTE( "799c08.j22",   0x10000, 0x08000, CRC(d01e0078) SHA1(7ac242eb24271ac2783ec4d9e97ae051f1f3363a) )
	ROM_LOAD16_BYTE( "799c09.k22",   0x10001, 0x08000, CRC(9baec75e) SHA1(a8f6102c8fd46f18678f336bc44be31458ca9256) )

	ROM_REGION( 0x100000, REGION_GFX2, 0 )	/* graphics (addressable by the main CPU) */
	ROM_LOAD( "799b04.h4",    0x00000, 0x80000, CRC(323e0c2b) SHA1(c108d656b6ceff13c910739e4ca760acbb640de3) )
	ROM_LOAD( "799b05.k4",    0x80000, 0x80000, CRC(571c5831) SHA1(2a18f0bcf6946ada6e0bde7edbd11afd4db1c170) )

	ROM_REGION( 0x0100, REGION_PROMS, 0 )
	ROM_LOAD( "63s141n.bin",  0x0000, 0x0100, CRC(61f6c8d1) SHA1(c70f1f8e434aaaffb89e30e2230a08374ef324ad) )	/* priority encoder (not used) */

	ROM_REGION( 0x80000, REGION_SOUND1, 0 )	/* 512k for 007232 samples */
	ROM_LOAD( "799b03.d4",    0x00000, 0x80000, CRC(f1cfd342) SHA1(079afc5c631de7f5b652d0ce6fd44b3aacd14a1b) )

	ROM_REGION( 0x80000, REGION_SOUND2, 0 )	/* 512k for the UPD7759C samples */
	ROM_LOAD( "799b06.c22",   0x00000, 0x80000, CRC(2c8c47d7) SHA1(18a899767177ddfd870df9ed156d8bbc04b58a19) )
ROM_END

ROM_START( devstors )
	ROM_REGION( 0x40000, REGION_CPU1, 0 )
	ROM_LOAD( "890-z02.k11",  0x10000, 0x08000, CRC(ebeb306f) SHA1(838fcfe95dfedd61f21f34301d48e337db765ab2) )
	ROM_CONTINUE(             0x08000, 0x08000 )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for the audio CPU */
	ROM_LOAD( "dev-k01.rom",  0x00000, 0x08000, CRC(d44b3eb0) SHA1(26109fc56668b65f1a5aa6d8ec2c08fd70ca7c51) )

	ROM_REGION( 0x40000, REGION_GFX1, 0 )	/* graphics (addressable by the main CPU) */
	ROM_LOAD16_BYTE( "dev-f06.rom",  0x00000, 0x10000, CRC(26592155) SHA1(aa1f8662f091ca1eb495223e41a35edd861ae9e9) )
	ROM_LOAD16_BYTE( "dev-f07.rom",  0x00001, 0x10000, CRC(6c74fa2e) SHA1(419a2ad31d269fafe4c474bf512e935d5e018846) )
	ROM_LOAD16_BYTE( "dev-f08.rom",  0x20000, 0x10000, CRC(29e12e80) SHA1(6d09e190055218e2dfd07838f1446dfb5f801206) )
	ROM_LOAD16_BYTE( "dev-f09.rom",  0x20001, 0x10000, CRC(67ca40d5) SHA1(ff719f55d2534ff076fbdd2bcb7d12c683bfe958) )

	ROM_REGION( 0x100000, REGION_GFX2, 0 )	/* graphics (addressable by the main CPU) */
	ROM_LOAD( "dev-f04.rom",  0x00000, 0x80000, CRC(f16cd1fa) SHA1(60ea19c19918a71aded3c9ea398c956908e217f1) )
	ROM_LOAD( "dev-f05.rom",  0x80000, 0x80000, CRC(da37db05) SHA1(0b48d1021cf0dec78dae0ef183b4c61fea783533) )

	ROM_REGION( 0x0100, REGION_PROMS, 0 )
	ROM_LOAD( "devaprom.bin", 0x0000, 0x0100, CRC(d3620106) SHA1(528a0a34754902d0f262a9619c6105da6de99354) )	/* priority encoder (not used) */

	ROM_REGION( 0x80000, REGION_SOUND1, 0 )	/* 512k for 007232 samples */
 	ROM_LOAD( "dev-f03.rom",  0x00000, 0x80000, CRC(19065031) SHA1(12c47fbe28f85fa2f901fe52601188a5e9633f22) )
ROM_END

ROM_START( devstor2 )
	ROM_REGION( 0x40000, REGION_CPU1, 0 )
	ROM_LOAD( "dev-x02.rom",  0x10000, 0x08000, CRC(e58ebb35) SHA1(4253b6a7128534cc0866bc910a271d91ac8b40fd) )
	ROM_CONTINUE(             0x08000, 0x08000 )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for the audio CPU */
	ROM_LOAD( "dev-k01.rom",  0x00000, 0x08000, CRC(d44b3eb0) SHA1(26109fc56668b65f1a5aa6d8ec2c08fd70ca7c51) )

	ROM_REGION( 0x40000, REGION_GFX1, 0 )	/* graphics (addressable by the main CPU) */
	ROM_LOAD16_BYTE( "dev-f06.rom",  0x00000, 0x10000, CRC(26592155) SHA1(aa1f8662f091ca1eb495223e41a35edd861ae9e9) )
	ROM_LOAD16_BYTE( "dev-f07.rom",  0x00001, 0x10000, CRC(6c74fa2e) SHA1(419a2ad31d269fafe4c474bf512e935d5e018846) )
	ROM_LOAD16_BYTE( "dev-f08.rom",  0x20000, 0x10000, CRC(29e12e80) SHA1(6d09e190055218e2dfd07838f1446dfb5f801206) )
	ROM_LOAD16_BYTE( "dev-f09.rom",  0x20001, 0x10000, CRC(67ca40d5) SHA1(ff719f55d2534ff076fbdd2bcb7d12c683bfe958) )

	ROM_REGION( 0x100000, REGION_GFX2, 0 )	/* graphics (addressable by the main CPU) */
	ROM_LOAD( "dev-f04.rom",  0x00000, 0x80000, CRC(f16cd1fa) SHA1(60ea19c19918a71aded3c9ea398c956908e217f1) )
	ROM_LOAD( "dev-f05.rom",  0x80000, 0x80000, CRC(da37db05) SHA1(0b48d1021cf0dec78dae0ef183b4c61fea783533) )

	ROM_REGION( 0x0100, REGION_PROMS, 0 )
	ROM_LOAD( "devaprom.bin", 0x0000, 0x0100, CRC(d3620106) SHA1(528a0a34754902d0f262a9619c6105da6de99354) )	/* priority encoder (not used) */

	ROM_REGION( 0x80000, REGION_SOUND1, 0 )	/* 512k for 007232 samples */
 	ROM_LOAD( "dev-f03.rom",  0x00000, 0x80000, CRC(19065031) SHA1(12c47fbe28f85fa2f901fe52601188a5e9633f22) )
ROM_END

ROM_START( devstor3 )
	ROM_REGION( 0x40000, REGION_CPU1, 0 )
	ROM_LOAD( "890k02.k11",   0x10000, 0x08000, CRC(52f4ccdd) SHA1(074e526ed170a5f2083c8c0808734291a2ea7403) )
	ROM_CONTINUE(             0x08000, 0x08000 )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for the audio CPU */
	ROM_LOAD( "dev-k01.rom",  0x00000, 0x08000, CRC(d44b3eb0) SHA1(26109fc56668b65f1a5aa6d8ec2c08fd70ca7c51) )

	ROM_REGION( 0x40000, REGION_GFX1, 0 )	/* graphics (addressable by the main CPU) */
	ROM_LOAD16_BYTE( "dev-f06.rom",  0x00000, 0x10000, CRC(26592155) SHA1(aa1f8662f091ca1eb495223e41a35edd861ae9e9) )
	ROM_LOAD16_BYTE( "dev-f07.rom",  0x00001, 0x10000, CRC(6c74fa2e) SHA1(419a2ad31d269fafe4c474bf512e935d5e018846) )
	ROM_LOAD16_BYTE( "dev-f08.rom",  0x20000, 0x10000, CRC(29e12e80) SHA1(6d09e190055218e2dfd07838f1446dfb5f801206) )
	ROM_LOAD16_BYTE( "dev-f09.rom",  0x20001, 0x10000, CRC(67ca40d5) SHA1(ff719f55d2534ff076fbdd2bcb7d12c683bfe958) )

	ROM_REGION( 0x100000, REGION_GFX2, 0 )	/* graphics (addressable by the main CPU) */
	ROM_LOAD( "dev-f04.rom",  0x00000, 0x80000, CRC(f16cd1fa) SHA1(60ea19c19918a71aded3c9ea398c956908e217f1) )
	ROM_LOAD( "dev-f05.rom",  0x80000, 0x80000, CRC(da37db05) SHA1(0b48d1021cf0dec78dae0ef183b4c61fea783533) )

	ROM_REGION( 0x0100, REGION_PROMS, 0 )
	ROM_LOAD( "devaprom.bin", 0x0000, 0x0100, CRC(d3620106) SHA1(528a0a34754902d0f262a9619c6105da6de99354) )	/* priority encoder (not used) */

	ROM_REGION( 0x80000, REGION_SOUND1, 0 )	/* 512k for 007232 samples */
 	ROM_LOAD( "dev-f03.rom",  0x00000, 0x80000, CRC(19065031) SHA1(12c47fbe28f85fa2f901fe52601188a5e9633f22) )
ROM_END

ROM_START( garuka )
	ROM_REGION( 0x40000, REGION_CPU1, 0 )
	ROM_LOAD( "890w02.bin",   0x10000, 0x08000, CRC(b2f6f538) SHA1(95dad3258a2e4c5648d0fc22c06fa3e2da3b5ed1) )
	ROM_CONTINUE(             0x08000, 0x08000 )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for the audio CPU */
	ROM_LOAD( "dev-k01.rom",  0x00000, 0x08000, CRC(d44b3eb0) SHA1(26109fc56668b65f1a5aa6d8ec2c08fd70ca7c51) )

	ROM_REGION( 0x40000, REGION_GFX1, 0 )	/* graphics (addressable by the main CPU) */
	ROM_LOAD16_BYTE( "dev-f06.rom",  0x00000, 0x10000, CRC(26592155) SHA1(aa1f8662f091ca1eb495223e41a35edd861ae9e9) )
	ROM_LOAD16_BYTE( "dev-f07.rom",  0x00001, 0x10000, CRC(6c74fa2e) SHA1(419a2ad31d269fafe4c474bf512e935d5e018846) )
	ROM_LOAD16_BYTE( "dev-f08.rom",  0x20000, 0x10000, CRC(29e12e80) SHA1(6d09e190055218e2dfd07838f1446dfb5f801206) )
	ROM_LOAD16_BYTE( "dev-f09.rom",  0x20001, 0x10000, CRC(67ca40d5) SHA1(ff719f55d2534ff076fbdd2bcb7d12c683bfe958) )

	ROM_REGION( 0x100000, REGION_GFX2, 0 )	/* graphics (addressable by the main CPU) */
	ROM_LOAD( "dev-f04.rom",  0x00000, 0x80000, CRC(f16cd1fa) SHA1(60ea19c19918a71aded3c9ea398c956908e217f1) )
	ROM_LOAD( "dev-f05.rom",  0x80000, 0x80000, CRC(da37db05) SHA1(0b48d1021cf0dec78dae0ef183b4c61fea783533) )

	ROM_REGION( 0x0100, REGION_PROMS, 0 )
	ROM_LOAD( "devaprom.bin", 0x0000, 0x0100, CRC(d3620106) SHA1(528a0a34754902d0f262a9619c6105da6de99354) )	/* priority encoder (not used) */

	ROM_REGION( 0x80000, REGION_SOUND1, 0 )	/* 512k for 007232 samples */
 	ROM_LOAD( "dev-f03.rom",  0x00000, 0x80000, CRC(19065031) SHA1(12c47fbe28f85fa2f901fe52601188a5e9633f22) )
ROM_END



static DRIVER_INIT( mainevt )
{
	konami_rom_deinterleave_2(REGION_GFX1);
	konami_rom_deinterleave_2(REGION_GFX2);
}



GAME( 1988, mainevt,  0,        mainevt,  mainevt,  mainevt, ROT0,  "Konami", "The Main Event (4 Players ver. Y)" )
GAME( 1988, mainevto, mainevt,  mainevt,  mainevt,  mainevt, ROT0,  "Konami", "The Main Event (4 Players ver. F)" )
GAME( 1988, mainev2p, mainevt,  mainevt,  mainev2p, mainevt, ROT0,  "Konami", "The Main Event (2 Players ver. X)" )
GAME( 1988, ringohja, mainevt,  mainevt,  mainev2p, mainevt, ROT0,  "Konami", "Ring no Ohja (Japan 2 Players ver. N)" )
GAME( 1988, devstors, 0,        devstors, devstors, mainevt, ROT90, "Konami", "Devastators (ver. Z)" )
GAME( 1988, devstor2, devstors, devstors, devstor2, mainevt, ROT90, "Konami", "Devastators (ver. X)" )
GAME( 1988, devstor3, devstors, devstors, devstors, mainevt, ROT90, "Konami", "Devastators (ver. V)" )
GAME( 1988, garuka,   devstors, devstors, devstor2, mainevt, ROT90, "Konami", "Garuka (Japan ver. W)" )
