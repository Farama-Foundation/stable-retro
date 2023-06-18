/***************************************************************************

Mikie memory map (preliminary)

driver by Allard Van Der Bas


MAIN BOARD:
2800-288f Sprite RAM (288f, not 287f - quite unusual)
3800-3bff Color RAM
3c00-3fff Video RAM
4000-5fff ROM (?)
5ff0	  Watchdog (?)
6000-ffff ROM


***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "cpu/m6809/m6809.h"

extern WRITE_HANDLER( mikie_videoram_w );
extern WRITE_HANDLER( mikie_colorram_w );
extern WRITE_HANDLER( mikie_palettebank_w );
extern WRITE_HANDLER( mikie_flipscreen_w );

extern PALETTE_INIT( mikie );
extern VIDEO_START( mikie );
extern VIDEO_UPDATE( mikie );

static READ_HANDLER( mikie_sh_timer_r )
{
	int clock;

#define TIMER_RATE 512

	clock = activecpu_gettotalcycles() / TIMER_RATE;

	return clock;
}

static WRITE_HANDLER( mikie_sh_irqtrigger_w )
{
	static int last;


	if (last == 0 && data == 1)
	{
		/* setting bit 0 low then high triggers IRQ on the sound CPU */
		cpu_set_irq_line_and_vector(1,0,HOLD_LINE,0xff);
	}

	last = data;
}

static WRITE_HANDLER( mikie_coin_counter_w )
{
	coin_counter_w(offset,data);
}


static MEMORY_READ_START( readmem )
	{ 0x0000, 0x00ff, MRA_RAM },	/* ???? */
	{ 0x2400, 0x2400, input_port_0_r },	/* coins + selftest */
	{ 0x2401, 0x2401, input_port_1_r },	/* player 1 controls */
	{ 0x2402, 0x2402, input_port_2_r },	/* player 2 controls */
	{ 0x2403, 0x2403, input_port_3_r },	/* flip */
	{ 0x2500, 0x2500, input_port_4_r },	/* Dipswitch settings */
	{ 0x2501, 0x2501, input_port_5_r },	/* Dipswitch settings */
	{ 0x2800, 0x2fff, MRA_RAM },	/* RAM BANK 2 */
	{ 0x3000, 0x37ff, MRA_RAM },	/* RAM BANK 3 */
	{ 0x3800, 0x3fff, MRA_RAM },	/* video RAM */
	{ 0x4000, 0x5fff, MRA_ROM },    /* Machine checks for extra rom */
	{ 0x6000, 0xffff, MRA_ROM },
MEMORY_END

static MEMORY_WRITE_START( writemem )
	{ 0x2000, 0x2001, mikie_coin_counter_w },
	{ 0x2002, 0x2002, mikie_sh_irqtrigger_w },
	{ 0x2006, 0x2006, mikie_flipscreen_w },
	{ 0x2007, 0x2007, interrupt_enable_w },
	{ 0x2100, 0x2100, watchdog_reset_w },
	{ 0x2200, 0x2200, mikie_palettebank_w },
	{ 0x2400, 0x2400, soundlatch_w },
	{ 0x2800, 0x288f, MWA_RAM, &spriteram, &spriteram_size },
	{ 0x2890, 0x37ff, MWA_RAM },
	{ 0x3800, 0x3bff, mikie_colorram_w, &colorram },
	{ 0x3c00, 0x3fff, mikie_videoram_w, &videoram },
	{ 0x6000, 0xffff, MWA_ROM },
MEMORY_END

static MEMORY_READ_START( sound_readmem )
	{ 0x0000, 0x3fff, MRA_ROM },
	{ 0x4000, 0x43ff, MRA_RAM },
	{ 0x8003, 0x8003, soundlatch_r },
	{ 0x8005, 0x8005, mikie_sh_timer_r },
MEMORY_END

static MEMORY_WRITE_START( sound_writemem )
	{ 0x0000, 0x3fff, MWA_ROM },
	{ 0x4000, 0x43ff, MWA_RAM },
	{ 0x8000, 0x8000, MWA_NOP },	/* sound command latch */
	{ 0x8001, 0x8001, MWA_NOP },	/* ??? */
	{ 0x8002, 0x8002, SN76496_0_w },	/* trigger read of latch */
	{ 0x8004, 0x8004, SN76496_1_w },	/* trigger read of latch */
	{ 0x8079, 0x8079, MWA_NOP },	/* ??? */
/*	{ 0xa003, 0xa003, MWA_RAM },*/
MEMORY_END



INPUT_PORTS_START( mikie )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_4WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_4WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_4WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x01, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x00, "Controls" )
	PORT_DIPSETTING(    0x00, "Single" )
	PORT_DIPSETTING(    0x02, "Dual" )
	PORT_BIT( 0xfc, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START	/* DSW0 */
	PORT_DIPNAME( 0x0f, 0x0f, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x05, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 3C_2C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 4C_3C ) )
	PORT_DIPSETTING(    0x0f, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 3C_4C ) )
	PORT_DIPSETTING(    0x07, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x0e, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 2C_5C ) )
	PORT_DIPSETTING(    0x0d, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x0b, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0x0a, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(    0x09, DEF_STR( 1C_7C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0xf0, 0xf0, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x50, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 3C_2C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 4C_3C ) )
	PORT_DIPSETTING(    0xf0, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 3C_4C ) )
	PORT_DIPSETTING(    0x70, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0xe0, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x60, DEF_STR( 2C_5C ) )
	PORT_DIPSETTING(    0xd0, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0xb0, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0xa0, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(    0x90, DEF_STR( 1C_7C ) )
	PORT_DIPSETTING(    0x00, "Disabled" )

	PORT_START	/* DSW1 */
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x03, "3" )
	PORT_DIPSETTING(    0x02, "4" )
	PORT_DIPSETTING(    0x01, "5" )
	PORT_DIPSETTING(    0x00, "7" )
	PORT_DIPNAME( 0x04, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x18, 0x18, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x18, "20000 50000" )
	PORT_DIPSETTING(    0x10, "30000 60000" )
	PORT_DIPSETTING(    0x08, "30000" )
	PORT_DIPSETTING(    0x00, "40000" )
	PORT_DIPNAME( 0x60, 0x60, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x60, "Easy" )
	PORT_DIPSETTING(    0x40, "Medium" )
	PORT_DIPSETTING(    0x20, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
INPUT_PORTS_END



static struct GfxLayout charlayout =
{
	8,8,    /* 8*8 characters */
	512,    /* 512 characters */
	4,      /* 4 bits per pixel */
	{ 0, 1, 2, 3 }, /* the bitplanes are packed */
	{ 0*4, 1*4, 2*4, 3*4, 4*4, 5*4, 6*4, 7*4 },
	{ 0*4*8, 1*4*8, 2*4*8, 3*4*8, 4*4*8, 5*4*8, 6*4*8, 7*4*8 },
	8*4*8     /* every char takes 32 consecutive bytes */
};

static struct GfxLayout spritelayout =
{
	16,16,	     /* 16*16 sprites */
	256,	        /* 256 sprites */
	4,	           /* 4 bits per pixel */
	{ 0, 4, 256*128*8+0, 256*128*8+4 },
	{ 32*8+0, 32*8+1, 32*8+2, 32*8+3, 16*8+0, 16*8+1, 16*8+2, 16*8+3,
			0, 1, 2, 3, 48*8+0, 48*8+1, 48*8+2, 48*8+3 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16,
			32*16, 33*16, 34*16, 35*16, 36*16, 37*16, 38*16, 39*16 },
	128*8	/* every sprite takes 64 bytes */
};


static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0x0000, &charlayout,         0, 16*8 },
	{ REGION_GFX2, 0x0000, &spritelayout, 16*8*16, 16*8 },
	{ REGION_GFX2, 0x0001, &spritelayout, 16*8*16, 16*8 },
	{ -1 } /* end of array */
};



static struct SN76496interface sn76496_interface =
{
	2,	/* 2 chips */
	{ 1789750, 3579500 },	/* 1.78975 MHz ??? */
	{ 60, 60 }
};



static MACHINE_DRIVER_START( mikie )

	/* basic machine hardware */
	MDRV_CPU_ADD(M6809, 1250000)        /* 1.25 MHz */
	MDRV_CPU_MEMORY(readmem,writemem)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,1)

	MDRV_CPU_ADD(Z80,14318180/4)
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)	/* ? */
	MDRV_CPU_MEMORY(sound_readmem,sound_writemem)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 2*8, 30*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(256)
	MDRV_COLORTABLE_LENGTH(16*8*16+16*8*16)

	MDRV_PALETTE_INIT(mikie)
	MDRV_VIDEO_START(mikie)
	MDRV_VIDEO_UPDATE(mikie)

	/* sound hardware */
	MDRV_SOUND_ADD(SN76496, sn76496_interface)
MACHINE_DRIVER_END



/***************************************************************************

  Game driver(s)

***************************************************************************/


ROM_START( mikie )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )     /* 64k for code */
	ROM_LOAD( "11c_n14.bin",  0x6000, 0x2000, CRC(f698e6dd) SHA1(99220eeee4e7b88caa26f2d08502689e1f1fcdf8) )
	ROM_LOAD( "12a_o13.bin",  0x8000, 0x4000, CRC(826e7035) SHA1(bd62783cb1ba4e7f0196f337280461bb7627f70f) )
	ROM_LOAD( "12d_o17.bin",  0xc000, 0x4000, CRC(161c25c8) SHA1(373a92b8412676ad9870cb562c73e47db7b40bea) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )
	ROM_LOAD( "06e_n10.bin",  0x0000, 0x2000, CRC(2cf9d670) SHA1(b324b92aff70d7878160128611dd5fdec6949659) )

	ROM_REGION( 0x04000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "o11",          0x00000, 0x4000, CRC(3c82aaf3) SHA1(c84256ac5fd5e40b197651c56e303c69aae72950) )

	ROM_REGION( 0x10000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "001",          0x00000, 0x4000, CRC(a2ba0df5) SHA1(873d49c1c2efbb222d1bf63396729d4b7d9477c3) )
	ROM_LOAD( "003",          0x04000, 0x4000, CRC(9775ab32) SHA1(0271567c5f5a6bb2eaffb9d5dc2af6b8142dc8a9) )
	ROM_LOAD( "005",          0x08000, 0x4000, CRC(ba44aeef) SHA1(410bfd9146242254a920092e280af87586709527) )
	ROM_LOAD( "007",          0x0c000, 0x4000, CRC(31afc153) SHA1(31ca33f585fddb86131ef61de73e3563fa027455) )

	ROM_REGION( 0x0500, REGION_PROMS, 0 )
	ROM_LOAD( "01i_d19.bin",  0x0000, 0x0100, CRC(8b83e7cf) SHA1(4fce779947f9f318023c7c54a71a4751f6bb8eb1) )	/* red component */
	ROM_LOAD( "03i_d21.bin",  0x0100, 0x0100, CRC(3556304a) SHA1(6f4fc3ef6b1b44278e7c8c1034ee4fbef90cf85a) )	/* green component */
	ROM_LOAD( "02i_d20.bin",  0x0200, 0x0100, CRC(676a0669) SHA1(14236a831204d52cdf8c2ef318a565d6c5587ce0) )	/* blue component */
	ROM_LOAD( "12h_d22.bin",  0x0300, 0x0100, CRC(872be05c) SHA1(1525303589d7ed909bc6e2827fbaa2c16ad4030b) )	/* character lookup table */
	ROM_LOAD( "f09_d18.bin",  0x0400, 0x0100, CRC(7396b374) SHA1(fedcc421a61d6623dc9c41b0a3e164efeb50ec7c) )	/* sprite lookup table */
ROM_END

ROM_START( mikiej )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )     /* 64k for code */
	ROM_LOAD( "11c_n14.bin",  0x6000, 0x2000, CRC(f698e6dd) SHA1(99220eeee4e7b88caa26f2d08502689e1f1fcdf8) )
	ROM_LOAD( "12a_o13.bin",  0x8000, 0x4000, CRC(826e7035) SHA1(bd62783cb1ba4e7f0196f337280461bb7627f70f) )
	ROM_LOAD( "12d_o17.bin",  0xc000, 0x4000, CRC(161c25c8) SHA1(373a92b8412676ad9870cb562c73e47db7b40bea) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )
	ROM_LOAD( "06e_n10.bin",  0x0000, 0x2000, CRC(2cf9d670) SHA1(b324b92aff70d7878160128611dd5fdec6949659) )

	ROM_REGION( 0x04000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "08i_q11.bin",  0x00000, 0x4000, CRC(c48b269b) SHA1(d7fcfa44fcda90f1a7df6c974210716ae82c47a3) )

	ROM_REGION( 0x10000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "f01_q01.bin",  0x00000, 0x4000, CRC(31551987) SHA1(b6cbdb8b511d99b27546a6c4d01f2948d5ad3a42) )
	ROM_LOAD( "f03_q03.bin",  0x04000, 0x4000, CRC(34414df0) SHA1(0189deac6f19de386b4e49cfe6322b212e74264a) )
	ROM_LOAD( "h01_q05.bin",  0x08000, 0x4000, CRC(f9e1ebb1) SHA1(c88c1fc22f21b3e7d558c47de2716dac01fdd621) )
	ROM_LOAD( "h03_q07.bin",  0x0c000, 0x4000, CRC(15dc093b) SHA1(0b5a5aea25283b8edb7f534fc84b13f5176e26d6) )

	ROM_REGION( 0x0500, REGION_PROMS, 0 )
	ROM_LOAD( "01i_d19.bin",  0x0000, 0x0100, CRC(8b83e7cf) SHA1(4fce779947f9f318023c7c54a71a4751f6bb8eb1) )	/* red component */
	ROM_LOAD( "03i_d21.bin",  0x0100, 0x0100, CRC(3556304a) SHA1(6f4fc3ef6b1b44278e7c8c1034ee4fbef90cf85a) )	/* green component */
	ROM_LOAD( "02i_d20.bin",  0x0200, 0x0100, CRC(676a0669) SHA1(14236a831204d52cdf8c2ef318a565d6c5587ce0) )	/* blue component */
	ROM_LOAD( "12h_d22.bin",  0x0300, 0x0100, CRC(872be05c) SHA1(1525303589d7ed909bc6e2827fbaa2c16ad4030b) )	/* character lookup table */
	ROM_LOAD( "f09_d18.bin",  0x0400, 0x0100, CRC(7396b374) SHA1(fedcc421a61d6623dc9c41b0a3e164efeb50ec7c) )	/* sprite lookup table */
ROM_END

ROM_START( mikiehs )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )     /* 64k for code */
	ROM_LOAD( "11c_l14.bin",  0x6000, 0x2000, CRC(633f3a6d) SHA1(9255e0cb8d53773a52cade2fbd2e4c1968164313) )
	ROM_LOAD( "12a_m13.bin",  0x8000, 0x4000, CRC(9c42d715) SHA1(533ae7c5bde6d341b9138dd439d9ff46fe0767f4) )
	ROM_LOAD( "12d_m17.bin",  0xc000, 0x4000, CRC(cb5c03c9) SHA1(6bc2f4c5f4f97c2bbcac8ff51358369ce07948e9) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )
	ROM_LOAD( "06e_h10.bin",  0x0000, 0x2000, CRC(4ed887d2) SHA1(953218a3b41019e2e52932dd3522741812c46c75) )

	ROM_REGION( 0x04000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "08i_l11.bin",  0x00000, 0x4000, CRC(5ba9d86b) SHA1(2246795dd68a62efb2c70a9177ee97a58ccb2566) )

	ROM_REGION( 0x10000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "f01_i01.bin",  0x00000, 0x4000, CRC(0c0cab5f) SHA1(c3eb4c3a432e86f4664329a0de5583cb5de7b6f5) )
	ROM_LOAD( "f03_i03.bin",  0x04000, 0x4000, CRC(694da32f) SHA1(02bd83d77f42822e42e48977856dfa0e3abfcab0) )
	ROM_LOAD( "h01_i05.bin",  0x08000, 0x4000, CRC(00e357e1) SHA1(d5b46709083d74950d0deedecb4fd631d0e74afb) )
	ROM_LOAD( "h03_i07.bin",  0x0c000, 0x4000, CRC(ceeba6ac) SHA1(ca5f715dece88540d9ed0e0146cff09f6868d09f) )

	ROM_REGION( 0x0500, REGION_PROMS, 0 )
	ROM_LOAD( "01i_d19.bin",  0x0000, 0x0100, CRC(8b83e7cf) SHA1(4fce779947f9f318023c7c54a71a4751f6bb8eb1) )	/* red component */
	ROM_LOAD( "03i_d21.bin",  0x0100, 0x0100, CRC(3556304a) SHA1(6f4fc3ef6b1b44278e7c8c1034ee4fbef90cf85a) )	/* green component */
	ROM_LOAD( "02i_d20.bin",  0x0200, 0x0100, CRC(676a0669) SHA1(14236a831204d52cdf8c2ef318a565d6c5587ce0) )	/* blue component */
	ROM_LOAD( "12h_d22.bin",  0x0300, 0x0100, CRC(872be05c) SHA1(1525303589d7ed909bc6e2827fbaa2c16ad4030b) )	/* character lookup table */
	ROM_LOAD( "f09_d18.bin",  0x0400, 0x0100, CRC(7396b374) SHA1(fedcc421a61d6623dc9c41b0a3e164efeb50ec7c) )	/* sprite lookup table */
ROM_END



GAME( 1984, mikie,   0,     mikie, mikie, 0, ROT270, "Konami", "Mikie" )
GAME( 1984, mikiej,  mikie, mikie, mikie, 0, ROT270, "Konami", "Shinnyuushain Tooru-kun" )
GAME( 1984, mikiehs, mikie, mikie, mikie, 0, ROT270, "Konami", "Mikie (High School Graffiti)" )
