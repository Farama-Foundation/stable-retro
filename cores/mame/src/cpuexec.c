/***************************************************************************

	cpuexec.c

	Core multi-CPU execution engine.

***************************************************************************/

#include <math.h>
#include "driver.h"
#include "timer.h"
#include "state.h"
#include "mamedbg.h"
#include "cpuexec.h"
#include "log.h"


#if (HAS_M68000 || HAS_M68010 || HAS_M68020 || HAS_M68EC020)
#include "cpu/m68000/m68000.h"
#endif
#if (HAS_CYCLONE)
#include "cpu/m68000_cyclone/c68000.h"
#endif

/*************************************
 *
 *	Macros to help verify active CPU
 *
 *************************************/

#define VERIFY_ACTIVECPU(retval, name)						\
	int activecpu = cpu_getactivecpu();						\
	if (activecpu < 0)										\
	{														\
		log_cb(RETRO_LOG_ERROR, LOGPRE #name "() called with no active cpu!\n");	\
		return retval;										\
	}

#define VERIFY_ACTIVECPU_VOID(name)							\
	int activecpu = cpu_getactivecpu();						\
	if (activecpu < 0)										\
	{														\
		log_cb(RETRO_LOG_ERROR, LOGPRE #name "() called with no active cpu!\n");	\
		return;												\
	}



/*************************************
 *
 *	Macros to help verify executing CPU
 *
 *************************************/

#define VERIFY_EXECUTINGCPU(retval, name)					\
	int activecpu = cpu_getexecutingcpu();					\
	if (activecpu < 0)										\
	{														\
		log_cb(RETRO_LOG_ERROR, LOGPRE #name "() called with no executing cpu!\n");\
		return retval;										\
	}

#define VERIFY_EXECUTINGCPU_VOID(name)						\
	int activecpu = cpu_getexecutingcpu();					\
	if (activecpu < 0)										\
	{														\
		log_cb(RETRO_LOG_ERROR, LOGPRE #name "() called with no executing cpu!\n");\
		return;												\
	}



/*************************************
 *
 *	Macros to help verify CPU index
 *
 *************************************/

#define VERIFY_CPUNUM(retval, name)							\
	if (cpunum < 0 || cpunum >= cpu_gettotalcpu())			\
	{														\
		log_cb(RETRO_LOG_ERROR, #name "() called for invalid cpu num!\n");	\
		return retval;										\
	}

#define VERIFY_CPUNUM_VOID(name)							\
	if (cpunum < 0 || cpunum >= cpu_gettotalcpu())			\
	{														\
		log_cb(RETRO_LOG_ERROR, #name "() called for invalid cpu num!\n");	\
		return;												\
	}



/*************************************
 *
 *	Triggers for the timer system
 *
 *************************************/

enum
{
	TRIGGER_TIMESLICE 	= -1000,
	TRIGGER_INT 		= -2000,
	TRIGGER_YIELDTIME 	= -3000,
	TRIGGER_SUSPENDTIME = -4000
};



/*************************************
 *
 *	Internal CPU info structure
 *
 *************************************/

struct cpuinfo
{
	int		suspend;				/* suspend reason mask (0 = not suspended) */
	int		nextsuspend;			/* pending suspend reason mask */
	int		eatcycles;				/* true if we eat cycles while suspended */
	int		nexteatcycles;			/* pending value */
	int		trigger;				/* pending trigger to release a trigger suspension */

	int 	iloops; 				/* number of interrupts remaining this frame */

	UINT64 	totalcycles;			/* total CPU cycles executed */
	double	localtime;				/* local time, relative to the timer system's global time */
	double	clockscale;				/* current active clock scale factor */

	int 	vblankint_countdown;	/* number of vblank callbacks left until we interrupt */
	int 	vblankint_multiplier;	/* number of vblank callbacks per interrupt */
	void *	vblankint_timer;		/* reference to elapsed time counter */
	double	vblankint_period;		/* timing period of the VBLANK interrupt */

	void *	timedint_timer;			/* reference to this CPU's timer */
	double	timedint_period; 		/* timing period of the timed interrupt */
};



/*************************************
 *
 *	General CPU variables
 *
 *************************************/

static struct cpuinfo cpu[MAX_CPU];

static int time_to_reset;
static int time_to_quit;

static int vblank;
static int current_frame;
static INT32 watchdog_counter;

static int cycles_running;
static int cycles_stolen;



/*************************************
 *
 *	Timer variables
 *
 *************************************/

static void *vblank_timer;
static int vblank_countdown;
static int vblank_multiplier;
static double vblank_period;

static void *refresh_timer;
static double refresh_period;
static double refresh_period_inv;

static void *timeslice_timer;
static double timeslice_period;

static double scanline_period;
static double scanline_period_inv;

static void *interleave_boost_timer;
static void *interleave_boost_timer_end;
static double perfect_interleave;




/*************************************
 *
 *	Static prototypes
 *
 *************************************/

static void cpu_timeslice(void);
static void cpu_inittimers(void);
static void cpu_vblankreset(void);
static void cpu_vblankcallback(int param);
static void cpu_updatecallback(int param);
static void end_interleave_boost(int param);
static void compute_perfect_interleave(void);



#if 0
#pragma mark CORE CPU
#endif

/*************************************
 *
 *	Initialize all the CPUs
 *
 *************************************/

int cpu_init(void)
{
	int cpunum;

	/* initialize the interfaces first */
	if (cpuintrf_init())
		return 1;

	/* loop over all our CPUs */
	for (cpunum = 0; cpunum < MAX_CPU; cpunum++)
	{
		int cputype = Machine->drv->cpu[cpunum].cpu_type;

		/* if this is a dummy, stop looking */
		if (cputype == CPU_DUMMY)
			break;

		/* set the save state tag */
		state_save_set_current_tag(cpunum + 1);

		/* initialize the cpuinfo struct */
		memset(&cpu[cpunum], 0, sizeof(cpu[cpunum]));
		cpu[cpunum].suspend = SUSPEND_REASON_RESET;
		cpu[cpunum].clockscale = cputype_get_interface(cputype)->overclock;

		/* compute the cycle times */
		sec_to_cycles[cpunum] = cpu[cpunum].clockscale * Machine->drv->cpu[cpunum].cpu_clock;
		cycles_to_sec[cpunum] = 1.0 / sec_to_cycles[cpunum];

		/* initialize this CPU */
		if (cpuintrf_init_cpu(cpunum, cputype))
			return 1;
	}

	/* compute the perfect interleave factor */
	compute_perfect_interleave();

	/* save some stuff in tag 0 */
	state_save_set_current_tag(0);
	state_save_register_INT32("cpu", 0, "watchdog count", &watchdog_counter, 1);

	/* reset the IRQ lines and save those */
	if (cpuint_init())
		return 1;

	return 0;
}



/*************************************
 *
 *	Prepare the system for execution
 *
 *************************************/

static void cpu_pre_run(void)
{
	int cpunum;

	log_cb(RETRO_LOG_INFO, LOGPRE "Preparing emulated CPUs for execution.\n");

	begin_resource_tracking();

	/* read hi scores information from hiscore.dat */
	hs_open(Machine->gamedrv->name);
	hs_init();

	/* initialize the various timers (suspends all CPUs at startup) */
	cpu_inittimers();
	watchdog_counter = -1;

	/* reset sound chips */
	sound_reset();

	/* first pass over CPUs */
	for (cpunum = 0; cpunum < cpu_gettotalcpu(); cpunum++)
	{
		/* enable all CPUs (except for audio CPUs if the sound is off) */
		if (!(Machine->drv->cpu[cpunum].cpu_flags & CPU_AUDIO_CPU) || Machine->sample_rate != 0)
			cpunum_resume(cpunum, SUSPEND_ANY_REASON);
		else
			cpunum_suspend(cpunum, SUSPEND_REASON_DISABLE, 1);

		/* reset the interrupt state */
		cpuint_reset_cpu(cpunum);

		/* reset the total number of cycles */
		cpu[cpunum].totalcycles = 0;
		cpu[cpunum].localtime = 0;
	}

	vblank = 0;

	/* do this AFTER the above so machine_init() can use cpu_halt() to hold the */
	/* execution of some CPUs, or disable interrupts */
	if (Machine->drv->machine_init)
		(*Machine->drv->machine_init)();

	/* now reset each CPU */
	for (cpunum = 0; cpunum < cpu_gettotalcpu(); cpunum++)
		cpunum_reset(cpunum, Machine->drv->cpu[cpunum].reset_param, cpu_irq_callbacks[cpunum]);

	/* reset the globals */
	cpu_vblankreset();
	current_frame = 0;
	state_save_dump_registry();
}



/*************************************
 *
 *	Finish up execution
 *
 *************************************/

static void cpu_post_run(void)
{
	/* write hi scores to disk - No scores saving if cheat */
	hs_close();

	/* stop the machine */
	if (Machine->drv->machine_stop)
		(*Machine->drv->machine_stop)();

	end_resource_tracking();
}



/*************************************
 *
 *	Execute until done
 *
 *************************************/

void (*pause_action)(void);

void mame_frame(void)
{
    if(!pause_action)
    {
        extern int gotFrame;

        while(!gotFrame)
        {
            cpu_timeslice();
        }

        gotFrame = 0;

        if(time_to_reset)
        {
            cpu_post_run();
            cpu_pre_run();
            time_to_reset = 0;
        }
    }
    else
    {
        pause_action();
    }
}

void cpu_run_done(void)
{
	cpu_post_run();
}

void cpu_run(void)
{
    cpu_pre_run();
}



/*************************************
 *
 *	Deinitialize all the CPUs
 *
 *************************************/

void cpu_exit(void)
{
	int cpunum;

	/* shut down the CPU cores */
	for (cpunum = 0; cpunum < cpu_gettotalcpu(); cpunum++)
		cpuintrf_exit_cpu(cpunum);
}



/*************************************
 *
 *	Force a reset at the end of this
 *	timeslice
 *
 *************************************/

void machine_reset(void)
{
	time_to_reset = 1;
}


#if 0
#pragma mark -
#pragma mark WATCHDOG
#endif

/*************************************
 *
 *	Watchdog routines
 *
 *************************************/

/*--------------------------------------------------------------

	Use these functions to initialize, and later maintain, the
	watchdog. For convenience, when the machine is reset, the
	watchdog is disabled. If you call this function, the
	watchdog is initialized, and from that point onwards, if you
	don't call it at least once every 3 seconds, the machine
	will be reset.

	The 3 seconds delay is targeted at qzshowby, which otherwise
	would reset at the start of a game.

--------------------------------------------------------------*/

static void watchdog_reset(void)
{
	if (watchdog_counter == -1)
		log_cb(RETRO_LOG_INFO, "watchdog armed\n");
	watchdog_counter = 3 * Machine->drv->frames_per_second;
}


WRITE_HANDLER( watchdog_reset_w )
{
	watchdog_reset();
}

WRITE_HANDLER( watchdog_400_reset_w )
{
		watchdog_counter = 400;
}


READ_HANDLER( watchdog_reset_r )
{
	watchdog_reset();
	return 0xff;
}


WRITE16_HANDLER( watchdog_reset16_w )
{
	watchdog_reset();
}


READ16_HANDLER( watchdog_reset16_r )
{
	watchdog_reset();
	return 0xffff;
}


WRITE32_HANDLER( watchdog_reset32_w )
{
	watchdog_reset();
}


READ32_HANDLER( watchdog_reset32_r )
{
	watchdog_reset();
	return 0xffffffff;
}



#if 0
#pragma mark -
#pragma mark HALT/RESET
#endif

/*************************************
 *
 *	Handle reset line changes
 *
 *************************************/

static void reset_callback(int param)
{
	int cpunum = param & 0xff;
	int state = param >> 8;

	/* if we're asserting the line, just halt the CPU */
	if (state == ASSERT_LINE)
	{
		cpunum_suspend(cpunum, SUSPEND_REASON_RESET, 1);
		return;
	}

	/* if we're clearing the line that was previously asserted, or if we're just */
	/* pulsing the line, reset the CPU */
	if ((state == CLEAR_LINE && (cpu[cpunum].suspend & SUSPEND_REASON_RESET)) || state == PULSE_LINE)
		cpunum_reset(cpunum, Machine->drv->cpu[cpunum].reset_param, cpu_irq_callbacks[cpunum]);

	/* if we're clearing the line, make sure the CPU is not halted */
	cpunum_resume(cpunum, SUSPEND_REASON_RESET);
}


void cpunum_set_reset_line(int cpunum, int state)
{
	timer_set(TIME_NOW, (cpunum & 0xff) | (state << 8), reset_callback);
}



/*************************************
 *
 *	Handle halt line changes
 *
 *************************************/

static void halt_callback(int param)
{
	int cpunum = param & 0xff;
	int state = param >> 8;

	/* if asserting, halt the CPU */
	if (state == ASSERT_LINE)
		cpunum_suspend(cpunum, SUSPEND_REASON_HALT, 1);

	/* if clearing, unhalt the CPU */
	else if (state == CLEAR_LINE)
		cpunum_resume(cpunum, SUSPEND_REASON_HALT);
}


void cpunum_set_halt_line(int cpunum, int state)
{
	timer_set(TIME_NOW, (cpunum & 0xff) | (state << 8), halt_callback);
}




#if 0
#pragma mark -
#pragma mark CPU SCHEDULING
#endif

/*************************************
 *
 *	Execute all the CPUs for one
 *	timeslice
 *
 *************************************/

static void cpu_timeslice(void)
{
	double target = timer_time_until_next_timer();
	int cpunum, ran;

	log_cb(RETRO_LOG_DEBUG, LOGPRE "------------------\n");
	log_cb(RETRO_LOG_DEBUG, LOGPRE "cpu_timeslice: target = %.9f\n", target);

	/* process any pending suspends */
	for (cpunum = 0; Machine->drv->cpu[cpunum].cpu_type != CPU_DUMMY; cpunum++)
	{
		if (cpu[cpunum].suspend != cpu[cpunum].nextsuspend)
			log_cb(RETRO_LOG_DEBUG, LOGPRE "--> updated CPU%d suspend from %X to %X\n", cpunum, cpu[cpunum].suspend, cpu[cpunum].nextsuspend);
		cpu[cpunum].suspend = cpu[cpunum].nextsuspend;
		cpu[cpunum].eatcycles = cpu[cpunum].nexteatcycles;
	}

	/* loop over CPUs */
	for (cpunum = 0; Machine->drv->cpu[cpunum].cpu_type != CPU_DUMMY; cpunum++)
	{
		/* only process if we're not suspended */
		if (!cpu[cpunum].suspend)
		{
			/* compute how long to run */
			cycles_running = TIME_TO_CYCLES(cpunum, target - cpu[cpunum].localtime);
			log_cb(RETRO_LOG_DEBUG, LOGPRE "  cpu %d: %d cycles\n", cpunum, cycles_running);

			/* run for the requested number of cycles */
			if (cycles_running > 0)
			{
				profiler_mark(PROFILER_CPU1 + cpunum);
				cycles_stolen = 0;
				ran = cpunum_execute(cpunum, cycles_running);
				ran -= cycles_stolen;
				profiler_mark(PROFILER_END);

				/* account for these cycles */
				cpu[cpunum].totalcycles += ran;
				cpu[cpunum].localtime += TIME_IN_CYCLES(ran, cpunum);
				log_cb(RETRO_LOG_DEBUG, LOGPRE "         %d ran, %d total, time = %.9f\n", ran, (INT32)cpu[cpunum].totalcycles, cpu[cpunum].localtime);

				/* if the new local CPU time is less than our target, move the target up */
				if (cpu[cpunum].localtime < target && cpu[cpunum].localtime > 0)
				{
					target = cpu[cpunum].localtime;
					log_cb(RETRO_LOG_DEBUG, LOGPRE "         (new target)\n");
				}
			}
		}
	}

	/* update the local times of all CPUs */
	for (cpunum = 0; Machine->drv->cpu[cpunum].cpu_type != CPU_DUMMY; cpunum++)
	{
		/* if we're suspended and counting, process */
		if (cpu[cpunum].suspend && cpu[cpunum].eatcycles && cpu[cpunum].localtime < target)
		{
			/* compute how long to run */
			cycles_running = TIME_TO_CYCLES(cpunum, target - cpu[cpunum].localtime);
			log_cb(RETRO_LOG_DEBUG, LOGPRE "  cpu %d: %d cycles (suspended)\n", cpunum, cycles_running);

			cpu[cpunum].totalcycles += cycles_running;
			cpu[cpunum].localtime += TIME_IN_CYCLES(cycles_running, cpunum);
			log_cb(RETRO_LOG_DEBUG, LOGPRE "         %d skipped, %d total, time = %.9f\n", cycles_running, (INT32)cpu[cpunum].totalcycles, cpu[cpunum].localtime);
		}

		/* update the suspend state */
		if (cpu[cpunum].suspend != cpu[cpunum].nextsuspend)
			log_cb(RETRO_LOG_DEBUG, LOGPRE "--> updated CPU%d suspend from %X to %X\n", cpunum, cpu[cpunum].suspend, cpu[cpunum].nextsuspend);
		cpu[cpunum].suspend = cpu[cpunum].nextsuspend;
		cpu[cpunum].eatcycles = cpu[cpunum].nexteatcycles;

		/* adjust to be relative to the global time */
		cpu[cpunum].localtime -= target;
	}

	/* update the global time */
	timer_adjust_global_time(target);

	/* huh? something for the debugger */
	#ifdef MAME_DEBUG
	{
		extern int debug_key_delay;
		debug_key_delay = 0x7ffe;
	}
	#endif
}



/*************************************
 *
 *	Abort the timeslice for the
 *	active CPU
 *
 *************************************/

void activecpu_abort_timeslice(void)
{
	int current_icount;

	VERIFY_EXECUTINGCPU_VOID(activecpu_abort_timeslice);
	log_cb(RETRO_LOG_DEBUG, LOGPRE "activecpu_abort_timeslice (CPU=%d, cycles_left=%d)\n", cpu_getexecutingcpu(), activecpu_get_icount() + 1);

	/* swallow the remaining cycles */
	current_icount = activecpu_get_icount() + 1;
	cycles_stolen += current_icount;
	cycles_running -= current_icount;
	activecpu_adjust_icount(-current_icount);
}



/*************************************
 *
 *	Return the current local time for
 *	a CPU, relative to the current
 *	timeslice
 *
 *************************************/

double cpunum_get_localtime(int cpunum)
{
	double result;

	/* There must be a cpu active to get local time */
	if (!cpu_gettotalcpu()) return 0;

	VERIFY_CPUNUM(0, cpunum_get_localtime);

	/* if we're active, add in the time from the current slice */
	result = cpu[cpunum].localtime;
	if (cpunum == cpu_getexecutingcpu())
	{
		int cycles = cycles_currently_ran();
		result += TIME_IN_CYCLES(cycles, cpunum);
	}
	return result;
}



/*************************************
 *
 *	Set a suspend reason for the
 *	given CPU
 *
 *************************************/

void cpunum_suspend(int cpunum, int reason, int eatcycles)
{
	VERIFY_CPUNUM_VOID(cpunum_suspend);
	log_cb(RETRO_LOG_DEBUG, LOGPRE "cpunum_suspend (CPU=%d, r=%X, eat=%d)\n", cpunum, reason, eatcycles);

	/* set the pending suspend bits, and force a resync */
	cpu[cpunum].nextsuspend |= reason;
	cpu[cpunum].nexteatcycles = eatcycles;
	if (cpu_getexecutingcpu() >= 0)
		activecpu_abort_timeslice();
}



/*************************************
 *
 *	Clear a suspend reason for a
 *	given CPU
 *
 *************************************/

void cpunum_resume(int cpunum, int reason)
{
	VERIFY_CPUNUM_VOID(cpunum_resume);
	log_cb(RETRO_LOG_DEBUG, LOGPRE "cpunum_resume (CPU=%d, r=%X)\n", cpunum, reason);

	/* clear the pending suspend bits, and force a resync */
	cpu[cpunum].nextsuspend &= ~reason;
	if (cpu_getexecutingcpu() >= 0)
		activecpu_abort_timeslice();
}



/*************************************
 *
 *	Return true if a given CPU is
 *	suspended
 *
 *************************************/

int cpunum_is_suspended(int cpunum, int reason)
{
	VERIFY_CPUNUM(0, cpunum_suspend);
	return ((cpu[cpunum].nextsuspend & reason) != 0);
}



/*************************************
 *
 *	Returns the current scaling factor
 *	for a CPU's clock speed
 *
 *************************************/

double cpunum_get_clockscale(int cpunum)
{
	VERIFY_CPUNUM(1.0, cpunum_get_clockscale);
	return cpu[cpunum].clockscale;
}



/*************************************
 *
 *	Sets the current scaling factor
 *	for a CPU's clock speed
 *
 *************************************/

void cpunum_set_clockscale(int cpunum, double clockscale)
{
	VERIFY_CPUNUM_VOID(cpunum_set_clockscale);

	cpu[cpunum].clockscale = clockscale;
	sec_to_cycles[cpunum] = cpu[cpunum].clockscale * Machine->drv->cpu[cpunum].cpu_clock;
	cycles_to_sec[cpunum] = 1.0 / sec_to_cycles[cpunum];

	/* re-compute the perfect interleave factor */
	compute_perfect_interleave();
}



/*************************************
 *
 *	Temporarily boosts the interleave
 *	factor
 *
 *************************************/

void cpu_boost_interleave(double timeslice_time, double boost_duration)
{
	/* if you pass 0 for the timeslice_time, it means pick something reasonable */
	if (timeslice_time < perfect_interleave)
		timeslice_time = perfect_interleave;

	log_cb(RETRO_LOG_DEBUG, LOGPRE "cpu_boost_interleave(%.9f, %.9f)\n", timeslice_time, boost_duration);

	/* adjust the interleave timer */
	timer_adjust(interleave_boost_timer, timeslice_time, 0, timeslice_time);

	/* adjust the end timer */
	timer_adjust(interleave_boost_timer_end, boost_duration, 0, TIME_NEVER);
}



#if 0
#pragma mark -
#pragma mark TIMING HELPERS
#endif

/*************************************
 *
 *	Return cycles ran this iteration
 *
 *************************************/

int cycles_currently_ran(void)
{
	VERIFY_EXECUTINGCPU(0, cycles_currently_ran);
	return cycles_running - activecpu_get_icount();
}



/*************************************
 *
 *	Return cycles remaining in this
 *	iteration
 *
 *************************************/

int cycles_left_to_run(void)
{
	VERIFY_EXECUTINGCPU(0, cycles_left_to_run);
	return activecpu_get_icount();
}



/*************************************
 *
 *	Return total number of CPU cycles
 *	for the active CPU or for a given CPU.
 *
 *************************************/

/*--------------------------------------------------------------

	IMPORTANT: this value wraps around in a relatively short
	time. For example, for a 6MHz CPU, it will wrap around in
	2^32/6000000 = 716 seconds = 12 minutes.

	Make sure you don't do comparisons between values returned
	by this function, but only use the difference (which will
	be correct regardless of wraparound).

	Alternatively, use the new 64-bit variants instead.

--------------------------------------------------------------*/

UINT32 activecpu_gettotalcycles(void)
{
	VERIFY_EXECUTINGCPU(0, cpu_gettotalcycles);
	return cpu[activecpu].totalcycles + cycles_currently_ran();
}

UINT32 cpu_gettotalcycles(int cpunum)
{
	VERIFY_CPUNUM(0, cpu_gettotalcycles);
	if (cpunum == cpu_getexecutingcpu())
		return cpu[cpunum].totalcycles + cycles_currently_ran();
	else
		return cpu[cpunum].totalcycles;
}


UINT64 activecpu_gettotalcycles64(void)
{
	VERIFY_EXECUTINGCPU(0, cpu_gettotalcycles);
	return cpu[activecpu].totalcycles + cycles_currently_ran();
}

UINT64 cpu_gettotalcycles64(int cpunum)
{
	VERIFY_CPUNUM(0, cpu_gettotalcycles);
	if (cpunum == cpu_getexecutingcpu())
		return cpu[cpunum].totalcycles + cycles_currently_ran();
	else
		return cpu[cpunum].totalcycles;
}



/*************************************
 *
 *	Return cycles until next interrupt
 *	handler call
 *
 *************************************/

int activecpu_geticount(void)
{
	int result;

/* remove me - only used by mamedbg, m92 */
	VERIFY_EXECUTINGCPU(0, cpu_geticount);
	result = TIME_TO_CYCLES(activecpu, cpu[activecpu].vblankint_period - timer_timeelapsed(cpu[activecpu].vblankint_timer));
	return (result < 0) ? 0 : result;
}



/*************************************
 *
 *	Safely eats cycles so we don't
 *	cross a timeslice boundary
 *
 *************************************/

void activecpu_eat_cycles(int cycles)
{
	int cyclesleft = activecpu_get_icount();
	if (cycles > cyclesleft)
		cycles = cyclesleft;
	activecpu_adjust_icount(-cycles);
}



/*************************************
 *
 *	Scales a given value by the fraction
 *	of time elapsed between refreshes
 *
 *************************************/

int cpu_scalebyfcount(int value)
{
	int result = (int)((double)value * timer_timeelapsed(refresh_timer) * refresh_period_inv);
	if (value >= 0)
		return (result < value) ? result : value;
	else
		return (result > value) ? result : value;
}



#if 0
#pragma mark -
#pragma mark VIDEO TIMING
#endif

/*************************************
 *
 *	Creates the refresh timer
 *
 *************************************/

void cpu_init_refresh_timer(void)
{
	/* allocate an infinite timer to track elapsed time since the last refresh */
	refresh_period = TIME_IN_HZ(Machine->drv->frames_per_second);
	refresh_period_inv = 1.0 / refresh_period;
	refresh_timer = timer_alloc(NULL);

	/* while we're at it, compute the scanline times */
	cpu_compute_scanline_timing();
}



/*************************************
 *
 *	Computes the scanline timing
 *
 *************************************/

void cpu_compute_scanline_timing(void)
{
	if (Machine->drv->vblank_duration)
		scanline_period = (refresh_period - TIME_IN_USEC(Machine->drv->vblank_duration)) /
				(double)(Machine->drv->default_visible_area.max_y - Machine->drv->default_visible_area.min_y + 1);
	else
		scanline_period = refresh_period / (double)Machine->drv->screen_height;
	scanline_period_inv = 1.0 / scanline_period;
}



/*************************************
 *
 *	Returns the current scanline
 *
 *************************************/

/*--------------------------------------------------------------

	Note: cpu_getscanline() counts from 0, 0 being the first
	visible line. You might have to adjust this value to match
	the hardware, since in many cases the first visible line
	is >0.

--------------------------------------------------------------*/

int cpu_getscanline(void)
{
	double result = floor(timer_timeelapsed(refresh_timer) * scanline_period_inv);
	return (int)result;
}



/*************************************
 *
 *	Returns time until given scanline
 *
 *************************************/

double cpu_getscanlinetime(int scanline)
{
	double scantime = timer_starttime(refresh_timer) + (double)scanline * scanline_period;
	double abstime = timer_get_time();
	double result;

	/* if we're already past the computed time, count it for the next frame */
	if (abstime >= scantime)
		scantime += TIME_IN_HZ(Machine->drv->frames_per_second);

	/* compute how long from now until that time */
	result = scantime - abstime;

	/* if it's small, just count a whole frame */
	if (result < TIME_IN_NSEC(1))
		result += TIME_IN_HZ(Machine->drv->frames_per_second);
	return result;
}



/*************************************
 *
 *	Returns time for one scanline
 *
 *************************************/

double cpu_getscanlineperiod(void)
{
	return scanline_period;
}



/*************************************
 *
 *	Returns a crude approximation
 *	of the horizontal position of the
 *	bream
 *
 *************************************/

int cpu_gethorzbeampos(void)
{
	double elapsed_time = timer_timeelapsed(refresh_timer);
	int scanline = (int)(elapsed_time * scanline_period_inv);
	double time_since_scanline = elapsed_time - (double)scanline * scanline_period;
	return (int)(time_since_scanline * scanline_period_inv * (double)Machine->drv->screen_width);
}



/*************************************
 *
 *	Returns the VBLANK state
 *
 *************************************/

int cpu_getvblank(void)
{
	return vblank;
}



/*************************************
 *
 *	Returns the current frame count
 *
 *************************************/

int cpu_getcurrentframe(void)
{
	return current_frame;
}



#if 0
#pragma mark -
#pragma mark SYNCHRONIZATION
#endif

/*************************************
 *
 *	Generate a specific trigger
 *
 *************************************/

void cpu_trigger(int trigger)
{
	int cpunum;

	/* cause an immediate resynchronization */
	if (cpu_getexecutingcpu() >= 0)
		activecpu_abort_timeslice();

	/* look for suspended CPUs waiting for this trigger and unsuspend them */
	for (cpunum = 0; cpunum < MAX_CPU; cpunum++)
	{
		/* if this is a dummy, stop looking */
		if (Machine->drv->cpu[cpunum].cpu_type == CPU_DUMMY)
			break;

		/* see if this is a matching trigger */
		if (cpu[cpunum].suspend && cpu[cpunum].trigger == trigger)
		{
			cpunum_resume(cpunum, SUSPEND_REASON_TRIGGER);
			cpu[cpunum].trigger = 0;
		}
	}
}



/*************************************
 *
 *	Generate a trigger in the future
 *
 *************************************/

void cpu_triggertime(double duration, int trigger)
{
	timer_set(duration, trigger, cpu_trigger);
}



/*************************************
 *
 *	Generate a trigger for an int
 *
 *************************************/

void cpu_triggerint(int cpunum)
{
	cpu_trigger(TRIGGER_INT + cpunum);
}



/*************************************
 *
 *	Burn/yield CPU cycles until a trigger
 *
 *************************************/

void cpu_spinuntil_trigger(int trigger)
{
	int cpunum = cpu_getexecutingcpu();

	VERIFY_EXECUTINGCPU_VOID(cpu_spinuntil_trigger);

	/* suspend the CPU immediately if it's not already */
	cpunum_suspend(cpunum, SUSPEND_REASON_TRIGGER, 1);

	/* set the trigger */
	cpu[cpunum].trigger = trigger;
}


void cpu_yielduntil_trigger(int trigger)
{
	int cpunum = cpu_getexecutingcpu();

	VERIFY_EXECUTINGCPU_VOID(cpu_yielduntil_trigger);

	/* suspend the CPU immediately if it's not already */
	cpunum_suspend(cpunum, SUSPEND_REASON_TRIGGER, 0);

	/* set the trigger */
	cpu[cpunum].trigger = trigger;
}



/*************************************
 *
 *	Burn/yield CPU cycles until an
 *	interrupt
 *
 *************************************/

void cpu_spinuntil_int(void)
{
	VERIFY_EXECUTINGCPU_VOID(cpu_spinuntil_int);
	cpu_spinuntil_trigger(TRIGGER_INT + activecpu);
}


void cpu_yielduntil_int(void)
{
	VERIFY_EXECUTINGCPU_VOID(cpu_yielduntil_int);
	cpu_yielduntil_trigger(TRIGGER_INT + activecpu);
}



/*************************************
 *
 *	Burn/yield CPU cycles until the
 *	end of the current timeslice
 *
 *************************************/

void cpu_spin(void)
{
	cpu_spinuntil_trigger(TRIGGER_TIMESLICE);
}


void cpu_yield(void)
{
	cpu_yielduntil_trigger(TRIGGER_TIMESLICE);
}



/*************************************
 *
 *	Burn/yield CPU cycles for a
 *	specific period of time
 *
 *************************************/

void cpu_spinuntil_time(double duration)
{
	static int timetrig = 0;

	cpu_spinuntil_trigger(TRIGGER_SUSPENDTIME + timetrig);
	cpu_triggertime(duration, TRIGGER_SUSPENDTIME + timetrig);
	timetrig = (timetrig + 1) & 255;
}


void cpu_yielduntil_time(double duration)
{
	static int timetrig = 0;

	cpu_yielduntil_trigger(TRIGGER_YIELDTIME + timetrig);
	cpu_triggertime(duration, TRIGGER_YIELDTIME + timetrig);
	timetrig = (timetrig + 1) & 255;
}



#if 0
#pragma mark -
#pragma mark CORE TIMING
#endif

/*************************************
 *
 *	Returns the number of times the
 *	interrupt handler will be called
 *	before the end of the current
 *	video frame.
 *
 *************************************/

/*--------------------------------------------------------------

	This can be useful to interrupt handlers to synchronize
	their operation. If you call this from outside an interrupt
	handler, add 1 to the result, i.e. if it returns 0, it means
	that the interrupt handler will be called once.

--------------------------------------------------------------*/

int cpu_getiloops(void)
{
	VERIFY_ACTIVECPU(0, cpu_getiloops);
	return cpu[activecpu].iloops;
}



/*************************************
 *
 *	Hook for updating things on the
 *	real VBLANK (once per frame)
 *
 *************************************/

static void cpu_vblankreset(void)
{
	int cpunum;

	/* read hi scores from disk */
	hs_update();

	/* read keyboard & update the status of the input ports */
	update_input_ports();

	/* reset the cycle counters */
	for (cpunum = 0; cpunum < cpu_gettotalcpu(); cpunum++)
	{
		if (!(cpu[cpunum].suspend & SUSPEND_REASON_DISABLE))
			cpu[cpunum].iloops = Machine->drv->cpu[cpunum].vblank_interrupts_per_frame - 1;
		else
			cpu[cpunum].iloops = -1;
	}
}



/*************************************
 *
 *	First-run callback for VBLANKs
 *
 *************************************/

static void cpu_firstvblankcallback(int param)
{
	/* now that we're synced up, pulse from here on out */
	timer_adjust(vblank_timer, vblank_period, param, vblank_period);

	/* but we need to call the standard routine as well */
	cpu_vblankcallback(param);
}



/*************************************
 *
 *	VBLANK core handler
 *
 *************************************/

static void cpu_vblankcallback(int param)
{
	int cpunum;

   if (vblank_countdown == 1)
      vblank = 1;

	/* loop over CPUs */
	for (cpunum = 0; cpunum < cpu_gettotalcpu(); cpunum++)
	{
		/* if the interrupt multiplier is valid */
		if (cpu[cpunum].vblankint_multiplier != -1)
		{
			/* decrement; if we hit zero, generate the interrupt and reset the countdown */
			if (!--cpu[cpunum].vblankint_countdown)
			{
				/* a param of -1 means don't call any callbacks */
				if (param != -1)
				{
					/* if the CPU has a VBLANK handler, call it */
					if (Machine->drv->cpu[cpunum].vblank_interrupt && cpu_getstatus(cpunum))
					{
						cpuintrf_push_context(cpunum);
						(*Machine->drv->cpu[cpunum].vblank_interrupt)();
						cpuintrf_pop_context();
					}

					/* update the counters */
					cpu[cpunum].iloops--;
				}

				/* reset the countdown and timer */
				cpu[cpunum].vblankint_countdown = cpu[cpunum].vblankint_multiplier;
				timer_adjust(cpu[cpunum].vblankint_timer, TIME_NEVER, 0, 0);
			}
		}

		/* else reset the VBLANK timer if this is going to be a real VBLANK */
		else if (vblank_countdown == 1)
			timer_adjust(cpu[cpunum].vblankint_timer, TIME_NEVER, 0, 0);
	}

	/* is it a real VBLANK? */
	if (!--vblank_countdown)
	{
		/* do we update the screen now? */
		if (!(Machine->drv->video_attributes & VIDEO_UPDATE_AFTER_VBLANK))
			time_to_quit = updatescreen();

		/* Set the timer to update the screen */
		timer_set(TIME_IN_USEC(Machine->drv->vblank_duration), 0, cpu_updatecallback);

		/* reset the globals */
		cpu_vblankreset();

		/* reset the counter */
		vblank_countdown = vblank_multiplier;
	}
}



/*************************************
 *
 *	End-of-VBLANK callback
 *
 *************************************/

static void cpu_updatecallback(int param)
{
	/* update the screen if we didn't before */
	if (Machine->drv->video_attributes & VIDEO_UPDATE_AFTER_VBLANK)
		time_to_quit = updatescreen();
	vblank = 0;

	/* update IPT_VBLANK input ports */
	inputport_vblank_end();

	/* reset partial updating */
	reset_partial_updates();

	/* check the watchdog */
	if (watchdog_counter > 0)
		if (--watchdog_counter == 0)
		{
			log_cb(RETRO_LOG_INFO, "reset caused by the watchdog\n");
			machine_reset();
		}

	/* track total frames */
	current_frame++;

	/* reset the refresh timer */
	timer_adjust(refresh_timer, TIME_NEVER, 0, 0);
}



/*************************************
 *
 *	Callback for timed interrupts
 *	(not tied to a VBLANK)
 *
 *************************************/

static void cpu_timedintcallback(int param)
{
	/* bail if there is no routine */
	if (Machine->drv->cpu[param].timed_interrupt && cpu_getstatus(param))
	{
		cpuintrf_push_context(param);
		(*Machine->drv->cpu[param].timed_interrupt)();
		cpuintrf_pop_context();
	}
}



/*************************************
 *
 *	Converts an integral timing rate
 *	into a period
 *
 *************************************/

/*--------------------------------------------------------------

	Rates can be specified as follows:

		rate <= 0		-> 0
		rate < 50000	-> 'rate' cycles per frame
		rate >= 50000	-> 'rate' nanoseconds

--------------------------------------------------------------*/

static double cpu_computerate(int value)
{
	/* values equal to zero are zero */
	if (value <= 0)
		return 0.0;

	/* values above between 0 and 50000 are in Hz */
	if (value < 50000)
		return TIME_IN_HZ(value);

	/* values greater than 50000 are in nanoseconds */
	else
		return TIME_IN_NSEC(value);
}



/*************************************
 *
 *	Callback to force a timeslice
 *
 *************************************/

static void cpu_timeslicecallback(int param)
{
	cpu_trigger(TRIGGER_TIMESLICE);
}



/*************************************
 *
 *	Callback to end a temporary
 *	interleave boost
 *
 *************************************/

static void end_interleave_boost(int param)
{
	timer_adjust(interleave_boost_timer, TIME_NEVER, 0, TIME_NEVER);
	log_cb(RETRO_LOG_DEBUG, LOGPRE "end_interleave_boost\n");
}



/*************************************
 *
 *	Compute the "perfect" interleave
 *	interval
 *
 *************************************/

static void compute_perfect_interleave(void)
{
	double smallest = cycles_to_sec[0];
	int cpunum;

	/* start with a huge time factor and find the 2nd smallest cycle time */
	perfect_interleave = 1.0;
	for (cpunum = 1; Machine->drv->cpu[cpunum].cpu_type != CPU_DUMMY; cpunum++)
	{
		/* find the 2nd smallest cycle interval */
		if (cycles_to_sec[cpunum] < smallest)
		{
			perfect_interleave = smallest;
			smallest = cycles_to_sec[cpunum];
		}
		else if (cycles_to_sec[cpunum] < perfect_interleave)
			perfect_interleave = cycles_to_sec[cpunum];
	}

	/* adjust the final value */
	if (perfect_interleave == 1.0)
		perfect_interleave = cycles_to_sec[0];

	log_cb(RETRO_LOG_DEBUG, LOGPRE "Perfect interleave = %.9f, smallest = %.9f\n", perfect_interleave, smallest);
}



/*************************************
 *
 *	Setup all the core timers
 *
 *************************************/

static void cpu_inittimers(void)
{
	double first_time;
	int cpunum, max, ipf;

	/* allocate a dummy timer at the minimum frequency to break things up */
	ipf = Machine->drv->cpu_slices_per_frame;
	if (ipf <= 0)
		ipf = 1;
	timeslice_period = TIME_IN_HZ(Machine->drv->frames_per_second * ipf);
	timeslice_timer = timer_alloc(cpu_timeslicecallback);
	timer_adjust(timeslice_timer, timeslice_period, 0, timeslice_period);

	/* allocate timers to handle interleave boosts */
	interleave_boost_timer = timer_alloc(NULL);
	interleave_boost_timer_end = timer_alloc(end_interleave_boost);

	/*
	 *	The following code finds all the CPUs that are interrupting in sync with the VBLANK
	 *	and sets up the VBLANK timer to run at the minimum number of cycles per frame in
	 *	order to service all the synced interrupts
	 */

	/* find the CPU with the maximum interrupts per frame */
	max = 1;
	for (cpunum = 0; cpunum < cpu_gettotalcpu(); cpunum++)
	{
		ipf = Machine->drv->cpu[cpunum].vblank_interrupts_per_frame;
		if (ipf > max)
			max = ipf;
	}

	/* now find the LCD with the rest of the CPUs (brute force - these numbers aren't huge) */
	vblank_multiplier = max;
	while (1)
	{
		for (cpunum = 0; cpunum < cpu_gettotalcpu(); cpunum++)
		{
			ipf = Machine->drv->cpu[cpunum].vblank_interrupts_per_frame;
			if (ipf > 0 && (vblank_multiplier % ipf) != 0)
				break;
		}
		if (cpunum == cpu_gettotalcpu())
			break;
		vblank_multiplier += max;
	}

	/* initialize the countdown timers and intervals */
	for (cpunum = 0; cpunum < cpu_gettotalcpu(); cpunum++)
	{
		ipf = Machine->drv->cpu[cpunum].vblank_interrupts_per_frame;
		if (ipf > 0)
			cpu[cpunum].vblankint_countdown = cpu[cpunum].vblankint_multiplier = vblank_multiplier / ipf;
		else
			cpu[cpunum].vblankint_countdown = cpu[cpunum].vblankint_multiplier = -1;
	}

	/* allocate a vblank timer at the frame rate * the LCD number of interrupts per frame */
	vblank_period = TIME_IN_HZ(Machine->drv->frames_per_second * vblank_multiplier);
	vblank_timer = timer_alloc(cpu_vblankcallback);
	vblank_countdown = vblank_multiplier;

	/*
	 *		The following code creates individual timers for each CPU whose interrupts are not
	 *		synced to the VBLANK, and computes the typical number of cycles per interrupt
	 */

	/* start the CPU interrupt timers */
	for (cpunum = 0; cpunum < cpu_gettotalcpu(); cpunum++)
	{
		ipf = Machine->drv->cpu[cpunum].vblank_interrupts_per_frame;

		/* compute the average number of cycles per interrupt */
		if (ipf <= 0)
			ipf = 1;
		cpu[cpunum].vblankint_period = TIME_IN_HZ(Machine->drv->frames_per_second * ipf);
		cpu[cpunum].vblankint_timer = timer_alloc(NULL);

		/* see if we need to allocate a CPU timer */
		ipf = Machine->drv->cpu[cpunum].timed_interrupts_per_second;
		if (ipf)
		{
			cpu[cpunum].timedint_period = cpu_computerate(ipf);
			cpu[cpunum].timedint_timer = timer_alloc(cpu_timedintcallback);
			timer_adjust(cpu[cpunum].timedint_timer, cpu[cpunum].timedint_period, cpunum, cpu[cpunum].timedint_period);
		}
	}

	/* note that since we start the first frame on the refresh, we can't pulse starting
	   immediately; instead, we back up one VBLANK period, and inch forward until we hit
	   positive time. That time will be the time of the first VBLANK timer callback */
	first_time = -TIME_IN_USEC(Machine->drv->vblank_duration) + vblank_period;
	while (first_time < 0)
	{
		cpu_vblankcallback(-1);
		first_time += vblank_period;
	}
	timer_set(first_time, 0, cpu_firstvblankcallback);
}
