/***************************************************************************

Argus (Early NMK driver 1986-1987)
-------------------------------------
driver by Yochizo


Special thanks to :
=====================
 - Gerardo Oporto Jorrin for dipswitch informations.
 - Suzuki2go for screenshots of Argus and Valtric.
 - Jarek Parchanski for Psychic5 driver.


Supported games :
==================
 Argus      (C) 1986 NMK / Jaleco
 Valtric    (C) 1986 NMK / Jaleco
 Butasan    (C) 1987 NMK / Jaleco


System specs :
===============
 Argus
 ---------------------------------------------------------------
   CPU    : Z80 (4MHz) + Z80 (4MHz, Sound)
   Sound  : YM2203 x 1
   Layers : BG0, BG1, Sprite, Text [BG0 is controlled by VROMs]
   Colors : 832 colors
             Sprite : 128 colors
             BG0    : 256 colors
             BG1    : 256 colors
             Text   : 256 colors
   Others : Brightness controller  (Emulated)
            Half transparent color (Not emulated)

 Valtric
 ---------------------------------------------------------------
   CPU    : Z80 (5MHz) + Z80 (5MHz, Sound)
   Sound  : YM2203 x 2
   Layers : BG1, Sprite, Text
   Colors : 768 colors
             Sprite : 256 colors
             BG1    : 256 colors
             Text   : 256 colors
   Others : Brightness controller  (Emulated)
            Half transparent color (Not emulated)
            Mosaic effect          (Not emulated)

 Butasan
 ---------------------------------------------------------------
   CPU    : Z80 (5MHz) + Z80 (5MHz, Sound)
   Sound  : YM2203 x 2
   Layers : BG0, BG1, Sprite, Text [BG0 and BG1 is not shown simultaneously]
   Colors : 672 colors
             Sprite : 16x4 + 8x8 = 128 colors
             BG0    : 256 colors
             BG1    : 32 colors
             Text   : 256 colors
   Others : 2 VRAM pages           (Emulated)
            Various sprite sizes   (Emulated)


Note :
=======
 - To enter test mode, press coin 2 key at start in Argus and Valtric.


Known issues :
===============
 - Mosaic effect in Valtric is not implemented.
 - Half transparent color (50% alpha blending) is not emulated.
 - Sprite priority switch of Butasan is shown in test mode. What will be
   happened when set it ? JFF is not implemented this mistery switch too.
 - In Butasan, text layer will corrupt completely when you take a special
   item.
 - Data proms of Butasan does exist. But I don't know what is used for.
 - Though clock speed of Argus is actually 4 MHz, major sprite problems
   are broken out in the middle of slowdown. So, it is set 5 MHz now.
 - Sprite locations of Argus delay around 1 or 2 frames when horizontal
   scroll occurs.

****************************************************************************/


#include "driver.h"
#include "vidhrdw/generic.h"



/***************************************************************************

  Variables

***************************************************************************/

extern data8_t *argus_paletteram;
extern data8_t *argus_txram;
extern data8_t *argus_bg0ram;
extern data8_t *argus_bg0_scrollx;
extern data8_t *argus_bg0_scrolly;
extern data8_t *argus_bg1ram;
extern data8_t *argus_bg1_scrollx;
extern data8_t *argus_bg1_scrolly;
extern data8_t *butasan_bg1ram;

VIDEO_START( argus );
VIDEO_START( valtric );
VIDEO_START( butasan );
VIDEO_UPDATE( argus );
VIDEO_UPDATE( valtric );
VIDEO_UPDATE( butasan );

static data8_t argus_bank_latch   = 0x00;
static data8_t butasan_page_latch = 0x00;

READ_HANDLER( argus_txram_r );
READ_HANDLER( butasan_txram_r );
READ_HANDLER( argus_bg1ram_r );
READ_HANDLER( butasan_bg0ram_r );
READ_HANDLER( butasan_bg1ram_r );
READ_HANDLER( argus_paletteram_r );
READ_HANDLER( butasan_txbackram_r );
READ_HANDLER( butasan_bg0backram_r );

WRITE_HANDLER( argus_txram_w );
WRITE_HANDLER( butasan_txram_w );
WRITE_HANDLER( argus_bg1ram_w );
WRITE_HANDLER( butasan_bg0ram_w );
WRITE_HANDLER( butasan_bg1ram_w );
WRITE_HANDLER( argus_bg0_scrollx_w );
WRITE_HANDLER( argus_bg0_scrolly_w );
WRITE_HANDLER( butasan_bg0_scrollx_w );
WRITE_HANDLER( argus_bg1_scrollx_w );
WRITE_HANDLER( argus_bg1_scrolly_w );
WRITE_HANDLER( argus_bg_status_w );
WRITE_HANDLER( valtric_bg_status_w );
WRITE_HANDLER( butasan_bg0_status_w );
WRITE_HANDLER( argus_flipscreen_w );
WRITE_HANDLER( argus_paletteram_w );
WRITE_HANDLER( valtric_paletteram_w );
WRITE_HANDLER( butasan_paletteram_w );
WRITE_HANDLER( butasan_txbackram_w );
WRITE_HANDLER( butasan_bg0backram_w );
WRITE_HANDLER( butasan_bg1_status_w );

/***************************************************************************

  Interrupt(s)

***************************************************************************/

static INTERRUPT_GEN( argus_interrupt )
{
	if (cpu_getiloops() == 0)
	   cpu_set_irq_line_and_vector(0, 0, HOLD_LINE, 0xd7);	/* RST 10h */
	else
	   cpu_set_irq_line_and_vector(0, 0, HOLD_LINE, 0xcf);	/* RST 08h */
}

/* Handler called by the YM2203 emulator when the internal timers cause an IRQ */
static void irqhandler(int irq)
{
	cpu_set_irq_line(1, 0, irq ? ASSERT_LINE : CLEAR_LINE);
}

static struct YM2203interface argus_ym2203_interface =
{
	1,				/* 1 chip  */
	6000000 / 4, 	/* 1.5 MHz */
	{ YM2203_VOL(50,15) },
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 },
	{ irqhandler }
};

static struct YM2203interface valtric_ym2203_interface =
{
	2,				/* 2 chips */
	6000000 / 4, 	/* 1.5 MHz */
	{ YM2203_VOL(50,15), YM2203_VOL(50,15) },
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 },
	{ irqhandler }
};

/* Volume setting is different from the others. */
static struct YM2203interface butasan_ym2203_interface =
{
	2,				/* 2 chips */
	6000000 / 4, 	/* 1.5 MHz */
	{ YM2203_VOL(100,30), YM2203_VOL(100,30) },
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 },
	{ irqhandler }
};


/***************************************************************************

  Memory Handler(s)

***************************************************************************/

#if 0
static READ_HANDLER( argus_bankselect_r )
{
	return argus_bank_latch;
}
#endif

static WRITE_HANDLER( argus_bankselect_w )
{
	data8_t *RAM = memory_region(REGION_CPU1);
	int bankaddress;

	if (data != argus_bank_latch)
	{
		argus_bank_latch = data;
		bankaddress = 0x10000 + ((data & 7) * 0x4000);
		cpu_setbank(1, &RAM[bankaddress]);	 /* Select 8 banks of 16k */
	}
}

static WRITE_HANDLER( butasan_pageselect_w )
{
	butasan_page_latch = data;
}

static READ_HANDLER( butasan_pagedram_r )
{
	if (!(butasan_page_latch & 0x01))
	{
		if (offset < 0x0800)		/* BG0 RAM */
		{
			return butasan_bg0ram_r( offset );
		}
		else if (offset < 0x1000)	/* Back BG0 RAM */
		{
			return butasan_bg0backram_r( offset - 0x0800 );
		}
	}
	else
	{
		if (offset < 0x0800)		/* Text RAM */
		{
			return butasan_txram_r( offset );
		}
		else if (offset < 0x1000)	/* Back text RAM */
		{
			return butasan_txbackram_r( offset - 0x0800 );
		}
	}

	return 0;
}

static WRITE_HANDLER( butasan_pagedram_w )
{
	if (!(butasan_page_latch & 0x01))
	{
		if (offset < 0x0800)		/* BG0 RAM */
		{
			butasan_bg0ram_w( offset, data );
		}
		else if (offset < 0x1000)	/* Back BG0 RAM */
		{
			butasan_bg0backram_w( offset - 0x0800, data );
		}
	}

	else
	{
		if (offset < 0x0800)		/* Text RAM */
		{
			butasan_txram_w( offset, data );
		}
		else if (offset < 0x1000)	/* Back text RAM */
		{
			butasan_txbackram_w( offset - 0x0800, data );
		}
	}
}


/***************************************************************************

  Memory Map(s)

***************************************************************************/

static MEMORY_READ_START( argus_readmem )
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0xbfff, MRA_BANK1 },
	{ 0xc000, 0xc000, input_port_0_r },			/* Coin*/
	{ 0xc001, 0xc001, input_port_1_r },			/* Player 1*/
	{ 0xc002, 0xc002, input_port_2_r },			/* Player 2*/
	{ 0xc003, 0xc003, input_port_3_r },			/* DSW 1*/
	{ 0xc004, 0xc004, input_port_4_r },			/* DSW 2*/
	{ 0xc400, 0xcfff, argus_paletteram_r, },
	{ 0xd000, 0xd7ff, argus_txram_r },
	{ 0xd800, 0xdfff, argus_bg1ram_r },
	{ 0xe000, 0xf1ff, MRA_RAM },
	{ 0xf200, 0xf7ff, MRA_RAM },
	{ 0xf800, 0xffff, MRA_RAM },
MEMORY_END

static MEMORY_WRITE_START( argus_writemem )
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0x8000, 0xbfff, MWA_BANK1 },
	{ 0xc200, 0xc200, soundlatch_w },
	{ 0xc201, 0xc201, argus_flipscreen_w },
	{ 0xc202, 0xc202, argus_bankselect_w },
	{ 0xc300, 0xc301, argus_bg0_scrollx_w, &argus_bg0_scrollx },
	{ 0xc302, 0xc303, argus_bg0_scrolly_w, &argus_bg0_scrolly },
	{ 0xc308, 0xc309, argus_bg1_scrollx_w, &argus_bg1_scrollx },
	{ 0xc30a, 0xc30b, argus_bg1_scrolly_w, &argus_bg1_scrolly },
	{ 0xc30c, 0xc30c, argus_bg_status_w },
	{ 0xc400, 0xcfff, argus_paletteram_w, &argus_paletteram },
	{ 0xd000, 0xd7ff, argus_txram_w, &argus_txram },
	{ 0xd800, 0xdfff, argus_bg1ram_w, &argus_bg1ram },
	{ 0xe000, 0xf1ff, MWA_RAM },
	{ 0xf200, 0xf7ff, MWA_RAM, &spriteram, &spriteram_size },
	{ 0xf800, 0xffff, MWA_RAM },
MEMORY_END

static MEMORY_READ_START( valtric_readmem )
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0xbfff, MRA_BANK1 },
	{ 0xc000, 0xc000, input_port_0_r },			/* Coin*/
	{ 0xc001, 0xc001, input_port_1_r },			/* Player 1*/
	{ 0xc002, 0xc002, input_port_2_r },			/* Player 2*/
	{ 0xc003, 0xc003, input_port_3_r },			/* DSW 1*/
	{ 0xc004, 0xc004, input_port_4_r },			/* DSW 2*/
	{ 0xc400, 0xcfff, argus_paletteram_r, },
	{ 0xd000, 0xd7ff, argus_txram_r },
	{ 0xd800, 0xdfff, argus_bg1ram_r },
	{ 0xe000, 0xf1ff, MRA_RAM },
	{ 0xf200, 0xf7ff, MRA_RAM },
	{ 0xf800, 0xffff, MRA_RAM },
MEMORY_END

static MEMORY_WRITE_START( valtric_writemem )
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0x8000, 0xbfff, MWA_BANK1 },
	{ 0xc200, 0xc200, soundlatch_w },
	{ 0xc201, 0xc201, argus_flipscreen_w },
	{ 0xc202, 0xc202, argus_bankselect_w },
	{ 0xc308, 0xc309, argus_bg1_scrollx_w, &argus_bg1_scrollx },
	{ 0xc30a, 0xc30b, argus_bg1_scrolly_w, &argus_bg1_scrolly },
	{ 0xc30c, 0xc30c, valtric_bg_status_w },
	{ 0xc400, 0xcfff, valtric_paletteram_w, &argus_paletteram },
	{ 0xd000, 0xd7ff, argus_txram_w, &argus_txram },
	{ 0xd800, 0xdfff, argus_bg1ram_w, &argus_bg1ram },
	{ 0xe000, 0xf1ff, MWA_RAM },
	{ 0xf200, 0xf7ff, MWA_RAM, &spriteram, &spriteram_size },
	{ 0xf800, 0xffff, MWA_RAM },
MEMORY_END

static MEMORY_READ_START( butasan_readmem )
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0xbfff, MRA_BANK1 },
	{ 0xc000, 0xc000, input_port_0_r },			/* Coin*/
	{ 0xc001, 0xc001, input_port_1_r },			/* Player 1*/
	{ 0xc002, 0xc002, input_port_2_r },			/* Player 2*/
	{ 0xc003, 0xc003, input_port_3_r },			/* DSW 1*/
	{ 0xc004, 0xc004, input_port_4_r },			/* DSW 2*/
	{ 0xc400, 0xc7ff, butasan_bg1ram_r },
	{ 0xc800, 0xcfff, argus_paletteram_r },
	{ 0xd000, 0xdfff, butasan_pagedram_r },
	{ 0xe000, 0xefff, MRA_RAM },
	{ 0xf000, 0xf67f, MRA_RAM },
	{ 0xf680, 0xffff, MRA_RAM },
MEMORY_END

static MEMORY_WRITE_START( butasan_writemem )
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0x8000, 0xbfff, MWA_BANK1 },
	{ 0xc200, 0xc200, soundlatch_w },
	{ 0xc201, 0xc201, argus_flipscreen_w },
	{ 0xc202, 0xc202, argus_bankselect_w },
	{ 0xc203, 0xc203, butasan_pageselect_w },
	{ 0xc300, 0xc301, butasan_bg0_scrollx_w, &argus_bg0_scrollx },
	{ 0xc302, 0xc303, argus_bg0_scrolly_w, &argus_bg0_scrolly },
	{ 0xc304, 0xc304, butasan_bg0_status_w },
	{ 0xc308, 0xc309, argus_bg1_scrollx_w, &argus_bg1_scrollx },
	{ 0xc30a, 0xc30b, argus_bg1_scrolly_w, &argus_bg1_scrolly },
	{ 0xc30c, 0xc30c, butasan_bg1_status_w },
	{ 0xc400, 0xc7ff, butasan_bg1ram_w, &butasan_bg1ram },
	{ 0xc800, 0xcfff, butasan_paletteram_w, &argus_paletteram },
	{ 0xd000, 0xdfff, butasan_pagedram_w },
	{ 0xe000, 0xefff, MWA_RAM },
	{ 0xf000, 0xf67f, MWA_RAM, &spriteram, &spriteram_size },
	{ 0xf680, 0xffff, MWA_RAM },
MEMORY_END

static MEMORY_READ_START( sound_readmem_a )
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0x87ff, MRA_RAM },
	{ 0xc000, 0xc000, soundlatch_r },
MEMORY_END

static MEMORY_WRITE_START( sound_writemem_a )
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0x8000, 0x87ff, MWA_RAM },
MEMORY_END

static MEMORY_READ_START( sound_readmem_b )
	{ 0x0000, 0xbfff, MRA_ROM },
	{ 0xc000, 0xc7ff, MRA_RAM },
	{ 0xe000, 0xe000, soundlatch_r },
MEMORY_END

static MEMORY_WRITE_START( sound_writemem_b )
	{ 0x0000, 0xbfff, MWA_ROM },
	{ 0xc000, 0xc7ff, MWA_RAM },
MEMORY_END

static PORT_READ_START( sound_readport_1 )
	{ 0x0000, 0x0000, YM2203_status_port_0_r },
	{ 0x0001, 0x0001, YM2203_read_port_0_r },
PORT_END

static PORT_WRITE_START( sound_writeport_1 )
	{ 0x00, 0x00, YM2203_control_port_0_w },
	{ 0x01, 0x01, YM2203_write_port_0_w },
PORT_END

static PORT_READ_START( sound_readport_2 )
	{ 0x0000, 0x0000, YM2203_status_port_0_r },
	{ 0x0001, 0x0001, YM2203_read_port_0_r },
	{ 0x0080, 0x0080, YM2203_status_port_1_r },
	{ 0x0081, 0x0081, YM2203_read_port_1_r },
PORT_END

static PORT_WRITE_START( sound_writeport_2 )
	{ 0x00, 0x00, YM2203_control_port_0_w },
	{ 0x01, 0x01, YM2203_write_port_0_w },
	{ 0x80, 0x80, YM2203_control_port_1_w },
	{ 0x81, 0x81, YM2203_write_port_1_w },
PORT_END


/***************************************************************************

  Input Port(s)

***************************************************************************/

INPUT_PORTS_START( argus )
	PORT_START      /* System control (0) */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN1 )

	PORT_START      /* Player 1 control (1) */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* Player 2 controls (2) */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START  /* DSW 1 (3) */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x06, 0x06, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x04, "Easy" )
	PORT_DIPSETTING(    0x06, "Medium" )
	PORT_DIPSETTING(    0x02, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Free_Play ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x00, DEF_STR( Cabinet) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x20, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0xc0, 0xc0, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x80, "2" )
	PORT_DIPSETTING(    0xc0, "3" )
	PORT_DIPSETTING(    0x40, "4" )
	PORT_DIPSETTING(    0x00, "5" )

	PORT_START  /* DSW 2 (4) */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x1c, 0x1c, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x0C, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x1C, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x18, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x14, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 1C_4C ) )
	PORT_DIPNAME( 0xe0, 0xe0, DEF_STR( Coin_A) )
	PORT_DIPSETTING(    0x00, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x60, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0xe0, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0xa0, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 1C_4C ) )
INPUT_PORTS_END

INPUT_PORTS_START( valtric )
	PORT_START      /* System control (0) */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN1 )

	PORT_START      /* Player 1 control (1) */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START		/* Player 2 controls (2) */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START		/* DSW 1 (3) */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x06, 0x06, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x04, "Easy" )
	PORT_DIPSETTING(    0x06, "Medium" )
	PORT_DIPSETTING(    0x02, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Free_Play ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x00, DEF_STR( Cabinet) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x20, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0xc0, 0xc0, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "2" )
	PORT_DIPSETTING(    0xc0, "3" )
	PORT_DIPSETTING(    0x80, "4" )
	PORT_DIPSETTING(    0x40, "5" )

	PORT_START		/* DSW 2 (4) */
	PORT_BITX(    0x01, 0x01, IPT_DIPSWITCH_NAME | IPF_CHEAT, "Invulnerability", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x1c, 0x1c, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x0C, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x1C, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x18, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x14, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 1C_4C ) )
	PORT_DIPNAME( 0xe0, 0xe0, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x60, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0xe0, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0xa0, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 1C_4C ) )
INPUT_PORTS_END

INPUT_PORTS_START( butasan )
	PORT_START      /* System control (0) */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_SERVICE( 0x20,	IP_ACTIVE_LOW )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN1 )

	PORT_START      /* Player 1 control (1) */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START		/* Player 2 controls (2) */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START		/* DSW 1 (3) */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Free_Play ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_BITX(    0x02, 0x02, IPT_DIPSWITCH_NAME | IPF_CHEAT, "Invulnerability", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x0c, "3" )
	PORT_DIPSETTING(    0x08, "4" )
	PORT_DIPSETTING(    0x04, "5" )
	PORT_DIPSETTING(    0x00, "6" )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x30, "Rank 1 (Medium)" )
	PORT_DIPSETTING(    0x20, "Rank 2" )
	PORT_DIPSETTING(    0x10, "Rank 3" )
	PORT_DIPSETTING(    0x00, "Rank 4" )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START		/* DSW 2 (4) */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x1c, 0x1c, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x1c, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x18, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x14, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 1C_4C ) )
	PORT_DIPNAME( 0xe0, 0xe0, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x60, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0xe0, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0xa0, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 1C_4C ) )
INPUT_PORTS_END


/***************************************************************************

  Machine Driver(s)

***************************************************************************/

static struct GfxLayout charlayout =
{
	8,8,    /* 8x8 characters */
	1024,	/* 1024 characters */
	4,      /* 4 bits per pixel */
	{ 0, 1, 2, 3 },
	{ 0, 4, 8, 12, 16, 20, 24, 28 },
	{ 0*8, 4*8, 8*8, 12*8, 16*8, 20*8, 24*8, 28*8 },
	32*8
};

static struct GfxLayout tilelayout_256 =
{
	16,16,  /* 16x16 characters */
	256,	/* 256 characters */
	4,      /* 4 bits per pixel */
	{ 0, 1, 2, 3 },
	{ 0, 4, 8, 12, 16, 20, 24, 28,
		64*8, 64*8+4, 64*8+8, 64*8+12, 64*8+16, 64*8+20, 64*8+24, 64*8+28 },
	{ 0*8, 4*8, 8*8, 12*8, 16*8, 20*8, 24*8, 28*8,
		32*8, 36*8, 40*8, 44*8, 48*8, 52*8, 56*8, 60*8 },
	128*8
};

static struct GfxLayout tilelayout_512 =
{
	16,16,  /* 16x16 characters */
	512,	/* 512 characters */
	4,      /* 4 bits per pixel */
	{ 0, 1, 2, 3 },
	{ 0, 4, 8, 12, 16, 20, 24, 28,
		64*8, 64*8+4, 64*8+8, 64*8+12, 64*8+16, 64*8+20, 64*8+24, 64*8+28 },
	{ 0*8, 4*8, 8*8, 12*8, 16*8, 20*8, 24*8, 28*8,
		32*8, 36*8, 40*8, 44*8, 48*8, 52*8, 56*8, 60*8 },
	128*8
};

static struct GfxLayout tilelayout_1024 =
{
	16,16,  /* 16x16 characters */
	1024,	/* 1024 characters */
	4,      /* 4 bits per pixel */
	{ 0, 1, 2, 3 },
	{ 0, 4, 8, 12, 16, 20, 24, 28,
		64*8, 64*8+4, 64*8+8, 64*8+12, 64*8+16, 64*8+20, 64*8+24, 64*8+28 },
	{ 0*8, 4*8, 8*8, 12*8, 16*8, 20*8, 24*8, 28*8,
		32*8, 36*8, 40*8, 44*8, 48*8, 52*8, 56*8, 60*8 },
	128*8
};

static struct GfxLayout tilelayout_2048 =
{
	16,16,  /* 16x16 characters */
	2048,	/* 2048 characters */
	4,      /* 4 bits per pixel */
	{ 0, 1, 2, 3 },
	{ 0, 4, 8, 12, 16, 20, 24, 28,
		64*8, 64*8+4, 64*8+8, 64*8+12, 64*8+16, 64*8+20, 64*8+24, 64*8+28 },
	{ 0*8, 4*8, 8*8, 12*8, 16*8, 20*8, 24*8, 28*8,
		32*8, 36*8, 40*8, 44*8, 48*8, 52*8, 56*8, 60*8 },
	128*8
};

static struct GfxLayout tilelayout_4096 =
{
	16,16,  /* 16x16 characters */
	4096,	/* 4096 characters */
	4,      /* 4 bits per pixel */
	{ 0, 1, 2, 3 },
	{ 0, 4, 8, 12, 16, 20, 24, 28,
		64*8, 64*8+4, 64*8+8, 64*8+12, 64*8+16, 64*8+20, 64*8+24, 64*8+28 },
	{ 0*8, 4*8, 8*8, 12*8, 16*8, 20*8, 24*8, 28*8,
		32*8, 36*8, 40*8, 44*8, 48*8, 52*8, 56*8, 60*8 },
	128*8
};

static struct GfxDecodeInfo argus_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &tilelayout_1024, 0*16,   8 },
	{ REGION_GFX2, 0, &tilelayout_1024, 8*16,  16 },
	{ REGION_GFX3, 0, &tilelayout_256,  24*16, 16 },
	{ REGION_GFX4, 0, &charlayout,      40*16, 16 },
	{ -1 } /* end of array */
};

static struct GfxDecodeInfo valtric_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &tilelayout_1024, 0*16, 16 },
	{ REGION_GFX2, 0, &tilelayout_2048, 16*16, 16 },
	{ REGION_GFX3, 0, &charlayout,      32*16, 16 },
	{ -1 } /* end of array */
};

static struct GfxDecodeInfo butasan_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &tilelayout_4096, 0*16,  16 },
	{ REGION_GFX2, 0, &tilelayout_1024, 16*16, 16 },
	{ REGION_GFX3, 0, &tilelayout_512,  12*16, 16 },
	{ REGION_GFX4, 0, &charlayout,      32*16, 16 },
	{ -1 } /* end of array */
};

static MACHINE_DRIVER_START( argus )

	/* basic machine hardware */
	MDRV_CPU_ADD(Z80, 5000000)			/* 4 MHz */
	MDRV_CPU_MEMORY(argus_readmem,argus_writemem)
	MDRV_CPU_VBLANK_INT(argus_interrupt,2)

	MDRV_CPU_ADD(Z80, 5000000)
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)			/* 4 MHz */
	MDRV_CPU_MEMORY(sound_readmem_a,sound_writemem_a)
	MDRV_CPU_PORTS(sound_readport_1,sound_writeport_1)

	MDRV_FRAMES_PER_SECOND(54)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)	/* This value is refered to psychic5 driver */
	MDRV_INTERLEAVE(10)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(32*16, 32*16)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 2*8, 30*8-1)
	MDRV_GFXDECODE(argus_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(896)

	MDRV_VIDEO_START(argus)
	MDRV_VIDEO_UPDATE(argus)

	/* sound hardware */
	MDRV_SOUND_ADD(YM2203, argus_ym2203_interface)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( valtric )

	/* basic machine hardware */
	MDRV_CPU_ADD(Z80, 5000000)			/* 5 MHz */
	MDRV_CPU_MEMORY(valtric_readmem,valtric_writemem)
	MDRV_CPU_VBLANK_INT(argus_interrupt,2)

	MDRV_CPU_ADD(Z80, 5000000)
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)			/* 5 MHz */
	MDRV_CPU_MEMORY(sound_readmem_a,sound_writemem_a)
	MDRV_CPU_PORTS(sound_readport_2,sound_writeport_2)

	MDRV_FRAMES_PER_SECOND(54)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)	/* This value is refered to psychic5 driver */
	MDRV_INTERLEAVE(10)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(32*16, 32*16)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 2*8, 30*8-1)
	MDRV_GFXDECODE(valtric_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(768)

	MDRV_VIDEO_START(valtric)
	MDRV_VIDEO_UPDATE(valtric)

	/* sound hardware */
	MDRV_SOUND_ADD(YM2203, valtric_ym2203_interface)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( butasan )

	/* basic machine hardware */
	MDRV_CPU_ADD(Z80, 5000000)			/* 5 MHz */
	MDRV_CPU_MEMORY(butasan_readmem,butasan_writemem)
	MDRV_CPU_VBLANK_INT(argus_interrupt,2)

	MDRV_CPU_ADD(Z80, 5000000)
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)			/* 5 MHz */
	MDRV_CPU_MEMORY(sound_readmem_b,sound_writemem_b)
	MDRV_CPU_PORTS(sound_readport_2,sound_writeport_2)

	MDRV_FRAMES_PER_SECOND(54)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)	/* This value is refered to psychic5 driver */
	MDRV_INTERLEAVE(10)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(32*16, 32*16)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 1*8, 31*8-1)
	MDRV_GFXDECODE(butasan_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(768)

	MDRV_VIDEO_START(butasan)
	MDRV_VIDEO_UPDATE(butasan)

	/* sound hardware */
	MDRV_SOUND_ADD(YM2203, butasan_ym2203_interface)
MACHINE_DRIVER_END


/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( argus )
	ROM_REGION( 0x28000, REGION_CPU1, 0 ) 					/* Main CPU */
	ROM_LOAD( "ag_02.bin", 0x00000, 0x08000, CRC(278a3f3d) SHA1(c5ac5a004ebf0194c33f71dab4020fa636cefbc2) )
	ROM_LOAD( "ag_03.bin", 0x10000, 0x08000, CRC(3a7f3bfa) SHA1(b11e134c084fc3c982dfe31836c1cf3fc0d481fd) )
	ROM_LOAD( "ag_04.bin", 0x18000, 0x08000, CRC(76adc9f6) SHA1(e223a8b2371c51f121958ee3687c777f597334c9) )
	ROM_LOAD( "ag_05.bin", 0x20000, 0x08000, CRC(f76692d6) SHA1(1dc353a042cdda909eb9f1b1ca749a3b3eaa01e4) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )					/* Sound CPU */
	ROM_LOAD( "ag_01.bin", 0x00000, 0x04000, CRC(769e3f57) SHA1(209160a96486ab0b90967c015143ec28fba2e2a4) )

	ROM_REGION( 0x20000, REGION_GFX1, ROMREGION_DISPOSE )	/* Sprite */
	ROM_LOAD( "ag_09.bin", 0x00000, 0x08000, CRC(6dbc1c58) SHA1(ef7b6901b702dd347b3a3f162162138175efe578) )
	ROM_LOAD( "ag_08.bin", 0x08000, 0x08000, CRC(ce6e987e) SHA1(9de257d8061ec917f4d443ff509fd457f995d73b) )
	ROM_LOAD( "ag_07.bin", 0x10000, 0x08000, CRC(bbb9638d) SHA1(61dec71d4d976bef3af26d0dc9c0355fd1098ffb) )
	ROM_LOAD( "ag_06.bin", 0x18000, 0x08000, CRC(655b48f8) SHA1(4fce1dffe091b97e7055955743434e49e97b4b79) )

	ROM_REGION( 0x20000, REGION_GFX2, ROMREGION_DISPOSE )	/* BG0 */
	ROM_LOAD( "ag_13.bin", 0x00000, 0x08000, CRC(20274268) SHA1(9b7767d14bd169dabe6add0623d353bf4b59779b) )
	ROM_LOAD( "ag_14.bin", 0x08000, 0x08000, CRC(ceb8860b) SHA1(90e094686d9d18e49e4848d18d1e31ac95f13937) )
	ROM_LOAD( "ag_11.bin", 0x10000, 0x08000, CRC(99ce8556) SHA1(39caababd6e20ecb0375b85fb6490ee0b04f0949) )
	ROM_LOAD( "ag_12.bin", 0x18000, 0x08000, CRC(e0e5377c) SHA1(b5981d832127d0b28b6a7bb0437716593e0ed71a) )

	ROM_REGION( 0x08000, REGION_GFX3, ROMREGION_DISPOSE )	/* BG1 */
	ROM_LOAD( "ag_17.bin", 0x00000, 0x08000, CRC(0f12d09b) SHA1(718db4ff016526dddacdf6f0088f247ee97c6543) )

	ROM_REGION( 0x08000, REGION_GFX4, ROMREGION_DISPOSE )	/* Text */
	ROM_LOAD( "ag_10.bin", 0x00000, 0x04000, CRC(2de696c4) SHA1(1ad0f1cde127a1618c2ea74a53e522963a79e5ce) )

	ROM_REGION( 0x08000, REGION_USER1, 0 )					/* Map */
	ROM_LOAD( "ag_15.bin", 0x00000, 0x08000, CRC(99834c1b) SHA1(330f271771b158493b28bb178c8cda98efd1d90c) )

	ROM_REGION( 0x08000, REGION_USER2, 0 )					/* Pattern */
	ROM_LOAD( "ag_16.bin", 0x00000, 0x08000, CRC(39a51714) SHA1(ad89a630f1352eb4d8beeeebf909d5e2b5d7cc12) )
ROM_END

ROM_START( valtric )
	ROM_REGION( 0x30000, REGION_CPU1, 0 ) 					/* Main CPU */
	ROM_LOAD( "vt_04.bin",    0x00000, 0x08000, CRC(709c705f) SHA1(b82e2209a0371dcbc2708c485b02985cea04353f) )
	ROM_LOAD( "vt_06.bin",    0x10000, 0x10000, CRC(c9cbb4e4) SHA1(3c84cda778263a9bb2031e29f6f29f29878d2070) )
	ROM_LOAD( "vt_05.bin",    0x20000, 0x10000, CRC(7ab2684b) SHA1(9bca7e2fd3b5f4043de37cd439d5235957e5012f) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )					/* Sound CPU */
	ROM_LOAD( "vt_01.bin",    0x00000, 0x08000, CRC(4616484f) SHA1(24d060218cc1542ebfc2100ecd6489a0e17b36ee) )

	ROM_REGION( 0x20000, REGION_GFX1, ROMREGION_DISPOSE )	/* Sprite */
	ROM_LOAD( "vt_02.bin",    0x00000, 0x10000, CRC(66401977) SHA1(91c527d0bcea54d723068715a12cb3c976d04294) )
	ROM_LOAD( "vt_03.bin",    0x10000, 0x10000, CRC(9203bbce) SHA1(f40cee48f62a87a0b5d18e271faa5b8dd36ae5f1) )

	ROM_REGION( 0x40000, REGION_GFX2, ROMREGION_DISPOSE )	/* BG */
	ROM_LOAD( "vt_08.bin",    0x00000, 0x10000, CRC(661dd338) SHA1(cc643a14607c10e4a1710766f77422cd89a6bf94) )
	ROM_LOAD( "vt_09.bin",    0x10000, 0x10000, CRC(085a35b1) SHA1(ff589e67b6b5a6e661f29294a32a3840f45a9304) )
	ROM_LOAD( "vt_10.bin",    0x20000, 0x10000, CRC(09c47323) SHA1(fcfbd5054e63fae00b6a3959228964ac8f3cbf37) )
	ROM_LOAD( "vt_11.bin",    0x30000, 0x10000, CRC(4cf800b5) SHA1(7241e284b15475d8a6d533e4caadd0acbf058231) )

	ROM_REGION( 0x08000, REGION_GFX3, ROMREGION_DISPOSE )	/* Text */
	ROM_LOAD( "vt_07.bin",    0x00000, 0x08000, CRC(d5f9bfb9) SHA1(6b3f11f9b8f76c0144a109f1506d8cbb01876237) )
ROM_END

ROM_START( butasan )
	ROM_REGION( 0x30000, REGION_CPU1, 0 ) 					/* Main CPU */
	ROM_LOAD( "buta-04.bin",  0x00000, 0x08000, CRC(47ff4ca9) SHA1(d89a41f6987c91d20b010f0cbda332cf54b21f8c) )
	ROM_LOAD( "buta-03.bin",  0x10000, 0x10000, CRC(69fd88c7) SHA1(fd827d7926a2de5ffe2982b3a59ea43de00ee46b) )
	ROM_LOAD( "buta-02.bin",  0x20000, 0x10000, CRC(519dc412) SHA1(48bbb01b217bd19c48ef7ab12c60805aaa02527c) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )					/* Sound CPU */
	ROM_LOAD( "buta-01.bin",  0x00000, 0x10000, CRC(c9d23e2d) SHA1(cee289d5bf7626fc35808a09f9f1f4628fa16974) )

	ROM_REGION( 0x80000, REGION_GFX1, ROMREGION_DISPOSE )	/* Sprite */
	ROM_LOAD( "buta-16.bin",  0x00000, 0x10000, CRC(e0ce51b6) SHA1(458c7c422a7b6ce42f397a8868610f6386fd815c) )
	ROM_LOAD( "buta-15.bin",  0x10000, 0x10000, CRC(3ed19daa) SHA1(b8090c3baa2b31681bed15c682a97c024e229df7) )
	ROM_LOAD( "buta-14.bin",  0x20000, 0x10000, CRC(8ec891c1) SHA1(e16f18a0eed300752af8f07fd3cef5cd825a2a05) )
	ROM_LOAD( "buta-13.bin",  0x30000, 0x10000, CRC(5023e74d) SHA1(edf43e6c89f0e537cebf1c21a671dba4cd7d91ea) )
	ROM_LOAD( "buta-12.bin",  0x40000, 0x10000, CRC(44f59905) SHA1(bf364f7f907fee551e9228db7c27c106bcfecf6c) )
	ROM_LOAD( "buta-11.bin",  0x50000, 0x10000, CRC(b8929f1d) SHA1(18a72f30284bed0c6723105f87eb10d64d3f461d) )
	ROM_LOAD( "buta-10.bin",  0x60000, 0x10000, CRC(fd4d3baf) SHA1(fa8e3970a8aac83efcb669fe5d4683adade9aa4f) )
	ROM_LOAD( "buta-09.bin",  0x70000, 0x10000, CRC(7da4c0fd) SHA1(fb2b148ccfee530313da886eddf7711ee83b4aeb) )

	ROM_REGION( 0x20000, REGION_GFX2, ROMREGION_DISPOSE )	/* BG0 */
	ROM_LOAD( "buta-05.bin",  0x00000, 0x10000, CRC(b8e026b0) SHA1(eb6ff9042b21b7190000c571ccba7d81f11ce9f1) )
	ROM_LOAD( "buta-06.bin",  0x10000, 0x10000, CRC(8bbacb81) SHA1(015be76e44ed2389eff912d8f61a757667d7670b) )

	ROM_REGION( 0x10000, REGION_GFX3, ROMREGION_DISPOSE )	/* BG1 */
	ROM_LOAD( "buta-07.bin",  0x00000, 0x10000, CRC(3a48d531) SHA1(0ff6256bb7ea909d95b2bfb994ebc5432ea6d055) )

	ROM_REGION( 0x08000, REGION_GFX4, ROMREGION_DISPOSE )	/* Text */
	ROM_LOAD( "buta-08.bin",  0x00000, 0x08000, CRC(5d45ce9c) SHA1(113c3e7ce20634ee4bb740705485572583298694) )

	ROM_REGION( 0x00200, REGION_PROMS, 0 )					/* Data proms ??? */
	ROM_LOAD( "buta-01.prm",  0x00000, 0x00100, CRC(45baedd0) SHA1(afdafb67d55007e6fb99518657e27ce61d2cb7e6) )
	ROM_LOAD( "buta-02.prm",  0x00100, 0x00100, CRC(0dcb18fc) SHA1(0b097b873c9484981f87a5e3d1af767f901ae05f) )
ROM_END


/*  ( YEAR   NAME     PARENT  MACHINE   INPUT     INIT  MONITOR  COMPANY                 FULLNAME ) */
GAME( 1986, argus,    0,      argus,    argus,    0,    ROT270,  "[NMK] (Jaleco license)", "Argus"           )
GAME( 1986, valtric,  0,      valtric,  valtric,  0,    ROT270,  "[NMK] (Jaleco license)", "Valtric"         )
GAME( 1987, butasan,  0,      butasan,  butasan,  0,    ROT0,    "[NMK] (Jaleco license)", "Butasan (Japan)" )
