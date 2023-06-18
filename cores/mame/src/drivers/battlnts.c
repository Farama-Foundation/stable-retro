/***************************************************************************

Battlantis(GX777) (c) 1987 Konami

Preliminary driver by:
	Manuel Abadia <manu@teleline.es>

***************************************************************************/

#include "driver.h"
#include "cpu/hd6309/hd6309.h"
#include "vidhrdw/generic.h"
#include "cpu/z80/z80.h"
#include "vidhrdw/konamiic.h"

/* from vidhrdw */
WRITE_HANDLER( battlnts_spritebank_w );
VIDEO_START( battlnts );
VIDEO_UPDATE( battlnts );

static INTERRUPT_GEN( battlnts_interrupt )
{
	if (K007342_is_INT_enabled())
		cpu_set_irq_line(0, HD6309_IRQ_LINE, HOLD_LINE);
}

WRITE_HANDLER( battlnts_sh_irqtrigger_w )
{
	cpu_set_irq_line_and_vector(1, 0, HOLD_LINE, 0xff);
}

static WRITE_HANDLER( battlnts_bankswitch_w )
{
	unsigned char *RAM = memory_region(REGION_CPU1);
	int bankaddress;

	/* bits 6 & 7 = bank number */
	bankaddress = 0x10000 + ((data & 0xc0) >> 6) * 0x4000;
	cpu_setbank(1,&RAM[bankaddress]);

	/* bits 4 & 5 = coin counters */
	coin_counter_w(0,data & 0x10);
	coin_counter_w(1,data & 0x20);

	/* other bits unknown */
}

static MEMORY_READ_START( battlnts_readmem )
	{ 0x0000, 0x1fff, K007342_r },			/* Color RAM + Video RAM */
	{ 0x2000, 0x21ff, K007420_r },			/* Sprite RAM */
	{ 0x2200, 0x23ff, K007342_scroll_r },	/* Scroll RAM */
	{ 0x2400, 0x24ff, paletteram_r },		/* Palette */
	{ 0x2e00, 0x2e00, input_port_0_r }, 	/* DIPSW #1 */
	{ 0x2e01, 0x2e01, input_port_4_r }, 	/* 2P controls */
	{ 0x2e02, 0x2e02, input_port_3_r }, 	/* 1P controls */
	{ 0x2e03, 0x2e03, input_port_2_r }, 	/* coinsw, testsw, startsw */
	{ 0x2e04, 0x2e04, input_port_1_r }, 	/* DISPW #2 */
	{ 0x4000, 0x7fff, MRA_BANK1 },			/* banked ROM */
	{ 0x8000, 0xffff, MRA_ROM },			/* ROM 777e02.bin */
MEMORY_END

static MEMORY_WRITE_START( battlnts_writemem )
	{ 0x0000, 0x1fff, K007342_w },				/* Color RAM + Video RAM */
	{ 0x2000, 0x21ff, K007420_w },				/* Sprite RAM */
	{ 0x2200, 0x23ff, K007342_scroll_w },		/* Scroll RAM */
	{ 0x2400, 0x24ff, paletteram_xBBBBBGGGGGRRRRR_swap_w, &paletteram },/* palette */
	{ 0x2600, 0x2607, K007342_vreg_w }, 		/* Video Registers */
	{ 0x2e08, 0x2e08, battlnts_bankswitch_w },	/* bankswitch control */
	{ 0x2e0c, 0x2e0c, battlnts_spritebank_w },	/* sprite bank select */
	{ 0x2e10, 0x2e10, watchdog_reset_w },		/* watchdog reset */
	{ 0x2e14, 0x2e14, soundlatch_w },			/* sound code # */
	{ 0x2e18, 0x2e18, battlnts_sh_irqtrigger_w },/* cause interrupt on audio CPU */
	{ 0x4000, 0x7fff, MWA_ROM },				/* banked ROM */
	{ 0x8000, 0xffff, MWA_ROM },				/* ROM 777e02.bin */
MEMORY_END

static MEMORY_READ_START( battlnts_readmem_sound )
	{ 0x0000, 0x7fff, MRA_ROM },				/* ROM 777c01.rom */
	{ 0x8000, 0x87ff, MRA_RAM },				/* RAM */
	{ 0xa000, 0xa000, YM3812_status_port_0_r }, /* YM3812 (chip 1) */
	{ 0xc000, 0xc000, YM3812_status_port_1_r }, /* YM3812 (chip 2) */
	{ 0xe000, 0xe000, soundlatch_r },			/* soundlatch_r */
MEMORY_END

static MEMORY_WRITE_START( battlnts_writemem_sound )
	{ 0x0000, 0x7fff, MWA_ROM },					/* ROM 777c01.rom */
	{ 0x8000, 0x87ff, MWA_RAM },					/* RAM */
	{ 0xa000, 0xa000, YM3812_control_port_0_w },	/* YM3812 (chip 1) */
	{ 0xa001, 0xa001, YM3812_write_port_0_w },		/* YM3812 (chip 1) */
	{ 0xc000, 0xc000, YM3812_control_port_1_w },	/* YM3812 (chip 2) */
	{ 0xc001, 0xc001, YM3812_write_port_1_w },		/* YM3812 (chip 2) */
MEMORY_END

/***************************************************************************

	Input Ports

***************************************************************************/

INPUT_PORTS_START( battlnts )
	PORT_START	/* DSW #1 */
	PORT_DIPNAME( 0x0f, 0x0f, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(	0x02, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(	0x05, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(	0x08, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(	0x04, DEF_STR( 3C_2C ) )
	PORT_DIPSETTING(	0x01, DEF_STR( 4C_3C ) )
	PORT_DIPSETTING(	0x0f, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(	0x03, DEF_STR( 3C_4C ) )
	PORT_DIPSETTING(	0x07, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(	0x0e, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(	0x06, DEF_STR( 2C_5C ) )
	PORT_DIPSETTING(	0x0d, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(	0x0c, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(	0x0b, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(	0x0a, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(	0x09, DEF_STR( 1C_7C ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0xf0, 0xf0, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(	0x20, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(	0x50, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(	0x80, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(	0x40, DEF_STR( 3C_2C ) )
	PORT_DIPSETTING(	0x10, DEF_STR( 4C_3C ) )
	PORT_DIPSETTING(	0xf0, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(	0x30, DEF_STR( 3C_4C ) )
	PORT_DIPSETTING(	0x70, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(	0xe0, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(	0x60, DEF_STR( 2C_5C ) )
	PORT_DIPSETTING(	0xd0, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(	0xc0, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(	0xb0, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(	0xa0, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(	0x90, DEF_STR( 1C_7C ) )
/*	PORT_DIPSETTING(	0x00, "Invalid" )*/

	PORT_START	/* DSW #2 */
	PORT_DIPNAME( 0x03, 0x02, DEF_STR( Lives ) )
	PORT_DIPSETTING(	0x03, "2" )
	PORT_DIPSETTING(	0x02, "3" )
	PORT_DIPSETTING(	0x01, "5" )
	PORT_DIPSETTING(	0x00, "7" )
	PORT_DIPNAME( 0x04, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(	0x04, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x18, 0x10, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(	0x18, "30k and every 70k" )
	PORT_DIPSETTING(	0x10, "40k and every 80k" )
	PORT_DIPSETTING(	0x08, "40k" )
	PORT_DIPSETTING(	0x00, "50k" )
	PORT_DIPNAME( 0x60, 0x40, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(	0x60, "Easy" )
	PORT_DIPSETTING(	0x40, "Normal" )
	PORT_DIPSETTING(	0x20, "Difficult" )
	PORT_DIPSETTING(	0x00, "Very Difficult" )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(	0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )

	PORT_START	/* COINSW */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START2 )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(	0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, "Upright Controls" )
	PORT_DIPSETTING(	0x40, "Single" )
	PORT_DIPSETTING(	0x00, "Dual" )
	PORT_SERVICE( 0x80, IP_ACTIVE_LOW )

	PORT_START	/* PLAYER 1 INPUTS */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP	  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_DIPNAME( 0x80, 0x80, "Continue limit" )
	PORT_DIPSETTING(	0x80, "3" )
	PORT_DIPSETTING(	0x00, "5" )

	PORT_START	/* PLAYER 2 INPUTS */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP	  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )
INPUT_PORTS_END

INPUT_PORTS_START( thehustj )
	PORT_START	/* DSW #1 */
	PORT_DIPNAME( 0x0f, 0x0f, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(	0x02, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(	0x05, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(	0x08, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(	0x04, DEF_STR( 3C_2C ) )
	PORT_DIPSETTING(	0x01, DEF_STR( 4C_3C ) )
	PORT_DIPSETTING(	0x0f, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(	0x03, DEF_STR( 3C_4C ) )
	PORT_DIPSETTING(	0x07, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(	0x0e, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(	0x06, DEF_STR( 2C_5C ) )
	PORT_DIPSETTING(	0x0d, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(	0x0c, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(	0x0b, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(	0x0a, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(	0x09, DEF_STR( 1C_7C ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0xf0, 0xf0, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(	0x20, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(	0x50, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(	0x80, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(	0x40, DEF_STR( 3C_2C ) )
	PORT_DIPSETTING(	0x10, DEF_STR( 4C_3C ) )
	PORT_DIPSETTING(	0xf0, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(	0x30, DEF_STR( 3C_4C ) )
	PORT_DIPSETTING(	0x70, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(	0xe0, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(	0x60, DEF_STR( 2C_5C ) )
	PORT_DIPSETTING(	0xd0, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(	0xc0, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(	0xb0, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(	0xa0, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(	0x90, DEF_STR( 1C_7C ) )
/*	PORT_DIPSETTING(	0x00, "Invalid" )*/

	PORT_START	/* DSW #2 */
	PORT_DIPNAME( 0x03, 0x02, "Balls" )
	PORT_DIPSETTING(	0x03, "1" )
	PORT_DIPSETTING(	0x02, "2" )
	PORT_DIPSETTING(	0x01, "3" )
	PORT_DIPSETTING(	0x00, "6" )
	PORT_DIPNAME( 0x04, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(	0x04, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(	0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(	0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x60, 0x40, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(	0x60, "Easy" )
	PORT_DIPSETTING(	0x40, "Normal" )
	PORT_DIPSETTING(	0x20, "Difficult" )
	PORT_DIPSETTING(	0x00, "Very Difficult" )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(	0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )

	PORT_START	/* COINSW */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START2 )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(	0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(	0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_SERVICE( 0x80, IP_ACTIVE_LOW )

	PORT_START	/* PLAYER 1 INPUTS */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP	  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(	0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )

	PORT_START	/* PLAYER 2 INPUTS */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP	  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )
INPUT_PORTS_END


static struct GfxLayout charlayout =
{
	8,8,			/* 8 x 8 characters */
	0x40000/32, 	/* 8192 characters */
	4,				/* 4bpp */
	{ 0, 1, 2, 3 }, /* the four bitplanes are packed in one nibble */
	{ 2*4, 3*4, 0*4, 1*4, 6*4, 7*4, 4*4, 5*4 },
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32 },
	32*8			/* every character takes 32 consecutive bytes */
};

static struct GfxLayout spritelayout =
{
	8,8,			/* 8*8 sprites */
	0x40000/32, /* 8192 sprites */
	4,				/* 4 bpp */
	{ 0, 1, 2, 3 }, /* the four bitplanes are packed in one nibble */
	{ 0*4, 1*4, 2*4, 3*4, 4*4, 5*4, 6*4, 7*4},
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32 },
	32*8			/* every sprite takes 32 consecutive bytes */
};


static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &charlayout,		0, 1 }, /* colors  0-15 */
	{ REGION_GFX2, 0, &spritelayout, 4*16, 1 }, /* colors 64-79 */
	{ -1 } /* end of array */
};

/***************************************************************************

	Machine Driver

***************************************************************************/

static struct YM3812interface ym3812_interface =
{
	2,				/* 2 chips */
	3000000,		/* ? */
	{ 100, 100 },
	{ 0, 0 },
};

static MACHINE_DRIVER_START( battlnts )

	/* basic machine hardware */
	MDRV_CPU_ADD(HD6309, 3000000)		/* ? */
	MDRV_CPU_MEMORY(battlnts_readmem,battlnts_writemem)
	MDRV_CPU_VBLANK_INT(battlnts_interrupt,1)

	MDRV_CPU_ADD(Z80, 3579545)
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)		/* ? */
	MDRV_CPU_MEMORY(battlnts_readmem_sound,battlnts_writemem_sound)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 2*8, 30*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(128)

	MDRV_VIDEO_START(battlnts)
	MDRV_VIDEO_UPDATE(battlnts)

	/* sound hardware */
	MDRV_SOUND_ADD(YM3812, ym3812_interface)
MACHINE_DRIVER_END


/***************************************************************************

  Game ROMs

***************************************************************************/

ROM_START( battlnts )
	ROM_REGION( 0x20000, REGION_CPU1, 0 ) /* code + banked roms */
	ROM_LOAD( "g02.7e",      0x08000, 0x08000, CRC(dbd8e17e) SHA1(586a22b714011c67a915c4a350ceca19ff875635) ) /* fixed ROM */
	ROM_LOAD( "g03.8e",      0x10000, 0x10000, CRC(7bd44fef) SHA1(308ec5246f5537b34e368535672ac687f456750a) ) /* banked ROM */

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* 64k for the sound CPU */
	ROM_LOAD( "777c01.bin",  0x00000, 0x08000, CRC(c21206e9) SHA1(7b133e04be67dc061a186ab0481d848b69b370d7) )

	ROM_REGION( 0x40000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "777c04.bin",  0x00000, 0x40000, CRC(45d92347) SHA1(8537b4ccd0a80ea3260ef82fde177f1d65a49c03) ) /* tiles */

	ROM_REGION( 0x40000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "777c05.bin",  0x00000, 0x40000, CRC(aeee778c) SHA1(fc58ada9c97361d13439b7b0918c947d48402445) ) /* sprites */
ROM_END

ROM_START( battlntj )
	ROM_REGION( 0x20000, REGION_CPU1, 0 ) /* code + banked roms */
	ROM_LOAD( "777e02.bin",  0x08000, 0x08000, CRC(d631cfcb) SHA1(7787da0dd8cd218abc27204e517e04d7a1913a3b) ) /* fixed ROM */
	ROM_LOAD( "777e03.bin",  0x10000, 0x10000, CRC(5ef1f4ef) SHA1(e3e6e1fc5a65328d94c23e2e76eef3504b70e58b) ) /* banked ROM */

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* 64k for the sound CPU */
	ROM_LOAD( "777c01.bin",  0x00000, 0x08000, CRC(c21206e9) SHA1(7b133e04be67dc061a186ab0481d848b69b370d7) )

	ROM_REGION( 0x40000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "777c04.bin",  0x00000, 0x40000, CRC(45d92347) SHA1(8537b4ccd0a80ea3260ef82fde177f1d65a49c03) ) /* tiles */

	ROM_REGION( 0x40000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "777c05.bin",  0x00000, 0x40000, CRC(aeee778c) SHA1(fc58ada9c97361d13439b7b0918c947d48402445) ) /* sprites */
ROM_END

ROM_START( thehustl )
	ROM_REGION( 0x20000, REGION_CPU1, 0 ) /* code + banked roms */
	ROM_LOAD( "765-m02.7e",  0x08000, 0x08000, CRC(934807b9) SHA1(84e13a5c1587ee28330f369f9a1180219edbda9d) ) /* fixed ROM */
	ROM_LOAD( "765-j03.8e",  0x10000, 0x10000, CRC(a13fd751) SHA1(27ec66835c85b7ac0221a813d38e9cca0d9be3b8) ) /* banked ROM */

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* 64k for the sound CPU */
	ROM_LOAD( "765-j01.10a", 0x00000, 0x08000, CRC(77ae753e) SHA1(9e463a825d31bb79644b083d24b25670d96441c5) )

	ROM_REGION( 0x40000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "765-e04.13a", 0x00000, 0x40000, CRC(08c2b72e) SHA1(02d9c690da839d6fee75fffdf66a4d3da35a0263) ) /* tiles */

	ROM_REGION( 0x40000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "765-e05.13e", 0x00000, 0x40000, CRC(ef044655) SHA1(c8272283eab8fc2899979da398819cb72c92a299) ) /* sprites */
ROM_END

ROM_START( thehustj )
	ROM_REGION( 0x20000, REGION_CPU1, 0 ) /* code + banked roms */
	ROM_LOAD( "765-j02.7e",  0x08000, 0x08000, CRC(2ac14c75) SHA1(b88f6279ab88719f4207e28486a0022554668382) ) /* fixed ROM */
	ROM_LOAD( "765-j03.8e",  0x10000, 0x10000, CRC(a13fd751) SHA1(27ec66835c85b7ac0221a813d38e9cca0d9be3b8) ) /* banked ROM */

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* 64k for the sound CPU */
	ROM_LOAD( "765-j01.10a", 0x00000, 0x08000, CRC(77ae753e) SHA1(9e463a825d31bb79644b083d24b25670d96441c5) )

	ROM_REGION( 0x40000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "765-e04.13a", 0x00000, 0x40000, CRC(08c2b72e) SHA1(02d9c690da839d6fee75fffdf66a4d3da35a0263) ) /* tiles */

	ROM_REGION( 0x40000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "765-e05.13e", 0x00000, 0x40000, CRC(ef044655) SHA1(c8272283eab8fc2899979da398819cb72c92a299) ) /* sprites */
ROM_END

ROM_START( rackemup )
	ROM_REGION( 0x20000, REGION_CPU1, 0 ) /* code + banked roms */
	ROM_LOAD( "765l02",      0x08000, 0x08000, CRC(3dfc48bd) SHA1(9ba98e9f27dd0a6efec145bea2a5ae7df8567437) ) /* fixed ROM */
	ROM_LOAD( "765-j03.8e",  0x10000, 0x10000, CRC(a13fd751) SHA1(27ec66835c85b7ac0221a813d38e9cca0d9be3b8) ) /* banked ROM */

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* 64k for the sound CPU */
	ROM_LOAD( "765-j01.10a", 0x00000, 0x08000, CRC(77ae753e) SHA1(9e463a825d31bb79644b083d24b25670d96441c5) )

	ROM_REGION( 0x40000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "765l04",      0x00000, 0x40000, CRC(acfbeee2) SHA1(c2bf750892ba33d4610fa4497170f49c101ed4c1) ) /* tiles */

	ROM_REGION( 0x40000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "765l05",      0x00000, 0x40000, CRC(1bb6855f) SHA1(251081564dfede8fa9a422081d58465fe5ca4ed1) ) /* sprites */
ROM_END


/*
	This recursive function doesn't use additional memory
	(it could be easily converted into an iterative one).
	It's called shuffle because it mimics the shuffling of a deck of cards.
*/
static void shuffle(UINT8 *buf,int len)
{
	int i;
	UINT8 t;

	if (len == 2) return;

	if (len % 4) exit(1);	/* must not happen */

	len /= 2;

	for (i = 0;i < len/2;i++)
	{
		t = buf[len/2 + i];
		buf[len/2 + i] = buf[len + i];
		buf[len + i] = t;
	}

	shuffle(buf,len);
	shuffle(buf + len,len);
}


static DRIVER_INIT( rackemup )
{
	/* rearrange char ROM */
	shuffle(memory_region(REGION_GFX1),memory_region_length(REGION_GFX1));
}



GAME( 1987, battlnts, 0,		battlnts, battlnts, 0,		  ROT90, "Konami", "Battlantis" )
GAME( 1987, battlntj, battlnts, battlnts, battlnts, 0,		  ROT90, "Konami", "Battlantis (Japan)" )
GAMEX(1987, thehustl, 0,		battlnts, thehustj, 0,		  ROT90, "Konami", "The Hustler (Japan version M)", GAME_NO_COCKTAIL )
GAMEX(1987, thehustj, thehustl, battlnts, thehustj, 0,		  ROT90, "Konami", "The Hustler (Japan version J)", GAME_NO_COCKTAIL )
GAMEX(1987, rackemup, thehustl, battlnts, thehustj, rackemup, ROT90, "Konami", "Rack 'em Up", GAME_NO_COCKTAIL )
