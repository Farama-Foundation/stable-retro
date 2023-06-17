#include <stdio.h>
#include <stdlib.h>
#include "m68k.h"
#include "m68000.h"
#include "state.h"

/* global access */

struct m68k_memory_interface m68k_memory_intf;

#ifndef A68K0

/****************************************************************************
 * 24-bit address, 16-bit data memory interface
 ****************************************************************************/

static data32_t readlong_a24_d16(offs_t address)
{
	data32_t result = cpu_readmem24bew_word(address) << 16;
	return result | cpu_readmem24bew_word(address + 2);
}

static void writelong_a24_d16(offs_t address, data32_t data)
{
	cpu_writemem24bew_word(address, data >> 16);
	cpu_writemem24bew_word(address + 2, data);
}

static void changepc_a24_d16(offs_t pc)
{
	change_pc24bew(pc);
}

/* interface for 24-bit address bus, 16-bit data bus (68000, 68010) */
static const struct m68k_memory_interface interface_a24_d16 =
{
	0,
	cpu_readmem24bew,
	cpu_readmem24bew_word,
	readlong_a24_d16,
	cpu_writemem24bew,
	cpu_writemem24bew_word,
	writelong_a24_d16,
	changepc_a24_d16
};

#endif /* A68K0 */

/****************************************************************************
 * 24-bit address, 32-bit data memory interface
 ****************************************************************************/

#ifndef A68K2

/* potentially misaligned 16-bit reads with a 32-bit data bus (and 24-bit address bus) */
static data16_t readword_a24_d32(offs_t address)
{
	data16_t result;

	if (!(address & 1))
		return cpu_readmem24bedw_word(address);
	result = cpu_readmem24bedw(address) << 8;
	return result | cpu_readmem24bedw(address + 1);
}

/* potentially misaligned 16-bit writes with a 32-bit data bus (and 24-bit address bus) */
static void writeword_a24_d32(offs_t address, data16_t data)
{
	if (!(address & 1))
	{
		cpu_writemem24bedw_word(address, data);
		return;
	}
	cpu_writemem24bedw(address, data >> 8);
	cpu_writemem24bedw(address + 1, data);
}

/* potentially misaligned 32-bit reads with a 32-bit data bus (and 24-bit address bus) */
static data32_t readlong_a24_d32(offs_t address)
{
	data32_t result;

	if (!(address & 3))
		return cpu_readmem24bedw_dword(address);
	else if (!(address & 1))
	{
		result = cpu_readmem24bedw_word(address) << 16;
		return result | cpu_readmem24bedw_word(address + 2);
	}
	result = cpu_readmem24bedw(address) << 24;
	result |= cpu_readmem24bedw_word(address + 1) << 8;
	return result | cpu_readmem24bedw(address + 3);
}

/* potentially misaligned 32-bit writes with a 32-bit data bus (and 24-bit address bus) */
static void writelong_a24_d32(offs_t address, data32_t data)
{
	if (!(address & 3))
	{
		cpu_writemem24bedw_dword(address, data);
		return;
	}
	else if (!(address & 1))
	{
		cpu_writemem24bedw_word(address, data >> 16);
		cpu_writemem24bedw_word(address + 2, data);
		return;
	}
	cpu_writemem24bedw(address, data >> 24);
	cpu_writemem24bedw_word(address + 1, data >> 8);
	cpu_writemem24bedw(address + 3, data);
}

static void changepc_a24_d32(offs_t pc)
{
	change_pc24bedw(pc);
}

/* interface for 24-bit address bus, 32-bit data bus (68EC020) */
static const struct m68k_memory_interface interface_a24_d32 =
{
	WORD_XOR_BE(0),
	cpu_readmem24bedw,
	readword_a24_d32,
	readlong_a24_d32,
	cpu_writemem24bedw,
	writeword_a24_d32,
	writelong_a24_d32,
	changepc_a24_d32
};


/****************************************************************************
 * 32-bit address, 32-bit data memory interface
 ****************************************************************************/

/* potentially misaligned 16-bit reads with a 32-bit data bus (and 32-bit address bus) */
static data16_t readword_a32_d32(offs_t address)
{
	data16_t result;

	if (!(address & 1))
		return cpu_readmem32bedw_word(address);
	result = cpu_readmem32bedw(address) << 8;
	return result | cpu_readmem32bedw(address + 1);
}

/* potentially misaligned 16-bit writes with a 32-bit data bus (and 32-bit address bus) */
static void writeword_a32_d32(offs_t address, data16_t data)
{
	if (!(address & 1))
	{
		cpu_writemem32bedw_word(address, data);
		return;
	}
	cpu_writemem32bedw(address, data >> 8);
	cpu_writemem32bedw(address + 1, data);
}

/* potentially misaligned 32-bit reads with a 32-bit data bus (and 32-bit address bus) */
static data32_t readlong_a32_d32(offs_t address)
{
	data32_t result;

	if (!(address & 3))
		return cpu_readmem32bedw_dword(address);
	else if (!(address & 1))
	{
		result = cpu_readmem32bedw_word(address) << 16;
		return result | cpu_readmem32bedw_word(address + 2);
	}
	result = cpu_readmem32bedw(address) << 24;
	result |= cpu_readmem32bedw_word(address + 1) << 8;
	return result | cpu_readmem32bedw(address + 3);
}

/* potentially misaligned 32-bit writes with a 32-bit data bus (and 32-bit address bus) */
static void writelong_a32_d32(offs_t address, data32_t data)
{
	if (!(address & 3))
	{
		cpu_writemem32bedw_dword(address, data);
		return;
	}
	else if (!(address & 1))
	{
		cpu_writemem32bedw_word(address,     data >> 16);
		cpu_writemem32bedw_word(address + 2, data);
		return;
	}
	cpu_writemem32bedw(address, data >> 24);
	cpu_writemem32bedw_word(address + 1, data >> 8);
	cpu_writemem32bedw(address + 3, data);
}

static void changepc_a32_d32(offs_t pc)
{
	change_pc32bedw(pc);
}

/* interface for 24-bit address bus, 32-bit data bus (68020) */
static const struct m68k_memory_interface interface_a32_d32 =
{
	WORD_XOR_BE(0),
	cpu_readmem32bedw,
	readword_a32_d32,
	readlong_a32_d32,
	cpu_writemem32bedw,
	writeword_a32_d32,
	writelong_a32_d32,
	changepc_a32_d32
};

/* global access */
struct m68k_memory_interface m68k_memory_intf;

#endif /* A68K2 */

/****************************************************************************
 * 68000 section
 ****************************************************************************/

#ifndef A68K0

static UINT8 m68000_reg_layout[] = {
	M68K_PC, M68K_ISP, -1,
	M68K_SR, M68K_USP, -1,
	M68K_D0, M68K_A0, -1,
	M68K_D1, M68K_A1, -1,
	M68K_D2, M68K_A2, -1,
	M68K_D3, M68K_A3, -1,
	M68K_D4, M68K_A4, -1,
	M68K_D5, M68K_A5, -1,
	M68K_D6, M68K_A6, -1,
	M68K_D7, M68K_A7, 0
};

static UINT8 m68000_win_layout[] = {
	48, 0,32,13,	/* register window (top right) */
	 0, 0,47,13,	/* disassembler window (top left) */
	 0,14,47, 8,	/* memory #1 window (left, middle) */
	48,14,32, 8,	/* memory #2 window (right, middle) */
	 0,23,80, 1 	/* command line window (bottom rows) */
};

void m68000_init(void)
{
	m68k_init();
	m68k_set_cpu_type(M68K_CPU_TYPE_68000);
	m68k_memory_intf = interface_a24_d16;
	m68k_state_register("m68000");
}

void m68000_reset(void* param)
{
	m68k_pulse_reset();
}

void m68000_exit(void)
{
	/* nothing to do */
}

int m68000_execute(int cycles)
{
	return m68k_execute(cycles);
}

unsigned m68000_get_context(void *dst)
{
	return m68k_get_context(dst);
}

void m68000_set_context(void *src)
{
	if (m68k_memory_intf.read8 != cpu_readmem24bew)
		m68k_memory_intf = interface_a24_d16;
	m68k_set_context(src);
}

unsigned m68000_get_reg(int regnum)
{
	switch( regnum )
	{
		case REG_PC:   return m68k_get_reg(NULL, M68K_REG_PC)&0x00ffffff;
		case M68K_PC:  return m68k_get_reg(NULL, M68K_REG_PC);
		case REG_SP:
		case M68K_SP:  return m68k_get_reg(NULL, M68K_REG_SP);
		case M68K_ISP: return m68k_get_reg(NULL, M68K_REG_ISP);
		case M68K_USP: return m68k_get_reg(NULL, M68K_REG_USP);
		case M68K_SR:  return m68k_get_reg(NULL, M68K_REG_SR);
		case M68K_D0:  return m68k_get_reg(NULL, M68K_REG_D0);
		case M68K_D1:  return m68k_get_reg(NULL, M68K_REG_D1);
		case M68K_D2:  return m68k_get_reg(NULL, M68K_REG_D2);
		case M68K_D3:  return m68k_get_reg(NULL, M68K_REG_D3);
		case M68K_D4:  return m68k_get_reg(NULL, M68K_REG_D4);
		case M68K_D5:  return m68k_get_reg(NULL, M68K_REG_D5);
		case M68K_D6:  return m68k_get_reg(NULL, M68K_REG_D6);
		case M68K_D7:  return m68k_get_reg(NULL, M68K_REG_D7);
		case M68K_A0:  return m68k_get_reg(NULL, M68K_REG_A0);
		case M68K_A1:  return m68k_get_reg(NULL, M68K_REG_A1);
		case M68K_A2:  return m68k_get_reg(NULL, M68K_REG_A2);
		case M68K_A3:  return m68k_get_reg(NULL, M68K_REG_A3);
		case M68K_A4:  return m68k_get_reg(NULL, M68K_REG_A4);
		case M68K_A5:  return m68k_get_reg(NULL, M68K_REG_A5);
		case M68K_A6:  return m68k_get_reg(NULL, M68K_REG_A6);
		case M68K_A7:  return m68k_get_reg(NULL, M68K_REG_A7);
		case M68K_PREF_ADDR:  return m68k_get_reg(NULL, M68K_REG_PREF_ADDR);
		case M68K_PREF_DATA:  return m68k_get_reg(NULL, M68K_REG_PREF_DATA);
		case REG_PREVIOUSPC: return m68k_get_reg(NULL, M68K_REG_PPC);
/* TODO: return contents of [SP + wordsize * (REG_SP_CONTENTS-regnum)] */
		default:
			if( regnum < REG_SP_CONTENTS )
			{
				unsigned offset = m68k_get_reg(NULL, M68K_REG_SP) + 4 * (REG_SP_CONTENTS - regnum);
				if( offset < 0xfffffd )
					return m68k_read_memory_32( offset );
			}
	}
	return 0;
}

void m68000_set_reg(int regnum, unsigned val)
{
	switch( regnum )
	{
		case REG_PC:   m68k_set_reg(M68K_REG_PC, val&0x00ffffff); break;
		case M68K_PC:  m68k_set_reg(M68K_REG_PC, val); break;
		case REG_SP:
		case M68K_SP:  m68k_set_reg(M68K_REG_SP, val); break;
		case M68K_ISP: m68k_set_reg(M68K_REG_ISP, val); break;
		case M68K_USP: m68k_set_reg(M68K_REG_USP, val); break;
		case M68K_SR:  m68k_set_reg(M68K_REG_SR, val); break;
		case M68K_D0:  m68k_set_reg(M68K_REG_D0, val); break;
		case M68K_D1:  m68k_set_reg(M68K_REG_D1, val); break;
		case M68K_D2:  m68k_set_reg(M68K_REG_D2, val); break;
		case M68K_D3:  m68k_set_reg(M68K_REG_D3, val); break;
		case M68K_D4:  m68k_set_reg(M68K_REG_D4, val); break;
		case M68K_D5:  m68k_set_reg(M68K_REG_D5, val); break;
		case M68K_D6:  m68k_set_reg(M68K_REG_D6, val); break;
		case M68K_D7:  m68k_set_reg(M68K_REG_D7, val); break;
		case M68K_A0:  m68k_set_reg(M68K_REG_A0, val); break;
		case M68K_A1:  m68k_set_reg(M68K_REG_A1, val); break;
		case M68K_A2:  m68k_set_reg(M68K_REG_A2, val); break;
		case M68K_A3:  m68k_set_reg(M68K_REG_A3, val); break;
		case M68K_A4:  m68k_set_reg(M68K_REG_A4, val); break;
		case M68K_A5:  m68k_set_reg(M68K_REG_A5, val); break;
		case M68K_A6:  m68k_set_reg(M68K_REG_A6, val); break;
		case M68K_A7:  m68k_set_reg(M68K_REG_A7, val); break;
/* TODO: set contents of [SP + wordsize * (REG_SP_CONTENTS-regnum)] */
		default:
			if( regnum < REG_SP_CONTENTS )
			{
				unsigned offset = m68k_get_reg(NULL, M68K_REG_SP) + 4 * (REG_SP_CONTENTS - regnum);
				if( offset < 0xfffffd )
					m68k_write_memory_16( offset, val );
			}
	}
}

void m68000_set_irq_line(int irqline, int state)
{
	if (irqline == IRQ_LINE_NMI)
		irqline = 7;
	switch(state)
	{
		case CLEAR_LINE:
			m68k_set_irq(0);
			break;
		case ASSERT_LINE:
			m68k_set_irq(irqline);
			break;
		default:
			m68k_set_irq(irqline);
			break;
	}
}

void m68000_set_irq_callback(int (*callback)(int irqline))
{
	m68k_set_int_ack_callback(callback);
}


const char *m68000_info(void *context, int regnum)
{
	static char buffer[32][47+1];
	static int which = 0;
	int sr;

	which = (which+1) % 32;
	buffer[which][0] = '\0';

	switch( regnum )
	{
		case CPU_INFO_REG+M68K_PC:	sprintf(buffer[which], "PC :%08X", m68k_get_reg(context, M68K_REG_PC)); break;
		case CPU_INFO_REG+M68K_SR:  sprintf(buffer[which], "SR :%04X", m68k_get_reg(context, M68K_REG_SR)); break;
		case CPU_INFO_REG+M68K_SP:  sprintf(buffer[which], "SP :%08X", m68k_get_reg(context, M68K_REG_SP)); break;
		case CPU_INFO_REG+M68K_ISP: sprintf(buffer[which], "ISP:%08X", m68k_get_reg(context, M68K_REG_ISP)); break;
		case CPU_INFO_REG+M68K_USP: sprintf(buffer[which], "USP:%08X", m68k_get_reg(context, M68K_REG_USP)); break;
		case CPU_INFO_REG+M68K_D0:	sprintf(buffer[which], "D0 :%08X", m68k_get_reg(context, M68K_REG_D0)); break;
		case CPU_INFO_REG+M68K_D1:	sprintf(buffer[which], "D1 :%08X", m68k_get_reg(context, M68K_REG_D1)); break;
		case CPU_INFO_REG+M68K_D2:	sprintf(buffer[which], "D2 :%08X", m68k_get_reg(context, M68K_REG_D2)); break;
		case CPU_INFO_REG+M68K_D3:	sprintf(buffer[which], "D3 :%08X", m68k_get_reg(context, M68K_REG_D3)); break;
		case CPU_INFO_REG+M68K_D4:	sprintf(buffer[which], "D4 :%08X", m68k_get_reg(context, M68K_REG_D4)); break;
		case CPU_INFO_REG+M68K_D5:	sprintf(buffer[which], "D5 :%08X", m68k_get_reg(context, M68K_REG_D5)); break;
		case CPU_INFO_REG+M68K_D6:	sprintf(buffer[which], "D6 :%08X", m68k_get_reg(context, M68K_REG_D6)); break;
		case CPU_INFO_REG+M68K_D7:	sprintf(buffer[which], "D7 :%08X", m68k_get_reg(context, M68K_REG_D7)); break;
		case CPU_INFO_REG+M68K_A0:	sprintf(buffer[which], "A0 :%08X", m68k_get_reg(context, M68K_REG_A0)); break;
		case CPU_INFO_REG+M68K_A1:	sprintf(buffer[which], "A1 :%08X", m68k_get_reg(context, M68K_REG_A1)); break;
		case CPU_INFO_REG+M68K_A2:	sprintf(buffer[which], "A2 :%08X", m68k_get_reg(context, M68K_REG_A2)); break;
		case CPU_INFO_REG+M68K_A3:	sprintf(buffer[which], "A3 :%08X", m68k_get_reg(context, M68K_REG_A3)); break;
		case CPU_INFO_REG+M68K_A4:	sprintf(buffer[which], "A4 :%08X", m68k_get_reg(context, M68K_REG_A4)); break;
		case CPU_INFO_REG+M68K_A5:	sprintf(buffer[which], "A5 :%08X", m68k_get_reg(context, M68K_REG_A5)); break;
		case CPU_INFO_REG+M68K_A6:	sprintf(buffer[which], "A6 :%08X", m68k_get_reg(context, M68K_REG_A6)); break;
		case CPU_INFO_REG+M68K_A7:	sprintf(buffer[which], "A7 :%08X", m68k_get_reg(context, M68K_REG_A7)); break;
		case CPU_INFO_REG+M68K_PREF_ADDR:	sprintf(buffer[which], "PAR:%08X", m68k_get_reg(context, M68K_REG_PREF_ADDR)); break;
		case CPU_INFO_REG+M68K_PREF_DATA:	sprintf(buffer[which], "PDA:%08X", m68k_get_reg(context, M68K_REG_PREF_DATA)); break;
		case CPU_INFO_FLAGS:
			sr = m68k_get_reg(context, M68K_REG_SR);
			sprintf(buffer[which], "%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c",
				sr & 0x8000 ? 'T':'.',
				sr & 0x4000 ? '?':'.',
				sr & 0x2000 ? 'S':'.',
				sr & 0x1000 ? '?':'.',
				sr & 0x0800 ? '?':'.',
				sr & 0x0400 ? 'I':'.',
				sr & 0x0200 ? 'I':'.',
				sr & 0x0100 ? 'I':'.',
				sr & 0x0080 ? '?':'.',
				sr & 0x0040 ? '?':'.',
				sr & 0x0020 ? '?':'.',
				sr & 0x0010 ? 'X':'.',
				sr & 0x0008 ? 'N':'.',
				sr & 0x0004 ? 'Z':'.',
				sr & 0x0002 ? 'V':'.',
				sr & 0x0001 ? 'C':'.');
			break;
		case CPU_INFO_NAME: return "68000";
		case CPU_INFO_FAMILY: return "Motorola 68K";
		case CPU_INFO_VERSION: return "3.2";
		case CPU_INFO_FILE: return __FILE__;
		case CPU_INFO_CREDITS: return "Copyright 1999-2000 Karl Stenerud. All rights reserved. (2.1 fixes HJB)";
		case CPU_INFO_REG_LAYOUT: return (const char*)m68000_reg_layout;
		case CPU_INFO_WIN_LAYOUT: return (const char*)m68000_win_layout;
	}
	return buffer[which];
}

unsigned m68000_dasm(char *buffer, unsigned pc)
{
	M68K_SET_PC_CALLBACK(pc);
#ifdef MAME_DEBUG
	return m68k_disassemble( buffer, pc, M68K_CPU_TYPE_68000 );
#else
	sprintf( buffer, "$%04X", m68k_read_immediate_16(pc) );
	return 2;
#endif
}


/****************************************************************************
 * M68010 section
 ****************************************************************************/
#if HAS_M68010

static UINT8 m68010_reg_layout[] = {
	M68K_PC,  M68K_ISP, -1,
	M68K_SR,  M68K_USP, -1,
	M68K_SFC, M68K_VBR, -1,
	M68K_DFC, -1,
	M68K_D0,  M68K_A0, -1,
	M68K_D1,  M68K_A1, -1,
	M68K_D2,  M68K_A2, -1,
	M68K_D3,  M68K_A3, -1,
	M68K_D4,  M68K_A4, -1,
	M68K_D5,  M68K_A5, -1,
	M68K_D6,  M68K_A6, -1,
	M68K_D7,  M68K_A7, 0
};

static UINT8 m68010_win_layout[] = {
	48, 0,32,13,	/* register window (top right) */
	 0, 0,47,13,	/* disassembler window (top left) */
	 0,14,47, 8,	/* memory #1 window (left, middle) */
	48,14,32, 8,	/* memory #2 window (right, middle) */
	 0,23,80, 1 	/* command line window (bottom rows) */
};


void m68010_init(void)
{
	m68k_init();
	m68k_set_cpu_type(M68K_CPU_TYPE_68010);
	m68k_memory_intf = interface_a24_d16;
	m68k_state_register("m68010");
}

void m68010_reset(void* param)
{
	m68k_pulse_reset();
}

void m68010_exit(void)
{
	/* nothing to do */
}

int m68010_execute(int cycles)
{
	return m68k_execute(cycles);
}

unsigned m68010_get_context(void *dst)
{
	return m68k_get_context(dst);
}

void m68010_set_context(void *src)
{
	if (m68k_memory_intf.read8 != cpu_readmem24bew)
		m68k_memory_intf = interface_a24_d16;
	m68k_set_context(src);
}

unsigned m68010_get_reg(int regnum)
{
	switch( regnum )
	{
		case M68K_VBR: return m68k_get_reg(NULL, M68K_REG_VBR); /* 68010+ */
		case M68K_SFC: return m68k_get_reg(NULL, M68K_REG_SFC); /* 68010" */
		case M68K_DFC: return m68k_get_reg(NULL, M68K_REG_DFC); /* 68010+ */
		case REG_PC:   return m68k_get_reg(NULL, M68K_REG_PC)&0x00ffffff;
		case M68K_PC:  return m68k_get_reg(NULL, M68K_REG_PC);
		case REG_SP:
		case M68K_SP:  return m68k_get_reg(NULL, M68K_REG_SP);
		case M68K_ISP: return m68k_get_reg(NULL, M68K_REG_ISP);
		case M68K_USP: return m68k_get_reg(NULL, M68K_REG_USP);
		case M68K_SR:  return m68k_get_reg(NULL, M68K_REG_SR);
		case M68K_D0:  return m68k_get_reg(NULL, M68K_REG_D0);
		case M68K_D1:  return m68k_get_reg(NULL, M68K_REG_D1);
		case M68K_D2:  return m68k_get_reg(NULL, M68K_REG_D2);
		case M68K_D3:  return m68k_get_reg(NULL, M68K_REG_D3);
		case M68K_D4:  return m68k_get_reg(NULL, M68K_REG_D4);
		case M68K_D5:  return m68k_get_reg(NULL, M68K_REG_D5);
		case M68K_D6:  return m68k_get_reg(NULL, M68K_REG_D6);
		case M68K_D7:  return m68k_get_reg(NULL, M68K_REG_D7);
		case M68K_A0:  return m68k_get_reg(NULL, M68K_REG_A0);
		case M68K_A1:  return m68k_get_reg(NULL, M68K_REG_A1);
		case M68K_A2:  return m68k_get_reg(NULL, M68K_REG_A2);
		case M68K_A3:  return m68k_get_reg(NULL, M68K_REG_A3);
		case M68K_A4:  return m68k_get_reg(NULL, M68K_REG_A4);
		case M68K_A5:  return m68k_get_reg(NULL, M68K_REG_A5);
		case M68K_A6:  return m68k_get_reg(NULL, M68K_REG_A6);
		case M68K_A7:  return m68k_get_reg(NULL, M68K_REG_A7);
		case M68K_PREF_ADDR:  return m68k_get_reg(NULL, M68K_REG_PREF_ADDR);
		case M68K_PREF_DATA:  return m68k_get_reg(NULL, M68K_REG_PREF_DATA);
		case REG_PREVIOUSPC: return m68k_get_reg(NULL, M68K_REG_PPC);
/* TODO: return contents of [SP + wordsize * (REG_SP_CONTENTS-regnum)] */
		default:
			if( regnum < REG_SP_CONTENTS )
			{
				unsigned offset = m68k_get_reg(NULL, M68K_REG_SP) + 4 * (REG_SP_CONTENTS - regnum);
				if( offset < 0xfffffd )
					return m68k_read_memory_32( offset );
			}
	}
	return 0;
}

void m68010_set_reg(int regnum, unsigned val)
{
	switch( regnum )
	{
		case M68K_VBR: m68k_set_reg(M68K_REG_VBR, val); break; /* 68010+ */
		case M68K_SFC: m68k_set_reg(M68K_REG_SFC, val); break; /* 68010+ */
		case M68K_DFC: m68k_set_reg(M68K_REG_DFC, val); break; /* 68010+ */
		case REG_PC:   m68k_set_reg(M68K_REG_PC, val&0x00ffffff); break;
		case M68K_PC:  m68k_set_reg(M68K_REG_PC, val); break;
		case REG_SP:
		case M68K_SP:  m68k_set_reg(M68K_REG_SP, val); break;
		case M68K_ISP: m68k_set_reg(M68K_REG_ISP, val); break;
		case M68K_USP: m68k_set_reg(M68K_REG_USP, val); break;
		case M68K_SR:  m68k_set_reg(M68K_REG_SR, val); break;
		case M68K_D0:  m68k_set_reg(M68K_REG_D0, val); break;
		case M68K_D1:  m68k_set_reg(M68K_REG_D1, val); break;
		case M68K_D2:  m68k_set_reg(M68K_REG_D2, val); break;
		case M68K_D3:  m68k_set_reg(M68K_REG_D3, val); break;
		case M68K_D4:  m68k_set_reg(M68K_REG_D4, val); break;
		case M68K_D5:  m68k_set_reg(M68K_REG_D5, val); break;
		case M68K_D6:  m68k_set_reg(M68K_REG_D6, val); break;
		case M68K_D7:  m68k_set_reg(M68K_REG_D7, val); break;
		case M68K_A0:  m68k_set_reg(M68K_REG_A0, val); break;
		case M68K_A1:  m68k_set_reg(M68K_REG_A1, val); break;
		case M68K_A2:  m68k_set_reg(M68K_REG_A2, val); break;
		case M68K_A3:  m68k_set_reg(M68K_REG_A3, val); break;
		case M68K_A4:  m68k_set_reg(M68K_REG_A4, val); break;
		case M68K_A5:  m68k_set_reg(M68K_REG_A5, val); break;
		case M68K_A6:  m68k_set_reg(M68K_REG_A6, val); break;
		case M68K_A7:  m68k_set_reg(M68K_REG_A7, val); break;
/* TODO: set contents of [SP + wordsize * (REG_SP_CONTENTS-regnum)] */
		default:
			if( regnum < REG_SP_CONTENTS )
			{
				unsigned offset = m68k_get_reg(NULL, M68K_REG_SP) + 4 * (REG_SP_CONTENTS - regnum);
				if( offset < 0xfffffd )
					m68k_write_memory_16( offset, val );
			}
	}
}

void m68010_set_irq_line(int irqline, int state)
{
	if (irqline == IRQ_LINE_NMI)
		irqline = 7;
	switch(state)
	{
		case CLEAR_LINE:
			m68k_set_irq(0);
			break;
		case ASSERT_LINE:
			m68k_set_irq(irqline);
			break;
		default:
			m68k_set_irq(irqline);
			break;
	}
}

void m68010_set_irq_callback(int (*callback)(int irqline))
{
	m68k_set_int_ack_callback(callback);
}


const char *m68010_info(void *context, int regnum)
{
	static char buffer[32][47+1];
	static int which = 0;
	int sr;

	which = (which+1) % 32;
	buffer[which][0] = '\0';

	switch( regnum )
	{
		case CPU_INFO_REG+M68K_SFC: sprintf(buffer[which], "SFC:%X",   m68k_get_reg(context, M68K_REG_SFC)); break;
		case CPU_INFO_REG+M68K_DFC: sprintf(buffer[which], "DFC:%X",   m68k_get_reg(context, M68K_REG_DFC)); break;
		case CPU_INFO_REG+M68K_VBR: sprintf(buffer[which], "VBR:%08X", m68k_get_reg(context, M68K_REG_VBR)); break;
		case CPU_INFO_REG+M68K_PC:	sprintf(buffer[which], "PC :%08X", m68k_get_reg(context, M68K_REG_PC)); break;
		case CPU_INFO_REG+M68K_SR:  sprintf(buffer[which], "SR :%04X", m68k_get_reg(context, M68K_REG_SR)); break;
		case CPU_INFO_REG+M68K_SP:  sprintf(buffer[which], "SP :%08X", m68k_get_reg(context, M68K_REG_SP)); break;
		case CPU_INFO_REG+M68K_ISP: sprintf(buffer[which], "ISP:%08X", m68k_get_reg(context, M68K_REG_ISP)); break;
		case CPU_INFO_REG+M68K_USP: sprintf(buffer[which], "USP:%08X", m68k_get_reg(context, M68K_REG_USP)); break;
		case CPU_INFO_REG+M68K_D0:	sprintf(buffer[which], "D0 :%08X", m68k_get_reg(context, M68K_REG_D0)); break;
		case CPU_INFO_REG+M68K_D1:	sprintf(buffer[which], "D1 :%08X", m68k_get_reg(context, M68K_REG_D1)); break;
		case CPU_INFO_REG+M68K_D2:	sprintf(buffer[which], "D2 :%08X", m68k_get_reg(context, M68K_REG_D2)); break;
		case CPU_INFO_REG+M68K_D3:	sprintf(buffer[which], "D3 :%08X", m68k_get_reg(context, M68K_REG_D3)); break;
		case CPU_INFO_REG+M68K_D4:	sprintf(buffer[which], "D4 :%08X", m68k_get_reg(context, M68K_REG_D4)); break;
		case CPU_INFO_REG+M68K_D5:	sprintf(buffer[which], "D5 :%08X", m68k_get_reg(context, M68K_REG_D5)); break;
		case CPU_INFO_REG+M68K_D6:	sprintf(buffer[which], "D6 :%08X", m68k_get_reg(context, M68K_REG_D6)); break;
		case CPU_INFO_REG+M68K_D7:	sprintf(buffer[which], "D7 :%08X", m68k_get_reg(context, M68K_REG_D7)); break;
		case CPU_INFO_REG+M68K_A0:	sprintf(buffer[which], "A0 :%08X", m68k_get_reg(context, M68K_REG_A0)); break;
		case CPU_INFO_REG+M68K_A1:	sprintf(buffer[which], "A1 :%08X", m68k_get_reg(context, M68K_REG_A1)); break;
		case CPU_INFO_REG+M68K_A2:	sprintf(buffer[which], "A2 :%08X", m68k_get_reg(context, M68K_REG_A2)); break;
		case CPU_INFO_REG+M68K_A3:	sprintf(buffer[which], "A3 :%08X", m68k_get_reg(context, M68K_REG_A3)); break;
		case CPU_INFO_REG+M68K_A4:	sprintf(buffer[which], "A4 :%08X", m68k_get_reg(context, M68K_REG_A4)); break;
		case CPU_INFO_REG+M68K_A5:	sprintf(buffer[which], "A5 :%08X", m68k_get_reg(context, M68K_REG_A5)); break;
		case CPU_INFO_REG+M68K_A6:	sprintf(buffer[which], "A6 :%08X", m68k_get_reg(context, M68K_REG_A6)); break;
		case CPU_INFO_REG+M68K_A7:	sprintf(buffer[which], "A7 :%08X", m68k_get_reg(context, M68K_REG_A7)); break;
		case CPU_INFO_REG+M68K_PREF_ADDR:	sprintf(buffer[which], "PAR:%08X", m68k_get_reg(context, M68K_REG_PREF_ADDR)); break;
		case CPU_INFO_REG+M68K_PREF_DATA:	sprintf(buffer[which], "PDA:%08X", m68k_get_reg(context, M68K_REG_PREF_DATA)); break;
		case CPU_INFO_FLAGS:
			sr = m68k_get_reg(context, M68K_REG_SR);
			sprintf(buffer[which], "%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c",
				sr & 0x8000 ? 'T':'.',
				sr & 0x4000 ? '?':'.',
				sr & 0x2000 ? 'S':'.',
				sr & 0x1000 ? '?':'.',
				sr & 0x0800 ? '?':'.',
				sr & 0x0400 ? 'I':'.',
				sr & 0x0200 ? 'I':'.',
				sr & 0x0100 ? 'I':'.',
				sr & 0x0080 ? '?':'.',
				sr & 0x0040 ? '?':'.',
				sr & 0x0020 ? '?':'.',
				sr & 0x0010 ? 'X':'.',
				sr & 0x0008 ? 'N':'.',
				sr & 0x0004 ? 'Z':'.',
				sr & 0x0002 ? 'V':'.',
				sr & 0x0001 ? 'C':'.');
			break;
		case CPU_INFO_NAME: return "68010";
		case CPU_INFO_FAMILY: return "Motorola 68K";
		case CPU_INFO_VERSION: return "3.2";
		case CPU_INFO_FILE: return __FILE__;
		case CPU_INFO_CREDITS: return "Copyright 1999-2000 Karl Stenerud. All rights reserved. (2.1 fixes HJB)";
		case CPU_INFO_REG_LAYOUT: return (const char*)m68010_reg_layout;
		case CPU_INFO_WIN_LAYOUT: return (const char*)m68010_win_layout;
	}
	return buffer[which];
}

unsigned m68010_dasm(char *buffer, unsigned pc)
{
	M68K_SET_PC_CALLBACK(pc);
#ifdef MAME_DEBUG
	return m68k_disassemble(buffer, pc, M68K_CPU_TYPE_68010);
#else
	sprintf( buffer, "$%04X", m68k_read_immediate_16(pc) );
	return 2;
#endif
}

#endif /* HAS_M68010 */

#endif /* A68K0 */

/****************************************************************************
 * M680EC20 section
 ****************************************************************************/

#ifndef A68K2

#if HAS_M68EC020

static UINT8 m68ec020_reg_layout[] = {
	M68K_PC,  M68K_MSP, -1,
	M68K_SR,  M68K_ISP, -1,
	M68K_SFC, M68K_USP, -1,
	M68K_DFC, M68K_VBR, -1,
	M68K_D0,  M68K_A0, -1,
	M68K_D1,  M68K_A1, -1,
	M68K_D2,  M68K_A2, -1,
	M68K_D3,  M68K_A3, -1,
	M68K_D4,  M68K_A4, -1,
	M68K_D5,  M68K_A5, -1,
	M68K_D6,  M68K_A6, -1,
	M68K_D7,  M68K_A7, 0
};

static UINT8 m68ec020_win_layout[] = {
	48, 0,32,13,	/* register window (top right) */
	 0, 0,47,13,	/* disassembler window (top left) */
	 0,14,47, 8,	/* memory #1 window (left, middle) */
	48,14,32, 8,	/* memory #2 window (right, middle) */
	 0,23,80, 1 	/* command line window (bottom rows) */
};


void m68ec020_init(void)
{
	m68k_init();
	m68k_set_cpu_type(M68K_CPU_TYPE_68EC020);
	m68k_memory_intf = interface_a24_d32;
	m68k_state_register("m68ec020");
}

void m68ec020_reset(void* param)
{
	m68k_pulse_reset();
}

void m68ec020_exit(void)
{
	/* nothing to do */
}

int m68ec020_execute(int cycles)
{
	return m68k_execute(cycles);
}

unsigned m68ec020_get_context(void *dst)
{
	return m68k_get_context(dst);
}

void m68ec020_set_context(void *src)
{
	if (m68k_memory_intf.read8 != cpu_readmem24bedw)
		m68k_memory_intf = interface_a24_d32;
	m68k_set_context(src);
}

unsigned m68ec020_get_reg(int regnum)
{
	switch( regnum )
	{
		case M68K_MSP: return m68k_get_reg(NULL, M68K_REG_MSP); /* 68020+ */
		case M68K_CACR:  return m68k_get_reg(NULL, M68K_REG_CACR); /* 68020+ */
		case M68K_CAAR:  return m68k_get_reg(NULL, M68K_REG_CAAR); /* 68020+ */
		case M68K_VBR: return m68k_get_reg(NULL, M68K_REG_VBR); /* 68010+ */
		case M68K_SFC: return m68k_get_reg(NULL, M68K_REG_SFC); /* 68010" */
		case M68K_DFC: return m68k_get_reg(NULL, M68K_REG_DFC); /* 68010+ */
		case REG_PC:   return m68k_get_reg(NULL, M68K_REG_PC)&0x00ffffff;
		case M68K_PC:  return m68k_get_reg(NULL, M68K_REG_PC);
		case REG_SP:
		case M68K_SP:  return m68k_get_reg(NULL, M68K_REG_SP);
		case M68K_ISP: return m68k_get_reg(NULL, M68K_REG_ISP);
		case M68K_USP: return m68k_get_reg(NULL, M68K_REG_USP);
		case M68K_SR:  return m68k_get_reg(NULL, M68K_REG_SR);
		case M68K_D0:  return m68k_get_reg(NULL, M68K_REG_D0);
		case M68K_D1:  return m68k_get_reg(NULL, M68K_REG_D1);
		case M68K_D2:  return m68k_get_reg(NULL, M68K_REG_D2);
		case M68K_D3:  return m68k_get_reg(NULL, M68K_REG_D3);
		case M68K_D4:  return m68k_get_reg(NULL, M68K_REG_D4);
		case M68K_D5:  return m68k_get_reg(NULL, M68K_REG_D5);
		case M68K_D6:  return m68k_get_reg(NULL, M68K_REG_D6);
		case M68K_D7:  return m68k_get_reg(NULL, M68K_REG_D7);
		case M68K_A0:  return m68k_get_reg(NULL, M68K_REG_A0);
		case M68K_A1:  return m68k_get_reg(NULL, M68K_REG_A1);
		case M68K_A2:  return m68k_get_reg(NULL, M68K_REG_A2);
		case M68K_A3:  return m68k_get_reg(NULL, M68K_REG_A3);
		case M68K_A4:  return m68k_get_reg(NULL, M68K_REG_A4);
		case M68K_A5:  return m68k_get_reg(NULL, M68K_REG_A5);
		case M68K_A6:  return m68k_get_reg(NULL, M68K_REG_A6);
		case M68K_A7:  return m68k_get_reg(NULL, M68K_REG_A7);
		case M68K_PREF_ADDR:  return m68k_get_reg(NULL, M68K_REG_PREF_ADDR);
		case M68K_PREF_DATA:  return m68k_get_reg(NULL, M68K_REG_PREF_DATA);
		case REG_PREVIOUSPC: return m68k_get_reg(NULL, M68K_REG_PPC);
/* TODO: return contents of [SP + wordsize * (REG_SP_CONTENTS-regnum)] */
		default:
			if( regnum < REG_SP_CONTENTS )
			{
				unsigned offset = m68k_get_reg(NULL, M68K_REG_SP) + 4 * (REG_SP_CONTENTS - regnum);
				if( offset < 0xfffffd )
					return m68k_read_memory_32( offset );
			}
	}
	return 0;
}

void m68ec020_set_reg(int regnum, unsigned val)
{
	switch( regnum )
	{
		case M68K_MSP:  m68k_set_reg(M68K_REG_MSP, val); break; /* 68020+ */
		case M68K_CACR: m68k_set_reg(M68K_REG_CACR, val); break; /* 68020+ */
		case M68K_CAAR: m68k_set_reg(M68K_REG_CAAR, val); break; /* 68020+ */
		case M68K_VBR: m68k_set_reg(M68K_REG_VBR, val); break; /* 68010+ */
		case M68K_SFC: m68k_set_reg(M68K_REG_SFC, val); break; /* 68010+ */
		case M68K_DFC: m68k_set_reg(M68K_REG_DFC, val); break; /* 68010+ */
		case REG_PC:   m68k_set_reg(M68K_REG_PC, val&0x00ffffff); break;
		case M68K_PC:  m68k_set_reg(M68K_REG_PC, val); break;
		case REG_SP:
		case M68K_SP:  m68k_set_reg(M68K_REG_SP, val); break;
		case M68K_ISP: m68k_set_reg(M68K_REG_ISP, val); break;
		case M68K_USP: m68k_set_reg(M68K_REG_USP, val); break;
		case M68K_SR:  m68k_set_reg(M68K_REG_SR, val); break;
		case M68K_D0:  m68k_set_reg(M68K_REG_D0, val); break;
		case M68K_D1:  m68k_set_reg(M68K_REG_D1, val); break;
		case M68K_D2:  m68k_set_reg(M68K_REG_D2, val); break;
		case M68K_D3:  m68k_set_reg(M68K_REG_D3, val); break;
		case M68K_D4:  m68k_set_reg(M68K_REG_D4, val); break;
		case M68K_D5:  m68k_set_reg(M68K_REG_D5, val); break;
		case M68K_D6:  m68k_set_reg(M68K_REG_D6, val); break;
		case M68K_D7:  m68k_set_reg(M68K_REG_D7, val); break;
		case M68K_A0:  m68k_set_reg(M68K_REG_A0, val); break;
		case M68K_A1:  m68k_set_reg(M68K_REG_A1, val); break;
		case M68K_A2:  m68k_set_reg(M68K_REG_A2, val); break;
		case M68K_A3:  m68k_set_reg(M68K_REG_A3, val); break;
		case M68K_A4:  m68k_set_reg(M68K_REG_A4, val); break;
		case M68K_A5:  m68k_set_reg(M68K_REG_A5, val); break;
		case M68K_A6:  m68k_set_reg(M68K_REG_A6, val); break;
		case M68K_A7:  m68k_set_reg(M68K_REG_A7, val); break;
/* TODO: set contents of [SP + wordsize * (REG_SP_CONTENTS-regnum)] */
		default:
			if( regnum < REG_SP_CONTENTS )
			{
				unsigned offset = m68k_get_reg(NULL, M68K_REG_SP) + 4 * (REG_SP_CONTENTS - regnum);
				if( offset < 0xfffffd )
					m68k_write_memory_16( offset, val );
			}
	}
}

void m68ec020_set_irq_line(int irqline, int state)
{
	if (irqline == IRQ_LINE_NMI)
		irqline = 7;
	switch(state)
	{
		case CLEAR_LINE:
			m68k_set_irq(0);
			break;
		case ASSERT_LINE:
			m68k_set_irq(irqline);
			break;
		default:
			m68k_set_irq(irqline);
			break;
	}
}

void m68ec020_set_irq_callback(int (*callback)(int irqline))
{
	m68k_set_int_ack_callback(callback);
}

const char *m68ec020_info(void *context, int regnum)
{
	static char buffer[32][47+1];
	static int which = 0;
	int sr;

	which = (which+1) % 32;
	buffer[which][0] = '\0';

	switch( regnum )
	{
		case CPU_INFO_REG+M68K_MSP:  sprintf(buffer[which], "MSP:%08X", m68k_get_reg(context, M68K_REG_MSP)); break;
		case CPU_INFO_REG+M68K_CACR: sprintf(buffer[which], "CCR:%08X", m68k_get_reg(context, M68K_REG_CACR)); break;
		case CPU_INFO_REG+M68K_CAAR: sprintf(buffer[which], "CAR:%08X", m68k_get_reg(context, M68K_REG_CAAR)); break;
		case CPU_INFO_REG+M68K_SFC: sprintf(buffer[which], "SFC:%X",   m68k_get_reg(context, M68K_REG_SFC)); break;
		case CPU_INFO_REG+M68K_DFC: sprintf(buffer[which], "DFC:%X",   m68k_get_reg(context, M68K_REG_DFC)); break;
		case CPU_INFO_REG+M68K_VBR: sprintf(buffer[which], "VBR:%08X", m68k_get_reg(context, M68K_REG_VBR)); break;
		case CPU_INFO_REG+M68K_PC:	sprintf(buffer[which], "PC :%08X", m68k_get_reg(context, M68K_REG_PC)); break;
		case CPU_INFO_REG+M68K_SR:  sprintf(buffer[which], "SR :%04X", m68k_get_reg(context, M68K_REG_SR)); break;
		case CPU_INFO_REG+M68K_SP:  sprintf(buffer[which], "SP :%08X", m68k_get_reg(context, M68K_REG_SP)); break;
		case CPU_INFO_REG+M68K_ISP: sprintf(buffer[which], "ISP:%08X", m68k_get_reg(context, M68K_REG_ISP)); break;
		case CPU_INFO_REG+M68K_USP: sprintf(buffer[which], "USP:%08X", m68k_get_reg(context, M68K_REG_USP)); break;
		case CPU_INFO_REG+M68K_D0:	sprintf(buffer[which], "D0 :%08X", m68k_get_reg(context, M68K_REG_D0)); break;
		case CPU_INFO_REG+M68K_D1:	sprintf(buffer[which], "D1 :%08X", m68k_get_reg(context, M68K_REG_D1)); break;
		case CPU_INFO_REG+M68K_D2:	sprintf(buffer[which], "D2 :%08X", m68k_get_reg(context, M68K_REG_D2)); break;
		case CPU_INFO_REG+M68K_D3:	sprintf(buffer[which], "D3 :%08X", m68k_get_reg(context, M68K_REG_D3)); break;
		case CPU_INFO_REG+M68K_D4:	sprintf(buffer[which], "D4 :%08X", m68k_get_reg(context, M68K_REG_D4)); break;
		case CPU_INFO_REG+M68K_D5:	sprintf(buffer[which], "D5 :%08X", m68k_get_reg(context, M68K_REG_D5)); break;
		case CPU_INFO_REG+M68K_D6:	sprintf(buffer[which], "D6 :%08X", m68k_get_reg(context, M68K_REG_D6)); break;
		case CPU_INFO_REG+M68K_D7:	sprintf(buffer[which], "D7 :%08X", m68k_get_reg(context, M68K_REG_D7)); break;
		case CPU_INFO_REG+M68K_A0:	sprintf(buffer[which], "A0 :%08X", m68k_get_reg(context, M68K_REG_A0)); break;
		case CPU_INFO_REG+M68K_A1:	sprintf(buffer[which], "A1 :%08X", m68k_get_reg(context, M68K_REG_A1)); break;
		case CPU_INFO_REG+M68K_A2:	sprintf(buffer[which], "A2 :%08X", m68k_get_reg(context, M68K_REG_A2)); break;
		case CPU_INFO_REG+M68K_A3:	sprintf(buffer[which], "A3 :%08X", m68k_get_reg(context, M68K_REG_A3)); break;
		case CPU_INFO_REG+M68K_A4:	sprintf(buffer[which], "A4 :%08X", m68k_get_reg(context, M68K_REG_A4)); break;
		case CPU_INFO_REG+M68K_A5:	sprintf(buffer[which], "A5 :%08X", m68k_get_reg(context, M68K_REG_A5)); break;
		case CPU_INFO_REG+M68K_A6:	sprintf(buffer[which], "A6 :%08X", m68k_get_reg(context, M68K_REG_A6)); break;
		case CPU_INFO_REG+M68K_A7:	sprintf(buffer[which], "A7 :%08X", m68k_get_reg(context, M68K_REG_A7)); break;
		case CPU_INFO_REG+M68K_PREF_ADDR:	sprintf(buffer[which], "PAR:%08X", m68k_get_reg(context, M68K_REG_PREF_ADDR)); break;
		case CPU_INFO_REG+M68K_PREF_DATA:	sprintf(buffer[which], "PDA:%08X", m68k_get_reg(context, M68K_REG_PREF_DATA)); break;
		case CPU_INFO_FLAGS:
			sr = m68k_get_reg(context, M68K_REG_SR);
			sprintf(buffer[which], "%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c",
				sr & 0x8000 ? 'T':'.',
				sr & 0x4000 ? 't':'.',
				sr & 0x2000 ? 'S':'.',
				sr & 0x1000 ? 'M':'.',
				sr & 0x0800 ? '?':'.',
				sr & 0x0400 ? 'I':'.',
				sr & 0x0200 ? 'I':'.',
				sr & 0x0100 ? 'I':'.',
				sr & 0x0080 ? '?':'.',
				sr & 0x0040 ? '?':'.',
				sr & 0x0020 ? '?':'.',
				sr & 0x0010 ? 'X':'.',
				sr & 0x0008 ? 'N':'.',
				sr & 0x0004 ? 'Z':'.',
				sr & 0x0002 ? 'V':'.',
				sr & 0x0001 ? 'C':'.');
			break;
		case CPU_INFO_NAME: return "68EC020";
		case CPU_INFO_FAMILY: return "Motorola 68K";
		case CPU_INFO_VERSION: return "3.2";
		case CPU_INFO_FILE: return __FILE__;
		case CPU_INFO_CREDITS: return "Copyright 1999-2000 Karl Stenerud. All rights reserved. (2.1 fixes HJB)";
		case CPU_INFO_REG_LAYOUT: return (const char*)m68ec020_reg_layout;
		case CPU_INFO_WIN_LAYOUT: return (const char*)m68ec020_win_layout;
	}
	return buffer[which];
}

unsigned m68ec020_dasm(char *buffer, unsigned pc)
{
	M68K_SET_PC_CALLBACK(pc);
#ifdef MAME_DEBUG
	return m68k_disassemble(buffer, pc, M68K_CPU_TYPE_68EC020);
#else
	sprintf( buffer, "$%04X", m68k_read_immediate_16(pc) );
	return 2;
#endif
}
#endif /* HAS_M68EC020 */

/****************************************************************************
 * M68020 section
 ****************************************************************************/
#if HAS_M68020

static UINT8 m68020_reg_layout[] = {
	M68K_PC,  M68K_MSP, -1,
	M68K_SR,  M68K_ISP, -1,
	M68K_SFC, M68K_USP, -1,
	M68K_DFC, M68K_VBR, -1,
	M68K_D0,  M68K_A0, -1,
	M68K_D1,  M68K_A1, -1,
	M68K_D2,  M68K_A2, -1,
	M68K_D3,  M68K_A3, -1,
	M68K_D4,  M68K_A4, -1,
	M68K_D5,  M68K_A5, -1,
	M68K_D6,  M68K_A6, -1,
	M68K_D7,  M68K_A7, 0
};

static UINT8 m68020_win_layout[] = {
	48, 0,32,13,	/* register window (top right) */
	 0, 0,47,13,	/* disassembler window (top left) */
	 0,14,47, 8,	/* memory #1 window (left, middle) */
	48,14,32, 8,	/* memory #2 window (right, middle) */
	 0,23,80, 1 	/* command line window (bottom rows) */
};


void m68020_init(void)
{
	m68k_init();
	m68k_set_cpu_type(M68K_CPU_TYPE_68020);
	m68k_memory_intf = interface_a32_d32;
	m68k_state_register("m68020");
}

void m68020_reset(void* param)
{
	m68k_pulse_reset();
}

void m68020_exit(void)
{
	/* nothing to do */
}

int m68020_execute(int cycles)
{
	return m68k_execute(cycles);
}

unsigned m68020_get_context(void *dst)
{
	return m68k_get_context(dst);
}

void m68020_set_context(void *src)
{
	if (m68k_memory_intf.read8 != cpu_readmem32bedw)
		m68k_memory_intf = interface_a32_d32;
	m68k_set_context(src);
}

unsigned m68020_get_reg(int regnum)
{
	switch( regnum )
	{
		case M68K_MSP: return m68k_get_reg(NULL, M68K_REG_MSP); /* 68020+ */
		case M68K_CACR:  return m68k_get_reg(NULL, M68K_REG_CACR); /* 68020+ */
		case M68K_CAAR:  return m68k_get_reg(NULL, M68K_REG_CAAR); /* 68020+ */
		case M68K_VBR: return m68k_get_reg(NULL, M68K_REG_VBR); /* 68010+ */
		case M68K_SFC: return m68k_get_reg(NULL, M68K_REG_SFC); /* 68010" */
		case M68K_DFC: return m68k_get_reg(NULL, M68K_REG_DFC); /* 68010+ */
		case REG_PC:
		case M68K_PC:  return m68k_get_reg(NULL, M68K_REG_PC);
		case REG_SP:
		case M68K_SP:  return m68k_get_reg(NULL, M68K_REG_SP);
		case M68K_ISP: return m68k_get_reg(NULL, M68K_REG_ISP);
		case M68K_USP: return m68k_get_reg(NULL, M68K_REG_USP);
		case M68K_SR:  return m68k_get_reg(NULL, M68K_REG_SR);
		case M68K_D0:  return m68k_get_reg(NULL, M68K_REG_D0);
		case M68K_D1:  return m68k_get_reg(NULL, M68K_REG_D1);
		case M68K_D2:  return m68k_get_reg(NULL, M68K_REG_D2);
		case M68K_D3:  return m68k_get_reg(NULL, M68K_REG_D3);
		case M68K_D4:  return m68k_get_reg(NULL, M68K_REG_D4);
		case M68K_D5:  return m68k_get_reg(NULL, M68K_REG_D5);
		case M68K_D6:  return m68k_get_reg(NULL, M68K_REG_D6);
		case M68K_D7:  return m68k_get_reg(NULL, M68K_REG_D7);
		case M68K_A0:  return m68k_get_reg(NULL, M68K_REG_A0);
		case M68K_A1:  return m68k_get_reg(NULL, M68K_REG_A1);
		case M68K_A2:  return m68k_get_reg(NULL, M68K_REG_A2);
		case M68K_A3:  return m68k_get_reg(NULL, M68K_REG_A3);
		case M68K_A4:  return m68k_get_reg(NULL, M68K_REG_A4);
		case M68K_A5:  return m68k_get_reg(NULL, M68K_REG_A5);
		case M68K_A6:  return m68k_get_reg(NULL, M68K_REG_A6);
		case M68K_A7:  return m68k_get_reg(NULL, M68K_REG_A7);
		case M68K_PREF_ADDR:  return m68k_get_reg(NULL, M68K_REG_PREF_ADDR);
		case M68K_PREF_DATA:  return m68k_get_reg(NULL, M68K_REG_PREF_DATA);
		case REG_PREVIOUSPC: return m68k_get_reg(NULL, M68K_REG_PPC);
/* TODO: return contents of [SP + wordsize * (REG_SP_CONTENTS-regnum)] */
		default:
			if( regnum < REG_SP_CONTENTS )
			{
				unsigned offset = m68k_get_reg(NULL, M68K_REG_SP) + 4 * (REG_SP_CONTENTS - regnum);
				if( offset < 0xfffffd )
					return m68k_read_memory_32( offset );
			}
	}
	return 0;
}

void m68020_set_reg(int regnum, unsigned val)
{
	switch( regnum )
	{
		case M68K_MSP:  m68k_set_reg(M68K_REG_MSP, val); break; /* 68020+ */
		case M68K_CACR: m68k_set_reg(M68K_REG_CACR, val); break; /* 68020+ */
		case M68K_CAAR: m68k_set_reg(M68K_REG_CAAR, val); break; /* 68020+ */
		case M68K_VBR: m68k_set_reg(M68K_REG_VBR, val); break; /* 68010+ */
		case M68K_SFC: m68k_set_reg(M68K_REG_SFC, val); break; /* 68010+ */
		case M68K_DFC: m68k_set_reg(M68K_REG_DFC, val); break; /* 68010+ */
		case REG_PC:
		case M68K_PC:  m68k_set_reg(M68K_REG_PC, val); break;
		case REG_SP:
		case M68K_SP:  m68k_set_reg(M68K_REG_SP, val); break;
		case M68K_ISP: m68k_set_reg(M68K_REG_ISP, val); break;
		case M68K_USP: m68k_set_reg(M68K_REG_USP, val); break;
		case M68K_SR:  m68k_set_reg(M68K_REG_SR, val); break;
		case M68K_D0:  m68k_set_reg(M68K_REG_D0, val); break;
		case M68K_D1:  m68k_set_reg(M68K_REG_D1, val); break;
		case M68K_D2:  m68k_set_reg(M68K_REG_D2, val); break;
		case M68K_D3:  m68k_set_reg(M68K_REG_D3, val); break;
		case M68K_D4:  m68k_set_reg(M68K_REG_D4, val); break;
		case M68K_D5:  m68k_set_reg(M68K_REG_D5, val); break;
		case M68K_D6:  m68k_set_reg(M68K_REG_D6, val); break;
		case M68K_D7:  m68k_set_reg(M68K_REG_D7, val); break;
		case M68K_A0:  m68k_set_reg(M68K_REG_A0, val); break;
		case M68K_A1:  m68k_set_reg(M68K_REG_A1, val); break;
		case M68K_A2:  m68k_set_reg(M68K_REG_A2, val); break;
		case M68K_A3:  m68k_set_reg(M68K_REG_A3, val); break;
		case M68K_A4:  m68k_set_reg(M68K_REG_A4, val); break;
		case M68K_A5:  m68k_set_reg(M68K_REG_A5, val); break;
		case M68K_A6:  m68k_set_reg(M68K_REG_A6, val); break;
		case M68K_A7:  m68k_set_reg(M68K_REG_A7, val); break;
/* TODO: set contents of [SP + wordsize * (REG_SP_CONTENTS-regnum)] */
		default:
			if( regnum < REG_SP_CONTENTS )
			{
				unsigned offset = m68k_get_reg(NULL, M68K_REG_SP) + 4 * (REG_SP_CONTENTS - regnum);
				if( offset < 0xfffffd )
					m68k_write_memory_16( offset, val );
			}
	}
}

void m68020_set_irq_line(int irqline, int state)
{
	if (irqline == IRQ_LINE_NMI)
		irqline = 7;
	switch(state)
	{
		case CLEAR_LINE:
			m68k_set_irq(0);
			break;
		case ASSERT_LINE:
			m68k_set_irq(irqline);
			break;
		default:
			m68k_set_irq(irqline);
			break;
	}
}

void m68020_set_irq_callback(int (*callback)(int irqline))
{
	m68k_set_int_ack_callback(callback);
}

const char *m68020_info(void *context, int regnum)
{
	static char buffer[32][47+1];
	static int which = 0;
	int sr;

	which = (which+1) % 32;
	buffer[which][0] = '\0';

	switch( regnum )
	{
		case CPU_INFO_REG+M68K_MSP:  sprintf(buffer[which], "MSP:%08X", m68k_get_reg(context, M68K_REG_MSP)); break;
		case CPU_INFO_REG+M68K_CACR: sprintf(buffer[which], "CCR:%08X", m68k_get_reg(context, M68K_REG_CACR)); break;
		case CPU_INFO_REG+M68K_CAAR: sprintf(buffer[which], "CAR:%08X", m68k_get_reg(context, M68K_REG_CAAR)); break;
		case CPU_INFO_REG+M68K_SFC: sprintf(buffer[which], "SFC:%X",   m68k_get_reg(context, M68K_REG_SFC)); break;
		case CPU_INFO_REG+M68K_DFC: sprintf(buffer[which], "DFC:%X",   m68k_get_reg(context, M68K_REG_DFC)); break;
		case CPU_INFO_REG+M68K_VBR: sprintf(buffer[which], "VBR:%08X", m68k_get_reg(context, M68K_REG_VBR)); break;
		case CPU_INFO_REG+M68K_PC:	sprintf(buffer[which], "PC :%08X", m68k_get_reg(context, M68K_REG_PC)); break;
		case CPU_INFO_REG+M68K_SR:  sprintf(buffer[which], "SR :%04X", m68k_get_reg(context, M68K_REG_SR)); break;
		case CPU_INFO_REG+M68K_SP:  sprintf(buffer[which], "SP :%08X", m68k_get_reg(context, M68K_REG_SP)); break;
		case CPU_INFO_REG+M68K_ISP: sprintf(buffer[which], "ISP:%08X", m68k_get_reg(context, M68K_REG_ISP)); break;
		case CPU_INFO_REG+M68K_USP: sprintf(buffer[which], "USP:%08X", m68k_get_reg(context, M68K_REG_USP)); break;
		case CPU_INFO_REG+M68K_D0:	sprintf(buffer[which], "D0 :%08X", m68k_get_reg(context, M68K_REG_D0)); break;
		case CPU_INFO_REG+M68K_D1:	sprintf(buffer[which], "D1 :%08X", m68k_get_reg(context, M68K_REG_D1)); break;
		case CPU_INFO_REG+M68K_D2:	sprintf(buffer[which], "D2 :%08X", m68k_get_reg(context, M68K_REG_D2)); break;
		case CPU_INFO_REG+M68K_D3:	sprintf(buffer[which], "D3 :%08X", m68k_get_reg(context, M68K_REG_D3)); break;
		case CPU_INFO_REG+M68K_D4:	sprintf(buffer[which], "D4 :%08X", m68k_get_reg(context, M68K_REG_D4)); break;
		case CPU_INFO_REG+M68K_D5:	sprintf(buffer[which], "D5 :%08X", m68k_get_reg(context, M68K_REG_D5)); break;
		case CPU_INFO_REG+M68K_D6:	sprintf(buffer[which], "D6 :%08X", m68k_get_reg(context, M68K_REG_D6)); break;
		case CPU_INFO_REG+M68K_D7:	sprintf(buffer[which], "D7 :%08X", m68k_get_reg(context, M68K_REG_D7)); break;
		case CPU_INFO_REG+M68K_A0:	sprintf(buffer[which], "A0 :%08X", m68k_get_reg(context, M68K_REG_A0)); break;
		case CPU_INFO_REG+M68K_A1:	sprintf(buffer[which], "A1 :%08X", m68k_get_reg(context, M68K_REG_A1)); break;
		case CPU_INFO_REG+M68K_A2:	sprintf(buffer[which], "A2 :%08X", m68k_get_reg(context, M68K_REG_A2)); break;
		case CPU_INFO_REG+M68K_A3:	sprintf(buffer[which], "A3 :%08X", m68k_get_reg(context, M68K_REG_A3)); break;
		case CPU_INFO_REG+M68K_A4:	sprintf(buffer[which], "A4 :%08X", m68k_get_reg(context, M68K_REG_A4)); break;
		case CPU_INFO_REG+M68K_A5:	sprintf(buffer[which], "A5 :%08X", m68k_get_reg(context, M68K_REG_A5)); break;
		case CPU_INFO_REG+M68K_A6:	sprintf(buffer[which], "A6 :%08X", m68k_get_reg(context, M68K_REG_A6)); break;
		case CPU_INFO_REG+M68K_A7:	sprintf(buffer[which], "A7 :%08X", m68k_get_reg(context, M68K_REG_A7)); break;
		case CPU_INFO_REG+M68K_PREF_ADDR:	sprintf(buffer[which], "PAR:%08X", m68k_get_reg(context, M68K_REG_PREF_ADDR)); break;
		case CPU_INFO_REG+M68K_PREF_DATA:	sprintf(buffer[which], "PDA:%08X", m68k_get_reg(context, M68K_REG_PREF_DATA)); break;
		case CPU_INFO_FLAGS:
			sr = m68k_get_reg(context, M68K_REG_SR);
			sprintf(buffer[which], "%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c",
				sr & 0x8000 ? 'T':'.',
				sr & 0x4000 ? 't':'.',
				sr & 0x2000 ? 'S':'.',
				sr & 0x1000 ? 'M':'.',
				sr & 0x0800 ? '?':'.',
				sr & 0x0400 ? 'I':'.',
				sr & 0x0200 ? 'I':'.',
				sr & 0x0100 ? 'I':'.',
				sr & 0x0080 ? '?':'.',
				sr & 0x0040 ? '?':'.',
				sr & 0x0020 ? '?':'.',
				sr & 0x0010 ? 'X':'.',
				sr & 0x0008 ? 'N':'.',
				sr & 0x0004 ? 'Z':'.',
				sr & 0x0002 ? 'V':'.',
				sr & 0x0001 ? 'C':'.');
			break;
		case CPU_INFO_NAME: return "68020";
		case CPU_INFO_FAMILY: return "Motorola 68K";
		case CPU_INFO_VERSION: return "3.2";
		case CPU_INFO_FILE: return __FILE__;
		case CPU_INFO_CREDITS: return "Copyright 1999-2000 Karl Stenerud. All rights reserved. (2.1 fixes HJB)";
		case CPU_INFO_REG_LAYOUT: return (const char*)m68020_reg_layout;
		case CPU_INFO_WIN_LAYOUT: return (const char*)m68020_win_layout;
	}
	return buffer[which];
}

unsigned m68020_dasm(char *buffer, unsigned pc)
{
	M68K_SET_PC_CALLBACK(pc);
#ifdef MAME_DEBUG
	return m68k_disassemble(buffer, pc, M68K_CPU_TYPE_68020);
#else
	sprintf( buffer, "$%04X", m68k_read_immediate_16(pc) );
	return 2;
#endif
}
#endif /* HAS_M68020 */

#endif /* A68K2 */
