/******************************************************************************

	Game Driver for Nichibutsu Mahjong series.

	Pastel Gal
	(c)1985 Nihon Bussan Co.,Ltd.

	Driver by Takahiro Nogi <nogi@kt.rim.or.jp> 2000/06/07 -

******************************************************************************/
/******************************************************************************
Memo:

- Custom chip used by pastelgl PCB is 1411M1.

- Some games display "GFXROM BANK OVER!!" or "GFXROM ADDRESS OVER!!"
  in Debug build.

- Screen flip is not perfect.

******************************************************************************/

#include "driver.h"
#include "cpu/z80/z80.h"
#include "vidhrdw/generic.h"
#include "nb1413m3.h"


#define	SIGNED_DAC	0		/* 0:unsigned DAC, 1:signed DAC*/


PALETTE_INIT( pastelgl );
VIDEO_UPDATE( pastelgl );
VIDEO_START( pastelgl );

void pastelgl_paltbl_w(int offset, int data);
void pastelgl_radrx_w(int data);
void pastelgl_radry_w(int data);
void pastelgl_sizex_w(int data);
void pastelgl_sizey_w(int data);
void pastelgl_drawx_w(int data);
void pastelgl_drawy_w(int data);
void pastelgl_dispflag_w(int data);
void pastelgl_romsel_w(int data);


static int voiradr_l, voiradr_h;


void pastelgl_voiradr_l_w(int data)
{
	voiradr_l = data;
}

void pastelgl_voiradr_h_w(int data)
{
	voiradr_h = data;
}

int pastelgl_sndrom_r(int offset)
{
	unsigned char *ROM = memory_region(REGION_SOUND1);

	return ROM[(((0x0100 * voiradr_h) + voiradr_l) & 0x7fff)];
}

static DRIVER_INIT( pastelgl )
{
	nb1413m3_type = NB1413M3_PASTELGL;
}


static MEMORY_READ_START( readmem_pastelgl )
	{ 0x0000, 0xbfff, MRA_ROM },
	{ 0xe000, 0xe7ff, MRA_RAM },
MEMORY_END

static MEMORY_WRITE_START( writemem_pastelgl )
	{ 0x0000, 0xbfff, MWA_ROM },
	{ 0xe000, 0xe7ff, MWA_RAM, &nb1413m3_nvram, &nb1413m3_nvram_size },
MEMORY_END


static READ_HANDLER( io_pastelgl_r )
{
	offset = (((offset & 0xff00) >> 8) | ((offset & 0x00ff) << 8));

	if (offset < 0x8000) return nb1413m3_sndrom_r(offset);

	switch (offset & 0xff00)
	{
		case	0x8100:	return AY8910_read_port_0_r(0);
		case	0x9000:	return nb1413m3_inputport0_r(0);
		case	0xa000:	return nb1413m3_inputport1_r(0);
		case	0xb000:	return nb1413m3_inputport2_r(0);
		case	0xc000:	return pastelgl_sndrom_r(0);
		case	0xe000:	return input_port_2_r(0);
		case	0xf000:	return nb1413m3_dipsw1_r(0);
		case	0xf100:	return nb1413m3_dipsw2_r(0);
		default:	return 0xff;
	}
}

static PORT_READ_START( readport_pastelgl )
	{ 0x0000, 0xffff, io_pastelgl_r },
PORT_END

static WRITE_HANDLER( io_pastelgl_w )
{
	offset = (((offset & 0xff00) >> 8) | ((offset & 0x00ff) << 8));

	if ((0xc000 <= offset) && (0xd000 > offset))
	{
		pastelgl_paltbl_w(((offset & 0x0f00) >> 8), data);
		return;
	}

	switch (offset & 0xff00)
	{
		case	0x0000:	break;
		case	0x8200:	AY8910_write_port_0_w(0, data); break;
		case	0x8300:	AY8910_control_port_0_w(0, data); break;
		case	0x9000:	pastelgl_radrx_w(data);
				pastelgl_voiradr_l_w(data); break;
		case	0x9100:	pastelgl_radry_w(data);
				pastelgl_voiradr_h_w(data); break;
		case	0x9200:	pastelgl_drawx_w(data); break;
		case	0x9300:	pastelgl_drawy_w(data); break;
		case	0x9400:	pastelgl_sizex_w(data); break;
		case	0x9500:	pastelgl_sizey_w(data); break;
		case	0x9600:	pastelgl_dispflag_w(data); break;
		case	0x9700:	break;
		case	0xa000:	nb1413m3_inputportsel_w(0,data); break;
		case	0xb000:	pastelgl_romsel_w(data);
				nb1413m3_sndrombank1_w(0,data);
				break;
#if SIGNED_DAC
		case	0xd000:	DAC_0_signed_data_w(0, data); break;
#else
		case	0xd000:	DAC_0_data_w(0, data); break;
#endif
	}
}

static PORT_WRITE_START( writeport_pastelgl )
	{ 0x0000, 0xffff, io_pastelgl_w },
PORT_END


INPUT_PORTS_START( pastelgl )
	PORT_START	/* (0) DIPSW-A */
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x03, "1 (Easy)" )
	PORT_DIPSETTING(    0x02, "2" )
	PORT_DIPSETTING(    0x01, "3" )
	PORT_DIPSETTING(    0x00, "4 (Hard)" )
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

	PORT_START	/* (1) DIPSW-B */
	PORT_DIPNAME( 0x03, 0x00, "Number of last chance" )
	PORT_DIPSETTING(    0x03, "0" )
	PORT_DIPSETTING(    0x02, "1" )
	PORT_DIPSETTING(    0x01, "3" )
	PORT_DIPSETTING(    0x00, "10" )
	PORT_DIPNAME( 0x04, 0x04, "No. of tiles on final match" )
	PORT_DIPSETTING(    0x04, "20" )
	PORT_DIPSETTING(    0x00, "10" )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x60, 0x00, "SANGEN Rush" )
	PORT_DIPSETTING(    0x06, "0" )
	PORT_DIPSETTING(    0x04, "1" )
	PORT_DIPSETTING(    0x02, "5" )
	PORT_DIPSETTING(    0x00, "infinite" )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START	/* (2) DIPSW-C */
	PORT_DIPNAME( 0x03, 0x03, "Change Rate" )
	PORT_DIPSETTING(    0x03, "Type-A" )
	PORT_DIPSETTING(    0x02, "Type-B" )
	PORT_DIPSETTING(    0x01, "Type-C" )
	PORT_DIPSETTING(    0x00, "Type-D" )
	PORT_DIPNAME( 0x04, 0x00, "Open CPU's hand on Player's Reach" )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x60, 0x60, "YAKUMAN cut" )
	PORT_DIPSETTING(    0x60, "10%" )
	PORT_DIPSETTING(    0x40, "30%" )
	PORT_DIPSETTING(    0x20, "50%" )
	PORT_DIPSETTING(    0x00, "90%" )
	PORT_DIPNAME( 0x80, 0x00, "Nudity" )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START	/* (3) PORT 0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNUSED )		/* DRAW BUSY*/
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNUSED )		/**/
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE3 )		/* MEMORY RESET*/
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_SERVICE2 )		/* ANALYZER*/
	PORT_SERVICE( 0x10, IP_ACTIVE_LOW )			/* TEST*/
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN1 )		/* COIN1*/
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_SERVICE4 )		/* CREDIT CLEAR*/
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_SERVICE1 )		/* SERVICE*/

	NBMJCTRL_PORT1	/* (4) PORT 1-1 */
	NBMJCTRL_PORT2	/* (5) PORT 1-2 */
	NBMJCTRL_PORT3	/* (6) PORT 1-3 */
	NBMJCTRL_PORT4	/* (7) PORT 1-4 */
	NBMJCTRL_PORT5	/* (8) PORT 1-5 */
INPUT_PORTS_END


static struct AY8910interface ay8910_interface =
{
	1,				/* 1 chip */
	1250000,			/* 1.25 MHz ?? */
	{ 35 },
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 }
};


static struct DACinterface dac_interface =
{
	1,				/* 1 channels */
	{ 50 }
};


static MACHINE_DRIVER_START( pastelgl )

	/* basic machine hardware */
	MDRV_CPU_ADD(Z80, 19968000/8)	/* 2.496 MHz ? */
	MDRV_CPU_FLAGS(CPU_16BIT_PORT)
	MDRV_CPU_MEMORY(readmem_pastelgl, writemem_pastelgl)
	MDRV_CPU_PORTS(readport_pastelgl, writeport_pastelgl)
	MDRV_CPU_VBLANK_INT(nb1413m3_interrupt,96)	/* nmiclock not written, chip is 1411M1 instead of 1413M3*/

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_NVRAM_HANDLER(nb1413m3)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(256, 256)
	MDRV_VISIBLE_AREA(0, 256-1, 16, 240-1)
	MDRV_PALETTE_LENGTH(32)

	MDRV_PALETTE_INIT(pastelgl)
	MDRV_VIDEO_START(pastelgl)
	MDRV_VIDEO_UPDATE(pastelgl)

	/* sound hardware */
	MDRV_SOUND_ADD(AY8910, ay8910_interface)
	MDRV_SOUND_ADD(DAC, dac_interface)
MACHINE_DRIVER_END


ROM_START( pastelgl )
	ROM_REGION( 0x10000, REGION_CPU1, 0 ) /* program */
	ROM_LOAD( "pgal_09.bin",  0x00000, 0x04000, CRC(1e494af3) SHA1(1597a7da22ecfbb1df83cf9d0acc7a8be461bc2c) )
	ROM_LOAD( "pgal_10.bin",  0x04000, 0x04000, CRC(677cccea) SHA1(a294bf4e3c5e74291160a0858371961868afc1d1) )
	ROM_LOAD( "pgal_11.bin",  0x08000, 0x04000, CRC(c2ccea38) SHA1(0374e8aa0e7961426e417ffe6e1a0d8dc7fd9ecf) )

	ROM_REGION( 0x08000, REGION_SOUND1, 0 ) /* voice */
	ROM_LOAD( "pgal_08.bin",  0x00000, 0x08000, CRC(895961a1) SHA1(f02d517f46cc490db02c4feb369e2a386c764297) )

	ROM_REGION( 0x38000, REGION_GFX1, 0 ) /* gfx */
	ROM_LOAD( "pgal_01.bin",  0x00000, 0x08000, CRC(1bb14d52) SHA1(b3974e3c9b56a752ddcb206f7bb2bc658b0e77f1) )
	ROM_LOAD( "pgal_02.bin",  0x08000, 0x08000, CRC(ea85673a) SHA1(85ef2bb736fe5229ce4153197db8a57bca982a8b) )
	ROM_LOAD( "pgal_03.bin",  0x10000, 0x08000, CRC(40011248) SHA1(935f442a47e02bf8c6ccb324c7fad1b481b8b19a) )
	ROM_LOAD( "pgal_04.bin",  0x18000, 0x08000, CRC(10613a66) SHA1(ad11f99f402e5b247d086cfccafea351da30c084) )
	ROM_LOAD( "pgal_05.bin",  0x20000, 0x08000, CRC(6a152703) SHA1(5dd46d876453c5c79f5a382d77234c690da75001) )
	ROM_LOAD( "pgal_06.bin",  0x28000, 0x08000, CRC(f56acfe8) SHA1(2f4ad3990f2d4d4a9fcec7adab119459423b308b) )
	ROM_LOAD( "pgal_07.bin",  0x30000, 0x08000, CRC(fa4226dc) SHA1(2313449521f81a191e87f1e4c0f3473f3c27dd9d) )

	ROM_REGION( 0x0040, REGION_PROMS, 0 ) /* color */
	ROM_LOAD( "pgal_bp1.bin", 0x0000, 0x0020, CRC(2b7fc61a) SHA1(278830e8728ea143208376feb20fff56de88ae1c) )
	ROM_LOAD( "pgal_bp2.bin", 0x0020, 0x0020, CRC(4433021e) SHA1(e0d6619a193d26ad24788d4af5ef01ee89cffacd) )
ROM_END


GAME( 1985, pastelgl, 0, pastelgl, pastelgl, pastelgl, ROT0, "Nichibutsu", "Pastel Gal (Japan 851224)" )
