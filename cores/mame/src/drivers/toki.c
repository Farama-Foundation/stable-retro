/***************************************************************************

Toki

driver by Jarek Parchanski


Coin inputs are handled by the sound CPU, so they don't work with sound
disabled. Use the service switch instead.


TODO
----

Does the bootleg use a 68000 @ 10MHz ? This causes some bad slow-
downs at the floating monkey machine (round 1), so set to 12 MHz
for now. Even at 12 this slowdown still happens a little.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "cpu/z80/z80.h"
#include "sndhrdw/seibu.h"

extern data16_t *toki_background1_videoram16;
extern data16_t *toki_background2_videoram16;
extern data16_t *toki_sprites_dataram16;
extern data16_t *toki_scrollram16;

INTERRUPT_GEN( toki_interrupt );
VIDEO_START( toki );
VIDEO_EOF( toki );
VIDEO_EOF( tokib );
VIDEO_UPDATE( toki );
VIDEO_UPDATE( tokib );
WRITE16_HANDLER( toki_background1_videoram16_w );
WRITE16_HANDLER( toki_background2_videoram16_w );
WRITE16_HANDLER( toki_control_w );
WRITE16_HANDLER( toki_foreground_videoram16_w );


static WRITE16_HANDLER( tokib_soundcommand16_w )
{
	soundlatch_w(0,data & 0xff);
	cpu_set_irq_line(1, 0, HOLD_LINE);
}

static READ16_HANDLER( pip16_r )
{
	return ~0;
}


static int msm5205next;

static void toki_adpcm_int (int data)
{
	static int toggle=0;

	MSM5205_data_w (0,msm5205next);
	msm5205next>>=4;

	toggle ^= 1;
	if (toggle)
		cpu_set_nmi_line(1, PULSE_LINE);
}

static WRITE_HANDLER( toki_adpcm_control_w )
{
	int bankaddress;
	unsigned char *RAM = memory_region(REGION_CPU2);


	/* the code writes either 2 or 3 in the bottom two bits */
	bankaddress = 0x10000 + (data & 0x01) * 0x4000;
	cpu_setbank(1,&RAM[bankaddress]);

	MSM5205_reset_w(0,data & 0x08);
}

static WRITE_HANDLER( toki_adpcm_data_w )
{
	msm5205next = data;
}


/*****************************************************************************/

static MEMORY_READ16_START( toki_readmem )
	{ 0x000000, 0x05ffff, MRA16_ROM },
	{ 0x060000, 0x06d7ff, MRA16_RAM },
	{ 0x06d800, 0x06dfff, MRA16_RAM },
	{ 0x06e000, 0x06e7ff, MRA16_RAM },
	{ 0x06e800, 0x06efff, MRA16_RAM },
	{ 0x06f000, 0x06f7ff, MRA16_RAM },
	{ 0x06f800, 0x06ffff, MRA16_RAM },
	{ 0x080000, 0x08000d, seibu_main_word_r },
	{ 0x0c0000, 0x0c0001, input_port_1_word_r },
	{ 0x0c0002, 0x0c0003, input_port_2_word_r },
	{ 0x0c0004, 0x0c0005, input_port_3_word_r },
MEMORY_END

static MEMORY_WRITE16_START( toki_writemem )
	{ 0x000000, 0x05ffff, MWA16_ROM },
	{ 0x060000, 0x06d7ff, MWA16_RAM },
	{ 0x06d800, 0x06dfff, MWA16_RAM, &spriteram16, &spriteram_size },
	{ 0x06e000, 0x06e7ff, paletteram16_xxxxBBBBGGGGRRRR_word_w, &paletteram16 },
	{ 0x06e800, 0x06efff, toki_background1_videoram16_w, &toki_background1_videoram16 },
	{ 0x06f000, 0x06f7ff, toki_background2_videoram16_w, &toki_background2_videoram16 },
	{ 0x06f800, 0x06ffff, toki_foreground_videoram16_w, &videoram16 },
	{ 0x080000, 0x08000d, seibu_main_word_w },
	{ 0x0a0000, 0x0a005f, toki_control_w, &toki_scrollram16 },
MEMORY_END

/* In the bootleg, sound and sprites are remapped to 0x70000 */
static MEMORY_READ16_START( tokib_readmem )
	{ 0x000000, 0x05ffff, MRA16_ROM },
	{ 0x060000, 0x06dfff, MRA16_RAM },
	{ 0x06e000, 0x06e7ff, MRA16_RAM },
	{ 0x06e800, 0x06efff, MRA16_RAM },
	{ 0x06f000, 0x06f7ff, MRA16_RAM },
	{ 0x06f800, 0x06ffff, MRA16_RAM },
	{ 0x072000, 0x072001, watchdog_reset16_r },   /* probably */
	{ 0x0c0000, 0x0c0001, input_port_0_word_r },
	{ 0x0c0002, 0x0c0003, input_port_1_word_r },
	{ 0x0c0004, 0x0c0005, input_port_2_word_r },
	{ 0x0c000e, 0x0c000f, pip16_r },  /* sound related, if we return 0 the code writes */
				/* the sound command quickly followed by 0 and the */
				/* sound CPU often misses the command. */
MEMORY_END

static MEMORY_WRITE16_START( tokib_writemem )
	{ 0x000000, 0x05ffff, MWA16_ROM },
	{ 0x060000, 0x06dfff, MWA16_RAM },
	{ 0x06e000, 0x06e7ff, paletteram16_xxxxBBBBGGGGRRRR_word_w, &paletteram16 },
	{ 0x06e800, 0x06efff, toki_background1_videoram16_w, &toki_background1_videoram16 },
	{ 0x06f000, 0x06f7ff, toki_background2_videoram16_w, &toki_background2_videoram16 },
	{ 0x06f800, 0x06ffff, toki_foreground_videoram16_w, &videoram16 },
	{ 0x071000, 0x071001, MWA16_NOP },	/* sprite related? seems another scroll register */
				/* gets written the same value as 75000a (bg2 scrollx) */
	{ 0x071804, 0x071807, MWA16_NOP },	/* sprite related, always 01be0100 */
	{ 0x07180e, 0x071e45, MWA16_RAM, &spriteram16, &spriteram_size },
	{ 0x075000, 0x075001, tokib_soundcommand16_w },
	{ 0x075004, 0x07500b, MWA16_RAM, &toki_scrollram16 },
MEMORY_END

/*****************************************************************************/

static MEMORY_READ_START( tokib_sound_readmem )
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0xbfff, MRA_BANK1 },
	{ 0xec00, 0xec00, YM3812_status_port_0_r },
	{ 0xf000, 0xf7ff, MRA_RAM },
	{ 0xf800, 0xf800, soundlatch_r },
MEMORY_END

static MEMORY_WRITE_START( tokib_sound_writemem )
	{ 0x0000, 0xbfff, MWA_ROM },
	{ 0xe000, 0xe000, toki_adpcm_control_w },	/* MSM5205 + ROM bank */
	{ 0xe400, 0xe400, toki_adpcm_data_w },
	{ 0xec00, 0xec00, YM3812_control_port_0_w },
	{ 0xec01, 0xec01, YM3812_write_port_0_w },
	{ 0xec08, 0xec08, YM3812_control_port_0_w },	/* mirror address, it seems */
	{ 0xec09, 0xec09, YM3812_write_port_0_w },	/* mirror address, it seems */
	{ 0xf000, 0xf7ff, MWA_RAM },
MEMORY_END

/*****************************************************************************/

INPUT_PORTS_START( toki )
	SEIBU_COIN_INPUTS	/* Must be port 0: coin inputs read through sound cpu */

	PORT_START
	PORT_DIPNAME( 0x001f, 0x001f, DEF_STR( Coinage ) )
	PORT_DIPSETTING(      0x0015, DEF_STR( 6C_1C ) )
	PORT_DIPSETTING(      0x0017, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(      0x0019, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(      0x001b, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0003, DEF_STR( 8C_3C ) )
	PORT_DIPSETTING(      0x001d, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0005, DEF_STR( 5C_3C ) )
	PORT_DIPSETTING(      0x0007, DEF_STR( 3C_2C ) )
	PORT_DIPSETTING(      0x001f, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0009, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(      0x0013, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x0011, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x000f, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(      0x000d, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(      0x000b, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(      0x001e, "A 1/1 B 1/2" )
	PORT_DIPSETTING(      0x0014, "A 2/1 B 1/3" )
	PORT_DIPSETTING(      0x000a, "A 3/1 B 1/5" )
	PORT_DIPSETTING(      0x0000, "A 5/1 B 1/6" )
	PORT_DIPSETTING(      0x0001, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0x0020, 0x0000, "Joysticks" )
	PORT_DIPSETTING(      0x0020, "1" )
	PORT_DIPSETTING(      0x0000, "2" )
	PORT_DIPNAME( 0x0040, 0x0040, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(      0x0040, DEF_STR( Upright ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x0080, 0x0080, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(      0x0080, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0300, 0x0300, DEF_STR( Lives ) )
	PORT_DIPSETTING(      0x0200, "2" )
	PORT_DIPSETTING(      0x0300, "3" )
	PORT_DIPSETTING(      0x0100, "5" )
	PORT_BITX( 0,         0x0000, IPT_DIPSWITCH_SETTING | IPF_CHEAT, "Infinite", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPNAME( 0x0c00, 0x0c00, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(      0x0800, "50000 150000" )
	PORT_DIPSETTING(      0x0000, "70000 140000 210000" )
	PORT_DIPSETTING(      0x0c00, "70000" )
	PORT_DIPSETTING(      0x0400, "100000 200000" )
	PORT_DIPNAME( 0x3000, 0x3000, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(      0x2000, "Easy" )
	PORT_DIPSETTING(      0x3000, "Medium" )
	PORT_DIPSETTING(      0x1000, "Hard" )
	PORT_DIPSETTING(      0x0000, "Hardest" )
	PORT_DIPNAME( 0x4000, 0x4000, "Allow Continue" )
	PORT_DIPSETTING(      0x0000, DEF_STR( No ) )
	PORT_DIPSETTING(      0x4000, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x8000, 0x8000, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x8000, DEF_STR( On ) )

	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW,  IPT_JOYSTICK_UP    | IPF_8WAY )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW,  IPT_JOYSTICK_DOWN  | IPF_8WAY )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW,  IPT_JOYSTICK_LEFT  | IPF_8WAY )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW,  IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW,  IPT_BUTTON1 )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW,  IPT_BUTTON2 )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW,  IPT_JOYSTICK_UP    | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW,  IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW,  IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW,  IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW,  IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW,  IPT_BUTTON2 | IPF_COCKTAIL )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW,  IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END

INPUT_PORTS_START( tokib )
	PORT_START
	PORT_DIPNAME( 0x001f, 0x001f, DEF_STR( Coinage ) )
	PORT_DIPSETTING(      0x0015, DEF_STR( 6C_1C ) )
	PORT_DIPSETTING(      0x0017, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(      0x0019, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(      0x001b, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0003, DEF_STR( 8C_3C ) )
	PORT_DIPSETTING(      0x001d, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0005, DEF_STR( 5C_3C ) )
	PORT_DIPSETTING(      0x0007, DEF_STR( 3C_2C ) )
	PORT_DIPSETTING(      0x001f, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0009, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(      0x0013, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x0011, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x000f, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(      0x000d, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(      0x000b, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(      0x001e, "A 1/1 B 1/2" )
	PORT_DIPSETTING(      0x0014, "A 2/1 B 1/3" )
	PORT_DIPSETTING(      0x000a, "A 3/1 B 1/5" )
	PORT_DIPSETTING(      0x0000, "A 5/1 B 1/6" )
	PORT_DIPSETTING(      0x0001, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0x0020, 0x0000, "Joysticks" )
	PORT_DIPSETTING(      0x0020, "1" )
	PORT_DIPSETTING(      0x0000, "2" )
	PORT_DIPNAME( 0x0040, 0x0040, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(      0x0040, DEF_STR( Upright ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x0080, 0x0080, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(      0x0080, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0300, 0x0300, DEF_STR( Lives ) )
	PORT_DIPSETTING(      0x0200, "2" )
	PORT_DIPSETTING(      0x0300, "3" )
	PORT_DIPSETTING(      0x0100, "5" )
	PORT_BITX( 0,         0x0000, IPT_DIPSWITCH_SETTING | IPF_CHEAT, "Infinite", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPNAME( 0x0c00, 0x0c00, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(      0x0800, "50000 150000" )
	PORT_DIPSETTING(      0x0000, "70000 140000 210000" )
	PORT_DIPSETTING(      0x0c00, "70000" )
	PORT_DIPSETTING(      0x0400, "100000 200000" )
	PORT_DIPNAME( 0x3000, 0x3000, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(      0x2000, "Easy" )
	PORT_DIPSETTING(      0x3000, "Medium" )
	PORT_DIPSETTING(      0x1000, "Hard" )
	PORT_DIPSETTING(      0x0000, "Hardest" )
	PORT_DIPNAME( 0x4000, 0x4000, "Allow Continue" )
	PORT_DIPSETTING(      0x0000, DEF_STR( No ) )
	PORT_DIPSETTING(      0x4000, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x8000, 0x8000, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x8000, DEF_STR( On ) )

	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW,  IPT_JOYSTICK_UP    | IPF_8WAY )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW,  IPT_JOYSTICK_DOWN  | IPF_8WAY )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW,  IPT_JOYSTICK_LEFT  | IPF_8WAY )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW,  IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW,  IPT_BUTTON1 )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW,  IPT_BUTTON2 )
	PORT_BIT( 0x0040, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x0080, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW,  IPT_JOYSTICK_UP    | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW,  IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW,  IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW,  IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW,  IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW,  IPT_BUTTON2 | IPF_COCKTAIL )
	PORT_BIT( 0x4000, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x8000, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW,  IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW,  IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW,  IPT_SERVICE1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW,  IPT_START1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW,  IPT_START2 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )
INPUT_PORTS_END


/*****************************************************************************/

static struct GfxLayout toki_charlayout =
{
	8,8,
	4096,
	4,
	{ 4096*16*8+0, 4096*16*8+4, 0, 4 },
	{ 3, 2, 1, 0, 8+3, 8+2, 8+1, 8+0 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16 },
	16*8
};

static struct GfxLayout toki_tilelayout =
{
	16,16,
	4096,
	4,
	{ 2*4, 3*4, 0*4, 1*4 },
	{ 3, 2, 1, 0, 16+3, 16+2, 16+1, 16+0,
			64*8+3, 64*8+2, 64*8+1, 64*8+0, 64*8+16+3, 64*8+16+2, 64*8+16+1, 64*8+16+0 },
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32,
			8*32, 9*32, 10*32, 11*32, 12*32, 13*32, 14*32, 15*32 },
	128*8
};

static struct GfxLayout toki_spritelayout =
{
	16,16,
	8192,
	4,
	{ 2*4, 3*4, 0*4, 1*4 },
	{ 3, 2, 1, 0, 16+3, 16+2, 16+1, 16+0,
			64*8+3, 64*8+2, 64*8+1, 64*8+0, 64*8+16+3, 64*8+16+2, 64*8+16+1, 64*8+16+0 },
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32,
			8*32, 9*32, 10*32, 11*32, 12*32, 13*32, 14*32, 15*32 },
	128*8
};

static struct GfxDecodeInfo toki_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &toki_charlayout,  16*16, 16 },
	{ REGION_GFX2, 0, &toki_spritelayout, 0*16, 16 },
	{ REGION_GFX3, 0, &toki_tilelayout,  32*16, 16 },
	{ REGION_GFX4, 0, &toki_tilelayout,  48*16, 16 },
	{ -1 } /* end of array */
};

static struct GfxLayout tokib_charlayout =
{
	8,8,	/* 8 by 8 */
	4096,	/* 4096 characters */
	4,	/* 4 bits per pixel */
	{4096*8*8*3,4096*8*8*2,4096*8*8*1,4096*8*8*0 }, /* planes */
	{ 0, 1,  2,  3,  4,  5,  6,  7},		/* x bit */
	{ 0, 8, 16, 24, 32, 40, 48, 56},		/* y bit */
	8*8
};

static struct GfxLayout tokib_tilelayout =
{
	16,16,	/* 16 by 16 */
	4096,	/* 4096 characters */
	4,	/* 4 bits per pixel */
	{ 4096*16*16*3,4096*16*16*2,4096*16*16*1,4096*16*16*0 },	/* planes */
	{ 0, 1, 2, 3, 4, 5, 6, 7,
	  0x8000*8+0, 0x8000*8+1, 0x8000*8+2, 0x8000*8+3, 0x8000*8+4,
	  0x8000*8+5, 0x8000*8+6, 0x8000*8+7 }, 			/* x bit */
	{
	  0,8,16,24,32,40,48,56,
	  0x10000*8+ 0, 0x10000*8+ 8, 0x10000*8+16, 0x10000*8+24, 0x10000*8+32,
	  0x10000*8+40, 0x10000*8+48, 0x10000*8+56 },			/* y bit */
	8*8
};

static struct GfxLayout tokib_spriteslayout =
{
	16,16,	/* 16 by 16 */
	8192,	/* 8192 sprites */
	4,	/* 4 bits per pixel */
	{ 8192*16*16*3,8192*16*16*2,8192*16*16*1,8192*16*16*0 },	/* planes */
	{	 0, 	1,	   2,	  3,	 4, 	5,	   6,	  7,
	 128+0, 128+1, 128+2, 128+3, 128+4, 128+5, 128+6, 128+7 },	/* x bit */
	{ 0,8,16,24,32,40,48,56,64,72,80,88,96,104,112,120 },		/* y bit */
	16*16
};

static struct GfxDecodeInfo tokib_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &tokib_charlayout,	16*16, 16 },
	{ REGION_GFX2, 0, &tokib_spriteslayout,  0*16, 16 },
	{ REGION_GFX3, 0, &tokib_tilelayout,	32*16, 16 },
	{ REGION_GFX4, 0, &tokib_tilelayout,	48*16, 16 },
	{ -1 } /* end of array */
};


/*****************************************************************************/

/* Parameters: YM3812 frequency, Oki frequency, Oki memory region */
SEIBU_SOUND_SYSTEM_YM3812_HARDWARE(14318180/4,8000,REGION_SOUND1);


static struct YM3812interface ym3812_tokib_interface =
{
	1,			/* 1 chip (no more supported) */
	3600000,	/* 3.600000 MHz ? (partially supported) */
	{ 100 }		/* (not supported) */
};

static struct MSM5205interface msm5205_interface =
{
	1,					/* 1 chip			  */
	384000, 			/* 384KHz			  */
	{ toki_adpcm_int },/* interrupt function */
	{ MSM5205_S96_4B },	/* 4KHz 			  */
	{ 50 }
};


static MACHINE_DRIVER_START( toki )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000,20000000/2) 	/* Accurate?  There is a 20MHz near the cpu, but a 12MHz elsewhere */
	MDRV_CPU_MEMORY(toki_readmem,toki_writemem)
	MDRV_CPU_VBLANK_INT(irq1_line_hold,1)/* VBL */

	SEIBU_SOUND_SYSTEM_CPU(14318180/4)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)
	MDRV_MACHINE_INIT(seibu_sound_1)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_BUFFERS_SPRITERAM)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 0*8, 30*8-1)
	MDRV_GFXDECODE(toki_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(1024)

	MDRV_VIDEO_START(toki)
	MDRV_VIDEO_EOF(toki)
	MDRV_VIDEO_UPDATE(toki)

	/* sound hardware */
	SEIBU_SOUND_SYSTEM_YM3812_INTERFACE
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( tokib )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 12000000)	/* 10MHz causes bad slowdowns with monkey machine rd1 */
	MDRV_CPU_MEMORY(tokib_readmem,tokib_writemem)
	MDRV_CPU_VBLANK_INT(irq6_line_hold,1)/* VBL (could be level1, same vector) */

	MDRV_CPU_ADD(Z80, 4000000)
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)	/* 4 MHz (?) */
	MDRV_CPU_MEMORY(tokib_sound_readmem,tokib_sound_writemem)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_BUFFERS_SPRITERAM)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 0*8, 30*8-1)
	MDRV_GFXDECODE(tokib_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(1024)

	MDRV_VIDEO_START(toki)
	MDRV_VIDEO_EOF(tokib)
	MDRV_VIDEO_UPDATE(tokib)

	/* sound hardware */
	MDRV_SOUND_ADD(YM3812, ym3812_tokib_interface)
	MDRV_SOUND_ADD(MSM5205, msm5205_interface)
MACHINE_DRIVER_END



/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( tokij )
	ROM_REGION( 0x60000, REGION_CPU1, 0 )	/* 6*64k for 68000 code */
	ROM_LOAD16_BYTE( "tokijp.006",   0x00000, 0x20000, CRC(03d726b1) SHA1(bbe3a1ea1943cd73b821b3de4d5bf3dfbffd2168) )
	ROM_LOAD16_BYTE( "tokijp.004",   0x00001, 0x20000, CRC(54a45e12) SHA1(240538c8b010bb6e1e7fea2ed2fb1d5f9bc64b2b) )
	ROM_LOAD16_BYTE( "tokijp.005",   0x40000, 0x10000, CRC(d6a82808) SHA1(9fcd3e97f7eaada5374347383dc8a6cea2378f7f) )
	ROM_LOAD16_BYTE( "tokijp.003",   0x40001, 0x10000, CRC(a01a5b10) SHA1(76d6da114105402aab9dd5167c0c00a0bddc3bba) )

	ROM_REGION( 0x20000*2, REGION_CPU2, 0 )	/* Z80 code, banked data */
	ROM_LOAD( "tokijp.008",   0x00000, 0x02000, CRC(6c87c4c5) SHA1(d76822bcde3d42afae72a0945b6acbf3c6a1d955) )	/* encrypted */
	ROM_LOAD( "tokijp.007",   0x10000, 0x10000, CRC(a67969c4) SHA1(99781fbb005b6ba4a19a9cc83c8b257a3b425fa6) )	/* banked stuff */

	ROM_REGION( 0x020000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "tokijp.001",   0x000000, 0x10000, CRC(8aa964a2) SHA1(875129bdd5f699ee30a98160718603a3bc958d84) )   /* chars */
	ROM_LOAD( "tokijp.002",   0x010000, 0x10000, CRC(86e87e48) SHA1(29634d8c58ef7195cd0ce166f1b7fae01bbc110b) )

	ROM_REGION( 0x100000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "toki.ob1",     0x000000, 0x80000, CRC(a27a80ba) SHA1(3dd3b6b0ace6ca6653603bea952b828b154a2223) )   /* sprites */
	ROM_LOAD( "toki.ob2",     0x080000, 0x80000, CRC(fa687718) SHA1(f194b742399d8124d97cfa3d59beb980c36cfb3c) )

	ROM_REGION( 0x080000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "toki.bk1",     0x000000, 0x80000, CRC(fdaa5f4b) SHA1(ea850361bc8274639e8433bd2a5307fd3a0c9a24) )   /* tiles 1 */

	ROM_REGION( 0x080000, REGION_GFX4, ROMREGION_DISPOSE )
	ROM_LOAD( "toki.bk2",     0x000000, 0x80000, CRC(d86ac664) SHA1(bcb64d8e7ad29b8201ebbada1f858075eb8a0f1d) )   /* tiles 2 */

	ROM_REGION( 0x20000, REGION_SOUND1, 0 )	/* ADPCM samples */
	ROM_LOAD( "tokijp.009",   0x00000, 0x20000, CRC(ae7a6b8b) SHA1(1d410f91354ffd1774896b2e64f20a2043607805) )
ROM_END

ROM_START( tokia )
	ROM_REGION( 0x60000, REGION_CPU1, 0 )	/* 6*64k for 68000 code */
	ROM_LOAD16_BYTE( "tokijp.006",   0x00000, 0x20000, CRC(03d726b1) SHA1(bbe3a1ea1943cd73b821b3de4d5bf3dfbffd2168) )
	ROM_LOAD16_BYTE( "4c.10k",       0x00001, 0x20000, CRC(b2c345c5) SHA1(ff8ff31551e835e29192d7ddd3e1601968b3e2c5) )
	ROM_LOAD16_BYTE( "tokijp.005",   0x40000, 0x10000, CRC(d6a82808) SHA1(9fcd3e97f7eaada5374347383dc8a6cea2378f7f) )
	ROM_LOAD16_BYTE( "tokijp.003",   0x40001, 0x10000, CRC(a01a5b10) SHA1(76d6da114105402aab9dd5167c0c00a0bddc3bba) )

	ROM_REGION( 0x20000*2, REGION_CPU2, 0 )	/* Z80 code, banked data */
	ROM_LOAD( "tokijp.008",   0x00000, 0x02000, CRC(6c87c4c5) SHA1(d76822bcde3d42afae72a0945b6acbf3c6a1d955) )	/* encrypted */
	ROM_LOAD( "tokijp.007",   0x10000, 0x10000, CRC(a67969c4) SHA1(99781fbb005b6ba4a19a9cc83c8b257a3b425fa6) )	/* banked stuff */

	ROM_REGION( 0x020000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "tokijp.001",   0x000000, 0x10000, CRC(8aa964a2) SHA1(875129bdd5f699ee30a98160718603a3bc958d84) )   /* chars */
	ROM_LOAD( "tokijp.002",   0x010000, 0x10000, CRC(86e87e48) SHA1(29634d8c58ef7195cd0ce166f1b7fae01bbc110b) )

	ROM_REGION( 0x100000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "toki.ob1",     0x000000, 0x80000, CRC(a27a80ba) SHA1(3dd3b6b0ace6ca6653603bea952b828b154a2223) )   /* sprites */
	ROM_LOAD( "toki.ob2",     0x080000, 0x80000, CRC(fa687718) SHA1(f194b742399d8124d97cfa3d59beb980c36cfb3c) )

	ROM_REGION( 0x080000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "toki.bk1",     0x000000, 0x80000, CRC(fdaa5f4b) SHA1(ea850361bc8274639e8433bd2a5307fd3a0c9a24) )   /* tiles 1 */

	ROM_REGION( 0x080000, REGION_GFX4, ROMREGION_DISPOSE )
	ROM_LOAD( "toki.bk2",     0x000000, 0x80000, CRC(d86ac664) SHA1(bcb64d8e7ad29b8201ebbada1f858075eb8a0f1d) )   /* tiles 2 */

	ROM_REGION( 0x20000, REGION_SOUND1, 0 )	/* ADPCM samples */
	ROM_LOAD( "tokijp.009",   0x00000, 0x20000, CRC(ae7a6b8b) SHA1(1d410f91354ffd1774896b2e64f20a2043607805) )
ROM_END

ROM_START( toki )
	ROM_REGION( 0x60000, REGION_CPU1, 0 )	/* 6*64k for 68000 code */
	ROM_LOAD16_BYTE( "l10_6.bin",    0x00000, 0x20000, CRC(94015d91) SHA1(8b8d7c589eff038467f55e81ffd450f726c5a8b5) )
	ROM_LOAD16_BYTE( "k10_4e.bin",   0x00001, 0x20000, CRC(531bd3ef) SHA1(2e561f92f5c5f2da16c4791274ccbd421b9b0a05) )
	ROM_LOAD16_BYTE( "tokijp.005",   0x40000, 0x10000, CRC(d6a82808) SHA1(9fcd3e97f7eaada5374347383dc8a6cea2378f7f) )
	ROM_LOAD16_BYTE( "tokijp.003",   0x40001, 0x10000, CRC(a01a5b10) SHA1(76d6da114105402aab9dd5167c0c00a0bddc3bba) )

	ROM_REGION( 0x20000*2, REGION_CPU2, 0 )	/* Z80 code, banked data */
	ROM_LOAD( "tokijp.008",   0x00000, 0x02000, CRC(6c87c4c5) SHA1(d76822bcde3d42afae72a0945b6acbf3c6a1d955) )	/* encrypted */
	ROM_LOAD( "tokijp.007",   0x10000, 0x10000, CRC(a67969c4) SHA1(99781fbb005b6ba4a19a9cc83c8b257a3b425fa6) )	/* banked stuff */

	ROM_REGION( 0x020000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "tokijp.001",   0x000000, 0x10000, CRC(8aa964a2) SHA1(875129bdd5f699ee30a98160718603a3bc958d84) )   /* chars */
	ROM_LOAD( "tokijp.002",   0x010000, 0x10000, CRC(86e87e48) SHA1(29634d8c58ef7195cd0ce166f1b7fae01bbc110b) )

	ROM_REGION( 0x100000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "toki.ob1",     0x000000, 0x80000, CRC(a27a80ba) SHA1(3dd3b6b0ace6ca6653603bea952b828b154a2223) )   /* sprites */
	ROM_LOAD( "toki.ob2",     0x080000, 0x80000, CRC(fa687718) SHA1(f194b742399d8124d97cfa3d59beb980c36cfb3c) )

	ROM_REGION( 0x080000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "toki.bk1",     0x000000, 0x80000, CRC(fdaa5f4b) SHA1(ea850361bc8274639e8433bd2a5307fd3a0c9a24) )   /* tiles 1 */

	ROM_REGION( 0x080000, REGION_GFX4, ROMREGION_DISPOSE )
	ROM_LOAD( "toki.bk2",     0x000000, 0x80000, CRC(d86ac664) SHA1(bcb64d8e7ad29b8201ebbada1f858075eb8a0f1d) )   /* tiles 2 */

	ROM_REGION( 0x20000, REGION_SOUND1, 0 )	/* ADPCM samples */
	ROM_LOAD( "tokijp.009",   0x00000, 0x20000, CRC(ae7a6b8b) SHA1(1d410f91354ffd1774896b2e64f20a2043607805) )
ROM_END

ROM_START( tokiu )
	ROM_REGION( 0x60000, REGION_CPU1, 0 )	/* 6*64k for 68000 code */
	ROM_LOAD16_BYTE( "6b.10m",       0x00000, 0x20000, CRC(3674d9fe) SHA1(7c610bee23b0f7e6a9e3d5d72d6084e025eb89ec) )
	ROM_LOAD16_BYTE( "14.10k",       0x00001, 0x20000, CRC(bfdd48af) SHA1(3e48375019471a51f0c00d3444b0c1d37d2f8e92) )
	ROM_LOAD16_BYTE( "tokijp.005",   0x40000, 0x10000, CRC(d6a82808) SHA1(9fcd3e97f7eaada5374347383dc8a6cea2378f7f) )
	ROM_LOAD16_BYTE( "tokijp.003",   0x40001, 0x10000, CRC(a01a5b10) SHA1(76d6da114105402aab9dd5167c0c00a0bddc3bba) )

	ROM_REGION( 0x20000*2, REGION_CPU2, 0 )	/* Z80 code, banked data */
	ROM_LOAD( "tokijp.008",   0x00000, 0x02000, CRC(6c87c4c5) SHA1(d76822bcde3d42afae72a0945b6acbf3c6a1d955) )	/* encrypted */
	ROM_LOAD( "tokijp.007",   0x10000, 0x10000, CRC(a67969c4) SHA1(99781fbb005b6ba4a19a9cc83c8b257a3b425fa6) )	/* banked stuff */

	ROM_REGION( 0x020000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "tokijp.001",   0x000000, 0x10000, CRC(8aa964a2) SHA1(875129bdd5f699ee30a98160718603a3bc958d84) )   /* chars */
	ROM_LOAD( "tokijp.002",   0x010000, 0x10000, CRC(86e87e48) SHA1(29634d8c58ef7195cd0ce166f1b7fae01bbc110b) )

	ROM_REGION( 0x100000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "toki.ob1",     0x000000, 0x80000, CRC(a27a80ba) SHA1(3dd3b6b0ace6ca6653603bea952b828b154a2223) )   /* sprites */
	ROM_LOAD( "toki.ob2",     0x080000, 0x80000, CRC(fa687718) SHA1(f194b742399d8124d97cfa3d59beb980c36cfb3c) )

	ROM_REGION( 0x080000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "toki.bk1",     0x000000, 0x80000, CRC(fdaa5f4b) SHA1(ea850361bc8274639e8433bd2a5307fd3a0c9a24) )   /* tiles 1 */

	ROM_REGION( 0x080000, REGION_GFX4, ROMREGION_DISPOSE )
	ROM_LOAD( "toki.bk2",     0x000000, 0x80000, CRC(d86ac664) SHA1(bcb64d8e7ad29b8201ebbada1f858075eb8a0f1d) )   /* tiles 2 */

	ROM_REGION( 0x20000, REGION_SOUND1, 0 )	/* ADPCM samples */
	ROM_LOAD( "tokijp.009",   0x00000, 0x20000, CRC(ae7a6b8b) SHA1(1d410f91354ffd1774896b2e64f20a2043607805) )
ROM_END

ROM_START( tokib )
	ROM_REGION( 0x60000, REGION_CPU1, 0 )	/* 6*64k for 68000 code */
	ROM_LOAD16_BYTE( "toki.e3",      0x00000, 0x20000, CRC(ae9b3da4) SHA1(14eabbd0b3596528e96e4399dde03f5817eddbaa) )
	ROM_LOAD16_BYTE( "toki.e5",      0x00001, 0x20000, CRC(66a5a1d6) SHA1(9a8330d19234863952b0a5dce3f5ad28fcabaa31) )
	ROM_LOAD16_BYTE( "tokijp.005",   0x40000, 0x10000, CRC(d6a82808) SHA1(9fcd3e97f7eaada5374347383dc8a6cea2378f7f) )
	ROM_LOAD16_BYTE( "tokijp.003",   0x40001, 0x10000, CRC(a01a5b10) SHA1(76d6da114105402aab9dd5167c0c00a0bddc3bba) )

	ROM_REGION( 0x18000, REGION_CPU2, 0 )	/* 64k for code + 32k for banked data */
	ROM_LOAD( "toki.e1",      0x00000, 0x8000, CRC(2832ef75) SHA1(c15dc67a1251230fe79625b582c255678f3714d8) )
	ROM_CONTINUE(             0x10000, 0x8000 ) /* banked at 8000-bfff */

	ROM_REGION( 0x020000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "toki.e21",     0x000000, 0x08000, CRC(bb8cacbd) SHA1(05cdd2efe63de30dec2e5d2948567cee22e82a63) )   /* chars */
	ROM_LOAD( "toki.e13",     0x008000, 0x08000, CRC(052ad275) SHA1(0f4a9c752348cf5fb43d706bacbcd3e5937441e7) )
	ROM_LOAD( "toki.e22",     0x010000, 0x08000, CRC(04dcdc21) SHA1(3b74019d764a13ffc155f154522c6fe60cf1c5ea) )
	ROM_LOAD( "toki.e7",      0x018000, 0x08000, CRC(70729106) SHA1(e343c02d139d20a54e837e65b6a964e202f5811e) )

	ROM_REGION( 0x100000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "toki.e26",     0x000000, 0x20000, CRC(a8ba71fc) SHA1(331d7396b6e862e32bb6a0d62c25fc201203b951) )   /* sprites */
	ROM_LOAD( "toki.e28",     0x020000, 0x20000, CRC(29784948) SHA1(9e17e57e2cb65a0aff61385c6d3a97b52474b6e7) )
	ROM_LOAD( "toki.e34",     0x040000, 0x20000, CRC(e5f6e19b) SHA1(77dc5cf961c8062b86ebeb896ad2075c3bfa2205) )
	ROM_LOAD( "toki.e36",     0x060000, 0x20000, CRC(96e8db8b) SHA1(9a0421fc57af27a8886e35b7a1a873aa06a112af) )
	ROM_LOAD( "toki.e30",     0x080000, 0x20000, CRC(770d2b1b) SHA1(27e57f21b462e36a10ffa2d4384955047b84190c) )
	ROM_LOAD( "toki.e32",     0x0a0000, 0x20000, CRC(c289d246) SHA1(596eda73b073e8fc3053734c780e7e2604fb5ca3) )
	ROM_LOAD( "toki.e38",     0x0c0000, 0x20000, CRC(87f4e7fb) SHA1(07d6bf00b1145a11f3d3f0af4425a3c5baeca3db) )
	ROM_LOAD( "toki.e40",     0x0e0000, 0x20000, CRC(96e87350) SHA1(754947f71261d8358e158fa9c8fcfd242cd58bc3) )

	ROM_REGION( 0x080000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "toki.e23",     0x000000, 0x10000, CRC(feb13d35) SHA1(1b78ce1e48d16e58ad0721b30ab87765ded7d24e) )   /* tiles 1 */
	ROM_LOAD( "toki.e24",     0x010000, 0x10000, CRC(5b365637) SHA1(434775b0614d904beaf40d7e00c1eaf59b704cb1) )
	ROM_LOAD( "toki.e15",     0x020000, 0x10000, CRC(617c32e6) SHA1(a80f93c83a06acf836e638e4ad2453692622015d) )
	ROM_LOAD( "toki.e16",     0x030000, 0x10000, CRC(2a11c0f0) SHA1(f9b1910c4932f5b95e5a9a8e8d5376c7210bcde7) )
	ROM_LOAD( "toki.e17",     0x040000, 0x10000, CRC(fbc3d456) SHA1(dd10455f2e6c415fb5e39fb239904c499b38ca3e) )
	ROM_LOAD( "toki.e18",     0x050000, 0x10000, CRC(4c2a72e1) SHA1(52a31f88e02e1689c2fffbbd86cbccd0bdab7dcc) )
	ROM_LOAD( "toki.e8",      0x060000, 0x10000, CRC(46a1b821) SHA1(74d9762aef3891463dc100d1bc2d4fdc3c1d163f) )
	ROM_LOAD( "toki.e9",      0x070000, 0x10000, CRC(82ce27f6) SHA1(db29396a336098664f48e3c04930b973a6ffe969) )

	ROM_REGION( 0x080000, REGION_GFX4, ROMREGION_DISPOSE )
	ROM_LOAD( "toki.e25",     0x000000, 0x10000, CRC(63026cad) SHA1(c8f3898985d99f2a61d4e17eba66b5989a23d0d7) )   /* tiles 2 */
	ROM_LOAD( "toki.e20",     0x010000, 0x10000, CRC(a7f2ce26) SHA1(6b12b3bd872112b42d91ce3c0d5bc95c0fc0f5b5) )
	ROM_LOAD( "toki.e11",     0x020000, 0x10000, CRC(48989aa0) SHA1(109c68c9f0966862194226cecc8b269d9307dd25) )
	ROM_LOAD( "toki.e12",     0x030000, 0x10000, CRC(c2ad9342) SHA1(7c9b5c14c8061e1a57797b79677741b1b98e64fa) )
	ROM_LOAD( "toki.e19",     0x040000, 0x10000, CRC(6cd22b18) SHA1(8281cfd46738448b6890c50c64fb72941e169bee) )
	ROM_LOAD( "toki.e14",     0x050000, 0x10000, CRC(859e313a) SHA1(18ac471a72b3ed42ba74456789adbe323f723660) )
	ROM_LOAD( "toki.e10",     0x060000, 0x10000, CRC(e15c1d0f) SHA1(d0d571dd1055d7307379850313216da86b0704e6) )
	ROM_LOAD( "toki.e6",      0x070000, 0x10000, CRC(6f4b878a) SHA1(4560b1e705a0eb9fad7fdc11fadf952ff67eb264) )
ROM_END


static DRIVER_INIT( toki )
{
	seibu_sound_decrypt(REGION_CPU2,0x2000);
}


DRIVER_INIT( tokib )
{
	unsigned char *temp = malloc (65536 * 2);
	int i, offs;

	/* invert the sprite data in the ROMs */
	for (i = 0; i < memory_region_length(REGION_GFX2); i++)
		memory_region(REGION_GFX2)[i] ^= 0xff;

	/* merge background tile graphics together */
	if (temp)
	{
		for (offs = 0; offs < memory_region_length(REGION_GFX3); offs += 0x20000)
		{
			unsigned char *base = &memory_region(REGION_GFX3)[offs];
			memcpy (temp, base, 65536 * 2);
			for (i = 0; i < 16; i++)
			{
				memcpy (&base[0x00000 + i * 0x800], &temp[0x0000 + i * 0x2000], 0x800);
				memcpy (&base[0x10000 + i * 0x800], &temp[0x0800 + i * 0x2000], 0x800);
				memcpy (&base[0x08000 + i * 0x800], &temp[0x1000 + i * 0x2000], 0x800);
				memcpy (&base[0x18000 + i * 0x800], &temp[0x1800 + i * 0x2000], 0x800);
			}
		}
		for (offs = 0; offs < memory_region_length(REGION_GFX4); offs += 0x20000)
		{
			unsigned char *base = &memory_region(REGION_GFX4)[offs];
			memcpy (temp, base, 65536 * 2);
			for (i = 0; i < 16; i++)
			{
				memcpy (&base[0x00000 + i * 0x800], &temp[0x0000 + i * 0x2000], 0x800);
				memcpy (&base[0x10000 + i * 0x800], &temp[0x0800 + i * 0x2000], 0x800);
				memcpy (&base[0x08000 + i * 0x800], &temp[0x1000 + i * 0x2000], 0x800);
				memcpy (&base[0x18000 + i * 0x800], &temp[0x1800 + i * 0x2000], 0x800);
			}
		}

		free (temp);
	}
}



GAME( 1989, toki,  0,    toki,  toki,  toki,  ROT0, "Tad", "Toki (World set 1)" )
GAME( 1989, tokia, toki, toki,  toki,  toki,  ROT0, "Tad", "Toki (World set 2)" )
GAME( 1989, tokij, toki, toki,  toki,  toki,  ROT0, "Tad", "JuJu Densetsu (Japan)" )
GAME( 1989, tokiu, toki, toki,  toki,  toki,  ROT0, "Tad (Fabtek license)", "Toki (US)" )
GAME( 1989, tokib, toki, tokib, tokib, tokib, ROT0, "bootleg", "Toki (bootleg)" )
