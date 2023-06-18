/***************************************************************************

	Atari Crystal Castles hardware

	driver by Pat Lawrence

	Games supported:
		* Crystal Castles (1983) [3 sets]

	Known issues:
		* none at this time

****************************************************************************

	Crystal Castles memory map.

	 Address  A A A A A A A A A A A A A A A A  R  D D D D D D D D  Function
	          1 1 1 1 1 1 9 8 7 6 5 4 3 2 1 0  /  7 6 5 4 3 2 1 0
	          5 4 3 2 1 0                      W
	-------------------------------------------------------------------------------
	0000      X X X X X X X X X X X X X X X X  W  X X X X X X X X  X Coordinate
	0001      0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 1  W  D D D D D D D D  Y Coordinate
	0002      0 0 0 0 0 0 0 0 0 0 0 0 0 0 1 0 R/W D D D D          Bit Mode
	0003-0BFF 0 0 0 0 A A A A A A A A A A A A R/W D D D D D D D D  RAM (DRAM)
	0C00-7FFF 0 A A A A A A A A A A A A A A A R/W D D D D D D D D  Screen RAM
	8000-8DFF 1 0 0 0 A A A A A A A A A A A A R/W D D D D D D D D  RAM (STATIC)
	8E00-8EFF 1 0 0 0 1 1 1 0 A A A A A A A A R/W D D D D D D D D  MOB BUF 2
	-------------------------------------------------------------------------------
	8F00-8FFF 1 0 0 0 1 1 1 1 A A A A A A A A R/W D D D D D D D D  MOB BUF 1
	                                      0 0 R/W D D D D D D D D  MOB Picture
	                                      0 1 R/W D D D D D D D D  MOB Vertical
	                                      1 0 R/W D D D D D D D D  MOB Priority
	                                      1 1 R/W D D D D D D D D  MOB Horizontal
	-------------------------------------------------------------------------------
	9000-90FF 1 0 0 1 0 0 X X A A A A A A A A R/W D D D D D D D D  NOVRAM
	9400-9401 1 0 0 1 0 1 0 X X X X X X X 0 A  R                   TRAK-BALL 1
	9402-9403 1 0 0 1 0 1 0 X X X X X X X 1 A  R                   TRAK-BALL 2
	9500-9501 1 0 0 1 0 1 0 X X X X X X X X A  R                   TRAK-BALL 1 mirror
	9600      1 0 0 1 0 1 1 X X X X X X X X X  R                   IN0
	                                           R                D  COIN R
	                                           R              D    COIN L
	                                           R            D      COIN AUX
	                                           R          D        SLAM
	                                           R        D          SELF TEST
	                                           R      D            VBLANK
	                                           R    D              JMP1
	                                           R  D                JMP2
	-------------------------------------------------------------------------------
	9800-980F 1 0 0 1 1 0 0 X X X X X A A A A R/W D D D D D D D D  CI/O 0
	9A00-9A0F 1 0 0 1 1 0 1 X X X X X A A A A R/W D D D D D D D D  CI/O 1
	9A08                                                    D D D  Option SW
	                                                      D        SPARE
	                                                    D          SPARE
	                                                  D            SPARE
	9C00      1 0 0 1 1 1 0 0 0 X X X X X X X  W                   RECALL
	-------------------------------------------------------------------------------
	9C80      1 0 0 1 1 1 0 0 1 X X X X X X X  W  D D D D D D D D  H Scr Ctr Load
	9D00      1 0 0 1 1 1 0 1 0 X X X X X X X  W  D D D D D D D D  V Scr Ctr Load
	9D80      1 0 0 1 1 1 0 1 1 X X X X X X X  W                   Int. Acknowledge
	9E00      1 0 0 1 1 1 1 0 0 X X X X X X X  W                   WDOG
	          1 0 0 1 1 1 1 0 1 X X X X A A A  W                D  OUT0
	9E80                                0 0 0  W                D  Trak Ball Light P1
	9E81                                0 0 1  W                D  Trak Ball Light P2
	9E82                                0 1 0  W                D  Store Low
	9E83                                0 1 1  W                D  Store High
	9E84                                1 0 0  W                D  Spare
	9E85                                1 0 1  W                D  Coin Counter R
	9E86                                1 1 0  W                D  Coin Counter L
	9E87                                1 1 1  W                D  BANK0-BANK1
	          1 0 0 1 1 1 1 1 0 X X X X A A A  W          D        OUT1
	9F00                                0 0 0  W          D        ^AX
	9F01                                0 0 1  W          D        ^AY
	9F02                                0 1 0  W          D        ^XINC
	9F03                                0 1 1  W          D        ^YINC
	9F04                                1 0 0  W          D        PLAYER2 (flip screen)
	9F05                                1 0 1  W          D        ^SIRE
	9F06                                1 1 0  W          D        BOTHRAM
	9F07                                1 1 1  W          D        BUF1/^BUF2 (sprite bank)
	9F80-9FBF 1 0 0 1 1 1 1 1 1 X A A A A A A  W  D D D D D D D D  COLORAM
	A000-FFFF 1 A A A A A A A A A A A A A A A  R  D D D D D D D D  Program ROM

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "ccastles.h"



/*************************************
 *
 *	Output ports
 *
 *************************************/

static WRITE_HANDLER( ccastles_led_w )
{
	set_led_status(offset,~data & 1);
}


static WRITE_HANDLER( ccastles_coin_counter_w )
{
	/* this is not working, haven't investigated why */
	coin_counter_w(offset^1, ~data);
}


static WRITE_HANDLER( ccastles_bankswitch_w )
{
	unsigned char *RAM = memory_region(REGION_CPU1);


	if (data) { cpu_setbank(1,&RAM[0x10000]); }
	else { cpu_setbank(1,&RAM[0xa000]); }
}


static WRITE_HANDLER( flip_screen_w )
{
	flip_screen_set(data);
}



/*************************************
 *
 *	Main CPU memory handlers
 *
 *************************************/

static MEMORY_READ_START( readmem )
	{ 0x0002, 0x0002, ccastles_bitmode_r },
	{ 0x0000, 0x8fff, MRA_RAM },
	{ 0x9000, 0x90ff, MRA_RAM },
	{ 0x9400, 0x9400, input_port_2_r },	/* trackball y - player 1 */
	{ 0x9402, 0x9402, input_port_2_r },	/* trackball y - player 2 */
	{ 0x9500, 0x9500, input_port_2_r },	/* trackball y - player 1 mirror */
	{ 0x9401, 0x9401, input_port_3_r },	/* trackball x - player 1 */
	{ 0x9403, 0x9403, input_port_3_r },	/* trackball x - player 2 */
	{ 0x9501, 0x9501, input_port_3_r },	/* trackball x - player 1 mirror */
	{ 0x9600, 0x9600, input_port_0_r },	/* IN0 */
	{ 0x9800, 0x980f, pokey1_r }, /* Random # generator on a Pokey */
	{ 0x9a00, 0x9a0f, pokey2_r }, /* Random #, IN1 */
	{ 0xa000, 0xdfff, MRA_BANK1 },
	{ 0xe000, 0xffff, MRA_ROM },	/* ROMs/interrupt vectors */
MEMORY_END


static MEMORY_WRITE_START( writemem )
	{ 0x0000, 0x0001, MWA_RAM, &ccastles_screen_addr },
	{ 0x0002, 0x0002, ccastles_bitmode_w },
	{ 0x0003, 0x0bff, MWA_RAM },
	{ 0x0c00, 0x7fff, MWA_RAM, &videoram },
	{ 0x8000, 0x8dff, MWA_RAM },
	{ 0x8e00, 0x8eff, MWA_RAM, &spriteram_2, &spriteram_size },
	{ 0x8f00, 0x8fff, MWA_RAM, &spriteram },
	{ 0x9000, 0x90ff, MWA_RAM, &generic_nvram, &generic_nvram_size },
	{ 0x9800, 0x980f, pokey1_w },
	{ 0x9a00, 0x9a0f, pokey2_w },
	{ 0x9c80, 0x9c80, MWA_RAM, &ccastles_scrollx },
	{ 0x9d00, 0x9d00, MWA_RAM, &ccastles_scrolly },
	{ 0x9d80, 0x9d80, MWA_NOP },
	{ 0x9e00, 0x9e00, watchdog_reset_w },
	{ 0x9e80, 0x9e81, ccastles_led_w },
	{ 0x9e85, 0x9e86, ccastles_coin_counter_w },
	{ 0x9e87, 0x9e87, ccastles_bankswitch_w },
	{ 0x9f00, 0x9f01, MWA_RAM, &ccastles_screen_inc_enable },
	{ 0x9f02, 0x9f03, MWA_RAM, &ccastles_screen_inc },
	{ 0x9f04, 0x9f04, flip_screen_w },
	{ 0x9f05, 0x9f06, MWA_RAM },
	{ 0x9f07, 0x9f07, MWA_RAM, &ccastles_sprite_bank },
	{ 0x9f80, 0x9fbf, ccastles_paletteram_w },
	{ 0xa000, 0xffff, MWA_ROM },
MEMORY_END



/*************************************
 *
 *	Port definitions
 *
 *************************************/

INPUT_PORTS_START( ccastles )
	PORT_START	/* IN0 */
	PORT_BIT ( 0x01, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT ( 0x02, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT ( 0x04, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT ( 0x08, IP_ACTIVE_LOW, IPT_TILT )
	PORT_SERVICE( 0x10, IP_ACTIVE_LOW )
	PORT_BIT ( 0x20, IP_ACTIVE_HIGH, IPT_VBLANK )
	PORT_BIT ( 0x40, IP_ACTIVE_LOW, IPT_BUTTON1 )				/* 1p Jump, non-cocktail start1 */
	PORT_BIT ( 0x80, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )		/* 2p Jump, non-cocktail start2 */

	PORT_START	/* IN1 */
	PORT_BIT ( 0x07, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT ( 0x08, IP_ACTIVE_LOW, IPT_START1 )				/* cocktail only */
	PORT_BIT ( 0x10, IP_ACTIVE_LOW, IPT_START2 )				/* cocktail only */
	PORT_DIPNAME( 0x20, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING (   0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING (   0x20, DEF_STR( Cocktail ) )
	PORT_BIT ( 0xc0, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START	/* IN2 */
	PORT_ANALOG( 0xff, 0x7f, IPT_TRACKBALL_Y | IPF_REVERSE, 10, 30, 0, 0 )

	PORT_START	/* IN3 */
	PORT_ANALOG( 0xff, 0x7f, IPT_TRACKBALL_X, 10, 30, 0, 0 )
INPUT_PORTS_END

INPUT_PORTS_START( ccastlej )
	PORT_START	/* IN0 */
	PORT_BIT ( 0x01, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT ( 0x02, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT ( 0x04, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT ( 0x08, IP_ACTIVE_LOW, IPT_TILT )
	PORT_SERVICE( 0x10, IP_ACTIVE_LOW )
	PORT_BIT ( 0x20, IP_ACTIVE_HIGH, IPT_VBLANK )
	PORT_BIT ( 0x40, IP_ACTIVE_LOW, IPT_BUTTON1 )				/* 1p Jump, non-cocktail start1 */
	PORT_BIT ( 0x80, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )		/* 2p Jump, non-cocktail start2 */

	PORT_START	/* IN1 */
	PORT_BIT ( 0x07, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT ( 0x08, IP_ACTIVE_LOW, IPT_START1 )				/* cocktail only */
	PORT_BIT ( 0x10, IP_ACTIVE_LOW, IPT_START2 )				/* cocktail only */
	PORT_DIPNAME( 0x20, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING (   0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING (   0x20, DEF_STR( Cocktail ) )
	PORT_BIT ( 0xc0, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT )
	PORT_BIT( 0xf0, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END


/*************************************
 *
 *	Graphics definitions
 *
 *************************************/

static struct GfxLayout ccastles_spritelayout =
{
	8,16,
	256,
	4,
	{ 0x2000*8+0, 0x2000*8+4, 0, 4 },
	{ 0, 1, 2, 3, 8+0, 8+1, 8+2, 8+3 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16,
			8*16, 9*16, 10*16, 11*16, 12*16, 13*16, 14*16, 15*16 },
	32*8
};


static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0x0000, &ccastles_spritelayout,  0, 1 },
	{ -1 }
};



/*************************************
 *
 *	Sound interfaces
 *
 *************************************/

static struct POKEYinterface pokey_interface =
{
	2,	/* 2 chips */
	1250000,	/* 1.25 MHz??? */
	{ 50, 50 },
	/* The 8 pot handlers */
	{ 0, 0 },
	{ 0, 0 },
	{ 0, 0 },
	{ 0, 0 },
	{ 0, 0 },
	{ 0, 0 },
	{ 0, 0 },
	{ 0, 0 },
	/* The allpot handler */
	{ 0, input_port_1_r },
};



/*************************************
 *
 *	Machine driver
 *
 *************************************/

static MACHINE_DRIVER_START( ccastles )

	/* basic machine hardware */
	MDRV_CPU_ADD(M6502,1500000)
	MDRV_CPU_MEMORY(readmem,writemem)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,4)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)

	MDRV_NVRAM_HANDLER(generic_0fill)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(256, 232)
	MDRV_VISIBLE_AREA(0, 255, 0, 231)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(32)

	MDRV_VIDEO_START(ccastles)
	MDRV_VIDEO_UPDATE(ccastles)

	/* sound hardware */
	MDRV_SOUND_ADD(POKEY, pokey_interface)
MACHINE_DRIVER_END



/*************************************
 *
 *	ROM definitions
 *
 *************************************/

ROM_START( ccastles )
     ROM_REGION( 0x14000, REGION_CPU1, 0 )	/* 64k for code */
     ROM_LOAD( "022-403.bin",  0x0a000, 0x2000, CRC(81471ae5) SHA1(8ec13b48119ecf8fe85207403c0a0de5240cded4) )
     ROM_LOAD( "022-404.bin",  0x0c000, 0x2000, CRC(820daf29) SHA1(a2cff00e9ddce201344692b75038431e4241fedd) )
     ROM_LOAD( "022-405.bin",  0x0e000, 0x2000, CRC(4befc296) SHA1(2e789a32903808014e9d5f3021d7eff57c3e2212) )
     ROM_LOAD( "ccastles.102", 0x10000, 0x2000, CRC(f6ccfbd4) SHA1(69c3da2cbefc5e03a77357e817e3015da5d8334a) )	/* Bank switched ROMs */
     ROM_LOAD( "ccastles.101", 0x12000, 0x2000, CRC(e2e17236) SHA1(81fa95b4d9beacb06d6b4afdf346d94117396557) )	/* containing level data. */

     ROM_REGION( 0x4000, REGION_GFX1, ROMREGION_DISPOSE )
     ROM_LOAD( "ccastles.107", 0x0000, 0x2000, CRC(39960b7d) SHA1(82bdf764ac23e72598883283c5e957169387abd4) )
     ROM_LOAD( "ccastles.106", 0x2000, 0x2000, CRC(9d1d89fc) SHA1(01c279edee322cc28f34506c312e4a9e3363b1be) )
ROM_END


ROM_START( ccastle3 )
     ROM_REGION( 0x14000, REGION_CPU1, 0 )	/* 64k for code */
     ROM_LOAD( "ccastles.303", 0x0a000, 0x2000, CRC(10e39fce) SHA1(5247f52e14ccf39f0ec699a39c8ebe35e61e07d2) )
     ROM_LOAD( "ccastles.304", 0x0c000, 0x2000, CRC(74510f72) SHA1(d22550f308ff395d51869b52449bc0669a4e35e4) )
     ROM_LOAD( "ccastles.305", 0x0e000, 0x2000, CRC(9418cf8a) SHA1(1f835db94270e4a16e721b2ac355fb7e7c052285) )
     ROM_LOAD( "ccastles.102", 0x10000, 0x2000, CRC(f6ccfbd4) SHA1(69c3da2cbefc5e03a77357e817e3015da5d8334a) )	/* Bank switched ROMs */
     ROM_LOAD( "ccastles.101", 0x12000, 0x2000, CRC(e2e17236) SHA1(81fa95b4d9beacb06d6b4afdf346d94117396557) )	/* containing level data. */

     ROM_REGION( 0x4000, REGION_GFX1, ROMREGION_DISPOSE )
     ROM_LOAD( "ccastles.107", 0x0000, 0x2000, CRC(39960b7d) SHA1(82bdf764ac23e72598883283c5e957169387abd4) )
     ROM_LOAD( "ccastles.106", 0x2000, 0x2000, CRC(9d1d89fc) SHA1(01c279edee322cc28f34506c312e4a9e3363b1be) )
ROM_END


ROM_START( ccastle2 )
     ROM_REGION( 0x14000, REGION_CPU1, 0 )	/* 64k for code */
     ROM_LOAD( "ccastles.203", 0x0a000, 0x2000, CRC(348a96f0) SHA1(76de7bf6a01ccb15a4fe7333c1209f623a2e0d1b) )
     ROM_LOAD( "ccastles.204", 0x0c000, 0x2000, CRC(d48d8c1f) SHA1(8744182a3e2096419de63e341feb77dd8a8bcb34) )
     ROM_LOAD( "ccastles.205", 0x0e000, 0x2000, CRC(0e4883cc) SHA1(a96abbf654e087409a90c1686d9dd553bd08c14e) )
     ROM_LOAD( "ccastles.102", 0x10000, 0x2000, CRC(f6ccfbd4) SHA1(69c3da2cbefc5e03a77357e817e3015da5d8334a) )	/* Bank switched ROMs */
     ROM_LOAD( "ccastles.101", 0x12000, 0x2000, CRC(e2e17236) SHA1(81fa95b4d9beacb06d6b4afdf346d94117396557) )	/* containing level data. */

     ROM_REGION( 0x4000, REGION_GFX1, ROMREGION_DISPOSE )
     ROM_LOAD( "ccastles.107", 0x0000, 0x2000, CRC(39960b7d) SHA1(82bdf764ac23e72598883283c5e957169387abd4) )
     ROM_LOAD( "ccastles.106", 0x2000, 0x2000, CRC(9d1d89fc) SHA1(01c279edee322cc28f34506c312e4a9e3363b1be) )
ROM_END

ROM_START( ccastlej )
	ROM_REGION( 0x14000, REGION_CPU1, 0 )
	ROM_LOAD( "a000.12m",      0x0a000, 0x2000, CRC(0d911ef4) SHA1(fbd6a5a0e4e865421ed3720aa61221d03583f248) )
	ROM_LOAD( "c000.13m",      0x0c000, 0x2000, CRC(246079de) SHA1(ade2c63656339c3e7e634470a17bc30da1006979) )
	ROM_LOAD( "e000.14m",      0x0e000, 0x2000, CRC(3beec4f3) SHA1(076caffe67910bdcd1f51a41f2cc4ebdb930c7ca) )
    ROM_LOAD( "ccastles.102",  0x10000, 0x2000, CRC(f6ccfbd4) SHA1(69c3da2cbefc5e03a77357e817e3015da5d8334a) )	/* Bank switched ROMs */
    ROM_LOAD( "ccastles.101",  0x12000, 0x2000, CRC(e2e17236) SHA1(81fa95b4d9beacb06d6b4afdf346d94117396557) )	/* containing level data. */

    ROM_REGION( 0x4000, REGION_GFX1, ROMREGION_DISPOSE )
    ROM_LOAD( "ccastles.107",  0x0000,  0x2000, CRC(39960b7d) SHA1(82bdf764ac23e72598883283c5e957169387abd4) )
    ROM_LOAD( "ccastles.106",  0x2000,  0x2000, CRC(9d1d89fc) SHA1(01c279edee322cc28f34506c312e4a9e3363b1be) )
ROM_END


/*************************************
 *
 *	Game drivers
 *
 *************************************/

GAME( 1983, ccastles, 0,        ccastles, ccastles, 0, ROT0, "Atari", "Crystal Castles (version 4)" )
GAME( 1983, ccastle3, ccastles, ccastles, ccastles, 0, ROT0, "Atari", "Crystal Castles (version 3)" )
GAME( 1983, ccastle2, ccastles, ccastles, ccastles, 0, ROT0, "Atari", "Crystal Castles (version 2)" )
GAME( 1983, ccastlej, ccastles, ccastles, ccastlej, 0, ROT0, "Atari", "Crystal Castles (joystick version)" )
