/*
 * MIPS disassembler for the MAME project written by smf
 *
 */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "psx.h"

#ifndef STANDALONE
#include "memory.h"
#endif

static char *make_signed_hex_str_16( UINT32 value )
{
	static char s_hex[ 20 ];

	value &= 0xffff;
	if( value & 0x8000 )
	{
		sprintf( s_hex, "-$%x", ( 0 - value ) & 0x7fff );
	}
	else
	{
		sprintf( s_hex, "$%x", value & 0x7fff );
	}
	return s_hex;
}

static const char *s_cpugenreg[] =
{
	"zero", "at", "v0", "v1", "a0", "a1", "a2", "a3",
	"t0", "t1", "t2", "t3", "t4", "t5", "t6", "t7",
	"s0", "s1", "s2", "s3", "s4", "s5", "s6", "s7",
	"t8", "t9", "k0", "k1", "gp", "sp", "fp", "ra"
};

static const char *s_cp0genreg[] =
{
	"Index", "Random", "EntryLo", "cp0r3", "Context", "cp0r5", "cp0r6", "cp0r7",
	"BadVAddr", "cp0r9", "EntryHi", "cp0r11", "SR", "Cause", "EPC", "PRId",
	"cp0r16", "cp0r17", "cp0r18", "cp0r19", "cp0r20", "cp0r21", "cp0r22", "cp0r23",
	"cp0r24", "cp0r25", "cp0r26", "cp0r27", "cp0r28", "cp0r29", "cp0r30", "cp0r31"
};

static const char *s_cp1genreg[] =
{
	"cp1r0", "cp1r1", "cp1r2", "cp1r3", "cp1r4", "cp1r5", "cp1r6", "cp1r7",
	"cp1r8", "cp1r9", "cp1r10", "cp1r11", "cp1r12", "cp1r13", "cp1r14", "cp1r15",
	"cp1r16", "cp1r17", "cp1r18", "cp1r19", "cp1r20", "cp1r21", "cp1r22", "cp1r22",
	"cp1r23", "cp1r24", "cp1r25", "cp1r26", "cp1r27", "cp1r28", "cp1r29", "cp1r30"
};

static const char *s_cp1ctlreg[] =
{
	"cp1cr0", "cp1cr1", "cp1cr2", "cp1cr3", "cp1cr4", "cp1cr5", "cp1cr6", "cp1cr7",
	"cp1cr8", "cp1cr9", "cp1cr10", "cp1cr11", "cp1cr12", "cp1cr13", "cp1cr14", "cp1cr15",
	"cp1cr16", "cp1cr17", "cp1cr18", "cp1cr19", "cp1cr20", "cp1cr21", "cp1cr22", "cp1cr23",
	"cp1cr24", "cp1cr25", "cp1cr26", "cp1cr27", "cp1cr28", "cp1cr29", "cp1cr30", "cp1cr31"
};

static const char *s_cp2genreg[] =
{
	"vxy0", "vz0", "vxy1", "vz1", "vxy2", "vz2", "rgb", "otz",
	"ir0", "ir1", "ir2", "ir3", "sxy0", "sxy1", "sxy2", "sxyp",
	"sz0", "sz1", "sz2", "sz3", "rgb0", "rgb1", "rgb2", "cp2cr23",
	"mac0", "mac1", "mac2", "mac3", "irgb", "orgb", "lzcs", "lzcr"
};

static const char *s_cp2ctlreg[] =
{
	"r11r12", "r13r21", "r22r23", "r31r32", "r33", "trx", "try", "trz",
	"l11l12", "l13l21", "l22l23", "l31l32", "l33", "rbk", "gbk", "bbk",
	"lr1lr2", "lr3lg1", "lg2lg3", "lb1lb2", "lb3", "rfc", "gfc", "bfc",
	"ofx", "ofy", "h", "dqa", "dqb", "zsf3", "zsf4", "flag"
};

static const char *s_gtesf[] =
{
	"0", "12"
};

static const char *s_gtemx[] =
{
	"rm", "lm", "cm", "0"
};

static const char *s_gtev[] =
{
	"v0", "v1", "v2", "ir"
};

static const char *s_gtecv[] =
{
	"tr", "bk", "fc", "0"
};

static const char *s_gtelm[] =
{
	"0", "1"
};

unsigned DasmMIPS( char *buffer, UINT32 oldpc )
{
	UINT32 pc, op;

	pc = oldpc;
#ifndef STANDALONE
	op = cpu_readop32( pc );
#else
	op = ( filebuf[ pc + order[ 0 ] - offset ] << 24 ) |
		( filebuf[ pc + order[ 1 ] - offset ] << 16 ) |
		( filebuf[ pc + order[ 2 ] - offset ] << 8 ) |
		( filebuf[ pc + order[ 3 ] - offset ] );
#endif
	pc += 4;

	sprintf( buffer, "dw      $%08x", op );

	switch( INS_OP( op ) )
	{
	case OP_SPECIAL:
		switch( INS_FUNCT( op ) )
		{
		case FUNCT_SLL:
			if( op == 0 )
			{
				/* the standard nop is "sll     zero,zero,$0000" */
				sprintf( buffer, "nop" );
			}
			else
			{
				sprintf( buffer, "sll     %s,%s,$%02x", s_cpugenreg[ INS_RD( op ) ], s_cpugenreg[ INS_RT( op ) ], INS_SHAMT( op ) );
			}
			break;
		case FUNCT_SRL:
			sprintf( buffer, "srl     %s,%s,$%02x", s_cpugenreg[ INS_RD( op ) ], s_cpugenreg[ INS_RT( op ) ], INS_SHAMT( op ) );
			break;
		case FUNCT_SRA:
			sprintf( buffer, "sra     %s,%s,$%02x", s_cpugenreg[ INS_RD( op ) ], s_cpugenreg[ INS_RT( op ) ], INS_SHAMT( op ) );
			break;
		case FUNCT_SLLV:
			sprintf( buffer, "sllv    %s,%s,%s", s_cpugenreg[ INS_RD( op ) ], s_cpugenreg[ INS_RT( op ) ], s_cpugenreg[ INS_RS( op ) ] );
			break;
		case FUNCT_SRLV:
			sprintf( buffer, "srlv    %s,%s,%s", s_cpugenreg[ INS_RD( op ) ], s_cpugenreg[ INS_RT( op ) ], s_cpugenreg[ INS_RS( op ) ] );
			break;
		case FUNCT_SRAV:
			sprintf( buffer, "srav    %s,%s,%s", s_cpugenreg[ INS_RD( op ) ], s_cpugenreg[ INS_RT( op ) ], s_cpugenreg[ INS_RS( op ) ] );
			break;
		case FUNCT_JR:
			if( INS_RD( op ) == 0 )
			{
				sprintf( buffer, "jr      %s", s_cpugenreg[ INS_RS( op ) ] );
			}
			break;
		case FUNCT_JALR:
			sprintf( buffer, "jalr    %s,%s", s_cpugenreg[ INS_RD( op ) ], s_cpugenreg[ INS_RS( op ) ] );
			break;
		case FUNCT_SYSCALL:
			sprintf( buffer, "syscall $%05x", INS_CODE( op ) );
			break;
		case FUNCT_BREAK:
			sprintf( buffer, "break   $%05x", INS_CODE( op ) );
			break;
		case FUNCT_MFHI:
			sprintf( buffer, "mfhi    %s", s_cpugenreg[ INS_RD( op ) ] );
			break;
		case FUNCT_MTHI:
			if( INS_RD( op ) == 0 )
			{
				sprintf( buffer, "mthi    %s", s_cpugenreg[ INS_RS( op ) ] );
			}
			break;
		case FUNCT_MFLO:
			sprintf( buffer, "mflo    %s", s_cpugenreg[ INS_RD( op ) ] );
			break;
		case FUNCT_MTLO:
			if( INS_RD( op ) == 0 )
			{
				sprintf( buffer, "mtlo    %s", s_cpugenreg[ INS_RS( op ) ] );
			}
			break;
		case FUNCT_MULT:
			if( INS_RD( op ) == 0 )
			{
				sprintf( buffer, "mult    %s,%s", s_cpugenreg[ INS_RS( op ) ], s_cpugenreg[ INS_RT( op ) ] );
			}
			break;
		case FUNCT_MULTU:
			if( INS_RD( op ) == 0 )
			{
				sprintf( buffer, "multu   %s,%s", s_cpugenreg[ INS_RS( op ) ], s_cpugenreg[ INS_RT( op ) ] );
			}
			break;
		case FUNCT_DIV:
			if( INS_RD( op ) == 0 )
			{
				sprintf( buffer, "div     %s,%s", s_cpugenreg[ INS_RS( op ) ], s_cpugenreg[ INS_RT( op ) ] );
			}
			break;
		case FUNCT_DIVU:
			if( INS_RD( op ) == 0 )
			{
				sprintf( buffer, "divu    %s,%s", s_cpugenreg[ INS_RS( op ) ], s_cpugenreg[ INS_RT( op ) ] );
			}
			break;
		case FUNCT_ADD:
			sprintf( buffer, "add     %s,%s,%s", s_cpugenreg[ INS_RD( op ) ], s_cpugenreg[ INS_RS( op ) ], s_cpugenreg[ INS_RT( op ) ] );
			break;
		case FUNCT_ADDU:
			sprintf( buffer, "addu    %s,%s,%s", s_cpugenreg[ INS_RD( op ) ], s_cpugenreg[ INS_RS( op ) ], s_cpugenreg[ INS_RT( op ) ] );
			break;
		case FUNCT_SUB:
			sprintf( buffer, "sub     %s,%s,%s", s_cpugenreg[ INS_RD( op ) ], s_cpugenreg[ INS_RS( op ) ], s_cpugenreg[ INS_RT( op ) ] );
			break;
		case FUNCT_SUBU:
			sprintf( buffer, "subu    %s,%s,%s", s_cpugenreg[ INS_RD( op ) ], s_cpugenreg[ INS_RS( op ) ], s_cpugenreg[ INS_RT( op ) ] );
			break;
		case FUNCT_AND:
			sprintf( buffer, "and     %s,%s,%s", s_cpugenreg[ INS_RD( op ) ], s_cpugenreg[ INS_RS( op ) ], s_cpugenreg[ INS_RT( op ) ] );
			break;
		case FUNCT_OR:
			sprintf( buffer, "or      %s,%s,%s", s_cpugenreg[ INS_RD( op ) ], s_cpugenreg[ INS_RS( op ) ], s_cpugenreg[ INS_RT( op ) ] );
			break;
		case FUNCT_XOR:
			sprintf( buffer, "xor     %s,%s,%s", s_cpugenreg[ INS_RD( op ) ], s_cpugenreg[ INS_RS( op ) ], s_cpugenreg[ INS_RT( op ) ] );
			break;
		case FUNCT_NOR:
			sprintf( buffer, "nor     %s,%s,%s", s_cpugenreg[ INS_RD( op ) ], s_cpugenreg[ INS_RS( op ) ], s_cpugenreg[ INS_RT( op ) ] );
			break;
		case FUNCT_SLT:
			sprintf( buffer, "slt     %s,%s,%s", s_cpugenreg[ INS_RD( op ) ], s_cpugenreg[ INS_RS( op ) ], s_cpugenreg[ INS_RT( op ) ] );
			break;
		case FUNCT_SLTU:
			sprintf( buffer, "sltu    %s,%s,%s", s_cpugenreg[ INS_RD( op ) ], s_cpugenreg[ INS_RS( op ) ], s_cpugenreg[ INS_RT( op ) ] );
			break;
		}
		break;
	case OP_REGIMM:
		switch( INS_RT( op ) )
		{
		case RT_BLTZ:
			sprintf( buffer, "bltz    %s,$%08x", s_cpugenreg[ INS_RS( op ) ], pc + ( MIPS_WORD_EXTEND( INS_IMMEDIATE( op ) ) << 2 ) );
			break;
		case RT_BGEZ:
			sprintf( buffer, "bgez    %s,$%08x", s_cpugenreg[ INS_RS( op ) ], pc + ( MIPS_WORD_EXTEND( INS_IMMEDIATE( op ) ) << 2 ) );
			break;
		case RT_BLTZAL:
			sprintf( buffer, "bltzal  %s,$%08x", s_cpugenreg[ INS_RS( op ) ], pc + ( MIPS_WORD_EXTEND( INS_IMMEDIATE( op ) ) << 2 ) );
			break;
		case RT_BGEZAL:
			sprintf( buffer, "bgezal  %s,$%08x", s_cpugenreg[ INS_RS( op ) ], pc + ( MIPS_WORD_EXTEND( INS_IMMEDIATE( op ) ) << 2 ) );
			break;
		}
		break;
	case OP_J:
		sprintf( buffer, "j       $%08x", ( pc & 0xF0000000 ) + ( INS_TARGET( op ) << 2 ) );
		break;
	case OP_JAL:
		sprintf( buffer, "jal     $%08x", ( pc & 0xF0000000 ) + ( INS_TARGET( op ) << 2 ) );
		break;
	case OP_BEQ:
		sprintf( buffer, "beq     %s,%s,$%08x", s_cpugenreg[ INS_RS( op ) ], s_cpugenreg[ INS_RT( op ) ], pc + ( MIPS_WORD_EXTEND( INS_IMMEDIATE( op ) ) << 2 ) );
		break;
	case OP_BNE:
		sprintf( buffer, "bne     %s,%s,$%08x", s_cpugenreg[ INS_RS( op ) ], s_cpugenreg[ INS_RT( op ) ], pc + ( MIPS_WORD_EXTEND( INS_IMMEDIATE( op ) ) << 2 ) );
		break;
	case OP_BLEZ:
		if( INS_RT( op ) == 0 )
		{
			sprintf( buffer, "blez    %s,$%08x", s_cpugenreg[ INS_RS( op ) ], pc + ( MIPS_WORD_EXTEND( INS_IMMEDIATE( op ) ) << 2 ) );
		}
		break;
	case OP_BGTZ:
		if( INS_RT( op ) == 0 )
		{
			sprintf( buffer, "bgtz    %s,$%08x", s_cpugenreg[ INS_RS( op ) ], pc + ( MIPS_WORD_EXTEND( INS_IMMEDIATE( op ) ) << 2 ) );
		}
		break;
	case OP_ADDI:
		sprintf( buffer, "addi    %s,%s,%s", s_cpugenreg[ INS_RT( op ) ], s_cpugenreg[ INS_RS( op ) ], make_signed_hex_str_16( INS_IMMEDIATE( op ) ) );
		break;
	case OP_ADDIU:
		sprintf( buffer, "addiu   %s,%s,%s", s_cpugenreg[ INS_RT( op ) ], s_cpugenreg[ INS_RS( op ) ], make_signed_hex_str_16( INS_IMMEDIATE( op ) ) );
		break;
	case OP_SLTI:
		sprintf( buffer, "slti    %s,%s,%s", s_cpugenreg[ INS_RT( op ) ], s_cpugenreg[ INS_RS( op ) ], make_signed_hex_str_16( INS_IMMEDIATE( op ) ) );
		break;
	case OP_SLTIU:
		sprintf( buffer, "sltiu   %s,%s,%s", s_cpugenreg[ INS_RT( op ) ], s_cpugenreg[ INS_RS( op ) ], make_signed_hex_str_16( INS_IMMEDIATE( op ) ) );
		break;
	case OP_ANDI:
		sprintf( buffer, "andi    %s,%s,$%04x", s_cpugenreg[ INS_RT( op ) ], s_cpugenreg[ INS_RS( op ) ], INS_IMMEDIATE( op ) );
		break;
	case OP_ORI:
		sprintf( buffer, "ori     %s,%s,$%04x", s_cpugenreg[ INS_RT( op ) ], s_cpugenreg[ INS_RS( op ) ], INS_IMMEDIATE( op ) );
		break;
	case OP_XORI:
		sprintf( buffer, "xori    %s,%s,$%04x", s_cpugenreg[ INS_RT( op ) ], s_cpugenreg[ INS_RS( op ) ], INS_IMMEDIATE( op ) );
		break;
	case OP_LUI:
		sprintf( buffer, "lui     %s,$%04x", s_cpugenreg[ INS_RT( op ) ], INS_IMMEDIATE( op ) );
		break;
	case OP_COP0:
		switch( INS_RS( op ) )
		{
		case RS_MFC:
			sprintf( buffer, "mfc0    %s,%s",  s_cpugenreg[ INS_RT( op ) ], s_cp0genreg[ INS_RD( op ) ] );
			break;
		case RS_MTC:
			sprintf( buffer, "mtc0    %s,%s",  s_cpugenreg[ INS_RT( op ) ], s_cp0genreg[ INS_RD( op ) ] );
			break;
		case RS_BC:
			switch( INS_RT( op ) )
			{
			case RT_BCF:
				sprintf( buffer, "bc0f    $%08x", pc + ( MIPS_WORD_EXTEND( INS_IMMEDIATE( op ) ) << 2 ) );
				break;
			case RT_BCT:
				sprintf( buffer, "bc0t    $%08x", pc + ( MIPS_WORD_EXTEND( INS_IMMEDIATE( op ) ) << 2 ) );
				break;
			}
			break;
		default:
			switch( INS_CO( op ) )
			{
			case 1:
				sprintf( buffer, "cop0    $%07x", INS_COFUN( op ) );

				switch( INS_CF( op ) )
				{
				case 1:
					sprintf( buffer, "tlbr" );
					break;
				case 4:
					sprintf( buffer, "tlbwi" );
					break;
				case 6:
					sprintf( buffer, "tlbwr" );
					break;
				case 8:
					sprintf( buffer, "tlbp" );
					break;
				case 16:
					sprintf( buffer, "rfe" );
					break;
				}
				break;
			}
			break;
		}
		break;
	case OP_COP1:
		switch( INS_RS( op ) )
		{
		case RS_MFC:
			sprintf( buffer, "mfc1    %s,%s",  s_cpugenreg[ INS_RT( op ) ], s_cp1genreg[ INS_RD( op ) ] );
			break;
		case RS_CFC:
			sprintf( buffer, "cfc1    %s,%s",  s_cpugenreg[ INS_RT( op ) ], s_cp1ctlreg[ INS_RD( op ) ] );
			break;
		case RS_MTC:
			sprintf( buffer, "mtc1    %s,%s",  s_cpugenreg[ INS_RT( op ) ], s_cp1genreg[ INS_RD( op ) ] );
			break;
		case RS_CTC:
			sprintf( buffer, "ctc1    %s,%s",  s_cpugenreg[ INS_RT( op ) ], s_cp1ctlreg[ INS_RD( op ) ] );
			break;
		case RS_BC:
			switch( INS_RT( op ) )
			{
			case RT_BCF:
				sprintf( buffer, "bc1f    $%08x", pc + ( MIPS_WORD_EXTEND( INS_IMMEDIATE( op ) ) << 2 ) );
				break;
			case RT_BCT:
				sprintf( buffer, "bc1t    $%08x", pc + ( MIPS_WORD_EXTEND( INS_IMMEDIATE( op ) ) << 2 ) );
				break;
			}
			break;
		default:
			switch( INS_CO( op ) )
			{
			case 1:
				sprintf( buffer, "cop1    $%07x", INS_COFUN( op ) );
				break;
			}
			break;
		}
		break;
	case OP_COP2:
		switch( INS_RS( op ) )
		{
		case RS_MFC:
			sprintf( buffer, "mfc2    %s,%s",  s_cpugenreg[ INS_RT( op ) ], s_cp2genreg[ INS_RD( op ) ] );
			break;
		case RS_CFC:
			sprintf( buffer, "cfc2    %s,%s",  s_cpugenreg[ INS_RT( op ) ], s_cp2ctlreg[ INS_RD( op ) ] );
			break;
		case RS_MTC:
			sprintf( buffer, "mtc2    %s,%s",  s_cpugenreg[ INS_RT( op ) ], s_cp2genreg[ INS_RD( op ) ] );
			break;
		case RS_CTC:
			sprintf( buffer, "ctc2    %s,%s",  s_cpugenreg[ INS_RT( op ) ], s_cp2ctlreg[ INS_RD( op ) ] );
			break;
		case RS_BC:
			switch( INS_RT( op ) )
			{
			case RT_BCF:
				sprintf( buffer, "bc2f    $%08x", pc + ( MIPS_WORD_EXTEND( INS_IMMEDIATE( op ) ) << 2 ) );
				break;
			case RT_BCT:
				sprintf( buffer, "bc2t    $%08x", pc + ( MIPS_WORD_EXTEND( INS_IMMEDIATE( op ) ) << 2 ) );
				break;
			}
			break;
		default:
			switch( INS_CO( op ) )
			{
			case 1:
				sprintf( buffer, "cop2    $%07x", INS_COFUN( op ) );

				switch( GTE_OP( op ) )
				{
				case 0x01:
					if( INS_CO( op ) == 0x0180001 )
					{
						sprintf( buffer, "rtps" );
					}
					break;
				case 0x02:
					if( INS_CO( op ) == 0x0280030 )
					{
						sprintf( buffer, "rtpt" );
					}
					break;
				case 0x04:
					if( GTE_CT( op ) == 0x012 ||
						GTE_CT( op ) == 0x412 )
					{
						sprintf( buffer, "mvmva%s %s + %s * %s (lm=%s)",
							s_gtesf[ GTE_SF( op ) ], s_gtecv[ GTE_CV( op ) ], s_gtemx[ GTE_MX( op ) ],
							s_gtev[ GTE_V( op ) ],  s_gtelm[ GTE_LM( op ) ] );
					}
					break;
				case 0x06:
					if( INS_CO( op ) == 0x0680029 )
					{
						sprintf( buffer, "dcpl" );
					}
					break;
				case 0x07:
					if( INS_CO( op ) == 0x0780010 )
					{
						sprintf( buffer, "dpcs" );
					}
					break;
				case 0x09:
					if( INS_CO( op ) == 0x0980011 )
					{
						sprintf( buffer, "intpl" );
					}
					break;
				case 0x0a:
					if( GTE_CT( op ) == 0x428 )
					{
						sprintf( buffer, "sqr%s", s_gtesf[ GTE_SF( op ) ] );
					}
					break;
				case 0x0c:
					if( INS_CO( op ) == 0x0c8041e )
					{
						sprintf( buffer, "ncs" );
					}
					break;
				case 0x0d:
					if( INS_CO( op ) == 0x0d80420 )
					{
						sprintf( buffer, "nct" );
					}
					break;
				case 0x0e:
					if( INS_CO( op ) == 0x0e80413 )
					{
						sprintf( buffer, "ncds" );
					}
					break;
				case 0x0f:
					if( INS_CO( op ) == 0x0f8002A )
					{
						sprintf( buffer, "dpct" );
					}
					else if( INS_CO( op ) == 0x0f80416 )
					{
						sprintf( buffer, "ncdt" );
					}
					break;
				case 0x10:
					if( INS_CO( op ) == 0x108041b )
					{
						sprintf( buffer, "nccs" );
					}
					break;
				case 0x11:
					if( INS_CO( op ) == 0x118043f )
					{
						sprintf( buffer, "ncct" );
					}
					break;
				case 0x12:
					if( INS_CO( op ) == 0x1280414 )
					{
						sprintf( buffer, "cdp" );
					}
					break;
				case 0x13:
					if( INS_CO( op ) == 0x138041c )
					{
						sprintf( buffer, "cc" );
					}
					break;
				case 0x14:
					if( INS_CO( op ) == 0x1400006 )
					{
						sprintf( buffer, "nclip" );
					}
					break;
				case 0x15:
					if( INS_CO( op ) == 0x158002d )
					{
						sprintf( buffer, "avsz3" );
					}
					break;
				case 0x16:
					if( INS_CO( op ) == 0x168002e )
					{
						sprintf( buffer, "avsz4" );
					}
					break;
				case 0x17:
					if( GTE_CT( op ) == 0x00c )
					{
						sprintf( buffer, "op%s", s_gtesf[ GTE_SF( op ) ] );
					}
					break;
				case 0x19:
					if( GTE_CT( op ) == 0x03d )
					{
						sprintf( buffer, "gpf%s", s_gtesf[ GTE_SF( op ) ] );
					}
					break;
				case 0x1a:
					if( GTE_CT( op ) == 0x03e )
					{
						sprintf( buffer, "gpl%s", s_gtesf[ GTE_SF( op ) ] );
					}
					break;
				}
			}
			break;
		}
		break;
	case OP_LB:
		sprintf( buffer, "lb      %s,%s(%s)", s_cpugenreg[ INS_RT( op ) ], make_signed_hex_str_16( INS_IMMEDIATE( op ) ), s_cpugenreg[ INS_RS( op ) ] );
		break;
	case OP_LH:
		sprintf( buffer, "lh      %s,%s(%s)", s_cpugenreg[ INS_RT( op ) ], make_signed_hex_str_16( INS_IMMEDIATE( op ) ), s_cpugenreg[ INS_RS( op ) ] );
		break;
	case OP_LWL:
		sprintf( buffer, "lwl     %s,%s(%s)", s_cpugenreg[ INS_RT( op ) ], make_signed_hex_str_16( INS_IMMEDIATE( op ) ), s_cpugenreg[ INS_RS( op ) ] );
		break;
	case OP_LW:
		sprintf( buffer, "lw      %s,%s(%s)", s_cpugenreg[ INS_RT( op ) ], make_signed_hex_str_16( INS_IMMEDIATE( op ) ), s_cpugenreg[ INS_RS( op ) ] );
		break;
	case OP_LBU:
		sprintf( buffer, "lbu     %s,%s(%s)", s_cpugenreg[ INS_RT( op ) ], make_signed_hex_str_16( INS_IMMEDIATE( op ) ), s_cpugenreg[ INS_RS( op ) ] );
		break;
	case OP_LHU:
		sprintf( buffer, "lhu     %s,%s(%s)", s_cpugenreg[ INS_RT( op ) ], make_signed_hex_str_16( INS_IMMEDIATE( op ) ), s_cpugenreg[ INS_RS( op ) ] );
		break;
	case OP_LWR:
		sprintf( buffer, "lwr     %s,%s(%s)", s_cpugenreg[ INS_RT( op ) ], make_signed_hex_str_16( INS_IMMEDIATE( op ) ), s_cpugenreg[ INS_RS( op ) ] );
		break;
	case OP_SB:
		sprintf( buffer, "sb      %s,%s(%s)", s_cpugenreg[ INS_RT( op ) ], make_signed_hex_str_16( INS_IMMEDIATE( op ) ), s_cpugenreg[ INS_RS( op ) ] );
		break;
	case OP_SH:
		sprintf( buffer, "sh      %s,%s(%s)", s_cpugenreg[ INS_RT( op ) ], make_signed_hex_str_16( INS_IMMEDIATE( op ) ), s_cpugenreg[ INS_RS( op ) ] );
		break;
	case OP_SWL:
		sprintf( buffer, "swl     %s,%s(%s)", s_cpugenreg[ INS_RT( op ) ], make_signed_hex_str_16( INS_IMMEDIATE( op ) ), s_cpugenreg[ INS_RS( op ) ] );
		break;
	case OP_SW:
		sprintf( buffer, "sw      %s,%s(%s)", s_cpugenreg[ INS_RT( op ) ], make_signed_hex_str_16( INS_IMMEDIATE( op ) ), s_cpugenreg[ INS_RS( op ) ] );
		break;
	case OP_SWR:
		sprintf( buffer, "swr     %s,%s(%s)", s_cpugenreg[ INS_RT( op ) ], make_signed_hex_str_16( INS_IMMEDIATE( op ) ), s_cpugenreg[ INS_RS( op ) ] );
		break;
	case OP_LWC1:
		sprintf( buffer, "lwc1    %s,%s(%s)", s_cp1genreg[ INS_RT( op ) ], make_signed_hex_str_16( INS_IMMEDIATE( op ) ), s_cpugenreg[ INS_RS( op ) ] );
		break;
	case OP_LWC2:
		sprintf( buffer, "lwc2    %s,%s(%s)", s_cp2genreg[ INS_RT( op ) ], make_signed_hex_str_16( INS_IMMEDIATE( op ) ), s_cpugenreg[ INS_RS( op ) ] );
		break;
	case OP_SWC1:
		sprintf( buffer, "swc1    %s,%s(%s)", s_cp1genreg[ INS_RT( op ) ], make_signed_hex_str_16( INS_IMMEDIATE( op ) ), s_cpugenreg[ INS_RS( op ) ] );
		break;
	case OP_SWC2:
		sprintf( buffer, "swc2    %s,%s(%s)", s_cp2genreg[ INS_RT( op ) ], make_signed_hex_str_16( INS_IMMEDIATE( op ) ), s_cpugenreg[ INS_RS( op ) ] );
		break;
	}
	return pc - oldpc;
}
