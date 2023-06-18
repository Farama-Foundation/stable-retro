/*

TODO: 1943 is almost identical to GunSmoke (one more scrolling playfield). We
      should merge the two drivers.
*/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "cpu/z80/z80.h"



extern unsigned char *c1943_scrollx;
extern unsigned char *c1943_scrolly;
extern unsigned char *c1943_bgscrolly;
WRITE_HANDLER( c1943_c804_w );	/* in vidhrdw/c1943.c */
WRITE_HANDLER( c1943_d806_w );	/* in vidhrdw/c1943.c */
PALETTE_INIT( 1943 );
VIDEO_UPDATE( 1943 );
VIDEO_START( 1943 );



/* this is a protection check. The game crashes (thru a jump to 0x8000) */
/* if a read from this address doesn't return the value it expects. */
static READ_HANDLER( c1943_protection_r )
{
	int data = activecpu_get_reg(Z80_BC) >> 8;
	log_cb(RETRO_LOG_DEBUG, LOGPRE "protection read, PC: %04x Result:%02x\n",activecpu_get_pc(),data);
	return data;
}



static MEMORY_READ_START( readmem )
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0xbfff, MRA_BANK1 },
	{ 0xd000, 0xd7ff, MRA_RAM },
	{ 0xc000, 0xc000, input_port_0_r },
	{ 0xc001, 0xc001, input_port_1_r },
	{ 0xc002, 0xc002, input_port_2_r },
	{ 0xc003, 0xc003, input_port_3_r },
	{ 0xc004, 0xc004, input_port_4_r },
	{ 0xc007, 0xc007, c1943_protection_r },
	{ 0xe000, 0xffff, MRA_RAM },
MEMORY_END

static MEMORY_WRITE_START( writemem )
	{ 0x0000, 0xbfff, MWA_ROM },
	{ 0xc800, 0xc800, soundlatch_w },
	{ 0xc804, 0xc804, c1943_c804_w },	/* ROM bank switch, screen flip */
	{ 0xc806, 0xc806, watchdog_reset_w },
	{ 0xc807, 0xc807, MWA_NOP }, 	/* protection chip write (we don't emulate it) */
	{ 0xd000, 0xd3ff, videoram_w, &videoram, &videoram_size },
	{ 0xd400, 0xd7ff, colorram_w, &colorram },
	{ 0xd800, 0xd801, MWA_RAM, &c1943_scrolly },
	{ 0xd802, 0xd802, MWA_RAM, &c1943_scrollx },
	{ 0xd803, 0xd804, MWA_RAM, &c1943_bgscrolly },
	{ 0xd806, 0xd806, c1943_d806_w },	/* sprites, bg1, bg2 enable */
	{ 0xe000, 0xefff, MWA_RAM },
	{ 0xf000, 0xffff, MWA_RAM, &spriteram, &spriteram_size },
MEMORY_END


static MEMORY_READ_START( sound_readmem )
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0xc000, 0xc7ff, MRA_RAM },
	{ 0xc800, 0xc800, soundlatch_r },
MEMORY_END

static MEMORY_WRITE_START( sound_writemem )
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0xc000, 0xc7ff, MWA_RAM },
	{ 0xe000, 0xe000, YM2203_control_port_0_w },
	{ 0xe001, 0xe001, YM2203_write_port_0_w },
	{ 0xe002, 0xe002, YM2203_control_port_1_w },
	{ 0xe003, 0xe003, YM2203_write_port_1_w },
MEMORY_END



INPUT_PORTS_START( 1943 )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* actually, this is VBLANK */
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably unused */
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably unused */
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN2 )

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* Button 3, probably unused */
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* Button 3, probably unused */
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* DSW0 */
	PORT_DIPNAME( 0x0f, 0x0f, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x0f, "1 (Easiest)" )
	PORT_DIPSETTING(    0x0e, "2" )
	PORT_DIPSETTING(    0x0d, "3" )
	PORT_DIPSETTING(    0x0c, "4" )
	PORT_DIPSETTING(    0x0b, "5" )
	PORT_DIPSETTING(    0x0a, "6" )
	PORT_DIPSETTING(    0x09, "7" )
	PORT_DIPSETTING(    0x08, "8" )
	PORT_DIPSETTING(    0x07, "9" )
	PORT_DIPSETTING(    0x06, "10" )
	PORT_DIPSETTING(    0x05, "11" )
	PORT_DIPSETTING(    0x04, "12" )
	PORT_DIPSETTING(    0x03, "13" )
	PORT_DIPSETTING(    0x02, "14" )
	PORT_DIPSETTING(    0x01, "15" )
	PORT_DIPSETTING(    0x00, "16 (Hardest)" )
	PORT_DIPNAME( 0x10, 0x10, "2 Players Game" )
	PORT_DIPSETTING(    0x00, "1 Credit" )
	PORT_DIPSETTING(    0x10, "2 Credits" )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ))
	PORT_DIPSETTING(    0x00, DEF_STR( On ))
	PORT_DIPNAME( 0x40, 0x40, "Freeze" )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ))
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_SERVICE( 0x80, IP_ACTIVE_LOW )

	PORT_START      /* DSW1 */
	PORT_DIPNAME( 0x07, 0x07, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 4C_1C ))
	PORT_DIPSETTING(    0x01, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x07, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x05, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_5C ) )
	PORT_DIPNAME( 0x38, 0x38, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 4C_1C ))
	PORT_DIPSETTING(    0x08, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x38, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x28, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x18, DEF_STR( 1C_5C ) )
	PORT_DIPNAME( 0x40, 0x40, "Allow Continue" )
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x80, DEF_STR( On ) )
INPUT_PORTS_END



static struct GfxLayout charlayout =
{
	8,8,	/* 8*8 characters */
	2048,	/* 2048 characters */
	2,	/* 2 bits per pixel */
	{ 4, 0 },
	{ 0, 1, 2, 3, 8+0, 8+1, 8+2, 8+3 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16 },
	16*8	/* every char takes 16 consecutive bytes */
};
static struct GfxLayout spritelayout =
{
	16,16,	/* 16*16 sprites */
	2048,	/* 2048 sprites */
	4,	/* 4 bits per pixel */
	{ 2048*64*8+4, 2048*64*8+0, 4, 0 },
	{ 0, 1, 2, 3, 8+0, 8+1, 8+2, 8+3,
			32*8+0, 32*8+1, 32*8+2, 32*8+3, 33*8+0, 33*8+1, 33*8+2, 33*8+3 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16,
			8*16, 9*16, 10*16, 11*16, 12*16, 13*16, 14*16, 15*16 },
	64*8	/* every sprite takes 64 consecutive bytes */
};
static struct GfxLayout fgtilelayout =
{
	32,32,  /* 32*32 tiles */
	512,    /* 512 tiles */
	4,      /* 4 bits per pixel */
	{ 512*256*8+4, 512*256*8+0, 4, 0 },
	{ 0, 1, 2, 3, 8+0, 8+1, 8+2, 8+3,
			64*8+0, 64*8+1, 64*8+2, 64*8+3, 65*8+0, 65*8+1, 65*8+2, 65*8+3,
			128*8+0, 128*8+1, 128*8+2, 128*8+3, 129*8+0, 129*8+1, 129*8+2, 129*8+3,
			192*8+0, 192*8+1, 192*8+2, 192*8+3, 193*8+0, 193*8+1, 193*8+2, 193*8+3 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16,
			8*16, 9*16, 10*16, 11*16, 12*16, 13*16, 14*16, 15*16,
			16*16, 17*16, 18*16, 19*16, 20*16, 21*16, 22*16, 23*16,
			24*16, 25*16, 26*16, 27*16, 28*16, 29*16, 30*16, 31*16 },
	256*8	/* every tile takes 256 consecutive bytes */
};
static struct GfxLayout bgtilelayout =
{
	32,32,  /* 32*32 tiles */
	128,    /* 128 tiles */
	4,      /* 4 bits per pixel */
	{ 128*256*8+4, 128*256*8+0, 4, 0 },
	{ 0, 1, 2, 3, 8+0, 8+1, 8+2, 8+3,
			64*8+0, 64*8+1, 64*8+2, 64*8+3, 65*8+0, 65*8+1, 65*8+2, 65*8+3,
			128*8+0, 128*8+1, 128*8+2, 128*8+3, 129*8+0, 129*8+1, 129*8+2, 129*8+3,
			192*8+0, 192*8+1, 192*8+2, 192*8+3, 193*8+0, 193*8+1, 193*8+2, 193*8+3 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16,
			8*16, 9*16, 10*16, 11*16, 12*16, 13*16, 14*16, 15*16,
			16*16, 17*16, 18*16, 19*16, 20*16, 21*16, 22*16, 23*16,
			24*16, 25*16, 26*16, 27*16, 28*16, 29*16, 30*16, 31*16 },
	256*8	/* every tile takes 256 consecutive bytes */
};



static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &charlayout,                  0, 32 },
	{ REGION_GFX2, 0, &fgtilelayout,             32*4, 16 },
	{ REGION_GFX3, 0, &bgtilelayout,       32*4+16*16, 16 },
	{ REGION_GFX4, 0, &spritelayout, 32*4+16*16+16*16, 16 },
	{ -1 } /* end of array */
};



static struct YM2203interface ym2203_interface =
{
	2,			/* 2 chips */
	1500000,	/* 1.5 MHz */
	{ YM2203_VOL(10,15), YM2203_VOL(10,15) },
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 }
};



static MACHINE_DRIVER_START( 1943 )

	/* basic machine hardware */
	MDRV_CPU_ADD(Z80, 6000000)	/* 6 MHz */
	MDRV_CPU_MEMORY(readmem,writemem)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,1)

	MDRV_CPU_ADD(Z80, 3000000)
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)	/* 3 MHz */
	MDRV_CPU_MEMORY(sound_readmem,sound_writemem)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,4)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 2*8, 30*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(256)
	MDRV_COLORTABLE_LENGTH(32*4+16*16+16*16+16*16)

	MDRV_PALETTE_INIT(1943)
	MDRV_VIDEO_START(1943)
	MDRV_VIDEO_UPDATE(1943)

	/* sound hardware */
	MDRV_SOUND_ADD(YM2203, ym2203_interface)
MACHINE_DRIVER_END



/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( 1943 )
	ROM_REGION( 0x30000, REGION_CPU1, 0 )	/* 64k for code + 128k for the banked ROMs images */
	ROM_LOAD( "1943.01",      0x00000, 0x08000, CRC(c686cc5c) SHA1(5efb2d9df737564d599f71b71a6438f7624b27c3) )
	ROM_LOAD( "1943.02",      0x10000, 0x10000, CRC(d8880a41) SHA1(2f9b6a3922efa05eed66c63284bace5f337304ac) )
	ROM_LOAD( "1943.03",      0x20000, 0x10000, CRC(3f0ee26c) SHA1(8da74fe91a6be3f23fc625f2a433f1f79c424994) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for the audio CPU */
	ROM_LOAD( "1943.05",      0x00000, 0x8000, CRC(ee2bd2d7) SHA1(4d2d019a9f8452fbbb247e893280568a2e86073e) )

	ROM_REGION( 0x8000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "1943.04",      0x00000, 0x8000, CRC(46cb9d3d) SHA1(96fd0e714b91fe13a2ca0d185ada9e4b4baa0c0b) )	/* characters */

	ROM_REGION( 0x40000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "1943.15",      0x00000, 0x8000, CRC(6b1a0443) SHA1(32337c840ccd6815fd5844c194365c58d708f6dc) )	/* bg tiles */
	ROM_LOAD( "1943.16",      0x08000, 0x8000, CRC(23c908c2) SHA1(42b83ff5781be9181802a21ff1b23c17ab1bc5a2) )
	ROM_LOAD( "1943.17",      0x10000, 0x8000, CRC(46bcdd07) SHA1(38feda668be25d1adc04aa36afc73b07c1545f89) )
	ROM_LOAD( "1943.18",      0x18000, 0x8000, CRC(e6ae7ba0) SHA1(959c306dc28b9be2adc54b3d46312d26764c7b8b) )
	ROM_LOAD( "1943.19",      0x20000, 0x8000, CRC(868ababc) SHA1(1c7be905f53c63bad25fbbd9b3cf82d2c7749bc3) )
	ROM_LOAD( "1943.20",      0x28000, 0x8000, CRC(0917e5d4) SHA1(62dd277bc1fa54cfe168ae2380bc147bd17f4205) )
	ROM_LOAD( "1943.21",      0x30000, 0x8000, CRC(9bfb0d89) SHA1(f1bae7ec46edcf46c7af84c054e89b322f8c8972) )
	ROM_LOAD( "1943.22",      0x38000, 0x8000, CRC(04f3c274) SHA1(932780c04abe285e1ec67b726b145175f73eafe0) )

	ROM_REGION( 0x10000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "1943.24",      0x00000, 0x8000, CRC(11134036) SHA1(88da112ab9fc7e0d8f0e901f273715b950ae588c) )	/* fg tiles */
	ROM_LOAD( "1943.25",      0x08000, 0x8000, CRC(092cf9c1) SHA1(19fe3c714b1d52cbb21dea25cdee5af841f525db) )

	ROM_REGION( 0x40000, REGION_GFX4, ROMREGION_DISPOSE )
	ROM_LOAD( "1943.06",      0x00000, 0x8000, CRC(97acc8af) SHA1(c9fa07cb61f6905408b355edabfe453fb652ff0d) )	/* sprites */
	ROM_LOAD( "1943.07",      0x08000, 0x8000, CRC(d78f7197) SHA1(6367c7e80e80d4a0d33d7840b5c843c63c80123e) )
	ROM_LOAD( "1943.08",      0x10000, 0x8000, CRC(1a626608) SHA1(755c27a07728fd686168e9d9e4dee3d8f274892a) )
	ROM_LOAD( "1943.09",      0x18000, 0x8000, CRC(92408400) SHA1(3ab299bad1ba115efead53ebd92254abe7a092ba) )
	ROM_LOAD( "1943.10",      0x20000, 0x8000, CRC(8438a44a) SHA1(873629b00cf3f6d8976a7fdafe63cd16e47b7491) )
	ROM_LOAD( "1943.11",      0x28000, 0x8000, CRC(6c69351d) SHA1(c213d5c3e76a5749bc32539604716dcef6dcb694) )
	ROM_LOAD( "1943.12",      0x30000, 0x8000, CRC(5e7efdb7) SHA1(fef271a38dc1a9e45a0c6e27e28e713c77c8f8c9) )
	ROM_LOAD( "1943.13",      0x38000, 0x8000, CRC(1143829a) SHA1(2b3a65e354a205c05a87f783e9938b64bc62396f) )

	ROM_REGION( 0x10000, REGION_GFX5, 0 )	/* tilemaps */
	ROM_LOAD( "1943.14",      0x0000, 0x8000, CRC(4d3c6401) SHA1(ce4f6dbf8fa030ad45cbb5afd58df27fed2d4618) )	/* front background */
	ROM_LOAD( "1943.23",      0x8000, 0x8000, CRC(a52aecbd) SHA1(45b0283d84d394c16c35802463ca95d70d1062d4) )	/* back background */

	ROM_REGION( 0x0c00, REGION_PROMS, 0 )
	ROM_LOAD( "bmprom.01",    0x0000, 0x0100, CRC(74421f18) SHA1(5b8b59f6f4e5ad358611de50608f47f41a5b0e51) )	/* red component */
	ROM_LOAD( "bmprom.02",    0x0100, 0x0100, CRC(ac27541f) SHA1(1796c4c9041dfe28e6319576f21df1dbcb8d12bf) )	/* green component */
	ROM_LOAD( "bmprom.03",    0x0200, 0x0100, CRC(251fb6ff) SHA1(d1118159b3d429d841e4efa938728ebedadd7ec5) )	/* blue component */
	ROM_LOAD( "bmprom.05",    0x0300, 0x0100, CRC(206713d0) SHA1(fa609f6d675af18c379838583505724d28bcff0e) )	/* char lookup table */
	ROM_LOAD( "bmprom.10",    0x0400, 0x0100, CRC(33c2491c) SHA1(13da924e4b182759c4aae49034f3a7cbe556ea65) )	/* foreground lookup table */
	ROM_LOAD( "bmprom.09",    0x0500, 0x0100, CRC(aeea4af7) SHA1(98f4570ee061e9aa58d8ed2d2f8ae59ce2ec5795) )	/* foreground palette bank */
	ROM_LOAD( "bmprom.12",    0x0600, 0x0100, CRC(c18aa136) SHA1(684f04d9a5b94ae1db5fb95763e65271f4cf8e01) )	/* background lookup table */
	ROM_LOAD( "bmprom.11",    0x0700, 0x0100, CRC(405aae37) SHA1(94a06f81b775c4e49d57d42fc064d3072a253bbd) )	/* background palette bank */
	ROM_LOAD( "bmprom.08",    0x0800, 0x0100, CRC(c2010a9e) SHA1(be9852500209066e2f0ff2770e0c217d1636a0b5) )	/* sprite lookup table */
	ROM_LOAD( "bmprom.07",    0x0900, 0x0100, CRC(b56f30c3) SHA1(9f5e6db464d21457a33ec8bdfdff069632b791db) )	/* sprite palette bank */
	ROM_LOAD( "bmprom.04",    0x0a00, 0x0100, CRC(91a8a2e1) SHA1(9583c87eff876f04bc2ccf7218cd8081f1bcdb94) )	/* priority encoder / palette selector (not used) */
	ROM_LOAD( "bmprom.06",    0x0b00, 0x0100, CRC(0eaf5158) SHA1(bafd4108708f66cd7b280e47152b108f3e254fc9) )	/* video timing (not used) */
ROM_END

ROM_START( 1943j )
	ROM_REGION( 0x30000, REGION_CPU1, 0 )	/* 64k for code + 128k for the banked ROMs images */
	ROM_LOAD( "1943jap.001",  0x00000, 0x08000, CRC(f6935937) SHA1(6fe8885d734447c2a667cf80dd545200aad6c767) )
	ROM_LOAD( "1943jap.002",  0x10000, 0x10000, CRC(af971575) SHA1(af1d8ce73e8671b7b41248ce6486c9b5aaf6a233) )
	ROM_LOAD( "1943jap.003",  0x20000, 0x10000, CRC(300ec713) SHA1(f66d2356b413a418c887b4085a5315475c7a8bba) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for the audio CPU */
	ROM_LOAD( "1943.05",      0x00000, 0x8000, CRC(ee2bd2d7) SHA1(4d2d019a9f8452fbbb247e893280568a2e86073e) )

	ROM_REGION( 0x8000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "1943.04",      0x00000, 0x8000, CRC(46cb9d3d) SHA1(96fd0e714b91fe13a2ca0d185ada9e4b4baa0c0b) )	/* characters */

	ROM_REGION( 0x40000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "1943.15",      0x00000, 0x8000, CRC(6b1a0443) SHA1(32337c840ccd6815fd5844c194365c58d708f6dc) )	/* bg tiles */
	ROM_LOAD( "1943.16",      0x08000, 0x8000, CRC(23c908c2) SHA1(42b83ff5781be9181802a21ff1b23c17ab1bc5a2) )
	ROM_LOAD( "1943.17",      0x10000, 0x8000, CRC(46bcdd07) SHA1(38feda668be25d1adc04aa36afc73b07c1545f89) )
	ROM_LOAD( "1943.18",      0x18000, 0x8000, CRC(e6ae7ba0) SHA1(959c306dc28b9be2adc54b3d46312d26764c7b8b) )
	ROM_LOAD( "1943.19",      0x20000, 0x8000, CRC(868ababc) SHA1(1c7be905f53c63bad25fbbd9b3cf82d2c7749bc3) )
	ROM_LOAD( "1943.20",      0x28000, 0x8000, CRC(0917e5d4) SHA1(62dd277bc1fa54cfe168ae2380bc147bd17f4205) )
	ROM_LOAD( "1943.21",      0x30000, 0x8000, CRC(9bfb0d89) SHA1(f1bae7ec46edcf46c7af84c054e89b322f8c8972) )
	ROM_LOAD( "1943.22",      0x38000, 0x8000, CRC(04f3c274) SHA1(932780c04abe285e1ec67b726b145175f73eafe0) )

	ROM_REGION( 0x10000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "1943.24",      0x00000, 0x8000, CRC(11134036) SHA1(88da112ab9fc7e0d8f0e901f273715b950ae588c) )	/* fg tiles */
	ROM_LOAD( "1943.25",      0x08000, 0x8000, CRC(092cf9c1) SHA1(19fe3c714b1d52cbb21dea25cdee5af841f525db) )

	ROM_REGION( 0x40000, REGION_GFX4, ROMREGION_DISPOSE )
	ROM_LOAD( "1943.06",      0x00000, 0x8000, CRC(97acc8af) SHA1(c9fa07cb61f6905408b355edabfe453fb652ff0d) )	/* sprites */
	ROM_LOAD( "1943.07",      0x08000, 0x8000, CRC(d78f7197) SHA1(6367c7e80e80d4a0d33d7840b5c843c63c80123e) )
	ROM_LOAD( "1943.08",      0x10000, 0x8000, CRC(1a626608) SHA1(755c27a07728fd686168e9d9e4dee3d8f274892a) )
	ROM_LOAD( "1943.09",      0x18000, 0x8000, CRC(92408400) SHA1(3ab299bad1ba115efead53ebd92254abe7a092ba) )
	ROM_LOAD( "1943.10",      0x20000, 0x8000, CRC(8438a44a) SHA1(873629b00cf3f6d8976a7fdafe63cd16e47b7491) )
	ROM_LOAD( "1943.11",      0x28000, 0x8000, CRC(6c69351d) SHA1(c213d5c3e76a5749bc32539604716dcef6dcb694) )
	ROM_LOAD( "1943.12",      0x30000, 0x8000, CRC(5e7efdb7) SHA1(fef271a38dc1a9e45a0c6e27e28e713c77c8f8c9) )
	ROM_LOAD( "1943.13",      0x38000, 0x8000, CRC(1143829a) SHA1(2b3a65e354a205c05a87f783e9938b64bc62396f) )

	ROM_REGION( 0x10000, REGION_GFX5, 0 )	/* tilemaps */
	ROM_LOAD( "1943.14",      0x0000, 0x8000, CRC(4d3c6401) SHA1(ce4f6dbf8fa030ad45cbb5afd58df27fed2d4618) )	/* front background */
	ROM_LOAD( "1943.23",      0x8000, 0x8000, CRC(a52aecbd) SHA1(45b0283d84d394c16c35802463ca95d70d1062d4) )	/* back background */

	ROM_REGION( 0x0c00, REGION_PROMS, 0 )
	ROM_LOAD( "bmprom.01",    0x0000, 0x0100, CRC(74421f18) SHA1(5b8b59f6f4e5ad358611de50608f47f41a5b0e51) )	/* red component */
	ROM_LOAD( "bmprom.02",    0x0100, 0x0100, CRC(ac27541f) SHA1(1796c4c9041dfe28e6319576f21df1dbcb8d12bf) )	/* green component */
	ROM_LOAD( "bmprom.03",    0x0200, 0x0100, CRC(251fb6ff) SHA1(d1118159b3d429d841e4efa938728ebedadd7ec5) )	/* blue component */
	ROM_LOAD( "bmprom.05",    0x0300, 0x0100, CRC(206713d0) SHA1(fa609f6d675af18c379838583505724d28bcff0e) )	/* char lookup table */
	ROM_LOAD( "bmprom.10",    0x0400, 0x0100, CRC(33c2491c) SHA1(13da924e4b182759c4aae49034f3a7cbe556ea65) )	/* foreground lookup table */
	ROM_LOAD( "bmprom.09",    0x0500, 0x0100, CRC(aeea4af7) SHA1(98f4570ee061e9aa58d8ed2d2f8ae59ce2ec5795) )	/* foreground palette bank */
	ROM_LOAD( "bmprom.12",    0x0600, 0x0100, CRC(c18aa136) SHA1(684f04d9a5b94ae1db5fb95763e65271f4cf8e01) )	/* background lookup table */
	ROM_LOAD( "bmprom.11",    0x0700, 0x0100, CRC(405aae37) SHA1(94a06f81b775c4e49d57d42fc064d3072a253bbd) )	/* background palette bank */
	ROM_LOAD( "bmprom.08",    0x0800, 0x0100, CRC(c2010a9e) SHA1(be9852500209066e2f0ff2770e0c217d1636a0b5) )	/* sprite lookup table */
	ROM_LOAD( "bmprom.07",    0x0900, 0x0100, CRC(b56f30c3) SHA1(9f5e6db464d21457a33ec8bdfdff069632b791db) )	/* sprite palette bank */
	ROM_LOAD( "bmprom.04",    0x0a00, 0x0100, CRC(91a8a2e1) SHA1(9583c87eff876f04bc2ccf7218cd8081f1bcdb94) )	/* priority encoder / palette selector (not used) */
	ROM_LOAD( "bmprom.06",    0x0b00, 0x0100, CRC(0eaf5158) SHA1(bafd4108708f66cd7b280e47152b108f3e254fc9) )	/* video timing (not used) */
ROM_END

ROM_START( 1943mii ) /* Prototype, location test or actual limited release? - PCB had genuine CAPCOM labels for roms */
	ROM_REGION( 0x30000, REGION_CPU1, 0 ) /* 64k for code + 128k for the banked ROMs images */
	ROM_LOAD( "01.12d",       0x00000, 0x08000, CRC(8ba22485) SHA1(ed67992d2cf7dcba72bc9525fbce6d2cb03d78c4) ) /* had USA hand written in pen on labels */
	ROM_LOAD( "02.13d",       0x10000, 0x10000, CRC(659a5455) SHA1(c4a2cea51c1326f7e60e404ae4d66e567abc4c96) )
	ROM_LOAD( "03.14d",       0x20000, 0x10000, CRC(159ea771) SHA1(d95ff1773cdc566203befd84e1ba961a7dc8f69b) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for the audio CPU */
	ROM_LOAD( "1943kai.05",   0x00000, 0x8000, CRC(25f37957) SHA1(1e50c2a920eb3b5c881843686db857e9fee5ba1d) )

	ROM_REGION( 0x8000,  REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "04.4k",        0x00000, 0x8000, CRC(8190e092) SHA1(17ca0fa8e61cc6f478d4807262a0333fdb3e4f94) )   /* characters - had USA hand written in pen on label */
	
	ROM_REGION( 0x40000, REGION_GFX2, ROMREGION_DISPOSE ) /* Mixture of standard and Kai roms */
	ROM_LOAD( "1943kai.15",   0x00000, 0x8000, CRC(6b1a0443) SHA1(32337c840ccd6815fd5844c194365c58d708f6dc) )	/* bg tiles */
	ROM_LOAD( "1943.16",      0x08000, 0x8000, CRC(23c908c2) SHA1(42b83ff5781be9181802a21ff1b23c17ab1bc5a2) )
	ROM_LOAD( "1943kai.17",   0x10000, 0x8000, CRC(3d5acab9) SHA1(887d45b648fda952ae2137579f383ab8ede1facd) )
	ROM_LOAD( "1943kai.18",   0x18000, 0x8000, CRC(7b62da1d) SHA1(1926109a2ab2f550ca87b0d2af73abd2b4a7498d) )
	ROM_LOAD( "1943kai.19",   0x20000, 0x8000, CRC(868ababc) SHA1(1c7be905f53c63bad25fbbd9b3cf82d2c7749bc3) )
	ROM_LOAD( "1943.20",      0x28000, 0x8000, CRC(0917e5d4) SHA1(62dd277bc1fa54cfe168ae2380bc147bd17f4205) )
	ROM_LOAD( "1943kai.21",   0x30000, 0x8000, CRC(8c7fe74a) SHA1(8846b57d7f47c10ab1f505c359ecf36dcbacb011) )
	ROM_LOAD( "1943kai.22",   0x38000, 0x8000, CRC(d5ef8a0e) SHA1(2e42b1fbbfe823a33740a56d1334657db56d24d2) )
	
	ROM_REGION( 0x10000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "24.14k",       0x00000, 0x8000, CRC(a0074c68) SHA1(c219de2253d1964ae3e3daf60c5f9a563b94b4eb) )  /* fg tiles */
	ROM_LOAD( "25.14l",       0x08000, 0x8000, CRC(f979b2f2) SHA1(06db7b812cf51b3e4476a56bca410ba04e55b925) )

	ROM_REGION( 0x40000, REGION_GFX4, ROMREGION_DISPOSE ) /* Only 08 & 12 match known roms, the rest are unique to this set */
	ROM_LOAD( "06.10a",       0x00000, 0x8000, CRC(b261d5d7) SHA1(4f249c213d2853b8a524baba148730fd4dd1536f) )  /* sprites */
	ROM_LOAD( "07.11a",       0x08000, 0x8000, CRC(2af8a6f2) SHA1(f97a08dbdb57de01c21821ddcc30ebe2d57edb17) )
	ROM_LOAD( "1943kai.08",   0x10000, 0x8000, CRC(159d51bd) SHA1(746aa49b18aff0eaf2fb875c573d455416d45a1d) )
	ROM_LOAD( "09.14a",       0x18000, 0x8000, CRC(70d9f9a7) SHA1(c8d1d3ab4d8baca7fbb5b1d9b3de72c46af5bbd7) )
	ROM_LOAD( "10.10c",       0x20000, 0x8000, CRC(de539920) SHA1(957ab527032e19e57ab1afa5e5e08763104d4c9a) )
	ROM_LOAD( "11.11c",       0x28000, 0x8000, CRC(a6abf183) SHA1(97cf3d00d23e062e15bcba7914e184b249f2c714) )
	ROM_LOAD( "1943kai.12",   0x30000, 0x8000, CRC(0f50c001) SHA1(0e6367d3f0ba39a00ee0fa6e42ae9d43d12da23d) )
	ROM_LOAD( "13.14c",       0x38000, 0x8000, CRC(f065f619) SHA1(d45b3a7ce306b3dc7b2ccea2484c13c1ff08a0f7) )

	ROM_REGION( 0x10000, REGION_GFX5, 0 )    /* tilemaps */
	ROM_LOAD( "14.5f",        0x0000, 0x8000, CRC(02a899f1) SHA1(0f094d925a6e38e922eb487af80da9c9ee7613aa) )    /* front background */
	ROM_LOAD( "23.8k",        0x8000, 0x8000, CRC(b6dfdf85) SHA1(c223ae136f67e5f9910cbfa49b9827e5122e018e) )    /* back background */

	ROM_REGION( 0x0c00, REGION_PROMS, 0 )
/*  PCB had standard BM0x for bproms 1 through 3, but clearly these should use the Kai BPROMs for correct colors*/
/*  BPROMs 4 through 8 macth the Kai set - labels were a non descript yellow dot with prom number*/
/*  BPROMs 9 through 12 are unique - labels were a non descript yellow dot with prom number*/
	ROM_LOAD( "bmk01.bin",    0x0000, 0x0100, CRC(e001ea33) SHA1(4204bdf87820ac84bab2a1b5571a2ee28c4cdfc5) )	/* red component */
	ROM_LOAD( "bmk02.bin",    0x0100, 0x0100, CRC(af34d91a) SHA1(94bc6514c980fdd1cb013ff0819d6f32464c581c) )	/* green component */
	ROM_LOAD( "bmk03.bin",    0x0200, 0x0100, CRC(43e9f6ef) SHA1(e1f58368fe0bd9b53f6c286ce5009b218a5197dc) )	/* blue component */
	ROM_LOAD( "bmk05.bin",    0x0300, 0x0100, CRC(41878934) SHA1(8f28210ab1d409c89600169a136b74a706001cdf) )	/* char lookup table */
	ROM_LOAD( "10.7l",        0x0400, 0x0100, CRC(db53adf0) SHA1(e3e3a3c262acc628541afa512cfa4ed0c6fc547f) )    /* foreground lookup table */
	ROM_LOAD( "9.6l",         0x0500, 0x0100, CRC(75d5cc90) SHA1(2f04236e7635583fe096c11165fa0a8a0e121d70) )    /* foreground palette bank */
	ROM_LOAD( "12.12m",       0x0600, 0x0100, CRC(784bdf33) SHA1(6a46c2048637770acd3f3d791e1b831e8caf8c99) )    /* background lookup table */
	ROM_LOAD( "11.12l",       0x0700, 0x0100, CRC(6fb2e170) SHA1(91a84f7138c373da0b50d4833de36f17db9a553e) )    /* background palette bank */
	ROM_LOAD( "bmk08.bin",    0x0800, 0x0100, CRC(dad17e2d) SHA1(fdb18ddc7574153bb7e27ba08b04b9dc87061c02) )	/* sprite lookup table */
	ROM_LOAD( "bmk07.bin",    0x0900, 0x0100, CRC(76307f8d) SHA1(8d655e2a5c50541795316d924b2f18b55f4b9571) )	/* sprite palette bank */
	ROM_LOAD( "bmprom.04",    0x0a00, 0x0100, CRC(91a8a2e1) SHA1(9583c87eff876f04bc2ccf7218cd8081f1bcdb94) )	/* priority encoder / palette selector (not used) */
	ROM_LOAD( "bmprom.06",    0x0b00, 0x0100, CRC(0eaf5158) SHA1(bafd4108708f66cd7b280e47152b108f3e254fc9) )	/* video timing (not used) */
ROM_END

ROM_START( 1943kai )
	ROM_REGION( 0x30000, REGION_CPU1, 0 )	/* 64k for code + 128k for the banked ROMs images */
	ROM_LOAD( "1943kai.01",   0x00000, 0x08000, CRC(7d2211db) SHA1(b02a0b3daf7e1e224b7cad8fbe93439bd5ec9f0b) )
	ROM_LOAD( "1943kai.02",   0x10000, 0x10000, CRC(2ebbc8c5) SHA1(3be5ad061411642723e3f2bcb7b3c3caa11ee15f) )
	ROM_LOAD( "1943kai.03",   0x20000, 0x10000, CRC(475a6ac5) SHA1(fa07a855ba9173b6f81641c806ec7d938b0c282e) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for the audio CPU */
	ROM_LOAD( "1943kai.05",   0x00000, 0x8000, CRC(25f37957) SHA1(1e50c2a920eb3b5c881843686db857e9fee5ba1d) )

	ROM_REGION( 0x8000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "1943kai.04",   0x00000, 0x8000, CRC(884a8692) SHA1(027aa8c868dc07ccd9e27705031107881aef4b91) )	/* characters */

	ROM_REGION( 0x40000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "1943kai.15",   0x00000, 0x8000, CRC(6b1a0443) SHA1(32337c840ccd6815fd5844c194365c58d708f6dc) )	/* bg tiles */
	ROM_LOAD( "1943kai.16",   0x08000, 0x8000, CRC(9416fe0d) SHA1(92fbc8fffa4497747ab80abe20eef361f6525114) )
	ROM_LOAD( "1943kai.17",   0x10000, 0x8000, CRC(3d5acab9) SHA1(887d45b648fda952ae2137579f383ab8ede1facd) )
	ROM_LOAD( "1943kai.18",   0x18000, 0x8000, CRC(7b62da1d) SHA1(1926109a2ab2f550ca87b0d2af73abd2b4a7498d) )
	ROM_LOAD( "1943kai.19",   0x20000, 0x8000, CRC(868ababc) SHA1(1c7be905f53c63bad25fbbd9b3cf82d2c7749bc3) )
	ROM_LOAD( "1943kai.20",   0x28000, 0x8000, CRC(b90364c1) SHA1(104bc02237eeead84c7f35462186d0a1af8761bc) )
	ROM_LOAD( "1943kai.21",   0x30000, 0x8000, CRC(8c7fe74a) SHA1(8846b57d7f47c10ab1f505c359ecf36dcbacb011) )
	ROM_LOAD( "1943kai.22",   0x38000, 0x8000, CRC(d5ef8a0e) SHA1(2e42b1fbbfe823a33740a56d1334657db56d24d2) )

	ROM_REGION( 0x10000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "1943kai.24",   0x00000, 0x8000, CRC(bf186ef2) SHA1(cacbb8a61f8a64c3ba4ffde5ca6f07fe120b9a7e) )	/* fg tiles */
	ROM_LOAD( "1943kai.25",   0x08000, 0x8000, CRC(a755faf1) SHA1(8ee286d6ad7454ae34971f5891ddba4b76c244b0) )

	ROM_REGION( 0x40000, REGION_GFX4, ROMREGION_DISPOSE )
	ROM_LOAD( "1943kai.06",   0x00000, 0x8000, CRC(5f7e38b3) SHA1(33f69ebe91a0ee45d9107171fed26da475aaab3a) )	/* sprites */
	ROM_LOAD( "1943kai.07",   0x08000, 0x8000, CRC(ff3751fd) SHA1(bc942ddd46e7b147115e8ac22d24c2d018a7c373) )
	ROM_LOAD( "1943kai.08",   0x10000, 0x8000, CRC(159d51bd) SHA1(746aa49b18aff0eaf2fb875c573d455416d45a1d) )
	ROM_LOAD( "1943kai.09",   0x18000, 0x8000, CRC(8683e3d2) SHA1(591dc4811b226fe11cd5441ecb51aa3e95e68ac5) )
	ROM_LOAD( "1943kai.10",   0x20000, 0x8000, CRC(1e0d9571) SHA1(44ea9603020e9ab717e3e506f7ecf288506c0502) )
	ROM_LOAD( "1943kai.11",   0x28000, 0x8000, CRC(f1fc5ee1) SHA1(4ffc8e57734d3b59df695b86070511f1c447b992) )
	ROM_LOAD( "1943kai.12",   0x30000, 0x8000, CRC(0f50c001) SHA1(0e6367d3f0ba39a00ee0fa6e42ae9d43d12da23d) )
	ROM_LOAD( "1943kai.13",   0x38000, 0x8000, CRC(fd1acf8e) SHA1(88477ff1e5fbbca251d8cd4f241b42618ba64a80) )

	ROM_REGION( 0x10000, REGION_GFX5, 0 )	/* tilemaps */
	ROM_LOAD( "1943kai.14",   0x0000, 0x8000, CRC(cf0f5a53) SHA1(dc50f3f937f52910dbd0cedbc232acfed0aa6a42) )	/* front background */
	ROM_LOAD( "1943kai.23",   0x8000, 0x8000, CRC(17f77ef9) SHA1(8ebb4b440042436ec2db52bad808cced832db77c) )	/* back background */

	ROM_REGION( 0x0c00, REGION_PROMS, 0 )
	ROM_LOAD( "bmk01.bin",    0x0000, 0x0100, CRC(e001ea33) SHA1(4204bdf87820ac84bab2a1b5571a2ee28c4cdfc5) )	/* red component */
	ROM_LOAD( "bmk02.bin",    0x0100, 0x0100, CRC(af34d91a) SHA1(94bc6514c980fdd1cb013ff0819d6f32464c581c) )	/* green component */
	ROM_LOAD( "bmk03.bin",    0x0200, 0x0100, CRC(43e9f6ef) SHA1(e1f58368fe0bd9b53f6c286ce5009b218a5197dc) )	/* blue component */
	ROM_LOAD( "bmk05.bin",    0x0300, 0x0100, CRC(41878934) SHA1(8f28210ab1d409c89600169a136b74a706001cdf) )	/* char lookup table */
	ROM_LOAD( "bmk10.bin",    0x0400, 0x0100, CRC(de44b748) SHA1(0694fb19d98ccda728424436fc7350da7b5bd05e) )	/* foreground lookup table */
	ROM_LOAD( "bmk09.bin",    0x0500, 0x0100, CRC(59ea57c0) SHA1(f961c7e9981cc819c2adf4efdc977841d284a3a2) )	/* foreground palette bank */
	ROM_LOAD( "bmk12.bin",    0x0600, 0x0100, CRC(8765f8b0) SHA1(f32bab8e3587434b864fe97da9423f2335ccba2e) )	/* background lookup table */
	ROM_LOAD( "bmk11.bin",    0x0700, 0x0100, CRC(87a8854e) SHA1(0cbc601b736d566d625867d65e0f7b2abb535c65) )	/* background palette bank */
	ROM_LOAD( "bmk08.bin",    0x0800, 0x0100, CRC(dad17e2d) SHA1(fdb18ddc7574153bb7e27ba08b04b9dc87061c02) )	/* sprite lookup table */
	ROM_LOAD( "bmk07.bin",    0x0900, 0x0100, CRC(76307f8d) SHA1(8d655e2a5c50541795316d924b2f18b55f4b9571) )	/* sprite palette bank */
	ROM_LOAD( "bmprom.04",    0x0a00, 0x0100, CRC(91a8a2e1) SHA1(9583c87eff876f04bc2ccf7218cd8081f1bcdb94) )	/* priority encoder / palette selector (not used) */
	ROM_LOAD( "bmprom.06",    0x0b00, 0x0100, CRC(0eaf5158) SHA1(bafd4108708f66cd7b280e47152b108f3e254fc9) )	/* video timing (not used) */
ROM_END



GAME( 1987, 1943,    0,    1943, 1943, 0, ROT270, "Capcom", "1943 - The Battle of Midway (US)" )
GAME( 1987, 1943j,   1943, 1943, 1943, 0, ROT270, "Capcom", "1943 - The Battle of Midway (Japan)" )
GAME( 1987, 1943mii, 0,    1943, 1943, 0, ROT270, "Capcom", "1943 - The Battle of Midway Mark II (US)" )
GAME( 1987, 1943kai, 0,    1943, 1943, 0, ROT270, "Capcom", "1943 Kai - Midway Kaisen" )
