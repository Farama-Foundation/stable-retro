/*** Glue Code (******************************************

Glue code to hook up Retrocade's CCPU emulator to MAME's
architecture.  Really, it's not so bad!

**********************************************************/

#include "driver.h"
#include "mamedbg.h"
#include "ccpu.h"

static UINT8 ccpu_reg_layout[] = {
	CCPU_PC, CCPU_CFLAG, CCPU_CSTATE, CCPU_A, CCPU_B, CCPU_I, -1,
	CCPU_P, CCPU_J, CCPU_ACC, CCPU_CMP, CCPU_PA0, 0,
};

/* Layout of the debugger windows x,y,w,h */
static UINT8 ccpu_win_layout[] = {
	25, 0,55, 2,	/* register window (top rows) */
	 0, 0,24,22,	/* disassembler window (left colums) */
    25, 3,55, 9,    /* memory #1 window (right, upper middle) */
    25,13,55, 9,    /* memory #2 window (right, lower middle) */
     0,23,80, 1,    /* command line window (bottom rows) */
};


/* the MAME version of the CCPU registers */
typedef struct ccpuRegs
{
    UINT16  accVal;
    UINT16  cmpVal;
    UINT8   pa0;
    UINT8   cFlag;
    UINT16  eRegPC;
    UINT16  eRegA;
    UINT16  eRegB;
    UINT16  eRegI;
    UINT16  eRegJ;
    UINT8   eRegP;
    UINT8   eCState;
} ccpuRegs;

#define CCPU_FETCH(a) 		(cpu_readop(BYTE_XOR_BE(a)+CCPU_PGM_OFFSET))
#define CCPU_READPORT(a)	(cpu_readport16bew_word(a))
#define CCPU_WRITEPORT(a,v) (cpu_writeport16bew_word(a,v))

/*
 * Read a word from given RAM memory location
 */
#define CCPU_RAM_RDMEM(A)	(cpu_readmem16bew_word((A<<1)+CCPU_DATA_OFFSET))

/*
 *	 Write a word to given RAM memory location
 */
#define CCPU_RAM_WRMEM(A,V) (cpu_writemem16bew_word((A<<1)+CCPU_DATA_OFFSET,V))


#define RAW_VECTORS 1

/* this #define works around the naming conflict with the opcode_table
   in cinedbg.c */
#define opcode_table _opcode_table

/* This prototype was missing */
extern void CinemaVectorData (int fromx, int fromy, int tox, int toy, int color);

int ccpu_icount = 1000;


extern UINT16 ioSwitches;
extern UINT16 ioInputs;


void ccpu_init(void)
{
}

void ccpu_reset(void *param)
{
	cineReset();
}

void ccpu_exit(void)
{
	/* nothing to do ? */
}

int ccpu_execute(int cycles)
{
	int newCycles;

	newCycles = cineExec(cycles);
	return newCycles;
}


unsigned ccpu_get_context(void *dst)
{
    if( dst )
    {
        CONTEXTCCPU context;
        ccpuRegs *Regs = dst;
        cGetContext (&context);
        Regs->accVal = context.accVal;
        Regs->cmpVal = context.cmpVal;
        Regs->pa0 = context.pa0;
        Regs->cFlag = context.cFlag;
        Regs->eRegPC = context.eRegPC;
        Regs->eRegA = context.eRegA;
        Regs->eRegB = context.eRegB;
        Regs->eRegI = context.eRegI;
        Regs->eRegJ = context.eRegJ;
        Regs->eRegP = context.eRegP;
        Regs->eCState = context.eCState;
    }
    return sizeof(ccpuRegs);
}


void ccpu_set_context(void *src)
{
	if( src )
	{
		CONTEXTCCPU context;
		ccpuRegs *Regs = src;
		context.accVal = Regs->accVal;
		context.cmpVal = Regs->cmpVal;
		context.pa0 = Regs->pa0;
		context.cFlag = Regs->cFlag;
		context.eRegPC = Regs->eRegPC;
		context.eRegA = Regs->eRegA;
		context.eRegB = Regs->eRegB;
		context.eRegI = Regs->eRegI;
		context.eRegJ = Regs->eRegJ;
		context.eRegP = Regs->eRegP;
		context.eCState = (CINESTATE)Regs->eCState;
		cSetContext (&context);
	}
}


unsigned ccpu_get_reg(int regnum)
{
	CONTEXTCCPU context;
	cGetContext (&context);

	switch( regnum )
	{
		case CCPU_ACC: return context.accVal;
		case CCPU_CMP: return context.cmpVal;
		case CCPU_PA0: return context.pa0;
		case CCPU_CFLAG: return context.cFlag;
		case REG_PC:
		case CCPU_PC: return context.eRegPC;
		case CCPU_A: return context.eRegA;
		case CCPU_B: return context.eRegB;
		case CCPU_I: return context.eRegI;
		case CCPU_J: return context.eRegJ;
		case REG_SP:
		case CCPU_P: return context.eRegP;
		case CCPU_CSTATE: return context.eCState;
/* TODO: return contents of [SP + wordsize * (REG_SP_CONTENTS-regnum)] */
		default:
			if( regnum <= REG_SP_CONTENTS )
				return 0;
	}
	return 0;
}

void ccpu_set_reg(int regnum, unsigned val)
{
	CONTEXTCCPU context;

	cGetContext (&context);
	switch( regnum )
	{
		case CCPU_ACC: context.accVal = val; break;
		case CCPU_CMP: context.cmpVal = val; break;
		case CCPU_PA0: context.pa0 = val; break;
		case CCPU_CFLAG: context.cFlag = val; break;
		case REG_PC:
		case CCPU_PC: context.eRegPC = val; break;
		case CCPU_A: context.eRegA = val; break;
		case CCPU_B: context.eRegB = val; break;
		case CCPU_I: context.eRegI = val; break;
		case CCPU_J: context.eRegJ = val; break;
		case REG_SP:
		case CCPU_P: context.eRegP = val; break;
		case CCPU_CSTATE: context.eCState = (CINESTATE) val; break;
/* TODO: set contents of [SP + wordsize * (REG_SP_CONTENTS-regnum)] */
		default:
			if( regnum <= REG_SP_CONTENTS )
			{
				unsigned offset = /* SP? + */ (REG_SP_CONTENTS-regnum);
				(void)offset;
			}
    }
	cSetContext (&context);
}


void ccpu_set_irq_line(int irqline, int state)
{
	/* nothing to do */
}

void ccpu_set_irq_callback(int (*callback)(int irqline))
{
	/* nothing to do */
}

const char *ccpu_info(void *context, int regnum)
{
	static char buffer[16][47+1];
	static int which = 0;
	CONTEXTCCPU *r = context;

	which = (which+1) % 16;
    buffer[which][0] = '\0';
	if( !context )
	{
        static CONTEXTCCPU tmp;
		cGetContext(&tmp);
		r = &tmp;
	}

    switch( regnum )
	{
        case CPU_INFO_REG+CCPU_PC: sprintf(buffer[which], "PC:%04X", r->eRegPC); break;
		case CPU_INFO_REG+CCPU_CFLAG: sprintf(buffer[which], "C:%02X", r->cFlag); break;
        case CPU_INFO_REG+CCPU_CSTATE: sprintf(buffer[which], "S:%X", r->eCState); break;
		case CPU_INFO_REG+CCPU_A: sprintf(buffer[which], "A:%03X", r->eRegA); break;
		case CPU_INFO_REG+CCPU_B: sprintf(buffer[which], "B:%03X", r->eRegB); break;
		case CPU_INFO_REG+CCPU_I: sprintf(buffer[which], "I:%03X", r->eRegI); break;
        case CPU_INFO_REG+CCPU_P: sprintf(buffer[which], "P:%X", r->eRegP); break;
		case CPU_INFO_REG+CCPU_J: sprintf(buffer[which], "J:%03X", r->eRegJ); break;
		case CPU_INFO_REG+CCPU_ACC: sprintf(buffer[which], "ACC:%03X", r->accVal); break;
        case CPU_INFO_REG+CCPU_CMP: sprintf(buffer[which], "CMP:%03X", r->cmpVal); break;
        case CPU_INFO_REG+CCPU_PA0: sprintf(buffer[which], "PA0:%02X", r->pa0); break;
			break;
		case CPU_INFO_FLAGS:
			/* TODO: no idea how the flags should look like */
			sprintf(buffer[which], "%c-%c%c%c%c",
				(r->cFlag) ? 'C' : 'c',
                (r->eCState == state_A || r->eCState == state_AA) ? 'A':' ',
                (r->eCState == state_A) ? 'A':' ',
                (r->eCState == state_B || r->eCState == state_BB) ? 'B':' ',
                (r->eCState == state_B) ? 'B':' ');
            break;
		case CPU_INFO_NAME: return "CCPU";
		case CPU_INFO_FAMILY: return "Cinematronics CPU";
		case CPU_INFO_VERSION: return "1.0";
		case CPU_INFO_FILE: return __FILE__;
		case CPU_INFO_CREDITS: return "Copyright 1997/1998 Jeff Mitchell and the Retrocade Alliance\nCopyright 1997 Zonn Moore";
		case CPU_INFO_REG_LAYOUT: return (const char *)ccpu_reg_layout;
		case CPU_INFO_WIN_LAYOUT: return (const char *)ccpu_win_layout;
    }
	return buffer[which];

}

/* TODO: hook up the disassembler */
unsigned ccpu_dasm(char *buffer, unsigned pc)
{
#ifdef MAME_DEBUG
	return DasmCCPU(buffer,pc);
#else
    sprintf( buffer, "$%02X", cpu_readop(pc) );
	return 1;
#endif
}

void ccpu_Config (int jmi, int msize, int monitor)
{
	cineSetJMI (jmi);
	cineSetMSize (msize);
	cineSetMonitor (monitor);
}


void ccpu_SetInputs(int inputs, int switches)
{
/*	ioInputs = inputs;
	ioSwitches = switches;*/
}


/* To do:
  - make RAM external */


/*============================================================================================*

	BELOW LIES THE CORE OF THE CCPU. THE CODE WAS KINDLY GIVEN TO MAME BY ZONN MOORE,
	JEFF MITCHELL, AND NEIL BRADLEY. I HAVE PRETTY HEAVILY CLEANED IT UP.

 *============================================================================================*/


/*
 * cinecore.c, cinedbg.c, cineops.c -- Cinematronics game emulator
 * copyright 1997/1998 Jeff Mitchell and the Retrocade Alliance
 * copyright 1997 Zonn Moore
 *
 * This Cinematronics emulator may be freely used in any non-commercial
 * venture, and is to be considered as under the Gnu Public Licence
 * (GPL, copyleft). To avoid a huge deluge of email, I also allow the
 * MAME team usage under the so-called MAME licence for non-commercial
 * use.
 *
 * There are some restrictions, however, mostly to help further development
 * of the emulator and continue the enjoyment these fine games have
 * provided.
 *
 * 1) Jeff Mitchell (skeezix@skeleton.org) is the authoritative maintainer for
 *    the C implementation of the "CCPU" written by Zonn Moore. Modifications
 *    or changes to this code may not be distributed in source form, in whole
 *    or in part, without the written permission of the maintainer. (ie: At
 *    some point after changes have been made, submit them to the maintainer
 *    for inclusion into the authoritative source tree, for all to use.)
 * 2) Credit must be given where appropriate: Jeff Mitchell, author of the
 *    C version. Zonn Moore for the ASM version and all original research,
 *    original documentation, etc. (The C version is a rewrite of Zonn's
 *    brilliant work.) Neil Bradley, for the 32bit conversion of the ASM
 *    core, and technical assistance. Without him, the C version wouldn't
 *    have been written. Where possible, please include references to the
 *    official website for the software (see below), so that others may
 *    easily find the sources.
 * 3) Users of this software are encouraged to periodically check the
 *    official website for new revisions, bugfixes, etc. This is an
 *    ongoing project, with many optimizations yet to be done.
 *
 * Games known to work 100% or nearly so, at some point or another of
 * the emulators development (but not necessarily right now :):
 * RipOff, Solar Quest, Spacewar, Speed Freak, Star Castle, Star Hawk,
 * Tail Gunner, War of the Worlds, Warrior
 *
 * For reference, all of the cinematronics games are:
 * Armor Attack, Barrier, Boxing Bugs, Demon, Ripoff, Solar Quest,
 * Spacewar, Speed Freak, Star Castle, Star Hawk, Sundance, Tail Gunner
 * War of the worlds, Warrior
 * (There is another, but it has not been made available yet)
 *
 * USAGE:
 * 1) The emulator works like any other "processor" emulator
 * 2) It does expect a few variables to be set, however, indicating
 *    which switches were connected on the original boards. (Most
 *    important is the variable to chose whethor or not to use the MI
 *    or EI flag...). See below.
 * 3) The emulator expects the memory to ALREADY be interleaved into
 *    "normalacy". The cinem ROMs are slow to read otherwise...
 * 4) Endianness doesn't matter; its code nice and slowly :)
 * 5) Compile time options may be set to obtain debugging tool, runtime
 *    traces, etc etc.
 * 6) The emulator does the vector handling; you need provide a routine
 *    to receive the vector draw requests, so that it may queue them up
 * 7) Zonn will be documenting the method by which the game palettes
 *    may be derived, where applicable.
 *
 * INITIALIZATION:
 * Take, for example, the game RipOff (the reaosn I got into this mess :).
 * The following variables need to be set, and the functions called, in
 * order to have the "cpu" run properly (courtesy Neil Bradley, Zonn Moore):
 * bFlipX = 0;  bFlipY = 1;     bSwapXY = 0;     (for vector generation)
 * ioInputs = 0xffff; ioSwitches = 0xfff0;  (cleared values)
 * bOverlay = 0xff;                                 (No overlay)
 * cineSetJMI(1);                                      (JMI Allowed)
 * cineSetMSize(0);                            (8K)
 * cineSetMonitor(0);                       (Bi-level display)
 *
 * If masking values are needed by anyone for the various game
 * controllers and whatnot, let me know, and I'll put up a document.
 *
 * SUPPORT:
 * FTP and WEB:
 *   Official support archive is at ftp.retrocade.com and www.retrocade.com.
 *   Various mirrors will be kept (ie: skeleton.org, the Emulation Repository,
 *   erc.), and are yet to be announced.
 * EMAIL:
 *   The C version: Jeff Mitchell is skeezix@skeleton.org
 *   The ASM version: Neil Bradley (neil@synthcom.com) and
 *                    Zonn Moore (zonn@zonn.com)
 *
 * An emulator for the Cinematronics vector game hardware, originally
 * for use in the Retrocade emulator project. All work is based on
 * documentation and sources and testbeds created by Zonn. He's a freak.
 *
 * Last modified: 02/17/98
 *
 * 12/04/97:      Created a "SETFC" macro for storing the flag_C var
 *                (shift and mask it at inspection time)
 * 12/??/97:      Fixed subtraction bugs (using register_B for A0 instead
 *                of register_A). Fixed input mask value in opINP.
 * 12/24/97:      Added #define for CCPUSSTEP for single stepping
 * 12/25/97:      Added ioSwitches &= (!SW_ABORT) into the top
 *                of the cineExec call. This fixed Star Castle, but
 *                broke others :/
 *                  Made opLSLe_A_.. handle multiple consecutive occurances
 *                so things would be a little faster, and so DOS versus
 *                Unix traces would be easier.
 *                  Changed above ioSwitches fix to ~SW_ABORT instead. This
 *                rebroke StarCastle and Sundance, but fixed Barrier. *sigh*
 */

/* Optional #defines for debugging:
 * CCPUBREAK -- causes stopage on unknown upcodes
 * For these to operate, the debugging variables must be set as well.
 * This allows runtime selection of traces, etc.
 * CCPUSSTEP -- force single stepping
 *
 * Debug Variables:
 * ccpubreak -- set to non-zero to enable "break"ing
 */

/* 1) Need to macro-ize everything, so that I can have this whole
 *    source file written by a perl script generator easier.
 * 4) Any of the jumps weird? JNC?
 * 5) JEI's all fucked? Are the tredirector's set right in the first place?
 *    What about all those damned JPP8, 16 and 32s? They work right?
 * 6) Store ccpu_jmi_dip and other new state vars in struct?
 * 7) Various OUT functions correct?
 */

#include <stdio.h>
#include <string.h>

/*
 * Use 0xF000 so as to keep the current page, since it may well
 * have been changed with JPP.
 */
#define JMP() register_PC = ((register_PC - 1) & 0xF000) + register_J; ccpu_icount -= 2

/* Declare needed macros */
#ifdef macintosh
#define UNFINISHED(x)  { SysBeep (0); }
#else
#define UNFINISHED(x)  { log_cb(RETRO_LOG_DEBUG, LOGPRE "UNFINISHED: %s\n", x); }
#endif

/* Handy new operators ... */

/* for Zonn translation :) */
#define SAR(var,arg)    (((signed short int) var) >> arg)

/* for setting/checking the A0 flag */
#define SETA0(var)    (acc_a0 = var)
#define GETA0()       (acc_a0)

/* for setting/checking the Carry flag */
#define SETFC(val)    (flag_C = val)
#define GETFC()       ((flag_C >> 8) & 0xFF)

static int bailOut = FALSE;

/* C-CPU context information begins --  */
static CINEWORD register_PC = 0; /* C-CPU registers; program counter */
static CINEWORD register_A = 0;  /* A-Register (accumulator) */
static CINEWORD register_B = 0;  /* B-Register (accumulator) */
static CINEBYTE register_I = 0;  /* I-Register (last access RAM location) */
static CINEWORD register_J = 0;  /* J-Register (target address for JMP opcodes) */
static CINEBYTE register_P = 0;  /* Page-Register (4 bits) */
static CINEWORD FromX = 0;       /* X-Register (start of a vector) */
static CINEWORD FromY = 0;       /* Y-Register (start of a vector) */
static CINEWORD register_T = 0;  /* T-Register (vector draw length timer) */
static CINEWORD flag_C = 0;      /* C-CPU flags; carry. Is word sized, instead
                                  * of CINEBYTE, so we can do direct assignment
                                  * and then change to BYTE during inspection.
                                  */

static CINEWORD cmp_old = 0;     /* last accumulator value */
static CINEWORD cmp_new = 0;     /* new accumulator value */
static CINEBYTE acc_a0 = 0;      /* bit0 of A-reg at last accumulator access */

static CINESTATE state = state_A;/* C-CPU state machine current state */

/*static CINEWORD  ram[256];       // C-CPU ram (for all pages) */
/*#define CCPU_RAM_RDMEM(a) (ram[a])*/
/*#define CCPU_RAM_WRMEM(a,v) (ram[a]=v)*/

static int ccpu_jmi_dip = 0;     /* as set by cineSetJMI */
static int ccpu_msize = 0;       /* as set by cineSetMSize */
static int ccpu_monitor = 0;     /* as set by cineSetMonitor */

static CINEBYTE vgShiftLength = 0; /* number of shifts loaded into length reg */
static CINEWORD vgColour = 0;
/* -- Context information ends. */


int bNewFrame;

/* Note: I have removed all of this assuming that the vector drawing function can handle things */
#if !RAW_VECTORS
int bFlipX;
int bFlipY;
int bSwapXY;
int bOverlay;

extern int sdwGameXSize;
extern int sdwGameYSize;
extern int sdwXOffset;
extern int sdwYOffset;
#endif



/* functions */

#include "ccputabl.c"

/*
 * cineExec() is what performs all the "processors" work; it will
 * continue to execute until something horrible happens, a watchpoint
 * is hit, cycle count exceeded, or other happy things.
 */

CINELONG cineExec (CINELONG cycles)
{
	ccpu_icount = cycles;
	bailOut = FALSE;

   	do
   	{
   		int opcode;

		CALL_MAME_DEBUG;
		/*
		 * goto the correct piece of code
		 * for the current opcode. That piece of code will set the state
		 * for the next run, as well.
		 */

		opcode = CCPU_FETCH (register_PC++);
		state = (*cineops[state][opcode]) (opcode);
        ccpu_icount -= ccpu_cycles[opcode];


		/*
		 * the opcode code has set a state and done mischief with flags and
		 * the program counter; now jump back to the top and run through another
		 * opcode.
		 */
		if (bailOut)
/*			ccpu_ICount = 0; */
			ccpu_icount -= 100;
	}
	while (ccpu_icount > 0);

	return cycles - ccpu_icount;
}


/*
 * the actual opcode code; each piece should be careful to
 * (1) set the correct state
 * (2) increment the program counter as necessary
 * (3) piss with the flags as needed
 * otherwise the next opcode will be completely buggered.
 */

CINESTATE opINP_A_AA (int opcode)
{
	/*
	 * bottom 4 bits of opcode are the position of the bit we want;
	 * obtain input value, shift over that no, and truncate to last bit.
	 * NOTE: Masking 0x07 does interesting things on Sundance and
	 * others, but masking 0x0F makes RipOff and others actually work :)
	 */

	cmp_new = (CCPU_READPORT (CCPU_PORT_IOINPUTS) >> (opcode & 0x0F)) & 0x01;

	SETA0 (register_A);               /* save old accA bit0 */
	SETFC (register_A);

	cmp_old = register_A;               /* save old accB */
	register_A = cmp_new;               /* load new accB; zero other bits */

	return state_AA;
}

CINESTATE opINP_B_AA (int opcode)
{
	/*
	 * bottom 3 bits of opcode are the position of the bit we want;
	 * obtain Switches value, shift over that no, and truncate to last bit.
	 */

	cmp_new = (CCPU_READPORT (CCPU_PORT_IOSWITCHES) >> (opcode & 0x07)) & 0x01;

	SETA0 (register_A);               /* save old accA bit0 */
	SETFC (register_A);

	cmp_old = register_B;               /* save old accB */
	register_B = cmp_new;               /* load new accB; zero other bits */

	return state_AA;
}

CINESTATE opOUTsnd_A (int opcode)
{
	if (!(register_A & 0x01))
		CCPU_WRITEPORT (CCPU_PORT_IOOUTPUTS, CCPU_READPORT (CCPU_PORT_IOOUTPUTS) | (0x01 << (opcode & 0x07)));
	else
		CCPU_WRITEPORT (CCPU_PORT_IOOUTPUTS, CCPU_READPORT (CCPU_PORT_IOOUTPUTS) & ~(0x01 << (opcode & 0x07)));

	if ((opcode & 0x07) == 0x05)
	{
		/* reset coin counter */
	}

	return state_A;
}

CINESTATE opOUTbi_A_A (int opcode)
{
	if ((opcode & 0x07) != 6)
		return opOUTsnd_A (opcode);

	vgColour = register_A & 0x01 ? 0x0f: 0x07;

	return state_A;
}

CINESTATE opOUT16_A_A (int opcode)
{
	if ((opcode & 0x07) != 6)
		return opOUTsnd_A (opcode);

	if ((register_A & 0x1) != 1)
	{
		vgColour = FromX & 0x0F;

		if (!vgColour)
			vgColour = 1;
	}

	return state_A;
}

CINESTATE opOUT64_A_A (int opcode)
{
	return state_A;
}

CINESTATE opOUTWW_A_A (int opcode)
{
	if ((opcode & 0x07) != 6)
		return opOUTsnd_A (opcode);

	if ((register_A & 0x1) == 1)
	{
		CINEWORD temp_word = ~FromX & 0x0FFF;
		if (!temp_word)   /* black */
			vgColour = 0;
		else
		{   /* non-black */
			if (temp_word & 0x0888)
				/* bright */
				vgColour = ((temp_word >> 1) & 0x04) | ((temp_word >> 6) & 0x02) | ((temp_word >> 11) & 0x01) | 0x08;
			else if (temp_word & 0x0444)
				/* dim bits */
				vgColour = (temp_word & 0x04) | ((temp_word >> 5) & 0x02) | ((temp_word >> 10) & 0x01);
		}
	} /* colour change? == 1 */

	return state_A;
}

CINESTATE opOUTsnd_B (int opcode)
{
	return state_BB;
}

CINESTATE opOUTbi_B_BB (int opcode)
{
	CINEBYTE temp_byte = opcode & 0x07;

	if (temp_byte - 0x06)
		return opOUTsnd_B (opcode);

	vgColour = ((register_B & 0x01) << 3) | 0x07;

	return state_BB;
}

CINESTATE opOUT16_B_BB (int opcode)
{
	CINEBYTE temp_byte = opcode & 0x07;

	if (temp_byte - 0x06)
		return opOUTsnd_B (opcode);

	if ((register_B & 0xFF) != 1)
	{
		vgColour = FromX & 0x0F;

		if (!vgColour)
			vgColour = 1;
	}

	return state_BB;
}

CINESTATE opOUT64_B_BB (int opcode)
{
	return state_BB;
}

CINESTATE opOUTWW_B_BB (int opcode)
{
	return state_BB;
}

  /* LDA imm (0x) */
CINESTATE opLDAimm_A_AA (int opcode)
{
	CINEWORD temp_word = opcode & 0x0F;   /* pick up immediate value */
	temp_word <<= 8;                          /* LDAimm is the HIGH nibble!*/

	cmp_new = temp_word;                      /* set new comparison flag */

	SETA0 (register_A);                     /* save old accA bit0 */
	SETFC (register_A);                     /* ??? clear carry? */

	cmp_old = register_A;                     /* step back cmp flag */
	register_A = temp_word;                   /* set the register */

	return state_AA;                           /* swap state and end opcode */
}

CINESTATE opLDAimm_B_AA (int opcode)
{
	CINEWORD temp_word = opcode & 0x0F;   /* pick up immediate value */
	temp_word <<= 8;                          /* LDAimm is the HIGH nibble!*/

	cmp_new = temp_word;                      /* set new comparison flag */

	SETA0 (register_A);                     /* save old accA bit0 */
	SETFC (register_A);

	cmp_old = register_B;                     /* step back cmp flag */
	register_B = temp_word;                   /* set the register */

	return state_AA;
}

CINESTATE opLDAdir_A_AA (int opcode)
{
	CINEBYTE temp_byte = opcode & 0x0F;        /* snag imm value */

	register_I = (register_P << 4) + temp_byte;  /* set I register */

	cmp_new = CCPU_RAM_RDMEM(register_I);                  /* new acc value */

	SETA0 (register_A);                          /* back up bit0 */
	SETFC (register_A);

	cmp_old = register_A;                          /* store old acc */
	register_A = cmp_new;                          /* store new acc */

	return state_AA;
}

CINESTATE opLDAdir_B_AA (int opcode)
{
	CINEBYTE temp_byte = opcode & 0x0F;        /* snag imm value */

	register_I = (register_P << 4) + temp_byte;  /* set I register */

	cmp_new = CCPU_RAM_RDMEM(register_I);                  /* new acc value */

	SETA0 (register_A);                          /* back up bit0 */
	SETFC (register_A);

	cmp_old = register_B;                          /* store old acc */
	register_B = cmp_new;                          /* store new acc */

	return state_AA;
}

CINESTATE opLDAirg_A_AA (int opcode)
{
	cmp_new = CCPU_RAM_RDMEM(register_I);

	SETA0 (register_A);
	SETFC (register_A);

	cmp_old = register_A;
	register_A = cmp_new;

	return state_AA;
}

CINESTATE opLDAirg_B_AA (int opcode)
{
	cmp_new = CCPU_RAM_RDMEM(register_I);

	SETA0 (register_A);
	SETFC (register_A);

	cmp_old = register_B;
	register_B = cmp_new;

	return state_AA;
}

  /* ADD imm */
CINESTATE opADDimm_A_AA (int opcode)
{
	CINEWORD temp_word = opcode & 0x0F;     /* get imm value */

	cmp_new = temp_word;                        /* new acc value */
	SETA0 (register_A);                       /* save old accA bit0 */
	cmp_old = register_A;                       /* store old acc for later */

	register_A += temp_word;                    /* add values */
	SETFC (register_A);                       /* store carry and extra */
	register_A &= 0xFFF;                        /* toss out >12bit carry */

	return state_AA;
}

CINESTATE opADDimm_B_AA (int opcode)
{
	CINEWORD temp_word = opcode & 0x0F;     /* get imm value */

	cmp_new = temp_word;                        /* new acc value */
	SETA0 (register_A);                       /* save old accA bit0 */
	cmp_old = register_B;                       /* store old acc for later */

	register_B += temp_word;                    /* add values */
	SETFC (register_B);                       /* store carry and extra */
	register_B &= 0xFFF;                        /* toss out >12bit carry */

	return state_AA;
}

  /* ADD imm extended */
CINESTATE opADDimmX_A_AA (int opcode)
{
	cmp_new = CCPU_FETCH (register_PC++);       /* get extended value */
	SETA0 (register_A);                       /* save old accA bit0 */
	cmp_old = register_A;                       /* store old acc for later */

	register_A += cmp_new;                      /* add values */
	SETFC (register_A);                       /* store carry and extra */
	register_A &= 0xFFF;                        /* toss out >12bit carry */

	return state_AA;
}

CINESTATE opADDimmX_B_AA (int opcode)
{
	cmp_new = CCPU_FETCH (register_PC++);       /* get extended value */
	SETA0 (register_A);                       /* save old accA bit0 */
	cmp_old = register_B;                       /* store old acc for later */

	register_B += cmp_new;                      /* add values */
	SETFC (register_B);                       /* store carry and extra */
	register_B &= 0xFFF;                        /* toss out >12bit carry */

	return state_AA;
}

CINESTATE opADDdir_A_AA (int opcode)
{
	CINEBYTE temp_byte = opcode & 0x0F;         /* fetch imm value */

	register_I = (register_P << 4) + temp_byte;   /* set regI addr */

	cmp_new = CCPU_RAM_RDMEM(register_I);                   /* fetch imm real value */
	SETA0 (register_A);                           /* store bit0 */
	cmp_old = register_A;                           /* store old acc value */

	register_A += cmp_new;                          /* do acc operation */
	SETFC (register_A);                           /* store carry and extra */
	register_A &= 0x0FFF;

	return state_AA;
}

CINESTATE opADDdir_B_AA (int opcode)
{
	CINEBYTE temp_byte = opcode & 0x0F;         /* fetch imm value */

	register_I = (register_P << 4) + temp_byte;   /* set regI addr */

	cmp_new = CCPU_RAM_RDMEM(register_I);                   /* fetch imm real value */
	SETA0 (register_A);                           /* store bit0 */
	cmp_old = register_B;                           /* store old acc value */

	register_B += cmp_new;                          /* do acc operation */
	SETFC (register_B);                           /* store carry and extra */
	register_B &= 0x0FFF;

	return state_AA;
}

CINESTATE opAWDirg_A_AA (int opcode)
{
	cmp_new = CCPU_RAM_RDMEM(register_I);
	SETA0 (register_A);
	cmp_old = register_A;

	register_A += cmp_new;
	SETFC (register_A);
	register_A &= 0x0FFF;

	return state_AA;
}

CINESTATE opAWDirg_B_AA (int opcode)
{
	cmp_new = CCPU_RAM_RDMEM(register_I);
	SETA0 (register_A);
	cmp_old = register_B;

	register_B += cmp_new;
	SETFC (register_B);
	register_B &= 0x0FFF;

	return state_AA;
}

CINESTATE opSUBimm_A_AA (int opcode)
{
	/*
	 * 	SUBtractions are negate-and-add instructions of the CCPU; what
	 * 	a pain in the ass.
	 */

	CINEWORD temp_word = opcode & 0x0F;

	cmp_new = temp_word;
	SETA0 (register_A);
	cmp_old = register_A;

	temp_word = (temp_word ^ 0xFFF) + 1;         /* ones compliment */
	register_A += temp_word;                       /* add */
	SETFC (register_A);                          /* pick up top bits */
	register_A &= 0x0FFF;                          /* mask final regA value */

	return state_AA;
}

CINESTATE opSUBimm_B_AA (int opcode)
{
	/*
	 * SUBtractions are negate-and-add instructions of the CCPU; what
	 * a pain in the ass.
	 */

	CINEWORD temp_word = opcode & 0x0F;

	cmp_new = temp_word;
	SETA0 (register_A);
	cmp_old = register_B;

	temp_word = (temp_word ^ 0xFFF) + 1;         /* ones compliment */
	register_B += temp_word;                       /* add */
	SETFC (register_B);                          /* pick up top bits */
	register_B &= 0x0FFF;                          /* mask final regA value */

	return state_AA;
}

CINESTATE opSUBimmX_A_AA (int opcode)
{
	CINEWORD temp_word = CCPU_FETCH (register_PC++);       /* snag imm value */

	cmp_new = temp_word;                          /* save cmp value */
	SETA0 (register_A);                         /* store bit0 */
	cmp_old = register_A;                         /* back up regA */

	temp_word = (temp_word ^ 0xFFF) + 1;        /* ones compliment */
	register_A += temp_word;                      /* add */
	SETFC (register_A);                         /* pick up top bits */
	register_A &= 0x0FFF;                         /* mask final regA value */

	return state_AA;
}

CINESTATE opSUBimmX_B_AA (int opcode)
{
	CINEWORD temp_word = CCPU_FETCH (register_PC++);       /* snag imm value */

	cmp_new = temp_word;                          /* save cmp value */
	SETA0 (register_A);                         /* store bit0 */
	cmp_old = register_B;                         /* back up regA */

	temp_word = (temp_word ^ 0xFFF) + 1;        /* ones compliment */
	register_B += temp_word;                      /* add */
	SETFC (register_B);                         /* pick up top bits */
	register_B &= 0x0FFF;                         /* mask final regA value */

	return state_AA;
}

CINESTATE opSUBdir_A_AA (int opcode)
{
	CINEWORD temp_word = opcode & 0x0F;         /* fetch imm value */

	register_I = (register_P << 4) + temp_word;   /* set regI addr */

	cmp_new = CCPU_RAM_RDMEM(register_I);
	SETA0 (register_A);
	cmp_old = register_A;

	temp_word = (cmp_new ^ 0xFFF) + 1;           /* ones compliment */
	register_A += temp_word;                       /* add */
	SETFC (register_A);                          /* pick up top bits */
	register_A &= 0x0FFF;                          /* mask final regA value */

	return state_AA;
}

CINESTATE opSUBdir_B_AA (int opcode)
{
	CINEWORD temp_word;
	CINEBYTE temp_byte = opcode & 0x0F;         /* fetch imm value */

	register_I = (register_P << 4) + temp_byte;   /* set regI addr */

	cmp_new = CCPU_RAM_RDMEM(register_I);
	SETA0 (register_A);
	cmp_old = register_B;

	temp_word = (cmp_new ^ 0xFFF) + 1;           /* ones compliment */
	register_B += temp_word;                       /* add */
	SETFC (register_B);                          /* pick up top bits */
	register_B &= 0x0FFF;                          /* mask final regA value */

	return state_AA;
}

CINESTATE opSUBirg_A_AA (int opcode)
{
	CINEWORD temp_word;

	/* sub [i] */
	cmp_new = CCPU_RAM_RDMEM(register_I);
	SETA0 (register_A);
	cmp_old = register_A;

	temp_word = (cmp_new ^ 0xFFF) + 1;           /* ones compliment */
	register_A += temp_word;                       /* add */
	SETFC (register_A);                          /* pick up top bits */
	register_A &= 0x0FFF;                          /* mask final regA value */

	return state_AA;
}

CINESTATE opSUBirg_B_AA (int opcode)
{
	CINEWORD temp_word;

	/* sub [i] */
	cmp_new = CCPU_RAM_RDMEM(register_I);
	SETA0 (register_A);
	cmp_old = register_B;

	temp_word = (cmp_new ^ 0xFFF) + 1;           /* ones compliment */
	register_B += temp_word;                       /* add */
	SETFC (register_B);                          /* pick up top bits */
	register_B &= 0x0FFF;                          /* mask final regA value */

	return state_AA;
}

  /* CMP dir */
CINESTATE opCMPdir_A_AA (int opcode)
{
	/*
	 * compare direct mode; don't modify regs, just set carry flag or not.
	 */

	CINEWORD temp_word;
	CINEBYTE temp_byte = opcode & 0x0F;       /* obtain relative addr */

	register_I = (register_P << 4) + temp_byte; /* build real addr */

	temp_word = CCPU_RAM_RDMEM(register_I);
	cmp_new = temp_word;                          /* new acc value */
	SETA0 (register_A);                         /* backup bit0 */
	cmp_old = register_A;                         /* backup old acc */

	temp_word = (temp_word ^ 0xFFF) + 1;        /* ones compliment */
	temp_word += register_A;
	SETFC (temp_word);                          /* pick up top bits */

	return state_AA;
}

CINESTATE opCMPdir_B_AA (int opcode)
{
	CINEWORD temp_word;
	CINEBYTE temp_byte = opcode & 0x0F;       /* obtain relative addr */

	register_I = (register_P << 4) + temp_byte; /* build real addr */

	temp_word = CCPU_RAM_RDMEM(register_I);
	cmp_new = temp_word;                          /* new acc value */
	SETA0 (register_A);                         /* backup bit0 */
	cmp_old = register_B;                         /* backup old acc */

	temp_word = (temp_word ^ 0xFFF) + 1;        /* ones compliment */
	temp_word += register_B;
	SETFC (temp_word);                          /* pick up top bits */

	return state_AA;
}

  /* AND [i] */
CINESTATE opANDirg_A_AA (int opcode)
{
	cmp_new = CCPU_RAM_RDMEM(register_I);                /* new acc value */
	SETA0 (register_A);
	SETFC (register_A);
	cmp_old = register_A;

	register_A &= cmp_new;

	return state_AA;
}

CINESTATE opANDirg_B_AA (int opcode)
{
	cmp_new = CCPU_RAM_RDMEM(register_I);                /* new acc value */
	SETA0 (register_A);
	SETFC (register_A);
	cmp_old = register_B;

	register_B &= cmp_new;

	return state_AA;
}

  /* LDJ imm */
CINESTATE opLDJimm_A_A (int opcode)
{
	CINEBYTE temp_byte = CCPU_FETCH (register_PC++);      /* upper part of address */
	temp_byte = (temp_byte << 4) |             /* Silly CCPU; Swap */
	            (temp_byte >> 4);              /* nibbles */

	/* put the upper 8 bits above the existing 4 bits */
	register_J = (opcode & 0x0F) | (temp_byte << 4);

	return state_A;
}

CINESTATE opLDJimm_B_BB (int opcode)
{
	CINEBYTE temp_byte = CCPU_FETCH (register_PC++);      /* upper part of address */
	temp_byte = (temp_byte << 4) |             /* Silly CCPU; Swap */
	            (temp_byte >> 4);              /* nibbles */

	/* put the upper 8 bits above the existing 4 bits */
	register_J = (opcode & 0x0F) | (temp_byte << 4);

	return state_BB;
}

  /* LDJ irg */
CINESTATE opLDJirg_A_A (int opcode)
{
	/* load J reg from value at last dir addr */
	register_J = CCPU_RAM_RDMEM(register_I);
	return state_A;
}

CINESTATE opLDJirg_B_BB (int opcode)
{
	register_J = CCPU_RAM_RDMEM(register_I);
	return state_BB;
}

  /* LDP imm */
CINESTATE opLDPimm_A_A (int opcode)
{
	/* load page register from immediate */
	register_P = opcode & 0x0F;  /* set page register */
	return state_A;
}

CINESTATE opLDPimm_B_BB (int opcode)
{
	/* load page register from immediate */
	register_P = opcode & 0x0F;  /* set page register */
	return state_BB;
}

  /* LDI dir */
CINESTATE opLDIdir_A_A (int opcode)
{
	/* load regI directly .. */

	CINEBYTE temp_byte = (register_P << 4) +           /* get ram page ... */
	         (opcode & 0x0F); /* and imm half of ram addr.. */

	register_I = CCPU_RAM_RDMEM(temp_byte) & 0xFF;      /* set/mask new register_I */

	return state_A;
}

CINESTATE opLDIdir_B_BB (int opcode)
{
	CINEBYTE temp_byte = (register_P << 4) +           /* get ram page ... */
	         (opcode & 0x0F); /* and imm half of ram addr.. */

	register_I = CCPU_RAM_RDMEM(temp_byte) & 0xFF;      /* set/mask new register_I */

	return state_BB;
}

  /* STA dir */
CINESTATE opSTAdir_A_A (int opcode)
{
	CINEBYTE temp_byte = opcode & 0x0F;        /* snag imm value */

	register_I = (register_P << 4) + temp_byte;  /* set I register */

	CCPU_RAM_WRMEM(register_I, register_A);               /* store acc to RAM */

	return state_A;
}

CINESTATE opSTAdir_B_BB (int opcode)
{
	CINEBYTE temp_byte = opcode & 0x0F;        /* snag imm value */

	register_I = (register_P << 4) + temp_byte;  /* set I register */

	CCPU_RAM_WRMEM(register_I, register_B);               /* store acc to RAM */

	return state_BB;
}

  /* STA irg */
CINESTATE opSTAirg_A_A (int opcode)
{
	/*
	 * STA into address specified in regI. Nice and easy :)
	 */

	CCPU_RAM_WRMEM(register_I, register_A);               /* store acc */

	return state_A;
}

CINESTATE opSTAirg_B_BB (int opcode)
{
	CCPU_RAM_WRMEM(register_I, register_B);               /* store acc */

	return state_BB;
}

  /* XLT */
CINESTATE opXLT_A_AA (int opcode)
{
	/*
	 * XLT is weird; it loads the current accumulator with the bytevalue
	 * at ROM location pointed to by the accumulator; this allows the
	 * program to read the program itself..
	 * 		NOTE! Next opcode is *IGNORED!* because of a twisted side-effect
	 */

	cmp_new = CCPU_FETCH (((register_PC - 1) & 0xF000) + register_A);   /* store new acc value */
	SETA0 (register_A);           /* store bit0 */
	SETFC (register_A);
	cmp_old = register_A;           /* back up acc */

	register_A = cmp_new;           /* new acc value */

	register_PC++;               /* bump PC twice because XLT is fucked up */
	return state_AA;
}

CINESTATE opXLT_B_AA (int opcode)
{
	cmp_new = CCPU_FETCH (((register_PC - 1) & 0xF000) + register_B);   /* store new acc value */
	SETA0 (register_A);           /* store bit0 */
	SETFC (register_A);
	cmp_old = register_B;           /* back up acc */

	register_B = cmp_new;           /* new acc value */

	register_PC++;               /* bump PC twice because XLT is fucked up */
	return state_AA;
}

  /* MUL [i] */
CINESTATE opMULirg_A_AA (int opcode)
{
	CINEBYTE temp_byte = opcode & 0xFF;    /* (for ease and speed) */
	CINEWORD temp_word = CCPU_RAM_RDMEM(register_I);               /* pick up ram value */

	cmp_new = temp_word;

	temp_word <<= 4;                              /* shift into ADD position */
	register_B <<= 4;                             /* get sign bit 15 */
	register_B |= (register_A >> 8);            /* bring in A high nibble */

	register_A = ((register_A & 0xFF) << 8) | /* shift over 8 bits */
	          temp_byte;  /* pick up opcode */

	if (register_A & 0x100)
	{        				   /* 1bit shifted out? */
		register_A = (register_A >> 8) |
		             ((register_B & 0xFF) << 8);

		SETA0 (register_A & 0xFF);                  /* store bit0 */

		register_A >>= 1;
		register_A &= 0xFFF;

		register_B = SAR(register_B,4);
		cmp_old = register_B & 0x0F;

		register_B = SAR(register_B,1);

		register_B &= 0xFFF;
		register_B += cmp_new;

		SETFC (register_B);

		register_B &= 0xFFF;
	}
	else
	{
		register_A = (register_A >> 8) |    /* Bhigh | Alow */
		             ((register_B & 0xFF) << 8);

		temp_word = register_A & 0xFFF;

		SETA0 (temp_word & 0xFF);                   /* store bit0 */
		cmp_old = temp_word;

		temp_word += cmp_new;
		SETFC (temp_word);

		register_A >>= 1;
		register_A &= 0xFFF;

		register_B = SAR(register_B,5);
		register_B &= 0xFFF;
	}

	return state_AA;
}

CINESTATE opMULirg_B_AA (int opcode)
{
	CINEWORD temp_word = CCPU_RAM_RDMEM(register_I);

	cmp_new = temp_word;
	cmp_old = register_B;
	SETA0 (register_A & 0xFF);

	register_B <<= 4;

	register_B = SAR(register_B,5);

	if (register_A & 0x01)
	{
		register_B += temp_word;
		SETFC (register_B);
		register_B &= 0x0FFF;
	}
	else
	{
		temp_word += register_B;
		SETFC (temp_word);
	}

	return state_AA;
}

  /* LSRe */
CINESTATE opLSRe_A_AA (int opcode)
{
	/*
	 * EB; right shift pure; fill new bit with zero.
	 */

	CINEWORD temp_word = 0x0BEB;

	cmp_new = temp_word;
	SETA0 (register_A);
	cmp_old = register_A;

	temp_word += register_A;
	SETFC (temp_word);

	register_A >>= 1;
	return state_AA;
}

CINESTATE opLSRe_B_AA (int opcode)
{
	CINEWORD temp_word = 0x0BEB;

	cmp_new = temp_word;
	SETA0 (register_A);
	cmp_old = register_B;

	temp_word += register_B;
	SETFC (temp_word);

	register_B >>= 1;

	return state_AA;
}

CINESTATE opLSRf_A_AA (int opcode)
{
	UNFINISHED ("opLSRf 1\n");
	return state_AA;
}

CINESTATE opLSRf_B_AA (int opcode)
{
	UNFINISHED ("opLSRf 2\n");
	return state_AA;
}

CINESTATE opLSLe_A_AA (int opcode)
{
	/*
	 * EC; left shift pure; fill new bit with zero *
	 */

	CINEWORD temp_word = 0x0CEC;

	cmp_new = temp_word;
	SETA0 (register_A);
	cmp_old = register_A;

	temp_word += register_A;
	SETFC (temp_word);

	register_A <<= 1;
	register_A &= 0x0FFF;

	return state_AA;
}

CINESTATE opLSLe_B_AA (int opcode)
{
	CINEWORD temp_word = 0x0CEC;                          /* data register */

	cmp_new = temp_word;                         /* magic value */
	SETA0 (register_A);                        /* back up bit0 */
	cmp_old = register_B;                        /* store old acc */

	temp_word += register_B;                     /* add to acc */
	SETFC (temp_word);                         /* store carry flag */
	register_B <<= 1;                            /* add regA to itself */
	register_B &= 0xFFF;                         /* toss excess bits */

	return state_AA;
}

CINESTATE opLSLf_A_AA (int opcode)
{
	UNFINISHED ("opLSLf 1\n");
	return state_AA;
}

CINESTATE opLSLf_B_AA (int opcode)
{
	UNFINISHED ("opLSLf 2\n");
	return state_AA;
}

CINESTATE opASRe_A_AA (int opcode)
{
	/* agh! I dislike these silly 12bit processors :P */

	cmp_new = 0x0DED;
	SETA0 (register_A);           /* store bit0 */
	SETFC (register_A);
	cmp_old = register_A;

	register_A <<= 4; /* get sign bit */
	register_A = SAR(register_A,5);
	register_A &= 0xFFF;

	return state_AA;
}

CINESTATE opASRe_B_AA (int opcode)
{
	cmp_new = 0x0DED;
	SETA0 (register_A);
	SETFC (register_A);
	cmp_old = register_B;

	register_B <<= 4;
	register_B = SAR(register_B,5);
	register_B &= 0x0FFF;

	return state_AA;
}

CINESTATE opASRf_A_AA (int opcode)
{
	UNFINISHED ("opASRf 1\n");
	return state_AA;
}

CINESTATE opASRf_B_AA (int opcode)
{
	UNFINISHED ("opASRf 2\n");
	return state_AA;
}

CINESTATE opASRDe_A_AA (int opcode)
{
	/*
	 * Arithmetic shift right of D (A+B) .. B is high (sign bits).
	 * divide by 2, but leave the sign bit the same. (ie: 1010 -> 1001)
	 */
	CINEWORD temp_word = 0x0EEE;
	CINEWORD temp_word_2;

	cmp_new = temp_word;          /* save new acc value */
	SETA0 (register_A & 0xFF);  /* save old accA bit0 */
	cmp_old = register_A;         /* save old acc */

	temp_word += register_A;
	SETFC (temp_word);

	register_A <<= 4;
	register_B <<= 4;

	temp_word_2 = (register_B >> 4) << 15;
	register_B = SAR(register_B,5);
	register_A = (register_A >> 1) | temp_word_2;
	register_A >>= 4;

	register_B &= 0x0FFF;
	return state_AA;
}

CINESTATE opASRDe_B_AA (int opcode)
{
	CINEWORD temp_word = 0x0EEE;

	cmp_new = temp_word;
	SETA0 (register_A & 0xFF);
	cmp_old = register_B;

	temp_word += register_B;
	SETFC (temp_word);
	register_B <<= 4;
	register_B = SAR(register_B,5);
	register_B &= 0x0FFF;

	return state_AA;
}

CINESTATE opASRDf_A_AA (int opcode)
{
	UNFINISHED ("opASRDf 1\n");
	return state_AA;
}

CINESTATE opASRDf_B_AA (int opcode)
{
	UNFINISHED ("opASRDf 2\n");
	return state_AA;
}

CINESTATE opLSLDe_A_AA (int opcode)
{
	/* LSLDe -- Left shift through both accumulators; lossy in middle. */

	CINEWORD temp_word = 0x0FEF;

	cmp_new = temp_word;
	SETA0 (register_A);
	cmp_old = register_A;

	temp_word += register_A;
	SETFC (temp_word);
	register_A <<= 1;                             /* logical shift left */
	register_A &= 0xFFF;

	register_B <<= 1;
	register_B &= 0xFFF;

	return state_AA;
}

CINESTATE opLSLDe_B_AA (int opcode)
{
	UNFINISHED ("opLSLD 1\n");
	return state_AA;
}

CINESTATE opLSLDf_A_AA (int opcode)
{
	/* LSLDf */

	CINEWORD temp_word = 0x0FFF;

	cmp_new = temp_word;
	SETA0 (register_A);
	cmp_old = register_A;

	temp_word += register_A;
	SETFC (temp_word);

	register_A <<= 1;
	register_A &= 0x0FFF;

	register_B <<= 1;
	register_B &= 0x0FFF;

	return state_AA;
}

CINESTATE opLSLDf_B_AA (int opcode)
{
	/* not 'the same' as the A->AA version above */

	CINEWORD temp_word = 0x0FFF;

	cmp_new = temp_word;
	SETA0 (register_A);
	cmp_old = register_B;

	temp_word += register_B;
	SETFC (temp_word);

	register_B <<= 1;
	register_B &= 0x0FFF;

	return state_AA;
}

CINESTATE opJMP_A_A (int opcode)
{
	/*
	 * simple jump; change PC and continue..
	 */

	JMP();
	return state_A;
}

CINESTATE opJMP_B_BB (int opcode)
{
	JMP();
	return state_BB;
}

CINESTATE opJEI_A_A (int opcode)
{
	if (FromX & 0x800)
		FromX |= 0xF000;
	if (!(CCPU_READPORT (CCPU_PORT_IOOUTPUTS) & 0x80))
	{
		if ((CCPU_READPORT (CCPU_PORT_IN_JOYSTICKY) - (CINESWORD)FromX) < 0x800)
			JMP();
	}
	else
	{
		if ((CCPU_READPORT (CCPU_PORT_IN_JOYSTICKX) - (CINESWORD)FromX) < 0x800)
			JMP();
	}

	return state_A;
}

CINESTATE opJEI_B_BB (int opcode)
{
	if (FromX & 0x800)
		FromX |= 0xF000;
	if (!(CCPU_READPORT (CCPU_PORT_IOOUTPUTS) & 0x80))
	{
		if ((CCPU_READPORT (CCPU_PORT_IN_JOYSTICKY) - (CINESWORD)FromX) < 0x800)
			JMP();
	}
	else
	{
		if ((CCPU_READPORT (CCPU_PORT_IN_JOYSTICKX) - (CINESWORD)FromX) < 0x800)
			JMP();
	}

	return state_BB;
}

CINESTATE opJEI_A_B (int opcode)
{
	if (FromX & 0x800)
		FromX |= 0xF000;
	if (!(CCPU_READPORT (CCPU_PORT_IOOUTPUTS) & 0x80))
	{
		if ((CCPU_READPORT (CCPU_PORT_IN_JOYSTICKY) - (CINESWORD)FromX) < 0x800)
			JMP();
	}
	else
	{
		if ((CCPU_READPORT (CCPU_PORT_IN_JOYSTICKX) - (CINESWORD)FromX) < 0x800)
			JMP();
	}

	return state_B;
}

CINESTATE opJMI_A_A (int opcode)
{
	/*
	 * previous instruction was not an ACC instruction, nor was the
	 * instruction twice back a USB, therefore minus flag test the
	 * current A-reg
	 */

	/* negative acc? */
	if (register_A & 0x800)
		JMP();	  /* yes -- do jump */

	return state_A;
}

CINESTATE opJMI_AA_A (int opcode)
{
	/* previous acc negative? Jump if so... */
	if (cmp_old & 0x800)
		JMP();

	return state_A;
}

CINESTATE opJMI_BB_A (int opcode)
{
	if (register_B & 0x800)
		JMP();

	return state_A;
}

CINESTATE opJMI_B_BB (int opcode)
{
	if (register_A & 0x800)
		JMP();

	return state_BB;
}

CINESTATE opJLT_A_A (int opcode)
{
	/* jump if old acc equals new acc */

	if (cmp_new < cmp_old)
		JMP();

	return state_A;
}

CINESTATE opJLT_B_BB (int opcode)
{
	if (cmp_new < cmp_old)
		JMP();

	return state_BB;
}

CINESTATE opJEQ_A_A (int opcode)
{
	/* jump if equal */

	if (cmp_new == cmp_old)
		JMP();

	return state_A;
}

CINESTATE opJEQ_B_BB (int opcode)
{
	if (cmp_new == cmp_old)
		JMP();

	return state_BB;
}

CINESTATE opJA0_A_A (int opcode)
{
	if (acc_a0 & 0x01)
		JMP();

	return state_A;
}

CINESTATE opJA0_B_BB (int opcode)
{
	if (acc_a0 & 0x01)
		JMP();

	return state_BB;
}

CINESTATE opJNC_A_A (int opcode)
{
	if (!(GETFC() & 0xF0))
		JMP(); /* no carry, so jump */

	return state_A;
}

CINESTATE opJNC_B_BB (int opcode)
{
	if (!(GETFC() & 0xF0))
		JMP(); /* no carry, so jump */

	return state_BB;
}

CINESTATE opJDR_A_A (int opcode)
{
	/*
	 * Calculate number of cycles executed since
	 * last 'VDR' instruction, add two and use as
	 * cycle count, never branch
	 */
	return state_A;
}

CINESTATE opJDR_B_BB (int opcode)
{
	/*
	 * Calculate number of cycles executed since
	 * last 'VDR' instruction, add two and use as
	 * cycle count, never branch
	 */
	return state_BB;
}

CINESTATE opNOP_A_A (int opcode)
{
	return state_A;
}

CINESTATE opNOP_B_BB (int opcode)
{
	return state_BB;
}

CINESTATE opJPP32_A_B (int opcode)
{
	/*
	 * 00 = Offset 0000h
	 * 01 = Offset 1000h
	 * 02 = Offset 2000h
	 * 03 = Offset 3000h
	 * 04 = Offset 4000h
	 * 05 = Offset 5000h
	 * 06 = Offset 6000h
	 * 07 = Offset 7000h
	 */
	CINEWORD temp_word = (register_P & 0x07) << 12;  /* rom offset */
	register_PC = register_J + temp_word;
	return state_B;
}

CINESTATE opJPP32_B_BB (int opcode)
{
	CINEWORD temp_word = (register_P & 0x07) << 12;  /* rom offset */
	register_PC = register_J + temp_word;
	return state_BB;
}

CINESTATE opJPP16_A_B (int opcode)
{
	/*
	 * 00 = Offset 0000h
	 * 01 = Offset 1000h
	 * 02 = Offset 2000h
	 * 03 = Offset 3000h
	 */
	CINEWORD temp_word = (register_P & 0x03) << 12;  /* rom offset */
	register_PC = register_J + temp_word;
	return state_B;
}

CINESTATE opJPP16_B_BB (int opcode)
{
	CINEWORD temp_word = (register_P & 0x03) << 12;  /* rom offset */
	register_PC = register_J + temp_word;
	return state_BB;
}

CINESTATE opJMP_A_B (int opcode)
{
	JMP();
	return state_B;
}

CINESTATE opJPP8_A_B (int opcode)
{
	/*
	 * "long jump"; combine P and J to jump to a new far location (that can
	 * 	be more than 12 bits in address). After this jump, further jumps
	 * are local to this new page.
	 */
	CINEWORD temp_word = ((register_P & 0x03) - 1) << 12;  /* rom offset */
	register_PC = register_J + temp_word;
	return state_B;
}

CINESTATE opJPP8_B_BB (int opcode)
{
	CINEWORD temp_word = ((register_P & 0x03) - 1) << 12;  /* rom offset */
	register_PC = register_J + temp_word;
	return state_BB;
}

CINESTATE opJMI_A_B (int opcode)
{
	if (register_A & 0x800)
		JMP();

	return state_B;
}

CINESTATE opJMI_AA_B (int opcode)
{
	UNFINISHED ("opJMI 3\n");
	return state_B;
}

CINESTATE opJMI_BB_B (int opcode)
{
	UNFINISHED ("opJMI 4\n");
	return state_B;
}

CINESTATE opJLT_A_B (int opcode)
{
	if (cmp_new < cmp_old)
		JMP();

	return state_B;
}

CINESTATE opJEQ_A_B (int opcode)
{
	if (cmp_new == cmp_old)
		JMP();

	return state_B;
}

CINESTATE opJA0_A_B (int opcode)
{
	if (GETA0() & 0x01)
		JMP();

	return state_B;
}

CINESTATE opJNC_A_B (int opcode)
{
	if (!(GETFC() & 0x0F0))
		JMP(); /* if no carry, jump */

	return state_B;
}

CINESTATE opJDR_A_B (int opcode)
{
	/* register_PC++; */
	log_cb(RETRO_LOG_DEBUG, LOGPRE "The hell? No PC incrementing?\n");
	return state_B;
}

  /* NOP */
CINESTATE opNOP_A_B (int opcode)
{
	return state_B;
}

CINESTATE opLLT_A_AA (int opcode)
{
	CINEBYTE temp_byte = 0;

	while (1)
	{
		CINEWORD temp_word = register_A >> 8;  /* register_A's high bits */
		temp_word &= 0x0A;                   /* only want PA11 and PA9 */

		if (temp_word)
		{
			temp_word ^= 0x0A;                   /* flip the bits */

			if (temp_word)
				break;                	        /* if not zero, mismatch found */
		}

		temp_word = register_B >> 8;         /* regB's top bits */
		temp_word &= 0x0A;                   /* only want SA11 and SA9 */

		if (temp_word)
		{
			temp_word ^= 0x0A;                   /* flip bits */

			if (temp_word)
				break;                          /* if not zero, mismatch found */
		}

		register_A <<= 1;                    /* shift regA */
		register_B <<= 1;                    /* shift regB */

		temp_byte ++;
		if (!temp_byte)
			return state_AA;
        /* try again */
	}

	vgShiftLength = temp_byte;
	register_A &= 0x0FFF;
	register_B &= 0x0FFF;
	return state_AA;
}

CINESTATE opLLT_B_AA (int opcode)
{
	UNFINISHED ("opLLT 1\n");
	return state_AA;
}

CINESTATE opVIN_A_A (int opcode)
{
	/* set the starting address of a vector */

	FromX = register_A & 0xFFF;            /* regA goes to x-coord */
	FromY = register_B & 0xFFF;            /* regB goes to y-coord */

	return state_A;
}

CINESTATE opVIN_B_BB (int opcode)
{
	FromX = register_A & 0xFFF;            /* regA goes to x-coord */
	FromY = register_B & 0xFFF;            /* regB goes to y-coord */

	return state_BB;
}

CINESTATE opWAI_A_A (int opcode)
{
	/* wait for a tick on the watchdog */
	bNewFrame = 1;
	bailOut = TRUE;
	return state;
}

CINESTATE opWAI_B_BB (int opcode)
{
	bNewFrame = 1;
	bailOut = TRUE;
	return state;
}

CINESTATE opVDR_A_A (int opcode)
{
	/* set ending points and draw the vector, or buffer for a later draw. */
	int ToX = register_A & 0xFFF;
	int ToY = register_B & 0xFFF;

	/*
	 * shl 20, sar 20; this means that if the CCPU reg should be -ve,
	 * we should be negative as well.. sign extended.
	 */
	if (FromX & 0x800)
		FromX |= 0xFFFFF000;
	if (ToX & 0x800)
		ToX |= 0xFFFFF000;
	if (FromY & 0x800)
		FromY |= 0xFFFFF000;
	if (ToY & 0x800)
		ToY |= 0xFFFFF000;

	/* figure out the vector */
	ToX -= FromX;
	ToX = SAR(ToX,vgShiftLength);
	ToX += FromX;

	ToY -= FromY;
	ToY = SAR(ToY,vgShiftLength);
	ToY += FromY;

	/* do orientation flipping, etc. */
	/* NOTE: this has been removed on the assumption that the vector draw routine can do it all */
#if !RAW_VECTORS
	if (bFlipX)
	{
		ToX = sdwGameXSize - ToX;
		FromX = sdwGameXSize - FromX;
	}

	if (bFlipY)
	{
		ToY = sdwGameYSize - ToY;
		FromY = sdwGameYSize - FromY;
	}

	FromX += sdwXOffset;
	ToX += sdwXOffset;

	FromY += sdwYOffset;
	ToY += sdwYOffset;

	/* check real coords */
	if (bSwapXY)
	{
		CINEWORD temp_word;

		temp_word = ToY;
		ToY = ToX;
		ToX = temp_word;

		temp_word = FromY;
		FromY = FromX;
		FromX = temp_word;
	}
#endif

	/* render the line */
	CinemaVectorData (FromX, FromY, ToX, ToY, vgColour);

	return state_A;
}

CINESTATE opVDR_B_BB (int opcode)
{
	UNFINISHED ("opVDR B 1\n");
	return state_BB;
}

/*
 * some code needs to be changed based on the machine or switches set.
 * Instead of getting disorganized, I'll put the extra dispatchers
 * here. The main dispatch loop jumps here, checks options, and
 * redispatches to the actual opcode handlers.
 */

/* JPP series of opcodes */
CINESTATE tJPP_A_B (int opcode)
{
	/* MSIZE -- 0 = 4k, 1 = 8k, 2 = 16k, 3 = 32k */
	switch (ccpu_msize)
	{
		case CCPU_MEMSIZE_4K:
		case CCPU_MEMSIZE_8K:
			return opJPP8_A_B (opcode);
		case CCPU_MEMSIZE_16K:
			return opJPP16_A_B (opcode);
		case CCPU_MEMSIZE_32K:
			return opJPP32_A_B (opcode);
	}
	log_cb(RETRO_LOG_DEBUG, LOGPRE "Out of range JPP!\n");
	return opJPP32_A_B (opcode);
}

CINESTATE tJPP_B_BB (int opcode)
{
	/* MSIZE -- 0 = 4k, 1 = 8k, 2 = 16k, 3 = 32k */
	switch (ccpu_msize)
	{
		case CCPU_MEMSIZE_4K:
		case CCPU_MEMSIZE_8K:
			return opJPP8_B_BB (opcode);
		case CCPU_MEMSIZE_16K:
			return opJPP16_B_BB (opcode);
		case CCPU_MEMSIZE_32K:
			return opJPP32_B_BB (opcode);
	}
	log_cb(RETRO_LOG_DEBUG, LOGPRE "Out of range JPP!\n");
	return state;
}

/* JMI series of opcodes */

CINESTATE tJMI_A_B (int opcode)
{
	return (ccpu_jmi_dip) ? opJMI_A_B (opcode) : opJEI_A_B (opcode);
}

CINESTATE tJMI_A_A (int opcode)
{
	return (ccpu_jmi_dip) ? opJMI_A_A (opcode) : opJEI_A_A (opcode);
}

CINESTATE tJMI_AA_B (int opcode)
{
	return (ccpu_jmi_dip) ? opJMI_AA_B (opcode) : opJEI_AA_B (opcode);
}

CINESTATE tJMI_AA_A (int opcode)
{
	return (ccpu_jmi_dip) ? opJMI_AA_A (opcode) : opJEI_A_A (opcode);
}

CINESTATE tJMI_B_BB1 (int opcode)
{
	return (ccpu_jmi_dip) ? opJMI_B_BB (opcode) : opJEI_B_BB (opcode);
}

CINESTATE tJMI_BB_B (int opcode)
{
	return (ccpu_jmi_dip) ? opJMI_BB_B (opcode) : opJEI_A_B (opcode);
}

CINESTATE tJMI_BB_A (int opcode)
{
	return (ccpu_jmi_dip) ? opJMI_BB_A (opcode) : opJEI_A_A (opcode);
}

/*
 * OUT series of opcodes:
 * ccpu_monitor can be one of:
 * 1 -- 16-level colour
 * 2 -- 64-level colour
 * 3 -- War of the Worlds colour
 * other -- bi-level
 */
CINESTATE tOUT_A_A (int opcode)
{
	switch (ccpu_monitor)
	{
		case CCPU_MONITOR_16LEV:
			return opOUT16_A_A (opcode);
		case CCPU_MONITOR_64LEV:
			return opOUT64_A_A (opcode);
		case CCPU_MONITOR_WOWCOL:
			return opOUTWW_A_A (opcode);
		default:
			return opOUTbi_A_A (opcode);
	}
}

CINESTATE tOUT_B_BB (int opcode)
{
	switch (ccpu_monitor)
	{
		case CCPU_MONITOR_16LEV:
			return opOUT16_B_BB (opcode);
		case CCPU_MONITOR_64LEV:
			return opOUT64_B_BB (opcode);
		case CCPU_MONITOR_WOWCOL:
			return opOUTWW_B_BB (opcode);
		default:
			return opOUTbi_B_BB (opcode);
	}
}

/* Reset C-CPU registers, flags, etc to default starting values
 */
void cineReset(void)
{
	/* zero registers */
	register_PC = 0;
	register_A = 0;
	register_B = 0;
	register_I = 0;
	register_J = 0;
	register_P = 0;
	FromX = 0;
	FromY = 0;
	register_T = 0;

	/* zero flags */
	flag_C = 0;

	/* reset state */
	state = state_A;

	/* reset internal state */
	cmp_old = 0;
	cmp_new = 0;
	SETA0 (0);
}

void cineSetJMI(int j)
{
	ccpu_jmi_dip = j;
/*
	if (ccpu_jmi_dip)
		fprintf (stderr, "CCPU JMI Set: Yes.\n");
	else
		fprintf (stderr, "CCPU JMI Set: No.\n");
*/
}

void cineSetMSize(int m)
{
	ccpu_msize = m;
/*
	switch (m)
	{
		case 0:
			fprintf (stderr, "CCPU Address Space: 4k\n");
			break;
		case 1:
			fprintf (stderr, "CCPU Address Space: 8k\n");
			break;
		case 2:
			fprintf (stderr, "CCPU Address Space: 16k\n");
			break;
		case 3:
			fprintf (stderr, "CCPU Address Space: 32k\n");
			break;
		default:
			fprintf (stderr, "CCPU Address Space: Error\n");
			break;
	}
*/
}

void cineSetMonitor(int m)
{
	ccpu_monitor = m;
/*
	switch (m)
	{
		case 1:
			fprintf (stderr, "CCPU Monitor: 16-colour\n");
			break;
		case 2:
			fprintf (stderr, "CCPU Monitor: 64-colour\n");
			break;
		case 3:
			fprintf (stderr, "CCPU Monitor: War-of-the-Worlds-colour\n");
			break;
		default:
			fprintf (stderr, "CCPU Monitor: bi-level-display\n");
			break;
	}
*/
}

void cSetContext(CONTEXTCCPU *c)
{
	cmp_old = c -> accVal;
	cmp_new = c -> cmpVal;
	SETA0 (c -> pa0);
	flag_C = c -> cFlag;
	register_PC = c -> eRegPC;
	register_A = c -> eRegA;
	register_B = c -> eRegB;
	register_I = c -> eRegI;
	register_J = c -> eRegJ;
	register_P = c -> eRegP;
	state = (CINESTATE)c -> eCState;
}

void cGetContext(CONTEXTCCPU *c)
{
	c -> accVal = cmp_old;
	c -> cmpVal = cmp_new;
	c -> pa0 = GETA0();
	c -> cFlag = GETFC();
	c -> eRegPC = register_PC;
	c -> eRegA = register_A;
	c -> eRegB = register_B;
	c -> eRegI = register_I;
	c -> eRegJ = register_J;
	c -> eRegP = register_P;
	c -> eCState = state;
}
