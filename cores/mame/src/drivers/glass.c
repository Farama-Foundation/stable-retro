/***************************************************************************

Glass (c) 1993 Gaelco (Developed by OMK. Produced by Gaelco)

Driver by Manuel Abadia <manu@teleline.es>

The DS5002FP has up to 128KB undumped gameplay code making the game unplayable :_(

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "cpu/m68000/m68000.h"

extern data16_t *glass_vregs;
extern data16_t *glass_videoram;
extern data16_t *glass_spriteram;
extern int glass_current_bit;

/* from vidhrdw/glass.c */
WRITE16_HANDLER( glass_vram_w );
WRITE16_HANDLER( glass_blitter_w );
VIDEO_START( glass );
VIDEO_UPDATE( glass );

static int cause_interrupt;

static MACHINE_INIT( glass )
{
	cause_interrupt = 1;
	glass_current_bit = 0;
}

static WRITE16_HANDLER( clr_int_w )
{
	cause_interrupt = 1;
}

static INTERRUPT_GEN( glass_interrupt )
{
	if (cause_interrupt){
		cpu_set_irq_line(0, 6, HOLD_LINE);
		cause_interrupt = 0;
	}
}


static struct GfxLayout glass_tilelayout16 =
{
	16,16,									/* 16x16 tiles */
	0x100000/32,							/* number of tiles */
	4,										/* 4 bpp */
	{ 3*0x100000*8, 2*0x100000*8, 1*0x100000*8, 0*0x100000*8 },
	{	
		0, 1, 2, 3, 4, 5, 6, 7, 
		16*8+0, 16*8+1, 16*8+2, 16*8+3, 16*8+4, 16*8+5, 16*8+6, 16*8+7
	},
	{ 
		0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8, 
		8*8, 9*8, 10*8, 11*8, 12*8, 13*8, 14*8, 15*8 
	},
	32*8
};

static struct GfxDecodeInfo glass_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0x000000, &glass_tilelayout16, 0, 64 },
	{ -1 }
};


static MEMORY_READ16_START( glass_readmem )
	{ 0x000000, 0x0fffff, MRA16_ROM },				/* ROM */
	{ 0x100000, 0x101fff, MRA16_RAM },				/* Video RAM */
	{ 0x102000, 0x102fff, MRA16_RAM },				/* Extra Video RAM */
	{ 0x200000, 0x2007ff, MRA16_RAM },				/* Palette */
	{ 0x440000, 0x440fff, MRA16_RAM },				/* Sprite RAM */
	{ 0x700000, 0x700001, input_port_0_word_r },	/* DIPSW #2 */
	{ 0x700002, 0x700003, input_port_1_word_r },	/* DIPSW #1 */
	{ 0x700004, 0x700005, input_port_2_word_r },	/* 1P Inputs */
	{ 0x700006, 0x700007, input_port_3_word_r },	/* 2P Inputs + Button 3 */
	{ 0x70000e, 0x70000f, OKIM6295_status_0_lsb_r },/* OKI6295 status register */
	{ 0xfec000, 0xfeffff, MRA16_RAM },				/* Work RAM (partially shared with DS5002FP) */
MEMORY_END


static WRITE16_HANDLER( OKIM6295_bankswitch_w )
{
	unsigned char *RAM = memory_region(REGION_SOUND1);

	if (ACCESSING_LSB){
		memcpy(&RAM[0x30000], &RAM[0x40000 + (data & 0x0f)*0x10000], 0x10000);
	}
}

static WRITE16_HANDLER( glass_coin_w )
{
	switch (offset >> 3){
		case 0x00:	/* Coin Lockouts */
		case 0x01:
			coin_lockout_w((offset >> 3) & 0x01, ~data & 0x01);
			break;
		case 0x02:	/* Coin Counters */
		case 0x03:
			coin_counter_w((offset >> 3) & 0x01, data & 0x01);
			break;
		case 0x04:	/* Sound Muting (if bit 0 == 1, sound output stream = 0) */
			break;
	}
}

static MEMORY_WRITE16_START( glass_writemem )
	{ 0x000000, 0x0fffff, MWA16_ROM },								/* ROM */
	{ 0x100000, 0x101fff, glass_vram_w, &glass_videoram },			/* Video RAM */
	{ 0x102000, 0x102fff, MWA16_RAM },								/* Extra Video RAM */
	{ 0x108000, 0x108007, MWA16_RAM, &glass_vregs },				/* Video Registers */
	{ 0x108008, 0x108009, clr_int_w },								/* CLR INT Video */
	{ 0x200000, 0x2007ff, paletteram16_xBBBBBGGGGGRRRRR_word_w, &paletteram16 },/*  Palette */
	{ 0x440000, 0x440fff, MWA16_RAM, &glass_spriteram },			/* Sprite RAM */
	{ 0x700008, 0x700009, glass_blitter_w },						/* serial blitter */
	{ 0x70000c, 0x70000d, OKIM6295_bankswitch_w },					/* OKI6295 bankswitch */
	{ 0x70000e, 0x70000f, OKIM6295_data_0_lsb_w },					/* OKI6295 data register */
	{ 0x70000a, 0x70004b, glass_coin_w },							/* Coin Counters/Lockout */
	{ 0xfec000, 0xfeffff, MWA16_RAM },								/* Work RAM (partially shared with DS5002FP) */
MEMORY_END


INPUT_PORTS_START( glass )
PORT_START	/* DSW #2 */
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x02, "Easy" )
	PORT_DIPSETTING(    0x03, "Normal" )
	PORT_DIPSETTING(    0x01, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x0c, "3" )
	PORT_DIPSETTING(    0x08, "1" )
	PORT_DIPSETTING(    0x04, "2" )
	PORT_DIPSETTING(    0x00, "4" )
	PORT_DIPNAME( 0x10, 0x10, "Version" )
	PORT_DIPSETTING(    0x10, "Normal" )
	PORT_DIPSETTING(    0x00, "Light" )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x20, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_SERVICE( 0x80, IP_ACTIVE_LOW )
	
PORT_START	/* DSW #1 */
	PORT_DIPNAME( 0x07, 0x07, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x07, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 3C_4C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x05, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 1C_6C ) )
	PORT_DIPNAME( 0x38, 0x38, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 6C_1C ) )
	PORT_DIPSETTING(    0x18, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x28, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 3C_2C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 4C_3C ) )
	PORT_DIPSETTING(    0x38, DEF_STR( 1C_1C ) )
	PORT_DIPNAME( 0x40, 0x40, "Credit configuration" )
	PORT_DIPSETTING(    0x40, "Start 1C" )
	PORT_DIPSETTING(    0x00, "Start 2C" )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Free_Play ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	
PORT_START	/* 1P INPUTS */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN2 )

PORT_START	/* 2P INPUTS + Button 3 */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER1 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )
	PORT_BIT( 0xfc00, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END



static struct OKIM6295interface glass_okim6295_interface =
{
	1,                  /* 1 chip */
	{ 8000 },			/* 8000 KHz? */
	{ REGION_SOUND1 },  /* memory region */
	{ 100 }				/* volume */
};

static MACHINE_DRIVER_START( glass )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000,24000000/2)		/* 12 MHz (M680000 P12) */
	MDRV_CPU_MEMORY(glass_readmem,glass_writemem)
	MDRV_CPU_VBLANK_INT(glass_interrupt, 1)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)

	MDRV_MACHINE_INIT(glass)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(32*16, 32*16)
	MDRV_VISIBLE_AREA(0, 368-1, 16, 256-1)
	MDRV_GFXDECODE(glass_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(1024)

	MDRV_VIDEO_START(glass)
	MDRV_VIDEO_UPDATE(glass)

	/* sound hardware */
	MDRV_SOUND_ADD(OKIM6295, glass_okim6295_interface)
MACHINE_DRIVER_END


ROM_START( glass ) /* Version 1.1 */
	ROM_REGION( 0x100000, REGION_CPU1, 0 )    /* 68000 code */
	ROM_LOAD16_BYTE( "1.c23", 0x000000, 0x040000, CRC(aeebd4ed) SHA1(04759dc146dff0fc74b78d70e79dfaebe68328f9) )
	ROM_LOAD16_BYTE( "2.c22", 0x000001, 0x040000, CRC(165e2e01) SHA1(180a2e2b5151f2321d85ac23eff7fbc9f52023a5) )

/*	ROM_REGION( 0x8000, "gaelco_ds5002fp:sram", 0 ) DS5002FP code
	ROM_LOAD( "glass_ds5002fp_sram.bin", 0x00000, 0x8000, CRC(47c9df4c) SHA1(e0ac4f3d3086a4e8164d42aaae125037c222118a) )

	ROM_REGION( 0x100, "gaelco_ds5002fp:mcu:internal", ROMREGION_ERASE00 )
	these are the default states stored in NVRAM
	DS5002FP_SET_MON( 0x29 )
	DS5002FP_SET_RPCTL( 0x00 )
	DS5002FP_SET_CRCR( 0x80 )
*/
	ROM_REGION( 0x400000, REGION_GFX1, ROMREGION_DISPOSE )	/* Graphics */
	/* 0x000000-0x3fffff filled in later in the DRIVER_INIT */

	ROM_REGION( 0x400000, REGION_GFX2, ROMREGION_DISPOSE )	/* Graphics */
	ROM_LOAD( "h13.bin", 0x000000, 0x200000, CRC(13ab7f31) SHA1(468424f74d6cccd1b445a9f20e2d24bc46d61ed6) )
	ROM_LOAD( "h11.bin", 0x200000, 0x200000, CRC(c6ac41c8) SHA1(22408ef1e35c66d0fba0c72972c46fad891d1193) )
	
	ROM_REGION( 0x100000, REGION_GFX3, 0 )   /* 16 bitmaps (320x200, indexed colors) */
	ROM_LOAD( "h9.bin", 0x000000, 0x100000, CRC(b9492557) SHA1(3f5c0d696d65e1cd492763dfa749c813dd56a9bf) )

	ROM_REGION( 0x140000, REGION_SOUND1, 0 )    /* ADPCM samples - sound chip is OKIM6295 */
	ROM_LOAD( "c1.bin", 0x000000, 0x100000, CRC(d9f075a2) SHA1(31a7a677861f39d512e9d1f51925c689e481159a) )
	/* 0x00000-0x2ffff is fixed, 0x30000-0x3ffff is bank switched from all the ROMs */
	ROM_RELOAD(         0x040000, 0x100000 )
ROM_END


ROM_START( glasskr )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )    /* 68000 code */
	ROM_LOAD16_BYTE( "glassk.c23", 0x000000, 0x080000, CRC(6ee19376) SHA1(8a8fdeebe094bd3e29c35cf59584e3cab708732d) )
	ROM_LOAD16_BYTE( "glassk.c22", 0x000001, 0x080000, CRC(bd546568) SHA1(bcd5e7591f4e68c9470999b8a0ef1ee4392c907c) )

	ROM_REGION( 0x400000, REGION_GFX1, ROMREGION_DISPOSE )	/* Graphics */
	/* 0x000000-0x3fffff filled in later in the DRIVER_INIT */

	ROM_REGION( 0x400000, REGION_GFX2, ROMREGION_DISPOSE )	/* Graphics */
	ROM_LOAD( "h13.bin", 0x000000, 0x200000, CRC(13ab7f31) SHA1(468424f74d6cccd1b445a9f20e2d24bc46d61ed6) )
	ROM_LOAD( "h11.bin", 0x200000, 0x200000, CRC(c6ac41c8) SHA1(22408ef1e35c66d0fba0c72972c46fad891d1193) )

	ROM_REGION( 0x100000, REGION_GFX3, 0 )   /* 16 bitmaps (320x200, indexed colors) */
	ROM_LOAD( "glassk.h9", 0x000000, 0x100000, CRC(d499be4c) SHA1(204f754813be687e8dc00bfe7b5dbc4857ac8738) )

	ROM_REGION( 0x140000, REGION_SOUND1, 0 )    /* ADPCM samples - sound chip is OKIM6295 */
	ROM_LOAD( "c1.bin", 0x000000, 0x100000, CRC(d9f075a2) SHA1(31a7a677861f39d512e9d1f51925c689e481159a) )
	/* 0x00000-0x2ffff is fixed, 0x30000-0x3ffff is bank switched from all the ROMs */
	ROM_RELOAD(         0x040000, 0x100000 )
ROM_END

/***************************************************************************

	Split even/odd bytes from ROMs in 16 bit mode to different memory areas

***************************************************************************/

void glass_ROM16_split(int src_reg, int dst_reg, int start, int length, int dest1, int dest2)
{
	int i;

	/* get a pointer to the source data */
	UINT8 *src = (UINT8 *)memory_region(src_reg);

	/* get a pointer to the destination data */
	UINT8 *dst = (UINT8 *)memory_region(dst_reg);

	/* fill destination areas with the proper data */
	for (i = 0; i < length/2; i++){
		dst[dest1 + i] = src[start + i*2 + 0];
		dst[dest2 + i] = src[start + i*2 + 1];
	}
}

static DRIVER_INIT( glass )
{
	/*
	For REGION_GFX2 we have this memory map:
		0x000000-0x1fffff ROM H13
		0x200000-0x3fffff ROM H11

	and we are going to construct this one for REGION_GFX1:
		0x000000-0x0fffff ROM H13 even bytes
		0x100000-0x1fffff ROM H13 odd bytes
		0x200000-0x2fffff ROM H11 even bytes
		0x300000-0x3fffff ROM H11 odd bytes
	*/

	/* split ROM H13 */
	glass_ROM16_split(REGION_GFX2, REGION_GFX1, 0x0000000, 0x0200000, 0x0000000, 0x0100000);

	/* split ROM H11 */
	glass_ROM16_split(REGION_GFX2, REGION_GFX1, 0x0200000, 0x0200000, 0x0200000, 0x0300000);
}

GAMEX(1993, glass,  0,      glass, glass, glass, ROT0, "OMK / Gaelco",                  "Glass (Ver 1.1, Break Edition, Checksum 49D5E66B, Version 1994)", GAME_UNEMULATED_PROTECTION | GAME_NOT_WORKING )
GAME( 1993, glasskr,glass,  glass, glass, glass, ROT0, "OMK / Gaelco (Promat license)", "Glass (Ver 1.1, Break Edition, Checksum D419AB69, Version 1994) (censored, unprotected)" )
