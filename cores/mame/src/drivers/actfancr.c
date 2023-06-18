/*******************************************************************************

	Act Fancer (Japan)				FD (c) 1989 Data East Corporation
	Act Fancer (World)				FE (c) 1989 Data East Corporation
	Trio The Punch (Japan)			FF (c) 1989 Data East Corporation

	The 'World' set has rom code FE, the 'Japan' set has rom code FD.

	Most Data East games give the Japanese version the earlier code, though
	there is no real difference between the sets.

	I believe the USA version of Act Fancer is called 'Out Fencer'

	Emulation by Bryan McPhail, mish@tendril.co.uk

*******************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "cpu/m6502/m6502.h"
#include "cpu/h6280/h6280.h"

VIDEO_UPDATE( actfancr );
VIDEO_UPDATE( triothep );
WRITE_HANDLER( actfancr_pf1_data_w );
READ_HANDLER( actfancr_pf1_data_r );
WRITE_HANDLER( actfancr_pf1_control_w );
WRITE_HANDLER( actfancr_pf2_data_w );
READ_HANDLER( actfancr_pf2_data_r );
WRITE_HANDLER( actfancr_pf2_control_w );
VIDEO_START( actfancr );
VIDEO_START( triothep );

extern unsigned char *actfancr_pf1_data,*actfancr_pf2_data,*actfancr_pf1_rowscroll_data;
static unsigned char *actfancr_ram;

/******************************************************************************/

static READ_HANDLER( actfan_control_0_r )
{
	return readinputport(2); /* VBL */
}

static READ_HANDLER( actfan_control_1_r )
{
	switch (offset) {
		case 0: return readinputport(0); /* Player 1 */
		case 1: return readinputport(1); /* Player 2 */
		case 2: return readinputport(3); /* Dip 1 */
		case 3: return readinputport(4); /* Dip 2 */
	}
	return 0xff;
}

static int trio_control_select;

static WRITE_HANDLER( triothep_control_select_w )
{
	trio_control_select=data;
}

static READ_HANDLER( triothep_control_r )
{
	switch (trio_control_select) {
		case 0: return readinputport(0); /* Player 1 */
		case 1: return readinputport(1); /* Player 2 */
		case 2: return readinputport(3); /* Dip 1 */
		case 3: return readinputport(4); /* Dip 2 */
		case 4: return readinputport(2); /* VBL */
	}

	return 0xff;
}

static WRITE_HANDLER( actfancr_sound_w )
{
	soundlatch_w(0,data & 0xff);
	cpu_set_irq_line(1, IRQ_LINE_NMI, PULSE_LINE);
}

/******************************************************************************/

static MEMORY_READ_START( actfan_readmem )
	{ 0x000000, 0x02ffff, MRA_ROM },
	{ 0x062000, 0x063fff, actfancr_pf1_data_r },
	{ 0x072000, 0x0727ff, actfancr_pf2_data_r },
	{ 0x100000, 0x1007ff, MRA_RAM },
	{ 0x130000, 0x130003, actfan_control_1_r },
	{ 0x140000, 0x140001, actfan_control_0_r },
	{ 0x120000, 0x1205ff, paletteram_r },
	{ 0x1f0000, 0x1f3fff, MRA_RAM },
MEMORY_END

static MEMORY_WRITE_START( actfan_writemem )
	{ 0x000000, 0x02ffff, MWA_ROM },
	{ 0x060000, 0x06001f, actfancr_pf1_control_w },
	{ 0x062000, 0x063fff, actfancr_pf1_data_w, &actfancr_pf1_data },
	{ 0x070000, 0x07001f, actfancr_pf2_control_w },
	{ 0x072000, 0x0727ff, actfancr_pf2_data_w, &actfancr_pf2_data },
	{ 0x100000, 0x1007ff, MWA_RAM, &spriteram, &spriteram_size },
	{ 0x110000, 0x110001, buffer_spriteram_w },
	{ 0x120000, 0x1205ff, paletteram_xxxxBBBBGGGGRRRR_w, &paletteram },
	{ 0x150000, 0x150001, actfancr_sound_w },
	{ 0x1f0000, 0x1f3fff, MWA_RAM, &actfancr_ram }, /* Main ram */
MEMORY_END

static MEMORY_READ_START( triothep_readmem )
	{ 0x000000, 0x03ffff, MRA_ROM },
	{ 0x044000, 0x045fff, actfancr_pf2_data_r },
	{ 0x064000, 0x0647ff, actfancr_pf1_data_r },
	{ 0x120000, 0x1207ff, MRA_RAM },
	{ 0x130000, 0x1305ff, paletteram_r },
	{ 0x140000, 0x140001, MRA_NOP }, /* Value doesn't matter */
	{ 0x1f0000, 0x1f3fff, MRA_RAM },
	{ 0x1ff000, 0x1ff001, triothep_control_r },
MEMORY_END

static MEMORY_WRITE_START( triothep_writemem )
	{ 0x000000, 0x03ffff, MWA_ROM },
	{ 0x040000, 0x04001f, actfancr_pf2_control_w },
	{ 0x044000, 0x045fff, actfancr_pf2_data_w, &actfancr_pf2_data },
	{ 0x046400, 0x0467ff, MWA_NOP }, /* Pf2 rowscroll - is it used? */
	{ 0x060000, 0x06001f, actfancr_pf1_control_w },
	{ 0x064000, 0x0647ff, actfancr_pf1_data_w, &actfancr_pf1_data },
	{ 0x066400, 0x0667ff, MWA_RAM, &actfancr_pf1_rowscroll_data },
	{ 0x100000, 0x100001, actfancr_sound_w },
	{ 0x110000, 0x110001, buffer_spriteram_w },
	{ 0x120000, 0x1207ff, MWA_RAM, &spriteram, &spriteram_size },
	{ 0x130000, 0x1305ff, paletteram_xxxxBBBBGGGGRRRR_w, &paletteram },
	{ 0x1f0000, 0x1f3fff, MWA_RAM, &actfancr_ram }, /* Main ram */
	{ 0x1ff000, 0x1ff001, triothep_control_select_w },
	{ 0x1ff402, 0x1ff403, H6280_irq_status_w },
MEMORY_END

/******************************************************************************/

static MEMORY_READ_START( dec0_s_readmem )
	{ 0x0000, 0x07ff, MRA_RAM },
	{ 0x3000, 0x3000, soundlatch_r },
	{ 0x3800, 0x3800, OKIM6295_status_0_r },
	{ 0x4000, 0xffff, MRA_ROM },
MEMORY_END

static MEMORY_WRITE_START( dec0_s_writemem )
	{ 0x0000, 0x07ff, MWA_RAM },
	{ 0x0800, 0x0800, YM2203_control_port_0_w },
	{ 0x0801, 0x0801, YM2203_write_port_0_w },
	{ 0x1000, 0x1000, YM3812_control_port_0_w },
	{ 0x1001, 0x1001, YM3812_write_port_0_w },
	{ 0x3800, 0x3800, OKIM6295_data_0_w },
	{ 0x4000, 0xffff, MWA_ROM },
MEMORY_END

/******************************************************************************/

INPUT_PORTS_START( actfancr )
	PORT_START	/* Player 1 controls */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START	/* Player 2 controls */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START2 )

	PORT_START	/* start buttons */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_VBLANK )

	PORT_START	/* Dip switch bank 1 */
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 1C_2C ) )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 1C_2C ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x20, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Cocktail ) )

	PORT_START	/* Dip switch bank 2 */
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x03, "3" )
	PORT_DIPSETTING(    0x02, "4" )
	PORT_DIPSETTING(    0x01, "5" )
	PORT_BITX(0,  0x00, IPT_DIPSWITCH_SETTING | IPF_CHEAT, "100", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x04, "Easy" )
	PORT_DIPSETTING(    0x0c, "Normal" )
	PORT_DIPSETTING(    0x08, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x20, "800000" )
	PORT_DIPSETTING(    0x00, "None" )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
INPUT_PORTS_END

INPUT_PORTS_START( triothep )
	PORT_START	/* Player 1 controls */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START	/* Player 2 controls */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_COCKTAIL )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START2 )

	PORT_START	/* start buttons */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_VBLANK )

	PORT_START	/* Dip switch bank 1 */
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 1C_2C ) )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 1C_2C ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x20, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Cocktail ) )

	PORT_START	/* Dip switch bank 2 */
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "8" )
	PORT_DIPSETTING(    0x01, "10" )
	PORT_DIPSETTING(    0x03, "12" )
	PORT_DIPSETTING(    0x02, "14" )
	PORT_DIPNAME( 0x0c, 0x0c, "Difficulty (Time)" )
	PORT_DIPSETTING(    0x08, "Easy (130)" )
	PORT_DIPSETTING(    0x0c, "Normal (100)" )
	PORT_DIPSETTING(    0x04, "Hard (70)" )
	PORT_DIPSETTING(    0x00, "Hardest (60)" )
	PORT_DIPNAME( 0x10, 0x10, "Bonus Lives" )
	PORT_DIPSETTING(    0x00, "2" )
	PORT_DIPSETTING(    0x10, "3" )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
INPUT_PORTS_END

/******************************************************************************/

static struct GfxLayout chars =
{
	8,8,	/* 8*8 chars */
	4096,
	4,		/* 4 bits per pixel  */
	{ 0x08000*8, 0x18000*8, 0x00000*8, 0x10000*8 },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8	/* every char takes 8 consecutive bytes */
};

static struct GfxLayout tiles =
{
	16,16,	/* 16*16 sprites */
	2048,
	4,
	{ 0, 0x10000*8, 0x20000*8,0x30000*8 },	/* plane offset */
	{ 16*8+0, 16*8+1, 16*8+2, 16*8+3, 16*8+4, 16*8+5, 16*8+6, 16*8+7,
			0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
			8*8, 9*8, 10*8, 11*8, 12*8, 13*8, 14*8, 15*8 },
	32*8	/* every sprite takes 32 consecutive bytes */
};

static struct GfxLayout sprites =
{
	16,16,	/* 16*16 sprites */
	2048+1024,
	4,
	{ 0, 0x18000*8, 0x30000*8, 0x48000*8 },	/* plane offset */
	{ 16*8+0, 16*8+1, 16*8+2, 16*8+3, 16*8+4, 16*8+5, 16*8+6, 16*8+7,
			0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
			8*8, 9*8, 10*8, 11*8, 12*8, 13*8, 14*8, 15*8 },
	32*8	/* every sprite takes 32 consecutive bytes */
};

static struct GfxDecodeInfo actfan_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &chars,       0, 16 },
	{ REGION_GFX2, 0, &sprites,   512, 16 },
	{ REGION_GFX3, 0, &tiles,     256, 16 },
	{ -1 } /* end of array */
};

static struct GfxDecodeInfo triothep_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &chars,       0, 16 },
	{ REGION_GFX2, 0, &sprites,   256, 16 },
	{ REGION_GFX3, 0, &tiles,     512, 16 },
	{ -1 } /* end of array */
};

/******************************************************************************/

static void sound_irq(int linestate)
{
	cpu_set_irq_line(1,0,linestate); /* IRQ */
}

static struct YM2203interface ym2203_interface =
{
	1,
	1500000, /* Should be accurate */
	{ YM2203_VOL(50,90) },
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 }
};

static struct YM3812interface ym3812_interface =
{
	1,			/* 1 chip */
	3000000,	/* 3.000000 MHz (Should be accurate) */
	{ 90 },
	{ sound_irq },
};

static struct OKIM6295interface okim6295_interface =
{
	1,              /* 1 chip */
	{ 7759 },       /* frequency */
	{ REGION_SOUND1 },
	{ 85 }
};

/******************************************************************************/

static MACHINE_DRIVER_START( actfancr )

	/* basic machine hardware */
	MDRV_CPU_ADD(H6280,21477200/3) /* Should be accurate */
	MDRV_CPU_MEMORY(actfan_readmem,actfan_writemem)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,1) /* VBL */

	MDRV_CPU_ADD(M6502, 1500000)
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU) /* Should be accurate */
	MDRV_CPU_MEMORY(dec0_s_readmem,dec0_s_writemem)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(529)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_BUFFERS_SPRITERAM)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 1*8, 31*8-1)
	MDRV_GFXDECODE(actfan_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(768)

	MDRV_VIDEO_START(actfancr)
	MDRV_VIDEO_UPDATE(actfancr)

	/* sound hardware */
	MDRV_SOUND_ADD(YM2203, ym2203_interface)
	MDRV_SOUND_ADD(YM3812, ym3812_interface)
	MDRV_SOUND_ADD(OKIM6295, okim6295_interface)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( triothep )

	/* basic machine hardware */
	MDRV_CPU_ADD(H6280,21477200/3) /* Should be accurate */
	MDRV_CPU_MEMORY(triothep_readmem,triothep_writemem)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,1) /* VBL */

	MDRV_CPU_ADD(M6502, 1500000)
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU) /* Should be accurate */
	MDRV_CPU_MEMORY(dec0_s_readmem,dec0_s_writemem)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(529)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_BUFFERS_SPRITERAM)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 1*8, 31*8-1)
	MDRV_GFXDECODE(triothep_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(768)

	MDRV_VIDEO_START(triothep)
	MDRV_VIDEO_UPDATE(triothep)

	/* sound hardware */
	MDRV_SOUND_ADD(YM2203, ym2203_interface)
	MDRV_SOUND_ADD(YM3812, ym3812_interface)
	MDRV_SOUND_ADD(OKIM6295, okim6295_interface)
MACHINE_DRIVER_END

/******************************************************************************/

ROM_START( actfancr )
	ROM_REGION( 0x200000, REGION_CPU1, 0 ) /* Need to allow full RAM allocation for now */
	ROM_LOAD( "fe08-2.bin", 0x00000, 0x10000, CRC(0d36fbfa) SHA1(cef5cfd053beac5ca2ac52421024c316bdbfba42) )
	ROM_LOAD( "fe09-2.bin", 0x10000, 0x10000, CRC(27ce2bb1) SHA1(52a423dfc2bba7b3330d1a10f4149ae6eeb9198c) )
	ROM_LOAD( "10",   0x20000, 0x10000, CRC(cabad137) SHA1(41ca833649671a29e9395968cde2be8137a9ff0a) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* 6502 Sound CPU */
	ROM_LOAD( "17-1", 0x08000, 0x8000, CRC(289ad106) SHA1(cf1b32ac41d3d92860fab04d82a08efe57b6ecf3) )

	ROM_REGION( 0x20000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "15", 0x00000, 0x10000, CRC(a1baf21e) SHA1(b85cf9180efae6c95cc0310064b52a78e591826a) ) /* Chars */
	ROM_LOAD( "16", 0x10000, 0x10000, CRC(22e64730) SHA1(f1376c6e2c9d021eca7ccee3daab00593ba724b6) )

	ROM_REGION( 0x60000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "02", 0x00000, 0x10000, CRC(b1db0efc) SHA1(a7bd7748ea37f473499ba5bf8ab4995b9240ff48) ) /* Sprites */
	ROM_LOAD( "03", 0x10000, 0x08000, CRC(f313e04f) SHA1(fe69758910d38f742971c1027fc8f498c88262b1) )
	ROM_LOAD( "06", 0x18000, 0x10000, CRC(8cb6dd87) SHA1(fab4fe76d2426c906a9070cbf7ce81200ba27ff6) )
	ROM_LOAD( "07", 0x28000, 0x08000, CRC(dd345def) SHA1(44fbf9da636a4e18c421fdc0a1eadc3c7ba66068) )
	ROM_LOAD( "00", 0x30000, 0x10000, CRC(d50a9550) SHA1(b366826e0df11ab6b97e2cb0e813432e95f9513d) )
	ROM_LOAD( "01", 0x40000, 0x08000, CRC(34935e93) SHA1(8cd02a72659f6cb0536b54c1c8b34dae818fbfdc) )
	ROM_LOAD( "04", 0x48000, 0x10000, CRC(bcf41795) SHA1(1d18afc974ac43fe6194e2840bbb2e93cd2b6cff) )
	ROM_LOAD( "05", 0x58000, 0x08000, CRC(d38b94aa) SHA1(773d01427744fda9104f673d2b4183a0f7471a39) )

	ROM_REGION( 0x40000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "14", 0x00000, 0x10000, CRC(d6457420) SHA1(d03d2e944e768b297ec0c3389320c42bc0259d00) ) /* Tiles */
	ROM_LOAD( "12", 0x10000, 0x10000, CRC(08787b7a) SHA1(23b10b75c4cbff8effadf4c6ed15d90b87648ce9) )
	ROM_LOAD( "13", 0x20000, 0x10000, CRC(c30c37dc) SHA1(0f7a325738eafa85239497e2b97aa51a6f2ffc4d) )
	ROM_LOAD( "11", 0x30000, 0x10000, CRC(1f006d9f) SHA1(74bc2d4d022ad7c65be781f974919262cacb4b64) )

	ROM_REGION( 0x10000, REGION_SOUND1, 0 ) /* ADPCM sounds */
	ROM_LOAD( "18",   0x00000, 0x10000, CRC(5c55b242) SHA1(62ba60b2f02483875da12aefe849f7e2fd137ef1) )
ROM_END

ROM_START( actfanc1 )
	ROM_REGION( 0x200000, REGION_CPU1, 0 ) /* Need to allow full RAM allocation for now */
	ROM_LOAD( "08-1", 0x00000, 0x10000, CRC(3bf214a4) SHA1(f7513672b2292d3acb4332b392695888bf6560a5) )
	ROM_LOAD( "09-1", 0x10000, 0x10000, CRC(13ae78d5) SHA1(eba77d3dbfe273e18c7fa9c0ca305ac2468f9381) )
	ROM_LOAD( "10",   0x20000, 0x10000, CRC(cabad137) SHA1(41ca833649671a29e9395968cde2be8137a9ff0a) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* 6502 Sound CPU */
	ROM_LOAD( "17-1", 0x08000, 0x8000, CRC(289ad106) SHA1(cf1b32ac41d3d92860fab04d82a08efe57b6ecf3) )

	ROM_REGION( 0x20000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "15", 0x00000, 0x10000, CRC(a1baf21e) SHA1(b85cf9180efae6c95cc0310064b52a78e591826a) ) /* Chars */
	ROM_LOAD( "16", 0x10000, 0x10000, CRC(22e64730) SHA1(f1376c6e2c9d021eca7ccee3daab00593ba724b6) )

	ROM_REGION( 0x60000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "02", 0x00000, 0x10000, CRC(b1db0efc) SHA1(a7bd7748ea37f473499ba5bf8ab4995b9240ff48) ) /* Sprites */
	ROM_LOAD( "03", 0x10000, 0x08000, CRC(f313e04f) SHA1(fe69758910d38f742971c1027fc8f498c88262b1) )
	ROM_LOAD( "06", 0x18000, 0x10000, CRC(8cb6dd87) SHA1(fab4fe76d2426c906a9070cbf7ce81200ba27ff6) )
	ROM_LOAD( "07", 0x28000, 0x08000, CRC(dd345def) SHA1(44fbf9da636a4e18c421fdc0a1eadc3c7ba66068) )
	ROM_LOAD( "00", 0x30000, 0x10000, CRC(d50a9550) SHA1(b366826e0df11ab6b97e2cb0e813432e95f9513d) )
	ROM_LOAD( "01", 0x40000, 0x08000, CRC(34935e93) SHA1(8cd02a72659f6cb0536b54c1c8b34dae818fbfdc) )
	ROM_LOAD( "04", 0x48000, 0x10000, CRC(bcf41795) SHA1(1d18afc974ac43fe6194e2840bbb2e93cd2b6cff) )
	ROM_LOAD( "05", 0x58000, 0x08000, CRC(d38b94aa) SHA1(773d01427744fda9104f673d2b4183a0f7471a39) )

	ROM_REGION( 0x40000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "14", 0x00000, 0x10000, CRC(d6457420) SHA1(d03d2e944e768b297ec0c3389320c42bc0259d00) ) /* Tiles */
	ROM_LOAD( "12", 0x10000, 0x10000, CRC(08787b7a) SHA1(23b10b75c4cbff8effadf4c6ed15d90b87648ce9) )
	ROM_LOAD( "13", 0x20000, 0x10000, CRC(c30c37dc) SHA1(0f7a325738eafa85239497e2b97aa51a6f2ffc4d) )
	ROM_LOAD( "11", 0x30000, 0x10000, CRC(1f006d9f) SHA1(74bc2d4d022ad7c65be781f974919262cacb4b64) )

	ROM_REGION( 0x10000, REGION_SOUND1, 0 ) /* ADPCM sounds */
	ROM_LOAD( "18",   0x00000, 0x10000, CRC(5c55b242) SHA1(62ba60b2f02483875da12aefe849f7e2fd137ef1) )
ROM_END

ROM_START( actfancj )
	ROM_REGION( 0x200000, REGION_CPU1, 0 ) /* Need to allow full RAM allocation for now */
	ROM_LOAD( "fd08-1.bin", 0x00000, 0x10000, CRC(69004b60) SHA1(7c6b876ca04377d2aa2d3c3f19d8e6cc7345363d) )
	ROM_LOAD( "fd09-1.bin", 0x10000, 0x10000, CRC(a455ae3e) SHA1(960798271c8370c1c4ffce2a453f59d7a301c9f9) )
	ROM_LOAD( "10",   0x20000, 0x10000, CRC(cabad137) SHA1(41ca833649671a29e9395968cde2be8137a9ff0a) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* 6502 Sound CPU */
	ROM_LOAD( "17-1", 0x08000, 0x8000, CRC(289ad106) SHA1(cf1b32ac41d3d92860fab04d82a08efe57b6ecf3) )

	ROM_REGION( 0x20000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "15", 0x00000, 0x10000, CRC(a1baf21e) SHA1(b85cf9180efae6c95cc0310064b52a78e591826a) ) /* Chars */
	ROM_LOAD( "16", 0x10000, 0x10000, CRC(22e64730) SHA1(f1376c6e2c9d021eca7ccee3daab00593ba724b6) )

	ROM_REGION( 0x60000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "02", 0x00000, 0x10000, CRC(b1db0efc) SHA1(a7bd7748ea37f473499ba5bf8ab4995b9240ff48) ) /* Sprites */
	ROM_LOAD( "03", 0x10000, 0x08000, CRC(f313e04f) SHA1(fe69758910d38f742971c1027fc8f498c88262b1) )
	ROM_LOAD( "06", 0x18000, 0x10000, CRC(8cb6dd87) SHA1(fab4fe76d2426c906a9070cbf7ce81200ba27ff6) )
	ROM_LOAD( "07", 0x28000, 0x08000, CRC(dd345def) SHA1(44fbf9da636a4e18c421fdc0a1eadc3c7ba66068) )
	ROM_LOAD( "00", 0x30000, 0x10000, CRC(d50a9550) SHA1(b366826e0df11ab6b97e2cb0e813432e95f9513d) )
	ROM_LOAD( "01", 0x40000, 0x08000, CRC(34935e93) SHA1(8cd02a72659f6cb0536b54c1c8b34dae818fbfdc) )
	ROM_LOAD( "04", 0x48000, 0x10000, CRC(bcf41795) SHA1(1d18afc974ac43fe6194e2840bbb2e93cd2b6cff) )
	ROM_LOAD( "05", 0x58000, 0x08000, CRC(d38b94aa) SHA1(773d01427744fda9104f673d2b4183a0f7471a39) )

	ROM_REGION( 0x40000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "14", 0x00000, 0x10000, CRC(d6457420) SHA1(d03d2e944e768b297ec0c3389320c42bc0259d00) ) /* Tiles */
	ROM_LOAD( "12", 0x10000, 0x10000, CRC(08787b7a) SHA1(23b10b75c4cbff8effadf4c6ed15d90b87648ce9) )
	ROM_LOAD( "13", 0x20000, 0x10000, CRC(c30c37dc) SHA1(0f7a325738eafa85239497e2b97aa51a6f2ffc4d) )
	ROM_LOAD( "11", 0x30000, 0x10000, CRC(1f006d9f) SHA1(74bc2d4d022ad7c65be781f974919262cacb4b64) )

	ROM_REGION( 0x10000, REGION_SOUND1, 0 ) /* ADPCM sounds */
	ROM_LOAD( "18",   0x00000, 0x10000, CRC(5c55b242) SHA1(62ba60b2f02483875da12aefe849f7e2fd137ef1) )
ROM_END

ROM_START( triothep )
	ROM_REGION( 0x200000, REGION_CPU1, 0 ) /* Need to allow full RAM allocation for now */
	ROM_LOAD( "ff16",     0x00000, 0x20000, CRC(84d7e1b6) SHA1(28381d2e1f6d22a959383eb2e8d73f2e03f4d39f) )
	ROM_LOAD( "ff15.bin", 0x20000, 0x10000, CRC(6eada47c) SHA1(98fc4e93c47bc42ea7c20e8ac994b117cd7cb5a5) )
	ROM_LOAD( "ff14.bin", 0x30000, 0x10000, CRC(4ba7de4a) SHA1(bf552fa33746f3d27f9b193424a38fef58fe0765) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* 6502 Sound CPU */
	ROM_LOAD( "ff18.bin", 0x00000, 0x10000, CRC(9de9ee63) SHA1(c91b824b9a791cb90365d45c8e1b69e67f7d065f) )

	ROM_REGION( 0x20000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "ff12.bin", 0x00000, 0x10000, CRC(15fb49f2) SHA1(a81ff1dbc813ab9b37edb832e01aab9a9a3ed5a1) ) /* Chars */
	ROM_LOAD( "ff13.bin", 0x10000, 0x10000, CRC(e20c9623) SHA1(b5a58599a016378f34217396212f81ede9272598) )

	ROM_REGION( 0x60000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "ff11.bin", 0x00000, 0x10000, CRC(19e885c7) SHA1(694f0aa4c1c976320d985ee50bb59c1894b853ed) ) /* Sprites */
	ROM_LOAD( "ff10.bin", 0x10000, 0x08000, CRC(4b6b477a) SHA1(77486e0ff957cbfdae16d2b5977e95b7a7ced948) )
	ROM_LOAD( "ff09.bin", 0x18000, 0x10000, CRC(79c6bc0e) SHA1(d4bf195f6114103d2eb68f3aaf65d4044947f600) )
	ROM_LOAD( "ff08.bin", 0x28000, 0x08000, CRC(1391e445) SHA1(bd53a969567bb5a46a35bd02e84bbb58c446a0a2) )
	ROM_LOAD( "ff03.bin", 0x30000, 0x10000, CRC(b36ad42d) SHA1(9d72cbb0904e82271e4835d668b133f17dec8255) )
	ROM_LOAD( "ff02.bin", 0x40000, 0x08000, CRC(6b9d24ce) SHA1(9d6d52e742fc37d83682291f918f3348395f0cd8) )
	ROM_LOAD( "ff01.bin", 0x48000, 0x10000, CRC(68d80a66) SHA1(526ed8c920915877f5ee0519c9c8eee7e5580c54) )
	ROM_LOAD( "ff00.bin", 0x58000, 0x08000, CRC(41232442) SHA1(1c10a4f5607e41d6239cb478ed7355963ad6b2d0) )

	ROM_REGION( 0x40000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "ff04.bin", 0x00000, 0x10000, CRC(7cea3c87) SHA1(b58156140a75f88ee6ec97ca7cdc02619ec51726) ) /* Tiles */
	ROM_LOAD( "ff06.bin", 0x10000, 0x10000, CRC(5e7f3e8f) SHA1(c92ec281b3985b442957f7d9237eb38a6d621cd4) )
	ROM_LOAD( "ff05.bin", 0x20000, 0x10000, CRC(8bb13f05) SHA1(f524cb0a38d0025c93124fc329d913e000155e9b) )
	ROM_LOAD( "ff07.bin", 0x30000, 0x10000, CRC(0d7affc3) SHA1(59f9fbf13216aaf67c7d1ad3a11a1738c4afd9e5) )

	ROM_REGION( 0x10000, REGION_SOUND1, 0 ) /* ADPCM sounds */
	ROM_LOAD( "ff17.bin", 0x00000, 0x10000, CRC(f0ab0d05) SHA1(29d3ab513a8d46a1cb70f5333fa56bb787a58288) )
ROM_END

/******************************************************************************/

static READ_HANDLER( cycle_r )
{
	int pc=activecpu_get_pc();
	int ret=actfancr_ram[0x26];

	if (offset==1) return actfancr_ram[0x27];

	if (pc==0xe29a && ret==0) {
		cpu_spinuntil_int();
		return 1;
	}

	return ret;
}

static READ_HANDLER( cyclej_r )
{
	int pc=activecpu_get_pc();
	int ret=actfancr_ram[0x26];

	if (offset==1) return actfancr_ram[0x27];

	if (pc==0xe2b1 && ret==0) {
		cpu_spinuntil_int();
		return 1;
	}

	return ret;
}

static DRIVER_INIT( actfancr )
{
	install_mem_read_handler(0, 0x1f0026, 0x1f0027, cycle_r);
}

static DRIVER_INIT( actfancj )
{
	install_mem_read_handler(0, 0x1f0026, 0x1f0027, cyclej_r);
}



GAME( 1989, actfancr, 0,        actfancr, actfancr, actfancr, ROT0, "Data East Corporation", "Act-Fancer Cybernetick Hyper Weapon (World revision 2)" )
GAME( 1989, actfanc1, actfancr, actfancr, actfancr, actfancr, ROT0, "Data East Corporation", "Act-Fancer Cybernetick Hyper Weapon (World revision 1)" )
GAME( 1989, actfancj, actfancr, actfancr, actfancr, actfancj, ROT0, "Data East Corporation", "Act-Fancer Cybernetick Hyper Weapon (Japan revision 1)" )
GAME( 1989, triothep, 0,        triothep, triothep, 0,        ROT0, "Data East Corporation", "Trio The Punch - Never Forget Me... (Japan)" )
