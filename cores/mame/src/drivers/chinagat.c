/*
China Gate.
By Paul Hampson from First Principles
(IE: Roms + a description of their contents and a list of CPUs on board.)

Based on ddragon.c:
"Double Dragon, Double Dragon (bootleg) & Double Dragon II"
"By Carlos A. Lozano & Rob Rosenbrock et. al."

NOTES:
A couple of things unaccounted for:

No backgrounds ROMs from the original board...
- This may be related to the SubCPU. I don't think it's contributing
  much right now, but I could be wrong. And it would explain that vast
  expanse of bankswitch ROM on a slave CPU....
- Just had a look at the sprites, and they seem like kosher sprites all
  the way up.... So it must be hidden in the sub-cpu somewhere?
- Got two bootleg sets with background gfx roms. Using those on the
  original games for now.

OBVIOUS SPEED PROBLEMS...
- Timers are too fast and/or too slow, and the whole thing's moving too fast

Port 0x2800 on the Sub CPU.
- All those I/O looking ports on the main CPU (0x3exx and 0x3fxx)
- One's scroll control. Prolly other vidhrdw control as well.
- Location 0x1a2ec in cgate51.bin (The main CPU's ROM) is 88. This is
  copied to videoram, and causes that minor visual discrepancy on
  the title screen. But the CPU tests that part of the ROM and passes
  it OK. Since it's just a simple summing of words, another word
  somewhere (or others in total) has lost 0x8000. Or the original
  game had this problem. (Not on the screenshot I got)
- The Japanese ones have a different title screen so I can't check.

ADPCM in the bootlegs is not quite right.... Misusing the data?
- They're nibble-swapped versions of the original roms...
- There's an Intel i8748 CPU on the bootlegs (bootleg 1 lists D8749 but
  the microcode dump's the same). This in conjunction with the different
  ADPCM chip (msm5205) are used to 'fake' a M6295.
- Bootleg 1 ADPCM is now wired up, but still not working :-(
  Definantly sync problems between the i8049 and the m5205 which need
  further looking at.


There's also a few small dumps from the boards.


MAJOR DIFFERENCES FROM DOUBLE DRAGON:
Sound system is like Double Dragon II (In fact for MAME's
purposes it's identical. I think DD3 and one or two others
also use this. Was it an addon on the original?
The dual-CPU setup looked similar to DD at first, but
the second CPU doesn't talk to the sprite RAM at all, but
just through the shared memory (which DD1 doesn't have,
except for the sprite RAM.)
Also the 2nd CPU in China Gate has just as much code as
the first CPU, and bankswitches similarly, where DD1 and DD2 have
different Sprite CPUs but only a small bank of code each.
More characters and colours of characters than DD1 or 2.
More sprites than DD1, less than DD2.
But the formats are the same (allowing for extra chars and colours)
Video hardware's like DD1 (thank god)
Input is unique but has a few similarities to DD2 (the coin inputs)


*/



#include "driver.h"
#include "vidhrdw/generic.h"
#include "cpu/m6809/m6809.h"
#include "cpu/z80/z80.h"
#include "cpu/i8039/i8039.h"
#include "sound/2151intf.h"
#include "sound/2203intf.h"

/**************** Video stuff ******************/

WRITE_HANDLER( ddragon_bgvideoram_w );
WRITE_HANDLER( ddragon_fgvideoram_w );

VIDEO_START( chinagat );
VIDEO_UPDATE( ddragon );

extern int technos_video_hw;
extern int ddragon_scrollx_hi, ddragon_scrolly_hi;
extern data8_t *ddragon_scrollx_lo;
extern data8_t *ddragon_scrolly_lo;
extern data8_t *ddragon_bgvideoram,*ddragon_fgvideoram;

/**************** Machine stuff ******************/
static int sprite_irq, sound_irq, adpcm_sound_irq;
static int saiyugb1_adpcm_addr;
static int saiyugb1_i8748_P1;
static int saiyugb1_i8748_P2;
static int saiyugb1_pcm_shift;
static int saiyugb1_pcm_nibble;
static int saiyugb1_mcu_command;
#if 0
static int saiyugb1_m5205_clk;
#endif



static MACHINE_INIT( chinagat )
{
	technos_video_hw = 1;
	sprite_irq = M6809_IRQ_LINE;
	sound_irq = IRQ_LINE_NMI;
}

WRITE_HANDLER( chinagat_video_ctrl_w )
{
	/***************************
	---- ---x   X Scroll MSB
	---- --x-   Y Scroll MSB
	---- -x--   Flip screen
	--x- ----   Enable video ???
	****************************/

	ddragon_scrolly_hi = ( ( data & 0x02 ) << 7 );
	ddragon_scrollx_hi = ( ( data & 0x01 ) << 8 );

	flip_screen_set(~data & 0x04);
}

static WRITE_HANDLER( chinagat_bankswitch_w )
{
	data8_t *RAM = memory_region(REGION_CPU1);
	cpu_setbank( 1,&RAM[ 0x10000 + (0x4000 * (data & 7)) ] );
}

static WRITE_HANDLER( chinagat_sub_bankswitch_w )
{
	data8_t *RAM = memory_region( REGION_CPU2 );
	cpu_setbank( 4,&RAM[ 0x10000 + (0x4000 * (data & 7)) ] );
}

static WRITE_HANDLER( chinagat_sub_IRQ_w )
{
	cpu_set_irq_line( 1, sprite_irq, (sprite_irq == IRQ_LINE_NMI) ? PULSE_LINE : HOLD_LINE );
}

static WRITE_HANDLER( chinagat_cpu_sound_cmd_w )
{
	soundlatch_w( offset, data );
	cpu_set_irq_line( 2, sound_irq, (sound_irq == IRQ_LINE_NMI) ? PULSE_LINE : HOLD_LINE );
}

static READ_HANDLER( saiyugb1_mcu_command_r )
{
#if 0
	if (saiyugb1_mcu_command == 0x78)
	{
		timer_suspendcpu(3, 1, SUSPEND_REASON_HALT);	/* Suspend (speed up) */
	}
#endif
	return saiyugb1_mcu_command;
}

static WRITE_HANDLER( saiyugb1_mcu_command_w )
{
	saiyugb1_mcu_command = data;
#if 0
	if (data != 0x78)
	{
		timer_suspendcpu(3, 0, SUSPEND_REASON_HALT);	/* Wake up */
	}
#endif
}

static WRITE_HANDLER( saiyugb1_adpcm_rom_addr_w )
{
	/* i8748 Port 1 write */
	saiyugb1_i8748_P1 = data;
}

static WRITE_HANDLER( saiyugb1_adpcm_control_w )
{
	/* i8748 Port 2 write */

	data8_t *saiyugb1_adpcm_rom = memory_region(REGION_SOUND1);

	if (data & 0x80)	/* Reset m5205 and disable ADPCM ROM outputs */
	{
		log_cb(RETRO_LOG_DEBUG, LOGPRE "ADPCM output disabled\n");
		saiyugb1_pcm_nibble = 0x0f;
		MSM5205_reset_w(0,1);
	}
	else
	{
		if ( (saiyugb1_i8748_P2 & 0xc) != (data & 0xc) )
		{
			if ((saiyugb1_i8748_P2 & 0xc) == 0)	/* Latch MSB Address */
			{
/*/				log_cb(RETRO_LOG_DEBUG, LOGPRE "Latching MSB\n");*/
				saiyugb1_adpcm_addr = (saiyugb1_adpcm_addr & 0x3807f) | (saiyugb1_i8748_P1 << 7);
			}
			if ((saiyugb1_i8748_P2 & 0xc) == 4)	/* Latch LSB Address */
			{
/*/				log_cb(RETRO_LOG_DEBUG, LOGPRE "Latching LSB\n");*/
				saiyugb1_adpcm_addr = (saiyugb1_adpcm_addr & 0x3ff80) | (saiyugb1_i8748_P1 >> 1);
				saiyugb1_pcm_shift = (saiyugb1_i8748_P1 & 1) * 4;
			}
		}

		saiyugb1_adpcm_addr = ((saiyugb1_adpcm_addr & 0x07fff) | (data & 0x70 << 11));

		saiyugb1_pcm_nibble = saiyugb1_adpcm_rom[saiyugb1_adpcm_addr & 0x3ffff];

		saiyugb1_pcm_nibble = (saiyugb1_pcm_nibble >> saiyugb1_pcm_shift) & 0x0f;

/*/		log_cb(RETRO_LOG_DEBUG, LOGPRE "Writing %02x to m5205. $ROM=%08x  P1=%02x  P2=%02x  Prev_P2=%02x  Nibble=%08x\n",saiyugb1_pcm_nibble,saiyugb1_adpcm_addr,saiyugb1_i8748_P1,data,saiyugb1_i8748_P2,saiyugb1_pcm_shift);*/

		if ( ((saiyugb1_i8748_P2 & 0xc) >= 8) && ((data & 0xc) == 4) )
		{
			MSM5205_data_w (0, saiyugb1_pcm_nibble);
			log_cb(RETRO_LOG_DEBUG, LOGPRE "Writing %02x to m5205\n",saiyugb1_pcm_nibble);
		}
		log_cb(RETRO_LOG_DEBUG, LOGPRE "$ROM=%08x  P1=%02x  P2=%02x  Prev_P2=%02x  Nibble=%1x  PCM_data=%02x\n",saiyugb1_adpcm_addr,saiyugb1_i8748_P1,data,saiyugb1_i8748_P2,saiyugb1_pcm_shift,saiyugb1_pcm_nibble);
	}
	saiyugb1_i8748_P2 = data;
}

static WRITE_HANDLER( saiyugb1_m5205_clk_w )
{
	/* i8748 T0 output clk mode */
	/* This signal goes through a divide by 8 counter */
	/* to the xtal pins of the MSM5205 */

	/* Actually, T0 output clk mode is not supported by the i8048 core */

#if 0
	saiyugb1_m5205_clk++;
	if (saiyugb1_m5205_clk == 8)
	}
		MSM5205_vclk_w (0, 1);		/* ??? */
		saiyugb1_m5205_clk = 0;
	}
	else
	}
		MSM5205_vclk_w (0, 0);		/* ??? */
	}
#endif
}

static READ_HANDLER( saiyugb1_m5205_irq_r )
{
	if (adpcm_sound_irq)
	{
		adpcm_sound_irq = 0;
		return 1;
	}
	return 0;
}
static void saiyugb1_m5205_irq_w(int num)
{
	adpcm_sound_irq = 1;
}

static MEMORY_READ_START( readmem )
	{ 0x0000, 0x1fff, MRA_BANK2 },
	{ 0x3f00, 0x3f00, input_port_0_r },
	{ 0x3f01, 0x3f01, input_port_1_r },
	{ 0x3f02, 0x3f02, input_port_2_r },
	{ 0x3f03, 0x3f03, input_port_3_r },
	{ 0x3f04, 0x3f04, input_port_4_r },
	{ 0x4000, 0x7fff, MRA_BANK1 },
	{ 0x8000, 0xffff, MRA_ROM },
MEMORY_END

static MEMORY_WRITE_START( writemem )
	{ 0x0000, 0x1fff, MWA_BANK2 },
	{ 0x2000, 0x27ff, ddragon_fgvideoram_w, &ddragon_fgvideoram },
	{ 0x2800, 0x2fff, ddragon_bgvideoram_w, &ddragon_bgvideoram },
	{ 0x3000, 0x317f, paletteram_xxxxBBBBGGGGRRRR_split1_w, &paletteram },
	{ 0x3400, 0x357f, paletteram_xxxxBBBBGGGGRRRR_split2_w, &paletteram_2 },
	{ 0x3800, 0x397f, MWA_BANK3, &spriteram, &spriteram_size },
	{ 0x3e00, 0x3e00, chinagat_cpu_sound_cmd_w },
/*	{ 0x3e01, 0x3e01, MWA_NOP },*/
/*	{ 0x3e02, 0x3e02, MWA_NOP },*/
/*	{ 0x3e03, 0x3e03, MWA_NOP },*/
	{ 0x3e04, 0x3e04, chinagat_sub_IRQ_w },
	{ 0x3e06, 0x3e06, MWA_RAM, &ddragon_scrolly_lo },
	{ 0x3e07, 0x3e07, MWA_RAM, &ddragon_scrollx_lo },
	{ 0x3f00, 0x3f00, chinagat_video_ctrl_w },
	{ 0x3f01, 0x3f01, chinagat_bankswitch_w },
	{ 0x4000, 0xffff, MWA_ROM },
MEMORY_END

static MEMORY_READ_START( sub_readmem )
	{ 0x0000, 0x1fff, MRA_BANK2 },
/*	{ 0x2a2b, 0x2a2b, MRA_NOP },  // What lives here? /*/
/*	{ 0x2a30, 0x2a30, MRA_NOP },  // What lives here? /*/
	{ 0x4000, 0x7fff, MRA_BANK4 },
	{ 0x8000, 0xffff, MRA_ROM },
MEMORY_END

static MEMORY_WRITE_START( sub_writemem )
	{ 0x0000, 0x1fff, MWA_BANK2 },
	{ 0x2000, 0x2000, chinagat_sub_bankswitch_w },
	{ 0x2800, 0x2800, MWA_RAM }, /* Called on CPU start and after return from jump table */
	{ 0x4000, 0xffff, MWA_ROM },
MEMORY_END

static MEMORY_READ_START( sound_readmem )
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0x87ff, MRA_RAM },
	{ 0x8801, 0x8801, YM2151_status_port_0_r },
	{ 0x9800, 0x9800, OKIM6295_status_0_r },
	{ 0xA000, 0xA000, soundlatch_r },
MEMORY_END

static MEMORY_WRITE_START( sound_writemem )
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0x8000, 0x87ff, MWA_RAM },
	{ 0x8800, 0x8800, YM2151_register_port_0_w },
	{ 0x8801, 0x8801, YM2151_data_port_0_w },
	{ 0x9800, 0x9800, OKIM6295_data_0_w },
MEMORY_END

static MEMORY_READ_START( ym2203c_sound_readmem )
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0x87ff, MRA_RAM },
	{ 0x8800, 0x8800, YM2203_status_port_0_r },
/*	{ 0x8802, 0x8802, OKIM6295_status_0_r },*/
	{ 0x8804, 0x8804, YM2203_status_port_1_r },
/*	{ 0x8801, 0x8801, YM2151_status_port_0_r },*/
/*	{ 0x9800, 0x9800, OKIM6295_status_0_r },*/
	{ 0xA000, 0xA000, soundlatch_r },
MEMORY_END

static MEMORY_WRITE_START( ym2203c_sound_writemem )
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0x8000, 0x87ff, MWA_RAM },
/* 8804 and/or 8805 make a gong sound when the coin goes in*/
/* but only on the title screen....*/

	{ 0x8800, 0x8800, YM2203_control_port_0_w },
	{ 0x8801, 0x8801, YM2203_write_port_0_w },
/*	{ 0x8802, 0x8802, OKIM6295_data_0_w },*/
/*	{ 0x8803, 0x8803, OKIM6295_data_0_w },*/
	{ 0x8804, 0x8804, YM2203_control_port_1_w },
	{ 0x8805, 0x8805, YM2203_write_port_1_w },
/*	{ 0x8804, 0x8804, MWA_RAM },*/
/*	{ 0x8805, 0x8805, MWA_RAM },*/

/*	{ 0x8800, 0x8800, YM2151_register_port_0_w },*/
/*	{ 0x8801, 0x8801, YM2151_data_port_0_w },*/
/*	{ 0x9800, 0x9800, OKIM6295_data_0_w },*/
MEMORY_END

static MEMORY_READ_START( saiyugb1_sound_readmem )
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0x87ff, MRA_RAM },
	{ 0x8801, 0x8801, YM2151_status_port_0_r },
	{ 0xA000, 0xA000, soundlatch_r },
MEMORY_END

static MEMORY_WRITE_START( saiyugb1_sound_writemem )
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0x8000, 0x87ff, MWA_RAM },
	{ 0x8800, 0x8800, YM2151_register_port_0_w },
	{ 0x8801, 0x8801, YM2151_data_port_0_w },
	{ 0x9800, 0x9800, saiyugb1_mcu_command_w },
MEMORY_END

static MEMORY_READ_START( i8748_readmem )
	{ 0x0000, 0x03ff, MRA_ROM },
	{ 0x0400, 0x07ff, MRA_ROM },	/* i8749 version */
MEMORY_END

static MEMORY_WRITE_START( i8748_writemem )
	{ 0x0000, 0x03ff, MWA_ROM },
	{ 0x0400, 0x07ff, MWA_ROM },
MEMORY_END

static PORT_READ_START( i8748_readport )
	{ I8039_bus, I8039_bus, saiyugb1_mcu_command_r },
	{ I8039_t1,  I8039_t1,  saiyugb1_m5205_irq_r },
PORT_END

static PORT_WRITE_START( i8748_writeport )
	{ I8039_t0,  I8039_t0,  saiyugb1_m5205_clk_w }, 		/* Drives the clock on the m5205 at 1/8 of this frequency */
	{ I8039_p1,  I8039_p1,  saiyugb1_adpcm_rom_addr_w },
	{ I8039_p2,  I8039_p2,  saiyugb1_adpcm_control_w },
PORT_END



INPUT_PORTS_START( chinagat )
	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_VBLANK )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0xf0, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START
	PORT_DIPNAME( 0x07, 0x07, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x07, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x05, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_5C ) )
	PORT_DIPNAME( 0x38, 0x38, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x38, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x28, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x18, DEF_STR( 1C_5C ) )
	/*PORT_DIPNAME( 0x40, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(	0x40, DEF_STR( Cocktail ) )*/
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(	0x01, "Easy" )
	PORT_DIPSETTING(	0x03, "Normal" )
	PORT_DIPSETTING(	0x02, "Hard" )
	PORT_DIPSETTING(	0x00, "Hardest" )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x04, DEF_STR( On ))
	PORT_DIPNAME( 0x30, 0x30, "Timer" )
	PORT_DIPSETTING(    0x00, "50" )
	PORT_DIPSETTING(    0x20, "55" )
	PORT_DIPSETTING(    0x30, "60" )
	PORT_DIPSETTING(    0x10, "70" )
	PORT_DIPNAME( 0xc0, 0x80, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "1" )
	PORT_DIPSETTING(    0xc0, "2" )
	PORT_DIPSETTING(    0x80, "3" )
	PORT_DIPSETTING(    0x40, "4" )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START2 )
INPUT_PORTS_END

static struct GfxLayout charlayout =
{
	8,8,			/* 8*8 chars */
	RGN_FRAC(1,1),	/* num of characters */
	4,				/* 4 bits per pixel */
	{ 0, 2, 4, 6 },		/* plane offset */
	{ 1, 0, 65, 64, 129, 128, 193, 192 },
	{ STEP8(0,8) },			/* { 0*8, 1*8 ... 6*8, 7*8 }, */
	32*8 /* every char takes 32 consecutive bytes */
};

static struct GfxLayout tilelayout =
{
	16,16,			/* 16x16 chars */
	RGN_FRAC(1,2),	/* num of Tiles/Sprites */
	4,				/* 4 bits per pixel */
	{ RGN_FRAC(1,2)+0, RGN_FRAC(1,2)+4, 0,4 }, /* plane offset */
	{ 3, 2, 1, 0, 16*8+3, 16*8+2, 16*8+1, 16*8+0,
		32*8+3,32*8+2 ,32*8+1 ,32*8+0 ,48*8+3 ,48*8+2 ,48*8+1 ,48*8+0 },
	{ STEP16(0,8) },		/* { 0*8, 1*8 ... 14*8, 15*8 }, */
	64*8 /* every char takes 64 consecutive bytes */
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &charlayout,   0,16 },	/*  8x8  chars */
	{ REGION_GFX2, 0, &tilelayout, 128, 8 },	/* 16x16 sprites */
	{ REGION_GFX3, 0, &tilelayout, 256, 8 },	/* 16x16 background tiles */
	{ -1 } /* end of array */
};

static void chinagat_irq_handler(int irq) {
	cpu_set_irq_line( 2, 0, irq ? ASSERT_LINE : CLEAR_LINE );
}

static struct YM2151interface ym2151_interface =
{
	1,			/* 1 chip */
	3579545,	/* 3.579545 oscillator */
	{ YM3012_VOL(80,MIXER_PAN_LEFT,80,MIXER_PAN_RIGHT) },	/* only right channel is connected */
	{ chinagat_irq_handler }
};


static struct OKIM6295interface okim6295_interface =
{
	1,					/* 1 chip */
	{ 11000 },			/* ??? frequency (Hz) */
	{ REGION_SOUND1 },	/* memory region */
	{ 45 }
};

/* This on the bootleg board, instead of the m6295 */
static struct MSM5205interface msm5205_interface =
{
	1,							/* 1 chip */
	9263750 / 24,				/* 385989.6 Hz from the 9.26375MHz oscillator */
	{ saiyugb1_m5205_irq_w },	/* Interrupt function */
	{ MSM5205_S64_4B },			/* vclk input mode (6030Hz, 4-bit) */
	{ 60 }
};

static INTERRUPT_GEN( chinagat_interrupt )
{
	cpu_set_irq_line(0, 1, HOLD_LINE);	/* hold the FIRQ line */
	cpu_set_nmi_line(0, PULSE_LINE);	/* pulse the NMI line */
}

/* This is only on the second bootleg board */
static struct YM2203interface ym2203_interface =
{
	2,			/* 2 chips */
	3579545,	/* 3.579545 oscillator */
	{ YM2203_VOL(80,50), YM2203_VOL(80,50) },
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 },
	{ chinagat_irq_handler }
};

static MACHINE_DRIVER_START( chinagat )

	/* basic machine hardware */
	MDRV_CPU_ADD(HD6309,12000000/8)		/* 1.5 MHz (12MHz oscillator ???) */
	MDRV_CPU_MEMORY(readmem,writemem)
	MDRV_CPU_VBLANK_INT(chinagat_interrupt,1)

	MDRV_CPU_ADD(HD6309,12000000/8)		/* 1.5 MHz (12MHz oscillator ???) */
	MDRV_CPU_MEMORY(sub_readmem,sub_writemem)

	MDRV_CPU_ADD(Z80, 3579545)	/* 3.579545 MHz */
	MDRV_CPU_MEMORY(sound_readmem,sound_writemem)

	MDRV_FRAMES_PER_SECOND(56)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(100) /* heavy interleaving to sync up sprite<->main cpu's */

	MDRV_MACHINE_INIT(chinagat)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(1*8, 31*8-1, 2*8, 30*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(384)

	MDRV_VIDEO_START(chinagat)
	MDRV_VIDEO_UPDATE(ddragon)

	/* sound hardware */
	MDRV_SOUND_ADD(YM2151, ym2151_interface)
	MDRV_SOUND_ADD(OKIM6295, okim6295_interface)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( saiyugb1 )

	/* basic machine hardware */
	MDRV_CPU_ADD(M6809,12000000/8)		/* 68B09EP 1.5 MHz (12MHz oscillator) */
	MDRV_CPU_MEMORY(readmem,writemem)
	MDRV_CPU_VBLANK_INT(chinagat_interrupt,1)

	MDRV_CPU_ADD(M6809,12000000/8)		/* 68B09EP 1.5 MHz (12MHz oscillator) */
	MDRV_CPU_MEMORY(sub_readmem,sub_writemem)

	MDRV_CPU_ADD(Z80, 3579545)		/* 3.579545 MHz oscillator */
	MDRV_CPU_MEMORY(saiyugb1_sound_readmem,saiyugb1_sound_writemem)

	MDRV_CPU_ADD(I8048,9263750/3)		/* 3.087916 MHz (9.263750 MHz oscillator) */
	MDRV_CPU_MEMORY(i8748_readmem,i8748_writemem)
	MDRV_CPU_PORTS(i8748_readport,i8748_writeport)

	MDRV_FRAMES_PER_SECOND(56)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(100)	/* heavy interleaving to sync up sprite<->main cpu's */

	MDRV_MACHINE_INIT(chinagat)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(1*8, 31*8-1, 2*8, 30*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(384)

	MDRV_VIDEO_START(chinagat)
	MDRV_VIDEO_UPDATE(ddragon)

	/* sound hardware */
	MDRV_SOUND_ADD(YM2151, ym2151_interface)
	MDRV_SOUND_ADD(MSM5205, msm5205_interface)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( saiyugb2 )

	/* basic machine hardware */
	MDRV_CPU_ADD(M6809,12000000/8)		/* 1.5 MHz (12MHz oscillator) */
	MDRV_CPU_MEMORY(readmem,writemem)
	MDRV_CPU_VBLANK_INT(chinagat_interrupt,1)

	MDRV_CPU_ADD(M6809,12000000/8)		/* 1.5 MHz (12MHz oscillator) */
	MDRV_CPU_MEMORY(sub_readmem,sub_writemem)

	MDRV_CPU_ADD(Z80, 3579545)		/* 3.579545 MHz oscillator */
	MDRV_CPU_MEMORY(ym2203c_sound_readmem,ym2203c_sound_writemem)

	MDRV_FRAMES_PER_SECOND(56)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(100) /* heavy interleaving to sync up sprite<->main cpu's */

	MDRV_MACHINE_INIT(chinagat)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(1*8, 31*8-1, 2*8, 30*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(384)

	MDRV_VIDEO_START(chinagat)
	MDRV_VIDEO_UPDATE(ddragon)

	/* sound hardware */
	MDRV_SOUND_ADD(YM2203, ym2203_interface)
MACHINE_DRIVER_END


/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( chinagat )
	ROM_REGION( 0x28000, REGION_CPU1, 0 )	/* Main CPU: 128KB for code (bankswitched using $3F01) */
	ROM_LOAD( "cgate51.bin", 0x10000, 0x18000, CRC(439a3b19) SHA1(01393b4302ac7a66390270b01e2757582240f6b8) )	/* Banks 0x4000 long @ 0x4000 */
	ROM_CONTINUE(            0x08000, 0x08000 )				/* Static code */

	ROM_REGION( 0x28000, REGION_CPU2, 0 )	/* Slave CPU: 128KB for code (bankswitched using $2000) */
	ROM_LOAD( "23j4-0.48",   0x10000, 0x18000, CRC(2914af38) SHA1(3d690fa50b7d36a22de82c026d59a16126a7b73c) ) /* Banks 0x4000 long @ 0x4000 */
	ROM_CONTINUE(            0x08000, 0x08000 )				/* Static code */

	ROM_REGION( 0x10000, REGION_CPU3, 0 )	/* Music CPU, 64KB */
	ROM_LOAD( "23j0-0.40",   0x00000, 0x08000, CRC(9ffcadb6) SHA1(606dbdd73aee3cabb2142200ac6f8c96169e4b19) )

	ROM_REGION(0x20000, REGION_GFX1, ROMREGION_DISPOSE )	/* Text */
	ROM_LOAD( "cgate18.bin", 0x00000, 0x20000, CRC(8d88d64d) SHA1(57265138ebb0c6419542cce5953aee7335bfa2bd) )	/* 0,1,2,3 */

	ROM_REGION(0x80000, REGION_GFX2, ROMREGION_DISPOSE )	/* Sprites */
	ROM_LOAD( "23j7-0.103",  0x00000, 0x20000, CRC(2f445030) SHA1(3fcf32097e655e963d952d01a30396dc195269ca) )	/* 2,3 */
	ROM_LOAD( "23j8-0.102",  0x20000, 0x20000, CRC(237f725a) SHA1(47bebe5b9878ca10fe6efd4f353717e53a372416) )	/* 2,3 */
	ROM_LOAD( "23j9-0.101",  0x40000, 0x20000, CRC(8caf6097) SHA1(50ad192f831b055586a4a9974f8c6c2f2063ede5) )	/* 0,1 */
	ROM_LOAD( "23ja-0.100",  0x60000, 0x20000, CRC(f678594f) SHA1(4bdcf9407543925f4630a8c7f1f48b85f76343a9) )	/* 0,1 */

	ROM_REGION(0x40000, REGION_GFX3, ROMREGION_DISPOSE )	/* Background */
	ROM_LOAD( "a1-13", 0x00000, 0x10000, BAD_DUMP CRC(b745cac4) SHA1(759767ca7c5123b03b9e1a42bb105d194cb76400) )
	ROM_LOAD( "a1-12", 0x10000, 0x10000, BAD_DUMP CRC(3c864299) SHA1(cb12616e4d6c53a82beb4cd51510a632894b359c) )
	ROM_LOAD( "a1-15", 0x20000, 0x10000, BAD_DUMP CRC(2f268f37) SHA1(f82cfe3b2001d5ed2a709ca9c51febcf624bb627) )
	ROM_LOAD( "a1-14", 0x30000, 0x10000, BAD_DUMP CRC(aef814c8) SHA1(f6b9229ca7beb9a0e47d1f6a1083c6102fdd20c8) )

	ROM_REGION(0x40000, REGION_SOUND1, 0 )	/* ADPCM */
	ROM_LOAD( "23j1-0.53", 0x00000, 0x20000, CRC(f91f1001) SHA1(378402a3c966cabd61e9662ae5decd66672a228b) )
	ROM_LOAD( "23j2-0.52", 0x20000, 0x20000, CRC(8b6f26e9) SHA1(7da26ae846814b3957b19c38b6bf7e83617dc6cc) )

	ROM_REGION(0x300, REGION_USER1, 0 )	/* Unknown Bipolar PROMs */
	ROM_LOAD( "23jb-0.16", 0x000, 0x200, CRC(46339529) SHA1(64f4c42a826d67b7cbaa8a23a45ebc4eb6248891) )	/* 82S131 on video board */
	ROM_LOAD( "23j5-0.45", 0x200, 0x100, CRC(fdb130a9) SHA1(4c4f214229b9fab2b5d69c745ec5428787b89e1f) )	/* 82S129 on main board */
ROM_END


ROM_START( saiyugou )
	ROM_REGION( 0x28000, REGION_CPU1, 0 )	/* Main CPU: 128KB for code (bankswitched using $3F01) */
	ROM_LOAD( "23j3-0.51",  0x10000, 0x18000, CRC(aa8132a2) SHA1(87c3bd447767f263113c4865afc905a0e484a625) )	/* Banks 0x4000 long @ 0x4000 */
	ROM_CONTINUE(           0x08000, 0x08000)				/* Static code */

	ROM_REGION( 0x28000, REGION_CPU2, 0 )	/* Slave CPU: 128KB for code (bankswitched using $2000) */
	ROM_LOAD( "23j4-0.48",  0x10000, 0x18000, CRC(2914af38) SHA1(3d690fa50b7d36a22de82c026d59a16126a7b73c) )	/* Banks 0x4000 long @ 0x4000 */
	ROM_CONTINUE(           0x08000, 0x08000)				/* Static code */

	ROM_REGION( 0x10000, REGION_CPU3, 0 )	/* Music CPU, 64KB */
	ROM_LOAD( "23j0-0.40",  0x00000, 0x8000, CRC(9ffcadb6) SHA1(606dbdd73aee3cabb2142200ac6f8c96169e4b19) )

	ROM_REGION(0x20000, REGION_GFX1, ROMREGION_DISPOSE )	/* Text */
	ROM_LOAD( "23j6-0.18",  0x00000, 0x20000, CRC(86d33df0) SHA1(3419959c28703c5177de9c11b61e1dba9e76aca5) )	/* 0,1,2,3 */

	ROM_REGION(0x80000, REGION_GFX2, ROMREGION_DISPOSE )	/* Sprites */
	ROM_LOAD( "23j7-0.103", 0x00000, 0x20000, CRC(2f445030) SHA1(3fcf32097e655e963d952d01a30396dc195269ca) )	/* 2,3 */
	ROM_LOAD( "23j8-0.102", 0x20000, 0x20000, CRC(237f725a) SHA1(47bebe5b9878ca10fe6efd4f353717e53a372416) )	/* 2,3 */
	ROM_LOAD( "23j9-0.101", 0x40000, 0x20000, CRC(8caf6097) SHA1(50ad192f831b055586a4a9974f8c6c2f2063ede5) )	/* 0,1 */
	ROM_LOAD( "23ja-0.100", 0x60000, 0x20000, CRC(f678594f) SHA1(4bdcf9407543925f4630a8c7f1f48b85f76343a9) )	/* 0,1 */

	ROM_REGION(0x40000, REGION_GFX3, ROMREGION_DISPOSE )	/* Background */
	ROM_LOAD( "a2-13", 0x00000, 0x10000, BAD_DUMP CRC(b745cac4) SHA1(759767ca7c5123b03b9e1a42bb105d194cb76400) )
	ROM_LOAD( "a2-12", 0x10000, 0x10000, BAD_DUMP CRC(3c864299) SHA1(cb12616e4d6c53a82beb4cd51510a632894b359c) )
	ROM_LOAD( "a2-15", 0x20000, 0x10000, BAD_DUMP CRC(2f268f37) SHA1(f82cfe3b2001d5ed2a709ca9c51febcf624bb627) )
	ROM_LOAD( "a2-14", 0x30000, 0x10000, BAD_DUMP CRC(aef814c8) SHA1(f6b9229ca7beb9a0e47d1f6a1083c6102fdd20c8) )

	ROM_REGION(0x40000, REGION_SOUND1, 0 )	/* ADPCM */
	ROM_LOAD( "23j1-0.53", 0x00000, 0x20000, CRC(f91f1001) SHA1(378402a3c966cabd61e9662ae5decd66672a228b) )
	ROM_LOAD( "23j2-0.52", 0x20000, 0x20000, CRC(8b6f26e9) SHA1(7da26ae846814b3957b19c38b6bf7e83617dc6cc) )

	ROM_REGION(0x300, REGION_USER1, 0 )	/* Unknown Bipolar PROMs */
	ROM_LOAD( "23jb-0.16", 0x000, 0x200, CRC(46339529) SHA1(64f4c42a826d67b7cbaa8a23a45ebc4eb6248891) )	/* 82S131 on video board */
	ROM_LOAD( "23j5-0.45", 0x200, 0x100, CRC(fdb130a9) SHA1(4c4f214229b9fab2b5d69c745ec5428787b89e1f) )	/* 82S129 on main board */
ROM_END

ROM_START( saiyugb1 )
	ROM_REGION( 0x28000, REGION_CPU1, 0 )	/* Main CPU: 128KB for code (bankswitched using $3F01) */
	ROM_LOAD( "23j3-0.51",  0x10000, 0x18000, CRC(aa8132a2) SHA1(87c3bd447767f263113c4865afc905a0e484a625) )	/* Banks 0x4000 long @ 0x4000 */
	/* Orientation of bootleg ROMs which are split, but otherwise the same.
	   ROM_LOAD( "a-5.bin", 0x10000, 0x10000, CRC(39795aa5) )	   Banks 0x4000 long @ 0x4000
	   ROM_LOAD( "a-9.bin", 0x20000, 0x08000, CRC(051ebe92) )	   Banks 0x4000 long @ 0x4000
	*/
	ROM_CONTINUE(           0x08000, 0x08000 )				/* Static code */

	ROM_REGION( 0x28000, REGION_CPU2, 0 )	/* Slave CPU: 128KB for code (bankswitched using $2000) */
	ROM_LOAD( "23j4-0.48",  0x10000, 0x18000, CRC(2914af38) SHA1(3d690fa50b7d36a22de82c026d59a16126a7b73c) )	/* Banks 0x4000 long @ 0x4000 */
	/* Orientation of bootleg ROMs which are split, but otherwise the same.
	   ROM_LOAD( "a-4.bin", 0x10000, 0x10000, CRC(9effddc1) )	   Banks 0x4000 long @ 0x4000
	   ROM_LOAD( "a-8.bin", 0x20000, 0x08000, CRC(a436edb8) )	   Banks 0x4000 long @ 0x4000
	*/
	ROM_CONTINUE(           0x08000, 0x08000 )				/* Static code */

	ROM_REGION( 0x10000, REGION_CPU3, 0 )	/* Music CPU, 64KB */
	ROM_LOAD( "a-1.bin",  0x00000, 0x8000,  CRC(46e5a6d4) SHA1(965ed7bdb727ab32ce3322ca49f1a4e3786e8051) )

	ROM_REGION( 0x800, REGION_CPU4, 0 )		/* ADPCM CPU, 1KB */
	ROM_LOAD( "mcu8748.bin", 0x000, 0x400, CRC(6d28d6c5) SHA1(20582c62a72545e68c2e155b063ee7e95e1228ce) )

	ROM_REGION(0x20000, REGION_GFX1, ROMREGION_DISPOSE )	/* Text */
	ROM_LOAD( "23j6-0.18",  0x00000, 0x20000, CRC(86d33df0) SHA1(3419959c28703c5177de9c11b61e1dba9e76aca5) )	/* 0,1,2,3 */
	/* Orientation of bootleg ROMs which are split, but otherwise the same.
	   ROM_LOAD( "a-2.bin", 0x00000, 0x10000, CRC(baa5a3b9) )	   0,1
	   ROM_LOAD( "a-3.bin", 0x10000, 0x10000, CRC(532d59be) )	   2,3
	*/

	ROM_REGION(0x80000, REGION_GFX2, ROMREGION_DISPOSE )	/* Sprites */
	ROM_LOAD( "23j7-0.103",  0x00000, 0x20000, CRC(2f445030) SHA1(3fcf32097e655e963d952d01a30396dc195269ca) )	/* 2,3 */
	ROM_LOAD( "23j8-0.102",  0x20000, 0x20000, CRC(237f725a) SHA1(47bebe5b9878ca10fe6efd4f353717e53a372416) )	/* 2,3 */
	ROM_LOAD( "23j9-0.101",  0x40000, 0x20000, CRC(8caf6097) SHA1(50ad192f831b055586a4a9974f8c6c2f2063ede5) )	/* 0,1 */
	ROM_LOAD( "23ja-0.100",  0x60000, 0x20000, CRC(f678594f) SHA1(4bdcf9407543925f4630a8c7f1f48b85f76343a9) )	/* 0,1 */
	/* Orientation of bootleg ROMs which are split, but otherwise the same
	   ROM_LOAD( "a-23.bin", 0x00000, 0x10000, CRC(12b56225) )	   2,3
	   ROM_LOAD( "a-22.bin", 0x10000, 0x10000, CRC(b592aa9b) )	   2,3
	   ROM_LOAD( "a-21.bin", 0x20000, 0x10000, CRC(a331ba3d) )	   2,3
	   ROM_LOAD( "a-20.bin", 0x30000, 0x10000, CRC(2515d742) )	   2,3
	   ROM_LOAD( "a-19.bin", 0x40000, 0x10000, CRC(d796f2e4) )	   0,1
	   ROM_LOAD( "a-18.bin", 0x50000, 0x10000, CRC(c9e1c2f9) )	   0,1
	   ROM_LOAD( "a-17.bin", 0x60000, 0x10000, CRC(00b6db0a) )	   0,1
	   ROM_LOAD( "a-16.bin", 0x70000, 0x10000, CRC(f196818b) )	   0,1
	*/

	ROM_REGION(0x40000, REGION_GFX3, ROMREGION_DISPOSE )	/* Background */
	ROM_LOAD( "a-13", 0x00000, 0x10000, CRC(b745cac4) SHA1(759767ca7c5123b03b9e1a42bb105d194cb76400) )
	ROM_LOAD( "a-12", 0x10000, 0x10000, CRC(3c864299) SHA1(cb12616e4d6c53a82beb4cd51510a632894b359c) )
	ROM_LOAD( "a-15", 0x20000, 0x10000, CRC(2f268f37) SHA1(f82cfe3b2001d5ed2a709ca9c51febcf624bb627) )
	ROM_LOAD( "a-14", 0x30000, 0x10000, CRC(aef814c8) SHA1(f6b9229ca7beb9a0e47d1f6a1083c6102fdd20c8) )

	/* Some bootlegs have incorrectly halved the ADPCM data ! */
	/* These are same as the 128k sample except nibble-swapped */
	ROM_REGION(0x40000, REGION_SOUND1, 0 )	/* ADPCM */		/* Bootleggers wrong data */
	ROM_LOAD ( "a-6.bin",   0x00000, 0x10000, CRC(4da4e935) SHA1(235a1589165a23cfad29e07cf66d7c3a777fc904) )	/* 0x8000, 0x7cd47f01 */
	ROM_LOAD ( "a-7.bin",   0x10000, 0x10000, CRC(6284c254) SHA1(e01be1bd4768ae0ccb1cec65b3a6bc80ed7a4b00) )	/* 0x8000, 0x7091959c */
	ROM_LOAD ( "a-10.bin",  0x20000, 0x10000, CRC(b728ec6e) SHA1(433b5f907e4918e89b79bd927e2993ad3030017b) )	/* 0x8000, 0x78349cb6 */
	ROM_LOAD ( "a-11.bin",  0x30000, 0x10000, CRC(a50d1895) SHA1(0c2c1f8a2e945d6c53ce43413f0e63ced45bae17) )	/* 0x8000, 0xaa5b6834 */

	ROM_REGION(0x300, REGION_USER1, 0 )	/* Unknown Bipolar PROMs */
	ROM_LOAD( "23jb-0.16", 0x000, 0x200, CRC(46339529) SHA1(64f4c42a826d67b7cbaa8a23a45ebc4eb6248891) )	/* 82S131 on video board */
	ROM_LOAD( "23j5-0.45", 0x200, 0x100, CRC(fdb130a9) SHA1(4c4f214229b9fab2b5d69c745ec5428787b89e1f) )	/* 82S129 on main board */
ROM_END

ROM_START( saiyugb2 )
	ROM_REGION( 0x28000, REGION_CPU1, 0 )	/* Main CPU: 128KB for code (bankswitched using $3F01) */
	ROM_LOAD( "23j3-0.51",   0x10000, 0x18000, CRC(aa8132a2) SHA1(87c3bd447767f263113c4865afc905a0e484a625) )	/* Banks 0x4000 long @ 0x4000 */
	/* Orientation of bootleg ROMs which are split, but otherwise the same.
	   ROM_LOAD( "sai5.bin", 0x10000, 0x10000, CRC(39795aa5) )	   Banks 0x4000 long @ 0x4000
	   ROM_LOAD( "sai9.bin", 0x20000, 0x08000, CRC(051ebe92) )	   Banks 0x4000 long @ 0x4000
	*/
	ROM_CONTINUE(            0x08000, 0x08000 )				/* Static code */

	ROM_REGION( 0x28000, REGION_CPU2, 0 )	/* Slave CPU: 128KB for code (bankswitched using $2000) */
	ROM_LOAD( "23j4-0.48", 0x10000, 0x18000, CRC(2914af38) SHA1(3d690fa50b7d36a22de82c026d59a16126a7b73c) )	/* Banks 0x4000 long @ 0x4000 */
	/* Orientation of bootleg ROMs which are split, but otherwise the same.
	   ROM_LOAD( "sai4.bin", 0x10000, 0x10000, CRC(9effddc1) )	   Banks 0x4000 long @ 0x4000
	   ROM_LOAD( "sai8.bin", 0x20000, 0x08000, CRC(a436edb8) )	   Banks 0x4000 long @ 0x4000
	*/
	ROM_CONTINUE(         0x08000, 0x08000 )				/* Static code */

	ROM_REGION( 0x10000, REGION_CPU3, 0 )	/* Music CPU, 64KB */
	ROM_LOAD( "sai-alt1.bin", 0x00000, 0x8000, CRC(8d397a8d) SHA1(52599521c3dbcecc1ae56bb80dc855e76d700134) )

/*	ROM_REGION( 0x800, REGION_CPU4, 0 )		 // ADPCM CPU, 1KB /*/
/*	ROM_LOAD( "sgr-8749.bin", 0x000, 0x800, CRC(9237e8c5) )  // same as above but padded with 00 for different mcu /*/

	ROM_REGION(0x20000, REGION_GFX1, ROMREGION_DISPOSE )	/* Text */
	ROM_LOAD( "23j6-0.18", 0x00000, 0x20000, CRC(86d33df0) SHA1(3419959c28703c5177de9c11b61e1dba9e76aca5) )	/* 0,1,2,3 */
	/* Orientation of bootleg ROMs which are split, but otherwise the same.
	   ROM_LOAD( "sai2.bin", 0x00000, 0x10000, CRC(baa5a3b9) )	   0,1
	   ROM_LOAD( "sai3.bin", 0x10000, 0x10000, CRC(532d59be) )	   2,3
	*/

	ROM_REGION(0x80000, REGION_GFX2, ROMREGION_DISPOSE )	/* Sprites */
	ROM_LOAD( "23j7-0.103",   0x00000, 0x20000, CRC(2f445030) SHA1(3fcf32097e655e963d952d01a30396dc195269ca) )	/* 2,3 */
	ROM_LOAD( "23j8-0.102",   0x20000, 0x20000, CRC(237f725a) SHA1(47bebe5b9878ca10fe6efd4f353717e53a372416) )	/* 2,3 */
	ROM_LOAD( "23j9-0.101",   0x40000, 0x20000, CRC(8caf6097) SHA1(50ad192f831b055586a4a9974f8c6c2f2063ede5) )	/* 0,1 */
	ROM_LOAD( "23ja-0.100",   0x60000, 0x20000, CRC(f678594f) SHA1(4bdcf9407543925f4630a8c7f1f48b85f76343a9) )	/* 0,1 */
	/* Orientation of bootleg ROMs which are split, but otherwise the same
	   ROM_LOAD( "sai23.bin", 0x00000, 0x10000, CRC(12b56225) )	   2,3
	   ROM_LOAD( "sai22.bin", 0x10000, 0x10000, CRC(b592aa9b) )	   2,3
	   ROM_LOAD( "sai21.bin", 0x20000, 0x10000, CRC(a331ba3d) )	   2,3
	   ROM_LOAD( "sai20.bin", 0x30000, 0x10000, CRC(2515d742) )	   2,3
	   ROM_LOAD( "sai19.bin", 0x40000, 0x10000, CRC(d796f2e4) )	   0,1
	   ROM_LOAD( "sai18.bin", 0x50000, 0x10000, CRC(c9e1c2f9) )	   0,1
	   ROM_LOAD( "roku17.bin",0x60000, 0x10000, CRC(00b6db0a) )	   0,1
	   ROM_LOAD( "sai16.bin", 0x70000, 0x10000, CRC(f196818b) )	   0,1
	*/

	ROM_REGION(0x40000, REGION_GFX3, ROMREGION_DISPOSE )	/* Background */
	ROM_LOAD( "a-13", 0x00000, 0x10000, CRC(b745cac4) SHA1(759767ca7c5123b03b9e1a42bb105d194cb76400) )
	ROM_LOAD( "a-12", 0x10000, 0x10000, CRC(3c864299) SHA1(cb12616e4d6c53a82beb4cd51510a632894b359c) )
	ROM_LOAD( "a-15", 0x20000, 0x10000, CRC(2f268f37) SHA1(f82cfe3b2001d5ed2a709ca9c51febcf624bb627) )
	ROM_LOAD( "a-14", 0x30000, 0x10000, CRC(aef814c8) SHA1(f6b9229ca7beb9a0e47d1f6a1083c6102fdd20c8) )

	ROM_REGION(0x40000, REGION_SOUND1, 0 )	/* ADPCM */
	/* These are same as the 128k sample except nibble-swapped */
	/* Some bootlegs have incorrectly halved the ADPCM data !  Bootleggers wrong data */
	ROM_LOAD ( "a-6.bin",   0x00000, 0x10000, CRC(4da4e935) SHA1(235a1589165a23cfad29e07cf66d7c3a777fc904) )	/* 0x8000, 0x7cd47f01 */
	ROM_LOAD ( "a-7.bin",   0x10000, 0x10000, CRC(6284c254) SHA1(e01be1bd4768ae0ccb1cec65b3a6bc80ed7a4b00) )	/* 0x8000, 0x7091959c */
	ROM_LOAD ( "a-10.bin",  0x20000, 0x10000, CRC(b728ec6e) SHA1(433b5f907e4918e89b79bd927e2993ad3030017b) )	/* 0x8000, 0x78349cb6 */
	ROM_LOAD ( "a-11.bin",  0x30000, 0x10000, CRC(a50d1895) SHA1(0c2c1f8a2e945d6c53ce43413f0e63ced45bae17) )	/* 0x8000, 0xaa5b6834 */

	ROM_REGION(0x300, REGION_USER1, 0 )	/* Unknown Bipolar PROMs */
	ROM_LOAD( "23jb-0.16", 0x000, 0x200, CRC(46339529) SHA1(64f4c42a826d67b7cbaa8a23a45ebc4eb6248891) )	/* 82S131 on video board */
	ROM_LOAD( "23j5-0.45", 0x200, 0x100, CRC(fdb130a9) SHA1(4c4f214229b9fab2b5d69c745ec5428787b89e1f) )	/* 82S129 on main board */
ROM_END



/*   ( YEAR  NAME      PARENT    MACHINE   INPUT     INIT    MONITOR COMPANY    FULLNAME     FLAGS ) */
GAME ( 1988, chinagat, 0,        chinagat, chinagat, 0     , ROT0, "[Technos] (Taito Romstar license)", "China Gate (US)" )
GAME ( 1988, saiyugou, chinagat, chinagat, chinagat, 0     , ROT0, "Technos", "Sai Yu Gou Ma Roku (Japan)" )
GAMEX( 1988, saiyugb1, chinagat, saiyugb1, chinagat, 0     , ROT0, "bootleg", "Sai Yu Gou Ma Roku (Japan bootleg 1)", GAME_IMPERFECT_SOUND )
GAME ( 1988, saiyugb2, chinagat, saiyugb2, chinagat, 0     , ROT0, "bootleg", "Sai Yu Gou Ma Roku (Japan bootleg 2)" )
