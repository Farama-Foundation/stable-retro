/***************************************************************************

						  -= Yun Sung 8 Bit Games =-

					driver by	Luca Elia (l.elia@tin.it)


Main  CPU    :  Z80B
Sound CPU    :  Z80A
Video Chips  :	?
Sound Chips  :	OKI M5205 + YM3812

---------------------------------------------------------------------------
Year + Game         Board#
---------------------------------------------------------------------------
95 Cannon Ball      ?
95 Magix / Rock     ?
---------------------------------------------------------------------------

Notes:

- "Magix" can change title to "Rock" through a DSW
- In service mode press Service Coin (e.g. '9')

To Do:

- Better Sound

***************************************************************************/

#include "driver.h"
#include "cpu/z80/z80.h"

/* Variables defined in vidhrdw: */

extern data8_t *yunsung8_videoram_0, *yunsung8_videoram_1;
extern int yunsung8_layers_ctrl;

/* Functions defined in vidhrdw: */

WRITE_HANDLER( yunsung8_videobank_w );

READ_HANDLER ( yunsung8_videoram_r );
WRITE_HANDLER( yunsung8_videoram_w );

WRITE_HANDLER( yunsung8_flipscreen_w );

VIDEO_START( yunsung8 );
VIDEO_UPDATE( yunsung8 );



MACHINE_INIT( yunsung8 )
{
	unsigned char *RAM = memory_region(REGION_CPU1) + 0x24000;

	yunsung8_videoram_0 = RAM + 0x0000;	/* Ram is banked*/
	yunsung8_videoram_1 = RAM + 0x2000;
	yunsung8_videobank_w(0,0);
}


/***************************************************************************


							Memory Maps - Main CPU


***************************************************************************/


WRITE_HANDLER( yunsung8_bankswitch_w )
{
	unsigned char *RAM = memory_region(REGION_CPU1);

	int bank			=	data & 7;		/* ROM bank*/
	yunsung8_layers_ctrl	=	data & 0x30;	/* Layers enable*/

	if (data & ~0x37)	log_cb(RETRO_LOG_DEBUG, LOGPRE "CPU #0 - PC %04X: Bank %02X\n",activecpu_get_pc(),data);

	if (bank < 3)	RAM = &RAM[0x4000 * bank];
	else			RAM = &RAM[0x4000 * (bank-3) + 0x10000];

	cpu_setbank(1, RAM);
}

/*
	Banked Video RAM:

	c000-c7ff	Palette	(bit 1 of port 0 switches between 2 banks)

	c800-cfff	Color	(bit 0 of port 0 switches between 2 banks)
	d000-dfff	Tiles	""
*/

static MEMORY_READ_START( yunsung8_readmem )
	{ 0x0000, 0x7fff, MRA_ROM				},	/* ROM*/
	{ 0x8000, 0xbfff, MRA_BANK1				},	/* Banked ROM*/
	{ 0xc000, 0xdfff, yunsung8_videoram_r	},	/* Video RAM (Banked)*/
	{ 0xe000, 0xffff, MRA_RAM				},	/* RAM*/
MEMORY_END

static MEMORY_WRITE_START( yunsung8_writemem )
	{ 0x0000, 0x0000, MWA_ROM				},	/* ROM*/
	{ 0x0001, 0x0001, yunsung8_bankswitch_w	},	/* ROM Bank (again?)*/
	{ 0x0002, 0xbfff, MWA_ROM				},	/* ROM*/
	{ 0xc000, 0xdfff, yunsung8_videoram_w	},	/* Video RAM (Banked)*/
	{ 0xe000, 0xffff, MWA_RAM				},	/* RAM*/
MEMORY_END


static PORT_READ_START( yunsung8_readport )
	{ 0x00, 0x00, input_port_0_r		},	/* Coins*/
	{ 0x01, 0x01, input_port_1_r		},	/* P1*/
	{ 0x02, 0x02, input_port_2_r		},	/* P2*/
	{ 0x03, 0x03, input_port_3_r		},	/* DSW 1*/
	{ 0x04, 0x04, input_port_4_r		},	/* DSW 2*/
PORT_END

static PORT_WRITE_START( yunsung8_writeport )
	{ 0x00, 0x00, yunsung8_videobank_w	},	/* Video RAM Bank*/
	{ 0x01, 0x01, yunsung8_bankswitch_w	},	/* ROM Bank + Layers Enable*/
	{ 0x02, 0x02, soundlatch_w			},	/* To Sound CPU*/
	{ 0x06, 0x06, yunsung8_flipscreen_w	},	/* Flip Screen*/
	{ 0x07, 0x07, IOWP_NOP				},	/* ? (end of IRQ, random value)*/
PORT_END



/***************************************************************************


							Memory Maps - Sound CPU


***************************************************************************/


static int adpcm;

WRITE_HANDLER( yunsung8_sound_bankswitch_w )
{
	unsigned char *RAM = memory_region(REGION_CPU2);
	int bank = data & 7;

	if ( bank != (data&(~0x20)) ) 	log_cb(RETRO_LOG_DEBUG, LOGPRE "CPU #1 - PC %04X: Bank %02X\n",activecpu_get_pc(),data);

	if (bank < 3)	RAM = &RAM[0x4000 * bank];
	else			RAM = &RAM[0x4000 * (bank-3) + 0x10000];

	cpu_setbank(2, RAM);

	MSM5205_reset_w(0,data & 0x20);
}

WRITE_HANDLER( yunsung8_adpcm_w )
{
	/* Swap the nibbles */
	adpcm = ((data&0xf)<<4) | ((data >>4)&0xf);
}



static MEMORY_READ_START( yunsung8_sound_readmem )
	{ 0x0000, 0x7fff, MRA_ROM						},	/* ROM*/
	{ 0x8000, 0xbfff, MRA_BANK2						},	/* Banked ROM*/
	{ 0xf000, 0xf7ff, MRA_RAM						},	/* RAM*/
	{ 0xf800, 0xf800, soundlatch_r					},	/* From Main CPU*/
MEMORY_END

static MEMORY_WRITE_START( yunsung8_sound_writemem )
	{ 0x0000, 0x7fff, MWA_ROM						},	/* ROM*/
	{ 0x8000, 0xbfff, MWA_ROM						},	/* Banked ROM*/
	{ 0xe000, 0xe000, yunsung8_sound_bankswitch_w	},	/* ROM Bank*/
	{ 0xe400, 0xe400, yunsung8_adpcm_w				},
	{ 0xec00, 0xec00, YM3812_control_port_0_w		},	/* YM3812*/
	{ 0xec01, 0xec01, YM3812_write_port_0_w			},
	{ 0xf000, 0xf7ff, MWA_RAM						},	/* RAM*/
MEMORY_END




/***************************************************************************


								Input Ports


***************************************************************************/

/***************************************************************************
									Magix
***************************************************************************/

INPUT_PORTS_START( magix )

	PORT_START	/* IN0 - Coins*/
	PORT_BIT(  0x01, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT(  0x02, IP_ACTIVE_LOW, IPT_START2   )
	PORT_BIT(  0x01, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT(  0x08, IP_ACTIVE_LOW, IPT_START1   )
	PORT_BIT(  0x01, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT(  0x20, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT(  0x01, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT(  0x80, IP_ACTIVE_LOW, IPT_COIN1    )

	PORT_START	/* IN1 - Player 1*/
	PORT_BIT(  0x01, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT(  0x02, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT(  0x04, IP_ACTIVE_LOW, IPT_BUTTON2        | IPF_PLAYER1 )	/* same as button1 !?*/
	PORT_BIT(  0x08, IP_ACTIVE_LOW, IPT_BUTTON1        | IPF_PLAYER1 )
	PORT_BIT(  0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER1 )
	PORT_BIT(  0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER1 )
	PORT_BIT(  0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER1 )
	PORT_BIT(  0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER1 )

	PORT_START	/* IN2 - Player 2*/
	PORT_BIT(  0x01, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT(  0x02, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT(  0x04, IP_ACTIVE_LOW, IPT_BUTTON2        | IPF_PLAYER2 )	/* same as button1 !?*/
	PORT_BIT(  0x08, IP_ACTIVE_LOW, IPT_BUTTON1        | IPF_PLAYER2 )
	PORT_BIT(  0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER2 )
	PORT_BIT(  0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER2 )
	PORT_BIT(  0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER2 )
	PORT_BIT(  0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER2 )

	PORT_START	/* IN3 - DSW 1*/
	PORT_SERVICE( 0x01, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x06, 0x06, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x00, "Easy" )
	PORT_DIPSETTING(    0x06, "Normal" )
	PORT_DIPSETTING(    0x04, "Hard" )
	PORT_DIPSETTING(    0x02, "Hardest" )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0xe0, 0xe0, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x60, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0xe0, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0xa0, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 1C_4C ) )

	PORT_START	/* IN4 - DSW 2*/
	PORT_DIPNAME( 0x01, 0x01, "Title" )
	PORT_DIPSETTING(    0x01, "Magix" )
	PORT_DIPSETTING(    0x00, "Rock" )
	PORT_DIPNAME( 0x02, 0x02, "Unknown 2-1" )	/* the rest seems unused*/
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, "Unknown 2-2" )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, "Unknown 2-3" )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, "Unknown 2-4" )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, "Unknown 2-5" )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, "Unknown 2-6" )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, "Unknown 2-7" )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

INPUT_PORTS_END


/***************************************************************************
								Cannon Ball
***************************************************************************/

INPUT_PORTS_START( cannball )

	PORT_START	/* IN0 - Coins*/
	PORT_BIT(  0x01, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT(  0x02, IP_ACTIVE_LOW, IPT_START2   )
	PORT_BIT(  0x01, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT(  0x08, IP_ACTIVE_LOW, IPT_START1   )
	PORT_BIT(  0x01, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT(  0x20, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT(  0x01, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT(  0x80, IP_ACTIVE_LOW, IPT_COIN1    )

	PORT_START	/* IN1 - Player 1*/
	PORT_BIT(  0x01, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT(  0x02, IP_ACTIVE_LOW, IPT_BUTTON3        | IPF_PLAYER1 )
	PORT_BIT(  0x04, IP_ACTIVE_LOW, IPT_BUTTON2        | IPF_PLAYER1 )
	PORT_BIT(  0x08, IP_ACTIVE_LOW, IPT_BUTTON1        | IPF_PLAYER1 )
	PORT_BIT(  0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER1 )
	PORT_BIT(  0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER1 )
	PORT_BIT(  0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER1 )
	PORT_BIT(  0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER1 )

	PORT_START	/* IN2 - Player 2*/
	PORT_BIT(  0x01, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT(  0x02, IP_ACTIVE_LOW, IPT_BUTTON3        | IPF_PLAYER2 )
	PORT_BIT(  0x04, IP_ACTIVE_LOW, IPT_BUTTON2        | IPF_PLAYER2 )
	PORT_BIT(  0x08, IP_ACTIVE_LOW, IPT_BUTTON1        | IPF_PLAYER2 )
	PORT_BIT(  0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER2 )
	PORT_BIT(  0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER2 )
	PORT_BIT(  0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER2 )
	PORT_BIT(  0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER2 )

	PORT_START	/* IN3 - DSW 1*/
	PORT_SERVICE( 0x01, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x06, 0x06, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x00, "Easy" )
	PORT_DIPSETTING(    0x06, "Normal" )
	PORT_DIPSETTING(    0x04, "Hard" )
	PORT_DIPSETTING(    0x02, "Hardest" )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0xe0, 0xe0, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x60, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0xe0, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0xa0, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 1C_4C ) )

	PORT_START	/* IN4 - DSW 2*/
	PORT_DIPNAME( 0x01, 0x01, "Unknown 2-0" )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, "Unknown 2-1" )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x0c, 0x0c, "Bombs" )
	PORT_DIPSETTING(    0x04, "1" )
	PORT_DIPSETTING(    0x08, "2" )
	PORT_DIPSETTING(    0x0c, "3" )
	PORT_DIPSETTING(    0x00, "4" )
	PORT_DIPNAME( 0x10, 0x10, "Unknown 2-4" )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, "Unknown 2-5" )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, "Unknown 2-6" )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, "Unknown 2-7" )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

INPUT_PORTS_END



/***************************************************************************


								Graphics Layouts


***************************************************************************/

/* 8x8x4 tiles in 2 roms */
static struct GfxLayout layout_8x8x4 =
{
	8,8,
	RGN_FRAC(1,2),
	4,
	{ STEP4(0,1) },
	{ RGN_FRAC(1,2)+1*4,RGN_FRAC(1,2)+0*4,1*4,0*4, RGN_FRAC(1,2)+3*4,RGN_FRAC(1,2)+2*4,3*4,2*4},
	{ STEP8(0,16) },
	8*8*4/2
};

/* 8x8x8 tiles in 4 roms */
static struct GfxLayout layout_8x8x8 =
{
	8,8,
	RGN_FRAC(1,4),
	8,
	{ STEP8(0,1) },
	{ RGN_FRAC(0,4) + 0*8, RGN_FRAC(1,4) + 0*8, RGN_FRAC(2,4) + 0*8, RGN_FRAC(3,4) + 0*8,
	  RGN_FRAC(0,4) + 1*8, RGN_FRAC(1,4) + 1*8, RGN_FRAC(2,4) + 1*8, RGN_FRAC(3,4) + 1*8 },
	{ STEP8(0,16) },
	8*8*8/4
};

static struct GfxDecodeInfo yunsung8_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &layout_8x8x8, 0, 0x08 }, /* [0] Tiles (Background)*/
	{ REGION_GFX2, 0, &layout_8x8x4, 0,	0x40 }, /* [1] Tiles (Text)*/
	{ -1 }
};



/***************************************************************************


								Machine Drivers


***************************************************************************/


static void yunsung8_adpcm_int(int irq)
{
	static int toggle=0;

	MSM5205_data_w (0,adpcm>>4);
	adpcm<<=4;

	toggle ^= 1;
	if (toggle)
		cpu_set_nmi_line(1,PULSE_LINE);
}

static struct YM3812interface yunsung8_ym3812_interface =
{
	1,
	4000000,	/* ? */
	{ 100 },
	{  0 },
};

struct MSM5205interface yunsung8_msm5205_interface =
{
	1,
	384000,
	{ yunsung8_adpcm_int },	/* interrupt function */
	{ MSM5205_S96_4B },		/* 4KHz, 4 Bits */
	{ 80 }
};


static MACHINE_DRIVER_START( yunsung8 )

	/* basic machine hardware */
	MDRV_CPU_ADD(Z80, 8000000)			/* Z80B */
	MDRV_CPU_MEMORY(yunsung8_readmem,yunsung8_writemem)
	MDRV_CPU_PORTS(yunsung8_readport,yunsung8_writeport)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,1)	/* No nmi routine */

	MDRV_CPU_ADD(Z80, 4000000)			/* ? */
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
	MDRV_CPU_MEMORY(yunsung8_sound_readmem,yunsung8_sound_writemem)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,1)	/* NMI caused by the MSM5205? */

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_MACHINE_INIT(yunsung8)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(512, 256)
	MDRV_VISIBLE_AREA(0+64, 512-64-1, 0+8, 256-8-1)
	MDRV_GFXDECODE(yunsung8_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(2048)

	MDRV_VIDEO_START(yunsung8)
	MDRV_VIDEO_UPDATE(yunsung8)

	/* sound hardware */
	MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
	MDRV_SOUND_ADD(YM3812, yunsung8_ym3812_interface)
	MDRV_SOUND_ADD(MSM5205, yunsung8_msm5205_interface)
MACHINE_DRIVER_END


/***************************************************************************


								ROMs Loading


***************************************************************************/

/***************************************************************************

									Magix

Yun Sung, 1995.
CPU : Z80B
SND : Z80A + YM3812 + Oki M5205
OSC : 16.000

***************************************************************************/

ROM_START( magix )

	ROM_REGION( 0x24000+0x4000, REGION_CPU1, 0 )		/* Main Z80 Code */
	ROM_LOAD( "yunsung8.07", 0x00000, 0x0c000, CRC(d4d0b68b) SHA1(d7e1fb57a14f8b822791b98cecc6d5a053a89e0f) )
	ROM_CONTINUE(         0x10000, 0x14000             )
	/* $2000 bytes for bank 0 of video ram (text) */
	/* $2000 bytes for bank 1 of video ram (background) */

	ROM_REGION( 0x24000, REGION_CPU2, 0 )		/* Sound Z80 Code */
	ROM_LOAD( "yunsung8.08", 0x00000, 0x0c000, CRC(6fd60be9) SHA1(87622dc2967842629e90a02b415bec86cc26cbc7) )
	ROM_CONTINUE(         0x10000, 0x14000             )

	ROM_REGION( 0x200000, REGION_GFX1, ROMREGION_DISPOSE )	/* Background */
	ROM_LOAD( "yunsung8.04",  0x000000, 0x80000, CRC(0a100d2b) SHA1(c36a2489748c8ac7b6d7457ad09d8153707c85be) )
	ROM_LOAD( "yunsung8.03",  0x080000, 0x80000, CRC(c8cb0373) SHA1(339c4e0fef44da3cab615e07dc8739bd925ebf28) )
	ROM_LOAD( "yunsung8.02",  0x100000, 0x80000, CRC(09efb8e5) SHA1(684bb5c4b579f8c77e79aab4decbefea495d9474) )
	ROM_LOAD( "yunsung8.01",  0x180000, 0x80000, CRC(4590d782) SHA1(af875166207793572b9ecf01bb6a24feba562a96) )

	ROM_REGION( 0x40000, REGION_GFX2, ROMREGION_DISPOSE )	/* Text */
	ROM_LOAD( "yunsung8.05", 0x00000, 0x20000, CRC(862d378c) SHA1(a4e2cf14b5b25c6b8725dd285ddea65ce9ee257a) )	/* only first $8000 bytes != 0*/
	ROM_LOAD( "yunsung8.06", 0x20000, 0x20000, CRC(8b2ab901) SHA1(1a5c05dd0cf830b645357a62d8e6e876b44c6b7f) )	/* only first $8000 bytes != 0*/

ROM_END


/***************************************************************************

								Cannon Ball

01, 02, 03, 04  are 27c020
05, 06, 07, 08  are 27c010
2 pals used

Z80b PROGRAM, Z80b SOUND

Cy7c384A
16MHz

***************************************************************************/

ROM_START( cannball )

	ROM_REGION( 0x24000+0x4000, REGION_CPU1, 0 )		/* Main Z80 Code */
	ROM_LOAD( "cannball.07", 0x00000, 0x0c000, CRC(17db56b4) SHA1(032e3dbde0b0e315dcb5f2b31f57e75e78818f2d) )
	ROM_CONTINUE(            0x10000, 0x14000             )
	/* $2000 bytes for bank 0 of video ram (text) */
	/* $2000 bytes for bank 1 of video ram (background) */

	ROM_REGION( 0x24000, REGION_CPU2, 0 )		/* Sound Z80 Code */
	ROM_LOAD( "cannball.08", 0x00000, 0x0c000, CRC(11403875) SHA1(9f583bc4f08e7aef3fd0f3fe3f31cce1d226641a) )
	ROM_CONTINUE(            0x10000, 0x14000             )

	ROM_REGION( 0x100000, REGION_GFX1, ROMREGION_DISPOSE )	/* Background */
	ROM_LOAD( "cannball.01",  0x000000, 0x40000, CRC(2d7785e4) SHA1(9911354c0be192506f8bfca3e85ede0bbc4828d5) )
	ROM_LOAD( "cannball.02",  0x040000, 0x40000, CRC(24df387e) SHA1(5f4afe11feb367ca3b3c4f5eb37a6b6c4edb83bb) )
	ROM_LOAD( "cannball.03",  0x080000, 0x40000, CRC(4d62f192) SHA1(8c60b9b4b36c13c2d145c49413580a10e71eb283) )
	ROM_LOAD( "cannball.04",  0x0c0000, 0x40000, CRC(37cf8b12) SHA1(f93df8e0babe2c4ec996aa3c2a48bf40a5a02e62) )

	ROM_REGION( 0x40000, REGION_GFX2, ROMREGION_DISPOSE )	/* Text */
	ROM_LOAD( "cannball.05", 0x00000, 0x20000, CRC(87c1f1fa) SHA1(dbc568d2133734e41b69fd8d18b76531648b32ef) )
	ROM_LOAD( "cannball.06", 0x20000, 0x20000, CRC(e722bee8) SHA1(3aed7df9df81a6776b6bf2f5b167965b0d689216) )

ROM_END


/***************************************************************************


								Game Drivers


***************************************************************************/

GAMEX( 1995, cannball, 0, yunsung8, cannball, 0, ROT0, "Yun Sung / Soft Vision", "Cannon Ball",  GAME_IMPERFECT_SOUND )
GAMEX( 1995, magix,    0, yunsung8, magix,    0, ROT0, "Yun Sung",               "Magix - Rock", GAME_IMPERFECT_SOUND ) /* Title: DSW*/
