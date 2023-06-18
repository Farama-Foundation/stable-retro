/*****************************************************************************

Strength & Skill (c) 1984 Sun Electronics

	Driver by Uki

	19/Jun/2001 -

*****************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

static UINT8 *strnskil_sharedram;

/****************************************************************************/

extern WRITE_HANDLER( strnskil_videoram_w );
extern WRITE_HANDLER( strnskil_scroll_x_w );
extern WRITE_HANDLER( strnskil_scrl_ctrl_w );

extern PALETTE_INIT( strnskil );
extern VIDEO_START( strnskil );
extern VIDEO_UPDATE( strnskil );

WRITE_HANDLER( strnskil_sharedram_w )
{
	strnskil_sharedram[offset] = data;
}

READ_HANDLER( strnskil_sharedram_r )
{
	return strnskil_sharedram[offset];
}

READ_HANDLER( strnskil_d800_r )
{
/* bit0: interrupt type?, bit1: CPU2 busack? */

	if (cpu_getiloops() == 0)
		return 0;
	return 1;
}

/****************************************************************************/

static READ_HANDLER( protection_r )
{
	int res;

	switch (activecpu_get_pc())
	{
		case 0x6066:	res = 0xa5;	break;
		case 0x60dc:	res = 0x20;	break;	/* bits 0-3 unknown */
		case 0x615d:	res = 0x30;	break;	/* bits 0-3 unknown */
		case 0x61b9:	res = 0x60|(rand()&0x0f);	break;	/* bits 0-3 unknown */
		case 0x6219:	res = 0x77;	break;
		case 0x626c:	res = 0xb4;	break;
		default:		res = 0xff; break;
	}

	log_cb(RETRO_LOG_DEBUG, LOGPRE "%04x: protection_r -> %02x\n",activecpu_get_pc(),res);
	return res;
}

static WRITE_HANDLER( protection_w )
{
	log_cb(RETRO_LOG_DEBUG, LOGPRE "%04x: protection_w %02x\n",activecpu_get_pc(),data);
}

/****************************************************************************/

static MEMORY_READ_START( strnskil_readmem1 )
	{ 0x0000, 0x9fff, MRA_ROM },

	{ 0xc000, 0xc7ff, MRA_RAM },
	{ 0xc800, 0xcfff, strnskil_sharedram_r },
	{ 0xd000, 0xd7ff, MRA_RAM }, /* videoram */

	{ 0xd800, 0xd800, strnskil_d800_r },
	{ 0xd801, 0xd801, input_port_0_r }, /* dsw 1 */
	{ 0xd802, 0xd802, input_port_1_r }, /* dsw 2 */
	{ 0xd803, 0xd803, input_port_4_r }, /* other inputs */
	{ 0xd804, 0xd804, input_port_2_r }, /* player1 */
	{ 0xd805, 0xd805, input_port_3_r }, /* player2 */

	{ 0xd806, 0xd806, protection_r }, /* protection data read (pettanp) */
MEMORY_END

static MEMORY_WRITE_START( strnskil_writemem1 )
	{ 0x0000, 0x9fff, MWA_ROM },

	{ 0xc000, 0xc7ff, MWA_RAM },
	{ 0xc800, 0xcfff, strnskil_sharedram_w },
	{ 0xd000, 0xd7ff, strnskil_videoram_w, &videoram },

	{ 0xd808, 0xd808, strnskil_scrl_ctrl_w },
	{ 0xd809, 0xd809, MWA_NOP }, /* coin counter? */
	{ 0xd80a, 0xd80b, strnskil_scroll_x_w },

/*	{ 0xd80c, 0xd80c, MWA_NOP },		 // protection reset? /*/
	{ 0xd80d, 0xd80d, protection_w },	/* protection data write (pettanp) */
MEMORY_END

static MEMORY_READ_START( strnskil_readmem2 )
	{ 0x0000, 0x5fff, MRA_ROM },
	{ 0xc000, 0xc7ff, spriteram_r },
	{ 0xc800, 0xcfff, MRA_RAM },
MEMORY_END

static MEMORY_WRITE_START( strnskil_writemem2 )
	{ 0x0000, 0x5fff, MWA_ROM },
	{ 0xc000, 0xc7ff, MWA_RAM, &spriteram, &spriteram_size },
	{ 0xc800, 0xcfff, MWA_RAM, &strnskil_sharedram },

	{ 0xd801, 0xd801, SN76496_0_w },
	{ 0xd802, 0xd802, SN76496_1_w },
MEMORY_END


/****************************************************************************/

INPUT_PORTS_START( strnskil )
	PORT_START  /* dsw1 */
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x00, "Unknown 1-2" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x02, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x00, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x04, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x00, "Unknown 1-4" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	PORT_DIPNAME( 0xf0, 0x00, "Coin1 / Coin2" )
	PORT_DIPSETTING(    0x00, "1C 1C / 1C 1C" )
	PORT_DIPSETTING(    0x10, "2C 1C / 2C 1C" )
	PORT_DIPSETTING(    0x20, "2C 1C / 1C 3C" )
	PORT_DIPSETTING(    0x30, "1C 1C / 1C 2C" )
	PORT_DIPSETTING(    0x40, "1C 1C / 1C 3C" )
	PORT_DIPSETTING(    0x50, "1C 1C / 1C 4C" )
	PORT_DIPSETTING(    0x60, "1C 1C / 1C 5C" )
	PORT_DIPSETTING(    0x70, "1C 1C / 1C 6C" )
	PORT_DIPSETTING(    0x80, "1C 2C / 1C 2C" )
	PORT_DIPSETTING(    0x90, "1C 2C / 1C 4C" )
	PORT_DIPSETTING(    0xa0, "1C 2C / 1C 5C" )
	PORT_DIPSETTING(    0xb0, "1C 2C / 1C 10C" )
	PORT_DIPSETTING(    0xc0, "1C 2C / 1C 11C" )
	PORT_DIPSETTING(    0xd0, "1C 2C / 1C 12C" )
	PORT_DIPSETTING(    0xe0, "1C 2C / 1C 6C" )
	PORT_DIPSETTING(    0xf0, DEF_STR( Free_Play ) )

	PORT_START  /* dsw2 */
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x00, "Normal" )
	PORT_DIPSETTING(    0x01, "Hard" )
	PORT_DIPNAME( 0x02, 0x00, "Unknown 2-2" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x02, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x00, "Unknown 2-3" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x04, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x00, "Unknown 2-4" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x00, "Unknown 2-5" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x10, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x00, "Unknown 2-6" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x20, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x00, "Unknown 2-7" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x00, "Freeze" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x80, DEF_STR( On ) )

	PORT_START /* d804 */
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON2 )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN  | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP    | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_8WAY )
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY )

	PORT_START /* d805 */
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON2 | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )

	PORT_START /* d803 */
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_SERVICE1 )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_SERVICE( 0x20, IP_ACTIVE_HIGH )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN1 )
INPUT_PORTS_END

INPUT_PORTS_START( pettanp )
	PORT_START  /* dsw1 */
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x01, "5" )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x04, 0x00, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x04, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x00, "Unknown 1-4" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	PORT_DIPNAME( 0xf0, 0x00, "Coin1 / Coin2" )
	PORT_DIPSETTING(    0x00, "1C 1C / 1C 1C" )
	PORT_DIPSETTING(    0x10, "2C 1C / 2C 1C" )
	PORT_DIPSETTING(    0x20, "2C 1C / 1C 3C" )
	PORT_DIPSETTING(    0x30, "1C 1C / 1C 2C" )
	PORT_DIPSETTING(    0x40, "1C 1C / 1C 3C" )
	PORT_DIPSETTING(    0x50, "1C 1C / 1C 4C" )
	PORT_DIPSETTING(    0x60, "1C 1C / 1C 5C" )
	PORT_DIPSETTING(    0x70, "1C 1C / 1C 6C" )
	PORT_DIPSETTING(    0x80, "1C 2C / 1C 2C" )
	PORT_DIPSETTING(    0x90, "1C 2C / 1C 4C" )
	PORT_DIPSETTING(    0xa0, "1C 2C / 1C 5C" )
	PORT_DIPSETTING(    0xb0, "1C 2C / 1C 10C" )
	PORT_DIPSETTING(    0xc0, "1C 2C / 1C 11C" )
	PORT_DIPSETTING(    0xd0, "1C 2C / 1C 12C" )
	PORT_DIPSETTING(    0xe0, "1C 2C / 1C 6C" )
	PORT_DIPSETTING(    0xf0, DEF_STR( Free_Play ) )

	PORT_START  /* dsw2 */
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x06, 0x00, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x00, "20000 50000" )
	PORT_DIPSETTING(    0x02, "20000 80000" )
	PORT_DIPSETTING(    0x04, "20000" )
	PORT_DIPSETTING(    0x06, "None" )
	PORT_DIPNAME( 0x08, 0x00, "Second Practice" )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x00, "Unknown 2-5" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x10, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x00, "Unknown 2-6" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x20, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x00, "Unknown 2-7" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x00, "Freeze" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x80, DEF_STR( On ) )

	PORT_START /* d804 */
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN  | IPF_4WAY )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP    | IPF_4WAY )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_4WAY )
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_4WAY )

	PORT_START /* d805 */
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN  | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP    | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_4WAY | IPF_COCKTAIL )

	PORT_START /* d803 */
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_SERVICE1 )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_SERVICE( 0x20, IP_ACTIVE_HIGH )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN1 )
INPUT_PORTS_END

/****************************************************************************/

static struct GfxLayout charlayout =
{
	8,8,    /* 8*8 characters */
	1024,   /* 1024 characters */
	3,      /* 3 bits per pixel */
	{0,8192*8,8192*8*2},
	{7,6,5,4,3,2,1,0},
	{8*0, 8*1, 8*2, 8*3, 8*4, 8*5, 8*6, 8*7},
	8*8
};

static struct GfxLayout spritelayout =
{
	16,16,  /* 16*16 characters */
	256,    /* 256 characters */
	3,      /* 3 bits per pixel */
	{8192*8*2,8192*8,0},
	{7,6,5,4,3,2,1,0,
		8*16+7,8*16+6,8*16+5,8*16+4,8*16+3,8*16+2,8*16+1,8*16+0},
	{8*0, 8*1, 8*2, 8*3, 8*4, 8*5, 8*6, 8*7,
		8*8,8*9,8*10,8*11,8*12,8*13,8*14,8*15},
	8*8*4
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX2, 0x0000, &charlayout,   512, 64 },
	{ REGION_GFX1, 0x0000, &spritelayout, 0,   64 },
	{ -1 } /* end of array */
};


static struct SN76496interface sn76496_interface =
{
	2,	/* 2 chips */
	{ 8000000/4, 8000000/2 },
	{ 75, 75 }
};


static MACHINE_DRIVER_START( strnskil )

	/* basic machine hardware */
	MDRV_CPU_ADD(Z80,8000000/2) /* 4.000MHz */
	MDRV_CPU_MEMORY(strnskil_readmem1,strnskil_writemem1)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,2)

	MDRV_CPU_ADD(Z80,8000000/2) /* 4.000MHz */
	MDRV_CPU_MEMORY(strnskil_readmem2,strnskil_writemem2)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,2)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(100)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(1*8, 31*8-1, 2*8, 30*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(256)
	MDRV_COLORTABLE_LENGTH(1024)

	MDRV_PALETTE_INIT(strnskil)
	MDRV_VIDEO_START(strnskil)
	MDRV_VIDEO_UPDATE(strnskil)

	/* sound hardware */
	MDRV_SOUND_ADD(SN76496, sn76496_interface)
MACHINE_DRIVER_END

/****************************************************************************/

ROM_START( strnskil )
	ROM_REGION( 0x10000, REGION_CPU1, 0 ) /* main CPU */
	ROM_LOAD( "tvg3.7",  0x0000,  0x2000, CRC(31fd793a) SHA1(b86efe8ea60edf414a23fb6abc09db691c085fe9) )
	ROM_CONTINUE(        0x8000,  0x2000 )
	ROM_LOAD( "tvg4.8",  0x2000,  0x2000, CRC(c58315b5) SHA1(2039cd89ef59d05f353f6c367fa851c0f60cdc4a) )
	ROM_LOAD( "tvg5.9",  0x4000,  0x2000, CRC(29e7ded5) SHA1(6eae5988139f22c3ff166192e4fda77db38a79bc) )
	ROM_LOAD( "tvg6.10", 0x6000,  0x2000, CRC(8b126a4b) SHA1(68b617c5dc120c777e152919cba9daeaf3ceac5f) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* sub CPU */
	ROM_LOAD( "tvg1.2",  0x0000,  0x2000, CRC(b586b753) SHA1(7c9891fb279b1323c059ffdcf7c009bf971037be) )
	ROM_LOAD( "tvg2.3",  0x2000,  0x2000, CRC(8bd71bb6) SHA1(cc35e1e4cbb893ab04f1b6ceef0a050243e3b462) )

	ROM_REGION( 0x6000, REGION_GFX1, ROMREGION_DISPOSE ) /* sprite */
	ROM_LOAD( "tvg7.90",   0x0000,  0x2000, CRC(ee3bd593) SHA1(398e426e53695cc184d5a2750fd32a1c2c68bf30) )
	ROM_LOAD( "tvg8.92",   0x2000,  0x2000, CRC(1b265360) SHA1(fbc64c504639106c1813bf91bd31bda1ce4c7ffe) )
	ROM_LOAD( "tvg9.94",   0x4000,  0x2000, CRC(776c7ca6) SHA1(23fd1ac15395822b318db4435e48dd4e0e3e61de) )

	ROM_REGION( 0x6000, REGION_GFX2, ROMREGION_DISPOSE ) /* bg */
	ROM_LOAD( "tvg12.102", 0x0000,  0x2000, CRC(68b9d888) SHA1(7a4071fe882c1949979f97a020d7c6e95643ef42) )
	ROM_LOAD( "tvg11.101", 0x2000,  0x2000, CRC(7f2179ff) SHA1(24fab1f4430ae883bc1f477d3df7643e06c67349) )
	ROM_LOAD( "tvg10.100", 0x4000,  0x2000, CRC(321ad963) SHA1(9b50fbf0c3b4ce7ce3c68339b99a2ccadef4646f) )

	ROM_REGION( 0x0800, REGION_PROMS, 0 ) /* color PROMs */
	ROM_LOAD( "15-3.prm", 0x0000,  0x0100, CRC(dbcd3bec) SHA1(1baeec277b16c82b67e10da9d4c84cf383ef4a82) ) /* R */
	ROM_LOAD( "15-4.prm", 0x0100,  0x0100, CRC(9eb7b6cf) SHA1(86451e8a510f8cfbc0be7d4e7bb1ee7dfd67f1f4) ) /* G */
	ROM_LOAD( "15-5.prm", 0x0200,  0x0100, CRC(9b30a7f3) SHA1(a0aefc2c8325b95ea227e404583d14622b04a3b9) ) /* B */
	ROM_LOAD( "15-1.prm", 0x0300,  0x0200, CRC(d4f5b3d7) SHA1(9a244c77a752df655ff756e063d56c2c767e37d9) ) /* sprite */
	ROM_LOAD( "15-2.prm", 0x0500,  0x0200, CRC(cdffede9) SHA1(3ecdf91e3f78eb6cdd3a6f58d1a89d448a676c52) ) /* bg */

	ROM_REGION( 0x0100, REGION_USER1, 0 ) /* scroll control PROM */
	ROM_LOAD( "15-6.prm", 0x0000,  0x0100, CRC(ec4faf5b) SHA1(7ebbf50807d04105ebadec91bded069408e399ba) )
ROM_END

ROM_START( guiness )
	ROM_REGION( 0x10000, REGION_CPU1, 0 ) /* main CPU */
	ROM_LOAD( "tvg3.15", 0x0000,  0x2000, CRC(3a605ad8) SHA1(f6e2dd4989fdb68bc55857f5a8f06601416139d5) )
	ROM_CONTINUE(        0x8000,  0x2000 )
	ROM_LOAD( "tvg4.8",  0x2000,  0x2000, CRC(c58315b5) SHA1(2039cd89ef59d05f353f6c367fa851c0f60cdc4a) )
	ROM_LOAD( "tvg5.9",  0x4000,  0x2000, CRC(29e7ded5) SHA1(6eae5988139f22c3ff166192e4fda77db38a79bc) )
	ROM_LOAD( "tvg6.10", 0x6000,  0x2000, CRC(8b126a4b) SHA1(68b617c5dc120c777e152919cba9daeaf3ceac5f) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* sub CPU */
	ROM_LOAD( "tvg1.2",  0x0000,  0x2000, CRC(b586b753) SHA1(7c9891fb279b1323c059ffdcf7c009bf971037be) )
	ROM_LOAD( "tvg2.3",  0x2000,  0x2000, CRC(8bd71bb6) SHA1(cc35e1e4cbb893ab04f1b6ceef0a050243e3b462) )

	ROM_REGION( 0x6000, REGION_GFX1, ROMREGION_DISPOSE ) /* sprite */
	ROM_LOAD( "tvg7.90",   0x0000,  0x2000, CRC(ee3bd593) SHA1(398e426e53695cc184d5a2750fd32a1c2c68bf30) )
	ROM_LOAD( "tvg8.92",   0x2000,  0x2000, CRC(1b265360) SHA1(fbc64c504639106c1813bf91bd31bda1ce4c7ffe) )
	ROM_LOAD( "tvg9.94",   0x4000,  0x2000, CRC(776c7ca6) SHA1(23fd1ac15395822b318db4435e48dd4e0e3e61de) )

	ROM_REGION( 0x6000, REGION_GFX2, ROMREGION_DISPOSE ) /* bg */
	ROM_LOAD( "tvg12.15", 0x0000,  0x2000, CRC(a82c923d) SHA1(2bd2b028d782fac18f2fe9c9ef73ce0af67db347) )
	ROM_LOAD( "tvg11.15", 0x2000,  0x2000, CRC(d432c96f) SHA1(0d4b3af778dbd40bc26bad4c673a9ce1ef537c04) )
	ROM_LOAD( "tvg10.15", 0x4000,  0x2000, CRC(a53959d6) SHA1(cdf7acf1a75d83b259948c482f06543624a695a3) )

	ROM_REGION( 0x0800, REGION_PROMS, 0 ) /* color PROMs */
	ROM_LOAD( "15-3.prm", 0x0000,  0x0100, CRC(dbcd3bec) SHA1(1baeec277b16c82b67e10da9d4c84cf383ef4a82) ) /* R */
	ROM_LOAD( "15-4.prm", 0x0100,  0x0100, CRC(9eb7b6cf) SHA1(86451e8a510f8cfbc0be7d4e7bb1ee7dfd67f1f4) ) /* G */
	ROM_LOAD( "15-5.prm", 0x0200,  0x0100, CRC(9b30a7f3) SHA1(a0aefc2c8325b95ea227e404583d14622b04a3b9) ) /* B */
	ROM_LOAD( "15-1.prm", 0x0300,  0x0200, CRC(d4f5b3d7) SHA1(9a244c77a752df655ff756e063d56c2c767e37d9) ) /* sprite */
	ROM_LOAD( "15-2.prm", 0x0500,  0x0200, CRC(cdffede9) SHA1(3ecdf91e3f78eb6cdd3a6f58d1a89d448a676c52) ) /* bg */

	ROM_REGION( 0x0100, REGION_USER1, 0 ) /* scroll control PROM */
	ROM_LOAD( "15-6.prm", 0x0000,  0x0100, CRC(ec4faf5b) SHA1(7ebbf50807d04105ebadec91bded069408e399ba) )
ROM_END

ROM_START( pettanp )
	ROM_REGION( 0x10000, REGION_CPU1, 0 ) /* main CPU */
	ROM_LOAD( "tvg2-16a.7",  0x0000,  0x2000, CRC(4cbbbd01) SHA1(3905cf9e9d324bb23688ab29c98d71529d3dbf0c) )
	ROM_CONTINUE(            0x8000,  0x2000 )
	ROM_LOAD( "tvg3-16a.8",  0x2000,  0x2000, CRC(aaa0420f) SHA1(aa7ead51002f8b1bbefd07ff23b9064804fc31b3) )
	ROM_LOAD( "tvg4-16a.9",  0x4000,  0x2000, CRC(43306369) SHA1(1eadebd3d962da49fd204eff8692f1e1a1e3cc98) )
	ROM_LOAD( "tvg5-16a.10", 0x6000,  0x2000, CRC(da9c635f) SHA1(3c084ad159dbabfd02a9772489c3193852d135b7) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* sub CPU */
	ROM_LOAD( "tvg1-16.2",   0x0000,  0x2000, CRC(e36009f6) SHA1(72c485e8c19fbfc9c850094cfd87f1055154c0c5) )

	ROM_REGION( 0x6000, REGION_GFX1, ROMREGION_DISPOSE ) /* sprite */
	ROM_LOAD( "tvg6-16.90",  0x0000,  0x2000, CRC(6905d9d5) SHA1(586bf72bab5ab6e3e319c925decc16d7f3711af1) )
	ROM_LOAD( "tvg7-16.92",  0x2000,  0x2000, CRC(40d02bfd) SHA1(2f6ca8197048318f7900b56169aba4c9fdf48693) )
	ROM_LOAD( "tvg8-16.94",  0x4000,  0x2000, CRC(b18a2244) SHA1(168061e050530e6a5bc78c14a64e635370256dfd) )

	ROM_REGION( 0x6000, REGION_GFX2, ROMREGION_DISPOSE ) /* bg */
	ROM_LOAD( "tvg11-16.102",0x0000,  0x2000, CRC(327b7a29) SHA1(4b8d57607c4a1e84c630c38eba3fa90b5496dcde) )
	ROM_LOAD( "tvg10-16.101",0x2000,  0x2000, CRC(624ac061) SHA1(9d479a8a256a8ff37c00bc7449b11357f9fe6cdc) )
	ROM_LOAD( "tvg9-16.100", 0x4000,  0x2000, CRC(c477e74c) SHA1(864eddcd9c817aeecb09423071f87d3b39eb5fc4) )

	ROM_REGION( 0x0700, REGION_PROMS, 0 ) /* color PROMs */
	ROM_LOAD( "16-3.66",  0x0000,  0x0100, CRC(dbcd3bec) SHA1(1baeec277b16c82b67e10da9d4c84cf383ef4a82) ) /* R */
	ROM_LOAD( "16-4.67",  0x0100,  0x0100, CRC(9eb7b6cf) SHA1(86451e8a510f8cfbc0be7d4e7bb1ee7dfd67f1f4) ) /* G */
	ROM_LOAD( "16-5.68",  0x0200,  0x0100, CRC(9b30a7f3) SHA1(a0aefc2c8325b95ea227e404583d14622b04a3b9) ) /* B */
	ROM_LOAD( "16-1.148", 0x0300,  0x0200, CRC(777e2770) SHA1(7f4ef42ab4e0546c2932d498cf573bd4f4296db7) ) /* sprite */
	ROM_LOAD( "16-2.97",  0x0500,  0x0200, CRC(7f95d4b2) SHA1(68dc311739a4d5d72f4cfbace27f3a82f05316ff) ) /* bg */

	ROM_REGION( 0x0100, REGION_USER1, 0 ) /* scroll control PROM */
/*	ROM_LOAD( "16-6",     0x0000,  0x0100, NO_DUMP )*/

	ROM_REGION( 0x1000, REGION_USER2, 0 ) /* protection? */
	ROM_LOAD( "tvg12-16.2", 0x0000,  0x1000, CRC(3abc6ba8) SHA1(15e0b0f9d068f6094e2be4f4f1dea0ff6e85686b) )
ROM_END

GAME(  1984, strnskil, 0,        strnskil, strnskil, 0, ROT0, "Sun Electronics", "Strength and Skill" )
GAME(  1984, guiness,  strnskil, strnskil, strnskil, 0, ROT0, "Sun Electronics", "The Guiness (Japan)" )
GAMEX( 1984, pettanp,  0,        strnskil, pettanp,  0, ROT0, "Sun Electronics", "Pettan Pyuu (Japan)", GAME_UNEMULATED_PROTECTION )
