/******************************************************************************

	Game Driver for Nichibutsu Mahjong series.

	Niyanpai
	(c)1996 Nihon Bussan Co.,Ltd.

	Driver by Takahiro Nogi <nogi@kt.rim.or.jp> 2000/12/23 -

******************************************************************************/
/******************************************************************************
Memo:

- TMP68301 emulation is not implemented (machine/m68kfmly.c, .h does nothing).

- niyanpai's 2p start does not mean 2p simultaneous or exchanging play.
  Simply uses controls for 2p side.

- Some games display "GFXROM BANK OVER!!" or "GFXROM ADDRESS OVER!!"
  in Debug build.

- Screen flip is not perfect.

******************************************************************************/

#include "driver.h"
#include "cpu/m68000/m68000.h"
#include "machine/m68kfmly.h"
#include "cpu/z80/z80.h"
#include "machine/z80fmly.h"
#include "vidhrdw/generic.h"


#define	SIGNED_DAC	0		/* 0:unsigned DAC, 1:signed DAC*/


VIDEO_UPDATE( niyanpai );
VIDEO_START( niyanpai );

READ16_HANDLER( niyanpai_palette_r );
WRITE16_HANDLER( niyanpai_palette_w );

WRITE16_HANDLER( niyanpai_gfxflag_0_w );
WRITE16_HANDLER( niyanpai_scrollx_0_w );
WRITE16_HANDLER( niyanpai_scrolly_0_w );
WRITE16_HANDLER( niyanpai_radr_0_w );
WRITE16_HANDLER( niyanpai_sizex_0_w );
WRITE16_HANDLER( niyanpai_sizey_0_w );
WRITE16_HANDLER( niyanpai_drawx_0_w );
WRITE16_HANDLER( niyanpai_drawy_0_w );

WRITE16_HANDLER( niyanpai_gfxflag_1_w );
WRITE16_HANDLER( niyanpai_scrollx_1_w );
WRITE16_HANDLER( niyanpai_scrolly_1_w );
WRITE16_HANDLER( niyanpai_radr_1_w );
WRITE16_HANDLER( niyanpai_sizex_1_w );
WRITE16_HANDLER( niyanpai_sizey_1_w );
WRITE16_HANDLER( niyanpai_drawx_1_w );
WRITE16_HANDLER( niyanpai_drawy_1_w );

WRITE16_HANDLER( niyanpai_gfxflag_2_w );
WRITE16_HANDLER( niyanpai_scrollx_2_w );
WRITE16_HANDLER( niyanpai_scrolly_2_w );
WRITE16_HANDLER( niyanpai_radr_2_w );
WRITE16_HANDLER( niyanpai_sizex_2_w );
WRITE16_HANDLER( niyanpai_sizey_2_w );
WRITE16_HANDLER( niyanpai_drawx_2_w );
WRITE16_HANDLER( niyanpai_drawy_2_w );

WRITE16_HANDLER( niyanpai_paltblnum_0_w );
WRITE16_HANDLER( niyanpai_paltblnum_1_w );
WRITE16_HANDLER( niyanpai_paltblnum_2_w );
WRITE16_HANDLER( niyanpai_paltbl_0_w );
WRITE16_HANDLER( niyanpai_paltbl_1_w );
WRITE16_HANDLER( niyanpai_paltbl_2_w );

READ16_HANDLER( niyanpai_gfxbusy_0_r );
READ16_HANDLER( niyanpai_gfxbusy_1_r );
READ16_HANDLER( niyanpai_gfxbusy_2_r );
READ16_HANDLER( niyanpai_gfxrom_0_r );
READ16_HANDLER( niyanpai_gfxrom_1_r );
READ16_HANDLER( niyanpai_gfxrom_2_r );


static void niyanpai_soundbank_w(int data)
{
	unsigned char *SNDROM = memory_region(REGION_CPU2);

	cpu_setbank(1, &SNDROM[0x08000 + (0x8000 * (data & 0x03))]);
}

static int niyanpai_sound_r(int offset)
{
	return soundlatch_r(0);
}

static WRITE16_HANDLER( niyanpai_sound_w )
{
	soundlatch_w(0, ((data >> 8) & 0xff));
}

static void niyanpai_soundclr_w(int offset, int data)
{
	soundlatch_clear_w(0, 0);
}


/* TMPZ84C011 PIO emulation */
static unsigned char pio_dir[5], pio_latch[5];

static int tmpz84c011_pio_r(int offset)
{
	int portdata;

	switch (offset)
	{
		case	0:			/* PA_0 */
			portdata = 0xff;
			break;
		case	1:			/* PB_0 */
			portdata = 0xff;
			break;
		case	2:			/* PC_0 */
			portdata = 0xff;
			break;
		case	3:			/* PD_0 */
			portdata = niyanpai_sound_r(0);
			break;
		case	4:			/* PE_0 */
			portdata = 0xff;
			break;

		default:
			log_cb(RETRO_LOG_DEBUG, LOGPRE "PC %04X: TMPZ84C011_PIO Unknown Port Read %02X\n", activecpu_get_pc(), offset);
			portdata = 0xff;
			break;
	}

	return portdata;
}

static void tmpz84c011_pio_w(int offset, int data)
{
	switch (offset)
	{
		case	0:			/* PA_0 */
			niyanpai_soundbank_w(data & 0x03);
			break;
		case	1:			/* PB_0 */
#if SIGNED_DAC
			DAC_1_signed_data_w(0, data);
#else
			DAC_1_data_w(0, data);
#endif
			break;
		case	2:			/* PC_0 */
#if SIGNED_DAC
			DAC_0_signed_data_w(0, data);
#else
			DAC_0_data_w(0, data);
#endif
			break;
		case	3:			/* PD_0 */
			break;
		case	4:			/* PE_0 */
			if (!(data & 0x01)) niyanpai_soundclr_w(0, 0);
			break;

		default:
			log_cb(RETRO_LOG_DEBUG, LOGPRE "PC %04X: TMPZ84C011_PIO Unknown Port Write %02X, %02X\n", activecpu_get_pc(), offset, data);
			break;
	}
}

/* CPU interface */
static READ_HANDLER( tmpz84c011_0_pa_r ) { return (tmpz84c011_pio_r(0) & ~pio_dir[0]) | (pio_latch[0] & pio_dir[0]); }
static READ_HANDLER( tmpz84c011_0_pb_r ) { return (tmpz84c011_pio_r(1) & ~pio_dir[1]) | (pio_latch[1] & pio_dir[1]); }
static READ_HANDLER( tmpz84c011_0_pc_r ) { return (tmpz84c011_pio_r(2) & ~pio_dir[2]) | (pio_latch[2] & pio_dir[2]); }
static READ_HANDLER( tmpz84c011_0_pd_r ) { return (tmpz84c011_pio_r(3) & ~pio_dir[3]) | (pio_latch[3] & pio_dir[3]); }
static READ_HANDLER( tmpz84c011_0_pe_r ) { return (tmpz84c011_pio_r(4) & ~pio_dir[4]) | (pio_latch[4] & pio_dir[4]); }

static WRITE_HANDLER( tmpz84c011_0_pa_w ) { pio_latch[0] = data; tmpz84c011_pio_w(0, data); }
static WRITE_HANDLER( tmpz84c011_0_pb_w ) { pio_latch[1] = data; tmpz84c011_pio_w(1, data); }
static WRITE_HANDLER( tmpz84c011_0_pc_w ) { pio_latch[2] = data; tmpz84c011_pio_w(2, data); }
static WRITE_HANDLER( tmpz84c011_0_pd_w ) { pio_latch[3] = data; tmpz84c011_pio_w(3, data); }
static WRITE_HANDLER( tmpz84c011_0_pe_w ) { pio_latch[4] = data; tmpz84c011_pio_w(4, data); }

static READ_HANDLER( tmpz84c011_0_dir_pa_r ) { return pio_dir[0]; }
static READ_HANDLER( tmpz84c011_0_dir_pb_r ) { return pio_dir[1]; }
static READ_HANDLER( tmpz84c011_0_dir_pc_r ) { return pio_dir[2]; }
static READ_HANDLER( tmpz84c011_0_dir_pd_r ) { return pio_dir[3]; }
static READ_HANDLER( tmpz84c011_0_dir_pe_r ) { return pio_dir[4]; }

static WRITE_HANDLER( tmpz84c011_0_dir_pa_w ) { pio_dir[0] = data; }
static WRITE_HANDLER( tmpz84c011_0_dir_pb_w ) { pio_dir[1] = data; }
static WRITE_HANDLER( tmpz84c011_0_dir_pc_w ) { pio_dir[2] = data; }
static WRITE_HANDLER( tmpz84c011_0_dir_pd_w ) { pio_dir[3] = data; }
static WRITE_HANDLER( tmpz84c011_0_dir_pe_w ) { pio_dir[4] = data; }


static void ctc0_interrupt(int state)
{
	cpu_set_irq_line_and_vector(1, 0, HOLD_LINE, Z80_VECTOR(0, state));
}

static z80ctc_interface ctc_intf =
{
	1,			/* 1 chip */
	{ 1 },			/* clock */
	{ 0 },			/* timer disables */
	{ ctc0_interrupt },	/* interrupt handler */
	{ z80ctc_0_trg3_w },	/* ZC/TO0 callback ctc1.zc0 -> ctc1.trg3 */
	{ 0 },			/* ZC/TO1 callback */
	{ 0 },			/* ZC/TO2 callback */
};

static void tmpz84c011_init(void)
{
	int i;

	/* initialize TMPZ84C011 PIO*/
	for (i = 0; i < 5; i++)
	{
		pio_dir[i] = pio_latch[i] = 0;
		tmpz84c011_pio_w(i, 0);
	}

	/* initialize the CTC*/
	ctc_intf.baseclock[0] = Machine->drv->cpu[1].cpu_clock;
	z80ctc_init(&ctc_intf);
}

static MACHINE_INIT( niyanpai )
{
	/**/
}

static void initialize_driver(void)
{
	unsigned char *MAINROM = memory_region(REGION_CPU1);
	unsigned char *SNDROM = memory_region(REGION_CPU2);

	/* main program patch (USR0 -> IRQ LEVEL1)*/
	MAINROM[(25 * 4) + 0] = MAINROM[(64 * 4) + 0];
	MAINROM[(25 * 4) + 1] = MAINROM[(64 * 4) + 1];
	MAINROM[(25 * 4) + 2] = MAINROM[(64 * 4) + 2];
	MAINROM[(25 * 4) + 3] = MAINROM[(64 * 4) + 3];

	/* sound program patch*/
	SNDROM[0x0213] = 0x00;			/* DI -> NOP*/

	/* initialize TMPZ84C011 PIO and CTC*/
	tmpz84c011_init();

	/* initialize sound rom bank*/
	niyanpai_soundbank_w(0);
}


static DRIVER_INIT( niyanpai ) { initialize_driver(); }


static READ16_HANDLER( niyanpai_dipsw_r )
{
	unsigned char dipsw_a, dipsw_b;

	dipsw_a = (((readinputport(0) & 0x01) << 7) | ((readinputport(0) & 0x02) << 5) |
		   ((readinputport(0) & 0x04) << 3) | ((readinputport(0) & 0x08) << 1) |
		   ((readinputport(0) & 0x10) >> 1) | ((readinputport(0) & 0x20) >> 3) |
		   ((readinputport(0) & 0x40) >> 5) | ((readinputport(0) & 0x80) >> 7));

	dipsw_b = (((readinputport(1) & 0x01) << 7) | ((readinputport(1) & 0x02) << 5) |
		   ((readinputport(1) & 0x04) << 3) | ((readinputport(1) & 0x08) << 1) |
		   ((readinputport(1) & 0x10) >> 1) | ((readinputport(1) & 0x20) >> 3) |
		   ((readinputport(1) & 0x40) >> 5) | ((readinputport(1) & 0x80) >> 7));

	return ((dipsw_a << 8) | dipsw_b);
}

static READ16_HANDLER( niyanpai_inputport_0_r )
{
	return ((readinputport(3) << 8) | (readinputport(4) << 0));
}

static READ16_HANDLER( niyanpai_inputport_1_r )
{
	return ((readinputport(2) << 8) | 0xff);
}


static MEMORY_READ16_START( niyanpai_readmem )
	{ 0x000000, 0x03ffff, MRA16_ROM },
	{ 0x040000, 0x040fff, MRA16_RAM },

	{ 0x0a0000, 0x0a08ff, niyanpai_palette_r },
	{ 0x0a0900, 0x0a11ff, MRA16_RAM },		/* palette work ram?*/

	{ 0x0bf800, 0x0bffff, MRA16_RAM },

	{ 0x240400, 0x240401, niyanpai_gfxbusy_0_r },
	{ 0x240402, 0x240403, niyanpai_gfxrom_0_r },
	{ 0x240600, 0x240601, niyanpai_gfxbusy_1_r },
	{ 0x240402, 0x240403, niyanpai_gfxrom_1_r },
	{ 0x240800, 0x240801, niyanpai_gfxbusy_2_r },
	{ 0x240402, 0x240403, niyanpai_gfxrom_2_r },

	{ 0x280000, 0x280001, niyanpai_dipsw_r },
	{ 0x280200, 0x280201, niyanpai_inputport_0_r },
	{ 0x280400, 0x280401, niyanpai_inputport_1_r },

	{ 0xfffc00, 0xfffc0f, tmp68301_address_decoder_r },
	{ 0xfffc80, 0xfffc9f, tmp68301_interrupt_controller_r },
	{ 0xfffd00, 0xfffd0f, tmp68301_parallel_interface_r },
	{ 0xfffd80, 0xfffdaf, tmp68301_serial_interface_r },
	{ 0xfffe00, 0xfffe4f, tmp68301_timer_r },
MEMORY_END

static MEMORY_WRITE16_START( niyanpai_writemem )
	{ 0x000000, 0x03ffff, MWA16_ROM },
	{ 0x040000, 0x040fff, MWA16_RAM, (data16_t **)&generic_nvram, &generic_nvram_size },

	{ 0x0a0000, 0x0a08ff, niyanpai_palette_w },
	{ 0x0a0900, 0x0a11ff, MWA16_RAM },		/* palette work ram?*/

	{ 0x0bf800, 0x0bffff, MWA16_RAM },

	{ 0x200000, 0x200001, niyanpai_sound_w },

	{ 0x200200, 0x200201, MWA16_NOP },		/* unknown*/
	{ 0x240000, 0x240009, MWA16_NOP },		/* unknown*/
	{ 0x240200, 0x2403ff, MWA16_NOP },		/* unknown*/

	{ 0x240400, 0x240401, niyanpai_gfxflag_0_w },
	{ 0x240402, 0x240405, niyanpai_scrollx_0_w },
	{ 0x240406, 0x240409, niyanpai_scrolly_0_w },
	{ 0x24040a, 0x24040f, niyanpai_radr_0_w },
	{ 0x240410, 0x240411, niyanpai_sizex_0_w },
	{ 0x240412, 0x240413, niyanpai_sizey_0_w },
	{ 0x240414, 0x240417, niyanpai_drawx_0_w },
	{ 0x240418, 0x24041b, niyanpai_drawy_0_w },

	{ 0x240420, 0x24043f, niyanpai_paltbl_0_w },

	{ 0x240600, 0x240601, niyanpai_gfxflag_1_w },
	{ 0x240602, 0x240605, niyanpai_scrollx_1_w },
	{ 0x240606, 0x240609, niyanpai_scrolly_1_w },
	{ 0x24060a, 0x24060f, niyanpai_radr_1_w },
	{ 0x240610, 0x240611, niyanpai_sizex_1_w },
	{ 0x240612, 0x240613, niyanpai_sizey_1_w },
	{ 0x240614, 0x240617, niyanpai_drawx_1_w },
	{ 0x240618, 0x24061b, niyanpai_drawy_1_w },

	{ 0x240620, 0x24063f, niyanpai_paltbl_1_w },

	{ 0x240800, 0x240801, niyanpai_gfxflag_2_w },
	{ 0x240802, 0x240805, niyanpai_scrollx_2_w },
	{ 0x240806, 0x240809, niyanpai_scrolly_2_w },
	{ 0x24080a, 0x24080f, niyanpai_radr_2_w },
	{ 0x240810, 0x240811, niyanpai_sizex_2_w },
	{ 0x240812, 0x240813, niyanpai_sizey_2_w },
	{ 0x240814, 0x240817, niyanpai_drawx_2_w },
	{ 0x240818, 0x24081b, niyanpai_drawy_2_w },

	{ 0x240820, 0x24083f, niyanpai_paltbl_2_w },

	{ 0x240a00, 0x240a01, niyanpai_paltblnum_0_w },
	{ 0x240c00, 0x240c01, niyanpai_paltblnum_1_w },
	{ 0x240e00, 0x240e01, niyanpai_paltblnum_2_w },

	{ 0xfffc00, 0xfffc0f, tmp68301_address_decoder_w },
	{ 0xfffc80, 0xfffc9f, tmp68301_interrupt_controller_w },
	{ 0xfffd00, 0xfffd0f, tmp68301_parallel_interface_w },
	{ 0xfffd80, 0xfffdaf, tmp68301_serial_interface_w },
	{ 0xfffe00, 0xfffe4f, tmp68301_timer_w },
MEMORY_END


static MEMORY_READ_START( sound_readmem )
	{ 0x0000, 0x77ff, MRA_ROM },
	{ 0x7800, 0x7fff, MRA_RAM },
	{ 0x8000, 0xffff, MRA_BANK1 },
MEMORY_END

static MEMORY_WRITE_START( sound_writemem )
	{ 0x0000, 0x77ff, MWA_ROM },
	{ 0x7800, 0x7fff, MWA_RAM },
MEMORY_END


static PORT_READ_START( sound_readport )
	{ 0x10, 0x13, z80ctc_0_r },
	{ 0x50, 0x50, tmpz84c011_0_pa_r },
	{ 0x51, 0x51, tmpz84c011_0_pb_r },
	{ 0x52, 0x52, tmpz84c011_0_pc_r },
	{ 0x30, 0x30, tmpz84c011_0_pd_r },
	{ 0x40, 0x40, tmpz84c011_0_pe_r },
	{ 0x54, 0x54, tmpz84c011_0_dir_pa_r },
	{ 0x55, 0x55, tmpz84c011_0_dir_pb_r },
	{ 0x56, 0x56, tmpz84c011_0_dir_pc_r },
	{ 0x34, 0x34, tmpz84c011_0_dir_pd_r },
	{ 0x44, 0x44, tmpz84c011_0_dir_pe_r },
PORT_END

static PORT_WRITE_START( sound_writeport )
	{ 0x10, 0x13, z80ctc_0_w },
	{ 0x50, 0x50, tmpz84c011_0_pa_w },
	{ 0x51, 0x51, tmpz84c011_0_pb_w },
	{ 0x52, 0x52, tmpz84c011_0_pc_w },
	{ 0x30, 0x30, tmpz84c011_0_pd_w },
	{ 0x40, 0x40, tmpz84c011_0_pe_w },
	{ 0x54, 0x54, tmpz84c011_0_dir_pa_w },
	{ 0x55, 0x55, tmpz84c011_0_dir_pb_w },
	{ 0x56, 0x56, tmpz84c011_0_dir_pc_w },
	{ 0x34, 0x34, tmpz84c011_0_dir_pd_w },
	{ 0x44, 0x44, tmpz84c011_0_dir_pe_w },
	{ 0x80, 0x80, YM3812_control_port_0_w },
	{ 0x81, 0x81, YM3812_write_port_0_w },
PORT_END


INPUT_PORTS_START( niyanpai )
	PORT_START	/* (0) DIPSW-A */
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x03, "1" )
	PORT_DIPSETTING(    0x02, "2" )
	PORT_DIPSETTING(    0x01, "3" )
	PORT_DIPSETTING(    0x00, "4" )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_3C ) )
	PORT_DIPNAME( 0x10, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x00, "Game Sounds" )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Cocktail ) )

	PORT_START	/* (1) DIPSW-B */
	PORT_DIPNAME( 0x01, 0x00, "Nudity" )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_BIT( 0x7e, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_DIPNAME( 0x80, 0x80, "Graphic ROM Test" )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START	/* (2) PORT 0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )		/* COIN1*/
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )		/* COIN2*/
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_START3 )		/* CREDIT CLEAR*/
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START2 )		/* START2*/
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_SERVICE2 )		/* ANALYZER*/
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START1 )		/* START1*/
/*	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_SERVICE3 )		*/ /* MEMORY RESET*/
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )		/* ?*/
 	PORT_SERVICE( 0x80, IP_ACTIVE_LOW )			/* TEST*/

	PORT_START	/* (3) PLAYER-1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* (4) PLAYER-2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END


static INTERRUPT_GEN( niyanpai_interrupt )
{
	cpu_set_irq_line(0, 1, HOLD_LINE);
}

static Z80_DaisyChain daisy_chain_sound[] =
{
	{ z80ctc_reset, z80ctc_interrupt, z80ctc_reti, 0 },	/* device 0 = CTC_1 */
	{ 0, 0, 0, -1 }		/* end mark */
};


static struct YM3812interface ym3812_interface =
{
	1,				/* 1 chip */
	4000000,			/* 4.00 MHz */
	{ 70 }
};

static struct DACinterface dac_interface =
{
	2,				/* 2 channels */
	{ 50, 75 },
};


static MACHINE_DRIVER_START( niyanpai )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000,12288000/2)		/* TMP68301, 6.144 MHz */
	MDRV_CPU_MEMORY(niyanpai_readmem,niyanpai_writemem)
	MDRV_CPU_VBLANK_INT(niyanpai_interrupt,1)

	MDRV_CPU_ADD(Z80, 8000000/1)	/* TMPZ84C011, 8.00 MHz */
	MDRV_CPU_CONFIG(daisy_chain_sound)
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
	MDRV_CPU_MEMORY(sound_readmem,sound_writemem)
	MDRV_CPU_PORTS(sound_readport,sound_writeport)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_MACHINE_INIT(niyanpai)
	MDRV_NVRAM_HANDLER(generic_0fill)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_PIXEL_ASPECT_RATIO_1_2)
	MDRV_SCREEN_SIZE(1024, 512)
	MDRV_VISIBLE_AREA(0, 640-1, 0, 240-1)
	MDRV_PALETTE_LENGTH(768)

	MDRV_VIDEO_START(niyanpai)
	MDRV_VIDEO_UPDATE(niyanpai)

	/* sound hardware */
	MDRV_SOUND_ADD(YM3812, ym3812_interface)
	MDRV_SOUND_ADD(DAC, dac_interface)
MACHINE_DRIVER_END


ROM_START( niyanpai )
	ROM_REGION( 0x40000, REGION_CPU1, 0 ) /* TMP68301 main program */
	ROM_LOAD16_BYTE( "npai_01.bin", 0x00000, 0x20000, CRC(a904e8a1) SHA1(77865d7b48cac96af1e3cac4a702f7de4b5ee82b) )
	ROM_LOAD16_BYTE( "npai_02.bin", 0x00001, 0x20000, CRC(244f9d6f) SHA1(afde18f32c4879a66c0707671d783c21c54cffa4) )

	ROM_REGION( 0x20000, REGION_CPU2, 0 ) /* TMPZ84C011 sound program */
	ROM_LOAD( "npai_03.bin", 0x000000, 0x20000, CRC(d154306b) SHA1(3375568a6d387d850b8996b8bad3d0220de13993) )

	ROM_REGION( 0x400000, REGION_GFX1, 0 ) /* gfx */
	ROM_LOAD( "npai_04.bin", 0x000000, 0x80000, CRC(bec845b5) SHA1(2b00b4fd0bdda84cdc08933e593afdd91dde8d07) )
	ROM_LOAD( "npai_05.bin", 0x080000, 0x80000, CRC(3300ce07) SHA1(dc2eeb804aaf0aeb6cfee1844260ea24c3164bd9) )
	ROM_LOAD( "npai_06.bin", 0x100000, 0x80000, CRC(448e4e39) SHA1(63ca27f76a23235d3538d7f6c18dcc309e0f1f1c) )
	ROM_LOAD( "npai_07.bin", 0x180000, 0x80000, CRC(2ad47e55) SHA1(dbda82e654a85b0d5303bffa3005aaf78bdf0d28) )
	ROM_LOAD( "npai_08.bin", 0x200000, 0x80000, CRC(2ff980a0) SHA1(055addac657a5f7ec37ba85385834805c7aa0402) )
	ROM_LOAD( "npai_09.bin", 0x280000, 0x80000, CRC(74037ee3) SHA1(d975e6af962b9c62304ac15adab46c0ce972194b) )
	ROM_LOAD( "npai_10.bin", 0x300000, 0x80000, CRC(d35a9af6) SHA1(9a41aeea84c59b194bd122e2f102476834303302) )
	ROM_LOAD( "npai_11.bin", 0x380000, 0x80000, CRC(0748eb73) SHA1(63849f6625928646238a76748fd7903cee3ece2e) )
ROM_END


GAME( 1996, niyanpai, 0, niyanpai, niyanpai, niyanpai, ROT0, "Nichibutsu", "Niyanpai (Japan)" )
