/***************************************************************************

Appoooh memory map (preliminary)
Similar to Bank Panic

driver by Tatsuyuki Satoh


0000-9fff ROM
a000-dfff BANKED ROM
e000-e7ff RAM
e800-efff RAM??

write:
f000-f01f Sprite RAM #1
f020-f3ff Video  RAM #1
f420-f7ff Color  RAM #1
f800-f81f Sprite RAM #2
f820-fbff Video  RAM #2
fc20-ffff Color  RAM #2

I/O

read:
00  IN0
01  IN1
03  DSW
04  IN2

write:
00  SN76496 #1
01  SN76496 #2
02  SN76496 #3
03  MSM5205 address write
04  bit 0   = NMI enable
    bit 1   = flipscreen
    bit 2-3 = ?
    bit 4-5 = priority
    bit 6   = bank rom select
    bit 7   = ?
05  horizontal scroll ??

Credits:
- Tatsuyuki Satoh: MAME driver

***************************************************************************/

#include "driver.h"
#include "appoooh.h"

static unsigned char *adpcmptr = 0;
static int appoooh_adpcm_data;

static void appoooh_adpcm_int(int num)
{
	if( adpcmptr )
	{
		if( appoooh_adpcm_data==-1)
		{
			appoooh_adpcm_data = *adpcmptr++;
			MSM5205_data_w(0,appoooh_adpcm_data >> 4);
			if(appoooh_adpcm_data==0x70)
			{
				adpcmptr = 0;
				MSM5205_reset_w(0,1);
			}
		}else{
			MSM5205_data_w(0,appoooh_adpcm_data & 0x0f );
			appoooh_adpcm_data =-1;
		}
	}
}
/* adpcm address write */
static WRITE_HANDLER( appoooh_adpcm_w )
{
	unsigned char *RAM = memory_region(REGION_SOUND1);
	adpcmptr  = &RAM[data*256];
	MSM5205_reset_w(0,0);
	appoooh_adpcm_data=-1;
}



static MEMORY_READ_START( readmem )
	{ 0x0000, 0x9fff, MRA_ROM },
	{ 0xa000, 0xdfff, MRA_BANK1 },
	{ 0xe000, 0xe7ff, MRA_RAM },
	{ 0xe800, 0xefff, MRA_RAM }, /* RAM ? */
	{ 0xf000, 0xffff, MRA_RAM },
MEMORY_END

static MEMORY_WRITE_START( writemem )
	{ 0x0000, 0xdfff, MWA_ROM },
	{ 0xe000, 0xe7ff, MWA_RAM },
	{ 0xe800, 0xefff, MWA_RAM }, /* RAM ? */
	{ 0xf000, 0xf01f, MWA_RAM, &spriteram  },
	{ 0xf020, 0xf3ff, appoooh_fg_videoram_w, &appoooh_fg_videoram },
	{ 0xf420, 0xf7ff, appoooh_fg_colorram_w, &appoooh_fg_colorram },
	{ 0xf800, 0xf81f, MWA_RAM, &spriteram_2 },
	{ 0xf820, 0xfbff, appoooh_bg_videoram_w, &appoooh_bg_videoram },
	{ 0xfc20, 0xffff, appoooh_bg_colorram_w, &appoooh_bg_colorram },
MEMORY_END

static PORT_READ_START( readport )
	{ 0x00, 0x00, input_port_0_r },	/* IN0 */
	{ 0x01, 0x01, input_port_1_r },	/* IN1 */
	{ 0x03, 0x03, input_port_3_r },	/* DSW */
	{ 0x04, 0x04, input_port_2_r },	/* IN2 */
PORT_END

static PORT_WRITE_START( writeport )
	{ 0x00, 0x00, SN76496_0_w },
	{ 0x01, 0x01, SN76496_1_w },
	{ 0x02, 0x02, SN76496_2_w },
	{ 0x03, 0x03, appoooh_adpcm_w },
	{ 0x04, 0x04, appoooh_out_w  },
	{ 0x05, 0x05, appoooh_scroll_w }, /* unknown */
PORT_END



INPUT_PORTS_START( appoooh )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_BUTTON2 )

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_BUTTON2 | IPF_COCKTAIL )

	PORT_START	/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON3 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_BUTTON3 | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_COIN3 )
	PORT_BIT( 0xf8, IP_ACTIVE_HIGH, IPT_UNKNOWN )	/* probably unused */

	PORT_START	/* DSW */
	PORT_DIPNAME( 0x07, 0x00, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x07, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x05, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 1C_4C ) )
	PORT_DIPNAME( 0x18, 0x00, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x18, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 1C_2C ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x20, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x00, "Easy" )
	PORT_DIPSETTING(    0x80, "Hard" )
INPUT_PORTS_END



static struct GfxLayout charlayout =
{
	8,8,	/* 8*8 characters */
	2048,	/* 2048 characters */
	3,	/* 3 bits per pixel */
	{ 2*2048*8*8, 1*2048*8*8, 0*2048*8*8 },	/* the bitplanes are separated */
	{ 7, 6, 5, 4, 3, 2, 1, 0 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8	/* every char takes 8 consecutive bytes */
};

static struct GfxLayout spritelayout =
{
	16,16,	/* 8*8 characters */
	512,	/* 512 characters */
	3,	/* 3 bits per pixel */
	{ 2*2048*8*8, 1*2048*8*8, 0*2048*8*8 },	/* the bitplanes are separated */
	{ 7, 6, 5, 4, 3, 2, 1, 0 ,
	  8*8+7,8*8+6,8*8+5,8*8+4,8*8+3,8*8+2,8*8+1,8*8+0},
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
	  16*8, 17*8, 18*8, 19*8, 20*8, 21*8, 22*8, 23*8 },
	32*8	/* every char takes 8 consecutive bytes */
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &charlayout,        0, 32 },
	{ REGION_GFX2, 0, &charlayout,     32*8, 32 },
	{ REGION_GFX1, 0, &spritelayout,      0, 32 },
	{ REGION_GFX2, 0, &spritelayout,   32*8, 32 },
	{ -1 } /* end of array */
};



static struct SN76496interface sn76496_interface =
{
	3,	/* 3 chips */
	{ 18432000/6, 18432000/6, 18432000/6 },	/* ??? */
	{ 30, 30, 30 }
};

static struct MSM5205interface msm5205_interface =
{
	1,					/* 1 chip             */
	384000,				/* 384KHz             */
	{ appoooh_adpcm_int },/* interrupt function */
	{ MSM5205_S64_4B },	/* 6KHz               */
	{ 50 }
};



static MACHINE_DRIVER_START( appoooh )

	/* basic machine hardware */
	MDRV_CPU_ADD(Z80,18432000/6)	/* ??? the main xtal is 18.432 MHz */
	MDRV_CPU_MEMORY(readmem,writemem)
	MDRV_CPU_PORTS(readport,writeport)
	MDRV_CPU_VBLANK_INT(nmi_line_pulse,1)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 2*8, 30*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(32)
	MDRV_COLORTABLE_LENGTH(32*8+32*8)

	MDRV_PALETTE_INIT(appoooh)
	MDRV_VIDEO_START(appoooh)
	MDRV_VIDEO_UPDATE(appoooh)

	/* sound hardware */
	MDRV_SOUND_ADD(SN76496, sn76496_interface)
	MDRV_SOUND_ADD(MSM5205, msm5205_interface)
MACHINE_DRIVER_END



/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( appoooh )
	ROM_REGION( 0x14000, REGION_CPU1, 0 )	/* 64k for code + 16k bank */
	ROM_LOAD( "epr-5906.bin", 0x00000, 0x2000, CRC(fffae7fe) SHA1(b4bb60eb6331e503759bd963eafefa69331d6b86) )
	ROM_LOAD( "epr-5907.bin", 0x02000, 0x2000, CRC(57696cd6) SHA1(74a005d18d55fed9ece9b579d2e7e6619a47538b) )
	ROM_LOAD( "epr-5908.bin", 0x04000, 0x2000, CRC(4537cddc) SHA1(ecb71cab7b9269d713399987cbc45ff54735019f) )
	ROM_LOAD( "epr-5909.bin", 0x06000, 0x2000, CRC(cf82718d) SHA1(4408c468a422735ae8f69c03003157782f1a0210) )
	ROM_LOAD( "epr-5910.bin", 0x08000, 0x2000, CRC(312636da) SHA1(18817df6f2e480810726f7b11f289c59e712ee45) )
	ROM_LOAD( "epr-5911.bin", 0x0a000, 0x2000, CRC(0bc2acaa) SHA1(1ae904658ce9e44cdb79f0a13202aaff5c9f9480) ) /* bank0      */
	ROM_LOAD( "epr-5913.bin", 0x0c000, 0x2000, CRC(f5a0e6a7) SHA1(7fad534d1fba52078c4ea580ca7601fdd23cbfa6) ) /* a000-dfff  */
	ROM_LOAD( "epr-5912.bin", 0x10000, 0x2000, CRC(3c3915ab) SHA1(28b501bda992ac06b10dbb5f1f7d6009f2f5f48c) ) /* bank1     */
	ROM_LOAD( "epr-5914.bin", 0x12000, 0x2000, CRC(58792d4a) SHA1(8acdb0ebee5faadadd64bd64db1fdf881ee70333) ) /* a000-dfff */

	ROM_REGION( 0x0c000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "epr-5895.bin", 0x00000, 0x4000, CRC(4b0d4294) SHA1(f9f4d928c76b32cbcbaf7bfd0ebec2d4dfc37566) )	/* playfield #1 chars */
	ROM_LOAD( "epr-5896.bin", 0x04000, 0x4000, CRC(7bc84d75) SHA1(36e98eaac1ba23ab842080205bdb5b76b888ddc2) )
	ROM_LOAD( "epr-5897.bin", 0x08000, 0x4000, CRC(745f3ffa) SHA1(03f5d1d567e786e7835defc6995d1b39aee2c28d) )

	ROM_REGION( 0x0c000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "epr-5898.bin", 0x00000, 0x4000, CRC(cf01644d) SHA1(0cc1b7f7a3b33b0edf4e277e320467b19dfc5bc8) )	/* playfield #2 chars */
	ROM_LOAD( "epr-5899.bin", 0x04000, 0x4000, CRC(885ad636) SHA1(d040948f7cf030e4ab0f0509df23cb855e9c920c) )
	ROM_LOAD( "epr-5900.bin", 0x08000, 0x4000, CRC(a8ed13f3) SHA1(31c4a52fea8f26b4a79564c7e8443a88d43aee12) )

	ROM_REGION( 0x0220, REGION_PROMS, 0 )
	ROM_LOAD( "pr5921.prm",   0x0000, 0x020, CRC(f2437229) SHA1(8fb4240142f4c77f820d7c153c22ff82f66aa7b1) ) 	/* palette */
	ROM_LOAD( "pr5922.prm",   0x0020, 0x100, CRC(85c542bf) SHA1(371d92fca2ae609a47d3a2ea349f14f30b846da8) ) 	/* charset #1 lookup table */
	ROM_LOAD( "pr5923.prm",   0x0120, 0x100, CRC(16acbd53) SHA1(e5791646730c6232efa2c0327b484472c47baf21) ) 	/* charset #2 lookup table */

	ROM_REGION( 0xa000, REGION_SOUND1, 0 )	/* adpcm voice data */
	ROM_LOAD( "epr-5901.bin", 0x0000, 0x2000, CRC(170a10a4) SHA1(7b0c8427c69525cbcbe9f88b22b12aafb6949bfd) )
	ROM_LOAD( "epr-5902.bin", 0x2000, 0x2000, CRC(f6981640) SHA1(1a93913ecb64d1c459e5bbcc28c4ca3ea90f21e1) )
	ROM_LOAD( "epr-5903.bin", 0x4000, 0x2000, CRC(0439df50) SHA1(1f981c1867366fa57de25ff8f421c121d82d7321) )
	ROM_LOAD( "epr-5904.bin", 0x6000, 0x2000, CRC(9988f2ae) SHA1(f70786a46515feb92fe168fc6c4334ab105c05b2) )
	ROM_LOAD( "epr-5905.bin", 0x8000, 0x2000, CRC(fb5cd70e) SHA1(c2b069ca29b78b845d0c35c7f7452b70c93cb867) )
ROM_END



GAME( 1984, appoooh, 0, appoooh, appoooh, 0, ROT0, "[Sanritsu] Sega", "Appoooh" )
