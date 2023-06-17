/*************************************************************************

	Atari Video Pinball driver

	by Sebastien Monassa (smonassa@mail.dotcom.fr) / overhaul by SJ

	Known issues:

		- plunger doesn't work in test mode - bug in the game code?

*************************************************************************/

#include "driver.h"
#include "artwork.h"

extern UINT8* videopin_video_ram;

extern WRITE_HANDLER( videopin_video_ram_w );
extern WRITE_HANDLER( videopin_ball_w );

extern VIDEO_START( videopin );
extern VIDEO_UPDATE( videopin );

static double time_pushed;
static double time_released;

static UINT8 prev = 0;
static UINT8 mask = 0;

static WRITE_HANDLER(videopin_out1_w);
static WRITE_HANDLER(videopin_out2_w);


static void update_plunger(void)
{
	UINT8 val = readinputport(3);

	if (prev != val)
	{
		if (val == 0)
		{
			time_released = timer_get_time();

			if (!mask)
			{
				cpu_set_nmi_line(0, ASSERT_LINE);
			}
		}
		else
		{
			time_pushed = timer_get_time();
		}

		prev = val;
	}
}


static void interrupt_callback(int scanline)
{
	update_plunger();

	cpu_set_irq_line(0, 0, ASSERT_LINE);

	scanline = scanline + 32;

	if (scanline >= 263)
	{
		scanline = 32;
	}

	timer_set(cpu_getscanlinetime(scanline), scanline, interrupt_callback);
}


static MACHINE_INIT( videopin )
{
	timer_set(cpu_getscanlinetime(32), 32, interrupt_callback);

	/* both output latches are cleared on reset */

	videopin_out1_w(0, 0);
	videopin_out2_w(0, 0);
}


static double calc_plunger_pos(void)
{
	return (timer_get_time() - time_released) * (time_released - time_pushed + 0.2);
}


static READ_HANDLER( videopin_misc_r )
{
	double plunger = calc_plunger_pos();

	/* The plunger of the ball shooter has a black piece of*/
	/* plastic (flag) attached to it. When the plunger flag passes*/
	/* between the first section of the optical coupler, the MPU*/
	/* receives a non-maskable interrupt. When the flag passes*/
	/* between the second section of the optical coupler, the MPU*/
	/* calculates the time between the PLUNGER1 and PLUNGER2*/
	/* signals received. This results in the MPU displaying the*/
	/* ball being shot onto the playfield at a certain speed.*/

	UINT8 val = readinputport(2);

	if (plunger >= 0.000 && plunger <= 0.001)
	{
		val &= ~1;   /* PLUNGER1 */
	}
	if (plunger >= 0.006 && plunger <= 0.007)
	{
		val &= ~2;   /* PLUNGER2 */
	}

	return val;
}


static WRITE_HANDLER( videopin_led_w )
{
	static const char* matrix[8][4] =
	{
		{ "LED26", "LED18", "LED11", "LED13" },
		{ "LED25", "LED17", "LED10", "LED08" },
		{ "LED24", "LED29", "LED09", "LED07" },
		{ "LED23", "LED28", "LED04", "LED06" },
		{ "LED22", "LED27", "LED03", "LED05" },
		{ "LED21", "LED16", "LED02", "-" },
		{ "LED20", "LED15", "LED01", "-" },
		{ "LED19", "LED14", "LED12", "-" }
	};

	int i = (cpu_getscanline() >> 5) & 7;

	artwork_show(matrix[i][0], data & 1);
	artwork_show(matrix[i][1], data & 2);
	artwork_show(matrix[i][2], data & 4);
	artwork_show(matrix[i][3], data & 8);

	if (i == 7)
	{
		set_led_status(0, data & 8);   /* start button */
	}

	cpu_set_irq_line(0, 0, CLEAR_LINE);
}


static WRITE_HANDLER( videopin_out1_w )
{
	/* D0 => OCTAVE0  */
	/* D1 => OCTACE1  */
	/* D2 => OCTAVE2  */
	/* D3 => LOCKOUT  */
	/* D4 => NMIMASK  */
	/* D5 => NOT USED */
	/* D6 => NOT USED */
	/* D7 => NOT USED */

	mask = ~data & 0x10;

	if (mask)
	{
		cpu_set_nmi_line(0, CLEAR_LINE);
	}

	coin_lockout_global_w(~data & 0x08);
}


static WRITE_HANDLER( videopin_out2_w )
{
	/* D0 => VOL0      */
	/* D1 => VOL1      */
	/* D2 => VOL2      */
	/* D3 => NOT USED  */
	/* D4 => COIN CNTR */
	/* D5 => BONG      */
	/* D6 => BELL      */
	/* D7 => ATTRACT   */

	coin_counter_w(0, data & 0x10);
}


static WRITE_HANDLER( videopin_note_dvsr_w )
{
	/* sound not implemented */
}


/*************************************
 *
 *	Main CPU memory handlers
 *
 *************************************/

static MEMORY_READ_START( videopin_readmem )
	{ 0x0000, 0x07ff, MRA_RAM },
	{ 0x0800, 0x0800, videopin_misc_r },
	{ 0x1000, 0x1000, input_port_0_r },
	{ 0x1800, 0x1800, input_port_1_r },
	{ 0x2000, 0x3fff, MRA_ROM },
	{ 0xe000, 0xffff, MRA_ROM },   /* mirror for 6502 vectors */
MEMORY_END


static MEMORY_WRITE_START( videopin_writemem )
	{ 0x0000, 0x01ff, MWA_RAM },
	{ 0x0200, 0x07ff, videopin_video_ram_w, &videopin_video_ram },
	{ 0x0800, 0x0800, videopin_note_dvsr_w },
	{ 0x0801, 0x0801, videopin_led_w },
	{ 0x0802, 0x0802, watchdog_reset_w },
	{ 0x0804, 0x0804, videopin_ball_w },
	{ 0x0805, 0x0805, videopin_out1_w },
	{ 0x0806, 0x0806, videopin_out2_w },
	{ 0x2000, 0x3fff, MWA_ROM },
MEMORY_END



/*************************************
 *
 *	Port definitions
 *
 *************************************/

INPUT_PORTS_START( videopin )
	PORT_START		/* IN0 */
	PORT_BIT ( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT ( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BITX( 0x04, IP_ACTIVE_LOW, IPT_BUTTON1, "Left Flipper", KEYCODE_LCONTROL, IP_JOY_DEFAULT )
	PORT_BITX( 0x08, IP_ACTIVE_LOW, IPT_BUTTON2, "Right Flipper", KEYCODE_RCONTROL, IP_JOY_DEFAULT )
	PORT_BIT ( 0x10, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT ( 0x20, IP_ACTIVE_LOW, IPT_TILT )
	PORT_SERVICE( 0x40, IP_ACTIVE_LOW )
	PORT_BIT ( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START		/* IN1 */
	PORT_DIPNAME( 0xc0, 0x80, DEF_STR( Coinage ) )
	PORT_DIPSETTING(	0xc0, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(	0x80, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(	0x40, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0x30, 0x00, "Language" )
	PORT_DIPSETTING(	0x00, "English" )
	PORT_DIPSETTING(	0x10, "German" )
	PORT_DIPSETTING(	0x20, "French" )
	PORT_DIPSETTING(	0x30, "Spanish" )
	PORT_DIPNAME( 0x08, 0x08, "Balls" )
	PORT_DIPSETTING(	0x08, "3" )
	PORT_DIPSETTING(	0x00, "5" )
	PORT_DIPNAME( 0x04, 0x00, "Replay" )
	PORT_DIPSETTING(	0x04, "Off (award 80000 points instead)" )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x00, "Extra Ball" )
	PORT_DIPSETTING(	0x02, "Off (award 50000 points instead)" )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x01, 0x01, "Replay Level" )
	PORT_DIPSETTING(	0x00, "180000 (3 balls) / 300000 (5 balls)" )
	PORT_DIPSETTING(	0x01, "210000 (3 balls) / 350000 (5 balls)" )

	PORT_START		/* IN2 */
	PORT_BIT ( 0x01, IP_ACTIVE_LOW, IPT_SPECIAL ) /* PLUNGER 1 */
	PORT_BIT ( 0x02, IP_ACTIVE_LOW, IPT_SPECIAL ) /* PLUNGER 2 */
	PORT_BIT ( 0x04, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT ( 0x08, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT ( 0x10, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT ( 0x20, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BITX( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3, "Nudge", KEYCODE_SPACE, IP_JOY_DEFAULT )
	PORT_BIT ( 0x80, IP_ACTIVE_HIGH, IPT_VBLANK )

	PORT_START
	PORT_BITX( 0xff, IP_ACTIVE_HIGH, IPT_BUTTON4, "Ball Shooter", KEYCODE_DOWN, IP_JOY_DEFAULT )
INPUT_PORTS_END



/*************************************
 *
 *	Graphics definitions
 *
 *************************************/

static struct GfxLayout tile_layout =
{
	8, 8,
	64,
	1,
	{ 0 },
	{
		0, 1, 2, 3, 4, 5, 6, 7
	},
	{
		0x00, 0x08, 0x10, 0x18, 0x20, 0x28, 0x30, 0x38
	},
	0x40
};


static struct GfxLayout ball_layout =
{
	16, 16,
	1,
	1,
	{ 0 },
	{
		0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
		0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87

	},
	{
		0x00, 0x08, 0x10, 0x18, 0x20, 0x28, 0x30, 0x38,
		0x40, 0x48, 0x50, 0x58, 0x60, 0x68, 0x70, 0x78
	},
	0x100
};


static struct GfxDecodeInfo videopin_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0x0000, &tile_layout, 0, 1 },
	{ REGION_GFX2, 0x0000, &ball_layout, 0, 1 },
	{ -1 }
};



/*************************************
 *
 *	Machine driver
 *
 *************************************/

static MACHINE_DRIVER_START( videopin )

	/* basic machine hardware */
	MDRV_CPU_ADD(M6502, 12096000 / 16)
	MDRV_CPU_MEMORY(videopin_readmem, videopin_writemem)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(23. * 1000000 / 15750)
	MDRV_MACHINE_INIT(videopin)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(304, 263)
	MDRV_VISIBLE_AREA(0, 303, 0, 255)
	MDRV_GFXDECODE(videopin_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(2)

	MDRV_PALETTE_INIT(black_and_white)
	MDRV_VIDEO_START(videopin)
	MDRV_VIDEO_UPDATE(videopin)

	/* sound hardware */
MACHINE_DRIVER_END



/*************************************
 *
 *	ROM definitions
 *
 *************************************/

ROM_START( videopin )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
    ROM_LOAD_NIB_LOW ( "34242-01.e0", 0x2000, 0x0400, CRC(c6a83795) SHA1(73a65cca7c1e337b336b7d515eafc2981e669be8) )
	ROM_LOAD_NIB_HIGH( "34237-01.k0", 0x2000, 0x0400, CRC(9b5ef087) SHA1(4ecf441742e7c39237cd544b0f0d9339943e1a2c) )
	ROM_LOAD_NIB_LOW ( "34243-01.d0", 0x2400, 0x0400, CRC(dc87d023) SHA1(1ecec121067a60b91b3912bd28737caaae463167) )
	ROM_LOAD_NIB_HIGH( "34238-01.j0", 0x2400, 0x0400, CRC(280d9e67) SHA1(229cc0448bb95f86fc7acbcb9594bc313f316580) )
	ROM_LOAD_NIB_LOW ( "34250-01.h1", 0x2800, 0x0400, CRC(26fdd5a3) SHA1(a5f1624b36f58fcdfc7c6c04784340bb08a89785) )
	ROM_LOAD_NIB_HIGH( "34249-01.h1", 0x2800, 0x0400, CRC(923b3609) SHA1(1b9fc60b27ff80b0ec26d897ea1817f466269506) )
	ROM_LOAD_NIB_LOW ( "34244-01.c0", 0x2c00, 0x0400, CRC(4c12a4b1) SHA1(4351887a8dada92cd24cfa5930456e7c5c251ceb) )
	ROM_LOAD_NIB_HIGH( "34240-01.h0", 0x2c00, 0x0400, CRC(d487eff5) SHA1(45bbf7e693f5471ea25e6ec71ce34708335d2d0b) )
	ROM_LOAD_NIB_LOW ( "34252-01.e1", 0x3000, 0x0400, CRC(4858d87a) SHA1(a50d7ef3d7a804defa25483768cf1a931dd799d5) )
	ROM_LOAD_NIB_HIGH( "34247-01.k1", 0x3000, 0x0400, CRC(d3083368) SHA1(c2083edb0f424dbf02caeaf786ab572326ae48d0) )
	ROM_LOAD_NIB_LOW ( "34246-01.a0", 0x3400, 0x0400, CRC(39ff2d49) SHA1(59221be088d783210516858a4272f7364e00e7b4) )
	ROM_LOAD_NIB_HIGH( "34239-01.h0", 0x3400, 0x0400, CRC(692de455) SHA1(ccbed14cdbeaf23961c356dfac98c6c7fb022486) )
	ROM_LOAD_NIB_LOW ( "34251-01.f1", 0x3800, 0x0400, CRC(5d416efc) SHA1(1debd835cc3e52f526fc0aab4955be7f3682b8c0) )
	ROM_LOAD_NIB_HIGH( "34248-01.j1", 0x3800, 0x0400, CRC(9f120e95) SHA1(d7434f437137690873cba66b408ec8e92b6509c1) )
	ROM_LOAD_NIB_LOW ( "34245-01.b0", 0x3c00, 0x0400, CRC(da02c194) SHA1(a4ec66c85f084286d13a9fc0b35ba5ad896bef44) )
	ROM_RELOAD(                       0xfc00, 0x0400 )
	ROM_LOAD_NIB_HIGH( "34241-01.f0", 0x3c00, 0x0400, CRC(5bfb83da) SHA1(9f392b0d4a972b6ae15ec12913a7e66761f4175d) )
	ROM_RELOAD(                       0xfc00, 0x0400 )

	ROM_REGION( 0x0200, REGION_GFX1, ROMREGION_DISPOSE )	/* tiles */
	ROM_LOAD_NIB_LOW ( "34259-01.d5", 0x0000, 0x0200, CRC(6cd98c06) SHA1(48bf077b7abbd2f529a19bdf85700b93014f39f9) )
	ROM_LOAD_NIB_HIGH( "34258-01.c5", 0x0000, 0x0200, CRC(91a5f117) SHA1(03ac6b0b3da0ed5faf1ba6695d16918d12ceeff5) )

	ROM_REGION( 0x0020, REGION_GFX2, ROMREGION_DISPOSE )	/* ball */
	ROM_LOAD( "34257-01.m1", 0x0000, 0x0020, CRC(50245866) SHA1(b0692bc8d44f127f6e7182a1ce75a785e22ac5b9) )

	ROM_REGION( 0x0100, REGION_PROMS, 0 )
	ROM_LOAD( "9402-01.h4",  0x0000, 0x0100, CRC(b8094b4c) SHA1(82dc6799a19984f3b204ee3aeeb007e55afc8be3) ) /* sync */
ROM_END



/*************************************
 *
 *	Game drivers
 *
 *************************************/

GAMEX( 1979, videopin, 0, videopin, videopin, 0, ROT270, "Atari", "Video Pinball", GAME_NO_SOUND | GAME_IMPERFECT_GRAPHICS )
