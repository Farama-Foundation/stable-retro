/***************************************************************

 Pro Baseball Skill Tryout (JPN Ver.)
 (c) 1985 Data East

 Driver by Pierpaolo Prazzoli and Bryan McPhail

****************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

extern data8_t *tryout_gfx_control;
static data8_t *tryout_rom;

extern READ_HANDLER( tryout_vram_r );
extern WRITE_HANDLER( tryout_videoram_w );
extern WRITE_HANDLER( tryout_vram_w );
extern WRITE_HANDLER( tryout_vram_bankswitch_w );
extern WRITE_HANDLER( tryout_flipscreen_w );

extern PALETTE_INIT( tryout );
extern VIDEO_START( tryout );
extern VIDEO_UPDATE( tryout );

static WRITE_HANDLER( tryout_nmi_ack_w )
{
	cpu_set_nmi_line( 0, CLEAR_LINE );
}

static WRITE_HANDLER( tryout_sound_w )
{
	soundlatch_w(0,data);
	cpu_set_irq_line( 1, 0, PULSE_LINE );
}

static WRITE_HANDLER( tryout_sound_irq_ack_w )
{
	cpu_set_irq_line( 1, 0, CLEAR_LINE );
}

static WRITE_HANDLER( tryout_bankswitch_w )
{
 	data8_t *RAM = memory_region(REGION_CPU1);
	int bankaddress;

	bankaddress = 0x10000 + (data & 0x01) * 0x2000;
	cpu_setbank(1,&RAM[bankaddress]);
}

static MEMORY_READ_START( readmem )
	{ 0x0000, 0x07ff, MRA_RAM },
	{ 0x2000, 0x3fff, MRA_BANK1 },
	{ 0x4000, 0xbfff, MRA_ROM },
	{ 0xc800, 0xc87f, MRA_RAM },
	{ 0xcc00, 0xcc7f, MRA_RAM },
	{ 0xd000, 0xd7ff, tryout_vram_r },
	{ 0xe000, 0xe000, input_port_0_r },
	{ 0xe001, 0xe001, input_port_1_r },
	{ 0xe002, 0xe002, input_port_2_r },
	{ 0xe003, 0xe003, input_port_3_r },
	{ 0xfff0, 0xffff, MRA_ROM },
MEMORY_END

static MEMORY_WRITE_START( writemem )
	{ 0x0000, 0x07ff, MWA_RAM },
	{ 0x1000, 0x17ff, tryout_videoram_w , &videoram },
	{ 0x2000, 0x3fff, MWA_BANK1 },
	{ 0x4000, 0xbfff, MWA_ROM },
	{ 0xc800, 0xc87f, MWA_RAM, &spriteram },
	{ 0xcc00, 0xcc7f, MWA_RAM, &spriteram_2 },
	{ 0xd000, 0xd7ff, tryout_vram_w },
	{ 0xe301, 0xe301, tryout_flipscreen_w },
	{ 0xe302, 0xe302, tryout_bankswitch_w },
	{ 0xe401, 0xe401, tryout_vram_bankswitch_w },
	{ 0xe402, 0xe404, MWA_RAM, &tryout_gfx_control },
	{ 0xe414, 0xe414, tryout_sound_w },
	{ 0xe417, 0xe417, tryout_nmi_ack_w },
	{ 0xfff0, 0xffff, MWA_ROM, &tryout_rom },
MEMORY_END

static MEMORY_READ_START( sound_readmem )
	{ 0x0000, 0x07ff, MRA_RAM },
	{ 0x4000, 0x4000, YM2203_status_port_0_r },
	{ 0xa000, 0xa000, soundlatch_r },
	{ 0xc000, 0xffff, MRA_ROM },
MEMORY_END

static MEMORY_WRITE_START( sound_writemem )
	{ 0x0000, 0x07ff, MWA_RAM },
	{ 0x4000, 0x4000, YM2203_control_port_0_w },
	{ 0x4001, 0x4001, YM2203_write_port_0_w },
	{ 0xd000, 0xd000, tryout_sound_irq_ack_w },
	{ 0xc000, 0xffff, MWA_ROM },
MEMORY_END

INPUT_PORTS_START( tryout )
	PORT_START
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 1C_3C ) )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_3C ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_4WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_4WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_4WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT_IMPULSE( 0x04, IP_ACTIVE_LOW, IPT_SERVICE1, 2 )
	PORT_BIT_IMPULSE( 0x08, IP_ACTIVE_LOW, IPT_COIN1, 2 )
	PORT_BIT_IMPULSE( 0x10, IP_ACTIVE_LOW, IPT_COIN2, 2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_VBLANK )
INPUT_PORTS_END

static struct GfxLayout charlayout =
{
	8,8,	/* 8*8 characters */
	RGN_FRAC(1,2),
	2,	/* 2 bits per pixel */
	{ 0, 4 },	/* the two bitplanes for 4 pixels are packed into one byte */
	{ 3, 2, 1, 0, RGN_FRAC(1,2)+3, RGN_FRAC(1,2)+2, RGN_FRAC(1,2)+1, RGN_FRAC(1,2)+0 },
	{ 7*8, 6*8, 5*8, 4*8, 3*8, 2*8, 1*8, 0*8 },
	8*8	/* every char takes 8 consecutive bytes */
};

static struct GfxLayout vramlayout =
{
	16, 16,
	128,
	3,
	{ 0, 0x2000 * 8, 0x4000 * 8 },
	{ 7, 6, 5, 4, 128+7, 128+6, 128+5, 128+4, 256+7, 256+6, 256+5, 256+4, 384+7, 384+6, 384+5, 384+4  },
	{ 15*8, 14*8, 13*8, 12*8, 11*8, 10*8, 9*8, 8*8,
	  7*8, 6*8,5*8,4*8,3*8,2*8,1*8,0*8 },
	64*8
};

static struct GfxLayout spritelayout =
{
	16,16,
	RGN_FRAC(1,3),
	3,
	{ RGN_FRAC(2,3), RGN_FRAC(1,3), RGN_FRAC(0,3) },
	{ 7, 6, 5, 4, 3, 2, 1, 0,
			16*8+7, 16*8+6, 16*8+5, 16*8+4, 16*8+3, 16*8+2, 16*8+1, 16*8+0 },
	{ 15*8, 14*8, 13*8, 12*8, 11*8, 10*8, 9*8, 8*8,
			7*8, 6*8, 5*8, 4*8, 3*8, 2*8, 1*8, 0*8 },
	32*8
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &charlayout,   0, 8 },
	{ REGION_GFX2, 0, &spritelayout, 0, 4 },
	{ 0,           0, &vramlayout,   0, 4 },
	{ -1 }
};

static INTERRUPT_GEN( tryout_interrupt )
{
	if ((input_port_3_r(0) & 0x1c)!=0x1c)
		cpu_set_nmi_line(0, ASSERT_LINE);
}

static struct YM2203interface ym2203_interface =
{
	1,	/* 1 chip */
	1500000,	/* 1.5 MHz */
	{ YM2203_VOL(50,50) },
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 },
};

static MACHINE_DRIVER_START( tryout )
	/* basic machine hardware */
	MDRV_CPU_ADD(M6502, 2000000)		 /* ? */
	MDRV_CPU_MEMORY(readmem,writemem)
	MDRV_CPU_VBLANK_INT(tryout_interrupt,1)

	MDRV_CPU_ADD(M6502, 1500000)		/* ? */
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
	MDRV_CPU_MEMORY(sound_readmem,sound_writemem)
	MDRV_CPU_VBLANK_INT(nmi_line_pulse,16) /* ? */

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(256, 256)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 1*8, 31*8-1)

	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(0x20)
	MDRV_PALETTE_INIT(tryout)

	MDRV_VIDEO_START(tryout)
	MDRV_VIDEO_UPDATE(tryout)

	/* sound hardware */
	MDRV_SOUND_ADD(YM2203, ym2203_interface)
MACHINE_DRIVER_END

ROM_START( tryout )
	ROM_REGION( 0x14000, REGION_CPU1, 0 )
	ROM_LOAD( "ch10-1.bin",   0x04000, 0x4000, CRC(d046231b) SHA1(145f9e9b0707824f7ae6d1587754b28c17907807) )
	ROM_LOAD( "ch11.bin",     0x08000, 0x4000, CRC(4d00b6f0) SHA1(cc1e700b8547672d7dd1d262c6181a5c321fbf72) )
	ROM_LOAD( "ch12.bin",     0x10000, 0x4000, CRC(bcd221be) SHA1(69869de8b5d56a97e2cd15fa275527aa767f1e44) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )
	ROM_LOAD( "ch00-1.bin",   0x0c000, 0x4000, CRC(8b33d968) SHA1(cf44529e5577d09978b87dc2bbe1415babbf36a0) )

	ROM_REGION( 0x4000, REGION_GFX1, 0 )
	ROM_LOAD( "ch13.bin",     0x00000, 0x4000, CRC(a9619c58) SHA1(92528b1c4afc95394ac8cad5b37f23da0c6a5310) )

	ROM_REGION( 0x24000, REGION_GFX2, 0 )
	ROM_LOAD( "ch09.bin",     0x00000, 0x4000, CRC(9c5e275b) SHA1(83b29996573d85c73bb4b63086c7a624fad19bde) )
	ROM_LOAD( "ch08.bin",     0x04000, 0x4000, CRC(88396abb) SHA1(2865a265ddfb91c2ad2770da5e0d84a544f3c419) )
	ROM_LOAD( "ch07.bin",     0x08000, 0x4000, CRC(901b5f5e) SHA1(f749b5ec0c51c66655798e8a37c887870370991e) )
	ROM_LOAD( "ch06.bin",     0x0c000, 0x4000, CRC(d937e326) SHA1(5870a82b02438f2fdae089f6d1b8e9ce13d213a6) )
	ROM_LOAD( "ch05.bin",     0x10000, 0x4000, CRC(27f0e7be) SHA1(5fa2bd666d012addfb836d009f962f89e4a00b2d) )
	ROM_LOAD( "ch04.bin",     0x14000, 0x4000, CRC(019e0b75) SHA1(4bfd7cd6c28ec6dfaf8e9bf009716e92759f06c2) )
	ROM_LOAD( "ch03.bin",     0x18000, 0x4000, CRC(b87e2464) SHA1(0089c0ff421929345a1d21951789a6374e0019ff) )
	ROM_LOAD( "ch02.bin",     0x1c000, 0x4000, CRC(62369772) SHA1(89f360003e916bee76d74b7e046bf08349726fda) )
	ROM_LOAD( "ch01.bin",     0x20000, 0x4000, CRC(ee6d57b5) SHA1(7dd2f3b962f088fcbc40fcb74c0a56783857fb7b) )

	ROM_REGION( 0x20, REGION_PROMS, 0 )
	ROM_LOAD( "ch14.bpr",     0x00000, 0x0020, CRC(8ce19925) SHA1(12f8f6022f1148b6ba1d019a34247452637063a7) )
ROM_END

DRIVER_INIT( tryout )
{
  /* set up data ROMs */
  memcpy(tryout_rom, &memory_region(REGION_CPU1)[0xbff0], 0x10);
}

GAME( 1985, tryout, 0, tryout, tryout, tryout, ROT90, "Data East Corporation", "Pro Baseball Skill Tryout (Japan)" )
