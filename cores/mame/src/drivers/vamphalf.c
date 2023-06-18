/********************************************************************

 Vampire 1/2 and other Hyperstone-based games

 ***VERY WIP***

 To be used only for testing Hyperstone CPU core, probably the only correct
 thing in the driver so far is the ROM loading and graphics decoding.

 These will be split into separate drivers later.

 CHANGELOG:

 MooglyGuy - 10/25/03
    - Changed prelim driver to only load the ROM in the upper part of mem,
      loading the ROM at 0x00000000 and setting the bank to point there was
      completely wrong since apparently there's RAM at 0x00000000.

*********************************************************************/
#include "driver.h"

static MEMORY_READ32_START( readmem )
	{ 0x00000000, 0x0007ffff, MRA32_RAM },
	{ 0xfff80000, 0xffffffff, MRA32_BANK1 },
MEMORY_END

static MEMORY_WRITE32_START( writemem )
	{ 0x00000000, 0x0007ffff, MWA32_RAM },
	{ 0xfff80000, 0xffffffff, MWA32_ROM },
MEMORY_END

static MEMORY_READ32_START( xfiles_readmem )
	{ 0x00000000, 0x0007ffff, MRA32_RAM },
	{ 0xffc00000, 0xffffffff, MRA32_BANK1 },
MEMORY_END

static MEMORY_WRITE32_START( xfiles_writemem )
	{ 0x00000000, 0x0007ffff, MWA32_RAM },
	{ 0xffc00000, 0xffffffff, MWA32_ROM },
MEMORY_END

INPUT_PORTS_START( vamphalf )
INPUT_PORTS_END


VIDEO_START( vamphalf )
{
	return 0;
}

VIDEO_UPDATE( vamphalf )
{

}

static struct GfxLayout vamphalf_layout =
{
	16,16,
	RGN_FRAC(1,1),
	8,
	{ 0,1,2,3,4,5,6,7 },
	{ 0,8,16,24, 32,40,48,56, 64,72,80,88 ,96,104,112,120 },
	{ 0*128, 1*128, 2*128, 3*128, 4*128, 5*128, 6*128, 7*128, 8*128,9*128,10*128,11*128,12*128,13*128,14*128,15*128 },
	16*128,
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &vamphalf_layout,   0x0, 1  }, /* bg tiles */
	{ -1 } /* end of array */
};

static MACHINE_DRIVER_START( vamphalf )
	MDRV_CPU_ADD(E132XS,10000000)		 /* ?? */
	MDRV_CPU_MEMORY(readmem,writemem)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(256, 256)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 0*8, 32*8-1)

	MDRV_GFXDECODE(gfxdecodeinfo)

	MDRV_PALETTE_LENGTH(256)

	MDRV_VIDEO_START(vamphalf)
	MDRV_VIDEO_UPDATE(vamphalf)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( xfiles )
	MDRV_CPU_ADD(E132XS,10000000)		 /* ?? */
	MDRV_CPU_MEMORY(xfiles_readmem,xfiles_writemem)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(256, 256)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 0*8, 32*8-1)

	MDRV_GFXDECODE(gfxdecodeinfo)

	MDRV_PALETTE_LENGTH(256)

	MDRV_VIDEO_START(vamphalf)
	MDRV_VIDEO_UPDATE(vamphalf)
MACHINE_DRIVER_END

/* f2 systems hardware */

ROM_START( vamphalf )
	ROM_REGION( 0x80000, REGION_CPU1, 0 )

	ROM_REGION32_BE( 0x80000, REGION_USER1, 0 ) /* Hyperstone CPU Code */
	ROM_LOAD("prom1", 0x00000000,    0x00080000,   CRC(f05e8e96) SHA1(c860e65c811cbda2dc70300437430fb4239d3e2d))

	ROM_REGION( 0x800000, REGION_GFX1, 0 ) /* 16x16x8 Sprites? */
	ROM_LOAD32_WORD( "roml00",       0x000000, 0x200000, CRC(cc075484) SHA1(6496d94740457cbfdac3d918dce2e52957341616) )
	ROM_LOAD32_WORD( "roml01",       0x400000, 0x200000, CRC(626c9925) SHA1(c90c72372d145165a8d3588def12e15544c6223b) )
	ROM_LOAD32_WORD( "romu00",       0x000002, 0x200000, CRC(711c8e20) SHA1(1ef7f500d6f5790f5ae4a8b58f96ee9343ef8d92) )
	ROM_LOAD32_WORD( "romu01",       0x400002, 0x200000, CRC(d5be3363) SHA1(dbdd0586909064e015f190087f338f37bbf205d2) )

	ROM_REGION( 0x040000, REGION_SOUND1, 0 ) /* Oki Samples */
	ROM_LOAD( "vrom1",        0x000000, 0x040000, CRC(ee9e371e) SHA1(3ead5333121a77d76e4e40a0e0bf0dbc75f261eb) )
ROM_END

/* eolith hardware */

/*



Name         Size     CRC32       Chip Type
-------------------------------------------
hc0_u39.bin  4194304  0xeefb6add  C32000 dumped as SGS 27C322
hc1_u34.bin  4194304  0x482f3e52  C32000 dumped as SGS 27C322
hc2_u40.bin  4194304  0x914a1544  C32000 dumped as SGS 27C322
hc3_u35.bin  4194304  0x80c59133  C32000 dumped as SGS 27C322
hc4_u41.bin  4194304  0x9a9e2203  C32000 dumped as SGS 27C322
hc5_u36.bin  4194304  0x74b1719d  C32000 dumped as SGS 27C322
hc_u108.bin   524288  0x2bae46cb  27C040
hc_u43.bin    524288  0x635b4478  27C040
hc_u97.bin    524288  0xebf9f77b  27C040
hc_u107.bin    32768  0xafd5263d  AMIC 275308 dumped as 27256
hc_u111.bin    32768  0x79012474  AMIC 275308 dumped as 27256
*/

ROM_START( hidnctch )
	ROM_REGION( 0x80000, REGION_CPU1, 0 )

	ROM_REGION32_BE( 0x80000, REGION_USER1, 0 ) /* Hyperstone CPU Code */
	ROM_LOAD("hc_u43.bin", 0x00000000,    0x080000,  CRC(635b4478) SHA1(31ea4a9725e0c329447c7d221c22494c905f6940) )

	ROM_REGION( 0x2000000, REGION_GFX1, 0 ) /* GFX (not tile based) */
	ROM_LOAD16_BYTE("hc0_u39.bin", 0x0000001,    0x0400000, CRC(eefb6add) SHA1(a0f6f2cf86699a666be0647274d8c9381782640d))
	ROM_LOAD16_BYTE("hc1_u34.bin", 0x0000000,    0x0400000, CRC(482f3e52) SHA1(7a527c6af4c80e10cc25219a04ccf7c7ea1b23af))
	ROM_LOAD16_BYTE("hc2_u40.bin", 0x0800001,    0x0400000, CRC(914a1544) SHA1(683cb007ace50d1ba88253da6ad71dc3a395299d))
	ROM_LOAD16_BYTE("hc3_u35.bin", 0x0800000,    0x0400000, CRC(80c59133) SHA1(66ca4c2c014c4a1c87c46a3971732f0a2be95408))
	ROM_LOAD16_BYTE("hc4_u41.bin", 0x1000001,    0x0400000, CRC(9a9e2203) SHA1(a90f5842b63696753e6c16114b1893bbeb91e45c))
	ROM_LOAD16_BYTE("hc5_u36.bin", 0x1000000,    0x0400000, CRC(74b1719d) SHA1(fe2325259117598ad7c23217426ac9c28440e3a0))

	ROM_REGION( 0x080000, REGION_GFX2, 0 ) /* ? */
	ROM_LOAD("hc_u108.bin", 0x000000,    0x080000, CRC(2bae46cb) SHA1(7c43f1002dfc20b9c1bb1647f7261dfa7ed2b4f9))

	ROM_REGION( 0x080000, REGION_GFX3, 0 ) /* ? */
	ROM_LOAD("hc_u107.bin", 0x000000,    0x08000, CRC(afd5263d) SHA1(71ace1b749d8a6b84d08b97185e7e512d04e4b8d) ) /* same in landbrk*/

	ROM_REGION( 0x080000, REGION_GFX4, 0 ) /* ? */
	ROM_LOAD("hc_u111.bin", 0x000000,    0x08000, CRC(79012474) SHA1(09a2d5705d7bc52cc2d1644c87c1e31ee44813ef))

	ROM_REGION( 0x080000, REGION_SOUND1, 0 ) /* ? */
	ROM_LOAD("hc_u97.bin", 0x000000,    0x080000, CRC(ebf9f77b) SHA1(5d472aeb84fc011e19b9e61d34aeddfe7d6ac216) )
ROM_END

/*

Documentation
-------------------------------------------
lb_pcb.jpg    614606  0xf041e24c

Name         Size     CRC32       Chip Type
-------------------------------------------
lb.107         32768  0xafd5263d  AMIC 275308 dumped as 27256
lb2-000.u39  4194304  0xb37faf7a  C32000 dumped as SGS 27C322
lb2-001.u34  4194304  0x07e620c9  C32000 dumped as SGS 27C322
lb2-002.u40  4194304  0x3bb4bca6  C32000 dumped as SGS 27C322
lb2-003.u35  4194304  0x28ce863a  C32000 dumped as SGS 27C322
lb2-004.u41  4194304  0xcbe84b06  C32000 dumped as SGS 27C322
lb2-005.u36  4194304  0x350c77a3  C32000 dumped as SGS 27C322
lb2-006.u42  4194304  0x22c57cd8  C32000 dumped as SGS 27C322
lb2-007.u37  4194304  0x31f957b3  C32000 dumped as SGS 27C322
lb_1.u43      524288  0xf8bbcf44  27C040
lb_2.108      524288  0xa99182d7  27C040
lb_3.u97      524288  0x5b34dff0  27C040

*/

ROM_START( landbrk )
	ROM_REGION( 0x80000, REGION_CPU1, 0 )

	ROM_REGION32_BE( 0x80000, REGION_USER1, 0 ) /* Hyperstone CPU Code */
	ROM_LOAD("lb_1.u43", 0x00000000,    0x080000,   CRC(f8bbcf44) SHA1(ad85a890ae2f921cd08c1897b4d9a230ccf9e072) )

	ROM_REGION( 0x2000000, REGION_GFX1, 0 ) /* GFX (not tile based) */
	ROM_LOAD16_BYTE("lb2-000.u39", 0x0000001,    0x0400000, CRC(b37faf7a) SHA1(30e9af3957ada7c72d85f55add221c2e9b3ea823) )
	ROM_LOAD16_BYTE("lb2-001.u34", 0x0000000,    0x0400000, CRC(07e620c9) SHA1(19f95316208fb4e52cef78f18c5d93460a644566) )
	ROM_LOAD16_BYTE("lb2-002.u40", 0x0800001,    0x0400000, CRC(3bb4bca6) SHA1(115029be4a4e322549a35f3ae5093ec161e9a421) )
	ROM_LOAD16_BYTE("lb2-003.u35", 0x0800000,    0x0400000, CRC(28ce863a) SHA1(1ba7d8be0ed4459dbdf99df18a2ad817904b9f04) )
	ROM_LOAD16_BYTE("lb2-004.u41", 0x1000001,    0x0400000, CRC(cbe84b06) SHA1(52505939fb88cd24f409c795fe5ceed5b41a52c2))
	ROM_LOAD16_BYTE("lb2-005.u36", 0x1000000,    0x0400000, CRC(350c77a3) SHA1(231e65ea7db19019615a8aa4444922bcd5cf9e5c) )
	ROM_LOAD16_BYTE("lb2-006.u42", 0x1800001,    0x0400000, CRC(22c57cd8) SHA1(c9eb745523005876395ff7f0b3e996994b3f1220))
	ROM_LOAD16_BYTE("lb2-007.u37", 0x1800000,    0x0400000, CRC(31f957b3) SHA1(ab1c4c50c2d5361ba8db047feb714423d84e6df4) )

	ROM_REGION( 0x080000, REGION_GFX2, 0 ) /* ? */
	ROM_LOAD("lb_2.108", 0x000000,    0x080000,  CRC(a99182d7) SHA1(628c8d09efb3917a4e97d9e02b6b0ca1f339825d) )

	ROM_REGION( 0x080000, REGION_GFX3, 0 ) /* ? */
	ROM_LOAD("lb.107", 0x000000,    0x08000,    CRC(afd5263d) SHA1(71ace1b749d8a6b84d08b97185e7e512d04e4b8d) )

	ROM_REGION( 0x080000, REGION_GFX4, 0 ) /* ? */
	/* 111 isn't populated? */

	ROM_REGION( 0x080000, REGION_SOUND1, 0 ) /* ? */
	ROM_LOAD("lb_3.u97", 0x000000,    0x080000,  CRC(5b34dff0) SHA1(1668763e977e272781ddcc74beba97b53477cc9d) )
ROM_END

/*

Racoon World by Eolith

U43, u97, u108   are 27c040 devices

u111, u107   are 27c256 devices

On the ROM sub board:
u1, u2, u5, u10, u11, u14  are all 27c160 devices
--------------------------------------------------------------------------
Stereo sound?
24MHz crystal near the sound section

there is a 4 position DIP switch.

Hyperstone E1-32N    45.00000 MHz  near this chip
QDSP     QS1001A
QDSP     QS1000
EOLITH  EV0514-001  custom??   14.31818MHz  xtl near this chip
12MHz crystal is near the U111

U107 and U97 are mostlikely sound roms but not sure

*/

ROM_START( racoon )
	ROM_REGION( 0x80000, REGION_CPU1, 0 )

	ROM_REGION32_BE( 0x80000, REGION_USER1, 0 ) /* Hyperstone CPU Code */
	ROM_LOAD("racoon-u.43", 0x00000000,    0x080000,  CRC(711ee026) SHA1(c55dfaa24cbaa7a613657cfb25e7f0085f1e4cbf) )

	ROM_REGION( 0x2000000, REGION_GFX1, 0 ) /* GFX (not tile based) */
	ROM_LOAD16_BYTE("racoon.u1", 0x0000001,    0x0200000, CRC(49775125) SHA1(2b8ee9dd767465999c828d65bb02b8aaad94177c) )
	ROM_LOAD16_BYTE("racoon.u10",0x0000000,    0x0200000, CRC(f702390e) SHA1(47520ba0e6d3f044136a517ebbec7426a66ce33d) )
	ROM_LOAD16_BYTE("racoon.u2", 0x0800001,    0x0200000, CRC(1eb00529) SHA1(d9af75e116f5237a3c6812538b77155b9c08dd5c) )
	ROM_LOAD16_BYTE("racoon.u11",0x0800000,    0x0200000, CRC(3f23f368) SHA1(eb1ea51def2cde5e7e4f334888294b794aa03dfc) )
	ROM_LOAD16_BYTE("racoon.u5", 0x1000001,    0x0200000, CRC(5fbac174) SHA1(1d3e3f40a737d61ff688627891dec183af7fa19a) )
	ROM_LOAD16_BYTE("racoon.u14",0x1000000,    0x0200000, CRC(870fe45e) SHA1(f8d800b92eb1ee9ef4663319fd3cb1f5e52d0e72) )

	ROM_REGION( 0x080000, REGION_GFX2, 0 ) /* ? */
	ROM_LOAD("racoon-u.108", 0x000000,    0x080000,  CRC(fc4f30ee) SHA1(74b9e60cceb03ad572e0e080fbe1de5cffa1b2c3) )

	ROM_REGION( 0x080000, REGION_GFX3, 0 ) /* ? */
	ROM_LOAD("racoon-u.107", 0x000000,    0x08000,    CRC(89450a2f) SHA1(d58efa805f497bec179fdbfb8c5860ac5438b4ec) )

	ROM_REGION( 0x080000, REGION_GFX4, 0 ) /* ? */
	ROM_LOAD("racoon-u.111", 0x000000,    0x08000, CRC(52f419ea) SHA1(79c9f135b0cf8b1928411faed9b447cd98a83287))

	ROM_REGION( 0x080000, REGION_SOUND1, 0 ) /* ? */
	ROM_LOAD("racoon-u.97", 0x000000,    0x080000,  CRC(fef828b1) SHA1(38352b67d18300db40113df9426c2aceec12a29b))
ROM_END

/* ?? dfpix hardware */

/*

X-Files
dfPIX Entertainment Inc. 1999

Contrary to what you might think on first hearing the title, this game
is like Match It 2 etc. However, the quality of the graphics
is outstanding, perhaps the most high quality seen in this "type" of game.
At the end of the level, you are presented with a babe, where you can use
the joystick and buttons to scroll up and down and zoom in for erm...
a closer inspection of the 'merchandise' ;-))


PCB Layout
----------


VRenderOMinus Rev4
-------------------------------------------------------
|                                                     |
|   DA1545A             C-O-N-N-1                 C   |
|                                                 O   |
|  POT1    T2316162               SEC KS0164      N   |
|  POT2    T2316162                               N   |
|J                                    169NDK19:   3   |
|A     14.31818MHz                     CONN2          |
|M  KA4558                                            |
|M                                                    |
|A                                SEC KM6161002CJ-12  |
|          E1-32XT                                    |
|                                 SEC KM6161002CJ-12  |
|                                                     |
|       ST7705C                   SEC KM6161002CJ-12  |
| B1             XCS05                                |
| B2 B3          14.31818MHz      SEC KM6161002CJ-12  |
-------------------------------------------------------


Notes
-----
ST7705C          : EEPROM?
E1-32XT          : Hyperstone E1-32XT CPU
169NDK19         : Xtal, 16.9MHz
CONN1,CONN2,CONN3: Connectors for small daughterboard containing
                   3x DA28F320J5 (32M surface mounted SSOP56 Flash ROM)
XCS05            : XILINX XCS05 PLD
B1,B2,B3         : Push Buttons for TEST, SERVICE and RESET
SEC KS0164       : Manufactured by Samsung Electronics. Possibly sound
                   related or Sound CPU? (QFP100)
T2316162         : Main program RAM (SOJ44)
SEC KM6161002    : Graphics RAM (SOJ44)

*/

ROM_START( xfiles )
	ROM_REGION( 0x400000, REGION_CPU1, 0 )

	ROM_REGION32_BE( 0x400000, REGION_USER1, 0 ) /* Hyperstone CPU Code */
	ROM_LOAD16_WORD_SWAP("u9.bin", 0x00000000,    0x400000,   CRC(ebdb75c0) SHA1(9aa5736bbf3215c35d62b424c2e5e40223227baf) )

	/* the following probably aren't in the right regions etc. */

	ROM_REGION( 0x400000, REGION_GFX1, 0 )
	ROM_LOAD16_WORD_SWAP("u8.bin", 0x00000000,    0x400000,   CRC(3b2c2bc1) SHA1(1c07fb5bd8a8c9b5fb169e6400fef845f3aee7aa) )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 )
	ROM_LOAD16_WORD_SWAP("u10.bin", 0x00000000,    0x400000,   CRC(f2ef1eb9) SHA1(d033d140fce6716d7d78509aa5387829f0a1404c) )
ROM_END

DRIVER_INIT( vamphalf )
{
	cpu_setbank(1, memory_region(REGION_USER1));
}

/*           rom       parent    machine   inp       init */
GAMEX( 19??, vamphalf, 0,        vamphalf, vamphalf, vamphalf, ROT0, "Danbi", "Vamp 1-2", GAME_NO_SOUND | GAME_NOT_WORKING )
GAMEX( 19??, hidnctch, 0,        vamphalf, vamphalf, vamphalf, ROT0, "Eolith", "Hidden Catch", GAME_NO_SOUND | GAME_NOT_WORKING )
GAMEX( 19??, landbrk,  0,        vamphalf, vamphalf, vamphalf, ROT0, "Eolith", "Land Breaker", GAME_NO_SOUND | GAME_NOT_WORKING )
GAMEX( 19??, racoon,   0,        vamphalf, vamphalf, vamphalf, ROT0, "Eolith", "Racoon World", GAME_NO_SOUND | GAME_NOT_WORKING )
GAMEX( 19??, xfiles,   0,        xfiles,   vamphalf, vamphalf, ROT0, "dfPIX Entertainment Inc.", "X-Files", GAME_NO_SOUND | GAME_NOT_WORKING )
