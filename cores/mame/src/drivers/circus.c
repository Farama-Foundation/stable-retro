/***************************************************************************

Circus memory map

driver by Mike Coates

0000-00FF Base Page RAM
0100-01FF Stack RAM
1000-1FFF ROM
2000      Clown Verticle Position
3000      Clown Horizontal Position
4000-43FF Video RAM
8000      Clown Rotation and Audio Controls
F000-FFF7 ROM
FFF8-FFFF Interrupt and Reset Vectors

A000      Control Switches
C000      Option Switches
D000      Paddle Position and Interrupt Reset

  CHANGES:
  MAB 09 MAR 99 - changed overlay support to use artwork functions
  AAT 12 MAY 02 - General: fixed red and white hue and removed dirty flags.
                  Ripcord: rewrote video driver, fixed gameplay, flipped
                           control and tuned DIP switches.
                  (Thanks Gregf for introducing these cool classics.)
  JPC 03 OCT 20 - added overlays for circus, ripcord and crash.

***************************************************************************/

#include "driver.h"
#include "artwork.h"
#include "vidhrdw/generic.h"

extern WRITE_HANDLER( circus_clown_x_w );
extern WRITE_HANDLER( circus_clown_y_w );
extern WRITE_HANDLER( circus_clown_z_w );

extern WRITE_HANDLER( circus_videoram_w );

extern VIDEO_START( circus );
extern VIDEO_UPDATE( crash );
extern VIDEO_UPDATE( circus );
extern VIDEO_UPDATE( robotbwl );
extern VIDEO_UPDATE( ripcord ); /*AT*/
extern VIDEO_EOF( ripcord ); /*AT*/

extern INTERRUPT_GEN( crash_interrupt );
extern struct Samplesinterface circus_samples_interface;
#if 0
static int circus_interrupt;

static READ_HANDLER( ripcord_IN2_r )
{
	circus_interrupt ++;
	log_cb(RETRO_LOG_DEBUG, LOGPRE "circus_int: %02x\n", circus_interrupt);
	return readinputport (2);
}
#endif

#define OVERLAY_BLUE		MAKE_ARGB(0x04,0x1f,0x75,0xfe)
#define OVERLAY_GREEN		MAKE_ARGB(0x04,0x20,0xff,0x20)
#define OVERLAY_YELLOW		MAKE_ARGB(0x04,0xff,0xff,0x20)
#define OVERLAY_LT_BLUE		MAKE_ARGB(0x04,0xad,0xd8,0xe6)
#define OVERLAY_BROWN		MAKE_ARGB(0x04,0x65,0x43,0x21)

/* Overlay colours are based on the video shown here: */
/* https://www.youtube.com/watch?v=bFm3b7e2tV8 */
/* The original overlay overlaps the edges, which is authentic to the */
/* real machine, though a little ugly and likely just because of */
/* limited ability to do overlays back in 1977. */
/* For the authentic look, uncomment the definition below. */
/*#define USE_AUTHENTIC_OVERLAY	1 */
OVERLAY_START( circus_overlay )
#ifdef USE_AUTHENTIC_OVERLAY
	OVERLAY_RECT( 0, 20, 248, 36, OVERLAY_BLUE )
	OVERLAY_RECT( 0, 36, 248, 48, OVERLAY_GREEN )
	OVERLAY_RECT( 0, 48, 248, 64, OVERLAY_YELLOW )
#else
	OVERLAY_RECT( 1, 20, 247, 36, OVERLAY_BLUE )
	OVERLAY_RECT( 1, 36, 247, 48, OVERLAY_GREEN )
	OVERLAY_RECT( 1, 48, 247, 64, OVERLAY_YELLOW )
#endif
OVERLAY_END

/* There does not appear to be any known overlays for */
/* this game, so one has been created for completion. */
OVERLAY_START( ripcord_overlay )
	OVERLAY_RECT(   0,   0, 248,  16, OVERLAY_GREEN )
	OVERLAY_RECT(   0, 128,   6, 134, OVERLAY_BROWN )
	OVERLAY_RECT(   0, 134,  10, 138, OVERLAY_BROWN )
	OVERLAY_RECT(   0, 138,  12, 148, OVERLAY_BROWN )
	OVERLAY_RECT(   0, 148,  24, 157, OVERLAY_BROWN )
	OVERLAY_RECT(   0, 157,  20, 165, OVERLAY_BROWN )
	OVERLAY_RECT(   0, 165,  18, 173, OVERLAY_BROWN )
	OVERLAY_RECT(   0, 173,  14, 183, OVERLAY_BROWN )
	OVERLAY_RECT(   0, 183,  16, 196, OVERLAY_BROWN )
	OVERLAY_RECT(   0, 196,  32, 216, OVERLAY_BROWN )
	OVERLAY_RECT(   0, 216,  36, 226, OVERLAY_BROWN )
	OVERLAY_RECT(   0, 226,  34, 235, OVERLAY_BROWN )
	OVERLAY_RECT(   0, 235,  40, 240, OVERLAY_BROWN )
	OVERLAY_RECT(   0, 240,  46, 244, OVERLAY_BROWN )
	OVERLAY_RECT(   0, 244,  64, 256, OVERLAY_BROWN )
	OVERLAY_RECT(  64, 240, 120, 256, OVERLAY_BLUE )
	OVERLAY_RECT( 120, 244, 151, 256, OVERLAY_YELLOW )
	OVERLAY_RECT( 151, 240, 194, 256, OVERLAY_BLUE )
	OVERLAY_RECT( 194, 244, 208, 256, OVERLAY_YELLOW )
	OVERLAY_RECT( 208, 236, 232, 256, OVERLAY_YELLOW )
	OVERLAY_RECT( 232, 240, 248, 256, OVERLAY_YELLOW )
	OVERLAY_RECT( 138, 237, 143, 244, OVERLAY_BROWN )
	OVERLAY_RECT( 135, 229, 149, 237, OVERLAY_GREEN )
	OVERLAY_RECT( 233, 200, 248, 240, OVERLAY_LT_BLUE )
	OVERLAY_RECT( 232, 200, 233, 208, OVERLAY_LT_BLUE )
	OVERLAY_RECT( 224, 204, 232, 208, OVERLAY_LT_BLUE )
OVERLAY_END

/* I can't find any evidence of there ever being an overlay */
/* for this game, but since this seems to be the accepted */
/* overlay often used, I have included it. */
OVERLAY_START( crash_overlay )
	OVERLAY_RECT(   0,   0, 248,  87, OVERLAY_BLUE )
	OVERLAY_RECT(   0,  87,  87, 168, OVERLAY_BLUE )
	OVERLAY_RECT(  87,  87, 160, 168, OVERLAY_YELLOW )
	OVERLAY_RECT( 160,  87, 248, 168, OVERLAY_BLUE )
	OVERLAY_RECT(   0, 168, 248, 256, OVERLAY_BLUE )
OVERLAY_END


static MEMORY_READ_START( readmem )
	{ 0x0000, 0x01ff, MRA_RAM },
	{ 0x1000, 0x1fff, MRA_ROM },
	{ 0x4000, 0x43ff, MRA_RAM },
	{ 0x8000, 0x8000, MRA_RAM },
	{ 0xa000, 0xa000, input_port_0_r },
	{ 0xc000, 0xc000, input_port_1_r }, /* DSW */
	{ 0xd000, 0xd000, input_port_2_r }, /*AT*/
	/*{ 0xd000, 0xd000, ripcord_IN2_r },*/
	{ 0xf000, 0xffff, MRA_ROM },
MEMORY_END

static MEMORY_WRITE_START( writemem )
	{ 0x0000, 0x01ff, MWA_RAM },
	{ 0x1000, 0x1fff, MWA_ROM },
	{ 0x2000, 0x2000, circus_clown_x_w },
	{ 0x3000, 0x3000, circus_clown_y_w },
	{ 0x4000, 0x43ff, circus_videoram_w, &videoram },
	{ 0x8000, 0x8000, circus_clown_z_w },
	{ 0xf000, 0xffff, MWA_ROM },
MEMORY_END


INPUT_PORTS_START( circus )
	PORT_START /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x7c, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_COIN1 )

	PORT_START      /* Dip Switch */
	PORT_DIPNAME( 0x03, 0x00, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x01, "5" )
	PORT_DIPSETTING(    0x02, "7" )
	PORT_DIPSETTING(    0x03, "9" )
	PORT_DIPNAME( 0x0c, 0x04, DEF_STR( Coinage ) )
/*  PORT_DIPSETTING(    0x08, DEF_STR( 2C_1C ) )*/
	PORT_DIPSETTING(    0x0c, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_2C ) )
	PORT_DIPNAME( 0x10, 0x00, "High Score" )
	PORT_DIPSETTING(    0x10, "Credit Awarded" )
	PORT_DIPSETTING(    0x00, "No Award" )
	PORT_DIPNAME( 0x20, 0x00, "Bonus" )
	PORT_DIPSETTING(    0x00, "Single Line" )
	PORT_DIPSETTING(    0x20, "Super Bonus" )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On ) )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_VBLANK )

	PORT_START      /* IN2 - paddle */
	PORT_ANALOG( 0xff, 115, IPT_PADDLE, 30, 10, 64, 167 )
INPUT_PORTS_END


INPUT_PORTS_START( robotbwl )
	PORT_START /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BITX(0x04, IP_ACTIVE_HIGH, 0,"Hook Right", KEYCODE_X, 0 )
	PORT_BITX(0x08, IP_ACTIVE_HIGH, 0,"Hook Left", KEYCODE_Z, 0 )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON2 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_COIN1 )

	PORT_START      /* Dip Switch */
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x01, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x02, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, "Beer Frame" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x04, DEF_STR( On ) )
	PORT_DIPNAME( 0x18, 0x08, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_2C ) )
/*  PORT_DIPSETTING(    0x18, DEF_STR( 1C_2C ) )*/
	PORT_DIPNAME( 0x60, 0x00, "Bowl Timer" )
	PORT_DIPSETTING(    0x00, "3 seconds" )
	PORT_DIPSETTING(    0x20, "5 seconds" )
	PORT_DIPSETTING(    0x40, "7 seconds" )
	PORT_DIPSETTING(    0x60, "9 seconds" )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_VBLANK )
INPUT_PORTS_END


INPUT_PORTS_START( crash )
	PORT_START /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_4WAY )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_4WAY )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1)
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP    | IPF_4WAY )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN  | IPF_4WAY )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_COIN1 )

	PORT_START      /* Dip Switch */
	PORT_DIPNAME( 0x03, 0x01, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "2" )
	PORT_DIPSETTING(    0x01, "3" )
	PORT_DIPSETTING(    0x02, "4" )
	PORT_DIPSETTING(    0x03, "5" )
	PORT_DIPNAME( 0x0C, 0x04, DEF_STR( Coinage ) )
/*	PORT_DIPSETTING(    0x0c, DEF_STR( 2C_1C ) )*/
	PORT_DIPSETTING(    0x08, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_2C ) )
	PORT_DIPNAME( 0x10, 0x00, "High Score" )
	PORT_DIPSETTING(    0x00, "No Award" )
	PORT_DIPSETTING(    0x10, "Credit Awarded" )
	PORT_BIT( 0x60, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_VBLANK )
INPUT_PORTS_END


INPUT_PORTS_START( ripcord )
	PORT_START /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_BUTTON2 )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_COIN1 )

	PORT_START      /* Dip Switch */
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x01, "5" )
	PORT_DIPSETTING(    0x02, "7" )
	PORT_DIPSETTING(    0x03, "9" )
	PORT_DIPNAME( 0x0c, 0x04, DEF_STR( Coinage ) )
/*	PORT_DIPSETTING(    0x0c, DEF_STR( 2C_1C ) )*/
	PORT_DIPSETTING(    0x08, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_2C ) )
	PORT_DIPNAME( 0x10, 0x00, "High Score" )
	PORT_DIPSETTING(    0x10, "Award Credit" )
	PORT_DIPSETTING(    0x00, "No Award" )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_VBLANK )

	PORT_START      /* IN2 - paddle */
	/*PORT_ANALOG( 0xff, 115, IPT_PADDLE, 30, 10, 64, 167 )*/
	PORT_ANALOG( 0xff, 115, IPT_PADDLE|IPF_REVERSE, 30, 10, 64, 167 ) /*AT*/
INPUT_PORTS_END


static struct GfxLayout charlayout =
{
	8,8,    /* 8*8 characters */
	256,    /* 256 characters */
	1,              /* 1 bit per pixel */
	{ 0 },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8     /* every char takes 8 consecutive bytes */
};

static struct GfxLayout clownlayout =
{
	16,16,  /* 16*16 characters */
	16,             /* 16 characters */
	1,              /* 1 bit per pixel */
	{ 0 },
	{ 0, 1, 2, 3, 4, 5, 6, 7,
	  16*8, 16*8+1, 16*8+2, 16*8+3, 16*8+4, 16*8+5, 16*8+6, 16*8+7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
	  8*8, 9*8, 10*8, 11*8, 12*8, 13*8, 14*8, 15*8 },
	16*16   /* every char takes 64 consecutive bytes */
};

static struct GfxLayout robotlayout =
{
	8,8,  /* 16*16 characters */
	1,      /* 1 character */
	1,      /* 1 bit per pixel */
	{ 0 },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &charlayout,  0, 1 },
	{ REGION_GFX2, 0, &clownlayout, 0, 1 },
	{ -1 } /* end of array */
};

static struct GfxDecodeInfo robotbwl_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &charlayout,  0, 1 },
	{ REGION_GFX2, 0, &robotlayout, 0, 1 },
	{ -1 } /* end of array */
};



/***************************************************************************
  Machine drivers
***************************************************************************/
#if 0
static INTERRUPT_GEN( ripcord_interrupt )
{
	circus_interrupt = 0;
}
#endif

static struct DACinterface dac_interface =
{
	1,
	{ 255, 255 }
};

static MACHINE_DRIVER_START( circus )

	/* basic machine hardware */
	MDRV_CPU_ADD(M6502,11289000/16) /* 705.562kHz */
	MDRV_CPU_MEMORY(readmem,writemem)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,1)

	MDRV_FRAMES_PER_SECOND(57)
	MDRV_VBLANK_DURATION(3500)  /* frames per second, vblank duration (complete guess) */

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 31*8-1, 0*8, 32*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(2)

	MDRV_PALETTE_INIT(black_and_white)
	MDRV_VIDEO_START(circus)
	MDRV_VIDEO_UPDATE(circus)

	/* sound hardware */
	MDRV_SOUND_ADD(SAMPLES, circus_samples_interface)
	MDRV_SOUND_ADD(DAC, dac_interface)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( robotbwl )

	/* basic machine hardware */
	MDRV_CPU_ADD(M6502,11289000/16) /* 705.562kHz */
	MDRV_CPU_MEMORY(readmem,writemem)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,1)

	MDRV_FRAMES_PER_SECOND(57)
	MDRV_VBLANK_DURATION(3500)  /* frames per second, vblank duration (complete guess) */

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 31*8-1, 0*8, 32*8-1)
	MDRV_GFXDECODE(robotbwl_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(2)

	MDRV_PALETTE_INIT(black_and_white)
	MDRV_VIDEO_START(circus)
	MDRV_VIDEO_UPDATE(robotbwl)

	/* sound hardware */
	MDRV_SOUND_ADD(DAC, dac_interface)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( crash )

	/* basic machine hardware */
	MDRV_CPU_ADD(M6502,11289000/16) /* 705.562kHz */
	MDRV_CPU_MEMORY(readmem,writemem)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,2)

	MDRV_FRAMES_PER_SECOND(57)
	MDRV_VBLANK_DURATION(3500)  /* frames per second, vblank duration (complete guess) */

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 31*8-1, 0*8, 32*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(2)

	MDRV_PALETTE_INIT(black_and_white)
	MDRV_VIDEO_START(circus)
	MDRV_VIDEO_UPDATE(crash)

	/* sound hardware */
	MDRV_SOUND_ADD(DAC, dac_interface)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( ripcord )

	/* basic machine hardware */
	MDRV_CPU_ADD(M6502, 705562)        /* 11.289MHz / 16 */
	MDRV_CPU_MEMORY(readmem,writemem)
	/*MDRV_CPU_VBLANK_INT(ripcord_interrupt,1) */ /*AT*/

	MDRV_FRAMES_PER_SECOND(57)
	MDRV_VBLANK_DURATION(3500)  /* frames per second, vblank duration (complete guess) */

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 31*8-1, 0*8, 32*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(2)

	MDRV_PALETTE_INIT(black_and_white)
	MDRV_VIDEO_START(circus)
	MDRV_VIDEO_UPDATE(ripcord)

	/* sound hardware */
	MDRV_SOUND_ADD(DAC, dac_interface)
MACHINE_DRIVER_END



ROM_START( circus )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )     /* 64k for code */
	ROM_LOAD( "circus.1a",    0x1000, 0x0200, CRC(7654ea75) SHA1(fa29417618157002b8ecb21f4c15104c8145a742) ) /* Code */
	ROM_LOAD( "circus.2a",    0x1200, 0x0200, CRC(b8acdbc5) SHA1(634bb11089f7a57a316b6829954cc4da4523f267) )
	ROM_LOAD( "circus.3a",    0x1400, 0x0200, CRC(901dfff6) SHA1(c1f48845456e88d54981608afd00ddb92d97da99) )
	ROM_LOAD( "circus.5a",    0x1600, 0x0200, CRC(9dfdae38) SHA1(dc59a5f90a5a49fa071aada67eda768d3ecef010) )
	ROM_LOAD( "circus.6a",    0x1800, 0x0200, CRC(c8681cf6) SHA1(681cfea75bee8a86f9f4645e6c6b94b44762dae9) )
	ROM_LOAD( "circus.7a",    0x1a00, 0x0200, CRC(585f633e) SHA1(46133409f42e8cbc095dde576ce07d97b235972d) )
	ROM_LOAD( "circus.8a",    0x1c00, 0x0200, CRC(69cc409f) SHA1(b77289e62313e8535ce40686df7238aa9c0035bc) )
	ROM_LOAD( "circus.9a",    0x1e00, 0x0200, CRC(aff835eb) SHA1(d6d95510d4a046f48358fef01103bcc760eb71ed) )
	ROM_RELOAD(               0xfe00, 0x0200 ) /* for the reset and interrupt vectors */

	ROM_REGION( 0x0800, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "circus.4c",    0x0000, 0x0200, CRC(6efc315a) SHA1(d5a4a64a901853fff56df3c65512afea8336aad2) )  /* Character Set */
	ROM_LOAD( "circus.3c",    0x0200, 0x0200, CRC(30d72ef5) SHA1(45fc8285e213bf3906a26205a8c0b22f311fd6c3) )
	ROM_LOAD( "circus.2c",    0x0400, 0x0200, CRC(361da7ee) SHA1(6e6fe5b37ccb4c11aa4abbd9b7df772953abfe7e) )
	ROM_LOAD( "circus.1c",    0x0600, 0x0200, CRC(1f954bb3) SHA1(62a958b48078caa639b96f62a690583a1c8e83f5) )

	ROM_REGION( 0x0200, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "circus.14d",   0x0000, 0x0200, CRC(2fde3930) SHA1(a21e2d342f16a39a07edf4bea8d698a52216ecba) )  /* Clown */
ROM_END

ROM_START( robotbwl )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )     /* 64k for code */
	ROM_LOAD( "robotbwl.1a",  0xf000, 0x0200, CRC(df387a0b) SHA1(97291f1a93cbbff987b0fbc16c2e87ad0db96e12) ) /* Code */
	ROM_LOAD( "robotbwl.2a",  0xf200, 0x0200, CRC(c948274d) SHA1(1bf8c6e994d601d4e6d30ca2a9da97e140ff5eee) )
	ROM_LOAD( "robotbwl.3a",  0xf400, 0x0200, CRC(8fdb3ec5) SHA1(a9290edccb8f75e7ec91416d46617516260d5944) )
	ROM_LOAD( "robotbwl.5a",  0xf600, 0x0200, CRC(ba9a6929) SHA1(9cc6e85431b5d82bf3a624f7b35ddec399ad6c80) )
	ROM_LOAD( "robotbwl.6a",  0xf800, 0x0200, CRC(16fd8480) SHA1(935bb0c87d25086f326571c83f94f831b1a8b036) )
	ROM_LOAD( "robotbwl.7a",  0xfa00, 0x0200, CRC(4cadbf06) SHA1(380c10aa83929bfbfd89facb252e68c307545755) )
	ROM_LOAD( "robotbwl.8a",  0xfc00, 0x0200, CRC(bc809ed3) SHA1(2bb4cdae8c9619eebea30cc323960a46a509bb58) )
	ROM_LOAD( "robotbwl.9a",  0xfe00, 0x0200, CRC(07487e27) SHA1(b5528fb3fec474df2b66f36e28df13a7e81f9ce3) )

	ROM_REGION( 0x0800, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "robotbwl.4c",  0x0000, 0x0200, CRC(a5f7acb9) SHA1(556dd34d0fa50415b128477e208e96bf0c050c2c) )  /* Character Set */
	ROM_LOAD( "robotbwl.3c",  0x0200, 0x0200, CRC(d5380c9b) SHA1(b9670e87011a1b3aebd1d386f1fe6a74f8c77be9) )
	ROM_LOAD( "robotbwl.2c",  0x0400, 0x0200, CRC(47b3e39c) SHA1(393c680fba3bd384e2c773150c3bae4d735a91bf) )
	ROM_LOAD( "robotbwl.1c",  0x0600, 0x0200, CRC(b2991e7e) SHA1(32b6d42bb9312d6cbe5b4113fcf2262bfeef3777) )

	ROM_REGION( 0x0020, REGION_GFX2, ROMREGION_DISPOSE | ROMREGION_INVERT )
	ROM_LOAD( "robotbwl.14d", 0x0000, 0x0020, CRC(a402ac06) SHA1(3bd75630786bcc86d9e9fbc826adc909eef9b41f) )  /* Ball */
ROM_END

ROM_START( crash )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )     /* 64k for code */
	ROM_LOAD( "crash.a1",     0x1000, 0x0200, CRC(b9571203) SHA1(1299e476598d07a67aa1640f3320de1198280296) ) /* Code */
	ROM_LOAD( "crash.a2",     0x1200, 0x0200, CRC(b4581a95) SHA1(b3662bda5013443a56eabbe21fefa91e255e18e7) )
	ROM_LOAD( "crash.a3",     0x1400, 0x0200, CRC(597555ae) SHA1(39a6d10e229be0e0d52b1061f2aa2f678b351f0b) )
	ROM_LOAD( "crash.a4",     0x1600, 0x0200, CRC(0a15d69f) SHA1(c3a7b5ce4406cce511108e5c015b1dd5587b75ed) )
	ROM_LOAD( "crash.a5",     0x1800, 0x0200, CRC(a9c7a328) SHA1(2f21ee58ba117bf4fe9101373c55449217a08da6) )
	ROM_LOAD( "crash.a6",     0x1a00, 0x0200, CRC(c7d62d27) SHA1(974800cbeba2f2d0d796200d235371e2ce3a1d28) )
	ROM_LOAD( "crash.a7",     0x1c00, 0x0200, CRC(5e5af244) SHA1(9ea27241a5ac97b260599d56f60bf9ec3ffcac7f) )
	ROM_LOAD( "crash.a8",     0x1e00, 0x0200, CRC(3dc50839) SHA1(5782ea7d70e5cbe8b8245ed1075ce92b57cc6ddf) )
	ROM_RELOAD(               0xfe00, 0x0200 ) /* for the reset and interrupt vectors */

	ROM_REGION( 0x0800, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "crash.c4",     0x0000, 0x0200, CRC(ba16f9e8) SHA1(fdbf8d36993196552ddb7729750420f8e31eee70) )  /* Character Set */
	ROM_LOAD( "crash.c3",     0x0200, 0x0200, CRC(3c8f7560) SHA1(ce4023167a0b4b912bbbc70b00fd3b462990a04c) )
	ROM_LOAD( "crash.c2",     0x0400, 0x0200, CRC(38f3e4ed) SHA1(4e537402c09b58997bc45498fd721d83a0eac3a7) )
	ROM_LOAD( "crash.c1",     0x0600, 0x0200, CRC(e9adf1e1) SHA1(c1f6d2a3be1e9b35c8675d1e3f57e6a85ddd99fd) )

	ROM_REGION( 0x0200, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "crash.d14",    0x0000, 0x0200, CRC(833f81e4) SHA1(78a0ace3510546691ecaf6f6275cb3269495edc9) )  /* Cars */
ROM_END

ROM_START( ripcord )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )     /* 64k for code */
	ROM_LOAD( "9027.1a",      0x1000, 0x0200, CRC(56b8dc06) SHA1(5432e4f2e321805a8dc9cfce20b8372793a9a4dd) ) /* Code */
	ROM_LOAD( "9028.2a",      0x1200, 0x0200, CRC(a8a78a30) SHA1(e6ddcba608f9b34e07a5402872793dafe5054156) )
	ROM_LOAD( "9029.4a",      0x1400, 0x0200, CRC(fc5c8e07) SHA1(4784a868491393f42520f6609266ffab21661ec3) )
	ROM_LOAD( "9030.5a",      0x1600, 0x0200, CRC(b496263c) SHA1(36321aa6d18e7c35461c1d445d2682d61279a8c7) )
	ROM_LOAD( "9031.6a",      0x1800, 0x0200, CRC(cdc7d46e) SHA1(369bb119320cd737641a5bf64d51c9b552578f8a) )
	ROM_LOAD( "9032.7a",      0x1a00, 0x0200, CRC(a6588bec) SHA1(76321ab29329b6291e4d4731bb445a6ac4ce2d86) )
	ROM_LOAD( "9033.8a",      0x1c00, 0x0200, CRC(fd49b806) SHA1(5205ee8e9cec53be6e79e0183bc1e9d96c8c2e55) )
	ROM_LOAD( "9034.9a",      0x1e00, 0x0200, CRC(7caf926d) SHA1(f51d010ce1909e21e04313e4262c70ab948c14e0) )
	ROM_RELOAD(               0xfe00, 0x0200 ) /* for the reset and interrupt vectors */

	ROM_REGION( 0x0800, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "9026.5c",      0x0000, 0x0200, CRC(06e7adbb) SHA1(0c119743eacc30d6d9eb50dfee0746b69bb17377) )  /* Character Set */
	ROM_LOAD( "9025.4c",      0x0200, 0x0200, CRC(3129527e) SHA1(3d0519811c9e4a5645f5c54ed8f0b411cdc5d54b) )
	ROM_LOAD( "9024.2c",      0x0400, 0x0200, CRC(bcb88396) SHA1(d92dff2436f58d977f9196a88fa7701c3032ef7d) )
	ROM_LOAD( "9023.1c",      0x0600, 0x0200, CRC(9f86ed5b) SHA1(fbe38c6d63887e603d919b0ab2216cd44b8955e4) )

	ROM_REGION( 0x0200, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "9035.14d",     0x0000, 0x0200, CRC(c9979802) SHA1(cf6dfad0821fa736c8fcf8735792054858232806) )
ROM_END


static DRIVER_INIT( ripcord )
{
	artwork_set_overlay(ripcord_overlay);
}

static DRIVER_INIT( crash )
{
	artwork_set_overlay(crash_overlay);
}

static DRIVER_INIT( circus )
{
	artwork_set_overlay(circus_overlay);
}


GAME( 1977, circus,   0, circus,   circus,   circus, ROT0, "Exidy", "Circus" )
GAMEX( 1977, robotbwl, 0, robotbwl, robotbwl, 0,      ROT0, "Exidy", "Robot Bowl", GAME_NO_SOUND )
GAMEX( 1979, crash,    0, crash,    crash,    crash,      ROT0, "Exidy", "Crash", GAME_IMPERFECT_SOUND )
GAMEX( 1979, ripcord,  0, ripcord,  ripcord,  ripcord,      ROT0, "Exidy", "Rip Cord", GAME_IMPERFECT_SOUND )
