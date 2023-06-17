/***************************************************************************

	Midway DCS Audio Board

****************************************************************************/

#include "driver.h"
#include "cpu/adsp2100/adsp2100.h"
#include "dcs.h"

#include <math.h>

#define LOG_DCS_TRANSFERS			(0)
#define LOG_DCS_IO					(0)
#define LOG_BUFFER_FILLING			(0)


bool activate_dcs_speedhack = 1; /* this is core maintaniers choice to always be active dont change */

/***************************************************************************
	CONSTANTS
****************************************************************************/

#define DCS_BUFFER_SIZE				4096
#define DCS_BUFFER_MASK				(DCS_BUFFER_SIZE - 1)

#define LCTRL_OUTPUT_EMPTY			0x400
#define LCTRL_INPUT_EMPTY			0x800

#define IS_OUTPUT_EMPTY()			(dcs.latch_control & LCTRL_OUTPUT_EMPTY)
#define IS_OUTPUT_FULL()			(!(dcs.latch_control & LCTRL_OUTPUT_EMPTY))
#define SET_OUTPUT_EMPTY()			(dcs.latch_control |= LCTRL_OUTPUT_EMPTY)
#define SET_OUTPUT_FULL()			(dcs.latch_control &= ~LCTRL_OUTPUT_EMPTY)

#define IS_INPUT_EMPTY()			(dcs.latch_control & LCTRL_INPUT_EMPTY)
#define IS_INPUT_FULL()				(!(dcs.latch_control & LCTRL_INPUT_EMPTY))
#define SET_INPUT_EMPTY()			(dcs.latch_control |= LCTRL_INPUT_EMPTY)
#define SET_INPUT_FULL()			(dcs.latch_control &= ~LCTRL_INPUT_EMPTY)

/* These are the some of the control register, we dont use them all */
enum
{
	S1_AUTOBUF_REG = 15,
	S1_RFSDIV_REG,
	S1_SCLKDIV_REG,
	S1_CONTROL_REG,
	S0_AUTOBUF_REG,
	S0_RFSDIV_REG,
	S0_SCLKDIV_REG,
	S0_CONTROL_REG,
	S0_MCTXLO_REG,
	S0_MCTXHI_REG,
	S0_MCRXLO_REG,
	S0_MCRXHI_REG,
	TIMER_SCALE_REG,
	TIMER_COUNT_REG,
	TIMER_PERIOD_REG,
	WAITSTATES_REG,
	SYSCONTROL_REG
};



/***************************************************************************
	STRUCTURES
****************************************************************************/

struct dcs_state
{
	int		stream;
	UINT8	auto_ack;

	UINT8 * mem;
	UINT16	size;
	UINT16	incs;
	void  * reg_timer;
	void  * sport_timer;
	int		ireg;
	UINT16	ireg_base;
	UINT16	control_regs[32];

	UINT16	rombank;
	UINT16	rombank_count;
	UINT16	srambank;
	UINT16	drambank;
	UINT16	drambank_count;
	UINT8	enabled;

	INT16 *	buffer;
	INT16 *	buffer2;
	UINT32	buffer_in;
	UINT32	sample_step;
	UINT32	sample_position;
	INT16	current_sample;

	UINT16	latch_control;
	UINT16	input_data;
	UINT16	output_data;
	UINT16	output_control;
	UINT32	output_control_cycles;
	
	UINT8	last_output_full;
	UINT8	last_input_empty;

	void	(*output_full_cb)(int);
	void	(*input_empty_cb)(int);
	UINT16	(*fifo_data_r)(void);
	UINT16	(*fifo_status_r)(void);
};



/***************************************************************************
	STATIC GLOBALS
****************************************************************************/

static INT8 dcs_cpunum;

static struct dcs_state dcs;

static data16_t *dcs_speedup1;
static data16_t *dcs_speedup2;
static data16_t *dcs_speedup3;
static data16_t *dcs_speedup4;

static data16_t *dcs_sram_bank0;
static data16_t *dcs_sram_bank1;
static data16_t *dcs_expanded_rom;

static data16_t *dcs_polling_base;

#if (LOG_DCS_TRANSFERS)
static data16_t *transfer_dest;
static int transfer_state;
static int transfer_start;
static int transfer_stop;
static int transfer_writes_left;
static UINT16 transfer_sum;
#endif



/***************************************************************************
	PROTOTYPES
****************************************************************************/

static int dcs_custom_start(const struct MachineSound *msound);
static int dcs2_custom_start(const struct MachineSound *msound);
static void dcs_dac_update(int num, INT16 *buffer, int length);
static void dcs2_dac_update(int num, INT16 **buffer, int length);

static READ16_HANDLER( dcs_sdrc_asic_ver_r );

static WRITE16_HANDLER( dcs_rombank_select_w );
static READ16_HANDLER( dcs_rombank_data_r );
static WRITE16_HANDLER( dcs_sram_bank_w );
static READ16_HANDLER( dcs_sram_bank_r );
static WRITE16_HANDLER( dcs_dram_bank_w );
static READ16_HANDLER( dcs_dram_bank_r );

static WRITE16_HANDLER( dcs_control_w );

static READ16_HANDLER( latch_status_r );
static READ16_HANDLER( fifo_input_r );
static READ16_HANDLER( input_latch_r );
static WRITE16_HANDLER( input_latch_ack_w );
static WRITE16_HANDLER( output_latch_w );
static READ16_HANDLER( output_control_r );
static WRITE16_HANDLER( output_control_w );

static void dcs_irq(int state);
static void sport0_irq(int state);
static void sound_tx_callback(int port, INT32 data);

static READ16_HANDLER( dcs_polling_r );

static WRITE16_HANDLER(dcs_speedup1_w);
static WRITE16_HANDLER(dcs_speedup2_w);
static WRITE16_HANDLER(dcs_speedup3_w);
static WRITE16_HANDLER(dcs_speedup4_w);
static void dcs_speedup_common(void);


/***************************************************************************
	PROCESSOR STRUCTURES
****************************************************************************/

/* DCS readmem/writemem structures */
MEMORY_READ16_START( dcs_readmem )
	{ ADSP_DATA_ADDR_RANGE(0x0000, 0x1fff), MRA16_RAM },			/* ??? */
	{ ADSP_DATA_ADDR_RANGE(0x2000, 0x2fff), dcs_rombank_data_r },	/* banked roms read */
	{ ADSP_DATA_ADDR_RANGE(0x3400, 0x3403), input_latch_r },		/* soundlatch read */
	{ ADSP_DATA_ADDR_RANGE(0x3800, 0x39ff), MRA16_RAM },			/* internal data ram */
	{ ADSP_PGM_ADDR_RANGE (0x0000, 0x1fff), MRA16_RAM },			/* internal/external program ram */
MEMORY_END


MEMORY_WRITE16_START( dcs_writemem )
	{ ADSP_DATA_ADDR_RANGE(0x0000, 0x1fff), MWA16_RAM },			/* ??? */
	{ ADSP_DATA_ADDR_RANGE(0x3000, 0x3000), dcs_rombank_select_w },	/* bank selector */
	{ ADSP_DATA_ADDR_RANGE(0x3400, 0x3403), output_latch_w },		/* soundlatch write */
	{ ADSP_DATA_ADDR_RANGE(0x3800, 0x39ff), MWA16_RAM },			/* internal data ram */
	{ ADSP_DATA_ADDR_RANGE(0x3fe0, 0x3fff), dcs_control_w },		/* adsp control regs */
	{ ADSP_PGM_ADDR_RANGE (0x0000, 0x1fff), MWA16_RAM },			/* internal/external program ram */
MEMORY_END



/* DCS with UART readmem/writemem structures */
MEMORY_READ16_START( dcs_uart_readmem )
	{ ADSP_DATA_ADDR_RANGE(0x0000, 0x1fff), MRA16_RAM },			/* ??? */
	{ ADSP_DATA_ADDR_RANGE(0x2000, 0x2fff), dcs_rombank_data_r },	/* banked roms read */
	{ ADSP_DATA_ADDR_RANGE(0x3400, 0x3402), MRA16_NOP },			/* UART (ignored) */
	{ ADSP_DATA_ADDR_RANGE(0x3403, 0x3403), input_latch_r },		/* soundlatch read */
	{ ADSP_DATA_ADDR_RANGE(0x3404, 0x3405), MRA16_NOP },			/* UART (ignored) */
	{ ADSP_DATA_ADDR_RANGE(0x3800, 0x39ff), MRA16_RAM },			/* internal data ram */
	{ ADSP_PGM_ADDR_RANGE (0x0000, 0x1fff), MRA16_RAM },			/* internal/external program ram */
MEMORY_END


MEMORY_WRITE16_START( dcs_uart_writemem )
	{ ADSP_DATA_ADDR_RANGE(0x0000, 0x1fff), MWA16_RAM },			/* ??? */
	{ ADSP_DATA_ADDR_RANGE(0x3000, 0x3000), dcs_rombank_select_w },	/* bank selector */
	{ ADSP_DATA_ADDR_RANGE(0x3400, 0x3402), MWA16_NOP },			/* UART (ignored) */
	{ ADSP_DATA_ADDR_RANGE(0x3403, 0x3403), output_latch_w },		/* soundlatch write */
	{ ADSP_DATA_ADDR_RANGE(0x3404, 0x3405), MWA16_NOP },			/* UART (ignored) */
	{ ADSP_DATA_ADDR_RANGE(0x3800, 0x39ff), MWA16_RAM },			/* internal data ram */
	{ ADSP_DATA_ADDR_RANGE(0x3fe0, 0x3fff), dcs_control_w },		/* adsp control regs */
	{ ADSP_PGM_ADDR_RANGE (0x0000, 0x1fff), MWA16_RAM },			/* internal/external program ram */
MEMORY_END



/* DCS2-based readmem/writemem structures */
MEMORY_READ16_START( dcs2_readmem )
	{ ADSP_DATA_ADDR_RANGE(0x0000, 0x03ff), MRA16_BANK20 },			/* D/RAM */
	{ ADSP_DATA_ADDR_RANGE(0x0400, 0x0400), input_latch_r },		/* input latch read */
	{ ADSP_DATA_ADDR_RANGE(0x0402, 0x0402), output_control_r },		/* secondary soundlatch read */
	{ ADSP_DATA_ADDR_RANGE(0x0403, 0x0403), latch_status_r },		/* latch status read */
	{ ADSP_DATA_ADDR_RANGE(0x0404, 0x0407), fifo_input_r },			/* FIFO input read */
	{ ADSP_DATA_ADDR_RANGE(0x0480, 0x0480), dcs_sram_bank_r },		/* S/RAM bank */
	{ ADSP_DATA_ADDR_RANGE(0x0481, 0x0481), MRA16_NOP },			/* LED in bit $2000 */
	{ ADSP_DATA_ADDR_RANGE(0x0482, 0x0482), dcs_dram_bank_r },		/* D/RAM bank */
	{ ADSP_DATA_ADDR_RANGE(0x0483, 0x0483), dcs_sdrc_asic_ver_r },	/* SDRC version number */
	{ ADSP_DATA_ADDR_RANGE(0x0800, 0x17ff), MRA16_RAM },			/* S/RAM */
	{ ADSP_DATA_ADDR_RANGE(0x1800, 0x27ff), MRA16_BANK21 },			/* banked S/RAM */
	{ ADSP_DATA_ADDR_RANGE(0x2800, 0x37ff), MRA16_RAM },			/* S/RAM */
	{ ADSP_DATA_ADDR_RANGE(0x3800, 0x39ff), MRA16_RAM },			/* internal data ram */
	{ ADSP_PGM_ADDR_RANGE (0x0000, 0x3fff), MRA16_RAM },			/* internal/external program ram */
MEMORY_END


MEMORY_WRITE16_START( dcs2_writemem )
	{ ADSP_DATA_ADDR_RANGE(0x0000, 0x03ff), MWA16_BANK20 },			/* D/RAM */
	{ ADSP_DATA_ADDR_RANGE(0x0400, 0x0400), input_latch_ack_w },	/* input latch ack */
	{ ADSP_DATA_ADDR_RANGE(0x0401, 0x0401), output_latch_w },		/* soundlatch write */
	{ ADSP_DATA_ADDR_RANGE(0x0402, 0x0402), output_control_w },		/* secondary soundlatch write */
	{ ADSP_DATA_ADDR_RANGE(0x0480, 0x0480), dcs_sram_bank_w },		/* S/RAM bank */
	{ ADSP_DATA_ADDR_RANGE(0x0481, 0x0481), MWA16_NOP },			/* LED in bit $2000 */
	{ ADSP_DATA_ADDR_RANGE(0x0482, 0x0482), dcs_dram_bank_w },		/* D/RAM bank */
	{ ADSP_DATA_ADDR_RANGE(0x0800, 0x17ff), MWA16_RAM },			/* S/RAM */
	{ ADSP_DATA_ADDR_RANGE(0x1800, 0x27ff), MWA16_BANK21, &dcs_sram_bank0 },/* banked S/RAM */
	{ ADSP_DATA_ADDR_RANGE(0x2800, 0x37ff), MWA16_RAM },			/* S/RAM */
	{ ADSP_DATA_ADDR_RANGE(0x3800, 0x39ff), MWA16_RAM },			/* internal data ram */
	{ ADSP_DATA_ADDR_RANGE(0x3fe0, 0x3fff), dcs_control_w },		/* adsp control regs */
	{ ADSP_PGM_ADDR_RANGE (0x0000, 0x3fff), MWA16_RAM },			/* internal/external program ram */
MEMORY_END



/***************************************************************************
	AUDIO STRUCTURES
****************************************************************************/

/* Custom structure (DCS variant) */
static struct CustomSound_interface dcs_custom_interface =
{
	dcs_custom_start,0,0
};

static struct CustomSound_interface dcs2_custom_interface =
{
	dcs2_custom_start,0,0
};



/***************************************************************************
	MACHINE DRIVERS
****************************************************************************/

MACHINE_DRIVER_START( dcs_audio )
	MDRV_CPU_ADD_TAG("dcs", ADSP2105, 10000000)
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
	MDRV_CPU_MEMORY(dcs_readmem,dcs_writemem)

	MDRV_SOUND_ADD(CUSTOM, dcs_custom_interface)
MACHINE_DRIVER_END


MACHINE_DRIVER_START( dcs_audio_uart )
	MDRV_IMPORT_FROM(dcs_audio)

	MDRV_CPU_MODIFY("dcs")
	MDRV_CPU_MEMORY(dcs_uart_readmem,dcs_uart_writemem)
MACHINE_DRIVER_END


MACHINE_DRIVER_START( dcs2_audio )
	MDRV_CPU_ADD_TAG("dcs2", ADSP2115, 16000000)
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
	MDRV_CPU_MEMORY(dcs2_readmem,dcs2_writemem)

	MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
	MDRV_SOUND_ADD(CUSTOM, dcs2_custom_interface)
MACHINE_DRIVER_END


MACHINE_DRIVER_START( dcs2_audio_2104 )
	MDRV_IMPORT_FROM(dcs2_audio)
	MDRV_CPU_REPLACE("dcs2", ADSP2104, 16000000)
MACHINE_DRIVER_END



/***************************************************************************
	INITIALIZATION
****************************************************************************/

static void dcs_boot(void)
{
	data8_t *src = (data8_t *)(memory_region(REGION_CPU1 + dcs_cpunum) + ADSP2100_SIZE);
	data32_t *dst = (data32_t *)(memory_region(REGION_CPU1 + dcs_cpunum) + ADSP2100_PGM_OFFSET);
	switch (Machine->drv->cpu[dcs_cpunum].cpu_type)
	{
		case CPU_ADSP2104:
			adsp2104_load_boot_data(src + 0x2000 * ((dcs.control_regs[SYSCONTROL_REG] >> 6) & 7), dst);
			break;
		case CPU_ADSP2105:
			adsp2105_load_boot_data(src + (dcs.rombank & 0x7ff) * 0x1000, dst);
			break;
		case CPU_ADSP2115:
			adsp2115_load_boot_data(src + (dcs.rombank & 0x7ff) * 0x1000, dst);
			break;
	}
}


static void dcs_reset(void)
{
	int i;

	/* initialize our state structure and install the transmit callback */
	dcs.mem = 0;
	dcs.size = 0;
	dcs.incs = 0;
	dcs.ireg = 0;

	/* initialize the ADSP control regs */
	for (i = 0; i < sizeof(dcs.control_regs) / sizeof(dcs.control_regs[0]); i++)
		dcs.control_regs[i] = 0;

	/* initialize banking */
	dcs.rombank = 0;
	dcs.srambank = 0;
	dcs.drambank = 0;
	if (dcs_sram_bank0)
	{
		cpu_setbank(20, memory_region(REGION_CPU1 + dcs_cpunum) + ADSP2100_SIZE + 0x8000);
		cpu_setbank(21, dcs_sram_bank0);
	}

	/* start with no sound output */
	dcs.enabled = 0;

	/* reset DAC generation */
	dcs.buffer_in = 0;
	dcs.sample_step = 0x10000;
	dcs.sample_position = 0;
	dcs.current_sample = 0;

	/* initialize the ADSP Tx callback */
	adsp2105_set_tx_callback(sound_tx_callback);

	/* clear all interrupts */
	cpu_set_irq_line(dcs_cpunum, ADSP2105_IRQ0, CLEAR_LINE);
	cpu_set_irq_line(dcs_cpunum, ADSP2105_IRQ1, CLEAR_LINE);
	cpu_set_irq_line(dcs_cpunum, ADSP2105_IRQ2, CLEAR_LINE);

	/* initialize the comm bits */
	SET_INPUT_EMPTY();
	SET_OUTPUT_EMPTY();
	if (!dcs.last_input_empty && dcs.input_empty_cb)
		(*dcs.input_empty_cb)(dcs.last_input_empty = 1);
	if (dcs.last_output_full && dcs.output_full_cb)
		(*dcs.output_full_cb)(dcs.last_output_full = 0);

	/* boot */
	dcs.control_regs[SYSCONTROL_REG] = 0;
	dcs_boot();

	/* start the SPORT0 timer */
	if (dcs.sport_timer)
		timer_adjust(dcs.sport_timer, TIME_IN_HZ(1000), 0, TIME_IN_HZ(1000));
}


void dcs_init(void)
{
	/* find the DCS CPU */
	dcs_cpunum = mame_find_cpu_index("dcs");
	
	/* reset RAM-based variables */
	dcs_sram_bank0 = dcs_sram_bank1 = NULL;


	/* install the speedup handler */
   if (activate_dcs_speedhack)
   {
      dcs_speedup1 = install_mem_write16_handler(dcs_cpunum, ADSP_DATA_ADDR_RANGE(0x04f8, 0x04f8), dcs_speedup1_w);
      dcs_speedup2 = install_mem_write16_handler(dcs_cpunum, ADSP_DATA_ADDR_RANGE(0x063d, 0x063d), dcs_speedup2_w);
      dcs_speedup3 = install_mem_write16_handler(dcs_cpunum, ADSP_DATA_ADDR_RANGE(0x063a, 0x063a), dcs_speedup3_w);
      dcs_speedup4 = install_mem_write16_handler(dcs_cpunum, ADSP_DATA_ADDR_RANGE(0x0641, 0x0641), dcs_speedup4_w);
   }

	/* create the timer */
	dcs.reg_timer = timer_alloc(dcs_irq);
	dcs.sport_timer = NULL;
	
	/* disable notification by default */
	dcs.output_full_cb = NULL;
	dcs.input_empty_cb = NULL;
	
	/* non-RAM based automatically acks */
	dcs.auto_ack = 1;

	/* reset the system */
	dcs_reset();
}


void dcs2_init(offs_t polling_offset)
{
	UINT8 *romsrc;
	int page, i;

	/* find the DCS CPU */
	dcs_cpunum = mame_find_cpu_index("dcs2");

	/* borrow memory for the extra 8k */
	dcs_sram_bank1 = (UINT16 *)(memory_region(REGION_CPU1 + dcs_cpunum) + 0x8000);
	
	/* borrow memory also for the expanded ROM data and expand it */
	romsrc = memory_region(REGION_CPU1 + dcs_cpunum) + ADSP2100_SIZE;
	dcs_expanded_rom = (UINT16 *)(memory_region(REGION_CPU1 + dcs_cpunum) + 0xc000);
	for (page = 0; page < 8; page++)
		for (i = 0; i < 0x400; i++)
			dcs_expanded_rom[0x400 * page + i] = romsrc[BYTE_XOR_LE(0x1000 * page + i)];
	
    /* install the speedup handler */
    if (activate_dcs_speedhack)
    {
      dcs_speedup1 = install_mem_write16_handler(dcs_cpunum, ADSP_DATA_ADDR_RANGE(0x04f8, 0x04f8), dcs_speedup1_w);
      dcs_speedup2 = install_mem_write16_handler(dcs_cpunum, ADSP_DATA_ADDR_RANGE(0x063d, 0x063d), dcs_speedup2_w);
	    dcs_speedup3 = install_mem_write16_handler(dcs_cpunum, ADSP_DATA_ADDR_RANGE(0x063a, 0x063a), dcs_speedup3_w);
      dcs_speedup4 = install_mem_write16_handler(dcs_cpunum, ADSP_DATA_ADDR_RANGE(0x0641, 0x0641), dcs_speedup4_w);
    }
	
	/* create the timer */
	dcs.reg_timer = timer_alloc(dcs_irq);
	dcs.sport_timer = timer_alloc(sport0_irq);

	/* RAM based doesn't do auto-ack, but it has a FIFO */
	dcs.auto_ack = 0;
	dcs.output_full_cb = NULL;
	dcs.input_empty_cb = NULL;
	dcs.fifo_data_r = NULL;
	dcs.fifo_status_r = NULL;
	
	/* install the speedup handler */
	if (polling_offset)
		dcs_polling_base = install_mem_read16_handler(dcs_cpunum, ADSP_DATA_ADDR_RANGE(polling_offset, polling_offset), dcs_polling_r);

	/* reset the system */
	dcs_reset();
}


void dcs_set_auto_ack(int state)
{
	dcs.auto_ack = state;
}



/***************************************************************************
	CUSTOM SOUND INTERFACES
****************************************************************************/

static int dcs_custom_start(const struct MachineSound *msound)
{
	/* allocate a DAC stream */
	dcs.stream = stream_init("DCS DAC", 100, Machine->sample_rate, 0, dcs_dac_update);

	/* allocate memory for our buffer */
	dcs.buffer = auto_malloc(DCS_BUFFER_SIZE * sizeof(INT16));
	dcs.buffer2 = NULL;
	if (!dcs.buffer)
		return 1;

	return 0;
}


static int dcs2_custom_start(const struct MachineSound *msound)
{
	const char *names[] = { "DCS DAC R", "DCS DAC L" };
	int vols[] = { MIXER(100, MIXER_PAN_RIGHT), MIXER(100, MIXER_PAN_LEFT) };

	/* allocate a DAC stream */
	dcs.stream = stream_init_multi(2, names, vols, Machine->sample_rate, 0, dcs2_dac_update);

	/* allocate memory for our buffer */
	dcs.buffer = auto_malloc(DCS_BUFFER_SIZE * sizeof(INT16));
	dcs.buffer2 = auto_malloc(DCS_BUFFER_SIZE * sizeof(INT16));
	if (!dcs.buffer || !dcs.buffer2)
		return 1;

	return 0;
}



/***************************************************************************
	DCS ASIC VERSION
****************************************************************************/

static READ16_HANDLER( dcs_sdrc_asic_ver_r )
{
	return 0x5a03;
}



/***************************************************************************
	DCS ROM BANK SELECT
****************************************************************************/

static WRITE16_HANDLER( dcs_rombank_select_w )
{
	dcs.rombank = data & 0x7ff;

	/* bit 11 = sound board led */
#if 0
	set_led_status(2, data & 0x800);
#endif

   if (activate_dcs_speedhack)
   {
      /* they write 0x800 here just before entering the stall loop */
      if (data == 0x800)
      {
         /* calculate the next buffer address */
         int source = activecpu_get_reg(ADSP2100_I0 + dcs.ireg);
         int ar = source + dcs.size / 2;

         /* check for wrapping */
         if (ar >= (dcs.ireg_base + dcs.size))
            ar = dcs.ireg_base;

         /* set it */
         activecpu_set_reg(ADSP2100_AR, ar);

         /* go around the buffer syncing code, we sync manually */
         activecpu_set_reg(ADSP2100_PC, activecpu_get_pc() + 8);
         cpu_spinuntil_int();
      }
   }
}


static READ16_HANDLER( dcs_rombank_data_r )
{
	UINT8	*banks = memory_region(REGION_CPU1 + dcs_cpunum) + ADSP2100_SIZE;

	offset += (dcs.rombank & 0x7ff) << 12;
	return banks[BYTE_XOR_LE(offset)];
}



/***************************************************************************
	DCS STATIC RAM BANK SELECT
****************************************************************************/

static WRITE16_HANDLER( dcs_sram_bank_w )
{
	COMBINE_DATA(&dcs.srambank);
	cpu_setbank(21, (dcs.srambank & 0x1000) ? dcs_sram_bank1 : dcs_sram_bank0);

	/* it appears that the Vegas games also access the boot ROM via this location */	
	if (((dcs.srambank >> 7) & 7) == dcs.drambank)
		cpu_setbank(20, dcs_expanded_rom + ((dcs.srambank >> 7) & 7) * 0x400);
}


static READ16_HANDLER( dcs_sram_bank_r )
{
	return dcs.srambank;
}



/***************************************************************************
	DCS DRAM BANK SELECT
****************************************************************************/

static WRITE16_HANDLER( dcs_dram_bank_w )
{
	dcs.drambank = data;
	cpu_setbank(20, memory_region(REGION_CPU1 + dcs_cpunum) + ADSP2100_SIZE + 0x8000 + (dcs.drambank & 0x7ff) * 0x400*2);
}


static READ16_HANDLER( dcs_dram_bank_r )
{
	return dcs.drambank;
}



/***************************************************************************
	DCS COMMUNICATIONS
****************************************************************************/

void dcs_set_io_callbacks(void (*output_full_cb)(int), void (*input_empty_cb)(int))
{
	dcs.input_empty_cb = input_empty_cb;
	dcs.output_full_cb = output_full_cb;
}


void dcs_set_fifo_callbacks(UINT16 (*fifo_data_r)(void), UINT16 (*fifo_status_r)(void))
{
	dcs.fifo_data_r = fifo_data_r;
	dcs.fifo_status_r = fifo_status_r;
}


int dcs_control_r(void)
{
	/* only boost for DCS2 boards */
	if (!dcs.auto_ack)
		cpu_boost_interleave(TIME_IN_USEC(0.5), TIME_IN_USEC(5));
	return dcs.latch_control;
}


void dcs_reset_w(int state)
{
	/* going low halts the CPU */
	if (!state)
	{
		log_cb(RETRO_LOG_DEBUG, LOGPRE "%08x: DCS reset = %d\n", activecpu_get_pc(), state);

		/* just run through the init code again */
		dcs_reset();
		cpu_set_reset_line(dcs_cpunum, ASSERT_LINE);
	}
	
	/* going high resets and reactivates the CPU */
	else
		cpu_set_reset_line(dcs_cpunum, CLEAR_LINE);
}


static READ16_HANDLER( latch_status_r )
{
	int result = 0;
	if (IS_INPUT_FULL())
		result |= 0x80;
	if (IS_OUTPUT_EMPTY())
		result |= 0x40;
	if (dcs.fifo_status_r)
		result |= (*dcs.fifo_status_r)() & 0x38;
	return result;
}


static READ16_HANDLER( fifo_input_r )
{
	if (dcs.fifo_data_r)
		return (*dcs.fifo_data_r)();
	else
		return 0xffff;
}



/***************************************************************************
	INPUT LATCH (data from host to DCS)
****************************************************************************/

void dcs_data_w(int data)
{
#if (LOG_DCS_TRANSFERS)
	if (dcs.sport_timer)
		switch (transfer_state)
		{
			case 0:
				if (data == 0x55d0 || data == 0x55d1)
				{
					log_cb(RETRO_LOG_DEBUG, LOGPRE "DCS Transfer command %04X\n", data);
					transfer_state++;
				}
				else
					log_cb(RETRO_LOG_DEBUG, LOGPRE "Command: %04X\n", data);
				break;
				
			case 1:
				transfer_start = data << 16;
				transfer_state++;
				break;
					
			case 2:
				transfer_start |= data;
				transfer_state++;
				transfer_dest = (data16_t *)(memory_region(REGION_CPU1 + dcs_cpunum) + ADSP2100_SIZE + 0x8000 + transfer_start*2);
				log_cb(RETRO_LOG_DEBUG, LOGPRE "Start address = %08X\n", transfer_start);
				break;

			case 3:
				transfer_stop = data << 16;
				transfer_state++;
				break;

			case 4:
				transfer_stop |= data;
				transfer_state++;
				log_cb(RETRO_LOG_DEBUG, LOGPRE "Stop address = %08X\n", transfer_stop);
				transfer_writes_left = transfer_stop - transfer_start + 1;
				transfer_sum = 0;
				break;

			case 5:
				transfer_sum += data;
				if (--transfer_writes_left == 0)
				{
					log_cb(RETRO_LOG_DEBUG, LOGPRE "Transfer done, sum = %04X\n", transfer_sum);
					transfer_state = 0;
				}
				break;
		}
#endif

	log_cb(RETRO_LOG_DEBUG, LOGPRE "%08X:dcs_data_w(%04X)\n", activecpu_get_pc(), data);

	cpu_boost_interleave(TIME_IN_USEC(0.5), TIME_IN_USEC(5));
	cpu_set_irq_line(dcs_cpunum, ADSP2105_IRQ2, ASSERT_LINE);

	if (dcs.last_input_empty && dcs.input_empty_cb)
		(*dcs.input_empty_cb)(dcs.last_input_empty = 0);
	SET_INPUT_FULL();
	dcs.input_data = data;
}


static WRITE16_HANDLER( input_latch_ack_w )
{
	if (!dcs.last_input_empty && dcs.input_empty_cb)
		(*dcs.input_empty_cb)(dcs.last_input_empty = 1);
	SET_INPUT_EMPTY();
	cpu_set_irq_line(dcs_cpunum, ADSP2105_IRQ2, CLEAR_LINE);
}


static READ16_HANDLER( input_latch_r )
{
	if (dcs.auto_ack)
		input_latch_ack_w(0,0,0);
  log_cb(RETRO_LOG_DEBUG, LOGPRE "%08X:input_latch_r(%04X)\n", activecpu_get_pc(), dcs.input_data);
	return dcs.input_data;
}



/***************************************************************************
	OUTPUT LATCH (data from DCS to host)
****************************************************************************/

static void latch_delayed_w(int data)
{
	if (!dcs.last_output_full && dcs.output_full_cb)
		(*dcs.output_full_cb)(dcs.last_output_full = 1);
	SET_OUTPUT_FULL();
	dcs.output_data = data;
}


static WRITE16_HANDLER( output_latch_w )
{
	log_cb(RETRO_LOG_DEBUG, LOGPRE "%08X:output_latch_w(%04X) (empty=%d)\n", activecpu_get_pc(), data, IS_OUTPUT_EMPTY());
	timer_set(TIME_NOW, data, latch_delayed_w);
}


static void delayed_ack_w(int param)
{
	SET_OUTPUT_EMPTY();
}


void dcs_ack_w(void)
{
	timer_set(TIME_NOW, 0, delayed_ack_w);
}


int dcs_data_r(void)
{
	/* data is actually only 8 bit (read from d8-d15) */
	if (dcs.last_output_full && dcs.output_full_cb)
		(*dcs.output_full_cb)(dcs.last_output_full = 0);
	if (dcs.auto_ack)
		delayed_ack_w(0);

	log_cb(RETRO_LOG_DEBUG, LOGPRE "%08X:dcs_data_r(%04X)\n", activecpu_get_pc(), dcs.output_data);
	return dcs.output_data;
}



/***************************************************************************
	OUTPUT CONTROL BITS (has 3 additional lines to the host)
****************************************************************************/

static void output_control_delayed_w(int data)
{
	log_cb(RETRO_LOG_DEBUG, LOGPRE "output_control = %04X\n", data);
	dcs.output_control = data;
	dcs.output_control_cycles = 0;
}


static WRITE16_HANDLER( output_control_w )
{
	log_cb(RETRO_LOG_DEBUG, LOGPRE "%04X:output_control = %04X\n", activecpu_get_pc(), data);
	timer_set(TIME_NOW, data, output_control_delayed_w);
}


static READ16_HANDLER( output_control_r )
{
	dcs.output_control_cycles = activecpu_gettotalcycles();
	return dcs.output_control;
}


int dcs_data2_r(void)
{
	return dcs.output_control;
}



/***************************************************************************
	SOUND GENERATION
****************************************************************************/

static void dcs_dac_update(int num, INT16 *buffer, int length)
{
	UINT32 current, step, indx;
	INT16 *source;
	int i;

	/* DAC generation */
	if (dcs.enabled)
	{
		source = dcs.buffer;
		current = dcs.sample_position;
		step = dcs.sample_step;

		/* fill in with samples until we hit the end or run out */
		for (i = 0; i < length; i++)
		{
			indx = current >> 16;
			if (indx >= dcs.buffer_in)
				break;
			current += step;
			*buffer++ = source[indx & DCS_BUFFER_MASK];
		}

		if (i < length)
			log_cb(RETRO_LOG_DEBUG, LOGPRE "DCS ran out of input data\n");

		/* fill the rest with the last sample */
		for ( ; i < length; i++)
			*buffer++ = source[(dcs.buffer_in - 1) & DCS_BUFFER_MASK];

		/* mask off extra bits */
		while (current >= (DCS_BUFFER_SIZE << 16))
		{
			current -= DCS_BUFFER_SIZE << 16;
			dcs.buffer_in -= DCS_BUFFER_SIZE;
		}

    log_cb(RETRO_LOG_DEBUG, LOGPRE "DCS dac update: bytes in buffer = %d\n", dcs.buffer_in - (current >> 16));

		/* update the final values */
		dcs.sample_position = current;
	}
	else
		memset(buffer, 0, length * sizeof(INT16));
}


static void dcs2_dac_update(int num, INT16 **buffer, int length)
{
	INT16 *destl = buffer[0], *destr = buffer[1];
	UINT32 current, step, indx;
	INT16 *sourcel, *sourcer;
	int i;

	/* DAC generation */
	if (dcs.enabled)
	{
		sourcel = dcs.buffer;
		sourcer = dcs.buffer2;
		current = dcs.sample_position;
		step = dcs.sample_step;

		/* fill in with samples until we hit the end or run out */
		for (i = 0; i < length; i++)
		{
			indx = current >> 16;
			if (indx >= dcs.buffer_in)
				break;
			current += step;
			*destl++ = sourcel[indx & DCS_BUFFER_MASK];
			*destr++ = sourcer[indx & DCS_BUFFER_MASK];
		}

		if (i < length)
			log_cb(RETRO_LOG_DEBUG, LOGPRE "DCS ran out of input data\n");

		/* fill the rest with the last sample */
		for ( ; i < length; i++)
		{
			*destl++ = sourcel[(dcs.buffer_in - 1) & DCS_BUFFER_MASK];
			*destr++ = sourcer[(dcs.buffer_in - 1) & DCS_BUFFER_MASK];
		}

		/* mask off extra bits */
		while (current >= (DCS_BUFFER_SIZE << 16))
		{
			current -= DCS_BUFFER_SIZE << 16;
			dcs.buffer_in -= DCS_BUFFER_SIZE;
		}

    log_cb(RETRO_LOG_DEBUG, LOGPRE "DCS dac update: bytes in buffer = %d\n", dcs.buffer_in - (current >> 16));

		/* update the final values */
		dcs.sample_position = current;
	}
	else
	{
		memset(destl, 0, length * sizeof(INT16));
		memset(destr, 0, length * sizeof(INT16));
	}
}



/***************************************************************************
	ADSP CONTROL & TRANSMIT CALLBACK
****************************************************************************/

/*
	The ADSP2105 memory map when in boot rom mode is as follows:

	Program Memory:
	0x0000-0x03ff = Internal Program Ram (contents of boot rom gets copied here)
	0x0400-0x07ff = Reserved
	0x0800-0x3fff = External Program Ram

	Data Memory:
	0x0000-0x03ff = External Data - 0 Waitstates
	0x0400-0x07ff = External Data - 1 Waitstates
	0x0800-0x2fff = External Data - 2 Waitstates
	0x3000-0x33ff = External Data - 3 Waitstates
	0x3400-0x37ff = External Data - 4 Waitstates
	0x3800-0x39ff = Internal Data Ram
	0x3a00-0x3bff = Reserved (extra internal ram space on ADSP2101, etc)
	0x3c00-0x3fff = Memory Mapped control registers & reserved.
*/

static WRITE16_HANDLER( dcs_control_w )
{
	dcs.control_regs[offset] = data;
	switch (offset)
	{
		case SYSCONTROL_REG:
			if (data & 0x0200)
			{
				/* boot force */
				cpu_set_reset_line(dcs_cpunum, PULSE_LINE);
				dcs_boot();
				dcs.control_regs[SYSCONTROL_REG] &= ~0x0200;
			}

			/* see if SPORT1 got disabled */
			stream_update(dcs.stream, 0);
			if ((data & 0x0800) == 0)
			{
				dcs.enabled = 0;
				timer_adjust(dcs.reg_timer, TIME_NEVER, 0, 0);
			}
			break;

		case S1_AUTOBUF_REG:
			/* autobuffer off: nuke the timer, and disable the DAC */
			stream_update(dcs.stream, 0);
			if ((data & 0x0002) == 0)
			{
				dcs.enabled = 0;
				timer_adjust(dcs.reg_timer, TIME_NEVER, 0, 0);
			}
			break;

		case S1_CONTROL_REG:
			if (((data >> 4) & 3) == 2)
				log_cb(RETRO_LOG_DEBUG, LOGPRE "Oh no!, the data is compresed with u-law encoding\n");
			if (((data >> 4) & 3) == 3)
				log_cb(RETRO_LOG_DEBUG, LOGPRE "Oh no!, the data is compresed with A-law encoding\n");
			break;
	}
}



/***************************************************************************
	DCS IRQ GENERATION CALLBACKS
****************************************************************************/

static void dcs_irq(int state)
{
	/* get the index register */
	int reg = cpunum_get_reg(dcs_cpunum, ADSP2100_I0 + dcs.ireg);

	/* translate into data memory bus address */
	int source = ADSP2100_DATA_OFFSET + (reg << 1);
	int i;

	/* copy the current data into the buffer */
	if (!dcs.buffer2)
	{
		for (i = 0; i < dcs.size / 2; i += dcs.incs)
			dcs.buffer[dcs.buffer_in++ & DCS_BUFFER_MASK] = ((UINT16 *)&dcs.mem[source])[i];
	}
	else
	{
		for (i = 0; i < dcs.size / 2; i += dcs.incs * 2)
		{
			dcs.buffer[dcs.buffer_in & DCS_BUFFER_MASK] = ((UINT16 *)&dcs.mem[source])[i];
			dcs.buffer2[dcs.buffer_in & DCS_BUFFER_MASK] = ((UINT16 *)&dcs.mem[source])[i + dcs.incs];
			dcs.buffer_in++;
		}
	}

	/* increment it */
	reg += dcs.size / 2;

	/* check for wrapping */
	if (reg >= dcs.ireg_base + dcs.size)
	{
		/* reset the base pointer */
		reg = dcs.ireg_base;

		/* generate the (internal, thats why the pulse) irq */
		cpu_set_irq_line(dcs_cpunum, ADSP2105_IRQ1, PULSE_LINE);
	}

	/* store it */
	cpunum_set_reg(dcs_cpunum, ADSP2100_I0 + dcs.ireg, reg);
   
   if (activate_dcs_speedhack)
	/* this is the same trigger as an interrupt */
      cpu_triggerint(dcs_cpunum);
}


static void sport0_irq(int state)
{
	/* this latches internally, so we just pulse */
	/* note that there is non-interrupt code that reads/modifies/writes the output_control */
	/* register; if we don't interlock it, we will eventually lose sound (see CarnEvil) */
	/* so we skip the SPORT interrupt if we read with output_control within the last 5 cycles */
	if ((cpunum_gettotalcycles(dcs_cpunum) - dcs.output_control_cycles) > 5)
		cpu_set_irq_line(dcs_cpunum, ADSP2115_SPORT0_RX, PULSE_LINE);
}


static void sound_tx_callback(int port, INT32 data)
{
	/* check if it's for SPORT1 */
	if (port != 1)
		return;

	/* check if SPORT1 is enabled */
	if (dcs.control_regs[SYSCONTROL_REG] & 0x0800) /* bit 11 */
	{
		/* we only support autobuffer here (wich is what this thing uses), bail if not enabled */
		if (dcs.control_regs[S1_AUTOBUF_REG] & 0x0002) /* bit 1 */
		{
			/* get the autobuffer registers */
			int		mreg, lreg;
			UINT16	source;
			int		sample_rate;

			stream_update(dcs.stream, 0);

			dcs.ireg = (dcs.control_regs[S1_AUTOBUF_REG] >> 9) & 7;
			mreg = (dcs.control_regs[S1_AUTOBUF_REG] >> 7) & 3;
			mreg |= dcs.ireg & 0x04; /* msb comes from ireg */
			lreg = dcs.ireg;

			/* now get the register contents in a more legible format */
			/* we depend on register indexes to be continuous (wich is the case in our core) */
			source = cpunum_get_reg(dcs_cpunum, ADSP2100_I0 + dcs.ireg);
			dcs.incs = cpunum_get_reg(dcs_cpunum, ADSP2100_M0 + mreg);
			dcs.size = cpunum_get_reg(dcs_cpunum, ADSP2100_L0 + lreg);

			/* get the base value, since we need to keep it around for wrapping */
			source -= dcs.incs;

			/* make it go back one so we dont lose the first sample */
			cpunum_set_reg(dcs_cpunum, ADSP2100_I0 + dcs.ireg, source);

			/* save it as it is now */
			dcs.ireg_base = source;

			/* get the memory chunk to read the data from */
			dcs.mem = memory_region(REGION_CPU1 + dcs_cpunum);

			/* enable the dac playing */
			dcs.enabled = 1;

			/* calculate how long until we generate an interrupt */

			/* frequency in Hz per each bit sent */
			sample_rate = Machine->drv->cpu[dcs_cpunum].cpu_clock / (2 * (dcs.control_regs[S1_SCLKDIV_REG] + 1));

			/* now put it down to samples, so we know what the channel frequency has to be */
			sample_rate /= 16;
			if (dcs.buffer2)
				sample_rate /= 2;

			/* fire off a timer wich will hit every half-buffer */
			if (!dcs.buffer2)
				timer_adjust(dcs.reg_timer, TIME_IN_HZ(sample_rate) * (dcs.size / (2 * dcs.incs)), 0, TIME_IN_HZ(sample_rate) * (dcs.size / (2 * dcs.incs)));
			else
				timer_adjust(dcs.reg_timer, TIME_IN_HZ(sample_rate) * (dcs.size / (4 * dcs.incs)), 0, TIME_IN_HZ(sample_rate) * (dcs.size / (4 * dcs.incs)));

			/* configure the DAC generator */
			dcs.sample_step = (int)(sample_rate * 65536.0 / (double)Machine->sample_rate);
			dcs.sample_position = 0;
			dcs.buffer_in = 0;

			return;
		}
		else
			log_cb(RETRO_LOG_DEBUG, LOGPRE  "ADSP SPORT1: trying to transmit and autobuffer not enabled!\n" );
	}

	/* if we get there, something went wrong. Disable playing */
	stream_update(dcs.stream, 0);
	dcs.enabled = 0;

	/* remove timer */
	timer_adjust(dcs.reg_timer, TIME_NEVER, 0, 0);
}

/***************************************************************************
	DCS SPEEDUPS
****************************************************************************/

static WRITE16_HANDLER( dcs_speedup1_w )
{
/*
	MK3:     trigger = $04F8 = 2, PC = $00FD, SKIPTO = $0128
	UMK3:    trigger = $04F8 = 2, PC = $00FD, SKIPTO = $0128
	OPENICE: trigger = $04F8 = 2, PC = $00FD, SKIPTO = $0128
	WWFMANIA:trigger = $04F8 = 2, PC = $00FD, SKIPTO = $0128
	NBAHANGT:trigger = $04F8 = 2, PC = $00FD, SKIPTO = $0128
	NBAMAXHT:trigger = $04F8 = 2, PC = $00FD, SKIPTO = $0128
	RMPGWT:  trigger = $04F8 = 2, PC = $00FD, SKIPTO = $0128
*/
	COMBINE_DATA(&dcs_speedup1[offset]);
	if (data == 2 && activecpu_get_pc() == 0xfd)
		dcs_speedup_common();
}


static WRITE16_HANDLER( dcs_speedup2_w )
{
/*
	MK2:     trigger = $063D = 2, PC = $00F6, SKIPTO = $0121
	REVX:    trigger = $063D = 2, PC = $00F6, SKIPTO = $0121
	KINST:   trigger = $063D = 2, PC = $00F6, SKIPTO = $0121
*/
	COMBINE_DATA(&dcs_speedup2[offset]);
	if (data == 2 && activecpu_get_pc() == 0xf6)
		dcs_speedup_common();
}


static WRITE16_HANDLER( dcs_speedup3_w )
{
/*
	CRUSNUSA: trigger = $063A = 2, PC = $00E1, SKIPTO = $010C
*/
	COMBINE_DATA(&dcs_speedup3[offset]);
	if (data == 2 && activecpu_get_pc() == 0xe1)
		dcs_speedup_common();
}


static WRITE16_HANDLER( dcs_speedup4_w )
{
/*
	CRUSNWLD: trigger = $0641 = 2, PC = $00DA, SKIPTO = $0105
	OFFROADC: trigger = $0641 = 2, PC = $00DA, SKIPTO = $0105
*/
	COMBINE_DATA(&dcs_speedup4[offset]);
	if (data == 2 && activecpu_get_pc() == 0xda)
		dcs_speedup_common();
}


static void dcs_speedup_common(void)
{
/*
	00F4: AR = $0002
	00F5: DM($063D) = AR
	00F6: SI = $0040
	00F7: DM($063E) = SI
	00F8: SR = LSHIFT SI BY -1 (LO)
	00F9: DM($063F) = SR0
	00FA: M0 = $3FFF
	00FB: CNTR = $0006
	00FC: DO $0120 UNTIL CE
		00FD: I4 = $0780
		00FE: I5 = $0700
		00FF: I0 = $3800
		0100: I1 = $3800
		0101: AY0 = DM($063E)
		0102: M2 = AY0
		0103: MODIFY (I1,M2)
		0104: I2 = I1
		0105: AR = AY0 - 1
		0106: M3 = AR
		0107: CNTR = DM($063D)
		0108: DO $0119 UNTIL CE
			0109: CNTR = DM($063F)
			010A: MY0 = DM(I4,M5)
			010B: MY1 = DM(I5,M5)
			010C: MX0 = DM(I1,M1)
			010D: DO $0116 UNTIL CE
				010E: MR = MX0 * MY0 (SS), MX1 = DM(I1,M1)
				010F: MR = MR - MX1 * MY1 (RND), AY0 = DM(I0,M1)
				0110: MR = MX1 * MY0 (SS), AX0 = MR1
				0111: MR = MR + MX0 * MY1 (RND), AY1 = DM(I0,M0)
				0112: AR = AY0 - AX0, MX0 = DM(I1,M1)
				0113: AR = AX0 + AY0, DM(I0,M1) = AR
				0114: AR = AY1 - MR1, DM(I2,M1) = AR
				0115: AR = MR1 + AY1, DM(I0,M1) = AR
				0116: DM(I2,M1) = AR
			0117: MODIFY (I2,M2)
			0118: MODIFY (I1,M3)
			0119: MODIFY (I0,M2)
		011A: SI = DM($063D)
		011B: SR = LSHIFT SI BY 1 (LO)
		011C: DM($063D) = SR0
		011D: SI = DM($063F)
		011E: DM($063E) = SI
		011F: SR = LSHIFT SI BY -1 (LO)
		0120: DM($063F) = SR0
*/

	INT16 *source = (INT16 *)memory_region(REGION_CPU1 + dcs_cpunum);
	int mem63d = 2;
	int mem63e = 0x40;
	int mem63f = mem63e >> 1;
	int i, j, k;

	for (i = 0; i < 6; i++)
	{
		INT16 *i4 = &source[0x780];
		INT16 *i5 = &source[0x700];
		INT16 *i0 = &source[0x3800];
		INT16 *i1 = &source[0x3800 + mem63e];
		INT16 *i2 = i1;

		for (j = 0; j < mem63d; j++)
		{
			INT32 mx0, mx1, my0, my1, ax0, ay0, ay1, mr1, temp;

			my0 = *i4++;
			my1 = *i5++;

			for (k = 0; k < mem63f; k++)
			{
				mx0 = *i1++;
				mx1 = *i1++;
				ax0 = (mx0 * my0 - mx1 * my1) >> 15;
				mr1 = (mx1 * my0 + mx0 * my1) >> 15;
				ay0 = i0[0];
				ay1 = i0[1];

				temp = ay0 - ax0;
				/*if (temp < -32768) temp = -32768;*/
				/*else if (temp > 32767) temp = 32767;*/
				*i0++ = temp;

				temp = ax0 + ay0;
				/*if (temp < -32768) temp = -32768;*/
				/*else if (temp > 32767) temp = 32767;*/
				*i2++ = temp;

				temp = ay1 - mr1;
				/*if (temp < -32768) temp = -32768;*/
				/*else if (temp > 32767) temp = 32767;*/
				*i0++ = temp;

				temp = ay1 + mr1;
				/*if (temp < -32768) temp = -32768;*/
				/*else if (temp > 32767) temp = 32767;*/
				*i2++ = temp;
			}
			i2 += mem63e;
			i1 += mem63e;
			i0 += mem63e;
		}
		mem63d <<= 1;
		mem63e = mem63f;
		mem63f >>= 1;
	}
	activecpu_set_reg(ADSP2100_PC, activecpu_get_pc() + 0x121 - 0xf6);
}


/* War Gods:
2BA4: 0000      AR = $0002
2BA5: 0000      DM($0A09) = AR
2BA6: 0000      SI = $0040
2BA7: 0000      DM($0A0B) = SI
2BA8: 0000      SR = LSHIFT SI BY -1 (LO)
2BA9: 0000      DM($0A0D) = SR0
2BAA: 0000      M0 = $3FFF
2BAB: 0000      CNTR = $0006
2BAC: 0000      DO $2BD0 UNTIL CE
2BAD: 0000          I4 = $1080
2BAE: 0000          I5 = $1000
2BAF: 0000          I0 = $2000
2BB0: 0000          I1 = $2000
2BB1: 0000          AY0 = DM($0A0B)
2BB2: 0000          M2 = AY0
2BB3: 0000          MODIFY (I1,M2)
2BB4: 0000          I2 = I1
2BB5: 0000          AR = AY0 - 1
2BB6: 0000          M3 = AR
2BB7: 0000          CNTR = DM($0A09)
2BB8: 0000          DO $2BC9 UNTIL CE
2BB9: 0000              CNTR = DM($0A0D)
2BBA: 0000              MY0 = DM(I4,M5)
2BBB: 0000              MY1 = DM(I5,M5)
2BBC: 0000              MX0 = DM(I1,M1)
2BBD: 0000              DO $2BC6 UNTIL CE
2BBE: 0000                  MR = MX0 * MY0 (SS), MX1 = DM(I1,M1)
2BBF: 0000                  MR = MR - MX1 * MY1 (RND), AY0 = DM(I0,M1)
2BC0: 0000                  MR = MX1 * MY0 (SS), AX0 = MR1
2BC1: 0000                  MR = MR + MX0 * MY1 (RND), AY1 = DM(I0,M0)
2BC2: 0000                  AR = AY0 - AX0, MX0 = DM(I1,M1)
2BC3: 0000                  AR = AX0 + AY0, DM(I0,M1) = AR
2BC4: 0000                  AR = AY1 - MR1, DM(I2,M1) = AR
2BC5: 0000                  AR = MR1 + AY1, DM(I0,M1) = AR
2BC6: 0000                  DM(I2,M1) = AR
2BC7: 0000              MODIFY (I2,M2)
2BC8: 0000              MODIFY (I1,M3)
2BC9: 0000              MODIFY (I0,M2)
2BCA: 0000          SI = DM($0A09)
2BCB: 0000          SR = LSHIFT SI BY 1 (LO)
2BCC: 0000          DM($0A09) = SR0
2BCD: 0000          SI = DM($0A0D)
2BCE: 0000          DM($0A0B) = SI
2BCF: 0000          SR = LSHIFT SI BY -1 (LO)
2BD0: 0000          DM($0A0D) = SR0
*/


/***************************************************************************
	VERY BASIC & SAFE OPTIMIZATIONS
****************************************************************************/

static READ16_HANDLER( dcs_polling_r )
{
	activecpu_eat_cycles(100);
	return *dcs_polling_base;
}
