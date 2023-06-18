/***************************************************************************

Double Dribble(GX690) (c) Konami 1986

Driver by Manuel Abadia <manu@teleline.es>

***************************************************************************/

#include "driver.h"
#include "cpu/m6809/m6809.h"
#include "vidhrdw/generic.h"

int ddrible_int_enable_0;
int ddrible_int_enable_1;

static unsigned char *ddrible_sharedram;
static unsigned char *ddrible_snd_sharedram;

extern unsigned char *ddrible_spriteram_1;
extern unsigned char *ddrible_spriteram_2;
extern unsigned char *ddrible_fg_videoram;
extern unsigned char *ddrible_bg_videoram;

/* video hardware memory handlers */
WRITE_HANDLER( ddrible_fg_videoram_w );
WRITE_HANDLER( ddrible_bg_videoram_w );

/* video hardware functions */
PALETTE_INIT( ddrible );
VIDEO_START( ddrible );
VIDEO_UPDATE( ddrible );
WRITE_HANDLER( K005885_0_w );
WRITE_HANDLER( K005885_1_w );


static INTERRUPT_GEN( ddrible_interrupt_0 )
{
	if (ddrible_int_enable_0)
		cpu_set_irq_line(0, M6809_FIRQ_LINE, HOLD_LINE);
}

static INTERRUPT_GEN( ddrible_interrupt_1 )
{
	if (ddrible_int_enable_1)
		cpu_set_irq_line(1, M6809_FIRQ_LINE, HOLD_LINE);
}


static WRITE_HANDLER( ddrible_bankswitch_w )
{
	int bankaddress;
	unsigned char *RAM = memory_region(REGION_CPU1);

	bankaddress = 0x10000 + (data & 0x0f)*0x2000;
	cpu_setbank(1,&RAM[bankaddress]);
}


static READ_HANDLER( ddrible_sharedram_r )
{
	return ddrible_sharedram[offset];
}

static WRITE_HANDLER( ddrible_sharedram_w )
{
	ddrible_sharedram[offset] = data;
}

static READ_HANDLER( ddrible_snd_sharedram_r )
{
	return ddrible_snd_sharedram[offset];
}

static WRITE_HANDLER( ddrible_snd_sharedram_w )
{
	ddrible_snd_sharedram[offset] = data;
}

static WRITE_HANDLER( ddrible_coin_counter_w )
{
	/* b4-b7: unused */
	/* b2-b3: unknown */
	/* b1: coin counter 2 */
	/* b0: coin counter 1 */
	coin_counter_w(0,(data) & 0x01);
	coin_counter_w(1,(data >> 1) & 0x01);
}

static READ_HANDLER( ddrible_vlm5030_busy_r )
{
	return rand(); /* patch */
	if (VLM5030_BSY()) return 1;
	else return 0;
}

static WRITE_HANDLER( ddrible_vlm5030_ctrl_w )
{
	unsigned char *SPEECH_ROM = memory_region(REGION_SOUND1);
	/* b7 : vlm data bus OE   */
	/* b6 : VLM5030-RST       */
	/* b5 : VLM5030-ST        */
	/* b4 : VLM5300-VCU       */
	/* b3 : ROM bank select   */
	VLM5030_RST( data & 0x40 ? 1 : 0 );
	VLM5030_ST(  data & 0x20 ? 1 : 0 );
	VLM5030_VCU( data & 0x10 ? 1 : 0 );
	VLM5030_set_rom(&SPEECH_ROM[data & 0x08 ? 0x10000 : 0]);
	/* b2 : SSG-C rc filter enable */
	/* b1 : SSG-B rc filter enable */
	/* b0 : SSG-A rc filter enable */
	set_RC_filter(2,1000,2200,1000,data & 0x04 ? 150000 : 0); /* YM2203-SSG-C */
	set_RC_filter(1,1000,2200,1000,data & 0x02 ? 150000 : 0); /* YM2203-SSG-B */
	set_RC_filter(0,1000,2200,1000,data & 0x01 ? 150000 : 0); /* YM2203-SSG-A */
}


static MEMORY_READ_START( readmem_cpu0 )
	{ 0x1800, 0x187f, MRA_RAM },			/* palette */
	{ 0x2000, 0x3fff, MRA_RAM },			/* Video RAM 1 + Object RAM 1 */
	{ 0x4000, 0x5fff, MRA_RAM },			/* shared RAM with CPU #1 */
	{ 0x6000, 0x7fff, MRA_RAM },			/* Video RAM 2 + Object RAM 2 */
	{ 0x8000, 0x9fff, MRA_BANK1 },			/* banked ROM */
	{ 0xa000, 0xffff, MRA_ROM },			/* ROM */
MEMORY_END


static MEMORY_WRITE_START( writemem_cpu0 )
	{ 0x0000, 0x0004, K005885_0_w },								/* video registers (005885 #1) */
	{ 0x0800, 0x0804, K005885_1_w },								/* video registers (005885 #2) */
	{ 0x1800, 0x187f, paletteram_xBBBBBGGGGGRRRRR_swap_w, &paletteram },/* seems wrong, MSB is used as well */
	{ 0x2000, 0x2fff, ddrible_fg_videoram_w, &ddrible_fg_videoram },/* Video RAM 1 */
	{ 0x3000, 0x3fff, MWA_RAM, &ddrible_spriteram_1 },				/* Object RAM 1 */
	{ 0x4000, 0x5fff, MWA_RAM, &ddrible_sharedram },				/* shared RAM with CPU #1 */
	{ 0x6000, 0x6fff, ddrible_bg_videoram_w, &ddrible_bg_videoram },/* Video RAM 2 */
	{ 0x7000, 0x7fff, MWA_RAM, &ddrible_spriteram_2 },				/* Object RAM 2 + Work RAM */
	{ 0x8000, 0x8000, ddrible_bankswitch_w },						/* bankswitch control */
	{ 0x8000, 0xffff, MWA_ROM },									/* ROM */
MEMORY_END

static MEMORY_READ_START( readmem_cpu1 )
	{ 0x0000, 0x1fff, ddrible_sharedram_r },		/* shared RAM with CPU #0 */
	{ 0x2000, 0x27ff, ddrible_snd_sharedram_r },	/* shared RAM with CPU #2 */
	{ 0x2800, 0x2800, input_port_3_r },				/* DSW #1 */
	{ 0x2801, 0x2801, input_port_0_r },				/* player 1 inputs */
	{ 0x2802, 0x2802, input_port_1_r },				/* player 2 inputs */
	{ 0x2803, 0x2803, input_port_2_r },				/* coinsw & start */
	{ 0x2c00, 0x2c00, input_port_4_r },				/* DSW #2 */
	{ 0x3000, 0x3000, input_port_5_r },				/* DSW #3 */
	{ 0x8000, 0xffff, MRA_ROM },					/* ROM */
MEMORY_END

static MEMORY_WRITE_START( writemem_cpu1 )
	{ 0x0000, 0x1fff, ddrible_sharedram_w },		/* shared RAM with CPU #0 */
	{ 0x2000, 0x27ff, ddrible_snd_sharedram_w },	/* shared RAM with CPU #2 */
	{ 0x3400, 0x3400, ddrible_coin_counter_w },		/* coin counters */
	{ 0x3c00, 0x3c00, watchdog_reset_w },			/* watchdog reset */
	{ 0x8000, 0xffff, MWA_ROM },					/* ROM */
MEMORY_END

static MEMORY_READ_START( readmem_cpu2 )
	{ 0x0000, 0x07ff, MRA_RAM },					/* shared RAM with CPU #1 */
	{ 0x1000, 0x1000, YM2203_status_port_0_r },		/* YM2203 */
	{ 0x1001, 0x1001, YM2203_read_port_0_r },		/* YM2203 */
	{ 0x8000, 0xffff, MRA_ROM },					/* ROM */
MEMORY_END

static MEMORY_WRITE_START( writemem_cpu2 )
	{ 0x0000, 0x07ff, MWA_RAM, &ddrible_snd_sharedram  },	/* shared RAM with CPU #1 */
	{ 0x1000, 0x1000, YM2203_control_port_0_w },			/* YM2203 */
	{ 0x1001, 0x1001, YM2203_write_port_0_w },				/* YM2203 */
	{ 0x3000, 0x3000, VLM5030_data_w },						/* Speech data */
	{ 0x8000, 0xffff, MWA_ROM },							/* ROM */
MEMORY_END

INPUT_PORTS_START( ddribble )
	PORT_START	/* PLAYER 1 INPUTS */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON3 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* PLAYER 2 INPUTS */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* COINSW & START */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* DSW #1 */
	PORT_DIPNAME( 0x0f, 0x0f, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x05, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 3C_2C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 4C_3C ) )
	PORT_DIPSETTING(    0x0f, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 4C_5C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 3C_4C ) )
	PORT_DIPSETTING(    0x07, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x0e, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 2C_5C ) )
	PORT_DIPSETTING(    0x0d, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x0b, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0x0a, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(    0x09, DEF_STR( 1C_7C ) )
	PORT_DIPNAME( 0xf0, 0xf0, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x50, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 3C_2C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 4C_3C ) )
	PORT_DIPSETTING(    0xf0, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 4C_5C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 3C_4C ) )
	PORT_DIPSETTING(    0x70, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0xe0, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x60, DEF_STR( 2C_5C ) )
	PORT_DIPSETTING(    0xd0, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0xb0, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0xa0, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(    0x90, DEF_STR( 1C_7C ) )

	PORT_START	/* DSW #2 */
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(	0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(	0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(	0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(	0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(	0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x60, 0x40, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(	0x60, "Easy" )
	PORT_DIPSETTING(	0x40, "Normal" )
	PORT_DIPSETTING(	0x20, "Hard" )
	PORT_DIPSETTING(	0x00, "Hardest" )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(	0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )

	PORT_START	/* DSW #3 */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(	0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(	0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_SERVICE( 0x04, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x08, 0x00, "Allow vs match with 1 Credit" )
	PORT_DIPSETTING(	0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )
INPUT_PORTS_END



static struct GfxLayout charlayout =
{
	8,8,
	RGN_FRAC(1,2),
	4,
	{ 0, 1, 2, 3 },
	{ 0*4, 1*4, 2*4, 3*4, 4*4, 5*4, 6*4, 7*4 },
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32 },
	32*8
};

static struct GfxLayout spritelayout =
{
	16,16,
	RGN_FRAC(1,2),
	4,
	{ 0, 1, 2, 3 },
	{ 0*4, 1*4, 2*4, 3*4, 4*4, 5*4, 6*4, 7*4,
			32*8+0*4, 32*8+1*4, 32*8+2*4, 32*8+3*4, 32*8+4*4, 32*8+5*4, 32*8+6*4, 32*8+7*4 },
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32,
			16*32, 17*32, 18*32, 19*32, 20*32, 21*32, 22*32, 23*32 },
	32*32
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0x00000, &charlayout,    48,  1 },	/* colors 48-63 */
	{ REGION_GFX2, 0x00000, &charlayout,    16,  1 },	/* colors 16-31 */
	{ REGION_GFX1, 0x20000, &spritelayout,  32,  1 },	/* colors 32-47 */
	{ REGION_GFX2, 0x40000, &spritelayout,  64, 16 },	/* colors  0-15 but using lookup table */
	{ -1 } /* end of array */
};

static struct YM2203interface ym2203_interface =
{
	1,			/* 1 chip */
	3580000,	/* 3.58 MHz */
	{ YM2203_VOL(25,25) },
	{ 0 },
	{ ddrible_vlm5030_busy_r },
	{ ddrible_vlm5030_ctrl_w },
	{ 0 }
};

static struct VLM5030interface vlm5030_interface =
{
	3580000,    /* 3.58 MHz */
	100,         /* volume */
	REGION_SOUND1,/* memory region of speech rom */
	0x10000     /* memory size 64Kbyte * 2 bank */
};

static MACHINE_DRIVER_START( ddribble )

	/* basic machine hardware */
	MDRV_CPU_ADD(M6809,	1536000)	/* 18432000/12 MHz? */
	MDRV_CPU_MEMORY(readmem_cpu0,writemem_cpu0)
	MDRV_CPU_VBLANK_INT(ddrible_interrupt_0,1)

	MDRV_CPU_ADD(M6809,	1536000)	/* 18432000/12 MHz? */
	MDRV_CPU_MEMORY(readmem_cpu1,writemem_cpu1)
	MDRV_CPU_VBLANK_INT(ddrible_interrupt_1,1)

	MDRV_CPU_ADD(M6809,	1536000)	/* 18432000/12 MHz? */
	MDRV_CPU_MEMORY(readmem_cpu2,writemem_cpu2)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(100)	/* we need heavy synch */

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 2*8, 30*8-1)
/*	MDRV_SCREEN_SIZE(64*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 64*8-1, 2*8, 30*8-1) */
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(64)
	MDRV_COLORTABLE_LENGTH(64 + 256)

	MDRV_PALETTE_INIT(ddrible)
	MDRV_VIDEO_START(ddrible)
	MDRV_VIDEO_UPDATE(ddrible)

	/* sound hardware */
	MDRV_SOUND_ADD(YM2203, ym2203_interface)
	MDRV_SOUND_ADD(VLM5030, vlm5030_interface)
MACHINE_DRIVER_END


ROM_START( ddribble )
	ROM_REGION( 0x1a000, REGION_CPU1, 0 ) /* 64K CPU #0 + 40K for Banked ROMS */
	ROM_LOAD( "690c03.bin",	0x10000, 0x0a000, CRC(07975a58) SHA1(96fd1b2348bbdf560067d8ee3cd4c0514e263d7a) )
	ROM_CONTINUE(			0x0a000, 0x06000 )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* 64 for the CPU #1 */
	ROM_LOAD( "690c02.bin", 0x08000, 0x08000, CRC(f07c030a) SHA1(db96a10f8bb657bf285266db9e775fa6af82f38c) )

	ROM_REGION( 0x10000, REGION_CPU3, 0 )	/* 64k for the SOUND CPU */
	ROM_LOAD( "690b01.bin", 0x08000, 0x08000, CRC(806b8453) SHA1(3184772c5e5181438a17ac72129070bf164b2965) )

	ROM_REGION( 0x40000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD16_BYTE( "690a05.bin",	0x00000, 0x20000, CRC(6a816d0d) SHA1(73f2527d5f2b9d51b784be36e07e0d0c566a28d9) )	/* characters & objects */
	ROM_LOAD16_BYTE( "690a06.bin",	0x00001, 0x20000, CRC(46300cd0) SHA1(07197a546fff452a41575fcd481da64ac6bf601e) )

	ROM_REGION( 0x80000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD16_BYTE( "690a10.bin", 0x00000, 0x20000, CRC(61efa222) SHA1(bd7b993ad1c06d8f6ac29fbc07c4a987abe1ab42) )	/* characters */
	ROM_LOAD16_BYTE( "690a09.bin", 0x00001, 0x20000, CRC(ab682186) SHA1(a28982835042a07354557e1539b097cdf93fc466) )
	ROM_LOAD16_BYTE( "690a08.bin", 0x40000, 0x20000, CRC(9a889944) SHA1(ca96815aefb1e336bd2288841b00a5c21cacf90f) )	/* objects */
	ROM_LOAD16_BYTE( "690a07.bin", 0x40001, 0x20000, CRC(faf81b3f) SHA1(0bd647b4cdd3f2209472e303fd22eedd5533d1b1) )

	ROM_REGION( 0x0100, REGION_PROMS, 0 )
	ROM_LOAD( "690a11.i15", 0x0000, 0x0100, CRC(f34617ad) SHA1(79ceba6fe204472a5a659641ac4f14bb1f0ee3f6) )	/* sprite lookup table */

	ROM_REGION( 0x20000, REGION_SOUND1, 0 )	/* 128k for the VLM5030 data */
	ROM_LOAD( "690a04.bin", 0x00000, 0x20000, CRC(1bfeb763) SHA1(f3e9acb2a7a9b4c8dee6838c1344a7a65c27ff77) )
ROM_END



GAME( 1986, ddribble, 0, ddribble, ddribble, 0, ROT0, "Konami", "Double Dribble")
