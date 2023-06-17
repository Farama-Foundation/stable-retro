/* ======================================================================== */
/* =============================== COPYRIGHT ============================== */
/* ======================================================================== */
/*

G65C816 CPU Emulator V0.93

Copyright (c) 2000 Karl Stenerud
All rights reserved.

Permission is granted to use this source code for non-commercial purposes.
To use this code for commercial purposes, you must get permission from the
author (Karl Stenerud) at karl@higashiyama-unet.ocn.ne.jp.


*/
/* ======================================================================== */
/* ================================= NOTES ================================ */
/* ======================================================================== */
/*

Changes:
	0.93 (2003-07-05):
			Angelo Salese <lordkale@libero.it>
			- Fixed the BCD conversion when using the Decimal Flag in ADC and SBC.
			- Removed the two conversion tables for ADC and SBC as they aren't
			  needed anymore.

	0.92 (2000-05-28):
			Lee Hammerton <lee-hammerton@hammerhead.ltd.uk>
			- Fixed debugger bug that caused D to be misrepresented.
			- Fixed MVN and MVP (they were reversed)

	0.91 (2000-05-22):
			Lee Hammerton <lee-hammerton@hammerhead.ltd.uk>
			- Fixed reset vector fetch to be little endian
			- Fixed disassembler call bug
			- Fixed C flag in SBC (should be inverted before operation)
			- Fixed JSR to stack PC-1 and RTS to pull PC and add 1

			Karl Stenerud <karl@higashiyama-unet.ocn.ne.jp>
			- Added correct timing for absolute indexed operations
			- SBC: fixed corruption of interim values

	0.90 (2000-05-17):
			Karl Stenerud <karl@higashiyama-unet.ocn.ne.jp>
			- first public release


Note on timings:
	- For instructions that write to memory (ASL, ASR, LSL, ROL, ROR, DEC,
	  INC, STA, STZ), the absolute indexed addressing mode takes 1 extra
	  cycle to complete.
	- The spec says fc (JMP axi) is 6 cyles, but elsewhere says 8 cycles
	  (which is what it should be)


TODO general:
	- WAI will not stop if RDY is held high.

	- RDY internally held low when WAI executed and returned to hi when RES,
	  ABORT, NMI, or IRQ asserted.

	- ABORT will terminate WAI instruction but wil not restart the processor

	- If interrupt occurs after ABORT of WAI, processor returns to WAI
	  instruction.

	- Add one cycle when indexing across page boundary and E=1 except for STA
	  and STZ instructions.

	- Add 1 cycle if branch is taken. In Emulation (E= 1 ) mode only --add 1
	  cycle if the branch is taken and crosses a page boundary.

	- Add 1 cycle in Emulation mode (E=1) for (dir),y; abs,x; and abs,y
	  addressing modes.

*/
/* ======================================================================== */
/* ================================= DATA ================================= */
/* ======================================================================== */

#include "g65816cm.h"

/* Our CPU structure */
g65816i_cpu_struct g65816i_cpu = {0};

int g65816_ICount = 0;

/* Temporary Variables */
uint g65816i_source;
uint g65816i_destination;

/* Layout of the registers in the MAME debugger */
static unsigned char g65816i_register_layout[] =
{
	G65816_PB, G65816_PC, G65816_S, G65816_DB, G65816_D, G65816_P, G65816_E, -1,
	G65816_A, G65816_X, G65816_Y, G65816_NMI_STATE, G65816_IRQ_STATE, 0
};

/* Layout of the MAME debugger windows x,y,w,h */
static unsigned char g65816i_window_layout[] = {
	25, 0,55, 2, /* register window (top, right rows) */
	 0, 0,24,22, /* disassembler window (left colums) */
	25, 3,55, 9, /* memory #1 window (right, upper middle) */
	25,13,55, 9, /* memory #2 window (right, lower middle) */
	 0,23,80, 1, /* command line window (bottom rows) */
};

extern void (*g65816i_opcodes_M0X0[])(void);
extern uint g65816i_get_reg_M0X0(int regnum);
extern void g65816i_set_reg_M0X0(int regnum, uint val);
extern void g65816i_set_line_M0X0(int line, int state);
extern int  g65816i_execute_M0X0(int cycles);

extern void (*g65816i_opcodes_M0X1[])(void);
extern uint g65816i_get_reg_M0X1(int regnum);
extern void g65816i_set_reg_M0X1(int regnum, uint val);
extern void g65816i_set_line_M0X1(int line, int state);
extern int  g65816i_execute_M0X1(int cycles);

extern void (*g65816i_opcodes_M1X0[])(void);
extern uint g65816i_get_reg_M1X0(int regnum);
extern void g65816i_set_reg_M1X0(int regnum, uint val);
extern void g65816i_set_line_M1X0(int line, int state);
extern int  g65816i_execute_M1X0(int cycles);

extern void (*g65816i_opcodes_M1X1[])(void);
extern uint g65816i_get_reg_M1X1(int regnum);
extern void g65816i_set_reg_M1X1(int regnum, uint val);
extern void g65816i_set_line_M1X1(int line, int state);
extern int  g65816i_execute_M1X1(int cycles);

extern void (*g65816i_opcodes_E[])(void);
extern uint g65816i_get_reg_E(int regnum);
extern void g65816i_set_reg_E(int regnum, uint val);
extern void g65816i_set_line_E(int line, int state);
extern int  g65816i_execute_E(int cycles);

void (**g65816i_opcodes[5])(void) =
{
	g65816i_opcodes_M0X0,
	g65816i_opcodes_M0X1,
	g65816i_opcodes_M1X0,
	g65816i_opcodes_M1X1,
	g65816i_opcodes_E
};

uint (*g65816i_get_reg[5])(int regnum) =
{
	g65816i_get_reg_M0X0,
	g65816i_get_reg_M0X1,
	g65816i_get_reg_M1X0,
	g65816i_get_reg_M1X1,
	g65816i_get_reg_E
};

void (*g65816i_set_reg[5])(int regnum, uint val) =
{
	g65816i_set_reg_M0X0,
	g65816i_set_reg_M0X1,
	g65816i_set_reg_M1X0,
	g65816i_set_reg_M1X1,
	g65816i_set_reg_E
};

void (*g65816i_set_line[5])(int line, int state) =
{
	g65816i_set_line_M0X0,
	g65816i_set_line_M0X1,
	g65816i_set_line_M1X0,
	g65816i_set_line_M1X1,
	g65816i_set_line_E
};

int (*g65816i_execute[5])(int cycles) =
{
	g65816i_execute_M0X0,
	g65816i_execute_M0X1,
	g65816i_execute_M1X0,
	g65816i_execute_M1X1,
	g65816i_execute_E
};

/* ======================================================================== */
/* ================================= API ================================== */
/* ======================================================================== */


void g65816_reset(void* param)
{
		/* Start the CPU */
		CPU_STOPPED = 0;

		/* Put into emulation mode */
		REGISTER_D = 0;
		REGISTER_PB = 0;
		REGISTER_DB = 0;
		REGISTER_S = (REGISTER_S & 0xff) | 0x100;
		REGISTER_X &= 0xff;
		REGISTER_Y &= 0xff;
		if(!FLAG_M)
		{
			REGISTER_B = REGISTER_A & 0xff00;
			REGISTER_A &= 0xff;
		}
		FLAG_E = EFLAG_SET;
		FLAG_M = MFLAG_SET;
		FLAG_X = XFLAG_SET;

		/* Clear D and set I */
		FLAG_D = DFLAG_CLEAR;
		FLAG_I = IFLAG_SET;

		/* Clear all pending interrupts (should we really do this?) */
		LINE_IRQ = 0;
		LINE_NMI = 0;
		IRQ_DELAY = 0;

		/* Set the function tables to emulation mode */
		g65816i_set_execution_mode(EXECUTION_MODE_E);

		/* 6502 expects these, but its not in the 65816 spec */
		FLAG_Z = ZFLAG_CLEAR;
		REGISTER_S = 0x1ff;

		/* Fetch the reset vector */
		REGISTER_PC = g65816_read_8(VECTOR_RESET) | (g65816_read_8(VECTOR_RESET+1)<<8);
		g65816i_jumping(REGISTER_PB | REGISTER_PC);
}

/* Exit and clean up */
void g65816_exit(void)
{
	/* nothing to do yet */
}

/* Execute some instructions */
int g65816_execute(int cycles)
{
	return FTABLE_EXECUTE(cycles);
}


/* Get the current CPU context */
unsigned g65816_get_context(void *dst_context)
{
	if(dst_context)
		*(g65816i_cpu_struct*)dst_context = g65816i_cpu;
	return sizeof(g65816i_cpu);
}

/* Set the current CPU context */
void g65816_set_context(void *src_context)
{
	if(src_context)
	{
		g65816i_cpu = *(g65816i_cpu_struct*)src_context;
		g65816i_jumping(REGISTER_PB | REGISTER_PC);
	}
}

/* Get the current Program Counter */
unsigned g65816_get_pc(void)
{
	return REGISTER_PC;
}

/* Set the Program Counter */
void g65816_set_pc(unsigned val)
{
	REGISTER_PC = MAKE_UINT_16(val);
	g65816_jumping(REGISTER_PB | REGISTER_PC);
}

/* Get the current Stack Pointer */
unsigned g65816_get_sp(void)
{
	return REGISTER_S;
}

/* Set the Stack Pointer */
void g65816_set_sp(unsigned val)
{
	REGISTER_S = FLAG_E ? MAKE_UINT_8(val) | 0x100 : MAKE_UINT_16(val);
}

/* Get a register */
unsigned g65816_get_reg(int regnum)
{
	/* Set the function tables to emulation mode if the FTABLE is NULL */
	if( FTABLE_GET_REG == NULL )
		g65816i_set_execution_mode(EXECUTION_MODE_E);

	return FTABLE_GET_REG(regnum);
}

/* Set a register */
void g65816_set_reg(int regnum, unsigned value)
{
	FTABLE_SET_REG(regnum, value);
}

/* Load a CPU state */
void g65816_state_load(void *file)
{
}

/* Save the current CPU state */
void g65816_state_save(void *file)
{
}

/* Set the non-maskable interrupt line */
void g65816_set_nmi_line(int state)
{
	FTABLE_SET_LINE(G65816_LINE_NMI, state);
}

/* Set an interrupt line */
void g65816_set_irq_line(int line, int state)
{
	FTABLE_SET_LINE(line, state);
}

/* Set the callback that is called when servicing an interrupt */
void g65816_set_irq_callback(int (*callback)(int))
{
	INT_ACK = callback;
}


/* Get a formatted string representing a register and its contents */
const char *g65816_info(void *context, int regnum)
{
	static char buffer[16][47+1];
	static int which = 0;
	g65816i_cpu_struct* r = context;

	which = (which + 1) % 16;
	buffer[which][0] = '\0';
	if(!context)
		r = &g65816i_cpu;

	switch(regnum)
	{
		case CPU_INFO_REG+G65816_PC:		sprintf(buffer[which], "PC:%04X", r->pc); break;
		case CPU_INFO_REG+G65816_PB:		sprintf(buffer[which], "PB:%02X", r->pb>>16); break;
		case CPU_INFO_REG+G65816_DB:		sprintf(buffer[which], "DB:%02X", r->db>>16); break;
		case CPU_INFO_REG+G65816_D:			sprintf(buffer[which], "D:%04X", r->d); break;
		case CPU_INFO_REG+G65816_S:			sprintf(buffer[which], "S:%04X", r->s); break;
		case CPU_INFO_REG+G65816_P:			sprintf(buffer[which], "P:%02X",
											 (r->flag_n&0x80)		|
											((r->flag_v>>1)&0x40)	|
											r->flag_m				|
											r->flag_x				|
											r->flag_d				|
											r->flag_i				|
											((!r->flag_z)<<1)		|
											((r->flag_c>>8)&1)); break;
		case CPU_INFO_REG+G65816_E:			sprintf(buffer[which], "E:%d", r->flag_e); break;
		case CPU_INFO_REG+G65816_A:			sprintf(buffer[which], "A:%04X", r->a | r->b); break;
		case CPU_INFO_REG+G65816_X:			sprintf(buffer[which], "X:%04X", r->x); break;
		case CPU_INFO_REG+G65816_Y:			sprintf(buffer[which], "Y:%04X", r->y); break;
		case CPU_INFO_REG+G65816_NMI_STATE:	sprintf(buffer[which], "NMI:%X", r->line_nmi); break;
		case CPU_INFO_REG+G65816_IRQ_STATE:	sprintf(buffer[which], "IRQ:%X", r->line_irq); break;
		case CPU_INFO_FLAGS:
			sprintf(buffer[which], "%c%c%c%c%c%c%c%c",
				r->flag_n & NFLAG_SET ? 'N':'.',
				r->flag_v & VFLAG_SET ? 'V':'.',
				r->flag_m & MFLAG_SET ? 'M':'.',
				r->flag_x & XFLAG_SET ? 'X':'.',
				r->flag_d & DFLAG_SET ? 'D':'.',
				r->flag_i & IFLAG_SET ? 'I':'.',
				r->flag_z == 0        ? 'Z':'.',
				r->flag_c & CFLAG_SET ? 'C':'.');
			break;
		case CPU_INFO_NAME: return "G65C816";
		case CPU_INFO_FAMILY: return "6500";
		case CPU_INFO_VERSION: return "0.90";
		case CPU_INFO_FILE: return __FILE__;
		case CPU_INFO_CREDITS: return "Copyright (c) 2000 Karl Stenerud, all rights reserved.";
		case CPU_INFO_REG_LAYOUT: return (const char*)g65816i_register_layout;
		case CPU_INFO_WIN_LAYOUT: return (const char*)g65816i_window_layout;
	}
	return buffer[which];
}


/* Disassemble an instruction */
#ifdef MAME_DEBUG
#include "g65816ds.h"
#endif
unsigned g65816_dasm(char *buffer, unsigned pc)
{
#ifdef MAME_DEBUG
	return g65816_disassemble(buffer, (pc&0xffff), REGISTER_PB>>16, FLAG_M, FLAG_X);
#else
	sprintf(buffer, "$%02X", g65816_read_8_immediate(REGISTER_PB | (pc&0xffff)));
	return 1;
#endif
}


void g65816_init(void){ return; }


/* ======================================================================== */
/* ============================== END OF FILE ============================= */
/* ======================================================================== */
