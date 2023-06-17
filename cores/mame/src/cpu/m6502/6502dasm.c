/*****************************************************************************
 *
 *	6502dasm.c
 *	6502/65c02/6510 disassembler
 *
 *	Copyright (c) 1998 Juergen Buchmueller, all rights reserved.
 *
 *	- This source code is released as freeware for non-commercial purposes.
 *	- You are free to use and redistribute this code in modified or
 *	  unmodified form, provided you list me in the credits.
 *	- If you modify this source code, you must add a notice to each modified
 *	  source file that it has been changed.  If you're a nice person, you
 *	  will clearly mark each change too.  :)
 *	- If you wish to use this for commercial purposes, please contact me at
 *	  pullmoll@t-online.de
 *	- The author of this copywritten work reserves the right to change the
 *	   terms of its usage and license at any time, including retroactively
 *	 - This entire notice must remain in the source code.
 *
 *****************************************************************************/
/* 2. February 2000 PeT added support for 65sc02 subtype */
/* 2. February 2000 PeT added support for 65ce02 variant */
/* 3. February 2000 PeT bbr bbs displayment */
/* 4. February 2000 PeT ply inw dew */
/* 4. February 2000 PeT fixed relative word operand */
/* 9. May 2000 PeT added m4510 */

#include <stdio.h>
#ifdef MAME_DEBUG
#include "driver.h"
#include "mamedbg.h"
#include "m6502.h"
#if (HAS_M65CE02)
#include "m65ce02.h"
#endif
#if (HAS_M6509)
#include "m6509.h"
#endif
#if (HAS_M4510)
#include "m4510.h"
#endif

#define OPCODE(A)  cpu_readop(A)
#define ARGBYTE(A) cpu_readop_arg(A)
#define ARGWORD(A) cpu_readop_arg(A)+(cpu_readop_arg((A+1) & 0xffff) << 8)

#define RDMEM(A)   cpu_readmem16(A)

enum addr_mode {
	non,   /* no additional arguments */
	imp,   /* implicit */
	acc,   /* accumulator */
	imm,   /* immediate */
	iw2,   /* immediate word (65ce02) */
	iw3,   /* augment (65ce02) */
	adr,   /* absolute address (jmp,jsr) */
	aba,   /* absolute */
	zpg,   /* zero page */
	zpx,   /* zero page + X */
	zpy,   /* zero page + Y */
	zpi,   /* zero page indirect (65c02) */
	zpb,   /* zero page and branch (65c02 bbr,bbs) */
	abx,   /* absolute + X */
	aby,   /* absolute + Y */
	rel,   /* relative */
	rw2,   /* relative word (65cs02, 65ce02) */
	idx,   /* zero page pre indexed */
	idy,   /* zero page post indexed */
	idz,   /* zero page post indexed (65ce02) */
	isy,   /* zero page pre indexed sp and post indexed y (65ce02) */
	ind,   /* indirect (jmp) */
	iax    /* indirect + X (65c02 jmp) */
};

enum opcodes {
	adc,  and, asl,  bcc,  bcs,  beq,  bit,  bmi,
	bne,  bpl, brk,  bvc,  bvs,  clc,  cld,  cli,
	clv,  cmp, cpx,  cpy,  dec,  dex,  dey,  eor,
	inc,  inx, iny,  jmp,  jsr,  lda,  ldx,  ldy,
	lsr,  nop, ora,  pha,  php,  pla,  plp,  rol,
	ror,  rti, rts,  sbc,  sec,  sed,  sei,  sta,
	stx,  sty, tax,  tay,  tsx,  txa,  txs,  tya,
	ill,
/* 65c02 (only) mnemonics */
	bbr,  bbs, bra,  rmb,  smb,  stz,  trb,  tsb,
/* 65sc02 (only) mnemonics */
	bsr,
/* 6510 + 65c02 mnemonics */
	anc,  asr, ast,  arr,  asx,  axa,  dcp,  dea,
	dop,  ina, isc,  lax,  phx,  phy,  plx,  ply,
	rla,  rra, sax,  slo,  sre,  sah,  say,  ssh,
	sxh,  syh, top,  oal,  kil,
/* 65ce02 mnemonics */
	cle,  see,  rtn,  aug,
	tab,  tba,	taz,  tza, tys, tsy,
	ldz,  stz2/* real register store */,
	dez,  inz,	cpz,  phz,	plz,
	neg,  asr2/* arithmetic shift right */,
	asw,  row,	dew,  inw,	phw,
/* 4510 mnemonics */
	map,

/* Deco CPU16 mnemonics */
	vbl,  u13,  u8F,  uAB,  u87,  u0B,  uA3,  u4B,
	u3F,  uBB,  u23
};


static const char *token[]=
{
	"adc", "and", "asl", "bcc", "bcs", "beq", "bit", "bmi",
	"bne", "bpl", "brk", "bvc", "bvs", "clc", "cld", "cli",
	"clv", "cmp", "cpx", "cpy", "dec", "dex", "dey", "eor",
	"inc", "inx", "iny", "jmp", "jsr", "lda", "ldx", "ldy",
	"lsr", "nop", "ora", "pha", "php", "pla", "plp", "rol",
	"ror", "rti", "rts", "sbc", "sec", "sed", "sei", "sta",
	"stx", "sty", "tax", "tay", "tsx", "txa", "txs", "tya",
	"ill",
/* 65c02 mnemonics */
	"bbr", "bbs", "bra", "rmb", "smb", "stz", "trb", "tsb",
/* 65sc02 (only) mnemonics */
	"bsr",
/* 6510 mnemonics */
	"anc", "asr", "ast", "arr", "asx", "axa", "dcp", "dea",
	"dop", "ina", "isc", "lax", "phx", "phy", "plx", "ply",
	"rla", "rra", "sax", "slo", "sre", "sah", "say", "ssh",
	"sxh", "syh", "top", "oal", "kil",
	/* 65ce02 mnemonics */
	"cle", "see", "rtn", "aug",
	"tab", "tba", "taz", "tza", "tys", "tsy",
	"ldz", "stz",
	"dez", "inz", "cpz", "phz", "plz",
	"neg", "asr",
	"asw", "row", "dew", "inw", "phw",
/* 4510 mnemonics */
	"map",

/* Deco CPU16 mnemonics */
	"VBL", "u13", "u8F", "uAB", "u87", "u0B", "uA3", "u4B",
	"u3F", "uBB", "u23"
};

/* Some really short names for the EA access modes */
#define VAL EA_VALUE
#define JMP EA_ABS_PC
#define BRA EA_REL_PC
#define MRD EA_MEM_RD
#define MWR EA_MEM_WR
#define MRW EA_MEM_RDWR
#define ZRD EA_ZPG_RD
#define ZWR EA_ZPG_WR
#define ZRW EA_ZPG_RDWR

/* words */
#define MRD2 EA_MEM_RD
#define MRW2 EA_MEM_RDWR
#define ZRD2 EA_ZPG_RD
#define ZRW2 EA_ZPG_RDWR

static const UINT8 op6502[256][3] = {
	{brk,imm,VAL},{ora,idx,MRD},{ill,non,0	},{ill,non,0 },/* 00 */
	{ill,non,0	},{ora,zpg,ZRD},{asl,zpg,ZRW},{ill,non,0 },
	{php,imp,0	},{ora,imm,VAL},{asl,acc,0	},{ill,non,0 },
	{ill,non,0	},{ora,aba,MRD},{asl,aba,MRW},{ill,non,0 },
	{bpl,rel,BRA},{ora,idy,MRD},{ill,non,0	},{ill,non,0 },/* 10 */
	{ill,non,0	},{ora,zpx,ZRD},{asl,zpx,ZRW},{ill,non,0 },
	{clc,imp,0	},{ora,aby,MRD},{ill,non,0	},{ill,non,0 },
	{ill,non,0	},{ora,abx,MRD},{asl,abx,MRW},{ill,non,0 },
	{jsr,adr,JMP},{and,idx,MRD},{ill,non,0	},{ill,non,0 },/* 20 */
	{bit,zpg,ZRD},{and,zpg,ZRD},{rol,zpg,ZRW},{ill,non,0 },
	{plp,imp,0	},{and,imm,VAL},{rol,acc,0	},{ill,non,0 },
	{bit,aba,MRD},{and,aba,MRD},{rol,aba,MRW},{ill,non,0 },
	{bmi,rel,BRA},{and,idy,MRD},{ill,non,0	},{ill,non,0 },/* 30 */
	{ill,non,0	},{and,zpx,ZRD},{rol,zpx,ZRW},{ill,non,0 },
	{sec,imp,0	},{and,aby,MRD},{ill,non,0	},{ill,non,0 },
	{ill,non,0	},{and,abx,MRD},{rol,abx,MRW},{ill,non,0 },
	{rti,imp,0	},{eor,idx,MRD},{ill,non,0	},{ill,non,0 },/* 40 */
	{ill,non,0	},{eor,zpg,ZRD},{lsr,zpg,ZRW},{ill,non,0  },
	{pha,imp,0	},{eor,imm,VAL},{lsr,acc,0	},{ill,non,0 },
	{jmp,adr,JMP},{eor,aba,MRD},{lsr,aba,MRW},{ill,non,0 },
	{bvc,rel,BRA},{eor,idy,MRD},{ill,non,0	},{ill,non,0 },/* 50 */
	{ill,non,0	},{eor,zpx,ZRD},{lsr,zpx,ZRW},{ill,non,0 },
	{cli,imp,0	},{eor,aby,MRD},{ill,non,0	},{ill,non,0 },
	{ill,non,0	},{eor,abx,MRD},{lsr,abx,MRW},{ill,non,0 },
	{rts,imp,0	},{adc,idx,MRD},{ill,non,0	},{ill,non,0 },/* 60 */
	{ill,non,0	},{adc,zpg,ZRD},{ror,zpg,ZRW},{ill,non,0 },
	{pla,imp,0	},{adc,imm,VAL},{ror,acc,0	},{ill,non,0 },
	{jmp,ind,JMP},{adc,aba,MRD},{ror,aba,MRW},{ill,non,0 },
	{bvs,rel,BRA},{adc,idy,MRD},{ill,non,0	},{ill,non,0 },/* 70 */
	{ill,non,0	},{adc,zpx,ZRD},{ror,zpx,ZRW},{ill,non,0 },
	{sei,imp,0	},{adc,aby,MRD},{ill,non,0	},{ill,non,0 },
	{ill,non,0	},{adc,abx,MRD},{ror,abx,MRW},{ill,non,0 },
	{ill,non,0	},{sta,idx,MWR},{ill,non,0	},{ill,non,0 },/* 80 */
	{sty,zpg,ZWR},{sta,zpg,ZWR},{stx,zpg,ZWR},{ill,non,0 },
	{dey,imp,0	},{ill,non,0  },{txa,imp,0	},{ill,non,0 },
	{sty,aba,MWR},{sta,aba,MWR},{stx,aba,MWR},{ill,non,0 },
	{bcc,rel,BRA},{sta,idy,MWR},{ill,non,0	},{ill,non,0 },/* 90 */
	{sty,zpx,ZWR},{sta,zpx,ZWR},{stx,zpy,ZWR},{ill,non,0 },
	{tya,imp,0	},{sta,aby,MWR},{txs,imp,0	},{ill,non,0 },
	{ill,non,0	},{sta,abx,MWR},{ill,non,0	},{ill,non,0 },
	{ldy,imm,VAL},{lda,idx,MRD},{ldx,imm,VAL},{ill,non,0 },/* a0 */
	{ldy,zpg,ZRD},{lda,zpg,ZRD},{ldx,zpg,ZRD},{ill,non,0 },
	{tay,imp,0	},{lda,imm,VAL},{tax,imp,0	},{ill,non,0 },
	{ldy,aba,MRD},{lda,aba,MRD},{ldx,aba,MRD},{ill,non,0 },
	{bcs,rel,BRA},{lda,idy,MRD},{ill,non,0	},{ill,non,0 },/* b0 */
	{ldy,zpx,ZRD},{lda,zpx,ZRD},{ldx,zpy,ZRD},{ill,non,0 },
	{clv,imp,0	},{lda,aby,MRD},{tsx,imp,0	},{ill,non,0 },
	{ldy,abx,MRD},{lda,abx,MRD},{ldx,aby,MRD},{ill,non,0 },
	{cpy,imm,VAL},{cmp,idx,MRD},{ill,non,0	},{ill,non,0 },/* c0 */
	{cpy,zpg,ZRD},{cmp,zpg,ZRD},{dec,zpg,ZRW},{ill,non,0 },
	{iny,imp,0	},{cmp,imm,VAL},{dex,imp,0	},{ill,non,0 },
	{cpy,aba,MRD},{cmp,aba,MRD},{dec,aba,MRW},{ill,non,0 },
	{bne,rel,BRA},{cmp,idy,MRD},{ill,non,0	},{ill,non,0 },/* d0 */
	{ill,non,0	},{cmp,zpx,ZRD},{dec,zpx,ZRW},{ill,non,0 },
	{cld,imp,0	},{cmp,aby,MRD},{ill,non,0	},{ill,non,0 },
	{ill,non,0	},{cmp,abx,MRD},{dec,abx,MRW},{ill,non,0 },
	{cpx,imm,VAL},{sbc,idx,MRD},{ill,non,0	},{ill,non,0 },/* e0 */
	{cpx,zpg,ZRD},{sbc,zpg,ZRD},{inc,zpg,ZRW},{ill,non,0 },
	{inx,imp,0	},{sbc,imm,VAL},{nop,imp,0	},{ill,non,0 },
	{cpx,aba,MRD},{sbc,aba,MRD},{inc,aba,MRW},{ill,non,0 },
	{beq,rel,BRA},{sbc,idy,MRD},{ill,non,0	},{ill,non,0 },/* f0 */
	{ill,non,0	},{sbc,zpx,ZRD},{inc,zpx,ZRW},{ill,non,0 },
	{sed,imp,0	},{sbc,aby,MRD},{ill,non,0	},{ill,non,0 },
	{ill,non,0	},{sbc,abx,MRD},{inc,abx,MRW},{ill,non,0 }
};

static const UINT8 op65c02[256][3] = {
	{brk,imm,VAL},{ora,idx,MRD},{ill,non,0	},{ill,non,0 },/* 00 */
	{tsb,zpg,0	},{ora,zpg,ZRD},{asl,zpg,ZRW},{rmb,zpg,ZRW},
	{php,imp,0	},{ora,imm,VAL},{asl,acc,MRW},{ill,non,0 },
	{tsb,aba,MRD},{ora,aba,MRD},{asl,aba,MRW},{bbr,zpb,ZRD},
	{bpl,rel,BRA},{ora,idy,MRD},{ora,zpi,MRD},{ill,non,0 },/* 10 */
	{trb,zpg,ZRD},{ora,zpx,ZRD},{asl,zpx,ZRW},{rmb,zpg,ZRW},
	{clc,imp,0	},{ora,aby,MRD},{ina,imp,0	},{ill,non,0 },
	{trb,aba,MRD},{ora,abx,MRD},{asl,abx,MRW},{bbr,zpb,ZRD},
	{jsr,adr,0	},{and,idx,MRD},{ill,non,0	},{ill,non,0 },/* 20 */
	{bit,zpg,ZRD},{and,zpg,ZRD},{rol,zpg,ZRW},{rmb,zpg,ZRW},
	{plp,imp,0	},{and,imm,VAL},{rol,acc,0	},{ill,non,0 },
	{bit,aba,MRD},{and,aba,MRD},{rol,aba,MRW},{bbr,zpb,ZRD},
	{bmi,rel,BRA},{and,idy,MRD},{and,zpi,MRD},{ill,non,0 },/* 30 */
	{bit,zpx,ZRD},{and,zpx,ZRD},{rol,zpx,ZRW},{rmb,zpg,ZRW},
	{sec,imp,0	},{and,aby,MRD},{dea,imp,0	},{ill,non,0 },
	{bit,abx,MRD},{and,abx,MRD},{rol,abx,MRW},{bbr,zpb,ZRD},
	{rti,imp,0	},{eor,idx,MRD},{ill,non,0	},{ill,non,0 },/* 40 */
	{ill,non,0	},{eor,zpg,ZRD},{lsr,zpg,ZRW},{rmb,zpg,ZRW},
	{pha,imp,0	},{eor,imm,VAL},{lsr,acc,0	},{ill,non,0 },
	{jmp,adr,JMP},{eor,aba,MRD},{lsr,aba,MRW},{bbr,zpb,ZRD},
	{bvc,rel,BRA},{eor,idy,MRD},{eor,zpi,MRD},{ill,non,0 },/* 50 */
	{ill,non,0	},{eor,zpx,ZRD},{lsr,zpx,ZRW},{rmb,zpg,ZRW},
	{cli,imp,0	},{eor,aby,MRD},{phy,imp,0	},{ill,non,0 },
	{ill,non,0	},{eor,abx,MRD},{lsr,abx,MRW},{bbr,zpb,ZRD},
	{rts,imp,0	},{adc,idx,MRD},{ill,non,0	},{ill,non,0 },/* 60 */
	{stz,zpg,ZWR},{adc,zpg,ZRD},{ror,zpg,ZRW},{rmb,zpg,ZRW},
	{pla,imp,0	},{adc,imm,VAL},{ror,acc,0	},{ill,non,0 },
	{jmp,ind,JMP},{adc,aba,MRD},{ror,aba,MRW},{bbr,zpb,ZRD},
	{bvs,rel,BRA},{adc,idy,MRD},{adc,zpi,MRD},{ill,non,0 },/* 70 */
	{stz,zpx,ZWR},{adc,zpx,ZRD},{ror,zpx,ZRW},{rmb,zpg,ZRW},
	{sei,imp,0	},{adc,aby,MRD},{ply,imp,0	},{ill,non,0 },
	{jmp,iax,JMP},{adc,abx,MRD},{ror,abx,MRW},{bbr,zpb,ZRD},
	{bra,rel,BRA},{sta,idx,MWR},{ill,non,0	},{ill,non,0 },/* 80 */
	{sty,zpg,ZWR},{sta,zpg,ZWR},{stx,zpg,ZWR},{smb,zpg,ZRW},
	{dey,imp,0	},{bit,imm,VAL},{txa,imp,0	},{ill,non,0 },
	{sty,aba,MWR},{sta,aba,MWR},{stx,aba,MWR},{bbs,zpb,ZRD},
	{bcc,rel,BRA},{sta,idy,MWR},{sta,zpi,MWR},{ill,non,0 },/* 90 */
	{sty,zpx,ZWR},{sta,zpx,ZWR},{stx,zpy,ZWR},{smb,zpg,ZRW},
	{tya,imp,0	},{sta,aby,MWR},{txs,imp,0	},{ill,non,0 },
	{stz,aba,MWR},{sta,abx,MWR},{stz,abx,MWR},{bbs,zpb,ZRD},
	{ldy,imm,VAL},{lda,idx,MRD},{ldx,imm,VAL},{ill,non,0 },/* a0 */
	{ldy,zpg,ZRD},{lda,zpg,ZRD},{ldx,zpg,ZRD},{smb,zpg,ZRW},
	{tay,imp,0	},{lda,imm,VAL},{tax,imp,0	},{ill,non,0 },
	{ldy,aba,MRD},{lda,aba,MRD},{ldx,aba,MRD},{bbs,zpb,ZRD},
	{bcs,rel,BRA},{lda,idy,MRD},{lda,zpi,MRD},{ill,non,0 },/* b0 */
	{ldy,zpx,ZRD},{lda,zpx,ZRD},{ldx,zpy,ZRD},{smb,zpg,ZRW},
	{clv,imp,0	},{lda,aby,MRD},{tsx,imp,0	},{ill,non,0 },
	{ldy,abx,MRD},{lda,abx,MRD},{ldx,aby,MRD},{bbs,zpb,ZRD},
	{cpy,imm,VAL},{cmp,idx,MRD},{ill,non,0	},{ill,non,0 },/* c0 */
	{cpy,zpg,ZRD},{cmp,zpg,ZRD},{dec,zpg,ZRW},{smb,zpg,ZRW},
	{iny,imp,0	},{cmp,imm,VAL},{dex,imp,0	},{ill,non,0 },
	{cpy,aba,MRD},{cmp,aba,MRD},{dec,aba,MRW},{bbs,zpb,ZRD},
	{bne,rel,BRA},{cmp,idy,MRD},{cmp,zpi,MRD},{ill,non,0 },/* d0 */
	{ill,non,0	},{cmp,zpx,ZRD},{dec,zpx,ZRW},{smb,zpg,ZRW},
	{cld,imp,0	},{cmp,aby,MRD},{phx,imp,0	},{ill,non,0 },
	{ill,non,0	},{cmp,abx,MRD},{dec,abx,MRW},{bbs,zpb,ZRD},
	{cpx,imm,VAL},{sbc,idx,MRD},{ill,non,0	},{ill,non,0 },/* e0 */
	{cpx,zpg,ZRD},{sbc,zpg,ZRD},{inc,zpg,ZRW},{smb,zpg,ZRW},
	{inx,imp,0	},{sbc,imm,VAL},{nop,imp,0	},{ill,non,0 },
	{cpx,aba,MRD},{sbc,aba,MRD},{inc,aba,MRW},{bbs,zpb,ZRD},
	{beq,rel,BRA},{sbc,idy,MRD},{sbc,zpi,MRD},{ill,non,0 },/* f0 */
	{ill,non,0	},{sbc,zpx,ZRD},{inc,zpx,ZRW},{smb,zpg,ZRW},
	{sed,imp,0	},{sbc,aby,MRD},{plx,imp,0	},{ill,non,0 },
	{ill,non,0	},{sbc,abx,MRD},{inc,abx,MRW},{bbs,zpb,ZRD}
};

/* only bsr additional to 65c02 yet */
static const UINT8 op65sc02[256][3] = {
	{brk,imm,VAL},{ora,idx,MRD},{ill,non,0	},{ill,non,0 },/* 00 */
	{tsb,zpg,0	},{ora,zpg,ZRD},{asl,zpg,ZRW},{rmb,zpg,ZRW},
	{php,imp,0	},{ora,imm,VAL},{asl,acc,MRW},{ill,non,0 },
	{tsb,aba,MRD},{ora,aba,MRD},{asl,aba,MRW},{bbr,zpb,ZRD},
	{bpl,rel,BRA},{ora,idy,MRD},{ora,zpi,MRD},{ill,non,0 },/* 10 */
	{trb,zpg,ZRD},{ora,zpx,ZRD},{asl,zpx,ZRW},{rmb,zpg,ZRW},
	{clc,imp,0	},{ora,aby,MRD},{ina,imp,0	},{ill,non,0 },
	{trb,aba,MRD},{ora,abx,MRD},{asl,abx,MRW},{bbr,zpb,ZRD},
	{jsr,adr,0	},{and,idx,MRD},{ill,non,0	},{ill,non,0 },/* 20 */
	{bit,zpg,ZRD},{and,zpg,ZRD},{rol,zpg,ZRW},{rmb,zpg,ZRW},
	{plp,imp,0	},{and,imm,VAL},{rol,acc,0	},{ill,non,0 },
	{bit,aba,MRD},{and,aba,MRD},{rol,aba,MRW},{bbr,zpb,ZRD},
	{bmi,rel,BRA},{and,idy,MRD},{and,zpi,MRD},{ill,non,0 },/* 30 */
	{bit,zpx,ZRD},{and,zpx,ZRD},{rol,zpx,ZRW},{rmb,zpg,ZRW},
	{sec,imp,0	},{and,aby,MRD},{dea,imp,0	},{ill,non,0 },
	{bit,abx,MRD},{and,abx,MRD},{rol,abx,MRW},{bbr,zpb,ZRD},
	{rti,imp,0	},{eor,idx,MRD},{ill,non,0	},{ill,non,0 },/* 40 */
	{ill,non,0	},{eor,zpg,ZRD},{lsr,zpg,ZRW},{rmb,zpg,ZRW},
	{pha,imp,0	},{eor,imm,VAL},{lsr,acc,0	},{ill,non,0 },
	{jmp,adr,JMP},{eor,aba,MRD},{lsr,aba,MRW},{bbr,zpb,ZRD},
	{bvc,rel,BRA},{eor,idy,MRD},{eor,zpi,MRD},{ill,non,0 },/* 50 */
	{ill,non,0	},{eor,zpx,ZRD},{lsr,zpx,ZRW},{rmb,zpg,ZRW},
	{cli,imp,0	},{eor,aby,MRD},{phy,imp,0	},{ill,non,0 },
	{ill,non,0	},{eor,abx,MRD},{lsr,abx,MRW},{bbr,zpb,ZRD},
	{rts,imp,0	},{adc,idx,MRD},{ill,non,0	},{bsr,rw2,BRA},/* 60 */
	{stz,zpg,ZWR},{adc,zpg,ZRD},{ror,zpg,ZRW},{rmb,zpg,ZRW},
	{pla,imp,0	},{adc,imm,VAL},{ror,acc,0	},{ill,non,0 },
	{jmp,ind,JMP},{adc,aba,MRD},{ror,aba,MRW},{bbr,zpb,ZRD},
	{bvs,rel,BRA},{adc,idy,MRD},{adc,zpi,MRD},{ill,non,0 },/* 70 */
	{stz,zpx,ZWR},{adc,zpx,ZRD},{ror,zpx,ZRW},{rmb,zpg,ZRW},
	{sei,imp,0	},{adc,aby,MRD},{ply,imp,0	},{ill,non,0 },
	{jmp,iax,JMP},{adc,abx,MRD},{ror,abx,MRW},{bbr,zpb,ZRD},
	{bra,rel,BRA},{sta,idx,MWR},{ill,non,0	},{ill,non,0 },/* 80 */
	{sty,zpg,ZWR},{sta,zpg,ZWR},{stx,zpg,ZWR},{smb,zpg,ZRW},
	{dey,imp,0	},{bit,imm,VAL},{txa,imp,0	},{ill,non,0 },
	{sty,aba,MWR},{sta,aba,MWR},{stx,aba,MWR},{bbs,zpb,ZRD},
	{bcc,rel,BRA},{sta,idy,MWR},{sta,zpi,MWR},{ill,non,0 },/* 90 */
	{sty,zpx,ZWR},{sta,zpx,ZWR},{stx,zpy,ZWR},{smb,zpg,ZRW},
	{tya,imp,0	},{sta,aby,MWR},{txs,imp,0	},{ill,non,0 },
	{stz,aba,MWR},{sta,abx,MWR},{stz,abx,MWR},{bbs,zpb,ZRD},
	{ldy,imm,VAL},{lda,idx,MRD},{ldx,imm,VAL},{ill,non,0 },/* a0 */
	{ldy,zpg,ZRD},{lda,zpg,ZRD},{ldx,zpg,ZRD},{smb,zpg,ZRW},
	{tay,imp,0	},{lda,imm,VAL},{tax,imp,0	},{ill,non,0 },
	{ldy,aba,MRD},{lda,aba,MRD},{ldx,aba,MRD},{bbs,zpb,ZRD},
	{bcs,rel,BRA},{lda,idy,MRD},{lda,zpi,MRD},{ill,non,0 },/* b0 */
	{ldy,zpx,ZRD},{lda,zpx,ZRD},{ldx,zpy,ZRD},{smb,zpg,ZRW},
	{clv,imp,0	},{lda,aby,MRD},{tsx,imp,0	},{ill,non,0 },
	{ldy,abx,MRD},{lda,abx,MRD},{ldx,aby,MRD},{bbs,zpb,ZRD},
	{cpy,imm,VAL},{cmp,idx,MRD},{ill,non,0	},{ill,non,0 },/* c0 */
	{cpy,zpg,ZRD},{cmp,zpg,ZRD},{dec,zpg,ZRW},{smb,zpg,ZRW},
	{iny,imp,0	},{cmp,imm,VAL},{dex,imp,0	},{ill,non,0 },
	{cpy,aba,MRD},{cmp,aba,MRD},{dec,aba,MRW},{bbs,zpb,ZRD},
	{bne,rel,BRA},{cmp,idy,MRD},{cmp,zpi,MRD},{ill,non,0 },/* d0 */
	{ill,non,0	},{cmp,zpx,ZRD},{dec,zpx,ZRW},{smb,zpg,ZRW},
	{cld,imp,0	},{cmp,aby,MRD},{phx,imp,0	},{ill,non,0 },
	{ill,non,0	},{cmp,abx,MRD},{dec,abx,MRW},{bbs,zpb,ZRD},
	{cpx,imm,VAL},{sbc,idx,MRD},{ill,non,0	},{ill,non,0 },/* e0 */
	{cpx,zpg,ZRD},{sbc,zpg,ZRD},{inc,zpg,ZRW},{smb,zpg,ZRW},
	{inx,imp,0	},{sbc,imm,VAL},{nop,imp,0	},{ill,non,0 },
	{cpx,aba,MRD},{sbc,aba,MRD},{inc,aba,MRW},{bbs,zpb,ZRD},
	{beq,rel,BRA},{sbc,idy,MRD},{sbc,zpi,MRD},{ill,non,0 },/* f0 */
	{ill,non,0	},{sbc,zpx,ZRD},{inc,zpx,ZRW},{smb,zpg,ZRW},
	{sed,imp,0	},{sbc,aby,MRD},{plx,imp,0	},{ill,non,0 },
	{ill,non,0	},{sbc,abx,MRD},{inc,abx,MRW},{bbs,zpb,ZRD}
};

static const UINT8 op6510[256][3] = {
	{brk,imm,VAL},{ora,idx,MRD},{kil,non,0	},{slo,idx,MRW},/* 00 */
	{dop,imm,VAL},{ora,zpg,ZRD},{asl,zpg,ZRW},{slo,zpg,ZRW},
	{php,imp,0	},{ora,imm,VAL},{asl,acc,0	},{anc,imm,VAL},
	{top,iw2,VAL},{ora,aba,MRD},{asl,aba,MRW},{slo,aba,MRW},
	{bpl,rel,BRA},{ora,idy,MRD},{nop,imp,0	},{slo,idy,MRW},/* 10 */
	{dop,imm,VAL},{ora,zpx,ZRD},{asl,zpx,ZRW},{slo,zpx,ZRW},
	{clc,imp,0	},{ora,aby,MRD},{kil,non,0	},{slo,aby,MRW},
	{top,iw2,VAL},{ora,abx,MRD},{asl,abx,MRW},{slo,abx,MRW},
	{jsr,adr,JMP},{and,idx,MRD},{kil,non,0	},{rla,idx,MRW},/* 20 */
	{bit,zpg,ZRD},{and,zpg,ZRD},{rol,zpg,ZRW},{rla,zpg,ZRW},
	{plp,imp,0	},{and,imm,VAL},{rol,acc,0	},{anc,imm,VAL},
	{bit,aba,MRD},{and,aba,MRD},{rol,aba,MRW},{rla,aba,MRW},
	{bmi,rel,BRA},{and,idy,MRD},{kil,non,0	},{rla,idy,MRW},/* 30 */
	{dop,imm,VAL},{and,zpx,ZRD},{rol,zpx,ZRW},{rla,zpx,ZRW},
	{sec,imp,0	},{and,aby,MRD},{nop,imp,0	},{rla,aby,MRW},
	{top,iw2,VAL},{and,abx,MRD},{rol,abx,MRW},{rla,abx,MRW},
	{rti,imp,0	},{eor,idx,MRD},{kil,non,0	},{sre,idx,MRW},/* 40 */
	{dop,imm,VAL},{eor,zpg,ZRD},{lsr,zpg,ZRW},{sre,zpg,ZRW},
	{pha,imp,0	},{eor,imm,VAL},{lsr,acc,0	},{asr,imm,VAL},
	{jmp,adr,JMP},{eor,aba,MRD},{lsr,aba,MRW},{sre,aba,MRW},
	{bvc,rel,BRA},{eor,idy,MRD},{kil,non,0	},{sre,idy,MRW},/* 50 */
	{dop,imm,VAL},{eor,zpx,ZRD},{lsr,zpx,ZRW},{sre,zpx,ZRW},
	{cli,imp,0	},{eor,aby,MRD},{nop,imp,0	},{sre,aby,MRW},
	{top,iw2,VAL},{eor,abx,MRD},{lsr,abx,MRW},{sre,abx,MRW},
	{rts,imp,0	},{adc,idx,MRD},{kil,non,0	},{rra,idx,MRW},/* 60 */
	{dop,imm,VAL},{adc,zpg,ZRD},{ror,zpg,ZRW},{rra,zpg,ZRW},
	{pla,imp,0	},{adc,imm,VAL},{ror,acc,0	},{arr,imm,VAL},
	{jmp,ind,JMP},{adc,aba,MRD},{ror,aba,MRW},{rra,aba,MRW},
	{bvs,rel,BRA},{adc,idy,MRD},{kil,non,0	},{rra,idy,MRW},/* 70 */
	{dop,imm,VAL},{adc,zpx,ZRD},{ror,zpx,ZRW},{rra,zpx,ZRW},
	{sei,imp,0	},{adc,aby,MRD},{nop,imp,0	},{rra,aby,MRW},
	{top,iw2,VAL},{adc,abx,MRD},{ror,abx,MRW},{rra,abx,MRW},
	{dop,imm,VAL},{sta,idx,MWR},{dop,imm,VAL},{sax,idx,MWR},/* 80 */
	{sty,zpg,ZWR},{sta,zpg,ZWR},{stx,zpg,ZWR},{sax,zpg,ZWR},
	{dey,imp,0	},{dop,imm,VAL},{txa,imp,0  },{axa,imm,VAL},
	{sty,aba,MWR},{sta,aba,MWR},{stx,aba,MWR},{sax,aba,MWR},
	{bcc,rel,BRA},{sta,idy,MWR},{kil,non,0	},{say,idy,MWR},/* 90 */
	{sty,zpx,ZWR},{sta,zpx,ZWR},{stx,zpy,ZWR},{sax,zpx,ZWR},
	{tya,imp,0	},{sta,aby,MWR},{txs,imp,0	},{ssh,aby,MWR},
	{syh,abx,0	},{sta,abx,MWR},{sxh,aby,MWR},{sah,aby,MWR},
	{ldy,imm,VAL},{lda,idx,MRD},{ldx,imm,VAL},{lax,idx,MRD},/* a0 */
	{ldy,zpg,ZRD},{lda,zpg,ZRD},{ldx,zpg,ZRD},{lax,zpg,ZRD},
	{tay,imp,0	},{lda,imm,VAL},{tax,imp,0	},{oal,imm,VAL},
	{ldy,aba,MRD},{lda,aba,MRD},{ldx,aba,MRD},{lax,aba,MRD},
	{bcs,rel,BRA},{lda,idy,MRD},{kil,non,0	},{lax,idy,MRD},/* b0 */
	{ldy,zpx,ZRD},{lda,zpx,ZRD},{ldx,zpy,ZRD},{lax,zpx,ZRD},
	{clv,imp,0	},{lda,aby,MRD},{tsx,imp,0	},{ast,aby,MRD},
	{ldy,abx,MRD},{lda,abx,MRD},{ldx,aby,MRD},{lax,abx,MRD},
	{cpy,imm,VAL},{cmp,idx,MRD},{dop,imm,VAL},{dcp,idx,MRW},/* c0 */
	{cpy,zpg,ZRD},{cmp,zpg,ZRD},{dec,zpg,ZRW},{dcp,zpg,ZRW},
	{iny,imp,0	},{cmp,imm,VAL},{dex,imp,0	},{asx,imm,VAL},
	{cpy,aba,MRD},{cmp,aba,MRD},{dec,aba,MRW},{dcp,aba,MRW},
	{bne,rel,BRA},{cmp,idy,MRD},{kil,non,0	},{dcp,idy,MRW},/* d0 */
	{dop,imm,VAL},{cmp,zpx,ZRD},{dec,zpx,ZRW},{dcp,zpx,ZRW},
	{cld,imp,0	},{cmp,aby,MRD},{nop,imp,0	},{dcp,aby,MRW},
	{top,iw2,VAL},{cmp,abx,MRD},{dec,abx,MRW},{dcp,abx,MRW},
	{cpx,imm,VAL},{sbc,idx,MRD},{dop,imm,VAL},{isc,idx,MRW},/* e0 */
	{cpx,zpg,ZRD},{sbc,zpg,ZRD},{inc,zpg,ZRW},{isc,zpg,ZRW},
	{inx,imp,0	},{sbc,imm,VAL},{nop,imp,0	},{sbc,imm,VAL},
	{cpx,aba,MRD},{sbc,aba,MRD},{inc,aba,MRW},{isc,aba,MRW},
	{beq,rel,BRA},{sbc,idy,MRD},{kil,non,0	},{isc,idy,MRW},/* f0 */
	{dop,imm,VAL},{sbc,zpx,ZRD},{inc,zpx,ZRW},{isc,zpx,ZRW},
	{sed,imp,0	},{sbc,aby,MRD},{nop,imp,0	},{isc,aby,MRW},
	{top,iw2,VAL},{sbc,abx,MRD},{inc,abx,MRW},{isc,abx,MRW}
};

#if (HAS_M65CE02)
static const UINT8 op65ce02[256][3] = {
	{brk,imm,VAL},{ora,idx,MRD},{cle,imp,0	},{see,imp,0  },/* 00 */
	{tsb,zpg,0	},{ora,zpg,ZRD},{asl,zpg,ZRW},{rmb,zpg,ZRW},
	{php,imp,0	},{ora,imm,VAL},{asl,acc,MRW},{tsy,imp,0  },
	{tsb,aba,MRD},{ora,aba,MRD},{asl,aba,MRW},{bbr,zpb,ZRD},
	{bpl,rel,BRA},{ora,idy,MRD},{ora,idz,MRD},{bpl,rw2,BRA},/* 10 */
	{trb,zpg,ZRD},{ora,zpx,ZRD},{asl,zpx,ZRW},{rmb,zpg,ZRW},
	{clc,imp,0	},{ora,aby,MRD},{ina,imp,0	},{inz,imp,0  },
	{trb,aba,MRD},{ora,abx,MRD},{asl,abx,MRW},{bbr,zpb,ZRD},
	{jsr,adr,0	},{and,idx,MRD},{jsr,ind,0	},{jsr,iax,0  },/* 20 */
	{bit,zpg,ZRD},{and,zpg,ZRD},{rol,zpg,ZRW},{rmb,zpg,ZRW},
	{plp,imp,0	},{and,imm,VAL},{rol,acc,0	},{tys,imp,0  },
	{bit,aba,MRD},{and,aba,MRD},{rol,aba,MRW},{bbr,zpb,ZRD},
	{bmi,rel,BRA},{and,idz,MRD},{and,zpi,MRD},{bmi,rw2,BRA},/* 30 */
	{bit,zpx,ZRD},{and,zpx,ZRD},{rol,zpx,ZRW},{rmb,zpg,ZRW},
	{sec,imp,0	},{and,aby,MRD},{dea,imp,0	},{dez,imp,0  },
	{bit,abx,MRD},{and,abx,MRD},{rol,abx,MRW},{bbr,zpb,ZRD},
	{rti,imp,0	},{eor,idx,MRD},{neg,imp,0	},{asr2,imp,0 },/* 40 */
	{asr2,zpg,ZRW},{eor,zpg,ZRD},{lsr,zpg,ZRW},{rmb,zpg,ZRW},
	{pha,imp,0	},{eor,imm,VAL},{lsr,acc,0	},{taz,imp,0  },
	{jmp,adr,JMP},{eor,aba,MRD},{lsr,aba,MRW},{bbr,zpb,ZRD},
	{bvc,rel,BRA},{eor,idy,MRD},{eor,idz,MRD},{bvc,rw2,BRA},/* 50 */
	{asr2,zpx,ZRW},{eor,zpx,ZRD},{lsr,zpx,ZRW},{rmb,zpg,ZRW},
	{cli,imp,0	},{eor,aby,MRD},{phy,imp,0	},{tab,imp,0  },
	{aug,iw3,0	},{eor,abx,MRD},{lsr,abx,MRW},{bbr,zpb,ZRD},
	{rts,imp,0	},{adc,idx,MRD},{rtn,imm,VAL},{bsr,rw2,BRA},/* 60 */
	{stz2,zpg,ZWR},{adc,zpg,ZRD},{ror,zpg,ZRW},{rmb,zpg,ZRW},
	{pla,imp,0	},{adc,imm,VAL},{ror,acc,0	},{tza,imp,0  },
	{jmp,ind,JMP},{adc,aba,MRD},{ror,aba,MRW},{bbr,zpb,ZRD},
	{bvs,rel,BRA},{adc,idy,MRD},{adc,zpi,MRD},{bvs,rw2,BRA},/* 70 */
	{stz2,zpx,ZWR},{adc,zpx,ZRD},{ror,zpx,ZRW},{rmb,zpg,ZRW},
	{sei,imp,0	},{adc,aby,MRD},{ply,imp,0	},{tba,imp,0  },
	{jmp,iax,JMP},{adc,abx,MRD},{ror,abx,MRW},{bbr,zpb,ZRD},
	{bra,rel,BRA},{sta,idx,MWR},{sta,isy,MWR},{bra,rw2,BRA},/* 80 */
	{sty,zpg,ZWR},{sta,zpg,ZWR},{stx,zpg,ZWR},{smb,zpg,ZRW},
	{dey,imp,0	},{bit,imm,VAL},{txa,imp,0	},{sty,abx,MWR},
	{sty,aba,MWR},{sta,aba,MWR},{stx,aba,MWR},{bbs,zpb,ZRD},
	{bcc,rel,BRA},{sta,idy,MWR},{sta,inz,MWR},{bcc,rw2,BRA},/* 90 */
	{sty,zpx,ZWR},{sta,zpx,ZWR},{stx,zpy,ZWR},{smb,zpg,ZRW},
	{tya,imp,0	},{sta,aby,MWR},{txs,imp,0	},{stx,aby,MWR},
	{stz2,aba,MWR},{sta,abx,MWR},{stz2,abx,MWR},{bbs,zpb,ZRD},
	{ldy,imm,VAL},{lda,idx,MRD},{ldx,imm,VAL},{ldz,imm,VAL},/* a0 */
	{ldy,zpg,ZRD},{lda,zpg,ZRD},{ldx,zpg,ZRD},{smb,zpg,ZRW},
	{tay,imp,0	},{lda,imm,VAL},{tax,imp,0	},{ldz,aba,MRD},
	{ldy,aba,MRD},{lda,aba,MRD},{ldx,aba,MRD},{bbs,zpb,ZRD},
	{bcs,rel,BRA},{lda,idy,MRD},{lda,inz,MRD},{bcs,rw2,BRA},/* b0 */
	{ldy,zpx,ZRD},{lda,zpx,ZRD},{ldx,zpy,ZRD},{smb,zpg,ZRW},
	{clv,imp,0	},{lda,aby,MRD},{tsx,imp,0	},{ldz,abx,MRD},
	{ldy,abx,MRD},{lda,abx,MRD},{ldx,aby,MRD},{bbs,zpb,ZRD},
	{cpy,imm,VAL},{cmp,idx,MRD},{cpz,imm,VAL},{dew,zpg,ZRW2},/* c0 */
	{cpy,zpg,ZRD},{cmp,zpg,ZRD},{dec,zpg,ZRW},{smb,zpg,ZRW},
	{iny,imp,0	},{cmp,imm,VAL},{dex,imp,0	},{asw,aba,MRW2},
	{cpy,aba,MRD},{cmp,aba,MRD},{dec,aba,MRW},{bbs,zpb,ZRD},
	{bne,rel,BRA},{cmp,idy,MRD},{cmp,idz,MRD},{bne,rw2,BRA},/* d0 */
	{cpz,zpg,MRD},{cmp,zpx,ZRD},{dec,zpx,ZRW},{smb,zpg,ZRW},
	{cld,imp,0	},{cmp,aby,MRD},{phx,imp,0	},{phz,imp,0  },
	{cpz,aba,MRD},{cmp,abx,MRD},{dec,abx,MRW},{bbs,zpb,ZRD},
	{cpx,imm,VAL},{sbc,idx,MRD},{lda,isy,MRD},{inw,zpg,ZRW2},/* e0 */
	{cpx,zpg,ZRD},{sbc,zpg,ZRD},{inc,zpg,ZRW},{smb,zpg,ZRW},
	{inx,imp,0	},{sbc,imm,VAL},{nop,imp,0	},{row,aba,MRW2},
	{cpx,aba,MRD},{sbc,aba,MRD},{inc,aba,MRW},{bbs,zpb,ZRD},
	{beq,rel,BRA},{sbc,idy,MRD},{sbc,idz,MRD},{beq,rw2,BRA},/* f0 */
	{phw,iw2,VAL},{sbc,zpx,ZRD},{inc,zpx,ZRW},{smb,zpg,ZRW},
	{sed,imp,0	},{sbc,aby,MRD},{plx,imp,0	},{plz,imp,0  },
	{phw,aba,MRD2},{sbc,abx,MRD},{inc,abx,MRW},{bbs,zpb,ZRD}
};
#endif

#if (HAS_M4510)
/* only map instead of aug and 20 bit memory management*/
static const UINT8 op4510[256][3] = {
	{brk,imm,VAL},{ora,idx,MRD},{cle,imp,0	},{see,imp,0  },/* 00 */
	{tsb,zpg,0	},{ora,zpg,ZRD},{asl,zpg,ZRW},{rmb,zpg,ZRW},
	{php,imp,0	},{ora,imm,VAL},{asl,acc,MRW},{tsy,imp,0  },
	{tsb,aba,MRD},{ora,aba,MRD},{asl,aba,MRW},{bbr,zpb,ZRD},
	{bpl,rel,BRA},{ora,idy,MRD},{ora,idz,MRD},{bpl,rw2,BRA},/* 10 */
	{trb,zpg,ZRD},{ora,zpx,ZRD},{asl,zpx,ZRW},{rmb,zpg,ZRW},
	{clc,imp,0	},{ora,aby,MRD},{ina,imp,0	},{inz,imp,0  },
	{trb,aba,MRD},{ora,abx,MRD},{asl,abx,MRW},{bbr,zpb,ZRD},
	{jsr,adr,0	},{and,idx,MRD},{jsr,ind,0	},{jsr,iax,0  },/* 20 */
	{bit,zpg,ZRD},{and,zpg,ZRD},{rol,zpg,ZRW},{rmb,zpg,ZRW},
	{plp,imp,0	},{and,imm,VAL},{rol,acc,0	},{tys,imp,0  },
	{bit,aba,MRD},{and,aba,MRD},{rol,aba,MRW},{bbr,zpb,ZRD},
	{bmi,rel,BRA},{and,idz,MRD},{and,zpi,MRD},{bmi,rw2,BRA},/* 30 */
	{bit,zpx,ZRD},{and,zpx,ZRD},{rol,zpx,ZRW},{rmb,zpg,ZRW},
	{sec,imp,0	},{and,aby,MRD},{dea,imp,0	},{dez,imp,0  },
	{bit,abx,MRD},{and,abx,MRD},{rol,abx,MRW},{bbr,zpb,ZRD},
	{rti,imp,0	},{eor,idx,MRD},{neg,imp,0	},{asr2,imp,0 },/* 40 */
	{asr2,zpg,ZRW},{eor,zpg,ZRD},{lsr,zpg,ZRW},{rmb,zpg,ZRW},
	{pha,imp,0	},{eor,imm,VAL},{lsr,acc,0	},{taz,imp,0  },
	{jmp,adr,JMP},{eor,aba,MRD},{lsr,aba,MRW},{bbr,zpb,ZRD},
	{bvc,rel,BRA},{eor,idy,MRD},{eor,idz,MRD},{bvc,rw2,BRA},/* 50 */
	{asr2,zpx,ZRW},{eor,zpx,ZRD},{lsr,zpx,ZRW},{rmb,zpg,ZRW},
	{cli,imp,0	},{eor,aby,MRD},{phy,imp,0	},{tab,imp,0  },
	{map,imp,0	},{eor,abx,MRD},{lsr,abx,MRW},{bbr,zpb,ZRD},
	{rts,imp,0	},{adc,idx,MRD},{rtn,imm,VAL},{bsr,rw2,BRA},/* 60 */
	{stz2,zpg,ZWR},{adc,zpg,ZRD},{ror,zpg,ZRW},{rmb,zpg,ZRW},
	{pla,imp,0	},{adc,imm,VAL},{ror,acc,0	},{tza,imp,0  },
	{jmp,ind,JMP},{adc,aba,MRD},{ror,aba,MRW},{bbr,zpb,ZRD},
	{bvs,rel,BRA},{adc,idy,MRD},{adc,zpi,MRD},{bvs,rw2,BRA},/* 70 */
	{stz2,zpx,ZWR},{adc,zpx,ZRD},{ror,zpx,ZRW},{rmb,zpg,ZRW},
	{sei,imp,0	},{adc,aby,MRD},{ply,imp,0	},{tba,imp,0  },
	{jmp,iax,JMP},{adc,abx,MRD},{ror,abx,MRW},{bbr,zpb,ZRD},
	{bra,rel,BRA},{sta,idx,MWR},{sta,isy,MWR},{bra,rw2,BRA},/* 80 */
	{sty,zpg,ZWR},{sta,zpg,ZWR},{stx,zpg,ZWR},{smb,zpg,ZRW},
	{dey,imp,0	},{bit,imm,VAL},{txa,imp,0	},{sty,abx,MWR},
	{sty,aba,MWR},{sta,aba,MWR},{stx,aba,MWR},{bbs,zpb,ZRD},
	{bcc,rel,BRA},{sta,idy,MWR},{sta,inz,MWR},{bcc,rw2,BRA},/* 90 */
	{sty,zpx,ZWR},{sta,zpx,ZWR},{stx,zpy,ZWR},{smb,zpg,ZRW},
	{tya,imp,0	},{sta,aby,MWR},{txs,imp,0	},{stx,aby,MWR},
	{stz2,aba,MWR},{sta,abx,MWR},{stz2,abx,MWR},{bbs,zpb,ZRD},
	{ldy,imm,VAL},{lda,idx,MRD},{ldx,imm,VAL},{ldz,imm,VAL},/* a0 */
	{ldy,zpg,ZRD},{lda,zpg,ZRD},{ldx,zpg,ZRD},{smb,zpg,ZRW},
	{tay,imp,0	},{lda,imm,VAL},{tax,imp,0	},{ldz,aba,MRD},
	{ldy,aba,MRD},{lda,aba,MRD},{ldx,aba,MRD},{bbs,zpb,ZRD},
	{bcs,rel,BRA},{lda,idy,MRD},{lda,inz,MRD},{bcs,rw2,BRA},/* b0 */
	{ldy,zpx,ZRD},{lda,zpx,ZRD},{ldx,zpy,ZRD},{smb,zpg,ZRW},
	{clv,imp,0	},{lda,aby,MRD},{tsx,imp,0	},{ldz,abx,MRD},
	{ldy,abx,MRD},{lda,abx,MRD},{ldx,aby,MRD},{bbs,zpb,ZRD},
	{cpy,imm,VAL},{cmp,idx,MRD},{cpz,imm,VAL},{dew,zpg,ZRW2},/* c0 */
	{cpy,zpg,ZRD},{cmp,zpg,ZRD},{dec,zpg,ZRW},{smb,zpg,ZRW},
	{iny,imp,0	},{cmp,imm,VAL},{dex,imp,0	},{asw,aba,MRW2},
	{cpy,aba,MRD},{cmp,aba,MRD},{dec,aba,MRW},{bbs,zpb,ZRD},
	{bne,rel,BRA},{cmp,idy,MRD},{cmp,idz,MRD},{bne,rw2,BRA},/* d0 */
	{cpz,zpg,MRD},{cmp,zpx,ZRD},{dec,zpx,ZRW},{smb,zpg,ZRW},
	{cld,imp,0	},{cmp,aby,MRD},{phx,imp,0	},{phz,imp,0  },
	{cpz,aba,MRD},{cmp,abx,MRD},{dec,abx,MRW},{bbs,zpb,ZRD},
	{cpx,imm,VAL},{sbc,idx,MRD},{lda,isy,MRD},{inw,zpg,ZRW2},/* e0 */
	{cpx,zpg,ZRD},{sbc,zpg,ZRD},{inc,zpg,ZRW},{smb,zpg,ZRW},
	{inx,imp,0	},{sbc,imm,VAL},{nop,imp,0	},{row,aba,MRW2},
	{cpx,aba,MRD},{sbc,aba,MRD},{inc,aba,MRW},{bbs,zpb,ZRD},
	{beq,rel,BRA},{sbc,idy,MRD},{sbc,idz,MRD},{beq,rw2,BRA},/* f0 */
	{phw,iw2,VAL},{sbc,zpx,ZRD},{inc,zpx,ZRW},{smb,zpg,ZRW},
	{sed,imp,0	},{sbc,aby,MRD},{plx,imp,0	},{plz,imp,0  },
	{phw,aba,MRD2},{sbc,abx,MRD},{inc,abx,MRW},{bbs,zpb,ZRD}
};
#endif

#if (HAS_DECO16)
static const UINT8 opdeco16[256][3] =
{
	{brk,imp,0  },{ora,idx,MRD},{ill,non,0  },{ill,non,0	},/* 00 */
	{ill,non,0  },{ora,zpg,ZRD},{asl,zpg,ZRW},{ill,non,0	},
	{php,imp,0  },{ora,imm,VAL},{asl,acc,0  },{u0B,zpg,0	},
	{ill,non,0  },{ora,aba,MRD},{asl,aba,MRW},{ill,non,0	},
	{bpl,rel,BRA},{ora,idy,MRD},{ill,non,0  },{u13,zpg,0	},/* 10 */
	{ill,non,0  },{ora,zpx,ZRD},{asl,zpx,ZRW},{ill,non,0	},
	{clc,imp,0  },{ora,aby,MRD},{ill,non,0  },{ill,non,0	},
	{ill,non,0  },{ora,abx,MRD},{asl,abx,MRW},{ill,non,0	},
	{jsr,adr,JMP},{and,idx,MRD},{ill,non,0  },{u23,zpg,0	},/* 20 */
	{bit,zpg,ZRD},{and,zpg,ZRD},{rol,zpg,ZRW},{ill,non,0	},
	{plp,imp,0  },{and,imm,VAL},{rol,acc,0  },{ill,non,0	},
	{bit,aba,MRD},{and,aba,MRD},{rol,aba,MRW},{ill,non,0	},
	{bmi,rel,BRA},{and,idy,MRD},{ill,non,0  },{ill,non,0	},/* 30 */
	{ill,non,0  },{and,zpx,ZRD},{rol,zpx,ZRW},{ill,non,0	},
	{sec,imp,0  },{and,aby,MRD},{ill,non,0  },{ill,non,0	},
	{ill,non,0  },{and,abx,MRD},{rol,abx,MRW},{u3F,zpg,0	},
   {rti,imp,0  },{eor,idx,MRD},{ill,non,0  },{ill,non,0	},/* 40 */
   {ill,non,0  },{eor,zpg,ZRD},{lsr,zpg,ZRW},{ill,non,0  },
   {pha,imp,0  },{eor,imm,VAL},{lsr,acc,0  },{u4B,zpg,0	},
   {jmp,adr,JMP},{eor,aba,MRD},{lsr,aba,MRW},{ill,non,0	},
   {bvc,rel,BRA},{eor,idy,MRD},{ill,non,0  },{ill,non,0	},/* 50 */
   {ill,non,0  },{eor,zpx,ZRD},{lsr,zpx,ZRW},{ill,non,0	},
   {cli,imp,0  },{eor,aby,MRD},{ill,non,0  },{ill,non,0	},
   {ill,non,0  },{eor,abx,MRD},{lsr,abx,MRW},{ill,non,0	},
   {rts,imp,0  },{adc,idx,MRD},{ill,non,0  },{ill,non,0	},/* 60 */
   {ill,non,0  },{adc,zpg,ZRD},{ror,zpg,ZRW},{vbl,zpg,0	},  		/* MISH*/
   {pla,imp,0  },{adc,imm,VAL},{ror,acc,0  },{ill,non,0	},
   {jmp,ind,JMP},{adc,aba,MRD},{ror,aba,MRW},{ill,non,0	},
   {bvs,rel,BRA},{adc,idy,MRD},{ill,non,0  },{ill,non,0	},/* 70 */
   {ill,non,0  },{adc,zpx,ZRD},{ror,zpx,ZRW},{ill,non,0	},
   {sei,imp,0  },{adc,aby,MRD},{ill,non,0  },{ill,non,0	},
   {ill,non,0  },{adc,abx,MRD},{ror,abx,MRW},{ill,non,0	},
   {ill,non,0  },{sta,idx,MWR},{ill,non,0  },{ill,non,0	},/* 80 */
   {sty,zpg,ZWR},{sta,zpg,ZWR},{stx,zpg,ZWR},{u87,zpg,0	},
   {dey,imp,0  },{ill,non,0	},{txa,imp,0  },{ill,non,0	},
   {sty,aba,MWR},{sta,aba,MWR},{stx,aba,MWR},{u8F,zpg,0	},
   {bcc,rel,BRA},{sta,idy,MWR},{ill,non,0  },{ill,non,0	},/* 90 */
   {sty,zpx,ZWR},{sta,zpx,ZWR},{stx,zpy,ZWR},{ill,non,0	},
   {tya,imp,0  },{sta,aby,MWR},{txs,imp,0  },{ill,non,0	},
   {ill,non,0  },{sta,abx,MWR},{ill,non,0  },{ill,non,0	},
   {ldy,imm,VAL},{lda,idx,MRD},{ldx,imm,VAL},{uA3,zpg,0	},/* a0 */
   {ldy,zpg,ZRD},{lda,zpg,ZRD},{ldx,zpg,ZRD},{ill,non,0	},
   {tay,imp,0  },{lda,imm,VAL},{tax,imp,0  },{uAB,zpg,0	},
   {ldy,aba,MRD},{lda,aba,MRD},{ldx,aba,MRD},{ill,non,0	},
   {bcs,rel,BRA},{lda,idy,MRD},{ill,non,0  },{ill,non,0	},/* b0 */
   {ldy,zpx,ZRD},{lda,zpx,ZRD},{ldx,zpy,ZRD},{ill,non,0	},
   {clv,imp,0  },{lda,aby,MRD},{tsx,imp,0  },{uBB,zpg,0	},
   {ldy,abx,MRD},{lda,abx,MRD},{ldx,aby,MRD},{ill,non,0	},
   {cpy,imm,VAL},{cmp,idx,MRD},{ill,non,0  },{ill,non,0	},/* c0 */
   {cpy,zpg,ZRD},{cmp,zpg,ZRD},{dec,zpg,ZRW},{ill,non,0	},
   {iny,imp,0  },{cmp,imm,VAL},{dex,imp,0  },{ill,non,0 },
   {cpy,aba,MRD},{cmp,aba,MRD},{dec,aba,MRW},{ill,non,0	},
   {bne,rel,BRA},{cmp,idy,MRD},{ill,non,0  },{ill,non,0	},/* d0 */
   {ill,non,0  },{cmp,zpx,ZRD},{dec,zpx,ZRW},{ill,non,0	},
   {cld,imp,0  },{cmp,aby,MRD},{ill,non,0  },{ill,non,0	},
   {ill,non,0  },{cmp,abx,MRD},{dec,abx,MRW},{ill,non,0	},
   {cpx,imm,VAL},{sbc,idx,MRD},{ill,non,0  },{ill,non,0	},/* e0 */
   {cpx,zpg,ZRD},{sbc,zpg,ZRD},{inc,zpg,ZRW},{ill,non,0	},
   {inx,imp,0  },{sbc,imm,VAL},{nop,imp,0  },{ill,non,0	},
   {cpx,aba,MRD},{sbc,aba,MRD},{inc,aba,MRW},{ill,non,0	},
   {beq,rel,BRA},{sbc,idy,MRD},{ill,non,0  },{ill,non,0	},/* f0 */
   {ill,non,0  },{sbc,zpx,ZRD},{inc,zpx,ZRW},{ill,non,0	},
   {sed,imp,0  },{sbc,aby,MRD},{ill,non,0  },{ill,non,0	},
   {ill,non,0  },{sbc,abx,MRD},{inc,abx,MRW},{ill,non,0	}
 };
#endif

/*****************************************************************************
 * Disassemble a single opcode starting at pc
 *****************************************************************************/
unsigned Dasm6502(char *buffer, unsigned pc)
{
	char *dst = buffer;
	const char *symbol;
	INT8 offset;
	INT16 offset16;
	unsigned PC = pc;
	UINT16 addr, ea;
	UINT8 op, opc, arg, access, value;

	op = OPCODE(pc++);

	switch ( m6502_get_reg(M6502_SUBTYPE) )
	{
#if (HAS_M65C02)
		case SUBTYPE_65C02:
			opc = op65c02[op][0];
			arg = op65c02[op][1];
			access = op65c02[op][2];
			break;
#endif
#if (HAS_M65SC02)
		case SUBTYPE_65SC02:
			opc = op65sc02[op][0];
			arg = op65sc02[op][1];
			access = op65sc02[op][2];
			break;
#endif
#if (HAS_M6510)
		case SUBTYPE_6510:
			opc = op6510[op][0];
			arg = op6510[op][1];
			access = op6510[op][2];
			break;
#endif
#if (HAS_DECO16)
		case SUBTYPE_DECO16:
			opc = opdeco16[op][0];
			arg = opdeco16[op][1];
			access = opdeco16[op][2];
			break;
#endif
		default:
			opc = op6502[op][0];
			arg = op6502[op][1];
			access = op6502[op][2];
			break;
	}
	dst += sprintf(dst, "%-5s", token[opc]);
	if( opc == bbr || opc == bbs || opc == rmb || opc == smb )
		dst += sprintf(dst, "%d,", (op >> 3) & 7);

	switch(arg)
	{
	case imp:
		break;

	case acc:
		dst += sprintf(dst,"a");
		break;

	case rel:
		offset = (INT8)ARGBYTE(pc++);
		symbol = set_ea_info( 0, pc, offset, access );
		dst += sprintf(dst,"%s", symbol);
		break;

	case rw2:
		offset16 = ARGWORD(pc)-1;
		pc += 2;
		symbol = set_ea_info( 0, pc, offset16, access );
		dst += sprintf(dst,"%s", symbol);
		break;

	case imm:
		value = ARGBYTE(pc++);
		symbol = set_ea_info( 0, value, EA_UINT8, access );
		dst += sprintf(dst,"#%s", symbol);
		break;

	case zpg:
		addr = ARGBYTE(pc++);
		symbol = set_ea_info( 0, addr, EA_UINT8, access );
		dst += sprintf(dst,"$%02X", addr);
		break;

	case zpx:
		addr = ARGBYTE(pc++);
		ea = (addr + m6502_get_reg(M6502_X)) & 0xff;
		symbol = set_ea_info( 0, ea, EA_UINT8, access );
		dst += sprintf(dst,"$%02X,x", addr);
		break;

	case zpy:
		addr = ARGBYTE(pc++);
		ea = (addr + m6502_get_reg(M6502_Y)) & 0xff;
		symbol = set_ea_info( 0, ea, EA_UINT8, access );
		dst += sprintf(dst,"$%02X,y", addr);
		break;

	case idx:
		addr = ARGBYTE(pc++);
		ea = (addr + m6502_get_reg(M6502_X)) & 0xff;
		ea = RDMEM(ea) + (RDMEM((ea+1) & 0xff) << 8);
		symbol = set_ea_info( 0, ea, EA_UINT16, access );
		dst += sprintf(dst,"($%02X,x)", addr);
		break;

	case idy:
		addr = ARGBYTE(pc++);
		ea = (RDMEM(addr) + (RDMEM((addr+1) & 0xff) << 8) + m6502_get_reg(M6502_Y)) & 0xffff;
		symbol = set_ea_info( 0, ea, EA_UINT16, access );
		dst += sprintf(dst,"($%02X),y", addr);
		break;

	case zpi:
		addr = ARGBYTE(pc++);
		ea = RDMEM(addr) + (RDMEM((addr+1) & 0xff) << 8);
		symbol = set_ea_info( 0, ea, EA_UINT16, access );
		dst += sprintf(dst,"($%02X)", addr);
		break;

	case zpb:
		addr = ARGBYTE(pc++);
		symbol = set_ea_info( 0, addr, EA_UINT8, access );
		dst += sprintf(dst,"$%02X", addr);
		offset = (INT8)ARGBYTE(pc++);
		symbol = set_ea_info( 1, pc, offset, BRA );
		dst += sprintf(dst,",%s", symbol);
		break;

	case adr:
		addr = ARGWORD(pc);
		pc += 2;
		symbol = set_ea_info( 0, addr, EA_UINT16, access );
		dst += sprintf(dst,"%s", symbol);
		break;

	case aba:
		addr = ARGWORD(pc);
		pc += 2;
		symbol = set_ea_info( 0, addr, EA_UINT16, access );
		dst += sprintf(dst,"%s", symbol);
		break;

	case abx:
		addr = ARGWORD(pc);
		pc += 2;
		ea = (addr + m6502_get_reg(M6502_X)) & 0xffff;
		symbol = set_ea_info( 0, ea, EA_UINT16, access );
		dst += sprintf(dst,"$%04X,x", addr);
		break;

	case aby:
		addr = ARGWORD(pc);
		pc += 2;
		ea = (addr + m6502_get_reg(M6502_Y)) & 0xffff;
		symbol = set_ea_info( 0, ea, EA_UINT16, access );
		dst += sprintf(dst,"$%04X,y", addr);
		break;

	case ind:
		addr = ARGWORD(pc);
		pc += 2;
		ea = ARGWORD(addr);
		symbol = set_ea_info( 0, ea, EA_UINT16, access );
		dst += sprintf(dst,"($%04X)", addr);
		break;

	case iax:
		addr = ARGWORD(pc);
		pc += 2;
		ea = (ARGWORD(addr) + m6502_get_reg(M6502_X)) & 0xffff;
		symbol = set_ea_info( 0, ea, EA_UINT16, access );
		dst += sprintf(dst,"($%04X),X", addr);
		break;

	default:
		dst += sprintf(dst,"$%02X", op);
	}
	return pc - PC;
}


#if (HAS_M65CE02 || HAS_M6509 || HAS_M6510 || HAS_M4510)

#if (HAS_M65CE02 || HAS_M6510)
static int m6502_get_argword(int addr)
{
	return cpu_readop_arg(addr)+(cpu_readop_arg((addr+1)&0xffff) << 8);
}
#endif

#if (HAS_M6509 || HAS_M4510)
static int m6509_get_argword(int addr)
{
	if ((addr&0xffff)==0xffff)
		return cpu_readop_arg(addr)+(cpu_readop_arg(addr&~0xffff) << 8);
	else
		return cpu_readop_arg(addr)+(cpu_readop_arg(addr+1) << 8);
}
#endif

typedef struct {
	const UINT8 *opcode;
	unsigned(*get_reg)(int regnum);
	mem_read_handler readmem;
	int(*argword)(int addr);
} CPU_TYPE;

#if 0
static CPU_TYPE type_m6502 = {
	(const UINT8*)op6502, m6502_get_reg, cpu_readmem16, m6502_get_argword
};
#endif
#ifdef HAS_M6510
static CPU_TYPE type_m6510 = {
	(const UINT8*)op6510, m6502_get_reg, cpu_readmem16, m6502_get_argword
};
#endif
#if (HAS_M6509)
static CPU_TYPE type_m6509 = {
	(const UINT8*)op6510, m6509_get_reg, cpu_readmem20, m6509_get_argword
};
#endif
#if 0
static CPU_TYPE type_m65c02 = {
	(const UINT8*)op65c02, m6502_get_reg, cpu_readmem16, m6502_get_argword
};
static CPU_TYPE type_m65sc02 = {
	(const UINT8*)op65sc02, m6502_get_reg, cpu_readmem16, m6502_get_argword
};
#endif
#if (HAS_M65CE02)
static CPU_TYPE type_m65ce02 = {
	(const UINT8*)op65ce02, m65ce02_get_reg, cpu_readmem16, m6502_get_argword
};
#endif
#if (HAS_M4510)
static READ_HANDLER(m4510_readmem)
{
	return cpu_readmem20( m4510_get_reg(M4510_MEM0+(offset>>13))+offset );
}

static CPU_TYPE type_m4510 = {
	(const UINT8*)op4510, m4510_get_reg, m4510_readmem, m6509_get_argword
};
#endif

/*****************************************************************************
 * Disassemble a single opcode starting at pc
 *****************************************************************************/
unsigned int Dasm6502Helper(CPU_TYPE *this, char *buffer, unsigned pc)
{
	char *dst = buffer;
	const char *symbol;
	INT8 offset;
	INT16 offset16;
	unsigned PC = pc;
	UINT16 addr, ea;
	UINT8 op, opc, arg, access, value;

	op = OPCODE(pc++);

	opc = this->opcode[op*3];
	arg = this->opcode[op*3+1];
	access = this->opcode[op*3+2];

	dst += sprintf(dst, "%-5s", token[opc]);
	if( opc == bbr || opc == bbs || opc == rmb || opc == smb )
		dst += sprintf(dst, "%d,", (op >> 3) & 7);

	switch(arg)
	{
	case imp:
		break;

	case acc:
		dst += sprintf(dst,"a");
		break;

	case rel:
		offset = (INT8)ARGBYTE(pc++);
		symbol = set_ea_info( 0, pc, offset, access );
#if 0
		dst += sprintf(dst,"%s", symbol);
#else
		dst += sprintf(dst,"$%04X", (pc+offset)&0xffff);
#endif
		break;

	case rw2:
		offset16 = this->argword(pc)-1;
		pc += 2;
		symbol = set_ea_info( 0, pc, offset16, access );
#if 0
		dst += sprintf(dst,"%s", symbol);
#else
		dst += sprintf(dst,"$%04X", (pc+offset16)&0xffff );
#endif
		break;

	case imm:
		value = ARGBYTE(pc++);
		symbol = set_ea_info( 0, value, EA_UINT8, access );
		dst += sprintf(dst,"#%s", symbol);
		break;

	case iw2:
		addr = ARGWORD(pc);
		pc += 2;
		symbol = set_ea_info( 0, addr, EA_UINT16, access );
		dst += sprintf(dst,"#%s", symbol);
		break;
	case iw3:
		addr = ARGWORD(pc);
		pc += 2;
		addr |= ARGBYTE(pc++)<<16;
/*		symbol = set_ea_info( 0, addr, EA_UINT16, access );*/
		dst += sprintf(dst,"#%.6x", addr);
		break;

	case zpg:
		addr = ARGBYTE(pc++);
		symbol = set_ea_info( 0, addr, EA_UINT8, access );
		dst += sprintf(dst,"$%02X", addr);
		break;

	case zpx:
		addr = ARGBYTE(pc++);
		ea = (addr + this->get_reg(M6502_X)) & 0xff;
		symbol = set_ea_info( 0, ea, EA_UINT8, access );
		dst += sprintf(dst,"$%02X,x", addr);
		break;

	case zpy:
		addr = ARGBYTE(pc++);
		ea = (addr + this->get_reg(M6502_Y)) & 0xff;
		symbol = set_ea_info( 0, ea, EA_UINT8, access );
		dst += sprintf(dst,"$%02X,y", addr);
		break;

	case idx:
		addr = ARGBYTE(pc++);
		ea = (addr + this->get_reg(M6502_X)) & 0xff;
		ea = this->readmem(ea) + (this->readmem((ea+1) & 0xff) << 8);
		symbol = set_ea_info( 0, ea, EA_UINT16, access );
		dst += sprintf(dst,"($%02X,x)", addr);
		break;

	case idy:
		addr = ARGBYTE(pc++);
		ea = (this->readmem(addr) + (this->readmem((addr+1) & 0xff) << 8)
			  + this->get_reg(M6502_Y)) & 0xffff;
		symbol = set_ea_info( 0, ea, EA_UINT16, access );
		dst += sprintf(dst,"($%02X),y", addr);
		break;

	case idz:
		addr = ARGBYTE(pc++);
		ea = (this->readmem(addr) + (this->readmem((addr+1) & 0xff) << 8)
			  + this->get_reg(M6502_Y)) & 0xffff;
		symbol = set_ea_info( 0, ea, EA_UINT16, access );
		dst += sprintf(dst,"($%02X),z", addr);
		break;

	case isy:
		op = ARGBYTE(pc++);
		addr = op+this->get_reg(M6502_S);
		ea = (this->readmem(addr)+(this->readmem(addr+1) << 8)+
			   this->get_reg(M6502_Y)) & 0xffff;
		symbol = set_ea_info( 0, ea, EA_UINT16, access );
		dst += sprintf(dst,"(s,$%02X),y", addr);
		break;

	case zpi:
		addr = ARGBYTE(pc++);
		ea = this->readmem(addr) + (this->readmem((addr+1) & 0xff) << 8);
		symbol = set_ea_info( 0, ea, EA_UINT16, access );
		dst += sprintf(dst,"($%02X)", addr);
		break;

	case zpb:
		addr = ARGBYTE(pc++);
		symbol = set_ea_info( 0, addr, EA_UINT8, access );
		dst += sprintf(dst,"$%02X", addr);
		offset = (INT8)ARGBYTE(pc++);
		symbol = set_ea_info( 1, pc, offset, BRA );
		dst += sprintf(dst,",%s", symbol);
		break;

	case adr:
		addr = this->argword(pc);
		pc += 2;
		symbol = set_ea_info( 0, addr, EA_UINT16, access );
#if 0
		dst += sprintf(dst,"%s", symbol);
#else
		dst += sprintf(dst, "$%04X", addr);
#endif
		break;

	case aba:
		addr = this->argword(pc);
		pc += 2;
		symbol = set_ea_info( 0, addr, EA_UINT16, access );
#if 0
		dst += sprintf(dst,"%s", symbol);
#else
		dst += sprintf(dst, "$%04X", addr);
#endif
		break;

	case abx:
		addr = this->argword(pc);
		pc += 2;
		ea = (addr + this->get_reg(M6502_X)) & 0xffff;
		symbol = set_ea_info( 0, ea, EA_UINT16, access );
		dst += sprintf(dst,"$%04X,x", addr);
		break;

	case aby:
		addr = this->argword(pc);
		pc += 2;
		ea = (addr + this->get_reg(M6502_Y)) & 0xffff;
		symbol = set_ea_info( 0, ea, EA_UINT16, access );
		dst += sprintf(dst,"$%04X,y", addr);
		break;

	case ind:
		addr = this->argword(pc);
		pc += 2;
		ea = this->argword(addr);
		symbol = set_ea_info( 0, ea, EA_UINT16, access );
		dst += sprintf(dst,"($%04X)", addr);
		break;

	case iax:
		addr = this->argword(pc);
		pc += 2;
		ea = (this->argword(addr) + this->get_reg(M6502_X)) & 0xffff;
		symbol = set_ea_info( 0, ea, EA_UINT16, access );
		dst += sprintf(dst,"($%04X),X", addr);
		break;

	default:
		dst += sprintf(dst,"$%02X", op);
	}
	return pc - PC;
}
#endif

#if (HAS_M65CE02)
/*****************************************************************************
 * Disassemble a single opcode starting at pc
 *****************************************************************************/
unsigned int Dasm65ce02(char *buffer, unsigned pc)
{
	return Dasm6502Helper(&type_m65ce02, buffer, pc);
}
#endif

#if (HAS_M6509)
/*****************************************************************************
 * Disassemble a single opcode starting at pc
 *****************************************************************************/
unsigned int Dasm6509(char *buffer, unsigned pc)
{
	return Dasm6502Helper(&type_m6509, buffer, pc);
}
#endif

#if (HAS_M6510)
/*****************************************************************************
 * Disassemble a single opcode starting at pc
 *****************************************************************************/
unsigned int Dasm6510(char *buffer, unsigned pc)
{
	return Dasm6502Helper(&type_m6510, buffer, pc);
}
#endif

#if (HAS_M4510)
/*****************************************************************************
 * Disassemble a single opcode starting at pc
 *****************************************************************************/
unsigned int Dasm4510(char *buffer, unsigned pc)
{
	return Dasm6502Helper(&type_m4510, buffer, pc);
}
#endif

#endif	/* MAME_DEBUG */
