/***************************************************************************

Solomon's Key

driver by Mirko Buffoni

***************************************************************************/
#include "driver.h"
#include "vidhrdw/generic.h"
#include "cpu/z80/z80.h"

extern UINT8 *solomon_videoram2;
extern UINT8 *solomon_colorram2;

extern WRITE_HANDLER( solomon_videoram_w );
extern WRITE_HANDLER( solomon_colorram_w );
extern WRITE_HANDLER( solomon_videoram2_w );
extern WRITE_HANDLER( solomon_colorram2_w );
extern WRITE_HANDLER( solomon_flipscreen_w );

extern VIDEO_START( solomon );
extern VIDEO_UPDATE( solomon );

static WRITE_HANDLER( solomon_sh_command_w )
{
	soundlatch_w(offset,data);
	cpu_set_irq_line(1,IRQ_LINE_NMI,PULSE_LINE);
}


static MEMORY_READ_START( readmem )
	{ 0x0000, 0xbfff, MRA_ROM },
	{ 0xc000, 0xcfff, MRA_RAM },	/* RAM */
	{ 0xd000, 0xdfff, MRA_RAM },	/* video + color + bg */
	{ 0xe000, 0xe07f, MRA_RAM },	/* spriteram  */
	{ 0xe400, 0xe5ff, MRA_RAM },	/* paletteram */
	{ 0xe600, 0xe600, input_port_0_r },
	{ 0xe601, 0xe601, input_port_1_r },
	{ 0xe602, 0xe602, input_port_2_r },
	{ 0xe604, 0xe604, input_port_3_r },	/* DSW1 */
	{ 0xe605, 0xe605, input_port_4_r },	/* DSW2 */
	{ 0xf000, 0xffff, MRA_ROM },
MEMORY_END

static MEMORY_WRITE_START( writemem )
	{ 0x0000, 0xbfff, MWA_ROM },
	{ 0xc000, 0xcfff, MWA_RAM },
	{ 0xd000, 0xd3ff, solomon_colorram_w, &colorram },
	{ 0xd400, 0xd7ff, solomon_videoram_w, &videoram },
	{ 0xd800, 0xdbff, solomon_colorram2_w, &solomon_colorram2 },
	{ 0xdc00, 0xdfff, solomon_videoram2_w, &solomon_videoram2 },
	{ 0xe000, 0xe07f, MWA_RAM, &spriteram, &spriteram_size },
	{ 0xe400, 0xe5ff, paletteram_xxxxBBBBGGGGRRRR_w, &paletteram },
	{ 0xe600, 0xe600, interrupt_enable_w },
	{ 0xe604, 0xe604, solomon_flipscreen_w },
	{ 0xe800, 0xe800, solomon_sh_command_w },
	{ 0xf000, 0xffff, MWA_ROM },
MEMORY_END

static MEMORY_READ_START( solomon_sound_readmem )
	{ 0x0000, 0x3fff, MRA_ROM },
	{ 0x4000, 0x47ff, MRA_RAM },
	{ 0x8000, 0x8000, soundlatch_r },
MEMORY_END

static MEMORY_WRITE_START( solomon_sound_writemem )
	{ 0x0000, 0x3fff, MWA_ROM },
	{ 0x4000, 0x47ff, MWA_RAM },
	{ 0xffff, 0xffff, MWA_NOP },	/* watchdog? */
MEMORY_END

static PORT_WRITE_START( solomon_sound_writeport )
	{ 0x10, 0x10, AY8910_control_port_0_w },
	{ 0x11, 0x11, AY8910_write_port_0_w },
	{ 0x20, 0x20, AY8910_control_port_1_w },
	{ 0x21, 0x21, AY8910_write_port_1_w },
	{ 0x30, 0x30, AY8910_control_port_2_w },
	{ 0x31, 0x31, AY8910_write_port_2_w },
PORT_END



INPUT_PORTS_START( solomon )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP    | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN  | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON2 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON2 | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START	/* COIN */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START	/* DSW1 */
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x0c, 0x00, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x0c, "2" )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x08, "4" )
	PORT_DIPSETTING(    0x04, "5" )
	PORT_DIPNAME( 0x30, 0x00, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_3C ) )
	PORT_DIPNAME( 0xc0, 0x00, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 1C_3C ) )

	PORT_START	/* DSW2 */
	PORT_DIPNAME( 0x03, 0x00, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x02, "Easy" )
	PORT_DIPSETTING(    0x00, "Normal" )
	PORT_DIPSETTING(    0x01, "Harder" )
	PORT_DIPSETTING(    0x03, "Difficult" )
	PORT_DIPNAME( 0x0c, 0x00, "Timer Speed" )
	PORT_DIPSETTING(    0x08, "Slow" )
	PORT_DIPSETTING(    0x00, "Normal" )
	PORT_DIPSETTING(    0x04, "Faster" )
	PORT_DIPSETTING(    0x0c, "Fastest" )
	PORT_DIPNAME( 0x10, 0x00, "Extra" )
	PORT_DIPSETTING(    0x00, "Normal" )
	PORT_DIPSETTING(    0x10, "Difficult" )
	PORT_DIPNAME( 0xe0, 0x00, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x00, "30k 200k 500k" )
	PORT_DIPSETTING(    0x80, "100k 300k 800k" )
	PORT_DIPSETTING(    0x40, "30k 200k" )
	PORT_DIPSETTING(    0xc0, "100k 300k" )
	PORT_DIPSETTING(    0x20, "30k" )
	PORT_DIPSETTING(    0xa0, "100k" )
	PORT_DIPSETTING(    0x60, "200k" )
	PORT_DIPSETTING(    0xe0, "None" )
INPUT_PORTS_END



static struct GfxLayout charlayout =
{
	8,8,	/* 8*8 characters */
	2048,	/* 2048 characters */
	4,	/* 4 bits per pixel */
	{ 0, 1, 2, 3 },	/* the bitplanes are packed in one nibble */
	{ 0*4, 1*4, 2*4, 3*4, 4*4, 5*4, 6*4, 7*4 },
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32 },
	32*8	/* every char takes 32 consecutive bytes */
};

static struct GfxLayout spritelayout =
{
	16,16,	/* 8*8 sprites */
	512,	/* 512 sprites */
	4,		/* 4 bits per pixel */
	{ 0, 512*32*8, 2*512*32*8, 3*512*32*8 },	/* the bitplanes are separated */
	{ 0, 1, 2, 3, 4, 5, 6, 7,	/* pretty straightforward layout */
			8*8+0, 8*8+1, 8*8+2, 8*8+3, 8*8+4, 8*8+5, 8*8+6, 8*8+7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
			16*8, 17*8, 18*8, 19*8, 20*8, 21*8, 22*8, 23*8 },
	32*8	/* every sprite takes 32 consecutive bytes */
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &charlayout,     0, 8 },	/* colors   0-127 */
	{ REGION_GFX2, 0, &charlayout,   128, 8 },	/* colors 128-255 */
	{ REGION_GFX3, 0, &spritelayout,   0, 8 },	/* colors   0-127 */
	{ -1 } /* end of array */
};

static struct AY8910interface ay8910_interface =
{
	3,	/* 3 chips */
	1500000,	/* 1.5 MHz?????? */
	{ 12, 12, 12 },
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 }
};

static MACHINE_DRIVER_START( solomon )

	/* basic machine hardware */
	MDRV_CPU_ADD(Z80, 4000000)	/* 4.0 MHz (?????) */
	MDRV_CPU_MEMORY(readmem,writemem)
	MDRV_CPU_VBLANK_INT(nmi_line_pulse,1)

	MDRV_CPU_ADD(Z80, 3072000)
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)	/* 3.072 MHz (?????) */
	MDRV_CPU_MEMORY(solomon_sound_readmem,solomon_sound_writemem)
	MDRV_CPU_PORTS(0,solomon_sound_writeport)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,2)	/* ??? */
						/* NMIs are caused by the main CPU */
	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 2*8, 30*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(256)

	MDRV_VIDEO_START(solomon)
	MDRV_VIDEO_UPDATE(solomon)

	/* sound hardware */
	MDRV_SOUND_ADD(AY8910, ay8910_interface)
MACHINE_DRIVER_END

/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( solomon )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "slmn_06.bin",  0x00000, 0x4000, CRC(e4d421ff) SHA1(9599fa6e2d42bf0cfe77d62c6b162f56eae5efff) )
	ROM_LOAD( "slmn_07.bin",  0x08000, 0x4000, CRC(d52d7e38) SHA1(8439eeeedd1e47d2b9719a05c85a05283c11d7a8) )
	ROM_CONTINUE(             0x04000, 0x4000 )
	ROM_LOAD( "slmn_08.bin",  0x0f000, 0x1000, CRC(b924d162) SHA1(6299b791ec874bc3ef0424b277ec8a736c8cdd9a) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for the audio CPU */
	ROM_LOAD( "slmn_01.bin",  0x0000, 0x4000, CRC(fa6e562e) SHA1(713036c0a80b623086aa674bb5f8a135b6fedb01) )

	ROM_REGION( 0x10000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "slmn_12.bin",  0x00000, 0x08000, CRC(aa26dfcb) SHA1(71748eaceeca878ae9f871e30d5951ca4dde37d6) )	/* characters */
	ROM_LOAD( "slmn_11.bin",  0x08000, 0x08000, CRC(6f94d2af) SHA1(2e070c0fd5b9d7eb9b7e0d53f25b1a5063ef3095) )

	ROM_REGION( 0x10000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "slmn_10.bin",  0x00000, 0x08000, CRC(8310c2a1) SHA1(8cc87ab8faacdb1973791d207bb25ea9b444b66d) )
	ROM_LOAD( "slmn_09.bin",  0x08000, 0x08000, CRC(ab7e6c42) SHA1(0fc3b4a0bd2b17b79e2d1f7d4fe445c09ce4e730) )

	ROM_REGION( 0x10000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "slmn_02.bin",  0x00000, 0x04000, CRC(80fa2be3) SHA1(8e7a78186473a6b5c42577ac9e4591ee2d1151f2) )	/* sprites */
	ROM_LOAD( "slmn_03.bin",  0x04000, 0x04000, CRC(236106b4) SHA1(8eaf3150568c407bd8dc1cdf874b8417e5cca3d2) )
	ROM_LOAD( "slmn_04.bin",  0x08000, 0x04000, CRC(088fe5d9) SHA1(e29ffb9fcff50ce982d5e502e10a8e29a4c47390) )
	ROM_LOAD( "slmn_05.bin",  0x0c000, 0x04000, CRC(8366232a) SHA1(1c7a01dab056ec7d787a6f55772b9fa6fe67305a) )
ROM_END



GAME( 1986, solomon, 0, solomon, solomon, 0, ROT0, "Tecmo", "Solomon's Key (Japan)" )
