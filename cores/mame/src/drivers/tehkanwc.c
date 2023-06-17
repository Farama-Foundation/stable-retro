/***************************************************************************

Tehkan World Cup - (c) Tehkan 1985


Ernesto Corvi
ernesto@imagina.com

Roberto Juan Fresca
robbiex@rocketmail.com

TODO:
- dip switches and input ports for Gridiron and Tee'd Off

NOTES:
- Samples MUST be on Memory Region 4

Additional notes (Steph 2002.01.14)

Even if there is NO "screen flipping" for 'tehkanwc' and 'gridiron', there are writes
to 0xfe60 and 0xfe70 of the main CPU with 00 ...

About 'teedoff' :

The main problem with that game is that it should sometimes jumps into shared memory
(see 'init_teedoff' function below) depending on a value that is supposed to be
in the palette RAM !

Palette RAM is reset here (main CPU) :

5D15: ED 57       ld   a,i
5D17: CB FF       set  7,a
5D19: ED 47       ld   i,a
5D1B: AF          xor  a
5D1C: 21 00 D8    ld   hl,$D800
5D1F: 01 80 0C    ld   bc,$0C80
5D22: 77          ld   (hl),a
5D23: 23          inc  hl
5D24: 0D          dec  c
5D25: 20 FB       jr   nz,$5D22
5D27: 0E 80       ld   c,$80
5D29: 10 F7       djnz $5D22
....

Then it is filled here (main CPU) :

5D50: 21 C4 70    ld   hl,$70C4
5D53: 11 00 D8    ld   de,$D800
5D56: 01 00 02    ld   bc,$0200
5D59: ED B0       ldir
5D5B: 21 C4 72    ld   hl,$72C4
5D5E: 01 00 01    ld   bc,$0100
5D61: ED B0       ldir
5D63: C9          ret

0x72c4 is in ROM and it's ALWAYS 00 !

Another place where the palette is filled is here (sub CPU) :

16AC: 21 06 1D    ld   hl,$1D06
16AF: 11 00 DA    ld   de,$DA00
16B2: 01 C0 00    ld   bc,$00C0
16B5: ED B0       ldir

But here again, 0x1d06 is in ROM and it's ALWAYS 00 !

So the "jp z" instruction at 0x0238 of the main CPU will ALWAYS jump
in shared memory when NO code seems to be written !

TO DO :

  - Check MEMORY_* definitions (too many M?A_NOP areas)
  - Check sound in all games (too many messages like this in the .log file :
    'Warning: sound latch 2 written before being read')
  - Figure out the controls in 'tehkanwc' (they are told to be better in MAME 0.34)
  - Figure out the controls in 'teedoff'
  - Confirm "Difficulty" Dip Switch in 'teedoff'

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "cpu/z80/z80.h"


extern UINT8 *tehkanwc_videoram2;

extern WRITE_HANDLER( tehkanwc_videoram_w );
extern WRITE_HANDLER( tehkanwc_colorram_w );
extern WRITE_HANDLER( tehkanwc_videoram2_w );
extern WRITE_HANDLER( tehkanwc_scroll_x_w );
extern WRITE_HANDLER( tehkanwc_scroll_y_w );
extern WRITE_HANDLER( tehkanwc_flipscreen_x_w );
extern WRITE_HANDLER( tehkanwc_flipscreen_y_w );
extern WRITE_HANDLER( gridiron_led0_w );
extern WRITE_HANDLER( gridiron_led1_w );

extern VIDEO_START( tehkanwc );
extern VIDEO_UPDATE( tehkanwc );


static UINT8 *shared_ram;

static READ_HANDLER( shared_r )
{
	return shared_ram[offset];
}

static WRITE_HANDLER( shared_w )
{
	shared_ram[offset] = data;
}

static WRITE_HANDLER( sub_cpu_halt_w )
{
	if (data)
		cpu_set_reset_line(1,CLEAR_LINE);
	else
		cpu_set_reset_line(1,ASSERT_LINE);
}



static int track0[2],track1[2];

static READ_HANDLER( tehkanwc_track_0_r )
{
	int joy;

	joy = readinputport(10) >> (2*offset);
	if (joy & 1) return -63;
	if (joy & 2) return 63;
	return readinputport(3 + offset) - track0[offset];
}

static READ_HANDLER( tehkanwc_track_1_r )
{
	int joy;

	joy = readinputport(10) >> (4+2*offset);
	if (joy & 1) return -63;
	if (joy & 2) return 63;
	return readinputport(6 + offset) - track1[offset];
}

static WRITE_HANDLER( tehkanwc_track_0_reset_w )
{
	/* reset the trackball counters */
	track0[offset] = readinputport(3 + offset) + data;
}

static WRITE_HANDLER( tehkanwc_track_1_reset_w )
{
	/* reset the trackball counters */
	track1[offset] = readinputport(6 + offset) + data;
}



static WRITE_HANDLER( sound_command_w )
{
	soundlatch_w(offset,data);
	cpu_set_irq_line(2,IRQ_LINE_NMI,PULSE_LINE);
}

static void reset_callback(int param)
{
	cpu_set_reset_line(2,PULSE_LINE);
}

static WRITE_HANDLER( sound_answer_w )
{
	soundlatch2_w(0,data);

	/* in Gridiron, the sound CPU goes in a tight loop after the self test, */
	/* probably waiting to be reset by a watchdog */
	if (activecpu_get_pc() == 0x08bc) timer_set(TIME_IN_SEC(1),0,reset_callback);
}


/* Emulate MSM sound samples with counters */

static int msm_data_offs;

static READ_HANDLER( tehkanwc_portA_r )
{
	return msm_data_offs & 0xff;
}

static READ_HANDLER( tehkanwc_portB_r )
{
	return (msm_data_offs >> 8) & 0xff;
}

static WRITE_HANDLER( tehkanwc_portA_w )
{
	msm_data_offs = (msm_data_offs & 0xff00) | data;
}

static WRITE_HANDLER( tehkanwc_portB_w )
{
	msm_data_offs = (msm_data_offs & 0x00ff) | (data << 8);
}

static WRITE_HANDLER( msm_reset_w )
{
	MSM5205_reset_w(0,data ? 0 : 1);
}

void tehkanwc_adpcm_int (int data)
{
	static int toggle;

	UINT8 *SAMPLES = memory_region(REGION_SOUND1);
	int msm_data = SAMPLES[msm_data_offs & 0x7fff];

	if (toggle == 0)
		MSM5205_data_w(0,(msm_data >> 4) & 0x0f);
	else
	{
		MSM5205_data_w(0,msm_data & 0x0f);
		msm_data_offs++;
	}

	toggle ^= 1;
}

/* End of MSM with counters emulation */



static MEMORY_READ_START( readmem )
	{ 0x0000, 0xbfff, MRA_ROM },
	{ 0xc000, 0xc7ff, MRA_RAM },
	{ 0xc800, 0xcfff, shared_r },
	{ 0xd000, 0xd3ff, MRA_RAM },
	{ 0xd400, 0xd7ff, MRA_RAM },
	{ 0xd800, 0xddff, MRA_RAM },
	{ 0xde00, 0xdfff, MRA_RAM },	/* unused part of the palette RAM, I think? Gridiron uses it */
	{ 0xe000, 0xe7ff, MRA_RAM },
	{ 0xe800, 0xebff, MRA_RAM }, /* sprites */
	{ 0xec00, 0xec01, MRA_RAM },
	{ 0xec02, 0xec02, MRA_RAM },
	{ 0xf800, 0xf801, tehkanwc_track_0_r }, /* track 0 x/y */
	{ 0xf802, 0xf802, input_port_9_r }, /* Coin & Start */
	{ 0xf803, 0xf803, input_port_5_r }, /* joy0 - button */
	{ 0xf810, 0xf811, tehkanwc_track_1_r }, /* track 1 x/y */
	{ 0xf813, 0xf813, input_port_8_r }, /* joy1 - button */
	{ 0xf820, 0xf820, soundlatch2_r },	/* answer from the sound CPU */
	{ 0xf840, 0xf840, input_port_0_r }, /* DSW1 */
	{ 0xf850, 0xf850, input_port_1_r },	/* DSW2 */
	{ 0xf860, 0xf860, watchdog_reset_r },
	{ 0xf870, 0xf870, input_port_2_r }, /* DSW3 */
MEMORY_END

static MEMORY_WRITE_START( writemem )
	{ 0x0000, 0xbfff, MWA_ROM },
	{ 0xc000, 0xc7ff, MWA_RAM },
	{ 0xc800, 0xcfff, shared_w, &shared_ram },
	{ 0xd000, 0xd3ff, tehkanwc_videoram_w, &videoram },
	{ 0xd400, 0xd7ff, tehkanwc_colorram_w, &colorram },
	{ 0xd800, 0xddff, paletteram_xxxxBBBBGGGGRRRR_swap_w, &paletteram },
	{ 0xde00, 0xdfff, MWA_RAM },	/* unused part of the palette RAM, I think? Gridiron uses it */
	{ 0xe000, 0xe7ff, tehkanwc_videoram2_w, &tehkanwc_videoram2 },
	{ 0xe800, 0xebff, spriteram_w, &spriteram, &spriteram_size }, /* sprites */
	{ 0xec00, 0xec01, tehkanwc_scroll_x_w },
	{ 0xec02, 0xec02, tehkanwc_scroll_y_w },
	{ 0xf800, 0xf801, tehkanwc_track_0_reset_w },
	{ 0xf802, 0xf802, gridiron_led0_w },
	{ 0xf810, 0xf811, tehkanwc_track_1_reset_w },
	{ 0xf812, 0xf812, gridiron_led1_w },
	{ 0xf820, 0xf820, sound_command_w },
	{ 0xf840, 0xf840, sub_cpu_halt_w },
	{ 0xf850, 0xf850, MWA_NOP },				/* ?? writes 0x00 or 0xff */
	{ 0xf860, 0xf860, tehkanwc_flipscreen_x_w },		/* Check if it's really X */
	{ 0xf870, 0xf870, tehkanwc_flipscreen_y_w },		/* Check if it's really Y */
MEMORY_END

static MEMORY_READ_START( gridiron_readmem )
	{ 0x0000, 0xbfff, MRA_ROM },
	{ 0xc000, 0xc7ff, MRA_RAM },
	{ 0xc800, 0xcfff, shared_r },
	{ 0xd000, 0xd3ff, MRA_RAM },
	{ 0xd400, 0xd7ff, MRA_RAM },
	{ 0xd800, 0xddff, MRA_RAM },
	{ 0xde00, 0xdfff, MRA_RAM },	/* unused part of the palette RAM, I think? Gridiron uses it */
	{ 0xe000, 0xe7ff, MRA_RAM },
	{ 0xe800, 0xebff, MRA_RAM }, /* sprites */
	{ 0xec00, 0xec01, MRA_RAM },
	{ 0xec02, 0xec02, MRA_RAM },
	{ 0xf800, 0xf801, tehkanwc_track_0_r }, /* track 0 x/y */
	{ 0xf802, 0xf802, input_port_9_r }, /* Coin & Start */
	{ 0xf803, 0xf803, input_port_5_r }, /* joy0 - button */
	{ 0xf810, 0xf811, tehkanwc_track_1_r }, /* track 1 x/y */
	{ 0xf813, 0xf813, input_port_8_r }, /* joy1 - button */
	{ 0xf820, 0xf820, soundlatch2_r },	/* answer from the sound CPU */
	{ 0xf840, 0xf840, input_port_0_r }, /* DSW1 */
	{ 0xf850, 0xf850, input_port_1_r },	/* DSW2 */
	{ 0xf860, 0xf860, watchdog_reset_r },
	{ 0xf870, 0xf870, MRA_NOP },	/* ?? read in the IRQ handler */
MEMORY_END

static MEMORY_WRITE_START( gridiron_writemem )
	{ 0x0000, 0xbfff, MWA_ROM },
	{ 0xc000, 0xc7ff, MWA_RAM },
	{ 0xc800, 0xcfff, shared_w, &shared_ram },
	{ 0xd000, 0xd3ff, tehkanwc_videoram_w, &videoram },
	{ 0xd400, 0xd7ff, tehkanwc_colorram_w, &colorram },
	{ 0xd800, 0xddff, paletteram_xxxxBBBBGGGGRRRR_swap_w, &paletteram },
	{ 0xde00, 0xdfff, MWA_RAM },	/* unused part of the palette RAM, I think? Gridiron uses it */
	{ 0xe000, 0xe7ff, tehkanwc_videoram2_w, &tehkanwc_videoram2 },
	{ 0xe800, 0xebff, spriteram_w, &spriteram, &spriteram_size }, /* sprites */
	{ 0xec00, 0xec01, tehkanwc_scroll_x_w },
	{ 0xec02, 0xec02, tehkanwc_scroll_y_w },
	{ 0xf800, 0xf801, tehkanwc_track_0_reset_w },
	{ 0xf802, 0xf802, gridiron_led0_w },
	{ 0xf810, 0xf811, tehkanwc_track_1_reset_w },
	{ 0xf812, 0xf812, gridiron_led1_w },
	{ 0xf820, 0xf820, sound_command_w },
	{ 0xf840, 0xf840, sub_cpu_halt_w },
	{ 0xf850, 0xf850, MWA_NOP },				/* ?? writes 0x00 or 0xff */
	{ 0xf860, 0xf860, tehkanwc_flipscreen_x_w },		/* Check if it's really X */
	{ 0xf870, 0xf870, tehkanwc_flipscreen_y_w },		/* Check if it's really Y */
MEMORY_END

static MEMORY_READ_START( teedoff_readmem )
	{ 0x0000, 0xbfff, MRA_ROM },
	{ 0xc000, 0xc7ff, MRA_RAM },
	{ 0xc800, 0xcfff, shared_r },
	{ 0xd000, 0xd3ff, MRA_RAM },
	{ 0xd400, 0xd7ff, MRA_RAM },
	{ 0xd800, 0xddff, MRA_RAM },
	{ 0xde00, 0xdfff, MRA_RAM },	/* unused part of the palette RAM, I think? Gridiron uses it */
	{ 0xe000, 0xe7ff, MRA_RAM },
	{ 0xe800, 0xebff, MRA_RAM }, /* sprites */
	{ 0xec00, 0xec01, MRA_RAM },
	{ 0xec02, 0xec02, MRA_RAM },
	{ 0xf800, 0xf801, tehkanwc_track_0_r }, /* track 0 x/y */
	{ 0xf802, 0xf802, input_port_9_r }, /* Coin */
	{ 0xf803, 0xf803, input_port_5_r }, /* joy0 - button */
	{ 0xf806, 0xf806, input_port_9_r }, /* Start */
	{ 0xf810, 0xf811, tehkanwc_track_1_r }, /* track 1 x/y */
	{ 0xf813, 0xf813, input_port_8_r }, /* joy1 - button */
	{ 0xf820, 0xf820, soundlatch2_r },	/* answer from the sound CPU */
	{ 0xf840, 0xf840, input_port_0_r }, /* DSW1 */
	{ 0xf850, 0xf850, input_port_1_r },	/* DSW2 */
	{ 0xf860, 0xf860, watchdog_reset_r },
MEMORY_END

static MEMORY_WRITE_START( teedoff_writemem )
	{ 0x0000, 0xbfff, MWA_ROM },
	{ 0xc000, 0xc7ff, MWA_RAM },
	{ 0xc800, 0xcfff, shared_w, &shared_ram },
	{ 0xd000, 0xd3ff, tehkanwc_videoram_w, &videoram },
	{ 0xd400, 0xd7ff, tehkanwc_colorram_w, &colorram },
	{ 0xd800, 0xddff, paletteram_xxxxBBBBGGGGRRRR_swap_w, &paletteram },
	{ 0xde00, 0xdfff, MWA_RAM },	/* unused part of the palette RAM, I think? Gridiron uses it */
	{ 0xe000, 0xe7ff, tehkanwc_videoram2_w, &tehkanwc_videoram2 },
	{ 0xe800, 0xebff, spriteram_w, &spriteram, &spriteram_size }, /* sprites */
	{ 0xec00, 0xec01, tehkanwc_scroll_x_w },
	{ 0xec02, 0xec02, tehkanwc_scroll_y_w },
	{ 0xf800, 0xf801, tehkanwc_track_0_reset_w },
	{ 0xf802, 0xf802, gridiron_led0_w },
	{ 0xf810, 0xf811, tehkanwc_track_1_reset_w },
	{ 0xf812, 0xf812, gridiron_led1_w },
	{ 0xf820, 0xf820, sound_command_w },
	{ 0xf840, 0xf840, sub_cpu_halt_w },
	{ 0xf850, 0xf850, MWA_NOP },				/* ?? Same value as in 0xf840 */
	{ 0xf860, 0xf860, tehkanwc_flipscreen_x_w },		/* Check if it's really X */
	{ 0xf870, 0xf870, tehkanwc_flipscreen_y_w },		/* Check if it's really Y */
MEMORY_END

static MEMORY_READ_START( readmem_sub )
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0xc7ff, MRA_RAM },
	{ 0xc800, 0xcfff, shared_r },
	{ 0xd000, 0xd3ff, MRA_RAM },
	{ 0xd400, 0xd7ff, MRA_RAM },
	{ 0xd800, 0xddff, MRA_RAM },
	{ 0xde00, 0xdfff, MRA_RAM },	/* unused part of the palette RAM, I think? Gridiron uses it */
	{ 0xe000, 0xe7ff, MRA_RAM },
	{ 0xe800, 0xebff, MRA_RAM }, /* sprites */
	{ 0xec00, 0xec01, MRA_RAM },
	{ 0xec02, 0xec02, MRA_RAM },
	{ 0xf860, 0xf860, watchdog_reset_r },
MEMORY_END

static MEMORY_WRITE_START( writemem_sub )
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0xc000, 0xc7ff, MWA_RAM },
	{ 0xc800, 0xcfff, shared_w },
	{ 0xd000, 0xd3ff, tehkanwc_videoram_w },
	{ 0xd400, 0xd7ff, tehkanwc_colorram_w },
	{ 0xd800, 0xddff, paletteram_xxxxBBBBGGGGRRRR_swap_w, &paletteram },
	{ 0xde00, 0xdfff, MWA_RAM },	/* unused part of the palette RAM, I think? Gridiron uses it */
	{ 0xe000, 0xe7ff, tehkanwc_videoram2_w },
	{ 0xe800, 0xebff, spriteram_w }, /* sprites */
	{ 0xec00, 0xec01, tehkanwc_scroll_x_w },
	{ 0xec02, 0xec02, tehkanwc_scroll_y_w },
MEMORY_END

static MEMORY_READ_START( readmem_sound )
	{ 0x0000, 0x3fff, MRA_ROM },
	{ 0x4000, 0x47ff, MRA_RAM },
	{ 0xc000, 0xc000, soundlatch_r },
MEMORY_END

static MEMORY_WRITE_START( writemem_sound )
	{ 0x0000, 0x3fff, MWA_ROM },
	{ 0x4000, 0x47ff, MWA_RAM },
	{ 0x8001, 0x8001, msm_reset_w },/* MSM51xx reset */
	{ 0x8002, 0x8002, MWA_NOP },	/* ?? written in the IRQ handler */
	{ 0x8003, 0x8003, MWA_NOP },	/* ?? written in the NMI handler */
	{ 0xc000, 0xc000, sound_answer_w },	/* answer for main CPU */
MEMORY_END

static PORT_READ_START( sound_readport )
	{ 0x00, 0x00, AY8910_read_port_0_r },
	{ 0x02, 0x02, AY8910_read_port_1_r },
PORT_END

static PORT_WRITE_START( sound_writeport )
	{ 0x00, 0x00, AY8910_write_port_0_w },
	{ 0x01, 0x01, AY8910_control_port_0_w },
	{ 0x02, 0x02, AY8910_write_port_1_w },
	{ 0x03, 0x03, AY8910_control_port_1_w },
PORT_END



INPUT_PORTS_START( tehkanwc )
	PORT_START /* DSW1 - Active LOW */
	PORT_DIPNAME( 0x07, 0x07, DEF_STR( Coin_A ) )
	PORT_DIPSETTING (   0x01, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING (   0x07, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING (   0x00, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING (   0x06, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING (   0x05, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING (   0x04, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING (   0x03, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING (   0x02, DEF_STR( 1C_6C ) )
	PORT_DIPNAME( 0x38, 0x38, DEF_STR( Coin_B ) )
	PORT_DIPSETTING (   0x08, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING (   0x38, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING (   0x00, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING (   0x30, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING (   0x28, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING (   0x20, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING (   0x18, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING (   0x10, DEF_STR( 1C_6C ) )
	PORT_DIPNAME( 0xc0, 0xc0, "Start Credits (P1&P2)/Extra" )
	PORT_DIPSETTING (   0x80, "1&1/200%" )
	PORT_DIPSETTING (   0xc0, "1&2/100%" )
	PORT_DIPSETTING (   0x40, "2&2/100%" )
	PORT_DIPSETTING (   0x00, "2&3/67%" )

	PORT_START /* DSW2 - Active LOW */
	PORT_DIPNAME( 0x03, 0x03, "1P Game Time" )
	PORT_DIPSETTING (   0x00, "2:30" )
	PORT_DIPSETTING (   0x01, "2:00" )
	PORT_DIPSETTING (   0x03, "1:30" )
	PORT_DIPSETTING (   0x02, "1:00" )
	PORT_DIPNAME( 0x7c, 0x7c, "2P Game Time" )
	PORT_DIPSETTING (   0x00, "5:00/3:00 Extra" )
	PORT_DIPSETTING (   0x60, "5:00/2:45 Extra" )
	PORT_DIPSETTING (   0x20, "5:00/2:35 Extra" )
	PORT_DIPSETTING (   0x40, "5:00/2:30 Extra" )
	PORT_DIPSETTING (   0x04, "4:00/2:30 Extra" )
	PORT_DIPSETTING (   0x64, "4:00/2:15 Extra" )
	PORT_DIPSETTING (   0x24, "4:00/2:05 Extra" )
	PORT_DIPSETTING (   0x44, "4:00/2:00 Extra" )
	PORT_DIPSETTING (   0x1c, "3:30/2:15 Extra" )
	PORT_DIPSETTING (   0x7c, "3:30/2:00 Extra" )
	PORT_DIPSETTING (   0x3c, "3:30/1:50 Extra" )
	PORT_DIPSETTING (   0x5c, "3:30/1:45 Extra" )
	PORT_DIPSETTING (   0x08, "3:00/2:00 Extra" )
	PORT_DIPSETTING (   0x68, "3:00/1:45 Extra" )
	PORT_DIPSETTING (   0x28, "3:00/1:35 Extra" )
	PORT_DIPSETTING (   0x48, "3:00/1:30 Extra" )
	PORT_DIPSETTING (   0x0c, "2:30/1:45 Extra" )
	PORT_DIPSETTING (   0x6c, "2:30/1:30 Extra" )
	PORT_DIPSETTING (   0x2c, "2:30/1:20 Extra" )
	PORT_DIPSETTING (   0x4c, "2:30/1:15 Extra" )
	PORT_DIPSETTING (   0x10, "2:00/1:30 Extra" )
	PORT_DIPSETTING (   0x70, "2:00/1:15 Extra" )
	PORT_DIPSETTING (   0x30, "2:00/1:05 Extra" )
	PORT_DIPSETTING (   0x50, "2:00/1:00 Extra" )
	PORT_DIPSETTING (   0x14, "1:30/1:15 Extra" )
	PORT_DIPSETTING (   0x74, "1:30/1:00 Extra" )
	PORT_DIPSETTING (   0x34, "1:30/0:50 Extra" )
	PORT_DIPSETTING (   0x54, "1:30/0:45 Extra" )
	PORT_DIPSETTING (   0x18, "1:00/1:00 Extra" )
	PORT_DIPSETTING (   0x78, "1:00/0:45 Extra" )
	PORT_DIPSETTING (   0x38, "1:00/0:35 Extra" )
	PORT_DIPSETTING (   0x58, "1:00/0:30 Extra" )
	PORT_DIPNAME( 0x80, 0x80, "Game Type" )
	PORT_DIPSETTING (   0x80, "Timer In" )
	PORT_DIPSETTING (   0x00, "Credit In" )

	PORT_START /* DSW3 - Active LOW */
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Difficulty ) )
	PORT_DIPSETTING (   0x02, "Easy" )
	PORT_DIPSETTING (   0x03, "Normal" )
	PORT_DIPSETTING (   0x01, "Hard" )
	PORT_DIPSETTING (   0x00, "Very Hard" )
	PORT_DIPNAME( 0x04, 0x04, "Timer Speed" )
	PORT_DIPSETTING (   0x04, "60/60" )
	PORT_DIPSETTING (   0x00, "55/60" )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING (   0x00, DEF_STR( Off ) )
	PORT_DIPSETTING (   0x08, DEF_STR( On ) )

	PORT_START /* IN0 - X AXIS */
	PORT_ANALOGX( 0xff, 0x80, IPT_TRACKBALL_X | IPF_PLAYER1, 100, 0, 0, 0, IP_KEY_NONE, IP_KEY_NONE, IP_JOY_NONE, IP_JOY_NONE )

	PORT_START /* IN0 - Y AXIS */
	PORT_ANALOGX( 0xff, 0x80, IPT_TRACKBALL_Y | IPF_PLAYER1, 100, 0, 0, 0, IP_KEY_NONE, IP_KEY_NONE, IP_JOY_NONE, IP_JOY_NONE )

	PORT_START /* IN0 - BUTTON */
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )

	PORT_START /* IN1 - X AXIS */
	PORT_ANALOGX( 0xff, 0x80, IPT_TRACKBALL_X | IPF_PLAYER2, 100, 0, 0, 0, IP_KEY_NONE, IP_KEY_NONE, IP_JOY_NONE, IP_JOY_NONE )

	PORT_START /* IN1 - Y AXIS */
	PORT_ANALOGX( 0xff, 0x80, IPT_TRACKBALL_Y | IPF_PLAYER2, 100, 0, 0, 0, IP_KEY_NONE, IP_KEY_NONE, IP_JOY_NONE, IP_JOY_NONE )

	PORT_START /* IN1 - BUTTON */
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )

	PORT_START /* IN2 - Active LOW */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START2 )

	PORT_START	/* fake port to emulate trackballs with keyboard */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
INPUT_PORTS_END


INPUT_PORTS_START( gridiron )
	PORT_START /* DSW1 - Active LOW */
	PORT_DIPNAME( 0x03, 0x03, "Start Credits (P1&P2)/Extra" )
	PORT_DIPSETTING (   0x01, "1&1/200%" )
	PORT_DIPSETTING (   0x03, "1&2/100%" )
/*	PORT_DIPSETTING (   0x00, "2&1/200%" )				*/ /* Is this setting possible ?*/
	PORT_DIPSETTING (   0x02, "2&2/100%" )
	/* This Dip Switch only has an effect in a 2 players game.
	   If offense player selects his formation before defense player,
	   defense formation time will be set to 3, 5 or 7 seconds.
	   Check code at 0x3ed9 and table at 0x3f89. */
	PORT_DIPNAME( 0x0c, 0x0c, "Formation Time (Defense)" )
	PORT_DIPSETTING (   0x0c, "Same as Offense" )
	PORT_DIPSETTING (   0x00, "7" )
	PORT_DIPSETTING (   0x08, "5" )
	PORT_DIPSETTING (   0x04, "3" )
	PORT_DIPNAME( 0x30, 0x30, "Timer Speed" )
	PORT_DIPSETTING (   0x30, "60/60" )
	PORT_DIPSETTING (   0x00, "57/60" )
	PORT_DIPSETTING (   0x10, "54/60" )
	PORT_DIPSETTING (   0x20, "50/60" )
	PORT_DIPNAME( 0xc0, 0xc0, "Formation Time (Offense)" )
	PORT_DIPSETTING (   0x00, "25" )
	PORT_DIPSETTING (   0x40, "20" )
	PORT_DIPSETTING (   0xc0, "15" )
	PORT_DIPSETTING (   0x80, "10" )

	PORT_START /* DSW2 - Active LOW */
	PORT_DIPNAME( 0x03, 0x03, "1P Game Time" )
	PORT_DIPSETTING (   0x00, "2:30" )
	PORT_DIPSETTING (   0x01, "2:00" )
	PORT_DIPSETTING (   0x03, "1:30" )
	PORT_DIPSETTING (   0x02, "1:00" )
	PORT_DIPNAME( 0x7c, 0x7c, "2P Game Time" )
	PORT_DIPSETTING (   0x60, "5:00/3:00 Extra" )
	PORT_DIPSETTING (   0x00, "5:00/2:45 Extra" )
	PORT_DIPSETTING (   0x20, "5:00/2:35 Extra" )
	PORT_DIPSETTING (   0x40, "5:00/2:30 Extra" )
	PORT_DIPSETTING (   0x64, "4:00/2:30 Extra" )
	PORT_DIPSETTING (   0x04, "4:00/2:15 Extra" )
	PORT_DIPSETTING (   0x24, "4:00/2:05 Extra" )
	PORT_DIPSETTING (   0x44, "4:00/2:00 Extra" )
	PORT_DIPSETTING (   0x68, "3:30/2:15 Extra" )
	PORT_DIPSETTING (   0x08, "3:30/2:00 Extra" )
	PORT_DIPSETTING (   0x28, "3:30/1:50 Extra" )
	PORT_DIPSETTING (   0x48, "3:30/1:45 Extra" )
	PORT_DIPSETTING (   0x6c, "3:00/2:00 Extra" )
	PORT_DIPSETTING (   0x0c, "3:00/1:45 Extra" )
	PORT_DIPSETTING (   0x2c, "3:00/1:35 Extra" )
	PORT_DIPSETTING (   0x4c, "3:00/1:30 Extra" )
	PORT_DIPSETTING (   0x7c, "2:30/1:45 Extra" )
	PORT_DIPSETTING (   0x1c, "2:30/1:30 Extra" )
	PORT_DIPSETTING (   0x3c, "2:30/1:20 Extra" )
	PORT_DIPSETTING (   0x5c, "2:30/1:15 Extra" )
	PORT_DIPSETTING (   0x70, "2:00/1:30 Extra" )
	PORT_DIPSETTING (   0x10, "2:00/1:15 Extra" )
	PORT_DIPSETTING (   0x30, "2:00/1:05 Extra" )
	PORT_DIPSETTING (   0x50, "2:00/1:00 Extra" )
	PORT_DIPSETTING (   0x74, "1:30/1:15 Extra" )
	PORT_DIPSETTING (   0x14, "1:30/1:00 Extra" )
	PORT_DIPSETTING (   0x34, "1:30/0:50 Extra" )
	PORT_DIPSETTING (   0x54, "1:30/0:45 Extra" )
	PORT_DIPSETTING (   0x78, "1:00/1:00 Extra" )
	PORT_DIPSETTING (   0x18, "1:00/0:45 Extra" )
	PORT_DIPSETTING (   0x38, "1:00/0:35 Extra" )
	PORT_DIPSETTING (   0x58, "1:00/0:30 Extra" )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Demo_Sounds ) )		/* Check code at 0x14b4*/
	PORT_DIPSETTING (   0x00, DEF_STR( Off ) )
	PORT_DIPSETTING (   0x80, DEF_STR( On ) )

	PORT_START /* no DSW3 */
	PORT_BIT( 0x0f, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START /* IN0 - X AXIS */
	PORT_ANALOG( 0xff, 0x80, IPT_TRACKBALL_X | IPF_PLAYER1, 100, 63, 0, 0 )

	PORT_START /* IN0 - Y AXIS */
	PORT_ANALOG( 0xff, 0x80, IPT_TRACKBALL_Y | IPF_PLAYER1, 100, 63, 0, 0 )

	PORT_START /* IN0 - BUTTON */
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )

	PORT_START /* IN1 - X AXIS */
	PORT_ANALOG( 0xff, 0x80, IPT_TRACKBALL_X | IPF_PLAYER2, 100, 63, 0, 0 )

	PORT_START /* IN1 - Y AXIS */
	PORT_ANALOG( 0xff, 0x80, IPT_TRACKBALL_Y | IPF_PLAYER2, 100, 63, 0, 0 )

	PORT_START /* IN1 - BUTTON */
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )

	PORT_START /* IN2 - Active LOW */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START2 )

	PORT_START	/* no fake port here */
	PORT_BIT( 0xff, IP_ACTIVE_HIGH, IPT_UNUSED )
INPUT_PORTS_END


INPUT_PORTS_START( teedoff )
	PORT_START /* DSW1 - Active LOW */
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Coin_A ) )
	PORT_DIPSETTING (   0x02, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING (   0x03, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING (   0x01, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING (   0x00, DEF_STR( 1C_3C ) )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Coin_B ) )
	PORT_DIPSETTING (   0x08, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING (   0x0c, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING (   0x04, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING (   0x00, DEF_STR( 1C_3C ) )
	PORT_DIPNAME( 0x30, 0x30, "Balls" )
	PORT_DIPSETTING (   0x30, "5" )
	PORT_DIPSETTING (   0x20, "6" )
	PORT_DIPSETTING (   0x10, "7" )
	PORT_DIPSETTING (   0x00, "8" )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Cabinet ) )			/* Check code at 0x0c5c*/
	PORT_DIPSETTING (   0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING (   0x40, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Demo_Sounds ) )		/* Check code at 0x5dd0*/
	PORT_DIPSETTING (   0x00, DEF_STR( Off ) )
	PORT_DIPSETTING (   0x80, DEF_STR( On ) )

	PORT_START /* DSW2 - Active LOW */
	PORT_BIT( 0x07, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_DIPNAME( 0x18, 0x18, "Penalty (Over Par)" )		/* Check table at 0x2d67*/
	PORT_DIPSETTING (   0x10, "1/1/2/3/4" )				/* +1 / +2 / +3 / +4 / +5 or +6*/
	PORT_DIPSETTING (   0x18, "1/2/3/3/4" )
	PORT_DIPSETTING (   0x08, "1/2/3/4/4" )
	PORT_DIPSETTING (   0x00, "2/3/3/4/4" )
	PORT_DIPNAME( 0x20, 0x20, "Bonus Balls (Multiple coins)" )
	PORT_DIPSETTING (   0x20, "None" )
	PORT_DIPSETTING (   0x00, "+1" )
	PORT_DIPNAME( 0xc0, 0xc0, "Difficulty?" )				/* Check table at 0x5df9*/
	PORT_DIPSETTING (   0x80, "Easy" )
	PORT_DIPSETTING (   0xc0, "Normal" )
	PORT_DIPSETTING (   0x40, "Hard" )
	PORT_DIPSETTING (   0x00, "Hardest" )

	PORT_START /* no DSW3 */
	PORT_BIT( 0x0f, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START /* IN0 - X AXIS */
	PORT_ANALOG( 0xff, 0x80, IPT_TRACKBALL_X | IPF_PLAYER1, 100, 63, 0, 0 )

	PORT_START /* IN0 - Y AXIS */
	PORT_ANALOG( 0xff, 0x80, IPT_TRACKBALL_Y | IPF_PLAYER1, 100, 63, 0, 0 )

	PORT_START /* IN0 - BUTTON */
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )

	PORT_START /* IN1 - X AXIS */
	PORT_ANALOG( 0xff, 0x80, IPT_TRACKBALL_X | IPF_PLAYER2, 100, 63, 0, 0 )

	PORT_START /* IN1 - Y AXIS */
	PORT_ANALOG( 0xff, 0x80, IPT_TRACKBALL_Y | IPF_PLAYER2, 100, 63, 0, 0 )

	PORT_START /* IN1 - BUTTON */
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )

	PORT_START /* IN2 - Active LOW */
	/* "Coin"  buttons are read from address 0xf802 */
	/* "Start" buttons are read from address 0xf806 */
	/* coin input must be active between 2 and 15 frames to be consistently recognized */
	PORT_BIT_IMPULSE( 0x01, IP_ACTIVE_LOW, IPT_COIN1, 2 )
	PORT_BIT_IMPULSE( 0x02, IP_ACTIVE_LOW, IPT_COIN2, 2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START2 )

	PORT_START	/* no fake port here */
	PORT_BIT( 0xff, IP_ACTIVE_HIGH, IPT_UNUSED )
INPUT_PORTS_END




static struct GfxLayout charlayout =
{
	8,8,	/* 8*8 characters */
	512,	/* 512 characters */
	4,	/* 4 bits per pixel */
	{ 0, 1, 2, 3 },	/* the bitplanes are packed in one nibble */
	{ 1*4, 0*4, 3*4, 2*4, 5*4, 4*4, 7*4, 6*4 },
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32 },
	32*8	/* every char takes 32 consecutive bytes */
};

static struct GfxLayout spritelayout =
{
	16,16,	/* 16*16 sprites */
	512,	/* 512 sprites */
	4,	/* 4 bits per pixel */
	{ 0, 1, 2, 3 },	/* the bitplanes are packed in one nibble */
	{ 1*4, 0*4, 3*4, 2*4, 5*4, 4*4, 7*4, 6*4,
			8*32+1*4, 8*32+0*4, 8*32+3*4, 8*32+2*4, 8*32+5*4, 8*32+4*4, 8*32+7*4, 8*32+6*4 },
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32,
			16*32, 17*32, 18*32, 19*32, 20*32, 21*32, 22*32, 23*32 },
	128*8	/* every char takes 32 consecutive bytes */
};

static struct GfxLayout tilelayout =
{
	16,8,	/* 16*8 characters */
	1024,	/* 1024 characters */
	4,	/* 4 bits per pixel */
	{ 0, 1, 2, 3 },	/* the bitplanes are packed in one nibble */
	{ 1*4, 0*4, 3*4, 2*4, 5*4, 4*4, 7*4, 6*4,
		32*8+1*4, 32*8+0*4, 32*8+3*4, 32*8+2*4, 32*8+5*4, 32*8+4*4, 32*8+7*4, 32*8+6*4 },
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32 },
	64*8	/* every char takes 64 consecutive bytes */
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &charlayout,     0, 16 }, /* Colors 0 - 255 */
	{ REGION_GFX2, 0, &spritelayout, 256,  8 }, /* Colors 256 - 383 */
	{ REGION_GFX3, 0, &tilelayout,   512, 16 }, /* Colors 512 - 767 */
	{ -1 } /* end of array */
};



static struct AY8910interface ay8910_interface =
{
	2,	/* 2 chips */
	1536000, 	/* ??? */
	{ 25, 25 },
	{ 0, tehkanwc_portA_r },
	{ 0, tehkanwc_portB_r },
	{ tehkanwc_portA_w, 0 },
	{ tehkanwc_portB_w, 0 }
};

static struct MSM5205interface msm5205_interface =
{
	1,					/* 1 chip             */
	384000,				/* 384KHz             */
	{ tehkanwc_adpcm_int },/* interrupt function */
	{ MSM5205_S48_4B },	/* 8KHz               */
	{ 45 }
};

static MACHINE_DRIVER_START( tehkanwc )

	/* basic machine hardware */
	MDRV_CPU_ADD_TAG("main", Z80, 4608000)	/* 18.432000 / 4 */
	MDRV_CPU_MEMORY(readmem,writemem)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,1)

	MDRV_CPU_ADD(Z80, 4608000)	/* 18.432000 / 4 */
	MDRV_CPU_MEMORY(readmem_sub,writemem_sub)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,1)

	MDRV_CPU_ADD(Z80, 4608000)	/* 18.432000 / 4; communication is bidirectional, can't mark it as AUDIO_CPU */
	MDRV_CPU_MEMORY(readmem_sound,writemem_sound)
	MDRV_CPU_PORTS(sound_readport,sound_writeport)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,1)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(10)	/* 10 CPU slices per frame - seems enough to keep the CPUs in sync */

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 2*8, 30*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(768)

	MDRV_VIDEO_START(tehkanwc)
	MDRV_VIDEO_UPDATE(tehkanwc)

	/* sound hardware */
	MDRV_SOUND_ADD(AY8910, ay8910_interface)
	MDRV_SOUND_ADD(MSM5205, msm5205_interface)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( gridiron )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(tehkanwc)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_MEMORY(gridiron_readmem,gridiron_writemem)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( teedoff )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(tehkanwc)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_MEMORY(teedoff_readmem,teedoff_writemem)
MACHINE_DRIVER_END



static DRIVER_INIT( teedoff )
{
	/* Patch to avoid the game jumping in shared memory */

	/* Code at 0x0233 (main CPU) :

		0233: 3A 00 DA    ld   a,($DA00)
		0236: CB 7F       bit  7,a
		0238: CA 00 C8    jp   z,$C800

	   changed to :

		0233: 3A 00 DA    ld   a,($DA00)
		0236: CB 7F       bit  7,a
		0238: 00          nop
		0239: 00          nop
		023A: 00          nop
	*/

	data8_t *ROM = memory_region(REGION_CPU1);

	ROM[0x0238] = 0x00;
	ROM[0x0239] = 0x00;
	ROM[0x023a] = 0x00;
}



/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( tehkanwc )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "twc-1.bin",    0x0000, 0x4000, CRC(34d6d5ff) SHA1(72f4d408b8a7766d348f6a229d395e0c98215c40) )
	ROM_LOAD( "twc-2.bin",    0x4000, 0x4000, CRC(7017a221) SHA1(4b4700af0a6ff64f976db369ba4b9d97cee1fd5f) )
	ROM_LOAD( "twc-3.bin",    0x8000, 0x4000, CRC(8b662902) SHA1(13bcd4bf23e34dd7193545561e05bb2cb2c95f9b) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for code */
	ROM_LOAD( "twc-4.bin",    0x0000, 0x8000, CRC(70a9f883) SHA1(ace04359265271eb37512a89eb0217eb013aecb7) )

	ROM_REGION( 0x10000, REGION_CPU3, 0 )	/* 64k for code */
	ROM_LOAD( "twc-6.bin",    0x0000, 0x4000, CRC(e3112be2) SHA1(7859e51b4312dc5df01c88e1d97cf608abc7ca72) )

	ROM_REGION( 0x04000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "twc-12.bin",   0x00000, 0x4000, CRC(a9e274f8) SHA1(02b46e1b149a856f0be74a23faaeb792935b66c7) )	/* fg tiles */

	ROM_REGION( 0x10000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "twc-8.bin",    0x00000, 0x8000, CRC(055a5264) SHA1(fe294ba57c2c858952e2fab0be1b8859730846cb) )	/* sprites */
	ROM_LOAD( "twc-7.bin",    0x08000, 0x8000, CRC(59faebe7) SHA1(85dad90928369601e039467d575750539410fcf6) )

	ROM_REGION( 0x10000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "twc-11.bin",   0x00000, 0x8000, CRC(669389fc) SHA1(a93e8455060ce5242cb65f78e47b4840aa13ab13) )	/* bg tiles */
	ROM_LOAD( "twc-9.bin",    0x08000, 0x8000, CRC(347ef108) SHA1(bb9c2f51d65f28655404e10c3be44d7ade98711b) )

	ROM_REGION( 0x8000, REGION_SOUND1, 0 )	/* ADPCM samples */
	ROM_LOAD( "twc-5.bin",    0x0000, 0x4000, CRC(444b5544) SHA1(0786d6d9ada7fe49c8ab9751b049095474d2e598) )
ROM_END

ROM_START( gridiron )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "gfight1.bin",  0x0000, 0x4000, CRC(51612741) SHA1(a0417a35f0ce51ba7fc81f27b356852a97f52a58) )
	ROM_LOAD( "gfight2.bin",  0x4000, 0x4000, CRC(a678db48) SHA1(5ddcb93b3ed52cec6ba04bb19832ae239b7d2287) )
	ROM_LOAD( "gfight3.bin",  0x8000, 0x4000, CRC(8c227c33) SHA1(c0b58dbebc159ee681aed33c858f5e0172edd75a) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for code */
	ROM_LOAD( "gfight4.bin",  0x0000, 0x4000, CRC(8821415f) SHA1(772ce0770ed869ebf625d210bc2b9c381b14b7ea) )

	ROM_REGION( 0x10000, REGION_CPU3, 0 )	/* 64k for code */
	ROM_LOAD( "gfight5.bin",  0x0000, 0x4000, CRC(92ca3c07) SHA1(580077ca8cf01996b29497187e41a54242de7f50) )

	ROM_REGION( 0x04000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "gfight7.bin",  0x00000, 0x4000, CRC(04390cca) SHA1(ff010c0c18ddd1f793b581f0a70bc1b98ef7d21d) )	/* fg tiles */

	ROM_REGION( 0x10000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "gfight8.bin",  0x00000, 0x4000, CRC(5de6a70f) SHA1(416aba9de59d46861671c49f8ca33489db1b8634) )	/* sprites */
	ROM_LOAD( "gfight9.bin",  0x04000, 0x4000, CRC(eac9dc16) SHA1(8b3cf87ede8aba45752cc2651a471a5942570037) )
	ROM_LOAD( "gfight10.bin", 0x08000, 0x4000, CRC(61d0690f) SHA1(cd7c81b0e5356bc865380cae5582d6c6b017dfa1) )
	/* 0c000-0ffff empty */

	ROM_REGION( 0x10000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "gfight11.bin", 0x00000, 0x4000, CRC(80b09c03) SHA1(41627bb6d0f163430c1709a449a42f0f216da852) )	/* bg tiles */
	ROM_LOAD( "gfight12.bin", 0x04000, 0x4000, CRC(1b615eae) SHA1(edfdb4311c5cc314806c8f017f190f7b94f8cd98) )
	/* 08000-0ffff empty */

	ROM_REGION( 0x8000, REGION_SOUND1, 0 )	/* ADPCM samples */
	ROM_LOAD( "gfight6.bin",  0x0000, 0x4000, CRC(d05d463d) SHA1(30f2bce0ad75c4a7d8344cff16bce27f5e3a3f5d) )
ROM_END

ROM_START( teedoff )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "to-1.bin",     0x0000, 0x4000, CRC(cc2aebc5) SHA1(358e77e53b35dd89fcfdb3b2484b8c4fbc34c1be) )
	ROM_LOAD( "to-2.bin",     0x4000, 0x4000, CRC(f7c9f138) SHA1(2fe56059ef67387b5938bb4751aa2f74a58b04fb) )
	ROM_LOAD( "to-3.bin",     0x8000, 0x4000, CRC(a0f0a6da) SHA1(72390c8dc5519d90e39a660e6ec18861fdbadcc8) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for code */
	ROM_LOAD( "to-4.bin",     0x0000, 0x8000, CRC(e922cbd2) SHA1(922c030be70150efb760fa81bda0bc54f2ec681a) )

	ROM_REGION( 0x10000, REGION_CPU3, 0 )	/* 64k for code */
	ROM_LOAD( "to-6.bin",     0x0000, 0x4000, CRC(d8dfe1c8) SHA1(d00a71ad89b530339990780334588f5738c60f25) )

	ROM_REGION( 0x04000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "to-12.bin",    0x00000, 0x4000, CRC(4f44622c) SHA1(161c3646a3ec2274bffc957240d47d55a35a8416) )	/* fg tiles */

	ROM_REGION( 0x10000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "to-8.bin",     0x00000, 0x8000, CRC(363bd1ba) SHA1(c5b7d56b0595712b18351403a9e3325a03de1676) )	/* sprites */
	ROM_LOAD( "to-7.bin",     0x08000, 0x8000, CRC(6583fa5b) SHA1(1041181887350d860c517c0a031ab064a20f5cee) )

	ROM_REGION( 0x10000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "to-11.bin",    0x00000, 0x8000, CRC(1ec00cb5) SHA1(0e61eed3d6fc44ff89d8b9e4f558f0989eb8094f) )	/* bg tiles */
	ROM_LOAD( "to-9.bin",     0x08000, 0x8000, CRC(a14347f0) SHA1(00a34ed56ec32336bb524424fcb007d8160163ec) )

	ROM_REGION( 0x8000, REGION_SOUND1, 0 )	/* ADPCM samples */
	ROM_LOAD( "to-5.bin",     0x0000, 0x8000, CRC(e5e4246b) SHA1(b2fe2e68fa86163ebe1ef00ecce73fb62cef6b19) )
ROM_END



GAME( 1985, tehkanwc, 0, tehkanwc, tehkanwc, 0,        ROT0,  "Tehkan", "Tehkan World Cup" )
GAME( 1985, gridiron, 0, gridiron, gridiron, 0,        ROT0,  "Tehkan", "Gridiron Fight" )
GAME( 1986, teedoff,  0, teedoff,  teedoff,  teedoff,  ROT90, "Tecmo", "Tee'd Off (Japan)" )
