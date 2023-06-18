/***************************************************************************

Deniam games

driver by Nicola Salmoria


http://deniam.co.kr/text/c_game01.htm

Title            System     Date
---------------- ---------- ----------
GO!GO!           deniam-16b 1995/10/11
Logic Pro        deniam-16b 1996/10/20
Karian Cross     deniam-16b 1997/04/17
LOTTERY GAME     deniam-16c 1997/05/21
Logic Pro 2      deniam-16c 1997/06/20
Propose          deniam-16c 1997/06/21

They call the hardware "deniam-16", but it's actually pretty much identical to
Sega System 16.


Notes:

- The logicpr2 OKIM6295 ROM has four banks, but the game seems to only use 0 and 1.
- logicpro dip switches might be wrong (using the logicpr2 ones)
- flip screen is not supported but these games don't use it (no flip screen dip
  and no cocktail mode)
- if it's like System 16, the top bit of palette ram should be an additional bit
  for Green. But is it ever not 0?

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "cpu/z80/z80.h"


extern data16_t *deniam_videoram,*deniam_textram;

DRIVER_INIT( logicpro );
DRIVER_INIT( karianx );
WRITE16_HANDLER( deniam_videoram_w );
WRITE16_HANDLER( deniam_textram_w );
WRITE16_HANDLER( deniam_palette_w );
READ16_HANDLER( deniam_coinctrl_r );
WRITE16_HANDLER( deniam_coinctrl_w );
VIDEO_START( deniam );
VIDEO_UPDATE( deniam );




static WRITE16_HANDLER( sound_command_w )
{
	if (ACCESSING_MSB)
	{
		soundlatch_w(offset,(data >> 8) & 0xff);
		cpu_set_irq_line(1,IRQ_LINE_NMI,PULSE_LINE);
	}
}

static WRITE_HANDLER( deniam16b_oki_rom_bank_w )
{
	OKIM6295_set_bank_base(0,(data & 0x40) ? 0x40000 : 0x00000);
}

static WRITE16_HANDLER( deniam16c_oki_rom_bank_w )
{
	if (ACCESSING_LSB)
		OKIM6295_set_bank_base(0,(data & 0x01) ? 0x40000 : 0x00000);
}

static MACHINE_INIT( deniam )
{
	/* logicpr2 does not reset the bank base on startup */
	OKIM6295_set_bank_base(0,0x00000);
}

static WRITE16_HANDLER( YM3812_control_port_0_msb_w )
{
	if (ACCESSING_MSB)
		YM3812_control_port_0_w(0,(data >> 8) & 0xff);
}

static WRITE16_HANDLER( YM3812_write_port_0_msb_w )
{
	if (ACCESSING_MSB)
		YM3812_write_port_0_w(0,(data >> 8) & 0xff);
}



static MEMORY_READ16_START( deniam16b_readmem )
	{ 0x000000, 0x0fffff, MRA16_ROM },
	{ 0x400000, 0x40ffff, MRA16_RAM },
	{ 0x410000, 0x410fff, MRA16_RAM },
	{ 0xc40002, 0xc40003, deniam_coinctrl_r },
	{ 0xc44000, 0xc44001, input_port_0_word_r },
	{ 0xc44002, 0xc44003, input_port_1_word_r },
	{ 0xc44004, 0xc44005, input_port_2_word_r },
	{ 0xc44006, 0xc44007, MRA16_NOP },	/* unused? */
	{ 0xc4400a, 0xc4400b, input_port_3_word_r },
	{ 0xff0000, 0xffffff, MRA16_RAM },
MEMORY_END

static MEMORY_WRITE16_START( deniam16b_writemem )
	{ 0x000000, 0x0fffff, MWA16_ROM },
	{ 0x400000, 0x40ffff, deniam_videoram_w, &deniam_videoram },
	{ 0x410000, 0x410fff, deniam_textram_w, &deniam_textram },
	{ 0x440000, 0x4407ff, MWA16_RAM, &spriteram16, &spriteram_size },
	{ 0x840000, 0x840fff, deniam_palette_w, &paletteram16 },
	{ 0xc40000, 0xc40001, sound_command_w },
	{ 0xc40002, 0xc40003, deniam_coinctrl_w },
	{ 0xc40004, 0xc40005, MWA16_NOP },	/* irq ack? */
	{ 0xff0000, 0xffffff, MWA16_RAM },
MEMORY_END

static MEMORY_READ_START( sound_readmem )
	{ 0x0000, 0xf7ff, MRA_ROM },
	{ 0xf800, 0xffff, MRA_RAM },
MEMORY_END

static MEMORY_WRITE_START( sound_writemem )
	{ 0x0000, 0xf7ff, MWA_ROM },
	{ 0xf800, 0xffff, MWA_RAM },
MEMORY_END

static PORT_READ_START( sound_readport )
	{ 0x01, 0x01, soundlatch_r },
	{ 0x05, 0x05, OKIM6295_status_0_r },
PORT_END

static PORT_WRITE_START( sound_writeport )
	{ 0x02, 0x02, YM3812_control_port_0_w },
	{ 0x03, 0x03, YM3812_write_port_0_w },
	{ 0x05, 0x05, OKIM6295_data_0_w },
	{ 0x07, 0x07, deniam16b_oki_rom_bank_w },
PORT_END


/* identical to 16b, but handles sound directly */
static MEMORY_READ16_START( deniam16c_readmem )
	{ 0x000000, 0x0fffff, MRA16_ROM },
	{ 0x400000, 0x40ffff, MRA16_RAM },
	{ 0x410000, 0x410fff, MRA16_RAM },
	{ 0xc40000, 0xc40001, OKIM6295_status_0_lsb_r },
	{ 0xc40002, 0xc40003, deniam_coinctrl_r },
	{ 0xc44000, 0xc44001, input_port_0_word_r },
	{ 0xc44002, 0xc44003, input_port_1_word_r },
	{ 0xc44004, 0xc44005, input_port_2_word_r },
	{ 0xc44006, 0xc44007, MRA16_NOP },	/* unused? */
	{ 0xc4400a, 0xc4400b, input_port_3_word_r },
	{ 0xff0000, 0xffffff, MRA16_RAM },
MEMORY_END

static MEMORY_WRITE16_START( deniam16c_writemem )
	{ 0x000000, 0x0fffff, MWA16_ROM },
	{ 0x400000, 0x40ffff, deniam_videoram_w, &deniam_videoram },
	{ 0x410000, 0x410fff, deniam_textram_w, &deniam_textram },
	{ 0x440000, 0x4407ff, MWA16_RAM, &spriteram16, &spriteram_size },
	{ 0x840000, 0x840fff, deniam_palette_w, &paletteram16 },
	{ 0xc40000, 0xc40001, OKIM6295_data_0_lsb_w },
	{ 0xc40002, 0xc40003, deniam_coinctrl_w },
	{ 0xc40004, 0xc40005, MWA16_NOP },	/* irq ack? */
	{ 0xc40006, 0xc40007, deniam16c_oki_rom_bank_w },
	{ 0xc40008, 0xc40009, YM3812_control_port_0_msb_w },
	{ 0xc4000a, 0xc4000b, YM3812_write_port_0_msb_w },
	{ 0xff0000, 0xffffff, MWA16_RAM },
MEMORY_END



INPUT_PORTS_START( karianx )
	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_SERVICE, DEF_STR( Service_Mode ), KEYCODE_F2, IP_JOY_NONE )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_4WAY | IPF_PLAYER1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_4WAY | IPF_PLAYER1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY | IPF_PLAYER1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_4WAY | IPF_PLAYER1 )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_4WAY | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_4WAY | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY | IPF_PLAYER2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_4WAY | IPF_PLAYER2 )

	PORT_START
	PORT_DIPNAME( 0x07, 0x07, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 3C_2C ) )
	PORT_DIPSETTING(    0x07, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x05, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0x18, 0x18, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x08, "Very Easy" )
	PORT_DIPSETTING(    0x10, "Easy" )
	PORT_DIPSETTING(    0x18, "Normal" )
	PORT_DIPSETTING(    0x00, "Hard" )
	PORT_DIPNAME( 0x20, 0x20, "Demo Music" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x20, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Service_Mode ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
INPUT_PORTS_END

INPUT_PORTS_START( logicpr2 )
	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_SERVICE, DEF_STR( Service_Mode ), KEYCODE_F2, IP_JOY_NONE )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_4WAY | IPF_PLAYER1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_4WAY | IPF_PLAYER1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY | IPF_PLAYER1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_4WAY | IPF_PLAYER1 )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_4WAY | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_4WAY | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY | IPF_PLAYER2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_4WAY | IPF_PLAYER2 )

	PORT_START
	PORT_DIPNAME( 0x07, 0x07, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 3C_2C ) )
	PORT_DIPSETTING(    0x07, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x05, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0x18, 0x18, "Play Time" )
	PORT_DIPSETTING(    0x08, "Slow" )
	PORT_DIPSETTING(    0x18, "Normal" )
	PORT_DIPSETTING(    0x10, "Fast" )
	PORT_DIPSETTING(    0x00, "Fastest" )
	PORT_DIPNAME( 0x60, 0x60, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x20, "Easy" )
	PORT_DIPSETTING(    0x60, "Normal" )
	PORT_DIPSETTING(    0x40, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x80, DEF_STR( On ) )
INPUT_PORTS_END



static struct GfxLayout charlayout =
{
	8,8,
	RGN_FRAC(1,3),
	3,
	{ RGN_FRAC(2,3), RGN_FRAC(1,3), RGN_FRAC(0,3) },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8
};


static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &charlayout, 0, 128 },	/* colors 0-1023 */
												/* sprites use colors 1024-2047 */
	{ -1 } /* end of array */
};


static void irqhandler(int linestate)
{
	/* system 16c doesn't have the sound CPU */
	if (Machine->drv->cpu[1].cpu_type)
		cpu_set_irq_line(1,0,linestate);
}

static struct YM3812interface ym3812_interface =
{
	1,			/* 1 chip */
	25000000/8,	/* ??? */
	{ 60 },	/* volume */
	{ irqhandler },
};

static struct OKIM6295interface okim6295_interface =
{
	1,                  /* 1 chip */
	{ 8000 },           /* 8000Hz frequency */
	{ REGION_SOUND1 },	/* memory region */
	{ 100 }				/* volume */
};



static MACHINE_DRIVER_START( deniam16b )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000,25000000/2)	/* ??? */
	MDRV_CPU_MEMORY(deniam16b_readmem,deniam16b_writemem)
	MDRV_CPU_VBLANK_INT(irq4_line_hold,1)

	MDRV_CPU_ADD(Z80,25000000/4)	/* (makes logicpro music tempo correct) */
	MDRV_CPU_MEMORY(sound_readmem,sound_writemem)
	MDRV_CPU_PORTS(sound_readport,sound_writeport)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_MACHINE_INIT(deniam)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(512, 256)
	MDRV_VISIBLE_AREA(24*8, 64*8-1, 0*8, 28*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(2048)

	MDRV_VIDEO_START(deniam)
	MDRV_VIDEO_UPDATE(deniam)

	/* sound hardware */
	MDRV_SOUND_ADD(YM3812, ym3812_interface)
	MDRV_SOUND_ADD(OKIM6295, okim6295_interface)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( deniam16c )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000,25000000/2)	/* ??? */
	MDRV_CPU_MEMORY(deniam16c_readmem,deniam16c_writemem)
	MDRV_CPU_VBLANK_INT(irq4_line_hold,1)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_MACHINE_INIT(deniam)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(512, 256)
	MDRV_VISIBLE_AREA(24*8, 64*8-1, 0*8, 28*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(2048)

	MDRV_VIDEO_START(deniam)
	MDRV_VIDEO_UPDATE(deniam)

	/* sound hardware */
	MDRV_SOUND_ADD(YM3812, ym3812_interface)
	MDRV_SOUND_ADD(OKIM6295, okim6295_interface)
MACHINE_DRIVER_END



/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( logicpro )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_BYTE( "logicpro.r4", 0x00000, 0x40000, CRC(c506d484) SHA1(5d662b109e1d2e09556bc4ecbc11bbf5ccb639d3) )
	ROM_LOAD16_BYTE( "logicpro.r3", 0x00001, 0x40000, CRC(d5a4cf62) SHA1(138ea4f1629e453c1a00410eda7086d3633240e3) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* sound */
	ROM_LOAD( "logicpro.r2", 0x0000, 0x10000, CRC(000d624b) SHA1(c0da218ee81d01b3dcef2159bbaaff5d3ddb7619) )

	ROM_REGION( 0x180000, REGION_GFX1, ROMREGION_DISPOSE )	/* chars */
	ROM_LOAD( "logicpro.r5", 0x000000, 0x080000, CRC(dedf18c9) SHA1(9725e096427f03ed5fd81584c0aa85a53f9681c9) )
	ROM_LOAD( "logicpro.r6", 0x080000, 0x080000, CRC(3ecbd1c2) SHA1(dd6afacd58eaaa2562e007a92b6667ecc968377d) )
	ROM_LOAD( "logicpro.r7", 0x100000, 0x080000, CRC(47135521) SHA1(ee6a93332190fc966f8e820430d652942f030b00) )

	ROM_REGION( 0x400000, REGION_GFX2, 0 )	/* sprites, used at run time */
	ROM_LOAD16_BYTE( "logicpro.r9", 0x000000, 0x080000, CRC(a98bc1d2) SHA1(f4aed07cccca892f3d3a91546b3a98fbe3e66d9c) )
	ROM_LOAD16_BYTE( "logicpro.r8", 0x000001, 0x080000, CRC(1de46298) SHA1(3385a2956d9a427c85554f39c8d85922bbeb1ce1) )

	ROM_REGION( 0x100000, REGION_SOUND1, 0 )	/* OKIM6295 samples */
	ROM_LOAD( "logicpro.r1", 0x0000, 0x080000, CRC(a1fec4d4) SHA1(4390cd18b4a7de2d8cb68270180ea3de42fd2282) )
ROM_END

ROM_START( croquis )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_BYTE( "r4.bin", 0x00000, 0x40000, CRC(03c9055e) SHA1(b1fa8e7a272887decca30eefe73ac782f296f0dd) )
	ROM_LOAD16_BYTE( "r3.bin", 0x00001, 0x40000, CRC(a98ae4f6) SHA1(80fcedb4ee0f35eb2d0b4a248c15f872af2e08f2) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* sound */
	ROM_LOAD( "logicpro.r2", 0x0000, 0x10000, CRC(000d624b) SHA1(c0da218ee81d01b3dcef2159bbaaff5d3ddb7619) )

	ROM_REGION( 0x180000, REGION_GFX1, ROMREGION_DISPOSE )	/* chars */
	ROM_LOAD( "logicpro.r5", 0x000000, 0x080000, CRC(dedf18c9) SHA1(9725e096427f03ed5fd81584c0aa85a53f9681c9) )
	ROM_LOAD( "logicpro.r6", 0x080000, 0x080000, CRC(3ecbd1c2) SHA1(dd6afacd58eaaa2562e007a92b6667ecc968377d) )
	ROM_LOAD( "logicpro.r7", 0x100000, 0x080000, CRC(47135521) SHA1(ee6a93332190fc966f8e820430d652942f030b00) )

	ROM_REGION( 0x400000, REGION_GFX2, 0 )	/* sprites, used at run time */
	ROM_LOAD16_BYTE( "logicpro.r9", 0x000000, 0x080000, CRC(a98bc1d2) SHA1(f4aed07cccca892f3d3a91546b3a98fbe3e66d9c) )
	ROM_LOAD16_BYTE( "logicpro.r8", 0x000001, 0x080000, CRC(1de46298) SHA1(3385a2956d9a427c85554f39c8d85922bbeb1ce1) )

	ROM_REGION( 0x100000, REGION_SOUND1, 0 )	/* OKIM6295 samples */
	ROM_LOAD( "logicpro.r1", 0x0000, 0x080000, CRC(a1fec4d4) SHA1(4390cd18b4a7de2d8cb68270180ea3de42fd2282) )
ROM_END

ROM_START( karianx )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_BYTE( "even",        0x00000, 0x80000, CRC(fd0ce238) SHA1(4b727366c942c62187d8700666b42a85c059c060) )
	ROM_LOAD16_BYTE( "odd",         0x00001, 0x80000, CRC(be173cdc) SHA1(13230b6129fd1910257624a69a3a4b74696e982e) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* sound */
	ROM_LOAD( "snd",         0x0000, 0x10000, CRC(fedd3375) SHA1(09fb2d5fc91704120f757acf9fa00d149f891a28) )

	ROM_REGION( 0x180000, REGION_GFX1, ROMREGION_DISPOSE )	/* chars */
	ROM_LOAD( "bkg1",        0x000000, 0x080000, CRC(5cb8558a) SHA1(9c6024c70a0f0cd529a0e2e853e467ec8d8ab446) )
	ROM_LOAD( "bkg2",        0x080000, 0x080000, CRC(95ff297c) SHA1(28f6c005e73e1680bd8be7ce355fa0d404827105) )
	ROM_LOAD( "bkg3",        0x100000, 0x080000, CRC(6c81f1b2) SHA1(14ef907a9c381b7ef45441d480bb4ccb015e474b) )

	ROM_REGION( 0x400000, REGION_GFX2, 0 )	/* sprites, used at run time */
	ROM_LOAD16_BYTE( "obj4",        0x000000, 0x080000, CRC(5f8d75a9) SHA1(0552d046742aeb2fee176887156e73480c75a1bd) )
	ROM_LOAD16_BYTE( "obj1",        0x000001, 0x080000, CRC(967ee97d) SHA1(689f2da67eab86653b846fada39139792cd4aee2) )
	ROM_LOAD16_BYTE( "obj5",        0x100000, 0x080000, CRC(e9fc22f9) SHA1(a1f7f779520346406949500e3224c0c42cbbe026) )
	ROM_LOAD16_BYTE( "obj2",        0x100001, 0x080000, CRC(d39eb04e) SHA1(c59c3e14a506cb04d09cc7eec9962daa242a0b0c) )
	ROM_LOAD16_BYTE( "obj6",        0x200000, 0x080000, CRC(c1ec35a5) SHA1(bf59f4c3de081c8cc398c825fc1f3e8577641f10) )
	ROM_LOAD16_BYTE( "obj3",        0x200001, 0x080000, CRC(6ac1ac87) SHA1(1954e25ac5489a8eca137b86c89c415f1fed360c) )

	ROM_REGION( 0x100000, REGION_SOUND1, 0 )	/* OKIM6295 samples */
	ROM_LOAD( "voi",         0x0000, 0x080000, CRC(c6506a80) SHA1(121229c501bd5678e55c7342619743c773a01a7e) )
ROM_END

ROM_START( logicpr2 )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_BYTE( "lp2-2",       0x00000, 0x80000, CRC(cc1880bf) SHA1(5ea542b63947a570aaf924f7ab739e060e359af8) )
	ROM_LOAD16_BYTE( "lp2-1",       0x00001, 0x80000, CRC(46d5e954) SHA1(7bf5ae19caeecd2123754698276bbc78d68984d9) )

	ROM_REGION( 0x180000, REGION_GFX1, ROMREGION_DISPOSE )	/* chars */
	ROM_LOAD( "log2-b01",    0x000000, 0x080000, CRC(fe789e07) SHA1(c3d542564519fd807bc605029f5a2cca571eec9f) )
	ROM_LOAD( "log2-b02",    0x080000, 0x080000, CRC(1e0c51cd) SHA1(c25b3259a173e77785dcee1407ddf191c3efad79) )
	ROM_LOAD( "log2-b03",    0x100000, 0x080000, CRC(916f2928) SHA1(8c73408664dcd3de42cb27fac0d22b87b540bf52) )

	ROM_REGION( 0x400000, REGION_GFX2, 0 )	/* sprites, used at run time */
	ROM_LOAD16_WORD_SWAP( "obj",         0x000000, 0x400000, CRC(f221f305) SHA1(aa1d3d86d13e009bfb44cbc6ff4401b811b19f97) )

	ROM_REGION( 0x100000, REGION_SOUND1, 0 )	/* OKIM6295 samples */
	ROM_LOAD( "log2-s01",    0x0000, 0x100000, CRC(2875c435) SHA1(633538d9ac53228ea344605482ac387852c29193) )
ROM_END



GAME( 1996, logicpro, 0,        deniam16b, logicpr2, logicpro, ROT0, "Deniam", "Logic Pro (Japan)" )
GAME( 1996, croquis,  logicpro, deniam16b, logicpr2, logicpro, ROT0, "Deniam", "Croquis (Germany)" )
GAME( 1996, karianx,  0,        deniam16b, karianx,  karianx,  ROT0, "Deniam", "Karian Cross (Rev. 1.0)" )
GAMEX(1997, logicpr2, 0,        deniam16c, logicpr2, logicpro, ROT0, "Deniam", "Logic Pro 2 (Japan)", GAME_IMPERFECT_SOUND )
