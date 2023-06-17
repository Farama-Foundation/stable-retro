/*
 *	Thunder Ceptor board
 *	(C) 1986 Namco
 *
 *	Hardware analyzed by nono
 *	Driver by BUT
 */

#include "driver.h"
#include "state.h"
#include "cpu/m6502/m6502.h"
#include "cpu/m6809/m6809.h"
#include "cpu/m6800/m6800.h"
#include "cpu/m68000/m68000.h"
#include "namcoic.h"

extern PALETTE_INIT( tceptor );
extern VIDEO_START( tceptor );
extern VIDEO_UPDATE( tceptor );

extern READ_HANDLER( tceptor_tile_ram_r );
extern WRITE_HANDLER( tceptor_tile_ram_w );
extern READ_HANDLER( tceptor_tile_attr_r );
extern WRITE_HANDLER( tceptor_tile_attr_w );
extern READ_HANDLER( tceptor_bg_ram_r );
extern WRITE_HANDLER( tceptor_bg_ram_w );
extern WRITE_HANDLER( tceptor_bg_scroll_w );

extern data8_t *tceptor_tile_ram;
extern data8_t *tceptor_tile_attr;
extern data8_t *tceptor_bg_ram;
extern data16_t *tceptor_sprite_ram;


/*******************************************************************/

static data8_t *m6502_a_shared_ram;
static data8_t *m6502_b_shared_ram;
static data8_t *m68k_shared_ram;
static data8_t *mcu_shared_ram;

static int m6809_irq_enable;
static int m68k_irq_enable;
static int mcu_irq_enable;


/*******************************************************************/

static READ_HANDLER( m6502_a_shared_r )
{
	return m6502_a_shared_ram[offset];
}

static WRITE_HANDLER( m6502_a_shared_w )
{
	m6502_a_shared_ram[offset] = data;
}

static READ_HANDLER( m6502_b_shared_r )
{
	return m6502_b_shared_ram[offset];
}

static WRITE_HANDLER( m6502_b_shared_w )
{
	m6502_b_shared_ram[offset] = data;
}


static READ_HANDLER( m68k_shared_r )
{
	return m68k_shared_ram[offset];
}

static WRITE_HANDLER( m68k_shared_w )
{
	m68k_shared_ram[offset] = data;
}

static READ16_HANDLER( m68k_shared_word_r )
{
	return m68k_shared_ram[offset];
}

static WRITE16_HANDLER( m68k_shared_word_w )
{
	if (ACCESSING_LSB16)
		m68k_shared_ram[offset] = data & 0xff;
}


static READ_HANDLER( mcu_shared_r )
{
	return mcu_shared_ram[offset];
}

static WRITE_HANDLER( mcu_shared_w )
{
	mcu_shared_ram[offset] = data;
}


/*******************************************************************/

static INTERRUPT_GEN( m6809_vb_interrupt )
{
	if (m6809_irq_enable)
		cpu_set_irq_line(0, 0, HOLD_LINE);
	else
		m6809_irq_enable = 1;
}

static WRITE_HANDLER( m6809_irq_enable_w )
{
	m6809_irq_enable = 1;
}

static WRITE_HANDLER( m6809_irq_disable_w )
{
	m6809_irq_enable = 0;
}


static INTERRUPT_GEN( m68k_vb_interrupt )
{
	if (m68k_irq_enable)
		cpu_set_irq_line(3, MC68000_IRQ_1, HOLD_LINE);
}

static WRITE16_HANDLER( m68k_irq_enable_w )
{
	m68k_irq_enable = data;
}


static INTERRUPT_GEN( mcu_vb_interrupt )
{
	if (mcu_irq_enable)
		cpu_set_irq_line(4, 0, HOLD_LINE);
	else
		mcu_irq_enable = 1;
}

static WRITE_HANDLER( mcu_irq_enable_w )
{
	mcu_irq_enable = 1;
}

static WRITE_HANDLER( mcu_irq_disable_w )
{
	mcu_irq_enable = 0;
}


static WRITE_HANDLER( voice_w )
{
	DAC_signed_data_16_w(0, data ? (data + 1) * 0x100 : 0x8000);
}


/* fix dsw/input data to memory mapped data */
static data8_t fix_input0(data8_t in1, data8_t in2)
{
	data8_t r = 0;

	r |= (in1 & 0x80) >> 7;
	r |= (in1 & 0x20) >> 4;
	r |= (in1 & 0x08) >> 1;
	r |= (in1 & 0x02) << 2;
	r |= (in2 & 0x80) >> 3;
	r |= (in2 & 0x20) >> 0;
	r |= (in2 & 0x08) << 3;
	r |= (in2 & 0x02) << 6;

	return r;
}

static data8_t fix_input1(data8_t in1, data8_t in2)
{
	data8_t r = 0;

	r |= (in1 & 0x40) >> 6;
	r |= (in1 & 0x10) >> 3;
	r |= (in1 & 0x04) >> 0;
	r |= (in1 & 0x01) << 3;
	r |= (in2 & 0x40) >> 2;
	r |= (in2 & 0x10) << 1;
	r |= (in2 & 0x04) << 4;
	r |= (in2 & 0x01) << 7;

	return r;
}

static READ_HANDLER( dsw0_r )
{
	return fix_input0(readinputport(0), readinputport(1));
}

static READ_HANDLER( dsw1_r )
{
	return fix_input1(readinputport(0), readinputport(1));
}

static READ_HANDLER( input0_r )
{
	return fix_input0(readinputport(2), readinputport(3));
}

static READ_HANDLER( input1_r )
{
	return fix_input1(readinputport(2), readinputport(3));
}

static READ_HANDLER( readFF )
{
	return 0xff;
}

/*******************************************************************/

static MEMORY_READ_START( m6809_readmem )
	{ 0x0000, 0x00ff, MRA_RAM },
	{ 0x0100, 0x16ff, MRA_RAM },
	{ 0x1700, 0x17ff, MRA_RAM },
	{ 0x1800, 0x1bff, tceptor_tile_ram_r },
	{ 0x1c00, 0x1fff, tceptor_tile_attr_r },
	{ 0x2000, 0x3fff, tceptor_bg_ram_r },		/* background (VIEW RAM)*/
	{ 0x4000, 0x40ff, namcos1_wavedata_r },
	{ 0x4000, 0x43ff, mcu_shared_r },
	{ 0x4f00, 0x4f00, MRA_NOP },			/* unknown*/
	{ 0x4f01, 0x4f01, input_port_4_r },		/* analog input (accel)*/
	{ 0x4f02, 0x4f02, input_port_5_r },		/* analog input (left/right)*/
	{ 0x4f03, 0x4f03, input_port_6_r },		/* analog input (up/down)*/
	{ 0x6000, 0x7fff, m68k_shared_r },		/* COM RAM*/
	{ 0x8000, 0xffff, MRA_ROM },
MEMORY_END

static MEMORY_WRITE_START( m6809_writemem )
	{ 0x0000, 0x00ff, MWA_RAM },
	{ 0x0100, 0x16ff, MWA_RAM },
	{ 0x1700, 0x17ff, MWA_RAM },
	{ 0x1800, 0x1bff, tceptor_tile_ram_w, &tceptor_tile_ram },
	{ 0x1c00, 0x1fff, tceptor_tile_attr_w, &tceptor_tile_attr },
	{ 0x2000, 0x3fff, tceptor_bg_ram_w, &tceptor_bg_ram },	/* background (VIEW RAM)*/
	{ 0x4000, 0x40ff, namcos1_wavedata_w },
	{ 0x4000, 0x43ff, mcu_shared_w },
	{ 0x4800, 0x4800, MWA_NOP },			/* 3D scope left/right?*/
	{ 0x4f00, 0x4f03, MWA_NOP },			/* analog input control?*/
	{ 0x5000, 0x5006, tceptor_bg_scroll_w },	/* bg scroll*/
	{ 0x6000, 0x7fff, m68k_shared_w, &m68k_shared_ram },
	{ 0x8000, 0x8000, m6809_irq_disable_w },
	{ 0x8800, 0x8800, m6809_irq_enable_w },
	{ 0x8000, 0xffff, MWA_ROM },
MEMORY_END


static MEMORY_READ_START( m6502_a_readmem )
	{ 0x0000, 0x00ff, m6502_b_shared_r },
	{ 0x0100, 0x01ff, MRA_RAM },
	{ 0x0200, 0x02ff, MRA_RAM },
	{ 0x0300, 0x030f, MRA_RAM },
	{ 0x2001, 0x2001, YM2151_status_port_0_r },
	{ 0x3000, 0x30ff, m6502_a_shared_r },
	{ 0x8000, 0xffff, MRA_ROM },
MEMORY_END

static MEMORY_WRITE_START( m6502_a_writemem )
	{ 0x0000, 0x00ff, m6502_b_shared_w },
	{ 0x0100, 0x01ff, MWA_RAM },
	{ 0x0200, 0x02ff, MWA_RAM },
	{ 0x0300, 0x030f, MWA_RAM },
	{ 0x2000, 0x2000, YM2151_register_port_0_w },
	{ 0x2001, 0x2001, YM2151_data_port_0_w },
	{ 0x3000, 0x30ff, m6502_a_shared_w, &m6502_a_shared_ram },
	{ 0x3c01, 0x3c01, MWA_RAM },
	{ 0x8000, 0xffff, MWA_ROM },
MEMORY_END


static MEMORY_READ_START( m6502_b_readmem )
	{ 0x0000, 0x00ff, m6502_b_shared_r },
	{ 0x0100, 0x01ff, MRA_RAM },
	{ 0x8000, 0xffff, MRA_ROM },
MEMORY_END

static MEMORY_WRITE_START( m6502_b_writemem )
	{ 0x0000, 0x00ff, m6502_b_shared_w, &m6502_b_shared_ram },
	{ 0x0100, 0x01ff, MWA_RAM },
	{ 0x4000, 0x4000, voice_w },			/* voice data*/
	{ 0x5000, 0x5000, MWA_RAM },			/* voice ctrl??*/
	{ 0x8000, 0xffff, MWA_ROM },
MEMORY_END


static MEMORY_READ16_START( m68k_readmem )
	{ 0x000000, 0x00ffff, MRA16_ROM },		/* M68K ERROR 1*/
	{ 0x100000, 0x10ffff, MRA16_ROM },		/* not sure*/
	{ 0x200000, 0x203fff, MRA16_RAM },		/* M68K ERROR 0*/
	{ 0x700000, 0x703fff, m68k_shared_word_r },
MEMORY_END

static MEMORY_WRITE16_START( m68k_writemem )
	{ 0x000000, 0x00ffff, MWA16_ROM },
	{ 0x100000, 0x10ffff, MWA16_ROM },		/* not sure*/
	{ 0x200000, 0x203fff, MWA16_RAM },
	{ 0x300000, 0x300001, MWA16_RAM },
	{ 0x400000, 0x4001ff, MWA16_RAM, &tceptor_sprite_ram },
	{ 0x500000, 0x51ffff, namco_road16_w },
	{ 0x600000, 0x600001, m68k_irq_enable_w },	/* not sure*/
	{ 0x700000, 0x703fff, m68k_shared_word_w },
MEMORY_END


static MEMORY_READ_START( mcu_readmem )
	{ 0x0000, 0x001f, hd63701_internal_registers_r },
	{ 0x0080, 0x00ff, MRA_RAM },
	{ 0x1000, 0x10ff, namcos1_wavedata_r },
	{ 0x1000, 0x13ff, mcu_shared_r },
	{ 0x1400, 0x154d, MRA_RAM },
	{ 0x17c0, 0x17ff, MRA_RAM },
	{ 0x2000, 0x20ff, m6502_a_shared_r },
	{ 0x2100, 0x2100, dsw0_r },
	{ 0x2101, 0x2101, dsw1_r },
	{ 0x2200, 0x2200, input0_r },
	{ 0x2201, 0x2201, input1_r },
	{ 0x8000, 0xbfff, MRA_ROM },
	{ 0xc000, 0xc7ff, MRA_RAM },
	{ 0xc800, 0xdfff, MRA_RAM },			/* Battery Backup*/
	{ 0xf000, 0xffff, MRA_ROM },
MEMORY_END

static MEMORY_WRITE_START( mcu_writemem )
	{ 0x0000, 0x001f, hd63701_internal_registers_w },
	{ 0x0080, 0x00ff, MWA_RAM },
	{ 0x1000, 0x10ff, namcos1_wavedata_w, &namco_wavedata },
	{ 0x1100, 0x113f, namcos1_sound_w, &namco_soundregs },
	{ 0x1000, 0x13ff, mcu_shared_w, &mcu_shared_ram },
	{ 0x1400, 0x154d, MWA_RAM },
	{ 0x17c0, 0x17ff, MWA_RAM },
	{ 0x2000, 0x20ff, m6502_a_shared_w },
	{ 0x8000, 0x8000, mcu_irq_disable_w },
	{ 0x8800, 0x8800, mcu_irq_enable_w },
	{ 0x8000, 0xbfff, MWA_ROM },
	{ 0xc000, 0xc7ff, MWA_RAM },
	{ 0xc800, 0xdfff, MWA_RAM, &generic_nvram, &generic_nvram_size },	/* Battery Backup*/
	{ 0xf000, 0xffff, MWA_ROM },
MEMORY_END


static PORT_READ_START( mcu_readport )
	{ HD63701_PORT1, HD63701_PORT1, readFF },
	{ HD63701_PORT2, HD63701_PORT2, readFF },
PORT_END

static PORT_WRITE_START( mcu_writeport )
	{ HD63701_PORT1, HD63701_PORT1, MWA_NOP },
	{ HD63701_PORT2, HD63701_PORT2, MWA_NOP },
PORT_END


/*******************************************************************/

INPUT_PORTS_START( tceptor )
	PORT_START      /* DSW 1 */
	PORT_DIPNAME( 0x07, 0x07, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 3C_2C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 4C_3C ) )
	PORT_DIPSETTING(    0x07, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x05, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_4C ) )
	PORT_DIPNAME( 0x18, 0x18, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 3C_2C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 4C_3C ) )
	PORT_DIPSETTING(    0x18, DEF_STR( 1C_1C ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x20, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, "Freeze" )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* DSW 2 */
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x02, "A" )
	PORT_DIPSETTING(    0x03, "B" )
	PORT_DIPSETTING(    0x01, "C" )
	PORT_DIPSETTING(    0x00, "D" )
	PORT_BIT( 0xfc, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* Memory Mapped Port */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 )	/* shot*/
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 )	/* bomb*/
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON3 )	/* shot*/
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON4 )	/* bomb*/
	PORT_BIT( 0xf0, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* Memory Mapped Port */
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_SERVICE( 0x04, IP_ACTIVE_LOW )		/* TEST SW*/
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0xf0, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* ADC0809 - 8 CHANNEL ANALOG - CHANNEL 1 */
	PORT_ANALOGX( 0xff, 0x00, IPT_PEDAL, 100, 16, 0x00, 0xd6, KEYCODE_Z, IP_JOY_DEFAULT, IP_KEY_DEFAULT, IP_JOY_DEFAULT )

	PORT_START	/* ADC0809 - 8 CHANNEL ANALOG - CHANNEL 2 */
	PORT_ANALOG(  0xff, 0x7f, IPT_AD_STICK_X | IPF_CENTER, 100, 16, 0x00, 0xfe )

	PORT_START	/* ADC08090 - 8 CHANNEL ANALOG - CHANNEL 3 */
	PORT_ANALOG(  0xff, 0x7f, IPT_AD_STICK_Y | IPF_CENTER, 100, 16, 0x00, 0xfe )
INPUT_PORTS_END

INPUT_PORTS_START( tceptor2 )
	PORT_START      /* DSW 1 */
	PORT_DIPNAME( 0x07, 0x07, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 3C_2C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 4C_3C ) )
	PORT_DIPSETTING(    0x07, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x05, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_4C ) )
	PORT_DIPNAME( 0x18, 0x18, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 3C_2C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 4C_3C ) )
	PORT_DIPSETTING(    0x18, DEF_STR( 1C_1C ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x20, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, "Freeze" )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* DSW 2 */
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x02, "A" )
	PORT_DIPSETTING(    0x03, "B" )
	PORT_DIPSETTING(    0x01, "C" )
	PORT_DIPSETTING(    0x00, "D" )
	PORT_DIPNAME( 0x04, 0x00, "MODE" )		/* set default as 2D mode*/
	PORT_DIPSETTING(    0x00, "2D" )
	PORT_DIPSETTING(    0x04, "3D" )
	PORT_BIT( 0xf8, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* Memory Mapped Port */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 )	/* shot*/
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 )	/* bomb*/
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON3 )	/* shot*/
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON4 )	/* bomb*/
	PORT_BIT( 0xf0, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* Memory Mapped Port */
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_SERVICE( 0x04, IP_ACTIVE_LOW )		/* TEST SW*/
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0xf0, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* ADC0809 - 8 CHANNEL ANALOG - CHANNEL 1 */
	PORT_ANALOGX( 0xff, 0x00, IPT_PEDAL, 100, 16, 0x00, 0xd6, KEYCODE_Z, IP_JOY_DEFAULT, IP_KEY_DEFAULT, IP_JOY_DEFAULT )

	PORT_START	/* ADC0809 - 8 CHANNEL ANALOG - CHANNEL 2 */
	PORT_ANALOG(  0xff, 0x7f, IPT_AD_STICK_X | IPF_CENTER, 100, 16, 0x00, 0xfe )

	PORT_START	/* ADC08090 - 8 CHANNEL ANALOG - CHANNEL 3 */
	PORT_ANALOG(  0xff, 0x7f, IPT_AD_STICK_Y | IPF_CENTER, 100, 16, 0x00, 0xfe )
INPUT_PORTS_END


/*******************************************************************/

static struct GfxLayout tile_layout =
{
	8, 8,
	512,
	2,
	{ 0x0000, 0x0004 }, /*,  0x8000, 0x8004 },*/
	{ 8*8, 8*8+1, 8*8+2, 8*8+3, 0*8+0, 0*8+1, 0*8+2, 0*8+3 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	2*8*8
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &tile_layout,     0,  256 },

	/* decode in VIDEO_START */
	/*{ REGION_GFX2, 0, &bg_layout,    2048,   64 },*/
	/*{ REGION_GFX3, 0, &spr16_layout, 1024,   64 },*/
	/*{ REGION_GFX4, 0, &spr32_layout, 1024,   64 },*/
	{ -1 }
};


/*******************************************************************/

static struct YM2151interface ym2151_interface =
{
	1,			/* 1 chip */
	14318180/4,		/* 3.579545 MHz */
	{ YM3012_VOL(100,MIXER_PAN_LEFT,100,MIXER_PAN_RIGHT) },
	{ 0 },
	{ 0 }
};

static struct namco_interface namco_interface =
{
	49152000/2048,		/* 24000Hz */
	8,			/* number of voices */
	40,			/* playback volume */
	-1,			/* memory region */
	1			/* stereo */
};

static struct DACinterface dac_interface =
{
	1,			/* 1 channel */
	{ 40 }			/* mixing level */
};


/*******************************************************************/

static MACHINE_INIT( tceptor )
{
	m6809_irq_enable = 0;
	m68k_irq_enable = 0;
	mcu_irq_enable = 0;
}

/*******************************************************************/

static MACHINE_DRIVER_START( tceptor )

	/* basic machine hardware */
	MDRV_CPU_ADD(M6809, 49152000/32)
	MDRV_CPU_MEMORY(m6809_readmem,m6809_writemem)
	MDRV_CPU_VBLANK_INT(m6809_vb_interrupt,1)

	MDRV_CPU_ADD(M65C02, 49152000/24)
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
	MDRV_CPU_MEMORY(m6502_a_readmem,m6502_a_writemem)

	MDRV_CPU_ADD(M65C02, 49152000/24)
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
	MDRV_CPU_MEMORY(m6502_b_readmem,m6502_b_writemem)

	MDRV_CPU_ADD(M68000, 49152000/4)
	MDRV_CPU_MEMORY(m68k_readmem,m68k_writemem)
	MDRV_CPU_VBLANK_INT(m68k_vb_interrupt,1)

	MDRV_CPU_ADD(HD63701, 49152000/32)	/* or compatible 6808 with extra instructions */
	MDRV_CPU_MEMORY(mcu_readmem,mcu_writemem)
	MDRV_CPU_PORTS(mcu_readport,mcu_writeport)
	MDRV_CPU_VBLANK_INT(mcu_vb_interrupt,1)

	MDRV_FRAMES_PER_SECOND(60.606060)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(100)

	MDRV_NVRAM_HANDLER(generic_1fill)

	MDRV_MACHINE_INIT(tceptor)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_NEEDS_6BITS_PER_GUN)
	MDRV_SCREEN_SIZE(38*8, 32*8)
	MDRV_VISIBLE_AREA(2*8, 34*8-1 + 2*8, 0*8, 28*8-1 + 0)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(1024)
	MDRV_COLORTABLE_LENGTH(4096)

	MDRV_PALETTE_INIT(tceptor)
	MDRV_VIDEO_START(tceptor)
	MDRV_VIDEO_UPDATE(tceptor)

	/* sound hardware */
	MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
	MDRV_SOUND_ADD(YM2151, ym2151_interface)
	MDRV_SOUND_ADD(NAMCO_15XX, namco_interface)
	MDRV_SOUND_ADD(DAC, dac_interface)
MACHINE_DRIVER_END


/*******************************************************************/

static DRIVER_INIT( tceptor )
{
	state_save_register_int("tceptor", 0, "m6809_irq_enable",  &m6809_irq_enable);
	state_save_register_int("tceptor", 0, "m68k_irq_enable",   &m68k_irq_enable);
	state_save_register_int("tceptor", 0, "mcu_irq_enable",    &mcu_irq_enable);
}


/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( tceptor )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )			/* 68A09EP*/
	ROM_LOAD( "tc1-1.10f",  0x08000, 0x08000, CRC(4C6B063E) SHA1(d9701657186f8051391084f51a720037f9f418b1) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )			/* RC65C02*/
	ROM_LOAD( "tc1-21.1m",  0x08000, 0x08000, CRC(2D0B2FA8) SHA1(16ecd70954e52a8661642b15a5cf1db51783e444) )

	ROM_REGION( 0x10000, REGION_CPU3, 0 )			/* RC65C02*/
	ROM_LOAD( "tc1-22.3m",  0x08000, 0x08000, CRC(9F5A3E98) SHA1(2b2ffe39fe647a3039b92721817bddc9e9a92d82) )

	ROM_REGION( 0x110000, REGION_CPU4, 0 )			/* MC68000-12*/
	ROM_LOAD16_BYTE( "tc1-4.8c",     0x000000, 0x08000, CRC(AE98B673) SHA1(5da1c69dd40db9bad2e3d4dc2af3a949172af940) )
	ROM_LOAD16_BYTE( "tc1-3.10c",    0x000001, 0x08000, CRC(779A4B25) SHA1(8563213a1f1caee0eb88aa4bbd37c6004f16b309) )
	/* socket 8d and 10d are emtpy*/

	ROM_REGION( 0x10000, REGION_CPU5, 0 )			/* Custom 60A1*/
	ROM_LOAD( "tc1-2.3a",    0x08000, 0x4000, CRC(B6DEF610) SHA1(d0eada92a25d0243206fb8239374f5757caaea47) )
	ROM_LOAD( "pl1-mcu.bin", 0x0f000, 0x1000, CRC(6ef08fb3) SHA1(4842590d60035a0059b0899eb2d5f58ae72c2529) ) /* microcontroller */

	ROM_REGION( 0x02000, REGION_GFX1, ROMREGION_DISPOSE )	/* text tilemap*/
	ROM_LOAD( "tc1-18.6b",  0x00000, 0x02000, CRC(662B5650) SHA1(ba82fe5efd1011854a6d0d7d87075475b65c0601) )

	ROM_REGION( 0x10000, REGION_GFX2, ROMREGION_DISPOSE )	/* background*/
	ROM_LOAD( "tc1-20.10e", 0x00000, 0x08000, CRC(3E5054B7) SHA1(ed359f8659a4a46d5ff7299d0da10550b1496db8) )
	ROM_LOAD( "tc1-19.10d", 0x08000, 0x04000, CRC(7406E6E7) SHA1(61ad77667e94fd7e11037da2721f7bbe0130286a) )

	ROM_REGION( 0x10000, REGION_GFX3, ROMREGION_DISPOSE )	/* 16x16 sprites*/
	ROM_LOAD( "tc1-16.8t",  0x00000, 0x08000, CRC(7C72BE33) SHA1(397e11727b86688d550c28fbdcb864bb9335d891) )
	ROM_LOAD( "tc1-15.10t", 0x08000, 0x08000, CRC(51268075) SHA1(75b6b935c6721adbc984795b9bf0a791fb8b209e) )

	ROM_REGION( 0x80000, REGION_GFX4, ROMREGION_DISPOSE )	/* 32x32 sprites*/
	ROM_LOAD( "tc1-8.8m",   0x00000, 0x10000, CRC(192A1F1F) SHA1(8424a6a19c080da0a83e173e33915f4d9326f379) )
	ROM_LOAD( "tc1-10.8p",  0x10000, 0x08000, CRC(7876BCEF) SHA1(09180b26d0eab51de18a13723f46d763541979fb) )
	ROM_RELOAD(             0x18000, 0x08000 )
	ROM_LOAD( "tc1-12.8r",  0x20000, 0x08000, CRC(E8F55842) SHA1(7397c8f279b9ddb7d9daf16f307669257a3fd9df) )
	ROM_RELOAD(             0x28000, 0x08000 )
	ROM_LOAD( "tc1-14.8s",  0x30000, 0x08000, CRC(723ACF62) SHA1(fa62ffa2a641629803537d0ef1ad30688b04f9ca) )
	ROM_RELOAD(             0x38000, 0x08000 )
	ROM_LOAD( "tc1-7.10m",  0x40000, 0x10000, CRC(828C80D5) SHA1(6d441cbb333aee21f9c3d9608aec951130f9b0c5) )
	ROM_LOAD( "tc1-9.10p",  0x50000, 0x08000, CRC(145CF59B) SHA1(0639a36030823ccd7a476561a8fe61724c8be9d3) )
	ROM_RELOAD(             0x58000, 0x08000 )
	ROM_LOAD( "tc1-11.10r", 0x60000, 0x08000, CRC(AD7C6C7E) SHA1(2ae889c135c6ee924dc336895f7b9b8a98b715d0) )
	ROM_RELOAD(             0x68000, 0x08000 )
	ROM_LOAD( "tc1-13.10s", 0x70000, 0x08000, CRC(E67CEF29) SHA1(ba8559caf498bbc1d9278d74da03ee2d910f76d8) )
	ROM_RELOAD(             0x78000, 0x08000 )

	ROM_REGION( 0x3600, REGION_PROMS, 0 )
	ROM_LOAD( "tc1-3.1k",   0x00000, 0x00400, CRC(FD2FCB57) SHA1(97d5b7527714acfd729b26ac56f0a9210982c551) )	/* red components*/
	ROM_LOAD( "tc1-1.1h",   0x00400, 0x00400, CRC(0241CF67) SHA1(9b2b579425b72a5b1f2c632f53d1c1d172b4ed1e) )	/* green components*/
	ROM_LOAD( "tc1-2.1j",   0x00800, 0x00400, CRC(EA9EB3DA) SHA1(0d7cfceac57afc53a063d7fe67cfc9bda0a8dbc8) )	/* blue components*/
	ROM_LOAD( "tc1-5.6a",   0x00c00, 0x00400, CRC(AFA8EDA8) SHA1(783efbcbf0bb7e4cf2e2618ddd0ef3b52a4518cc) )	/* tiles color table*/
	ROM_LOAD( "tc1-4.2e",   0x01000, 0x00100, CRC(A4E73D53) SHA1(df8231720e9b57cf2751f86ac3ed7433804f51ca) )	/* road color table*/
	ROM_LOAD( "tc1-6.7s",   0x01100, 0x00400, CRC(72707677) SHA1(122c1b619c9efa3b7055908dda3102ee28230504) )	/* sprite color table*/
	ROM_LOAD( "tc1-17.7k",  0x01500, 0x02000, CRC(90DB1BF6) SHA1(dbb9e50a8efc3b4012fcf587cc87da9ef42a1b80) )	/* sprite related*/
ROM_END

ROM_START( tceptor2 )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )			/* 68A09EP*/
	ROM_LOAD( "tc2-1.10f",  0x08000, 0x08000, CRC(F953F153) SHA1(f4cd0a133d23b4bf3c24c70c28c4ecf8ad4daf6f) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )			/* RC65C02*/
	ROM_LOAD( "tc1-21.1m",  0x08000, 0x08000, CRC(2D0B2FA8) SHA1(16ecd70954e52a8661642b15a5cf1db51783e444) )

	ROM_REGION( 0x10000, REGION_CPU3, 0 )			/* RC65C02*/
	ROM_LOAD( "tc1-22.3m",  0x08000, 0x08000, CRC(9F5A3E98) SHA1(2b2ffe39fe647a3039b92721817bddc9e9a92d82) )

	ROM_REGION( 0x110000, REGION_CPU4, 0 )			/* MC68000-12*/
	ROM_LOAD16_BYTE( "tc2-4.8c",     0x000000, 0x08000, CRC(6C2EFC04) SHA1(3a91f5b8bbf7040083e2da2bd0fb2ab3c51ec45c) )
	ROM_LOAD16_BYTE( "tc2-3.10c",    0x000001, 0x08000, CRC(312B781A) SHA1(37bf3ced16b765d78bf8de7a4916c2b518b702ed) )
	ROM_LOAD16_BYTE( "tc2-6.8d",     0x100000, 0x08000, CRC(20711F14) SHA1(39623592bb4be3b3be2bff4b3219ac16ba612761) )
	ROM_LOAD16_BYTE( "tc2-5.10d",    0x100001, 0x08000, CRC(925F2560) SHA1(81fcef6a9c7e9dfb6884043cf2266854bc87cd69) )

	ROM_REGION( 0x10000, REGION_CPU5, 0 )			/* Custom 60A1*/
	ROM_LOAD( "tc1-2.3a",    0x08000, 0x4000, CRC(B6DEF610) SHA1(d0eada92a25d0243206fb8239374f5757caaea47) )
	ROM_LOAD( "pl1-mcu.bin", 0x0f000, 0x1000, CRC(6ef08fb3) SHA1(4842590d60035a0059b0899eb2d5f58ae72c2529) ) /* microcontroller */

	ROM_REGION( 0x02000, REGION_GFX1, ROMREGION_DISPOSE )	/* text tilemap*/
	ROM_LOAD( "tc1-18.6b",  0x00000, 0x02000, CRC(662B5650) SHA1(ba82fe5efd1011854a6d0d7d87075475b65c0601) )

	ROM_REGION( 0x10000, REGION_GFX2, ROMREGION_DISPOSE )	/* background*/
	ROM_LOAD( "tc2-20.10e", 0x00000, 0x08000, CRC(E72738FC) SHA1(53664400f343acdc1d8cf7e00e261ae42b857a5f) )
	ROM_LOAD( "tc2-19.10d", 0x08000, 0x04000, CRC(9C221E21) SHA1(58bcbb998dcf2190cf46dd3d22b116ac673285a6) )

	ROM_REGION( 0x10000, REGION_GFX3, ROMREGION_DISPOSE )	/* 16x16 sprites*/
	ROM_LOAD( "tc2-16.8t",  0x00000, 0x08000, CRC(DCF4DA96) SHA1(e953cb46d60171271128b3e0ef4e958d1fab1d04) )
	ROM_LOAD( "tc2-15.10t", 0x08000, 0x08000, CRC(FB0A9F89) SHA1(cc9be6ff542b5d5e6ad3baca7a355b9bd31b3dd1) )

	ROM_REGION( 0x80000, REGION_GFX4, ROMREGION_DISPOSE )	/* 32x32 sprites*/
	ROM_LOAD( "tc2-8.8m",   0x00000, 0x10000, CRC(03528D79) SHA1(237810fa55c36b6d87c7e02e02f19feb64e5a11f) )
	ROM_LOAD( "tc2-10.8p",  0x10000, 0x10000, CRC(561105EB) SHA1(101a0e48a740ce4acc34a7d1a50191bb857e7371) )
	ROM_LOAD( "tc2-12.8r",  0x20000, 0x10000, CRC(626CA8FB) SHA1(0b51ced00b3de1f672f6f8c7cc5dd9e2ea2e4f8d) )
	ROM_LOAD( "tc2-14.8s",  0x30000, 0x10000, CRC(B9EEC79D) SHA1(ae69033d6f80be0be883f919544c167e8f91db27) )
	ROM_LOAD( "tc2-7.10m",  0x40000, 0x10000, CRC(0E3523E0) SHA1(eb4670333ad383099fafda1c930f42e48e82f5c5) )
	ROM_LOAD( "tc2-9.10p",  0x50000, 0x10000, CRC(CCFD9FF6) SHA1(2934e098aa5231af18dbfb888fe05faab9576a7d) )
	ROM_LOAD( "tc2-11.10r", 0x60000, 0x10000, CRC(40724380) SHA1(57549094fc8403f1f528e57fe3fa64844bf89e22) )
	ROM_LOAD( "tc2-13.10s", 0x70000, 0x10000, CRC(519EC7C1) SHA1(c4abe279d7cf6f626dcbb6f6c4dc2a138b818f51) )

	ROM_REGION( 0x3600, REGION_PROMS, 0 )
	ROM_LOAD( "tc2-3.1k",   0x00000, 0x00400, CRC(E3504F1A) SHA1(1ac3968e993030a6b2f4719702ff870267ab6918) )	/* red components*/
	ROM_LOAD( "tc2-1.1h",   0x00400, 0x00400, CRC(E8A96FDA) SHA1(42e5d2b351000ac0705b01ab484c5fe8e294a08b) )	/* green components*/
	ROM_LOAD( "tc2-2.1j",   0x00800, 0x00400, CRC(C65EDA61) SHA1(c316b748daa6be68eebbb480557637efc9f44781) )	/* blue components*/
	ROM_LOAD( "tc1-5.6a",   0x00c00, 0x00400, CRC(AFA8EDA8) SHA1(783efbcbf0bb7e4cf2e2618ddd0ef3b52a4518cc) )	/* tiles color table*/
	ROM_LOAD( "tc2-4.2e",   0x01000, 0x00100, CRC(6B49FC30) SHA1(66ca39cd7985643acd71905111ae2d931c082465) )	/* road color table*/
	ROM_LOAD( "tc2-6.7s",   0x01100, 0x00400, CRC(BADCDA76) SHA1(726e0019241d31716f3af9ebe900089bce771477) )	/* sprite color table*/
	ROM_LOAD( "tc1-17.7k",  0x01500, 0x02000, CRC(90DB1BF6) SHA1(dbb9e50a8efc3b4012fcf587cc87da9ef42a1b80) )	/* sprite related*/
ROM_END


/*  ( YEAR  NAME      PARENT    MACHINE   INPUT     INIT      MONITOR   COMPANY FULLNAME ) */
GAME( 1986, tceptor,  0,        tceptor,  tceptor,  tceptor,  ROT0,     "Namco",  "Thunder Ceptor")
GAME( 1986, tceptor2, tceptor,  tceptor,  tceptor2, tceptor,  ROT0,     "Namco",  "Thunder Ceptor II")
