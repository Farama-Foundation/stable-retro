/*****************************************************************************
 *
 *	 z8000.c
 *	 Portable Z8000(2) emulator
 *	 Z8000 MAME interface
 *
 *	 Copyright (C) 1998,1999 Juergen Buchmueller, all rights reserved.
 *	 Bug fixes and MSB_FIRST compliance Ernesto Corvi.
 *
 *	 - This source code is released as freeware for non-commercial purposes.
 *	 - You are free to use and redistribute this code in modified or
 *	   unmodified form, provided you list me in the credits.
 *	 - If you modify this source code, you must add a notice to each modified
 *	   source file that it has been changed.  If you're a nice person, you
 *	   will clearly mark each change too.  :)
 *	 - If you wish to use this for commercial purposes, please contact me at
 *	   pullmoll@t-online.de
 *	 - The author of this copywritten work reserves the right to change the
 *     terms of its usage and license at any time, including retroactively
 *   - This entire notice must remain in the source code.
 *
 *****************************************************************************/

#include "driver.h"
#include "mamedbg.h"
#include "z8000.h"
#include "z8000cpu.h"

static UINT8 z8000_reg_layout[] = {
	Z8000_PC, Z8000_NSP, Z8000_FCW, Z8000_PSAP, Z8000_REFRESH, -1,
	Z8000_R0, Z8000_R1, Z8000_R2, Z8000_R3, Z8000_R4, Z8000_R5, Z8000_R6, Z8000_R7, -1,
	Z8000_R8, Z8000_R9, Z8000_R10,Z8000_R11,Z8000_R12,Z8000_R13,Z8000_R14,Z8000_R15,-1,
	Z8000_IRQ_REQ, Z8000_IRQ_SRV, Z8000_IRQ_VEC, Z8000_NMI_STATE, Z8000_NVI_STATE, Z8000_VI_STATE, 0
};

static UINT8 z8000_win_layout[] = {
	 0, 0,80, 4,	/* register window (top rows) */
	 0, 5,26,17,	/* disassembler window (left colums) */
	27, 5,53, 8,	/* memory #1 window (right, upper middle) */
	27,14,53, 8,	/* memory #2 window (right, lower middle) */
	 0,23,80, 1,	/* command line window (bottom rows) */
};

/* opcode execution table */
Z8000_exec *z8000_exec = NULL;

typedef union {
    UINT8   B[16]; /* RL0,RH0,RL1,RH1...RL7,RH7 */
    UINT16  W[16]; /* R0,R1,R2...R15 */
    UINT32  L[8];  /* RR0,RR2,RR4..RR14 */
    UINT64  Q[4];  /* RQ0,RQ4,..RQ12 */
}   z8000_reg_file;

typedef struct {
    UINT16  op[4];      /* opcodes/data of current instruction */
	UINT16	ppc;		/* previous program counter */
    UINT16  pc;         /* program counter */
    UINT16  psap;       /* program status pointer */
    UINT16  fcw;        /* flags and control word */
    UINT16  refresh;    /* refresh timer/counter */
    UINT16  nsp;        /* system stack pointer */
    UINT16  irq_req;    /* CPU is halted, interrupt or trap request */
    UINT16  irq_srv;    /* serviced interrupt request */
    UINT16  irq_vec;    /* interrupt vector */
    z8000_reg_file regs;/* registers */
	int nmi_state;		/* NMI line state */
	int irq_state[2];	/* IRQ line states (NVI, VI) */
    int (*irq_callback)(int irqline);
}   z8000_Regs;

int z8000_ICount;

/* current CPU context */
static z8000_Regs Z;

/* zero, sign and parity flags for logical byte operations */
static UINT8 z8000_zsp[256];

/* conversion table for Z8000 DAB opcode */
#include "z8000dab.h"

/**************************************************************************
 * This is the register file layout:
 *
 * BYTE 	   WORD 		LONG		   QUAD
 * msb	 lsb	   bits 		  bits			 bits
 * RH0 - RL0   R 0 15- 0	RR 0  31-16    RQ 0  63-48
 * RH1 - RL1   R 1 15- 0		  15- 0 		 47-32
 * RH2 - RL2   R 2 15- 0	RR 2  31-16 		 31-16
 * RH3 - RL3   R 3 15- 0		  15- 0 		 15- 0
 * RH4 - RL4   R 4 15- 0	RR 4  31-16    RQ 4  63-48
 * RH5 - RL5   R 5 15- 0		  15- 0 		 47-32
 * RH6 - RL6   R 6 15- 0	RR 6  31-16 		 31-16
 * RH7 - RL7   R 7 15- 0		  15- 0 		 15- 0
 *			   R 8 15- 0	RR 8  31-16    RQ 8  63-48
 *			   R 9 15- 0		  15- 0 		 47-32
 *			   R10 15- 0	RR10  31-16 		 31-16
 *			   R11 15- 0		  15- 0 		 15- 0
 *			   R12 15- 0	RR12  31-16    RQ12  63-48
 *			   R13 15- 0		  15- 0 		 47-32
 *			   R14 15- 0	RR14  31-16 		 31-16
 *			   R15 15- 0		  15- 0 		 15- 0
 *
 * Note that for non-MSB_FIRST machines we have the case that the RR registers
 * use the lower numbered R registers in the higher bit positions.
 * And also the RQ registers use the lower numbered RR registers in the
 * higher bit positions.
 * That's the reason for the ordering in the following pointer table.
 **************************************************************************/
#ifdef	MSB_FIRST

    /* pointers to byte (8bit) registers */
	static UINT8	*pRB[16] =
	{
		&Z.regs.B[ 0],&Z.regs.B[ 2],&Z.regs.B[ 4],&Z.regs.B[ 6],
		&Z.regs.B[ 8],&Z.regs.B[10],&Z.regs.B[12],&Z.regs.B[14],
		&Z.regs.B[ 1],&Z.regs.B[ 3],&Z.regs.B[ 5],&Z.regs.B[ 7],
		&Z.regs.B[ 9],&Z.regs.B[11],&Z.regs.B[13],&Z.regs.B[15]
	};

	/* pointers to word (16bit) registers */
	static UINT16	*pRW[16] =
	{
		&Z.regs.W[ 0],&Z.regs.W[ 1],&Z.regs.W[ 2],&Z.regs.W[ 3],
		&Z.regs.W[ 4],&Z.regs.W[ 5],&Z.regs.W[ 6],&Z.regs.W[ 7],
		&Z.regs.W[ 8],&Z.regs.W[ 9],&Z.regs.W[10],&Z.regs.W[11],
		&Z.regs.W[12],&Z.regs.W[13],&Z.regs.W[14],&Z.regs.W[15]
	};

	/* pointers to long (32bit) registers */
	static UINT32	*pRL[16] =
	{
		&Z.regs.L[ 0],&Z.regs.L[ 0],&Z.regs.L[ 1],&Z.regs.L[ 1],
		&Z.regs.L[ 2],&Z.regs.L[ 2],&Z.regs.L[ 3],&Z.regs.L[ 3],
		&Z.regs.L[ 4],&Z.regs.L[ 4],&Z.regs.L[ 5],&Z.regs.L[ 5],
		&Z.regs.L[ 6],&Z.regs.L[ 6],&Z.regs.L[ 7],&Z.regs.L[ 7]
	};
#else

	/* pointers to byte (8bit) registers */
	static UINT8	*pRB[16] =
	{
		&Z.regs.B[ 7],&Z.regs.B[ 5],&Z.regs.B[ 3],&Z.regs.B[ 1],
		&Z.regs.B[15],&Z.regs.B[13],&Z.regs.B[11],&Z.regs.B[ 9],
		&Z.regs.B[ 6],&Z.regs.B[ 4],&Z.regs.B[ 2],&Z.regs.B[ 0],
		&Z.regs.B[14],&Z.regs.B[12],&Z.regs.B[10],&Z.regs.B[ 8]
	};

	static UINT16	*pRW[16] =
	{
        &Z.regs.W[ 3],&Z.regs.W[ 2],&Z.regs.W[ 1],&Z.regs.W[ 0],
        &Z.regs.W[ 7],&Z.regs.W[ 6],&Z.regs.W[ 5],&Z.regs.W[ 4],
        &Z.regs.W[11],&Z.regs.W[10],&Z.regs.W[ 9],&Z.regs.W[ 8],
        &Z.regs.W[15],&Z.regs.W[14],&Z.regs.W[13],&Z.regs.W[12]
    };

    /* pointers to long (32bit) registers */
	static UINT32	*pRL[16] =
	{
		&Z.regs.L[ 1],&Z.regs.L[ 1],&Z.regs.L[ 0],&Z.regs.L[ 0],
		&Z.regs.L[ 3],&Z.regs.L[ 3],&Z.regs.L[ 2],&Z.regs.L[ 2],
		&Z.regs.L[ 5],&Z.regs.L[ 5],&Z.regs.L[ 4],&Z.regs.L[ 4],
		&Z.regs.L[ 7],&Z.regs.L[ 7],&Z.regs.L[ 6],&Z.regs.L[ 6]
    };

#endif

/* pointers to quad word (64bit) registers */
static UINT64   *pRQ[16] = {
    &Z.regs.Q[ 0],&Z.regs.Q[ 0],&Z.regs.Q[ 0],&Z.regs.Q[ 0],
    &Z.regs.Q[ 1],&Z.regs.Q[ 1],&Z.regs.Q[ 1],&Z.regs.Q[ 1],
    &Z.regs.Q[ 2],&Z.regs.Q[ 2],&Z.regs.Q[ 2],&Z.regs.Q[ 2],
    &Z.regs.Q[ 3],&Z.regs.Q[ 3],&Z.regs.Q[ 3],&Z.regs.Q[ 3]};

static INLINE UINT16 RDOP(void)
{
	UINT16 res = cpu_readop16(PC);
    PC += 2;
    return res;
}

static INLINE UINT8 RDMEM_B(UINT16 addr)
{
	return cpu_readmem16bew(addr);
}

static INLINE UINT16 RDMEM_W(UINT16 addr)
{
	addr &= ~1;
	return cpu_readmem16bew_word(addr);
}

static INLINE UINT32 RDMEM_L(UINT16 addr)
{
	UINT32 result;
	addr &= ~1;
	result = cpu_readmem16bew_word(addr) << 16;
	return result + cpu_readmem16bew_word(addr + 2);
}

static INLINE void WRMEM_B(UINT16 addr, UINT8 value)
{
	cpu_writemem16bew(addr, value);
}

static INLINE void WRMEM_W(UINT16 addr, UINT16 value)
{
	addr &= ~1;
	cpu_writemem16bew_word(addr, value);
}

static INLINE void WRMEM_L(UINT16 addr, UINT32 value)
{
	addr &= ~1;
	cpu_writemem16bew_word(addr, value >> 16);
	cpu_writemem16bew_word((UINT16)(addr + 2), value & 0xffff);
}

static INLINE UINT8 RDPORT_B(int mode, UINT16 addr)
{
	if( mode == 0 )
	{
		return cpu_readport16(addr);
	}
	else
	{
		/* how to handle MMU reads? */
		return 0x00;
	}
}

static INLINE UINT16 RDPORT_W(int mode, UINT16 addr)
{
	if( mode == 0 )
	{
		return cpu_readport16((UINT16)(addr)) +
			  (cpu_readport16((UINT16)(addr+1)) << 8);
	}
	else
	{
		/* how to handle MMU reads? */
		return 0x0000;
	}
}

static INLINE UINT32 RDPORT_L(int mode, UINT16 addr)
{
	if( mode == 0 )
	{
		return	cpu_readport16((UINT16)(addr)) +
			   (cpu_readport16((UINT16)(addr+1)) <<  8) +
			   (cpu_readport16((UINT16)(addr+2)) << 16) +
			   (cpu_readport16((UINT16)(addr+3)) << 24);
	}
	else
	{
		/* how to handle MMU reads? */
		return 0x00000000;
	}
}

static INLINE void WRPORT_B(int mode, UINT16 addr, UINT8 value)
{
	if( mode == 0 )
	{
        cpu_writeport16(addr,value);
	}
	else
	{
		/* how to handle MMU writes? */
    }
}

static INLINE void WRPORT_W(int mode, UINT16 addr, UINT16 value)
{
	if( mode == 0 )
	{
		cpu_writeport16((UINT16)(addr),value & 0xff);
		cpu_writeport16((UINT16)(addr+1),(value >> 8) & 0xff);
	}
	else
	{
		/* how to handle MMU writes? */
    }
}

static INLINE void WRPORT_L(int mode, UINT16 addr, UINT32 value)
{
	if( mode == 0 )
	{
		cpu_writeport16((UINT16)(addr),value & 0xff);
		cpu_writeport16((UINT16)(addr+1),(value >> 8) & 0xff);
		cpu_writeport16((UINT16)(addr+2),(value >> 16) & 0xff);
		cpu_writeport16((UINT16)(addr+3),(value >> 24) & 0xff);
	}
	else
	{
		/* how to handle MMU writes? */
	}
}

#include "z8000ops.c"
#include "z8000tbl.c"

static INLINE void set_irq(int type)
{
    switch ((type >> 8) & 255)
    {
        case Z8000_TRAP >> 8:
            if (IRQ_SRV >= Z8000_TRAP)
                return; /* double TRAP.. very bad :( */
            IRQ_REQ = type;
            break;
        case Z8000_NMI >> 8:
            if (IRQ_SRV >= Z8000_NMI)
                return; /* no NMIs inside trap */
            IRQ_REQ = type;
            break;
        case Z8000_SEGTRAP >> 8:
            if (IRQ_SRV >= Z8000_SEGTRAP)
                return; /* no SEGTRAPs inside NMI/TRAP */
            IRQ_REQ = type;
            break;
        case Z8000_NVI >> 8:
            if (IRQ_SRV >= Z8000_NVI)
                return; /* no NVIs inside SEGTRAP/NMI/TRAP */
            IRQ_REQ = type;
            break;
        case Z8000_VI >> 8:
            if (IRQ_SRV >= Z8000_VI)
                return; /* no VIs inside NVI/SEGTRAP/NMI/TRAP */
            IRQ_REQ = type;
            break;
        case Z8000_SYSCALL >> 8:
            log_cb(RETRO_LOG_DEBUG, LOGPRE "Z8K#%d SYSCALL $%02x\n", cpu_getactivecpu(), type & 0xff);
            IRQ_REQ = type;
            break;
        default:
            log_cb(RETRO_LOG_DEBUG, LOGPRE "Z8000 invalid Cause_Interrupt %04x\n", type);
            return;
    }
    /* set interrupt request flag, reset HALT flag */
    IRQ_REQ = type & ~Z8000_HALT;
}


static INLINE void Interrupt(void)
{
    UINT16 fcw = FCW;

    if (IRQ_REQ & Z8000_NVI)
    {
        int type = (*Z.irq_callback)(0);
        set_irq(type);
    }

    if (IRQ_REQ & Z8000_VI)
    {
        int type = (*Z.irq_callback)(1);
        set_irq(type);
    }

   /* trap ? */
   if ( IRQ_REQ & Z8000_TRAP )
   {
        CHANGE_FCW(fcw | F_S_N);/* swap to system stack */
        PUSHW( SP, PC );        /* save current PC */
        PUSHW( SP, fcw );       /* save current FCW */
        PUSHW( SP, IRQ_REQ );   /* save interrupt/trap type tag */
        IRQ_SRV = IRQ_REQ;
        IRQ_REQ &= ~Z8000_TRAP;
        PC = TRAP;
        log_cb(RETRO_LOG_DEBUG, LOGPRE "Z8K#%d trap $%04x\n", cpu_getactivecpu(), PC );
   }
   else
   if ( IRQ_REQ & Z8000_SYSCALL )
   {
        CHANGE_FCW(fcw | F_S_N);/* swap to system stack */
        PUSHW( SP, PC );        /* save current PC */
        PUSHW( SP, fcw );       /* save current FCW */
        PUSHW( SP, IRQ_REQ );   /* save interrupt/trap type tag */
        IRQ_SRV = IRQ_REQ;
        IRQ_REQ &= ~Z8000_SYSCALL;
        PC = SYSCALL;
        log_cb(RETRO_LOG_DEBUG, LOGPRE "Z8K#%d syscall $%04x\n", cpu_getactivecpu(), PC );
   }
   else
   if ( IRQ_REQ & Z8000_SEGTRAP )
   {
        CHANGE_FCW(fcw | F_S_N);/* swap to system stack */
        PUSHW( SP, PC );        /* save current PC */
        PUSHW( SP, fcw );       /* save current FCW */
        PUSHW( SP, IRQ_REQ );   /* save interrupt/trap type tag */
        IRQ_SRV = IRQ_REQ;
        IRQ_REQ &= ~Z8000_SEGTRAP;
        PC = SEGTRAP;
        log_cb(RETRO_LOG_DEBUG, LOGPRE "Z8K#%d segtrap $%04x\n", cpu_getactivecpu(), PC );
   }
   else
   if ( IRQ_REQ & Z8000_NMI )
   {
        CHANGE_FCW(fcw | F_S_N);/* swap to system stack */
        PUSHW( SP, PC );        /* save current PC */
        PUSHW( SP, fcw );       /* save current FCW */
        PUSHW( SP, IRQ_REQ );   /* save interrupt/trap type tag */
        IRQ_SRV = IRQ_REQ;
        fcw = RDMEM_W( NMI );
        PC = RDMEM_W( NMI + 2 );
        IRQ_REQ &= ~Z8000_NMI;
        CHANGE_FCW(fcw);
        PC = NMI;
        log_cb(RETRO_LOG_DEBUG, LOGPRE "Z8K#%d NMI $%04x\n", cpu_getactivecpu(), PC );
    }
    else
    if ( (IRQ_REQ & Z8000_NVI) && (FCW & F_NVIE) )
    {
        CHANGE_FCW(fcw | F_S_N);/* swap to system stack */
        PUSHW( SP, PC );        /* save current PC */
        PUSHW( SP, fcw );       /* save current FCW */
        PUSHW( SP, IRQ_REQ );   /* save interrupt/trap type tag */
        IRQ_SRV = IRQ_REQ;
        fcw = RDMEM_W( NVI );
        PC = RDMEM_W( NVI + 2 );
        IRQ_REQ &= ~Z8000_NVI;
        CHANGE_FCW(fcw);
        log_cb(RETRO_LOG_DEBUG, LOGPRE "Z8K#%d NVI $%04x\n", cpu_getactivecpu(), PC );
    }
    else
    if ( (IRQ_REQ & Z8000_VI) && (FCW & F_VIE) )
    {
        CHANGE_FCW(fcw | F_S_N);/* swap to system stack */
        PUSHW( SP, PC );        /* save current PC */
        PUSHW( SP, fcw );       /* save current FCW */
        PUSHW( SP, IRQ_REQ );   /* save interrupt/trap type tag */
        IRQ_SRV = IRQ_REQ;
        fcw = RDMEM_W( IRQ_VEC );
        PC = RDMEM_W( VEC00 + 2 * (IRQ_REQ & 0xff) );
        IRQ_REQ &= ~Z8000_VI;
        CHANGE_FCW(fcw);
        log_cb(RETRO_LOG_DEBUG, LOGPRE "Z8K#%d VI [$%04x/$%04x] fcw $%04x, pc $%04x\n", cpu_getactivecpu(), IRQ_VEC, VEC00 + VEC00 + 2 * (IRQ_REQ & 0xff), FCW, PC );
    }
}


void z8000_reset(void *param)
{
	memset(&Z, 0, sizeof(z8000_Regs));
	FCW = RDMEM_W( 2 ); /* get reset FCW */
	PC	= RDMEM_W( 4 ); /* get reset PC  */
	change_pc16bew(PC);
}

void z8000_exit(void)
{
	z8000_deinit();
}

int z8000_execute(int cycles)
{
    z8000_ICount = cycles;

    do
    {
        /* any interrupt request pending? */
        if (IRQ_REQ)
			Interrupt();

		CALL_MAME_DEBUG;

		if (IRQ_REQ & Z8000_HALT)
        {
            z8000_ICount = 0;
        }
        else
        {
            Z8000_exec *exec;
            Z.op[0] = RDOP();
            exec = &z8000_exec[Z.op[0]];

            if (exec->size > 1)
                Z.op[1] = RDOP();
            if (exec->size > 2)
                Z.op[2] = RDOP();

            z8000_ICount -= exec->cycles;
            (*exec->opcode)();

        }
    } while (z8000_ICount > 0);

    return cycles - z8000_ICount;

}

unsigned z8000_get_context(void *dst)
{
	if( dst )
		*(z8000_Regs*)dst = Z;
    return sizeof(z8000_Regs);
}

void z8000_set_context(void *src)
{
	if( src )
	{
		Z = *(z8000_Regs*)src;
		change_pc16bew(PC);
	}
}

unsigned z8000_get_reg(int regnum)
{
	switch( regnum )
	{
		case REG_PC:
		case Z8000_PC: return PC;
		case REG_SP:
        case Z8000_NSP: return NSP;
        case Z8000_FCW: return FCW;
		case Z8000_PSAP: return PSAP;
		case Z8000_REFRESH: return REFRESH;
		case Z8000_IRQ_REQ: return IRQ_REQ;
		case Z8000_IRQ_SRV: return IRQ_SRV;
		case Z8000_IRQ_VEC: return IRQ_VEC;
		case Z8000_R0: return RW( 0);
		case Z8000_R1: return RW( 1);
		case Z8000_R2: return RW( 2);
		case Z8000_R3: return RW( 3);
		case Z8000_R4: return RW( 4);
		case Z8000_R5: return RW( 5);
		case Z8000_R6: return RW( 6);
		case Z8000_R7: return RW( 7);
		case Z8000_R8: return RW( 8);
		case Z8000_R9: return RW( 9);
		case Z8000_R10: return RW(10);
		case Z8000_R11: return RW(11);
		case Z8000_R12: return RW(12);
		case Z8000_R13: return RW(13);
		case Z8000_R14: return RW(14);
		case Z8000_R15: return RW(15);
		case Z8000_NMI_STATE: return Z.nmi_state;
		case Z8000_NVI_STATE: return Z.irq_state[0];
		case Z8000_VI_STATE: return Z.irq_state[1];
		case REG_PREVIOUSPC: return PPC;
		default:
			if( regnum <= REG_SP_CONTENTS )
			{
				unsigned offset = NSP + 2 * (REG_SP_CONTENTS - regnum);
				if( offset < 0xffff )
					return RDMEM_W( offset );
			}
	}
    return 0;
}

void z8000_set_reg(int regnum, unsigned val)
{
	switch( regnum )
	{
		case REG_PC: PC = val; change_pc16bew(PC); break;
		case Z8000_PC: PC = val; break;
		case REG_SP:
		case Z8000_NSP: NSP = val; break;
		case Z8000_FCW: FCW = val; break;
		case Z8000_PSAP: PSAP = val; break;
		case Z8000_REFRESH: REFRESH = val; break;
		case Z8000_IRQ_REQ: IRQ_REQ = val; break;
		case Z8000_IRQ_SRV: IRQ_SRV = val; break;
		case Z8000_IRQ_VEC: IRQ_VEC = val; break;
		case Z8000_R0: RW( 0) = val; break;
		case Z8000_R1: RW( 1) = val; break;
		case Z8000_R2: RW( 2) = val; break;
		case Z8000_R3: RW( 3) = val; break;
		case Z8000_R4: RW( 4) = val; break;
		case Z8000_R5: RW( 5) = val; break;
		case Z8000_R6: RW( 6) = val; break;
		case Z8000_R7: RW( 7) = val; break;
		case Z8000_R8: RW( 8) = val; break;
		case Z8000_R9: RW( 9) = val; break;
		case Z8000_R10: RW(10) = val; break;
		case Z8000_R11: RW(11) = val; break;
		case Z8000_R12: RW(12) = val; break;
		case Z8000_R13: RW(13) = val; break;
		case Z8000_R14: RW(14) = val; break;
		case Z8000_R15: RW(15) = val; break;
		case Z8000_NMI_STATE: Z.nmi_state = val; break;
		case Z8000_NVI_STATE: Z.irq_state[0] = val; break;
		case Z8000_VI_STATE: Z.irq_state[1] = val; break;
		default:
			if( regnum < REG_SP_CONTENTS )
			{
				unsigned offset = NSP + 2 * (REG_SP_CONTENTS - regnum);
				if( offset < 0xffff )
					WRMEM_W( offset, val & 0xffff );
			}
    }
}

void z8000_set_irq_line(int irqline, int state)
{
	if (irqline == IRQ_LINE_NMI)
	{
		if (Z.nmi_state == state)
			return;

	    Z.nmi_state = state;

	    if (state != CLEAR_LINE)
		{
			if (IRQ_SRV >= Z8000_NMI)	/* no NMIs inside trap */
				return;
			IRQ_REQ = Z8000_NMI;
			IRQ_VEC = NMI;
		}
	}
	else if (irqline < 2)
	{
		Z.irq_state[irqline] = state;
		if (irqline == 0)
		{
			if (state == CLEAR_LINE)
			{
				if (!(FCW & F_NVIE))
					IRQ_REQ &= ~Z8000_NVI;
			}
			else
			{
				if (FCW & F_NVIE)
					IRQ_REQ |= Z8000_NVI;
	        }
		}
		else
		{
			if (state == CLEAR_LINE)
			{
				if (!(FCW & F_VIE))
					IRQ_REQ &= ~Z8000_VI;
			}
			else
			{
				if (FCW & F_VIE)
					IRQ_REQ |= Z8000_VI;
			}
		}
	}
}

void z8000_set_irq_callback(int (*callback)(int irqline))
{
	Z.irq_callback = callback;
}

/****************************************************************************
 * Return a formatted string for a register
 ****************************************************************************/
const char *z8000_info(void *context, int regnum)
{
	static char buffer[32][47+1];
	static int which = 0;
	z8000_Regs *r = (z8000_Regs *)context;

	which = (which+1) % 32;
    buffer[which][0] = '\0';
	if( !context )
		r = &Z;

    switch( regnum )
	{
		case CPU_INFO_NAME: return "Z8002";
		case CPU_INFO_FAMILY: return "Zilog Z8000";
		case CPU_INFO_VERSION: return "1.1";
		case CPU_INFO_FILE: return __FILE__;
		case CPU_INFO_CREDITS: return "Copyright (C) 1998,1999 Juergen Buchmueller, all rights reserved.";

		case CPU_INFO_REG_LAYOUT: return (const char*)z8000_reg_layout;
		case CPU_INFO_WIN_LAYOUT: return (const char*)z8000_win_layout;

        case CPU_INFO_FLAGS:
			sprintf(buffer[which], "%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c",
				r->fcw & 0x8000 ? 's':'.',
				r->fcw & 0x4000 ? 'n':'.',
				r->fcw & 0x2000 ? 'e':'.',
				r->fcw & 0x1000 ? '2':'.',
				r->fcw & 0x0800 ? '1':'.',
				r->fcw & 0x0400 ? '?':'.',
				r->fcw & 0x0200 ? '?':'.',
				r->fcw & 0x0100 ? '?':'.',
				r->fcw & 0x0080 ? 'C':'.',
				r->fcw & 0x0040 ? 'Z':'.',
				r->fcw & 0x0020 ? 'S':'.',
				r->fcw & 0x0010 ? 'V':'.',
				r->fcw & 0x0008 ? 'D':'.',
				r->fcw & 0x0004 ? 'H':'.',
				r->fcw & 0x0002 ? '?':'.',
				r->fcw & 0x0001 ? '?':'.');
            break;
		case CPU_INFO_REG+Z8000_PC: sprintf(buffer[which], "PC :%04X", r->pc); break;
		case CPU_INFO_REG+Z8000_NSP: sprintf(buffer[which], "SP :%04X", r->nsp); break;
		case CPU_INFO_REG+Z8000_FCW: sprintf(buffer[which], "FCW:%04X", r->fcw); break;
		case CPU_INFO_REG+Z8000_PSAP: sprintf(buffer[which], "NSP:%04X", r->psap); break;
		case CPU_INFO_REG+Z8000_REFRESH: sprintf(buffer[which], "REFR:%04X", r->refresh); break;
		case CPU_INFO_REG+Z8000_IRQ_REQ: sprintf(buffer[which], "IRQR:%04X", r->irq_req); break;
		case CPU_INFO_REG+Z8000_IRQ_SRV: sprintf(buffer[which], "IRQS:%04X", r->irq_srv); break;
		case CPU_INFO_REG+Z8000_IRQ_VEC: sprintf(buffer[which], "IRQV:%04X", r->irq_vec); break;
#ifdef	MSB_FIRST
#define REG_XOR 0
#else
#define REG_XOR 3
#endif
		case CPU_INFO_REG+Z8000_R0: sprintf(buffer[which], "R0 :%04X", r->regs.W[ 0^REG_XOR]); break;
		case CPU_INFO_REG+Z8000_R1: sprintf(buffer[which], "R1 :%04X", r->regs.W[ 1^REG_XOR]); break;
		case CPU_INFO_REG+Z8000_R2: sprintf(buffer[which], "R2 :%04X", r->regs.W[ 2^REG_XOR]); break;
		case CPU_INFO_REG+Z8000_R3: sprintf(buffer[which], "R3 :%04X", r->regs.W[ 3^REG_XOR]); break;
		case CPU_INFO_REG+Z8000_R4: sprintf(buffer[which], "R4 :%04X", r->regs.W[ 4^REG_XOR]); break;
		case CPU_INFO_REG+Z8000_R5: sprintf(buffer[which], "R5 :%04X", r->regs.W[ 5^REG_XOR]); break;
		case CPU_INFO_REG+Z8000_R6: sprintf(buffer[which], "R6 :%04X", r->regs.W[ 6^REG_XOR]); break;
		case CPU_INFO_REG+Z8000_R7: sprintf(buffer[which], "R7 :%04X", r->regs.W[ 7^REG_XOR]); break;
		case CPU_INFO_REG+Z8000_R8: sprintf(buffer[which], "R8 :%04X", r->regs.W[ 8^REG_XOR]); break;
		case CPU_INFO_REG+Z8000_R9: sprintf(buffer[which], "R9 :%04X", r->regs.W[ 9^REG_XOR]); break;
		case CPU_INFO_REG+Z8000_R10: sprintf(buffer[which], "R10:%04X", r->regs.W[10^REG_XOR]); break;
		case CPU_INFO_REG+Z8000_R11: sprintf(buffer[which], "R11:%04X", r->regs.W[11^REG_XOR]); break;
		case CPU_INFO_REG+Z8000_R12: sprintf(buffer[which], "R12:%04X", r->regs.W[12^REG_XOR]); break;
		case CPU_INFO_REG+Z8000_R13: sprintf(buffer[which], "R13:%04X", r->regs.W[13^REG_XOR]); break;
		case CPU_INFO_REG+Z8000_R14: sprintf(buffer[which], "R14:%04X", r->regs.W[14^REG_XOR]); break;
		case CPU_INFO_REG+Z8000_R15: sprintf(buffer[which], "R15:%04X", r->regs.W[15^REG_XOR]); break;
		case CPU_INFO_REG+Z8000_NMI_STATE: sprintf(buffer[which], "NMI:%X", r->nmi_state); break;
		case CPU_INFO_REG+Z8000_NVI_STATE: sprintf(buffer[which], "NVI:%X", r->irq_state[0]); break;
		case CPU_INFO_REG+Z8000_VI_STATE: sprintf(buffer[which], "VI :%X", r->irq_state[1]); break;
    }
	return buffer[which];
}


unsigned z8000_dasm(char *buffer, unsigned pc)
{
#ifdef MAME_DEBUG
    return DasmZ8000(buffer,pc);
#else
	sprintf( buffer, "$%04X", cpu_readop16(pc) );
	return 2;
#endif
}
