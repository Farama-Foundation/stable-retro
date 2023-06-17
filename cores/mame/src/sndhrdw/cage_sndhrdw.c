/***************************************************************************

	Atari CAGE Audio Board

****************************************************************************/


#define LOG_COMM			(0)
#define LOG_WAVE			(0)


#include "driver.h"
#include "cpu/tms32031/tms32031.h"
#include "cage.h"

#if (LOG_WAVE)
#include "sound/wavwrite.h"
#endif



/*************************************
 *
 *	Constants/macros
 *
 *************************************/

#define DAC_BUFFER_CHANNELS		4
#define DAC_BUFFER_FRAMES		4096
#define DAC_BUFFER_SAMPLES		(DAC_BUFFER_FRAMES * DAC_BUFFER_CHANNELS)
#define DAC_BUFFER_FRAMES_MASK	(DAC_BUFFER_FRAMES - 1)
#define DAC_BUFFER_SAMPLES_MASK	(DAC_BUFFER_SAMPLES - 1)

#define ADDR_RANGE(s,e) ((s)*4), ((e)*4+3)



/*************************************
 *
 *	Statics
 *
 *************************************/

static int sound_stream;
static int cage_cpu;
static double cage_cpu_clock_period;
static double cage_cpu_h1_clock;

static UINT8 cpu_to_cage_ready;
static UINT8 cage_to_cpu_ready;

static void (*cage_irqhandler)(int);

static INT16 *sound_buffer;
static UINT32 buffer_in, buffer_out, buffer_out_step;

static double serial_time_per_word;

static UINT8 dma_enabled;
static void *dma_timer;

static UINT8 timer_enabled[2];
static void *timer[2];

static data32_t *tms32031_io_regs;

static UINT16 cage_from_main;
static UINT16 cage_control;

static data32_t *speedup_ram;

#if (LOG_WAVE)
static void *wavfile;
#endif



/*************************************
 *
 *	I/O port definitions
 *
 *************************************/

#define DMA_GLOBAL_CTL			0x00
#define DMA_SOURCE_ADDR			0x04
#define DMA_DEST_ADDR			0x06
#define DMA_TRANSFER_COUNT		0x08

#define TIMER0_GLOBAL_CTL		0x20
#define TIMER0_COUNTER			0x24
#define TIMER0_PERIOD			0x28

#define TIMER1_GLOBAL_CTL		0x30
#define TIMER1_COUNTER			0x34
#define TIMER1_PERIOD			0x38

#define SPORT_GLOBAL_CTL		0x40
#define SPORT_TX_CTL			0x42
#define SPORT_RX_CTL			0x43
#define SPORT_TIMER_CTL			0x44
#define SPORT_TIMER_COUNTER		0x45
#define SPORT_TIMER_PERIOD		0x46
#define SPORT_DATA_TX			0x48
#define SPORT_DATA_RX			0x4c

static const char *register_names[] =
{
	"TMS32031-DMA global control", NULL, NULL, NULL,
	"TMS32031-DMA source address", NULL, "TMS32031-DMA destination address", NULL,
	"TMS32031-DMA transfer counter", NULL, NULL, NULL,
	NULL, NULL, NULL, NULL,

	NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL,

	"TMS32031-Timer 0 global control", NULL, NULL, NULL,
	"TMS32031-Timer 0 counter", NULL, NULL, NULL,
	"TMS32031-Timer 0 period", NULL, NULL, NULL,
	NULL, NULL, NULL, NULL,

	"TMS32031-Timer 1 global control", NULL, NULL, NULL,
	"TMS32031-Timer 1 counter", NULL, NULL, NULL,
	"TMS32031-Timer 1 period", NULL, NULL, NULL,
	NULL, NULL, NULL, NULL,

	"TMS32031-Serial port global control", NULL, "TMS32031-Serial port TX control", "TMS32031-Serial port RX control",
	"TMS32031-Serial port timer control", "TMS32031-Serial port timer counter", "TMS32031-Serial port timer period", NULL,
	"TMS32031-Serial port data TX", NULL, NULL, NULL,
	"TMS32031-Serial port data RX", NULL, NULL, NULL,

	NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL,

	NULL, NULL, NULL, NULL,
	"TMS32031-Primary bus control", NULL, NULL, NULL,
	NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL,

	NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL
};



/*************************************
 *
 *	Prototypes
 *
 *************************************/

static void dma_timer_callback(int param);
static void timer_callback(int param);
static void update_timer(int which);
static WRITE32_HANDLER( speedup_w );



/*************************************
 *
 *	Initialization
 *
 *************************************/

void cage_init(int boot_region, offs_t speedup)
{
	cage_irqhandler = NULL;

	cpu_setbank(10, memory_region(boot_region));
	cpu_setbank(11, memory_region(boot_region + 1));

	cage_cpu = mame_find_cpu_index("cage");
	cage_cpu_clock_period = 1.0 / (double)Machine->drv->cpu[cage_cpu].cpu_clock;
	cage_cpu_h1_clock = cage_cpu_clock_period * 2.0;

	dma_timer = timer_alloc(dma_timer_callback);
	timer[0] = timer_alloc(timer_callback);
	timer[1] = timer_alloc(timer_callback);
	
	buffer_in = buffer_out = 0;
	
	if (speedup)
		speedup_ram = install_mem_write32_handler(cage_cpu, ADDR_RANGE(speedup, speedup), speedup_w);
}


void cage_set_irq_handler(void (*irqhandler)(int))
{
	cage_irqhandler = irqhandler;
}


void cage_reset_w(int state)
{
	if (state)
		cage_control_w(0);
	cpunum_set_reset_line(cage_cpu, state ? ASSERT_LINE : CLEAR_LINE);
}



/*************************************
 *
 *	DAC update routine
 *
 *************************************/

static void dac_update(int num, INT16 **buffer, int length)
{
	INT16 *dest[DAC_BUFFER_CHANNELS];
	UINT32 current = buffer_out;
	UINT32 step = buffer_out_step;
	int i, j;
	
	for (j = 0; j < DAC_BUFFER_CHANNELS; j++)
		dest[j] = buffer[j];

	/* fill in with samples until we hit the end or run out */
	for (i = 0; i < length; i++)
	{
		UINT32 indx = (current >> 16) * DAC_BUFFER_CHANNELS;
		if (indx + DAC_BUFFER_CHANNELS - 1 >= buffer_in)
			break;
		current += step;
		for (j = 0; j < DAC_BUFFER_CHANNELS; j++)
			*dest[j]++ = sound_buffer[(indx + j) & DAC_BUFFER_SAMPLES_MASK];
	}

	/* fill the rest with the last sample */
	for ( ; i < length; i++)
	{
		UINT32 indx = ((buffer_in - 1) / DAC_BUFFER_CHANNELS) * DAC_BUFFER_CHANNELS;
		for (j = 0; j < DAC_BUFFER_CHANNELS; j++)
			*dest[j]++ = sound_buffer[(indx + j) & DAC_BUFFER_SAMPLES_MASK];
	}

	/* mask off extra bits */
	while (current >= (DAC_BUFFER_FRAMES << 16))
	{
		current -= DAC_BUFFER_FRAMES << 16;
		buffer_in -= DAC_BUFFER_SAMPLES;
	}

	/* update the final values */
	buffer_out = current;

#if (LOG_WAVE)
#if (DAC_BUFFER_CHANNELS == 4)
	wav_add_data_16lr(wavfile, buffer[0], buffer[1], length);
#else
	wav_add_data_16lr(wavfile, (buffer[0] + buffer[2]) / 2, (buffer[1] + buffer[3]) / 2, length);
#endif
#endif
}



/*************************************
 *
 *	Custom sound start/stop
 *
 *************************************/

static int custom_start(const struct MachineSound *msound)
{
#if (DAC_BUFFER_CHANNELS == 4)
	const char *names[] = { "Forward R", "Back L", "Forward L", "Back R" };
	int mixing_levels[] = { MIXER(50, MIXER_PAN_RIGHT), MIXER(50, MIXER_PAN_LEFT), MIXER(50, MIXER_PAN_LEFT), MIXER(50, MIXER_PAN_RIGHT) };
#else
	const char *names[] = { "DAC L", "DAC R" };
	int mixing_levels[] = { MIXER(100, MIXER_PAN_LEFT), MIXER(100, MIXER_PAN_RIGHT)) };
#endif

	/* allocate a DAC stream */
	sound_stream = stream_init_multi(DAC_BUFFER_CHANNELS, names, mixing_levels, Machine->sample_rate, 0, dac_update);

	/* allocate memory for our buffers */
	sound_buffer = auto_malloc(DAC_BUFFER_SAMPLES * sizeof(INT16));
	if (!sound_buffer)
		return 1;

#if (LOG_WAVE)
	wavfile = wav_open("cage.wav", Machine->sample_rate, 2);
#endif

	return 0;
}


static void custom_stop(void)
{
#if (LOG_WAVE)
	wav_close(wavfile);
#endif
}



/*************************************
 *
 *	DMA timers
 *
 *************************************/

static void dma_timer_callback(int param)
{
	/* set the final count to 0 and the source address to the final address */
	tms32031_io_regs[DMA_TRANSFER_COUNT] = 0;
	tms32031_io_regs[DMA_SOURCE_ADDR] = param;
	
	/* set the interrupt */
	cpu_set_irq_line(cage_cpu, TMS32031_DINT, ASSERT_LINE);
	dma_enabled = 0;
}


static void update_dma_state(void)
{
	/* determine the new enabled state */
	int enabled = ((tms32031_io_regs[DMA_GLOBAL_CTL] & 3) == 3) && (tms32031_io_regs[DMA_TRANSFER_COUNT] != 0);
	
	/* see if we turned on */
	if (enabled && !dma_enabled)
	{
		UINT32 addr, inc;
		int i;
		
		/* make sure our assumptions are correct */
		if (tms32031_io_regs[DMA_DEST_ADDR] != 0x808048)
			log_cb(RETRO_LOG_DEBUG, LOGPRE "CAGE DMA: unexpected dest address %08X!\n", tms32031_io_regs[DMA_DEST_ADDR]);
		if ((tms32031_io_regs[DMA_GLOBAL_CTL] & 0xfef) != 0xe03)
			log_cb(RETRO_LOG_DEBUG, LOGPRE "CAGE DMA: unexpected transfer params %08X!\n", tms32031_io_regs[DMA_GLOBAL_CTL]);
		
		/* do the DMA up front */
		addr = tms32031_io_regs[DMA_SOURCE_ADDR];
		inc = (tms32031_io_regs[DMA_GLOBAL_CTL] >> 4) & 1;
		for (i = 0; i < tms32031_io_regs[DMA_TRANSFER_COUNT]; i++)
		{
			sound_buffer[(buffer_in + i) & DAC_BUFFER_SAMPLES_MASK] = cpu_readmem26ledw_dword(addr * 4);
			addr += inc;
		}
		buffer_in += tms32031_io_regs[DMA_TRANSFER_COUNT];

		/* compute the time of the interrupt and set the timer */
		timer_adjust(dma_timer, serial_time_per_word * tms32031_io_regs[DMA_TRANSFER_COUNT], addr, TIME_NEVER);
	}
	
	/* see if we turned off */
	else if (!enabled && dma_enabled)
	{
		timer_adjust(dma_timer, TIME_NEVER, 0, TIME_NEVER);
	}
	
	/* set the new state */
	dma_enabled = enabled;
}



/*************************************
 *
 *	Internal timers
 *
 *************************************/

static void timer_callback(int which)
{
	/* set the interrupt */
	cpu_set_irq_line(cage_cpu, TMS32031_TINT0 + which, ASSERT_LINE);
	timer_enabled[which] = 0;
	update_timer(which);
}


static void update_timer(int which)
{
	/* determine the new enabled state */
	int base = 0x10 * which;
	int enabled = ((tms32031_io_regs[base + TIMER0_GLOBAL_CTL] & 0xc0) == 0xc0);
	
	/* see if we turned on */
	if (enabled && !timer_enabled[which])
	{
		double period = cage_cpu_h1_clock * 2. * (double)tms32031_io_regs[base + TIMER0_PERIOD];
		
		/* make sure our assumptions are correct */
		if (tms32031_io_regs[base + TIMER0_GLOBAL_CTL] != 0x2c1)
			log_cb(RETRO_LOG_DEBUG, LOGPRE "CAGE TIMER%d: unexpected timer config %08X!\n", which, tms32031_io_regs[base + TIMER0_GLOBAL_CTL]);

		timer_adjust(timer[which], period, which, TIME_NEVER);
	}
	
	/* see if we turned off */
	else if (!enabled && timer_enabled[which])
	{
		timer_adjust(timer[which], TIME_NEVER, which, TIME_NEVER);
	}
	
	/* set the new state */
	timer_enabled[which] = enabled;
}



/*************************************
 *
 *	Serial port I/O
 *
 *************************************/

static void update_serial(void)
{
	double serial_clock, bit_clock;

	/* we start out at half the H1 frequency (or 2x the H1 period) */
	serial_clock = cage_cpu_h1_clock * 2.0;

	/* if we're in clock mode, muliply by another factor of 2 */
	if (tms32031_io_regs[SPORT_GLOBAL_CTL] & 4)
		serial_clock *= 2.0;
	
	/* now multiply by the timer period */
	bit_clock = serial_clock * (double)(tms32031_io_regs[SPORT_TIMER_PERIOD] & 0xffff);
	
	/* and times the number of bits per sample */
	serial_time_per_word = bit_clock * 8.0 * (double)(((tms32031_io_regs[SPORT_GLOBAL_CTL] >> 18) & 3) + 1);

	/* compute the step value to stretch this to the Machine->sample_rate */
	buffer_out_step = (UINT32)(65536.0 / (serial_time_per_word * DAC_BUFFER_CHANNELS * (double)Machine->sample_rate));
}



/*************************************
 *
 *	Master read/write
 *
 *************************************/

static READ32_HANDLER( tms32031_io_r )
{
	UINT16 result = tms32031_io_regs[offset];
	
	switch (offset)
	{
		case DMA_GLOBAL_CTL:
			result = (result & ~0xc) | (dma_enabled ? 0xc : 0x0);
			break;
	}

	log_cb(RETRO_LOG_DEBUG, LOGPRE "CAGE:%06X:%s read -> %08X\n", activecpu_get_pc(), register_names[offset & 0x7f], result);
	return result;
}


static WRITE32_HANDLER( tms32031_io_w )
{
	COMBINE_DATA(&tms32031_io_regs[offset]);

	log_cb(RETRO_LOG_DEBUG, LOGPRE "CAGE:%06X:%s write = %08X\n", activecpu_get_pc(), register_names[offset & 0x7f], tms32031_io_regs[offset]);
	
	switch (offset)
	{
		case DMA_GLOBAL_CTL:
		case DMA_SOURCE_ADDR:
		case DMA_DEST_ADDR:
		case DMA_TRANSFER_COUNT:
			update_dma_state();
			break;
		
		case TIMER0_GLOBAL_CTL:
		case TIMER0_COUNTER:
		case TIMER0_PERIOD:
			update_timer(0);
			break;
		
		case TIMER1_GLOBAL_CTL:
		case TIMER1_COUNTER:
		case TIMER1_PERIOD:
			update_timer(1);
			break;
		
		case SPORT_TX_CTL:
		case SPORT_RX_CTL:
		case SPORT_TIMER_COUNTER:
		case SPORT_DATA_RX:
			break;

		case SPORT_DATA_TX:
#if (DAC_BUFFER_CHANNELS == 4)
			if ((int)(1.0 / serial_time_per_word) == 22050*4 && (tms32031_io_regs[SPORT_RX_CTL] & 0xff) == 0x62)
				tms32031_io_regs[SPORT_RX_CTL] ^= 0x800;
#endif
			break;

		case SPORT_GLOBAL_CTL:
		case SPORT_TIMER_CTL:
		case SPORT_TIMER_PERIOD:
			update_serial();
			break;
	}
}



/*************************************
 *
 *	External interfaces
 *
 *************************************/

static void update_control_lines(void)
{
	int val;
	
	/* set the IRQ to the main CPU */
	if (cage_irqhandler)
	{
		int reason = 0;
		
		if ((cage_control & 3) == 3 && !cpu_to_cage_ready)
			reason |= CAGE_IRQ_REASON_BUFFER_EMPTY;
		if ((cage_control & 2) && cage_to_cpu_ready)
			reason |= CAGE_IRQ_REASON_DATA_READY;

		(*cage_irqhandler)(reason);
	}

	/* set the IOF input lines */
	cpuintrf_push_context(cage_cpu);
	val = activecpu_get_reg(TMS32031_IOF);
	val &= ~0x88;
	if (cpu_to_cage_ready) val |= 0x08;
	if (cage_to_cpu_ready) val |= 0x80;
	activecpu_set_reg(TMS32031_IOF, val);
	cpuintrf_pop_context();
}


static READ32_HANDLER( cage_from_main_r )
{
	log_cb(RETRO_LOG_DEBUG, LOGPRE "%06X:CAGE read command = %04X\n", activecpu_get_pc(), cage_from_main);
	cpu_to_cage_ready = 0;
	update_control_lines();
	return cage_from_main;
}


static WRITE32_HANDLER( cage_from_main_ack_w )
{
	log_cb(RETRO_LOG_DEBUG, LOGPRE "%06X:CAGE ack command = %04X\n", activecpu_get_pc(), cage_from_main);
}


static WRITE32_HANDLER( cage_to_main_w )
{
	log_cb(RETRO_LOG_DEBUG, LOGPRE "%06X:Data from CAGE = %04X\n", activecpu_get_pc(), data);
	soundlatch_word_w(0, data, mem_mask);
	cage_to_cpu_ready = 1;
	update_control_lines();
}


static READ32_HANDLER( cage_io_status_r )
{
	int result = 0;
	if (cpu_to_cage_ready)
		result |= 0x80;
	if (!cage_to_cpu_ready)
		result |= 0x40;
	return result;
}


UINT16 main_from_cage_r(void)
{
	log_cb(RETRO_LOG_DEBUG, LOGPRE "%06X:main read data = %04X\n", activecpu_get_pc(), soundlatch_word_r(0, 0));
	cage_to_cpu_ready = 0;
	update_control_lines();
	return soundlatch_word_r(0, 0);
}


static void deferred_cage_w(int param)
{
	cage_from_main = param;
	cpu_to_cage_ready = 1;
	update_control_lines();
	cpu_set_irq_line(cage_cpu, TMS32031_IRQ0, ASSERT_LINE);
}


void main_to_cage_w(UINT16 data)
{
	log_cb(RETRO_LOG_DEBUG, LOGPRE "%06X:Command to CAGE = %04X\n", activecpu_get_pc(), data);
	timer_set(TIME_NOW, data, deferred_cage_w);
}


UINT16 cage_control_r(void)
{
	UINT16 result = 0;
	
	if (cpu_to_cage_ready)
		result |= 2;
	if (cage_to_cpu_ready)
		result |= 1;
	
	return result;
}


void cage_control_w(UINT16 data)
{
	cage_control = data;

	/* CPU is reset if both control lines are 0 */
	if (!(cage_control & 3))
	{
		cpu_set_reset_line(cage_cpu, ASSERT_LINE);
		
		dma_enabled = 0;
		timer_adjust(dma_timer, TIME_NEVER, 0, TIME_NEVER);
		
		timer_enabled[0] = 0;
		timer_enabled[1] = 0;
		timer_adjust(timer[0], TIME_NEVER, 0, TIME_NEVER);
		timer_adjust(timer[1], TIME_NEVER, 0, TIME_NEVER);
		
		memset(tms32031_io_regs, 0, 0x60 * 4);

		cpu_to_cage_ready = 0;
		cage_to_cpu_ready = 0;
	}
	else
		cpu_set_reset_line(cage_cpu, CLEAR_LINE);
	
	/* update the control state */
	update_control_lines();
}



/*************************************
 *
 *	Speedups
 *
 *************************************/

static WRITE32_HANDLER( speedup_w )
{
	activecpu_eat_cycles(100);
	COMBINE_DATA(&speedup_ram[offset]);
}



/*************************************
 *
 *	CPU memory map & config
 *
 *************************************/

static struct tms32031_config cage_config =
{
	0x400000
};


static MEMORY_READ32_START( readmem_cage )
	{ ADDR_RANGE(0x000000, 0x00ffff), MRA32_RAM },
	{ ADDR_RANGE(0x400000, 0x47ffff), MRA32_BANK10 },
	{ ADDR_RANGE(0x808000, 0x8080ff), tms32031_io_r },
	{ ADDR_RANGE(0x809800, 0x809fff), MRA32_RAM },
	{ ADDR_RANGE(0xa00000, 0xa00000), cage_from_main_r },
	{ ADDR_RANGE(0xc00000, 0xffffff), MRA32_BANK11 },
MEMORY_END


static MEMORY_WRITE32_START( writemem_cage )
	{ ADDR_RANGE(0x000000, 0x00ffff), MWA32_RAM },
	{ ADDR_RANGE(0x200000, 0x200000), MWA32_NOP },
	{ ADDR_RANGE(0x400000, 0x47ffff), MWA32_ROM },
	{ ADDR_RANGE(0x808000, 0x8080ff), tms32031_io_w, &tms32031_io_regs },
	{ ADDR_RANGE(0x809800, 0x809fff), MWA32_RAM },
	{ ADDR_RANGE(0xa00000, 0xa00000), cage_to_main_w },
	{ ADDR_RANGE(0xc00000, 0xffffff), MWA32_ROM },
MEMORY_END


static MEMORY_READ32_START( readmem_cage_seattle )
	{ ADDR_RANGE(0x000000, 0x00ffff), MRA32_RAM },
	{ ADDR_RANGE(0x400000, 0x47ffff), MRA32_BANK10 },
	{ ADDR_RANGE(0x808000, 0x8080ff), tms32031_io_r },
	{ ADDR_RANGE(0x809800, 0x809fff), MRA32_RAM },
	{ ADDR_RANGE(0xa00000, 0xa00000), cage_from_main_r },
	{ ADDR_RANGE(0xa00003, 0xa00003), cage_io_status_r },
	{ ADDR_RANGE(0xc00000, 0xffffff), MRA32_BANK11 },
MEMORY_END


static MEMORY_WRITE32_START( writemem_cage_seattle )
	{ ADDR_RANGE(0x000000, 0x00ffff), MWA32_RAM },
	{ ADDR_RANGE(0x200000, 0x200000), MWA32_NOP },
	{ ADDR_RANGE(0x400000, 0x47ffff), MWA32_ROM },
	{ ADDR_RANGE(0x808000, 0x8080ff), tms32031_io_w, &tms32031_io_regs },
	{ ADDR_RANGE(0x809800, 0x809fff), MWA32_RAM },
	{ ADDR_RANGE(0xa00000, 0xa00000), cage_from_main_ack_w },
	{ ADDR_RANGE(0xa00001, 0xa00001), cage_to_main_w },
	{ ADDR_RANGE(0xc00000, 0xffffff), MWA32_ROM },
MEMORY_END



/*************************************
 *
 *	CAGE machine driver
 *
 *************************************/

/* Custom structure */
struct CustomSound_interface cage_custom_interface =
{
	custom_start,custom_stop,0
};


MACHINE_DRIVER_START( cage )

	/* basic machine hardware */
	MDRV_CPU_ADD_TAG("cage", TMS32031, 33868800)
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
	MDRV_CPU_CONFIG(cage_config)
	MDRV_CPU_MEMORY(readmem_cage,writemem_cage)

	/* sound hardware */
	MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
	MDRV_SOUND_ADD(CUSTOM, cage_custom_interface)
MACHINE_DRIVER_END


MACHINE_DRIVER_START( cage_seattle )
	MDRV_IMPORT_FROM(cage)
	
	MDRV_CPU_MODIFY("cage")
	MDRV_CPU_MEMORY(readmem_cage_seattle,writemem_cage_seattle)
MACHINE_DRIVER_END
