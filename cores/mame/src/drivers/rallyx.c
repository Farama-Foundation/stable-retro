/***************************************************************************

Rally X memory map (preliminary)

driver by Nicola Salmoria


0000-3fff ROM
8000-83ff Radar video RAM + other
8400-87ff video RAM
8800-8bff Radar color RAM + other
8c00-8fff color RAM
9800-9fff RAM

memory mapped ports:

read:
a000	  IN0
a080	  IN1
a100	  DSW1

write:
8014-801f sprites - 6 pairs: code (including flipping) and X position
8814-881f sprites - 6 pairs: Y position and color
8034-880c radar car indicators x position
8834-883c radar car indicators y position
a004-a00c radar car indicators color and x position MSB
a080	  watchdog reset
a105	  sound voice 1 waveform (nibble)
a111-a113 sound voice 1 frequency (nibble)
a115	  sound voice 1 volume (nibble)
a10a	  sound voice 2 waveform (nibble)
a116-a118 sound voice 2 frequency (nibble)
a11a	  sound voice 2 volume (nibble)
a10f	  sound voice 3 waveform (nibble)
a11b-a11d sound voice 3 frequency (nibble)
a11f	  sound voice 3 volume (nibble)
a130	  virtual screen X scroll position
a140	  virtual screen Y scroll position
a170	  ? this is written to A LOT of times every frame
a180	  explosion sound trigger
a181	  interrupt enable
a182	  ?
a183	  flip screen
a184	  1 player start lamp
a185	  2 players start lamp
a186	  coin lockout
a187	  coin counter

I/O ports:
OUT on port $0 sets the interrupt vector/instruction (the game uses both
IM 2 and IM 0)

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"



WRITE_HANDLER( pengo_sound_w );
extern unsigned char *pengo_soundregs;

extern unsigned char *rallyx_videoram2,*rallyx_colorram2;
extern unsigned char *rallyx_radarx,*rallyx_radary,*rallyx_radarattr;
extern size_t rallyx_radarram_size;
extern unsigned char *rallyx_scrollx,*rallyx_scrolly;
WRITE_HANDLER( rallyx_videoram2_w );
WRITE_HANDLER( rallyx_colorram2_w );
WRITE_HANDLER( rallyx_spriteram_w );
WRITE_HANDLER( rallyx_flipscreen_w );
PALETTE_INIT( rallyx );
VIDEO_START( rallyx );
VIDEO_UPDATE( rallyx );


static WRITE_HANDLER( rallyx_coin_lockout_w )
{
	coin_lockout_w(offset, data ^ 1);
}

static WRITE_HANDLER( rallyx_coin_counter_w )
{
	coin_counter_w(offset, data);
}

static WRITE_HANDLER( rallyx_leds_w )
{
	set_led_status(offset,data & 1);
}

static WRITE_HANDLER( rallyx_play_sound_w )
{
	static int last;


	if (data == 0 && last != 0)
		sample_start(0,0,0);

	last = data;
}

static MEMORY_READ_START( readmem )
	{ 0x0000, 0x3fff, MRA_ROM },
	{ 0x8000, 0x8fff, MRA_RAM },
	{ 0x9800, 0x9fff, MRA_RAM },
	{ 0xa000, 0xa000, input_port_0_r }, /* IN0 */
	{ 0xa080, 0xa080, input_port_1_r }, /* IN1 */
	{ 0xa100, 0xa100, input_port_2_r }, /* DSW1 */
MEMORY_END

static MEMORY_WRITE_START( writemem )
	{ 0x0000, 0x3fff, MWA_ROM },
	{ 0x8000, 0x83ff, videoram_w, &videoram, &videoram_size },
	{ 0x8400, 0x87ff, rallyx_videoram2_w, &rallyx_videoram2 },
	{ 0x8800, 0x8bff, colorram_w, &colorram },
	{ 0x8c00, 0x8fff, rallyx_colorram2_w, &rallyx_colorram2 },
	{ 0x9800, 0x9fff, MWA_RAM },
	{ 0xa004, 0xa00f, MWA_RAM, &rallyx_radarattr },
	{ 0xa080, 0xa080, watchdog_reset_w },
	{ 0xa100, 0xa11f, pengo_sound_w, &pengo_soundregs },
	{ 0xa130, 0xa130, MWA_RAM, &rallyx_scrollx },
	{ 0xa140, 0xa140, MWA_RAM, &rallyx_scrolly },
	/*{ 0xa170, 0xa170, MWA_NOP },	 // ????? /*/
	{ 0xa180, 0xa180, rallyx_play_sound_w },
	{ 0xa181, 0xa181, interrupt_enable_w },
	{ 0xa183, 0xa183, rallyx_flipscreen_w },
	{ 0xa184, 0xa185, rallyx_leds_w },
	{ 0xa186, 0xa186, rallyx_coin_lockout_w },
	{ 0xa187, 0xa187, rallyx_coin_counter_w },
	{ 0x8014, 0x801f, MWA_RAM, &spriteram, &spriteram_size },	/* these are here just to initialize */
	{ 0x8814, 0x881f, MWA_RAM, &spriteram_2 },	/* the pointers. */
	{ 0x8034, 0x803f, MWA_RAM, &rallyx_radarx, &rallyx_radarram_size }, /* ditto */
	{ 0x8834, 0x883f, MWA_RAM, &rallyx_radary },
MEMORY_END

static PORT_WRITE_START( writeport )
	{ 0, 0, interrupt_vector_w },
PORT_END



INPUT_PORTS_START( rallyx )
	PORT_START		/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_4WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT |IPF_4WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_4WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_4WAY )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN1 )

	PORT_START		/* IN1 */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(	0x01, DEF_STR( Upright ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Cocktail ) )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN2 )

	PORT_START		/* DSW0 */
	PORT_SERVICE( 0x01, IP_ACTIVE_LOW )
	/* TODO: the bonus score depends on the number of lives */
	PORT_DIPNAME( 0x06, 0x02, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(	0x02, "A" )
	PORT_DIPSETTING(	0x04, "B" )
	PORT_DIPSETTING(	0x06, "C" )
	PORT_DIPSETTING(	0x00, "None" )
	PORT_DIPNAME( 0x38, 0x08, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(	0x10, "1 Car, Medium" )
	PORT_DIPSETTING(	0x28, "1 Car, Hard" )
	PORT_DIPSETTING(	0x00, "2 Cars, Easy" )
	PORT_DIPSETTING(	0x18, "2 Cars, Medium" )
	PORT_DIPSETTING(	0x30, "2 Cars, Hard" )
	PORT_DIPSETTING(	0x08, "3 Cars, Easy" )
	PORT_DIPSETTING(	0x20, "3 Cars, Medium" )
	PORT_DIPSETTING(	0x38, "3 Cars, Hard" )
	PORT_DIPNAME( 0xc0, 0xc0, DEF_STR( Coinage ) )
	PORT_DIPSETTING(	0x40, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(	0xc0, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(	0x80, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Free_Play ) )
INPUT_PORTS_END

INPUT_PORTS_START( nrallyx )
	PORT_START		/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_4WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT |IPF_4WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_4WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_4WAY )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN1 )

	PORT_START		/* IN1 */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(	0x01, DEF_STR( Upright ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Cocktail ) )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN2 )

	PORT_START		/* DSW0 */
	PORT_SERVICE( 0x01, IP_ACTIVE_LOW )
	/* TODO: the bonus score depends on the number of lives */
	PORT_DIPNAME( 0x06, 0x02, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(	0x02, "A" )
	PORT_DIPSETTING(	0x04, "B" )
	PORT_DIPSETTING(	0x06, "C" )
	PORT_DIPSETTING(	0x00, "None" )
	PORT_DIPNAME( 0x38, 0x00, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(	0x10, "1 Car, Medium" )
	PORT_DIPSETTING(	0x28, "1 Car, Hard" )
	PORT_DIPSETTING(	0x18, "2 Cars, Medium" )
	PORT_DIPSETTING(	0x30, "2 Cars, Hard" )
	PORT_DIPSETTING(	0x00, "3 Cars, Easy" )
	PORT_DIPSETTING(	0x20, "3 Cars, Medium" )
	PORT_DIPSETTING(	0x38, "3 Cars, Hard" )
	PORT_DIPSETTING(	0x08, "4 Cars, Easy" )
	PORT_DIPNAME( 0xc0, 0xc0, DEF_STR( Coinage ) )
	PORT_DIPSETTING(	0x40, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(	0xc0, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(	0x80, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Free_Play ) )
INPUT_PORTS_END

INPUT_PORTS_START( nrallyv )
	PORT_START		/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_4WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN |IPF_4WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_4WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN1 )

	PORT_START		/* IN1 */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(	0x01, DEF_STR( Upright ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Cocktail ) )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN2 )

	PORT_START		/* DSW0 */
	PORT_SERVICE( 0x01, IP_ACTIVE_LOW )
	/* TODO: the bonus score depends on the number of lives */
	PORT_DIPNAME( 0x06, 0x02, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(	0x02, "A" )
	PORT_DIPSETTING(	0x04, "B" )
	PORT_DIPSETTING(	0x06, "C" )
	PORT_DIPSETTING(	0x00, "None" )
	PORT_DIPNAME( 0x38, 0x00, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(	0x10, "1 Car, Medium" )
	PORT_DIPSETTING(	0x28, "1 Car, Hard" )
	PORT_DIPSETTING(	0x18, "2 Cars, Medium" )
	PORT_DIPSETTING(	0x30, "2 Cars, Hard" )
	PORT_DIPSETTING(	0x00, "3 Cars, Easy" )
	PORT_DIPSETTING(	0x20, "3 Cars, Medium" )
	PORT_DIPSETTING(	0x38, "3 Cars, Hard" )
	PORT_DIPSETTING(	0x08, "4 Cars, Easy" )
	PORT_DIPNAME( 0xc0, 0xc0, DEF_STR( Coinage ) )
	PORT_DIPSETTING(	0x40, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(	0xc0, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(	0x80, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Free_Play ) )
INPUT_PORTS_END


static struct GfxLayout charlayout =
{
	8,8,	/* 8*8 characters */
	256,	/* 256 characters */
	2,	/* 2 bits per pixel */
	{ 0, 4 },	/* the two bitplanes for 4 pixels are packed into one byte */
	{ 8*8+0, 8*8+1, 8*8+2, 8*8+3, 0, 1, 2, 3 }, /* bits are packed in groups of four */
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	16*8	/* every char takes 16 bytes */
};
static struct GfxLayout spritelayout =
{
	16,16,	/* 16*16 sprites */
	64, /* 64 sprites */
	2,	/* 2 bits per pixel */
	{ 0, 4 },	/* the two bitplanes for 4 pixels are packed into one byte */
	{ 8*8+0, 8*8+1, 8*8+2, 8*8+3, 16*8+0, 16*8+1, 16*8+2, 16*8+3,	/* bits are packed in groups of four */
			 24*8+0, 24*8+1, 24*8+2, 24*8+3, 0, 1, 2, 3 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
			32*8, 33*8, 34*8, 35*8, 36*8, 37*8, 38*8, 39*8 },
	64*8	/* every sprite takes 64 bytes */
};

static struct GfxLayout dotlayout =
{
	4,4,	/* 4*4 characters */
	8,	/* 8 characters */
	2,	/* 2 bits per pixel */
	{ 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8 },
	{ 0*32, 1*32, 2*32, 3*32 },
	16*8	/* every char takes 16 consecutive bytes */
};



static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &charlayout,		  0, 64 },
	{ REGION_GFX1, 0, &spritelayout,	  0, 64 },
	{ REGION_GFX2, 0, &dotlayout,	   64*4,  1 },
	{ -1 } /* end of array */
};



static struct namco_interface namco_interface =
{
	3072000/32, /* sample rate */
	3,			/* number of voices */
	100,		/* playback volume */
	REGION_SOUND1	/* memory region */
};

static const char *rallyx_sample_names[] =
{
	"*rallyx",
	"bang.wav",
	0	/* end of array */
};

static struct Samplesinterface samples_interface =
{
	1,	/* 1 channel */
	80, /* volume */
	rallyx_sample_names
};



static MACHINE_DRIVER_START( rallyx )

	/* basic machine hardware */
	MDRV_CPU_ADD(Z80, 3072000)	/* 3.072 MHz ? */
	MDRV_CPU_MEMORY(readmem,writemem)
	MDRV_CPU_PORTS(0,writeport)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,1)

	MDRV_FRAMES_PER_SECOND(60.606060)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(36*8, 28*8)
	MDRV_VISIBLE_AREA(0*8, 36*8-1, 0*8, 28*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(32)
	MDRV_COLORTABLE_LENGTH(64*4+4)

	MDRV_PALETTE_INIT(rallyx)
	MDRV_VIDEO_START(rallyx)
	MDRV_VIDEO_UPDATE(rallyx)

	/* sound hardware */
	MDRV_SOUND_ADD(NAMCO_15XX, namco_interface)
	MDRV_SOUND_ADD(SAMPLES, samples_interface)
MACHINE_DRIVER_END



/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( rallyx )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "1b",           0x0000, 0x1000, CRC(5882700d) SHA1(b6029e9730f1694894fe8b729ac0ba8d6712dea9) )
	ROM_LOAD( "rallyxn.1e",   0x1000, 0x1000, CRC(ed1eba2b) SHA1(82d3a4b34b0ff5cfdb8ca7c18ad5c63d943b8484) )
	ROM_LOAD( "rallyxn.1h",   0x2000, 0x1000, CRC(4f98dd1c) SHA1(8a20fadcea76802d1c412ba62086abb846ad54a8) )
	ROM_LOAD( "rallyxn.1k",   0x3000, 0x1000, CRC(9aacccf0) SHA1(9b22079972c0f9970d62d62751db4783a87796d5) )

	ROM_REGION( 0x1000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "8e",           0x0000, 0x1000, CRC(277c1de5) SHA1(30bc57263e8dad870c501c76bce6f42d69ab9e00) )

	ROM_REGION( 0x0100, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "im5623.8m",    0x0000, 0x0100, CRC(3c16f62c) SHA1(7a3800be410e306cf85753b9953ffc5575afbcd6) )  /* dots */

	ROM_REGION( 0x0120, REGION_PROMS, 0 )
	ROM_LOAD( "m3-7603.11n",  0x0000, 0x0020, CRC(c7865434) SHA1(70c1c9610ba6f1ead77f347e7132958958bccb31) )
	ROM_LOAD( "im5623.8p",    0x0020, 0x0100, CRC(834d4fda) SHA1(617864d3df0917a513e8255ad8d96ae7a04da5a1) )

	ROM_REGION( 0x0200, REGION_SOUND1, 0 ) /* sound proms */
	ROM_LOAD( "im5623.3p",    0x0000, 0x0100, CRC(4bad7017) SHA1(3e6da9d798f5e07fa18d6ce7d0b148be98c766d5) )
	ROM_LOAD( "im5623.2m",    0x0100, 0x0100, CRC(77245b66) SHA1(0c4d0bee858b97632411c440bea6948a74759746) )  /* timing - not used */
ROM_END

ROM_START( rallyxm )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "1b",           0x0000, 0x1000, CRC(5882700d) SHA1(b6029e9730f1694894fe8b729ac0ba8d6712dea9) )
	ROM_LOAD( "1e",           0x1000, 0x1000, CRC(786585ec) SHA1(8aa75f10d695f4b3483c4bf7030b733318fd3bf3) )
	ROM_LOAD( "1h",           0x2000, 0x1000, CRC(110d7dcd) SHA1(23e0855c2c9300f2068711d160fcdfaedd07832f) )
	ROM_LOAD( "1k",           0x3000, 0x1000, CRC(473ab447) SHA1(f0a37ccc48c97c53672f754ca2ac37dc0dc91a9f) )

	ROM_REGION( 0x1000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "8e",           0x0000, 0x1000, CRC(277c1de5) SHA1(30bc57263e8dad870c501c76bce6f42d69ab9e00) )

	ROM_REGION( 0x0100, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "im5623.8m",    0x0000, 0x0100, CRC(3c16f62c) SHA1(7a3800be410e306cf85753b9953ffc5575afbcd6) )  /* dots */

	ROM_REGION( 0x0120, REGION_PROMS, 0 )
	ROM_LOAD( "m3-7603.11n",  0x0000, 0x0020, CRC(c7865434) SHA1(70c1c9610ba6f1ead77f347e7132958958bccb31) )
	ROM_LOAD( "im5623.8p",    0x0020, 0x0100, CRC(834d4fda) SHA1(617864d3df0917a513e8255ad8d96ae7a04da5a1) )

	ROM_REGION( 0x0200, REGION_SOUND1, 0 ) /* sound proms */
	ROM_LOAD( "im5623.3p",    0x0000, 0x0100, CRC(4bad7017) SHA1(3e6da9d798f5e07fa18d6ce7d0b148be98c766d5) )
	ROM_LOAD( "im5623.2m",    0x0100, 0x0100, CRC(77245b66) SHA1(0c4d0bee858b97632411c440bea6948a74759746) )  /* timing - not used */
ROM_END

ROM_START( nrallyx )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "nrallyx.1b",   0x0000, 0x1000, CRC(9404c8d6) SHA1(ee7e45c22a2fbf72d3ac5ac26ab1111a22623fc5) )
	ROM_LOAD( "nrallyx.1e",   0x1000, 0x1000, CRC(ac01bf3f) SHA1(8e1a7cce92ef709d18727db6ee7f89936f4b8df8) )
	ROM_LOAD( "nrallyx.1h",   0x2000, 0x1000, CRC(aeba29b5) SHA1(2a6e4568729b83c430bf70e43c4146ad6a556b1b) )
	ROM_LOAD( "nrallyx.1k",   0x3000, 0x1000, CRC(78f17da7) SHA1(1e035746a10f91e898166a58093d45bdb158ae47) )

	ROM_REGION( 0x1000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "nrallyx.8e",   0x0000, 0x1000, CRC(ca7a174a) SHA1(dc553df18c45ba399661122be75b71d6cb54d6a2) )

	ROM_REGION( 0x0100, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "im5623.8m",    0x0000, 0x0100, CRC(3c16f62c) SHA1(7a3800be410e306cf85753b9953ffc5575afbcd6) )    /* dots */

	ROM_REGION( 0x0120, REGION_PROMS, 0 )
	ROM_LOAD( "nrallyx.pr1",  0x0000, 0x0020, CRC(a0a49017) SHA1(494c920a157e9f876d533c1b0146275a366c4989) )
	ROM_LOAD( "nrallyx.pr2",  0x0020, 0x0100, CRC(b2b7ca15) SHA1(e604d58f2f20ebf042f28b01e74eddeacf5baba9) )

	ROM_REGION( 0x0200, REGION_SOUND1, 0 ) /* sound proms */
	ROM_LOAD( "nrallyx.spr",  0x0000, 0x0100, CRC(b75c4e87) SHA1(450f79a5ae09e34f7624d37769815baf93c0028e) )
	ROM_LOAD( "im5623.2m",    0x0100, 0x0100, CRC(77245b66) SHA1(0c4d0bee858b97632411c440bea6948a74759746) )  /* timing - not used */
ROM_END

#if 0
ROM_START( nrallyv )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "nrallyx.1b",   0x0000, 0x1000, CRC(9404c8d6) SHA1(ee7e45c22a2fbf72d3ac5ac26ab1111a22623fc5) )
	ROM_LOAD( "nrallyx.1e",   0x1000, 0x1000, CRC(ac01bf3f) SHA1(8e1a7cce92ef709d18727db6ee7f89936f4b8df8) )
	ROM_LOAD( "nrallyx.1h",   0x2000, 0x1000, CRC(aeba29b5) SHA1(2a6e4568729b83c430bf70e43c4146ad6a556b1b) )
	ROM_LOAD( "nrallyx.1k",   0x3000, 0x1000, CRC(78f17da7) SHA1(1e035746a10f91e898166a58093d45bdb158ae47) )

	ROM_REGION( 0x1000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "nrallyv.8e",   0x0000, 0x1000, CRC(031acfc5) SHA1(7e0df966b7f2be416e22ec2c36bb86425138c203) )

	ROM_REGION( 0x0100, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "im5623.8m",    0x0000, 0x0100, CRC(3c16f62c) SHA1(7a3800be410e306cf85753b9953ffc5575afbcd6) )    /* dots */

	ROM_REGION( 0x0120, REGION_PROMS, 0 )
	ROM_LOAD( "nrallyx.pr1",  0x0000, 0x0020, CRC(a0a49017) SHA1(494c920a157e9f876d533c1b0146275a366c4989) )
	ROM_LOAD( "nrallyx.pr2",  0x0020, 0x0100, CRC(b2b7ca15) SHA1(e604d58f2f20ebf042f28b01e74eddeacf5baba9) )

	ROM_REGION( 0x0200, REGION_SOUND1, 0 ) /* sound proms */
	ROM_LOAD( "nrallyx.spr",  0x0000, 0x0100, CRC(b75c4e87) SHA1(450f79a5ae09e34f7624d37769815baf93c0028e) )
	ROM_LOAD( "im5623.2m",    0x0100, 0x0100, CRC(77245b66) SHA1(0c4d0bee858b97632411c440bea6948a74759746) )  /* timing - not used */
ROM_END
#endif

GAME( 1980, rallyx,  0,       rallyx, rallyx,  0, ROT0, "Namco", "Rally X" )
GAME( 1980, rallyxm, rallyx,  rallyx, rallyx,  0, ROT0, "[Namco] (Midway license)", "Rally X (Midway)" )
GAME( 1981, nrallyx, 0,       rallyx, nrallyx, 0, ROT0, "Namco", "New Rally X" )
/*GAME( 1981, nrallyv, nrallyx, rallyx, nrallyv, 0, ROT90, "hack", "New Rally X (Vertical Screen)" )*/
