/***************************************************************************

    Data East 'Rohga' era hardware:

    Rogha Armour Attack         (c) 1991 Data East Corporation
    Wizard Fire                 (c) 1992 Data East Corporation
    Nitro Ball/Gun Ball         (c) 1992 Data East Corporation
    Hang Zo                     (c) 1992 Hot B

    This hardware is capable of alpha-blending on sprites and playfields


***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "cpu/h6280/h6280.h"
#include "decocrpt.h"
#include "decoprot.h"
#include "deco16ic.h"

VIDEO_START( rohga );
VIDEO_START( wizdfire );
VIDEO_START( nitrobal );
VIDEO_UPDATE( rohga );
VIDEO_UPDATE( hangzo );
VIDEO_UPDATE( wizdfire );
VIDEO_UPDATE( nitrobal );
WRITE16_HANDLER( rohga_buffer_spriteram16_w );

static READ16_HANDLER( rohga_dip3_r ) { return readinputport(3); }
static READ16_HANDLER( nitrobal_control_r ) { return readinputport(3); }
static READ16_HANDLER( hangzo_control_r ) { return readinputport(1); }

static READ16_HANDLER( rohga_irq_ack_r )
{
	cpu_set_irq_line(0, 6, CLEAR_LINE);
	return 0;
}


static WRITE16_HANDLER( wizdfire_irq_ack_w )
{
	/* This might actually do more, nitrobal for example sets 0xca->0xffff->0x80 at startup then writes 7 all the time
	   except when a credit is inserted (writes 6 twice).
	   Wizard Fire / Dark Seal 2 just writes 1 all the time, so I just don't trust it much for now... -AS */
	cpu_set_irq_line(0, 6, CLEAR_LINE);
}


/**********************************************************************************/

static MEMORY_READ16_START( rohga_readmem )
    { 0x000000, 0x1fffff, MRA16_ROM },
	{ 0x280000, 0x2807ff, deco16_104_rohga_prot_r }, /* Protection device */
	{ 0x280800, 0x280fff, MRA16_RAM }, /* Mirror */
	{ 0x2c0000, 0x2c0001, rohga_dip3_r },
	{ 0x321100, 0x321101, rohga_irq_ack_r }, /* Irq ack?  Value not used */
	{ 0x3c0000, 0x3c1fff, MRA16_RAM },
	{ 0x3c2000, 0x3c2fff, MRA16_RAM },
	{ 0x3c4000, 0x3c4fff, MRA16_RAM },
	{ 0x3c6000, 0x3c6fff, MRA16_RAM },
	{ 0x3c8000, 0x3c8fff, MRA16_RAM },
	{ 0x3c9000, 0x3c9fff, MRA16_RAM }, /* Mirror */
	{ 0x3ca000, 0x3cafff, MRA16_RAM },
	{ 0x3cb000, 0x3cbfff, MRA16_RAM }, /* Mirror */
	{ 0x3cc000, 0x3ccfff, MRA16_RAM },
	{ 0x3cd000, 0x3cdfff, MRA16_RAM }, /* Mirror */
	{ 0x3ce000, 0x3cefff, MRA16_RAM },
	{ 0x3cf000, 0x3cffff, MRA16_RAM }, /* Mirror */
	{ 0x3d0000, 0x3d0fff, MRA16_RAM },
	{ 0x3e0000, 0x3e1fff, MRA16_RAM },
	{ 0x3f0000, 0x3f3fff, MRA16_RAM },
MEMORY_END


static MEMORY_WRITE16_START( rohga_writemem )
    { 0x000000, 0x1fffff, MWA16_ROM },
	{ 0x200000, 0x20000f, MWA16_RAM, &deco16_pf12_control },
	{ 0x240000, 0x24000f, MWA16_RAM, &deco16_pf34_control },
	{ 0x280000, 0x2807ff, deco16_104_rohga_prot_w, &deco16_prot_ram }, /* Protection writes */
	{ 0x280800, 0x280fff, deco16_104_rohga_prot_w }, /* Mirror */
	{ 0x300000, 0x300001, rohga_buffer_spriteram16_w }, /* write 1 for sprite dma */
	{ 0x310000, 0x310009, MWA16_NOP }, /* Palette control? */
	{ 0x31000a, 0x31000b, deco16_palette_dma_w }, /* Write 1111 for dma?  (Or any value?) */
	{ 0x320000, 0x320001, MWA16_NOP }, /* ? */
	{ 0x322000, 0x322001, deco16_priority_w },
	{ 0x3c0000, 0x3c1fff, deco16_pf1_data_w, &deco16_pf1_data },
	{ 0x3c2000, 0x3c2fff, deco16_pf2_data_w, &deco16_pf2_data },
	{ 0x3c4000, 0x3c4fff, deco16_pf3_data_w, &deco16_pf3_data },
	{ 0x3c6000, 0x3c6fff, deco16_pf4_data_w, &deco16_pf4_data },
	{ 0x3c8000, 0x3c8fff, MWA16_RAM, &deco16_pf1_rowscroll },
	{ 0x3c9000, 0x3c9fff, MWA16_RAM }, /* Mirror */
	{ 0x3ca000, 0x3cafff, MWA16_RAM, &deco16_pf2_rowscroll },
	{ 0x3cb000, 0x3cbfff, MWA16_RAM }, /* Mirror */
	{ 0x3cc000, 0x3ccfff, MWA16_RAM, &deco16_pf3_rowscroll },
	{ 0x3cd000, 0x3cdfff, MWA16_RAM }, /* Mirror */
	{ 0x3ce000, 0x3cefff, MWA16_RAM, &deco16_pf4_rowscroll },
	{ 0x3cf000, 0x3cffff, MWA16_RAM }, /* Mirror */
	{ 0x3d0000, 0x3d07ff, MWA16_RAM, &spriteram16, &spriteram_size },
	{ 0x3e0000, 0x3e1fff, deco16_buffered_palette_w, &paletteram16 },
	{ 0x3f0000, 0x3f3fff, MWA16_RAM }, /* Main ram */
MEMORY_END


static MEMORY_READ16_START( wizdfire_readmem )
    { 0x000000, 0x1fffff, MRA16_ROM },
	{ 0x200000, 0x200fff, MRA16_RAM },
	{ 0x202000, 0x202fff, MRA16_RAM },
	{ 0x208000, 0x208fff, MRA16_RAM },
	{ 0x20a000, 0x20afff, MRA16_RAM },
	{ 0x20c000, 0x20cfff, MRA16_RAM },
	{ 0x20e000, 0x20efff, MRA16_RAM },
	{ 0x340000, 0x3407ff, MRA16_RAM },
	{ 0x360000, 0x3607ff, MRA16_RAM },
	{ 0x380000, 0x381fff, MRA16_RAM },
	{ 0xfdc000, 0xfe3fff, MRA16_RAM },
	{ 0xfe4000, 0xfe47ff, deco16_104_prot_r }, /* Protection device */
	{ 0xfe5000, 0xfeffff, MRA16_RAM },
MEMORY_END

static MEMORY_WRITE16_START( wizdfire_writemem )
    { 0x000000, 0x1fffff, MWA16_ROM },
	{ 0x200000, 0x200fff, deco16_pf1_data_w, &deco16_pf1_data },
	{ 0x202000, 0x202fff, deco16_pf2_data_w, &deco16_pf2_data },
	{ 0x208000, 0x208fff, deco16_pf3_data_w, &deco16_pf3_data },
	{ 0x20a000, 0x20afff, deco16_pf4_data_w, &deco16_pf4_data },
	{ 0x20b000, 0x20b3ff, MWA16_RAM }, /* ? Always 0 written */
	{ 0x20c000, 0x20c7ff, MWA16_RAM, &deco16_pf3_rowscroll },
	{ 0x20e000, 0x20e7ff, MWA16_RAM, &deco16_pf4_rowscroll },
	{ 0x300000, 0x30000f, MWA16_RAM, &deco16_pf12_control },
	{ 0x310000, 0x31000f, MWA16_RAM, &deco16_pf34_control },
	{ 0x320000, 0x320001, deco16_priority_w }, /* Priority */
	{ 0x320002, 0x320003, MWA16_NOP }, /* ? */
	{ 0x320004, 0x320005, wizdfire_irq_ack_w }, /* VBL IRQ ack */
	{ 0x340000, 0x3407ff, MWA16_RAM, &spriteram16, &spriteram_size },
	{ 0x350000, 0x350001, buffer_spriteram16_w }, /* Triggers DMA for spriteram */
	{ 0x360000, 0x3607ff, MWA16_RAM, &spriteram16_2, &spriteram_2_size },
	{ 0x370000, 0x370001, buffer_spriteram16_2_w }, /* Triggers DMA for spriteram */
	{ 0x380000, 0x381fff, deco16_buffered_palette_w, &paletteram16 },
	{ 0x390008, 0x390009, deco16_palette_dma_w },
	{ 0xfe4000, 0xfe47ff, deco16_104_prot_w, &deco16_prot_ram }, /* Protection writes */
	{ 0xfdc000, 0xfeffff, MWA16_RAM }, /* Main ram */
MEMORY_END

static MEMORY_READ16_START( nitrobal_readmem )
    { 0x000000, 0x1fffff, MRA16_ROM },
	{ 0x200000, 0x200fff, MRA16_RAM },
	{ 0x202000, 0x202fff, MRA16_RAM },
	{ 0x204000, 0x2047ff, MRA16_RAM },
	{ 0x206000, 0x2067ff, MRA16_RAM },
	{ 0x208000, 0x208fff, MRA16_RAM },
	{ 0x20a000, 0x20afff, MRA16_RAM },
	{ 0x20c000, 0x20c7ff, MRA16_RAM },
	{ 0x20e000, 0x20e7ff, MRA16_RAM },
	{ 0x300000, 0x30000f, MRA16_RAM },
	{ 0x310000, 0x31000f, MRA16_RAM },
	{ 0x320000, 0x320001, nitrobal_control_r },
	{ 0x340000, 0x3407ff, MRA16_RAM },
	{ 0x360000, 0x3607ff, MRA16_RAM },
	{ 0x380000, 0x381fff, MRA16_RAM },
	{ 0xff4000, 0xff47ff, deco16_146_nitroball_prot_r }, /* Protection device */
	{ 0xfec000, 0xffffff, MRA16_RAM },
MEMORY_END


static MEMORY_WRITE16_START( nitrobal_writemem )
    { 0x000000, 0x1fffff, MWA16_ROM },
	{ 0x200000, 0x200fff, deco16_pf1_data_w, &deco16_pf1_data },
	{ 0x202000, 0x202fff, deco16_pf2_data_w, &deco16_pf2_data },
	{ 0x208000, 0x208fff, deco16_pf3_data_w, &deco16_pf3_data },
	{ 0x20a000, 0x20afff, deco16_pf4_data_w, &deco16_pf4_data },
	{ 0x204000, 0x2047ff, MWA16_RAM, &deco16_pf1_rowscroll },
	{ 0x206000, 0x2067ff, MWA16_RAM, &deco16_pf2_rowscroll },
	{ 0x20c000, 0x20c7ff, MWA16_RAM, &deco16_pf3_rowscroll },
	{ 0x20e000, 0x20e7ff, MWA16_RAM, &deco16_pf4_rowscroll },
	{ 0x300000, 0x30000f, MWA16_RAM, &deco16_pf12_control },
	{ 0x310000, 0x31000f, MWA16_RAM, &deco16_pf34_control },
	{ 0x320000, 0x320001, deco16_priority_w }, /* Priority */
	{ 0x320002, 0x320003, MWA16_NOP }, /* ? */
	{ 0x320004, 0x320005, wizdfire_irq_ack_w }, /* VBL IRQ ack */
	{ 0x340000, 0x3407ff, MWA16_RAM, &spriteram16, &spriteram_size },
	{ 0x350000, 0x350001, buffer_spriteram16_w },/* Triggers DMA for spriteram */
	{ 0x360000, 0x3607ff, MWA16_RAM, &spriteram16_2, &spriteram_2_size },
	{ 0x370000, 0x370001, buffer_spriteram16_2_w },/* Triggers DMA for spriteram */
	{ 0x380000, 0x381fff, deco16_buffered_palette_w, &paletteram16 },
	{ 0x390008, 0x390009, deco16_palette_dma_w },
	{ 0xff4000, 0xff47ff, deco16_146_nitroball_prot_w, &deco16_prot_ram }, /* Protection writes */
	{ 0xfec000, 0xffffff, MWA16_RAM }, /* Main ram */
MEMORY_END


static MEMORY_READ16_START( hangzo_readmem )
    { 0x000000, 0x0fffff, MRA16_ROM },
	{ 0x280000, 0x2807ff, deco16_104_rohga_prot_r }, /* Protection device */
	{ 0x280800, 0x280fff, MRA16_RAM }, /* Mirror */
	{ 0x2c0000, 0x2c0001, rohga_dip3_r },
	{ 0x300000, 0x300001, nitrobal_control_r },
	{ 0x310002, 0x310003, hangzo_control_r },
	{ 0x321100, 0x321101, MRA16_NOP }, /* Irq ack?  Value not used */
	{ 0x3c0000, 0x3c1fff, MRA16_RAM },
	{ 0x3c2000, 0x3c2fff, MRA16_RAM },
	{ 0x3c4000, 0x3c4fff, MRA16_RAM },
	{ 0x3c6000, 0x3c6fff, MRA16_RAM },
	{ 0x3c8000, 0x3c9fff, MRA16_RAM },
	{ 0x3ca000, 0x3cafff, MRA16_RAM },
	{ 0x3cb000, 0x3cbfff, MRA16_RAM }, /* Mirror */
	{ 0x3cc000, 0x3ccfff, MRA16_RAM },
	{ 0x3cd000, 0x3cdfff, MRA16_RAM }, /* Mirror */
	{ 0x3ce000, 0x3cefff, MRA16_RAM },
	{ 0x3cf000, 0x3cffff, MRA16_RAM }, /* Mirror */
	{ 0x3d0000, 0x3d07ff, MRA16_RAM },
	{ 0x3e0000, 0x3e1fff, MRA16_RAM },
	{ 0x3e2000, 0x3e3fff, MRA16_RAM }, /* Mirror */
    { 0x3f0000, 0x3f3fff, MRA16_RAM },
MEMORY_END

static MEMORY_WRITE16_START( hangzo_writemem )
    { 0x000000, 0x1fffff, MWA16_ROM },
	{ 0x200000, 0x20000f, MWA16_RAM, &deco16_pf12_control },
	{ 0x240000, 0x24000f, MWA16_RAM, &deco16_pf34_control },
	{ 0x280000, 0x2807ff, deco16_104_rohga_prot_w, &deco16_prot_ram }, /* Protection writes */
	{ 0x280800, 0x280fff, deco16_104_rohga_prot_w }, /* Mirror */
	{ 0x300000, 0x300001, rohga_buffer_spriteram16_w }, /* write 1 for sprite dma */
	{ 0x310000, 0x310009, MWA16_NOP }, /* Palette control? */
	{ 0x31000a, 0x31000b, deco16_palette_dma_w }, /* Write 1111 for dma?  (Or any value?) */
	{ 0x320000, 0x320001, MWA16_NOP }, /* ? */
	{ 0x322000, 0x322001, deco16_priority_w },
	{ 0x3c0000, 0x3c1fff, deco16_pf1_data_w, &deco16_pf1_data },
	{ 0x3c2000, 0x3c2fff, deco16_pf2_data_w, &deco16_pf2_data },
	{ 0x3c4000, 0x3c4fff, deco16_pf3_data_w, &deco16_pf3_data },
	{ 0x3c6000, 0x3c6fff, deco16_pf4_data_w, &deco16_pf4_data },
	{ 0x3c8000, 0x3c9fff, MWA16_RAM, &deco16_pf1_rowscroll },
	{ 0x3ca000, 0x3cafff, MWA16_RAM, &deco16_pf2_rowscroll },
	{ 0x3cb000, 0x3cbfff, MWA16_RAM }, /* Mirror */
	{ 0x3cc000, 0x3ccfff, MWA16_RAM, &deco16_pf3_rowscroll },
	{ 0x3cd000, 0x3cdfff, MWA16_RAM }, /* Mirror */
	{ 0x3ce000, 0x3cefff, MWA16_RAM, &deco16_pf4_rowscroll },
	{ 0x3cf000, 0x3cffff, MWA16_RAM }, /* Mirror */
	{ 0x3d0000, 0x3d07ff, MWA16_RAM, &spriteram16, &spriteram_size },
	{ 0x3e0000, 0x3e1fff, deco16_buffered_palette_w, &paletteram16 },
	{ 0x3e2000, 0x3e3fff, MWA16_RAM }, /* Mirror */
	{ 0x3f0000, 0x3f3fff, MWA16_RAM }, /* Main ram */
MEMORY_END


/******************************************************************************/

static MEMORY_READ_START( sound_readmem )
	{ 0x000000, 0x00ffff, MRA_ROM },
	{ 0x100000, 0x100001, MRA_NOP },
	{ 0x110000, 0x110001, YM2151_status_port_0_r },
	{ 0x120000, 0x120001, OKIM6295_status_0_r },
	{ 0x130000, 0x130001, OKIM6295_status_1_r },
	{ 0x140000, 0x140001, soundlatch_r },
	{ 0x1f0000, 0x1f1fff, MRA_BANK8 },
MEMORY_END

static MEMORY_WRITE_START( sound_writemem )
	{ 0x000000, 0x00ffff, MWA_ROM },
	{ 0x100000, 0x100001, MWA_NOP },
	{ 0x110000, 0x110001, YM2151_word_0_w },
	{ 0x120000, 0x120001, OKIM6295_data_0_w },
    { 0x130000, 0x130001, OKIM6295_data_1_w },
    { 0x1f0000, 0x1f1fff, MWA_BANK8 },
    { 0x1fec00, 0x1fec01, H6280_timer_w },
    { 0x1ff402, 0x1ff403, H6280_irq_status_w },
MEMORY_END

/**********************************************************************************/

INPUT_PORTS_START( rohga )
	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_BUTTON3 )
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
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_VBLANK )

	PORT_START	/* Dip switch bank 1/2 */
	PORT_DIPNAME( 0x0007, 0x0007, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0001, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0007, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0006, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x0005, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x0004, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(      0x0003, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( 1C_6C ) )
	PORT_DIPNAME( 0x0038, 0x0038, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0008, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0038, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0030, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x0028, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(      0x0018, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(      0x0010, DEF_STR( 1C_6C ) )
	PORT_DIPNAME( 0x0040, 0x0040, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(      0x0040, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0080, 0x0080, "2 Credits to Start, 1 to Continue" )
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
	PORT_DIPNAME( 0x3000, 0x3000, "Player's Vitality" )
	PORT_DIPSETTING(      0x3000, "Normal" )
	PORT_DIPSETTING(      0x2000, "Low" )
	PORT_DIPSETTING(      0x1000, "Lowest" )
	PORT_DIPSETTING(      0x0000, "High" )
	PORT_DIPNAME( 0x4000, 0x4000, "Allow_Continue" )
	PORT_DIPSETTING(      0x4000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x8000, 0x0000, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(      0x8000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )

	PORT_START	/* Dip switch bank 3 */
	PORT_DIPNAME( 0x0001, 0x0001, "Stage Clear Bonus" ) /* Life Recovery At stage clear */
	PORT_DIPSETTING(      0x0001, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0002, 0x0002, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x000c, 0x000c, "Enemy's Vitality" )
	PORT_DIPSETTING(      0x0008, "Low" )
	PORT_DIPSETTING(      0x000c, "Normal" )
	PORT_DIPSETTING(      0x0004, "High" )
	PORT_DIPSETTING(      0x0000, "Highest" )
	PORT_DIPNAME( 0x0030, 0x0030, "Enemy Encounter Rate" )
	PORT_DIPSETTING(      0x0020, "Low" )
	PORT_DIPSETTING(      0x0030, "Normal" )
	PORT_DIPSETTING(      0x0010, "High" )
	PORT_DIPSETTING(      0x0000, "Highest" )
	PORT_DIPNAME( 0x00c0, 0x00c0, "Enemy's Weapon Speed" )
	PORT_DIPSETTING(      0x0080, "Slow" )
	PORT_DIPSETTING(      0x00c0, "Normal" )
	PORT_DIPSETTING(      0x0040, "Fast" )
	PORT_DIPSETTING(      0x0000, "Fastest" )
INPUT_PORTS_END


INPUT_PORTS_START( wizdfire )
	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_START2 )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_VBLANK )

	PORT_START	/* Dip switch bank 1/2 */
	PORT_DIPNAME( 0x0007, 0x0007, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0001, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0007, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0006, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x0005, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x0004, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(      0x0003, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( 1C_6C ) )
	PORT_DIPNAME( 0x0038, 0x0038, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0008, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0038, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0030, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x0028, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(      0x0018, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(      0x0010, DEF_STR( 1C_6C ) )
	PORT_DIPNAME( 0x0040, 0x0040, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(      0x0040, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0080, 0x0080, "2 Credits to Start, 1 to Continue" )
	PORT_DIPSETTING(      0x0080, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0300, 0x0300, DEF_STR( Lives ) )
	PORT_DIPSETTING(      0x0000, "2" )
	PORT_DIPSETTING(      0x0100, "3" )
	PORT_DIPSETTING(      0x0300, "4" )
	PORT_DIPSETTING(      0x0200, "5" )
	PORT_DIPNAME( 0x0c00, 0x0c00, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(      0x0800, "Easy" )
	PORT_DIPSETTING(      0x0c00, "Normal" )
	PORT_DIPSETTING(      0x0400, "Hard" )
	PORT_DIPSETTING(      0x0000, "Hardest" )
	PORT_DIPNAME( 0x3000, 0x3000, "Magic Gauge Speed" )
	PORT_DIPSETTING(      0x0000, "Very Slow" )
	PORT_DIPSETTING(      0x1000, "Slow" )
	PORT_DIPSETTING(      0x3000, "Normal" )
	PORT_DIPSETTING(      0x2000, "Fast" )
	PORT_DIPNAME( 0x4000, 0x4000, DEF_STR( Unused ) )
	PORT_DIPSETTING(      0x4000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x8000, 0x0000, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(      0x8000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
INPUT_PORTS_END

INPUT_PORTS_START( nitrobal )
	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_START2 )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_VBLANK )

	PORT_START	/* Dip switch bank 1/2 */
	PORT_DIPNAME( 0x0007, 0x0007, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0001, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0007, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0006, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x0005, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x0004, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(      0x0003, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( 1C_6C ) )
	PORT_DIPNAME( 0x0038, 0x0038, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0008, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0038, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0030, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x0028, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(      0x0018, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(      0x0010, DEF_STR( 1C_6C ) )
	PORT_DIPNAME( 0x0040, 0x0040, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(      0x0040, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0080, 0x0080, "2 Credits to Start, 1 to Continue" )
	PORT_DIPSETTING(      0x0080, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0300, 0x0300, DEF_STR( Lives ) )
	PORT_DIPSETTING(      0x0100, "1" )
	PORT_DIPSETTING(      0x0000, "2" )
	PORT_DIPSETTING(      0x0300, "3" )
	PORT_DIPSETTING(      0x0200, "4" )
	PORT_DIPNAME( 0x0c00, 0x0c00, "Difficulty?"  )
	PORT_DIPSETTING(      0x0800, "Easy" )
	PORT_DIPSETTING(      0x0c00, "Normal" )
	PORT_DIPSETTING(      0x0400, "Hard" )
	PORT_DIPSETTING(      0x0000, "Hardest" )
	PORT_DIPNAME( 0x1000, 0x1000, "Split Coin Chutes" )
	PORT_DIPSETTING(      0x1000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x2000, 0x2000, "Players" )
	PORT_DIPSETTING(      0x2000, "2" )
	PORT_DIPSETTING(      0x0000, "3" )
	PORT_DIPNAME( 0x4000, 0x4000, "Shot Button to Start" )
	PORT_DIPSETTING(      0x4000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x8000, 0x0000, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(      0x8000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )

	PORT_START
	PORT_BIT( 0x1, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_PLAYER3 )
	PORT_BIT( 0x2, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_PLAYER3 )
	PORT_BIT( 0x4, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_PLAYER3 )
	PORT_BIT( 0x8, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER3 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER3 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER3 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN3 )
INPUT_PORTS_END


INPUT_PORTS_START( hangzo )
	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
    PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_BUTTON3 )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_START2 )

	PORT_START
    PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_VBLANK )

	PORT_START
    PORT_DIPNAME( 0x0007, 0x0007, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0001, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0007, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0006, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x0005, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x0004, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(      0x0003, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( 1C_6C ) )
	PORT_DIPNAME( 0x0038, 0x0038, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0008, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0038, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0030, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x0028, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(      0x0018, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(      0x0010, DEF_STR( 1C_6C ) )
	PORT_DIPNAME( 0x0040, 0x0040, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(      0x0040, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0080, 0x0080, "2 Credits to Start, 1 to Continue" )
	PORT_DIPSETTING(      0x0080, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0300, 0x0300, DEF_STR( Lives ) )
	PORT_DIPSETTING(      0x0100, "1" )
	PORT_DIPSETTING(      0x0000, "2" )
	PORT_DIPSETTING(      0x0300, "3" )
	PORT_DIPSETTING(      0x0200, "4" )
	PORT_DIPNAME( 0x0400, 0x0400, DEF_STR( Unknown ) ) /* Either 3 & 4 are Difficulty */
	PORT_DIPSETTING(      0x0400, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0800, 0x0800, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0800, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x1000, 0x1000, DEF_STR( Unknown ) ) /* or more likely 5 & 6 are Player's Vitality like Rohga (all other dips seem to match Rohga) */
	PORT_DIPSETTING(      0x1000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x2000, 0x2000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x2000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x4000, 0x4000, "Allow_Continue" )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x4000, DEF_STR( On ) )
	PORT_DIPNAME( 0x8000, 0x0000, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(      0x8000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )

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
INPUT_PORTS_END


/**********************************************************************************/

static struct GfxLayout charlayout =
{
	8,8,
	RGN_FRAC(1,2),
	4,
	{ RGN_FRAC(1,2)+8, RGN_FRAC(1,2), 8, 0 },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16 },
	16*8	/* every char takes 8 consecutive bytes */
};

static struct GfxLayout spritelayout =
{
	16,16,
	RGN_FRAC(1,1),
	4,
	{ 16, 0, 24, 8 },
	{ 64*8+0, 64*8+1, 64*8+2, 64*8+3, 64*8+4, 64*8+5, 64*8+6, 64*8+7,
		0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32,
			8*32, 9*32, 10*32, 11*32, 12*32, 13*32, 14*32, 15*32 },
	128*8
};

static struct GfxLayout spritelayout_6bpp =
{
	16,16,
	4096*8,
	6,
	{ 0x400000*8+8, 0x400000*8, 0x200000*8+8, 0x200000*8, 8, 0 },
	{ 7,6,5,4,3,2,1,0,
	32*8+7, 32*8+6, 32*8+5, 32*8+4, 32*8+3, 32*8+2, 32*8+1, 32*8+0,  },
	{ 15*16, 14*16, 13*16, 12*16, 11*16, 10*16, 9*16, 8*16,
			7*16, 6*16, 5*16, 4*16, 3*16, 2*16, 1*16, 0*16 },
	64*8
};

static struct GfxLayout spritelayout2 =
{
	16,16,
	4096*8,
	4,
	{ 0x200000*8+8, 0x200000*8, 8, 0 },
	{ 7,6,5,4,3,2,1,0,
	32*8+7, 32*8+6, 32*8+5, 32*8+4, 32*8+3, 32*8+2, 32*8+1, 32*8+0,  },
	{ 15*16, 14*16, 13*16, 12*16, 11*16, 10*16, 9*16, 8*16,
			7*16, 6*16, 5*16, 4*16, 3*16, 2*16, 1*16, 0*16 },
	64*8
};

static struct GfxLayout tilelayout =
{
	16,16,
	RGN_FRAC(1,2),
	4,
	{ RGN_FRAC(1,2)+8, RGN_FRAC(1,2), 8, 0 },
	{ 32*8+0, 32*8+1, 32*8+2, 32*8+3, 32*8+4, 32*8+5, 32*8+6, 32*8+7,
		0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16,
			8*16, 9*16, 10*16, 11*16, 12*16, 13*16, 14*16, 15*16 },
	64*8
};

static struct GfxLayout tilelayout_8bpp =
{
	16,16,
	RGN_FRAC(1,4),
	8,
	{ RGN_FRAC(3,4)+8, RGN_FRAC(3,4), RGN_FRAC(1,4)+8, RGN_FRAC(1,4), RGN_FRAC(2,4)+8, RGN_FRAC(2,4), 8, 0 },
	{ 32*8+0, 32*8+1, 32*8+2, 32*8+3, 32*8+4, 32*8+5, 32*8+6, 32*8+7,
		0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16,
			8*16, 9*16, 10*16, 11*16, 12*16, 13*16, 14*16, 15*16 },
	64*8
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &charlayout,          0, 32 },	/* Characters 8x8 */
	{ REGION_GFX2, 0, &tilelayout,          0, 32 },	/* Tiles 16x16 */
	{ REGION_GFX3, 0, &tilelayout,        512, 32 },	/* Tiles 16x16 */
	{ REGION_GFX4, 0, &spritelayout_6bpp,1024, 16 },	/* Sprites 16x16 */
	{ -1 } /* end of array */
};

static struct GfxDecodeInfo gfxdecodeinfo_wizdfire[] =
{
	{ REGION_GFX1, 0, &charlayout,        0, 32 },	/* Gfx chip 1 as 8x8 */
	{ REGION_GFX2, 0, &tilelayout,        0, 32 },	/* Gfx chip 1 as 16x16 */
	{ REGION_GFX3, 0, &tilelayout,      512, 32 },  /* Gfx chip 2 as 16x16 */
	{ REGION_GFX4, 0, &spritelayout,   1024, 32 }, /* Sprites 16x16 */
	{ REGION_GFX5, 0, &spritelayout,   1536, 32 },
	{ -1 } /* end of array */
};

static struct GfxDecodeInfo gfxdecodeinfo_nitrobal[] =
{
	{ REGION_GFX1, 0, &charlayout,        0, 32 },	/* Gfx chip 1 as 8x8 */
	{ REGION_GFX2, 0, &tilelayout,        0, 32 },	/* Gfx chip 1 as 16x16 */
	{ REGION_GFX3, 0, &tilelayout_8bpp, 512,  2 },  /* Gfx chip 2 as 16x16 */
	{ REGION_GFX4, 0, &spritelayout,   1024, 32 },  /* Sprites 16x16 */
	{ REGION_GFX5, 0, &spritelayout,   1536, 32 },
	{ -1 } /* end of array */
};

static struct GfxDecodeInfo gfxdecodeinfo_hangzo[] =
{
	{ REGION_GFX1, 0, &charlayout,          0, 32 },	/* Characters 8x8 */
	{ REGION_GFX2, 0, &tilelayout,          0, 32 },	/* Tiles 16x16 */
	{ REGION_GFX3, 0, &tilelayout,        512, 32 },	/* Tiles 16x16 */
	{ REGION_GFX4, 0, &spritelayout2,    1024, 64 },	/* Sprites 16x16 */
	{ -1 } /* end of array */
};

/**********************************************************************************/

static void sound_irq(int state)
{
	cpu_set_irq_line(1,1,state); /* IRQ 2 */
}

static WRITE_HANDLER( sound_bankswitch_w )
{
	OKIM6295_set_bank_base(0, ((data & 1)>>0) * 0x40000);
	OKIM6295_set_bank_base(1, ((data & 2)>>1) * 0x40000);
}

static struct YM2151interface ym2151_interface =
{
	1,
	32220000/9, /* Accurate, audio section crystal is 32.220 MHz */
	{ YM3012_VOL(40,MIXER_PAN_LEFT,40,MIXER_PAN_RIGHT) },
	{ sound_irq }, 
	{ sound_bankswitch_w }
};

static struct OKIM6295interface okim6295_interface =
{
	2,              /* 2 chips */
	{ 32220000/32/132, 32220000/16/132 },/* Frequency */
	{ REGION_SOUND1, REGION_SOUND2 },
	{ 95, 40 } /* Note!  Keep chip 1 (voices) louder than chip 2 */
};

/**********************************************************************************/

static MACHINE_DRIVER_START( rohga )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 14000000)
	MDRV_CPU_MEMORY(rohga_readmem,rohga_writemem)
	MDRV_CPU_VBLANK_INT(irq6_line_assert,1)

	MDRV_CPU_ADD(H6280,32220000/4)
	MDRV_CPU_MEMORY(sound_readmem,sound_writemem)

	MDRV_FRAMES_PER_SECOND(58)
	MDRV_VBLANK_DURATION(529)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_NEEDS_6BITS_PER_GUN | VIDEO_BUFFERS_SPRITERAM)
	MDRV_SCREEN_SIZE(40*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 40*8-1, 1*8, 31*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(2048)

	MDRV_VIDEO_START(rohga)
	MDRV_VIDEO_UPDATE(rohga)

	/* sound hardware */
	MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
	MDRV_SOUND_ADD(YM2151, ym2151_interface)
	MDRV_SOUND_ADD(OKIM6295, okim6295_interface)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( wizdfire )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 14000000)
	MDRV_CPU_MEMORY(wizdfire_readmem,wizdfire_writemem)
	MDRV_CPU_VBLANK_INT(irq6_line_assert,1)

	MDRV_CPU_ADD(H6280,32220000/4)
	MDRV_CPU_MEMORY(sound_readmem,sound_writemem)

	MDRV_FRAMES_PER_SECOND(58)
	MDRV_VBLANK_DURATION(529)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_NEEDS_6BITS_PER_GUN | VIDEO_BUFFERS_SPRITERAM | VIDEO_RGB_DIRECT)
	MDRV_SCREEN_SIZE(40*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 40*8-1, 1*8, 31*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo_wizdfire)
	MDRV_PALETTE_LENGTH(2048)

	MDRV_VIDEO_START(wizdfire)
	MDRV_VIDEO_UPDATE(wizdfire)

	/* sound hardware */
	MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
	MDRV_SOUND_ADD(YM2151, ym2151_interface)
	MDRV_SOUND_ADD(OKIM6295, okim6295_interface)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( nitrobal )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 14000000)
	MDRV_CPU_MEMORY(nitrobal_readmem,nitrobal_writemem)
	MDRV_CPU_VBLANK_INT(irq6_line_assert,1)

	MDRV_CPU_ADD(H6280,32220000/4)
	MDRV_CPU_MEMORY(sound_readmem,sound_writemem)

	MDRV_FRAMES_PER_SECOND(58)
	MDRV_VBLANK_DURATION(529)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_NEEDS_6BITS_PER_GUN | VIDEO_BUFFERS_SPRITERAM | VIDEO_RGB_DIRECT)
	MDRV_SCREEN_SIZE(40*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 40*8-1, 1*8, 31*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo_nitrobal)
	MDRV_PALETTE_LENGTH(2048)

	MDRV_VIDEO_START(nitrobal)
	MDRV_VIDEO_UPDATE(nitrobal)

	/* sound hardware */
	MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
	MDRV_SOUND_ADD(YM2151, ym2151_interface)
	MDRV_SOUND_ADD(OKIM6295, okim6295_interface)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( hangzo )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 14000000)
	MDRV_CPU_MEMORY(hangzo_readmem,hangzo_writemem)
	MDRV_CPU_VBLANK_INT(irq6_line_hold,1)

	MDRV_CPU_ADD(H6280,32220000/4)
	MDRV_CPU_MEMORY(sound_readmem,sound_writemem)

	MDRV_FRAMES_PER_SECOND(58)
	MDRV_VBLANK_DURATION(529)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_NEEDS_6BITS_PER_GUN | VIDEO_BUFFERS_SPRITERAM)
	MDRV_SCREEN_SIZE(40*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 40*8-1, 1*8, 31*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo_hangzo)
	MDRV_PALETTE_LENGTH(2048)

	MDRV_VIDEO_START(rohga)
	MDRV_VIDEO_UPDATE(hangzo)

	/* sound hardware */
	MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
	MDRV_SOUND_ADD(YM2151, ym2151_interface)
	MDRV_SOUND_ADD(OKIM6295, okim6295_interface)
MACHINE_DRIVER_END


/**********************************************************************************/

ROM_START( rohga )
	ROM_REGION(0x200000, REGION_CPU1, 0 ) /* 68000 code */
	ROM_LOAD16_BYTE( "jd00.bin",  0x000000, 0x40000, CRC(e046c77a) SHA1(bb4d987a579a1a1524bc150ebda9cd24ed77a733) )
	ROM_LOAD16_BYTE( "jd03.bin",  0x000001, 0x40000, CRC(2c5120b8) SHA1(41b6618f0f086efd48486f72ada2fb6f184ad85b) )
	ROM_LOAD16_BYTE( "mam00.8a",  0x100000, 0x80000, CRC(0fa440a6) SHA1(f0f84c630fc30ec164acc21de871c857d391c398) )
	ROM_LOAD16_BYTE( "mam07.8d",  0x100001, 0x80000, CRC(f8bc7f20) SHA1(909324248bd207f3b01d9f694975b629d8ccaa08) )

	ROM_REGION(0x10000, REGION_CPU2, 0 ) /* Sound CPU */
	ROM_LOAD( "ha04.18p",  0x00000,  0x10000,  CRC(eb6608eb) SHA1(0233677970aba12783dd4d6d58d70568ef641115) )

	ROM_REGION( 0x020000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD16_BYTE( "ha01.13a",  0x00000,  0x10000,  CRC(fb8f8519) SHA1(0a237426561e5fef6a062e1ad5ae02204f72d5f9) ) /* Encrypted tiles */
	ROM_LOAD16_BYTE( "ha02.14a",  0x00001,  0x10000,  CRC(aa47c17f) SHA1(830dfcbfaef90133d93b0fbf3cf2067498fa658b) )

	ROM_REGION( 0x100000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "mam01.10a", 0x000000, 0x080000,  CRC(dbf4fbcc) SHA1(2f289556fd25beb7d30501cba17ac35ad28c5b91) ) /* Encrypted tiles */
	ROM_LOAD( "mam02.11a", 0x080000, 0x080000,  CRC(b1fac481) SHA1(da370499ea8ff7b3dd338b31f3799b760fd0d981) )

	ROM_REGION( 0x200000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "mam08.17d",  0x000000, 0x100000,  CRC(ca97a83f) SHA1(2e097840ae56cf19ad2651d59c31182f47239d60) ) /* tiles 1 & 2 */
	ROM_LOAD( "mam09.18d",  0x100000, 0x100000,  CRC(3f57d56f) SHA1(0d4537da6ab62762179215deae72fe2e6a7869e1) )

	ROM_REGION( 0x600000, REGION_GFX4, ROMREGION_DISPOSE )
	ROM_LOAD( "mam05.19a", 0x000000, 0x100000,  CRC(307a2cd1) SHA1(d7a795e47cf1533f0bb5a96162c8025282abe09f) ) /* 6bpp sprites */
	ROM_LOAD( "mam06.20a", 0x100000, 0x100000,  CRC(a1119a2d) SHA1(876f9295c2032ce491b45a103ffafc750d8c78e1) )
	ROM_LOAD( "mam10.19d", 0x200000, 0x100000,  CRC(99f48f9f) SHA1(685787de54e9158ced80f3821996c3a63f2a72a2) )
	ROM_LOAD( "mam11.20d", 0x300000, 0x100000,  CRC(c3f12859) SHA1(45fdfd55f606316c936f0a9e6b4940740138d344) )
	ROM_LOAD( "mam03.17a", 0x400000, 0x100000,  CRC(fc4dfd48) SHA1(0c5f5a09833ebeb3018e65edd6f7ce06d4ba84ed) )
	ROM_LOAD( "mam04.18a", 0x500000, 0x100000,  CRC(7d3b38bf) SHA1(9f83ad7497ed57405ad648f403eb69f776567a50) )

	ROM_REGION(0x80000, REGION_SOUND2, 0 ) /* Oki samples */
	ROM_LOAD( "mam12.14p", 0x00000,  0x80000,  CRC(6f00b791) SHA1(c9fbc9ab5ce84fec79efa0a23373be97a27bf898) )

	ROM_REGION(0x80000, REGION_SOUND1, 0 ) /* Oki samples */
	ROM_LOAD( "mam13.15p", 0x00000,  0x80000,  CRC(525b9461) SHA1(1d9bb3725dfe601b05a779b84b4191455087b969) )

	ROM_REGION( 512, REGION_PROMS, 0 )
	ROM_LOAD( "hb-00.11p", 0x00000,  0x200,  CRC(b7a7baad) SHA1(39781c3412493b985d3616ac31142fc00bbcddf4) )	/* ? */
ROM_END

ROM_START( rohgah )
	ROM_REGION(0x200000, REGION_CPU1, 0 ) /* 68000 code */
	ROM_LOAD16_BYTE( "jd00-2.2a", 0x000000, 0x40000, CRC(ec70646a) SHA1(5e25fe8ce0dfebf8f5903ebe9aa5ef01ca7aa2f0) )
	ROM_LOAD16_BYTE( "jd03-2.2d", 0x000001, 0x40000, CRC(11d4c9a2) SHA1(9afe684d749665f65e44a3665d5a1dc61458faa0) )
	ROM_LOAD16_BYTE( "mam00.8a",  0x100000, 0x80000, CRC(0fa440a6) SHA1(f0f84c630fc30ec164acc21de871c857d391c398) )
	ROM_LOAD16_BYTE( "mam07.8d",  0x100001, 0x80000, CRC(f8bc7f20) SHA1(909324248bd207f3b01d9f694975b629d8ccaa08) )

	ROM_REGION(0x10000, REGION_CPU2, 0 ) /* Sound CPU */
	ROM_LOAD( "ha04.18p",  0x00000,  0x10000,  CRC(eb6608eb) SHA1(0233677970aba12783dd4d6d58d70568ef641115) )

	ROM_REGION( 0x020000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD16_BYTE( "ha01.13a",  0x00000,  0x10000,  CRC(fb8f8519) SHA1(0a237426561e5fef6a062e1ad5ae02204f72d5f9) ) /* Encrypted tiles */
	ROM_LOAD16_BYTE( "ha02.14a",  0x00001,  0x10000,  CRC(aa47c17f) SHA1(830dfcbfaef90133d93b0fbf3cf2067498fa658b) )

	ROM_REGION( 0x100000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "mam01.10a", 0x000000, 0x080000,  CRC(dbf4fbcc) SHA1(2f289556fd25beb7d30501cba17ac35ad28c5b91) ) /* Encrypted tiles */
	ROM_LOAD( "mam02.11a", 0x080000, 0x080000,  CRC(b1fac481) SHA1(da370499ea8ff7b3dd338b31f3799b760fd0d981) )

	ROM_REGION( 0x200000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "mam08.17d",  0x000000, 0x100000,  CRC(ca97a83f) SHA1(2e097840ae56cf19ad2651d59c31182f47239d60) ) /* tiles 1 & 2 */
	ROM_LOAD( "mam09.18d",  0x100000, 0x100000,  CRC(3f57d56f) SHA1(0d4537da6ab62762179215deae72fe2e6a7869e1) )

	ROM_REGION( 0x600000, REGION_GFX4, ROMREGION_DISPOSE )
	ROM_LOAD( "mam05.19a", 0x000000, 0x100000,  CRC(307a2cd1) SHA1(d7a795e47cf1533f0bb5a96162c8025282abe09f) ) /* 6bpp sprites */
	ROM_LOAD( "mam06.20a", 0x100000, 0x100000,  CRC(a1119a2d) SHA1(876f9295c2032ce491b45a103ffafc750d8c78e1) )
	ROM_LOAD( "mam10.19d", 0x200000, 0x100000,  CRC(99f48f9f) SHA1(685787de54e9158ced80f3821996c3a63f2a72a2) )
	ROM_LOAD( "mam11.20d", 0x300000, 0x100000,  CRC(c3f12859) SHA1(45fdfd55f606316c936f0a9e6b4940740138d344) )
	ROM_LOAD( "mam03.17a", 0x400000, 0x100000,  CRC(fc4dfd48) SHA1(0c5f5a09833ebeb3018e65edd6f7ce06d4ba84ed) )
	ROM_LOAD( "mam04.18a", 0x500000, 0x100000,  CRC(7d3b38bf) SHA1(9f83ad7497ed57405ad648f403eb69f776567a50) )

	ROM_REGION(0x80000, REGION_SOUND2, 0 ) /* Oki samples */
	ROM_LOAD( "mam12.14p", 0x00000,  0x80000,  CRC(6f00b791) SHA1(c9fbc9ab5ce84fec79efa0a23373be97a27bf898) )

	ROM_REGION(0x80000, REGION_SOUND1, 0 ) /* Oki samples */
	ROM_LOAD( "mam13.15p", 0x00000,  0x80000,  CRC(525b9461) SHA1(1d9bb3725dfe601b05a779b84b4191455087b969) )

	ROM_REGION( 512, REGION_PROMS, 0 )
	ROM_LOAD( "hb-00.11p", 0x00000,  0x200,  CRC(b7a7baad) SHA1(39781c3412493b985d3616ac31142fc00bbcddf4) )	/* ? */
ROM_END

ROM_START( rohgau )
	ROM_REGION(0x200000, REGION_CPU1, 0 ) /* 68000 code */
	ROM_LOAD16_BYTE( "ha00.2a",  0x000000, 0x40000, CRC(d8d13052) SHA1(24113244200f15a16fed82c64de3e9e4e87d1257) )
	ROM_LOAD16_BYTE( "ha03.2d",  0x000001, 0x40000, CRC(5f683bbf) SHA1(a367b833fd1f64bff9618ce06be22aed218d4225) )
	ROM_LOAD16_BYTE( "mam00.8a",  0x100000, 0x80000, CRC(0fa440a6) SHA1(f0f84c630fc30ec164acc21de871c857d391c398) )
	ROM_LOAD16_BYTE( "mam07.8d",  0x100001, 0x80000, CRC(f8bc7f20) SHA1(909324248bd207f3b01d9f694975b629d8ccaa08) )

	ROM_REGION(0x10000, REGION_CPU2, 0 ) /* Sound CPU */
	ROM_LOAD( "ha04.18p",  0x00000,  0x10000,  CRC(eb6608eb) SHA1(0233677970aba12783dd4d6d58d70568ef641115) )

	ROM_REGION( 0x020000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD16_BYTE( "ha01.13a",  0x00000,  0x10000,  CRC(fb8f8519) SHA1(0a237426561e5fef6a062e1ad5ae02204f72d5f9) ) /* Encrypted tiles */
	ROM_LOAD16_BYTE( "ha02.14a",  0x00001,  0x10000,  CRC(aa47c17f) SHA1(830dfcbfaef90133d93b0fbf3cf2067498fa658b) )

	ROM_REGION( 0x100000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "mam01.10a", 0x000000, 0x080000,  CRC(dbf4fbcc) SHA1(2f289556fd25beb7d30501cba17ac35ad28c5b91) ) /* Encrypted tiles */
	ROM_LOAD( "mam02.11a", 0x080000, 0x080000,  CRC(b1fac481) SHA1(da370499ea8ff7b3dd338b31f3799b760fd0d981) )

	ROM_REGION( 0x200000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "mam08.17d",  0x000000, 0x100000,  CRC(ca97a83f) SHA1(2e097840ae56cf19ad2651d59c31182f47239d60) ) /* tiles 1 & 2 */
	ROM_LOAD( "mam09.18d",  0x100000, 0x100000,  CRC(3f57d56f) SHA1(0d4537da6ab62762179215deae72fe2e6a7869e1) )

	ROM_REGION( 0x600000, REGION_GFX4, ROMREGION_DISPOSE )
	ROM_LOAD( "mam05.19a", 0x000000, 0x100000,  CRC(307a2cd1) SHA1(d7a795e47cf1533f0bb5a96162c8025282abe09f) ) /* 6bpp sprites */
	ROM_LOAD( "mam06.20a", 0x100000, 0x100000,  CRC(a1119a2d) SHA1(876f9295c2032ce491b45a103ffafc750d8c78e1) )
	ROM_LOAD( "mam10.19d", 0x200000, 0x100000,  CRC(99f48f9f) SHA1(685787de54e9158ced80f3821996c3a63f2a72a2) )
	ROM_LOAD( "mam11.20d", 0x300000, 0x100000,  CRC(c3f12859) SHA1(45fdfd55f606316c936f0a9e6b4940740138d344) )
	ROM_LOAD( "mam03.17a", 0x400000, 0x100000,  CRC(fc4dfd48) SHA1(0c5f5a09833ebeb3018e65edd6f7ce06d4ba84ed) )
	ROM_LOAD( "mam04.18a", 0x500000, 0x100000,  CRC(7d3b38bf) SHA1(9f83ad7497ed57405ad648f403eb69f776567a50) )

	ROM_REGION(0x80000, REGION_SOUND2, 0 ) /* Oki samples */
	ROM_LOAD( "mam12.14p", 0x00000,  0x80000,  CRC(6f00b791) SHA1(c9fbc9ab5ce84fec79efa0a23373be97a27bf898) )

	ROM_REGION(0x80000, REGION_SOUND1, 0 ) /* Oki samples */
	ROM_LOAD( "mam13.15p", 0x00000,  0x80000,  CRC(525b9461) SHA1(1d9bb3725dfe601b05a779b84b4191455087b969) )

	ROM_REGION( 512, REGION_PROMS, 0 )
	ROM_LOAD( "hb-00.11p", 0x00000,  0x200,  CRC(b7a7baad) SHA1(39781c3412493b985d3616ac31142fc00bbcddf4) )	/* ? */
ROM_END


ROM_START( wizdfire )
	ROM_REGION(0x200000, REGION_CPU1, 0 ) /* 68000 code */
	ROM_LOAD16_BYTE( "jf-01.3d",   0x000000, 0x20000, CRC(bde42a41) SHA1(0379de9c4cdcce35554b5dc15241ed2c4f0d7611) )
	ROM_LOAD16_BYTE( "jf-00.3a",   0x000001, 0x20000, CRC(bca3c995) SHA1(dbebc9e301c04ee82ca4b658d3ab870790d1605b) )
	ROM_LOAD16_BYTE( "jf-03.5d",   0x040000, 0x20000, CRC(5217d404) SHA1(7cfcdb9e2c812bf0d4ac8306834242876ac47844) )
	ROM_LOAD16_BYTE( "jf-02.5a",   0x040001, 0x20000, CRC(36a1ce28) SHA1(62d52d720c89022de97759777230c45c460d8fb6) )
	ROM_LOAD16_BYTE( "mas13",   0x080000, 0x80000, CRC(7e5256ce) SHA1(431d78ad185ba0216097f131fb2583a1a067e4f0) )
	ROM_LOAD16_BYTE( "mas12",   0x080001, 0x80000, CRC(005bd499) SHA1(862079022f97bd11f2f33677dce55bd3b144a81b) )

	ROM_REGION(0x10000, REGION_CPU2, 0 ) /* Sound CPU */
	ROM_LOAD( "jf-06.20r",  0x00000,  0x10000,  CRC(79042546) SHA1(231561df9415a289756a533709f610894fb9176e) )

	ROM_REGION( 0x020000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD16_BYTE( "jf-04.10d",  0x00000,  0x10000,  CRC(73cba800) SHA1(dd7612fe1482713fcee5960b7db158be872d7fda) ) /* Chars */
	ROM_LOAD16_BYTE( "jf-05.12d",  0x00001,  0x10000,  CRC(22e2c49d) SHA1(06cc2d0476156d1f521c4c57621ce3922a23aa04) )

	ROM_REGION( 0x200000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "mas00", 0x000000, 0x100000,  CRC(3d011034) SHA1(167d6d088d51a41f196be104d795ffe24297c96a) ) /* Tiles */
	ROM_LOAD( "mas01", 0x100000, 0x100000,  CRC(6d0c9d0b) SHA1(63e19dfd6451810637664b08e880aef139ca6ed5) )

	ROM_REGION( 0x100000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "mas02", 0x000000, 0x080000,  CRC(af00e620) SHA1(43f4680b22ac6baf840274462c07fee68a2fbdfb) )
	ROM_LOAD( "mas03", 0x080000, 0x080000,  CRC(2fe61ea2) SHA1(0909e6c689c3e10225d7c074bd654ff2ada96983) )

	ROM_REGION( 0x400000, REGION_GFX4, ROMREGION_DISPOSE )
	ROM_LOAD16_BYTE( "mas04", 0x000001, 0x100000,  CRC(1e56953b) SHA1(0655ac7f3c5030a80c2d6bad5c3a79b2cb1ae4a2) ) /* Sprites #1 */
	ROM_LOAD16_BYTE( "mas05", 0x000000, 0x100000,  CRC(3826b8f8) SHA1(d59197b4e0525b86876f9cce6fbf80caba976851) )
	ROM_LOAD16_BYTE( "mas06", 0x200001, 0x100000,  CRC(3b8bbd45) SHA1(c9f9d4daf9c0cba5385af26f3762b29c291ff62b) )
	ROM_LOAD16_BYTE( "mas07", 0x200000, 0x100000,  CRC(31303769) SHA1(509604be06ec8e0c1b56a81a8ffccdf0f79e9fd7) )

	ROM_REGION( 0x100000, REGION_GFX5, ROMREGION_DISPOSE )
	ROM_LOAD16_BYTE( "mas08", 0x000001, 0x080000,  CRC(e224fb7a) SHA1(9aa92fb98bddff313db2077c4db102e94c7af09b) ) /* Sprites #2 */
	ROM_LOAD16_BYTE( "mas09", 0x000000, 0x080000,  CRC(5f6deb41) SHA1(850d0e157b4355e866ec770a2012293b2c55648f) )

	ROM_REGION(0x80000, REGION_SOUND1, 0 ) /* Oki samples */
	ROM_LOAD( "mas10",  0x00000,  0x80000,  CRC(6edc06a7) SHA1(8ab92cca9d4a5d4fed3d99737c6f023f3f606db2) )

	ROM_REGION(0x80000, REGION_SOUND2, 0 ) /* Oki samples */
	ROM_LOAD( "mas11",  0x00000,  0x80000,  CRC(c2f0a4f2) SHA1(af71d649aea273c17d7fbcf8693e8a1d4b31f7f8) )

	ROM_REGION( 1024, REGION_PROMS, 0 )
	ROM_LOAD( "mb7122h.16l", 0x00000,  0x400,  CRC(2bee57cc) SHA1(bc48670aa7c39f6ff7fae4c819eab22ed2db875b) )	/* Priority (unused) */
ROM_END

ROM_START( darksel2 )
	ROM_REGION(0x200000, REGION_CPU1, 0 ) /* 68000 code */
	ROM_LOAD16_BYTE( "jb01-3",  0x000000, 0x20000, CRC(82308c01) SHA1(aa0733e244f14f2c84b6929236771cbc99532bb2) )
	ROM_LOAD16_BYTE( "jb00-3",  0x000001, 0x20000, CRC(1d38113a) SHA1(69dc5a4dbe9d9737df198240f3db6f2115e311a5) )
	ROM_LOAD16_BYTE( "jf-03.5d",0x040000, 0x20000, CRC(5217d404) SHA1(7cfcdb9e2c812bf0d4ac8306834242876ac47844) )
	ROM_LOAD16_BYTE( "jf-02.5a",0x040001, 0x20000, CRC(36a1ce28) SHA1(62d52d720c89022de97759777230c45c460d8fb6) )
	ROM_LOAD16_BYTE( "mas13",   0x080000, 0x80000, CRC(7e5256ce) SHA1(431d78ad185ba0216097f131fb2583a1a067e4f0) )
	ROM_LOAD16_BYTE( "mas12",   0x080001, 0x80000, CRC(005bd499) SHA1(862079022f97bd11f2f33677dce55bd3b144a81b) )

	ROM_REGION(0x10000, REGION_CPU2, 0 ) /* Sound CPU */
	ROM_LOAD( "jb06",  0x00000,  0x10000,  CRC(2066a1dd) SHA1(a0d136e90825fa9c089894a6852c634676d64579) )

	ROM_REGION( 0x020000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD16_BYTE( "jf-04.10d",  0x00000,  0x10000,  CRC(73cba800) SHA1(dd7612fe1482713fcee5960b7db158be872d7fda) ) /* Chars */
	ROM_LOAD16_BYTE( "jf-05.12d",  0x00001,  0x10000,  CRC(22e2c49d) SHA1(06cc2d0476156d1f521c4c57621ce3922a23aa04) )

	ROM_REGION( 0x200000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "mas00", 0x000000, 0x100000,  CRC(3d011034) SHA1(167d6d088d51a41f196be104d795ffe24297c96a) ) /* Tiles */
	ROM_LOAD( "mas01", 0x100000, 0x100000,  CRC(6d0c9d0b) SHA1(63e19dfd6451810637664b08e880aef139ca6ed5) )

	ROM_REGION( 0x100000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "mas02", 0x000000, 0x080000,  CRC(af00e620) SHA1(43f4680b22ac6baf840274462c07fee68a2fbdfb) )
	ROM_LOAD( "mas03", 0x080000, 0x080000,  CRC(2fe61ea2) SHA1(0909e6c689c3e10225d7c074bd654ff2ada96983) )

	ROM_REGION( 0x400000, REGION_GFX4, ROMREGION_DISPOSE )
	ROM_LOAD16_BYTE( "mas04", 0x000001, 0x100000,  CRC(1e56953b) SHA1(0655ac7f3c5030a80c2d6bad5c3a79b2cb1ae4a2) ) /* Sprites #1 */
	ROM_LOAD16_BYTE( "mas05", 0x000000, 0x100000,  CRC(3826b8f8) SHA1(d59197b4e0525b86876f9cce6fbf80caba976851) )
	ROM_LOAD16_BYTE( "mas06", 0x200001, 0x100000,  CRC(3b8bbd45) SHA1(c9f9d4daf9c0cba5385af26f3762b29c291ff62b) )
	ROM_LOAD16_BYTE( "mas07", 0x200000, 0x100000,  CRC(31303769) SHA1(509604be06ec8e0c1b56a81a8ffccdf0f79e9fd7) )

	ROM_REGION( 0x100000, REGION_GFX5, ROMREGION_DISPOSE )
	ROM_LOAD16_BYTE( "mas08", 0x000001, 0x080000,  CRC(e224fb7a) SHA1(9aa92fb98bddff313db2077c4db102e94c7af09b) ) /* Sprites #2 */
	ROM_LOAD16_BYTE( "mas09", 0x000000, 0x080000,  CRC(5f6deb41) SHA1(850d0e157b4355e866ec770a2012293b2c55648f) )

	ROM_REGION(0x80000, REGION_SOUND1, 0 ) /* Oki samples */
	ROM_LOAD( "mas10",  0x00000,  0x80000,  CRC(6edc06a7) SHA1(8ab92cca9d4a5d4fed3d99737c6f023f3f606db2) )

	ROM_REGION(0x80000, REGION_SOUND2, 0 ) /* Oki samples */
	ROM_LOAD( "mas11",  0x00000,  0x80000,  CRC(c2f0a4f2) SHA1(af71d649aea273c17d7fbcf8693e8a1d4b31f7f8) )

	ROM_REGION( 1024, REGION_PROMS, 0 )
	ROM_LOAD( "mb7122h.16l", 0x00000,  0x400,  CRC(2bee57cc) SHA1(bc48670aa7c39f6ff7fae4c819eab22ed2db875b) )	/* Priority (unused) */
ROM_END

ROM_START( nitrobal )
	ROM_REGION(0x200000, REGION_CPU1, 0 ) /* 68000 code */
	ROM_LOAD16_BYTE( "jl01-4.d3",   0x000000, 0x20000, CRC(0414e409) SHA1(bc19e7d2d9e768ce4052511043867c0ef9b0b61b) )
	ROM_LOAD16_BYTE( "jl00-4.b3",   0x000001, 0x20000, CRC(dd9e2bcc) SHA1(dede49a4fafcfa03f38ba6c1149c9f8b115fb306) )
	ROM_LOAD16_BYTE( "jl03-4.d5",   0x040000, 0x20000, CRC(ea264ac5) SHA1(ccdb87bbdd9e38537dd290d237d76ec32559efa3) )
	ROM_LOAD16_BYTE( "jl02-4.b5",   0x040001, 0x20000, CRC(74047997) SHA1(bfd2f24889250e06945bb4798b40a56f832a9b19) )
	ROM_LOAD16_BYTE( "jl05-2.d6",   0x080000, 0x40000, CRC(b820fa20) SHA1(8509567cf988fe27552d37241b25b66a6e1a9c39) )
	ROM_LOAD16_BYTE( "jl04-2.b6",   0x080001, 0x40000, CRC(1fd8995b) SHA1(75d77835500e4b7caca92ba634859d7a2ad9b84c) )
	/* Two empty rom slots at d7, b7 */

	ROM_REGION(0x10000, REGION_CPU2, 0 ) /* Sound CPU */
	ROM_LOAD( "jl08.r20",  0x00000,  0x10000,  CRC(93d93fe1) SHA1(efc618724251d23a23b3019d475f7739a7e88751) )

	ROM_REGION( 0x020000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD16_BYTE( "jl06.d10",  0x00000,  0x10000,  CRC(91cf668e) SHA1(fc153eaa09777f79369037a139470ad1118e8d7e) ) /* Chars */
	ROM_LOAD16_BYTE( "jl07.d12",  0x00001,  0x10000,  CRC(e61d0e42) SHA1(80d6ada356c721b0be826554ec6731dbbc19e0ab) )

	ROM_REGION( 0x100000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "mav00.b10", 0x00000, 0x80000,  CRC(34785d97) SHA1(094f881cd699d1b9fd079778f20f8c9d83283e6e) ) /* Tiles */
	ROM_LOAD( "mav01.b12", 0x80000, 0x80000,  CRC(8b531b16) SHA1(f734286f4510b2c09dc2d6d2b8c8da9dc4424287) )

	ROM_REGION( 0x200000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "mav02.b16", 0x000000, 0x100000,  CRC(20723bf7) SHA1(b3491d98ff415701fec2b58d85f99c743d71b013) ) /* Tiles */
	ROM_LOAD( "mav03.e16", 0x100000, 0x100000,  CRC(ef6195f0) SHA1(491bc030519c78b84396f7f8a21df9daf8acc140) )

	ROM_REGION( 0x300000, REGION_GFX4, ROMREGION_DISPOSE )
	ROM_LOAD16_BYTE( "mav05.e19", 0x000000, 0x100000,  CRC(d92d769c) SHA1(8012e7f2b9a7cbccde8da90025647443beb6c47c) ) /* Sprites #1 */
	ROM_LOAD16_BYTE( "mav04.b19", 0x000001, 0x100000,  CRC(8ba48385) SHA1(926ae1e0e99b8e022b6798ceb29dd080cfc1bada) )
	ROM_LOAD16_BYTE( "mav07.e20", 0x200000, 0x080000,  CRC(5fc10ccd) SHA1(7debcf223802d5c2ea3d29d39850c8756c863b31) )
	ROM_LOAD16_BYTE( "mav06.b20", 0x200001, 0x080000,  CRC(ae6201a5) SHA1(c0ae87fa96d12377c5522cb8adfed03373ab3757) )

	ROM_REGION( 0x80000, REGION_GFX5, ROMREGION_DISPOSE )
	ROM_LOAD16_BYTE( "mav09.e23", 0x000000, 0x040000,  CRC(1ce7b51a) SHA1(17ed8f34bf6d057e0504e72e95f448d5923aa82e) ) /* Sprites #2 */
	ROM_LOAD16_BYTE( "mav08.b23", 0x000001, 0x040000,  CRC(64966576) SHA1(40c14c0f62eef0317abfb7192505e0337fb5cde5) )

	ROM_REGION(0x80000, REGION_SOUND1, 0 ) /* Oki samples */
	ROM_LOAD( "mav10.r17",  0x00000,  0x80000,  CRC(8ad734b0) SHA1(768b9f54bbf4b54591cafecb7a27960da919ce84) )

	ROM_REGION(0x80000, REGION_SOUND2, 0 ) /* Oki samples */
	ROM_LOAD( "mav11.r19",  0x00000,  0x80000,  CRC(ef513908) SHA1(72db6c704071d7a784b3768c256fc51087e9e93c) )
ROM_END

ROM_START( hangzo )
	ROM_REGION(0x200000, REGION_CPU1, 0 ) /* 68000 code */
	ROM_LOAD16_BYTE( "Pro0H 12.18.2A.27C1001",  0x000000, 0x20000, CRC(ac8087db) SHA1(518193372cde6024fda96c6ed1862245e0bfb465) )
	ROM_LOAD16_BYTE( "Pro0H 12.18.2D.27C1001",  0x000001, 0x20000, CRC(a6b7f4f4) SHA1(1b3a00ef124d130317171d9042018fbb30662fec) )
	ROM_LOAD16_BYTE( "Pro1H 12.10.4A.27C010",   0x040000, 0x20000, CRC(0d04f43d) SHA1(167b595450f6f9b842dc909f6c61a96fa34b7991) )
	ROM_LOAD16_BYTE( "Pro1L 12.10.4D.27C010",   0x040001, 0x20000, CRC(2e323918) SHA1(f3d9168f395e835b075dfcbb0464770044d350f3) )
	ROM_LOAD16_BYTE( "Pro2H 12.10.6A.27C010",   0x080000, 0x20000, CRC(bb3185a6) SHA1(fa4ba7b4b53a5b3486c36441463a290b12c2acbe) )
	ROM_LOAD16_BYTE( "Pro2L 12.10.6D.27C010",   0x080001, 0x20000, CRC(11ce97bb) SHA1(d9c1872762f9acaeed1ebc640a71fa7a4b9d013c) )

	ROM_REGION(0x10000, REGION_CPU2, 0 ) /* Sound CPU */
	ROM_LOAD( "SND 12.18.18P.27C512",  0x00000,  0x10000,  CRC(97c592dc) SHA1(7a0e08f3ffd42d07d1d0a9db52c7fd85dba28bd8) )

	ROM_REGION( 0x040000, REGION_GFX1, ROMREGION_DISPOSE )
	/* filled in later */

	ROM_REGION( 0x100000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "BK1L 12.10.9A.574200",  0x000000, 0x080000,  CRC(5199729b) SHA1(1cb8e7b91e5d0b3a699e47b6bbb3e6e9c53c8590) ) /* Encrypted tiles */
	ROM_LOAD( "BK1H 12.10.11A.574200", 0x080000, 0x080000,  CRC(85887bd8) SHA1(6cb6f9c9e6e5824c6b8006ab195f27014031907e) )

	ROM_REGION( 0x200000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "BK23L 12.10.17D.574200",  0x000000, 0x080000,  CRC(ed4e47c6) SHA1(b09ed1a6bf1b42139c7817bae18cd5580c42cf91) ) /* tiles 1 & 2 */
	ROM_LOAD( "BK23H 12.10.18D.574200",  0x100000, 0x080000,  CRC(6a725fb2) SHA1(f4da4da62eb7e3ec2f1a54b57eaf94dc748dec68) )

	ROM_REGION( 0x400000, REGION_GFX4, ROMREGION_DISPOSE )
	ROM_LOAD( "OBJ01L 12.10.19A.27C4000", 0x000000, 0x080000,  CRC(c141e310) SHA1(81eb0b977aaf44a110a663416e385ca617de8f28) ) /* 6bpp sprites */
	ROM_LOAD( "OBJ01H 12.10.20A.27C4000", 0x100000, 0x080000,  CRC(6a7b4252) SHA1(4bd588bc96c07cc9367afdeab4976af6f8dcc823) )
	ROM_LOAD( "OBJ23L 12.10.19D.27C4000", 0x200000, 0x080000,  CRC(0db6df6c) SHA1(fe7ef7b5a279656d9e46334c4833ab8911caa5db) )
	ROM_LOAD( "OBJ23H 12.10.20D.27C4000", 0x300000, 0x080000,  CRC(165031a1) SHA1(0e88fe45fd78d352fdbd398c1d98feefe1b43917) )

	ROM_REGION(0x80000, REGION_SOUND2, 0 ) /* Oki samples */
	ROM_LOAD( "PCM16K 11.5.14P.574000", 0x00000,  0x80000,  CRC(5b95c6c7) SHA1(587e7f87d085af3a5d24f317fffc1716c8027e43) )

	ROM_REGION(0x80000, REGION_SOUND1, 0 ) /* Oki samples */
	ROM_LOAD( "PCM8K 11.5.15P.27C020", 0x00000,  0x40000,  CRC(02682a9a) SHA1(914ffc7c16e90c1ac28a228df415a956684f8192) )

	ROM_REGION( 512, REGION_PROMS,  0 )
	ROM_LOAD( "hb-00.11p", 0x00000,  0x200,  CRC(b7a7baad) SHA1(39781c3412493b985d3616ac31142fc00bbcddf4) ) /* ? */
ROM_END


/**********************************************************************************/

extern void decoprot_reset(void);

static DRIVER_INIT( rohga )
{
	deco56_decrypt(REGION_GFX1);
	deco56_decrypt(REGION_GFX2);
    decoprot_reset();
}



static DRIVER_INIT( wizdfire )
{
	deco74_decrypt(REGION_GFX1);
	deco74_decrypt(REGION_GFX2);
	deco74_decrypt(REGION_GFX3);
}

static DRIVER_INIT( nitrobal )
{
	deco56_decrypt(REGION_GFX1);
	deco56_decrypt(REGION_GFX2);
	deco74_decrypt(REGION_GFX3);
}

static DRIVER_INIT( hangzo )
{
	const data8_t *src = memory_region(REGION_GFX2);
	data8_t *dst = memory_region(REGION_GFX1);

	memcpy(dst,src,0x20000);
	memcpy(dst+0x20000,src+0x80000,0x20000);

    decoprot_reset();
}



GAME(1991, rohga,    0,       rohga,    rohga,    rohga,    ROT0,   "Data East Corporation", "Rohga Armour Force (Asia-Europe v3.0)" )
GAME(1991, rohgah,   rohga,   rohga,    rohga,    rohga,    ROT0,   "Data East Corporation", "Rohga Armour Force (Hong Kong v3.0)" )
GAME(1991, rohgau,   rohga,   rohga,    rohga,    rohga,    ROT0,   "Data East Corporation", "Rohga Armour Force (US v1.0)" )
GAME(1992, wizdfire, 0,       wizdfire, wizdfire, wizdfire, ROT0,   "Data East Corporation", "Wizard Fire (US v1.1)" )
GAME(1992, darksel2, wizdfire,wizdfire, wizdfire, wizdfire, ROT0,   "Data East Corporation", "Dark Seal 2 (Japan v2.1)" )
GAME(1992, nitrobal, 0,       nitrobal, nitrobal, nitrobal, ROT270, "Data East Corporation", "Nitro Ball (US)" )
GAME(1992, hangzo,   0,       hangzo,   hangzo,   hangzo,   ROT0,   "Hot-B",                 "Hangzo (Japan, Prototype)" )
