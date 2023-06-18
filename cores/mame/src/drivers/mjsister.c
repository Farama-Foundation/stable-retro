/*****************************************************************************

Mahjong Sisters (c) 1986 Toa Plan

	Driver by Uki

*****************************************************************************/

#include "driver.h"

#define MCLK 12000000

extern int mjsister_flip_screen;
extern int mjsister_video_enable;
extern int mjsister_screen_redraw;

extern int vrambank;
extern int colorbank;

VIDEO_START( mjsister );
VIDEO_UPDATE( mjsister );
WRITE_HANDLER( mjsister_videoram_w );

static int mjsister_input_sel1;
static int mjsister_input_sel2;

static int rombank0,rombank1;

static unsigned int dac_adr,dac_bank,dac_adr_s,dac_adr_e,dac_busy;

/****************************************************************************/

static void dac_callback(int param)
{
	data8_t *DACROM = memory_region(REGION_SOUND1);

	DAC_data_w(0,DACROM[(dac_bank * 0x10000 + dac_adr++) & 0x1ffff]);

	if (((dac_adr & 0xff00 ) >> 8) !=  dac_adr_e )
		timer_set(TIME_IN_HZ(MCLK/1024),0,dac_callback);
	else
		dac_busy = 0;
}

static WRITE_HANDLER( mjsister_dac_adr_s_w )
{
	dac_adr_s = data;
}

static WRITE_HANDLER( mjsister_dac_adr_e_w )
{
	dac_adr_e = data;
	dac_adr = dac_adr_s << 8;

	if (dac_busy == 0)
		timer_set(TIME_NOW,0,dac_callback);

	dac_busy = 1;
}

static MACHINE_INIT( mjsister )
{
	dac_busy = 0;
}

static WRITE_HANDLER( mjsister_banksel1_w )
{
	data8_t *BANKROM = memory_region(REGION_CPU1);
	int tmp = colorbank;

	switch (data)
	{
		case 0x0: rombank0 = 0 ; break;
		case 0x1: rombank0 = 1 ; break;

		case 0x2: mjsister_flip_screen = 0 ; break;
		case 0x3: mjsister_flip_screen = 1 ; break;

		case 0x4: colorbank &=0xfe; break;
		case 0x5: colorbank |=0x01; break;
		case 0x6: colorbank &=0xfd; break;
		case 0x7: colorbank |=0x02; break;
		case 0x8: colorbank &=0xfb; break;
		case 0x9: colorbank |=0x04; break;

		case 0xa: mjsister_video_enable = 0 ; break;
		case 0xb: mjsister_video_enable = 1 ; break;

		case 0xe: vrambank = 0 ; break;
		case 0xf: vrambank = 1 ; break;

		default:
			log_cb(RETRO_LOG_DEBUG, LOGPRE "%04x p30_w:%02x\n",activecpu_get_pc(),data);
	}

	if (tmp != colorbank)
		mjsister_screen_redraw = 1;

	cpu_setbank(1,&BANKROM[rombank0*0x10000+rombank1*0x8000]+0x10000);
}

static WRITE_HANDLER( mjsister_banksel2_w )
{
	data8_t *BANKROM = memory_region(REGION_CPU1);

	switch (data)
	{
		case 0xa: dac_bank = 0; break;
		case 0xb: dac_bank = 1; break;

		case 0xc: rombank1 = 0; break;
		case 0xd: rombank1 = 1; break;

		default:
			log_cb(RETRO_LOG_DEBUG, LOGPRE "%04x p31_w:%02x\n",activecpu_get_pc(),data);
	}

	cpu_setbank(1,&BANKROM[rombank0*0x10000+rombank1*0x8000]+0x10000);
}

static WRITE_HANDLER( mjsister_input_sel1_w )
{
	mjsister_input_sel1 = data;
}

static WRITE_HANDLER( mjsister_input_sel2_w )
{
	mjsister_input_sel2 = data;
}

static READ_HANDLER( mjsister_keys_r )
{
	int p,i,ret = 0;

	p = mjsister_input_sel1 & 0x3f;
/*	p |= ((mjsister_input_sel2 & 8) << 4) | ((mjsister_input_sel2 & 0x20) << 1);*/

	for (i=0; i<6; i++)
	{
		if (p & (1 << i))
			ret |= readinputport(i+3);
	}

	return ret;
}

/****************************************************************************/

static MEMORY_READ_START( mjsister_readmem )
	{ 0x0000, 0x77ff, MRA_ROM },
	{ 0x7800, 0x7fff, MRA_RAM },
	{ 0x8000, 0xffff, MRA_BANK1 },
MEMORY_END

static MEMORY_WRITE_START( mjsister_writemem )
	{ 0x0000, 0x77ff, MWA_ROM },
	{ 0x7800, 0x7fff, MWA_RAM },
	{ 0x8000, 0xffff, mjsister_videoram_w },
MEMORY_END

static PORT_READ_START( mjsister_readport )
	{ 0x11, 0x11, AY8910_read_port_0_r },
	{ 0x20, 0x20, mjsister_keys_r },
	{ 0x21, 0x21, input_port_2_r },
PORT_END

static PORT_WRITE_START( mjsister_writeport )
	{ 0x00, 0x01, IOWP_NOP }, /* HD46505? */
	{ 0x10, 0x10, AY8910_control_port_0_w },
	{ 0x12, 0x12, AY8910_write_port_0_w },
	{ 0x30, 0x30, mjsister_banksel1_w },
	{ 0x31, 0x31, mjsister_banksel2_w },
	{ 0x32, 0x32, mjsister_input_sel1_w },
	{ 0x33, 0x33, mjsister_input_sel2_w },
	{ 0x34, 0x34, mjsister_dac_adr_s_w },
	{ 0x35, 0x35, mjsister_dac_adr_e_w },
PORT_END

/****************************************************************************/

INPUT_PORTS_START( mjsister )

	PORT_START	/* DSW1 (0) */
	PORT_DIPNAME( 0x07, 0x07, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x07, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x05, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_4C ) )
	PORT_DIPNAME( 0x08, 0x08, "Unknown 1-4" )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, "Unknown 1-5" )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, "Unknown 1-6" )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_BIT(           0x40, IP_ACTIVE_LOW, IPT_UNKNOWN ) /* service mode */
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START	/* DSW2 (1) */
	PORT_DIPNAME( 0x01, 0x01, "Unknown 2-1" )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, "Unknown 2-2" )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, "Unknown 2-3" )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, "Unknown 2-4" )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, "Unknown 2-5" )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, "Unknown 2-6" )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, "Unknown 2-7" )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, "Unknown 2-8" )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START	/* (2) */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_UNKNOWN ) /* memory reset 1 */
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNKNOWN ) /* analyzer */
	PORT_SERVICE( 0x08, IP_ACTIVE_HIGH )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNKNOWN ) /* memory reset 2 */
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN ) /* pay out */
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN ) /* hopper */

	PORT_START	/* (3) PORT 1-0 */
	PORT_BITX(0x01, IP_ACTIVE_HIGH, 0, "P1 A",   KEYCODE_A, IP_JOY_NONE )
	PORT_BITX(0x02, IP_ACTIVE_HIGH, 0, "P1 B",   KEYCODE_B, IP_JOY_NONE )
	PORT_BITX(0x04, IP_ACTIVE_HIGH, 0, "P1 C",   KEYCODE_C, IP_JOY_NONE )
	PORT_BITX(0x08, IP_ACTIVE_HIGH, 0, "P1 D",   KEYCODE_D, IP_JOY_NONE )
	PORT_BITX(0x10, IP_ACTIVE_HIGH, 0, "P1 Last Chance",   KEYCODE_RALT, IP_JOY_NONE )
	PORT_BIT( 0xe0, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START	/* (4) PORT 1-1 */
	PORT_BITX(0x01, IP_ACTIVE_HIGH, 0, "P1 E",     KEYCODE_E, IP_JOY_NONE )
	PORT_BITX(0x02, IP_ACTIVE_HIGH, 0, "P1 F",     KEYCODE_F, IP_JOY_NONE )
	PORT_BITX(0x04, IP_ACTIVE_HIGH, 0, "P1 G",     KEYCODE_G, IP_JOY_NONE )
	PORT_BITX(0x08, IP_ACTIVE_HIGH, 0, "P1 H",     KEYCODE_H, IP_JOY_NONE )
	PORT_BITX(0x10, IP_ACTIVE_HIGH, 0, "P1 Take Score", KEYCODE_RCONTROL, IP_JOY_NONE )
	PORT_BIT( 0xe0, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START	/* (5) PORT 1-2 */
	PORT_BITX(0x01, IP_ACTIVE_HIGH, 0, "P1 I",   KEYCODE_I, IP_JOY_NONE )
	PORT_BITX(0x02, IP_ACTIVE_HIGH, 0, "P1 J",   KEYCODE_J, IP_JOY_NONE )
	PORT_BITX(0x04, IP_ACTIVE_HIGH, 0, "P1 K",   KEYCODE_K, IP_JOY_NONE )
	PORT_BITX(0x08, IP_ACTIVE_HIGH, 0, "P1 L",   KEYCODE_L, IP_JOY_NONE )
	PORT_BITX(0x10, IP_ACTIVE_HIGH, 0, "P1 Double Up", KEYCODE_RSHIFT, IP_JOY_NONE )
	PORT_BIT( 0xe0, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START	/* (6) PORT 1-3 */
	PORT_BITX(0x01, IP_ACTIVE_HIGH, 0, "P1 M",   KEYCODE_M, IP_JOY_NONE )
	PORT_BITX(0x02, IP_ACTIVE_HIGH, 0, "P1 N",   KEYCODE_N, IP_JOY_NONE )
	PORT_BITX(0x04, IP_ACTIVE_HIGH, 0, "P1 Chi", KEYCODE_SPACE, IP_JOY_NONE )
	PORT_BITX(0x08, IP_ACTIVE_HIGH, 0, "P1 Pon", KEYCODE_LALT, IP_JOY_NONE )
	PORT_BITX(0x10, IP_ACTIVE_HIGH, 0, "P1 Flip Flop", KEYCODE_X, IP_JOY_NONE )
	PORT_BIT( 0xe0, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START	/* (7) PORT 1-4 */
	PORT_BITX(0x01, IP_ACTIVE_HIGH, 0, "P1 Kan",   KEYCODE_LCONTROL, IP_JOY_NONE )
	PORT_BITX(0x02, IP_ACTIVE_HIGH, 0, "P1 Reach", KEYCODE_LSHIFT, IP_JOY_NONE )
	PORT_BITX(0x04, IP_ACTIVE_HIGH, 0, "P1 Ron", KEYCODE_Z, IP_JOY_NONE )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BITX(0x10, IP_ACTIVE_HIGH, 0, "P1 Big", KEYCODE_ENTER, IP_JOY_NONE )
	PORT_BIT( 0xe0, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START	/* (8) PORT 1-5 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BITX(0x02, IP_ACTIVE_HIGH, 0, "P1 Bet", KEYCODE_2, IP_JOY_NONE )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BITX(0x10, IP_ACTIVE_HIGH, 0, "P1 Small", KEYCODE_BACKSPACE, IP_JOY_NONE )
	PORT_BIT( 0xe0, IP_ACTIVE_HIGH, IPT_UNUSED )

INPUT_PORTS_END

/****************************************************************************/

static struct AY8910interface ay8910_interface =
{
	1,      /* 1 chip */
	MCLK/8, /* 1.500 MHz */
	{ 15 },
	{ input_port_0_r },
	{ input_port_1_r },
	{ 0 },
	{ 0 }
};

static struct DACinterface dac_interface =
{
	1,
	{ 100 }
};

static MACHINE_DRIVER_START( mjsister )

	/* basic machine hardware */
	MDRV_CPU_ADD(Z80, MCLK/2) /* 6.000 MHz */
	MDRV_CPU_MEMORY(mjsister_readmem,mjsister_writemem)
	MDRV_CPU_PORTS(mjsister_readport,mjsister_writeport)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,2)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)
	MDRV_MACHINE_INIT(mjsister)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(256+4, 256)
	MDRV_VISIBLE_AREA(0, 255+4, 8, 247)
	MDRV_PALETTE_INIT(RRRR_GGGG_BBBB)
	MDRV_PALETTE_LENGTH(256)

	MDRV_VIDEO_START(mjsister)
	MDRV_VIDEO_UPDATE(mjsister)

	/* sound hardware */
	MDRV_SOUND_ADD(AY8910, ay8910_interface)
	MDRV_SOUND_ADD(DAC, dac_interface)

MACHINE_DRIVER_END

/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( mjsister )
	ROM_REGION( 0x30000, REGION_CPU1, 0 )   /* CPU */
	ROM_LOAD( "ms00.bin",  0x00000, 0x08000, CRC(9468c33b) SHA1(63aecdcaa8493d58549dfd1d217743210cf953bc) )
	ROM_LOAD( "ms01t.bin", 0x10000, 0x10000, CRC(a7b6e530) SHA1(fda9bea214968a8814d2c43226b3b32316581050) ) /* banked */
	ROM_LOAD( "ms02t.bin", 0x20000, 0x10000, CRC(7752b5ba) SHA1(84dcf27a62eb290ba07c85af155897ec72f320a8) ) /* banked */

	ROM_REGION( 0x20000, REGION_SOUND1, 0 ) /* samples */
	ROM_LOAD( "ms03.bin", 0x00000,  0x10000, CRC(10a68e5e) SHA1(a0e2fa34c1c4f34642f65fbf17e9da9c2554a0c6) )
	ROM_LOAD( "ms04.bin", 0x10000,  0x10000, CRC(641b09c1) SHA1(15cde906175bcb5190d36cc91cbef003ef91e425) )

	ROM_REGION( 0x00400, REGION_PROMS, 0 ) /* color PROMs */
	ROM_LOAD( "ms05.bpr", 0x0000,  0x0100, CRC(dd231a5f) SHA1(be008593ac8ba8f5a1dd5b188dc7dc4c03016805) ) /* R*/
	ROM_LOAD( "ms06.bpr", 0x0100,  0x0100, CRC(df8e8852) SHA1(842a891440aef55a560d24c96f249618b9f4b97f) ) /* G*/
	ROM_LOAD( "ms07.bpr", 0x0200,  0x0100, CRC(6cb3a735) SHA1(468ae3d40552dc2ec24f5f2988850093d73948a6) ) /* B*/
	ROM_LOAD( "ms08.bpr", 0x0300,  0x0100, CRC(da2b3b38) SHA1(4de99c17b227653bc1b904f1309f447f5a0ab516) ) /* ?*/
ROM_END

GAME( 1986, mjsister, 0, mjsister, mjsister, 0, ROT0, "Toaplan", "Mahjong Sisters (Japan)" )
