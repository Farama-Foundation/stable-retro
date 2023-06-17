/***************************************************************************

Championship Baseball

driver by Nicola Salmoria

TODO:
champbbj and champbb2 don't work due to protection - a custom mcu probably.
The protection involves locations a006-a007 and 6000-63ff.  It pulls
addresses to routines from there.

champbja is a patched version of champbbj with different protection.

0000-5fff ROM
7800-7fff ROM (Champion Baseball 2 only)
8000-83ff Video RAM
8400-87ff Color RAM
8800-8fff RAM

read:
a000      IN0
a040      IN1
a080      DSW
a0a0      ?
a0c0      COIN

write:
7000      8910 write
7001      8910 control
8ff0-8fff sprites
a000      ?
a060-a06f sprites
a080      command for the sound CPU
a0c0      watchdog reset???


The second CPU plays speech

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"



extern WRITE_HANDLER( champbas_videoram_w );
extern WRITE_HANDLER( champbas_colorram_w );
extern WRITE_HANDLER( champbas_gfxbank_w );
extern WRITE_HANDLER( champbas_flipscreen_w );

extern PALETTE_INIT( champbas );
extern VIDEO_START( champbas );
extern VIDEO_UPDATE( champbas );



WRITE_HANDLER( champbas_dac_w )
{
	DAC_signed_data_w(0,data<<2);
}

static MEMORY_READ_START( readmem )
	{ 0x0000, 0x5fff, MRA_ROM },
	{ 0x7800, 0x7fff, MRA_ROM },
	{ 0x8000, 0x8fff, MRA_RAM },
	{ 0xa000, 0xa000, input_port_0_r },
	{ 0xa040, 0xa040, input_port_1_r },
	{ 0xa080, 0xa080, input_port_2_r },
/*	{ 0xa0a0, 0xa0a0,  },	???? */
	{ 0xa0c0, 0xa0c0, input_port_3_r },
MEMORY_END

static MEMORY_WRITE_START( writemem )
	{ 0x0000, 0x5fff, MWA_ROM },
	/*{ 0x6000, 0x63ff, champbas_protection_w },*/
	{ 0x7000, 0x7000, AY8910_write_port_0_w },
	{ 0x7001, 0x7001, AY8910_control_port_0_w },
	{ 0x7800, 0x7fff, MWA_ROM },
	{ 0x8000, 0x83ff, champbas_videoram_w, &videoram },
	{ 0x8400, 0x87ff, champbas_colorram_w, &colorram },
	{ 0x8800, 0x8fef, MWA_RAM },
	{ 0x8ff0, 0x8fff, MWA_RAM, &spriteram, &spriteram_size},
	{ 0xa000, 0xa000, interrupt_enable_w },
	{ 0xa002, 0xa002, champbas_gfxbank_w },
	{ 0xa003, 0xa003, champbas_flipscreen_w },
	/*{ 0xa006, 0xa007, champbas_protection_w },*/
	{ 0xa060, 0xa06f, MWA_RAM, &spriteram_2 },
	{ 0xa080, 0xa080, soundlatch_w },
	{ 0xa0c0, 0xa0c0, watchdog_reset_w },
MEMORY_END

static MEMORY_READ_START( readmem2 )
	{ 0x0000, 0x5fff, MRA_ROM },
	{ 0x6000, 0x6000, soundlatch_r },
	{ 0xe000, 0xe3ff, MRA_RAM },
MEMORY_END

static MEMORY_WRITE_START( writemem2 )
	{ 0x0000, 0x5fff, MWA_ROM },
/*	{ 0x8000, 0x8000, MWA_NOP },	unknown - maybe DAC enable */
	{ 0xa000, 0xa000, soundlatch_w },	/* probably. The sound latch has to be cleared some way */
	{ 0xc000, 0xc000, champbas_dac_w },
	{ 0xe000, 0xe3ff, MWA_RAM },
MEMORY_END



INPUT_PORTS_START( champbas )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON3 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON4 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY )

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_COCKTAIL )

	PORT_START	/* DSW */
	PORT_DIPNAME( 0x03, 0x02, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x03, "A 2/1 B 3/2" )
	PORT_DIPSETTING(    0x02, "A 1/1 B 2/1")
	PORT_DIPSETTING(    0x01, "A 1/2 B 1/6" )
	PORT_DIPSETTING(    0x00, "A 1/3 B 1/6")
	PORT_DIPNAME( 0x04, 0x00, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ))
	PORT_DIPSETTING(    0x04, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ))
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ))
	PORT_DIPSETTING(    0x10, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x20, 0x00, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x20, "Easy" )
	PORT_DIPSETTING(    0x00, "Hard")
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ))
	PORT_DIPSETTING(    0x40, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Unknown ) ) /* The game won't boot if set to ON */
	PORT_DIPSETTING(    0x00, DEF_STR( Off ))
	PORT_DIPSETTING(    0x80, DEF_STR( On ) )

	PORT_START	/* COIN */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END



static struct GfxLayout charlayout =
{
	8,8,	/* 8*8 characters */
	256,	/* 256 characters */
	2,	/* 2 bits per pixel */
	{ 0, 4 },	/* the two bitplanes for 4 pixels are packed into one byte */
	{ 8*8+0, 8*8+1, 8*8+2, 8*8+3, 0, 1, 2, 3 },	/* bits are packed in groups of four */
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	16*8	/* every char takes 16 bytes */
};

static struct GfxLayout spritelayout =
{
	16,16,	/* 16*16 sprites */
	64,	/* 64 sprites */
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
	{ REGION_GFX1, 0x0000, &charlayout,   0, 64 },
	{ REGION_GFX2, 0x0000, &charlayout,   0, 64 },
	{ REGION_GFX1, 0x1000, &spritelayout, 0, 64 },
	{ REGION_GFX2, 0x1000, &spritelayout, 0, 64 },
	{ -1 } /* end of array */
};




static struct AY8910interface ay8910_interface =
{
	1,	/* 1 chip */
	1500000,	/* 1.5 MHz ? */
	{ 30 },
	{ input_port_0_r },
	{ input_port_1_r },
	{ 0 },
	{ 0 }
};

static struct DACinterface dac_interface =
{
	1,
	{ 70 }
};



static MACHINE_DRIVER_START( champbas )

	/* basic machine hardware */
	MDRV_CPU_ADD(Z80, 3072000)	/* 3.072 MHz (?) */
	MDRV_CPU_MEMORY(readmem,writemem)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,1)

	MDRV_CPU_ADD(Z80, 3072000)
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)	/* 3.072 MHz ? */
	MDRV_CPU_MEMORY(readmem2,writemem2)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 2*8, 30*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(32)
	MDRV_COLORTABLE_LENGTH(64*4)

	MDRV_PALETTE_INIT(champbas)
	MDRV_VIDEO_START(champbas)
	MDRV_VIDEO_UPDATE(champbas)

	/* sound hardware */
	MDRV_SOUND_ADD(AY8910, ay8910_interface)
	MDRV_SOUND_ADD(DAC, dac_interface)
MACHINE_DRIVER_END



/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( champbas )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "champbb.1",    0x0000, 0x2000, CRC(218de21e) SHA1(7577fd04bdda4666c017f3b36e81ec23bcddd845) )
	ROM_LOAD( "champbb.2",    0x2000, 0x2000, CRC(5ddd872e) SHA1(68e21572e27707c991180b1bd0a6b31f7b64abf6) )
	ROM_LOAD( "champbb.3",    0x4000, 0x2000, CRC(f39a7046) SHA1(3097bffe84ac74ce9e6481028a0ebbe8b1d6eaf9) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for the speech CPU */
	ROM_LOAD( "champbb.6",    0x0000, 0x2000, CRC(26ab3e16) SHA1(019b9d34233a6b7a53e204154b782ceb42915d2b) )
	ROM_LOAD( "champbb.7",    0x2000, 0x2000, CRC(7c01715f) SHA1(b15b2001b8c110f2599eee3aeed79f67686ebd7e) )
	ROM_LOAD( "champbb.8",    0x4000, 0x2000, CRC(3c911786) SHA1(eea0c467e213d237b5bb9d04b19a418d6090c2dc) )

	ROM_REGION( 0x2000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "champbb.4",    0x0000, 0x2000, CRC(1930fb52) SHA1(cae0b2701c2b53b79e9df3a7496442ba3472e996) )

	ROM_REGION( 0x2000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "champbb.5",    0x0000, 0x2000, CRC(a4cef5a1) SHA1(fa00ed0d075e00992a1ddce3c1327ed74770a735) )

	ROM_REGION( 0x0120, REGION_PROMS, 0 )
	ROM_LOAD( "champbb.pr2",  0x0000, 0x020, CRC(2585ffb0) SHA1(ce7f62f37955c2bbb4f82b139cc716978b084767) ) /* palette */
	ROM_LOAD( "champbb.pr1",  0x0020, 0x100, CRC(872dd450) SHA1(6c1e2c4a2fc072f4bf4996c731adb0b01b347506) ) /* look-up table */
ROM_END

ROM_START( champbbj )
	ROM_REGION( 0x10000, REGION_CPU1, 0 ) /* 64k for code */
	ROM_LOAD( "11.2e",      0x0000, 0x2000, CRC(e2dfc166) SHA1(482e084d7d21b1cf2d17431699e6bab4c4b6ac15) )
	ROM_LOAD( "12.2g",      0x2000, 0x2000, CRC(7b4e5faa) SHA1(b7201816a819ef313ddc81f312d26982b83ef1c7) )
	ROM_LOAD( "13.2h",      0x4000, 0x2000, CRC(b201e31f) SHA1(bba3b611ff60ad8d5dd8484df4cfc2026f4fd344) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* 64k for the speech CPU */
	ROM_LOAD( "16.2k",      0x0000, 0x2000, CRC(24c482ee) SHA1(c25bdf77014e095fc11a9a6b17f16858f19db451) )
	ROM_LOAD( "17.2l",      0x2000, 0x2000, CRC(f10b148b) SHA1(d66516d509f6f16e51ee59d27c4867e276064c3f) )
	ROM_LOAD( "18.2n",      0x4000, 0x2000, CRC(2dc484dd) SHA1(28bd68c787d7e6989849ca52009948dbd5cdcc79) )

	ROM_REGION( 0x2000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "14.5e",      0x0000, 0x2000, CRC(1b8202b3) SHA1(889b77fc3d0cb029baf8c47be260f513f3ed59bd) )

	ROM_REGION( 0x2000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "15.5g",      0x0000, 0x2000, CRC(a67c0c40) SHA1(3845839eff8c1624d26937f28ffde67a5fcb4805) )

	ROM_REGION( 0x0120, REGION_PROMS, 0 )
	ROM_LOAD( "1e.bpr",    0x0000, 0x0020, CRC(f5ce825e) SHA1(956f580840f1a7d24bfbd72b2929d14e9ee1b660) ) /* palette */
	ROM_LOAD( "5k.bpr",    0x0020, 0x0100, CRC(2e481ffa) SHA1(bc8979efd43bee8be0ce96ebdacc873a5821e06e) ) /* look-up table */
ROM_END

ROM_START( champbja )
	ROM_REGION( 0x10000, REGION_CPU1, 0 ) /* 64k for code */
	ROM_LOAD( "10",         0x0000, 0x2000, CRC(f7cdaf8e) SHA1(d4c840f2107394fadbcf822d64aaa381ac900367) ) 
	ROM_LOAD( "09",         0x2000, 0x2000, CRC(9d39e5b3) SHA1(11c1a1d2296c0bf16d7610eaa79b034bfd813740) ) 
	ROM_LOAD( "08",         0x4000, 0x2000, CRC(53468a0f) SHA1(d4b5ea48b27754eebe593c8b4fcf5bf117f27ae4) ) 

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* 64k for the speech CPU */
	ROM_LOAD( "16.2k",      0x0000, 0x2000, CRC(24c482ee) SHA1(c25bdf77014e095fc11a9a6b17f16858f19db451) )
	ROM_LOAD( "17.2l",      0x2000, 0x2000, CRC(f10b148b) SHA1(d66516d509f6f16e51ee59d27c4867e276064c3f) )
	ROM_LOAD( "18.2n",      0x4000, 0x2000, CRC(2dc484dd) SHA1(28bd68c787d7e6989849ca52009948dbd5cdcc79) )

	ROM_REGION( 0x2000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "14.5e",      0x0000, 0x2000, CRC(1b8202b3) SHA1(889b77fc3d0cb029baf8c47be260f513f3ed59bd) )

	ROM_REGION( 0x2000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "15.5g",      0x0000, 0x2000, CRC(a67c0c40) SHA1(3845839eff8c1624d26937f28ffde67a5fcb4805) )

	ROM_REGION( 0x0120, REGION_PROMS, 0 )
	ROM_LOAD( "clr",       0x0000, 0x0020, CRC(8f989357) SHA1(d0916fb5ef4b43bdf84663cd403418ffc5e98c17) ) /* palette */ 
	ROM_LOAD( "5k.bpr",    0x0020, 0x0100, CRC(2e481ffa) SHA1(bc8979efd43bee8be0ce96ebdacc873a5821e06e) ) /* look-up table */
ROM_END

ROM_START( champbb2 )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "epr5932",      0x0000, 0x2000, CRC(528e3c78) SHA1(ee300201580c1bace783f1340bd4f1ea2a00dffa) )
	ROM_LOAD( "epr5929",      0x2000, 0x2000, CRC(17b6057e) SHA1(67c5aed950acf4d045edf39019066af2896265e1) )
	ROM_LOAD( "epr5930",      0x4000, 0x2000, CRC(b6570a90) SHA1(5a2651aeac986000913b5854792b2d81df6b2fc6) )
	ROM_LOAD( "epr5931",      0x7800, 0x0800, CRC(0592434d) SHA1(a7f61546c39ffdbff46c4db485c9b3f6eefcf1ac) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for the speech CPU */
	ROM_LOAD( "epr5933",      0x0000, 0x2000, CRC(26ab3e16) SHA1(019b9d34233a6b7a53e204154b782ceb42915d2b) )
	ROM_LOAD( "epr5934",      0x2000, 0x2000, CRC(7c01715f) SHA1(b15b2001b8c110f2599eee3aeed79f67686ebd7e) )
	ROM_LOAD( "epr5935",      0x4000, 0x2000, CRC(3c911786) SHA1(eea0c467e213d237b5bb9d04b19a418d6090c2dc) )

	ROM_REGION( 0x2000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "epr5936",      0x0000, 0x2000, CRC(c4a4df75) SHA1(7b85dbf405697b0b8881f910c08f6db6c828b19a) )

	ROM_REGION( 0x2000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "epr5937",      0x0000, 0x2000, CRC(5c80ec42) SHA1(9b79737577e48a6b2ec20ce145252545955e82c3) )

	ROM_REGION( 0x0120, REGION_PROMS, 0 )
	ROM_LOAD( "pr5957",       0x0000, 0x020, CRC(f5ce825e) SHA1(956f580840f1a7d24bfbd72b2929d14e9ee1b660) ) /* palette */
	ROM_LOAD( "pr5956",       0x0020, 0x100, CRC(872dd450) SHA1(6c1e2c4a2fc072f4bf4996c731adb0b01b347506) ) /* look-up table */
ROM_END



GAME(1983, champbas, 0,        champbas, champbas, 0, ROT0, "Sega", "Champion Baseball" )
GAMEX(1983, champbbj, champbas, champbas, champbas, 0, ROT0, "Alpha Denshi Co.", "Champion Baseball (Japan set 1)", GAME_NOT_WORKING )
GAMEX(1983, champbja, champbas, champbas, champbas, 0, ROT0, "Alpha Denshi Co.", "Champion Baseball (Japan set 2)", GAME_NOT_WORKING )
GAMEX(1983, champbb2, 0,        champbas, champbas, 0, ROT0, "Sega", "Champion Baseball II", GAME_NOT_WORKING )
