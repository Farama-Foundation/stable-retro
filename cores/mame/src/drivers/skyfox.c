/***************************************************************************

						-= Sky Fox / Exerizer =-

				driver by	Luca Elia (l.elia@tin.it)


CPU  :	Z80A x 2
Sound:	YM2203C x 2
Other:	2 HM6116LP-3 (one on each board)
		1 KM6264L-15 (on bottom board)

To Do:	The background rendering is entirely guesswork

***************************************************************************/
#include "driver.h"
#include "vidhrdw/generic.h"

/* Variables defined in vidhrdw: */

extern int skyfox_bg_pos, skyfox_bg_ctrl;


/* Functions defined in vidhrdw: */

READ_HANDLER( skyfox_vregs_r );
WRITE_HANDLER( skyfox_vregs_w );

PALETTE_INIT( skyfox );

VIDEO_UPDATE( skyfox );


/***************************************************************************


								Main CPU


***************************************************************************/


/***************************************************************************
								Sky Fox
***************************************************************************/

static MEMORY_READ_START( skyfox_readmem )
	{ 0x0000, 0xbfff, MRA_ROM				},	/* ROM*/
	{ 0xc000, 0xdfff, MRA_RAM				},	/* RAM*/
	{ 0xe000, 0xe000, input_port_0_r		},	/* Input Ports*/
	{ 0xe001, 0xe001, input_port_1_r		},	/**/
	{ 0xe002, 0xe002, input_port_2_r		},	/**/
	{ 0xf001, 0xf001, input_port_3_r		},	/**/
/*	{ 0xff00, 0xff07, skyfox_vregs_r		},	*/ /* fake to read the vregs*/
MEMORY_END

static MEMORY_WRITE_START( skyfox_writemem )
	{ 0x0000, 0xbfff, MWA_ROM								},	/* ROM*/
	{ 0xc000, 0xcfff, MWA_RAM								},	/* RAM*/
	{ 0xd000, 0xd3ff, MWA_RAM, &spriteram, &spriteram_size	},	/* Sprites*/
	{ 0xd400, 0xdfff, MWA_RAM								},	/* RAM?*/
	{ 0xe008, 0xe00f, skyfox_vregs_w						},	/* Video Regs*/
MEMORY_END






/***************************************************************************


								Sound CPU


***************************************************************************/


/***************************************************************************
								Sky Fox
***************************************************************************/


static MEMORY_READ_START( skyfox_sound_readmem )
	{ 0x0000, 0x7fff, MRA_ROM				},	/* ROM*/
	{ 0x8000, 0x87ff, MRA_RAM				},	/* RAM*/
	{ 0xa001, 0xa001, YM2203_read_port_0_r 	},	/* YM2203 #1*/
/*	{ 0xc001, 0xc001, YM2203_read_port_1_r 	},	*/ /* YM2203 #2*/
	{ 0xb000, 0xb000, soundlatch_r			},	/* From Main CPU*/
MEMORY_END

static MEMORY_WRITE_START( skyfox_sound_writemem )
	{ 0x0000, 0x7fff, MWA_ROM					},	/* ROM*/
	{ 0x8000, 0x87ff, MWA_RAM					},	/* RAM*/
/*	{ 0x9000, 0x9001, MWA_NOP					},	*/ /* ??*/
	{ 0xa000, 0xa000, YM2203_control_port_0_w 	},	/* YM2203 #1*/
	{ 0xa001, 0xa001, YM2203_write_port_0_w 	},	/**/
/*	{ 0xb000, 0xb001, MWA_NOP					},	*/ /* ??*/
	{ 0xc000, 0xc000, YM2203_control_port_1_w 	},	/* YM2203 #2*/
	{ 0xc001, 0xc001, YM2203_write_port_1_w 	},	/**/
MEMORY_END




/***************************************************************************


								Input Ports


***************************************************************************/

INPUT_PORTS_START( skyfox )

	PORT_START	/* IN0 - Player 1*/
	PORT_BIT(  0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    )
	PORT_BIT(  0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  )
	PORT_BIT(  0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT )
	PORT_BIT(  0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  )
	PORT_BIT(  0x10, IP_ACTIVE_LOW, IPT_BUTTON2        )
	PORT_BIT(  0x20, IP_ACTIVE_LOW, IPT_BUTTON1        )
	PORT_BIT(  0x40, IP_ACTIVE_LOW, IPT_START1         )
	PORT_BIT(  0x80, IP_ACTIVE_LOW, IPT_START2         )


	PORT_START	/* IN1 - DSW*/
	PORT_DIPNAME( 0x01, 0x01, "Unknown 1-0" )		/* rest unused?*/
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, "Unknown 1-1" )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, "Unknown 1-2" )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x18, 0x08, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x00, "20K" )
	PORT_DIPSETTING(    0x08, "30K" )
	PORT_DIPSETTING(    0x10, "40K" )
	PORT_DIPSETTING(    0x18, "50K" )
	PORT_DIPNAME( 0x20, 0x00, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x20, "Medium" )
	PORT_DIPSETTING(    0x00, "Hard" )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Cocktail ) )


	PORT_START	/* IN2 - Coins + DSW + Vblank*/
	PORT_BIT(  0x01, IP_ACTIVE_LOW, IPT_VBLANK  )
	PORT_BIT(  0x02, IP_ACTIVE_LOW, IPT_COIN1   )
	PORT_DIPNAME( 0x0c, 0x00, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x00, "1 Coin/1 Credit  2C/1C" )	/* coin A & B*/
	PORT_DIPSETTING(    0x04, "1 Coin/2 Credits 3C/1C" )
	PORT_DIPSETTING(    0x08, "1 Coin/3 Credits 4C/1C" )
	PORT_DIPSETTING(    0x0c, "1 Coin/4 Credits 5C/1C" )
	PORT_BIT(  0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )


	PORT_START	/* IN3 - DSW*/
	PORT_DIPNAME( 0x07, 0x02, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "1" )
	PORT_DIPSETTING(    0x01, "2" )
	PORT_DIPSETTING(    0x02, "3" )
	PORT_DIPSETTING(    0x03, "4" )
	PORT_DIPSETTING(    0x04, "5" )
/*	PORT_DIPSETTING(    0x05, "5" )*/
/*	PORT_DIPSETTING(    0x06, "5" )*/
	PORT_BITX( 0x07, 0x07, IPT_DIPSWITCH_SETTING | IPF_CHEAT, "Infinite", IP_KEY_NONE, IP_JOY_NONE )
	PORT_BIT(  0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* IN4 - Fake input port, polled every VBLANK to generate an NMI upon coin insertion*/
	PORT_BIT_IMPULSE(  0x01, IP_ACTIVE_LOW, IPT_COIN1, 1 )
	PORT_BIT_IMPULSE(  0x02, IP_ACTIVE_LOW, IPT_COIN2, 1 )


INPUT_PORTS_END




/***************************************************************************


								Graphics Layouts


***************************************************************************/

/* 8x8x8 tiles (note that the tiles in the ROMs are 32x32x8, but
   we cut them in 8x8x8 ones in the init function, in order to
   support 8x8, 16x16 and 32x32 sprites. */

static struct GfxLayout layout_8x8x8 =
{
	8,8,
	RGN_FRAC(1,1),
	8,
	{0,1,2,3,4,5,6,7},
	{0*8,1*8,2*8,3*8,4*8,5*8,6*8,7*8},
	{0*64,1*64,2*64,3*64,4*64,5*64,6*64,7*64},
	8*8*8
};

/***************************************************************************
								Sky Fox
***************************************************************************/

static struct GfxDecodeInfo skyfox_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &layout_8x8x8,   0, 1 }, /* [0] Sprites*/
	{ -1 }
};






/***************************************************************************


								Machine Drivers


***************************************************************************/


/***************************************************************************
								Sky Fox
***************************************************************************/

/* Check for coin insertion once a frame (polling a fake input port).
   Generate an NMI in case. Scroll the background too. */

static INTERRUPT_GEN( skyfox_interrupt )
{
	/* Scroll the bg */
	skyfox_bg_pos += (skyfox_bg_ctrl >> 1) & 0x7;	/* maybe..*/

	/* Check coin 1 & 2 */
	if ((readinputport(4) & 3) != 3) cpu_set_irq_line(0, IRQ_LINE_NMI, PULSE_LINE);
}

static struct YM2203interface skyfox_ym2203_interface =
{
	2,
	1748000,		/* ? same as sound cpu ? */
	{ YM2203_VOL(80,80), YM2203_VOL(80,80) },
	{ 0, 0 },
	{ 0, 0 },
	{ 0, 0 },
	{ 0, 0 },
	{ 0, 0 }
};

static MACHINE_DRIVER_START( skyfox )

	/* basic machine hardware */
	MDRV_CPU_ADD(Z80, 4000000)
	MDRV_CPU_MEMORY(skyfox_readmem,skyfox_writemem)
	MDRV_CPU_VBLANK_INT(skyfox_interrupt,1)		/* NMI caused by coin insertion */

	MDRV_CPU_ADD(Z80, 1748000)
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
	MDRV_CPU_MEMORY(skyfox_sound_readmem,skyfox_sound_writemem)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)	/* we're using IPT_VBLANK*/

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(512, 256)
	MDRV_VISIBLE_AREA(0+0x60, 320-1+0x60, 0+16, 256-1-16)	/* from $30*2 to $CC*2+8*/
	MDRV_GFXDECODE(skyfox_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(256+256)	/* 256 static colors (+256 for the background??) */

	MDRV_PALETTE_INIT(skyfox)
	MDRV_VIDEO_UPDATE(skyfox)

	/* sound hardware */
	MDRV_SOUND_ADD(YM2203, skyfox_ym2203_interface)
MACHINE_DRIVER_END






/***************************************************************************


								ROMs Loading


***************************************************************************/



/***************************************************************************

									Sky Fox


c042	:	Lives
c044-5	:	Score (BCD)
c048-9	:	Power (BCD)

***************************************************************************/

/***************************************************************************

								Exerizer [Bootleg]

malcor

Location     Type     File ID    Checksum
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
TB 5E       27C256      1-J        F302      [  background  ]
TB 5N       27C256      1-I        F5E3      [    sound     ]
LB          27C256      1-A        A53E      [  program 1   ]
LB          27C256      1-B        382C      [  program 2   ]
LB          27C512      1-C        CDAC      [     GFX      ]
LB          27C512      1-D        9C7A      [     GFX      ]
LB          27C512      1-E        D808      [     GFX      ]
LB          27C512      1-F        F87E      [     GFX      ]
LB          27C512      1-G        9709      [     GFX      ]
LB          27C512      1-H        DFDA      [     GFX      ]
TB          82S129      1.BPR      0972      [ video blue   ]
TB          82S129      2.BPR      0972      [ video red    ]
TB          82S129      3.BPR      0972      [ video green  ]

Lower board ROM locations:

---=======------=======----
|    CN2          CN1     |
|                     1-A |
|                         |
|                     1-B |
|                         |
|                         |
|              1 1 1 1 1 1|
|              H G F E D C|
---------------------------

Notes  -  This archive is of a bootleg copy,
       -  Japanese program revision
       -  Although the colour PROMs have the same checksums,
          they are not the same.

Main processor  - Z80  4MHz
Sound processor - Z80  1.748MHz
                - YM2203C x2

***************************************************************************/


ROM_START( skyfox )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )		/* Main Z80 Code */
	ROM_LOAD( "skyfox1.bin", 0x00000, 0x8000, CRC(b4d4bb6f) SHA1(ed1cf6d91ca7170cb7d1c80b586c11164430fd49) )
	ROM_LOAD( "skyfox2.bin", 0x08000, 0x8000, CRC(e15e0263) SHA1(005934327834aed46b17161aef82117ee508e9c4) )	/* identical halves*/

	ROM_REGION( 0x10000, REGION_CPU2, 0 )		/* Sound Z80 Code */
	ROM_LOAD( "skyfox9.bin", 0x00000, 0x8000, CRC(0b283bf5) SHA1(5b14d0beea689ee7e9174017e5a127435df4fbe3) )

	ROM_REGION( 0x60000, REGION_GFX1, ROMREGION_DISPOSE )	/* Sprites */
	ROM_LOAD( "skyfox3.bin", 0x00000, 0x10000, CRC(3a17a929) SHA1(973fb36af416161e04a83d7869819d9b13df18cd) )
	ROM_LOAD( "skyfox4.bin", 0x10000, 0x10000, CRC(358053bb) SHA1(589e3270eda0d44e73fbc7ac06e782f332920b39) )
	ROM_LOAD( "skyfox5.bin", 0x20000, 0x10000, CRC(c1215a6e) SHA1(5ca30be8a68ac6a00907cc9e47ede0acec980f46) )
	ROM_LOAD( "skyfox6.bin", 0x30000, 0x10000, CRC(cc37e15d) SHA1(80806df6185f7b8c2d3ab98420ca514df3e63c8d) )
	ROM_LOAD( "skyfox7.bin", 0x40000, 0x10000, CRC(fa2ab5b4) SHA1(c0878b25dae28f7d49e14376ff885d1d4e3d5dfe) )
	ROM_LOAD( "skyfox8.bin", 0x50000, 0x10000, CRC(0e3edc49) SHA1(3d1c59ecaabe1c9517203b7e814db41d5cff0cd4) )

	ROM_REGION( 0x08000, REGION_GFX2, 0 )	/* Background - do not dispose */
	ROM_LOAD( "skyfox10.bin", 0x0000, 0x8000, CRC(19f58f9c) SHA1(6887216243b47152129448cbb4c7d52309feed03) )

	ROM_REGION( 0x300, REGION_PROMS, 0 )	/* Color Proms */
	ROM_LOAD( "sfoxrprm.bin", 0x000, 0x100, CRC(79913c7f) SHA1(e64e6a3eb55f37984cb2597c8ffba6bc3bad49c7) )	/* R*/
	ROM_LOAD( "sfoxgprm.bin", 0x100, 0x100, CRC(fb73d434) SHA1(4a9bd61fbdce9441753c5921f95ead5c4655957e) )	/* G*/
	ROM_LOAD( "sfoxbprm.bin", 0x200, 0x100, CRC(60d2ab41) SHA1(e58a54f2aaee5c07136d5437e513d61fb18fbd9f) )	/* B*/
ROM_END

ROM_START( exerizrb )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )		/* Main Z80 Code */
	ROM_LOAD( "1-a",         0x00000, 0x8000, CRC(5df72a5d) SHA1(ca35ac06f3702fd650a584da2f442fbc61c00fce) )
	ROM_LOAD( "skyfox2.bin", 0x08000, 0x8000, CRC(e15e0263) SHA1(005934327834aed46b17161aef82117ee508e9c4) )	/* 1-b*/

	ROM_REGION( 0x10000, REGION_CPU2, 0 )		/* Sound Z80 Code */
	ROM_LOAD( "skyfox9.bin", 0x00000, 0x8000, CRC(0b283bf5) SHA1(5b14d0beea689ee7e9174017e5a127435df4fbe3) )	/* 1-i*/

	ROM_REGION( 0x60000, REGION_GFX1, ROMREGION_DISPOSE )	/* Sprites */
	ROM_LOAD( "1-c",         0x00000, 0x10000, CRC(450e9381) SHA1(f99b2ca73f1e4ba91b8066bb6d28d33b66a3ee81) )
	ROM_LOAD( "skyfox4.bin", 0x10000, 0x10000, CRC(358053bb) SHA1(589e3270eda0d44e73fbc7ac06e782f332920b39) )	/* 1-d*/
	ROM_LOAD( "1-e",         0x20000, 0x10000, CRC(50a38c60) SHA1(a4b8d530914d6c85b15940a7821b4365068de668) )
	ROM_LOAD( "skyfox6.bin", 0x30000, 0x10000, CRC(cc37e15d) SHA1(80806df6185f7b8c2d3ab98420ca514df3e63c8d) )	/* 1-f*/
	ROM_LOAD( "1-g",         0x40000, 0x10000, CRC(c9bbfe5c) SHA1(ce3f7d32baa8bb0bfc110877b5b5f4648ee959ac) )
	ROM_LOAD( "skyfox8.bin", 0x50000, 0x10000, CRC(0e3edc49) SHA1(3d1c59ecaabe1c9517203b7e814db41d5cff0cd4) )	/* 1-h*/

	ROM_REGION( 0x08000, REGION_GFX2, 0 )	/* Background - do not dispose */
	ROM_LOAD( "skyfox10.bin", 0x0000, 0x8000, CRC(19f58f9c) SHA1(6887216243b47152129448cbb4c7d52309feed03) )	/* 1-j*/

	ROM_REGION( 0x300, REGION_PROMS, 0 )	/* Color Proms */
	ROM_LOAD( "sfoxrprm.bin", 0x000, 0x100, CRC(79913c7f) SHA1(e64e6a3eb55f37984cb2597c8ffba6bc3bad49c7) )	/* 2-bpr*/
	ROM_LOAD( "sfoxgprm.bin", 0x100, 0x100, CRC(fb73d434) SHA1(4a9bd61fbdce9441753c5921f95ead5c4655957e) )	/* 3-bpr*/
	ROM_LOAD( "sfoxbprm.bin", 0x200, 0x100, CRC(60d2ab41) SHA1(e58a54f2aaee5c07136d5437e513d61fb18fbd9f) )	/* 1-bpr*/
ROM_END




/* Untangle the graphics: cut each 32x32x8 tile in 16 8x8x8 tiles */
DRIVER_INIT( skyfox )
{
	unsigned char *RAM = memory_region(REGION_GFX1);
	unsigned char *end = RAM + memory_region_length(REGION_GFX1);
	unsigned char buf[32*32];

	while (RAM < end)
	{
		int i;
		for (i=0;i<(32*32);i++)
			buf[i] = RAM[(i%8) + ((i/8)%8)*32 + ((i/64)%4)*8 + (i/256)*256];

		memcpy(RAM,buf,32*32);
		RAM += 32*32;
	}
}



GAME( 1987, skyfox,   0,      skyfox, skyfox, skyfox, ROT90, "Jaleco (Nichibutsu USA license)", "Sky Fox"  )
GAME( 1987, exerizrb, skyfox, skyfox, skyfox, skyfox, ROT90, "Jaleco", "Exerizer (Japan) (bootleg)" )
