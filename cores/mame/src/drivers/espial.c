/***************************************************************************

 Espial hardware games

***************************************************************************/

#include "driver.h"
#include "espial.h"
#include "cpu/z80/z80.h"


MACHINE_INIT( espial )
{
	/* we must start with NMI interrupts disabled */
	/*interrupt_enable = 0;*/
	cpu_interrupt_enable(0,0);
}


WRITE_HANDLER( zodiac_master_interrupt_enable_w )
{
	interrupt_enable_w(offset,~data & 1);
}


INTERRUPT_GEN( zodiac_master_interrupt )
{
	if (cpu_getiloops() == 0)
		nmi_line_pulse();
	else
		irq0_line_hold();
}


WRITE_HANDLER( zodiac_master_soundlatch_w )
{
	soundlatch_w(offset, data);
	cpu_set_irq_line(1, 0, HOLD_LINE);
}



static MEMORY_READ_START( espial_readmem )
	{ 0x0000, 0x4fff, MRA_ROM },
	{ 0x5800, 0x5fff, MRA_RAM },
	{ 0x6081, 0x6081, input_port_0_r },
	{ 0x6082, 0x6082, input_port_1_r },
	{ 0x6083, 0x6083, input_port_2_r },
	{ 0x6084, 0x6084, input_port_3_r },
	{ 0x6090, 0x6090, soundlatch_r },	/* the main CPU reads the command back from the slave */
	{ 0x7000, 0x7000, watchdog_reset_r },
	{ 0x8000, 0x803f, MRA_RAM },
	{ 0x8400, 0x87ff, MRA_RAM },
	{ 0x8c00, 0x903f, MRA_RAM },
	{ 0x9400, 0x97ff, MRA_RAM },
	{ 0xc000, 0xcfff, MRA_ROM },
MEMORY_END

static MEMORY_WRITE_START( espial_writemem )
	{ 0x0000, 0x4fff, MWA_ROM },
	{ 0x5800, 0x5fff, MWA_RAM },
	{ 0x6090, 0x6090, zodiac_master_soundlatch_w },
	{ 0x7000, 0x7000, watchdog_reset_w },
	{ 0x7100, 0x7100, zodiac_master_interrupt_enable_w },
	{ 0x7200, 0x7200, espial_flipscreen_w },
	{ 0x8000, 0x801f, MWA_RAM, &espial_spriteram_1 },
	{ 0x8400, 0x87ff, espial_videoram_w, &espial_videoram },
	{ 0x8800, 0x880f, MWA_RAM, &espial_spriteram_3 },
	{ 0x8c00, 0x8fff, espial_attributeram_w, &espial_attributeram },
	{ 0x9000, 0x901f, MWA_RAM, &espial_spriteram_2 },
	{ 0x9020, 0x903f, espial_scrollram_w, &espial_scrollram },
	{ 0x9400, 0x97ff, espial_colorram_w, &espial_colorram },
	{ 0xc000, 0xcfff, MWA_ROM },
MEMORY_END


/* there are a lot of unmapped reads from all over memory as the
   code uses POP instructions in a delay loop */
static MEMORY_READ_START( netwars_readmem )
	{ 0x0000, 0x3fff, MRA_ROM },
	{ 0x5800, 0x5fff, MRA_RAM },
	{ 0x6081, 0x6081, input_port_0_r },
	{ 0x6082, 0x6082, input_port_1_r },
	{ 0x6083, 0x6083, input_port_2_r },
	{ 0x6084, 0x6084, input_port_3_r },
	{ 0x6090, 0x6090, soundlatch_r },	/* the main CPU reads the command back from the slave */
	{ 0x7000, 0x7000, watchdog_reset_r },
	{ 0x8000, 0x97ff, MRA_RAM },
MEMORY_END

static MEMORY_WRITE_START( netwars_writemem )
	{ 0x0000, 0x3fff, MWA_ROM },
	{ 0x5800, 0x5fff, MWA_RAM },
	{ 0x6090, 0x6090, zodiac_master_soundlatch_w },
	{ 0x7000, 0x7000, watchdog_reset_w },
	{ 0x7100, 0x7100, zodiac_master_interrupt_enable_w },
	{ 0x7200, 0x7200, espial_flipscreen_w },
	{ 0x8000, 0x801f, MWA_RAM, &espial_spriteram_1 },
	{ 0x8000, 0x87ff, espial_videoram_w, &espial_videoram },
	{ 0x8800, 0x880f, MWA_RAM, &espial_spriteram_3 },
	{ 0x8800, 0x8fff, espial_attributeram_w, &espial_attributeram },
	{ 0x9000, 0x901f, MWA_RAM, &espial_spriteram_2 },
	{ 0x9020, 0x903f, espial_scrollram_w, &espial_scrollram },
	{ 0x9000, 0x97ff, espial_colorram_w, &espial_colorram },
MEMORY_END


static MEMORY_READ_START( sound_readmem )
	{ 0x0000, 0x1fff, MRA_ROM },
	{ 0x2000, 0x23ff, MRA_RAM },
	{ 0x6000, 0x6000, soundlatch_r },
MEMORY_END

static MEMORY_WRITE_START( sound_writemem )
	{ 0x0000, 0x1fff, MWA_ROM },
	{ 0x2000, 0x23ff, MWA_RAM },
	{ 0x4000, 0x4000, interrupt_enable_w },
	{ 0x6000, 0x6000, soundlatch_w },
MEMORY_END

static PORT_WRITE_START( sound_writeport )
	{ 0x00, 0x00, AY8910_control_port_0_w },
	{ 0x01, 0x01, AY8910_write_port_0_w },
PORT_END


INPUT_PORTS_START( espial )
	PORT_START	/* IN0 */
	PORT_DIPNAME( 0x01, 0x00, "Number of Buttons" )
	PORT_DIPSETTING(    0x01, "1" )
	PORT_DIPSETTING(    0x00, "2" )
	PORT_DIPNAME( 0x02, 0x02, "Enemy Bullets Vulnerable" )	/* you can shoot bullets */
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x02, DEF_STR( On ) )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START	/* IN1 */
	PORT_DIPNAME( 0x03, 0x00, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x01, "4" )
	PORT_DIPSETTING(    0x02, "5" )
	PORT_DIPSETTING(    0x03, "6" )
	PORT_DIPNAME( 0x1c, 0x00, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x14, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x18, DEF_STR( 3C_2C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(    0x1c, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0x20, 0x00, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(	0x00, "20k and every 70k" )
	PORT_DIPSETTING(	0x20, "50k and every 100k" )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(	0x40, DEF_STR( Upright ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x80, DEF_STR( On ) )

	PORT_START	/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN  | IPF_8WAY )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_COCKTAIL )

	PORT_START	/* IN3 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP    | IPF_8WAY )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_8WAY )
INPUT_PORTS_END


INPUT_PORTS_START( netwars )
	PORT_START	/* IN0 */
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Unknown ) )	/* probably unused */
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x01, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Unknown ) )	/* used */
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x02, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x00, DEF_STR( Unknown ) )	/* probably unused */
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x04, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Unknown ) )	/* used */
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START	/* IN1 */
	PORT_DIPNAME( 0x03, 0x00, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x01, "4" )
	PORT_DIPSETTING(    0x02, "5" )
	PORT_DIPSETTING(    0x03, "6" )
	PORT_DIPNAME( 0x1c, 0x00, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x14, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x18, DEF_STR( 3C_2C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(    0x1c, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0x20, 0x00, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(	0x00, "20k and every 70k" )
	PORT_DIPSETTING(	0x20, "50k and every 100k" )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(	0x40, DEF_STR( Upright ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x80, DEF_STR( On ) )

	PORT_START	/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP    | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN  | IPF_4WAY )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN  | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START	/* IN3 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_4WAY )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP    | IPF_4WAY )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_4WAY )
INPUT_PORTS_END


static struct GfxLayout charlayout =
{
	8,8,
	RGN_FRAC(2,2),
	2,
	{ 0, 4 },
	{ STEP4(0,1), STEP4(8*8,1) },
	{ STEP8(0,8) },
	16*8
};

static struct GfxLayout spritelayout =
{
	16,16,
	RGN_FRAC(1,2),
	2,
	{ RGN_FRAC(0,2), RGN_FRAC(1,2) },
	{ STEP8(0,1), STEP8(8*8,1) },
	{ STEP8(0,8), STEP8(16*8,8) },
	32*8
};


static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &charlayout,    0, 64 },
	{ REGION_GFX2, 0, &spritelayout,  0, 64 },
	{ -1 } /* end of array */
};



static struct AY8910interface ay8910_interface =
{
	1,	/* 1 chip */
	1500000,	/* 1.5 MHz?????? */
	{ 50 },
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 }
};



static MACHINE_DRIVER_START( espial )

	/* basic machine hardware */
	MDRV_CPU_ADD_TAG("main", Z80, 3072000)	/* 3.072 MHz */
	MDRV_CPU_MEMORY(espial_readmem,espial_writemem)
	MDRV_CPU_VBLANK_INT(zodiac_master_interrupt,2)

	MDRV_CPU_ADD(Z80, 3072000)	/* 2 MHz?????? */
	MDRV_CPU_MEMORY(sound_readmem,sound_writemem)
	MDRV_CPU_PORTS(0,sound_writeport)
	MDRV_CPU_VBLANK_INT(nmi_line_pulse,4)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)

	MDRV_MACHINE_INIT(espial)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 2*8, 30*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(256)

	MDRV_PALETTE_INIT(espial)
	MDRV_VIDEO_START(espial)
	MDRV_VIDEO_UPDATE(espial)

	/* sound hardware */
	MDRV_SOUND_ADD(AY8910, ay8910_interface)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( netwars )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(espial)

	MDRV_CPU_MODIFY("main")
	MDRV_CPU_MEMORY(netwars_readmem,netwars_writemem)

	/* video hardware */
	MDRV_SCREEN_SIZE(32*8, 64*8)

	MDRV_VIDEO_START(netwars)
MACHINE_DRIVER_END



/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( espial )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "espial.3",     0x0000, 0x2000, CRC(10f1da30) SHA1(8954ca3c7fccb8dd8433015ee303bb75a98f3474) )
	ROM_LOAD( "espial.4",     0x2000, 0x2000, CRC(d2adbe39) SHA1(13c6041fd0e7c49988af89e3bab1b20999336928) )
	ROM_LOAD( "espial.6",     0x4000, 0x1000, CRC(baa60bc1) SHA1(fc3d3f2e0316efb31161b28984fc8bd94473b783) )
	ROM_LOAD( "espial.5",     0xc000, 0x1000, CRC(6d7bbfc1) SHA1(d886a76ce4a23c1310135bf1e4ffeda6d44625e7) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for the audio CPU */
	ROM_LOAD( "espial.1",     0x0000, 0x1000, CRC(1e5ec20b) SHA1(f3bee38737321edf2d1ea753124421416441666e) )
	ROM_LOAD( "espial.2",     0x1000, 0x1000, CRC(3431bb97) SHA1(97343bfb5e49cd1d26799723d8c5a31eff7b1170) )

	ROM_REGION( 0x3000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "espial.8",     0x0000, 0x2000, CRC(2f43036f) SHA1(316e9fab778d6c0abb0b6673aba33dfbe44b1262) )
	ROM_LOAD( "espial.7",     0x2000, 0x1000, CRC(ebfef046) SHA1(5aa6efb7254fb42e814c1a29c5363f2d0727452f) )

	ROM_REGION( 0x2000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "espial.10",    0x0000, 0x1000, CRC(de80fbc1) SHA1(f5601eac8cb35a92c51bf81e5ac5a2b79bcde28f) )
	ROM_LOAD( "espial.9",     0x1000, 0x1000, CRC(48c258a0) SHA1(55e72b9072ddc05f848e5a6fae159c554102010b) )

	ROM_REGION( 0x0200, REGION_PROMS, 0 )
	ROM_LOAD( "espial.1f",    0x0000, 0x0100, CRC(d12de557) SHA1(53e8a57dfab677cc5b9cdd83d2fbeb93169bcefd) ) /* palette low 4 bits */
	ROM_LOAD( "espial.1h",    0x0100, 0x0100, CRC(4c84fe70) SHA1(7ac52bd5b19663b9526ecb678e61db9939d2285d) ) /* palette high 4 bits */
ROM_END

ROM_START( espiale )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "2764.3",       0x0000, 0x2000, CRC(0973c8a4) SHA1(d1fc6775870710b3dfea4e58a937ab996021adb1) )
	ROM_LOAD( "2764.4",       0x2000, 0x2000, CRC(6034d7e5) SHA1(62c9699088f4ee1c69ec10a2f82feddd4083efef) )
	ROM_LOAD( "2732.6",       0x4000, 0x1000, CRC(357025b4) SHA1(8bc62f564fcbe37bd490452b2d569d1981f76db1) )
	ROM_LOAD( "2732.5",       0xc000, 0x1000, CRC(d03a2fc4) SHA1(791d70e4354350507f4c39d6115c046254168895) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for the audio CPU */
	ROM_LOAD( "2732.1",       0x0000, 0x1000, CRC(fc7729e9) SHA1(96dfec574521fa4fe2588fbac2ef1caba6c1b884) )
	ROM_LOAD( "2732.2",       0x1000, 0x1000, CRC(e4e256da) SHA1(8007471405bdcf90e29657a3ac2c2f84c9db7c9b) )

	ROM_REGION( 0x3000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "espial.8",     0x0000, 0x2000, CRC(2f43036f) SHA1(316e9fab778d6c0abb0b6673aba33dfbe44b1262) )
	ROM_LOAD( "espial.7",     0x2000, 0x1000, CRC(ebfef046) SHA1(5aa6efb7254fb42e814c1a29c5363f2d0727452f) )

	ROM_REGION( 0x2000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "espial.10",    0x0000, 0x1000, CRC(de80fbc1) SHA1(f5601eac8cb35a92c51bf81e5ac5a2b79bcde28f) )
	ROM_LOAD( "espial.9",     0x1000, 0x1000, CRC(48c258a0) SHA1(55e72b9072ddc05f848e5a6fae159c554102010b) )

	ROM_REGION( 0x0200, REGION_PROMS, 0 )
	ROM_LOAD( "espial.1f",    0x0000, 0x0100, CRC(d12de557) SHA1(53e8a57dfab677cc5b9cdd83d2fbeb93169bcefd) ) /* palette low 4 bits */
	ROM_LOAD( "espial.1h",    0x0100, 0x0100, CRC(4c84fe70) SHA1(7ac52bd5b19663b9526ecb678e61db9939d2285d) ) /* palette high 4 bits */
ROM_END

ROM_START( netwars )
	ROM_REGION( 0x10000, REGION_CPU1, 0 ) /* 64k for code */
	ROM_LOAD( "netw3.4f",     0x0000, 0x2000, CRC(8e782991) SHA1(4fd533035b61b7006ef94300bb63474fb9e1c9f0) )
	ROM_LOAD( "netw4.4h",     0x2000, 0x2000, CRC(6e219f61) SHA1(a27304017251777be501861e106a670fff078d54) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* 64k for the audio CPU */
	ROM_LOAD( "netw1.4n",     0x0000, 0x1000, CRC(53939e16) SHA1(938f505db0cfcfafb751378ae0c139b7f32404cb) )
	ROM_LOAD( "netw2.4r",     0x1000, 0x1000, CRC(c096317a) SHA1(e61a3e9107481fd80309172a1a9a431903e02489) )

	ROM_REGION( 0x4000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "netw8.4b",     0x0000, 0x2000, CRC(2320277e) SHA1(4e05f6833de89f8f7cc0a0d1cbec03656f8b54a1) )
	ROM_LOAD( "netw7.4a",     0x2000, 0x2000, CRC(25cc5b7f) SHA1(2e089c3d5f8ebba676a959ba71bc9c1750312721) )

	ROM_REGION( 0x2000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "netw10.4e",    0x0000, 0x1000, CRC(87b65625) SHA1(a702726c0fbe7669604f48bf2c19a54031645731) )
	ROM_LOAD( "netw9.4d",     0x1000, 0x1000, CRC(830d0218) SHA1(c726a4a9dd1f10279f79cbe5fdd693a62d9d3ac5) )

	ROM_REGION( 0x0200, REGION_PROMS, 0 )
	ROM_LOAD( "netw5.1f",     0x0000, 0x0100, CRC(f3ae1fe2) SHA1(4f259f8da3c9ecdc6010f83b6abc1371366bd0ab) ) /* palette low 4 bits */
	ROM_LOAD( "netw6.1h",     0x0100, 0x0100, CRC(c44c3771) SHA1(c86125fac28fafc744957258bf3bb5a6dc664b54) ) /* palette high 4 bits */
ROM_END



GAME( 1983, espial,  0,      espial,  espial,  0, ROT0,  "[Orca] Thunderbolt", "Espial (US[Q])" )
GAME( 1983, espiale, espial, espial,  espial,  0, ROT0,  "[Orca] Thunderbolt", "Espial (Europe)" )
GAME( 1983, netwars, 0,      netwars, netwars, 0, ROT90, "Orca (Esco Trading Co license)", "Net Wars" )
