/*###################################################################################################
**
**	TMS34010: Portable Texas Instruments TMS34010 emulator
**
**	Copyright (C) Alex Pasadyn/Zsolt Vasvari 1998
**	 Parts based on code by Aaron Giles
**
**#################################################################################################*/

#include <stdio.h>
#include <stdlib.h>
#include "driver.h"
#include "osd_cpu.h"
#include "cpuintrf.h"
#include "mamedbg.h"
#include "tms34010.h"
#include "34010ops.h"
#include "state.h"

#ifdef MAME_DEBUG
extern int debug_key_pressed;
#endif


/*###################################################################################################
**	DEBUG STATE & STRUCTURES
**#################################################################################################*/

static UINT8 tms34010_reg_layout[] =
{
	TMS34010_PC, TMS34010_SP, -1,
	TMS34010_A0, TMS34010_B0, -1,
	TMS34010_A1, TMS34010_B1, -1,
	TMS34010_A2, TMS34010_B2, -1,
	TMS34010_A3, TMS34010_B3, -1,
	TMS34010_A4, TMS34010_B4, -1,
	TMS34010_A5, TMS34010_B5, -1,
	TMS34010_A6, TMS34010_B6, -1,
	TMS34010_A7, TMS34010_B7, -1,
	TMS34010_A8, TMS34010_B8, -1,
	TMS34010_A9, TMS34010_B9, -1,
	TMS34010_A10,TMS34010_B10,-1,
	TMS34010_A11,TMS34010_B11,-1,
	TMS34010_A12,TMS34010_B12,-1,
	TMS34010_A13,TMS34010_B13,-1,
	TMS34010_A14,TMS34010_B14, 0
};

static UINT8 tms34010_win_layout[] =
{
	40, 0,39,17,	/* register window (top right) */
	 0, 0,39,17,	/* disassembler window (left, upper) */
	 0,18,39, 4,	/* memory #1 window (left, middle) */
	40,18,39, 4,	/* memory #2 window (lower) */
	 0,23,80, 1 	/* command line window (bottom rows) */
};



/*###################################################################################################
**	CORE STATE
**#################################################################################################*/

/* TMS34010 State */
typedef struct
{
#ifdef MSB_FIRST
	INT16 y;
	INT16 x;
#else
	INT16 x;
	INT16 y;
#endif
} XY;

typedef struct tms34010_regs
{
	UINT32 op;
	UINT32 pc;
	UINT32 st;					/* Only here so we can display it in the debug window */
	UINT32 flat_aregs[16];		/* This is a "flat" variant of the registers */
	UINT32 flat_bregs[15];		/* This is a "flat" variant of the registers */
	UINT32 nflag;
	UINT32 cflag;
	UINT32 notzflag;  /* So we can just do an assignment to set it */
	UINT32 vflag;
	UINT32 pflag;
	UINT32 ieflag;
	UINT32 fe0flag;
	UINT32 fe1flag;
	UINT32 fw[2];
	UINT32 fw_inc[2];  /* Same as fw[], except when fw = 0, fw_inc = 32 */
	UINT32 reset_deferred;
	void (*f0_write)(offs_t offset,data32_t data);
	void (*f1_write)(offs_t offset,data32_t data);
	void (*pixel_write)(offs_t offset,data32_t data);
	data32_t (*f0_read)(offs_t offset);
	data32_t (*f1_read)(offs_t offset);
	data32_t (*pixel_read)(offs_t offset);
	UINT32 transparency;
	UINT32 window_checking;
	 INT32 (*raster_op)(INT32 newpix, INT32 oldpix);
	UINT32 convsp;
	UINT32 convdp;
	UINT32 convmp;
	UINT32 pixelshift;
	data16_t *shiftreg;
	int gfxcycles;
	UINT8  is_34020;
	UINT8  ext_irq_lines;
	int (*irq_callback)(int irqline);
	int last_update_vcount;
	struct tms34010_config *config;

	/* for the 34010, we only copy 32 of these into the new state */
	UINT16 IOregs[64];

	/*************************
	  note: in order to speed things up, we don't copy any data from here
	  forward on a state transition
	*************************/
	union						/* The register files are interleaved, so */
	{							/* that the SP occupies the same location in both */
		INT32 Bregs[241];   	/* Only every 16th entry is actually used */
		XY BregsXY[241];
		struct
		{
			INT32 unused[225];
			union
			{
				INT32 Aregs[16];
				XY AregsXY[16];
			} a;
		} a;
	} regs;
} tms34010_regs;

#define TMS34010_STATE_SIZE		(offsetof(tms34010_regs, IOregs) + 32 * sizeof(UINT16))
#define TMS34020_STATE_SIZE		(offsetof(tms34010_regs, IOregs) + 64 * sizeof(UINT16))



/*###################################################################################################
**	GLOBAL VARIABLES
**#################################################################################################*/

/* public globals */
int	tms34010_ICount;

/* internal state */
static tms34010_regs 	state;
static UINT8			external_host_access;
static void *			dpyint_timer[MAX_CPU];		  /* Display interrupt timer */
static void *			vsblnk_timer[MAX_CPU];		  /* VBLANK start timer */

/* default configuration */
static struct tms34010_config default_config =
{
	0,					/* don't halt on reset */
	NULL,				/* no interrupt callback */
	NULL,				/* no shiftreg functions */
	NULL				/* no shiftreg functions */
};

static void check_interrupt(void);
static void vsblnk_callback(int cpunum);
static void dpyint_callback(int cpunum);
static void tms34010_state_presave(void);
static void tms34010_state_postload(void);



/*###################################################################################################
**	FUNCTION TABLES
**#################################################################################################*/

extern void wfield_01(offs_t offset,data32_t data);
extern void wfield_02(offs_t offset,data32_t data);
extern void wfield_03(offs_t offset,data32_t data);
extern void wfield_04(offs_t offset,data32_t data);
extern void wfield_05(offs_t offset,data32_t data);
extern void wfield_06(offs_t offset,data32_t data);
extern void wfield_07(offs_t offset,data32_t data);
extern void wfield_08(offs_t offset,data32_t data);
extern void wfield_09(offs_t offset,data32_t data);
extern void wfield_10(offs_t offset,data32_t data);
extern void wfield_11(offs_t offset,data32_t data);
extern void wfield_12(offs_t offset,data32_t data);
extern void wfield_13(offs_t offset,data32_t data);
extern void wfield_14(offs_t offset,data32_t data);
extern void wfield_15(offs_t offset,data32_t data);
extern void wfield_16(offs_t offset,data32_t data);
extern void wfield_17(offs_t offset,data32_t data);
extern void wfield_18(offs_t offset,data32_t data);
extern void wfield_19(offs_t offset,data32_t data);
extern void wfield_20(offs_t offset,data32_t data);
extern void wfield_21(offs_t offset,data32_t data);
extern void wfield_22(offs_t offset,data32_t data);
extern void wfield_23(offs_t offset,data32_t data);
extern void wfield_24(offs_t offset,data32_t data);
extern void wfield_25(offs_t offset,data32_t data);
extern void wfield_26(offs_t offset,data32_t data);
extern void wfield_27(offs_t offset,data32_t data);
extern void wfield_28(offs_t offset,data32_t data);
extern void wfield_29(offs_t offset,data32_t data);
extern void wfield_30(offs_t offset,data32_t data);
extern void wfield_31(offs_t offset,data32_t data);
extern void wfield_32(offs_t offset,data32_t data);

static void (*wfield_functions[32])(offs_t offset,data32_t data) =
{
	wfield_32, wfield_01, wfield_02, wfield_03, wfield_04, wfield_05,
	wfield_06, wfield_07, wfield_08, wfield_09, wfield_10, wfield_11,
	wfield_12, wfield_13, wfield_14, wfield_15, wfield_16, wfield_17,
	wfield_18, wfield_19, wfield_20, wfield_21, wfield_22, wfield_23,
	wfield_24, wfield_25, wfield_26, wfield_27, wfield_28, wfield_29,
	wfield_30, wfield_31
};

extern data32_t rfield_z_01(offs_t offset);
extern data32_t rfield_z_02(offs_t offset);
extern data32_t rfield_z_03(offs_t offset);
extern data32_t rfield_z_04(offs_t offset);
extern data32_t rfield_z_05(offs_t offset);
extern data32_t rfield_z_06(offs_t offset);
extern data32_t rfield_z_07(offs_t offset);
extern data32_t rfield_z_08(offs_t offset);
extern data32_t rfield_z_09(offs_t offset);
extern data32_t rfield_z_10(offs_t offset);
extern data32_t rfield_z_11(offs_t offset);
extern data32_t rfield_z_12(offs_t offset);
extern data32_t rfield_z_13(offs_t offset);
extern data32_t rfield_z_14(offs_t offset);
extern data32_t rfield_z_15(offs_t offset);
extern data32_t rfield_z_16(offs_t offset);
extern data32_t rfield_z_17(offs_t offset);
extern data32_t rfield_z_18(offs_t offset);
extern data32_t rfield_z_19(offs_t offset);
extern data32_t rfield_z_20(offs_t offset);
extern data32_t rfield_z_21(offs_t offset);
extern data32_t rfield_z_22(offs_t offset);
extern data32_t rfield_z_23(offs_t offset);
extern data32_t rfield_z_24(offs_t offset);
extern data32_t rfield_z_25(offs_t offset);
extern data32_t rfield_z_26(offs_t offset);
extern data32_t rfield_z_27(offs_t offset);
extern data32_t rfield_z_28(offs_t offset);
extern data32_t rfield_z_29(offs_t offset);
extern data32_t rfield_z_30(offs_t offset);
extern data32_t rfield_z_31(offs_t offset);
extern data32_t rfield_32(offs_t offset);

static data32_t (*rfield_functions_z[32])(offs_t offset) =
{
	rfield_32  , rfield_z_01, rfield_z_02, rfield_z_03, rfield_z_04, rfield_z_05,
	rfield_z_06, rfield_z_07, rfield_z_08, rfield_z_09, rfield_z_10, rfield_z_11,
	rfield_z_12, rfield_z_13, rfield_z_14, rfield_z_15, rfield_z_16, rfield_z_17,
	rfield_z_18, rfield_z_19, rfield_z_20, rfield_z_21, rfield_z_22, rfield_z_23,
	rfield_z_24, rfield_z_25, rfield_z_26, rfield_z_27, rfield_z_28, rfield_z_29,
	rfield_z_30, rfield_z_31
};

extern data32_t rfield_s_01(offs_t offset);
extern data32_t rfield_s_02(offs_t offset);
extern data32_t rfield_s_03(offs_t offset);
extern data32_t rfield_s_04(offs_t offset);
extern data32_t rfield_s_05(offs_t offset);
extern data32_t rfield_s_06(offs_t offset);
extern data32_t rfield_s_07(offs_t offset);
extern data32_t rfield_s_08(offs_t offset);
extern data32_t rfield_s_09(offs_t offset);
extern data32_t rfield_s_10(offs_t offset);
extern data32_t rfield_s_11(offs_t offset);
extern data32_t rfield_s_12(offs_t offset);
extern data32_t rfield_s_13(offs_t offset);
extern data32_t rfield_s_14(offs_t offset);
extern data32_t rfield_s_15(offs_t offset);
extern data32_t rfield_s_16(offs_t offset);
extern data32_t rfield_s_17(offs_t offset);
extern data32_t rfield_s_18(offs_t offset);
extern data32_t rfield_s_19(offs_t offset);
extern data32_t rfield_s_20(offs_t offset);
extern data32_t rfield_s_21(offs_t offset);
extern data32_t rfield_s_22(offs_t offset);
extern data32_t rfield_s_23(offs_t offset);
extern data32_t rfield_s_24(offs_t offset);
extern data32_t rfield_s_25(offs_t offset);
extern data32_t rfield_s_26(offs_t offset);
extern data32_t rfield_s_27(offs_t offset);
extern data32_t rfield_s_28(offs_t offset);
extern data32_t rfield_s_29(offs_t offset);
extern data32_t rfield_s_30(offs_t offset);
extern data32_t rfield_s_31(offs_t offset);

static data32_t (*rfield_functions_s[32])(offs_t offset) =
{
	rfield_32  , rfield_s_01, rfield_s_02, rfield_s_03, rfield_s_04, rfield_s_05,
	rfield_s_06, rfield_s_07, rfield_s_08, rfield_s_09, rfield_s_10, rfield_s_11,
	rfield_s_12, rfield_s_13, rfield_s_14, rfield_s_15, rfield_s_16, rfield_s_17,
	rfield_s_18, rfield_s_19, rfield_s_20, rfield_s_21, rfield_s_22, rfield_s_23,
	rfield_s_24, rfield_s_25, rfield_s_26, rfield_s_27, rfield_s_28, rfield_s_29,
	rfield_s_30, rfield_s_31
};



/*###################################################################################################
**	MACROS
**#################################################################################################*/

/* context finder */
static INLINE tms34010_regs *FINDCONTEXT(int cpu)
{
	tms34010_regs *context = cpunum_get_context_ptr(cpu);
	if (!context)
		context = &state;
	return context;
}

/* register definitions and shortcuts */
#define PC				(state.pc)
#define ST				(state.st)
#define N_FLAG			(state.nflag)
#define NOTZ_FLAG		(state.notzflag)
#define C_FLAG			(state.cflag)
#define V_FLAG			(state.vflag)
#define P_FLAG			(state.pflag)
#define IE_FLAG			(state.ieflag)
#define FE0_FLAG		(state.fe0flag)
#define FE1_FLAG		(state.fe1flag)

/* register file access */
#define AREG(i)			(state.regs.a.a.Aregs[i])
#define AREG_XY(i)		(state.regs.a.a.AregsXY[i])
#define AREG_X(i)		(state.regs.a.a.AregsXY[i].x)
#define AREG_Y(i)		(state.regs.a.a.AregsXY[i].y)
#define BREG(i)			(state.regs.Bregs[i])
#define BREG_XY(i)		(state.regs.BregsXY[i])
#define BREG_X(i)		(state.regs.BregsXY[i].x)
#define BREG_Y(i)		(state.regs.BregsXY[i].y)
#define SP				AREG(15)
#define FW(i)			(state.fw[i])
#define FW_INC(i)		(state.fw_inc[i])
#define BINDEX(i)		((i) << 4)

/* opcode decode helpers */
#define ASRCREG			((state.op >> 5) & 0x0f)
#define ADSTREG			(state.op & 0x0f)
#define BSRCREG			((state.op & 0x1e0) >> 1)
#define BDSTREG			((state.op & 0x0f) << 4)
#define SKIP_WORD		(PC += (2 << 3))
#define SKIP_LONG		(PC += (4 << 3))
#define PARAM_K			((state.op >> 5) & 0x1f)
#define PARAM_N			(state.op & 0x1f)
#define PARAM_REL8		((INT8)state.op)

/* memory I/O */
#define WFIELD0(a,b)	state.f0_write(a,b)
#define WFIELD1(a,b)	state.f1_write(a,b)
#define RFIELD0(a)		state.f0_read(a)
#define RFIELD1(a)		state.f1_read(a)
#define WPIXEL(a,b)		state.pixel_write(a,b)
#define RPIXEL(a)		state.pixel_read(a)

/* Implied Operands */
#define SADDR			BREG(BINDEX(0))
#define SADDR_X			BREG_X(BINDEX(0))
#define SADDR_Y			BREG_Y(BINDEX(0))
#define SADDR_XY		BREG_XY(BINDEX(0))
#define SPTCH			BREG(BINDEX(1))
#define DADDR			BREG(BINDEX(2))
#define DADDR_X			BREG_X(BINDEX(2))
#define DADDR_Y			BREG_Y(BINDEX(2))
#define DADDR_XY		BREG_XY(BINDEX(2))
#define DPTCH			BREG(BINDEX(3))
#define OFFSET			BREG(BINDEX(4))
#define WSTART_X		BREG_X(BINDEX(5))
#define WSTART_Y		BREG_Y(BINDEX(5))
#define WEND_X			BREG_X(BINDEX(6))
#define WEND_Y			BREG_Y(BINDEX(6))
#define DYDX_X			BREG_X(BINDEX(7))
#define DYDX_Y			BREG_Y(BINDEX(7))
#define COLOR0			BREG(BINDEX(8))
#define COLOR1			BREG(BINDEX(9))
#define COUNT			BREG(BINDEX(10))
#define INC1_X			BREG_X(BINDEX(11))
#define INC1_Y			BREG_Y(BINDEX(11))
#define INC2_X			BREG_X(BINDEX(12))
#define INC2_Y			BREG_Y(BINDEX(12))
#define PATTRN			BREG(BINDEX(13))
#define TEMP			BREG(BINDEX(14))


/*###################################################################################################
**	INLINE SHORTCUTS
**#################################################################################################*/

/* set the field widths - shortcut */
static INLINE void SET_FW(void)
{
	FW_INC(0) = FW(0) ? FW(0) : 0x20;
	FW_INC(1) = FW(1) ? FW(1) : 0x20;

	state.f0_write = wfield_functions[FW(0)];
	state.f1_write = wfield_functions[FW(1)];

	if (FE0_FLAG)
		state.f0_read  = rfield_functions_s[FW(0)];	/* Sign extend */
	else
		state.f0_read  = rfield_functions_z[FW(0)];	/* Zero extend */

	if (FE1_FLAG)
		state.f1_read  = rfield_functions_s[FW(1)];	/* Sign extend */
	else
		state.f1_read  = rfield_functions_z[FW(1)];	/* Zero extend */
}

/* Intialize Status to 0x0010 */
static INLINE void RESET_ST(void)
{
	N_FLAG = C_FLAG = V_FLAG = P_FLAG = IE_FLAG = FE0_FLAG = FE1_FLAG = 0;
	NOTZ_FLAG = 1;
	FW(0) = 0x10;
	FW(1) = 0;
	SET_FW();
}

/* Combine indiviual flags into the Status Register */
static INLINE UINT32 GET_ST(void)
{
	return (     N_FLAG ? 0x80000000 : 0) |
		   (     C_FLAG ? 0x40000000 : 0) |
		   (  NOTZ_FLAG ? 0 : 0x20000000) |
		   (     V_FLAG ? 0x10000000 : 0) |
		   (     P_FLAG ? 0x02000000 : 0) |
		   (    IE_FLAG ? 0x00200000 : 0) |
		   (   FE1_FLAG ? 0x00000800 : 0) |
		   (FW(1) << 6)                   |
		   (   FE0_FLAG ? 0x00000020 : 0) |
		   FW(0);
}

/* Break up Status Register into indiviual flags */
static INLINE void SET_ST(UINT32 st)
{
	N_FLAG    =    st & 0x80000000;
	C_FLAG    =    st & 0x40000000;
	NOTZ_FLAG =  !(st & 0x20000000);
	V_FLAG    =    st & 0x10000000;
	P_FLAG    =    st & 0x02000000;
	IE_FLAG   =    st & 0x00200000;
	FE1_FLAG  =    st & 0x00000800;
	FW(1)     =   (st >> 6) & 0x1f;
	FE0_FLAG  =    st & 0x00000020;
	FW(0)     =    st & 0x1f;
	SET_FW();

	/* interrupts might have been enabled, check it */
	check_interrupt();
}

/* shortcuts for reading opcodes */
static INLINE UINT32 ROPCODE(void)
{
	UINT32 pc = TOBYTE(PC);
	PC += 2 << 3;
	return cpu_readop16(pc);
}

static INLINE INT16 PARAM_WORD(void)
{
	UINT32 pc = TOBYTE(PC);
	PC += 2 << 3;
	return cpu_readop_arg16(pc);
}

static INLINE INT32 PARAM_LONG(void)
{
	UINT32 pc = TOBYTE(PC);
	PC += 4 << 3;
	return (UINT16)cpu_readop_arg16(pc) | (cpu_readop_arg16(pc + 2) << 16);
}

static INLINE INT16 PARAM_WORD_NO_INC(void)
{
	return cpu_readop_arg16(TOBYTE(PC));
}

static INLINE INT32 PARAM_LONG_NO_INC(void)
{
	UINT32 pc = TOBYTE(PC);
	return (UINT16)cpu_readop_arg16(pc) | (cpu_readop_arg16(pc + 2) << 16);
}

/* read memory byte */
static INLINE data32_t RBYTE(offs_t offset)
{
	UINT32 ret;
	RFIELDMAC_8;
	return ret;
}

/* write memory byte */
static INLINE void WBYTE(offs_t offset,data32_t data)
{
	WFIELDMAC_8;
}

/* read memory long */
static INLINE data32_t RLONG(offs_t offset)
{
	RFIELDMAC_32;
}

/* write memory long */
static INLINE void WLONG(offs_t offset,data32_t data)
{
	WFIELDMAC_32;
}

/* pushes/pops a value from the stack */
static INLINE void PUSH(UINT32 data)
{
	SP -= 0x20;
	WLONG(SP, data);
}

static INLINE INT32 POP(void)
{
	INT32 ret = RLONG(SP);
	SP += 0x20;
	return ret;
}



/*###################################################################################################
**	PIXEL READS
**#################################################################################################*/

#define RP(m1,m2)  											\
	/* TODO: Plane masking */								\
	return (TMS34010_RDMEM_WORD(TOBYTE(offset & 0xfffffff0)) >> (offset & m1)) & m2;

static data32_t read_pixel_1(offs_t offset) { RP(0x0f,0x01) }
static data32_t read_pixel_2(offs_t offset) { RP(0x0e,0x03) }
static data32_t read_pixel_4(offs_t offset) { RP(0x0c,0x0f) }
static data32_t read_pixel_8(offs_t offset) { RP(0x08,0xff) }
static data32_t read_pixel_16(offs_t offset)
{
	/* TODO: Plane masking */
	return TMS34010_RDMEM_WORD(TOBYTE(offset & 0xfffffff0));
}

/* Shift register read */
static data32_t read_pixel_shiftreg(offs_t offset)
{
	if (state.config->to_shiftreg)
		state.config->to_shiftreg(offset, &state.shiftreg[0]);
	else
		log_cb(RETRO_LOG_DEBUG, LOGPRE "To ShiftReg function not set. PC = %08X\n", PC);
	return state.shiftreg[0];
}



/*###################################################################################################
**	PIXEL WRITES
**#################################################################################################*/

/* No Raster Op + No Transparency */
#define WP(m1,m2)  																			\
	UINT32 a = TOBYTE(offset & 0xfffffff0);													\
	UINT32 pix = TMS34010_RDMEM_WORD(a);													\
	UINT32 shiftcount = offset & m1;														\
																							\
	/* TODO: plane masking */																\
	data &= m2;																				\
	pix = (pix & ~(m2 << shiftcount)) | (data << shiftcount);								\
	TMS34010_WRMEM_WORD(a, pix);															\

/* No Raster Op + Transparency */
#define WP_T(m1,m2)  																		\
	/* TODO: plane masking */																\
	data &= m2;																				\
	if (data)																				\
	{																						\
		UINT32 a = TOBYTE(offset & 0xfffffff0);												\
		UINT32 pix = TMS34010_RDMEM_WORD(a);												\
		UINT32 shiftcount = offset & m1;													\
																							\
		/* TODO: plane masking */															\
		pix = (pix & ~(m2 << shiftcount)) | (data << shiftcount);							\
		TMS34010_WRMEM_WORD(a, pix);														\
	}						  																\

/* Raster Op + No Transparency */
#define WP_R(m1,m2)  																		\
	UINT32 a = TOBYTE(offset & 0xfffffff0);													\
	UINT32 pix = TMS34010_RDMEM_WORD(a);													\
	UINT32 shiftcount = offset & m1;														\
																							\
	/* TODO: plane masking */																\
	data = state.raster_op(data & m2, (pix >> shiftcount) & m2) & m2;						\
	pix = (pix & ~(m2 << shiftcount)) | (data << shiftcount);								\
	TMS34010_WRMEM_WORD(a, pix);															\

/* Raster Op + Transparency */
#define WP_R_T(m1,m2)  																		\
	UINT32 a = TOBYTE(offset & 0xfffffff0);													\
	UINT32 pix = TMS34010_RDMEM_WORD(a);													\
	UINT32 shiftcount = offset & m1;														\
																							\
	/* TODO: plane masking */																\
	data = state.raster_op(data & m2, (pix >> shiftcount) & m2) & m2;						\
	if (data)																				\
	{																						\
		pix = (pix & ~(m2 << shiftcount)) | (data << shiftcount);							\
		TMS34010_WRMEM_WORD(a, pix);														\
	}						  																\


/* No Raster Op + No Transparency */
static void write_pixel_1(offs_t offset,data32_t data) { WP(0x0f, 0x01); }
static void write_pixel_2(offs_t offset,data32_t data) { WP(0x0e, 0x03); }
static void write_pixel_4(offs_t offset,data32_t data) { WP(0x0c, 0x0f); }
static void write_pixel_8(offs_t offset,data32_t data) { WP(0x08, 0xff); }
static void write_pixel_16(offs_t offset,data32_t data)
{
	/* TODO: plane masking */
	TMS34010_WRMEM_WORD(TOBYTE(offset & 0xfffffff0), data);
}

/* No Raster Op + Transparency */
static void write_pixel_t_1(offs_t offset,data32_t data) { WP_T(0x0f, 0x01); }
static void write_pixel_t_2(offs_t offset,data32_t data) { WP_T(0x0e, 0x03); }
static void write_pixel_t_4(offs_t offset,data32_t data) { WP_T(0x0c, 0x0f); }
static void write_pixel_t_8(offs_t offset,data32_t data) { WP_T(0x08, 0xff); }
static void write_pixel_t_16(offs_t offset,data32_t data)
{
	/* TODO: plane masking */
	if (data)
		TMS34010_WRMEM_WORD(TOBYTE(offset & 0xfffffff0), data);
}

/* Raster Op + No Transparency */
static void write_pixel_r_1(offs_t offset,data32_t data) { WP_R(0x0f, 0x01); }
static void write_pixel_r_2(offs_t offset,data32_t data) { WP_R(0x0e, 0x03); }
static void write_pixel_r_4(offs_t offset,data32_t data) { WP_R(0x0c, 0x0f); }
static void write_pixel_r_8(offs_t offset,data32_t data) { WP_R(0x08, 0xff); }
static void write_pixel_r_16(offs_t offset,data32_t data)
{
	/* TODO: plane masking */
	UINT32 a = TOBYTE(offset & 0xfffffff0);
	TMS34010_WRMEM_WORD(a, state.raster_op(data, TMS34010_RDMEM_WORD(a)));
}

/* Raster Op + Transparency */
static void write_pixel_r_t_1(offs_t offset,data32_t data) { WP_R_T(0x0f,0x01); }
static void write_pixel_r_t_2(offs_t offset,data32_t data) { WP_R_T(0x0e,0x03); }
static void write_pixel_r_t_4(offs_t offset,data32_t data) { WP_R_T(0x0c,0x0f); }
static void write_pixel_r_t_8(offs_t offset,data32_t data) { WP_R_T(0x08,0xff); }
static void write_pixel_r_t_16(offs_t offset,data32_t data)
{
	/* TODO: plane masking */
	UINT32 a = TOBYTE(offset & 0xfffffff0);
	data = state.raster_op(data, TMS34010_RDMEM_WORD(a));

	if (data)
		TMS34010_WRMEM_WORD(a, data);
}

/* Shift register write */
static void write_pixel_shiftreg(offs_t offset,data32_t data)
{
	if (state.config->from_shiftreg)
		state.config->from_shiftreg(offset, &state.shiftreg[0]);
	else
		log_cb(RETRO_LOG_DEBUG, LOGPRE "From ShiftReg function not set. PC = %08X\n", PC);
}



/*###################################################################################################
**	RASTER OPS
**#################################################################################################*/

/* Raster operations */
static INT32 raster_op_1(INT32 newpix, INT32 oldpix)  { return newpix & oldpix; }
static INT32 raster_op_2(INT32 newpix, INT32 oldpix)  { return newpix & ~oldpix; }
static INT32 raster_op_3(INT32 newpix, INT32 oldpix)  { return 0; }
static INT32 raster_op_4(INT32 newpix, INT32 oldpix)  { return newpix | ~oldpix; }
static INT32 raster_op_5(INT32 newpix, INT32 oldpix)  { return ~(newpix ^ oldpix); }
static INT32 raster_op_6(INT32 newpix, INT32 oldpix)  { return ~oldpix; }
static INT32 raster_op_7(INT32 newpix, INT32 oldpix)  { return ~(newpix | oldpix); }
static INT32 raster_op_8(INT32 newpix, INT32 oldpix)  { return newpix | oldpix; }
static INT32 raster_op_9(INT32 newpix, INT32 oldpix)  { return oldpix; }
static INT32 raster_op_10(INT32 newpix, INT32 oldpix) { return newpix ^ oldpix; }
static INT32 raster_op_11(INT32 newpix, INT32 oldpix) { return ~newpix & oldpix; }
static INT32 raster_op_12(INT32 newpix, INT32 oldpix) { return 0xffff; }
static INT32 raster_op_13(INT32 newpix, INT32 oldpix) { return ~newpix | oldpix; }
static INT32 raster_op_14(INT32 newpix, INT32 oldpix) { return ~(newpix & oldpix); }
static INT32 raster_op_15(INT32 newpix, INT32 oldpix) { return ~newpix; }
static INT32 raster_op_16(INT32 newpix, INT32 oldpix) { return newpix + oldpix; }
static INT32 raster_op_17(INT32 newpix, INT32 oldpix)
{
	INT32 max = (UINT32)0xffffffff >> (32 - IOREG(REG_PSIZE));
	INT32 res = newpix + oldpix;
	return (res > max) ? max : res;
}
static INT32 raster_op_18(INT32 newpix, INT32 oldpix) { return oldpix - newpix; }
static INT32 raster_op_19(INT32 newpix, INT32 oldpix) { return (oldpix > newpix) ? oldpix - newpix : 0; }
static INT32 raster_op_20(INT32 newpix, INT32 oldpix) { return (oldpix > newpix) ? oldpix : newpix; }
static INT32 raster_op_21(INT32 newpix, INT32 oldpix) { return (oldpix > newpix) ? newpix : oldpix; }



/*###################################################################################################
**	OPCODE TABLE & IMPLEMENTATIONS
**#################################################################################################*/

/* includes the static function prototypes and the master opcode table */
#include "34010tbl.c"

/* includes the actual opcode implementations */
#include "34010ops.c"
#include "34010gfx.c"



/*###################################################################################################
**	Internal interrupt check
*#################################################################################################*/

/* Generate pending interrupts. Do NOT inline this function on DJGPP,
   it causes a slowdown */
static void check_interrupt(void)
{
	int vector = 0;
	int irqline = -1;
	int irq;

	/* early out if no interrupts pending */
	irq = IOREG(REG_INTPEND);
	if (!irq)
		return;

	/* check for NMI first */
	if (irq & TMS34010_NMI)
	{
		log_cb(RETRO_LOG_DEBUG, LOGPRE "TMS34010#%d takes NMI\n", cpu_getactivecpu());

		/* ack the NMI */
		IOREG(REG_INTPEND) &= ~TMS34010_NMI;

		/* handle NMI mode bit */
		if (!(IOREG(REG_HSTCTLH) & 0x0200))
		{
			PUSH(PC);
			PUSH(GET_ST());
		}

		/* leap to the vector */
		RESET_ST();
		PC = RLONG(0xfffffee0);
		change_pc29lew(TOBYTE(PC));
		return;
	}

	/* early out if everything else is disabled */
	irq &= IOREG(REG_INTENB);
	if (!IE_FLAG || !irq)
		return;

	/* host interrupt */
	if (irq & TMS34010_HI)
	{
		log_cb(RETRO_LOG_DEBUG, LOGPRE "TMS34010#%d takes HI\n", cpu_getactivecpu());
		vector = 0xfffffec0;
	}

	/* display interrupt */
	else if (irq & TMS34010_DI)
	{
		log_cb(RETRO_LOG_DEBUG, LOGPRE "TMS34010#%d takes DI\n", cpu_getactivecpu());
		vector = 0xfffffea0;
	}

	/* window violation interrupt */
	else if (irq & TMS34010_WV)
	{
		log_cb(RETRO_LOG_DEBUG, LOGPRE "TMS34010#%d takes WV\n", cpu_getactivecpu());
		vector = 0xfffffe80;
	}

	/* external 1 interrupt */
	else if (irq & TMS34010_INT1)
	{
		log_cb(RETRO_LOG_DEBUG, LOGPRE "TMS34010#%d takes INT1\n", cpu_getactivecpu());
		vector = 0xffffffc0;
		irqline = 0;
	}

	/* external 2 interrupt */
	else if (irq & TMS34010_INT2)
	{
		log_cb(RETRO_LOG_DEBUG, LOGPRE "TMS34010#%d takes INT2\n", cpu_getactivecpu());
		vector = 0xffffffa0;
		irqline = 1;
	}

	/* if we took something, generate it */
	if (vector)
	{
		PUSH(PC);
		PUSH(GET_ST());
		RESET_ST();
		PC = RLONG(vector);
		change_pc29lew(TOBYTE(PC));

		/* call the callback for externals */
		if (irqline >= 0)
			(void)(*state.irq_callback)(irqline);
	}
}



/*###################################################################################################
**	Reset the CPU emulation
**#################################################################################################*/

void tms34010_init(void)
{
	int cpunum;
	int i;

	external_host_access = 0;

	for (i = 0; i < MAX_CPU; i++)
	{
		dpyint_timer[i] = timer_alloc(dpyint_callback);
		vsblnk_timer[i] = timer_alloc(vsblnk_callback);
	}

	cpunum = cpu_getactivecpu();
	state_save_register_UINT32("tms34010", cpunum, "OP",        &state.op, 1);
	state_save_register_UINT32("tms34010", cpunum, "PC",        &state.pc, 1);
	state_save_register_UINT32("tms34010", cpunum, "ST",        &state.st, 1);
	state_save_register_UINT32("tms34010", cpunum, "AREGS",     state.flat_aregs, 16);
	state_save_register_UINT32("tms34010", cpunum, "BREGS",     state.flat_bregs, 15);
	state_save_register_UINT32("tms34010", cpunum, "NFLAG",     &state.nflag, 1);
	state_save_register_UINT32("tms34010", cpunum, "CFLAG",     &state.cflag, 1);
	state_save_register_UINT32("tms34010", cpunum, "NOTZFLAG",  &state.notzflag, 1);
	state_save_register_UINT32("tms34010", cpunum, "VFLAG",     &state.vflag, 1);
	state_save_register_UINT32("tms34010", cpunum, "PFLAG",     &state.pflag, 1);
	state_save_register_UINT32("tms34010", cpunum, "IEFLAG",    &state.ieflag, 1);
	state_save_register_UINT32("tms34010", cpunum, "FE0FLAG",   &state.fe0flag, 1);
	state_save_register_UINT32("tms34010", cpunum, "FE1FLAG",   &state.fe1flag, 1);
	state_save_register_UINT32("tms34010", cpunum, "FW",        state.fw, sizeof(state.fw) / sizeof(state.fw[0]));
	state_save_register_UINT32("tms34010", cpunum, "FW_INC",    state.fw_inc, sizeof(state.fw_inc) / sizeof(state.fw_inc[0]));
	state_save_register_UINT32("tms34010", cpunum, "RESET_DEF", &state.reset_deferred, 1);
	state_save_register_UINT16("tms34010", cpunum, "SHIFTREG",  state.shiftreg, SHIFTREG_SIZE / sizeof(UINT16));
	state_save_register_UINT16("tms34010", cpunum, "IORegs",    state.IOregs, 16);
	state_save_register_UINT32("tms34010", cpunum, "TRANSPAR",  &state.transparency, 1);
	state_save_register_UINT32("tms34010", cpunum, "WINCHK",    &state.window_checking, 1);
	state_save_register_UINT32("tms34010", cpunum, "CONVSP",    &state.convsp, 1);
	state_save_register_UINT32("tms34010", cpunum, "CONVDP",    &state.convdp, 1);
	state_save_register_UINT32("tms34010", cpunum, "CONVMP",    &state.convmp, 1);
	state_save_register_UINT32("tms34010", cpunum, "PIXELSHFT", &state.pixelshift, 1);
	state_save_register_int("tms34010", cpunum, "gfxcycles",        &state.gfxcycles);
	state_save_register_int("tms34010", cpunum, "luvcount",     &state.last_update_vcount);
	state_save_register_int("tms34010", cpunum, "ICount",       &tms34010_ICount);
	state_save_register_func_presave(tms34010_state_presave);
	state_save_register_func_postload(tms34010_state_postload);
}

void tms34010_reset(void *param)
{
	struct tms34010_config *config = param ? param : &default_config;

	/* zap the state and copy in the config pointer */
	memset(&state, 0, sizeof(state));
	state.config = config;

	/* allocate the shiftreg */
	state.shiftreg = malloc(SHIFTREG_SIZE);

	/* fetch the initial PC and reset the state */
	PC = RLONG(0xffffffe0) & 0xfffffff0;
	change_pc29lew(TOBYTE(PC));
	RESET_ST();

	/* HALT the CPU if requested, and remember to re-read the starting PC */
	/* the first time we are run */
	state.reset_deferred = config->halt_on_reset;
	if (config->halt_on_reset)
		tms34010_io_register_w(REG_HSTCTLH, 0x8000, 0);
}

void tms34020_init(void)
{
	tms34010_init();
}

void tms34020_reset(void *param)
{
	tms34010_reset(param);
	state.is_34020 = 1;
}



/*###################################################################################################
**	Shut down the CPU emulation
**#################################################################################################*/

void tms34010_exit(void)
{
	int i;

	/* clear out the timers */
	for (i = 0; i < MAX_CPU; i++)
		dpyint_timer[i] = vsblnk_timer[i] = NULL;

	/* free memory */
	if (state.shiftreg)
		free(state.shiftreg);
	state.shiftreg = NULL;
}

void tms34020_exit(void)
{
	tms34010_exit();
}



/*###################################################################################################
**	Get all registers in given buffer
**#################################################################################################*/

unsigned tms34010_get_context(void *dst)
{
	if (dst)
	{
		int i;

		for (i = 0; i < 16; i++)
			state.flat_aregs[i] = AREG(i);
		for (i = 0; i < 15; i++)
			state.flat_bregs[i] = BREG(BINDEX(i));
		memcpy(dst, &state, TMS34010_STATE_SIZE);
	}
	return TMS34010_STATE_SIZE;
}

unsigned tms34020_get_context(void *dst)
{
	if (dst)
	{
		int i;

		for (i = 0; i < 16; i++)
			state.flat_aregs[i] = AREG(i);
		for (i = 0; i < 15; i++)
			state.flat_bregs[i] = BREG(BINDEX(i));
		memcpy(dst, &state, TMS34020_STATE_SIZE);
	}
	return TMS34020_STATE_SIZE;
}



/*###################################################################################################
**	Set all registers to given values
**#################################################################################################*/

void tms34010_set_context(void *src)
{
	if (src)
	{
		int i;

		memcpy(&state, src, TMS34010_STATE_SIZE);
		for (i = 0; i < 16; i++)
			AREG(i) = state.flat_aregs[i];
		for (i = 0; i < 15; i++)
			BREG(BINDEX(i)) = state.flat_bregs[i];
	}
	change_pc29lew(TOBYTE(PC));
	check_interrupt();
}

void tms34020_set_context(void *src)
{
	if (src)
	{
		int i;

		memcpy(&state, src, TMS34020_STATE_SIZE);
		for (i = 0; i < 16; i++)
			AREG(i) = state.flat_aregs[i];
		for (i = 0; i < 15; i++)
			BREG(BINDEX(i)) = state.flat_bregs[i];
	}
	change_pc29lew(TOBYTE(PC));
	check_interrupt();
}



/*###################################################################################################
**	Return a specific register
**#################################################################################################*/

unsigned tms34010_get_reg(int regnum)
{
	switch (regnum)
	{
		case REG_PC:
		case TMS34010_PC:  return PC;
		case REG_SP:
		case TMS34010_SP:  return SP;
		case TMS34010_ST:  return ST;
		case TMS34010_A0:  return AREG(0);
		case TMS34010_A1:  return AREG(1);
		case TMS34010_A2:  return AREG(2);
		case TMS34010_A3:  return AREG(3);
		case TMS34010_A4:  return AREG(4);
		case TMS34010_A5:  return AREG(5);
		case TMS34010_A6:  return AREG(6);
		case TMS34010_A7:  return AREG(7);
		case TMS34010_A8:  return AREG(8);
		case TMS34010_A9:  return AREG(9);
		case TMS34010_A10: return AREG(10);
		case TMS34010_A11: return AREG(11);
		case TMS34010_A12: return AREG(12);
		case TMS34010_A13: return AREG(13);
		case TMS34010_A14: return AREG(14);
		case TMS34010_B0:  return BREG(BINDEX(0));
		case TMS34010_B1:  return BREG(BINDEX(1));
		case TMS34010_B2:  return BREG(BINDEX(2));
		case TMS34010_B3:  return BREG(BINDEX(3));
		case TMS34010_B4:  return BREG(BINDEX(4));
		case TMS34010_B5:  return BREG(BINDEX(5));
		case TMS34010_B6:  return BREG(BINDEX(6));
		case TMS34010_B7:  return BREG(BINDEX(7));
		case TMS34010_B8:  return BREG(BINDEX(8));
		case TMS34010_B9:  return BREG(BINDEX(9));
		case TMS34010_B10: return BREG(BINDEX(10));
		case TMS34010_B11: return BREG(BINDEX(11));
		case TMS34010_B12: return BREG(BINDEX(12));
		case TMS34010_B13: return BREG(BINDEX(13));
		case TMS34010_B14: return BREG(BINDEX(14));
/* TODO: return contents of [SP + wordsize * (CPU_SP_CONTENTS-regnum)] */
		default:
			if (regnum <= REG_SP_CONTENTS)
			{
				unsigned offset = SP + 4 * (REG_SP_CONTENTS - regnum);
				return (cpu_readmem29lew_word(TOBYTE(offset)+2) << 16) | cpu_readmem29lew_word(TOBYTE(offset));
			}
	}
	return 0;
}

unsigned tms34020_get_reg(int regnum)
{
	return tms34010_get_reg(regnum);
}



/*###################################################################################################
**	Set a specific register
**#################################################################################################*/

void tms34010_set_reg(int regnum, unsigned val)
{
	switch (regnum)
	{
		case REG_PC:       PC = val; change_pc29lew(TOBYTE(PC)); break;
		case TMS34010_PC:  PC = val; break;
		case REG_SP:
		case TMS34010_SP:  SP = val; break;
		case TMS34010_ST:  ST = val; break;
		case TMS34010_A0:  AREG(0) = val; break;
		case TMS34010_A1:  AREG(1) = val; break;
		case TMS34010_A2:  AREG(2) = val; break;
		case TMS34010_A3:  AREG(3) = val; break;
		case TMS34010_A4:  AREG(4) = val; break;
		case TMS34010_A5:  AREG(5) = val; break;
		case TMS34010_A6:  AREG(6) = val; break;
		case TMS34010_A7:  AREG(7) = val; break;
		case TMS34010_A8:  AREG(8) = val; break;
		case TMS34010_A9:  AREG(9) = val; break;
		case TMS34010_A10: AREG(10) = val; break;
		case TMS34010_A11: AREG(11) = val; break;
		case TMS34010_A12: AREG(12) = val; break;
		case TMS34010_A13: AREG(13) = val; break;
		case TMS34010_A14: AREG(14) = val; break;
		case TMS34010_B0:  BREG(BINDEX(0)) = val; break;
		case TMS34010_B1:  BREG(BINDEX(1)) = val; break;
		case TMS34010_B2:  BREG(BINDEX(2)) = val; break;
		case TMS34010_B3:  BREG(BINDEX(3)) = val; break;
		case TMS34010_B4:  BREG(BINDEX(4)) = val; break;
		case TMS34010_B5:  BREG(BINDEX(5)) = val; break;
		case TMS34010_B6:  BREG(BINDEX(6)) = val; break;
		case TMS34010_B7:  BREG(BINDEX(7)) = val; break;
		case TMS34010_B8:  BREG(BINDEX(8)) = val; break;
		case TMS34010_B9:  BREG(BINDEX(9)) = val; break;
		case TMS34010_B10: BREG(BINDEX(10)) = val; break;
		case TMS34010_B11: BREG(BINDEX(11)) = val; break;
		case TMS34010_B12: BREG(BINDEX(12)) = val; break;
		case TMS34010_B13: BREG(BINDEX(13)) = val; break;
		case TMS34010_B14: BREG(BINDEX(14)) = val; break;
/* TODO: set contents of [SP + wordsize * (CPU_SP_CONTENTS-regnum)] */
		default:
			if (regnum <= REG_SP_CONTENTS)
			{
				unsigned offset = SP + 4 * (REG_SP_CONTENTS - regnum);
				cpu_writemem29lew_word(TOBYTE(offset), val); /* ??? */
			}
	}
}

void tms34020_set_reg(int regnum, unsigned val)
{
	tms34010_set_reg(regnum, val);
}



/*###################################################################################################
**	Set IRQ line state
**#################################################################################################*/

void tms34010_set_irq_line(int irqline, int linestate)
{
	log_cb(RETRO_LOG_DEBUG, LOGPRE "TMS34010#%d set irq line %d state %d\n", cpu_getactivecpu(), irqline, linestate);

	/* set the pending interrupt */
	switch (irqline)
	{
		case 0:
			if (linestate != CLEAR_LINE)
			{
				state.ext_irq_lines |= 1;
				IOREG(REG_INTPEND) |= TMS34010_INT1;
			}
			else
			{
				state.ext_irq_lines &= ~1;
				IOREG(REG_INTPEND) &= ~TMS34010_INT1;
			}
			break;

		case 1:
			if (linestate != CLEAR_LINE)
			{
				state.ext_irq_lines |= 2;
				IOREG(REG_INTPEND) |= TMS34010_INT2;
			}
			else
			{
				state.ext_irq_lines &= ~2;
				IOREG(REG_INTPEND) &= ~TMS34010_INT2;
			}
			break;
	}
	check_interrupt();
}

void tms34020_set_irq_line(int irqline, int linestate)
{
	tms34010_set_irq_line(irqline, linestate);
}



/*###################################################################################################
**	Set IRQ callback
**#################################################################################################*/

void tms34010_set_irq_callback(int (*callback)(int irqline))
{
	state.irq_callback = callback;
}

void tms34020_set_irq_callback(int (*callback)(int irqline))
{
	tms34010_set_irq_callback(callback);
}



/*###################################################################################################
**	Generate internal interrupt
*#################################################################################################*/

static void internal_interrupt_callback(int param)
{
	int cpunum = param & 0xff;
	int type = param >> 8;

	/* call through to the CPU to generate the int */
	cpuintrf_push_context(cpunum);
	IOREG(REG_INTPEND) |= type;
	log_cb(RETRO_LOG_DEBUG, LOGPRE "TMS34010#%d set internal interrupt $%04x\n", cpu_getactivecpu(), type);
	check_interrupt();
	cpuintrf_pop_context();

	/* generate triggers so that spin loops can key off them */
	cpu_triggerint(cpunum);
}



/*###################################################################################################
**	Execute
*#################################################################################################*/

int tms34010_execute(int cycles)
{
	/* Get out if CPU is halted. Absolutely no interrupts must be taken!!! */
	if (IOREG(REG_HSTCTLH) & 0x8000)
		return cycles;

	/* if the CPU's reset was deferred, do it now */
	if (state.reset_deferred)
	{
		state.reset_deferred = 0;
		PC = RLONG(0xffffffe0);
	}

	/* execute starting now */
	tms34010_ICount = cycles;
	change_pc29lew(TOBYTE(PC));
	do
	{
		#ifdef	MAME_DEBUG
		if (mame_debug) { state.st = GET_ST(); MAME_Debug(); }
		#endif
		state.op = ROPCODE();
		(*opcode_table[state.op >> 4])();

	} while (tms34010_ICount > 0);

	return cycles - tms34010_ICount;
}

int tms34020_execute(int cycles)
{
	return tms34010_execute(cycles);
}



/*###################################################################################################
**	Return a formatted string for a register
*#################################################################################################*/

const char *tms34010_info(void *context, int regnum)
{
	static char buffer[40][63+1];
	static int which = 0;
	tms34010_regs *r = context;

	which = (which+1) % 40;
	buffer[which][0] = '\0';
	if (!context)
		r = &state;

	switch (regnum)
	{
		case CPU_INFO_NAME: return "TMS34010";
		case CPU_INFO_FAMILY: return "Texas Instruments 34010";
		case CPU_INFO_VERSION: return "1.0";
		case CPU_INFO_FILE: return __FILE__;
		case CPU_INFO_CREDITS: return "Copyright (C) Alex Pasadyn/Zsolt Vasvari 1998\nParts based on code by Aaron Giles";
		case CPU_INFO_REG_LAYOUT: return (const char *)tms34010_reg_layout;
		case CPU_INFO_WIN_LAYOUT: return (const char *)tms34010_win_layout;

		case CPU_INFO_FLAGS:
			sprintf(buffer[which], "%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c",
				r->st & 0x80000000 ? 'N':'.',
				r->st & 0x40000000 ? 'C':'.',
				r->st & 0x20000000 ? 'Z':'.',
				r->st & 0x10000000 ? 'V':'.',
				r->st & 0x08000000 ? '?':'.',
				r->st & 0x04000000 ? '?':'.',
				r->st & 0x02000000 ? 'P':'.',
				r->st & 0x01000000 ? '?':'.',
				r->st & 0x00800000 ? '?':'.',
				r->st & 0x00400000 ? '?':'.',
				r->st & 0x00200000 ? 'I':'.',
				r->st & 0x00100000 ? '?':'.',
				r->st & 0x00080000 ? '?':'.',
				r->st & 0x00040000 ? '?':'.',
				r->st & 0x00020000 ? '?':'.',
				r->st & 0x00010000 ? '?':'.',
				r->st & 0x00008000 ? '?':'.',
				r->st & 0x00004000 ? '?':'.',
				r->st & 0x00002000 ? '?':'.',
				r->st & 0x00001000 ? '?':'.',
				r->st & 0x00000800 ? 'E':'.',
				r->st & 0x00000400 ? 'F':'.',
				r->st & 0x00000200 ? 'F':'.',
				r->st & 0x00000100 ? 'F':'.',
				r->st & 0x00000080 ? 'F':'.',
				r->st & 0x00000040 ? 'F':'.',
				r->st & 0x00000020 ? 'E':'.',
				r->st & 0x00000010 ? 'F':'.',
				r->st & 0x00000008 ? 'F':'.',
				r->st & 0x00000004 ? 'F':'.',
				r->st & 0x00000002 ? 'F':'.',
				r->st & 0x00000001 ? 'F':'.');
			break;
		case CPU_INFO_REG+TMS34010_PC: sprintf(buffer[which], "PC :%08X", r->pc); break;
		case CPU_INFO_REG+TMS34010_SP: sprintf(buffer[which], "SP :%08X", (r == &state) ? r->regs.a.a.Aregs[15] : r->flat_aregs[15]); break;
		case CPU_INFO_REG+TMS34010_ST: sprintf(buffer[which], "ST :%08X", r->st); break;
		case CPU_INFO_REG+TMS34010_A0: sprintf(buffer[which], "A0 :%08X", (r == &state) ? r->regs.a.a.Aregs[ 0] : r->flat_aregs[ 0]); break;
		case CPU_INFO_REG+TMS34010_A1: sprintf(buffer[which], "A1 :%08X", (r == &state) ? r->regs.a.a.Aregs[ 1] : r->flat_aregs[ 1]); break;
		case CPU_INFO_REG+TMS34010_A2: sprintf(buffer[which], "A2 :%08X", (r == &state) ? r->regs.a.a.Aregs[ 2] : r->flat_aregs[ 2]); break;
		case CPU_INFO_REG+TMS34010_A3: sprintf(buffer[which], "A3 :%08X", (r == &state) ? r->regs.a.a.Aregs[ 3] : r->flat_aregs[ 3]); break;
		case CPU_INFO_REG+TMS34010_A4: sprintf(buffer[which], "A4 :%08X", (r == &state) ? r->regs.a.a.Aregs[ 4] : r->flat_aregs[ 4]); break;
		case CPU_INFO_REG+TMS34010_A5: sprintf(buffer[which], "A5 :%08X", (r == &state) ? r->regs.a.a.Aregs[ 5] : r->flat_aregs[ 5]); break;
		case CPU_INFO_REG+TMS34010_A6: sprintf(buffer[which], "A6 :%08X", (r == &state) ? r->regs.a.a.Aregs[ 6] : r->flat_aregs[ 6]); break;
		case CPU_INFO_REG+TMS34010_A7: sprintf(buffer[which], "A7 :%08X", (r == &state) ? r->regs.a.a.Aregs[ 7] : r->flat_aregs[ 7]); break;
		case CPU_INFO_REG+TMS34010_A8: sprintf(buffer[which], "A8 :%08X", (r == &state) ? r->regs.a.a.Aregs[ 8] : r->flat_aregs[ 8]); break;
		case CPU_INFO_REG+TMS34010_A9: sprintf(buffer[which], "A9 :%08X", (r == &state) ? r->regs.a.a.Aregs[ 9] : r->flat_aregs[ 9]); break;
		case CPU_INFO_REG+TMS34010_A10: sprintf(buffer[which],"A10:%08X", (r == &state) ? r->regs.a.a.Aregs[10] : r->flat_aregs[10]); break;
		case CPU_INFO_REG+TMS34010_A11: sprintf(buffer[which],"A11:%08X", (r == &state) ? r->regs.a.a.Aregs[11] : r->flat_aregs[11]); break;
		case CPU_INFO_REG+TMS34010_A12: sprintf(buffer[which],"A12:%08X", (r == &state) ? r->regs.a.a.Aregs[12] : r->flat_aregs[12]); break;
		case CPU_INFO_REG+TMS34010_A13: sprintf(buffer[which],"A13:%08X", (r == &state) ? r->regs.a.a.Aregs[13] : r->flat_aregs[13]); break;
		case CPU_INFO_REG+TMS34010_A14: sprintf(buffer[which],"A14:%08X", (r == &state) ? r->regs.a.a.Aregs[14] : r->flat_aregs[14]); break;
		case CPU_INFO_REG+TMS34010_B0: sprintf(buffer[which], "B0 :%08X", (r == &state) ? r->regs.Bregs[BINDEX( 0)] : r->flat_bregs[ 0]); break;
		case CPU_INFO_REG+TMS34010_B1: sprintf(buffer[which], "B1 :%08X", (r == &state) ? r->regs.Bregs[BINDEX( 1)] : r->flat_bregs[ 1]); break;
		case CPU_INFO_REG+TMS34010_B2: sprintf(buffer[which], "B2 :%08X", (r == &state) ? r->regs.Bregs[BINDEX( 2)] : r->flat_bregs[ 2]); break;
		case CPU_INFO_REG+TMS34010_B3: sprintf(buffer[which], "B3 :%08X", (r == &state) ? r->regs.Bregs[BINDEX( 3)] : r->flat_bregs[ 3]); break;
		case CPU_INFO_REG+TMS34010_B4: sprintf(buffer[which], "B4 :%08X", (r == &state) ? r->regs.Bregs[BINDEX( 4)] : r->flat_bregs[ 4]); break;
		case CPU_INFO_REG+TMS34010_B5: sprintf(buffer[which], "B5 :%08X", (r == &state) ? r->regs.Bregs[BINDEX( 5)] : r->flat_bregs[ 5]); break;
		case CPU_INFO_REG+TMS34010_B6: sprintf(buffer[which], "B6 :%08X", (r == &state) ? r->regs.Bregs[BINDEX( 6)] : r->flat_bregs[ 6]); break;
		case CPU_INFO_REG+TMS34010_B7: sprintf(buffer[which], "B7 :%08X", (r == &state) ? r->regs.Bregs[BINDEX( 7)] : r->flat_bregs[ 7]); break;
		case CPU_INFO_REG+TMS34010_B8: sprintf(buffer[which], "B8 :%08X", (r == &state) ? r->regs.Bregs[BINDEX( 8)] : r->flat_bregs[ 8]); break;
		case CPU_INFO_REG+TMS34010_B9: sprintf(buffer[which], "B9 :%08X", (r == &state) ? r->regs.Bregs[BINDEX( 9)] : r->flat_bregs[ 9]); break;
		case CPU_INFO_REG+TMS34010_B10: sprintf(buffer[which],"B10:%08X", (r == &state) ? r->regs.Bregs[BINDEX(10)] : r->flat_bregs[10]); break;
		case CPU_INFO_REG+TMS34010_B11: sprintf(buffer[which],"B11:%08X", (r == &state) ? r->regs.Bregs[BINDEX(11)] : r->flat_bregs[11]); break;
		case CPU_INFO_REG+TMS34010_B12: sprintf(buffer[which],"B12:%08X", (r == &state) ? r->regs.Bregs[BINDEX(12)] : r->flat_bregs[12]); break;
		case CPU_INFO_REG+TMS34010_B13: sprintf(buffer[which],"B13:%08X", (r == &state) ? r->regs.Bregs[BINDEX(13)] : r->flat_bregs[13]); break;
		case CPU_INFO_REG+TMS34010_B14: sprintf(buffer[which],"B14:%08X", (r == &state) ? r->regs.Bregs[BINDEX(14)] : r->flat_bregs[14]); break;
	}
	return buffer[which];
}

const char *tms34020_info(void *context, int regnum)
{
	switch (regnum)
	{
		case CPU_INFO_NAME: 	return "TMS34020";
		case CPU_INFO_FAMILY: 	return "Texas Instruments 34020";
		default:				return tms34010_info(context, regnum);
	}
}



/*###################################################################################################
**	Disassemble
*#################################################################################################*/

unsigned tms34010_dasm(char *buffer, unsigned pc)
{
#ifdef MAME_DEBUG
	return Dasm34010(buffer,pc);
#else
	sprintf( buffer, "$%04X", cpu_readop16(pc>>3) );
	return 2;
#endif
}

unsigned tms34020_dasm(char *buffer, unsigned pc)
{
#ifdef MAME_DEBUG
	return Dasm34020(buffer,pc);
#else
	sprintf( buffer, "$%04X", cpu_readop16(pc>>3) );
	return 2;
#endif
}



/*###################################################################################################
**	PIXEL OPS
**#################################################################################################*/

static void (*pixel_write_ops[4][5])(offs_t offset,data32_t data) =
{
	{ write_pixel_1,     write_pixel_2,     write_pixel_4,     write_pixel_8,     write_pixel_16     },
	{ write_pixel_r_1,   write_pixel_r_2,   write_pixel_r_4,   write_pixel_r_8,   write_pixel_r_16   },
	{ write_pixel_t_1,   write_pixel_t_2,   write_pixel_t_4,   write_pixel_t_8,   write_pixel_t_16   },
	{ write_pixel_r_t_1, write_pixel_r_t_2, write_pixel_r_t_4, write_pixel_r_t_8, write_pixel_r_t_16 }
};

static data32_t (*pixel_read_ops[5])(offs_t offset) =
{
	read_pixel_1,        read_pixel_2,      read_pixel_4,      read_pixel_8,      read_pixel_16
};


static void set_pixel_function(void)
{
	UINT32 i1,i2;

	if (IOREG(REG_DPYCTL) & 0x0800)
	{
		/* Shift Register Transfer */
		state.pixel_write = write_pixel_shiftreg;
		state.pixel_read  = read_pixel_shiftreg;
		return;
	}

	switch (IOREG(REG_PSIZE))
	{
		default:
		case 0x01: i2 = 0; break;
		case 0x02: i2 = 1; break;
		case 0x04: i2 = 2; break;
		case 0x08: i2 = 3; break;
		case 0x10: i2 = 4; break;
	}

	if (state.transparency)
		i1 = state.raster_op ? 3 : 2;
	else
		i1 = state.raster_op ? 1 : 0;

	state.pixel_write = pixel_write_ops[i1][i2];
	state.pixel_read  = pixel_read_ops [i2];
}



/*###################################################################################################
**	RASTER OPS
**#################################################################################################*/

static INT32 (*raster_ops[32]) (INT32 newpix, INT32 oldpix) =
{
	           0, raster_op_1 , raster_op_2 , raster_op_3,
	raster_op_4 , raster_op_5 , raster_op_6 , raster_op_7,
	raster_op_8 , raster_op_9 , raster_op_10, raster_op_11,
	raster_op_12, raster_op_13, raster_op_14, raster_op_15,
	raster_op_16, raster_op_17, raster_op_18, raster_op_19,
	raster_op_20, raster_op_21,            0,            0,
	           0,            0,            0,            0,
	           0,            0,            0,            0,
};


static void set_raster_op(void)
{
	state.raster_op = raster_ops[(IOREG(REG_CONTROL) >> 10) & 0x1f];
}



/*###################################################################################################
**	VIDEO TIMING HELPERS
**#################################################################################################*/

static INLINE int scanline_to_vcount(int scanline)
{
	if (Machine->visible_area.min_y == 0)
		scanline += SMART_IOREG(VEBLNK);
	if (scanline > SMART_IOREG(VTOTAL))
		scanline -= SMART_IOREG(VTOTAL);
	return scanline;
}


static INLINE int vcount_to_scanline(int vcount)
{
	if (Machine->visible_area.min_y == 0)
		vcount -= SMART_IOREG(VEBLNK);
	if (vcount < 0)
		vcount += SMART_IOREG(VTOTAL);
	return vcount;
}


static void update_display_address(int vcount)
{
	UINT32 dpyadr = IOREG(REG_DPYADR) & 0xfffc;
	UINT32 dpytap = IOREG(REG_DPYTAP) & 0x3fff;
	INT32 dudate = IOREG(REG_DPYCTL) & 0x03fc;
	int org = IOREG(REG_DPYCTL) & 0x0400;
	int scans = (IOREG(REG_DPYSTRT) & 3) + 1;

	/* anytime during VBLANK is effectively the start of the next frame */
	if (vcount >= SMART_IOREG(VSBLNK) || vcount <= SMART_IOREG(VEBLNK))
		state.last_update_vcount = vcount = SMART_IOREG(VEBLNK);

	/* otherwise, compute the updated address */
	else
	{
		int rows = vcount - state.last_update_vcount;
		if (rows < 0) rows += SMART_IOREG(VCOUNT);
		dpyadr -= rows * dudate / scans;
		IOREG(REG_DPYADR) = dpyadr | (IOREG(REG_DPYADR) & 0x0003);
		state.last_update_vcount = vcount;
	}

	/* now compute the actual address */
	if (org == 0) dpyadr ^= 0xfffc;
	dpyadr <<= 8;
	dpyadr |= dpytap << 4;

	/* callback */
	if (state.config->display_addr_changed)
	{
		if (org != 0) dudate = -dudate;
		(*state.config->display_addr_changed)(dpyadr & 0x00ffffff, (dudate << 8) / scans, vcount_to_scanline(vcount));
	}
}


static void vsblnk_callback(int cpunum)
{
	double interval = TIME_IN_HZ(Machine->drv->frames_per_second);

	/* reset timer for next frame before going into the CPU context */
	timer_adjust(vsblnk_timer[cpunum], interval, cpunum, 0);

	/* set the CPU's context and update the display state */
	cpuintrf_push_context(cpunum);
	IOREG(REG_DPYADR) = IOREG(REG_DPYSTRT);
	update_display_address(SMART_IOREG(VSBLNK));

	cpuintrf_pop_context();
}


static void dpyint_callback(int cpunum)
{
	double interval = TIME_IN_HZ(Machine->drv->frames_per_second);

  log_cb(RETRO_LOG_DEBUG, LOGPRE "-- dpyint(%d) @ %d --\n", cpunum, cpu_getscanline());

	/* reset timer for next frame before going into the CPU context */
	timer_adjust(dpyint_timer[cpunum], interval, cpunum, 0);

	/* set the CPU's context and queue an interrupt */
	cpuintrf_push_context(cpunum);
	timer_set(TIME_NOW, cpunum | (TMS34010_DI << 8), internal_interrupt_callback);

	/* allow a callback so we can update before they are likely to do nasty things */
	if (state.config->display_int_callback)
		(*state.config->display_int_callback)(vcount_to_scanline(IOREG(REG_DPYINT)));

	cpuintrf_pop_context();
}


static void update_timers(void)
{
	int cpunum = cpu_getactivecpu();
	int dpyint = IOREG(REG_DPYINT);
	int vsblnk = SMART_IOREG(VSBLNK);

	/* set new timers */
	timer_adjust(dpyint_timer[cpunum], cpu_getscanlinetime(vcount_to_scanline(dpyint)), cpunum, 0);
	timer_adjust(vsblnk_timer[cpunum], cpu_getscanlinetime(vcount_to_scanline(vsblnk)), cpunum, 0);
}



/*###################################################################################################
**	I/O REGISTER WRITES
**#################################################################################################*/

#if 0
static const char *ioreg_name[] =
{
	"HESYNC", "HEBLNK", "HSBLNK", "HTOTAL",
	"VESYNC", "VEBLNK", "VSBLNK", "VTOTAL",
	"DPYCTL", "DPYSTART", "DPYINT", "CONTROL",
	"HSTDATA", "HSTADRL", "HSTADRH", "HSTCTLL",

	"HSTCTLH", "INTENB", "INTPEND", "CONVSP",
	"CONVDP", "PSIZE", "PMASK", "RESERVED",
	"RESERVED", "RESERVED", "RESERVED", "DPYTAP",
	"HCOUNT", "VCOUNT", "DPYADR", "REFCNT"
};
#endif

WRITE16_HANDLER( tms34010_io_register_w )
{
	int cpunum = cpu_getactivecpu();
	int oldreg, newreg;

	/* Set register */
	oldreg = IOREG(offset);
	IOREG(offset) = data;

	switch (offset)
	{
		case REG_DPYINT:
			if (data != oldreg || !dpyint_timer[cpunum])
				update_timers();
			break;

		case REG_VSBLNK:
			if (data != oldreg || !vsblnk_timer[cpunum])
				update_timers();
			break;

		case REG_VEBLNK:
			if (data != oldreg)
				update_timers();
			break;

		case REG_CONTROL:
			state.transparency = data & 0x20;
			state.window_checking = (data >> 6) & 0x03;
			set_raster_op();
			set_pixel_function();
			break;

		case REG_PSIZE:
			set_pixel_function();

			switch (data)
			{
				default:
				case 0x01: state.pixelshift = 0; break;
				case 0x02: state.pixelshift = 1; break;
				case 0x04: state.pixelshift = 2; break;
				case 0x08: state.pixelshift = 3; break;
				case 0x10: state.pixelshift = 4; break;
			}
			break;

		case REG_PMASK:
			if (data) log_cb(RETRO_LOG_DEBUG, LOGPRE "Plane masking not supported. PC=%08X\n", activecpu_get_pc());
			break;

		case REG_DPYCTL:
			set_pixel_function();
			if ((oldreg ^ data) & 0x03fc)
				update_display_address(scanline_to_vcount(cpu_getscanline()));
			break;

		case REG_DPYADR:
			if (data != oldreg)
			{
				state.last_update_vcount = scanline_to_vcount(cpu_getscanline());
				update_display_address(state.last_update_vcount);
			}
			break;

		case REG_DPYSTRT:
			if (data != oldreg)
				update_display_address(scanline_to_vcount(cpu_getscanline()));
			break;

		case REG_DPYTAP:
			if ((oldreg ^ data) & 0x3fff)
				update_display_address(scanline_to_vcount(cpu_getscanline()));
			break;

		case REG_HSTCTLH:
			/* if the CPU is halting itself, stop execution right away */
			if ((data & 0x8000) && !external_host_access)
				tms34010_ICount = 0;
			cpu_set_halt_line(cpunum, (data & 0x8000) ? ASSERT_LINE : CLEAR_LINE);

			/* NMI issued? */
			if (data & 0x0100)
				timer_set(TIME_NOW, cpunum | (TMS34010_NMI << 8), internal_interrupt_callback);
			break;

		case REG_HSTCTLL:
			/* the TMS34010 can change MSGOUT, can set INTOUT, and can clear INTIN */
			if (!external_host_access)
			{
				newreg = (oldreg & 0xff8f) | (data & 0x0070);
				newreg |= data & 0x0080;
				newreg &= data | ~0x0008;
			}

			/* the host can change MSGIN, can set INTIN, and can clear INTOUT */
			else
			{
				newreg = (oldreg & 0xfff8) | (data & 0x0007);
				newreg &= data | ~0x0080;
				newreg |= data & 0x0008;
			}
			IOREG(offset) = newreg;

      log_cb(RETRO_LOG_DEBUG, LOGPRE "oldreg=%04X newreg=%04X\n", oldreg, newreg);

			/* the TMS34010 can set output interrupt? */
			if (!(oldreg & 0x0080) && (newreg & 0x0080))
			{
				if (state.config->output_int)
					(*state.config->output_int)(1);
			}
			else if ((oldreg & 0x0080) && !(newreg & 0x0080))
			{
				if (state.config->output_int)
					(*state.config->output_int)(0);
			}

			/* input interrupt? (should really be state-based, but the functions don't exist!) */
			if (!(oldreg & 0x0008) && (newreg & 0x0008))
				timer_set(TIME_NOW, cpunum | (TMS34010_HI << 8), internal_interrupt_callback);
			else if ((oldreg & 0x0008) && !(newreg & 0x0008))
				IOREG(REG_INTPEND) &= ~TMS34010_HI;
			break;

		case REG_CONVSP:
			state.convsp = 1 << (~data & 0x1f);
			break;

		case REG_CONVDP:
			state.convdp = 1 << (~data & 0x1f);
			break;

		case REG_INTENB:
			if (IOREG(REG_INTENB) & IOREG(REG_INTPEND))
				check_interrupt();
			break;

		case REG_INTPEND:
			/* X1P, X2P and HIP are read-only */
			/* WVP and DIP can only have 0's written to them */
			IOREG(REG_INTPEND) = oldreg;
			if (!(data & TMS34010_WV))
				IOREG(REG_INTPEND) &= ~TMS34010_WV;
			if (!(data & TMS34010_DI))
				IOREG(REG_INTPEND) &= ~TMS34010_DI;
			check_interrupt();
			break;
	}

  /*log_cb(RETRO_LOG_DEBUG, LOGPRE "CPU#%d@%08X: %s = %04X (%d)\n", cpunum, activecpu_get_pc(), ioreg_name[offset], IOREG(offset), cpu_getscanline());*/

}

#if 0
static const char *ioreg020_name[] =
{
	"VESYNC", "HESYNC", "VEBLNK", "HEBLNK",
	"VSBLNK", "HSBLNK", "VTOTAL", "HTOTAL",
	"DPYCTL", "DPYSTRT", "DPYINT", "CONTROL",
	"HSTDATA", "HSTADRL", "HSTADRH", "HSTCTLL",

	"HSTCTLH", "INTENB", "INTPEND", "CONVSP",
	"CONVDP", "PSIZE", "PMASKL", "PMASKH",
	"CONVMP", "CONTROL2", "CONFIG", "DPYTAP",
	"VCOUNT", "HCOUNT", "DPYADR", "REFADR",

	"DPYSTL", "DPYSTH", "DPYNXL", "DPYNXH",
	"DINCL", "DINCH", "RES0", "HESERR",
	"RES1", "RES2", "RES3", "RES4",
	"SCOUNT", "BSFLTST", "DPYMSK", "RES5",

	"SETVCNT", "SETHCNT", "BSFLTDL", "BSFLTDH",
	"RES6", "RES7", "RES8", "RES9",
	"IHOST1L", "IHOST1H", "IHOST2L", "IHOST2H",
	"IHOST3L", "IHOST3H", "IHOST4L", "IHOST4H"
};
#endif

WRITE16_HANDLER( tms34020_io_register_w )
{
	int cpunum = cpu_getactivecpu();
	int oldreg, newreg;

	/* Set register */
	oldreg = IOREG(offset);
	IOREG(offset) = data;

  /*log_cb(RETRO_LOG_DEBUG, LOGPRE "CPU#%d@%08X: %s = %04X (%d)\n", cpunum, activecpu_get_pc(), ioreg020_name[offset], IOREG(offset), cpu_getscanline());*/

	switch (offset)
	{
		case REG020_DPYINT:
			if (data != oldreg || !dpyint_timer[cpunum])
				update_timers();
			break;

		case REG020_VSBLNK:
			if (data != oldreg || !vsblnk_timer[cpunum])
				update_timers();
			break;

		case REG020_VEBLNK:
			if (data != oldreg)
				update_timers();
			break;

		case REG020_CONTROL:
		case REG020_CONTROL2:
			IOREG(REG020_CONTROL) = data;
			IOREG(REG020_CONTROL2) = data;
			state.transparency = data & 0x20;
			state.window_checking = (data >> 6) & 0x03;
			set_raster_op();
			set_pixel_function();
			break;

		case REG020_PSIZE:
			set_pixel_function();

			switch (data)
			{
				default:
				case 0x01: state.pixelshift = 0; break;
				case 0x02: state.pixelshift = 1; break;
				case 0x04: state.pixelshift = 2; break;
				case 0x08: state.pixelshift = 3; break;
				case 0x10: state.pixelshift = 4; break;
			}
			break;

		case REG020_PMASKL:
		case REG020_PMASKH:
			/*if (data) log_cb(RETRO_LOG_DEBUG, LOGPRE "Plane masking not supported. PC=%08X\n", activecpu_get_pc());*/
			break;

		case REG020_DPYCTL:
			set_pixel_function();
			if ((oldreg ^ data) & 0x03fc)
				update_display_address(scanline_to_vcount(cpu_getscanline()));
			break;

		case REG020_HSTCTLH:
			/* if the CPU is halting itself, stop execution right away */
			if ((data & 0x8000) && !external_host_access)
				tms34010_ICount = 0;
			cpu_set_halt_line(cpunum, (data & 0x8000) ? ASSERT_LINE : CLEAR_LINE);

			/* NMI issued? */
			if (data & 0x0100)
				timer_set(TIME_NOW, cpunum | (TMS34010_NMI << 8), internal_interrupt_callback);
			break;

		case REG020_HSTCTLL:
			/* the TMS34010 can change MSGOUT, can set INTOUT, and can clear INTIN */
			if (!external_host_access)
			{
				newreg = (oldreg & 0xff8f) | (data & 0x0070);
				newreg |= data & 0x0080;
				newreg &= data | ~0x0008;
			}

			/* the host can change MSGIN, can set INTIN, and can clear INTOUT */
			else
			{
				newreg = (oldreg & 0xfff8) | (data & 0x0007);
				newreg &= data | ~0x0080;
				newreg |= data & 0x0008;
			}
			IOREG(offset) = newreg;

			/* the TMS34010 can set output interrupt? */
			if (!(oldreg & 0x0080) && (newreg & 0x0080))
			{
				if (state.config->output_int)
					(*state.config->output_int)(1);
			}
			else if ((oldreg & 0x0080) && !(newreg & 0x0080))
			{
				if (state.config->output_int)
					(*state.config->output_int)(0);
			}

			/* input interrupt? (should really be state-based, but the functions don't exist!) */
			if (!(oldreg & 0x0008) && (newreg & 0x0008))
				timer_set(TIME_NOW, cpunum | (TMS34010_HI << 8), internal_interrupt_callback);
			else if ((oldreg & 0x0008) && !(newreg & 0x0008))
				IOREG(REG020_INTPEND) &= ~TMS34010_HI;
			break;

		case REG020_INTENB:
			if (IOREG(REG020_INTENB) & IOREG(REG020_INTPEND))
				check_interrupt();
			break;

		case REG020_INTPEND:
			/* X1P, X2P and HIP are read-only */
			/* WVP and DIP can only have 0's written to them */
			IOREG(REG020_INTPEND) = oldreg;
			if (!(data & TMS34010_WV))
				IOREG(REG020_INTPEND) &= ~TMS34010_WV;
			if (!(data & TMS34010_DI))
				IOREG(REG020_INTPEND) &= ~TMS34010_DI;
			check_interrupt();
			break;

		case REG020_CONVSP:
			if (data & 0x001f)
			{
				if (data & 0x1f00)
					state.convsp = (1 << (~data & 0x1f)) + (1 << (~(data >> 8) & 0x1f));
				else
					state.convsp = 1 << (~data & 0x1f);
			}
			else
				state.convsp = data;
			break;

		case REG020_CONVDP:
			if (data & 0x001f)
			{
				if (data & 0x1f00)
					state.convdp = (1 << (~data & 0x1f)) + (1 << (~(data >> 8) & 0x1f));
				else
					state.convdp = 1 << (~data & 0x1f);
			}
			else
				state.convdp = data;
			break;

		case REG020_CONVMP:
			if (data & 0x001f)
			{
				if (data & 0x1f00)
					state.convmp = (1 << (~data & 0x1f)) + (1 << (~(data >> 8) & 0x1f));
				else
					state.convmp = 1 << (~data & 0x1f);
			}
			else
				state.convmp = data;
			break;

		case REG020_DPYSTRT:
		case REG020_DPYADR:
		case REG020_DPYTAP:
			break;

		case REG020_DPYSTL:
		case REG020_DPYSTH:
			if (data != oldreg)
			{
				state.last_update_vcount = scanline_to_vcount(cpu_getscanline());
				update_display_address(state.last_update_vcount);
			}
			break;
	}
}



/*###################################################################################################
**	I/O REGISTER READS
**#################################################################################################*/

READ16_HANDLER( tms34010_io_register_r )
{
	int cpunum = cpu_getactivecpu();
	int result, total;

  /*log_cb(RETRO_LOG_DEBUG, LOGPRE "CPU#%d@%08X: read %s\n", cpunum, activecpu_get_pc(), ioreg_name[offset]);*/

	switch (offset)
	{
		case REG_VCOUNT:
			return scanline_to_vcount(cpu_getscanline());

		case REG_HCOUNT:
			/* scale the horizontal position from screen width to HTOTAL */
			result = cpu_gethorzbeampos();
			total = IOREG(REG_HTOTAL);
			result = result * total / Machine->drv->screen_width;

			/* offset by the HBLANK end */
			result += IOREG(REG_HEBLNK);

			/* wrap around */
			if (result > total)
				result -= total;
			return result;

		case REG_DPYADR:
			update_display_address(scanline_to_vcount(cpu_getscanline()));
			break;

		case REG_REFCNT:
			return (activecpu_gettotalcycles() / 16) & 0xfffc;
			
		case REG_INTPEND:
			result = IOREG(offset);

			/* Cool Pool loops in mainline code on the appearance of the DI, even though they */
			/* have an IRQ handler. For this reason, we return it signalled a bit early in order */
			/* to make it past these loops. */
			if (dpyint_timer[cpunum] && timer_timeleft(dpyint_timer[cpunum]) < 3 * TIME_IN_HZ(40000000/TMS34010_CLOCK_DIVIDER))
				result |= TMS34010_DI;
			return result;
	}

	return IOREG(offset);
}


READ16_HANDLER( tms34020_io_register_r )
{
	/*int cpunum = cpu_getactivecpu();*/
	int result, total;

  /*log_cb(RETRO_LOG_DEBUG, LOGPRE "CPU#%d@%08X: read %s\n", cpunum, activecpu_get_pc(), ioreg_name[offset]);*/

	switch (offset)
	{
		case REG020_VCOUNT:
			return scanline_to_vcount(cpu_getscanline());

		case REG020_HCOUNT:
			/* scale the horizontal position from screen width to HTOTAL */
			result = cpu_gethorzbeampos();
			total = IOREG(REG020_HTOTAL);
			result = result * total / Machine->drv->screen_width;

			/* offset by the HBLANK end */
			result += IOREG(REG020_HEBLNK);

			/* wrap around */
			if (result > total)
				result -= total;
			return result;

		case REG020_DPYADR:
			update_display_address(scanline_to_vcount(cpu_getscanline()));
			break;

		case REG020_REFADR:
		{
			int refreshrate = (IOREG(REG020_CONFIG) >> 8) & 7;
			if (refreshrate < 6)
				return (activecpu_gettotalcycles() / refreshrate) & 0xffff;
			break;
		}
	}

	return IOREG(offset);
}



/*###################################################################################################
**	UTILITY FUNCTIONS
**#################################################################################################*/

int tms34010_io_display_blanked(int cpu)
{
	int result;
	cpuintrf_push_context(cpu);
	result = !(IOREG(REG_DPYCTL) & 0x8000);
	cpuintrf_pop_context();
	return result;
}


int tms34020_io_display_blanked(int cpu)
{
	int result;
	cpuintrf_push_context(cpu);
	result = !(IOREG(REG020_DPYCTL) & 0x8000);
	cpuintrf_pop_context();
	return result;
}


int tms34010_get_DPYSTRT(int cpu)
{
	int result;
	cpuintrf_push_context(cpu);
	result = IOREG(REG_DPYSTRT);
	cpuintrf_pop_context();
	return result;
}


int tms34020_get_DPYSTRT(int cpu)
{
	int result;
	cpuintrf_push_context(cpu);
	result = (IOREG(REG020_DPYSTH) << 16) | (IOREG(REG020_DPYSTL) & ~0x1f);
	cpuintrf_pop_context();
	return result;
}



/*###################################################################################################
**	SAVE STATE
**#################################################################################################*/

void tms34010_state_save(int cpunum, mame_file *f)
{
	tms34010_regs *context = FINDCONTEXT(cpunum);
	mame_fwrite(f,context,sizeof(state));
	mame_fwrite(f,&tms34010_ICount,sizeof(tms34010_ICount));
	mame_fwrite(f,state.shiftreg,sizeof(SHIFTREG_SIZE));
}


void tms34010_state_load(int cpunum, mame_file *f)
{
	/* Don't reload the following */
	UINT16 *shiftreg_save = state.shiftreg;
	tms34010_regs *context = FINDCONTEXT(cpunum);

	mame_fread(f,context,sizeof(state));
	mame_fread(f,&tms34010_ICount,sizeof(tms34010_ICount));
	change_pc29lew(TOBYTE(PC));
	SET_FW();
	tms34010_io_register_w(REG_DPYINT,IOREG(REG_DPYINT),0);
	set_raster_op();
	set_pixel_function();

	state.shiftreg      = shiftreg_save;

	mame_fread(f,state.shiftreg,sizeof(SHIFTREG_SIZE));
}

static void tms34010_state_presave(void)
{
	int i;
	for (i = 0; i < 16; i++)
		state.flat_aregs[i] = AREG(i);
	for (i = 0; i < 15; i++)
		state.flat_bregs[i] = BREG(BINDEX(i));
}

static void tms34010_state_postload(void)
{
	int i;
	for (i = 0; i < 16; i++)
		AREG(i) = state.flat_aregs[i];
	for (i = 0; i < 15; i++)
		BREG(BINDEX(i)) = state.flat_bregs[i];

	change_pc29lew(TOBYTE(PC));
	SET_FW();
	tms34010_io_register_w(REG_DPYINT,IOREG(REG_DPYINT),0);
	set_raster_op();
	set_pixel_function();
}


/*###################################################################################################
**	HOST INTERFACE WRITES
**#################################################################################################*/

void tms34010_host_w(int cpunum, int reg, int data)
{
	unsigned int addr;

	/* swap to the target cpu */
	cpuintrf_push_context(cpunum);

	switch (reg)
	{
		/* upper 16 bits of the address */
		case TMS34010_HOST_ADDRESS_H:
			IOREG(REG_HSTADRH) = data;
			break;

		/* lower 16 bits of the address */
		case TMS34010_HOST_ADDRESS_L:
			IOREG(REG_HSTADRL) = data;
			break;

		/* actual data */
		case TMS34010_HOST_DATA:

			/* write to the address */
			addr = (IOREG(REG_HSTADRH) << 16) | IOREG(REG_HSTADRL);
			TMS34010_WRMEM_WORD(TOBYTE(addr & 0xfffffff0), data);

			/* optional postincrement */
			if (IOREG(REG_HSTCTLH) & 0x0800)
			{
				addr += 0x10;
				IOREG(REG_HSTADRH) = addr >> 16;
				IOREG(REG_HSTADRL) = (UINT16)addr;
			}
			break;

		/* control register */
		case TMS34010_HOST_CONTROL:
			external_host_access = 1;
			tms34010_io_register_w(REG_HSTCTLH, data & 0xff00, 0);
			tms34010_io_register_w(REG_HSTCTLL, data & 0x00ff, 0);
			external_host_access = 0;
			break;

		/* error case */
		default:
			/*logerror("tms34010_host_control_w called on invalid register %d\n", reg);*/
			break;
	}

	/* swap back */
	cpuintrf_pop_context();
	activecpu_reset_banking();
}



/*###################################################################################################
**	HOST INTERFACE READS
**#################################################################################################*/

int tms34010_host_r(int cpunum, int reg)
{
	unsigned int addr;
	int result = 0;

	/* swap to the target cpu */
	cpuintrf_push_context(cpunum);

	switch (reg)
	{
		/* upper 16 bits of the address */
		case TMS34010_HOST_ADDRESS_H:
			result = IOREG(REG_HSTADRH);
			break;

		/* lower 16 bits of the address */
		case TMS34010_HOST_ADDRESS_L:
			result = IOREG(REG_HSTADRL);
			break;

		/* actual data */
		case TMS34010_HOST_DATA:

			/* read from the address */
			addr = (IOREG(REG_HSTADRH) << 16) | IOREG(REG_HSTADRL);
			result = TMS34010_RDMEM_WORD(TOBYTE(addr & 0xfffffff0));

			/* optional postincrement (it says preincrement, but data is preloaded, so it
			   is effectively a postincrement */
			if (IOREG(REG_HSTCTLH) & 0x1000)
			{
				addr += 0x10;
				IOREG(REG_HSTADRH) = addr >> 16;
				IOREG(REG_HSTADRL) = (UINT16)addr;
			}
			break;

		/* control register */
		case TMS34010_HOST_CONTROL:
			result = (IOREG(REG_HSTCTLH) & 0xff00) | (IOREG(REG_HSTCTLL) & 0x00ff);
			break;

		/* error case */
		default:
			/*logerror("tms34010_host_control_r called on invalid register %d\n", reg);*/
			break;
	}

	/* swap back */
	cpuintrf_pop_context();
	activecpu_reset_banking();

	return result;
}
