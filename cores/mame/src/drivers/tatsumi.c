/***************************************************************************

    Apache 3                                            ATF-011
	Round Up 5											ATC-011
	Cycle Warriors										ABA-011
    Big Fight                                           ABA-011
	
	Incredibly complex hardware!  These are all different boards, but share
	a similar sprite chip (TZB215 on Apache 3, TZB315 on others).  Other
	graphics (road, sky, bg/fg layers) all differ between games.

	Todo:
		Sprite rotation
		Finish road layer (Round Up 5)
		Implement road layer (Apache 3)
		BG layer (Round Up 5) - May be driven by missing VRAM data
		Round Up 5 always boots with a coin inserted
		Round Up 5 doesn't survive a reset
		Dip switches
		Various other things..

	Emulation by Bryan McPhail, mish@tendril.co.uk


	Cycle Warriors Board Layout

	ABA-011


			6296             CW24A                   5864
								CW25A                   5864
			YM2151                  50MHz

								TZ8315                 CW26A
													5864
		TC51821  TC51832                               D780C-1
		TC51821  TC51832
		TC51821  TC51832                     16MHz
		TC51821  TC51832

		CW00A   CW08A
		CW01A   CW09A
		CW02A   CW10A
		CW03A   CW11A            68000-12              81C78
		CW04A   CW12A                                  81C78
		CW05A   CW13A                CW16B  CW18B      65256
		CW06A   CW14A                CW17A  CW19A      65256
		CW07A   CW15A                       CW20A
											CW21       65256
								68000-12   CW22A      65256
											CW23

	ABA-012

							HD6445


							51832
							51832
							51832
							51832

							CW28
							CW29
							CW30

	CW27

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "tatsumi.h"

static data16_t *cyclwarr_cpua_ram, *cyclwarr_cpub_ram;
data16_t *tatsumi_c_ram, *apache3_g_ram;
data16_t *roundup5_d0000_ram, *roundup5_e0000_ram;
data8_t *tatsumi_rom_sprite_lookup1, *tatsumi_rom_sprite_lookup2;
data8_t *tatsumi_rom_clut0, *tatsumi_rom_clut1;
data8_t *roundup5_unknown0, *roundup5_unknown1, *roundup5_unknown2;
UINT16 bigfight_a20000[8], bigfight_a40000[2], bigfight_a60000[2];
extern UINT16 bigfight_bank;
data8_t *apache3_bg_ram;

/***************************************************************************/

/*static READ16_HANDLER(cyclwarr_cpu_b_r) { return cyclwarr_cpub_ram[offset+0x800]; } */
/*static WRITE16_HANDLER(cyclwarr_cpu_b_w){ COMBINE_DATA(&cyclwarr_cpub_ram[offset+0x800]); } */
static READ16_HANDLER(cyclwarr_cpu_bb_r){ return cyclwarr_cpub_ram[offset]; }
static WRITE16_HANDLER(cyclwarr_cpu_bb_w) { COMBINE_DATA(&cyclwarr_cpub_ram[offset]); }
static READ16_HANDLER(cyclwarr_palette_r) { return paletteram16[offset]; }
static READ16_HANDLER(cyclwarr_sprite_r) { return spriteram16[offset]; }
static WRITE16_HANDLER(cyclwarr_sprite_w) { COMBINE_DATA(&spriteram16[offset]); }
static READ16_HANDLER(cyclwarr_input_r) { return readinputport(offset); }
static READ16_HANDLER(cyclwarr_input2_r) { return readinputport(offset+4); }
static WRITE16_HANDLER(bigfight_a20000_w) { COMBINE_DATA(&bigfight_a20000[offset]); }
static WRITE16_HANDLER(bigfight_a40000_w) { COMBINE_DATA(&bigfight_a40000[offset]); }
static WRITE16_HANDLER(bigfight_a60000_w) { COMBINE_DATA(&bigfight_a60000[offset]); }

/***************************************************************************/

static WRITE16_HANDLER(cyclwarr_sound_w)
{
	soundlatch_w(0, data >> 8);
	cpu_set_nmi_line(2, PULSE_LINE);
}

static MEMORY_READ16_START( bigfight_68000a_readmem )
    { 0x000000, 0x00ffff, MRA16_RAM },
	{ 0x03e000, 0x03efff, MRA16_RAM },
	{ 0x040000, 0x04ffff, cyclwarr_cpu_bb_r },
	{ 0x080000, 0x08ffff, cyclwarr_videoram1_r },
	{ 0x090000, 0x09ffff, cyclwarr_videoram0_r },
	{ 0x0b9002, 0x0b9009, cyclwarr_input_r }, /* Coins, P1 input, P2 input, dip 3 */
	{ 0x0ba000, 0x0ba007, cyclwarr_input2_r }, /* Dip 1, Dip 2, P3 input, P4 input */
	{ 0x0ba008, 0x0ba009, cyclwarr_control_r },
	{ 0x0c0000, 0x0c3fff, cyclwarr_sprite_r },
	{ 0x0d0000, 0x0d3fff, cyclwarr_palette_r },
	{ 0x100000, 0x17ffff, MRA16_BANK2 }, /* CPU A ROM */
	{ 0x200000, 0x27ffff, MRA16_BANK1 }, /* CPU B ROM */
MEMORY_END

static MEMORY_WRITE16_START( bigfight_68000a_writemem )
    { 0x000000, 0x00ffff, MWA16_RAM, &cyclwarr_cpua_ram },
	{ 0x03e000, 0x03efff, MWA16_RAM },
	{ 0x040000, 0x04ffff, cyclwarr_cpu_bb_w },
	{ 0x080000, 0x08ffff, cyclwarr_videoram1_w, &cyclwarr_videoram1 },
	{ 0x090000, 0x09ffff, cyclwarr_videoram0_w, &cyclwarr_videoram0 },
	{ 0x0a2000, 0x0a2007, bigfight_a20000_w },
	{ 0x0a4000, 0x0a4001, bigfight_a40000_w },
	{ 0x0a6000, 0x0a6001, bigfight_a60000_w },
	{ 0x0ba008, 0x0ba009, cyclwarr_control_w },
    { 0x0b8000, 0x0b8001, cyclwarr_sound_w },
	{ 0x0c0000, 0x0c3fff, cyclwarr_sprite_w, &spriteram16 },
	{ 0x0ca000, 0x0ca1ff, tatsumi_sprite_control_w, &tatsumi_sprite_control_ram },
	{ 0x0d0000, 0x0d3fff, paletteram16_xRRRRRGGGGGBBBBB_word_w, &paletteram16 },
MEMORY_END

static MEMORY_READ16_START( bigfight_68000b_readmem )
    { 0x000000, 0x00ffff, MRA16_RAM },
	{ 0x080000, 0x08ffff, cyclwarr_videoram1_r },
	{ 0x090000, 0x09ffff, cyclwarr_videoram0_r },
	{ 0x0b9002, 0x0b9009, cyclwarr_input_r }, /* Coins, P1 input, P2 input, dip 3 */
	{ 0x0ba000, 0x0ba007, cyclwarr_input2_r }, /* Dip 1, Dip 2, P3 input, P4 input */
	{ 0x0ba008, 0x0ba009, cyclwarr_control_r },
	{ 0x0c0000, 0x0c3fff, cyclwarr_sprite_r },
	{ 0x0d0000, 0x0d3fff, cyclwarr_palette_r },	
    { 0x100000, 0x17ffff, MRA16_BANK2 }, /* CPU A ROM */
	{ 0x200000, 0x27ffff, MRA16_BANK1 }, /* CPU B ROM */
MEMORY_END

static MEMORY_WRITE16_START( bigfight_68000b_writemem )
    { 0x000000, 0x00ffff, MWA16_RAM, &cyclwarr_cpub_ram },
	{ 0x080000, 0x08ffff, cyclwarr_videoram1_w },
	{ 0x090000, 0x09ffff, cyclwarr_videoram0_w },
	{ 0x0a2000, 0x0a2007, bigfight_a20000_w },
	{ 0x0a4000, 0x0a4001, bigfight_a40000_w },
	{ 0x0a6000, 0x0a6001, bigfight_a60000_w },
	{ 0x0c0000, 0x0c3fff, cyclwarr_sprite_w },
	{ 0x0ca000, 0x0ca1ff, tatsumi_sprite_control_w },
	{ 0x0d0000, 0x0d3fff, paletteram16_xRRRRRGGGGGBBBBB_word_w },
MEMORY_END

static MEMORY_READ16_START( cyclwarr_68000a_readmem )
    { 0x000000, 0x00ffff, MRA16_RAM },
	{ 0x03e000, 0x03efff, MRA16_RAM },
	{ 0x040000, 0x043fff, cyclwarr_cpu_bb_r },
	{ 0x080000, 0x08ffff, cyclwarr_videoram1_r },
	{ 0x090000, 0x09ffff, cyclwarr_videoram0_r },
	{ 0x0b9002, 0x0b9009, cyclwarr_input_r }, /* Coins, P1 input, P2 input, dip 3 */
	{ 0x0ba000, 0x0ba007, cyclwarr_input2_r }, /* Dip 1, Dip 2, P3 input, P4 input */
	{ 0x0ba008, 0x0ba009, cyclwarr_control_r },
	{ 0x0c0000, 0x0c3fff, cyclwarr_sprite_r },
	{ 0x0d0000, 0x0d3fff, cyclwarr_palette_r },
	{ 0x140000, 0x1bffff, MRA16_BANK2 }, /* CPU B ROM */
	{ 0x2c0000, 0x33ffff, MRA16_BANK1 }, /* CPU A ROM */
MEMORY_END

static MEMORY_WRITE16_START( cyclwarr_68000a_writemem )
    { 0x000000, 0x00ffff, MWA16_RAM, &cyclwarr_cpua_ram },
	{ 0x03e000, 0x03efff, MWA16_RAM },
	{ 0x040000, 0x043fff, cyclwarr_cpu_bb_w },
	{ 0x080000, 0x08ffff, cyclwarr_videoram1_w, &cyclwarr_videoram1 },
	{ 0x090000, 0x09ffff, cyclwarr_videoram0_w, &cyclwarr_videoram0 },
	{ 0x0a2000, 0x0a2007, bigfight_a20000_w },
	{ 0x0a4000, 0x0a4001, bigfight_a40000_w },
	{ 0x0a6000, 0x0a6001, bigfight_a60000_w },
	{ 0x0ba008, 0x0ba009, cyclwarr_control_w },
    { 0x0b8000, 0x0b8001, cyclwarr_sound_w },
	{ 0x0c0000, 0x0c3fff, cyclwarr_sprite_w, &spriteram16 },
	{ 0x0ca000, 0x0ca1ff, tatsumi_sprite_control_w, &tatsumi_sprite_control_ram },
	{ 0x0d0000, 0x0d3fff, paletteram16_xRRRRRGGGGGBBBBB_word_w, &paletteram16 },
MEMORY_END

static MEMORY_READ16_START( cyclwarr_68000b_readmem )
    { 0x000000, 0x00ffff, MRA16_RAM },
	{ 0x080000, 0x08ffff, cyclwarr_videoram1_r },
	{ 0x090000, 0x09ffff, cyclwarr_videoram0_r },
	{ 0x0b9002, 0x0b9009, cyclwarr_input_r }, /* Coins, P1 input, P2 input, dip 3 */
	{ 0x0ba000, 0x0ba007, cyclwarr_input2_r }, /* Dip 1, Dip 2, P3 input, P4 input */
	{ 0x0ba008, 0x0ba009, cyclwarr_control_r },
	{ 0x0c0000, 0x0c3fff, cyclwarr_sprite_r },
	{ 0x0d0000, 0x0d3fff, cyclwarr_palette_r },
	{ 0x140000, 0x1bffff, MRA16_BANK2 }, /* CPU B ROM */
	{ 0x2c0000, 0x33ffff, MRA16_BANK1 }, /* CPU A ROM */
MEMORY_END

static MEMORY_WRITE16_START( cyclwarr_68000b_writemem )
    { 0x000000, 0x00ffff, MWA16_RAM, &cyclwarr_cpub_ram },
	{ 0x080000, 0x08ffff, cyclwarr_videoram1_w },
	{ 0x090000, 0x09ffff, cyclwarr_videoram0_w },
	{ 0x0a2000, 0x0a2007, bigfight_a20000_w },
	{ 0x0a4000, 0x0a4001, bigfight_a40000_w },
	{ 0x0a6000, 0x0a6001, bigfight_a60000_w },
	{ 0x0c0000, 0x0c3fff, cyclwarr_sprite_w },
	{ 0x0ca000, 0x0ca1ff, tatsumi_sprite_control_w },
	{ 0x0d0000, 0x0d3fff, paletteram16_xRRRRRGGGGGBBBBB_word_w },
MEMORY_END

static MEMORY_READ_START( readmem_bigfight_c )
    { 0x0000, 0xdfff, MRA_ROM },
	{ 0xe000, 0xffef, MRA_RAM }, /* maybe less than this... */
	{ 0xfff1, 0xfff1, tatsumi_hack_ym2151_r },/*YM2151_status_port_0_r) */
	{ 0xfff4, 0xfff4, tatsumi_hack_oki_r },/* OKIM6295_status_0_r) */
    { 0xfffc, 0xfffc, soundlatch_r },
MEMORY_END

static MEMORY_WRITE_START( writemem_bigfight_c )
    { 0x0000, 0xdfff, MWA_ROM },
	{ 0xe000, 0xffef, MWA_RAM },
	{ 0xfff0, 0xfff0, YM2151_register_port_0_w },
	{ 0xfff1, 0xfff1, YM2151_data_port_0_w },
	{ 0xfff4, 0xfff4, OKIM6295_data_0_w },
    { 0xfffe, 0xfffe, MWA_NOP },
MEMORY_END

static MEMORY_READ_START( readmem_roundup5 )
    { 0x00000, 0x07fff, MRA_RAM },
	{ 0x08000, 0x0bfff, MRA_RAM },
	{ 0x0d000, 0x0d000, input_port_3_r }, /* Dip 1 */
	{ 0x0d001, 0x0d001, input_port_4_r }, /* Dip 2 */
	{ 0x0f000, 0x0ffff, MRA_RAM },
	{ 0x10000, 0x1ffff, roundup_v30_z80_r },
	{ 0x20000, 0x2ffff, tatsumi_v30_68000_r },
	{ 0x30000, 0x3ffff, roundup5_vram_r },
	{ 0x80000, 0xfffff, MRA_ROM },
MEMORY_END

static MEMORY_WRITE_START( writemem_roundup5 )
    { 0x00000, 0x07fff, MWA_RAM },
	{ 0x08000, 0x0bfff, roundup5_text_w, &videoram },
	{ 0x0c000, 0x0c003, roundup5_crt_w },
	{ 0x0d400, 0x0d40f, MWA_RAM, &roundup5_unknown0 },
	{ 0x0d800, 0x0d801, MWA_RAM, &roundup5_unknown1 }, /* VRAM2 X scroll (todo) */
	{ 0x0dc00, 0x0dc01, MWA_RAM, &roundup5_unknown2 }, /* VRAM2 Y scroll (todo) */
	{ 0x0e000, 0x0e001, roundup5_control_w },
	{ 0x0f000, 0x0ffff, roundup5_palette_w, &paletteram },
	{ 0x10000, 0x1ffff, roundup_v30_z80_w },
	{ 0x20000, 0x23fff, tatsumi_v30_68000_w },
	{ 0x30000, 0x3ffff, roundup5_vram_w },
	{ 0x80000, 0xfffff, MWA_ROM },
MEMORY_END

static MEMORY_READ16_START( readmem_roundup5_sub )
    { 0x00000, 0x7ffff, MRA16_ROM },
	{ 0x80000, 0x83fff, MRA16_RAM },
	{ 0x90000, 0x93fff, MRA16_RAM },
	{ 0xa0000, 0xa0fff, MRA16_RAM },
	{ 0xb0000, 0xb0fff, MRA16_RAM },
	{ 0xc0000, 0xc0fff, MRA16_RAM },
MEMORY_END

static MEMORY_WRITE16_START( writemem_roundup5_sub )
    { 0x00000, 0x7ffff, MWA16_ROM },
	{ 0x80000, 0x83fff, MWA16_RAM, &tatsumi_68k_ram },
	{ 0x90000, 0x93fff, MWA16_RAM, &spriteram16 },
	{ 0x9a000, 0x9a1ff, tatsumi_sprite_control_w, &tatsumi_sprite_control_ram },
	{ 0xa0000, 0xa0fff, MWA16_RAM, &roundup_r_ram }, /* Road control data */
	{ 0xb0000, 0xb0fff, MWA16_RAM, &roundup_p_ram }, /* Road pixel data */
	{ 0xc0000, 0xc0fff, MWA16_RAM, &roundup_l_ram }, /* Road colour data */
	{ 0xd0002, 0xd0003, roundup5_d0000_w, &roundup5_d0000_ram },
	{ 0xe0000, 0xe0001, roundup5_e0000_w, &roundup5_e0000_ram },
MEMORY_END

static MEMORY_READ_START( readmem_roundup5_sound )
    { 0x0000, 0xdfff, MRA_ROM },
	{ 0xe000, 0xffef, MRA_RAM }, /* maybe less than this... */
	{ 0xfff1, 0xfff1, tatsumi_hack_ym2151_r }, /*YM2151_status_port_0_r) */
	{ 0xfff4, 0xfff4, OKIM6295_status_0_r }, /*OKIM6295_status_0_r) */
	{ 0xfff8, 0xfff8, input_port_0_r },
	{ 0xfff9, 0xfff9, input_port_1_r },
	{ 0xfffc, 0xfffc, input_port_2_r },
MEMORY_END

static MEMORY_WRITE_START( writemem_roundup5_sound )
    { 0x0000, 0xdfff, MWA_ROM },
	{ 0xe000, 0xffef, MWA_RAM },
	{ 0xfff0, 0xfff0, YM2151_register_port_0_w },
	{ 0xfff1, 0xfff1, YM2151_data_port_0_w },
	{ 0xfff4, 0xfff4, OKIM6295_data_0_w },
	{ 0xfff9, 0xfff9, MWA_NOP },/*irq ack? */
	{ 0xfffa, 0xfffa, MWA_NOP }, /*irq ack? */
MEMORY_END

/******************************************************************************/

INPUT_PORTS_START( bigfight )
	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN4 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_SERVICE2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_SERVICE3 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_SERVICE4 )

	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER1)
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1)
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1)
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1)
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1)
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1)
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1)

	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2)
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2)
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2)
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2)
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2)
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2)
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2)

    PORT_START
	PORT_DIPNAME( 0x0001, 0x0001, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0001, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0002, 0x0000, "Player Select" )
	PORT_DIPSETTING(      0x0002, "Coin Slot" )
	PORT_DIPSETTING(      0x0000, "Select SW" )
	PORT_DIPNAME( 0x0004, 0x0004, DEF_STR( Service_Mode ) ) 
	PORT_DIPSETTING(      0x0004, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0008, 0x0008, "Hardware Test Mode" )
	PORT_DIPSETTING(      0x0008, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )

	PORT_START
	PORT_DIPNAME( 0x000f, 0x000f, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 5C_1C ) )
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
	PORT_DIPSETTING(      0x0009, DEF_STR( 1C_7C ) )
	PORT_DIPNAME( 0x00f0, 0x00f0, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 6C_1C ) )
	PORT_DIPSETTING(      0x0010, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(      0x0050, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0080, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0040, DEF_STR( 3C_2C ) )
	PORT_DIPSETTING(      0x00f0, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0030, DEF_STR( 3C_4C ) )
	PORT_DIPSETTING(      0x0070, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(      0x00e0, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x0060, DEF_STR( 2C_5C ) )
	PORT_DIPSETTING(      0x00d0, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x00c0, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(      0x00b0, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(      0x00a0, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(      0x0090, DEF_STR( 1C_7C ) )

	PORT_START
	PORT_DIPNAME( 0x0003, 0x0003, DEF_STR( Lives ) )
	PORT_DIPSETTING(      0x0000, "1" )
	PORT_DIPSETTING(      0x0003, "2" )
	PORT_DIPSETTING(      0x0002, "3" )
	PORT_DIPSETTING(      0x0001, "4" )
	PORT_DIPNAME( 0x0004, 0x0004, "Ticket Dispenser" )
	PORT_DIPSETTING(      0x0004, "100000" )
	PORT_DIPSETTING(      0x0000, "150000" )
	PORT_DIPNAME( 0x0008, 0x0008, "Continue Coin" )
	PORT_DIPSETTING(      0x0008, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0010, 0x0010, "Extend" )
	PORT_DIPSETTING(      0x0010, "100000" )
	PORT_DIPSETTING(      0x0000, "None" )
	PORT_DIPNAME( 0x0060, 0x0060, DEF_STR( Difficulty ) ) 
	PORT_DIPSETTING(      0x0000, "Very_Hard" )
	PORT_DIPSETTING(      0x0020, "Hard" )
	PORT_DIPSETTING(      0x0040, "Easy" )
	PORT_DIPSETTING(      0x0060, "Normal" )
	PORT_DIPNAME( 0x0080, 0x0000, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(      0x0080, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	
	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_START3 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER3)
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER3)
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER3)
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER3)
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER3)
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER3)
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER3)

	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_START4 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER4)
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER4)
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER4)
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER4)
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER4)
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER4)
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER4)
INPUT_PORTS_END

INPUT_PORTS_START( cyclwarr )
	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN4 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_SERVICE2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_SERVICE3 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_SERVICE4 )

	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER1)
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1)
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1)
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1)
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1)
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1)
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1)

	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2)
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2)
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2)
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2)
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2)
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2)
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2)

	PORT_START
	PORT_DIPNAME( 0x0001, 0x0001, DEF_STR( Unknown ) ) 
	PORT_DIPSETTING(      0x0001, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0002, 0x0000, "Player Select" )
	PORT_DIPSETTING(      0x0002, "Coin Slot" )
	PORT_DIPSETTING(      0x0000, "Select SW" )
	PORT_DIPNAME( 0x0004, 0x0004, DEF_STR( Service_Mode ) )
	PORT_DIPSETTING(      0x0004, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0008, 0x0008, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0008, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )

	PORT_START
	PORT_DIPNAME( 0x000f, 0x000f, DEF_STR( Coin_A ) ) 
	PORT_DIPSETTING(      0x0000, DEF_STR( 5C_1C ) )
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
	PORT_DIPSETTING(      0x0009, DEF_STR( 1C_7C ) )
	PORT_DIPNAME( 0x00f0, 0x00f0, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 6C_1C ) )
	PORT_DIPSETTING(      0x0010, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(      0x0050, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0080, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0040, DEF_STR( 3C_2C ) )
	PORT_DIPSETTING(      0x00f0, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0030, DEF_STR( 3C_4C ) )
	PORT_DIPSETTING(      0x0070, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(      0x00e0, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x0060, DEF_STR( 2C_5C ) )
	PORT_DIPSETTING(      0x00d0, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x00c0, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(      0x00b0, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(      0x00a0, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(      0x0090, DEF_STR( 1C_7C ) )

	PORT_START
	PORT_DIPNAME( 0x0003, 0x0002, DEF_STR( Lives ) )
	PORT_DIPSETTING(      0x0000, "5" )
	PORT_DIPSETTING(      0x0001, "3" )
	PORT_DIPSETTING(      0x0002, "2" )
	PORT_DIPSETTING(      0x0003, "1" )
	PORT_DIPNAME( 0x0004, 0x0004, "Ticket Dispenser" ) 
	PORT_DIPSETTING(      0x0004, "10000" )
	PORT_DIPSETTING(      0x0000, "15000" )
	PORT_DIPNAME( 0x0018, 0x0008, "Machine Type" )
/*	PORT_DIPSETTING(      0x0000, "2 Players" ) */
	PORT_DIPSETTING(      0x0008, "2 Players" )
	PORT_DIPSETTING(      0x0010, "3 Players" )
	PORT_DIPSETTING(      0x0018, "4 Players" )
	PORT_DIPNAME( 0x0060, 0x0060, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(      0x0000, "Very_Hard" )
	PORT_DIPSETTING(      0x0020, "Hard" )
	PORT_DIPSETTING(      0x0040, "Easy" )
	PORT_DIPSETTING(      0x0060, "Normal" )
	PORT_DIPNAME( 0x0080, 0x0000, DEF_STR( Demo_Sounds ) ) 
	PORT_DIPSETTING(      0x0080, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )

	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_START3 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER3)
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER3)
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER3)
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER3)
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER3)
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER3)
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER3)

	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_START4 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER4)
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER4)
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER4)
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER4)
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER4)
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER4)
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER4)

INPUT_PORTS_END

INPUT_PORTS_START( roundup5 )
	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1)
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1)
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER1)
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER1)
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_SERVICE, DEF_STR( Service_Mode ), KEYCODE_F2, IP_JOY_NONE )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2) /*PORT_NAME(DEF_STR(Free_Play)) PORT_TOGGLE */
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2) /*PORT_NAME("Extra 2") PORT_TOGGLE */
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2) /*PORT_NAME("Extra 3") PORT_TOGGLE */
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER2) /*PORT_NAME("Extra 4") PORT_TOGGLE */

	PORT_START
   /* PORT_ANALOG( 0xff, 0x7f, IPT_AD_STICK_X | IPF_PLAYER1, 1, 0x */
    PORT_ANALOG( 0xff, 0x7f, IPT_AD_STICK_X | IPF_PLAYER1, 25, 15, 0, 0xff)

	PORT_START
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x01, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x02, DEF_STR( On ) )
	PORT_DIPNAME( 0x03, 0x00, "Game_Time" )
	PORT_DIPSETTING(    0x03, "Shortest" )
	PORT_DIPSETTING(    0x02, "Short" )
	PORT_DIPSETTING(    0x00, "Medium" )
	PORT_DIPSETTING(    0x01, "Long" )
	PORT_DIPNAME( 0x0c, 0x00, "Difficulty" )
	PORT_DIPSETTING(    0x04, "Easy" )
	PORT_DIPSETTING(    0x00, "Medium" )
	PORT_DIPSETTING(    0x08, "Hard" )
	PORT_DIPSETTING(    0x0c, "Hardest" )
	PORT_DIPNAME( 0x04, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x04, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x00, "Language" )
	PORT_DIPSETTING(    0x20, "Japanese" )
	PORT_DIPSETTING(    0x00, "English" )
	PORT_DIPNAME( 0x40, 0x00, "Stage 5 Continue" )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x00, "Output Mode" )
	PORT_DIPSETTING(    0x00, "A" )
	PORT_DIPSETTING(    0x80, "B" )

	PORT_START
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x01, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x04, DEF_STR( On ) )
	PORT_DIPNAME( 0x07, 0x00, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 6C_1C ) )
	PORT_DIPSETTING(    0x05, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x07, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0x38, 0x00, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x18, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0x28, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_7C ) )
	PORT_DIPSETTING(    0x38, DEF_STR( Free_Play ) )
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
	PORT_DIPNAME( 0x80, 0x80, "Test" )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
INPUT_PORTS_END

/******************************************************************************/

static struct GfxLayout roundup5_charlayout =
{
	8,8,	/* 16*16 sprites */
	RGN_FRAC(1,1),	/* 4096 sprites */
	4,	/* 4 bits per pixel */
	{ 0, 1, 2, 3 },
	{ 8,12,0,4, 24,28, 16,20},
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32},
	32*8	/* every sprite takes 32 consecutive bytes */
};

static struct GfxLayout roundup5_vramlayout =
{
	8,8,
	4096 + 2048,
	3,
	{ 0x30000 * 8, 0x18000 * 8, 0 },
	{ 0, 1, 2, 3, 4, 5, 6, 7},
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16},
	8*16
};

static struct GfxLayout bigfight_charlayout =
{
	8,8,
	RGN_FRAC(1,3),
	3,
	{ RGN_FRAC(2,3), RGN_FRAC(1,3), RGN_FRAC(0,3)},
	{ 0, 1, 2, 3, 4, 5, 6, 7},
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8},
	8*8
};


static struct GfxDecodeInfo bigfight_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &roundup5_charlayout,    8192, 512},
	{ REGION_GFX5, 0, &bigfight_charlayout,    0, 512},
	{ -1 } /* end of array */
};

static struct GfxDecodeInfo roundup5_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &roundup5_charlayout,     1024, 256},
	{ 0, 0, &roundup5_vramlayout,					0, 16},
	{ -1 } /* end of array */
};

/******************************************************************************/

static void sound_irq(int state)
{
	cpu_set_irq_line(2, 0, state);
}

static struct YM2151interface ym2151_interface =
{
	1,
	16000000/4, /* ? */
	{ YM3012_VOL(45,MIXER_PAN_LEFT,45,MIXER_PAN_RIGHT) },
	{ sound_irq }
};

static struct OKIM6295interface okim6295_interface =
{
	1,
	{ 16000000/4/2/132 }, /* Frequency */ /* ? */
	{ REGION_SOUND1 },
	{ 75 }
};

static struct OKIM6295interface cyclwarr_okim6295_interface =
{
	1,
	{ 16000000/8/132 }, /* Frequency */ /* ? */
	{ REGION_SOUND1 },
	{ 75 }
};

static struct OKIM6295interface bigfight_okim6295_interface =
{
	1,
	{ 16000000/8/2/132 }, /* Frequency */ /* ? */
	{ REGION_SOUND1 },
	{ 75 }
};

static MACHINE_DRIVER_START( bigfight )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 20000000 / 2) /* Confirmed */
	MDRV_CPU_MEMORY(bigfight_68000a_readmem,bigfight_68000a_writemem)
	MDRV_CPU_VBLANK_INT(irq5_line_hold,1)

	MDRV_CPU_ADD(M68000, 20000000 / 2) /* Confirmed */
	MDRV_CPU_MEMORY(bigfight_68000b_readmem,bigfight_68000b_writemem)
	MDRV_CPU_VBLANK_INT(irq5_line_hold,1)

	MDRV_CPU_ADD(Z80, 16000000 / 4) /* Confirmed */
	MDRV_CPU_MEMORY(readmem_bigfight_c,writemem_bigfight_c)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(100)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_NEEDS_6BITS_PER_GUN | VIDEO_RGB_DIRECT)
	MDRV_SCREEN_SIZE(40*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 40*8-1, 0*8, 30*8-1)
	MDRV_GFXDECODE(bigfight_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(8192 + 8192)

	MDRV_VIDEO_START(bigfight)
	MDRV_VIDEO_UPDATE(bigfight)

	/* sound hardware */
	MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
	MDRV_SOUND_ADD(YM2151, ym2151_interface)
	MDRV_SOUND_ADD(OKIM6295, bigfight_okim6295_interface)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( cyclwarr )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 20000000 / 2) /* Confirmed */
	MDRV_CPU_MEMORY(cyclwarr_68000a_readmem,cyclwarr_68000a_writemem)
	MDRV_CPU_VBLANK_INT(irq5_line_hold,1)

	MDRV_CPU_ADD(M68000, 20000000 / 2) /* Confirmed */
	MDRV_CPU_MEMORY(cyclwarr_68000b_readmem,cyclwarr_68000b_writemem)
	MDRV_CPU_VBLANK_INT(irq5_line_hold,1)

	MDRV_CPU_ADD(Z80, 16000000 / 4) /* Confirmed */
	MDRV_CPU_MEMORY(readmem_bigfight_c,writemem_bigfight_c)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(100)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_NEEDS_6BITS_PER_GUN | VIDEO_RGB_DIRECT)
	MDRV_SCREEN_SIZE(40*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 40*8-1, 0*8, 30*8-1)
	MDRV_GFXDECODE(bigfight_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(8192 + 8192)

	MDRV_VIDEO_START(cyclwarr)
	MDRV_VIDEO_UPDATE(cyclwarr)

	/* sound hardware */
	MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
	MDRV_SOUND_ADD(YM2151, ym2151_interface)
	MDRV_SOUND_ADD(OKIM6295, cyclwarr_okim6295_interface)
MACHINE_DRIVER_END


static INTERRUPT_GEN( roundup5_interrupt )
{
	cpu_set_irq_line_and_vector(0, 0, HOLD_LINE, 0xc8/4);	/* VBL */
}

static MACHINE_DRIVER_START( roundup5 )

	/* basic machine hardware */
	MDRV_CPU_ADD(V30,20000000 / 2) /* NEC V30 CPU, 20MHz / 2 */
	MDRV_CPU_MEMORY(readmem_roundup5,writemem_roundup5)
	MDRV_CPU_VBLANK_INT(roundup5_interrupt,1)

	MDRV_CPU_ADD(M68000,20000000 / 2) /* 68000 CPU, 20MHz / 2 */
	MDRV_CPU_MEMORY(readmem_roundup5_sub,writemem_roundup5_sub)

	MDRV_CPU_ADD(Z80, 4000000) /*??? */
	MDRV_CPU_MEMORY(readmem_roundup5_sound,writemem_roundup5_sound)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(100)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_NEEDS_6BITS_PER_GUN | VIDEO_RGB_DIRECT)
	MDRV_SCREEN_SIZE(40*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 40*8-1, 0*8, 30*8-1)
	MDRV_GFXDECODE(roundup5_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(1024 + 4096) /* 1024 real colours, and 4096 arranged as series of cluts */

	MDRV_VIDEO_START(roundup5)
	MDRV_VIDEO_UPDATE(roundup5)

	/* sound hardware */
	MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
	MDRV_SOUND_ADD(YM2151, ym2151_interface)
	MDRV_SOUND_ADD(OKIM6295, okim6295_interface)
MACHINE_DRIVER_END

/***************************************************************************/

ROM_START( bigfight )
/* using the same rom loading style as Cycle Warriors otherwise wont boot up */
	ROM_REGION( 0x180000, REGION_CPU1, 0 ) /* 68000 main cpu */
	ROM_LOAD16_BYTE( "rom16.ic77",   0x100000, 0x40000, CRC(e7304ec8) SHA1(31a37e96bf963b349d36534bc5ebbf45e19ad00e) )
	ROM_LOAD16_BYTE( "rom17.ic98",   0x100001, 0x40000, CRC(4cf090f6) SHA1(9ae0274c890e829a90108ce316aff9665128c982) )
/* using the same rom loading style as Cycle Warriors otherwise wont boot up */
	ROM_REGION( 0x180000, REGION_CPU2, 0 ) /* 68000 sub cpu */
	ROM_LOAD16_BYTE( "rom18.ic100",   0x100000, 0x40000, CRC(49df6207) SHA1(c4126f4542add11a3a3d236311c8787c24c98440) )
	ROM_LOAD16_BYTE( "rom19.ic102",   0x100001, 0x40000, CRC(c12aa9e9) SHA1(19cc7feaa97c6f5148ae8c0077174f96be684f05) )

	ROM_REGION( 0x10000, REGION_CPU3, 0 ) /* 64k code for sound Z80 */
	ROM_LOAD( "rom20.ic91",   0x000000, 0x10000, CRC(b3add091) SHA1(8a67bfff75c13fe4d9b89d30449199200d11cea7) )

	ROM_REGION( 0x400000, REGION_GFX1, ROMREGION_DISPOSE )
	/* Filled in by both regions below */

	ROM_REGION( 0x200000, REGION_GFX2, 0 )
	ROM_LOAD32_BYTE( "rom0.ic26",   0x000000, 0x80000, CRC(a4a3c8d6) SHA1(b5365d9bc6068260c23ba9d5971c7c7d7cc07a97) )
	ROM_LOAD32_BYTE( "rom8.ic45",   0x000001, 0x80000, CRC(220956ed) SHA1(68e0ba1e850101b4cc2778819dfa76f04d88d2d6) )
	ROM_LOAD32_BYTE( "rom2.ic28",   0x000002, 0x80000, CRC(c4f6d243) SHA1(e23b241b5a40b332165a34e2f1bc4366973b2070) )
	ROM_LOAD32_BYTE( "rom10.ic47",  0x000003, 0x80000, CRC(0212d472) SHA1(5549461195fd7b6b43c0174462d7fe1a1bac24e9) )

	ROM_REGION( 0x200000, REGION_GFX3, 0 )
	ROM_LOAD32_BYTE( "rom4.ic30",   0x000000, 0x80000, CRC(999ff7e9) SHA1(a53b06ad084722d7a52fcf01c52967f68620e609) )
	ROM_LOAD32_BYTE( "rom12.ic49",  0x000001, 0x80000, CRC(cb4c1f0b) SHA1(32d64b78ed3d5971eb5d25be2c38e6f2c9048f74) )
	ROM_LOAD32_BYTE( "rom6.ic32",   0x000002, 0x80000, CRC(f70e2d47) SHA1(00517b5f3b2deb6f3f3bd12df421e63884c22b2e) )
	ROM_LOAD32_BYTE( "rom14.ic51",  0x000003, 0x80000, CRC(77430bc9) SHA1(0b1fd54ace84a9fb5b44d5600de8089a20bcbd47) )

	ROM_REGION( 0x20000, REGION_GFX4, 0 )
	ROM_LOAD( "rom21.ic128",   0x000000, 0x20000, CRC(da027dcf) SHA1(47d18a8a273fea72cb3ad3d58166fe38ca28a860) )

	ROM_REGION( 0x60000, REGION_GFX5, ROMREGION_DISPOSE )
	ROM_LOAD( "rom24.ic73",   0x000000, 0x20000, CRC(c564185d) SHA1(e9b5fc10a5a5014735852c22db2a054d5787d8cb) )
	ROM_LOAD( "rom23.ic72",   0x020000, 0x20000, CRC(f8bb340b) SHA1(905a1ec778d6ed5c6f53d9d08cd105eed7e307ca) )
	ROM_LOAD( "rom22.ic71",   0x040000, 0x20000, CRC(fb505074) SHA1(b6d9b20be7c3e971e5a4392736f087e807b9c850) )

	ROM_REGION( 0x40000, REGION_SOUND1, 0 )	 /* ADPCM samples */
	ROM_LOAD( "rom15.ic39",   0x000000, 0x40000, CRC(58d136e8) SHA1(4aa063c4b9b057cba4655ecbe44a87c8c411e3aa) )
ROM_END

ROM_START( cyclwarr )
	ROM_REGION( 0x180000, REGION_CPU1, 0 ) /* 68000 main cpu */
	ROM_LOAD16_BYTE( "cw16b",   0x100000, 0x20000, CRC(cb1a737a) SHA1(a603ee1256be5641d00a72f64efaaacb65ed9d7d) )
	ROM_LOAD16_BYTE( "cw18b",   0x100001, 0x20000, CRC(0633ddcb) SHA1(1196ab17065352ec5b37f2f6b383a43a2d0fa3a6) )
	ROM_LOAD16_BYTE( "cw17a",   0x140000, 0x20000, CRC(2ad6f836) SHA1(5fa4275b433013943ba1d1b64a3c725097f946f9) )
	ROM_LOAD16_BYTE( "cw19a",   0x140001, 0x20000, CRC(d3853658) SHA1(c9338083a04f55bd22285176831f4b0bdb78564f) )

	ROM_REGION( 0x180000, REGION_CPU2, 0 ) /* 68000 sub cpu */
	ROM_LOAD16_BYTE( "cw20a",   0x100000, 0x20000, CRC(c3578ac1) SHA1(21d369da874f01922d0f0b757a42b4321df891d4) )
	ROM_LOAD16_BYTE( "cw22a",   0x100001, 0x20000, CRC(5339ed24) SHA1(5b0a54c2442dcf7373ff8b55b91af9772473ff77) )
	ROM_LOAD16_BYTE( "cw21",    0x140000, 0x20000, CRC(ed90d956) SHA1(f533f93da31ac6eb631fb506357717e7cac8e186) )
	ROM_LOAD16_BYTE( "cw23",    0x140001, 0x20000, CRC(009cdc78) SHA1(a77933a7736546397e8c69226703d6f9be7b55e5) )

	ROM_REGION( 0x10000, REGION_CPU3, 0 ) /* 64k code for sound Z80 */
	ROM_LOAD( "cw26a",   0x000000, 0x10000, CRC(f7a70e3a) SHA1(5581633bf1f15d7f5c1e03de897d65d60f9f1e33) )

	ROM_REGION( 0x200000, REGION_GFX1, ROMREGION_DISPOSE )
	/* Filled in by both regions below */

	ROM_REGION( 0x100000, REGION_GFX2, 0 )
	ROM_LOAD32_BYTE( "cw00a",   0x000000, 0x20000, CRC(058a77f1) SHA1(93f99fcf6ce6714d76af6f6e930115516f0379d3) )
	ROM_LOAD32_BYTE( "cw08a",   0x000001, 0x20000, CRC(f53993e7) SHA1(ef2d502ab180d2bc0bdb698c2878fdee9a2c33a8) )
	ROM_LOAD32_BYTE( "cw02a",   0x000002, 0x20000, CRC(4dadf3cb) SHA1(e42c56e295a443cb605d48eba23a16fab3c86525) )
	ROM_LOAD32_BYTE( "cw10a",   0x000003, 0x20000, CRC(3b7cd251) SHA1(52b9637404fa193421294dfb52c1a7bba0d94c9b) )
	ROM_LOAD32_BYTE( "cw01a",   0x080000, 0x20000, CRC(7c639948) SHA1(d58ff5735cd3179ffafead385a625baa7962e1d0) )
	ROM_LOAD32_BYTE( "cw09a",   0x080001, 0x20000, CRC(4ba24af5) SHA1(9203c2639e04aaa09996339f11259750ff8129b9) )
	ROM_LOAD32_BYTE( "cw03a",   0x080002, 0x20000, CRC(3ca6f98e) SHA1(8526fe38d3b4c66e09049ba18651a9e7255d85d6) )
	ROM_LOAD32_BYTE( "cw11a",   0x080003, 0x20000, CRC(5d760392) SHA1(7bbda2880af4659c267193ce10ed887a1b54a981) )

	ROM_REGION( 0x100000, REGION_GFX3, 0 )
	ROM_LOAD32_BYTE( "cw04a",   0x000000, 0x20000, CRC(f05f594d) SHA1(80effaa517b2154c013419e0bc05fd0797b74c8d) )
	ROM_LOAD32_BYTE( "cw12a",   0x000001, 0x20000, CRC(4ac07e8b) SHA1(f9de96fba39d5752d61b8f6be87fb605694624ed) )
	ROM_LOAD32_BYTE( "cw06a",   0x000002, 0x20000, CRC(f628edc9) SHA1(473f7ec28000e6bf72782c1c3f4afb5e021bd430) )
	ROM_LOAD32_BYTE( "cw14a",   0x000003, 0x20000, CRC(a9131f5f) SHA1(3a2059946984733e6939f3298f0db676e6a3301b)	)
	ROM_LOAD32_BYTE( "cw05a",   0x080000, 0x20000, CRC(c8f5faa9) SHA1(f374531ffd645597eeb1440fd2cadb426fcd3d79) )
	ROM_LOAD32_BYTE( "cw13a",   0x080001, 0x20000, CRC(8091d381) SHA1(7faf068ce20b2877559f0335df55d61be13146b4) )
	ROM_LOAD32_BYTE( "cw07a",   0x080002, 0x20000, CRC(314579b5) SHA1(3c10ec490f7821a5b5412295232bbb104d0e4b83) )
	ROM_LOAD32_BYTE( "cw15a",   0x080003, 0x20000, CRC(7ed4b721) SHA1(b87865effeff77a9ea74354ef2b5911a5102a647) )

	ROM_REGION( 0x20000, REGION_GFX4, 0 )
	ROM_LOAD( "cw27",   0x000000, 0x20000, CRC(2db48a9e) SHA1(16c307340d17cd3b5455ebcee681fbe0335dec58) )

	ROM_REGION( 0x60000, REGION_GFX5, ROMREGION_DISPOSE )
    ROM_LOAD( "cw30",   0x000000, 0x20000, CRC(331d0711) SHA1(82251fe1f1d36f079080943ab1fd04a60077c353) )
	ROM_LOAD( "cw29",   0x020000, 0x20000, CRC(64dd519c) SHA1(e23611fc2be896861997063546c3eb03527eaf8e) )
	ROM_LOAD( "cw28",   0x040000, 0x20000, CRC(3fc568ed) SHA1(91125c9deddc659449ca6791a847fe908c2818b2) )

	ROM_REGION( 0x40000, REGION_SOUND1, 0 )	 /* ADPCM samples */
	ROM_LOAD( "cw24a",   0x000000, 0x20000, CRC(22600cba) SHA1(a1514fbe037942f1493a17eb0b7986949470cb22) )
	ROM_LOAD( "cw25a",   0x020000, 0x20000, CRC(372c6bc8) SHA1(d4875bf3bffecf338bebba3b8d6a791585556a06) )
ROM_END

ROM_START( roundup5 )
	ROM_REGION( 0x100000, REGION_CPU1, 0 ) /* v30 main cpu */
	ROM_LOAD16_BYTE( "ru-23s",   0x080000, 0x20000, CRC(2dc8c521) SHA1(b78de101db3ef00fc4375ae32a7871e0da2dac6c) )
	ROM_LOAD16_BYTE( "ru-26s",   0x080001, 0x20000, CRC(1e16b531) SHA1(d7badef29cf1c4a9bd262933ecd1ca3343ea94bd) )
	ROM_LOAD16_BYTE( "ru-22t",   0x0c0000, 0x20000, CRC(9611382e) SHA1(c99258782dbad6d69ba7f54115ee3aa218f9b6ee) )
	ROM_LOAD16_BYTE( "ru-25t",   0x0c0001, 0x20000, CRC(b6cd0f2d) SHA1(61925c2346d79baaf9bce3d19a7dfc45b8232f92) )

	ROM_REGION( 0x80000, REGION_CPU2, 0 ) /* 68000 sub cpu */
	ROM_LOAD16_BYTE( "ru-20s",   0x000000, 0x20000, CRC(c5524558) SHA1(a94e7e4548148c83a332524ab4e06607732e13d5) )
	ROM_LOAD16_BYTE( "ru-18s",   0x000001, 0x20000, CRC(163ef03d) SHA1(099ac2d74164bdc6402b08efb521f49275780858) )
	ROM_LOAD16_BYTE( "ru-24s",   0x040000, 0x20000, CRC(b9f91b70) SHA1(43c5d9dafb60ed3e5c3eb0e612c2dbc5497f8a6c) )
	ROM_LOAD16_BYTE( "ru-19s",   0x040001, 0x20000, CRC(e3953800) SHA1(28fbc6bf154b512fcefeb04fe12db598b1b20cfe) )

	ROM_REGION( 0x10000, REGION_CPU3, 0 ) /* 64k code for sound Z80 */
	ROM_LOAD( "ru-28d",   0x000000, 0x10000, CRC(df36c6c5) SHA1(c046482043f6b54c55696ba3d339ffb11d78f674) )

	ROM_REGION( 0x180000, REGION_GFX1, ROMREGION_DISPOSE )
	/* Filled in by both regions below */

	ROM_REGION( 0x0c0000, REGION_GFX2, 0 )
	ROM_LOAD32_BYTE( "ru-00b",   0x000000, 0x20000, CRC(388a0647) SHA1(e4ab43832872f44c0fe1aaede4372cc00ca7d32b) )
	ROM_LOAD32_BYTE( "ru-02b",   0x000001, 0x20000, CRC(eff33945) SHA1(3f4c3aaa11ccf945c2f898dfdf815705d8539e21) )
	ROM_LOAD32_BYTE( "ru-04b",   0x000002, 0x20000, CRC(40fda247) SHA1(f5fbc07fda024baedf35ac209210e94df9f15065) )
	ROM_LOAD32_BYTE( "ru-06b",   0x000003, 0x20000, CRC(cd2484f3) SHA1(a23a4d36a8b913104bcc75228317b2979afec888) )
	ROM_LOAD32_BYTE( "ru-01b",   0x080000, 0x10000, CRC(5e91f401) SHA1(df976c5ba0f14b14f5642b5ca35b996bca64e369) )
	ROM_LOAD32_BYTE( "ru-03b",   0x080001, 0x10000, CRC(2fb109de) SHA1(098c103e6bae0f52ec66f0cdda2da60bd7108736) )
	ROM_LOAD32_BYTE( "ru-05b",   0x080002, 0x10000, CRC(23dd10e1) SHA1(f30ff1a8c7ed9bc567b901cbdd202028fffb9f80) )
	ROM_LOAD32_BYTE( "ru-07b",   0x080003, 0x10000, CRC(bb40f46e) SHA1(da694e16d19f60a0dee47551f00f3e50b2d5dcaf) )

	ROM_REGION( 0x0c0000, REGION_GFX3, 0 )
	ROM_LOAD32_BYTE( "ru-08b",   0x000000, 0x20000, CRC(01729e3c) SHA1(1445287fde0b993d053aab73efafc902a6b7e2cc) )
	ROM_LOAD32_BYTE( "ru-10b",   0x000001, 0x20000, CRC(cd2357a7) SHA1(313460a74244325ce2c659816f2b738f3dc5358a) )
	ROM_LOAD32_BYTE( "ru-12b",   0x000002, 0x20000, CRC(ca63b1f8) SHA1(a50ef8259745dc166eb0a1b2c812ff620818a755) )
	ROM_LOAD32_BYTE( "ru-14b",   0x000003, 0x20000, CRC(dde79bfc) SHA1(2d5888189a6f954801f248a3365e328370fed837) )
	ROM_LOAD32_BYTE( "ru-09b",   0x080000, 0x10000, CRC(629ac0a6) SHA1(c3eeccd6c07be7455cf180c9c7d5efcd6d08c0b5) )
	ROM_LOAD32_BYTE( "ru-11b",   0x080001, 0x10000, CRC(fe3fbf53) SHA1(7400c088025ac22e5d9db816792533fc02f2dcf5) )
	ROM_LOAD32_BYTE( "ru-13b",   0x080002, 0x10000, CRC(d0f6e747) SHA1(ef15ed41124b2d37bc6e92254138690dd644e50f) )
	ROM_LOAD32_BYTE( "ru-15b",   0x080003, 0x10000, CRC(6ee6b22e) SHA1(a28edaf23ca6c7231264de962d5ea37bad39f996) )

	ROM_REGION( 0x40000, REGION_SOUND1, 0 )	 /* ADPCM samples */
	ROM_LOAD( "ru-17b",   0x000000, 0x20000, CRC(82391b47) SHA1(6b1977522c6e906503abc50bdd24c4c38cdc9bdb) )
	ROM_LOAD( "ru-16e",   0x020000, 0x10000, CRC(374fe170) SHA1(5d190a2735698b0384948bfdb1a900f56f0d7ebc) )
ROM_END


/***************************************************************************/

static DRIVER_INIT( cyclwarr )
{
	UINT8 *dst = memory_region(REGION_GFX1);
	UINT8 *src1 = memory_region(REGION_GFX2);
	UINT8 *src2 = memory_region(REGION_GFX3);
	int i;
	for (i=0; i<0x100000; i+=32) {
		memcpy(dst,src1,32);
		src1+=32;
		dst+=32;
		memcpy(dst,src2,32);
		dst+=32;
		src2+=32;
	}

	dst = memory_region(REGION_CPU1);
	memcpy(cyclwarr_cpua_ram,dst+0x100000,8);
	cpu_setbank(1, memory_region(REGION_CPU1) + 0x100000);

	dst = memory_region(REGION_CPU2);
	memcpy(cyclwarr_cpub_ram,dst+0x100000,8);
	cpu_setbank(2, memory_region(REGION_CPU2) + 0x100000);

	/* Copy sprite & palette data out of GFX rom area */
    tatsumi_rom_sprite_lookup1 = memory_region(REGION_GFX2);
	tatsumi_rom_sprite_lookup2 = memory_region(REGION_GFX3);
	tatsumi_rom_clut0 = memory_region(REGION_GFX2)+0x100000 - 0x1000;
	tatsumi_rom_clut1 = memory_region(REGION_GFX3)+0x100000 - 0x1000;

	tatsumi_reset();
}


/* hack using same hookup as Cycle Warriors GFX 123 are double the size */
static DRIVER_INIT( bigfight )
{
	UINT8 *dst = memory_region(REGION_GFX1);
	UINT8 *src1 = memory_region(REGION_GFX2);
	UINT8 *src2 = memory_region(REGION_GFX3);
	int i;
	for (i=0; i<0x200000; i+=32) {
		memcpy(dst,src1,32);
		src1+=32;
		dst+=32;
		memcpy(dst,src2,32);
		dst+=32;
		src2+=32;
	}

	dst = memory_region(REGION_CPU1);
	memcpy(cyclwarr_cpua_ram,dst+0x100000,8);
	cpu_setbank(1, memory_region(REGION_CPU1) + 0x100000);

	dst = memory_region(REGION_CPU2);
	memcpy(cyclwarr_cpub_ram,dst+0x100000,8);
	cpu_setbank(2, memory_region(REGION_CPU2) + 0x100000);

	/* Copy sprite & palette data out of GFX rom area */
    tatsumi_rom_sprite_lookup1 = memory_region(REGION_GFX2);
	tatsumi_rom_sprite_lookup2 = memory_region(REGION_GFX3);
	tatsumi_rom_clut0 = memory_region(REGION_GFX2)+0x200000 - 0x1000;
	tatsumi_rom_clut1 = memory_region(REGION_GFX3)+0x200000 - 0x1000;

	tatsumi_reset();
}

static DRIVER_INIT( roundup5 )
{
	UINT8 *dst = memory_region(REGION_GFX1);
	UINT8 *src1 = memory_region(REGION_GFX2);
	UINT8 *src2 = memory_region(REGION_GFX3);
	int i;

	for (i=0; i<0xc0000; i+=32) {
		memcpy(dst,src1,32);
		src1+=32;
		dst+=32;
		memcpy(dst,src2,32);
		dst+=32;
		src2+=32;
	}

	/* Copy sprite & palette data out of GFX rom area */
	tatsumi_rom_sprite_lookup1 = memory_region(REGION_GFX2);
	tatsumi_rom_sprite_lookup2 = memory_region(REGION_GFX3);
	tatsumi_rom_clut0 = memory_region(REGION_GFX2)+0xc0000-0x800;
	tatsumi_rom_clut1 = memory_region(REGION_GFX3)+0xc0000-0x800;

	tatsumi_reset();
}

/***************************************************************************/
/* 1986 Lock On */
/* 1987 Gray Out */
/* 1988 Apache 3 */ /* Not really working bad GFX so didn't support it here */
GAMEX( 1989, roundup5, 0, roundup5, roundup5, roundup5, ROT0, "Tatsumi", "Round Up 5 - Super Delta Force", GAME_IMPERFECT_GRAPHICS )
GAMEX( 1992, bigfight, 0, bigfight, bigfight, bigfight, ROT0, "Tatsumi", "Big Fight - Big Trouble In The Atlantic Ocean", GAME_IMPERFECT_GRAPHICS )
GAMEX( 1991, cyclwarr, 0, cyclwarr, cyclwarr, cyclwarr, ROT0, "Tatsumi", "Cycle Warriors", GAME_IMPERFECT_GRAPHICS )
