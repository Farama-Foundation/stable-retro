/***************************************************************************

Thunder Hoop II: Strikes Back (c) 1994 Gaelco

Driver by Manuel Abadia <manu@teleline.es>

Very similar to maniacsq and biomtoy but protected :_(
The DS5002FP has up to 128 KB undumped gameplay code

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "cpu/m68000/m68000.h"

extern data16_t *thoop2_vregs;
extern data16_t *thoop2_videoram;
extern data16_t *thoop2_spriteram;

/* from vidhrdw/thoop2.c */
WRITE16_HANDLER( thoop2_vram_w );
VIDEO_START( thoop2 );
VIDEO_UPDATE( thoop2 );


static struct GfxLayout thoop2_tilelayout =
{
	8,8,									/* 8x8 tiles */
	0x400000/16,							/* number of tiles */
	4,										/* 4 bpp */
	{ 0*0x400000*8+8, 0*0x400000*8, 1*0x400000*8+8, 1*0x400000*8 },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16 },
	16*8
};

static struct GfxLayout thoop2_tilelayout_16 =
{
	16,16,									/* 16x16 tiles */
	0x400000/64,							/* number of tiles */
	4,										/* 4 bpp */
	{ 0*0x400000*8+8, 0*0x400000*8, 1*0x400000*8+8, 1*0x400000*8 },
	{ 0, 1, 2, 3, 4, 5, 6, 7,
		16*16+0, 16*16+1, 16*16+2, 16*16+3, 16*16+4, 16*16+5, 16*16+6, 16*16+7 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16,
	  8*16, 9*16, 10*16, 11*16, 12*16, 13*16, 14*16, 15*16, },
	64*8
};


static struct GfxDecodeInfo thoop2_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0x000000, &thoop2_tilelayout, 0,		64 },
	{ REGION_GFX1, 0x000000, &thoop2_tilelayout_16, 0,	64 },
	{ -1 }
};


static MEMORY_READ16_START( thoop2_readmem )
	{ 0x000000, 0x0fffff, MRA16_ROM },			/* ROM */
	{ 0x100000, 0x101fff, MRA16_RAM },			/* Video RAM */
	{ 0x200000, 0x2007ff, MRA16_RAM },			/* Palette */
	{ 0x440000, 0x440fff, MRA16_RAM },			/* Sprite RAM */
	{ 0x700000, 0x700001, input_port_0_word_r },/* DIPSW #2 */
	{ 0x700002, 0x700003, input_port_1_word_r },/* DIPSW #1 */
	{ 0x700004, 0x700005, input_port_2_word_r },/* INPUT #1 */
	{ 0x700006, 0x700007, input_port_3_word_r },/* INPUT #2 */
	{ 0x700008, 0x700009, input_port_4_word_r },/* INPUT #3 */
	{ 0x70000e, 0x70000f, OKIM6295_status_0_lsb_r },/* OKI6295 status register */
	{ 0xfe0000, 0xfeffff, MRA16_RAM },			/* Work RAM (partially shared with DS5002FP) */
MEMORY_END

static WRITE16_HANDLER( OKIM6295_bankswitch_w )
{
	unsigned char *RAM = memory_region(REGION_SOUND1);

	if (ACCESSING_LSB){
		memcpy(&RAM[0x30000], &RAM[0x40000 + (data & 0x0f)*0x10000], 0x10000);
	}
}

WRITE16_HANDLER( thoop2_coin_w )
{
	if (ACCESSING_LSB){
		switch ((offset >> 3)){
			case 0x00:	/* Coin Lockouts */
			case 0x01:
				coin_lockout_w((offset >> 3) & 0x01, ~data & 0x01);
				break;
			case 0x02:	/* Coin Counters */
			case 0x03:
				coin_counter_w((offset >> 3) & 0x01, data & 0x01);
				break;
		}
	}

	/* 04b unknown. Sound related? */
	/* 05b unknown */
}

static MEMORY_WRITE16_START( thoop2_writemem )
	{ 0x000000, 0x0fffff, MWA16_ROM },								/* ROM */
	{ 0x100000, 0x101fff, thoop2_vram_w, &thoop2_videoram },		/* Video RAM */
	{ 0x108000, 0x108007, MWA16_RAM, &thoop2_vregs },				/* Video Registers */
	{ 0x10800c, 0x10800d, watchdog_reset16_w },						/* INT 6 ACK/Watchdog timer */
	{ 0x200000, 0x2007ff, paletteram16_xBBBBBGGGGGRRRRR_word_w, &paletteram16 },/* Palette */
	{ 0x440000, 0x440fff, MWA16_RAM, &thoop2_spriteram },			/* Sprite RAM */
	{ 0x70000c, 0x70000d, OKIM6295_bankswitch_w },					/* OKI6295 bankswitch */
	{ 0x70000e, 0x70000f, OKIM6295_data_0_lsb_w },					/* OKI6295 data register */
	{ 0x70000a, 0x70005b, thoop2_coin_w },							/* Coin Counters + Coin Lockout */
	{ 0xfe0000, 0xfeffff, MWA16_RAM },								/* Work RAM (partially shared with DS5002FP) */
MEMORY_END


INPUT_PORTS_START( thoop2 )

PORT_START	/* DSW #2 */
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x03, "Normal" )
	PORT_DIPSETTING(    0x02, "Easy" )
	PORT_DIPSETTING(    0x01, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x18, 0x18, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x18, "2" )
	PORT_DIPSETTING(    0x10, "3" )
	PORT_DIPSETTING(    0x08, "4" )
	PORT_DIPSETTING(    0x00, "1" )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x20, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_SERVICE( 0x80, IP_ACTIVE_LOW )


PORT_START	/* DSW #1 */
	PORT_DIPNAME( 0x07, 0x07, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 6C_1C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x05, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 3C_2C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 4C_3C ) )
	PORT_DIPSETTING(    0x07, DEF_STR( 1C_1C ) )
	PORT_DIPNAME( 0x38, 0x38, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x38, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 3C_4C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x28, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x18, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 1C_6C ) )
	PORT_DIPNAME( 0x40, 0x40, "Credit configuration" )
	PORT_DIPSETTING(    0x40, "Start 1C/Continue 1C" )
	PORT_DIPSETTING(    0x00, "Start 2C/Continue 1C" )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Free_Play ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

PORT_START	/* 1P INPUTS & COINSW */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN2 )

PORT_START	/* 2P INPUTS & STARTSW */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START2 )

PORT_START	/* INPUTS, TEST & SERVICE */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_SERVICE2 )	/* test button */
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )
INPUT_PORTS_END


static struct OKIM6295interface thoop2_okim6295_interface =
{
	1,                  /* 1 chip */
	{ 8000 },			/* 8000 KHz? */
	{ REGION_SOUND1 },  /* memory region */
	{ 100 }				/* volume */
};

static MACHINE_DRIVER_START( thoop2 )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000,24000000/2)			/* 12 MHz */
	MDRV_CPU_MEMORY(thoop2_readmem,thoop2_writemem)
	MDRV_CPU_VBLANK_INT(irq6_line_hold,1)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(32*16, 32*16)
	MDRV_VISIBLE_AREA(0, 320-1, 16, 256-1)
	MDRV_GFXDECODE(thoop2_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(1024)

	MDRV_VIDEO_START(thoop2)
	MDRV_VIDEO_UPDATE(thoop2)

	/* sound hardware */
	MDRV_SOUND_ADD(OKIM6295, thoop2_okim6295_interface)
MACHINE_DRIVER_END


ROM_START( thoop2 )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )	/* 68000 code */
	ROM_LOAD16_BYTE(	"th2c23.040",	0x000000, 0x080000, CRC(3e465753) SHA1(1ea1173b9fe5d652e7b5fafb822e2535cecbc198) )
	ROM_LOAD16_BYTE(	"th2c22.040",	0x000001, 0x080000, CRC(837205b7) SHA1(f78b90c2be0b4dddaba26f074ea00eff863cfdb2) )

	ROM_REGION( 0x800000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "th2-h8.32m",		0x000000, 0x400000, CRC(60328a11) SHA1(fcdb374d2fc7ef5351a4181c471d192199dc2081) )
	ROM_LOAD( "th2-h12.32m",	0x400000, 0x400000, CRC(b25c2d3e) SHA1(d70f3e4e2432d80c2ac87cd81208ada303bac04a) )

	ROM_REGION( 0x140000, REGION_SOUND1, 0 )	/* ADPCM samples - sound chip is OKIM6295 */
	ROM_LOAD( "th2-c1.080",		0x000000, 0x100000, CRC(8fac8c30) SHA1(8e49bb596144761eae95f3e1266e57fb386664f2) )
	ROM_RELOAD(					0x040000, 0x100000 )
	/* 0x00000-0x2ffff is fixed, 0x30000-0x3ffff is bank switched */
ROM_END

GAMEX( 1994, thoop2,  0, thoop2, thoop2,  0, ROT0, "Gaelco", "TH Strikes Back", GAME_UNEMULATED_PROTECTION )
