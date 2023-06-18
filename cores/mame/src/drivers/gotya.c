/****************************************************************************

	Gotya / The Hand driver by Zsolt Vasvari


TODO: Emulated sound

	  Hitachi HD38880BP
         	  HD38882PA06

	  I think HD38880 is a CPU/MCU, because the game just sends it a sound command (0-0x1a)

****************************************************************************/

/****************************************************************************
 About GotYa (from the board owner)

 I believe it is a prototype for several reasons.
 There were quite a few jumpers on the board, hand written labels with
 the dates on them. I also have the manual, the game name is clearly Got-Ya
 and is a Game-A-Tron game.  The game itself had a few flyers from GAT inside
 so I have a hard time believing it was a bootleg.

----

 so despite the fact that 'gotya' might look like its a bootleg of thehand,
 its more likely just a prototype / alternate version, its hard to tell
****************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"


extern UINT8 *gotya_scroll;
extern UINT8 *gotya_videoram2;

extern WRITE_HANDLER( gotya_videoram_w );
extern WRITE_HANDLER( gotya_colorram_w );
extern WRITE_HANDLER( gotya_videoram2_w );

extern PALETTE_INIT( gotya );
extern VIDEO_START( gotya );
extern VIDEO_UPDATE( gotya );

extern WRITE_HANDLER( gotya_video_control_w );

extern WRITE_HANDLER( gotya_soundlatch_w );


static MEMORY_READ_START( readmem )
	{ 0x0000, 0x3fff, MRA_ROM },
	{ 0x5000, 0x5fff, MRA_RAM },
	{ 0x6000, 0x6000, input_port_0_r },
	{ 0x6001, 0x6001, input_port_1_r },
	{ 0x6002, 0x6002, input_port_2_r },
	{ 0xc000, 0xd3ff, MRA_RAM },
MEMORY_END

static MEMORY_WRITE_START( writemem )
	{ 0x0000, 0x3fff, MWA_ROM },
	{ 0x5000, 0x5fff, MWA_RAM },
	{ 0x6004, 0x6004, gotya_video_control_w },
	{ 0x6005, 0x6005, gotya_soundlatch_w },
	{ 0x6006, 0x6006, MWA_RAM, &gotya_scroll },
	{ 0x6007, 0x6007, watchdog_reset_w },
	{ 0xc000, 0xc7ff, gotya_videoram_w, &videoram },
	{ 0xc800, 0xcfff, gotya_colorram_w, &colorram },
	{ 0xd000, 0xd3df, MWA_RAM, &gotya_videoram2 },
	{ 0xd3e0, 0xd3ff, MWA_RAM, &spriteram },
MEMORY_END


INPUT_PORTS_START( gotya )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_PLAYER1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_PLAYER1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_PLAYER1 )
	PORT_SERVICE( 0x10, IP_ACTIVE_LOW )
	PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER1, "P1 Paper", IP_KEY_DEFAULT, IP_JOY_DEFAULT )
	PORT_BITX(0x40, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1, "P1 Scissors", IP_KEY_DEFAULT, IP_JOY_DEFAULT )
	PORT_BITX(0x80, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1, "P1 Rock", IP_KEY_DEFAULT, IP_JOY_DEFAULT )

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_PLAYER2 )
	PORT_DIPNAME( 0x10, 0x10, "Sound Test" )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2, "P2 Paper", IP_KEY_DEFAULT, IP_JOY_DEFAULT )
	PORT_BITX(0x40, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2, "P2 Scissors", IP_KEY_DEFAULT, IP_JOY_DEFAULT )
	PORT_BITX(0x80, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2, "P2 Rock", IP_KEY_DEFAULT, IP_JOY_DEFAULT )

	PORT_START	/* DSW1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Cocktail ) )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START2 )
	PORT_DIPNAME( 0x10, 0x00, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x00, "Easy" )
	PORT_DIPSETTING(    0x10, "Hard" )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x00, "None" )
	PORT_DIPSETTING(    0x20, "15000" )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x40, "5" )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x80, DEF_STR( On ) )

INPUT_PORTS_END


static struct GfxLayout charlayout =
{
	8,8,	/* 8*8 characters */
	256,	/* 256 characters */
	2,	    /* 2 bits per pixel */
	{ 0, 4 },	/* the bitplanes are packed in one byte */
	{ 0, 1, 2, 3, 8*8+0, 8*8+1, 8*8+2, 8*8+3 },
	{ 7*8, 6*8, 5*8, 4*8, 3*8, 2*8, 1*8, 0*8 },
	16*8	/* every char takes 16 consecutive bytes */
};

static struct GfxLayout spritelayout =
{
	16,16,	/* 16*16 characters */
	64,		/* 64 characters */
	2,	    /* 2 bits per pixel */
	{ 0, 4 },	/* the bitplanes are packed in one byte */
	{ 0, 1, 2, 3, 24*8+0, 24*8+1, 24*8+2, 24*8+3,
	  16*8+0, 16*8+1, 16*8+2, 16*8+3, 8*8+0, 8*8+1, 8*8+2, 8*8+3 },
	{ 39*8, 38*8, 37*8, 36*8, 35*8, 34*8, 33*8, 32*8,
	   7*8,  6*8,  5*8,  4*8,  3*8,  2*8,  1*8,  0*8 },
	64*8	/* every char takes 64 consecutive bytes */
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &charlayout,   0, 16 },
	{ REGION_GFX2, 0, &spritelayout, 0, 16 },
	{ -1 } /* end of array */
};


static const char *sample_names[] =
{												/* Address triggered at*/
	"*thehand",
	"01.wav",	/* game start tune */			/* 075f*/
	"02.wav",	/* coin in */					/* 0074*/
	"03.wav",	/* eat dot */					/* 0e45*/
	"05.wav",	/* eat dollar sign */			/* 0e45*/

	"06.wav",	/* door open */					/* 19e1*/
	"07.wav",	/* door close */				/* 1965*/

	"08.wav",	/* theme song */				/* 0821*/
	/*"09.wav"									*/ /* 1569*/

	/* one of these two is played after eating the last dot */
	"0a.wav",	/* piccolo */					/* 17af*/
	"0b.wav",	/* tune */						/* 17af*/

	/*"0f.wav"									*/ /* 08ee*/
	"10.wav",	/* 'We're even. Bye Bye!' */	/* 162a*/
	"11.wav",	/* 'You got me!' */				/* 1657*/
	"12.wav",	/* 'You have lost out' */		/* 085e*/

	"13.wav",	/* 'Rock' */					/* 14de*/
	"14.wav",	/* 'Scissors' */				/* 14f3*/
	"15.wav",	/* 'Paper' */					/* 1508*/

	/* one of these is played when going by the girl between levels */
	"16.wav",	/* 'Very good!' */				/* 194a*/
	"17.wav",	/* 'Wonderful!' */				/* 194a*/
	"18.wav",	/* 'Come on!' */				/* 194a*/
	"19.wav",	/* 'I love you!' */				/* 194a*/
	"1a.wav",	/* 'See you again!' */			/* 194a*/
	0       /* end of array */
};

static struct Samplesinterface samples_interface =
{
	4,	/* 4 channels */
	50,	/* volume */
	sample_names
};


static MACHINE_DRIVER_START( gotya )

	/* basic machine hardware */
	MDRV_CPU_ADD(Z80,18432000/6)	/* 3.072 MHz ??? */
	MDRV_CPU_MEMORY(readmem,writemem)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,1)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(36*8, 32*8)
	MDRV_VISIBLE_AREA(0, 36*8-1, 2*8, 30*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(8)
	MDRV_COLORTABLE_LENGTH(16*4)

	MDRV_PALETTE_INIT(gotya)
	MDRV_VIDEO_START(gotya)
	MDRV_VIDEO_UPDATE(gotya)

	/* sound hardware */
	MDRV_SOUND_ADD(SAMPLES, samples_interface)
MACHINE_DRIVER_END

/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( thehand )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )     /* 64k for main CPU */
	ROM_LOAD( "hand6.bin",	0x0000, 0x1000, CRC(a33b806c) SHA1(1e552af5362e7b003f55e78bb59589e1db55557c) )
	ROM_LOAD( "hand5.bin",	0x1000, 0x1000, CRC(89bcde82) SHA1(d074bb6a1975160eb533d5fd9289170a68209046) )
	ROM_LOAD( "hand4.bin",	0x2000, 0x1000, CRC(c6844a83) SHA1(84e220dce3f5ddee9dd0377f3bebdd4027fc9108) )
	ROM_LOAD( "gb-03.bin",	0x3000, 0x1000, CRC(f34d90ab) SHA1(bec5f6a34a273f308083a280f2b425d9c273c69b) )

	ROM_REGION( 0x1000,  REGION_GFX1, ROMREGION_DISPOSE )	/* characters */
	ROM_LOAD( "hand12.bin",	0x0000, 0x1000, CRC(95773b46) SHA1(db8d7ace4eafd4c72edfeff6003ca6e96e0239b5) )

	ROM_REGION( 0x1000,  REGION_GFX2, ROMREGION_DISPOSE )	/* sprites */
	ROM_LOAD( "gb-11.bin",	0x0000, 0x1000, CRC(5d5eca1b) SHA1(d7c6b5f4d398d5e33cc411ed593d6f53a9979493) )

	ROM_REGION( 0x0120,  REGION_PROMS, 0 )
	ROM_LOAD( "prom.1a",    0x0000, 0x0020, CRC(4864a5a0) SHA1(5b49f60b085fa026d4e8d4a5ad28ee7037a8ff9c) )    /* color PROM */
	ROM_LOAD( "prom.4c",    0x0020, 0x0100, CRC(4745b5f6) SHA1(02a7f759e9bc8089cbd9213a71bbe671f9641638) )    /* lookup table */

	ROM_REGION( 0x1000,  REGION_USER1, 0 )		/* no idea what these are */
	ROM_LOAD( "hand1.bin",	0x0000, 0x0800, CRC(ccc537e0) SHA1(471fd49225aa14b91d085178e1b58b6c4ae76481) )
	ROM_LOAD( "gb-02.bin",	0x0800, 0x0800, CRC(65a7e284) SHA1(91e9c34dcf20608863ad5475dc0c4309971c8eee) )

	ROM_REGION( 0x4000,  REGION_USER2, 0 )		/* HD38880 code? */
	ROM_LOAD( "gb-10.bin",	0x0000, 0x1000, CRC(8101915f) SHA1(c4d21b1938ea7e0d47c48e74037f005280ac101b) )
	ROM_LOAD( "gb-09.bin",	0x1000, 0x1000, CRC(619bba76) SHA1(2a2deffe6f058fc840329fbfffbc0c70a0147c14) )
	ROM_LOAD( "gb-08.bin",	0x2000, 0x1000, CRC(82f59528) SHA1(6bfa2329eb291040bfc229c56420865253b0132a) )
	ROM_LOAD( "hand7.bin",	0x3000, 0x1000, CRC(fbf1c5de) SHA1(dd3181a8da1972e3c997678bb868256a10f33d04) )
ROM_END

ROM_START( gotya )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )     /* 64k for main CPU */
	ROM_LOAD( "gb-06.bin",	0x0000, 0x1000, CRC(7793985a) SHA1(23aa8bd161e700bea59b92075423cdf55e9a26c3) )
	ROM_LOAD( "gb-05.bin",	0x1000, 0x1000, CRC(683d188b) SHA1(5341c62f5cf384c73be0d7a0a230bb8cebfbe709) )
	ROM_LOAD( "gb-04.bin",	0x2000, 0x1000, CRC(15b72f09) SHA1(bd941722ed1310d5c8ca8a44899368cba3815f3b) )
	ROM_LOAD( "gb-03.bin",	0x3000, 0x1000, CRC(f34d90ab) SHA1(bec5f6a34a273f308083a280f2b425d9c273c69b) )    /* this is the only ROM that passes the ROM test */

	ROM_REGION( 0x1000,  REGION_GFX1, ROMREGION_DISPOSE )	/* characters */
	ROM_LOAD( "gb-12.bin",	0x0000, 0x1000, CRC(4993d735) SHA1(9e47876238a8af3659721191a5f75c33507ed1a5) )

	ROM_REGION( 0x1000,  REGION_GFX2, ROMREGION_DISPOSE )	/* sprites */
	ROM_LOAD( "gb-11.bin",	0x0000, 0x1000, CRC(5d5eca1b) SHA1(d7c6b5f4d398d5e33cc411ed593d6f53a9979493) )

	ROM_REGION( 0x0120,  REGION_PROMS, 0 )
	ROM_LOAD( "prom.1a",    0x0000, 0x0020, CRC(4864a5a0) SHA1(5b49f60b085fa026d4e8d4a5ad28ee7037a8ff9c) )    /* color PROM */
	ROM_LOAD( "prom.4c",    0x0020, 0x0100, CRC(4745b5f6) SHA1(02a7f759e9bc8089cbd9213a71bbe671f9641638) )    /* lookup table */

	ROM_REGION( 0x1000,  REGION_USER1, 0 )		/* no idea what these are */
	ROM_LOAD( "gb-01.bin",	0x0000, 0x0800, CRC(c31dba64) SHA1(15ae54b7d475ca3f0a3acc45cd8da2916c5fdef2) )
	ROM_LOAD( "gb-02.bin",	0x0800, 0x0800, CRC(65a7e284) SHA1(91e9c34dcf20608863ad5475dc0c4309971c8eee) )

	ROM_REGION( 0x4000,  REGION_USER2, 0 )		/* HD38880 code? */
	ROM_LOAD( "gb-10.bin",	0x0000, 0x1000, CRC(8101915f) SHA1(c4d21b1938ea7e0d47c48e74037f005280ac101b) )
	ROM_LOAD( "gb-09.bin",	0x1000, 0x1000, CRC(619bba76) SHA1(2a2deffe6f058fc840329fbfffbc0c70a0147c14) )
	ROM_LOAD( "gb-08.bin",	0x2000, 0x1000, CRC(82f59528) SHA1(6bfa2329eb291040bfc229c56420865253b0132a) )
	ROM_LOAD( "gb-07.bin",	0x3000, 0x1000, CRC(92a9f8bf) SHA1(9231cd86f24f1e6a585c3a919add50c1f8e42a4c) )
ROM_END

GAME( 1981, thehand, 0,       gotya, gotya, 0, ROT270, "T.I.C."     , "The Hand" )
GAME( 1981, gotya,   thehand, gotya, gotya, 0, ROT270, "Game-A-Tron", "Got-Ya (12-24-1981, prototype[Q])" )
