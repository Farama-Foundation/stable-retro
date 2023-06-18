/***************************************************************************

	Jaleco Exerion hardware

****************************************************************************

	Exerion is a unique driver in that it has idiosyncracies that are straight
	out of Bizarro World. I submit for your approval:

	* The mystery reads from $d802 - timer-based protection?
	* The freakish graphics encoding scheme, which no other MAME-supported game uses
	* The sprite-ram, and all the funky parameters that go along with it

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "exerion.h"



/*************************************
 *
 *	Interrupts & inputs
 *
 *************************************/

static READ_HANDLER( exerion_port01_r )
{
	/* the cocktail flip bit muxes between ports 0 and 1 */
	return exerion_cocktail_flip ? input_port_1_r(offset) : input_port_0_r(offset);
}


static READ_HANDLER( exerion_port3_r )
{
	/* bit 0 is VBLANK, which we simulate manually */
	int result = input_port_3_r(offset);
	int ybeam = cpu_getscanline();
	if (ybeam > Machine->visible_area.max_y)
		result |= 1;
	return result;
}


static INTERRUPT_GEN( exerion_interrupt )
{
	/* Exerion triggers NMIs on coin insertion */
	if (readinputport(4) & 1)
		cpu_set_irq_line(0, IRQ_LINE_NMI, PULSE_LINE);
}



/*************************************
 *
 *	Protection??
 *
 *************************************/

/* This is the first of many Exerion "features." No clue if it's */
/* protection or some sort of timer. */
static UINT8 porta;
static UINT8 portb;

static READ_HANDLER( exerion_porta_r )
{
	porta ^= 0x40;
	return porta;
}


static WRITE_HANDLER( exerion_portb_w )
{
	/* pull the expected value from the ROM */
	porta = memory_region(REGION_CPU1)[0x5f76];
	portb = data;

	log_cb(RETRO_LOG_DEBUG, LOGPRE "Port B = %02X\n", data);
}


static READ_HANDLER( exerion_protection_r )
{
	UINT8 *RAM = memory_region(REGION_CPU1);

	if (activecpu_get_pc() == 0x4143)
		return RAM[0x33c0 + (RAM[0x600d] << 2) + offset];
	else
		return RAM[0x6008 + offset];
}



/*************************************
 *
 *	Main CPU memory handlers
 *
 *************************************/

static MEMORY_READ_START( readmem )
	{ 0x0000, 0x5fff, MRA_ROM },
	{ 0x6008, 0x600b, exerion_protection_r },
	{ 0x6000, 0x67ff, MRA_RAM },
	{ 0x8000, 0x8bff, MRA_RAM },
	{ 0xa000, 0xa000, exerion_port01_r },
	{ 0xa800, 0xa800, input_port_2_r },
	{ 0xb000, 0xb000, exerion_port3_r },
	{ 0xd802, 0xd802, AY8910_read_port_1_r },
MEMORY_END


static MEMORY_WRITE_START( writemem )
	{ 0x0000, 0x5fff, MWA_ROM },
	{ 0x6000, 0x67ff, MWA_RAM },
	{ 0x8000, 0x87ff, videoram_w, &videoram, &videoram_size },
	{ 0x8800, 0x887f, MWA_RAM, &spriteram, &spriteram_size },
	{ 0x8800, 0x8bff, MWA_RAM },
	{ 0xc000, 0xc000, exerion_videoreg_w },
	{ 0xc800, 0xc800, soundlatch_w },
	{ 0xd000, 0xd000, AY8910_control_port_0_w },
	{ 0xd001, 0xd001, AY8910_write_port_0_w },
	{ 0xd800, 0xd800, AY8910_control_port_1_w },
	{ 0xd801, 0xd801, AY8910_write_port_1_w },
MEMORY_END



/*************************************
 *
 *	Sub CPU memory handlers
 *
 *************************************/

static MEMORY_READ_START( cpu2_readmem )
	{ 0x0000, 0x1fff, MRA_ROM },
	{ 0x4000, 0x47ff, MRA_RAM },
	{ 0x6000, 0x6000, soundlatch_r },
	{ 0xa000, 0xa000, exerion_video_timing_r },
MEMORY_END


static MEMORY_WRITE_START( cpu2_writemem )
	{ 0x0000, 0x1fff, MWA_ROM },
	{ 0x4000, 0x47ff, MWA_RAM },
	{ 0x8000, 0x800c, exerion_video_latch_w },
MEMORY_END



/*************************************
 *
 *	Port definitions
 *
 *************************************/

INPUT_PORTS_START( exerion )
	PORT_START      /* player 1 inputs (muxed on 0xa000) */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START2 )

	PORT_START      /* player 2 inputs (muxed on 0xa000) */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START2 )

	PORT_START      /* dip switches (0xa800) */
	PORT_DIPNAME( 0x07, 0x02, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "1" )
	PORT_DIPSETTING(    0x01, "2" )
	PORT_DIPSETTING(    0x02, "3" )
	PORT_DIPSETTING(    0x03, "4" )
	PORT_DIPSETTING(    0x04, "5" )
	PORT_BITX(0,        0x07, IPT_DIPSWITCH_SETTING | IPF_CHEAT, "Infinite", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPNAME( 0x18, 0x00, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x00, "10000" )
	PORT_DIPSETTING(    0x08, "20000" )
	PORT_DIPSETTING(    0x10, "30000" )
	PORT_DIPSETTING(    0x18, "40000" )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )      /* used */
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x20, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x00, "Easy" )
	PORT_DIPSETTING(    0x40, "Hard" )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Cocktail ) )

	PORT_START      /* dip switches/VBLANK (0xb000) */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_UNUSED )		/* VBLANK */
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x02, DEF_STR( On ) )
	PORT_DIPNAME( 0x0c, 0x00, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( 1C_4C ) )
	PORT_BIT( 0xf0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START      /* FAKE */
	/* The coin slots are not memory mapped. */
	/* This fake input port is used by the interrupt */
	/* handler to be notified of coin insertions. We use IMPULSE to */
	/* trigger exactly one interrupt, without having to check when the */
	/* user releases the key. */
	PORT_BIT_IMPULSE( 0x01, IP_ACTIVE_HIGH, IPT_COIN1, 1 )
INPUT_PORTS_END



/*************************************
 *
 *	Graphics layouts
 *
 *************************************/

static struct GfxLayout charlayout =
{
	8,8,
	RGN_FRAC(1,1),
	2,
	{ 0, 4 },
	{ 3, 2, 1, 0, 8+3, 8+2, 8+1, 8+0 },
	{ 16*0, 16*1, 16*2, 16*3, 16*4, 16*5, 16*6, 16*7 },
	16*8
};


/* 16 x 16 sprites -- requires reorganizing characters in init_exerion() */
static struct GfxLayout spritelayout =
{
	16,16,
	RGN_FRAC(1,1),
	2,
	{ 0, 4 },
	{  3, 2, 1, 0, 8+3, 8+2, 8+1, 8+0,
			16+3, 16+2, 16+1, 16+0, 24+3, 24+2, 24+1, 24+0 },
	{ 32*0, 32*1, 32*2, 32*3, 32*4, 32*5, 32*6, 32*7,
			32*8, 32*9, 32*10, 32*11, 32*12, 32*13, 32*14, 32*15 },
	64*8
};


/* Quick and dirty way to emulate pixel-doubled sprites. */
static struct GfxLayout bigspritelayout =
{
	32,32,
	RGN_FRAC(1,1),
	2,
	{ 0, 4 },
	{  3, 3, 2, 2, 1, 1, 0, 0,
			8+3, 8+3, 8+2, 8+2, 8+1, 8+1, 8+0, 8+0,
			16+3, 16+3, 16+2, 16+2, 16+1, 16+1, 16+0, 16+0,
			24+3, 24+3, 24+2, 24+2, 24+1, 24+1, 24+0, 24+0 },
	{ 32*0, 32*0, 32*1, 32*1, 32*2, 32*2, 32*3, 32*3,
			32*4, 32*4, 32*5, 32*5, 32*6, 32*6, 32*7, 32*7,
			32*8, 32*8, 32*9, 32*9, 32*10, 32*10, 32*11, 32*11,
			32*12, 32*12, 32*13, 32*13, 32*14, 32*14, 32*15, 32*15 },
	64*8
};


static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &charlayout,         0, 64 },
	{ REGION_GFX2, 0, &spritelayout,     256, 64 },
	{ REGION_GFX2, 0, &bigspritelayout,  256, 64 },
	{ -1 }
};



/*************************************
 *
 *	Sound interfaces
 *
 *************************************/

static struct AY8910interface ay8910_interface =
{
	2,  /* 2 chips */
	10000000/6, /* 1.666 MHz */
	{ 30, 30 },
	{ 0, exerion_porta_r },
	{ 0 },
	{ 0 },
	{ 0, exerion_portb_w }
};



/*************************************
 *
 *	Machine drivers
 *
 *************************************/

static MACHINE_DRIVER_START( exerion )

	MDRV_CPU_ADD(Z80, 10000000/3)
	MDRV_CPU_MEMORY(readmem,writemem)
	MDRV_CPU_VBLANK_INT(exerion_interrupt,1)

	MDRV_CPU_ADD(Z80, 10000000/3)
	MDRV_CPU_MEMORY(cpu2_readmem,cpu2_writemem)

	MDRV_FRAMES_PER_SECOND(60)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(64*8, 32*8)
	MDRV_VISIBLE_AREA(12*8, 52*8-1, 2*8, 30*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(32)
	MDRV_COLORTABLE_LENGTH(256*3)

	MDRV_PALETTE_INIT(exerion)
	MDRV_VIDEO_START(exerion)
	MDRV_VIDEO_UPDATE(exerion)

	/* sound hardware */
	MDRV_SOUND_ADD(AY8910, ay8910_interface)
MACHINE_DRIVER_END



/*************************************
 *
 *	ROM definitions
 *
 *************************************/

ROM_START( exerion )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )     /* 64k for code */
	ROM_LOAD( "exerion.07",   0x0000, 0x2000, CRC(4c78d57d) SHA1(ac702e9ad2bc05493fb1355858667c31c36acfe4) )
	ROM_LOAD( "exerion.08",   0x2000, 0x2000, CRC(dcadc1df) SHA1(91388f617cfaa4289ca1c84c697fcfdd8834ae15) )
	ROM_LOAD( "exerion.09",   0x4000, 0x2000, CRC(34cc4d14) SHA1(511c9de038f7bcaf6f7c96f2cbbe50a80673fa72) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )     /* 64k for the second CPU */
	ROM_LOAD( "exerion.05",   0x0000, 0x2000, CRC(32f6bff5) SHA1(a4d0289f9d1d9eea7ca9a32a0616af48da74b401) )

	ROM_REGION( 0x02000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "exerion.06",   0x00000, 0x2000, CRC(435a85a4) SHA1(f6846bfee11df754405d4d796e7d8ac0321b6eb6) ) /* fg chars */

	ROM_REGION( 0x04000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "exerion.11",   0x00000, 0x2000, CRC(f0633a09) SHA1(8989bcb12abadde34777f7c189cfa6e2dfe92d62) ) /* sprites */
	ROM_LOAD( "exerion.10",   0x02000, 0x2000, CRC(80312de0) SHA1(4fa3bb9d5c62e41a54e8909f8d3b47637137e913) )

	ROM_REGION( 0x08000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "exerion.03",   0x00000, 0x2000, CRC(790595b8) SHA1(8016ac2394b25db38e962bcff4805380082f6683) ) /* bg data */
	ROM_LOAD( "exerion.04",   0x02000, 0x2000, CRC(d7abd0b9) SHA1(ca6413ecd324cf84e11b703a4eda2c1e6d28ff15) )
	ROM_LOAD( "exerion.01",   0x04000, 0x2000, CRC(5bb755cb) SHA1(ec92c518c116a78dbb23381468cefb3f930212cc) )
	ROM_LOAD( "exerion.02",   0x06000, 0x2000, CRC(a7ecbb70) SHA1(3c359d5bb21290a45d3eb18fea2b1f9439b931be) )

	ROM_REGION( 0x0420, REGION_PROMS, 0 )
	ROM_LOAD( "exerion.e1",   0x0000, 0x0020, CRC(2befcc20) SHA1(a24d3f691413378fde545a6ddcef7e5118e74019) ) /* palette */
	ROM_LOAD( "exerion.i8",   0x0020, 0x0100, CRC(31db0e08) SHA1(1041a778e86d3fe6f057cf40a0a08b30760f3887) ) /* fg char lookup table */
	ROM_LOAD( "exerion.h10",  0x0120, 0x0100, CRC(63b4c555) SHA1(30243041be4fa77ada71e8b29d721cad51640c29) ) /* sprite lookup table */
	ROM_LOAD( "exerion.i3",   0x0220, 0x0100, CRC(fe72ab79) SHA1(048a72e6db4768df687df927acaa70ef906b3dc0) ) /* bg char lookup table */
	ROM_LOAD( "exerion.k4",   0x0320, 0x0100, CRC(ffc2ba43) SHA1(03be1c41d6ac3fc11439caef04ef5ffa60d6aec4) ) /* bg char mixer */
ROM_END


ROM_START( exeriont )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )     /* 64k for code */
	ROM_LOAD( "prom5.4p",     0x0000, 0x4000, CRC(58b4dc1b) SHA1(3e34d1eda0b0537dac1062e96259d4cc7c64049c) )
	ROM_LOAD( "prom6.4s",     0x4000, 0x2000, CRC(fca18c2d) SHA1(31077dada3ed4aa2e26af933f589e01e0c71e5cd) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )     /* 64k for the second CPU */
	ROM_LOAD( "exerion.05",   0x0000, 0x2000, CRC(32f6bff5) SHA1(a4d0289f9d1d9eea7ca9a32a0616af48da74b401) )

	ROM_REGION( 0x02000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "exerion.06",   0x00000, 0x2000, CRC(435a85a4) SHA1(f6846bfee11df754405d4d796e7d8ac0321b6eb6) ) /* fg chars */

	ROM_REGION( 0x04000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "exerion.11",   0x00000, 0x2000, CRC(f0633a09) SHA1(8989bcb12abadde34777f7c189cfa6e2dfe92d62) ) /* sprites */
	ROM_LOAD( "exerion.10",   0x02000, 0x2000, CRC(80312de0) SHA1(4fa3bb9d5c62e41a54e8909f8d3b47637137e913) )

	ROM_REGION( 0x08000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "exerion.03",   0x00000, 0x2000, CRC(790595b8) SHA1(8016ac2394b25db38e962bcff4805380082f6683) ) /* bg data */
	ROM_LOAD( "exerion.04",   0x02000, 0x2000, CRC(d7abd0b9) SHA1(ca6413ecd324cf84e11b703a4eda2c1e6d28ff15) )
	ROM_LOAD( "exerion.01",   0x04000, 0x2000, CRC(5bb755cb) SHA1(ec92c518c116a78dbb23381468cefb3f930212cc) )
	ROM_LOAD( "exerion.02",   0x06000, 0x2000, CRC(a7ecbb70) SHA1(3c359d5bb21290a45d3eb18fea2b1f9439b931be) )

	ROM_REGION( 0x0420, REGION_PROMS, 0 )
	ROM_LOAD( "exerion.e1",   0x0000, 0x0020, CRC(2befcc20) SHA1(a24d3f691413378fde545a6ddcef7e5118e74019) ) /* palette */
	ROM_LOAD( "exerion.i8",   0x0020, 0x0100, CRC(31db0e08) SHA1(1041a778e86d3fe6f057cf40a0a08b30760f3887) ) /* fg char lookup table */
	ROM_LOAD( "exerion.h10",  0x0120, 0x0100, CRC(63b4c555) SHA1(30243041be4fa77ada71e8b29d721cad51640c29) ) /* sprite lookup table */
	ROM_LOAD( "exerion.i3",   0x0220, 0x0100, CRC(fe72ab79) SHA1(048a72e6db4768df687df927acaa70ef906b3dc0) ) /* bg char lookup table */
	ROM_LOAD( "exerion.k4",   0x0320, 0x0100, CRC(ffc2ba43) SHA1(03be1c41d6ac3fc11439caef04ef5ffa60d6aec4) ) /* bg char mixer */
ROM_END


ROM_START( exerionb )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )     /* 64k for code */
	ROM_LOAD( "eb5.bin",      0x0000, 0x4000, CRC(da175855) SHA1(11ea46fd1d504e16e5ffc604d74c1ce210d6be1c) )
	ROM_LOAD( "eb6.bin",      0x4000, 0x2000, CRC(0dbe2eff) SHA1(5b0e5e8453619beec46c4350d1b2ed571fe3dc24) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )     /* 64k for the second CPU */
	ROM_LOAD( "exerion.05",   0x0000, 0x2000, CRC(32f6bff5) SHA1(a4d0289f9d1d9eea7ca9a32a0616af48da74b401) )

	ROM_REGION( 0x02000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "exerion.06",   0x00000, 0x2000, CRC(435a85a4) SHA1(f6846bfee11df754405d4d796e7d8ac0321b6eb6) ) /* fg chars */

	ROM_REGION( 0x04000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "exerion.11",   0x00000, 0x2000, CRC(f0633a09) SHA1(8989bcb12abadde34777f7c189cfa6e2dfe92d62) ) /* sprites */
	ROM_LOAD( "exerion.10",   0x02000, 0x2000, CRC(80312de0) SHA1(4fa3bb9d5c62e41a54e8909f8d3b47637137e913) )

	ROM_REGION( 0x08000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "exerion.03",   0x00000, 0x2000, CRC(790595b8) SHA1(8016ac2394b25db38e962bcff4805380082f6683) ) /* bg data */
	ROM_LOAD( "exerion.04",   0x02000, 0x2000, CRC(d7abd0b9) SHA1(ca6413ecd324cf84e11b703a4eda2c1e6d28ff15) )
	ROM_LOAD( "exerion.01",   0x04000, 0x2000, CRC(5bb755cb) SHA1(ec92c518c116a78dbb23381468cefb3f930212cc) )
	ROM_LOAD( "exerion.02",   0x06000, 0x2000, CRC(a7ecbb70) SHA1(3c359d5bb21290a45d3eb18fea2b1f9439b931be) )

	ROM_REGION( 0x0420, REGION_PROMS, 0 )
	ROM_LOAD( "exerion.e1",   0x0000, 0x0020, CRC(2befcc20) SHA1(a24d3f691413378fde545a6ddcef7e5118e74019) ) /* palette */
	ROM_LOAD( "exerion.i8",   0x0020, 0x0100, CRC(31db0e08) SHA1(1041a778e86d3fe6f057cf40a0a08b30760f3887) ) /* fg char lookup table */
	ROM_LOAD( "exerion.h10",  0x0120, 0x0100, CRC(63b4c555) SHA1(30243041be4fa77ada71e8b29d721cad51640c29) ) /* sprite lookup table */
	ROM_LOAD( "exerion.i3",   0x0220, 0x0100, CRC(fe72ab79) SHA1(048a72e6db4768df687df927acaa70ef906b3dc0) ) /* bg char lookup table */
	ROM_LOAD( "exerion.k4",   0x0320, 0x0100, CRC(ffc2ba43) SHA1(03be1c41d6ac3fc11439caef04ef5ffa60d6aec4) ) /* bg char mixer */
ROM_END


/*************************************
 *
 *	Driver initialization
 *
 *************************************/

static DRIVER_INIT( exerion )
{
	UINT32 oldaddr, newaddr, length;
	UINT8 *src, *dst, *temp;

	/* allocate some temporary space */
	temp = malloc(0x10000);
	if (!temp)
		return;

	/* make a temporary copy of the character data */
	src = temp;
	dst = memory_region(REGION_GFX1);
	length = memory_region_length(REGION_GFX1);
	memcpy(src, dst, length);

	/* decode the characters */
	/* the bits in the ROM are ordered: n8-n7 n6 n5 n4-v2 v1 v0 n3-n2 n1 n0 h2 */
	/* we want them ordered like this:  n8-n7 n6 n5 n4-n3 n2 n1 n0-v2 v1 v0 h2 */
	for (oldaddr = 0; oldaddr < length; oldaddr++)
	{
		newaddr = ((oldaddr     ) & 0x1f00) |       /* keep n8-n4 */
		          ((oldaddr << 3) & 0x00f0) |       /* move n3-n0 */
		          ((oldaddr >> 4) & 0x000e) |       /* move v2-v0 */
		          ((oldaddr     ) & 0x0001);        /* keep h2 */
		dst[newaddr] = src[oldaddr];
	}

	/* make a temporary copy of the sprite data */
	src = temp;
	dst = memory_region(REGION_GFX2);
	length = memory_region_length(REGION_GFX2);
	memcpy(src, dst, length);

	/* decode the sprites */
	/* the bits in the ROMs are ordered: n9 n8 n3 n7-n6 n5 n4 v3-v2 v1 v0 n2-n1 n0 h3 h2 */
	/* we want them ordered like this:   n9 n8 n7 n6-n5 n4 n3 n2-n1 n0 v3 v2-v1 v0 h3 h2 */
	for (oldaddr = 0; oldaddr < length; oldaddr++)
	{
		newaddr = ((oldaddr << 1) & 0x3c00) |       /* move n7-n4 */
		          ((oldaddr >> 4) & 0x0200) |       /* move n3 */
		          ((oldaddr << 4) & 0x01c0) |       /* move n2-n0 */
		          ((oldaddr >> 3) & 0x003c) |       /* move v3-v0 */
		          ((oldaddr     ) & 0xc003);        /* keep n9-n8 h3-h2 */
		dst[newaddr] = src[oldaddr];
	}

	free(temp);
}


static DRIVER_INIT( exerionb )
{
	UINT8 *ram = memory_region(REGION_CPU1);
	int addr;

	/* the program ROMs have data lines D1 and D2 swapped. Decode them. */
	for (addr = 0; addr < 0x6000; addr++)
		ram[addr] = (ram[addr] & 0xf9) | ((ram[addr] & 2) << 1) | ((ram[addr] & 4) >> 1);

	/* also convert the gfx as in Exerion */
	init_exerion();
}



/*************************************
 *
 *	Game drivers
 *
 *************************************/

GAME( 1983, exerion,  0,       exerion, exerion, exerion,  ROT90, "Jaleco", "Exerion" )
GAME( 1983, exeriont, exerion, exerion, exerion, exerion,  ROT90, "Jaleco (Taito America license)", "Exerion (Taito)" )
GAME( 1983, exerionb, exerion, exerion, exerion, exerionb, ROT90, "Jaleco", "Exerion (bootleg)" )
