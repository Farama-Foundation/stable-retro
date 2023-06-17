/***************************************************************************

Return of the Invaders

driver by Jarek Parchanski, Andrea Mazzoleni

the game was developed by UPL for Taito.

Notes:
- I derived the ROM names from the board diagram in the manual. There might
  be some mistakes. The diagram actually shows 4 PROMs on the ROM board
  (a37-17, -18, -19 and -20), while we only have one: 82s191n. I think it's
  possible that the single 2KB PROM replaced four 512B PROMs in a later
  revision of the board.

- The video hardware (especially the sprite system) is quite obviously derived
  from a Namco design.

- Two bits of tilemap RAM might be used for tile flip, but the game never sets
  them so we can't verify without schematics.

- We don't have a dump of the original MCU. We have a dump from a bootleg MCU,
  which however cannot be the same as the original. The gaame works fine with it,
  but only when the flip screen dip switch is set to off. If it is set to on, it
  hangs when starting a game because the mcu doesn't answer a command.
  See MCU code at $206 and $435: when the dip switch is on, the lda #$00 should
  be replaced by lda #$01.

***************************************************************************/

#include "driver.h"

/* in machine */
READ_HANDLER( retofinv_68705_portA_r );
WRITE_HANDLER( retofinv_68705_portA_w );
WRITE_HANDLER( retofinv_68705_ddrA_w );
READ_HANDLER( retofinv_68705_portB_r );
WRITE_HANDLER( retofinv_68705_portB_w );
WRITE_HANDLER( retofinv_68705_ddrB_w );
READ_HANDLER( retofinv_68705_portC_r );
WRITE_HANDLER( retofinv_68705_portC_w );
WRITE_HANDLER( retofinv_68705_ddrC_w );
WRITE_HANDLER( retofinv_mcu_w );
READ_HANDLER( retofinv_mcu_r );
READ_HANDLER( retofinv_mcu_status_r );

/* in vidhrdw */
VIDEO_START( retofinv );
PALETTE_INIT( retofinv );
VIDEO_UPDATE( retofinv );
READ_HANDLER( retofinv_bg_videoram_r );
READ_HANDLER( retofinv_fg_videoram_r );
WRITE_HANDLER( retofinv_bg_videoram_w );
WRITE_HANDLER( retofinv_fg_videoram_w );
WRITE_HANDLER( retofinv_gfx_ctrl_w );

extern data8_t *retofinv_fg_videoram;
extern data8_t *retofinv_bg_videoram;
extern data8_t *retofinv_sharedram;

static unsigned char cpu2_m6000=0;


static READ_HANDLER( retofinv_shared_ram_r )
{
	return retofinv_sharedram[offset];
}

static WRITE_HANDLER( retofinv_shared_ram_w )
{
	retofinv_sharedram[offset] = data;
}

static WRITE_HANDLER( cpu1_reset_w )
{
	cpu_set_reset_line(1, data ? CLEAR_LINE : ASSERT_LINE);
}

static WRITE_HANDLER( cpu2_reset_w )
{
	cpu_set_reset_line(2, data ? CLEAR_LINE : ASSERT_LINE);
}

static WRITE_HANDLER( mcu_reset_w )
{
	/* the bootlegs don't have a MCU, so make sure it's there before trying to reset it */
	if (cpu_gettotalcpu() >= 4)
		cpu_set_reset_line(3, data ? CLEAR_LINE : ASSERT_LINE);
}

static WRITE_HANDLER( cpu2_m6000_w )
{
	cpu2_m6000 = data;
}

static READ_HANDLER( cpu0_mf800_r )
{
	return cpu2_m6000;
}

static WRITE_HANDLER( soundcommand_w )
{
      soundlatch_w(0, data);
      cpu_set_irq_line(2, 0, HOLD_LINE);
}

static WRITE_HANDLER( irq0_ack_w )
{
	int bit = data & 1;

	cpu_interrupt_enable(0,bit);
	if (!bit)
		cpu_set_irq_line(0, 0, CLEAR_LINE);
}

static WRITE_HANDLER( irq1_ack_w )
{
	int bit = data & 1;

	cpu_interrupt_enable(1,bit);
	if (!bit)
		cpu_set_irq_line(1, 0, CLEAR_LINE);
}

static WRITE_HANDLER( coincounter_w )
{
	coin_counter_w(0, data & 1);
}

static WRITE_HANDLER( coinlockout_w )
{
	coin_lockout_w(0,~data & 1);
}


static MEMORY_READ_START( main_readmem )
    { 0x0000, 0x5fff, MRA_ROM },
	{ 0x7b00, 0x7bff, MRA_ROM },	/* space for diagnostic ROM? The code looks */
									/* for a string here, and jumps if it's present */
	{ 0x8000, 0x87ff, retofinv_fg_videoram_r },
	{ 0x8800, 0x9fff, retofinv_shared_ram_r },
	{ 0xa000, 0xa7ff, retofinv_bg_videoram_r },
	{ 0xc000, 0xc000, input_port_1_r },
	{ 0xc001, 0xc001, input_port_2_r },
	{ 0xc002, 0xc002, MRA_NOP },	/* bit 7 must be 0, otherwise game resets */
	{ 0xc003, 0xc003, retofinv_mcu_status_r },
	{ 0xc004, 0xc004, input_port_0_r },
	{ 0xc005, 0xc005, input_port_3_r },
	{ 0xc006, 0xc006, input_port_4_r },
	{ 0xc007, 0xc007, input_port_5_r },
	{ 0xe000, 0xe000, retofinv_mcu_r },
	{ 0xf800, 0xf800, cpu0_mf800_r },
MEMORY_END

static MEMORY_WRITE_START( main_writemem )
    { 0x0000, 0x5fff, MWA_ROM },
	{ 0x7fff, 0x7fff, coincounter_w },
	{ 0x7b00, 0x7bff, MWA_ROM },	/* space for diagnostic ROM? The code looks */
									/* for a string here, and jumps if it's present */
	{ 0x8000, 0x87ff, retofinv_fg_videoram_w, &retofinv_fg_videoram },
	{ 0x8800, 0x9fff, retofinv_shared_ram_w, &retofinv_sharedram },
	{ 0xa000, 0xa7ff, retofinv_bg_videoram_w, &retofinv_bg_videoram },
	{ 0xb800, 0xb802, retofinv_gfx_ctrl_w },
	{ 0xc800, 0xc800, irq0_ack_w },
	{ 0xc801, 0xc801, coinlockout_w },
	{ 0xc802, 0xc802, cpu2_reset_w },
	{ 0xc803, 0xc803, mcu_reset_w },
/*	{ 0xc804, 0xc804, irq1_ack_w }, */ /* presumably (meaning memory map is shared with cpu 1) */
    { 0xc805, 0xc805, cpu1_reset_w },
	{ 0xd000, 0xd000, watchdog_reset_w },
	{ 0xd800, 0xd800, soundcommand_w },
	{ 0xe800, 0xe800, retofinv_mcu_w },
MEMORY_END

static MEMORY_READ_START( sub_readmem )
    { 0x0000, 0x1fff, MRA_ROM },
	{ 0x8000, 0x87ff, retofinv_fg_videoram_r },
	{ 0x8800, 0x9fff, retofinv_shared_ram_r },
	{ 0xa000, 0xa7ff, retofinv_bg_videoram_r },
MEMORY_END

static MEMORY_WRITE_START( sub_writemem )
    { 0x0000, 0x1fff, MWA_ROM },
	{ 0x8000, 0x87ff, retofinv_fg_videoram_w },
	{ 0x8800, 0x9fff, retofinv_shared_ram_w },
	{ 0xa000, 0xa7ff, retofinv_bg_videoram_w },
	{ 0xc804, 0xc804, irq1_ack_w },
MEMORY_END

static MEMORY_READ_START( sound_readmem )
    { 0x0000, 0x1fff, MRA_ROM },
	{ 0x2000, 0x27ff, MRA_RAM },
	{ 0x4000, 0x4000, soundlatch_r },
	{ 0xe000, 0xffff, MRA_ROM }, 		/* space for diagnostic ROM */
MEMORY_END

static MEMORY_WRITE_START( sound_writemem )
    { 0x0000, 0x1fff, MWA_ROM },
	{ 0x2000, 0x27ff, MWA_RAM },
	{ 0x6000, 0x6000, cpu2_m6000_w },
	{ 0x8000, 0x8000, SN76496_0_w },
	{ 0xa000, 0xa000, SN76496_1_w },
	{ 0xe000, 0xffff, MWA_ROM }, 		/* space for diagnostic ROM */
MEMORY_END

static MEMORY_READ_START( mcu_readmem )
	{ 0x0000, 0x0000, retofinv_68705_portA_r },
	{ 0x0001, 0x0001, retofinv_68705_portB_r },
	{ 0x0002, 0x0002, retofinv_68705_portC_r },
	{ 0x0010, 0x007f, MRA_RAM },
	{ 0x0080, 0x07ff, MRA_ROM },
MEMORY_END

static MEMORY_WRITE_START( mcu_writemem )
	{ 0x0000, 0x0000, retofinv_68705_portA_w },
	{ 0x0001, 0x0001, retofinv_68705_portB_w },
	{ 0x0002, 0x0002, retofinv_68705_portC_w },
	{ 0x0004, 0x0004, retofinv_68705_ddrA_w },
	{ 0x0005, 0x0005, retofinv_68705_ddrB_w },
	{ 0x0006, 0x0006, retofinv_68705_ddrC_w },
	{ 0x0010, 0x007f, MWA_RAM },
	{ 0x0080, 0x07ff, MWA_ROM },
MEMORY_END


INPUT_PORTS_START( retofinv )
	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_TILT )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH,IPT_COIN1 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH,IPT_COIN2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_2WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_2WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON1 )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_2WAY | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_2WAY | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )

	PORT_START
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x03, "30k, 80k & every 80k" )
	PORT_DIPSETTING(    0x02, "30k, 80k" )
	PORT_DIPSETTING(    0x01, "30k" )
	PORT_DIPSETTING(    0x00, "None" )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Free_Play ) )
	PORT_DIPSETTING(    0x04, DEF_STR( No ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x18, 0x08, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x18, "1" )
	PORT_DIPSETTING(    0x10, "2" )
	PORT_DIPSETTING(    0x08, "3" )
	PORT_DIPSETTING(    0x00, "5" )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unused ) )	/* according to manual */
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Cocktail ) )

    PORT_START
	PORT_DIPNAME( 0x0f, 0x00, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x0f, DEF_STR( 9C_1C ) )
	PORT_DIPSETTING(    0x0e, DEF_STR( 8C_1C ) )
	PORT_DIPSETTING(    0x0d, DEF_STR( 7C_1C ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( 6C_1C ) )
	PORT_DIPSETTING(    0x0b, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(    0x0a, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x09, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0x05, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 1C_7C ) )
	PORT_DIPSETTING(    0x07, DEF_STR( 1C_8C ) )
	PORT_DIPNAME( 0xf0, 0x00, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0xf0, DEF_STR( 9C_1C ) )
	PORT_DIPSETTING(    0xe0, DEF_STR( 8C_1C ) )
	PORT_DIPSETTING(    0xd0, DEF_STR( 7C_1C ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 6C_1C ) )
	PORT_DIPSETTING(    0xb0, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(    0xa0, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x90, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0x50, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(    0x60, DEF_STR( 1C_7C ) )
	PORT_DIPSETTING(    0x70, DEF_STR( 1C_8C ) )

	PORT_START
	PORT_DIPNAME( 0x01, 0x01, "Push Start to Skip Stage (Cheat)")
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unused ) )	/* according to manual */
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unused ) )	/* according to manual */
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unused ) )	/* according to manual */
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, "Coin Per Play Display" )
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x20, 0x20, "Year Display" )
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x40, 0x40, "Invulnerability (Cheat)")
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Coinage ) )	/* unused according to manual */
	PORT_DIPSETTING(    0x80, "A and B" )
	PORT_DIPSETTING(    0x00, "A only" )
INPUT_PORTS_END


static struct GfxLayout charlayout =
{
	8,8,
	RGN_FRAC(1,2),	/* bottom half of ROM is empty */
	1,
	{ 0 },
	{ 7, 6, 5, 4, 3, 2, 1, 0 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8
};

static struct GfxLayout bglayout =
{
	8,8,
	RGN_FRAC(1,2),
	4,
	{ 0, RGN_FRAC(1,2)+4, RGN_FRAC(1,2), 4 },
	{ 0, 1, 2, 3, 8*8, 8*8+1, 8*8+2, 8*8+3 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	16*8
};

static struct GfxLayout spritelayout =
{
	16,16,
	RGN_FRAC(1,2),
	4,
	{ 0, RGN_FRAC(1,2)+4, RGN_FRAC(1,2), 4 },
	{ 0, 1, 2, 3, 8*8, 8*8+1, 8*8+2, 8*8+3, 16*8+0, 16*8+1, 16*8+2, 16*8+3,
			24*8+0, 24*8+1, 24*8+2, 24*8+3 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
			32*8, 33*8, 34*8, 35*8, 36*8, 37*8, 38*8, 39*8 },
  	64*8
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &charlayout,             0, 256 },
	{ REGION_GFX2, 0, &bglayout,     64*16+256*2,  64 },
	{ REGION_GFX3, 0, &spritelayout,       256*2,  64 },
	{ -1 } /* end of array */
};



static struct SN76496interface sn76496_interface =
{
	2,		/* 2 chips */
	{ 18432000/6, 18432000/6 },
	{ 80, 80 }
};



static MACHINE_DRIVER_START( retofinv )

	/* basic machine hardware */
	MDRV_CPU_ADD(Z80, 18432000/6)	/* 3.072 MHz? */
	MDRV_CPU_MEMORY(main_readmem,main_writemem)
	MDRV_CPU_VBLANK_INT(irq0_line_assert,1)

	MDRV_CPU_ADD(Z80, 18432000/6)	/* 3.072 MHz? */
	MDRV_CPU_MEMORY(sub_readmem,sub_writemem)
	MDRV_CPU_VBLANK_INT(irq0_line_assert,1)

	MDRV_CPU_ADD(Z80, 18432000/6)	/* 3.072 MHz? */
	MDRV_CPU_MEMORY(sound_readmem,sound_writemem)
	MDRV_CPU_VBLANK_INT(nmi_line_pulse,2)

	MDRV_CPU_ADD(M68705,18432000/6)	/* 3.072 MHz? */
	MDRV_CPU_MEMORY(mcu_readmem,mcu_writemem)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(100)	/* 100 CPU slices per frame - enough for the sound CPU to read all commands */

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(36*8, 28*8)
	MDRV_VISIBLE_AREA(0*8, 36*8-1, 0*8, 28*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(256)
	MDRV_COLORTABLE_LENGTH(256*2+64*16+64*16)

	MDRV_PALETTE_INIT(retofinv)
	MDRV_VIDEO_START(retofinv)
	MDRV_VIDEO_UPDATE(retofinv)

	/* sound hardware */
	MDRV_SOUND_ADD(SN76496, sn76496_interface)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( retofinb )

	/* basic machine hardware */
	MDRV_CPU_ADD(Z80, 18432000/6)	/* 3.072 MHz? */
	MDRV_CPU_MEMORY(main_readmem,main_writemem)
	MDRV_CPU_VBLANK_INT(irq0_line_assert,1)

	MDRV_CPU_ADD(Z80, 18432000/6)	/* 3.072 MHz? */
	MDRV_CPU_MEMORY(sub_readmem,sub_writemem)
	MDRV_CPU_VBLANK_INT(irq0_line_assert,1)

	MDRV_CPU_ADD(Z80, 18432000/6)	/* 3.072 MHz? */
	MDRV_CPU_MEMORY(sound_readmem,sound_writemem)
	MDRV_CPU_VBLANK_INT(nmi_line_pulse,2)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(100)	/* 100 CPU slices per frame - enough for the sound CPU to read all commands */

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(36*8, 28*8)
	MDRV_VISIBLE_AREA(0*8, 36*8-1, 0*8, 28*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(256)
	MDRV_COLORTABLE_LENGTH(256*2+64*16+64*16)

	MDRV_PALETTE_INIT(retofinv)
	MDRV_VIDEO_START(retofinv)
	MDRV_VIDEO_UPDATE(retofinv)

	/* sound hardware */
	MDRV_SOUND_ADD(SN76496, sn76496_interface)
MACHINE_DRIVER_END


/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( retofinv )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "a37-03.70", 0x0000, 0x2000, CRC(eae7459d) SHA1(c105f6adbd4c09decaad68ed13163d8f9b55e646) )
	ROM_LOAD( "a37-02.71", 0x2000, 0x2000, CRC(72895e37) SHA1(42fb904338e9f92a79d587eac401d456e7fb6e55) )
	ROM_LOAD( "a37-01.72", 0x4000, 0x2000, CRC(505dd20b) SHA1(3a34b1515bb834ff9e2d86b0b43a752d9619307b) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for code */
	ROM_LOAD( "a37-04.62", 0x0000, 0x2000, CRC(d2899cc1) SHA1(fdbec743b06f4cdcc134ef863e4e71337ad0b2c5) )

	ROM_REGION( 0x10000, REGION_CPU3, 0 )	/* 64k for sound cpu */
	ROM_LOAD( "a37-05.17", 0x0000, 0x2000, CRC(9025abea) SHA1(2f03e8572f23624d7cd1215a55109e97fd66e271) )

	ROM_REGION( 0x0800,  REGION_CPU4, 0 )	/* 8k for the microcontroller */
	ROM_LOAD( "a37__09.ic37", 0x00000, 0x0800, CRC(6a6d008d) SHA1(dce55b65db22ba97cb7b3d6d545575e7945d42ad) ) /* original mc68705p5 from Taito board */
	
	ROM_REGION( 0x02000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "a37-16.61", 0x0000, 0x2000, CRC(4e3f501c) SHA1(2d832f4038ae65bfdeedfab870f6f1176ec6b676) )

	ROM_REGION( 0x04000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "a37-14.55", 0x0000, 0x2000, CRC(ef7f8651) SHA1(2d91057501e5e9c4255e0d55fff0d99c2a5be7e8) )
	ROM_LOAD( "a37-15.56", 0x2000, 0x2000, CRC(03b40905) SHA1(c10d87796e8a6e6a2a37c6fb713821cc87299cc8) )

	ROM_REGION( 0x08000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "a37-10.8",  0x0000, 0x2000, CRC(6afdeec8) SHA1(782fe0a8aea48c3c270318b7ba011fc6fce0db7a) )
	ROM_LOAD( "a37-11.9",  0x2000, 0x2000, CRC(d3dc9da3) SHA1(0d98d6e993b5a4845a23802751023b7a593dce29) )
	ROM_LOAD( "a37-12.10", 0x4000, 0x2000, CRC(d10b2eed) SHA1(3809a0adf935a119f9ee0d4c24f1456c35d2a6fa) )
	ROM_LOAD( "a37-13.11", 0x6000, 0x2000, CRC(00ca6b3d) SHA1(08ce5b13d5ebc79cc803949f4ba9e630e6cd92b8) )

	ROM_REGION( 0x0b00, REGION_PROMS, 0 )
	ROM_LOAD( "a37-06.13", 0x0000, 0x0100, CRC(e9643b8b) SHA1(7bbb92a42e7c3effb701fc7b2c24f2470f31b063) )	/* palette red bits  */
	ROM_LOAD( "a37-07.4",  0x0100, 0x0100, CRC(e8f34e11) SHA1(8f438561b8d46ffff00747ed8baf0ebb6a081615) )	/* palette green bits */
	ROM_LOAD( "a37-08.3",  0x0200, 0x0100, CRC(50030af0) SHA1(e748ae0b8702b7d20fb65c254dceee23246b3d13) )	/* palette blue bits   */
	ROM_LOAD( "82s191n",   0x0300, 0x0800, CRC(93c891e3) SHA1(643a0107717b6a434432dda73a0102e6e8adbca7) )	/* lookup table */
ROM_END

ROM_START( retofin1 )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "roi.02",  0x0000, 0x2000, CRC(d98fd462) SHA1(fd35e13b7dee58639a01b040b8f84d42bb40b633) )
	ROM_LOAD( "roi.01b", 0x2000, 0x2000, CRC(3379f930) SHA1(c67d687a10b6240bd6e2fbdb15e1b7d276e6fc07) )
	ROM_LOAD( "roi.01",  0x4000, 0x2000, CRC(57679062) SHA1(4f121101ab1cb8de8e693e5984ef23fa587fe696) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for code */
	ROM_LOAD( "a37-04.62", 0x0000, 0x2000, CRC(d2899cc1) SHA1(fdbec743b06f4cdcc134ef863e4e71337ad0b2c5) )

	ROM_REGION( 0x10000, REGION_CPU3, 0 )	/* 64k for sound cpu */
	ROM_LOAD( "a37-05.17", 0x0000, 0x2000, CRC(9025abea) SHA1(2f03e8572f23624d7cd1215a55109e97fd66e271) )

	ROM_REGION( 0x02000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "a37-16.61", 0x0000, 0x2000, CRC(4e3f501c) SHA1(2d832f4038ae65bfdeedfab870f6f1176ec6b676) )

	ROM_REGION( 0x04000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "a37-14.55", 0x0000, 0x2000, CRC(ef7f8651) SHA1(2d91057501e5e9c4255e0d55fff0d99c2a5be7e8) )
	ROM_LOAD( "a37-15.56", 0x2000, 0x2000, CRC(03b40905) SHA1(c10d87796e8a6e6a2a37c6fb713821cc87299cc8) )

	ROM_REGION( 0x08000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "a37-10.8",  0x0000, 0x2000, CRC(6afdeec8) SHA1(782fe0a8aea48c3c270318b7ba011fc6fce0db7a) )
	ROM_LOAD( "a37-11.9",  0x2000, 0x2000, CRC(d3dc9da3) SHA1(0d98d6e993b5a4845a23802751023b7a593dce29) )
	ROM_LOAD( "a37-12.10", 0x4000, 0x2000, CRC(d10b2eed) SHA1(3809a0adf935a119f9ee0d4c24f1456c35d2a6fa) )
	ROM_LOAD( "a37-13.11", 0x6000, 0x2000, CRC(00ca6b3d) SHA1(08ce5b13d5ebc79cc803949f4ba9e630e6cd92b8) )

	ROM_REGION( 0x0b00, REGION_PROMS, 0 )
	ROM_LOAD( "a37-06.13", 0x0000, 0x0100, CRC(e9643b8b) SHA1(7bbb92a42e7c3effb701fc7b2c24f2470f31b063) )	/* palette red bits  */
	ROM_LOAD( "a37-07.4",  0x0100, 0x0100, CRC(e8f34e11) SHA1(8f438561b8d46ffff00747ed8baf0ebb6a081615) )	/* palette green bits */
	ROM_LOAD( "a37-08.3",  0x0200, 0x0100, CRC(50030af0) SHA1(e748ae0b8702b7d20fb65c254dceee23246b3d13) )	/* palette blue bits   */
	ROM_LOAD( "82s191n",   0x0300, 0x0800, CRC(93c891e3) SHA1(643a0107717b6a434432dda73a0102e6e8adbca7) )	/* lookup table */
ROM_END

ROM_START( retofin2 )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "ri-c.1e", 0x0000, 0x2000, CRC(e3c31260) SHA1(cc8774251c567da2e4a54091223927c95f497fe8) )
	ROM_LOAD( "roi.01b", 0x2000, 0x2000, CRC(3379f930) SHA1(c67d687a10b6240bd6e2fbdb15e1b7d276e6fc07) )
	ROM_LOAD( "ri-a.1c", 0x4000, 0x2000, CRC(3ae7c530) SHA1(5d1be375494fa07124071067661c4bfc2d724d54) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for code */
	ROM_LOAD( "a37-04.62", 0x0000, 0x2000, CRC(d2899cc1) SHA1(fdbec743b06f4cdcc134ef863e4e71337ad0b2c5) )

	ROM_REGION( 0x10000, REGION_CPU3, 0 )	/* 64k for sound cpu */
	ROM_LOAD( "a37-05.17", 0x0000, 0x2000, CRC(9025abea) SHA1(2f03e8572f23624d7cd1215a55109e97fd66e271) )

	ROM_REGION( 0x02000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "a37-16.61", 0x0000, 0x2000, CRC(4e3f501c) SHA1(2d832f4038ae65bfdeedfab870f6f1176ec6b676) )

	ROM_REGION( 0x04000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "a37-14.55", 0x0000, 0x2000, CRC(ef7f8651) SHA1(2d91057501e5e9c4255e0d55fff0d99c2a5be7e8) )
	ROM_LOAD( "a37-15.56", 0x2000, 0x2000, CRC(03b40905) SHA1(c10d87796e8a6e6a2a37c6fb713821cc87299cc8) )

	ROM_REGION( 0x08000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "a37-10.8",  0x0000, 0x2000, CRC(6afdeec8) SHA1(782fe0a8aea48c3c270318b7ba011fc6fce0db7a) )
	ROM_LOAD( "a37-11.9",  0x2000, 0x2000, CRC(d3dc9da3) SHA1(0d98d6e993b5a4845a23802751023b7a593dce29) )
	ROM_LOAD( "a37-12.10", 0x4000, 0x2000, CRC(d10b2eed) SHA1(3809a0adf935a119f9ee0d4c24f1456c35d2a6fa) )
	ROM_LOAD( "a37-13.11", 0x6000, 0x2000, CRC(00ca6b3d) SHA1(08ce5b13d5ebc79cc803949f4ba9e630e6cd92b8) )

	ROM_REGION( 0x0b00, REGION_PROMS, 0 )
	ROM_LOAD( "a37-06.13", 0x0000, 0x0100, CRC(e9643b8b) SHA1(7bbb92a42e7c3effb701fc7b2c24f2470f31b063) )	/* palette red bits  */
	ROM_LOAD( "a37-07.4",  0x0100, 0x0100, CRC(e8f34e11) SHA1(8f438561b8d46ffff00747ed8baf0ebb6a081615) )	/* palette green bits */
	ROM_LOAD( "a37-08.3",  0x0200, 0x0100, CRC(50030af0) SHA1(e748ae0b8702b7d20fb65c254dceee23246b3d13) )	/* palette blue bits   */
	ROM_LOAD( "82s191n",   0x0300, 0x0800, CRC(93c891e3) SHA1(643a0107717b6a434432dda73a0102e6e8adbca7) )	/* lookup table */
ROM_END


GAME( 1985, retofinv, 0,        retofinv, retofinv, 0, ROT90, "Taito Corporation", "Return of the Invaders" )
GAME( 1985, retofin1, retofinv, retofinb, retofinv, 0, ROT90, "bootleg", "Return of the Invaders (bootleg set 1)" )
GAME( 1985, retofin2, retofinv, retofinb, retofinv, 0, ROT90, "bootleg", "Return of the Invaders (bootleg set 2)" )
