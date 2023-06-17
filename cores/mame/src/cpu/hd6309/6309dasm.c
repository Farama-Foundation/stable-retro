/* this code was hacked out of the fully-featured 6809 disassembler by Sean Riddle */
/* and then mutliated into a 6309 disassembler by tim lindner					   */

/* 6309dasm.c - a 6309 opcode disassembler		*/
/* Version 1.0 5-AUG-2000						*/
/* Copyright © 2000 tim lindner 				*/
/*												*/
/* based on:									*/
/*		6809dasm.c - a 6809 opcode disassembler */
/*		Version 1.4 1-MAR-95					*/
/*		Copyright © 1995 Sean Riddle			*/
/*												*/
/*		thanks to Franklin Bowen for bug fixes, ideas */

/* Freely distributable on any medium given all copyrights are retained */
/* by the author and no charge greater than $7.00 is made for obtaining */
/* this software */

/* Please send all bug reports, update ideas and data files to: */
/* tlindner@ix.netcom.com */

#include <stdio.h>

#ifdef MAME_DEBUG

#include <string.h>
#include "osd_cpu.h"
#include "cpuintrf.h"
#include "mamedbg.h"
#include "hd6309.h"

#ifndef TRUE
#define TRUE	-1
#define FALSE	0
#endif

typedef struct {				/* opcode structure */
   UINT8	opcode; 			/* 8-bit opcode value */
   UINT8	numoperands;
   char 	name[6];			/* opcode name */
   UINT8	mode;				/* addressing mode */
   UINT8	size;				/* access size */
   UINT8	access; 			/* access mode */
   UINT8	numcycles;			/* number of cycles - not used */
} opcodeinfo;

/* 6309 ADDRESSING MODES */
enum HD6309_ADDRESSING_MODES {
	INH,
	DIR,
	IND,
	REL,
	EXT,
	IMM,
	LREL,
	PG2,						/* PAGE SWITCHES -	Page 2 */
	PG3 						/*					Page 3 */
};

/* number of opcodes in each page */
/*#define NUMPG1OPS 237*/
/*#define NUMPG2OPS 118*/
/*#define NUMPG3OPS 83*/

#if 0
static char modenames[10][19] = {
   "inherent",
   "direct",
   "indexed",
   "relative",
   "extended",
   "immediate",
   "long relative",
   "page 2",
   "page 3",
};
#endif

/* page 1 ops */
static opcodeinfo pg1opcodes[] =
{
	{ 0x00,1,"NEG",     DIR, EA_UINT8,  EA_ZPG_RDWR,  6},
	{ 0x01,2,"OIM",     DIR, EA_UINT8,  EA_ZPG_RDWR,  6},
	{ 0x02,2,"AIM",     DIR, EA_UINT8,  EA_ZPG_RDWR,  6},
	{ 0x03,1,"COM",     DIR, EA_UINT8,  EA_ZPG_RDWR,  6},
	{ 0x04,1,"LSR",     DIR, EA_UINT8,  EA_ZPG_RDWR,  6},
	{ 0x05,2,"EIM",     DIR, EA_UINT8,  EA_ZPG_RDWR,  6},
	{ 0x06,1,"ROR",     DIR, EA_UINT8,  EA_ZPG_RDWR,  6},
	{ 0x07,1,"ASR",     DIR, EA_UINT8,  EA_ZPG_RDWR,  6},
	{ 0x08,1,"ASL",     DIR, EA_UINT8,  EA_ZPG_RDWR,  6},
	{ 0x09,1,"ROL",     DIR, EA_UINT8,  EA_ZPG_RDWR,  6},
	{ 0x0a,1,"DEC",     DIR, EA_UINT8,  EA_ZPG_RDWR,  6},
	{ 0x0b,2,"TIM",     DIR, EA_UINT8,  EA_ZPG_RD,    6},  /* Unsure about this cycle count (TL)*/
	{ 0x0c,1,"INC",     DIR, EA_UINT8,  EA_ZPG_RDWR,  6},
	{ 0x0d,1,"TST",     DIR, EA_UINT8,  EA_ZPG_RDWR,  6},
	{ 0x0e,1,"JMP",     DIR, EA_UINT8,  EA_ABS_PC,    3},
	{ 0x0f,1,"CLR",     DIR, EA_UINT8,  EA_ZPG_WR,    6},

	{ 0x10,1,"page2",   PG2, 0,        0,            0},
	{ 0x11,1,"page3",   PG3, 0,        0,            0},
	{ 0x12,0,"NOP",     INH, 0,        0,            2},
	{ 0x13,0,"SYNC",    INH, 0,        0,            4},
	{ 0x14,0,"SEXW",    INH, 0,        0,            4},
	{ 0x16,2,"LBRA",    LREL,EA_INT16, EA_REL_PC,    5},
	{ 0x17,2,"LBSR",    LREL,EA_INT16, EA_REL_PC,    9},
	{ 0x19,0,"DAA",     INH, 0,        0,            2},
	{ 0x1a,1,"ORCC",    IMM, 0,        0,            3},
	{ 0x1c,1,"ANDCC",   IMM, 0,        0,            3},
	{ 0x1d,0,"SEX",     INH, 0,        0,            2},
	{ 0x1e,1,"EXG",     IMM, 0,        0,            8},
	{ 0x1f,1,"TFR",     IMM, 0,        0,            6},

	{ 0x20,1,"BRA",     REL, EA_INT8,  EA_REL_PC,    3},
	{ 0x21,1,"BRN",     REL, EA_INT8,  EA_REL_PC,    3},
	{ 0x22,1,"BHI",     REL, EA_INT8,  EA_REL_PC,    3},
	{ 0x23,1,"BLS",     REL, EA_INT8,  EA_REL_PC,    3},
	{ 0x24,1,"BCC",     REL, EA_INT8,  EA_REL_PC,    3},
	{ 0x25,1,"BCS",     REL, EA_INT8,  EA_REL_PC,    3},
	{ 0x26,1,"BNE",     REL, EA_INT8,  EA_REL_PC,    3},
	{ 0x27,1,"BEQ",     REL, EA_INT8,  EA_REL_PC,    3},
	{ 0x28,1,"BVC",     REL, EA_INT8,  EA_REL_PC,    3},
	{ 0x29,1,"BVS",     REL, EA_INT8,  EA_REL_PC,    3},
	{ 0x2a,1,"BPL",     REL, EA_INT8,  EA_REL_PC,    3},
	{ 0x2b,1,"BMI",     REL, EA_INT8,  EA_REL_PC,    3},
	{ 0x2c,1,"BGE",     REL, EA_INT8,  EA_REL_PC,    3},
	{ 0x2d,1,"BLT",     REL, EA_INT8,  EA_REL_PC,    3},
	{ 0x2e,1,"BGT",     REL, EA_INT8,  EA_REL_PC,    3},
	{ 0x2f,1,"BLE",     REL, EA_INT8,  EA_REL_PC,    3},

	{ 0x30,1,"LEAX",    IND, EA_UINT16,EA_VALUE,     2},
	{ 0x31,1,"LEAY",    IND, EA_UINT16,EA_VALUE,     2},
	{ 0x32,1,"LEAS",    IND, EA_UINT16,EA_VALUE,     2},
	{ 0x33,1,"LEAU",    IND, EA_UINT16,EA_VALUE,     2},
	{ 0x34,1,"PSHS",    INH, 0,        0,            5},
	{ 0x35,1,"PULS",    INH, 0,        0,            5},
	{ 0x36,1,"PSHU",    INH, 0,        0,            5},
	{ 0x37,1,"PULU",    INH, 0,        0,            5},
	{ 0x39,0,"RTS",     INH, 0,        0,            5},
	{ 0x3A,0,"ABX",     INH, 0,        0,            3},
	{ 0x3B,0,"RTI",     INH, 0,        0,            6},
	{ 0x3C,1,"CWAI",    IMM, 0,        0,           20},
	{ 0x3D,0,"MUL",     INH, 0,        0,           11},
	{ 0x3F,0,"SWI",     INH, 0,        0,           19},

	{ 0x40,0,"NEGA",    INH, 0,        0,            2},
	{ 0x43,0,"COMA",    INH, 0,        0,            2},
	{ 0x44,0,"LSRA",    INH, 0,        0,            2},
	{ 0x46,0,"RORA",    INH, 0,        0,            2},
	{ 0x47,0,"ASRA",    INH, 0,        0,            2},
	{ 0x48,0,"ASLA",    INH, 0,        0,            2},
	{ 0x49,0,"ROLA",    INH, 0,        0,            2},
	{ 0x4A,0,"DECA",    INH, 0,        0,            2},
	{ 0x4C,0,"INCA",    INH, 0,        0,            2},
	{ 0x4D,0,"TSTA",    INH, 0,        0,            2},
	{ 0x4F,0,"CLRA",    INH, 0,        0,            2},

	{ 0x50,0,"NEGB",    INH, 0,        0,            2},
	{ 0x53,0,"COMB",    INH, 0,        0,            2},
	{ 0x54,0,"LSRB",    INH, 0,        0,            2},
	{ 0x56,0,"RORB",    INH, 0,        0,            2},
	{ 0x57,0,"ASRB",    INH, 0,        0,            2},
	{ 0x58,0,"ASLB",    INH, 0,        0,            2},
	{ 0x59,0,"ROLB",    INH, 0,        0,            2},
	{ 0x5A,0,"DECB",    INH, 0,        0,            2},
	{ 0x5C,0,"INCB",    INH, 0,        0,            2},
	{ 0x5D,0,"TSTB",    INH, 0,        0,            2},
	{ 0x5F,0,"CLRB",    INH, 0,        0,            2},

	{ 0x60,1,"NEG",     IND, EA_UINT8,  EA_MEM_RDWR,  6},
	{ 0x61,2,"OIM",     IND, EA_UINT8,  EA_MEM_RDWR,  7},
	{ 0x62,2,"AIM",     IND, EA_UINT8,  EA_MEM_RDWR,  7},
	{ 0x63,1,"COM",     IND, EA_UINT8,  EA_MEM_RDWR,  6},
	{ 0x64,1,"LSR",     IND, EA_UINT8,  EA_MEM_RDWR,  6},
	{ 0x65,2,"EIM",     IND, EA_UINT8,  EA_MEM_RDWR,  7},
	{ 0x66,1,"ROR",     IND, EA_UINT8,  EA_MEM_RDWR,  6},
	{ 0x67,1,"ASR",     IND, EA_UINT8,  EA_MEM_RDWR,  6},
	{ 0x68,1,"ASL",     IND, EA_UINT8,  EA_MEM_RDWR,  6},
	{ 0x69,1,"ROL",     IND, EA_UINT8,  EA_MEM_RDWR,  6},
	{ 0x6A,1,"DEC",     IND, EA_UINT8,  EA_MEM_RDWR,  6},
	{ 0x6B,2,"TIM",     IND, EA_UINT8,  EA_MEM_RD,    7},
	{ 0x6C,1,"INC",     IND, EA_UINT8,  EA_MEM_RDWR,  6},
	{ 0x6D,1,"TST",     IND, EA_UINT8,  EA_MEM_RD,    6},
	{ 0x6E,1,"JMP",     IND, EA_UINT8,  EA_ABS_PC,    3},
	{ 0x6F,1,"CLR",     IND, EA_UINT8,  EA_MEM_WR,    6},

	{ 0x70,2,"NEG",     EXT, EA_UINT8,  EA_MEM_RDWR,  7},
	{ 0x71,3,"OIM",     EXT, EA_UINT8,  EA_MEM_RDWR,  7},
	{ 0x72,3,"AIM",     EXT, EA_UINT8,  EA_MEM_RDWR,  7},
	{ 0x73,2,"COM",     EXT, EA_UINT8,  EA_MEM_RDWR,  7},
	{ 0x74,2,"LSR",     EXT, EA_UINT8,  EA_MEM_RDWR,  7},
	{ 0x75,3,"EIM",     EXT, EA_UINT8,  EA_MEM_RDWR,  7},
	{ 0x76,2,"ROR",     EXT, EA_UINT8,  EA_MEM_RDWR,  7},
	{ 0x77,2,"ASR",     EXT, EA_UINT8,  EA_MEM_RDWR,  7},
	{ 0x78,2,"ASL",     EXT, EA_UINT8,  EA_MEM_RDWR,  7},
	{ 0x79,2,"ROL",     EXT, EA_UINT8,  EA_MEM_RDWR,  7},
	{ 0x7A,2,"DEC",     EXT, EA_UINT8,  EA_MEM_RDWR,  7},
	{ 0x7B,3,"TIM",     EXT, EA_UINT8,  EA_MEM_RD,    7},
	{ 0x7C,2,"INC",     EXT, EA_UINT8,  EA_MEM_RDWR,  7},
	{ 0x7D,2,"TST",     EXT, EA_UINT8,  EA_MEM_RD,    7},
	{ 0x7E,2,"JMP",     EXT, EA_UINT8,  EA_ABS_PC,    4},
	{ 0x7F,2,"CLR",     EXT, EA_UINT8,  EA_MEM_WR,    7},

	{ 0x80,1,"SUBA",    IMM, EA_UINT8, EA_VALUE,     2},
	{ 0x81,1,"CMPA",    IMM, EA_UINT8, EA_VALUE,     2},
	{ 0x82,1,"SBCA",    IMM, EA_UINT8, EA_VALUE,     2},
	{ 0x83,2,"SUBD",    IMM, EA_UINT16,EA_VALUE,     4},
	{ 0x84,1,"ANDA",    IMM, EA_UINT8, EA_VALUE,     2},
	{ 0x85,1,"BITA",    IMM, EA_UINT8, EA_VALUE,     2},
	{ 0x86,1,"LDA",     IMM, EA_UINT8, EA_VALUE,     2},
	{ 0x88,1,"EORA",    IMM, EA_UINT8, EA_VALUE,     2},
	{ 0x89,1,"ADCA",    IMM, EA_UINT8, EA_VALUE,     2},
	{ 0x8A,1,"ORA",     IMM, EA_UINT8, EA_VALUE,     2},
	{ 0x8B,1,"ADDA",    IMM, EA_UINT8, EA_VALUE,     2},
	{ 0x8C,2,"CMPX",    IMM, EA_UINT16,EA_VALUE,     4},
	{ 0x8D,1,"BSR",     REL, EA_INT8,  EA_REL_PC,    7},
	{ 0x8E,2,"LDX",     IMM, EA_UINT16,EA_VALUE,     3},

	{ 0x90,1,"SUBA",    DIR, EA_UINT8, EA_ZPG_RD,    4},
	{ 0x91,1,"CMPA",    DIR, EA_UINT8, EA_ZPG_RD,    4},
	{ 0x92,1,"SBCA",    DIR, EA_UINT8, EA_ZPG_RD,    4},
	{ 0x93,1,"SUBD",    DIR, EA_UINT16,EA_ZPG_RD,    6},
	{ 0x94,1,"ANDA",    DIR, EA_UINT8, EA_ZPG_RD,    4},
	{ 0x95,1,"BITA",    DIR, EA_UINT8, EA_ZPG_RD,    4},
	{ 0x96,1,"LDA",     DIR, EA_UINT8, EA_ZPG_RD,    4},
	{ 0x97,1,"STA",     DIR, EA_UINT8, EA_ZPG_WR,    4},
	{ 0x98,1,"EORA",    DIR, EA_UINT8, EA_ZPG_RD,    4},
	{ 0x99,1,"ADCA",    DIR, EA_UINT8, EA_ZPG_RD,    4},
	{ 0x9A,1,"ORA",     DIR, EA_UINT8, EA_ZPG_RD,    4},
	{ 0x9B,1,"ADDA",    DIR, EA_UINT8, EA_ZPG_RD,    4},
	{ 0x9C,1,"CMPX",    DIR, EA_UINT16,EA_ZPG_RD,    6},
	{ 0x9D,1,"JSR",     DIR, EA_UINT8, EA_ABS_PC,    7},
	{ 0x9E,1,"LDX",     DIR, EA_UINT16,EA_ZPG_RD,    5},
	{ 0x9F,1,"STX",     DIR, EA_UINT16,EA_ZPG_WR,    5},

	{ 0xA0,1,"SUBA",    IND, EA_UINT8, EA_MEM_RD,    4},
	{ 0xA1,1,"CMPA",    IND, EA_UINT8, EA_MEM_RD,    4},
	{ 0xA2,1,"SBCA",    IND, EA_UINT8, EA_MEM_RD,    4},
	{ 0xA3,1,"SUBD",    IND, EA_UINT16,EA_MEM_RD,    6},
	{ 0xA4,1,"ANDA",    IND, EA_UINT8, EA_MEM_RD,    4},
	{ 0xA5,1,"BITA",    IND, EA_UINT8, EA_MEM_RD,    4},
	{ 0xA6,1,"LDA",     IND, EA_UINT8, EA_MEM_RD,    4},
	{ 0xA7,1,"STA",     IND, EA_UINT8, EA_MEM_WR,    4},
	{ 0xA8,1,"EORA",    IND, EA_UINT8, EA_MEM_RD,    4},
	{ 0xA9,1,"ADCA",    IND, EA_UINT8, EA_MEM_RD,    4},
	{ 0xAA,1,"ORA",     IND, EA_UINT8, EA_MEM_RD,    4},
	{ 0xAB,1,"ADDA",    IND, EA_UINT8, EA_MEM_RD,    4},
	{ 0xAC,1,"CMPX",    IND, EA_UINT16,EA_MEM_RD,    6},
	{ 0xAD,1,"JSR",     IND, EA_UINT8, EA_ABS_PC,    7},
	{ 0xAE,1,"LDX",     IND, EA_UINT16,EA_MEM_RD,    5},
	{ 0xAF,1,"STX",     IND, EA_UINT16,EA_MEM_WR,    5},

	{ 0xB0,2,"SUBA",    EXT, EA_UINT8, EA_MEM_RD,    5},
	{ 0xB1,2,"CMPA",    EXT, EA_UINT8, EA_MEM_RD,    5},
	{ 0xB2,2,"SBCA",    EXT, EA_UINT8, EA_MEM_RD,    5},
	{ 0xB3,2,"SUBD",    EXT, EA_UINT16,EA_MEM_RD,    7},
	{ 0xB4,2,"ANDA",    EXT, EA_UINT8, EA_MEM_RD,    5},
	{ 0xB5,2,"BITA",    EXT, EA_UINT8, EA_MEM_RD,    5},
	{ 0xB6,2,"LDA",     EXT, EA_UINT8, EA_MEM_RD,    5},
	{ 0xB7,2,"STA",     EXT, EA_UINT8, EA_MEM_WR,    5},
	{ 0xB8,2,"EORA",    EXT, EA_UINT8, EA_MEM_RD,    5},
	{ 0xB9,2,"ADCA",    EXT, EA_UINT8, EA_MEM_RD,    5},
	{ 0xBA,2,"ORA",     EXT, EA_UINT8, EA_MEM_RD,    5},
	{ 0xBB,2,"ADDA",    EXT, EA_UINT8, EA_MEM_RD,    5},
	{ 0xBC,2,"CMPX",    EXT, EA_UINT16,EA_MEM_RD,    7},
	{ 0xBD,2,"JSR",     EXT, EA_UINT8, EA_ABS_PC,    8},
	{ 0xBE,2,"LDX",     EXT, EA_UINT16,EA_MEM_RD,    6},
	{ 0xBF,2,"STX",     EXT, EA_UINT16,EA_MEM_WR,    6},

	{ 0xC0,1,"SUBB",    IMM, EA_UINT8, EA_VALUE,     2},
	{ 0xC1,1,"CMPB",    IMM, EA_UINT8, EA_VALUE,     2},
	{ 0xC2,1,"SBCB",    IMM, EA_UINT8, EA_VALUE,     2},
	{ 0xC3,2,"ADDD",    IMM, EA_UINT16,EA_VALUE,     4},
	{ 0xC4,1,"ANDB",    IMM, EA_UINT8, EA_VALUE,     2},
	{ 0xC5,1,"BITB",    IMM, EA_UINT8, EA_VALUE,     2},
	{ 0xC6,1,"LDB",     IMM, EA_UINT8, EA_VALUE,     2},
	{ 0xC8,1,"EORB",    IMM, EA_UINT8, EA_VALUE,     2},
	{ 0xC9,1,"ADCB",    IMM, EA_UINT8, EA_VALUE,     2},
	{ 0xCA,1,"ORB",     IMM, EA_UINT8, EA_VALUE,     2},
	{ 0xCB,1,"ADDB",    IMM, EA_UINT8, EA_VALUE,     2},
	{ 0xCC,2,"LDD",     IMM, EA_UINT16,EA_VALUE,     3},
	{ 0xCD,4,"LDQ",     IMM, EA_UINT32,EA_VALUE,     5},
	{ 0xCE,2,"LDU",     IMM, EA_UINT16,EA_VALUE,     3},

	{ 0xD0,1,"SUBB",    DIR, EA_UINT8, EA_ZPG_RD,    4},
	{ 0xD1,1,"CMPB",    DIR, EA_UINT8, EA_ZPG_RD,    4},
	{ 0xD2,1,"SBCB",    DIR, EA_UINT8, EA_ZPG_RD,    4},
	{ 0xD3,1,"ADDD",    DIR, EA_UINT8, EA_ZPG_RD,    6},
	{ 0xD4,1,"ANDB",    DIR, EA_UINT8, EA_ZPG_RD,    4},
	{ 0xD5,1,"BITB",    DIR, EA_UINT8, EA_ZPG_RD,    4},
	{ 0xD6,1,"LDB",     DIR, EA_UINT8, EA_ZPG_RD,    4},
	{ 0xD7,1,"STB",     DIR, EA_UINT8, EA_ZPG_WR,    4},
	{ 0xD8,1,"EORB",    DIR, EA_UINT8, EA_ZPG_RD,    4},
	{ 0xD9,1,"ADCB",    DIR, EA_UINT8, EA_ZPG_RD,    4},
	{ 0xDA,1,"ORB",     DIR, EA_UINT8, EA_ZPG_RD,    4},
	{ 0xDB,1,"ADDB",    DIR, EA_UINT8, EA_ZPG_RD,    4},
	{ 0xDC,1,"LDD",     DIR, EA_UINT16,EA_ZPG_RD,    5},
	{ 0xDD,1,"STD",     DIR, EA_UINT16,EA_ZPG_WR,    5},
	{ 0xDE,1,"LDU",     DIR, EA_UINT16,EA_ZPG_RD,    5},
	{ 0xDF,1,"STU",     DIR, EA_UINT16,EA_ZPG_WR,    5},

	{ 0xE0,1,"SUBB",    IND, EA_UINT8, EA_MEM_RD,    4},
	{ 0xE1,1,"CMPB",    IND, EA_UINT8, EA_MEM_RD,    4},
	{ 0xE2,1,"SBCB",    IND, EA_UINT8, EA_MEM_RD,    4},
	{ 0xE3,1,"ADDD",    IND, EA_UINT8, EA_MEM_RD,    6},
	{ 0xE4,1,"ANDB",    IND, EA_UINT8, EA_MEM_RD,    4},
	{ 0xE5,1,"BITB",    IND, EA_UINT8, EA_MEM_RD,    4},
	{ 0xE6,1,"LDB",     IND, EA_UINT8, EA_MEM_RD,    4},
	{ 0xE7,1,"STB",     IND, EA_UINT8, EA_MEM_WR,    4},
	{ 0xE8,1,"EORB",    IND, EA_UINT8, EA_MEM_RD,    4},
	{ 0xE9,1,"ADCB",    IND, EA_UINT8, EA_MEM_RD,    4},
	{ 0xEA,1,"ORB",     IND, EA_UINT8, EA_MEM_RD,    4},
	{ 0xEB,1,"ADDB",    IND, EA_UINT8, EA_MEM_RD,    4},
	{ 0xEC,1,"LDD",     IND, EA_UINT16,EA_MEM_RD,    5},
	{ 0xED,1,"STD",     IND, EA_UINT16,EA_MEM_WR,    5},
	{ 0xEE,1,"LDU",     IND, EA_UINT16,EA_MEM_RD,    5},
	{ 0xEF,1,"STU",     IND, EA_UINT16,EA_MEM_WR,    5},

	{ 0xF0,2,"SUBB",    EXT, EA_UINT8, EA_MEM_RD,    5},
	{ 0xF1,2,"CMPB",    EXT, EA_UINT8, EA_MEM_RD,    5},
	{ 0xF2,2,"SBCB",    EXT, EA_UINT8, EA_MEM_RD,    5},
	{ 0xF3,2,"ADDD",    EXT, EA_UINT8, EA_MEM_RD,    7},
	{ 0xF4,2,"ANDB",    EXT, EA_UINT8, EA_MEM_RD,    5},
	{ 0xF5,2,"BITB",    EXT, EA_UINT8, EA_MEM_RD,    5},
	{ 0xF6,2,"LDB",     EXT, EA_UINT8, EA_MEM_RD,    5},
	{ 0xF7,2,"STB",     EXT, EA_UINT8, EA_MEM_WR,    5},
	{ 0xF8,2,"EORB",    EXT, EA_UINT8, EA_MEM_RD,    5},
	{ 0xF9,2,"ADCB",    EXT, EA_UINT8, EA_MEM_RD,    5},
	{ 0xFA,2,"ORB",     EXT, EA_UINT8, EA_MEM_RD,    5},
	{ 0xFB,2,"ADDB",    EXT, EA_UINT8, EA_MEM_RD,    5},
	{ 0xFC,2,"LDD",     EXT, EA_UINT16,EA_MEM_RD,    6},
	{ 0xFD,2,"STD",     EXT, EA_UINT16,EA_MEM_WR,    6},
	{ 0xFE,2,"LDU",     EXT, EA_UINT16,EA_MEM_RD,    6},
	{ 0xFF,2,"STU",     EXT, EA_UINT16,EA_MEM_WR,    6},
};

/* page 2 ops 10xx*/
static opcodeinfo pg2opcodes[] =
{
	{ 0x21,3,"LBRN",    LREL,EA_INT16, EA_REL_PC,    5},
	{ 0x22,3,"LBHI",    LREL,EA_INT16, EA_REL_PC,    5},
	{ 0x23,3,"LBLS",    LREL,EA_INT16, EA_REL_PC,    5},
	{ 0x24,3,"LBCC",    LREL,EA_INT16, EA_REL_PC,    5},
	{ 0x25,3,"LBCS",    LREL,EA_INT16, EA_REL_PC,    5},
	{ 0x26,3,"LBNE",    LREL,EA_INT16, EA_REL_PC,    5},
	{ 0x27,3,"LBEQ",    LREL,EA_INT16, EA_REL_PC,    5},
	{ 0x28,3,"LBVC",    LREL,EA_INT16, EA_REL_PC,    5},
	{ 0x29,3,"LBVS",    LREL,EA_INT16, EA_REL_PC,    5},
	{ 0x2A,3,"LBPL",    LREL,EA_INT16, EA_REL_PC,    5},
	{ 0x2B,3,"LBMI",    LREL,EA_INT16, EA_REL_PC,    5},
	{ 0x2C,3,"LBGE",    LREL,EA_INT16, EA_REL_PC,    5},
	{ 0x2D,3,"LBLT",    LREL,EA_INT16, EA_REL_PC,    5},
	{ 0x2E,3,"LBGT",    LREL,EA_INT16, EA_REL_PC,    5},
	{ 0x2F,3,"LBLE",    LREL,EA_INT16, EA_REL_PC,    5},

	{ 0x30,2,"ADDR",     IMM,        0,        0,    4},
	{ 0x31,2,"ADCR",     IMM,        0,        0,    4},
	{ 0x32,2,"SUBR",     IMM,        0,        0,    4},
	{ 0x33,2,"SBCR",     IMM,        0,        0,    4},
	{ 0x34,2,"ANDR",     IMM,        0,        0,    4},
	{ 0x35,2,"ORR",      IMM,        0,        0,    4},
	{ 0x36,2,"EORR",     IMM,        0,        0,    4},
	{ 0x37,2,"CMPR",     IMM,        0,        0,    4},

	{ 0x38,0,"PSHSW",    INH,        0,        0,    6},
	{ 0x39,0,"PULSW",    INH,        0,        0,    6},
	{ 0x3A,0,"PSHUW",    INH,        0,        0,    6},
	{ 0x3B,0,"PULSW",    INH,        0,        0,    6},

	{ 0x3F,2,"SWI2",     INH,        0,        0,   20},

	{ 0x40,0,"NEGD",     INH,        0,        0,    3},
	{ 0x43,0,"COMD",     INH,        0,        0,    3},
	{ 0x44,0,"LSRD",     INH,        0,        0,    3},
	{ 0x46,0,"RORD",     INH,        0,        0,    3},
	{ 0x47,0,"ASRD",     INH,        0,        0,    3},
	{ 0x48,0,"ASLD",     INH,        0,        0,    3},
	{ 0x49,0,"ROLD",     INH,        0,        0,    3},

	{ 0x4A,0,"DECD",     INH,        0,        0,    3},
	{ 0x4C,0,"INCD",     INH,        0,        0,    3},
	{ 0x4D,0,"TSTD",     INH,        0,        0,    3},
	{ 0x4f,0,"CLRD",     INH,        0,        0,    3},

	{ 0x53,0,"COMW",     INH,        0,        0,    3},
	{ 0x54,0,"LSRW",     INH,        0,        0,    3},
	{ 0x56,0,"RORW",     INH,        0,        0,    3},
	{ 0x59,0,"ROLW",     INH,        0,        0,    3},
	{ 0x5A,0,"DECW",     INH,        0,        0,    3},
	{ 0x5C,0,"INCW",     INH,        0,        0,    3},
	{ 0x5D,0,"TSTW",     INH,        0,        0,    3},
	{ 0x5F,0,"CLRW",     INH,        0,        0,    3},
	{ 0x80,3,"SUBW",     IMM, EA_UINT16, EA_VALUE,    5},
	{ 0x81,3,"CMPW",     IMM, EA_UINT16, EA_VALUE,    5},
	{ 0x82,3,"SBCD",     IMM, EA_UINT16, EA_VALUE,    5},

	{ 0x83,3,"CMPD",     IMM, EA_UINT16, EA_VALUE,    5},

	{ 0x84,3,"ANDD",     IMM, EA_UINT16, EA_VALUE,    5},
	{ 0x85,3,"BITD",     IMM, EA_UINT16, EA_VALUE,    5},
	{ 0x86,3,"LDW",      IMM, EA_UINT16, EA_VALUE,    5},
	{ 0x88,3,"EORD",     IMM, EA_UINT16, EA_VALUE,    5},
	{ 0x89,3,"ADCD",     IMM, EA_UINT16, EA_VALUE,    5},
	{ 0x8A,3,"ORD",      IMM, EA_UINT16, EA_VALUE,    5},
	{ 0x8B,3,"ADDW",     IMM, EA_UINT16, EA_VALUE,    5},

	{ 0x8C,3,"CMPY",     IMM, EA_UINT16, EA_VALUE,    5},
	{ 0x8E,3,"LDY",      IMM, EA_UINT16, EA_VALUE,    4},

	{ 0x90,2,"SUBW",     DIR, EA_UINT16, EA_ZPG_RD,   7},
	{ 0x91,2,"CMPW",     DIR, EA_UINT16, EA_ZPG_RD,   7},
	{ 0x92,2,"SBCD",     DIR, EA_UINT16, EA_ZPG_RD,   7},

	{ 0x93,2,"CMPD",     DIR, EA_UINT16, EA_ZPG_RD,   7},

	{ 0x94,2,"ANDD",     DIR, EA_UINT16, EA_ZPG_RD,   7},
	{ 0x95,2,"BITD",     DIR, EA_UINT16, EA_ZPG_RD,   7},
	{ 0x96,2,"LDW",      DIR, EA_UINT16, EA_ZPG_RD,   6},
	{ 0x97,2,"STW",      DIR, EA_UINT16, EA_ZPG_RD,   6},
	{ 0x98,2,"EORD",     DIR, EA_UINT16, EA_ZPG_RD,   7},
	{ 0x99,2,"ADCD",     DIR, EA_UINT16, EA_ZPG_RD,   7},
	{ 0x9A,2,"ORD",      DIR, EA_UINT16, EA_ZPG_RD,   7},
	{ 0x9B,2,"ADDW",     DIR, EA_UINT16, EA_ZPG_RD,   7},


	{ 0x9C,2,"CMPY",     DIR, EA_UINT16, EA_ZPG_RD,   7},
	{ 0x9E,2,"LDY",      DIR, EA_UINT16, EA_ZPG_RD,   6},
	{ 0x9F,2,"STY",      DIR, EA_UINT16, EA_ZPG_RD,   6},

	{ 0xA0,2,"SUBW",     IND, EA_UINT16, EA_MEM_RD,   7},
	{ 0xA1,2,"CMPW",     IND, EA_UINT16, EA_MEM_RD,   7},
	{ 0xA2,2,"SBCD",     IND, EA_UINT16, EA_MEM_RD,   7},

	{ 0xA3,2,"CMPD",     IND, EA_UINT16, EA_MEM_RD,   7},

	{ 0xA4,2,"ANDD",     IND, EA_UINT16, EA_MEM_RD,   7},
	{ 0xA5,2,"BITD",     IND, EA_UINT16, EA_MEM_RD,   7},

	{ 0xA6,2,"LDW",      IND, EA_UINT16, EA_MEM_RD,   6},
	{ 0xA7,2,"STW",      IND, EA_UINT16, EA_MEM_RD,   6},
	{ 0xA8,2,"EORD",     IND, EA_UINT16, EA_MEM_RD,   7},
	{ 0xA9,2,"ADCD",     IND, EA_UINT16, EA_MEM_RD,   7},
	{ 0xAA,2,"ORD",      IND, EA_UINT16, EA_MEM_RD,   7},
	{ 0xAB,2,"ADDW",     IND, EA_UINT16, EA_MEM_RD,   7},

	{ 0xAC,2,"CMPY",     IND, EA_UINT16, EA_MEM_RD,   7},
	{ 0xAE,2,"LDY",      IND, EA_UINT16, EA_MEM_RD,   6},
	{ 0xAF,2,"STY",      IND, EA_UINT16, EA_MEM_RD,   6},

	{ 0xB0,3,"SUBW",     EXT, EA_UINT16, EA_MEM_RD,   8},
	{ 0xB1,3,"CMPW",     EXT, EA_UINT16, EA_MEM_RD,   8},
	{ 0xB2,3,"SBCD",     EXT, EA_UINT16, EA_MEM_RD,   8},

	{ 0xB3,3,"CMPD",     EXT, EA_UINT16, EA_MEM_RD,   8},

	{ 0xB4,3,"ANDD",     EXT, EA_UINT16, EA_MEM_RD,   8},
	{ 0xB5,3,"BITD",     EXT, EA_UINT16, EA_MEM_RD,   8},
	{ 0xB6,3,"LDW",      EXT, EA_UINT16, EA_MEM_RD,   7},
	{ 0xB7,3,"STW",      EXT, EA_UINT16, EA_MEM_RD,   7},
	{ 0xB8,3,"EORD",     EXT, EA_UINT16, EA_MEM_RD,   8},
	{ 0xB9,3,"ADCD",     EXT, EA_UINT16, EA_MEM_RD,   8},
	{ 0xBA,3,"ORD",      EXT, EA_UINT16, EA_MEM_RD,   8},
	{ 0xBB,3,"ADDW",     EXT, EA_UINT16, EA_MEM_RD,   8},

	{ 0xBC,3,"CMPY",     EXT, EA_UINT16, EA_MEM_RD,   8},
	{ 0xBE,3,"LDY",      EXT, EA_UINT16, EA_MEM_RD,   7},
	{ 0xBF,3,"STY",      EXT, EA_UINT16, EA_MEM_RD,   7},
	{ 0xCE,3,"LDS",      IMM, EA_UINT16, EA_VALUE,    4},

	{ 0xDC,2,"LDQ",      DIR, EA_UINT16, EA_ZPG_RD,   8},
	{ 0xDD,2,"STQ",      DIR, EA_UINT16, EA_ZPG_RD,   8},

	{ 0xDE,2,"LDS",      DIR, EA_UINT16, EA_ZPG_RD,   6},
	{ 0xDF,2,"STS",      DIR, EA_UINT16, EA_ZPG_WR,   6},

	{ 0xEC,2,"LDQ",      IND, EA_UINT16, EA_MEM_RD,   8},
	{ 0xED,2,"STQ",      IND, EA_UINT16, EA_MEM_WR,   8},
	{ 0xEE,2,"LDS",      IND, EA_UINT16, EA_MEM_RD,   8},

	{ 0xEE,2,"LDS",      IND, EA_UINT16, EA_MEM_RD,   6},
	{ 0xEF,2,"STS",      IND, EA_UINT16, EA_MEM_WR,   6},

	{ 0xFC,3,"LDQ",      EXT, EA_UINT16, EA_MEM_RD,   9},
	{ 0xFD,3,"STQ",      EXT, EA_UINT16, EA_MEM_WR,   9},

	{ 0xFE,3,"LDS",      EXT, EA_UINT16, EA_MEM_RD,   7},
	{ 0xFF,3,"STS",      EXT, EA_UINT16, EA_MEM_WR,   7},
};

/* page 3 ops 11xx */
static opcodeinfo pg3opcodes[] =
{
	{ 0x30,3,"BAND",    DIR, EA_UINT8, EA_ZPG_RDWR,  7},
	{ 0x31,3,"BIAND",   DIR, EA_UINT8, EA_ZPG_RDWR,  7},
	{ 0x32,3,"BOR",     DIR, EA_UINT8, EA_ZPG_RDWR,  7},
	{ 0x33,3,"BIOR",    DIR, EA_UINT8, EA_ZPG_RDWR,  7},
	{ 0x34,3,"BEOR",    DIR, EA_UINT8, EA_ZPG_RDWR,  7},
	{ 0x35,3,"BIEOR",   DIR, EA_UINT8, EA_ZPG_RDWR,  7},

	{ 0x36,3,"LDBT",    DIR, EA_UINT8, EA_ZPG_RD,    7},
	{ 0x37,3,"STBT",    DIR, EA_UINT8, EA_ZPG_RDWR,  7},

	{ 0x38,2,"TFM",     IMM, 0,        0,            6},
	{ 0x39,2,"TFM",     IMM, 0,        0,            6},
	{ 0x3A,2,"TFM",     IMM, 0,        0,            6},
	{ 0x3B,2,"TFM",     IMM, 0,        0,            6},

	{ 0x3C,2,"BITMD",   IMM, EA_UINT8, EA_VALUE,     4},
	{ 0x3D,2,"LDMD",    IMM, EA_UINT8, EA_VALUE,     5},

	{ 0x3F,1,"SWI3",    INH, 0,        0,           20},

	{ 0x43,1,"COME",    INH, 0,        0,            3},
	{ 0x4A,1,"DECE",    INH, 0,        0,            3},
	{ 0x4C,1,"INCE",    INH, 0,        0,            3},
	{ 0x4D,1,"TSTE",    INH, 0,        0,            3},
	{ 0x4F,1,"CLRE",    INH, 0,        0,            3},
	{ 0x53,1,"COMF",    INH, 0,        0,            3},
	{ 0x5A,1,"DECF",    INH, 0,        0,            3},
	{ 0x5C,1,"INCF",    INH, 0,        0,            3},
	{ 0x5D,1,"TSTF",    INH, 0,        0,            3},
	{ 0x5F,1,"CLRF",    INH, 0,        0,            3},

	{ 0x80,2,"SUBE",    IMM, EA_UINT8, EA_VALUE,     3},
	{ 0x81,2,"CMPE",    IMM, EA_UINT8, EA_VALUE,     3},

	{ 0x83,3,"CMPU",    IMM, EA_UINT16,EA_VALUE,     5},

	{ 0x86,2,"LDE",     IMM, EA_UINT8, EA_VALUE,     3},
	{ 0x8b,2,"ADDE",    IMM, EA_UINT8, EA_VALUE,     3},

	{ 0x8C,3,"CMPS",    IMM, EA_UINT16,EA_VALUE,     5},

	{ 0x8D,2,"DIVD",    IMM, EA_UINT8, EA_VALUE,    25},
	{ 0x8E,3,"DIVQ",    IMM, EA_UINT16,EA_VALUE,    36},
	{ 0x8F,3,"MULD",    IMM, EA_UINT16,EA_VALUE,    28},
	{ 0x90,2,"SUBE",    DIR, EA_UINT8, EA_ZPG_RD,    5},
	{ 0x91,2,"CMPE",    DIR, EA_UINT8, EA_ZPG_RD,    5},

	{ 0x93,2,"CMPU",    DIR, EA_UINT16,EA_ZPG_RD,    7},

	{ 0x96,2,"LDE",     DIR, EA_UINT8, EA_ZPG_RD,    5},
	{ 0x97,2,"STE",     DIR, EA_UINT8, EA_ZPG_RD,    5},
	{ 0x9B,2,"ADDE",    DIR, EA_UINT8, EA_ZPG_RD,    5},

	{ 0x9C,2,"CMPS",    DIR, EA_UINT16,EA_ZPG_RD,    7},

	{ 0x9D,2,"DIVD",    DIR, EA_UINT8 ,EA_ZPG_RD,   27},
	{ 0x9E,2,"DIVQ",    DIR, EA_UINT16,EA_ZPG_RD,   36},
	{ 0x9F,2,"MULD",    DIR, EA_UINT16,EA_ZPG_RD,   30},

	{ 0xA0,2,"SUBE",    IND, EA_UINT8, EA_MEM_RD,    5},
	{ 0xA1,2,"CMPE",    IND, EA_UINT8, EA_MEM_RD,    5},

	{ 0xA3,2,"CMPU",    IND, EA_UINT16,EA_MEM_RD,    7},

	{ 0xA6,2,"LDE",     IND, EA_UINT8, EA_MEM_RD,    5},
	{ 0xA7,2,"STE",     IND, EA_UINT8, EA_MEM_WR,    5},
	{ 0xAB,2,"ADDE",    IND, EA_UINT8, EA_MEM_WR,    5},

	{ 0xAC,2,"CMPS",    IND, EA_UINT16,EA_MEM_RD,    7},

	{ 0xAD,2,"DIVD",    IND, EA_UINT8 ,EA_MEM_RD,   27},
	{ 0xAE,2,"DIVQ",    IND, EA_UINT16,EA_MEM_RD,   36},
	{ 0xAF,2,"MULD",    IND, EA_UINT16,EA_MEM_RD,   30},
	{ 0xB0,3,"SUBE",    EXT, EA_UINT8, EA_MEM_RD,    6},
	{ 0xB1,3,"CMPE",    EXT, EA_UINT8, EA_MEM_RD,    6},

	{ 0xB3,3,"CMPU",    EXT, EA_UINT16,EA_MEM_RD,    8},

	{ 0xB6,3,"LDE",     EXT, EA_UINT8, EA_MEM_RD,    6},
	{ 0xB7,3,"STE",     EXT, EA_UINT8, EA_MEM_WR,    6},

	{ 0xBB,3,"ADDE",    EXT, EA_UINT8, EA_MEM_RD,    6},
	{ 0xBC,3,"CMPS",    EXT, EA_UINT16,EA_MEM_RD,    8},

	{ 0xBD,3,"DIVD",    EXT, EA_UINT8 ,EA_MEM_RD,   28},
	{ 0xBE,3,"DIVQ",    EXT, EA_UINT16,EA_MEM_RD,   37},
	{ 0xBF,3,"MULD",    EXT, EA_UINT16,EA_MEM_RD,   31},

	{ 0xC0,2,"SUBF",    IMM, EA_UINT8, EA_VALUE,     3},
	{ 0xC1,2,"CMPF",    IMM, EA_UINT8, EA_VALUE,     3},
	{ 0xC6,2,"LDF",     IMM, EA_UINT8, EA_VALUE,     3},
	{ 0xCB,2,"ADDF",    IMM, EA_UINT8, EA_VALUE,     3},

	{ 0xD0,2,"SUBF",    DIR, EA_UINT8, EA_ZPG_RD,    5},
	{ 0xD1,2,"CMPF",    DIR, EA_UINT8, EA_ZPG_RD,    5},
	{ 0xD6,2,"LDF",     DIR, EA_UINT8, EA_ZPG_RD,    5},
	{ 0xD7,2,"STF",     DIR, EA_UINT8, EA_ZPG_WR,    5},
	{ 0xDB,2,"ADDF",    DIR, EA_UINT8, EA_ZPG_RD,    5},

	{ 0xE0,2,"SUBF",    IND, EA_UINT8, EA_MEM_RD,    5},
	{ 0xE1,2,"CMPF",    IND, EA_UINT8, EA_MEM_RD,    5},
	{ 0xE6,2,"LDF",     IND, EA_UINT8, EA_MEM_RD,    5},
	{ 0xE7,2,"STF",     IND, EA_UINT8, EA_MEM_WR,    5},
	{ 0xEB,2,"ADDF",    IND, EA_UINT8, EA_MEM_RD,    5},

	{ 0xF0,3,"SUBF",    EXT, EA_UINT8, EA_MEM_RD,    6},
	{ 0xF1,3,"CMPF",    EXT, EA_UINT8, EA_MEM_RD,    6},
	{ 0xF6,3,"LDF",     EXT, EA_UINT8, EA_MEM_RD,    6},
	{ 0xF7,3,"STF",     EXT, EA_UINT8, EA_MEM_WR,    6},
	{ 0xFB,3,"ADDF",    EXT, EA_UINT8, EA_MEM_RD,    6},

};

int numops6309[3] =
{
	sizeof(pg1opcodes)/sizeof(opcodeinfo),
	sizeof(pg2opcodes)/sizeof(opcodeinfo),
	sizeof(pg3opcodes)/sizeof(opcodeinfo)
};

static opcodeinfo *pgpointers[3] =
{
   pg1opcodes,pg2opcodes,pg3opcodes,
};

static const UINT8 regid_6309[5] = {
	HD6309_X, HD6309_Y, HD6309_U, HD6309_S, HD6309_PC
};

static const char *regs_6309[5] = { "X","Y","U","S","PC" };

static const UINT8 btmRegs_id[] = { HD6309_CC, HD6309_A, HD6309_B, 0 };

static const char *btwRegs[5] = { "CC", "A", "B", "inv" };

static const char *teregs[16] = {
	"D","X","Y","U","S","PC","W","V",
	"A","B","CC","DP","0","0","E","F"
};

static const char *tfmregs[16] = {
	"D","X","Y","U","S","inv","inv","inv",
	"inv","inv","inv","inv","inv","inv","inv","inv"
};

static const char *tfm_s[] = { "%s+,%s+", "%s-,%s-", "%s+,%s", "%s,%s+" };

static char *hexstring (int address)
{
	static char labtemp[10];
	sprintf (labtemp, "$%04hX", address);
	return labtemp;
}

unsigned Dasm6309 (char *buffer, unsigned pc)
{
	int i, j, k, page=0, opcode, numoperands, mode, size, access;
	UINT8 operandarray[4];
	const char *sym1, *sym2;
	int rel, pb, offset, reg, pb2;
	unsigned ea = 0;
	int p = 0;

	*buffer = '\0';

	opcode = cpu_readop(pc+(p++));
	for( i = 0; i < numops6309[0]; i++ )
		if (pg1opcodes[i].opcode == opcode)
			break;

	if( i < numops6309[0] )
	{
		if( pg1opcodes[i].mode >= PG2 )
		{
			opcode = cpu_readop(pc+(p++));
			page = pg1opcodes[i].mode - PG2 + 1;		  /* get page # */
			for( k = 0; k < numops6309[page]; k++ )
				if (opcode == pgpointers[page][k].opcode)
					break;

			if( k != numops6309[page] )
			{	/* opcode found */
				numoperands = pgpointers[page][k].numoperands - 1;
				for (j = 0; j < numoperands; j++)
					operandarray[j] = cpu_readop_arg(pc+(p++));
				mode = pgpointers[page][k].mode;
				size = pgpointers[page][k].size;
				access = pgpointers[page][k].access;
				buffer += sprintf (buffer, "%-6s", pgpointers[page][k].name);
			 }
			 else
			 {	/* not found in alternate page */
				strcpy (buffer, "Illegal Opcode (JMP [$FFF0])");
				return 2;
			 }
		}
		else
		{	/* page 1 opcode */
			numoperands = pg1opcodes[i].numoperands;
			for( j = 0; j < numoperands; j++ )
				operandarray[j] = cpu_readop_arg(pc+(p++));
			mode = pg1opcodes[i].mode;
			size = pg1opcodes[i].size;
			access = pg1opcodes[i].access;
			buffer += sprintf (buffer, "%-6s", pg1opcodes[i].name);
		}
	}
	else
	{
		strcpy (buffer, "Illegal Opcode (JMP [$FFF0])");
		return 1;
	}

	pc += p;

	if( opcode != 0x1f &&	/* reg <-> reg instructions*/
		opcode != 0x1e &&
		opcode != 0x31 &&
		opcode != 0x30 &&
		opcode != 0x34 &&
		opcode != 0x37 &&
		opcode != 0x36 &&
		opcode != 0x35 &&
		opcode != 0x33 &&
		opcode != 0x32 &&
		opcode != 0x38 &&
		opcode != 0x39 &&
		opcode != 0x3a &&
		opcode != 0x3b )
	{
		if( mode == IMM )
			buffer += sprintf (buffer, "#");
	}

	switch (mode)
	{
	case REL:	  /* 8-bit relative */
		rel = operandarray[0];
		sym1 = set_ea_info(0, pc, (INT8)rel, access);
		buffer += sprintf (buffer, "%s", sym1);
		break;

	case LREL:	  /* 16-bit long relative */
		rel = (operandarray[0] << 8) + operandarray[1];
		sym1 = set_ea_info(0, pc, (INT16)rel, access);
		buffer += sprintf (buffer, "%s", sym1);
		break;

	case IND:	  /* indirect- many flavors */

		if (numoperands == 2 )
		{
			buffer += sprintf (buffer, "#");
			ea = operandarray[0];
			sym1 = set_ea_info(0, ea, EA_UINT8, EA_VALUE );
			buffer += sprintf (buffer, "%s", sym1 );
			buffer += sprintf (buffer, ",");
			pb = operandarray[1];
		}
		else
		{
			pb = operandarray[0];
		}
		reg = (pb >> 5) & 3;
		pb2 = pb & 0x8f;
		if( pb2 == 0x88 || pb2 == 0x8c )
		{	/* 8-bit offset */

			/* KW 11/05/98 Fix of indirect opcodes		*/
			offset = (INT8)cpu_readop_arg(pc);
			p++;
			if( pb == 0x8c ) reg = 4;
			if( (pb & 0x90) == 0x90 ) buffer += sprintf (buffer, "[");
			if( pb == 0x8c )
			{
				sym1 = set_ea_info(1, pc, (INT8)offset, EA_REL_PC);
				ea = (pc + (INT8)offset + activecpu_get_reg(regid_6309[reg])) & 0xffff;
				buffer += sprintf (buffer, "%s,%s", sym1, regs_6309[reg]);
			}
			else
			{
				sym1 = set_ea_info(1, offset, EA_INT8, EA_VALUE);
				ea = (activecpu_get_reg(regid_6309[reg]) + offset) & 0xffff;
				buffer += sprintf (buffer, "%s,%s", sym1, regs_6309[reg]);
			}
		}
		else
		if( pb2 == 0x89 || pb2 == 0x8d || pb2 == 0x8f )
		{	/* 16-bit */

			/* KW 11/05/98 Fix of indirect opcodes		*/

			offset = (INT16)( (cpu_readop_arg(pc) << 8) + cpu_readop_arg(pc+1) );
			p += 2;

			if( pb == 0x8d )
				reg = 4;
			if( (pb&0x90) == 0x90 )
				buffer += sprintf(buffer, "[");
			if( pb == 0x8d )
			{
				sym1 = set_ea_info(1, pc, (INT16)offset, EA_REL_PC);
				buffer += sprintf (buffer, "%s,%s", sym1, regs_6309[reg]);
			}
			else if ( pb == 0x9f || pb == 0xbf || pb == 0xdf || pb == 0xff )
			{
				sym1 = set_ea_info(1, offset, EA_UINT16, EA_VALUE);
                buffer += sprintf (buffer, "%s", sym1);
			}
			else
			{
				sym1 = set_ea_info(1, offset, EA_INT16, EA_VALUE);
				ea = (activecpu_get_reg(regid_6309[reg]) + offset) & 0xffff;
				buffer += sprintf (buffer, "%s,%s", sym1, regs_6309[reg]);
			}
		}
		else
		if ( pb == 0x8f || pb == 0x90 )
		{
			if( (pb & 0x90) == 0x90 ) buffer += sprintf (buffer, "[");
			ea = (activecpu_get_reg(HD6309_E) << 8) + activecpu_get_reg(HD6309_F);
			buffer += sprintf (buffer, ",W");
		}
		else
		if ( pb == 0xaf || pb == 0xb0 )
		{
			if( (pb & 0x90) == 0x90 ) buffer += sprintf (buffer, "[");

			offset = (INT16)( (cpu_readop_arg(pc) << 8) + cpu_readop_arg(pc+1) );
			p += 2;

			sym1 = set_ea_info(1, offset, EA_INT16, EA_MEM_WR);
			ea = ((activecpu_get_reg(HD6309_E) << 8) + (activecpu_get_reg(HD6309_F)) + offset) & 0xffff;
			buffer += sprintf (buffer, "%s,W", sym1);
		}
		else
		if ( pb == 0xcf || pb == 0xd0 )
		{
			if( (pb & 0x90) == 0x90 ) buffer += sprintf (buffer, "[");
			ea = (activecpu_get_reg(HD6309_E) << 8) + activecpu_get_reg(HD6309_F);
			buffer += sprintf (buffer, ",W++");
		}
		else
		if ( pb == 0xef || pb == 0xf0 )
		{
			if( (pb & 0x90) == 0x90 ) buffer += sprintf (buffer, "[");
			ea = (activecpu_get_reg(HD6309_E) << 8) + activecpu_get_reg(HD6309_F);
			buffer += sprintf (buffer, ",--W");
		}
		else
		if( (pb & 0x80) )
		{
			if( (pb & 0x90) == 0x90 )
				buffer += sprintf (buffer, "[");
			switch( pb & 0x8f )
			{
			case 0x80:
				ea = activecpu_get_reg(regid_6309[reg]);
				buffer += sprintf (buffer, ",%s+", regs_6309[reg]);
				break;
			case 0x81:
				ea = activecpu_get_reg(regid_6309[reg]);
				buffer += sprintf (buffer, ",%s++", regs_6309[reg]);
				break;
			case 0x82:
				ea = activecpu_get_reg(regid_6309[reg]);
				buffer += sprintf (buffer, ",-%s", regs_6309[reg]);
				break;
			case 0x83:
				ea = activecpu_get_reg(regid_6309[reg]);
				buffer += sprintf (buffer, ",--%s", regs_6309[reg]);
				break;
			case 0x84:
				ea = activecpu_get_reg(regid_6309[reg]);
				buffer += sprintf (buffer, ",%s", regs_6309[reg]);
				break;
			case 0x85:
				ea = (activecpu_get_reg(regid_6309[reg]) + (INT8) activecpu_get_reg(HD6309_B)) & 0xffff;
				buffer += sprintf (buffer, "B,%s", regs_6309[reg]);
				break;
			case 0x86:
				ea = (activecpu_get_reg(regid_6309[reg]) + (INT8) activecpu_get_reg(HD6309_A)) & 0xffff;
				buffer += sprintf (buffer, "A,%s", regs_6309[reg]);
				break;
			case 0x87:
				ea = (activecpu_get_reg(regid_6309[reg]) + (INT8) activecpu_get_reg(HD6309_E)) & 0xffff;
				buffer += sprintf (buffer, "E,%s", regs_6309[reg]);
				break;
			case 0x8a:
				ea = (activecpu_get_reg(regid_6309[reg]) + (INT8) activecpu_get_reg(HD6309_F)) & 0xffff;
				buffer += sprintf (buffer, "F,%s", regs_6309[reg]);
				break;
			case 0x8b:
				ea = (activecpu_get_reg(regid_6309[reg]) + (activecpu_get_reg(HD6309_A) << 8) + activecpu_get_reg(HD6309_B)) & 0xffff;
				buffer += sprintf (buffer, "D,%s", regs_6309[reg]);
				break;
			case 0x8e:
				ea = (activecpu_get_reg(regid_6309[reg]) + (activecpu_get_reg(HD6309_E) << 8) + activecpu_get_reg(HD6309_F)) & 0xffff;
				buffer += sprintf (buffer, "W,%s", regs_6309[reg]);
				break;
			}
		}
		else
		{										   /* 5-bit offset */
			offset = pb & 0x1f;
			if (offset > 15)
				offset = offset - 32;
			buffer += sprintf (buffer, "%s,%s", hexstring (offset), regs_6309[reg]);
		}
		/* indirect */
		if( (pb & 0x90) == 0x90 )
		{
			ea = ( cpu_readmem16( ea ) << 8 ) + cpu_readmem16( (ea+1) & 0xffff );
			buffer += sprintf (buffer, "]");
		}
		sym2 = set_ea_info(0, ea, size, access);
		break;
	default:
		if( page == 2 && ( opcode == 0x30 || opcode == 0x31 || opcode == 0x32 || opcode == 0x33 || opcode == 0x34 || opcode == 0x35 || opcode == 0x36 || opcode == 0x37 ) )
		{
			/* BAND, BIAND, BOR, BIOR, BEOR, BIEOR, LDBT, STBT */

			/* Decode register */

			pb = operandarray[0];

			buffer += sprintf (buffer, "%s", btwRegs[ ((pb & 0xc0) >> 6) ]);
			buffer += sprintf (buffer, ",");
			buffer += sprintf (buffer, "%d", ((pb & 0x38) >> 3) );
			buffer += sprintf (buffer, ",");
			buffer += sprintf (buffer, "%d", (pb & 0x07) );
			buffer += sprintf (buffer, ",");

			/* print zero page access */

			ea = operandarray[1];
			sym1 = set_ea_info(0, ea, size, access );
			buffer += sprintf (buffer, "%s", sym1 );
		}
		else
		if( page == 2 && ( opcode == 0x38 || opcode == 0x39 || opcode == 0x3a || opcode == 0x3b ) )
		{
			/* TFM instructions */
			buffer += sprintf (buffer, tfm_s[opcode & 0x07], tfmregs[ (operandarray[0] >> 4) & 0xf], tfmregs[operandarray[0] & 0xf]);
		}
		else
		if( opcode == 0x1f || opcode == 0x1e || ( page == 1 && (opcode == 0x31 || opcode == 0x30 || opcode == 0x34 || opcode == 0x37 || opcode == 0x35 || opcode == 0x33 || opcode == 0x1e || opcode == 0x32 ) ) )
		{	/* TFR/EXG + new 2nd page reg<->reg instructions*/
			buffer += sprintf (buffer, "%s,%s", teregs[ (operandarray[0] >> 4) & 0xf], teregs[operandarray[0] & 0xf]);
		}
		else
		if( opcode == 0x34 || opcode == 0x36 )
		{	/* PUSH */
			pb2 = operandarray[0];
			if( pb2 & 0x80 )
			{
				buffer += sprintf (buffer, "PC");
			}
			if( pb2 & 0x40 )
			{
				if( pb2 & 0x80 ) buffer += sprintf (buffer, ",");
				if( opcode == 0x34 || opcode == 0x35 )
				   buffer += sprintf (buffer, "U");
				else
				   buffer += sprintf (buffer, "S");
			}
			if( pb2 & 0x20 )
			{
				if( pb2 & 0xc0 ) buffer += sprintf (buffer, ",");
				buffer += sprintf (buffer, "Y");
			}
			if( pb2 & 0x10 )
			{
				if( pb2 & 0xe0 ) buffer += sprintf (buffer, ",");
				buffer += sprintf (buffer, "X");
			}
			if( pb2 & 0x08 )
			{
				if( pb2 & 0xf0 ) buffer += sprintf (buffer, ",");
				buffer += sprintf (buffer, "DP");
			}
			if( pb2 & 0x04 )
			{
				if( pb2 & 0xf8 ) buffer += sprintf (buffer, ",");
				buffer += sprintf (buffer, "B");
			}
			if( pb2 & 0x02 )
			{
				if( pb2 & 0xfc ) buffer += sprintf (buffer, ",");
				buffer += sprintf (buffer, "A");
			}
			if( pb2 & 0x01 )
			{
				if( pb2 & 0xfe ) buffer += sprintf (buffer, ",");
				strcat (buffer, "CC");
			}
		}
		else
		if( opcode == 0x35 || opcode == 0x37 )
		{	/* PULL */
			pb2 = operandarray[0];
			if( pb2 & 0x01 )
			{
				buffer += sprintf (buffer, "CC");
			}
			if( pb2 & 0x02 )
			{
				if( pb2 & 0x01 ) buffer += sprintf (buffer, ",");
				buffer += sprintf (buffer, "A");
			}
			if( pb2 & 0x04 )
			{
				if( pb2 & 0x03 ) buffer += sprintf (buffer, ",");
				buffer += sprintf (buffer, "B");
			}
			if( pb2 & 0x08 )
			{
				if( pb2 & 0x07 ) buffer += sprintf (buffer, ",");
				buffer += sprintf (buffer, "DP");
			}
			if( pb2 & 0x10 )
			{
				if( pb2 & 0x0f ) buffer += sprintf (buffer, ",");
				buffer += sprintf (buffer, "X");
			}
			if( pb2 & 0x20 )
			{
				if( pb2 & 0x1f ) buffer += sprintf (buffer, ",");
				buffer += sprintf (buffer, "Y");
			}
			if( pb2 & 0x40 )
			{
				if( pb2 & 0x3f ) buffer += sprintf (buffer, ",");
				if( opcode == 0x34 || opcode == 0x35 )
					buffer += sprintf (buffer, "U");
				else
					buffer += sprintf (buffer, "S");
			}
			if( pb2 & 0x80 )
			{
				if( pb2 & 0x7f ) buffer += sprintf (buffer, ",");
				buffer += sprintf (buffer, "PC");
				buffer += sprintf (buffer, " ; (PUL? PC=RTS)");
			}
		}
		else
		{
			if ( numoperands == 4)
			{
				ea = (operandarray[0] << 24) + (operandarray[1] << 16) + (operandarray[2] << 8) + operandarray[3];
				sym1 = set_ea_info(0, ea, size, access );
				buffer += sprintf (buffer, "%s", sym1 );
			}
			else
			if ( numoperands == 3)
			{
				buffer += sprintf (buffer, "#");
				ea = operandarray[0];
				sym1 = set_ea_info(0, ea, EA_INT8, EA_VALUE );
				buffer += sprintf (buffer, "%s", sym1 );

				buffer += sprintf (buffer, ",");

				ea = (operandarray[1] << 8) + operandarray[2];
				sym1 = set_ea_info(0, ea, size, access );
				buffer += sprintf (buffer, "%s", sym1 );
			}
			else
			if( numoperands == 2 )
			{
				if( opcode == 0x01 || opcode == 0x02 || opcode == 0x05 || opcode == 0x0B )
				{
					buffer += sprintf (buffer, "#");
					ea = operandarray[0];
					sym1 = set_ea_info(0, ea, EA_UINT8, EA_VALUE );
					buffer += sprintf (buffer, "%s", sym1 );

					buffer += sprintf (buffer, ",");

					ea = operandarray[1];
					sym1 = set_ea_info(0, ea, size, access );
					buffer += sprintf (buffer, "%s", sym1 );
				}
				else
				{
					ea = (operandarray[0] << 8) + operandarray[1];
					sym1 = set_ea_info(0, ea, size, access );
					buffer += sprintf (buffer, "%s", sym1 );
				}
			}
			else
			if( numoperands == 1 )
			{
				ea = operandarray[0];
				sym1 = set_ea_info(0, ea, size, access );
				buffer += sprintf (buffer, "%s", sym1 );
			}
		}
		break;
	}

	return p;
}

#endif
