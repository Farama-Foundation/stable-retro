/***************************************************************************

	Stadium Hero (Japan)			(c) 1988 Data East Corporation

	Emulation by Bryan McPhail, mish@tendril.co.uk

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "cpu/m6502/m6502.h"


/* Video emulation definitions */
VIDEO_START( stadhero );
VIDEO_UPDATE( stadhero );

extern data16_t *stadhero_pf1_data,*stadhero_pf2_data;

WRITE16_HANDLER( stadhero_pf1_data_w );
WRITE16_HANDLER( stadhero_pf2_data_w );
WRITE16_HANDLER( stadhero_pf2_control_0_w );
WRITE16_HANDLER( stadhero_pf2_control_1_w );

/******************************************************************************/

static READ16_HANDLER( stadhero_control_r )
{
	switch (offset<<1)
	{
		case 0: /* Player 1 & 2 joystick & buttons */
			return (readinputport(0) + (readinputport(1) << 8));

		case 2: /* Credits, start buttons */
			return readinputport(2) | (readinputport(2)<<8);

		case 4: /* Byte 4: Dipswitch bank 2, Byte 5: Dipswitch Bank 1 */
			return (readinputport(3) + (readinputport(4) << 8));
      
	}

	log_cb(RETRO_LOG_DEBUG, LOGPRE "CPU #0 PC %06x: warning - read unmapped memory address %06x\n",activecpu_get_pc(),0x30c000+offset);
	return ~0;
}

static WRITE16_HANDLER( stadhero_control_w )
{
	switch (offset<<1)
	{
		case 4: /* Interrupt ack (VBL - IRQ 5) */
			break;
		case 6: /* 6502 sound cpu */
			soundlatch_w(0,data & 0xff);
			cpu_set_irq_line(1,IRQ_LINE_NMI,PULSE_LINE);
			break;
		default:
			log_cb(RETRO_LOG_DEBUG, LOGPRE "CPU #0 PC %06x: warning - write %02x to unmapped memory address %06x\n",activecpu_get_pc(),data,0x30c010+offset);
			break;
	}
}

static WRITE16_HANDLER( spriteram_mirror_w )
{
	spriteram16[offset]=data;
}

static READ16_HANDLER( stadhero_pf1_data_r ) { return stadhero_pf1_data[offset]; }
static READ16_HANDLER( stadhero_pf2_data_r ) { return stadhero_pf2_data[offset]; }

/******************************************************************************/

static MEMORY_READ16_START( stadhero_readmem )
	{ 0x000000, 0x01ffff, MRA16_ROM },
	{ 0x200000, 0x2007ff, stadhero_pf1_data_r },
	{ 0x260000, 0x261fff, stadhero_pf2_data_r },
	{ 0x30c000, 0x30c00b, stadhero_control_r },
	{ 0x310000, 0x3107ff, MRA16_RAM },
	{ 0xff8000, 0xffbfff, MRA16_RAM }, /* Main ram */
	{ 0xffc000, 0xffc7ff, MRA16_RAM }, /* Sprites */
MEMORY_END

static MEMORY_WRITE16_START( stadhero_writemem )
	{ 0x000000, 0x01ffff, MWA16_ROM },
	{ 0x200000, 0x2007ff, stadhero_pf1_data_w, &stadhero_pf1_data },
	{ 0x240000, 0x240007, stadhero_pf2_control_0_w },
	{ 0x240010, 0x240017, stadhero_pf2_control_1_w },
	{ 0x260000, 0x261fff, stadhero_pf2_data_w, &stadhero_pf2_data },
	{ 0x30c000, 0x30c00b, stadhero_control_w },
	{ 0x310000, 0x3107ff, paletteram16_xxxxBBBBGGGGRRRR_word_w, &paletteram16 },
	{ 0xff8000, 0xffbfff, MWA16_RAM },
	{ 0xffc000, 0xffc7ff, MWA16_RAM, &spriteram16 },
	{ 0xffc800, 0xffcfff, spriteram_mirror_w },
MEMORY_END

/******************************************************************************/

static WRITE_HANDLER( YM3812_w )
{
	switch (offset) {
	case 0:
		YM3812_control_port_0_w(0,data);
		break;
	case 1:
		YM3812_write_port_0_w(0,data);
		break;
	}
}

static WRITE_HANDLER( YM2203_w )
{
	switch (offset) {
	case 0:
		YM2203_control_port_0_w(0,data);
		break;
	case 1:
		YM2203_write_port_0_w(0,data);
		break;
	}
}

static MEMORY_READ_START( stadhero_s_readmem )
	{ 0x0000, 0x05ff, MRA_RAM },
	{ 0x3000, 0x3000, soundlatch_r },
	{ 0x3800, 0x3800, OKIM6295_status_0_r },
	{ 0x8000, 0xffff, MRA_ROM },
MEMORY_END

static MEMORY_WRITE_START( stadhero_s_writemem )
	{ 0x0000, 0x05ff, MWA_RAM },
	{ 0x0800, 0x0801, YM2203_w },
	{ 0x1000, 0x1001, YM3812_w },
	{ 0x3800, 0x3800, OKIM6295_data_0_w },
	{ 0x8000, 0xffff, MWA_ROM },
MEMORY_END

/******************************************************************************/

INPUT_PORTS_START( stadhero )
	PORT_START	/* Player 1 controls */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START	/* Player 2 controls */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START2 )

	PORT_START	/* Credits, start buttons */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN3 ) /* Service */
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_VBLANK )

	PORT_START	/* Dip switch bank 1 */
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 1C_2C ) )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 1C_2C ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START	/* Dip switch bank 2 */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
INPUT_PORTS_END

/******************************************************************************/

static struct GfxLayout charlayout =
{
	8,8,	/* 8*8 chars */
	4096,
	3,		/* 4 bits per pixel  */
	{ 0x00000*8,0x8000*8,0x10000*8 },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8	/* every char takes 8 consecutive bytes */
};

static struct GfxLayout tile_3bpp =
{
	16,16,
	2048,
	3,
	{ 0x20000*8, 0x10000*8, 0x00000*8 },
	{ 16*8+0, 16*8+1, 16*8+2, 16*8+3, 16*8+4, 16*8+5, 16*8+6, 16*8+7,
			0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
			8*8, 9*8, 10*8, 11*8, 12*8, 13*8, 14*8, 15*8 },
	16*16
};

static struct GfxLayout spritelayout =
{
	16,16,
	4096,
	4,
	{ 0x60000*8,0x40000*8,0x20000*8,0x00000*8 },
	{ 16*8+0, 16*8+1, 16*8+2, 16*8+3, 16*8+4, 16*8+5, 16*8+6, 16*8+7,
			0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
			8*8, 9*8, 10*8, 11*8, 12*8, 13*8, 14*8, 15*8 },
	16*16
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &charlayout,     0, 16 },	/* Characters 8x8 */
	{ REGION_GFX2, 0, &tile_3bpp,    512, 16 },	/* Tiles 16x16 */
	{ REGION_GFX3, 0, &spritelayout, 256, 16 },	/* Sprites 16x16 */
	{ -1 } /* end of array */
};

/******************************************************************************/

static void irqhandler(int linestate)
{
	cpu_set_irq_line(1,0,linestate);
}

static struct YM2203interface ym2203_interface =
{
	1,
	1500000,	/* 12MHz clock divided by 8 = 1.50 MHz */
	{ YM2203_VOL(40,95) },
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 }
};

static struct YM3812interface ym3812_interface =
{
	1,			/* 1 chip */
	3000000,	/* 3 MHz (12MHz/4) */
	{ 80 },
	{ irqhandler },
};

static struct OKIM6295interface okim6295_interface =
{
	1,              /* 1 chip */
	{ 7757 },           /* 8000Hz frequency */
	{ REGION_SOUND1 },	/* memory region 3 */
	{ 80 }
};

/******************************************************************************/

static MACHINE_DRIVER_START( stadhero )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 10000000)
	MDRV_CPU_MEMORY(stadhero_readmem,stadhero_writemem)
	MDRV_CPU_VBLANK_INT(irq5_line_hold,1)/* VBL */

	MDRV_CPU_ADD(M6502, 1500000)
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
	MDRV_CPU_MEMORY(stadhero_s_readmem,stadhero_s_writemem)

	MDRV_FRAMES_PER_SECOND(58)
	MDRV_VBLANK_DURATION(529)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 1*8, 31*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(1024)

	MDRV_VIDEO_START(stadhero)
	MDRV_VIDEO_UPDATE(stadhero)

	/* sound hardware */
	MDRV_SOUND_ADD(YM2203, ym2203_interface)
	MDRV_SOUND_ADD(YM3812, ym3812_interface)
	MDRV_SOUND_ADD(OKIM6295, okim6295_interface)
MACHINE_DRIVER_END

/******************************************************************************/

ROM_START( stadhero )
	ROM_REGION( 0x20000, REGION_CPU1, 0 )	/* 6*64k for 68000 code */
	ROM_LOAD16_BYTE( "ef15.bin",  0x00000, 0x10000, CRC(bbba364e) SHA1(552096102f402085596635f02096462c6b8e13a7) )
	ROM_LOAD16_BYTE( "ef13.bin",  0x00001, 0x10000, CRC(97c6717a) SHA1(6c81260f49a59f70c71f520e51330a6833828684) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 6502 Sound */
	ROM_LOAD( "ef18.bin",  0x8000, 0x8000, CRC(20fd9668) SHA1(058e34a0ebfc372aaa9230c2bc9164ee2e85e217) )

	ROM_REGION( 0x18000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "ef08.bin",     0x000000, 0x10000, CRC(e84752fe) SHA1(9af2140ddbb44be793ab5b39787bac27f5b1c1f2) )	/* chars */
	ROM_LOAD( "ef09.bin",     0x010000, 0x08000, CRC(2ade874d) SHA1(5c884535214438a4ea79fd262700a346bc12ad81) )

	ROM_REGION( 0x30000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "ef10.bin",     0x000000, 0x10000, CRC(dca3d599) SHA1(2b97a70065f3065e7fbb54fb53cb120d9e5013b3) )	/* tiles */
	ROM_LOAD( "ef11.bin",     0x010000, 0x10000, CRC(af563e96) SHA1(c88eaff4a1ea133d708f4511bb1dbc99ef066eed) )
	ROM_LOAD( "ef12.bin",     0x020000, 0x10000, CRC(9a1bf51c) SHA1(e733c193b305496878551fc6eefc21587ba75c82) )

	ROM_REGION( 0x80000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "ef00.bin",     0x000000, 0x10000, CRC(94ed257c) SHA1(caa4a4c8bf3b34d2288e117cfc704cca4c6f913b) )	/* sprites */
	ROM_LOAD( "ef01.bin",     0x010000, 0x10000, CRC(6eb9a721) SHA1(0f9dce614e67e57612e3a4ce187f0f9c12b78281) )
	ROM_LOAD( "ef02.bin",     0x020000, 0x10000, CRC(850cb771) SHA1(ccb54036191674d76965270a5831fba3e62f47c0) )
	ROM_LOAD( "ef03.bin",     0x030000, 0x10000, CRC(24338b96) SHA1(7730486bd0b84ba0a69b5547e348ee0058d4e7f1) )
	ROM_LOAD( "ef04.bin",     0x040000, 0x10000, CRC(9e3d97a7) SHA1(d02722376721caa5d8498f15f16959f42b75e7c1) )
	ROM_LOAD( "ef05.bin",     0x050000, 0x10000, CRC(88631005) SHA1(3c1787fb3aabdd9fecf679b2f4a9f833bf660885) )
	ROM_LOAD( "ef06.bin",     0x060000, 0x10000, CRC(9f47848f) SHA1(e23337684c8999483cbd11d3d953b06c34f13069) )
	ROM_LOAD( "ef07.bin",     0x070000, 0x10000, CRC(8859f655) SHA1(b3d69c5808b3ba7347ddb7f9693499903e9bfe6b) )

	ROM_REGION( 0x10000, REGION_SOUND1, 0 )	/* ADPCM samples */
	ROM_LOAD( "ef17.bin",  0x0000, 0x10000, CRC(07c78358) SHA1(ce82b429eec0193fd9665b717336756a514db144) )
ROM_END

/******************************************************************************/

GAME( 1988, stadhero, 0, stadhero, stadhero, 0, ROT0, "Data East Corporation", "Stadium Hero (Japan)" )
