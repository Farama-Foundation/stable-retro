/*****************************************************************************
 *
 *	 9900dasm.c
 *	 TMS 9900 disassembler
 *
 *	 Copyright (c) 1998 John Butler, all rights reserved.
 *	 Based on 6502dasm.c 6502/65c02/6510 disassembler by Juergen Buchmueller
 *
 *	 - This source code is released as freeware for non-commercial purposes.
 *	 - You are free to use and redistribute this code in modified or
 *	   unmodified form, provided you list me in the credits.
 *	 - If you modify this source code, you must add a notice to each modified
 *	   source file that it has been changed.  If you're a nice person, you
 *	   will clearly mark each change too.  :)
 *	 - The author of this copywritten work reserves the right to change the
 *     terms of its usage and license at any time, including retroactively
 *   - This entire notice must remain in the source code.
 *
 *****************************************************************************/


#include <stdio.h>
#include <string.h>
#include "memory.h"

#define RDOP(A) (cpu_readop(A) << 8) + (cpu_readop((A+1) & 0xffff))
#define RDWORD(A) (cpu_readop_arg(A) << 8) + (cpu_readop_arg((A+1) & 0xffff))

#define BITS_0to3	((OP>>12) & 0xf)
#define BITS_2to5	((OP>>10) & 0xf)
#define BITS_5to9	((OP>>6) & 0x1f)
#define BITS_3to7	((OP>>8) & 0x1f)
#define BITS_6to10	((OP>>5) & 0x1f)

#define BITS_0to1	((OP>>14) & 0x3)
#define BITS_0to4	((OP>>11) & 0x1f)
#define BITS_0to2	((OP>>13) & 0x7)
#define BITS_0to5	((OP>>10) & 0x3f)

#define	MASK	0x0000ffff
#define OPBITS(n1,n2)	((OP>>(15-(n2))) & (MASK>>(15-((n2)-(n1)))))


enum opcodes {
	_a=0,	_ab,	_c,		_cb,	_s,		_sb,	_soc,	_socb,	_szc,	_szcb,
	_mov,	_movb,	_coc,	_czc,	_xor,	_mpy,	_div,	_xop,	_b,		_bl,
	_blwp,	_clr,	_seto,	_inv,	_neg,	_abs,	_swpb,	_inc,	_inct,	_dec,
	_dect,	_x,		_ldcr,	_stcr,	_sbo,	_sbz,	_tb,	_jeq,	_jgt,	_jh,
	_jhe,	_jl,	_jle,	_jlt,	_jmp,	_jnc,	_jne,	_jno,	_joc,	_jop,
	_sla,	_sra,	_src,	_srl,	_ai,	_andi,	_ci,	_li,	_ori,	_lwpi,
	_limi,	_stst,	_stwp,	_rtwp,	_idle,	_rset,	_ckof,	_ckon,	_lrex,	_ill
};


static const char *token[]=
{
	"a",	"ab",	"c",	"cb",	"s",	"sb",	"soc",	"socb",	"szc",	"szcb",
	"mov",	"movb",	"coc",	"czc",	"xor",	"mpy",	"div",	"xop",	"b",	"bl",
	"blwp",	"clr",	"seto",	"inv",	"neg",	"abs",	"swpb",	"inc",	"inct",	"dec",
	"dect",	"x",	"ldcr",	"stcr",	"sbo",	"sbz",	"tb",	"jeq",	"jgt",	"jh",
	"jhe",	"jl",	"jle",	"jlt",	"jmp",	"jnc",	"jne",	"jno",	"joc",	"jop",
	"sla",	"sra",	"src",	"srl",	"ai",	"andi",	"ci",	"li",	"ori",	"lwpi",
	"limi",	"stst",	"stwp",	"rtwp",	"idle",	"rset",	"ckof",	"ckon",	"lrex",	"ill"
};


static const enum opcodes ops0to3[16]=
{
	_ill,	_ill,	_ill,	_ill,	_szc,	_szcb,	_s,		_sb,	/*0000-0111*/
	_c,		_cb,	_a,		_ab,	_mov,	_movb,	_soc,	_socb	/*1000-1111*/
};


static const enum opcodes ops2to5[16]=
{
	_ill,	_ill,	_ill,	_ill,	_ill,	_ill,	_ill,	_ill,	/*0000-0111*/
	_coc,	_czc,	_xor,	_xop,	_ldcr,	_stcr,	_mpy,	_div	/*1000-1111*/
};


static const enum opcodes ops5to9[32]=
{
	_ill,	_ill,	_ill,	_ill,	_ill,	_ill,	_ill,	_ill,	/*00000-00111*/
	_ill,	_ill,	_ill,	_ill,	_ill,	_ill,	_ill,	_ill,	/*01000-01111*/
	_blwp,	_b,		_x,		_clr,	_neg,	_inv,	_inc,	_inct,	/*10000-10111*/
	_dec,	_dect,	_bl,	_swpb,	_seto,	_abs,	_ill,	_ill	/*11000-11111*/
};


static const enum opcodes ops3to7[32]=
{
	_ill,	_ill,	_ill,	_ill,	_ill,	_ill,	_ill,	_ill,	/*00000-00111*/
	_sra,	_srl,	_sla,	_src,	_ill,	_ill,	_ill,	_ill,	/*01000-01111*/
	_jmp,	_jlt,	_jle,	_jeq,	_jhe,	_jgt,	_jne,	_jnc,	/*10000-10111*/
	_joc,	_jno,	_jl,	_jh,	_jop,	_sbo,	_sbz,	_tb		/*11000-11111*/
};


static const enum opcodes ops6to10[32]=
{
	_ill,	_ill,	_ill,	_ill,	_ill,	_ill,	_ill,	_ill,	/*00000-00111*/
	_ill,	_ill,	_ill,	_ill,	_ill,	_ill,	_ill,	_ill,	/*01000-01111*/
	_li,	_ai,	_andi,	_ori,	_ci,	_stwp,	_stst,	_lwpi,	/*10000-10111*/
	_limi,	_ill,	_idle,	_rset,	_rtwp,	_ckon,	_ckof,	_lrex	/*11000-11111*/
};

static int PC;


static char *print_arg (int mode, int arg)
{
	static char temp[20];
	int	base;

	switch (mode)
	{
		case 0x0:	/* workspace register */
			sprintf (temp, "R%d", arg);
			break;
		case 0x1:	/* workspace register indirect */
			sprintf (temp, "*R%d", arg);
			break;
		case 0x2:	/* symbolic|indexed */
			base = RDWORD(PC); PC+=2;
			if (arg) 	/* indexed */
				sprintf (temp, "@>%04x(R%d)", base, arg);
			else		/* symbolic (direct) */
				sprintf (temp, "@>%04x", base);
			break;
		case 0x3:	/* workspace register indirect auto increment */
			sprintf (temp, "*R%d+", arg);
			break;
	}
	return temp;
}


/*****************************************************************************
 *	Disassemble a single command and return the number of bytes it uses.
 *****************************************************************************/
int Dasm9900 (char *buffer, int pc)
{
	int	OP, opc;
	int sarg, darg, smode, dmode;

	PC = pc;
	OP = RDOP(PC); PC+=2;

	if ((opc = ops0to3[BITS_0to3]) != _ill)
	{
		smode = OPBITS(10,11);
		sarg = OPBITS(12,15);
		dmode = OPBITS(4,5);
		darg = OPBITS(6,9);

 		sprintf (buffer, "%-4s ", token[opc]);
		strcat (buffer, print_arg (smode, sarg));
		strcat (buffer, ",");
		strcat (buffer, print_arg (dmode, darg));
	}
	else if (BITS_0to1==0 && (opc = ops2to5[BITS_2to5]) != _ill)
	{
		smode = OPBITS(10,11);
		sarg = OPBITS(12,15);
		darg = OPBITS(6,9);

		if (darg==0 && (opc==_ldcr || opc==_stcr))
			darg = 16;

		if (opc==_xop || opc==_ldcr || opc==_stcr)
			sprintf (buffer, "%-4s %s,%d", token[opc], print_arg (smode, sarg), darg);
		else	/* _coc, _czc, _xor, _mpy, _div */
			sprintf (buffer, "%-4s %s,R%d", token[opc], print_arg (smode, sarg), darg);
	}
	else if (BITS_0to2==0 && (opc = ops3to7[BITS_3to7]) != _ill)
	{
		switch (opc)
		{
			case _sra: case _srl: case _sla: case _src:
				sarg = OPBITS(12,15);
				darg = OPBITS(8,11);

				sprintf (buffer, "%-4s R%d,%d", token[opc], sarg, darg);
				break;
			case _jmp: case _jlt: case _jle: case _jeq: case _jhe: case _jgt:
			case _jne: case _jnc: case _joc: case _jno: case _jl:  case _jh: case _jop:
				{
					signed char displacement;

					displacement = (signed char)OPBITS(8,15);
					sprintf (buffer, "%-4s >%04x", token[opc], 0xffff & (PC + displacement * 2));
				}
				break;
			case _sbo: case _sbz: case _tb:
				{
					signed char displacement;

					displacement = (signed char)OPBITS(8,15);
					sprintf (buffer, "%-4s >%04x", token[opc], 0xffff & displacement);
				}
				break;
		}
	}
	else if (BITS_0to4==0 && (opc = ops5to9[BITS_5to9]) != _ill)
	{
		smode = OPBITS(10,11);
		sarg = OPBITS(12,15);

		sprintf (buffer, "%-4s %s", token[opc], print_arg (smode, sarg));
	}
	else if (BITS_0to5==0 && (opc = ops6to10[BITS_6to10]) != _ill)
	{
		switch (opc)
		{
			case _li:   case _ai:   case _andi: case _ori:  case _ci:
				darg = OPBITS(12,15);
				sarg = RDWORD(PC); PC+=2;

				sprintf (buffer, "%-4s R%d,>%04x", token[opc], darg, sarg);
				break;
			case _lwpi: case _limi:
				sarg = RDWORD(PC); PC+=2;

				sprintf (buffer, "%-4s >%04x", token[opc], sarg);
				break;
			case _stwp: case _stst:
				sarg = OPBITS(12,15);

				sprintf (buffer, "%-4s R%d", token[opc], sarg);
				break;
			case _idle: case _rset: case _rtwp: case _ckon: case _ckof: case _lrex:
				sprintf (buffer, "%-4s", token[opc]);
				break;
		}
	}
	else
		sprintf (buffer, "data >%04x", OP);

	return PC - pc;
}
