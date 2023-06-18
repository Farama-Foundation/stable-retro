/***************************************************************************

Baraduke/Metro-Cross (c) Namco 1985

Driver by Manuel Abadia <manu@teleline.es>

TO DO:
	- in Metro-Cross test mode, inputs are unnecessarily repeated, so
	selecting the sound you want to listen to is almost impossible.
	- remove the sound kludge in Baraduke.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "cpu/m6800/m6800.h"

static unsigned char *sharedram;
extern unsigned char *baraduke_textram, *spriteram, *baraduke_videoram;

/* from vidhrdw/baraduke.c */
VIDEO_START( baraduke );
VIDEO_START( metrocrs );
VIDEO_UPDATE( baraduke );
VIDEO_UPDATE( metrocrs );
READ_HANDLER( baraduke_textlayer_r );
READ_HANDLER( baraduke_videoram_r );
WRITE_HANDLER( baraduke_textlayer_w );
WRITE_HANDLER( baraduke_videoram_w );
WRITE_HANDLER( baraduke_scroll0_w );
WRITE_HANDLER( baraduke_scroll1_w );
PALETTE_INIT( baraduke );

static int inputport_selected;

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
		case 0x00:	/* DSW A (bits 0-4) */
			data = ~(reverse_bitstrm(readinputport(0) & 0x1f)); break;
		case 0x01:	/* DSW A (bits 5-7), DSW B (bits 0-1) */
			data = ~(reverse_bitstrm((((readinputport(0) & 0xe0) >> 5) | ((readinputport(1) & 0x03) << 3)))); break;
		case 0x02:	/* DSW B (bits 2-6) */
			data = ~(reverse_bitstrm(((readinputport(1) & 0x7c) >> 2))); break;
		case 0x03:	/* DSW B (bit 7), DSW C (bits 0-3) */
			data = ~(reverse_bitstrm((((readinputport(1) & 0x80) >> 7) | ((readinputport(2) & 0x0f) << 1)))); break;
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

static WRITE_HANDLER( baraduke_lamps_w )
{
	set_led_status(0,data & 0x08);
	set_led_status(1,data & 0x10);
}

READ_HANDLER( baraduke_sharedram_r )
{
	return sharedram[offset];
}
WRITE_HANDLER( baraduke_sharedram_w )
{
	sharedram[offset] = data;
}

static MEMORY_READ_START( baraduke_readmem )
	{ 0x0000, 0x17ff, MRA_RAM },				/* RAM */
	{ 0x1800, 0x1fff, MRA_RAM },				/* Sprite RAM */
	{ 0x2000, 0x3fff, baraduke_videoram_r },	/* Video RAM */
	{ 0x4000, 0x40ff, namcos1_wavedata_r },		/* PSG device, shared RAM */
	{ 0x4000, 0x43ff, baraduke_sharedram_r },	/* shared RAM with the MCU */
	{ 0x4800, 0x4fff, MRA_RAM },				/* video RAM (text layer) */
	{ 0x6000, 0xffff, MRA_ROM },				/* ROM */
MEMORY_END

static MEMORY_WRITE_START( baraduke_writemem )
	{ 0x0000, 0x17ff, MWA_RAM },				/* RAM */
	{ 0x1800, 0x1fff, MWA_RAM, &spriteram },	/* Sprite RAM */
	{ 0x2000, 0x3fff, baraduke_videoram_w, &baraduke_videoram },/* Video RAM */
	{ 0x4000, 0x40ff, namcos1_wavedata_w },		/* PSG device, shared RAM */
	{ 0x4000, 0x43ff, baraduke_sharedram_w, &sharedram },/* shared RAM with the MCU */
	{ 0x4800, 0x4fff, MWA_RAM, &baraduke_textram },/* video RAM (text layer) */
	{ 0x8000, 0x8000, watchdog_reset_w },		/* watchdog reset */
/*	{ 0x8800, 0x8800, MWA_NOP },				 // ??? /*/
	{ 0xb000, 0xb002, baraduke_scroll0_w },		/* scroll (layer 0) */
	{ 0xb004, 0xb006, baraduke_scroll1_w },		/* scroll (layer 1) */
	{ 0x6000, 0xffff, MWA_ROM },				/* ROM */
MEMORY_END

READ_HANDLER( soundkludge_r )
{
	static int counter;

	return ((counter++) >> 4) & 0xff;
}

static MEMORY_READ_START( mcu_readmem )
	{ 0x0000, 0x001f, hd63701_internal_registers_r },/* internal registers */
	{ 0x0080, 0x00ff, MRA_RAM },					/* built in RAM */
	{ 0x1000, 0x10ff, namcos1_wavedata_r },			/* PSG device, shared RAM */
	{ 0x1105, 0x1105, soundkludge_r },				/* cures speech */
	{ 0x1100, 0x113f, MRA_RAM },					/* PSG device */
	{ 0x1000, 0x13ff, baraduke_sharedram_r },		/* shared RAM with the 6809 */
	{ 0x8000, 0xbfff, MRA_ROM },					/* MCU external ROM */
	{ 0xc000, 0xc800, MRA_RAM },					/* RAM */
	{ 0xf000, 0xffff, MRA_ROM },					/* MCU internal ROM */
MEMORY_END

static MEMORY_WRITE_START( mcu_writemem )
	{ 0x0000, 0x001f, hd63701_internal_registers_w },/* internal registers */
	{ 0x0080, 0x00ff, MWA_RAM },				/* built in RAM */
	{ 0x1000, 0x10ff, namcos1_wavedata_w, &namco_wavedata },/* PSG device, shared RAM */
	{ 0x1100, 0x113f, namcos1_sound_w, &namco_soundregs },/* PSG device */
	{ 0x1000, 0x13ff, baraduke_sharedram_w },	/* shared RAM with the 6809 */
/*	{ 0x8000, 0x8000, MWA_NOP },				 // ??? /*/
/*	{ 0x8800, 0x8800, MWA_NOP },				 // ??? /*/
	{ 0x8000, 0xbfff, MWA_ROM },				/* MCU external ROM */
	{ 0xc000, 0xc800, MWA_RAM },				/* RAM */
	{ 0xf000, 0xffff, MWA_ROM },				/* MCU internal ROM */
MEMORY_END

static PORT_READ_START( mcu_readport )
	{ HD63701_PORT1, HD63701_PORT1, inputport_r },			/* input ports read */
PORT_END

static PORT_WRITE_START( mcu_writeport )
	{ HD63701_PORT1, HD63701_PORT1, inputport_select_w },	/* input port select */
	{ HD63701_PORT2, HD63701_PORT2, baraduke_lamps_w },		/* lamps */
PORT_END

INPUT_PORTS_START( baraduke )
	PORT_START	/* DSW A */
	PORT_SERVICE( 0x01, IP_ACTIVE_HIGH )
	PORT_DIPNAME( 0x06, 0x00, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x04, "2" )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x02, "4" )
	PORT_DIPSETTING(    0x06, "5" )
	PORT_DIPNAME( 0x18, 0x00, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x18, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 1C_2C ) )
	PORT_DIPNAME( 0x20, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0xc0, 0x00, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 1C_2C ) )

	PORT_START	/* DSW B */
	PORT_DIPNAME( 0x03, 0x00, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x02, "Every 10k" )
	PORT_DIPSETTING(    0x00, "10k and every 20k" )
	PORT_DIPSETTING(    0x01, "Every 20k" )
	PORT_DIPSETTING(    0x03, "None" )
	PORT_DIPNAME( 0x0c, 0x00, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x08, "Easy" )
	PORT_DIPSETTING(    0x00, "Normal" )
	PORT_DIPSETTING(    0x04, "Hard" )
	PORT_DIPSETTING(    0x0c, "Very hard" )
	PORT_BITX(    0x10, 0x00, IPT_DIPSWITCH_NAME | IPF_CHEAT, "Rack test", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x10, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x00, "Freeze" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x20, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x00, "Allow continue from last level" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x80, DEF_STR( On ) )

	PORT_START	/* DSW C */
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x01, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x02, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Cocktail ) )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_SERVICE ) /* Another service dip */
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
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN  | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP    | IPF_8WAY )
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

INPUT_PORTS_START( metrocrs )
	PORT_START	/* DSW A */
	PORT_SERVICE( 0x01, IP_ACTIVE_HIGH )
	PORT_DIPNAME( 0x06, 0x00, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_2C ) )
	PORT_DIPNAME( 0x18, 0x00, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x10, "Easy" )
	PORT_DIPSETTING(    0x00, "Normal" )
	PORT_DIPSETTING(    0x08, "Hard" )
	PORT_DIPSETTING(    0x18, "Very hard" )
	PORT_DIPNAME( 0x20, 0x00, "Allow Continue" )
	PORT_DIPSETTING(    0x20, DEF_STR( No ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Yes ) )
	PORT_DIPNAME( 0xc0, 0x00, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 1C_2C ) )

	PORT_START	/* DSW B */
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_BITX(    0x02, 0x00, IPT_DIPSWITCH_NAME | IPF_CHEAT, "Rack test", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x02, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x00, "Freeze" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x04, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x10, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x20, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x80, DEF_STR( On ) )

	PORT_START	/* DSW C */
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x01, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x02, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Cocktail ) )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_SERVICE ) /* Another service dip */
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
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN  | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP    | IPF_8WAY )
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

static struct GfxLayout tile_layout1 =
{
	8,8,		/* 8*8 characters */
	512,		/* 512 characters */
	3,			/* 3 bits per pixel */
	{ 0x8000*8, 0, 4 },
	{ 0, 1, 2, 3, 8+0, 8+1, 8+2, 8+3 },
	{ 0*8, 2*8, 4*8, 6*8, 8*8, 10*8, 12*8, 14*8 },
	16*8		/* every char takes 16 consecutive bytes */
};

static struct GfxLayout tile_layout2 =
{
	8,8,		/* 8*8 characters */
	512,		/* 512 characters */
	3,			/* 3 bits per pixel */
	{ 0x8000*8+4, 0x2000*8, 0x2000*8+4 },
	{ 0, 1, 2, 3, 8+0, 8+1, 8+2, 8+3 },
	{ 0*8, 2*8, 4*8, 6*8, 8*8, 10*8, 12*8, 14*8 },
	16*8		/* every char takes 16 consecutive bytes */
};

static struct GfxLayout tile_layout3 =
{
	8,8,		/* 8*8 characters */
	512,		/* 512 characters */
	3,			/* 3 bits per pixel */
	{ 0xa000*8, 0x4000*8, 0x4000*8+4 },
	{ 0, 1, 2, 3, 8+0, 8+1, 8+2, 8+3 },
	{ 0*8, 2*8, 4*8, 6*8, 8*8, 10*8, 12*8, 14*8 },
	16*8		/* every char takes 16 consecutive bytes */
};

static struct GfxLayout tile_layout4 =
{
	8,8,		/* 8*8 characters */
	512,		/* 512 characters */
	3,			/* 3 bits per pixel */
	{ 0xa000*8+4, 0x6000*8, 0x6000*8+4 },
	{ 0, 1, 2, 3, 8+0, 8+1, 8+2, 8+3 },
	{ 0*8, 2*8, 4*8, 6*8, 8*8, 10*8, 12*8, 14*8 },
	16*8		/* every char takes 16 consecutive bytes */
};

static struct GfxLayout spritelayout =
{
	16,16,		/* 16*16 sprites */
	512,		/* 512 sprites */
	4,			/* 4 bits per pixel */
	{ 0, 1, 2, 3 },
	{ 0*4, 1*4, 2*4, 3*4, 4*4, 5*4, 6*4, 7*4,
		8*4, 9*4, 10*4, 11*4, 12*4, 13*4, 14*4, 15*4 },
    { 8*8*0, 8*8*1, 8*8*2, 8*8*3, 8*8*4, 8*8*5, 8*8*6, 8*8*7,
	8*8*8, 8*8*9, 8*8*10, 8*8*11, 8*8*12, 8*8*13, 8*8*14, 8*8*15 },
	128*8		/* every sprite takes 128 consecutive bytes */
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &text_layout,		0, 512 },
	{ REGION_GFX2, 0, &tile_layout1,	0, 256 },
	{ REGION_GFX2, 0, &tile_layout2,	0, 256 },
	{ REGION_GFX2, 0, &tile_layout3,	0, 256 },
	{ REGION_GFX2, 0, &tile_layout4,	0, 256 },
	{ REGION_GFX3, 0, &spritelayout,	0, 128 },
	{ -1 }
};

static struct namco_interface namco_interface =
{
	49152000/2048, 		/* 24000Hz */
	8,					/* number of voices */
	100,				/* playback volume */
	-1,					/* memory region */
	0					/* stereo */
};


static MACHINE_DRIVER_START( baraduke )

	/* basic machine hardware */
	MDRV_CPU_ADD(M6809,49152000/32)	/* ??? */
	MDRV_CPU_MEMORY(baraduke_readmem,baraduke_writemem)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,1)

	MDRV_CPU_ADD(HD63701,49152000/32)	/* or compatible 6808 with extra instructions */
	MDRV_CPU_MEMORY(mcu_readmem,mcu_writemem)
	MDRV_CPU_PORTS(mcu_readport,mcu_writeport)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,1)

	MDRV_FRAMES_PER_SECOND(60.606060)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(100)		/* we need heavy synch */

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(36*8, 28*8)
	MDRV_VISIBLE_AREA(0*8, 36*8-1, 0*8, 28*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(2048)
	MDRV_COLORTABLE_LENGTH(2048*4)

	MDRV_PALETTE_INIT(baraduke)
	MDRV_VIDEO_START(baraduke)
	MDRV_VIDEO_UPDATE(baraduke)

	/* sound hardware */
	MDRV_SOUND_ADD(NAMCO_15XX, namco_interface)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( metrocrs )

	/* basic machine hardware */
	MDRV_CPU_ADD(M6809,49152000/32)	/* ??? */
	MDRV_CPU_MEMORY(baraduke_readmem,baraduke_writemem)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,1)

	MDRV_CPU_ADD(HD63701,49152000/32)	/* or compatible 6808 with extra instructions */
	MDRV_CPU_MEMORY(mcu_readmem,mcu_writemem)
	MDRV_CPU_PORTS(mcu_readport,mcu_writeport)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,1)

	MDRV_FRAMES_PER_SECOND(60.606060)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(100)		/* we need heavy synch */

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(36*8, 28*8)
	MDRV_VISIBLE_AREA(0*8, 36*8-1, 0*8, 28*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(2048)
	MDRV_COLORTABLE_LENGTH(2048*4)

	MDRV_PALETTE_INIT(baraduke)
	MDRV_VIDEO_START(baraduke)
	MDRV_VIDEO_UPDATE(metrocrs)

	/* sound hardware */
	MDRV_SOUND_ADD(NAMCO_15XX, namco_interface)
MACHINE_DRIVER_END

ROM_START( baraduke )
	ROM_REGION( 0x10000, REGION_CPU1, 0 ) /* 6809 code */
	ROM_LOAD( "prg1.9c",	0x6000, 0x02000, CRC(ea2ea790) SHA1(ab6f523803b2b0ea04b78f2f252de6c2d344a26c) )
	ROM_LOAD( "prg2.9a",	0x8000, 0x04000, CRC(9a0a9a87) SHA1(6d88fb5b443c822ede4884d4452e333834b16aca) )
	ROM_LOAD( "prg3.9b",	0xc000, 0x04000, CRC(383e5458) SHA1(091f25e287f0a81649c9a4fa196ebe4112a82295) )

	ROM_REGION(  0x10000 , REGION_CPU2, 0 ) /* MCU code */
	ROM_LOAD( "prg4.3b",	0x8000,  0x4000, CRC(abda0fe7) SHA1(f7edcb5f9fa47bb38a8207af5678cf4ccc243547) )	/* subprogram for the MCU */
	ROM_LOAD( "pl1-mcu.bin",0xf000,	 0x1000, CRC(6ef08fb3) SHA1(4842590d60035a0059b0899eb2d5f58ae72c2529) )	/* The MCU internal code is missing */
															/* Using Pacland code (probably similar) */
	ROM_REGION( 0x02000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "ch1.3j",		0x00000, 0x2000, CRC(706b7fee) SHA1(e5694289bd4346c1a3a004feaa940710cea755c6) )	/* characters */

	ROM_REGION( 0x0c000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "ch2.4p",		0x00000, 0x4000, CRC(b0bb0710) SHA1(797832aea59bf80342fd2a3505645f2766bde65b) )	/* tiles */
	ROM_LOAD( "ch3.4n",		0x04000, 0x4000, CRC(0d7ebec9) SHA1(6b86b476db61f5760bc8610b51adc1115cfdad96) )
	ROM_LOAD( "ch4.4m",		0x08000, 0x4000, CRC(e5da0896) SHA1(abb8bf7e9dc1c60bc0a20a691109fb145bb1d8e0) )

	ROM_REGION( 0x10000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "obj1.8k",	0x00000, 0x4000, CRC(87a29acc) SHA1(3aa00efc95d1da50f6e4637d101640328287dea1) )	/* sprites */
	ROM_LOAD( "obj2.8l",	0x04000, 0x4000, CRC(72b6d20c) SHA1(e40b48dacefce4fd62ab28d3e6ff3932d4ff005b) )
	ROM_LOAD( "obj3.8m",	0x08000, 0x4000, CRC(3076af9c) SHA1(57ce09b298fd0bae94e4d8c817a34ce812c3ddfc) )
	ROM_LOAD( "obj4.8n",	0x0c000, 0x4000, CRC(8b4c09a3) SHA1(46e0ef39cb313c6780f6137769153dc4a054c77f) )

	ROM_REGION( 0x1000, REGION_PROMS, 0 )
	ROM_LOAD( "prmcolbg.1n",0x0000, 0x0800, CRC(0d78ebc6) SHA1(0a0c1e23eb4d1748c4e6c448284d785286c77911) )	/* Blue + Green palette */
	ROM_LOAD( "prmcolr.2m",	0x0800, 0x0800, CRC(03f7241f) SHA1(16ae059f084ba0ac4ddaa95dbeed113295f106ea) )	/* Red palette */
ROM_END

ROM_START( baraduka )
	ROM_REGION( 0x10000, REGION_CPU1, 0 ) /* 6809 code */
	ROM_LOAD( "prg1.9c",	0x6000, 0x02000, CRC(ea2ea790) SHA1(ab6f523803b2b0ea04b78f2f252de6c2d344a26c) )
	ROM_LOAD( "bd1_1.9a",	0x8000, 0x04000, CRC(4e9f2bdc) SHA1(bc6e71d4d3b2064e662a105c1a77d2731070d58e) )
	ROM_LOAD( "bd1_2.9b",	0xc000, 0x04000, CRC(40617fcd) SHA1(51d17f3a2fc96e13c8ef5952efece526e0fb33f4) )

	ROM_REGION(  0x10000 , REGION_CPU2, 0 ) /* MCU code */
	ROM_LOAD( "bd1_4b.3b",	0x8000,  0x4000, CRC(a47ecd32) SHA1(a2a75e65deb28224a5729ed134ee4d5ea8c50706) )	/* subprogram for the MCU */
	ROM_LOAD( "pl1-mcu.bin",0xf000,	 0x1000, CRC(6ef08fb3) SHA1(4842590d60035a0059b0899eb2d5f58ae72c2529) )	/* The MCU internal code is missing */
															/* Using Pacland code (probably similar) */
	ROM_REGION( 0x02000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "ch1.3j",		0x00000, 0x2000, CRC(706b7fee) SHA1(e5694289bd4346c1a3a004feaa940710cea755c6) )	/* characters */

	ROM_REGION( 0x0c000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "ch2.4p",		0x00000, 0x4000, CRC(b0bb0710) SHA1(797832aea59bf80342fd2a3505645f2766bde65b) )	/* tiles */
	ROM_LOAD( "ch3.4n",		0x04000, 0x4000, CRC(0d7ebec9) SHA1(6b86b476db61f5760bc8610b51adc1115cfdad96) )
	ROM_LOAD( "ch4.4m",		0x08000, 0x4000, CRC(e5da0896) SHA1(abb8bf7e9dc1c60bc0a20a691109fb145bb1d8e0) )

	ROM_REGION( 0x10000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "obj1.8k",	0x00000, 0x4000, CRC(87a29acc) SHA1(3aa00efc95d1da50f6e4637d101640328287dea1) )	/* sprites */
	ROM_LOAD( "obj2.8l",	0x04000, 0x4000, CRC(72b6d20c) SHA1(e40b48dacefce4fd62ab28d3e6ff3932d4ff005b) )
	ROM_LOAD( "obj3.8m",	0x08000, 0x4000, CRC(3076af9c) SHA1(57ce09b298fd0bae94e4d8c817a34ce812c3ddfc) )
	ROM_LOAD( "obj4.8n",	0x0c000, 0x4000, CRC(8b4c09a3) SHA1(46e0ef39cb313c6780f6137769153dc4a054c77f) )

	ROM_REGION( 0x1000, REGION_PROMS, 0 )
	ROM_LOAD( "prmcolbg.1n",0x0000, 0x0800, CRC(0d78ebc6) SHA1(0a0c1e23eb4d1748c4e6c448284d785286c77911) )	/* Blue + Green palette */
	ROM_LOAD( "prmcolr.2m",	0x0800, 0x0800, CRC(03f7241f) SHA1(16ae059f084ba0ac4ddaa95dbeed113295f106ea) )	/* Red palette */
ROM_END

ROM_START( metrocrs )
	ROM_REGION( 0x10000, REGION_CPU1, 0 ) /* 6809 code */
	ROM_LOAD( "mc1-3.9c",	0x6000, 0x02000, CRC(3390b33c) SHA1(0733aece368acc913e2ff32e8817194cb4b630fb) )
	ROM_LOAD( "mc1-1.9a",	0x8000, 0x04000, CRC(10b0977e) SHA1(6266d173b55075da1f252092bf38185880bc4969) )
	ROM_LOAD( "mc1-2.9b",	0xc000, 0x04000, CRC(5c846f35) SHA1(3c98a0f1131f2e2477fc75a588123c57ff5350ad) )

	ROM_REGION(  0x10000 , REGION_CPU2, 0 ) /* MCU code */
	ROM_LOAD( "mc1-4.3b",	0x8000, 0x02000, CRC(9c88f898) SHA1(d6d0345002b70c5aca41c664f34181715cd87669) )	/* subprogram for the MCU */
	ROM_LOAD( "pl1-mcu.bin",0xf000,	 0x1000, CRC(6ef08fb3) SHA1(4842590d60035a0059b0899eb2d5f58ae72c2529) )	/* The MCU internal code is missing */
															/* Using Pacland code (probably similar) */
	ROM_REGION( 0x02000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "mc1-5.3j",	0x00000, 0x2000, CRC(9b5ea33a) SHA1(a8108e71e3440b645ebdb5cdbd87712151299789) )	/* characters */

	ROM_REGION( 0x0c000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "mc1-7.4p",	0x00000, 0x4000, CRC(c9dfa003) SHA1(86e8f9fc25de67691ce5385d93b723e7eb836b2b) )	/* tiles */
	ROM_LOAD( "mc1-6.4n",	0x04000, 0x4000, CRC(9686dc3c) SHA1(1caf712eedb1f70559169685e5421e11866e518c) )
	/* empty space to decode the roms as 3bpp */

	ROM_REGION( 0x10000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "mc1-8.8k",	0x00000, 0x4000, CRC(265b31fa) SHA1(d46e6db5d6f325954d2b6159157b11e10fe5838d) )	/* sprites */
	ROM_LOAD( "mc1-9.8l",	0x04000, 0x4000, CRC(541ec029) SHA1(a3096d8405b6bbc862b03773889f6cbd43739f5b) )
	/* 8000-ffff empty */

	ROM_REGION( 0x1000, REGION_PROMS, 0 )
	ROM_LOAD( "mc1-1.1n",	0x0000, 0x0800, CRC(32a78a8b) SHA1(545a59bc3c5868ac1749d2947210110205fb3da2) )	/* Blue + Green palette */
	ROM_LOAD( "mc1-2.2m",	0x0800, 0x0800, CRC(6f4dca7b) SHA1(781134c02853aded2cba63719c0e4c78b227da1c) )	/* Red palette */
ROM_END

ROM_START( metrocra )
	ROM_REGION( 0x10000, REGION_CPU1, 0 ) /* 6809 code */
	ROM_LOAD( "mc2-3.9b",	0x6000, 0x02000, CRC(ffe08075) SHA1(4e1341d5a9a58f171e1e6f9aa18092d5557a6947) )
	ROM_LOAD( "mc2-1.9a",	0x8000, 0x04000, CRC(05a239ea) SHA1(3e7c7d305d0f48e2431d60b176a0cb451ddc4637) )
	ROM_LOAD( "mc2-2.9a",	0xc000, 0x04000, CRC(db9b0e6d) SHA1(2772b59fe7dc0e78ee29dd001a6bba75b94e0334) )

	ROM_REGION(  0x10000 , REGION_CPU2, 0 ) /* MCU code */
	ROM_LOAD( "mc1-4.3b",	0x8000, 0x02000, CRC(9c88f898) SHA1(d6d0345002b70c5aca41c664f34181715cd87669) )	/* subprogram for the MCU */
	ROM_LOAD( "pl1-mcu.bin",0xf000,	 0x1000, CRC(6ef08fb3) SHA1(4842590d60035a0059b0899eb2d5f58ae72c2529) )	/* The MCU internal code is missing */
															/* Using Pacland code (probably similar) */
	ROM_REGION( 0x02000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "mc1-5.3j",	0x00000, 0x2000, CRC(9b5ea33a) SHA1(a8108e71e3440b645ebdb5cdbd87712151299789) )	/* characters */

	ROM_REGION( 0x0c000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "mc1-7.4p",	0x00000, 0x4000, CRC(c9dfa003) SHA1(86e8f9fc25de67691ce5385d93b723e7eb836b2b) )	/* tiles */
	ROM_LOAD( "mc1-6.4n",	0x04000, 0x4000, CRC(9686dc3c) SHA1(1caf712eedb1f70559169685e5421e11866e518c) )
	/* empty space to decode the roms as 3bpp */

	ROM_REGION( 0x10000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "mc1-8.8k",	0x00000, 0x4000, CRC(265b31fa) SHA1(d46e6db5d6f325954d2b6159157b11e10fe5838d) )	/* sprites */
	ROM_LOAD( "mc1-9.8l",	0x04000, 0x4000, CRC(541ec029) SHA1(a3096d8405b6bbc862b03773889f6cbd43739f5b) )
	/* 8000-ffff empty */

	ROM_REGION( 0x1000, REGION_PROMS, 0 )
	ROM_LOAD( "mc1-1.1n",	0x0000, 0x0800, CRC(32a78a8b) SHA1(545a59bc3c5868ac1749d2947210110205fb3da2) )	/* Blue + Green palette */
	ROM_LOAD( "mc1-2.2m",	0x0800, 0x0800, CRC(6f4dca7b) SHA1(781134c02853aded2cba63719c0e4c78b227da1c) )	/* Red palette */
ROM_END

static DRIVER_INIT( metrocrs )
{
	int i;
	unsigned char *rom = memory_region(REGION_GFX2);

	for(i = 0x8000;i < memory_region_length(REGION_GFX2);i++)
		rom[i] = 0xff;
}



GAME( 1985, baraduke, 0,        baraduke, baraduke, 0,        ROT0, "Namco", "Baraduke (set 1)" )
GAME( 1985, baraduka, baraduke, baraduke, baraduke, 0,        ROT0, "Namco", "Baraduke (set 2)" )
GAME( 1985, metrocrs, 0,        metrocrs, metrocrs, metrocrs, ROT0, "Namco", "Metro-Cross (set 1)" )
GAME( 1985, metrocra, metrocrs, metrocrs, metrocrs, metrocrs, ROT0, "Namco", "Metro-Cross (set 2)" )
