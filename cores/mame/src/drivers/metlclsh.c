/***************************************************************************

							  -= Metal Clash =-

	driver by Luca Elia, based on brkthru.c by Phil Stroffolino


CPUs 	:	2 x 6809
Sound	:	YM2203  +  YM3526
Video	:	TC15G008AP + TC15G032CY (TOSHIBA)

---------------------------------------------------------------------------
Year + Game			Boards
---------------------------------------------------------------------------
85	Metal Clash		DE-0212-1 & DE-0213-1
---------------------------------------------------------------------------

Notes:

- Similar hardware to that in brkthru.c
- Screenshots here: www.ne.jp/asahi/cc-sakura/akkun/bekkan/metal.html

To Do:

metlclsh:
- Clocks are all unknown
- Text on the title screen has wrong colors the first time around
 (unitialized foreground palette 1, will be initialized shortly)
- The background tilemap ram is bankswitched with other (not understood) ram
- There are a few unknown writes

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "cpu/m6809/m6809.h"

/* Variables defined in vidhrdw: */

extern data8_t *metlclsh_bgram, *metlclsh_fgram, *metlclsh_scrollx;

/* Functions defined in vidhrdw: */

WRITE_HANDLER( metlclsh_bgram_w );
WRITE_HANDLER( metlclsh_fgram_w );
WRITE_HANDLER( metlclsh_gfxbank_w );
WRITE_HANDLER( metlclsh_rambank_w );

VIDEO_START( metlclsh );
VIDEO_UPDATE( metlclsh );

/***************************************************************************

							Memory Maps - CPU #1

***************************************************************************/

static data8_t *sharedram;

static READ_HANDLER ( sharedram_r )	{ return sharedram[offset]; }
static WRITE_HANDLER( sharedram_w )	{ sharedram[offset] = data; }

static WRITE_HANDLER( metlclsh_cause_irq )
{
	cpu_set_irq_line(1,M6809_IRQ_LINE,ASSERT_LINE);
}

static WRITE_HANDLER( metlclsh_ack_nmi )
{
	cpu_set_irq_line(0,IRQ_LINE_NMI,CLEAR_LINE);
}

static MEMORY_READ_START( metlclsh_readmem )
	{ 0x0000, 0x7fff, MRA_ROM					},
	{ 0x8000, 0x9fff, sharedram_r				},
	{ 0xa000, 0xbfff, MRA_ROM					},
	{ 0xc000, 0xc000, input_port_0_r			},
	{ 0xc001, 0xc001, input_port_1_r			},
	{ 0xc002, 0xc002, input_port_2_r			},
	{ 0xc003, 0xc003, input_port_3_r			},
/*	{ 0xc800, 0xc82f, MRA_RAM					},	*/ /* not actually read*/
/*	{ 0xcc00, 0xcc2f, MRA_RAM					},	*/ /* ""*/
	{ 0xd000, 0xd000, YM2203_status_port_0_r	},
/*	{ 0xd800, 0xdfff, MRA_RAM					},	*/ /* not actually read*/
	{ 0xe800, 0xe9ff, MRA_RAM					},
	{ 0xfff0, 0xffff, MRA_ROM					},	/* Reset/IRQ vectors*/
MEMORY_END

static MEMORY_WRITE_START( metlclsh_writemem )
	{ 0x0000, 0x7fff, MWA_ROM					},
	{ 0x8000, 0x9fff, sharedram_w, &sharedram	},
	{ 0xa000, 0xbfff, MWA_ROM					},
	{ 0xc080, 0xc080, MWA_NOP					},	/* ? 0*/
	{ 0xc0c2, 0xc0c2, metlclsh_cause_irq		},	/* cause irq on cpu #2*/
	{ 0xc0c3, 0xc0c3, metlclsh_ack_nmi			},	/* nmi ack*/
	{ 0xc800, 0xc82f, paletteram_xxxxBBBBGGGGRRRR_split1_w, &paletteram		},
	{ 0xcc00, 0xcc2f, paletteram_xxxxBBBBGGGGRRRR_split2_w, &paletteram_2	},
	{ 0xd000, 0xd000, YM2203_control_port_0_w	},
	{ 0xd001, 0xd001, YM2203_write_port_0_w		},
	{ 0xe000, 0xe000, YM3526_control_port_0_w	},
	{ 0xe001, 0xe001, YM3526_write_port_0_w		},
	{ 0xe800, 0xe9ff, MWA_RAM, &spriteram, &spriteram_size	},
	{ 0xd800, 0xdfff, metlclsh_fgram_w, &metlclsh_fgram		},
	{ 0xfff0, 0xffff, MWA_ROM					},
MEMORY_END


/***************************************************************************

							Memory Maps - CPU #2

***************************************************************************/

static WRITE_HANDLER( metlclsh_cause_nmi2 )
{
	cpu_set_irq_line(0,IRQ_LINE_NMI,ASSERT_LINE);
}

static WRITE_HANDLER( metlclsh_ack_irq2 )
{
	cpu_set_irq_line(1,M6809_IRQ_LINE,CLEAR_LINE);
}

static WRITE_HANDLER( metlclsh_ack_nmi2 )
{
	cpu_set_irq_line(1,IRQ_LINE_NMI,CLEAR_LINE);
}

static WRITE_HANDLER( metlclsh_flipscreen_w )
{
	flip_screen_set(data & 1);
}

static MEMORY_READ_START( metlclsh_readmem2 )
	{ 0x0000, 0x7fff, MRA_ROM			},
	{ 0x8000, 0x9fff, sharedram_r		},
	{ 0xc000, 0xc000, input_port_0_r	},
	{ 0xc001, 0xc001, input_port_1_r	},
	{ 0xc002, 0xc002, input_port_2_r	},
	{ 0xc003, 0xc003, input_port_3_r	},
	{ 0xd000, 0xd7ff, MRA_BANK1			},
	{ 0xfff0, 0xffff, MRA_ROM			},	/* Reset/IRQ vectors*/
MEMORY_END

static MEMORY_WRITE_START( metlclsh_writemem2 )
	{ 0x0000, 0x7fff, MWA_ROM						},
	{ 0x8000, 0x9fff, sharedram_w					},
	{ 0xc000, 0xc000, metlclsh_gfxbank_w			},	/* bg tiles bank*/
	{ 0xc0c0, 0xc0c0, metlclsh_cause_nmi2			},	/* cause nmi on cpu #1*/
	{ 0xc0c1, 0xc0c1, metlclsh_ack_irq2				},	/* irq ack*/
	{ 0xd000, 0xd7ff, metlclsh_bgram_w, &metlclsh_bgram	},	/* this is banked*/
	{ 0xe417, 0xe417, metlclsh_ack_nmi2				},	/* nmi ack*/
	{ 0xe301, 0xe301, metlclsh_flipscreen_w			},	/* 0/1*/
	{ 0xe401, 0xe401, metlclsh_rambank_w			},
	{ 0xe402, 0xe403, MWA_RAM, &metlclsh_scrollx	},
/*	{ 0xe404, 0xe404, MWA_NOP						},	*/ /* ? 0*/
/*	{ 0xe410, 0xe410, MWA_NOP						},	*/ /* ? 0 on startup only*/
	{ 0xfff0, 0xffff, MWA_ROM						},
MEMORY_END


/***************************************************************************

								Input Ports

***************************************************************************/

INPUT_PORTS_START( metlclsh )
	PORT_START	/* IN0 - c000 */
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 1C_3C ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_BITX(    0x40, 0x40, IPT_DIPSWITCH_NAME | IPF_CHEAT, "Infinite Energy", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_BITX(    0x80, 0x80, IPT_DIPSWITCH_NAME | IPF_CHEAT, "Infinite Lives", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START	/* IN1 - c001 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT	)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT	)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP		)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN	)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1			)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2			)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START1			)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START2			)

	PORT_START	/* IN2 - c002 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT	|	IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT	|	IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP		|	IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN	|	IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1			|	IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2			|	IPF_COCKTAIL )
	PORT_BIT_IMPULSE( 0x40, IP_ACTIVE_LOW, IPT_COIN1, 1 )
	PORT_BIT_IMPULSE( 0x80, IP_ACTIVE_LOW, IPT_COIN2, 1 )

	PORT_START      /* IN3 - c003 */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "2" )
	PORT_DIPSETTING(    0x01, "3" )
	PORT_DIPNAME( 0x02, 0x02, "Enemies Speed" )
	PORT_DIPSETTING(    0x02, "Low" )
	PORT_DIPSETTING(    0x00, "High" )
	PORT_DIPNAME( 0x04, 0x04, "Enemies Energy" )
	PORT_DIPSETTING(    0x04, "Low" )
	PORT_DIPSETTING(    0x00, "High" )
	PORT_DIPNAME( 0x08, 0x08, "Time" )
	PORT_DIPSETTING(    0x00, "75" )
	PORT_DIPSETTING(    0x08, "90" )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )	/* cpu2 will clr c040 on startup forever*/
	PORT_BIT_IMPULSE( 0x40, IP_ACTIVE_LOW, IPT_SERVICE1, 1 )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_VBLANK )
INPUT_PORTS_END


/***************************************************************************

							Graphics Layouts

***************************************************************************/

static struct GfxLayout spritelayout =
{
	16,16,
	RGN_FRAC(1,3),
	3,
	{ RGN_FRAC(2,3), RGN_FRAC(1,3), RGN_FRAC(0,3) },
	{ STEP8(8*8*2,1), STEP8(8*8*0,1) },
	{ STEP8(8*8*0,8), STEP8(8*8*1,8) },
	16*16
};

static struct GfxLayout tilelayout16 =
{
	16,16,
	RGN_FRAC(1,4),
	3,
	{ RGN_FRAC(2,4), RGN_FRAC(1,4), RGN_FRAC(0,4) },
	{ STEP8(8*8*0+7,-1), STEP8(8*8*2+7,-1) },
	{ STEP8(8*8*0,8), STEP8(8*8*1,8) },
	16*16
};

static struct GfxLayout tilelayout8 =
{
	8,8,
	RGN_FRAC(1,2),
	2,
	{ 0, 4 },
	{ STEP4(RGN_FRAC(1,2),1), STEP4(0,1) },
	{ STEP8(0,8) },
	8*8
};

static struct GfxDecodeInfo metlclsh_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &spritelayout, 0x00, 2 }, /* [0] Sprites*/
	{ REGION_GFX2, 0, &tilelayout16, 0x10, 1 }, /* [1] Background*/
	{ REGION_GFX3, 0, &tilelayout8,  0x20, 4 }, /* [2] Foreground*/
	{ -1 }
};


/***************************************************************************

								Machine Drivers

***************************************************************************/

static void metlclsh_irqhandler(int linestate)
{
	cpu_set_irq_line(0,M6809_IRQ_LINE,linestate);
}

static struct YM2203interface metlclsh_ym2203_interface =
{
	1,
	1500000,	/* ?*/
	{ YM2203_VOL(50,10) },
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 }
};

static struct YM3526interface metlclsh_ym3526_interface =
{
	1,
	3000000,	/* ?*/
	{ 50 },
	{ metlclsh_irqhandler },
};

INTERRUPT_GEN( metlclsh_interrupt2 )
{
	if (cpu_getiloops() == 0)
		return;
	/* generate NMI on coin insertion */
	if ((~readinputport(2) & 0xc0) || (~readinputport(3) & 0x40))
		cpu_set_nmi_line(1, ASSERT_LINE);
}

static MACHINE_INIT( metlclsh )
{
	flip_screen_set(0);
}

static MACHINE_DRIVER_START( metlclsh )

	/* basic machine hardware */
	MDRV_CPU_ADD(M6809, 1500000)        /* ?*/
	MDRV_CPU_MEMORY(metlclsh_readmem, metlclsh_writemem)
	/* IRQ by YM3526, NMI by cpu #2*/

	MDRV_CPU_ADD(M6809, 1500000)        /* ?*/
	MDRV_CPU_MEMORY(metlclsh_readmem2, metlclsh_writemem2)
	MDRV_CPU_VBLANK_INT(metlclsh_interrupt2,2)
	/* IRQ by cpu #1, NMI by coins insertion*/

	MDRV_FRAMES_PER_SECOND(58)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)	/* we're using IPT_VBLANK*/

	MDRV_MACHINE_INIT(metlclsh)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 1*8, 30*8-1)
	MDRV_GFXDECODE(metlclsh_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(3 * 16)

	MDRV_VIDEO_START(metlclsh)
	MDRV_VIDEO_UPDATE(metlclsh)

	/* sound hardware */
	MDRV_SOUND_ADD(YM2203, metlclsh_ym2203_interface)
	MDRV_SOUND_ADD(YM3526, metlclsh_ym3526_interface)
MACHINE_DRIVER_END


/***************************************************************************

								ROMs Loading

***************************************************************************/

/***************************************************************************

METAL CLASH[DATA EAST] JAPAN (c)1985
ROM Type:2764,27256

Name            Size    Location
--------------------------------
CS00.BIN	2764	C11 cpu
CS01.BIN	27256	C12 cpu
CS02.BIN	27256	C14 cpu
CS03.BIN	27256	C15 cpu
CS04.BIN	27256	C17 cpu
CS05.BIN	27256	H7  sound

CS06.BIN	27256	D9  Video
CS07.BIN	27256	D10 Video
CS08.BIN	27256	D12 Video

TTL-PROM 82S123(Color Table,8bit x 32Byte).
0000:3A 78 79 71 75 74 76 32
0008:3A 3D 29 21 25 14 16 12
0010:00 00 00 00 00 00 00 00
0018:00 00 00 00 00 00 00 00

This ROM work at DE-0212-1 & DE-0213-1

cpu   :6809(MAIN),6809(SOUND)
sound :YM2203,YM3526
custom:TC15G008AP,TC15G032CY(TOSHIBA)
color :82S123

DIP-SW
SW1
1 Coin CHARGE SELECT 1
2 Coin CHARGE SELECT 1
3 Coin CHARGE SELECT 2
4 Coin CHARGE SELECT 2
5 DON't CHANGE(for SERVICE ??)
6 ATTRACT SOUND
7 My ROBOT Infinity
8 My ROBOT LEFT not Decriment

SW2
1 My ROBOT LEFT 2/3
2 EMENY SPEED  EASY/DIFFICULT
3 EMENY ENERGY EASY/DIFFICULT
4 TIME LONG/DIFFICULT
5 SCREEN CHANGE NORMAL/FLIP
6 none ??
7 none ??
8 DON't CHANGE(for SERVICE ??)

"DARWIN 4078" use TC15G032CY too.

***************************************************************************/

ROM_START( metlclsh )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "cs04.bin",    0x00000, 0x8000, CRC(c2cc79a6) SHA1(0f586d4145afabbb45ea4865ed7a6590b14a2ab0) )
	ROM_LOAD( "cs00.bin",    0x0a000, 0x2000, CRC(af0f2998) SHA1(09dd2516406168660d5cd3a36be1e5f0adbcdb8a) )
	ROM_COPY( REGION_CPU1, 0x7ff0, 0xfff0, 0x10 )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )
	ROM_LOAD( "cs03.bin",    0x00000, 0x8000, CRC(51c4720c) SHA1(7fd93bdcf029e7d2509b73b32f61fddf85f3453f) )
	ROM_COPY( REGION_CPU2, 0x7ff0, 0xfff0, 0x10 )

	ROM_REGION( 0x18000, REGION_GFX1, ROMREGION_DISPOSE )	/* Sprites*/
	ROM_LOAD( "cs06.bin",    0x00000, 0x8000, CRC(9f61403f) SHA1(0ebb1cb9d4983746b6b32ec948e7b9efd90783d1) )
	ROM_LOAD( "cs07.bin",    0x08000, 0x8000, CRC(d0610ea5) SHA1(3dfa16cbe93a4c08993111f78a8dd22c874fdd28) )
	ROM_LOAD( "cs08.bin",    0x10000, 0x8000, CRC(a8b02125) SHA1(145a22b2910b2fbfb28925f58968ee2bdeae1dda) )

	ROM_REGION( 0x10000, REGION_GFX2, ROMREGION_DISPOSE )	/* Background*/
	ROM_LOAD( "cs01.bin",    0x00000, 0x1000, CRC(9c72343d) SHA1(c5618be7874ab6c930b0e68935c93f1958a1916d) )
	ROM_CONTINUE(            0x04000, 0x1000 )
	ROM_CONTINUE(            0x08000, 0x1000 )
	ROM_CONTINUE(            0x0c000, 0x1000 )
	ROM_CONTINUE(            0x01000, 0x1000 )
	ROM_CONTINUE(            0x05000, 0x1000 )
	ROM_CONTINUE(            0x09000, 0x1000 )
	ROM_CONTINUE(            0x0d000, 0x1000 )
	ROM_LOAD( "cs02.bin",    0x02000, 0x1000, CRC(3674673e) SHA1(8ba8864cefcb79afe5fe6821005a9d19742756e9) )
	ROM_CONTINUE(            0x06000, 0x1000 )
	ROM_CONTINUE(            0x0a000, 0x1000 )
	ROM_CONTINUE(            0x0e000, 0x1000 )
	ROM_CONTINUE(            0x03000, 0x1000 )
	ROM_CONTINUE(            0x07000, 0x1000 )
	ROM_CONTINUE(            0x0b000, 0x1000 )
	ROM_CONTINUE(            0x0f000, 0x1000 )

	ROM_REGION( 0x04000, REGION_GFX3, ROMREGION_DISPOSE )	/* Foreground*/
	ROM_LOAD( "cs05.bin",    0x00000, 0x4000, CRC(f90c9c6b) SHA1(ca8e497e9c388078343dd1303beef6ee38748d6a) )
	ROM_CONTINUE(            0x00000, 0x4000 )	/* first half is empty*/

	ROM_REGION( 0x020, REGION_PROMS, 0 )	/* ?*/
	ROM_LOAD( "82s123.prm",   0x0000, 0x20, CRC(6844cc88) SHA1(89d23367aa6ff541205416e82781fe938dfeeb52) )
ROM_END

GAME( 1985, metlclsh, 0, metlclsh, metlclsh, 0, ROT0, "Data East", "Metal Clash (Japan)" )
