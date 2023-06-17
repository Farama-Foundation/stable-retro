#include "driver.h"
#include "vidhrdw/generic.h"

extern WRITE_HANDLER( pingpong_videoram_w );
extern WRITE_HANDLER( pingpong_colorram_w );

extern PALETTE_INIT( pingpong );
extern VIDEO_START( pingpong );
extern VIDEO_UPDATE( pingpong );

static int intenable;

static WRITE_HANDLER( coin_w )
{
	/* bit 2 = irq enable, bit 3 = nmi enable */
	intenable = data & 0x0c;

	/* bit 0/1 = coin counters */
	coin_counter_w(0,data & 1);
	coin_counter_w(1,data & 2);

	/* other bits unknown */
}

static INTERRUPT_GEN( pingpong_interrupt )
{
	if (cpu_getiloops() == 0)
	{
		if (intenable & 0x04) cpu_set_irq_line(0, 0, HOLD_LINE);
	}
	else if (cpu_getiloops() % 2)
	{
		if (intenable & 0x08) cpu_set_irq_line(0, IRQ_LINE_NMI, PULSE_LINE);
	}
}



static MEMORY_READ_START( readmem )
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0x87ff, MRA_RAM },
	{ 0x9000, 0x97ff, MRA_RAM },
	{ 0xa800, 0xa800, input_port_0_r },
	{ 0xa880, 0xa880, input_port_1_r },
	{ 0xa900, 0xa900, input_port_2_r },
	{ 0xa980, 0xa980, input_port_3_r },
MEMORY_END

static MEMORY_WRITE_START( writemem )
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0x8000, 0x83ff, pingpong_colorram_w, &colorram },
	{ 0x8400, 0x87ff, pingpong_videoram_w, &videoram },
	{ 0x9000, 0x9002, MWA_RAM },
	{ 0x9003, 0x9052, MWA_RAM, &spriteram, &spriteram_size },
	{ 0x9053, 0x97ff, MWA_RAM },
	{ 0xa000, 0xa000, coin_w },	/* coin counters + irq enables */
	{ 0xa200, 0xa200, MWA_NOP },		/* SN76496 data latch */
	{ 0xa400, 0xa400, SN76496_0_w },	/* trigger read */
	{ 0xa600, 0xa600, watchdog_reset_w },
MEMORY_END



INPUT_PORTS_START( pingpong )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_SERVICE( 0x04, IP_ACTIVE_LOW )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN1 )

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_2WAY | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT| IPF_2WAY | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_2WAY )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_2WAY )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON1 )

	PORT_START	/* DSW1 */
	PORT_DIPNAME( 0x0F, 0x0F, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x0A, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 3C_2C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 4C_3C ) )
	PORT_DIPSETTING(    0x0F, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x0C, DEF_STR( 3C_4C ) )
	PORT_DIPSETTING(    0x0E, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x07, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 2C_5C ) )
	PORT_DIPSETTING(    0x0B, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x0D, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0x05, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(    0x09, DEF_STR( 1C_7C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0xF0, 0xF0, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0xA0, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 3C_2C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 4C_3C ) )
	PORT_DIPSETTING(    0xF0, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0xC0, DEF_STR( 3C_4C ) )
	PORT_DIPSETTING(    0xE0, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x70, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x60, DEF_STR( 2C_5C ) )
	PORT_DIPSETTING(    0xB0, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0xD0, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0x50, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(    0x90, DEF_STR( 1C_7C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Free_Play ) )

	PORT_START	/* DSW2 */
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x06, 0x06, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x06, "Easy" )
	PORT_DIPSETTING(    0x02, "Normal" )
	PORT_DIPSETTING(    0x04, "Difficult" )
	PORT_DIPSETTING(    0x00, "Very Difficult" )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )
INPUT_PORTS_END



static struct GfxLayout charlayout =
{
	8,8,		/* 8*8 characters */
	512,		/* 512 characters */
	2,		/* 2 bits per pixel */
	{ 4, 0 },	/* the bitplanes are packed in one nibble */
	{ 3, 2, 1, 0, 8*8+3, 8*8+2, 8*8+1, 8*8+0 },	/* x bit */
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8   },     /* y bit */
	16*8	/* every char takes 16 consecutive bytes */
};

static struct GfxLayout spritelayout =
{
	16,16,		/* 16*16 sprites */
	128,		/* 128 sprites */
	2,		/* 2 bits per pixel */
	{ 4, 0 },	/* the bitplanes are packed in one nibble */
	{ 12*16+3,12*16+2,12*16+1,12*16+0,
	   8*16+3, 8*16+2, 8*16+1, 8*16+0,
	   4*16+3, 4*16+2, 4*16+1, 4*16+0,
	        3,      2,      1,      0 },			/* x bit */
	{  0*8,  1*8,  2*8,  3*8,  4*8,  5*8,  6*8,  7*8,
	  32*8, 33*8, 34*8, 35*8, 36*8, 37*8, 38*8, 39*8  },    /* y bit */
	64*8	/* every char takes 64 consecutive bytes */
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &charlayout,         0, 64 },
	{ REGION_GFX2, 0, &spritelayout,    64*4, 64 },
	{ -1 } /* end of array */
};



static struct SN76496interface sn76496_interface =
{
	1,			/* 1 chip */
	{ 18432000/8 },	/* 2.304 MHz */
	{ 100 }
};


static MACHINE_DRIVER_START( pingpong )

	/* basic machine hardware */
	MDRV_CPU_ADD(Z80,18432000/6)		/* 3.072 MHz (probably) */
	MDRV_CPU_MEMORY(readmem,writemem)
	MDRV_CPU_VBLANK_INT(pingpong_interrupt,16)	/* 1 IRQ + 8 NMI */

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 2*8, 30*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(32)
	MDRV_COLORTABLE_LENGTH(64*4+64*4)

	MDRV_PALETTE_INIT(pingpong)
	MDRV_VIDEO_START(pingpong)
	MDRV_VIDEO_UPDATE(pingpong)

	/* sound hardware */
	MDRV_SOUND_ADD(SN76496, sn76496_interface)
MACHINE_DRIVER_END



/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( pingpong )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "pp_e04.rom",   0x0000, 0x4000, CRC(18552f8f) SHA1(cb03659b5e8a68003e72182a20979384d829280f) )
	ROM_LOAD( "pp_e03.rom",   0x4000, 0x4000, CRC(ae5f01e8) SHA1(f0d6a2c64822f2662fed3f601e279db18246f894) )

	ROM_REGION( 0x2000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "pp_e01.rom",   0x0000, 0x2000, CRC(d1d6f090) SHA1(7b7d7cb90bed746dda871227463145263e4b0c5a) )

	ROM_REGION( 0x2000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "pp_e02.rom",   0x0000, 0x2000, CRC(33c687e0) SHA1(7c90de4d163d2ffad00c8cb6a194fa6125a4f4c1) )

	ROM_REGION( 0x0220, REGION_PROMS, 0 )
	ROM_LOAD( "pingpong.3j",  0x0000, 0x0020, CRC(3e04f06e) SHA1(a642c350f148e062d56eb2a2fc53c470603000e3) ) /* palette (this might be bad) */
	ROM_LOAD( "pingpong.11j", 0x0020, 0x0100, CRC(09d96b08) SHA1(81405e33eacc47f91ea4c7221d122f7e6f5b1e5d) ) /* sprites */
	ROM_LOAD( "pingpong.5h",  0x0120, 0x0100, CRC(8456046a) SHA1(8226f1325c14eb8aed5cd3c3d6bad9f9fd88c5fa) ) /* characters */
ROM_END



GAME( 1985, pingpong, 0, pingpong, pingpong, 0, ROT0, "Konami", "Ping Pong" )
