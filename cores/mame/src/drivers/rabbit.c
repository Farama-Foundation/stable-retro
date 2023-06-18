/*  68020 + IMAGETEK 15000 hardware

Rabbit (c) 1997 Electronic Arts

appears to be blitter based, not surprising
as the previous IMAGETEK chips were too
(used in metro.c)


*/

/*

Rabbit PCB Layout
-----------------

VG5330-B
|---------------------------------|
|    62256  62256        61    60 |
|    62256  62256        51    50 |
|    62256  62256                 |
|    62256  62256        43    42 |
|                        41    40 |
|                                 |
|J     IMAGETEK          33    32 |
|A      15000            23    22 |
|M            40MHz               |
|M                       13    12 |
|A             68EC020   03    02 |
|     ALTERA                      |
|     EPM7032 24MHz      31    30 |
|     93C46              21    20 |
|                                 |
| JPR2  JPR0  62256      11    10 |
| JPR3  JPR1  62256      01    00 |
|---------------------------------|

Notes:
      68020 clock: 24.000MHz
      VSync: 60Hz


Tokimeki Mahjong Paradise - Dear My Love
-----------------------------------------

Board:	VG5550-B

CPU:	MC68EC020FG25
OSC:	40.00000MHz
	24.00000MHz

Custom:	Imagetek 15000 (2ch video & 2ch sound)

*/

#include "driver.h"

READ32_HANDLER( rabbit_unk_r1 )
{
	return 0xffffffff;
}

static MEMORY_READ32_START( rabbit_readmem )
	{ 0x000000, 0x1fffff, MRA32_ROM },
	{ 0x200000, 0x200003, rabbit_unk_r1 },
	{ 0x440000, 0x44ffff, MRA32_RAM },
	{ 0x450000, 0x45ffff, MRA32_RAM },
	{ 0x460000, 0x46ffff, MRA32_RAM },
	{ 0x470000, 0x47ffff, MRA32_RAM },

	{ 0xff0000, 0xffffff, MRA32_RAM },
MEMORY_END

static MEMORY_WRITE32_START( rabbit_writemem )
	{ 0x000000, 0x1fffff, MWA32_ROM },
	{ 0xff0000, 0xffffff, MWA32_RAM },
MEMORY_END

INPUT_PORTS_START( rabbit )
INPUT_PORTS_END


VIDEO_START(rabbit)
{
	return 0;
}

VIDEO_UPDATE(rabbit)
{

}


static MACHINE_DRIVER_START( rabbit )
	MDRV_CPU_ADD(M68020, 24000000 )
	MDRV_CPU_MEMORY(rabbit_readmem,rabbit_writemem)
	MDRV_CPU_VBLANK_INT(irq6_line_hold,1)
	/* (rabbit) */
/*	lev 1 : 0x64 : 0000 027c -
	lev 2 : 0x68 : 0000 3106 - unused?
	lev 3 : 0x6c : 0000 3106 - unused?
	lev 4 : 0x70 : 0000 0268 -
	lev 5 : 0x74 : 0000 0278 -
	lev 6 : 0x78 : 0000 0204 -
	lev 7 : 0x7c : 0000 3106 - unused?
*/

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(64*8, 32*8)
	MDRV_VISIBLE_AREA(8*8, 48*8-1, 2*8, 30*8-1)
	MDRV_PALETTE_LENGTH(0x200)

	MDRV_VIDEO_START(rabbit)
	MDRV_VIDEO_UPDATE(rabbit)
MACHINE_DRIVER_END


ROM_START( rabbit )
	ROM_REGION( 0x200000, REGION_CPU1, 0 ) /* 68020 Code */
	ROM_LOAD32_BYTE( "jpr0.0", 0x000000, 0x080000, CRC(52bb18c0) SHA1(625bc8a4daa6d08cacd92d9110cf67a95a91325a) )
	ROM_LOAD32_BYTE( "jpr1.1", 0x000001, 0x080000, CRC(38299d0d) SHA1(72ccd51781b47636bb16ac18037cb3121d17199f) )
	ROM_LOAD32_BYTE( "jpr2.2", 0x000002, 0x080000, CRC(fa3fd91a) SHA1(ac0e658af30b37b752ede833b44ff5423b93bdb1) )
	ROM_LOAD32_BYTE( "jpr3.3", 0x000003, 0x080000, CRC(d22727ca) SHA1(8415cb2d3864b11fe5623ac65f2e28fd62c61bd1) )

	ROM_REGION( 0x1000000, REGION_USER1, 0 ) /* Other Roms probably accessable by cpu / blitter, order is no doubt wrong */
	ROM_LOAD16_BYTE( "jfv0.00", 0x000000, 0x400000, CRC(b2a4d3d3) SHA1(0ab71d82a37ff94442b91712a28d3470619ba575) )
	ROM_LOAD16_BYTE( "jfv1.01", 0x000001, 0x400000, CRC(83f3926e) SHA1(b1c479e675d35fc08c9a7648ff40348a24654e7e) )
	ROM_LOAD16_BYTE( "jfv2.02", 0x800000, 0x400000, CRC(b264bfb5) SHA1(8fafedb6af74150465b1773e80aef0edc3da4678) )
	ROM_LOAD16_BYTE( "jfv3.03", 0x800001, 0x400000, CRC(3e1a9be2) SHA1(2082a4ae8cda84cec5ea0fc08753db387bb70d41) )

	ROM_REGION( 0xc00000, REGION_USER2, 0 ) /* Other Roms probably accessable by cpu / blitter, order is no doubt wrong */
	/* looking at the board layout these might just map after USER1 with no even / odd rom */
	ROM_LOAD16_BYTE( "jbg0.40", 0x000000, 0x200000, CRC(89662944) SHA1(ca916ba38480fa588af19fc9682603f5195ad6c7) )
	ROM_LOAD16_BYTE( "jbg1.50", 0x400000, 0x200000, CRC(1fc7f6e0) SHA1(b36062d2a9683683ffffd3003d5244a185f53280) )
	ROM_LOAD16_BYTE( "jbg2.60", 0x800000, 0x200000, CRC(aee265fc) SHA1(ec420ab30b9b5141162223fc1fbf663ad9f211e6) )

	ROM_REGION( 0x600000, REGION_USER3, 0 ) /* Sound Roms? */
	ROM_LOAD( "jsn0.11", 0x000000, 0x400000, CRC(e1f726e8) SHA1(598d75f3ff9e43ec8ce6131ed37f4345bf2f2d8e) )
ROM_END

ROM_START( tmmjprd )
	ROM_REGION( 0x200000, REGION_CPU1, 0 ) /* 68020 Code - doesn't seem to dsam quite right, bitswap? */
	ROM_LOAD32_BYTE( "p00.bin", 0x000000, 0x080000, CRC(a1efd960) SHA1(7f41ab58de32777bccbfe28e6e5a1f2dca35bb90) )
	ROM_LOAD32_BYTE( "p01.bin", 0x000001, 0x080000, CRC(9c325374) SHA1(1ddf1c292fc1bcf4dcefb5d4aa3abdeb1489c020) )
 	ROM_LOAD32_BYTE( "p02.bin", 0x000002, 0x080000, CRC(729a5f12) SHA1(615704d36afdceb4b1ff2e5dc34856e614181e16) )
	ROM_LOAD32_BYTE( "p03.bin", 0x000003, 0x080000, CRC(595615ab) SHA1(aca746d74aa6e7e856eb5c9b740d884778743b27) )

	ROM_REGION( 0x4000000, REGION_USER1, 0 ) /* Other Roms probably accessable by cpu / blitter, order is no doubt wrong */
	ROM_LOAD16_BYTE( "00.bin", 0x0000000, 0x400000, CRC(303e91a1) SHA1(c29a22061ab8af8b72e0e6bdb36915a0cb5b2a5c) )
	ROM_LOAD16_BYTE( "01.bin", 0x0000001, 0x400000, CRC(3371b775) SHA1(131dd850bd01dac52fa82c41948d900c4833db3c) )
	ROM_LOAD16_BYTE( "02.bin", 0x0800000, 0x400000, CRC(4c1e13b9) SHA1(d244eb74f755350604824670db58ab2a56a856cb) )
	ROM_LOAD16_BYTE( "03.bin", 0x0800001, 0x400000, CRC(9cf86152) SHA1(e27e0d9befb12ad5c2acf547afe80d1c7921a4d1) )

	ROM_LOAD16_BYTE( "10.bin", 0x1000000, 0x400000, CRC(5ab6af41) SHA1(e29cee23c84e17dd8dabd2ec71e622c25418646e) )
	ROM_LOAD16_BYTE( "11.bin", 0x1000001, 0x400000, CRC(1d1fd633) SHA1(655be5b72bb70a90d23e49512ca84d9978d87b0b) )
	ROM_LOAD16_BYTE( "12.bin", 0x1800000, 0x400000, CRC(5b8bb9d6) SHA1(ee93774077d8a2ddcf70869a9c2f4961219a85b4) )
	ROM_LOAD16_BYTE( "13.bin", 0x1800001, 0x400000, CRC(d950df0a) SHA1(3b109341ab4ad87005113fb481b5d1ed9a82f50f) )

	ROM_LOAD16_BYTE( "40.bin", 0x2000000, 0x400000, CRC(8bedc606) SHA1(7159c8b86e8d7d5ae202c239638483ccdc7dfc25) )
	ROM_LOAD16_BYTE( "41.bin", 0x2000001, 0x400000, CRC(e19713dd) SHA1(a8f1b716913f2e391abf277e5bf0e9986cc75898) )
	ROM_LOAD16_BYTE( "50.bin", 0x2800000, 0x400000, CRC(85ca9ce9) SHA1(c5a7270507522e11e9485196be325508846fda90) )
	ROM_LOAD16_BYTE( "51.bin", 0x2800001, 0x400000, CRC(6ba1d2ec) SHA1(bbe7309b33f213c8cb9ab7adb3221ea79f89e8b0) )

	ROM_LOAD16_BYTE( "60.bin", 0x3000000, 0x400000, CRC(7cb132e0) SHA1(f9c366befec46c7f6e307111a62eede029202b16) )
	ROM_LOAD16_BYTE( "61.bin", 0x3000001, 0x400000, CRC(caa7e854) SHA1(592867e001abd0781f83a5124bf9aa62ad1aa7f3) )
	ROM_LOAD16_BYTE( "70.bin", 0x3800000, 0x400000, CRC(9b737ae4) SHA1(0b62a90d42ace81ee32db073a57731a55a32f989) )
	ROM_LOAD16_BYTE( "71.bin", 0x3800001, 0x400000, CRC(189f694e) SHA1(ad0799d4aadade51be38d824910d299257a758a3) )

	ROM_REGION( 0xc00000, REGION_USER2, 0 ) /* Other Roms */

	ROM_REGION( 0x800000, REGION_USER3, 0 ) /* Sound Roms? */
	/* probably actually just maps with the others ... */
	ROM_LOAD16_BYTE( "21.bin", 0x0000001, 0x400000, CRC(bb5fa8da) SHA1(620e609b3e2524d06d58844625f186fd4682205f))
ROM_END


GAMEX( 1997, rabbit, 0, rabbit, rabbit, 0, ROT0, "Electronic Arts", "Rabbit", GAME_NOT_WORKING | GAME_NO_SOUND )
GAMEX( 1997, tmmjprd, 0, rabbit, rabbit, 0, ROT0, "Media / Sonnet", "Tokimeki Mahjong Paradise - Dear My Love", GAME_NOT_WORKING | GAME_NO_SOUND )
