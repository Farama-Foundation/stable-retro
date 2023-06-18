/***************************************************************************

  Tao Taido             (c) 1993 Video System


	driver by David Haywood - Dip Switches and Inputs by Stephane Humbert
	based on other Video System drivers

Stephh's notes (based on the games M68000 code and some tests) :

0) all games

  - Don't trust the test mode ! It shows inputs for 4 players as well as
    3 buttons for each player, while the game is a 2 players game with only
    2 buttons (punch and kick) for each player.
    IMO, it's a leftover from a previous game.
    If you want to test all inputs, turn TAOTAIDO_SHOW_ALL_INPUTS to 1.

  - The "Buy-In" features allows a player to recover some energy and reset timer
    by pressing his "Start" button (provided he has at least one credit).

  - 'taotaido' allows to play a 2 players game with a single credit while
    this isn't possible with 'taotaida'.
    Now, telling which version is the newest one is another story ;)

  - 'taotaido' seems to show you how to do the special moves, 'taotaida' doesn't
    and they don't seem to work in the same way (unless this is a bug)

  - Coin buttons act differently depending on the "Coin Slot" Dip Switch :

      * "Coin Slot" Dip Switch set to "Same" :

          . COIN1 : adds coin(s)/credit(s) depending on "Coinage" Dip Switch
          . COIN2 : adds 1 credit
          . SERVICE1 : adds coin(s)/credit(s) depending on "Coinage" Dip Switch

      * "Coin Slot" Dip Switch set to "Individual" :

          . COIN1 : adds coin(s)/credit(s) for player 1 depending on "Coinage" Dip Switch
          . COIN2 : adds coin(s)/credit(s) for player 2 depending on "Coinage" Dip Switch
          . SERVICE1 : adds 1 credit for player 1

***************************************************************************/

/* Tao Taido
(c)1993 Video System

CPU:	68000-16
Sound:	Z80-B
		YM2610
OSC:	14.31818MHz
		20.0000MHz
		32.0000MHz
Chips:	VS9108
		VS920B
		VS9209 x2

****************************************************************************

zooming might be wrong

***************************************************************************/



#include "driver.h"

#define TAOTAIDO_SHOW_ALL_INPUTS	0

data16_t *taotaido_spriteram;
data16_t *taotaido_spriteram2;
data16_t *taotaido_scrollram;
data16_t *taotaido_bgram;

WRITE16_HANDLER( taotaido_sprite_character_bank_select_w );
WRITE16_HANDLER( taotaido_tileregs_w );
WRITE16_HANDLER( taotaido_bgvideoram_w );
VIDEO_START( taotaido );
VIDEO_UPDATE( taotaido );
VIDEO_EOF( taotaido );

int pending_command;

static READ16_HANDLER( pending_command_r )
{
	/* Only bit 0 is tested */
	return pending_command;
}

static WRITE16_HANDLER( sound_command_w )
{
	if (ACCESSING_LSB)
	{
		pending_command = 1;
		soundlatch_w(offset,data & 0xff);
		cpu_set_irq_line(1, IRQ_LINE_NMI, PULSE_LINE);
	}
}
static MEMORY_READ16_START( taotaido_readmem )
	{ 0x000000, 0x0fffff, MRA16_ROM },
	{ 0x800000, 0x803fff, MRA16_RAM },			/* bg ram?*/
	{ 0xa00000, 0xa01fff, MRA16_RAM },			/* sprite ram*/
	{ 0xc00000, 0xc0ffff, MRA16_RAM },			/* sprite tile look up*/
	{ 0xfe0000, 0xfeffff, MRA16_RAM },			/* main ram*/
	{ 0xffc000, 0xffcfff, MRA16_RAM },			/* palette ram*/
	{ 0xffe000, 0xffe3ff, MRA16_RAM },			/* rowscroll / rowselect / scroll ram*/

	{ 0xffff80, 0xffff81, input_port_0_word_r },	/* player 1 inputs*/
	{ 0xffff82, 0xffff83, input_port_1_word_r },	/* player 2 inputs*/
	{ 0xffff84, 0xffff85, input_port_2_word_r },	/* system inputs*/
	{ 0xffff86, 0xffff87, input_port_3_word_r },	/* DSWA*/
	{ 0xffff88, 0xffff89, input_port_4_word_r },	/* DSWB*/
	{ 0xffff8a, 0xffff8b, input_port_5_word_r },	/* DSWC*/
	{ 0xffff8c, 0xffff8d, MRA16_RAM },			/* unknown*/
	{ 0xffff8e, 0xffff8f, input_port_6_word_r },	/* jumpers*/
#if TAOTAIDO_SHOW_ALL_INPUTS
	{ 0xffffa0, 0xffffa1, input_port_7_word_r },	/* player 3 inputs (unused)*/
	{ 0xffffa2, 0xffffa3, input_port_8_word_r },	/* player 4 inputs (unused)*/
#endif

	{ 0xffffe0, 0xffffe1, pending_command_r },	/* guess - seems to be needed for all the sounds to work*/
MEMORY_END

static MEMORY_WRITE16_START( taotaido_writemem )
	{ 0x000000, 0x0fffff, MWA16_ROM },
	{ 0x800000, 0x803fff, taotaido_bgvideoram_w, &taotaido_bgram },	/* bg ram?*/
	{ 0xa00000, 0xa01fff, MWA16_RAM, &taotaido_spriteram },		/* sprite ram*/
	{ 0xc00000, 0xc0ffff, MWA16_RAM, &taotaido_spriteram2 },		/* sprite tile lookup ram*/
	{ 0xfe0000, 0xfeffff, MWA16_RAM },						/* main ram*/
	{ 0xffc000, 0xffcfff, paletteram16_xRRRRRGGGGGBBBBB_word_w, &paletteram16 },	/* palette ram*/
	{ 0xffe000, 0xffe3ff, MWA16_RAM, &taotaido_scrollram },		/* rowscroll / rowselect / scroll ram*/

	{ 0xffff00, 0xffff0f, taotaido_tileregs_w },
	{ 0xffff10, 0xffff11, MWA16_NOP },						/* unknown*/
	{ 0xffff20, 0xffff21, MWA16_NOP },						/* unknown - flip screen related*/
	{ 0xffff40, 0xffff47, taotaido_sprite_character_bank_select_w },
	{ 0xffffc0, 0xffffc1, sound_command_w },					/* seems right*/
MEMORY_END

/* sound cpu - same as aerofgt */


static WRITE_HANDLER( pending_command_clear_w )
{
	pending_command = 0;
}

static WRITE_HANDLER( taotaido_sh_bankswitch_w )
{
	data8_t *rom = memory_region(REGION_CPU2) + 0x10000;

	cpu_setbank(1,rom + (data & 0x03) * 0x8000);
}

static MEMORY_READ_START( sound_readmem )
	{ 0x0000, 0x77ff, MRA_ROM },
	{ 0x7800, 0x7fff, MRA_RAM },
	{ 0x8000, 0xffff, MRA_BANK1 },
MEMORY_END

static MEMORY_WRITE_START( sound_writemem )
	{ 0x0000, 0x77ff, MWA_ROM },
	{ 0x7800, 0x7fff, MWA_RAM },
	{ 0x8000, 0xffff, MWA_ROM },
MEMORY_END

static PORT_READ_START( taotaido_sound_readport )
	{ 0x00, 0x00, YM2610_status_port_0_A_r },
	{ 0x02, 0x02, YM2610_status_port_0_B_r },
	{ 0x0c, 0x0c, soundlatch_r },
PORT_END

static PORT_WRITE_START( taotaido_sound_writeport )
	{ 0x00, 0x00, YM2610_control_port_0_A_w },
	{ 0x01, 0x01, YM2610_data_port_0_A_w },
	{ 0x02, 0x02, YM2610_control_port_0_B_w },
	{ 0x03, 0x03, YM2610_data_port_0_B_w },
	{ 0x04, 0x04, taotaido_sh_bankswitch_w },
	{ 0x08, 0x08, pending_command_clear_w },
PORT_END



INPUT_PORTS_START( taotaido )
	PORT_START	/* Player 1 controls (0xffff81.b) */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )	/* "Punch"*/
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )	/* "Kick"*/
#if TAOTAIDO_SHOW_ALL_INPUTS
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER1 )
#else
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
#endif
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* Player 2 controls (0xffff83.b) */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )	/* "Punch"*/
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )	/* "Kick"*/
#if TAOTAIDO_SHOW_ALL_INPUTS
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )
#else
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
#endif
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* System inputs (0xffff85.b) */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )	/* see notes*/
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START2 )
#if TAOTAIDO_SHOW_ALL_INPUTS
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_SERVICE, "Test", KEYCODE_F1, IP_JOY_NONE )
#else
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_SERVICE )	/* "Test" in "test mode"*/
#endif
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_TILT )		/* not working ?*/
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_SERVICE1 )	/* see notes - SERVICE in "test mode"*/
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* VBLANK ? The game freezes when ON*/

	PORT_START	/* DSW A (0xffff87.b -> !0xfe2f6c.w or !0xfe30d0) */
	PORT_DIPNAME( 0x01, 0x01, "Coin Slot" )
	PORT_DIPSETTING(    0x01, "Same" )
	PORT_DIPSETTING(    0x00, "Individual" )
	PORT_DIPNAME( 0x0e, 0x0e, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x0a, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x0e, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_6C ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, "2 Coins to Start, 1 to Continue" )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START	/* DSW B (0xffff89.b -> !0xfe73c2.w or !0xfe751c.w) */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Flip_Screen ) )	/* check code at 0x0963e2 or 0x845e2*/
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Free_Play ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x18, 0x18, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x10, "Easy" )
	PORT_DIPSETTING(    0x18, "Normal" )
	PORT_DIPSETTING(    0x08, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_SERVICE( 0x20, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START	/* DSW C (0xffff8b.b -> !0xfe2f94.w or !0xfe30f8.w) */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) )	/* doesn't seem to be demo sounds*/
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x01, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, "Buy In" )			/* see notes*/
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START	/* Jumpers (0xffff8f.b) */
	PORT_DIPNAME( 0x0f, 0x08, "Country" )
	PORT_DIPSETTING(    0x00, "US" )				/* also (c) Mc O'River Inc*/
	PORT_DIPSETTING(    0x01, "Japan" )
	PORT_DIPSETTING(    0x02, "Hong-Kong/Taiwan" )
/*	PORT_DIPSETTING(    0x03, "Japan" )*/
	PORT_DIPSETTING(    0x04, "Korea" )
/*	PORT_DIPSETTING(    0x05, "Japan" )*/
/*	PORT_DIPSETTING(    0x06, "Japan" )*/
/*	PORT_DIPSETTING(    0x07, "Japan" )*/
	PORT_DIPSETTING(    0x08, "World" )
	/* 0x09 to 0x0f : "Japan" */

#if TAOTAIDO_SHOW_ALL_INPUTS
	/* These inputs are only to fit the test mode - leftover from another game ? */
	PORT_START	/* Player 3 inputs (0xffffa1.b) */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER3 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER3 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER3 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER3 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER3 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER3 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER3 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* Player 4 inputs (0xffffa3.b) */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER4 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER4 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER4 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER4 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER4 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER4 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER4 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )
#endif
INPUT_PORTS_END


static struct GfxLayout taotaido_layout =
{
	16,16,
	RGN_FRAC(1,1),
	4,
	{ 0, 1, 2, 3 },
	{ 1*4, 0*4, 3*4, 2*4, 5*4, 4*4, 7*4, 6*4,
			9*4, 8*4, 11*4, 10*4, 13*4, 12*4, 15*4, 14*4 },
	{ 0*64, 1*64, 2*64, 3*64, 4*64, 5*64, 6*64, 7*64,
			8*64, 9*64, 10*64, 11*64, 12*64, 13*64, 14*64, 15*64 },
	128*8
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &taotaido_layout,  0x000, 256  }, /* sprites */
	{ REGION_GFX2, 0, &taotaido_layout,  0x300, 256  }, /* bg tiles */
	{ -1 } /* end of array */
};

static void irqhandler(int irq)
{
	cpu_set_irq_line(1,0,irq ? ASSERT_LINE : CLEAR_LINE);
}

static struct YM2610interface ym2610_interface =
{
	1,
	8000000,	/* 8 MHz??? */
	{ 25 },
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 },
	{ irqhandler },
	{ REGION_SOUND1 },
	{ REGION_SOUND2 },
	{ YM3012_VOL(100,MIXER_PAN_LEFT,100,MIXER_PAN_RIGHT) }
};

static MACHINE_DRIVER_START( taotaido )
	MDRV_CPU_ADD(M68000, 32000000/2)
	MDRV_CPU_MEMORY(taotaido_readmem,taotaido_writemem)
	MDRV_CPU_VBLANK_INT(irq1_line_hold,1)

	MDRV_CPU_ADD(Z80,20000000/4) /* ??*/
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
	MDRV_CPU_MEMORY(sound_readmem,sound_writemem)
	MDRV_CPU_PORTS(taotaido_sound_readport,taotaido_sound_writeport)
								/* IRQs are triggered by the YM2610 */

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_GFXDECODE(gfxdecodeinfo)

	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER )
	MDRV_SCREEN_SIZE(40*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 40*8-1, 0*8, 28*8-1)
	MDRV_PALETTE_LENGTH(0x800)

	MDRV_VIDEO_START(taotaido)
	MDRV_VIDEO_UPDATE(taotaido)
	MDRV_VIDEO_EOF( taotaido )

	/* sound hardware */
	MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
	MDRV_SOUND_ADD(YM2610, ym2610_interface)
MACHINE_DRIVER_END


ROM_START( taotaido )
	ROM_REGION( 0x100000, REGION_CPU1, 0 ) /* 68000 Code */
	ROM_LOAD16_WORD_SWAP( "1-u90.bin", 0x00000, 0x80000, CRC(a3ee30da) SHA1(920a83ce9192bf785bffdc041e280f1a420de4c9) )
	ROM_LOAD16_WORD_SWAP( "2-u91.bin", 0x80000, 0x80000, CRC(30b7e4fb) SHA1(15e1f6d252c736fdee33b691a0a1a45f0307bffb) )

	ROM_REGION( 0x30000, REGION_CPU2, 0 ) /* z80 Code */
	ROM_LOAD( "3-u113.bin", 0x000000, 0x20000, CRC(a167c4e4) SHA1(d32184e7040935cd440d4d82c66491b710ec87a8) )
	ROM_RELOAD ( 0x10000, 0x20000 )

	ROM_REGION( 0x100000, REGION_SOUND1, 0 ) /* sound samples */
	ROM_LOAD( "u104.bin",     0x000000, 0x100000, CRC(e89387a9) SHA1(1deeee056af367d1a5aa0722dd3d6c68a82d0489) )

	ROM_REGION( 0x200000, REGION_SOUND2, 0 ) /* sound samples */
	ROM_LOAD( "u127.bin",     0x00000, 0x200000, CRC(0cf0cb23) SHA1(a87e7159db2fa0d50446cbf45ec9fbf585b8f396) )

	ROM_REGION( 0x600000, REGION_GFX1, 0 ) /* Sprites */
	ROM_LOAD( "u86.bin", 0x000000, 0x200000, CRC(908e251e) SHA1(5a135787f3263bfb195f8fd1e814c580d840531f) )
	ROM_LOAD( "u87.bin", 0x200000, 0x200000, CRC(c4290ba6) SHA1(4132ffad4668f1dd3f708f009e18435e7dd60120) )
	ROM_LOAD( "u88.bin", 0x400000, 0x200000, CRC(407d9aeb) SHA1(d532c7b80f6c192dba86542fb6eb3ef24fbbbdb9) )

	ROM_REGION( 0x200000, REGION_GFX2, 0 ) /* BG Tiles */
	ROM_LOAD( "u15.bin", 0x000000, 0x200000, CRC(e95823e9) SHA1(362583944ad4fdde4f9e29928cf34376c7ad931f) )
ROM_END

ROM_START( taotaida )
	ROM_REGION( 0x100000, REGION_CPU1, 0 ) /* 68000 Code */
	ROM_LOAD16_WORD_SWAP( "tt0-u90.bin", 0x00000, 0x80000, CRC(69d4cca7) SHA1(f1aba74fef8fe4271d19763f428fc0e2674d08b3) )
	ROM_LOAD16_WORD_SWAP( "tt1-u91.bin", 0x80000, 0x80000, CRC(41025469) SHA1(fa3a424ca3ecb513f418e436e4191ff76f6a0de1) )

	ROM_REGION( 0x30000, REGION_CPU2, 0 ) /* z80 Code */
	ROM_LOAD( "3-u113.bin", 0x000000, 0x20000, CRC(a167c4e4) SHA1(d32184e7040935cd440d4d82c66491b710ec87a8) )
	ROM_RELOAD ( 0x10000, 0x20000 )

	ROM_REGION( 0x100000, REGION_SOUND1, 0 ) /* sound samples */
	ROM_LOAD( "u104.bin",     0x000000, 0x100000, CRC(e89387a9) SHA1(1deeee056af367d1a5aa0722dd3d6c68a82d0489) )

	ROM_REGION( 0x200000, REGION_SOUND2, 0 ) /* sound samples */
	ROM_LOAD( "u127.bin",     0x00000, 0x200000, CRC(0cf0cb23) SHA1(a87e7159db2fa0d50446cbf45ec9fbf585b8f396) )

	ROM_REGION( 0x600000, REGION_GFX1, 0 ) /* Sprites */
	ROM_LOAD( "u86.bin", 0x000000, 0x200000, CRC(908e251e) SHA1(5a135787f3263bfb195f8fd1e814c580d840531f) )
	ROM_LOAD( "u87.bin", 0x200000, 0x200000, CRC(c4290ba6) SHA1(4132ffad4668f1dd3f708f009e18435e7dd60120) )
	ROM_LOAD( "u88.bin", 0x400000, 0x200000, CRC(407d9aeb) SHA1(d532c7b80f6c192dba86542fb6eb3ef24fbbbdb9) )

	ROM_REGION( 0x200000, REGION_GFX2, 0 ) /* BG Tiles */
	ROM_LOAD( "u15.bin", 0x000000, 0x200000, CRC(e95823e9) SHA1(362583944ad4fdde4f9e29928cf34376c7ad931f) )
ROM_END

GAMEX( 1993, taotaido, 0,        taotaido, taotaido, 0, ROT0, "Video System Co.", "Tao Taido (set 1)", GAME_NO_COCKTAIL )
GAMEX( 1993, taotaida, taotaido, taotaido, taotaido, 0, ROT0, "Video System Co.", "Tao Taido (set 2)", GAME_NO_COCKTAIL )
