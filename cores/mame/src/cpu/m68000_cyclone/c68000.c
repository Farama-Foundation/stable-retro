#include "c68000.h"
#include "driver.h"
#include "cpuintrf.h"
#include "cyclone.h"
#include "memory.h"


typedef struct
{
	struct Cyclone regs;
	int pending_interrupts;
	int (*MAMEIrqCallback)(int int_level);
} Cyclone_Regs;

static Cyclone_Regs cyclone;
int cyclone_ICount;

static unsigned int MyCheckPc(unsigned int pc)
{
  	pc = (pc-cyclone.regs.membase) & 0xffffff; /* Get the real program counter */
	change_pc24bew(pc);
	cyclone.regs.membase=(unsigned int)OP_ROM;
  	return cyclone.regs.membase+pc; /* New program counter */
}

static void update_irq_line(void)
{
	if (cyclone.pending_interrupts & 0xff000000)
	{
		int level, mask = 0x80000000;
		for (level = 7; level; level--, mask >>= 1)
			if (cyclone.pending_interrupts & mask)
				break;
		cyclone.regs.irq = level;
	}
	else
	{
		cyclone.regs.irq = 0;
	}
}

static int MyIrqCallback(int irq)
{
	cyclone.pending_interrupts &= ~(1 << (24 + irq));
	if (cyclone.MAMEIrqCallback)
		cyclone.MAMEIrqCallback(irq);
	update_irq_line();
	return CYCLONE_INT_ACK_AUTOVECTOR;
}

static void cyclone_Cause_Interrupt(int level)
{
	if (level >= 1 && level <= 7)
		cyclone.pending_interrupts |= 1 << (24 + level);
	update_irq_line();
}

static void cyclone_Clear_Interrupt (int level)
{
	if (level >= 1 && level <= 7)
		cyclone.pending_interrupts &= ~(1 << (24 + level));
	update_irq_line();
}

static data32_t cpu_readmem24bew_dword(offs_t address)
{
	data32_t result = cpu_readmem24bew_word(address) << 16;
	return result | cpu_readmem24bew_word(address + 2);
}

static void cpu_writemem24bew_dword(offs_t address, data32_t data)
{
	cpu_writemem24bew_word(address, data >> 16);
	cpu_writemem24bew_word(address + 2, data);
}

void cyclone_init(void)
{
   	CycloneInit();
}

void cyclone_reset(void *param)
{
	memset(&cyclone, 0,sizeof(Cyclone_Regs));
	cyclone.regs.checkpc=MyCheckPc;
	cyclone.regs.read8  = (unsigned int (*)(unsigned int)) cpu_readmem24bew;
	cyclone.regs.read16 = (unsigned int (*)(unsigned int)) cpu_readmem24bew_word;
	cyclone.regs.read32 = (unsigned int (*)(unsigned int)) cpu_readmem24bew_dword;
	cyclone.regs.write8 = (void (*)(unsigned int, unsigned char)) cpu_writemem24bew;
	cyclone.regs.write16= (void (*)(unsigned int, unsigned short)) cpu_writemem24bew_word;
	cyclone.regs.write32= (void (*)(unsigned int, unsigned int)) cpu_writemem24bew_dword;
	cyclone.regs.fetch8  = (unsigned int (*)(unsigned int)) cpu_readmem24bew;
	cyclone.regs.fetch16 = (unsigned int (*)(unsigned int)) cpu_readmem24bew_word;
	cyclone.regs.fetch32 = (unsigned int (*)(unsigned int)) cpu_readmem24bew_dword;
	cyclone.regs.IrqCallback=MyIrqCallback; /* 0 */
  	cyclone.regs.srh=0x27; /* Set supervisor mode */
  	cyclone.regs.a[7]=cyclone.regs.read32(0); /* Get Stack Pointer */
  	cyclone.regs.membase=0;
  	cyclone.regs.pc=cyclone.regs.checkpc(cyclone.regs.read32(4)); /* Get Program Counter */
	cyclone.regs.state_flags = 0; /* not stopped or tracing */
   	cyclone.pending_interrupts = 0;
	CycloneReset(&cyclone.regs);
}

unsigned int cyclone_get_pc(void)
{
	return (cyclone.regs.pc - cyclone.regs.membase) & 0xffffff;
}

void cyclone_set_context(void *src)
{
	if (src)
	{
		memcpy(&cyclone,src,sizeof(Cyclone_Regs));
	}
	update_irq_line();
}

unsigned cyclone_get_context(void *dst)
{
	if (dst)
	{
		memcpy(dst,&cyclone,sizeof(Cyclone_Regs));
	}
	return sizeof(Cyclone_Regs);

}

int cyclone_execute(int cycles)
{
	cyclone.regs.cycles = cycles;
	cyclone_ICount = cyclone.regs.cycles;
	CycloneRun(&cyclone.regs);
	cyclone_ICount = cyclone.regs.cycles;
   	return (cycles - cyclone.regs.cycles);
}

void cyclone_exit(void)
{
}

void cyclone_set_pc(unsigned val)
{
	cyclone.regs.pc=MyCheckPc(val);
}
unsigned cyclone_get_sp(void)
{
	return cyclone.regs.a[7];
}

void cyclone_set_sp(unsigned val)
{
	cyclone.regs.a[7] = val;
}

enum
{
	/* NOTE: M68K_SP fetches the current SP, be it USP, ISP, or MSP */
	M68K_PC=1, M68K_SP, M68K_ISP, M68K_USP, M68K_MSP, M68K_SR, M68K_VBR,
	M68K_SFC, M68K_DFC, M68K_CACR, M68K_CAAR, M68K_PREF_ADDR, M68K_PREF_DATA,
	M68K_D0, M68K_D1, M68K_D2, M68K_D3, M68K_D4, M68K_D5, M68K_D6, M68K_D7,
	M68K_A0, M68K_A1, M68K_A2, M68K_A3, M68K_A4, M68K_A5, M68K_A6, M68K_A7
};

unsigned cyclone_get_reg(int regnum)
{
    switch( regnum )
    {
		case M68K_PC: return cyclone_get_pc();
		case M68K_SP: return cyclone_get_sp();
		case M68K_ISP: return cyclone.regs.osp;
		case M68K_USP: return cyclone.regs.osp;
		case M68K_SR: return CycloneGetSr(&cyclone.regs);
		case M68K_D0: return cyclone.regs.d[0];
		case M68K_D1: return cyclone.regs.d[1];
		case M68K_D2: return cyclone.regs.d[2];
		case M68K_D3: return cyclone.regs.d[3];
		case M68K_D4: return cyclone.regs.d[4];
		case M68K_D5: return cyclone.regs.d[5];
		case M68K_D6: return cyclone.regs.d[6];
		case M68K_D7: return cyclone.regs.d[7];
		case M68K_A0: return cyclone.regs.a[0];
		case M68K_A1: return cyclone.regs.a[1];
		case M68K_A2: return cyclone.regs.a[2];
		case M68K_A3: return cyclone.regs.a[3];
		case M68K_A4: return cyclone.regs.a[4];
		case M68K_A5: return cyclone.regs.a[5];
		case M68K_A6: return cyclone.regs.a[6];
		case M68K_A7: return cyclone.regs.a[7];
		case REG_PREVIOUSPC: return (cyclone.regs.prev_pc - cyclone.regs.membase - 2) & 0xffffff;
		default:
			if( regnum < REG_SP_CONTENTS )
			{
				unsigned offset = cyclone_get_sp() + 4 * (REG_SP_CONTENTS - regnum);
				if( offset < 0xfffffd )
					return cpu_readmem24bew_dword( offset );
            }
    }
    return 0;
}

void cyclone_set_reg(int regnum, unsigned val)
{
    switch( regnum )
    {
		case M68K_PC: cyclone_set_pc(val); break;
		case M68K_SP: cyclone_set_sp(val); break;
		case M68K_ISP: cyclone.regs.osp = val; break;
		case M68K_USP: cyclone.regs.osp = val; break;
		case M68K_SR: CycloneSetSr (&cyclone.regs,val); break;
		case M68K_D0: cyclone.regs.d[0] = val; break;
		case M68K_D1: cyclone.regs.d[1] = val; break;
		case M68K_D2: cyclone.regs.d[2] = val; break;
		case M68K_D3: cyclone.regs.d[3] = val; break;
		case M68K_D4: cyclone.regs.d[4] = val; break;
		case M68K_D5: cyclone.regs.d[5] = val; break;
		case M68K_D6: cyclone.regs.d[6] = val; break;
		case M68K_D7: cyclone.regs.d[7] = val; break;
		case M68K_A0: cyclone.regs.a[0] = val; break;
		case M68K_A1: cyclone.regs.a[1] = val; break;
		case M68K_A2: cyclone.regs.a[2] = val; break;
		case M68K_A3: cyclone.regs.a[3] = val; break;
		case M68K_A4: cyclone.regs.a[4] = val; break;
		case M68K_A5: cyclone.regs.a[5] = val; break;
		case M68K_A6: cyclone.regs.a[6] = val; break;
		case M68K_A7: cyclone.regs.a[7] = val; break;
		default:
			if( regnum < REG_SP_CONTENTS )
			{
				unsigned offset = cyclone_get_sp() + 4 * (REG_SP_CONTENTS - regnum);
				if( offset < 0xfffffd )
					cpu_writemem24bew_word( offset, val );
            }
    }
}

void cyclone_set_irq_line(int irqline, int state)
{
	if (irqline == IRQ_LINE_NMI)
		irqline = 7;
	switch(state)
	{
		case CLEAR_LINE:
			cyclone_Clear_Interrupt(irqline);
			return;
		case ASSERT_LINE:
			cyclone_Cause_Interrupt(irqline);
			return;
		default:
			cyclone_Cause_Interrupt(irqline);
			return;
	}
}

void cyclone_set_irq_callback(int (*callback)(int irqline))
{
	cyclone.MAMEIrqCallback = callback;
}

const char *cyclone_info(void *context, int regnum)
{
	switch( regnum )
	{
		case CPU_INFO_NAME: return "Cyclone 68000";
		case CPU_INFO_FAMILY: return "Motorola 68K";
		case CPU_INFO_VERSION: return "v0.0088";
		case CPU_INFO_FILE: return __FILE__;
		case CPU_INFO_CREDITS: return "Copyright Copyright 2004-2007 Dave, Reesy and Notaz. All rights reserved";
	}
	return "";
}

unsigned cyclone_dasm(char *buffer, unsigned pc)
{
	change_pc24(pc);
	sprintf(buffer, "$%04X", cpu_readop16(pc) );
	return 2;
}
