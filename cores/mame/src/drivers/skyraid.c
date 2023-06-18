/***************************************************************************

Atari Sky Raider driver

***************************************************************************/

#include "driver.h"

extern UINT8* skyraid_alpha_num_ram;
extern UINT8* skyraid_pos_ram;
extern UINT8* skyraid_obj_ram;

extern int skyraid_scroll;

extern VIDEO_START(skyraid);
extern VIDEO_UPDATE(skyraid);

static int analog_range;
static int analog_offset;


static PALETTE_INIT( skyraid )
{
	palette_set_color( 0, 0x00, 0x00, 0x00);	/* terrain */
	palette_set_color( 1, 0x18, 0x18, 0x18);
	palette_set_color( 2, 0x30, 0x30, 0x30);
	palette_set_color( 3, 0x48, 0x48, 0x48);
	palette_set_color( 4, 0x60, 0x60, 0x60);
	palette_set_color( 5, 0x78, 0x78, 0x78);
	palette_set_color( 6, 0x90, 0x90, 0x90);
	palette_set_color( 7, 0xA8, 0xA8, 0xA8);
	palette_set_color( 8, 0x10, 0x10, 0x10);	/* sprites */
	palette_set_color( 9, 0xE0, 0xE0, 0xE0);
	palette_set_color(10, 0xA0, 0xA0, 0xA0);
	palette_set_color(11, 0x48, 0x48, 0x48);
	palette_set_color(12, 0x10, 0x10, 0x10);
	palette_set_color(13, 0x48, 0x48, 0x48);
	palette_set_color(14, 0xA0, 0xA0, 0xA0);
	palette_set_color(15, 0xE0, 0xE0, 0xE0);
	palette_set_color(16, 0x00, 0x00, 0x00);	/* missiles */
	palette_set_color(17, 0xFF, 0xFF, 0xFF);
	palette_set_color(18, 0x00, 0x00, 0x00);	/* text */
	palette_set_color(19, 0xE0, 0xE0, 0xE0);
}


static READ_HANDLER( skyraid_zeropage_r )
{
	return memory_region(REGION_CPU1)[offset & 0xff];
}


static READ_HANDLER( skyraid_alpha_num_r)
{
	return skyraid_alpha_num_ram[offset & 0x7f];
}


static READ_HANDLER( skyraid_port_0_r )
{
	UINT8 val = readinputport(0);

	if (readinputport(4) > analog_range)
		val |= 0x40;
	if (readinputport(5) > analog_range)
		val |= 0x80;

	return val;
}


static WRITE_HANDLER( skyraid_zeropage_w )
{
	memory_region(REGION_CPU1)[offset & 0xff] = data;
}


static WRITE_HANDLER( skyraid_alpha_num_w )
{
	skyraid_alpha_num_ram[offset & 0x7f] = data;
}


static WRITE_HANDLER( skyraid_sound_w )
{
	/* BIT0 => PLANE SWEEP */
	/* BIT1 => MISSILE     */
	/* BIT2 => EXPLOSION   */
	/* BIT3 => START LAMP  */
	/* BIT4 => PLANE ON    */
	/* BIT5 => ATTRACT     */

	set_led_status(0, !(data & 0x08));
}


static WRITE_HANDLER( skyraid_range_w )
{
	analog_range = data & 0x3f;
}


static WRITE_HANDLER( skyraid_offset_w )
{
	analog_offset = data & 0x3f;
}


static WRITE_HANDLER( skyraid_scroll_w )
{
	skyraid_scroll = data;
}


static MEMORY_READ_START( skyraid_readmem )
	{ 0x0000, 0x00ff, MRA_RAM },
	{ 0x0100, 0x03ff, skyraid_zeropage_r },
	{ 0x0800, 0x087f, MRA_RAM },
	{ 0x0880, 0x0bff, skyraid_alpha_num_r },
	{ 0x1000, 0x1000, skyraid_port_0_r },
	{ 0x1000, 0x1001, input_port_1_r },
	{ 0x1400, 0x1400, input_port_2_r },
	{ 0x1400, 0x1401, input_port_3_r },
	{ 0x7000, 0x7fff, MRA_ROM },
	{ 0xf000, 0xffff, MRA_ROM },
MEMORY_END


static MEMORY_WRITE_START( skyraid_writemem )
	{ 0x0000, 0x00ff, MWA_RAM },
	{ 0x0100, 0x03ff, skyraid_zeropage_w },
	{ 0x0400, 0x040f, MWA_RAM, &skyraid_pos_ram },
	{ 0x0800, 0x087f, MWA_RAM, &skyraid_alpha_num_ram },
	{ 0x0880, 0x0bff, skyraid_alpha_num_w },
	{ 0x1c00, 0x1c0f, MWA_RAM, &skyraid_obj_ram },
	{ 0x4000, 0x4000, skyraid_scroll_w },
	{ 0x4400, 0x4400, skyraid_sound_w },
	{ 0x4800, 0x4800, skyraid_range_w },
	{ 0x5000, 0x5000, watchdog_reset_w },
	{ 0x5800, 0x5800, skyraid_offset_w },
	{ 0x7000, 0x7fff, MWA_ROM },
	{ 0xf000, 0xffff, MWA_ROM },
MEMORY_END


INPUT_PORTS_START( skyraid )
	PORT_START
	PORT_DIPNAME( 0x30, 0x00, "Language" )
	PORT_DIPSETTING(    0x00, "English" )
	PORT_DIPSETTING(    0x10, "French" )
	PORT_DIPSETTING(    0x20, "German" )
	PORT_DIPSETTING(    0x30, "Spanish" )
	PORT_BIT (0x40, IP_ACTIVE_HIGH, IPT_UNUSED) /* POT1 */
	PORT_BIT (0x80, IP_ACTIVE_HIGH, IPT_UNUSED) /* POT0 */

	PORT_START
	PORT_DIPNAME( 0x30, 0x10, "Play Time" )
	PORT_DIPSETTING(    0x00, "60 Seconds" )
	PORT_DIPSETTING(    0x10, "80 Seconds" )
	PORT_DIPSETTING(    0x20, "100 Seconds" )
	PORT_DIPSETTING(    0x30, "120 Seconds" )
	PORT_DIPNAME( 0x40, 0x40, "DIP #5" )	/* must be OFF */
	PORT_DIPSETTING(    0x40, DEF_STR( Off ))
	PORT_DIPSETTING(    0x00, DEF_STR( On ))
	PORT_DIPNAME( 0x80, 0x00, "Extended Play" )
	PORT_DIPSETTING(    0x80, DEF_STR( No ))
	PORT_DIPSETTING(    0x00, DEF_STR( Yes ))

	/* coinage settings are insane, refer to the manual */

	PORT_START
	PORT_DIPNAME( 0x0F, 0x01, DEF_STR( Coinage )) /* dial */
	PORT_DIPSETTING(    0x00, "Mode 0" )
	PORT_DIPSETTING(    0x01, "Mode 1" )
	PORT_DIPSETTING(    0x02, "Mode 2" )
	PORT_DIPSETTING(    0x03, "Mode 3" )
	PORT_DIPSETTING(    0x04, "Mode 4" )
	PORT_DIPSETTING(    0x05, "Mode 5" )
	PORT_DIPSETTING(    0x06, "Mode 6" )
	PORT_DIPSETTING(    0x07, "Mode 7" )
	PORT_DIPSETTING(    0x08, "Mode 8" )
	PORT_DIPSETTING(    0x09, "Mode 9" )
	PORT_DIPSETTING(    0x0A, "Mode A" )
	PORT_DIPSETTING(    0x0B, "Mode B" )
	PORT_DIPSETTING(    0x0C, "Mode C" )
	PORT_DIPSETTING(    0x0D, "Mode D" )
	PORT_DIPSETTING(    0x0E, "Mode E" )
	PORT_DIPSETTING(    0x0F, "Mode F" )
	PORT_DIPNAME( 0x10, 0x10, "Score for Extended Play" )
	PORT_DIPSETTING(    0x00, "Low" )
	PORT_DIPSETTING(    0x10, "High" )
	PORT_BIT (0x20, IP_ACTIVE_LOW, IPT_BUTTON1)
	PORT_BIT (0x40, IP_ACTIVE_HIGH, IPT_COIN1)
	PORT_BIT (0x80, IP_ACTIVE_HIGH, IPT_COIN2)

	PORT_START
	PORT_BIT (0x10, IP_ACTIVE_LOW, IPT_TILT)
	PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_BUTTON7, "Hiscore Reset", KEYCODE_H, IP_JOY_DEFAULT)
	PORT_BIT (0x40, IP_ACTIVE_LOW, IPT_START1)
	PORT_SERVICE(0x80, IP_ACTIVE_LOW)

	PORT_START
	PORT_ANALOG( 0x3f, 0x20, IPT_AD_STICK_Y | IPF_REVERSE, 10, 10, 0, 63 )

	PORT_START
	PORT_ANALOG( 0x3f, 0x20, IPT_AD_STICK_X, 10, 10, 0, 63 )
INPUT_PORTS_END


static struct GfxLayout skyraid_text_layout =
{
	16, 8,  /* width, height */
	64,     /* total         */
	1,      /* planes        */
	{ 0 },  /* plane offsets */
	{
		0, 0, 1, 1, 2, 2, 3, 3,
		4, 4, 5, 5, 6, 6, 7, 7
	},
	{
		0x38, 0x30, 0x28, 0x20, 0x18, 0x10, 0x08, 0x00
	},
	0x40
};


static struct GfxLayout skyraid_sprite_layout =
{
	32, 32, /* width, height */
	8,      /* total         */
	2,      /* planes        */
	        /* plane offsets */
	{ 0, 1 },
	{
		0x00, 0x02, 0x04, 0x06, 0x08, 0x0A, 0x0C, 0x0E,
		0x10, 0x12, 0x14, 0x16, 0x18, 0x1A, 0x1C, 0x1E,
		0x20, 0x22, 0x24, 0x26, 0x28, 0x2A, 0x2C, 0x2E,
		0x30, 0x32, 0x34, 0x36, 0x38, 0x3A, 0x3C, 0x3E
	},
	{
		0x000, 0x040, 0x080, 0x0C0, 0x100, 0x140, 0x180, 0x1C0,
		0x200, 0x240, 0x280, 0x2C0, 0x300, 0x340, 0x380, 0x3C0,
		0x400, 0x440, 0x480, 0x4C0, 0x500, 0x540, 0x580, 0x5C0,
		0x600, 0x640, 0x680, 0x6C0, 0x700, 0x740, 0x780, 0x7C0
	},
	0x800
};


static struct GfxLayout skyraid_missile_layout =
{
	16, 16, /* width, height */
	8,      /* total         */
	1,      /* planes        */
	{ 0 },  /* plane offsets */
	{
		0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
		0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F
	},
	{
		0x00, 0x10, 0x20, 0x30, 0x40, 0x50, 0x60, 0x70,
		0x80, 0x90, 0xA0, 0xB0, 0xC0, 0xD0, 0xE0, 0xF0
	},
	0x100
};


static struct GfxDecodeInfo skyraid_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &skyraid_text_layout, 18, 1 },
	{ REGION_GFX2, 0, &skyraid_sprite_layout, 8, 2 },
	{ REGION_GFX3, 0, &skyraid_missile_layout, 16, 1 },
	{ -1 }
};


static MACHINE_DRIVER_START( skyraid )

	/* basic machine hardware */
	MDRV_CPU_ADD(M6502, 12096000 / 12)
	MDRV_CPU_MEMORY(skyraid_readmem, skyraid_writemem)
	MDRV_CPU_VBLANK_INT(irq0_line_hold, 1)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(22 * 1000000 / 15750)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(512, 240)
	MDRV_VISIBLE_AREA(0, 511, 0, 239)
	MDRV_GFXDECODE(skyraid_gfxdecodeinfo)

	MDRV_PALETTE_INIT(skyraid)
	MDRV_PALETTE_LENGTH(20)

	MDRV_VIDEO_START(skyraid)
	MDRV_VIDEO_UPDATE(skyraid)

	/* sound hardware */
MACHINE_DRIVER_END


ROM_START( skyraid )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "030595.e1", 0x7000, 0x800, CRC(c6cb3a2b) SHA1(e4cb8d259446d0614c0c8f097f97dcf21869782e) )
	ROM_RELOAD(            0xF000, 0x800 )
	ROM_LOAD( "030594.d1", 0x7800, 0x800, CRC(27979e96) SHA1(55ffe3094c6764e6b99ee148e3dd730ca263fa3a) )
	ROM_RELOAD(            0xF800, 0x800 )

	ROM_REGION( 0x0200, REGION_GFX1, ROMREGION_DISPOSE ) /* alpha numerics */
	ROM_LOAD( "030598.h2", 0x0000, 0x200, CRC(2a7c5fa0) SHA1(93a79e5948dfcd9b6c2ff390e85a43f7a8cac327) )

	ROM_REGION( 0x0800, REGION_GFX2, ROMREGION_DISPOSE ) /* sprites */
	ROM_LOAD( "030599.m7", 0x0000, 0x800, CRC(0cd179ea) SHA1(e3c763f76e6103e5909e7b5a979206b262d6e96a) )

	ROM_REGION( 0x0100, REGION_GFX3, ROMREGION_DISPOSE ) /* missiles */
	ROM_LOAD_NIB_LOW ( "030597.n5", 0x0000, 0x100, CRC(319ff49c) SHA1(ff4d8b20436179910bf30c720d98df4678f683a9) )
	ROM_LOAD_NIB_HIGH( "030596.m4", 0x0000, 0x100, CRC(30454ed0) SHA1(4216a54c13d9c4803f88f2de35cdee31290bb15e) )

	ROM_REGION( 0x0800, REGION_USER1, 0 ) /* terrain */
	ROM_LOAD_NIB_LOW ( "030584.j5", 0x0000, 0x800, CRC(81f6e8a5) SHA1(ad77b469ed0c9d5dfaa221ecf47d0db4a7f7ac91) )
	ROM_LOAD_NIB_HIGH( "030585.k5", 0x0000, 0x800, CRC(b49bec3f) SHA1(b55d25230ec11c52e7b47d2c10194a49adbeb50a) )

	ROM_REGION( 0x0100, REGION_USER2, 0 ) /* trapezoid */
	ROM_LOAD_NIB_LOW ( "030582.a6", 0x0000, 0x100, CRC(0eacd595) SHA1(5469e312a1f522ce0a61054b50895a5b1a3f19ba) )
	ROM_LOAD_NIB_HIGH( "030583.b6", 0x0000, 0x100, CRC(3edd6fbc) SHA1(0418ea78cf51e18c51087b43a41cd9e13aac0a16) )

	ROM_REGION( 0x0300, REGION_PROMS, 0 )
	ROM_LOAD( "006559.c4", 0x0200, 0x100, CRC(5a8d0e42) SHA1(772220c4c24f18769696ddba26db2bc2e5b0909d) ) /* sync */
ROM_END


GAMEX( 1978, skyraid, 0, skyraid, skyraid, 0, ORIENTATION_FLIP_Y, "Atari", "Sky Raider", GAME_NO_SOUND | GAME_IMPERFECT_COLORS )
