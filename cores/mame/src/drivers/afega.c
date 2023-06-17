/***************************************************************************

							  -= Afega Games =-

					driver by	Luca Elia (l.elia@tin.it)


Main  CPU	:	M68000
Video Chips	:	AFEGA AFI-GFSK  (68 Pin PLCC)
				AFEGA AFI-GFLK (208 Pin PQFP)

Sound CPU	:	Z80
Sound Chips	:	M6295 (AD-65)  +  YM2151 (BS901)  +  YM3014 (BS90?)

---------------------------------------------------------------------------
Year + Game						Notes
---------------------------------------------------------------------------
97 Red Hawk                     US Version of Stagger 1
98 Sen Jin - Guardian Storm		Some text missing (protection, see service mode)
98 Stagger I
98 Bubble 2000                  By Tuning, but it seems to be the same HW
01 Fire Hawk					By ESD with different sound hardware: 2 M6295
---------------------------------------------------------------------------

The Sen Jin protection supplies some 68k code seen in the 2760-29cf range

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

/* Variables defined in vidhrdw: */

extern data16_t *afega_vram_0, *afega_scroll_0;
extern data16_t *afega_vram_1, *afega_scroll_1;

/* Functions defined in vidhrdw: */

WRITE16_HANDLER( afega_vram_0_w );
WRITE16_HANDLER( afega_vram_1_w );
WRITE16_HANDLER( afega_palette_w );

PALETTE_INIT( grdnstrm );

VIDEO_START( afega );
VIDEO_START( firehawk );
VIDEO_UPDATE( afega );
VIDEO_UPDATE( bubl2000 );
VIDEO_UPDATE( firehawk );


/***************************************************************************


							Memory Maps - Main CPU


***************************************************************************/

READ16_HANDLER( afega_unknown_r )
{
	/* This fixes the text in Service Mode. */
	return 0x0100;
}

WRITE16_HANDLER( afega_soundlatch_w )
{
	if (ACCESSING_LSB && Machine->sample_rate)
	{
		soundlatch_w(0,data&0xff);
		cpu_set_irq_line(1, 0, PULSE_LINE); /* Fire Hawk */
	}
}

static WRITE_HANDLER( spec2k_oki1_banking_w )
{
 	if(data == 0xfe)
 		OKIM6295_set_bank_base(1, 0);
 	else if(data == 0xff)
 		OKIM6295_set_bank_base(1, 0x40000);
}


/*
 Lines starting with an empty comment in the following MemoryReadAddress
 arrays are there for debug (e.g. the game does not read from those ranges
 AFAIK)
*/

static MEMORY_READ16_START( afega_readmem )
	{ 0x000000, 0x07ffff, MRA16_ROM					},	/* ROM*/
	{ 0x080000, 0x080001, input_port_0_word_r		},	/* Buttons*/
	{ 0x080002, 0x080003, input_port_1_word_r		},	/* P1 + P2*/
	{ 0x080004, 0x080005, input_port_2_word_r		},	/* 2 x DSW*/
	{ 0x080012, 0x080013, afega_unknown_r           },
/**/{ 0x088000, 0x0885ff, MRA16_RAM					},	/* Palette*/
/**/{ 0x08c000, 0x08c003, MRA16_RAM					},	/* Scroll*/
/**/{ 0x08c004, 0x08c007, MRA16_RAM					},	/**/
/**/{ 0x090000, 0x091fff, MRA16_RAM					},	/* Layer 0*/
/**/{ 0x092000, 0x093fff, MRA16_RAM           		},	/* ?*/
/**/{ 0x09c000, 0x09c7ff, MRA16_RAM					},	/* Layer 1*/
	{ 0x3c0000, 0x3c7fff, MRA16_RAM					},	/* RAM*/
	{ 0x3c8000, 0x3c8fff, MRA16_RAM					},	/* Sprites*/
	{ 0x3c9000, 0x3cffff, MRA16_RAM					},	/* RAM*/
	{ 0xff8000, 0xff8fff, MRA16_BANK1				},	/* Sprites Mirror*/
MEMORY_END

static MEMORY_WRITE16_START( afega_writemem )
	{ 0x000000, 0x07ffff, MWA16_ROM						},	/* ROM*/
	{ 0x080000, 0x08001d, MWA16_RAM						},	/**/
	{ 0x08001e, 0x08001f, afega_soundlatch_w			},	/* To Sound CPU*/
	{ 0x080020, 0x087fff, MWA16_RAM						},	/**/
	{ 0x088000, 0x0885ff, afega_palette_w, &paletteram16},	/* Palette*/
	{ 0x088600, 0x08bfff, MWA16_RAM						},	/**/
	{ 0x08c000, 0x08c003, MWA16_RAM, &afega_scroll_0	},	/* Scroll*/
	{ 0x08c004, 0x08c007, MWA16_RAM, &afega_scroll_1	},	/**/
	{ 0x08c008, 0x08ffff, MWA16_RAM						},	/**/
	{ 0x090000, 0x091fff, afega_vram_0_w, &afega_vram_0	},	/* Layer 0*/
	{ 0x092000, 0x093fff, MWA16_RAM						},	/* ?*/
	{ 0x09c000, 0x09c7ff, afega_vram_1_w, &afega_vram_1	},	/* Layer 1*/
	{ 0x3c0000, 0x3c7fff, MWA16_RAM						},	/* RAM*/
	{ 0x3c8000, 0x3c8fff, MWA16_RAM, &spriteram16, &spriteram_size	},	/* Sprites*/
	{ 0x3c9000, 0x3cffff, MWA16_RAM						},	/* RAM*/
	{ 0xff8000, 0xff8fff, MWA16_BANK1				},	/* Sprites Mirror*/
MEMORY_END

/* redhawk has main ram / sprites in a different location */

static MEMORY_READ16_START( redhawk_readmem )
	{ 0x000000, 0x07ffff, MRA16_ROM					},	/* ROM*/
	{ 0x080000, 0x080001, input_port_0_word_r		},	/* Buttons*/
	{ 0x080002, 0x080003, input_port_1_word_r		},	/* P1 + P2*/
	{ 0x080004, 0x080005, input_port_2_word_r		},	/* 2 x DSW*/
/**/{ 0x088000, 0x0885ff, MRA16_RAM					},	/* Palette*/
/**/{ 0x08c000, 0x08c003, MRA16_RAM					},	/* Scroll*/
/**/{ 0x08c004, 0x08c007, MRA16_RAM					},	/**/
/**/{ 0x090000, 0x091fff, MRA16_RAM					},	/* Layer 0*/
/**/{ 0x092000, 0x093fff, MRA16_RAM					},	/* ?*/
/**/{ 0x09c000, 0x09c7ff, MRA16_RAM					},	/* Layer 1*/
	{ 0x0c0000, 0x0c7fff, MRA16_RAM					},	/* RAM*/
	{ 0x0c8000, 0x0c8fff, MRA16_RAM					},	/* Sprites*/
	{ 0x0c9000, 0x0cffff, MRA16_RAM					},	/* RAM*/
	{ 0xff8000, 0xff8fff, MRA16_BANK1				},	/* Sprites Mirror*/
MEMORY_END

static MEMORY_WRITE16_START( redhawk_writemem )
	{ 0x000000, 0x07ffff, MWA16_ROM						},	/* ROM*/
	{ 0x080000, 0x08001d, MWA16_RAM						},	/**/
	{ 0x08001e, 0x08001f, afega_soundlatch_w			},	/* To Sound CPU*/
	{ 0x080020, 0x087fff, MWA16_RAM						},	/**/
	{ 0x088000, 0x0885ff, afega_palette_w, &paletteram16},	/* Palette*/
	{ 0x088600, 0x08bfff, MWA16_RAM						},	/**/
	{ 0x08c000, 0x08c003, MWA16_RAM, &afega_scroll_0	},	/* Scroll*/
	{ 0x08c004, 0x08c007, MWA16_RAM, &afega_scroll_1	},	/**/
	{ 0x08c008, 0x08ffff, MWA16_RAM						},	/**/
	{ 0x090000, 0x091fff, afega_vram_0_w, &afega_vram_0	},	/* Layer 0*/
	{ 0x092000, 0x093fff, MWA16_RAM						},	/* ?*/
	{ 0x09c000, 0x09c7ff, afega_vram_1_w, &afega_vram_1	},	/* Layer 1*/
	{ 0x0c0000, 0x0c7fff, MWA16_RAM						},	/* RAM*/
	{ 0x0c8000, 0x0c8fff, MWA16_RAM, &spriteram16, &spriteram_size	},	/* Sprites*/
	{ 0x0c9000, 0x0cffff, MWA16_RAM						},	/* RAM*/
	{ 0xff8000, 0xff8fff, MWA16_BANK1				},	/* Sprites Mirror*/
MEMORY_END

/* firehawk has main ram / sprites / sprite banking in a different location */

static MEMORY_READ16_START( firehawk_readmem )
MEMORY_ADDRESS_BITS(20) /* required for the game to boot */
	{ 0x000000, 0x07ffff, MRA16_ROM					},	/* ROM*/
	{ 0x080000, 0x080001, input_port_0_word_r		},	/* Buttons*/
	{ 0x080002, 0x080003, input_port_1_word_r		},	/* P1 + P2*/
	{ 0x080004, 0x080005, input_port_2_word_r		},	/* 2 x DSW*/
/**/{ 0x088000, 0x0885ff, MRA16_RAM					},	/* Palette*/
/**/{ 0x08c000, 0x08c003, MRA16_RAM					},	/* Scroll*/
/**/{ 0x08c004, 0x08c007, MRA16_RAM					},	/**/
/**/{ 0x090000, 0x091fff, MRA16_RAM					},	/* Layer 0*/
/**/{ 0x092000, 0x093fff, MRA16_RAM					},	/* ?*/
/**/{ 0x09c000, 0x09c7ff, MRA16_RAM					},	/* Layer 1*/
	{ 0x0c0000, 0x0c7fff, MRA16_RAM					},	/* RAM*/
	{ 0x0c8000, 0x0c8fff, MRA16_RAM					},	/* Sprites*/
	{ 0x0c9000, 0x0cffff, MRA16_RAM					},	/* RAM*/
	{ 0x0f8000, 0x0f8fff, MRA16_BANK1				},	/* Sprites Mirror*/
MEMORY_END

static MEMORY_WRITE16_START( firehawk_writemem )
MEMORY_ADDRESS_BITS(20)
	{ 0x000000, 0x07ffff, MWA16_ROM						},	/* ROM*/
	{ 0x080000, 0x08001d, MWA16_RAM						},	/**/
	{ 0x08001e, 0x08001f, afega_soundlatch_w			},	/* To Sound CPU*/
	{ 0x080020, 0x087fff, MWA16_RAM						},	/**/
	{ 0x088000, 0x0885ff, afega_palette_w, &paletteram16},	/* Palette*/
	{ 0x088600, 0x08bfff, MWA16_RAM						},	/**/
	{ 0x08c000, 0x08c003, MWA16_RAM, &afega_scroll_0	},	/* Scroll*/
	{ 0x08c004, 0x08c007, MWA16_RAM, &afega_scroll_1	},	/**/
	{ 0x08c008, 0x08ffff, MWA16_RAM						},	/**/
	{ 0x090000, 0x091fff, afega_vram_0_w, &afega_vram_0	},	/* Layer 0*/
	{ 0x092000, 0x093fff, MWA16_RAM						},	/* ?*/
	{ 0x09c000, 0x09c7ff, afega_vram_1_w, &afega_vram_1	},	/* Layer 1*/
	{ 0x0c0000, 0x0c7fff, MWA16_RAM						},	/* RAM*/
	{ 0x0c8000, 0x0c8fff, MWA16_RAM, &spriteram16, &spriteram_size	},	/* Sprites*/
	{ 0x0c9000, 0x0cffff, MWA16_RAM						},	/* RAM*/
	{ 0x0f8000, 0x0f8fff, MWA16_BANK1				},	/* Sprites Mirror*/
MEMORY_END

/***************************************************************************


							Memory Maps - Sound CPU


***************************************************************************/

static MEMORY_READ_START( afega_sound_readmem )
	{ 0x0000, 0xefff, MRA_ROM					},	/* ROM*/
	{ 0xf000, 0xf7ff, MRA_RAM					},	/* RAM*/
	{ 0xf800, 0xf800, soundlatch_r				},	/* From Main CPU*/
	{ 0xf809, 0xf809, YM2151_status_port_0_r	},	/* YM2151*/
	{ 0xf80a, 0xf80a, OKIM6295_status_0_r		},	/* M6295*/
MEMORY_END

static MEMORY_WRITE_START( afega_sound_writemem )
	{ 0x0000, 0xefff, MWA_ROM					},	/* ROM*/
	{ 0xf000, 0xf7ff, MWA_RAM					},	/* RAM*/
	{ 0xf808, 0xf808, YM2151_register_port_0_w	},	/* YM2151*/
	{ 0xf809, 0xf809, YM2151_data_port_0_w		},	/**/
	{ 0xf80a, 0xf80a, OKIM6295_data_0_w			},	/* M6295*/
MEMORY_END

static MEMORY_READ_START( firehawk_sound_readmem )
    { 0x0000, 0xefff, MRA_ROM },
	{ 0xf000, 0xf7ff, MRA_RAM },
	{ 0xfff0, 0xfff0, soundlatch_r },
	{ 0xfff8, 0xfff8, OKIM6295_status_1_r },
	{ 0xfffa, 0xfffa, OKIM6295_status_0_r },
	{ 0xf800, 0xffff, MRA_RAM }, /* not used, only tested */
MEMORY_END

static MEMORY_WRITE_START( firehawk_sound_writemem )
    { 0x0000, 0xefff, MWA_ROM },
	{ 0xf000, 0xf7ff, MWA_RAM },
	{ 0xfff2, 0xfff2, spec2k_oki1_banking_w },
	{ 0xfff8, 0xfff8, OKIM6295_data_1_w },
	{ 0xfffa, 0xfffa, OKIM6295_data_0_w },
	{ 0xf800, 0xffff, MWA_RAM }, /* not used, only  tested */
MEMORY_END

/***************************************************************************


								Input Ports


***************************************************************************/

/***************************************************************************
								Stagger I
***************************************************************************/

INPUT_PORTS_START( stagger1 )
	PORT_START	/* IN0 - $080000.w*/
	PORT_BIT(  0x0001, IP_ACTIVE_LOW, IPT_COIN1    )
	PORT_BIT(  0x0002, IP_ACTIVE_LOW, IPT_COIN2    )
	PORT_BIT(  0x0004, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT(  0x0008, IP_ACTIVE_LOW, IPT_START1   )
	PORT_BIT(  0x0010, IP_ACTIVE_LOW, IPT_START2   )
	PORT_BIT(  0x0020, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT(  0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT(  0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT(  0xff00, IP_ACTIVE_LOW, IPT_UNKNOWN  )

	PORT_START	/* IN1 - $080002.w*/
	PORT_BIT(  0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER1 )
	PORT_BIT(  0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER1 )
	PORT_BIT(  0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER1 )
	PORT_BIT(  0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER1 )
	PORT_BIT(  0x0010, IP_ACTIVE_LOW, IPT_BUTTON1        | IPF_PLAYER1 )
	PORT_BIT(  0x0020, IP_ACTIVE_LOW, IPT_BUTTON2        | IPF_PLAYER1 )
	PORT_BIT(  0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_BIT(  0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER2 )
	PORT_BIT(  0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER2 )
	PORT_BIT(  0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER2 )
	PORT_BIT(  0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER2 )
	PORT_BIT(  0x1000, IP_ACTIVE_LOW, IPT_BUTTON1        | IPF_PLAYER2 )
	PORT_BIT(  0x2000, IP_ACTIVE_LOW, IPT_BUTTON2        | IPF_PLAYER2 )
	PORT_BIT(  0x4000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* IN2 - $080004.w*/
	PORT_SERVICE( 0x0001, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x0002, 0x0002, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( On ) )
	PORT_DIPNAME( 0x0004, 0x0004, DEF_STR( Unused ) )
	PORT_DIPSETTING(      0x0004, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0008, 0x0008, DEF_STR( Unused ) )
	PORT_DIPSETTING(      0x0008, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0010, 0x0010, DEF_STR( Unused ) )
	PORT_DIPSETTING(      0x0010, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0020, 0x0020, DEF_STR( Unused ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x00c0, 0x00c0, DEF_STR( Lives ) )
	PORT_DIPSETTING(      0x0000, "1" )
	PORT_DIPSETTING(      0x0080, "2" )
	PORT_DIPSETTING(      0x00c0, "3" )
	PORT_DIPSETTING(      0x0040, "5" )

	PORT_DIPNAME( 0x0300, 0x0300, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(      0x0300, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPSETTING(      0x0200, "Horizontally" )
	PORT_DIPSETTING(      0x0100, "Vertically" )
	PORT_DIPNAME( 0x0400, 0x0400, DEF_STR( Unused ) )
	PORT_DIPSETTING(      0x0400, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x1800, 0x1800, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(      0x0800, "Easy" )
	PORT_DIPSETTING(      0x1800, "Normal" )
	PORT_DIPSETTING(      0x1000, "Hard" )
	PORT_DIPSETTING(      0x0000, "Hardest" )
	PORT_DIPNAME( 0xe000, 0xe000, DEF_STR( Coinage ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(      0x8000, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x4000, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0xc000, DEF_STR( 3C_2C ) )
	PORT_DIPSETTING(      0xe000, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x2000, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(      0x6000, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0xa000, DEF_STR( 1C_3C ) )
INPUT_PORTS_END


/***************************************************************************
							Sen Jin - Guardian Storm
***************************************************************************/

INPUT_PORTS_START( grdnstrm )
	PORT_START	/* IN0 - $080000.w*/
	PORT_BIT(  0x0001, IP_ACTIVE_LOW, IPT_COIN1    )
	PORT_BIT(  0x0002, IP_ACTIVE_LOW, IPT_COIN2    )
	PORT_BIT(  0x0004, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT(  0x0008, IP_ACTIVE_LOW, IPT_START1   )
	PORT_BIT(  0x0010, IP_ACTIVE_LOW, IPT_START2   )
	PORT_BIT(  0x0020, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT(  0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT(  0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT(  0xff00, IP_ACTIVE_LOW, IPT_UNKNOWN  )

	PORT_START	/* IN1 - $080002.w*/
	PORT_BIT(  0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER1 )
	PORT_BIT(  0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER1 )
	PORT_BIT(  0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER1 )
	PORT_BIT(  0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER1 )
	PORT_BIT(  0x0010, IP_ACTIVE_LOW, IPT_BUTTON1        | IPF_PLAYER1 )
	PORT_BIT(  0x0020, IP_ACTIVE_LOW, IPT_BUTTON2        | IPF_PLAYER1 )
	PORT_BIT(  0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_BIT(  0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER2 )
	PORT_BIT(  0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER2 )
	PORT_BIT(  0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER2 )
	PORT_BIT(  0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER2 )
	PORT_BIT(  0x1000, IP_ACTIVE_LOW, IPT_BUTTON1        | IPF_PLAYER2 )
	PORT_BIT(  0x2000, IP_ACTIVE_LOW, IPT_BUTTON2        | IPF_PLAYER2 )
	PORT_BIT(  0x4000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* IN2 - $080004.w*/
	PORT_SERVICE( 0x0001, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x0002, 0x0002, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( On ) )
	PORT_DIPNAME( 0x0004, 0x0004, DEF_STR( Free_Play ) )
	PORT_DIPSETTING(      0x0004, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0008, 0x0008, "Bombs" )
	PORT_DIPSETTING(      0x0008, "2" )
	PORT_DIPSETTING(      0x0000, "3" )
	PORT_DIPNAME( 0x0010, 0x0010, DEF_STR( Unused ) )
	PORT_DIPSETTING(      0x0010, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0020, 0x0020, DEF_STR( Unused ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x00c0, 0x00c0, DEF_STR( Lives ) )
	PORT_DIPSETTING(      0x0000, "1" )
	PORT_DIPSETTING(      0x0080, "2" )
	PORT_DIPSETTING(      0x00c0, "3" )
	PORT_DIPSETTING(      0x0040, "5" )

	PORT_DIPNAME( 0x0300, 0x0300, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(      0x0300, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPSETTING(      0x0200, "Horizontally" )
	PORT_DIPSETTING(      0x0100, "Vertically" )
	PORT_DIPNAME( 0x0400, 0x0400, DEF_STR( Unused ) )
	PORT_DIPSETTING(      0x0400, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x1800, 0x1800, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(      0x0800, "Easy" )
	PORT_DIPSETTING(      0x1800, "Normal" )
	PORT_DIPSETTING(      0x1000, "Hard" )
	PORT_DIPSETTING(      0x0000, "Hardest" )
	PORT_DIPNAME( 0xe000, 0xe000, DEF_STR( Coinage ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(      0x8000, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x4000, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0xc000, DEF_STR( 3C_2C ) )
	PORT_DIPSETTING(      0xe000, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x2000, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(      0x6000, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0xa000, DEF_STR( 1C_3C ) )
INPUT_PORTS_END


/***************************************************************************
								Stagger I
***************************************************************************/

INPUT_PORTS_START( bubl2000 )
	PORT_START	/* IN0 - $080000.w*/
	PORT_BIT(  0x0001, IP_ACTIVE_LOW, IPT_COIN1    )
	PORT_BIT(  0x0002, IP_ACTIVE_LOW, IPT_COIN2    )
	PORT_BIT(  0x0004, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT(  0x0008, IP_ACTIVE_LOW, IPT_START1   )
	PORT_BIT(  0x0010, IP_ACTIVE_LOW, IPT_START2   )
	PORT_BIT(  0x0020, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT(  0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT(  0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT(  0xff00, IP_ACTIVE_LOW, IPT_UNKNOWN  )

	PORT_START	/* IN1 - $080002.w*/
	PORT_BIT(  0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER1 )
	PORT_BIT(  0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER1 )
	PORT_BIT(  0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER1 )
	PORT_BIT(  0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER1 )
	PORT_BIT(  0x0010, IP_ACTIVE_LOW, IPT_BUTTON1        | IPF_PLAYER1 )
	PORT_BIT(  0x0020, IP_ACTIVE_LOW, IPT_BUTTON2        | IPF_PLAYER1 )
	PORT_BIT(  0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_BIT(  0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER2 )
	PORT_BIT(  0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER2 )
	PORT_BIT(  0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER2 )
	PORT_BIT(  0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER2 )
	PORT_BIT(  0x1000, IP_ACTIVE_LOW, IPT_BUTTON1        | IPF_PLAYER2 )
	PORT_BIT(  0x2000, IP_ACTIVE_LOW, IPT_BUTTON2        | IPF_PLAYER2 )
	PORT_BIT(  0x4000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* IN2 - $080004.w*/
	PORT_DIPNAME( 0x0001, 0x0001, DEF_STR( Unused ) )
	PORT_DIPSETTING(      0x0001, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0002, 0x0002, DEF_STR( Unused ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x000c, 0x000c, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(      0x0008, "Easy" )
	PORT_DIPSETTING(      0x000c, "Normal" )
	PORT_DIPSETTING(      0x0004, "Hard" )
	PORT_DIPSETTING(      0x0000, "Hardest" )
	PORT_DIPNAME( 0x0010, 0x0010, DEF_STR( Unused ) )
	PORT_DIPSETTING(      0x0010, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0020, 0x0020, DEF_STR( Unused ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x00c0, 0x00c0, "Free Credit" )
	PORT_DIPSETTING(      0x0080, "500k" )
	PORT_DIPSETTING(      0x00c0, "800k" )
	PORT_DIPSETTING(      0x0040, "1000k" )
	PORT_DIPSETTING(      0x0000, "1500k" )

	PORT_DIPNAME( 0x0100, 0x0100, DEF_STR( Unused ) )
	PORT_DIPSETTING(      0x0100, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0200, 0x0200, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0200, DEF_STR( On ) )
	PORT_DIPNAME( 0x1c00, 0x1c00, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(      0x1000, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(      0x0800, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x1800, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x1c00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0c00, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x1400, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x0400, DEF_STR( 1C_4C ) )
/*	PORT_DIPSETTING(      0x0000, "Disabled" )*/
	PORT_DIPNAME( 0xe000, 0xe000, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(      0x8000, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(      0x4000, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0xc000, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0xe000, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x6000, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0xa000, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x2000, DEF_STR( 1C_4C ) )
/*	PORT_DIPSETTING(      0x0000, "Disabled" )*/
INPUT_PORTS_END

/***************************************************************************
								Fire Hawk
***************************************************************************/

INPUT_PORTS_START( firehawk )
	PORT_START	/* IN0 - $080000.w */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT( 0xff00, IP_ACTIVE_LOW, IPT_UNKNOWN  )

	PORT_START	/* IN1 - $080002.w */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER1 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER1 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER1 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER1 )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER2 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER2 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER2 )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER2 )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN  )

	PORT_START 	/* IN2 - $080004.w */
	PORT_DIPNAME( 0x0001, 0x0001, "Show Dip-Switch Settings" )
	PORT_DIPSETTING(      0x0001, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x000e, 0x000e, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(      0x0006, "Very Easy" )
	PORT_DIPSETTING(      0x0008, "Easy" )
	PORT_DIPSETTING(      0x000e, "Normal" )
	PORT_DIPSETTING(      0x0002, "Hard" )
	PORT_DIPSETTING(      0x0004, "Hardest" )
	PORT_DIPSETTING(      0x000c, "Very Hard" )
	PORT_DIPNAME( 0x0010, 0x0010, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0010, DEF_STR( On ) )
	PORT_DIPNAME( 0x0020, 0x0020, "Number of Bombs" )
	PORT_DIPSETTING(      0x0020, "2" )
	PORT_DIPSETTING(      0x0000, "3" )
	PORT_DIPNAME( 0x00c0, 0x00c0, DEF_STR( Lives ) )
	PORT_DIPSETTING(      0x0000, "1" )
	PORT_DIPSETTING(      0x0080, "2" )
	PORT_DIPSETTING(      0x00c0, "3" )
	PORT_DIPSETTING(      0x0040, "4" )
	PORT_DIPNAME( 0x0100, 0x0100, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0100, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0200, 0x0200, "Region" )
	PORT_DIPSETTING(      0x0200, "English" )
	PORT_DIPSETTING(      0x0000, "China" )
	PORT_DIPNAME( 0x0400, 0x0400, DEF_STR( Free_Play ) )
	PORT_DIPSETTING(      0x0400, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x1800, 0x1800, "Continue Coins" )
	PORT_DIPSETTING(      0x1800, "1 Coin" )
	PORT_DIPSETTING(      0x0800, "2 Coins" )
	PORT_DIPSETTING(      0x1000, "3 Coins" )
	PORT_DIPSETTING(      0x0000, "4 Coins" )
	PORT_DIPNAME( 0xe000, 0xe000, DEF_STR( Coinage ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(      0x8000, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x4000, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0xc000, DEF_STR( 3C_2C ) )
	PORT_DIPSETTING(      0xe000, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x2000, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(      0x6000, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0xa000, DEF_STR( 1C_3C ) )
INPUT_PORTS_END

INPUT_PORTS_START( spec2k )
	PORT_START	/* IN0 - $080000.w */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT( 0xff00, IP_ACTIVE_LOW, IPT_UNKNOWN  )

	PORT_START	/* IN1 - $080002.w */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER1 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER1 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER1 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER1 )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER2 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER2 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER2 )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER2 )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN  )

	PORT_START 	/* IN2 - $080004.w */
	PORT_SERVICE(  0x01, IP_ACTIVE_LOW  )
	PORT_DIPNAME( 0x0002, 0x0002, DEF_STR( Demo_Sounds ) )	
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( On ) )
	PORT_DIPNAME( 0x0004, 0x0004, DEF_STR( Free_Play ) )
	PORT_DIPSETTING(      0x0004, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0008, 0x0008, "Number of Bombs" )	
	PORT_DIPSETTING(      0x0008, "2" )
	PORT_DIPSETTING(      0x0000, "3" )
	PORT_DIPNAME( 0x0010, 0x0010, "Copyright Notice" )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0010, DEF_STR( On ) )
	PORT_DIPNAME( 0x0020, 0x0020, DEF_STR( Unknown ) )	
	PORT_DIPSETTING(      0x0020, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x00c0, 0x00c0, DEF_STR( Lives ) )	
	PORT_DIPSETTING(      0x0000, "1" )
	PORT_DIPSETTING(      0x0080, "2" )
	PORT_DIPSETTING(      0x00c0, "3" )
	PORT_DIPSETTING(      0x0040, "5" )
	PORT_DIPNAME( 0x0100, 0x0100, DEF_STR( Unknown ) )	
	PORT_DIPSETTING(      0x0100, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0200, 0x0200, DEF_STR( Unknown ) )	
	PORT_DIPSETTING(      0x0200, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0400, 0x0400, DEF_STR( Unknown ) )	
	PORT_DIPSETTING(      0x0400, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x1800, 0x1800, DEF_STR( Difficulty ) )		
	PORT_DIPSETTING(      0x0800, "Easy" )
	PORT_DIPSETTING(      0x1800, "Normal" )
	PORT_DIPSETTING(      0x1000, "Hard" )
	PORT_DIPSETTING(      0x0000, "Hardest" )
	PORT_DIPNAME( 0xe000, 0xe000, DEF_STR( Coinage ) )	
	PORT_DIPSETTING(      0x0000, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(      0x8000, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x4000, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0xc000, DEF_STR( 3C_2C ) )
	PORT_DIPSETTING(      0xe000, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x2000, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(      0x6000, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0xa000, DEF_STR( 1C_3C ) )
INPUT_PORTS_END

/***************************************************************************


							Graphics Layouts


***************************************************************************/

static struct GfxLayout layout_8x8x4 =
{
	8,8,
	RGN_FRAC(1,1),
	4,
	{ STEP4(0,1)	},
	{ STEP8(0,4)	},
	{ STEP8(0,8*4)	},
	8*8*4
};

static struct GfxLayout layout_16x16x4 =
{
	16,16,
	RGN_FRAC(1,1),
	4,
	{ STEP4(0,1)	},
	{ STEP8(0,4),   STEP8(8*8*4*2,4)	},
	{ STEP8(0,8*4), STEP8(8*8*4*1,8*4)	},
	16*16*4
};

static struct GfxLayout layout_16x16x4_2 =
{
	16,16,
	RGN_FRAC(1,1),
	4,
	{ STEP4(0,1)	},
	{ STEP8(0,4),   STEP8(8*8*4*2,4)	},
	{ STEP8(0,8*4), STEP8(8*8*4*1,8*4)	},
	16*16*4
};

static struct GfxLayout layout_16x16x8 =
{
	16,16,
	RGN_FRAC(1,2),
	8,
	{ STEP4(RGN_FRAC(0,2),1), STEP4(RGN_FRAC(1,2),1)	},
	{ STEP8(0,4),   STEP8(8*8*4*2,4)	},
	{ STEP8(0,8*4), STEP8(8*8*4*1,8*4)	},
	16*16*4
};


static struct GfxDecodeInfo grdnstrm_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &layout_16x16x4, 256*1, 16 }, /* [0] Sprites*/
	{ REGION_GFX2, 0, &layout_16x16x8, 256*3, 16 }, /* [1] Layer 0*/
	{ REGION_GFX3, 0, &layout_8x8x4,   256*2, 16 }, /* [2] Layer 1*/
	{ -1 }
};

static struct GfxDecodeInfo stagger1_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &layout_16x16x4_2, 256*1, 16 }, /* [0] Sprites*/
	{ REGION_GFX2, 0, &layout_16x16x4,   256*0, 16 }, /* [1] Layer 0*/
	{ REGION_GFX3, 0, &layout_8x8x4,     256*2, 16 }, /* [2] Layer 1*/
	{ -1 }
};


/***************************************************************************


								Machine Drivers


***************************************************************************/

static void irq_handler(int irq)
{
	cpu_set_irq_line(1,0,irq ? ASSERT_LINE : CLEAR_LINE);
}

static struct YM2151interface afega_ym2151_intf =
{
	1,
	4000000,	/* ? */
	{ YM3012_VOL(30,MIXER_PAN_LEFT,30,MIXER_PAN_RIGHT) },
	{ irq_handler }
};

static struct OKIM6295interface afega_m6295_intf =
{
	1,
	{ 8000 },	/* ? */
	{ REGION_SOUND1 },
	{ 70 }
};

static struct OKIM6295interface firehawk_m6295_interface =
{
	2,									/* 2 chips */
	{ 1000000/132, 1000000/132 },		/* frequency (Hz) */
	{ REGION_SOUND1, REGION_SOUND2 },	/* memory region */
	{ 100, 100 }
};



INTERRUPT_GEN( interrupt_afega )
{
	switch ( cpu_getiloops() )
	{
		case 0:		irq2_line_hold();	break;
		case 1:		irq4_line_hold();	break;
	}
}

MACHINE_INIT( afega )
{
	/* Sprites Mirror required due to bug in the game code ( movem.w instead of movem.l ) */
	cpu_setbank( 1, spriteram16 );
}

static MACHINE_DRIVER_START( stagger1 )

	/* basic machine hardware */
	MDRV_CPU_ADD_TAG("main",M68000,10000000)	/* 3.072 MHz */
	MDRV_CPU_MEMORY(afega_readmem,afega_writemem)
	MDRV_CPU_VBLANK_INT(interrupt_afega,2)

	MDRV_CPU_ADD(Z80, 3000000)
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)	/* ? */
	MDRV_CPU_MEMORY(afega_sound_readmem,afega_sound_writemem)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_MACHINE_INIT(afega)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(256, 256)
	MDRV_VISIBLE_AREA(0, 256-1, 0+16, 256-16-1)
	MDRV_GFXDECODE(stagger1_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(768)

	MDRV_VIDEO_START(afega)
	MDRV_VIDEO_UPDATE(afega)

	/* sound hardware */
	MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
	MDRV_SOUND_ADD(YM2151, afega_ym2151_intf)
	MDRV_SOUND_ADD(OKIM6295, afega_m6295_intf)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( redhawk )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(stagger1)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_MEMORY(redhawk_readmem,redhawk_writemem)

MACHINE_DRIVER_END

static MACHINE_DRIVER_START( grdnstrm )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 10000000)
	MDRV_CPU_MEMORY(afega_readmem,afega_writemem)
	MDRV_CPU_VBLANK_INT(interrupt_afega,2)

	MDRV_CPU_ADD(Z80, 3000000)
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)	/* ? */
	MDRV_CPU_MEMORY(afega_sound_readmem,afega_sound_writemem)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_MACHINE_INIT(afega)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(256, 256)
	MDRV_VISIBLE_AREA(0, 256-1, 0+16, 256-16-1)
	MDRV_GFXDECODE(grdnstrm_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(768)
	MDRV_COLORTABLE_LENGTH(768 + 16*256)

	MDRV_PALETTE_INIT(grdnstrm)
	MDRV_VIDEO_START(afega)
	MDRV_VIDEO_UPDATE(afega)

	/* sound hardware */
	MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
	MDRV_SOUND_ADD(YM2151, afega_ym2151_intf)
	MDRV_SOUND_ADD(OKIM6295, afega_m6295_intf)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( bubl2000 )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(grdnstrm)

	/* video hardware */
	MDRV_VIDEO_UPDATE(bubl2000)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( firehawk )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000,12000000)
	MDRV_CPU_MEMORY(firehawk_readmem,firehawk_writemem)
	MDRV_CPU_VBLANK_INT(interrupt_afega,2)

	MDRV_CPU_ADD(Z80,4000000)
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
	MDRV_CPU_MEMORY(firehawk_sound_readmem,firehawk_sound_writemem)

	MDRV_FRAMES_PER_SECOND(56)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)
	
	MDRV_MACHINE_INIT(afega)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(256, 256)
	MDRV_VISIBLE_AREA(8, 256-8-1, 16, 256-16-1)
	MDRV_GFXDECODE(grdnstrm_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(768)
	MDRV_COLORTABLE_LENGTH(768 + 16*256)

	MDRV_PALETTE_INIT(grdnstrm)
	MDRV_VIDEO_START(firehawk)
	MDRV_VIDEO_UPDATE(firehawk)

	/* sound hardware */
	MDRV_SOUND_ADD(OKIM6295, firehawk_m6295_interface)
MACHINE_DRIVER_END

/***************************************************************************


								ROMs Loading


***************************************************************************/

/* Address lines scrambling */

static void decryptcode( int a23, int a22, int a21, int a20, int a19, int a18, int a17, int a16, int a15, int a14, int a13, int a12,
	int a11, int a10, int a9, int a8, int a7, int a6, int a5, int a4, int a3, int a2, int a1, int a0 )
{
	int i;
	data8_t *RAM = memory_region( REGION_CPU1 );
	size_t  size = memory_region_length( REGION_CPU1 );
	data8_t *buffer = malloc( size );

	if( buffer )
	{
		memcpy( buffer, RAM, size );
		for( i = 0; i < size; i++ )
		{
			RAM[ i ] = buffer[ BITSWAP24( i, a23, a22, a21, a20, a19, a18, a17, a16, a15, a14, a13, a12,
				a11, a10, a9, a8, a7, a6, a5, a4, a3, a2, a1, a0 ) ];
		}
		free( buffer );
	}
}

/***************************************************************************

									Stagger I
(AFEGA 1998)

Parts:

1 MC68HC000P10
1 Z80
2 Lattice ispLSI 1032E

***************************************************************************/

ROM_START( stagger1 )
	ROM_REGION( 0x40000, REGION_CPU1, 0 )		/* 68000 Code */
	ROM_LOAD16_BYTE( "2.bin", 0x000000, 0x020000, CRC(8555929b) SHA1(b405d81c2a45191111b1a4458ac6b5c0a129b8f1) )
	ROM_LOAD16_BYTE( "3.bin", 0x000001, 0x020000, CRC(5b0b63ac) SHA1(239f793b6845a88d1630da790a2762da730a450d) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )		/* Z80 Code */
	ROM_LOAD( "1.bin", 0x00000, 0x10000, CRC(5d8cf28e) SHA1(2a440bf5136f95af137b6688e566a14e65be94b1) )

	ROM_REGION( 0x100000, REGION_GFX1, ROMREGION_DISPOSE )	/* Sprites, 16x16x4 */
	ROM_LOAD16_BYTE( "7.bin", 0x00000, 0x80000, CRC(048f7683) SHA1(7235b7dcfbb72abf44e60b114e3f504f16d29ebf) )
	ROM_LOAD16_BYTE( "6.bin", 0x00001, 0x80000, CRC(051d4a77) SHA1(664182748e72b3e44202caa20f337d02e946ca62) )

	ROM_REGION( 0x080000, REGION_GFX2, ROMREGION_DISPOSE )	/* Layer 0, 16x16x4 */
	ROM_LOAD( "4.bin", 0x00000, 0x80000, CRC(46463d36) SHA1(4265bc4d24ff64e39d9273965701c740d7e3fee0) )

	ROM_REGION( 0x00100, REGION_GFX3, ROMREGION_DISPOSE | ROMREGION_ERASEFF )	/* Layer 1, 8x8x4 */
	/* Unused*/

	ROM_REGION( 0x40000, REGION_SOUND1, ROMREGION_SOUNDONLY )	/* Samples */
	ROM_LOAD( "5", 0x00000, 0x40000, CRC(e911ce33) SHA1(a29c4dea98a22235122303325c63c15fadd3431d) )
ROM_END


/***************************************************************************

							Red Hawk (c)1997 Afega



    6116         ym2145(?)  MSM6295   5    4MHz
    1
    Z80        pLSI1032    4
                                      76C88
       6116   76C256                  76C88
       6116   76C256
    2  76C256 76C256
    3  76C256 76C256

    68000-10        6116
                    6116
   SW1                             6
   SW21                pLSI1032    7
    12MHz

***************************************************************************/

ROM_START( redhawk )
	ROM_REGION( 0x40000, REGION_CPU1, 0 )		/* 68000 Code */
	ROM_LOAD16_BYTE( "2", 0x000000, 0x020000, CRC(3ef5f326) SHA1(e89c7c24a05886a14995d7c399958dc00ad35d63) )
	ROM_LOAD16_BYTE( "3", 0x000001, 0x020000, CRC(9b3a10ef) SHA1(d03480329b23474e5a9e42a75b09d2140eed4443) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )		/* Z80 Code */
	ROM_LOAD( "1.bin", 0x00000, 0x10000, CRC(5d8cf28e) SHA1(2a440bf5136f95af137b6688e566a14e65be94b1) )

	ROM_REGION( 0x100000, REGION_GFX1, ROMREGION_DISPOSE )	/* Sprites, 16x16x4 */
	ROM_LOAD16_BYTE( "6", 0x000001, 0x080000, CRC(61560164) SHA1(d727ab2d037dab40745dec9c4389744534fdf07d) )
	ROM_LOAD16_BYTE( "7", 0x000000, 0x080000, CRC(66a8976d) SHA1(dd9b89cf29eb5557845599d55ef3a15f53c070a4) )

	ROM_REGION( 0x080000, REGION_GFX2, ROMREGION_DISPOSE )	/* Layer 0, 16x16x8 */
	ROM_LOAD( "4", 0x000000, 0x080000, CRC(d6427b8a) SHA1(556de1b5ce29d1c3c54bb315dcaa4dd0848ca462) )

	ROM_REGION( 0x00100, REGION_GFX3, ROMREGION_DISPOSE | ROMREGION_ERASEFF )	/* Layer 1, 8x8x4 */
	/* Unused*/

	ROM_REGION( 0x40000, REGION_SOUND1, ROMREGION_SOUNDONLY )	/* Samples */
	ROM_LOAD( "5", 0x00000, 0x40000, CRC(e911ce33) SHA1(a29c4dea98a22235122303325c63c15fadd3431d) )
ROM_END

static DRIVER_INIT( redhawk )
{
	decryptcode( 23, 22, 21, 20, 19, 18, 16, 15, 14, 17, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0 );
}

/***************************************************************************

							Sen Jin - Guardian Storm

(C) Afega 1998

CPU: 68HC000FN10 (68000, 68 pin PLCC)
SND: Z84C000FEC (Z80, 44 pin PQFP), AD-65 (44 pin PQFP, Probably OKI M6295),
     BS901 (Probably YM2151 or YM3812, 24 pin DIP), BS901 (possibly YM3014 or similar? 16 pin DIP)
OSC: 12.000MHz (near 68000), 4.000MHz (Near Z84000)
RAM: LH52B256 x 8, 6116 x 7
DIPS: 2 x 8 position

Other Chips: AFEGA AFI-GFSK (68 pin PLCC, located next to 68000)
             AFEGA AFI-GFLK (208 pin PQFP)

ROMS:
GST-01.U92   27C512, \
GST-02.U95   27C2000  > Sound Related, all located near Z80
GST-03.U4    27C512  /

GST-04.112   27C2000 \
GST-05.107   27C2000  /Main Program

GST-06.C13   read as 27C160  label = AF1-SP (Sprites?)
GST-07.C08   read as 27C160  label = AF1=B2 (Backgrounds?)
GST-08.C03   read as 27C160  label = AF1=B1 (Backgrounds?)

***************************************************************************/

ROM_START( grdnstrm )
	ROM_REGION( 0x80000, REGION_CPU1, 0 )		/* 68000 Code */
	ROM_LOAD16_BYTE( "gst-04.112", 0x000000, 0x040000, CRC(922c931a) SHA1(1d1511033c8c424535a73f5c5bf58560a8b1842e) )
	ROM_LOAD16_BYTE( "gst-05.107", 0x000001, 0x040000, CRC(d22ca2dc) SHA1(fa21c8ec804570d64f4b167b7f65fd5811435e46) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )		/* Z80 Code */
	ROM_LOAD( "gst-01.u92", 0x00000, 0x10000, CRC(5d8cf28e) SHA1(2a440bf5136f95af137b6688e566a14e65be94b1) )

	ROM_REGION( 0x200000, REGION_GFX1, ROMREGION_DISPOSE )	/* Sprites, 16x16x4 */
	ROM_LOAD( "gst-06.c13", 0x000000, 0x200000, CRC(7d4d4985) SHA1(15c6c1aecd3f12050c1db2376f929f1a26a1d1cf) )

	ROM_REGION( 0x400000, REGION_GFX2, ROMREGION_DISPOSE )	/* Layer 0, 16x16x8 */
	ROM_LOAD( "gst-07.c08", 0x000000, 0x200000, CRC(d68588c2) SHA1(c5f397d74a6ecfd2e375082f82e37c5a330fba62) )
	ROM_LOAD( "gst-08.c03", 0x200000, 0x200000, CRC(f8b200a8) SHA1(a6c43dd57b752d87138d7125b47dc0df83df8987) )

	ROM_REGION( 0x10000, REGION_GFX3, ROMREGION_DISPOSE )	/* Layer 1, 8x8x4 */
	ROM_LOAD( "gst-03.u4",  0x00000, 0x10000, CRC(a1347297) SHA1(583f4da991eeedeb523cf4fa3b6900d40e342063) )

	ROM_REGION( 0x40000, REGION_SOUND1, ROMREGION_SOUNDONLY )	/* Samples */
	ROM_LOAD( "gst-02.u95", 0x00000, 0x40000, CRC(e911ce33) SHA1(a29c4dea98a22235122303325c63c15fadd3431d) )
ROM_END

static DRIVER_INIT( grdnstrm )
{
	decryptcode( 23, 22, 21, 20, 19, 18, 16, 17, 14, 15, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0 );
}


/***************************************************************************

							Bubble 2000 (c)1998 Tuning

Bubble 2000
Tuning, 1998

CPU   : TMP68HC000P-10 (68000)
SOUND : Z840006 (Z80, 44 pin QFP), YM2151, OKI M6295
OSC   : 4.000MHZ, 12.000MHz
DIPSW : 8 position (x2)
RAM   : 6116 (x5, gfx related?) 6116 (x1, sound program ram), 6116 (x1, near rom3)
        64256 (x4, gfx related?), 62256 (x2, main program ram), 6264 (x2, gfx related?)
PALs/PROMs: None
Custom: Unknown 208 pin QFP labelled LTC2 (Graphics generator)
        Unknown 68 pin PLCC labelled LTC1 (?, near rom 2 and rom 3)
ROMs  :

Filename    Type        Possible Use
----------------------------------------------
rom01.92    27C512      Sound Program
rom02.95    27C020      Oki Samples
rom03.4     27C512      ? (located near rom 1 and 2 and near LTC1)
rom04.1     27C040   \
rom05.3     27C040    |
rom06.6     27C040    |
rom07.9     27C040    | Gfx
rom08.11    27C040    |
rom09.14    27C040    |
rom12.2     27C040    |
rom13.7     27C040   /

rom10.112   27C040   \  Main Program
rom11.107   27C040   /




***************************************************************************/

ROM_START( bubl2000 )
	ROM_REGION( 0x80000, REGION_CPU1, 0 )		/* 68000 Code */
	ROM_LOAD16_BYTE( "rom10.112", 0x00000, 0x20000, CRC(87f960d7) SHA1(d22fe1740217ac20963bd9003245850598ccecf2) )
	ROM_LOAD16_BYTE( "rom11.107", 0x00001, 0x20000, CRC(b386041a) SHA1(cac36e22a39b5be0c5cd54dce5c912ff811edb28) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )		/* Z80 Code */
	ROM_LOAD( "rom01.92", 0x00000, 0x10000, CRC(5d8cf28e) SHA1(2a440bf5136f95af137b6688e566a14e65be94b1) ) /* same as the other games on this driver */

	ROM_REGION( 0x080000, REGION_GFX1, ROMREGION_DISPOSE )	/* Sprites, 16x16x4 */
	ROM_LOAD16_BYTE( "rom08.11", 0x000000, 0x040000, CRC(519dfd82) SHA1(116b06f6e7b283a5417338f716bbaab6cfadb41d) )
	ROM_LOAD16_BYTE( "rom09.14", 0x000001, 0x040000, CRC(04fcb5c6) SHA1(7594fa6bf98fc01b8848473a222a621c7c9ff00d) )

	ROM_REGION( 0x300000, REGION_GFX2, ROMREGION_DISPOSE )	/* Layer 0, 16x16x8 */
	ROM_LOAD( "rom06.6",  0x000000, 0x080000, CRC(ac1aabf5) SHA1(abce6ba381b189ab3ec703a8ef74bccbe10876e0) )
	ROM_LOAD( "rom07.9",  0x080000, 0x080000, CRC(69aff769) SHA1(89b98c1023710861e622c8a186b6ec48f5109d42) )
	ROM_LOAD( "rom13.7",  0x100000, 0x080000, CRC(3a5b7226) SHA1(1127740c5bc2f830d73a77c8831e1b0db6606375) )
	ROM_LOAD( "rom04.1",  0x180000, 0x080000, CRC(46acd054) SHA1(1bd7a1b6b2ce6a3daa8c92843c546beb377af8fb) )
	ROM_LOAD( "rom05.3",  0x200000, 0x080000, CRC(37deb6a1) SHA1(3a8a3d961800bb15fd389429b92fa1e5b5f416df) )
	ROM_LOAD( "rom12.2",  0x280000, 0x080000, CRC(1fdc59dd) SHA1(d38e21c878241b4315a36e0590397211ca63f2c4) )

	ROM_REGION( 0x10000, REGION_GFX3, ROMREGION_DISPOSE )	/* Layer 1, 8x8x4 */
	ROM_LOAD( "rom03.4",  0x00000, 0x10000, CRC(f4c15588) SHA1(a21ae71c0a8c7c1df63f9905fd86303bc2d3991c) )

	ROM_REGION( 0x40000, REGION_SOUND1, ROMREGION_SOUNDONLY )	/* Samples */
	ROM_LOAD( "rom02.95", 0x00000, 0x40000, CRC(859a86e5) SHA1(7b51964227411a40aac54b9cd9ff64f091bdf2b0) )
ROM_END

static DRIVER_INIT( bubl2000 )
{
	decryptcode( 23, 22, 21, 20, 19, 18, 13, 14, 15, 16, 17, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0 );
}

/*
Fire Hawk - ESD
---------------
- To enter test mode, hold on button 1 at boot up
PCB Layout
----------
ESD-PROT-002
|------------------------------------------------|
|      FHAWK_S1.U40   FHAWK_S2.U36               |
|      6116     6295  FHAWK_S3.U41               |
|               6295                 FHAWK_G1.UC6|
|    PAL   Z80                       FHAWK_G2.UC5|
|    4MHz                             |--------| |
|                                     | ACTEL  | |
|J   6116             62256           |A54SX16A| |
|A   6116             62256           |        | |
|M                                    |(QFP208)| |
|M                                    |--------| |
|A     DSW1              FHAWK_G3.UC2            |
|      DSW2                           |--------| |
|      DSW3                           | ACTEL  | |
|                     6116            |A54SX16A| |
|                     6116            |        | |
|      62256                          |(QFP208)| |
| FHAWK_P1.U59                        |--------| |
| FHAWK_P2.U60  PAL                  62256  62256|
|                                                |
|12MHz 62256   68000                 62256  62256|
|------------------------------------------------|
Notes:
      68000 clock: 12.000MHz
        Z80 clock: 4.000MHz
      6295 clocks: 1.000MHz (both), sample rate = 1000000 / 132 (both)
            VSync: 56Hz
*/

ROM_START( firehawk )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )	/* 68000 Code */
	ROM_LOAD16_BYTE( "fhawk_p1.u59", 0x00001, 0x80000, CRC(d6d71a50) SHA1(e947720a0600d049b7ea9486442e1ba5582536c2) )
	ROM_LOAD16_BYTE( "fhawk_p2.u60", 0x00000, 0x80000, CRC(9f35d245) SHA1(5a22146f16bff7db924550970ed2a3048bc3edab) )

	ROM_REGION( 0x20000, REGION_CPU2, 0 )	/* Z80 Code */
	ROM_LOAD( "fhawk_s1.u40", 0x00000, 0x20000, CRC(c6609c39) SHA1(fe9b5f6c3ab42c48cb493fecb1181901efabdb58) )

	ROM_REGION( 0x200000, REGION_GFX1,ROMREGION_DISPOSE ) /* Sprites, 16x16x4 */
	ROM_LOAD( "fhawk_g3.uc2", 0x00000, 0x200000,  CRC(cae72ff4) SHA1(7dca7164015228ea039deffd234778d0133971ab) )

	ROM_REGION( 0x400000, REGION_GFX2, ROMREGION_DISPOSE ) /* Layer 0, 16x16x8 */
	ROM_LOAD( "fhawk_g1.uc6", 0x000000, 0x200000, CRC(2ab0b06b) SHA1(25362f6a517f188c62bac28b1a7b7b49622b1518) )
	ROM_LOAD( "fhawk_g2.uc5", 0x200000, 0x200000, CRC(d11bfa20) SHA1(15142004ab49f7f1e666098211dff0835c61df8d) )

	ROM_REGION( 0x00100, REGION_GFX3, ROMREGION_DISPOSE | ROMREGION_ERASEFF )	/* Layer 1, 8x8x4 */
	/* Unused */

	ROM_REGION( 0x040000, REGION_SOUND1, ROMREGION_SOUNDONLY ) /* Samples */
	ROM_LOAD( "fhawk_s2.u36", 0x00000, 0x40000, CRC(d16aaaad) SHA1(96ca173ca433164ed0ae51b41b42343bd3cfb5fe) )

	ROM_REGION( 0x040000, REGION_SOUND2, ROMREGION_SOUNDONLY ) /* Samples */
	ROM_LOAD( "fhawk_s3.u41", 0x00000, 0x40000, CRC(3fdcfac2) SHA1(c331f2ea6fd682cfb00f73f9a5b995408eaab5cf) )
ROM_END

ROM_START( spec2k )
	ROM_REGION( 0x80000, REGION_CPU1, 0 )	/* 68000 Code */
	ROM_LOAD16_BYTE( "yonatech5.u124", 0x00000, 0x40000, CRC(72ab5c05) SHA1(182a811982b89b8cda0677547ef0625c274f5c6b) )
	ROM_LOAD16_BYTE( "yonatech6.u120", 0x00001, 0x40000, CRC(7e44bd9c) SHA1(da59685be14a09ec037743fcec34fb293f7d588d) )

	ROM_REGION( 0x20000, REGION_CPU2, 0 )	/* Z80 Code */
	ROM_LOAD( "yonatech1.u103", 0x00000, 0x10000, CRC(ef5acda7) SHA1(e55b36a1598ecbbbad984997d61599dfa3958f60) )

	ROM_REGION( 0x200000, REGION_GFX1, ROMREGION_DISPOSE ) /* Sprites, 16x16x4 */
	ROM_LOAD( "u154.bin", 0x00000, 0x200000, CRC(f77b764e) SHA1(37e249bd4d7174c5232261880ce8debf42723716) ) /* UC1 MX29F1610ML Flash ROM */

	ROM_REGION( 0x400000, REGION_GFX2, ROMREGION_DISPOSE ) /* Layer 0, 16x16x8 */
	ROM_LOAD( "u153.bin", 0x000000, 0x200000, CRC(a00bbf8f) SHA1(622f52ef50d52cdd5e6b250d68439caae5c13404) ) /* UC2 MX29F1610ML Flash ROM */
	ROM_LOAD( "u152.bin", 0x200000, 0x200000, CRC(f6423fab) SHA1(253e0791eb58efa1df42e9c74d397e6e65c8c252) ) /* UC3 MX29F1610ML Flash ROM */

	ROM_REGION( 0x20000, REGION_GFX3, ROMREGION_ERASEFF )	/* Layer 1, 8x8x4 */
	ROM_LOAD( "yonatech4.u3", 0x00000, 0x20000, CRC(5626b08e) SHA1(63207ed6b4fc8684690bf3fe1991a4f3babd73e8) )

	ROM_REGION( 0x40000, REGION_SOUND1, ROMREGION_SOUNDONLY ) /* Samples */
	ROM_LOAD( "yonatech2.u101", 0x00000, 0x20000, CRC(4160f172) SHA1(0478a5a4bbba115e6cfb5501aa55aa2836c963bf) )

	ROM_REGION( 0x080000, REGION_SOUND2, ROMREGION_SOUNDONLY ) /* Samples */
	ROM_LOAD( "yonatech3.u106", 0x00000, 0x80000, CRC(6644c404) SHA1(b7ad3f9f08971432d024ef8be3fa3140f0bbae67) )
ROM_END

static DRIVER_INIT( spec2k )
{
	decryptcode( 23, 22, 21, 20, 19, 18, 17, 13, 14, 15, 16, 12, 11, 10, 9,  8, 7,  6,  5,  4, 3,  2,  1,  0 );
}

/***************************************************************************


								Game Drivers


***************************************************************************/

GAME ( 1998, stagger1, 0,        stagger1, stagger1, 0,        ROT270,             "Afega",    "Stagger I (Japan)" )
GAME ( 1997, redhawk,  stagger1, redhawk,  stagger1, redhawk,  ROT270,             "Afega",    "Red Hawk (US)" )
GAME ( 1998, grdnstrm, 0,        grdnstrm, grdnstrm, grdnstrm, ROT270,             "Afega",    "Sen Jin - Guardian Storm (Korea)" )
GAMEX( 1998, bubl2000, 0,        bubl2000, bubl2000, bubl2000, ROT0,               "Tuning",   "Bubble 2000", GAME_IMPERFECT_GRAPHICS )
GAME ( 2000, spec2k,   0,        firehawk, spec2k,   spec2k,   ORIENTATION_FLIP_Y, "Yonatech", "Spectrum 2000 (Euro)" )
GAME ( 2001, firehawk, 0,        firehawk, firehawk, 0,        ORIENTATION_FLIP_Y, "ESD",      "Fire Hawk" )
