/***************************************************************************

World Rally (c) 1993 Gaelco (Designed & Developed by Zigurat. Produced by Gaelco)

Preliminary driver by Manuel Abadia <manu@teleline.es>

Encyption tables provided by Mike Coates who connected a fluke to the PCB.
Nicola Salmoria made the decode function based on that info.

Current decryption is incomplete
The DS5002FP has 32KB undumped gameplay code making the game unplayable :(

Main PCB components:
====================

CPUs related:
=============
* 1xDS5002FP @ D12 (Dallas security processor @ 12 MHz)
* 1xHM62256ALFP-8T (32KB NVSRAM) @ C11 (encrypted DS5002FP program code)
* 1xLithium cell
* 2xMS6264A-20NC (32KB SRAM) @ D14 & D15 (shared memory between M68000 & DS5002FP)
* 4x74LS157 (Quad 2 input multiplexer) @ F14, F15, F16 & F17 (used to select M68000 or DS5002FP address bus)
* 4x74LS245 (Octal bus transceiver) @ C14, C15, C16 & C17 (used to store shared RAM data)
* 2x74LS373 (Octal tristate latch) @ D16 & D17 (used by DS5002FP to access data from shared RAM)
* 1xMC68000P12 @ C20 (Motorola 68000 @ 12 MHz)
* 1xOSC24MHz @ B20
* 2xM27C4001 @ C22 & C23 (M68000 program ROMs)
* 1xPAL20L8 @ B23 (handles 1st level M68000 memory map)
	0 -> DTACK (M68000 data ack)
	1 -> SELACT
	2 -> Input/sound (see below)
	3 -> ACTEXT
	4 -> SELMOV
	5 -> CSW
	6 -> CSR
	7 -> EXT

* 1x74LS138 (3 to 8 line decoder) @ B13 (handles 2nd level M68000 memory map)
	0 -> IN0	DIPSW #1 & #2
	1 -> IN1	Joystick 1P & 2P, COINSW, STARTSW
	2 -> IN2	Wheel input
	3 -> -
	4 -> IN4	TESTSW & SERVICESW
	5 -> OUT (see below)
	6 -> CSBAN	OKIM6295 bankswitch
	7 -> CSSON	OKIM6295 R/W

* 1x74LS259 (8 bit addressable latches) @A7 (handles 3rd level M68000 memory map)
	0 -> Coin lockout 1
	1 -> Coin lockout 2
	2 -> Coin counter 1
	3 -> Coin counter 2
	4 -> Sound muting
	5 -> flip screen
	6 -> ENA/D?
	7 -> CKA/D?

Sound related:
==============
* 1xOKIM6295 @ C6
* 2xM27C4001 @ C1 & C3 (OKI ADPCM samples)
* 1xPAL16R4 @ E2 (handles OKI ROM banking)

Graphics related:
=================
* 1xOSC30MHz @ D5
* 2xTPC1020AFN-84C (FPGA) @ G8 & G13 (GFX processing)
* 2xMS6264A-20NC (8KB SRAM) @ I16 & I17 (Video RAM)
* 4xUM6116BK-25 (2KB SRAM) @ H1, H2, H4 & H5
* 2xUM6116BK-25 (2KB SRAM) @ H22 & H23

Palette related:
================
* 2xMS6264A-20NC (8KB SRAM) @ C8 & C9 (palette RAM (xxxxBBBBRRRRGGGG))
* 2x74HCT273 (octal D-Type flip-flop with clear) @ B8 & B9 (connected to RGB output)

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "cpu/m68000/m68000.h"

/* from vidhrdw/wrally.c */
extern data16_t *wrally_vregs;
extern data16_t *wrally_videoram;
extern data16_t *wrally_spriteram;

WRITE16_HANDLER( wrally_vram_w );
VIDEO_START( wrally );
VIDEO_UPDATE( wrally );

/* from machine/wrally.c */
DRIVER_INIT( wrally );
WRITE16_HANDLER( OKIM6295_bankswitch_w );
WRITE16_HANDLER( wrally_coin_counter_w );
WRITE16_HANDLER( wrally_coin_lockout_w );



static MEMORY_READ16_START( wrally_readmem )
	{ 0x000000, 0x0fffff, MRA16_ROM },			/* ROM */
	{ 0x100000, 0x103fff, MRA16_RAM },			/* encrypted Video RAM */
	{ 0x200000, 0x203fff, MRA16_RAM },			/* Palette */
	{ 0x440000, 0x440fff, MRA16_RAM },			/* Sprite RAM */
	{ 0x700000, 0x700001, input_port_0_word_r },/* DSW #1 & #2 */
	{ 0x700002, 0x700003, input_port_1_word_r },/* INPUT 1P & 2P, COINSW, STARTSW */
	{ 0x700004, 0x700005, input_port_2_word_r },/* Wheel */
	{ 0x700008, 0x700009, input_port_3_word_r },/* TESTSW & SERVICESW */
	{ 0x70000e, 0x70000f, OKIM6295_status_0_lsb_r },/* OKI6295 status register */
	{ 0xfe0000, 0xfeffff, MRA16_RAM },			/* Work RAM (partially shared with DS5002FP) */
MEMORY_END

static WRITE16_HANDLER( unknown_w )
{
	usrintf_showmessage("write %04x to %04x", data, offset*2 + 0x6a);
}

static MEMORY_WRITE16_START( wrally_writemem )
	{ 0x000000, 0x0fffff, MWA16_ROM },								/* ROM */
	{ 0x100000, 0x103fff, wrally_vram_w, &wrally_videoram },		/* encrypted Video RAM */
	{ 0x108000, 0x108007, MWA16_RAM, &wrally_vregs },				/* Video Registers */
	{ 0x10800c, 0x10800d, MWA16_NOP },								/* CLR INT Video */
	{ 0x200000, 0x203fff, paletteram16_xxxxBBBBRRRRGGGG_word_w, &paletteram16 },/* Palette */
	{ 0x440000, 0x440fff, MWA16_RAM, &wrally_spriteram },			/* Sprite RAM */
	{ 0x70000c, 0x70000d, OKIM6295_bankswitch_w },					/* OKI6295 bankswitch */
	{ 0x70000e, 0x70000f, OKIM6295_data_0_lsb_w },					/* OKI6295 data register */
	{ 0x70000a, 0x70001b, wrally_coin_lockout_w },					/* Coin lockouts */
	{ 0x70002a, 0x70003b, wrally_coin_counter_w },					/* Coin counters */
	{ 0x70004a, 0x70004b, MWA16_NOP },								/* sound muting */
	{ 0x70005a, 0x70005b, MWA16_NOP },								/* flip screen */
	{ 0x70006a, 0x70007b, unknown_w },								/* ??? */
	{ 0xfe0000, 0xfeffff, MWA16_RAM },								/* Work RAM (partially shared with DS5002FP) */
MEMORY_END


INPUT_PORTS_START( wrally )
PORT_START	/* DSW #1 & #2 */
	PORT_DIPNAME( 0x0003, 0x0003, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(      0x0003, "Normal" )
	PORT_DIPSETTING(      0x0002, "Easy" )
	PORT_DIPSETTING(      0x0001, "Hard" )
	PORT_DIPSETTING(      0x0000, "Hardest" )
	PORT_DIPNAME( 0x0004, 0x0004, "Number of Joysticks" )
	PORT_DIPSETTING(      0x0004, "2" )
	PORT_DIPSETTING(      0x0000, "1" )
	PORT_DIPNAME( 0x0018, 0x0018, "Control Configuration" )
	PORT_DIPSETTING(      0x0018, "Joystick" )
	PORT_DIPSETTING(      0x0010, "Pot Wheel" )
	PORT_DIPSETTING(      0x0000, "Opt Wheel" )
	PORT_DIPNAME( 0x0020, 0x0020, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( On ) )
	PORT_DIPNAME( 0x0040, 0x0040, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(      0x0040, DEF_STR( Upright ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Cocktail ) )
	PORT_SERVICE( 0x0080, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x0700, 0x0700, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(      0x0700, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 3C_4C ) )
	PORT_DIPSETTING(      0x0100, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(      0x0600, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x0500, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x0400, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(      0x0300, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(      0x0200, DEF_STR( 1C_6C ) )
	PORT_DIPNAME( 0x3800, 0x3800, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(      0x1000, DEF_STR( 6C_1C ) )
	PORT_DIPSETTING(      0x1800, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(      0x2000, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(      0x2800, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x3000, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0800, DEF_STR( 3C_2C ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 4C_3C ) )
	PORT_DIPSETTING(      0x3800, DEF_STR( 1C_1C ) )
	PORT_DIPNAME( 0x4000, 0x4000, "Credit configuration" )
	PORT_DIPSETTING(      0x4000, "Start 1C/Continue 1C" )
	PORT_DIPSETTING(      0x0000, "Start 2C/Continue 1C" )
	PORT_DIPNAME( 0x8000, 0x8000, DEF_STR( Free_Play ) )
	PORT_DIPSETTING(      0x8000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )

PORT_START	/* INPUTS, COINSW & STARTSW */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_START2 )

PORT_START	/* Wheel control? */
	PORT_BIT( 0xffff, IP_ACTIVE_LOW, IPT_UNKNOWN ) /* not implemented yet */

PORT_START	/* INPUTS, TEST & SERVICE */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_SERVICE2 )	/* Go to test mode NOW */
	PORT_BIT( 0xfffc, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END



static struct GfxLayout wrally_tilelayout16 =
{
	16,16,									/* 16x16 tiles */
	RGN_FRAC(1,2),							/* number of tiles */
	4,										/* 4 bpp */
	{ RGN_FRAC(1,2)+8, RGN_FRAC(1,2)+0, 8, 0 },
	{ 0, 1, 2, 3, 4, 5, 6, 7,
		16*16+0, 16*16+1, 16*16+2, 16*16+3, 16*16+4, 16*16+5, 16*16+6, 16*16+7 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16,
	  8*16, 9*16, 10*16, 11*16, 12*16, 13*16, 14*16, 15*16, },
	64*8
};

static struct GfxDecodeInfo wrally_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0x000000, &wrally_tilelayout16, 0, 64*8 },
	{ -1 }
};



static struct OKIM6295interface wrally_okim6295_interface =
{
	1,                  /* 1 chip */
	{ 8000 },			/* 8000 KHz? */
	{ REGION_SOUND1 },  /* memory region */
	{ 100 }				/* volume */
};

static MACHINE_DRIVER_START( wrally )
	/* basic machine hardware */
	MDRV_CPU_ADD(M68000,24000000/2)			/* 12 MHz */
	MDRV_CPU_MEMORY(wrally_readmem,wrally_writemem)
	MDRV_CPU_VBLANK_INT(irq6_line_hold,1)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(64*16, 32*16)
	MDRV_VISIBLE_AREA(0, 64*16-1, 0, 32*16-1)
/*	MDRV_VISIBLE_AREA(0, 320-1, 16, 256-1)*/
	MDRV_GFXDECODE(wrally_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(1024*8)

	MDRV_VIDEO_START(wrally)
	MDRV_VIDEO_UPDATE(wrally)

	/* sound hardware */
	MDRV_SOUND_ADD(OKIM6295, wrally_okim6295_interface)
MACHINE_DRIVER_END



ROM_START( wrally )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )	/* 68000 code */
	ROM_LOAD16_BYTE(	"worldr17.c23",	0x000000, 0x080000, CRC(050f5629) SHA1(74fc2cd5114f3bc4b2429f1d8d7eeb1658f9f179) )
	ROM_LOAD16_BYTE(	"worldr16.c22",	0x000001, 0x080000, CRC(9e0d126c) SHA1(369360b7ec2c3497af3bf62b4eba24c3d9f94675) )

	ROM_REGION( 0x200000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD16_BYTE( "worldr21.i13",	0x000000, 0x080000, CRC(b7fddb12) SHA1(619a75daac8cbba7e85c97ca19733e2196d66d5c) )
	ROM_LOAD16_BYTE( "worldr20.i11",	0x000001, 0x080000, CRC(58b2809a) SHA1(8741ec544c54e2a2f5d17ac2f8400ee2ce382e83) )
	ROM_LOAD16_BYTE( "worldr19.i09",	0x100000, 0x080000, CRC(018b35bb) SHA1(ca789e23d18cc7d7e48b6858e6b61e03bf88b475) )
	ROM_LOAD16_BYTE( "worldr18.i07",	0x100001, 0x080000, CRC(b37c807e) SHA1(9e6155a2b5206c0d4dca669d24d9fe9830027651) )

	ROM_REGION( 0x140000, REGION_SOUND1, 0 )	/* ADPCM samples - sound chip is OKIM6295 */
	ROM_LOAD( "worldr14.c01",	0x000000, 0x080000, CRC(e931c2ee) SHA1(ea1cf8ad52713e5136a370e289567eea9e6403d6) )
	/* 0x00000-0x2ffff is fixed, 0x30000-0x3ffff is bank switched from all the ROMs */
	ROM_RELOAD(		0x040000, 0x080000 )
	ROM_LOAD( "worldr15.c03",	0x0c0000, 0x080000, CRC(11f0fe2c) SHA1(96c2a04874fa036576b7cfc5559bb0e33582ffd2) )
ROM_END

ROM_START( wrallya )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )	/* 68000 code */
	ROM_LOAD16_BYTE(	"c23.bin",	0x000000, 0x080000, CRC(8b7d93c3) SHA1(ce4163eebc5d4a0c1266d650523b1ffc702d1b87) )
	ROM_LOAD16_BYTE(	"c22.bin",	0x000001, 0x080000, CRC(56da43b6) SHA1(02db8f969ed5e7f5e5356c45c0312faf5f000335) )

	ROM_REGION( 0x200000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD16_BYTE( "worldr21.i13",	0x000000, 0x080000, CRC(b7fddb12) SHA1(619a75daac8cbba7e85c97ca19733e2196d66d5c) )
	ROM_LOAD16_BYTE( "worldr20.i11",	0x000001, 0x080000, CRC(58b2809a) SHA1(8741ec544c54e2a2f5d17ac2f8400ee2ce382e83) )
	ROM_LOAD16_BYTE( "worldr19.i09",	0x100000, 0x080000, CRC(018b35bb) SHA1(ca789e23d18cc7d7e48b6858e6b61e03bf88b475) )
	ROM_LOAD16_BYTE( "worldr18.i07",	0x100001, 0x080000, CRC(b37c807e) SHA1(9e6155a2b5206c0d4dca669d24d9fe9830027651) )
/*  same data, different layout*/
/*	ROM_LOAD( "h12.bin",	0x000000, 0x100000, CRC(3353dc00) )*/
/*	ROM_LOAD( "h8.bin",		0x100000, 0x100000, CRC(58dcd024) )*/

	ROM_REGION( 0x140000, REGION_SOUND1, 0 )	/* ADPCM samples - sound chip is OKIM6295 */
	ROM_LOAD( "worldr14.c01",	0x000000, 0x080000, CRC(e931c2ee) SHA1(ea1cf8ad52713e5136a370e289567eea9e6403d6) )
	/* 0x00000-0x2ffff is fixed, 0x30000-0x3ffff is bank switched from all the ROMs */
	ROM_RELOAD(		0x040000, 0x080000 )
	ROM_LOAD( "worldr15.c03",	0x0c0000, 0x080000, CRC(11f0fe2c) SHA1(96c2a04874fa036576b7cfc5559bb0e33582ffd2) )
ROM_END



GAMEX( 1993, wrally,  0, 	  wrally, wrally, wrally, ROT0, "Gaelco", "World Rally (set 1)", GAME_NOT_WORKING )
GAMEX( 1993, wrallya, wrally, wrally, wrally, wrally, ROT0, "Gaelco", "World Rally (set 2)", GAME_NOT_WORKING )
