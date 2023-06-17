/***************************************************************************

Based on drivers from Juno First emulator by Chris Hardy (chrish@kcbbs.gen.nz)

To enter service mode, keep 1&2 pressed on reset

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "cpu/m6809/m6809.h"


void konami1_decode(void);

extern unsigned char *circusc_spritebank;
extern unsigned char *circusc_scroll;
extern unsigned char *circusc_videoram,*circusc_colorram;

WRITE_HANDLER( circusc_videoram_w );
WRITE_HANDLER( circusc_colorram_w );

VIDEO_START( circusc );
WRITE_HANDLER( circusc_flipscreen_w );
PALETTE_INIT( circusc );
WRITE_HANDLER( circusc_sprite_bank_select_w );
VIDEO_UPDATE( circusc );



static READ_HANDLER( circusc_sh_timer_r )
{
	int clock;
#define CIRCUSCHALIE_TIMER_RATE (14318180/6144)

	clock = (activecpu_gettotalcycles()*4) / CIRCUSCHALIE_TIMER_RATE;

	return clock & 0xF;
}

static WRITE_HANDLER( circusc_sh_irqtrigger_w )
{
	cpu_set_irq_line_and_vector(1,0,HOLD_LINE,0xff);
}

static WRITE_HANDLER( circusc_dac_w )
{
	DAC_data_w(0,data);
}

static WRITE_HANDLER( circusc_coin_counter_w )
{
	coin_counter_w(offset,data);
}



static MEMORY_READ_START( readmem )
	{ 0x1000, 0x1000, input_port_0_r }, /* IO Coin */
	{ 0x1001, 0x1001, input_port_1_r }, /* P1 IO */
	{ 0x1002, 0x1002, input_port_2_r }, /* P2 IO */
	{ 0x1400, 0x1400, input_port_3_r }, /* DIP 1 */
	{ 0x1800, 0x1800, input_port_4_r }, /* DIP 2 */
	{ 0x2000, 0x39ff, MRA_RAM },
	{ 0x6000, 0xffff, MRA_ROM },
MEMORY_END

static MEMORY_WRITE_START( writemem )
	{ 0x0000, 0x0000, circusc_flipscreen_w },
	{ 0x0001, 0x0001, interrupt_enable_w },
	{ 0x0003, 0x0004, circusc_coin_counter_w },  /* Coin counters */
	{ 0x0005, 0x0005, MWA_RAM, &circusc_spritebank },
	{ 0x0400, 0x0400, watchdog_reset_w },
	{ 0x0800, 0x0800, soundlatch_w },
	{ 0x0c00, 0x0c00, circusc_sh_irqtrigger_w },  /* cause interrupt on audio CPU */
	{ 0x1c00, 0x1c00, MWA_RAM, &circusc_scroll },
	{ 0x2000, 0x2fff, MWA_RAM },
	{ 0x3000, 0x33ff, circusc_colorram_w, &circusc_colorram },
	{ 0x3400, 0x37ff, circusc_videoram_w, &circusc_videoram },
	{ 0x3800, 0x38ff, MWA_RAM, &spriteram_2 },
	{ 0x3900, 0x39ff, MWA_RAM, &spriteram, &spriteram_size },
	{ 0x3a00, 0x3fff, MWA_RAM },
	{ 0x6000, 0xffff, MWA_ROM },
MEMORY_END

static MEMORY_READ_START( sound_readmem )
	{ 0x0000, 0x3fff, MRA_ROM },
	{ 0x4000, 0x43ff, MRA_RAM },
	{ 0x6000, 0x6000, soundlatch_r },
	{ 0x8000, 0x8000, circusc_sh_timer_r },
MEMORY_END

static MEMORY_WRITE_START( sound_writemem )
	{ 0x0000, 0x3fff, MWA_ROM },
	{ 0x4000, 0x43ff, MWA_RAM },
	{ 0xa000, 0xa000, MWA_NOP },    /* latch command for the 76496. We should buffer this */
									/* command and send it to the chip, but we just use */
									/* the triggers below because the program always writes */
									/* the same number here and there. */
	{ 0xa001, 0xa001, SN76496_0_w },        /* trigger the 76496 to read the latch */
	{ 0xa002, 0xa002, SN76496_1_w },        /* trigger the 76496 to read the latch */
	{ 0xa003, 0xa003, circusc_dac_w },
	{ 0xa004, 0xa004, MWA_NOP },            /* ??? */
	{ 0xa07c, 0xa07c, MWA_NOP },            /* ??? */
MEMORY_END



INPUT_PORTS_START( circusc )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_2WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_2WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_2WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_2WAY | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_START      /* DSW0 */

	PORT_DIPNAME( 0x0f, 0x0f, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x05, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 3C_2C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 4C_3C ) )
	PORT_DIPSETTING(    0x0f, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 3C_4C ) )
	PORT_DIPSETTING(    0x07, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x0e, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 2C_5C ) )
	PORT_DIPSETTING(    0x0d, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x0b, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0x0a, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(    0x09, DEF_STR( 1C_7C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0xf0, 0xf0, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x50, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 3C_2C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 4C_3C ) )
	PORT_DIPSETTING(    0xf0, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 3C_4C ) )
	PORT_DIPSETTING(    0x70, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0xe0, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x60, DEF_STR( 2C_5C ) )
	PORT_DIPSETTING(    0xd0, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0xb0, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0xa0, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(    0x90, DEF_STR( 1C_7C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Free_Play ) )

	PORT_START      /* DSW1 */
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x03, "3" )
	PORT_DIPSETTING(    0x02, "4" )
	PORT_DIPSETTING(    0x01, "5" )
	PORT_DIPSETTING(    0x00, "7" )
	PORT_DIPNAME( 0x04, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x08, "20000 70000" )
	PORT_DIPSETTING(    0x00, "30000 80000" )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x60, 0x40, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x60, "Easy" )
	PORT_DIPSETTING(    0x40, "Normal" )
	PORT_DIPSETTING(    0x20, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
INPUT_PORTS_END



static struct GfxLayout charlayout =
{
	8,8,    /* 8*8 characters */
	512,    /* 512 characters */
	4,      /* 4 bits per pixel */
	{ 0, 1, 2, 3 }, /* the four bitplanes are packed in one nibble */
	{ 0*4, 1*4, 2*4, 3*4, 4*4, 5*4, 6*4, 7*4 },
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32 },
	32*8    /* every char takes 8 consecutive bytes */
};

static struct GfxLayout spritelayout =
{
	16,16,  /* 16*16 sprites */
	384,    /* 384 sprites */
	4,      /* 4 bits per pixel */
	{ 0, 1, 2, 3 },        /* the bitplanes are packed */
	{ 0*4, 1*4, 2*4, 3*4, 4*4, 5*4, 6*4, 7*4,
			8*4, 9*4, 10*4, 11*4, 12*4, 13*4, 14*4, 15*4 },
	{ 0*4*16, 1*4*16, 2*4*16, 3*4*16, 4*4*16, 5*4*16, 6*4*16, 7*4*16,
			8*4*16, 9*4*16, 10*4*16, 11*4*16, 12*4*16, 13*4*16, 14*4*16, 15*4*16 },
	32*4*8    /* every sprite takes 128 consecutive bytes */
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &charlayout,       0, 16 },
	{ REGION_GFX2, 0, &spritelayout, 16*16, 16 },
	{ -1 } /* end of array */
};



static struct SN76496interface sn76496_interface =
{
	2,      /* 2 chips */
	{ 14318180/8, 14318180/8 },     /*  1.7897725 MHz */
	{ 100, 100 }
};

static struct DACinterface dac_interface =
{
	1,
	{ 100 }
};



static MACHINE_DRIVER_START( circusc )

	/* basic machine hardware */
	MDRV_CPU_ADD(M6809, 2048000)        /* 2 MHz */
	MDRV_CPU_MEMORY(readmem,writemem)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,1)

	MDRV_CPU_ADD(Z80,14318180/4)
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)     /* Z80 Clock is derived from a 14.31818 MHz crystal */
	MDRV_CPU_MEMORY(sound_readmem,sound_writemem)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 2*8, 30*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(32)
	MDRV_COLORTABLE_LENGTH(16*16+16*16)

	MDRV_PALETTE_INIT(circusc)
	MDRV_VIDEO_START(circusc)
	MDRV_VIDEO_UPDATE(circusc)

	/* sound hardware */
	MDRV_SOUND_ADD(SN76496, sn76496_interface)
	MDRV_SOUND_ADD(DAC, dac_interface)
MACHINE_DRIVER_END



/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( circusc )
	ROM_REGION( 2*0x10000, REGION_CPU1, 0 )     /* 64k for code + 64k for decrypted opcodes */
	ROM_LOAD( "s05",          0x6000, 0x2000, CRC(48feafcf) SHA1(0e5bd350fa5fee42569eb0c4accf7512d645b792) )
	ROM_LOAD( "q04",          0x8000, 0x2000, CRC(c283b887) SHA1(458c398911453d558003f49c298b0d593c941c11) )
	ROM_LOAD( "q03",          0xa000, 0x2000, CRC(e90c0e86) SHA1(03211f0cc90b6e356989c5e2a41b70f4ff2ead83) )
	ROM_LOAD( "q02",          0xc000, 0x2000, CRC(4d847dc6) SHA1(a1f65e73c4e5abff1b0970bad32a128173245561) )
	ROM_LOAD( "q01",          0xe000, 0x2000, CRC(18c20adf) SHA1(2f40e1a109d129bb127a8b98e27817988cd08c8b) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )     /* 64k for the audio CPU */
	ROM_LOAD( "cd05_l14.bin", 0x0000, 0x2000, CRC(607df0fb) SHA1(67103d61994fd3a1e2de7cf9487e4f763234b18e) )
	ROM_LOAD( "cd07_l15.bin", 0x2000, 0x2000, CRC(a6ad30e1) SHA1(14f305717edcc2471e763b262960a0b96eef3530) )

	ROM_REGION( 0x04000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "a04_j12.bin",  0x0000, 0x2000, CRC(56e5b408) SHA1(73b9e3d46dfe9e39b390c634df153648a0906876) )
	ROM_LOAD( "a05_k13.bin",  0x2000, 0x2000, CRC(5aca0193) SHA1(4d0b0a773c385b7f1dcf024760d0437f47e78fbe) )

	ROM_REGION( 0x0c000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "e11_j06.bin",  0x0000, 0x2000, CRC(df0405c6) SHA1(70a50dcc86dfbdaa9c2af613105aae7f90747804) )
	ROM_LOAD( "e12_j07.bin",  0x2000, 0x2000, CRC(23dfe3a6) SHA1(2ad7cbcbdbb434dc43e9c94cd00df9e57ac097f5) )
	ROM_LOAD( "e13_j08.bin",  0x4000, 0x2000, CRC(3ba95390) SHA1(b22ad7cfda392894208eb4b39505f38bfe4c4342) )
	ROM_LOAD( "e14_j09.bin",  0x6000, 0x2000, CRC(a9fba85a) SHA1(1a649ec667d377ffab26b4694be790b3a2742f30) )
	ROM_LOAD( "e15_j10.bin",  0x8000, 0x2000, CRC(0532347e) SHA1(4c02b75a62993cce60d2cb87b81c7738abbc9a0d) )
	ROM_LOAD( "e16_j11.bin",  0xa000, 0x2000, CRC(e1725d24) SHA1(d315588e6cc2f4263be621d2d8603c8215a90046) )

	ROM_REGION( 0x0220, REGION_PROMS, 0 )
	ROM_LOAD( "a02_j18.bin",  0x0000, 0x020, CRC(10dd4eaa) SHA1(599acd25f36445221c553510a5de23ddba5ecc15) ) /* palette */
	ROM_LOAD( "c10_j16.bin",  0x0020, 0x100, CRC(c244f2aa) SHA1(86df21c8e0b1ed51a0a4bd33dbb33f6efdea7d39) ) /* character lookup table */
	ROM_LOAD( "b07_j17.bin",  0x0120, 0x100, CRC(13989357) SHA1(0d61d468f6d3e1570fd18d236ec8cab92db4ed5c) ) /* sprite lookup table */
ROM_END

ROM_START( circusc2 )
	ROM_REGION( 2*0x10000, REGION_CPU1, 0 )     /* 64k for code + 64k for decrypted opcodes */
	ROM_LOAD( "h03_r05.bin",  0x6000, 0x2000, CRC(ed52c60f) SHA1(aa9dc6a57e29895be521ac6a146de56a7beef957) )
	ROM_LOAD( "h04_n04.bin",  0x8000, 0x2000, CRC(fcc99e33) SHA1(da140a849ac22419e8890414b8984aa264f7e3c7) )
	ROM_LOAD( "h05_n03.bin",  0xa000, 0x2000, CRC(5ef5b3b5) SHA1(b058600c915a0d6653eaa5fc87ecee44a38eed00) )
	ROM_LOAD( "h06_n02.bin",  0xc000, 0x2000, CRC(a5a5e796) SHA1(a41700b272ff4198447ed75138d65ec3a759d221) )
	ROM_LOAD( "h07_n01.bin",  0xe000, 0x2000, CRC(70d26721) SHA1(eb71cb0da26991a3628150f45f1389c2f2ef90fc) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )     /* 64k for the audio CPU */
	ROM_LOAD( "cd05_l14.bin", 0x0000, 0x2000, CRC(607df0fb) SHA1(67103d61994fd3a1e2de7cf9487e4f763234b18e) )
	ROM_LOAD( "cd07_l15.bin", 0x2000, 0x2000, CRC(a6ad30e1) SHA1(14f305717edcc2471e763b262960a0b96eef3530) )

	ROM_REGION( 0x04000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "a04_j12.bin",  0x0000, 0x2000, CRC(56e5b408) SHA1(73b9e3d46dfe9e39b390c634df153648a0906876) )
	ROM_LOAD( "a05_k13.bin",  0x2000, 0x2000, CRC(5aca0193) SHA1(4d0b0a773c385b7f1dcf024760d0437f47e78fbe) )

	ROM_REGION( 0x0c000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "e11_j06.bin",  0x0000, 0x2000, CRC(df0405c6) SHA1(70a50dcc86dfbdaa9c2af613105aae7f90747804) )
	ROM_LOAD( "e12_j07.bin",  0x2000, 0x2000, CRC(23dfe3a6) SHA1(2ad7cbcbdbb434dc43e9c94cd00df9e57ac097f5) )
	ROM_LOAD( "e13_j08.bin",  0x4000, 0x2000, CRC(3ba95390) SHA1(b22ad7cfda392894208eb4b39505f38bfe4c4342) )
	ROM_LOAD( "e14_j09.bin",  0x6000, 0x2000, CRC(a9fba85a) SHA1(1a649ec667d377ffab26b4694be790b3a2742f30) )
	ROM_LOAD( "e15_j10.bin",  0x8000, 0x2000, CRC(0532347e) SHA1(4c02b75a62993cce60d2cb87b81c7738abbc9a0d) )
	ROM_LOAD( "e16_j11.bin",  0xa000, 0x2000, CRC(e1725d24) SHA1(d315588e6cc2f4263be621d2d8603c8215a90046) )

	ROM_REGION( 0x0220, REGION_PROMS, 0 )
	ROM_LOAD( "a02_j18.bin",  0x0000, 0x020, CRC(10dd4eaa) SHA1(599acd25f36445221c553510a5de23ddba5ecc15) ) /* palette */
	ROM_LOAD( "c10_j16.bin",  0x0020, 0x100, CRC(c244f2aa) SHA1(86df21c8e0b1ed51a0a4bd33dbb33f6efdea7d39) ) /* character lookup table */
	ROM_LOAD( "b07_j17.bin",  0x0120, 0x100, CRC(13989357) SHA1(0d61d468f6d3e1570fd18d236ec8cab92db4ed5c) ) /* sprite lookup table */
ROM_END

ROM_START( circuscc )
	ROM_REGION( 2*0x10000, REGION_CPU1, 0 )     /* 64k for code + 64k for decrypted opcodes */
	ROM_LOAD( "cc_u05.h3",    0x6000, 0x2000, CRC(964c035a) SHA1(bd69bb755be327d04fc95cd33115663b33b33ed3) )
	ROM_LOAD( "p04",          0x8000, 0x2000, CRC(dd0c0ee7) SHA1(e56e48f6f251430b7ce0e2cc59cfd00b5c760b9c) )
	ROM_LOAD( "p03",          0xa000, 0x2000, CRC(190247af) SHA1(f2128fb5e6c16791493af1c77628b610b86d4677) )
	ROM_LOAD( "p02",          0xc000, 0x2000, CRC(7e63725e) SHA1(f731f15956c6e7a0a4e8225513f8b9e6017c7a17) )
	ROM_LOAD( "p01",          0xe000, 0x2000, CRC(eedaa5b2) SHA1(0c606ca4d092c3dc290c30b1a73f94e3b348e8fd) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )     /* 64k for the audio CPU */
	ROM_LOAD( "cd05_l14.bin", 0x0000, 0x2000, CRC(607df0fb) SHA1(67103d61994fd3a1e2de7cf9487e4f763234b18e) )
	ROM_LOAD( "cd07_l15.bin", 0x2000, 0x2000, CRC(a6ad30e1) SHA1(14f305717edcc2471e763b262960a0b96eef3530) )

	ROM_REGION( 0x04000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "a04_j12.bin",  0x0000, 0x2000, CRC(56e5b408) SHA1(73b9e3d46dfe9e39b390c634df153648a0906876) )
	ROM_LOAD( "a05_k13.bin",  0x2000, 0x2000, CRC(5aca0193) SHA1(4d0b0a773c385b7f1dcf024760d0437f47e78fbe) )

	ROM_REGION( 0x0c000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "e11_j06.bin",  0x0000, 0x2000, CRC(df0405c6) SHA1(70a50dcc86dfbdaa9c2af613105aae7f90747804) )
	ROM_LOAD( "e12_j07.bin",  0x2000, 0x2000, CRC(23dfe3a6) SHA1(2ad7cbcbdbb434dc43e9c94cd00df9e57ac097f5) )
	ROM_LOAD( "e13_j08.bin",  0x4000, 0x2000, CRC(3ba95390) SHA1(b22ad7cfda392894208eb4b39505f38bfe4c4342) )
	ROM_LOAD( "e14_j09.bin",  0x6000, 0x2000, CRC(a9fba85a) SHA1(1a649ec667d377ffab26b4694be790b3a2742f30) )
	ROM_LOAD( "e15_j10.bin",  0x8000, 0x2000, CRC(0532347e) SHA1(4c02b75a62993cce60d2cb87b81c7738abbc9a0d) )
	ROM_LOAD( "e16_j11.bin",  0xa000, 0x2000, CRC(e1725d24) SHA1(d315588e6cc2f4263be621d2d8603c8215a90046) )

	ROM_REGION( 0x0220, REGION_PROMS, 0 )
	ROM_LOAD( "a02_j18.bin",  0x0000, 0x020, CRC(10dd4eaa) SHA1(599acd25f36445221c553510a5de23ddba5ecc15) ) /* palette */
	ROM_LOAD( "c10_j16.bin",  0x0020, 0x100, CRC(c244f2aa) SHA1(86df21c8e0b1ed51a0a4bd33dbb33f6efdea7d39) ) /* character lookup table */
	ROM_LOAD( "b07_j17.bin",  0x0120, 0x100, CRC(13989357) SHA1(0d61d468f6d3e1570fd18d236ec8cab92db4ed5c) ) /* sprite lookup table */
ROM_END

ROM_START( circusce )
	ROM_REGION( 2*0x10000, REGION_CPU1, 0 )     /* 64k for code + 64k for decrypted opcodes */
	ROM_LOAD( "p05",          0x6000, 0x2000, CRC(7ca74494) SHA1(326e081490e413b0638ec77de184b128fb2afd14) )
	ROM_LOAD( "p04",          0x8000, 0x2000, CRC(dd0c0ee7) SHA1(e56e48f6f251430b7ce0e2cc59cfd00b5c760b9c) )
	ROM_LOAD( "p03",          0xa000, 0x2000, CRC(190247af) SHA1(f2128fb5e6c16791493af1c77628b610b86d4677) )
	ROM_LOAD( "p02",          0xc000, 0x2000, CRC(7e63725e) SHA1(f731f15956c6e7a0a4e8225513f8b9e6017c7a17) )
	ROM_LOAD( "p01",          0xe000, 0x2000, CRC(eedaa5b2) SHA1(0c606ca4d092c3dc290c30b1a73f94e3b348e8fd) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )     /* 64k for the audio CPU */
	ROM_LOAD( "cd05_l14.bin", 0x0000, 0x2000, CRC(607df0fb) SHA1(67103d61994fd3a1e2de7cf9487e4f763234b18e) )
	ROM_LOAD( "cd07_l15.bin", 0x2000, 0x2000, CRC(a6ad30e1) SHA1(14f305717edcc2471e763b262960a0b96eef3530) )

	ROM_REGION( 0x04000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "a04_j12.bin",  0x0000, 0x2000, CRC(56e5b408) SHA1(73b9e3d46dfe9e39b390c634df153648a0906876) )
	ROM_LOAD( "a05_k13.bin",  0x2000, 0x2000, CRC(5aca0193) SHA1(4d0b0a773c385b7f1dcf024760d0437f47e78fbe) )

	ROM_REGION( 0x0c000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "e11_j06.bin",  0x0000, 0x2000, CRC(df0405c6) SHA1(70a50dcc86dfbdaa9c2af613105aae7f90747804) )
	ROM_LOAD( "e12_j07.bin",  0x2000, 0x2000, CRC(23dfe3a6) SHA1(2ad7cbcbdbb434dc43e9c94cd00df9e57ac097f5) )
	ROM_LOAD( "e13_j08.bin",  0x4000, 0x2000, CRC(3ba95390) SHA1(b22ad7cfda392894208eb4b39505f38bfe4c4342) )
	ROM_LOAD( "e14_j09.bin",  0x6000, 0x2000, CRC(a9fba85a) SHA1(1a649ec667d377ffab26b4694be790b3a2742f30) )
	ROM_LOAD( "e15_j10.bin",  0x8000, 0x2000, CRC(0532347e) SHA1(4c02b75a62993cce60d2cb87b81c7738abbc9a0d) )
	ROM_LOAD( "e16_j11.bin",  0xa000, 0x2000, CRC(e1725d24) SHA1(d315588e6cc2f4263be621d2d8603c8215a90046) )

	ROM_REGION( 0x0220, REGION_PROMS, 0 )
	ROM_LOAD( "a02_j18.bin",  0x0000, 0x020, CRC(10dd4eaa) SHA1(599acd25f36445221c553510a5de23ddba5ecc15) ) /* palette */
	ROM_LOAD( "c10_j16.bin",  0x0020, 0x100, CRC(c244f2aa) SHA1(86df21c8e0b1ed51a0a4bd33dbb33f6efdea7d39) ) /* character lookup table */
	ROM_LOAD( "b07_j17.bin",  0x0120, 0x100, CRC(13989357) SHA1(0d61d468f6d3e1570fd18d236ec8cab92db4ed5c) ) /* sprite lookup table */
ROM_END


static DRIVER_INIT( circusc )
{
	konami1_decode();
}


GAME( 1984, circusc,  0,       circusc, circusc, circusc, ROT90, "Konami", "Circus Charlie" )
GAME( 1984, circusc2, circusc, circusc, circusc, circusc, ROT90, "Konami", "Circus Charlie (no level select)" )
GAME( 1984, circuscc, circusc, circusc, circusc, circusc, ROT90, "Konami (Centuri license)", "Circus Charlie (Centuri)" )
GAME( 1984, circusce, circusc, circusc, circusc, circusc, ROT90, "Konami (Centuri license)", "Circus Charlie (Centuri, earlier)" )
