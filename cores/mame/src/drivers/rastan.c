/***************************************************************************

Rastan

driver by Jarek Burczynski

***************************************************************************/

#include "driver.h"
#include "state.h"
#include "vidhrdw/generic.h"
#include "vidhrdw/taitoic.h"
#include "sndhrdw/taitosnd.h"

data16_t *rastan_ram;	/* speedup hack */

WRITE16_HANDLER( rastan_spritectrl_w );

VIDEO_START( rastan );
VIDEO_UPDATE( rastan );

WRITE_HANDLER( rastan_adpcm_trigger_w );
WRITE_HANDLER( rastan_c000_w );
WRITE_HANDLER( rastan_d000_w );


static READ16_HANDLER( rastan_cycle_r )
{
	if (activecpu_get_pc()==0x3b088) cpu_spinuntil_int();

	return rastan_ram[0x1c10/2];
}


static MEMORY_READ16_START( rastan_readmem )
	{ 0x000000, 0x05ffff, MRA16_ROM },
	{ 0x10dc10, 0x10dc11, rastan_cycle_r },
	{ 0x10c000, 0x10ffff, MRA16_RAM },	/* RAM */
	{ 0x200000, 0x200fff, MRA16_RAM },	/* palette */
	{ 0x3e0000, 0x3e0001, MRA16_NOP },
	{ 0x3e0002, 0x3e0003, taitosound_comm16_lsb_r },
	{ 0x390000, 0x390001, input_port_0_word_r },
	{ 0x390002, 0x390003, input_port_1_word_r },
	{ 0x390006, 0x390007, input_port_2_word_r },
	{ 0x390008, 0x390009, input_port_3_word_r },
	{ 0x39000a, 0x39000b, input_port_4_word_r },
	{ 0xc00000, 0xc0ffff, PC080SN_word_0_r },
	{ 0xd00000, 0xd03fff, PC090OJ_word_0_r },	/* sprite ram */
MEMORY_END

static MEMORY_WRITE16_START( rastan_writemem )
	{ 0x000000, 0x05ffff, MWA16_ROM },
	{ 0x10c000, 0x10ffff, MWA16_RAM, &rastan_ram },
	{ 0x200000, 0x200fff, paletteram16_xBBBBBGGGGGRRRRR_word_w, &paletteram16 },
	{ 0x350008, 0x35000b, MWA16_NOP },	/* 0 only (often) ? */
	{ 0x380000, 0x380003, rastan_spritectrl_w },	/* sprite palette bank, coin counters, other unknowns */
	{ 0x3c0000, 0x3c0003, MWA16_NOP },	/* 0000,0020,0063,0992,1753 (very often) watchdog ? */
	{ 0x3e0000, 0x3e0001, taitosound_port16_lsb_w },
	{ 0x3e0002, 0x3e0003, taitosound_comm16_lsb_w },
	{ 0xc00000, 0xc0ffff, PC080SN_word_0_w },
	{ 0xc20000, 0xc20003, PC080SN_yscroll_word_0_w },
	{ 0xc40000, 0xc40003, PC080SN_xscroll_word_0_w },
	{ 0xc50000, 0xc50003, PC080SN_ctrl_word_0_w },
	{ 0xd00000, 0xd03fff, PC090OJ_word_0_w },	/* sprite ram */
MEMORY_END


static WRITE_HANDLER( rastan_bankswitch_w )
{
	cpu_setbank( 5, memory_region(REGION_CPU2) + ((data ^1) & 0x01) * 0x4000 + 0x10000 );
}

static MEMORY_READ_START( rastan_s_readmem )
	{ 0x0000, 0x3fff, MRA_ROM },
	{ 0x4000, 0x7fff, MRA_BANK5 },
	{ 0x8000, 0x8fff, MRA_RAM },
	{ 0x9001, 0x9001, YM2151_status_port_0_r },
	{ 0x9002, 0x9100, MRA_RAM },
	{ 0xa001, 0xa001, taitosound_slave_comm_r },
MEMORY_END

static MEMORY_WRITE_START( rastan_s_writemem )
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0x8000, 0x8fff, MWA_RAM },
	{ 0x9000, 0x9000, YM2151_register_port_0_w },
	{ 0x9001, 0x9001, YM2151_data_port_0_w },
	{ 0xa000, 0xa000, taitosound_slave_port_w },
	{ 0xa001, 0xa001, taitosound_slave_comm_w },
	{ 0xb000, 0xb000, rastan_adpcm_trigger_w },
	{ 0xc000, 0xc000, rastan_c000_w },
	{ 0xd000, 0xd000, rastan_d000_w },
MEMORY_END



INPUT_PORTS_START( rastan )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW,  IPT_SERVICE1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW,  IPT_TILT )
	PORT_BIT( 0x08, IP_ACTIVE_LOW,  IPT_START1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW,  IPT_START2 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START	/* DSW0 */
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_SERVICE( 0x04, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_1C ) )
	PORT_DIPNAME( 0xc0, 0xc0, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_6C ) )

	PORT_START	/* DSW1 */
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x02, "Easy" )
	PORT_DIPSETTING(    0x03, "Medium" )
	PORT_DIPSETTING(    0x01, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x0c, "100000" )
	PORT_DIPSETTING(    0x08, "150000" )
	PORT_DIPSETTING(    0x04, "200000" )
	PORT_DIPSETTING(    0x00, "250000" )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x30, "3" )
	PORT_DIPSETTING(    0x20, "4" )
	PORT_DIPSETTING(    0x10, "5" )
	PORT_DIPSETTING(    0x00, "6" )
	PORT_DIPNAME( 0x40, 0x40, "Allow Continue" )
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
INPUT_PORTS_END

INPUT_PORTS_START( rastsaga )		/* same as rastan, coinage is different */
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW,  IPT_SERVICE1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW,  IPT_TILT )
	PORT_BIT( 0x08, IP_ACTIVE_LOW,  IPT_START1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW,  IPT_START2 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START	/* DSW0 */
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_SERVICE( 0x04, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 1C_2C ) )
	PORT_DIPNAME( 0xc0, 0xc0, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 1C_2C ) )

	PORT_START	/* DSW1 */
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x02, "Easy" )
	PORT_DIPSETTING(    0x03, "Medium" )
	PORT_DIPSETTING(    0x01, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x0c, "100000" )
	PORT_DIPSETTING(    0x08, "150000" )
	PORT_DIPSETTING(    0x04, "200000" )
	PORT_DIPSETTING(    0x00, "250000" )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x30, "3" )
	PORT_DIPSETTING(    0x20, "4" )
	PORT_DIPSETTING(    0x10, "5" )
	PORT_DIPSETTING(    0x00, "6" )
	PORT_DIPNAME( 0x40, 0x40, "Allow Continue" )
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
INPUT_PORTS_END



static struct GfxLayout spritelayout1 =
{
	8,8,	/* 8*8 sprites */
	0x4000,	/* 16384 sprites */
	4,	/* 4 bits per pixel */
	{ 0, 1, 2, 3 },
	{ 0, 4, 0x40000*8+0 ,0x40000*8+4, 8+0, 8+4, 0x40000*8+8+0, 0x40000*8+8+4 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16 },
	16*8	/* every sprite takes 16 consecutive bytes */
};

static struct GfxLayout spritelayout2 =
{
	16,16,	/* 16*16 sprites */
	4096,	/* 4096 sprites */
	4,	/* 4 bits per pixel */
	{ 0, 1, 2, 3 },
	{
	0, 4, 0x40000*8+0 ,0x40000*8+4,
	8+0, 8+4, 0x40000*8+8+0, 0x40000*8+8+4,
	16+0, 16+4, 0x40000*8+16+0, 0x40000*8+16+4,
	24+0, 24+4, 0x40000*8+24+0, 0x40000*8+24+4
	},
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32,
			8*32, 9*32, 10*32, 11*32, 12*32, 13*32, 14*32, 15*32 },
	64*8	/* every sprite takes 64 consecutive bytes */
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX2, 0, &spritelayout2,  0, 0x80 },	/* sprites 16x16*/
	{ REGION_GFX1, 0, &spritelayout1,  0, 0x80 },	/* sprites 8x8*/
	{ -1 } /* end of array */
};


/* handler called by the YM2151 emulator when the internal timers cause an IRQ */
static void irqhandler(int irq)
{
	cpu_set_irq_line(1,0,irq ? ASSERT_LINE : CLEAR_LINE);
}

static struct YM2151interface ym2151_interface =
{
	1,			/* 1 chip */
	4000000,	/* 4 MHz ? */
	{ YM3012_VOL(50,MIXER_PAN_CENTER,50,MIXER_PAN_CENTER) },
	{ irqhandler },
	{ rastan_bankswitch_w }
};

static struct ADPCMinterface adpcm_interface =
{
	1,			/* 1 channel */
	8000,		/* 8000Hz playback */
	REGION_SOUND1,	/* memory region */
	{ 60 }		/* volume */
};



static MACHINE_DRIVER_START( rastan )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 8000000)	/* 8 MHz */
	MDRV_CPU_MEMORY(rastan_readmem,rastan_writemem)
	MDRV_CPU_VBLANK_INT(irq5_line_hold,1)

	MDRV_CPU_ADD(Z80, 4000000)	/* 4 MHz */
	MDRV_CPU_MEMORY(rastan_s_readmem,rastan_s_writemem)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(10)	/* 10 CPU slices per frame - enough for the sound CPU to read all commands */

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(40*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 40*8-1, 1*8, 31*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(8192)

	MDRV_VIDEO_START(rastan)
	MDRV_VIDEO_UPDATE(rastan)

	/* sound hardware */
	MDRV_SOUND_ADD(YM2151, ym2151_interface)
	MDRV_SOUND_ADD(ADPCM, adpcm_interface)
MACHINE_DRIVER_END



/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( rastan )
	ROM_REGION( 0x60000, REGION_CPU1, 0 )	/* 6*64k for 68000 code */
	ROM_LOAD16_BYTE( "ic19_38.bin", 0x00000, 0x10000, CRC(1c91dbb1) SHA1(17fc55e8546cc0b847aebd67fb4570a1e9f128f3) )
	ROM_LOAD16_BYTE( "ic07_37.bin", 0x00001, 0x10000, CRC(ecf20bdd) SHA1(92e46b1edef40a19be17091c09daba598d77bca8) )
	ROM_LOAD16_BYTE( "ic20_40.bin", 0x20000, 0x10000, CRC(0930d4b3) SHA1(c269b3856040ed9409de99cca48f22a2f355fc4c) )
	ROM_LOAD16_BYTE( "ic08_39.bin", 0x20001, 0x10000, CRC(d95ade5e) SHA1(f47557dcfa9d3137e2a3838e45858fc21471cc91) )
	ROM_LOAD16_BYTE( "ic21_42.bin", 0x40000, 0x10000, CRC(1857a7cb) SHA1(7d967d04ade648c6ddb19aad9e184b6e272856da) )
	ROM_LOAD16_BYTE( "ic09_43.bin", 0x40001, 0x10000, CRC(c34b9152) SHA1(6ed9247ad455bc3b71d78b541591b269969830cb) )

	ROM_REGION( 0x1c000, REGION_CPU2, 0 )	/* 64k for the audio CPU */
	ROM_LOAD( "ic49_19.bin", 0x00000, 0x4000, CRC(ee81fdd8) SHA1(fa59dac2583a7d2979550dffc6f9c6c2bd67bfd5) )
	ROM_CONTINUE(            0x10000, 0xc000 )

	ROM_REGION( 0x080000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "ic40_01.bin",  0x00000, 0x20000, CRC(cd30de19) SHA1(f8d158d38cd07a24cb5ddefd4ce90beec706924d) )        /* 8x8 0 */
	ROM_LOAD( "ic39_03.bin",  0x20000, 0x20000, CRC(ab67e064) SHA1(5c49f0ff9221cba9f2bb8da86eb4448c73012410) )        /* 8x8 0 */
	ROM_LOAD( "ic67_02.bin",  0x40000, 0x20000, CRC(54040fec) SHA1(a2bea2ce1cebd25b33be41723299ca0512d95f9e) )        /* 8x8 1 */
	ROM_LOAD( "ic66_04.bin",  0x60000, 0x20000, CRC(94737e93) SHA1(3df7f085fe6468bda11fab2e86252df6f74f7a99) )        /* 8x8 1 */

	ROM_REGION( 0x080000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "ic15_05.bin",  0x00000, 0x20000, CRC(c22d94ac) SHA1(04f69f9af7ac4242e95dba32988afa3616d75a92) )        /* sprites 1a */
	ROM_LOAD( "ic14_07.bin",  0x20000, 0x20000, CRC(b5632a51) SHA1(da6ebe6afe245443a76b33714213549356c0c5c3) )        /* sprites 3a */
	ROM_LOAD( "ic28_06.bin",  0x40000, 0x20000, CRC(002ccf39) SHA1(fdc29f39198f9b488e298ee89b0eeb3417527733) )        /* sprites 1b */
	ROM_LOAD( "ic27_08.bin",  0x60000, 0x20000, CRC(feafca05) SHA1(9de9ff1fcf037e5ab25c181b678245041238d6ae) )        /* sprites 3b */

	ROM_REGION( 0x10000, REGION_SOUND1, 0 )	/* 64k for the samples */
	ROM_LOAD( "ic76_20.bin", 0x0000, 0x10000, CRC(fd1a34cc) SHA1(b1682959521fa295769207b75cf7d839e9ec95fd) ) /* samples are 4bit ADPCM */
ROM_END

ROM_START( rastanu )
	ROM_REGION( 0x60000, REGION_CPU1, 0 )	/* 6*64k for 68000 code */
	ROM_LOAD16_BYTE( "ic19_38.bin", 0x00000, 0x10000, CRC(1c91dbb1) SHA1(17fc55e8546cc0b847aebd67fb4570a1e9f128f3) )
	ROM_LOAD16_BYTE( "ic07_37.bin", 0x00001, 0x10000, CRC(ecf20bdd) SHA1(92e46b1edef40a19be17091c09daba598d77bca8) )
	ROM_LOAD16_BYTE( "b04-45.20",   0x20000, 0x10000, CRC(362812dd) SHA1(f7df037ef423d931ca780b34813d4e9e4db67054) )
	ROM_LOAD16_BYTE( "b04-44.8",    0x20001, 0x10000, CRC(51cc5508) SHA1(2bd266351a4d1b94c8c3a489e4d267695d93db5e) )
	ROM_LOAD16_BYTE( "ic21_42.bin", 0x40000, 0x10000, CRC(1857a7cb) SHA1(7d967d04ade648c6ddb19aad9e184b6e272856da) )
	ROM_LOAD16_BYTE( "b04-41-1.9",  0x40001, 0x10000, CRC(bd403269) SHA1(14aee828d5efb65370a5e453c8fd1c7b3e718074) )

	ROM_REGION( 0x1c000, REGION_CPU2, 0 )	/* 64k for the audio CPU */
	ROM_LOAD( "ic49_19.bin", 0x00000, 0x4000, CRC(ee81fdd8) SHA1(fa59dac2583a7d2979550dffc6f9c6c2bd67bfd5) )
	ROM_CONTINUE(            0x10000, 0xc000 )

	ROM_REGION( 0x080000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "ic40_01.bin",  0x00000, 0x20000, CRC(cd30de19) SHA1(f8d158d38cd07a24cb5ddefd4ce90beec706924d) )        /* 8x8 0 */
	ROM_LOAD( "ic39_03.bin",  0x20000, 0x20000, CRC(ab67e064) SHA1(5c49f0ff9221cba9f2bb8da86eb4448c73012410) )        /* 8x8 0 */
	ROM_LOAD( "ic67_02.bin",  0x40000, 0x20000, CRC(54040fec) SHA1(a2bea2ce1cebd25b33be41723299ca0512d95f9e) )        /* 8x8 1 */
	ROM_LOAD( "ic66_04.bin",  0x60000, 0x20000, CRC(94737e93) SHA1(3df7f085fe6468bda11fab2e86252df6f74f7a99) )        /* 8x8 1 */

	ROM_REGION( 0x080000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "ic15_05.bin",  0x00000, 0x20000, CRC(c22d94ac) SHA1(04f69f9af7ac4242e95dba32988afa3616d75a92) )        /* sprites 1a */
	ROM_LOAD( "ic14_07.bin",  0x20000, 0x20000, CRC(b5632a51) SHA1(da6ebe6afe245443a76b33714213549356c0c5c3) )        /* sprites 3a */
	ROM_LOAD( "ic28_06.bin",  0x40000, 0x20000, CRC(002ccf39) SHA1(fdc29f39198f9b488e298ee89b0eeb3417527733) )        /* sprites 1b */
	ROM_LOAD( "ic27_08.bin",  0x60000, 0x20000, CRC(feafca05) SHA1(9de9ff1fcf037e5ab25c181b678245041238d6ae) )        /* sprites 3b */

	ROM_REGION( 0x10000, REGION_SOUND1, 0 )	/* 64k for the samples */
	ROM_LOAD( "ic76_20.bin", 0x0000, 0x10000, CRC(fd1a34cc) SHA1(b1682959521fa295769207b75cf7d839e9ec95fd) ) /* samples are 4bit ADPCM */
ROM_END

ROM_START( rastanu2 )
	ROM_REGION( 0x60000, REGION_CPU1, 0 )	/* 6*64k for 68000 code */
	ROM_LOAD16_BYTE( "rs19_38.bin", 0x00000, 0x10000, CRC(a38ac909) SHA1(66d792fee03c6bd87d15060b9d5cae74137c5ebd) )
	ROM_LOAD16_BYTE( "b04-21.7",    0x00001, 0x10000, CRC(7c8dde9a) SHA1(0cfc3b4f3bc7b940a6c07267ac95e4aae25801ea) )
	ROM_LOAD16_BYTE( "b04-23.20",   0x20000, 0x10000, CRC(254b3dce) SHA1(5126cd5268abaa78dfdcd2ca70621c093c79be67) )
	ROM_LOAD16_BYTE( "b04-22.8",    0x20001, 0x10000, CRC(98e8edcf) SHA1(cc89ef36da6d21192efc19c3bbb215b1635b7ef3) )
	ROM_LOAD16_BYTE( "b04-25.21",   0x40000, 0x10000, CRC(d1e5adee) SHA1(eafc275a0023aecb2efaff14cd890915fa162624) )
	ROM_LOAD16_BYTE( "b04-24.9",    0x40001, 0x10000, CRC(a3dcc106) SHA1(3a8854530b08864a1f7f46c427e49ceec8297806) )

	ROM_REGION( 0x1c000, REGION_CPU2, 0 )	/* 64k for the audio CPU */
	ROM_LOAD( "ic49_19.bin", 0x00000, 0x4000, CRC(ee81fdd8) SHA1(fa59dac2583a7d2979550dffc6f9c6c2bd67bfd5) )
	ROM_CONTINUE(            0x10000, 0xc000 )

	ROM_REGION( 0x080000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "ic40_01.bin",  0x00000, 0x20000, CRC(cd30de19) SHA1(f8d158d38cd07a24cb5ddefd4ce90beec706924d) )        /* 8x8 0 */
	ROM_LOAD( "ic39_03.bin",  0x20000, 0x20000, CRC(ab67e064) SHA1(5c49f0ff9221cba9f2bb8da86eb4448c73012410) )        /* 8x8 0 */
	ROM_LOAD( "ic67_02.bin",  0x40000, 0x20000, CRC(54040fec) SHA1(a2bea2ce1cebd25b33be41723299ca0512d95f9e) )        /* 8x8 1 */
	ROM_LOAD( "ic66_04.bin",  0x60000, 0x20000, CRC(94737e93) SHA1(3df7f085fe6468bda11fab2e86252df6f74f7a99) )        /* 8x8 1 */

	ROM_REGION( 0x080000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "ic15_05.bin",  0x00000, 0x20000, CRC(c22d94ac) SHA1(04f69f9af7ac4242e95dba32988afa3616d75a92) )        /* sprites 1a */
	ROM_LOAD( "ic14_07.bin",  0x20000, 0x20000, CRC(b5632a51) SHA1(da6ebe6afe245443a76b33714213549356c0c5c3) )        /* sprites 3a */
	ROM_LOAD( "ic28_06.bin",  0x40000, 0x20000, CRC(002ccf39) SHA1(fdc29f39198f9b488e298ee89b0eeb3417527733) )        /* sprites 1b */
	ROM_LOAD( "ic27_08.bin",  0x60000, 0x20000, CRC(feafca05) SHA1(9de9ff1fcf037e5ab25c181b678245041238d6ae) )        /* sprites 3b */

	ROM_REGION( 0x10000, REGION_SOUND1, 0 )	/* 64k for the samples */
	ROM_LOAD( "ic76_20.bin", 0x0000, 0x10000, CRC(fd1a34cc) SHA1(b1682959521fa295769207b75cf7d839e9ec95fd) ) /* samples are 4bit ADPCM */
ROM_END

ROM_START( rastsaga )
	ROM_REGION( 0x60000, REGION_CPU1, 0 )	/* 6*64k for 68000 code */
	ROM_LOAD16_BYTE( "rs19_38.bin", 0x00000, 0x10000, CRC(a38ac909) SHA1(66d792fee03c6bd87d15060b9d5cae74137c5ebd) )
	ROM_LOAD16_BYTE( "rs07_37.bin", 0x00001, 0x10000, CRC(bad60872) SHA1(e020f79b3ac3d2abccfcd5d135d2dc49e1335c7d) )
	ROM_LOAD16_BYTE( "rs20_40.bin", 0x20000, 0x10000, CRC(6bcf70dc) SHA1(3e369548ac01981c503150b44c2747e6c2cec12a) )
	ROM_LOAD16_BYTE( "rs08_39.bin", 0x20001, 0x10000, CRC(8838ecc5) SHA1(42b43ab77969bbacdf178fbe73a0a27652ccb297) )
	ROM_LOAD16_BYTE( "rs21_42.bin", 0x40000, 0x10000, CRC(b626c439) SHA1(976e820edc4ba107c5b579edaaee1e354e85fb67) )
	ROM_LOAD16_BYTE( "rs09_43.bin", 0x40001, 0x10000, CRC(c928a516) SHA1(fe87fdf2d1b7ba93e1986460eb6af648b58f42e4) )

	ROM_REGION( 0x1c000, REGION_CPU2, 0 )	/* 64k for the audio CPU */
	ROM_LOAD( "ic49_19.bin", 0x00000, 0x4000, CRC(ee81fdd8) SHA1(fa59dac2583a7d2979550dffc6f9c6c2bd67bfd5) )
	ROM_CONTINUE(            0x10000, 0xc000 )

	ROM_REGION( 0x080000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "ic40_01.bin",  0x00000, 0x20000, CRC(cd30de19) SHA1(f8d158d38cd07a24cb5ddefd4ce90beec706924d) )        /* 8x8 0 */
	ROM_LOAD( "ic39_03.bin",  0x20000, 0x20000, CRC(ab67e064) SHA1(5c49f0ff9221cba9f2bb8da86eb4448c73012410) )        /* 8x8 0 */
	ROM_LOAD( "ic67_02.bin",  0x40000, 0x20000, CRC(54040fec) SHA1(a2bea2ce1cebd25b33be41723299ca0512d95f9e) )        /* 8x8 1 */
	ROM_LOAD( "ic66_04.bin",  0x60000, 0x20000, CRC(94737e93) SHA1(3df7f085fe6468bda11fab2e86252df6f74f7a99) )        /* 8x8 1 */

	ROM_REGION( 0x080000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "ic15_05.bin",  0x00000, 0x20000, CRC(c22d94ac) SHA1(04f69f9af7ac4242e95dba32988afa3616d75a92) )        /* sprites 1a */
	ROM_LOAD( "ic14_07.bin",  0x20000, 0x20000, CRC(b5632a51) SHA1(da6ebe6afe245443a76b33714213549356c0c5c3) )        /* sprites 3a */
	ROM_LOAD( "ic28_06.bin",  0x40000, 0x20000, CRC(002ccf39) SHA1(fdc29f39198f9b488e298ee89b0eeb3417527733) )        /* sprites 1b */
	ROM_LOAD( "ic27_08.bin",  0x60000, 0x20000, CRC(feafca05) SHA1(9de9ff1fcf037e5ab25c181b678245041238d6ae) )        /* sprites 3b */

	ROM_REGION( 0x10000, REGION_SOUND1, 0 )	/* 64k for the samples */
	ROM_LOAD( "ic76_20.bin", 0x0000, 0x10000, CRC(fd1a34cc) SHA1(b1682959521fa295769207b75cf7d839e9ec95fd) ) /* samples are 4bit ADPCM */
ROM_END


GAME( 1987, rastan,   0,      rastan, rastan,   0, ROT0, "Taito Corporation Japan", "Rastan (World)")
/* IDENTICAL to rastan, only difference is copyright notice and Coin B coinage */
GAME( 1987, rastanu,  rastan, rastan, rastsaga, 0, ROT0, "Taito America Corporation", "Rastan (US set 1)")
GAME( 1987, rastanu2, rastan, rastan, rastsaga, 0, ROT0, "Taito America Corporation", "Rastan (US set 2)")
GAME( 1987, rastsaga, rastan, rastan, rastsaga, 0, ROT0, "Taito Corporation", "Rastan Saga (Japan)")
