/* Jaleco Mahjong Games */
/* Board:	MJ-8956 */

/*

Board:	MJ-8956

CPU:	68000-8
	M50747 (not dumped)
Sound:	M6295
OSC:	12.000MHz
	4.000MHz


done:
rom loading
gfx decode

todo:

everything else
finish me :-)

similar to nmk16.c?

*/

/*

68k interrupts
lev 1 : 0x64 : 0000 049e -
lev 2 : 0x68 : 0000 049e -
lev 3 : 0x6c : 0000 049e -
lev 4 : 0x70 : 0000 091a -
lev 5 : 0x74 : 0000 0924 -
lev 6 : 0x78 : 0000 092e -
lev 7 : 0x7c : 0000 0938 -

*/

#include "driver.h"

static int respcount;

static READ16_HANDLER( daireika_mcu_r )
{
	static int resp[] = {	0x99, 0xd8, 0x00,
							0x2a, 0x6a, 0x00,
							0x9c, 0xd8, 0x00,
							0x2f, 0x6f, 0x00,
							0x22, 0x62, 0x00,
							0x25, 0x65, 0x00 };
	int res;

	res = resp[respcount++];
	if (respcount >= sizeof(resp)/sizeof(resp[0])) respcount = 0;

log_cb(RETRO_LOG_DEBUG, LOGPRE "%04x: mcu_r %02x\n",activecpu_get_pc(),res);

	return res;
}

static MACHINE_INIT (daireika)
{
	respcount = 0;
}

VIDEO_START(jalmah)
{
	return 0;
}


VIDEO_UPDATE(jalmah)
{

}

INPUT_PORTS_START( jalmah )
INPUT_PORTS_END

static MEMORY_READ16_START( readmem )
	{ 0x000000, 0x03ffff, MRA16_ROM },
	{ 0x080004, 0x080005, daireika_mcu_r },
	{ 0x0f0000, 0x0fffff, MRA16_RAM },
MEMORY_END

static MEMORY_WRITE16_START( writemem )
	{ 0x000000, 0x03ffff, MWA16_ROM },
	{ 0x0f0000, 0x0fffff, MWA16_RAM },
MEMORY_END

static struct GfxLayout charlayout =
{
	8,8,
	RGN_FRAC(1,1),
	4,
	{ 0, 1, 2, 3 },
	{ 0*4, 1*4, 2*4, 3*4, 4*4, 5*4, 6*4, 7*4 },
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32 },
	32*8
};

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

static struct GfxDecodeInfo jalmah_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &charlayout, 0x000, 16 },
	{ REGION_GFX2, 0, &tilelayout, 0x000, 16 },
	{ REGION_GFX3, 0, &tilelayout, 0x000, 16 },
	{ REGION_GFX4, 0, &tilelayout, 0x000, 16 },
	{ -1 } /* end of array */
};

static MACHINE_DRIVER_START( jalmah )
	MDRV_CPU_ADD(M68000, 8000000)
	MDRV_CPU_MEMORY(readmem,writemem)
/*	MDRV_CPU_VBLANK_INT(irq1_line_hold,1)*/


	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_GFXDECODE(jalmah_gfxdecodeinfo)

	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(40*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 40*8-1, 1*8, 30*8-1)
	MDRV_PALETTE_LENGTH(0x300)
	MDRV_MACHINE_INIT ( daireika )

	MDRV_VIDEO_START(jalmah)
	MDRV_VIDEO_UPDATE(jalmah)

MACHINE_DRIVER_END




/*

Mahjong Daireikai (JPN Ver.)
(c)1989 Jaleco / NMK

*/

ROM_START( daireika )
	ROM_REGION( 0x80000, REGION_CPU1, 0 ) /* 68000 Code */
	ROM_LOAD16_BYTE( "mj1.bin", 0x00001, 0x20000, CRC(3b4e8357) SHA1(1ad3e40ec6b6ff4f1c9c09d7b530f67b460151d8) )
	ROM_LOAD16_BYTE( "mj2.bin", 0x00000, 0x20000, CRC(c54d2f9b) SHA1(d59fc5a9e5bbb96b3b6a43378f4f2215c368b671) )

	ROM_REGION( 0x80000, REGION_SOUND1, 0 ) /* Samples */
	ROM_LOAD( "mj3.bin", 0x00000, 0x80000, CRC(65bb350c) SHA1(e77866f2d612a0973adc616717e7c89a37d6c48e) )

	ROM_REGION( 0x10000, REGION_GFX1, 0 ) /* BG0 */
	ROM_LOAD( "mj14.bin", 0x00000, 0x10000, CRC(c84c5577) SHA1(6437368d3be39739d62158590ecd373aa070a9b2) )

	ROM_REGION( 0x10000, REGION_GFX2, 0 ) /* BG1 */
	ROM_LOAD( "mj13.bin", 0x00000, 0x10000, CRC(c54bca14) SHA1(ee9c99858817aedd70bd6266b7a71c3c5ad00607) )

	ROM_REGION( 0x40000, REGION_GFX3, 0 ) /* BG2 */
	ROM_LOAD( "mj12.bin", 0x00000, 0x20000, CRC(236f809f) SHA1(9e15dd8a810a9d4f7f75f084d6bd277ea7d0e40a) )
	ROM_LOAD( "mj11.bin", 0x20000, 0x20000, CRC(14867c51) SHA1(b282b5048a55c9ad72ceb0d23f010a0fee78704f) )

	ROM_REGION( 0x80000, REGION_GFX4, 0 ) /* BG3 */
	ROM_LOAD( "mj10.bin", 0x00000, 0x80000, CRC(1f5509a5) SHA1(4dcdee0e159956cf73f5f85ce278479be2a9ca9f) )

	ROM_REGION( 0x220, REGION_USER1, 0 ) /* Proms */
	ROM_LOAD( "mj15.bpr", 0x000, 0x100, CRC(ebac41f9) SHA1(9d1629d977849663392cbf03a3ddf76665f88608) )
	ROM_LOAD( "mj16.bpr", 0x100, 0x100, CRC(8d5dc1f6) SHA1(9f723e7cd44f8c09ec30b04725644346484ec753) )
	ROM_LOAD( "mj17.bpr", 0x200, 0x020, CRC(a17c3e8a) SHA1(d7969fad7cec9c792c53aa457f4ad764a727e0a5) )
ROM_END

/*

Mahjong Channel Zoom In (JPN Ver.)
(c)1990 Jaleco

*/

ROM_START( mjzoomin )
	ROM_REGION( 0x80000, REGION_CPU1, 0 ) /* 68000 Code */
	ROM_LOAD16_BYTE( "zoomin-1.bin", 0x00001, 0x20000, CRC(b8b04d30) SHA1(abb163a9965421b4d92114bba974ccb13bb57f5a) )
	ROM_LOAD16_BYTE( "zoomin-2.bin", 0x00000, 0x20000, CRC(c7eb982c) SHA1(9006ded2aa1fef38bde114110d76b20747c32658) )

	ROM_REGION( 0x80000, REGION_SOUND1, 0 ) /* Samples */
	ROM_LOAD( "zoomin-3.bin", 0x00000, 0x80000, CRC(07d7b8cd) SHA1(e05ce80ffb945b04f93f8c49d0c840b0bff6310b) )

	ROM_REGION( 0x20000, REGION_GFX1, 0 ) /* BG0 */
	ROM_LOAD( "zoomin14.bin", 0x00000, 0x20000, CRC(4e32aa45) SHA1(450a3449ca8b4f0dfe8b62cceaee9366eaf3dc3d) )

	ROM_REGION( 0x20000, REGION_GFX2, 0 ) /* BG1 */
	ROM_LOAD( "zoomin13.bin", 0x00000, 0x20000, CRC(888d79fe) SHA1(eb9671d4c7608edd1231dc0cae47aab2430cbd66) )

	ROM_REGION( 0x40000, REGION_GFX3, 0 ) /* BG2 */
	ROM_LOAD( "zoomin12.bin", 0x00000, 0x40000, CRC(b0b94554) SHA1(10490b7475810910140ce075e62f604b914e5511) )

	ROM_REGION( 0x80000, REGION_GFX4, 0 ) /* BG3 */
	ROM_LOAD( "zoomin10.bin", 0x00000, 0x80000, CRC(40aec575) SHA1(ef7a3c7a94523c5967ab774936b873c9629e0e44) )

	ROM_REGION( 0x220, REGION_USER1, 0 ) /* Proms */
	ROM_LOAD( "mj15.bpr", 0x000, 0x100, CRC(ebac41f9) SHA1(9d1629d977849663392cbf03a3ddf76665f88608) )
	ROM_LOAD( "mj16.bpr", 0x100, 0x100, CRC(8d5dc1f6) SHA1(9f723e7cd44f8c09ec30b04725644346484ec753) )
	ROM_LOAD( "mj17.bpr", 0x200, 0x020, CRC(a17c3e8a) SHA1(d7969fad7cec9c792c53aa457f4ad764a727e0a5) )
ROM_END

/*

Mahjong Kakumei (JPN Ver.)
(c)1990 Jaleco


*/

ROM_START( kakumei )
	ROM_REGION( 0x80000, REGION_CPU1, 0 ) /* 68000 Code */
	ROM_LOAD16_BYTE( "mj-re-1.bin", 0x00001, 0x20000, CRC(b90215be) SHA1(10384237f734836acefb4b5f53a6ddd9054d63ff) )
	ROM_LOAD16_BYTE( "mj-re-2.bin", 0x00000, 0x20000, CRC(37eff266) SHA1(1d9e88c0270daadfafff1f73eb617e77b1d199d6) )

	ROM_REGION( 0x40000, REGION_SOUND1, 0 ) /* Samples */
	ROM_LOAD( "rom3.bin", 0x00000, 0x40000, CRC(c9b7a526) SHA1(edec57e66d4ff601c8fdef7b1405af84a3f3d883) )

	ROM_REGION( 0x20000, REGION_GFX1, 0 ) /* BG0 */
	ROM_LOAD( "rom14.bin", 0x00000, 0x20000, CRC(63e88dd6) SHA1(58734c8caf1b1ddc4cf0437ffd8109292b76c4e1) )

	ROM_REGION( 0x20000, REGION_GFX2, 0 ) /* BG1 */
	ROM_LOAD( "rom13.bin", 0x00000, 0x20000, CRC(9bef4fc2) SHA1(6598ab9dba513efcda01e47cc7752b47a97f2c6a) )

	ROM_REGION( 0x40000, REGION_GFX3, 0 ) /* BG2 */
	ROM_LOAD( "rom12.bin", 0x00000, 0x40000, CRC(31620a61) SHA1(11593ca7760e1a628e63aa48d9ad3800cf7af275) )

	ROM_REGION( 0x80000, REGION_GFX4, 0 ) /* BG3 */
	ROM_LOAD( "rom10.bin", 0x00000, 0x80000, CRC(88366377) SHA1(163a08415a631c8a09a0a55bc2819988d850f2ad) )

	ROM_REGION( 0x220, REGION_USER1, 0 ) /* Proms */
	ROM_LOAD( "mj15.bpr", 0x000, 0x100, CRC(ebac41f9) SHA1(9d1629d977849663392cbf03a3ddf76665f88608) )
	ROM_LOAD( "mj16.bpr", 0x100, 0x100, CRC(8d5dc1f6) SHA1(9f723e7cd44f8c09ec30b04725644346484ec753) )
	ROM_LOAD( "mj17.bpr", 0x200, 0x020, CRC(a17c3e8a) SHA1(d7969fad7cec9c792c53aa457f4ad764a727e0a5) )
ROM_END

/*

Mahjong Kakumei2 Princess League (JPN Ver.)
(c)1992 Jaleco

*/

ROM_START( kakumei2 )
	ROM_REGION( 0x80000, REGION_CPU1, 0 ) /* 68000 Code */
	ROM_LOAD16_BYTE( "mj-8956.1", 0x00001, 0x40000, CRC(db4ce32f) SHA1(1ae13627b9922143f462b1c3bbed87374f6e1667) )
	ROM_LOAD16_BYTE( "mj-8956.2", 0x00000, 0x40000, CRC(0f942507) SHA1(7ec2fbeb9a34dfc80c4df3de8397388db13f5c7c) )

	ROM_REGION( 0x80000, REGION_SOUND1, 0 ) /* Samples */
	ROM_LOAD( "92000-01.3", 0x00000, 0x80000, CRC(4b0ed440) SHA1(11961d217a41f92b60d5083a5e346c245f7db620) )

	ROM_REGION( 0x20000, REGION_GFX1, 0 ) /* BG0 */
	ROM_LOAD( "mj-8956.14", 0x00000, 0x20000, CRC(2b2fe999) SHA1(d9d601e2c008791f5bff6e7b1340f754dd094201) )

	ROM_REGION( 0x20000, REGION_GFX2, 0 ) /* BG1 */
	ROM_LOAD( "mj-8956.13", 0x00000, 0x20000, CRC(afe93cf4) SHA1(1973dc5821c6df68e20f8a84b5c9ae281dd3f85f) )

	ROM_REGION( 0x20000, REGION_GFX3, 0 ) /* BG2 */
	ROM_LOAD( "mj-8956.12", 0x00000, 0x20000, CRC(43f7853d) SHA1(54fb523b27e79aa295900c478f09cc080fea0adf) )

	ROM_REGION( 0x80000, REGION_GFX4, 0 ) /* BG3 */
	ROM_LOAD( "92000-02.10", 0x00000, 0x80000, CRC(338fa9b2) SHA1(05ba4b3c44249cf92be238bf53d6345dc49b0881) )

	ROM_REGION( 0x220, REGION_USER1, 0 ) /* Proms */
	ROM_LOAD( "mj15.bpr", 0x000, 0x100, CRC(ebac41f9) SHA1(9d1629d977849663392cbf03a3ddf76665f88608) )
	ROM_LOAD( "mj16.bpr", 0x100, 0x100, CRC(8d5dc1f6) SHA1(9f723e7cd44f8c09ec30b04725644346484ec753) )
	ROM_LOAD( "mj17.bpr", 0x200, 0x020, CRC(a17c3e8a) SHA1(d7969fad7cec9c792c53aa457f4ad764a727e0a5) )
ROM_END



GAMEX( 1989, daireika, 0, jalmah, jalmah, 0, ROT0, "Jaleco / NMK", "Mahjong Daireikai", GAME_NO_SOUND | GAME_NOT_WORKING )
GAMEX( 1990, mjzoomin, 0, jalmah, jalmah, 0, ROT0, "Jaleco",       "Mahjong Channel Zoom In", GAME_NO_SOUND | GAME_NOT_WORKING  )
GAMEX( 1990, kakumei,  0, jalmah, jalmah, 0, ROT0, "Jaleco",       "Mahjong Kakumei", GAME_NO_SOUND | GAME_NOT_WORKING  )
GAMEX( 1992, kakumei2, 0, jalmah, jalmah, 0, ROT0, "Jaleco",       "Mahjong Kakumei 2 - Princess League", GAME_NO_SOUND | GAME_NOT_WORKING  )
