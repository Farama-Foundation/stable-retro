/***************************************************************************

	Lemmings				(c) 1991 Data East USA (DE-0357)

	Prototype!  Licensed from the home computer version this game never
	made it past the arcade field test stage.  Unlike most Data East games
	this hardware features a pixel layer and a VRAM layer, probably to
	make the transition from the pixel addressable computer code to the
	arcade hardware.

	As prototype software it seems to have a couple of non-critical bugs,
	the palette ram check and vram check both overrun their actual ramsize.

	Emulation by Bryan McPhail, mish@tendril.co.uk

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "lemmings.h"

/******************************************************************************/

static WRITE16_HANDLER( lemmings_control_w )
{
	/* Offset==0 Pixel layer X scroll */
	if (offset==4) return; /* Watchdog or IRQ ack */
	COMBINE_DATA(&lemmings_control_data[offset]);
}

static READ16_HANDLER( lemmings_trackball_r )
{
	switch (offset) {
	case 0: return readinputport(4); break;
	case 1: return readinputport(5); break;
	case 4: return readinputport(6); break;
	case 5: return readinputport(7); break;
	}
	return 0;
}

/* Same as Robocop 2 protection chip */
static READ16_HANDLER( lemmings_prot_r )
{
 	switch (offset<<1) {
		case 0x41a: /* Player input */
			return readinputport(0);

		case 0x320: /* Coins */
			return readinputport(1);

		case 0x4e6: /* Dips */
			return (readinputport(2) + (readinputport(3) << 8));
	}

	return 0;
}

static WRITE16_HANDLER( lemmings_palette_24bit_w )
{
	int r,g,b;

	COMBINE_DATA(&paletteram16[offset]);
	if (offset&1) offset--;

	b = (paletteram16[offset] >> 0) & 0xff;
	g = (paletteram16[offset+1] >> 8) & 0xff;
	r = (paletteram16[offset+1] >> 0) & 0xff;

	palette_set_color(offset/2,r,g,b);
}

static WRITE16_HANDLER( lemmings_sound_w )
{
	soundlatch_w(0,data&0xff);
	cpu_set_irq_line(1,1,HOLD_LINE);
}

static WRITE_HANDLER( lemmings_sound_ack_w )
{
	cpu_set_irq_line(1,1,CLEAR_LINE);
}

/******************************************************************************/

static MEMORY_READ16_START( lemmings_readmem )
	{ 0x000000, 0x0fffff, MRA16_ROM },
	{ 0x100000, 0x10ffff, MRA16_RAM },
	{ 0x120000, 0x1207ff, MRA16_RAM },
	{ 0x140000, 0x1407ff, MRA16_RAM },
	{ 0x160000, 0x160fff, MRA16_RAM },
	{ 0x1a0000, 0x1a07ff, lemmings_prot_r },
	{ 0x190000, 0x19000f, lemmings_trackball_r },
	{ 0x200000, 0x202fff, MRA16_RAM },
	{ 0x300000, 0x37ffff, MRA16_RAM },
	{ 0x380000, 0x39ffff, MRA16_RAM },
MEMORY_END

static MEMORY_WRITE16_START( lemmings_writemem )
	{ 0x000000, 0x0fffff, MWA16_ROM },
	{ 0x100000, 0x10ffff, MWA16_RAM },
	{ 0x120000, 0x1207ff, MWA16_RAM, &spriteram16, &spriteram_size },
	{ 0x140000, 0x1407ff, MWA16_RAM, &spriteram16_2, &spriteram_2_size },
	{ 0x160000, 0x160fff, lemmings_palette_24bit_w, &paletteram16 },
	{ 0x170000, 0x17000f, lemmings_control_w, &lemmings_control_data },
	{ 0x1a0064, 0x1a0065, lemmings_sound_w },
	{ 0x1c0000, 0x1c0001, buffer_spriteram16_w }, /* 1 written once a frame */
	{ 0x1e0000, 0x1e0001, buffer_spriteram16_2_w }, /* 1 written once a frame */
	{ 0x200000, 0x201fff, lemmings_vram_w, &lemmings_vram_data },
	{ 0x300000, 0x37ffff, lemmings_pixel_0_w, &lemmings_pixel_0_data },
	{ 0x380000, 0x39ffff, lemmings_pixel_1_w, &lemmings_pixel_1_data },
MEMORY_END

/******************************************************************************/

static MEMORY_READ_START( sound_readmem )
	{ 0x0000, 0x07ff, MRA_RAM },
	{ 0x0801, 0x0801, YM2151_status_port_0_r },
	{ 0x1000, 0x1000, OKIM6295_status_0_r },
	{ 0x1800, 0x1800, soundlatch_r },
	{ 0x8000, 0xffff, MRA_ROM },
MEMORY_END

static MEMORY_WRITE_START( sound_writemem )
	{ 0x0000, 0x07ff, MWA_RAM },
	{ 0x0800, 0x0800, YM2151_register_port_0_w },
	{ 0x0801, 0x0801, YM2151_data_port_0_w },
	{ 0x1000, 0x1000, OKIM6295_data_0_w },
	{ 0x1800, 0x1800, lemmings_sound_ack_w },
	{ 0x8000, 0xffff, MWA_ROM },
MEMORY_END

/******************************************************************************/

INPUT_PORTS_START( lemmings )
	PORT_START	/* Player 1 controls */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_BUTTON1 ) /* Select 1 */
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_BUTTON2 ) /* Hurry 1 */
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 ) /* Select 2 */
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 ) /* Hurry 2 */
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* Credits */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BITX(0x0004, IP_ACTIVE_LOW, IPT_SERVICE, DEF_STR( Service_Mode ), KEYCODE_F2, IP_JOY_NONE )
	PORT_BIT( 0x0008, IP_ACTIVE_HIGH, IPT_VBLANK )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0xff00, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* Dip switch bank 1 */
	PORT_DIPNAME( 0x03, 0x03, "Credits for 1 Player" )
	PORT_DIPSETTING(    0x03, "1" )
	PORT_DIPSETTING(    0x02, "2" )
	PORT_DIPSETTING(    0x01, "3" )
	PORT_DIPSETTING(    0x00, "4" )
	PORT_DIPNAME( 0x0c, 0x0c, "Credits for 2 Player" )
	PORT_DIPSETTING(    0x0c, "1" )
	PORT_DIPSETTING(    0x08, "2" )
	PORT_DIPSETTING(    0x04, "3" )
	PORT_DIPSETTING(    0x00, "4" )
	PORT_DIPNAME( 0x30, 0x30, "Credits for Continue" )
	PORT_DIPSETTING(    0x30, "1" )
	PORT_DIPSETTING(    0x20, "2" )
	PORT_DIPSETTING(    0x10, "3" )
	PORT_DIPSETTING(    0x00, "4" )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Free_Play ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START	/* Dip switch bank 2 */
	PORT_DIPNAME( 0x07, 0x07, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x07, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x05, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 1C_7C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_8C ) )
	PORT_DIPNAME( 0x38, 0x38, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x38, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x28, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x18, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 1C_7C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_8C ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START
	PORT_ANALOG( 0xff, 0x00, IPT_TRACKBALL_X | IPF_PLAYER1 | IPF_REVERSE, 70, 10, 0, 0 )

	PORT_START
	PORT_ANALOG( 0xff, 0x00, IPT_TRACKBALL_Y | IPF_PLAYER1, 70, 10, 0, 0 )

	PORT_START
	PORT_ANALOG( 0xff, 0x00, IPT_TRACKBALL_X | IPF_PLAYER2 | IPF_REVERSE, 70, 10, 0, 0 )

	PORT_START
	PORT_ANALOG( 0xff, 0x00, IPT_TRACKBALL_Y | IPF_PLAYER2, 70, 10, 0, 0 )
INPUT_PORTS_END

/******************************************************************************/

static struct GfxLayout charlayout =
{
	8,8,
	2048,
	4,
	{ 4, 5, 6, 7 },
    { 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	{ 0*0, 1*64, 2*64, 3*64, 4*64, 5*64, 6*64, 7*64 },
	8*64
};

static struct GfxLayout sprite_layout =
{
	16,16,
	RGN_FRAC(1,4),
	4,
	{ 0x30000*8, 0x20000*8, 0x10000*8, 0x00000*8 },
	{
		7, 6, 5, 4, 3, 2, 1, 0, 16*8+7, 16*8+6, 16*8+5, 16*8+4, 16*8+3, 16*8+2, 16*8+1, 16*8+0
	},
	{ 15*8, 14*8, 13*8, 12*8, 11*8, 10*8, 9*8, 8*8, 7*8, 6*8, 5*8, 4*8, 3*8, 2*8, 1*8, 0*8 },
	32*8
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &sprite_layout,  32*16, 16 },	/* Sprites 16x16 */
	{ REGION_GFX2, 0, &sprite_layout,  48*16, 16 },	/* Sprites 16x16 */
	{ 0,           0, &charlayout,         0, 16 }, /* Dynamically modified */
	{ -1 } /* end of array */
};

/******************************************************************************/

static struct OKIM6295interface okim6295_interface =
{
	1,          /* 1 chip */
	{ 7757 },	/* Frequency */
	{ REGION_SOUND1 },	/* memory region */
	{ 50 }
};

static void sound_irq(int state)
{
	cpu_set_irq_line(1,0,state);
}

static struct YM2151interface ym2151_interface =
{
	1,
	32220000/9,
	{ YM3012_VOL(45,MIXER_PAN_LEFT,45,MIXER_PAN_RIGHT) },
	{ sound_irq }
};

static MACHINE_DRIVER_START( lemmings )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 14000000)
	MDRV_CPU_MEMORY(lemmings_readmem,lemmings_writemem)
	MDRV_CPU_VBLANK_INT(irq6_line_hold,1)

	MDRV_CPU_ADD(M6809,32220000/8)
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
	MDRV_CPU_MEMORY(sound_readmem,sound_writemem)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(529)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_NEEDS_6BITS_PER_GUN | VIDEO_BUFFERS_SPRITERAM)
	MDRV_SCREEN_SIZE(40*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 40*8-1, 2*8, 30*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(1024)

	MDRV_VIDEO_EOF(lemmings)
	MDRV_VIDEO_START(lemmings)
	MDRV_VIDEO_STOP(lemmings)
	MDRV_VIDEO_UPDATE(lemmings)

	/* sound hardware */
	MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
	MDRV_SOUND_ADD(YM2151, ym2151_interface)
	MDRV_SOUND_ADD(OKIM6295, okim6295_interface)
MACHINE_DRIVER_END

/******************************************************************************/

ROM_START( lemmings )
	ROM_REGION( 0x100000, REGION_CPU1, 0 ) /* 68000 code */
	ROM_LOAD16_BYTE( "lemmings.5", 0x00000, 0x20000, CRC(e9a2b439) SHA1(873723a06d71bb41772951f451a75578b30267d5) )
	ROM_LOAD16_BYTE( "lemmings.1", 0x00001, 0x20000, CRC(bf52293b) SHA1(47a1ed64bf02776db086fdce80997b8a0c068791) )
	ROM_LOAD16_BYTE( "lemmings.6", 0x40000, 0x20000, CRC(0e3dc0ea) SHA1(533abf66ca4b578d03566d5de922dc5828c26eca) )
	ROM_LOAD16_BYTE( "lemmings.2", 0x40001, 0x20000, CRC(0cf3d7ce) SHA1(95dc43a8cded860fcf8743b62cbe4f2a97f43215) )
	ROM_LOAD16_BYTE( "lemmings.7", 0x80000, 0x20000, CRC(d020219c) SHA1(9678d8636798d1e528269fe2f9eb532e189c134e) )
	ROM_LOAD16_BYTE( "lemmings.3", 0x80001, 0x20000, CRC(c635494a) SHA1(e105dc79bd3c425d971629a3066c38dbf08b6428) )
	ROM_LOAD16_BYTE( "lemmings.8", 0xc0000, 0x20000, CRC(9166ce09) SHA1(7f0970cc07ebdbfc9a738342259d07d37b397161) )
	ROM_LOAD16_BYTE( "lemmings.4", 0xc0001, 0x20000, CRC(aa845488) SHA1(d17ec80f43d2a0123e93fad83d4e1319eb18d7c7) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* Sound CPU */
	ROM_LOAD( "lemmings.15",    0x00000, 0x10000, CRC(f0b24a35) SHA1(1aaeb1e6faee04d2e433161fd7aa965b58e3b8a7) )

	ROM_REGION( 0x40000, REGION_GFX1, ROMREGION_DISPOSE )
  	ROM_LOAD( "lemmings.9",  0x000000, 0x10000, CRC(e06442f5) SHA1(d9c8b681cce1d0257a0446bc820c7d679e2a1168) )
	ROM_LOAD( "lemmings.10", 0x010000, 0x10000, CRC(36398848) SHA1(6c6956607f889c35367e6df4a32359042fad695e) )
  	ROM_LOAD( "lemmings.11", 0x020000, 0x10000, CRC(b46a54e5) SHA1(53b053346f80357aecff4ab888a8562f99cb318f) )
	ROM_FILL(                0x030000, 0x10000, 0 ) /* 3bpp data but sprite chip expects 4 */

	ROM_REGION( 0x40000, REGION_GFX2, ROMREGION_DISPOSE )
 	ROM_LOAD( "lemmings.12", 0x000000, 0x10000, CRC(dc9047ff) SHA1(1bbe573fa51127a9e8b970a353f3cceab00f486a) )
  	ROM_LOAD( "lemmings.13", 0x010000, 0x10000, CRC(7cc15491) SHA1(73c1c11b2738f6679c70cae8ac4c55cdc9b8fc27) )
	ROM_LOAD( "lemmings.14", 0x020000, 0x10000, CRC(c162788f) SHA1(e1f669efa59699cd1b7da71b112701ee79240c18) )
	ROM_FILL(                0x030000, 0x10000, 0 ) /* 3bpp data but sprite chip expects 4 */

	ROM_REGION( 0x20000, REGION_SOUND1, 0 )	/* ADPCM samples */
  	ROM_LOAD( "lemmings.16",    0x00000, 0x20000, CRC(f747847c) SHA1(00880fa6dff979e5d15daea61938bd18c768c92f) )
ROM_END

/******************************************************************************/

GAME( 1991, lemmings, 0, lemmings, lemmings, 0, ROT0, "Data East USA", "Lemmings (US Prototype)" )
