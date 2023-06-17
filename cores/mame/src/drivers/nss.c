/***************************************************************************

  snes.c

  Driver file to handle emulation of the Nintendo Super NES.

  Anthony Kruize
  Based on the original MESS driver by Lee Hammerton (aka Savoury Snax)

  Driver is preliminary right now.
  Sound emulation currently consists of the SPC700 and that's about it. Without
  the DSP being emulated, there's no sound even if the code is being executed.
  I need to figure out how to get the 65816 and the SPC700 to stay in sync.

  The memory map included below is setup in a way to make it easier to handle
  Mode 20 and Mode 21 ROMs.

  Todo (in no particular order):
    - Emulate extra chips - superfx, dsp2, sa-1 etc.
    - Add sound emulation. Currently the SPC700 is emulated, but that's it.
    - Add horizontal mosaic, hi-res. interlaced etc to video emulation.
    - Add support for fullgraphic mode(partially done).
    - Fix support for Mode 7. (In Progress)
    - Handle interleaved roms (maybe even multi-part roms, but how?)
    - Add support for running at 3.58Mhz at the appropriate time.
    - I'm sure there's lots more ...

    Nintendo Super System

  There is a second processor and Menu system for selecting the games
  controlling timer etc.? which still needs emulating there are dipswitches too

***************************************************************************/
#include "driver.h"
#include "vidhrdw/generic.h"
#include "includes/snes.h"

extern DRIVER_INIT( snes );

static MEMORY_READ_START( snes_readmem )
	{ 0x000000, 0x2fffff, snes_r_bank1 },	/* I/O and ROM (repeats for each bank) */
	{ 0x300000, 0x3fffff, snes_r_bank2 },	/* I/O and ROM (repeats for each bank) */
	{ 0x400000, 0x5fffff, snes_r_bank3 },	/* ROM (and reserved in Mode 20) */
	{ 0x600000, 0x6fffff, MRA_NOP },		/* Reserved */
	{ 0x700000, 0x77ffff, snes_r_sram },	/* 256KB Mode 20 save ram + reserved from 0x8000 - 0xffff */
	{ 0x780000, 0x7dffff, MRA_NOP },		/* Reserved */
	{ 0x7e0000, 0x7fffff, MRA_RAM },		/* 8KB Low RAM, 24KB High RAM, 96KB Expanded RAM */
	{ 0x800000, 0xffffff, snes_r_bank4 },	/* Mirror and ROM */
MEMORY_END

static MEMORY_WRITE_START( snes_writemem )
	{ 0x000000, 0x2fffff, snes_w_bank1 },	/* I/O and ROM (repeats for each bank) */
	{ 0x300000, 0x3fffff, snes_w_bank2 },	/* I/O and ROM (repeats for each bank) */
	{ 0x400000, 0x5fffff, MWA_ROM },		/* ROM (and reserved in Mode 20) */
	{ 0x600000, 0x6fffff, MWA_NOP },		/* Reserved */
	{ 0x700000, 0x77ffff, MWA_RAM },		/* 256KB Mode 20 save ram + reserved from 0x8000 - 0xffff */
	{ 0x780000, 0x7dffff, MWA_NOP },		/* Reserved */
	{ 0x7e0000, 0x7fffff, MWA_RAM },		/* 8KB Low RAM, 24KB High RAM, 96KB Expanded RAM */
	{ 0x800000, 0xffffff, snes_w_bank4 },	/* Mirror and ROM */
MEMORY_END

static MEMORY_READ_START( spc_readmem )
	{ 0x0000, 0x00ef, MRA_RAM },			/* lower 32k ram */
	{ 0x00f0, 0x00ff, spc_io_r },			/* spc io */
	{ 0x0100, 0x7fff, MRA_RAM },			/* lower 32k ram continued */
	{ 0x8000, 0xffbf, MRA_RAM },			/* upper 32k ram */
	{ 0xffc0, 0xffff, spc_bank_r },			/* upper 32k ram continued or Initial Program Loader ROM */
MEMORY_END

static MEMORY_WRITE_START( spc_writemem )
	{ 0x0000, 0x00ef, MWA_RAM },			/* lower 32k ram */
	{ 0x00f0, 0x00ff, spc_io_w },			/* spc io */
	{ 0x0100, 0x7fff, MWA_RAM },			/* lower 32k ram continued */
	{ 0x8000, 0xffbf, MWA_RAM },			/* upper 32k ram */
	{ 0xffc0, 0xffff, spc_bank_w },			/* upper 32k ram continued or Initial Program Loader ROM */
MEMORY_END

INPUT_PORTS_START( snes )
	PORT_START  /* IN 0 : Joypad 1 - L */
	PORT_BIT_NAME( 0x80, IP_ACTIVE_HIGH, IPT_BUTTON3   | IPF_PLAYER1, "P1 Button A" )
	PORT_BIT_NAME( 0x40, IP_ACTIVE_HIGH, IPT_BUTTON4   | IPF_PLAYER1, "P1 Button X" )
	PORT_BIT_NAME( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON5   | IPF_PLAYER1, "P1 Button L" )
	PORT_BIT_NAME( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON6   | IPF_PLAYER1, "P1 Button R" )
	PORT_START  /* IN 1 : Joypad 1 - H */
	PORT_BIT_NAME( 0x80, IP_ACTIVE_HIGH, IPT_BUTTON1   | IPF_PLAYER1, "P1 Button B" )
	PORT_BIT_NAME( 0x40, IP_ACTIVE_HIGH, IPT_BUTTON2   | IPF_PLAYER1, "P1 Button Y" )
	PORT_BIT_NAME( 0x20, IP_ACTIVE_HIGH, IPT_SERVICE1, "P1 Select" )
	PORT_BIT_NAME( 0x10, IP_ACTIVE_HIGH, IPT_START1,  "P1 Start" )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP    | IPF_PLAYER1 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN  | IPF_PLAYER1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_PLAYER1 )
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_PLAYER1 )

	PORT_START  /* IN 2 : Joypad 2 - L */
	PORT_BIT_NAME( 0x80, IP_ACTIVE_HIGH, IPT_BUTTON3   | IPF_PLAYER2, "P2 Button A" )
	PORT_BIT_NAME( 0x40, IP_ACTIVE_HIGH, IPT_BUTTON4   | IPF_PLAYER2, "P2 Button X" )
	PORT_BIT_NAME( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON5   | IPF_PLAYER2, "P2 Button L" )
	PORT_BIT_NAME( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON6   | IPF_PLAYER2, "P2 Button R" )
	PORT_START  /* IN 3 : Joypad 2 - H */
	PORT_BIT_NAME( 0x80, IP_ACTIVE_HIGH, IPT_BUTTON1   | IPF_PLAYER2, "P2 Button B" )
	PORT_BIT_NAME( 0x40, IP_ACTIVE_HIGH, IPT_BUTTON2   | IPF_PLAYER2, "P2 Button Y" )
	PORT_BIT_NAME( 0x20, IP_ACTIVE_HIGH, IPT_SERVICE2, "P2 Select" )
	PORT_BIT_NAME( 0x10, IP_ACTIVE_HIGH, IPT_START2,  "P2 Start" )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP    | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN  | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_PLAYER2 )
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_PLAYER2 )

	PORT_START  /* IN 4 : Joypad 3 - L */
	PORT_BIT_NAME( 0x80, IP_ACTIVE_HIGH, IPT_BUTTON3   | IPF_PLAYER3, "P3 Button A" )
	PORT_BIT_NAME( 0x40, IP_ACTIVE_HIGH, IPT_BUTTON4   | IPF_PLAYER3, "P3 Button X" )
	PORT_BIT_NAME( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON5   | IPF_PLAYER3, "P3 Button L" )
	PORT_BIT_NAME( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON6   | IPF_PLAYER3, "P3 Button R" )
	PORT_START  /* IN 5 : Joypad 3 - H */
	PORT_BIT_NAME( 0x80, IP_ACTIVE_HIGH, IPT_BUTTON1   | IPF_PLAYER3, "P3 Button B" )
	PORT_BIT_NAME( 0x40, IP_ACTIVE_HIGH, IPT_BUTTON2   | IPF_PLAYER3, "P3 Button Y" )
	PORT_BIT_NAME( 0x20, IP_ACTIVE_HIGH, IPT_SERVICE3, "P3 Select" )
	PORT_BIT_NAME( 0x10, IP_ACTIVE_HIGH, IPT_START3,  "P3 Start" )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP    | IPF_PLAYER3 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN  | IPF_PLAYER3 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_PLAYER3 )
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_PLAYER3 )

	PORT_START  /* IN 6 : Joypad 4 - L */
	PORT_BIT_NAME( 0x80, IP_ACTIVE_HIGH, IPT_BUTTON3   | IPF_PLAYER4, "P4 Button A" )
	PORT_BIT_NAME( 0x40, IP_ACTIVE_HIGH, IPT_BUTTON4   | IPF_PLAYER4, "P4 Button X" )
	PORT_BIT_NAME( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON5   | IPF_PLAYER4, "P4 Button L" )
	PORT_BIT_NAME( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON6   | IPF_PLAYER4, "P4 Button R" )
	PORT_START  /* IN 7 : Joypad 4 - H */
	PORT_BIT_NAME( 0x80, IP_ACTIVE_HIGH, IPT_BUTTON1   | IPF_PLAYER4, "P4 Button B" )
	PORT_BIT_NAME( 0x40, IP_ACTIVE_HIGH, IPT_BUTTON2   | IPF_PLAYER4, "P4 Button Y" )
	PORT_BIT_NAME( 0x20, IP_ACTIVE_HIGH, IPT_SERVICE4, "P4 Select" )
	PORT_BIT_NAME( 0x10, IP_ACTIVE_HIGH, IPT_START4,  "P4 Start" )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP    | IPF_PLAYER4 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN  | IPF_PLAYER4 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_PLAYER4 )
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_PLAYER4 )

	PORT_START	/* IN 8 : Internal switches */
	PORT_DIPNAME( 0x1, 0x1, "Enforce 32 sprites/line" )
	PORT_DIPSETTING(   0x0, DEF_STR( No )  )
	PORT_DIPSETTING(   0x1, DEF_STR( Yes ) )

#ifdef MAME_DEBUG
	PORT_START	/* IN 9 : debug switches */
	PORT_DIPNAME( 0x3, 0x0, "Browse tiles" )
	PORT_DIPSETTING(   0x0, DEF_STR( Off ) )
	PORT_DIPSETTING(   0x1, "2bpl"  )
	PORT_DIPSETTING(   0x2, "4bpl"  )
	PORT_DIPSETTING(   0x3, "8bpl"  )
	PORT_DIPNAME( 0xc, 0x0, "Browse maps" )
	PORT_DIPSETTING(   0x0, DEF_STR( Off ) )
	PORT_DIPSETTING(   0x4, "2bpl"  )
	PORT_DIPSETTING(   0x8, "4bpl"  )
	PORT_DIPSETTING(   0xc, "8bpl"  )

	PORT_START	/* IN 10 : debug switches */
	PORT_BIT_NAME( 0x1, IP_ACTIVE_HIGH, IPT_BUTTON7  | IPF_PLAYER2,  "Toggle BG 1" )
	PORT_BIT_NAME( 0x2, IP_ACTIVE_HIGH, IPT_BUTTON8  | IPF_PLAYER2,  "Toggle BG 2" )
	PORT_BIT_NAME( 0x4, IP_ACTIVE_HIGH, IPT_BUTTON9  | IPF_PLAYER2,  "Toggle BG 3" )
	PORT_BIT_NAME( 0x8, IP_ACTIVE_HIGH, IPT_BUTTON10 | IPF_PLAYER2,  "Toggle BG 4" )
	PORT_BIT_NAME( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON7 | IPF_PLAYER3,  "Toggle Objects" )
	PORT_BIT_NAME( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON8 | IPF_PLAYER3,  "Toggle Main/Sub" )
	PORT_BIT_NAME( 0x40, IP_ACTIVE_HIGH, IPT_BUTTON9 | IPF_PLAYER3,  "Toggle Back col" )
	PORT_BIT_NAME( 0x80, IP_ACTIVE_HIGH, IPT_BUTTON10 | IPF_PLAYER3, "Toggle Windows" )

	PORT_START	/* IN 11 : debug input */
	PORT_BIT_NAME( 0x1, IP_ACTIVE_HIGH, IPT_BUTTON9,  "Pal prev" )
	PORT_BIT_NAME( 0x2, IP_ACTIVE_HIGH, IPT_BUTTON10, "Pal next" )
	PORT_BIT_NAME( 0x4, IP_ACTIVE_HIGH, IPT_BUTTON7 | IPF_PLAYER4, "Toggle Transparency" )
#endif

	PORT_START	/* IN 12 : dip-switches */
	PORT_DIPNAME( 0x03, 0x00, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x00, "Normal" )
	PORT_DIPSETTING(    0x03, "Hard"  )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x0c, "2" )
	PORT_DIPSETTING(    0x08, "3" )
	PORT_DIPSETTING(    0x04, "4" )
	PORT_DIPSETTING(    0x00, "5" )
	PORT_DIPNAME( 0x30, 0x00, "Time limit per level?" ) /* taken from the scan of nss_adam*/
	PORT_DIPSETTING(    0x10, "104 sec." )
	PORT_DIPSETTING(    0x20, "112 sec." )
	PORT_DIPSETTING(    0x00, "120 sec." )
	PORT_DIPSETTING(    0x30, "? sec." )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x80, DEF_STR( On ) )
INPUT_PORTS_END

static struct CustomSound_interface snes_sound_interface =
{ snes_sh_start, 0, 0 };

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ -1 } /* end of array */
};

static PALETTE_INIT( snes )
{
	int i, r, g, b;

	for( i = 0; i < 32768; i++ )
	{
		r = (i & 0x1F) << 3;
		g = ((i >> 5) & 0x1F) << 3;
		b = ((i >> 10) & 0x1F) << 3;
		palette_set_color( i, r, g, b );
	}

	/* The colortable can be black */
	for( i = 0; i < 256; i++ )
		colortable[i] = 0;
}

static MACHINE_DRIVER_START( snes )
	/* basic machine hardware */
	MDRV_CPU_ADD_TAG("main", G65816, 2680000)	/* 2.68Mhz, also 3.58Mhz */
	MDRV_CPU_MEMORY(snes_readmem, snes_writemem)
	MDRV_CPU_VBLANK_INT(snes_scanline_interrupt, SNES_MAX_LINES_NTSC)

	MDRV_CPU_ADD_TAG("sound", SPC700, 2048000)	/* 2.048 Mhz */
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
	MDRV_CPU_MEMORY(spc_readmem, spc_writemem)
	MDRV_CPU_VBLANK_INT(NULL, 0)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(1)

	MDRV_MACHINE_INIT( snes )

	/* video hardware */
	MDRV_VIDEO_START( generic_bitmapped )
	MDRV_VIDEO_UPDATE( snes )

	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(SNES_SCR_WIDTH * 2, SNES_SCR_HEIGHT * 2)
	MDRV_VISIBLE_AREA(0, SNES_SCR_WIDTH-1, 0, SNES_SCR_HEIGHT-1 )
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(32768)
	MDRV_COLORTABLE_LENGTH(257)
	MDRV_PALETTE_INIT( snes )

	/* sound hardware */
	MDRV_SOUND_ADD(CUSTOM, snes_sound_interface)
	MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
MACHINE_DRIVER_END

/***************************************************************************

  Game driver(s)

***************************************************************************/

#define NSS_BIOS \
	ROM_REGION(0x1000000,       REGION_CPU1,  0)		/* 65C816 */ \
	ROM_REGION(SNES_VRAM_SIZE,  REGION_GFX1,  0)		/* VRAM */ \
	ROM_REGION(SNES_CGRAM_SIZE, REGION_USER1, 0)		/* CGRAM */ \
	ROM_REGION(SNES_OAM_SIZE,   REGION_USER2, 0)		/* OAM */ \
	ROM_REGION(0x10000,         REGION_CPU2,  0)		/* SPC700 */ \
	ROM_LOAD("spc700.rom", 0xFFC0, 0x40, CRC(38000b6b) SHA1(9f3af3d51c229e67daa68041492afa27287aad31) )	/* boot rom */ \
	ROM_REGION(0x10000,         REGION_CPU3,  0)		/* Bios CPU (what is it?) */ \
	ROM_LOAD("nss-c.dat"  , 0, 0x8000, CRC(a8e202b3) SHA1(b7afcfe4f5cf15df53452dc04be81929ced1efb2) )	/* bios */ \
	ROM_LOAD("nss-ic14.02", 0, 0x8000, CRC(e06cb58f) SHA1(62f507e91a2797919a78d627af53f029c7d81477) )	/* bios */ \

ROM_START( nss )
	NSS_BIOS
ROM_END


ROM_START( nss_actr )
	NSS_BIOS
	ROM_REGION( 0x100000, REGION_USER3, 0 )
	ROM_LOAD( "act-rais.ic3", 0x00000, 0x80000, CRC(c9f788c2) SHA1(fba2331fd5bcbe51d74115528fd3a9becf072e8d) )
	ROM_LOAD( "act-rais.ic2", 0x80000, 0x80000, CRC(4df9cc63) SHA1(3e98d9693d60d125a1257ba79701f27bda688261) )

	/* instruction / data rom for bios */
	ROM_REGION( 0x8000, REGION_USER4, 0 )
	ROM_LOAD( "act-rais.ic8", 0x0000, 0x8000, CRC(08b38ce6) SHA1(4cbb7fd28d98ffef0f17747201625883af954e3a) )
ROM_END

ROM_START( nss_con3 )
	NSS_BIOS
	ROM_REGION( 0x100000, REGION_USER3, 0 )
	ROM_LOAD( "contra3.ic3", 0x00000, 0x80000, CRC(33b03501) SHA1(c7f4835d5ec4983e487b00f0b4c49fede2f03b9c) )
	ROM_LOAD( "contra3.ic2", 0x80000, 0x80000, CRC(2f3e3b5b) SHA1(0186b92f022701f6ae29984252e6d346acf6550b) )

	/* instruction / data rom for bios */
	ROM_REGION( 0x8000, REGION_USER4, 0 )
	ROM_LOAD( "contra3.ic8", 0x0000, 0x8000, CRC(0fbfa23b) SHA1(e7a1a78a58c64297e7b9623350ec57aed8035a4f) )
ROM_END

ROM_START( nss_adam )
	NSS_BIOS
	ROM_REGION( 0x100000, REGION_USER3, 0 )
	ROM_LOAD( "addams.ic3", 0x00000, 0x80000, CRC(44643930) SHA1(a45204b2eb13c6befca30d130061b5b8ba054270) )
	ROM_LOAD( "addams.ic2", 0x80000, 0x80000, CRC(6196adcf) SHA1(a450f278a37d5822f607aa3631831a461e8b147e) )

	/* instruction / data rom for bios */
	ROM_REGION( 0x8000, REGION_USER4, 0 )
	ROM_LOAD( "addams.ic8", 0x0000, 0x8000, CRC(57c7f72c) SHA1(2e3642b4b5438f6c535d6d1eb668e1663062cf78) )
ROM_END

ROM_START( nss_aten )
	NSS_BIOS
	ROM_REGION( 0x100000, REGION_USER3, 0 )
	ROM_LOAD( "amtennis.ic3", 0x00000, 0x80000, CRC(aeabaf2a) SHA1(b355e0a322b57454e767785a49c14d4c7f492488) )
	ROM_LOAD( "amtennis.ic2", 0x80000, 0x80000, CRC(7738c5f2) SHA1(eb0089e9724c7b3834d9f6c47b92f5a1bb26fc77) )

	/* instruction / data rom for bios */
	ROM_REGION( 0x8000, REGION_USER4, 0 )
	ROM_LOAD( "amtennis.ic8", 0x0000, 0x8000, CRC(d2cd3926) SHA1(49fc253b1b9497ef1374c7db0bd72c163ffb07e7) )
ROM_END

ROM_START( nss_rob3 )
	NSS_BIOS
	ROM_REGION( 0x100000, REGION_USER3, 0 )
	ROM_LOAD( "robocop3.ic3", 0x00000, 0x80000, CRC(60916c42) SHA1(462d9645210a58bfd5204bd209eae2cdadb4493e) )
	ROM_LOAD( "robocop3.ic2", 0x80000, 0x80000, CRC(a94e1b56) SHA1(7403d70504310ad5949a3b45b4a1e71e7d2bce77) )

	/* instruction / data rom for bios */
	ROM_REGION( 0x8000, REGION_USER4, 0 )
	ROM_LOAD( "robocop3.ic8", 0x0000, 0x8000, CRC(90d13c51) SHA1(6751dab14b7d178350ac333f07dd2c3852e4ae23) )
ROM_END

ROM_START( nss_ncaa )
	NSS_BIOS
	ROM_REGION( 0x100000, REGION_USER3, 0 )
	ROM_LOAD( "ncaa.ic3", 0x00000, 0x80000, CRC(ef49ad8c) SHA1(4c40f3746b995b53f006434b9ccec06d8fe16e1f) )
	ROM_LOAD( "ncaa.ic2", 0x80000, 0x80000, CRC(83ef6936) SHA1(8e0f38c763861e33684c6ddb742385b0522af78a) )

	/* instruction / data rom for bios */
	ROM_REGION( 0x8000, REGION_USER4, 0 )
	ROM_LOAD( "ncaa.ic8", 0x0000, 0x8000, CRC(b9fa28d5) SHA1(bc538bcff5c19eae4becc6582b5c111d287b76fa) )
ROM_END

ROM_START( nss_skin )
	NSS_BIOS
	ROM_REGION( 0x100000, REGION_USER3, 0 )
	ROM_LOAD( "skins.ic3", 0x00000, 0x80000, CRC(ee1bb84d) SHA1(549ad9319e94a5d75cd4af017e63ea93ab407c87) )
	ROM_LOAD( "skins.ic2", 0x80000, 0x80000, CRC(365fd19e) SHA1(f60d7ac39fe83fb98730e73fbef410c90a4ff35b) )

	/* instruction / data rom for bios */
	ROM_REGION( 0x8000, REGION_USER4, 0 )
	ROM_LOAD( "skins.ic8", 0x0000, 0x8000, CRC(9f33d5ce) SHA1(4d279ad3665bd94c7ca9cb2778572bed42c5b298) )
ROM_END

ROM_START( nss_lwep )
	NSS_BIOS
	ROM_REGION( 0x100000, REGION_USER3, 0 )
	ROM_LOAD( "nss-lw.ic3", 0x00000, 0x80000, CRC(32564666) SHA1(bf371218fa303ce95eab09fb6017a522071dcd7e) )
	ROM_LOAD( "nss-lw.ic2", 0x80000, 0x80000, CRC(86365042) SHA1(f818024c6f858fd2780396b6c83d3a37a97fa08a) )

	/* instruction / data rom for bios */
	ROM_REGION( 0x8000, REGION_USER4, 0 )
	ROM_LOAD( "nss-lw.ic8", 0x0000, 0x8000, CRC(1acc1d5d) SHA1(4c8b100ac5847915aaf3b5bfbcb4f632606c97de) )
ROM_END

ROM_START( nss_ssoc )
	NSS_BIOS
	ROM_REGION( 0x100000, REGION_USER3, 0 )
	ROM_LOAD( "s-soccer.ic1", 0x00000, 0x80000,  CRC(70b7f50e) SHA1(92856118528995e3a0b7d22340d440bef5fd61ac) )

	/* instruction / data rom for bios */
	ROM_REGION( 0x8000, REGION_USER4, 0 )
	ROM_LOAD( "s-soccer.ic3", 0x0000, 0x8000, CRC(c09211c3) SHA1(b274a57f93ae0a8774664df3d3615fb7dbecfa2e) )
ROM_END

/* the ones below could be bad, smw certainly is */

ROM_START( nss_smw )
	NSS_BIOS
	ROM_REGION( 0x100000, REGION_USER3, 0 )
	ROM_LOAD( "mw.ic1", 0x00000, 0x40000, BAD_DUMP CRC(cfa601e7) SHA1(5c9a9a9fccd4b4fcbbda06dfdee41e92dc4e9097) ) /* half size?*/

	/* instruction / data rom for bios */
	ROM_REGION( 0x8000, REGION_USER4, 0 )
	ROM_LOAD( "mw.ic3", 0x0000, 0x8000, CRC(f2c5466e) SHA1(e116f01342fcf359498ed8750741c139093b1fb2) )
ROM_END

ROM_START( nss_fzer )
	NSS_BIOS
	ROM_REGION( 0x100000, REGION_USER3, 0 )
	ROM_LOAD( "fz.ic2", 0x00000, 0x40000, BAD_DUMP CRC(cd281855) SHA1(c9f7895028bbeed3deaa82f4fb51f2569093124b) ) /* maybe wrong size?*/

	/* instruction / data rom for bios */
	ROM_REGION( 0x8000, REGION_USER4, 0 )
	ROM_LOAD( "fz.ic7", 0x0000, 0x8000, CRC(48ae570d) SHA1(934f9fec47dcf9e49936388968d2db50c69950da) )
ROM_END

ROM_START( nss_sten )
	NSS_BIOS
	ROM_REGION( 0x100000, REGION_USER3, 0 )
	ROM_LOAD( "st.ic1", 0x00000, 0x40000, BAD_DUMP CRC(e94b48d9) SHA1(1ce9c25f8fb62798bbe016b2d1de1724d52e5674) )/* maybe wrong size?*/

	/* instruction / data rom for bios */
	ROM_REGION( 0x8000, REGION_USER4, 0 )
	ROM_LOAD( "st.ic3", 0x0000, 0x8000, CRC(8880596e) SHA1(ec6d68fc2f51f7d94f496cd72cf898db65324542) )
ROM_END

GAMEX( 199?, nss,       0,		  snes,	     snes,    snes,		ROT0, "Nintendo",					"Nintendo Super System - BIOS", NOT_A_DRIVER )
GAMEX( 1992, nss_actr,  nss,	  snes,	     snes,    snes,		ROT0, "Enix",						"Nintendo Super System - Act Raiser", GAME_NO_SOUND | GAME_NOT_WORKING ) /* time broken*/
GAMEX( 1992, nss_adam,  nss,	  snes,	     snes,    snes,		ROT0, "Ocean",						"Nintendo Super System - The Addams Family", GAME_NO_SOUND | GAME_NOT_WORKING ) /* crashes mame*/
GAMEX( 1992, nss_aten,  nss,	  snes,	     snes,    snes,		ROT0, "Absolute Entertainment Inc.","Nintendo Super System - David Crane's Amazing Tennis", GAME_NO_SOUND | GAME_IMPERFECT_GRAPHICS ) /* gfx problems with net*/
GAMEX( 1992, nss_con3,  nss,	  snes,	     snes,    snes,		ROT0, "Konami",						"Nintendo Super System - Contra 3 - The Alien Wars", GAME_NO_SOUND )
GAMEX( 1992, nss_lwep,  nss,	  snes,	     snes,    snes,		ROT0, "Ocean",						"Nintendo Super System - Lethal Weapon", GAME_NO_SOUND )
GAMEX( 1992, nss_ncaa,  nss,	  snes,	     snes,    snes,		ROT0, "Sculptured Software Inc.",	"Nintendo Super System - NCAA Basketball", GAME_NO_SOUND | GAME_NOT_WORKING ) /* severe gfx problems, no inputs*/
GAMEX( 1992, nss_rob3,  nss,	  snes,	     snes,    snes,		ROT0, "Ocean",						"Nintendo Super System - Robocop 3", GAME_NO_SOUND | GAME_NOT_WORKING ) /* invisible enemy? gameplay prob?*/
GAMEX( 1992, nss_skin,  nss,	  snes,	     snes,    snes,		ROT0, "Irem",						"Nintendo Super System - Skins Game", GAME_NO_SOUND | GAME_NOT_WORKING ) /* uses some gfx modes not implemented*/
GAMEX( 1992, nss_ssoc,  nss,	  snes,	     snes,    snes,		ROT0, "Human Inc.",					"Nintendo Super System - Super Soccer", GAME_NO_SOUND | GAME_NOT_WORKING ) /* lots of gfx problems*/
GAMEX( 199?, nss_smw,   nss,	  snes,	     snes,    snes,		ROT0, "Nintendo",					"Nintendo Super System - Super Mario World", GAME_NO_SOUND | GAME_NOT_WORKING ) /* bad rom*/
GAMEX( 199?, nss_fzer,  nss,	  snes,	     snes,    snes,		ROT0, "Nintendo",					"Nintendo Super System - F-Zero", GAME_NO_SOUND | GAME_NOT_WORKING ) /* bad rom*/
GAMEX( 199?, nss_sten,  nss,	  snes,	     snes,    snes,		ROT0, "Nintendo",					"Nintendo Super System - Super Tennis", GAME_NO_SOUND | GAME_NOT_WORKING ) /* bad rom*/
