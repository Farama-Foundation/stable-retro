/* this code was hacked out of the fully-featured 6809 disassembler by Sean Riddle */

/* 6809dasm.c - a 6809 opcode disassembler */
/* Version 1.4 1-MAR-95 */
/* Copyright © 1995 Sean Riddle */

/* thanks to Franklin Bowen for bug fixes, ideas */

/* Freely distributable on any medium given all copyrights are retained */
/* by the author and no charge greater than $7.00 is made for obtaining */
/* this software */

/* Please send all bug reports, update ideas and data files to: */
/* sriddle@ionet.net */
#include <stdio.h>

#ifdef MAME_DEBUG

#include <string.h>
#include "osd_cpu.h"
#include "cpuintrf.h"
#include "mamedbg.h"
#include "m6809.h"

#ifndef TRUE
#define TRUE	-1
#define FALSE	0
#endif

typedef struct {                /* opcode structure */
   UINT8	opcode; 			/* 8-bit opcode value */
   UINT8	numoperands;
   char 	name[6];			/* opcode name */
   UINT8	mode;				/* addressing mode */
   UINT8	size;				/* access size */
   UINT8	access; 			/* access mode */
   UINT8	numcycles;			/* number of cycles - not used */
} opcodeinfo;

/* 6809 ADDRESSING MODES */
enum M6809_ADDRESSING_MODES {
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
#define NUMPG1OPS 223
#define NUMPG2OPS 38
#define NUMPG3OPS 9

int numops[3] =
{
   NUMPG1OPS,NUMPG2OPS,NUMPG3OPS,
};

#if 0
static char modenames[9][14] = {
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
static opcodeinfo pg1opcodes[NUMPG1OPS] =
{
	{  0,1,"NEG",     DIR, EA_UINT8, EA_ZPG_RDWR,  6},
	{  3,1,"COM",     DIR, EA_UINT8, EA_ZPG_RDWR,  6},
	{  4,1,"LSR",     DIR, EA_UINT8, EA_ZPG_RDWR,  6},
	{  6,1,"ROR",     DIR, EA_UINT8, EA_ZPG_RDWR,  6},
	{  7,1,"ASR",     DIR, EA_UINT8, EA_ZPG_RDWR,  6},
	{  8,1,"ASL",     DIR, EA_UINT8, EA_ZPG_RDWR,  6},
	{  9,1,"ROL",     DIR, EA_UINT8, EA_ZPG_RDWR,  6},
	{ 10,1,"DEC",     DIR, EA_UINT8, EA_ZPG_RDWR,  6},
	{ 12,1,"INC",     DIR, EA_UINT8, EA_ZPG_RDWR,  6},
	{ 13,1,"TST",     DIR, EA_UINT8, EA_ZPG_RDWR,  6},
	{ 14,1,"JMP",     DIR, EA_UINT8, EA_ABS_PC,    3},
	{ 15,1,"CLR",     DIR, EA_UINT8, EA_ZPG_WR,    6},

	{ 16,1,"page2",   PG2, 0,        0,            0},
	{ 17,1,"page3",   PG3, 0,        0,            0},
	{ 18,0,"NOP",     INH, 0,        0,            2},
	{ 19,0,"SYNC",    INH, 0,        0,            4},
	{ 22,2,"LBRA",    LREL,EA_INT16, EA_REL_PC,    5},
	{ 23,2,"LBSR",    LREL,EA_INT16, EA_REL_PC,    9},
	{ 25,0,"DAA",     INH, 0,        0,            2},
	{ 26,1,"ORCC",    IMM, 0,        0,            3},
	{ 28,1,"ANDCC",   IMM, 0,        0,            3},
	{ 29,0,"SEX",     INH, 0,        0,            2},
	{ 30,1,"EXG",     IMM, 0,        0,            8},
	{ 31,1,"TFR",     IMM, 0,        0,            6},

	{ 32,1,"BRA",     REL, EA_INT8,  EA_REL_PC,    3},
	{ 33,1,"BRN",     REL, EA_INT8,  EA_REL_PC,    3},
	{ 34,1,"BHI",     REL, EA_INT8,  EA_REL_PC,    3},
	{ 35,1,"BLS",     REL, EA_INT8,  EA_REL_PC,    3},
	{ 36,1,"BCC",     REL, EA_INT8,  EA_REL_PC,    3},
	{ 37,1,"BCS",     REL, EA_INT8,  EA_REL_PC,    3},
	{ 38,1,"BNE",     REL, EA_INT8,  EA_REL_PC,    3},
	{ 39,1,"BEQ",     REL, EA_INT8,  EA_REL_PC,    3},
	{ 40,1,"BVC",     REL, EA_INT8,  EA_REL_PC,    3},
	{ 41,1,"BVS",     REL, EA_INT8,  EA_REL_PC,    3},
	{ 42,1,"BPL",     REL, EA_INT8,  EA_REL_PC,    3},
	{ 43,1,"BMI",     REL, EA_INT8,  EA_REL_PC,    3},
	{ 44,1,"BGE",     REL, EA_INT8,  EA_REL_PC,    3},
	{ 45,1,"BLT",     REL, EA_INT8,  EA_REL_PC,    3},
	{ 46,1,"BGT",     REL, EA_INT8,  EA_REL_PC,    3},
	{ 47,1,"BLE",     REL, EA_INT8,  EA_REL_PC,    3},

	{ 48,1,"LEAX",    IND, EA_UINT16,EA_VALUE,     2},
	{ 49,1,"LEAY",    IND, EA_UINT16,EA_VALUE,     2},
	{ 50,1,"LEAS",    IND, EA_UINT16,EA_VALUE,     2},
	{ 51,1,"LEAU",    IND, EA_UINT16,EA_VALUE,     2},
	{ 52,1,"PSHS",    INH, 0,        0,            5},
	{ 53,1,"PULS",    INH, 0,        0,            5},
	{ 54,1,"PSHU",    INH, 0,        0,            5},
	{ 55,1,"PULU",    INH, 0,        0,            5},
	{ 57,0,"RTS",     INH, 0,        0,            5},
	{ 58,0,"ABX",     INH, 0,        0,            3},
	{ 59,0,"RTI",     INH, 0,        0,            6},
	{ 60,1,"CWAI",    IMM, 0,        0,           20},
	{ 61,0,"MUL",     INH, 0,        0,           11},
	{ 63,0,"SWI",     INH, 0,        0,           19},

	{ 64,0,"NEGA",    INH, 0,        0,            2},
	{ 67,0,"COMA",    INH, 0,        0,            2},
	{ 68,0,"LSRA",    INH, 0,        0,            2},
	{ 70,0,"RORA",    INH, 0,        0,            2},
	{ 71,0,"ASRA",    INH, 0,        0,            2},
	{ 72,0,"ASLA",    INH, 0,        0,            2},
	{ 73,0,"ROLA",    INH, 0,        0,            2},
	{ 74,0,"DECA",    INH, 0,        0,            2},
	{ 76,0,"INCA",    INH, 0,        0,            2},
	{ 77,0,"TSTA",    INH, 0,        0,            2},
	{ 79,0,"CLRA",    INH, 0,        0,            2},

	{ 80,0,"NEGB",    INH, 0,        0,            2},
	{ 83,0,"COMB",    INH, 0,        0,            2},
	{ 84,0,"LSRB",    INH, 0,        0,            2},
	{ 86,0,"RORB",    INH, 0,        0,            2},
	{ 87,0,"ASRB",    INH, 0,        0,            2},
	{ 88,0,"ASLB",    INH, 0,        0,            2},
	{ 89,0,"ROLB",    INH, 0,        0,            2},
	{ 90,0,"DECB",    INH, 0,        0,            2},
	{ 92,0,"INCB",    INH, 0,        0,            2},
	{ 93,0,"TSTB",    INH, 0,        0,            2},
	{ 95,0,"CLRB",    INH, 0,        0,            2},

	{ 96,1,"NEG",     IND, EA_UINT8, EA_MEM_RDWR,  6},
	{ 99,1,"COM",     IND, EA_UINT8, EA_MEM_RDWR,  6},
	{100,1,"LSR",     IND, EA_UINT8, EA_MEM_RDWR,  6},
	{102,1,"ROR",     IND, EA_UINT8, EA_MEM_RDWR,  6},
	{103,1,"ASR",     IND, EA_UINT8, EA_MEM_RDWR,  6},
	{104,1,"ASL",     IND, EA_UINT8, EA_MEM_RDWR,  6},
	{105,1,"ROL",     IND, EA_UINT8, EA_MEM_RDWR,  6},
	{106,1,"DEC",     IND, EA_UINT8, EA_MEM_RDWR,  6},
	{108,1,"INC",     IND, EA_UINT8, EA_MEM_RDWR,  6},
	{109,1,"TST",     IND, EA_UINT8, EA_MEM_RD,    6},
	{110,1,"JMP",     IND, EA_UINT8, EA_ABS_PC,    3},
	{111,1,"CLR",     IND, EA_UINT8, EA_MEM_WR,    6},

	{112,2,"NEG",     EXT, EA_UINT8, EA_MEM_RDWR,  7},
	{115,2,"COM",     EXT, EA_UINT8, EA_MEM_RDWR,  7},
	{116,2,"LSR",     EXT, EA_UINT8, EA_MEM_RDWR,  7},
	{118,2,"ROR",     EXT, EA_UINT8, EA_MEM_RDWR,  7},
	{119,2,"ASR",     EXT, EA_UINT8, EA_MEM_RDWR,  7},
	{120,2,"ASL",     EXT, EA_UINT8, EA_MEM_RDWR,  7},
	{121,2,"ROL",     EXT, EA_UINT8, EA_MEM_RDWR,  7},
	{122,2,"DEC",     EXT, EA_UINT8, EA_MEM_RDWR,  7},
	{124,2,"INC",     EXT, EA_UINT8, EA_MEM_RDWR,  7},
	{125,2,"TST",     EXT, EA_UINT8, EA_MEM_RD,    7},
	{126,2,"JMP",     EXT, EA_UINT8, EA_ABS_PC,    4},
	{127,2,"CLR",     EXT, EA_UINT8, EA_MEM_WR,    7},

	{128,1,"SUBA",    IMM, EA_UINT8, EA_VALUE,     2},
	{129,1,"CMPA",    IMM, EA_UINT8, EA_VALUE,     2},
	{130,1,"SBCA",    IMM, EA_UINT8, EA_VALUE,     2},
	{131,2,"SUBD",    IMM, EA_UINT16,EA_VALUE,     4},
	{132,1,"ANDA",    IMM, EA_UINT8, EA_VALUE,     2},
	{133,1,"BITA",    IMM, EA_UINT8, EA_VALUE,     2},
	{134,1,"LDA",     IMM, EA_UINT8, EA_VALUE,     2},
	{136,1,"EORA",    IMM, EA_UINT8, EA_VALUE,     2},
	{137,1,"ADCA",    IMM, EA_UINT8, EA_VALUE,     2},
	{138,1,"ORA",     IMM, EA_UINT8, EA_VALUE,     2},
	{139,1,"ADDA",    IMM, EA_UINT8, EA_VALUE,     2},
	{140,2,"CMPX",    IMM, EA_UINT16,EA_VALUE,     4},
	{141,1,"BSR",     REL, EA_INT8,  EA_REL_PC,    7},
	{142,2,"LDX",     IMM, EA_UINT16,EA_VALUE,     3},

	{144,1,"SUBA",    DIR, EA_UINT8, EA_ZPG_RD,    4},
	{145,1,"CMPA",    DIR, EA_UINT8, EA_ZPG_RD,    4},
	{146,1,"SBCA",    DIR, EA_UINT8, EA_ZPG_RD,    4},
	{147,1,"SUBD",    DIR, EA_UINT16,EA_ZPG_RD,    6},
	{148,1,"ANDA",    DIR, EA_UINT8, EA_ZPG_RD,    4},
	{149,1,"BITA",    DIR, EA_UINT8, EA_ZPG_RD,    4},
	{150,1,"LDA",     DIR, EA_UINT8, EA_ZPG_RD,    4},
	{151,1,"STA",     DIR, EA_UINT8, EA_ZPG_WR,    4},
	{152,1,"EORA",    DIR, EA_UINT8, EA_ZPG_RD,    4},
	{153,1,"ADCA",    DIR, EA_UINT8, EA_ZPG_RD,    4},
	{154,1,"ORA",     DIR, EA_UINT8, EA_ZPG_RD,    4},
	{155,1,"ADDA",    DIR, EA_UINT8, EA_ZPG_RD,    4},
	{156,1,"CMPX",    DIR, EA_UINT16,EA_ZPG_RD,    6},
	{157,1,"JSR",     DIR, EA_UINT8, EA_ABS_PC,    7},
	{158,1,"LDX",     DIR, EA_UINT16,EA_ZPG_RD,    5},
	{159,1,"STX",     DIR, EA_UINT16,EA_ZPG_WR,    5},

	{160,1,"SUBA",    IND, EA_UINT8, EA_MEM_RD,    4},
	{161,1,"CMPA",    IND, EA_UINT8, EA_MEM_RD,    4},
	{162,1,"SBCA",    IND, EA_UINT8, EA_MEM_RD,    4},
	{163,1,"SUBD",    IND, EA_UINT16,EA_MEM_RD,    6},
	{164,1,"ANDA",    IND, EA_UINT8, EA_MEM_RD,    4},
	{165,1,"BITA",    IND, EA_UINT8, EA_MEM_RD,    4},
	{166,1,"LDA",     IND, EA_UINT8, EA_MEM_RD,    4},
	{167,1,"STA",     IND, EA_UINT8, EA_MEM_WR,    4},
	{168,1,"EORA",    IND, EA_UINT8, EA_MEM_RD,    4},
	{169,1,"ADCA",    IND, EA_UINT8, EA_MEM_RD,    4},
	{170,1,"ORA",     IND, EA_UINT8, EA_MEM_RD,    4},
	{171,1,"ADDA",    IND, EA_UINT8, EA_MEM_RD,    4},
	{172,1,"CMPX",    IND, EA_UINT16,EA_MEM_RD,    6},
	{173,1,"JSR",     IND, EA_UINT8, EA_ABS_PC,    7},
	{174,1,"LDX",     IND, EA_UINT16,EA_MEM_RD,    5},
	{175,1,"STX",     IND, EA_UINT16,EA_MEM_WR,    5},

	{176,2,"SUBA",    EXT, EA_UINT8, EA_MEM_RD,    5},
	{177,2,"CMPA",    EXT, EA_UINT8, EA_MEM_RD,    5},
	{178,2,"SBCA",    EXT, EA_UINT8, EA_MEM_RD,    5},
	{179,2,"SUBD",    EXT, EA_UINT16,EA_MEM_RD,    7},
	{180,2,"ANDA",    EXT, EA_UINT8, EA_MEM_RD,    5},
	{181,2,"BITA",    EXT, EA_UINT8, EA_MEM_RD,    5},
	{182,2,"LDA",     EXT, EA_UINT8, EA_MEM_RD,    5},
	{183,2,"STA",     EXT, EA_UINT8, EA_MEM_WR,    5},
	{184,2,"EORA",    EXT, EA_UINT8, EA_MEM_RD,    5},
	{185,2,"ADCA",    EXT, EA_UINT8, EA_MEM_RD,    5},
	{186,2,"ORA",     EXT, EA_UINT8, EA_MEM_RD,    5},
	{187,2,"ADDA",    EXT, EA_UINT8, EA_MEM_RD,    5},
	{188,2,"CMPX",    EXT, EA_UINT16,EA_MEM_RD,    7},
	{189,2,"JSR",     EXT, EA_UINT8, EA_ABS_PC,    8},
	{190,2,"LDX",     EXT, EA_UINT16,EA_MEM_RD,    6},
	{191,2,"STX",     EXT, EA_UINT16,EA_MEM_WR,    6},

	{192,1,"SUBB",    IMM, EA_UINT8, EA_VALUE,     2},
	{193,1,"CMPB",    IMM, EA_UINT8, EA_VALUE,     2},
	{194,1,"SBCB",    IMM, EA_UINT8, EA_VALUE,     2},
	{195,2,"ADDD",    IMM, EA_UINT16,EA_VALUE,     4},
	{196,1,"ANDB",    IMM, EA_UINT8, EA_VALUE,     2},
	{197,1,"BITB",    IMM, EA_UINT8, EA_VALUE,     2},
	{198,1,"LDB",     IMM, EA_UINT8, EA_VALUE,     2},
	{200,1,"EORB",    IMM, EA_UINT8, EA_VALUE,     2},
	{201,1,"ADCB",    IMM, EA_UINT8, EA_VALUE,     2},
	{202,1,"ORB",     IMM, EA_UINT8, EA_VALUE,     2},
	{203,1,"ADDB",    IMM, EA_UINT8, EA_VALUE,     2},
	{204,2,"LDD",     IMM, EA_UINT16,EA_VALUE,     3},
	{206,2,"LDU",     IMM, EA_UINT16,EA_VALUE,     3},

	{208,1,"SUBB",    DIR, EA_UINT8, EA_ZPG_RD,    4},
	{209,1,"CMPB",    DIR, EA_UINT8, EA_ZPG_RD,    4},
	{210,1,"SBCB",    DIR, EA_UINT8, EA_ZPG_RD,    4},
	{211,1,"ADDD",    DIR, EA_UINT8, EA_ZPG_RD,    6},
	{212,1,"ANDB",    DIR, EA_UINT8, EA_ZPG_RD,    4},
	{213,1,"BITB",    DIR, EA_UINT8, EA_ZPG_RD,    4},
	{214,1,"LDB",     DIR, EA_UINT8, EA_ZPG_RD,    4},
	{215,1,"STB",     DIR, EA_UINT8, EA_ZPG_WR,    4},
	{216,1,"EORB",    DIR, EA_UINT8, EA_ZPG_RD,    4},
	{217,1,"ADCB",    DIR, EA_UINT8, EA_ZPG_RD,    4},
	{218,1,"ORB",     DIR, EA_UINT8, EA_ZPG_RD,    4},
	{219,1,"ADDB",    DIR, EA_UINT8, EA_ZPG_RD,    4},
	{220,1,"LDD",     DIR, EA_UINT16,EA_ZPG_RD,    5},
	{221,1,"STD",     DIR, EA_UINT16,EA_ZPG_WR,    5},
	{222,1,"LDU",     DIR, EA_UINT16,EA_ZPG_RD,    5},
	{223,1,"STU",     DIR, EA_UINT16,EA_ZPG_WR,    5},

	{224,1,"SUBB",    IND, EA_UINT8, EA_MEM_RD,    4},
	{225,1,"CMPB",    IND, EA_UINT8, EA_MEM_RD,    4},
	{226,1,"SBCB",    IND, EA_UINT8, EA_MEM_RD,    4},
	{227,1,"ADDD",    IND, EA_UINT8, EA_MEM_RD,    6},
	{228,1,"ANDB",    IND, EA_UINT8, EA_MEM_RD,    4},
	{229,1,"BITB",    IND, EA_UINT8, EA_MEM_RD,    4},
	{230,1,"LDB",     IND, EA_UINT8, EA_MEM_RD,    4},
	{231,1,"STB",     IND, EA_UINT8, EA_MEM_WR,    4},
	{232,1,"EORB",    IND, EA_UINT8, EA_MEM_RD,    4},
	{233,1,"ADCB",    IND, EA_UINT8, EA_MEM_RD,    4},
	{234,1,"ORB",     IND, EA_UINT8, EA_MEM_RD,    4},
	{235,1,"ADDB",    IND, EA_UINT8, EA_MEM_RD,    4},
	{236,1,"LDD",     IND, EA_UINT16,EA_MEM_RD,    5},
	{237,1,"STD",     IND, EA_UINT16,EA_MEM_WR,    5},
	{238,1,"LDU",     IND, EA_UINT16,EA_MEM_RD,    5},
	{239,1,"STU",     IND, EA_UINT16,EA_MEM_WR,    5},

	{240,2,"SUBB",    EXT, EA_UINT8, EA_MEM_RD,    5},
	{241,2,"CMPB",    EXT, EA_UINT8, EA_MEM_RD,    5},
	{242,2,"SBCB",    EXT, EA_UINT8, EA_MEM_RD,    5},
	{243,2,"ADDD",    EXT, EA_UINT8, EA_MEM_RD,    7},
	{244,2,"ANDB",    EXT, EA_UINT8, EA_MEM_RD,    5},
	{245,2,"BITB",    EXT, EA_UINT8, EA_MEM_RD,    5},
	{246,2,"LDB",     EXT, EA_UINT8, EA_MEM_RD,    5},
	{247,2,"STB",     EXT, EA_UINT8, EA_MEM_WR,    5},
	{248,2,"EORB",    EXT, EA_UINT8, EA_MEM_RD,    5},
	{249,2,"ADCB",    EXT, EA_UINT8, EA_MEM_RD,    5},
	{250,2,"ORB",     EXT, EA_UINT8, EA_MEM_RD,    5},
	{251,2,"ADDB",    EXT, EA_UINT8, EA_MEM_RD,    5},
	{252,2,"LDD",     EXT, EA_UINT16,EA_MEM_RD,    6},
	{253,2,"STD",     EXT, EA_UINT16,EA_MEM_WR,    6},
	{254,2,"LDU",     EXT, EA_UINT16,EA_MEM_RD,    6},
	{255,2,"STU",     EXT, EA_UINT16,EA_MEM_WR,    6},
};

/* page 2 ops 10xx*/
static opcodeinfo pg2opcodes[NUMPG2OPS] =
{
	{ 33,3,"LBRN",    LREL,EA_INT16, EA_REL_PC,    5},
	{ 34,3,"LBHI",    LREL,EA_INT16, EA_REL_PC,    5},
	{ 35,3,"LBLS",    LREL,EA_INT16, EA_REL_PC,    5},
	{ 36,3,"LBCC",    LREL,EA_INT16, EA_REL_PC,    5},
	{ 37,3,"LBCS",    LREL,EA_INT16, EA_REL_PC,    5},
	{ 38,3,"LBNE",    LREL,EA_INT16, EA_REL_PC,    5},
	{ 39,3,"LBEQ",    LREL,EA_INT16, EA_REL_PC,    5},
	{ 40,3,"LBVC",    LREL,EA_INT16, EA_REL_PC,    5},
	{ 41,3,"LBVS",    LREL,EA_INT16, EA_REL_PC,    5},
	{ 42,3,"LBPL",    LREL,EA_INT16, EA_REL_PC,    5},
	{ 43,3,"LBMI",    LREL,EA_INT16, EA_REL_PC,    5},
	{ 44,3,"LBGE",    LREL,EA_INT16, EA_REL_PC,    5},
	{ 45,3,"LBLT",    LREL,EA_INT16, EA_REL_PC,    5},
	{ 46,3,"LBGT",    LREL,EA_INT16, EA_REL_PC,    5},
	{ 47,3,"LBLE",    LREL,EA_INT16, EA_REL_PC,    5},
	{ 63,2,"SWI2",    INH, 0,        0,           20},
	{131,3,"CMPD",    IMM, EA_UINT16,EA_VALUE,     5},
	{140,3,"CMPY",    IMM, EA_UINT16,EA_VALUE,     5},
	{142,3,"LDY",     IMM, EA_UINT16,EA_VALUE,     4},
	{147,2,"CMPD",    DIR, EA_UINT16,EA_ZPG_RD,    7},
	{156,2,"CMPY",    DIR, EA_UINT16,EA_ZPG_RD,    7},
	{158,2,"LDY",     DIR, EA_UINT16,EA_ZPG_RD,    6},
	{159,2,"STY",     DIR, EA_UINT16,EA_ZPG_RD,    6},
	{163,2,"CMPD",    IND, EA_UINT16,EA_MEM_RD,    7},
	{172,2,"CMPY",    IND, EA_UINT16,EA_MEM_RD,    7},
	{174,2,"LDY",     IND, EA_UINT16,EA_MEM_RD,    6},
	{175,2,"STY",     IND, EA_UINT16,EA_MEM_RD,    6},
	{179,3,"CMPD",    EXT, EA_UINT16,EA_MEM_RD,    8},
	{188,3,"CMPY",    EXT, EA_UINT16,EA_MEM_RD,    8},
	{190,3,"LDY",     EXT, EA_UINT16,EA_MEM_RD,    7},
	{191,3,"STY",     EXT, EA_UINT16,EA_MEM_RD,    7},
	{206,3,"LDS",     IMM, EA_UINT16,EA_VALUE,     4},
	{222,2,"LDS",     DIR, EA_UINT16,EA_ZPG_RD,    6},
	{223,2,"STS",     DIR, EA_UINT16,EA_ZPG_WR,    6},
	{238,2,"LDS",     IND, EA_UINT16,EA_MEM_RD,    6},
	{239,2,"STS",     IND, EA_UINT16,EA_MEM_WR,    6},
	{254,3,"LDS",     EXT, EA_UINT16,EA_MEM_RD,    7},
	{255,3,"STS",     EXT, EA_UINT16,EA_MEM_WR,    7},
};

/* page 3 ops 11xx */
static opcodeinfo pg3opcodes[NUMPG3OPS] =
{
	{ 63,1,"SWI3",    INH, 0,        0,           20},
	{131,3,"CMPU",    IMM, EA_UINT16,EA_VALUE,     5},
	{140,3,"CMPS",    IMM, EA_UINT16,EA_VALUE,     5},
	{147,2,"CMPU",    DIR, EA_UINT16,EA_ZPG_RD,    7},
	{156,2,"CMPS",    DIR, EA_UINT16,EA_ZPG_RD,    7},
	{163,2,"CMPU",    IND, EA_UINT16,EA_MEM_RD,    7},
	{172,2,"CMPS",    IND, EA_UINT16,EA_MEM_RD,    7},
	{179,3,"CMPU",    EXT, EA_UINT16,EA_MEM_RD,    8},
	{188,3,"CMPS",    EXT, EA_UINT16,EA_MEM_RD,    8},
};

static opcodeinfo *pgpointers[3] =
{
   pg1opcodes,pg2opcodes,pg3opcodes,
};

static const UINT8 regid_6809[5] = {
	M6809_X, M6809_Y, M6809_U, M6809_S, M6809_PC
};

static const char *regs_6809[5] = {
	"X","Y","U","S","PC"
};

static const char *teregs[16] = {
	"D","X","Y","U","S","PC","inv","inv",
	"A","B","CC","DP","inv","inv","inv","inv"
};

static char *hexstring (int address)
{
	static char labtemp[10];
	sprintf (labtemp, "$%04hX", address);
	return labtemp;
}

unsigned Dasm6809 (char *buffer, unsigned pc)
{
	int i, j, k, page, opcode, numoperands, mode, size, access;
	UINT8 operandarray[4];
	const char *sym1, *sym2;
	int rel, pb, offset, reg, pb2;
	unsigned ea = 0;
    int p = 0;

	*buffer = '\0';

    opcode = cpu_readop(pc+(p++));
	for( i = 0; i < numops[0]; i++ )
		if (pg1opcodes[i].opcode == opcode)
			break;

	if( i < numops[0] )
	{
		if( pg1opcodes[i].mode >= PG2 )
		{
			opcode = cpu_readop(pc+(p++));
			page = pg1opcodes[i].mode - PG2 + 1;          /* get page # */
			for( k = 0; k < numops[page]; k++ )
				if (opcode == pgpointers[page][k].opcode)
					break;

			if( k != numops[page] )
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
				strcpy (buffer, "Illegal Opcode");
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
		strcpy (buffer, "Illegal Opcode");
		return 1;
	}

	pc += p;

	if( opcode != 0x1f && opcode != 0x1e )
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
		pb = operandarray[0];
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
				ea = (pc + (INT8)offset + activecpu_get_reg(regid_6809[reg])) & 0xffff;
				buffer += sprintf (buffer, "%s,%s", sym1, regs_6809[reg]);
			}
			else
			{
				sym1 = set_ea_info(1, offset, EA_INT8, EA_VALUE);
				ea = (activecpu_get_reg(regid_6809[reg]) + offset) & 0xffff;
				buffer += sprintf (buffer, "%s,%s", sym1, regs_6809[reg]);
			}
/*			  if( pb == 0x8c )*/
/*				  buffer += sprintf (buffer, " ; ($%04X)", offset + pc);*/
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
                buffer += sprintf (buffer, "%s,%s", sym1, regs_6809[reg]);
			}
			else
			if( pb2 == 0x8f )
			{
				sym1 = set_ea_info(1, offset, EA_INT16, EA_VALUE);
				ea = offset;
                buffer += sprintf (buffer, "%s", sym1);
			}
			else
			{
				sym1 = set_ea_info(1, offset, EA_INT16, EA_VALUE);
				ea = (activecpu_get_reg(regid_6809[reg]) + offset) & 0xffff;
                buffer += sprintf (buffer, "%s,%s", sym1, regs_6809[reg]);
			}
/*			  if( pb == 0x8d )*/
/*				  buffer += sprintf (buffer, " ; ($%04X)", offset + pc);*/
		}
		else
		if( pb & 0x80 )
		{
			if( (pb & 0x90) == 0x90 )
				buffer += sprintf (buffer, "[");
			switch( pb & 0x8f )
			{
			case 0x80:
				ea = activecpu_get_reg(regid_6809[reg]);
				buffer += sprintf (buffer, ",%s+", regs_6809[reg]);
				break;
			case 0x81:
				ea = activecpu_get_reg(regid_6809[reg]);
                buffer += sprintf (buffer, ",%s++", regs_6809[reg]);
				break;
			case 0x82:
				ea = activecpu_get_reg(regid_6809[reg]);
                buffer += sprintf (buffer, ",-%s", regs_6809[reg]);
				break;
			case 0x83:
				ea = activecpu_get_reg(regid_6809[reg]);
                buffer += sprintf (buffer, ",--%s", regs_6809[reg]);
				break;
			case 0x84:
				ea = activecpu_get_reg(regid_6809[reg]);
                buffer += sprintf (buffer, ",%s", regs_6809[reg]);
				break;
			case 0x85:
				ea = (activecpu_get_reg(regid_6809[reg]) + (INT8) activecpu_get_reg(M6809_B)) & 0xffff;
                buffer += sprintf (buffer, "B,%s", regs_6809[reg]);
				break;
			case 0x86:
				ea = (activecpu_get_reg(regid_6809[reg]) + (INT8) activecpu_get_reg(M6809_A)) & 0xffff;
                buffer += sprintf (buffer, "A,%s", regs_6809[reg]);
				break;
			case 0x8b:
				ea = (activecpu_get_reg(regid_6809[reg]) + (activecpu_get_reg(M6809_A) << 8) + activecpu_get_reg(M6809_B)) & 0xffff;
                buffer += sprintf (buffer, "D,%s", regs_6809[reg]);
				break;
			}
		}
		else
		{										   /* 5-bit offset */
			offset = pb & 0x1f;
			if (offset > 15)
				offset = offset - 32;
			buffer += sprintf (buffer, "%s,%s", hexstring (offset), regs_6809[reg]);
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
		if( opcode == 0x1f || opcode == 0x1e )
		{	/* TFR/EXG */
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
                strcat (buffer, "A");
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
			if( numoperands == 2 )
			{
				ea = (operandarray[0] << 8) + operandarray[1];
				sym1 = set_ea_info(0, ea, size, access );
				buffer += sprintf (buffer, "%s", sym1 );
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
