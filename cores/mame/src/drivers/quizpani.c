/*************************************************************************
	Quiz Panicuru Fantasy
	(c) 1993 NMK

	Driver by David Haywood and Pierpaolo Prazzoli

	PCB No: QZ93094
	CPU   : TMP68000P-12
	SOUND : Oki M6295
	OSC   : 16.000MHz, 10.000MHz
	RAM   : 62256 (x2), 6116 (x2), 6264 (x4)
	DIPSW : 8 position (x2)
	CUSTOM: NMK112 (QFP64, near ROMs 31,32,4, possible sound chip/CPU?)
			NMK111 (QFP64, 1x input-related near JAMMA, 2x gfx related near ROMs 11,12,21,22)
			NMK903 (QFP44, x2, near ROMs 11,12,21,22)
			NMK005 (QFP64, near DIPs, possible MCU?)

	ROMs  :
			93094-51.127	27c4002     near 68000
			93094-52.126    27c4001     near 68000
			93094-53.125    27c1001     near 68000
			93090-4.56      8M Mask     oki samples
			93090-31.58     8M Mask     oki samples
			93090-32.57     8M Mask     oki samples
			93090-21.10     8M Mask     gfx
			93090-22.9      8M Mask     gfx
			93090-11.2      8M Mask     gfx
			93090-12.1      8M Mask     gfx
			QZ6.88          82s129      prom
			QZ7.99          82s129      prom
			QZ8.121         82s135      prom

*************************************************************************/

#include "driver.h"

extern data16_t *quizpani_bg_videoram, *quizpani_txt_videoram;
extern data16_t *quizpani_scrollreg;

extern WRITE16_HANDLER( quizpani_bg_videoram_w );
extern WRITE16_HANDLER( quizpani_txt_videoram_w );
extern WRITE16_HANDLER( quizpani_tilesbank_w );

extern VIDEO_START( quizpani );
extern VIDEO_UPDATE( quizpani );

static WRITE16_HANDLER( quizpani_oki6295_bankswitch_w )
{
	#define TABLESIZE 0x100
	#define BANKSIZE 0x10000
	int banknum = offset & 3;
	unsigned char *rom = memory_region(REGION_SOUND1);
	int bankaddr = data * BANKSIZE;

	/* copy the samples */
	memcpy(rom + banknum * BANKSIZE, rom + 0x40000 + bankaddr, BANKSIZE);

	/* and also copy the samples address table */
	rom += banknum * TABLESIZE;
	memcpy(rom, rom + 0x40000 + bankaddr, TABLESIZE);
}

static MEMORY_READ16_START( quizpani_readmem )
	{ 0x000000, 0x07ffff, MRA16_ROM },
	{ 0x100000, 0x100001, input_port_0_word_r },
	{ 0x100002, 0x100003, input_port_1_word_r },
	{ 0x100008, 0x100009, input_port_2_word_r },
	{ 0x10000a, 0x10000b, input_port_3_word_r },
	{ 0x104000, 0x104001, OKIM6295_status_0_lsb_r },
	{ 0x108000, 0x1083ff, MRA16_RAM },
	{ 0x110000, 0x113fff, MRA16_RAM },
	{ 0x180000, 0x18ffff, MRA16_RAM },
	{ 0x200000, 0x33ffff, MRA16_ROM },
MEMORY_END

static MEMORY_WRITE16_START( quizpani_writemem )
	{ 0x000000, 0x07ffff, MWA16_ROM },
	{ 0x100014, 0x100015, MWA16_NOP },
	{ 0x100016, 0x100017, MWA16_NOP }, /* IRQ eanble? */
	{ 0x100018, 0x100019, quizpani_tilesbank_w },
	{ 0x104000, 0x104001, OKIM6295_data_0_lsb_w },
	{ 0x104020, 0x104027, quizpani_oki6295_bankswitch_w },
	{ 0x108000, 0x1083ff, paletteram16_RRRRGGGGBBBBRGBx_word_w, &paletteram16 },
	{ 0x108400, 0x1085ff, MWA16_NOP },
	{ 0x10c000, 0x10c007, MWA16_RAM, &quizpani_scrollreg },
	{ 0x10c008, 0x10c403, MWA16_NOP },
	{ 0x110000, 0x113fff, quizpani_bg_videoram_w, &quizpani_bg_videoram },
	{ 0x11c000, 0x11ffff, quizpani_txt_videoram_w, &quizpani_txt_videoram },
	{ 0x180000, 0x18ffff, MWA16_RAM },
	{ 0x200000, 0x33ffff, MWA16_ROM },
MEMORY_END

INPUT_PORTS_START( quizpani )
	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0xff00, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER1 )
	PORT_DIPNAME( 0x0010, 0x0010, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0010, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0020, 0x0020, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0040, 0x0040, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0040, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0080, 0x0080, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0080, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER2 )
	PORT_DIPNAME( 0x1000, 0x1000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x1000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x2000, 0x2000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x2000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x4000, 0x4000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x4000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x8000, 0x8000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x8000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )

	PORT_START
	PORT_DIPNAME( 0x0001, 0x0001, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0001, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0002, 0x0002, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0004, 0x0004, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0004, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0008, 0x0008, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0008, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0030, 0x0030, DEF_STR( Lives ) )
	PORT_DIPSETTING(      0x0000, "1" )
	PORT_DIPSETTING(      0x0020, "2" )
	PORT_DIPSETTING(      0x0030, "3" )
	PORT_DIPSETTING(      0x0010, "4" )
	PORT_DIPNAME( 0x0040, 0x0040, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0040, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0080, 0x0080, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0080, DEF_STR( On ) )
	PORT_DIPNAME( 0x0100, 0x0100, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0100, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0200, 0x0200, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0200, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0400, 0x0400, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0400, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0800, 0x0800, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0800, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x1000, 0x1000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x1000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x2000, 0x2000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x2000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x4000, 0x4000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x4000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x8000, 0x8000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x8000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )

	PORT_START
	PORT_DIPNAME( 0x0001, 0x0001, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0001, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0002, 0x0002, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x001c, 0x001c, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(      0x0010, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(      0x0008, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0018, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x001c, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x000c, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x0014, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x0004, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0x00e0, 0x00e0, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(      0x0080, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(      0x0040, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x00c0, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x00e0, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0060, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x00a0, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0x0100, 0x0100, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0100, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0200, 0x0200, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0200, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0400, 0x0400, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0400, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0800, 0x0800, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0800, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x1000, 0x1000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x1000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x2000, 0x2000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x2000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x4000, 0x4000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x4000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x8000, 0x8000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x8000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
INPUT_PORTS_END

static struct GfxLayout tilelayout =
{
	16,16,
	RGN_FRAC(1,1),
	4,
	{ 0, 1, 2, 3 },
	{ 0*4, 1*4, 2*4, 3*4, 4*4, 5*4, 6*4, 7*4,
			16*32+0*4, 16*32+1*4, 16*32+2*4, 16*32+3*4, 16*32+4*4, 16*32+5*4, 16*32+6*4, 16*32+7*4 },
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32,
			8*32, 9*32, 10*32, 11*32, 12*32, 13*32, 14*32, 15*32 },
	32*32
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &tilelayout, 0x100, 16 }, /* Background */
	{ REGION_GFX2, 0, &tilelayout, 0x000, 16 }, /* Text */
	{ -1 } /* end of array */
};

static struct OKIM6295interface okim6295_interface =
{
	1,					/* 1 chip */
	{ 16000000/4/165 },	/* frequency (Hz) */
	{ REGION_SOUND1 },	/* memory region */
	{ 100 }
};


static MACHINE_DRIVER_START( quizpani )
	MDRV_CPU_ADD(M68000, 10000000)
	MDRV_CPU_MEMORY(quizpani_readmem,quizpani_writemem)
	MDRV_CPU_VBLANK_INT(irq4_line_hold,1)
	MDRV_CPU_PERIODIC_INT(irq1_line_hold,164) /* music tempo*/

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(0x200)

	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(64*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 48*8-1, 0*8, 28*8-1)

	MDRV_VIDEO_START(quizpani)
	MDRV_VIDEO_UPDATE(quizpani)

	MDRV_SOUND_ADD(OKIM6295, okim6295_interface)
MACHINE_DRIVER_END

ROM_START( quizpani )
	ROM_REGION( 0x340000, REGION_CPU1, 0 ) /* 68000 Code */
	ROM_LOAD16_WORD_SWAP( "93094-51.127", 0x000000, 0x080000, CRC(2b7a29d4) SHA1(f87b875e69410745ee46d5d94b6c28e5417afb0d) )
	/* No EVEN rom */
	ROM_LOAD16_BYTE( "93094-52.126",	  0x200001, 0x080000, CRC(0617524e) SHA1(91ab5cb8a605c37c92632cf007ddb67172cc9863) )
	/* No EVEN rom */
	ROM_LOAD16_BYTE( "93094-53.125",	  0x300001, 0x020000, CRC(7e0ab49c) SHA1(dd10f723ef74f3153e04b1a271b8761585799aa6) )

	ROM_REGION( 0x200000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "93090-11.2",  0x000000, 0x100000, CRC(4b3ab155) SHA1(fc1210853ca262c42b927689cb8f04aca15de7d6) )
	ROM_LOAD( "93090-12.1",  0x100000, 0x100000, CRC(3f2ebfa5) SHA1(1c935d566f3980483356264a9216f9bf298bb815) )

	ROM_REGION( 0x200000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "93090-21.10", 0x000000, 0x100000, CRC(63754242) SHA1(3698b89d8515b45b9bc0fff87ca94ab5c2b3d53a) )
	ROM_LOAD( "93090-22.9",  0x100000, 0x100000, CRC(93382cd3) SHA1(6527e92f696c21aae65d008bb237231eaba7a105) )

	ROM_REGION( 0x340000, REGION_SOUND1, 0 )
	ROM_LOAD( "93090-31.58", 0x040000, 0x100000, CRC(1cce0e13) SHA1(43816762e7907a8ff4b5a7b8da9f799b5baa64d5) )
	ROM_LOAD( "93090-32.57", 0x140000, 0x100000, CRC(5d38f62e) SHA1(22fe95de6e1de1be0cec73b8163ab4283f2b8186) )
	ROM_LOAD( "93090-4.56",  0x240000, 0x100000, CRC(ee370ed6) SHA1(9b1edfada5805014aa23d28d0c70227728b0e04f) )

	ROM_REGION( 0x300, REGION_PROMS, 0 )
	ROM_LOAD( "qz6.88",  0x000, 0x100, CRC(19dbbad2) SHA1(ebf7950d1869ca3bc1e72228505fbc17d095746a) ) /* unknown */
	ROM_LOAD( "qz7.99",  0x100, 0x100, CRC(1f802af1) SHA1(617bb7e5105ac202b5a8cf83c8c66178b91099e0) ) /* unknown */
	ROM_LOAD( "qz8.121", 0x200, 0x100, CRC(b4c19741) SHA1(a6d3686bad6ef2336463b89bc2d249003d9b4bcc) ) /* unknown */
ROM_END

GAME( 1993, quizpani, 0, quizpani, quizpani, 0, ROT0, "NMK", "Quiz Panicuru Fantasy" )
