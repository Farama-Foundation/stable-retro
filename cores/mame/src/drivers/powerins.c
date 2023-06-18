/***************************************************************************

						  -= Power Instinct =-
							(C) 1993 Atlus

				driver by	Luca Elia (l.elia@tin.it)

CPU:	MC68000
Sound:	OKIM6295

- Note:	To enter test mode press F2 (Test)
		Use 9 (Service Coin) to change page.

TODO:
- sprites flip y (not used by the game)


***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

/* Variables that vidhrdw has access to */

/* Variables defined in vidhrdw */
extern data16_t *powerins_vram_0, *powerins_vctrl_0;
extern data16_t *powerins_vram_1, *powerins_vctrl_1;

/* Functions defined in vidhrdw */

WRITE16_HANDLER( powerins_flipscreen_w );
WRITE16_HANDLER( powerins_tilebank_w );

WRITE16_HANDLER( powerins_paletteram16_w );

WRITE16_HANDLER( powerins_vram_0_w );
WRITE16_HANDLER( powerins_vram_1_w );

VIDEO_START( powerins );
VIDEO_UPDATE( powerins );


/***************************************************************************

								Memory Maps

***************************************************************************/

static int oki_bank;

static WRITE16_HANDLER( powerins_okibank_w )
{
	if (ACCESSING_LSB)
	{
		unsigned char *RAM = memory_region(REGION_SOUND1);
		int new_bank = data & 0x7;

		if (new_bank != oki_bank)
		{
			oki_bank = new_bank;
			memcpy(&RAM[0x30000],&RAM[0x40000 + 0x10000*new_bank],0x10000);
		}
	}
}

static WRITE16_HANDLER( powerin2_soundlatch_w )
{
	if (ACCESSING_LSB)
	{
		soundlatch_w(0, data & 0xff);
		cpu_set_irq_line(1,0,HOLD_LINE);
	}
}

static MEMORY_READ16_START( powerins_readmem )
	{ 0x000000, 0x0fffff, MRA16_ROM					},	/* ROM*/
	{ 0x100000, 0x100001, input_port_0_word_r		},	/* Coins + Start Buttons*/
	{ 0x100002, 0x100003, input_port_1_word_r		},	/* P1 + P2*/
	{ 0x100008, 0x100009, input_port_2_word_r		},	/* DSW 1*/
	{ 0x10000a, 0x10000b, input_port_3_word_r		},	/* DSW 2*/
	{ 0x10003e, 0x10003f, OKIM6295_status_0_lsb_r	},	/* OKI Status*/
	{ 0x120000, 0x120fff, MRA16_RAM					},	/* Palette*/
/**/{ 0x130000, 0x130007, MRA16_RAM					},	/* VRAM 0 Control*/
	{ 0x140000, 0x143fff, MRA16_RAM					},	/* VRAM 0*/
	{ 0x170000, 0x170fff, MRA16_RAM					},	/* VRAM 1*/
	{ 0x180000, 0x18ffff, MRA16_RAM					},	/* RAM + Sprites*/
MEMORY_END

static MEMORY_WRITE16_START( powerins_writemem )
	{ 0x000000, 0x0fffff, MWA16_ROM								},	/* ROM*/
	{ 0x100014, 0x100015, powerins_flipscreen_w					},	/* Flip Screen*/
	{ 0x100016, 0x100017, MWA16_NOP								},	/* ? always 1*/
	{ 0x100018, 0x100019, powerins_tilebank_w					},	/* Tiles Banking (VRAM 0)*/
	{ 0x10001e, 0x10001f, powerin2_soundlatch_w					},	/* Sound Latch*/
	{ 0x100030, 0x100031, powerins_okibank_w					},	/* Sound*/
	{ 0x10003e, 0x10003f, OKIM6295_data_0_lsb_w					},	/**/
	{ 0x120000, 0x120fff, powerins_paletteram16_w, &paletteram16	},	/* Palette*/
	{ 0x130000, 0x130007, MWA16_RAM, &powerins_vctrl_0			},	/* VRAM 0 Control*/
	{ 0x140000, 0x143fff, powerins_vram_0_w, &powerins_vram_0	},	/* VRAM 0*/
	{ 0x170000, 0x170fff, powerins_vram_1_w, &powerins_vram_1	},	/* VRAM 1*/
	{ 0x171000, 0x171fff, powerins_vram_1_w						},	/* Mirror of VRAM 1?*/
	{ 0x180000, 0x18ffff, MWA16_RAM, &spriteram16				},	/* RAM + Sprites*/
MEMORY_END

/* There is an hidden test mode screen (set 18ff08 to 4 during test mode)
   that calls the data writtent to $10001e "sound code".
   This is a bootleg, so the original may have a sound CPU */

static MEMORY_READ_START( readmem_snd )
	{ 0x0000, 0xbfff, MRA_ROM },
	{ 0xc000, 0xdfff, MRA_RAM },
	{ 0xe000, 0xe000, MRA_NOP }, /* ?*/
MEMORY_END

static MEMORY_WRITE_START( writemem_snd )
	{ 0x0000, 0xbfff, MWA_ROM },
	{ 0xc000, 0xdfff, MWA_RAM },
	{ 0xe000, 0xe000, MWA_NOP }, /* ? written only once ?*/
	{ 0xe001, 0xe001, MWA_NOP }, /* ?*/
MEMORY_END

static PORT_READ_START( readport_snd )
	{ 0x00, 0x00, soundlatch_r },
	{ 0x01, 0x01, MRA_NOP }, /* ?*/
	{ 0x80, 0x80, OKIM6295_status_0_r },
	{ 0x88, 0x88, OKIM6295_status_1_r },
PORT_END

static PORT_WRITE_START( writeport_snd )
	{ 0x00, 0x00, MWA_NOP }, /* ?*/
	{ 0x01, 0x01, MWA_NOP }, /* ?*/
	{ 0x80, 0x80, OKIM6295_data_0_w },
	{ 0x88, 0x88, OKIM6295_data_1_w },
	{ 0x90, 0x97, MWA_NOP }, /* oki bank ?*/
PORT_END

/***************************************************************************

								Input Ports

***************************************************************************/

INPUT_PORTS_START( powerins )
	PORT_START	/* IN0 - $100000 - Coins*/
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_COIN1    )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_COIN2    )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_START1   )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_START2   )
	PORT_BITX(0x0020, IP_ACTIVE_LOW, IPT_SERVICE, DEF_STR( Service_Mode ), KEYCODE_F2, IP_JOY_NONE )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN  )

	PORT_START	/* IN1 - $100002 - Player 1 & 2*/
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER1 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT	 | IPF_PLAYER1 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER1 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER1 )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1        | IPF_PLAYER1 )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON2        | IPF_PLAYER1 )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_BUTTON3        | IPF_PLAYER1 )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_BUTTON4        | IPF_PLAYER1 )

	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER2 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT	 | IPF_PLAYER2 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER2 )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER2 )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON1        | IPF_PLAYER2 )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON2        | IPF_PLAYER2 )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_BUTTON3        | IPF_PLAYER2 )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_BUTTON4        | IPF_PLAYER2 )

	PORT_START	/* IN2 - $100008 - DSW 1*/
	PORT_DIPNAME( 0x0001, 0x0001, DEF_STR( Free_Play ) )
	PORT_DIPSETTING(      0x0001, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x000e, 0x000e, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(      0x0008, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(      0x0004, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x000c, DEF_STR( 2C_1C ) )
/*	PORT_DIPSETTING(      0x0000, DEF_STR( 2C_1C ) )  // 2C to start, 1C to continue /*/
	PORT_DIPSETTING(      0x000e, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0006, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x000a, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( 1C_4C ) )
	PORT_DIPNAME( 0x0070, 0x0070, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(      0x0040, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0060, DEF_STR( 2C_1C ) )
/*	PORT_DIPSETTING(      0x0000, DEF_STR( 2C_1C ) )  // 2C to start, 1C to continue /*/
	PORT_DIPSETTING(      0x0070, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0030, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x0050, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x0010, DEF_STR( 1C_4C ) )
	PORT_DIPNAME( 0x0080, 0x0080, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(      0x0080, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On )  )

	PORT_START	/* IN3 - $10000a - DSW 2*/
	PORT_DIPNAME( 0x0001, 0x0001, "Coin Chutes" )
	PORT_DIPSETTING(      0x0001, "1 Chute" )
	PORT_DIPSETTING(      0x0000, "2 Chutes" )
	PORT_DIPNAME( 0x0002, 0x0002, "Join In Mode" )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( On ) )
/*
	In "Join In" mode, a second player can join even if one player has aready
	begun to play.  Please refer to chart below:

	Join In Mode	Credit			Join In		Game Over
	-----------------------------------------------------------------------------------------------
	Join In ON	1C per Player		Anytime		Winner of VS Plays Computer
	Join In OFF	1C = VS Mode 2 players	Cannot		After win VS Game Over for both players

*/
	PORT_DIPNAME( 0x0004, 0x0000, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(      0x0004, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0008, 0x0008, "Allow Continue" )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0008, DEF_STR( On ) )
	PORT_DIPNAME( 0x0010, 0x0010, "Blood Color" )
	PORT_DIPSETTING(      0x0010, "Red" )
	PORT_DIPSETTING(      0x0000, "Blue" )
	PORT_DIPNAME( 0x0020, 0x0020, "Game Time" )
	PORT_DIPSETTING(      0x0020, "Normal" )
	PORT_DIPSETTING(      0x0000, "Short" )
	PORT_DIPNAME( 0x00c0, 0x00c0, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(      0x0040, "Easy" )
	PORT_DIPSETTING(      0x00c0, "Normal" )
	PORT_DIPSETTING(      0x0080, "Hard" )
	PORT_DIPSETTING(      0x0000, "Hardest" )
INPUT_PORTS_END



/***************************************************************************

								Graphics Layouts

***************************************************************************/

/* 8x8x4 tiles */
static struct GfxLayout layout_8x8x4 =
{
	8,8,
	RGN_FRAC(1,1),
	4,
	{0,1,2,3},
	{0*4,1*4,2*4,3*4,4*4,5*4,6*4,7*4},
	{0*32,1*32,2*32,3*32,4*32,5*32,6*32,7*32},
	8*8*4
};


/* 16x16x4 tiles (made of four 8x8 tiles) */
static struct GfxLayout layout_16x16x4 =
{
	16,16,
	RGN_FRAC(1,1),
	4,
	{0,1,2,3},
	{0*4,1*4,2*4,3*4,4*4,5*4,6*4,7*4,
	 128*4,129*4,130*4,131*4,132*4,133*4,134*4,135*4},
	{0*32,1*32,2*32,3*32,4*32,5*32,6*32,7*32,
	 8*32,9*32,10*32,11*32,12*32,13*32,14*32,15*32},
	16*16*4
};


/* 16x16x4 tiles (made of four 8x8 tiles). The bytes are swapped */
static struct GfxLayout layout_16x16x4_swap =
{
	16,16,
	RGN_FRAC(1,1),
	4,
	{0,1,2,3},
	{2*4,3*4,0*4,1*4,6*4,7*4,4*4,5*4,
	 130*4,131*4,128*4,129*4,134*4,135*4,132*4,133*4},
	{0*32,1*32,2*32,3*32,4*32,5*32,6*32,7*32,
	 8*32,9*32,10*32,11*32,12*32,13*32,14*32,15*32},
	16*16*4
};


static struct GfxDecodeInfo powerins_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &layout_16x16x4,      0x000, 0x20 }, /* [0] Tiles*/
	{ REGION_GFX2, 0, &layout_8x8x4,        0x200, 0x10 }, /* [1] Tiles*/
	{ REGION_GFX3, 0, &layout_16x16x4_swap, 0x400, 0x40 }, /* [2] Sprites*/
	{ -1 }
};






/***************************************************************************

								Machine Drivers

***************************************************************************/

MACHINE_INIT( powerins )
{
	oki_bank = -1;	/* samples bank "unitialised"*/
}

static struct OKIM6295interface powerins_okim6295_interface =
{
	1,
	{ 6000 },		/* ? */
	{ REGION_SOUND1 },
	{ 100 }
};

static struct OKIM6295interface powerin2_okim6295_interface =
{
	2,
	{ 4000 },		/* 4 Mhz */
	{ REGION_SOUND1, REGION_SOUND2 },
	{ 100, 100 }
};

static MACHINE_DRIVER_START( powerins )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 12000000)	/* ? (it affects the game's speed!) */
	MDRV_CPU_MEMORY(powerins_readmem,powerins_writemem)
	MDRV_CPU_VBLANK_INT(irq4_line_hold,1)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_MACHINE_INIT(powerins)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(320, 256)
	MDRV_VISIBLE_AREA(0, 320-1, 0+16, 256-16-1)
	MDRV_GFXDECODE(powerins_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(2048)

	MDRV_VIDEO_START(powerins)
	MDRV_VIDEO_UPDATE(powerins)

	/* sound hardware */
	MDRV_SOUND_ADD_TAG("sound", OKIM6295, powerins_okim6295_interface)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( powerina )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(powerins)

	MDRV_CPU_ADD(Z80, 6000000) /* 6 MHz */
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
	MDRV_CPU_MEMORY(readmem_snd,writemem_snd)
	MDRV_CPU_PORTS(readport_snd,writeport_snd)

	MDRV_SOUND_REPLACE("sound", OKIM6295, powerin2_okim6295_interface)
MACHINE_DRIVER_END



/***************************************************************************

								ROMs Loading

***************************************************************************/



/***************************************************************************

								Power Instinct

Location     Device       File ID     Checksum
----------------------------------------------
             27C240        ROM1         4EA1    [ MAIN PROGRAM ]
             27C240        ROM2         FE60    [ PROGRAM DATA ]
             27C010        ROM3         B9F7    [  CHARACTER   ]
             27C040        ROM4         2780    [  BACKGROUND  ]
             27C040        ROM5         98E0    [   PCM DATA   ]
            23C1600        ROM6         D9E9    [  BACKGROUND  ]
            23C1600        ROM7         8B04    [  MOTION OBJ  ]
            23C1600        ROM8         54B2    [  MOTION OBJ  ]
            23C1600        ROM9         C7C8    [  MOTION OBJ  ]
            23C1600        ROM10        852A    [  MOTION OBJ  ]

Notes:  This archive is of a bootleg version
        The main program is encrypted, using PLDs

Brief hardware overview
-----------------------

Main processor  -  68000
                -  TPC1020AFN-084C (CPLD)

Sound processor -  Main processor
                -  K-665-9249      (M6295)

***************************************************************************/

ROM_START( powerins )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )		/* 68000 Code */
	ROM_LOAD16_WORD_SWAP( "rom1", 0x000000, 0x080000, CRC(b86c84d6) SHA1(2ec0933130925dfae859ea6abe62a8c92385aee8) )
	ROM_LOAD16_WORD_SWAP( "rom2", 0x080000, 0x080000, CRC(d3d7a782) SHA1(7846de0ebb09bd9b2534cd451ff9aa5175e60647) )

	ROM_REGION( 0x280000, REGION_GFX1, ROMREGION_DISPOSE )	/* Layer 0 */
	ROM_LOAD( "rom6",  0x000000, 0x200000, CRC(b6c10f80) SHA1(feece0aeaa01a455d0c4885a3699f8bda14fe00f) )
	ROM_LOAD( "rom4",  0x200000, 0x080000, CRC(2dd76149) SHA1(975e4d371fdfbbd9a568da4d4c91ffd3f0ae636e) )

	ROM_REGION( 0x100000, REGION_GFX2, ROMREGION_DISPOSE )	/* Layer 1 */
	ROM_LOAD( "rom3",  0x000000, 0x020000, CRC(6a579ee0) SHA1(438e87b930e068e0cf7352e614a14049ebde6b8a) )

	ROM_REGION( 0x800000, REGION_GFX3, ROMREGION_DISPOSE )	/* Sprites */
	ROM_LOAD( "rom10", 0x000000, 0x200000, CRC(efad50e8) SHA1(89e8c307b927e987a32d22ab4ab7f3be037cca03) )
	ROM_LOAD( "rom9",  0x200000, 0x200000, CRC(08229592) SHA1(759679e89832b475adfdc783630d9ee2c105b0f3) )
	ROM_LOAD( "rom8",  0x400000, 0x200000, CRC(b02fdd6d) SHA1(1e2c52b4e9999f0b564fcf13ff41b097ad7d0c39) )
	ROM_LOAD( "rom7",  0x600000, 0x200000, CRC(92ab9996) SHA1(915ec8f383cc3652c3816a9b56ee54e22e104a5c) )

	ROM_REGION( 0x090000, REGION_SOUND1, 0 )	/* 8 bit adpcm (banked) */
	ROM_LOAD( "rom5", 0x000000, 0x030000, CRC(88579c8f) SHA1(13083934ab294c9b08d3e36f55c00a6a2e5a0507) )
	ROM_CONTINUE(     0x040000, 0x050000 )
ROM_END

/***************************************************************************

Power Instinct
Atlus, 1993

This is a bootleg US version with different sound hardware to the existing bootleg set.
The PCB is very large and has 2 plug-in daughterboards and many MASK ROMs.
The addition of the contents of the MASK ROMs would probably equal the contents of presumably
larger MASK ROMs found on the original PCB....

PCB Layout

|-------------------------------------------------------------|
|   M6295    4A 5A                             62256  62256   |
|   M6295    4B 5B                             62256  62256   |
|            4C 5C                             62256  62256   |
|            4D 5D                             62256  62256   |
| Z80        16MHz                             62256  62256   |
| 1F                                           62256  62256   |
| 6264       6116                              62256  62256   |
|            6116                              62256  62256   |
|J                                                            |
|A                                                            |
|M                                                            |
|M                                                            |
|A                                    82S123  11G 12G 13G  14G|
|                                             11I             |
|                           TPC1020  6116     11J             |
|DSW1        6116  6N                6116     11K     13K     |
|DSW2        6116              6264           11L     13L  14M|
|                      82S147  6264           11O     13O  14N|
|                                             11P 12P 13P  14P|
|      2Q    62256                            11Q     13Q     |
|      2R    62256                                    13R     |
|      68000                                                  |
| 12MHz              14.31818MHz                              |
|-------------------------------------------------------------|

Notes:
      68000 clock: 12.000MHz
      Z80 clock  :  6.000MHz
      M6295 clock:  4.000MHz (both); sample rate = 4000000/165 (both)
      VSync      :  60Hz

      ROMs 1F and 6N are 1M MASK (MX27C1000), all other ROMs are 4M MASK (MX27C4000).
      ROMS at 5* are located on a plug-in daughterboard.
      ROMS at 11*, 12*, 13G, 13P and 14* are located on a plug-in daughterboard.
      82S123 and 82S147 are PROMs.

***************************************************************************/

ROM_START( powerina )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )		/* 68000 Code */
	ROM_LOAD16_BYTE( "2q.bin", 0x000000, 0x80000, CRC(11bf3f2a) SHA1(c840add78da9b19839c667f9bbd77e0a7c560ed7) )
	ROM_LOAD16_BYTE( "2r.bin", 0x000001, 0x80000, CRC(d8d621be) SHA1(91d501ac661c1ff52c85eee96c455c008a7dad1c) )

	ROM_REGION( 0x20000, REGION_CPU2, 0 )		/* Z80 Code */
	ROM_LOAD( "1f.bin",  0x000000, 0x20000, CRC(4b123cc6) SHA1(ed61d3a2ab20c86b91fd7bafa717be3ce26159be) )

	ROM_REGION( 0x280000, REGION_GFX1, ROMREGION_DISPOSE )	/* Layer 0 */
	ROM_LOAD( "13k.bin", 0x000000, 0x80000, CRC(1975b4b8) SHA1(cb400967744fa602df1bd2d88950dfdbdc77073f) )
	ROM_LOAD( "13l.bin", 0x080000, 0x80000, CRC(376e4919) SHA1(12baa17382c176838df1b5ef86f1fa6dbcb978dd) )
	ROM_LOAD( "13o.bin", 0x100000, 0x80000, CRC(0d5ff532) SHA1(4febdb9cdacd85903a4a28e8df945dee0ce85558) )
	ROM_LOAD( "13q.bin", 0x180000, 0x80000, CRC(99b25791) SHA1(82f4bb5780826773d2e5f7143afb3ba209f57652) )
	ROM_LOAD( "13r.bin", 0x200000, 0x80000, CRC(2dd76149) SHA1(975e4d371fdfbbd9a568da4d4c91ffd3f0ae636e) )

	ROM_REGION( 0x100000, REGION_GFX2, ROMREGION_DISPOSE )	/* Layer 1 */
	ROM_LOAD( "6n.bin", 0x000000, 0x20000, CRC(6a579ee0) SHA1(438e87b930e068e0cf7352e614a14049ebde6b8a) )

	ROM_REGION( 0x800000, REGION_GFX3, ROMREGION_DISPOSE )	/* Sprites */
	ROM_LOAD16_BYTE( "14g.bin", 0x000000, 0x80000, CRC(8b9b89c9) SHA1(f1d39d1a62e40a14642d8f22fc38b764465a8daa) )
	ROM_LOAD16_BYTE( "11g.bin", 0x000001, 0x80000, CRC(4d127bdf) SHA1(26a7c277e7660a7c7c0c11cacadf815d2487ba8a) )
	ROM_LOAD16_BYTE( "13g.bin", 0x100000, 0x80000, CRC(298eb50e) SHA1(2b922c1473bb559a1e8bd6221619141658179bb9) )
	ROM_LOAD16_BYTE( "11i.bin", 0x100001, 0x80000, CRC(57e6d283) SHA1(4701576c8663ba47f388a02e61ef078a9dbbd212) )
	ROM_LOAD16_BYTE( "12g.bin", 0x200000, 0x80000, CRC(fb184167) SHA1(20924d3f35509f2f6af61f565b852ea72326d02c) )
	ROM_LOAD16_BYTE( "11j.bin", 0x200001, 0x80000, CRC(1b752a4d) SHA1(1b13f28af208542bee9da298d6e9db676cbc0845) )
	ROM_LOAD16_BYTE( "14m.bin", 0x300000, 0x80000, CRC(2f26ba7b) SHA1(026f960fa4de09ed940dd83a3db467c3676c5024) )
	ROM_LOAD16_BYTE( "11k.bin", 0x300001, 0x80000, CRC(0263d89b) SHA1(526b8ed05dffcbe98a44372bd55ad7b0ba91fc0f) )
	ROM_LOAD16_BYTE( "14n.bin", 0x400000, 0x80000, CRC(c4633294) SHA1(9578f516eaf09e743ee0262ce227f811bea1be8f) )
	ROM_LOAD16_BYTE( "11l.bin", 0x400001, 0x80000, CRC(5e4b5655) SHA1(f86509e75ec0c68f728715a5a325f6d1a30cfd93) )
	ROM_LOAD16_BYTE( "14p.bin", 0x500000, 0x80000, CRC(4d4b0e4e) SHA1(782c5edc533f10757cb18d2411046e44aa075ba1) )
	ROM_LOAD16_BYTE( "11o.bin", 0x500001, 0x80000, CRC(7e9f2d2b) SHA1(cfee03c38a6c781ad370638748244a164b83d588) )
	ROM_LOAD16_BYTE( "13p.bin", 0x600000, 0x80000, CRC(0e7671f2) SHA1(301af5c4229451cba9fdf40285dd7243626ffed4) )
	ROM_LOAD16_BYTE( "11p.bin", 0x600001, 0x80000, CRC(ee59b1ec) SHA1(437bc50c3b32c2edee549f5021345f1c924896b4) )
	ROM_LOAD16_BYTE( "12p.bin", 0x700000, 0x80000, CRC(9ab1998c) SHA1(fadaa4a46cefe0093ee1ebeddbae63143fa7bb5a) )
	ROM_LOAD16_BYTE( "11q.bin", 0x700001, 0x80000, CRC(1ab0c88a) SHA1(8bc72732f5911e0d4e0cf12fd2fb12d67e03299e) )

	ROM_REGION( 0x240000, REGION_SOUND1, 0 )	/* 8 bit adpcm (banked) */
	ROM_LOAD( "4a.bin", 0x040000, 0x80000, CRC(8cd6824e) SHA1(aa6d8917558de4f2aa8d80527209b9fe91122eb3) )
	ROM_LOAD( "4b.bin", 0x0c0000, 0x80000, CRC(e31ae04d) SHA1(c08d58a4250d8bdb68b8e5012624f345936520e1) )
	ROM_LOAD( "4c.bin", 0x140000, 0x80000, CRC(c4c9f599) SHA1(1d74acd626406052bec533a918ca24e14a2578f2) )
	ROM_LOAD( "4d.bin", 0x1c0000, 0x80000, CRC(f0a9f0e1) SHA1(4221e0824cdc8bcd6ea1c3811f4e3b7cd99478f2) )

	ROM_REGION( 0x240000, REGION_SOUND2, 0 )	/* 8 bit adpcm (banked) */
	ROM_LOAD( "5a.bin", 0x040000, 0x80000, CRC(62557502) SHA1(d72abdaec1c6f55f9b0099b7a8a297e0e14f920c) )
	ROM_LOAD( "5b.bin", 0x0c0000, 0x80000, CRC(dbc86bd7) SHA1(6f1bc3c7e6976fdcd4b2341cea07002fb0cefb14) )
	ROM_LOAD( "5c.bin", 0x140000, 0x80000, CRC(5839a2bd) SHA1(53988086ef97b2671044f6da9d97b1886900b64d) )
	ROM_LOAD( "5d.bin", 0x1c0000, 0x80000, CRC(446f9dc3) SHA1(5c81eb9a7cbea995db9a10d3b6460d02e104825f) )

	ROM_REGION( 0x0220, REGION_PROMS, 0 )		/* unknown */
	ROM_LOAD( "82s123.bin", 0x0000, 0x0020, CRC(67d5ec4b) SHA1(87d32948a0c88277dcdd0eaa035bde40fc7db5fe) )
	ROM_LOAD( "82s147.bin", 0x0020, 0x0200, CRC(d7818542) SHA1(e94f8004c804f260874a117d59dfa0637c5d3d73) )
ROM_END

GAME( 1993, powerins, 0,		powerins, powerins, 0, ROT0, "Atlus", "Power Instinct (USA bootleg) (set 1)" )
GAMEX(1993, powerina, powerins, powerina, powerins, 0, ROT0, "Atlus", "Power Instinct (USA bootleg) (set 2)", GAME_NO_SOUND )
