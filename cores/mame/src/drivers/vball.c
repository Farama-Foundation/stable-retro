/**********************************************************************************************************************
 Championship VBall
 Driver by Paul "TBBle" Hampson

 TODO:
 Needs to be tilemapped. The background layer and sprite layer are identical to spdodgeb, except for the
  back-switched graphics roms and the size of the pallete banks.
 Someone needs to look at Naz's board, and see what PCM sound chips are present.
 And get whatever's in the dip package on Naz's board. (BG/FG Roms, I hope)
 I'd also love to know whether Naz's is a bootleg or is missing the story for a different reason (US release?)

 03/28/03 - Additions by Steve Ellenoff
 ---------------------------------------

 -Corrected background tiles (tiles are really 512x512 not 256x256 as previously setup)
 -Converted rendering to tilemap system
 -Implemented Scroll Y registers
 -Implemented X Line Scrolling (only seems to be used for displaying Hawaii and Airfield Map Screen)
 -Adjusted visible screen size to match more closely the real game
 -Added support for cocktail mode/flip screen
 -Adjusted Difficulty Dip settings based on some game testing I did
 -Confirmed the US version uses the oki6295 and does not display the story in attract mode like the JP version
 -Confirmed the Background graphics are contained in that unusual looking dip package on the US board,
  (need help figuring out the pinout so I can try and dump it)


 Remaining Issues:
 -1) IRQ & NMI code is totally guessed, and needs to be solved properly

Measurements from Guru (someone needs to rewrite INTERRUPT_GEN() in vidhrdw/vball.c):
6502 /IRQ = 1.720kHz
6202 /NMI = 58 Hz
VBlank = 58Hz


 -2) X Line Scrolling doesn't work 100% when Flip Screen Dip is set
 -3) 2 Player Version - Dips for difficulty don't seem to work or just need more testing

 -4) 2 Player Version - sound ROM is different and the adpmc chip is addressed differently
                        Changed it to use a rom that was dumped from original PCB (readme below),
                        this makes the non-working ROM not used - i don't know where it come from.



  U.S. Championship V'Ball (Japan)
  Technos, 1988

  PCB Layout
  ----------


  TA-0025-P1-02 (M6100357A BEACH VOLLEY 880050B04)
  |---------------------------------------------------------------------|
  |          YM3014  M6295             25J1-0.47   YM2151   3.579545MHz |
  |                1.056MHz  25J0-0.78   Z80       6116                 |
  |                                                                     |
  |                                                                     |
  |                                                                     |
  |                                                                     |
  |    6502 25J2-2-5.124 6116                                           |
  |                                                                     |
  |                    2016                                     12MHz   |
  |J                                                                    |
  |A                                             2016  2016             |
  |M                                                                    |
  |M                                                                    |
  |A                                                                    |
  |  DSW1                              6264     25J4-0.35  25J3-0.5     |
  |  DSW2                                                               |
  |       25J6-0.144                                                    |
  |       25J5-0.143 2016                                               |
  |                       -------------------                           |
  |25J7-0.160             |                 |                           |
  |                       | TOSHIBA  0615   |                           |
  |                  2016 |                 |                           |
  |                       | T5324   TRJ-101 |                           |
  |                       |                 |                           |
  |-----------------------|-----------------|---------------------------|


  Notes:
        6502 clock: 2.000MHz
         Z80 clock: 3.579545MHz
      YM2151 clock: 3.579545MHz
       M6295 clock: 1.056MHz, sample rate = 8kHz (i.e. 1056000/132)


  *********************************************************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "cpu/m6502/m6502.h"
#include "cpu/z80/z80.h"

/* from vidhrdw */
extern unsigned char *vb_attribram;
extern unsigned char *vb_spriteram;
extern unsigned char *vb_videoram;
extern unsigned char *vb_scrolly_lo;
extern int vb_scrollx_hi;
extern int vb_scrolly_hi;
extern int vb_scrollx_lo;
extern int vball_gfxset;

VIDEO_START( vb );
VIDEO_UPDATE( vb );
extern void vb_bgprombank_w(int bank);
extern void vb_spprombank_w(int bank);
extern WRITE_HANDLER( vb_attrib_w );
extern WRITE_HANDLER( vb_videoram_w );
extern void vb_mark_all_dirty(void);

INTERRUPT_GEN( vball_interrupt );

/* end of extern code & data */



/* bit 0 = bank switch
   bit 1 = ?
   bit 2 = ?
   bit 3 = ?
   bit 4 = ?
   bit 5 = graphics tile offset
   bit 6 = scroll y hi
   bit 7 = ?
*/
static WRITE_HANDLER( vb_bankswitch_w )
{
	unsigned char *RAM = memory_region(REGION_CPU1);
	cpu_setbank( 1,&RAM[ 0x10000 + ( 0x4000 * ( data & 1 ) ) ] );

	if (vball_gfxset != ((data  & 0x20) ^ 0x20)) {
		vball_gfxset = (data  & 0x20) ^ 0x20;
			vb_mark_all_dirty();
	}
	vb_scrolly_hi = (data & 0x40)<<2;
}

/* The sound system comes all but verbatim from Double Dragon */


WRITE_HANDLER( cpu_sound_command_w ) {
	soundlatch_w( offset, data );
	cpu_set_irq_line( 1, IRQ_LINE_NMI, PULSE_LINE );
}


/* bit 0 = flip screen
   bit 1 = scrollx hi
   bit 2 = bg prom bank
   bit 3 = bg prom bank
   bit 4 = bg prom bank
   bit 5 = sp prom bank
   bit 6 = sp prom bank
   bit 7 = sp prom bank
*/
WRITE_HANDLER( vb_scrollx_hi_w )
{
	flip_screen_set(~data&1);
	vb_scrollx_hi = (data & 0x02) << 7;
	vb_bgprombank_w((data >> 2)&0x07);
	vb_spprombank_w((data >> 5)&0x07);
	/*logerror("%04x: vb_scrollx_hi = %d\n",activecpu_get_previouspc(), vb_scrollx_hi);*/
}

static MEMORY_READ_START( readmem )
	{ 0x0000, 0x0fff, MRA_RAM },
	{ 0x1000, 0x1000, input_port_0_r },
	{ 0x1001, 0x1001, input_port_1_r },
	{ 0x1002, 0x1002, input_port_2_r },
	{ 0x1003, 0x1003, input_port_3_r },
	{ 0x1004, 0x1004, input_port_4_r },
	{ 0x1005, 0x1005, input_port_5_r },
	{ 0x1006, 0x1006, input_port_6_r },
	{ 0x4000, 0x7fff, MRA_BANK1 },
	{ 0x8000, 0xffff, MRA_ROM },
MEMORY_END

static MEMORY_READ_START( vball2pj_readmem )
	{ 0x0000, 0x0fff, MRA_RAM },
	{ 0x1000, 0x1000, input_port_0_r },
	{ 0x1001, 0x1001, input_port_1_r },
	{ 0x1002, 0x1002, input_port_2_r },
	{ 0x1003, 0x1003, input_port_3_r },
	{ 0x1004, 0x1004, input_port_4_r },
	{ 0x1005, 0x1005, MRA_RAM },		/*Strange, that these are read!*/
	{ 0x1006, 0x1006, MRA_RAM },		/*Strange, that these are read!*/
	{ 0x4000, 0x7fff, MRA_BANK1 },
	{ 0x8000, 0xffff, MRA_ROM },
MEMORY_END

WRITE_HANDLER(vb_scrollx_lo_w)
{
	vb_scrollx_lo = data;
	/*logerror("%04x: vb_scrollx_lo =%d\n",activecpu_get_previouspc(), vb_scrollx_lo);*/
}

/*Cheaters note: Scores are stored in ram @ 0x57-0x58 (though the space is used for other things between matches)*/
static MEMORY_WRITE_START( writemem )
	{ 0x0000, 0x07ff, MWA_RAM },
	{ 0x0800, 0x08ff, MWA_RAM, &spriteram, &spriteram_size },
	{ 0x1008, 0x1008, vb_scrollx_hi_w },
	{ 0x1009, 0x1009, vb_bankswitch_w },
	{ 0x100a, 0x100a, MWA_RAM },
	{ 0x100b, 0x100b, MWA_RAM },		/*Counts from 0 to 7 continuously*/
	{ 0x100c, 0x100c, vb_scrollx_lo_w },
	{ 0x100d, 0x100d, cpu_sound_command_w },
	{ 0x100e, 0x100e, MWA_RAM, &vb_scrolly_lo },
	{ 0x2000, 0x2fff, vb_videoram_w, &vb_videoram },
	{ 0x3000, 0x3fff, vb_attrib_w, &vb_attribram },
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
	{ 0x9800, 0x9803, OKIM6295_data_0_w },
MEMORY_END

#define COMMON_PORTS_BEFORE  PORT_START \
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 ) \
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 ) \
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 ) \
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 ) \
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 ) \
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 ) \
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN ) \
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 ) \
	PORT_START \
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 ) \
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 ) \
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 ) \
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 ) \
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 ) \
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 ) \
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN ) \
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START2 ) \
	PORT_START \
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 ) \
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 ) \
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE1 ) \
	PORT_BIT( 0x08, IP_ACTIVE_HIGH,IPT_VBLANK ) \
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNUSED ) \
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNUSED ) \
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED ) \
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED ) \

#define COMMON_PORTS_COINS  PORT_START \
	PORT_DIPNAME( 0x07, 0x07, DEF_STR( Coin_A )) \
	PORT_DIPSETTING(    0x00, DEF_STR( 4C_1C )) \
	PORT_DIPSETTING(    0x01, DEF_STR( 3C_1C )) \
	PORT_DIPSETTING(    0x02, DEF_STR( 2C_1C )) \
	PORT_DIPSETTING(    0x07, DEF_STR( 1C_1C )) \
	PORT_DIPSETTING(    0x06, DEF_STR( 1C_2C )) \
	PORT_DIPSETTING(    0x05, DEF_STR( 1C_3C )) \
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_4C )) \
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_5C )) \
	PORT_DIPNAME( 0x38, 0x38, DEF_STR( Coin_B )) \
	PORT_DIPSETTING(    0x00, DEF_STR( 4C_1C )) \
	PORT_DIPSETTING(    0x08, DEF_STR( 3C_1C )) \
	PORT_DIPSETTING(    0x10, DEF_STR( 2C_1C )) \
	PORT_DIPSETTING(    0x38, DEF_STR( 1C_1C )) \
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_2C )) \
	PORT_DIPSETTING(    0x28, DEF_STR( 1C_3C )) \
	PORT_DIPSETTING(    0x20, DEF_STR( 1C_4C )) \
	PORT_DIPSETTING(    0x18, DEF_STR( 1C_5C )) \
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Flip_Screen )) \
	PORT_DIPSETTING(    0x00, DEF_STR( Off )) \
	PORT_DIPSETTING(    0x40, DEF_STR( On )) \
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Demo_Sounds )) \
	PORT_DIPSETTING(    0x00, DEF_STR( Off )) \
	PORT_DIPSETTING(    0x80, DEF_STR( On )) \

INPUT_PORTS_START (vball)
	COMMON_PORTS_BEFORE
	/* The dipswitch instructions in naz's dump (vball) don't quite sync here) */
	/* Looks like the pins from the dips to the board were mixed up a little. */

	PORT_START
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Difficulty ))
/* I've adjusted these to what I think is correct from gameplay testing - SJE - 03/28/03*/
	PORT_DIPSETTING(    0x02, "Easy")
	PORT_DIPSETTING(    0x03, "Medium")
	PORT_DIPSETTING(    0x01, "Hard")
	PORT_DIPSETTING(    0x00, "Very Hard")
	PORT_DIPNAME( 0x0c, 0x00, "Single Player Game Time")
	PORT_DIPSETTING(    0x00, "1:15")
	PORT_DIPSETTING(    0x04, "1:30")
	PORT_DIPSETTING(    0x0c, "1:45")
	PORT_DIPSETTING(    0x08, "2:00")
	PORT_DIPNAME( 0x30, 0x00, "Start Buttons (4-player)")
	PORT_DIPSETTING(    0x00, "Normal")
	PORT_DIPSETTING(    0x20, "Button A")
	PORT_DIPSETTING(    0x10, "Button B")
	PORT_DIPSETTING(    0x30, "Normal")
	PORT_DIPNAME( 0x40, 0x40, "PL 1&4 (4-player)")
	PORT_DIPSETTING(    0x40, "Normal")
	PORT_DIPSETTING(    0x00, "Rot 90")
	PORT_DIPNAME( 0x80, 0x00, "Player Mode")
	PORT_DIPSETTING(    0x80, "2")
	PORT_DIPSETTING(    0x00, "4")

	COMMON_PORTS_COINS

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER3 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER3 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER3 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER3 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER3 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER3 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START3 )
	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER4 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER4 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER4 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER4 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER4 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER4 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START4 )
INPUT_PORTS_END

INPUT_PORTS_START (vball2pj)
	COMMON_PORTS_BEFORE

/* The 2-player roms have the game-time in the difficulty spot, and
   I've assumed vice-versa. (VS the instructions scanned in Naz's dump)
*/
	PORT_START
	PORT_DIPNAME( 0x03, 0x00, "Single Player Game Time")
	PORT_DIPSETTING(    0x00, "1:30")
	PORT_DIPSETTING(    0x01, "1:45")
	PORT_DIPSETTING(    0x03, "2:00")
	PORT_DIPSETTING(    0x02, "2:15")
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Difficulty ))
/* This ordering is assumed. Someone has to play it a lot and find out.*/
	PORT_DIPSETTING(    0x04, "Easy")
	PORT_DIPSETTING(    0x00, "Medium")
	PORT_DIPSETTING(    0x08, "Hard")
	PORT_DIPSETTING(    0x0c, "Very Hard")
	COMMON_PORTS_COINS
INPUT_PORTS_END


static struct GfxLayout charlayout =
{
	8,8,
	RGN_FRAC(1,1),
	4,
	{ 0, 2, 4, 6 },
	{ 0*8*8+1, 0*8*8+0, 1*8*8+1, 1*8*8+0, 2*8*8+1, 2*8*8+0, 3*8*8+1, 3*8*8+0 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	32*8
};

static struct GfxLayout spritelayout =
{
	16,16,
	RGN_FRAC(1,2),
	4,
	{ RGN_FRAC(1,2)+0, RGN_FRAC(1,2)+4, 0, 4 },
	{ 3, 2, 1, 0, 16*8+3, 16*8+2, 16*8+1, 16*8+0,
		  32*8+3, 32*8+2, 32*8+1, 32*8+0, 48*8+3, 48*8+2, 48*8+1, 48*8+0 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
		  8*8, 9*8, 10*8, 11*8, 12*8, 13*8, 14*8, 15*8 },
	64*8
};


static struct GfxDecodeInfo vb_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &charlayout,     0, 8 },	/* 8x8 chars */
	{ REGION_GFX2, 0, &spritelayout, 128, 8 },	/* 16x16 sprites */
	{ -1 } /* end of array */
};

static void vball_irq_handler(int irq)
{
	cpu_set_irq_line( 1, 0 , irq ? ASSERT_LINE : CLEAR_LINE );
}

static struct YM2151interface ym2151_interface =
{
	1,			/* 1 chip */
	3579545,	/* 3579545 Hz */
	{ YM3012_VOL(60,MIXER_PAN_LEFT,60,MIXER_PAN_RIGHT) },
	{ vball_irq_handler }
};

static struct OKIM6295interface okim6295_interface =
{
	1,              /* 1 chip */
	{ 1056000/132 },           /* frequency (Hz) */
	{ REGION_SOUND1 },  /* memory region */
	{ 100 }
};

static MACHINE_DRIVER_START( vball )

	/* basic machine hardware */
 	MDRV_CPU_ADD(M6502, 2000000)	/* 2 MHz - measured by guru but it makes the game far far too slow ?! */
	MDRV_CPU_MEMORY(readmem,writemem)
	MDRV_CPU_VBLANK_INT(vball_interrupt,32)	/* ??1 IRQ every 8 visible scanlines, plus NMI for vblank?? */

	MDRV_CPU_ADD(Z80, 3579545)
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)	/* 3.579545 MHz */
	MDRV_CPU_MEMORY(sound_readmem,sound_writemem)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(1*8, 31*8-1, 1*8, 31*8-1)	/* 240 x 240 */
	MDRV_GFXDECODE(vb_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(256)

	MDRV_VIDEO_START(vb)
	MDRV_VIDEO_UPDATE(vb)

	/* sound hardware */
	MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
	MDRV_SOUND_ADD(YM2151, ym2151_interface)
	MDRV_SOUND_ADD(OKIM6295, okim6295_interface)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( vball2pj )

	/* basic machine hardware */
 	MDRV_CPU_ADD(M6502, 2000000)	/* 2.0 MHz */
	MDRV_CPU_MEMORY(vball2pj_readmem,writemem)
	MDRV_CPU_VBLANK_INT(vball_interrupt,32)	/* ??1 IRQ every 8 visible scanlines, plus NMI for vblank?? */

	MDRV_CPU_ADD(Z80, 3579545)
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)	/* 3.579545 MHz */
	MDRV_CPU_MEMORY(sound_readmem,sound_writemem)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(1*8, 31*8-1, 1*8, 31*8-1)	/* 240 x 240 */
	MDRV_GFXDECODE(vb_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(256)

	MDRV_VIDEO_START(vb)
	MDRV_VIDEO_UPDATE(vb)

	/* sound hardware */
	MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
	MDRV_SOUND_ADD(YM2151, ym2151_interface)
	MDRV_SOUND_ADD(OKIM6295, okim6295_interface)
MACHINE_DRIVER_END


/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( vball )
	ROM_REGION( 0x18000, REGION_CPU1, 0 )	/* Main CPU: 64k for code */
	ROM_LOAD( "vball.124",  0x10000, 0x08000, CRC(be04c2b5) SHA1(40fed4ae272719e940f1796ef35420ab451ab7b6) )/* Bankswitched */
	ROM_CONTINUE(		0x08000, 0x08000 )		 /* Static code  */

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* region#2: music CPU, 64kb */
	ROM_LOAD( "vball.47",  0x00000, 0x8000,  CRC(10ca79ad) SHA1(aad4a09d6745ca0b5665cb00ff7a4e08ea434068) )

	/* These are from the bootleg; the original has the image data stored in a special dip rom */
	ROM_REGION(0x80000, REGION_GFX1, ROMREGION_DISPOSE )	 /* fg tiles */
	ROM_LOAD( "vball13.bin",  0x00000, 0x10000, CRC(f26df8e1) SHA1(72186c1430d07c7fd9211245b539f05a0660bebe) ) /* 0,1,2,3 */
	ROM_LOAD( "vball14.bin",  0x10000, 0x10000, CRC(c9798d0e) SHA1(ec156f6c7ecccaa216ce8076f75ad7627ee90945) ) /* 0,1,2,3 */
	ROM_LOAD( "vball15.bin",  0x20000, 0x10000, CRC(68e69c4b) SHA1(9870674c91cab7215ad8ed40eb82facdee478fde) ) /* 0,1,2,3 */
	ROM_LOAD( "vball16.bin",  0x30000, 0x10000, CRC(936457ba) SHA1(1662bbd777fcd33a298d192a3f06681809b9d049) ) /* 0,1,2,3 */
	ROM_LOAD( "vball09.bin",  0x40000, 0x10000, CRC(42874924) SHA1(a75eed7934e089f035000b7f35f6ba8dd96f1e98) ) /* 0,1,2,3 */
	ROM_LOAD( "vball10.bin",  0x50000, 0x10000, CRC(6cc676ee) SHA1(6e8c590946211baa9266b19b871f252829057696) ) /* 0,1,2,3 */
	ROM_LOAD( "vball11.bin",  0x60000, 0x10000, CRC(4754b303) SHA1(8630f077b542590ef1340a2f0a6b94086ff91c40) ) /* 0,1,2,3 */
	ROM_LOAD( "vball12.bin",  0x70000, 0x10000, CRC(21294a84) SHA1(b36ea9ddf6879443d3104241997fa0f916856528) ) /* 0,1,2,3 */

	ROM_REGION(0x40000, REGION_GFX2, ROMREGION_DISPOSE )	 /* sprites */
	ROM_LOAD( "vball.35",  0x00000, 0x20000, CRC(877826d8) SHA1(fd77298f9343051f66259dad9127f40afb95f385) ) /* 0,1,2,3 */
	ROM_LOAD( "vball.5",   0x20000, 0x20000, CRC(c6afb4fa) SHA1(6d7c966300ce5fb2094476b393434486965d62b4) ) /* 0,1,2,3 */

	ROM_REGION(0x20000, REGION_SOUND1, 0 ) /* Sound region#1: adpcm */
	ROM_LOAD( "vball.78a",  0x00000, 0x10000, CRC(f3e63b76) SHA1(da54d1d7d7d55b73e49991e4363bc6f46e0f70eb) )
	ROM_LOAD( "vball.78b",  0x10000, 0x10000, CRC(7ad9d338) SHA1(3e3c270fa69bda93b03f07a54145eb5e211ec8ba) )

	ROM_REGION(0x1000, REGION_PROMS, 0 )	/* color PROMs */
	ROM_LOAD_NIB_LOW ( "vball.44",   0x0000, 0x00800, CRC(a317240f) SHA1(bd57ad516f7a8ff774276fd26b02dd34659d41ad) )
	ROM_LOAD_NIB_HIGH( "vball.43",   0x0000, 0x00800, CRC(1ff70b4f) SHA1(a469baa0dda844ba307c09ddefb23f239cfe7b5f) )
	ROM_LOAD( "vball.160",  0x0800, 0x00800, CRC(2ffb68b3) SHA1(d560fdcd5e5c79d37e5b5bde22fbaf662fe89252) )
ROM_END

ROM_START( vball2pj )
	ROM_REGION( 0x18000, REGION_CPU1, 0 )	/* Main CPU */
	ROM_LOAD( "25j2-2-5.124",  0x10000, 0x08000,  CRC(432509c4) SHA1(6de50e21d279f4ac9674bc91990ba9535e80908c) )/* Bankswitched */
	ROM_CONTINUE(		  0x08000, 0x08000 )		 /* Static code  */

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* region#2: music CPU, 64kb */
	ROM_LOAD( "25j1-0.47",  0x00000, 0x8000,  CRC(10ca79ad) SHA1(aad4a09d6745ca0b5665cb00ff7a4e08ea434068) )
/*ROM_LOAD( "vball04.bin",  0x00000, 0x8000,  CRC(534dfbd9) SHA1(d0cb37caf94fa85da4ebdfe15e7a78109084bf91) )*/

   /* the original has the image data stored in a special ceramic embedded package made by Toshiba
     with part number 'TOSHIBA TRJ-101' (which has been dumped using a custom made adapter)
     there are a few bytes different between the bootleg and the original (the original is correct though!) */
    ROM_REGION(0x80000, REGION_GFX1, ROMREGION_DISPOSE ) /* fg tiles */
    ROM_LOAD( "trj-101.96",  0x00000, 0x80000, CRC(f343eee4) SHA1(1ce95285631f7ec91fe3f6c3d62b13f565d3816a) )

	ROM_REGION(0x40000, REGION_GFX2, ROMREGION_DISPOSE )	 /* sprites */
	ROM_LOAD( "25j4-0.35",  0x00000, 0x20000, CRC(877826d8) SHA1(fd77298f9343051f66259dad9127f40afb95f385) ) /* 0,1,2,3 */
	ROM_LOAD( "25j3-0.5",   0x20000, 0x20000, CRC(c6afb4fa) SHA1(6d7c966300ce5fb2094476b393434486965d62b4) ) /* 0,1,2,3 */

	ROM_REGION(0x20000, REGION_SOUND1, 0 ) /* Sound region#1: adpcm */
	ROM_LOAD( "25j0-0.78",  0x00000, 0x20000, CRC(8e04bdbf) SHA1(baafc5033c9442b83cb332c2c453c13117b31a3b) )

	ROM_REGION(0x1000, REGION_PROMS, 0 )	/* color PROMs */
	ROM_LOAD_NIB_LOW ( "vball.44",   0x0000, 0x00800, CRC(a317240f) SHA1(bd57ad516f7a8ff774276fd26b02dd34659d41ad) )
	ROM_LOAD_NIB_HIGH( "vball.43",   0x0000, 0x00800, CRC(1ff70b4f) SHA1(a469baa0dda844ba307c09ddefb23f239cfe7b5f) )
	ROM_LOAD( "vball.160",  0x0800, 0x00800, CRC(2ffb68b3) SHA1(d560fdcd5e5c79d37e5b5bde22fbaf662fe89252) )
ROM_END


GAME( 1988, vball,    0,     vball,    vball,    0, ROT0, "Technos", "U.S. Championship V'ball (set 1)" )
GAME( 1988, vball2pj, vball, vball2pj, vball2pj, 0, ROT0, "Technos", "U.S. Championship V'ball (Japan)" )
