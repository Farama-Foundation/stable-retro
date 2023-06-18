/***************************************************************************

Atari Sprint 4 driver

  set 1 => large car sprites [12x12 pixels]
  set 2 => small car sprites [11x11 pixels]

***************************************************************************/

#include "driver.h"

extern VIDEO_EOF( sprint4 );
extern VIDEO_START( sprint4 );
extern VIDEO_UPDATE( sprint4 );

extern WRITE_HANDLER( sprint4_video_ram_w );

extern UINT8* sprint4_video_ram;

extern int sprint4_collision[4];

static int analog;
static int steer_dir[4];
static int steer_flag[4];
static int gear[4];


static PALETTE_INIT( sprint4 )
{
	palette_set_color(0, 0x00, 0x00, 0x00); /* black  */
	palette_set_color(1, 0xfc, 0xdf, 0x80); /* peach  */
	palette_set_color(2, 0xf0, 0x00, 0xf0); /* violet */
	palette_set_color(3, 0x00, 0xf0, 0x0f); /* green  */
	palette_set_color(4, 0x30, 0x4f, 0xff); /* blue   */
	palette_set_color(5, 0xff, 0xff, 0xff); /* white  */

	memset(colortable, 0, 10 * sizeof (colortable[0]));

	colortable[1] = 1;
	colortable[3] = 2;
	colortable[5] = 3;
	colortable[7] = 4;
	colortable[9] = 5;
}


static void input_callback(int dummy)
{
	static UINT8 dial[4];

	/* handle steering wheels and gear shift levers */

	int i;

	for (i = 0; i < 4; i++)
	{
		signed char delta = readinputport(5 + i) - dial[i];

		if (delta < 0)
		{
			steer_dir[i] = 0;
		}
		if (delta > 0)
		{
			steer_dir[i] = 1;
		}

		steer_flag[i] = dial[i] & 1;

		switch (readinputport(9 + i))
		{
		case 1: gear[i] = 1; break;
		case 2: gear[i] = 2; break;
		case 4: gear[i] = 3; break;
		case 8: gear[i] = 4; break;
		}

		dial[i] += delta;
	}
}


static void nmi_callback(int scanline)
{
	scanline += 64;

	if (scanline >= 262)
	{
		scanline = 32;
	}

	/* NMI is disabled during service mode */

	if (readinputport(2) & 0x40)
	{
		cpu_set_nmi_line(0, PULSE_LINE);
	}

	timer_set(cpu_getscanlinetime(scanline), scanline, nmi_callback);
}


static MACHINE_INIT( sprint4 )
{
	timer_set(cpu_getscanlinetime(32), 32, nmi_callback);

	timer_pulse(TIME_IN_HZ(60), 0, input_callback);
}


static READ_HANDLER( sprint4_wram_r )
{
	return sprint4_video_ram[0x380 + offset % 0x80];
}


static READ_HANDLER( sprint4_analog_r )
{
	int n = (offset >> 1) & 3;

	UINT8 val;

	if (offset & 1)
	{
		val = 4 * gear[n];
	}
	else
	{
		val = 8 * steer_flag[n] + 8 * steer_dir[n];
	}

	return val > analog ? 0x80 : 0x00;
}


static READ_HANDLER( sprint4_coin_r )
{
	return (readinputport(1) << ((offset & 7) ^ 7)) & 0x80;
}


static READ_HANDLER( sprint4_gas_r )
{
	UINT8 val = readinputport(0);

	if (sprint4_collision[0]) val |= 0x02;
	if (sprint4_collision[1]) val |= 0x08;
	if (sprint4_collision[2]) val |= 0x20;
	if (sprint4_collision[3]) val |= 0x80;

	return (val << ((offset & 7) ^ 7)) & 0x80;
}


static READ_HANDLER( sprint4_dip_r )
{
	return (readinputport(4) >> (2 * (offset & 3))) & 3;
}


static WRITE_HANDLER( sprint4_wram_w )
{
	sprint4_video_ram[0x380 + offset % 0x80] = data;
}


static WRITE_HANDLER( sprint4_collision_reset_w )
{
	sprint4_collision[(offset >> 1) & 3] = 0;
}


static WRITE_HANDLER( sprint4_analog_w )
{
	analog = data & 15;
}


static WRITE_HANDLER( sprint4_lamp_w )
{
	set_led_status((offset >> 1) & 3, offset & 1);
}


static WRITE_HANDLER( sprint4_attract_w )
{
	/* sound */
}
static WRITE_HANDLER( sprint4_crash_w )
{
	/* sound */
}
static WRITE_HANDLER( sprint4_skid_w )
{
	/* sound */
}


static MEMORY_READ_START( readmem )
	{ 0x0000, 0x001f, sprint4_analog_r },
	{ 0x0020, 0x003f, sprint4_coin_r },
	{ 0x0040, 0x005f, sprint4_gas_r },
	{ 0x0060, 0x007f, sprint4_dip_r },
	{ 0x0080, 0x00ff, sprint4_wram_r },
	{ 0x0180, 0x01ff, sprint4_wram_r },
	{ 0x0800, 0x0bff, MRA_RAM },
	{ 0x1000, 0x17ff, input_port_2_r },
	{ 0x1800, 0x1fff, input_port_3_r },
	{ 0x2000, 0x3fff, MRA_ROM },
	{ 0xe800, 0xffff, MRA_ROM },
MEMORY_END


static MEMORY_WRITE_START( writemem )
	{ 0x0000, 0x001f, sprint4_attract_w },
	{ 0x0020, 0x003f, sprint4_collision_reset_w },
	{ 0x0040, 0x0041, sprint4_analog_w },
	{ 0x0042, 0x0043, sprint4_crash_w },
	{ 0x0044, 0x0045, MWA_NOP }, /* watchdog, disabled during service mode */
	{ 0x0046, 0x0047, MWA_NOP }, /* SPARE */
	{ 0x0060, 0x0067, sprint4_lamp_w },
	{ 0x0068, 0x006f, sprint4_skid_w },
	{ 0x0080, 0x00ff, sprint4_wram_w },
	{ 0x0180, 0x01ff, sprint4_wram_w },
	{ 0x0800, 0x0bff, sprint4_video_ram_w, &sprint4_video_ram },
	{ 0x2000, 0x3fff, MWA_ROM },
	{ 0x4000, 0x4000, MWA_NOP }, /* diagnostic ROM location */
	{ 0xe800, 0xffff, MWA_ROM },
MEMORY_END


INPUT_PORTS_START( sprint4 )

	PORT_START
	PORT_BITX( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1, "Player 1 Gas", IP_KEY_DEFAULT, IP_JOY_DEFAULT )
	PORT_BIT ( 0x02, IP_ACTIVE_HIGH, IPT_SPECIAL ) /* P1 collision */
	PORT_BITX( 0x04, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2, "Player 2 Gas", IP_KEY_DEFAULT, IP_JOY_DEFAULT )
	PORT_BIT ( 0x08, IP_ACTIVE_HIGH, IPT_SPECIAL ) /* P2 collision */
	PORT_BITX( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER3, "Player 3 Gas", IP_KEY_DEFAULT, IP_JOY_DEFAULT )
	PORT_BIT ( 0x20, IP_ACTIVE_HIGH, IPT_SPECIAL ) /* P3 collision */
	PORT_BITX( 0x40, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER4, "Player 4 Gas", IP_KEY_DEFAULT, IP_JOY_DEFAULT )
	PORT_BIT ( 0x80, IP_ACTIVE_HIGH, IPT_SPECIAL ) /* P4 collision */

	PORT_START
	PORT_BIT ( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT ( 0x02, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT ( 0x04, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT ( 0x08, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT ( 0x10, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT ( 0x20, IP_ACTIVE_LOW, IPT_START3 )
	PORT_BIT ( 0x40, IP_ACTIVE_LOW, IPT_COIN4 )
	PORT_BIT ( 0x80, IP_ACTIVE_LOW, IPT_START4 )

	PORT_START
	PORT_SERVICE( 0x40, IP_ACTIVE_LOW )
	PORT_BIT ( 0x80, IP_ACTIVE_LOW, IPT_VBLANK )

	PORT_START
	PORT_BITX( 0x80, IP_ACTIVE_LOW, IPT_BUTTON2, "Track Select", KEYCODE_SPACE, IP_JOY_DEFAULT )

	PORT_START
	PORT_DIPNAME( 0x03, 0x03, "Language" )
	PORT_DIPSETTING(    0x00, "German" )
	PORT_DIPSETTING(    0x01, "French" )
	PORT_DIPSETTING(    0x02, "Spanish" )
	PORT_DIPSETTING(    0x03, "English" )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Coinage ))
	PORT_DIPSETTING(    0x00, DEF_STR( 2C_1C ))
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_1C ))
	PORT_DIPNAME( 0x08, 0x08, "Allow Late Entry" )
	PORT_DIPSETTING(    0x08, DEF_STR( No ))
	PORT_DIPSETTING(    0x00, DEF_STR( Yes ))
	PORT_DIPNAME( 0xf0, 0xb0, "Play Time" )
	PORT_DIPSETTING(    0x70, "60 seconds" )
	PORT_DIPSETTING(    0xb0, "90 seconds" )
	PORT_DIPSETTING(    0xd0, "120 seconds" )
	PORT_DIPSETTING(    0xe0, "150 seconds" )

	PORT_START
	PORT_ANALOG( 0xff, 0x00, IPT_DIAL | IPF_PLAYER1, 100, 1, 0, 0 )

	PORT_START
	PORT_ANALOG( 0xff, 0x00, IPT_DIAL | IPF_PLAYER2, 100, 1, 0, 0 )

	PORT_START
	PORT_ANALOG( 0xff, 0x00, IPT_DIAL | IPF_PLAYER3, 100, 1, 0, 0 )

	PORT_START
	PORT_ANALOG( 0xff, 0x00, IPT_DIAL | IPF_PLAYER4, 100, 1, 0, 0 )

	PORT_START
	PORT_BITX(0x01, IP_ACTIVE_HIGH, IPT_BUTTON2 | IPF_PLAYER1, "Player 1 Gear 1", KEYCODE_Z, IP_JOY_DEFAULT )
	PORT_BITX(0x02, IP_ACTIVE_HIGH, IPT_BUTTON3 | IPF_PLAYER1, "Player 1 Gear 2", KEYCODE_X, IP_JOY_DEFAULT )
	PORT_BITX(0x04, IP_ACTIVE_HIGH, IPT_BUTTON4 | IPF_PLAYER1, "Player 1 Gear 3", KEYCODE_C, IP_JOY_DEFAULT )
	PORT_BITX(0x08, IP_ACTIVE_HIGH, IPT_BUTTON5 | IPF_PLAYER1, "Player 1 Gear 4", KEYCODE_V, IP_JOY_DEFAULT )

	PORT_START
	PORT_BITX(0x01, IP_ACTIVE_HIGH, IPT_BUTTON2 | IPF_PLAYER2, "Player 2 Gear 1", KEYCODE_Q, IP_JOY_DEFAULT )
	PORT_BITX(0x02, IP_ACTIVE_HIGH, IPT_BUTTON3 | IPF_PLAYER2, "Player 2 Gear 2", KEYCODE_W, IP_JOY_DEFAULT )
	PORT_BITX(0x04, IP_ACTIVE_HIGH, IPT_BUTTON4 | IPF_PLAYER2, "Player 2 Gear 3", KEYCODE_E, IP_JOY_DEFAULT )
	PORT_BITX(0x08, IP_ACTIVE_HIGH, IPT_BUTTON5 | IPF_PLAYER2, "Player 2 Gear 4", KEYCODE_R, IP_JOY_DEFAULT )

	PORT_START
	PORT_BITX(0x01, IP_ACTIVE_HIGH, IPT_BUTTON2 | IPF_PLAYER3, "Player 3 Gear 1", KEYCODE_Y, IP_JOY_DEFAULT )
	PORT_BITX(0x02, IP_ACTIVE_HIGH, IPT_BUTTON3 | IPF_PLAYER3, "Player 3 Gear 2", KEYCODE_U, IP_JOY_DEFAULT )
	PORT_BITX(0x04, IP_ACTIVE_HIGH, IPT_BUTTON4 | IPF_PLAYER3, "Player 3 Gear 3", KEYCODE_I, IP_JOY_DEFAULT )
	PORT_BITX(0x08, IP_ACTIVE_HIGH, IPT_BUTTON5 | IPF_PLAYER3, "Player 3 Gear 4", KEYCODE_O, IP_JOY_DEFAULT )

	PORT_START
	PORT_BITX(0x01, IP_ACTIVE_HIGH, IPT_BUTTON2 | IPF_PLAYER3, "Player 4 Gear 1", IP_KEY_DEFAULT, IP_JOY_DEFAULT )
	PORT_BITX(0x02, IP_ACTIVE_HIGH, IPT_BUTTON3 | IPF_PLAYER3, "Player 4 Gear 2", IP_KEY_DEFAULT, IP_JOY_DEFAULT )
	PORT_BITX(0x04, IP_ACTIVE_HIGH, IPT_BUTTON4 | IPF_PLAYER3, "Player 4 Gear 3", IP_KEY_DEFAULT, IP_JOY_DEFAULT )
	PORT_BITX(0x08, IP_ACTIVE_HIGH, IPT_BUTTON5 | IPF_PLAYER3, "Player 4 Gear 4", IP_KEY_DEFAULT, IP_JOY_DEFAULT )

INPUT_PORTS_END


static struct GfxLayout tile_layout =
{
	8, 8,
	256,
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


static struct GfxLayout car_layout =
{
	12, 16,
	64,
	1,
	{ 0 },
	{
		0x0007, 0x0006, 0x0005, 0x0004,
		0x2007, 0x2006, 0x2005, 0x2004,
		0x4007, 0x4006, 0x4005, 0x4004
	},
	{
		0x00, 0x08, 0x10, 0x18, 0x20, 0x28, 0x30, 0x38,
		0x40, 0x48, 0x50, 0x58, 0x60, 0x68, 0x70, 0x78
	},
	0x80
};


static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &tile_layout, 0, 5 },
	{ REGION_GFX2, 0, &car_layout, 0, 5 },
	{ -1 }
};


static MACHINE_DRIVER_START( sprint4 )

	/* basic machine hardware */
	MDRV_CPU_ADD(M6502, 12096000 / 16)
	MDRV_CPU_MEMORY(readmem, writemem)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(38 * 1000000 / 15750)
	MDRV_MACHINE_INIT(sprint4)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(256, 262)
	MDRV_VISIBLE_AREA(0, 255, 0, 223)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(6)
	MDRV_COLORTABLE_LENGTH(10)

	MDRV_PALETTE_INIT(sprint4)
	MDRV_VIDEO_START(sprint4)
	MDRV_VIDEO_UPDATE(sprint4)
	MDRV_VIDEO_EOF(sprint4)

	/* sound hardware */
MACHINE_DRIVER_END


ROM_START( sprint4 )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD         ( "30031.c1",    0x2800, 0x0800, CRC(017ee7c4) SHA1(9386cacc619669c18af31f66691a45af6dafef64) ) 
	ROM_RELOAD       (                0xe800, 0x0800 )
	ROM_LOAD_NIB_LOW ( "30036-02.n1", 0x3000, 0x0800, CRC(883b9d7c) SHA1(af52ffdd9cd8dfed54013c9b0d3c6e48c7419d17) ) 
	ROM_RELOAD       (                0xf000, 0x0800 )
	ROM_LOAD_NIB_HIGH( "30037-02.k1", 0x3000, 0x0800, CRC(c297fbd8) SHA1(8cc0f486429e12bee21a5dd1135e799196480044) ) 
	ROM_RELOAD       (                0xf000, 0x0800 )
	ROM_LOAD         ( "30033.e1",    0x3800, 0x0800, CRC(b8b717b7) SHA1(2f6b1a0e9803901d9ba79d1f19a025f6a6134756) ) 
	ROM_RELOAD       (                0xf800, 0x0800 )

	ROM_REGION( 0x0800, REGION_GFX1, ROMREGION_DISPOSE ) /* tiles */
	ROM_LOAD( "30027.h5", 0x0000, 0x0800, CRC(3a752e07) SHA1(d990f94b296409d47e8bada98ddbed5f76567e1b) ) 

	ROM_REGION( 0x0c00, REGION_GFX2, ROMREGION_DISPOSE ) /* cars */
	ROM_LOAD( "30028-01.n6", 0x0000, 0x0400, CRC(3ebcb13f) SHA1(e0b87239081f12f6613d3db6a8cb5b80937df7d7) ) 
	ROM_LOAD( "30029-01.m6", 0x0400, 0x0400, CRC(963a8424) SHA1(d52a0e73c54154531e825153012687bdb85e479a) ) 
	ROM_LOAD( "30030-01.l6", 0x0800, 0x0400, CRC(e94dfc2d) SHA1(9c5b1401c4aadda0a3aee76e4f92e73ae1d35cb7) ) 

	ROM_REGION( 0x0200, REGION_PROMS, 0 )
	ROM_LOAD( "30024-01.p8", 0x0000, 0x0200, CRC(e71d2e22) SHA1(434c3a8237468604cce7feb40e6061d2670013b3) )	/* SYNC */
ROM_END


ROM_START( sprint4a )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD         ( "30031.c1",    0x2800, 0x0800, CRC(017ee7c4) SHA1(9386cacc619669c18af31f66691a45af6dafef64) ) 
	ROM_RELOAD       (                0xe800, 0x0800 )
	ROM_LOAD_NIB_LOW ( "30036-02.n1", 0x3000, 0x0800, CRC(883b9d7c) SHA1(af52ffdd9cd8dfed54013c9b0d3c6e48c7419d17) ) 
	ROM_RELOAD       (                0xf000, 0x0800 )
	ROM_LOAD_NIB_HIGH( "30037-02.k1", 0x3000, 0x0800, CRC(c297fbd8) SHA1(8cc0f486429e12bee21a5dd1135e799196480044) ) 
	ROM_RELOAD       (                0xf000, 0x0800 )
	ROM_LOAD         ( "30033.e1",    0x3800, 0x0800, CRC(b8b717b7) SHA1(2f6b1a0e9803901d9ba79d1f19a025f6a6134756) ) 
	ROM_RELOAD       (                0xf800, 0x0800 )

	ROM_REGION( 0x0800, REGION_GFX1, ROMREGION_DISPOSE ) /* tiles */
	ROM_LOAD( "30027.h5", 0x0000, 0x0800, CRC(3a752e07) SHA1(d990f94b296409d47e8bada98ddbed5f76567e1b) ) 

	ROM_REGION( 0x0c00, REGION_GFX2, ROMREGION_DISPOSE ) /* cars */
	ROM_LOAD( "30028-03.n6", 0x0000, 0x0400, CRC(d3337030) SHA1(3e73fc45bdcaa52dc1aa01489b46240284562ab7) ) 
	ROM_LOAD( "30029-03.m6", 0x0400, 0x0400, NO_DUMP ) 
	ROM_LOAD( "30030-03.l6", 0x0800, 0x0400, CRC(aa1b45ab) SHA1(1ddb64d4ec92a1383866daaefa556499837decd1) ) 

	ROM_REGION( 0x0200, REGION_PROMS, 0 )
	ROM_LOAD( "30024-01.p8", 0x0000, 0x0200, CRC(e71d2e22) SHA1(434c3a8237468604cce7feb40e6061d2670013b3) )	/* SYNC */
ROM_END


GAMEX( 1977, sprint4,  0,       sprint4, sprint4, 0, ROT0, "Atari", "Sprint 4 (set 1)", GAME_NO_SOUND )
GAMEX( 1977, sprint4a, sprint4, sprint4, sprint4, 0, ROT0, "Atari", "Sprint 4 (set 2)", GAME_NO_SOUND )
