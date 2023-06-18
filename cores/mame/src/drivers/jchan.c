/* Jackie Chan - Kung Fu Master

(c) Kaneko 1995

WIP, everything to do ...

Some of the GFX chips are kaneko16.c at least

basically this does nothing, i haven't touched it in years and i probably won't
touch it again, feel free to finish it - djh

*/

/*

68k interrupts
lev 1 : 0x64 : 0000 1ed8 -
lev 2 : 0x68 : 0000 1f26 -
lev 3 : 0x6c : 0000 1f68 -
lev 4 : 0x70 : 0000 1f94 -
lev 5 : 0x74 : 0000 1f96 -
lev 6 : 0x78 : 0000 1f98 -
lev 7 : 0x7c : 0000 1f9a -

*/

/*

Developers note:

This document is primarily intended for person attempting to emulate this game.

Complete dump of...

Kung Fu Master - Jackie Chan (C) Kaneko 1995

KANEKO PCB NUMBER: JC01X00047

CPU: TMP68HC000N-16 x 2
SND: YAMAHA YMZ280B
OSC: 16.0000MHZ, 33.3333MHZ, 28.6364MHZ
DIPS:1 DIP LABELLED SW2, 8 POSITION
     Location for SW1 on PCB, but empty.

Eproms

Location	Rom Type	PCB Label
U164		27C2001		SPA-7A
U165		27C2001		SPA-7B
U13		27C1001		27C1001A
U56		27C2001		23C8001E
U67		27C040		27C4001
U68		27C040		27C4001
U69		27C040		27C4001
U70		27C040		27C4001
U86		27C040		27C4001
U87		27C040		27C4001


there are 12 mask roms (42 pin) labelled....

Rom Label			Label on PCB		Location
JC-100-00  9511 D		SPA-0			U179
JC-101-00  9511 D		SPA-1			U180
JC-102-00  9511 D		SPA-2			U181
JC-103-00  9511 D		SPA-3			U182
JC-104-00  T39 9510K7092	SPA-4			U183
JC-105-00  T40 9510K7094	SPA-5			U184
JC-108-00  T65 9517K7012	SPA-6			U185
JC-106-00  T41 9510K7091	SPB-0			U171
JC-107-00  T42 9510K7096	SPB-1			U172
JC-200-00  W10 9510K7055	BG-0			U177
JC-300-00  T43 9510K7098	23C16000		U84
JC-301-00  W11 9510K7059	23C16000		U85

there are other positions for mask roms, but they are empty. the locations are labelled SPB-2, SPB-3 and SPA-7. perhaps this pcb is used for other Kaneko games?

The mask roms have been dumped in 4 meg banks with a 40 to 42 pin adapter.
Since this is my first 42 pin dump, I'm not 100% sure my dumping method is correct.
i intended to put them together as parts A + B + C + D.= MASKROM
i have left the parts separate incase i got the quarters mixed up somehow
so i leave it up to you to put them back together. if they are mixed up, i dumped them all the same way, so when you figure out the correct order, just rename them the same way when putting them back together.
I would appreciate some feedback on whether these are dumped correctly.

i have the original manual for this game aswell, which has the dips and other info. i've already made it available to Mamedev. if you need it, just email me and i'll send it to you.

I can see a large space on the PCB near the JAMMA edge connector. Written on the PCB is MC1091. I would assume this is some sort of protection module, which unfortunately someone has ripped off the pcb. Yep, that's right, this PCB is not working.

Other chips
located next to U13 is a small 8 pin chip..... AMTEL AT93C46
LATTICE pLSI 2032-80LJ (x 2, square, socketed)
FUJITSU CG24143 4181 9449 Z01 (x 2, square SMD)
FUJITSU CG24173 6186 9447 Z01 (x 2, square SMD)
KANEKO VIEW2-CHIP 1633F1208 (square, SMD)
KANEKO BABY004 9511EX009 VT-171 (square, SMD)
KANEKO TBS0P01 452 9430HK001 (square, SMD)


Ram i can see...
SONY CXK58257ASP-10L (x 8)
NEC D42101C-3 (x 4)
SHARP LH5497D-20 (x 2)
SANYO LC3564SM-85 (x 12, SMD)
NEC 42S4260-70 (x 4, SMD)

there are 9 PALS on the pcb (not dumped)

*/

/*  this one is not gurus dump

524288  DeflatX  70733  87%  03-08-2000  22:26  b1aadc5a --wa  Jm00x3-U68   // 68k code (Main)
524288  DeflatX 156800  71%  03-08-2000  22:25  c0adb141 --wa  Jm01x3-U67   // 68k code (Main)
524288  DeflatX  38310  93%  03-08-2000  22:27  d2e3f913 --wa  Jm11x3-U69   // 68k code (Main)
524288  DeflatX  54963  90%  03-08-2000  22:28  ee08fee1 --wa  Jm10x3-U70   // 68k code (Main)

524288  DeflatX  25611  96%  03-08-2000  22:23  d15d2b8e --wa  Jsp1x3-U86   // 68k code (2nd)
524288  DeflatX  18389  97%  03-08-2000  22:24  ebec50b1 --wa  Jsp0x3-U87   // 68k code (2nd)

// SPA-x
2097152  DeflatX 1203535  43%  03-08-2000  21:06  c38c5f84 --wa  Jc-100-00
2097152  DeflatX 1555500  26%  03-08-2000  21:07  cc47d68a --wa  Jc-101-00
2097152  DeflatX 1446459  32%  03-08-2000  22:12  e08f1dee --wa  Jc-102-00
2097152  DeflatX 1569324  26%  03-08-2000  22:14  ce0c81d8 --wa  Jc-103-00
2097152  DeflatX 1535427  27%  03-08-2000  22:16  6b2a2e93 --wa  Jc-104-00
2097152  DeflatX 236167   89%  03-08-2000  22:19  73cad1f0 --wa  Jc-105-00
2097152  DeflatX 1138206  46%  03-08-2000  22:21  67dd1131 --wa  Jc-108-00

// SPB-x
2097152  DeflatX 1596989  24%  03-08-2000  20:59  bc65661b --wa  Jc-106-00
2097152  DeflatX 1571950  26%  03-08-2000  21:02  92a86e8b --wa  Jc-107-00

// BG
1048576  DeflatX 307676  71%  03-08-2000  21:03  1f30c24e --wa  Jc-200-00

// AUDIO
2097152  DeflatX 1882895  11%  03-08-2000  20:57  13d5b1eb --wa  Jc-300-00
1048576  DeflatX 858475   19%  03-08-2000  22:42  9c5b3077 --wa  Jc-301-00

// AUDIO2 ?
 262144  DeflatX 218552  17%  03-08-2000  22:31  bcf25c2a --wa  Jcw0x0-U56

// MCU DATA?
 131072  DeflatX 123787   6%  03-08-2000  22:28  2a41da9c --wa  Jcd0x1-U13

// UNKNOWNS
 262144  DeflatX 160330  39%  03-08-2000  22:29  9a012cbc --wa  Jcs0x3-U164
 262144  DeflatX 160491  39%  03-08-2000  22:30  57ae7c8d --wa  Jcs1x3-U165

 */

#include "driver.h"

/* vidhrdw */


VIDEO_START(jchan)
{
	return 0;
}

VIDEO_UPDATE(jchan)
{

}

/* memory maps */

data16_t prot_data;

READ16_HANDLER ( jchan_prot_r )
{
	return prot_data;
}

WRITE16_HANDLER( jchan_prot_w )
{
	prot_data = data;
}

static MEMORY_READ16_START( readmem )
	{ 0x000000, 0x1fffff, MRA16_ROM },
	{ 0x200000, 0x20ffff, MRA16_RAM },
	{ 0x300000, 0x30ffff, MRA16_RAM },
/*	{ 0x400000, 0x40ffff, MRA16_RAM },*/
	{ 0x400002, 0x400003, jchan_prot_r }, /* maybe*/
	{ 0x500000, 0x503fff, MRA16_RAM },
	{ 0x700000, 0x7007ff, MRA16_RAM },
	{ 0x700800, 0x701fff, MRA16_RAM },
	{ 0x708000, 0x70ffff, MRA16_RAM },
MEMORY_END

static MEMORY_WRITE16_START( writemem )
	{ 0x000000, 0x1fffff, MWA16_ROM },
	{ 0x200000, 0x20ffff, MWA16_RAM },
	{ 0x300000, 0x30ffff, MWA16_RAM },
/*	{ 0x400000, 0x40ffff, MWA16_RAM },*/
	{ 0x403ffe, 0x403fff, jchan_prot_w }, /* maybe*/
	{ 0x500000, 0x503fff, MWA16_RAM },
	{ 0x700000, 0x7007ff, MWA16_RAM },
	{ 0x700800, 0x701fff, MWA16_RAM },
	{ 0x708000, 0x70ffff, MWA16_RAM },
	{ 0xF80000, 0xF80001, MWA16_RAM }, /* ???*/
MEMORY_END



/* wrong */
static struct GfxLayout charlayout =
{
	16,16,
	RGN_FRAC(1,1),
	4,
	{ 0,1,2,3 },
	{ 4, 0, 12, 8, 20, 16, 28, 24, 8*32+4, 8*32+0, 8*32+12, 8*32+8, 8*32+20, 8*32+16, 8*32+28, 8*32+24 },
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32, 16*32,17*32, 18*32, 19*32, 20*32, 21*32, 22*32, 23*32 },
	32*32
};

/* gfx decode , this one seems ok */

static struct GfxLayout char2layout =
{
	8,8,
	RGN_FRAC(1,1),
	1,
	{ 0 },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &char2layout,   0, 1  },
	{ REGION_GFX2, 0, &char2layout,   0, 1  },
	{ REGION_GFX3, 0, &charlayout,   0, 1  },
	{ REGION_GFX4, 0, &char2layout,   0, 1  },
	{ -1 } /* end of array */
};

/* input ports */

INPUT_PORTS_START( jchan )
	PORT_START	/* 16-bit */
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

	PORT_START	/* 8-bit */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
INPUT_PORTS_END

/* sound stuff */

/* machine driver */

static MACHINE_DRIVER_START( jchan )
	MDRV_CPU_ADD(M68000, 12000000)
	MDRV_CPU_MEMORY(readmem,writemem)
	MDRV_CPU_VBLANK_INT(irq1_line_hold,1) /* ?*/


	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_GFXDECODE(gfxdecodeinfo)

	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(64*8, 64*8)
	MDRV_VISIBLE_AREA(0*8, 64*8-1, 0*8, 64*8-1)
	MDRV_PALETTE_LENGTH(0x2000)

	MDRV_VIDEO_START(jchan)
	MDRV_VIDEO_UPDATE(jchan)

	/* sound hardware */
MACHINE_DRIVER_END

/* rom loading */

ROM_START( jchan )
	ROM_REGION( 0x200000, REGION_CPU1, 0 ) /* 68000 Code */
	ROM_LOAD16_BYTE( "jm01x3.u67", 0x000001, 0x080000, CRC(c0adb141) SHA1(de265e1da06e723492e0c2465cd04e25ce1c237f) )
	ROM_LOAD16_BYTE( "jm00x3.u68", 0x000000, 0x080000, CRC(b1aadc5a) SHA1(0a93693088c0a4b8a79159fb0ebac47d5556d800) )
	ROM_LOAD16_BYTE( "jm11x3.u69", 0x100001, 0x080000, CRC(d2e3f913) SHA1(db2d790fba5351660a9525f545ab1b23dfe319b0) )
	ROM_LOAD16_BYTE( "jm10x3.u70", 0x100000, 0x080000, CRC(ee08fee1) SHA1(5514bd8c625bc7cf8dd5da2f76b760716609b925) )

	ROM_REGION( 0x200000, REGION_CPU2, 0 ) /* 68000 Code */
	ROM_LOAD16_BYTE( "jsp1x3.u86", 0x000001, 0x080000, CRC(d15d2b8e) SHA1(e253f2d64fee6627f68833b441f41ea6bbb3ab07) ) /* 1xxxxxxxxxxxxxxxxxx = 0xFF*/
	ROM_LOAD16_BYTE( "jsp0x3.u87", 0x000000, 0x080000, CRC(ebec50b1) SHA1(57d7bd728349c2b9d662bcf20a3be92902cb3ffb) ) /* 1xxxxxxxxxxxxxxxxxx = 0xFF*/

	ROM_REGION( 0xc00000, REGION_GFX1, ROMREGION_DISPOSE ) /* SPA GFX? */
	ROM_LOAD( "jc-100.00", 0x000000, 0x200000, CRC(c38c5f84) SHA1(ce072c28ce02a0f79b7f5b5a2c16e5121ef6fc27) )
	ROM_LOAD( "jc-101.00", 0x200000, 0x200000, CRC(cc47d68a) SHA1(0ce2e010f2ced13d8dfca88d6f997175ba7ee417) )
	ROM_LOAD( "jc-102.00", 0x400000, 0x200000, CRC(e08f1dee) SHA1(ca3efc531f65a1cb40d17aa933e0eb506459c836) )
	ROM_LOAD( "jc-103.00", 0x600000, 0x200000, CRC(ce0c81d8) SHA1(b94b699317b123a0498f6da6d6e348d669e6860e) )
	ROM_LOAD( "jc-104.00", 0x800000, 0x200000, CRC(6b2a2e93) SHA1(e34010e39043b67493bcb23a04828ab7cda8ba4d) )
	ROM_LOAD( "jc-105.00", 0xa00000, 0x200000, CRC(73cad1f0) SHA1(5dbe4e318948e4f74bfc2d0d59455d43ba030c0d) ) /* 11xxxxxxxxxxxxxxxxxxx = 0xFF*/

	ROM_REGION( 0x600000, REGION_GFX2, ROMREGION_DISPOSE ) /* SPB GFX? */
	ROM_LOAD( "jc-106.00", 0x000000, 0x200000, CRC(bc65661b) SHA1(da28b8fcd7c7a0de427a54be2cf41a1d6a295164) )
	ROM_LOAD( "jc-107.00", 0x200000, 0x200000, CRC(92a86e8b) SHA1(c37eddbc9d84239deb543504e27b5bdaf2528f79) )
	ROM_LOAD( "jc-108.00", 0x400000, 0x200000, CRC(67dd1131) SHA1(96f334378ae0267bdb3dc528635d8d03564bd859) ) /* other region ? (label is spa)*/

	ROM_REGION( 0x100000, REGION_GFX3, ROMREGION_DISPOSE ) /* BG GFX? */
	ROM_LOAD( "jc-200.00", 0x000000, 0x100000, CRC(1f30c24e) SHA1(0c413fc67c3ec020e6786e7157d82aa242c8d2ad) )

	ROM_REGION( 0x300000, REGION_SOUND1, 0 ) /* Audio 1? */
	ROM_LOAD( "jc-300.00", 0x000000, 0x200000, CRC(13d5b1eb) SHA1(b047594d0f1a71d89b8f072879ccba480f54a483) )
	ROM_LOAD( "jc-301.00", 0x000000, 0x100000, CRC(9c5b3077) SHA1(db9a31e1c65d9f12d0f2fb316ced48a02aae089d) )

	ROM_REGION( 0x040000, REGION_SOUND2, 0 ) /* Audio 2? */
	ROM_LOAD( "jcw0x0.u56", 0x000000, 0x040000, CRC(bcf25c2a) SHA1(b57a563ab5c05b05d133eed3d099c4de997f37e4) )

	ROM_REGION( 0x020000, REGION_USER1, 0 ) /* MCU Code? */
	ROM_LOAD( "jcd0x1.u13", 0x000000, 0x020000, CRC(2a41da9c) SHA1(7b1ba0efc0544e276196b9605df1881fde871708) )

	ROM_REGION( 0x080000, REGION_GFX4, 0 ) /* Unknown, More GFX? */
	ROM_LOAD( "jcs0x3.164", 0x000000, 0x040000, CRC(9a012cbc) SHA1(b3e7390220c90d55dccfb96397f0af73925e36f9) )
	ROM_LOAD( "jcs1x3.165", 0x040000, 0x040000, CRC(57ae7c8d) SHA1(4086f638c2aabcee84e838243f0fd15cec5c040d) )
ROM_END


/* the program code of gurus set is the same, the graphic roms were split up, and might be bad but they should be the same since
the code was, decoding the gfx on the second set they appear to be partly bad. */



/* game drivers */
GAMEX( 1995, jchan, 0, jchan, jchan, 0, ROT0, "Kaneko", "Jackie Chan - Kung Fu Master", GAME_NOT_WORKING | GAME_NO_SOUND )

/* other jchan copy / paste bits

static void draw_sprites( struct mame_bitmap *bitmap, const struct rectangle *cliprect )
{

	const UINT16 *source = jchan_spriteram;
	const UINT16 *finish = source+0x2000;  // or whatever size ..
	const struct GfxElement *gfx = Machine->gfx[0];

	while( source<finish )
	{
		int xpos = source[0] & 0x00ff;
		int ypos = source[4] & 0x00ff;
		int num = (source[3] & 0x00ff) | ((source[2] & 0x00ff) << 8);
		int attr = source[1];
		int flipx = (attr & 0x0080)>>7;
		int colour = (attr & 0x000f);
		int flipy = 0;

		drawgfx(
				bitmap,
				gfx,
				num,
				colour,
				flipx,flipy,
				xpos,ypos,
				cliprect,
				TRANSPARENCY_PEN,0
				);

		source += 0x8;
	}
}


static void get_jchan_tile_info(int tile_index)
{
	int tileno;

	tileno = jchan_videoram[tile_index*2];

	SET_TILE_INFO(0,tileno,0,0)
}

WRITE16_HANDLER( jchan_videoram_w )
{
	if (jchan_videoram[offset] != data)
	{
		jchan_videoram[offset] = data;
		tilemap_mark_tile_dirty(jchan_tilemap,offset/2);
	}
}

	static struct tilemap *jchan_tilemap;
	data16_t *jchan_videoram;

	jchan_tilemap = tilemap_create(get_jchan_tile_info,tilemap_scan_rows,TILEMAP_OPAUQE, 8, 8,64,64);

	tilemap_draw(bitmap,cliprect,jchan_tilemap,0,0);


*/
