/***************************************************************************

							  -= Paradise / Penky / Target Ball / Torus =-

					driver by	Luca Elia (l.elia@tin.it)


CPU          :  Z8400B
Video Chips  :	TPC1024AFN-084C
Sound Chips  :	2 x AR17961 (OKI M6295) (only 1 in Torus)

Notes:

paradise: I'm not sure it's working correctly:

- The high scores table can't be entered !?
- note added: complete 90% or more to play a bonus game.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "paradise.h"

/***************************************************************************

								Memory Maps

***************************************************************************/

static WRITE_HANDLER( paradise_rombank_w )
{
	int bank = data;
	int bank_n = memory_region_length(REGION_CPU1)/0x4000 - 1;
	if (bank >= bank_n)
	{
		log_cb(RETRO_LOG_DEBUG, LOGPRE "PC %04X - invalid rom bank %x\n",activecpu_get_pc(),bank);
		bank %= bank_n;
	}

	if (bank >= 3)	bank+=1;
	cpu_setbank(1, memory_region(REGION_CPU1) + bank * 0x4000);
}

static WRITE_HANDLER( paradise_okibank_w )
{
	if (data & ~0x02)	log_cb(RETRO_LOG_DEBUG, LOGPRE "CPU #0 - PC %04X: unknown oki bank bits %02X\n",activecpu_get_pc(),data);
	OKIM6295_set_bank_base(1, (data & 0x02) ? 0x40000 : 0);
}

static WRITE_HANDLER( torus_coin_counter_w )
{
	coin_counter_w(0, data ^ 0xff);
}

static irq_count = 0;

static INTERRUPT_GEN(paradise_irq)
{
	if (irq_count<300)
		irq_count++;
	else
		cpu_set_irq_line(0, 0, HOLD_LINE);
}

static MEMORY_READ_START( paradise_readmem )
	{ 0x0000, 0x7fff, MRA_ROM		},	/* ROM*/
	{ 0x8000, 0xbfff, MRA_BANK1		},	/* ROM (banked)*/
	{ 0xc000, 0xffff, MRA_RAM		},	/* RAM*/
MEMORY_END

#define STANDARD_MAP	\
	{ 0x0000, 0x7fff, MWA_ROM		},	/* ROM*/	\
	{ 0x8000, 0xbfff, MWA_ROM		},	/* ROM (banked)*/ \
	{ 0xc000, 0xc7ff, paradise_vram_2_w,&paradise_vram_2	},	/* Background*/ \
	{ 0xc800, 0xcfff, paradise_vram_1_w,&paradise_vram_1	},	/* Midground*/ \
	{ 0xd000, 0xd7ff, paradise_vram_0_w,&paradise_vram_0	},	/* Foreground*/ \

static MEMORY_WRITE_START( paradise_writemem )
	STANDARD_MAP
	{ 0xd800, 0xd8ff, MWA_RAM								},	/* RAM*/
	{ 0xd900, 0xe0ff, MWA_RAM, &spriteram, &spriteram_size	},	/* Sprites*/
	{ 0xe100, 0xffff, MWA_RAM								},	/* RAM*/
MEMORY_END

static MEMORY_WRITE_START( tgtball_writemem )
	STANDARD_MAP
	{ 0xd800, 0xd8ff, MWA_RAM								},	/* RAM*/
	{ 0xd900, 0xd9ff, MWA_RAM, &spriteram, &spriteram_size	},	/* Sprites*/
	{ 0xda00, 0xffff, MWA_RAM								},	/* RAM*/
MEMORY_END

static MEMORY_WRITE_START( torus_writemem )
	STANDARD_MAP
	{ 0xd800, 0xdfff, MWA_RAM, &spriteram, &spriteram_size	},	/* Sprites*/
	{ 0xea00, 0xffff, MWA_RAM								},	/* RAM*/
MEMORY_END

static PORT_READ_START( paradise_readport )
	{ 0x0000, 0x17ff, paletteram_r			},	/* Palette*/
	{ 0x2010, 0x2010, OKIM6295_status_0_r	},	/* OKI 0*/
	{ 0x2030, 0x2030, OKIM6295_status_1_r	},	/* OKI 1*/
	{ 0x2020, 0x2020, input_port_0_r		},	/* DSW 1*/
	{ 0x2021, 0x2021, input_port_1_r		},	/* DSW 2*/
	{ 0x2022, 0x2022, input_port_2_r		},	/* P1*/
	{ 0x2023, 0x2023, input_port_3_r		},	/* P2*/
	{ 0x2024, 0x2024, input_port_4_r		},	/* Coins*/
	{ 0x8000, 0xffff, videoram_r			},	/* Pixmap*/
PORT_END

static PORT_WRITE_START( paradise_writeport )
	{ 0x0000, 0x17ff, paradise_palette_w	},	/* Palette*/
	{ 0x1800, 0x1800, paradise_priority_w	},	/* Layers priority*/
	{ 0x2001, 0x2001, paradise_flipscreen_w	},	/* Flip Screen*/
	{ 0x2004, 0x2004, paradise_palbank_w	},	/* Layers palette bank*/
	{ 0x2006, 0x2006, paradise_rombank_w	},	/* ROM bank*/
	{ 0x2007, 0x2007, paradise_okibank_w	},	/* OKI 1 samples bank*/
	{ 0x2010, 0x2010, OKIM6295_data_0_w		},	/* OKI 0*/
	{ 0x2030, 0x2030, OKIM6295_data_1_w		},	/* OKI 1*/
	{ 0x8000, 0xffff, paradise_pixmap_w		},	/* Pixmap*/
PORT_END


/***************************************************************************

								Input Ports

***************************************************************************/

/***************************************************************************
								Paradise
***************************************************************************/

INPUT_PORTS_START( paradise )
	PORT_START	/* IN0 - port $2020 - DSW 1*/
	PORT_DIPNAME( 0x03, 0x02, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x03, "Easy" )
	PORT_DIPSETTING(    0x02, "Normal" )
	PORT_DIPSETTING(    0x01, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_DIPNAME( 0x0c, 0x08, "Fill Area" )
	PORT_DIPSETTING(    0x0c, "75%" )
	PORT_DIPSETTING(    0x08, "80%" )
	PORT_DIPSETTING(    0x04, "85%" )
	PORT_DIPSETTING(    0x00, "90%" )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x10, "5" )
	PORT_DIPNAME( 0x60, 0x20, "Time" )
	PORT_DIPSETTING(    0x00, "45" )
	PORT_DIPSETTING(    0x20, "60" )
	PORT_DIPSETTING(    0x40, "75" )
	PORT_DIPSETTING(    0x60, "90" )
	PORT_DIPNAME( 0x80, 0x80, "Sound Test" )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START	/* IN1 - port $2021 - DSW 2*/
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 1C_2C ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unused ) ) /* Listed as "Unused" */
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unused ) ) /* Listed as "Unused" */
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unused ) ) /* Listed as "Unused" */
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x20, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, "Slide Show" ) /* Player1 button used to advance one time through ALL backgrounds */
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START	/* IN2 - port $2022 - Player 1*/
	PORT_BIT(  0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER1 )
	PORT_BIT(  0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER1 )
	PORT_BIT(  0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER1 )
	PORT_BIT(  0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER1 )
	PORT_BIT(  0x10, IP_ACTIVE_LOW, IPT_BUTTON1        | IPF_PLAYER1 )
	PORT_BIT(  0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* alias for button1?*/
	PORT_BIT(  0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* alias for button1?*/
	PORT_BIT(  0x80, IP_ACTIVE_LOW, IPT_START1  )

	PORT_START	/* IN3 - port $2023 - Player 2*/
	PORT_BIT(  0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER2 )
	PORT_BIT(  0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER2 )
	PORT_BIT(  0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER2 )
	PORT_BIT(  0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER2 )
	PORT_BIT(  0x10, IP_ACTIVE_LOW, IPT_BUTTON1        | IPF_PLAYER2 )
	PORT_BIT(  0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* alias for button1?*/
	PORT_BIT(  0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* alias for button1?*/
	PORT_BIT(  0x80, IP_ACTIVE_LOW, IPT_START2  )

	PORT_START	/* IN4 - port $2024 - Coins*/
	PORT_BIT_IMPULSE(  0x01, IP_ACTIVE_LOW, IPT_COIN1, 5)
	PORT_BIT_IMPULSE(  0x02, IP_ACTIVE_LOW, IPT_COIN2, 5)
	PORT_BIT(  0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x40, IP_ACTIVE_LOW, IPT_VBLANK  )
	PORT_BIT(  0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END

INPUT_PORTS_START( tgtball )
	PORT_START	/* IN0 - port $2020 - DSW 1 */
	PORT_DIPNAME( 0x03, 0x02, "Time for 1 Player" )
	PORT_DIPSETTING(    0x03, "1:00" )
	PORT_DIPSETTING(    0x02, "1:20" )
	PORT_DIPSETTING(    0x01, "1:40" )
	PORT_DIPSETTING(    0x00, "2:00" )
	PORT_DIPNAME( 0x0c, 0x08, "Bonus Time?" ) /* Difficulty or Bonus (time)? */
	PORT_DIPSETTING(    0x0c, "15" )
	PORT_DIPSETTING(    0x08, "20" )
	PORT_DIPSETTING(    0x04, "25" )
	PORT_DIPSETTING(    0x00, "30" )
	PORT_DIPNAME( 0x30, 0x20, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x30, "4" )
	PORT_DIPSETTING(    0x20, "5" )
	PORT_DIPSETTING(    0x10, "6" )
	PORT_DIPSETTING(    0x00, "7" )
	PORT_DIPNAME( 0x40, 0x40, "Balls Sequence Length" )
	PORT_DIPSETTING(    0x40, "3" )
	PORT_DIPSETTING(    0x00, "5" )
	PORT_DIPNAME( 0x80, 0x80, "Game Goal" )
	PORT_DIPSETTING(    0x80, "Target Score" )
	PORT_DIPSETTING(    0x00, "Balls Sequence" )

	PORT_START	/* IN1 - port $2021 - DSW 2 */
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 1C_2C ) )
	PORT_DIPNAME( 0x0c, 0x08, "Time for 2 Players" )
	PORT_DIPSETTING(    0x0c, "2:00" )
	PORT_DIPSETTING(    0x08, "2:40" )
	PORT_DIPSETTING(    0x04, "3:20" )
	PORT_DIPSETTING(    0x00, "4:00" )
	PORT_DIPNAME( 0x10, 0x10, "Vs. Matches" )
	PORT_DIPSETTING(    0x10, "1" )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPNAME( 0x20, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, "Slide Show" ) /* Player1 button used to advance one time through ALL backgrounds */
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START	/* IN2 - port $2022 - Player 1 */
	PORT_BIT(  0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_PLAYER1 )
	PORT_BIT(  0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_PLAYER1 )
	PORT_BIT(  0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_PLAYER1 )
	PORT_BIT(  0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER1 )
	PORT_BIT(  0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT(  0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT(  0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x80, IP_ACTIVE_LOW, IPT_START1  )

	PORT_START	/* IN3 - port $2023 - Player 2 */
	PORT_BIT(  0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_PLAYER2 )
	PORT_BIT(  0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_PLAYER2 )
	PORT_BIT(  0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_PLAYER2 )
	PORT_BIT(  0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER2 )
	PORT_BIT(  0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT(  0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT(  0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x80, IP_ACTIVE_LOW, IPT_START2  )

	PORT_START	/* IN4 - port $2024 - Coins */
	PORT_BIT_IMPULSE(  0x01, IP_ACTIVE_LOW, IPT_COIN1, 5 )
	PORT_BIT_IMPULSE(  0x02, IP_ACTIVE_LOW, IPT_COIN2, 5 )
	PORT_BIT(  0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x08, IP_ACTIVE_LOW, IPT_VBLANK  )
	PORT_BIT(  0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END

INPUT_PORTS_START( torus )
	PORT_START	/* IN0 - port $2020 - DSW 1 */
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 1C_2C ) )
	PORT_DIPNAME( 0x0c, 0x0c, "Dropping Speed" )
	PORT_DIPSETTING(    0x0c, "Normal" )
	PORT_DIPSETTING(    0x08, "Fast" )
	PORT_DIPSETTING(    0x04, "Faster" )
	PORT_DIPSETTING(    0x00, "Fastest" )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START	/* IN1 - port $2021 - DSW 2 */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, "Slide Show" )    /* Player1 Button to pull the blinds down in sections, continuous loop */
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START	/* IN2 - port $2022 - Player 1 */
	PORT_BIT(  0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_PLAYER1 )
	PORT_BIT(  0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_PLAYER1 )
	PORT_BIT(  0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_PLAYER1 )
	PORT_BIT(  0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER1 )
	PORT_BIT(  0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT(  0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT(  0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x80, IP_ACTIVE_LOW, IPT_START1  )

	PORT_START	/* IN3 - port $2023 - Player 2 */
	PORT_BIT(  0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_PLAYER2 )
	PORT_BIT(  0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_PLAYER2 )
	PORT_BIT(  0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_PLAYER2 )
	PORT_BIT(  0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER2 )
	PORT_BIT(  0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT(  0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT(  0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x80, IP_ACTIVE_LOW, IPT_START2  )

	PORT_START	/* IN4 - port $2024 - Coins */
	PORT_BIT_IMPULSE(  0x01, IP_ACTIVE_LOW, IPT_COIN1, 5 )
	PORT_BIT(  0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x40, IP_ACTIVE_LOW, IPT_VBLANK  )
	PORT_BIT(  0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END

INPUT_PORTS_START( madball )
	PORT_START	/* 8bit DSW 1 */
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 1C_3C ) )
	PORT_DIPNAME( 0x0c, 0x08, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x0c, "3" )
	PORT_DIPSETTING(    0x08, "4" )
	PORT_DIPSETTING(    0x04, "5" )
	PORT_DIPSETTING(    0x00, "6" )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x00, "Controls" )
	PORT_DIPSETTING(    0x20, "Spinner" )
	PORT_DIPSETTING(    0x00, "Joystick" )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, "Slide Show" ) /* Use P1 button to advance */
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START	/* 8bit DSW 2 */
	PORT_DIPNAME( 0x03, 0x02, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x02, "Easy" )
	PORT_DIPSETTING(    0x03, "Normal" )
	PORT_DIPSETTING(    0x01, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START	/* IN2 - port $2022 - Player 1 */
	PORT_BIT(  0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_PLAYER1 )
	PORT_BIT(  0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_PLAYER1 )
	PORT_BIT(  0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_PLAYER1 )
	PORT_BIT(  0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER1 )
	PORT_BIT(  0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT(  0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT(  0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x80, IP_ACTIVE_LOW, IPT_START1  )

	PORT_START	/* IN3 - port $2023 - Player 2 */
	PORT_BIT(  0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_PLAYER2 )
	PORT_BIT(  0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_PLAYER2 )
	PORT_BIT(  0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_PLAYER2 )
	PORT_BIT(  0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER2 )
	PORT_BIT(  0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT(  0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT(  0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x80, IP_ACTIVE_LOW, IPT_START2  )

	PORT_START	/* IN4 - port $2024 - Coins */
	PORT_BIT_IMPULSE(  0x01, IP_ACTIVE_LOW, IPT_COIN1, 5 )
	PORT_BIT(  0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x40, IP_ACTIVE_LOW, IPT_VBLANK  )
	PORT_BIT(  0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END

INPUT_PORTS_START( penky )
	PORT_START	/* port $2020 */
	PORT_DIPNAME( 0x03, 0x03, "Time" )	
	PORT_DIPSETTING(    0x00, "0:40" )
	PORT_DIPSETTING(    0x01, "0:50" )
	PORT_DIPSETTING(    0x02, "1:00" )
	PORT_DIPSETTING(    0x03, "1:10" )
	PORT_DIPNAME( 0x0c, 0x08, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x0c, "Easy" )
	PORT_DIPSETTING(    0x08, "Normal" )
	PORT_DIPSETTING(    0x04, "Hard" )
	PORT_DIPSETTING(    0x00, "Very_Hard" )
	PORT_DIPNAME( 0x30, 0x30, "Fill % to Win" )
	PORT_DIPSETTING(    0x30, "Majority at Time or 99.9%" )
	PORT_DIPSETTING(    0x20, "Majority at Time or 90%" )
	PORT_DIPSETTING(    0x10, "Majority at Time or 85%" )
	PORT_DIPSETTING(    0x00, "Majority at Time or 80%" )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START	/* port $2021 */
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 1C_2C ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, "Vs. Matches" )		
	PORT_DIPSETTING(    0x00, "2" )
	PORT_DIPSETTING(    0x10, "3" )
	PORT_DIPNAME( 0x20, 0x00, DEF_STR( Demo_Sounds ) )	
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, "Slide Show" ) /* Player1 button used to advance one time through the backgrounds */
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START	/* port $2022 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_PLAYER1)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_PLAYER1)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_PLAYER1)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER1)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* alias for button1? */
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* alias for button1? */
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START	/* port $2023 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_PLAYER2)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_PLAYER2)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_PLAYER2)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER2)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* alias for button1? */
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* alias for button1? */
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START2 )

	PORT_START	/* port $2024 */
	PORT_BIT_IMPULSE(  0x01, IP_ACTIVE_LOW, IPT_COIN1, 5 )
	PORT_BIT_IMPULSE(  0x02, IP_ACTIVE_LOW, IPT_COIN2, 5 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_VBLANK )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END

/***************************************************************************

								Graphics Layouts

***************************************************************************/

static struct GfxLayout layout_8x8x4 =
{
	8,8,
	RGN_FRAC(1,1),
	4,
	{ STEP4(0,1) },
	{ STEP8(0,4) },
	{ STEP8(0,4*8) },
	8*8*4
};

static struct GfxLayout layout_8x8x8 =
{
	8,8,
	RGN_FRAC(1,2),
	8,
	{ STEP4(RGN_FRAC(1,2),1), STEP4(RGN_FRAC(0,2),1) },
	{ STEP8(0,4) },
	{ STEP8(0,4*8) },
	8*8*4
};

static struct GfxLayout layout_16x16x8 =
{
	16,16,
	RGN_FRAC(1,2),
	8,
	{ STEP4(RGN_FRAC(1,2),1), STEP4(RGN_FRAC(0,2),1) },
	{ STEP8(8*8*4*0,4), STEP8(8*8*4*1,4) },
	{ STEP8(8*8*4*0,4*8), STEP8(8*8*4*2,4*8) },
	16*16*4
};

static struct GfxLayout torus_layout_16x16x8 =
{
	16,16,
	RGN_FRAC(1,2),
	8,
	{ STEP4(RGN_FRAC(1,2),1), STEP4(RGN_FRAC(0,2),1) },
	{ STEP8(0,4),STEP8(4*8,4) },
	{ STEP16(0,8*8) },
	128*8
};

static struct GfxDecodeInfo paradise_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &layout_16x16x8,	0x100, 1  }, /* [0] Sprites*/
	{ REGION_GFX2, 0, &layout_8x8x4,	0x400, 16 }, /* [1] Background*/
	{ REGION_GFX3, 0, &layout_8x8x8,	0x300, 1  }, /* [2] Midground*/
	{ REGION_GFX4, 0, &layout_8x8x8,	0x000, 1  }, /* [3] Foreground*/
	{ -1 }
};

static struct GfxDecodeInfo torus_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &torus_layout_16x16x8, 0x100, 1  }, /* [0] Sprites */
	{ REGION_GFX2, 0, &layout_8x8x4,	 0x400, 16 }, /* [1] Background */
	{ REGION_GFX3, 0, &layout_8x8x8,	 0x300, 1  }, /* [2] Midground */
	{ REGION_GFX4, 0, &layout_8x8x8,	 0x000, 1  }, /* [3] Foreground */
	{ -1 }
};

static struct GfxDecodeInfo madball_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &torus_layout_16x16x8, 0x500, 1  }, /* [0] Sprites */
	{ REGION_GFX2, 0, &layout_8x8x4,	 0x400, 16 }, /* [1] Background */
	{ REGION_GFX3, 0, &layout_8x8x8,	 0x300, 1  }, /* [2] Midground */
	{ REGION_GFX4, 0, &layout_8x8x8,	 0x000, 1  }, /* [3] Foreground */
	{ -1 }
};


/***************************************************************************

								Machine Drivers

***************************************************************************/

static struct OKIM6295interface paradise_okim6295_intf =
{
	2,
	{ 1000000/132,1000000/132 },		/* 1Mhz / 132 verified */
	{ REGION_SOUND1,REGION_SOUND2 },
	{ 50,50 }
};

static MACHINE_DRIVER_START( paradise )

	/* basic machine hardware */
	MDRV_CPU_ADD_TAG("main", Z80, 6000000)			/* Z8400B */
	MDRV_CPU_FLAGS(CPU_16BIT_PORT)
	MDRV_CPU_MEMORY(paradise_readmem,paradise_writemem)
	MDRV_CPU_PORTS(paradise_readport,paradise_writeport)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,4)	/* No nmi routine */

	MDRV_FRAMES_PER_SECOND(54) /* 54 verified */
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)	/* we're using IPT_VBLANK */

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_NEEDS_6BITS_PER_GUN)
	MDRV_SCREEN_SIZE(256, 256)
	MDRV_VISIBLE_AREA(0, 256-1, 0+16, 256-1-16)
	MDRV_GFXDECODE(paradise_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(0x800 + 16)

	MDRV_VIDEO_START(paradise)
	MDRV_VIDEO_UPDATE(paradise)

	/* sound hardware */
	MDRV_SOUND_ADD(OKIM6295, paradise_okim6295_intf)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( tgtball )
	/* basic machine hardware */
	MDRV_IMPORT_FROM( paradise )
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_MEMORY(paradise_readmem,tgtball_writemem)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( torus )
	/* basic machine hardware */
	MDRV_IMPORT_FROM(paradise)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_MEMORY(paradise_readmem,torus_writemem)

	MDRV_GFXDECODE(torus_gfxdecodeinfo)

	MDRV_VIDEO_UPDATE(torus)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( madball )
	/* basic machine hardware */
	MDRV_IMPORT_FROM(paradise)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_MEMORY(paradise_readmem,torus_writemem)

	MDRV_GFXDECODE(madball_gfxdecodeinfo)

	MDRV_VIDEO_UPDATE(madball)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( penky )
	/* basic machine hardware */
	MDRV_IMPORT_FROM(paradise)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_MEMORY(paradise_readmem,torus_writemem)
	MDRV_CPU_PORTS(paradise_readport,paradise_writeport)
	MDRV_CPU_VBLANK_INT(paradise_irq,4)	/* No nmi routine */
MACHINE_DRIVER_END

/***************************************************************************

								ROMs Loading

***************************************************************************/

/***************************************************************************

									Paradise

(c) yun sung  year ??
another porn qix alike game
1 main cpu tpc1024afn-084c  ??
1 sound z8400b ps
2 ar17961  (oki?)
1 12.000 oscillator cristal

The year is not shown but must be >= 1994, since the development system
(cross compiler?) they used left a "1994.8-1989" in the rom

***************************************************************************/

ROM_START( paradise )
	ROM_REGION( 0x44000, REGION_CPU1, 0 )		/* Z80 Code */
	ROM_LOAD( "u128", 0x00000, 0x0c000, CRC(8e5b5a24) SHA1(a4e559d9329f8a7a9d12cd90d98d0525958085d8) )
	ROM_CONTINUE(     0x10000, 0x34000    )

	ROM_REGION( 0x80000, REGION_GFX1, ROMREGION_DISPOSE | ROMREGION_INVERT)	/* 16x16x8 Sprites */
	ROM_LOAD( "u114", 0x00000, 0x40000, CRC(c748ba3b) SHA1(ad23bda4e001ca539f849c1ca256de5daf7c233b) )
	ROM_LOAD( "u115", 0x40000, 0x40000, CRC(0d517bbb) SHA1(5bf7c5036f3d660901e26f14baaea1a3c0327dfe) )

	ROM_REGION( 0x20000, REGION_GFX2, ROMREGION_DISPOSE | ROMREGION_INVERT)	/* 8x8x4 Background */
	ROM_LOAD( "u94", 0x00000, 0x20000, CRC(e3a99209) SHA1(5db79dc1a38d93b458b043499a58516285c65aa8) )

	ROM_REGION( 0x100000, REGION_GFX3, ROMREGION_DISPOSE | ROMREGION_INVERT)	/* 8x8x8 Foreground */
	ROM_LOAD( "u92", 0x00000, 0x80000, CRC(633d24f0) SHA1(26b25ec1014fba1a3d0d2bdba0c867c57034647d) )
	ROM_LOAD( "u93", 0x80000, 0x80000, CRC(bbf5c632) SHA1(9d31e136f014c2dd7dd988c3aee0adfcfea91bc9) )

	ROM_REGION( 0x40000, REGION_GFX4, ROMREGION_DISPOSE | ROMREGION_INVERT)	/* 8x8x8 Midground */
	ROM_LOAD( "u110", 0x00000, 0x20000, CRC(9807a7e6) SHA1(30e2a741a93954cfe672c61c93a990d0c3b25145) )
	ROM_LOAD( "u111", 0x20000, 0x20000, CRC(bc9f93f0) SHA1(dd4cfc849a0c0f918ac0dfeb7f00a67aae5a1c13) )

	ROM_REGION( 0x40000, REGION_SOUND1, ROMREGION_SOUNDONLY )	/* Samples */
	ROM_LOAD( "u85", 0x00000, 0x40000, CRC(bf3c3065) SHA1(54dd7ffea2fb3f31ed575e982b82691cddc2581a) )

	ROM_REGION( 0x80000, REGION_SOUND2, ROMREGION_SOUNDONLY )	/* Samples (banked) */
	ROM_LOAD( "u113", 0x00000, 0x80000, CRC(53de6025) SHA1(c94b3778b57ff7f46ce4cff661841019fb187d5d) )
ROM_END

ROM_START( tgtball )
	ROM_REGION( 0x44000, REGION_CPU1, 0 )		/* Z80 Code */
	ROM_LOAD( "rom7.bin", 0x00000, 0x0c000, CRC(8dbeab12) SHA1(7181c23459990aecbe2d13377aaf19f65108eac6) )
	ROM_CONTINUE(         0x10000, 0x34000    )

	ROM_REGION( 0x80000, REGION_GFX1, ROMREGION_DISPOSE | ROMREGION_INVERT)	/* 16x16x8 Sprites */
	ROM_LOAD( "yunsung.114", 0x00000, 0x40000, CRC(3dbe1872) SHA1(754f90123a3944ca548fc66ee65a93615155bf30) )
	ROM_LOAD( "yunsung.115", 0x40000, 0x40000, CRC(30f49dac) SHA1(b70d37973bd03069c48641d6c0804be6f9aa6553) )

	ROM_REGION( 0x20000, REGION_GFX2, ROMREGION_DISPOSE | ROMREGION_ERASEFF)	/* 8x8x4 Background */
	/* not for this game? */

	ROM_REGION( 0x100000, REGION_GFX3, ROMREGION_DISPOSE | ROMREGION_INVERT)	/* 8x8x8 Foreground */
	ROM_LOAD( "rom2.bin", 0x00000, 0x80000, CRC(fe4004ec) SHA1(fde782665445ad465b8f8fb95df5f60cd24016ad) )
	ROM_LOAD( "rom1.bin", 0x80000, 0x80000, CRC(aef17762) SHA1(3dd8924695b67eec0f25549dbe2461b927268b8f) )

	ROM_REGION( 0x100000, REGION_GFX4, ROMREGION_DISPOSE | ROMREGION_INVERT)	/* 8x8x8 Midground */
	ROM_LOAD( "rom4.bin", 0x00000, 0x80000,  CRC(0a5abf62) SHA1(6900d598764300c81c90f5a7efb294639178bee6) )
	ROM_LOAD( "rom3.bin", 0x80000, 0x80000,  CRC(94822bbf) SHA1(9fa6595eb819f163b58181926c276346cfa5c332) )

	ROM_REGION( 0x40000, REGION_SOUND1, ROMREGION_SOUNDONLY )	/* Samples */
	ROM_LOAD( "yunsung.85", 0x00000, 0x20000, CRC(cdf3336b) SHA1(98029d6d5d8ffb3b24ae2bcf950618a7d5b404c3) )

	ROM_REGION( 0x80000, REGION_SOUND2, ROMREGION_SOUNDONLY )	/* Samples (banked) */
	ROM_LOAD( "yunsung.113", 0x00000, 0x40000, CRC(150a6cc6) SHA1(b435fcf8ba48006f506db6b63ba54a30a6b3eade) )
ROM_END

ROM_START( tgtballa )
	ROM_REGION( 0x44000, REGION_CPU1, 0 )		/* Z80 Code */
	ROM_LOAD( "yunsung.128", 0x00000, 0x0c000, CRC(cb0f3d46) SHA1(b56c4abbd4248074c1559a0f1902d2ea11cb01a8) )
	ROM_CONTINUE(         0x10000, 0x34000    )

	ROM_REGION( 0x80000, REGION_GFX1, ROMREGION_DISPOSE | ROMREGION_INVERT)	/* 16x16x8 Sprites */
	ROM_LOAD( "yunsung.114", 0x00000, 0x40000, CRC(3dbe1872) SHA1(754f90123a3944ca548fc66ee65a93615155bf30) )
	ROM_LOAD( "yunsung.115", 0x40000, 0x40000, CRC(30f49dac) SHA1(b70d37973bd03069c48641d6c0804be6f9aa6553) )

	ROM_REGION( 0x20000, REGION_GFX2, ROMREGION_DISPOSE | ROMREGION_ERASEFF)	/* 8x8x4 Background */
	/* not for this game? */

	ROM_REGION( 0x100000, REGION_GFX3, ROMREGION_DISPOSE | ROMREGION_INVERT)	/* 8x8x8 Foreground */
	ROM_LOAD( "yunsung.92", 0x00000, 0x80000, CRC(bcf206a9) SHA1(0db2cee21c025b7b8d2d5b898c7231c77e36904d) )
	ROM_LOAD( "yunsung.93", 0x80000, 0x80000, CRC(64edb93c) SHA1(94f8d4fd159c682d952d6a4c38dc50f2c0c0824d) )

	ROM_REGION( 0x100000, REGION_GFX4, ROMREGION_DISPOSE | ROMREGION_INVERT)	/* 8x8x8 Midground */
	ROM_LOAD( "yunsung.110", 0x00000, 0x80000, CRC(c209201e) SHA1(ba1cb3a204f689f9a3636834628d2265927e34f7) )
	ROM_LOAD( "yunsung.111", 0x80000, 0x80000, CRC(82334337) SHA1(4b2a07196027b190366131cd7b8eca87a1bd0b1c) )

	ROM_REGION( 0x40000, REGION_SOUND1, ROMREGION_SOUNDONLY )	/* Samples */
	ROM_LOAD( "yunsung.85", 0x00000, 0x20000, CRC(cdf3336b) SHA1(98029d6d5d8ffb3b24ae2bcf950618a7d5b404c3) )

	ROM_REGION( 0x80000, REGION_SOUND2, ROMREGION_SOUNDONLY )	/* Samples (banked) */
	ROM_LOAD( "yunsung.113", 0x00000, 0x40000, CRC(150a6cc6) SHA1(b435fcf8ba48006f506db6b63ba54a30a6b3eade) )
ROM_END

ROM_START( torus )
	ROM_REGION( 0x14000, REGION_CPU1, 0 )		/* Z80 Code */
	ROM_LOAD( "bc13.bin",     0x00000, 0xc000, CRC(55d3ef3e) SHA1(195463271fdb3f9f5c19068efd1c99105f761fe9) )
	ROM_CONTINUE(             0x10000, 0x4000 )

	ROM_REGION( 0x80000, REGION_GFX1, ROMREGION_DISPOSE | ROMREGION_INVERT)	/* 16x16x8 Sprites */
	ROM_LOAD( "bc5.bin",      0x00000, 0x40000, CRC(5b60ce9f) SHA1(d5c091145e0bae7cd776e642ea17895d086ed2b0) )
	ROM_LOAD( "bc6.bin",      0x40000, 0x40000, CRC(4caa0c50) SHA1(a971b6e87cd1162cf370d39cfeafefbb1557e14e) )

	ROM_REGION( 0x20000, REGION_GFX2, ROMREGION_DISPOSE | ROMREGION_ERASEFF)	/* 8x8x4 Background */
	/* not for this game */

	ROM_REGION( 0x100000, REGION_GFX3, ROMREGION_DISPOSE | ROMREGION_INVERT)	/* 8x8x8 Foreground */
	ROM_LOAD( "bc2.bin",      0x00000, 0x80000, CRC(67c5ba1a) SHA1(0e39752ddc5ee9469647140a3fc9e6bb69d6afa1) )
	ROM_LOAD( "bc1.bin",      0x80000, 0x80000, CRC(efb105e9) SHA1(7bfe6ff64b25797dd524a7077def5669f25f16ec) )

	ROM_REGION( 0x40000, REGION_GFX4, ROMREGION_DISPOSE | ROMREGION_INVERT)	/* 8x8x8 Midground */
	ROM_LOAD( "bc4.bin",      0x00000, 0x20000, CRC(ee914caf) SHA1(42f3d760a4c14658ac2eb0ba7f54fb9916368b50) )
	ROM_LOAD( "bc3.bin",      0x20000, 0x20000, CRC(aff1dab9) SHA1(ae488abd605c1e78b8b73452a2c1391cc0fe6b00) )

	ROM_REGION( 0x40000, REGION_SOUND1, ROMREGION_SOUNDONLY )	/* Samples */
	ROM_LOAD( "bc15.bin",     0x00000, 0x40000, CRC(12d84839) SHA1(840d82253c0651ebe6799ea2bb5bae334e963e12) )
ROM_END

/*
Yun Sung Mad Ball
PCB: YS-0402
Z80A
AD-65 (OKI M6295)
Actel A1020B PLC84C
OSC: 12.000 MHz, 4.000Mhz
RAM 4 Hyundai HY62256ALP-70
    1 Hyundai HY6264LP-10 (by P rom & Z80A)
DSW 2 8-switch DIP
P.u1  Intel i27C010A - Program (next to Z80A)
s.u28 ST M27C4001    - Sound (next to AD-65)
1.u66  ST M27C2001
2.u67  ST M27C2001
3.u92  TI 27C040
4.u93  TI 27C040
5.u105 TI 27C040
6.u106 TI 27C040
All roms with manufacturer's IDs and routines
*/

ROM_START( madball ) /* Models in swimsuits only, no nudity */
	ROM_REGION( 0x24000, REGION_CPU1, 0 )		/* Z80 Code */
	ROM_LOAD( "p.u1",     0x00000, 0xc000, CRC(73008425) SHA1(6eded60fd5c637a63783247c858d999d5974d378) )
	ROM_CONTINUE(             0x10000, 0x14000 )

	ROM_REGION( 0x80000, REGION_GFX1, ROMREGION_DISPOSE | ROMREGION_INVERT)	/* 16x16x8 Sprites */
	ROM_LOAD( "2.u67",      0x00000, 0x40000, CRC(1f3a6cd5) SHA1(7a17549f2fff003605d91703c84a398488b2f74c) )
	ROM_LOAD( "1.u66",      0x40000, 0x40000, CRC(8637c7b4) SHA1(e0026e48f0e8f3554a5b448e0d1f9d1c5551dbfb) )

	ROM_REGION( 0x20000, REGION_GFX2, ROMREGION_DISPOSE | ROMREGION_ERASEFF)	/* 8x8x4 Background */
	/* not for this game */

	ROM_REGION( 0x100000, REGION_GFX3, ROMREGION_DISPOSE | ROMREGION_INVERT)	/* 8x8x8 Foreground */
	ROM_LOAD( "5.u105",      0x00000, 0x80000, CRC(f26aac1e) SHA1(50ad34ee70bf45fa4e1dc9281b83bcdd7c7db3f8) )
	ROM_LOAD( "6.u106",      0x80000, 0x80000, CRC(27b78907) SHA1(ab6645457adc0d17b141e366aac7e00e8ce4296b) )

	ROM_REGION( 0x100000, REGION_GFX4, ROMREGION_DISPOSE | ROMREGION_INVERT)	/* 8x8x8 Midground */
	ROM_LOAD( "4.u93",      0x80000, 0x80000, CRC(c3be56ad) SHA1(9cfa0b38c60798deccca74dc6b0ce0826ff7f467) )
	ROM_LOAD( "3.u92",      0x00000, 0x80000, CRC(846019a6) SHA1(571bfa299e13b96ca263bd7e62c760bdbe3438bd) )

	ROM_REGION( 0x80000, REGION_SOUND1, 0 )	/* Samples */
	ROM_LOAD( "s.u28",     0x00000, 0x80000, CRC(78f02584) SHA1(70542e126db73a573db9ef41399d3a07fb7ea94b) )
ROM_END

ROM_START( madballn ) /* Even numbered stages show topless models.  Is nudity controlled by a dipswitch? */
	ROM_REGION( 0x24000, REGION_CPU1, 0 )		/* Z80 Code */
	ROM_LOAD( "bc13.u1",     0x00000, 0xc000, CRC(531fa919) SHA1(0eafc663b9ad50d0dfc5491fe96c9bcf30483991) )
	ROM_CONTINUE(             0x10000, 0x14000 )

	ROM_REGION( 0x80000, REGION_GFX1, ROMREGION_DISPOSE | ROMREGION_INVERT)	/* 16x16x8 Sprites */
	ROM_LOAD( "2.u67",      0x00000, 0x40000, CRC(1f3a6cd5) SHA1(7a17549f2fff003605d91703c84a398488b2f74c) )
	ROM_LOAD( "1.u66",      0x40000, 0x40000, CRC(8637c7b4) SHA1(e0026e48f0e8f3554a5b448e0d1f9d1c5551dbfb) )

	ROM_REGION( 0x20000, REGION_GFX2, ROMREGION_DISPOSE | ROMREGION_ERASEFF)	/* 8x8x4 Background */
	/* not for this game */

	ROM_REGION( 0x100000, REGION_GFX3, ROMREGION_DISPOSE | ROMREGION_INVERT)	/* 8x8x8 Foreground */
	ROM_LOAD( "bc2.u105",      0x00000, 0x80000, CRC(d75faa62) SHA1(95badf932e8a8084e67aa7df8d6cb2cb2917d5fc) )
	ROM_LOAD( "bc1.u106",      0x80000, 0x80000, CRC(04b8f7a5) SHA1(97555880f200d0ecc521f8c76bcaa4a0f0eb1aa9) )

	ROM_REGION( 0x100000, REGION_GFX4, ROMREGION_DISPOSE | ROMREGION_INVERT)	/* 8x8x8 Midground */
	ROM_LOAD( "bc3.u93",      0x80000, 0x80000, CRC(f07a5fe6) SHA1(0b1117d8ff0f2a6c953ab1988065b75a33e2c949) )
	ROM_LOAD( "bc4.u92",      0x00000, 0x80000, CRC(7ed233ab) SHA1(8a4bc31741b4e6e1c03974f9b00f747a29c78ebf) )

	ROM_REGION( 0x80000, REGION_SOUND1, 0 )	/* Samples */
	ROM_LOAD( "s.u28",     0x00000, 0x80000, CRC(78f02584) SHA1(70542e126db73a573db9ef41399d3a07fb7ea94b) )
ROM_END

/***************************************************************************
                          Penky
YS951004
  CPU: Z8400B PS (Z80 6Mhz)
Sound: OKI M6295 x 2
Video: Actel A1020A PL84C
  OSC: 12.000MHz & 4.000MHz
YS951004
+--------------------------------------------+
|    M6295 M6295   Z80    4MHz    U110  U111 |
|        U113*    U128                  U92  |
|        U85     6264                   U93  |
|                                       U94  |
|               6116                         |
|J                     +-------+             |
|A                     | Actel |  6116       |
|M              6116   |A1020A |             |
|M                     | PL84C |             |
|A              6116   +-------+             |
|       12MHz                                |
| DSW1                                  6116 |
|                     4464              6116 |
|                     4464  U114        6116 |
| DSW2                4464  U115             |
|                     4464                   |
+--------------------------------------------+
U113 is not populated on this PCB
Notes, the clocks should be the same as other boards of this era/type. IE:
      Z80 clock: 6.000MHz
     6295 clock: 1.000MHz (both), sample rate = 1000000/132 (both)
***************************************************************************/

ROM_START( penky )
	ROM_REGION( 0x44000, REGION_CPU1, 0 )		/* Z80 Code */
	ROM_LOAD( "yunsung.u128", 0x00000, 0x0c000, CRC(57baeada) SHA1(360fd2d352b201e57436ed9c9f0510a052452738) )
	ROM_CONTINUE(             0x10000, 0x34000 )

	ROM_REGION( 0x100000, REGION_GFX1, ROMREGION_DISPOSE | ROMREGION_INVERT)	/* 16x16x8 Sprites */
	ROM_LOAD( "yunsung.u114", 0x00000, 0x80000, CRC(cb6b1cfd) SHA1(22406f70fc2ad839d5ca4d00d503a2857b295cf5) )
	ROM_LOAD( "yunsung.u115", 0x80000, 0x80000, CRC(55c5ff90) SHA1(f68a22628b9da77c3e301fa57bf673c572760869) )

	ROM_REGION( 0x20000, REGION_GFX2, ROMREGION_DISPOSE | ROMREGION_INVERT)	/* 8x8x4 Background */
	ROM_LOAD( "yunsung.u94", 0x00000, 0x20000, CRC(58b31c0e) SHA1(eea9a0c17737ce071895f818499edee7790d98f7) )

	ROM_REGION( 0x100000, REGION_GFX3, ROMREGION_DISPOSE | ROMREGION_INVERT)	/* 8x8x8 Foreground */
	ROM_LOAD( "yunsung.u92", 0x00000, 0x80000, CRC(31993a6c) SHA1(8cdcae52472768f40dc7cbefaa459982d008deaa) )
	ROM_LOAD( "yunsung.u93", 0x80000, 0x80000, CRC(b570dc0c) SHA1(1f55681412db144e2d5cbb7a89783edc5059add7) )

	ROM_REGION( 0x100000, REGION_GFX4, ROMREGION_DISPOSE | ROMREGION_INVERT)	/* 8x8x8 Midground */
	ROM_LOAD( "yunsung.u110", 0x00000, 0x80000, CRC(ba3173a1) SHA1(6667bced70eb6be9853239feb69d4b30daf2d0c1) )
	ROM_LOAD( "yunsung.u111", 0x80000, 0x80000, CRC(9223ef85) SHA1(f8da8fc5c8178165e8142eb52889b4ef1c710e24) )

	ROM_REGION( 0x40000, REGION_SOUND1, 0 )	/* Samples */
	ROM_LOAD( "yunsung.u85", 0x00000, 0x40000, CRC(c664d0cc) SHA1(52d5122407e727d4c98bc6f2f939534de4b725ae) )

	ROM_REGION( 0x80000, REGION_SOUND2, ROMREGION_ERASE00 )	/* Samples (banked) */
	/* not populated for this game */
ROM_END

DRIVER_INIT (paradise)
{
	paradise_sprite_inc = 0x20;
}

/* Inverted flipscreen and sprites are packed in less memory (same number though) */
DRIVER_INIT (tgtball)
{
	paradise_sprite_inc = 4;
	install_port_write_handler(0, 0x2001, 0x2001, tgtball_flipscreen_w );
}

DRIVER_INIT (torus)
{
	paradise_sprite_inc = 4;
	install_port_write_handler(0, 0x2070, 0x2070, torus_coin_counter_w);
}


/***************************************************************************

								Game Drivers

***************************************************************************/

GAME( 1994+, paradise, 0,       paradise, paradise, paradise, ROT90, "Yun Sung", "Paradise" )
GAME( 1995,  tgtball,  0,       tgtball,  tgtball,  tgtball,  ROT0,  "Yun Sung", "Target Ball (Nude)" )
GAME( 1995,  tgtballa, tgtball, tgtball,  tgtball,  tgtball,  ROT0,  "Yun Sung", "Target Ball" )
GAME( 1995,  penky,    0,       penky,    penky,    tgtball,  ROT0,  "Yun Sung", "Penky" )
GAME( 1996,  torus,    0,       torus,    torus,    torus,    ROT90, "Yun Sung", "Torus" )
GAME( 1998,  madball,  0,       madball,  madball,  tgtball,  ROT0,  "Yun Sung", "Mad Ball V2.0" )
GAME( 1997,  madballn, madball, madball,  madball,  tgtball,  ROT0,  "Yun Sung", "Mad Ball V2.0 (With Nudity)" )
