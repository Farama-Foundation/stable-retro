/***************************************************************************

	Atari Runaway hardware

	Games supported:
		* Qwak (prototype)
		* Runaway (prototype)

	original Qwak driver written by Mike Balfour

****************************************************************************/

#include "driver.h"
#include "machine/atari_vg.h"

extern VIDEO_START( runaway );
extern VIDEO_START( qwak );
extern VIDEO_UPDATE( runaway );
extern VIDEO_UPDATE( qwak );

extern UINT8* runaway_video_ram;
extern UINT8* runaway_sprite_ram;

extern WRITE_HANDLER( runaway_paletteram_w );
extern WRITE_HANDLER( runaway_video_ram_w );
extern WRITE_HANDLER( runaway_tile_bank_w );


static void interrupt_callback(int scanline)
{
	/* assume Centipede-style interrupt timing */

	cpu_set_irq_line(0, 0, (scanline & 32) ? ASSERT_LINE : CLEAR_LINE);

	scanline += 32;

	if (scanline >= 263)
	{
		scanline = 16;
	}

	timer_set(cpu_getscanlinetime(scanline), scanline, interrupt_callback);
}


static MACHINE_INIT( runaway )
{
	timer_set(cpu_getscanlinetime(16), 16, interrupt_callback);
}


static READ_HANDLER( runaway_input_r )
{
	UINT8 val = 0;

	if (readinputport(0) & (1 << offset))
	{
		val |= 0x80;
	}
	if (readinputport(1) & (1 << offset))
	{
		val |= 0x40;
	}

	return val;
}


static READ_HANDLER( runaway_pot_r )
{
	return (readinputport(4) << (7 - offset)) & 0x80;
}


static WRITE_HANDLER( runaway_led_w )
{
	set_led_status(offset, ~data & 1);
}


static WRITE_HANDLER( runaway_irq_ack_w )
{
	cpu_set_irq_line(0, 0, CLEAR_LINE);
}


static MEMORY_READ_START( readmem )
	{ 0x0000, 0x03ff, MRA_RAM },
	{ 0x0400, 0x07ff, MRA_RAM },
	{ 0x3000, 0x3007, runaway_input_r },
	{ 0x4000, 0x4000, input_port_2_r },
	{ 0x5000, 0x5000, atari_vg_earom_r },
	{ 0x6000, 0x600f, pokey1_r },
	{ 0x7000, 0x700f, pokey2_r },
	{ 0x8000, 0xcfff, MRA_ROM },
	{ 0xf000, 0xffff, MRA_ROM },	/* for the interrupt vectors */
MEMORY_END


static MEMORY_WRITE_START( writemem )
	{ 0x0000, 0x03ff, MWA_RAM },
	{ 0x0400, 0x07bf, runaway_video_ram_w, &runaway_video_ram },
	{ 0x07c0, 0x07ff, MWA_RAM, &runaway_sprite_ram },
	{ 0x1000, 0x1000, runaway_irq_ack_w },
	{ 0x1400, 0x143F, atari_vg_earom_w },
	{ 0x1800, 0x1800, atari_vg_earom_ctrl_w },
	{ 0x1c00, 0x1c0f, runaway_paletteram_w },
	{ 0x2000, 0x2000, MWA_NOP }, /* coin counter? */
	{ 0x2001, 0x2001, MWA_NOP }, /* coin counter? */
	{ 0x2003, 0x2004, runaway_led_w },
	{ 0x2005, 0x2005, runaway_tile_bank_w },
	{ 0x6000, 0x600f, pokey1_w },
	{ 0x7000, 0x700f, pokey2_w },
	{ 0x8000, 0xcfff, MWA_ROM },
MEMORY_END


INPUT_PORTS_START( qwak )
	PORT_START	/* 3000 D7 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_VBLANK )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN3 )

	PORT_START	/* 3000 D6 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* 4000 */
	PORT_DIPNAME( 0x01, 0x00, "DIP 1" )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ))
	PORT_DIPSETTING(    0x00, DEF_STR( On ))
	PORT_DIPNAME( 0x02, 0x00, "DIP 2" )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ))
	PORT_DIPSETTING(    0x00, DEF_STR( On ))
	PORT_DIPNAME( 0x04, 0x00, "DIP 3" )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ))
	PORT_DIPSETTING(    0x00, DEF_STR( On ))
	PORT_DIPNAME( 0x08, 0x00, "DIP 4" )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ))
	PORT_DIPSETTING(    0x00, DEF_STR( On ))
	PORT_DIPNAME( 0x10, 0x00, "DIP 5" )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ))
	PORT_DIPSETTING(    0x00, DEF_STR( On ))
	PORT_DIPNAME( 0x20, 0x00, "DIP 6" )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ))
	PORT_DIPSETTING(    0x00, DEF_STR( On ))
	PORT_DIPNAME( 0x40, 0x00, "DIP 7" )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ))
	PORT_DIPSETTING(    0x00, DEF_STR( On ))
	PORT_DIPNAME( 0x80, 0x00, "DIP 8" )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ))
	PORT_DIPSETTING(    0x00, DEF_STR( On ))

	PORT_START /* 6008 not used */
	PORT_START /* 7000 not used */
INPUT_PORTS_END


INPUT_PORTS_START( runaway )
	PORT_START /* 3000 D7 */
	PORT_BIT ( 0x01, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT ( 0x02, IP_ACTIVE_HIGH, IPT_VBLANK )
	PORT_BIT ( 0x04, IP_ACTIVE_LOW, IPT_TILT )
	PORT_BIT ( 0x08, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_SERVICE( 0x10, IP_ACTIVE_LOW )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN3 )

	PORT_START /* 3000 D6 */
	PORT_BIT ( 0x01, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT ( 0x02, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT ( 0x04, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT ( 0x08, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT ( 0x10, IP_ACTIVE_LOW, IPT_START1 )	/* also level skip if invincibility is on */
	PORT_BIT ( 0x20, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT ( 0x40, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT ( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START /* 4000 */
	PORT_DIPNAME( 0x03, 0x01, DEF_STR( Coinage ))
	PORT_DIPSETTING(    0x00, DEF_STR( 2C_1C ))
	PORT_DIPSETTING(    0x01, DEF_STR( 1C_1C ))
	PORT_DIPSETTING(    0x02, DEF_STR( 1C_2C ))
	PORT_DIPSETTING(    0x03, DEF_STR( Free_Play ))
	PORT_DIPNAME( 0x0c, 0x04, "Coin 3 Multiplier" )
	PORT_DIPSETTING(    0x0c, "1" )
	PORT_DIPSETTING(    0x08, "4" )
	PORT_DIPSETTING(    0x04, "5" )
	PORT_DIPSETTING(    0x00, "6" )
	PORT_DIPNAME( 0x10, 0x10, "Coin 2 Multiplier" )
	PORT_DIPSETTING(    0x10, "1" )
	PORT_DIPSETTING(    0x00, "2" )
	PORT_DIPNAME( 0x20, 0x00, DEF_STR( Unknown ))
	PORT_DIPSETTING(    0x20, DEF_STR( Off ))
	PORT_DIPSETTING(    0x00, DEF_STR( On ))
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Unknown ))
	PORT_DIPSETTING(    0x40, DEF_STR( Off ))
	PORT_DIPSETTING(    0x00, DEF_STR( On ))
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Unknown ))
	PORT_DIPSETTING(    0x80, DEF_STR( Off ))
	PORT_DIPSETTING(    0x00, DEF_STR( On ))

	PORT_START /* 6008 */
	PORT_DIPNAME( 0x03, 0x01, DEF_STR( Lives ))
	PORT_DIPSETTING(    0x00, "2" )
	PORT_DIPSETTING(    0x01, "3" )
	PORT_DIPSETTING(    0x02, "4" )
	PORT_DIPSETTING(    0x03, "5" )
	PORT_DIPNAME( 0x0c, 0x08, DEF_STR( Bonus_Life ))
	PORT_DIPSETTING(    0x00, "Never" )
	PORT_DIPSETTING(    0x04, "Every 5000" )
	PORT_DIPSETTING(    0x08, "Every 10000" )
	PORT_DIPSETTING(    0x0c, "Every 15000" )
	PORT_DIPNAME( 0x10, 0x00, DEF_STR( Unused ))
	PORT_DIPSETTING(    0x10, DEF_STR( Off ))
	PORT_DIPSETTING(    0x00, DEF_STR( On ))
	PORT_DIPNAME( 0x20, 0x00, DEF_STR( Unused ))
	PORT_DIPSETTING(    0x20, DEF_STR( Off ))
	PORT_DIPSETTING(    0x00, DEF_STR( On ))
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Difficulty ))
	PORT_DIPSETTING(    0x00, "Easy" )
	PORT_DIPSETTING(    0x40, "Hard" )
	PORT_DIPNAME( 0x80, 0x00, "Invincibility" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ))
	PORT_DIPSETTING(    0x80, DEF_STR( On ))

	PORT_START /* 7000 */
	PORT_BIT ( 0x01, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_TOGGLE )
	PORT_BIT ( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT ( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT ( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT ( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_DIPNAME( 0x60, 0x00, "Language" )
	PORT_DIPSETTING(    0x00, "English" )
	PORT_DIPSETTING(    0x20, "German" )
	PORT_DIPSETTING(    0x40, "French" )
	PORT_DIPSETTING(    0x60, "Spanish" )
	PORT_BIT ( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )
INPUT_PORTS_END


static struct GfxLayout runaway_tile_layout =
{
	8, 8,
	256,
	3,
	{
		RGN_FRAC(2,3), RGN_FRAC(1,3), RGN_FRAC(0,3)
	},
	{
		0, 1, 2, 3, 4, 5, 6, 7
	},
	{
		0x00, 0x08, 0x10, 0x18, 0x20, 0x28, 0x30, 0x38
	},
	0x40
};


static struct GfxLayout qwak_tile_layout =
{
	8, 8,
	256,
	4,
	{
		RGN_FRAC(3,4), RGN_FRAC(2,4), RGN_FRAC(1,4), RGN_FRAC(0,4)
	},
	{
		0, 1, 2, 3, 4, 5, 6, 7
	},
	{
		0x00, 0x08, 0x10, 0x18, 0x20, 0x28, 0x30, 0x38
	},
	0x40
};


static struct GfxLayout runaway_sprite_layout =
{
	8, 16,
	384,
	3,
	{
		RGN_FRAC(2,3), RGN_FRAC(1,3), RGN_FRAC(0,3)
	},
	{
		0, 1, 2, 3, 4, 5, 6, 7
	},
	{
		0x00, 0x08, 0x10, 0x18, 0x20, 0x28, 0x30, 0x38,
		0x40, 0x48, 0x50, 0x58, 0x60, 0x68, 0x70, 0x78
	},
	0x80
};


static struct GfxLayout qwak_sprite_layout =
{
	8, 16,
	128,
	4,
	{
		RGN_FRAC(3,4), RGN_FRAC(2,4), RGN_FRAC(1,4), RGN_FRAC(0,4)
	},
	{
		0, 1, 2, 3, 4, 5, 6, 7
	},
	{
		0x00, 0x08, 0x10, 0x18, 0x20, 0x28, 0x30, 0x38,
		0x40, 0x48, 0x50, 0x58, 0x60, 0x68, 0x70, 0x78
	},
	0x80
};


static struct GfxDecodeInfo runaway_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0x000, &runaway_tile_layout,   0, 1 },
	{ REGION_GFX1, 0x800, &runaway_sprite_layout, 8, 1 },
	{ -1 }
};


static struct GfxDecodeInfo qwak_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0x800, &qwak_tile_layout,   0, 1 },
	{ REGION_GFX1, 0x000, &qwak_sprite_layout, 0, 1 },
	{ -1 }
};


static struct POKEYinterface pokey_interface =
{
	2,
	12096000 / 8,
	{ 100, 100 },
	{ 0, runaway_pot_r },
	{ 0, runaway_pot_r },
	{ 0, runaway_pot_r },
	{ 0, runaway_pot_r },
	{ 0, runaway_pot_r },
	{ 0, runaway_pot_r },
	{ 0, runaway_pot_r },
	{ 0, runaway_pot_r },
	{ input_port_3_r, 0 },
};


static MACHINE_DRIVER_START( runaway )

	/* basic machine hardware */
	MDRV_CPU_ADD(M6502, 12096000 / 8) /* ? */
	MDRV_CPU_MEMORY(readmem, writemem)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(23. * 1000000 / 15750)

	MDRV_MACHINE_INIT(runaway)
	MDRV_NVRAM_HANDLER(atari_vg)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(256, 240)
	MDRV_VISIBLE_AREA(0, 255, 0, 239)
	MDRV_GFXDECODE(runaway_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(16)

	MDRV_VIDEO_START(runaway)
	MDRV_VIDEO_UPDATE(runaway)

	/* sound hardware */
	MDRV_SOUND_ADD(POKEY, pokey_interface)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( qwak )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(runaway)

	/* video hardware */
	MDRV_GFXDECODE(qwak_gfxdecodeinfo)

	MDRV_VIDEO_START(qwak)
	MDRV_VIDEO_UPDATE(qwak)

MACHINE_DRIVER_END


ROM_START( runaway )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "choo8000.d1", 0x8000, 0x1000, CRC(35794abe) SHA1(5ce872bda8bb2ed4888ba8b47ecd1afbe24b22eb) )
	ROM_LOAD( "choo9000.d1", 0x9000, 0x1000, CRC(0d63756d) SHA1(2549a57ca106635f5c53ea1b03f5a0d6e901ab47) )
	ROM_LOAD( "chooa000.e1", 0xa000, 0x1000, CRC(e6806b6b) SHA1(c260eaa35cbc46f0c0fd4006ec6d04315c3bb851) )
	ROM_LOAD( "choob000.f1", 0xb000, 0x1000, CRC(6aa52bc4) SHA1(5992d441ae8607859abd111c946783036ef6253f) )
	ROM_LOAD( "chooc000",    0xc000, 0x1000, CRC(452ddea2) SHA1(1072de8935aae23eb1ef7b16e308180cd3e91da2) )
	ROM_RELOAD(              0xf000, 0x1000 )

	ROM_REGION( 0x6000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "chop0", 0x0000, 0x2000, CRC(225a8c5e) SHA1(13394320640355d67414e085ad28364814147b63) )
	ROM_LOAD( "chop1", 0x2000, 0x2000, CRC(70389c0f) SHA1(6baf4a17c11e9b27a1e09cce301f931f5099978d) )
	ROM_LOAD( "chop2", 0x4000, 0x2000, CRC(63655f1c) SHA1(c235be3945067c873c03ce8a0c5cfb76984f66ff) )
ROM_END


ROM_START( qwak )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "qwak8000.bin", 0x8000, 0x1000, CRC(4d002d8a) SHA1(8621e7ec1ab3cb8d003858227e858354cd79dbf1) )
	ROM_LOAD( "qwak9000.bin", 0x9000, 0x1000, CRC(e0c78fd7) SHA1(f5f397950971d12a7ae47fc64aa8f5751463b8a5) )
	ROM_LOAD( "qwaka000.bin", 0xa000, 0x1000, CRC(e5770fc9) SHA1(c9556e9c2f7b6c37755ac9f10d95027118317b4a) )
	ROM_LOAD( "qwakb000.bin", 0xb000, 0x1000, CRC(90771cc0) SHA1(5715e5bfccb05c51d871b443e42b0950ec23e330) )
	ROM_RELOAD(               0xf000, 0x1000 )

	ROM_REGION( 0x4000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "qwakgfx0.bin", 0x0000, 0x1000, CRC(bed2c067) SHA1(53d909b414042d54fe2e86ae0d6c7a4ded16b87e) )
	ROM_LOAD( "qwakgfx1.bin", 0x1000, 0x1000, CRC(73a31d28) SHA1(bbe076432866398bcd02962dd90eb178e3a38fb1) )
	ROM_LOAD( "qwakgfx2.bin", 0x2000, 0x1000, CRC(07fd9e80) SHA1(83d5f22b8316ac7e88d8ecdb238182a35a6f6362) )
	ROM_LOAD( "qwakgfx3.bin", 0x3000, 0x1000, CRC(e8416f2b) SHA1(171f6539575f2c06b431ab5118e5cbaf740f557d) )
ROM_END


GAME( 1982, qwak,    0, qwak,    qwak,    0, ROT270, "Atari", "Qwak (prototype)" )
GAME( 1982, runaway, 0, runaway, runaway, 0, ROT0,   "Atari", "Runaway (prototype)" )
