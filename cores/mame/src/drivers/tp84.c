/***************************************************************************

Time Pilot 84  (c) 1984 Konami

driver by Marc Lafontaine

TODO:
- the slave CPU multiplexes sprites. We are cheating now, and reading them
  from somewhere else.


The schematics are available on the net.

There is 3 CPU for this game.
 Two 68A09E for the game.
 A Z80A for the sound

As I understand it, the second 6809 is for displaying
 the sprites. If we do not emulate him, all work well, except
 that the player cannot die.
 Address 57ff must read 0 to pass the RAM test if the second CPU
 is not emulated.


---- Master 6809 ------

Write
 2000-27ff MAFR Watch dog ?
 2800      COL0 a register that index the colors Proms
 3000      reset IRQ
 3001      OUT2  Coin Counter 2
 3002      OUT1  Coin Counter 1
 3003      MUT
 3004      HREV	 Flip Screen X
 3005      VREV	 Flip Screen Y
 3006      -
 3007      GMED
 3800      SON   Sound on
 3A00      SDA   Sound data
 3C00      SHF0 SHF1 J2 J3 J4 J5 J6 J7  background Y position
 3E00      L0 - L7                      background X position

Read:
 2800      in0  Buttons 1
 2820      in1  Buttons 2
 2840      in2  Buttons 3
 2860      in3  Dip switches 1
 3000      in4  Dip switches 2
 3800      in5  Dip switches 3 (not used)

Read/Write
 4000-47ff Char ram, 2 pages
 4800-4fff Background character ram, 2 pages
 5000-57ff Ram (Common for the Master and Slave 6809)  0x5000-0x517f sprites data
 6000-ffff Rom (only from $8000 to $ffff is used in this game)


------ Slave 6809 --------
 0000-1fff SAFR Watch dog ?
 2000      seem to be the beam position (if always 0, no player collision is detected)
 4000      enable or reset IRQ
 6000-67ff DRA
 8000-87ff Ram (Common for the Master and Slave 6809)
 E000-ffff Rom


------ Sound CPU (Z80) -----
There are 3 or 4 76489AN chips driven by the Z80

0000-1fff Rom program (A6)
2000-3fff Rom Program (A4) (not used or missing?)
4000-43ff Ram
6000-7fff Sound data in
8000-9fff Timer
A000-Bfff Filters
C000      Store Data that will go to one of the 76489AN
C001      76489 #1 trigger
C002      76489 #2 (optional) trigger
C003      76489 #3 trigger
C004      76489 #4 trigger

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"


extern UINT8 *tp84_videoram2, *tp84_colorram2;

extern WRITE_HANDLER( tp84_videoram_w );
extern WRITE_HANDLER( tp84_colorram_w );
extern WRITE_HANDLER( tp84_videoram2_w );
extern WRITE_HANDLER( tp84_colorram2_w );
extern WRITE_HANDLER( tp84_scroll_x_w );
extern WRITE_HANDLER( tp84_scroll_y_w );
extern WRITE_HANDLER( tp84_flipscreen_x_w );
extern WRITE_HANDLER( tp84_flipscreen_y_w );
extern WRITE_HANDLER( tp84_col0_w );
extern READ_HANDLER( tp84_scanline_r );

extern PALETTE_INIT( tp84 );
extern VIDEO_START( tp84 );
extern VIDEO_UPDATE( tp84 );

extern INTERRUPT_GEN( tp84_6809_interrupt );


static UINT8 *sharedram;

static READ_HANDLER( sharedram_r )
{
	return sharedram[offset];
}

static WRITE_HANDLER( sharedram_w )
{
	sharedram[offset] = data;
}



static READ_HANDLER( tp84_sh_timer_r )
{
	/* main xtal 14.318MHz, divided by 4 to get the CPU clock, further */
	/* divided by 2048 to get this timer */
	/* (divide by (2048/2), and not 1024, because the CPU cycle counter is */
	/* incremented every other state change of the clock) */
	return (activecpu_gettotalcycles() / (2048/2)) & 0x0f;
}

static WRITE_HANDLER( tp84_filter_w )
{
	int C;

	/* 76489 #0 */
	C = 0;
	if (offset & 0x008) C +=  47000;	/*  47000pF = 0.047uF */
	if (offset & 0x010) C += 470000;	/* 470000pF = 0.47uF */
	set_RC_filter(0,1000,2200,1000,C);

	/* 76489 #1 (optional) */
	C = 0;
	if (offset & 0x020) C +=  47000;	/*  47000pF = 0.047uF */
	if (offset & 0x040) C += 470000;	/* 470000pF = 0.47uF */
/*	set_RC_filter(1,1000,2200,1000,C);*/

	/* 76489 #2 */
	C = 0;
	if (offset & 0x080) C += 470000;	/* 470000pF = 0.47uF */
	set_RC_filter(1,1000,2200,1000,C);

	/* 76489 #3 */
	C = 0;
	if (offset & 0x100) C += 470000;	/* 470000pF = 0.47uF */
	set_RC_filter(2,1000,2200,1000,C);
}

static WRITE_HANDLER( tp84_sh_irqtrigger_w )
{
	cpu_set_irq_line_and_vector(2,0,HOLD_LINE,0xff);
}



/* CPU 1 read addresses */
static MEMORY_READ_START( readmem )
	{ 0x2800, 0x2800, input_port_0_r },
	{ 0x2820, 0x2820, input_port_1_r },
	{ 0x2840, 0x2840, input_port_2_r },
	{ 0x2860, 0x2860, input_port_3_r },
	{ 0x3000, 0x3000, input_port_4_r },
	{ 0x4000, 0x4fff, MRA_RAM },
	{ 0x5000, 0x57ff, sharedram_r },
	{ 0x8000, 0xffff, MRA_ROM },
MEMORY_END

/* CPU 1 write addresses */
static MEMORY_WRITE_START( writemem )
	{ 0x2000, 0x2000, watchdog_reset_w },
	{ 0x2800, 0x2800, tp84_col0_w },
	{ 0x3000, 0x3000, MWA_RAM },
	{ 0x3004, 0x3004, tp84_flipscreen_x_w },
	{ 0x3005, 0x3005, tp84_flipscreen_y_w },
	{ 0x3800, 0x3800, tp84_sh_irqtrigger_w },
	{ 0x3a00, 0x3a00, soundlatch_w },
	{ 0x3c00, 0x3c00, tp84_scroll_x_w },
	{ 0x3e00, 0x3e00, tp84_scroll_y_w },
	{ 0x4000, 0x43ff, tp84_videoram_w, &videoram },
	{ 0x4400, 0x47ff, tp84_videoram2_w, &tp84_videoram2 },
	{ 0x4800, 0x4bff, tp84_colorram_w, &colorram },
	{ 0x4c00, 0x4fff, tp84_colorram2_w, &tp84_colorram2 },
	{ 0x5000, 0x57ff, sharedram_w, &sharedram },
	{ 0x8000, 0xffff, MWA_ROM },
MEMORY_END


/* CPU 2 read addresses */
static MEMORY_READ_START( readmem_cpu2 )
/*	{ 0x0000, 0x0000, MRA_RAM },*/
	{ 0x2000, 0x2000, tp84_scanline_r }, /* beam position */
	{ 0x6000, 0x67ff, MRA_RAM },
	{ 0x8000, 0x87ff, sharedram_r },
	{ 0xe000, 0xffff, MRA_ROM },
MEMORY_END

/* CPU 2 write addresses */
static MEMORY_WRITE_START( writemem_cpu2 )
/*	{ 0x0000, 0x0000, MWA_RAM },  // Watch dog ?/*/
	{ 0x4000, 0x4000, interrupt_enable_w }, /* IRQ enable */
	{ 0x6000, 0x679f, MWA_RAM },
	{ 0x67a0, 0x67ff, MWA_RAM, &spriteram, &spriteram_size },	/* REAL (multiplexed) */
	{ 0x8000, 0x87ff, sharedram_w },
	{ 0xe000, 0xffff, MWA_ROM },
MEMORY_END


static MEMORY_READ_START( sound_readmem )
	{ 0x0000, 0x3fff, MRA_ROM },
	{ 0x4000, 0x43ff, MRA_RAM },
	{ 0x6000, 0x6000, soundlatch_r },
	{ 0x8000, 0x8000, tp84_sh_timer_r },
MEMORY_END

static MEMORY_WRITE_START( sound_writemem )
	{ 0x0000, 0x3fff, MWA_ROM },
	{ 0x4000, 0x43ff, MWA_RAM },
	{ 0xa000, 0xa1ff, tp84_filter_w },
	{ 0xc000, 0xc000, MWA_NOP },
	{ 0xc001, 0xc001, SN76496_0_w },
	{ 0xc003, 0xc003, SN76496_1_w },
	{ 0xc004, 0xc004, SN76496_2_w },
MEMORY_END



INPUT_PORTS_START( tp84 )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* DSW0 */
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
	PORT_DIPSETTING(    0x00, "Invalid" )

	PORT_START      /* DSW1 */
	PORT_DIPNAME( 0x03, 0x02, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x03, "2" )
	PORT_DIPSETTING(    0x02, "3" )
	PORT_DIPSETTING(    0x01, "5" )
	PORT_DIPSETTING(    0x00, "7" )
	PORT_DIPNAME( 0x04, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x18, 0x10, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x18, "10000 and every 50000" )
	PORT_DIPSETTING(    0x10, "20000 and every 60000" )
	PORT_DIPSETTING(    0x08, "30000 and every 70000" )
	PORT_DIPSETTING(    0x00, "40000 and every 80000" )
	PORT_DIPNAME( 0x60, 0x20, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x60, "Easy" )
	PORT_DIPSETTING(    0x40, "Normal" )
	PORT_DIPSETTING(    0x20, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
INPUT_PORTS_END

INPUT_PORTS_START( tp84a )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* DSW0 */
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
	PORT_DIPSETTING(    0x00, "Invalid" )

	PORT_START      /* DSW1 */
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x03, "3" )
	PORT_DIPSETTING(    0x02, "4" )
	PORT_DIPSETTING(    0x01, "5" )
	PORT_DIPSETTING(    0x00, "7" )
	PORT_DIPNAME( 0x04, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x18, 0x10, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x18, "10000 and every 50000" )
	PORT_DIPSETTING(    0x10, "20000 and every 60000" )
	PORT_DIPSETTING(    0x08, "30000 and every 70000" )
	PORT_DIPSETTING(    0x00, "40000 and every 80000" )
	PORT_DIPNAME( 0x60, 0x20, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x60, "Easy" )
	PORT_DIPSETTING(    0x40, "Normal" )
	PORT_DIPSETTING(    0x20, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
INPUT_PORTS_END


static struct GfxLayout charlayout =
{
	8,8,
	RGN_FRAC(1,1),
	2,
	{ 4, 0 },
	{  0, 1, 2, 3, 8*8+0, 8*8+1, 8*8+2, 8*8+3 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	16*8
};
static struct GfxLayout spritelayout =
{
	16,16,
	RGN_FRAC(1,2),
	4,
	{ RGN_FRAC(1,2)+4, RGN_FRAC(1,2)+0, 4 ,0 },
	{ 0, 1, 2, 3, 8*8+0, 8*8+1, 8*8+2, 8*8+3,
			16*8+0, 16*8+1, 16*8+2, 16*8+3, 24*8+0, 24*8+1, 24*8+2, 24*8+3 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
			32*8, 33*8, 34*8, 35*8, 36*8, 37*8, 38*8, 39*8 },
	64*8
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &charlayout,        0, 64*8 },
	{ REGION_GFX2, 0, &spritelayout, 64*4*8, 16*8 },
	{ -1 } /* end of array */
};



static struct SN76496interface sn76496_interface =
{
	3,	/* 3 chips */
	{ 14318180/8, 14318180/8, 14318180/8 },
	{ 75, 75, 75 }
};



static MACHINE_DRIVER_START( tp84 )

	/* basic machine hardware */
	MDRV_CPU_ADD(M6809, 1500000)	/* ??? */
	MDRV_CPU_MEMORY(readmem,writemem)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,1)

	MDRV_CPU_ADD(M6809, 1500000)	/* ??? */
	MDRV_CPU_MEMORY(readmem_cpu2,writemem_cpu2)
	MDRV_CPU_VBLANK_INT(tp84_6809_interrupt,256)

	MDRV_CPU_ADD(Z80,14318180/4)
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
	MDRV_CPU_MEMORY(sound_readmem,sound_writemem)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(100)	/* 100 CPU slices per frame - an high value to ensure proper */
							/* synchronization of the CPUs */

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 2*8, 30*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(256)
	MDRV_COLORTABLE_LENGTH(4096)

	MDRV_PALETTE_INIT(tp84)
	MDRV_VIDEO_START(tp84)
	MDRV_VIDEO_UPDATE(tp84)

	/* sound hardware */
	MDRV_SOUND_ADD(SN76496, sn76496_interface)
MACHINE_DRIVER_END



/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( tp84 )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "tp84_7j.bin",  0x8000, 0x2000, CRC(605f61c7) SHA1(6848ef35ec7f92cccefb0fb2de42c4b0e9ec476f) )
	ROM_LOAD( "tp84_8j.bin",  0xa000, 0x2000, CRC(4b4629a4) SHA1(f3bb1ee66c9e47d050370ac9ca74f3020cb9cfa3) )
	ROM_LOAD( "tp84_9j.bin",  0xc000, 0x2000, CRC(dbd5333b) SHA1(65dee1fd4c940a5423d57cb55a7f2ad89c59c5c6) )
	ROM_LOAD( "tp84_10j.bin", 0xe000, 0x2000, CRC(a45237c4) SHA1(896e31c59aedf1c7e73e6f30fbe78cc020b457ab) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for the second CPU */
	ROM_LOAD( "tp84_10d.bin", 0xe000, 0x2000, CRC(36462ff1) SHA1(118a1b46ee01a583e6cf39af59b073321c76dbff) )

	ROM_REGION( 0x10000, REGION_CPU3, 0 )	/* 64k for code of sound cpu Z80 */
	ROM_LOAD( "tp84s_6a.bin", 0x0000, 0x2000, CRC(c44414da) SHA1(981289f5bdf7dc1348f4ca547ac933ef503b6588) )

	ROM_REGION( 0x4000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "tp84_2j.bin",  0x0000, 0x2000, CRC(05c7508f) SHA1(1a3c7cd47ad34e37a7b0f3014e10c055cbb2b559) ) /* chars */
	ROM_LOAD( "tp84_1j.bin",  0x2000, 0x2000, CRC(498d90b7) SHA1(6975f3a1603b14132aab58329195a4845a6e28bb) )

	ROM_REGION( 0x8000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "tp84_12a.bin", 0x0000, 0x2000, CRC(cd682f30) SHA1(6f48d3efc53d63171ec655e64b225412de1374e4) ) /* sprites */
	ROM_LOAD( "tp84_13a.bin", 0x2000, 0x2000, CRC(888d4bd6) SHA1(7e2dde080bb614709561431a81b0490b2aaa42a9) )
	ROM_LOAD( "tp84_14a.bin", 0x4000, 0x2000, CRC(9a220b39) SHA1(792aaa4daedc8eb807d5a66d87da4641739b1660) )
	ROM_LOAD( "tp84_15a.bin", 0x6000, 0x2000, CRC(fac98397) SHA1(d90f99b19ab3cddfdfd37a273fb437be098088bc) )

	ROM_REGION( 0x0500, REGION_PROMS, 0 )
	ROM_LOAD( "tp84_2c.bin",  0x0000, 0x0100, CRC(d737eaba) SHA1(e39026f87f5b995cf4a38b5d3d3fee7561762ae6) ) /* palette red component */
	ROM_LOAD( "tp84_2d.bin",  0x0100, 0x0100, CRC(2f6a9a2a) SHA1(f09d8b92c7f9bf046cdd815c5282d0510e61b6e0) ) /* palette green component */
	ROM_LOAD( "tp84_1e.bin",  0x0200, 0x0100, CRC(2e21329b) SHA1(9ba8af294dbd6f3a5d039c74a56e0605a913c037) ) /* palette blue component */
	ROM_LOAD( "tp84_1f.bin",  0x0300, 0x0100, CRC(61d2d398) SHA1(3f74ad733b07b6a31cf9d4956d171eb9253dd6bf) ) /* char lookup table */
	ROM_LOAD( "tp84_16c.bin", 0x0400, 0x0100, CRC(13c4e198) SHA1(42ab23206be99e840bd9c52cefa175c12fac8e5b) ) /* sprite lookup table */
ROM_END

ROM_START( tp84a )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "tp84_7j.bin",  0x8000, 0x2000, CRC(605f61c7) SHA1(6848ef35ec7f92cccefb0fb2de42c4b0e9ec476f) )
	ROM_LOAD( "f05",          0xa000, 0x2000, CRC(e97d5093) SHA1(c76c119574d19d2ac10e6987150744542803ef5b) )
	ROM_LOAD( "tp84_9j.bin",  0xc000, 0x2000, CRC(dbd5333b) SHA1(65dee1fd4c940a5423d57cb55a7f2ad89c59c5c6) )
	ROM_LOAD( "f07",          0xe000, 0x2000, CRC(8fbdb4ef) SHA1(e615c4d9964ab00f6776147c54925b4b6100b360) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for the second CPU */
	ROM_LOAD( "tp84_10d.bin", 0xe000, 0x2000, CRC(36462ff1) SHA1(118a1b46ee01a583e6cf39af59b073321c76dbff) )

	ROM_REGION( 0x10000, REGION_CPU3, 0 )	/* 64k for code of sound cpu Z80 */
	ROM_LOAD( "tp84s_6a.bin", 0x0000, 0x2000, CRC(c44414da) SHA1(981289f5bdf7dc1348f4ca547ac933ef503b6588) )

	ROM_REGION( 0x4000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "tp84_2j.bin",  0x0000, 0x2000, CRC(05c7508f) SHA1(1a3c7cd47ad34e37a7b0f3014e10c055cbb2b559) ) /* chars */
	ROM_LOAD( "tp84_1j.bin",  0x2000, 0x2000, CRC(498d90b7) SHA1(6975f3a1603b14132aab58329195a4845a6e28bb) )

	ROM_REGION( 0x8000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "tp84_12a.bin", 0x0000, 0x2000, CRC(cd682f30) SHA1(6f48d3efc53d63171ec655e64b225412de1374e4) ) /* sprites */
	ROM_LOAD( "tp84_13a.bin", 0x2000, 0x2000, CRC(888d4bd6) SHA1(7e2dde080bb614709561431a81b0490b2aaa42a9) )
	ROM_LOAD( "tp84_14a.bin", 0x4000, 0x2000, CRC(9a220b39) SHA1(792aaa4daedc8eb807d5a66d87da4641739b1660) )
	ROM_LOAD( "tp84_15a.bin", 0x6000, 0x2000, CRC(fac98397) SHA1(d90f99b19ab3cddfdfd37a273fb437be098088bc) )

	ROM_REGION( 0x0500, REGION_PROMS, 0 )
	ROM_LOAD( "tp84_2c.bin",  0x0000, 0x0100, CRC(d737eaba) SHA1(e39026f87f5b995cf4a38b5d3d3fee7561762ae6) ) /* palette red component */
	ROM_LOAD( "tp84_2d.bin",  0x0100, 0x0100, CRC(2f6a9a2a) SHA1(f09d8b92c7f9bf046cdd815c5282d0510e61b6e0) ) /* palette green component */
	ROM_LOAD( "tp84_1e.bin",  0x0200, 0x0100, CRC(2e21329b) SHA1(9ba8af294dbd6f3a5d039c74a56e0605a913c037) ) /* palette blue component */
	ROM_LOAD( "tp84_1f.bin",  0x0300, 0x0100, CRC(61d2d398) SHA1(3f74ad733b07b6a31cf9d4956d171eb9253dd6bf) ) /* char lookup table */
	ROM_LOAD( "tp84_16c.bin", 0x0400, 0x0100, CRC(13c4e198) SHA1(42ab23206be99e840bd9c52cefa175c12fac8e5b) ) /* sprite lookup table */
ROM_END



GAME( 1984, tp84,  0,    tp84, tp84, 0, ROT90, "Konami", "Time Pilot '84 (set 1)" )
GAME( 1984, tp84a, tp84, tp84, tp84a,0, ROT90, "Konami", "Time Pilot '84 (set 2)" )
