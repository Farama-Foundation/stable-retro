/***************************************************************************

	Sega Pengo

    driver by Nicola Salmoria

    Games supported:
		* Pengo

	Known issues:
		* none

****************************************************************************

	Pengo memory map (preliminary)

	driver by Nicola Salmoria

	0000-7fff ROM
	8000-83ff Video RAM
	8400-87ff Color RAM
	8800-8fff RAM

	memory mapped ports:

	read:
	9000      DSW1
	9040      DSW0
	9080      IN1
	90c0      IN0

	write:
	8ff2-8ffd 6 pairs of two bytes:
	          the first byte contains the sprite image number (bits 2-7), Y flip (bit 0),
			  X flip (bit 1); the second byte the color
	9005      sound voice 1 waveform (nibble)
	9011-9013 sound voice 1 frequency (nibble)
	9015      sound voice 1 volume (nibble)
	900a      sound voice 2 waveform (nibble)
	9016-9018 sound voice 2 frequency (nibble)
	901a      sound voice 2 volume (nibble)
	900f      sound voice 3 waveform (nibble)
	901b-901d sound voice 3 frequency (nibble)
	901f      sound voice 3 volume (nibble)
	9022-902d Sprite coordinates, x/y pairs for 6 sprites
	9040      interrupt enable
	9041      sound enable
	9042      palette bank selector
	9043      flip screen
	9044-9045 coin counters
	9046      color lookup table bank selector
	9047      character/sprite bank selector
	9070      watchdog reset

	Main clock: XTAL = 18.432 MHz
	Z80 Clock: XTAL/6 = 3.072 MHz
	Horizontal video frequency: HSYNC = XTAL/3/192/2 = 16 kHz
	Video frequency: VSYNC = HSYNC/132/2 = 60.606060 Hz
	VBlank duration: 1/VSYNC * (20/132) = 2500 us

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "machine/segacrpt.h"
#include "pacman.h"



/*************************************
 *
 *	Main CPU memory handlers
 *
 *************************************/

static MEMORY_READ_START( readmem )
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0x8fff, MRA_RAM },	/* video and color RAM, scratchpad RAM, sprite codes */
	{ 0x9000, 0x903f, input_port_3_r },	/* DSW1 */
	{ 0x9040, 0x907f, input_port_2_r },	/* DSW0 */
	{ 0x9080, 0x90bf, input_port_1_r },	/* IN1 */
	{ 0x90c0, 0x90ff, input_port_0_r },	/* IN0 */
MEMORY_END


static MEMORY_WRITE_START( writemem )
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0x8000, 0x83ff, pacman_videoram_w, &videoram, &videoram_size },
	{ 0x8400, 0x87ff, pacman_colorram_w, &colorram },
	{ 0x8800, 0x8fef, MWA_RAMROM },
	{ 0x8ff0, 0x8fff, MWA_RAM, &spriteram, &spriteram_size},
	{ 0x9000, 0x901f, pengo_sound_w, &pengo_soundregs },
	{ 0x9020, 0x902f, MWA_RAM, &spriteram_2 },
	{ 0x9040, 0x9040, interrupt_enable_w },
	{ 0x9041, 0x9041, pengo_sound_enable_w },
	{ 0x9042, 0x9042, MWA_NOP },
	{ 0x9043, 0x9043, pacman_flipscreen_w },
	{ 0x9044, 0x9046, MWA_NOP },
	{ 0x9047, 0x9047, pengo_gfxbank_w },
	{ 0x9070, 0x9070, MWA_NOP },
MEMORY_END



/*************************************
 *
 *	Port definitions
 *
 *************************************/

INPUT_PORTS_START( pengo )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_4WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_4WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_4WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY )
	/* the coin input must stay low for no less than 2 frames and no more */
	/* than 9 frames to pass the self test check. */
	/* Moreover, this way we avoid the game freezing until the user releases */
	/* the "coin" key. */
	PORT_BIT_IMPULSE( 0x10, IP_ACTIVE_LOW, IPT_COIN1, 2 )
	PORT_BIT_IMPULSE( 0x20, IP_ACTIVE_LOW, IPT_COIN2, 2 )
	/* Coin Aux doesn't need IMPULSE to pass the test, but it still needs it */
	/* to avoid the freeze. */
	PORT_BIT_IMPULSE( 0x40, IP_ACTIVE_LOW, IPT_COIN3, 2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON1 )

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY | IPF_COCKTAIL )
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_SERVICE, DEF_STR( Service_Mode ), KEYCODE_F2, IP_JOY_NONE )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )

	PORT_START	/* DSW0 */
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x00, "30000" )
	PORT_DIPSETTING(    0x01, "50000" )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x18, 0x10, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x18, "2" )
	PORT_DIPSETTING(    0x10, "3" )
	PORT_DIPSETTING(    0x08, "4" )
	PORT_DIPSETTING(    0x00, "5" )
	PORT_BITX(    0x20, 0x20, IPT_DIPSWITCH_NAME | IPF_CHEAT, "Rack Test", KEYCODE_F1, IP_JOY_NONE )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0xc0, 0x80, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0xc0, "Easy" )
	PORT_DIPSETTING(    0x80, "Medium" )
	PORT_DIPSETTING(    0x40, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )

	PORT_START	/* DSW1 */
	PORT_DIPNAME( 0x0f, 0x0c, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x09, "2 Coins/1 Credit 5/3" )
	PORT_DIPSETTING(    0x05, "2 Coins/1 Credit 4/3" )
	PORT_DIPSETTING(    0x0c, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x0d, "1 Coin/1 Credit 5/6" )
	PORT_DIPSETTING(    0x03, "1 Coin/1 Credit 4/5" )
	PORT_DIPSETTING(    0x0b, "1 Coin/1 Credit 2/3" )
	PORT_DIPSETTING(    0x02, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x07, "1 Coin/2 Credits 5/11" )
	PORT_DIPSETTING(    0x0f, "1 Coin/2 Credits 4/9" )
	PORT_DIPSETTING(    0x0a, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x0e, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 1C_6C ) )
	PORT_DIPNAME( 0xf0, 0xc0, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x90, "2 Coins/1 Credit 5/3" )
	PORT_DIPSETTING(    0x50, "2 Coins/1 Credit 4/3" )
	PORT_DIPSETTING(    0xc0, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0xd0, "1 Coin/1 Credit 5/6" )
	PORT_DIPSETTING(    0x30, "1 Coin/1 Credit 4/5" )
	PORT_DIPSETTING(    0xb0, "1 Coin/1 Credit 2/3" )
	PORT_DIPSETTING(    0x20, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x70, "1 Coin/2 Credits 5/11" )
	PORT_DIPSETTING(    0xf0, "1 Coin/2 Credits 4/9" )
	PORT_DIPSETTING(    0xa0, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x60, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0xe0, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 1C_6C ) )
INPUT_PORTS_END



/*************************************
 *
 *	Graphics layouts
 *
 *************************************/

static struct GfxLayout tilelayout =
{
	8,8,	/* 8*8 characters */
    RGN_FRAC(1,2),    /* 256 characters */
    2,  /* 2 bits per pixel */
    { 0, 4 },   /* the two bitplanes for 4 pixels are packed into one byte */
    { 8*8+0, 8*8+1, 8*8+2, 8*8+3, 0, 1, 2, 3 }, /* bits are packed in groups of four */
    { 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
    16*8    /* every char takes 16 bytes */
};


static struct GfxLayout spritelayout =
{
	16,16,	/* 16*16 sprites */
	RGN_FRAC(1,2),	/* 64 sprites */
	2,	/* 2 bits per pixel */
	{ 0, 4 },	/* the two bitplanes for 4 pixels are packed into one byte */
	{ 8*8, 8*8+1, 8*8+2, 8*8+3, 16*8+0, 16*8+1, 16*8+2, 16*8+3,
			24*8+0, 24*8+1, 24*8+2, 24*8+3, 0, 1, 2, 3 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
			32*8, 33*8, 34*8, 35*8, 36*8, 37*8, 38*8, 39*8 },
	64*8	/* every sprite takes 64 bytes */
};


static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0x0000, &tilelayout,   0, 128 },
	{ REGION_GFX1, 0x2000, &spritelayout, 0, 128 },
	{ -1 } /* end of array */
};



/*************************************
 *
 *	Sound interfaces
 *
 *************************************/

static struct namco_interface namco_interface =
{
	3072000/32,	/* sample rate */
	3,			/* number of voices */
	100,		/* playback volume */
	REGION_SOUND1	/* memory region */
};



/*************************************
 *
 *	Machine drivers
 *
 *************************************/

static MACHINE_DRIVER_START( pengo )

	/* basic machine hardware */
	MDRV_CPU_ADD(Z80, 3020000)
	/* The correct speed is 3.072 MHz, but 3.020 gives a more */
	/* accurate emulation speed (time for two attract mode */
	/* cycles after power up, until the high score list appears */
	/* for the second time: 3'39") */
	MDRV_CPU_MEMORY(readmem,writemem)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,1)

	MDRV_FRAMES_PER_SECOND(60.606060)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(36*8, 28*8)
	MDRV_VISIBLE_AREA(0*8, 36*8-1, 0*8, 28*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(32)
	MDRV_COLORTABLE_LENGTH(4*128)

	MDRV_PALETTE_INIT(pacman)
	MDRV_VIDEO_START(pengo)
	MDRV_VIDEO_UPDATE(pacman)

	/* sound hardware */
	MDRV_SOUND_ADD(NAMCO_15XX, namco_interface)
MACHINE_DRIVER_END



/*************************************
 *
 *	ROM definitions
 *
 *************************************/

ROM_START( pengo )
	ROM_REGION( 2*0x10000, REGION_CPU1, 0 ) /* 64k for code + 64k for decrypted opcodes */
	ROM_LOAD( "ep1689c.8",    0x0000, 0x1000, CRC(f37066a8) SHA1(0930de17a763a527057f60783a92662b09554426) )
	ROM_LOAD( "ep1690b.7",    0x1000, 0x1000, CRC(baf48143) SHA1(4c97529e61eeca5d94938b1dfbeac41bf8cbaf7d) )
	ROM_LOAD( "ep1691b.15",   0x2000, 0x1000, CRC(adf0eba0) SHA1(c8949fbdbfe5023ee17a789ef60205e834a76c81) )
	ROM_LOAD( "ep1692b.14",   0x3000, 0x1000, CRC(a086d60f) SHA1(7079769d14dfe3873ffe29623ba0a93413706c6d) )
	ROM_LOAD( "ep1693b.21",   0x4000, 0x1000, CRC(b72084ec) SHA1(c0508951c2ad8dc31481be8b3bfee2063e3fb0d7) )
	ROM_LOAD( "ep1694b.20",   0x5000, 0x1000, CRC(94194a89) SHA1(7b47aec61593efd758e2a031f72a854bb0ba8af1) )
	ROM_LOAD( "ep5118b.32",   0x6000, 0x1000, CRC(af7b12c4) SHA1(207ed466546f40ca60a38031b83aef61446902e2) )
	ROM_LOAD( "ep5119c.31",   0x7000, 0x1000, CRC(933950fe) SHA1(fec7236b3dee2ea6e39c68440a6d2d9e3f72675a) )

	ROM_REGION( 0x4000, REGION_GFX1, 0 )
	ROM_LOAD( "ep1640.92",    0x0000, 0x1000, CRC(d7eec6cd) SHA1(e542bcc28f292be9a0a29d949de726e0b55e654a) ) /* tiles (bank 1) */
	ROM_CONTINUE(             0x2000, 0x1000 ) /* sprites (bank 1)*/
	ROM_LOAD( "ep1695.105",   0x1000, 0x1000, CRC(5bfd26e9) SHA1(bdec535e486b43a8f5550334beff423eeace10b2) ) /* tiles (bank 2) */
	ROM_CONTINUE(             0x3000, 0x1000 ) /* sprites (bank 2) */

	ROM_REGION( 0x0420, REGION_PROMS, 0 )
	ROM_LOAD( "pr1633.78",    0x0000, 0x0020, CRC(3a5844ec) SHA1(680eab0e1204c9b74adc11588461651b474021bb) ) /* color palette */
	ROM_LOAD( "pr1634.88",    0x0020, 0x0400, CRC(766b139b) SHA1(3fcd66610fcaee814953a115bf5e04788923181f) ) /* color lookup */

	ROM_REGION( 0x0200, REGION_SOUND1, 0 )
	ROM_LOAD( "pr1635.51",    0x0000, 0x0100, CRC(c29dea27) SHA1(563c9770028fe39188e62630711589d6ed242a66) ) /* waveform */
	ROM_LOAD( "pr1636.70",    0x0100, 0x0100, CRC(77245b66) SHA1(0c4d0bee858b97632411c440bea6948a74759746) ) /* timing - not used */
ROM_END


ROM_START( pengo2 )
	ROM_REGION( 2*0x10000, REGION_CPU1, 0 ) /* 64k for code + 64k for decrypted opcodes */
	ROM_LOAD( "ic8.2",        0x0000, 0x1000, CRC(e4924b7b) SHA1(44297658af8f8c884eba02b792346c5008137dfe) )
	ROM_LOAD( "ic7.2",        0x1000, 0x1000, CRC(72e7775d) SHA1(49e04178ee171f727dd023c019395679cfbad452) )
	ROM_LOAD( "ic15.2",       0x2000, 0x1000, CRC(7410ef1e) SHA1(7ed8e16c6ce401904c0da9758e2a405d7b9b451b) )
	ROM_LOAD( "ic14.2",       0x3000, 0x1000, CRC(55b3f379) SHA1(bc244f97132f0514adb3d6ceda8afbd45c1c587a) )
	ROM_LOAD( "ep1693b.21",   0x4000, 0x1000, CRC(b72084ec) SHA1(c0508951c2ad8dc31481be8b3bfee2063e3fb0d7) )
	ROM_LOAD( "ic20.2",       0x5000, 0x1000, CRC(770570cf) SHA1(43ead8236f53d39041ffc21bdeef10b3a77ce7f2) )
	ROM_LOAD( "ep5118b.32",   0x6000, 0x1000, CRC(af7b12c4) SHA1(207ed466546f40ca60a38031b83aef61446902e2) )
	ROM_LOAD( "ic31.2",       0x7000, 0x1000, CRC(669555c1) SHA1(50d5cf8022af6d6bd022235ab06015cb5c8aa433) )

	ROM_REGION( 0x4000, REGION_GFX1, 0 )
	ROM_LOAD( "ep1640.92",    0x0000, 0x1000, CRC(d7eec6cd) SHA1(e542bcc28f292be9a0a29d949de726e0b55e654a) ) /* tiles (bank 1) */
	ROM_CONTINUE(             0x2000, 0x1000 ) /* sprites (bank 1) */
	ROM_LOAD( "ep1695.105",   0x1000, 0x1000, CRC(5bfd26e9) SHA1(bdec535e486b43a8f5550334beff423eeace10b2) ) /* tiles (bank 2) */
	ROM_CONTINUE(             0x3000, 0x1000 ) /* sprites (bank 2) */

	ROM_REGION( 0x0420, REGION_PROMS, 0 )
	ROM_LOAD( "pr1633.78",    0x0000, 0x0020, CRC(3a5844ec) SHA1(680eab0e1204c9b74adc11588461651b474021bb) ) /* color palette */
	ROM_LOAD( "pr1634.88",    0x0020, 0x0400, CRC(766b139b) SHA1(3fcd66610fcaee814953a115bf5e04788923181f) ) /* color lookup */

	ROM_REGION( 0x0200, REGION_SOUND1, 0 )
	ROM_LOAD( "pr1635.51",    0x0000, 0x0100, CRC(c29dea27) SHA1(563c9770028fe39188e62630711589d6ed242a66) ) /* waveform */
	ROM_LOAD( "pr1636.70",    0x0100, 0x0100, CRC(77245b66) SHA1(0c4d0bee858b97632411c440bea6948a74759746) ) /* timing - not used */
ROM_END


ROM_START( pengo2u )
	ROM_REGION( 0x10000, REGION_CPU1, 0 ) /* 64k for code */
	ROM_LOAD( "pengo.u8",     0x0000, 0x1000, CRC(3dfeb20e) SHA1(a387b72501da77bf38b58619d2099083a0463e1f) )
	ROM_LOAD( "pengo.u7",     0x1000, 0x1000, CRC(1db341bd) SHA1(d1c66bb9cf479e6960dbcd35c820097a81eaa555) )
	ROM_LOAD( "pengo.u15",    0x2000, 0x1000, CRC(7c2842d5) SHA1(a8a568da68babd0ccb9f2cee4182fc01c3138494) )
	ROM_LOAD( "pengo.u14",    0x3000, 0x1000, CRC(6e3c1f2f) SHA1(2ee821b0f6e0f3cfeae7f5ff25a6e9bd977efce0) )
	ROM_LOAD( "ep5124.21",    0x4000, 0x1000, CRC(95f354ff) SHA1(fdebc68a6d87f8ecdf52a57a34ae5ae844a13510) )
	ROM_LOAD( "pengo.u20",    0x5000, 0x1000, CRC(0fdb04b8) SHA1(ed814d58318c1055e475ff678609d189727bf9b4) )
	ROM_LOAD( "ep5126.32",    0x6000, 0x1000, CRC(e5920728) SHA1(0ac5ffdad7bdcb32e630b9582e1b1aaece5198c9) )
	ROM_LOAD( "pengo.u31",    0x7000, 0x1000, CRC(13de47ed) SHA1(332b484d47c9921ed93432755bb2d7a9d4628939) )

	ROM_REGION( 0x4000, REGION_GFX1, 0 )
	ROM_LOAD( "ep1640.92",    0x0000, 0x1000, CRC(d7eec6cd) SHA1(e542bcc28f292be9a0a29d949de726e0b55e654a) ) /* tiles (bank 1) */
	ROM_CONTINUE(             0x2000, 0x1000 ) /* sprites (bank 1) */
	ROM_LOAD( "ep1695.105",   0x1000, 0x1000, CRC(5bfd26e9) SHA1(bdec535e486b43a8f5550334beff423eeace10b2) ) /* tiles (bank 2) */
	ROM_CONTINUE(             0x3000, 0x1000 ) /* sprites (bank 2) */

	ROM_REGION( 0x0420, REGION_PROMS, 0 )
	ROM_LOAD( "pr1633.78",    0x0000, 0x0020, CRC(3a5844ec) SHA1(680eab0e1204c9b74adc11588461651b474021bb) ) /* color palette */
	ROM_LOAD( "pr1634.88",    0x0020, 0x0400, CRC(766b139b) SHA1(3fcd66610fcaee814953a115bf5e04788923181f) ) /* color lookup */

	ROM_REGION( 0x0200, REGION_SOUND1, 0 )
	ROM_LOAD( "pr1635.51",    0x0000, 0x0100, CRC(c29dea27) SHA1(563c9770028fe39188e62630711589d6ed242a66) ) /* waveform */
	ROM_LOAD( "pr1636.70",    0x0100, 0x0100, CRC(77245b66) SHA1(0c4d0bee858b97632411c440bea6948a74759746) ) /* timing - not used */
ROM_END


ROM_START( pengo3u )
	ROM_REGION( 0x10000, REGION_CPU1, 0 ) /* 64k for code */
	ROM_LOAD( "ep5120.8",     0x0000, 0x1000, CRC(f01afb60) SHA1(1db732a17a9f79f8f1751f80c77889142928e41b) )
	ROM_LOAD( "ep5121.7",     0x1000, 0x1000, CRC(2eb38353) SHA1(d351347f93a3ed01c8b5274ec19352dd611a8dd4) )
	ROM_LOAD( "ep5122.15",    0x2000, 0x1000, CRC(c33400d7) SHA1(7b9617d22a9de8d3658abe34b5d2171ce37acc39) )
	ROM_LOAD( "ep5123.14",    0x3000, 0x1000, CRC(6a85c6a2) SHA1(444acc08607c892bb20b3a02753169addf5b11de) )
	ROM_LOAD( "ep5124.21",    0x4000, 0x1000, CRC(95f354ff) SHA1(fdebc68a6d87f8ecdf52a57a34ae5ae844a13510) )
	ROM_LOAD( "ep5125.20",    0x5000, 0x1000, CRC(1a42310f) SHA1(fef20385299a709ee17ed16510ac5702bd5cc257) )
	ROM_LOAD( "ep5126.32",    0x6000, 0x1000, CRC(e5920728) SHA1(0ac5ffdad7bdcb32e630b9582e1b1aaece5198c9) )
	ROM_LOAD( "ep5127.31",    0x7000, 0x1000, CRC(a7d3d1d6) SHA1(20e4353208c3803d8879b25f821ea617e9a19cc4) )

	ROM_REGION( 0x4000, REGION_GFX1, 0 )
	ROM_LOAD( "ep1640.92",    0x0000, 0x1000, CRC(d7eec6cd) SHA1(e542bcc28f292be9a0a29d949de726e0b55e654a) ) /* tiles (bank 1) */
	ROM_CONTINUE(             0x2000, 0x1000 ) /* sprites (bank 1) */
	ROM_LOAD( "ep1695.105",   0x1000, 0x1000, CRC(5bfd26e9) SHA1(bdec535e486b43a8f5550334beff423eeace10b2) ) /* tiles (bank 2) */
	ROM_CONTINUE(             0x3000, 0x1000 ) /* sprites (bank 2) */

	ROM_REGION( 0x0420, REGION_PROMS, 0 )
	ROM_LOAD( "pr1633.78",    0x0000, 0x0020, CRC(3a5844ec) SHA1(680eab0e1204c9b74adc11588461651b474021bb) ) /* color palette */
	ROM_LOAD( "pr1634.88",    0x0020, 0x0400, CRC(766b139b) SHA1(3fcd66610fcaee814953a115bf5e04788923181f) ) /* color lookup */

	ROM_REGION( 0x0200, REGION_SOUND1, 0 )
	ROM_LOAD( "pr1635.51",    0x0000, 0x0100, CRC(c29dea27) SHA1(563c9770028fe39188e62630711589d6ed242a66) ) /* waveform */
	ROM_LOAD( "pr1636.70",    0x0100, 0x0100, CRC(77245b66) SHA1(0c4d0bee858b97632411c440bea6948a74759746) ) /* timing - not used */
ROM_END


ROM_START( pengob )
	ROM_REGION( 2*0x10000, REGION_CPU1, 0 ) /* 64k for code + 64k for decrypted opcodes */
	ROM_LOAD( "1",            0x0000, 0x2000, CRC(e04064db) SHA1(6689066b443807646894a357317f468bfc92368a) )
	ROM_LOAD( "2",            0x2000, 0x2000, CRC(75752424) SHA1(634e696a692c7245dfa5c5dfd4ce87755c2a90d4) )
	ROM_LOAD( "021_pn03.bin", 0x4000, 0x1000, CRC(7824e3ef) SHA1(3395bb537614de8da763d05f0d2e312145017e8f) ) /* 3 (1/2) */
	ROM_LOAD( "020_pn07.bin", 0x5000, 0x1000, CRC(377b9663) SHA1(35327dc0f0c19fa5a863aaf8d8f3bfcd2a5717a9) ) /* 3 (2/2) */
	ROM_LOAD( "032_pn04.bin", 0x6000, 0x1000, CRC(bfde44c1) SHA1(97e8a360ce09faa36d864d7020b1669a349867c6) ) /* 4 (1/2) */
	ROM_LOAD( "031_pn08.bin", 0x7000, 0x1000, CRC(64e8c30d) SHA1(aa50c21db2ac8361fc575f0785e2aae57f338564) ) /* 4 (2/2) */

	ROM_REGION( 0x4000, REGION_GFX1, 0 )
	ROM_LOAD( "5",            0x0000, 0x1000, CRC(1232437b) SHA1(7ec410a2a802514449ccb05684762c25f29556b0) ) /* tiles (bank 1) */
	ROM_CONTINUE(             0x2000, 0x1000 ) /* sprites (bank 1) */
	ROM_LOAD( "ep1695.105",   0x1000, 0x1000, CRC(5bfd26e9) SHA1(bdec535e486b43a8f5550334beff423eeace10b2) ) /* tiles (bank 2) */
	ROM_CONTINUE(             0x3000, 0x1000 ) /* sprites (bank 2) */

	ROM_REGION( 0x0420, REGION_PROMS, 0 )
	ROM_LOAD( "pr1633.78",    0x0000, 0x0020, CRC(3a5844ec) SHA1(680eab0e1204c9b74adc11588461651b474021bb) ) /* color palette */
	ROM_LOAD( "pr1634.88",    0x0020, 0x0400, CRC(766b139b) SHA1(3fcd66610fcaee814953a115bf5e04788923181f) ) /* color lookup */

	ROM_REGION( 0x0200, REGION_SOUND1, 0 )
	ROM_LOAD( "pr1635.51",    0x0000, 0x0100, CRC(c29dea27) SHA1(563c9770028fe39188e62630711589d6ed242a66) ) /* waveform */
	ROM_LOAD( "pr1636.70",    0x0100, 0x0100, CRC(77245b66) SHA1(0c4d0bee858b97632411c440bea6948a74759746) ) /* timing - not used */
ROM_END


ROM_START( penta )
	ROM_REGION( 2*0x10000, REGION_CPU1, 0 )     /* 64k for code + 64k for decrypted opcodes */
	ROM_LOAD( "008_pn01.bin", 0x0000, 0x1000, CRC(22f328df) SHA1(ba13b8d20ccde995a158cf62b4bc48cb369a0788) )
	ROM_LOAD( "007_pn05.bin", 0x1000, 0x1000, CRC(15bbc7d3) SHA1(1823e3ba7388d3f4d9262e9b9cf70f123862c546) )
	ROM_LOAD( "015_pn02.bin", 0x2000, 0x1000, CRC(de82b74a) SHA1(301c1223dd0b111f8439affcb96b6e29106364ed) )
	ROM_LOAD( "014_pn06.bin", 0x3000, 0x1000, CRC(160f3836) SHA1(ff90c82d52ed0c2c17a7aeabc9401ee9d8cf4d2d) )
	ROM_LOAD( "021_pn03.bin", 0x4000, 0x1000, CRC(7824e3ef) SHA1(3395bb537614de8da763d05f0d2e312145017e8f) )
	ROM_LOAD( "020_pn07.bin", 0x5000, 0x1000, CRC(377b9663) SHA1(35327dc0f0c19fa5a863aaf8d8f3bfcd2a5717a9) )
	ROM_LOAD( "032_pn04.bin", 0x6000, 0x1000, CRC(bfde44c1) SHA1(97e8a360ce09faa36d864d7020b1669a349867c6) )
	ROM_LOAD( "031_pn08.bin", 0x7000, 0x1000, CRC(64e8c30d) SHA1(aa50c21db2ac8361fc575f0785e2aae57f338564) )

	ROM_REGION( 0x4000, REGION_GFX1, 0 )
	ROM_LOAD( "092_pn09.bin", 0x0000, 0x1000, CRC(6afeba9d) SHA1(cd723fb94aa90dbaac9a6fe085c0f4786d2fa092) ) /* tiles (bank 1) */
	ROM_CONTINUE(             0x2000, 0x1000 ) /* sprites (bank 1) */
	ROM_LOAD( "ep1695.105",   0x1000, 0x1000, CRC(5bfd26e9) SHA1(bdec535e486b43a8f5550334beff423eeace10b2) ) /* tiles (bank 2) */
	ROM_CONTINUE(             0x3000, 0x1000 ) /* sprites (bank 2) */

	ROM_REGION( 0x0420, REGION_PROMS, 0 )
	ROM_LOAD( "pr1633.78",    0x0000, 0x0020, CRC(3a5844ec) SHA1(680eab0e1204c9b74adc11588461651b474021bb) ) /* color palette */
	ROM_LOAD( "pr1634.88",    0x0020, 0x0400, CRC(766b139b) SHA1(3fcd66610fcaee814953a115bf5e04788923181f) ) /* color lookup */

	ROM_REGION( 0x0200, REGION_SOUND1, 0 )
	ROM_LOAD( "pr1635.51",    0x0000, 0x0100, CRC(c29dea27) SHA1(563c9770028fe39188e62630711589d6ed242a66) ) /* waveform */
	ROM_LOAD( "pr1636.70",    0x0100, 0x0100, CRC(77245b66) SHA1(0c4d0bee858b97632411c440bea6948a74759746) ) /* timing - not used */
ROM_END



/*************************************
 *
 *	Driver initialization
 *
 *************************************/

static DRIVER_INIT( pengo )
{
	pengo_decode();
}


static DRIVER_INIT( penta )
{
/*
	the values vary, but the translation mask is always laid out like this:

	  0 1 2 3 4 5 6 7 8 9 a b c d e f
	0 A A B B A A B B C C D D C C D D
	1 A A B B A A B B C C D D C C D D
	2 E E F F E E F F G G H H G G H H
	3 E E F F E E F F G G H H G G H H
	4 A A B B A A B B C C D D C C D D
	5 A A B B A A B B C C D D C C D D
	6 E E F F E E F F G G H H G G H H
	7 E E F F E E F F G G H H G G H H
	8 H H G G H H G G F F E E F F E E
	9 H H G G H H G G F F E E F F E E
	a D D C C D D C C B B A A B B A A
	b D D C C D D C C B B A A B B A A
	c H H G G H H G G F F E E F F E E
	d H H G G H H G G F F E E F F E E
	e D D C C D D C C B B A A B B A A
	f D D C C D D C C B B A A B B A A

	(e.g. 0xc0 is XORed with H)
	therefore in the following tables we only keep track of A, B, C, D, E, F, G and H.
*/
	static const unsigned char data_xortable[2][8] =
	{
		{ 0xa0,0x82,0x28,0x0a,0x82,0xa0,0x0a,0x28 },	/* ...............0 */
		{ 0x88,0x0a,0x82,0x00,0x88,0x0a,0x82,0x00 }		/* ...............1 */
	};
	static const unsigned char opcode_xortable[8][8] =
	{
		{ 0x02,0x08,0x2a,0x20,0x20,0x2a,0x08,0x02 },	/* ...0...0...0.... */
		{ 0x88,0x88,0x00,0x00,0x88,0x88,0x00,0x00 },	/* ...0...0...1.... */
		{ 0x88,0x0a,0x82,0x00,0xa0,0x22,0xaa,0x28 },	/* ...0...1...0.... */
		{ 0x88,0x0a,0x82,0x00,0xa0,0x22,0xaa,0x28 },	/* ...0...1...1.... */
		{ 0x2a,0x08,0x2a,0x08,0x8a,0xa8,0x8a,0xa8 },	/* ...1...0...0.... */
		{ 0x2a,0x08,0x2a,0x08,0x8a,0xa8,0x8a,0xa8 },	/* ...1...0...1.... */
		{ 0x88,0x0a,0x82,0x00,0xa0,0x22,0xaa,0x28 },	/* ...1...1...0.... */
		{ 0x88,0x0a,0x82,0x00,0xa0,0x22,0xaa,0x28 }		/* ...1...1...1.... */
	};
	int A;
	unsigned char *rom = memory_region(REGION_CPU1);
	int diff = memory_region_length(REGION_CPU1) / 2;


	memory_set_opcode_base(0,rom+diff);

	for (A = 0x0000;A < 0x8000;A++)
	{
		int i,j;
		unsigned char src;


		src = rom[A];

		/* pick the translation table from bit 0 of the address */
		i = A & 1;

		/* pick the offset in the table from bits 1, 3 and 5 of the source data */
		j = ((src >> 1) & 1) + (((src >> 3) & 1) << 1) + (((src >> 5) & 1) << 2);
		/* the bottom half of the translation table is the mirror image of the top */
		if (src & 0x80) j = 7 - j;

		/* decode the ROM data */
		rom[A] = src ^ data_xortable[i][j];

		/* now decode the opcodes */
		/* pick the translation table from bits 4, 8 and 12 of the address */
		i = ((A >> 4) & 1) + (((A >> 8) & 1) << 1) + (((A >> 12) & 1) << 2);
		rom[A + diff] = src ^ opcode_xortable[i][j];
	}
}



/*************************************
 *
 *	Game drivers
 *
 *************************************/

GAME( 1982, pengo,    0,     pengo, pengo, pengo,    ROT90, "Sega", "Pengo (set 1 rev c)" )
GAME( 1982, pengo2,   pengo, pengo, pengo, pengo,    ROT90, "Sega", "Pengo (set 2)" )
GAME( 1982, pengo2u,  pengo, pengo, pengo, 0,        ROT90, "Sega", "Pengo (set 2 not encrypted)" )
GAME( 1982, pengo3u,  pengo, pengo, pengo, 0,        ROT90, "Sega", "Pengo (set 3 not encrypted)" )
GAME( 1982, pengob,   pengo, pengo, pengo, penta,    ROT90, "bootleg", "Pengo (bootleg)" )
GAME( 1982, penta,    pengo, pengo, pengo, penta,    ROT90, "bootleg", "Penta" )
