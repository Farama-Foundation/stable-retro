/***************************************************************************

Buggy Challenge - (c) 1984 Taito Corporation

driver by Ernesto Corvi and Nicola Salmoria

TODO:
- I'm almost sure that I'm not handling the zoom x ROM table correctly. Gives
  reasonable results, though. I'm confident that the zoom y table handling is
  correct.
- Tilemap and sprite placement might not be accurate, there aren't many
  references.
- The way I'm handling opaqueness in the top portion of the screen is definitely
  wrong (see the high score entry screen). Actually there doesn't seem to be
  a way to make the fg opaque, but not doing so leaves parts of the bg visible
  at the top of the screen.
- The gradient sky is completely wrong - it's more of a placeholder to show
  that it's supposed to be there. It is supposed to skew along with the
  background, and the gradient can move around (the latter doesn't seem to
  be used except for making it cover the whole screen on the title screen,
  and start at the middle during gameplay)
- Video driver is largely unoptimized
- Support for the 7630's controlling the sound chip outputs (bass/treble,
  volume) is completely missing.
- The sound Z80 seems to write answers for the main Z80, but the latter doesn't
  seem to read them.

Notes:
- There is also a 4-channel version of the sound board for the cockpit
  cabinet (ROMs not dumped)


Memory Map
----------
0000 - 3fff = ROM A22-04 (23)
4000 - 7fff = ROM A22-05 (22)
8000 - 87ff = RAM (36)
8800 - 8fff = RAM (35)

c800 - cbff = videoram
cc00 - cfff = videoram

d100 = /ANYOUT
	bit7 = lamp
	bit6 = lockout
	bit4 = OJMODE (sprite palette bank)
	bit3 = SKY OFF
	bit2 = /SN3OFF
	bit1 = flip screen X
	bit0 = flip screen Y
d200 = bank switch
	bit2 = Bank Select bit 1
	bit1 = Bank Select bit 0
	bit0 = EA13 (high/low part of banked ROM)
d300 = /TRESET (Watchdog reset?)
d301 = No name?
	bit6 = FLPF2 (W-6)
	bit5 = FLPE2 (W-5)
	bit4 = FLPD2 (W-4)
	bot2 = FLPF1 (W-3)
	bit1 = FLPE1 (W-2)
	bit0 = FLPD1 (W-1)
d302 - bit 0 = /RESET line on the 68705

d304 - d307 = SCCON1 to SCCON4

d613 = /SoundCS = /RESET line on all audio CPUs

d700 - d7ff = ( /VCRRQ - palette ram )

d800 - d8ff /ScrollRQ (S37)
da00 - daff /ScrollRQ (S37)
db00 - dbff /ScrollRQ (S37)

dcxx = /SPOSI (S36)


***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "cpu/z80/z80.h"


/* in machine */
READ_HANDLER( buggychl_68705_portA_r );
WRITE_HANDLER( buggychl_68705_portA_w );
WRITE_HANDLER( buggychl_68705_ddrA_w );
READ_HANDLER( buggychl_68705_portB_r );
WRITE_HANDLER( buggychl_68705_portB_w );
WRITE_HANDLER( buggychl_68705_ddrB_w );
READ_HANDLER( buggychl_68705_portC_r );
WRITE_HANDLER( buggychl_68705_portC_w );
WRITE_HANDLER( buggychl_68705_ddrC_w );
WRITE_HANDLER( buggychl_mcu_w );
READ_HANDLER( buggychl_mcu_r );
READ_HANDLER( buggychl_mcu_status_r );

/* in vidhrdw */
extern unsigned char *buggychl_scrollv,*buggychl_scrollh;
extern unsigned char buggychl_sprite_lookup[0x2000];
extern unsigned char *buggychl_character_ram;

PALETTE_INIT( buggychl );
VIDEO_START( buggychl );
WRITE_HANDLER( buggychl_chargen_w );
WRITE_HANDLER( buggychl_sprite_lookup_bank_w );
WRITE_HANDLER( buggychl_sprite_lookup_w );
WRITE_HANDLER( buggychl_ctrl_w );
WRITE_HANDLER( buggychl_bg_scrollx_w );
VIDEO_UPDATE( buggychl );




static WRITE_HANDLER( bankswitch_w )
{
	cpu_setbank(1,&memory_region(REGION_CPU1)[0x10000 + (data & 7) * 0x2000]);
}


static int sound_nmi_enable,pending_nmi;

static void nmi_callback(int param)
{
	if (sound_nmi_enable) cpu_set_irq_line(1,IRQ_LINE_NMI,PULSE_LINE);
	else pending_nmi = 1;
}

static WRITE_HANDLER( sound_command_w )
{
	soundlatch_w(0,data);
	timer_set(TIME_NOW,data,nmi_callback);
}

static WRITE_HANDLER( nmi_disable_w )
{
	sound_nmi_enable = 0;
}

static WRITE_HANDLER( nmi_enable_w )
{
	sound_nmi_enable = 1;
	if (pending_nmi)
	{
		cpu_set_irq_line(1,IRQ_LINE_NMI,PULSE_LINE);
		pending_nmi = 0;
	}
}

static WRITE_HANDLER( sound_enable_w )
{
	mixer_sound_enable_global_w(data & 1);
}



static MEMORY_READ_START( readmem )
	{ 0x0000, 0x3fff, MRA_ROM }, /* A22-04 (23) */
	{ 0x4000, 0x7fff, MRA_ROM }, /* A22-05 (22) */
	{ 0x8000, 0x87ff, MRA_RAM }, /* 6116 SRAM (36) */
	{ 0x8800, 0x8fff, MRA_RAM }, /* 6116 SRAM (35) */
	{ 0xa000, 0xbfff, MRA_BANK1 },
	{ 0xc800, 0xcfff, videoram_r },
	{ 0xd400, 0xd400, buggychl_mcu_r },
	{ 0xd401, 0xd401, buggychl_mcu_status_r },
	{ 0xd600, 0xd600, input_port_0_r },	/* dsw */
	{ 0xd601, 0xd601, input_port_1_r },	/* dsw */
	{ 0xd602, 0xd602, input_port_2_r },	/* dsw */
	{ 0xd603, 0xd603, input_port_3_r },	/* player inputs */
	{ 0xd608, 0xd608, input_port_4_r },	/* wheel */
	{ 0xd609, 0xd609, input_port_5_r },	/* coin + accelerator */
/*	{ 0xd60a, 0xd60a, other inputs, not used?*/
/*	{ 0xd60b, 0xd60b, other inputs, not used?*/
MEMORY_END

static MEMORY_WRITE_START( writemem )
	{ 0x0000, 0x3fff, MWA_ROM }, /* A22-04 (23) */
	{ 0x4000, 0x7fff, MWA_ROM }, /* A22-05 (22) */
	{ 0x8000, 0x87ff, MWA_RAM }, /* 6116 SRAM (36) */
	{ 0x8800, 0x8fff, MWA_RAM }, /* 6116 SRAM (35) */
	{ 0x9000, 0x9fff, buggychl_sprite_lookup_w },
	{ 0xa000, 0xbfff, buggychl_chargen_w, &buggychl_character_ram },
	{ 0xc800, 0xcfff, videoram_w, &videoram, &videoram_size },
/*	{ 0xd000, 0xd000, horizon*/
	{ 0xd100, 0xd100, buggychl_ctrl_w },
	{ 0xd200, 0xd200, bankswitch_w },
	{ 0xd300, 0xd300, watchdog_reset_w },
/*	{ 0xd301, 0xd301,*/
/*	{ 0xd302, 0xd302, reset mcu*/
	{ 0xd303, 0xd303, buggychl_sprite_lookup_bank_w },
/*	{ 0xd304, 0xd307, sccon 1-4*/
	{ 0xd400, 0xd400, buggychl_mcu_w },
	{ 0xd500, 0xd57f, MWA_RAM, &spriteram, &spriteram_size },
	{ 0xd610, 0xd610, sound_command_w },
/*	{ 0xd613, 0xd613, reset sound cpu & sound chips*/
	{ 0xd618, 0xd618, MWA_NOP },	/* accelerator clear */
	{ 0xd700, 0xd7ff, paletteram_xxxxRRRRGGGGBBBB_swap_w, &paletteram },
	{ 0xd840, 0xd85f, MWA_RAM, &buggychl_scrollv },
	{ 0xdb00, 0xdbff, MWA_RAM, &buggychl_scrollh },
	{ 0xdc04, 0xdc04, MWA_RAM },	/* should be fg scroll */
	{ 0xdc06, 0xdc06, buggychl_bg_scrollx_w },
MEMORY_END

static MEMORY_READ_START( sound_readmem )
	{ 0x0000, 0x3fff, MRA_ROM },
	{ 0x4000, 0x47ff, MRA_RAM },
	{ 0x5000, 0x5000, soundlatch_r },
/*	{ 0x5001, 0x5001, MRA_RAM },	 // is command pending? /*/
	{ 0xe000, 0xefff, MRA_ROM },	/* space for diagnostics ROM */
MEMORY_END

static MEMORY_WRITE_START( sound_writemem )
	{ 0x0000, 0x3fff, MWA_ROM },
	{ 0x4000, 0x47ff, MWA_RAM },
	{ 0x4800, 0x4800, AY8910_control_port_0_w },
	{ 0x4801, 0x4801, AY8910_write_port_0_w },
	{ 0x4802, 0x4802, AY8910_control_port_1_w },
	{ 0x4803, 0x4803, AY8910_write_port_1_w },
	{ 0x4810, 0x481d, MSM5232_0_w },
	{ 0x4820, 0x4820, MWA_RAM },	/* VOL/BAL   for the 7630 on the MSM5232 output */
	{ 0x4830, 0x4830, MWA_RAM },	/* TRBL/BASS for the 7630 on the MSM5232 output  */
/*	{ 0x5000, 0x5000, MWA_RAM },	 // to main cpu /*/
	{ 0x5001, 0x5001, nmi_enable_w },
	{ 0x5002, 0x5002, nmi_disable_w },
	{ 0x5003, 0x5003, sound_enable_w },
	{ 0xe000, 0xefff, MWA_ROM },
MEMORY_END

static MEMORY_READ_START( mcu_readmem )
	{ 0x0000, 0x0000, buggychl_68705_portA_r },
	{ 0x0001, 0x0001, buggychl_68705_portB_r },
	{ 0x0002, 0x0002, buggychl_68705_portC_r },
	{ 0x0010, 0x007f, MRA_RAM },
	{ 0x0080, 0x07ff, MRA_ROM },
MEMORY_END

static MEMORY_WRITE_START( mcu_writemem )
	{ 0x0000, 0x0000, buggychl_68705_portA_w },
	{ 0x0001, 0x0001, buggychl_68705_portB_w },
	{ 0x0002, 0x0002, buggychl_68705_portC_w },
	{ 0x0004, 0x0004, buggychl_68705_ddrA_w },
	{ 0x0005, 0x0005, buggychl_68705_ddrB_w },
	{ 0x0006, 0x0006, buggychl_68705_ddrC_w },
	{ 0x0010, 0x007f, MWA_RAM },
	{ 0x0080, 0x07ff, MWA_ROM },
MEMORY_END

/******************************************************************************/

INPUT_PORTS_START( buggychl )
	PORT_START	/* IN0 */
	PORT_DIPNAME( 0x03, 0x03, "Game Over Bonus" )	/* Arks/Flags/Fuel*/
	PORT_DIPSETTING(    0x03, "2000/1000/50" )
	PORT_DIPSETTING(    0x02, "1000/500/30" )
	PORT_DIPSETTING(    0x01, "500/200/10" )
	PORT_DIPSETTING(    0x00, "None" )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Free_Play ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x18, 0x18, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x18, "Easy" )			/* 1300 units of fuel*/
	PORT_DIPSETTING(    0x10, "Normal" )		/* 1200 units of fuel*/
	PORT_DIPSETTING(    0x08, "Hard" )			/* 1100 units of fuel*/
	PORT_DIPSETTING(    0x00, "Hardest" )		/* 1000 units of fuel*/
	PORT_SERVICE( 0x20, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START	/* IN1 */
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

	PORT_START /* IN2 */
	PORT_DIPNAME( 0x01, 0x01, "Start button needed" )
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Yes ) )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BITX(    0x04, 0x04, IPT_DIPSWITCH_NAME | IPF_CHEAT, "Fuel loss", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(    0x04, "Normal" )
	PORT_DIPSETTING(    0x00, "Crash only" )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_DIPNAME( 0x10, 0x10, "Coinage Display" )
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x20, 0x20, "Year Display" )
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Yes ) )
	PORT_BITX(    0x40, 0x40, IPT_DIPSWITCH_NAME | IPF_CHEAT, "Invulnerability", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, "Coin Slots" )
	PORT_DIPSETTING(    0x00, "1" )
	PORT_DIPSETTING(    0x80, "2" )

	PORT_START /* IN3 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_BUTTON2 )	/* shift */
	PORT_BITX(0x10, IP_ACTIVE_HIGH, IPT_SERVICE, "Test Button", KEYCODE_F1, IP_JOY_NONE )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START /* IN4 - wheel */
	PORT_ANALOG( 0xff, 0x00, IPT_DIAL | IPF_REVERSE, 30, 15, 0, 0)

	PORT_START /* IN5 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_SERVICE1 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_TILT )
	PORT_BIT( 0xf0, IP_ACTIVE_HIGH, IPT_BUTTON1 )	/* accelerator */
INPUT_PORTS_END



static struct GfxLayout charlayout =
{
	8,8,
	256,
	4,
	{ 3*0x800*8, 2*0x800*8, 0x800*8, 0 },
	{ 7, 6, 5, 4, 3, 2, 1, 0 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8
};

static struct GfxLayout spritelayout =
{
	16,1,
	RGN_FRAC(1,8),
	4,
	{ RGN_FRAC(3,4), RGN_FRAC(2,4), RGN_FRAC(1,4), RGN_FRAC(0,4) },
	{ RGN_FRAC(1,8)+7, RGN_FRAC(1,8)+6, RGN_FRAC(1,8)+5, RGN_FRAC(1,8)+4, RGN_FRAC(1,8)+3, RGN_FRAC(1,8)+2, RGN_FRAC(1,8)+1, RGN_FRAC(1,8)+0,
			7, 6, 5, 4, 3, 2, 1, 0 },
	{ 0 },
	8
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ 0,           0, &charlayout,   0, 8 }, /* decoded at runtime */
	/* sprites are drawn pixel by pixel by draw_sprites() */
	{ REGION_GFX1, 0, &spritelayout, 0, 8 },
	{ -1 } /* end of array */
};



static WRITE_HANDLER( portA_0_w )
{
	/* VOL/BAL   for the 7630 on this 8910 output */
}
static WRITE_HANDLER( portB_0_w )
{
	/* TRBL/BASS for the 7630 on this 8910 output */
}
static WRITE_HANDLER( portA_1_w )
{
	/* VOL/BAL   for the 7630 on this 8910 output */
}
static WRITE_HANDLER( portB_1_w )
{
	/* TRBL/BASS for the 7630 on this 8910 output */
}


static struct AY8910interface ay8910_interface =
{
	2,	/* 2 chips */
	8000000/4,	/* 2 MHz */
	{ 30, 30 },
	{ 0 },
	{ 0 },
	{ portA_0_w, portA_1_w },
	{ portB_0_w, portB_1_w }
};

static struct MSM5232interface msm5232_interface =
{
	1, /* number of chips */
	2000000, /* 2 MHz ? */
	{ { 0.39e-6, 0.39e-6, 0.39e-6, 0.39e-6, 0.39e-6, 0.39e-6, 0.39e-6, 0.39e-6 } },	/* default 0.39 uF capacitors (not verified) */
	{ 100 } /* ? */
};


static MACHINE_DRIVER_START( buggychl )

	/* basic machine hardware */
	MDRV_CPU_ADD(Z80, 4000000) /* 4 MHz??? */
	MDRV_CPU_MEMORY(readmem,writemem)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,1)

	MDRV_CPU_ADD(Z80, 4000000)
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU) /* 4 MHz??? */
	MDRV_CPU_MEMORY(sound_readmem,sound_writemem)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,60)	/* irq is timed, tied to the cpu clock and not to vblank */
							/* nmi is caused by the main cpu */

	MDRV_CPU_ADD(M68705,8000000/2)  /* 4 MHz */
	MDRV_CPU_MEMORY(mcu_readmem,mcu_writemem)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 2*8, 30*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(128+128)
	MDRV_COLORTABLE_LENGTH(128)

	MDRV_PALETTE_INIT(buggychl)
	MDRV_VIDEO_START(buggychl)
	MDRV_VIDEO_UPDATE(buggychl)

	/* sound hardware */
	MDRV_SOUND_ADD(AY8910, ay8910_interface)
	MDRV_SOUND_ADD(MSM5232, msm5232_interface)
MACHINE_DRIVER_END

/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( buggychl )
	ROM_REGION( 0x1c000, REGION_CPU1, 0 )  /* 64k for code */
	ROM_LOAD( "a22-04-2.23", 0x00000, 0x4000, CRC(16445a6a) SHA1(5ce7b0b1aeb3b6cd400965467f913558f39c251f) )
	ROM_LOAD( "a22-05-2.22", 0x04000, 0x4000, CRC(d57430b2) SHA1(3e5b8c21a342d8e26c12a78535748073bc5b8742) )
	ROM_LOAD( "a22-01.3",    0x10000, 0x4000, CRC(af3b7554) SHA1(fd4f5a6cf9253f64c7e86d566802a02baae3b379) ) /* banked */
	ROM_LOAD( "a22-02.2",    0x14000, 0x4000, CRC(b8a645fb) SHA1(614a0656dee0cfa1d7e16ec1e0138a423ecaf18b) ) /* banked */
	ROM_LOAD( "a22-03.1",    0x18000, 0x4000, CRC(5f45d469) SHA1(3a1b9ab2d57c06bfffb1271583944c90d3f6b5a2) ) /* banked */

	ROM_REGION( 0x10000, REGION_CPU2, 0 )  /* sound Z80 */
	ROM_LOAD( "a22-24.28",   0x00000, 0x4000, CRC(1e7f841f) SHA1(2dc0787b08d32acb78291b689c02dbb83d04d08c) )

	ROM_REGION( 0x0800, REGION_CPU3, 0 )	/* 8k for the microcontroller */
	ROM_LOAD( "a22-19.31",   0x00000, 0x0800, CRC(06a71df0) SHA1(28183e6769e1471e7f28dc2a9f5b54e14b7ef339) )

	ROM_REGION( 0x20000, REGION_GFX1, ROMREGION_DISPOSE )	/* sprites */
	ROM_LOAD( "a22-06.111",  0x00000, 0x4000, CRC(1df91b17) SHA1(440d33bf984042fb4eac8f17bb385992ccdc6113) )
	ROM_LOAD( "a22-07.110",  0x04000, 0x4000, CRC(2f0ab9b7) SHA1(07b98e23d12da834d522e29fe7891503dc258b05) )
	ROM_LOAD( "a22-08.109",  0x08000, 0x4000, CRC(49cb2134) SHA1(f9998617c097b90be7257ba6fc1e46ff9e1f8916) )
	ROM_LOAD( "a22-09.108",  0x0c000, 0x4000, CRC(e682e200) SHA1(3e2b5dd97e4f522f208d331f6903c69d49555b61) )
	ROM_LOAD( "a22-10.107",  0x10000, 0x4000, CRC(653b7e25) SHA1(70c69288438caf6725c6d96ff75cdc82af005b2b) )
	ROM_LOAD( "a22-11.106",  0x14000, 0x4000, CRC(8057b55c) SHA1(9eeb4980cb14fb1c9b6f3aeff4e0aef1338fc71c) )
	ROM_LOAD( "a22-12.105",  0x18000, 0x4000, CRC(8b365b24) SHA1(a306c1f6fe1f5563602ab424f1b4f6ac17d1e47d) )
	ROM_LOAD( "a22-13.104",  0x1c000, 0x4000, CRC(2c6d68fe) SHA1(9e1a0e44ae2b9986d0ebff49a0fd4df3e8a7f4e7) )

	ROM_REGION( 0x4000, REGION_GFX2, 0 )	/* sprite zoom tables */
	ROM_LOAD( "a22-14.59",   0x0000, 0x2000, CRC(a450b3ef) SHA1(42646bfaed19ea01ffe06996bb6c2fd6c70076d6) )	/* vertical */
	ROM_LOAD( "a22-15.115",  0x2000, 0x1000, CRC(337a0c14) SHA1(2aa6814f74497c5c55bf7098d7f6f5508845e36c) )	/* horizontal */
	ROM_LOAD( "a22-16.116",  0x3000, 0x1000, CRC(337a0c14) SHA1(2aa6814f74497c5c55bf7098d7f6f5508845e36c) )	/* horizontal */
ROM_END

ROM_START( buggycht )
	ROM_REGION( 0x1c000, REGION_CPU1, 0 )  /* 64k for code */
	ROM_LOAD( "bu04.bin",    0x00000, 0x4000, CRC(f90ab854) SHA1(d4536c98be35de3d888548e2de15f8435ca4f08c) )
	ROM_LOAD( "bu05.bin",    0x04000, 0x4000, CRC(543d0949) SHA1(b7b0b0319f5376e7cfcfd0e8a4fa6fea566e0206) )
	ROM_LOAD( "a22-01.3",    0x10000, 0x4000, CRC(af3b7554) SHA1(fd4f5a6cf9253f64c7e86d566802a02baae3b379) ) /* banked */
	ROM_LOAD( "a22-02.2",    0x14000, 0x4000, CRC(b8a645fb) SHA1(614a0656dee0cfa1d7e16ec1e0138a423ecaf18b) ) /* banked */
	ROM_LOAD( "a22-03.1",    0x18000, 0x4000, CRC(5f45d469) SHA1(3a1b9ab2d57c06bfffb1271583944c90d3f6b5a2) ) /* banked */

	ROM_REGION( 0x10000, REGION_CPU2, 0 )  /* sound Z80 */
	ROM_LOAD( "a22-24.28",   0x00000, 0x4000, CRC(1e7f841f) SHA1(2dc0787b08d32acb78291b689c02dbb83d04d08c) )

	ROM_REGION( 0x0800, REGION_CPU3, 0 )	/* 8k for the microcontroller */
	ROM_LOAD( "a22-19.31",   0x00000, 0x0800, CRC(06a71df0) SHA1(28183e6769e1471e7f28dc2a9f5b54e14b7ef339) )

	ROM_REGION( 0x20000, REGION_GFX1, ROMREGION_DISPOSE )	/* sprites */
	ROM_LOAD( "a22-06.111",  0x00000, 0x4000, CRC(1df91b17) SHA1(440d33bf984042fb4eac8f17bb385992ccdc6113) )
	ROM_LOAD( "a22-07.110",  0x04000, 0x4000, CRC(2f0ab9b7) SHA1(07b98e23d12da834d522e29fe7891503dc258b05) )
	ROM_LOAD( "a22-08.109",  0x08000, 0x4000, CRC(49cb2134) SHA1(f9998617c097b90be7257ba6fc1e46ff9e1f8916) )
	ROM_LOAD( "a22-09.108",  0x0c000, 0x4000, CRC(e682e200) SHA1(3e2b5dd97e4f522f208d331f6903c69d49555b61) )
	ROM_LOAD( "a22-10.107",  0x10000, 0x4000, CRC(653b7e25) SHA1(70c69288438caf6725c6d96ff75cdc82af005b2b) )
	ROM_LOAD( "a22-11.106",  0x14000, 0x4000, CRC(8057b55c) SHA1(9eeb4980cb14fb1c9b6f3aeff4e0aef1338fc71c) )
	ROM_LOAD( "a22-12.105",  0x18000, 0x4000, CRC(8b365b24) SHA1(a306c1f6fe1f5563602ab424f1b4f6ac17d1e47d) )
	ROM_LOAD( "a22-13.104",  0x1c000, 0x4000, CRC(2c6d68fe) SHA1(9e1a0e44ae2b9986d0ebff49a0fd4df3e8a7f4e7) )

	ROM_REGION( 0x4000, REGION_GFX2, 0 )	/* sprite zoom tables */
	ROM_LOAD( "a22-14.59",   0x0000, 0x2000, CRC(a450b3ef) SHA1(42646bfaed19ea01ffe06996bb6c2fd6c70076d6) )	/* vertical */
	ROM_LOAD( "a22-15.115",  0x2000, 0x1000, CRC(337a0c14) SHA1(2aa6814f74497c5c55bf7098d7f6f5508845e36c) )	/* horizontal */
	ROM_LOAD( "a22-16.116",  0x3000, 0x1000, CRC(337a0c14) SHA1(2aa6814f74497c5c55bf7098d7f6f5508845e36c) )	/* horizontal */
ROM_END


GAMEX( 1984, buggychl, 0,        buggychl, buggychl, 0, ROT270, "Taito Corporation", "Buggy Challenge", GAME_IMPERFECT_SOUND | GAME_IMPERFECT_GRAPHICS )
GAMEX( 1984, buggycht, buggychl, buggychl, buggychl, 0, ROT270, "Taito Corporation (Tecfri license)", "Buggy Challenge (Tecfri)", GAME_IMPERFECT_SOUND | GAME_IMPERFECT_GRAPHICS )
