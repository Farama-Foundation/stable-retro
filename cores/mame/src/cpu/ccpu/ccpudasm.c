#include <stdio.h>
#include <string.h>
#include "ccpu.h"

#define CCPU_FETCH(A)	cpu_readop(CCPU_PGM_OFFSET+A)

/*
 * opcode tables for debugging
 */
typedef enum {
  CLR,  LDA,  INP,  ADD,  SUB,  LDJ,  LDP,  OUT,
  CMP,  LDI,  STA,  VIN,  VDR,  XLT,  MUL,  LLT,
  WAI,  AWD,  AND,  LSR,  LSL,  ASR,  ASRD, LSLD,
  JPPB, JMIB, JDRB, JLTB, JEQB, JNCB, JAOB, NOPB,
  JMPA, JMIA, JDRA, JLTA, JEQA, JNCA, JAOA, NOPA,
  JEIA, JEIB
} opcode_mnemonics;

typedef struct {
  opcode_mnemonics od_opcode;
  const char *od_name;
} opcode_detail;

opcode_detail opcodes[] = {
  { CLR, "clr" },
  { LDA, "lda" },
  { INP, "inp" },
  { ADD, "add" },
  { SUB, "sub" },
  { LDJ, "ldj" },
  { LDP, "ldp" },
  { OUT, "out" },
  { CMP, "cmp" },
  { LDI, "ldi" },
  { STA, "sta" },
  { VIN, "vin" },
  { VDR, "vdr" },
  { XLT, "xlt" },
  { MUL, "mul" },
  { LLT, "llt" },
  { WAI, "wai" },
  { AWD, "awd" },
  { AND, "and" },
  { LSR, "lsr" },
  { LSL, "lsl" },
  { ASR, "asr" },
  { ASRD, "asrd" },
  { LSLD, "lsld" },
  { JPPB, "jppb" },
  { JMIB, "jmib" },
  { JDRB, "jdrb" },
  { JLTB, "jltb" },
  { JEQB, "jeqb" },
  { JNCB, "jncb" },
  { JAOB, "jaob" },
  { NOPB, "nopb" },
  { JMPA, "jmpa" },
  { JMIA, "jmia" },
  { JDRA, "jdra" },
  { JLTA, "jlta" },
  { JEQA, "jeqa" },
  { JNCA, "jnca" },
  { JAOA, "jaoa" },
  { NOPA, "nopa" },
  { JEIA, "jeia" },
  { JEIB, "jeib" }
};

typedef enum {
  ACC,      /* Accumulator*/
  ADIR,     /* Acc Direct memory access*/
  AIM4,     /* Acc 4 bit immediate*/
  AIM4X,    /* Acc 4 bit immediate extended size*/
  AIM8,     /* Acc 8 bit immediate*/
  AINDM,    /* Acc indirect through memory*/
  AIMX,     /* Acc immediate extended A-reg*/
  AXLT,     /* Acc lookup ROM using Acc as pointer*/
  AIRG,     /* Acc Through the I-reg*/
  IRG,      /* Through the I-reg*/
  IM4,      /* 4 bit immediate*/
  IM12,     /* 12 bit immediate*/
  DIR,      /* Direct memory access*/
  IMP,      /* Implied*/
  JUMP,     /* Acc selection/Jump instruction*/
  JUMPX     /* Acc selection/Extended jump instruction*/
} amode_mnemonics;

typedef struct {
  amode_mnemonics ad_amode;
  const char *ad_name;
} amode_detail;

amode_detail amodes[] = {
  { ACC,   "acc" },
  { ADIR,  "adir" },
  { AIM4,  "aim4" },
  { AIM4X, "aim4x" },
  { AIM8,  "aim8" },
  { AINDM, "aindm" },
  { AIMX,  "aimx" },
  { AXLT,  "axlt" },
  { AIRG,  "airg" },
  { IRG,   "irg" },
  { IM4,   "im4" },
  { IM12,  "im12" },
  { DIR,   "dir" },
  { IMP,   "imp" },
  { JUMP,  "jump" },
  { JUMPX, "jumpx" }
};

typedef struct {
  opcode_mnemonics ot_opcode;
  amode_mnemonics  ot_amode;
} opcode_table_entry;

opcode_table_entry opcode_table[] = {
  { CLR, ACC   },           /* 00*/
  { LDA, AIM4X },           /* 01*/
  { LDA, AIM4X },           /* 02*/
  { LDA, AIM4X },           /* 03*/
  { LDA, AIM4X },           /* 04*/
  { LDA, AIM4X },           /* 05*/
  { LDA, AIM4X },           /* 06*/
  { LDA, AIM4X },           /* 07*/
  { LDA, AIM4X },           /* 08*/
  { LDA, AIM4X },           /* 09*/
  { LDA, AIM4X },           /* 0A*/
  { LDA, AIM4X },           /* 0B*/
  { LDA, AIM4X },           /* 0C*/
  { LDA, AIM4X },           /* 0D*/
  { LDA, AIM4X },           /* 0E*/
  { LDA, AIM4X },           /* 0F*/

  { INP, ADIR },            /* 10*/
  { INP, ADIR },            /* 11*/
  { INP, ADIR },            /* 12*/
  { INP, ADIR },            /* 13*/
  { INP, ADIR },            /* 14*/
  { INP, ADIR },            /* 15*/
  { INP, ADIR },            /* 16*/
  { INP, ADIR },            /* 17*/
  { INP, ADIR },            /* 18*/
  { INP, ADIR },            /* 19*/
  { INP, ADIR },            /* 1A*/
  { INP, ADIR },            /* 1B*/
  { INP, ADIR },            /* 1C*/
  { INP, ADIR },            /* 1D*/
  { INP, ADIR },            /* 1E*/
  { INP, ADIR },            /* 1F*/

  { ADD, AIM8 },            /* 20*/
  { ADD, AIM4 },            /* 21*/
  { ADD, AIM4 },            /* 22*/
  { ADD, AIM4 },            /* 23*/
  { ADD, AIM4 },            /* 24*/
  { ADD, AIM4 },            /* 25*/
  { ADD, AIM4 },            /* 26*/
  { ADD, AIM4 },            /* 27*/
  { ADD, AIM4 },            /* 28*/
  { ADD, AIM4 },            /* 29*/
  { ADD, AIM4 },            /* 2A*/
  { ADD, AIM4 },            /* 2B*/
  { ADD, AIM4 },            /* 2C*/
  { ADD, AIM4 },            /* 2D*/
  { ADD, AIM4 },            /* 2E*/
  { ADD, AIM4 },            /* 2F*/

  { SUB, AIM8 },            /* 30*/
  { SUB, AIM4 },            /* 31*/
  { SUB, AIM4 },            /* 32*/
  { SUB, AIM4 },            /* 33*/
  { SUB, AIM4 },            /* 34*/
  { SUB, AIM4 },            /* 35*/
  { SUB, AIM4 },            /* 36*/
  { SUB, AIM4 },            /* 37*/
  { SUB, AIM4 },            /* 38*/
  { SUB, AIM4 },            /* 39*/
  { SUB, AIM4 },            /* 3A*/
  { SUB, AIM4 },            /* 3B*/
  { SUB, AIM4 },            /* 3C*/
  { SUB, AIM4 },            /* 3D*/
  { SUB, AIM4 },            /* 3E*/
  { SUB, AIM4 },            /* 3F*/

  { LDJ, IM12 },            /* 40*/
  { LDJ, IM12 },            /* 41*/
  { LDJ, IM12 },            /* 42*/
  { LDJ, IM12 },            /* 43*/
  { LDJ, IM12 },            /* 44*/
  { LDJ, IM12 },            /* 45*/
  { LDJ, IM12 },            /* 46*/
  { LDJ, IM12 },            /* 47*/
  { LDJ, IM12 },            /* 48*/
  { LDJ, IM12 },            /* 49*/
  { LDJ, IM12 },            /* 4A*/
  { LDJ, IM12 },            /* 4B*/
  { LDJ, IM12 },            /* 4C*/
  { LDJ, IM12 },            /* 4D*/
  { LDJ, IM12 },            /* 4E*/
  { LDJ, IM12 },            /* 4F*/

  { JPPB, JUMP },           /* 50*/
  { JMIB, JUMP },           /* 51*/
  { JDRB, JUMP },           /* 52*/
  { JLTB, JUMP },           /* 53*/
  { JEQB, JUMP },           /* 54*/
  { JNCB, JUMP },           /* 55*/
  { JAOB, JUMP },           /* 56*/
  { NOPB, IMP },            /* 57*/

  { JMPA, JUMP },           /* 58*/
  { JMIA, JUMP },           /* 59*/
  { JDRA, JUMP },           /* 5A*/
  { JLTA, JUMP },           /* 5B*/
  { JEQA, JUMP },           /* 5C*/
  { JNCA, JUMP },           /* 5D*/
  { JAOA, JUMP },           /* 5E*/
  { NOPA, IMP },            /* 5F*/

  { ADD, ADIR },            /* 60*/
  { ADD, ADIR },            /* 61*/
  { ADD, ADIR },            /* 62*/
  { ADD, ADIR },            /* 63*/
  { ADD, ADIR },            /* 64*/
  { ADD, ADIR },            /* 65*/
  { ADD, ADIR },            /* 66*/
  { ADD, ADIR },            /* 67*/
  { ADD, ADIR },            /* 68*/
  { ADD, ADIR },            /* 69*/
  { ADD, ADIR },            /* 6A*/
  { ADD, ADIR },            /* 6B*/
  { ADD, ADIR },            /* 6C*/
  { ADD, ADIR },            /* 6D*/
  { ADD, ADIR },            /* 6E*/
  { ADD, ADIR },            /* 6F*/

  { SUB, ADIR },            /* 70*/
  { SUB, ADIR },            /* 71*/
  { SUB, ADIR },            /* 72*/
  { SUB, ADIR },            /* 73*/
  { SUB, ADIR },            /* 74*/
  { SUB, ADIR },            /* 75*/
  { SUB, ADIR },            /* 76*/
  { SUB, ADIR },            /* 77*/
  { SUB, ADIR },            /* 78*/
  { SUB, ADIR },            /* 79*/
  { SUB, ADIR },            /* 7A*/
  { SUB, ADIR },            /* 7B*/
  { SUB, ADIR },            /* 7C*/
  { SUB, ADIR },            /* 7D*/
  { SUB, ADIR },            /* 7E*/
  { SUB, ADIR },            /* 7F*/

  { LDP, IM4  },            /* 80*/
  { LDP, IM4  },            /* 81*/
  { LDP, IM4  },            /* 82*/
  { LDP, IM4  },            /* 83*/
  { LDP, IM4  },            /* 84*/
  { LDP, IM4  },            /* 85*/
  { LDP, IM4  },            /* 86*/
  { LDP, IM4  },            /* 87*/
  { LDP, IM4  },            /* 88*/
  { LDP, IM4  },            /* 89*/
  { LDP, IM4  },            /* 8A*/
  { LDP, IM4  },            /* 8B*/
  { LDP, IM4  },            /* 8C*/
  { LDP, IM4  },            /* 8D*/
  { LDP, IM4  },            /* 8E*/
  { LDP, IM4  },            /* 8F*/

  { OUT, ADIR },            /* 90*/
  { OUT, ADIR },            /* 91*/
  { OUT, ADIR },            /* 92*/
  { OUT, ADIR },            /* 93*/
  { OUT, ADIR },            /* 94*/
  { OUT, ADIR },            /* 95*/
  { OUT, ADIR },            /* 96*/
  { OUT, ADIR },            /* 97*/
  { OUT, ADIR },            /* 98*/
  { OUT, ADIR },            /* 99*/
  { OUT, ADIR },            /* 9A*/
  { OUT, ADIR },            /* 9B*/
  { OUT, ADIR },            /* 9C*/
  { OUT, ADIR },            /* 9D*/
  { OUT, ADIR },            /* 9E*/
  { OUT, ADIR },            /* 9F*/

  { LDA, ADIR },            /* A0*/
  { LDA, ADIR },            /* A1*/
  { LDA, ADIR },            /* A2*/
  { LDA, ADIR },            /* A3*/
  { LDA, ADIR },            /* A4*/
  { LDA, ADIR },            /* A5*/
  { LDA, ADIR },            /* A6*/
  { LDA, ADIR },            /* A7*/
  { LDA, ADIR },            /* A8*/
  { LDA, ADIR },            /* A9*/
  { LDA, ADIR },            /* AA*/
  { LDA, ADIR },            /* AB*/
  { LDA, ADIR },            /* AC*/
  { LDA, ADIR },            /* AD*/
  { LDA, ADIR },            /* AE*/
  { LDA, ADIR },            /* AF*/

  { CMP, ADIR },            /* B0*/
  { CMP, ADIR },            /* B1*/
  { CMP, ADIR },            /* B2*/
  { CMP, ADIR },            /* B3*/
  { CMP, ADIR },            /* B4*/
  { CMP, ADIR },            /* B5*/
  { CMP, ADIR },            /* B6*/
  { CMP, ADIR },            /* B7*/
  { CMP, ADIR },            /* B8*/
  { CMP, ADIR },            /* B9*/
  { CMP, ADIR },            /* BA*/
  { CMP, ADIR },            /* BB*/
  { CMP, ADIR },            /* BC*/
  { CMP, ADIR },            /* BD*/
  { CMP, ADIR },            /* BE*/
  { CMP, ADIR },            /* BF*/

  { LDI, DIR  },            /* C0*/
  { LDI, DIR  },            /* C1*/
  { LDI, DIR  },            /* C2*/
  { LDI, DIR  },            /* C3*/
  { LDI, DIR  },            /* C4*/
  { LDI, DIR  },            /* C5*/
  { LDI, DIR  },            /* C6*/
  { LDI, DIR  },            /* C7*/
  { LDI, DIR  },            /* C8*/
  { LDI, DIR  },            /* C9*/
  { LDI, DIR  },            /* CA*/
  { LDI, DIR  },            /* CB*/
  { LDI, DIR  },            /* CC*/
  { LDI, DIR  },            /* CD*/
  { LDI, DIR  },            /* CE*/
  { LDI, DIR  },            /* CF*/

  { STA, ADIR },            /* D0*/
  { STA, ADIR },            /* D1*/
  { STA, ADIR },            /* D2*/
  { STA, ADIR },            /* D3*/
  { STA, ADIR },            /* D4*/
  { STA, ADIR },            /* D5*/
  { STA, ADIR },            /* D6*/
  { STA, ADIR },            /* D7*/
  { STA, ADIR },            /* D8*/
  { STA, ADIR },            /* D9*/
  { STA, ADIR },            /* DA*/
  { STA, ADIR },            /* DB*/
  { STA, ADIR },            /* DC*/
  { STA, ADIR },            /* DD*/
  { STA, ADIR },            /* DE*/
  { STA, ADIR },            /* DF*/

  { VDR, IMP  },            /* E0*/
  { LDJ, IRG  },            /* E1*/
  { XLT, AXLT },            /* E2*/
  { MUL, IRG  },            /* E3*/
  { LLT, IMP  },            /* E4*/
  { WAI, IMP  },            /* E5*/
  { STA, AIRG },            /* E6*/
  { ADD, AIRG },            /* E7*/
  { SUB, AIRG },            /* E8*/
  { AND, AIRG },            /* E9*/
  { LDA, AIRG },            /* EA*/
  { LSR, ACC  },            /* EB*/
  { LSL, ACC  },            /* EC*/
  { ASR, ACC  },            /* ED*/
  { ASRD, IMP },            /* EE*/
  { LSLD, IMP },            /* EF*/

  { VIN, IMP  },            /* F0*/
  { LDJ, IRG  },            /* F1*/
  { XLT, AXLT },            /* F2*/
  { MUL, IRG  },            /* F3*/
  { LLT, IMP  },            /* F4*/
  { WAI, IMP  },            /* F5*/
  { STA, AIRG },            /* F6*/
  { AWD, AIRG },            /* F7*/
  { SUB, AIRG },            /* F8*/
  { AND, AIRG },            /* F9*/
  { LDA, AIRG },            /* FA*/
  { LSR, ACC  },            /* FB*/
  { LSL, ACC  },            /* FC*/
  { ASR, ACC  },            /* FD*/
  { ASRD, IMP },            /* FE*/
  { LSLD, IMP }             /* FF*/
};

unsigned DasmCCPU(char *buffer, unsigned pc)
{
	char ambuffer [ 40 ];	/* text buffer for addressing mode values */
	CINEBYTE opcode;
	CINEBYTE opsize = 1;

	/* which opcode in opcode table? (includes immediate data) */
	opcode = CCPU_FETCH (pc);

	/* build addressing mode (value to opcode) */
	memset(ambuffer, 0, sizeof(ambuffer));
	switch (amodes [ opcode_table [ opcode ].ot_amode ].ad_amode)
	{
	/* 4-bit immediate value */
	case AIM4:
	case IM4:
		sprintf (ambuffer, "#$%X", opcode & 0x0F);
		break;

	/* 4-bit extended immediate value */
	case AIM4X:
		sprintf (ambuffer, "#$%X", (opcode & 0x0F) << 8);
		break;

	/* 8-bit immediate value */
	case AIM8:
		sprintf (ambuffer, "#$%02X", CCPU_FETCH (pc + 1));
		opsize ++; /* required second byte for value */
		break;

	/* [m] -- indirect through memory */
	case AINDM:
		sprintf (ambuffer, "AINDM/Error");
		break;

	/* [i] -- indirect through 'I' */
	case AIRG:
	case IRG:
		sprintf (ambuffer, "[i]");
		break;

	/* extended 12-bit immediate value */
	case AIMX:
		sprintf (ambuffer, "AIMX/Error");
		break;

	/* no special params */
	case ACC:
	case AXLT:
	case IMP:
		ambuffer [ 0 ] = '\0';
		break;

	/* 12-bit immediate value */
	case IM12:
		sprintf (ambuffer, "#$%03X",
			(CCPU_FETCH (pc) & 0x0F) +
			(CCPU_FETCH (pc + 1) & 0xF0) +
			((CCPU_FETCH (pc + 1) & 0x0F) << 8));
		opsize ++;
		break;

	/* display address of direct addressing modes */
	case ADIR:
	case DIR:
		sprintf (ambuffer, "$%X", opcode & 0x0F);
		break;

	/* jump through J register */
	case JUMP:
		sprintf (ambuffer, "<jump>");
		break;

	/* extended jump */
	case JUMPX:
		sprintf (ambuffer, "JUMPX/Error");
		break;

	} /* switch on addressing mode */

#if 0
    /* build flags dump */
	sprintf (flbuffer,
	    "A=%03X B=%03X I=%03XZ J=%03X P=%X " \
	    "A0=%02X %02X N%X O%X %c%c%c%c",
	    register_A, register_B,
	    register_I << 1,               /* use the <<1 for Zonn style */
	    register_J, register_P, GETA0(),
	    GETFC() /* ? 'C' : 'c' */,
	    cmp_new, cmp_old,
	    ((state == state_A) || (state == state_AA)) ? 'A' : ' ',
	    (state == state_AA) ? 'A' : ' ',
	    ((state == state_B) || (state == state_BB)) ? 'B' : ' ',
	    (state == state_BB) ? 'B' : ' ');
#endif

	/* create final output */
	sprintf (buffer, "%-5s%s",
		opcodes [ opcode_table [ opcode ].ot_opcode ].od_name, ambuffer);

	return opsize;
}
