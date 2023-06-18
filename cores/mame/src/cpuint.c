/***************************************************************************

	cpuint.c

	Core multi-CPU interrupt engine.

***************************************************************************/

#include "driver.h"
#include "timer.h"
#include "state.h"
#include "mamedbg.h"
#include "cpuint.h"

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
		return retval;										\
	}

#define VERIFY_ACTIVECPU_VOID(name)							\
	int activecpu = cpu_getactivecpu();						\
	if (activecpu < 0)										\
	{														\
		return;												\
	}



/*************************************
 *
 *	CPU interrupt variables
 *
 *************************************/

/* current states for each CPU */
static UINT8 interrupt_enable[MAX_CPU];
static INT32 interrupt_vector[MAX_CPU][MAX_IRQ_LINES];

/* deferred states written in callbacks */
static UINT8 irq_line_state[MAX_CPU][MAX_IRQ_LINES];
static INT32 irq_line_vector[MAX_CPU][MAX_IRQ_LINES];

/* ick, interrupt event queues */
#define MAX_IRQ_EVENTS		256
static INT32 irq_event_queue[MAX_CPU][MAX_IRQ_EVENTS];
static int irq_event_index[MAX_CPU];



/*************************************
 *
 *	IRQ acknowledge callbacks
 *
 *************************************/

static int cpu_0_irq_callback(int irqline);
static int cpu_1_irq_callback(int irqline);
static int cpu_2_irq_callback(int irqline);
static int cpu_3_irq_callback(int irqline);
static int cpu_4_irq_callback(int irqline);
static int cpu_5_irq_callback(int irqline);
static int cpu_6_irq_callback(int irqline);
static int cpu_7_irq_callback(int irqline);

int (*cpu_irq_callbacks[MAX_CPU])(int) =
{
	cpu_0_irq_callback,
	cpu_1_irq_callback,
	cpu_2_irq_callback,
	cpu_3_irq_callback,
	cpu_4_irq_callback,
	cpu_5_irq_callback,
	cpu_6_irq_callback,
	cpu_7_irq_callback
};

static int (*drv_irq_callbacks[MAX_CPU])(int);



#if 0
#pragma mark CORE CPU
#endif

/*************************************
 *
 *	Initialize a CPU's interrupt states
 *
 *************************************/

int cpuint_init(void)
{
	int cpunum;
	int irqline;

	/* loop over all CPUs */
	for (cpunum = 0; cpunum < cpu_gettotalcpu(); cpunum++)
	{
		/* reset the IRQ lines */
		for (irqline = 0; irqline < MAX_IRQ_LINES; irqline++)
		{
			irq_line_state[cpunum][irqline] = CLEAR_LINE;
			interrupt_vector[cpunum][irqline] =
			irq_line_vector[cpunum][irqline] = cpunum_default_irq_vector(cpunum);
		}
		
		/* reset the IRQ event queues */
		irq_event_index[cpunum] = 0;
	}

	/* set up some stuff to save */
	state_save_set_current_tag(0);
	state_save_register_UINT8("cpu", 0, "irq enable",     interrupt_enable,  cpu_gettotalcpu());
	state_save_register_INT32("cpu", 0, "irq vector",     &interrupt_vector[0][0],cpu_gettotalcpu() * MAX_IRQ_LINES);
	state_save_register_UINT8("cpu", 0, "irqline state",  &irq_line_state[0][0],  cpu_gettotalcpu() * MAX_IRQ_LINES);
	state_save_register_INT32("cpu", 0, "irqline vector", &irq_line_vector[0][0], cpu_gettotalcpu() * MAX_IRQ_LINES);

	return 0;
}



/*************************************
 *
 *	Reset a CPU's interrupt states
 *
 *************************************/

void cpuint_reset_cpu(int cpunum)
{
	int irqline;

	/* start with interrupts enabled, so the generic routine will work even if */
	/* the machine doesn't have an interrupt enable port */
	interrupt_enable[cpunum] = 1;
	for (irqline = 0; irqline < MAX_IRQ_LINES; irqline++)
	{
		interrupt_vector[cpunum][irqline] = cpunum_default_irq_vector(cpunum);
		irq_event_index[cpunum] = 0;
	}

	/* reset any driver hooks into the IRQ acknowledge callbacks */
	drv_irq_callbacks[cpunum] = NULL;
}



#if 0
#pragma mark -
#pragma mark INTERRUPT HANDLING
#endif

/*************************************
 *
 *	Set IRQ callback for drivers
 *
 *************************************/

void cpu_set_irq_callback(int cpunum, int (*callback)(int))
{
	drv_irq_callbacks[cpunum] = callback;
}



/*************************************
 *
 *	Internal IRQ callbacks
 *
 *************************************/

static INLINE int cpu_irq_callback(int cpunum, int irqline)
{
	int vector = irq_line_vector[cpunum][irqline];

	/*LOG(("cpu_%d_irq_callback(%d) $%04x\n", cpunum, irqline, vector));*/

	/* if the IRQ state is HOLD_LINE, clear it */
	if (irq_line_state[cpunum][irqline] == HOLD_LINE)
	{
		/*LOG(("->set_irq_line(%d,%d,%d)\n", cpunum, irqline, CLEAR_LINE));*/
		activecpu_set_irq_line(irqline, INTERNAL_CLEAR_LINE);
		irq_line_state[cpunum][irqline] = CLEAR_LINE;
	}

	/* if there's a driver callback, run it */
	if (drv_irq_callbacks[cpunum])
		vector = (*drv_irq_callbacks[cpunum])(irqline);

	/* otherwise, just return the current vector */
	return vector;
}

static int cpu_0_irq_callback(int irqline) { return cpu_irq_callback(0, irqline); }
static int cpu_1_irq_callback(int irqline) { return cpu_irq_callback(1, irqline); }
static int cpu_2_irq_callback(int irqline) { return cpu_irq_callback(2, irqline); }
static int cpu_3_irq_callback(int irqline) { return cpu_irq_callback(3, irqline); }
static int cpu_4_irq_callback(int irqline) { return cpu_irq_callback(4, irqline); }
static int cpu_5_irq_callback(int irqline) { return cpu_irq_callback(5, irqline); }
static int cpu_6_irq_callback(int irqline) { return cpu_irq_callback(6, irqline); }
static int cpu_7_irq_callback(int irqline) { return cpu_irq_callback(7, irqline); }



/*************************************
 *
 *	Set the IRQ vector for a given
 *	IRQ line on a CPU
 *
 *************************************/

void cpu_irq_line_vector_w(int cpunum, int irqline, int vector)
{
	if (cpunum < cpu_gettotalcpu() && irqline >= 0 && irqline < MAX_IRQ_LINES)
	{
		/*LOG(("cpu_irq_line_vector_w(%d,%d,$%04x)\n",cpunum,irqline,vector));*/
		interrupt_vector[cpunum][irqline] = vector;
		return;
	}
	/*LOG(("cpu_irq_line_vector_w CPU#%d irqline %d > max irq lines\n", cpunum, irqline));*/
}



/*************************************
 *
 *	Generate a IRQ interrupt
 *
 *************************************/

static void cpu_empty_event_queue(int cpunum)
{
	int i;

	/* swap to the CPU's context */
	cpuintrf_push_context(cpunum);

	/* loop over all events */
	for (i = 0; i < irq_event_index[cpunum]; i++)
	{
		INT32 irq_event = irq_event_queue[cpunum][i];
		int state = irq_event & 0xff;
		int irqline = (irq_event >> 8) & 0xff;
		int vector = irq_event >> 16;

		/*LOG(("cpu_empty_event_queue %d,%d,%d\n",cpunum,irqline,state));*/

	/* set the IRQ line state and vector */
	if (irqline >= 0 && irqline < MAX_IRQ_LINES)
	{
		irq_line_state[cpunum][irqline] = state;
			irq_line_vector[cpunum][irqline] = vector;
	}

	/* switch off the requested state */
	switch (state)
	{
		case PULSE_LINE:
			activecpu_set_irq_line(irqline, INTERNAL_ASSERT_LINE);
			activecpu_set_irq_line(irqline, INTERNAL_CLEAR_LINE);
			break;

		case HOLD_LINE:
		case ASSERT_LINE:
			activecpu_set_irq_line(irqline, INTERNAL_ASSERT_LINE);
			break;

		case CLEAR_LINE:
			activecpu_set_irq_line(irqline, INTERNAL_CLEAR_LINE);
			break;
	}

	/* generate a trigger to unsuspend any CPUs waiting on the interrupt */
	if (state != CLEAR_LINE)
		cpu_triggerint(cpunum);
}

	/* swap back */
	cpuintrf_pop_context();

	/* reset counter */
	irq_event_index[cpunum] = 0;
}

	
void cpu_set_irq_line(int cpunum, int irqline, int state)
{
	int vector = (irqline >= 0 && irqline < MAX_IRQ_LINES) ? interrupt_vector[cpunum][irqline] : 0xff;
	cpu_set_irq_line_and_vector(cpunum, irqline, state, vector);
}


void cpu_set_irq_line_and_vector(int cpunum, int irqline, int state, int vector)
{
	INT32 irq_event = (state & 0xff) | ((irqline & 0xff) << 8) | (vector << 16);
	int event_index = irq_event_index[cpunum]++;

	/*LOG(("cpu_set_irq_line(%d,%d,%d,%02x)\n", cpunum, irqline, state, vector));*/

	/* enqueue the event */
	if (event_index < MAX_IRQ_EVENTS)
	{
		irq_event_queue[cpunum][event_index] = irq_event;
		
		/* if this is the first one, set the timer */
		if (event_index == 0)
			timer_set(TIME_NOW, cpunum, cpu_empty_event_queue);
	}
#if 0
	else
		log_cb(RETRO_LOG_ERROR, LOGPRE "Exceeded pending IRQ event queue on CPU %d!\n", cpunum);
#endif
}



#if 0
#pragma mark -
#pragma mark PREFERRED INTERRUPT HANDLING
#endif


/*************************************
 *
 *	NMI interrupt generation
 *
 *************************************/

INTERRUPT_GEN( nmi_line_pulse )
{
	int cpunum = cpu_getactivecpu();
	if (interrupt_enable[cpunum])
		cpu_set_irq_line(cpunum, IRQ_LINE_NMI, PULSE_LINE);
}

INTERRUPT_GEN( nmi_line_assert )
{
	int cpunum = cpu_getactivecpu();
	if (interrupt_enable[cpunum])
		cpu_set_irq_line(cpunum, IRQ_LINE_NMI, ASSERT_LINE);
}



/*************************************
 *
 *	IRQ n interrupt generation
 *
 *************************************/

static INLINE void irqn_line_hold(int irqline)
{
	int cpunum = cpu_getactivecpu();
	if (interrupt_enable[cpunum])
	{
		int vector = (irqline >= 0 && irqline < MAX_IRQ_LINES) ? interrupt_vector[cpunum][irqline] : 0xff;
		cpu_set_irq_line_and_vector(cpunum, irqline, HOLD_LINE, vector);
	}
}

static INLINE void irqn_line_pulse(int irqline)
{
	int cpunum = cpu_getactivecpu();
	if (interrupt_enable[cpunum])
	{
		int vector = (irqline >= 0 && irqline < MAX_IRQ_LINES) ? interrupt_vector[cpunum][irqline] : 0xff;
		cpu_set_irq_line_and_vector(cpunum, irqline, PULSE_LINE, vector);
	}
}

static INLINE void irqn_line_assert(int irqline)
{
	int cpunum = cpu_getactivecpu();
	if (interrupt_enable[cpunum])
	{
		int vector = (irqline >= 0 && irqline < MAX_IRQ_LINES) ? interrupt_vector[cpunum][irqline] : 0xff;
		cpu_set_irq_line_and_vector(cpunum, irqline, ASSERT_LINE, vector);
	}
}



/*************************************
 *
 *	IRQ interrupt generation
 *
 *************************************/

INTERRUPT_GEN( irq0_line_hold )		{ irqn_line_hold(0); }
INTERRUPT_GEN( irq0_line_pulse )	{ irqn_line_pulse(0); }
INTERRUPT_GEN( irq0_line_assert )	{ irqn_line_assert(0); }

INTERRUPT_GEN( irq1_line_hold )		{ irqn_line_hold(1); }
INTERRUPT_GEN( irq1_line_pulse )	{ irqn_line_pulse(1); }
INTERRUPT_GEN( irq1_line_assert )	{ irqn_line_assert(1); }

INTERRUPT_GEN( irq2_line_hold )		{ irqn_line_hold(2); }
INTERRUPT_GEN( irq2_line_pulse )	{ irqn_line_pulse(2); }
INTERRUPT_GEN( irq2_line_assert )	{ irqn_line_assert(2); }

INTERRUPT_GEN( irq3_line_hold )		{ irqn_line_hold(3); }
INTERRUPT_GEN( irq3_line_pulse )	{ irqn_line_pulse(3); }
INTERRUPT_GEN( irq3_line_assert )	{ irqn_line_assert(3); }

INTERRUPT_GEN( irq4_line_hold )		{ irqn_line_hold(4); }
INTERRUPT_GEN( irq4_line_pulse )	{ irqn_line_pulse(4); }
INTERRUPT_GEN( irq4_line_assert )	{ irqn_line_assert(4); }

INTERRUPT_GEN( irq5_line_hold )		{ irqn_line_hold(5); }
INTERRUPT_GEN( irq5_line_pulse )	{ irqn_line_pulse(5); }
INTERRUPT_GEN( irq5_line_assert )	{ irqn_line_assert(5); }

INTERRUPT_GEN( irq6_line_hold )		{ irqn_line_hold(6); }
INTERRUPT_GEN( irq6_line_pulse )	{ irqn_line_pulse(6); }
INTERRUPT_GEN( irq6_line_assert )	{ irqn_line_assert(6); }

INTERRUPT_GEN( irq7_line_hold )		{ irqn_line_hold(7); }
INTERRUPT_GEN( irq7_line_pulse )	{ irqn_line_pulse(7); }
INTERRUPT_GEN( irq7_line_assert )	{ irqn_line_assert(7); }



#if 0
#pragma mark -
#pragma mark OBSOLETE INTERRUPT HANDLING
#endif

/*************************************
 *
 *	Interrupt enabling
 *
 *************************************/

static void cpu_clearintcallback(int cpunum)
{
	int irqcount = cputype_get_interface(Machine->drv->cpu[cpunum].cpu_type)->num_irqs;
	int irqline;

	cpuintrf_push_context(cpunum);

	/* clear NMI and all IRQs */
	activecpu_set_irq_line(IRQ_LINE_NMI, INTERNAL_CLEAR_LINE);
	for (irqline = 0; irqline < irqcount; irqline++)
		activecpu_set_irq_line(irqline, INTERNAL_CLEAR_LINE);

	cpuintrf_pop_context();
}


void cpu_interrupt_enable(int cpunum,int enabled)
{
	interrupt_enable[cpunum] = enabled;

/*LOG(("CPU#%d interrupt_enable=%d\n", cpunum, enabled));*/

	/* make sure there are no queued interrupts */
	if (enabled == 0)
		timer_set(TIME_NOW, cpunum, cpu_clearintcallback);
}


WRITE_HANDLER( interrupt_enable_w )
{
	VERIFY_ACTIVECPU_VOID(interrupt_enable_w);
	cpu_interrupt_enable(activecpu, data);
}


READ_HANDLER( interrupt_enable_r )
{
	VERIFY_ACTIVECPU(1, interrupt_enable_r);
	return interrupt_enable[activecpu];
}


WRITE_HANDLER( interrupt_vector_w )
{
	VERIFY_ACTIVECPU_VOID(interrupt_vector_w);
	if (interrupt_vector[activecpu][0] != data)
	{
		/*LOG(("CPU#%d interrupt_vector_w $%02x\n", activecpu, data));*/
		interrupt_vector[activecpu][0] = data;

		/* make sure there are no queued interrupts */
		timer_set(TIME_NOW, activecpu, cpu_clearintcallback);
	}
}
