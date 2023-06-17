/***************************************************************************

 Lasso and similar hardware

		driver by Phil Stroffolino, Nicola Salmoria, Luca Elia

---------------------------------------------------------------------------
Year + Game					By				CPUs		Sound Chips
---------------------------------------------------------------------------
82	Lasso					SNK				3 x 6502	2 x SN76489
83	Chameleon				Jaleco			2 x 6502	2 x SN76489
84	Wai Wai Jockey Gate-In!	Jaleco/Casio	2 x 6502	2 x SN76489 + DAC
84  Pinbo                   Jaleco          6502 + Z80  2 x AY-8910
---------------------------------------------------------------------------

Notes:

- unknown CPU speeds (affect game timing)
- Lasso: fire button auto-repeats on high score entry screen (real behavior?)

***************************************************************************/

#include "driver.h"
#include "cpu/z80/z80.h"
#include "lasso.h"


/* IRQ = VBlank, NMI = Coin Insertion */

static INTERRUPT_GEN( lasso_interrupt )
{
	static int old;
	int new;

	/* VBlank*/
	if (cpu_getiloops() == 0)
	{
		cpu_set_irq_line(0, 0, HOLD_LINE);
		return;
	}

	/* Coins*/
	new = ~readinputport(3) & 0x30;

	if ( ((new & 0x10) && !(old & 0x10)) ||
		 ((new & 0x20) && !(old & 0x20)) )
		cpu_set_irq_line(0, IRQ_LINE_NMI, PULSE_LINE);

	old = new;
}


/* Shared RAM between Main CPU and sub CPU */

static data8_t *lasso_sharedram;

static READ_HANDLER( lasso_sharedram_r )
{
	return lasso_sharedram[offset];
}
static WRITE_HANDLER( lasso_sharedram_w )
{
	lasso_sharedram[offset] = data;
}


/* Write to the sound latch and generate an IRQ on the sound CPU */

static WRITE_HANDLER( sound_command_w )
{
	soundlatch_w(offset,data);
	cpu_set_irq_line( 1, 0, PULSE_LINE );
}

static READ_HANDLER( sound_status_r )
{
	/*	0x01: chip#0 ready; 0x02: chip#1 ready */
	return 0x03;
}

static data8_t lasso_chip_data;

static WRITE_HANDLER( sound_data_w )
{
	lasso_chip_data = BITSWAP8(data,0,1,2,3,4,5,6,7);
}

static WRITE_HANDLER( sound_select_w )
{
	if (~data & 0x01)	/* chip #0 */
		SN76496_0_w(0,lasso_chip_data);

	if (~data & 0x02)	/* chip #1 */
		SN76496_1_w(0,lasso_chip_data);
}



static MEMORY_READ_START( lasso_readmem )
	{ 0x0000, 0x0c7f, MRA_RAM },
	{ 0x1000, 0x17ff, lasso_sharedram_r	},
	{ 0x1804, 0x1804, input_port_0_r },
	{ 0x1805, 0x1805, input_port_1_r },
	{ 0x1806, 0x1806, input_port_2_r },
	{ 0x1807, 0x1807, input_port_3_r },
	{ 0x8000, 0xffff, MRA_ROM },
MEMORY_END

static MEMORY_WRITE_START( lasso_writemem )
	{ 0x0000, 0x03ff, MWA_RAM },
	{ 0x0400, 0x07ff, lasso_videoram_w, &lasso_videoram },
	{ 0x0800, 0x0bff, lasso_colorram_w, &lasso_colorram },
	{ 0x0c00, 0x0c7f, MWA_RAM, &lasso_spriteram, &lasso_spriteram_size },
	{ 0x1000, 0x17ff, lasso_sharedram_w	},
	{ 0x1800, 0x1800, sound_command_w },
	{ 0x1801, 0x1801, lasso_backcolor_w	},
	{ 0x1802, 0x1802, lasso_video_control_w },
	{ 0x1806, 0x1806, MWA_NOP },	/* games uses 'lsr' to read port*/
	{ 0x8000, 0xffff, MWA_ROM },
MEMORY_END


static MEMORY_READ_START( chameleo_readmem )
	{ 0x0000, 0x10ff, MRA_RAM },
	{ 0x1804, 0x1804, input_port_0_r },
	{ 0x1805, 0x1805, input_port_1_r },
	{ 0x1806, 0x1806, input_port_2_r },
	{ 0x1807, 0x1807, input_port_3_r },
	{ 0x2000, 0xffff, MRA_ROM },
MEMORY_END

static MEMORY_WRITE_START( chameleo_writemem )
	{ 0x0000, 0x03ff, MWA_RAM },
	{ 0x0400, 0x07ff, lasso_videoram_w, &lasso_videoram },
	{ 0x0800, 0x0bff, lasso_colorram_w, &lasso_colorram },
	{ 0x0c00, 0x0fff, MWA_RAM },	/**/
	{ 0x1000, 0x107f, MWA_RAM, &lasso_spriteram, &lasso_spriteram_size },
	{ 0x1080, 0x10ff, MWA_RAM },
	{ 0x1800, 0x1800, sound_command_w },
	{ 0x1801, 0x1801, lasso_backcolor_w	},
	{ 0x1802, 0x1802, lasso_video_control_w },
	{ 0x2000, 0xffff, MWA_ROM },
MEMORY_END


static MEMORY_READ_START( wwjgtin_readmem )
	{ 0x0000, 0x10ff, MRA_RAM },
	{ 0x1804, 0x1804, input_port_0_r },
	{ 0x1805, 0x1805, input_port_1_r },
	{ 0x1806, 0x1806, input_port_2_r },
	{ 0x1807, 0x1807, input_port_3_r },
	{ 0x5000, 0xbfff, MRA_ROM },
	{ 0xfffa, 0xffff, MRA_ROM },
MEMORY_END

static MEMORY_WRITE_START( wwjgtin_writemem )
	{ 0x0000, 0x07ff, MWA_RAM },
	{ 0x0800, 0x0bff, lasso_videoram_w, &lasso_videoram },
	{ 0x0c00, 0x0fff, lasso_colorram_w, &lasso_colorram },
	{ 0x1000, 0x10ff, MWA_RAM, &lasso_spriteram, &lasso_spriteram_size },
	{ 0x1800, 0x1800, sound_command_w },
	{ 0x1801, 0x1801, lasso_backcolor_w	},
	{ 0x1802, 0x1802, wwjgtin_video_control_w	},
	{ 0x1c00, 0x1c03, wwjgtin_lastcolor_w },
	{ 0x1c04, 0x1c07, MWA_RAM, &wwjgtin_track_scroll },
	{ 0x5000, 0xbfff, MWA_ROM },
	{ 0xfffa, 0xffff, MWA_ROM },
MEMORY_END


static MEMORY_WRITE_START( pinbo_writemem )
	{ 0x0000, 0x03ff, MWA_RAM },
	{ 0x0400, 0x07ff, lasso_videoram_w, &lasso_videoram },
	{ 0x0800, 0x0bff, lasso_colorram_w, &lasso_colorram },
	{ 0x1000, 0x10ff, MWA_RAM, &lasso_spriteram, &lasso_spriteram_size },
	{ 0x1800, 0x1800, sound_command_w },
	{ 0x1802, 0x1802, pinbo_video_control_w },
	{ 0x2000, 0xffff, MWA_ROM },
MEMORY_END


static MEMORY_READ_START( lasso_coprocessor_readmem )
	{ 0x0000, 0x07ff, MRA_RAM },
	{ 0x2000, 0x3fff, MRA_RAM },
	{ 0x8000, 0x8fff, MRA_ROM },
	{ 0xf000, 0xffff, MRA_ROM },
MEMORY_END

static MEMORY_WRITE_START( lasso_coprocessor_writemem )
	{ 0x0000, 0x07ff, MWA_RAM, &lasso_sharedram},
	{ 0x2000, 0x3fff, MWA_RAM, &lasso_bitmap_ram },
	{ 0x8000, 0x8fff, MWA_ROM },
	{ 0xf000, 0xffff, MWA_ROM },
MEMORY_END


static MEMORY_READ_START( lasso_sound_readmem )
	{ 0x0000, 0x01ff, MRA_RAM },
	{ 0x5000, 0x7fff, MRA_ROM },
	{ 0xb004, 0xb004, sound_status_r },
	{ 0xb005, 0xb005, soundlatch_r },
	{ 0xf000, 0xffff, MRA_ROM },
MEMORY_END

static MEMORY_WRITE_START( lasso_sound_writemem )
	{ 0x0000, 0x01ff, MWA_RAM },
	{ 0x5000, 0x7fff, MWA_ROM },
	{ 0xb000, 0xb000, sound_data_w },
	{ 0xb001, 0xb001, sound_select_w },
	{ 0xf000, 0xffff, MWA_ROM },
MEMORY_END


static MEMORY_READ_START( chameleo_sound_readmem )
	{ 0x0000, 0x01ff, MRA_RAM },
	{ 0x1000, 0x1fff, MRA_ROM },
	{ 0x6000, 0x7fff, MRA_ROM },
	{ 0xb004, 0xb004, sound_status_r },
	{ 0xb005, 0xb005, soundlatch_r },
	{ 0xfffa, 0xffff, MRA_ROM },
MEMORY_END

static MEMORY_WRITE_START( chameleo_sound_writemem )
	{ 0x0000, 0x01ff, MWA_RAM },
	{ 0x1000, 0x1fff, MWA_ROM },
	{ 0x6000, 0x7fff, MWA_ROM },
	{ 0xb000, 0xb000, sound_data_w },
	{ 0xb001, 0xb001, sound_select_w },
	{ 0xfffa, 0xffff, MWA_ROM },
MEMORY_END


static MEMORY_WRITE_START( wwjgtin_sound_writemem )
	{ 0x0000, 0x01ff, MWA_RAM },
	{ 0x5000, 0x7fff, MWA_ROM },
	{ 0xb000, 0xb000, sound_data_w },
	{ 0xb001, 0xb001, sound_select_w },
	{ 0xb003, 0xb003, DAC_0_data_w },
	{ 0xfffa, 0xffff, MWA_ROM },
MEMORY_END


static MEMORY_READ_START( pinbo_sound_readmem )
	{ 0x0000, 0x1fff, MRA_ROM },
	{ 0xf000, 0xffff, MRA_RAM },
MEMORY_END

static MEMORY_WRITE_START( pinbo_sound_writemem )
	{ 0x0000, 0x1fff, MWA_ROM },
	{ 0xf000, 0xffff, MWA_RAM },
MEMORY_END

static PORT_READ_START( pinbo_sound_readport )
	{ 0x02, 0x02, AY8910_read_port_0_r },
	{ 0x06, 0x06, AY8910_read_port_1_r },
	{ 0x08, 0x08, soundlatch_r },
PORT_END

static PORT_WRITE_START( pinbo_sound_writeport )
	{ 0x00, 0x00, AY8910_control_port_0_w },
	{ 0x01, 0x01, AY8910_write_port_0_w },
	{ 0x04, 0x04, AY8910_control_port_1_w },
	{ 0x05, 0x05, AY8910_write_port_1_w },
	{ 0x08, 0x08, MWA_NOP },	/* ??? */
	{ 0x14, 0x14, MWA_NOP },	/* ??? */
PORT_END



INPUT_PORTS_START( lasso )
	PORT_START /* 1804 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_4WAY )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_4WAY )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP    | IPF_4WAY )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN  | IPF_4WAY )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 ) /* lasso */
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON2 ) /* shoot */
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNUSED  )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNUSED  )

	PORT_START /* 1805 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_COCKTAIL | IPF_4WAY )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_COCKTAIL | IPF_4WAY )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP    | IPF_COCKTAIL | IPF_4WAY )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN  | IPF_COCKTAIL | IPF_4WAY )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON2	| IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNUSED  )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNUSED  )

	PORT_START /* 1806 */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(	0x01, DEF_STR( Upright ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x0e, 0x0e, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(	0x02, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(	0x0e, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(	0x08, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(	0x04, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(	0x0c, DEF_STR( 1C_6C ) )
/*	PORT_DIPSETTING(	0x06, DEF_STR( 1C_1C ) )*/
/*	PORT_DIPSETTING(	0x00, DEF_STR( 1C_1C ) )*/
/*	PORT_DIPSETTING(	0x0a, DEF_STR( 1C_1C ) )*/
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x30, "3" )
	PORT_DIPSETTING(    0x10, "4" )
	PORT_DIPSETTING(    0x20, "5" )
/*	PORT_DIPSETTING(    0x00, "3" )*/
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(	0x40, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(	0x00, DEF_STR( 1C_1C ) )
	PORT_DIPNAME( 0x80, 0x80, "Warm-Up Instructions" )
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x80, DEF_STR( Yes ) )

	PORT_START /* 1807 */
	PORT_DIPNAME( 0x01, 0x00, "Warm-Up" )
	PORT_DIPSETTING(    0x01, DEF_STR( No ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x02, 0x00, "Warm-Up Language" )
	PORT_DIPSETTING(    0x00, "English" )
	PORT_DIPSETTING(    0x02, "German" )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )	/* used */
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_BITX(    0x08, 0x00, IPT_DIPSWITCH_NAME | IPF_CHEAT, "Invulnerability", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_COIN2    )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN1    )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_START2  )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_START1  )
INPUT_PORTS_END

INPUT_PORTS_START( chameleo )
	PORT_START /* 1804 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_4WAY )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_4WAY )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP    | IPF_4WAY )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN  | IPF_4WAY )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START /* 1805 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_COCKTAIL | IPF_4WAY )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_COCKTAIL | IPF_4WAY )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP    | IPF_COCKTAIL | IPF_4WAY )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN  | IPF_COCKTAIL | IPF_4WAY )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START /* 1806 */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(	0x01, DEF_STR( Upright ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x0e, 0x0e, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(	0x02, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(	0x0e, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(	0x08, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(	0x04, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(	0x0c, DEF_STR( 1C_6C ) )
/*	PORT_DIPSETTING(	0x06, DEF_STR( 1C_1C ) )*/
/*	PORT_DIPSETTING(	0x00, DEF_STR( 1C_1C ) )*/
/*	PORT_DIPSETTING(	0x0a, DEF_STR( 1C_1C ) )*/
	PORT_DIPNAME( 0x30, 0x00, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x30, "5" )
/*	PORT_DIPSETTING(    0x10, "5" )*/
	PORT_BITX(0,        0x20, IPT_DIPSWITCH_SETTING | IPF_CHEAT, "Infinite", IP_JOY_NONE, IP_KEY_NONE )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(	0x40, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(	0x00, DEF_STR( 1C_1C ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START /* 1807 */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) )	/* probably unused */
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )	/* probably unused */
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )	/* probably unused */
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_COIN2    )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN1    )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_START2  )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_START1  )
INPUT_PORTS_END

INPUT_PORTS_START( wwjgtin )
	PORT_START /* 1804 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_4WAY )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_4WAY )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP    | IPF_4WAY )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN  | IPF_4WAY )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START /* 1805 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_PLAYER2 | IPF_4WAY )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_PLAYER2 | IPF_4WAY )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP    | IPF_PLAYER2 | IPF_4WAY )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN  | IPF_PLAYER2 | IPF_4WAY )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START /* 1806 */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) )	/* used - has to do with the controls */
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x0e, 0x0e, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(	0x02, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(	0x0e, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(	0x08, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(	0x04, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(	0x0c, DEF_STR( 1C_6C ) )
/*	PORT_DIPSETTING(	0x06, DEF_STR( 1C_1C ) )*/
/*	PORT_DIPSETTING(	0x00, DEF_STR( 1C_1C ) )*/
/*	PORT_DIPSETTING(	0x0a, DEF_STR( 1C_1C ) )*/
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )	/* probably unused */
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )	/* probably unused */
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(	0x40, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(	0x00, DEF_STR( 1C_1C ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )	/* probably unused */
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START /* 1807 */
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Bonus_Life) )
	PORT_DIPSETTING(    0x00, "20k" )
	PORT_DIPSETTING(    0x01, "50k" )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x02, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )	/* probably unused */
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )	/* probably unused */
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_BIT( 0x10, IP_ACTIVE_LOW,  IPT_COIN2   )
	PORT_BIT( 0x20, IP_ACTIVE_LOW,  IPT_COIN1   )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_START1  )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_START2  )
INPUT_PORTS_END

INPUT_PORTS_START( pinbo )
	PORT_START  /* 1804 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP    | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN  | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON2 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START  /* 1805 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON2 | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START /* 1806 */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x0e, 0x0e, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(	0x02, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(	0x0e, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(	0x08, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(	0x04, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(	0x0c, DEF_STR( 1C_6C ) )
/*	PORT_DIPSETTING(	0x06, DEF_STR( 1C_1C ) )*/
/*	PORT_DIPSETTING(	0x00, DEF_STR( 1C_1C ) )*/
/*	PORT_DIPSETTING(	0x0a, DEF_STR( 1C_1C ) )*/
	PORT_DIPNAME( 0x30, 0x00, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x10, "4" )
	PORT_DIPSETTING(    0x20, "5" )
	PORT_BITX( 0,       0x30, IPT_DIPSWITCH_SETTING | IPF_CHEAT, "70", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )	/* probably unused */
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START /* 1807 */
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Bonus_Life) )
	PORT_DIPSETTING(    0x00, "500000,1000000" )
	PORT_DIPSETTING(    0x01, "none" )
	PORT_DIPNAME( 0x02, 0x02, "Controls" )
	PORT_DIPSETTING(    0x02, "Normal" )
	PORT_DIPSETTING(    0x00, "Reversed" )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )	/* probably unused */
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	PORT_BIT( 0x10, IP_ACTIVE_LOW,  IPT_COIN2   )
	PORT_BIT( 0x20, IP_ACTIVE_LOW,  IPT_COIN1   )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_START1 )
INPUT_PORTS_END

INPUT_PORTS_START( pinbos )
	PORT_START  /* 1804 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP    | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN  | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON2 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START  /* 1805 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON2 | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START /* 1806 */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x0e, 0x0e, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(	0x02, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(	0x0e, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(	0x08, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(	0x04, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(	0x0c, DEF_STR( 1C_6C ) )
/*	PORT_DIPSETTING(	0x06, DEF_STR( 1C_1C ) )*/
/*	PORT_DIPSETTING(	0x00, DEF_STR( 1C_1C ) )*/
/*	PORT_DIPSETTING(	0x0a, DEF_STR( 1C_1C ) )*/
	PORT_DIPNAME( 0x30, 0x00, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x10, "4" )
	PORT_DIPSETTING(    0x20, "5" )
	PORT_BITX( 0,       0x30, IPT_DIPSWITCH_SETTING | IPF_CHEAT, "70", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )	/* probably unused */
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START /* 1807 */
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Bonus_Life) )
	PORT_DIPSETTING(    0x00, "500000,1000000" )
	PORT_DIPSETTING(    0x01, "none" )
	PORT_DIPNAME( 0x02, 0x02, "Controls" )
	PORT_DIPSETTING(    0x02, "Normal" )
	PORT_DIPSETTING(    0x00, "Reversed" )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )	/* probably unused */
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	PORT_BIT( 0x10, IP_ACTIVE_LOW,  IPT_COIN2   )
	PORT_BIT( 0x20, IP_ACTIVE_LOW,  IPT_COIN1   )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_START1 )
INPUT_PORTS_END



static struct GfxLayout lasso_charlayout =
{
	8,8,
	RGN_FRAC(1,4),
	2,
	{ RGN_FRAC(0,4), RGN_FRAC(2,4) },
	{ STEP8(0,1) },
	{ STEP8(0,8) },
	8*8
};

static struct GfxLayout lasso_spritelayout =
{
	16,16,
	RGN_FRAC(1,4),
	2,
	{ RGN_FRAC(1,4), RGN_FRAC(3,4) },
	{ STEP8(0,1), STEP8(8*8*1,1) },
	{ STEP8(0,8), STEP8(8*8*2,8) },
	16*16
};

static struct GfxLayout wwjgtin_tracklayout =
{
	16,16,
	RGN_FRAC(1,4),
	4,
	{ RGN_FRAC(1,4), RGN_FRAC(3,4), RGN_FRAC(0,4), RGN_FRAC(2,4) },
	{ STEP8(0,1), STEP8(8*8*1,1) },
	{ STEP8(0,8), STEP8(8*8*2,8) },
	16*16
};

/* Pinbo is 3bpp, otherwise the same */
static struct GfxLayout pinbo_charlayout =
{
	8,8,
	RGN_FRAC(1,6),
	3,
	{ RGN_FRAC(0,6), RGN_FRAC(2,6), RGN_FRAC(4,6) },
	{ STEP8(0,1) },
	{ STEP8(0,8) },
	8*8
};

static struct GfxLayout pinbo_spritelayout =
{
	16,16,
	RGN_FRAC(1,6),
	3,
	{ RGN_FRAC(1,6), RGN_FRAC(3,6), RGN_FRAC(5,6) },
	{ STEP8(0,1), STEP8(8*8*1,1) },
	{ STEP8(0,8), STEP8(8*8*2,8) },
	16*16
};


static struct GfxDecodeInfo lasso_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &lasso_charlayout,   0, 16 },
	{ REGION_GFX1, 0, &lasso_spritelayout, 0, 16 },
	{ -1 }
};

static struct GfxDecodeInfo wwjgtin_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &lasso_charlayout,       0, 16 },
	{ REGION_GFX1, 0, &lasso_spritelayout,     0, 16 },
	{ REGION_GFX2, 0, &wwjgtin_tracklayout,	4*16, 16 },
	{ -1 }
};

static struct GfxDecodeInfo pinbo_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &pinbo_charlayout,   0, 16 },
	{ REGION_GFX1, 0, &pinbo_spritelayout, 0, 16 },
	{ -1 }
};



static struct SN76496interface sn76496_interface =
{
	2,	/* 2 chips */
	{ 2000000, 2000000 },	/* ? MHz */
	{ 100, 100 }
};

static struct DACinterface dac_interface =
{
	1,
	{ 100 }
};

static struct AY8910interface ay8910_interface =
{
	2,		/* 2 chips */
	1250000,	/* 1.25 MHz? */
	{ 25, 25 },
	{ 0, 0 },
	{ 0, 0 },
	{ 0, 0 },
	{ 0, 0 }
};

static MACHINE_DRIVER_START( lasso )

	/* basic machine hardware */
	MDRV_CPU_ADD_TAG("main", M6502, 2000000)	/* 2 MHz (?) */
	MDRV_CPU_MEMORY(lasso_readmem,lasso_writemem)
	MDRV_CPU_VBLANK_INT(lasso_interrupt,2)		/* IRQ = VBlank, NMI = Coin Insertion */

	MDRV_CPU_ADD_TAG("audio", M6502, 600000)
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)		/* ?? (controls music tempo) */
	MDRV_CPU_MEMORY(lasso_sound_readmem,lasso_sound_writemem)

	MDRV_CPU_ADD_TAG("blitter", M6502, 2000000)	/* 2 MHz (?) */
	MDRV_CPU_MEMORY(lasso_coprocessor_readmem,lasso_coprocessor_writemem)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(100)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(0, 32*8-1, 2*8, 30*8-1)
	MDRV_GFXDECODE(lasso_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(0x40)

	MDRV_PALETTE_INIT(lasso)
	MDRV_VIDEO_START(lasso)
	MDRV_VIDEO_UPDATE(lasso)

	/* sound hardware */
	MDRV_SOUND_ADD_TAG("sn76496", SN76496, sn76496_interface)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( chameleo )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(lasso)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_MEMORY(chameleo_readmem,chameleo_writemem)

	MDRV_CPU_MODIFY("audio")
	MDRV_CPU_MEMORY(chameleo_sound_readmem,chameleo_sound_writemem)

	MDRV_CPU_REMOVE("blitter")

	/* video hardware */
	MDRV_VIDEO_UPDATE(chameleo)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( wwjgtin )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(lasso)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_MEMORY(wwjgtin_readmem,wwjgtin_writemem)

	MDRV_CPU_MODIFY("audio")
	MDRV_CPU_MEMORY(lasso_sound_readmem,wwjgtin_sound_writemem)

	MDRV_CPU_REMOVE("blitter")

	/* video hardware */
	MDRV_VISIBLE_AREA(1*8, 31*8-1, 2*8, 30*8-1)	/* Smaller visible area?*/
	MDRV_GFXDECODE(wwjgtin_gfxdecodeinfo)	/* Has 1 additional layer*/
	MDRV_PALETTE_LENGTH(0x40+1)
	MDRV_COLORTABLE_LENGTH(4*16 + 16*16)	/* Reserve 1 color for black*/

	MDRV_PALETTE_INIT(wwjgtin)
	MDRV_VIDEO_START(wwjgtin)
	MDRV_VIDEO_UPDATE(wwjgtin)

	/* sound hardware */
	MDRV_SOUND_ADD(DAC, dac_interface)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( pinbo )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(lasso)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_MEMORY(chameleo_readmem,pinbo_writemem)

	MDRV_CPU_REPLACE("audio", Z80, 3000000)
	MDRV_CPU_MEMORY(pinbo_sound_readmem,pinbo_sound_writemem)
	MDRV_CPU_PORTS(pinbo_sound_readport,pinbo_sound_writeport)

	MDRV_CPU_REMOVE("blitter")

	/* video hardware */
	MDRV_GFXDECODE(pinbo_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(256)

	MDRV_PALETTE_INIT(RRRR_GGGG_BBBB)
	MDRV_VIDEO_START(pinbo)
	MDRV_VIDEO_UPDATE(chameleo)

	/* sound hardware */
	MDRV_SOUND_REMOVE("sn76496")
	MDRV_SOUND_ADD(AY8910, ay8910_interface)
MACHINE_DRIVER_END


ROM_START( lasso )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "wm3",       0x8000, 0x2000, CRC(f93addd6) SHA1(b0a1b263874da8608c3bab4e8785358e2aa19c2e) )
	ROM_RELOAD(            0xc000, 0x2000)
	ROM_LOAD( "wm4",       0xe000, 0x2000, CRC(77719859) SHA1(d206b6af9a567f70d69624866ae9973652527065) )
	ROM_RELOAD(            0xa000, 0x2000)

	ROM_REGION( 0x10000, REGION_CPU2, 0 )
	ROM_LOAD( "wmc",       0x5000, 0x1000, CRC(8b4eb242) SHA1(55ada50036abbaa10799f37e35254e6ff70ee947) )
	ROM_LOAD( "wmb",       0x6000, 0x1000, CRC(4658bcb9) SHA1(ecc83ef99edbe5f69a884a142478ff0f56edba12) )
	ROM_LOAD( "wma",       0x7000, 0x1000, CRC(2e7de3e9) SHA1(665a89b9914ca16b9c08b751e142cf7320aaf793) )
	ROM_RELOAD(            0xf000, 0x1000 )

	ROM_REGION( 0x10000, REGION_CPU3, 0 ) /* 6502 code (lasso image blitter) */
	ROM_LOAD( "wm5",       0xf000, 0x1000, CRC(7dc3ff07) SHA1(46aaa9186940d06fd679a573330e9ad3796aa647) )
	ROM_RELOAD(            0x8000, 0x1000)

	ROM_REGION( 0x4000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "wm1",       0x0000, 0x0800, CRC(7db77256) SHA1(d12305bdfb6923c32982982a5544ae9bd8dbc2cb) )	/* Tiles   */
	ROM_CONTINUE(          0x1000, 0x0800             )	/* Sprites */
	ROM_CONTINUE(          0x0800, 0x0800             )
	ROM_CONTINUE(          0x1800, 0x0800             )
	ROM_LOAD( "wm2",       0x2000, 0x0800, CRC(9e7d0b6f) SHA1(c82be332209bf7331718e51926004fe9aa6f5ebd) )	/* 2nd bitplane */
	ROM_CONTINUE(          0x3000, 0x0800             )
	ROM_CONTINUE(          0x2800, 0x0800             )
	ROM_CONTINUE(          0x3800, 0x0800             )

	ROM_REGION( 0x40, REGION_PROMS, 0 )
	ROM_LOAD( "82s123.69", 0x0000, 0x0020, CRC(1eabb04d) SHA1(3dc5b407bc1b1dea77337b4e913f1e945386d5c9) )
	ROM_LOAD( "82s123.70", 0x0020, 0x0020, CRC(09060f8c) SHA1(8f14b00bcfb7ab89d2e443cc82f7a65dc96ee819) )
ROM_END

ROM_START( chameleo )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )		/* 6502 Code (Main CPU) */
	ROM_LOAD( "chamel4.bin", 0x4000, 0x2000, CRC(97379c47) SHA1(b29fa2318d4260c29fc95d22a461173dc960ad1a) )
	ROM_LOAD( "chamel5.bin", 0x6000, 0x2000, CRC(0a2cadfd) SHA1(1ccc43accd60ca15b8f03ed1c3fda76a840a2bb1) )
	ROM_LOAD( "chamel6.bin", 0x8000, 0x2000, CRC(b023c354) SHA1(0424ecf81ac9f0e055f9ff01cf0bd6d5c9ff866c) )
	ROM_LOAD( "chamel7.bin", 0xa000, 0x2000, CRC(a5a03375) SHA1(c1eac4596c2bda419f3c513ecd3df9fae49ae159) )
	ROM_RELOAD(              0xe000, 0x2000             )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )		/* 6502 Code (Sound CPU) */
	ROM_LOAD( "chamel3.bin", 0x1000, 0x1000, CRC(52eab9ec) SHA1(554c34134e3af970262da89fe82feeaf47fd30bc) )
	ROM_LOAD( "chamel2.bin", 0x6000, 0x1000, CRC(81dcc49c) SHA1(7e1b4351775f9c140a43f531da8b055271b7b28c) )
	ROM_LOAD( "chamel1.bin", 0x7000, 0x1000, CRC(96031d3b) SHA1(a143b54b98891423d355e0ba08c3b88d70fa0e23) )
	ROM_RELOAD(              0xf000, 0x1000             )

	ROM_REGION( 0x4000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "chamel8.bin", 0x0800, 0x0800, CRC(dc67916b) SHA1(8b3fad0d5d42925b44e51df7f88ea4b6a8dbb4f6) )	/* Tiles   */
	ROM_CONTINUE(            0x1800, 0x0800             )	/* Sprites */
	ROM_CONTINUE(            0x0000, 0x0800             )
	ROM_CONTINUE(            0x1000, 0x0800             )
	ROM_LOAD( "chamel9.bin", 0x2800, 0x0800, CRC(6b559bf1) SHA1(b7b8b8bccbd88ea868e2d3ccb42513615120d8e6) )	/* 2nd bitplane */
	ROM_CONTINUE(            0x3800, 0x0800             )
	ROM_CONTINUE(            0x2000, 0x0800             )
	ROM_CONTINUE(            0x3000, 0x0800             )

	ROM_REGION( 0x40, REGION_PROMS, ROMREGION_DISPOSE )	/* Colors */
	ROM_LOAD( "chambprm.bin", 0x0000, 0x0020, CRC(e3ad76df) SHA1(cd115cece4931bfcfc0f60147b942998a5c21bf7) )
	ROM_LOAD( "chamaprm.bin", 0x0020, 0x0020, CRC(c7063b54) SHA1(53baed3806848207ab3a8fafd182cabec3be4b04) )
ROM_END

ROM_START( wwjgtin )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )		/* 6502 Code (Main CPU) */
	ROM_LOAD( "ic2.6", 0x4000, 0x4000, CRC(744ba45b) SHA1(cccf3e2dd3c27bf54d2abd366cd9a044311aa031) )
	ROM_LOAD( "ic5.5", 0x8000, 0x4000, CRC(af751614) SHA1(fc0f0a3967524b1743a182c1da4f9b0c3097a157) )
	ROM_RELOAD(        0xc000, 0x4000             )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )		/* 6502 Code (Sound CPU) */
	ROM_LOAD( "ic59.9", 0x4000, 0x4000, CRC(2ecb4d98) SHA1(d5b0d447b24f64fca452dc13e6ff95b090fce2d7) )
	ROM_RELOAD(         0xc000, 0x4000             )

	ROM_REGION( 0x8000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "ic81.7", 0x0000, 0x0800, CRC(a27f1a63) SHA1(3c770424bd4996f648687afce4aecea252da83a7) )	/* Tiles   */
	ROM_CONTINUE(       0x2000, 0x0800             )	/* Sprites */
	ROM_CONTINUE(       0x0800, 0x0800             )
	ROM_CONTINUE(       0x2800, 0x0800             )
	ROM_CONTINUE(       0x1000, 0x0800             )
	ROM_CONTINUE(       0x3000, 0x0800             )
	ROM_CONTINUE(       0x1800, 0x0800             )
	ROM_CONTINUE(       0x3800, 0x0800             )
	ROM_LOAD( "ic82.8", 0x4000, 0x0800, CRC(ea2862b3) SHA1(f7604fd324560c54311c35f806a17e30e018032a) )	/* 2nd bitplane */
	ROM_CONTINUE(       0x6000, 0x0800             )	/* Sprites */
	ROM_CONTINUE(       0x4800, 0x0800             )
	ROM_CONTINUE(       0x6800, 0x0800             )
	ROM_CONTINUE(       0x5000, 0x0800             )
	ROM_CONTINUE(       0x7000, 0x0800             )
	ROM_CONTINUE(       0x5800, 0x0800             )
	ROM_CONTINUE(       0x7800, 0x0800             )

	ROM_REGION( 0x4000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "ic47.3", 0x0000, 0x2000, CRC(40594c59) SHA1(94533be8e267d9aa5bcdd52b45f6974436d3fed5) )	/* 1xxxxxxxxxxxx = 0xFF*/
	ROM_LOAD( "ic46.4", 0x2000, 0x2000, CRC(d1921348) SHA1(8b5506ff80a31ce721aed515cad1b4a7e52e47a2) )

	ROM_REGION( 0x4000, REGION_USER1, 0 )				/* tilemap */
	ROM_LOAD( "ic48.2", 0x0000, 0x2000, CRC(a4a7df77) SHA1(476aab702346a402169ab404a8b06589e4932d37) )
	ROM_LOAD( "ic49.1", 0x2000, 0x2000, CRC(e480fbba) SHA1(197c86747ef8477040169f90eb6e04d928aedbe5) )	/* FIXED BITS (1111xxxx)*/

	ROM_REGION( 0x40, REGION_PROMS, ROMREGION_DISPOSE )
	ROM_LOAD( "2.bpr",  0x0000, 0x0020, CRC(79adda5d) SHA1(e54de3eb02f744d49f524cd81e1cf993338916e3) )
	ROM_LOAD( "1.bpr",  0x0020, 0x0020, CRC(c1a93cc8) SHA1(805641ea2ce86589b968f1ff44e5d3ab9377769d) )
ROM_END

ROM_START( pinbo )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "rom2.b7",     0x2000, 0x2000, CRC(9a185338) SHA1(4029cf927686b5e14ef7600b17ea3056cc58b15b) )
	ROM_LOAD( "rom3.e7",     0x6000, 0x2000, CRC(1cd1b3bd) SHA1(388ea72568f5bfd39856d872415327a2afaf7fad) )
	ROM_LOAD( "rom4.h7",     0x8000, 0x2000, CRC(ba043fa7) SHA1(ef3d67b6dab5c82035c58290879a3ca969a0256d) )
	ROM_LOAD( "rom5.j7",     0xa000, 0x2000, CRC(e71046c4) SHA1(f49133544c98df5f3e1a1d2ae92e17261b1504fc) )
	ROM_RELOAD(              0xe000, 0x2000 )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )  /* 64K for sound */
	ROM_LOAD( "rom1.s8",     0x0000, 0x2000, CRC(ca45a1be) SHA1(d0b2d8f1e6d01b60cba83d2bd458a57548549b4b) )

	ROM_REGION( 0xc000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "rom6.a1",     0x0000, 0x0800, CRC(74fe8e98) SHA1(3c9ac38d7054b2831a515786b6f204b1804aaea3) )	/* tiles   */
	ROM_CONTINUE(       	 0x2000, 0x0800             )	/* sprites */
	ROM_CONTINUE(       	 0x0800, 0x0800             )
	ROM_CONTINUE(       	 0x2800, 0x0800             )
	ROM_CONTINUE(       	 0x1000, 0x0800             )
	ROM_CONTINUE(       	 0x3000, 0x0800             )
	ROM_CONTINUE(       	 0x1800, 0x0800             )
	ROM_CONTINUE(       	 0x3800, 0x0800             )
	ROM_LOAD( "rom8.c1",     0x4000, 0x0800, CRC(5a800fe7) SHA1(375269ec73fab7f0cf017a79e002e31b006f5ad7) )	/* 2nd bitplane */
	ROM_CONTINUE(       	 0x6000, 0x0800             )
	ROM_CONTINUE(       	 0x4800, 0x0800             )
	ROM_CONTINUE(       	 0x6800, 0x0800             )
	ROM_CONTINUE(       	 0x5000, 0x0800             )
	ROM_CONTINUE(       	 0x7000, 0x0800             )
	ROM_CONTINUE(       	 0x5800, 0x0800             )
	ROM_CONTINUE(       	 0x7800, 0x0800             )
	ROM_LOAD( "rom7.d1",     0x8000, 0x0800, CRC(327a3c21) SHA1(e938915d28ac4ec033b20d33728788493e3f30f6) )	/* 3rd bitplane */
	ROM_CONTINUE(       	 0xa000, 0x0800             )
	ROM_CONTINUE(       	 0x8800, 0x0800             )
	ROM_CONTINUE(       	 0xa800, 0x0800             )
	ROM_CONTINUE(       	 0x9000, 0x0800             )
	ROM_CONTINUE(       	 0xb000, 0x0800             )
	ROM_CONTINUE(       	 0x9800, 0x0800             )
	ROM_CONTINUE(       	 0xb800, 0x0800             )

	ROM_REGION( 0x00300, REGION_PROMS, 0 ) /* Color PROMs */
	ROM_LOAD( "red.l10",     0x0000, 0x0100, CRC(e6c9ba52) SHA1(6ea96f9bd71de6181d675b0f2d59a8c5e1be5aa3) )
	ROM_LOAD( "green.k10",   0x0100, 0x0100, CRC(1bf2d335) SHA1(dcb074d3de939dfc652743e25bc66bd6fbdc3289) )
	ROM_LOAD( "blue.n10",    0x0200, 0x0100, CRC(e41250ad) SHA1(2e9a2babbacb1753057d46cf1dd6dc183611747e) )
ROM_END

ROM_START( pinbos )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "b4.bin",      0x2000, 0x2000, CRC(d9452d4f) SHA1(c744ee037275b880c0ddc2fd83b3c05eb0a53621) )
	ROM_LOAD( "b5.bin",      0x6000, 0x2000, CRC(f80b204c) SHA1(ee9b4ae1d8ea2fc062022fcfae67df87ed7aff41) )
	ROM_LOAD( "b6.bin",      0x8000, 0x2000, CRC(ae967d83) SHA1(e79db85917a31821d10f919c4c429da33e97894d) )
	ROM_LOAD( "b7.bin",      0xa000, 0x2000, CRC(7a584b4e) SHA1(2eb55b706815228b3b12ee5c0f6c415cd1d612e6) )
	ROM_RELOAD(              0xe000, 0x2000 )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )  /* 64K for sound */
	ROM_LOAD( "b8.bin",      0x0000, 0x2000, CRC(32d1df14) SHA1(c0d4181378bbd6f2c594e923e2f8b21647c7fb0e) )

	ROM_REGION( 0xc000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "rom6.a1",     0x0000, 0x0800, CRC(74fe8e98) SHA1(3c9ac38d7054b2831a515786b6f204b1804aaea3) )	/* tiles   */
	ROM_CONTINUE(       	 0x2000, 0x0800             )	/* sprites */
	ROM_CONTINUE(       	 0x0800, 0x0800             )
	ROM_CONTINUE(       	 0x2800, 0x0800             )
	ROM_CONTINUE(       	 0x1000, 0x0800             )
	ROM_CONTINUE(       	 0x3000, 0x0800             )
	ROM_CONTINUE(       	 0x1800, 0x0800             )
	ROM_CONTINUE(       	 0x3800, 0x0800             )
	ROM_LOAD( "rom8.c1",     0x4000, 0x0800, CRC(5a800fe7) SHA1(375269ec73fab7f0cf017a79e002e31b006f5ad7) )	/* 2nd bitplane */
	ROM_CONTINUE(       	 0x6000, 0x0800             )
	ROM_CONTINUE(       	 0x4800, 0x0800             )
	ROM_CONTINUE(       	 0x6800, 0x0800             )
	ROM_CONTINUE(       	 0x5000, 0x0800             )
	ROM_CONTINUE(       	 0x7000, 0x0800             )
	ROM_CONTINUE(       	 0x5800, 0x0800             )
	ROM_CONTINUE(       	 0x7800, 0x0800             )
	ROM_LOAD( "rom7.d1",     0x8000, 0x0800, CRC(327a3c21) SHA1(e938915d28ac4ec033b20d33728788493e3f30f6) )	/* 3rd bitplane */
	ROM_CONTINUE(       	 0xa000, 0x0800             )
	ROM_CONTINUE(       	 0x8800, 0x0800             )
	ROM_CONTINUE(       	 0xa800, 0x0800             )
	ROM_CONTINUE(       	 0x9000, 0x0800             )
	ROM_CONTINUE(       	 0xb000, 0x0800             )
	ROM_CONTINUE(       	 0x9800, 0x0800             )
	ROM_CONTINUE(       	 0xb800, 0x0800             )

	ROM_REGION( 0x00300, REGION_PROMS, 0 ) /* Color PROMs */
	ROM_LOAD( "red.l10",     0x0000, 0x0100, CRC(e6c9ba52) SHA1(6ea96f9bd71de6181d675b0f2d59a8c5e1be5aa3) )
	ROM_LOAD( "green.k10",   0x0100, 0x0100, CRC(1bf2d335) SHA1(dcb074d3de939dfc652743e25bc66bd6fbdc3289) )
	ROM_LOAD( "blue.n10",    0x0200, 0x0100, CRC(e41250ad) SHA1(2e9a2babbacb1753057d46cf1dd6dc183611747e) )
ROM_END


/***************************************************************************

								Game Drivers

***************************************************************************/

GAME( 1982, lasso,    0,     lasso,    lasso,    0, ROT90, "SNK", "Lasso"                   )
GAME( 1983, chameleo, 0,     chameleo, chameleo, 0, ROT0,  "Jaleco", "Chameleon"               )
GAME( 1984, wwjgtin,  0,     wwjgtin,  wwjgtin,  0, ROT0,  "Jaleco / Casio", "Wai Wai Jockey Gate-In!" )
GAME( 1984, pinbo,    0,     pinbo,    pinbo,    0, ROT90, "Jaleco", "Pinbo" )
GAME( 1984, pinbos,   pinbo, pinbo,    pinbos,   0, ROT90, "bootleg?", "Pinbo (Strike)" )
