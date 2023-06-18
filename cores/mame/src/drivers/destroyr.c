/***************************************************************************

Atari Destroyer Driver

***************************************************************************/

#include "driver.h"

extern VIDEO_UPDATE( destroyr );

extern int destroyr_wavemod;
extern int destroyr_cursor;

extern UINT8* destroyr_major_obj_ram;
extern UINT8* destroyr_minor_obj_ram;
extern UINT8* destroyr_alpha_num_ram;

static int destroyr_potmask[2];
static int destroyr_potsense[2];
static int destroyr_attract;
static int destroyr_motor_speed;
static int destroyr_noise;

static UINT8* destroyr_zero_page;


static void destroyr_dial_callback(int dial)
{
	/* Analog inputs come from the player's depth control potentiometer.
	   The voltage is compared to a voltage ramp provided by a discrete
	   analog circuit that conditions the VBLANK signal. When the ramp
	   voltage exceeds the input voltage an NMI signal is generated. The
	   computer then reads the VSYNC data functions to tell where the
	   cursor should be located. */

	destroyr_potsense[dial] = 1;

	if (destroyr_potmask[dial])
	{
		cpu_set_nmi_line(0, PULSE_LINE);
	}
}


static void destroyr_frame_callback(int dummy)
{
	destroyr_potsense[0] = 0;
	destroyr_potsense[1] = 0;

	/* PCB supports two dials, but cab has only got one */

	timer_set(cpu_getscanlinetime(readinputport(3)), 0, destroyr_dial_callback);
}


static MACHINE_INIT( destroyr )
{
	timer_pulse(cpu_getscanlinetime(0), 0, destroyr_frame_callback);
}


WRITE_HANDLER( destroyr_ram_w )
{
	destroyr_zero_page[offset & 0xff] = data;
}


WRITE_HANDLER( destroyr_misc_w )
{
	/* bits 0 to 2 connect to the sound circuits */

	destroyr_attract = data & 1;
	destroyr_noise = data & 2;
	destroyr_motor_speed = data & 4;
	destroyr_potmask[0] = data & 8;
	destroyr_wavemod = data & 16;
	destroyr_potmask[1] = data & 32;

	coin_lockout_w(0, !destroyr_attract);
	coin_lockout_w(1, !destroyr_attract);
}


WRITE_HANDLER( destroyr_cursor_load_w )
{
	destroyr_cursor = data;

	watchdog_reset_w(offset, data);
}


WRITE_HANDLER( destroyr_interrupt_ack_w )
{
	cpu_set_irq_line(0, 0, CLEAR_LINE);
}


WRITE_HANDLER( destroyr_output_w )
{
	offset &= 15;

	switch (offset)
	{
	case 0:
		set_led_status(0, data & 1);
		break;
	case 1:
		set_led_status(1, data & 1); /* no second LED present on cab */
		break;
	case 2:
		/* bit 0 => songate */
		break;
	case 3:
		/* bit 0 => launch */
		break;
	case 4:
		/* bit 0 => explosion */
		break;
	case 5:
		/* bit 0 => sonar */
		break;
	case 6:
		/* bit 0 => high explosion */
		break;
	case 7:
		/* bit 0 => low explosion */
		break;
	case 8:
		destroyr_misc_w(offset, data);
		break;
	default:
		log_cb(RETRO_LOG_DEBUG, LOGPRE "unmapped output port %d\n", offset);
		break;
	}
}


READ_HANDLER( destroyr_ram_r )
{
	return destroyr_zero_page[offset & 0xff];
}


READ_HANDLER( destroyr_input_r )
{
	offset &= 15;

	if (offset == 0)
	{
		UINT8 ret = readinputport(0);

		if (destroyr_potsense[0] && destroyr_potmask[0])
			ret |= 4;
		if (destroyr_potsense[1] && destroyr_potmask[1])
			ret |= 8;

		return ret;
	}

	if (offset == 1)
	{
		return readinputport(1);
	}

	log_cb(RETRO_LOG_DEBUG, LOGPRE "unmapped input port %d\n", offset);

	return 0;
}


READ_HANDLER( destroyr_scanline_r )
{
	return cpu_getscanline();
}


static MEMORY_READ_START( destroyr_readmem )
	{ 0x0000, 0x00ff, MRA_RAM },
	{ 0x0100, 0x0fff, destroyr_ram_r },
	{ 0x1000, 0x1fff, destroyr_input_r },
	{ 0x2000, 0x2fff, input_port_2_r },
	{ 0x6000, 0x6fff, destroyr_scanline_r },
	{ 0x7000, 0x77ff, MRA_NOP }, /* missing translation ROMs */
	{ 0x7800, 0x7fff, MRA_ROM }, /* program */
	{ 0xf800, 0xffff, MRA_ROM }, /* program mirror */
MEMORY_END

static MEMORY_WRITE_START( destroyr_writemem )
	{ 0x0000, 0x00ff, MWA_RAM, &destroyr_zero_page },
	{ 0x0100, 0x0fff, destroyr_ram_w },
	{ 0x1000, 0x1fff, destroyr_output_w },
	{ 0x3000, 0x30ff, MWA_RAM, &destroyr_alpha_num_ram },
	{ 0x4000, 0x401f, MWA_RAM, &destroyr_major_obj_ram },
	{ 0x5000, 0x5000, destroyr_cursor_load_w },
	{ 0x5001, 0x5001, destroyr_interrupt_ack_w },
	{ 0x5002, 0x5007, MWA_RAM, &destroyr_minor_obj_ram },
	{ 0x7000, 0x77ff, MWA_NOP }, /* missing translation ROMs */
	{ 0x7800, 0x7fff, MWA_ROM }, /* program */
	{ 0xf800, 0xffff, MWA_ROM }, /* program mirror */
MEMORY_END


INPUT_PORTS_START( destroyr )
	PORT_START /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW,  IPT_UNUSED ) /* call 7400 */
	PORT_BIT( 0x02, IP_ACTIVE_LOW,  IPT_UNUSED )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNUSED ) /* potsense1 */
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNUSED ) /* potsense2 */
	PORT_BIT( 0x10, IP_ACTIVE_LOW,  IPT_START1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW,  IPT_START2 )
	PORT_DIPNAME( 0xc0, 0x80, "Extended Play" )
	PORT_DIPSETTING( 0x40, "1500 points" )
	PORT_DIPSETTING( 0x80, "2500 points" )
	PORT_DIPSETTING( 0xc0, "3500 points" )
	PORT_DIPSETTING( 0x00, "never" )

	PORT_START /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW,  IPT_TILT )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_BUTTON2 ) /* actually a lever */
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_SERVICE( 0x08, IP_ACTIVE_LOW )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW,  IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_VBLANK )

	PORT_START /* IN2 */
	PORT_DIPNAME( 0x03, 0x02, DEF_STR( Coinage ) )
	PORT_DIPSETTING( 0x03, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING( 0x02, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING( 0x01, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING( 0x00, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0x0c, 0x08, "Play Time" )
	PORT_DIPSETTING( 0x00, "50 seconds" )
	PORT_DIPSETTING( 0x04, "75 seconds" )
	PORT_DIPSETTING( 0x08, "100 seconds" )
	PORT_DIPSETTING( 0x0c, "125 seconds" )
	PORT_DIPNAME( 0x30, 0x00, "Language" ) /* requires translation ROMs */
	PORT_DIPSETTING( 0x30, "German" )
	PORT_DIPSETTING( 0x20, "French" )
	PORT_DIPSETTING( 0x10, "Spanish" )
	PORT_DIPSETTING( 0x00, "English" )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START /* IN3 */
	PORT_ANALOG( 0xff, 0x00, IPT_PADDLE_V | IPF_REVERSE, 30, 10, 0, 160)
INPUT_PORTS_END


static struct GfxLayout destroyr_alpha_num_layout =
{
	8, 8,     /* width, height */
	64,       /* total         */
	1,        /* planes        */
	{ 0 },    /* plane offsets */
	{
		0x4, 0x5, 0x6, 0x7, 0xC, 0xD, 0xE, 0xF
	},
	{
		0x00, 0x10, 0x20, 0x30, 0x40, 0x50, 0x60, 0x70
	},
	0x80      /* increment */
};


static struct GfxLayout destroyr_minor_object_layout =
{
	16, 16,   /* width, height */
	16,       /* total         */
	1,        /* planes        */
	{ 0 },    /* plane offsets */
	{
	  0x04, 0x05, 0x06, 0x07, 0x0C, 0x0D, 0x0E, 0x0F,
	  0x14, 0x15, 0x16, 0x17, 0x1C, 0x1D, 0x1E, 0x1F
	},
	{
	  0x000, 0x020, 0x040, 0x060, 0x080, 0x0a0, 0x0c0, 0x0e0,
	  0x100, 0x120, 0x140, 0x160, 0x180, 0x1a0, 0x1c0, 0x1e0
	},
	0x200     /* increment */
};


static struct GfxLayout destroyr_major_object_layout =
{
	64, 16,   /* width, height */
	4,        /* total         */
	2,        /* planes        */
	{ 1, 0 }, /* plane offsets */
	{
	  0x00, 0x02, 0x04, 0x06, 0x08, 0x0A, 0x0C, 0x0E,
	  0x10, 0x12, 0x14, 0x16, 0x18, 0x1A, 0x1C, 0x1E,
	  0x20, 0x22, 0x24, 0x26, 0x28, 0x2A, 0x2C, 0x2E,
	  0x30, 0x32, 0x34, 0x36, 0x38, 0x3A, 0x3C, 0x3E,
	  0x40, 0x42, 0x44, 0x46, 0x48, 0x4A, 0x4C, 0x4E,
	  0x50, 0x52, 0x54, 0x56, 0x58, 0x5A, 0x5C, 0x5E,
	  0x60, 0x62, 0x64, 0x66, 0x68, 0x6A, 0x6C, 0x6E,
	  0x70, 0x72, 0x74, 0x76, 0x78, 0x7A, 0x7C, 0x7E
	},
	{
	  0x000, 0x080, 0x100, 0x180, 0x200, 0x280, 0x300, 0x380,
	  0x400, 0x480, 0x500, 0x580, 0x600, 0x680, 0x700, 0x780
	},
	0x0800     /* increment */
};


static struct GfxLayout destroyr_waves_layout =
{
	64, 2,    /* width, height */
	2,        /* total         */
	1,        /* planes        */
	{ 0 },    /* plane offsets */
	{
	  0x00, 0x01, 0x02, 0x03, 0x08, 0x09, 0x0A, 0x0B,
	  0x10, 0x11, 0x12, 0x13, 0x18, 0x19, 0x1A, 0x1B,
	  0x20, 0x21, 0x22, 0x23, 0x28, 0x29, 0x2A, 0x2B,
	  0x30, 0x31, 0x32, 0x33, 0x38, 0x39, 0x3A, 0x3B,
	  0x40, 0x41, 0x42, 0x43, 0x48, 0x49, 0x4A, 0x4B,
	  0x50, 0x51, 0x52, 0x53, 0x58, 0x59, 0x5A, 0x5B,
	  0x60, 0x61, 0x62, 0x63, 0x68, 0x69, 0x6A, 0x6B,
	  0x70, 0x71, 0x72, 0x73, 0x78, 0x79, 0x7A, 0x7B
	},
	{
	  0x00, 0x80
	},
	0x04     /* increment */
};


static struct GfxDecodeInfo destroyr_gfx_decode_info[] =
{
	{ REGION_GFX1, 0, &destroyr_alpha_num_layout, 4, 1 },
	{ REGION_GFX2, 0, &destroyr_minor_object_layout, 4, 1 },
	{ REGION_GFX3, 0, &destroyr_major_object_layout, 0, 1 },
	{ REGION_GFX4, 0, &destroyr_waves_layout, 4, 1 },
	{ -1 } /* end of array */
};


static PALETTE_INIT( destroyr )
{
	palette_set_color(0, 0x00, 0x00, 0x00);   /* major objects */
	palette_set_color(1, 0x50, 0x50, 0x50);
	palette_set_color(2, 0xAF, 0xAF, 0xAF);
	palette_set_color(3, 0xFF ,0xFF, 0xFF);
	palette_set_color(4, 0x00, 0x00, 0x00);   /* alpha numerics, waves, minor objects */
	palette_set_color(5, 0xFF, 0xFF, 0xFF);
	palette_set_color(6, 0x00, 0x00, 0x00);   /* cursor */
	palette_set_color(7, 0x78, 0x78, 0x78);
}


static MACHINE_DRIVER_START( destroyr )

	/* basic machine hardware */
	MDRV_CPU_ADD(M6800, 12096000 / 16)
	MDRV_CPU_MEMORY(destroyr_readmem, destroyr_writemem)
	MDRV_CPU_VBLANK_INT(irq0_line_assert, 4)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION((int) ((22. * 1000000) / (262. * 60) + 0.5))

	MDRV_MACHINE_INIT(destroyr)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(256, 240)
	MDRV_VISIBLE_AREA(0, 255, 0, 239)
	MDRV_GFXDECODE(destroyr_gfx_decode_info)
	MDRV_PALETTE_LENGTH(8)
	MDRV_PALETTE_INIT(destroyr)
	MDRV_VIDEO_UPDATE(destroyr)

	/* sound hardware */
MACHINE_DRIVER_END


ROM_START( destroyr )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )                  /* program code */
	ROM_LOAD( "30146-01.c3", 0x7800, 0x0800, CRC(e560c712) SHA1(0505ab57eee5421b4ff4e87d14505e02b18fd54c) )
	ROM_RELOAD(              0xF800, 0x0800 )

	ROM_REGION( 0x0400, REGION_GFX1, ROMREGION_DISPOSE )   /* alpha numerics */
	ROM_LOAD( "30135-01.p4", 0x0000, 0x0400, CRC(184824cf) SHA1(713cfd1d41ef7b1c345ea0038b652c4ba3f08301) )

	ROM_REGION( 0x0400, REGION_GFX2, ROMREGION_DISPOSE )   /* minor objects */
	ROM_LOAD( "30132-01.f4", 0x0000, 0x0400, CRC(e09d3d55) SHA1(b26013397ef2cb32d0416ecb118387b9c2dffa9a) )

	ROM_REGION( 0x0400, REGION_GFX3, ROMREGION_DISPOSE )   /* major objects */
	ROM_LOAD_NIB_HIGH( "30134-01.p8", 0x0000, 0x0400, CRC(6259e007) SHA1(049f5f7160305cb4f4b499dd113cb11eea73fc95) )
	ROM_LOAD_NIB_LOW ( "30133-01.n8", 0x0000, 0x0400, CRC(108d3e2c) SHA1(8c993369d37c6713670483af78e6d04d38f4b4fc) )

	ROM_REGION( 0x0020, REGION_GFX4, ROMREGION_DISPOSE )   /* waves */
	ROM_LOAD( "30136-01.k2", 0x0000, 0x0020, CRC(532c11b1) SHA1(18ab5369a3f2cfcc9a44f38fa8649524bea5b203) )

	ROM_REGION( 0x0100, REGION_USER1, 0 )                  /* sync (unused) */
	ROM_LOAD( "30131-01.m1", 0x0000, 0x0100, CRC(b8094b4c) SHA1(82dc6799a19984f3b204ee3aeeb007e55afc8be3) )
ROM_END


GAMEX( 1977, destroyr, 0, destroyr, destroyr, 0, ORIENTATION_FLIP_X, "Atari", "Destroyer", GAME_NO_SOUND )
