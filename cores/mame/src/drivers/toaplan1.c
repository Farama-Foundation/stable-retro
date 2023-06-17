/***************************************************************************

		ToaPlan game hardware from 1988-1991
		------------------------------------
		MAME Driver by: Darren Olafson
		Technical info: Carl-Henrik Skarstedt  &  Magnus Danielsson
		Driver updates: Quench
		Video updates : SUZ


Supported games:

	ROM set		Toaplan
	name		board No		Game name
	--------------------------------------------------
	rallybik	TP-012		Rally Bike/Dash Yarou
	truxton		TP-013B		Truxton/Tatsujin
	hellfire	TP-???		HellFire (2 Player version)
	hellfir1	TP-???		HellFire (1 Player version)
	zerowing	TP-015		Zero Wing
	demonwld	TP-016		Demon's World/Horror Story [1990]
	demonwl1	TP-016		Demon's World/Horror Story [1989] (Taito license)
	fireshrk	TP-017		Fire Shark (World)			 [1990]
	samesame	TP-017		Same! Same! Same! (Japan)	 [1989] (1 Player version)
	samesam2	TP-017		Same! Same! Same! (Japan)	 [1989] (2 Player version)
	outzone		TP-018		Out Zone
	outzonea	TP-018		Out Zone (From board serial number 2122)
	vimana		TP-019		Vimana (From board serial number 1547.04 [July '94])
	vimana1		TP-019		Vimana (older version)
	vimanan		TP-019		Vimana (Nova Apparate GMBH & Co  license)


Notes:
	Fire Shark and Same! Same! Same! have a hidden function for the
	service input. When invulnerability is enabled, pressing the
	service input makes the screen scroll faster.

	OutZone (set 2) has a bug in the 68K code. An Jump instruction at $3E6
	goes to to an invalid instruction at $13DA4. It should really jump to
	$13DAA. This bad jump is executed by flicking the 'Service DSW' while
	after the game has booted. The other Outzone set correctly goes to
	service mode, but this set just loses the plot.

	OutZone sprite priorities are unusual. On level 4, a character with
	low priority (6) is hidden by a higher priority (8) character, yet it
	shouldn't be. The character is a shooting enemy hidden by a sliding
	left to right platform (which he should be standing on).
	So how does the real hardware deal with this ?

	OutZone (set 2) uses different enemies in some stages and has extra
	bonuses compared to set 1. The music sequences are also in different
	orders between the sets. So which is the newest version ?

	Demonwld (Toaplan copyright) is a newer version, and has a different game
	level sequence compared to the Taito licensed version.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "cpu/tms32010/tms32010.h"
#include "toaplan1.h"

/* Fire Shark / Same Same Same! sound*/
data8_t to_mcu;
data8_t cmdavailable;

WRITE16_HANDLER(samesame_mcu_w);
READ_HANDLER(samesame_soundlatch_r);
WRITE_HANDLER(samesame_sound_done_w);
READ_HANDLER(samesame_cmdavailable_r);
READ_HANDLER(vimana_dswb_invert_r);
READ_HANDLER(vimana_tjump_invert_r);

/* 68000 memory maps */
static MEMORY_READ16_START( rallybik_readmem )
	{ 0x000000, 0x07ffff, MRA16_ROM },
	{ 0x080000, 0x083fff, MRA16_RAM },
	{ 0x0c0000, 0x0c0fff, MRA16_RAM },				/* sprite ram data */
	{ 0x100002, 0x100003, toaplan1_tileram_offs_r },
	{ 0x100004, 0x100007, rallybik_tileram16_r },	/* tile layers */
	{ 0x100010, 0x10001f, toaplan1_scroll_regs_r },
	{ 0x140000, 0x140001, input_port_0_word_r },
	{ 0x144000, 0x1447ff, toaplan1_colorram1_r },
	{ 0x146000, 0x1467ff, toaplan1_colorram2_r },
	{ 0x180000, 0x180fff, toaplan1_shared_r },
MEMORY_END

static MEMORY_WRITE16_START( rallybik_writemem )
	{ 0x000000, 0x07ffff, MWA16_ROM },
	{ 0x080000, 0x083fff, MWA16_RAM },
	{ 0x0c0000, 0x0c0fff, MWA16_RAM, &spriteram16, &spriteram_size },
	{ 0x100000, 0x100001, rallybik_bcu_flipscreen_w },
	{ 0x100002, 0x100003, toaplan1_tileram_offs_w },
	{ 0x100004, 0x100007, toaplan1_tileram16_w },
	{ 0x100010, 0x10001f, toaplan1_scroll_regs_w },
/*	{ 0x140000, 0x140001, ?? video frame related ??  },*/
	{ 0x140002, 0x140003, toaplan1_intenable_w },
	{ 0x140008, 0x14000f, toaplan1_bcu_control_w },
	{ 0x144000, 0x1447ff, toaplan1_colorram1_w, &toaplan1_colorram1, &toaplan1_colorram1_size },
	{ 0x146000, 0x1467ff, toaplan1_colorram2_w, &toaplan1_colorram2, &toaplan1_colorram2_size },
	{ 0x180000, 0x180fff, toaplan1_shared_w },
	{ 0x1c0000, 0x1c0003, toaplan1_tile_offsets_w },
	{ 0x1c8000, 0x1c8001, toaplan1_reset_sound },
MEMORY_END

static MEMORY_READ16_START( truxton_readmem )
	{ 0x000000, 0x03ffff, MRA16_ROM },
	{ 0x080000, 0x083fff, MRA16_RAM },
	{ 0x0c0000, 0x0c0001, toaplan1_frame_done_r },
	{ 0x0c0002, 0x0c0003, toaplan1_spriteram_offs_r },
	{ 0x0c0004, 0x0c0005, toaplan1_spriteram16_r },
	{ 0x0c0006, 0x0c0007, toaplan1_spritesizeram16_r },
	{ 0x100002, 0x100003, toaplan1_tileram_offs_r },
	{ 0x100004, 0x100007, toaplan1_tileram16_r },
	{ 0x100010, 0x10001f, toaplan1_scroll_regs_r },
	{ 0x140000, 0x140001, input_port_0_word_r },
	{ 0x144000, 0x1447ff, toaplan1_colorram1_r },
	{ 0x146000, 0x1467ff, toaplan1_colorram2_r },
	{ 0x180000, 0x180fff, toaplan1_shared_r },
MEMORY_END

static MEMORY_WRITE16_START( truxton_writemem )
	{ 0x000000, 0x03ffff, MWA16_ROM },
	{ 0x080000, 0x083fff, MWA16_RAM },
	{ 0x0c0002, 0x0c0003, toaplan1_spriteram_offs_w },
	{ 0x0c0004, 0x0c0005, toaplan1_spriteram16_w },
	{ 0x0c0006, 0x0c0007, toaplan1_spritesizeram16_w },
	{ 0x100000, 0x100001, toaplan1_bcu_flipscreen_w },
	{ 0x100002, 0x100003, toaplan1_tileram_offs_w },
	{ 0x100004, 0x100007, toaplan1_tileram16_w },
	{ 0x100010, 0x10001f, toaplan1_scroll_regs_w },
/*	{ 0x140000, 0x140001, ?? video frame related ??  },*/
	{ 0x140002, 0x140003, toaplan1_intenable_w },
	{ 0x140008, 0x14000f, toaplan1_bcu_control_w },
	{ 0x144000, 0x1447ff, toaplan1_colorram1_w, &toaplan1_colorram1, &toaplan1_colorram1_size },
	{ 0x146000, 0x1467ff, toaplan1_colorram2_w, &toaplan1_colorram2, &toaplan1_colorram2_size },
	{ 0x180000, 0x180fff, toaplan1_shared_w },
	{ 0x1c0000, 0x1c0003, toaplan1_tile_offsets_w },
	{ 0x1c0006, 0x1c0007, toaplan1_fcu_flipscreen_w },
	{ 0x1d0000, 0x1d0001, toaplan1_reset_sound },
MEMORY_END

static MEMORY_READ16_START( hellfire_readmem )
	{ 0x000000, 0x03ffff, MRA16_ROM },
	{ 0x040000, 0x047fff, MRA16_RAM },
	{ 0x080000, 0x080001, input_port_0_word_r },
	{ 0x084000, 0x0847ff, toaplan1_colorram1_r },
	{ 0x086000, 0x0867ff, toaplan1_colorram2_r },
	{ 0x0c0000, 0x0c0fff, toaplan1_shared_r },
	{ 0x100002, 0x100003, toaplan1_tileram_offs_r },
	{ 0x100004, 0x100007, toaplan1_tileram16_r },
	{ 0x100010, 0x10001f, toaplan1_scroll_regs_r },
	{ 0x140000, 0x140001, toaplan1_frame_done_r },
	{ 0x140002, 0x140003, toaplan1_spriteram_offs_r },
	{ 0x140004, 0x140005, toaplan1_spriteram16_r },
	{ 0x140006, 0x140007, toaplan1_spritesizeram16_r },
MEMORY_END

static MEMORY_WRITE16_START( hellfire_writemem )
	{ 0x000000, 0x03ffff, MWA16_ROM },
	{ 0x040000, 0x047fff, MWA16_RAM },
/*	{ 0x080000, 0x080001, ?? video frame related ??  },	*/
	{ 0x080002, 0x080003, toaplan1_intenable_w },
	{ 0x080008, 0x08000f, toaplan1_bcu_control_w },
	{ 0x084000, 0x0847ff, toaplan1_colorram1_w, &toaplan1_colorram1, &toaplan1_colorram1_size },
	{ 0x086000, 0x0867ff, toaplan1_colorram2_w, &toaplan1_colorram2, &toaplan1_colorram2_size },
	{ 0x0c0000, 0x0c0fff, toaplan1_shared_w },
	{ 0x100000, 0x100001, toaplan1_bcu_flipscreen_w },
	{ 0x100002, 0x100003, toaplan1_tileram_offs_w },
	{ 0x100004, 0x100007, toaplan1_tileram16_w },
	{ 0x100010, 0x10001f, toaplan1_scroll_regs_w },
	{ 0x140002, 0x140003, toaplan1_spriteram_offs_w },
	{ 0x140004, 0x140005, toaplan1_spriteram16_w },
	{ 0x140006, 0x140007, toaplan1_spritesizeram16_w },
	{ 0x180000, 0x180003, toaplan1_tile_offsets_w },
	{ 0x180006, 0x180007, toaplan1_fcu_flipscreen_w },
	{ 0x180008, 0x180009, toaplan1_reset_sound },
MEMORY_END

static MEMORY_READ16_START( zerowing_readmem )
	{ 0x000000, 0x07ffff, MRA16_ROM },
	{ 0x080000, 0x087fff, MRA16_RAM },
	{ 0x400000, 0x400001, input_port_0_word_r },
	{ 0x404000, 0x4047ff, toaplan1_colorram1_r },
	{ 0x406000, 0x4067ff, toaplan1_colorram2_r },
	{ 0x440000, 0x440fff, toaplan1_shared_r },
	{ 0x480002, 0x480003, toaplan1_tileram_offs_r },
	{ 0x480004, 0x480007, toaplan1_tileram16_r },
	{ 0x480010, 0x48001f, toaplan1_scroll_regs_r },
	{ 0x4c0000, 0x4c0001, toaplan1_frame_done_r },
	{ 0x4c0002, 0x4c0003, toaplan1_spriteram_offs_r },
	{ 0x4c0004, 0x4c0005, toaplan1_spriteram16_r },
	{ 0x4c0006, 0x4c0007, toaplan1_spritesizeram16_r },
MEMORY_END

static MEMORY_WRITE16_START( zerowing_writemem )
	{ 0x000000, 0x07ffff, MWA16_ROM },
	{ 0x080000, 0x087fff, MWA16_RAM },
	{ 0x0c0000, 0x0c0003, toaplan1_tile_offsets_w },
	{ 0x0c0006, 0x0c0007, toaplan1_fcu_flipscreen_w },
/*	{ 0x400000, 0x400001, ?? video frame related ??  },	*/
	{ 0x400002, 0x400003, toaplan1_intenable_w },
	{ 0x400008, 0x40000f, toaplan1_bcu_control_w },
	{ 0x404000, 0x4047ff, toaplan1_colorram1_w, &toaplan1_colorram1, &toaplan1_colorram1_size },
	{ 0x406000, 0x4067ff, toaplan1_colorram2_w, &toaplan1_colorram2, &toaplan1_colorram2_size },
	{ 0x440000, 0x440fff, toaplan1_shared_w },
	{ 0x480000, 0x480001, toaplan1_bcu_flipscreen_w },
	{ 0x480002, 0x480003, toaplan1_tileram_offs_w },
	{ 0x480004, 0x480007, toaplan1_tileram16_w },
	{ 0x480010, 0x48001f, toaplan1_scroll_regs_w },
	{ 0x4c0002, 0x4c0003, toaplan1_spriteram_offs_w },
	{ 0x4c0004, 0x4c0005, toaplan1_spriteram16_w },
	{ 0x4c0006, 0x4c0007, toaplan1_spritesizeram16_w },
MEMORY_END

static MEMORY_READ16_START( demonwld_readmem )
	{ 0x000000, 0x03ffff, MRA16_ROM },
	{ 0x400000, 0x400001, input_port_0_word_r },
	{ 0x404000, 0x4047ff, toaplan1_colorram1_r },
	{ 0x406000, 0x4067ff, toaplan1_colorram2_r },
	{ 0x600000, 0x600fff, toaplan1_shared_r },
	{ 0x800002, 0x800003, toaplan1_tileram_offs_r },
	{ 0x800004, 0x800007, toaplan1_tileram16_r },
	{ 0x800010, 0x80001f, toaplan1_scroll_regs_r },
	{ 0xa00000, 0xa00001, toaplan1_frame_done_r },
	{ 0xa00002, 0xa00003, toaplan1_spriteram_offs_r },
	{ 0xa00004, 0xa00005, toaplan1_spriteram16_r },
	{ 0xa00006, 0xa00007, toaplan1_spritesizeram16_r },
	{ 0xc00000, 0xc03fff, MRA16_RAM},
MEMORY_END

static MEMORY_WRITE16_START( demonwld_writemem )
	{ 0x000000, 0x03ffff, MWA16_ROM },
/*	{ 0x400000, 0x400001, ?? video frame related ??  },	*/
	{ 0x400002, 0x400003, toaplan1_intenable_w },
	{ 0x400008, 0x40000f, toaplan1_bcu_control_w },
	{ 0x404000, 0x4047ff, toaplan1_colorram1_w, &toaplan1_colorram1, &toaplan1_colorram1_size },
	{ 0x406000, 0x4067ff, toaplan1_colorram2_w, &toaplan1_colorram2, &toaplan1_colorram2_size },
	{ 0x600000, 0x600fff, toaplan1_shared_w },
	{ 0x800000, 0x800001, toaplan1_bcu_flipscreen_w },
	{ 0x800002, 0x800003, toaplan1_tileram_offs_w },
	{ 0x800004, 0x800007, toaplan1_tileram16_w },
	{ 0x800010, 0x80001f, toaplan1_scroll_regs_w },
	{ 0xa00002, 0xa00003, toaplan1_spriteram_offs_w },
	{ 0xa00004, 0xa00005, toaplan1_spriteram16_w },
	{ 0xa00006, 0xa00007, toaplan1_spritesizeram16_w },
	{ 0xc00000, 0xc03fff, MWA16_RAM },
	{ 0xe00000, 0xe00003, toaplan1_tile_offsets_w },
	{ 0xe00006, 0xe00007, toaplan1_fcu_flipscreen_w },
	{ 0xe00008, 0xe00009, toaplan1_reset_sound },
	{ 0xe0000a, 0xe0000b, demonwld_dsp_ctrl_w },	/* DSP Comms control */
MEMORY_END

static MEMORY_READ16_START( samesame_readmem )
	{ 0x000000, 0x00ffff, MRA16_ROM },
	{ 0x040000, 0x07ffff, MRA16_ROM },
	{ 0x0c0000, 0x0c3fff, MRA16_RAM },
	{ 0x100000, 0x100001, input_port_0_word_r },
	{ 0x104000, 0x1047ff, toaplan1_colorram1_r },
	{ 0x106000, 0x1067ff, toaplan1_colorram2_r },
	{ 0x140000, 0x140001, input_port_1_word_r },
	{ 0x140002, 0x140003, input_port_2_word_r },
	{ 0x140004, 0x140005, input_port_3_word_r },
	{ 0x140006, 0x140007, input_port_4_word_r },
	{ 0x140008, 0x140009, input_port_5_word_r },
	{ 0x14000a, 0x14000b, samesame_port_6_word_r },	/* Territory, and MCU ready */
	{ 0x180002, 0x180003, toaplan1_tileram_offs_r },
	{ 0x180004, 0x180007, toaplan1_tileram16_r },
	{ 0x180010, 0x18001f, toaplan1_scroll_regs_r },
	{ 0x1c0000, 0x1c0001, toaplan1_frame_done_r },
	{ 0x1c0002, 0x1c0003, toaplan1_spriteram_offs_r },
	{ 0x1c0004, 0x1c0005, toaplan1_spriteram16_r },
	{ 0x1c0006, 0x1c0007, toaplan1_spritesizeram16_r },
MEMORY_END

static MEMORY_WRITE16_START( samesame_writemem )
	{ 0x000000, 0x00ffff, MWA16_ROM },
	{ 0x040000, 0x07ffff, MWA16_ROM },
	{ 0x080000, 0x080003, toaplan1_tile_offsets_w },
	{ 0x080006, 0x080007, toaplan1_fcu_flipscreen_w },
	{ 0x0c0000, 0x0c3fff, MWA16_RAM },			/* Frame done at $c1ada */
/*	{ 0x100000, 0x100001, ?? video frame related ??  },	*/
	{ 0x100002, 0x100003, toaplan1_intenable_w },
	{ 0x100008, 0x10000f, toaplan1_bcu_control_w },
	{ 0x104000, 0x1047ff, toaplan1_colorram1_w, &toaplan1_colorram1, &toaplan1_colorram1_size },
	{ 0x106000, 0x1067ff, toaplan1_colorram2_w, &toaplan1_colorram2, &toaplan1_colorram2_size },
	{ 0x14000c, 0x14000d, samesame_coin_w },	/* Coin counter/lockout */
	{ 0x14000e, 0x14000f, samesame_mcu_w },		/* Commands sent to HD647180 */
	{ 0x180000, 0x180001, toaplan1_bcu_flipscreen_w },
	{ 0x180002, 0x180003, toaplan1_tileram_offs_w },
	{ 0x180004, 0x180007, toaplan1_tileram16_w },
	{ 0x180010, 0x18001f, toaplan1_scroll_regs_w },
/*	{ 0x1c0000, 0x1c0001, ??? },				disable sprite refresh ? */
	{ 0x1c0002, 0x1c0003, toaplan1_spriteram_offs_w },
	{ 0x1c0004, 0x1c0005, toaplan1_spriteram16_w },
	{ 0x1c0006, 0x1c0007, toaplan1_spritesizeram16_w },
MEMORY_END

static MEMORY_READ16_START( outzone_readmem )
	{ 0x000000, 0x03ffff, MRA16_ROM },
	{ 0x100000, 0x100001, toaplan1_frame_done_r },
	{ 0x100002, 0x100003, toaplan1_spriteram_offs_r },
	{ 0x100004, 0x100005, toaplan1_spriteram16_r },
	{ 0x100006, 0x100007, toaplan1_spritesizeram16_r },
	{ 0x140000, 0x140fff, toaplan1_shared_r },
	{ 0x200002, 0x200003, toaplan1_tileram_offs_r },
	{ 0x200004, 0x200007, toaplan1_tileram16_r },
	{ 0x200010, 0x20001f, toaplan1_scroll_regs_r },
	{ 0x240000, 0x243fff, MRA16_RAM },
	{ 0x300000, 0x300001, input_port_0_word_r },
	{ 0x304000, 0x3047ff, toaplan1_colorram1_r },
	{ 0x306000, 0x3067ff, toaplan1_colorram2_r },
MEMORY_END

static MEMORY_WRITE16_START( outzone_writemem )
	{ 0x000000, 0x03ffff, MWA16_ROM },
	{ 0x100002, 0x100003, toaplan1_spriteram_offs_w },
	{ 0x100004, 0x100005, toaplan1_spriteram16_w },
	{ 0x100006, 0x100007, toaplan1_spritesizeram16_w },
	{ 0x140000, 0x140fff, toaplan1_shared_w },
	{ 0x200000, 0x200001, toaplan1_bcu_flipscreen_w },
	{ 0x200002, 0x200003, toaplan1_tileram_offs_w },
	{ 0x200004, 0x200007, toaplan1_tileram16_w },
	{ 0x200010, 0x20001f, toaplan1_scroll_regs_w },
	{ 0x240000, 0x243fff, MWA16_RAM },
/*	{ 0x300000, 0x300001, ?? video frame related ??  },	*/
	{ 0x300002, 0x300003, toaplan1_intenable_w },
	{ 0x300008, 0x30000f, toaplan1_bcu_control_w },
	{ 0x304000, 0x3047ff, toaplan1_colorram1_w, &toaplan1_colorram1, &toaplan1_colorram1_size },
	{ 0x306000, 0x3067ff, toaplan1_colorram2_w, &toaplan1_colorram2, &toaplan1_colorram2_size },
	{ 0x340000, 0x340003, toaplan1_tile_offsets_w },
	{ 0x340006, 0x340007, toaplan1_fcu_flipscreen_w },
MEMORY_END

static MEMORY_READ16_START( vimana_readmem )
	{ 0x000000, 0x03ffff, MRA16_ROM },
	{ 0x0c0000, 0x0c0001, toaplan1_frame_done_r },
	{ 0x0c0002, 0x0c0003, toaplan1_spriteram_offs_r },
	{ 0x0c0004, 0x0c0005, toaplan1_spriteram16_r },
	{ 0x0c0006, 0x0c0007, toaplan1_spritesizeram16_r },
	{ 0x400000, 0x400001, input_port_0_word_r },
	{ 0x404000, 0x4047ff, toaplan1_colorram1_r },
	{ 0x406000, 0x4067ff, toaplan1_colorram2_r },
	{ 0x440000, 0x4407ff, toaplan1_shared_r },/* inputs, coins and sound handled by 647180 MCU via this space */
	{ 0x480000, 0x487fff, MRA16_RAM },
	{ 0x4c0002, 0x4c0003, toaplan1_tileram_offs_r },
	{ 0x4c0004, 0x4c0007, toaplan1_tileram16_r },
	{ 0x4c0010, 0x4c001f, toaplan1_scroll_regs_r },
MEMORY_END

static MEMORY_WRITE16_START( vimana_writemem )
	{ 0x000000, 0x03ffff, MWA16_ROM },
	{ 0x080000, 0x080003, toaplan1_tile_offsets_w },
	{ 0x080006, 0x080007, toaplan1_fcu_flipscreen_w },
	{ 0x0c0002, 0x0c0003, toaplan1_spriteram_offs_w },
	{ 0x0c0004, 0x0c0005, toaplan1_spriteram16_w },
	{ 0x0c0006, 0x0c0007, toaplan1_spritesizeram16_w },
/*	{ 0x400000, 0x400001, ?? video frame related ??  },	*/
	{ 0x400002, 0x400003, toaplan1_intenable_w },
	{ 0x400008, 0x40000f, toaplan1_bcu_control_w },
	{ 0x404000, 0x4047ff, toaplan1_colorram1_w, &toaplan1_colorram1, &toaplan1_colorram1_size },
	{ 0x406000, 0x4067ff, toaplan1_colorram2_w, &toaplan1_colorram2, &toaplan1_colorram2_size },
	{ 0x440000, 0x4407ff, toaplan1_shared_w },/* inputs, coins and sound handled by 647180 MCU via this space */
	{ 0x480000, 0x487fff, MWA16_RAM },
	{ 0x4c0000, 0x4c0001, toaplan1_bcu_flipscreen_w },
	{ 0x4c0002, 0x4c0003, toaplan1_tileram_offs_w },
	{ 0x4c0004, 0x4c0007, toaplan1_tileram16_w },
	{ 0x4c0010, 0x4c001f, toaplan1_scroll_regs_w },
MEMORY_END



/* Z80 memory/port maps */
static MEMORY_READ_START( sound_readmem )
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0xffff, MRA_RAM },
MEMORY_END

static MEMORY_WRITE_START( sound_writemem )
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0x8000, 0xffff, MWA_RAM, &toaplan1_sharedram },
MEMORY_END


static PORT_READ_START( truxton_sound_readport )
	{ 0x00, 0x00, input_port_1_r },	/* Player 1 */
	{ 0x10, 0x10, input_port_2_r },	/* Player 2 */
	{ 0x20, 0x20, input_port_5_r },	/* Coin/Start inputs */
	{ 0x40, 0x40, input_port_3_r },	/* DSW1 */
	{ 0x50, 0x50, input_port_4_r },	/* DSW2 */
	{ 0x60, 0x60, YM3812_status_port_0_r },
	{ 0x70, 0x70, input_port_6_r },	/* Territory Jumper Block for Truxton */
PORT_END

static PORT_WRITE_START( truxton_sound_writeport )
	{ 0x30, 0x30, toaplan1_coin_w },	/* Coin counter/lockout */
	{ 0x60, 0x60, YM3812_control_port_0_w },
	{ 0x61, 0x61, YM3812_write_port_0_w },
PORT_END

static PORT_WRITE_START( rallybik_sound_writeport )
	{ 0x30, 0x30, rallybik_coin_w },	/* Coin counter/lockout */
	{ 0x60, 0x60, YM3812_control_port_0_w },
	{ 0x61, 0x61, YM3812_write_port_0_w },
PORT_END

static PORT_READ_START( hellfire_sound_readport )
	{ 0x00, 0x00, input_port_3_r },	/* DSW1 */
	{ 0x10, 0x10, input_port_4_r },	/* DSW2 */
	{ 0x20, 0x20, input_port_6_r },	/* Territory Jumper Block */
	{ 0x40, 0x40, input_port_1_r },	/* Player 1 */
	{ 0x50, 0x50, input_port_2_r },	/* Player 2 */
	{ 0x60, 0x60, input_port_5_r },	/* Coin/Start inputs */
	{ 0x70, 0x70, YM3812_status_port_0_r },
PORT_END

static PORT_WRITE_START( hellfire_sound_writeport )
	{ 0x30, 0x30, toaplan1_coin_w },	/* Coin counter/lockout */
	{ 0x70, 0x70, YM3812_control_port_0_w },
	{ 0x71, 0x71, YM3812_write_port_0_w },
PORT_END

static PORT_READ_START( zerowing_sound_readport )
	{ 0x00, 0x00, input_port_1_r },	/* Player 1 */
	{ 0x08, 0x08, input_port_2_r },	/* Player 2 */
	{ 0x20, 0x20, input_port_3_r },	/* DSW1 */
	{ 0x28, 0x28, input_port_4_r },	/* DSW2 */
	{ 0x80, 0x80, input_port_5_r },	/* Coin/Start inputs */
	{ 0x88, 0x88, input_port_6_r },	/* Territory Jumper Block */
	{ 0xa8, 0xa8, YM3812_status_port_0_r },
PORT_END

static PORT_WRITE_START( zerowing_sound_writeport )
	{ 0xa0, 0xa0, toaplan1_coin_w },	/* Coin counter/lockout */
	{ 0xa8, 0xa8, YM3812_control_port_0_w },
	{ 0xa9, 0xa9, YM3812_write_port_0_w },
PORT_END

static PORT_READ_START( demonwld_sound_readport )
	{ 0x00, 0x00, YM3812_status_port_0_r },
	{ 0x20, 0x20, input_port_6_r },
	{ 0x60, 0x60, input_port_5_r },
	{ 0x80, 0x80, input_port_1_r },
	{ 0xa0, 0xa0, input_port_4_r },
	{ 0xc0, 0xc0, input_port_2_r },
	{ 0xe0, 0xe0, input_port_3_r },
PORT_END

static PORT_WRITE_START( demonwld_sound_writeport )
	{ 0x00, 0x00, YM3812_control_port_0_w },
	{ 0x01, 0x01, YM3812_write_port_0_w },
	{ 0x40, 0x40, toaplan1_coin_w },	/* Coin counter/lockout */
PORT_END

static PORT_READ_START( outzone_sound_readport )
	{ 0x08, 0x08, input_port_3_r },
	{ 0x0c, 0x0c, input_port_4_r },
	{ 0x10, 0x10, input_port_5_r },
	{ 0x14, 0x14, input_port_1_r },
	{ 0x18, 0x18, input_port_2_r },
	{ 0x1c, 0x1c, input_port_6_r },
	{ 0x00, 0x00, YM3812_status_port_0_r },
PORT_END

static PORT_WRITE_START( outzone_sound_writeport )
	{ 0x00, 0x00, YM3812_control_port_0_w },
	{ 0x01, 0x01, YM3812_write_port_0_w },
	{ 0x04, 0x04, toaplan1_coin_w },	/* Coin counter/lockout */
PORT_END


/* TMS32010 memory/port maps */
static MEMORY_READ16_START( DSP_readmem )
	{ TMS32010_DATA_ADDR_RANGE(0x000, 0x08f), MRA16_RAM },	/* 90h words internal RAM */
	{ TMS32010_PGM_ADDR_RANGE(0x000, 0x7ff), MRA16_ROM },	/* 800h words. The real DSPs ROM is at */
									/* address 0 */
									/* View it at 8000h in the debugger */
MEMORY_END

static MEMORY_WRITE16_START( DSP_writemem )
	{ TMS32010_DATA_ADDR_RANGE(0x000, 0x08f), MWA16_RAM },
	{ TMS32010_PGM_ADDR_RANGE(0x000, 0x7ff), MWA16_ROM },
MEMORY_END


static PORT_READ16_START( DSP_readport )
	{ TMS32010_PORT_RANGE(1, 1),  demonwld_dsp_r },
	{ TMS32010_PORT_RANGE(TMS32010_BIO, TMS32010_BIO), demonwld_BIO_r },
PORT_END

static PORT_WRITE16_START( DSP_writeport )
	{ TMS32010_PORT_RANGE(0, 3), demonwld_dsp_w },
PORT_END

WRITE16_HANDLER(samesame_mcu_w)
{
    to_mcu = data;
	cmdavailable = 1;
};

READ_HANDLER(samesame_soundlatch_r)
{
	return to_mcu;
};

WRITE_HANDLER(samesame_sound_done_w)
{
	to_mcu = data;
	cmdavailable = 0;
}

READ_HANDLER(samesame_cmdavailable_r)
{
	if (cmdavailable) return 0xff;
	else return 0x00;
};

 
static MEMORY_READ_START( samesame_hd647180_readmem )
	{ 0x00000, 0x03fff, MRA_ROM },
	{ 0x0fe00, 0x0ffff, MRA_RAM },
MEMORY_END

static MEMORY_WRITE_START( samesame_hd647180_writemem )
	{ 0x00000, 0x03fff, MWA_ROM },
	{ 0x0fe00, 0x0ffff, MWA_RAM },
MEMORY_END
 
static PORT_READ_START( samesame_hd647180_readport )
    { 0x63, 0x63, samesame_cmdavailable_r },
	{ 0xa0, 0xa0, samesame_soundlatch_r },
	{ 0x80, 0x80, YM3812_status_port_0_r },
	{ 0x81, 0x81, YM3812_status_port_0_r },
PORT_END

static PORT_WRITE_START( samesame_hd647180_writeport )
	{ 0xb0, 0xb0, samesame_sound_done_w },	/* Coin counter/lockout */
	{ 0x80, 0x80, YM3812_control_port_0_w },
	{ 0x81, 0x81, YM3812_write_port_0_w },
PORT_END

READ_HANDLER(vimana_dswb_invert_r)
{
    return input_port_4_r(0)^0xFF;
}
 
READ_HANDLER(vimana_tjump_invert_r)
{
	return (input_port_6_r(0)^0xFF)|0xC0; /* high 2 bits of port G always read as 1*/
} 
 
static MEMORY_READ_START( vimana_hd647180_readmem )
	{ 0x00000, 0x03fff, MRA_ROM },
	{ 0x08000, 0x087ff, MRA_RAM }, /* 2048 bytes of shared ram w/maincpu */
	{ 0x0fe00, 0x0ffff, MRA_RAM },
MEMORY_END

static MEMORY_WRITE_START( vimana_hd647180_writemem )
	{ 0x00000, 0x03fff, MWA_ROM },
	{ 0x08000, 0x087ff, MWA_RAM, &toaplan1_sharedram }, /* 2048 bytes of shared ram w/maincpu */
	{ 0x0fe00, 0x0ffff, MWA_RAM },
MEMORY_END
 
static PORT_READ_START( vimana_hd647180_readport )
    { 0x60, 0x60, vimana_dswb_invert_r },
	{ 0x66, 0x66, vimana_tjump_invert_r },
	{ 0x80, 0x80, input_port_1_r },
	{ 0x81, 0x81, input_port_2_r },
	{ 0x82, 0x82, input_port_3_r },
	{ 0x83, 0x83, input_port_5_r },
	{ 0x87, 0x87, YM3812_status_port_0_r },
	{ 0x8f, 0x8f, YM3812_status_port_0_r },
PORT_END

static PORT_WRITE_START( vimana_hd647180_writeport )
    { 0x32, 0x32, MWA_NOP }, /* DMA WAIT/Control register*/
	{ 0x33, 0x33, MWA_NOP }, /* IL (int vector low) register*/
	{ 0x36, 0x36, MWA_NOP }, /* refresh control register for RFSH pin*/
	{ 0x71, 0x71, MWA_NOP }, /* ddr for port B*/
	{ 0x72, 0x72, MWA_NOP }, /* ddr for port C*/
	{ 0x73, 0x73, MWA_NOP }, /* ddr for port D*/
	{ 0x74, 0x74, MWA_NOP }, /* ddr for port E*/
	{ 0x75, 0x75, MWA_NOP }, /* ddr for port F*/
	{ 0x84, 0x84, toaplan1_coin_w },  /* Coin counter/lockout */ /* needs verify*/
	{ 0x87, 0x87, YM3812_control_port_0_w },
	{ 0x8f, 0x8f, YM3812_write_port_0_w },
PORT_END

/*****************************************************************************
	Input Port definitions
*****************************************************************************/

#define  TOAPLAN1_PLAYER_INPUT( player, button3 )										\
	PORT_START																	\
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP    | IPF_8WAY | player )	\
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN  | IPF_8WAY | player )	\
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_8WAY | player )	\
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY | player )	\
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 | player )						\
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON2 | player )						\
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, button3 | player )									\
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

#define  TOAPLAN1_SYSTEM_INPUTS						\
	PORT_START										\
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_SERVICE1 ) 	\
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_TILT )		\
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNKNOWN )	\
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_COIN1 )		\
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_COIN2 )		\
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_START1 )	\
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_START2 )	\
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_VBLANK )

#define  TOAPLAN1_VBLANK_INPUT						\
	PORT_START										\
	PORT_BIT( 0x0001, IP_ACTIVE_HIGH, IPT_VBLANK )	\
	PORT_BIT( 0xfffe, IP_ACTIVE_HIGH, IPT_UNKNOWN )


INPUT_PORTS_START( rallybik )
	TOAPLAN1_VBLANK_INPUT

	TOAPLAN1_PLAYER_INPUT( IPF_PLAYER1, IPT_UNKNOWN )

	TOAPLAN1_PLAYER_INPUT( IPF_COCKTAIL, IPT_UNKNOWN )

	PORT_START		/* DSW A */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x02, DEF_STR( On ) )
	PORT_SERVICE( 0x04, IP_ACTIVE_HIGH )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x30, 0x00, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 1C_2C ) )
	PORT_DIPNAME( 0xc0, 0x00, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 1C_2C ) )

	PORT_START		/* DSW B */
	PORT_DIPNAME( 0x03, 0x00, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x01, "Easy" )
	PORT_DIPSETTING(    0x00, "Medium" )
	PORT_DIPSETTING(    0x02, "Hard" )
	PORT_DIPSETTING(    0x03, "Hardest" )
	PORT_DIPNAME( 0x04, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x04, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	PORT_DIPNAME( 0x30, 0x20, "Territory/Copyright" )
	PORT_DIPSETTING(    0x20, "World/Taito Corp Japan" )
	PORT_DIPSETTING(    0x10, "USA/Taito America" )
	PORT_DIPSETTING(    0x00, "Japan/Taito Corp" )
	PORT_DIPSETTING(    0x30, "USA/Taito America (Romstar)" )
	PORT_DIPNAME( 0x40, 0x00, "Dip Switch Display" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x00, "Allow Continue" )
	PORT_DIPSETTING(    0x80, DEF_STR( No ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Yes ) )

	TOAPLAN1_SYSTEM_INPUTS

INPUT_PORTS_END

INPUT_PORTS_START( truxton )
	TOAPLAN1_VBLANK_INPUT

	TOAPLAN1_PLAYER_INPUT( IPF_PLAYER1, IPT_UNKNOWN )

	TOAPLAN1_PLAYER_INPUT( IPF_COCKTAIL, IPT_UNKNOWN )

	PORT_START		/* DSW A */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x02, DEF_STR( On ) )
	PORT_SERVICE( 0x04, IP_ACTIVE_HIGH )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
/* credits per coin options change, depending on the territory setting */
	PORT_DIPNAME( 0x30, 0x00, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 1C_2C ) )
	PORT_DIPNAME( 0xc0, 0x00, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 1C_2C ) )
/* The following are coin settings for Japan
	PORT_DIPNAME( 0x30, 0x00, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPNAME( 0xc0, 0x00, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 1C_6C ) )
*/

	PORT_START		/* DSW B */
	PORT_DIPNAME( 0x03, 0x00, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x01, "Easy" )
	PORT_DIPSETTING(    0x00, "Medium" )
	PORT_DIPSETTING(    0x02, "Hard" )
	PORT_DIPSETTING(    0x03, "Hardest" )
	PORT_DIPNAME( 0x0c, 0x00, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x04, "50K, every 150K" )
	PORT_DIPSETTING(    0x00, "70K, every 200K" )
	PORT_DIPSETTING(    0x08, "100K only" )
	PORT_DIPSETTING(    0x0c, "None" )
	PORT_DIPNAME( 0x30, 0x00, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x30, "2" )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x20, "4" )
	PORT_DIPSETTING(    0x10, "5" )
	PORT_DIPNAME( 0x40, 0x00, "Show Dip Switches" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x00, "Allow Continue" )
	PORT_DIPSETTING(    0x80, DEF_STR( No ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Yes ) )

	TOAPLAN1_SYSTEM_INPUTS

	PORT_START		/* Territory Jumper Block */
	PORT_DIPNAME( 0x07, 0x02, "Territory/Copyright" )
	PORT_DIPSETTING(    0x02, "World/Taito Corp" )
	PORT_DIPSETTING(    0x06, "World/Taito America" )
	PORT_DIPSETTING(    0x04, "US/Taito America" )
	PORT_DIPSETTING(    0x01, "US/Romstar" )
	PORT_DIPSETTING(    0x00, "Japan/Taito Corp" )
/*	PORT_DIPSETTING(    0x05, "Same as 0x04" )*/
/*	PORT_DIPSETTING(    0x03, "Same as 0x02" )*/
/*	PORT_DIPSETTING(    0x07, "Same as 0x06" )*/
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	PORT_BIT( 0xf0, IP_ACTIVE_HIGH, IPT_UNKNOWN )
INPUT_PORTS_END

INPUT_PORTS_START( hellfire )
	TOAPLAN1_VBLANK_INPUT

	TOAPLAN1_PLAYER_INPUT( IPF_PLAYER1, IPT_UNKNOWN )

	TOAPLAN1_PLAYER_INPUT( IPF_PLAYER2, IPT_UNKNOWN )

	PORT_START		/* DSW A */
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x01, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x02, DEF_STR( On ) )
	PORT_SERVICE( 0x04, IP_ACTIVE_HIGH )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x30, 0x00, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPNAME( 0xc0, 0x00, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 1C_6C ) )

	PORT_START		/* DSWB */
	PORT_DIPNAME( 0x03, 0x00, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x01, "Easy" )
	PORT_DIPSETTING(    0x00, "Medium" )
	PORT_DIPSETTING(    0x02, "Hard" )
	PORT_DIPSETTING(    0x03, "Hardest" )
	PORT_DIPNAME( 0x0c, 0x00, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x00, "70K, every 200K" )
	PORT_DIPSETTING(    0x04, "100K, every 250K" )
	PORT_DIPSETTING(    0x08, "100K" )
	PORT_DIPSETTING(    0x0c, "200K" )
	PORT_DIPNAME( 0x30, 0x00, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x30, "2" )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x20, "4" )
	PORT_DIPSETTING(    0x10, "5" )
	PORT_DIPNAME( 0x40, 0x00, "Invulnerability" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x80, DEF_STR( On ) )

	TOAPLAN1_SYSTEM_INPUTS

	PORT_START		/* Territory Jumper block */
	PORT_DIPNAME( 0x03, 0x02, "Territory" )
	PORT_DIPSETTING(    0x02, "Europe" )
/*	PORT_DIPSETTING(    0x03, "Europe" )*/
	PORT_DIPSETTING(    0x01, "US" )
	PORT_DIPSETTING(    0x00, "Japan" )
	PORT_DIPNAME( 0x04, 0x00, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x04, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	PORT_BIT( 0xf0, IP_ACTIVE_HIGH, IPT_UNKNOWN )
INPUT_PORTS_END

INPUT_PORTS_START( hellfir1 )
	TOAPLAN1_VBLANK_INPUT

	TOAPLAN1_PLAYER_INPUT( IPF_PLAYER1, IPT_UNKNOWN )

	TOAPLAN1_PLAYER_INPUT( IPF_PLAYER2, IPT_UNKNOWN )

	PORT_START		/* DSW A */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x02, DEF_STR( On ) )
	PORT_SERVICE( 0x04, IP_ACTIVE_HIGH )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x30, 0x00, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPNAME( 0xc0, 0x00, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 1C_6C ) )

	PORT_START		/* DSWB */
	PORT_DIPNAME( 0x03, 0x00, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x01, "Easy" )
	PORT_DIPSETTING(    0x00, "Medium" )
	PORT_DIPSETTING(    0x02, "Hard" )
	PORT_DIPSETTING(    0x03, "Hardest" )
	PORT_DIPNAME( 0x0c, 0x00, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x00, "70K, every 200K" )
	PORT_DIPSETTING(    0x04, "100K, every 250K" )
	PORT_DIPSETTING(    0x08, "100K" )
	PORT_DIPSETTING(    0x0c, "200K" )
	PORT_DIPNAME( 0x30, 0x00, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x30, "2" )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x20, "4" )
	PORT_DIPSETTING(    0x10, "5" )
	PORT_DIPNAME( 0x40, 0x00, "Invulnerability" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x00, "Allow Continue" )
	PORT_DIPSETTING(    0x80, DEF_STR( No ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Yes ) )

	TOAPLAN1_SYSTEM_INPUTS

	PORT_START		/* Territory Jumper block */
	PORT_DIPNAME( 0x03, 0x02, "Territory" )
	PORT_DIPSETTING(    0x02, "Europe" )
/*	PORT_DIPSETTING(    0x03, "Europe" )*/
	PORT_DIPSETTING(    0x01, "US" )
	PORT_DIPSETTING(    0x00, "Japan" )
	PORT_DIPNAME( 0x04, 0x00, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x04, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	PORT_BIT( 0xf0, IP_ACTIVE_HIGH, IPT_UNKNOWN )
INPUT_PORTS_END

INPUT_PORTS_START( zerowing )
	TOAPLAN1_VBLANK_INPUT

	TOAPLAN1_PLAYER_INPUT( IPF_PLAYER1, IPT_UNKNOWN )

	TOAPLAN1_PLAYER_INPUT( IPF_COCKTAIL, IPT_UNKNOWN )

	PORT_START		/* DSW A */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x02, DEF_STR( On ) )
	PORT_SERVICE( 0x04, IP_ACTIVE_HIGH )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x30, 0x00, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPNAME( 0xc0, 0x00, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 1C_6C ) )

	PORT_START		/* DSW B */
	PORT_DIPNAME( 0x03, 0x00, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x01, "Easy" )
	PORT_DIPSETTING(    0x00, "Medium" )
	PORT_DIPSETTING(    0x02, "Hard" )
	PORT_DIPSETTING(    0x03, "Hardest" )
	PORT_DIPNAME( 0x0c, 0x00, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x00, "200K, every 500K" )
	PORT_DIPSETTING(    0x04, "500K, every 1M" )
	PORT_DIPSETTING(    0x08, "500K" )
	PORT_DIPSETTING(    0x0c, "None" )
	PORT_DIPNAME( 0x30, 0x00, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x30, "2" )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x20, "4" )
	PORT_DIPSETTING(    0x10, "5" )
	PORT_DIPNAME( 0x40, 0x00, "Invulnerability" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x00, "Allow Continue" )
	PORT_DIPSETTING(    0x80, DEF_STR( No ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Yes ) )

	TOAPLAN1_SYSTEM_INPUTS

	PORT_START		/* Territory Jumper block */
	PORT_DIPNAME( 0x03, 0x02, "Territory" )
	PORT_DIPSETTING(    0x02, "Europe" )
/*	PORT_DIPSETTING(    0x03, "Europe" )*/
	PORT_DIPSETTING(    0x01, "US" )
	PORT_DIPSETTING(    0x00, "Japan" )
	PORT_DIPNAME( 0x04, 0x00, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x04, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	PORT_BIT( 0xf0, IP_ACTIVE_HIGH, IPT_UNKNOWN )
INPUT_PORTS_END

INPUT_PORTS_START( demonwld )
	TOAPLAN1_VBLANK_INPUT

	TOAPLAN1_PLAYER_INPUT( IPF_PLAYER1, IPT_BUTTON3 )

	TOAPLAN1_PLAYER_INPUT( IPF_PLAYER2, IPT_BUTTON3 )

	PORT_START		/* DSW A */
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x01, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x02, DEF_STR( On ) )
	PORT_SERVICE( 0x04, IP_ACTIVE_HIGH )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x30, 0x00, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPNAME( 0xc0, 0x00, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 1C_6C ) )

	PORT_START		/* DSW B */
	PORT_DIPNAME( 0x03, 0x00, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x01, "Easy" )
	PORT_DIPSETTING(    0x00, "Medium" )
	PORT_DIPSETTING(    0x02, "Hard" )
	PORT_DIPSETTING(    0x03, "Hardest" )
	PORT_DIPNAME( 0x0c, 0x00, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x00, "30K, every 100K" )
	PORT_DIPSETTING(    0x04, "50K and 100K" )
	PORT_DIPSETTING(    0x08, "100K only" )
	PORT_DIPSETTING(    0x0c, "None" )
	PORT_DIPNAME( 0x30, 0x00, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x30, "1" )
	PORT_DIPSETTING(    0x20, "2" )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x10, "5" )
	PORT_DIPNAME( 0x40, 0x00, "Invulnerability" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x80, DEF_STR( On ) )

	TOAPLAN1_SYSTEM_INPUTS

	PORT_START		/* Territory Jumper Block */
	PORT_DIPNAME( 0x01, 0x01, "Territory/Copyright" )
	PORT_DIPSETTING(    0x01, "Toaplan" )
	PORT_DIPSETTING(    0x00, "Japan/Taito Corp" )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x02, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x04, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	PORT_BIT( 0xf0, IP_ACTIVE_HIGH, IPT_UNKNOWN )
INPUT_PORTS_END

INPUT_PORTS_START( demonwl1 )
	TOAPLAN1_VBLANK_INPUT

	TOAPLAN1_PLAYER_INPUT( IPF_PLAYER1, IPT_BUTTON3 )

	TOAPLAN1_PLAYER_INPUT( IPF_PLAYER2, IPT_BUTTON3 )

	PORT_START		/* DSW A */
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x01, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x02, DEF_STR( On ) )
	PORT_SERVICE( 0x04, IP_ACTIVE_HIGH )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x30, 0x00, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPNAME( 0xc0, 0x00, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 1C_6C ) )

	PORT_START		/* DSW B */
	PORT_DIPNAME( 0x03, 0x00, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x01, "Easy" )
	PORT_DIPSETTING(    0x00, "Medium" )
	PORT_DIPSETTING(    0x02, "Hard" )
	PORT_DIPSETTING(    0x03, "Hardest" )
	PORT_DIPNAME( 0x0c, 0x00, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x00, "30K, every 100K" )
	PORT_DIPSETTING(    0x04, "50K and 100K" )
	PORT_DIPSETTING(    0x08, "100K only" )
	PORT_DIPSETTING(    0x0c, "None" )
	PORT_DIPNAME( 0x30, 0x00, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x30, "1" )
	PORT_DIPSETTING(    0x20, "2" )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x10, "5" )
	PORT_DIPNAME( 0x40, 0x00, "Invulnerability" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x80, DEF_STR( On ) )

	TOAPLAN1_SYSTEM_INPUTS

	PORT_START		/* Territory Jumper Block */
	PORT_DIPNAME( 0x03, 0x02, "Territory/Copyright" )
	PORT_DIPSETTING(    0x02, "World/Taito Japan" )
	PORT_DIPSETTING(    0x03, "US/Toaplan" )
	PORT_DIPSETTING(    0x01, "US/Taito America" )
	PORT_DIPSETTING(    0x00, "Japan/Taito Corp" )
	PORT_DIPNAME( 0x04, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x04, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	PORT_BIT( 0xf0, IP_ACTIVE_HIGH, IPT_UNKNOWN )
INPUT_PORTS_END

INPUT_PORTS_START( samesame )
	TOAPLAN1_VBLANK_INPUT

	TOAPLAN1_PLAYER_INPUT( IPF_PLAYER1, IPT_UNKNOWN )

	TOAPLAN1_PLAYER_INPUT( IPF_COCKTAIL, IPT_UNKNOWN )

	PORT_START		/* DSW A */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x02, DEF_STR( On ) )
	PORT_SERVICE( 0x04, IP_ACTIVE_HIGH )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
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
/* settings listed in service mode, but not actually used ???
	PORT_DIPNAME( 0x30, 0x00, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 1C_2C ) )
	PORT_DIPNAME( 0xc0, 0x00, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 1C_2C ) )
*/

	PORT_START		/* DSW B */
	PORT_DIPNAME( 0x03, 0x00, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x01, "Easy" )
	PORT_DIPSETTING(    0x00, "Medium" )
	PORT_DIPSETTING(    0x02, "Hard" )
	PORT_DIPSETTING(    0x03, "Hardest" )
	PORT_DIPNAME( 0x0c, 0x00, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x04, "50K, every 150K" )
	PORT_DIPSETTING(    0x00, "70K, every 200K" )
	PORT_DIPSETTING(    0x08, "100K" )
	PORT_DIPSETTING(    0x0c, "None" )
	PORT_DIPNAME( 0x30, 0x00, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x30, "2" )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x20, "4" )
	PORT_DIPSETTING(    0x10, "5" )
	PORT_DIPNAME( 0x40, 0x00, "Invulnerability" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x00, "Allow Continue" )
	PORT_DIPSETTING(    0x80, DEF_STR( No ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Yes ) )

	TOAPLAN1_SYSTEM_INPUTS

	PORT_START		/* Territory Jumper Block */
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x01, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x02, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x04, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	PORT_BIT( 0xf0, IP_ACTIVE_HIGH, IPT_UNKNOWN )
INPUT_PORTS_END

INPUT_PORTS_START( samesam2 )
	TOAPLAN1_VBLANK_INPUT

	TOAPLAN1_PLAYER_INPUT( IPF_PLAYER1, IPT_UNKNOWN )

	TOAPLAN1_PLAYER_INPUT( IPF_PLAYER2, IPT_UNKNOWN )

	PORT_START		/* DSW A */
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x01, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x02, DEF_STR( On ) )
	PORT_SERVICE( 0x04, IP_ACTIVE_HIGH )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
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
/* settings listed in service mode, but not actually used ???
	PORT_DIPNAME( 0x30, 0x00, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 1C_2C ) )
	PORT_DIPNAME( 0xc0, 0x00, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 1C_2C ) )
*/

	PORT_START		/* DSW B */
	PORT_DIPNAME( 0x03, 0x00, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x01, "Easy" )
	PORT_DIPSETTING(    0x00, "Medium" )
	PORT_DIPSETTING(    0x02, "Hard" )
	PORT_DIPSETTING(    0x03, "Hardest" )
	PORT_DIPNAME( 0x0c, 0x00, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x04, "50K, every 150K" )
	PORT_DIPSETTING(    0x00, "70K, every 200K" )
	PORT_DIPSETTING(    0x08, "100K" )
	PORT_DIPSETTING(    0x0c, "None" )
	PORT_DIPNAME( 0x30, 0x00, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x30, "2" )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x20, "4" )
	PORT_DIPSETTING(    0x10, "5" )
	PORT_DIPNAME( 0x40, 0x00, "Invulnerability" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x80, DEF_STR( On ) )

	TOAPLAN1_SYSTEM_INPUTS

	PORT_START		/* Territory Jumper Block */
/* settings listed in service mode, but not actually used
	PORT_DIPNAME( 0x03, 0x00, "Territory" )
//	PORT_DIPSETTING(    0x01, "Europe" )
//	PORT_DIPSETTING(    0x02, "Europe" )
	PORT_DIPSETTING(    0x03, "Europe" )
	PORT_DIPSETTING(    0x00, "USA" )
*/
	PORT_DIPNAME( 0x01, 0x00, "Show Territory Notice" )
	PORT_DIPSETTING(    0x01, DEF_STR( No ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Yes ) )
/*	PORT_DIPNAME( 0x02, 0x00, "Show Territory Notice" )	 // Same as Bit 1 */
/*	PORT_DIPSETTING(    0x02, DEF_STR( No ) )*/
/*	PORT_DIPSETTING(    0x00, DEF_STR( Yes ) )*/
	PORT_DIPNAME( 0x04, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x04, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	PORT_BIT( 0xf2, IP_ACTIVE_HIGH, IPT_UNKNOWN )	/* Mask bit 2 aswell */
INPUT_PORTS_END

INPUT_PORTS_START( fireshrk )
	TOAPLAN1_VBLANK_INPUT

	TOAPLAN1_PLAYER_INPUT( IPF_PLAYER1, IPT_UNKNOWN )

	TOAPLAN1_PLAYER_INPUT( IPF_PLAYER2, IPT_UNKNOWN )

	PORT_START		/* DSW A */
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x01, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x02, DEF_STR( On ) )
	PORT_SERVICE( 0x04, IP_ACTIVE_HIGH )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x30, 0x00, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPNAME( 0xc0, 0x00, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 1C_6C ) )

	PORT_START		/* DSW B */
	PORT_DIPNAME( 0x03, 0x00, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x01, "Easy" )
	PORT_DIPSETTING(    0x00, "Medium" )
	PORT_DIPSETTING(    0x02, "Hard" )
	PORT_DIPSETTING(    0x03, "Hardest" )
	PORT_DIPNAME( 0x0c, 0x00, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x04, "50K, every 150K" )
	PORT_DIPSETTING(    0x00, "70K, every 200K" )
	PORT_DIPSETTING(    0x08, "100K" )
	PORT_DIPSETTING(    0x0c, "None" )
	PORT_DIPNAME( 0x30, 0x00, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x30, "2" )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x20, "4" )
	PORT_DIPSETTING(    0x10, "5" )
	PORT_DIPNAME( 0x40, 0x00, "Invulnerability" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On ) )
	PORT_DIPNAME( 0x80,	0x00, DEF_STR( Unused ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x80, DEF_STR( On ) )

	TOAPLAN1_SYSTEM_INPUTS

	PORT_START		/* Territory Jumper Block */
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x01, DEF_STR( On ) )
	PORT_DIPNAME( 0x06, 0x02, "Territory" )
	PORT_DIPSETTING(    0x02, "Europe" )
	PORT_DIPSETTING(    0x04, "USA" )
	PORT_DIPSETTING(    0x00, "USA (Romstar)" )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	PORT_BIT( 0xf0, IP_ACTIVE_HIGH, IPT_UNKNOWN )
INPUT_PORTS_END

INPUT_PORTS_START( outzone )
	TOAPLAN1_VBLANK_INPUT

	TOAPLAN1_PLAYER_INPUT( IPF_PLAYER1, IPT_BUTTON3 )

	TOAPLAN1_PLAYER_INPUT( IPF_PLAYER2, IPT_BUTTON3 )

	PORT_START		/* DSW A */
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x01, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x02, DEF_STR( On ) )
	PORT_SERVICE( 0x04, IP_ACTIVE_HIGH )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x30, 0x00, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPNAME( 0xc0, 0x00, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 1C_6C ) )

	PORT_START		/* DSW B */
	PORT_DIPNAME( 0x03, 0x00, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x01, "Easy" )
	PORT_DIPSETTING(    0x00, "Medium" )
	PORT_DIPSETTING(    0x02, "Hard" )
	PORT_DIPSETTING(    0x03, "Hardest" )
	PORT_DIPNAME( 0x0c, 0x00, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x00, "Every 300K" )
	PORT_DIPSETTING(    0x04, "200K and 500K" )
	PORT_DIPSETTING(    0x08, "300K only" )
	PORT_DIPSETTING(    0x0c, "None" )
	PORT_DIPNAME( 0x30, 0x00, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x30, "1" )
	PORT_DIPSETTING(    0x20, "2" )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x10, "5" )
	PORT_DIPNAME( 0x40, 0x00, "Invulnerability" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x80, DEF_STR( On ) )

	TOAPLAN1_SYSTEM_INPUTS

	PORT_START		/* Territory Jumper Block */
	PORT_DIPNAME( 0x07, 0x02, "Territory" )
	PORT_DIPSETTING(    0x02, "Europe" )
	PORT_DIPSETTING(    0x01, "US" )
	PORT_DIPSETTING(    0x00, "Japan" )
	PORT_DIPSETTING(    0x03, "Hong Kong" )
	PORT_DIPSETTING(    0x04, "Korea" )
	PORT_DIPSETTING(    0x05, "Taiwan" )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	PORT_BIT( 0xf0, IP_ACTIVE_HIGH, IPT_UNKNOWN )
INPUT_PORTS_END

INPUT_PORTS_START( vimana )
	TOAPLAN1_VBLANK_INPUT

	TOAPLAN1_PLAYER_INPUT( IPF_PLAYER1, IPT_BUTTON3 )

	TOAPLAN1_PLAYER_INPUT( IPF_PLAYER2, IPT_BUTTON3 )

	PORT_START		/* DSW A */
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x01, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x02, DEF_STR( On ) )
	PORT_SERVICE( 0x04, IP_ACTIVE_HIGH )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x30, 0x00, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPNAME( 0xc0, 0x00, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 1C_6C ) )

	PORT_START		/* DSW B */
	PORT_DIPNAME( 0x03, 0x00, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x01, "Easy" )
	PORT_DIPSETTING(    0x00, "Medium" )
	PORT_DIPSETTING(    0x02, "Hard" )
	PORT_DIPSETTING(    0x03, "Hardest" )
	PORT_DIPNAME( 0x0c, 0x00, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x00, "70K and 200K" )
	PORT_DIPSETTING(    0x04, "100K and 250K" )
	PORT_DIPSETTING(    0x08, "100K" )
	PORT_DIPSETTING(    0x0c, "200K" )
	PORT_DIPNAME( 0x30, 0x00, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x30, "2" )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x20, "4" )
	PORT_DIPSETTING(    0x10, "5" )
	PORT_DIPNAME( 0x40, 0x00, "Invulnerability" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x00, "Allow Continue" )
	PORT_DIPSETTING(    0x80, DEF_STR( No ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Yes ) )

	TOAPLAN1_SYSTEM_INPUTS

	PORT_START
    PORT_DIPNAME( 0x0f, 0x02, "Region" )
	PORT_DIPSETTING(    0x02, "Europe" )
	PORT_DIPSETTING(    0x01, "USA" )
	PORT_DIPSETTING(    0x07, "USA (Romstar license)" )
    /*  PORT_DIPSETTING(    0x00, "Japan (distributed by Tecmo)" )*/
    /*  PORT_DIPSETTING(    0x0f, "Japan (distributed by Tecmo)" )*/
	PORT_DIPSETTING(    0x04, "Korea")
	PORT_DIPSETTING(    0x03, "Hong_Kong" )
	PORT_DIPSETTING(    0x08, "Hong Kong (Honest Trading license)" )
	PORT_DIPSETTING(    0x05, "Taiwan" )
	PORT_DIPSETTING(    0x06, "Taiwan (Spacy license)" )
    /*  PORT_DIPSETTING(    0x09, "???" )*/
    /*  PORT_DIPSETTING(    0x0a, "???" )*/
    /*  PORT_DIPSETTING(    0x0b, "???" )*/
    /*  PORT_DIPSETTING(    0x0c, "???" )*/
    /*  PORT_DIPSETTING(    0x0d, "???" )*/
    /*  PORT_DIPSETTING(    0x0e, "???" )*/
	PORT_BIT( 0xf0, IP_ACTIVE_HIGH, IPT_UNKNOWN )
INPUT_PORTS_END

INPUT_PORTS_START( vimanan )
	TOAPLAN1_VBLANK_INPUT

	TOAPLAN1_PLAYER_INPUT( IPF_PLAYER1, IPT_BUTTON3 )

	TOAPLAN1_PLAYER_INPUT( IPF_PLAYER2, IPT_BUTTON3 )

	PORT_START		/* DSW A */
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x01, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x02, DEF_STR( On ) )
	PORT_SERVICE( 0x04, IP_ACTIVE_HIGH )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x00, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x10, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x00, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x20, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x80, DEF_STR( On ) )
/* settings for other territories (non Nova license)
	PORT_DIPNAME( 0x30, 0x00, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPNAME( 0xc0, 0x00, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 1C_6C ) )
*/

	PORT_START		/* DSW B */
	PORT_DIPNAME( 0x03, 0x00, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x01, "Easy" )
	PORT_DIPSETTING(    0x00, "Medium" )
	PORT_DIPSETTING(    0x02, "Hard" )
	PORT_DIPSETTING(    0x03, "Hardest" )
	PORT_DIPNAME( 0x0c, 0x00, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x00, "70K and 200K" )
	PORT_DIPSETTING(    0x04, "100K and 250K" )
	PORT_DIPSETTING(    0x08, "100K" )
	PORT_DIPSETTING(    0x0c, "200K" )
	PORT_DIPNAME( 0x30, 0x00, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x30, "2" )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x20, "4" )
	PORT_DIPSETTING(    0x10, "5" )
	PORT_DIPNAME( 0x40, 0x00, "Invulnerability" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x00, "Allow Continue" )
	PORT_DIPSETTING(    0x80, DEF_STR( No ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Yes ) )

	TOAPLAN1_SYSTEM_INPUTS

	PORT_START
    PORT_DIPNAME( 0x0f, 0x02, "Region" )
	PORT_DIPSETTING(    0x02, "Europe" )
	PORT_DIPSETTING(    0x01, "USA" )
	PORT_DIPSETTING(    0x07, "USA (Romstar license)" )
    /*  PORT_DIPSETTING(    0x00, "Japan (distributed by Tecmo)" )*/
    /*  PORT_DIPSETTING(    0x0f, "Japan (distributed by Tecmo)" )*/
	PORT_DIPSETTING(    0x04, "Korea")
	PORT_DIPSETTING(    0x03, "Hong_Kong" )
	PORT_DIPSETTING(    0x08, "Hong Kong (Honest Trading license)" )
	PORT_DIPSETTING(    0x05, "Taiwan" )
	PORT_DIPSETTING(    0x06, "Taiwan (Spacy license)" )
    /*  PORT_DIPSETTING(    0x09, "???" )*/
    /*  PORT_DIPSETTING(    0x0a, "???" )*/
    /*  PORT_DIPSETTING(    0x0b, "???" )*/
    /*  PORT_DIPSETTING(    0x0c, "???" )*/
    /*  PORT_DIPSETTING(    0x0d, "???" )*/
    /*  PORT_DIPSETTING(    0x0e, "???" )*/
	PORT_BIT( 0xf0, IP_ACTIVE_HIGH, IPT_UNKNOWN )
INPUT_PORTS_END


static struct GfxLayout tilelayout =
{
	8,8,	/* 8x8 */
	16384,	/* 16384 tiles */
	4,		/* 4 bits per pixel */
	{ 3*8*0x20000, 2*8*0x20000, 1*8*0x20000, 0*8*0x20000 },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0, 0x08, 0x10, 0x18, 0x20, 0x28, 0x30, 0x38 },
	64
};

static struct GfxLayout rallybik_spr_layout =
{
	16,16,	/* 16*16 sprites */
	2048,	/* 2048 sprites */
	4,		/* 4 bits per pixel */
	{ 0*2048*32*8, 1*2048*32*8, 2*2048*32*8, 3*2048*32*8 },
	{ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16,
			8*16, 9*16, 10*16, 11*16, 12*16, 13*16, 14*16, 15*16 },
	32*8	/* every sprite takes 32 consecutive bytes */
};

static struct GfxLayout vm_tilelayout =
{
	8,8,	/* 8x8 */
	32768,	/* 32768 tiles */
	4,		/* 4 bits per pixel */
	{ 8*0x80000+8, 8*0x80000, 8, 0 },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0, 0x10, 0x20, 0x30, 0x40, 0x50, 0x60, 0x70 },
	128
};


static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0x00000, &tilelayout,		0, 64 },
	{ REGION_GFX2, 0x00000, &tilelayout,	64*16, 64 },
	{ -1 } /* end of array */
};

static struct GfxDecodeInfo rallybik_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0x00000, &tilelayout,			  0, 64 },
	{ REGION_GFX2, 0x00000, &rallybik_spr_layout, 64*16, 64 },
	{ -1 } /* end of array */
};

static struct GfxDecodeInfo outzone_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0x00000, &vm_tilelayout, 	0, 64 },
	{ REGION_GFX2, 0x00000, &tilelayout,	64*16, 64 },
	{ -1 } /* end of array */
};

static struct GfxDecodeInfo vm_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0x00000, &tilelayout,		0, 64 },
	{ REGION_GFX2, 0x00000, &vm_tilelayout, 64*16, 64 },
	{ -1 } /* end of array */
};


static void irqhandler(int linestate)
{
	cpu_set_irq_line(1,0,linestate);
}

static struct YM3812interface ym3812_interface =
{
	1,
	28000000/8,		/* 3.5MHz (28MHz Oscillator) */
	{ 100 },
	{ irqhandler },
};



static MACHINE_DRIVER_START( rallybik )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 10000000)
	MDRV_CPU_MEMORY(rallybik_readmem,rallybik_writemem)
	MDRV_CPU_VBLANK_INT(toaplan1_interrupt,1)

	MDRV_CPU_ADD(Z80,28000000/8)		/* 3.5MHz (28MHz Oscillator) */
	MDRV_CPU_MEMORY(sound_readmem,sound_writemem)
	MDRV_CPU_PORTS(truxton_sound_readport,rallybik_sound_writeport)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(10)

	MDRV_MACHINE_INIT(toaplan1)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_UPDATE_BEFORE_VBLANK | VIDEO_BUFFERS_SPRITERAM)
	MDRV_SCREEN_SIZE(320, 240)
	MDRV_VISIBLE_AREA(0, 319, 0, 239)
	MDRV_GFXDECODE(rallybik_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH((64*16)+(64*16))

	MDRV_VIDEO_START(rallybik)
	MDRV_VIDEO_EOF(rallybik)
	MDRV_VIDEO_UPDATE(rallybik)

	/* sound hardware */
	MDRV_SOUND_ADD(YM3812, ym3812_interface)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( truxton )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 10000000)
	MDRV_CPU_MEMORY(truxton_readmem,truxton_writemem)
	MDRV_CPU_VBLANK_INT(toaplan1_interrupt,1)

	MDRV_CPU_ADD(Z80,28000000/8)		/* 3.5MHz (28MHz Oscillator) */
	MDRV_CPU_MEMORY(sound_readmem,sound_writemem)
	MDRV_CPU_PORTS(truxton_sound_readport,truxton_sound_writeport)

	MDRV_FRAMES_PER_SECOND( (28000000.0 / 4) / (450 * 270) )
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(10)


	MDRV_MACHINE_INIT(toaplan1)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_UPDATE_BEFORE_VBLANK)
	MDRV_SCREEN_SIZE(320, 240)
	MDRV_VISIBLE_AREA(0, 319, 0, 239)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH((64*16)+(64*16))

	MDRV_VIDEO_START(toaplan1)
	MDRV_VIDEO_EOF(toaplan1)
	MDRV_VIDEO_UPDATE(toaplan1)

	/* sound hardware */
	MDRV_SOUND_ADD(YM3812, ym3812_interface)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( hellfire )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 10000000)
	MDRV_CPU_MEMORY(hellfire_readmem,hellfire_writemem)
	MDRV_CPU_VBLANK_INT(toaplan1_interrupt,1)

	MDRV_CPU_ADD(Z80,28000000/8)		/* 3.5MHz (28MHz Oscillator) */
	MDRV_CPU_MEMORY(sound_readmem,sound_writemem)
	MDRV_CPU_PORTS(hellfire_sound_readport,hellfire_sound_writeport)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(10)

	MDRV_MACHINE_INIT(toaplan1)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_UPDATE_BEFORE_VBLANK)
	MDRV_SCREEN_SIZE(320, 256)
	MDRV_VISIBLE_AREA(0, 319, 16, 255)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH((64*16)+(64*16))

	MDRV_VIDEO_START(toaplan1)
	MDRV_VIDEO_EOF(toaplan1)
	MDRV_VIDEO_UPDATE(toaplan1)

	/* sound hardware */
	MDRV_SOUND_ADD(YM3812, ym3812_interface)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( zerowing )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 10000000)
	MDRV_CPU_MEMORY(zerowing_readmem,zerowing_writemem)
	MDRV_CPU_VBLANK_INT(toaplan1_interrupt,1)

	MDRV_CPU_ADD(Z80,28000000/8)		/* 3.5MHz (28MHz Oscillator) */
	MDRV_CPU_MEMORY(sound_readmem,sound_writemem)
	MDRV_CPU_PORTS(zerowing_sound_readport,zerowing_sound_writeport)

	MDRV_FRAMES_PER_SECOND( (28000000.0 / 4) / (450 * 282) )	/* fixed by SUZ */
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(10)

	MDRV_MACHINE_INIT(zerozone)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_UPDATE_BEFORE_VBLANK)
	MDRV_SCREEN_SIZE(320, 256)
	MDRV_VISIBLE_AREA(0, 319, 16, 255)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH((64*16)+(64*16))

	MDRV_VIDEO_START(toaplan1)
	MDRV_VIDEO_EOF(toaplan1)
	MDRV_VIDEO_UPDATE(zerowing)

	/* sound hardware */
	MDRV_SOUND_ADD(YM3812, ym3812_interface)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( demonwld )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 10000000)
	MDRV_CPU_MEMORY(demonwld_readmem,demonwld_writemem)
	MDRV_CPU_VBLANK_INT(toaplan1_interrupt,1)

	MDRV_CPU_ADD(Z80,28000000/8)		/* 3.5MHz (28MHz Oscillator) */
	MDRV_CPU_MEMORY(sound_readmem,sound_writemem)
	MDRV_CPU_PORTS(demonwld_sound_readport,demonwld_sound_writeport)

	MDRV_CPU_ADD(TMS32010,28000000/2)	/* 14MHz CLKin */
	MDRV_CPU_MEMORY(DSP_readmem,DSP_writemem)
	MDRV_CPU_PORTS(DSP_readport,DSP_writeport)

	MDRV_FRAMES_PER_SECOND( (28000000.0 / 4) / (450 * 282) )
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(10)

	MDRV_MACHINE_INIT(demonwld)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_UPDATE_BEFORE_VBLANK)
	MDRV_SCREEN_SIZE(320, 256)
	MDRV_VISIBLE_AREA(0, 319, 16, 255)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH((64*16)+(64*16))

	MDRV_VIDEO_START(toaplan1)
	MDRV_VIDEO_EOF(toaplan1)
	MDRV_VIDEO_UPDATE(demonwld)

	/* sound hardware */
	MDRV_SOUND_ADD(YM3812, ym3812_interface)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( samesame )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 10000000)
	MDRV_CPU_MEMORY(samesame_readmem,samesame_writemem)
	MDRV_CPU_VBLANK_INT(toaplan1_interrupt,1)
	
	MDRV_CPU_ADD(Z180, 28000000/8)    /* HD647180XOFS6 CPU */
	MDRV_CPU_MEMORY(samesame_hd647180_readmem,samesame_hd647180_writemem)
	MDRV_CPU_PORTS(samesame_hd647180_readport,samesame_hd647180_writeport)

	MDRV_FRAMES_PER_SECOND( (28000000.0 / 4) / (450 * 270) )
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(10)

	MDRV_MACHINE_INIT(toaplan1)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_UPDATE_BEFORE_VBLANK)
	MDRV_SCREEN_SIZE(320, 240)
	MDRV_VISIBLE_AREA(0, 319, 0, 239)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH((64*16)+(64*16))

	MDRV_VIDEO_START(toaplan1)
	MDRV_VIDEO_EOF(samesame)
	MDRV_VIDEO_UPDATE(toaplan1)

	/* sound hardware */
	MDRV_SOUND_ADD(YM3812, ym3812_interface)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( outzone )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 10000000)
	MDRV_CPU_MEMORY(outzone_readmem,outzone_writemem)
	MDRV_CPU_VBLANK_INT(toaplan1_interrupt,1)

	MDRV_CPU_ADD(Z80,28000000/8)		/* 3.5MHz (28MHz Oscillator) */
	MDRV_CPU_MEMORY(sound_readmem,sound_writemem)
	MDRV_CPU_PORTS(outzone_sound_readport,outzone_sound_writeport)

	MDRV_FRAMES_PER_SECOND( (28000000.0 / 4) / (450 * 282) )
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(10)

	MDRV_MACHINE_INIT(zerozone)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_UPDATE_BEFORE_VBLANK)
	MDRV_SCREEN_SIZE(320, 240)
	MDRV_VISIBLE_AREA(0, 319, 0, 239)
	MDRV_GFXDECODE(outzone_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH((64*16)+(64*16))

	MDRV_VIDEO_START(toaplan1)
	MDRV_VIDEO_EOF(toaplan1)
	MDRV_VIDEO_UPDATE(toaplan1)

	/* sound hardware */
	MDRV_SOUND_ADD(YM3812, ym3812_interface)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( vimana )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 10000000)
	MDRV_CPU_MEMORY(vimana_readmem,vimana_writemem)
	MDRV_CPU_VBLANK_INT(toaplan1_interrupt,1)
	
	MDRV_CPU_ADD(Z180, 28000000/8)    /* HD647180XOFS6 CPU */
	MDRV_CPU_MEMORY(vimana_hd647180_readmem,vimana_hd647180_writemem)
	MDRV_CPU_PORTS(vimana_hd647180_readport,vimana_hd647180_writeport)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(10)

	MDRV_MACHINE_INIT(vimana)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_UPDATE_BEFORE_VBLANK)
	MDRV_SCREEN_SIZE(320, 240)
	MDRV_VISIBLE_AREA(0, 319, 0, 239)
	MDRV_GFXDECODE(vm_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH((64*16)+(64*16))

	MDRV_VIDEO_START(toaplan1)
	MDRV_VIDEO_EOF(toaplan1)
	MDRV_VIDEO_UPDATE(toaplan1)

	/* sound hardware */
	MDRV_SOUND_ADD(YM3812, ym3812_interface)
MACHINE_DRIVER_END




/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( rallybik )
	ROM_REGION( 0x080000, REGION_CPU1, 0 )	/* Main 68K code */
	ROM_LOAD16_BYTE( "b45-02.rom",  0x000000, 0x08000, CRC(383386d7) SHA1(fc420b6adc79a408a68f0661d0c62ed7dbe8b6d7) )
	ROM_LOAD16_BYTE( "b45-01.rom",  0x000001, 0x08000, CRC(7602f6a7) SHA1(2939c261a4bc63586681080f5643916c85e81c7d) )
	ROM_LOAD16_BYTE( "b45-04.rom",  0x040000, 0x20000, CRC(e9b005b1) SHA1(19b5acfd5fb2683a56a701400b11ee6f64a9bdf1) )
	ROM_LOAD16_BYTE( "b45-03.rom",  0x040001, 0x20000, CRC(555344ce) SHA1(398963f488fe6f19c0b8518d80c946c242d0fc45) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* Sound Z80 code */
	ROM_LOAD( "b45-05.rom",  0x0000, 0x4000, CRC(10814601) SHA1(bad7a834d8849752a7f3000bb5154ec0fa50d695) )

	ROM_REGION( 0x80000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "b45-09.bin",  0x00000, 0x20000, CRC(1dc7b010) SHA1(67e8633bd787ffcae0e7867e7e591c492c4f2d63) )
	ROM_LOAD( "b45-08.bin",  0x20000, 0x20000, CRC(fab661ba) SHA1(acc43cd6d979b1c6a348727f315643d7b8f1496a) )
	ROM_LOAD( "b45-07.bin",  0x40000, 0x20000, CRC(cd3748b4) SHA1(a20eb19a0f813112b4e5d9cd91db29de9b37af17) )
	ROM_LOAD( "b45-06.bin",  0x60000, 0x20000, CRC(144b085c) SHA1(84b7412d58fe9c5e9915896db92e80a621571b74) )

	ROM_REGION( 0x40000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "b45-11.rom",  0x00000, 0x10000, CRC(0d56e8bb) SHA1(c29cb53f846c73b7cf9936051fb0f9dd3805f53f) )
	ROM_LOAD( "b45-10.rom",  0x10000, 0x10000, CRC(dbb7c57e) SHA1(268132965cd65b5e972ca9d0258c30b8a86f3703) )
	ROM_LOAD( "b45-12.rom",  0x20000, 0x10000, CRC(cf5aae4e) SHA1(5832c52d2e9b86414d8ee2926fa190abe9e41da4) )
	ROM_LOAD( "b45-13.rom",  0x30000, 0x10000, CRC(1683b07c) SHA1(54356893357cd1297f24f1d85b7289d80740262d) )

	ROM_REGION( 0x240, REGION_PROMS, 0 )		/* nibble bproms, lo/hi order to be determined */
	ROM_LOAD( "b45-15.bpr",  0x000, 0x100, CRC(24e7d62f) SHA1(1c06a1ef1b6a722794ca1d5ee2c476ecaa5178a3) )	/* sprite priority control ?? */
	ROM_LOAD( "b45-16.bpr",  0x100, 0x100, CRC(a50cef09) SHA1(55cafb5b2551b80ae708e9b966cf37c70a16d310) )	/* sprite priority control ?? */
	ROM_LOAD( "b45-14.bpr",  0x200, 0x020, CRC(f72482db) SHA1(b0cb911f9c81f6088a5aa8760916ddae1f8534d7) )	/* sprite control ?? */
	ROM_LOAD( "b45-17.bpr",  0x220, 0x020, CRC(bc88cced) SHA1(5055362710c0f58823c05fb4c0e0eec638b91e3d) )	/* sprite attribute (flip/position) ?? */
ROM_END

ROM_START( truxton )
	ROM_REGION( 0x040000, REGION_CPU1, 0 )	/* Main 68K code */
	ROM_LOAD16_BYTE( "b65_11.bin",  0x000000, 0x20000, CRC(1a62379a) SHA1(b9470d4b70c38f2523b22636874d742abe4099eb) )
	ROM_LOAD16_BYTE( "b65_10.bin",  0x000001, 0x20000, CRC(aff5195d) SHA1(a7f379dc35e3acf9e7a8ae8a47a9b5b4193f93a1) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* Sound Z80 code */
	ROM_LOAD( "b65_09.bin",  0x0000, 0x8000, CRC(f1c0f410) SHA1(05deb759f8acb14fff92c56b536134cfd84516a8) )

	ROM_REGION( 0x80000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "b65_08.bin",  0x00000, 0x20000, CRC(d2315b37) SHA1(eb42a884df319728c830c067c2423043ed4536ee) )
	ROM_LOAD( "b65_07.bin",  0x20000, 0x20000, CRC(fb83252a) SHA1(48a38584d223f56286137f7acdfaec86ee6588e7) )
	ROM_LOAD( "b65_06.bin",  0x40000, 0x20000, CRC(36cedcbe) SHA1(f79d4b1e98b3c9091ae907fb671ad201d3698b42) )
	ROM_LOAD( "b65_05.bin",  0x60000, 0x20000, CRC(81cd95f1) SHA1(526a437fbe033ac21054ee5c3bf1ba2fed354c7a) )

	ROM_REGION( 0x80000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "b65_04.bin",  0x00000, 0x20000, CRC(8c6ff461) SHA1(5199e31f4eb23bad01f7d1079f3618fe39d8a32e) )
	ROM_LOAD( "b65_03.bin",  0x20000, 0x20000, CRC(58b1350b) SHA1(7eb2fe329579a6f651d3c1aed9155ac6ffefbc4b) )
	ROM_LOAD( "b65_02.bin",  0x40000, 0x20000, CRC(1dd55161) SHA1(c537456ac56801dea0ac48fb1389228530d00a61) )
	ROM_LOAD( "b65_01.bin",  0x60000, 0x20000, CRC(e974937f) SHA1(ab282472c04ce6d9ed368956c427403275bc9080) )

	ROM_REGION( 0x40, REGION_PROMS, 0 )		/* nibble bproms, lo/hi order to be determined */
	ROM_LOAD( "b65_12.bpr",  0x00, 0x20, CRC(bc88cced) SHA1(5055362710c0f58823c05fb4c0e0eec638b91e3d) )	/* sprite attribute (flip/position) ?? */
	ROM_LOAD( "b65_13.bpr",  0x20, 0x20, CRC(a1e17492) SHA1(9ddec4c97f2d541f69f3c32c47aaa21fd9699ae2) )	/* ??? */
ROM_END

ROM_START( hellfire )
	ROM_REGION( 0x040000, REGION_CPU1, 0 )	/* Main 68K code */
	ROM_LOAD16_BYTE( "b90_14.0",   0x000000, 0x20000, CRC(101df9f5) SHA1(27e1430d4c96fe2c830143999a760470c8381ada) )
	ROM_LOAD16_BYTE( "b90_15.1",   0x000001, 0x20000, CRC(e67fd452) SHA1(baec2a702238f000d0499705d79d7c7577fc2279) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* Sound Z80 code */
	ROM_LOAD( "b90_03.2",   0x0000, 0x8000, CRC(4058fa67) SHA1(155c364273c270cd74955f447efc804bb4c9b560) )

	ROM_REGION( 0x80000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "b90_04.3",   0x00000, 0x20000, CRC(ea6150fc) SHA1(1116947d10ce14fbc6a3b86368fc2024c6f51803) )
	ROM_LOAD( "b90_05.4",   0x20000, 0x20000, CRC(bb52c507) SHA1(b0b1821476647f10c7023f92a66a7f54b92f50c3) )
	ROM_LOAD( "b90_06.5",   0x40000, 0x20000, CRC(cf5b0252) SHA1(e2102967af61afb11d2290a40d13d2faf9ef1e12) )
	ROM_LOAD( "b90_07.6",   0x60000, 0x20000, CRC(b98af263) SHA1(54d636a50a41dbb58b54c22dfab3eabfdb452575) )

	ROM_REGION( 0x80000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "b90_11.10",  0x00000, 0x20000, CRC(c33e543c) SHA1(b85cba30cc651f820aeedd41e04584df92078ed9) )
	ROM_LOAD( "b90_10.9",   0x20000, 0x20000, CRC(35fd1092) SHA1(5e136a35eea45034ccd4aea52cc0ffeec944e27e) )
	ROM_LOAD( "b90_09.8",   0x40000, 0x20000, CRC(cf01009e) SHA1(e260c479fa97f23a65c220e5071aaf2dc2baf46d) )
	ROM_LOAD( "b90_08.7",   0x60000, 0x20000, CRC(3404a5e3) SHA1(f717b9e31c2a093dbb060b8ea54a8c3f52688d7a) )

	ROM_REGION( 0x40, REGION_PROMS, 0 )		/* nibble bproms, lo/hi order to be determined */
	ROM_LOAD( "3w.bpr",     0x00, 0x20, CRC(bc88cced) SHA1(5055362710c0f58823c05fb4c0e0eec638b91e3d) )	/* sprite attribute (flip/position) ?? */
	ROM_LOAD( "6b.bpr",     0x20, 0x20, CRC(a1e17492) SHA1(9ddec4c97f2d541f69f3c32c47aaa21fd9699ae2) )	/* ??? */
ROM_END

ROM_START( hellfir1 )
	ROM_REGION( 0x040000, REGION_CPU1, 0 )	/* Main 68K code */
	ROM_LOAD16_BYTE( "b90_14x.0",   0x000000, 0x20000, CRC(a3141ea5) SHA1(9b456cb908e193198110a628d98567a3b8351591) )
	ROM_LOAD16_BYTE( "b90_15x.1",   0x000001, 0x20000, CRC(e864daf4) SHA1(382f02df8419310cef5d7fb68a9376eeac2f3685) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* Sound Z80 code */
	ROM_LOAD( "b90_03x.2",  0x0000, 0x8000, CRC(f58c368f) SHA1(2ee5396a4b70a3374f3a3bbd791b1d962f6a8a52) )

	ROM_REGION( 0x80000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "b90_04.3",   0x00000, 0x20000, CRC(ea6150fc) SHA1(1116947d10ce14fbc6a3b86368fc2024c6f51803) )
	ROM_LOAD( "b90_05.4",   0x20000, 0x20000, CRC(bb52c507) SHA1(b0b1821476647f10c7023f92a66a7f54b92f50c3) )
	ROM_LOAD( "b90_06.5",   0x40000, 0x20000, CRC(cf5b0252) SHA1(e2102967af61afb11d2290a40d13d2faf9ef1e12) )
	ROM_LOAD( "b90_07.6",   0x60000, 0x20000, CRC(b98af263) SHA1(54d636a50a41dbb58b54c22dfab3eabfdb452575) )

	ROM_REGION( 0x80000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "b90_11.10",  0x00000, 0x20000, CRC(c33e543c) SHA1(b85cba30cc651f820aeedd41e04584df92078ed9) )
	ROM_LOAD( "b90_10.9",   0x20000, 0x20000, CRC(35fd1092) SHA1(5e136a35eea45034ccd4aea52cc0ffeec944e27e) )
	ROM_LOAD( "b90_09.8",   0x40000, 0x20000, CRC(cf01009e) SHA1(e260c479fa97f23a65c220e5071aaf2dc2baf46d) )
	ROM_LOAD( "b90_08.7",   0x60000, 0x20000, CRC(3404a5e3) SHA1(f717b9e31c2a093dbb060b8ea54a8c3f52688d7a) )

	ROM_REGION( 0x40, REGION_PROMS, 0 )		/* nibble bproms, lo/hi order to be determined */
	ROM_LOAD( "3w.bpr",     0x00, 0x20, CRC(bc88cced) SHA1(5055362710c0f58823c05fb4c0e0eec638b91e3d) )	/* sprite attribute (flip/position) ?? */
	ROM_LOAD( "6b.bpr",     0x20, 0x20, CRC(a1e17492) SHA1(9ddec4c97f2d541f69f3c32c47aaa21fd9699ae2) )	/* ??? */
ROM_END

ROM_START( zerowing )
	ROM_REGION( 0x080000, REGION_CPU1, 0 )	/* Main 68K code */
	ROM_LOAD16_BYTE( "o15-11.rom",  0x000000, 0x08000, CRC(6ff2b9a0) SHA1(c9f2a631f185689dfc42a451d85fac23c2f4b64b) )
	ROM_LOAD16_BYTE( "o15-12.rom",  0x000001, 0x08000, CRC(9773e60b) SHA1(b733e9d38a233d010cc5ea41e7e61695082c3a22) )
	ROM_LOAD16_BYTE( "o15-09.rom",  0x040000, 0x20000, CRC(13764e95) SHA1(61da49b73ba81edd951e96e9ce6673c1c3bd65f2) )
	ROM_LOAD16_BYTE( "o15-10.rom",  0x040001, 0x20000, CRC(351ba71a) SHA1(937331549140506711b08252497cc0f2efa58268) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* Sound Z80 code */
	ROM_LOAD( "o15-13.rom",  0x0000, 0x8000, CRC(e7b72383) SHA1(ea1f6f33a86d14d58bd396fd46081462f00177d5) )

	ROM_REGION( 0x80000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "o15-05.rom",  0x00000, 0x20000, CRC(4e5dd246) SHA1(5366b4a6f3c900a4f57a6583b7399163a06f42d7) )
	ROM_LOAD( "o15-06.rom",  0x20000, 0x20000, CRC(c8c6d428) SHA1(76ee5bcb8f10fe201fc5c32697beee3de9d8b751) )
	ROM_LOAD( "o15-07.rom",  0x40000, 0x20000, CRC(efc40e99) SHA1(a04fad4197a7fb4787cd9bebf43e1d9b02b2f61b) )
	ROM_LOAD( "o15-08.rom",  0x60000, 0x20000, CRC(1b019eab) SHA1(c9569ca85696825142acc5cde9ac829e82b1ca1b) )

	ROM_REGION( 0x80000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "o15-03.rom",  0x00000, 0x20000, CRC(7f245fd3) SHA1(efbcb3663d4accc4f8128a8fee5475bc109bc17a) )
	ROM_LOAD( "o15-04.rom",  0x20000, 0x20000, CRC(0b1a1289) SHA1(ce6c06342392d11952873e3b1d6aea8dc02a551c) )
	ROM_LOAD( "o15-01.rom",  0x40000, 0x20000, CRC(70570e43) SHA1(acc9baec71b0930cb2f193677e0663efa5d5551d) )
	ROM_LOAD( "o15-02.rom",  0x60000, 0x20000, CRC(724b487f) SHA1(06af31520866eea69aebbd5d428f80e882289a15) )

	ROM_REGION( 0x40, REGION_PROMS, 0 )		/* nibble bproms, lo/hi order to be determined */
	ROM_LOAD( "tp015_14.bpr",  0x00, 0x20, CRC(bc88cced) SHA1(5055362710c0f58823c05fb4c0e0eec638b91e3d) )	/* sprite attribute (flip/position) ?? */
	ROM_LOAD( "tp015_15.bpr",  0x20, 0x20, CRC(a1e17492) SHA1(9ddec4c97f2d541f69f3c32c47aaa21fd9699ae2) )	/* ??? */
ROM_END

ROM_START( demonwld )
	ROM_REGION( 0x040000, REGION_CPU1, 0 )	/* Main 68K code */
	ROM_LOAD16_BYTE( "rom10.v2",  0x000000, 0x20000, CRC(ca8194f3) SHA1(176da6739b35ba38b40150fc62380108bcae5a24) )
	ROM_LOAD16_BYTE( "rom09.v2",  0x000001, 0x20000, CRC(7baea7ba) SHA1(ae2b40f9efb4440ff7edbcc4f80641655f7c4671) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* Sound Z80 code */
	ROM_LOAD( "rom11.v2",  0x0000, 0x8000, CRC(dbe08c85) SHA1(536a242bfe916d15744b079261507af6f12b5b50) )

	ROM_REGION( 0x10000, REGION_CPU3, 0 )	/* Co-Processor TMS320C10 MCU code */
	ROM_LOAD16_BYTE( "dsp_21.bin",  0x8000, 0x0800, CRC(2d135376) SHA1(67a2cc774d272ee1cd6e6bc1c5fc33fc6968837e) )
	ROM_LOAD16_BYTE( "dsp_22.bin",  0x8001, 0x0800, CRC(79389a71) SHA1(14ec4c1c9b06702319e89a7a250d0038393437f4) )

	ROM_REGION( 0x80000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "rom05",  0x00000, 0x20000, CRC(6506c982) SHA1(6d4c1ef91e5617724789ff196abb7abf23e4a7fb) )
	ROM_LOAD( "rom07",  0x20000, 0x20000, CRC(a3a0d993) SHA1(50311b9447eb04271b17b212ca31d083ab5b2414) )
	ROM_LOAD( "rom06",  0x40000, 0x20000, CRC(4fc5e5f3) SHA1(725d4b009d575ff8ffbe1c00df352ccf235465d7) )
	ROM_LOAD( "rom08",  0x60000, 0x20000, CRC(eb53ab09) SHA1(d98195cc1b65b76335b5b24adb31deae1b313f3a) )

	ROM_REGION( 0x80000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "rom01",  0x00000, 0x20000, CRC(1b3724e9) SHA1(3dbb0450ab1e40e6df2b7c7356352419cd3f113d) )
	ROM_LOAD( "rom02",  0x20000, 0x20000, CRC(7b20a44d) SHA1(4dc1a2fa2058077b112c73492808ee9381060ec7) )
	ROM_LOAD( "rom03",  0x40000, 0x20000, CRC(2cacdcd0) SHA1(92216d1c6859e05d39363c30e0beb45bc0ae4e1c) )
	ROM_LOAD( "rom04",  0x60000, 0x20000, CRC(76fd3201) SHA1(7a12737bf90bd9760074132edeb22f3fd3e16b4f) )

	ROM_REGION( 0x40, REGION_PROMS, 0 )		/* nibble bproms, lo/hi order to be determined */
	ROM_LOAD( "prom12.bpr",  0x00, 0x20, CRC(bc88cced) SHA1(5055362710c0f58823c05fb4c0e0eec638b91e3d) )	/* sprite attribute (flip/position) ?? */
	ROM_LOAD( "prom13.bpr",  0x20, 0x20, CRC(a1e17492) SHA1(9ddec4c97f2d541f69f3c32c47aaa21fd9699ae2) )	/* ??? */
ROM_END

ROM_START( demonwl1 )
	ROM_REGION( 0x040000, REGION_CPU1, 0 )	/* Main 68K code */
	ROM_LOAD16_BYTE( "rom10",  0x000000, 0x20000, CRC(036ee46c) SHA1(60868e5e08e0c9a538ae786de0de6b2531b30b11) )
	ROM_LOAD16_BYTE( "rom09",  0x000001, 0x20000, CRC(bed746e3) SHA1(056668edb7df99bbd240e387af17cf252d1448f3) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* Sound Z80 code */
	ROM_LOAD( "rom11",  0x0000, 0x8000, CRC(397eca1b) SHA1(84073ff6d1bc46ec6162d66ec5de305700938380) )

	ROM_REGION( 0x10000, REGION_CPU3, 0 )	/* Co-Processor TMS320C10 MCU code */
	ROM_LOAD16_BYTE( "dsp_21.bin",  0x8000, 0x0800, CRC(2d135376) SHA1(67a2cc774d272ee1cd6e6bc1c5fc33fc6968837e) )
	ROM_LOAD16_BYTE( "dsp_22.bin",  0x8001, 0x0800, CRC(79389a71) SHA1(14ec4c1c9b06702319e89a7a250d0038393437f4) )

	ROM_REGION( 0x80000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "rom05",  0x00000, 0x20000, CRC(6506c982) SHA1(6d4c1ef91e5617724789ff196abb7abf23e4a7fb) )
	ROM_LOAD( "rom07",  0x20000, 0x20000, CRC(a3a0d993) SHA1(50311b9447eb04271b17b212ca31d083ab5b2414) )
	ROM_LOAD( "rom06",  0x40000, 0x20000, CRC(4fc5e5f3) SHA1(725d4b009d575ff8ffbe1c00df352ccf235465d7) )
	ROM_LOAD( "rom08",  0x60000, 0x20000, CRC(eb53ab09) SHA1(d98195cc1b65b76335b5b24adb31deae1b313f3a) )

	ROM_REGION( 0x80000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "rom01",  0x00000, 0x20000, CRC(1b3724e9) SHA1(3dbb0450ab1e40e6df2b7c7356352419cd3f113d) )
	ROM_LOAD( "rom02",  0x20000, 0x20000, CRC(7b20a44d) SHA1(4dc1a2fa2058077b112c73492808ee9381060ec7) )
	ROM_LOAD( "rom03",  0x40000, 0x20000, CRC(2cacdcd0) SHA1(92216d1c6859e05d39363c30e0beb45bc0ae4e1c) )
	ROM_LOAD( "rom04",  0x60000, 0x20000, CRC(76fd3201) SHA1(7a12737bf90bd9760074132edeb22f3fd3e16b4f) )

	ROM_REGION( 0x40, REGION_PROMS, 0 )		/* nibble bproms, lo/hi order to be determined */
	ROM_LOAD( "prom12.bpr",  0x00, 0x20, CRC(bc88cced) SHA1(5055362710c0f58823c05fb4c0e0eec638b91e3d) )	/* sprite attribute (flip/position) ?? */
	ROM_LOAD( "prom13.bpr",  0x20, 0x20, CRC(a1e17492) SHA1(9ddec4c97f2d541f69f3c32c47aaa21fd9699ae2) )	/* ??? */
ROM_END

ROM_START( samesame )
	ROM_REGION( 0x080000, REGION_CPU1, 0 )	/* Main 68K code */
	ROM_LOAD16_BYTE( "o17_09.bin",  0x000000, 0x08000, CRC(3f69e437) SHA1(f2a40fd42cb5ecb2e514b72e7550aa479a9f9ad6) )
	ROM_LOAD16_BYTE( "o17_10.bin",  0x000001, 0x08000, CRC(4e723e0a) SHA1(e06394d50addeda1045c02c646964afbc6005a82) )
	ROM_LOAD16_BYTE( "o17_11.bin",  0x040000, 0x20000, CRC(be07d101) SHA1(1eda14ba24532b565d6ad57490b73ff312f98b53) )
	ROM_LOAD16_BYTE( "o17_12.bin",  0x040001, 0x20000, CRC(ef698811) SHA1(4c729704eba0bf469599c79009327e4fa5dc540b) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* Sound HD647180 code */
        ROM_LOAD( "hd647180.017", 0x00000, 0x08000, CRC(43523032) SHA1(1b94003a00e7bf6bdf1b1b946f42ff5d04629949) )

	ROM_REGION( 0x80000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "o17_05.bin",  0x00000, 0x20000, CRC(565315f8) SHA1(6b1c5ef52359483228b329c89c2e1174e3fbf017) )
	ROM_LOAD( "o17_06.bin",  0x20000, 0x20000, CRC(95262d4c) SHA1(16f3aabecb1c87ce7eadf4f0ff61b29a4c017614) )
	ROM_LOAD( "o17_07.bin",  0x40000, 0x20000, CRC(4c4b735c) SHA1(812c3bf46bd7764b2bb812bd2b9eb0331ed257ae) )
	ROM_LOAD( "o17_08.bin",  0x60000, 0x20000, CRC(95c6586c) SHA1(ff87901f79d80f73ad09664b0c0d892898570616) )

	ROM_REGION( 0x80000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "o17_01.bin",  0x00000, 0x20000, CRC(ea12e491) SHA1(02190722b7c5383471e0af9596be7039a5367240) )
	ROM_LOAD( "o17_02.bin",  0x20000, 0x20000, CRC(32a13a9f) SHA1(1446acdfd21cd41f3d97aaf30f498c0c5d890605) )
	ROM_LOAD( "o17_03.bin",  0x40000, 0x20000, CRC(68723dc9) SHA1(4f1b7aa2469c955e03737b611a7d2524f1e4f61e) )
	ROM_LOAD( "o17_04.bin",  0x60000, 0x20000, CRC(fe0ecb13) SHA1(634a49262b9c092c25f11b14c6757fe94ea9eddc) )

	ROM_REGION( 0x40, REGION_PROMS, 0 )		/* nibble bproms, lo/hi order to be determined */
	ROM_LOAD( "tp017_13.bpr",  0x00, 0x20, CRC(bc88cced) SHA1(5055362710c0f58823c05fb4c0e0eec638b91e3d) )	/* sprite attribute (flip/position) ?? */
	ROM_LOAD( "tp017_14.bpr",  0x20, 0x20, CRC(a1e17492) SHA1(9ddec4c97f2d541f69f3c32c47aaa21fd9699ae2) )	/* ??? */
ROM_END

ROM_START( samesam2 )
	ROM_REGION( 0x080000, REGION_CPU1, 0 )	/* Main 68K code */
	ROM_LOAD16_BYTE( "o17_09x.bin", 0x000000, 0x08000, CRC(3472e03e) SHA1(a0f12622a1963bfac2d5f357afbfb5d7db2cd8df) )
	ROM_LOAD16_BYTE( "o17_10x.bin", 0x000001, 0x08000, CRC(a3ac49b5) SHA1(c5adf026b9129b64acee5a079e102377a8488220) )
	ROM_LOAD16_BYTE( "o17_11x.bin", 0x040000, 0x20000, CRC(6beac378) SHA1(041ba98a89a4bac32575858db8a061bdf7804594) )
	ROM_LOAD16_BYTE( "o17_12x.bin", 0x040001, 0x20000, CRC(6adb6eb5) SHA1(9b6e63aa50d271c2bb0b4cf822fc6f3684f10230) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* Sound HD647180 code */
        ROM_LOAD( "hd647180.017", 0x00000, 0x08000, CRC(43523032) SHA1(1b94003a00e7bf6bdf1b1b946f42ff5d04629949) )

	ROM_REGION( 0x80000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "o17_05.bin",  0x00000, 0x20000, CRC(565315f8) SHA1(6b1c5ef52359483228b329c89c2e1174e3fbf017) )
	ROM_LOAD( "o17_06.bin",  0x20000, 0x20000, CRC(95262d4c) SHA1(16f3aabecb1c87ce7eadf4f0ff61b29a4c017614) )
	ROM_LOAD( "o17_07.bin",  0x40000, 0x20000, CRC(4c4b735c) SHA1(812c3bf46bd7764b2bb812bd2b9eb0331ed257ae) )
	ROM_LOAD( "o17_08.bin",  0x60000, 0x20000, CRC(95c6586c) SHA1(ff87901f79d80f73ad09664b0c0d892898570616) )

	ROM_REGION( 0x80000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "o17_01.bin",  0x00000, 0x20000, CRC(ea12e491) SHA1(02190722b7c5383471e0af9596be7039a5367240) )
	ROM_LOAD( "o17_02.bin",  0x20000, 0x20000, CRC(32a13a9f) SHA1(1446acdfd21cd41f3d97aaf30f498c0c5d890605) )
	ROM_LOAD( "o17_03.bin",  0x40000, 0x20000, CRC(68723dc9) SHA1(4f1b7aa2469c955e03737b611a7d2524f1e4f61e) )
	ROM_LOAD( "o17_04.bin",  0x60000, 0x20000, CRC(fe0ecb13) SHA1(634a49262b9c092c25f11b14c6757fe94ea9eddc) )

	ROM_REGION( 0x40, REGION_PROMS, 0 )		/* nibble bproms, lo/hi order to be determined */
	ROM_LOAD( "tp017_13.bpr",  0x00, 0x20, CRC(bc88cced) SHA1(5055362710c0f58823c05fb4c0e0eec638b91e3d) )	/* sprite attribute (flip/position) ?? */
	ROM_LOAD( "tp017_14.bpr",  0x20, 0x20, CRC(a1e17492) SHA1(9ddec4c97f2d541f69f3c32c47aaa21fd9699ae2) )	/* ??? */
ROM_END

ROM_START( fireshrk )
	ROM_REGION( 0x080000, REGION_CPU1, 0 )	/* Main 68K code */
	ROM_LOAD16_BYTE( "09.bin",      0x000000, 0x08000, CRC(f0c70e6f) SHA1(037690448786d61aa116b24b638430c577ea78e2) )
	ROM_LOAD16_BYTE( "10.bin",      0x000001, 0x08000, CRC(9d253d77) SHA1(0414d1f475abb9ccfd7daa11c2f400a14f25db09) )
	ROM_LOAD16_BYTE( "o17_11x.bin", 0x040000, 0x20000, CRC(6beac378) SHA1(041ba98a89a4bac32575858db8a061bdf7804594) )
	ROM_LOAD16_BYTE( "o17_12x.bin", 0x040001, 0x20000, CRC(6adb6eb5) SHA1(9b6e63aa50d271c2bb0b4cf822fc6f3684f10230) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* Sound HD647180 code */
        ROM_LOAD( "hd647180.017", 0x00000, 0x08000, CRC(43523032) SHA1(1b94003a00e7bf6bdf1b1b946f42ff5d04629949) )

	ROM_REGION( 0x80000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "o17_05.bin",  0x00000, 0x20000, CRC(565315f8) SHA1(6b1c5ef52359483228b329c89c2e1174e3fbf017) )
	ROM_LOAD( "o17_06.bin",  0x20000, 0x20000, CRC(95262d4c) SHA1(16f3aabecb1c87ce7eadf4f0ff61b29a4c017614) )
	ROM_LOAD( "o17_07.bin",  0x40000, 0x20000, CRC(4c4b735c) SHA1(812c3bf46bd7764b2bb812bd2b9eb0331ed257ae) )
	ROM_LOAD( "o17_08.bin",  0x60000, 0x20000, CRC(95c6586c) SHA1(ff87901f79d80f73ad09664b0c0d892898570616) )

	ROM_REGION( 0x80000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "o17_01.bin",  0x00000, 0x20000, CRC(ea12e491) SHA1(02190722b7c5383471e0af9596be7039a5367240) )
	ROM_LOAD( "o17_02.bin",  0x20000, 0x20000, CRC(32a13a9f) SHA1(1446acdfd21cd41f3d97aaf30f498c0c5d890605) )
	ROM_LOAD( "o17_03.bin",  0x40000, 0x20000, CRC(68723dc9) SHA1(4f1b7aa2469c955e03737b611a7d2524f1e4f61e) )
	ROM_LOAD( "o17_04.bin",  0x60000, 0x20000, CRC(fe0ecb13) SHA1(634a49262b9c092c25f11b14c6757fe94ea9eddc) )

	ROM_REGION( 0x40, REGION_PROMS, 0 )		/* nibble bproms, lo/hi order to be determined */
	ROM_LOAD( "tp017_13.bpr",  0x00, 0x20, CRC(bc88cced) SHA1(5055362710c0f58823c05fb4c0e0eec638b91e3d) )	/* sprite attribute (flip/position) ?? */
	ROM_LOAD( "tp017_14.bpr",  0x20, 0x20, CRC(a1e17492) SHA1(9ddec4c97f2d541f69f3c32c47aaa21fd9699ae2) )	/* ??? */
ROM_END

ROM_START( outzone )
	ROM_REGION( 0x040000, REGION_CPU1, 0 )	/* Main 68K code */
	ROM_LOAD16_BYTE( "rom7.bin",  0x000000, 0x20000, CRC(936e25d8) SHA1(ffb7990ea1539d868a9ad2fb711b0febd90f098d) )
	ROM_LOAD16_BYTE( "rom8.bin",  0x000001, 0x20000, CRC(d19b3ecf) SHA1(b406999b9f1e2104d958b42cc745bf79dbfe50b3) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* Sound Z80 code */
	ROM_LOAD( "rom9.bin",  0x0000, 0x8000, CRC(73d8e235) SHA1(f37ad497259a467cdf2ec8b3e6e7d3e873087e6c) )

	ROM_REGION( 0x100000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "rom5.bin",  0x00000, 0x80000, CRC(c64ec7b6) SHA1(e73b51c3713c2ea7a572a02531c15d1261ddeaa0) )
	ROM_LOAD( "rom6.bin",  0x80000, 0x80000, CRC(64b6c5ac) SHA1(07fa20115f603445c0d51af3465c0471c09d76b1) )

	ROM_REGION( 0x80000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "rom2.bin",  0x00000, 0x20000, CRC(6bb72d16) SHA1(a127b10d9c255542bd09fcb5df057c12fd28c0d1) )
	ROM_LOAD( "rom1.bin",  0x20000, 0x20000, CRC(0934782d) SHA1(e4a775ead23227d7d6e76aea23aa3103b511d031) )
	ROM_LOAD( "rom3.bin",  0x40000, 0x20000, CRC(ec903c07) SHA1(75906f31200877fc8f6e78c2606ad5be49778165) )
	ROM_LOAD( "rom4.bin",  0x60000, 0x20000, CRC(50cbf1a8) SHA1(cfab1504746654b4a61912155e9aeca746c65321) )

	ROM_REGION( 0x40, REGION_PROMS, 0 )		/* nibble bproms, lo/hi order to be determined */
	ROM_LOAD( "tp018_10.bpr",  0x00, 0x20, CRC(bc88cced) SHA1(5055362710c0f58823c05fb4c0e0eec638b91e3d) )	/* sprite attribute (flip/position) ?? */
	ROM_LOAD( "tp018_11.bpr",  0x20, 0x20, CRC(a1e17492) SHA1(9ddec4c97f2d541f69f3c32c47aaa21fd9699ae2) )	/* ??? */
ROM_END

ROM_START( outzonea )					/* From board serial number 2122 */
	ROM_REGION( 0x040000, REGION_CPU1, 0 )	/* Main 68K code */
	ROM_LOAD16_BYTE( "18.bin",  0x000000, 0x20000, CRC(31a171bb) SHA1(4ee707e758ab21d2809b65daf0081f86bd9328d9) )
	ROM_LOAD16_BYTE( "19.bin",  0x000001, 0x20000, CRC(804ecfd1) SHA1(7dead8064445c6d44ebd0889583deb5e17b1954a) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* Sound Z80 code */
	ROM_LOAD( "rom9.bin",  0x0000, 0x8000, CRC(73d8e235) SHA1(f37ad497259a467cdf2ec8b3e6e7d3e873087e6c) )

	ROM_REGION( 0x100000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "rom5.bin",  0x00000, 0x80000, CRC(c64ec7b6) SHA1(e73b51c3713c2ea7a572a02531c15d1261ddeaa0) )
	ROM_LOAD( "rom6.bin",  0x80000, 0x80000, CRC(64b6c5ac) SHA1(07fa20115f603445c0d51af3465c0471c09d76b1) )
/* a pirate board exists using the same data in a different layout
	ROM_LOAD16_BYTE( "04.bin",  0x000000, 0x10000, CRC(3d11eae0) )
	ROM_LOAD16_BYTE( "08.bin",  0x000001, 0x10000, CRC(c7628891) )
	ROM_LOAD16_BYTE( "13.bin",  0x080000, 0x10000, CRC(b23dd87e) )
	ROM_LOAD16_BYTE( "09.bin",  0x080001, 0x10000, CRC(445651ba) )
	ROM_LOAD16_BYTE( "03.bin",  0x020000, 0x10000, CRC(6b347646) )
	ROM_LOAD16_BYTE( "07.bin",  0x020001, 0x10000, CRC(461b47f9) )
	ROM_LOAD16_BYTE( "14.bin",  0x0a0000, 0x10000, CRC(b28ae37a) )
	ROM_LOAD16_BYTE( "10.bin",  0x0a0001, 0x10000, CRC(6596a076) )
	ROM_LOAD16_BYTE( "02.bin",  0x040000, 0x10000, CRC(11a781c3) )
	ROM_LOAD16_BYTE( "06.bin",  0x040001, 0x10000, CRC(1055da17) )
	ROM_LOAD16_BYTE( "15.bin",  0x0c0000, 0x10000, CRC(9c9e811b) )
	ROM_LOAD16_BYTE( "11.bin",  0x0c0001, 0x10000, CRC(4c4d44dc) )
	ROM_LOAD16_BYTE( "01.bin",  0x060000, 0x10000, CRC(e8c46aea) )
	ROM_LOAD16_BYTE( "05.bin",  0x060001, 0x10000, CRC(f8a2fe01) )
	ROM_LOAD16_BYTE( "16.bin",  0x0e0000, 0x10000, CRC(cffcb99b) )
	ROM_LOAD16_BYTE( "12.bin",  0x0e0001, 0x10000, CRC(90d37ded) )
*/

	ROM_REGION( 0x80000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "rom2.bin",  0x00000, 0x20000, CRC(6bb72d16) SHA1(a127b10d9c255542bd09fcb5df057c12fd28c0d1) )
	ROM_LOAD( "rom1.bin",  0x20000, 0x20000, CRC(0934782d) SHA1(e4a775ead23227d7d6e76aea23aa3103b511d031) )
	ROM_LOAD( "rom3.bin",  0x40000, 0x20000, CRC(ec903c07) SHA1(75906f31200877fc8f6e78c2606ad5be49778165) )
	ROM_LOAD( "rom4.bin",  0x60000, 0x20000, CRC(50cbf1a8) SHA1(cfab1504746654b4a61912155e9aeca746c65321) )

	ROM_REGION( 0x40, REGION_PROMS, 0 )		/* nibble bproms, lo/hi order to be determined */
	ROM_LOAD( "tp018_10.bpr",  0x00, 0x20, CRC(bc88cced) SHA1(5055362710c0f58823c05fb4c0e0eec638b91e3d) )	/* sprite attribute (flip/position) ?? */
	ROM_LOAD( "tp018_11.bpr",  0x20, 0x20, CRC(a1e17492) SHA1(9ddec4c97f2d541f69f3c32c47aaa21fd9699ae2) )	/* ??? */
ROM_END

ROM_START( vimana )			/* From board serial number 1547.04 (July '94) */
	ROM_REGION( 0x040000, REGION_CPU1, 0 )	/* Main 68K code */
	ROM_LOAD16_BYTE( "tp019-7a.bin",  0x000000, 0x20000, CRC(5a4bf73e) SHA1(9a43d822bc24b59278f294d0b3275595de997d16) )
	ROM_LOAD16_BYTE( "tp019-8a.bin",  0x000001, 0x20000, CRC(03ba27e8) SHA1(edb5fe741d2a6a7fe5cde9a82317ea1e9447cf73) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* Sound HD647180 code */
	/* sound CPU is a HD647180 (Z180) with internal ROM */
	ROM_LOAD( "hd647180.019", 0x00000, 0x08000, CRC(41a97ebe) SHA1(9b377086e4d9b8de6e3c8c7d2dd099b80ab88934) )

	ROM_REGION( 0x80000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "vim6.bin",  0x00000, 0x20000, CRC(2886878d) SHA1(f44933d87bbcd3bd58f46e0f0f89b05c409b713b) )
	ROM_LOAD( "vim5.bin",  0x20000, 0x20000, CRC(61a63d7a) SHA1(5cdebc03110252cc43d31b6f87f9a23556892977) )
	ROM_LOAD( "vim4.bin",  0x40000, 0x20000, CRC(b0515768) SHA1(9907b52b4d30ce5324270a12c40250068adafca8) )
	ROM_LOAD( "vim3.bin",  0x60000, 0x20000, CRC(0b539131) SHA1(07f3e3b9b28c8218e36668c24d16dbb6e9a66889) )

	ROM_REGION( 0x100000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "vim1.bin",  0x00000, 0x80000, CRC(cdde26cd) SHA1(27893af4692ec7bcbaac9e790c0707c98df84e62) )
	ROM_LOAD( "vim2.bin",  0x80000, 0x80000, CRC(1dbfc118) SHA1(4fd039a3172f73ad910349b2d360e8ae77ccddb2) )

	ROM_REGION( 0x40, REGION_PROMS, 0 )		/* nibble bproms, lo/hi order to be determined */
	ROM_LOAD( "tp019-09.bpr",  0x00, 0x20, CRC(bc88cced) SHA1(5055362710c0f58823c05fb4c0e0eec638b91e3d) )	/* sprite attribute (flip/position) ?? */
	ROM_LOAD( "tp019-10.bpr",  0x20, 0x20, CRC(a1e17492) SHA1(9ddec4c97f2d541f69f3c32c47aaa21fd9699ae2) )	/* ??? */
ROM_END

ROM_START( vimana1 )
	ROM_REGION( 0x040000, REGION_CPU1, 0 )	/* Main 68K code */
	ROM_LOAD16_BYTE( "vim07.bin",  0x000000, 0x20000, CRC(1efaea84) SHA1(f9c5d2365d8948fa66dbe61d355919db15843a28) )
	ROM_LOAD16_BYTE( "vim08.bin",  0x000001, 0x20000, CRC(e45b7def) SHA1(6b92a91d64581954da8ecdbeb5fed79bcc9c5217) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* Sound HD647180 code */
	/* sound CPU is a HD647180 (Z180) with internal ROM */
	ROM_LOAD( "hd647180.019", 0x00000, 0x08000, CRC(41a97ebe) SHA1(9b377086e4d9b8de6e3c8c7d2dd099b80ab88934) )

	ROM_REGION( 0x80000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "vim6.bin",  0x00000, 0x20000, CRC(2886878d) SHA1(f44933d87bbcd3bd58f46e0f0f89b05c409b713b) )
	ROM_LOAD( "vim5.bin",  0x20000, 0x20000, CRC(61a63d7a) SHA1(5cdebc03110252cc43d31b6f87f9a23556892977) )
	ROM_LOAD( "vim4.bin",  0x40000, 0x20000, CRC(b0515768) SHA1(9907b52b4d30ce5324270a12c40250068adafca8) )
	ROM_LOAD( "vim3.bin",  0x60000, 0x20000, CRC(0b539131) SHA1(07f3e3b9b28c8218e36668c24d16dbb6e9a66889) )

	ROM_REGION( 0x100000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "vim1.bin",  0x00000, 0x80000, CRC(cdde26cd) SHA1(27893af4692ec7bcbaac9e790c0707c98df84e62) )
	ROM_LOAD( "vim2.bin",  0x80000, 0x80000, CRC(1dbfc118) SHA1(4fd039a3172f73ad910349b2d360e8ae77ccddb2) )

	ROM_REGION( 0x40, REGION_PROMS, 0 )		/* nibble bproms, lo/hi order to be determined */
	ROM_LOAD( "tp019-09.bpr",  0x00, 0x20, CRC(bc88cced) SHA1(5055362710c0f58823c05fb4c0e0eec638b91e3d) )	/* sprite attribute (flip/position) ?? */
	ROM_LOAD( "tp019-10.bpr",  0x20, 0x20, CRC(a1e17492) SHA1(9ddec4c97f2d541f69f3c32c47aaa21fd9699ae2) )	/* ??? */
ROM_END

ROM_START( vimanan )
	ROM_REGION( 0x040000, REGION_CPU1, 0 )	/* Main 68K code */
	ROM_LOAD16_BYTE( "tp019-07.rom",  0x000000, 0x20000, CRC(78888ff2) SHA1(7e1d248f806d585952eb35ceec6a7e63ae4e22f9) )
	ROM_LOAD16_BYTE( "tp019-08.rom",  0x000001, 0x20000, CRC(6cd2dc3c) SHA1(029d974eb938c5e2fbe7575f0dda342b4b12b731) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* Sound HD647180 code */
	/* sound CPU is a HD647180 (Z180) with internal ROM */
	ROM_LOAD( "hd647180.019", 0x00000, 0x08000, CRC(41a97ebe) SHA1(9b377086e4d9b8de6e3c8c7d2dd099b80ab88934) )

	ROM_REGION( 0x80000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "vim6.bin",  0x00000, 0x20000, CRC(2886878d) SHA1(f44933d87bbcd3bd58f46e0f0f89b05c409b713b) )
	ROM_LOAD( "vim5.bin",  0x20000, 0x20000, CRC(61a63d7a) SHA1(5cdebc03110252cc43d31b6f87f9a23556892977) )
	ROM_LOAD( "vim4.bin",  0x40000, 0x20000, CRC(b0515768) SHA1(9907b52b4d30ce5324270a12c40250068adafca8) )
	ROM_LOAD( "vim3.bin",  0x60000, 0x20000, CRC(0b539131) SHA1(07f3e3b9b28c8218e36668c24d16dbb6e9a66889) )

	ROM_REGION( 0x100000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "vim1.bin",  0x00000, 0x80000, CRC(cdde26cd) SHA1(27893af4692ec7bcbaac9e790c0707c98df84e62) )
	ROM_LOAD( "vim2.bin",  0x80000, 0x80000, CRC(1dbfc118) SHA1(4fd039a3172f73ad910349b2d360e8ae77ccddb2) )

	ROM_REGION( 0x40, REGION_PROMS, 0 )		/* nibble bproms, lo/hi order to be determined */
	ROM_LOAD( "tp019-09.bpr",  0x00, 0x20, CRC(bc88cced) SHA1(5055362710c0f58823c05fb4c0e0eec638b91e3d) )	/* sprite attribute (flip/position) ?? */
	ROM_LOAD( "tp019-10.bpr",  0x20, 0x20, CRC(a1e17492) SHA1(9ddec4c97f2d541f69f3c32c47aaa21fd9699ae2) )	/* ??? */
ROM_END



GAME ( 1988, rallybik, 0,        rallybik, rallybik, 0, ROT270, "[Toaplan] Taito Corporation", "Rally Bike - Dash Yarou" )
GAME ( 1988, truxton,  0,        truxton,  truxton,  0, ROT270, "[Toaplan] Taito Corporation", "Truxton - Tatsujin" )
GAME ( 1989, hellfire, 0,        hellfire, hellfire, 0, ROT0,   "Toaplan (Taito license)", "Hellfire" )
GAME ( 1989, hellfir1, hellfire, hellfire, hellfir1, 0, ROT0,   "Toaplan (Taito license)", "Hellfire (1P Ver.)" )
GAME ( 1989, zerowing, 0,        zerowing, zerowing, 0, ROT0,   "Toaplan", "Zero Wing" )
GAME ( 1990, demonwld, 0,        demonwld, demonwld, 0, ROT0,   "Toaplan", "Demon's World - Horror Story" )
GAME ( 1989, demonwl1, demonwld, demonwld, demonwl1, 0, ROT0,   "Toaplan (Taito license)", "Demon's World - Horror Story (Taito license)" )
GAME ( 1990, fireshrk, 0,        samesame, fireshrk, 0, ROT270, "Toaplan", "Fire Shark" )
GAME ( 1989, samesame, fireshrk, samesame, samesame, 0, ROT270, "Toaplan", "Same! Same! Same!" )
GAME ( 1989, samesam2, fireshrk, samesame, samesam2, 0, ROT270, "Toaplan", "Same! Same! Same! (2P Ver.)" )
GAME ( 1990, outzone,  0,        outzone,  outzone,  0, ROT270, "Toaplan", "Out Zone (set 1)" )
GAME ( 1990, outzonea, outzone,  outzone,  outzone,  0, ROT270, "Toaplan", "Out Zone (set 2)" )
GAME ( 1991, vimana,   0,        vimana,   vimana,   0, ROT270, "Toaplan", "Vimana" )
GAME ( 1991, vimana1,  vimana,   vimana,   vimana,   0, ROT270, "Toaplan", "Vimana (old set)" )
GAME ( 1991, vimanan,  vimana,   vimana,   vimanan,  0, ROT270, "Toaplan (Nova Apparate GMBH and Co license)", "Vimana (Nova Apparate GMBH and Co)" )
