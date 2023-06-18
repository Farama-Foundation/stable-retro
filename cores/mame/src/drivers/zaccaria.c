/***************************************************************************

Jack Rabbit memory map (preliminary)

driver by Nicola Salmoria
thanks to Andrea Babich for the manual.

TODO:
- correctly hook up TMS5200 (there's a kludge in zaccaria_ca2_r to make it work)

- there seems to be a strange kind of DAC connected to 8910 #0 port A, but it sounds
  horrible so I'm leaving its volume at 0.

- The 8910 outputs go through some analog circuitry to make them sound more like
  real intruments.
  #0 Ch. A = "rullante"/"cassa" (drum roll/bass drum) (selected by bits 3&4 of port A)
  #0 Ch. B = "basso" (bass)
  #0 Ch. C = straight out through an optional filter
  #1 Ch. A = "piano"
  #1 Ch. B = "tromba" (trumpet) (level selected by bit 0 of port A)
  #1 Ch. C = disabled (there's an open jumper, otherwise would go out through a filter)

- some minor color issues (see vidhrdw)


Notes:
- There is a protection device which I haven't located on the schematics. It
  sits on bits 4-7 of the data bus, and is read from locations where only bits
  0-3 are connected to regular devices (6400-6407 has 4-bit RAM, while 6c00-6c07
  has a 4-bit input port).

- The 6802 driving the TMS5220 has a push button connected to the NMI line. Test?

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "machine/6821pia.h"
#include "machine/8255ppi.h"


extern data8_t *zaccaria_videoram,*zaccaria_attributesram;

PALETTE_INIT( zaccaria );
VIDEO_START( zaccaria );
WRITE_HANDLER( zaccaria_videoram_w );
WRITE_HANDLER( zaccaria_attributes_w );
WRITE_HANDLER( zaccaria_flip_screen_x_w );
WRITE_HANDLER( zaccaria_flip_screen_y_w );
VIDEO_UPDATE( zaccaria );


static int dsw;

static WRITE_HANDLER( zaccaria_dsw_sel_w )
{
	switch (data & 0xf0)
	{
		case 0xe0:
			dsw = 0;
			break;

		case 0xd0:
			dsw = 1;
			break;

		case 0xb0:
			dsw = 2;
			break;

		default:
log_cb(RETRO_LOG_DEBUG, LOGPRE "PC %04x: portsel = %02x\n",activecpu_get_pc(),data);
			break;
	}
}

static READ_HANDLER( zaccaria_dsw_r )
{
	return readinputport(dsw);
}



static WRITE_HANDLER( ay8910_port0a_w )
{
	/* bits 0-2 go to a weird kind of DAC ??*/
	/* bits 3-4 control the analog drum emulation on 8910 #0 ch. A*/

	if (data & 1)	/* DAC enable */
	{
		/* TODO: is this right? it sound awful */
		static int table[4] = { 0x05, 0x1b, 0x0b, 0x55 };
		DAC_signed_data_w(0,table[(data & 0x06) >> 1]);
	}
	else
		DAC_signed_data_w(0,0x80);
}


void zaccaria_irq0a(int state) { cpu_set_nmi_line(1,  state ? ASSERT_LINE : CLEAR_LINE); }
void zaccaria_irq0b(int state) { cpu_set_irq_line(1,0,state ? ASSERT_LINE : CLEAR_LINE); }

static int active_8910,port0a,acs;

static READ_HANDLER( zaccaria_port0a_r )
{
	if (active_8910 == 0)
		return AY8910_read_port_0_r(0);
	else
		return AY8910_read_port_1_r(0);
}

static WRITE_HANDLER( zaccaria_port0a_w )
{
	port0a = data;
}

static WRITE_HANDLER( zaccaria_port0b_w )
{
	static int last;


	/* bit 1 goes to 8910 #0 BDIR pin  */
	if ((last & 0x02) == 0x02 && (data & 0x02) == 0x00)
	{
		/* bit 0 goes to the 8910 #0 BC1 pin */
		if (last & 0x01)
			AY8910_control_port_0_w(0,port0a);
		else
			AY8910_write_port_0_w(0,port0a);
	}
	else if ((last & 0x02) == 0x00 && (data & 0x02) == 0x02)
	{
		/* bit 0 goes to the 8910 #0 BC1 pin */
		if (last & 0x01)
			active_8910 = 0;
	}
	/* bit 3 goes to 8910 #1 BDIR pin  */
	if ((last & 0x08) == 0x08 && (data & 0x08) == 0x00)
	{
		/* bit 2 goes to the 8910 #1 BC1 pin */
		if (last & 0x04)
			AY8910_control_port_1_w(0,port0a);
		else
			AY8910_write_port_1_w(0,port0a);
	}
	else if ((last & 0x08) == 0x00 && (data & 0x08) == 0x08)
	{
		/* bit 2 goes to the 8910 #1 BC1 pin */
		if (last & 0x04)
			active_8910 = 1;
	}

	last = data;
}

static INTERRUPT_GEN( zaccaria_cb1_toggle )
{
	static int toggle;

	pia_0_cb1_w(0,toggle & 1);
	toggle ^= 1;
}



static int port1a,port1b;

static READ_HANDLER( zaccaria_port1a_r )
{
	if (~port1b & 1) return tms5220_status_r(0);
	else return port1a;
}

static WRITE_HANDLER( zaccaria_port1a_w )
{
	port1a = data;
}

static WRITE_HANDLER( zaccaria_port1b_w )
{
	port1b = data;

	/* bit 0 = /RS*/

	/* bit 1 = /WS*/
	if (~data & 2) tms5220_data_w(0,port1a);

	/* bit 3 = "ACS" (goes, inverted, to input port 6 bit 3)*/
	acs = ~data & 0x08;

	/* bit 4 = led (for testing?)*/
	set_led_status(0,~data & 0x10);
}

static READ_HANDLER( zaccaria_ca2_r )
{
/* TODO: this doesn't work, why?*/
/*	return !tms5220_ready_r();*/

static int counter;
counter = (counter+1) & 0x0f;

return counter;

}

static void tms5220_irq_handler(int state)
{
	pia_1_cb1_w(0,state ? 0 : 1);
}



static struct pia6821_interface pia_0_intf =
{
	/*inputs : A/B,CA/B1,CA/B2 */ zaccaria_port0a_r, 0, 0, 0, 0, 0,
	/*outputs: A/B,CA/B2       */ zaccaria_port0a_w, zaccaria_port0b_w, 0, 0,
	/*irqs   : A/B             */ zaccaria_irq0a, zaccaria_irq0b
};

static struct pia6821_interface pia_1_intf =
{
	/*inputs : A/B,CA/B1,CA/B2 */ zaccaria_port1a_r, 0, 0, 0, zaccaria_ca2_r, 0,
	/*outputs: A/B,CA/B2       */ zaccaria_port1a_w, zaccaria_port1b_w, 0, 0,
	/*irqs   : A/B             */ 0, 0
};


static ppi8255_interface ppi8255_intf =
{
	1, 								/* 1 chip */
	{input_port_3_r},				/* Port A read */
	{input_port_4_r},				/* Port B read */
	{input_port_5_r},				/* Port C read */
	{0},							/* Port A write */
	{0},							/* Port B write */
	{zaccaria_dsw_sel_w}, 			/* Port C write */
};


static MACHINE_INIT( zaccaria )
{
	ppi8255_init(&ppi8255_intf);

	pia_unconfig();
	pia_config(0, PIA_STANDARD_ORDERING, &pia_0_intf);
	pia_config(1, PIA_STANDARD_ORDERING, &pia_1_intf);
	pia_reset();
}


static WRITE_HANDLER( sound_command_w )
{
	soundlatch_w(0,data);
	cpu_set_irq_line(2,0,(data & 0x80) ? CLEAR_LINE : ASSERT_LINE);
}

static WRITE_HANDLER( sound1_command_w )
{
	pia_0_ca1_w(0,data & 0x80);
	soundlatch2_w(0,data);
}

static WRITE_HANDLER( mc1408_data_w )
{
	DAC_data_w(1,data);
}


struct GameDriver monymony_driver;

static READ_HANDLER( zaccaria_prot1_r )
{
	switch (offset)
	{
		case 0:
			return 0x50;    /* Money Money */

		case 4:
			return 0x40;    /* Jack Rabbit */

		case 6:
			if (Machine->gamedrv == &monymony_driver)
				return 0x70;    /* Money Money */
			return 0xa0;    /* Jack Rabbit */

		default:
			return 0;
	}
}

static READ_HANDLER( zaccaria_prot2_r )
{
	switch (offset)
	{
		case 0:
			return (input_port_6_r(0) & 0x07) | (acs & 0x08);   /* bits 4 and 5 must be 0 in Jack Rabbit */

		case 2:
			return 0x10;    /* Jack Rabbit */

		case 4:
			return 0x80;    /* Money Money */

		case 6:
			return 0x00;    /* Money Money */

		default:
			return 0;
	}
}


static WRITE_HANDLER( coin_w )
{
	coin_counter_w(0,data & 1);
}

static WRITE_HANDLER( nmienable_w )
{
	interrupt_enable_w(0,data & 1);
}



static MEMORY_READ_START( readmem )
	{ 0x0000, 0x5fff, MRA_ROM },
	{ 0x6000, 0x63ff, MRA_RAM },
	{ 0x6400, 0x6407, zaccaria_prot1_r },
	{ 0x6c00, 0x6c07, zaccaria_prot2_r },
	{ 0x6e00, 0x6e00, zaccaria_dsw_r },
	{ 0x7000, 0x77ff, MRA_RAM },
	{ 0x7800, 0x7803, ppi8255_0_r },
	{ 0x7c00, 0x7c00, watchdog_reset_r },
	{ 0x8000, 0xdfff, MRA_ROM },
MEMORY_END

static MEMORY_WRITE_START( writemem )
	{ 0x0000, 0x5fff, MWA_ROM },
	{ 0x6000, 0x67ff, zaccaria_videoram_w, &zaccaria_videoram },	/* 6400-67ff is 4 bits wide */
	{ 0x6800, 0x683f, zaccaria_attributes_w, &zaccaria_attributesram },
	{ 0x6840, 0x685f, MWA_RAM, &spriteram, &spriteram_size },
	{ 0x6881, 0x68bc, MWA_RAM, &spriteram_2, &spriteram_2_size },
	{ 0x6c00, 0x6c00, zaccaria_flip_screen_x_w },
	{ 0x6c01, 0x6c01, zaccaria_flip_screen_y_w },
	{ 0x6c02, 0x6c02, MWA_NOP },    /* sound reset */
	{ 0x6e00, 0x6e00, sound_command_w },
	{ 0x6c06, 0x6c06, coin_w },
	{ 0x6c07, 0x6c07, nmienable_w },
	{ 0x7000, 0x77ff, MWA_RAM },
	{ 0x7800, 0x7803, ppi8255_0_w },
	{ 0x8000, 0xdfff, MWA_ROM },
MEMORY_END

static MEMORY_READ_START( sound_readmem1 )
	{ 0x0000, 0x007f, MRA_RAM },
	{ 0x500c, 0x500f, pia_0_r },
	{ 0xa000, 0xbfff, MRA_ROM },
	{ 0xe000, 0xffff, MRA_ROM },
MEMORY_END

static MEMORY_WRITE_START( sound_writemem1 )
	{ 0x0000, 0x007f, MWA_RAM },
	{ 0x500c, 0x500f, pia_0_w },
	{ 0xa000, 0xbfff, MWA_ROM },
	{ 0xe000, 0xffff, MWA_ROM },
MEMORY_END

static MEMORY_READ_START( sound_readmem2 )
	{ 0x0000, 0x007f, MRA_RAM },
	{ 0x0090, 0x0093, pia_1_r },
	{ 0x1800, 0x1800, soundlatch_r },
	{ 0xa000, 0xbfff, MRA_ROM },
	{ 0xe000, 0xffff, MRA_ROM },
MEMORY_END

static MEMORY_WRITE_START( sound_writemem2 )
	{ 0x0000, 0x007f, MWA_RAM },
	{ 0x0090, 0x0093, pia_1_w },
	{ 0x1000, 0x1000, mc1408_data_w },	/* MC1408 */
	{ 0x1400, 0x1400, sound1_command_w },
	{ 0xa000, 0xbfff, MWA_ROM },
	{ 0xe000, 0xffff, MWA_ROM },
MEMORY_END



INPUT_PORTS_START( monymony )
	PORT_START
	PORT_DIPNAME( 0x03, 0x01, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "2" )
	PORT_DIPSETTING(    0x01, "3" )
	PORT_DIPSETTING(    0x02, "4" )
	PORT_DIPSETTING(    0x03, "5" )
	PORT_BITX(    0x04, 0x00, IPT_DIPSWITCH_NAME | IPF_CHEAT, "Infinite Lives", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x04, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x00, "Easy" )
	PORT_DIPSETTING(    0x08, "Hard" )
	PORT_DIPNAME( 0x10, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x20, 0x00, "Freeze" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x20, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x00, "Cross Hatch Pattern" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Unknown ) )  /* random high scores? */
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START
	PORT_DIPNAME( 0x03, 0x01, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x01, "200000" )
	PORT_DIPSETTING(    0x02, "300000" )
	PORT_DIPSETTING(    0x03, "400000" )
	PORT_DIPSETTING(    0x00, "None" )
	PORT_DIPNAME( 0x04, 0x00, "Table Title" )
	PORT_DIPSETTING(    0x00, "Todays High Scores" )
	PORT_DIPSETTING(    0x04, "High Scores" )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x00, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x00, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_SERVICE( 0x80, IP_ACTIVE_HIGH )

	PORT_START
	PORT_DIPNAME( 0x03, 0x02, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_3C ) )
	PORT_DIPNAME( 0x8c, 0x84, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x8c, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x88, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x84, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_6C ) )
	PORT_DIPNAME( 0x70, 0x50, "Coin C" )
	PORT_DIPSETTING(    0x70, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x60, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x50, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_7C ) )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_TILT )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	/* other bits are outputs */

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_SPECIAL )	/* "ACS" - from pin 13 of a PIA on the sound board */
	/* other bits come from a protection device */
INPUT_PORTS_END

INPUT_PORTS_START( jackrabt )
	PORT_START
	PORT_DIPNAME( 0x03, 0x01, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "2" )
	PORT_DIPSETTING(    0x01, "3" )
	PORT_DIPSETTING(    0x02, "4" )
	PORT_DIPSETTING(    0x03, "5" )
	PORT_BITX(    0x04, 0x00, IPT_DIPSWITCH_NAME | IPF_CHEAT, "Infinite Lives", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x04, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x20, 0x00, "Freeze" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x20, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x00, "Cross Hatch Pattern" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x00, "Table Title" )
	PORT_DIPSETTING(    0x00, "Todays High Scores" )
	PORT_DIPSETTING(    0x04, "High Scores" )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_SERVICE( 0x80, IP_ACTIVE_HIGH )

	PORT_START
	PORT_DIPNAME( 0x03, 0x02, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_3C ) )
	PORT_DIPNAME( 0x8c, 0x84, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x8c, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x88, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x84, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_6C ) )
	PORT_DIPNAME( 0x70, 0x50, "Coin C" )
	PORT_DIPSETTING(    0x70, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x60, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x50, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_7C ) )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_TILT )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	/* other bits are outputs */

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_SPECIAL )	/* "ACS" - from pin 13 of a PIA on the sound board */
	/* other bits come from a protection device */
INPUT_PORTS_END



static struct GfxLayout charlayout =
{
	8,8,
	RGN_FRAC(1,3),
	3,
	{ RGN_FRAC(2,3), RGN_FRAC(1,3), RGN_FRAC(0,3) },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8
};

static struct GfxLayout spritelayout =
{
	16,16,
	RGN_FRAC(1,3),
	3,
	{ RGN_FRAC(2,3), RGN_FRAC(1,3), RGN_FRAC(0,3) },
	{ 0, 1, 2, 3, 4, 5, 6, 7,
			8*8+0, 8*8+1, 8*8+2, 8*8+3, 8*8+4, 8*8+5, 8*8+6, 8*8+7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
			16*8, 17*8, 18*8, 19*8, 20*8, 21*8, 22*8, 23*8 },
	32*8
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &charlayout,      0, 32 },
	{ REGION_GFX1, 0, &spritelayout, 32*8, 32 },
	{ -1 } /* end of array */
};


struct AY8910interface ay8910_interface =
{
	2,	/* 2 chips */
	3580000/2,
	{ 15, 15 },
	{ 0, 0 },
	{ soundlatch2_r, 0 },
	{ ay8910_port0a_w, 0 },
	{ 0, 0 }
};

static struct DACinterface dac_interface =
{
	2,
	{ 0,80 }	/* I'm leaving the first DAC(?) off because it sounds awful */
};

static struct TMS5220interface tms5220_interface =
{
	640000,				/* clock speed (80*samplerate) */
	80,					/* volume */
	tms5220_irq_handler	/* IRQ handler */
};



static MACHINE_DRIVER_START( zaccaria )

	/* basic machine hardware */
	MDRV_CPU_ADD(Z80,18432000/6)	/* 3.072 MHz */
	MDRV_CPU_MEMORY(readmem,writemem)
	MDRV_CPU_VBLANK_INT(nmi_line_pulse,1)

	MDRV_CPU_ADD(M6802,3580000/4)
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)	/* 895 kHz */
	MDRV_CPU_MEMORY(sound_readmem1,sound_writemem1)
	MDRV_CPU_PERIODIC_INT(zaccaria_cb1_toggle,3580000/4096)

	MDRV_CPU_ADD(M6802,3580000/4)
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)	/* 895 kHz */
	MDRV_CPU_MEMORY(sound_readmem2,sound_writemem2)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_MACHINE_INIT(zaccaria)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 2*8, 30*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(512)
	MDRV_COLORTABLE_LENGTH(32*8+32*8)

	MDRV_PALETTE_INIT(zaccaria)
	MDRV_VIDEO_START(zaccaria)
	MDRV_VIDEO_UPDATE(zaccaria)

	/* sound hardware */
	MDRV_SOUND_ADD(AY8910, ay8910_interface)
	MDRV_SOUND_ADD(DAC, dac_interface)
	MDRV_SOUND_ADD(TMS5220, tms5220_interface)
MACHINE_DRIVER_END



/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( monymony )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )     /* 64k for code */
	ROM_LOAD( "1a",           0x0000, 0x1000, CRC(13c227ca) SHA1(be305d112917904dd130b08f6b5186e3fbcb858a) )
	ROM_CONTINUE(             0x8000, 0x1000 )
	ROM_LOAD( "1b",           0x1000, 0x1000, CRC(87372545) SHA1(04618d007a93b3f6706f56b10bdf39727d7d748d) )
	ROM_CONTINUE(             0x9000, 0x1000 )
	ROM_LOAD( "1c",           0x2000, 0x1000, CRC(6aea9c01) SHA1(36a57f4dfae52d674dcf55d2b93dbacf734866b1) )
	ROM_CONTINUE(             0xa000, 0x1000 )
	ROM_LOAD( "1d",           0x3000, 0x1000, CRC(5fdec451) SHA1(0f955c907e0a61a725a951018fdf5cc321139863) )
	ROM_CONTINUE(             0xb000, 0x1000 )
	ROM_LOAD( "2a",           0x4000, 0x1000, CRC(af830e3c) SHA1(bed57c341ae3500f147efe31bcf01f81466ec1c0) )
	ROM_CONTINUE(             0xc000, 0x1000 )
	ROM_LOAD( "2c",           0x5000, 0x1000, CRC(31da62b1) SHA1(486f07087244f8537510afacb64ddd59eb512a4d) )
	ROM_CONTINUE(             0xd000, 0x1000 )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* 64k for first 6802 */
	ROM_LOAD( "2g",           0xa000, 0x2000, CRC(78b01b98) SHA1(2aabed56cdae9463deb513c0c5021f6c8dfd271e) )
	ROM_LOAD( "1i",           0xe000, 0x2000, CRC(94e3858b) SHA1(04961f67b95798b530bd83355dec612389f22255) )

	ROM_REGION( 0x10000, REGION_CPU3, 0 ) /* 64k for second 6802 */
	ROM_LOAD( "1h",           0xa000, 0x1000, CRC(aad76193) SHA1(e08fc184efced392ee902c4cc9daaaf3310cdfe2) )
	ROM_CONTINUE(             0xe000, 0x1000 )
	ROM_LOAD( "1g",           0xb000, 0x1000, CRC(1e8ffe3e) SHA1(858ee7abe88d5801237e519cae2b50ae4bf33a58) )
	ROM_CONTINUE(             0xf000, 0x1000 )

	ROM_REGION( 0x6000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "2d",           0x0000, 0x2000, CRC(82ab4d1a) SHA1(5aaf42a508df236f2e7c844d377132d73053907b) )
	ROM_LOAD( "1f",           0x2000, 0x2000, CRC(40d4e4d1) SHA1(79cbade30f1c9269e70ddb9c4332cfe1e8dc50a9) )
	ROM_LOAD( "1e",           0x4000, 0x2000, CRC(36980455) SHA1(4140b0cd4137c8f209124b12d9c0eb3b04f91991) )

	ROM_REGION( 0x0400, REGION_PROMS, 0 )
	ROM_LOAD( "monymony.9g",  0x0000, 0x0200, CRC(fc9a0f21) SHA1(2a93d684645ee1b70315386127223151582ab370) )
	ROM_LOAD( "monymony.9f",  0x0200, 0x0200, CRC(93106704) SHA1(d3b8281c87d253a2ed40ff400438e879ca40c2b7) )
ROM_END

ROM_START( jackrabt )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )     /* 64k for code */
	ROM_LOAD( "cpu-01.1a",    0x0000, 0x1000, CRC(499efe97) SHA1(f0efc910a5343001b27637779e1d4de218d44a4e) )
	ROM_CONTINUE(             0x8000, 0x1000 )
	ROM_LOAD( "cpu-01.2l",    0x1000, 0x1000, CRC(4772e557) SHA1(71c1eb49c978799294e732e65a77eba330d8da9b) )
	ROM_LOAD( "cpu-01.3l",    0x2000, 0x1000, CRC(1e844228) SHA1(0525fe95a0f90c50b54c0bf618eb083ccf20e6c4) )
	ROM_LOAD( "cpu-01.4l",    0x3000, 0x1000, CRC(ebffcc38) SHA1(abaf0e96d92f9c828a95446af6d5301053416f3d) )
	ROM_LOAD( "cpu-01.5l",    0x4000, 0x1000, CRC(275e0ed6) SHA1(c0789007a4de1aa848b7e5d26cf9fe847cc5d8a4) )
	ROM_LOAD( "cpu-01.6l",    0x5000, 0x1000, CRC(8a20977a) SHA1(ba15f4c62f600372390e56c2067b4a8ab1f2dba9) )
	ROM_LOAD( "cpu-01.2h",    0x9000, 0x1000, CRC(21f2be2a) SHA1(7d10489ca7325eebfa309ae4ffd4962a4310c403) )
	ROM_LOAD( "cpu-01.3h",    0xa000, 0x1000, CRC(59077027) SHA1(d6c2e68b4b2f1dce8a2141ec259812e732c1c69c) )
	ROM_LOAD( "cpu-01.4h",    0xb000, 0x1000, CRC(0b9db007) SHA1(836f8cacf2a097fd80d5c045bdc49b3a3174b89e) )
	ROM_LOAD( "cpu-01.5h",    0xc000, 0x1000, CRC(785e1a01) SHA1(a748d300be9455cad4f912e01c2279bb8465edfe) )
	ROM_LOAD( "cpu-01.6h",    0xd000, 0x1000, CRC(dd5979cf) SHA1(e9afe7002b2258a1c3132bdd951c6e20d473fb6a) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* 64k for first 6802 */
	ROM_LOAD( "13snd.2g",     0xa000, 0x2000, CRC(fc05654e) SHA1(ed9c66672fe89c41e320e1d27b53f5efa92dce9c) )
	ROM_LOAD( "9snd.1i",      0xe000, 0x2000, CRC(3dab977f) SHA1(3e79c06d2e70b050f01b7ac58be5127ba87904b0) )

	ROM_REGION( 0x10000, REGION_CPU3, 0 ) /* 64k for second 6802 */
	ROM_LOAD( "8snd.1h",      0xa000, 0x1000, CRC(f4507111) SHA1(0513f0831b94aeda84aa4f3b4a7c60dfc5113b2d) )
	ROM_CONTINUE(             0xe000, 0x1000 )
	ROM_LOAD( "7snd.1g",      0xb000, 0x1000, CRC(c722eff8) SHA1(d8d1c091ab80ea2d6616e4dc030adc9905c0a496) )
	ROM_CONTINUE(             0xf000, 0x1000 )

	ROM_REGION( 0x6000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "1bg.2d",       0x0000, 0x2000, CRC(9f880ef5) SHA1(0ee20fb7c794f6dafdaf2c9ee8456221c9d668c5) )
	ROM_LOAD( "2bg.1f",       0x2000, 0x2000, CRC(afc04cd7) SHA1(f4349e86b9caee71c9bf9faf68b86603417d9a2b) )
	ROM_LOAD( "3bg.1e",       0x4000, 0x2000, CRC(14f23cdd) SHA1(e5f3dac52288c56f2fd2940b397bb6c896131a26) )

	ROM_REGION( 0x0400, REGION_PROMS, 0 )
	ROM_LOAD( "jr-ic9g",      0x0000, 0x0200, CRC(85577107) SHA1(76575fa68b66130b18dfe7374d1a03740963cc73) )
	ROM_LOAD( "jr-ic9f",      0x0200, 0x0200, CRC(085914d1) SHA1(3d6f9318f5a9f08ce89e4184e3efb9881f671fa7) )
ROM_END

ROM_START( jackrab2 )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )     /* 64k for code */
	ROM_LOAD( "1cpu2.1a",     0x0000, 0x1000, CRC(f9374113) SHA1(521f293f1894bcaf21e44bc7841a20ae29232da3) )
	ROM_CONTINUE(             0x8000, 0x1000 )
	ROM_LOAD( "2cpu2.1b",     0x1000, 0x1000, CRC(0a0eea4a) SHA1(4dfd9b2511d480bb5cc918f7d91013205911d377) )
	ROM_CONTINUE(             0x9000, 0x1000 )
	ROM_LOAD( "3cpu2.1c",     0x2000, 0x1000, CRC(291f5772) SHA1(958c2601d43de3c95ed5e3d79737199703263a6a) )
	ROM_CONTINUE(             0xa000, 0x1000 )
	ROM_LOAD( "4cpu2.1d",     0x3000, 0x1000, CRC(10972cfb) SHA1(30dd473b3416ee37f887d930ba0017b5b694398e) )
	ROM_CONTINUE(             0xb000, 0x1000 )
	ROM_LOAD( "5cpu2.2a",     0x4000, 0x1000, CRC(aa95d06d) SHA1(2216effe6cacd02a5320e71a85842087dda5f85a) )
	ROM_CONTINUE(             0xc000, 0x1000 )
	ROM_LOAD( "6cpu2.2c",     0x5000, 0x1000, CRC(404496eb) SHA1(44381e27e540fe9d8cacab4c3b1fe9a4f20d26a8) )
	ROM_CONTINUE(             0xd000, 0x1000 )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* 64k for first 6802 */
	ROM_LOAD( "13snd.2g",     0xa000, 0x2000, CRC(fc05654e) SHA1(ed9c66672fe89c41e320e1d27b53f5efa92dce9c) )
	ROM_LOAD( "9snd.1i",      0xe000, 0x2000, CRC(3dab977f) SHA1(3e79c06d2e70b050f01b7ac58be5127ba87904b0) )

	ROM_REGION( 0x10000, REGION_CPU3, 0 ) /* 64k for second 6802 */
	ROM_LOAD( "8snd.1h",      0xa000, 0x1000, CRC(f4507111) SHA1(0513f0831b94aeda84aa4f3b4a7c60dfc5113b2d) )
	ROM_CONTINUE(             0xe000, 0x1000 )
	ROM_LOAD( "7snd.1g",      0xb000, 0x1000, CRC(c722eff8) SHA1(d8d1c091ab80ea2d6616e4dc030adc9905c0a496) )
	ROM_CONTINUE(             0xf000, 0x1000 )

	ROM_REGION( 0x6000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "1bg.2d",       0x0000, 0x2000, CRC(9f880ef5) SHA1(0ee20fb7c794f6dafdaf2c9ee8456221c9d668c5) )
	ROM_LOAD( "2bg.1f",       0x2000, 0x2000, CRC(afc04cd7) SHA1(f4349e86b9caee71c9bf9faf68b86603417d9a2b) )
	ROM_LOAD( "3bg.1e",       0x4000, 0x2000, CRC(14f23cdd) SHA1(e5f3dac52288c56f2fd2940b397bb6c896131a26) )

	ROM_REGION( 0x0400, REGION_PROMS, 0 )
	ROM_LOAD( "jr-ic9g",      0x0000, 0x0200, CRC(85577107) SHA1(76575fa68b66130b18dfe7374d1a03740963cc73) )
	ROM_LOAD( "jr-ic9f",      0x0200, 0x0200, CRC(085914d1) SHA1(3d6f9318f5a9f08ce89e4184e3efb9881f671fa7) )
ROM_END

ROM_START( jackrabs )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )     /* 64k for code */
	ROM_LOAD( "1cpu.1a",      0x0000, 0x1000, CRC(6698dc65) SHA1(33e3518846e88dc34f4b6c4e9ca9f8999c0460c8) )
	ROM_CONTINUE(             0x8000, 0x1000 )
	ROM_LOAD( "2cpu.1b",      0x1000, 0x1000, CRC(42b32929) SHA1(5b400d434ce903c74f58780a422a8c2594af90be) )
	ROM_CONTINUE(             0x9000, 0x1000 )
	ROM_LOAD( "3cpu.1c",      0x2000, 0x1000, CRC(89b50c9a) SHA1(5ab56247de013b5196c1c5765ead4361a5df53e0) )
	ROM_CONTINUE(             0xa000, 0x1000 )
	ROM_LOAD( "4cpu.1d",      0x3000, 0x1000, CRC(d5520665) SHA1(69b34d87d50e6d6e8d365ba0479405380ba3cf11) )
	ROM_CONTINUE(             0xb000, 0x1000 )
	ROM_LOAD( "5cpu.2a",      0x4000, 0x1000, CRC(0f9a093c) SHA1(7fba0d2b8d5d4d1597decec96ed93b997c721d99) )
	ROM_CONTINUE(             0xc000, 0x1000 )
	ROM_LOAD( "6cpu.2c",      0x5000, 0x1000, CRC(f53d6356) SHA1(9b167edca59cf81a2468368a372bab132f15e2ea) )
	ROM_CONTINUE(             0xd000, 0x1000 )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* 64k for first 6802 */
	ROM_LOAD( "13snd.2g",     0xa000, 0x2000, CRC(fc05654e) SHA1(ed9c66672fe89c41e320e1d27b53f5efa92dce9c) )
	ROM_LOAD( "9snd.1i",      0xe000, 0x2000, CRC(3dab977f) SHA1(3e79c06d2e70b050f01b7ac58be5127ba87904b0) )

	ROM_REGION( 0x10000, REGION_CPU3, 0 ) /* 64k for second 6802 */
	ROM_LOAD( "8snd.1h",      0xa000, 0x1000, CRC(f4507111) SHA1(0513f0831b94aeda84aa4f3b4a7c60dfc5113b2d) )
	ROM_CONTINUE(             0xe000, 0x1000 )
	ROM_LOAD( "7snd.1g",      0xb000, 0x1000, CRC(c722eff8) SHA1(d8d1c091ab80ea2d6616e4dc030adc9905c0a496) )
	ROM_CONTINUE(             0xf000, 0x1000 )

	ROM_REGION( 0x6000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "1bg.2d",       0x0000, 0x2000, CRC(9f880ef5) SHA1(0ee20fb7c794f6dafdaf2c9ee8456221c9d668c5) )
	ROM_LOAD( "2bg.1f",       0x2000, 0x2000, CRC(afc04cd7) SHA1(f4349e86b9caee71c9bf9faf68b86603417d9a2b) )
	ROM_LOAD( "3bg.1e",       0x4000, 0x2000, CRC(14f23cdd) SHA1(e5f3dac52288c56f2fd2940b397bb6c896131a26) )

	ROM_REGION( 0x0400, REGION_PROMS, 0 )
	ROM_LOAD( "jr-ic9g",      0x0000, 0x0200, CRC(85577107) SHA1(76575fa68b66130b18dfe7374d1a03740963cc73) )
	ROM_LOAD( "jr-ic9f",      0x0200, 0x0200, CRC(085914d1) SHA1(3d6f9318f5a9f08ce89e4184e3efb9881f671fa7) )
ROM_END



GAMEX( 1983, monymony, 0,        zaccaria, monymony, 0, ROT90, "Zaccaria", "Money Money", GAME_IMPERFECT_SOUND )
GAMEX( 1984, jackrabt, 0,        zaccaria, jackrabt, 0, ROT90, "Zaccaria", "Jack Rabbit (set 1)", GAME_IMPERFECT_SOUND )
GAMEX( 1984, jackrab2, jackrabt, zaccaria, jackrabt, 0, ROT90, "Zaccaria", "Jack Rabbit (set 2)", GAME_IMPERFECT_SOUND )
GAMEX( 1984, jackrabs, jackrabt, zaccaria, jackrabt, 0, ROT90, "Zaccaria", "Jack Rabbit (special)", GAME_IMPERFECT_SOUND )
