/***************************************************************************
	Midway/Williams Audio Boards
	----------------------------
	6809 MEMORY MAP
	Function                                  Address     R/W  Data
	---------------------------------------------------------------
	Program RAM                               0000-07FF   R/W  D0-D7
	Music (YM-2151)                           2000-2001   R/W  D0-D7
	6821 PIA                                  4000-4003   R/W  D0-D7
	HC55516 clock low, digit latch            6000        W    D0
	HC55516 clock high                        6800        W    xx
	Bank select                               7800        W    D0-D2
	Banked Program ROM                        8000-FFFF   R    D0-D7
****************************************************************************/

#include "driver.h"
#include "machine/6821pia.h"
#include "cpu/m6809/m6809.h"
#include "williams.h"
#include "ost_samples.h"



/***************************************************************************
	STATIC GLOBALS
****************************************************************************/


UINT8 williams_sound_int_state;

static INT8 sound_cpunum;
static INT8 soundalt_cpunum;
static UINT8 williams_pianum;

static UINT8 adpcm_bank_count;



/***************************************************************************
	PROTOTYPES
****************************************************************************/

static void init_audio_state(void);

static void cvsd_ym2151_irq(int state);
static void adpcm_ym2151_irq(int state);
static void cvsd_irqa(int state);
static void cvsd_irqb(int state);

static READ_HANDLER( cvsd_pia_r );

static WRITE_HANDLER( cvsd_pia_w );
static WRITE_HANDLER( cvsd_bank_select_w );

static READ_HANDLER( adpcm_command_r );
static WRITE_HANDLER( adpcm_bank_select_w );
static WRITE_HANDLER( adpcm_6295_bank_select_w );

static READ_HANDLER( narc_command_r );
static READ_HANDLER( narc_command2_r );
static WRITE_HANDLER( narc_command2_w );
static WRITE_HANDLER( narc_master_bank_select_w );
static WRITE_HANDLER( narc_slave_bank_select_w );



/***************************************************************************
	PROCESSOR STRUCTURES
****************************************************************************/

/* CVSD readmem/writemem structures */
MEMORY_READ_START( williams_cvsd_readmem )
	{ 0x0000, 0x07ff, MRA_RAM },
	{ 0x2000, 0x2001, YM2151_status_port_0_r },
	{ 0x4000, 0x4003, cvsd_pia_r },
	{ 0x8000, 0xffff, MRA_BANK6 },
MEMORY_END


MEMORY_WRITE_START( williams_cvsd_writemem )
	{ 0x0000, 0x07ff, MWA_RAM },
	{ 0x2000, 0x2000, YM2151_register_port_0_w },
	{ 0x2001, 0x2001, YM2151_data_port_0_w },
	{ 0x4000, 0x4003, cvsd_pia_w },
	{ 0x6000, 0x6000, hc55516_0_digit_clock_clear_w },
	{ 0x6800, 0x6800, hc55516_0_clock_set_w },
	{ 0x7800, 0x7800, cvsd_bank_select_w },
	{ 0x8000, 0xffff, MWA_ROM },
MEMORY_END



/* ADPCM readmem/writemem structures */
MEMORY_READ_START( williams_adpcm_readmem )
	{ 0x0000, 0x1fff, MRA_RAM },
	{ 0x2401, 0x2401, YM2151_status_port_0_r },
	{ 0x2c00, 0x2c00, OKIM6295_status_0_r },
	{ 0x3000, 0x3000, adpcm_command_r },
	{ 0x4000, 0xbfff, MRA_BANK6 },
	{ 0xc000, 0xffff, MRA_ROM },
MEMORY_END


MEMORY_WRITE_START( williams_adpcm_writemem )
	{ 0x0000, 0x1fff, MWA_RAM },
	{ 0x2000, 0x2000, adpcm_bank_select_w },
	{ 0x2400, 0x2400, YM2151_register_port_0_w },
	{ 0x2401, 0x2401, YM2151_data_port_0_w },
	{ 0x2800, 0x2800, DAC_0_data_w },
	{ 0x2c00, 0x2c00, OKIM6295_data_0_w },
	{ 0x3400, 0x3400, adpcm_6295_bank_select_w },
	{ 0x3c00, 0x3c00, MWA_NOP },/*mk_sound_talkback_w }, -- talkback port? */
	{ 0x4000, 0xffff, MWA_ROM },
MEMORY_END



/* NARC master readmem/writemem structures */
MEMORY_READ_START( williams_narc_master_readmem )
	{ 0x0000, 0x1fff, MRA_RAM },
	{ 0x2001, 0x2001, YM2151_status_port_0_r },
	{ 0x3000, 0x3000, MRA_NOP },
	{ 0x3400, 0x3400, narc_command_r },
	{ 0x4000, 0xbfff, MRA_BANK6 },
	{ 0xc000, 0xffff, MRA_ROM },
MEMORY_END


MEMORY_WRITE_START( williams_narc_master_writemem )
	{ 0x0000, 0x1fff, MWA_RAM },
	{ 0x2000, 0x2000, YM2151_register_port_0_w },
	{ 0x2001, 0x2001, YM2151_data_port_0_w },
	{ 0x2800, 0x2800, MWA_NOP },/*mk_sound_talkback_w }, -- talkback port? */
	{ 0x2c00, 0x2c00, narc_command2_w },
	{ 0x3000, 0x3000, DAC_0_data_w },
	{ 0x3800, 0x3800, narc_master_bank_select_w },
	{ 0x4000, 0xffff, MWA_ROM },
MEMORY_END



/* NARC slave readmem/writemem structures */
MEMORY_READ_START( williams_narc_slave_readmem )
	{ 0x0000, 0x1fff, MRA_RAM },
	{ 0x3000, 0x3000, MRA_NOP },
	{ 0x3400, 0x3400, narc_command2_r },
	{ 0x4000, 0xbfff, MRA_BANK5 },
	{ 0xc000, 0xffff, MRA_ROM },
MEMORY_END


MEMORY_WRITE_START( williams_narc_slave_writemem )
	{ 0x0000, 0x1fff, MWA_RAM },
	{ 0x2000, 0x2000, hc55516_0_clock_set_w },
	{ 0x2400, 0x2400, hc55516_0_digit_clock_clear_w },
	{ 0x3000, 0x3000, DAC_1_data_w },
	{ 0x3800, 0x3800, narc_slave_bank_select_w },
	{ 0x3c00, 0x3c00, MWA_NOP },
	{ 0x4000, 0xffff, MWA_ROM },
MEMORY_END



/* PIA structure */
static struct pia6821_interface cvsd_pia_intf =
{
	/*inputs : A/B,CA/B1,CA/B2 */ 0, 0, 0, 0, 0, 0,
	/*outputs: A/B,CA/B2       */ DAC_0_data_w, 0, 0, 0,
	/*irqs   : A/B             */ cvsd_irqa, cvsd_irqb
};



/***************************************************************************
	AUDIO STRUCTURES
****************************************************************************/

/* YM2151 structure (CVSD variant) */
static struct YM2151interface cvsd_ym2151_interface =
{
	1,			/* 1 chip */
	3579580,
	{ YM3012_VOL(10,MIXER_PAN_CENTER,10,MIXER_PAN_CENTER) },
	{ cvsd_ym2151_irq }
};


/* YM2151 structure (ADPCM variant) */
static struct YM2151interface adpcm_ym2151_interface =
{
	1,			/* 1 chip */
	3579580,
	{ YM3012_VOL(10,MIXER_PAN_CENTER,10,MIXER_PAN_CENTER) },
	{ adpcm_ym2151_irq }
};


/* DAC structure (single DAC variant) */
static struct DACinterface single_dac_interface =
{
	1,
	{ 50 }
};


/* DAC structure (double DAC variant) */
static struct DACinterface double_dac_interface =
{
	2,
	{ 50, 50 }
};


/* CVSD structure */
static struct hc55516_interface cvsd_interface =
{
	1,			/* 1 chip */
	{ 80 }
};


/* OKIM6295 structure(s) */
static struct OKIM6295interface adpcm_6295_interface =
{
	1,          	/* 1 chip */
	{ 8000 },       /* 8000 Hz frequency */
	{ REGION_SOUND1 },  /* memory */
	{ 50 }
};



/***************************************************************************
	MACHINE DRIVERS
****************************************************************************/

MACHINE_DRIVER_START( williams_cvsd_sound )
	MDRV_CPU_ADD_TAG("cvsd", M6809, 8000000/4)
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
	MDRV_CPU_MEMORY(williams_cvsd_readmem,williams_cvsd_writemem)

	MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
	MDRV_SOUND_ADD(YM2151, cvsd_ym2151_interface)
	MDRV_SOUND_ADD(DAC,    single_dac_interface)
	MDRV_SOUND_ADD(HC55516,cvsd_interface)
MACHINE_DRIVER_END


MACHINE_DRIVER_START( williams_adpcm_sound )
	MDRV_CPU_ADD_TAG("adpcm", M6809, 8000000/4)
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
	MDRV_CPU_MEMORY(williams_adpcm_readmem,williams_adpcm_writemem)

	MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
	MDRV_SOUND_ADD(YM2151,  adpcm_ym2151_interface)
	MDRV_SOUND_ADD(DAC,     single_dac_interface)
	MDRV_SOUND_ADD(OKIM6295,adpcm_6295_interface)
MACHINE_DRIVER_END


MACHINE_DRIVER_START( williams_narc_sound )
	MDRV_CPU_ADD_TAG("narc1", M6809, 8000000/4)
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
	MDRV_CPU_MEMORY(williams_narc_master_readmem,williams_narc_master_writemem)

	MDRV_CPU_ADD_TAG("narc2", M6809, 8000000/4)
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
	MDRV_CPU_MEMORY(williams_narc_slave_readmem,williams_narc_slave_writemem)

	MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
	MDRV_SOUND_ADD(YM2151, adpcm_ym2151_interface)
	MDRV_SOUND_ADD(DAC,    double_dac_interface)
	MDRV_SOUND_ADD(HC55516,cvsd_interface)
MACHINE_DRIVER_END



/***************************************************************************
static INLINES
****************************************************************************/

static INLINE UINT8 *get_cvsd_bank_base(int data)
{
	UINT8 *RAM = memory_region(REGION_CPU1 + sound_cpunum);
	int bank = data & 3;
	int quarter = (data >> 2) & 3;
	if (bank == 3) bank = 0;
	return &RAM[0x10000 + (bank * 0x20000) + (quarter * 0x8000)];
}


static INLINE UINT8 *get_adpcm_bank_base(int data)
{
	UINT8 *RAM = memory_region(REGION_CPU1 + sound_cpunum);
	int bank = data & 7;
	return &RAM[0x10000 + (bank * 0x8000)];
}


static INLINE UINT8 *get_narc_master_bank_base(int data)
{
	UINT8 *RAM = memory_region(REGION_CPU1 + sound_cpunum);
	int bank = data & 3;
	if (!(data & 4)) bank = 0;
	return &RAM[0x10000 + (bank * 0x8000)];
}


static INLINE UINT8 *get_narc_slave_bank_base(int data)
{
	UINT8 *RAM = memory_region(REGION_CPU1 + soundalt_cpunum);
	int bank = data & 7;
	return &RAM[0x10000 + (bank * 0x8000)];
}



/***************************************************************************
	INITIALIZATION
****************************************************************************/

void williams_cvsd_init(int cpunum, int pianum)
{
	/* configure the CPU */
	sound_cpunum = mame_find_cpu_index("cvsd");
	soundalt_cpunum = -1;

	/* configure the PIA */
	williams_pianum = pianum;
	pia_config(pianum, PIA_STANDARD_ORDERING, &cvsd_pia_intf);

	/* initialize the global variables */
	init_audio_state();

	/* reset the chip */
	williams_cvsd_reset_w(1);
	williams_cvsd_reset_w(0);

	/* reset the IRQ state */
	pia_set_input_ca1(williams_pianum, 1);
}


void williams_adpcm_init(int cpunum)
{
	UINT8 *RAM;
	int i;

	/* configure the CPU */
	sound_cpunum = mame_find_cpu_index("adpcm");
	soundalt_cpunum = -1;

	/* install the fixed ROM */
	RAM = memory_region(REGION_CPU1 + sound_cpunum);
	memcpy(&RAM[0xc000], &RAM[0x4c000], 0x4000);

	/* initialize the global variables */
	init_audio_state();

	/* reset the chip */
	williams_adpcm_reset_w(1);
	williams_adpcm_reset_w(0);

	/* find the number of banks in the ADPCM space */
	for (i = 0; i < MAX_SOUND; i++)
		if (Machine->drv->sound[i].sound_type == SOUND_OKIM6295)
		{
			struct OKIM6295interface *intf = (struct OKIM6295interface *)Machine->drv->sound[i].sound_interface;
			adpcm_bank_count = memory_region_length(intf->region[0]) / 0x40000;
		}
}


void williams_narc_init(int cpunum)
{
	UINT8 *RAM;

	/* configure the CPU */
	sound_cpunum = mame_find_cpu_index("narc1");
	soundalt_cpunum = mame_find_cpu_index("narc2");

	/* install the fixed ROM */
	RAM = memory_region(REGION_CPU1 + sound_cpunum);
	memcpy(&RAM[0xc000], &RAM[0x2c000], 0x4000);
	RAM = memory_region(REGION_CPU1 + soundalt_cpunum);
	memcpy(&RAM[0xc000], &RAM[0x4c000], 0x4000);

	/* initialize the global variables */
	init_audio_state();

	/* reset the chip */
	williams_narc_reset_w(1);
	williams_narc_reset_w(0);
}


static void init_audio_state(void)
{
	/* reset the YM2151 state */
	YM2151_sh_reset();

	/* clear all the interrupts */
	williams_sound_int_state = 0;
	if (sound_cpunum != -1)
	{
		cpu_set_irq_line(sound_cpunum, M6809_FIRQ_LINE, CLEAR_LINE);
		cpu_set_irq_line(sound_cpunum, M6809_IRQ_LINE, CLEAR_LINE);
		cpu_set_nmi_line(sound_cpunum, CLEAR_LINE);
	}
	if (soundalt_cpunum != -1)
	{
		cpu_set_irq_line(soundalt_cpunum, M6809_FIRQ_LINE, CLEAR_LINE);
		cpu_set_irq_line(soundalt_cpunum, M6809_IRQ_LINE, CLEAR_LINE);
		cpu_set_nmi_line(soundalt_cpunum, CLEAR_LINE);
	}
}


#if 0
static void locate_audio_hotspot(UINT8 *base, UINT16 start)
{
	int i;
	/* search for the loop that kills performance so we can optimize it */
	for (i = start; i < 0x10000; i++)
	{
		if (base[i + 0] == 0x1a && base[i + 1] == 0x50 &&			/* 1A 50       ORCC  #$0050  */
			base[i + 2] == 0x93 &&									/* 93 xx       SUBD  $xx     */
			base[i + 4] == 0xe3 && base[i + 5] == 0x4c &&			/* E3 4C       ADDD  $000C,U */
			base[i + 6] == 0x9e && base[i + 7] == base[i + 3] &&	/* 9E xx       LDX   $xx     */
			base[i + 8] == 0xaf && base[i + 9] == 0x4c &&			/* AF 4C       STX   $000C,U */
			base[i +10] == 0x1c && base[i +11] == 0xaf)				/* 1C AF       ANDCC #$00AF  */
		{
/*			counter.hotspot_start = i;*/
/*			counter.hotspot_stop = i + 12;*/
			log_cb(RETRO_LOG_DEBUG, LOGPRE "Found hotspot @ %04X", i);
			return;
		}
	}
	log_cb(RETRO_LOG_DEBUG, LOGPRE "Found no hotspot!");
}
#endif



/***************************************************************************
	CVSD IRQ GENERATION CALLBACKS
****************************************************************************/

static void cvsd_ym2151_irq(int state)
{
	pia_set_input_ca1(williams_pianum, !state);
}


static void cvsd_irqa(int state)
{
	cpu_set_irq_line(sound_cpunum, M6809_FIRQ_LINE, state ? ASSERT_LINE : CLEAR_LINE);
}


static void cvsd_irqb(int state)
{
	cpu_set_nmi_line(sound_cpunum, state ? ASSERT_LINE : CLEAR_LINE);
}



/***************************************************************************
	ADPCM IRQ GENERATION CALLBACKS
****************************************************************************/

static void adpcm_ym2151_irq(int state)
{
	cpu_set_irq_line(sound_cpunum, M6809_FIRQ_LINE, state ? ASSERT_LINE : CLEAR_LINE);
}



/***************************************************************************
	CVSD BANK SELECT
****************************************************************************/

static WRITE_HANDLER( cvsd_bank_select_w )
{
	cpu_setbank(6, get_cvsd_bank_base(data));
}



/***************************************************************************
	ADPCM BANK SELECT
****************************************************************************/

static WRITE_HANDLER( adpcm_bank_select_w )
{
	cpu_setbank(6, get_adpcm_bank_base(data));
}


static WRITE_HANDLER( adpcm_6295_bank_select_w )
{
	if (adpcm_bank_count <= 3)
	{
		if (!(data & 0x04))
			OKIM6295_set_bank_base(0, 0x00000);
		else if (data & 0x01)
			OKIM6295_set_bank_base(0, 0x40000);
		else
			OKIM6295_set_bank_base(0, 0x80000);
	}
	else
	{
		data &= 7;
		if (data != 0)
			OKIM6295_set_bank_base(0, (data - 1) * 0x40000);
	}
}



/***************************************************************************
	NARC BANK SELECT
****************************************************************************/

static WRITE_HANDLER( narc_master_bank_select_w )
{
	cpu_setbank(6, get_narc_master_bank_base(data));
}


static WRITE_HANDLER( narc_slave_bank_select_w )
{
	cpu_setbank(5, get_narc_slave_bank_base(data));
}



/***************************************************************************
	PIA INTERFACES
****************************************************************************/

static READ_HANDLER( cvsd_pia_r )
{
	return pia_read(williams_pianum, offset);
}


static WRITE_HANDLER( cvsd_pia_w )
{
	pia_write(williams_pianum, offset, data);
}



/***************************************************************************
	CVSD COMMUNICATIONS
****************************************************************************/

static void williams_cvsd_delayed_data_w(int param)
{
	pia_set_input_b(williams_pianum, param & 0xff);
	pia_set_input_cb1(williams_pianum, param & 0x100);
	pia_set_input_cb2(williams_pianum, param & 0x200);
}


void williams_cvsd_data_w(int data)
{
	timer_set(TIME_NOW, data, williams_cvsd_delayed_data_w);
}


void williams_cvsd_reset_w(int state)
{
	/* going high halts the CPU */
	if (state)
	{
		cvsd_bank_select_w(0, 0);
		init_audio_state();
		cpu_set_reset_line(sound_cpunum, ASSERT_LINE);
	}
	/* going low resets and reactivates the CPU */
	else
		cpu_set_reset_line(sound_cpunum, CLEAR_LINE);
}



/***************************************************************************
	ADPCM COMMUNICATIONS
****************************************************************************/

static READ_HANDLER( adpcm_command_r )
{
	cpu_set_irq_line(sound_cpunum, M6809_IRQ_LINE, CLEAR_LINE);
	williams_sound_int_state = 0;
	return soundlatch_r(0);
}

void williams_adpcm_data_w(int data)
{
	if( ost_support_enabled(OST_SUPPORT_NBA_JAM) ) {
		if(generate_ost_sound( data )) soundlatch_w(0, data & 0xff);
	}
	else if( ost_support_enabled(OST_SUPPORT_MK) ) {
		if(generate_ost_sound( data )) soundlatch_w(0, data & 0xff);
	}
	else {
		soundlatch_w(0, data & 0xff);
	}

	if (!(data & 0x200))
	{
		cpu_set_irq_line(sound_cpunum, M6809_IRQ_LINE, ASSERT_LINE);
		williams_sound_int_state = 1;
	}
}


void williams_adpcm_reset_w(int state)
{
	/* going high halts the CPU */
	if (state)
	{
		adpcm_bank_select_w(0, 0);
		init_audio_state();
		cpu_set_reset_line(sound_cpunum, ASSERT_LINE);
	}
	/* going low resets and reactivates the CPU */
	else
		cpu_set_reset_line(sound_cpunum, CLEAR_LINE);
}



/***************************************************************************
	NARC COMMUNICATIONS
****************************************************************************/

static READ_HANDLER( narc_command_r )
{
	cpu_set_nmi_line(sound_cpunum, CLEAR_LINE);
	cpu_set_irq_line(sound_cpunum, M6809_IRQ_LINE, CLEAR_LINE);
	williams_sound_int_state = 0;
	return soundlatch_r(0);
}


void williams_narc_data_w(int data)
{
	soundlatch_w(0, data & 0xff);
	if (!(data & 0x100))
		cpu_set_nmi_line(sound_cpunum, ASSERT_LINE);
	if (!(data & 0x200))
	{
		cpu_set_irq_line(sound_cpunum, M6809_IRQ_LINE, ASSERT_LINE);
		williams_sound_int_state = 1;
	}
}


void williams_narc_reset_w(int state)
{
	/* going high halts the CPU */
	if (state)
	{
		narc_master_bank_select_w(0, 0);
		narc_slave_bank_select_w(0, 0);
		init_audio_state();
		cpu_set_reset_line(sound_cpunum, ASSERT_LINE);
		cpu_set_reset_line(soundalt_cpunum, ASSERT_LINE);
	}
	/* going low resets and reactivates the CPU */
	else
	{
		cpu_set_reset_line(sound_cpunum, CLEAR_LINE);
		cpu_set_reset_line(soundalt_cpunum, CLEAR_LINE);
	}
}


static READ_HANDLER( narc_command2_r )
{
	cpu_set_irq_line(soundalt_cpunum, M6809_FIRQ_LINE, CLEAR_LINE);
	return soundlatch2_r(0);
}


static WRITE_HANDLER( narc_command2_w )
{
	soundlatch2_w(0, data & 0xff);
	cpu_set_irq_line(soundalt_cpunum, M6809_FIRQ_LINE, ASSERT_LINE);
}
