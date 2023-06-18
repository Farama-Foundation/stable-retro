/***************************************************************************

PacLand memory map (preliminary)

Ernesto Corvi
ernesto@imagina.com

Main CPU:
0000-1fff Character ram ( video ram ) + colorram
2000-3000 spriteram?
3000-3800 RAM
3800-3801 Background 1 scrolling
3a00-3a01 Background 2 scrolling
3c00-3c00 Bank and ROM Selector
4000-6000 Banked ROMs
6800-6bff Shared RAM with the MCU
8000-ffff ROM

MCU:
0000-0027 Internal registers, timers and ports.
0040-13ff RAM
1000-13ff Shared RAM with the Main CPU
8000-a000 MCU external ROM
c000-cfff Namco Sound ?
d000-d003 Dip Switches/Joysticks
f000-ffff MCU internal ROM

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "cpu/m6800/m6800.h"


static UINT8 *sharedram1;

extern UINT8 *pacland_videoram2;

extern WRITE_HANDLER( pacland_videoram_w );
extern WRITE_HANDLER( pacland_videoram2_w );
extern WRITE_HANDLER( pacland_scroll0_w );
extern WRITE_HANDLER( pacland_scroll1_w );
extern WRITE_HANDLER( pacland_bankswitch_w );
extern WRITE_HANDLER( pacland_flipscreen_w );

extern PALETTE_INIT( pacland );
extern VIDEO_START( pacland );
extern VIDEO_UPDATE( pacland );


static READ_HANDLER( sharedram1_r )
{
	return sharedram1[offset];
}

static WRITE_HANDLER( sharedram1_w )
{
	sharedram1[offset] = data;
}

static WRITE_HANDLER( pacland_halt_mcu_w )
{
	if (offset == 0)
		cpu_set_reset_line(1,CLEAR_LINE);
	else
		cpu_set_reset_line(1,ASSERT_LINE);
}


/* Stubs to pass the correct Dip Switch setup to the MCU */
static READ_HANDLER( dsw0_r )
{
	/* Hi 4 bits = DSWA Hi 4 bits */
	/* Lo 4 bits = DSWB Hi 4 bits */
	int r = readinputport( 0 );
	r &= 0xf0;
	r |= ( readinputport( 1 ) >> 4 ) & 0x0f;
	return ~r; /* Active Low */
}

static READ_HANDLER( dsw1_r )
{
	/* Hi 4 bits = DSWA Lo 4 bits */
	/* Lo 4 bits = DSWB Lo 4 bits */
	int r = ( readinputport( 0 ) & 0x0f ) << 4;
	r |= readinputport( 1 ) & 0x0f;
	return ~r; /* Active Low */
}

static WRITE_HANDLER( pacland_coin_w )
{
	coin_lockout_global_w(data & 1);
	coin_counter_w(0,~data & 2);
	coin_counter_w(1,~data & 4);
}

static WRITE_HANDLER( pacland_led_w )
{
	set_led_status(0,data & 0x08);
	set_led_status(1,data & 0x10);
}


static MEMORY_READ_START( readmem )
	{ 0x0000, 0x1fff, MRA_RAM },
	{ 0x2000, 0x37ff, MRA_RAM },
	{ 0x4000, 0x5fff, MRA_BANK1 },
	{ 0x6800, 0x68ff, namcos1_wavedata_r },		/* PSG device, shared RAM */
	{ 0x6800, 0x6bff, sharedram1_r },
	{ 0x7800, 0x7800, MRA_NOP },	/* ??? */
	{ 0x8000, 0xffff, MRA_ROM },
MEMORY_END

static MEMORY_WRITE_START( writemem )
	{ 0x0000, 0x0fff, pacland_videoram_w, &videoram },
	{ 0x1000, 0x1fff, pacland_videoram2_w, &pacland_videoram2 },
	{ 0x2000, 0x37ff, MWA_RAM },
	{ 0x2700, 0x27ff, MWA_RAM, &spriteram, &spriteram_size },
	{ 0x2f00, 0x2fff, MWA_RAM, &spriteram_2 },
	{ 0x3700, 0x37ff, MWA_RAM, &spriteram_3 },
	{ 0x3800, 0x3801, pacland_scroll0_w },
	{ 0x3a00, 0x3a01, pacland_scroll1_w },
	{ 0x3c00, 0x3c00, pacland_bankswitch_w },
	{ 0x4000, 0x5fff, MWA_ROM },
	{ 0x6800, 0x68ff, namcos1_wavedata_w }, /* PSG device, shared RAM */
	{ 0x6800, 0x6bff, sharedram1_w, &sharedram1 },
	{ 0x7000, 0x7000, MWA_NOP },	/* ??? */
	{ 0x7800, 0x7800, MWA_NOP },	/* ??? */
	{ 0x8000, 0x8800, pacland_halt_mcu_w },
	{ 0x9000, 0x9800, pacland_flipscreen_w },
	/*{ 0x8000, 0xffff, MWA_ROM },*/
MEMORY_END

static MEMORY_READ_START( mcu_readmem )
	{ 0x0000, 0x001f, hd63701_internal_registers_r },
	{ 0x0080, 0x00ff, MRA_RAM },
	{ 0x1000, 0x10ff, namcos1_wavedata_r },			/* PSG device, shared RAM */
	{ 0x1100, 0x113f, MRA_RAM }, /* PSG device */
	{ 0x1000, 0x13ff, sharedram1_r },
	{ 0x8000, 0x9fff, MRA_ROM },
	{ 0xc000, 0xc800, MRA_RAM },
	{ 0xd000, 0xd000, dsw0_r },
	{ 0xd000, 0xd001, dsw1_r },
	{ 0xd000, 0xd002, input_port_2_r },
	{ 0xd000, 0xd003, input_port_3_r },
	{ 0xf000, 0xffff, MRA_ROM },
MEMORY_END

static MEMORY_WRITE_START( mcu_writemem )
	{ 0x0000, 0x001f, hd63701_internal_registers_w },
	{ 0x0080, 0x00ff, MWA_RAM },
	{ 0x1000, 0x10ff, namcos1_wavedata_w, &namco_wavedata },		/* PSG device, shared RAM */
	{ 0x1100, 0x113f, namcos1_sound_w, &namco_soundregs }, /* PSG device */
	{ 0x1000, 0x13ff, sharedram1_w },
	{ 0x2000, 0x2000, MWA_NOP }, /* ???? (w)*/
	{ 0x4000, 0x4000, MWA_NOP }, /* ???? (w)*/
	{ 0x6000, 0x6000, MWA_NOP }, /* ???? (w)*/
	{ 0x8000, 0x9fff, MWA_ROM },
	{ 0xc000, 0xc7ff, MWA_RAM },
	{ 0xf000, 0xffff, MWA_ROM },
MEMORY_END


static READ_HANDLER( readFF )
{
	return 0xff;
}

static PORT_READ_START( mcu_readport )
	{ HD63701_PORT1, HD63701_PORT1, input_port_4_r },
	{ HD63701_PORT2, HD63701_PORT2, readFF },	/* leds won't work otherwise */
PORT_END

static PORT_WRITE_START( mcu_writeport )
	{ HD63701_PORT1, HD63701_PORT1, pacland_coin_w },
	{ HD63701_PORT2, HD63701_PORT2, pacland_led_w },
PORT_END



INPUT_PORTS_START( pacland )
	PORT_START      /* DSWA */
	PORT_DIPNAME( 0x03, 0x00, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 1C_2C ) )
	PORT_DIPNAME( 0x04, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x18, 0x00, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x18, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 1C_2C ) )
	PORT_DIPNAME( 0x60, 0x00, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x20, "2" )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x40, "4" )
	PORT_DIPSETTING(    0x60, "5" )
	/*PORT_SERVICE( 0x80, IP_ACTIVE_LOW )*/

	PORT_START      /* DSWB */
	PORT_DIPNAME( 0x01, 0x00, "Trip Select" )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x00, "Freeze" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x02, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x00, "Round Select" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x04, DEF_STR( On ) )
	PORT_DIPNAME( 0x18, 0x00, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x00, "A" )
	PORT_DIPSETTING(    0x08, "B" )
	PORT_DIPSETTING(    0x10, "C" )
	PORT_DIPSETTING(    0x18, "D" )
	PORT_DIPNAME( 0xe0, 0x00, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x00, "30K 80K 130K 300K 500K 1M" )
	PORT_DIPSETTING(    0x20, "30K 100K 200K 400K 600K 1M" )
	PORT_DIPSETTING(    0x40, "40K 100K 180K 300K 500K 1M" )
	PORT_DIPSETTING(    0x60, "30K 80K 100K+" )
	PORT_DIPSETTING(    0x80, "50K 150K 200K+" )
	PORT_DIPSETTING(    0xa0, "30K 80K 150K" )
	PORT_DIPSETTING(    0xc0, "40K 100K 200K" )
	PORT_DIPSETTING(    0xe0, "40K" )

	PORT_START	/* Memory Mapped Port */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_UP )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START2 )
	PORT_SERVICE( 0x40, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Cocktail ) )

	PORT_START	/* Memory Mapped Port */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN2 )

	PORT_START	/* MCU Input Port */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_SPECIAL )	/* OUT:coin lockout */
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_SPECIAL )	/* OUT:coin counter 1 */
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SPECIAL )	/* OUT:coin counter 2 */
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_COCKTAIL )
INPUT_PORTS_END



static struct GfxLayout spritelayout =
{
	16,16,       	/* 16*16 sprites */
	256,           	/* 256 sprites */
	4,              /* 4 bits per pixel */
	{ 0, 4, 16384*8, 16384*8+4 },
	{ 0, 1, 2, 3, 8*8, 8*8+1, 8*8+2, 8*8+3,
			16*8+0, 16*8+1, 16*8+2, 16*8+3, 24*8+0, 24*8+1, 24*8+2, 24*8+3 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
			32*8, 33*8, 34*8, 35*8, 36*8, 37*8, 38*8, 39*8 },
	64*8    /* every sprite takes 256 bytes */
};

static struct GfxLayout charlayout =
{
	8,8,	/* 8*8 characters */
	512,	/* 512 characters */
	2,	/* 2 bits per pixel */
	{ 0, 4 },	/* the bitplanes are packed in the same byte */
	{ 8*8, 8*8+1, 8*8+2, 8*8+3, 0, 1, 2, 3 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	16*8	/* every char takes 16 consecutive bytes */
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &charlayout,              0, 256 },
	{ REGION_GFX2, 0, &charlayout,          256*4, 256 },
	{ REGION_GFX3, 0, &spritelayout,  256*4+256*4, 3*64 },
	{ REGION_GFX4, 0, &spritelayout,  256*4+256*4, 3*64 },
	{ -1 } /* end of array */
};


static struct namco_interface namco_interface =
{
	24000,	/* sample rate */
	8,		/* number of voices */
	100,	/* playback volume */
	-1,		/* memory region */
	0		/* stereo */
};


static MACHINE_DRIVER_START( pacland )

	/* basic machine hardware */
	MDRV_CPU_ADD(M6809, 1500000)	/* 1.500 MHz (?) */
	MDRV_CPU_MEMORY(readmem,writemem)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,1)

	MDRV_CPU_ADD(HD63701, 6000000/3.9)	/* or compatible 6808 with extra instructions */
/*			6000000/4,		 // ??? /*/
	MDRV_CPU_MEMORY(mcu_readmem,mcu_writemem)
	MDRV_CPU_PORTS(mcu_readport,mcu_writeport)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,1)

	MDRV_FRAMES_PER_SECOND(60.606060)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(100)	/* we need heavy synching between the MCU and the CPU */

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_UPDATE_AFTER_VBLANK)
	MDRV_SCREEN_SIZE(42*8, 32*8)
	MDRV_VISIBLE_AREA(3*8, 39*8-1, 2*8, 30*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(256)
	MDRV_COLORTABLE_LENGTH(256*4+256*4+3*64*16)

	MDRV_PALETTE_INIT(pacland)
	MDRV_VIDEO_START(pacland)
	MDRV_VIDEO_UPDATE(pacland)

	/* sound hardware */
	MDRV_SOUND_ADD(NAMCO_15XX, namco_interface)
MACHINE_DRIVER_END


/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( pacland )
	ROM_REGION( 0x20000, REGION_CPU1, 0 )	/* 128k for code */
	ROM_LOAD( "pl5_01b.bin",  0x08000, 0x4000, CRC(b0ea7631) SHA1(424afa6f397310c7af39c9e8b580aa9ccd42c39c) )
	ROM_LOAD( "pl5_02.bin",   0x0C000, 0x4000, CRC(d903e84e) SHA1(25338726227bfbec65847879aac5228a6a307db4) )
	/* all the following are banked at 0x4000-0x5fff */
	ROM_LOAD( "pl1-3",        0x10000, 0x4000, CRC(aa9fa739) SHA1(7b1f7857eb5f68e166b1f8988c82051aaf05df48) )
	ROM_LOAD( "pl1-4",        0x14000, 0x4000, CRC(2b895a90) SHA1(820f8873c6a5a736089406d0f03d491dfb82d00d) )
	ROM_LOAD( "pl1-5",        0x18000, 0x4000, CRC(7af66200) SHA1(f44161ded1633e9801b7a9cd84d481e53823f5d9) )
	ROM_LOAD( "pl3_06.bin",   0x1c000, 0x4000, CRC(2ffe3319) SHA1(c2540321cd5a1fe29ecb077abdf8f997893192e9) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for code */
	ROM_LOAD( "pl1-7",        0x8000, 0x2000, CRC(8c5becae) SHA1(14d67136395c4c64472980a69648ce2d479ae67f) ) /* sub program for the mcu */
	ROM_LOAD( "pl1-mcu.bin",  0xf000, 0x1000, CRC(6ef08fb3) SHA1(4842590d60035a0059b0899eb2d5f58ae72c2529) ) /* microcontroller */

	ROM_REGION( 0x02000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "pl2_12.bin",   0x00000, 0x2000, CRC(a63c8726) SHA1(b15903fa2267375280af03af0a7157e1b0bcb86d) )	/* chars */

	ROM_REGION( 0x02000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "pl4_13.bin",   0x00000, 0x2000, CRC(3ae582fd) SHA1(696b2cfadb6b071de8e43d20cd65b37713ca3b30) )

	ROM_REGION( 0x08000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "pl1-9",        0x00000, 0x4000, CRC(f5d5962b) SHA1(8d008a9bc06dc562c241955d9c551647b5c1f4e9) )	/* sprites */
	ROM_LOAD( "pl1-10",       0x04000, 0x4000, CRC(c7cf1904) SHA1(7ca8ed20ee32eb8609ac96b4e4fcb3b6027b598a) )

	ROM_REGION( 0x08000, REGION_GFX4, ROMREGION_DISPOSE )
	ROM_LOAD( "pl1-8",        0x00000, 0x4000, CRC(a2ebfa4a) SHA1(4a2a2b43a23a7a46266751415d1bde118143429c) )
	ROM_LOAD( "pl1-11",       0x04000, 0x4000, CRC(6621361a) SHA1(4efa40adba803006e86d5e12514983d4132b5efb) )

	ROM_REGION( 0x1400, REGION_PROMS, 0 )
	ROM_LOAD( "pl1-2.bin",    0x0000, 0x0400, CRC(472885de) SHA1(8d552c90b8d5bc6ad6c60934c00f4303cd180ce7) )	/* red and green component */
	ROM_LOAD( "pl1-1.bin",    0x0400, 0x0400, CRC(a78ebdaf) SHA1(8ea215701eb5e1a2a329ef92c19fc69b18fc28c7) )	/* blue component */
	ROM_LOAD( "pl1-3.bin",    0x0800, 0x0400, CRC(80558da8) SHA1(7e1483467817295f36d1e2bdb32934c4f2617d52) )	/* sprites lookup table */
	ROM_LOAD( "pl1-5.bin",    0x0c00, 0x0400, CRC(4b7ee712) SHA1(dd0ec4c632d8b160f7b54d8f18fcf4ef1508d832) )	/* foreground lookup table */
	ROM_LOAD( "pl1-4.bin",    0x1000, 0x0400, CRC(3a7be418) SHA1(475cdc68205e3acce83fe79b00b74c6a7e28dde4) )	/* background lookup table */
ROM_END

ROM_START( pacland2 )
	ROM_REGION( 0x20000, REGION_CPU1, 0 )	/* 128k for code */
	ROM_LOAD( "pl6_01.bin",   0x08000, 0x4000, CRC(4c96e11c) SHA1(c136dc3877155b7a600173c876f6a53394d9260d) )
	ROM_LOAD( "pl6_02.bin",   0x0C000, 0x4000, CRC(8cf5bd8d) SHA1(0771ca1ab5db58f5632583a5e6e84660e8ab727d) )
	/* all the following are banked at 0x4000-0x5fff */
	ROM_LOAD( "pl1-3",        0x10000, 0x4000, CRC(aa9fa739) SHA1(7b1f7857eb5f68e166b1f8988c82051aaf05df48) )
	ROM_LOAD( "pl1-4",        0x14000, 0x4000, CRC(2b895a90) SHA1(820f8873c6a5a736089406d0f03d491dfb82d00d) )
	ROM_LOAD( "pl1-5",        0x18000, 0x4000, CRC(7af66200) SHA1(f44161ded1633e9801b7a9cd84d481e53823f5d9) )
	ROM_LOAD( "pl1-6",        0x1c000, 0x4000, CRC(b01e59a9) SHA1(e5b093852d33a4d09969d111fa6e42e964aa4dac) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for code */
	ROM_LOAD( "pl1-7",        0x8000, 0x2000, CRC(8c5becae) SHA1(14d67136395c4c64472980a69648ce2d479ae67f) ) /* sub program for the mcu */
	ROM_LOAD( "pl1-mcu.bin",  0xf000, 0x1000, CRC(6ef08fb3) SHA1(4842590d60035a0059b0899eb2d5f58ae72c2529) ) /* microcontroller */

	ROM_REGION( 0x02000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "pl0_12.bin",   0x00000, 0x2000, CRC(c8cb61ab) SHA1(ec33d64949a8c011430e889f55f54816b33c4218) )	/* chars */

	ROM_REGION( 0x02000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "pl1-13",       0x00000, 0x2000, CRC(6c5ed9ae) SHA1(db919c9254289179e98ba5d2ed8c66d67ae95f35) )

	ROM_REGION( 0x08000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "pl1_09b.bin",  0x00000, 0x4000, CRC(80768a87) SHA1(1572f309e810d9eb007a1c8b2aa8463027c146ca) )	/* sprites */
	ROM_LOAD( "pl1_10b.bin",  0x04000, 0x4000, CRC(ffd9d66e) SHA1(9a6e9ad500fcb7a67cb3c45d029c2aa7636a64f9) )

	ROM_REGION( 0x08000, REGION_GFX4, ROMREGION_DISPOSE )
	ROM_LOAD( "pl1_08.bin",   0x00000, 0x4000, CRC(2b20e46d) SHA1(9f78952ae94fef6a83a15de35d5fefdf71e78488) )
	ROM_LOAD( "pl1_11.bin",   0x04000, 0x4000, CRC(c59775d8) SHA1(034281c8101719d79043df31ef845fd28c0c69c0) )

	ROM_REGION( 0x1400, REGION_PROMS, 0 )
	ROM_LOAD( "pl1-2.bin",    0x0000, 0x0400, CRC(472885de) SHA1(8d552c90b8d5bc6ad6c60934c00f4303cd180ce7) )	/* red and green component */
	ROM_LOAD( "pl1-1.bin",    0x0400, 0x0400, CRC(a78ebdaf) SHA1(8ea215701eb5e1a2a329ef92c19fc69b18fc28c7) )	/* blue component */
	ROM_LOAD( "pl1-3.bin",    0x0800, 0x0400, CRC(80558da8) SHA1(7e1483467817295f36d1e2bdb32934c4f2617d52) )	/* sprites lookup table */
	ROM_LOAD( "pl1-5.bin",    0x0c00, 0x0400, CRC(4b7ee712) SHA1(dd0ec4c632d8b160f7b54d8f18fcf4ef1508d832) )	/* foreground lookup table */
	ROM_LOAD( "pl1-4.bin",    0x1000, 0x0400, CRC(3a7be418) SHA1(475cdc68205e3acce83fe79b00b74c6a7e28dde4) )	/* background lookup table */
ROM_END

ROM_START( pacland3 )
	ROM_REGION( 0x20000, REGION_CPU1, 0 )	/* 128k for code */
	ROM_LOAD( "pln1-1",       0x08000, 0x4000, CRC(f729fb94) SHA1(332ff2e4aae67eb8ed0f52048097f74323a176f8) )
	ROM_LOAD( "pln1-2",       0x0C000, 0x4000, CRC(5c66eb6f) SHA1(376233f51e655df8922886c1e808a2f37ccae5d4) )
	/* all the following are banked at 0x4000-0x5fff */
	ROM_LOAD( "pl1-3",        0x10000, 0x4000, CRC(aa9fa739) SHA1(7b1f7857eb5f68e166b1f8988c82051aaf05df48) )
	ROM_LOAD( "pl1-4",        0x14000, 0x4000, CRC(2b895a90) SHA1(820f8873c6a5a736089406d0f03d491dfb82d00d) )
	ROM_LOAD( "pl1-5",        0x18000, 0x4000, CRC(7af66200) SHA1(f44161ded1633e9801b7a9cd84d481e53823f5d9) )
	ROM_LOAD( "pl1-6",        0x1c000, 0x4000, CRC(b01e59a9) SHA1(e5b093852d33a4d09969d111fa6e42e964aa4dac) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for code */
	ROM_LOAD( "pl1-7",        0x8000, 0x2000, CRC(8c5becae) SHA1(14d67136395c4c64472980a69648ce2d479ae67f) ) /* sub program for the mcu */
	ROM_LOAD( "pl1-mcu.bin",  0xf000, 0x1000, CRC(6ef08fb3) SHA1(4842590d60035a0059b0899eb2d5f58ae72c2529) ) /* microcontroller */

	ROM_REGION( 0x02000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "pl1-12",       0x00000, 0x2000, CRC(c159fbce) SHA1(b0326c85b7df407f3e94c38a5971f911968d7b27) )	/* chars */

	ROM_REGION( 0x02000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "pl1-13",       0x00000, 0x2000, CRC(6c5ed9ae) SHA1(db919c9254289179e98ba5d2ed8c66d67ae95f35) )

	ROM_REGION( 0x08000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "pl1_09b.bin",  0x00000, 0x4000, CRC(80768a87) SHA1(1572f309e810d9eb007a1c8b2aa8463027c146ca) )	/* sprites */
	ROM_LOAD( "pl1_10b.bin",  0x04000, 0x4000, CRC(ffd9d66e) SHA1(9a6e9ad500fcb7a67cb3c45d029c2aa7636a64f9) )

	ROM_REGION( 0x08000, REGION_GFX4, ROMREGION_DISPOSE )
	ROM_LOAD( "pl1_08.bin",   0x00000, 0x4000, CRC(2b20e46d) SHA1(9f78952ae94fef6a83a15de35d5fefdf71e78488) )
	ROM_LOAD( "pl1_11.bin",   0x04000, 0x4000, CRC(c59775d8) SHA1(034281c8101719d79043df31ef845fd28c0c69c0) )

	ROM_REGION( 0x1400, REGION_PROMS, 0 )
	ROM_LOAD( "pl1-2.bin",    0x0000, 0x0400, CRC(472885de) SHA1(8d552c90b8d5bc6ad6c60934c00f4303cd180ce7) )	/* red and green component */
	ROM_LOAD( "pl1-1.bin",    0x0400, 0x0400, CRC(a78ebdaf) SHA1(8ea215701eb5e1a2a329ef92c19fc69b18fc28c7) )	/* blue component */
	ROM_LOAD( "pl1-3.bin",    0x0800, 0x0400, CRC(80558da8) SHA1(7e1483467817295f36d1e2bdb32934c4f2617d52) )	/* sprites lookup table */
	ROM_LOAD( "pl1-5.bin",    0x0c00, 0x0400, CRC(4b7ee712) SHA1(dd0ec4c632d8b160f7b54d8f18fcf4ef1508d832) )	/* foreground lookup table */
	ROM_LOAD( "pl1-4.bin",    0x1000, 0x0400, CRC(3a7be418) SHA1(475cdc68205e3acce83fe79b00b74c6a7e28dde4) )	/* background lookup table */
ROM_END

ROM_START( paclandm )
	ROM_REGION( 0x20000, REGION_CPU1, 0 )	/* 128k for code */
	ROM_LOAD( "pl1-1",        0x08000, 0x4000, CRC(a938ae99) SHA1(bf12097d8c69685cb7af763f9b9617c767aaed2f) )
	ROM_LOAD( "pl1-2",        0x0C000, 0x4000, CRC(3fe43bb5) SHA1(14e6144d06ff2fd786f383f36f1b8238ac364849) )
	/* all the following are banked at 0x4000-0x5fff */
	ROM_LOAD( "pl1-3",        0x10000, 0x4000, CRC(aa9fa739) SHA1(7b1f7857eb5f68e166b1f8988c82051aaf05df48) )
	ROM_LOAD( "pl1-4",        0x14000, 0x4000, CRC(2b895a90) SHA1(820f8873c6a5a736089406d0f03d491dfb82d00d) )
	ROM_LOAD( "pl1-5",        0x18000, 0x4000, CRC(7af66200) SHA1(f44161ded1633e9801b7a9cd84d481e53823f5d9) )
	ROM_LOAD( "pl1-6",        0x1c000, 0x4000, CRC(b01e59a9) SHA1(e5b093852d33a4d09969d111fa6e42e964aa4dac) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for code */
	ROM_LOAD( "pl1-7",        0x8000, 0x2000, CRC(8c5becae) SHA1(14d67136395c4c64472980a69648ce2d479ae67f) ) /* sub program for the mcu */
	ROM_LOAD( "pl1-mcu.bin",  0xf000, 0x1000, CRC(6ef08fb3) SHA1(4842590d60035a0059b0899eb2d5f58ae72c2529) ) /* microcontroller */

	ROM_REGION( 0x02000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "pl1-12",       0x00000, 0x2000, CRC(c159fbce) SHA1(b0326c85b7df407f3e94c38a5971f911968d7b27) )	/* chars */

	ROM_REGION( 0x02000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "pl1-13",       0x00000, 0x2000, CRC(6c5ed9ae) SHA1(db919c9254289179e98ba5d2ed8c66d67ae95f35) )

	ROM_REGION( 0x08000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "pl1-9",        0x00000, 0x4000, CRC(f5d5962b) SHA1(8d008a9bc06dc562c241955d9c551647b5c1f4e9) )	/* sprites */
	ROM_LOAD( "pl1-10",       0x04000, 0x4000, CRC(c7cf1904) SHA1(7ca8ed20ee32eb8609ac96b4e4fcb3b6027b598a) )

	ROM_REGION( 0x08000, REGION_GFX4, ROMREGION_DISPOSE )
	ROM_LOAD( "pl1-8",        0x00000, 0x4000, CRC(a2ebfa4a) SHA1(4a2a2b43a23a7a46266751415d1bde118143429c) )
	ROM_LOAD( "pl1-11",       0x04000, 0x4000, CRC(6621361a) SHA1(4efa40adba803006e86d5e12514983d4132b5efb) )

	ROM_REGION( 0x1400, REGION_PROMS, 0 )
	ROM_LOAD( "pl1-2.bin",    0x0000, 0x0400, CRC(472885de) SHA1(8d552c90b8d5bc6ad6c60934c00f4303cd180ce7) )	/* red and green component */
	ROM_LOAD( "pl1-1.bin",    0x0400, 0x0400, CRC(a78ebdaf) SHA1(8ea215701eb5e1a2a329ef92c19fc69b18fc28c7) )	/* blue component */
	ROM_LOAD( "pl1-3.bin",    0x0800, 0x0400, CRC(80558da8) SHA1(7e1483467817295f36d1e2bdb32934c4f2617d52) )	/* sprites lookup table */
	ROM_LOAD( "pl1-5.bin",    0x0c00, 0x0400, CRC(4b7ee712) SHA1(dd0ec4c632d8b160f7b54d8f18fcf4ef1508d832) )	/* foreground lookup table */
	ROM_LOAD( "pl1-4.bin",    0x1000, 0x0400, CRC(3a7be418) SHA1(475cdc68205e3acce83fe79b00b74c6a7e28dde4) )	/* background lookup table */
ROM_END



GAME( 1984, pacland,  0,       pacland, pacland, 0, ROT0, "Namco", "Pac-Land (set 1)" )
GAME( 1984, pacland2, pacland, pacland, pacland, 0, ROT0, "Namco", "Pac-Land (set 2)" )
GAME( 1984, pacland3, pacland, pacland, pacland, 0, ROT0, "Namco", "Pac-Land (set 3)" )
GAME( 1984, paclandm, pacland, pacland, pacland, 0, ROT0, "[Namco] (Bally Midway license)", "Pac-Land (Midway)" )
