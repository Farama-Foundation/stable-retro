/***************************************************************************

					  -= Billiard Academy Real Break =-

					driver by	Luca Elia (l.elia@tin.it)


CPU    :	TMP68301

Sound  :	YMZ280b + YM2413

Chips  :	4L10F2467 (QFP160)
			4L10F2468 (QFP160)
			ACTEL A1010A
			PST532 (system reset and battery backup switching)


---------------------------------------------------------------------------
Year + Game							Board
---------------------------------------------------------------------------
98	Billiard Academy Real Break		NM523-1-9805
---------------------------------------------------------------------------

Notes:

- To reach the (hidden?) test modes put a breakpoint at 9f80, go in service
  mode and select "results" from the menu - this hits the breakpoint - then
  set ($3,a2) to non zero.

To Do:

- Priorities (e.g during the intro, there are two black bands in the backround
  that should obscure sprites).
- Sometimes sprites are shrinked to end up overlapping the background image
  in the tilemaps, but they are a few pixels off

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "machine/tmp68301.h"
#include "realbrk.h"

static data16_t *realbrk_dsw_select;

/* Read 4 ten bit dip switches */
static READ16_HANDLER( realbrk_dsw_r )
{
	data16_t sel = ~realbrk_dsw_select[0];
	if (sel & 0x01)	return	(readinputport(2) & 0x00ff) << 8;		/* DSW1 low bits*/
	if (sel & 0x02)	return	(readinputport(3) & 0x00ff) << 8;		/* DSW2 low bits*/
	if (sel & 0x04)	return	(readinputport(4) & 0x00ff) << 8;		/* DSW3 low bits*/
	if (sel & 0x08)	return	(readinputport(5) & 0x00ff) << 8;		/* DSW4 low bits*/

	if (sel & 0x10)	return	((readinputport(2) & 0x0300) << 0) |	/* DSWs high 2 bits*/
							((readinputport(3) & 0x0300) << 2) |
							((readinputport(4) & 0x0300) << 4) |
							((readinputport(5) & 0x0300) << 6) ;

	log_cb(RETRO_LOG_DEBUG, LOGPRE "CPU #0 PC %06X: read with unknown dsw_select = %02x\n",activecpu_get_pc(),realbrk_dsw_select[0]);
	return 0xffff;
}

/***************************************************************************

								Memory Maps

***************************************************************************/

static MEMORY_READ16_START( realbrk_readmem )
	{ 0x000000, 0x0fffff, MRA16_ROM					},	/* ROM*/
	{ 0x200000, 0x203fff, MRA16_RAM					},	/* Sprites*/
	{ 0x400000, 0x40ffff, MRA16_RAM					},	/* Palette*/
	{ 0x600000, 0x601fff, MRA16_RAM					},	/* Background	(0)*/
	{ 0x602000, 0x603fff, MRA16_RAM					},	/* Background	(1)*/
	{ 0x604000, 0x604fff, MRA16_RAM					},	/* Text			(2)*/
	{ 0x606000, 0x60600f, MRA16_RAM					},	/* Scroll + Video Regs*/
	{ 0x605000, 0x61ffff, MRA16_RAM					},	/**/
	{ 0x800002, 0x800003, YMZ280B_status_0_msb_r	},	/* YMZ280*/
	{ 0xc00000, 0xc00001, input_port_0_word_r		},	/* P1 & P2 (Inputs)*/
	{ 0xc00002, 0xc00003, input_port_1_word_r		},	/* Coins*/
	{ 0xc00004, 0xc00005, realbrk_dsw_r				},	/* 4 x DSW (10 bits each)*/
	{ 0xfe0000, 0xfeffff, MRA16_RAM					},	/* RAM*/
	{ 0xff0000, 0xfffbff, MRA16_RAM					},	/* RAM*/
	{ 0xfffc00, 0xffffff, MRA16_RAM					},	/* TMP68301 Registers*/
MEMORY_END

static MEMORY_WRITE16_START( realbrk_writemem )
	{ 0x000000, 0x0fffff, MWA16_ROM							},	/* ROM*/
	{ 0x200000, 0x203fff, MWA16_RAM, &spriteram16			},	/* Sprites*/
	{ 0x400000, 0x40ffff, paletteram16_xBBBBBGGGGGRRRRR_word_w, &paletteram16	},	/* Palette*/
	{ 0x600000, 0x601fff, realbrk_vram_0_w, &realbrk_vram_0	},	/* Background	(0)*/
	{ 0x602000, 0x603fff, realbrk_vram_1_w, &realbrk_vram_1	},	/* Background	(1)*/
	{ 0x604000, 0x604fff, realbrk_vram_2_w, &realbrk_vram_2	},	/* Text			(2)*/
	{ 0x606000, 0x60600f, realbrk_vregs_w, &realbrk_vregs  	},	/* Scroll + Video Regs*/
	{ 0x605000, 0x61ffff, MWA16_RAM							},	/**/
	{ 0x800000, 0x800001, YMZ280B_register_0_msb_w			},	/* YMZ280*/
	{ 0x800002, 0x800003, YMZ280B_data_0_msb_w				},	/**/
	{ 0x800008, 0x800009, YM2413_register_port_0_lsb_w		},	/* YM2413*/
	{ 0x80000a, 0x80000b, YM2413_data_port_0_lsb_w			},	/**/
	{ 0xc00004, 0xc00005, MWA16_RAM, &realbrk_dsw_select	},	/* DSW select*/
	{ 0xfe0000, 0xfeffff, MWA16_RAM							},	/* RAM*/
	{ 0xff0000, 0xfffbff, MWA16_RAM							},	/* RAM*/
	{ 0xfffd0a, 0xfffd0b, realbrk_flipscreen_w				},	/* Hack! Parallel port data register*/
	{ 0xfffc00, 0xffffff, tmp68301_regs_w, &tmp68301_regs	},	/* TMP68301 Registers*/
MEMORY_END


/***************************************************************************

								Input Ports

***************************************************************************/

/***************************************************************************
						Billiard Academy Real Break
***************************************************************************/

INPUT_PORTS_START( realbrk )
	PORT_START	/* IN0 - $c00000.w*/
	PORT_BIT(  0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER2 )
	PORT_BIT(  0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER2 )
	PORT_BIT(  0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER2 )
	PORT_BIT(  0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER2 )
	PORT_BIT(  0x0010, IP_ACTIVE_LOW, IPT_BUTTON1        | IPF_PLAYER2 )
	PORT_BIT(  0x0020, IP_ACTIVE_LOW, IPT_BUTTON2        | IPF_PLAYER2 )
	PORT_BIT(  0x0040, IP_ACTIVE_LOW, IPT_UNUSED )	/* BUTTON3 | IPF_PLAYER2 in test mode*/
	PORT_BIT(  0x0080, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT(  0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER1 )
	PORT_BIT(  0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER1 )
	PORT_BIT(  0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER1 )
	PORT_BIT(  0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER1 )
	PORT_BIT(  0x1000, IP_ACTIVE_LOW, IPT_BUTTON1        | IPF_PLAYER1 )
	PORT_BIT(  0x2000, IP_ACTIVE_LOW, IPT_BUTTON2        | IPF_PLAYER1 )
	PORT_BIT(  0x4000, IP_ACTIVE_LOW, IPT_UNUSED )	/* BUTTON3 | IPF_PLAYER1 in test mode*/
	PORT_BIT(  0x8000, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START	/* IN1 - $c00002.w*/
	PORT_BIT(  0x00ff, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x0100, IP_ACTIVE_LOW,  IPT_COIN1 )
	PORT_BIT(  0x0200, IP_ACTIVE_LOW,  IPT_COIN2 )
	PORT_BIT(  0x0400, IP_ACTIVE_LOW,  IPT_SERVICE1 )
	PORT_BITX( 0x0800, IP_ACTIVE_LOW,  IPT_SERVICE, "Test", KEYCODE_F1, IP_JOY_NONE )
	PORT_BIT(  0x1000, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT(  0x2000, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT(  0x4000, IP_ACTIVE_HIGH, IPT_UNKNOWN )	/* the vblank routine wants these 2 bits high*/
	PORT_BIT(  0x8000, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START	/* IN2 - $c00004.w (DSW1)*/
	PORT_DIPNAME( 0x0007, 0x0007, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(      0x0001, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0003, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0007, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0006, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x0005, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x0004, DEF_STR( 1C_5C ) )
	PORT_DIPNAME( 0x0038, 0x0038, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(      0x0008, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(      0x0010, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0018, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0038, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0030, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x0028, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( 1C_5C ) )
	PORT_DIPNAME( 0x0040, 0x0040, DEF_STR( Free_Play ) )
	PORT_DIPSETTING(      0x0040, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0080, 0x0080, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0080, DEF_STR( On ) )
	PORT_DIPNAME( 0x0100, 0x0100, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(      0x0100, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_SERVICE( 0x0200, IP_ACTIVE_LOW )

	PORT_START	/* IN3 - $c00004.w (DSW2)*/
	PORT_DIPNAME( 0x0003, 0x0003, DEF_STR( Lives ) )
	PORT_DIPSETTING(      0x0000, "4" )
	PORT_DIPSETTING(      0x0001, "5" )
	PORT_DIPSETTING(      0x0003, "6" )
	PORT_DIPSETTING(      0x0002, "7" )
	PORT_DIPNAME( 0x000c, 0x000c, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(      0x0008, "Easy"    )
	PORT_DIPSETTING(      0x000c, "Normal"  )
	PORT_DIPSETTING(      0x0004, "Harder"  )
	PORT_DIPSETTING(      0x0000, "Hardest" )
	PORT_DIPNAME( 0x0030, 0x0030, "Time" )
	PORT_DIPSETTING(      0x0000, "110" )
	PORT_DIPSETTING(      0x0030, "120" )
	PORT_DIPSETTING(      0x0020, "130" )
	PORT_DIPSETTING(      0x0010, "150" )
	PORT_DIPNAME( 0x0040, 0x0040, "? Show time in easy mode ?" )
	PORT_DIPSETTING(      0x0000, DEF_STR( No ) )
	PORT_DIPSETTING(      0x0040, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x0080, 0x0080, "? Coins ?" )
	PORT_DIPSETTING(      0x0000, "1" )
	PORT_DIPSETTING(      0x0080, "2" )

	PORT_START	/* IN4 - $c00004.w (DSW3) - Unused*/
	PORT_BIT(  0xffff, IP_ACTIVE_LOW,  IPT_UNKNOWN )

	PORT_START	/* IN5 - $c00004.w (DSW4) - Unused*/
	PORT_BIT(  0xffff, IP_ACTIVE_LOW,  IPT_UNKNOWN )
INPUT_PORTS_END


/***************************************************************************

							Graphics Layouts

***************************************************************************/

static struct GfxLayout layout_8x8x4 =
{
	8,8,
	RGN_FRAC(1,1),
	4,
	{	STEP4(0,1)		},
	{	STEP4(3*4,-4),STEP4(7*4,-4)		},
	{	STEP8(0,8*4)	},
	8*8*4
};

static struct GfxLayout layout_16x16x4 =
{
	16,16,
	RGN_FRAC(1,1),
	4,
	{	STEP4(0,1)		},
	{	1*4,0*4,3*4,2*4,5*4,4*4,7*4,6*4,9*4,8*4,11*4,10*4,13*4,12*4,15*4,14*4 },
	{	STEP16(0,16*4)	},
	16*16*4
};

static struct GfxLayout layout_16x16x8 =
{
	16,16,
	RGN_FRAC(1,1),
	8,
	{	STEP8(0,1)		},
	{	STEP16(0,8)		},
	{	STEP16(0,16*8)	},
	16*16*8
};

static struct GfxDecodeInfo realbrk_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &layout_16x16x8,		0, 0x80		},	/* [0] Backgrounds*/
	{ REGION_GFX2, 0, &layout_8x8x4,		0, 0x800	},	/* [1] Text*/
	{ REGION_GFX3, 0, &layout_16x16x8,		0, 0x80		},	/* [2] Sprites (256 colors)*/
	{ REGION_GFX4, 0, &layout_16x16x4,		0, 0x800	},	/* [3] Sprites (16 colors)*/
	{ -1 }
};


/***************************************************************************

								Machine Drivers

***************************************************************************/

/***************************************************************************
						Billiard Academy Real Break
***************************************************************************/

static struct YMZ280Binterface realbrk_ymz280b_intf =
{
	1,
	{ 33868800 / 2 },
	{ REGION_SOUND1 },
	{ YM3012_VOL(50,MIXER_PAN_LEFT,50,MIXER_PAN_RIGHT) },
	{ 0 }
};

static struct YM2413interface realbrk_ym2413_intf =
{
	1,	/* 1 chip */
	3579000,	/* 3.579 MHz */
	{ YM2413_VOL(50,MIXER_PAN_CENTER,50,MIXER_PAN_CENTER) }
};

static INTERRUPT_GEN( realbrk_interrupt )
{
	switch ( cpu_getiloops() )
	{
		case 0:
			/* VBlank is connected to INT1 (external interrupts pin 1) */
			tmp68301_external_interrupt_1();
			break;
	}
}

static MACHINE_DRIVER_START( realbrk )

	/* basic machine hardware */
	MDRV_CPU_ADD_TAG("main",M68000,32000000 / 2)			/* !! TMP68301 !! */
	MDRV_CPU_MEMORY(realbrk_readmem,realbrk_writemem)
	MDRV_CPU_VBLANK_INT(realbrk_interrupt,1)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_MACHINE_INIT( tmp68301 )

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(0x140, 0xe0)
	MDRV_VISIBLE_AREA(0, 0x140-1, 0, 0xe0-1)
	MDRV_GFXDECODE(realbrk_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(0x8000)

	MDRV_VIDEO_START(realbrk)
	MDRV_VIDEO_UPDATE(realbrk)

	/* sound hardware */
	MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
	MDRV_SOUND_ADD(YMZ280B,	realbrk_ymz280b_intf)
	MDRV_SOUND_ADD(YM2413,	realbrk_ym2413_intf)
MACHINE_DRIVER_END

/***************************************************************************

								ROMs Loading

***************************************************************************/

/***************************************************************************

						Billiard Academy Real Break

Dynax, 199?

PCB Layout
----------


NM523-1-9805
------------------------------------------------------------------------
|                                                                      |
|  68301-16    52302    ACTEL    M548262   YMZ280B     *    52304      |
|              52301    A1010A   M548262             52303  52305      |
|                             33.8688MHz                               |
|                      TC55257               PST532A   %               |
|                      TC55257   4L10F2468                4L10F2467    |
|                      TC55257   (QFP160)      32.000MHz  (QFP160)     |
| J                    TC55257                                         |
| A                                     PAL                            |
| M                                                                    |
| M                              LH5160 52306     52308     52310      |
| A     VOL1                     LH5160      52307     52309     52311 |
|       VOL2                     LH5160                                |
|                                LH5160                                |
|                                                                      |
|          YM2413                TC55257                               |
|                    3.579MHz    TC55257  *  52312  *    *    *    *   |
|                                                                      |
|   YAC516                                                             |
|                             TC55257  TC55257                         |
|   DSW2(10) DSW1(10)         TC55257  TC55257                         |
|                                                                      |
------------------------------------------------------------------------

Notes:
            *: unpopulated 42 pin sockets.
            %: unpopulated position for coin battery.
       PST532: IC for system reset and battery backup switching
      DIPSW's: Each have 10 switches.
    VOL1/VOL2: Separate volume levels for sound and voice
Vertical Sync: 60Hz
  Horiz. Sync: 15.81kHz


***************************************************************************/

ROM_START( realbrk )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )		/* TMP68301 Code */
	ROM_LOAD16_BYTE( "52302.1r", 0x000000, 0x080000, CRC(ab0379b0) SHA1(67af6670f2b37a7d4d6e03508f291f8ffe64d4cb) )
	ROM_LOAD16_BYTE( "52301.2r", 0x000001, 0x080000, CRC(9cc1596e) SHA1(a598f18eaac1ed6943069e9500b07b77e263f0d0) )

	ROM_REGION( 0x800000, REGION_GFX1, ROMREGION_DISPOSE )	/* Backgrounds */
	ROM_LOAD32_WORD( "52310.9b", 0x0000000, 0x400000, CRC(07dfd9f5) SHA1(8722a98adc33f56df1e3b194ce923bc987e15cbe) )
	ROM_LOAD32_WORD( "52311.9a", 0x0000002, 0x400000, CRC(136a93a4) SHA1(b4bd46ba6c2b367aaf362f67d8be4757f1160864) )

	ROM_REGION( 0x40000, REGION_GFX2, ROMREGION_DISPOSE )	/* Text Layer */
	ROM_LOAD16_BYTE( "52305.1a", 0x000000, 0x020000, CRC(56546fb4) SHA1(5e4dc1665ca96bf24b89d92c24f5ff8420cb465e) )	/* 1xxxxxxxxxxxxxxxx = 0xFF*/
	ROM_LOAD16_BYTE( "52304.1b", 0x000001, 0x020000, CRC(b22b0aac) SHA1(8c62e19071a4031d0dcad621cce0ba550702659b) )	/* 1xxxxxxxxxxxxxxxx = 0xFF*/

	ROM_REGION( 0xc00000, REGION_GFX3, ROMREGION_DISPOSE )	/* Sprites (256 colors) */
	ROM_LOAD32_WORD( "52306.9f", 0x0000000, 0x400000, CRC(5ff0f666) SHA1(e3f1d9dc84fbef73af37cefd90bdf87a35f59e0e) )
	ROM_LOAD32_WORD( "52308.9d", 0x0000002, 0x400000, CRC(20817051) SHA1(4c9a443b5d6353ce67d5b1fe716f5ac20d194ef0) )
	ROM_LOAD32_WORD( "52307.9e", 0x0800000, 0x200000, CRC(01555191) SHA1(7751e2e16345acc638d4dff997a5b52e9171fced) )
	ROM_LOAD32_WORD( "52309.9c", 0x0800002, 0x200000, CRC(ef4f4bd9) SHA1(3233f501002a2622ddda581167ae24b1a13ea79e) )

	ROM_REGION( 0x200000, REGION_GFX4, ROMREGION_DISPOSE )	/* Sprites (16 colors) */
	ROM_LOAD( "52312.14f", 0x000000, 0x200000, CRC(2203d7c5) SHA1(0403f02b8f2bfc6cf98ff598eb9c2e3facc7ac4c) )

	ROM_REGION( 0x400000, REGION_SOUND1, ROMREGION_SOUNDONLY )	/* Samples */
	ROM_LOAD( "52303.2e", 0x000000, 0x400000, CRC(d3005b1e) SHA1(3afd10cdbc3aa7605083a9fcf3c4b8276937c2c4) )
ROM_END

GAMEX( 1998, realbrk, 0, realbrk, realbrk, 0, ROT0, "Nakanihon", "Billiard Academy Real Break (Japan)", GAME_IMPERFECT_GRAPHICS )
