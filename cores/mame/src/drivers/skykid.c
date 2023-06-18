/***************************************************************************

Dragon Buster (c) Namco 1984
Sky Kid	(c) Namco 1985

Driver by Manuel Abadia <manu@teleline.es>

****************************************************************************

Notes for "Sky Kid" :

In the "test mode", the "Flip Screen" Dip Switch is inverted :
  - when set to "Off", you can read "FLIP ON"
  - when set to "On" , you can read "FLIP OFF"
Correct behavior or emulation bug ?


Notes for "Dragon Buster" :

When "Cabinet" Dip Switch is set to "Cockail", the screen is flipped for
player 1 and normal for player 2 ! The controls are correct though.
Correct behavior or emulation bug ?

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "cpu/m6800/m6800.h"
#include "cpu/m6809/m6809.h"

static unsigned char *sharedram;
extern unsigned char *skykid_textram, *spriteram, *skykid_videoram;

/* from vidhrdw/skykid.c */
VIDEO_START( skykid );
READ_HANDLER( skykid_videoram_r );
WRITE_HANDLER( skykid_videoram_w );
WRITE_HANDLER( skykid_scroll_x_w );
WRITE_HANDLER( skykid_scroll_y_w );
WRITE_HANDLER( skykid_flipscreen_w );
VIDEO_UPDATE( skykid );
VIDEO_UPDATE( drgnbstr );
PALETTE_INIT( skykid );


static int irq_disabled = 1;
static int inputport_selected;

static INTERRUPT_GEN( skykid_interrupt )
{
	if (!irq_disabled)
		cpu_set_irq_line(0, M6809_IRQ_LINE, HOLD_LINE);
}

static WRITE_HANDLER( skykid_irq_ctrl_w )
{
	irq_disabled = offset;
}

static WRITE_HANDLER( inputport_select_w )
{
	if ((data & 0xe0) == 0x60)
		inputport_selected = data & 0x07;
	else if ((data & 0xe0) == 0xc0)
	{
		coin_lockout_global_w(~data & 1);
		coin_counter_w(0,data & 2);
		coin_counter_w(1,data & 4);
	}
}

#define reverse_bitstrm(data) ((data & 0x01) << 4) | ((data & 0x02) << 2) | (data & 0x04) \
							| ((data & 0x08) >> 2) | ((data & 0x10) >> 4)

static READ_HANDLER( inputport_r )
{
	int data = 0;

	switch (inputport_selected){
		case 0x00:	/* DSW B (bits 0-4) */
			data = ~(reverse_bitstrm(readinputport(1) & 0x1f)); break;
		case 0x01:	/* DSW B (bits 5-7), DSW A (bits 0-1) */
			data = ~(reverse_bitstrm((((readinputport(1) & 0xe0) >> 5) | ((readinputport(0) & 0x03) << 3)))); break;
		case 0x02:	/* DSW A (bits 2-6) */
			data = ~(reverse_bitstrm(((readinputport(0) & 0x7c) >> 2))); break;
		case 0x03:	/* DSW A (bit 7), DSW C (bits 0-3) */
			data = ~(reverse_bitstrm((((readinputport(0) & 0x80) >> 7) | ((readinputport(2) & 0x0f) << 1)))); break;
		case 0x04:	/* coins, start */
			data = ~(readinputport(3)); break;
		case 0x05:	/* 2P controls */
			data = ~(readinputport(5)); break;
		case 0x06:	/* 1P controls */
			data = ~(readinputport(4)); break;
		default:
			data = 0xff;
	}

	return data;
}

static WRITE_HANDLER( skykid_led_w )
{
	set_led_status(0,data & 0x08);
	set_led_status(1,data & 0x10);
}

static WRITE_HANDLER( skykid_halt_mcu_w )
{
	if (offset == 0){
		cpu_set_reset_line(1,PULSE_LINE);
		cpu_set_halt_line( 1, CLEAR_LINE );
	}
	else{
		cpu_set_halt_line( 1, ASSERT_LINE );
	}
}

READ_HANDLER( skykid_sharedram_r )
{
	return sharedram[offset];
}
WRITE_HANDLER( skykid_sharedram_w )
{
	sharedram[offset] = data;
}

WRITE_HANDLER( skykid_bankswitch_w )
{
	int bankaddress;
	unsigned char *RAM = memory_region(REGION_CPU1);

	bankaddress = 0x10000 + (offset ? 0 : 0x2000);
	cpu_setbank(1,&RAM[bankaddress]);
}


static MEMORY_READ_START( skykid_readmem )
	{ 0x0000, 0x1fff, MRA_BANK1 },				/* banked ROM */
	{ 0x2000, 0x2fff, skykid_videoram_r },		/* Video RAM (background) */
	{ 0x4000, 0x47ff, MRA_RAM },				/* video RAM (text layer) */
	{ 0x4800, 0x5fff, MRA_RAM },				/* RAM + Sprite RAM */
	{ 0x6800, 0x68ff, namcos1_wavedata_r },		/* PSG device, shared RAM */
	{ 0x6800, 0x6bff, skykid_sharedram_r },		/* shared RAM with the MCU */
	{ 0x7800, 0x7800, watchdog_reset_r },		/* watchdog reset */
	{ 0x8000, 0xffff, MRA_ROM },				/* ROM */
MEMORY_END

static MEMORY_WRITE_START( skykid_writemem )
	{ 0x0000, 0x1fff, MWA_ROM },				/* banked ROM */
	{ 0x2000, 0x2fff, skykid_videoram_w, &skykid_videoram },/* Video RAM (background) */
	{ 0x4000, 0x47ff, MWA_RAM, &skykid_textram },/* video RAM (text layer) */
	{ 0x4800, 0x5fff, MWA_RAM },				/* RAM + Sprite RAM */
	{ 0x6000, 0x60ff, skykid_scroll_y_w },		/* Y scroll register map */
	{ 0x6200, 0x63ff, skykid_scroll_x_w },		/* X scroll register map */
	{ 0x6800, 0x68ff, namcos1_wavedata_w, &namco_wavedata },/* PSG device, shared RAM */
	{ 0x6800, 0x6bff, skykid_sharedram_w, &sharedram },	/* shared RAM with the MCU */
	{ 0x7000, 0x7800, skykid_irq_ctrl_w },		/* IRQ control */
	{ 0x8000, 0x8800, skykid_halt_mcu_w },		/* MCU control */
	{ 0x9000, 0x9800, skykid_bankswitch_w },	/* Bankswitch control */
	{ 0xa000, 0xa001, skykid_flipscreen_w },	/* flip screen */
	{ 0x8000, 0xffff, MWA_ROM },				/* ROM */
MEMORY_END

static MEMORY_READ_START( mcu_readmem )
	{ 0x0000, 0x001f, hd63701_internal_registers_r },/* internal registers */
	{ 0x0080, 0x00ff, MRA_RAM },					/* built in RAM */
	{ 0x1000, 0x10ff, namcos1_wavedata_r },			/* PSG device, shared RAM */
	{ 0x1100, 0x113f, MRA_RAM },					/* PSG device */
	{ 0x1000, 0x13ff, skykid_sharedram_r },			/* shared RAM with the 6809 */
	{ 0x8000, 0xbfff, MRA_ROM },					/* MCU external ROM */
	{ 0xc000, 0xc800, MRA_RAM },					/* RAM */
	{ 0xf000, 0xffff, MRA_ROM },					/* MCU internal ROM */
MEMORY_END

static MEMORY_WRITE_START( mcu_writemem )
	{ 0x0000, 0x001f, hd63701_internal_registers_w },/* internal registers */
	{ 0x0080, 0x00ff, MWA_RAM },					/* built in RAM */
	{ 0x1000, 0x10ff, namcos1_wavedata_w },			/* PSG device, shared RAM */
	{ 0x1100, 0x113f, namcos1_sound_w, &namco_soundregs },/* PSG device */
	{ 0x1000, 0x13ff, skykid_sharedram_w },			/* shared RAM with the 6809 */
	{ 0x2000, 0x2000, MWA_NOP },					/* ??? */
	{ 0x4000, 0x4000, MWA_NOP },					/* ??? */
	{ 0x6000, 0x6000, MWA_NOP },					/* ??? */
	{ 0x8000, 0xbfff, MWA_ROM },					/* MCU external ROM */
	{ 0xc000, 0xc800, MWA_RAM },					/* RAM */
	{ 0xf000, 0xffff, MWA_ROM },					/* MCU internal ROM */
MEMORY_END


static READ_HANDLER( readFF )
{
	return 0xff;
}

static PORT_READ_START( mcu_readport )
	{ HD63701_PORT1, HD63701_PORT1, inputport_r },			/* input ports read */
	{ HD63701_PORT2, HD63701_PORT2, readFF },	/* leds won't work otherwise */
PORT_END

static PORT_WRITE_START( mcu_writeport )
	{ HD63701_PORT1, HD63701_PORT1, inputport_select_w },	/* input port select */
	{ HD63701_PORT2, HD63701_PORT2, skykid_led_w },			/* lamps */
PORT_END

INPUT_PORTS_START( skykid )
	PORT_START	/* DSW A */
	PORT_SERVICE( 0x01, IP_ACTIVE_HIGH )
	PORT_DIPNAME( 0x06, 0x00, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_2C ) )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(	0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x00, "Round Skip" )
	PORT_DIPSETTING(	0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x10, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x00, "Freeze screen" )
	PORT_DIPSETTING(	0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x20, DEF_STR( On ) )
	PORT_DIPNAME( 0xc0, 0x00, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 1C_2C ) )

	PORT_START	/* DSW B */
	PORT_DIPNAME( 0x03, 0x00, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x02, "1" )
	PORT_DIPSETTING(    0x01, "2" )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x03, "5" )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x0c, "20k every 80k" )
	PORT_DIPSETTING(    0x08, "30k every 90k" )
	PORT_DIPSETTING(    0x04, "20k and 80k" )
	PORT_DIPSETTING(    0x00, "30k and 90k" )
	PORT_DIPNAME( 0x10, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x10, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x20, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START	/* DSW C */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_DIPNAME( 0x04, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x04, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	PORT_BIT( 0xf0, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START	/* IN 0 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_SERVICE1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START	/* IN 1 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START	/* IN 2 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNUSED )
INPUT_PORTS_END

INPUT_PORTS_START( drgnbstr )
	PORT_START	/* DSW A */
	PORT_SERVICE( 0x01, IP_ACTIVE_HIGH )
	PORT_DIPNAME( 0x06, 0x00, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_2C ) )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(	0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x00, "Round Skip" )
	PORT_DIPSETTING(	0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x10, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x00, "Freeze screen" )
	PORT_DIPSETTING(	0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x20, DEF_STR( On ) )
	PORT_DIPNAME( 0xc0, 0x00, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 1C_2C ) )

	PORT_START	/* DSW B */
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x01, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x02, DEF_STR( On ) )
	PORT_DIPNAME( 0x0c, 0x00, "Starting Vitality" )			/* Difficulty ?*/
	PORT_DIPSETTING(    0x0c, "160" )
	PORT_DIPSETTING(    0x00, "128" )
	PORT_DIPSETTING(    0x04, "96" )
	PORT_DIPSETTING(    0x08, "64" )
	PORT_DIPNAME( 0x10, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x10, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x20, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Unknown ) )			/* Duplicated "Service Mode" ?*/
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x80, DEF_STR( On ) )

	PORT_START	/* DSW C */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_DIPNAME( 0x04, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	PORT_BIT( 0xf0, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START	/* IN 0 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_SERVICE1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START	/* IN 1 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_4WAY )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_4WAY )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN  | IPF_4WAY )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP    | IPF_4WAY )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START	/* IN 2 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN  | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP    | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNUSED )
INPUT_PORTS_END


static struct GfxLayout text_layout =
{
	8,8,		/* 8*8 characters */
	512,		/* 512 characters */
	2,			/* 2 bits per pixel */
	{ 0, 4 },	/* the bitplanes are packed in the same byte */
	{ 8*8, 8*8+1, 8*8+2, 8*8+3, 0, 1, 2, 3 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	16*8		/* every char takes 16 consecutive bytes */
};

static struct GfxLayout tile_layout =
{
	8,8,		/* 8*8 characters */
	512,		/* 512 characters */
	2,			/* 2 bits per pixel */
	{ 0, 4 },
	{ 0, 1, 2, 3, 8+0, 8+1, 8+2, 8+3 },
	{ 0*8, 2*8, 4*8, 6*8, 8*8, 10*8, 12*8, 14*8 },
	16*8		/* every char takes 16 consecutive bytes */
};

static struct GfxLayout sprite_layout1 =
{
	16,16,       	/* 16*16 sprites */
	128,           	/* 128 sprites */
	3,              /* 3 bits per pixel */
	{ 0x4000*8+4, 0, 4 },
	{ 0, 1, 2, 3, 8*8, 8*8+1, 8*8+2, 8*8+3,
			16*8+0, 16*8+1, 16*8+2, 16*8+3, 24*8+0, 24*8+1, 24*8+2, 24*8+3 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
			32*8, 33*8, 34*8, 35*8, 36*8, 37*8, 38*8, 39*8 },
	64*8    /* every sprite takes 64 bytes */
};

static struct GfxLayout sprite_layout2 =
{
	16,16,       	/* 16*16 sprites */
	128,           	/* 128 sprites */
	3,              /* 3 bits per pixel */
	{ 0x4000*8, 0x2000*8, 0x2000*8+4 },
	{ 0, 1, 2, 3, 8*8, 8*8+1, 8*8+2, 8*8+3,
			16*8+0, 16*8+1, 16*8+2, 16*8+3, 24*8+0, 24*8+1, 24*8+2, 24*8+3 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
			32*8, 33*8, 34*8, 35*8, 36*8, 37*8, 38*8, 39*8 },
	64*8    /* every sprite takes 64 bytes */
};

static struct GfxLayout sprite_layout3 =
{
	16,16,       	/* 16*16 sprites */
	128,           	/* 128 sprites */
	3,              /* 3 bits per pixel */
	{ 0x8000*8, 0x6000*8, 0x6000*8+4 },
	{ 0, 1, 2, 3, 8*8, 8*8+1, 8*8+2, 8*8+3,
			16*8+0, 16*8+1, 16*8+2, 16*8+3, 24*8+0, 24*8+1, 24*8+2, 24*8+3 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
			32*8, 33*8, 34*8, 35*8, 36*8, 37*8, 38*8, 39*8 },
	64*8    /* every sprite takes 64 bytes */
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &text_layout,		0, 64 },
	{ REGION_GFX2, 0, &tile_layout,		64*4, 128 },
	{ REGION_GFX3, 0, &sprite_layout1,	64*4+128*4, 64 },
	{ REGION_GFX3, 0, &sprite_layout2,	64*4+128*4, 64 },
	{ REGION_GFX3, 0, &sprite_layout3,	64*4+128*4, 64 },
	{-1 }
};


static struct namco_interface namco_interface =
{
	49152000/2048, 		/* 24000 Hz */
	8,					/* number of voices */
	100,				/* playback volume */
	-1,					/* memory region */
	0					/* stereo */
};

static MACHINE_DRIVER_START( skykid )

	/* basic machine hardware */
	MDRV_CPU_ADD(M6809,49152000/32)	/* ??? */
	MDRV_CPU_MEMORY(skykid_readmem,skykid_writemem)
	MDRV_CPU_VBLANK_INT(skykid_interrupt,1)

	MDRV_CPU_ADD(HD63701,49152000/32)	/* or compatible 6808 with extra instructions */
	MDRV_CPU_MEMORY(mcu_readmem,mcu_writemem)
	MDRV_CPU_PORTS(mcu_readport,mcu_writeport)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,1)

	MDRV_FRAMES_PER_SECOND(60.606060)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(100)	/* we need heavy synch */

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(36*8, 28*8)
	MDRV_VISIBLE_AREA(0*8, 36*8-1, 0*8, 28*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(256)
	MDRV_COLORTABLE_LENGTH(64*4+128*4+64*8)

	MDRV_PALETTE_INIT(skykid)
	MDRV_VIDEO_START(skykid)
	MDRV_VIDEO_UPDATE(skykid)

	/* sound hardware */
	MDRV_SOUND_ADD(NAMCO_15XX, namco_interface)
MACHINE_DRIVER_END


ROM_START( skykid )
	ROM_REGION( 0x14000, REGION_CPU1, 0 )	/* 6809 code */
	ROM_LOAD( "sk1_2.6c",		0x08000, 0x4000, CRC(ea8a5822) SHA1(5b13133410bcb7d647e662b476dbfd2edab8aac0) )
	ROM_LOAD( "sk1-1x.6b",		0x0c000, 0x4000, CRC(7abe6c6c) SHA1(7d2631cc6149fa3e02b1355cb899de5474ff5d0a) )
	ROM_LOAD( "sk1_3.6d",		0x10000, 0x4000, CRC(314b8765) SHA1(d90a8a853ce672fe5ee190f07bcb33262c73df3b) )	/* banked ROM */

	ROM_REGION(  0x10000 , REGION_CPU2, 0 ) /* MCU code */
	ROM_LOAD( "sk1_4.3c",		0x8000, 0x2000, CRC(a460d0e0) SHA1(7124ffeb3b84b282940dcbf9421ae4934bcce1c8) )	/* subprogram for the MCU */
	ROM_LOAD( "sk1-mcu.bin",	0xf000, 0x1000, CRC(6ef08fb3) SHA1(4842590d60035a0059b0899eb2d5f58ae72c2529) )	/* MCU internal code */
																/* Using Pacland code (probably similar) */

	ROM_REGION( 0x02000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "sk1_6.6l",		0x00000, 0x2000, CRC(58b731b9) SHA1(40f7be85914833ce02a734c20d68c0db8b77911d) )	/* chars */

	ROM_REGION( 0x02000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "sk1_5.7e",		0x00000, 0x2000, CRC(c33a498e) SHA1(9f89a514888418a9bebbca341a8cc66e41b58acb) )

	ROM_REGION( 0x0a000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "sk1_8.10n",		0x00000, 0x4000, CRC(44bb7375) SHA1(5b2fa6782671150bab5f3c3ac46b47bc23f3d7e0) )	/* sprites */
	ROM_LOAD( "sk1_7.10m",		0x04000, 0x4000, CRC(3454671d) SHA1(723b26a0f208addc2a22736457cb4be6ab6c69cc) )
	/* empty space to decode the sprites as 3bpp */

	ROM_REGION( 0x0700, REGION_PROMS, 0 )
	ROM_LOAD( "sk1-1.2n",		0x0000, 0x0100, CRC(0218e726) SHA1(8b766162a4783c058d9a1ecf8741673d7ef955fb) )	/* red component */
	ROM_LOAD( "sk1-2.2p",		0x0100, 0x0100, CRC(fc0d5b85) SHA1(d1b13e42e735b24594cf0b840dee8110de23369e) )	/* green component */
	ROM_LOAD( "sk1-3.2r",		0x0200, 0x0100, CRC(d06b620b) SHA1(968a2d62c65e201d521e9efa8fcf6ad15898e4b3) )	/* blue component */
	ROM_LOAD( "sk1-4.5n",		0x0300, 0x0200, CRC(c697ac72) SHA1(3b79755e6cbb22c14fc4affdbd3f4521da1d90e8) )	/* tiles lookup table */
	ROM_LOAD( "sk1-5.6n",		0x0500, 0x0200, CRC(161514a4) SHA1(4488ce60d12be6586e4a1ddbbfd06bf4e7dfaceb) )	/* sprites lookup table */
ROM_END

ROM_START( skykido )
	ROM_REGION( 0x14000, REGION_CPU1, 0 )	/* 6809 code */
	ROM_LOAD( "sk1_2.6c",		0x08000, 0x4000, CRC(ea8a5822) SHA1(5b13133410bcb7d647e662b476dbfd2edab8aac0) )
	ROM_LOAD( "sk1_1.6b",		0x0c000, 0x4000, CRC(070a49d4) SHA1(4b994bde3e34b574bd927843804d2fb1a08d1bdf) )
	ROM_LOAD( "sk1_3.6d",		0x10000, 0x4000, CRC(314b8765) SHA1(d90a8a853ce672fe5ee190f07bcb33262c73df3b) )	/* banked ROM */

	ROM_REGION(  0x10000 , REGION_CPU2, 0 ) /* MCU code */
	ROM_LOAD( "sk1_4.3c",		0x8000, 0x2000, CRC(a460d0e0) SHA1(7124ffeb3b84b282940dcbf9421ae4934bcce1c8) )	/* subprogram for the MCU */
	ROM_LOAD( "sk1-mcu.bin",	0xf000, 0x1000, CRC(6ef08fb3) SHA1(4842590d60035a0059b0899eb2d5f58ae72c2529) )	/* MCU internal code */
																/* Using Pacland code (probably similar) */

	ROM_REGION( 0x02000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "sk1_6.6l",		0x00000, 0x2000, CRC(58b731b9) SHA1(40f7be85914833ce02a734c20d68c0db8b77911d) )	/* chars */

	ROM_REGION( 0x02000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "sk1_5.7e",		0x00000, 0x2000, CRC(c33a498e) SHA1(9f89a514888418a9bebbca341a8cc66e41b58acb) )

	ROM_REGION( 0x0a000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "sk1_8.10n",		0x00000, 0x4000, CRC(44bb7375) SHA1(5b2fa6782671150bab5f3c3ac46b47bc23f3d7e0) )	/* sprites */
	ROM_LOAD( "sk1_7.10m",		0x04000, 0x4000, CRC(3454671d) SHA1(723b26a0f208addc2a22736457cb4be6ab6c69cc) )
	/* empty space to decode the sprites as 3bpp */

	ROM_REGION( 0x0700, REGION_PROMS, 0 )
	ROM_LOAD( "sk1-1.2n",		0x0000, 0x0100, CRC(0218e726) SHA1(8b766162a4783c058d9a1ecf8741673d7ef955fb) )	/* red component */
	ROM_LOAD( "sk1-2.2p",		0x0100, 0x0100, CRC(fc0d5b85) SHA1(d1b13e42e735b24594cf0b840dee8110de23369e) )	/* green component */
	ROM_LOAD( "sk1-3.2r",		0x0200, 0x0100, CRC(d06b620b) SHA1(968a2d62c65e201d521e9efa8fcf6ad15898e4b3) )	/* blue component */
	ROM_LOAD( "sk1-4.5n",		0x0300, 0x0200, CRC(c697ac72) SHA1(3b79755e6cbb22c14fc4affdbd3f4521da1d90e8) )	/* tiles lookup table */
	ROM_LOAD( "sk1-5.6n",		0x0500, 0x0200, CRC(161514a4) SHA1(4488ce60d12be6586e4a1ddbbfd06bf4e7dfaceb) )	/* sprites lookup table */
ROM_END

ROM_START( skykidd )
	ROM_REGION( 0x14000, REGION_CPU1, 0 )	/* 6809 code */
	ROM_LOAD( "sk1_2x.6c",		0x08000, 0x4000, CRC(8370671a) SHA1(7038f952ebfc4482440b73ee4027fa908561d122) )
	ROM_LOAD( "sk1_1.6b",		0x0c000, 0x4000, CRC(070a49d4) SHA1(4b994bde3e34b574bd927843804d2fb1a08d1bdf) )
	ROM_LOAD( "sk1_3.6d",		0x10000, 0x4000, CRC(314b8765) SHA1(d90a8a853ce672fe5ee190f07bcb33262c73df3b) )	/* banked ROM */

	ROM_REGION(  0x10000 , REGION_CPU2, 0 ) /* MCU code */
	ROM_LOAD( "sk1_4x.3c",		0x8000, 0x2000, CRC(887137cc) SHA1(dd0f66afb78833c4da73539b692854346f448c0d) )	/* subprogram for the MCU */
	ROM_LOAD( "sk1-mcu.bin",	0xf000, 0x1000, CRC(6ef08fb3) SHA1(4842590d60035a0059b0899eb2d5f58ae72c2529) )	/* MCU internal code */
																/* Using Pacland code (probably similar) */

	ROM_REGION( 0x02000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "sk1_6.6l",		0x00000, 0x2000, CRC(58b731b9) SHA1(40f7be85914833ce02a734c20d68c0db8b77911d) )	/* chars */

	ROM_REGION( 0x02000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "sk1_5.7e",		0x00000, 0x2000, CRC(c33a498e) SHA1(9f89a514888418a9bebbca341a8cc66e41b58acb) )

	ROM_REGION( 0x0a000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "sk1_8.10n",		0x00000, 0x4000, CRC(44bb7375) SHA1(5b2fa6782671150bab5f3c3ac46b47bc23f3d7e0) )	/* sprites */
	ROM_LOAD( "sk1_7.10m",		0x04000, 0x4000, CRC(3454671d) SHA1(723b26a0f208addc2a22736457cb4be6ab6c69cc) )
	/* empty space to decode the sprites as 3bpp */

	ROM_REGION( 0x0700, REGION_PROMS, 0 )
	ROM_LOAD( "sk1-1.2n",		0x0000, 0x0100, CRC(0218e726) SHA1(8b766162a4783c058d9a1ecf8741673d7ef955fb) )	/* red component */
	ROM_LOAD( "sk1-2.2p",		0x0100, 0x0100, CRC(fc0d5b85) SHA1(d1b13e42e735b24594cf0b840dee8110de23369e) )	/* green component */
	ROM_LOAD( "sk1-3.2r",		0x0200, 0x0100, CRC(d06b620b) SHA1(968a2d62c65e201d521e9efa8fcf6ad15898e4b3) )	/* blue component */
	ROM_LOAD( "sk1-4.5n",		0x0300, 0x0200, CRC(c697ac72) SHA1(3b79755e6cbb22c14fc4affdbd3f4521da1d90e8) )	/* tiles lookup table */
	ROM_LOAD( "sk1-5.6n",		0x0500, 0x0200, CRC(161514a4) SHA1(4488ce60d12be6586e4a1ddbbfd06bf4e7dfaceb) )	/* sprites lookup table */
ROM_END

ROM_START( drgnbstr )
	ROM_REGION( 0x14000, REGION_CPU1, 0 ) /* 6809 code */
	ROM_LOAD( "db1_2b.6c",		0x08000, 0x04000, CRC(0f11cd17) SHA1(691d853f4f08898ecf4bccfb70a568de309329f1) )
	ROM_LOAD( "db1_1.6b",		0x0c000, 0x04000, CRC(1c7c1821) SHA1(8b6111afc42e2996bdc2fc276be0c40556cd431e) )
	ROM_LOAD( "db1_3.6d",		0x10000, 0x04000, CRC(6da169ae) SHA1(235211c26562fef0660e3fde1e87f2e52626d119) )	/* banked ROM */

	ROM_REGION(  0x10000 , REGION_CPU2, 0 ) /* MCU code */
	ROM_LOAD( "db1_4.3c",		0x8000, 0x02000, CRC(8a0b1fc1) SHA1(c2861d0da63e2d17f2d1ad46dccf753ecd902ce3) )	/* subprogram for the MCU */
	ROM_LOAD( "pl1-mcu.bin",	0xf000,	0x01000, CRC(6ef08fb3) SHA1(4842590d60035a0059b0899eb2d5f58ae72c2529) )	/* The MCU internal code is missing */
																/* Using Pacland code (probably similar) */

	ROM_REGION( 0x02000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "db1_6.6l",		0x00000, 0x2000, CRC(c080b66c) SHA1(05dcd45274d0bd12ef8ae7fd10c8719e679b3e7b) )	/* tiles */

	ROM_REGION( 0x02000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "db1_5.7e",		0x00000, 0x2000, CRC(28129aed) SHA1(d7f52e871d97179ec88c142a1c70eb6ad09e534a) )

	ROM_REGION( 0x0a000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "db1_8.10n",		0x00000, 0x4000, CRC(11942c61) SHA1(0f065cb82cf83967e90b3c7326b36956f4fa9a52) )	/* sprites */
	ROM_LOAD( "db1_7.10m",		0x04000, 0x4000, CRC(cc130fe2) SHA1(4f5d4f21152b3b4e523a6d17dd5ff5cef52447f2) )
		/* empty space to decode the sprites as 3bpp */

	ROM_REGION( 0x0700, REGION_PROMS, 0 )
	ROM_LOAD( "db1-1.2n",		0x00000, 0x0100, CRC(3f8cce97) SHA1(027b3fb0f322a9d68b434b207a40b31799a8a8d6) )	/* red component */
	ROM_LOAD( "db1-2.2p",		0x00100, 0x0100, CRC(afe32436) SHA1(e405787f7f2aa992edd63078e3944334d8acddb1) )	/* green component */
	ROM_LOAD( "db1-3.2r",		0x00200, 0x0100, CRC(c95ff576) SHA1(861a7340d29e6a6a0d5ead93abd3f73cc3df0cc7) )	/* blue component */
	ROM_LOAD( "db1-4.5n",		0x00300, 0x0200, CRC(b2180c21) SHA1(a5d14c31d54f04494ea99c3d94bd1b5e072b612e) )	/* tiles lookup table */
	ROM_LOAD( "db1-5.6n",		0x00500, 0x0200, CRC(5e2b3f74) SHA1(ef58661fa12a52bc358e81179254d37de7551b38) )	/* sprites lookup table */
ROM_END



GAME( 1985, skykid,   0,      skykid, skykid,   0, ROT0, "Namco", "Sky Kid (New Ver.)" )
GAME( 1985, skykido,  skykid, skykid, skykid,   0, ROT0, "Namco", "Sky Kid (Old Ver.)" )
GAMEX(1985, skykidd,  skykid, skykid, skykid,   0, ROT0, "Namco", "Sky Kid (60A1 Ver.)", GAME_NOT_WORKING )
GAME( 1984, drgnbstr, 0,      skykid, drgnbstr, 0, ROT0, "Namco", "Dragon Buster" )
