/***************************************************************************

Atari Boxer (prototype) driver

  AKA Boxing, both game titles appear in the schematics

  This game had some weird controls that don't work well in MAME.

***************************************************************************/

#include "driver.h"

extern UINT8* boxer_tile_ram;
extern UINT8* boxer_sprite_ram;

extern VIDEO_UPDATE( boxer );

static UINT8 pot_state;
static UINT8 pot_latch;


static void pot_interrupt(int mask)
{
	if (pot_latch & mask)
	{
		cpu_set_nmi_line(0, ASSERT_LINE);
	}

	pot_state |= mask;
}


static void periodic_callback(int scanline)
{
	cpu_set_irq_line(0, 0, ASSERT_LINE);

	if (scanline == 0)
	{
		UINT8 mask[256];

		int i;

		memset(mask, 0, sizeof mask);

		mask[readinputport(3)] |= 0x01;
		mask[readinputport(4)] |= 0x02;
		mask[readinputport(5)] |= 0x04;
		mask[readinputport(6)] |= 0x08;
		mask[readinputport(7)] |= 0x10;
		mask[readinputport(8)] |= 0x20;

		for (i = 1; i < 256; i++)
		{
			if (mask[i] != 0)
			{
				timer_set(cpu_getscanlinetime(i), mask[i], pot_interrupt);
			}
		}

		pot_state = 0;
	}

	scanline += 64;

	if (scanline >= 262)
	{
		scanline = 0;
	}

	timer_set(cpu_getscanlinetime(scanline), scanline, periodic_callback);
}


static PALETTE_INIT( boxer )
{
	palette_set_color(0, 0x00, 0x00, 0x00);
	palette_set_color(1, 0xff, 0xff, 0xff);

	colortable[0] = 0;
	colortable[1] = 1;
	colortable[2] = 1;
	colortable[3] = 0;
}


static MACHINE_INIT( boxer )
{
	timer_set(cpu_getscanlinetime(0), 0, periodic_callback);

	pot_latch = 0;
}


static READ_HANDLER( boxer_input_r )
{
	UINT8 val = readinputport(0);

	if (readinputport(9) < cpu_getscanline())
	{
		val |= 0x02;
	}

	return (val << ((offset & 7) ^ 7)) & 0x80;
}


static READ_HANDLER( boxer_misc_r )
{
	UINT8 val = 0;

	switch (offset & 3)
	{
	case 0:
		val = pot_state & pot_latch;
		break;

	case 1:
		val = cpu_getscanline();
		break;

	case 2:
		val = readinputport(1);
		break;

	case 3:
		val = readinputport(2);
		break;
	}

	return val ^ 0x3f;
}


static READ_HANDLER( boxer_bad_address_r )
{
	cpu_set_reset_line(0, PULSE_LINE);

	return 0;
}


static WRITE_HANDLER( boxer_bell_w )
{
}


static WRITE_HANDLER( boxer_sound_w )
{
}


static WRITE_HANDLER( boxer_pot_w )
{
	/* BIT0 => HPOT1 */
	/* BIT1 => VPOT1 */
	/* BIT2 => RPOT1 */
	/* BIT3 => HPOT2 */
	/* BIT4 => VPOT2 */
	/* BIT5 => RPOT2 */

	pot_latch = data & 0x3f;

	cpu_set_nmi_line(0, CLEAR_LINE);
}


static WRITE_HANDLER( boxer_irq_reset_w )
{
	cpu_set_irq_line(0, 0, CLEAR_LINE);
}


static WRITE_HANDLER( boxer_crowd_w )
{
	/* BIT0 => ATTRACT */
	/* BIT1 => CROWD-1 */
	/* BIT2 => CROWD-2 */
	/* BIT3 => CROWD-3 */

	coin_lockout_global_w(data & 1);
}


static WRITE_HANDLER( boxer_led_w )
{
	set_led_status(1, !(data & 1));
	set_led_status(0, !(data & 2));
}


static WRITE_HANDLER( boxer_bad_address_w )
{
	cpu_set_reset_line(0, PULSE_LINE);
}


static MEMORY_READ_START( boxer_readmem )
	{ 0x0000, 0x01ff, MRA_RAM },
	{ 0x0200, 0x03ff, MRA_RAM },
	{ 0x0800, 0x08ff, boxer_input_r },
	{ 0x1000, 0x17ff, boxer_misc_r },
	{ 0x3000, 0x3fff, MRA_ROM },
	{ 0x4000, 0xbfff, boxer_bad_address_r },
	{ 0xf000, 0xffff, MRA_ROM },
MEMORY_END


static MEMORY_WRITE_START( boxer_writemem )
	{ 0x0000, 0x01ff, MWA_RAM },
	{ 0x0200, 0x03ff, MWA_RAM, &boxer_tile_ram },
	{ 0x1800, 0x1800, boxer_pot_w },
	{ 0x1900, 0x19ff, boxer_led_w },
	{ 0x1a00, 0x1aff, boxer_sound_w },
	{ 0x1b00, 0x1bff, boxer_crowd_w },
	{ 0x1c00, 0x1cff, boxer_irq_reset_w },
	{ 0x1d00, 0x1dff, boxer_bell_w },
	{ 0x1e00, 0x1eff, MWA_RAM, &boxer_sprite_ram },
	{ 0x1f00, 0x1fff, watchdog_reset_w },
	{ 0x3000, 0x3fff, MWA_ROM },
	{ 0x4000, 0xbfff, boxer_bad_address_w },
	{ 0xf000, 0xffff, MWA_ROM },
MEMORY_END


INPUT_PORTS_START( boxer )

	PORT_START
	PORT_BIT ( 0x01, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT ( 0x02, IP_ACTIVE_HIGH, IPT_UNUSED ) /* TIMER */
	PORT_BIT ( 0x04, IP_ACTIVE_LOW, IPT_TILT )
	PORT_SERVICE( 0x08, IP_ACTIVE_HIGH )
	PORT_BIT ( 0x10, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT ( 0x20, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT ( 0x40, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT ( 0x80, IP_ACTIVE_LOW, IPT_COIN2 )

	PORT_START
	PORT_DIPNAME( 0x03, 0x01, "Number of Rounds" )
	PORT_DIPSETTING(    0x03, "1" )
	PORT_DIPSETTING(    0x02, "2" )
	PORT_DIPSETTING(    0x01, "3" )
	PORT_DIPSETTING(    0x00, "4" )

	PORT_START
	PORT_DIPNAME( 0x03, 0x01, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( Free_Play ) )

	PORT_START
	PORT_ANALOG ( 0xff, 0x80, IPT_AD_STICK_X | IPF_REVERSE | IPF_PLAYER1, 30, 16, 0x20, 0xe0)

	PORT_START
	PORT_ANALOG ( 0xff, 0x80, IPT_AD_STICK_Y | IPF_REVERSE | IPF_PLAYER1, 30, 16, 0x20, 0xe0)

	PORT_START
	PORT_ANALOGX( 0xff, 0x80, IPT_PADDLE | IPF_PLAYER1, 30, 16, 0x20, 0xe0, KEYCODE_Z, KEYCODE_X, IP_JOY_NONE, IP_JOY_NONE )

	PORT_START
	PORT_ANALOG ( 0xff, 0x80, IPT_AD_STICK_X | IPF_REVERSE | IPF_PLAYER2, 30, 16, 0x20, 0xe0)

	PORT_START
	PORT_ANALOG ( 0xff, 0x80, IPT_AD_STICK_Y | IPF_REVERSE | IPF_PLAYER2, 30, 16, 0x20, 0xe0)

	PORT_START
	PORT_ANALOGX( 0xff, 0x80, IPT_PADDLE | IPF_PLAYER2, 30, 16, 0x20, 0xe0, KEYCODE_Q, KEYCODE_W, IP_JOY_NONE, IP_JOY_NONE )

	PORT_START
	PORT_DIPNAME( 0xff, 0x5C, "Round Time" ) /* actually a potentiometer */
	PORT_DIPSETTING(    0x3C, "15 seconds" )
	PORT_DIPSETTING(    0x5C, "30 seconds" )
	PORT_DIPSETTING(    0x7C, "45 seconds" )

INPUT_PORTS_END


static struct GfxLayout tile_layout =
{
	8, 8,
	64,
	1,
	{ 0 },
	{
		0x7, 0x6, 0x5, 0x4, 0xf, 0xe, 0xd, 0xc
	},
	{
		0x00, 0x10, 0x20, 0x30, 0x40, 0x50, 0x60, 0x70
	},
	0x80
};


static struct GfxLayout sprite_layout =
{
	8, 8,
	64,
	1,
	{ 0 },
	{
		0x4, 0x5, 0x6, 0x7, 0xc, 0xd, 0xe, 0xf
	},
	{
		0x00, 0x10, 0x20, 0x30, 0x40, 0x50, 0x60, 0x70
	},
	0x80
};


static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &sprite_layout, 0, 1 },
	{ REGION_GFX2, 0, &sprite_layout, 0, 1 },
	{ REGION_GFX3, 0, &tile_layout, 2, 1 },
	{ -1 } /* end of array */
};


static MACHINE_DRIVER_START(boxer)

	/* basic machine hardware */
	MDRV_CPU_ADD(M6502, 12096000 / 16)
	MDRV_CPU_MEMORY(boxer_readmem, boxer_writemem)

	MDRV_FRAMES_PER_SECOND(60)

	/* video hardware */
	MDRV_MACHINE_INIT(boxer)

	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(256, 262)
	MDRV_VISIBLE_AREA(8, 247, 0, 239)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(2)
	MDRV_COLORTABLE_LENGTH(4)
	MDRV_PALETTE_INIT(boxer)
	MDRV_VIDEO_UPDATE(boxer)

	/* sound hardware */
MACHINE_DRIVER_END


ROM_START( boxer )

	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD_NIB_LOW ( "3400l.e1", 0x3400, 0x0400, CRC(df85afa4) SHA1(5a74a08f1e0b0bbec02999d5e46513d8afd333ac) )
	ROM_RELOAD       (             0xF400, 0x0400 )
	ROM_LOAD_NIB_HIGH( "3400m.a1", 0x3400, 0x0400, CRC(23fe06aa) SHA1(03a4eedbf60f07d1dd8d7af576828df5f032146e) )
	ROM_RELOAD       (             0xF400, 0x0400 )
	ROM_LOAD_NIB_LOW ( "3800l.j1", 0x3800, 0x0400, CRC(087263fb) SHA1(cc3715a68bd05f23b4abf9f18ca14a8fe55163f7) )
	ROM_RELOAD       (             0xF800, 0x0400 )
	ROM_LOAD_NIB_HIGH( "3800m.d1", 0x3800, 0x0400, CRC(3bbf605e) SHA1(be4ff1702eb837710421a7dafcdc60fe2d3259e8) )
	ROM_RELOAD       (             0xF800, 0x0400 )
	ROM_LOAD_NIB_LOW ( "3c00l.h1", 0x3C00, 0x0400, CRC(09e204f2) SHA1(565d4c8865da7d96a45e909973d570101de61f63) )
	ROM_RELOAD       (             0xFC00, 0x0400 )
	ROM_LOAD_NIB_HIGH( "3c00m.c1", 0x3C00, 0x0400, CRC(2f8ebc85) SHA1(05a4e29ec7e49173200d5fe5344274fd6afd16d7) )
	ROM_RELOAD       (             0xFC00, 0x0400 )

	ROM_REGION( 0x0400, REGION_GFX1, ROMREGION_DISPOSE ) /* lower boxer */
	ROM_LOAD( "bx137l.c8", 0x0000, 0x0400, CRC(e91f2048) SHA1(64039d07557e210aa4f6663cd7e72814cb881310) )

	ROM_REGION( 0x0400, REGION_GFX2, ROMREGION_DISPOSE ) /* upper boxer */
	ROM_LOAD( "bx137u.m8", 0x0000, 0x0400, CRC(e4fee386) SHA1(79b70aca4a92c56363689a363b643d46294d3e88) )

	ROM_REGION( 0x0400, REGION_GFX3, ROMREGION_DISPOSE ) /* tiles */
	ROM_LOAD( "9417.k2", 0x0000, 0x0400, CRC(7e3d22cf) SHA1(92e6bbe049dc8fcd674f2ff96cde3786f714508d) )

	ROM_REGION( 0x0200, REGION_USER1, 0 ) /* lower boxer map */
	ROM_LOAD( "bx115l.b7", 0x0000, 0x0200, CRC(31f2234f) SHA1(d53f3a1d0db3cf3024de61ef64f76c6dfdf6861c) )

	ROM_REGION( 0x0200, REGION_USER2, 0 ) /* upper boxer map */
	ROM_LOAD( "bx115u.l7", 0x0000, 0x0200, CRC(124d3f24) SHA1(09fab2ae218b8584c0e3c8e02f5680ce083a33d6) )

	ROM_REGION( 0x0100, REGION_PROMS, 0 ) /* sync prom */
	ROM_LOAD( "9402.m3", 0x0000, 0x0100, CRC(00e224a0) SHA1(1a384ef488791c62566c91b18d6a1fb4a5def2ba) )
ROM_END


GAMEX( 1978, boxer, 0, boxer, boxer, 0, 0, "Atari", "Boxer (prototype)", GAME_NO_SOUND )
