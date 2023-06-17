/***************************************************************************

Taxi Driver  (c) 1984 Graphic Techno

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "machine/8255ppi.h"
#include "taxidrvr.h"



WRITE_HANDLER( p2a_w ) { taxidrvr_spritectrl_w(0,data); }
WRITE_HANDLER( p2b_w ) { taxidrvr_spritectrl_w(1,data); }
WRITE_HANDLER( p2c_w ) { taxidrvr_spritectrl_w(2,data); }
WRITE_HANDLER( p3a_w ) { taxidrvr_spritectrl_w(3,data); }
WRITE_HANDLER( p3b_w ) { taxidrvr_spritectrl_w(4,data); }
WRITE_HANDLER( p3c_w ) { taxidrvr_spritectrl_w(5,data); }
WRITE_HANDLER( p4a_w ) { taxidrvr_spritectrl_w(6,data); }
WRITE_HANDLER( p4b_w ) { taxidrvr_spritectrl_w(7,data); }
WRITE_HANDLER( p4c_w ) { taxidrvr_spritectrl_w(8,data); }




static int s1,s2,s3,s4,latchA,latchB;

static READ_HANDLER( p0a_r )
{
	return latchA;
}

static READ_HANDLER( p0c_r )
{
	return (s1 << 7);
}

static WRITE_HANDLER( p0b_w )
{
	latchB = data;
}

static WRITE_HANDLER( p0c_w )
{
	s2 = data & 1;

	taxidrvr_bghide = data & 2;

	/* bit 2 toggles during gameplay */

	flip_screen_set(data & 8);

/*	usrintf_showmessage("%02x",data&0x0f);*/
}

static READ_HANDLER( p1b_r )
{
	return latchB;
}

static READ_HANDLER( p1c_r )
{
	return (s2 << 7) | (s4 << 6) | ((readinputport(5) & 1) << 4);
}

static WRITE_HANDLER( p1a_w )
{
	latchA = data;
}

static WRITE_HANDLER( p1c_w )
{
	s1 = data & 1;
	s3 = (data & 2) >> 1;
}

static READ_HANDLER( p8910_0a_r )
{
	return latchA;
}

static READ_HANDLER( p8910_1a_r )
{
	return s3;
}

/* note that a lot of writes happen with port B set as input. I think this is a bug in the
   original, since it works anyway even if the communication is flawed. */
static WRITE_HANDLER( p8910_0b_w )
{
	s4 = data & 1;
}


static ppi8255_interface ppi8255_intf =
{
	5, 										/* 5 chips */
	{ p0a_r, NULL,  NULL,  NULL,  NULL  },	/* Port A read */
	{ NULL,  p1b_r, NULL,  NULL,  NULL  },	/* Port B read */
	{ p0c_r, p1c_r, NULL,  NULL,  NULL  },	/* Port C read */
	{ NULL,  p1a_w, p2a_w, p3a_w, p4a_w },	/* Port A write */
	{ p0b_w, NULL,  p2b_w, p3b_w, p4b_w },	/* Port B write */
	{ p0c_w, p1c_w, p2c_w, p3c_w, p4c_w }	/* Port C write */
};

MACHINE_INIT( taxidrvr )
{
	ppi8255_init(&ppi8255_intf);
}



static MEMORY_READ_START( readmem1 )
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0x8fff, MRA_RAM },
	{ 0x9000, 0x9fff, MRA_RAM },
	{ 0xa000, 0xafff, MRA_RAM },
	{ 0xb000, 0xbfff, MRA_RAM },
	{ 0xc000, 0xc7ff, MRA_RAM },
	{ 0xd800, 0xdfff, MRA_RAM },
	{ 0xe000, 0xf3ff, MRA_RAM },
	{ 0xf400, 0xf403, ppi8255_0_r },
	{ 0xf480, 0xf483, ppi8255_2_r },
	{ 0xf500, 0xf503, ppi8255_3_r },
	{ 0xf580, 0xf583, ppi8255_4_r },
	{ 0xf800, 0xffff, MRA_RAM },
MEMORY_END

static MEMORY_WRITE_START( writemem1 )
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0x8000, 0x8fff, MWA_RAM },	/* ??? */
	{ 0x9000, 0x9fff, MWA_RAM },	/* ??? */
	{ 0xa000, 0xafff, MWA_RAM },	/* ??? */
	{ 0xb000, 0xbfff, MWA_RAM },	/* ??? */
	{ 0xc000, 0xc7ff, MWA_RAM, &taxidrvr_vram4 },	/* radar bitmap */
	{ 0xc800, 0xcfff, MWA_RAM, &taxidrvr_vram5 },	/* "sprite1" bitmap */
	{ 0xd000, 0xd7ff, MWA_RAM, &taxidrvr_vram6 },	/* "sprite2" bitmap */
	{ 0xd800, 0xdfff, MWA_RAM, &taxidrvr_vram7 },	/* "sprite3" bitmap */
	{ 0xe000, 0xe3ff, MWA_RAM, &taxidrvr_vram1 },	/* car tilemap */
	{ 0xe400, 0xebff, MWA_RAM, &taxidrvr_vram2 },	/* bg1 tilemap */
	{ 0xec00, 0xefff, MWA_RAM, &taxidrvr_vram0 },	/* fg tilemap */
	{ 0xf000, 0xf3ff, MWA_RAM, &taxidrvr_vram3 },	/* bg2 tilemap */
	{ 0xf400, 0xf403, ppi8255_0_w },
	{ 0xf480, 0xf483, ppi8255_2_w },	/* "sprite1" placement */
	{ 0xf500, 0xf503, ppi8255_3_w },	/* "sprite2" placement */
	{ 0xf580, 0xf583, ppi8255_4_w },	/* "sprite3" placement */
/*	{ 0xf780, 0xf781, MWA_RAM },		 // more scroll registers? /*/
	{ 0xf782, 0xf787, MWA_RAM, &taxidrvr_scroll },	/* bg scroll (three copies always identical) */
	{ 0xf800, 0xffff, MWA_RAM },
MEMORY_END

static MEMORY_READ_START( readmem2 )
	{ 0x0000, 0x3fff, MRA_ROM },
	{ 0x6000, 0x67ff, MRA_RAM },
	{ 0x8000, 0x87ff, MRA_RAM },
	{ 0xa000, 0xa003, ppi8255_1_r },
	{ 0xe000, 0xe000, input_port_0_r },
	{ 0xe001, 0xe001, input_port_1_r },
	{ 0xe002, 0xe002, input_port_2_r },
	{ 0xe003, 0xe003, input_port_3_r },
	{ 0xe004, 0xe004, input_port_4_r },
MEMORY_END

static MEMORY_WRITE_START( writemem2 )
	{ 0x0000, 0x3fff, MWA_ROM },
	{ 0x6000, 0x67ff, MWA_RAM },
	{ 0x8000, 0x87ff, MWA_RAM },
	{ 0xa000, 0xa003, ppi8255_1_w },
MEMORY_END

static MEMORY_READ_START( readmem3 )
	{ 0x0000, 0x1fff, MRA_ROM },
	{ 0x2000, 0x2000, MRA_NOP },	/* irq ack? */
	{ 0xfc00, 0xffff, MRA_RAM },
MEMORY_END

static MEMORY_WRITE_START( writemem3 )
	{ 0x0000, 0x1fff, MWA_ROM },
	{ 0xfc00, 0xffff, MWA_RAM },
MEMORY_END

static PORT_READ_START( readport3 )
	{ 0x01, 0x01, AY8910_read_port_0_r },
	{ 0x03, 0x03, AY8910_read_port_1_r },
PORT_END

static PORT_WRITE_START( writeport3 )
	{ 0x00, 0x00, AY8910_control_port_0_w },
	{ 0x01, 0x01, AY8910_write_port_0_w },
	{ 0x02, 0x02, AY8910_control_port_1_w },
	{ 0x03, 0x03, AY8910_write_port_1_w },
PORT_END



INPUT_PORTS_START( taxidrvr )
	PORT_START
	PORT_DIPNAME( 0x0f, 0x00, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x0d, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x0a, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x0e, DEF_STR( 4C_2C ) )
	PORT_DIPSETTING(    0x07, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x0b, DEF_STR( 3C_2C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( 3C_4C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x09, DEF_STR( 2C_5C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0x05, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 1C_7C ) )
	PORT_DIPSETTING(    0x0f, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0xf0, 0x00, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0xd0, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0xa0, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0xe0, DEF_STR( 4C_2C ) )
	PORT_DIPSETTING(    0x70, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0xb0, DEF_STR( 3C_2C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 3C_4C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x90, DEF_STR( 2C_5C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0x50, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(    0x60, DEF_STR( 1C_7C ) )
	PORT_DIPSETTING(    0xf0, DEF_STR( Free_Play ) )

	PORT_START
	PORT_DIPNAME( 0x03, 0x00, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x01, "4" )
	PORT_DIPSETTING(    0x02, "5" )
	PORT_BITX( 0,       0x03, IPT_DIPSWITCH_SETTING | IPF_CHEAT, "255", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x38, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, "1" )
	PORT_DIPSETTING(    0x08, "2" )
	PORT_DIPSETTING(    0x10, "3" )
	PORT_DIPSETTING(    0x18, "4" )
	PORT_DIPSETTING(    0x20, "5" )
	PORT_DIPSETTING(    0x28, "6" )
	PORT_DIPSETTING(    0x30, "7" )
	PORT_DIPSETTING(    0x38, "8" )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, "0" )
	PORT_DIPSETTING(    0x40, "1" )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x80, DEF_STR( On ) )

	PORT_START
	PORT_DIPNAME( 0x07, 0x00, "Fuel Consumption" )
	PORT_DIPSETTING(    0x00, "Slowest" )
	PORT_DIPSETTING(    0x01, "2" )
	PORT_DIPSETTING(    0x02, "3" )
	PORT_DIPSETTING(    0x03, "4" )
	PORT_DIPSETTING(    0x04, "5" )
	PORT_DIPSETTING(    0x05, "6" )
	PORT_DIPSETTING(    0x06, "7" )
	PORT_DIPSETTING(    0x07, "Fastest" )
	PORT_DIPNAME( 0x38, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, "1" )
	PORT_DIPSETTING(    0x08, "2" )
	PORT_DIPSETTING(    0x10, "3" )
	PORT_DIPSETTING(    0x18, "4" )
	PORT_DIPSETTING(    0x20, "5" )
	PORT_DIPSETTING(    0x28, "6" )
	PORT_DIPSETTING(    0x30, "7" )
	PORT_DIPSETTING(    0x38, "8" )
	PORT_DIPNAME( 0xc0, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, "40/30" )
	PORT_DIPSETTING(    0x40, "30/20" )
	PORT_DIPSETTING(    0x80, "20/15" )
	PORT_DIPSETTING(    0xc0, "10/10" )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_BUTTON2 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP    | IPF_4WAY )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN  | IPF_4WAY )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_4WAY )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_4WAY )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_BUTTON2 | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP    | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN  | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_4WAY | IPF_COCKTAIL )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_SERVICE1 )	/* handled by p1c_r() */
INPUT_PORTS_END



static struct GfxLayout charlayout =
{
	8,8,
	RGN_FRAC(1,1),
	4,
	{ 0, 1, 2, 3  },
	{ 1*4, 0*4, 3*4, 2*4, 5*4, 4*4, 7*4, 6*4 },
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32 },
	32*8
};

static struct GfxLayout charlayout2 =
{
	4,4,
	RGN_FRAC(1,1),
	4,
	{ 0, 1, 2, 3  },
	{ 1*4, 0*4, 3*4, 2*4 },
	{ 0*16, 1*16, 2*16, 3*16 },
	16*4
};


static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &charlayout, 0, 1 },
	{ REGION_GFX2, 0, &charlayout, 0, 1 },
	{ REGION_GFX3, 0, &charlayout, 0, 1 },
	{ REGION_GFX4, 0, &charlayout, 0, 1 },
	{ REGION_GFX5, 0, &charlayout2, 0, 1 },
	{ -1 } /* end of array */
};

static PALETTE_INIT( taxidrvr )
{
	int	i;

	/* TODO: resistors, 1k & 470*/

	for (i = 0; i < 0x10; ++i)
	{
	    int bit0, bit1, r, g, b;

		bit0 = (color_prom[0] >> 0) & 0x01;
		bit1 = (color_prom[0] >> 1) & 0x01;
		r = 0x55 * bit0 + 0xaa * bit1;
		bit0 = (color_prom[0] >> 2) & 0x01;
		bit1 = (color_prom[0] >> 3) & 0x01;
		g = 0x55 * bit0 + 0xaa * bit1;
		bit0 = (color_prom[0] >> 4) & 0x01;
		bit1 = (color_prom[0] >> 5) & 0x01;
		b = 0x55 * bit0 + 0xaa * bit1;

		palette_set_color(i,r,g,b);
		color_prom++;
	}
}


static struct AY8910interface ay8910_interface =
{
	2,	/* 2 chips */
	1250000,	/* 1.25 MHz ??? */
	{ 25, 25 },
	{ p8910_0a_r, p8910_1a_r },
	{ 0, 0 },
	{ 0, 0 },
	{ p8910_0b_w, 0 }
};



static MACHINE_DRIVER_START( taxidrvr )

	/* basic machine hardware */
	MDRV_CPU_ADD(Z80,4000000)	/* 4 MHz ??? */
	MDRV_CPU_MEMORY(readmem1,writemem1)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,1)

	MDRV_CPU_ADD(Z80,4000000)	/* 4 MHz ??? */
	MDRV_CPU_MEMORY(readmem2,writemem2)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,1)	/* ??? */

	MDRV_CPU_ADD(Z80,4000000)	/* 4 MHz ??? */
	MDRV_CPU_MEMORY(readmem3,writemem3)
	MDRV_CPU_PORTS(readport3,writeport3)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,1)	/* ??? */

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(100)	/* 100 CPU slices per frame - an high value to ensure proper */
							/* synchronization of the CPUs */
	MDRV_MACHINE_INIT(taxidrvr)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 1*8, 27*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(16)

  MDRV_PALETTE_INIT(taxidrvr)
	MDRV_VIDEO_UPDATE(taxidrvr)

	/* sound hardware */
	MDRV_SOUND_ADD(AY8910, ay8910_interface)
MACHINE_DRIVER_END



/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( taxidrvr )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "1",            0x0000, 0x2000, CRC(6b2424e9) SHA1(a65bb01da8f3b0649d945981cc4f1324b7fac5c7) )
	ROM_LOAD( "2",            0x2000, 0x2000, CRC(15111229) SHA1(0350918f9504b0e470684ebc94a823bb2513a54d) )
	ROM_LOAD( "3",            0x4000, 0x2000, CRC(a7782eee) SHA1(0f10b7876420f4237937b1b922aa410de3f79af1) )
	ROM_LOAD( "4",            0x6000, 0x2000, CRC(8eb0b16b) SHA1(a0015744373ee91bc505f077a04ab3546f8bb6fb) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )
	ROM_LOAD( "8",            0x0000, 0x2000, CRC(9f9a3865) SHA1(908cf4f2cc68c088649241997276ea25c27d9718) )
	ROM_LOAD( "9",            0x2000, 0x2000, CRC(b28b766c) SHA1(21e08ef1e2671c8540380e3fa0858e8a4d821945) )

	ROM_REGION( 0x10000, REGION_CPU3, 0 )
	ROM_LOAD( "7",            0x0000, 0x2000, CRC(2b4cbfe6) SHA1(a2a900831116554d5aea1a81c93245d3bb424d48) )

	ROM_REGION( 0x2000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "5",            0x0000, 0x2000, CRC(a3aa5f2f) SHA1(7e046e2a5d230c62d93a83f5a773e6e4d6e85961) )

	ROM_REGION( 0x2000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "6",            0x0000, 0x2000, CRC(bfddd550) SHA1(f528c2701c635bc61eda14fbe2cfe9b44cb75c20) )

	ROM_REGION( 0x6000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "11",           0x0000, 0x2000, CRC(7485eaea) SHA1(8d69c61145470003cfeb33b11b81345c5e5e6503) )
	ROM_LOAD( "14",           0x2000, 0x2000, CRC(0d99a33e) SHA1(0df29464ea43aecd866ae322f4f7ca9152422023) )
	ROM_LOAD( "15",           0x4000, 0x2000, CRC(410fdf7c) SHA1(0957f335b84c4fbde983271786e7bf199fc22682) )

	ROM_REGION( 0x2000, REGION_GFX4, ROMREGION_DISPOSE )
	ROM_LOAD( "10",           0x0000, 0x2000, CRC(c370b177) SHA1(4b3f73f764ff95cc7777fe01333558201658cead) )

	ROM_REGION( 0x4000, REGION_GFX5, ROMREGION_DISPOSE )	/* not used?? */
	ROM_LOAD( "12",           0x0000, 0x2000, CRC(684b7bb0) SHA1(d83c45ff3adf94c649340227794020482231399f) )
	ROM_LOAD( "13",           0x2000, 0x2000, CRC(d1ef110e) SHA1(e34b6b4b70c783a8cf1296a05d3cec6af5820d0c) )

  ROM_REGION( 0x020, REGION_PROMS, 0 )
	ROM_LOAD( "prom.ic2",     0x0000, 0x020, CRC(c366a9c5) SHA1(d38581e5c425cab4a4f216d99651e86d8034a7d2) ) /* color prom located at edge of pcb */
ROM_END



GAMEX( 1984, taxidrvr, 0, taxidrvr, taxidrvr, 0, ROT90, "Graphic Techno", "Taxi Driver", GAME_NO_COCKTAIL )
