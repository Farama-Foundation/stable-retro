/***************************************************************************

  Tumblepop (World)     (c) 1991 Data East Corporation
  Tumblepop (Japan)     (c) 1991 Data East Corporation

  the following don't run on the same phyical board but many features of the
  hardware are identical

  Tumblepop             (c) 1991 Data East Corporation (Bootleg 1)
  Tumblepop             (c) 1991 Data East Corporation (Bootleg 2)
  Jump Kids             (c) 1993 Comad
  Fancy World           (c) 1995 Unico
  Hatch Catch			(c) 1995 SemiCom
  Choky! Choky!         (c) 1995 SemiCom
  Cookie & Bibi[2]      (c) 1995 SemiCom
  B.C. Story            (c) 1997 SemiCom
  S.D. Fighter          (c) 1996 SemiCom
  Super Trio[1]         (c) 1994 GameAce

  [1] has the same sprites as the bootlegs, not much else is the same tho

  [2] test mode crashes the same way on the real board

  Bootleg sound is not quite correct yet (Nothing on bootleg 2).

  If you reset the game while pressing START1 and START2, "VER 0.00 JAPAN"
  is put into tile ram then MAME crashes !

  One of the Jump Kids Sprite roms is bad, same with
  the Sound CPU code, there's one unknown ROM.

  Sometimes a garbage sprite gets left after the SemiCom logo in Hatch
  Catch - other semicom games on different hw do this too, might just
  be a bug in their code

  Emulation by Bryan McPhail, mish@tendril.co.uk
  Jumping Pop sound emulation by R. Belmont


Stephh's notes (based on the games M68000 code and some tests) :

1) 'tumblep*' and 'jumpkids'

  - I don't understand the interest of the "Remove Monsters" Dip Switch :
    as I haven't found a way to "end" a level, I guess that it was used to
    test the backgrounds and the "platforms".

  - The "Edit Levels" Dip Switch allows you to add/delete monsters and
    change their position.

    Notes (for 'tumblep', 'tumblepj', 'tumblep2') :
      * "worlds" and levels are 0-based (00-09 & 00-09) :

          World      Name
            0      America
            1      Brazil
            2      Asia
            3      Soviet
            4      Europe
            5      Egypt
            6      Australia
            7      Antartica
            8      Stratosphere
            9      Space

      * As levels x-9 and 9-x are only constitued of a "big boss", you can't
        edit them !
      * All data is stored within the range 0x02b8c8-0x02d2c9, but it should be
        extended to 0x02ebeb (and perhaps 0x02ffff). TO BE CONFIRMED !
      * Once your levels are ready, turn the Dip Switch OFF and reset the game.
      * Of course, there is no possibility to save the levels when you exit
        MAME, nor the way to reload the default ones 8(

    Additional notes (for 'tumblepb') :
      * All data is stored within the range 0x02b8c8-0x02d2c9, but it should be
        extended to 0x02ebeb (and perhaps 0x02ebff). TO BE CONFIRMED !

    Additional notes (for 'jumpkids') :
      * As there are only 9 "worlds", editing "world" 9 ("Space") might cause
        unpredictable weird results !
      * The "worlds" names are the same, but the background is different :

          World      Name            Background
            0      America         Stadium
            1      Brazil          Beach
            2      Asia            Planet
            3      Soviet          Prehistoric Ages
            4      Europe          Castle
            5      Egypt           Pyramids
            6      Australia       Lunar base
            7      Antartica       Bridge
            8      Stratosphere    ???
            9      Space           DOES NOT EXIST !

        As I'm not sure of the description of the background, feel free to
        improve the previous list.
      * All data is stored within the range 0x02776e-0x029207, but it should be
        extended to 0x02ab29 (and perhaps 0x02ab49). TO BE CONFIRMED !


2) 'fncywrld'

  - I'm not sure about the release date of this game :
      * on the title screen, it ALWAYS displays 1996
      * when "Language" Dip Switch is set to "English", there is a (c) 1996 "warning"
        screen, but when it is set to "Korean", there is a (c) 1995 "warning" screen !

  - I don't understand the interest of the "Remove Monsters" Dip Switch :
    as I haven't found a way to "end" a level, I guess that it was used to
    test the backgrounds and the "platforms".

  - The "Edit Levels" Dip Switch allows you to add/delete monsters and
    change their position.

    This needs more investigation to get similar infos to the ones for the other
    games in the driver.


***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "cpu/h6280/h6280.h"
#include "decocrpt.h"


#define TUMBLEP_HACK	0
#define FNCYWLD_HACK	0

VIDEO_START( tumblep );
VIDEO_START( fncywld );
VIDEO_START( jumppop );
VIDEO_UPDATE( tumblep );
VIDEO_UPDATE( tumblepb );
VIDEO_UPDATE( jumpkids );
VIDEO_UPDATE( fncywld );
VIDEO_UPDATE( jumppop );
VIDEO_UPDATE( semicom );
VIDEO_UPDATE( semicom_altoffsets );
VIDEO_UPDATE( bcstory );
VIDEO_START( suprtrio );
VIDEO_UPDATE( suprtrio );
VIDEO_UPDATE( sdfight );

WRITE16_HANDLER( tumblep_pf1_data_w );
WRITE16_HANDLER( tumblep_pf2_data_w );
WRITE16_HANDLER( fncywld_pf1_data_w );
WRITE16_HANDLER( fncywld_pf2_data_w );
WRITE16_HANDLER( tumblep_control_0_w );
WRITE16_HANDLER( semicom_soundcmd_w );

extern WRITE16_HANDLER( bcstory_tilebank_w );
extern WRITE16_HANDLER( suprtrio_tilebank_w );
extern WRITE16_HANDLER( chokchok_tilebank_w );

extern UINT16 *tumblep_pf1_data,*tumblep_pf2_data;
UINT16* tumblep_mainram;
UINT16* jumppop_control;
UINT16* suprtrio_control;

/* magipur */
static data16_t *mainram;
static data16_t *maincpu;

/******************************************************************************/

static WRITE16_HANDLER( tumblep_oki_w )
{
	OKIM6295_data_0_w(0,data&0xff);
    /* STUFF IN OTHER BYTE TOO..*/
}

static READ16_HANDLER( tumblep_prot_r )
{
	return ~0;
}

static WRITE16_HANDLER( tumblep_sound_w )
{
	soundlatch_w(0,data & 0xff);
	cpu_set_irq_line(1,0,HOLD_LINE);
}

static WRITE16_HANDLER( jumppop_sound_w )
{
	soundlatch_w(0,data & 0xff);
	cpu_set_irq_line( 1, 0, ASSERT_LINE );
}

/******************************************************************************/

static READ16_HANDLER( tumblepop_controls_r )
{
 	switch (offset<<1)
	{
		case 0: /* Player 1 & Player 2 joysticks & fire buttons */
			return (readinputport(0) + (readinputport(1) << 8));
		case 2: /* Dips */
			return (readinputport(3) + (readinputport(4) << 8));
		case 8: /* Credits */
			return readinputport(2);
		case 10: /* ? */
		case 12:
        	return 0;
	}

	return ~0;
}

/******************************************************************************/

static MEMORY_READ16_START( tumblepop_readmem )
	{ 0x000000, 0x07ffff, MRA16_ROM },
	{ 0x120000, 0x123fff, MRA16_RAM },
	{ 0x140000, 0x1407ff, MRA16_RAM },
	{ 0x180000, 0x18000f, tumblepop_controls_r },
	{ 0x1a0000, 0x1a07ff, MRA16_RAM },
	{ 0x320000, 0x320fff, MRA16_RAM },
	{ 0x322000, 0x322fff, MRA16_RAM },
MEMORY_END

static MEMORY_WRITE16_START( tumblepop_writemem )
#if TUMBLEP_HACK
	{ 0x000000, 0x07ffff, MWA16_RAM },	/* To write levels modifications */
#else
	{ 0x000000, 0x07ffff, MWA16_ROM },
#endif
	{ 0x100000, 0x100001, tumblep_sound_w },
	{ 0x120000, 0x123fff, MWA16_RAM },
	{ 0x140000, 0x1407ff, paletteram16_xxxxBBBBGGGGRRRR_word_w, &paletteram16 },
	{ 0x18000c, 0x18000d, MWA16_NOP },
	{ 0x1a0000, 0x1a07ff, MWA16_RAM, &spriteram16 },
	{ 0x300000, 0x30000f, tumblep_control_0_w },
	{ 0x320000, 0x320fff, tumblep_pf1_data_w, &tumblep_pf1_data },
	{ 0x322000, 0x322fff, tumblep_pf2_data_w, &tumblep_pf2_data },
	{ 0x340000, 0x3401ff, MWA16_NOP }, /* Unused row scroll */
	{ 0x340400, 0x34047f, MWA16_NOP }, /* Unused col scroll */
	{ 0x342000, 0x3421ff, MWA16_NOP },
	{ 0x342400, 0x34247f, MWA16_NOP },
MEMORY_END

static MEMORY_READ16_START( tumblepopb_readmem )
	{ 0x000000, 0x07ffff, MRA16_ROM },
	{ 0x100000, 0x100001, tumblep_prot_r },
	{ 0x120000, 0x123fff, MRA16_RAM },
	{ 0x140000, 0x1407ff, MRA16_RAM },
	{ 0x160000, 0x1607ff, MRA16_RAM },
	{ 0x180000, 0x18000f, tumblepop_controls_r },
	{ 0x1a0000, 0x1a07ff, MRA16_RAM },
MEMORY_END

static MEMORY_WRITE16_START( tumblepopb_writemem )
#if TUMBLEP_HACK
	{ 0x000000, 0x07ffff, MWA16_RAM },	/* To write levels modifications */
#else
	{ 0x000000, 0x07ffff, MWA16_ROM },
#endif
	{ 0x100000, 0x100001, tumblep_oki_w },
	{ 0x120000, 0x123fff, MWA16_RAM },
	{ 0x140000, 0x1407ff, paletteram16_xxxxBBBBGGGGRRRR_word_w, &paletteram16 },
	{ 0x160000, 0x1607ff, MWA16_RAM, &spriteram16 }, /* Bootleg sprite buffer */
	{ 0x18000c, 0x18000d, MWA16_NOP },
	{ 0x1a0000, 0x1a07ff, MWA16_RAM },
	{ 0x300000, 0x30000f, tumblep_control_0_w },
	{ 0x320000, 0x320fff, tumblep_pf1_data_w, &tumblep_pf1_data },
	{ 0x322000, 0x322fff, tumblep_pf2_data_w, &tumblep_pf2_data },
	{ 0x340000, 0x3401ff, MWA16_NOP }, /* Unused row scroll */
	{ 0x340400, 0x34047f, MWA16_NOP }, /* Unused col scroll */
	{ 0x342000, 0x3421ff, MWA16_NOP },
	{ 0x342400, 0x34247f, MWA16_NOP },
MEMORY_END

static MEMORY_READ16_START( fncywld_readmem )
	{ 0x000000, 0x0fffff, MRA16_ROM },
	{ 0x100000, 0x100001, YM2151_status_port_0_lsb_r },
	{ 0x100002, 0x100003, MRA16_NOP }, /* ym? */
	{ 0x100004, 0x100005, OKIM6295_status_0_lsb_r },
	{ 0x140000, 0x140fff, MRA16_RAM },
	{ 0x160000, 0x1607ff, MRA16_RAM },
	{ 0x180000, 0x18000f, tumblepop_controls_r },
	{ 0x320000, 0x321fff, MRA16_RAM },
	{ 0x322000, 0x323fff, MRA16_RAM },
	{ 0x1a0000, 0x1a07ff, MRA16_RAM },
	{ 0xff0000, 0xffffff, MRA16_RAM }, /* RAM */
MEMORY_END

static MEMORY_WRITE16_START( fncywld_writemem )
#if FNCYWLD_HACK
	{ 0x000000, 0x0fffff, MWA16_RAM },	/* To write levels modifications */
#else
	{ 0x000000, 0x0fffff, MWA16_ROM },
#endif
	{ 0x100000, 0x100001, YM2151_register_port_0_lsb_w },
	{ 0x100002, 0x100003, YM2151_data_port_0_lsb_w },
	{ 0x100004, 0x100005, OKIM6295_data_0_lsb_w },
	{ 0x140000, 0x140fff, paletteram16_xxxxRRRRGGGGBBBB_word_w, &paletteram16 },
	{ 0x160000, 0x1607ff, MWA16_RAM, &spriteram16 }, /* sprites */
	{ 0x160800, 0x16080f, MWA16_RAM }, /* goes slightly past the end of spriteram? */
	{ 0x18000c, 0x18000d, MWA16_NOP },
	{ 0x1a0000, 0x1a07ff, MWA16_RAM },
	{ 0x300000, 0x30000f, tumblep_control_0_w },
	{ 0x320000, 0x321fff, fncywld_pf1_data_w, &tumblep_pf1_data },
	{ 0x322000, 0x323fff, fncywld_pf2_data_w, &tumblep_pf2_data },
	{ 0x340000, 0x3401ff, MWA16_NOP }, /* Unused row scroll */
	{ 0x340400, 0x34047f, MWA16_NOP }, /* Unused col scroll */
	{ 0x342000, 0x3421ff, MWA16_NOP },
	{ 0x342400, 0x34247f, MWA16_NOP },
	{ 0xff0000, 0xffffff, MWA16_RAM }, /* RAM */
MEMORY_END

static MEMORY_READ16_START( magipur_readmem )
    { 0x000000, 0x00ffff, MRA16_RAM },
	{ 0xf00000, 0xffffff, MRA16_ROM },
	{ 0x100000, 0x100001, YM2151_status_port_0_lsb_r },
	{ 0x100002, 0x100003, MRA16_NOP }, /* ym? */
	{ 0x100004, 0x100005, OKIM6295_status_0_lsb_r },
	{ 0x140000, 0x140fff, MRA16_RAM },
	{ 0x160000, 0x1607ff, MRA16_RAM },
	{ 0x180000, 0x18000f, tumblepop_controls_r },
	{ 0x320000, 0x321fff, MRA16_RAM },
	{ 0x322000, 0x323fff, MRA16_RAM },
	{ 0x1a0000, 0x1a07ff, MRA16_RAM },
	{ 0xff0000, 0xffffff, MRA16_RAM }, /* RAM */
MEMORY_END

static MEMORY_WRITE16_START( magipur_writemem )
    { 0x000000, 0x00ffff, MWA16_RAM, &mainram },
	{ 0xf00000, 0xffffff, MWA16_ROM, &maincpu },
	{ 0x100000, 0x100001, YM2151_register_port_0_lsb_w },
	{ 0x100002, 0x100003, YM2151_data_port_0_lsb_w },
	{ 0x100004, 0x100005, OKIM6295_data_0_lsb_w },
	{ 0x100010, 0x100011, MWA16_NOP },
	{ 0x140000, 0x140fff, paletteram16_xxxxRRRRGGGGBBBB_word_w, &paletteram16 },
	{ 0x160000, 0x1607ff, MWA16_RAM, &spriteram16 }, /* sprites */
	{ 0x160800, 0x16080f, MWA16_RAM }, /* goes slightly past the end of spriteram? */
	{ 0x18000c, 0x18000d, MWA16_NOP },
	{ 0x1a0000, 0x1a07ff, MWA16_RAM },
	{ 0x300000, 0x30000f, tumblep_control_0_w },
	{ 0x320000, 0x321fff, fncywld_pf1_data_w, &tumblep_pf1_data },
	{ 0x322000, 0x323fff, fncywld_pf2_data_w, &tumblep_pf2_data },
	{ 0x340000, 0x3401ff, MWA16_NOP }, /* Unused row scroll */
	{ 0x340400, 0x34047f, MWA16_NOP }, /* Unused col scroll */
	{ 0x342000, 0x3421ff, MWA16_NOP },
	{ 0x342400, 0x34247f, MWA16_NOP },
	{ 0xff0000, 0xffffff, MWA16_RAM }, /* RAM */
MEMORY_END

static MEMORY_READ16_START( htchctch_readmem )
    { 0x000000, 0x0fffff, MRA16_ROM },
	{ 0x120000, 0x123fff, MRA16_RAM },
	{ 0x140000, 0x1407ff, MRA16_RAM },
	{ 0x160000, 0x160fff, MRA16_RAM },
	{ 0x180000, 0x18000f, tumblepop_controls_r },
	{ 0x1a0000, 0x1a0fff, MRA16_RAM },
	{ 0x341000, 0x342fff, MRA16_RAM },
MEMORY_END

static MEMORY_WRITE16_START( htchctch_writemem )
    { 0x000000, 0x0fffff, MWA16_ROM },
	{ 0x100000, 0x100001, semicom_soundcmd_w },
	{ 0x100002, 0x100003, bcstory_tilebank_w },
	{ 0x120000, 0x123fff, MWA16_RAM, &tumblep_mainram },
	{ 0x140000, 0x1407ff, paletteram16_xBBBBBGGGGGRRRRR_word_w, &paletteram16 },
	{ 0x160000, 0x160fff, MWA16_RAM, &spriteram16, &spriteram_size }, /* Bootleg sprite buffer */
	{ 0x18000c, 0x18000d, MWA16_NOP },
	{ 0x1a0000, 0x1a0fff, MWA16_RAM },
	{ 0x300000, 0x30000f, tumblep_control_0_w },
	{ 0x320000, 0x320fff, tumblep_pf1_data_w, &tumblep_pf1_data },
	{ 0x322000, 0x322fff, tumblep_pf2_data_w, &tumblep_pf2_data },
	{ 0x341000, 0x342fff, MWA16_RAM }, /* extra ram? */
MEMORY_END


static MEMORY_READ16_START( suprtrio_readmem )
    { 0x000000, 0x07ffff, MRA16_ROM },
	{ 0x700000, 0x700fff, MRA16_RAM },
	{ 0xa00000, 0xa0000f, MRA16_RAM },
	{ 0xa20000, 0xa20fff, MRA16_RAM },
	{ 0xa22000, 0xa22fff, MRA16_RAM },
	{ 0xcf0000, 0xcf05ff, MRA16_RAM },
	{ 0xe00000, 0xe00001, input_port_0_word_r },
	{ 0xe40000, 0xe40001, input_port_1_word_r },
	{ 0xe80002, 0xe80003, input_port_2_word_r },
	{ 0xf00000, 0xf07fff, MRA16_RAM },
MEMORY_END

static MEMORY_WRITE16_START( suprtrio_writemem )
    { 0x000000, 0x07ffff, MWA16_ROM },
	{ 0x700000, 0x700fff, MWA16_RAM, &spriteram16, &spriteram_size },
	{ 0xa00000, 0xa0000f, MWA16_RAM, &suprtrio_control },
	{ 0xa20000, 0xa20fff, tumblep_pf1_data_w, &tumblep_pf1_data },
	{ 0xa22000, 0xa22fff, tumblep_pf2_data_w, &tumblep_pf2_data },
	{ 0xcf0000, 0xcf05ff, paletteram16_xBBBBBGGGGGRRRRR_word_w, &paletteram16 },
	{ 0xe00000, 0xe00001, suprtrio_tilebank_w },
	{ 0xec0000, 0xec0001, semicom_soundcmd_w },
	{ 0xf00000, 0xf07fff, MWA16_RAM },
MEMORY_END

/******************************************************************************/

static WRITE_HANDLER( YM2151_w )
{
	switch (offset) {
	case 0:
		YM2151_register_port_0_w(0,data);
		break;
	case 1:
		YM2151_data_port_0_w(0,data);
		break;
	}
}

/* Physical memory map (21 bits) */
static MEMORY_READ_START( sound_readmem )
	{ 0x000000, 0x00ffff, MRA_ROM },
	{ 0x100000, 0x100001, MRA_NOP },
	{ 0x110000, 0x110001, YM2151_status_port_0_r },
	{ 0x120000, 0x120001, OKIM6295_status_0_r },
	{ 0x130000, 0x130001, MRA_NOP }, /* This board only has 1 oki chip */
	{ 0x140000, 0x140001, soundlatch_r },
	{ 0x1f0000, 0x1f1fff, MRA_BANK8 },
MEMORY_END

static MEMORY_WRITE_START( sound_writemem )
	{ 0x000000, 0x00ffff, MWA_ROM },
	{ 0x100000, 0x100001, MWA_NOP }, /* YM2203 - this board doesn't have one */
	{ 0x110000, 0x110001, YM2151_w },
	{ 0x120000, 0x120001, OKIM6295_data_0_w },
	{ 0x130000, 0x130001, MWA_NOP },
	{ 0x1f0000, 0x1f1fff, MWA_BANK8 },
	{ 0x1fec00, 0x1fec01, H6280_timer_w },
	{ 0x1ff402, 0x1ff403, H6280_irq_status_w },
MEMORY_END

WRITE16_HANDLER( semicom_soundcmd_w )
{
	if (ACCESSING_LSB)
	{
		soundlatch_w(0,data & 0xff);
		/* needed for Super Trio which reads the sound with polling */
/*      cpu_spinuntil_time(TIME_IN_USEC(100)); */
		cpu_boost_interleave(0, TIME_IN_USEC(20));

	}
}

static WRITE_HANDLER( oki_sound_bank_w )
{
	UINT8 *oki = memory_region(REGION_SOUND1);
	memcpy(&oki[0x30000], &oki[(data * 0x10000) + 0x40000], 0x10000);
}

static MEMORY_READ_START( semicom_sound_readmem )
    { 0x0000, 0xcfff, MRA_ROM },
	{ 0xd000, 0xd7ff, MRA_RAM },
	{ 0xf001, 0xf001, YM2151_status_port_0_r },
	{ 0xf002, 0xf002, OKIM6295_status_0_r },
	{ 0xf008, 0xf008, soundlatch_r },
MEMORY_END

static MEMORY_WRITE_START( semicom_sound_writemem )
    { 0x0000, 0xcfff, MWA_ROM },
	{ 0xd000, 0xd7ff, MWA_RAM },
	{ 0xf000, 0xf000, YM2151_register_port_0_w },
	{ 0xf001, 0xf001, YM2151_data_port_0_w },
	{ 0xf002, 0xf002, OKIM6295_data_0_w },
    { 0xf00e, 0xf00e, oki_sound_bank_w },
MEMORY_END


/******************************************************************************/

INPUT_PORTS_START( tumblep )
	PORT_START	/* Player 1 controls */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* button 3 - unused */
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START	/* Player 2 controls */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* button 3 - unused */
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START2 )

	PORT_START	/* Credits */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_VBLANK )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* Dip switch bank 1 */
	PORT_DIPNAME( 0xe0, 0xe0, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0xe0, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x60, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0xa0, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 1C_6C ) )
	PORT_DIPNAME( 0x1c, 0x1c, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x1c, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x14, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x18, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 1C_6C ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x01, 0x01, "2 Coins to Start, 1 to Continue" )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START	/* Dip switch bank 2 */
	PORT_DIPNAME( 0xc0, 0xc0, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x80, "1" )
	PORT_DIPSETTING(    0x00, "2" )
	PORT_DIPSETTING(    0xc0, "3" )
	PORT_DIPSETTING(    0x40, "4" )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x10, "Easy" )
	PORT_DIPSETTING(    0x30, "Normal" )
	PORT_DIPSETTING(    0x20, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
#if TUMBLEP_HACK
	PORT_DIPNAME( 0x08, 0x08, "Remove Monsters" )
#else
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unused ) )		/* See notes */
#endif
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
#if TUMBLEP_HACK
	PORT_DIPNAME( 0x04, 0x04, "Edit Levels" )
#else
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unused ) )		/* See notes */
#endif
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, "Allow Continue" )
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
INPUT_PORTS_END

INPUT_PORTS_START( fncywld )
	PORT_START	/* Player 1 controls */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START	/* Player 2 controls */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START2 )

	PORT_START	/* Credits */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_VBLANK )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* Dip switch bank 1 */
	PORT_DIPNAME( 0xe0, 0xe0, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x60, DEF_STR( 2C_1C ) )
/*	PORT_DIPSETTING(    0x00, DEF_STR( 2C_1C ) )*/		/* duplicated setting */
	PORT_DIPSETTING(    0xe0, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0xa0, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 1C_4C ) )
	PORT_DIPNAME( 0x10, 0x10, "Allow Continue" )
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Demo_Sounds ) )	/* to be confirmed */
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, "Language" )			/* only seems to the title screen */
	PORT_DIPSETTING(    0x04, "English" )
	PORT_DIPSETTING(    0x00, "Korean" )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x01, 0x01, "2 Coins to Start, 1 to Continue" )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START	/* Dip switch bank 2 */
	PORT_DIPNAME( 0xc0, 0xc0, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x80, "1" )
	PORT_DIPSETTING(    0x00, "2" )
	PORT_DIPSETTING(    0xc0, "3" )
	PORT_DIPSETTING(    0x40, "4" )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Difficulty ) )	/* to be confirmed */
	PORT_DIPSETTING(    0x30, "Easy" )
	PORT_DIPSETTING(    0x20, "Normal" )
	PORT_DIPSETTING(    0x10, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
#if FNCYWLD_HACK
	PORT_DIPNAME( 0x08, 0x08, "Remove Monsters" )
#else
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unused ) )		/* See notes */
#endif
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
#if FNCYWLD_HACK
	PORT_DIPNAME( 0x04, 0x04, "Edit Levels" )
#else
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unused ) )		/* See notes */
#endif
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
INPUT_PORTS_END

INPUT_PORTS_START( magipur )
	PORT_START	/* Player 1 controls */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START	/* Player 2 controls */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START2 )

	PORT_START	/* Credits */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_VBLANK )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* Dip switch bank 1 */
	PORT_DIPNAME( 0xe0, 0xe0, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x60, DEF_STR( 2C_1C ) )
/*	PORT_DIPSETTING(    0x00, DEF_STR( 2C_1C ) )*/		/* duplicated setting */
	PORT_DIPSETTING(    0xe0, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0xa0, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 1C_4C ) )
	PORT_DIPNAME( 0x10, 0x10, "Allow Continue" )
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Demo_Sounds ) )	/* to be confirmed */
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x01, 0x01, "2 Coins to Start, 1 to Continue" )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START	/* Dip switch bank 2 */
	PORT_DIPNAME( 0xc0, 0xc0, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x80, "1" )
	PORT_DIPSETTING(    0x00, "2" )
	PORT_DIPSETTING(    0xc0, "3" )
	PORT_DIPSETTING(    0x40, "4" )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Difficulty ) )	/* to be confirmed */
	PORT_DIPSETTING(    0x30, "Easy" )
	PORT_DIPSETTING(    0x20, "Normal" )
	PORT_DIPSETTING(    0x10, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unused ) )		/* See notes */
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unused ) )		/* See notes */
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
INPUT_PORTS_END

INPUT_PORTS_START( htchctch )
	PORT_START	/* Player 1 controls */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* button 3 - unused */
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START	/* Player 2 controls */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* button 3 - unused */
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START2 )

	PORT_START	/* Credits */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_VBLANK )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* Dip switch bank 1 (wrong) */
	PORT_DIPNAME( 0xe0, 0xe0, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0xe0, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x60, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0xa0, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 1C_6C ) )
	PORT_DIPNAME( 0x1c, 0x1c, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x1c, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x14, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x18, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 1C_6C ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x01, 0x01, "2 Coins to Start, 1 to Continue" )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START	/* Dip switch bank 2 */
	PORT_DIPNAME( 0xc0, 0xc0, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x80, "1" )
	PORT_DIPSETTING(    0x00, "2" )
	PORT_DIPSETTING(    0xc0, "3" )
	PORT_DIPSETTING(    0x40, "4" )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x10, "Easy" )
	PORT_DIPSETTING(    0x30, "Normal" )
	PORT_DIPSETTING(    0x20, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, "Allow Continue" )
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Yes ) )
	PORT_SERVICE( 0x01, IP_ACTIVE_LOW )
INPUT_PORTS_END

INPUT_PORTS_START( cookbib )
	PORT_START	/* Player 1 controls */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_PLAYER1)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_PLAYER1)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_PLAYER1)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* button 3 - unused */
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START	/* Player 2 controls */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_PLAYER2)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_PLAYER2)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_PLAYER2)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* button 3 - unused */
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START2 )

	PORT_START	/* Credits */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_VBLANK )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* Dip switch bank 1 */
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
	PORT_DIPNAME( 0x80, 0x80, "Stage Skip" )	/* to be confirmed */
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START	/* Dip switch bank 2 */
	PORT_SERVICE( 0x01, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x06, 0x06, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x00, "Easy" )
	PORT_DIPSETTING(    0x06, "Normal" )
	PORT_DIPSETTING(    0x02, "Hard" )
	PORT_DIPSETTING(    0x04, "Very_Hard" )
	PORT_DIPNAME( 0x38, 0x38, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x38, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x28, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x18, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 1C_3C ) )
	PORT_DIPNAME( 0x40, 0x40, "VS Round" )
	PORT_DIPSETTING(    0x40, "3" )
	PORT_DIPSETTING(    0x00, "1" )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
INPUT_PORTS_END

INPUT_PORTS_START( bcstory )
	PORT_START	/* Player 1 controls */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_PLAYER1)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_PLAYER1)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_PLAYER1)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER1)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START	/* Player 2 controls */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_PLAYER2)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_PLAYER2)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_PLAYER2)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START2 )

	PORT_START	/* Credits */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_VBLANK )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* Dip switch bank 1 */
	PORT_SERVICE( 0x01, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x0e, 0x0e, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x04, "Level 1" )
	PORT_DIPSETTING(    0x08, "Level 2" )
	PORT_DIPSETTING(    0x00, "Level 3" )
	PORT_DIPSETTING(    0x0e, "Level 4" )
	PORT_DIPSETTING(    0x06, "Level 5" )
	PORT_DIPSETTING(    0x0a, "Level 6" )
	PORT_DIPSETTING(    0x02, "Level 7" )
	PORT_DIPSETTING(    0x0c, "Level 8" )
	PORT_DIPNAME( 0x70, 0x70, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x60, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x70, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x50, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 1C_3C ) )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START	/* Dip switch bank 2 */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Free_Play) )
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
	PORT_DIPNAME( 0x20, 0x00, "Event Free (Allow Event Selection)" )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, "Control Type" )
	PORT_DIPSETTING(    0x40, "Joystick + Buttons" )
	PORT_DIPSETTING(    0x00, "Buttons Only" )
	PORT_DIPNAME( 0x80, 0x80, "Test Mode (Easy Event Select)" )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
INPUT_PORTS_END

INPUT_PORTS_START( chokchok )
	PORT_START	/* Player 1 controls */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_PLAYER1)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_PLAYER1)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_PLAYER1)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER1)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START	/* Player 2 controls */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_PLAYER2)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_PLAYER2)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_PLAYER2)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START2 )

	PORT_START	/* Credits */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_VBLANK )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* Dip switch bank 1 */
	PORT_DIPNAME( 0x01, 0x01, "VS Round" )
	PORT_DIPSETTING(    0x01, "3" )
	PORT_DIPSETTING(    0x00, "2" )
	PORT_DIPNAME( 0x06, 0x06, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x00, "Easy" )
	PORT_DIPSETTING(    0x06, "Normal" )
	PORT_DIPSETTING(    0x02, "Hard" )
	PORT_DIPSETTING(    0x04, "Very_Hard" )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, "Time" )
	PORT_DIPSETTING(    0x20, "60 Seconds" )
	PORT_DIPSETTING(    0x00, "90 Seconds" )
	PORT_DIPNAME( 0xc0, 0xc0, "Starting Balls" )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0xc0, "4" )
	PORT_DIPSETTING(    0x40, "5" )
	PORT_DIPSETTING(    0x80, "6" )

	PORT_START	/* Dip switch bank 2 */
	PORT_SERVICE( 0x01, IP_ACTIVE_LOW )		/* Sprite Adjustments */
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x38, 0x38, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x38, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x28, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x18, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 1C_3C ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
INPUT_PORTS_END

INPUT_PORTS_START( sdfight )
	PORT_START	/* Player 1 controls */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_PLAYER1)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_PLAYER1)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_PLAYER1)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER1)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START	/* Player 2 controls */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_PLAYER2)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_PLAYER2)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_PLAYER2)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START2 )

	PORT_START	/* Credits */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_VBLANK )	/* to be confirmed */
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* Dip switch bank 1 */
	PORT_SERVICE( 0x01, IP_ACTIVE_LOW ) /* These dips were done from the Test mode screens */
	PORT_DIPNAME( 0x0e, 0x0e, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x04, "Level 1" )
	PORT_DIPSETTING(    0x08, "Level 2" )
	PORT_DIPSETTING(    0x00, "Level 3" )
	PORT_DIPSETTING(    0x0e, "Level 4" )
	PORT_DIPSETTING(    0x06, "Level 5" )
	PORT_DIPSETTING(    0x0a, "Level 6" )
	PORT_DIPSETTING(    0x02, "Level 7" )
	PORT_DIPSETTING(    0x0c, "Level 8" )
	PORT_DIPNAME( 0x70, 0x70, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x60, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x70, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x50, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 1C_3C ) )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START	/* Dip switch bank 2 */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Free_Play ) ) /* Only Free Play shows in Test Mode */
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, "Rounds to Win" ) /* Does not show up in Test Mode screen */
	PORT_DIPSETTING(    0x08, "2 Rounds" )
	PORT_DIPSETTING(    0x00, "3 Rounds" )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0xc0, 0xc0, "Time" ) /* Does not show up in Test Mode screen */
	PORT_DIPSETTING(    0x40, "30" )
	PORT_DIPSETTING(    0x80, "50" )
	PORT_DIPSETTING(    0xc0, "70" )
	PORT_DIPSETTING(    0x00, "90" )
INPUT_PORTS_END

INPUT_PORTS_START( suprtrio )
	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER1 )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_START2 )

	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0xfffe, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_DIPNAME( 0x0007, 0x0000, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(      0x0006, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(      0x0005, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(      0x0004, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0003, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0001, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x0007, DEF_STR( Free_Play ))
	PORT_DIPNAME( 0x0018, 0x0010, DEF_STR( Lives ) )
	PORT_DIPSETTING(      0x0000, "1" )
	PORT_DIPSETTING(      0x0008, "2" )
	PORT_DIPSETTING(      0x0010, "3" )
	PORT_DIPSETTING(      0x0018, "5" )
	PORT_DIPNAME( 0x0020, 0x0000, DEF_STR(Difficulty) )
	PORT_DIPSETTING(      0x0000, "Normal" )
	PORT_DIPSETTING(      0x0020, "Hard" )
	PORT_DIPNAME( 0x0040, 0x0000, DEF_STR(Bonus_Life) )
	PORT_DIPSETTING(      0x0000, "50000" )
	PORT_DIPSETTING(      0x0040, "60000" )
	PORT_SERVICE( 0x0080, IP_ACTIVE_HIGH )
	PORT_BIT( 0xff00, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END


/******************************************************************************/

static struct GfxLayout tcharlayout =
{
	8,8,
	RGN_FRAC(1,2),
	4,
	{ RGN_FRAC(1,2)+8, RGN_FRAC(1,2)+0, 8, 0 },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16 },
	16*8
};

static struct GfxLayout jumppop_tcharlayout =
{
	8,8,
	RGN_FRAC(1,2),
	8,
	{ 0,1,2,3,4,5,6,7 },
	{RGN_FRAC(1,2)+0,RGN_FRAC(1,2)+8,0,8,RGN_FRAC(1,2)+16,RGN_FRAC(1,2)+24,16,24 },
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32 },
	32*8
};

static struct GfxLayout tlayout =
{
	16,16,
	RGN_FRAC(1,2),
	4,
	{ RGN_FRAC(1,2)+8, RGN_FRAC(1,2)+0, 8, 0 },
	{ 32*8+0, 32*8+1, 32*8+2, 32*8+3, 32*8+4, 32*8+5, 32*8+6, 32*8+7,
			0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16,
			8*16, 9*16, 10*16, 11*16, 12*16, 13*16, 14*16, 15*16 },
	64*8
};

static struct GfxLayout suprtrio_tlayout =
{
	16,16,
	RGN_FRAC(1,4),
	4,
	{ RGN_FRAC(2,4),RGN_FRAC(0,4), RGN_FRAC(3,4), RGN_FRAC(1,4) },

	{ 0, 1, 2, 3, 4, 5, 6, 7,16*8+0, 16*8+1, 16*8+2, 16*8+3, 16*8+4, 16*8+5, 16*8+6,16*8+7 },
	{ 1*8, 0*8, 2*8, 3*8, 5*8, 4*8, 6*8, 7*8,
			8*8, 9*8, 10*8, 11*8, 12*8, 13*8, 14*8, 15*8 },
	32*8
};


static struct GfxLayout jumpop_tlayout =
{
	16,16,
	RGN_FRAC(1,2),
	8,
	{ 0,1,2,3,4,5,6,7 },
	{RGN_FRAC(1,2)+0,RGN_FRAC(1,2)+8,0,8,RGN_FRAC(1,2)+16,RGN_FRAC(1,2)+24,16,24,
	256+RGN_FRAC(1,2)+0,256+RGN_FRAC(1,2)+8,256+0,256+8,256+RGN_FRAC(1,2)+16,256+RGN_FRAC(1,2)+24,256+16,256+24
	},
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32,
	512+0*32, 512+1*32, 512+2*32, 512+3*32, 512+4*32, 512+5*32, 512+6*32, 512+7*32
	},
	128*8
};



static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &tcharlayout, 256, 16 },	/* Characters 8x8 */
	{ REGION_GFX1, 0, &tlayout,     512, 16 },	/* Tiles 16x16 */
	{ REGION_GFX1, 0, &tlayout,     256, 16 },	/* Tiles 16x16 */
	{ REGION_GFX2, 0, &tlayout,       0, 16 },	/* Sprites 16x16 */
	{ -1 } /* end of array */
};

static struct GfxDecodeInfo suprtrio_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &tcharlayout, 256, 16 },	/* Characters 8x8 */
	{ REGION_GFX1, 0, &suprtrio_tlayout,     512, 16 },	/* Tiles 16x16 */
	{ REGION_GFX1, 0, &suprtrio_tlayout,     256, 16 },	/* Tiles 16x16 */
	{ REGION_GFX2, 0, &tlayout,       0, 16 },	/* Sprites 16x16 */
	{ -1 } /* end of array */
};

static struct GfxDecodeInfo fncywld_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &tcharlayout, 0x400, 0x40 },	/* Characters 8x8 */
	{ REGION_GFX1, 0, &tlayout,     0x400, 0x40 },	/* Tiles 16x16 */
	{ REGION_GFX1, 0, &tlayout,     0x200, 0x40 },	/* Tiles 16x16 */
	{ REGION_GFX2, 0, &tlayout,       0, 0x40 },	/* Sprites 16x16 */
	{ -1 } /* end of array */
};

static struct GfxDecodeInfo jumppop_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &jumppop_tcharlayout, 0x100, 0x40 },	/* Characters 8x8 */
	{ REGION_GFX1, 0, &jumpop_tlayout,     0x100, 0x40 },	/* Tiles 16x16 */
	{ REGION_GFX1, 0, &jumpop_tlayout,     0x100, 0x40 },	/* Tiles 16x16 */
	{ REGION_GFX2, 0, &tlayout,       0, 0x40 },	/* Sprites 16x16 */
	{ -1 } /* end of array */
};


/******************************************************************************/

static struct OKIM6295interface okim6295_interface3 =
{
	1,          		/* 1 chip */
	{ 875000/132 },		/* Frequency */
	{ REGION_SOUND1 },	/* memory region */
	{ 50 }
};

static struct OKIM6295interface okim6295_interface2 =
{
	1,          /* 1 chip */
	{ 7757 },   /* 8000Hz frequency */
	{ REGION_SOUND1 },	/* memory region */
	{ 70 }
};

static struct OKIM6295interface okim6295_interface =
{
	1,          /* 1 chip */
	{ 7757 },	/* Frequency */
	{ REGION_SOUND1 },	/* memory region */
	{ 50 }
};

static void sound_irq(int state)
{
	cpu_set_irq_line(1,1,state); /* IRQ 2 */
}

static struct YM2151interface ym2151_interface =
{
	1,
	32220000/9, /* May not be correct, there is another crystal near the ym2151 */
	{ YM3012_VOL(45,MIXER_PAN_LEFT,45,MIXER_PAN_RIGHT) },
	{ sound_irq }
};

static struct OKIM6295interface fncy_okim6295_interface =
{
	1,          /* 1 chip */
	{ 7757 },	/* Frequency */
	{ REGION_SOUND1 },	/* memory region */
	{ 100 }
};

static struct YM2151interface fncy_ym2151_interface =
{
	1,
	32220000/9,
	{ YM3012_VOL(20,MIXER_PAN_LEFT,20,MIXER_PAN_RIGHT) },
	{ 0 }
};


static struct YM2151interface magipur_ym2151_interface =
{
	1,
	4000000,
	{ YM3012_VOL(20,MIXER_PAN_LEFT,20,MIXER_PAN_RIGHT) },
	{ 0 }
};

static MACHINE_DRIVER_START( tumblep )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 14000000)
	MDRV_CPU_MEMORY(tumblepop_readmem,tumblepop_writemem)
	MDRV_CPU_VBLANK_INT(irq6_line_hold,1)

	MDRV_CPU_ADD(H6280, 32220000/8)	/* Custom chip 45; Audio section crystal is 32.220 MHz */
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
	MDRV_CPU_MEMORY(sound_readmem,sound_writemem)

	MDRV_FRAMES_PER_SECOND(58)
	MDRV_VBLANK_DURATION(529)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(40*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 40*8-1, 1*8, 31*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(1024)

	MDRV_VIDEO_START(tumblep)
	MDRV_VIDEO_UPDATE(tumblep)

	/* sound hardware */
	MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
	MDRV_SOUND_ADD(YM2151, ym2151_interface)
	MDRV_SOUND_ADD(OKIM6295, okim6295_interface)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( tumblepb )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 14000000)
	MDRV_CPU_MEMORY(tumblepopb_readmem,tumblepopb_writemem)
	MDRV_CPU_VBLANK_INT(irq6_line_hold,1)

	MDRV_FRAMES_PER_SECOND(58)
	MDRV_VBLANK_DURATION(529)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(40*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 40*8-1, 1*8, 31*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(1024)

	MDRV_VIDEO_START(tumblep)
	MDRV_VIDEO_UPDATE(tumblepb)

	/* sound hardware */
	MDRV_SOUND_ADD(OKIM6295, okim6295_interface2)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( jumpkids )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 14000000)
	MDRV_CPU_MEMORY(tumblepopb_readmem,tumblepopb_writemem)
	MDRV_CPU_VBLANK_INT(irq6_line_hold,1)

	/* z80? */

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(529)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(40*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 40*8-1, 1*8, 31*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(1024)

	MDRV_VIDEO_START(tumblep)
	MDRV_VIDEO_UPDATE(jumpkids)

	/* sound hardware */
	MDRV_SOUND_ADD(OKIM6295, okim6295_interface2)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( fncywld )
	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 12000000)
	MDRV_CPU_MEMORY(fncywld_readmem,fncywld_writemem)
	MDRV_CPU_VBLANK_INT(irq6_line_hold,1)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(529)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(40*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 40*8-1, 1*8, 31*8-1)
	MDRV_GFXDECODE(fncywld_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(0x800)

	MDRV_VIDEO_START(fncywld)
	MDRV_VIDEO_UPDATE(fncywld)

	/* sound hardware */
	MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
	MDRV_SOUND_ADD(YM2151, fncy_ym2151_interface)
	MDRV_SOUND_ADD(OKIM6295, fncy_okim6295_interface)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( magipur )
	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 12000000)
	MDRV_CPU_MEMORY(magipur_readmem,magipur_writemem)
	MDRV_CPU_VBLANK_INT(irq6_line_hold,1)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(529)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(40*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 40*8-1, 1*8, 31*8-1)
	MDRV_GFXDECODE(fncywld_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(0x800)

	MDRV_VIDEO_START(fncywld)
	MDRV_VIDEO_UPDATE(fncywld)

	/* sound hardware */
	MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
	MDRV_SOUND_ADD(YM2151, magipur_ym2151_interface)
	MDRV_SOUND_ADD(OKIM6295, fncy_okim6295_interface)
MACHINE_DRIVER_END

static void semicom_irqhandler(int irq)
{
	cpu_set_irq_line(1,0,irq ? ASSERT_LINE : CLEAR_LINE);
}

/* SemiCom Sound */

static struct YM2151interface semicom_ym2151_interface =
{
	1,
	3427190,	/* verified */
	{ YM3012_VOL(10,MIXER_PAN_LEFT,10,MIXER_PAN_RIGHT) },
	{ semicom_irqhandler }
};

static struct OKIM6295interface semicom_okim6295_interface =
{
	1,			/* 1 chip */
	{ 1024000/132 },		/* verified */
	{ REGION_SOUND1 },
	{ 100 }
};

MACHINE_INIT (htchctch)
{
	/* copy protection data every reset */

	UINT16 *PROTDATA = (UINT16*)memory_region(REGION_USER1);
	int i;

	for (i = 0;i < 0x200/2;i++)
		tumblep_mainram[0x000/2 + i] = PROTDATA[i];

}

static MACHINE_DRIVER_START( htchctch )
	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 15000000) /* verified */
	MDRV_CPU_MEMORY(htchctch_readmem,htchctch_writemem)
	MDRV_CPU_VBLANK_INT(irq6_line_hold,1)

	MDRV_CPU_ADD( Z80, 3427190) /* verified */

	/* audio CPU */
	MDRV_CPU_MEMORY(semicom_sound_readmem,semicom_sound_writemem)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(529)

	MDRV_MACHINE_INIT (htchctch)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(40*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 40*8-1, 1*8, 31*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(1024)

	MDRV_VIDEO_START(tumblep)
	MDRV_VIDEO_UPDATE(semicom)

	/* sound hardware - same as hyperpac */
	MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
	MDRV_SOUND_ADD(YM2151, semicom_ym2151_interface)
	MDRV_SOUND_ADD(OKIM6295, semicom_okim6295_interface)
MACHINE_DRIVER_END

MACHINE_DRIVER_START( cookbib )
	MDRV_IMPORT_FROM(htchctch)
	MDRV_VIDEO_UPDATE( semicom_altoffsets )
MACHINE_DRIVER_END

MACHINE_DRIVER_START( bcstory )
	MDRV_IMPORT_FROM(htchctch)
	MDRV_VIDEO_UPDATE( bcstory )
MACHINE_DRIVER_END

MACHINE_DRIVER_START( sdfight )
	MDRV_IMPORT_FROM(htchctch)
	MDRV_VIDEO_UPDATE( sdfight )
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( suprtrio )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 14000000) /* 14mhz should be correct, but lots of sprite flicker later in game */
	MDRV_CPU_MEMORY(suprtrio_readmem,suprtrio_writemem)
	MDRV_CPU_VBLANK_INT(irq6_line_hold,1)

	MDRV_CPU_ADD(Z80, 8000000)
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
	MDRV_CPU_MEMORY(semicom_sound_readmem,semicom_sound_writemem)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(529)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(40*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 40*8-1, 1*8-1, 31*8-2)
	MDRV_GFXDECODE(suprtrio_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(1024)

	MDRV_VIDEO_START(suprtrio)
	MDRV_VIDEO_UPDATE(suprtrio)

	/* sound hardware */
	MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
	MDRV_SOUND_ADD(OKIM6295, okim6295_interface3)
MACHINE_DRIVER_END


/******************************************************************************/

ROM_START( tumblep )
	ROM_REGION( 0x80000, REGION_CPU1, 0 ) /* 68000 code */
	ROM_LOAD16_BYTE("hl00-1.f12", 0x00000, 0x40000, CRC(fd697c1b) SHA1(1a3dee4c7383f2bc2d73037e80f8f5d8297e7433) )
	ROM_LOAD16_BYTE("hl01-1.f13", 0x00001, 0x40000, CRC(d5a62a3f) SHA1(7249563993fa8e1f19ddae51306d4a576b5cb206) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* Sound cpu */
	ROM_LOAD( "hl02-.f16",    0x00000, 0x10000, CRC(a5cab888) SHA1(622f6adb01e31b8f3adbaed2b9900b54c5922c57) )

	ROM_REGION( 0x080000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "map-02.rom",   0x00000, 0x80000, CRC(dfceaa26) SHA1(83e391ff39efda71e5fa368ac68ba7d6134bac21) )	/* encrypted */

	ROM_REGION( 0x100000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "map-01.rom",   0x00000, 0x80000, CRC(e81ffa09) SHA1(01ada9557ead91eb76cf00db118d6c432104a398) )
	ROM_LOAD( "map-00.rom",   0x80000, 0x80000, CRC(8c879cfe) SHA1(a53ef7811f14a8b105749b1cf29fe8a3a33bab5e) )

	ROM_REGION( 0x20000, REGION_SOUND1, 0 ) /* Oki samples */
	ROM_LOAD( "hl03-.j15",    0x00000, 0x20000, CRC(01b81da0) SHA1(914802f3206dc59a720af9d57eb2285bc8ba822b) )
ROM_END

ROM_START( tumblepj )
	ROM_REGION( 0x80000, REGION_CPU1, 0 ) /* 68000 code */
	ROM_LOAD16_BYTE("hk00-1.f12", 0x00000, 0x40000, CRC(2d3e4d3d) SHA1(0acc8b93bd49395904dff11c582bdbaccdbd3eef) )
	ROM_LOAD16_BYTE("hk01-1.f13", 0x00001, 0x40000, CRC(56912a00) SHA1(0545f6bff2a0aa2f36adda0f9d73b165387abc3a) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* Sound cpu */
	ROM_LOAD( "hl02-.f16",    0x00000, 0x10000, CRC(a5cab888) SHA1(622f6adb01e31b8f3adbaed2b9900b54c5922c57) )

	ROM_REGION( 0x080000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "map-02.rom",   0x00000, 0x80000, CRC(dfceaa26) SHA1(83e391ff39efda71e5fa368ac68ba7d6134bac21) )	/* encrypted */

	ROM_REGION( 0x100000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "map-01.rom",   0x00000, 0x80000, CRC(e81ffa09) SHA1(01ada9557ead91eb76cf00db118d6c432104a398) )
	ROM_LOAD( "map-00.rom",   0x80000, 0x80000, CRC(8c879cfe) SHA1(a53ef7811f14a8b105749b1cf29fe8a3a33bab5e) )

	ROM_REGION( 0x20000, REGION_SOUND1, 0 ) /* Oki samples */
	ROM_LOAD( "hl03-.j15",    0x00000, 0x20000, CRC(01b81da0) SHA1(914802f3206dc59a720af9d57eb2285bc8ba822b) )
ROM_END

ROM_START( tumblepb )
	ROM_REGION( 0x80000, REGION_CPU1, 0 ) /* 68000 code */
	ROM_LOAD16_BYTE ("thumbpop.12", 0x00000, 0x40000, CRC(0c984703) SHA1(588d2b2464e0027c8d0703a2b62ebda225ba4276) )
	ROM_LOAD16_BYTE( "thumbpop.13", 0x00001, 0x40000, CRC(864c4053) SHA1(013eb35e79aa7a7cd1a8061c4b75b37a8bfb10c6) )

	ROM_REGION( 0x080000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD16_BYTE( "thumbpop.19",  0x00000, 0x40000, CRC(0795aab4) SHA1(85b38804446f6b0b4d8c3a59a8958d520c567a4e) )
	ROM_LOAD16_BYTE( "thumbpop.18",  0x00001, 0x40000, CRC(ad58df43) SHA1(2e562bfffb42543af767dd9e82a1d2465dfcd8b8) )

	ROM_REGION( 0x100000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "map-01.rom",   0x00000, 0x80000, CRC(e81ffa09) SHA1(01ada9557ead91eb76cf00db118d6c432104a398) )
	ROM_LOAD( "map-00.rom",   0x80000, 0x80000, CRC(8c879cfe) SHA1(a53ef7811f14a8b105749b1cf29fe8a3a33bab5e) )

	ROM_REGION( 0x80000, REGION_SOUND1, 0 ) /* Oki samples */
	ROM_LOAD( "thumbpop.snd", 0x00000, 0x80000, CRC(fabbf15d) SHA1(de60be43a5cd1d4b93c142bde6cbfc48a25545a3) )
ROM_END

ROM_START( tumblep2 )
	ROM_REGION( 0x80000, REGION_CPU1, 0 ) /* 68000 code */
	ROM_LOAD16_BYTE ("thumbpop.2", 0x00000, 0x40000, CRC(34b016e1) SHA1(b4c496358d48469d170a69e8bba58e0ea919b418) )
	ROM_LOAD16_BYTE( "thumbpop.3", 0x00001, 0x40000, CRC(89501c71) SHA1(2c202218934b845fdf7c99eaf280dccad90767f2) )

	ROM_REGION( 0x080000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD16_BYTE( "thumbpop.19",  0x00000, 0x40000, CRC(0795aab4) SHA1(85b38804446f6b0b4d8c3a59a8958d520c567a4e) )
	ROM_LOAD16_BYTE( "thumbpop.18",  0x00001, 0x40000, CRC(ad58df43) SHA1(2e562bfffb42543af767dd9e82a1d2465dfcd8b8) )

 	ROM_REGION( 0x100000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "map-01.rom",   0x00000, 0x80000, CRC(e81ffa09) SHA1(01ada9557ead91eb76cf00db118d6c432104a398) )
	ROM_LOAD( "map-00.rom",   0x80000, 0x80000, CRC(8c879cfe) SHA1(a53ef7811f14a8b105749b1cf29fe8a3a33bab5e) )

	ROM_REGION( 0x80000, REGION_SOUND1, 0 ) /* Oki samples */
	ROM_LOAD( "thumbpop.snd", 0x00000, 0x80000, CRC(fabbf15d) SHA1(de60be43a5cd1d4b93c142bde6cbfc48a25545a3) )
ROM_END

ROM_START( jumpkids )
	ROM_REGION( 0x80000, REGION_CPU1, 0 ) /* 68000 Code */
	ROM_LOAD16_BYTE( "23.15c", 0x00000, 0x40000, CRC(6ba11e91) SHA1(9f83ef79beb97af1625e7b46858d6f0681dafb23) )
	ROM_LOAD16_BYTE( "24.16c", 0x00001, 0x40000, CRC(5795d98b) SHA1(d1435f0b79a4fa45770c56b91f078c1885fbd048) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* Z80 Code */
	ROM_LOAD( "23.3c", 0x00000, 0x10000, BAD_DUMP CRC(d7dbbd8c) SHA1(3fbbd0c205ddc8aa8c14cc8f482dbe27d4a909c4)  ) /* bad */

	ROM_REGION( 0x80000, REGION_GFX1, ROMREGION_DISPOSE ) /* GFX */
	ROM_LOAD16_BYTE( "30.15j", 0x00000, 0x40000, CRC(44b9a089) SHA1(b6f99b0b597d540b375616dad4354fc9dbb75a21) )
	ROM_LOAD16_BYTE( "29.13j", 0x00001, 0x40000, CRC(3f98ec69) SHA1(f09a62d9bd7ab7681436a1f2f450565573927165) )

	ROM_REGION( 0x100000, REGION_GFX2, ROMREGION_DISPOSE ) /* GFX */
	ROM_LOAD16_BYTE( "25.1g",  0x00000, 0x40000, CRC(176ae857) SHA1(e3178d2a15452a36eb94caf5e5ff3a561783a5f4) )
	ROM_LOAD16_BYTE( "28.1l",  0x00001, 0x40000, BAD_DUMP CRC(dc35c5a0) SHA1(854429261151c58b94542e9ca51b0807e8bc1f5f)  ) /* bad */
	ROM_LOAD16_BYTE( "26.2g",  0x80000, 0x40000, CRC(e8b34980) SHA1(edbf5517c6c9c9c3344d11eabb4a58da87386725) )
	ROM_LOAD16_BYTE( "27.1j",  0x80001, 0x40000, CRC(3918dda3) SHA1(9409b5a5dc4c44c1ddcb77278541d012b5d8e052) )

	ROM_REGION( 0x80000, REGION_USER1, 0 ) /* ? */
	ROM_LOAD( "21.1c", 0x00000, 0x80000, CRC(e5094f75) SHA1(578f32d4e4212c6cfdef186c2a6dc1d9408e8dfc) )

	ROM_REGION( 0x20000, REGION_SOUND1, 0 ) /* Samples */
	ROM_LOAD( "22.2c", 0x00000, 0x20000, CRC(fae44fbf) SHA1(142215ccca9e405232afbfc95527e13cc5b8296e) )
ROM_END

ROM_START( fncywld )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )		/* 68000 Code */
	ROM_LOAD16_BYTE( "01_fw02.bin", 0x000000, 0x080000, CRC(ecb978c1) SHA1(68fbf93a81875f744c6f9820dc4c7d88e912e0a0) )
	ROM_LOAD16_BYTE( "02_fw03.bin", 0x000001, 0x080000, CRC(2d233b42) SHA1(aebeb5d3e06e73d14f713f201b25466bcac97a68) )

	ROM_REGION( 0x100000, REGION_GFX2, ROMREGION_DISPOSE  )
	ROM_LOAD16_BYTE( "05_fw06.bin",  0x00000, 0x40000, CRC(e141ecdc) SHA1(fd656ceb2baccefadfa1e9f6932b1e0f0ec0a189) )
	ROM_LOAD16_BYTE( "06_fw07.bin",  0x00001, 0x40000, CRC(0058a812) SHA1(fc6101a11af63536d0a345c820bcd234bb4ce91a) )
	ROM_LOAD16_BYTE( "03_fw04.bin",  0x80000, 0x40000, CRC(6ad38c14) SHA1(a9951432c2ec5e07ed2ee5faac3f2558242438f2) )
	ROM_LOAD16_BYTE( "04_fw05.bin",  0x80001, 0x40000, CRC(b8d079a6) SHA1(8ad63fba26f7588a9764a0585c159fb57cb8c7ed) )

	ROM_REGION( 0x100000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD16_BYTE( "08_fw09.bin", 0x00000, 0x40000, CRC(a4a00de9) SHA1(65f03a65569f70fb6f3a0fc7caf038bb44a7f503) )
	ROM_LOAD16_BYTE( "07_fw08.bin", 0x00001, 0x40000, CRC(b48cd1d4) SHA1(a95eeba38ae1ce0a2086edb767f636a9cdbd0176) )
	ROM_LOAD16_BYTE( "10_fw11.bin", 0x80000, 0x40000, CRC(f21bab48) SHA1(84371b31487ca5abcbf57152a64f384959d19209) )
	ROM_LOAD16_BYTE( "09_fw10.bin", 0x80001, 0x40000, CRC(6aea8e0f) SHA1(91e2eeef001351c73b1bfbc1a7840e37d3f89900) )

	ROM_REGION( 0x40000, REGION_SOUND1, 0 )	/* Samples */
	ROM_LOAD( "00_fw01.bin", 0x000000, 0x040000, CRC(b395fe01) SHA1(ac7f2e21413658f8d2a1abf3a76b7817a4e050c9) )
ROM_END

ROM_START( magipur )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )        /* 68000 Code */
	ROM_LOAD16_BYTE( "2-27c040.bin", 0x000000, 0x080000, CRC(135c5de7) SHA1(95c75e9e69793f67df9378391ae45915ef9bbb89) )
	ROM_LOAD16_BYTE( "3-27c040.bin", 0x000001, 0x080000, CRC(ee4b16da) SHA1(82391ed4d21d3944ca482be00ab7c0838cf190ff) )

	ROM_REGION( 0x100000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD16_BYTE( "4-27c040.bin",  0x80000, 0x40000, CRC(e460a77d) SHA1(bde15705750e002bd576098700161b0944984401) )
	ROM_CONTINUE(0x80001, 0x40000)
	ROM_LOAD16_BYTE( "5-27c040.bin",  0x00000, 0x40000, CRC(79c53627) SHA1(9e2673b3becf0508f630f3bd8ff5fc30520b120b) )
	ROM_CONTINUE(0x00001, 0x40000)

	ROM_REGION( 0x100000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD16_BYTE( "6-27c040.bin", 0x00001, 0x40000, CRC(b25b5872) SHA1(88a6a110073060c3b7b2987cc41d23c4ca412b43) )
	ROM_CONTINUE(0x00000, 0x40000)
	ROM_LOAD16_BYTE( "7-27c040.bin", 0x80001, 0x40000, CRC(d3c3a672) SHA1(5bbd67a953e1d47d05006a4ef4aa7a23e807f11b) )
	ROM_CONTINUE(0x80000, 0x40000)

	ROM_REGION( 0x40000, REGION_SOUND1, 0 ) /* Samples */
	ROM_LOAD( "1-27c020.bin", 0x000000, 0x040000, CRC(84dcf771) SHA1(f8a693a11b14608a582a90b7fd7d3be92e46a0e1) )
ROM_END

/* Hatch Catch
Interrupts

Lev 1 0x64 0000 00c0 <- just reset .. not used
Lev 2 0x68 0000 00c0  ""
Lev 3 0x6c 0000 00c0  ""
Lev 4 0x70 0000 00c0  ""
Lev 5 0x74 0000 00c0  ""
Lev 6 0x78 0012 0000 <- RAM shared with protection device (first 0x200 bytes?)

*/

ROM_START( htchctch )
	ROM_REGION( 0x80000, REGION_CPU1, 0 ) /* 68000 Code */
	ROM_LOAD16_BYTE( "p03.b16",  0x00001, 0x20000, CRC(eff14c40) SHA1(8fdda1fb859546c16f940e51f7e126768205154c) )
	ROM_LOAD16_BYTE( "p04.b17",  0x00000, 0x20000, CRC(6991483a) SHA1(c8d868ef1f87655c37f0b1efdbb71cd26918f270) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* Z80 Code */
	ROM_LOAD( "p02.b5", 0x00000, 0x10000 , CRC(c5a03186) SHA1(42561ab36e6d7a43828d3094e64bd1229ab893ba) )

	ROM_REGION( 0x10000, REGION_CPU3, 0 ) /* Intel 87C52 MCU Code */
/*	ROM_LOAD( "87c52.mcu", 0x00000, 0x10000 , NO_DUMP )*/ /* can't be dumped */ 

	ROM_REGION16_BE( 0x200, REGION_USER1, 0 ) /* Data from Shared RAM */
	/* this is not a real rom but instead the data extracted from
       shared ram, the MCU puts it there */
	ROM_LOAD16_WORD( "protdata.bin", 0x00000, 0x200 , CRC(5b27adb6) SHA1(a0821093d8c73765ff15767bdfc0afa95aa1371d) )

	ROM_REGION( 0x020000, REGION_SOUND1, 0 ) /* Samples */
	ROM_LOAD( "p01.c1", 0x00000, 0x20000, CRC(18c06829) SHA1(46b180319ed33abeaba70d2cc61f17639e59bfdb) )

	ROM_REGION( 0x80000, REGION_GFX1, 0 ) /* Sprites */
	ROM_LOAD16_BYTE( "p06srom5.bin", 0x00001, 0x40000, CRC(3d2cbb0d) SHA1(bc80be594a40989e3c23539fc2021de65a2444c5) )
	ROM_LOAD16_BYTE( "p07srom6.bin", 0x00000, 0x40000, CRC(0207949c) SHA1(84b4dcd27fe89a5350b6642ef99719bb85514174) )

	ROM_REGION( 0x80000, REGION_GFX2, ROMREGION_DISPOSE ) /* GFX */
	ROM_LOAD16_BYTE( "p08uor1.bin",  0x00000, 0x20000, CRC(6811e7b6) SHA1(8157f92a3168ffbac86cd8c6294b9c0f3ee0835d) )
	ROM_LOAD16_BYTE( "p09uor2.bin",  0x00001, 0x20000, CRC(1c6549cf) SHA1(c05aba9b744144db4537e472842b0d53325aa78f) )
	ROM_LOAD16_BYTE( "p10uor3.bin",  0x40000, 0x20000, CRC(6462e6e0) SHA1(0d107214dfb257e15931701bad6b42c6aadd8a18) )
	ROM_LOAD16_BYTE( "p11uor4.bin",  0x40001, 0x20000, CRC(9c511d98) SHA1(6615cbb125bd1e1b4da400ec4c4a0f4df8f6fa75) )
ROM_END

/* Cookie & Bibi */

ROM_START( cookbib )
	ROM_REGION( 0x80000, REGION_CPU1, 0 ) /* 68000 Code */
	ROM_LOAD16_BYTE( "prg1.ub16",  0x00001, 0x20000, CRC(cda6335f) SHA1(34a57785a458d3e9a66c91734b4511fbc9f3455c) )
	ROM_LOAD16_BYTE( "prg2.ub17",  0x00000, 0x20000, CRC(2664a335) SHA1(8d1c4825720a09db6156599ab905292640b04cba) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* Z80 Code */
	ROM_LOAD( "prg-s.ub5", 0x00000, 0x10000 , CRC(547d6ea3) SHA1(42929e453c4f1c90c29197a9bed953139cfe2873) )

	ROM_REGION( 0x10000, REGION_CPU3, 0 ) /* Intel 87C52 MCU Code */
/*	ROM_LOAD( "87c52.mcu", 0x00000, 0x10000 , NO_DUMP ) /* can't be dumped */ 

	ROM_REGION16_BE( 0x200, REGION_USER1, 0 ) /* Data from Shared RAM */
	/* this is not a real rom but instead the data extracted from
       shared ram, the MCU puts it there */
	ROM_LOAD16_WORD( "protdata.bin", 0x00000, 0x200 , CRC(a77d13f4) SHA1(13db72f5b171b0c1226e97ea98d9edd7144d56d9) )

	ROM_REGION( 0x020000, REGION_SOUND1, 0 ) /* Samples */
	ROM_LOAD( "sound.uc1", 0x00000, 0x20000, CRC(545e19b6) SHA1(ef518bbe44b22e7ef77ee6af337ebcad9b2674e0) )

	ROM_REGION( 0x80000, REGION_GFX1, 0 ) /* */
	ROM_LOAD16_BYTE( "srom5.bin", 0x00001, 0x40000, CRC(73a46e43) SHA1(054fac2dc5dffcbb9d81600689c07774d2e200b6) )
	ROM_LOAD16_BYTE( "srom6.bin", 0x00000, 0x40000, CRC(ade2dbec) SHA1(12d385d22307d8251e711788dff2e503c8f8ca7c) )

	ROM_REGION( 0x80000, REGION_GFX2, ROMREGION_DISPOSE ) /* GFX */
	ROM_LOAD16_BYTE( "uor1.bin",  0x00000, 0x20000, CRC(a7d91f23) SHA1(eb9694e05b8a04ed1cdbb834e1bf745a2b0260be) )
	ROM_LOAD16_BYTE( "uor2.bin",  0x00001, 0x20000, CRC(9aacbec2) SHA1(c1cfe243a7d51c950785073f235d72cc01724cdb) )
	ROM_LOAD16_BYTE( "uor3.bin",  0x40000, 0x20000, CRC(3fee0c3c) SHA1(c71439ba8033c549e40522db5270caf4a297fb99) )
	ROM_LOAD16_BYTE( "uor4.bin",  0x40001, 0x20000, CRC(bed9ed2d) SHA1(7103b99cd0d54df864ea4a0f269011e30ad29ed7) )
ROM_END

/* Choky Choky */

ROM_START( chokchok )
	ROM_REGION( 0x80000, REGION_CPU1, 0 ) /* 68000 Code */
	ROM_LOAD16_BYTE( "ub18.bin",  0x00001, 0x40000, CRC(b183852a) SHA1(fd50c6d91dba64b936ac367e5e5235d09ed60fdd) )
	ROM_LOAD16_BYTE( "ub17.bin",  0x00000, 0x40000, CRC(ecdb45ca) SHA1(03eb2d27ae4de25aa15477135d3b4de8b3b7f7f0) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* Z80 Code */
	ROM_LOAD( "ub5.bin", 0x00000, 0x10000 , CRC(30c2171d) SHA1(3954e286d57b955af6ba9b1a0b49c442d7f295ae) )

	ROM_REGION( 0x10000, REGION_CPU3, 0 ) /* Intel 87C52 MCU Code */
/*	ROM_LOAD( "87c52.mcu", 0x00000, 0x10000 , NO_DUMP )*/ /* can't be dumped */ 

	ROM_REGION16_BE( 0x200, REGION_USER1, 0 ) /* Data from Shared RAM */
	/* this is not a real rom but instead the data extracted from
       shared ram, the MCU puts it there */
	ROM_LOAD16_WORD( "protdata.bin", 0x00000, 0x200 , CRC(0bd39834) SHA1(2860c2b7fcb74546afde11a59d4b359612ab6e68) )

	ROM_REGION( 0x040000, REGION_SOUND1, 0 ) /* Samples */
	ROM_LOAD( "uc1.bin", 0x00000, 0x40000, CRC(f3f57abd) SHA1(601dc669020ef9156fa221e768be9b88454e3f55) )

	ROM_REGION( 0x200000, REGION_GFX1, 0 ) /* tiles */
	ROM_LOAD16_BYTE( "srom5.bin", 0x00001, 0x20000, CRC(836608b8) SHA1(7aa624274efee0a7affb6a1a417752b5ce116c04) )
	ROM_CONTINUE ( 0x100001,0x20000)
	ROM_CONTINUE ( 0x040001,0x20000)
	ROM_CONTINUE ( 0x140001,0x20000)
	ROM_LOAD16_BYTE( "srom6.bin", 0x00000, 0x20000, CRC(31d5715d) SHA1(32612464124290b273c4b1a8b571291f9aeff01c) )
	ROM_CONTINUE ( 0x100000,0x20000)
	ROM_CONTINUE ( 0x040000,0x20000)
	ROM_CONTINUE ( 0x140000,0x20000)

	ROM_REGION( 0x200000, REGION_GFX2, ROMREGION_DISPOSE ) /* sprites */
	ROM_LOAD16_BYTE( "uor1.bin",  0x000000, 0x80000, CRC(ded6642a) SHA1(357c836ebe62e0f7f9e7afdf7428f42d827ede06) )
	ROM_LOAD16_BYTE( "uor2.bin",  0x000001, 0x80000, CRC(493f9516) SHA1(2e1d38493558dc79cd4d232ac421cd5649f4119a) )
	ROM_LOAD16_BYTE( "uor3.bin",  0x100000, 0x80000, CRC(e2dc3e12) SHA1(9e2571f93d27b9048fe8e42d3f13a8e509b3adca) )
	ROM_LOAD16_BYTE( "uor4.bin",  0x100001, 0x80000, CRC(6f377530) SHA1(1367987e3af0baa8e22f09d1b40ad838f33371bc) )
ROM_END

/* BC Story */

ROM_START( bcstry )
	ROM_REGION( 0x80000, REGION_CPU1, 0 ) /* 68000 Code */
	ROM_LOAD16_BYTE( "bcstry_u.35",  0x40001, 0x20000, CRC(d25b80a4) SHA1(6ea1c28cf508b856e93a06063e634a09291cb32c) )
	ROM_CONTINUE ( 0x00001, 0x20000)
	ROM_LOAD16_BYTE( "bcstry_u.62",  0x40000, 0x20000, CRC(7f7aa244) SHA1(ee9bb2bf22d16f06d7935168e2bd09296fba3abc) )
	ROM_CONTINUE ( 0x00000, 0x20000)

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* Z80 Code */
	ROM_LOAD( "bcstry_u.21", 0x04000, 0x4000 , CRC(3ba072d4) SHA1(8b64d3ab4c63132f2f77b2cf38a88eea1a8f11e0) )
	ROM_CONTINUE( 0x0000, 0x4000 )
	ROM_CONTINUE( 0xc000, 0x4000 )
	ROM_CONTINUE( 0x8000, 0x4000 )

	ROM_REGION( 0x10000, REGION_CPU3, 0 ) /* Intel 87C52 MCU Code */
/*	ROM_LOAD( "87c52.mcu", 0x00000, 0x10000 , NO_DUMP )*/ /* can't be dumped */

	ROM_REGION16_BE( 0x200, REGION_USER1, 0 ) /* Data from Shared RAM */
	/* this is not a real rom but instead the data extracted from
       shared ram, the MCU puts it there */
	/* taken from other set, check... */
	ROM_LOAD16_WORD( "protdata.bin", 0x00000, 0x200 , CRC(e84e328c) SHA1(ce21988980654acb573bfb7396fd2f536204ecf0) )

	ROM_REGION( 0x040000, REGION_SOUND1, 0 ) /* Samples */
	ROM_LOAD( "bcstry_u.64", 0x00000, 0x40000, CRC(23f0e0fe) SHA1(a8c3cbb6378797db353ca2873e73ff157a6f8a3c) )

	ROM_REGION( 0x200000, REGION_GFX1, 0 ) /* Tiles */
	ROM_LOAD16_BYTE( "bcstry_u.109", 0x000000, 0x20000, CRC(eb04d37a) SHA1(818dc7aafac577920d94c65e47d965dc0474d92c) ) /* tiles a plane 0 */
	ROM_CONTINUE ( 0x100000,0x20000) /* tiles a plane 1 */
	ROM_CONTINUE ( 0x040000,0x20000) /* tiles b plane 0 */
	ROM_CONTINUE ( 0x140000,0x20000) /* tiles b plane 1 */
	ROM_LOAD16_BYTE( "bcstry_u.113", 0x000001, 0x20000, CRC(746ecdd7) SHA1(afb6dbc0fb94e7ce96a9b219f5f7cd3721d1c1c4) ) /* tiles a plane 2 */
	ROM_CONTINUE ( 0x100001,0x20000) /* tiles a plane 3 */
	ROM_CONTINUE ( 0x040001,0x20000) /* tiles b plane 2 */
	ROM_CONTINUE ( 0x140001,0x20000) /* tiles b plane 3 */
	ROM_LOAD16_BYTE( "bcstry_u.110", 0x080000, 0x20000, CRC(1bfe65c3) SHA1(27dec16b271866ff336d8b25d352977ca80c35bf) ) /* tiles c plane 0 */
	ROM_CONTINUE ( 0x180000,0x20000) /* tiles c plane 1 */
	ROM_CONTINUE ( 0x0c0000,0x20000) /* tiles d plane 0 */
	ROM_CONTINUE ( 0x1c0000,0x20000) /* tiles d plane 1 */
	ROM_LOAD16_BYTE( "bcstry_u.111", 0x080001, 0x20000, CRC(c8bf3a3c) SHA1(604fc57c4d3a581016aa2516236c568488d23c77) ) /* tiles c plane 2 */
	ROM_CONTINUE ( 0x180001,0x20000) /* tiles c plane 3 */
	ROM_CONTINUE ( 0x0c0001,0x20000) /* tiles d plane 2 */
	ROM_CONTINUE ( 0x1c0001,0x20000) /* tiles d plane 3 */

	ROM_REGION( 0x400000, REGION_GFX2, ROMREGION_DISPOSE ) /* Sprites */
	ROM_LOAD16_BYTE( "bcstry_u.100", 0x000000, 0x80000, CRC(8c11cbed) SHA1(e04e53af4fe732bf9d20a9ae5c2a90b576ee0b83) ) /* b */
	ROM_LOAD16_BYTE( "bcstry_u.106", 0x000001, 0x80000, CRC(5219bcbf) SHA1(4b88eab7ffc2dc1de451ae4ee52f1536e179ea13) ) /* b */
	ROM_LOAD16_BYTE( "bcstry_u.99",  0x100000, 0x80000, CRC(cdb1af87) SHA1(df1fbda5c7ce4fbd64d6db9eb80946e06119f096) ) /* a */
	ROM_LOAD16_BYTE( "bcstry_u.105", 0x100001, 0x80000, CRC(8166b596) SHA1(cbf6f5cec5f6991bb1d4ec0ea03cd617ff38fc3b) ) /* a */
	ROM_LOAD16_BYTE( "bcstry_u.104", 0x200000, 0x80000, CRC(377c0c71) SHA1(77efa9530b1c311d93c84dd8452701414f740269) ) /* b */
	ROM_LOAD16_BYTE( "bcstry_u.108", 0x200001, 0x80000, CRC(442307ed) SHA1(71b7f19af64d9961f0f9205b86b4b0ebc13fddda) ) /* b */
	ROM_LOAD16_BYTE( "bcstry_u.102", 0x300000, 0x80000, CRC(71b40ece) SHA1(1a13dfd7615a6f61851897ebcb10fa69bc8ae525) ) /* a */
	ROM_LOAD16_BYTE( "bcstry_u.107", 0x300001, 0x80000, CRC(ab3c923a) SHA1(aaca1d2ed7b53e0933e0bd94a19458dd1598f204) ) /* a */
ROM_END

ROM_START( bcstrya )
	ROM_REGION( 0x80000, REGION_CPU1, 0 ) /* 68000 Code */
	ROM_LOAD16_BYTE( "prg1.ic35",  0x40001, 0x20000, CRC(2c55100a) SHA1(bc98a0015c99ef84ebd3fc3f7b7a3bdfd700e1da) )
	ROM_CONTINUE ( 0x00001, 0x20000)
	ROM_LOAD16_BYTE( "prg2.ic62",  0x40000, 0x20000, CRC(f54c0a96) SHA1(79a3635792a23f47fc914d1d5e118b5a643ca100) )
	ROM_CONTINUE ( 0x00000, 0x20000)

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* Z80 Code */
	ROM_LOAD( "bcstry_u.21", 0x04000, 0x4000 , CRC(3ba072d4) SHA1(8b64d3ab4c63132f2f77b2cf38a88eea1a8f11e0) )
	ROM_CONTINUE( 0x0000, 0x4000 )
	ROM_CONTINUE( 0xc000, 0x4000 )
	ROM_CONTINUE( 0x8000, 0x4000 )

	ROM_REGION( 0x10000, REGION_CPU3, 0 ) /* Intel 87C52 MCU Code */
/*	ROM_LOAD( "87c52.mcu", 0x00000, 0x10000 , NO_DUMP )*/ /* can't be dumped */ 

	ROM_REGION16_BE( 0x200, REGION_USER1, 0 ) /* Data from Shared RAM */
	/* this is not a real rom but instead the data extracted from
       shared ram, the MCU puts it there */
	ROM_LOAD16_WORD( "protdata.bin", 0x00000, 0x200 , CRC(e84e328c) SHA1(ce21988980654acb573bfb7396fd2f536204ecf0) )

	ROM_REGION( 0x040000, REGION_SOUND1, 0 ) /* Samples */
	ROM_LOAD( "bcstry_u.64", 0x00000, 0x40000, CRC(23f0e0fe) SHA1(a8c3cbb6378797db353ca2873e73ff157a6f8a3c) )

	ROM_REGION( 0x200000, REGION_GFX1, 0 ) /* Tiles */
	ROM_LOAD16_BYTE( "bcstry_u.109", 0x000000, 0x20000, CRC(eb04d37a) SHA1(818dc7aafac577920d94c65e47d965dc0474d92c) ) /* tiles a plane 0 */
	ROM_CONTINUE ( 0x100000,0x20000) /* tiles a plane 1 */
	ROM_CONTINUE ( 0x040000,0x20000) /* tiles b plane 0 */
	ROM_CONTINUE ( 0x140000,0x20000) /* tiles b plane 1 */
	ROM_LOAD16_BYTE( "bcstry_u.113", 0x000001, 0x20000, CRC(746ecdd7) SHA1(afb6dbc0fb94e7ce96a9b219f5f7cd3721d1c1c4) ) /* tiles a plane 2 */
	ROM_CONTINUE ( 0x100001,0x20000) /* tiles a plane 3 */
	ROM_CONTINUE ( 0x040001,0x20000) /* tiles b plane 2 */
	ROM_CONTINUE ( 0x140001,0x20000) /* tiles b plane 3 */
	ROM_LOAD16_BYTE( "bcstry_u.110", 0x080000, 0x20000, CRC(1bfe65c3) SHA1(27dec16b271866ff336d8b25d352977ca80c35bf) ) /* tiles c plane 0 */
	ROM_CONTINUE ( 0x180000,0x20000) /* tiles c plane 1 */
	ROM_CONTINUE ( 0x0c0000,0x20000) /* tiles d plane 0 */
	ROM_CONTINUE ( 0x1c0000,0x20000) /* tiles d plane 1 */
	ROM_LOAD16_BYTE( "bcstry_u.111", 0x080001, 0x20000, CRC(c8bf3a3c) SHA1(604fc57c4d3a581016aa2516236c568488d23c77) ) /* tiles c plane 2 */
	ROM_CONTINUE ( 0x180001,0x20000) /* tiles c plane 3 */
	ROM_CONTINUE ( 0x0c0001,0x20000) /* tiles d plane 2 */
	ROM_CONTINUE ( 0x1c0001,0x20000) /* tiles d plane 3 */

	ROM_REGION( 0x400000, REGION_GFX2, ROMREGION_DISPOSE ) /* Sprites */
	ROM_LOAD16_BYTE( "bcstry_u.100", 0x000000, 0x80000, CRC(8c11cbed) SHA1(e04e53af4fe732bf9d20a9ae5c2a90b576ee0b83) ) /* b */
	ROM_LOAD16_BYTE( "bcstry_u.106", 0x000001, 0x80000, CRC(5219bcbf) SHA1(4b88eab7ffc2dc1de451ae4ee52f1536e179ea13) ) /* b */
	ROM_LOAD16_BYTE( "bcstry_u.99",  0x100000, 0x80000, CRC(cdb1af87) SHA1(df1fbda5c7ce4fbd64d6db9eb80946e06119f096) ) /* a */
	ROM_LOAD16_BYTE( "bcstry_u.105", 0x100001, 0x80000, CRC(8166b596) SHA1(cbf6f5cec5f6991bb1d4ec0ea03cd617ff38fc3b) ) /* a */
	ROM_LOAD16_BYTE( "bcstry_u.104", 0x200000, 0x80000, CRC(377c0c71) SHA1(77efa9530b1c311d93c84dd8452701414f740269) ) /* b */
	ROM_LOAD16_BYTE( "bcstry_u.108", 0x200001, 0x80000, CRC(442307ed) SHA1(71b7f19af64d9961f0f9205b86b4b0ebc13fddda) ) /* b */
	ROM_LOAD16_BYTE( "bcstry_u.102", 0x300000, 0x80000, CRC(71b40ece) SHA1(1a13dfd7615a6f61851897ebcb10fa69bc8ae525) ) /* a */
	ROM_LOAD16_BYTE( "bcstry_u.107", 0x300001, 0x80000, CRC(ab3c923a) SHA1(aaca1d2ed7b53e0933e0bd94a19458dd1598f204) ) /* a */
ROM_END

/* SD Fight */

ROM_START( sdfight )
	ROM_REGION( 0x100000, REGION_CPU1, 0 ) /* 68000 Code */
	ROM_LOAD16_BYTE( "u818",  0xc0001, 0x20000, CRC(a60e5b22) SHA1(eda1a5de881718f78a45720c3ca43a6288a0e65d) )
	ROM_CONTINUE( 0x80001, 0x20000)
	ROM_CONTINUE( 0x40001, 0x20000)
	ROM_CONTINUE( 0x00001, 0x20000)
	ROM_LOAD16_BYTE( "u817",  0xc0000, 0x20000, CRC(9f284f4d) SHA1(f4a471fb09c2fd73692ddaa03083644493256aae) )
	ROM_CONTINUE( 0x80000, 0x20000)
	ROM_CONTINUE( 0x40000, 0x20000)
	ROM_CONTINUE( 0x00000, 0x20000)

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* Z80 Code */
	ROM_LOAD( "ua7", 0x00000, 0x10000 , CRC(c3d36da4) SHA1(7290a977bfa9a3d5e0c98a0f589d877e38aa10a1) )

	ROM_REGION( 0x10000, REGION_CPU3, 0 ) /* Intel 87C52 MCU Code */
	/*ROM_LOAD( "87c52.mcu", 0x00000, 0x10000 , NO_DUMP )*/ /* can't be dumped */ 
	ROM_REGION16_BE( 0x200, REGION_USER1, ROMREGION_ERASE00 ) /* Data from Shared RAM */
	ROM_LOAD16_WORD( "protdata.bin", 0x00000, 0x200 , CRC(efb8b822) SHA1(139c39771c057ae322d3601f7e0a58b43fa8860a) )

	ROM_REGION( 0x040000, REGION_SOUND1, 0 ) /* Samples */
	ROM_LOAD( "uc1", 0x00000, 0x40000, CRC(535cae2c) SHA1(e9d59ab23cbbc0375987ea68e170ddb1cc75cff8) )

	ROM_REGION( 0x200000, REGION_GFX1, 0 ) /* Tiles */
	ROM_LOAD16_BYTE( "9.ug11", 0x000001, 0x20000, CRC(bf809ccd) SHA1(4d648d7cdeb5ce4a918b8372dbd33c2fbf307dc0) ) /* tiles a plane 0 */
	ROM_CONTINUE ( 0x100001,0x20000) /* tiles a plane 1 */
	ROM_CONTINUE ( 0x040001,0x20000) /* tiles b plane 0 */
	ROM_CONTINUE ( 0x140001,0x20000) /* tiles b plane 1 */
	ROM_LOAD16_BYTE( "10.ug12", 0x000000, 0x20000, CRC(a5a3bfa2) SHA1(9b0d791f80f4cba14b7fab1aa7550784d6c4c4f7) ) /* tiles a plane 2 */
	ROM_CONTINUE ( 0x100000,0x20000) /* tiles a plane 3 */
	ROM_CONTINUE ( 0x040000,0x20000) /* tiles b plane 2 */
	ROM_CONTINUE ( 0x140000,0x20000) /* tiles b plane 3 */
	ROM_LOAD16_BYTE( "15.ui11", 0x080001, 0x20000, CRC(3bc8aa6d) SHA1(a8983957da5e286ec437f2fc83dfabf81fe56ca2) ) /* tiles c plane 0 */
	ROM_CONTINUE ( 0x180001,0x20000) /* tiles c plane 1 */
	ROM_CONTINUE ( 0x0c0001,0x20000) /* tiles d plane 0 */
	ROM_CONTINUE ( 0x1c0001,0x20000) /* tiles d plane 1 */
	ROM_LOAD16_BYTE( "16.ui12", 0x080000, 0x20000, CRC(71e6b78d) SHA1(a676395b2357093c4800d8520df10f7ef17cb3ee) ) /* tiles c plane 2 */
	ROM_CONTINUE ( 0x180000,0x20000) /* tiles c plane 3 */
	ROM_CONTINUE ( 0x0c0000,0x20000) /* tiles d plane 2 */
	ROM_CONTINUE ( 0x1c0000,0x20000) /* tiles d plane 3 */

	ROM_REGION( 0x400000, REGION_GFX2, ROMREGION_DISPOSE ) /* Sprites */
	ROM_LOAD16_BYTE( "11.uk2", 0x000000, 0x80000, CRC(d006fadc) SHA1(79014bc0c7909763829ba02d5434d4543b4b80e5) ) /* b */
	ROM_LOAD16_BYTE( "12.uk3", 0x000001, 0x80000, CRC(2a2f4153) SHA1(d86692ee17ad052fdd8fccded57e3e30012026f6) ) /* b */
	ROM_LOAD16_BYTE( "5.uj2",  0x100000, 0x80000, CRC(f1246cbf) SHA1(de80a8f0d29ee76e11f38d9982ffcb4fd228153a) ) /* b */
	ROM_LOAD16_BYTE( "6.uj3",  0x100001, 0x80000, CRC(d346878c) SHA1(93174f6f6cc797323c5e429bf324d4ffe081f072) ) /* b */
	ROM_LOAD16_BYTE( "13.uk4", 0x200000, 0x80000, CRC(9bc40774) SHA1(b56c57258ec9c07c7efff9c0c632390d2d5ce4e2) ) /* a */
	ROM_LOAD16_BYTE( "14.uk5", 0x200001, 0x80000, CRC(a1e61674) SHA1(a5a50f479a019b39082429fa3425a95480838f84) ) /* a */
	ROM_LOAD16_BYTE( "7.uj4",  0x300000, 0x80000, CRC(dbdece8a) SHA1(20199cc915a1f8088372682c054cac69bc3b4918) ) /* a */
	ROM_LOAD16_BYTE( "8.uj5",  0x300001, 0x80000, CRC(60be7dd1) SHA1(d212dee3acf696cac0843e968a71ec1fb9b16dc9) ) /* a */
ROM_END

/* Super Trio */

ROM_START( suprtrio )
	ROM_REGION( 0x80000, REGION_CPU1, 0 ) /* 68k */
	ROM_LOAD16_BYTE( "rom2",  0x00000, 0x40000, CRC(4102e59d) SHA1(f06f1273dbbb91fa61d84541aa124d9c88ee94c1) )
	ROM_LOAD16_BYTE( "rom1",  0x00001, 0x40000, CRC(cc3a83c3) SHA1(6f8b1b6b666ce11c02e9defcba751d88621e572d) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* Z80 */
	ROM_LOAD( "rom4l", 0x000000, 0x10000, CRC(466aa96d) SHA1(37f1ba148dbad27ed8e71a0b3434ff970fcb519f) )

	ROM_REGION( 0x100000, REGION_GFX1, ROMREGION_DISPOSE ) /* bg tiles */
	ROM_LOAD( "rom4",  0x00000, 0x20000, CRC(cd2dfae4) SHA1(1d872b5abaf72d34bd4a45f6be69aa6474887b4b) )
	ROM_CONTINUE(      0x40000, 0x20000 )
	ROM_CONTINUE(      0x20000, 0x20000 )
	ROM_CONTINUE(      0x60000, 0x20000 )
	ROM_LOAD( "rom5",  0x80000, 0x20000, CRC(4e64da64) SHA1(f2518b3d83d7fd46000ca982b2d91ce75034b411) )
	ROM_CONTINUE(      0xc0000, 0x20000 )
	ROM_CONTINUE(      0xa0000, 0x20000 )
	ROM_CONTINUE(      0xe0000, 0x20000 )

	ROM_REGION( 0x100000, REGION_GFX2, ROMREGION_DISPOSE ) /* sprites */
	ROM_LOAD16_BYTE( "rom9l", 0x00000, 0x40000, CRC(cc45f437) SHA1(fa735c3b3f96266ddfb611af6908abe72d5ae9d9) )
	ROM_LOAD16_BYTE( "rom8l", 0x00001, 0x40000, CRC(9bc90169) SHA1(3bc0d34911f063ff79c529346f41695376428f75) )
	ROM_LOAD16_BYTE( "rom7l", 0x80000, 0x40000, CRC(bfc7c756) SHA1(e533f633dec63c27ac78f170e222e590e815a022) )
	ROM_LOAD16_BYTE( "rom6l", 0x80001, 0x40000, CRC(bb3499af) SHA1(1a0a6a63227e8ad28aa23afc6d076037518b4802) )

	ROM_REGION( 0xb0000, REGION_SOUND1, 0 ) /* samples */
	ROM_LOAD( "rom3h", 0x00000, 0x30000, CRC(34ea7ec9) SHA1(1f80a2c7ed4fb13610731732b11268d1d7be5bb2) )
	ROM_CONTINUE(      0x40000, 0x50000 )
	ROM_LOAD( "rom3l", 0x90000, 0x20000, CRC(1b73233b) SHA1(5d82bbdc31d99f8d77bdb5c2f6e5e23037b4bca0) )
ROM_END


/******************************************************************************/

void tumblep_patch_code(UINT16 offset)
{
	/* A hack which enables all Dip Switches effects */
	UINT16 *RAM = (UINT16 *)memory_region(REGION_CPU1);
	RAM[(offset + 0)/2] = 0x0240;
	RAM[(offset + 2)/2] = 0xffff;	/* andi.w  #$f3ff, D0 */
}


static void tumblepb_gfx1_decrypt(void)
{
	UINT8 *rom = memory_region(REGION_GFX1);
	int len = memory_region_length(REGION_GFX1);
	int i;

	/* gfx data is in the wrong order */
	for (i = 0;i < len;i++)
	{
		if ((i & 0x20) == 0)
		{
			int t = rom[i]; rom[i] = rom[i + 0x20]; rom[i + 0x20] = t;
		}
	}
	/* low/high half are also swapped */
	for (i = 0;i < len/2;i++)
	{
		int t = rom[i]; rom[i] = rom[i + len/2]; rom[i + len/2] = t;
	}
}

static DRIVER_INIT( tumblep )
{
	deco56_decrypt(REGION_GFX1);

	#if TUMBLEP_HACK
	tumblep_patch_code(0x000132);
	#endif
}

static DRIVER_INIT( tumblepb )
{
	tumblepb_gfx1_decrypt();

	#if TUMBLEP_HACK
	tumblep_patch_code(0x000132);
	#endif
}

static DRIVER_INIT( jumpkids )
{
	tumblepb_gfx1_decrypt();

	#if TUMBLEP_HACK
	tumblep_patch_code(0x00013a);
	#endif
}

static DRIVER_INIT( fncywld )
{
	#if FNCYWLD_HACK
	/* This is a hack to allow you to use the extra features
         of the 2 first "Unused" Dip Switch (see notes above). */
	UINT16 *RAM = (UINT16 *)memory_region(REGION_CPU1);
	RAM[0x0005fa/2] = 0x4e71;
	RAM[0x00060a/2] = 0x4e71;
	#endif

	tumblepb_gfx1_decrypt();
}

static DRIVER_INIT( magipur )
{
	
	UINT16 *src = (UINT16 *)memory_region(REGION_CPU1);
	/* copy vector table? game expects RAM at 0, and ROM at f00000?! */
	memcpy(mainram, src, 0x80);
	memcpy(maincpu, memory_region(REGION_CPU1), memory_region_length(REGION_CPU1));

    tumblepb_gfx1_decrypt();
	
}


static READ16_HANDLER( bcstory_1a0_read )
{
/*  printf("bcstory_io %06x\n",activecpu_get_pc()); */
	if (activecpu_get_pc()==0x0560) return 0x1a0;
	else return readinputport(2);
}

static DRIVER_INIT ( bcstory )
{
	tumblepb_gfx1_decrypt();
	install_mem_read16_handler(0, 0x180008, 0x180009, bcstory_1a0_read ); /* io should be here?? */
}



static DRIVER_INIT( htchctch )
{

/*  UINT16 *HCROM = (UINT16*)memory_region(REGION_CPU1); */
	UINT16 *PROTDATA = (UINT16*)memory_region(REGION_USER1);
	int i;
	/* simulate RAM initialization done by the protection MCU */
	/* verified on real hardware */
/*  static UINT16 htchctch_mcu68k[] = */
/*  { */
/*      moved to protdata.bin file .. */ 
/*  }; */


/*  for (i = 0;i < sizeof(htchctch_mcu68k)/sizeof(htchctch_mcu68k[0]);i++) */
/*      tumblep_mainram[0x000/2 + i] = htchctch_mcu68k[i]; */

	for (i = 0;i < 0x200/2;i++)
		tumblep_mainram[0x000/2 + i] = PROTDATA[i];



	tumblepb_gfx1_decrypt();

/* trojan.. */
#if 0
	/* patch the irq 6 vector */
	HCROM[0x00078/2] = 0x0001;
	HCROM[0x0007a/2] = 0xe000;

	/* our new interrupt code */

	/* put registers on stack */
	HCROM[0x1e000/2] = 0x48e7;
	HCROM[0x1e002/2] = 0xfffe;

	/* put the address we want to copy FROM in A0 */
	HCROM[0x1e004/2] = 0x41f9;
	HCROM[0x1e006/2] = 0x0012;
	HCROM[0x1e008/2] = 0x0000;

	/* put the address we want to copy TO in A1 */
	HCROM[0x1e00a/2] = 0x43f9;
	HCROM[0x1e00c/2] = 0x0012;
	HCROM[0x1e00e/2] = 0x2000;

	/* put the number of words we want to copy into D0 */
	HCROM[0x1e010/2] = 0x203c;
	HCROM[0x1e012/2] = 0x0000;
	HCROM[0x1e014/2] = 0x0100;

	/* copy a word */
	HCROM[0x1e016/2] = 0x32d8;

	/* decrease counter d0 */
	HCROM[0x1e018/2] = 0x5380;

	/* compare d0 to 0 */
	HCROM[0x1e01a/2] = 0x0c80;
	HCROM[0x1e01c/2] = 0x0000;
	HCROM[0x1e01e/2] = 0x0000;

	/* if its not 0 then branch back */
	HCROM[0x1e020/2] = 0x66f4;




	/* jump to drawing subroutine */
	HCROM[0x1e022/2] = 0x4eb9;
	HCROM[0x1e024/2] = 0x0001;
	HCROM[0x1e026/2] = 0xe100;

	/* get back registers from stack*/
	HCROM[0x1e028/2] = 0x4cdf;
	HCROM[0x1e02a/2] = 0x7fff;

	/* jump to where the interrupt vector was copied to */
	HCROM[0x1e02c/2] = 0x4ef9;
	HCROM[0x1e02e/2] = 0x0012;
	HCROM[0x1e030/2] = 0x2000;
	/* we're back in the game code */


	/* these subroutines are called from the new interrupt code above, i use them to draw */

	/* DRAWING SUBROUTINE */

	/* put the address we want to write to in A0 */
	HCROM[0x1e100/2] = 0x41f9;
	HCROM[0x1e102/2] = 0x0032;
	HCROM[0x1e104/2] = 0x0104;

	/* put the character we want to draw into D0 */
	/* this bit isn't needed .. we end up using d4 then copying it over */
	HCROM[0x1e106/2] = 0x203c;
	HCROM[0x1e108/2] = 0x0000;
	HCROM[0x1e10a/2] = 0x0007;

	/* put the address we to read to in A2 */
	HCROM[0x1e10c/2] = 0x45f9;
	HCROM[0x1e10e/2] = 0x0012;
/*  HCROM[0x1e110/2] = 0x2000; */
	HCROM[0x1e110/2] = 0x2000+0x60+0x60+0x60+0x60+0x60;

	/* put the number of rows into D3 */
	HCROM[0x1e112/2] = 0x263c;
	HCROM[0x1e114/2] = 0x0000;
	HCROM[0x1e116/2] = 0x000c;

	/* put the number of bytes per row into D2 */
	HCROM[0x1e118/2] = 0x243c;
	HCROM[0x1e11a/2] = 0x0000;
	HCROM[0x1e11c/2] = 0x0008;


	/* move content of a2 to d4 (byte) */
	HCROM[0x1e11e/2] = 0x1812;

	HCROM[0x1e120/2] = 0xe84c; /* shift d4 right by 4 */

	HCROM[0x1e122/2] = 0x0244; /* mask with 0x000f */
	HCROM[0x1e124/2] = 0x000f; /* */

	HCROM[0x1e126/2] = 0x3004; /* d4 -> d0 */

	/* jump to character draw to draw first bit */
	HCROM[0x1e128/2] = 0x4eb9;
	HCROM[0x1e12a/2] = 0x0001;
	HCROM[0x1e12c/2] = 0xe200;

	/* add 2 to draw address a0 */
	HCROM[0x1e12e/2] = 0xd1fc;
	HCROM[0x1e130/2] = 0x0000;
	HCROM[0x1e132/2] = 0x0002;


	/* move content of a2 to d4 (byte) */
	HCROM[0x1e134/2] = 0x1812;

	HCROM[0x1e136/2] = 0x0244; /* mask with 0x000f */
	HCROM[0x1e138/2] = 0x000f; /* */

	HCROM[0x1e13a/2] = 0x3004; /* d4 -> d0 */

	/* jump to character draw to draw second bit */
	HCROM[0x1e13c/2] = 0x4eb9;
	HCROM[0x1e13e/2] = 0x0001;
	HCROM[0x1e140/2] = 0xe200;

	/* add 2 to draw address a0 */
	HCROM[0x1e142/2] = 0xd1fc;
	HCROM[0x1e144/2] = 0x0000;
	HCROM[0x1e146/2] = 0x0002;

	/* add 1 to read address a2 */
	HCROM[0x1e148/2] = 0xd5fc;
	HCROM[0x1e14a/2] = 0x0000;
	HCROM[0x1e14c/2] = 0x0001;

/* brr */
	/* decrease counter d2 */
	HCROM[0x1e14e/2] = 0x5382;

	/* compare d2 to 0 */
	HCROM[0x1e150/2] = 0x0c82;
	HCROM[0x1e152/2] = 0x0000;
	HCROM[0x1e154/2] = 0x0000;

	/* if its not 0 then branch back */
	HCROM[0x1e156/2] = 0x66c6;

	/* add 0xe0 to draw address a0 (0x100-0x20) */
	HCROM[0x1e158/2] = 0xd1fc;
	HCROM[0x1e15a/2] = 0x0000;
	HCROM[0x1e15c/2] = 0x00e0;

	/* decrease counter d2 */
	HCROM[0x1e15e/2] = 0x5383;

	/* compare d2 to 0 */
	HCROM[0x1e160/2] = 0x0c83;
	HCROM[0x1e162/2] = 0x0000;
	HCROM[0x1e164/2] = 0x0000;

	/* if its not 0 then branch back */
	HCROM[0x1e166/2] = 0x66b0;

	HCROM[0x1e168/2] = 0x4e75; /* rts */

	/* DRAW CHARACTER SUBROUTINE, note, this won't restore a1,d1, don't other places! */

	/* move address into A0->A1 for use by this subroutine */
	HCROM[0x1e200/2] = 0x2248;

	/* move address into D0->D1 for top half of character */
	HCROM[0x1e202/2] = 0x2200;

	/* add 0x30 to d1 to get the REAL tile code */
	HCROM[0x1e204/2] = 0x0681;
	HCROM[0x1e206/2] = 0x0000;
	HCROM[0x1e208/2] = 0x0030;

	/* or with 0xf000 to add the tile attribute */
	HCROM[0x1e20a/2] = 0x0081;
	HCROM[0x1e20c/2] = 0x0000;
	HCROM[0x1e20e/2] = 0xf000;

	/* write d1 -> a1 for TOP half */
	HCROM[0x1e210/2] = 0x32c1; /* not ideal .. we don't need to increase a1 */

	/* move address into A0->A1 for use by this subroutine */
	HCROM[0x1e212/2] = 0x2248;

	/* add 0x80 to the address so we have the bottom location */
	HCROM[0x1e214/2] = 0xd2fc;
	HCROM[0x1e216/2] = 0x0080;

	/* move address into D0->D1 for bottom  half of character */
	HCROM[0x1e218/2] = 0x2200;

	/* add 0x54 to d1 to get the REAL tile code for bottom half */
	HCROM[0x1e21a/2] = 0x0681;
	HCROM[0x1e21c/2] = 0x0000;
	HCROM[0x1e21e/2] = 0x0054;

	/* or with 0xf000 to add the tile attribute */
	HCROM[0x1e220/2] = 0x0081;
	HCROM[0x1e222/2] = 0x0000;
	HCROM[0x1e224/2] = 0xf000;

	/* write d1 -> a1 for BOTTOM half */
	HCROM[0x1e226/2] = 0x32c1; /* not ideal .. we don't need to increase a1 */


	HCROM[0x1e228/2] = 0x4e75;

	install_mem_write16_handler(0, 0x140000, 0x1407ff, MWA16_NOP ); /* kill palette writes as the interrupt code we don't have controls them */


	{
		FILE *fp;

		fp=fopen("cookbib", "w+b");
		if (fp)
		{
			fwrite(HCROM, 0x40000, 1, fp);
			fclose(fp);
		}
	}
#endif

}

DRIVER_INIT( chokchok )
{
	init_htchctch();

	/* different palette format, closer to tumblep */
	install_mem_write16_handler(0, 0x140000, 0x140fff, paletteram16_xxxxBBBBGGGGRRRR_word_w);

	/* slightly different banking */
    install_mem_write16_handler(0, 0x100002, 0x100003, chokchok_tilebank_w);
}

static void suprtrio_decrypt_code(void)
{
	UINT16 *rom = (UINT16 *)memory_region(REGION_CPU1);
	UINT16 *buf = malloc(0x80000);
	int i;

	/* decrypt main ROMs */
	if (buf)
	{
		memcpy(buf,rom,0x80000);
		for (i = 0;i < 0x40000;i++)
		{
			int j = i ^ 0x06;
			if ((i & 1) == 0) j ^= 0x02;
			if ((i & 3) == 0) j ^= 0x08;
			rom[i] = buf[j];
		}
		free(buf);
	}
}

static void suprtrio_decrypt_gfx(void)
{
	UINT16 *rom = (UINT16 *)memory_region(REGION_GFX1);
	UINT16 *buf = malloc(0x100000);
	int i;

	/* decrypt tiles */
	if (buf)
	{
		memcpy(buf,rom,0x100000);
		for (i = 0;i < 0x80000;i++)
		{
			int j = i ^ 0x02;
			if (i & 1) j ^= 0x04;
			rom[i] = buf[j];
		}
		free(buf);
	}
}

DRIVER_INIT( suprtrio )
{
	suprtrio_decrypt_code();
	suprtrio_decrypt_gfx();
}


/******************************************************************************/

GAME( 1991, tumblep,  0,       tumblep,   tumblep,  tumblep,  ROT0, "Data East Corporation", "Tumble Pop (World)" )
GAME( 1991, tumblepj, tumblep, tumblep,   tumblep,  tumblep,  ROT0, "Data East Corporation", "Tumble Pop (Japan)" )
GAMEX(1991, tumblepb, tumblep, tumblepb,  tumblep,  tumblepb, ROT0, "bootleg", "Tumble Pop (bootleg set 1)", GAME_IMPERFECT_SOUND )
GAMEX(1991, tumblep2, tumblep, tumblepb,  tumblep,  tumblepb, ROT0, "bootleg", "Tumble Pop (bootleg set 2)", GAME_IMPERFECT_SOUND )
GAMEX(1993, jumpkids, 0,       jumpkids,  tumblep,  jumpkids, ROT0, "Comad",   "Jump Kids", GAME_NO_SOUND )
GAME( 1996, fncywld,  0,       fncywld,   fncywld,  fncywld,  ROT0, "Unico",   "Fancy World - Earth of Crisis" ) /* game says 1996, testmode 1995? */
GAME( 1996, magipur,  0,       magipur,   magipur,  magipur,  ROT0, "Unico",   "Magic Purple" )
GAME( 1995, htchctch, 0,       htchctch,  htchctch, htchctch, ROT0, "SemiCom", "Hatch Catch" ) /* not 100% sure about gfx offsets */
GAME( 1995, cookbib,  0,       cookbib,   cookbib,  htchctch, ROT0, "SemiCom", "Cookie & Bibi" ) /* not 100% sure about gfx offsets */
GAMEX(1995, chokchok, 0,       cookbib,   chokchok, chokchok, ROT0, "SemiCom", "Choky! Choky!", GAME_IMPERFECT_GRAPHICS ) /* corruption during attract mode (tmap disable?) */
GAMEX(1997, bcstry,   0,       bcstory,   bcstory,  bcstory,  ROT0, "SemiCom", "B.C. Story (set 1)", GAME_IMPERFECT_GRAPHICS) /* gfx offsets? */
GAMEX(1997, bcstrya,  bcstry,  bcstory,   bcstory,  bcstory,  ROT0, "SemiCom", "B.C. Story (set 2)", GAME_IMPERFECT_GRAPHICS) /* gfx offsets? */
GAME( 1996, sdfight,  0,       sdfight,   sdfight,  bcstory,  ROT0, "SemiCom", "SD Fighter (Korea)")
GAME( 1994, suprtrio, 0,       suprtrio,  suprtrio, suprtrio, ROT0, "Gameace", "Super Trio" )
