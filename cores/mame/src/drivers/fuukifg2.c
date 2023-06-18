/***************************************************************************

						  -= Fuuki 16 Bit Games =-

					driver by	Luca Elia (l.elia@tin.it)


Main  CPU	:	M68000
Sound Chips	:	YM2203  +  YM3812  +  M6295
Video Chips	:	FI-002K (208pin PQFP, GA2)
				FI-003K (208pin PQFP, GA3)
Other		:	Mitsubishi M60067-0901FP 452100 (208pin PQFP, GA1)


---------------------------------------------------------------------------
Year + Game
---------------------------------------------------------------------------
95  Go Go! Mile Smile
96  Gyakuten!! Puzzle Bancho
---------------------------------------------------------------------------

To Do:

- Raster effects (level 5 interrupt is used for that). In pbancho
  they involve changing the *vertical* scroll value of the layers
  each scanline (when you are about to die, in the solo game).
  In gogomile they weave the water backgrounds and do some
  parallactic scrolling on later levels. *partly done, could do with
  some tweaking

- ADPCM samples banking in gogomile.

- The scroll values are generally wrong when flip screen is on and rasters are often incorrect

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

static int fuuki16_raster_enable = 1; /* Enabled by default */

/* Variables defined in vidhrdw: */

extern data16_t *fuuki16_vram_0, *fuuki16_vram_1;
extern data16_t *fuuki16_vram_2, *fuuki16_vram_3;
extern data16_t *fuuki16_vregs,  *fuuki16_priority, *fuuki16_unknown;

/* Functions defined in vidhrdw: */

WRITE16_HANDLER( fuuki16_vram_0_w );
WRITE16_HANDLER( fuuki16_vram_1_w );
WRITE16_HANDLER( fuuki16_vram_2_w );
WRITE16_HANDLER( fuuki16_vram_3_w );

VIDEO_START( fuuki16 );
VIDEO_UPDATE( fuuki16 );

/***************************************************************************


							Memory Maps - Main CPU


***************************************************************************/

static WRITE16_HANDLER( fuuki16_sound_command_w )
{
	if (ACCESSING_LSB)
	{
		soundlatch_w(0,data & 0xff);
		cpu_set_nmi_line(1,PULSE_LINE);
/*		cpu_spinuntil_time(TIME_IN_USEC(50));	*/ /* Allow the other CPU to reply*/
		cpu_boost_interleave(0, TIME_IN_USEC(50)); /* Fixes glitching in rasters*/
	}
}

static MEMORY_READ16_START( fuuki16_readmem )
	{ 0x000000, 0x0fffff, MRA16_ROM					},	/* ROM*/
	{ 0x400000, 0x40ffff, MRA16_RAM					},	/* RAM*/
	{ 0x500000, 0x507fff, MRA16_RAM					},	/* Layers*/
	{ 0x600000, 0x601fff, spriteram16_r				},	/* Sprites*/
	{ 0x608000, 0x609fff, spriteram16_r				},	/* Sprites (? Mirror ?)*/
	{ 0x700000, 0x703fff, MRA16_RAM					},	/* Palette*/
	{ 0x800000, 0x800001, input_port_0_word_r		},	/* Buttons (Inputs)*/
	{ 0x810000, 0x810001, input_port_1_word_r		},	/* P1 + P2*/
	{ 0x880000, 0x880001, input_port_2_word_r		},	/* 2 x DSW*/
	{ 0x8c0000, 0x8c001f, MRA16_RAM					},	/* Video Registers*/
/**/{ 0x8d0000, 0x8d0003, MRA16_RAM, 				},	/**/
/**/{ 0x8e0000, 0x8e0001, MRA16_RAM, 				},	/**/
MEMORY_END

static MEMORY_WRITE16_START( fuuki16_writemem )
	{ 0x000000, 0x0fffff, MWA16_ROM							},	/* ROM*/
	{ 0x400000, 0x40ffff, MWA16_RAM							},	/* RAM*/
	{ 0x500000, 0x501fff, fuuki16_vram_0_w, &fuuki16_vram_0	},	/* Layers*/
	{ 0x502000, 0x503fff, fuuki16_vram_1_w, &fuuki16_vram_1	},	/**/
	{ 0x504000, 0x505fff, fuuki16_vram_2_w, &fuuki16_vram_2	},	/**/
	{ 0x506000, 0x507fff, fuuki16_vram_3_w, &fuuki16_vram_3	},	/**/
	{ 0x506000, 0x507fff, MWA16_RAM							},	/**/
	{ 0x600000, 0x601fff, spriteram16_w, &spriteram16, &spriteram_size	},	/* Sprites*/
	{ 0x608000, 0x609fff, spriteram16_w						},	/* Sprites (? Mirror ?)*/
	{ 0x700000, 0x703fff, paletteram16_xRRRRRGGGGGBBBBB_word_w, &paletteram16	},	/* Palette*/
	{ 0x8c0000, 0x8c001f, MWA16_RAM, &fuuki16_vregs			},	/* Video Registers*/
	{ 0x8a0000, 0x8a0001, fuuki16_sound_command_w			},	/* To Sound CPU*/
	{ 0x8d0000, 0x8d0003, MWA16_RAM, &fuuki16_unknown		},	/**/
	{ 0x8e0000, 0x8e0001, MWA16_RAM, &fuuki16_priority		},	/**/
MEMORY_END


/***************************************************************************


							Memory Maps - Sound CPU


To do: ADPCM samples banking in gogomile.

***************************************************************************/

static WRITE_HANDLER( fuuki16_sound_rombank_w )
{
	if (data <= 2)
		cpu_setbank(1, memory_region(REGION_CPU2) + 0x8000 * data + 0x10000);
	else
	 	log_cb(RETRO_LOG_DEBUG, LOGPRE "CPU #1 - PC %04X: unknown bank bits: %02X\n",activecpu_get_pc(),data);
}

static MEMORY_READ_START( fuuki16_sound_readmem )
	{ 0x0000, 0x5fff, MRA_ROM		},	/* ROM*/
	{ 0x6000, 0x7fff, MRA_RAM		},	/* RAM*/
	{ 0x8000, 0xffff, MRA_BANK1		},	/* Banked ROM*/
MEMORY_END

static MEMORY_WRITE_START( fuuki16_sound_writemem )
	{ 0x0000, 0x5fff, MWA_ROM		},	/* ROM*/
	{ 0x6000, 0x7fff, MWA_RAM		},	/* RAM*/
	{ 0x8000, 0xffff, MWA_ROM		},	/* Banked ROM*/
MEMORY_END

static PORT_READ_START( fuuki16_sound_readport )
	{ 0x11, 0x11, soundlatch_r				},	/* From Main CPU*/
	{ 0x50, 0x50, YM3812_status_port_0_r	},	/* YM3812*/
	{ 0x60, 0x60, OKIM6295_status_0_r		},	/* M6295*/
PORT_END

static PORT_WRITE_START( fuuki16_sound_writeport )
	{ 0x00, 0x00, fuuki16_sound_rombank_w 	},	/* ROM Bank*/
	{ 0x11, 0x11, IOWP_NOP					},	/* ? To Main CPU*/
	{ 0x20, 0x20, IOWP_NOP					},	/* ? 0x10, 0x32, 0x54: 2 volumes ?*/
	{ 0x30, 0x30, IOWP_NOP					},	/* ? In the NMI routine*/
	{ 0x40, 0x40, YM2203_control_port_0_w	},	/* YM2203*/
	{ 0x41, 0x41, YM2203_write_port_0_w		},
	{ 0x50, 0x50, YM3812_control_port_0_w	},	/* YM3812*/
	{ 0x51, 0x51, YM3812_write_port_0_w		},
	{ 0x61, 0x61, OKIM6295_data_0_w			},	/* M6295*/
PORT_END


/***************************************************************************


								Input Ports


***************************************************************************/

INPUT_PORTS_START( gogomile )

	PORT_START	/* IN0 - $800000.w*/
	PORT_BIT(  0x0001, IP_ACTIVE_LOW, IPT_COIN1    )
	PORT_BIT(  0x0002, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT(  0x0004, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT(  0x0008, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT(  0x0010, IP_ACTIVE_LOW, IPT_START1   )
	PORT_BIT(  0x0020, IP_ACTIVE_LOW, IPT_START2   )
	PORT_BIT(  0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT(  0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT(  0x0100, IP_ACTIVE_LOW, IPT_COIN2    )
	PORT_BIT(  0xfe00, IP_ACTIVE_LOW, IPT_UNKNOWN  )

	PORT_START	/* IN1 - $810000.w*/
	PORT_BIT(  0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER1 )
	PORT_BIT(  0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER1 )
	PORT_BIT(  0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER1 )
	PORT_BIT(  0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER1 )
	PORT_BIT(  0x0010, IP_ACTIVE_LOW, IPT_BUTTON1        | IPF_PLAYER1 )
	PORT_BIT(  0x0020, IP_ACTIVE_LOW, IPT_UNKNOWN                      )	/* There's code that uses*/
	PORT_BIT(  0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN                      )	/* these unknown bits*/
	PORT_BIT(  0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN                      )

	PORT_BIT(  0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER2 )
	PORT_BIT(  0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER2 )
	PORT_BIT(  0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER2 )
	PORT_BIT(  0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER2 )
	PORT_BIT(  0x1000, IP_ACTIVE_LOW, IPT_BUTTON1        | IPF_PLAYER2 )
	PORT_BIT(  0x2000, IP_ACTIVE_LOW, IPT_UNKNOWN                      )
	PORT_BIT(  0x4000, IP_ACTIVE_LOW, IPT_UNKNOWN                      )
	PORT_BIT(  0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN                      )

	PORT_START	/* IN2 - $880000.w*/
	PORT_SERVICE( 0x0001, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x0002, 0x0002, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( On ) )
	PORT_DIPNAME( 0x000c, 0x000c, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(      0x0000, "Easy?"    )	/* 0*/
	PORT_DIPSETTING(      0x000c, "Normal"   )	/* 1*/
	PORT_DIPSETTING(      0x0008, "Hard?"    )	/* 2*/
	PORT_DIPSETTING(      0x0004, "Hardest?" )	/* 3*/
	PORT_DIPNAME( 0x0030, 0x0020, "Country" )	/* Default Country: USA*/
	PORT_DIPSETTING(      0x0010, "China" )
	PORT_DIPSETTING(      0x0030, "Japan"  )
	PORT_DIPSETTING(      0x0000, "Korea" )
	PORT_DIPSETTING(      0x0020, "USA"    )
	PORT_DIPNAME( 0x00c0, 0x00c0, DEF_STR( Lives ) )
	PORT_DIPSETTING(      0x0000, "2" )
	PORT_DIPSETTING(      0x00c0, "3" )
	PORT_DIPSETTING(      0x0080, "4" )
	PORT_DIPSETTING(      0x0040, "5" )

	PORT_DIPNAME( 0x0100, 0x0100, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(      0x0100, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0200, 0x0200, "Unknown 2-1" )
	PORT_DIPSETTING(      0x0200, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x1c00, 0x1c00, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(      0x0400, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(      0x1400, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0c00, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x1c00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x1800, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x0800, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x1000, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0xe000, 0xe000, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(      0x2000, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(      0xa000, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x6000, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0xe000, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0xc000, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x4000, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x8000, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Free_Play ) )

INPUT_PORTS_END


/* Same as gogomile, but the default country is different and
   the coinage settings too. */
INPUT_PORTS_START( gogomilj )

	PORT_START	/* IN0 - $800000.w*/
	PORT_BIT(  0x0001, IP_ACTIVE_LOW, IPT_COIN1    )
	PORT_BIT(  0x0002, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT(  0x0004, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT(  0x0008, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT(  0x0010, IP_ACTIVE_LOW, IPT_START1   )
	PORT_BIT(  0x0020, IP_ACTIVE_LOW, IPT_START2   )
	PORT_BIT(  0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT(  0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT(  0x0100, IP_ACTIVE_LOW, IPT_COIN2    )
	PORT_BIT(  0xfe00, IP_ACTIVE_LOW, IPT_UNKNOWN  )

	PORT_START	/* IN1 - $810000.w*/
	PORT_BIT(  0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER1 )
	PORT_BIT(  0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER1 )
	PORT_BIT(  0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER1 )
	PORT_BIT(  0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER1 )
	PORT_BIT(  0x0010, IP_ACTIVE_LOW, IPT_BUTTON1        | IPF_PLAYER1 )
	PORT_BIT(  0x0020, IP_ACTIVE_LOW, IPT_UNKNOWN                      )	/* There's code that uses*/
	PORT_BIT(  0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN                      )	/* these unknown bits*/
	PORT_BIT(  0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN                      )

	PORT_BIT(  0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER2 )
	PORT_BIT(  0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER2 )
	PORT_BIT(  0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER2 )
	PORT_BIT(  0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER2 )
	PORT_BIT(  0x1000, IP_ACTIVE_LOW, IPT_BUTTON1        | IPF_PLAYER2 )
	PORT_BIT(  0x2000, IP_ACTIVE_LOW, IPT_UNKNOWN                      )
	PORT_BIT(  0x4000, IP_ACTIVE_LOW, IPT_UNKNOWN                      )
	PORT_BIT(  0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN                      )

	PORT_START	/* IN2 - $880000.w*/
	PORT_SERVICE( 0x0001, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x0002, 0x0002, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( On ) )
	PORT_DIPNAME( 0x000c, 0x000c, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(      0x0000, "Easy?"    )	/* 0*/
	PORT_DIPSETTING(      0x000c, "Normal"   )	/* 1*/
	PORT_DIPSETTING(      0x0008, "Hard?"    )	/* 2*/
	PORT_DIPSETTING(      0x0004, "Hardest?" )	/* 3*/
	PORT_DIPNAME( 0x0030, 0x0030, "Country" )	/* Default Country: Japan*/
	PORT_DIPSETTING(      0x0010, "China" )
	PORT_DIPSETTING(      0x0030, "Japan"  )
	PORT_DIPSETTING(      0x0000, "Korea" )
	PORT_DIPSETTING(      0x0020, "USA"    )
	PORT_DIPNAME( 0x00c0, 0x00c0, DEF_STR( Lives ) )
	PORT_DIPSETTING(      0x0000, "2" )
	PORT_DIPSETTING(      0x00c0, "3" )
	PORT_DIPSETTING(      0x0080, "4" )
	PORT_DIPSETTING(      0x0040, "5" )

	PORT_DIPNAME( 0x0100, 0x0100, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(      0x0100, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0200, 0x0200, "Unknown 2-1" )
	PORT_DIPSETTING(      0x0200, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x1c00, 0x1c00, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(      0x1800, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(      0x1400, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x1000, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x1c00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0400, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x0800, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x0c00, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0xe000, 0xe000, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(      0xc000, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(      0xa000, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x8000, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0xe000, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x2000, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x4000, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x6000, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Free_Play ) )

INPUT_PORTS_END



INPUT_PORTS_START( pbancho )

	PORT_START	/* IN0 - $800000.w*/
	PORT_BIT(  0x0001, IP_ACTIVE_LOW, IPT_COIN1    )
	PORT_BIT(  0x0002, IP_ACTIVE_LOW, IPT_COIN2    )
	PORT_BIT(  0x0004, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT(  0x0008, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT(  0x0010, IP_ACTIVE_LOW, IPT_START1   )
	PORT_BIT(  0x0020, IP_ACTIVE_LOW, IPT_START2   )
	PORT_BIT(  0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT(  0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT(  0x0100, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT(  0xfe00, IP_ACTIVE_LOW, IPT_UNKNOWN  )

	PORT_START	/* IN1 - $810000.w*/
	PORT_BIT(  0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER1 )
	PORT_BIT(  0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER1 )
	PORT_BIT(  0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER1 )
	PORT_BIT(  0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER1 )
	PORT_BIT(  0x0010, IP_ACTIVE_LOW, IPT_BUTTON1        | IPF_PLAYER1 )
	PORT_BIT(  0x0020, IP_ACTIVE_LOW, IPT_UNKNOWN                      )	/* There's code that uses*/
	PORT_BIT(  0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN                      )	/* these unknown bits*/
	PORT_BIT(  0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN                      )

	PORT_BIT(  0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER2 )
	PORT_BIT(  0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER2 )
	PORT_BIT(  0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER2 )
	PORT_BIT(  0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER2 )
	PORT_BIT(  0x1000, IP_ACTIVE_LOW, IPT_BUTTON1        | IPF_PLAYER2 )
	PORT_BIT(  0x2000, IP_ACTIVE_LOW, IPT_UNKNOWN                      )
	PORT_BIT(  0x4000, IP_ACTIVE_LOW, IPT_UNKNOWN                      )
	PORT_BIT(  0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN                      )

	PORT_START	/* IN2 - $880000.w*/
	PORT_SERVICE( 0x0001, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x0002, 0x0002, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( On ) )
	PORT_DIPNAME( 0x001c, 0x001c, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(      0x0008, "Easiest" )	/* 1*/
	PORT_DIPSETTING(      0x0010, "Easy"    )	/* 2*/
	PORT_DIPSETTING(      0x001c, "Normal"  )	/* 3*/
	PORT_DIPSETTING(      0x0018, "Hard"    )	/* 4*/
	PORT_DIPSETTING(      0x0004, "Hardest" )	/* 5*/
/*	PORT_DIPSETTING(      0x0000, "Normal"  )	*/ /* 3*/
/*	PORT_DIPSETTING(      0x000c, "Normal"  )	*/ /* 3*/
/*	PORT_DIPSETTING(      0x0014, "Normal"  )	*/ /* 3*/
	PORT_DIPNAME( 0x0060, 0x0060, "Lives (Vs Mode)" )
	PORT_DIPSETTING(      0x0000, "1" )	/* 1 1*/
	PORT_DIPSETTING(      0x0060, "2" )	/* 2 3*/
/*	PORT_DIPSETTING(      0x0020, "2" )	*/ /* 2 3*/
	PORT_DIPSETTING(      0x0040, "3" )	/* 3 5*/
	PORT_DIPNAME( 0x0080, 0x0080, "? Senin Mode ?" )
	PORT_DIPSETTING(      0x0080, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )

	PORT_DIPNAME( 0x0100, 0x0100, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(      0x0100, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0200, 0x0200, "Allow Game Selection" )	/* "unused" in the manual?*/
	PORT_DIPSETTING(      0x0200, DEF_STR( Yes ) )
/*	PORT_DIPSETTING(      0x0000, DEF_STR( No ) )	*/ /* Why cripple the game!?*/
	PORT_DIPNAME( 0x1c00, 0x1c00, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(      0x0c00, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(      0x1400, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0400, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x1c00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x1000, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x0800, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x1800, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0xe000, 0xe000, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(      0x6000, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(      0xa000, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x2000, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0xe000, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x8000, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x4000, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0xc000, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Free_Play ) )

INPUT_PORTS_END



/***************************************************************************


							Graphics Layouts


***************************************************************************/

/* 8x8x4 */
static struct GfxLayout layout_8x8x4 =
{
	8,8,
	RGN_FRAC(1,1),
	4,
	{ STEP4(0,1) },
	{ 2*4,3*4,   0*4,1*4,   6*4,7*4, 4*4,5*4 },
	{ STEP8(0,8*4) },
	8*8*4
};

/* 16x16x4 */
static struct GfxLayout layout_16x16x4 =
{
	16,16,
	RGN_FRAC(1,1),
	4,
	{ STEP4(0,1) },
	{	2*4,3*4,   0*4,1*4,   6*4,7*4, 4*4,5*4,
		10*4,11*4, 8*4,9*4, 14*4,15*4, 12*4,13*4	},
	{ STEP16(0,16*4) },
	16*16*4
};

/* 16x16x8 */
static struct GfxLayout layout_16x16x8 =
{
	16,16,
	RGN_FRAC(1,2),
	8,
	{ STEP4(RGN_FRAC(1,2),1), STEP4(0,1) },
	{	2*4,3*4,   0*4,1*4,   6*4,7*4, 4*4,5*4,
		10*4,11*4, 8*4,9*4, 14*4,15*4, 12*4,13*4	},
	{ STEP16(0,16*4) },
	16*16*4
};

static struct GfxDecodeInfo fuuki16_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &layout_16x16x4, 0x400*2, 0x40 }, /* [0] Sprites*/
	{ REGION_GFX2, 0, &layout_16x16x4, 0x400*0, 0x40 }, /* [1] Layer 0*/
	{ REGION_GFX3, 0, &layout_16x16x8, 0x400*1, 0x40 }, /* [2] Layer 1*/
	{ REGION_GFX4, 0, &layout_8x8x4,   0x400*3, 0x40 }, /* [3] Layer 2*/
	{ REGION_GFX4, 0, &layout_8x8x4,   0x400*3, 0x40 }, /* [4] Layer 3 (GFX4!)*/
	{ -1 }
};


/***************************************************************************


								Machine Drivers


***************************************************************************/

static void soundirq(int state)
{
	cpu_set_irq_line(1, 0, state);
}

static struct YM2203interface fuuki16_ym2203_intf =
{
	1,
	4000000,		/* ? */
	{ YM2203_VOL(15,15) },
	{ 0 },			/* Port A Read  */
	{ 0 },			/* Port B Read  */
	{ 0 },			/* Port A Write */
	{ 0 },			/* Port B Write */
	{ 0 }			/* IRQ handler  */
};

static struct YM3812interface fuuki16_ym3812_intf =
{
	1,
	4000000,		/* ? */
	{ 30 },
	{ soundirq },	/* IRQ Line */
};

static struct OKIM6295interface fuuki16_m6295_intf =
{
	1,
	{ 8000 },		/* ? */
	{ REGION_SOUND1 },
	{ 85 }
};

/*
	- Interrupts (pbancho) -

	Lev 1:	Sets bit 5 of $400010. Prints "credit .." with sprites.
	Lev 2:	Sets bit 7 of $400010. Clears $8c0012.
			It seems unused by the game.
	Lev 3:	VBlank.
	Lev 5:	Programmable to happen on a raster line. Used to do raster
			effects when you die and its clearing the blocks
			also used for water effects and titlescreen linescroll on gogomile
*/
#define INTERRUPTS_NUM	(256-1) /* Give much better results than 256..*/
INTERRUPT_GEN( fuuki16_interrupt )
{
	if ( cpu_getiloops() == 1 )
		cpu_set_irq_line(0, 1, PULSE_LINE);

/*	if ( cpu_getiloops() == 2 )  // Not used - Glitches hiscore table? /*/
/*		cpu_set_irq_line(0, 2, PULSE_LINE);*/

	if ( cpu_getiloops() == 0 )
	{
		cpu_set_irq_line(0, 3, PULSE_LINE);	/* VBlank IRQ*/

		if (keyboard_pressed_memory(KEYCODE_F1))
		{
			fuuki16_raster_enable ^= 1;
			usrintf_showmessage("raster effects %sabled",fuuki16_raster_enable ? "en" : "dis");
		}
	}

	if ( (fuuki16_vregs[0x1c/2] & 0xff) == (INTERRUPTS_NUM-1 - cpu_getiloops()) )
	{
		cpu_set_irq_line(0, 5, PULSE_LINE);	/* Raster Line IRQ*/
		if(fuuki16_raster_enable) force_partial_update(cpu_getscanline());
	}
}

static MACHINE_DRIVER_START( fuuki16 )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 16000000)
	MDRV_CPU_MEMORY(fuuki16_readmem,fuuki16_writemem)
	MDRV_CPU_VBLANK_INT(fuuki16_interrupt,INTERRUPTS_NUM)

	MDRV_CPU_ADD(Z80, 3000000)
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)	/* ? */
	MDRV_CPU_MEMORY(fuuki16_sound_readmem,fuuki16_sound_writemem)
	MDRV_CPU_PORTS(fuuki16_sound_readport,fuuki16_sound_writeport)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(320, 256)
	MDRV_VISIBLE_AREA(0, 320-1, 0, 256-16-1)
	MDRV_GFXDECODE(fuuki16_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(0x800*4)

	MDRV_VIDEO_START(fuuki16)
	MDRV_VIDEO_UPDATE(fuuki16)

	/* sound hardware */
	MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
	MDRV_SOUND_ADD(YM2203, fuuki16_ym2203_intf)
	MDRV_SOUND_ADD(YM3812, fuuki16_ym3812_intf)
	MDRV_SOUND_ADD(OKIM6295, fuuki16_m6295_intf)
MACHINE_DRIVER_END


/***************************************************************************


								ROMs Loading


***************************************************************************/

/***************************************************************************

					Go! Go! Mile Smile / Susume! Mile Smile

(c)1995 Fuuki
FG-1C AI AM-2 (same board as Gyakuten Puzzle Banchou)

CPU  : TMP68HC000P-16
Sound: Z80 YM2203C YM3812 M6295 Y3014Bx2
OSC  : 32.00000MHz(OSC1) 28.64000MHz(OSC2) 12.000MHz(Xtal1)

ROMs:
fp2.2 - Main programs (27c4000)
fp1.1 /

lh538n1d.25 - Samples (Sharp mask, read as 27c8001)
fs1.24 - Sound program (27c010)

lh5370h8.11 - Sprites? (Sharp Mask, read as 27c160)
lh5370ha.12 |
lh5370h7.15 |
lh5370h9.16 /

lh537k2r.20 - Tiles? (Sharp Mask, read as 27c160)
lh5370hb.19 |
lh5370h6.3  /

Custom chips:
FI-002K (208pin PQFP, GA2)
FI-003K (208pin PQFP, GA3)


Others:
Mitsubishi M60067-0901FP 452100 (208pin PQFP, GA1)
4 GALs (16V8B, not dumped)

***************************************************************************/

ROM_START( gogomile )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )		/* 68000 Code */
	ROM_LOAD16_BYTE( "fp2n", 0x000000, 0x080000, CRC(e73583a0) SHA1(05c6ee5cb2c151b32c462e8b920f9a57fb6cce5b) )
	ROM_LOAD16_BYTE( "fp1n", 0x000001, 0x080000, CRC(7b110824) SHA1(980e326d3b9e113ed522be3076663a249da4e739) )

	ROM_REGION( 0x28000, REGION_CPU2, 0 )		/* Z80 Code */
	ROM_LOAD( "fs1.24", 0x00000, 0x08000, CRC(4e4bd371) SHA1(429e776135ce8960e147762763d952d16ed3f9d4) )	/* same as japanese version*/
	ROM_CONTINUE(       0x10000, 0x18000             )

	ROM_REGION( 0x200000, REGION_GFX1, ROMREGION_DISPOSE )	/* 16x16x4 Sprites */
	ROM_LOAD( "lh537k2r.20", 0x000000, 0x200000, CRC(525dbf51) SHA1(f21876676cc60ed65bc86884da894b24830826bb) )

	ROM_REGION( 0x200000, REGION_GFX2, ROMREGION_DISPOSE )	/* 16x16x4 Tiles */
	ROM_LOAD( "lh5370h6.3", 0x000000, 0x200000, CRC(e2ca7107) SHA1(7174c2e1e2106275ad41b53af22651dca492367a) )	/* x11xxxxxxxxxxxxxxxxxx = 0xFF*/

	ROM_REGION( 0x800000, REGION_GFX3, ROMREGION_DISPOSE )	/* 16x16x8 Tiles */
	ROM_LOAD( "lh5370h8.11", 0x000000, 0x200000, CRC(9961c925) SHA1(c47b4f19f090527b3e0c04dd046aa9cd51ca0e16) )
	ROM_LOAD( "lh5370ha.12", 0x200000, 0x200000, CRC(5f2a87de) SHA1(d7ed8f01b40aaf58126aaeee10ec7d948a144080) )
	ROM_LOAD( "lh5370h7.15", 0x400000, 0x200000, CRC(34921680) SHA1(d9862f106caa14ea6ad925174e6bf2d542511593) )
	ROM_LOAD( "lh5370h9.16", 0x600000, 0x200000, CRC(e0118483) SHA1(36f9068e6c81c171b4426c3794277742bbc926f5) )

	ROM_REGION( 0x200000, REGION_GFX4, ROMREGION_DISPOSE )	/* 16x16x4 Tiles */
	ROM_LOAD( "lh5370hb.19", 0x000000, 0x200000, CRC(bd1e896f) SHA1(075f7600cbced1d285cf32fc196844720eb12671) )	/* FIRST AND SECOND HALF IDENTICAL*/

	/* 0x40000 * 4: sounds+speech (japanese),sounds+speech (english) */
	ROM_REGION( 0x100000, REGION_SOUND1, ROMREGION_SOUNDONLY )	/* Samples */
	ROM_LOAD( "lh538n1d.25", 0x000000, 0x080000, CRC(01622a95) SHA1(8d414bfc6dcfab1cf9cfe5738eb5c2ff31b77df6) )
	ROM_CONTINUE(            0x000000, 0x080000             )	/* english half first*/
ROM_END

ROM_START( gogomilj )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )		/* 68000 Code */
	ROM_LOAD16_BYTE( "fp2.2", 0x000000, 0x080000, CRC(28fd3e4e) SHA1(3303e5759c0781035c74354587e1916719695754) )	/* 1xxxxxxxxxxxxxxxxxx = 0xFF*/
	ROM_LOAD16_BYTE( "fp1.1", 0x000001, 0x080000, CRC(35a5fc45) SHA1(307207791cee7f40e88feffc5805ac25008a8566) )	/* 1xxxxxxxxxxxxxxxxxx = 0xFF*/

	ROM_REGION( 0x28000, REGION_CPU2, 0 )		/* Z80 Code */
	ROM_LOAD( "fs1.24", 0x00000, 0x08000, CRC(4e4bd371) SHA1(429e776135ce8960e147762763d952d16ed3f9d4) )
	ROM_CONTINUE(       0x10000, 0x18000             )

	ROM_REGION( 0x200000, REGION_GFX1, ROMREGION_DISPOSE )	/* 16x16x4 Sprites */
	ROM_LOAD( "lh537k2r.20", 0x000000, 0x200000, CRC(525dbf51) SHA1(f21876676cc60ed65bc86884da894b24830826bb) )

	ROM_REGION( 0x200000, REGION_GFX2, ROMREGION_DISPOSE )	/* 16x16x4 Tiles */
	ROM_LOAD( "lh5370h6.3", 0x000000, 0x200000, CRC(e2ca7107) SHA1(7174c2e1e2106275ad41b53af22651dca492367a) )	/* x11xxxxxxxxxxxxxxxxxx = 0xFF*/

	ROM_REGION( 0x800000, REGION_GFX3, ROMREGION_DISPOSE )	/* 16x16x8 Tiles */
	ROM_LOAD( "lh5370h8.11", 0x000000, 0x200000, CRC(9961c925) SHA1(c47b4f19f090527b3e0c04dd046aa9cd51ca0e16) )
	ROM_LOAD( "lh5370ha.12", 0x200000, 0x200000, CRC(5f2a87de) SHA1(d7ed8f01b40aaf58126aaeee10ec7d948a144080) )
	ROM_LOAD( "lh5370h7.15", 0x400000, 0x200000, CRC(34921680) SHA1(d9862f106caa14ea6ad925174e6bf2d542511593) )
	ROM_LOAD( "lh5370h9.16", 0x600000, 0x200000, CRC(e0118483) SHA1(36f9068e6c81c171b4426c3794277742bbc926f5) )

	ROM_REGION( 0x200000, REGION_GFX4, ROMREGION_DISPOSE )	/* 16x16x4 Tiles */
	ROM_LOAD( "lh5370hb.19", 0x000000, 0x200000, CRC(bd1e896f) SHA1(075f7600cbced1d285cf32fc196844720eb12671) )	/* FIRST AND SECOND HALF IDENTICAL*/

	/* 0x40000 * 4: sounds+speech (japanese),sounds+speech (english) */
	ROM_REGION( 0x100000, REGION_SOUND1, ROMREGION_SOUNDONLY )	/* Samples */
	ROM_LOAD( "lh538n1d.25", 0x000000, 0x100000, CRC(01622a95) SHA1(8d414bfc6dcfab1cf9cfe5738eb5c2ff31b77df6) )	/* 0x40000 * 4*/
ROM_END



/***************************************************************************

							Gyakuten!! Puzzle Bancho

(c)1996 Fuuki
FG-1C AI AM-2

CPU  : TMP68HC000P-16
Sound: Z80 YM2203 YM3812 M6295
OSC  : 32.00000MHz(OSC1) 28.64000MHz(OSC2) 12.000MHz(Xtal1)

ROMs:
rom2.no1 - Main program (even)(27c4000)
rom1.no2 - Main program (odd) (27c4000)

rom25.no3 - Samples (27c2001)
rom24.no4 - Sound program (27c010)

rom11.61 - Graphics (Mask, read as 27c160)
rom15.59 |
rom20.58 |
rom3.60  /

Custom chips:
FI-002K (208pin PQFP, GA2)
FI-003K (208pin PQFP, GA3)

Others:
Mitsubishi M60067-0901FP 452100 (208pin PQFP, GA1)

***************************************************************************/

ROM_START( pbancho )

	ROM_REGION( 0x100000, REGION_CPU1, 0 )		/* 68000 Code */
	ROM_LOAD16_BYTE( "rom2.no1", 0x000000, 0x080000, CRC(1b4fd178) SHA1(02cf3d2554b29cd253470d68ea959738f3b98dbe) )	/* 1xxxxxxxxxxxxxxxxxx = 0xFF*/
	ROM_LOAD16_BYTE( "rom1.no2", 0x000001, 0x080000, CRC(9cf510a5) SHA1(08e79b5bbd1c011c32f82dd15fba42d7898861be) )	/* 1xxxxxxxxxxxxxxxxxx = 0xFF*/

	ROM_REGION( 0x28000, REGION_CPU2, 0 )		/* Z80 Code */
	ROM_LOAD( "rom24.no4", 0x00000, 0x08000, CRC(dfbfdb81) SHA1(84b0cbe843a9bbae43975afdbd029a9b76fd488b) )
	ROM_CONTINUE(          0x10000, 0x18000             )

	ROM_REGION( 0x200000, REGION_GFX1, ROMREGION_DISPOSE )	/* 16x16x4 Sprites */
	ROM_LOAD( "rom20.58", 0x000000, 0x200000, CRC(4dad0a2e) SHA1(a4f70557503110a5457b9096a79a5f249095fa55) )

	ROM_REGION( 0x200000, REGION_GFX2, ROMREGION_DISPOSE )	/* 16x16x4 Tiles */
	ROM_LOAD( "rom3.60",  0x000000, 0x200000, CRC(a50a3c1b) SHA1(a2b30f9f83f5dc2e069d7559aefbda9929fc640c) )

	ROM_REGION( 0x400000, REGION_GFX3, ROMREGION_DISPOSE )	/* 16x16x8 Tiles */
	ROM_LOAD( "rom11.61", 0x000000, 0x200000, CRC(7f1213b9) SHA1(f8d6432b270c4d0954602e430ddd26841eb05656) )
	ROM_LOAD( "rom15.59", 0x200000, 0x200000, CRC(b83dcb70) SHA1(b0b9df451535d85612fa095b4f694cf2e7930bca) )

	ROM_REGION( 0x200000, REGION_GFX4, ROMREGION_DISPOSE )	/* 16x16x4 Tiles */
	ROM_LOAD( "rom3.60",  0x000000, 0x200000, CRC(a50a3c1b) SHA1(a2b30f9f83f5dc2e069d7559aefbda9929fc640c) )	/* ?maybe?*/

	ROM_REGION( 0x040000, REGION_SOUND1, ROMREGION_SOUNDONLY )	/* Samples */
	ROM_LOAD( "rom25.no3", 0x000000, 0x040000, CRC(a7bfb5ea) SHA1(61937eae4f8855bc09c494aff52d76d41dc3b76a) )

ROM_END


/***************************************************************************


								Game Drivers


***************************************************************************/

GAMEX(1995, gogomile, 0,        fuuki16, gogomile, 0, ROT0, "Fuuki", "Go Go! Mile Smile",          GAME_IMPERFECT_SOUND )
GAMEX(1995, gogomilj, gogomile, fuuki16, gogomilj, 0, ROT0, "Fuuki", "Susume! Mile Smile (Japan)", GAME_IMPERFECT_SOUND )
GAME( 1996, pbancho,  0,        fuuki16, pbancho,  0, ROT0, "Fuuki", "Gyakuten!! Puzzle Bancho (Japan)")
