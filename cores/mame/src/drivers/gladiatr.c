/*
Taito Gladiator (1986)
Known ROM SETS: Golden Castle, Ohgon no Siro

Credits:
- Victor Trucco: original emulation and MAME driver
- Steve Ellenoff: YM2203 Sound, ADPCM Sound, dip switch fixes, high score save,
		  input port patches, panning fix, sprite banking,
		  Golden Castle Rom Set Support
- Phil Stroffolino: palette, sprites, misc video driver fixes
- Tatsuyuki Satoh: YM2203 sound improvements, NEC 8741 simulation,ADPCM with MC6809

special thanks to:
- Camilty for precious hardware information and screenshots
- Jason Richmond for hardware information and misc. notes
- Joe Rounceville for schematics
- and everyone else who'se offered support along the way!

Issues:
- YM2203 mixing problems (loss of bass notes)
- YM2203 some sound effects just don't sound correct
- Audio Filter Switch not hooked up (might solve YM2203 mixing issue)
- Ports 60,61,80,81 not fully understood yet...
- CPU speed may not be accurate
- some sprites linger on later stages (perhaps a sprite enable bit?)
- Flipscreen not implemented
- Scrolling issues in Test mode!
- The four 8741 ROMs are available but not used.

Preliminary Gladiator Memory Map

Main CPU (Z80)

$0000-$3FFF	QB0-5
$4000-$5FFF	QB0-4

$6000-$7FFF	QB0-1 Paged
$8000-$BFFF	QC0-3 Paged

    if port 02 = 00     QB0-1 offset (0000-1fff) at location 6000-7fff
                        QC0-3 offset (0000-3fff) at location 8000-bfff

    if port 02 = 01     QB0-1 offset (2000-3fff) at location 6000-7fff
                        QC0-3 offset (4000-7fff) at location 8000-bfff

$C000-$C3FF	sprite RAM
$C400-$C7FF	sprite attributes
$C800-$CBFF	more sprite attributes

$CC00-$D7FF	video registers

(scrolling, 2 screens wide)
$D800-DFFF	background layer VRAM (tiles)
$E000-E7FF	background layer VRAM (attributes)
$E800-EFFF	foreground text layer VRAM

$F000-$F3FF	Battery Backed RAM
$F400-$F7FF	Work RAM

Audio CPU (Z80)
$0000-$3FFF	QB0-17
$8000-$83FF	Work RAM 2.CPU


Preliminary Descriptions of I/O Ports.

Main z80
8 pins of LS259:
  00 - OBJACS ? (I can't read the name in schematics)
  01 - OBJCGBK (Sprite banking)
  02 - PR.BK (ROM banking)
  03 - NMIFG (connects to NMI of main Z80, but there's no code in 0066)
  04 - SRST (probably some type of reset)
  05 - CBK0 (unknown)
  06 - LOBJ (connected near graphic ROMs)
  07 - REVERS
  9E - Send data to NEC 8741-0 (comunicate with 2nd z80)
		(dip switch 1 is read in these ports too)
  9F - Send commands to NEC 8741-0

  C0-DF 8251 (Debug port ?)

2nd z80

00 - YM2203 Control Reg.
01 - YM2203 Data Read / Write Reg.
		Port B of the YM2203 is connected to dip switch 3
20 - Send data to NEC 8741-1 (comunicate with Main z80)
		(dip switch 2 is read in these ports too)
21 - Send commands to NEC 8741-1 (comunicate with Main z80)
40 - Clear Interrupt latch
60 - Send data to NEC 8741-2 (Read Joystick and Coin Slot (both players)
61 - Send commands to NEC 8741-2
80 - Send data to NEC 8741-3 (Read buttons (Fire 1, 2 and 3 (both players), service button) )
81 - Send commands to NEC 8741-3

A0-BF  - Audio mixer control ?
E0     - Comunication port to 6809
*/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "machine/tait8741.h"
#include "cpu/z80/z80.h"

/*Video functions*/
extern unsigned char *gladiator_text;
WRITE_HANDLER( gladiatr_video_registers_w );
READ_HANDLER( gladiatr_video_registers_r );
WRITE_HANDLER( gladiatr_paletteram_rg_w );
WRITE_HANDLER( gladiatr_paletteram_b_w );
extern VIDEO_START( gladiatr );
extern VIDEO_UPDATE( gladiatr );
WRITE_HANDLER( gladiatr_spritebank_w );

/*Rom bankswitching*/
static int banka;
WRITE_HANDLER( gladiatr_bankswitch_w );
READ_HANDLER( gladiatr_bankswitch_r );

/*Rom bankswitching*/
WRITE_HANDLER( gladiatr_bankswitch_w ){
	static int bank1[2] = { 0x10000, 0x12000 };
	static int bank2[2] = { 0x14000, 0x18000 };
	unsigned char *RAM = memory_region(REGION_CPU1);
	banka = data;
	cpu_setbank(1,&RAM[bank1[(data & 0x03)]]);
	cpu_setbank(2,&RAM[bank2[(data & 0x03)]]);
}

READ_HANDLER( gladiatr_bankswitch_r ){
	return banka;
}

static READ_HANDLER( gladiator_dsw1_r )
{
	int orig = readinputport(0); /* DSW1 */
/*Reverse all bits for Input Port 0*/
/*ie..Bit order is: 0,1,2,3,4,5,6,7*/
return   ((orig&0x01)<<7) | ((orig&0x02)<<5)
       | ((orig&0x04)<<3) | ((orig&0x08)<<1)
       | ((orig&0x10)>>1) | ((orig&0x20)>>3)
       | ((orig&0x40)>>5) | ((orig&0x80)>>7);;
}

static READ_HANDLER( gladiator_dsw2_r )
{
	int orig = readinputport(1); /* DSW2 */
/*Bits 2-7 are reversed for Input Port 1*/
/*ie..Bit order is: 2,3,4,5,6,7,1,0*/
return	  (orig&0x01) | (orig&0x02)
	| ((orig&0x04)<<5) | ((orig&0x08)<<3)
	| ((orig&0x10)<<1) | ((orig&0x20)>>1)
	| ((orig&0x40)>>3) | ((orig&0x80)>>5);
}

static READ_HANDLER( gladiator_controll_r )
{
	int coins = 0;

	if( readinputport(4) & 0xc0 ) coins = 0x80;
	switch(offset)
	{
	case 0x01: /* start button , coins */
		return readinputport(3) | coins;
	case 0x02: /* Player 1 Controller , coins */
		return readinputport(5) | coins;
	case 0x04: /* Player 2 Controller , coins */
		return readinputport(6) | coins;
	}
	/* unknown */
	return 0;
}

static READ_HANDLER( gladiator_button3_r )
{
	switch(offset)
	{
	case 0x01: /* button 3 */
		return readinputport(7);
	}
	/* unknown */
	return 0;
}

static struct TAITO8741interface gsword_8741interface=
{
	4,         /* 4 chips */
	{TAITO8741_MASTER,TAITO8741_SLAVE,TAITO8741_PORT,TAITO8741_PORT},/* program mode */
	{1,0,0,0},	/* serial port connection */
	{gladiator_dsw1_r,gladiator_dsw2_r,gladiator_button3_r,gladiator_controll_r}	/* port handler */
};

static MACHINE_INIT( gladiator )
{
	TAITO8741_start(&gsword_8741interface);
	/* 6809 bank memory set */
	{
		unsigned char *RAM = memory_region(REGION_CPU3);
		cpu_setbank(3,&RAM[0x10000]);
		cpu_setbank(4,&RAM[0x18000]);
		cpu_setbank(5,&RAM[0x20000]);
	}
}

#if 1
/* !!!!! patch to IRQ timming for 2nd CPU !!!!! */
WRITE_HANDLER( gladiatr_irq_patch_w )
{
	cpu_set_irq_line(1,0,HOLD_LINE);
}
#endif

/* YM2203 port A handler (input) */
static READ_HANDLER( gladiator_dsw3_r )
{
	return input_port_2_r(offset)^0xff;
}
/* YM2203 port B handler (output) */
static WRITE_HANDLER( gladiator_int_control_w )
{
	/* bit 7   : SSRST = sound reset ? */
	/* bit 6-1 : N.C.                  */
	/* bit 0   : ??                    */
}
/* YM2203 IRQ */
static void gladiator_ym_irq(int irq)
{
	/* NMI IRQ is not used by gladiator sound program */
	cpu_set_nmi_line(1,irq ? ASSERT_LINE : CLEAR_LINE);
}

/*Sound Functions*/
static WRITE_HANDLER( glad_adpcm_w )
{
	unsigned char *RAM = memory_region(REGION_CPU3);
	/* bit6 = bank offset */
	int bankoffset = data&0x40 ? 0x4000 : 0;
	cpu_setbank(3,&RAM[0x10000+bankoffset]);
	cpu_setbank(4,&RAM[0x18000+bankoffset]);
	cpu_setbank(5,&RAM[0x20000+bankoffset]);

	MSM5205_data_w(0,data);         /* bit0..3  */
	MSM5205_reset_w(0,(data>>5)&1); /* bit 5    */
	MSM5205_vclk_w (0,(data>>4)&1); /* bit4     */
}

static WRITE_HANDLER( glad_cpu_sound_command_w )
{
	soundlatch_w(0,data);
	cpu_set_nmi_line(2,ASSERT_LINE);
}

static READ_HANDLER( glad_cpu_sound_command_r )
{
	cpu_set_nmi_line(2,CLEAR_LINE);
	return soundlatch_r(0);
}




static MEMORY_READ_START( readmem )
	{ 0x0000, 0x5fff, MRA_ROM },
	{ 0x6000, 0x7fff, MRA_BANK1},
	{ 0x8000, 0xbfff, MRA_BANK2},
	{ 0xc000, 0xcbff, MRA_RAM },
	{ 0xcc00, 0xcfff, gladiatr_video_registers_r },
	{ 0xd000, 0xffff, MRA_RAM },
MEMORY_END

static MEMORY_WRITE_START( writemem )
	{ 0x0000, 0xbfff, MWA_ROM },
	{ 0xc000, 0xcbff, MWA_RAM, &spriteram },
	{ 0xcc00, 0xcfff, gladiatr_video_registers_w },
	{ 0xd000, 0xd1ff, gladiatr_paletteram_rg_w, &paletteram },
	{ 0xd200, 0xd3ff, MWA_RAM },
	{ 0xd400, 0xd5ff, gladiatr_paletteram_b_w, &paletteram_2 },
	{ 0xd600, 0xd7ff, MWA_RAM },
	{ 0xd800, 0xdfff, videoram_w, &videoram },
	{ 0xe000, 0xe7ff, colorram_w, &colorram },
	{ 0xe800, 0xefff, MWA_RAM, &gladiator_text },
	{ 0xf000, 0xf3ff, MWA_RAM, &generic_nvram, &generic_nvram_size }, /* battery backed RAM */
	{ 0xf400, 0xffff, MWA_RAM },
MEMORY_END

static MEMORY_READ_START( readmem_cpu2 )
	{ 0x0000, 0x3fff, MRA_ROM },
	{ 0x8000, 0x83ff, MRA_RAM },
MEMORY_END

static MEMORY_WRITE_START( writemem_cpu2 )
	{ 0x0000, 0x3fff, MWA_ROM },
	{ 0x8000, 0x83ff, MWA_RAM },
MEMORY_END

static MEMORY_READ_START( sound_readmem )
	{ 0x2000, 0x2fff, glad_cpu_sound_command_r },
	{ 0x4000, 0x7fff, MRA_BANK3 }, /* BANKED ROM */
	{ 0x8000, 0xbfff, MRA_BANK4 }, /* BANKED ROM */
	{ 0xc000, 0xffff, MRA_BANK5 }, /* BANKED ROM */
MEMORY_END

static MEMORY_WRITE_START( sound_writemem )
	{ 0x1000, 0x1fff, glad_adpcm_w },
MEMORY_END


static PORT_READ_START( readport )
	{ 0x02, 0x02, gladiatr_bankswitch_r },
	{ 0x9e, 0x9f, TAITO8741_0_r },
PORT_END

static PORT_WRITE_START( writeport )
	{ 0x01, 0x01, gladiatr_spritebank_w},
	{ 0x02, 0x02, gladiatr_bankswitch_w},
	{ 0x04, 0x04, gladiatr_irq_patch_w}, /* !!! patch to 2nd CPU IRQ !!! */
	{ 0x9e, 0x9f, TAITO8741_0_w },
	{ 0xbf, 0xbf, IOWP_NOP },
PORT_END

static PORT_READ_START( readport_cpu2 )
	{ 0x00, 0x00, YM2203_status_port_0_r },
	{ 0x01, 0x01, YM2203_read_port_0_r },
	{ 0x20, 0x21, TAITO8741_1_r },
	{ 0x40, 0x40, IORP_NOP },
	{ 0x60, 0x61, TAITO8741_2_r },
	{ 0x80, 0x81, TAITO8741_3_r },
PORT_END

static PORT_WRITE_START( writeport_cpu2 )
	{ 0x00, 0x00, YM2203_control_port_0_w },
	{ 0x01, 0x01, YM2203_write_port_0_w },
	{ 0x20, 0x21, TAITO8741_1_w },
	{ 0x60, 0x61, TAITO8741_2_w },
	{ 0x80, 0x81, TAITO8741_3_w },
/*	{ 0x40, 0x40, glad_sh_irq_clr }, */
	{ 0xe0, 0xe0, glad_cpu_sound_command_w },
PORT_END

INPUT_PORTS_START( gladiatr )
	PORT_START		/* DSW1 (8741-0 parallel port)*/
	PORT_DIPNAME( 0x03, 0x01, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x00, "Easy" )
	PORT_DIPSETTING(    0x01, "Medium" )
	PORT_DIPSETTING(    0x02, "Hard" )
	PORT_DIPSETTING(    0x03, "Hardest" )
	PORT_DIPNAME( 0x04, 0x04, "After 4 Stages" )
	PORT_DIPSETTING(    0x04, "Continues" )
	PORT_DIPSETTING(    0x00, "Ends" )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Bonus_Life ) )   /*NOTE: Actual manual has these settings reversed(typo?)! */
	PORT_DIPSETTING(    0x00, "Only at 100000" )
	PORT_DIPSETTING(    0x08, "Every 100000" )
	PORT_DIPNAME( 0x30, 0x20, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "1" )
	PORT_DIPSETTING(    0x10, "2" )
	PORT_DIPSETTING(    0x20, "3" )
	PORT_DIPSETTING(    0x30, "4" )
	PORT_DIPNAME( 0x40, 0x00, "Allow Continue" )
	PORT_DIPSETTING(    0x40, DEF_STR( No ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x80, DEF_STR( On ) )

	PORT_START      /* DSW2  (8741-1 parallel port) - Dips 6 Unused */
	PORT_DIPNAME( 0x03, 0x00, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_5C ) )
	PORT_DIPNAME( 0x0c, 0x00, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 2C_1C ) )
	PORT_DIPNAME( 0x10, 0x00, DEF_STR( Free_Play ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x10, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x80, DEF_STR( On ) )

	PORT_START      /* DSW3 (YM2203 port B) - Dips 5,6,7 Unused */
	PORT_BITX(    0x01, 0x00, IPT_DIPSWITCH_NAME | IPF_CHEAT, "Invulnerability", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x01, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x00, "Memory Backup" )
	PORT_DIPSETTING(    0x00, "Normal" )
	PORT_DIPSETTING(    0x02, "Clear" )
	PORT_DIPNAME( 0x0c, 0x00, "Starting Stage" )
	PORT_DIPSETTING(    0x00, "1" )
	PORT_DIPSETTING(    0x04, "2" )
	PORT_DIPSETTING(    0x08, "3" )
	PORT_DIPSETTING(    0x0c, "4" )
	PORT_SERVICE( 0x80, IP_ACTIVE_HIGH )

	PORT_START	/* IN0 (8741-3 parallel port 1) */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN ) /* COINS */

	PORT_START	/* COINS (8741-3 parallel port bit7) */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT_IMPULSE( 0x40, IP_ACTIVE_HIGH, IPT_COIN1, 1 )
	PORT_BIT_IMPULSE( 0x80, IP_ACTIVE_HIGH, IPT_COIN2, 1 )

	PORT_START	/* IN1 (8741-3 parallel port 2) */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN ) /* COINS */

	PORT_START	/* IN2 (8741-3 parallel port 4) */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON2 | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN ) /* COINS */

	PORT_START	/* IN3 (8741-2 parallel port 1) */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON3 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_BUTTON3 | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )
INPUT_PORTS_END

/*******************************************************************/

static struct GfxLayout gladiator_text_layout  =   /* gfxset 0 */
{
	8,8,	/* 8*8 tiles */
	1024,	/* number of tiles */
	1,		/* bits per pixel */
	{ 0 },	/* plane offsets */
	{ 0,1,2,3,4,5,6,7 }, /* x offsets */
	{ 0*8,1*8,2*8,3*8,4*8,5*8,6*8,7*8 }, /* y offsets */
	64 /* offset to next tile */
};

/*******************************************************************/

#define DEFINE_LAYOUT( NAME,P0,P1,P2) static struct GfxLayout NAME = { \
	8,8,512,3, \
	{ P0, P1, P2}, \
	{ 0,1,2,3,64+0,64+1,64+2,64+3 }, \
	{ 0*8,1*8,2*8,3*8,4*8,5*8,6*8,7*8 }, \
	128 \
};

DEFINE_LAYOUT( gladiator_tile0, 4,			0x08000*8, 0x08000*8+4 )
DEFINE_LAYOUT( gladiator_tile1, 0,			0x0A000*8, 0x0A000*8+4 )
DEFINE_LAYOUT( gladiator_tile2, 4+0x2000*8, 0x10000*8, 0x10000*8+4 )
DEFINE_LAYOUT( gladiator_tile3, 0+0x2000*8, 0x12000*8, 0x12000*8+4 )
DEFINE_LAYOUT( gladiator_tile4, 4+0x4000*8, 0x0C000*8, 0x0C000*8+4 )
DEFINE_LAYOUT( gladiator_tile5, 0+0x4000*8, 0x0E000*8, 0x0E000*8+4 )
DEFINE_LAYOUT( gladiator_tile6, 4+0x6000*8, 0x14000*8, 0x14000*8+4 )
DEFINE_LAYOUT( gladiator_tile7, 0+0x6000*8, 0x16000*8, 0x16000*8+4 )
DEFINE_LAYOUT( gladiator_tileA, 4+0x2000*8, 0x0A000*8, 0x0A000*8+4 )
DEFINE_LAYOUT( gladiator_tileB, 0,			0x10000*8, 0x10000*8+4 )
DEFINE_LAYOUT( gladiator_tileC, 4+0x6000*8, 0x0E000*8, 0x0E000*8+4 )
DEFINE_LAYOUT( gladiator_tileD, 0+0x4000*8, 0x14000*8, 0x14000*8+4 )

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	/* monochrome text layer */
	{ REGION_GFX1, 0x00000, &gladiator_text_layout, 512, 1 },

	/* background tiles */
	{ REGION_GFX2, 0x00000, &gladiator_tile0, 0, 64 },
	{ REGION_GFX2, 0x00000, &gladiator_tile1, 0, 64 },
	{ REGION_GFX2, 0x00000, &gladiator_tile2, 0, 64 },
	{ REGION_GFX2, 0x00000, &gladiator_tile3, 0, 64 },
	{ REGION_GFX2, 0x00000, &gladiator_tile4, 0, 64 },
	{ REGION_GFX2, 0x00000, &gladiator_tile5, 0, 64 },
	{ REGION_GFX2, 0x00000, &gladiator_tile6, 0, 64 },
	{ REGION_GFX2, 0x00000, &gladiator_tile7, 0, 64 },

	/* sprites */
	{ REGION_GFX3, 0x00000, &gladiator_tile0, 0, 64 },
	{ REGION_GFX3, 0x00000, &gladiator_tileB, 0, 64 },
	{ REGION_GFX3, 0x00000, &gladiator_tileA, 0, 64 },
	{ REGION_GFX3, 0x00000, &gladiator_tile3, 0, 64 }, /* "GLAD..." */
	{ REGION_GFX3, 0x18000, &gladiator_tile0, 0, 64 },
	{ REGION_GFX3, 0x18000, &gladiator_tileB, 0, 64 },
	{ REGION_GFX3, 0x18000, &gladiator_tileA, 0, 64 },
	{ REGION_GFX3, 0x18000, &gladiator_tile3, 0, 64 }, /* ...DIATOR */
	{ REGION_GFX3, 0x18000, &gladiator_tile4, 0, 64 },
	{ REGION_GFX3, 0x18000, &gladiator_tileD, 0, 64 },
	{ REGION_GFX3, 0x18000, &gladiator_tileC, 0, 64 },
	{ REGION_GFX3, 0x18000, &gladiator_tile7, 0, 64 },

	{ -1 } /* end of array */
};

#undef DEFINE_LAYOUT



static struct YM2203interface ym2203_interface =
{
	1,		/* 1 chip */
	1500000,	/* 1.5 MHz? */
	{ YM2203_VOL(25,25) },
	{ 0 },
	{ gladiator_dsw3_r },         /* port B read */
	{ gladiator_int_control_w }, /* port A write */
	{ 0 },
	{ gladiator_ym_irq }          /* NMI request for 2nd cpu */
};

static struct MSM5205interface msm5205_interface =
{
	1,					/* 1 chip             */
	455000,				/* 455KHz ??          */
	{ 0 },				/* interrupt function */
	{ MSM5205_SEX_4B},	/* vclk input mode    */
	{ 60 }
};



static MACHINE_DRIVER_START( gladiatr )

	/* basic machine hardware */
	MDRV_CPU_ADD(Z80, 6000000) /* 6 MHz? */
	MDRV_CPU_MEMORY(readmem,writemem)
	MDRV_CPU_PORTS(readport,writeport)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,1)

	MDRV_CPU_ADD(Z80, 3000000) /* 3 MHz? */
	MDRV_CPU_MEMORY(readmem_cpu2,writemem_cpu2)
	MDRV_CPU_PORTS(readport_cpu2,writeport_cpu2)

	MDRV_CPU_ADD(M6809, 750000)
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU) /* 750 kHz (hand tuned) */
	MDRV_CPU_MEMORY(sound_readmem,sound_writemem)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION) /* fps, vblank duration */
	MDRV_INTERLEAVE(10)

	MDRV_MACHINE_INIT(gladiator)
	MDRV_NVRAM_HANDLER(generic_0fill)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(0, 255, 0+16, 255-16)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(512+2)

	MDRV_VIDEO_START(gladiatr)
	MDRV_VIDEO_UPDATE(gladiatr)

	/* sound hardware */
	MDRV_SOUND_ADD(YM2203, ym2203_interface)
	MDRV_SOUND_ADD(MSM5205, msm5205_interface)
MACHINE_DRIVER_END

/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( gladiatr )
	ROM_REGION( 0x1c000, REGION_CPU1, 0 )
	ROM_LOAD( "qb0-5",          0x00000, 0x4000, CRC(25b19efb) SHA1(c41344278f6c7f3d6527aced3e459ed1ba86dea5) )
	ROM_LOAD( "qb0-4",          0x04000, 0x2000, CRC(347ec794) SHA1(51100f9fef2e96f00e94fce709eed6583b01a2eb) )
	ROM_LOAD( "qb0-1",          0x10000, 0x4000, CRC(040c9839) SHA1(8c0d9a246847461a59eb5e6a53a94218e701d6c3) )
	ROM_LOAD( "qc0-3",          0x14000, 0x8000, CRC(8d182326) SHA1(f0af3757c2cf9e1e8035272567adee6efc733319) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* Code for the 2nd CPU */
	ROM_LOAD( "qb0-17",       	0x0000, 0x4000, CRC(e78be010) SHA1(157231d858d13a006b57a4ab419368168e64edb7) )

	ROM_REGION( 0x28000, REGION_CPU3, 0 )  /* 6809 Code & ADPCM data */
	ROM_LOAD( "qb0-20",         0x10000, 0x8000, CRC(15916eda) SHA1(6558bd2ae6f14d630ae93e66ce7d09be33870cce) )
	ROM_LOAD( "qb0-19",         0x18000, 0x8000, CRC(79caa7ed) SHA1(57adc8429ad016c4da41deda6b7b6fe36de5a225) )
	ROM_LOAD( "qb0-18",         0x20000, 0x8000, CRC(e9591260) SHA1(e427aa10c683fbeb98171f6d1820781d21075a24) )

	ROM_REGION( 0x02000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "qc0-15",       	0x00000, 0x2000, CRC(a7efa340) SHA1(f87e061b8e4d8cd0834fab301779a8493549419b) ) /* (monochrome) */

	ROM_REGION( 0x18000, REGION_GFX2, ROMREGION_DISPOSE )	/* tiles */
	ROM_LOAD( "qb0-12",       	0x00000, 0x8000, CRC(0585d9ac) SHA1(e3cb07e9dc5ec2fcfa0c90294d32f0b751f67752) ) /* plane 3 */
	ROM_LOAD( "qb0-13",       	0x08000, 0x8000, CRC(a6bb797b) SHA1(852e9993270e5557c1a0350007d0beaec5ca6286) ) /* planes 1,2 */
	ROM_LOAD( "qb0-14",       	0x10000, 0x8000, CRC(85b71211) SHA1(81545cd168da4a707e263fdf0ee9902e3a13ba93) ) /* planes 1,2 */

	ROM_REGION( 0x30000, REGION_GFX3, ROMREGION_DISPOSE )	/* sprites */
	ROM_LOAD( "qc1-6",        	0x00000, 0x4000, CRC(651e6e44) SHA1(78ce576e6c29e43d590c42f0d4926cff82fd0268) ) /* plane 3 */
	ROM_LOAD( "qc0-8",        	0x08000, 0x4000, CRC(1c7ffdad) SHA1(b224fd4cce078186f22e6393a38c7a2d84dc0066) ) /* planes 1,2 */
	ROM_LOAD( "qc1-9",        	0x10000, 0x4000, CRC(01043e03) SHA1(6a6dddc0a036873135dceaa989e757bdd2455ae7) ) /* planes 1,2 */
	ROM_LOAD( "qc2-7",        	0x18000, 0x8000, CRC(c992c4f7) SHA1(3263973474af07c8b93c4ec97924568848cb7201) ) /* plane 3 */
	ROM_LOAD( "qc1-10",       	0x20000, 0x8000, CRC(364cdb58) SHA1(4d8548f9dfa9d105dd277c61cf3d56583a5ebbcb) ) /* planes 1,2 */
	ROM_LOAD( "qc2-11",       	0x28000, 0x8000, CRC(c9fecfff) SHA1(7c13ace4293fbfab7fe924b7b24c498d8cefc7ac) ) /* planes 1,2 */
ROM_END

ROM_START( ogonsiro )
	ROM_REGION( 0x1c000, REGION_CPU1, 0 )
	ROM_LOAD( "qb0-5",          0x00000, 0x4000, CRC(25b19efb) SHA1(c41344278f6c7f3d6527aced3e459ed1ba86dea5) )
	ROM_LOAD( "qb0-4",          0x04000, 0x2000, CRC(347ec794) SHA1(51100f9fef2e96f00e94fce709eed6583b01a2eb) )
	ROM_LOAD( "qb0-1",          0x10000, 0x4000, CRC(040c9839) SHA1(8c0d9a246847461a59eb5e6a53a94218e701d6c3) )
	ROM_LOAD( "qb0_3",          0x14000, 0x8000, CRC(d6a342e7) SHA1(96274ae3bda4679108a25fcc514b625552abda30) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* Code for the 2nd CPU */
	ROM_LOAD( "qb0-17",       	0x0000, 0x4000, CRC(e78be010) SHA1(157231d858d13a006b57a4ab419368168e64edb7) )

	ROM_REGION( 0x28000, REGION_CPU3, 0 )  /* 6809 Code & ADPCM data */
	ROM_LOAD( "qb0-20",         0x10000, 0x8000, CRC(15916eda) SHA1(6558bd2ae6f14d630ae93e66ce7d09be33870cce) )
	ROM_LOAD( "qb0-19",         0x18000, 0x8000, CRC(79caa7ed) SHA1(57adc8429ad016c4da41deda6b7b6fe36de5a225) )
	ROM_LOAD( "qb0-18",         0x20000, 0x8000, CRC(e9591260) SHA1(e427aa10c683fbeb98171f6d1820781d21075a24) )

	ROM_REGION( 0x02000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "qb0_15",       	0x00000, 0x2000, CRC(5e1332b8) SHA1(fab6e2c7ea9bc94c1245bf759b4004a70c57d666) ) /* (monochrome) */

	ROM_REGION( 0x18000, REGION_GFX2, ROMREGION_DISPOSE )	/* tiles */
	ROM_LOAD( "qb0-12",       	0x00000, 0x8000, CRC(0585d9ac) SHA1(e3cb07e9dc5ec2fcfa0c90294d32f0b751f67752) ) /* plane 3 */
	ROM_LOAD( "qb0-13",       	0x08000, 0x8000, CRC(a6bb797b) SHA1(852e9993270e5557c1a0350007d0beaec5ca6286) ) /* planes 1,2 */
	ROM_LOAD( "qb0-14",       	0x10000, 0x8000, CRC(85b71211) SHA1(81545cd168da4a707e263fdf0ee9902e3a13ba93) ) /* planes 1,2 */

	ROM_REGION( 0x30000, REGION_GFX3, ROMREGION_DISPOSE )	/* sprites */
	ROM_LOAD( "qb0_6",        	0x00000, 0x4000, CRC(1a2bc769) SHA1(498861f4d0cffeaff90609c8000c921a114756b6) ) /* plane 3 */
	ROM_LOAD( "qc0-8",        	0x08000, 0x4000, CRC(1c7ffdad) SHA1(b224fd4cce078186f22e6393a38c7a2d84dc0066) ) /* planes 1,2 */
	ROM_LOAD( "qb0_9",        	0x10000, 0x4000, CRC(38f5152d) SHA1(fbb7b13a625999807d180a3212e6e12870629438) ) /* planes 1,2 */
	ROM_LOAD( "qb0_7",        	0x18000, 0x8000, CRC(4b677bd9) SHA1(3314ef58ff5307faf0ecd8f99950d43d571c91a6) ) /* plane 3 */
	ROM_LOAD( "qb0_10",       	0x20000, 0x8000, CRC(87ab6cc4) SHA1(50bc1108ff5609c0e7dad615e92e16eb72b7bc03) ) /* planes 1,2 */
	ROM_LOAD( "qb0_11",       	0x28000, 0x8000, CRC(25eaa4ff) SHA1(3547fc600a617ba7fe5240a7830edb90230b6c51) ) /* planes 1,2 */
ROM_END



GAMEX( 1986, gladiatr, 0,        gladiatr, gladiatr, 0, ROT0, "Taito America Corporation", "Gladiator (US)", GAME_NO_COCKTAIL )
GAMEX( 1986, ogonsiro, gladiatr, gladiatr, gladiatr, 0, ROT0, "Taito Corporation", "Ohgon no Siro (Japan)", GAME_NO_COCKTAIL )
