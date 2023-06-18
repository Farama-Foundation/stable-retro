/***************************************************************************

	Atari Cloud 9 (prototype) hardware

	driver by Mike Balfour

	Games supported:
		* Cloud 9

	Known issues:
		* none at this time

****************************************************************************

	Cloud9 (prototype) driver.

	This hardware is yet another variant of the Centipede/Millipede hardware,
	but as you can see there are some significant deviations...

	0000			R/W 	X index into the bitmap
	0001			R/W 	Y index into the bitmap
	0002			R/W 	Current bitmap pixel value
	0003-05FF 	R/W 	RAM
	0600-3FFF 	R/W 	Bitmap RAM bank 0 (and bank 1 ?)
	5000-5073 	R/W 	Motion Object RAM
	5400			W		Watchdog
	5480			W		IRQ Acknowledge
	5500-557F 	W		Color RAM (9 bits, 4 banks, LSB of Blue is addr&$40)

	5580			W		Auto-increment X bitmap index (~D7)
	5581			W		Auto-increment Y bitmap index (~D7)
	5584			W		VRAM Both Banks - (D7) seems to allow writing to both banks
	5585			W		Invert screen?
	5586			W		VRAM Bank select?
	5587			W		Color bank select

	5600			W		Coin Counter 1 (D7)
	5601			W		Coin Counter 2 (D7)
	5602			W		Start1 LED (~D7)
	5603			W		Start2 LED (~D7)

	5680			W		Force Write to EAROM?
	5700			W		EAROM Off?
	5780			W		EAROM On?

	5800			R		IN0 (D7=Vblank, D6=Right Coin, D5=Left Coin, D4=Aux, D3=Self Test)
	5801			R		IN1 (D7=Start1, D6=Start2, D5=Fire, D4=Zap)
	5900			R		Trackball Vert
	5901			R		Trackball Horiz

	5A00-5A0F 	R/W 	Pokey 1
	5B00-5B0F 	R/W 	Pokey 2
	5C00-5CFF 	W		EAROM
	6000-FFFF 	R		Program ROM

	If you have any questions about how this driver works, don't hesitate to
	ask.  - Mike Balfour (mab22@po.cwru.edu)

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "cloud9.h"


/*************************************
 *
 *	Output ports
 *
 *************************************/

static WRITE_HANDLER( cloud9_led_w )
{
	set_led_status(offset,~data & 0x80);
}


static WRITE_HANDLER( cloud9_coin_counter_w )
{
	coin_counter_w(offset,data);
}



/*************************************
 *
 *	Main CPU memory handlers
 *
 *************************************/

static MEMORY_READ_START( readmem )
	{ 0x0000, 0x0002, cloud9_bitmap_regs_r },
	{ 0x0003, 0x05ff, MRA_RAM },
	{ 0x0600, 0x3fff, MRA_RAM },
	{ 0x5500, 0x557f, MRA_RAM },
	{ 0x5800, 0x5800, input_port_0_r },
	{ 0x5801, 0x5801, input_port_1_r },
	{ 0x5900, 0x5900, input_port_2_r },
	{ 0x5901, 0x5901, input_port_3_r },
	{ 0x5a00, 0x5a0f, pokey1_r },
	{ 0x5b00, 0x5b0f, pokey2_r },
	{ 0x5c00, 0x5cff, MRA_RAM },	/* EAROM */
	{ 0x6000, 0xffff, MRA_ROM },
MEMORY_END


static MEMORY_WRITE_START( writemem )
	{ 0x0000, 0x0002, cloud9_bitmap_regs_w, &cloud9_bitmap_regs },
	{ 0x0003, 0x05ff, MWA_RAM },
	{ 0x0600, 0x3fff, cloud9_bitmap_w, &videoram, &videoram_size },
	{ 0x5000, 0x50ff, MWA_RAM, &spriteram },
	{ 0x5400, 0x5400, watchdog_reset_w },
	{ 0x5480, 0x5480, MWA_NOP },	/* IRQ Ack */
	{ 0x5500, 0x557f, cloud9_paletteram_w, &paletteram },
	{ 0x5580, 0x5580, MWA_RAM, &cloud9_auto_inc_x },
	{ 0x5581, 0x5581, MWA_RAM, &cloud9_auto_inc_y },
	{ 0x5584, 0x5584, MWA_RAM, &cloud9_both_banks },
	{ 0x5586, 0x5586, MWA_RAM, &cloud9_vram_bank },
	{ 0x5587, 0x5587, MWA_RAM, &cloud9_color_bank },
	{ 0x5600, 0x5601, cloud9_coin_counter_w },
	{ 0x5602, 0x5603, cloud9_led_w },
	{ 0x5a00, 0x5a0f, pokey1_w },
	{ 0x5b00, 0x5b0f, pokey2_w },
	{ 0x5c00, 0x5cff, MWA_RAM, &generic_nvram, &generic_nvram_size },
	{ 0x6000, 0xffff, MWA_ROM },
MEMORY_END



/*************************************
 *
 *	Port definitions
 *
 *************************************/

INPUT_PORTS_START( cloud9 )
	PORT_START	/* IN0 */
	PORT_BIT ( 0x07, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_SERVICE( 0x08, IP_ACTIVE_LOW )
	PORT_BIT ( 0x10, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT ( 0x20, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT ( 0x40, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT ( 0x80, IP_ACTIVE_LOW, IPT_VBLANK )

	PORT_START		/* IN1 */
	PORT_BIT ( 0x0F, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT ( 0x10, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT ( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT ( 0x40, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT ( 0x80, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START		/* IN2 */
	PORT_ANALOG( 0xff, 0x7f, IPT_TRACKBALL_Y | IPF_REVERSE, 30, 30, 0, 0 )

	PORT_START		/* IN3 */
	PORT_ANALOG( 0xff, 0x7f, IPT_TRACKBALL_X, 30, 30, 0, 0 )

	PORT_START	/* IN4 */ /* DSW1 */
	PORT_BIT ( 0xFF, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START	/* IN5 */ /* DSW2 */
	PORT_BIT ( 0x01, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_DIPNAME( 0x06, 0x04, DEF_STR( Coinage ) )
	PORT_DIPSETTING (	0x06, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING (	0x04, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING (	0x02, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING (	0x00, DEF_STR( Free_Play ) )
	PORT_DIPNAME(0x18,	0x00, "Right Coin" )
	PORT_DIPSETTING (	0x00, "*1" )
	PORT_DIPSETTING (	0x08, "*4" )
	PORT_DIPSETTING (	0x10, "*5" )
	PORT_DIPSETTING (	0x18, "*6" )
	PORT_DIPNAME(0x20,	0x00, "Middle Coin" )
	PORT_DIPSETTING (	0x00, "*1" )
	PORT_DIPSETTING (	0x20, "*2" )
	PORT_DIPNAME(0xC0,	0x00, "Bonus Coins" )
	PORT_DIPSETTING (	0xC0, "4 coins + 2 coins" )
	PORT_DIPSETTING (	0x80, "4 coins + 1 coin" )
	PORT_DIPSETTING (	0x40, "2 coins + 1 coin" )
	PORT_DIPSETTING (	0x00, "None" )
INPUT_PORTS_END



/*************************************
 *
 *	Graphics definitions
 *
 *************************************/

static struct GfxLayout charlayout =
{
	8,8,
	128,
	4,
	{ 0x3000*8, 0x2000*8, 0x1000*8, 0 },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	16*8
};


static struct GfxLayout spritelayout =
{
	16,16,
	64,
	4,
	{ 0x3000*8, 0x2000*8, 0x1000*8, 0x0000*8 },
	{ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 },
	{ 0*8, 2*8, 4*8, 6*8, 8*8, 10*8, 12*8, 14*8,
			16*8, 18*8, 20*8, 22*8, 24*8, 26*8, 28*8, 30*8 },
	32*8
};


static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0x0800, &charlayout,   0, 4 },
	{ REGION_GFX1, 0x0808, &charlayout,   0, 4 },
	{ REGION_GFX1, 0x0000, &spritelayout, 0, 4 },
	{ -1 }
};



/*************************************
 *
 *	Sound interfaces
 *
 *************************************/

static struct POKEYinterface pokey_interface =
{
	2,	/* 2 chips */
	1500000,	/* 1.5 MHz??? */
	{ 50, 50 },
	/* The 8 pot handlers */
	{ 0, 0 },
	{ 0, 0 },
	{ 0, 0 },
	{ 0, 0 },
	{ 0, 0 },
	{ 0, 0 },
	{ 0, 0 },
	{ 0, 0 },
	/* The allpot handler */
	{ input_port_4_r, input_port_5_r },
};



/*************************************
 *
 *	Machine driver
 *
 *************************************/

static MACHINE_DRIVER_START( cloud9 )

	/* basic machine hardware */
	MDRV_CPU_ADD(M6502,12096000/8)	/* 1.512 MHz?? */
	MDRV_CPU_MEMORY(readmem,writemem)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,4)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(1460)

	MDRV_NVRAM_HANDLER(generic_0fill)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 2*8, 32*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(64)

	MDRV_VIDEO_START(cloud9)
	MDRV_VIDEO_UPDATE(cloud9)

	/* sound hardware */
	MDRV_SOUND_ADD(POKEY, pokey_interface)
MACHINE_DRIVER_END

/*************************************
 *
 *	ROM definitions
 *
 *************************************/

ROM_START( cloud9 )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "c9_6000.bin", 0x6000, 0x2000, CRC(b5d95d98) SHA1(9a347e5fc6e9e753e5c6972341725b5f4412e451) )
	ROM_LOAD( "c9_8000.bin", 0x8000, 0x2000, CRC(49af8f22) SHA1(c118372bec0c428c2b60d29df95f358b302d5e66) )
	ROM_LOAD( "c9_a000.bin", 0xa000, 0x2000, CRC(7cf404a6) SHA1(d20b662102f8426af51b1ca4ed8e18b00d711365) )
	ROM_LOAD( "c9_c000.bin", 0xc000, 0x2000, CRC(26a4d7df) SHA1(8eef0a5f5d1ff13eec75d0c50f5a5dea28486ae5) )
	ROM_LOAD( "c9_e000.bin", 0xe000, 0x2000, CRC(6e663bce) SHA1(4f4a5dc57ba6bc38a17973a6644849f6f5a2dfd1) )

	ROM_REGION( 0x4000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "c9_gfx0.bin", 0x0000, 0x1000, CRC(d01a8019) SHA1(a77d6125b116ab4bf9446e3b99469dad2719f7e5) )
	ROM_LOAD( "c9_gfx1.bin", 0x1000, 0x1000, CRC(514ac009) SHA1(f05081d8da47e650b0bd12cd00460c98a4f745b1) )
	ROM_LOAD( "c9_gfx2.bin", 0x2000, 0x1000, CRC(930c1ade) SHA1(ba22cb7b105da2ab8c40574e70f18d594d833452) )
	ROM_LOAD( "c9_gfx3.bin", 0x3000, 0x1000, CRC(27e9b88d) SHA1(a1d27e62eea9cdff662a3c160f650bbdb32b7f47) )
ROM_END



/*************************************
 *
 *	Game drivers
 *
 *************************************/

GAMEX( 1983, cloud9, 0, cloud9, cloud9, 0, ROT0, "Atari", "Cloud 9 (prototype)", GAME_NO_COCKTAIL )
