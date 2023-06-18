#ifndef HEADER__G65816
#define HEADER__G65816

/* ======================================================================== */
/* =============================== COPYRIGHT ============================== */
/* ======================================================================== */
/*

G65C816 CPU Emulator V0.92

Copyright (c) 2000 Karl Stenerud
All rights reserved.

Permission is granted to use this source code for non-commercial purposes.
To use this code for commercial purposes, you must get permission from the
author (Karl Stenerud) at karl@higashiyama-unet.ocn.ne.jp.


*/
/* ======================================================================== */
/* ============================= Configuration ============================ */
/* ======================================================================== */

/* GTE Microcircuits G65816 */

/* ======================================================================== */
/* =============================== DEFINES ================================ */
/* ======================================================================== */

/* Interrupt lines - used with g65816_set_irq_line() */
enum
{
	G65816_LINE_NONE,
	G65816_LINE_IRQ,
	G65816_LINE_NMI,
	G65816_LINE_ABORT,
	G65816_LINE_SO,
	G65816_LINE_RDY,
	G65816_LINE_RESET
};

#define G65816_INT_NONE G65816_LINE_NONE
#define G65816_INT_IRQ G65816_LINE_IRQ
#define G65816_INT_NMI G65816_LINE_NMI


/* Registers - used by g65816_set_reg() and g65816_get_reg() */
enum
{
	G65816_PC=1, G65816_S, G65816_P, G65816_A, G65816_X, G65816_Y,
	G65816_PB, G65816_DB, G65816_D, G65816_E,
	G65816_NMI_STATE, G65816_IRQ_STATE
};



/* ======================================================================== */
/* ============================== PROTOTYPES ============================== */
/* ======================================================================== */

extern int g65816_ICount;				/* cycle count */



/* ======================================================================== */
/* ================================== API ================================= */
/* ======================================================================== */

/* --------------------- */
/* CPU Peek and Poke API */
/* --------------------- */

void g65816_init(void);

/* Get the current CPU context */
unsigned g65816_get_context(void *dst);

/* Set the current CPU context */
void g65816_set_context(void *src);

/* Get the current Program Counter */
unsigned g65816_get_pc(void);

/* Set the current Program Counter */
void g65816_set_pc(unsigned val);

/* Get the current Stack Pointer */
unsigned g65816_get_sp(void);

/* Set the current Stack Pointer */
void g65816_set_sp(unsigned val);

/* Get a register from the core */
unsigned g65816_get_reg(int regnum);

/* Set a register in the core */
void g65816_set_reg(int regnum, unsigned val);

/* Set the callback that will be called when an interrupt is serviced */
void g65816_set_irq_callback(int (*callback)(int));


/* -------------------------- */
/* CPU Hardware Interface API */
/* -------------------------- */

/* Set the RESET line on the CPU */
void g65816_reset(void* param);

/* Note about NMI:
 *   NMI is a one-shot trigger.  In order to trigger NMI again, you must
 *   clear NMI and then assert it again.
 */
void g65816_set_nmi_line(int state);

/* Assert or clear the IRQ pin */
void g65816_set_irq_line(int line, int state);

/* Execute instructions for <clocks> CPU cycles */
int g65816_execute(int clocks);




/* ======================================================================== */
/* =================== Functions Implemented by the Host ================== */
/* ======================================================================== */

/* Read data from RAM */
unsigned int g65816_read_8(unsigned int address);

/* Read data from ROM */
unsigned int g65816_read_8_immediate(unsigned int address);

/* Write data to RAM */
void g65816_write_8(unsigned int address, unsigned int value);

/* Notification of PC changes */
void g65816_jumping(unsigned int new_pc);
void g65816_branching(unsigned int new_pc);



/* ======================================================================== */
/* ================================= MAME ================================= */
/* ======================================================================== */

/* Clean up after the emulation core - Not used in this core - */
void g65816_exit(void);

/* Save the current CPU state to disk */
void g65816_state_save(void *file);

/* Load a CPU state from disk */
void g65816_state_load(void *file);

/* Get a formatted string representing a register and its contents */
const char *g65816_info(void *context, int regnum);

/* Disassemble an instruction */
unsigned g65816_dasm(char *buffer, unsigned pc);


#include "cpuintrf.h"
#include "memory.h"
#include "driver.h"
#include "state.h"
#include "mamedbg.h"

#undef G65816_CALL_DEBUGGER
#define G65816_CALL_DEBUGGER CALL_MAME_DEBUG

#define g65816_read_8(addr) 			cpu_readmem24(addr)
#define g65816_write_8(addr,data)		cpu_writemem24(addr,data)
#define g65816_read_8_immediate(A)		cpu_readmem24(A)
#define g65816_jumping(A)				change_pc24(A)
#define g65816_branching(A)



/* ======================================================================== */
/* ============================== END OF FILE ============================= */
/* ======================================================================== */

#endif /* HEADER__G65816 */
