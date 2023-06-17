#include "driver.h"
#include "cpuintrf.h"
#include "state.h"
#include "drz80_z80.h"
#include "drz80.h"

typedef struct {
	struct DrZ80 regs;
	unsigned int nmi_state;
	unsigned int irq_state;
	int previouspc;
	int (*MAMEIrqCallback)(int int_level);
} drz80_regs;

static drz80_regs DRZ80;
int drz80_ICount;

#define INT_IRQ 0x01
#define NMI_IRQ 0x02

#ifdef __cplusplus
extern "C" {
#endif
void Interrupt(void)
{
	if (DRZ80.regs.Z80_IRQ&NMI_IRQ)
	{
		DRZ80.previouspc=0xffffffff;
	}
	else if ((DRZ80.irq_state!=CLEAR_LINE) && (DRZ80.regs.Z80IF&1))
	{
		DRZ80.previouspc=0xffffffff;
		DRZ80.regs.Z80_IRQ=DRZ80.regs.Z80_IRQ|INT_IRQ;
		DRZ80.regs.z80irqvector=(*DRZ80.MAMEIrqCallback)(0);
	}
}
#ifdef __cplusplus
} /* End of extern "C" */
#endif

static unsigned int drz80_rebasePC(unsigned short address)
{
	change_pc16(address);
	DRZ80.regs.Z80PC_BASE = (unsigned int) OP_ROM;
	DRZ80.regs.Z80PC = DRZ80.regs.Z80PC_BASE + address;
	return (DRZ80.regs.Z80PC);
}

static unsigned int drz80_rebaseSP(unsigned short address)
{
	change_pc16(address);	
	DRZ80.regs.Z80SP_BASE=(unsigned int) OP_ROM;
	DRZ80.regs.Z80SP=DRZ80.regs.Z80SP_BASE + address;
	return (DRZ80.regs.Z80SP);
}

static void drz80_irq_callback(void)
{
	if (DRZ80.regs.Z80_IRQ&NMI_IRQ)
		DRZ80.regs.Z80_IRQ=DRZ80.regs.Z80_IRQ&(~NMI_IRQ);
	else
		DRZ80.regs.Z80_IRQ=DRZ80.regs.Z80_IRQ&(~INT_IRQ);
	DRZ80.previouspc=0;
}

static unsigned char drz80_read8(unsigned short Addr)
{
	return ((cpu_readmem16(Addr))&0xff);
}

static unsigned short drz80_read16(unsigned short Addr)
{
	return (((cpu_readmem16(Addr))&0xff)|((cpu_readmem16(Addr+1)&0xff)<<8));
}

static void drz80_write8(unsigned char Value,unsigned short Addr)
{
	cpu_writemem16(Addr,Value);
}

static void drz80_write16(unsigned short Value,unsigned short Addr)
{
	cpu_writemem16(Addr,(unsigned char)Value&0xff);
	cpu_writemem16(Addr+1,((unsigned char)(Value>>8))&0xff);
}

static unsigned char drz80_in(unsigned short Addr)
{
	return ((cpu_readport16(Addr))&0xff);
}

static void drz80_out(unsigned short Addr,unsigned char Value)
{
	cpu_writeport16(Addr,Value);
}

void drz80_init(void)
{

}

void drz80_reset(void *param)
{
	memset (&DRZ80, 0, sizeof(drz80_regs));
  	DRZ80.regs.z80_rebasePC=drz80_rebasePC;
  	DRZ80.regs.z80_rebaseSP=drz80_rebaseSP;
  	DRZ80.regs.z80_read8   =drz80_read8;
  	DRZ80.regs.z80_read16  =drz80_read16;
  	DRZ80.regs.z80_write8  =drz80_write8;
  	DRZ80.regs.z80_write16 =drz80_write16;
  	DRZ80.regs.z80_in      =drz80_in;
  	DRZ80.regs.z80_out     =drz80_out;
  	DRZ80.regs.z80_irq_callback=drz80_irq_callback;
  	DRZ80.regs.Z80IX = 0xffff<<16;
  	DRZ80.regs.Z80IY = 0xffff<<16;
  	DRZ80.regs.Z80F = (1<<2); /* set ZFlag */
  	DRZ80.nmi_state=CLEAR_LINE;
  	DRZ80.irq_state=CLEAR_LINE;
	/*DRZ80.MAMEIrqCallback=NULL;*/
	DRZ80.previouspc=0;
  	DRZ80.regs.Z80SP=DRZ80.regs.z80_rebaseSP(0xf000);
  	DRZ80.regs.Z80PC=DRZ80.regs.z80_rebasePC(0);
}

void drz80_exit(void)
{
}

int drz80_execute(int cycles)
{
	DRZ80.regs.cycles = cycles;
	drz80_ICount = DRZ80.regs.cycles;
	DrZ80Run(&DRZ80.regs, cycles);
	change_pc16(DRZ80.regs.Z80PC - DRZ80.regs.Z80PC_BASE);
	drz80_ICount = DRZ80.regs.cycles;
	return (cycles-DRZ80.regs.cycles);
}

void drz80_burn(int cycles)
{
	if( cycles > 0 )
	{
		/* NOP takes 4 cycles per instruction */
		int n = (cycles + 3) / 4;
		//DRZ80.regs.Z80R += n;
		DRZ80.regs.cycles -= 4 * n;
		drz80_ICount = DRZ80.regs.cycles;
	}
}

unsigned drz80_get_context (void *dst)
{
	if (dst)
		memcpy (dst,&DRZ80,sizeof (drz80_regs));
	return sizeof(drz80_regs);
}

void drz80_set_context (void *src)
{
	if (src)
		memcpy (&DRZ80, src, sizeof (drz80_regs));
	change_pc16(DRZ80.regs.Z80PC - DRZ80.regs.Z80PC_BASE);
}

const void *drz80_get_cycle_table (int which)
{
	return NULL;
}

void drz80_set_cycle_table (int which, void *new_table)
{
}

unsigned drz80_get_pc (void)
{
    	return (DRZ80.regs.Z80PC - DRZ80.regs.Z80PC_BASE);
}

void drz80_set_pc (unsigned val)
{
	DRZ80.regs.Z80PC=drz80_rebasePC(val);
}

unsigned drz80_get_sp (void)
{
    	return (DRZ80.regs.Z80SP - DRZ80.regs.Z80SP_BASE);
}

void drz80_set_sp (unsigned val)
{
	DRZ80.regs.Z80SP=drz80_rebaseSP(val);
}

enum {
	Z80_PC=1, Z80_SP, Z80_AF, Z80_BC, Z80_DE, Z80_HL,
	Z80_IX, Z80_IY,	Z80_AF2, Z80_BC2, Z80_DE2, Z80_HL2,
	Z80_R, Z80_I, Z80_IM, Z80_IFF1, Z80_IFF2, Z80_HALT,
	Z80_NMI_STATE, Z80_IRQ_STATE, Z80_DC0, Z80_DC1, Z80_DC2, Z80_DC3
};

unsigned drz80_get_reg (int regnum)
{
	switch( regnum )
	{
		case Z80_PC: return drz80_get_pc();
		case Z80_SP: return drz80_get_sp();
		case Z80_AF: return ((DRZ80.regs.Z80A>>16) | (DRZ80.regs.Z80F>>24));
		case Z80_BC: return (DRZ80.regs.Z80BC>>16);
		case Z80_DE: return (DRZ80.regs.Z80DE>>16);
		case Z80_HL: return (DRZ80.regs.Z80HL>>16);
		case Z80_IX: return (DRZ80.regs.Z80IX>>16);
		case Z80_IY: return (DRZ80.regs.Z80IY>>16);
        case Z80_R: return DRZ80.regs.Z80R; /*???*/
		case Z80_I: return DRZ80.regs.Z80I;
		case Z80_AF2: return ((DRZ80.regs.Z80A2>>16) | (DRZ80.regs.Z80F2>>24));
		case Z80_BC2: return (DRZ80.regs.Z80BC2>>16);
		case Z80_DE2: return (DRZ80.regs.Z80DE2>>16);
		case Z80_HL2: return (DRZ80.regs.Z80HL2>>16);
		case Z80_IM: return DRZ80.regs.Z80IM;
		case Z80_IFF1: return ((DRZ80.regs.Z80IF&1)!=0);
		case Z80_IFF2: return ((DRZ80.regs.Z80IF&2)!=0);
		case Z80_HALT: return ((DRZ80.regs.Z80IF&4)!=0);
		case Z80_NMI_STATE: return DRZ80.nmi_state;
		case Z80_IRQ_STATE: return DRZ80.irq_state;
		case Z80_DC0: return 0; /* daisy chain */
		case Z80_DC1: return 0; /* daisy chain */
		case Z80_DC2: return 0; /* daisy chain */
		case Z80_DC3: return 0; /* daisy chain */
        case REG_PREVIOUSPC: return (DRZ80.previouspc==0xffffffff?0xffffffff:drz80_get_pc());
		default:
			if( regnum <= REG_SP_CONTENTS )
			{
				unsigned offset = drz80_get_sp() + 2 * (REG_SP_CONTENTS - regnum);
				if( offset < 0xffff )
					return drz80_read16(offset);
			}
	}
    	return 0;
}

void drz80_set_reg (int regnum, unsigned val)
{
	switch( regnum )
	{
		case Z80_PC: drz80_set_pc(val); break;
		case Z80_SP: drz80_set_sp(val); break;
		case Z80_AF: DRZ80.regs.Z80A=((val&0xff00)<<16); DRZ80.regs.Z80F=((val&0x00ff)<<24); break;
		case Z80_BC: DRZ80.regs.Z80BC=(val<<16); break;
		case Z80_DE: DRZ80.regs.Z80DE=(val<<16); break;
		case Z80_HL: DRZ80.regs.Z80HL=(val<<16); break;
		case Z80_IX: DRZ80.regs.Z80IX=(val<<16); break;
		case Z80_IY: DRZ80.regs.Z80IY=(val<<16); break;
        case Z80_R: DRZ80.regs.Z80R=val; break; /*???*/
		case Z80_I: DRZ80.regs.Z80I = val; break;
		case Z80_AF2: DRZ80.regs.Z80A2=((val&0xff00)<<16); DRZ80.regs.Z80F2=((val&0x00ff)<<24); break;
		case Z80_BC2: DRZ80.regs.Z80BC2=(val<<16); break;
		case Z80_DE2: DRZ80.regs.Z80DE2=(val<<16); break;
		case Z80_HL2: DRZ80.regs.Z80HL2=(val<<16); break;
		case Z80_IM: DRZ80.regs.Z80IM = val; break;
		case Z80_IFF1: DRZ80.regs.Z80IF=(DRZ80.regs.Z80IF)&(~(val==0)); break;
		case Z80_IFF2: DRZ80.regs.Z80IF=(DRZ80.regs.Z80IF)&(~((val==0)<<1)); break;
		case Z80_HALT: DRZ80.regs.Z80IF=(DRZ80.regs.Z80IF)&(~((val==0)<<2)); break;
		case Z80_NMI_STATE: drz80_set_irq_line(IRQ_LINE_NMI,val); break;
		case Z80_IRQ_STATE: drz80_set_irq_line(0,val); break;
		case Z80_DC0: break; /* daisy chain */
		case Z80_DC1: break; /* daisy chain */
		case Z80_DC2: break; /* daisy chain */
		case Z80_DC3: break; /* daisy chain */
        	default:
			if( regnum <= REG_SP_CONTENTS )
			{
				unsigned offset = drz80_get_sp() + 2 * (REG_SP_CONTENTS - regnum);
				if( offset < 0xffff )
					drz80_write16(val,offset);
			}
    	}
}

void drz80_set_irq_line(int irqline, int state)
{
	if(irqline == IRQ_LINE_NMI){
		DRZ80.nmi_state=state;
		if (state!=CLEAR_LINE)
			DRZ80.regs.Z80_IRQ=DRZ80.regs.Z80_IRQ|NMI_IRQ;
	} else {
		DRZ80.irq_state=state;
	}
}

void drz80_set_irq_callback(int (*callback)(int))
{
	DRZ80.MAMEIrqCallback=callback;
}

void drz80_state_save(void *file)
{
}

void drz80_state_load(void *file)
{
}

const char *drz80_info(void *context, int regnum)
{
	switch( regnum )
	{
		case CPU_INFO_NAME: return "DRZ80 Z80";
        case CPU_INFO_FAMILY: return "Zilog Z80";
		case CPU_INFO_VERSION: return "1.0";
		case CPU_INFO_FILE: return __FILE__;
		case CPU_INFO_CREDITS: return "Copyright 2005 Reesy, all rights reserved.";
	}
	return "";
}

unsigned drz80_dasm( char *buffer, unsigned pc )
{
	sprintf( buffer, "$%02X", cpu_readop(pc) );
	return 1;
}
