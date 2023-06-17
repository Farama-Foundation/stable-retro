/******************************************************************

Cabal  (c)1998 Tad

driver by Carlos A. Lozano Baides

68000 + Z80

The original uses 2xYM3931 for sound
The bootleg uses YM2151 + 2xZ80 used as ADPCM players

Coin inputs are handled by the sound CPU, so they don't work with sound
disabled. Use the service switch instead.


MEMORY MAP
0x00000 - 0x3ffff   ROM
0x40000 - 0x4ffff   RAM
[of which: 0x43800 - 0x43fff   VRAM (Sprites)]
0x60000 - 0x607ff   VRAM (Tiles)
0x80000 - 0x803ff   VRAM (Background)
0xa0000 - 0xa000f   Input Ports
0xc0040 - 0xc0040   Watchdog??
0xc0080 - 0xc0080   Screen Flip (+ others?)
0xe0000 - 0xe07ff   COLORRAM (----BBBBGGGGRRRR)
0xe8000 - 0xe800f   Communication with sound CPU (also coins)

VRAM (Background)
0x80000 - 0x801ff  (16x16 of 16x16 tiles, 2 bytes per tile)
0x80200 - 0x803ff  unused foreground layer??

VRAM (Text)
0x60000 - 0x607ff  (32x32 of 8x8 tiles, 2 bytes per tile)

VRAM (Sprites)
0x43800 - 0x43bff  (128 sprites, 8 bytes every sprite)

COLORRAM (Colors)
0xe0000 - 0xe07ff  (1024 colors, ----BBBBGGGGRRRR)

******************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "cpu/z80/z80.h"
#include "sndhrdw/seibu.h"

extern VIDEO_START( cabal );
extern VIDEO_UPDATE( cabal );
WRITE16_HANDLER( cabal_flipscreen_w );
WRITE16_HANDLER( cabal_background_videoram16_w );
WRITE16_HANDLER( cabal_text_videoram16_w );


static int cabal_sound_command1, cabal_sound_command2;

static MACHINE_INIT( cabalbl )
{
	cabal_sound_command1 = cabal_sound_command2 = 0xff;
}


/******************************************************************************************/


static void cabalbl_play_adpcm( int channel, int which ){
	if( which!=0xff ){
		unsigned char *RAM = memory_region(REGION_SOUND1);
		int offset = channel*0x10000;
		int start, len;

		which = which&0x7f;
		if( which ){
			which = which*2+0x100;
			start = RAM[offset+which] + 256*RAM[offset+which+1];
			len = (RAM[offset+start]*256 + RAM[offset+start+1])*2;
			start+=2;
			ADPCM_play( channel,offset+start,len );
		}
	}
}

static WRITE16_HANDLER( cabalbl_sndcmd_w )
{
	switch (offset)
	{
		case 0x0:
			cabal_sound_command1 = data;
			break;

		case 0x1: /* ?? */
			cabal_sound_command2 = data & 0xff;
			break;
	}
}



static int last[4];

static WRITE16_HANDLER( track_reset_w )
{
	int i;

	for (i = 0;i < 4;i++)
		last[i] = readinputport(3+i);
}

static READ16_HANDLER( track_r )
{
	switch (offset)
	{
		default:
		case 0:	return (( readinputport(3) - last[0]) & 0x00ff)		  | (((readinputport(5) - last[2]) & 0x00ff) << 8);	/* X lo */
		case 1:	return (((readinputport(3) - last[0]) & 0xff00) >> 8) | (( readinputport(5) - last[2]) & 0xff00);		/* X hi */
		case 2:	return (( readinputport(4) - last[1]) & 0x00ff)		  | (((readinputport(6) - last[3]) & 0x00ff) << 8);	/* Y lo */
		case 3:	return (((readinputport(4) - last[1]) & 0xff00) >> 8) | (( readinputport(6) - last[3]) & 0xff00);		/* Y hi */
	}
}


WRITE16_HANDLER( cabal_sound_irq_trigger_word_w )
{
	seibu_main_word_w(4,data,mem_mask);

	/* spin for a while to let the Z80 read the command, otherwise coins "stick" */
	cpu_spinuntil_time(TIME_IN_USEC(50));
}

WRITE16_HANDLER( cabalbl_sound_irq_trigger_word_w )
{
	cpu_set_irq_line( 1, IRQ_LINE_NMI, PULSE_LINE );
}


static MEMORY_READ16_START( readmem_cpu )
	{ 0x00000, 0x3ffff, MRA16_ROM },
	{ 0x40000, 0x437ff, MRA16_RAM },
	{ 0x43800, 0x43fff, MRA16_RAM },
	{ 0x44000, 0x4ffff, MRA16_RAM },
	{ 0x60000, 0x607ff, MRA16_RAM },
	{ 0x80000, 0x801ff, MRA16_RAM },
	{ 0x80200, 0x803ff, MRA16_RAM },
	{ 0xa0000, 0xa0001, input_port_0_word_r },
	{ 0xa0008, 0xa000f, track_r },
	{ 0xa0010, 0xa0011, input_port_1_word_r },
	{ 0xe0000, 0xe07ff, MRA16_RAM },
	{ 0xe8000, 0xe800d, seibu_main_word_r },
MEMORY_END

static MEMORY_WRITE16_START( writemem_cpu )
	{ 0x00000, 0x3ffff, MWA16_ROM },
	{ 0x40000, 0x437ff, MWA16_RAM },
	{ 0x43800, 0x43fff, MWA16_RAM, &spriteram16, &spriteram_size },
	{ 0x44000, 0x4ffff, MWA16_RAM },
	{ 0x60000, 0x607ff, cabal_text_videoram16_w, &colorram16 },
	{ 0x80000, 0x801ff, cabal_background_videoram16_w, &videoram16, &videoram_size },
	{ 0x80200, 0x803ff, MWA16_RAM },
	{ 0xc0000, 0xc0001, track_reset_w },
	{ 0xc0040, 0xc0041, MWA16_NOP }, /* ??? */
	{ 0xc0080, 0xc0081, cabal_flipscreen_w },
	{ 0xe0000, 0xe07ff, paletteram16_xxxxBBBBGGGGRRRR_word_w, &paletteram16 },
	{ 0xe8008, 0xe8009, cabal_sound_irq_trigger_word_w },	/* fix coin insertion*/
	{ 0xe8000, 0xe800d, seibu_main_word_w },
MEMORY_END

static MEMORY_READ16_START( cabalbl_readmem_cpu )
	{ 0x00000, 0x3ffff, MRA16_ROM },
	{ 0x40000, 0x437ff, MRA16_RAM },
	{ 0x43800, 0x43fff, MRA16_RAM },
	{ 0x44000, 0x4ffff, MRA16_RAM },
	{ 0x60000, 0x607ff, MRA16_RAM },
	{ 0x80000, 0x801ff, MRA16_RAM },
	{ 0x80200, 0x803ff, MRA16_RAM },
	{ 0xa0000, 0xa0001, input_port_0_word_r },
	{ 0xa0008, 0xa0009, input_port_1_word_r },
	{ 0xa0010, 0xa0011, input_port_2_word_r },
	{ 0xe0000, 0xe07ff, MRA16_RAM },
	{ 0xe8004, 0xe8005, soundlatch2_word_r },
MEMORY_END

static MEMORY_WRITE16_START( cabalbl_writemem_cpu )
	{ 0x00000, 0x3ffff, MWA16_ROM },
	{ 0x40000, 0x437ff, MWA16_RAM },
	{ 0x43800, 0x43fff, MWA16_RAM, &spriteram16, &spriteram_size },
	{ 0x44000, 0x4ffff, MWA16_RAM },
	{ 0x60000, 0x607ff, cabal_text_videoram16_w, &colorram16 },
	{ 0x80000, 0x801ff, cabal_background_videoram16_w, &videoram16, &videoram_size },
	{ 0x80200, 0x803ff, MWA16_RAM },
	{ 0xc0040, 0xc0041, MWA16_NOP }, /* ??? */
	{ 0xc0080, 0xc0081, cabal_flipscreen_w },
	{ 0xe0000, 0xe07ff, paletteram16_xxxxBBBBGGGGRRRR_word_w, &paletteram16 },
	{ 0xe8000, 0xe8003, cabalbl_sndcmd_w },
	{ 0xe8008, 0xe8009, cabalbl_sound_irq_trigger_word_w },
MEMORY_END

/*********************************************************************/

static READ_HANDLER( cabalbl_snd_r )
{
	switch(offset){
		case 0x06: return input_port_3_r(0);
		case 0x08: return cabal_sound_command2;
		case 0x0a: return cabal_sound_command1;
		default: return(0xff);
	}
}

static WRITE_HANDLER( cabalbl_snd_w )
{
	switch( offset ){
		case 0x00: cabalbl_play_adpcm( 0, data ); break;
		case 0x02: cabalbl_play_adpcm( 1, data ); break;
     }
}

static MEMORY_READ_START( readmem_sound )
	{ 0x0000, 0x1fff, MRA_ROM },
	{ 0x2000, 0x27ff, MRA_RAM },
	{ 0x4009, 0x4009, YM2151_status_port_0_r },
	{ 0x4010, 0x4011, seibu_soundlatch_r },
	{ 0x4012, 0x4012, seibu_main_data_pending_r },
	{ 0x4013, 0x4013, input_port_2_r },
	{ 0x8000, 0xffff, MRA_ROM },
MEMORY_END

static MEMORY_WRITE_START( writemem_sound )
	{ 0x0000, 0x1fff, MWA_ROM },
	{ 0x2000, 0x27ff, MWA_RAM },
	{ 0x4001, 0x4001, seibu_irq_clear_w },
	{ 0x4002, 0x4002, seibu_rst10_ack_w },
	{ 0x4003, 0x4003, seibu_rst18_ack_w },
	{ 0x4005, 0x4006, seibu_adpcm_adr_1_w },
	{ 0x401a, 0x401a, seibu_adpcm_ctl_1_w },
	{ 0x4008, 0x4008, YM2151_register_port_0_w },
	{ 0x4009, 0x4009, YM2151_data_port_0_w },
	{ 0x4018, 0x4019, seibu_main_data_w },
	{ 0x401b, 0x401b, seibu_coin_w },
	{ 0x6005, 0x6006, seibu_adpcm_adr_2_w },
	{ 0x601a, 0x601a, seibu_adpcm_ctl_2_w },
	{ 0x8000, 0xffff, MWA_ROM },
MEMORY_END

static MEMORY_READ_START( cabalbl_readmem_sound )
	{ 0x0000, 0x1fff, MRA_ROM },
	{ 0x2000, 0x2fff, MRA_RAM },
	{ 0x4000, 0x400d, cabalbl_snd_r },
	{ 0x400f, 0x400f, YM2151_status_port_0_r },
	{ 0x8000, 0xffff, MRA_ROM },
MEMORY_END

static MEMORY_WRITE_START( cabalbl_writemem_sound )
	{ 0x0000, 0x1fff, MWA_ROM },
	{ 0x2000, 0x2fff, MWA_RAM },
	{ 0x4000, 0x4002, cabalbl_snd_w },
	{ 0x400c, 0x400c, soundlatch2_w },
	{ 0x400e, 0x400e, YM2151_register_port_0_w },
	{ 0x400f, 0x400f, YM2151_data_port_0_w },
	{ 0x6000, 0x6000, MWA_NOP },  /*???*/
	{ 0x8000, 0xffff, MWA_ROM },
MEMORY_END

/* ADPCM CPU (common) */

#if 0
static MEMORY_READ_START( cabalbl_readmem_adpcm )
	{ 0x0000, 0xffff, MRA_ROM },
MEMORY_END
static MEMORY_WRITE_START( cabalbl_writemem_adpcm )
	{ 0x0000, 0xffff, MWA_NOP },
MEMORY_END
#endif


/***************************************************************************/

INPUT_PORTS_START( cabal )
	PORT_START
/* Coin Mode 1, todo Mode 2 */
	PORT_DIPNAME( 0x000f, 0x000f, DEF_STR( Coinage ) )
	PORT_DIPSETTING(      0x000a, DEF_STR( 6C_1C ) )
	PORT_DIPSETTING(      0x000b, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(      0x000c, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(      0x000d, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0001, DEF_STR( 8C_3C ) )
	PORT_DIPSETTING(      0x000e, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( 5C_3C ) )
	PORT_DIPSETTING(      0x0003, DEF_STR( 3C_2C ) )
	PORT_DIPSETTING(      0x000f, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0004, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(      0x0009, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x0008, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x0007, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(      0x0006, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(      0x0005, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Free_Play ) )
/* mode 2
	PORT_DIPNAME( 0x0003, 0x0003, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(      0x0001, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0003, DEF_STR( 1C_1C ) )
	PORT_DIPNAME( 0x000c, 0x000c, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(      0x000c, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x0008, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x0004, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 1C_6C ) )
*/
	PORT_DIPNAME( 0x0010, 0x0010, "Coin Mode" )
	PORT_DIPSETTING(      0x0010, "Mode 1" )
	PORT_DIPSETTING(      0x0000, "Mode 2" )
	PORT_DIPNAME( 0x0020, 0x0020, "Invert Buttons" )
	PORT_DIPSETTING(      0x0020, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0040, 0x0040, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(      0x0040, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0080, 0x0080, "Trackball" )
	PORT_DIPSETTING(      0x0080, "Small" )
	PORT_DIPSETTING(      0x0000, "Large" )
	PORT_DIPNAME( 0x0300, 0x0300, DEF_STR( Lives ) )
	PORT_DIPSETTING(      0x0200, "2" )
	PORT_DIPSETTING(      0x0300, "3" )
	PORT_DIPSETTING(      0x0100, "5" )
	PORT_BITX( 0,         0x0000, IPT_DIPSWITCH_SETTING | IPF_CHEAT, "121", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPNAME( 0x0c00, 0x0c00, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(      0x0c00, "20k 50k" )
	PORT_DIPSETTING(      0x0800, "30k 100k" )
	PORT_DIPSETTING(      0x0400, "50k 150k" )
	PORT_DIPSETTING(      0x0000, "70K" )
	PORT_DIPNAME( 0x3000, 0x2000, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(      0x3000, "Easy" )
	PORT_DIPSETTING(      0x2000, "Normal" )
	PORT_DIPSETTING(      0x1000, "Hard" )
	PORT_DIPSETTING(      0x0000, "Very Hard" )
	PORT_DIPNAME( 0x4000, 0x4000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x4000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x8000, 0x8000, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x8000, DEF_STR( On ) )

	PORT_START	/* IN0 */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x0ff0, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER1 )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START
	PORT_BIT_IMPULSE( 0x01, IP_ACTIVE_HIGH, IPT_COIN1, 4 )	/* read through sound cpu */
	PORT_BIT_IMPULSE( 0x02, IP_ACTIVE_HIGH, IPT_COIN2, 4 )	/* read through sound cpu */

	PORT_START	/* IN0 */
	PORT_BIT( 0x00ff, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* IN1 */
	PORT_BIT( 0x00ff, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )

	PORT_START	/* IN3 */
	PORT_BIT( 0x00ff, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END

INPUT_PORTS_START( cabalt )
	PORT_START
/* Coin Mode 1, todo Mode 2 */
	PORT_DIPNAME( 0x000f, 0x000f, DEF_STR( Coinage ) )
	PORT_DIPSETTING(      0x000a, DEF_STR( 6C_1C ) )
	PORT_DIPSETTING(      0x000b, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(      0x000c, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(      0x000d, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0001, DEF_STR( 8C_3C ) )
	PORT_DIPSETTING(      0x000e, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( 5C_3C ) )
	PORT_DIPSETTING(      0x0003, DEF_STR( 3C_2C ) )
	PORT_DIPSETTING(      0x000f, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0004, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(      0x0009, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x0008, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x0007, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(      0x0006, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(      0x0005, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Free_Play ) )
/* mode 2
	PORT_DIPNAME( 0x0003, 0x0003, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(      0x0001, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0003, DEF_STR( 1C_1C ) )
	PORT_DIPNAME( 0x000c, 0x000c, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(      0x000c, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x0008, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x0004, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 1C_6C ) )
*/
	PORT_DIPNAME( 0x0010, 0x0010, "Coin Mode" )
	PORT_DIPSETTING(      0x0010, "Mode 1" )
	PORT_DIPSETTING(      0x0000, "Mode 2" )
	PORT_DIPNAME( 0x0020, 0x0020, "Invert Buttons" )
	PORT_DIPSETTING(      0x0020, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0040, 0x0040, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(      0x0040, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0080, 0x0080, "Trackball" )
	PORT_DIPSETTING(      0x0080, "Small" )
	PORT_DIPSETTING(      0x0000, "Large" )
	PORT_DIPNAME( 0x0300, 0x0300, DEF_STR( Lives ) )
	PORT_DIPSETTING(      0x0200, "2" )
	PORT_DIPSETTING(      0x0300, "3" )
	PORT_DIPSETTING(      0x0100, "5" )
	PORT_BITX( 0,         0x0000, IPT_DIPSWITCH_SETTING | IPF_CHEAT, "121", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPNAME( 0x0c00, 0x0c00, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(      0x0c00, "20k 50k" )
	PORT_DIPSETTING(      0x0800, "30k 100k" )
	PORT_DIPSETTING(      0x0400, "50k 150k" )
	PORT_DIPSETTING(      0x0000, "70K" )
	PORT_DIPNAME( 0x3000, 0x2000, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(      0x3000, "Easy" )
	PORT_DIPSETTING(      0x2000, "Normal" )
	PORT_DIPSETTING(      0x1000, "Hard" )
	PORT_DIPSETTING(      0x0000, "Very Hard" )
	PORT_DIPNAME( 0x4000, 0x4000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x4000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x8000, 0x8000, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x8000, DEF_STR( On ) )

	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x0ff0, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START
	PORT_BIT_IMPULSE( 0x01, IP_ACTIVE_HIGH, IPT_COIN1, 4 )	/* read through sound cpu */
	PORT_BIT_IMPULSE( 0x02, IP_ACTIVE_HIGH, IPT_COIN2, 4 )	/* read through sound cpu */

	PORT_START
	PORT_ANALOG( 0x0fff, 0x0000, IPT_TRACKBALL_X | IPF_PLAYER1, 100, 30, 0, 0 )

	PORT_START
	PORT_ANALOG( 0x0fff, 0x0000, IPT_TRACKBALL_Y | IPF_REVERSE | IPF_PLAYER1, 100, 30, 0, 0 )

	PORT_START
	PORT_ANALOG( 0x0fff, 0x0000, IPT_TRACKBALL_X | IPF_PLAYER2, 100, 30, 0, 0 )

	PORT_START
	PORT_ANALOG( 0x0fff, 0x0000, IPT_TRACKBALL_Y | IPF_PLAYER2, 100, 30, 0, 0 )
INPUT_PORTS_END

INPUT_PORTS_START( cabalbl )
	PORT_START
/* Coin Mode 1, todo Mode 2 */
	PORT_DIPNAME( 0x000f, 0x000f, DEF_STR( Coinage ) )
	PORT_DIPSETTING(      0x000a, DEF_STR( 6C_1C ) )
	PORT_DIPSETTING(      0x000b, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(      0x000c, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(      0x000d, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0001, DEF_STR( 8C_3C ) )
	PORT_DIPSETTING(      0x000e, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( 5C_3C ) )
	PORT_DIPSETTING(      0x0003, DEF_STR( 3C_2C ) )
	PORT_DIPSETTING(      0x000f, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0004, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(      0x0009, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x0008, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x0007, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(      0x0006, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(      0x0005, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Free_Play ) )
/* mode 2
	PORT_DIPNAME( 0x0003, 0x0003, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(      0x0001, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0003, DEF_STR( 1C_1C ) )
	PORT_DIPNAME( 0x000c, 0x000c, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(      0x000c, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x0008, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x0004, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 1C_6C ) )
*/
	PORT_DIPNAME( 0x0010, 0x0010, "Coin Mode" )
	PORT_DIPSETTING(      0x0010, "Mode 1" )
	PORT_DIPSETTING(      0x0000, "Mode 2" )
	PORT_DIPNAME( 0x0020, 0x0020, "Invert Buttons" )
	PORT_DIPSETTING(      0x0020, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0040, 0x0040, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(      0x0040, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0080, 0x0080, "Trackball" )
	PORT_DIPSETTING(      0x0080, "Small" )
	PORT_DIPSETTING(      0x0000, "Large" )
	PORT_DIPNAME( 0x0300, 0x0300, DEF_STR( Lives ) )
	PORT_DIPSETTING(      0x0200, "2" )
	PORT_DIPSETTING(      0x0300, "3" )
	PORT_DIPSETTING(      0x0100, "5" )
	PORT_BITX( 0,         0x0000, IPT_DIPSWITCH_SETTING | IPF_CHEAT, "121", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPNAME( 0x0c00, 0x0c00, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(      0x0c00, "20k 50k" )
	PORT_DIPSETTING(      0x0800, "30k 100k" )
	PORT_DIPSETTING(      0x0400, "50k 150k" )
	PORT_DIPSETTING(      0x0000, "70K" )
	PORT_DIPNAME( 0x3000, 0x2000, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(      0x3000, "Easy" )
	PORT_DIPSETTING(      0x2000, "Normal" )
	PORT_DIPSETTING(      0x1000, "Hard" )
	PORT_DIPSETTING(      0x0000, "Very Hard" )
	PORT_DIPNAME( 0x4000, 0x4000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x4000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x8000, 0x8000, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x8000, DEF_STR( On ) )

	PORT_START	/* IN0 */
	PORT_BIT( 0x00ff, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )

	PORT_START	/* IN1 */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x00f0, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0f00, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER1 )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START
	PORT_BIT_IMPULSE( 0x01, IP_ACTIVE_HIGH, IPT_COIN1, 4 )	/* read through sound cpu */
	PORT_BIT_IMPULSE( 0x02, IP_ACTIVE_HIGH, IPT_COIN2, 4 )	/* read through sound cpu */
INPUT_PORTS_END



static struct GfxLayout text_layout =
{
	8,8,
	RGN_FRAC(1,1),
	2,
	{ 0,4 },
	{ 3, 2, 1, 0, 8+3, 8+2, 8+1, 8+0},
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16 },
	16*8
};

static struct GfxLayout tile_layout =
{
	16,16,
	RGN_FRAC(1,1),
	4,
	{ 8, 12, 0, 4 },
	{ 3, 2, 1, 0, 16+3, 16+2, 16+1, 16+0,
			32*16+3, 32*16+2, 32*16+1, 32*16+0, 33*16+3, 33*16+2, 33*16+1, 33*16+0 },
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32,
			8*32, 9*32, 10*32,  11*32,  12*32,  13*32, 14*32,  15*32 },
	64*16
};

static struct GfxLayout sprite_layout =
{
	16,16,
	RGN_FRAC(1,1),
	4,
	{ 8, 12, 0, 4 },
	{ 3, 2, 1, 0, 16+3, 16+2, 16+1, 16+0,
			32+3, 32+2, 32+1, 32+0, 48+3, 48+2, 48+1, 48+0 },
	{ 30*32, 28*32, 26*32, 24*32, 22*32, 20*32, 18*32, 16*32,
			14*32, 12*32, 10*32,  8*32,  6*32,  4*32,  2*32,  0*32 },
	64*16
};

static struct GfxDecodeInfo cabal_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &text_layout,		0, 1024/4 },
	{ REGION_GFX2, 0, &tile_layout,		32*16, 16 },
	{ REGION_GFX3, 0, &sprite_layout,	16*16, 16 },
	{ -1 }
};



static struct YM2151interface ym2151_interface =
{
 	1,			/* 1 chip */
 	3579580,	/* 3.58 MHz ? */ /* 4 MHz in raine */
	{ YM3012_VOL(80,MIXER_PAN_LEFT,80,MIXER_PAN_RIGHT) },
	{ seibu_ym3812_irqhandler },
};

static void irqhandler(int irq)
{
	cpu_set_irq_line(1,0,irq ? ASSERT_LINE : CLEAR_LINE);
}

static struct YM2151interface cabalbl_ym2151_interface =
{
 	1,			/* 1 chip */
 	3579580,	/* 3.58 MHz ? */ /* 4 MHz in raine */
	{ YM3012_VOL(80,MIXER_PAN_LEFT,80,MIXER_PAN_RIGHT) },
	{ irqhandler },
};

SEIBU_SOUND_SYSTEM_ADPCM_HARDWARE

static MACHINE_DRIVER_START( cabal )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 12000000) /* 12 MHz */
	MDRV_CPU_MEMORY(readmem_cpu,writemem_cpu)
	MDRV_CPU_VBLANK_INT(irq1_line_hold,1)

	MDRV_CPU_ADD(Z80, 4000000)
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)	/* 4 MHz */
	MDRV_CPU_MEMORY(readmem_sound,writemem_sound)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)

	MDRV_MACHINE_INIT(seibu_sound_1)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(256, 256)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 2*8, 30*8-1)
	MDRV_GFXDECODE(cabal_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(1024)

	MDRV_VIDEO_START(cabal)
	MDRV_VIDEO_UPDATE(cabal)

	/* sound hardware */
	MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
	MDRV_SOUND_ADD(YM2151, ym2151_interface)
	SEIBU_SOUND_SYSTEM_ADPCM_INTERFACE
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( cabalbl )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 12000000) /* 12 MHz */
	MDRV_CPU_MEMORY(cabalbl_readmem_cpu,cabalbl_writemem_cpu)
	MDRV_CPU_VBLANK_INT(irq1_line_hold,1)

	MDRV_CPU_ADD(Z80, 4000000)
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)	/* 4 MHz */
	MDRV_CPU_MEMORY(cabalbl_readmem_sound,cabalbl_writemem_sound)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(10)

	MDRV_MACHINE_INIT(cabalbl)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(256, 256)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 2*8, 30*8-1)
	MDRV_GFXDECODE(cabal_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(1024)

	MDRV_VIDEO_START(cabal)
	MDRV_VIDEO_UPDATE(cabal)

	/* sound hardware */
	MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
	MDRV_SOUND_ADD(YM2151, cabalbl_ym2151_interface)
	MDRV_SOUND_ADD(ADPCM, adpcm_interface)
MACHINE_DRIVER_END



ROM_START( cabal )
	ROM_REGION( 0x50000, REGION_CPU1, 0 )	/* 64k for cpu code */
	ROM_LOAD16_BYTE( "13.7h",    0x00000, 0x10000, CRC(00abbe0c) SHA1(bacf17444abfb4f56248ff56e37b0aa2b1a3800d) )
	ROM_LOAD16_BYTE( "11.6h",    0x00001, 0x10000, CRC(44736281) SHA1(1d6da95ef96d9c02aea70791e1cb87b70097d5ed) )
	ROM_LOAD16_BYTE( "12.7j",    0x20000, 0x10000, CRC(d763a47c) SHA1(146d8082a404b6eddaf2dc9ba41a997949c17f8a) )
	ROM_LOAD16_BYTE( "10.6j",    0x20001, 0x10000, CRC(96d5e8af) SHA1(ed7d854f08e87db5ae6cf526eafa029dfd2bfb9f) )

	ROM_REGION( 0x10000*2, REGION_CPU2, 0 )	/* 64k for sound cpu code */
	ROM_LOAD( "4-3n",         0x0000, 0x2000, CRC(4038eff2) SHA1(0bcafc1b78c3bef9a0e9b822c482ea4a942fd180) )
	ROM_LOAD( "3-3p",         0x8000, 0x8000, CRC(d9defcbf) SHA1(f26b10b1dbe5aa6446f70fd18e5f1379455578ec) )

	ROM_REGION( 0x4000,  REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "5-6s",           0x00000, 0x04000, CRC(6a76955a) SHA1(733cb4b862b5dac97c2641b58f2362471e62fcf2) ) /* characters */

	/* The Joystick versions use a sub-board instead of the mask roms
       the content is the same as the mask roms */
	ROM_REGION( 0x80000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD16_BYTE( "bg_rom1.bin",   0x00000, 0x10000, CRC(1023319b) SHA1(38fcc8159776b82779b3163329b07c61be939fae) )
	ROM_LOAD16_BYTE( "bg_rom2.bin",   0x00001, 0x10000, CRC(3b6d2b09) SHA1(4cdcd22836dce4ee6348c4e6df7c6360d12ef912) )
	ROM_LOAD16_BYTE( "bg_rom3.bin",   0x20000, 0x10000, CRC(420b0801) SHA1(175be6e3ca3cb98672e4cdbc9b5f5b007bc531c9) )
	ROM_LOAD16_BYTE( "bg_rom4.bin",   0x20001, 0x10000, CRC(77bc7a60) SHA1(4d148241835f6a6b63f66494636c09a1fc1d3c06) )
	ROM_LOAD16_BYTE( "bg_rom5.bin",   0x40000, 0x10000, CRC(543fcb37) SHA1(78c40f6a78a8b9ca9f73fc67fc87f78b15e7abbe) )
	ROM_LOAD16_BYTE( "bg_rom6.bin",   0x40001, 0x10000, CRC(0bc50075) SHA1(565eb59b41f71fb69f62397f9747f5ae18b83009) )
	ROM_LOAD16_BYTE( "bg_rom7.bin",   0x60000, 0x10000, CRC(d28d921e) SHA1(e133de5129a33ca9ff449948a959621bbfc58c11) )
	ROM_LOAD16_BYTE( "bg_rom8.bin",   0x60001, 0x10000, CRC(67e4fe47) SHA1(15620fc5e985a249677da333b77331e40d2b24ab) )

	ROM_REGION( 0x80000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD16_BYTE( "sp_rom1.bin",   0x00000, 0x10000, CRC(34d3cac8) SHA1(a6a2304fb576267db2c72cfbf0a3f66740ebe60e) )
	ROM_LOAD16_BYTE( "sp_rom2.bin",   0x00001, 0x10000, CRC(4e49c28e) SHA1(ea74443a9423b14611a1f97e44692badfedd0ead) )
	ROM_LOAD16_BYTE( "sp_rom3.bin",   0x20000, 0x10000, CRC(7065e840) SHA1(baa8cd28be60c678d782ecfabde6cd5e36480415) )
	ROM_LOAD16_BYTE( "sp_rom4.bin",   0x20001, 0x10000, CRC(6a0e739d) SHA1(e3f4f5b4587f573426ec00417f33e94a257c77e6) )
	ROM_LOAD16_BYTE( "sp_rom5.bin",   0x40000, 0x10000, CRC(0e1ec30e) SHA1(4b1f092fc1e92da0f92e55d1548db7961a13f717) )
	ROM_LOAD16_BYTE( "sp_rom6.bin",   0x40001, 0x10000, CRC(581a50c1) SHA1(5afd65c15a0a63a54727e6d882011f0718a9fefc) )
	ROM_LOAD16_BYTE( "sp_rom7.bin",   0x60000, 0x10000, CRC(55c44764) SHA1(7fad1f2084664b5b4d1384c8081371b0c79c4f5e) )
	ROM_LOAD16_BYTE( "sp_rom8.bin",   0x60001, 0x10000, CRC(702735c9) SHA1(e4ac799dc85ff5b7c8e578611605989c78f9e8b3) )

	ROM_REGION( 0x20000, REGION_SOUND1, 0 )	/* Samples */
	ROM_LOAD( "2-1s",           0x00000, 0x10000, CRC(850406b4) SHA1(23ac1650c6d6f35607a5264b3aa89868401a645a) )
	ROM_LOAD( "1-1u",           0x10000, 0x10000, CRC(8b3e0789) SHA1(b1450db1b1bada237c90930623e4def321099f13) )
ROM_END

ROM_START( cabala )
	ROM_REGION( 0x50000, REGION_CPU1, 0 )	/* 64k for cpu code */
	ROM_LOAD16_BYTE( "epr-a-9.7h",    0x00000, 0x10000, CRC(00abbe0c) SHA1(bacf17444abfb4f56248ff56e37b0aa2b1a3800d) )
	ROM_LOAD16_BYTE( "epr-a-7.6h",    0x00001, 0x10000, CRC(c89608db) SHA1(a56e77526227af5b693eea9ef74da0d9d57cc55c) )
	ROM_LOAD16_BYTE( "epr-a-8.7k",    0x20000, 0x08000, CRC(fe84788a) SHA1(29c49ebbe62357c27befcdcc4c19841a8bf32b2d) )
	ROM_RELOAD(0x30000,0x08000)
	ROM_LOAD16_BYTE( "epr-a-6.6k",    0x20001, 0x08000, CRC(81eb1355) SHA1(bbf926d40164d78319e982da0e8fb8ec4d4f8b87) )
	ROM_RELOAD(0x30001,0x08000)

	ROM_REGION( 0x10000*2, REGION_CPU2, 0 )	/* 64k for sound cpu code */
	ROM_LOAD( "epr-a-4.3n",         0x0000, 0x2000, CRC(4038eff2) SHA1(0bcafc1b78c3bef9a0e9b822c482ea4a942fd180) )
	ROM_LOAD( "epr-a-3.3p",         0x8000, 0x4000, CRC(c0097c55) SHA1(874f813c1b466dab2d15a707e340b9bdb200246c) )

	ROM_REGION( 0x8000,  REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "epr-a-5.6s",           0x00000, 0x08000, CRC(189033fd) SHA1(814f0cbc5f72345c04922d6d7c986f99d57335fa) ) /* characters */

	ROM_REGION( 0x80000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "tad-2.7s",       0x00000, 0x80000, CRC(13ca7ae1) SHA1(b26bb4876a6518e3809e0fa4d442616508b3e7e8) ) /* tiles */

	ROM_REGION( 0x80000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "tad-1.5e",       0x00000, 0x80000, CRC(8324a7fe) SHA1(aed4470df35ec18e65e35bddc9c217a5019fdcbf) ) /* sprites */

	ROM_REGION( 0x20000, REGION_SOUND1, 0 )	/* Samples */
	ROM_LOAD( "epr-a-2.1s",           0x00000, 0x10000, CRC(850406b4) SHA1(23ac1650c6d6f35607a5264b3aa89868401a645a) )
	ROM_LOAD( "epr-a-1.1u",           0x10000, 0x10000, CRC(8b3e0789) SHA1(b1450db1b1bada237c90930623e4def321099f13) )
ROM_END

ROM_START( cabalus )
	ROM_REGION( 0x50000, REGION_CPU1, 0 )	/* 64k for cpu code */
	ROM_LOAD16_BYTE( "h7_512.bin",      0x00000, 0x10000, CRC(8fe16fb4) SHA1(fedb2d0c6c21516f68cfa99093772fe8fa862389) )
	ROM_LOAD16_BYTE( "h6_512.bin",      0x00001, 0x10000, CRC(6968101c) SHA1(d65005ac235dae5c32bbcd182cb365e8fa067fe7) )
	ROM_LOAD16_BYTE( "k7_512.bin",      0x20000, 0x10000, CRC(562031a2) SHA1(ed5ef50a66c7797a7f345e479162cf83d6777f7c) )
	ROM_LOAD16_BYTE( "k6_512.bin",      0x20001, 0x10000, CRC(4fda2856) SHA1(a213cb7443cdccbad3f2610e8d42b2e149cbedb9) )

	ROM_REGION( 0x10000*2, REGION_CPU2, 0 )	/* 64k for sound cpu code */
	ROM_LOAD( "4-3n",         0x0000, 0x2000, CRC(4038eff2) SHA1(0bcafc1b78c3bef9a0e9b822c482ea4a942fd180) )
	ROM_LOAD( "3-3p",         0x8000, 0x8000, CRC(d9defcbf) SHA1(f26b10b1dbe5aa6446f70fd18e5f1379455578ec) )

	ROM_REGION( 0x4000,  REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "t6_128.bin",     0x00000, 0x04000, CRC(1ccee214) SHA1(7c842bc1c6002ec90693160fd5407345092420bb) ) /* characters */

	ROM_REGION( 0x80000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "tad-2.7s",       0x00000, 0x80000, CRC(13ca7ae1) SHA1(b26bb4876a6518e3809e0fa4d442616508b3e7e8) ) /* tiles */

	ROM_REGION( 0x80000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "tad-1.5e",       0x00000, 0x80000, CRC(8324a7fe) SHA1(aed4470df35ec18e65e35bddc9c217a5019fdcbf) ) /* sprites */

	ROM_REGION( 0x20000, REGION_SOUND1, 0 )	/* Samples? */
	ROM_LOAD( "2-1s",           0x00000, 0x10000, CRC(850406b4) SHA1(23ac1650c6d6f35607a5264b3aa89868401a645a) )
	ROM_LOAD( "1-1u",           0x10000, 0x10000, CRC(8b3e0789) SHA1(b1450db1b1bada237c90930623e4def321099f13) )

	ROM_REGION( 0x0200, REGION_PROMS, 0 )	/* unknown */
	ROM_LOAD( "prom05.8e",      0x0000, 0x0100, CRC(a94b18c2) SHA1(e7db4c1efc9e313e36eef3f53ae5b2e573a38920) )
	ROM_LOAD( "prom10.4j",      0x0100, 0x0100, CRC(261c93bc) SHA1(942470198143d584d3766f28587d1879abd912c1) )
ROM_END

ROM_START( cabalus2 )
	ROM_REGION( 0x50000, REGION_CPU1, 0 )	/* 64k for cpu code */
	ROM_LOAD16_BYTE( "9-7h",            0x00000, 0x10000, CRC(ebbb9484) SHA1(2c77d5b4acdc37720dc7ccab526862981bf8da51) )
	ROM_LOAD16_BYTE( "7-6h",            0x00001, 0x10000, CRC(51aeb49e) SHA1(df38dc58d8c6fa3d35904bf34e29111e7bd523ad) )
	ROM_LOAD16_BYTE( "8-7k",            0x20000, 0x10000, CRC(4c24ed9a) SHA1(f0fc25c3e7dc8ac71fdad3e91ab618cd7a037123) )
	ROM_LOAD16_BYTE( "6-6k",            0x20001, 0x10000, CRC(681620e8) SHA1(c9eacfb55059986dbecc2fae1339069a852f917b) )

	ROM_REGION( 0x10000*2, REGION_CPU2, 0 )	/* 64k for sound cpu code */
	ROM_LOAD( "4-3n",         0x0000, 0x2000, CRC(4038eff2) SHA1(0bcafc1b78c3bef9a0e9b822c482ea4a942fd180) )
	ROM_LOAD( "3-3p",         0x8000, 0x8000, CRC(d9defcbf) SHA1(f26b10b1dbe5aa6446f70fd18e5f1379455578ec) )

	ROM_REGION( 0x4000,  REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "5-6s",           0x00000, 0x04000, CRC(6a76955a) SHA1(733cb4b862b5dac97c2641b58f2362471e62fcf2) ) /* characters */

	ROM_REGION( 0x80000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "tad-2.7s",       0x00000, 0x80000, CRC(13ca7ae1) SHA1(b26bb4876a6518e3809e0fa4d442616508b3e7e8) ) /* tiles */

	ROM_REGION( 0x80000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "tad-1.5e",       0x00000, 0x80000, CRC(8324a7fe) SHA1(aed4470df35ec18e65e35bddc9c217a5019fdcbf) ) /* sprites */

	ROM_REGION( 0x20000, REGION_SOUND1, 0 )	/* Samples? */
	ROM_LOAD( "2-1s",           0x00000, 0x10000, CRC(850406b4) SHA1(23ac1650c6d6f35607a5264b3aa89868401a645a) )
	ROM_LOAD( "1-1u",           0x10000, 0x10000, CRC(8b3e0789) SHA1(b1450db1b1bada237c90930623e4def321099f13) )

	ROM_REGION( 0x0200, REGION_PROMS, 0 )	/* unknown */
	ROM_LOAD( "prom05.8e",      0x0000, 0x0100, CRC(a94b18c2) SHA1(e7db4c1efc9e313e36eef3f53ae5b2e573a38920) )
	ROM_LOAD( "prom10.4j",      0x0100, 0x0100, CRC(261c93bc) SHA1(942470198143d584d3766f28587d1879abd912c1) )
ROM_END

ROM_START( cabalbl )
	ROM_REGION( 0x50000, REGION_CPU1, 0 )	/* 64k for cpu code */
	ROM_LOAD16_BYTE( "cabal_24.bin",    0x00000, 0x10000, CRC(00abbe0c) SHA1(bacf17444abfb4f56248ff56e37b0aa2b1a3800d) )
	ROM_LOAD16_BYTE( "cabal_22.bin",    0x00001, 0x10000, CRC(78c4af27) SHA1(31049d1ec76d76284682de7a0592f63d97019240) )
	ROM_LOAD16_BYTE( "cabal_23.bin",    0x20000, 0x10000, CRC(d763a47c) SHA1(146d8082a404b6eddaf2dc9ba41a997949c17f8a) )
	ROM_LOAD16_BYTE( "cabal_21.bin",    0x20001, 0x10000, CRC(96d5e8af) SHA1(ed7d854f08e87db5ae6cf526eafa029dfd2bfb9f) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for sound cpu code */
	ROM_LOAD( "cabal_11.bin",    0x0000, 0x10000, CRC(d308a543) SHA1(4f45db42512f83266001daee55d06f49e7908e35) )

	ROM_REGION( 0x4000,  REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "5-6s",           0x00000, 0x04000, CRC(6a76955a) SHA1(733cb4b862b5dac97c2641b58f2362471e62fcf2) ) /* characters */

	ROM_REGION( 0x80000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "tad-2.7s",       0x00000, 0x80000, CRC(13ca7ae1) SHA1(b26bb4876a6518e3809e0fa4d442616508b3e7e8) ) /* tiles */
#if 0	/* same data, different layout */
	ROM_LOAD16_BYTE( "cabal_17.bin",   0x00000, 0x10000, CRC(3b6d2b09) )
	ROM_LOAD16_BYTE( "cabal_15.bin",   0x00001, 0x10000, CRC(1023319b) )
	ROM_LOAD16_BYTE( "cabal_16.bin",   0x20000, 0x10000, CRC(77bc7a60) )
	ROM_LOAD16_BYTE( "cabal_14.bin",   0x20001, 0x10000, CRC(420b0801) )
	ROM_LOAD16_BYTE( "cabal_18.bin",   0x40000, 0x10000, CRC(0bc50075) )
	ROM_LOAD16_BYTE( "cabal_12.bin",   0x40001, 0x10000, CRC(543fcb37) )
	ROM_LOAD16_BYTE( "cabal_19.bin",   0x60000, 0x10000, CRC(67e4fe47) )
	ROM_LOAD16_BYTE( "cabal_13.bin",   0x60001, 0x10000, CRC(d28d921e) )
#endif

	ROM_REGION( 0x80000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "tad-1.5e",       0x00000, 0x80000, CRC(8324a7fe) SHA1(aed4470df35ec18e65e35bddc9c217a5019fdcbf) ) /* sprites */
#if 0	/* same data, different layout */
	ROM_LOAD16_BYTE( "cabal_05.bin",   0x00000, 0x10000, CRC(4e49c28e) )
	ROM_LOAD16_BYTE( "cabal_04.bin",   0x00001, 0x10000, CRC(34d3cac8) )
	ROM_LOAD16_BYTE( "cabal_06.bin",   0x20000, 0x10000, CRC(6a0e739d) )
	ROM_LOAD16_BYTE( "cabal_03.bin",   0x20001, 0x10000, CRC(7065e840) )
	ROM_LOAD16_BYTE( "cabal_07.bin",   0x40000, 0x10000, CRC(581a50c1) )
	ROM_LOAD16_BYTE( "cabal_02.bin",   0x40001, 0x10000, CRC(0e1ec30e) )
	ROM_LOAD16_BYTE( "cabal_08.bin",   0x60000, 0x10000, CRC(702735c9) )
	ROM_LOAD16_BYTE( "cabal_01.bin",   0x60001, 0x10000, CRC(55c44764) )
#endif

	ROM_REGION( 0x20000, REGION_SOUND1, 0 )
	ROM_LOAD( "cabal_09.bin",   0x00000, 0x10000, CRC(4ffa7fe3) SHA1(381d8e765a7b94678fb3308965c748bbe9f8e247) ) /* Z80 code/adpcm data */
	ROM_LOAD( "cabal_10.bin",   0x10000, 0x10000, CRC(958789b6) SHA1(344c3ee8a1e272b56499e5c0415bb714aec0ddcf) ) /* Z80 code/adpcm data */
ROM_END



static DRIVER_INIT( cabal )
{
	seibu_sound_decrypt(REGION_CPU2,0x2000);
	seibu_adpcm_decrypt(REGION_SOUND1);
}


GAME( 1988, cabal,   0,     cabal,   cabal,    cabal,   ROT0, "Tad Corporation", "Cabal (World, Joystick version)" )
GAME( 1989, cabala,  cabal, cabal,   cabal,    cabal,   ROT0, "Tad Corporation (Alpha Trading license)", "Cabal (Alpha Trading)" ) /* korea?*/
GAME( 1988, cabalus, cabal, cabal,   cabalt,   cabal,   ROT0, "Tad (Fabtek license)", "Cabal (US set 1, Trackball version)" )
GAME( 1988, cabalus2,cabal, cabal,   cabalt,   cabal,   ROT0, "Tad (Fabtek license)", "Cabal (US set 2, Trackball version)" )
GAMEX(1988, cabalbl, cabal, cabalbl, cabalbl,  0,       ROT0, "bootleg", "Cabal (bootleg of Joystick version)", GAME_IMPERFECT_SOUND )
