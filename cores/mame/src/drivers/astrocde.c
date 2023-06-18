/****************************************************************************

Bally Astrocade style games

driver by Nicola Salmoria, Mike Coates, Frank Palazzolo

TODO:
- support stereo sound in Gorf (and maybe others, Wow has 3 speakers)

Notes:
- In seawolf2, service mode dip switch turns on memory test. Reset with 2 pressed
  to get to an input check screen, reset with 1+2 pressed to get to a convergence
  test screen.


memory map (preliminary)

0000-3fff ROM but also "magic" RAM (which processes data and copies it to the video RAM)
4000-7fff SCREEN RAM (bitmap)
8000-cfff ROM
d000-d3ff STATIC RAM

I/O ports:
IN:
08        intercept register (collision detector)
	      bit 0: intercept in pixel 3 in an OR or XOR write since last reset
	      bit 1: intercept in pixel 2 in an OR or XOR write since last reset
	      bit 2: intercept in pixel 1 in an OR or XOR write since last reset
	      bit 3: intercept in pixel 0 in an OR or XOR write since last reset
	      bit 4: intercept in pixel 3 in last OR or XOR write
	      bit 5: intercept in pixel 2 in last OR or XOR write
	      bit 6: intercept in pixel 1 in last OR or XOR write
	      bit 7: intercept in pixel 0 in last OR or XOR write
10        IN0
11        IN1
12        IN2
13        DSW
14		  Video Retrace
15        ?
17        Speech Synthesizer (Output)

OUT:
00-07     palette (00-03 = right part of screen; 04-07 left part)
08        select video mode (0 = low res 160x102, 1 = high res 320x204)
09        --xxxxxx position where to switch from the "left" to the "right" palette (/2).
          xx------ background color (portion of screen after vblank)
0a        screen height
0b        color block transfer
0c        magic RAM control
	      bit 7: ?
	      bit 6: flip
	      bit 5: draw in XOR mode
	      bit 4: draw in OR mode
	      bit 3: "expand" mode (convert 1bpp data to 2bpp)
	      bit 2: "rotate" mode (rotate 90 degrees - NOT EMULATED)
	      bit 1:\ shift amount to be applied before copying
	      bit 0:/
0d        set interrupt vector
10-18     sound
19        magic RAM expand mode color
78-7e     pattern board (see vidhrdw.c for details)

****************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

extern unsigned char *wow_videoram;

extern const char *wow_sample_names[];
extern const char *gorf_sample_names[];

PALETTE_INIT( astrocde );
READ_HANDLER( wow_intercept_r );
WRITE_HANDLER( wow_videoram_w );
WRITE_HANDLER( astrocde_magic_expand_color_w );
WRITE_HANDLER( astrocde_magic_control_w );
WRITE_HANDLER( wow_magicram_w );
WRITE_HANDLER( astrocde_pattern_board_w );
VIDEO_UPDATE( astrocde );
READ_HANDLER( wow_video_retrace_r );

WRITE_HANDLER( astrocde_interrupt_vector_w );
WRITE_HANDLER( astrocde_interrupt_enable_w );
WRITE_HANDLER( astrocde_interrupt_w );
INTERRUPT_GEN( wow_interrupt );

READ_HANDLER( seawolf2_controller1_r );
READ_HANDLER( seawolf2_controller2_r );
VIDEO_UPDATE( seawolf2 );

INTERRUPT_GEN( gorf_interrupt );
READ_HANDLER( gorf_timer_r );
READ_HANDLER( gorf_io_r );

VIDEO_START( astrocde );
VIDEO_START( astrocde_stars );

int  wow_sh_start(const struct MachineSound *msound);
void wow_sh_update(void);

READ_HANDLER( wow_speech_r );
READ_HANDLER( wow_port_2_r );
READ_HANDLER( wow_io_r );

int  gorf_sh_start(const struct MachineSound *msound);
void gorf_sh_update(void);
READ_HANDLER( gorf_speech_r );
READ_HANDLER( gorf_port_2_r );
WRITE_HANDLER( gorf_sound_control_a_w );

WRITE_HANDLER( astrocde_mode_w );
WRITE_HANDLER( astrocde_vertical_blank_w );
WRITE_HANDLER( astrocde_colour_register_w );
WRITE_HANDLER( astrocde_colour_split_w );
WRITE_HANDLER( astrocde_colour_block_w );

WRITE_HANDLER( ebases_trackball_select_w );
READ_HANDLER( ebases_trackball_r );

static UINT8 port_1_last;
static UINT8 port_2_last;

static MACHINE_INIT( seawolf2 )
{
	port_1_last = port_2_last = 0xff;
}

static WRITE_HANDLER( seawolf2_sound_1_w )  /* Port 40 */
{
	UINT8 rising_bits = data & ~port_1_last;
	port_1_last = data;

	if (rising_bits & 0x01) sample_start(1, 1, 0);  /* Left Torpedo */
	if (rising_bits & 0x02) sample_start(0, 0, 0);  /* Left Ship Hit */
	if (rising_bits & 0x04) sample_start(4, 4, 0);  /* Left Mine Hit */
	if (rising_bits & 0x08) sample_start(6, 1, 0);  /* Right Torpedo */
	if (rising_bits & 0x10) sample_start(5, 0, 0);  /* Right Ship Hit */
	if (rising_bits & 0x20) sample_start(9, 4, 0);  /* Right Mine Hit */
}


static WRITE_HANDLER( seawolf2_sound_2_w )  /* Port 41 */
{
	UINT8 rising_bits = data & ~port_2_last;
	port_2_last = data;

	sample_set_volume(0, (data & 0x80) ? 100 : 0);
	sample_set_volume(1, (data & 0x80) ? 100 : 0);
	sample_set_volume(3, (data & 0x80) ? 100 : 0);
	sample_set_volume(4, (data & 0x80) ? 100 : 0);
	sample_set_volume(5, (data & 0x80) ? 100 : 0);
	sample_set_volume(6, (data & 0x80) ? 100 : 0);
	sample_set_volume(8, (data & 0x80) ? 100 : 0);
	sample_set_volume(9, (data & 0x80) ? 100 : 0);

	/* dive panning controlled by low 3 bits */
	sample_set_volume(2, (float)(~data & 0x07) / 70);
	sample_set_volume(7, (float)(data & 0x07) / 70);

	if (rising_bits & 0x08)
	{
		sample_start(2, 2, 0);
		sample_start(7, 2, 0);
	}
	if (rising_bits & 0x10) sample_start(8, 3, 0);  /* Right Sonar */
	if (rising_bits & 0x20) sample_start(3, 3, 0);  /* Left Sonar */

	coin_counter_w(0, data & 0x40);    /* Coin Counter */
}

static WRITE_HANDLER( seawolf2_lamps_w )
{
	/* 0x42 = player 2 (left), 0x43 = player 1 (right) */
	/* --x----- explosion */
	/* ---x---- RELOAD (active low) */
	/* ----x--- torpedo 1 available */
	/* -----x-- torpedo 2 available */
	/* ------x- torpedo 3 available */
	/* -------x torpedo 4 available */

	/* I'm only supporting the "RELOAD" lamp since we don't have enough leds ;-) */
	set_led_status(offset^1,data & 0x10);
}


static MEMORY_READ_START( seawolf2_readmem )
	{ 0x0000, 0x1fff, MRA_ROM },
	{ 0x4000, 0x7fff, MRA_RAM },
	{ 0xc000, 0xc3ff, MRA_RAM },
MEMORY_END

static MEMORY_WRITE_START( seawolf2_writemem )
	{ 0x0000, 0x3fff, wow_magicram_w },
	{ 0x4000, 0x7fff, wow_videoram_w, &wow_videoram, &videoram_size },
	{ 0xc000, 0xc3ff, MWA_RAM },
MEMORY_END

static MEMORY_READ_START( readmem )
	{ 0x0000, 0x3fff, MRA_ROM },
	{ 0x4000, 0x7fff, MRA_RAM },
	{ 0x8000, 0xcfff, MRA_ROM },
	{ 0xd000, 0xdfff, MRA_RAM },
MEMORY_END

static MEMORY_WRITE_START( writemem )
	{ 0x0000, 0x3fff, wow_magicram_w },
	{ 0x4000, 0x7fff, wow_videoram_w, &wow_videoram, &videoram_size },	/* ASG */
	{ 0x8000, 0xcfff, MWA_ROM },
	{ 0xd000, 0xdfff, MWA_RAM },
MEMORY_END

static MEMORY_READ_START( robby_readmem )
	{ 0x0000, 0x3fff, MRA_ROM },
	{ 0x4000, 0x7fff, MRA_RAM },
	{ 0x8000, 0xdfff, MRA_ROM },
	{ 0xe000, 0xffff, MRA_RAM },
MEMORY_END

static MEMORY_WRITE_START( robby_writemem )
	{ 0x0000, 0x3fff, wow_magicram_w },
	{ 0x4000, 0x7fff, wow_videoram_w, &wow_videoram, &videoram_size },
	{ 0x8000, 0xdfff, MWA_ROM },
	{ 0xe000, 0xffff, MWA_RAM },
MEMORY_END

static MEMORY_READ_START( profpac_readmem )
	{ 0x0000, 0x3fff, MRA_ROM },
	{ 0x8000, 0xdfff, MRA_ROM },
	{ 0xe000, 0xffff, MRA_RAM },
MEMORY_END

static MEMORY_WRITE_START( profpac_writemem )
	{ 0x0000, 0x3fff, wow_magicram_w },
	{ 0x4000, 0x7fff, wow_videoram_w, &wow_videoram, &videoram_size },
	{ 0x8000, 0xdfff, MWA_ROM },
	{ 0xe000, 0xffff, MWA_RAM },
MEMORY_END

static PORT_READ_START( readport )
	{ 0x08, 0x08, wow_intercept_r },
	{ 0x0e, 0x0e, wow_video_retrace_r },
	{ 0x10, 0x10, input_port_0_r },
	{ 0x11, 0x11, input_port_1_r },
  	{ 0x12, 0x12, input_port_2_r },
	{ 0x13, 0x13, input_port_3_r },
PORT_END

static PORT_WRITE_START( seawolf2_writeport )
	{ 0x00, 0x07, astrocde_colour_register_w },
	{ 0x08, 0x08, astrocde_mode_w },
	{ 0x09, 0x09, astrocde_colour_split_w },
	{ 0x0a, 0x0a, astrocde_vertical_blank_w },
	{ 0x0b, 0x0b, astrocde_colour_block_w },
	{ 0x0c, 0x0c, astrocde_magic_control_w },
	{ 0x0d, 0x0d, astrocde_interrupt_vector_w },
	{ 0x0e, 0x0e, astrocde_interrupt_enable_w },
	{ 0x0f, 0x0f, astrocde_interrupt_w },
	{ 0x19, 0x19, astrocde_magic_expand_color_w },
	{ 0x40, 0x40, seawolf2_sound_1_w }, /* analog sound */
	{ 0x41, 0x41, seawolf2_sound_2_w }, /* analog sound */
	{ 0x42, 0x43, seawolf2_lamps_w },	/* cabinet lamps */
PORT_END

static PORT_WRITE_START( writeport )
	{ 0x00, 0x07, astrocde_colour_register_w },
	{ 0x08, 0x08, astrocde_mode_w },
	{ 0x09, 0x09, astrocde_colour_split_w },
	{ 0x0a, 0x0a, astrocde_vertical_blank_w },
	{ 0x0b, 0x0b, astrocde_colour_block_w },
	{ 0x0c, 0x0c, astrocde_magic_control_w },
	{ 0x0d, 0x0d, astrocde_interrupt_vector_w },
	{ 0x0e, 0x0e, astrocde_interrupt_enable_w },
	{ 0x0f, 0x0f, astrocde_interrupt_w },
	{ 0x10, 0x18, astrocade_sound1_w },
	{ 0x19, 0x19, astrocde_magic_expand_color_w },
	{ 0x50, 0x58, astrocade_sound2_w },
	{ 0x5b, 0x5b, MWA_NOP }, /* speech board ? Wow always sets this to a5*/
	{ 0x78, 0x7e, astrocde_pattern_board_w },
/*	{ 0xf8, 0xff, MWA_NOP }, */ /* Gorf uses these */
PORT_END



INPUT_PORTS_START( seawolf2 )
	PORT_START /* IN0 */
	PORT_ANALOG( 0x3f, 0x20, IPT_PADDLE | IPF_REVERSE | IPF_PLAYER1, 20, 5, 0, 0x3f)
	PORT_DIPNAME( 0x40, 0x00, "Language 1" )
	PORT_DIPSETTING(    0x00, "Language 2" )
	PORT_DIPSETTING(    0x40, "French" )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_PLAYER1 )

	PORT_START /* IN1 */
	PORT_ANALOG( 0x3f, 0x20, IPT_PADDLE | IPF_REVERSE | IPF_PLAYER2, 20, 5, 0, 0x3f)
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_PLAYER2 )

	PORT_START /* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_DIPNAME( 0x08, 0x00, "Language 2" )
	PORT_DIPSETTING(    0x00, "English" )
	PORT_DIPSETTING(    0x08, "German" )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START /* Dip Switch */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 1C_1C ) )
	PORT_DIPNAME( 0x06, 0x00, "Play Time" )
	PORT_DIPSETTING(    0x06, "40" )
	PORT_DIPSETTING(    0x04, "50" )
	PORT_DIPSETTING(    0x02, "60" )
	PORT_DIPSETTING(    0x00, "70" )
	PORT_DIPNAME( 0x08, 0x08, "2 Players Game" )
	PORT_DIPSETTING(    0x00, "1 Credit" )
	PORT_DIPSETTING(    0x08, "2 Credits" )
	PORT_DIPNAME( 0x30, 0x00, "Extended Play" )
	PORT_DIPSETTING(    0x10, "5000" )
	PORT_DIPSETTING(    0x20, "6000" )
	PORT_DIPSETTING(    0x30, "7000" )
	PORT_DIPSETTING(    0x00, "None" )
	PORT_DIPNAME( 0x40, 0x40, "Monitor" )
	PORT_DIPSETTING(    0x40, "Color" )
	PORT_DIPSETTING(    0x00, "B/W" )
	PORT_SERVICE( 0x80, IP_ACTIVE_LOW )
INPUT_PORTS_END

INPUT_PORTS_START( spacezap )
	PORT_START /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_TILT )
	PORT_SERVICE( 0x08, IP_ACTIVE_LOW )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START2 )	/* starts a 1 player game if 1 credit*/
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0xe0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START /* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_4WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_4WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_4WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Cocktail ) )
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START /* Dip Switch */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 1C_1C ) )
	PORT_DIPNAME( 0x06, 0x06, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_5C ) )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x10, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x20, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x80, DEF_STR( On ) )
INPUT_PORTS_END

INPUT_PORTS_START( ebases )
	PORT_START /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x0c, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_TILT )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x00, "Monitor" )
	PORT_DIPSETTING(    0x00, "Color" )
	PORT_DIPSETTING(    0x10, "B/W" )
	PORT_DIPNAME( 0x20, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x20, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x80, DEF_STR( On ) )

	PORT_START /* Dip Switch */
	PORT_DIPNAME( 0x01, 0x00, "2 Players Game" )
	PORT_DIPSETTING(    0x00, "1 Credit" )
	PORT_DIPSETTING(    0x01, "2 Credits" )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x02, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x04, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x10, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x20, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x80, DEF_STR( On ) )

	PORT_START
	PORT_ANALOGX( 0xff, 0x00, IPT_TRACKBALL_X | IPF_PLAYER2 | IPF_CENTER, 50, 10, 0, 0, IP_KEY_NONE, IP_KEY_NONE, IP_JOY_NONE, IP_JOY_NONE )	\

	PORT_START
	PORT_ANALOGX( 0xff, 0x00, IPT_TRACKBALL_Y | IPF_PLAYER2 | IPF_CENTER, 50, 10, 0, 0, IP_KEY_NONE, IP_KEY_NONE, IP_JOY_NONE, IP_JOY_NONE )	\

	PORT_START
	PORT_ANALOGX( 0xff, 0x00, IPT_TRACKBALL_X | IPF_CENTER, 50, 10, 0, 0, IP_KEY_NONE, IP_KEY_NONE, IP_JOY_NONE, IP_JOY_NONE )	\

	PORT_START
	PORT_ANALOGX( 0xff, 0x00, IPT_TRACKBALL_Y | IPF_CENTER, 50, 10, 0, 0, IP_KEY_NONE, IP_KEY_NONE, IP_JOY_NONE, IP_JOY_NONE )	\
INPUT_PORTS_END

INPUT_PORTS_START( wow )
	PORT_START /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_SERVICE( 0x08, IP_ACTIVE_LOW )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_TILT )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START2 )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START /* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )	/* speech status */

	PORT_START /* Dip Switch */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 1C_1C ) )
	PORT_DIPNAME( 0x06, 0x06, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_5C ) )
	PORT_DIPNAME( 0x08, 0x08, "Language" )
	PORT_DIPSETTING(    0x08, "English" )
	PORT_DIPSETTING(    0x00, "Foreign (NEED ROM)" )
	PORT_DIPNAME( 0x10, 0x00, DEF_STR( Lives ) )
 	PORT_DIPSETTING(    0x10, "2 for 1 Credit / 5 for 2 Credits" )
 	PORT_DIPSETTING(    0x00, "3 for 1 Credit / 7 for 2 Credits" )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x20, "3rd Level" )
	PORT_DIPSETTING(    0x00, "4th Level" )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Free_Play ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, "On only when controls are touched" )
	PORT_DIPSETTING(    0x80, "Always On"  )
INPUT_PORTS_END

INPUT_PORTS_START( gorf )
	PORT_START /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_SERVICE( 0x04, IP_ACTIVE_LOW )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_TILT )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START2 )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Cocktail ) )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START /* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )	/* speech status */

	PORT_START /* Dip Switch */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 1C_1C ) )
	PORT_DIPNAME( 0x06, 0x06, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_5C ) )
	PORT_DIPNAME( 0x08, 0x08, "Language" )
	PORT_DIPSETTING(    0x08, "English" )
	PORT_DIPSETTING(    0x00, "Foreign (NEED ROM)" )
	PORT_DIPNAME( 0x10, 0x00, "Lives per Credit" )
	PORT_DIPSETTING(    0x10, "2" )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPNAME( 0x20, 0x00, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x00, "Mission 5" )
	PORT_DIPSETTING(    0x20, "None" )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Free_Play ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x80, DEF_STR( On ) )
INPUT_PORTS_END

INPUT_PORTS_START( robby )
	PORT_START /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_SERVICE( 0x08, IP_ACTIVE_LOW )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_TILT )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START /* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_4WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_4WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_4WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START /* Dip Switch */
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x01, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x02, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Free_Play ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x10, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x10, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x20, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x80, DEF_STR( On ) )
INPUT_PORTS_END

static const char *seawolf_sample_names[] =
{
	"*seawolf",
	"shiphit.wav",
	"torpedo.wav",
	"dive.wav",
	"sonar.wav",
	"minehit.wav",
	0       /* end of array */
};

struct Samplesinterface seawolf2_samples_interface =
{
	10,	/* 5*2 channels */
	25,	/* volume */
	seawolf_sample_names
};

static struct Samplesinterface wow_samples_interface =
{
	8,	/* 8 channels */
	25,	/* volume */
	wow_sample_names
};

static struct Samplesinterface gorf_samples_interface =
{
	8,	/* 8 channels */
	25,	/* volume */
	gorf_sample_names
};

static struct astrocade_interface astrocade_2chip_interface =
{
	2,			/* Number of chips */
	1789773,	/* Clock speed */
	{255,255}			/* Volume */
};

static struct astrocade_interface astrocade_1chip_interface =
{
	1,			/* Number of chips */
	1789773,	/* Clock speed */
	{255}			/* Volume */
};

static struct CustomSound_interface gorf_custom_interface =
{
	gorf_sh_start,
	0,
	gorf_sh_update
};

static struct CustomSound_interface wow_custom_interface =
{
	wow_sh_start,
	0,
	wow_sh_update
};


static MACHINE_DRIVER_START( seawolf2 )

	/* basic machine hardware */
	MDRV_CPU_ADD(Z80, 1789773)	/* 1.789 MHz */
	MDRV_CPU_MEMORY(seawolf2_readmem,seawolf2_writemem)
	MDRV_CPU_PORTS(readport,seawolf2_writeport)
	MDRV_CPU_VBLANK_INT(wow_interrupt,256)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)
	MDRV_MACHINE_INIT(seawolf2)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(320, 204)
	MDRV_VISIBLE_AREA(0, 320-1, 0, 204-1)
	MDRV_PALETTE_LENGTH(256)

	MDRV_PALETTE_INIT(astrocde)
	MDRV_VIDEO_START(astrocde)
	MDRV_VIDEO_UPDATE(seawolf2)

	/* sound hardware */
	MDRV_SOUND_ADD(SAMPLES, seawolf2_samples_interface)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( spacezap )

	/* basic machine hardware */
	MDRV_CPU_ADD(Z80, 1789773)	/* 1.789 MHz */
	MDRV_CPU_MEMORY(readmem,writemem)
	MDRV_CPU_PORTS(readport,writeport)
	MDRV_CPU_VBLANK_INT(wow_interrupt,256)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(320, 204)
	MDRV_VISIBLE_AREA(0, 320-1, 0, 204-1)
	MDRV_PALETTE_LENGTH(256)

	MDRV_PALETTE_INIT(astrocde)
	MDRV_VIDEO_START(astrocde)
	MDRV_VIDEO_UPDATE(astrocde)

	/* sound hardware */
	MDRV_SOUND_ADD(ASTROCADE, astrocade_2chip_interface)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( ebases )

	/* basic machine hardware */
	MDRV_CPU_ADD(Z80, 1789773)	/* 1.789 MHz */
	MDRV_CPU_MEMORY(readmem,writemem)
	MDRV_CPU_PORTS(readport,writeport)
	MDRV_CPU_VBLANK_INT(wow_interrupt,256)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(320, 204)
	MDRV_VISIBLE_AREA(0, 320-1, 0, 204-1)
	MDRV_PALETTE_LENGTH(256)

	MDRV_PALETTE_INIT(astrocde)
	MDRV_VIDEO_START(astrocde)
	MDRV_VIDEO_UPDATE(astrocde)

	/* sound hardware */
	MDRV_SOUND_ADD(ASTROCADE, astrocade_1chip_interface)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( wow )

	/* basic machine hardware */
	MDRV_CPU_ADD(Z80, 1789773)	/* 1.789 MHz */
	MDRV_CPU_MEMORY(readmem,writemem)
	MDRV_CPU_PORTS(readport,writeport)
	MDRV_CPU_VBLANK_INT(wow_interrupt,256)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(320, 204)
	MDRV_VISIBLE_AREA(0, 320-1, 0, 204-1)
	MDRV_PALETTE_LENGTH(256)
	MDRV_PALETTE_INIT(astrocde)

	MDRV_VIDEO_START(astrocde_stars)
	MDRV_VIDEO_UPDATE(astrocde)

	/* sound hardware */
	MDRV_SOUND_ADD(ASTROCADE, astrocade_2chip_interface)
	MDRV_SOUND_ADD(SAMPLES, wow_samples_interface)
	MDRV_SOUND_ADD(CUSTOM, wow_custom_interface)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( gorf )

	/* basic machine hardware */
	MDRV_CPU_ADD(Z80, 1789773)	/* 1.789 MHz */
	MDRV_CPU_MEMORY(readmem,writemem)
	MDRV_CPU_PORTS(readport,writeport)
	MDRV_CPU_VBLANK_INT(gorf_interrupt,256)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	/* video hardware */
	/* it may look like the right hand side of the screen needs clipping, but */
	/* this isn't the case: cocktail mode would be clipped on the wrong side */

	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(320, 204)
	MDRV_VISIBLE_AREA(0, 320-1, 0, 204-1)
	MDRV_PALETTE_LENGTH(256)

	MDRV_PALETTE_INIT(astrocde)
	MDRV_VIDEO_START(astrocde_stars)
	MDRV_VIDEO_UPDATE(astrocde)

	/* sound hardware */
	MDRV_SOUND_ADD(ASTROCADE, astrocade_2chip_interface)
	MDRV_SOUND_ADD(SAMPLES, gorf_samples_interface)
	MDRV_SOUND_ADD(CUSTOM, gorf_custom_interface)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( robby )

	/* basic machine hardware */
	MDRV_CPU_ADD(Z80, 1789773)	/* 1.789 MHz */
	MDRV_CPU_MEMORY(robby_readmem,robby_writemem)
	MDRV_CPU_PORTS(readport,writeport)
	MDRV_CPU_VBLANK_INT(wow_interrupt,256)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(320, 204)
	MDRV_VISIBLE_AREA(0, 320-1, 0, 204-1)
	MDRV_PALETTE_LENGTH(256)
	MDRV_PALETTE_INIT(astrocde)

	MDRV_VIDEO_START(astrocde)
	MDRV_VIDEO_UPDATE(astrocde)

	/* sound hardware */
	MDRV_SOUND_ADD(ASTROCADE, astrocade_2chip_interface)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( profpac )

	/* basic machine hardware */
	MDRV_CPU_ADD(Z80, 1789773)	/* 1.789 MHz */
	MDRV_CPU_MEMORY(profpac_readmem,profpac_writemem)
	MDRV_CPU_PORTS(readport,writeport)
	MDRV_CPU_VBLANK_INT(wow_interrupt,256)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(320, 204)
	MDRV_VISIBLE_AREA(0, 320-1, 0, 204-1)
	MDRV_PALETTE_LENGTH(256)

	MDRV_PALETTE_INIT(astrocde)
	MDRV_VIDEO_START(astrocde)
	MDRV_VIDEO_UPDATE(astrocde)

	/* sound hardware */
	MDRV_SOUND_ADD(ASTROCADE, astrocade_2chip_interface)
MACHINE_DRIVER_END



ROM_START( seawolf2 )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "sw2x1.bin",    0x0000, 0x0800, CRC(ad0103f6) SHA1(c6e411444a824ce54b0eee10f7dc15e4229ec070) )
	ROM_LOAD( "sw2x2.bin",    0x0800, 0x0800, CRC(e0430f0a) SHA1(63d8c6b77e0aa536b4f5bb774bc9285f736d4265) )
	ROM_LOAD( "sw2x3.bin",    0x1000, 0x0800, CRC(05ad1619) SHA1(c9dbeaa4540dc95f98970f501a420b18b9898c91) )
	ROM_LOAD( "sw2x4.bin",    0x1800, 0x0800, CRC(1a1a14a2) SHA1(57d0ddea9f8bf082f50d0468a726fd91aaabf4e4) )
ROM_END

ROM_START( spacezap )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "0662.01",      0x0000, 0x1000, CRC(a92de312) SHA1(784ac67c75c7c101f97ebfd39b2b3f7bf7fa470a) )
	ROM_LOAD( "0663.xx",      0x1000, 0x1000, CRC(4836ebf1) SHA1(ad0e8c34a209c827c1336f0250cc61fee667fb03) )
	ROM_LOAD( "0664.xx",      0x2000, 0x1000, CRC(d8193a80) SHA1(72151e773562da62acd2c1d9638711711cbc13a3) )
	ROM_LOAD( "0665.xx",      0x3000, 0x1000, CRC(3784228d) SHA1(5aabd720a106158a892368c4920d9cd0f5235e34) )
ROM_END

ROM_START( ebases )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "m761a",        0x0000, 0x1000, CRC(34422147) SHA1(6483ca1359b675b0dd739605db2a1dbd4b7fb8cb) )
	ROM_LOAD( "m761b",        0x1000, 0x1000, CRC(4f28dfd6) SHA1(52e571e671fa61b0f9ab397a5947094c24f6c388) )
	ROM_LOAD( "m761c",        0x2000, 0x1000, CRC(bff6c97e) SHA1(e41fb9db919039c8a48b4caebf80821a066d7ccf) )
	ROM_LOAD( "m761d",        0x3000, 0x1000, CRC(5173781a) SHA1(e60c3f4b075f8b811ff6a8637c4aa0b089847a82) )
ROM_END

ROM_START( wow )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "wow.x1",       0x0000, 0x1000, CRC(c1295786) SHA1(1e4f30cc15537aed6603b4e664e6e60f4bccb5c5) )
	ROM_LOAD( "wow.x2",       0x1000, 0x1000, CRC(9be93215) SHA1(0bc8ee6d8391104eb217b612f32856b105946682) )
	ROM_LOAD( "wow.x3",       0x2000, 0x1000, CRC(75e5a22e) SHA1(50a8ca11909ce49412c47de4da69e39a083ce5af) )
	ROM_LOAD( "wow.x4",       0x3000, 0x1000, CRC(ef28eb84) SHA1(d6318b3649fccafc2d0a05e5530e88819d299356) )
	ROM_LOAD( "wow.x5",       0x8000, 0x1000, CRC(16912c2b) SHA1(faf9c96d99bc111c5f1618f6863f22fd9269027b) )
	ROM_LOAD( "wow.x6",       0x9000, 0x1000, CRC(35797f82) SHA1(376bba29e88c16d95438fa996913b76581df0937) )
	ROM_LOAD( "wow.x7",       0xa000, 0x1000, CRC(ce404305) SHA1(a52c6c7b77842f25c79515460be6b7ed959b5edb) )
/*	ROM_LOAD( "wow.x8",       0xc000, CRC(00001000) , ? )	here would go the foreign language ROM */
ROM_END

ROM_START( gorf )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "gorf-a.bin",   0x0000, 0x1000, CRC(5b348321) SHA1(76e2e3ad1a66755f1a369167fdb157690fd44a52) )
	ROM_LOAD( "gorf-b.bin",   0x1000, 0x1000, CRC(62d6de77) SHA1(2601faf12d0ab4972c5535ffd722b03ecd8c097c) )
	ROM_LOAD( "gorf-c.bin",   0x2000, 0x1000, CRC(1d3bc9c9) SHA1(0b363a71d7585a4828e08668ebb2999c55e02721) )
	ROM_LOAD( "gorf-d.bin",   0x3000, 0x1000, CRC(70046e56) SHA1(392214cc6ed4155bfe022d36f0f86c2594a5ab57) )
	ROM_LOAD( "gorf-e.bin",   0x8000, 0x1000, CRC(2d456eb5) SHA1(720fb8b48e20c1fc281d8804259016c3c5364a07) )
	ROM_LOAD( "gorf-f.bin",   0x9000, 0x1000, CRC(f7e4e155) SHA1(9c9d6d3bfee6556dc7a01de81d6148dd02f04fc9) )
	ROM_LOAD( "gorf-g.bin",   0xa000, 0x1000, CRC(4e2bd9b9) SHA1(9edccceea5af015275582553ed238c40c73d8f4f) )
	ROM_LOAD( "gorf-h.bin",   0xb000, 0x1000, CRC(fe7b863d) SHA1(5aa8d824814ee1c30eaf0044da78d3aa8220dcaa) )
ROM_END

ROM_START( gorfpgm1 )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "873a",         0x0000, 0x1000, CRC(97cb4a6a) SHA1(efdae9a437c665fb861665a38c6cb13fd848ad91) )
	ROM_LOAD( "873b",         0x1000, 0x1000, CRC(257236f8) SHA1(d1e8555fe5e6705ef88535bcd6071d1072b01386) )
	ROM_LOAD( "873c",         0x2000, 0x1000, CRC(16b0638b) SHA1(65e1e2e4df80140976915e0982ce3219b14beece) )
	ROM_LOAD( "873d",         0x3000, 0x1000, CRC(b5e821dc) SHA1(152840e353d567cbf5a86206dde70e5b64b27236) )
	ROM_LOAD( "873e",         0x8000, 0x1000, CRC(8e82804b) SHA1(24250edb30efa63c80514629c86c9372b7ca3020) )
	ROM_LOAD( "873f",         0x9000, 0x1000, CRC(715fb4d9) SHA1(c9f33162093e6ed7e3cb6bb716419e5bc43c0381) )
	ROM_LOAD( "873g",         0xa000, 0x1000, CRC(8a066456) SHA1(f64bcdadbc62566b55573039b03baf5358e24a36) )
	ROM_LOAD( "873h",         0xb000, 0x1000, CRC(56d40c7c) SHA1(c7c9a618d9438a76121972ac029ad7036bcf8c6f) )
ROM_END

ROM_START( robby )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "rotox1.bin",   0x0000, 0x1000, CRC(a431b85a) SHA1(3478da56addba1cdd98cbef7a15b17fca9aed2cd) )
	ROM_LOAD( "rotox2.bin",   0x1000, 0x1000, CRC(33cdda83) SHA1(ccbc741a2fc0b7385ca42afe5b377432249b44cb) )
	ROM_LOAD( "rotox3.bin",   0x2000, 0x1000, CRC(dbf97491) SHA1(11574baf04af02b38ae147be8409de7c34e87611) )
	ROM_LOAD( "rotox4.bin",   0x3000, 0x1000, CRC(a3b90ac8) SHA1(8c585d26011c9ea047895a0388835ff2bb80e1ff) )
	ROM_LOAD( "rotox5.bin",   0x8000, 0x1000, CRC(46ae8a94) SHA1(218edcc5257c9cc58c5e667fff64767b313daaab) )
	ROM_LOAD( "rotox6.bin",   0x9000, 0x1000, CRC(7916b730) SHA1(c5166625a404da4a93a1a7ae21d01fdb6e78680e) )
	ROM_LOAD( "rotox7.bin",   0xa000, 0x1000, CRC(276dc4a5) SHA1(d740b30c28f6a94ee2348291e80d57af5c2e2d99) )
	ROM_LOAD( "rotox8.bin",   0xb000, 0x1000, CRC(1ef13457) SHA1(4dc1ee9ce2a28c4ba75e630fbfe4659cd68d3a66) )
  	ROM_LOAD( "rotox9.bin",   0xc000, 0x1000, CRC(370352bf) SHA1(72cd35b4306b46de3d2a3e4e46fa4917ed9d18cb) )
	ROM_LOAD( "rotox10.bin",  0xd000, 0x1000, CRC(e762cbda) SHA1(48c274a859963097a90f80c48366250301eddb5f) )
ROM_END

ROM_START( profpac )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "pps1",         0x0000, 0x2000, CRC(a244a62d) SHA1(f7a9606ce6d66c3e6d210cc25572904aeab2b6c8) )
	ROM_LOAD( "pps2",         0x2000, 0x2000, CRC(8a9a6653) SHA1(b730b24088dcfddbe954670ff9212b7383c923f6) )
	ROM_LOAD( "pps7",         0x8000, 0x2000, CRC(f9c26aba) SHA1(201b930cca9669114ffc97978cade69587e34a0f) )
	ROM_LOAD( "pps8",         0xa000, 0x2000, CRC(4d201e41) SHA1(786b30cd7a7db55bdde05909d7a1a7f122b6e546) )
	ROM_LOAD( "pps9",         0xc000, 0x2000, CRC(17a0b418) SHA1(8b7ed84090dbc5181deef6f55ec755c05d4c0d5e) )

	ROM_REGION( 0x04000, REGION_USER1, 0 )
	ROM_LOAD( "pps3",         0x0000, 0x2000, CRC(15717fd8) SHA1(ffbb156f417d20478117b39de28a15680993b528) )
	ROM_LOAD( "pps4",         0x0000, 0x2000, CRC(36540598) SHA1(33c797c690801afded45091d822347e1ecc72b54) )
	ROM_LOAD( "pps5",         0x0000, 0x2000, CRC(8dc89a59) SHA1(fb4d3ba40697425d69ee19bfdcf00aea1df5fa80) )
	ROM_LOAD( "pps6",         0x0000, 0x2000, CRC(5a2186c3) SHA1(f706cef6518b7d839377aa8a7c75fdeed4985c57) )
	ROM_LOAD( "ppq1",         0x0000, 0x4000, CRC(dddc2ccc) SHA1(d81caaa639f63d971a0d3199b9da6359211edf3d) )
	ROM_LOAD( "ppq2",         0x0000, 0x4000, CRC(33bbcabe) SHA1(f9455868c70f479ede0e0621f21f69da165d9b7a) )
	ROM_LOAD( "ppq3",         0x0000, 0x4000, CRC(3534d895) SHA1(24fb14c6b31b7f27e0737605cfbf963d29dd3fc5) )
	ROM_LOAD( "ppq4",         0x0000, 0x4000, CRC(17e3581d) SHA1(92d2391e4c8aef46cc8e92b8cf9a8ec9a1b5ff68) )
	ROM_LOAD( "ppq5",         0x0000, 0x4000, CRC(80882a93) SHA1(d5d6afaadb022b109c14c3911eceb0769204df6c) )
	ROM_LOAD( "ppq6",         0x0000, 0x4000, CRC(e5ddaee5) SHA1(45b4925709da6790676319268398f6cfcf12794b) )
	ROM_LOAD( "ppq7",         0x0000, 0x4000, CRC(c029cd34) SHA1(f2f09fdb13920012a6a43958b640d7a06c0c8e69) )
	ROM_LOAD( "ppq8",         0x0000, 0x4000, CRC(fb3a1ac9) SHA1(e8fe02c85e90320680a14ad560204d5c235730ad) )
	ROM_LOAD( "ppq9",         0x0000, 0x4000, CRC(5e944488) SHA1(2f03f799c319309b5ebf9a5299891d1824398ba5) )
	ROM_LOAD( "ppq10",        0x0000, 0x4000, CRC(ed72a81f) SHA1(db991b93001d2da16b398ee8e9b01b8f0dfe5740) )
	ROM_LOAD( "ppq11",        0x0000, 0x4000, CRC(98295020) SHA1(7f68a8b89117b7ab8724869401a861fe7cff28d9) )
	ROM_LOAD( "ppq12",        0x0000, 0x4000, CRC(e01a8dbe) SHA1(c7052bf9ce9d2006dda5ddc07ad164d0119b86ea) )
	ROM_LOAD( "ppq13",        0x0000, 0x4000, CRC(87165d4f) SHA1(d47655300c8747698a46f30deb65fe762073e869) )
	ROM_LOAD( "ppq14",        0x0000, 0x4000, CRC(ecb861de) SHA1(73d28a79b76795d3016dd608f9ab3d255f40e477) )
ROM_END



static DRIVER_INIT( seawolf2 )
{
	install_port_read_handler(0, 0x10, 0x10, seawolf2_controller2_r);
	install_port_read_handler(0, 0x11, 0x11, seawolf2_controller1_r);
}
static DRIVER_INIT( ebases )
{
	install_port_read_handler (0, 0x13, 0x13, ebases_trackball_r);
	install_port_write_handler(0, 0x28, 0x28, ebases_trackball_select_w);
}
static DRIVER_INIT( wow )
{
	install_port_read_handler(0, 0x12, 0x12, wow_port_2_r);
	install_port_read_handler(0, 0x15, 0x15, wow_io_r);
	install_port_read_handler(0, 0x17, 0x17, wow_speech_r);
}
static DRIVER_INIT( gorf )
{
	install_mem_read_handler (0, 0xd0a5, 0xd0a5, gorf_timer_r);

	install_port_read_handler(0, 0x12, 0x12, gorf_port_2_r);
	install_port_read_handler(0, 0x15, 0x16, gorf_io_r);
	install_port_read_handler(0, 0x17, 0x17, gorf_speech_r);
}


GAME( 1978, seawolf2, 0,    seawolf2, seawolf2, seawolf2, ROT0,   "Midway", "Sea Wolf II" )
GAME( 1980, spacezap, 0,    spacezap, spacezap, 0,        ROT0,   "Midway", "Space Zap" )
GAME( 1980, ebases,   0,    ebases,   ebases,   ebases,   ROT0,   "Midway", "Extra Bases" )
GAME( 1980, wow,      0,    wow,      wow,      wow,      ROT0,   "Midway", "Wizard of Wor" )
GAME( 1981, gorf,     0,    gorf,     gorf,     gorf,     ROT270, "Midway", "Gorf" )
GAME( 1981, gorfpgm1, gorf, gorf,     gorf,     gorf,     ROT270, "Midway", "Gorf (Program 1)" )
GAME( 1981, robby,    0,    robby,    robby,    0,        ROT0,   "Bally Midway", "Robby Roto" )
GAMEX( 1983,profpac,  0,    profpac,  gorf,     0,        ROT0,   "Bally Midway", "Professor PacMan", GAME_NOT_WORKING )
