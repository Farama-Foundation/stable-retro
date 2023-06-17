/**************************************************************************

Based on drivers from Juno First emulator by Chris Hardy (chris@junofirst.freeserve.co.uk)

To enter service mode, keep 1&2 pressed on reset

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "cpu/m6809/m6809.h"
#include "cpu/i8039/i8039.h"
#include "cpu/z80/z80.h"


void konami1_decode(void);

extern unsigned char *megazone_scrollx;
extern unsigned char *megazone_scrolly;

static unsigned char *megazone_sharedram;

extern unsigned char *megazone_videoram2;
extern unsigned char *megazone_colorram2;
extern size_t megazone_videoram2_size;

static int i8039_status;

VIDEO_START( megazone );

WRITE_HANDLER( megazone_flipscreen_w );
PALETTE_INIT( megazone );
WRITE_HANDLER( megazone_sprite_bank_select_w );
VIDEO_UPDATE( megazone );



READ_HANDLER( megazone_portA_r )
{
	int clock,timer;


	/* main xtal 14.318MHz, divided by 8 to get the AY-3-8910 clock, further */
	/* divided by 1024 to get this timer */
	/* The base clock for the CPU and 8910 is NOT the same, so we have to */
	/* compensate. */
	/* (divide by (1024/2), and not 1024, because the CPU cycle counter is */
	/* incremented every other state change of the clock) */

	clock = activecpu_gettotalcycles() * 7159/12288;	/* = (14318/8)/(18432/6) */
	timer = (clock / (1024/2)) & 0x0f;

	/* low three bits come from the 8039 */

	return (timer << 4) | i8039_status;
}

static WRITE_HANDLER( megazone_portB_w )
{
	int i;


	for (i = 0;i < 3;i++)
	{
		int C;


		C = 0;
		if (data & 1) C +=  10000;	/*  10000pF = 0.01uF */
		if (data & 2) C += 220000;	/* 220000pF = 0.22uF */
		data >>= 2;
		set_RC_filter(i,1000,2200,200,C);
	}
}

WRITE_HANDLER( megazone_videoram2_w )
{
	if (megazone_videoram2[offset] != data)
	{
		megazone_videoram2[offset] = data;
	}
}

WRITE_HANDLER( megazone_colorram2_w )
{
	if (megazone_colorram2[offset] != data)
	{
		megazone_colorram2[offset] = data;
	}
}

READ_HANDLER( megazone_sharedram_r )
{
	return(megazone_sharedram[offset]);
}

WRITE_HANDLER( megazone_sharedram_w )
{
	megazone_sharedram[offset] = data;
}

static WRITE_HANDLER( megazone_i8039_irq_w )
{
	cpu_set_irq_line(2, 0, ASSERT_LINE);
}

WRITE_HANDLER( i8039_irqen_and_status_w )
{
	if ((data & 0x80) == 0)
		cpu_set_irq_line(2, 0, CLEAR_LINE);
	i8039_status = (data & 0x70) >> 4;
}

static WRITE_HANDLER( megazone_coin_counter_w )
{
	coin_counter_w(1-offset,data);		/* 1-offset, because coin counters are in reversed order */
}



static MEMORY_READ_START( readmem )
	{ 0x2000, 0x2fff, MRA_RAM },
	{ 0x3000, 0x33ff, MRA_RAM },
	{ 0x3800, 0x3fff, megazone_sharedram_r },
	{ 0x4000, 0xffff, MRA_ROM },		/* 4000->5FFF is a debug rom */
MEMORY_END

static MEMORY_WRITE_START( writemem )
	{ 0x0000, 0x0001, megazone_coin_counter_w }, /* coin counter 2, coin counter 1 */
	{ 0x0005, 0x0005, megazone_flipscreen_w },
	{ 0x0007, 0x0007, interrupt_enable_w },
	{ 0x0800, 0x0800, watchdog_reset_w },
	{ 0x1800, 0x1800, MWA_RAM, &megazone_scrollx },
	{ 0x1000, 0x1000, MWA_RAM, &megazone_scrolly },
	{ 0x2000, 0x23ff, videoram_w, &videoram, &videoram_size },
	{ 0x2400, 0x27ff, megazone_videoram2_w, &megazone_videoram2, &megazone_videoram2_size },
	{ 0x2800, 0x2bff, colorram_w, &colorram },
	{ 0x2c00, 0x2fff, megazone_colorram2_w, &megazone_colorram2 },
	{ 0x3000, 0x33ff, MWA_RAM, &spriteram, &spriteram_size },
	{ 0x3800, 0x3fff, megazone_sharedram_w, &megazone_sharedram },
	{ 0x4000, 0xffff, MWA_ROM },
MEMORY_END

static MEMORY_READ_START( sound_readmem )
	{ 0x0000, 0x1fff, MRA_ROM },
	{ 0x6000, 0x6000, input_port_0_r }, /* IO Coin */
	{ 0x6001, 0x6001, input_port_1_r }, /* P1 IO */
	{ 0x6002, 0x6002, input_port_2_r }, /* P2 IO */
	{ 0x8000, 0x8000, input_port_3_r }, /* DIP 1 */
	{ 0x8001, 0x8001, input_port_4_r }, /* DIP 2 */
	{ 0xe000, 0xe7ff, megazone_sharedram_r },  /* Shared with $3800->3fff of main CPU */
MEMORY_END

static MEMORY_WRITE_START( sound_writemem )
	{ 0x0000, 0x1fff, MWA_ROM },
	{ 0x2000, 0x2000, megazone_i8039_irq_w },	/* START line. Interrupts 8039 */
	{ 0x4000, 0x4000, soundlatch_w },			/* CODE  line. Command Interrupts 8039 */
	{ 0xa000, 0xa000, MWA_RAM },				/* INTMAIN - Interrupts main CPU (unused) */
	{ 0xc000, 0xc000, MWA_RAM },				/* INT (Actually is NMI) enable/disable (unused)*/
	{ 0xc001, 0xc001, watchdog_reset_w },
	{ 0xe000, 0xe7ff, megazone_sharedram_w },	/* Shared with $3800->3fff of main CPU */
MEMORY_END

static PORT_READ_START( sound_readport )
	{ 0x00, 0x02, AY8910_read_port_0_r },
PORT_END

static PORT_WRITE_START( sound_writeport )
	{ 0x00, 0x00, AY8910_control_port_0_w },
	{ 0x02, 0x02, AY8910_write_port_0_w },
PORT_END

static MEMORY_READ_START( i8039_readmem )
	{ 0x0000, 0x0fff, MRA_ROM },
MEMORY_END

static MEMORY_WRITE_START( i8039_writemem )
	{ 0x0000, 0x0fff, MWA_ROM },
MEMORY_END

static PORT_READ_START( i8039_readport )
	{ 0x00, 0xff, soundlatch_r },
PORT_END

static PORT_WRITE_START( i8039_writeport )
	{ I8039_p1, I8039_p1, DAC_0_data_w },
	{ I8039_p2, I8039_p2, i8039_irqen_and_status_w },
PORT_END

INPUT_PORTS_START( megazone )
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
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_COCKTAIL)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_COCKTAIL)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* DSW1 */
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x03, "3" )
	PORT_DIPSETTING(    0x02, "4" )
	PORT_DIPSETTING(    0x01, "5" )
	PORT_BITX(0,        0x00, IPT_DIPSWITCH_SETTING | IPF_CHEAT, "Infinite", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPNAME( 0x04, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x18, 0x18, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x18, "20k 70k" )
	PORT_DIPSETTING(    0x10, "20k 80k" )
	PORT_DIPSETTING(    0x08, "30k 90k" )
	PORT_DIPSETTING(    0x00, "30k 100k" )
	PORT_DIPNAME( 0x60, 0x40, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x60, "Easy" )
	PORT_DIPSETTING(    0x40, "Normal" )
	PORT_DIPSETTING(    0x20, "Difficult" )
	PORT_DIPSETTING(    0x00, "Very Difficult" )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START      /* DSW2 */
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
	16,16,	/* 16*16 sprites */
	256,	/* 256 sprites */
	4,	/* 4 bits per pixel */
	{ 0x4000*8+4, 0x4000*8+0, 4, 0 },
	{ 0, 1, 2, 3, 8*8+0, 8*8+1, 8*8+2, 8*8+3,
			16*8+0, 16*8+1, 16*8+2, 16*8+3, 24*8+0, 24*8+1, 24*8+2, 24*8+3 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 ,
		32*8, 33*8, 34*8, 35*8, 36*8, 37*8, 38*8, 39*8, },
	64*8	/* every sprite takes 64 consecutive bytes */
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &charlayout,       0, 16 },
	{ REGION_GFX2, 0, &spritelayout, 16*16, 16 },
	{ -1 } /* end of array */
};



static struct AY8910interface ay8910_interface =
{
	1,	/* 1 chip */
	14318000/8,	/* 1.78975 MHz */
	{ 30 },
	{ megazone_portA_r },
	{ 0 },
	{ 0 },
	{ megazone_portB_w }
};

static struct DACinterface dac_interface =
{
	1,
	{ 50 }
};



static MACHINE_DRIVER_START( megazone )

	/* basic machine hardware */
	MDRV_CPU_ADD(M6809, 18432000/9)        /* 2 MHz */
	MDRV_CPU_MEMORY(readmem,writemem)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,1)

	MDRV_CPU_ADD(Z80,18432000/6)     /* Z80 Clock is derived from the H1 signal */
	MDRV_CPU_MEMORY(sound_readmem,sound_writemem)
	MDRV_CPU_PORTS(sound_readport,sound_writeport)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,1)

	MDRV_CPU_ADD(I8039,(14318000/2)/I8039_CLOCK_DIVIDER)
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)	/* 1/2 14MHz crystal */
	MDRV_CPU_MEMORY(i8039_readmem,i8039_writemem)
	MDRV_CPU_PORTS(i8039_readport,i8039_writeport)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(15)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(36*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 36*8-1, 2*8, 30*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(32)
	MDRV_COLORTABLE_LENGTH(16*16+16*16)

	MDRV_PALETTE_INIT(megazone)
	MDRV_VIDEO_START(megazone)
	MDRV_VIDEO_UPDATE(megazone)

	/* sound hardware */
	MDRV_SOUND_ADD(AY8910, ay8910_interface)
	MDRV_SOUND_ADD(DAC, dac_interface)
MACHINE_DRIVER_END



/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( megazone )
	ROM_REGION( 2*0x10000, REGION_CPU1, 0 )     /* 64k for code + 64k for decrypted opcodes */
	ROM_LOAD( "319i07.bin",    0x6000, 0x2000, CRC(94b22ea8) SHA1(dc3ed2a0d1a12df51e46561324d78b7d655be313) )
	ROM_LOAD( "319i06.bin",    0x8000, 0x2000, CRC(0468b619) SHA1(a6755728fab37674749f9b77cb53f6f228102f2f) )
	ROM_LOAD( "319i05.bin",    0xa000, 0x2000, CRC(ac59000c) SHA1(c7568589f6b0e1706e996fdfed9c16755541951e) )
	ROM_LOAD( "319i04.bin",    0xc000, 0x2000, CRC(1e968603) SHA1(fd818f678a3dc8d48a30f9f7670bfcb42a3009a2) )
	ROM_LOAD( "319i03.bin",    0xe000, 0x2000, CRC(0888b803) SHA1(37249bfb14c6c3ce40ad68be457ab1f66fd7ea70) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )     /* 64k for the audio CPU */
	ROM_LOAD( "319e02.bin",   0x0000, 0x2000, CRC(d5d45edb) SHA1(3808d1b58fe152f8f5b49bf0aa40c53e9c9dd4bd) )

	ROM_REGION( 0x1000, REGION_CPU3, 0 )     /* 4k for the 8039 DAC CPU */
	ROM_LOAD( "319e01.bin",   0x0000, 0x1000, CRC(ed5725a0) SHA1(64f54621487291fbfe827fb4cecca299fd0db781) )

	ROM_REGION( 0x04000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "319e12.bin",    0x0000, 0x2000, CRC(e0fb7835) SHA1(44ccaaf92bdb83323f45e08dbe118697720e9105) )
	ROM_LOAD( "319e13.bin",    0x2000, 0x2000, CRC(3d8f3743) SHA1(1f6fbf804dacfa44cd11b4cf41d0bedb7f2ff6b6) )

	ROM_REGION( 0x08000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "319e11.bin",    0x0000, 0x2000, CRC(965a7ff6) SHA1(210aae91a3838e5f7c78747d9b7419d266538ffc) )
	ROM_LOAD( "319e09.bin",    0x2000, 0x2000, CRC(5eaa7f3e) SHA1(4c038e80d575988407252897a1f1bc6b76af597c) )
	ROM_LOAD( "319e10.bin",    0x4000, 0x2000, CRC(7bb1aeee) SHA1(be2dd46cd0121cedad6dab90a22643798a3176ab) )
	ROM_LOAD( "319e08.bin",    0x6000, 0x2000, CRC(6add71b1) SHA1(fc8c0ecd3b7f03d63b6c3143143986883345fa38) )

	ROM_REGION( 0x0260, REGION_PROMS, 0 )
	ROM_LOAD( "319b18.a16",  0x0000, 0x020, CRC(23cb02af) SHA1(aba459826a75ec07bc6d97580e934f58aa22f4f4) ) /* palette */
	ROM_LOAD( "319b16.c6",   0x0020, 0x100, CRC(5748e933) SHA1(c1478c31533a11421cd4579ccfdbb430e193d17b) ) /* sprite lookup table */
	ROM_LOAD( "319b17.a11",  0x0120, 0x100, CRC(1fbfce73) SHA1(1c58eb91982d5f10511d54a83070e859ac57d2f1) ) /* character lookup table */
	ROM_LOAD( "319b14.e7",   0x0220, 0x020, CRC(55044268) SHA1(29cf4158314ed897daf045a7f07be865dd977663) ) /* timing (not used) */
	ROM_LOAD( "319b15.e8",   0x0240, 0x020, CRC(31fd7ab9) SHA1(04d6e51b4930619c8ee12fb0d7b5f981e4d6d8d3) ) /* timing (not used) */
ROM_END

ROM_START( megaznik )
	ROM_REGION( 2*0x10000, REGION_CPU1, 0 )     /* 64k for code + 64k for decrypted opcodes */
	ROM_LOAD( "ic59_cpu.bin",  0x6000, 0x2000, CRC(f41922a0) SHA1(9f54509da18721a76593921c6e52085e62e6ea6b) )
	ROM_LOAD( "ic58_cpu.bin",  0x8000, 0x2000, CRC(7fd7277b) SHA1(e773247e0c9419cae49e04962ea362a2976c2db2) )
	ROM_LOAD( "ic57_cpu.bin",  0xa000, 0x2000, CRC(a4b33b51) SHA1(12bb4da0319a7fe355e5ea4945759c8709aed5fe) )
	ROM_LOAD( "ic56_cpu.bin",  0xc000, 0x2000, CRC(2aabcfbf) SHA1(f0054af98bd68158eab3328f8cf2a04b35e812c7) )
	ROM_LOAD( "ic55_cpu.bin",  0xe000, 0x2000, CRC(b33a3c37) SHA1(2f1fdf1b9f18fcc9bd97cc9adeecc4ce77dd30c9) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )     /* 64k for the audio CPU */
	ROM_LOAD( "319e02.bin",   0x0000, 0x2000, CRC(d5d45edb) SHA1(3808d1b58fe152f8f5b49bf0aa40c53e9c9dd4bd) )

	ROM_REGION( 0x1000, REGION_CPU3, 0 )     /* 4k for the 8039 DAC CPU */
	ROM_LOAD( "319e01.bin",   0x0000, 0x1000, CRC(ed5725a0) SHA1(64f54621487291fbfe827fb4cecca299fd0db781) )

	ROM_REGION( 0x04000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "ic40_vid.bin",  0x0000, 0x2000, CRC(07b8b24b) SHA1(faadcb20ee8b26b9ab0692df6a81e5423514863e) )
	ROM_LOAD( "319e13.bin",    0x2000, 0x2000, CRC(3d8f3743) SHA1(1f6fbf804dacfa44cd11b4cf41d0bedb7f2ff6b6) )

	ROM_REGION( 0x08000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "319e11.bin",    0x0000, 0x2000, CRC(965a7ff6) SHA1(210aae91a3838e5f7c78747d9b7419d266538ffc) )
	ROM_LOAD( "319e09.bin",    0x2000, 0x2000, CRC(5eaa7f3e) SHA1(4c038e80d575988407252897a1f1bc6b76af597c) )
	ROM_LOAD( "319e10.bin",    0x4000, 0x2000, CRC(7bb1aeee) SHA1(be2dd46cd0121cedad6dab90a22643798a3176ab) )
	ROM_LOAD( "319e08.bin",    0x6000, 0x2000, CRC(6add71b1) SHA1(fc8c0ecd3b7f03d63b6c3143143986883345fa38) )

	ROM_REGION( 0x0260, REGION_PROMS, 0 )
	ROM_LOAD( "319b18.a16",  0x0000, 0x020, CRC(23cb02af) SHA1(aba459826a75ec07bc6d97580e934f58aa22f4f4) ) /* palette */
	ROM_LOAD( "319b16.c6",   0x0020, 0x100, CRC(5748e933) SHA1(c1478c31533a11421cd4579ccfdbb430e193d17b) ) /* sprite lookup table */
	ROM_LOAD( "319b17.a11",  0x0120, 0x100, CRC(1fbfce73) SHA1(1c58eb91982d5f10511d54a83070e859ac57d2f1) ) /* character lookup table */
	ROM_LOAD( "319b14.e7",   0x0220, 0x020, CRC(55044268) SHA1(29cf4158314ed897daf045a7f07be865dd977663) ) /* timing (not used) */
	ROM_LOAD( "319b15.e8",   0x0240, 0x020, CRC(31fd7ab9) SHA1(04d6e51b4930619c8ee12fb0d7b5f981e4d6d8d3) ) /* timing (not used) */
ROM_END


static DRIVER_INIT( megazone )
{
	konami1_decode();
}


GAME( 1983, megazone, 0,        megazone, megazone, megazone, ROT90, "Konami", "Mega Zone" )
GAME( 1983, megaznik, megazone, megazone, megazone, megazone, ROT90, "Konami / Interlogic + Kosuka", "Mega Zone (Kosuka)" )
