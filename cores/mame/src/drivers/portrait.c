/*
Portraits 1983 (c) Olympia

Driver by Steve Ellenoff & Peo

*Very Preliminary*

Notes

- no sound
- no colors
- input (especially camera) may not be quite right
- scrolling isn't hooked up; the registers are used in an unusual way
- for some reason, the credit button needs to be tapped 2x to register!
- service switches do not react the way the manual says they should

RAM Location 9240: Controls what level you are on: 0-3 (for each scene)
**************************************************************************/

#include "driver.h"
#include "cpu/z80/z80.h"

extern data8_t *portrait_bgvideoram,*portrait_fgvideoram,*portrait_spriteram;

int portrait_scrollx_hi, portrait_scrollx_lo;

PALETTE_INIT( portrait );
VIDEO_START( portrait );
VIDEO_UPDATE( portrait );
WRITE_HANDLER( portrait_bgvideo_write );
WRITE_HANDLER( portrait_fgvideo_write );

static struct GfxLayout tile_layout =
{
	16,16, /* tile width, height   */
	1024,  /* number of characters  */
	3,     /* bits per pixel */
	{ 0, 0x4000*8, 0x8000*8 }, /* bitplane offsets */
	{
		RGN_FRAC(1,2)+7,
		RGN_FRAC(1,2)+6,
		RGN_FRAC(1,2)+5,
		RGN_FRAC(1,2)+4,
		RGN_FRAC(1,2)+3,
		RGN_FRAC(1,2)+2,
		RGN_FRAC(1,2)+1,
		RGN_FRAC(1,2)+0,
		0, 1, 2, 3, 4, 5, 6, 7
	},
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8, 8*8, 8*9, 8*10, 8*11, 8*12, 8*13, 8*14, 8*15 },
	8*16 /* character offset */
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0x00000, &tile_layout, 0, 32 },
	{ -1 } /* end of array */
};

static READ_HANDLER(a000_r)
{
	switch( offset )
	{
	case 0x00: /*Dipswitch 1*/
		return input_port_2_r(0)^0xff;

	case 0x04: /*Dipswitch 2*/
		return input_port_3_r(0)^0xff;

	/*Service Switches? Coin Inputs, Player 1 & 2 Start, and more?*/
	case 0x08:
		/* Prelim map for a008 I/O
		  Bit 0 = Coin 1
		  Bit 1 = Coin 2
		  Bit 2 = Coin 3
		  Bit 3 = TILT?
		  Bit 4 = Player 1 Start
		  Bit 5 = Player 2 Start
		  Bit 6 = Service Switch 1? (Inverted?) (If 0, then switch is on!)
		  Bit 7 = Service Switch 2? (Inverted?) (If 0, then switch is on!)
		*/
		return	((input_port_4_r(0)^0x03)<<6)|	/*Grab cab switches*/
				(input_port_0_r(0) & 0x3f);		/*Grab player inputs*/

	/*Player Inputs and Camera ready status? Note: it's inverted, but perhaps it works if I change to ACTIVE_LOW signal*/
	case 0x10:
		/* Prelim map for a010 I/O
		  Bit 0 = Joy Up
		  Bit 1 = Joy Down
		  Bit 2 = Joy Right
		  Bit 3 = Joy Left
		  Bit 4 = Button 1
		  Bit 5 = Unused??
		  Bit 6 = Camera status? Ready flag?
		  Bit 7 = Unused??
		*/
		return input_port_1_r(0)^0xff;

	case 0x18:
		return portrait_scrollx_hi;
	case 0x19:
		return portrait_scrollx_lo;

	default:
		return 0x00;
	}
}

static WRITE_HANDLER(a000_w)
{
	switch( offset )
	{
	case 0x00: /* sound command? */
		return;

	case 0x08: /* coin counter */
		return;

	case 0x10: /* DAC? */
		/*DAC_0_data_w( 0, data );*/
		return;

	case 0x18:
		portrait_scrollx_hi = data;
		return;

	case 0x19:
		portrait_scrollx_lo = data;
		return;

	default:
		break;
	}
}

static MEMORY_READ_START( readmem )
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0x87ff, MRA_RAM },
	{ 0x8800, 0x8fff, MRA_RAM },
	{ 0x9000, 0x91ff, MRA_RAM },
	{ 0x9200, 0x97ff, MRA_RAM },
	{ 0xa000, 0xafff, a000_r },
	{ 0xffff, 0xffff, MRA_RAM }, /* unknown */
MEMORY_END


static MEMORY_WRITE_START( writemem )
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0x8000, 0x87ff, portrait_bgvideo_write, &portrait_bgvideoram },
	{ 0x8800, 0x8fff, portrait_fgvideo_write, &portrait_fgvideoram },
	{ 0x9000, 0x91ff, MWA_RAM, &portrait_spriteram },
	{ 0x9200, 0x97ff, MWA_RAM },
	{ 0xa000, 0xafff, a000_w },
	{ 0xb000, 0xbfff, MWA_RAM }, /* unknown */
MEMORY_END

INPUT_PORTS_START( portrait )
	PORT_START		/* IN 0 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_SERVICE1 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_TILT )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_START2 )

	PORT_START      /* IN 1 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP    | IPF_4WAY )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN  | IPF_4WAY )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_4WAY )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_4WAY )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_BUTTON2 )	/*camera status ready flag?*/
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START       /* DSW 1 */				/*Most (but not all) verified and correct!*/
	PORT_DIPNAME( 0x0f, 0x07, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x07, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x05, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 1C_7C ) )
	PORT_DIPSETTING(    0x01, "1 Coin / 10 Credits" )
	PORT_DIPSETTING(    0x00, "1 Coin / 12 Credits" )
	PORT_DIPSETTING(    0x0f, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x0b, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x0a, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x09, DEF_STR( 2C_5C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 2C_7C ) )
	PORT_DIPSETTING(    0x0e, "3 Coins / 5 Credits" )
	PORT_DIPSETTING(    0x0d, "3 Coins / 7 Credits" )
	PORT_DIPSETTING(    0x0c, "3 Coins / 10 Credits" )
	PORT_DIPNAME( 0x70, 0x20, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0x00, "1 Coin / 10 Credits" )
	PORT_DIPSETTING(    0x70, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x60, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x50, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 2C_5C ) )
	PORT_DIPNAME( 0x80, 0x80, "Collision during photo" )
	PORT_DIPSETTING(    0x80, "Easy" )
	PORT_DIPSETTING(    0x00, "Difficult" )

	PORT_START      /* DSW 2 */						/*Verified to be correct (from attract mode at least)*/
	PORT_DIPNAME( 0x01, 0x01, "Game Play" )
	PORT_DIPSETTING(    0x01, "Normal Play" )
	PORT_DIPSETTING(    0x00, "Freeplay (255 Cameras)" )
	PORT_DIPNAME( 0x02, 0x00, "High Score" )
	PORT_DIPSETTING(    0x02, "11.350 Points" )
	PORT_DIPSETTING(    0x00, "1.350 Points" )
	PORT_DIPNAME( 0x0c, 0x00, "Mistakes Allowed" )
	PORT_DIPSETTING(    0x0c, "2" )
	PORT_DIPSETTING(    0x08, "3" )
	PORT_DIPSETTING(    0x04, "4" )
	PORT_DIPSETTING(    0x00, "5" )
	PORT_DIPNAME( 0x30, 0x00, "Extra Camera" )
	PORT_DIPSETTING(    0x30, "No Action" )
	PORT_DIPSETTING(    0x20, "10.000 Points" )
	PORT_DIPSETTING(    0x10, "20.000 Points" )
	PORT_DIPSETTING(    0x00, "30.000 Points" )
	PORT_DIPNAME( 0x40, 0x40, "Slow Ostrich" )
	PORT_DIPSETTING(    0x40, "Easy Game" )
	PORT_DIPSETTING(    0x00, "Difficult" )
	PORT_DIPNAME( 0x80, 0x80, "Obstacles" )
	PORT_DIPSETTING(    0x00, "Easy Game" )
	PORT_DIPSETTING(    0x80, "Difficult" )

	/*Make the cabinet service switches fake dips*/
	PORT_START      /* DSW 3 */
	PORT_DIPNAME( 0x01, 0x00, "Service Switch 1" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x01, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x00, "Service Switch 2" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x02, DEF_STR( On ) )
INPUT_PORTS_END

static struct DACinterface dac_interface =
{
	1,
	{ 100 }
};

static MACHINE_DRIVER_START( portrait )
	MDRV_CPU_ADD(Z80, 4000000)     /* 4 MHz ? */
	MDRV_CPU_MEMORY(readmem,writemem)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,1)

	MDRV_FRAMES_PER_SECOND(50)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)

	MDRV_SCREEN_SIZE(64*8, 64*8)
	MDRV_VISIBLE_AREA(6*8, 54*8-1, 0*8, 40*8-1)

	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(256)

	MDRV_PALETTE_INIT(portrait)

	MDRV_VIDEO_START(portrait)
	MDRV_VIDEO_UPDATE(portrait)

	/* sound hardware */
	MDRV_SOUND_ADD(DAC, dac_interface) /* ? */
MACHINE_DRIVER_END


ROM_START( portrait )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )     /* 64k for the cpu */
	ROM_LOAD( "prt-p0.bin",  0x0000, 0x2000, CRC(a21874fa) SHA1(3db863f465a35d7d14dd71b47aa7dfe7b39fccf0) )
	ROM_LOAD( "prt-p1.bin",  0x2000, 0x2000, CRC(4d4d7793) SHA1(f828950ebbf285fc92c65f24421a20ceacef1cb9) )
	ROM_LOAD( "prt-p2.bin",  0x4000, 0x2000, CRC(83d88c9c) SHA1(c876f72b66537a49620fa27a5cb8a4aecd378f0a) )
	ROM_LOAD( "prt-p3.bin",  0x6000, 0x2000, CRC(bd32d007) SHA1(cdf814b00c22f9a4503fa54d43fb5781251b67a7) )

	ROM_REGION( 0x20000, REGION_GFX1, ROMREGION_DISPOSE | ROMREGION_INVERT )
	ROM_LOAD( "prt-00.bin",    0x00000, 0x2000, CRC(eb3e1c12) SHA1(2d38b66f52546b40553244c8a5c961279559f5b6) )	/*bit plane 1*/
	ROM_LOAD( "prt-10.bin",    0x02000, 0x2000, CRC(0f44e377) SHA1(1955f9f4deab2166f637f43c1f326bd65fc90f6a) )	/*bit plane 1*/
	ROM_LOAD( "prt-02.bin",    0x04000, 0x2000, CRC(bd93a3f9) SHA1(9cb479b8840cafd6043ff0cb9d5ca031dcd332ba) )	/*bit plane 2*/
	ROM_LOAD( "prt-12.bin",    0x06000, 0x2000, CRC(656b9f20) SHA1(c1907aba3d19be79d92cd73784b8e7ae94910da6) )	/*bit plane 2*/
	ROM_LOAD( "prt-04.bin",    0x08000, 0x2000, CRC(2a99feb5) SHA1(b373d2a2bd28aad6dd7a15a2166e03a8b7a34d9b) )	/*bit plane 3*/
	ROM_LOAD( "prt-14.bin",    0x0a000, 0x2000, CRC(224b7a58) SHA1(b84e70d22d1cab41e5773fc9daa2e4e55ec9d96e) )	/*bit plane 3*/

	ROM_LOAD( "prt-01.bin",    0x10000, 0x2000, CRC(70d27508) SHA1(d011f85b31bb3aa6f386e8e0edb91df10f4c4eb6) )	/*bit plane 1*/
	ROM_LOAD( "prt-11.bin",    0x12000, 0x2000, CRC(f498e395) SHA1(beb1d12433a350e5b773126de3f2803a9f5620c1) )	/*bit plane 1*/
	ROM_LOAD( "prt-03.bin",    0x14000, 0x2000, CRC(03d4153a) SHA1(7ce69ce6a101870dbfca1a9787fb1e660024bc02) )	/*bit plane 2*/
	ROM_LOAD( "prt-13.bin",    0x16000, 0x2000, CRC(10fa22b8) SHA1(e8f4c24fcdda0ce5e33bc600acd574a232a9bb21) )	/*bit plane 2*/
	ROM_LOAD( "prt-05.bin",    0x18000, 0x2000, CRC(43ea7951) SHA1(df0ae7fa802365979514063e1d67cdd45ecada90) )	/*bit plane 3*/
	ROM_LOAD( "prt-15.bin",    0x1a000, 0x2000, CRC(ab20b438) SHA1(ea5d60f6a9f06397bd0c6ee028b463c684090c01) )	/*bit plane 3*/

	/* proms? */
ROM_END

GAMEX( 1983, portrait,  0,    portrait, portrait,  0, ROT270, "Olympia", "Portraits", GAME_NO_SOUND | GAME_IMPERFECT_GRAPHICS | GAME_WRONG_COLORS | GAME_NOT_WORKING )
