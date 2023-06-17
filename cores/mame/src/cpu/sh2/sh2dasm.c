

#include "driver.h"
#ifdef	MAME_DEBUG
#include "mamedbg.h"
#include "sh2.h"

#define SIGNX8(x)	(((INT32)(x) << 24) >> 24)
#define SIGNX12(x)	(((INT32)(x) << 20) >> 20)

#define Rn ((opcode >> 8) & 15)
#define Rm ((opcode >> 4) & 15)

static const char *regname[16] = {
	"R0", "R1", "R2", "R3", "R4", "R5", "R6", "R7",
	"R8", "R9", "R10","R11","R12","R13","R14","SP"
};

static void op0000(char *buffer, UINT32 pc, UINT16 opcode)
{
	const char *sym;

	switch(opcode & 0x3f)
	{
	case 0x02:
		sprintf(buffer,"STC     SR,%s", regname[Rn]);
		break;
	case 0x03:
		sym = set_ea_info(0, activecpu_get_reg(SH2_R0 + Rn), EA_UINT32, EA_ABS_PC);
		sprintf(buffer,"BSRF    %s", regname[Rn]);
		break;
	case 0x08:
		sprintf(buffer,"CLRT");
		break;
	case 0x09:
		sprintf(buffer,"NOP");
		break;
	case 0x0A:
		sprintf(buffer,"STS     MACH,%s", regname[Rn]);
		break;
	case 0x0B:
		sprintf(buffer,"RTS");
		break;
	case 0x12:
		sprintf(buffer,"STS     GBR,%s", regname[Rn]);
		break;
	case 0x18:
		sprintf(buffer,"SETT");
		break;
	case 0x19:
		sprintf(buffer,"DIV0U");
		break;
	case 0x1A:
		sprintf(buffer,"STS     MACL,%s", regname[Rn]);
		break;
	case 0x1B:
		sprintf(buffer,"SLEEP");
		break;
	case 0x22:
		sprintf(buffer,"STC     VBR,%s", regname[Rn]);
		break;
	case 0x23:
		sym = set_ea_info(0, activecpu_get_reg(SH2_R0 + Rn), EA_UINT32, EA_ABS_PC);
		sprintf(buffer,"BRAF    %s", regname[Rn]);
		break;
	case 0x28:
		sprintf(buffer,"CLRMAC");
		break;
	case 0x29:
		sprintf(buffer,"MOVT    %s", regname[Rn]);
		break;
	case 0x2A:
		sprintf(buffer,"STS     PR,%s", regname[Rn]);
		break;
	case 0x2B:
		sprintf(buffer,"RTE");
		break;
	default:
		switch(opcode & 15)
		{
		case  0:
			sym = set_ea_info(0, opcode, EA_UINT16, EA_VALUE);
			sprintf(buffer, "??????  %s", sym);
			break;
		case  1:
			sym = set_ea_info(0, opcode, EA_UINT16, EA_VALUE);
			sprintf(buffer, "??????  %s", sym);
			break;
		case  2:
			sym = set_ea_info(0, opcode, EA_UINT16, EA_VALUE);
			sprintf(buffer, "??????  %s", sym);
			break;
		case  3:
			sym = set_ea_info(0, opcode, EA_UINT16, EA_VALUE);
			sprintf(buffer, "??????  %s", sym);
			break;
		case  4:
			sym = set_ea_info(0, activecpu_get_reg(SH2_R0) + activecpu_get_reg(SH2_R0+Rn), EA_UINT8, EA_MEM_RD);
			sprintf(buffer, "MOV.B   %s,@(R0,%s)", regname[Rm], regname[Rn]);
			break;
		case  5:
			sym = set_ea_info(0, activecpu_get_reg(SH2_R0) + activecpu_get_reg(SH2_R0+Rn), EA_UINT16, EA_MEM_RD);
			sprintf(buffer, "MOV.W   %s,@(R0,%s)", regname[Rm], regname[Rn]);
			break;
		case  6:
			sym = set_ea_info(0, activecpu_get_reg(SH2_R0) + activecpu_get_reg(SH2_R0+Rn), EA_UINT32, EA_MEM_RD);
			sprintf(buffer, "MOV.L   %s,@(R0,%s)", regname[Rm], regname[Rn]);
			break;
		case  7:
			sprintf(buffer, "MUL.L   %s,%s\n", regname[Rm], regname[Rn]);
			break;
		case  8:
			sym = set_ea_info(0, opcode, EA_UINT16, EA_VALUE);
			sprintf(buffer, "??????  %s", sym);
			break;
		case  9:
			sym = set_ea_info(0, opcode, EA_UINT16, EA_VALUE);
			sprintf(buffer, "??????  %s", sym);
			break;
		case 10:
			sym = set_ea_info(0, opcode, EA_UINT16, EA_VALUE);
			sprintf(buffer, "??????  %s", sym);
			break;
		case 11:
			sym = set_ea_info(0, opcode, EA_UINT16, EA_VALUE);
			sprintf(buffer, "??????  %s", sym);
			break;
		case 12:
			sym = set_ea_info(0, activecpu_get_reg(SH2_R0) + activecpu_get_reg(SH2_R0+Rm), EA_UINT8, EA_MEM_WR);
			sprintf(buffer, "MOV.B   @(R0,%s),%s", regname[Rm], regname[Rn]);
			break;
		case 13:
			sym = set_ea_info(0, activecpu_get_reg(SH2_R0) + activecpu_get_reg(SH2_R0+Rm), EA_UINT16, EA_MEM_WR);
			sprintf(buffer, "MOV.W   @(R0,%s),%s", regname[Rm], regname[Rn]);
			break;
		case 14:
			sym = set_ea_info(0, activecpu_get_reg(SH2_R0) + activecpu_get_reg(SH2_R0+Rm), EA_UINT32, EA_MEM_WR);
			sprintf(buffer, "MOV.L   @(R0,%s),%s", regname[Rm], regname[Rn]);
			break;
		case 15:
			set_ea_info(0, activecpu_get_reg(SH2_R0+Rn), EA_UINT32, EA_MEM_RD);
			set_ea_info(1, activecpu_get_reg(SH2_R0+Rm), EA_UINT32, EA_MEM_RD);
			sprintf(buffer, "MAC.L   @%s+,@%s+", regname[Rn], regname[Rm]);
			break;
		}
	}
}

static void op0001(char *buffer, UINT32 pc, UINT16 opcode)
{
	const char *sym;
	sym = set_ea_info(0, (opcode & 15) * 4, EA_UINT8, EA_VALUE);
	set_ea_info(1, activecpu_get_reg(SH2_R0+Rn) + (opcode & 15) * 4, EA_UINT32, EA_MEM_RD);
	sprintf(buffer,"MOV.L   %s,@(%s,%s)\n", regname[Rm], sym, regname[Rn]);
}

static void op0010(char *buffer, UINT32 pc, UINT16 opcode)
{
	switch (opcode & 15)
	{
	case  0:
		set_ea_info(0, activecpu_get_reg(SH2_R0+Rm), EA_UINT8, EA_MEM_WR);
		sprintf(buffer, "MOV.B   %s,@%s", regname[Rm], regname[Rn]);
		break;
	case  1:
		set_ea_info(0, activecpu_get_reg(SH2_R0+Rm), EA_UINT16, EA_MEM_WR);
		sprintf(buffer, "MOV.W   %s,@%s", regname[Rm], regname[Rn]);
		break;
	case  2:
		set_ea_info(0, activecpu_get_reg(SH2_R0+Rm), EA_UINT32, EA_MEM_WR);
		sprintf(buffer, "MOV.L   %s,@%s", regname[Rm], regname[Rn]);
		break;
	case  3:
		sprintf(buffer, "??????  $%04X", opcode);
		break;
	case  4:
		set_ea_info(0, activecpu_get_reg(SH2_R0+Rm) - 1, EA_UINT8, EA_MEM_WR);
		sprintf(buffer, "MOV.B   %s,@-%s", regname[Rm], regname[Rn]);
		break;
	case  5:
		set_ea_info(0, activecpu_get_reg(SH2_R0+Rm) - 2, EA_UINT16, EA_MEM_WR);
		sprintf(buffer, "MOV.W   %s,@-%s", regname[Rm], regname[Rn]);
		break;
	case  6:
		set_ea_info(0, activecpu_get_reg(SH2_R0+Rm) - 4, EA_UINT32, EA_MEM_WR);
		sprintf(buffer, "MOV.L   %s,@-%s", regname[Rm], regname[Rn]);
		break;
	case  7:
		sprintf(buffer, "DIV0S   %s,%s", regname[Rm], regname[Rn]);
		break;
	case  8:
		sprintf(buffer, "TST     %s,%s", regname[Rm], regname[Rn]);
		break;
	case  9:
		sprintf(buffer, "AND     %s,%s", regname[Rm], regname[Rn]);
		break;
	case 10:
		sprintf(buffer, "XOR     %s,%s", regname[Rm], regname[Rn]);
		break;
	case 11:
		sprintf(buffer, "OR      %s,%s", regname[Rm], regname[Rn]);
		break;
	case 12:
		sprintf(buffer, "CMP/STR %s,%s", regname[Rm], regname[Rn]);
		break;
	case 13:
		sprintf(buffer, "XTRCT   %s,%s", regname[Rm], regname[Rn]);
		break;
	case 14:
		sprintf(buffer, "MULU.W  %s,%s", regname[Rm], regname[Rn]);
		break;
	case 15:
		sprintf(buffer, "MULS.W  %s,%s", regname[Rm], regname[Rn]);
		break;
	}
}

static void op0011(char *buffer, UINT32 pc, UINT16 opcode)
{
	switch (opcode & 15)
	{
	case  0:
		sprintf(buffer, "CMP/EQ  %s,%s", regname[Rm], regname[Rn]);
		break;
	case  1:
		sprintf(buffer, "??????  %s,%s", regname[Rm], regname[Rn]);
		break;
	case  2:
		sprintf(buffer, "CMP/HS  %s,%s", regname[Rm], regname[Rn]);
		break;
	case  3:
		sprintf(buffer, "CMP/GE  %s,%s", regname[Rm], regname[Rn]);
		break;
	case  4:
		sprintf(buffer, "DIV1    %s,%s", regname[Rm], regname[Rn]);
		break;
	case  5:
		sprintf(buffer, "DMULU.L %s,%s", regname[Rm], regname[Rn]);
		break;
	case  6:
		sprintf(buffer, "CMP/HI  %s,%s", regname[Rm], regname[Rn]);
		break;
	case  7:
		sprintf(buffer, "CMP/GT  %s,%s", regname[Rm], regname[Rn]);
		break;
	case  8:
		sprintf(buffer, "SUB     %s,%s", regname[Rm], regname[Rn]);
		break;
	case  9:
		sprintf(buffer, "??????  %s,%s", regname[Rm], regname[Rn]);
		break;
	case 10:
		sprintf(buffer, "SUBC    %s,%s", regname[Rm], regname[Rn]);
		break;
	case 11:
		sprintf(buffer, "SUBV    %s,%s", regname[Rm], regname[Rn]);
		break;
	case 12:
		sprintf(buffer, "ADD     %s,%s", regname[Rm], regname[Rn]);
		break;
	case 13:
		sprintf(buffer, "DMULS.L %s,%s", regname[Rm], regname[Rn]);
		break;
	case 14:
		sprintf(buffer, "ADDC    %s,%s", regname[Rm], regname[Rn]);
		break;
	case 15:
		sprintf(buffer, "ADDV    %s,%s", regname[Rm], regname[Rn]);
		break;
	}
}

static void op0100(char *buffer, UINT32 pc, UINT16 opcode)
{
	switch(opcode & 0x3F)
	{
	case 0x00:
		sprintf(buffer, "SHLL    %s", regname[Rn]);
		break;
	case 0x01:
		sprintf(buffer, "SHLR    %s", regname[Rn]);
		break;
	case 0x02:
		set_ea_info(0, activecpu_get_reg(SH2_R0+Rn) - 4, EA_UINT32, EA_MEM_WR);
		sprintf(buffer, "STS.L   MACH,@-%s", regname[Rn]);
		break;
	case 0x03:
		set_ea_info(0, activecpu_get_reg(SH2_R0+Rn) - 4, EA_UINT32, EA_MEM_WR);
		sprintf(buffer, "STC.L   SR,@-%s", regname[Rn]);
		break;
	case 0x04:
		sprintf(buffer, "ROTL    %s", regname[Rn]);
		break;
	case 0x05:
		sprintf(buffer, "ROTR    %s", regname[Rn]);
		break;
	case 0x06:
		set_ea_info(0, activecpu_get_reg(SH2_R0+Rn), EA_UINT32, EA_MEM_RD);
		sprintf(buffer, "LDS.L   @%s+,MACH", regname[Rn]);
		break;
	case 0x07:
		set_ea_info(0, activecpu_get_reg(SH2_R0+Rn), EA_UINT32, EA_MEM_RD);
		sprintf(buffer, "LDC.L   @%s+,SR", regname[Rn]);
		break;
	case 0x08:
		sprintf(buffer, "SHLL2   %s", regname[Rn]);
		break;
	case 0x09:
		sprintf(buffer, "SHLR2   %s", regname[Rn]);
		break;
	case 0x0a:
		sprintf(buffer, "LDS     %s,MACH", regname[Rn]);
		break;
	case 0x0b:
		sprintf(buffer, "JSR     %s", regname[Rn]);
		break;
	case 0x0e:
		sprintf(buffer, "LDC     %s,SR", regname[Rn]);
		break;
	case 0x10:
		sprintf(buffer, "DT      %s", regname[Rn]);
		break;
	case 0x11:
		sprintf(buffer, "CMP/PZ  %s", regname[Rn]);
		break;
	case 0x12:
		sprintf(buffer, "STS.L   MACL,@-%s", regname[Rn]);
		break;
	case 0x13:
		sprintf(buffer, "STC.L   GBR,@-%s", regname[Rn]);
		break;
	case 0x15:
		sprintf(buffer, "CMP/PL  %s", regname[Rn]);
		break;
	case 0x16:
		set_ea_info(0, activecpu_get_reg(SH2_R0+Rn), EA_UINT32, EA_MEM_RD);
		sprintf(buffer, "LDS.L   @%s+,MACL", regname[Rn]);
		break;
	case 0x17:
		set_ea_info(0, activecpu_get_reg(SH2_R0+Rn), EA_UINT32, EA_MEM_RD);
		sprintf(buffer, "LDC.L   @%s+,GBR", regname[Rn]);
		break;
	case 0x18:
		sprintf(buffer, "SHLL8   %s", regname[Rn]);
		break;
	case 0x19:
		sprintf(buffer, "SHLR8   %s", regname[Rn]);
		break;
	case 0x1a:
		sprintf(buffer, "LDS     %s,MACL", regname[Rn]);
		break;
	case 0x1b:
		sprintf(buffer, "TAS     %s", regname[Rn]);
		break;
	case 0x1e:
		sprintf(buffer, "LDC     %s,GBR", regname[Rn]);
		break;
	case 0x20:
		sprintf(buffer, "SHAL    %s", regname[Rn]);
		break;
	case 0x21:
		sprintf(buffer, "SHAR    %s", regname[Rn]);
		break;
	case 0x22:
		set_ea_info(0, activecpu_get_reg(SH2_R0+Rn) - 4, EA_UINT32, EA_MEM_WR);
		sprintf(buffer, "STS.L   PR,@-%s", regname[Rn]);
		break;
	case 0x23:
		set_ea_info(0, activecpu_get_reg(SH2_R0+Rn) - 4, EA_UINT32, EA_MEM_WR);
		sprintf(buffer, "STC.L   VBR,@-%s", regname[Rn]);
		break;
	case 0x24:
		sprintf(buffer, "ROTCL   %s", regname[Rn]);
		break;
	case 0x25:
		sprintf(buffer, "ROTCR   %s", regname[Rn]);
		break;
	case 0x26:
		set_ea_info(0, activecpu_get_reg(SH2_R0+Rn), EA_UINT32, EA_MEM_RD);
		sprintf(buffer, "LDS.L   @%s+,PR", regname[Rn]);
		break;
	case 0x27:
		set_ea_info(0, activecpu_get_reg(SH2_R0+Rn), EA_UINT32, EA_MEM_RD);
		sprintf(buffer, "LDC.L   @%s+,VBR", regname[Rn]);
		break;
	case 0x28:
		sprintf(buffer, "SHLL16  %s", regname[Rn]);
		break;
	case 0x29:
		sprintf(buffer, "SHLR16  %s", regname[Rn]);
		break;
	case 0x2a:
		sprintf(buffer, "LDS     %s,PR", regname[Rn]);
		break;
	case 0x2b:
		sprintf(buffer, "JMP     %s", regname[Rn]);
		break;
	case 0x2e:
		sprintf(buffer, "LDC     %s,VBR", regname[Rn]);
		break;
	default:
		if ((opcode & 15) == 15)
		{
			set_ea_info(0, activecpu_get_reg(SH2_R0+Rm), EA_UINT32, EA_MEM_RD);
			set_ea_info(1, activecpu_get_reg(SH2_R0+Rn), EA_UINT32, EA_MEM_RD);
			sprintf(buffer, "MAC.W   @%s+,@%s+", regname[Rm], regname[Rn]);
		}
		else
			sprintf(buffer, "??????  $%04X", opcode);
	}
}

static void op0101(char *buffer, UINT32 pc, UINT16 opcode)
{
	const char *sym;
	sym = set_ea_info(0, (opcode & 15) * 4, EA_UINT8, EA_VALUE);
	set_ea_info(1, activecpu_get_reg(SH2_R0+Rm) + (opcode & 15) * 4, EA_UINT32, EA_MEM_RD);
	sprintf(buffer, "MOV.L   @(%s,%s),%s\n", sym, regname[Rm], regname[Rn]);
}

static void op0110(char *buffer, UINT32 pc, UINT16 opcode)

{
	switch(opcode & 0xF)
	{
	case 0x00:
		set_ea_info(0, activecpu_get_reg(SH2_R0+Rm), EA_UINT8, EA_MEM_RD);
		sprintf(buffer, "MOV.B   @%s,%s", regname[Rm], regname[Rn]);
		break;
	case 0x01:
		set_ea_info(0, activecpu_get_reg(SH2_R0+Rm), EA_UINT16, EA_MEM_RD);
		sprintf(buffer, "MOV.W   @%s,%s", regname[Rm], regname[Rn]);
		break;
	case 0x02:
		set_ea_info(0, activecpu_get_reg(SH2_R0+Rm), EA_UINT32, EA_MEM_RD);
		sprintf(buffer, "MOV.L   @%s,%s", regname[Rm], regname[Rn]);
		break;
	case 0x03:
		sprintf(buffer, "MOV     %s,%s", regname[Rm], regname[Rn]);
		break;
	case 0x04:
		set_ea_info(0, activecpu_get_reg(SH2_R0+Rm), EA_UINT8, EA_MEM_RD);
		sprintf(buffer, "MOV.B   @%s+,%s", regname[Rm], regname[Rn]);
		break;
	case 0x05:
		set_ea_info(0, activecpu_get_reg(SH2_R0+Rm), EA_UINT16, EA_MEM_RD);
		sprintf(buffer, "MOV.W   @%s+,%s", regname[Rm], regname[Rn]);
		break;
	case 0x06:
		set_ea_info(0, activecpu_get_reg(SH2_R0+Rm), EA_UINT32, EA_MEM_RD);
		sprintf(buffer, "MOV.L   @%s+,%s", regname[Rm], regname[Rn]);
		break;
	case 0x07:
		sprintf(buffer, "NOT     %s,%s", regname[Rm], regname[Rn]);
		break;
	case 0x08:
		sprintf(buffer, "SWAP.B  %s,%s", regname[Rm], regname[Rn]);
		break;
	case 0x09:
		sprintf(buffer, "SWAP.W  %s,%s", regname[Rm], regname[Rn]);
		break;
	case 0x0a:
		sprintf(buffer, "NEGC    %s,%s", regname[Rm], regname[Rn]);
		break;
	case 0x0b:
		sprintf(buffer, "NEG     %s,%s", regname[Rm], regname[Rn]);
		break;
	case 0x0c:
		sprintf(buffer, "EXTU.B  %s,%s", regname[Rm], regname[Rn]);
		break;
	case 0x0d:
		sprintf(buffer, "EXTU.W  %s,%s", regname[Rm], regname[Rn]);
		break;
	case 0x0e:
		sprintf(buffer, "EXTS.B  %s,%s", regname[Rm], regname[Rn]);
		break;
	case 0x0f:
		sprintf(buffer, "EXTS.W  %s,%s", regname[Rm], regname[Rn]);
		break;
	}
}

static void op0111(char *buffer, UINT32 pc, UINT16 opcode)
{
	const char *sym;
	sym = set_ea_info(0, opcode & 0xff, EA_UINT8, EA_VALUE);
	sprintf(buffer, "ADD     #%s,%s\n", sym, regname[Rn]);
}

static void op1000(char *buffer, UINT32 pc, UINT16 opcode)
{
	const char *sym;
	switch((opcode >> 8) & 15)
	{
	case  0:
		sym = set_ea_info(0, (opcode & 15), EA_UINT8, EA_VALUE);
		set_ea_info(1, activecpu_get_reg(SH2_R0+Rm) + (opcode & 15), EA_UINT8, EA_MEM_WR);
		sprintf(buffer, "MOV.B   R0,@(%s,%s)", sym, regname[Rm]);
		break;
	case  1:
		sym = set_ea_info(0, (opcode & 15) * 2, EA_UINT8, EA_VALUE);
		set_ea_info(1, activecpu_get_reg(SH2_R0+Rm) + (opcode & 15)*2, EA_UINT16, EA_MEM_WR);
		sprintf(buffer, "MOV.W   R0,@(%s,%s)", sym, regname[Rm]);
		break;
	case  4:
		sym = set_ea_info(0, (opcode & 15), EA_UINT8, EA_VALUE);
		set_ea_info(1, activecpu_get_reg(SH2_R0+Rm) + (opcode & 15), EA_UINT8, EA_MEM_RD);
		sprintf(buffer, "MOV.B   @(%s,%s),R0", sym, regname[Rm]);
		break;
	case  5:
		sym = set_ea_info(0, (opcode & 15), EA_UINT8, EA_VALUE);
		set_ea_info(1, activecpu_get_reg(SH2_R0+Rm) + (opcode & 15)*2, EA_UINT16, EA_MEM_RD);
		sprintf(buffer, "MOV.W   @(%s,%s),R0", sym, regname[Rm]);
		break;
	case  8:
		sym = set_ea_info(0, (opcode & 0xff), EA_UINT8, EA_VALUE);
		sprintf(buffer, "CMP/EQ  #%s,R0", sym);
		break;
	case  9:
		sym = set_ea_info(0, pc, SIGNX8(opcode & 0xff) * 2 + 2, EA_REL_PC);
		sprintf(buffer, "BT      %s", sym);
		break;
	case 11:
		sym = set_ea_info(0, pc, SIGNX8(opcode & 0xff) * 2 + 2, EA_REL_PC);
		sprintf(buffer, "BF      %s", sym);
		break;
	case 13:
		sym = set_ea_info(0, pc, SIGNX8(opcode & 0xff) * 2 + 2, EA_REL_PC);
		sprintf(buffer, "BTS     %s", sym);
		break;
	case 15:
		sym = set_ea_info(0, pc, SIGNX8(opcode & 0xff) * 2 + 2, EA_REL_PC);
		sprintf(buffer, "BFS     %s", sym);
		break;
	default :
		sym = set_ea_info(0, opcode, EA_UINT16, EA_VALUE);
		sprintf(buffer, "invalid %s\n", sym);
	}
}

static void op1001(char *buffer, UINT32 pc, UINT16 opcode)
{
	const char *sym;
	sym = set_ea_info(0, (opcode & 0xff) * 2, EA_UINT16, EA_VALUE);
	set_ea_info(0, (opcode & 0xff) * 2 + pc + 2, EA_UINT16, EA_MEM_RD);
	sprintf(buffer, "MOV.W   @(%s,PC),%s", sym, regname[Rn]);
}

static void op1010(char *buffer, UINT32 pc, UINT16 opcode)
{
	const char *sym;
	sym = set_ea_info(0, SIGNX12(opcode & 0xfff) * 2 + pc + 2, EA_UINT32, EA_ABS_PC);
	sprintf(buffer, "BRA     %s", sym);
}

static void op1011(char *buffer, UINT32 pc, UINT16 opcode)
{
	const char *sym;
	sym = set_ea_info(0, SIGNX12(opcode & 0xfff) * 2 + pc + 2, EA_UINT32, EA_ABS_PC);
	sprintf(buffer, "BSR     %s", sym);
}

static void op1100(char *buffer, UINT32 pc, UINT16 opcode)
{
	const char *sym;
	switch((opcode >> 8) & 15)
	{
	case  0:
		sym = set_ea_info(0, opcode & 0xff, EA_UINT8, EA_VALUE);
		set_ea_info(1, activecpu_get_reg(SH2_GBR) + (opcode & 0xff), EA_UINT8, EA_MEM_WR);
		sprintf(buffer, "MOV.B   R0,@(%s,GBR)", sym);
		break;
	case  1:
		sym = set_ea_info(0, (opcode & 0xff) * 2, EA_UINT16, EA_VALUE);
		set_ea_info(1, (opcode & 0xff) * 2 + activecpu_get_reg(SH2_GBR), EA_UINT16, EA_MEM_WR);
		sprintf(buffer, "MOV.W   R0,@(%s,GBR)", sym);
		break;
	case  2:
		sym = set_ea_info(0, (opcode & 0xff) * 4, EA_UINT16, EA_VALUE);
		set_ea_info(1, (opcode & 0xff) * 4 + activecpu_get_reg(SH2_GBR), EA_UINT32, EA_MEM_WR);
		sprintf(buffer, "MOV.L   R0,@(%s,GBR)", sym);
		break;
	case  3:
		sym = set_ea_info(0, opcode & 0xff, EA_UINT8, EA_VALUE);
		sprintf(buffer, "TRAPA   #%s", sym);
		break;
	case  4:
		sym = set_ea_info(0, opcode & 0xff, EA_UINT8, EA_VALUE);
		set_ea_info(1, activecpu_get_reg(SH2_GBR) + (opcode & 0xff), EA_UINT8, EA_MEM_RD);
		sprintf(buffer, "MOV.B   @(%s,GBR),R0", sym);
		break;
	case  5:
		sym = set_ea_info(0, (opcode & 0xff) * 2, EA_UINT16, EA_VALUE);
		set_ea_info(1, (opcode & 0xff) * 2 + activecpu_get_reg(SH2_GBR), EA_UINT16, EA_MEM_RD);
		sprintf(buffer, "MOV.W   @(%s,GBR),R0", sym);
		break;
	case  6:
		sym = set_ea_info(0, (opcode & 0xff) * 4, EA_UINT16, EA_VALUE);
		set_ea_info(1, (opcode & 0xff) * 4 + activecpu_get_reg(SH2_GBR), EA_UINT32, EA_MEM_RD);
		sprintf(buffer, "MOV.L   @(%s,GBR),R0", sym);
		break;
	case  7:
		sym = set_ea_info(0, (opcode & 0xff) * 4, EA_UINT16, EA_VALUE);
		set_ea_info(1, (opcode & 0xff) * 4 + pc + 2, EA_UINT32, EA_ABS_PC);
		sprintf(buffer, "MOVA    @(%s,PC),R0", sym);
		break;
	case  8:
		sym = set_ea_info(0, opcode & 0xff, EA_UINT8, EA_VALUE);
		sprintf(buffer, "TST     #%s,R0", sym);
		break;
	case  9:
		sym = set_ea_info(0, opcode & 0xff, EA_UINT8, EA_VALUE);
		sprintf(buffer, "AND     #%s,R0", sym);
		break;
	case 10:
		sym = set_ea_info(0, opcode & 0xff, EA_UINT8, EA_VALUE);
		sprintf(buffer, "XOR     #%s,R0", sym);
		break;
	case 11:
		sym = set_ea_info(0, opcode & 0xff, EA_UINT8, EA_VALUE);
		sprintf(buffer, "OR      #%s,R0", sym);
		break;
	case 12:
		sym = set_ea_info(0, opcode & 0xff, EA_UINT8, EA_VALUE);
		set_ea_info(1, activecpu_get_reg(SH2_R0) + activecpu_get_reg(SH2_GBR), EA_UINT8, EA_MEM_RD);
		sprintf(buffer, "TST.B   #%s,@(R0,GBR)", sym);
		break;
	case 13:
		sym = set_ea_info(0, opcode & 0xff, EA_UINT8, EA_VALUE);
		set_ea_info(1, activecpu_get_reg(SH2_R0) + activecpu_get_reg(SH2_GBR), EA_UINT8, EA_MEM_RDWR);
		sprintf(buffer, "AND.B   #%s,@(R0,GBR)", sym);
		break;
	case 14:
		sym = set_ea_info(0, opcode & 0xff, EA_UINT8, EA_VALUE);
		set_ea_info(1, activecpu_get_reg(SH2_R0) + activecpu_get_reg(SH2_GBR), EA_UINT8, EA_MEM_RDWR);
		sprintf(buffer, "XOR.B   #%s,@(R0,GBR)", sym);
		break;
	case 15:
		sym = set_ea_info(0, opcode & 0xff, EA_UINT8, EA_VALUE);
		set_ea_info(1, activecpu_get_reg(SH2_R0) + activecpu_get_reg(SH2_GBR), EA_UINT8, EA_MEM_RDWR);
		sprintf(buffer, "OR.B    #%s,@(R0,GBR)", sym);
		break;
	}
}

static void op1101(char *buffer, UINT32 pc, UINT16 opcode)
{
	const char *sym;
	sym = set_ea_info(0, (opcode & 0xff) * 4, EA_UINT8, EA_VALUE);
	set_ea_info(1, (opcode & 0xff) * 4 + ((pc + 2) & ~3), EA_UINT32, EA_ABS_PC);
	sprintf(buffer, "MOV.L   @(%s,PC),%s", sym, regname[Rn]);
}

static void op1110(char *buffer, UINT32 pc, UINT16 opcode)
{
	const char *sym;
	sym = set_ea_info(0, (UINT32)(INT32)(INT16)(INT8)(opcode & 0xff), EA_UINT8, EA_VALUE);
	sprintf(buffer, "MOV     #%s,%s", sym, regname[Rn]);
}

static void op1111(char *buffer, UINT32 pc, UINT16 opcode)
{
	const char *sym;
	sym = set_ea_info(0, opcode, EA_UINT16, EA_VALUE);
	sprintf(buffer, "unknown %s", sym);
}

unsigned DasmSH2(char *buffer, unsigned pc)
{
	UINT16 opcode;
	opcode = cpu_readmem32bedw_word(pc & 0x1fffffff);
	pc += 2;

	switch((opcode >> 12) & 15)
	{
	case  0: op0000(buffer,pc,opcode);	  break;
	case  1: op0001(buffer,pc,opcode);	  break;
	case  2: op0010(buffer,pc,opcode);	  break;
	case  3: op0011(buffer,pc,opcode);	  break;
	case  4: op0100(buffer,pc,opcode);	  break;
	case  5: op0101(buffer,pc,opcode);	  break;
	case  6: op0110(buffer,pc,opcode);	  break;
	case  7: op0111(buffer,pc,opcode);	  break;
	case  8: op1000(buffer,pc,opcode);	  break;
	case  9: op1001(buffer,pc,opcode);	  break;
	case 10: op1010(buffer,pc,opcode);	  break;
	case 11: op1011(buffer,pc,opcode);	  break;
	case 12: op1100(buffer,pc,opcode);	  break;
	case 13: op1101(buffer,pc,opcode);	  break;
	case 14: op1110(buffer,pc,opcode);	  break;
	default: op1111(buffer,pc,opcode);	  break;
	}
	return 2;
}

#endif
