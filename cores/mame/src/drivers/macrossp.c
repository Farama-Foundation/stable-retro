/*** DRIVER INFO **************************************************************

Macross Plus                        (c)1996 Banpresto
Quiz Bisyoujo Senshi Sailor Moon    (c)1997 Banpresto
Driver by David Haywood

TODO:
- macrossp: tilemap zoom is wrong, see title screen (misplaced) and level 2 boss
  (background scrolls faster than sprites)
- should use VIDEO_RGB_DIRECT for alpha blending to work, but tilemap_draw_roz()
  doesn't support it.
- lines of garbage tiles in a couple of places in macrossp (e.g. level 2). Bad ROM?
- Sprite Zoom on quizmoon title screen isn't right
- Tilemap zoom effect on macrossp title screen and probably other places
- Priorities (Sprites & Backgrounds) - see quizmoon attract mode
- sprite/tilemap priorities might not be 100% correct
- Sound
- palette fade on macrossp Banpresto logo screen (and black on some screen
  transitions). quizmoon doesn't use that register.
- All Other Unused Reads / Writes
- Correct DSWs / Ports
- Clean Up

 Notes:

 Whats the BIOS rom? should it be in the 68020 map, its different between
 games.

 68020 interrupts
 lev 1 : 0x64 : 0000 084c - unknown..
 lev 2 : 0x68 : 0000 0882 - unknown..
 lev 3 : 0x6c : 0000 08b0 - vblank?
 lev 4 : 0x70 : 001f 002a - x
 lev 5 : 0x74 : 001f 002a - x
 lev 6 : 0x78 : 001f 002a - x
 lev 7 : 0x7c : 001f 002a - x

******************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

/*** README INFO **************************************************************

--- ROMSET: macrossp ---

Macross Plus
Banpresto Co., Ltd., 1996

PCB: BP964
CPU: MC68EC020FG25
SND: TMP68HC000N-16, OTTOR2 ES5506000102
OSC: 50.000MHz, 32.000MHz, 27.000MHz
RAM: TC55257CFL-85 x 8 (28 PIN TSOP), MCM62068AEJ25 x 12 (28 PIN SSOP), TC55328AJ-15 x 6 (28 PIN SSOP)
GFX: IKM-AA004   (208 PIN PQFP)
     IKM-AA0062  (208 PIN PQFP)
     IKM-AA005   x 2 (208 PIN PQFP)
DIPS: 2x 8-Position


(Typed directly from original sheet supplied with PCB)

SW1:

			1	2	3	4	5	6	7	8
-------------------------------------------------------------------------------------
COIN A	1COIN 1PLAY	OFF	OFF	OFF	OFF
	1C2P		ON	OFF	OFF	OFF
	1C3P		OFF	ON	OFF	OFF
	1C4P		ON	ON	OFF	OFF
	1C5P		OFF	OFF	ON	OFF
	1C6P		ON	OFF	ON	OFF
	1C7P		OFF	ON	ON	OFF
	2C1P		ON	ON	ON	OFF
	2C3P		OFF	OFF	OFF	ON
	2C5P		ON	OFF	OFF	ON
	3C1P		OFF	ON	OFF	ON
	3C2P		ON	ON	OFF	ON
	3C4P		OFF	OFF	ON	ON
	4C1P		ON	OFF	ON	ON
	4C3P		OFF	ON	ON	ON
	FREEPLAY	ON	ON	ON	ON

COIN B	1COIN 1PLAY					OFF	OFF	OFF	OFF
	1C2P						ON	OFF	OFF	OFF
	1C3P						OFF	ON	OFF	OFF
	1C4P						ON	ON	OFF	OFF
	1C5P						OFF	OFF	ON	OFF
	1C6P						ON	OFF	ON	OFF
	1C7P						OFF	ON	ON	OFF
	2C1P						ON	ON	ON	OFF
	2C3P						OFF	OFF	OFF	ON
	2C5P						ON	OFF	OFF	ON
	3C1P						OFF	ON	OFF	ON
	3C2P						ON	ON	OFF	ON
	3C4P						OFF	OFF	ON	ON
	4C1P						ON	OFF	ON	ON
	4C3P						OFF	ON	ON	ON
	5C3P						ON	ON	ON	ON

FACTORY SETTING = ALL OFF


SW2

			1	2	3	4	5	6	7	8
--------------------------------------------------------------------------------------
DIFFICULTY	NORMAL	OFF	OFF
		EASY	ON	OFF
		HARD	OFF	ON
		HARDEST	ON	ON

PLAYERS		3			OFF	OFF
                4                       ON	OFF
                5                       OFF	ON
                2                       ON	ON

DEMO SOUND	YES					OFF
		NO					ON

SCREEN F/F	PLAY						OFF
		MUTE						ON

NOT USED								OFF

MODE		GAME								OFF
		TEST								ON


FACTORY SETTING = ALL OFF


ROMs:  (Filename = ROM label, extension also on ROM label)

TOP ROM PCB
-----------
BP964A-C.U1	\
BP964A-C.U2	 |
BP964A-C.U3	 |
BP964A-C.U4	 > 27C040
BP964A.U19	 |
BP964A.U20	 |
BP964A.U21	/

BP964A.U9	\
BP964A.U10	 |
BP964A.U11	 |
BP964A.U12	 |
BP964A.U13	 |
BP964A.U14	 > 32M (44 pin SOP - surface mounted)
BP964A.U15	 |
BP964A.U16	 |
BP964A.U17	 |
BP964A.U18	 |
BP964A.U24	/

ROMs:  (Filename = ROM Label)

MOTHERBOARD PCB
---------------
BP964A.U49	27C010

--- ROMSET: quizmoon ---

Quiz Bisyoujo Senshi Sailor Moon - Chiryoku Tairyoku Toki no Un -
(c)1997 Banpresto / Gazelle
BP965A

-----------
Motherboard
-----------
CPU  : MC68EC020FG25
Sound: TMP68HC000N-16, ENSONIQ OTTO R2 (ES5506)
OSC  : 50.000MHz (X1), 32.000MHz (X2), 27.000MHz (X3)

ROMs:
u49.bin - (ST 27c1001)

GALs (16V8B, not dumped):
u009.bin
u200.bin

Custom chips:
IKM-AA004 1633JF8433 JAPAN 9523YAA (U62, 208pin QFP)
IKM-AA005 1670F1541 JAPAN 9525EAI (U47&48, 208pin QFP)
IKM-AA006 1633JF8432 JAPAN 9525YAA (U31, 208pin QFP)

--------------
Mask ROM board
--------------
u5.bin - Main programs (TI 27c040)
u6.bin |
u7.bin |
u8.bin |
u1.bin | (ST 27c1001)
u2.bin |
u3.bin |
u4.bin /

u09.bin - Graphics (uPD23C32000GX)
u10.bin |
u11.bin |
u12.bin |
u13.bin |
u15.bin |
u17.bin / (uPD23C16000GX)

u20.bin - Sound programs (ST 27c1001)
u21.bin /

u24.bin - Samples (uPD23C32000GX)
u25.bin |
u26.bin |
u27.bin /

******************************************************************************/

extern data32_t *macrossp_scra_videoram, *macrossp_scra_videoregs;
extern data32_t *macrossp_scrb_videoram, *macrossp_scrb_videoregs;
extern data32_t *macrossp_scrc_videoram, *macrossp_scrc_videoregs;
extern data32_t *macrossp_text_videoram, *macrossp_text_videoregs;
extern data32_t *macrossp_spriteram;

static data32_t *macrossp_mainram;

/* in vidhrdw */
WRITE32_HANDLER( macrossp_scra_videoram_w );
WRITE32_HANDLER( macrossp_scrb_videoram_w );
WRITE32_HANDLER( macrossp_scrc_videoram_w );
WRITE32_HANDLER( macrossp_text_videoram_w );
VIDEO_START(macrossp);
VIDEO_UPDATE(macrossp);
VIDEO_EOF(macrossp);

/*** VARIOUS READ / WRITE HANDLERS *******************************************/

static READ32_HANDLER ( macrossp_ports1_r )
{
	return ((readinputport(0) << 16) |  (readinputport(1) << 0));
}

static READ32_HANDLER ( macrossp_ports2_r )
{
	return ((readinputport(2) << 16) |  (readinputport(3) << 0));
}

static WRITE32_HANDLER( paletteram32_macrossp_w )
{
	int r,g,b;
	COMBINE_DATA(&paletteram32[offset]);

	b = ((paletteram32[offset] & 0x0000ff00) >>8);
	g = ((paletteram32[offset] & 0x00ff0000) >>16);
	r = ((paletteram32[offset] & 0xff000000) >>24);

	palette_set_color(offset,r,g,b);
}


static int sndpending;

static READ32_HANDLER ( macrossp_soundstatus_r )
{
	static int toggle;

/*	log_cb(RETRO_LOG_DEBUG, LOGPRE "%08x read soundstatus\n",activecpu_get_pc());*/

	/* bit 1 is sound status */
	/* bit 0 unknown - it is expected to toggle, vblank? */

	toggle ^= 1;

	if (Machine->sample_rate == 0) return (rand()&2) | toggle;

	return (sndpending << 1) | toggle;
}

static WRITE32_HANDLER( macrossp_soundcmd_w )
{
	if (ACCESSING_MSW32)
	{
		/*logerror("%08x write soundcmd %08x (%08x)\n",activecpu_get_pc(),data,mem_mask);*/
		soundlatch_word_w(0,data >> 16,0);
		sndpending = 1;
		cpu_set_irq_line(1,2,HOLD_LINE);
		/* spin for a while to let the sound CPU read the command */
		cpu_spinuntil_time(TIME_IN_USEC(50));
	}
}

static READ16_HANDLER( macrossp_soundcmd_r )
{
/*	log_cb(RETRO_LOG_DEBUG, LOGPRE "%06x read soundcmd\n",activecpu_get_pc());*/
	sndpending = 0;
	return soundlatch_word_r(offset,mem_mask);
}


/*** MEMORY MAPS *************************************************************/

static MEMORY_READ32_START( readmem )
	{ 0x000000, 0x3fffff, MRA32_ROM },

	{ 0x800000, 0x802fff, MRA32_RAM },

	{ 0x900000, 0x903fff, MRA32_RAM },
	{ 0x908000, 0x90bfff, MRA32_RAM },
	{ 0x910000, 0x913fff, MRA32_RAM },
	{ 0x918000, 0x91bfff, MRA32_RAM },

	{ 0xa00000, 0xa03fff, MRA32_RAM },

	{ 0xb00000, 0xb00003, macrossp_ports1_r },
	{ 0xb00004, 0xb00007, macrossp_soundstatus_r },
	{ 0xb0000c, 0xb0000f, macrossp_ports2_r },

	{ 0xf00000, 0xf1ffff, MRA32_RAM },

/*	{ 0xfe0000, 0xfe0003, MRA32_NOP },*/
MEMORY_END

static MEMORY_WRITE32_START( writemem )
	{ 0x000000, 0x3fffff, MWA32_ROM },
	{ 0x800000, 0x802fff, MWA32_RAM, &macrossp_spriteram, &spriteram_size },

	/* SCR A Layer */
	{ 0x900000, 0x903fff, macrossp_scra_videoram_w, &macrossp_scra_videoram },
	{ 0x904200, 0x9043ff, MWA32_RAM }, /* W/O? */
	{ 0x905000, 0x90500b, MWA32_RAM, &macrossp_scra_videoregs }, /* W/O? */
	/* SCR B Layer */
	{ 0x908000, 0x90bfff, macrossp_scrb_videoram_w, &macrossp_scrb_videoram },
	{ 0x90c200, 0x90c3ff, MWA32_RAM }, /* W/O? */
	{ 0x90d000, 0x90d00b, MWA32_RAM, &macrossp_scrb_videoregs }, /* W/O? */
	/* SCR C Layer */
	{ 0x910000, 0x913fff, macrossp_scrc_videoram_w, &macrossp_scrc_videoram },
	{ 0x914200, 0x9143ff, MWA32_RAM }, /* W/O? */
	{ 0x915000, 0x91500b, MWA32_RAM, &macrossp_scrc_videoregs }, /* W/O? */
	/* Text Layer */
	{ 0x918000, 0x91bfff, macrossp_text_videoram_w, &macrossp_text_videoram  },
	{ 0x91c200, 0x91c3ff, MWA32_RAM }, /* W/O? */
	{ 0x91d000, 0x91d00b, MWA32_RAM, &macrossp_text_videoregs }, /* W/O? */

	{ 0xa00000, 0xa03fff, paletteram32_macrossp_w, &paletteram32 },

	{ 0xb00004, 0xb00007, MWA32_NOP },	/* ????*/
	{ 0xb00008, 0xb0000b, MWA32_NOP },	/* ????*/
/*	{ 0xb0000c, 0xb0000f, MWA32_NOP },*/
	{ 0xb00010, 0xb00013, MWA32_RAM },	/* macrossp palette fade*/
/*	{ 0xb00020, 0xb00023, MWA32_NOP },*/

	{ 0xc00000, 0xc00003, macrossp_soundcmd_w },

	{ 0xf00000, 0xf1ffff, MWA32_RAM, &macrossp_mainram }, /* Main Ram */

/*	{ 0xfe0000, 0xfe0003, MWA32_NOP },*/
MEMORY_END


static MEMORY_READ16_START( sound_readmem )
	{ 0x000000, 0x0fffff, MRA16_ROM },
	{ 0x200000, 0x207fff, MRA16_RAM },
	{ 0x400000, 0x40007f, ES5506_data_0_word_r },
	{ 0x600000, 0x600001, macrossp_soundcmd_r },
MEMORY_END

static MEMORY_WRITE16_START( sound_writemem )
	{ 0x000000, 0x0fffff, MWA16_ROM },
	{ 0x200000, 0x207fff, MWA16_RAM },
	{ 0x400000, 0x40007f, ES5506_data_0_word_w },
MEMORY_END

/*** INPUT PORTS *************************************************************/

INPUT_PORTS_START( macrossp )
	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER1 )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER1 )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER2 )

	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_DIPNAME( 0x0010, 0x0010, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0010, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_DIPNAME( 0x0040, 0x0040, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0040, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0080, 0x0080, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0080, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0100, 0x0100, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0100, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0200, 0x0200, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0200, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0400, 0x0400, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0400, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0800, 0x0800, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0800, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x1000, 0x1000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x1000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x2000, 0x2000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x2000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x4000, 0x4000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x4000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x8000, 0x8000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x8000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )

	PORT_START	/* DSW */
	PORT_DIPNAME( 0x000f, 0x000f, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(      0x0005, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0008, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0004, DEF_STR( 3C_2C ) )
	PORT_DIPSETTING(      0x0001, DEF_STR( 4C_3C ) )
	PORT_DIPSETTING(      0x000f, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0003, DEF_STR( 3C_4C ) )
	PORT_DIPSETTING(      0x0007, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(      0x000e, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x0006, DEF_STR( 2C_5C ) )
	PORT_DIPSETTING(      0x000d, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x000c, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(      0x000b, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(      0x000a, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(      0x0009, "1 Coins/7 Credits" )
	PORT_DIPSETTING(      0x0000, DEF_STR( Free_Play) )
	PORT_DIPNAME( 0x00f0, 0x00f0, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(      0x0050, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0080, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 5C_3C ) )
	PORT_DIPSETTING(      0x0040, DEF_STR( 3C_2C ) )
	PORT_DIPSETTING(      0x0010, DEF_STR( 4C_3C ) )
	PORT_DIPSETTING(      0x00f0, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0030, DEF_STR( 3C_4C ) )
	PORT_DIPSETTING(      0x0070, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(      0x00e0, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x0060, DEF_STR( 2C_5C ) )
	PORT_DIPSETTING(      0x00d0, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x00c0, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(      0x00b0, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(      0x00a0, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(      0x0090, "1 Coins/7 Credits" )
	PORT_DIPNAME( 0x0300, 0x0300, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(      0x0200, "Easy" )
	PORT_DIPSETTING(      0x0300, "Normal" )
	PORT_DIPSETTING(      0x0100, "Hard" )
	PORT_DIPSETTING(      0x0000, "Hardest" )
	PORT_DIPNAME( 0x0c00, 0x0c00, DEF_STR( Lives ) )
	PORT_DIPSETTING(      0x0000, "2" )
	PORT_DIPSETTING(      0x0c00, "3" )
	PORT_DIPSETTING(      0x0800, "4" )
	PORT_DIPSETTING(      0x0400, "5" )
	PORT_DIPNAME( 0x1000, 0x1000, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x1000, DEF_STR( On ) )
	PORT_DIPNAME( 0x2000, 0x2000, DEF_STR( Unknown ) )	/* See above for manual listing....*/
	PORT_DIPSETTING(      0x2000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x4000, 0x4000, "Language" )	/* See title page for difference :-)*/
	PORT_DIPSETTING(      0x4000, "Japanese" )
	PORT_DIPSETTING(      0x0000, "English" )
	PORT_SERVICE( 0x8000, IP_ACTIVE_LOW )

	PORT_START
	PORT_DIPNAME( 0x0001, 0x0001, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0001, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0002, 0x0002, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0004, 0x0004, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0004, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0008, 0x0008, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0008, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0010, 0x0010, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0010, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0020, 0x0020, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0040, 0x0040, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0040, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0080, 0x0080, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0080, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0100, 0x0100, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0100, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0200, 0x0200, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0200, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0400, 0x0400, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0400, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0800, 0x0800, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0800, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x1000, 0x1000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x1000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x2000, 0x2000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x2000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x4000, 0x4000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x4000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x8000, 0x8000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x8000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )

INPUT_PORTS_END


INPUT_PORTS_START( quizmoon )
	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER1 )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER1 )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER2 )

	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_DIPNAME( 0x0010, 0x0010, "Test" )
	PORT_DIPSETTING(      0x0010, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_DIPNAME( 0x0040, 0x0040, "Tilt" )
	PORT_DIPSETTING(      0x0040, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0080, 0x0080, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0080, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0100, 0x0100, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0100, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0200, 0x0200, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0200, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0400, 0x0400, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0400, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0800, 0x0800, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0800, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x1000, 0x1000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x1000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x2000, 0x2000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x2000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x4000, 0x4000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x4000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x8000, 0x8000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x8000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )

	PORT_START	/* DSW */
	PORT_DIPNAME( 0x000f, 0x000f, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(      0x0005, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0008, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0004, DEF_STR( 3C_2C ) )
	PORT_DIPSETTING(      0x0001, DEF_STR( 4C_3C ) )
	PORT_DIPSETTING(      0x000f, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0003, DEF_STR( 3C_4C ) )
	PORT_DIPSETTING(      0x0007, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(      0x000e, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x0006, DEF_STR( 2C_5C ) )
	PORT_DIPSETTING(      0x000d, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x000c, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(      0x000b, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(      0x000a, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(      0x0009, "1 Coins/7 Credits" )
	PORT_DIPSETTING(      0x0000, DEF_STR( Free_Play) )
	PORT_DIPNAME( 0x00f0, 0x00f0, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(      0x0050, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0080, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 5C_3C ) )
	PORT_DIPSETTING(      0x0040, DEF_STR( 3C_2C ) )
	PORT_DIPSETTING(      0x0010, DEF_STR( 4C_3C ) )
	PORT_DIPSETTING(      0x00f0, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0030, DEF_STR( 3C_4C ) )
	PORT_DIPSETTING(      0x0070, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(      0x00e0, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x0060, DEF_STR( 2C_5C ) )
	PORT_DIPSETTING(      0x00d0, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x00c0, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(      0x00b0, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(      0x00a0, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(      0x0090, "1 Coins/7 Credits" )
	PORT_DIPNAME( 0x0300, 0x0300, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(      0x0200, "Easy" )
	PORT_DIPSETTING(      0x0300, "Normal" )
	PORT_DIPSETTING(      0x0100, "Hard" )
	PORT_DIPSETTING(      0x0000, "Hardest" )
	PORT_DIPNAME( 0x0c00, 0x0c00, DEF_STR( Lives ) )
	PORT_DIPSETTING(      0x0000, "2" )
	PORT_DIPSETTING(      0x0c00, "3" )
	PORT_DIPSETTING(      0x0800, "4" )
	PORT_DIPSETTING(      0x0400, "5" )
	PORT_DIPNAME( 0x1000, 0x1000, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x1000, DEF_STR( On ) )
	PORT_DIPNAME( 0x2000, 0x2000, DEF_STR( Flip_Screen ) )	/* See in test mode ok but not working.... */
	PORT_DIPSETTING(      0x2000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x4000, 0x4000, DEF_STR( Unused ) )
	PORT_DIPSETTING(      0x4000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_SERVICE( 0x8000, IP_ACTIVE_LOW )

	PORT_START
	PORT_DIPNAME( 0x0001, 0x0001, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0001, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0002, 0x0002, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0004, 0x0004, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0004, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0008, 0x0008, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0008, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0010, 0x0010, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0010, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0020, 0x0020, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0040, 0x0040, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0040, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0080, 0x0080, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0080, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0100, 0x0100, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0100, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0200, 0x0200, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0200, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0400, 0x0400, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0400, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0800, 0x0800, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0800, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x1000, 0x1000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x1000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x2000, 0x2000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x2000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x4000, 0x4000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x4000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x8000, 0x8000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x8000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )

INPUT_PORTS_END

/*** GFX DECODE **************************************************************/

static struct GfxLayout macrossp_char16x16x4layout =
{
	16,16,
	RGN_FRAC(1,1),
	4,
	{ 0,1,2,3 },
	{ 0, 4, 8, 12, 16, 20, 24, 28, 32+0,32+4,32+8,32+12,32+16,32+20,32+24,32+28 },
	{ 0*64, 1*64, 2*64, 3*64, 4*64, 5*64, 6*64, 7*64,
	  8*64, 9*64, 10*64,11*64,12*64,13*64,14*64,15*64},
	16*64
};

static struct GfxLayout macrossp_char16x16x8layout =
{
	16,16,
	RGN_FRAC(1,1),
	8,
	{ 0,1,2,3,4,5,6,7 },
	{ 0, 8, 16, 24, 32, 40, 48, 56, 64+0,64+8,64+16,64+24,64+32,64+40,64+48,64+56 },
	{ 0*128, 1*128, 2*128, 3*128, 4*128, 5*128, 6*128, 7*128,
	  8*128, 9*128, 10*128,11*128,12*128,13*128,14*128,15*128},
	16*128
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &macrossp_char16x16x8layout,   0x000, 0x20 },	/* 8bpp but 6bpp granularity */
	{ REGION_GFX2, 0, &macrossp_char16x16x8layout,   0x800, 0x20 },	/* 8bpp but 6bpp granularity */
	{ REGION_GFX3, 0, &macrossp_char16x16x8layout,   0x800, 0x20 },	/* 8bpp but 6bpp granularity */
	{ REGION_GFX4, 0, &macrossp_char16x16x8layout,   0x800, 0x20 },	/* 8bpp but 6bpp granularity */
	{ REGION_GFX5, 0, &macrossp_char16x16x4layout,   0x800, 0x80 },
	{ -1 } /* end of array */
};

/*** MACHINE DRIVER **********************************************************/

static void irqhandler(int irq)
{
	log_cb(RETRO_LOG_DEBUG, LOGPRE "ES5506 irq %d\n",irq);

	/* IRQ lines 1 & 4 on the sound 68000 are definitely triggered by the ES5506,
	but I haven't noticed the ES5506 ever assert the line - maybe only used when developing the game? */
/*	cpu_set_irq_line(1,1,irq ? ASSERT_LINE : CLEAR_LINE);*/
}

static struct ES5506interface es5506_interface =
{
	1,
	{ 16000000 },
	{ REGION_SOUND1 },
	{ REGION_SOUND2 },
	{ REGION_SOUND3 },
	{ REGION_SOUND4 },
	{ YM3012_VOL(100,MIXER_PAN_LEFT,100,MIXER_PAN_RIGHT) },
	{ irqhandler },
	{ 0 }
};


static MACHINE_DRIVER_START( macrossp )
	/* basic machine hardware */
	MDRV_CPU_ADD(M68EC020, 50000000/2)	/* 25 MHz */
	MDRV_CPU_MEMORY(readmem,writemem)
	MDRV_CPU_VBLANK_INT(irq3_line_hold,1) /* there are others ...*/

	MDRV_CPU_ADD(M68000, 32000000/2)	/* 16 MHz */
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
	MDRV_CPU_MEMORY(sound_readmem,sound_writemem)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	/* video hardware */
/*	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_NEEDS_6BITS_PER_GUN | VIDEO_RGB_DIRECT)  // only needs 6 bits because of alpha blending /*/
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_NEEDS_6BITS_PER_GUN) /* only needs 6 bits because of alpha blending */
	MDRV_SCREEN_SIZE(32*16, 16*16)
	MDRV_VISIBLE_AREA(0*16, 24*16-1, 0*16, 15*16-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(0x1000)

	MDRV_VIDEO_START(macrossp)
	MDRV_VIDEO_EOF(macrossp)
	MDRV_VIDEO_UPDATE(macrossp)

	/* sound hardware */
	MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
	MDRV_SOUND_ADD(ES5506, es5506_interface)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( quizmoon )
	MDRV_IMPORT_FROM(macrossp)

	MDRV_VISIBLE_AREA(0, 24*16-1, 0*8, 14*16-1)
MACHINE_DRIVER_END



/*** ROM LOADING *************************************************************/

ROM_START( macrossp )
	ROM_REGION( 0x200000, REGION_CPU1, 0 )
	ROM_LOAD32_BYTE( "bp964a-c.u1", 0x000003, 0x080000, CRC(e37904e4) SHA1(e51e446f3699632c53c40e4c16d0ffb76ed1123f) )
	ROM_LOAD32_BYTE( "bp964a-c.u2", 0x000002, 0x080000, CRC(86d0ca6a) SHA1(8cc1b4a83cbba8b07e1343c5c20e2590d3ce471a) )
	ROM_LOAD32_BYTE( "bp964a-c.u3", 0x000001, 0x080000, CRC(fb895a7b) SHA1(547e5d3d43e503a15573748ab49a44e5569db1d7) )
	ROM_LOAD32_BYTE( "bp964a-c.u4", 0x000000, 0x080000, CRC(8c8b966c) SHA1(881fa2b7aefc3ea916924b715d2cd0ceaffe2d92) )

	ROM_REGION( 0x100000, REGION_CPU2, 0 )
	ROM_LOAD16_BYTE( "bp964a.u20", 0x000001, 0x080000, CRC(12960cbb) SHA1(7182c4b36849a5d34ddf388bf5f4485ed360fe84) )
	ROM_LOAD16_BYTE( "bp964a.u21", 0x000000, 0x080000, CRC(87bdd2fc) SHA1(c33f087ebca6e98db195404788ca8e0cc6663622) )

	ROM_REGION( 0x20000, REGION_USER1, 0 )
	ROM_LOAD( "bp964a.u49", 0x000000, 0x020000, CRC(ad203f76) SHA1(3eb86eeeb020349dfd88ebc8b4fc9579d1cc50fb) )  /* 'BIOS'*/

	ROM_REGION( 0x1000000, REGION_GFX1, 0 ) /* sprites - 16x16x8 */
	ROM_LOAD32_BYTE( "bp964a.u9",  0x000003, 0x400000, CRC(bd51a70d) SHA1(3447ae9d368e4e33df2d4e2848b4fd5aa0fc6840) )
	ROM_LOAD32_BYTE( "bp964a.u10", 0x000002, 0x400000, CRC(ab84bba7) SHA1(d30876b2e45c4b78cda27d3c648100e60f739d9c) )
	ROM_LOAD32_BYTE( "bp964a.u11", 0x000001, 0x400000, CRC(b9ae1d0b) SHA1(bc541a8bd622c99cf5065b3a793f0b5f6420ac64) )
	ROM_LOAD32_BYTE( "bp964a.u12", 0x000000, 0x400000, CRC(8dda1052) SHA1(c374335e98859ae98ac392a7cdb44f15b4e1c23a) )

	ROM_REGION( 0x800000, REGION_GFX2, 0 ) /* backgrounds - 16x16x8 */
	ROM_LOAD( "bp964a.u13", 0x000000, 0x400000, CRC(f4d3c5bf) SHA1(82522d276a6d49148da8a4fb11846a039429bcf8) )
	ROM_LOAD( "bp964a.u14", 0x400000, 0x400000, CRC(4f2dd1b2) SHA1(30a2c9fb26bca8bb27fbc5637878f99e7f6ad8f4) )

	ROM_REGION( 0x800000, REGION_GFX3, 0 ) /* backgrounds - 16x16x8 */
	ROM_LOAD( "bp964a.u15", 0x000000, 0x400000, CRC(5b97a870) SHA1(16f3921649b28ecb6d628871214f972333bbeca4) )
	ROM_LOAD( "bp964a.u16", 0x400000, 0x400000, CRC(c8a0cd64) SHA1(2a30a4d4ec3f94631783eb08c62003b116bb2ee3) )

	ROM_REGION( 0x800000, REGION_GFX4, 0 ) /* backgrounds - 16x16x8 */
	ROM_LOAD( "bp964a.u17", 0x000000, 0x400000, CRC(f2470876) SHA1(e683208432f71f3cc19ced245fa5b8a82466d19b) )
	ROM_LOAD( "bp964a.u18", 0x400000, 0x400000, CRC(52ef21f3) SHA1(08fb1969ad0ffd0c5bf11d3d5448a26112d562b0) )

	ROM_REGION( 0x400000, REGION_GFX5, 0 ) /* foreground - 16x16x4 */
	ROM_LOAD( "bp964a.u19", 0x000000, 0x080000, CRC(19c7acd9) SHA1(b7631e74f359c5570c44addf46c3e96c80adc6c3) )

	ROM_REGION16_BE( 0x800000, REGION_SOUND1, ROMREGION_ERASEFF )
	ROM_LOAD16_BYTE( "bp964a.u24", 0x000000, 0x400000, CRC(93f90336) SHA1(75daa2f8cedc732cf5ef98254f61748c94b94aea) )

	ROM_REGION16_BE( 0x400000, REGION_SOUND2, 0 )
	/* Filled in below in init routine */
ROM_END

ROM_START( quizmoon )
	ROM_REGION( 0x400000, REGION_CPU1, 0 )
	ROM_LOAD32_BYTE( "u1.bin",  0x000003, 0x020000, CRC(ea404553) SHA1(123bb8e399a5b54c43f4eb41d3e5f52c6947900f) )
	ROM_LOAD32_BYTE( "u2.bin",  0x000002, 0x020000, CRC(024eedff) SHA1(cbaa2b71980a2686e582331616dc36e34ecd9e67) )
	ROM_LOAD32_BYTE( "u3.bin",  0x000001, 0x020000, CRC(545b1d17) SHA1(f1b15260942482857c48b574ada1e2a3b728f395) )
	ROM_LOAD32_BYTE( "u4.bin",  0x000000, 0x020000, CRC(60b3d18c) SHA1(230342a084938fdbd2b4da23df2054391eab165b) )
	ROM_LOAD32_BYTE( "u5.bin",  0x200003, 0x080000, CRC(4cc65f5e) SHA1(eebad4c1bf761f08cacbf8c75e7f7bd421ee65ca) )
	ROM_LOAD32_BYTE( "u6.bin",  0x200002, 0x080000, CRC(d84b7c6c) SHA1(ba6ab34fb5c61aa1a97159b7aa3d89e978fb0538) )
	ROM_LOAD32_BYTE( "u7.bin",  0x200001, 0x080000, CRC(656b2125) SHA1(ac3874e71ec0aa4e77ac0d556e4572606ce673c7) )
	ROM_LOAD32_BYTE( "u8.bin",  0x200000, 0x080000, CRC(944df309) SHA1(ee85f6dbfe970b63943d01d9f8b491717a4d5a71) )

	ROM_REGION( 0x100000, REGION_CPU2, 0 )
	ROM_LOAD16_BYTE( "u20.bin", 0x000001, 0x020000, CRC(d7ad1ffb) SHA1(9d375285628b32296c93456a00bc005a3f40ce38) )
	ROM_LOAD16_BYTE( "u21.bin", 0x000000, 0x020000, CRC(6fc625c6) SHA1(542bc025cf0e37686eae5d6c80bc5e047d6389fd) )

	ROM_REGION( 0x20000, REGION_USER1, 0 )
	ROM_LOAD( "u49.bin", 0x000000, 0x020000, CRC(1590ad81) SHA1(04fb8119d9eafc6d2a921700dfb11e9c8b705c88) )  /* 'BIOS'*/

	ROM_REGION( 0x1000000, REGION_GFX1, 0 )
	ROM_LOAD32_BYTE( "u9.bin",  0x0000003, 0x0400000, CRC(aaaf2ca9) SHA1(b9e59590daf4cdee4b1deeb6d4ecc80eb12a2e18) )
	ROM_LOAD32_BYTE( "u10.bin", 0x0000002, 0x0400000, CRC(f0349691) SHA1(623a680ad164d407be0af585a15540f0dca995a4) )
	ROM_LOAD32_BYTE( "u11.bin", 0x0000001, 0x0400000, CRC(893ab178) SHA1(ba68b9a3e81af4c2565715504ada35c7da3f135f) )
	ROM_LOAD32_BYTE( "u12.bin", 0x0000000, 0x0400000, CRC(39b731b8) SHA1(2bf1d083fc6d8058a0d26b29714945e8be0e2c79) )

	ROM_REGION( 0x400000, REGION_GFX2, 0 )
	ROM_LOAD( "u13.bin", 0x0000000, 0x0400000, CRC(3dcbb041) SHA1(fcff67113707fcf14d49538551724490498c0909) )

	ROM_REGION( 0x400000, REGION_GFX3, 0 )
	ROM_LOAD( "u15.bin", 0x0000000, 0x0400000, CRC(b84224f0) SHA1(7163aec2cc118111b2c5d8deb61133d762a5d74c) )

	ROM_REGION( 0x0200000, REGION_GFX4, 0 )
	ROM_LOAD( "u17.bin", 0x0000000, 0x0200000, CRC(ff93c949) SHA1(13917d73a6cb70d03d0335bd816bf6b094758d0b) )

	ROM_REGION( 0x400000, REGION_GFX5, 0 )
	/* nothing on this game? */

	ROM_REGION16_BE( 0x800000, REGION_SOUND1, 0 )
	ROM_LOAD16_BYTE( "u26.bin", 0x0000000, 0x0400000, CRC(6c8f30d4) SHA1(7e215589e4a52cbce7f2bb31b333f874a9f83d00) )
	ROM_LOAD16_BYTE( "u24.bin", 0x0000001, 0x0400000, CRC(5b12d0b1) SHA1(c5ddff2053148a1da0710a10f48689bf5c736ae4) )

	ROM_REGION16_BE( 0x400000, REGION_SOUND2, 0 )
	/* Filled in below in init routine */

	ROM_REGION16_BE( 0x800000, REGION_SOUND3, 0 )
	ROM_LOAD16_BYTE( "u27.bin", 0x0000000, 0x0400000, CRC(bd75d165) SHA1(2da770d15c812cbfdb4e3048d320071edffccfa1) )
	ROM_LOAD16_BYTE( "u25.bin", 0x0000001, 0x0400000, CRC(3b9689bc) SHA1(0857c3d3e9810f9468f7c17f8b795825c55a9f08) )

	ROM_REGION16_BE( 0x400000, REGION_SOUND4, 0 )
	/* Filled in below in init routine */
ROM_END



static WRITE32_HANDLER( macrossp_speedup_w )
{
/*
PC :00018104 018104: addq.w  #1, $f1015a.l
PC :0001810A 01810A: cmp.w   $f10140.l, D0
PC :00018110 018110: beq     18104
*/
	COMBINE_DATA(&macrossp_mainram[0x10158/4]);
	if (activecpu_get_pc()==0x001810A) cpu_spinuntil_int();
}

static DRIVER_INIT( macrossp )
{
	/* Expand top half of sound ROM into second banked sound area */
	const data8_t* src=memory_region(REGION_SOUND1);
	data8_t* dst=memory_region(REGION_SOUND2);

	memcpy(dst,src+0x400000,0x400000);

	install_mem_write32_handler(0, 0xf10158, 0xf1015b, macrossp_speedup_w );
}

static DRIVER_INIT( quizmoon )
{
	/* Expand top half of sound ROM into second banked sound area */
	const data8_t* src=memory_region(REGION_SOUND1);
	data8_t* dst=memory_region(REGION_SOUND2);

	memcpy(dst,src+0x400000,0x400000);

	src=memory_region(REGION_SOUND3);
	dst=memory_region(REGION_SOUND4);
	memcpy(dst,src+0x400000,0x400000);
}

GAMEX( 1996, macrossp, 0, macrossp, macrossp, macrossp, ROT270, "Banpresto", "Macross Plus", GAME_IMPERFECT_GRAPHICS )
GAMEX( 1997, quizmoon, 0, quizmoon, quizmoon, quizmoon, ROT0,   "Banpresto", "Quiz Bisyoujo Senshi Sailor Moon - Chiryoku Tairyoku Toki no Un", GAME_IMPERFECT_GRAPHICS )
