/*
	Interface routine for 68kem <-> Mame
*/

#include "driver.h"
#include "mamedbg.h"
#include "m68000.h"
#include "state.h"

struct m68k_memory_interface a68k_memory_intf;

/* If we are only using assembler cores, we need to define these
 * otherwise they are declared by the C core. */

#ifdef A68K0
#ifdef A68K2
int m68k_ICount;
struct m68k_memory_interface m68k_memory_intf;
#endif
#endif

enum
{
	M68K_CPU_TYPE_INVALID,
	M68K_CPU_TYPE_68000,
	M68K_CPU_TYPE_68010,
	M68K_CPU_TYPE_68EC020,
	M68K_CPU_TYPE_68020,
	M68K_CPU_TYPE_68030,	/* Supported by disassembler ONLY */
	M68K_CPU_TYPE_68040		/* Supported by disassembler ONLY */
};

#define A68K_SET_PC_CALLBACK(A)     (*a68k_memory_intf.changepc)(A)

int illegal_op = 0 ;
int illegal_pc = 0 ;

#ifdef MAME_DEBUG
void m68k_illegal_opcode(void)
{
	log_cb(RETRO_LOG_DEBUG, LOGPRE "Illegal Opcode %4x at %8x\n",illegal_op,illegal_pc);
}
#endif

unsigned int m68k_disassemble(char* str_buff, unsigned int pc, unsigned int cpu_type);

#ifdef _WIN32
#define CONVENTION __cdecl
#else
#define CONVENTION
#endif

/* Use the x86 assembly core */
typedef struct
{
    UINT32 d[8];             /* 0x0004 8 Data registers */
    UINT32 a[8];             /* 0x0024 8 Address registers */

    UINT32 isp;              /* 0x0048 */

    UINT32 sr_high;          /* 0x004C System registers */
    UINT32 ccr;              /* 0x0050 CCR in Intel Format */
    UINT32 x_carry;          /* 0x0054 Extended Carry */

    UINT32 pc;               /* 0x0058 Program Counter */

    UINT32 IRQ_level;        /* 0x005C IRQ level you want the MC68K process (0=None)  */

    /* Backward compatible with C emulator - Only set in Debug compile */

    UINT16 sr;
    UINT16 filler;

    int (*irq_callback)(int irqline);

    UINT32 previous_pc;      /* last PC used */

    int (*reset_callback)(void);

    UINT32 sfc;              /* Source Function Code. (68010) */
    UINT32 dfc;              /* Destination Function Code. (68010) */
    UINT32 usp;              /* User Stack (All) */
    UINT32 vbr;              /* Vector Base Register. (68010) */

    UINT32 BankID;			 /* Memory bank in use */
    UINT32 CPUtype;		  	 /* CPU Type 0=68000,1=68010,2=68020 */
	UINT32 FullPC;

	struct m68k_memory_interface Memory_Interface;

} a68k_cpu_context;


static UINT8 M68K_layout[] = {
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

static UINT8 m68k_win_layout[] = {
	48, 0,32,13,	/* register window (top right) */
	 0, 0,47,13,	/* disassembler window (top left) */
	 0,14,47, 8,	/* memory #1 window (left, middle) */
	48,14,32, 8,	/* memory #2 window (right, middle) */
	 0,23,80, 1 	/* command line window (bottom rows) */
};

#ifdef A68K0
extern a68k_cpu_context M68000_regs;

extern void CONVENTION M68000_RUN(void);
extern void CONVENTION M68000_RESET(void);

#endif

#ifdef A68K2
extern a68k_cpu_context M68020_regs;

extern void CONVENTION M68020_RUN(void);
extern void CONVENTION M68020_RESET(void);
#endif

/***************************************************************************/
/* Save State stuff                                                        */
/***************************************************************************/

static int IntelFlag[32] = {
	0x0000,0x0001,0x0800,0x0801,0x0040,0x0041,0x0840,0x0841,
    0x0080,0x0081,0x0880,0x0881,0x00C0,0x00C1,0x08C0,0x08C1,
    0x0100,0x0101,0x0900,0x0901,0x0140,0x0141,0x0940,0x0941,
    0x0180,0x0181,0x0980,0x0981,0x01C0,0x01C1,0x09C0,0x09C1
};


/* The assembler engine only keeps flags in intel format, so ... */

static UINT32 zero = 0;
static int stopped = 0;

static void a68k_prepare_substate(void)
{
	stopped = ((M68000_regs.IRQ_level & 0x80) != 0);

	M68000_regs.sr = ((M68000_regs.ccr >> 4) & 0x1C)
                   | (M68000_regs.ccr & 0x01)
                   | ((M68000_regs.ccr >> 10) & 0x02)
                   | (M68000_regs.sr_high << 8);
}

static void a68k_post_load(void)
{
	int intel = M68000_regs.sr & 0x1f;

    M68000_regs.sr_high = M68000_regs.sr >> 8;
    M68000_regs.x_carry = (IntelFlag[intel] >> 8) & 0x01;
    M68000_regs.ccr     = IntelFlag[intel] & 0x0EFF;
}

void a68k_state_register(const char *type)
{
	int cpu = cpu_getactivecpu();

	state_save_register_UINT32(type, cpu, "D"         , &M68000_regs.d[0], 8);
	state_save_register_UINT32(type, cpu, "A"         , &M68000_regs.a[0], 8);
	state_save_register_UINT32(type, cpu, "PPC"       , &M68000_regs.previous_pc, 1);
	state_save_register_UINT32(type, cpu, "PC"        , &M68000_regs.pc, 1);
	state_save_register_UINT32(type, cpu, "USP"       , &M68000_regs.usp, 1);
	state_save_register_UINT32(type, cpu, "ISP"       , &M68000_regs.isp, 1);
	state_save_register_UINT32(type, cpu, "MSP"       , &zero, 1);
	state_save_register_UINT32(type, cpu, "VBR"       , &M68000_regs.vbr, 1);
	state_save_register_UINT32(type, cpu, "SFC"       , &M68000_regs.sfc, 1);
	state_save_register_UINT32(type, cpu, "DFC"       , &M68000_regs.dfc, 1);
	state_save_register_UINT32(type, cpu, "CACR"      , &zero, 1);
	state_save_register_UINT32(type, cpu, "CAAR"      , &zero, 1);
	state_save_register_UINT16(type, cpu, "SR"        , &M68000_regs.sr, 1);
	state_save_register_UINT32(type, cpu, "INT_LEVEL" , &M68000_regs.IRQ_level, 1);
	state_save_register_UINT32(type, cpu, "INT_CYCLES", (UINT32 *)&m68k_ICount, 1);
	state_save_register_int   (type, cpu, "STOPPED"   , &stopped);
	state_save_register_int   (type, cpu, "HALTED"    , (int *)&zero);
	state_save_register_UINT32(type, cpu, "PREF_ADDR" , &zero, 1);
	state_save_register_UINT32(type, cpu, "PREF_DATA" , &zero, 1);
  	state_save_register_func_presave(a68k_prepare_substate);
  	state_save_register_func_postload(a68k_post_load);
}

/****************************************************************************
 * 24-bit address, 16-bit data memory interface
 ****************************************************************************/

#ifdef A68K0

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
	changepc_a24_d16,
	cpu_readmem24bew,				/* Encrypted Versions */
	cpu_readmem24bew_word,
	readlong_a24_d16,
	cpu_readmem24bew_word,
	readlong_a24_d16
};

#endif /* A68k0 */

/****************************************************************************
 * 24-bit address, 32-bit data memory interface
 ****************************************************************************/

#ifdef A68K2

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
	changepc_a24_d32,
	cpu_readmem24bedw,
	readword_a24_d32,
	readlong_a24_d32,
	readword_a24_d32,
	readlong_a24_d32
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
	changepc_a32_d32,
	cpu_readmem32bedw,
	readword_a32_d32,
	readlong_a32_d32,
	readword_a32_d32,
	readlong_a32_d32
};

#endif /* A68K2 */

/********************************************/
/* Interface routines to link Mame -> 68KEM */
/********************************************/

#define READOP(a)	(cpu_readop16((a) ^ a68k_memory_intf.opcode_xor))

#ifdef A68K0

void m68000_init(void)
{
	a68k_state_register("m68000");
	M68000_regs.reset_callback = 0;
}

static void m68k16_reset_common(void)
{
	int (*rc)(void);

	rc = M68000_regs.reset_callback;
	memset(&M68000_regs,0,sizeof(M68000_regs));
	M68000_regs.reset_callback = rc;

    M68000_regs.a[7] = M68000_regs.isp = (( READOP(0) << 16 ) | READOP(2));
    M68000_regs.pc   = (( READOP(4) << 16 ) | READOP(6)) & 0xffffff;
    M68000_regs.sr_high = 0x27;

	#ifdef MAME_DEBUG
		M68000_regs.sr = 0x2700;
	#endif

    M68000_RESET();
}

void m68000_reset(void *param)
{
	struct m68k_encryption_interface *interface = param;

    /* Default Memory Routines */
	if (a68k_memory_intf.read8 != cpu_readmem24bew)
		a68k_memory_intf = interface_a24_d16;

	/* Import encryption routines if present */
	if (param)
	{
		a68k_memory_intf.read8pc = interface->read8pc;
		a68k_memory_intf.read16pc = interface->read16pc;
		a68k_memory_intf.read32pc = interface->read32pc;
		a68k_memory_intf.read16d = interface->read16d;
		a68k_memory_intf.read32d = interface->read32d;
	}

	m68k16_reset_common();
    M68000_regs.Memory_Interface = a68k_memory_intf;
}

void m68000_exit(void)
{
	/* nothing to do ? */
}


#ifdef TRACE68K 							/* Trace */
	static int skiptrace=0;
#endif

int m68000_execute(int cycles)
{
	if (M68000_regs.IRQ_level == 0x80) return cycles;		/* STOP with no IRQs */

	m68k_ICount = cycles;

#ifdef MAME_DEBUG
    do
    {
      if (mame_debug)
      {
        #ifdef TRACE68K

        int StartCycle = m68k_ICount;

        skiptrace++;

        if (skiptrace > 0)
        {
          int mycount, areg, dreg;
          areg = dreg = 0;
          
          for (mycount=7;mycount>=0;mycount--)
          {
            areg = areg + M68000_regs.a[mycount];
            dreg = dreg + M68000_regs.d[mycount];
          }

          log_cb(RETRO_LOG_DEBUG, LOGPRE "=> %8x %8x ",areg,dreg);
          log_cb(RETRO_LOG_DEBUG, LOGPRE "%6x %4x %d\n",M68000_regs.pc,M68000_regs.sr & 0x271F,m68k_ICount);
        }
        #endif

        /*m68k_memory_intf = a68k_memory_intf;*/

			  MAME_Debug();
        M68000_RUN();

        #ifdef TRACE68K
        if ((M68000_regs.IRQ_level & 0x80) || (cpu_getstatus(cpu_getactivecpu()) == 0))
          m68k_ICount = 0;
        else
          m68k_ICount = StartCycle - 12;
        #endif
      }
      else
			  M68000_RUN();
    } while (m68k_ICount > 0);

#else

	M68000_RUN();

#endif /* MAME_DEBUG */

	return (cycles - m68k_ICount);
}


unsigned m68000_get_context(void *dst)
{
	if( dst )
		*(a68k_cpu_context*)dst = M68000_regs;
	return sizeof(a68k_cpu_context);
}

void m68000_set_context(void *src)
{
	if( src )
	{
		M68000_regs = *(a68k_cpu_context*)src;
        a68k_memory_intf = M68000_regs.Memory_Interface;
    }
}

unsigned m68000_get_reg(int regnum)
{
    switch( regnum )
    {
    	case REG_PC:
		case M68K_PC: return M68000_regs.pc;
		case REG_SP:
		case M68K_ISP: return M68000_regs.isp;
		case M68K_USP: return M68000_regs.usp;
		case M68K_SR: return M68000_regs.sr;
		case M68K_VBR: return M68000_regs.vbr;
		case M68K_SFC: return M68000_regs.sfc;
		case M68K_DFC: return M68000_regs.dfc;
		case M68K_D0: return M68000_regs.d[0];
		case M68K_D1: return M68000_regs.d[1];
		case M68K_D2: return M68000_regs.d[2];
		case M68K_D3: return M68000_regs.d[3];
		case M68K_D4: return M68000_regs.d[4];
		case M68K_D5: return M68000_regs.d[5];
		case M68K_D6: return M68000_regs.d[6];
		case M68K_D7: return M68000_regs.d[7];
		case M68K_A0: return M68000_regs.a[0];
		case M68K_A1: return M68000_regs.a[1];
		case M68K_A2: return M68000_regs.a[2];
		case M68K_A3: return M68000_regs.a[3];
		case M68K_A4: return M68000_regs.a[4];
		case M68K_A5: return M68000_regs.a[5];
		case M68K_A6: return M68000_regs.a[6];
		case M68K_A7: return M68000_regs.a[7];
		case REG_PREVIOUSPC: return M68000_regs.previous_pc;
/* TODO: Verify that this is the right thing to do for the purpose? */
		default:
			if( regnum <= REG_SP_CONTENTS )
			{
				unsigned offset = M68000_regs.isp + 4 * (REG_SP_CONTENTS - regnum);
				if( offset < 0xfffffd )
					return (*a68k_memory_intf.read32)( offset );
            }
    }
    return 0;
}

void m68000_set_reg(int regnum, unsigned val)
{
    switch( regnum )
    {
    	case REG_PC:
		case M68K_PC: M68000_regs.pc = val; break;
		case REG_SP:
		case M68K_ISP: M68000_regs.isp = val; break;
		case M68K_USP: M68000_regs.usp = val; break;
		case M68K_SR: M68000_regs.sr = val; break;
		case M68K_VBR: M68000_regs.vbr = val; break;
		case M68K_SFC: M68000_regs.sfc = val; break;
		case M68K_DFC: M68000_regs.dfc = val; break;
		case M68K_D0: M68000_regs.d[0] = val; break;
		case M68K_D1: M68000_regs.d[1] = val; break;
		case M68K_D2: M68000_regs.d[2] = val; break;
		case M68K_D3: M68000_regs.d[3] = val; break;
		case M68K_D4: M68000_regs.d[4] = val; break;
		case M68K_D5: M68000_regs.d[5] = val; break;
		case M68K_D6: M68000_regs.d[6] = val; break;
		case M68K_D7: M68000_regs.d[7] = val; break;
		case M68K_A0: M68000_regs.a[0] = val; break;
		case M68K_A1: M68000_regs.a[1] = val; break;
		case M68K_A2: M68000_regs.a[2] = val; break;
		case M68K_A3: M68000_regs.a[3] = val; break;
		case M68K_A4: M68000_regs.a[4] = val; break;
		case M68K_A5: M68000_regs.a[5] = val; break;
		case M68K_A6: M68000_regs.a[6] = val; break;
		case M68K_A7: M68000_regs.a[7] = val; break;
/* TODO: Verify that this is the right thing to do for the purpose? */
		default:
			if( regnum <= REG_SP_CONTENTS )
			{
				unsigned offset = M68000_regs.isp + 4 * (REG_SP_CONTENTS - regnum);
				if( offset < 0xfffffd )
					(*a68k_memory_intf.write32)( offset, val );
            }
    }
}

void m68k_assert_irq(int int_line)
{
	/* Save icount */
	int StartCount = m68k_ICount;

	M68000_regs.IRQ_level = int_line;

    /* Now check for Interrupt */

	m68k_ICount = -1;
    M68000_RUN();

    /* Restore Count */
	m68k_ICount = StartCount;
}

void m68k_clear_irq(int int_line)
{
	M68000_regs.IRQ_level = 0;
}

void m68000_set_irq_line(int irqline, int state)
{
	if (irqline == IRQ_LINE_NMI)
		irqline = 7;
	switch(state)
	{
		case CLEAR_LINE:
			m68k_clear_irq(irqline);
			return;
		case ASSERT_LINE:
			m68k_assert_irq(irqline);
			return;
		default:
			m68k_assert_irq(irqline);
			return;
	}
}

void m68000_set_irq_callback(int (*callback)(int irqline))
{
	M68000_regs.irq_callback = callback;
}

void m68000_set_reset_callback(int (*callback)(void))
{
	M68000_regs.reset_callback = callback;
}

/****************************************************************************
 * Return a formatted string for a register
 ****************************************************************************/
const char *m68000_info(void *context, int regnum)
{
#ifdef MAME_DEBUG
extern int m68k_disassemble(char* str_buff, int pc, int cputype);
#endif

    static char buffer[32][47+1];
	static int which;
	a68k_cpu_context *r = context;

	which = (which+1) % 32;
	buffer[which][0] = '\0';
	if( !context )
		r = &M68000_regs;

	switch( regnum )
	{
		case CPU_INFO_REG+M68K_PC: sprintf(buffer[which], "PC:%06X", r->pc); break;
		case CPU_INFO_REG+M68K_ISP: sprintf(buffer[which], "ISP:%08X", r->isp); break;
		case CPU_INFO_REG+M68K_USP: sprintf(buffer[which], "USP:%08X", r->usp); break;
		case CPU_INFO_REG+M68K_SR: sprintf(buffer[which], "SR:%08X", r->sr); break;
		case CPU_INFO_REG+M68K_VBR: sprintf(buffer[which], "VBR:%08X", r->vbr); break;
		case CPU_INFO_REG+M68K_SFC: sprintf(buffer[which], "SFC:%08X", r->sfc); break;
		case CPU_INFO_REG+M68K_DFC: sprintf(buffer[which], "DFC:%08X", r->dfc); break;
		case CPU_INFO_REG+M68K_D0: sprintf(buffer[which], "D0:%08X", r->d[0]); break;
		case CPU_INFO_REG+M68K_D1: sprintf(buffer[which], "D1:%08X", r->d[1]); break;
		case CPU_INFO_REG+M68K_D2: sprintf(buffer[which], "D2:%08X", r->d[2]); break;
		case CPU_INFO_REG+M68K_D3: sprintf(buffer[which], "D3:%08X", r->d[3]); break;
		case CPU_INFO_REG+M68K_D4: sprintf(buffer[which], "D4:%08X", r->d[4]); break;
		case CPU_INFO_REG+M68K_D5: sprintf(buffer[which], "D5:%08X", r->d[5]); break;
		case CPU_INFO_REG+M68K_D6: sprintf(buffer[which], "D6:%08X", r->d[6]); break;
		case CPU_INFO_REG+M68K_D7: sprintf(buffer[which], "D7:%08X", r->d[7]); break;
		case CPU_INFO_REG+M68K_A0: sprintf(buffer[which], "A0:%08X", r->a[0]); break;
		case CPU_INFO_REG+M68K_A1: sprintf(buffer[which], "A1:%08X", r->a[1]); break;
		case CPU_INFO_REG+M68K_A2: sprintf(buffer[which], "A2:%08X", r->a[2]); break;
		case CPU_INFO_REG+M68K_A3: sprintf(buffer[which], "A3:%08X", r->a[3]); break;
		case CPU_INFO_REG+M68K_A4: sprintf(buffer[which], "A4:%08X", r->a[4]); break;
		case CPU_INFO_REG+M68K_A5: sprintf(buffer[which], "A5:%08X", r->a[5]); break;
		case CPU_INFO_REG+M68K_A6: sprintf(buffer[which], "A6:%08X", r->a[6]); break;
		case CPU_INFO_REG+M68K_A7: sprintf(buffer[which], "A7:%08X", r->a[7]); break;
		case CPU_INFO_FLAGS:
			sprintf(buffer[which], "%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c",
				r->sr & 0x8000 ? 'T':'.',
				r->sr & 0x4000 ? '?':'.',
				r->sr & 0x2000 ? 'S':'.',
				r->sr & 0x1000 ? '?':'.',
				r->sr & 0x0800 ? '?':'.',
				r->sr & 0x0400 ? 'I':'.',
				r->sr & 0x0200 ? 'I':'.',
				r->sr & 0x0100 ? 'I':'.',
				r->sr & 0x0080 ? '?':'.',
				r->sr & 0x0040 ? '?':'.',
				r->sr & 0x0020 ? '?':'.',
				r->sr & 0x0010 ? 'X':'.',
				r->sr & 0x0008 ? 'N':'.',
				r->sr & 0x0004 ? 'Z':'.',
				r->sr & 0x0002 ? 'V':'.',
				r->sr & 0x0001 ? 'C':'.');
            break;
		case CPU_INFO_NAME: return "68000";
		case CPU_INFO_FAMILY: return "Motorola 68K";
		case CPU_INFO_VERSION: return "0.16";
		case CPU_INFO_FILE: return __FILE__;
		case CPU_INFO_CREDITS: return "Copyright 1998,99 Mike Coates, Darren Olafson. All rights reserved";
		case CPU_INFO_REG_LAYOUT: return (const char*)M68K_layout;
        case CPU_INFO_WIN_LAYOUT: return (const char*)m68k_win_layout;
	}
	return buffer[which];
}

unsigned m68000_dasm(char *buffer, unsigned pc)
{
	A68K_SET_PC_CALLBACK(pc);

#ifdef MAME_DEBUG
	m68k_memory_intf = a68k_memory_intf;
	return m68k_disassemble(buffer, pc, M68K_CPU_TYPE_68000);
#else
	sprintf(buffer, "$%04X", cpu_readop16(pc) );
	return 2;
#endif
}

/****************************************************************************
 * M68010 section
 ****************************************************************************/

#if (HAS_M68010)

void m68010_reset(void *param)
{
	if (a68k_memory_intf.read8 != cpu_readmem24bew)
		a68k_memory_intf = interface_a24_d16;

	m68k16_reset_common();

    M68000_regs.CPUtype=1;
    M68000_regs.Memory_Interface = a68k_memory_intf;
}

void m68010_init(void) { m68000_init(); }
void m68010_exit(void) { m68000_exit(); }
int  m68010_execute(int cycles) { return m68000_execute(cycles); }
unsigned m68010_get_context(void *dst) { return m68000_get_context(dst); }

void m68010_set_context(void *src)
{
	if( src )
    {
		M68000_regs = *(a68k_cpu_context*)src;
        a68k_memory_intf = M68000_regs.Memory_Interface;
    }
}

unsigned m68010_get_reg(int regnum) { return m68000_get_reg(regnum); }
void m68010_set_reg(int regnum, unsigned val) { m68000_set_reg(regnum,val); }
void m68010_set_irq_line(int irqline, int state)  { m68000_set_irq_line(irqline,state); }
void m68010_set_irq_callback(int (*callback)(int irqline))  { m68000_set_irq_callback(callback); }

const char *m68010_info(void *context, int regnum)
{
	switch( regnum )
	{
		case CPU_INFO_NAME: return "68010";
	}
	return m68000_info(context,regnum);
}

unsigned m68010_dasm(char *buffer, unsigned pc)
{
	A68K_SET_PC_CALLBACK(pc);

#ifdef MAME_DEBUG
	m68k_memory_intf = a68k_memory_intf;
	return m68k_disassemble(buffer, pc, M68K_CPU_TYPE_68010);
#else
	sprintf(buffer, "$%04X", cpu_readop16(pc) );
	return 2;
#endif
}
#endif


#endif /* A68K0 */


/****************************************************************************
 * M68020 section
 ****************************************************************************/

#ifdef A68K2

void m68020_init(void)
{
	a68k_state_register("m68020");
	M68020_regs.reset_callback = 0;
}

static void m68k32_reset_common(void)
{
	int (*rc)(void);

	rc = M68020_regs.reset_callback;
	memset(&M68020_regs,0,sizeof(M68020_regs));
	M68020_regs.reset_callback = rc;

    M68020_regs.a[7] = M68020_regs.isp = (( READOP(0) << 16 ) | READOP(2));
    M68020_regs.pc   = (( READOP(4) << 16 ) | READOP(6)) & 0xffffff;
    M68020_regs.sr_high = 0x27;

	#ifdef MAME_DEBUG
		M68020_regs.sr = 0x2700;
	#endif

    M68020_RESET();
}

#if (HAS_M68020)

void m68020_reset(void *param)
{
	if (a68k_memory_intf.read8 != cpu_readmem32bedw)
		a68k_memory_intf = interface_a32_d32;

	m68k32_reset_common();

    M68020_regs.CPUtype=2;
    M68020_regs.Memory_Interface = a68k_memory_intf;
}

void m68020_exit(void)
{
	/* nothing to do ? */
}

int m68020_execute(int cycles)
{
	if (M68020_regs.IRQ_level == 0x80) return cycles;		/* STOP with no IRQs */

	m68k_ICount = cycles;

#ifdef MAME_DEBUG
    do
    {
      if (mame_debug)
      {
        #ifdef TRACE68K

        int StartCycle = m68k_ICount;

        skiptrace++;

        if (skiptrace > 0)
        {
			    int mycount, areg, dreg;
          areg = dreg = 0;
          for (mycount=7;mycount>=0;mycount--)
          {
            areg = areg + M68020_regs.a[mycount];
            dreg = dreg + M68020_regs.d[mycount];
          }

          log_cb(RETRO_LOG_DEBUG, LOGPRE "=> %8x %8x ",areg,dreg);
          log_cb(RETRO_LOG_DEBUG, LOGPRE "%6x %4x %d\n",M68020_regs.pc,M68020_regs.sr & 0x271F,m68k_ICount);
        }
        #endif

#if 0
	        m68k_memory_intf = a68k_memory_intf;
#endif
			MAME_Debug();
      M68020_RUN();

            #ifdef TRACE68K
            if ((M68020_regs.IRQ_level & 0x80) || (cpu_getstatus(cpu_getactivecpu()) == 0))
    			m68k_ICount = 0;
            else
				m68k_ICount = StartCycle - 12;
            #endif
        }
        else
			M68020_RUN();

    } while (m68k_ICount > 0);

#else

	M68020_RUN();

#endif /* MAME_DEBUG */

	return (cycles - m68k_ICount);
}

unsigned m68020_get_context(void *dst)
{
	if( dst )
		*(a68k_cpu_context*)dst = M68020_regs;
	return sizeof(a68k_cpu_context);
}

void m68020_set_context(void *src)
{
	if( src )
    {
		M68020_regs = *(a68k_cpu_context*)src;
        a68k_memory_intf = M68020_regs.Memory_Interface;
    }
}

unsigned m68020_get_reg(int regnum)
{
    switch( regnum )
    {
    	case REG_PC:
		case M68K_PC: return M68020_regs.pc;
		case REG_SP:
		case M68K_ISP: return M68020_regs.isp;
		case M68K_USP: return M68020_regs.usp;
		case M68K_SR: return M68020_regs.sr;
		case M68K_VBR: return M68020_regs.vbr;
		case M68K_SFC: return M68020_regs.sfc;
		case M68K_DFC: return M68020_regs.dfc;
		case M68K_D0: return M68020_regs.d[0];
		case M68K_D1: return M68020_regs.d[1];
		case M68K_D2: return M68020_regs.d[2];
		case M68K_D3: return M68020_regs.d[3];
		case M68K_D4: return M68020_regs.d[4];
		case M68K_D5: return M68020_regs.d[5];
		case M68K_D6: return M68020_regs.d[6];
		case M68K_D7: return M68020_regs.d[7];
		case M68K_A0: return M68020_regs.a[0];
		case M68K_A1: return M68020_regs.a[1];
		case M68K_A2: return M68020_regs.a[2];
		case M68K_A3: return M68020_regs.a[3];
		case M68K_A4: return M68020_regs.a[4];
		case M68K_A5: return M68020_regs.a[5];
		case M68K_A6: return M68020_regs.a[6];
		case M68K_A7: return M68020_regs.a[7];
		case REG_PREVIOUSPC: return M68020_regs.previous_pc;
/* TODO: Verify that this is the right thing to do for the purpose? */
		default:
			if( regnum <= REG_SP_CONTENTS )
			{
				unsigned offset = M68020_regs.isp + 4 * (REG_SP_CONTENTS - regnum);
				if( offset < 0xfffffd )
					return (*a68k_memory_intf.read32)( offset );
            }
    }
    return 0;
}

void m68020_set_reg(int regnum, unsigned val)
{
    switch( regnum )
    {
    	case REG_PC:
		case M68K_PC: M68020_regs.pc = val; break;
		case REG_SP:
		case M68K_ISP: M68020_regs.isp = val; break;
		case M68K_USP: M68020_regs.usp = val; break;
		case M68K_SR: M68020_regs.sr = val; break;
		case M68K_VBR: M68020_regs.vbr = val; break;
		case M68K_SFC: M68020_regs.sfc = val; break;
		case M68K_DFC: M68020_regs.dfc = val; break;
		case M68K_D0: M68020_regs.d[0] = val; break;
		case M68K_D1: M68020_regs.d[1] = val; break;
		case M68K_D2: M68020_regs.d[2] = val; break;
		case M68K_D3: M68020_regs.d[3] = val; break;
		case M68K_D4: M68020_regs.d[4] = val; break;
		case M68K_D5: M68020_regs.d[5] = val; break;
		case M68K_D6: M68020_regs.d[6] = val; break;
		case M68K_D7: M68020_regs.d[7] = val; break;
		case M68K_A0: M68020_regs.a[0] = val; break;
		case M68K_A1: M68020_regs.a[1] = val; break;
		case M68K_A2: M68020_regs.a[2] = val; break;
		case M68K_A3: M68020_regs.a[3] = val; break;
		case M68K_A4: M68020_regs.a[4] = val; break;
		case M68K_A5: M68020_regs.a[5] = val; break;
		case M68K_A6: M68020_regs.a[6] = val; break;
		case M68K_A7: M68020_regs.a[7] = val; break;
/* TODO: Verify that this is the right thing to do for the purpose? */
		default:
			if( regnum <= REG_SP_CONTENTS )
			{
				unsigned offset = M68020_regs.isp + 4 * (REG_SP_CONTENTS - regnum);
				if( offset < 0xfffffd )
					(*a68k_memory_intf.write32)( offset, val );
            }
    }
}

void m68020_assert_irq(int int_line)
{
	/* Save icount */
	int StartCount = m68k_ICount;

	M68020_regs.IRQ_level = int_line;

    /* Now check for Interrupt */

	m68k_ICount = -1;
    M68020_RUN();

    /* Restore Count */
	m68k_ICount = StartCount;
}

void m68020_clear_irq(int int_line)
{
	M68020_regs.IRQ_level = 0;
}

void m68020_set_irq_line(int irqline, int state)
{
	if (irqline == IRQ_LINE_NMI)
		irqline = 7;
	switch(state)
	{
		case CLEAR_LINE:
			m68020_clear_irq(irqline);
			return;
		case ASSERT_LINE:
			m68020_assert_irq(irqline);
			return;
		default:
			m68020_assert_irq(irqline);
			return;
	}
}

void m68020_set_irq_callback(int (*callback)(int irqline))
{
	M68020_regs.irq_callback = callback;
}

void m68020_set_reset_callback(int (*callback)(void))
{
	M68020_regs.reset_callback = callback;
}

const char *m68020_info(void *context, int regnum)
{
#ifdef MAME_DEBUG
extern int m68k_disassemble(char* str_buff, int pc, int cputype);
#endif

    static char buffer[32][47+1];
	static int which;
	a68k_cpu_context *r = context;

	which = (which + 1) % 32;
	buffer[which][0] = '\0';
	if( !context )
		r = &M68020_regs;

	switch( regnum )
	{
		case CPU_INFO_REG+M68K_PC: sprintf(buffer[which], "PC:%06X", r->pc); break;
		case CPU_INFO_REG+M68K_ISP: sprintf(buffer[which], "ISP:%08X", r->isp); break;
		case CPU_INFO_REG+M68K_USP: sprintf(buffer[which], "USP:%08X", r->usp); break;
		case CPU_INFO_REG+M68K_SR: sprintf(buffer[which], "SR:%08X", r->sr); break;
		case CPU_INFO_REG+M68K_VBR: sprintf(buffer[which], "VBR:%08X", r->vbr); break;
		case CPU_INFO_REG+M68K_SFC: sprintf(buffer[which], "SFC:%08X", r->sfc); break;
		case CPU_INFO_REG+M68K_DFC: sprintf(buffer[which], "DFC:%08X", r->dfc); break;
		case CPU_INFO_REG+M68K_D0: sprintf(buffer[which], "D0:%08X", r->d[0]); break;
		case CPU_INFO_REG+M68K_D1: sprintf(buffer[which], "D1:%08X", r->d[1]); break;
		case CPU_INFO_REG+M68K_D2: sprintf(buffer[which], "D2:%08X", r->d[2]); break;
		case CPU_INFO_REG+M68K_D3: sprintf(buffer[which], "D3:%08X", r->d[3]); break;
		case CPU_INFO_REG+M68K_D4: sprintf(buffer[which], "D4:%08X", r->d[4]); break;
		case CPU_INFO_REG+M68K_D5: sprintf(buffer[which], "D5:%08X", r->d[5]); break;
		case CPU_INFO_REG+M68K_D6: sprintf(buffer[which], "D6:%08X", r->d[6]); break;
		case CPU_INFO_REG+M68K_D7: sprintf(buffer[which], "D7:%08X", r->d[7]); break;
		case CPU_INFO_REG+M68K_A0: sprintf(buffer[which], "A0:%08X", r->a[0]); break;
		case CPU_INFO_REG+M68K_A1: sprintf(buffer[which], "A1:%08X", r->a[1]); break;
		case CPU_INFO_REG+M68K_A2: sprintf(buffer[which], "A2:%08X", r->a[2]); break;
		case CPU_INFO_REG+M68K_A3: sprintf(buffer[which], "A3:%08X", r->a[3]); break;
		case CPU_INFO_REG+M68K_A4: sprintf(buffer[which], "A4:%08X", r->a[4]); break;
		case CPU_INFO_REG+M68K_A5: sprintf(buffer[which], "A5:%08X", r->a[5]); break;
		case CPU_INFO_REG+M68K_A6: sprintf(buffer[which], "A6:%08X", r->a[6]); break;
		case CPU_INFO_REG+M68K_A7: sprintf(buffer[which], "A7:%08X", r->a[7]); break;
		case CPU_INFO_FLAGS:
			sprintf(buffer[which], "%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c",
				r->sr & 0x8000 ? 'T':'.',
				r->sr & 0x4000 ? '?':'.',
				r->sr & 0x2000 ? 'S':'.',
				r->sr & 0x1000 ? '?':'.',
				r->sr & 0x0800 ? '?':'.',
				r->sr & 0x0400 ? 'I':'.',
				r->sr & 0x0200 ? 'I':'.',
				r->sr & 0x0100 ? 'I':'.',
				r->sr & 0x0080 ? '?':'.',
				r->sr & 0x0040 ? '?':'.',
				r->sr & 0x0020 ? '?':'.',
				r->sr & 0x0010 ? 'X':'.',
				r->sr & 0x0008 ? 'N':'.',
				r->sr & 0x0004 ? 'Z':'.',
				r->sr & 0x0002 ? 'V':'.',
				r->sr & 0x0001 ? 'C':'.');
            break;
		case CPU_INFO_NAME: return "68020";
		case CPU_INFO_FAMILY: return "Motorola 68K";
		case CPU_INFO_VERSION: return "0.16";
		case CPU_INFO_FILE: return __FILE__;
		case CPU_INFO_CREDITS: return "Copyright 1998,99 Mike Coates, Darren Olafson. All rights reserved";
		case CPU_INFO_REG_LAYOUT: return (const char*)M68K_layout;
        case CPU_INFO_WIN_LAYOUT: return (const char*)m68k_win_layout;
	}
	return buffer[which];
}

unsigned m68020_dasm(char *buffer, unsigned pc)
{
	A68K_SET_PC_CALLBACK(pc);

#ifdef MAME_DEBUG
	m68k_memory_intf = a68k_memory_intf;
	return m68k_disassemble(buffer, pc, M68K_CPU_TYPE_68020);
#else
	sprintf(buffer, "$%04X", cpu_readop16(pc) );
	return 2;
#endif
}
#endif

#if (HAS_M68EC020)

void m68ec020_reset(void *param)
{
	if (a68k_memory_intf.read8 != cpu_readmem24bedw)
		a68k_memory_intf = interface_a24_d32;

	m68k32_reset_common();

    M68020_regs.CPUtype=2;
    M68020_regs.Memory_Interface = a68k_memory_intf;
}

void m68ec020_init(void) { m68020_init(); }
void m68ec020_exit(void) { m68020_exit(); }
int  m68ec020_execute(int cycles) { return m68020_execute(cycles); }
unsigned m68ec020_get_context(void *dst) { return m68020_get_context(dst); }

void m68ec020_set_context(void *src)
{
	if( src )
    {
		M68020_regs = *(a68k_cpu_context*)src;
        a68k_memory_intf = M68020_regs.Memory_Interface;
    }
}

unsigned m68ec020_get_reg(int regnum) { return m68020_get_reg(regnum); }
void m68ec020_set_reg(int regnum, unsigned val) { m68020_set_reg(regnum,val); }
void m68ec020_set_irq_line(int irqline, int state)  { m68020_set_irq_line(irqline,state); }
void m68ec020_set_irq_callback(int (*callback)(int irqline))  { m68020_set_irq_callback(callback); }

const char *m68ec020_info(void *context, int regnum)
{
	switch( regnum )
	{
		case CPU_INFO_NAME: return "68EC020";
	}
	return m68020_info(context,regnum);
}

unsigned m68ec020_dasm(char *buffer, unsigned pc)
{
	A68K_SET_PC_CALLBACK(pc);

#ifdef MAME_DEBUG
	m68k_memory_intf = a68k_memory_intf;
	return m68k_disassemble(buffer, pc, M68K_CPU_TYPE_68EC020);
#else
	sprintf(buffer, "$%04X", cpu_readop16(pc) );
	return 2;
#endif
}

#endif

#endif /* A68K2 */
