/***************************************************************************

	cpuexec.h

	Core multi-CPU execution engine.

***************************************************************************/

#ifndef CPUEXEC_H
#define CPUEXEC_H

#include "osd_cpu.h"
#include "memory.h"
#include "timer.h"
#include "hiscore.h"

#ifdef __cplusplus
extern "C" {
#endif



/*************************************
 *
 *	CPU cycle timing arrays
 *
 *************************************/

extern double cycles_to_sec[];
extern double sec_to_cycles[];



/*************************************
 *
 *	CPU description for drivers
 *
 *************************************/

struct MachineCPU
{
	int			cpu_type;					/* index for the CPU type */
	int			cpu_flags;					/* flags; see #defines below */
	int			cpu_clock;					/* in Hertz */
	const void *memory_read;				/* struct Memory_ReadAddress */
	const void *memory_write;				/* struct Memory_WriteAddress */
	const void *port_read;
	const void *port_write;
	void 		(*vblank_interrupt)(void);	/* for interrupts tied to VBLANK */
	int 		vblank_interrupts_per_frame;/* usually 1 */
	void 		(*timed_interrupt)(void);	/* for interrupts not tied to VBLANK */
	int 		timed_interrupts_per_second;
	void *		reset_param;				/* parameter for cpu_reset */
	const char *tag;
};



/*************************************
 *
 *	CPU flag constants
 *
 *************************************/

enum
{
	/* set this if the CPU is used as a slave for audio. It will not be emulated if */
	/* sound is disabled, therefore speeding up a lot the emulation. */
	CPU_AUDIO_CPU = 0x0002,

	/* the Z80 can be wired to use 16 bit addressing for I/O ports */
	CPU_16BIT_PORT = 0x0001
};




/*************************************
 *
 *	Core CPU execution
 *
 *************************************/

/* Prepare CPUs for execution */
int cpu_init(void);

/* Run CPUs until the user quits */
void cpu_run(void);

/* Clean up after quitting */
void cpu_exit(void);

/* Force a reset after the current timeslice */
void machine_reset(void);

void mame_frame(void);

/*************************************
 *
 *	Optional watchdog
 *
 *************************************/

/* 8-bit watchdog read/write handlers */
WRITE_HANDLER( watchdog_reset_w );
WRITE_HANDLER( watchdog_400_reset_w );
READ_HANDLER( watchdog_reset_r );

/* 16-bit watchdog read/write handlers */
WRITE16_HANDLER( watchdog_reset16_w );
READ16_HANDLER( watchdog_reset16_r );

/* 32-bit watchdog read/write handlers */
WRITE32_HANDLER( watchdog_reset32_w );
READ32_HANDLER( watchdog_reset32_r );



/*************************************
 *
 *	CPU halt/reset lines
 *
 *************************************/

/* Set the logical state (ASSERT_LINE/CLEAR_LINE) of the RESET line on a CPU */
void cpunum_set_reset_line(int cpunum, int state);

/* Set the logical state (ASSERT_LINE/CLEAR_LINE) of the HALT line on a CPU */
void cpunum_set_halt_line(int cpunum, int state);

/* Backwards compatibility */
#define cpu_set_reset_line 		cpunum_set_reset_line
#define cpu_set_halt_line 		cpunum_set_halt_line



/*************************************
 *
 *	CPU scheduling
 *
 *************************************/

/* Suspension reasons */
enum
{
	SUSPEND_REASON_HALT 	= 0x0001,
	SUSPEND_REASON_RESET 	= 0x0002,
	SUSPEND_REASON_SPIN 	= 0x0004,
	SUSPEND_REASON_TRIGGER 	= 0x0008,
	SUSPEND_REASON_DISABLE 	= 0x0010,
	SUSPEND_ANY_REASON 		= ~0
};

/* Suspend the given CPU for a specific reason */
void cpunum_suspend(int cpunum, int reason, int eatcycles);

/* Suspend the given CPU for a specific reason */
void cpunum_resume(int cpunum, int reason);

/* Returns true if the given CPU is suspended for any of the given reasons */
int cpunum_is_suspended(int cpunum, int reason);

/* Aborts the timeslice for the active CPU */
void activecpu_abort_timeslice(void);

/* Returns the current local time for a CPU, relative to the current timeslice */
double cpunum_get_localtime(int cpunum);

/* Returns the current scaling factor for a CPU's clock speed */
double cpunum_get_clockscale(int cpunum);

/* Sets the current scaling factor for a CPU's clock speed */
void cpunum_set_clockscale(int cpunum, double clockscale);

/* Temporarily boosts the interleave factor */
void cpu_boost_interleave(double timeslice_time, double boost_duration);

/* Backwards compatibility */
#define timer_suspendcpu(cpunum, suspend, reason)	do { if (suspend) cpunum_suspend(cpunum, reason, 1); else cpunum_resume(cpunum, reason); } while (0)
#define timer_holdcpu(cpunum, suspend, reason)		do { if (suspend) cpunum_suspend(cpunum, reason, 0); else cpunum_resume(cpunum, reason); } while (0)
#define cpu_getstatus(cpunum)						(!cpunum_is_suspended(cpunum, SUSPEND_REASON_HALT | SUSPEND_REASON_RESET | SUSPEND_REASON_DISABLE))
#define timer_get_overclock(cpunum)					cpunum_get_clockscale(cpunum)
#define timer_set_overclock(cpunum, overclock)		cpunum_set_clockscale(cpunum, overclock)



/*************************************
 *
 *	Timing helpers
 *
 *************************************/

/* Returns the number of cycles run so far this timeslice */
int cycles_currently_ran(void);

/* Returns the number of cycles left to run in this timeslice */
int cycles_left_to_run(void);

/* Returns the total number of CPU cycles */
UINT32 activecpu_gettotalcycles(void);
UINT64 activecpu_gettotalcycles64(void);

/* Returns the total number of CPU cycles for a given CPU */
UINT32 cpunum_gettotalcycles(int cpunum);
UINT64 cpunum_gettotalcycles64(int cpunum);

/* Returns the number of CPU cycles before the next interrupt handler call */
int activecpu_geticount(void);

/* Safely eats cycles so we don't cross a timeslice boundary */
void activecpu_eat_cycles(int cycles);

/* Scales a given value by the ratio of fcount / fperiod */
int cpu_scalebyfcount(int value);

/* Backwards compatibility */
#define cpu_gettotalcycles cpunum_gettotalcycles
#define cpu_gettotalcycles64 cpunum_gettotalcycles64




/*************************************
 *
 *	Video timing
 *
 *************************************/

/* Initialize the refresh timer */
void cpu_init_refresh_timer(void);

/* Recomputes the scanling timing after, e.g., a visible area change */
void cpu_compute_scanline_timing(void);

/* Returns the number of the video frame we are currently playing */
int cpu_getcurrentframe(void);

/* Returns the current scanline number */
int cpu_getscanline(void);

/* Returns the amount of time until a given scanline */
double cpu_getscanlinetime(int scanline);

/* Returns the duration of a single scanline */
double cpu_getscanlineperiod(void);

/* Returns the current horizontal beam position in pixels */
int cpu_gethorzbeampos(void);

/* Returns the current VBLANK state */
int cpu_getvblank(void);

/* Returns the number of the video frame we are currently playing */
int cpu_getcurrentframe(void);



/*************************************
 *
 *	Synchronization
 *
 *************************************/

/* generate a trigger now */
void cpu_trigger(int trigger);

/* generate a trigger after a specific period of time */
void cpu_triggertime(double duration, int trigger);

/* generate a trigger corresponding to an interrupt on the given CPU */
void cpu_triggerint(int cpunum);

/* burn CPU cycles until a timer trigger */
void cpu_spinuntil_trigger(int trigger);

/* yield our timeslice until a timer trigger */
void cpu_yielduntil_trigger(int trigger);

/* burn CPU cycles until the next interrupt */
void cpu_spinuntil_int(void);

/* yield our timeslice until the next interrupt */
void cpu_yielduntil_int(void);

/* burn CPU cycles until our timeslice is up */
void cpu_spin(void);

/* yield our current timeslice */
void cpu_yield(void);

/* burn CPU cycles for a specific period of time */
void cpu_spinuntil_time(double duration);

/* yield our timeslice for a specific period of time */
void cpu_yielduntil_time(double duration);



/*************************************
 *
 *	Core timing
 *
 *************************************/

/* Returns the number of times the interrupt handler will be called before
   the end of the current video frame. This is can be useful to interrupt
   handlers to synchronize their operation. If you call this from outside
   an interrupt handler, add 1 to the result, i.e. if it returns 0, it means
   that the interrupt handler will be called once. */
int cpu_getiloops(void);



/*************************************
 *
 *	Z80 daisy chain
 *
 *************************************/

/* fix me - where should this stuff go? */

/* daisy-chain link */
typedef struct
{
	void (*reset)(int); 			/* reset callback	  */
	int  (*interrupt_entry)(int);	/* entry callback	  */
	void (*interrupt_reti)(int);	/* reti callback	  */
	int irq_param;					/* callback paramater */
} Z80_DaisyChain;

#define Z80_MAXDAISY	4		/* maximum of daisy chan device */

#define Z80_INT_REQ 	0x01	/* interrupt request mask		*/
#define Z80_INT_IEO 	0x02	/* interrupt disable mask(IEO)	*/

#define Z80_VECTOR(device,state) (((device)<<8)|(state))


#ifdef __cplusplus
}
#endif

#endif	/* CPUEXEC_H */
