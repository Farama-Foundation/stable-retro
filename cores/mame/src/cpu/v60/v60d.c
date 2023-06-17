
#include "cpuintrf.h"
#include "osd_cpu.h"
#include "mame2003.h"
#include "mamedbg.h"

#include <stdio.h>
#include "v60.h"

static UINT8 (*readop)(offs_t adr);

static signed char read8(unsigned pc)
{
	return readop(pc);
}

static signed short read16(unsigned pc)
{
	return readop(pc) | (readop(pc+1) << 8);
}

static signed int read32(unsigned pc)
{
	return readop(pc) | (readop(pc+1) << 8)| (readop(pc+2) << 16)| (readop(pc+3) << 24);
}

static void out_AM_Register(int reg, char *out)
{
	strcat(out, v60_reg_names[reg]);
}

static void out_AM_RegisterIndirect(int reg, int opsize, char *out)
{
	if(opsize & 0x80)
		*out++ = '@';
	sprintf(out, "[%s]", v60_reg_names[reg]);
}

static void out_AM_RegisterIndirectIndexed(int rn, int rx, int opsize, char *out)
{
	if(opsize & 0x80)
		sprintf(out, "%s@[%s]", v60_reg_names[rx], v60_reg_names[rn]);
	else
		sprintf(out, "[%s](%s)", v60_reg_names[rn], v60_reg_names[rx]);

}

static void out_AM_Autoincrement(int reg, int opsize, char *out)
{
	if(opsize & 0x80)
		*out++ = '@';
	sprintf(out, "[%s+]", v60_reg_names[reg]);
}

static void out_AM_Autodecrement(int reg, int opsize, char *out)
{
	if(opsize & 0x80)
		*out++ = '@';
	sprintf(out, "[-%s]", v60_reg_names[reg]);
}

static void out_AM_Displacement(int reg, int disp, int opsize, char *out)
{
	sprintf(out, "%s%X%s[%s]",
			disp >= 0 ? "" : "-", disp >= 0 ? disp : -disp,
			opsize & 0x80 ? "@" : "",
			v60_reg_names[reg]);
}

static void out_AM_DisplacementIndexed(int rn, int rx, int disp, int opsize, char *out)
{
	if(opsize & 0x80)
		sprintf(out, "%s@%s%X[%s]", v60_reg_names[rx], disp >= 0 ? "" : "-", disp >= 0 ? disp : -disp,v60_reg_names[rn]);
	else
		sprintf(out, "%s%X[%s](%s)", disp >= 0 ? "" : "-", disp >= 0 ? disp : -disp,v60_reg_names[rn], v60_reg_names[rx]);
}

static void out_AM_PCDisplacement(unsigned pc, int disp, int opsize, char *out)
{
	sprintf(out, "%X%s[PC]", pc+disp, opsize & 0x80 ? "@" : "");
}

static void out_AM_PCDisplacementIndexed(unsigned pc, int disp, int rx, int opsize, char *out)
{
	if(opsize & 0x80)
		sprintf(out, "%s@%X[PC]", v60_reg_names[rx], pc+disp);
	else
		sprintf(out, "%X[PC](%s)", pc+disp, v60_reg_names[rx]);
}

static void out_AM_DisplacementIndirect(int reg, int disp, int opsize, char *out)
{
	sprintf(out, "%s[%s%X[%s]]",
			opsize & 0x80 ? "@" : "",
			disp >= 0 ? "" : "-", disp >= 0 ? disp : -disp,
			v60_reg_names[reg]);
}

static void out_AM_DisplacementIndirectIndexed(int rn, int rx, int disp, int opsize, char *out)
{
	if(opsize & 0x80)
		sprintf(out, "%s@[%s%X[%s]]", v60_reg_names[rx], disp >= 0 ? "" : "-", disp >= 0 ? disp : -disp,v60_reg_names[rn]);
	else
		sprintf(out, "[%s%X[%s]](%s)", disp >= 0 ? "" : "-", disp >= 0 ? disp : -disp,v60_reg_names[rn], v60_reg_names[rx]);
}

static void out_AM_PCDisplacementIndirect(unsigned pc, int disp, int opsize, char *out)
{
	sprintf(out, "%s[%X[PC]]", opsize & 0x80 ? "@" : "", pc+disp);
}

static void out_AM_PCDisplacementIndirectIndexed(unsigned pc, int disp, int rx, int opsize, char *out)
{
	if(opsize & 0x80)
		sprintf(out, "%s@[%X[PC]]", v60_reg_names[rx], pc+disp);
	else
		sprintf(out, "[%X[PC]](%s)", pc+disp, v60_reg_names[rx]);
}

static void out_AM_DoubleDisplacement(int reg, int disp2, int disp1, int opsize, char *out)
{
	sprintf(out, "%s%X%s[%s%X[%s]]",
			disp1 >= 0 ? "" : "-", disp1 >= 0 ? disp1 : -disp1,
			opsize & 0x80 ? "@" : "",
			disp2 >= 0 ? "" : "-", disp2 >= 0 ? disp2 : -disp2,
			v60_reg_names[reg]);
}

static void out_AM_PCDoubleDisplacement(unsigned pc, int disp2, int disp1, int opsize, char *out)
{
	sprintf(out, "%s%X%s[%X[PC]]",
			disp1 >= 0 ? "" : "-", disp1 >= 0 ? disp1 : -disp1,
			opsize & 0x80 ? "@" : "",
			disp2 + pc);
}

static void out_AM_DirectAddress(unsigned addr, int opsize, char *out)
{
	if(opsize & 0x80)
		*out++ = '@';
	sprintf(out, "%X", addr);
}

static void out_AM_DirectAddressIndexed(unsigned addr, int rx, int opsize, char *out)
{
	if(opsize & 0x80)
		sprintf(out, "%s@%X", v60_reg_names[rx], addr);
	else
		sprintf(out, "%X(%s)", addr, v60_reg_names[rx]);
}

static void out_AM_DirectAddressDeferred(unsigned addr, int opsize, char *out)
{
	if(opsize & 0x80)
		*out++ = '@';
	sprintf(out, "[%X]", addr);
}

static void out_AM_DirectAddressDeferredIndexed(unsigned addr, int rx, int opsize, char *out)
{
	if(opsize & 0x80)
		sprintf(out, "%s@[%X]", v60_reg_names[rx], addr);
	else
		sprintf(out, "[%X](%s)", addr, v60_reg_names[rx]);
}

static void out_AM_Immediate(unsigned value, int opsize, char *out)
{
	if(opsize == 0)
		value &= 0xff;
	else if(opsize == 1)
		value &= 0xffff;

	sprintf(out, "#%X", value);
}

static int decode_AM(unsigned ipc, unsigned pc, int m, int opsize, char *out)
{
	unsigned char mod = readop(pc);
	if(m) {
		switch(mod>>5) {
		case 0: /* Double displacement (8 bit)*/
			out_AM_DoubleDisplacement(mod&0x1F, read8(pc+1), read8(pc+2), opsize, out);
			return 3;

		case 1: /* Double displacement (16 bit)*/
			out_AM_DoubleDisplacement(mod&0x1F, read16(pc+1), read16(pc+3), opsize, out);
			return 5;

		case 2: /* Double displacement (32 bit)*/
			out_AM_DoubleDisplacement(mod&0x1F, read32(pc+1), read32(pc+5), opsize, out);
			return 9;

		case 3: /* Register*/
			out_AM_Register(mod&0x1F, out);
			return 1;

		case 4: /* Autoincrement*/
			out_AM_Autoincrement(mod&0x1F, opsize, out);
			return 1;

		case 5: /* Autodecrement*/
			out_AM_Autodecrement(mod&0x1F, opsize, out);
			return 1;

		case 6:
			switch (readop(pc+1)>>5)
				{
				case 0: /* Displacement indexed (8 bit)*/
					out_AM_DisplacementIndexed(readop(pc+1)&0x1F, mod&0x1F, read8(pc+2), opsize, out);
					return 3;

				case 1: /* Displacement indexed (16 bit)*/
					out_AM_DisplacementIndexed(readop(pc+1)&0x1F, mod&0x1F, read16(pc+2), opsize, out);
					return 4;

				case 2: /* Displacement indexed (32 bit)*/
					out_AM_DisplacementIndexed(readop(pc+1)&0x1F, mod&0x1F, read32(pc+2), opsize, out);
					return 6;

				case 3:	/* Register indirect indexed*/
					out_AM_RegisterIndirectIndexed(readop(pc+1)&0x1F, mod&0x1F, opsize, out);
					return 2;

				case 4: /* Displacement indirect indexed (8 bit)*/
					out_AM_DisplacementIndirectIndexed(readop(pc+1)&0x1F, mod&0x1F, read8(pc+2), opsize, out);
					return 3;

				case 5: /* Displacement indirect indexed (16 bit)*/
					out_AM_DisplacementIndirectIndexed(readop(pc+1)&0x1F, mod&0x1F, read16(pc+2), opsize, out);
					return 4;

				case 6: /* Displacement indirect indexed (32 bit)*/
					out_AM_DisplacementIndirectIndexed(readop(pc+1)&0x1F, mod&0x1F, read32(pc+2), opsize, out);
					return 6;

				case 7:
					switch (readop(pc+1)&0x1F)
						{
						case 16: /* PC Displacement Indexed (8 bit)*/
							out_AM_PCDisplacementIndexed(ipc, read8(pc+2), mod&0x1F, opsize, out);
							return 3;

						case 17: /* PC Displacement Indexed (16 bit)*/
							out_AM_PCDisplacementIndexed(ipc, read16(pc+2), mod&0x1F, opsize, out);
							return 4;

						case 18: /* PC Displacement Indexed (32 bit)*/
							out_AM_PCDisplacementIndexed(ipc, read32(pc+2), mod&0x1F, opsize, out);
							return 6;

						case 19: /* Direct Address Indexed*/
							out_AM_DirectAddressIndexed(read32(pc+2), mod&0x1F, opsize, out);
							return 6;

						case 24: /* PC Displacement Indirect Indexed(8 bit)*/
							out_AM_PCDisplacementIndirectIndexed(ipc, read8(pc+2), mod&0x1F, opsize, out);
							return 3;

						case 25: /* PC Displacement Indirect Indexed (16 bit)*/
							out_AM_PCDisplacementIndirectIndexed(ipc, read16(pc+2), mod&0x1F, opsize, out);
							return 4;

						case 26: /* PC Displacement Indirect Indexed (32 bit)*/
							out_AM_PCDisplacementIndirectIndexed(ipc, read32(pc+2), mod&0x1F, opsize, out);
							return 6;

						case 27: /* Direct Address Deferred Indexed*/
							out_AM_DirectAddressDeferredIndexed(read32(pc+2), mod&0x1F, opsize, out);
							return 6;

						default:
							strcat(out, "!ERRAM3");
							return 1;
						}

				default:
					strcat(out, "!ERRAM2");
					return 1;
				}

		default:
			strcat(out, "!ERRAM1");
			return 1;
		}
	} else {
		switch(mod>>5) {
		case 0: /* Displacement (8 bit)*/
			out_AM_Displacement(mod&0x1F, read8(pc+1), opsize, out);
			return 2;

		case 1: /* Displacement (16 bit)*/
			out_AM_Displacement(mod&0x1F, read16(pc+1), opsize, out);
			return 3;

		case 2: /* Displacement (32 bit)*/
			out_AM_Displacement(mod&0x1F, read32(pc+1), opsize, out);
			return 5;

		case 3: /* Register indirect*/
			out_AM_RegisterIndirect(mod&0x1F, opsize, out);
			return 1;

		case 4: /* Displacement indirect (8 bit)*/
			out_AM_DisplacementIndirect(mod&0x1F, read8(pc+1), opsize, out);
			return 2;

		case 5: /* Displacement indirect (16 bit)*/
			out_AM_DisplacementIndirect(mod&0x1F, read16(pc+1), opsize, out);
			return 3;

		case 6: /* Displacement indirect (32 bit)*/
			out_AM_DisplacementIndirect(mod&0x1F, read32(pc+1), opsize, out);
			return 5;

		case 7:
			switch(mod&0x1F) {
			case 0:
			case 1:
			case 2:
			case 3:
			case 4:
			case 5:
			case 6:
			case 7:
			case 8:
			case 9:
			case 10:
			case 11:
			case 12:
			case 13:
			case 14:
			case 15:
				out_AM_Immediate(mod&0x1F, opsize, out);
				return 1;

			case 16: /* PC Displacement (8 bit)*/
				out_AM_PCDisplacement(ipc, read8(pc+1), opsize, out);
				return 2;

			case 17: /* PC Displacement (16 bit)*/
				out_AM_PCDisplacement(ipc, read16(pc+1), opsize, out);
				return 3;

			case 18: /* PC Displacement (32 bit)*/
				out_AM_PCDisplacement(ipc, read32(pc+1), opsize, out);
				return 5;

			case 19: /* Direct Address*/
				out_AM_DirectAddress(read32(pc+1), opsize, out);
				return 5;


			case 20:
				switch(opsize&0x7F) {
				case 0: /* Immediate (8 bit)*/
					out_AM_Immediate(read8(pc+1), opsize, out);
					return 2;

				case 1: /* Immediate (16 bit)*/
					out_AM_Immediate(read16(pc+1), opsize, out);
					return 3;

				case 2: /* Immediate (32 bit)*/
					out_AM_Immediate(read32(pc+1), opsize, out);
					return 5;

				default:
					strcat(out, "!ERRAM6");
					return 1;
				}

			case 24: /* PC Displacement Indirect (8 bit)*/
				out_AM_PCDisplacementIndirect(ipc, read8(pc+1), opsize, out);
				return 2;

			case 25: /* PC Displacement Indirect (16 bit)*/
				out_AM_PCDisplacementIndirect(ipc, read16(pc+1), opsize, out);
				return 3;

			case 26: /* PC Displacement Indirect (32 bit)*/
				out_AM_PCDisplacementIndirect(ipc, read32(pc+1), opsize, out);
				return 5;

			case 27: /* Direct Address Deferred*/
				out_AM_DirectAddressDeferred(read32(pc+1), opsize, out);
				return 5;

			case 28: /* PC Double Displacement (8 bit)*/
				out_AM_PCDoubleDisplacement(ipc, read8(pc+1), read8(pc+2), opsize, out);
				return 3;

			case 29: /* PC Double Displacement (16 bit)*/
				out_AM_PCDoubleDisplacement(ipc, read16(pc+1), read16(pc+3), opsize, out);
				return 3;

			case 30: /* PC Double Displacement (32 bit)*/
				out_AM_PCDoubleDisplacement(ipc, read32(pc+1), read32(pc+5), opsize, out);
				return 3;

			default:
				strcat(out, "!ERRAM5");
				return 1;
			}

		default:
			strcat(out, "!ERRAM4");
			return 1;
		}
	}
}


static int decode_F1(const char *opnm, int opsize1, int opsize2, unsigned ipc, unsigned pc, char *out)
{
	unsigned char code = readop(pc);
	sprintf(out, "%s ", opnm);
	if(code & 0x20) {
		int ret = decode_AM(ipc, pc+1, code & 0x40, opsize1, out + strlen(out)) + 2;
		strcat(out, ", ");
		out_AM_Register(code & 0x1f, out + strlen(out));
		return ret;
	} else {
		out_AM_Register(code & 0x1f, out + strlen(out));
		strcat(out, ", ");
		return decode_AM(ipc, pc+1, code & 0x40, opsize1, out + strlen(out)) + 2;
	}
}

static int decode_F2(const char *opnm, int opsize1, int opsize2, unsigned ipc, unsigned pc, char *out)
{
	int ret;
	unsigned char code = readop(pc);
	sprintf(out, "%s ", opnm);
	ret = decode_AM(ipc, pc+1, code & 0x40, opsize1, out + strlen(out));
	strcat(out, ", ");
	ret += decode_AM(ipc, pc+1+ret, code & 0x20, opsize2, out + strlen(out));
	return ret+2;
}

static int decode_F1F2(const char *opnm, int opsize1, int opsize2, unsigned ipc, unsigned pc, char *out)
{
	if(readop(pc) & 0x80)
		return decode_F2(opnm, opsize1, opsize2, ipc, pc, out);
	else
		return decode_F1(opnm, opsize1, opsize2, ipc, pc, out);
}

static int decode_F3(const char *opnm, int opsize1, int opsize2, unsigned ipc, unsigned pc, char *out)
{
	sprintf(out, "%s ", opnm);
	return decode_AM(ipc, pc, readop(pc-1) & 1, opsize1, out + strlen(out)) + 1;
}

static int decode_F4a(const char *opnm, int opsize1, int opsize2, unsigned ipc, unsigned pc, char *out)
{
	sprintf(out, "%s %X", opnm, ipc+read8(pc));
	return 2;
}

static int decode_F4b(const char *opnm, int opsize1, int opsize2, unsigned ipc, unsigned pc, char *out)
{
	sprintf(out, "%s %X", opnm, ipc+read16(pc));
	return 3;
}

static int decode_F5(const char *opnm, int opsize1, int opsize2, unsigned ipc, unsigned pc, char *out)
{
	strcpy(out, opnm);
	return 1;
}

static int decode_F6(const char *opnm, int opsize1, int opsize2, unsigned ipc, unsigned pc, char *out)
{
	sprintf(out, "%s %s, %X[PC]", opnm, v60_reg_names[readop(pc) & 0x1f], ipc+read16(pc+1));
	return 4;
}

static int decode_F7a(const char *opnm, int opsize1, int opsize2, unsigned ipc, unsigned pc, char *out)
{
	int ret;
	unsigned char code = readop(pc);
	unsigned char code2;

	sprintf(out, "%s ", opnm);
	ret = decode_AM(ipc, pc+1, code & 0x40, opsize1, out + strlen(out));
	strcat(out, ", ");

	code2 = readop(pc+1+ret);
	if(code2 & 0x80)
		out_AM_Register(code2 & 0x1f, out + strlen(out));
	else
		out_AM_Immediate(code2, 1, out + strlen(out));
	strcat(out, ", ");

	ret += decode_AM(ipc, pc+2+ret, code & 0x20, opsize2, out + strlen(out));
	strcat(out, ", ");

	code2 = readop(pc+2+ret);
	if(code2 & 0x80)
		out_AM_Register(code2 & 0x1f, out + strlen(out));
	else
		out_AM_Immediate(code2, 1, out + strlen(out));

	return ret+4;
}

static int decode_F7b(const char *opnm, int opsize1, int opsize2, unsigned ipc, unsigned pc, char *out)
{
	int ret;
	unsigned char code = readop(pc);
	unsigned char code2;

	sprintf(out, "%s ", opnm);
	ret = decode_AM(ipc, pc+1, code & 0x40, opsize1, out + strlen(out));
	strcat(out, ", ");

	code2 = readop(pc+1+ret);
	if(code2 & 0x80)
		out_AM_Register(code2 & 0x1f, out + strlen(out));
	else
		out_AM_Immediate(code2, 1, out + strlen(out));
	strcat(out, ", ");

	ret += decode_AM(ipc, pc+2+ret, code & 0x20, opsize2, out + strlen(out));

	return ret+3;
}

static int decode_F7c(const char *opnm, int opsize1, int opsize2, unsigned ipc, unsigned pc, char *out)
{
	int ret;
	unsigned char code = readop(pc);
	unsigned char code2;

	sprintf(out, "%s ", opnm);
	ret = decode_AM(ipc, pc+1, code & 0x40, opsize1, out + strlen(out));
	strcat(out, ", ");

	ret += decode_AM(ipc, pc+1+ret, code & 0x20, opsize2, out + strlen(out));
	strcat(out, ", ");

	code2 = readop(pc+1+ret);
	if(code2 & 0x80)
		out_AM_Register(code2 & 0x1f, out + strlen(out));
	else
		out_AM_Immediate(code2, 1, out + strlen(out));

	return ret+3;
}

static int (*dasm_optable[256])(unsigned ipc, unsigned pc, char *out);
static int (*dasm_optable_58[64])(unsigned ipc, unsigned pc, char *out);
static int (*dasm_optable_5A[64])(unsigned ipc, unsigned pc, char *out);
static int (*dasm_optable_5B[64])(unsigned ipc, unsigned pc, char *out);
static int (*dasm_optable_5C[64])(unsigned ipc, unsigned pc, char *out);
static int (*dasm_optable_5D[64])(unsigned ipc, unsigned pc, char *out);
static int (*dasm_optable_5E[64])(unsigned ipc, unsigned pc, char *out);
static int (*dasm_optable_5F[64])(unsigned ipc, unsigned pc, char *out);
static int (*dasm_optable_C6[64])(unsigned ipc, unsigned pc, char *out);
static int (*dasm_optable_C7[64])(unsigned ipc, unsigned pc, char *out);

static int dopUNHANDLED(unsigned ipc, unsigned pc, char *out)
{
	sprintf(out, "$%02X", readop(pc));
	return 1;
}

static int dop58UNHANDLED(unsigned ipc, unsigned pc, char *out)
{
	sprintf(out, "$58");
	return 1;
}

static int dop5AUNHANDLED(unsigned ipc, unsigned pc, char *out)
{
	sprintf(out, "$5A");
	return 1;
}

static int dop5BUNHANDLED(unsigned ipc, unsigned pc, char *out)
{
	sprintf(out, "$5B");
	return 1;
}

static int dop5CUNHANDLED(unsigned ipc, unsigned pc, char *out)
{
	sprintf(out, "$5C");
	return 1;
}

static int dop5DUNHANDLED(unsigned ipc, unsigned pc, char *out)
{
	sprintf(out, "$5D");
	return 1;
}

static int dop5EUNHANDLED(unsigned ipc, unsigned pc, char *out)
{
	sprintf(out, "$5E");
	return 1;
}

static int dop5FUNHANDLED(unsigned ipc, unsigned pc, char *out)
{
	sprintf(out, "$5F");
	return 1;
}

static int dopC6UNHANDLED(unsigned ipc, unsigned pc, char *out)
{
	sprintf(out, "$C6");
	return 1;
}

static int dopC7UNHANDLED(unsigned ipc, unsigned pc, char *out)
{
	sprintf(out, "$C7");
	return 1;
}

static int dop58(unsigned ipc, unsigned pc, char *out)
{
	return dasm_optable_58[readop(pc) & 0x1f](ipc, pc, out);
}

static int dop5A(unsigned ipc, unsigned pc, char *out)
{
	return dasm_optable_5A[readop(pc) & 0x1f](ipc, pc, out);
}

static int dop5B(unsigned ipc, unsigned pc, char *out)
{
	return dasm_optable_5B[readop(pc) & 0x1f](ipc, pc, out);
}

static int dop5C(unsigned ipc, unsigned pc, char *out)
{
	return dasm_optable_5C[readop(pc) & 0x1f](ipc, pc, out);
}

static int dop5D(unsigned ipc, unsigned pc, char *out)
{
	return dasm_optable_5D[readop(pc) & 0x1f](ipc, pc, out);
}

static int dop5E(unsigned ipc, unsigned pc, char *out)
{
	return dasm_optable_5E[readop(pc) & 0x1f](ipc, pc, out);
}

static int dop5F(unsigned ipc, unsigned pc, char *out)
{
	return dasm_optable_5F[readop(pc) & 0x1f](ipc, pc, out);
}

static int dopC6(unsigned ipc, unsigned pc, char *out)
{
	return dasm_optable_C6[readop(pc) >> 5](ipc, pc, out);
}

static int dopC7(unsigned ipc, unsigned pc, char *out)
{
	return dasm_optable_C7[readop(pc) >> 5](ipc, pc, out);
}

#define DEFINE_EASY_OPCODE(name, opnm, ftype, opsize1, opsize2) \
	static int dop ## name(unsigned ipc, unsigned pc, char *out) \
	{ \
		return decode_ ## ftype(opnm, opsize1, opsize2, ipc, pc, out); \
	}

#define DEFINE_TRIPLE_OPCODE(name, string, ftype) \
	DEFINE_EASY_OPCODE(name##B,string ".b", ftype, 0, 0) \
	DEFINE_EASY_OPCODE(name##H,string ".h", ftype, 1, 1) \
	DEFINE_EASY_OPCODE(name##W,string ".w", ftype, 2, 2)

#define DEFINE_DOUBLE_OPCODE(name, string, ftype) \
	DEFINE_EASY_OPCODE(name##B,string ".b", ftype, 0, 0) \
	DEFINE_EASY_OPCODE(name##H,string ".h", ftype, 1, 1)

#define DEFINE_FPU_OPCODE(name,string,ftype) \
	DEFINE_EASY_OPCODE(name##S,string ".s", ftype, 2, 2) \
	DEFINE_EASY_OPCODE(name##L,string ".l", ftype, 2, 2)


DEFINE_FPU_OPCODE(ABSF, "absf", F2)
DEFINE_TRIPLE_OPCODE(ADD, "add", F1F2)
DEFINE_TRIPLE_OPCODE(ADDC, "addc", F1F2)
DEFINE_FPU_OPCODE(ADDF, "addf", F2)
DEFINE_TRIPLE_OPCODE(AND, "and", F1F2)
DEFINE_EASY_OPCODE(ANDBSU, "andbsu", F7b, 0x80, 0x80)
DEFINE_EASY_OPCODE(ANDBSD, "andbsd", F7b, 0x80, 0x80)
DEFINE_EASY_OPCODE(ANDNBSU, "andnbsu", F7b, 0x80, 0x80)
DEFINE_EASY_OPCODE(ANDNBSD, "andnbsd", F7b, 0x80, 0x80)
DEFINE_EASY_OPCODE(BGT8, "bgt", F4a, 0, 0)
DEFINE_EASY_OPCODE(BGT16, "bgt", F4b, 0, 0)
DEFINE_EASY_OPCODE(BGE8, "bge", F4a, 0, 0)
DEFINE_EASY_OPCODE(BGE16, "bge", F4b, 0, 0)
DEFINE_EASY_OPCODE(BLT8, "blt", F4a, 0, 0)
DEFINE_EASY_OPCODE(BLT16, "blt", F4b, 0, 0)
DEFINE_EASY_OPCODE(BLE8, "ble", F4a, 0, 0)
DEFINE_EASY_OPCODE(BLE16, "ble", F4b, 0, 0)
DEFINE_EASY_OPCODE(BH8, "bh", F4a, 0, 0)
DEFINE_EASY_OPCODE(BH16, "bh", F4b, 0, 0)
DEFINE_EASY_OPCODE(BNL8, "bnl", F4a, 0, 0)
DEFINE_EASY_OPCODE(BNL16, "bnl", F4b, 0, 0)
DEFINE_EASY_OPCODE(BL8, "bl", F4a, 0, 0)
DEFINE_EASY_OPCODE(BL16, "bl", F4b, 0, 0)
DEFINE_EASY_OPCODE(BNH8, "bnh", F4a, 0, 0)
DEFINE_EASY_OPCODE(BNH16, "bnh", F4b, 0, 0)
DEFINE_EASY_OPCODE(BE8, "be", F4a, 0, 0)
DEFINE_EASY_OPCODE(BE16, "be", F4b, 0, 0)
DEFINE_EASY_OPCODE(BNE8, "bne", F4a, 0, 0)
DEFINE_EASY_OPCODE(BNE16, "bne", F4b, 0, 0)
DEFINE_EASY_OPCODE(BV8, "bv", F4a, 0, 0)
DEFINE_EASY_OPCODE(BV16, "bv", F4b, 0, 0)
DEFINE_EASY_OPCODE(BNV8, "bnv", F4a, 0, 0)
DEFINE_EASY_OPCODE(BNV16, "bnv", F4b, 0, 0)
DEFINE_EASY_OPCODE(BN8, "bn", F4a, 0, 0)
DEFINE_EASY_OPCODE(BN16, "bn", F4b, 0, 0)
DEFINE_EASY_OPCODE(BP8, "bp", F4a, 0, 0)
DEFINE_EASY_OPCODE(BP16, "bp", F4b, 0, 0)
DEFINE_EASY_OPCODE(BR8, "br", F4a, 0, 0)
DEFINE_EASY_OPCODE(BR16, "br", F4b, 0, 0)
DEFINE_EASY_OPCODE(BRK, "brk", F5, 0, 0)
DEFINE_EASY_OPCODE(BRKV, "brkv", F5, 0, 0)
DEFINE_EASY_OPCODE(BSR, "bsr", F4b, 0, 0)
DEFINE_EASY_OPCODE(CALL, "call", F1F2, 0, 2)
DEFINE_EASY_OPCODE(CAXI, "caxi", F1, 2, 2)
DEFINE_EASY_OPCODE(CHKAR, "chkar", F1F2, 0, 0) /* ?*/
DEFINE_EASY_OPCODE(CHKAW, "chkaw", F1F2, 0, 0) /* ?*/
DEFINE_EASY_OPCODE(CHKAE, "chkae", F1F2, 0, 0)
DEFINE_EASY_OPCODE(CHLVL, "chlvl", F1F2, 0, 0)
DEFINE_EASY_OPCODE(CLR1, "clr1", F1F2, 2, 2)
DEFINE_EASY_OPCODE(CLRTLB, "clrtlb", F3, 0, 0) /* ?*/
DEFINE_EASY_OPCODE(CLRTLBA, "clrtlba", F5, 0, 0)
DEFINE_TRIPLE_OPCODE(CMP, "cmp", F1F2)
DEFINE_EASY_OPCODE(CMPBFS, "cmpbfs", F7b, 0x82, 2)
DEFINE_EASY_OPCODE(CMPBFZ, "cmpbfz", F7b, 0x82, 2)
DEFINE_EASY_OPCODE(CMPBFL, "cmpbfl", F7b, 0x82, 2)
DEFINE_DOUBLE_OPCODE(CMPC, "cmpc", F7a)
DEFINE_DOUBLE_OPCODE(CMPCF, "cmpcf", F7a)
DEFINE_DOUBLE_OPCODE(CMPCS, "cmpcs", F7a)
DEFINE_FPU_OPCODE(CMPF, "cmpf", F2)
DEFINE_EASY_OPCODE(CVTSL, "cvt.sl", F2, 0, 1)
DEFINE_EASY_OPCODE(CVTLS, "cvt.ls", F2, 1, 0)
DEFINE_EASY_OPCODE(CVTWS, "cvt.ws", F2, 2, 0)
DEFINE_EASY_OPCODE(CVTWL, "cvt.wl", F2, 2, 1)
DEFINE_EASY_OPCODE(CVTSW, "cvt.sw", F2, 0, 2)
DEFINE_EASY_OPCODE(CVTLW, "cvt.lw", F2, 1, 2)
DEFINE_EASY_OPCODE(DBGT, "dbgt", F6, 0, 0)
DEFINE_EASY_OPCODE(DBGE, "dbge", F6, 0, 0)
DEFINE_EASY_OPCODE(DBLT, "dbgt", F6, 0, 0)
DEFINE_EASY_OPCODE(DBLE, "dbge", F6, 0, 0)
DEFINE_EASY_OPCODE(DBH, "dbh", F6, 0, 0)
DEFINE_EASY_OPCODE(DBNL, "dbnl", F6, 0, 0)
DEFINE_EASY_OPCODE(DBL, "dbl", F6, 0, 0)
DEFINE_EASY_OPCODE(DBNH, "dbnh", F6, 0, 0)
DEFINE_EASY_OPCODE(DBE, "dbe", F6, 0, 0)
DEFINE_EASY_OPCODE(DBNE, "dbne", F6, 0, 0)
DEFINE_EASY_OPCODE(DBV, "dbe", F6, 0, 0)
DEFINE_EASY_OPCODE(DBNV, "dbne", F6, 0, 0)
DEFINE_EASY_OPCODE(DBN, "dbn", F6, 0, 0)
DEFINE_EASY_OPCODE(DBP, "dbp", F6, 0, 0)
DEFINE_EASY_OPCODE(DBR, "dbr", F6, 0, 0)
DEFINE_TRIPLE_OPCODE(DEC, "dec", F3)
DEFINE_EASY_OPCODE(DISPOSE, "dispose", F5, 0, 0)
DEFINE_TRIPLE_OPCODE(DIV, "div", F1F2)
DEFINE_FPU_OPCODE(DIVF, "divf", F2)
DEFINE_TRIPLE_OPCODE(DIVU, "divu", F1F2)
DEFINE_EASY_OPCODE(DIVX, "divx", F1F2, 2, 3)
DEFINE_EASY_OPCODE(DIVUX, "divux", F1F2, 2, 3)
DEFINE_EASY_OPCODE(EXTBFS, "extbfs", F7b, 0x82, 2)
DEFINE_EASY_OPCODE(EXTBFZ, "extbfz", F7b, 0x82, 2)
DEFINE_EASY_OPCODE(EXTBFL, "extbfl", F7b, 0x82, 2)
DEFINE_EASY_OPCODE(GETATE, "getate", F1F2, 0, 3) /* ?*/
DEFINE_EASY_OPCODE(GETPSW, "getpsw", F3, 2, 0)
DEFINE_EASY_OPCODE(GETPTE, "getpte", F1F2, 0, 2) /* ?*/
DEFINE_EASY_OPCODE(GETRA, "getra", F1F2, 0, 2) /* ?*/
DEFINE_EASY_OPCODE(HALT, "halt", F5, 0, 0)
DEFINE_TRIPLE_OPCODE(IN, "in", F1F2)
DEFINE_TRIPLE_OPCODE(INC, "inc", F3)
DEFINE_EASY_OPCODE(INSBFL, "insbfl", F7c, 2, 0x82)
DEFINE_EASY_OPCODE(INSBFR, "insbfr", F7c, 2, 0x82)
DEFINE_EASY_OPCODE(JMP, "jmp", F3, 0, 0)
DEFINE_EASY_OPCODE(JSR, "jsr", F3, 0, 0)
DEFINE_EASY_OPCODE(LDPR, "ldpr", F1F2, 2, 2)
DEFINE_EASY_OPCODE(LDTASK, "ldtask", F1F2, 2, 2)
DEFINE_TRIPLE_OPCODE(MOV, "mov", F1F2)
DEFINE_EASY_OPCODE(MOVBSU, "movbsu", F7b, 0x80, 0x80)
DEFINE_EASY_OPCODE(MOVBSD, "movbsd", F7b, 0x80, 0x80)
DEFINE_DOUBLE_OPCODE(MOVCU, "movcu", F7a)
DEFINE_DOUBLE_OPCODE(MOVCD, "movcd", F7a)
DEFINE_DOUBLE_OPCODE(MOVCFU, "movcfu", F7a)
DEFINE_DOUBLE_OPCODE(MOVCFD, "movcfd", F7a)
DEFINE_DOUBLE_OPCODE(MOVCS, "movcs", F7a)
DEFINE_EASY_OPCODE(MOVD, "mov.d", F1F2, 3, 3)
DEFINE_EASY_OPCODE(MOVEAB, "movea.b", F1F2, 0, 2)
DEFINE_EASY_OPCODE(MOVEAH, "movea.h", F1F2, 1, 2)
DEFINE_EASY_OPCODE(MOVEAW, "movea.w", F1F2, 2, 2)
DEFINE_FPU_OPCODE(MOVF, "movf", F2)
DEFINE_EASY_OPCODE(MOVSBH, "movs.bh", F1F2, 0, 1)
DEFINE_EASY_OPCODE(MOVSBW, "movs.bw", F1F2, 0, 2)
DEFINE_EASY_OPCODE(MOVSHW, "movs.hw", F1F2, 1, 2)
DEFINE_EASY_OPCODE(MOVTHB, "movt.hb", F1F2, 1, 0)
DEFINE_EASY_OPCODE(MOVTWB, "movt.wb", F1F2, 2, 0)
DEFINE_EASY_OPCODE(MOVTWH, "movt.wh", F1F2, 2, 1)
DEFINE_EASY_OPCODE(MOVZBH, "movz.bh", F1F2, 0, 1)
DEFINE_EASY_OPCODE(MOVZBW, "movz.bw", F1F2, 0, 2)
DEFINE_EASY_OPCODE(MOVZHW, "movz.hw", F1F2, 1, 2)
DEFINE_TRIPLE_OPCODE(MUL, "mul", F1F2)
DEFINE_FPU_OPCODE(MULF, "mulf", F2)
DEFINE_TRIPLE_OPCODE(MULU, "mulu", F1F2)
DEFINE_EASY_OPCODE(MULX, "mulx", F1F2, 2, 3)
DEFINE_EASY_OPCODE(MULUX, "mulux", F1F2, 2, 3)
DEFINE_TRIPLE_OPCODE(NEG, "neg", F1F2)
DEFINE_FPU_OPCODE(NEGF, "negf", F2)
DEFINE_EASY_OPCODE(NOP, "nop", F5, 0, 0)
DEFINE_TRIPLE_OPCODE(NOT, "not", F1F2)
DEFINE_EASY_OPCODE(NOT1, "not1", F1F2, 2, 2)
DEFINE_EASY_OPCODE(NOTBSU, "notbsu", F7b, 0x80, 0x80)
DEFINE_EASY_OPCODE(NOTBSD, "notbsd", F7b, 0x80, 0x80)
DEFINE_TRIPLE_OPCODE(OR, "or", F1F2)
DEFINE_EASY_OPCODE(ORBSU, "orbsu", F7b, 0x80, 0x80)
DEFINE_EASY_OPCODE(ORBSD, "orbsd", F7b, 0x80, 0x80)
DEFINE_EASY_OPCODE(ORNBSU, "ornbsu", F7b, 0x80, 0x80)
DEFINE_EASY_OPCODE(ORNBSD, "ornbsd", F7b, 0x80, 0x80)
DEFINE_TRIPLE_OPCODE(OUT, "out", F1F2)
DEFINE_EASY_OPCODE(POP, "pop", F3, 2, 0)
DEFINE_EASY_OPCODE(POPM, "popm", F3, 2, 0)
DEFINE_EASY_OPCODE(PREPARE, "prepare", F3, 2, 0)
DEFINE_EASY_OPCODE(PUSH, "push", F3, 2, 0)
DEFINE_EASY_OPCODE(PUSHM, "pushm", F3, 2, 0)
DEFINE_TRIPLE_OPCODE(REM, "rem", F1F2)
DEFINE_TRIPLE_OPCODE(REMU, "remu", F1F2)
DEFINE_EASY_OPCODE(RET, "ret", F3, 2, 0)
DEFINE_EASY_OPCODE(RETIU, "retiu", F3, 1, 0)
DEFINE_EASY_OPCODE(RETIS, "retis", F3, 1, 0)
DEFINE_EASY_OPCODE(ROTB, "rot.b", F1F2, 0, 0)
DEFINE_EASY_OPCODE(ROTH, "rot.h", F1F2, 0, 1)
DEFINE_EASY_OPCODE(ROTW, "rot.w", F1F2, 0, 2)
DEFINE_EASY_OPCODE(ROTCB, "rotc.b", F1F2, 0, 0)
DEFINE_EASY_OPCODE(ROTCH, "rotc.h", F1F2, 0, 1)
DEFINE_EASY_OPCODE(ROTCW, "rotc.w", F1F2, 0, 2)
DEFINE_EASY_OPCODE(RSR, "rsr", F5, 0, 0)
DEFINE_EASY_OPCODE(RVBIT, "rvbit", F1F2, 0, 0)
DEFINE_EASY_OPCODE(RVBYT, "rvbyt", F1F2, 2, 2)
DEFINE_EASY_OPCODE(SCH0BSU, "sch0bsu", F7b, 0x80, 2)
DEFINE_EASY_OPCODE(SCH0BSD, "sch0bsd", F7b, 0x80, 2)
DEFINE_EASY_OPCODE(SCH1BSU, "sch1bsu", F7b, 0x80, 2)
DEFINE_EASY_OPCODE(SCH1BSD, "sch1bsd", F7b, 0x80, 2)
DEFINE_EASY_OPCODE(SCHCUB, "schcu.b", F7b, 0, 0)
DEFINE_EASY_OPCODE(SCHCDB, "schcd.b", F7b, 0, 0)
DEFINE_EASY_OPCODE(SCHCUH, "schcu.h", F7b, 1, 1)
DEFINE_EASY_OPCODE(SCHCDH, "schcd.h", F7b, 1, 1)
DEFINE_EASY_OPCODE(SCLFS, "sclf.s", F2, 1, 2)
DEFINE_EASY_OPCODE(SCLFL, "sclf.l", F2, 1, 2)
DEFINE_EASY_OPCODE(SET1, "set1", F1F2, 2, 2)
DEFINE_EASY_OPCODE(SETF, "setf", F1F2, 0, 0)
DEFINE_EASY_OPCODE(SHAB, "sha.b", F1F2, 0, 0)
DEFINE_EASY_OPCODE(SHAH, "sha.h", F1F2, 0, 1)
DEFINE_EASY_OPCODE(SHAW, "sha.w", F1F2, 0, 2)
DEFINE_EASY_OPCODE(SHLB, "shl.b", F1F2, 0, 0)
DEFINE_EASY_OPCODE(SHLH, "shl.h", F1F2, 0, 1)
DEFINE_EASY_OPCODE(SHLW, "shl.w", F1F2, 0, 2)
DEFINE_EASY_OPCODE(SKPCUB, "skpcu.b", F7b, 0, 0)
DEFINE_EASY_OPCODE(SKPCDB, "skpcd.b", F7b, 0, 0)
DEFINE_EASY_OPCODE(SKPCUH, "skpcu.h", F7b, 1, 1)
DEFINE_EASY_OPCODE(SKPCDH, "skpcd.h", F7b, 1, 1)
DEFINE_EASY_OPCODE(STPR, "stpr", F1F2, 2, 2)
DEFINE_EASY_OPCODE(STTASK, "sttask", F3, 2, 0)
DEFINE_TRIPLE_OPCODE(SUB, "sub", F1F2)
DEFINE_TRIPLE_OPCODE(SUBC, "subc", F1F2)
DEFINE_FPU_OPCODE(SUBF, "subf", F2)
DEFINE_EASY_OPCODE(TASI, "tasi", F3, 0, 0)
DEFINE_EASY_OPCODE(TB, "tb", F6, 0, 0)
DEFINE_TRIPLE_OPCODE(TEST, "test", F3)
DEFINE_EASY_OPCODE(TEST1, "test1", F1F2, 2, 2)
DEFINE_EASY_OPCODE(TRAP, "trap", F3, 0, 0)
DEFINE_EASY_OPCODE(TRAPFL, "trapfl", F5, 0, 0)
DEFINE_EASY_OPCODE(UPDATE, "update", F1F2, 0, 3) /* ?*/
DEFINE_EASY_OPCODE(UPDPSWH, "updpsw.h", F1F2, 2, 2)
DEFINE_EASY_OPCODE(UPDPSWW, "updpsw.w", F1F2, 2, 2)
DEFINE_EASY_OPCODE(UPDPTE, "updpte", F1F2, 0, 2) /* ?*/
DEFINE_TRIPLE_OPCODE(XCH, "xch", F1)
DEFINE_TRIPLE_OPCODE(XOR, "xor", F1F2)
DEFINE_EASY_OPCODE(XORBSU, "xorbsu", F7b, 0x80, 0x80)
DEFINE_EASY_OPCODE(XORBSD, "xorbsd", F7b, 0x80, 0x80)
DEFINE_EASY_OPCODE(XORNBSU, "xornbsu", F7b, 0x80, 0x80)
DEFINE_EASY_OPCODE(XORNBSD, "xornbsd", F7b, 0x80, 0x80)

void v60_dasm_init(void)
{
	int i;
	for (i=0;i<256;i++)
		dasm_optable[i] = dopUNHANDLED;

	for (i=0; i<64; i++)
	{
		dasm_optable_58[i] = dop58UNHANDLED;
		dasm_optable_5A[i] = dop5AUNHANDLED;
		dasm_optable_5B[i] = dop5BUNHANDLED;
		dasm_optable_5C[i] = dop5CUNHANDLED;
		dasm_optable_5D[i] = dop5DUNHANDLED;
		dasm_optable_5E[i] = dop5EUNHANDLED;
		dasm_optable_5F[i] = dop5FUNHANDLED;
		dasm_optable_C6[i] = dopC6UNHANDLED;
		dasm_optable_C7[i] = dopC7UNHANDLED;
	}

	dasm_optable_58[0x00] = dopCMPCB;
	dasm_optable_5A[0x00] = dopCMPCH;
	dasm_optable_58[0x01] = dopCMPCFB;
	dasm_optable_5A[0x01] = dopCMPCFH;
	dasm_optable_58[0x02] = dopCMPCSB;
	dasm_optable_5A[0x02] = dopCMPCSH;
	dasm_optable_58[0x08] = dopMOVCUB;
	dasm_optable_5A[0x08] = dopMOVCUH;
	dasm_optable_58[0x09] = dopMOVCDB;
	dasm_optable_5A[0x09] = dopMOVCDH;
	dasm_optable_58[0x0A] = dopMOVCFUB;
	dasm_optable_5A[0x0A] = dopMOVCFUH;
	dasm_optable_58[0x0B] = dopMOVCFDB;
	dasm_optable_5A[0x0B] = dopMOVCFDH;
	dasm_optable_58[0x0C] = dopMOVCSB;
	dasm_optable_5A[0x0C] = dopMOVCSH;
	dasm_optable_58[0x18] = dopSCHCUB;
	dasm_optable_5A[0x18] = dopSCHCUH;
	dasm_optable_58[0x19] = dopSCHCDB;
	dasm_optable_5A[0x19] = dopSCHCDH;
	dasm_optable_58[0x1A] = dopSKPCUB;
	dasm_optable_5A[0x1A] = dopSKPCUH;
	dasm_optable_58[0x1B] = dopSKPCDB;
	dasm_optable_5A[0x1B] = dopSKPCDH;

	dasm_optable_5C[0x00] = dopCMPFS;
	dasm_optable_5E[0x00] = dopCMPFL;
	dasm_optable_5C[0x08] = dopMOVFS;
	dasm_optable_5E[0x08] = dopMOVFL;
	dasm_optable_5C[0x09] = dopNEGFS;
	dasm_optable_5E[0x09] = dopNEGFL;
	dasm_optable_5C[0x0A] = dopABSFS;
	dasm_optable_5E[0x0A] = dopABSFL;
	dasm_optable_5C[0x10] = dopSCLFS;
	dasm_optable_5E[0x10] = dopSCLFL;
	dasm_optable_5C[0x18] = dopADDFS;
	dasm_optable_5E[0x18] = dopADDFL;
	dasm_optable_5C[0x19] = dopSUBFS;
	dasm_optable_5E[0x19] = dopSUBFL;
	dasm_optable_5C[0x1A] = dopMULFS;
	dasm_optable_5E[0x1A] = dopMULFL;
	dasm_optable_5C[0x1B] = dopDIVFS;
	dasm_optable_5E[0x1B] = dopDIVFL;

	dasm_optable_5F[0x00] = dopCVTWS;
	dasm_optable_5F[0x01] = dopCVTSW;
	dasm_optable_5F[0x08] = dopCVTLS;
	dasm_optable_5F[0x09] = dopCVTLW;
	dasm_optable_5F[0x10] = dopCVTSL;
	dasm_optable_5F[0x11] = dopCVTWL;

	dasm_optable_5B[0x00] = dopSCH0BSU;
	dasm_optable_5B[0x01] = dopSCH0BSD;
	dasm_optable_5B[0x02] = dopSCH1BSU;
	dasm_optable_5B[0x03] = dopSCH1BSD;
	dasm_optable_5B[0x08] = dopMOVBSU;
	dasm_optable_5B[0x09] = dopMOVBSD;
	dasm_optable_5B[0x0A] = dopNOTBSU;
	dasm_optable_5B[0x0B] = dopNOTBSD;
	dasm_optable_5B[0x10] = dopANDBSU;
	dasm_optable_5B[0x11] = dopANDBSD;
	dasm_optable_5B[0x12] = dopANDNBSU;
	dasm_optable_5B[0x13] = dopANDNBSD;
	dasm_optable_5B[0x14] = dopORBSU;
	dasm_optable_5B[0x15] = dopORBSD;
	dasm_optable_5B[0x16] = dopORNBSU;
	dasm_optable_5B[0x17] = dopORNBSD;
	dasm_optable_5B[0x18] = dopXORBSU;
	dasm_optable_5B[0x19] = dopXORBSD;
	dasm_optable_5B[0x1A] = dopXORNBSU;
	dasm_optable_5B[0x1B] = dopXORNBSD;

	dasm_optable_5D[0x00] = dopCMPBFS;
	dasm_optable_5D[0x01] = dopCMPBFZ;
	dasm_optable_5D[0x02] = dopCMPBFL;
	dasm_optable_5D[0x08] = dopEXTBFS;
	dasm_optable_5D[0x09] = dopEXTBFZ;
	dasm_optable_5D[0x0A] = dopEXTBFL;
	dasm_optable_5D[0x18] = dopINSBFR;
	dasm_optable_5D[0x19] = dopINSBFL;

	dasm_optable_C7[0x7] = dopDBGT;
	dasm_optable_C7[0x6] = dopDBGE;
	dasm_optable_C6[0x6] = dopDBLT;
	dasm_optable_C6[0x7] = dopDBLE;
	dasm_optable_C7[0x3] = dopDBH;
	dasm_optable_C7[0x1] = dopDBNL;
	dasm_optable_C6[0x1] = dopDBL;
	dasm_optable_C6[0x3] = dopDBNH;
	dasm_optable_C6[0x2] = dopDBE;
	dasm_optable_C7[0x2] = dopDBNE;
	dasm_optable_C6[0x0] = dopDBV;
	dasm_optable_C7[0x0] = dopDBNV;
	dasm_optable_C6[0x4] = dopDBN;
	dasm_optable_C7[0x4] = dopDBP;
	dasm_optable_C6[0x5] = dopDBR;
	dasm_optable_C7[0x5] = dopTB;

	dasm_optable[0x00] = dopHALT;
	dasm_optable[0x01] = dopLDTASK;
	dasm_optable[0x02] = dopSTPR;
	dasm_optable[0x03] = dopGETRA;
	dasm_optable[0x04] = dopGETPTE;
	dasm_optable[0x05] = dopGETATE;
	dasm_optable[0x08] = dopRVBIT;
	dasm_optable[0x09] = dopMOVB;
	dasm_optable[0x0A] = dopMOVSBH;
	dasm_optable[0x0B] = dopMOVZBH;
	dasm_optable[0x0C] = dopMOVSBW;
	dasm_optable[0x0D] = dopMOVZBW;
	dasm_optable[0x10] = dopCLRTLBA;
	dasm_optable[0x12] = dopLDPR;
	dasm_optable[0x13] = dopUPDPSWW;
	dasm_optable[0x14] = dopUPDPTE;
	dasm_optable[0x15] = dopUPDATE;
	dasm_optable[0x19] = dopMOVTHB;
	dasm_optable[0x1B] = dopMOVH;
	dasm_optable[0x1C] = dopMOVSHW;
	dasm_optable[0x1D] = dopMOVZHW;
	dasm_optable[0x20] = dopINB;
	dasm_optable[0x21] = dopOUTB;
	dasm_optable[0x22] = dopINH;
	dasm_optable[0x23] = dopOUTH;
	dasm_optable[0x24] = dopINW;
	dasm_optable[0x25] = dopOUTW;
	dasm_optable[0x29] = dopMOVTWB;
	dasm_optable[0x2B] = dopMOVTWH;
	dasm_optable[0x2C] = dopRVBYT;
	dasm_optable[0x2D] = dopMOVW;
	dasm_optable[0x38] = dopNOTB;
	dasm_optable[0x39] = dopNEGB;
	dasm_optable[0x3A] = dopNOTH;
	dasm_optable[0x3B] = dopNEGH;
	dasm_optable[0x3C] = dopNOTW;
	dasm_optable[0x3D] = dopNEGW;
	dasm_optable[0x3F] = dopMOVD;
	dasm_optable[0x40] = dopMOVEAB;
	dasm_optable[0x41] = dopXCHB;
	dasm_optable[0x42] = dopMOVEAH;
	dasm_optable[0x43] = dopXCHH;
	dasm_optable[0x44] = dopMOVEAW;
	dasm_optable[0x45] = dopXCHW;
	dasm_optable[0x47] = dopSETF;
	dasm_optable[0x48] = dopBSR;
	dasm_optable[0x49] = dopCALL;
	dasm_optable[0x4A] = dopUPDPSWH;
	dasm_optable[0x4B] = dopCHLVL;
	dasm_optable[0x4C] = dopCAXI;
	dasm_optable[0x4D] = dopCHKAR;
	dasm_optable[0x4E] = dopCHKAW;
	dasm_optable[0x4F] = dopCHKAE;
	dasm_optable[0x50] = dopREMB;
	dasm_optable[0x51] = dopREMUB;
	dasm_optable[0x52] = dopREMH;
	dasm_optable[0x53] = dopREMUH;
	dasm_optable[0x54] = dopREMW;
	dasm_optable[0x55] = dopREMUW;
	dasm_optable[0x58] = dop58;
	dasm_optable[0x5A] = dop5A;
	dasm_optable[0x5B] = dop5B;
	dasm_optable[0x5C] = dop5C;
	dasm_optable[0x5D] = dop5D;
	dasm_optable[0x5E] = dop5E;
	dasm_optable[0x5F] = dop5F;
	dasm_optable[0x60] = dopBV8;
	dasm_optable[0x61] = dopBNV8;
	dasm_optable[0x62] = dopBL8;
	dasm_optable[0x63] = dopBNL8;
	dasm_optable[0x64] = dopBE8;
	dasm_optable[0x65] = dopBNE8;
	dasm_optable[0x66] = dopBNH8;
	dasm_optable[0x67] = dopBH8;
	dasm_optable[0x68] = dopBN8;
	dasm_optable[0x69] = dopBP8;
	dasm_optable[0x6A] = dopBR8;
	dasm_optable[0x6C] = dopBLT8;
	dasm_optable[0x6D] = dopBGE8;
	dasm_optable[0x6E] = dopBLE8;
	dasm_optable[0x6F] = dopBGT8;
	dasm_optable[0x70] = dopBV16;
	dasm_optable[0x71] = dopBNV16;
	dasm_optable[0x72] = dopBL16;
	dasm_optable[0x73] = dopBNL16;
	dasm_optable[0x74] = dopBE16;
	dasm_optable[0x75] = dopBNE16;
	dasm_optable[0x76] = dopBNH16;
	dasm_optable[0x77] = dopBH16;
	dasm_optable[0x78] = dopBN16;
	dasm_optable[0x79] = dopBP16;
	dasm_optable[0x7A] = dopBR16;
	dasm_optable[0x7C] = dopBLT16;
	dasm_optable[0x7D] = dopBGE16;
	dasm_optable[0x7E] = dopBLE16;
	dasm_optable[0x7F] = dopBGT16;
	dasm_optable[0x80] = dopADDB;
	dasm_optable[0x81] = dopMULB;
	dasm_optable[0x82] = dopADDH;
	dasm_optable[0x83] = dopMULH;
	dasm_optable[0x84] = dopADDW;
	dasm_optable[0x85] = dopMULW;
	dasm_optable[0x86] = dopMULX;
	dasm_optable[0x87] = dopTEST1;
	dasm_optable[0x88] = dopORB;
	dasm_optable[0x89] = dopROTB;
	dasm_optable[0x8A] = dopORH;
	dasm_optable[0x8B] = dopROTH;
	dasm_optable[0x8C] = dopORW;
	dasm_optable[0x8D] = dopROTW;
	dasm_optable[0x90] = dopADDCB;
	dasm_optable[0x91] = dopMULUB;
	dasm_optable[0x92] = dopADDCH;
	dasm_optable[0x93] = dopMULUH;
	dasm_optable[0x94] = dopADDCW;
	dasm_optable[0x95] = dopMULUW;
	dasm_optable[0x96] = dopMULUX;
	dasm_optable[0x97] = dopSET1;
	dasm_optable[0x98] = dopSUBCB;
	dasm_optable[0x99] = dopROTCB;
	dasm_optable[0x9A] = dopSUBCH;
	dasm_optable[0x9B] = dopROTCH;
	dasm_optable[0x9C] = dopSUBCW;
	dasm_optable[0x9D] = dopROTCW;
	dasm_optable[0xA0] = dopANDB;
	dasm_optable[0xA1] = dopDIVB;
	dasm_optable[0xA2] = dopANDH;
	dasm_optable[0xA3] = dopDIVH;
	dasm_optable[0xA4] = dopANDW;
	dasm_optable[0xA5] = dopDIVW;
	dasm_optable[0xA6] = dopDIVX;
	dasm_optable[0xA7] = dopCLR1;
	dasm_optable[0xA8] = dopSUBB;
	dasm_optable[0xA9] = dopSHLB;
	dasm_optable[0xAA] = dopSUBH;
	dasm_optable[0xAB] = dopSHLH;
	dasm_optable[0xAC] = dopSUBW;
	dasm_optable[0xAD] = dopSHLW;
	dasm_optable[0xB0] = dopXORB;
	dasm_optable[0xB1] = dopDIVUB;
	dasm_optable[0xB2] = dopXORH;
	dasm_optable[0xB3] = dopDIVUH;
	dasm_optable[0xB4] = dopXORW;
	dasm_optable[0xB5] = dopDIVUW;
	dasm_optable[0xB6] = dopDIVUX;
	dasm_optable[0xB7] = dopNOT1;
	dasm_optable[0xB8] = dopCMPB;
	dasm_optable[0xB9] = dopSHAB;
	dasm_optable[0xBA] = dopCMPH;
	dasm_optable[0xBB] = dopSHAH;
	dasm_optable[0xBC] = dopCMPW;
	dasm_optable[0xBD] = dopSHAW;
	dasm_optable[0xC6] = dopC6;
	dasm_optable[0xC7] = dopC7;
	dasm_optable[0xC8] = dopBRK;
	dasm_optable[0xC9] = dopBRKV;
	dasm_optable[0xCA] = dopRSR;
	dasm_optable[0xCB] = dopTRAPFL;
	dasm_optable[0xCC] = dopDISPOSE;
	dasm_optable[0xCD] = dopNOP;
	dasm_optable[0xD0] = dopDECB;
	dasm_optable[0xD1] = dopDECB;
	dasm_optable[0xD2] = dopDECH;
	dasm_optable[0xD3] = dopDECH;
	dasm_optable[0xD4] = dopDECW;
	dasm_optable[0xD5] = dopDECW;
	dasm_optable[0xD6] = dopJMP;
	dasm_optable[0xD7] = dopJMP;
	dasm_optable[0xD8] = dopINCB;
	dasm_optable[0xD9] = dopINCB;
	dasm_optable[0xDA] = dopINCH;
	dasm_optable[0xDB] = dopINCH;
	dasm_optable[0xDC] = dopINCW;
	dasm_optable[0xDD] = dopINCW;
	dasm_optable[0xDE] = dopPREPARE;
	dasm_optable[0xDF] = dopPREPARE;
	dasm_optable[0xE0] = dopTASI;
	dasm_optable[0xE1] = dopTASI;
	dasm_optable[0xE2] = dopRET;
	dasm_optable[0xE3] = dopRET;
	dasm_optable[0xE4] = dopPOPM;
	dasm_optable[0xE5] = dopPOPM;
	dasm_optable[0xE6] = dopPOP;
	dasm_optable[0xE7] = dopPOP;
	dasm_optable[0xE8] = dopJSR;
	dasm_optable[0xE9] = dopJSR;
	dasm_optable[0xEA] = dopRETIU;
	dasm_optable[0xEB] = dopRETIU;
	dasm_optable[0xEC] = dopPUSHM;
	dasm_optable[0xED] = dopPUSHM;
	dasm_optable[0xEE] = dopPUSH;
	dasm_optable[0xEF] = dopPUSH;
	dasm_optable[0xF0] = dopTESTB;
	dasm_optable[0xF1] = dopTESTB;
	dasm_optable[0xF2] = dopTESTH;
	dasm_optable[0xF3] = dopTESTH;
	dasm_optable[0xF4] = dopTESTW;
	dasm_optable[0xF5] = dopTESTW;
	dasm_optable[0xFA] = dopRETIS;
	dasm_optable[0xFB] = dopRETIS;
	dasm_optable[0xF6] = dopGETPSW;
	dasm_optable[0xF7] = dopGETPSW;
	dasm_optable[0xF8] = dopTRAP;
	dasm_optable[0xF9] = dopTRAP;
	dasm_optable[0xFC] = dopSTTASK;
	dasm_optable[0xFD] = dopSTTASK;
	dasm_optable[0xFE] = dopCLRTLB;
	dasm_optable[0xFF] = dopCLRTLB;
}

#ifdef MAME_DEBUG
unsigned v60_dasm(char *buffer, unsigned pc)
{
	readop = cpu_readmem24lew;
	return dasm_optable[readop(pc)](pc, pc+1, buffer);
}

unsigned v70_dasm(char *buffer, unsigned pc)
{
	readop = cpu_readmem32ledw;
	return dasm_optable[readop(pc)](pc, pc+1, buffer);
}
#endif
