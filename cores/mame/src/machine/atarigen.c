/*##########################################################################

	atarigen.c

	General functions for Atari raster games.

##########################################################################*/


#include "driver.h"
#include "atarigen.h"
#include "slapstic.h"
#include "cpu/m6502/m6502.h"



/*##########################################################################
	CONSTANTS
##########################################################################*/

#define SOUND_INTERLEAVE_RATE		TIME_IN_USEC(50)
#define SOUND_INTERLEAVE_REPEAT		20



/*##########################################################################
	GLOBAL VARIABLES
##########################################################################*/

UINT8 				atarigen_scanline_int_state;
UINT8 				atarigen_sound_int_state;
UINT8 				atarigen_video_int_state;

const data16_t *	atarigen_eeprom_default;
data16_t *			atarigen_eeprom;
size_t 				atarigen_eeprom_size;

UINT8 				atarigen_cpu_to_sound_ready;
UINT8 				atarigen_sound_to_cpu_ready;

data16_t *			atarigen_playfield;
data16_t *			atarigen_playfield2;
data16_t *			atarigen_playfield_upper;
data16_t *			atarigen_alpha;
data16_t *			atarigen_alpha2;
data16_t *			atarigen_xscroll;
data16_t *			atarigen_yscroll;

data32_t *			atarigen_playfield32;
data32_t *			atarigen_alpha32;

struct tilemap *	atarigen_playfield_tilemap;
struct tilemap *	atarigen_playfield2_tilemap;
struct tilemap *	atarigen_alpha_tilemap;
struct tilemap *	atarigen_alpha2_tilemap;

data16_t *			atarivc_data;
data16_t *			atarivc_eof_data;
struct atarivc_state_desc atarivc_state;



/*##########################################################################
	STATIC VARIABLES
##########################################################################*/

static atarigen_int_callback update_int_callback;
static void *		scanline_interrupt_timer;

static UINT8 		eeprom_unlocked;

static UINT8 		atarigen_slapstic_num;
static data16_t *	atarigen_slapstic;
static int			atarigen_slapstic_bank;
static void *		atarigen_slapstic_bank0;

static UINT8 		sound_cpu_num;
static UINT8 		atarigen_cpu_to_sound;
static UINT8 		atarigen_sound_to_cpu;
static UINT8 		timed_int;
static UINT8 		ym2151_int;

static UINT8 *		speed_a, *speed_b;
static UINT32 		speed_pc;

static atarigen_scanline_callback scanline_callback;
static int 			scanlines_per_callback;
static double 		scanline_callback_period;
static int 			last_scanline;

static int 			actual_vc_latch0;
static int 			actual_vc_latch1;
static UINT8		atarivc_playfields;

static int			playfield_latch;
static int			playfield2_latch;



/*##########################################################################
	STATIC FUNCTION DECLARATIONS
##########################################################################*/

static void scanline_interrupt_callback(int param);

static void decompress_eeprom_word(const data16_t *data);
static void decompress_eeprom_byte(const data16_t *data);

static void update_6502_irq(void);
static void sound_comm_timer(int reps_left);
static void delayed_sound_reset(int param);
static void delayed_sound_w(int param);
static void delayed_6502_sound_w(int param);

static READ_HANDLER( m6502_speedup_r );
static void atarigen_set_vol(int volume, const char *string);

static void vblank_timer(int param);
static void scanline_timer(int scanline);

static void atarivc_common_w(offs_t offset, data16_t newword);

static void unhalt_cpu(int param);



/*##########################################################################
	INTERRUPT HANDLING
##########################################################################*/

/*---------------------------------------------------------------
	atarigen_interrupt_reset: Initializes the state of all
	the interrupt sources.
---------------------------------------------------------------*/

void atarigen_interrupt_reset(atarigen_int_callback update_int)
{
	/* set the callback */
	update_int_callback = update_int;

	/* reset the interrupt states */
	atarigen_video_int_state = atarigen_sound_int_state = atarigen_scanline_int_state = 0;
	scanline_interrupt_timer = NULL;
	
	/* create a timer for scanlines */
	scanline_interrupt_timer = timer_alloc(scanline_interrupt_callback);
}


/*---------------------------------------------------------------
	atarigen_update_interrupts: Forces the interrupt callback
	to be called with the current VBLANK and sound interrupt
	states.
---------------------------------------------------------------*/

void atarigen_update_interrupts(void)
{
	(*update_int_callback)();
}



/*---------------------------------------------------------------
	atarigen_scanline_int_set: Sets the scanline when the next
	scanline interrupt should be generated.
---------------------------------------------------------------*/

void atarigen_scanline_int_set(int scanline)
{
	timer_adjust(scanline_interrupt_timer, cpu_getscanlinetime(scanline), 0, 0);
}


/*---------------------------------------------------------------
	atarigen_scanline_int_gen: Standard interrupt routine
	which sets the scanline interrupt state.
---------------------------------------------------------------*/

INTERRUPT_GEN( atarigen_scanline_int_gen )
{
	atarigen_scanline_int_state = 1;
	(*update_int_callback)();
}


/*---------------------------------------------------------------
	atarigen_scanline_int_ack_w: Resets the state of the
	scanline interrupt.
---------------------------------------------------------------*/

WRITE16_HANDLER( atarigen_scanline_int_ack_w )
{
	atarigen_scanline_int_state = 0;
	(*update_int_callback)();
}

WRITE32_HANDLER( atarigen_scanline_int_ack32_w )
{
	atarigen_scanline_int_state = 0;
	(*update_int_callback)();
}


/*---------------------------------------------------------------
	atarigen_sound_int_gen: Standard interrupt routine which
	sets the sound interrupt state.
---------------------------------------------------------------*/

INTERRUPT_GEN( atarigen_sound_int_gen )
{
	atarigen_sound_int_state = 1;
	(*update_int_callback)();
}


/*---------------------------------------------------------------
	atarigen_sound_int_ack_w: Resets the state of the sound
	interrupt.
---------------------------------------------------------------*/

WRITE16_HANDLER( atarigen_sound_int_ack_w )
{
	atarigen_sound_int_state = 0;
	(*update_int_callback)();
}

WRITE32_HANDLER( atarigen_sound_int_ack32_w )
{
	atarigen_sound_int_state = 0;
	(*update_int_callback)();
}


/*---------------------------------------------------------------
	atarigen_video_int_gen: Standard interrupt routine which
	sets the video interrupt state.
---------------------------------------------------------------*/

INTERRUPT_GEN( atarigen_video_int_gen )
{
	atarigen_video_int_state = 1;
	(*update_int_callback)();
}


/*---------------------------------------------------------------
	atarigen_video_int_ack_w: Resets the state of the video
	interrupt.
---------------------------------------------------------------*/

WRITE16_HANDLER( atarigen_video_int_ack_w )
{
	atarigen_video_int_state = 0;
	(*update_int_callback)();
}

WRITE32_HANDLER( atarigen_video_int_ack32_w )
{
	atarigen_video_int_state = 0;
	(*update_int_callback)();
}


/*---------------------------------------------------------------
	scanline_interrupt_callback: Signals an interrupt.
---------------------------------------------------------------*/

static void scanline_interrupt_callback(int param)
{
	/* generate the interrupt */
	atarigen_scanline_int_gen();

	/* set a new timer to go off at the same scan line next frame */
	timer_adjust(scanline_interrupt_timer, TIME_IN_HZ(Machine->drv->frames_per_second), 0, 0);
}



/*##########################################################################
	EEPROM HANDLING
##########################################################################*/

/*---------------------------------------------------------------
	atarigen_eeprom_reset: Makes sure that the unlocked state
	is cleared when we reset.
---------------------------------------------------------------*/

void atarigen_eeprom_reset(void)
{
	eeprom_unlocked = 0;
}


/*---------------------------------------------------------------
	atarigen_eeprom_enable_w: Any write to this handler will
	allow one byte to be written to the EEPROM data area the
	next time.
---------------------------------------------------------------*/

WRITE16_HANDLER( atarigen_eeprom_enable_w )
{
	eeprom_unlocked = 1;
}

WRITE32_HANDLER( atarigen_eeprom_enable32_w )
{
	eeprom_unlocked = 1;
}


/*---------------------------------------------------------------
	atarigen_eeprom_w: Writes a "word" to the EEPROM, which is
	almost always accessed via the low byte of the word only.
	If the EEPROM hasn't been unlocked, the write attempt is
	ignored.
---------------------------------------------------------------*/

WRITE16_HANDLER( atarigen_eeprom_w )
{
	if (!eeprom_unlocked)
		return;

	COMBINE_DATA(&atarigen_eeprom[offset]);
	eeprom_unlocked = 0;
}

WRITE32_HANDLER( atarigen_eeprom32_w )
{
	if (!eeprom_unlocked)
		return;

	COMBINE_DATA(&atarigen_eeprom[offset * 2 + 1]);
	data >>= 16;
	mem_mask >>= 16;
	COMBINE_DATA(&atarigen_eeprom[offset * 2]);
	eeprom_unlocked = 0;
}


/*---------------------------------------------------------------
	atarigen_eeprom_r: Reads a "word" from the EEPROM, which is
	almost always accessed via the low byte of the word only.
---------------------------------------------------------------*/

READ16_HANDLER( atarigen_eeprom_r )
{
	return atarigen_eeprom[offset] | 0xff00;
}

READ16_HANDLER( atarigen_eeprom_upper_r )
{
	return atarigen_eeprom[offset] | 0x00ff;
}

READ32_HANDLER( atarigen_eeprom_upper32_r )
{
	return (atarigen_eeprom[offset * 2] << 16) | atarigen_eeprom[offset * 2 + 1] | 0x00ff00ff;
}


/*---------------------------------------------------------------
	nvram_handler_atarigen: Loads the EEPROM data.
---------------------------------------------------------------*/

NVRAM_HANDLER( atarigen )
{
	if (read_or_write)
		mame_fwrite(file, atarigen_eeprom, atarigen_eeprom_size);
	else if (file)
		mame_fread(file, atarigen_eeprom, atarigen_eeprom_size);
	else
	{
		/* all 0xff's work for most games */
		memset(atarigen_eeprom, 0xff, atarigen_eeprom_size);

		/* anything else must be decompressed */
		if (atarigen_eeprom_default)
		{
			if (atarigen_eeprom_default[0] == 0)
				decompress_eeprom_byte(atarigen_eeprom_default + 1);
			else
				decompress_eeprom_word(atarigen_eeprom_default + 1);
		}
	}
}


/*---------------------------------------------------------------
	decompress_eeprom_word: Used for decompressing EEPROM data
	that has every other byte invalid.
---------------------------------------------------------------*/

void decompress_eeprom_word(const data16_t *data)
{
	data16_t *dest = (data16_t *)atarigen_eeprom;
	data16_t value;

	while ((value = *data++) != 0)
	{
		int count = (value >> 8);
		value = (value << 8) | (value & 0xff);

		while (count--)
			*dest++ = value;
	}
}


/*---------------------------------------------------------------
	decompress_eeprom_byte: Used for decompressing EEPROM data
	that is byte-packed.
---------------------------------------------------------------*/

void decompress_eeprom_byte(const data16_t *data)
{
	UINT8 *dest = (UINT8 *)atarigen_eeprom;
	data16_t value;

	while ((value = *data++) != 0)
	{
		int count = (value >> 8);
		value = (value << 8) | (value & 0xff);

		while (count--)
			*dest++ = value;
	}
}



/*##########################################################################
	SLAPSTIC HANDLING
##########################################################################*/

static INLINE void update_bank(int bank)
{
	/* if the bank has changed, copy the memory; Pit Fighter needs this */
	if (bank != atarigen_slapstic_bank)
	{
		/* bank 0 comes from the copy we made earlier */
		if (bank == 0)
			memcpy(atarigen_slapstic, atarigen_slapstic_bank0, 0x2000);
		else
			memcpy(atarigen_slapstic, &atarigen_slapstic[bank * 0x1000], 0x2000);

		/* remember the current bank */
		atarigen_slapstic_bank = bank;
	}
}


/*---------------------------------------------------------------
	atarigen_slapstic_init: Installs memory handlers for the
	slapstic and sets the chip number.
---------------------------------------------------------------*/

void atarigen_slapstic_init(int cpunum, int base, int chipnum)
{
	atarigen_slapstic_num = chipnum;
	atarigen_slapstic = NULL;

	/* if we have a chip, install it */
	if (chipnum)
	{
		/* initialize the slapstic */
		slapstic_init(chipnum);

		/* install the memory handlers */
		atarigen_slapstic = install_mem_read16_handler(cpunum, base, base + 0x7fff, atarigen_slapstic_r);
		atarigen_slapstic = install_mem_write16_handler(cpunum, base, base + 0x7fff, atarigen_slapstic_w);

		/* allocate memory for a copy of bank 0 */
		atarigen_slapstic_bank0 = auto_malloc(0x2000);
		if (atarigen_slapstic_bank0)
			memcpy(atarigen_slapstic_bank0, atarigen_slapstic, 0x2000);
	}
}


/*---------------------------------------------------------------
	atarigen_slapstic_reset: Makes the selected slapstic number
	active and resets its state.
---------------------------------------------------------------*/

void atarigen_slapstic_reset(void)
{
	if (atarigen_slapstic_num)
	{
		slapstic_reset();
		update_bank(slapstic_bank());
	}
}


/*---------------------------------------------------------------
	atarigen_slapstic_w: Assuming that the slapstic sits in
	ROM memory space, we just simply tweak the slapstic at this
	address and do nothing more.
---------------------------------------------------------------*/

WRITE16_HANDLER( atarigen_slapstic_w )
{
	update_bank(slapstic_tweak(offset));
}


/*---------------------------------------------------------------
	atarigen_slapstic_r: Tweaks the slapstic at the appropriate
	address and then reads a word from the underlying memory.
---------------------------------------------------------------*/

READ16_HANDLER( atarigen_slapstic_r )
{
	/* fetch the result from the current bank first */
	int result = atarigen_slapstic[offset & 0xfff];

	/* then determine the new one */
	update_bank(slapstic_tweak(offset));
	return result;
}



/*##########################################################################
	SOUND I/O
##########################################################################*/

/*---------------------------------------------------------------
	atarigen_sound_io_reset: Resets the state of the sound I/O.
---------------------------------------------------------------*/

void atarigen_sound_io_reset(int cpu_num)
{
	/* remember which CPU is the sound CPU */
	sound_cpu_num = cpu_num;

	/* reset the internal interrupts states */
	timed_int = ym2151_int = 0;

	/* reset the sound I/O states */
	atarigen_cpu_to_sound = atarigen_sound_to_cpu = 0;
	atarigen_cpu_to_sound_ready = atarigen_sound_to_cpu_ready = 0;
}


/*---------------------------------------------------------------
	atarigen_6502_irq_gen: Generates an IRQ signal to the 6502
	sound processor.
---------------------------------------------------------------*/

INTERRUPT_GEN( atarigen_6502_irq_gen )
{
	timed_int = 1;
	update_6502_irq();
}


/*---------------------------------------------------------------
	atarigen_6502_irq_ack_r: Resets the IRQ signal to the 6502
	sound processor. Both reads and writes can be used.
---------------------------------------------------------------*/

READ_HANDLER( atarigen_6502_irq_ack_r )
{
	timed_int = 0;
	update_6502_irq();
	return 0;
}

WRITE_HANDLER( atarigen_6502_irq_ack_w )
{
	timed_int = 0;
	update_6502_irq();
}


/*---------------------------------------------------------------
	atarigen_ym2151_irq_gen: Sets the state of the YM2151's
	IRQ line.
---------------------------------------------------------------*/

void atarigen_ym2151_irq_gen(int irq)
{
	ym2151_int = irq;
	update_6502_irq();
}


/*---------------------------------------------------------------
	atarigen_sound_reset_w: Write handler which resets the
	sound CPU in response.
---------------------------------------------------------------*/

WRITE16_HANDLER( atarigen_sound_reset_w )
{
	timer_set(TIME_NOW, 0, delayed_sound_reset);
}


/*---------------------------------------------------------------
	atarigen_sound_reset: Resets the state of the sound CPU
	manually.
---------------------------------------------------------------*/

void atarigen_sound_reset(void)
{
	timer_set(TIME_NOW, 1, delayed_sound_reset);
}


/*---------------------------------------------------------------
	atarigen_sound_w: Handles communication from the main CPU
	to the sound CPU. Two versions are provided, one with the
	data byte in the low 8 bits, and one with the data byte in
	the upper 8 bits.
---------------------------------------------------------------*/

WRITE16_HANDLER( atarigen_sound_w )
{
	if (ACCESSING_LSB)
		timer_set(TIME_NOW, data & 0xff, delayed_sound_w);
}

WRITE16_HANDLER( atarigen_sound_upper_w )
{
	if (ACCESSING_MSB)
		timer_set(TIME_NOW, (data >> 8) & 0xff, delayed_sound_w);
}

WRITE32_HANDLER( atarigen_sound_upper32_w )
{
	if (ACCESSING_MSB32)
		timer_set(TIME_NOW, (data >> 24) & 0xff, delayed_sound_w);
}


/*---------------------------------------------------------------
	atarigen_sound_r: Handles reading data communicated from the
	sound CPU to the main CPU. Two versions are provided, one
	with the data byte in the low 8 bits, and one with the data
	byte in the upper 8 bits.
---------------------------------------------------------------*/

READ16_HANDLER( atarigen_sound_r )
{
	atarigen_sound_to_cpu_ready = 0;
	atarigen_sound_int_ack_w(0, 0, 0);
	return atarigen_sound_to_cpu | 0xff00;
}

READ16_HANDLER( atarigen_sound_upper_r )
{
	atarigen_sound_to_cpu_ready = 0;
	atarigen_sound_int_ack_w(0, 0, 0);
	return (atarigen_sound_to_cpu << 8) | 0x00ff;
}

READ32_HANDLER( atarigen_sound_upper32_r )
{
	atarigen_sound_to_cpu_ready = 0;
	atarigen_sound_int_ack32_w(0, 0, 0);
	return (atarigen_sound_to_cpu << 24) | 0x00ffffff;
}


/*---------------------------------------------------------------
	atarigen_6502_sound_w: Handles communication from the sound
	CPU to the main CPU.
---------------------------------------------------------------*/

WRITE_HANDLER( atarigen_6502_sound_w )
{
	timer_set(TIME_NOW, data, delayed_6502_sound_w);
}


/*---------------------------------------------------------------
	atarigen_6502_sound_r: Handles reading data communicated
	from the main CPU to the sound CPU.
---------------------------------------------------------------*/

READ_HANDLER( atarigen_6502_sound_r )
{
	atarigen_cpu_to_sound_ready = 0;
	cpu_set_nmi_line(sound_cpu_num, CLEAR_LINE);
	return atarigen_cpu_to_sound;
}


/*---------------------------------------------------------------
	update_6502_irq: Called whenever the IRQ state changes. An
	interrupt is generated if either atarigen_6502_irq_gen()
	was called, or if the YM2151 generated an interrupt via
	the atarigen_ym2151_irq_gen() callback.
---------------------------------------------------------------*/

void update_6502_irq(void)
{
	if (timed_int || ym2151_int)
		cpu_set_irq_line(sound_cpu_num, M6502_IRQ_LINE, ASSERT_LINE);
	else
		cpu_set_irq_line(sound_cpu_num, M6502_IRQ_LINE, CLEAR_LINE);
}


/*---------------------------------------------------------------
	sound_comm_timer: Set whenever a command is written from
	the main CPU to the sound CPU, in order to temporarily bump
	up the interleave rate. This helps ensure that communications
	between the two CPUs works properly.
---------------------------------------------------------------*/

static void sound_comm_timer(int reps_left)
{
	if (--reps_left)
		timer_set(SOUND_INTERLEAVE_RATE, reps_left, sound_comm_timer);
}


/*---------------------------------------------------------------
	delayed_sound_reset: Synchronizes the sound reset command
	between the two CPUs.
---------------------------------------------------------------*/

static void delayed_sound_reset(int param)
{
	/* unhalt and reset the sound CPU */
	if (param == 0)
	{
		cpu_set_halt_line(sound_cpu_num, CLEAR_LINE);
		cpu_set_reset_line(sound_cpu_num, PULSE_LINE);
	}

	/* reset the sound write state */
	atarigen_sound_to_cpu_ready = 0;
	atarigen_sound_int_ack_w(0, 0, 0);
}


/*---------------------------------------------------------------
	delayed_sound_w: Synchronizes a data write from the main
	CPU to the sound CPU.
---------------------------------------------------------------*/

static void delayed_sound_w(int param)
{
	/* warn if we missed something */
	if (atarigen_cpu_to_sound_ready)
		log_cb(RETRO_LOG_DEBUG, LOGPRE "Missed command from 68010\n");

	/* set up the states and signal an NMI to the sound CPU */
	atarigen_cpu_to_sound = param;
	atarigen_cpu_to_sound_ready = 1;
	cpu_set_nmi_line(sound_cpu_num, ASSERT_LINE);

	/* allocate a high frequency timer until a response is generated */
	/* the main CPU is *very* sensistive to the timing of the response */
	timer_set(SOUND_INTERLEAVE_RATE, SOUND_INTERLEAVE_REPEAT, sound_comm_timer);
}


/*---------------------------------------------------------------
	delayed_6502_sound_w: Synchronizes a data write from the
	sound CPU to the main CPU.
---------------------------------------------------------------*/

static void delayed_6502_sound_w(int param)
{
	/* warn if we missed something */
	if (atarigen_sound_to_cpu_ready)
		log_cb(RETRO_LOG_DEBUG, LOGPRE "Missed result from 6502\n");

	/* set up the states and signal the sound interrupt to the main CPU */
	atarigen_sound_to_cpu = param;
	atarigen_sound_to_cpu_ready = 1;
	atarigen_sound_int_gen();
}



/*##########################################################################
	SOUND HELPERS
##########################################################################*/

/*---------------------------------------------------------------
	atarigen_init_6502_speedup: Installs a special read handler
	to catch the main spin loop in the 6502 sound code. The
	addresses accessed seem to be the same across a large
	number of games, though the PC shifts.
---------------------------------------------------------------*/

void atarigen_init_6502_speedup(int cpunum, int compare_pc1, int compare_pc2)
{
	UINT8 *memory = memory_region(REGION_CPU1+cpunum);
	int address_low, address_high;

	/* determine the pointer to the first speed check location */
	address_low = memory[compare_pc1 + 1] | (memory[compare_pc1 + 2] << 8);
	address_high = memory[compare_pc1 + 4] | (memory[compare_pc1 + 5] << 8);
	if (address_low != address_high - 1)
		log_cb(RETRO_LOG_DEBUG, LOGPRE "Error: address %04X does not point to a speedup location!", compare_pc1);
	speed_a = &memory[address_low];

	/* determine the pointer to the second speed check location */
	address_low = memory[compare_pc2 + 1] | (memory[compare_pc2 + 2] << 8);
	address_high = memory[compare_pc2 + 4] | (memory[compare_pc2 + 5] << 8);
	if (address_low != address_high - 1)
		log_cb(RETRO_LOG_DEBUG, LOGPRE "Error: address %04X does not point to a speedup location!", compare_pc2);
	speed_b = &memory[address_low];

	/* install a handler on the second address */
	speed_pc = compare_pc2;
	install_mem_read_handler(cpunum, address_low, address_low, m6502_speedup_r);
}


/*---------------------------------------------------------------
	atarigen_set_vol: Scans for a particular sound chip and
	changes the volume on all channels associated with it.
---------------------------------------------------------------*/

void atarigen_set_vol(int volume, const char *string)
{
	int ch;

	for (ch = 0; ch < MIXER_MAX_CHANNELS; ch++)
	{
		const char *name = mixer_get_name(ch);
		if (name && strstr(name, string))
			mixer_set_volume(ch, volume);
	}
}


/*---------------------------------------------------------------
	atarigen_set_XXXXX_vol: Sets the volume for a given type
	of chip.
---------------------------------------------------------------*/

void atarigen_set_ym2151_vol(int volume)
{
	atarigen_set_vol(volume, "2151");
}

void atarigen_set_ym2413_vol(int volume)
{
	atarigen_set_vol(volume, "2413");
}

void atarigen_set_pokey_vol(int volume)
{
	atarigen_set_vol(volume, "POKEY");
}

void atarigen_set_tms5220_vol(int volume)
{
	atarigen_set_vol(volume, "5220");
}

void atarigen_set_oki6295_vol(int volume)
{
	atarigen_set_vol(volume, "6295");
}


/*---------------------------------------------------------------
	m6502_speedup_r: Handles speeding up the 6502.
---------------------------------------------------------------*/

static READ_HANDLER( m6502_speedup_r )
{
	int result = speed_b[0];

	if (activecpu_get_previouspc() == speed_pc && speed_a[0] == speed_a[1] && result == speed_b[1])
		cpu_spinuntil_int();

	return result;
}



/*##########################################################################
	SCANLINE TIMING
##########################################################################*/

/*---------------------------------------------------------------
	atarigen_scanline_timer_reset: Sets up the scanline timer.
---------------------------------------------------------------*/

void atarigen_scanline_timer_reset(atarigen_scanline_callback update_graphics, int frequency)
{
	/* set the scanline callback */
	scanline_callback = update_graphics;
	scanline_callback_period = (double)frequency * cpu_getscanlineperiod();
	scanlines_per_callback = frequency;

	/* compute the last scanline */
	last_scanline = (int)(TIME_IN_HZ(Machine->drv->frames_per_second) / cpu_getscanlineperiod());

	/* set a timer to go off on the next VBLANK */
	timer_set(cpu_getscanlinetime(Machine->drv->screen_height), 0, vblank_timer);
}


/*---------------------------------------------------------------
	vblank_timer: Called once every VBLANK to prime the scanline
	timers.
---------------------------------------------------------------*/

static void vblank_timer(int param)
{
	/* set a timer to go off at scanline 0 */
	timer_set(TIME_IN_USEC(Machine->drv->vblank_duration), 0, scanline_timer);

	/* set a timer to go off on the next VBLANK */
	timer_set(cpu_getscanlinetime(Machine->drv->screen_height), 1, vblank_timer);
}


/*---------------------------------------------------------------
	scanline_timer: Called once every n scanlines to generate
	the periodic callback to the main system.
---------------------------------------------------------------*/

static void scanline_timer(int scanline)
{
	/* callback */
	if (scanline_callback)
	{
		(*scanline_callback)(scanline);

		/* generate another? */
		scanline += scanlines_per_callback;
		if (scanline < last_scanline && scanlines_per_callback)
			timer_set(scanline_callback_period, scanline, scanline_timer);
	}
}



/*##########################################################################
	VIDEO CONTROLLER
##########################################################################*/

/*---------------------------------------------------------------
	atarivc_eof_update: Callback that slurps up data and feeds
	it into the video controller registers every refresh.
---------------------------------------------------------------*/

static void atarivc_eof_update(int param)
{
	atarivc_update(atarivc_eof_data);
	timer_set(cpu_getscanlinetime(0), 0, atarivc_eof_update);
}


/*---------------------------------------------------------------
	atarivc_reset: Initializes the video controller.
---------------------------------------------------------------*/

void atarivc_reset(data16_t *eof_data, int playfields)
{
	/* this allows us to manually reset eof_data to NULL if it's not used */
	atarivc_eof_data = eof_data;
	atarivc_playfields = playfields;

	/* clear the RAM we use */
	memset(atarivc_data, 0, 0x40);
	memset(&atarivc_state, 0, sizeof(atarivc_state));

	/* reset the latches */
	atarivc_state.latch1 = atarivc_state.latch2 = -1;
	actual_vc_latch0 = actual_vc_latch1 = -1;

	/* start a timer to go off a little before scanline 0 */
	if (atarivc_eof_data)
		timer_set(cpu_getscanlinetime(0), 0, atarivc_eof_update);
}


/*---------------------------------------------------------------
	atarivc_update: Copies the data from the specified location
	once/frame into the video controller registers.
---------------------------------------------------------------*/

void atarivc_update(const data16_t *data)
{
	int i;

	/* echo all the commands to the video controller */
	for (i = 0; i < 0x1c; i++)
		if (data[i])
			atarivc_common_w(i, data[i]);

	/* update the scroll positions */
	atarimo_set_xscroll(0, atarivc_state.mo_xscroll);
	atarimo_set_yscroll(0, atarivc_state.mo_yscroll);

	tilemap_set_scrollx(atarigen_playfield_tilemap, 0, atarivc_state.pf0_xscroll);
	tilemap_set_scrolly(atarigen_playfield_tilemap, 0, atarivc_state.pf0_yscroll);

	if (atarivc_playfields > 1)
	{
		tilemap_set_scrollx(atarigen_playfield2_tilemap, 0, atarivc_state.pf1_xscroll);
		tilemap_set_scrolly(atarigen_playfield2_tilemap, 0, atarivc_state.pf1_yscroll);
	}

	/* use this for debugging the video controller values */
#if 0
	if (keyboard_pressed(KEYCODE_8))
	{
		static FILE *out;
		if (!out) out = fopen("scroll.log", "w");
		if (out)
		{
			for (i = 0; i < 64; i++)
				fprintf(out, "%04X ", data[i]);
			fprintf(out, "\n");
		}
	}
#endif
}


/*---------------------------------------------------------------
	atarivc_w: Handles an I/O write to the video controller.
---------------------------------------------------------------*/

WRITE16_HANDLER( atarivc_w )
{
	int oldword = atarivc_data[offset];
	int newword = oldword;

	COMBINE_DATA(&newword);
	atarivc_common_w(offset, newword);
}



/*---------------------------------------------------------------
	atarivc_common_w: Does the bulk of the word for an I/O
	write.
---------------------------------------------------------------*/

static void atarivc_common_w(offs_t offset, data16_t newword)
{
	int oldword = atarivc_data[offset];
	atarivc_data[offset] = newword;

	/* switch off the offset */
	switch (offset)
	{
		/*
			additional registers:

				01 = vertical start (for centering)
				04 = horizontal start (for centering)
		*/

		/* set the scanline interrupt here */
		case 0x03:
			if (oldword != newword)
				atarigen_scanline_int_set(newword & 0x1ff);
			break;

		/* latch enable */
		case 0x0a:

			/* reset the latches when disabled */
			atarigen_set_playfield_latch((newword & 0x0080) ? actual_vc_latch0 : -1);
			atarigen_set_playfield2_latch((newword & 0x0080) ? actual_vc_latch1 : -1);

			/* check for rowscroll enable */
			atarivc_state.rowscroll_enable = (newword & 0x2000) >> 13;

			/* check for palette banking */
			if (atarivc_state.palette_bank != (((newword & 0x0400) >> 10) ^ 1))
			{
				force_partial_update(cpu_getscanline());
				atarivc_state.palette_bank = ((newword & 0x0400) >> 10) ^ 1;
			}
			break;

		/* indexed parameters */
		case 0x10: case 0x11: case 0x12: case 0x13:
		case 0x14: case 0x15: case 0x16: case 0x17:
		case 0x18: case 0x19: case 0x1a: case 0x1b:
			switch (newword & 15)
			{
				case 9:
					atarivc_state.mo_xscroll = (newword >> 7) & 0x1ff;
					break;

				case 10:
					atarivc_state.pf1_xscroll_raw = (newword >> 7) & 0x1ff;
					atarivc_update_pf_xscrolls();
					break;

				case 11:
					atarivc_state.pf0_xscroll_raw = (newword >> 7) & 0x1ff;
					atarivc_update_pf_xscrolls();
					break;

				case 13:
					atarivc_state.mo_yscroll = (newword >> 7) & 0x1ff;
					break;

				case 14:
					atarivc_state.pf1_yscroll = (newword >> 7) & 0x1ff;
					break;

				case 15:
					atarivc_state.pf0_yscroll = (newword >> 7) & 0x1ff;
					break;
			}
			break;

		/* latch 1 value */
		case 0x1c:
			actual_vc_latch0 = -1;
			actual_vc_latch1 = newword;
			atarigen_set_playfield_latch((atarivc_data[0x0a] & 0x80) ? actual_vc_latch0 : -1);
			atarigen_set_playfield2_latch((atarivc_data[0x0a] & 0x80) ? actual_vc_latch1 : -1);
			break;

		/* latch 2 value */
		case 0x1d:
			actual_vc_latch0 = newword;
			actual_vc_latch1 = -1;
			atarigen_set_playfield_latch((atarivc_data[0x0a] & 0x80) ? actual_vc_latch0 : -1);
			atarigen_set_playfield2_latch((atarivc_data[0x0a] & 0x80) ? actual_vc_latch1 : -1);
			break;

		/* scanline IRQ ack here */
		case 0x1e:
			atarigen_scanline_int_ack_w(0, 0, 0);
			break;

		/* log anything else */
		case 0x00:
		default:
			if (oldword != newword)
				log_cb(RETRO_LOG_DEBUG, LOGPRE "vc_w(%02X, %04X) ** [prev=%04X]\n", offset, newword, oldword);
			break;
	}
}


/*---------------------------------------------------------------
	atarivc_r: Handles an I/O read from the video controller.
---------------------------------------------------------------*/

READ16_HANDLER( atarivc_r )
{
	log_cb(RETRO_LOG_DEBUG, LOGPRE "vc_r(%02X)\n", offset);

	/* a read from offset 0 returns the current scanline */
	/* also sets bit 0x4000 if we're in VBLANK */
	if (offset == 0)
	{
		int result = cpu_getscanline();

		if (result > 255)
			result = 255;
		if (result > Machine->visible_area.max_y)
			result |= 0x4000;

		return result;
	}
	else
		return atarivc_data[offset];
}



/*##########################################################################
	PLAYFIELD/ALPHA MAP HELPERS
##########################################################################*/

/*---------------------------------------------------------------
	atarigen_alpha_w: Generic write handler for alpha RAM.
---------------------------------------------------------------*/

WRITE16_HANDLER( atarigen_alpha_w )
{
	COMBINE_DATA(&atarigen_alpha[offset]);
	tilemap_mark_tile_dirty(atarigen_alpha_tilemap, offset);
}

WRITE32_HANDLER( atarigen_alpha32_w )
{
	COMBINE_DATA(&atarigen_alpha32[offset]);
	if ((mem_mask & 0xffff0000) != 0xffff0000)
		tilemap_mark_tile_dirty(atarigen_alpha_tilemap, offset * 2);
	if ((mem_mask & 0x0000ffff) != 0x0000ffff)
		tilemap_mark_tile_dirty(atarigen_alpha_tilemap, offset * 2 + 1);
}

WRITE16_HANDLER( atarigen_alpha2_w )
{
	COMBINE_DATA(&atarigen_alpha2[offset]);
	tilemap_mark_tile_dirty(atarigen_alpha2_tilemap, offset);
}



/*---------------------------------------------------------------
	atarigen_set_playfield_latch: Sets the latch for the latched
	playfield handlers below.
---------------------------------------------------------------*/

void atarigen_set_playfield_latch(int data)
{
	playfield_latch = data;
}

void atarigen_set_playfield2_latch(int data)
{
	playfield2_latch = data;
}



/*---------------------------------------------------------------
	atarigen_playfield_w: Generic write handler for PF RAM.
---------------------------------------------------------------*/

WRITE16_HANDLER( atarigen_playfield_w )
{
	COMBINE_DATA(&atarigen_playfield[offset]);
	tilemap_mark_tile_dirty(atarigen_playfield_tilemap, offset);
}

WRITE32_HANDLER( atarigen_playfield32_w )
{
	COMBINE_DATA(&atarigen_playfield32[offset]);
	if ((mem_mask & 0xffff0000) != 0xffff0000)
		tilemap_mark_tile_dirty(atarigen_playfield_tilemap, offset * 2);
	if ((mem_mask & 0x0000ffff) != 0x0000ffff)
		tilemap_mark_tile_dirty(atarigen_playfield_tilemap, offset * 2 + 1);
}

WRITE16_HANDLER( atarigen_playfield2_w )
{
	COMBINE_DATA(&atarigen_playfield2[offset]);
	tilemap_mark_tile_dirty(atarigen_playfield2_tilemap, offset);
}



/*---------------------------------------------------------------
	atarigen_playfield_large_w: Generic write handler for
	large (2-word) playfield RAM.
---------------------------------------------------------------*/

WRITE16_HANDLER( atarigen_playfield_large_w )
{
	COMBINE_DATA(&atarigen_playfield[offset]);
	tilemap_mark_tile_dirty(atarigen_playfield_tilemap, offset / 2);
}



/*---------------------------------------------------------------
	atarigen_playfield_upper_w: Generic write handler for
	upper word of split playfield RAM.
---------------------------------------------------------------*/

WRITE16_HANDLER( atarigen_playfield_upper_w )
{
	COMBINE_DATA(&atarigen_playfield_upper[offset]);
	tilemap_mark_tile_dirty(atarigen_playfield_tilemap, offset);
}



/*---------------------------------------------------------------
	atarigen_playfield_dual_upper_w: Generic write handler for
	upper word of split dual playfield RAM.
---------------------------------------------------------------*/

WRITE16_HANDLER( atarigen_playfield_dual_upper_w )
{
	COMBINE_DATA(&atarigen_playfield_upper[offset]);
	tilemap_mark_tile_dirty(atarigen_playfield_tilemap, offset);
	tilemap_mark_tile_dirty(atarigen_playfield2_tilemap, offset);
}



/*---------------------------------------------------------------
	atarigen_playfield_latched_lsb_w: Generic write handler for
	lower word of playfield RAM with a latch in the LSB of the
	upper word.
---------------------------------------------------------------*/

WRITE16_HANDLER( atarigen_playfield_latched_lsb_w )
{
	COMBINE_DATA(&atarigen_playfield[offset]);
	tilemap_mark_tile_dirty(atarigen_playfield_tilemap, offset);
	
	if (playfield_latch != -1)
		atarigen_playfield_upper[offset] = (atarigen_playfield_upper[offset] & ~0x00ff) | (playfield_latch & 0x00ff);
}



/*---------------------------------------------------------------
	atarigen_playfield_latched_lsb_w: Generic write handler for
	lower word of playfield RAM with a latch in the MSB of the
	upper word.
---------------------------------------------------------------*/

WRITE16_HANDLER( atarigen_playfield_latched_msb_w )
{
	COMBINE_DATA(&atarigen_playfield[offset]);
	tilemap_mark_tile_dirty(atarigen_playfield_tilemap, offset);
	
	if (playfield_latch != -1)
		atarigen_playfield_upper[offset] = (atarigen_playfield_upper[offset] & ~0xff00) | (playfield_latch & 0xff00);
}



/*---------------------------------------------------------------
	atarigen_playfield_latched_lsb_w: Generic write handler for
	lower word of second playfield RAM with a latch in the MSB 
	of the upper word.
---------------------------------------------------------------*/

WRITE16_HANDLER( atarigen_playfield2_latched_msb_w )
{
	COMBINE_DATA(&atarigen_playfield2[offset]);
	tilemap_mark_tile_dirty(atarigen_playfield2_tilemap, offset);
	
	if (playfield2_latch != -1)
		atarigen_playfield_upper[offset] = (atarigen_playfield_upper[offset] & ~0xff00) | (playfield2_latch & 0xff00);
}




/*##########################################################################
	VIDEO HELPERS
##########################################################################*/

/*---------------------------------------------------------------
	atarigen_get_hblank: Returns a guesstimate about the current
	HBLANK state, based on the assumption that HBLANK represents
	10% of the scanline period.
---------------------------------------------------------------*/

int atarigen_get_hblank(void)
{
	return (cpu_gethorzbeampos() > (Machine->drv->screen_width * 9 / 10));
}


/*---------------------------------------------------------------
	atarigen_halt_until_hblank_0_w: Halts CPU 0 until the
	next HBLANK.
---------------------------------------------------------------*/

WRITE16_HANDLER( atarigen_halt_until_hblank_0_w )
{
	/* halt the CPU until the next HBLANK */
	int hpos = cpu_gethorzbeampos();
	int hblank = Machine->drv->screen_width * 9 / 10;
	double fraction;

	/* if we're in hblank, set up for the next one */
	if (hpos >= hblank)
		hblank += Machine->drv->screen_width;

	/* halt and set a timer to wake up */
	fraction = (double)(hblank - hpos) / (double)Machine->drv->screen_width;
	timer_set(cpu_getscanlineperiod() * fraction, 0, unhalt_cpu);
	cpu_set_halt_line(0, ASSERT_LINE);
}


/*---------------------------------------------------------------
	atarigen_666_paletteram_w: 6-6-6 RGB palette RAM handler.
---------------------------------------------------------------*/

WRITE16_HANDLER( atarigen_666_paletteram_w )
{
	int newword, r, g, b;

	COMBINE_DATA(&paletteram16[offset]);
	newword = paletteram16[offset];

	r = ((newword >> 9) & 0x3e) | ((newword >> 15) & 1);
	g = ((newword >> 4) & 0x3e) | ((newword >> 15) & 1);
	b = ((newword << 1) & 0x3e) | ((newword >> 15) & 1);

	r = (r << 2) | (r >> 4);
	g = (g << 2) | (g >> 4);
	b = (b << 2) | (b >> 4);

	palette_set_color(offset, r, g, b);
}


/*---------------------------------------------------------------
	atarigen_expanded_666_paletteram_w: 6-6-6 RGB expanded
	palette RAM handler.
---------------------------------------------------------------*/

WRITE16_HANDLER( atarigen_expanded_666_paletteram_w )
{
	COMBINE_DATA(&paletteram16[offset]);

	if (ACCESSING_MSB)
	{
		int palentry = offset / 2;
		int newword = (paletteram16[palentry * 2] & 0xff00) | (paletteram16[palentry * 2 + 1] >> 8);

		int r, g, b;

		r = ((newword >> 9) & 0x3e) | ((newword >> 15) & 1);
		g = ((newword >> 4) & 0x3e) | ((newword >> 15) & 1);
		b = ((newword << 1) & 0x3e) | ((newword >> 15) & 1);

		r = (r << 2) | (r >> 4);
		g = (g << 2) | (g >> 4);
		b = (b << 2) | (b >> 4);

		palette_set_color(palentry & 0x1ff, r, g, b);
	}
}


/*---------------------------------------------------------------
	atarigen_666_paletteram32_w: 6-6-6 RGB palette RAM handler.
---------------------------------------------------------------*/

WRITE32_HANDLER( atarigen_666_paletteram32_w )
{
	int newword, r, g, b;

	COMBINE_DATA(&paletteram32[offset]);

	if (ACCESSING_MSW32)
	{
		newword = paletteram32[offset] >> 16;

		r = ((newword >> 9) & 0x3e) | ((newword >> 15) & 1);
		g = ((newword >> 4) & 0x3e) | ((newword >> 15) & 1);
		b = ((newword << 1) & 0x3e) | ((newword >> 15) & 1);

		r = (r << 2) | (r >> 4);
		g = (g << 2) | (g >> 4);
		b = (b << 2) | (b >> 4);

		palette_set_color(offset * 2, r, g, b);
	}

	if (ACCESSING_LSW32)
	{
		newword = paletteram32[offset] & 0xffff;

		r = ((newword >> 9) & 0x3e) | ((newword >> 15) & 1);
		g = ((newword >> 4) & 0x3e) | ((newword >> 15) & 1);
		b = ((newword << 1) & 0x3e) | ((newword >> 15) & 1);

		r = (r << 2) | (r >> 4);
		g = (g << 2) | (g >> 4);
		b = (b << 2) | (b >> 4);

		palette_set_color(offset * 2 + 1, r, g, b);
	}
}


/*---------------------------------------------------------------
	unhalt_cpu: Timer callback to release the CPU from a halted state.
---------------------------------------------------------------*/

static void unhalt_cpu(int param)
{
	cpu_set_halt_line(param, CLEAR_LINE);
}



/*##########################################################################
	MISC HELPERS
##########################################################################*/

/*---------------------------------------------------------------
	atarigen_swap_mem: Inverts the bits in a region.
---------------------------------------------------------------*/

void atarigen_swap_mem(void *ptr1, void *ptr2, int bytes)
{
	UINT8 *p1 = ptr1;
	UINT8 *p2 = ptr2;
	while (bytes--)
	{
		int temp = *p1;
		*p1++ = *p2;
		*p2++ = temp;
	}
}


/*---------------------------------------------------------------
	atarigen_blend_gfx: Takes two GFXElements and blends their
	data together to form one. Then frees the second.
---------------------------------------------------------------*/

void atarigen_blend_gfx(int gfx0, int gfx1, int mask0, int mask1)
{
	struct GfxElement *gx0 = Machine->gfx[gfx0];
	struct GfxElement *gx1 = Machine->gfx[gfx1];
	int c, x, y;

	/* loop over elements */
	for (c = 0; c < gx0->total_elements; c++)
	{
		UINT8 *c0base = gx0->gfxdata + gx0->char_modulo * c;
		UINT8 *c1base = gx1->gfxdata + gx1->char_modulo * c;
		UINT32 usage = 0;

		/* loop over height */
		for (y = 0; y < gx0->height; y++)
		{
			UINT8 *c0 = c0base, *c1 = c1base;

			for (x = 0; x < gx0->width; x++, c0++, c1++)
			{
				*c0 = (*c0 & mask0) | (*c1 & mask1);
				usage |= 1 << *c0;
			}
			c0base += gx0->line_modulo;
			c1base += gx1->line_modulo;
			if (gx0->pen_usage)
				gx0->pen_usage[c] = usage;
		}
	}

	/* free the second graphics element */
	freegfx(gx1);
	Machine->gfx[gfx1] = NULL;
}
