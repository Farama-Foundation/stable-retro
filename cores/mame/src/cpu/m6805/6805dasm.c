/*
 *   A quick-hack 68(7)05 disassembler
 *
 *   Note: this is not the good and proper way to disassemble anything, but it works
 *
 *   I'm afraid to put my name on it, but I feel obligated:
 *   This code written by Aaron Giles (agiles@sirius.com) for the MAME project
 *
 */

#include <string.h>

#ifdef MAME_DEBUG

#include <stdio.h>
#include "cpuintrf.h"
#include "mamedbg.h"
#include "m6805.h"

enum addr_mode {
	_imp=0, 	/* implicit */
	_btr,		/* bit test and relative */
	_bit,		/* bit set/clear */
	_rel,		/* relative */
	_imm,		/* immediate */
	_dir,		/* direct address */
	_ext,		/* extended address */
	_idx,		/* indexed */
	_ix1,		/* indexed + byte offset */
	_ix2		/* indexed + word offset */
};

enum op_names {
	adca=0, adda,	anda,	asl,	asla,	aslx,	asr,	asra,
	asrx,	bcc,	bclr,	bcs,	beq,	bhcc,	bhcs,	bhi,
	bih,	bil,	bita,	bls,	bmc,	bmi,	bms,	bne,
	bpl,	bra,	brclr,	brn,	brset,	bset,	bsr,	clc,
	cli,	clr,	clra,	clrx,	cmpa,	com,	coma,	comx,
	cpx,	dec,	deca,	decx,	eora,	ill,	inc,	inca,
	incx,	jmp,	jsr,	lda,	ldx,	lsr,	lsra,	lsrx,
	neg,	nega,	negx,	nop,	ora,	rol,	rola,	rolx,
	ror,	rora,	rorx,	rsp,	rti,	rts,	sbca,	sec,
	sei,	sta,	stx,	suba,	swi,	tax,	tst,	tsta,
	tstx,	txa
};

static const char *op_name_str[] = {
	"adca", "adda", "anda", "asl",  "asla", "aslx", "asr",  "asra",
	"asrx", "bcc",  "bclr", "bcs",  "beq",  "bhcc", "bhcs", "bhi",
	"bih",  "bil",  "bita", "bls",  "bmc",  "bmi",  "bms",  "bne",
	"bpl",  "bra",  "brclr","brn",  "brset","bset", "bsr",  "clc",
	"cli",  "clr",  "clra", "clrx", "cmpa", "com",  "coma", "comx",
	"cpx",  "dec",  "deca", "decx", "eora", "*ill", "inc",  "inca",
	"incx", "jmp",  "jsr",  "lda",  "ldx",  "lsr",  "lsra", "lsrx",
	"neg",  "nega", "negx", "nop",  "ora",  "rol",  "rola", "rolx",
	"ror",  "rora", "rorx", "rsp",  "rti",  "rts",  "sbca", "sec",
	"sei",  "sta",  "stx",  "suba", "swi",  "tax",  "tst",  "tsta",
	"tstx", "txa"
};

#define _0		0,0
#define _bra	0,EA_REL_PC
#define _jmp	0,EA_ABS_PC
#define _zrd	EA_UINT8,EA_ZPG_RD
#define _zwr	EA_UINT8,EA_ZPG_WR
#define _zrw	EA_UINT8,EA_ZPG_RDWR
#define _mrd	EA_UINT8,EA_MEM_RD
#define _mwr	EA_UINT8,EA_MEM_WR
#define _mrw	EA_UINT8,EA_MEM_RDWR

const unsigned char disasm[0x100][4] = {
	{brset,_btr,_zrd},{brclr,_btr,_zrd},{brset,_btr,_zrd},{brclr,_btr,_zrd},/* 00 */
	{brset,_btr,_zrd},{brclr,_btr,_zrd},{brset,_btr,_zrd},{brclr,_btr,_zrd},
	{brset,_btr,_zrd},{brclr,_btr,_zrd},{brset,_btr,_zrd},{brclr,_btr,_zrd},
	{brset,_btr,_zrd},{brclr,_btr,_zrd},{brset,_btr,_zrd},{brclr,_btr,_zrd},
	{bset, _bit,_zwr},{bclr, _bit,_zwr},{bset, _bit,_zwr},{bclr, _bit,_zwr},/* 10 */
	{bset, _bit,_zwr},{bclr, _bit,_zwr},{bset, _bit,_zwr},{bclr, _bit,_zwr},
	{bset, _bit,_zwr},{bclr, _bit,_zwr},{bset, _bit,_zwr},{bclr, _bit,_zwr},
	{bset, _bit,_zwr},{bclr, _bit,_zwr},{bset, _bit,_zwr},{bclr, _bit,_zwr},
	{bra,  _rel,_bra},{brn,  _rel,_bra},{bhi,  _rel,_bra},{bls,  _rel,_bra},/* 20 */
	{bcc,  _rel,_bra},{bcs,  _rel,_bra},{bne,  _rel,_bra},{beq,  _rel,_bra},
	{bhcc, _rel,_bra},{bhcs, _rel,_bra},{bpl,  _rel,_bra},{bmi,  _rel,_bra},
	{bmc,  _rel,_bra},{bms,  _rel,_bra},{bil,  _rel,_bra},{bih,  _rel,_bra},
	{neg,  _dir,_zrw},{ill,  _imp,_0  },{ill,  _imp,_0	},{com,  _dir,_zrw},/* 30 */
	{lsr,  _dir,_zrw},{ill,  _imp,_0  },{ror,  _dir,_zrw},{asr,  _dir,_zrw},
	{asl,  _dir,_zrw},{rol,  _dir,_zrw},{dec,  _dir,_zrw},{ill,  _imp,_0  },
	{inc,  _dir,_zrw},{tst,  _dir,_zrd},{ill,  _imp,_0	},{clr,  _dir,_zwr},
	{nega, _imp,_0	},{ill,  _imp,_0  },{ill,  _imp,_0	},{coma, _imp,_0  },/* 40 */
	{lsra, _imp,_0	},{ill,  _imp,_0  },{rora, _imp,_0	},{asra, _imp,_0  },
	{asla, _imp,_0	},{rola, _imp,_0  },{deca, _imp,_0	},{ill,  _imp,_0  },
	{inca, _imp,_0	},{tsta, _imp,_0  },{ill,  _imp,_0	},{clra, _imp,_0  },
	{negx, _imp,_0	},{ill,  _imp,_0  },{ill,  _imp,_0	},{comx, _imp,_0  },/* 50 */
	{lsrx, _imp,_0	},{ill,  _imp,_0  },{rorx, _imp,_0	},{asrx, _imp,_0  },
	{aslx, _imp,_0	},{rolx, _imp,_0  },{decx, _imp,_0	},{ill,  _imp,_0  },
	{incx, _imp,_0	},{tstx, _imp,_0  },{ill,  _imp,_0	},{clrx, _imp,_0  },
	{neg,  _ix1,_zrw},{ill,  _imp,_0  },{ill,  _imp,_0	},{com,  _ix1,_zrw},/* 60 */
	{lsr,  _ix1,_zrw},{ill,  _imp,_0  },{ror,  _ix1,_zrw},{asr,  _ix1,_zrw},
	{asl,  _ix1,_zrw},{rol,  _ix1,_zrw},{dec,  _ix1,_zrw},{ill,  _imp,_0  },
	{inc,  _ix1,_zrw},{tst,  _ix1,_zrw},{jmp,  _ix1,_jmp},{clr,  _ix1,_zwr},
	{neg,  _idx,_mrw},{ill,  _imp,_0  },{ill,  _imp,_0	},{com,  _idx,_mrw},/* 70 */
	{lsr,  _idx,_mrw},{ill,  _imp,_0  },{ror,  _idx,_mrw},{asr,  _idx,_mrw},
	{asl,  _idx,_mrw},{rol,  _idx,_mrw},{dec,  _idx,_mrw},{ill,  _imp,_0  },
	{inc,  _idx,_mrw},{tst,  _idx,_mrd},{jmp,  _idx,_jmp},{clr,  _idx,_mwr},
	{rti,  _imp,_0	},{rts,  _imp,_0  },{ill,  _imp,_0	},{swi,  _imp,_0  },/* 80 */
	{ill,  _imp,_0	},{ill,  _imp,_0  },{ill,  _imp,_0	},{ill,  _imp,_0  },
	{ill,  _imp,_0	},{ill,  _imp,_0  },{ill,  _imp,_0	},{ill,  _imp,_0  },
	{ill,  _imp,_0	},{ill,  _imp,_0  },{ill,  _imp,_0	},{ill,  _imp,_0  },
	{ill,  _imp,_0	},{ill,  _imp,_0  },{ill,  _imp,_0	},{ill,  _imp,_0  },/* 90 */
	{ill,  _imp,_0	},{ill,  _imp,_0  },{ill,  _imp,_0	},{tax,  _imp,_0  },
	{clc,  _imp,_0	},{sec,  _imp,_0  },{cli,  _imp,_0	},{sei,  _imp,_0  },
	{rsp,  _imp,_0	},{nop,  _imp,_0  },{ill,  _imp,_0	},{txa,  _imp,_0  },
	{suba, _imm,_0	},{cmpa, _imm,_0  },{sbca, _imm,_0	},{cpx,  _imm,_0  },/* a0 */
	{anda, _imm,_0	},{bita, _imm,_0  },{lda,  _imm,_0	},{ill,  _imp,_0  },
	{eora, _imm,_0	},{adca, _imm,_0  },{ora,  _imm,_0	},{adda, _imm,_0  },
	{ill,  _imp,_0	},{bsr,  _rel,_bra},{ldx,  _imm,_0	},{ill,  _imp,_0  },
	{suba, _dir,_zrd},{cmpa, _dir,_zrd},{sbca, _dir,_zrd},{cpx,  _dir,_zrd},/* b0 */
	{anda, _dir,_zrd},{bita, _dir,_zrd},{lda,  _dir,_zrd},{sta,  _dir,_zwr},
	{eora, _dir,_zrd},{adca, _dir,_zrd},{ora,  _dir,_zrd},{adda, _dir,_zrd},
	{jmp,  _dir,_zrd},{jsr,  _dir,_jmp},{ldx,  _dir,_zrd},{stx,  _dir,_zwr},
	{suba, _ext,_mrd},{cmpa, _ext,_mrd},{sbca, _ext,_mrd},{cpx,  _ext,_mrd},/* c0 */
	{anda, _ext,_mrd},{bita, _ext,_mrd},{lda,  _ext,_mrd},{sta,  _ext,_mwr},
	{eora, _ext,_mrd},{adca, _ext,_mrd},{ora,  _ext,_mrd},{adda, _ext,_mrd},
	{jmp,  _ext,_jmp},{jsr,  _ext,_jmp},{ldx,  _ext,_mrd},{stx,  _ext,_mwr},
	{suba, _ix2,_mrd},{cmpa, _ix2,_mrd},{sbca, _ix2,_mrd},{cpx,  _ix2,_mrd},/* d0 */
	{anda, _ix2,_mrd},{bita, _ix2,_mrd},{lda,  _ix2,_mrd},{sta,  _ix2,_mwr},
	{eora, _ix2,_mrd},{adca, _ix2,_mrd},{ora,  _ix2,_mrd},{adda, _ix2,_mrd},
	{jmp,  _ix2,_jmp},{jsr,  _ix2,_jmp},{ldx,  _ix2,_mrd},{stx,  _ix2,_mwr},
	{suba, _ix1,_zrd},{cmpa, _ix1,_zrd},{sbca, _ix1,_zrd},{cpx,  _ix1,_zrd},/* e0 */
	{anda, _ix1,_zrd},{bita, _ix1,_zrd},{lda,  _ix1,_zrd},{sta,  _ix1,_zwr},
	{eora, _ix1,_zrd},{adca, _ix1,_zrd},{ora,  _ix1,_zrd},{adda, _ix1,_zrd},
	{jmp,  _ix1,_jmp},{jsr,  _ix1,_jmp},{ldx,  _ix1,_zrd},{stx,  _ix1,_zwr},
	{suba, _idx,_mrd},{cmpa, _idx,_mrd},{sbca, _idx,_mrd},{cpx,  _idx,_mrd},/* f0 */
	{anda, _idx,_mrd},{bita, _idx,_mrd},{lda,  _idx,_mrd},{sta,  _idx,_mwr},
	{eora, _idx,_mrd},{adca, _idx,_mrd},{ora,  _idx,_mrd},{adda, _idx,_mrd},
	{jmp,  _idx,_jmp},{jsr,  _idx,_jmp},{ldx,  _idx,_mrd},{stx,  _idx,_mwr}
};

#if 0
static char *opcode_strings[0x0100] =
{
	"brset0", 	"brclr0", 	"brset1", 	"brclr1", 	"brset2", 	"brclr2",	"brset3",	"brclr3",		/*00*/
	"brset4",	"brclr4",	"brset5",	"brclr5",	"brset6",	"brclr6",	"brset7",	"brclr7",
	"bset0",	"bclr0",	"bset1", 	"bclr1", 	"bset2", 	"bclr2", 	"bset3",	"bclr3",		/*10*/
	"bset4", 	"bclr4",	"bset5", 	"bclr5",	"bset6", 	"bclr6", 	"bset7", 	"bclr7",
	"bra",		"brn",		"bhi",		"bls",		"bcc",		"bcs",		"bne",		"beq",			/*20*/
	"bhcc",		"bhcs",		"bpl",		"bmi",		"bmc",		"bms",		"bil",		"bih",
	"neg_di",   "illegal",  "illegal",  "com_di",   "lsr_di",   "illegal",  "ror_di",   "asr_di",       /*30*/
	"asl_di",	"rol_di",	"dec_di",	"illegal", 	"inc_di",	"tst_di",	"illegal", 	"clr_di",
	"nega",		"illegal", 	"illegal", 	"coma",		"lsra",		"illegal", 	"rora",		"asra",			/*40*/
	"asla",		"rola",		"deca",		"illegal", 	"inca",		"tsta",		"illegal", 	"clra",
	"negx",		"illegal", 	"illegal", 	"comx",		"lsrx",		"illegal", 	"rorx",		"asrx",			/*50*/
	"aslx",		"rolx",		"decx",		"illegal", 	"incx",		"tstx",		"illegal", 	"clrx",
	"neg_ix1",	"illegal", 	"illegal", 	"com_ix1",	"lsr_ix1",	"illegal", 	"ror_ix1",	"asr_ix1",		/*60*/
	"asl_ix1",	"rol_ix1",	"dec_ix1",	"illegal", 	"inc_ix1",	"tst_ix1",	"jmp_ix1",	"clr_ix1",
	"neg_ix",	"illegal", 	"illegal", 	"com_ix",	"lsr_ix",	"illegal", 	"ror_ix",	"asr_ix",		/*70*/
	"asl_ix",	"rol_ix",	"dec_ix",	"illegal", 	"inc_ix",	"tst_ix",	"jmp_ix",	"clr_ix",
	"rti",		"rts",		"illegal",	"swi",		"illegal",	"illegal",	"illegal",	"illegal",		/*80*/
	"illegal",	"illegal",	"illegal",	"illegal",	"illegal",	"illegal",	"illegal",	"illegal",
	"illegal",	"illegal",	"illegal",	"illegal",	"illegal",	"illegal",	"illegal",	"tax",			/*90*/
	"clc",		"sec",		"cli",		"sei",		"rsp",		"nop",		"illegal",	"txa",
	"suba_im",	"cmpa_im",	"sbca_im",	"cpx_im", 	"anda_im",	"bita_im",	"lda_im",	"illegal",		/*A0*/
	"eora_im",	"adca_im",	"ora_im",	"adda_im",	"illegal",	"bsr",		"ldx_im",	"illegal",
	"suba_di",	"cmpa_di",	"sbca_di",	"cpx_di", 	"anda_di",	"bita_di",	"lda_di",	"sta_di",		/*B0*/
	"eora_di",	"adca_di",	"ora_di",	"adda_di",	"jmp_di",	"jsr_di",	"ldx_di",	"stx_di",
	"suba_ex",	"cmpa_ex",	"sbca_ex",	"cpx_ex", 	"anda_ex",	"bita_ex",	"lda_ex",	"sta_ex",		/*C0*/
	"eora_ex",	"adca_ex",	"ora_ex",	"adda_ex",	"jmp_ex",	"jsr_ex",	"ldx_ex",	"stx_ex",
	"suba_ix2",	"cmpa_ix2",	"sbca_ix2",	"cpx_ix2", 	"anda_ix2",	"bita_ix2",	"lda_ix2",	"sta_ix2",		/*D0*/
	"eora_ix2",	"adca_ix2",	"ora_ix2",	"adda_ix2",	"jmp_ix2",	"jsr_ix2",	"ldx_ix2",	"stx_ix2",
	"suba_ix1",	"cmpa_ix1",	"sbca_ix1",	"cpx_ix1", 	"anda_ix1",	"bita_ix1",	"lda_ix1",	"sta_ix1",		/*E0*/
	"eora_ix1",	"adca_ix1",	"ora_ix1",	"adda_ix1",	"jmp_ix1",	"jsr_ix1",	"ldx_ix1",	"stx_ix1",
	"suba_ix",	"cmpa_ix",	"sbca_ix",	"cpx_ix", 	"anda_ix",	"bita_ix",	"lda_ix",	"sta_ix",		/*F0*/
	"eora_ix",	"adca_ix",	"ora_ix",	"adda_ix",	"jmp_ix",	"jsr_ix",	"ldx_ix",	"stx_ix"
};
#endif

unsigned Dasm6805 (char *buf, unsigned pc)
{
	const char *sym1, *sym2;
    int code, bit;
	unsigned addr, ea, size, access;

	code = cpu_readop(pc);
	size = disasm[code][2];
	access = disasm[code][3];

	buf += sprintf(buf, "%-6s", op_name_str[disasm[code][0]]);

	switch( disasm[code][1] )
	{
	case _btr:	/* bit test and relative branch */
		bit = (code >> 1) & 7;
		ea = cpu_readop_arg(pc+1);
		sym1 = set_ea_info(1, ea, EA_UINT8, EA_ZPG_RD);
		sym2 = set_ea_info(0, pc + 3, (INT8)cpu_readop_arg(pc+2), EA_REL_PC);
		sprintf (buf, "%d,%s,%s", bit, sym1, sym2);
		return 3;
	case _bit:	/* bit test */
		bit = (code >> 1) & 7;
		ea = cpu_readop_arg(pc+1);
		sym1 = set_ea_info(1, ea, EA_UINT8, EA_MEM_RD);
		sprintf (buf, "%d,%s", bit, sym1);
		return 2;
	case _rel:	/* relative */
		sym1 = set_ea_info(0, pc + 2, (INT8)cpu_readop_arg(pc+1), access);
		sprintf (buf, "%s", sym1);
		return 2;
	case _imm:	/* immediate */
		sym1 = set_ea_info(0, cpu_readop_arg(pc+1), EA_UINT8, EA_VALUE);
		sprintf (buf, "#%s", sym1);
		return 2;
	case _dir:	/* direct (zero page address) */
		addr = cpu_readop_arg(pc+1);
		ea = addr;
		sym1 = set_ea_info(1, ea, size, access);
        sprintf (buf, "%s", sym1);
		return 2;
	case _ext:	/* extended (16 bit address) */
		addr = (cpu_readop_arg(pc+1) << 8) + cpu_readop_arg(pc+2);
		ea = addr;
		sym1 = set_ea_info(1, ea, size, access);
		sprintf (buf, "%s", sym1);
		return 3;
	case _idx:	/* indexed */
		ea = m6805_get_reg(M6805_X);
		set_ea_info(0, ea, size, access);
		sprintf (buf, "(x)");
		return 1;
	case _ix1:	/* indexed + byte (zero page) */
		addr = cpu_readop_arg(pc+1);
		ea = (addr + activecpu_get_reg(M6805_X)) & 0xff;
		sym1 = set_ea_info(0, addr, EA_UINT8, EA_VALUE);
		sym2 = set_ea_info(0, ea, size, access);
		sprintf (buf, "(x+%s)", sym1);
		return 2;
	case _ix2:	/* indexed + word (16 bit address) */
		addr = (cpu_readop_arg(pc+1) << 8) + cpu_readop_arg(pc+2);
		ea = (addr + activecpu_get_reg(M6805_X)) & 0xffff;
		sym1 = set_ea_info(0, addr, EA_UINT16, EA_VALUE);
		sym2 = set_ea_info(0, ea, size, access);
		sprintf (buf, "(x+%s)", sym1);
		return 3;
    default:    /* implicit */
		return 1;
    }
}

#endif
