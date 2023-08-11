/***************************************************************************

    TMS34010: Portable Texas Instruments TMS34010 emulator

    Copyright Alex Pasadyn/Zsolt Vasvari
    Parts based on code by Aaron Giles

***************************************************************************/

#include "burnint.h"
#include "tiles_generic.h"
#include "tms34010.h"
#include "34010ops.h"
#include "stddef.h"


/***************************************************************************
    DEBUG STATE & STRUCTURES
***************************************************************************/

#define VERBOSE 			0
#define LOG_CONTROL_REGS	0
#define LOG_GRAPHICS_OPS	0

#define logerror
#define fatalerror

#define LOG(x)	do { if (VERBOSE) logerror x; } while (0)

enum {
	CLEAR_LINE = 0,
	ASSERT_LINE,
	HOLD_LINE,
	PULSE_LINE
};


/***************************************************************************
    CORE STATE
***************************************************************************/

/* TMS34010 State */
typedef struct
{
#ifdef LSB_FIRST
	INT16 x;
	INT16 y;
#else
	INT16 y;
	INT16 x;
#endif
} XY;

typedef struct tms34010_regs
{
	UINT16 op;
	UINT32 pc;
	UINT32 st;
	UINT32 convsp;
	UINT32 convdp;
	UINT32 convmp;
	INT32 gfxcycles;
	UINT8 pixelshift;
	UINT8 is_34020;
	UINT8 reset_deferred;
	UINT8 hblank_stable;

	INT32 irqhold[2];

	INT64 total_cycles;
	INT32 timer_cyc;
	UINT32 timer_active;
	INT32 cycles_start;
	INT32 icounter;
	INT32 stop;

	/* A registers 0-15 map to regs[0]-regs[15] */
	/* B registers 0-15 map to regs[30]-regs[15] */
	union
	{
		INT32 reg;
		XY xy;
	} regs[31];

	/* for the 34010, we only copy 32 of these into the new state */
	UINT16 IOregs[64];

	UINT8 external_host_access;

	UINT32 pointer_separator;

	tms34010_config config;
	UINT16 *shiftreg;
	void (*timer_cb)();

	void (*pixel_write)(offs_t offset, UINT32 data);
	UINT32 (*pixel_read)(offs_t offset);
	UINT32 (*raster_op)(UINT32 newpix, UINT32 oldpix);
} tms34010_regs;

/***************************************************************************
    GLOBAL VARIABLES
***************************************************************************/

/* public globals */
#define tms34010_ICount state.icounter

/* internal state */
static tms34010_regs 		state;

static void check_interrupt(void);
static void check_timer(int cyc);
static void set_raster_op(void);
static void set_pixel_function(void);

int tms34010_context_size()
{
	return sizeof(tms34010_regs);
}

void tms34010_get_context(void *get)
{
	memcpy(get, &state, sizeof(tms34010_regs));
}

void tms34010_set_context(void *set)
{
	memcpy(&state, set, sizeof(tms34010_regs));
}

/***************************************************************************
    MACROS
***************************************************************************/

/* status register definitions */
#define STBIT_N			(1 << 31)
#define STBIT_C			(1 << 30)
#define STBIT_Z			(1 << 29)
#define STBIT_V			(1 << 28)
#define STBIT_P			(1 << 25)
#define STBIT_IE		(1 << 21)
#define STBIT_FE1		(1 << 11)
#define STBITS_F1		(0x1f << 6)
#define STBIT_FE0		(1 << 5)
#define STBITS_F0		(0x1f << 0)

/* register definitions and shortcuts */
#define PC				(state.pc)
#define ST				(state.st)
#define N_FLAG			(state.st & STBIT_N)
#define Z_FLAG			(state.st & STBIT_Z)
#define C_FLAG			(state.st & STBIT_C)
#define V_FLAG			(state.st & STBIT_V)
#define P_FLAG			(state.st & STBIT_P)
#define IE_FLAG			(state.st & STBIT_IE)
#define FE0_FLAG		(state.st & STBIT_FE0)
#define FE1_FLAG		(state.st & STBIT_FE1)

/* register file access */
#define AREG(i)			(state.regs[i].reg)
#define AREG_XY(i)		(state.regs[i].xy)
#define AREG_X(i)		(state.regs[i].xy.x)
#define AREG_Y(i)		(state.regs[i].xy.y)
#define BREG(i)			(state.regs[30 - (i)].reg)
#define BREG_XY(i)		(state.regs[30 - (i)].xy)
#define BREG_X(i)		(state.regs[30 - (i)].xy.x)
#define BREG_Y(i)		(state.regs[30 - (i)].xy.y)
#define SP				AREG(15)
#define FW(i)			((state.st >> (i ? 6 : 0)) & 0x1f)
#define FWEX(i)			((state.st >> (i ? 6 : 0)) & 0x3f)

/* opcode decode helpers */
#define SRCREG			((state.op >> 5) & 0x0f)
#define DSTREG			(state.op & 0x0f)
#define SKIP_WORD		(PC += (2 << 3))
#define SKIP_LONG		(PC += (4 << 3))
#define PARAM_K			((state.op >> 5) & 0x1f)
#define PARAM_N			(state.op & 0x1f)
#define PARAM_REL8		((INT8)state.op)

/* memory I/O */
#define WFIELD0(a,b)	(*tms34010_wfield_functions[FW(0)])(a,b)
#define WFIELD1(a,b)	(*tms34010_wfield_functions[FW(1)])(a,b)
#define RFIELD0(a)		(*tms34010_rfield_functions[FWEX(0)])(a)
#define RFIELD1(a)		(*tms34010_rfield_functions[FWEX(1)])(a)
#define WPIXEL(a,b)		state.pixel_write(a,b)
#define RPIXEL(a)		state.pixel_read(a)

/* Implied Operands */
#define SADDR			BREG(0)
#define SADDR_X			BREG_X(0)
#define SADDR_Y			BREG_Y(0)
#define SADDR_XY		BREG_XY(0)
#define SPTCH			BREG(1)
#define DADDR			BREG(2)
#define DADDR_X			BREG_X(2)
#define DADDR_Y			BREG_Y(2)
#define DADDR_XY		BREG_XY(2)
#define DPTCH			BREG(3)
#define OFFSET			BREG(4)
#define WSTART_X		BREG_X(5)
#define WSTART_Y		BREG_Y(5)
#define WEND_X			BREG_X(6)
#define WEND_Y			BREG_Y(6)
#define DYDX_X			BREG_X(7)
#define DYDX_Y			BREG_Y(7)
#define COLOR0			BREG(8)
#define COLOR1			BREG(9)
#define COUNT			BREG(10)
#define INC1_X			BREG_X(11)
#define INC1_Y			BREG_Y(11)
#define INC2_X			BREG_X(12)
#define INC2_Y			BREG_Y(12)
#define PATTRN			BREG(13)
#define TEMP			BREG(14)

/* I/O registers */
#define WINDOW_CHECKING	((IOREG(REG_CONTROL) >> 6) & 0x03)



/***************************************************************************
    INLINE SHORTCUTS
***************************************************************************/

/* Combine indiviual flags into the Status Register */
INLINE UINT32 GET_ST(void)
{
	return state.st;
}

/* Break up Status Register into indiviual flags */
INLINE void SET_ST(UINT32 st)
{
	state.st = st;
	/* interrupts might have been enabled, check it */
	check_interrupt();
}

/* Intialize Status to 0x0010 */
INLINE void RESET_ST(void)
{
	SET_ST(0x00000010);
}

/* shortcuts for reading opcodes */
INLINE UINT32 ROPCODE(void)
{
	UINT32 pc = TOBYTE(PC);
	PC += 2 << 3;
	return cpu_readop16(pc);
}

INLINE INT16 PARAM_WORD(void)
{
	UINT32 pc = TOBYTE(PC);
	PC += 2 << 3;
	return cpu_readop_arg16(pc);
}

INLINE INT32 PARAM_LONG(void)
{
	UINT32 pc = TOBYTE(PC);
	PC += 4 << 3;
	return (UINT16)cpu_readop_arg16(pc) | (cpu_readop_arg16(pc + 2) << 16);
}

INLINE INT16 PARAM_WORD_NO_INC(void)
{
	return cpu_readop_arg16(TOBYTE(PC));
}

INLINE INT32 PARAM_LONG_NO_INC(void)
{
	UINT32 pc = TOBYTE(PC);
	return (UINT16)cpu_readop_arg16(pc) | (cpu_readop_arg16(pc + 2) << 16);
}

/* read memory byte */
INLINE UINT32 RBYTE(offs_t offset)
{
	UINT32 ret;
	RFIELDMAC_8;
	return ret;
}

/* write memory byte */
INLINE void WBYTE(offs_t offset, UINT32 data)
{
	WFIELDMAC_8;
}

/* read memory long */
INLINE UINT32 RLONG(offs_t offset)
{
	RFIELDMAC_32;
}

/* write memory long */
INLINE void WLONG(offs_t offset, UINT32 data)
{
	WFIELDMAC_32;
}

/* pushes/pops a value from the stack */
INLINE void PUSH(UINT32 data)
{
	SP -= 0x20;
	WLONG(SP, data);
}

INLINE INT32 POP(void)
{
	INT32 ret = RLONG(SP);
	SP += 0x20;
	return ret;
}



/***************************************************************************
    PIXEL READS
***************************************************************************/

#define RP(m1,m2)  											\
	/* TODO: Plane masking */								\
	return (TMS34010_RDMEM_WORD(TOBYTE(offset & 0xfffffff0)) >> (offset & m1)) & m2;

static UINT32 read_pixel_1(offs_t offset) { RP(0x0f,0x01) }
static UINT32 read_pixel_2(offs_t offset) { RP(0x0e,0x03) }
static UINT32 read_pixel_4(offs_t offset) { RP(0x0c,0x0f) }
static UINT32 read_pixel_8(offs_t offset) { RP(0x08,0xff) }
static UINT32 read_pixel_16(offs_t offset)
{
	/* TODO: Plane masking */
	return TMS34010_RDMEM_WORD(TOBYTE(offset & 0xfffffff0));
}
static UINT32 read_pixel_32(offs_t offset)
{
	/* TODO: Plane masking */
	return TMS34010_RDMEM_DWORD(TOBYTE(offset & 0xffffffe0));
}

/* Shift register read */
static UINT32 read_pixel_shiftreg(offs_t offset)
{
	if (state.config.to_shiftreg)
		state.config.to_shiftreg(offset, &state.shiftreg[0]);
	else
		fatalerror("To ShiftReg function not set. PC = %08X\n", PC);
	return state.shiftreg[0];
}



/***************************************************************************
    PIXEL WRITES
***************************************************************************/

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
static void write_pixel_1(offs_t offset, UINT32 data) { WP(0x0f, 0x01); }
static void write_pixel_2(offs_t offset, UINT32 data) { WP(0x0e, 0x03); }
static void write_pixel_4(offs_t offset, UINT32 data) { WP(0x0c, 0x0f); }
static void write_pixel_8(offs_t offset, UINT32 data) { WP(0x08, 0xff); }
static void write_pixel_16(offs_t offset, UINT32 data)
{
	/* TODO: plane masking */
	TMS34010_WRMEM_WORD(TOBYTE(offset & 0xfffffff0), data);
}
static void write_pixel_32(offs_t offset, UINT32 data)
{
	/* TODO: plane masking */
	TMS34010_WRMEM_WORD(TOBYTE(offset & 0xffffffe0), data);
}

/* No Raster Op + Transparency */
static void write_pixel_t_1(offs_t offset, UINT32 data) { WP_T(0x0f, 0x01); }
static void write_pixel_t_2(offs_t offset, UINT32 data) { WP_T(0x0e, 0x03); }
static void write_pixel_t_4(offs_t offset, UINT32 data) { WP_T(0x0c, 0x0f); }
static void write_pixel_t_8(offs_t offset, UINT32 data) { WP_T(0x08, 0xff); }
static void write_pixel_t_16(offs_t offset, UINT32 data)
{
	/* TODO: plane masking */
	if (data)
		TMS34010_WRMEM_WORD(TOBYTE(offset & 0xfffffff0), data);
}
static void write_pixel_t_32(offs_t offset, UINT32 data)
{
	/* TODO: plane masking */
	if (data)
		TMS34010_WRMEM_DWORD(TOBYTE(offset & 0xffffffe0), data);
}

/* Raster Op + No Transparency */
static void write_pixel_r_1(offs_t offset, UINT32 data) { WP_R(0x0f, 0x01); }
static void write_pixel_r_2(offs_t offset, UINT32 data) { WP_R(0x0e, 0x03); }
static void write_pixel_r_4(offs_t offset, UINT32 data) { WP_R(0x0c, 0x0f); }
static void write_pixel_r_8(offs_t offset, UINT32 data) { WP_R(0x08, 0xff); }
static void write_pixel_r_16(offs_t offset, UINT32 data)
{
	/* TODO: plane masking */
	UINT32 a = TOBYTE(offset & 0xfffffff0);
	TMS34010_WRMEM_WORD(a, state.raster_op(data, TMS34010_RDMEM_WORD(a)));
}
static void write_pixel_r_32(offs_t offset, UINT32 data)
{
	/* TODO: plane masking */
	UINT32 a = TOBYTE(offset & 0xffffffe0);
	TMS34010_WRMEM_DWORD(a, state.raster_op(data, TMS34010_RDMEM_DWORD(a)));
}

/* Raster Op + Transparency */
static void write_pixel_r_t_1(offs_t offset, UINT32 data) { WP_R_T(0x0f,0x01); }
static void write_pixel_r_t_2(offs_t offset, UINT32 data) { WP_R_T(0x0e,0x03); }
static void write_pixel_r_t_4(offs_t offset, UINT32 data) { WP_R_T(0x0c,0x0f); }
static void write_pixel_r_t_8(offs_t offset, UINT32 data) { WP_R_T(0x08,0xff); }
static void write_pixel_r_t_16(offs_t offset, UINT32 data)
{
	/* TODO: plane masking */
	UINT32 a = TOBYTE(offset & 0xfffffff0);
	data = state.raster_op(data, TMS34010_RDMEM_WORD(a));

	if (data)
		TMS34010_WRMEM_WORD(a, data);
}
static void write_pixel_r_t_32(offs_t offset, UINT32 data)
{
	/* TODO: plane masking */
	UINT32 a = TOBYTE(offset & 0xffffffe0);
	data = state.raster_op(data, TMS34010_RDMEM_DWORD(a));

	if (data)
		TMS34010_WRMEM_DWORD(a, data);
}

/* Shift register write */
static void write_pixel_shiftreg(offs_t offset, UINT32 data)
{
	if (state.config.from_shiftreg)
		state.config.from_shiftreg(offset, &state.shiftreg[0]);
	else
		fatalerror("From ShiftReg function not set. PC = %08X\n", PC);
}



/***************************************************************************
    RASTER OPS
***************************************************************************/

/* Raster operations */
static UINT32 raster_op_1(UINT32 newpix, UINT32 oldpix)  { return newpix & oldpix; }
static UINT32 raster_op_2(UINT32 newpix, UINT32 oldpix)  { return newpix & ~oldpix; }
static UINT32 raster_op_3(UINT32 newpix, UINT32 oldpix)  { return 0; }
static UINT32 raster_op_4(UINT32 newpix, UINT32 oldpix)  { return newpix | ~oldpix; }
static UINT32 raster_op_5(UINT32 newpix, UINT32 oldpix)  { return ~(newpix ^ oldpix); }
static UINT32 raster_op_6(UINT32 newpix, UINT32 oldpix)  { return ~oldpix; }
static UINT32 raster_op_7(UINT32 newpix, UINT32 oldpix)  { return ~(newpix | oldpix); }
static UINT32 raster_op_8(UINT32 newpix, UINT32 oldpix)  { return newpix | oldpix; }
static UINT32 raster_op_9(UINT32 newpix, UINT32 oldpix)  { return oldpix; }
static UINT32 raster_op_10(UINT32 newpix, UINT32 oldpix) { return newpix ^ oldpix; }
static UINT32 raster_op_11(UINT32 newpix, UINT32 oldpix) { return ~newpix & oldpix; }
static UINT32 raster_op_12(UINT32 newpix, UINT32 oldpix) { return 0xffff; }
static UINT32 raster_op_13(UINT32 newpix, UINT32 oldpix) { return ~newpix | oldpix; }
static UINT32 raster_op_14(UINT32 newpix, UINT32 oldpix) { return ~(newpix & oldpix); }
static UINT32 raster_op_15(UINT32 newpix, UINT32 oldpix) { return ~newpix; }
static UINT32 raster_op_16(UINT32 newpix, UINT32 oldpix) { return newpix + oldpix; }
static UINT32 raster_op_17(UINT32 newpix, UINT32 oldpix)
{
	UINT32 max = (UINT32)0xffffffff >> (32 - IOREG(REG_PSIZE));
	UINT32 res = newpix + oldpix;
	return (res > max) ? max : res;
}
static UINT32 raster_op_18(UINT32 newpix, UINT32 oldpix) { return oldpix - newpix; }
static UINT32 raster_op_19(UINT32 newpix, UINT32 oldpix) { return (oldpix > newpix) ? oldpix - newpix : 0; }
static UINT32 raster_op_20(UINT32 newpix, UINT32 oldpix) { return (oldpix > newpix) ? oldpix : newpix; }
static UINT32 raster_op_21(UINT32 newpix, UINT32 oldpix) { return (oldpix > newpix) ? newpix : oldpix; }



/***************************************************************************
    OPCODE TABLE & IMPLEMENTATIONS
***************************************************************************/

/* includes the static function prototypes and the master opcode table */
#include "34010tbl.c"
#include "34010fld.c"

/* includes the actual opcode implementations */
#include "34010ops.c"
#include "34010gfx.c"

static int cpu_getactivecpu()
{
	return 0;
}

/***************************************************************************
    Internal interrupt check
****************************************************************************/

/* Generate pending interrupts. */
static void check_interrupt(void)
{
	int vector = 0;
	int irqline = -1;
	int irq;

	/* check for NMI first */
	if (IOREG(REG_HSTCTLH) & 0x0100)
	{
		LOG(("TMS34010#%d takes NMI\n", cpu_getactivecpu()));

		/* ack the NMI */
		IOREG(REG_HSTCTLH) &= ~0x0100;

		/* handle NMI mode bit */
		if (!(IOREG(REG_HSTCTLH) & 0x0200))
		{
			PUSH(PC);
			PUSH(GET_ST());
		}

		/* leap to the vector */
		RESET_ST();
		PC = RLONG(0xfffffee0);
		change_pc(TOBYTE(PC));
		COUNT_CYCLES(16);
		return;
	}

	/* early out if everything else is disabled */
	irq = IOREG(REG_INTPEND) & IOREG(REG_INTENB);

	//if (irq)
	//	bprintf(0, _T("check_interrupt(): %x  IE_FLAG %x\n"), irq, IE_FLAG);

	if (!IE_FLAG || !irq)
		return;

	/* host interrupt */
	if (irq & TMS34010_HI)
	{
		LOG(("TMS34010#%d takes HI\n", cpu_getactivecpu()));
		vector = 0xfffffec0;
	}

	/* display interrupt */
	else if (irq & TMS34010_DI)
	{
		//bprintf(0, _T("TMS34010#%d takes DI\n"), cpu_getactivecpu());
		vector = 0xfffffea0;
	}

	/* window violation interrupt */
	else if (irq & TMS34010_WV)
	{
		LOG(("TMS34010#%d takes WV\n", cpu_getactivecpu()));
		vector = 0xfffffe80;
	}

	/* external 1 interrupt */
	else if (irq & TMS34010_INT1)
	{
		//bprintf(0, _T("TMS34 takes INT1 (irq0)\n"));
		LOG(("TMS34010#%d takes INT1\n", cpu_getactivecpu()));
		vector = 0xffffffc0;
		irqline = 0;
	}

	/* external 2 interrupt */
	else if (irq & TMS34010_INT2)
	{
		LOG(("TMS34010#%d takes INT2\n", cpu_getactivecpu()));
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
		change_pc(TOBYTE(PC));
		COUNT_CYCLES(16);

		// irq hold function
		if (irqline >= 0 && state.irqhold[irqline]) {
			tms34010_set_irq_line(irqline, 0);
			state.irqhold[irqline] = 0;
		}
	}
}



/***************************************************************************
    Reset the CPU emulation
***************************************************************************/
void tms34010_init()
{
	memset(&state, 0, sizeof(state));

	state.shiftreg = (UINT16*)BurnMalloc(SHIFTREG_SIZE);
}

void tms34010_set_cycperframe(INT32 cpf)
{
	state.config.cpu_cyc_per_frame = cpf;
}

void tms34010_set_pixclock(INT32 pxlclock, INT32 pxl_per_clock)
{
	state.config.pixclock = pxlclock;
	state.config.pixperclock = pxl_per_clock;
}

void tms34010_set_output_int(void (*oi_func)(INT32))
{
	state.config.output_int = oi_func;
}

void tms34010_set_halt_on_reset(INT32 onoff)
{
	state.config.halt_on_reset = onoff;
}

void tms34010_set_toshift(void (*to_shiftreg)(UINT32, UINT16 *))
{
	state.config.to_shiftreg = to_shiftreg;
}

void tms34010_set_fromshift(void (*from_shiftreg)(UINT32, UINT16 *))
{
	state.config.from_shiftreg = from_shiftreg;
}

void tms34010_exit()
{
	if (state.shiftreg) {
		BurnFree(state.shiftreg);
	}
}

void tms34010_reset()
{
	/* clear the state & leave the config section alone */
	memset(&state, 0, STRUCT_SIZE_HELPER(struct tms34010_regs, pointer_separator));

	/* fetch the initial PC and reset the state */
	PC = RLONG(0xffffffe0) & 0xfffffff0;
	change_pc(TOBYTE(PC));
	RESET_ST();

	/* HALT the CPU if requested, and remember to re-read the starting PC */
	/* the first time we are run */
	state.reset_deferred = state.config.halt_on_reset;
	if (state.config.halt_on_reset)
		tms34010_io_register_w(REG_HSTCTLH << 4, 0x8000);

	state.timer_active = 0;
	state.timer_cyc = 0;
}

#if (HAS_TMS34020)
void tms34020_reset()
{
	tms34010_reset();
	state.is_34020 = 1;
}
#endif

void tms34010_scan(INT32 nAction)
{
	if (nAction & ACB_DRIVER_DATA) {
		ScanVar(&state, STRUCT_SIZE_HELPER(struct tms34010_regs, pointer_separator), "TMS340x0 Struct");
		ScanVar(state.shiftreg, SHIFTREG_SIZE, "TMS340x0 Shiftreg");
	}

	if (nAction & ACB_WRITE) { // load state
		change_pc(TOBYTE(PC));
		set_raster_op();
		set_pixel_function();
	}
}

static void check_timer(int cyc)
{
	if (state.timer_active) {
		state.timer_cyc -= cyc;
		if (state.timer_cyc <= 0) {
			//bprintf(0, _T("timer hit @ %I64d\n"), TMS34010TotalCycles());
			state.timer_active = 0;
			state.timer_cyc = 0;

			if (state.timer_cb) {
				state.timer_cb();
			} else {
				bprintf(0, _T("no timer cb!\n"));
			}
		}
	}
}

void tms34010_timer_set_cb(void (*t_cb)())
{
	state.timer_cb = t_cb;
}

void tms34010_timer_arm(int cycle)
{
	//bprintf(0, _T("timer arm @ %d   time now %I64d\n"), cycle, TMS34010TotalCycles());
	if (state.timer_active) {
		bprintf(0, _T("TMS34010: timer_arm() arm timer when timer pending!\n"));
	}
	state.timer_active = 1;
	state.timer_cyc = cycle;
}


/***************************************************************************
    Set IRQ line state
***************************************************************************/

void tms34010_set_irq_line(int irqline, int linestate)
{
	//bprintf(0, _T("TMS34010 set irq %x  state %x\n"), irqline, linestate);
	/* set the pending interrupt */

	switch (irqline)
	{
		case 0:
			if (linestate != CLEAR_LINE) {
				IOREG(REG_INTPEND) |= TMS34010_INT1;

				if (linestate == CPU_IRQSTATUS_HOLD) {
					state.irqhold[0] = 1;
				}
			} else {
				IOREG(REG_INTPEND) &= ~TMS34010_INT1;
			}
			break;

		case 1:
			if (linestate != CLEAR_LINE) {
				IOREG(REG_INTPEND) |= TMS34010_INT2;

				if (linestate == CPU_IRQSTATUS_HOLD) {
					state.irqhold[1] = 1;
				}
			} else {
				IOREG(REG_INTPEND) &= ~TMS34010_INT2;
			}
			break;
	}
}



/***************************************************************************
    Generate internal interrupt
***************************************************************************/

static void internal_interrupt_callback(INT32 type)
{
	//bprintf(0, _T("TMS34010#%d set internal interrupt $%04x\n"), cpu_getactivecpu(), type);
	IOREG(REG_INTPEND) |= type;
}



/***************************************************************************
    Execute
***************************************************************************/

int tms34010_run(int cycles)
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
	state.cycles_start = cycles;
	tms34010_ICount = cycles;
	state.stop = 0;

	change_pc(TOBYTE(PC));

	/* check interrupts first */
	check_timer(0);
	do
	{
		check_interrupt();
		state.op = ROPCODE();
		(*opcode_table[state.op >> 4])();

	} while (tms34010_ICount > 0 && !state.stop);

	cycles = cycles - tms34010_ICount;
	state.total_cycles += cycles;

	state.cycles_start = 0;
	tms34010_ICount = 0;

	return cycles;
}

void tms34010_stop()
{
	state.stop = 1;
}

void tms34010_modify_timeslice(int cycles)
{ // only call this from a read/write handler while cpu is running!
	tms34010_ICount += cycles;
}

int tms34010_idle(int cycles)
{
	state.total_cycles += cycles;

	return cycles;
}

INT64 tms34010_total_cycles()
{
	return state.total_cycles + (state.cycles_start - tms34010_ICount);
}

void tms34010_new_frame()
{
	state.total_cycles = 0;
}

UINT32 tms34010_get_pc()
{
	return state.pc;
}



/***************************************************************************
    PIXEL OPS
***************************************************************************/

static void (*const pixel_write_ops[4][6])(offs_t offset, UINT32 data) =
{
	{ write_pixel_1,     write_pixel_2,     write_pixel_4,     write_pixel_8,     write_pixel_16,     write_pixel_32     },
	{ write_pixel_r_1,   write_pixel_r_2,   write_pixel_r_4,   write_pixel_r_8,   write_pixel_r_16,   write_pixel_r_32   },
	{ write_pixel_t_1,   write_pixel_t_2,   write_pixel_t_4,   write_pixel_t_8,   write_pixel_t_16,	  write_pixel_t_32   },
	{ write_pixel_r_t_1, write_pixel_r_t_2, write_pixel_r_t_4, write_pixel_r_t_8, write_pixel_r_t_16, write_pixel_r_t_32 }
};

static UINT32 (*const pixel_read_ops[6])(offs_t offset) =
{
	read_pixel_1,        read_pixel_2,      read_pixel_4,      read_pixel_8,      read_pixel_16,      read_pixel_32
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
		case 0x20: i2 = 5; break;
	}

	if (IOREG(REG_CONTROL) & 0x20)
		i1 = state.raster_op ? 3 : 2;
	else
		i1 = state.raster_op ? 1 : 0;

	state.pixel_write = pixel_write_ops[i1][i2];
	state.pixel_read  = pixel_read_ops [i2];
}



/***************************************************************************
    RASTER OPS
***************************************************************************/

static UINT32 (*const raster_ops[32]) (UINT32 newpix, UINT32 oldpix) =
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



/***************************************************************************
    VIDEO TIMING HELPERS
***************************************************************************/

void tms34010_generate_scanline(INT32 line, scanline_render_t render)
{
	int vsblnk, veblnk, vtotal;
	int vcount = line;
	int enabled;
	int master;

	/* fetch the core timing parameters */
	enabled = SMART_IOREG(DPYCTL) & 0x8000;
	master = (state.is_34020 || (SMART_IOREG(DPYCTL) & 0x2000));
	vsblnk = SMART_IOREG(VSBLNK);
	veblnk = SMART_IOREG(VEBLNK);
	vtotal = SMART_IOREG(VTOTAL);

#if 0  // for driver debug -dink
	if (line == 1) {
		bprintf(0, _T("vsblnk %x\n"), vsblnk);
		bprintf(0, _T("veblnk %x\n"), veblnk);
		bprintf(0, _T("vtotal %x\n"), vtotal);
		bprintf(0, _T("enabled: %x  master %x\n"), enabled, master);
	}
#endif

	if (!master)
	{
		vtotal = MIN(nScreenHeight - 1, vtotal);
		vcount = line;
	}

	/* update the VCOUNT */
	SMART_IOREG(VCOUNT) = vcount;

	/* if we match the display interrupt scanline, signal an interrupt */
	if (enabled && vcount == SMART_IOREG(DPYINT))
	{
		/* generate the display interrupt signal */
		internal_interrupt_callback(TMS34010_DI);
	}

	/* at the start of VBLANK, load the starting display address */
	if (vcount == vsblnk)
	{
		/* 34010 loads DPYADR with DPYSTRT, and inverts if the origin is 0 */
		if (!state.is_34020)
		{
			IOREG(REG_DPYADR) = IOREG(REG_DPYSTRT);
			LOG(("Start of VBLANK, DPYADR = %04X\n", IOREG(REG_DPYADR)));
		}

		/* 34020 loads DPYNXx with DPYSTx */
		else
		{
			IOREG(REG020_DPYNXL) = IOREG(REG020_DPYSTL) & 0xffe0;
			IOREG(REG020_DPYNXH) = IOREG(REG020_DPYSTH);
		}
	}

	/* at the end of the screen, update the display parameters */
#if 0
	if (vcount == vtotal)
	{
		/* only do this if we have an incoming pixel clock */
		/* also, only do it if the HEBLNK/HSBLNK values are stable */
		if (master && state.config.scanline_callback != NULL)
		{
			int htotal = SMART_IOREG(HTOTAL);
			if (htotal > 0 && vtotal > 0)
			{
				attoseconds_t refresh = HZ_TO_ATTOSECONDS(state.config.pixclock) * (htotal + 1) * (vtotal + 1);
				int width = (htotal + 1) * state.config.pixperclock;
				int height = vtotal + 1;
				rectangle visarea;

				/* extract the visible area */
				visarea.min_x = SMART_IOREG(HEBLNK) * state.config.pixperclock;
				visarea.max_x = SMART_IOREG(HSBLNK) * state.config.pixperclock - 1;
				visarea.min_y = veblnk;
				visarea.max_y = vsblnk - 1;

				/* if everything looks good, set the info */
				if (visarea.min_x < visarea.max_x && visarea.max_x <= width && visarea.min_y < visarea.max_y && visarea.max_y <= height)
				{
					/* because many games play with the HEBLNK/HSBLNK for effects, we don't change
                       if they are the only thing that has changed, unless they are stable for a couple
                       of frames */
					int current_width  = video_screen_get_width(state.screen);
					int current_height = video_screen_get_height(state.screen);

					if (width != current_width || height != current_height || visarea.min_y != current_visarea->min_y || visarea.max_y != current_visarea->max_y ||
						(state.hblank_stable > 2 && (visarea.min_x != current_visarea->min_x || visarea.max_x != current_visarea->max_x)))
					{
						video_screen_configure(state.screen, width, height, &visarea, refresh);
					}
					state.hblank_stable++;
				}

				LOG(("Configuring screen: HTOTAL=%3d BLANK=%3d-%3d VTOTAL=%3d BLANK=%3d-%3d refresh=%f\n",
						htotal, SMART_IOREG(HEBLNK), SMART_IOREG(HSBLNK), vtotal, veblnk, vsblnk, ATTOSECONDS_TO_HZ(refresh)));

				/* interlaced timing not supported */
				if ((SMART_IOREG(DPYCTL) & 0x4000) == 0)
					fatalerror("Interlaced video configured on the TMS34010 (unsupported)");
			}
		}
	}
#endif

	if (vcount >= 0 && vcount <= vsblnk) {
		tms34010_display_params params;

		tms34010_get_display_params(&params);

		// screen update callback - clipping is done in the driver -dink
		if (render) {
			render(line, &params);
		}
	}

	/* if we are in the visible area, increment DPYADR by DUDATE */
	if (vcount >= veblnk && vcount < vsblnk)
	{
		/* 34010 increments by the DUDATE field in DPYCTL */
		if (!state.is_34020)
		{
			UINT16 dpyadr = IOREG(REG_DPYADR);
			if ((dpyadr & 3) == 0)
				dpyadr = ((dpyadr & 0xfffc) - (IOREG(REG_DPYCTL) & 0x03fc)) | (IOREG(REG_DPYSTRT) & 0x0003);
			else
				dpyadr = (dpyadr & 0xfffc) | ((dpyadr - 1) & 3);
			IOREG(REG_DPYADR) = dpyadr;
		}

		/* 34020 updates based on the DINC register, including zoom */
		else
		{
			UINT32 dpynx = IOREG(REG020_DPYNXL) | (IOREG(REG020_DPYNXH) << 16);
			UINT32 dinc = IOREG(REG020_DINCL) | (IOREG(REG020_DINCH) << 16);
			dpynx = (dpynx & 0xffffffe0) | ((dpynx + dinc) & 0x1f);
			if ((dpynx & 0x1f) == 0)
				dpynx += dinc & 0xffffffe0;
			IOREG(REG020_DPYNXL) = dpynx;
			IOREG(REG020_DPYNXH) = dpynx >> 16;
		}
	}

	/* adjust for the next callback */
	vcount++;
	if (vcount > vtotal)
		vcount = 0;

	/* note that we add !master (0 or 1) as a attoseconds value; this makes no practical difference */
	/* but helps ensure that masters are updated first before slaves */
	//timer_adjust_oneshot(state.scantimer, attotime_add_attoseconds(video_screen_get_time_until_pos(state.screen, vcount, 0), !master), cpunum | (vcount << 8));
}


void tms34010_get_display_params(tms34010_display_params *params)
{
	params->enabled = ((SMART_IOREG(DPYCTL) & 0x8000) != 0);
	params->vcount = SMART_IOREG(VCOUNT);
	params->veblnk = SMART_IOREG(VEBLNK);
	params->vsblnk = SMART_IOREG(VSBLNK);
	params->heblnk = SMART_IOREG(HEBLNK) * state.config.pixperclock;
	params->hsblnk = SMART_IOREG(HSBLNK) * state.config.pixperclock;

	params->htotal = SMART_IOREG(HTOTAL);
	params->vtotal = SMART_IOREG(VTOTAL);

	/* 34010 gets its address from DPYADR and DPYTAP */
	if (!state.is_34020)
	{
		UINT16 dpyadr = IOREG(REG_DPYADR);
		if (!(IOREG(REG_DPYCTL) & 0x0400))
			dpyadr ^= 0xfffc;
		params->rowaddr = dpyadr >> 4;
		params->coladdr = ((dpyadr & 0x007c) << 4) | (IOREG(REG_DPYTAP) & 0x3fff);
		params->yoffset = (IOREG(REG_DPYSTRT) - IOREG(REG_DPYADR)) & 3;
	}

	/* 34020 gets its address from DPYNX */
	else
	{
		params->rowaddr = IOREG(REG020_DPYNXH);
		params->coladdr = IOREG(REG020_DPYNXL) & 0xffe0;
		params->yoffset = 0;
		if ((IOREG(REG020_DINCL) & 0x1f) != 0)
			params->yoffset = (IOREG(REG020_DPYNXL) & 0x1f) / (IOREG(REG020_DINCL) & 0x1f);
	}
}

#if 0
VIDEO_UPDATE( tms340x0 )
{
	pen_t blackpen = get_black_pen(screen->machine);
	tms34010_display_params params;
	int cpunum = -1;
	int x;

	/* find the owning CPU */
	for (cpunum = 0; cpunum < MAX_CPU; cpunum++)
		if (screen_to_cpu[cpunum] == screen)
			break;

	assert(cpunum >= 0);

	/* get the display parameters for the screen */
	tms34010_get_display_params(cpunum, &params);

	/* if the display is enabled, call the scanline callback */
	if (params.enabled)
	{
		/* call through to the callback */
		LOG(("  Update: scan=%3d ROW=%04X COL=%04X\n", cliprect->min_y, params.rowaddr, params.coladdr));
		(*state.config.scanline_callback)(screen, bitmap, cliprect->min_y, &params);
	}

	/* otherwise, just blank the current scanline */
	else
		params.heblnk = params.hsblnk = cliprect->max_x + 1;

	/* blank out the blank regions */
	if (bitmap->bpp == 16)
	{
		UINT16 *dest = BITMAP_ADDR16(bitmap, cliprect->min_y, 0);
		for (x = cliprect->min_x; x < params.heblnk; x++)
			dest[x] = blackpen;
		for (x = params.hsblnk; x <= cliprect->max_y; x++)
			dest[x] = blackpen;
	}
	else if (bitmap->bpp == 32)
	{
		UINT32 *dest = BITMAP_ADDR32(bitmap, cliprect->min_y, 0);
		for (x = cliprect->min_x; x < params.heblnk; x++)
			dest[x] = blackpen;
		for (x = params.hsblnk; x <= cliprect->max_y; x++)
			dest[x] = blackpen;
	}
	return 0;
}
#endif

/***************************************************************************
    I/O REGISTER WRITES
***************************************************************************/

static const char *const ioreg_name[] =
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

void tms34010_io_register_w(INT32 address, UINT32 data)
{
	int oldreg, newreg;
	INT32 offset = (address >> 4) & 0x1f;
	//bprintf(0, _T("tms34_io_w  %x  %S -> %x\n"), address, ioreg_name[offset], data);

	/* Set register */
	oldreg = IOREG(offset);
	IOREG(offset) = data;

	switch (offset)
	{
		case REG_CONTROL:
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
			//if (data) logerror("Plane masking not supported. PC=%08X\n", activecpu_get_pc());
			break;

		case REG_DPYCTL:
			set_pixel_function();
			break;

		case REG_HSTCTLH:
			/* if the CPU is halting itself, stop execution right away */
			if ((data & 0x8000) && !state.external_host_access)
				tms34010_ICount = 0;
			//cpunum_set_input_line(state.screen->machine, cpunum, INPUT_LINE_HALT, (data & 0x8000) ? ASSERT_LINE : CLEAR_LINE);

			/* NMI issued? */
			if (data & 0x0100) {
				//bprintf(0, _T("NMI issued?\n"));
			}
			break;

		case REG_HSTCTLL:
			/* the TMS34010 can change MSGOUT, can set INTOUT, and can clear INTIN */
			if (!state.external_host_access)
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
				if (state.config.output_int)
					(*state.config.output_int)(1);
			}
			else if ((oldreg & 0x0080) && !(newreg & 0x0080))
			{
				if (state.config.output_int)
					(*state.config.output_int)(0);
			}

			/* input interrupt? (should really be state-based, but the functions don't exist!) */
			if (!(oldreg & 0x0008) && (newreg & 0x0008))
				internal_interrupt_callback(TMS34010_HI);
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
			break;

		case REG_HEBLNK:
		case REG_HSBLNK:
			if (oldreg != data)
				state.hblank_stable = 0;
			break;
	}

	//if (LOG_CONTROL_REGS)
	//	logerror("CPU#%d@%08X: %s = %04X (%d)\n", cpunum, activecpu_get_pc(), ioreg_name[offset], IOREG(offset), video_screen_get_vpos(state.screen));
}


static const char *const ioreg020_name[] =
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

void tms34020_io_register_w(INT32 address, UINT32 data)
{
	int oldreg, newreg;

	INT32 offset = (address >> 4) & 0x3f;

	/* Set register */
	oldreg = IOREG(offset);
	IOREG(offset) = data;

	//if (LOG_CONTROL_REGS)
	//	logerror("CPU#%d@%08X: %s = %04X (%d)\n", cpunum, activecpu_get_pc(), ioreg020_name[offset], IOREG(offset), video_screen_get_vpos(state.screen));

	switch (offset)
	{
		case REG020_CONTROL:
		case REG020_CONTROL2:
			IOREG(REG020_CONTROL) = data;
			IOREG(REG020_CONTROL2) = data;
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
				case 0x20: state.pixelshift = 5; break;
			}
			break;

		case REG020_PMASKL:
		case REG020_PMASKH:
			//if (data) logerror("Plane masking not supported. PC=%08X\n", activecpu_get_pc());
			break;

		case REG020_DPYCTL:
			set_pixel_function();
			break;

		case REG020_HSTCTLH:
			/* if the CPU is halting itself, stop execution right away */
			if ((data & 0x8000) && !state.external_host_access)
				tms34010_ICount = 0;
			//cpunum_set_input_line(state.screen->machine, cpunum, INPUT_LINE_HALT, (data & 0x8000) ? ASSERT_LINE : CLEAR_LINE);

			/* NMI issued? */
			//if (data & 0x0100)
			//	internal_interrupt_callback();
			break;

		case REG020_HSTCTLL:
			/* the TMS34010 can change MSGOUT, can set INTOUT, and can clear INTIN */
			if (!state.external_host_access)
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
				if (state.config.output_int)
					(*state.config.output_int)(1);
			}
			else if ((oldreg & 0x0080) && !(newreg & 0x0080))
			{
				if (state.config.output_int)
					(*state.config.output_int)(0);
			}

			/* input interrupt? (should really be state-based, but the functions don't exist!) */
			if (!(oldreg & 0x0008) && (newreg & 0x0008))
				internal_interrupt_callback(TMS34010_HI);
			else if ((oldreg & 0x0008) && !(newreg & 0x0008))
				IOREG(REG020_INTPEND) &= ~TMS34010_HI;
			break;

		case REG020_INTENB:
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

		case REG020_HEBLNK:
		case REG020_HSBLNK:
			if (oldreg != data)
				state.hblank_stable = 0;
			break;
	}
}



/***************************************************************************
    I/O REGISTER READS
***************************************************************************/

UINT16 tms34010_io_register_r(INT32 address)
{
	int result, total, cyc_per_line;
	INT32 offset = (address >> 4) & 0x1f;

	//bprintf(0, _T("tms34_io_r  %x  %S  rv: 0x%X (%d)\n"), offset, ioreg_name[offset], IOREG(offset), IOREG(offset));

	//if (LOG_CONTROL_REGS)
	//	logerror("CPU#%d@%08X: read %s\n", cpunum, activecpu_get_pc(), ioreg_name[offset]);

	switch (offset)
	{
		case REG_HCOUNT:
			/* scale the horizontal position from screen width to HTOTAL */
			cyc_per_line = state.config.cpu_cyc_per_frame / IOREG(REG_VTOTAL);
			result = TMS34010TotalCycles() % cyc_per_line;
			total = IOREG(REG_HTOTAL) + 1;
			result = result * total / cyc_per_line;

			/* offset by the HBLANK end */
			result += IOREG(REG_HEBLNK);

			/* wrap around */
			if (result > total)
				result -= total;
			return result;

		case REG_REFCNT:
			return (TMS34010TotalCycles() / 16) & 0xfffc;

		case REG_INTPEND:
			result = IOREG(offset);

			/* Cool Pool loops in mainline code on the appearance of the DI, even though they */
			/* have an IRQ handler. For this reason, we return it signalled a bit early in order */
			/* to make it past these loops. */
			//if (SMART_IOREG(VCOUNT) + 1 == SMART_IOREG(DPYINT) &&
			//	attotime_compare(timer_timeleft(state.scantimer), ATTOTIME_IN_HZ(40000000/8/3)) < 0)
			//	result |= TMS34010_DI;
			return result;
	}

	return IOREG(offset);
}


UINT16 tms34020_io_register_r(INT32 address)
{
	int result, total, cyc_per_line;

	INT32 offset = (address >> 4) & 0x3f;

	//if (LOG_CONTROL_REGS)
	//	logerror("CPU#%d@%08X: read %s\n", cpunum, activecpu_get_pc(), ioreg_name[offset]);

	switch (offset)
	{
		case REG020_HCOUNT:
			/* scale the horizontal position from screen width to HTOTAL */
			cyc_per_line = state.config.cpu_cyc_per_frame / IOREG(REG020_VTOTAL);
			result = TMS34010TotalCycles() % cyc_per_line;
			total = IOREG(REG020_HTOTAL) + 1;
			result = result * total / cyc_per_line;

			/* offset by the HBLANK end */
			result += IOREG(REG020_HEBLNK);

			/* wrap around */
			if (result > total)
				result -= total;
			return result;

		case REG020_REFADR:
		{
			int refreshrate = (IOREG(REG020_CONFIG) >> 8) & 7;
			if (refreshrate < 6)
				return (TMS34010TotalCycles() / refreshrate) & 0xffff;
			break;
		}
	}

	return IOREG(offset);
}


#if 0
/***************************************************************************
    UTILITY FUNCTIONS
***************************************************************************/

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



/***************************************************************************
    SAVE STATE
***************************************************************************/

static STATE_POSTLOAD( tms34010_state_postload )
{
	change_pc(TOBYTE(PC));
	set_raster_op();
	set_pixel_function();
}

#endif

/***************************************************************************
    HOST INTERFACE WRITES
***************************************************************************/

void tms34010_host_w(INT32 reg, UINT16 data)
{
	unsigned int addr;

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
			state.external_host_access = 1;
			tms34010_io_register_w(REG_HSTCTLH << 4, data & 0xff00);
			tms34010_io_register_w(REG_HSTCTLL << 4, data & 0x00ff);
			state.external_host_access = 0;
			break;

		/* error case */
		default:
			//logerror("tms34010_host_control_w called on invalid register %d\n", reg);
			break;
	}
}



/***************************************************************************
    HOST INTERFACE READS
***************************************************************************/

UINT16 tms34010_host_r(INT32 reg)
{
	unsigned int addr;
	int result = 0;

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
			//logerror("tms34010_host_control_r called on invalid register %d\n", reg);
			break;
	}

	return result;
}

#if 0

/**************************************************************************
 * Generic set_info
 **************************************************************************/

static void tms34010_set_info(UINT32 _state, cpuinfo *info)
{
	switch (_state)
	{
		/* --- the following bits of info are set as 64-bit signed integers --- */
		case CPUINFO_INT_INPUT_STATE + 0:				set_irq_line(0, info->i);				break;
		case CPUINFO_INT_INPUT_STATE + 1:				set_irq_line(1, info->i);				break;

		case CPUINFO_INT_PC:       						PC = info->i; change_pc(TOBYTE(PC));	break;
		case CPUINFO_INT_REGISTER + TMS34010_PC:		PC = info->i;							break;
		case CPUINFO_INT_SP:
		case CPUINFO_INT_REGISTER + TMS34010_SP:		SP = info->i;							break;
		case CPUINFO_INT_REGISTER + TMS34010_ST:		ST = info->i;							break;
		case CPUINFO_INT_REGISTER + TMS34010_A0:		AREG(0) = info->i;						break;
		case CPUINFO_INT_REGISTER + TMS34010_A1:		AREG(1) = info->i;						break;
		case CPUINFO_INT_REGISTER + TMS34010_A2:		AREG(2) = info->i;						break;
		case CPUINFO_INT_REGISTER + TMS34010_A3:		AREG(3) = info->i;						break;
		case CPUINFO_INT_REGISTER + TMS34010_A4:		AREG(4) = info->i;						break;
		case CPUINFO_INT_REGISTER + TMS34010_A5:		AREG(5) = info->i;						break;
		case CPUINFO_INT_REGISTER + TMS34010_A6:		AREG(6) = info->i;						break;
		case CPUINFO_INT_REGISTER + TMS34010_A7:		AREG(7) = info->i;						break;
		case CPUINFO_INT_REGISTER + TMS34010_A8:		AREG(8) = info->i;						break;
		case CPUINFO_INT_REGISTER + TMS34010_A9:		AREG(9) = info->i;						break;
		case CPUINFO_INT_REGISTER + TMS34010_A10:		AREG(10) = info->i;						break;
		case CPUINFO_INT_REGISTER + TMS34010_A11:		AREG(11) = info->i;						break;
		case CPUINFO_INT_REGISTER + TMS34010_A12:		AREG(12) = info->i;						break;
		case CPUINFO_INT_REGISTER + TMS34010_A13:		AREG(13) = info->i;						break;
		case CPUINFO_INT_REGISTER + TMS34010_A14:		AREG(14) = info->i;						break;
		case CPUINFO_INT_REGISTER + TMS34010_B0:		BREG(0) = info->i;						break;
		case CPUINFO_INT_REGISTER + TMS34010_B1:		BREG(1) = info->i;						break;
		case CPUINFO_INT_REGISTER + TMS34010_B2:		BREG(2) = info->i;						break;
		case CPUINFO_INT_REGISTER + TMS34010_B3:		BREG(3) = info->i;						break;
		case CPUINFO_INT_REGISTER + TMS34010_B4:		BREG(4) = info->i;						break;
		case CPUINFO_INT_REGISTER + TMS34010_B5:		BREG(5) = info->i;						break;
		case CPUINFO_INT_REGISTER + TMS34010_B6:		BREG(6) = info->i;						break;
		case CPUINFO_INT_REGISTER + TMS34010_B7:		BREG(7) = info->i;						break;
		case CPUINFO_INT_REGISTER + TMS34010_B8:		BREG(8) = info->i;						break;
		case CPUINFO_INT_REGISTER + TMS34010_B9:		BREG(9) = info->i;						break;
		case CPUINFO_INT_REGISTER + TMS34010_B10:		BREG(10) = info->i;						break;
		case CPUINFO_INT_REGISTER + TMS34010_B11:		BREG(11) = info->i;						break;
		case CPUINFO_INT_REGISTER + TMS34010_B12:		BREG(12) = info->i;						break;
		case CPUINFO_INT_REGISTER + TMS34010_B13:		BREG(13) = info->i;						break;
		case CPUINFO_INT_REGISTER + TMS34010_B14:		BREG(14) = info->i;						break;
	}
}



/**************************************************************************
 * Generic get_info
 **************************************************************************/

void tms34010_get_info(UINT32 _state, cpuinfo *info)
{
	switch (_state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case CPUINFO_INT_CONTEXT_SIZE:					info->i = TMS34010_STATE_SIZE;			break;
		case CPUINFO_INT_INPUT_LINES:					info->i = 2;							break;
		case CPUINFO_INT_DEFAULT_IRQ_VECTOR:			info->i = 0;							break;
		case CPUINFO_INT_ENDIANNESS:					info->i = CPU_IS_LE;					break;
		case CPUINFO_INT_CLOCK_MULTIPLIER:				info->i = 1;							break;
		case CPUINFO_INT_CLOCK_DIVIDER:					info->i = 8;							break;
		case CPUINFO_INT_MIN_INSTRUCTION_BYTES:			info->i = 2;							break;
		case CPUINFO_INT_MAX_INSTRUCTION_BYTES:			info->i = 10;							break;
		case CPUINFO_INT_MIN_CYCLES:					info->i = 1;							break;
		case CPUINFO_INT_MAX_CYCLES:					info->i = 10000;						break;

		case CPUINFO_INT_DATABUS_WIDTH + ADDRESS_SPACE_PROGRAM:	info->i = 16;					break;
		case CPUINFO_INT_ADDRBUS_WIDTH + ADDRESS_SPACE_PROGRAM: info->i = 32;					break;
		case CPUINFO_INT_ADDRBUS_SHIFT + ADDRESS_SPACE_PROGRAM: info->i = 3;					break;
		case CPUINFO_INT_DATABUS_WIDTH + ADDRESS_SPACE_DATA:	info->i = 0;					break;
		case CPUINFO_INT_ADDRBUS_WIDTH + ADDRESS_SPACE_DATA: 	info->i = 0;					break;
		case CPUINFO_INT_ADDRBUS_SHIFT + ADDRESS_SPACE_DATA: 	info->i = 0;					break;
		case CPUINFO_INT_DATABUS_WIDTH + ADDRESS_SPACE_IO:		info->i = 0;					break;
		case CPUINFO_INT_ADDRBUS_WIDTH + ADDRESS_SPACE_IO: 		info->i = 0;					break;
		case CPUINFO_INT_ADDRBUS_SHIFT + ADDRESS_SPACE_IO: 		info->i = 0;					break;

		case CPUINFO_INT_INPUT_STATE + 0:				info->i = (IOREG(REG_INTPEND) & TMS34010_INT1) ? ASSERT_LINE : CLEAR_LINE; break;
		case CPUINFO_INT_INPUT_STATE + 1:				info->i = (IOREG(REG_INTPEND) & TMS34010_INT2) ? ASSERT_LINE : CLEAR_LINE; break;

		case CPUINFO_INT_PREVIOUSPC:					/* not implemented */					break;

		case CPUINFO_INT_PC:
		case CPUINFO_INT_REGISTER + TMS34010_PC:		info->i = PC;							break;
		case CPUINFO_INT_SP:
		case CPUINFO_INT_REGISTER + TMS34010_SP:		info->i = SP;							break;
		case CPUINFO_INT_REGISTER + TMS34010_ST:		info->i = ST;							break;
		case CPUINFO_INT_REGISTER + TMS34010_A0:		info->i = AREG(0);						break;
		case CPUINFO_INT_REGISTER + TMS34010_A1:		info->i = AREG(1);						break;
		case CPUINFO_INT_REGISTER + TMS34010_A2:		info->i = AREG(2);						break;
		case CPUINFO_INT_REGISTER + TMS34010_A3:		info->i = AREG(3);						break;
		case CPUINFO_INT_REGISTER + TMS34010_A4:		info->i = AREG(4);						break;
		case CPUINFO_INT_REGISTER + TMS34010_A5:		info->i = AREG(5);						break;
		case CPUINFO_INT_REGISTER + TMS34010_A6:		info->i = AREG(6);						break;
		case CPUINFO_INT_REGISTER + TMS34010_A7:		info->i = AREG(7);						break;
		case CPUINFO_INT_REGISTER + TMS34010_A8:		info->i = AREG(8);						break;
		case CPUINFO_INT_REGISTER + TMS34010_A9:		info->i = AREG(9);						break;
		case CPUINFO_INT_REGISTER + TMS34010_A10:		info->i = AREG(10);						break;
		case CPUINFO_INT_REGISTER + TMS34010_A11:		info->i = AREG(11);						break;
		case CPUINFO_INT_REGISTER + TMS34010_A12:		info->i = AREG(12);						break;
		case CPUINFO_INT_REGISTER + TMS34010_A13:		info->i = AREG(13);						break;
		case CPUINFO_INT_REGISTER + TMS34010_A14:		info->i = AREG(14);						break;
		case CPUINFO_INT_REGISTER + TMS34010_B0:		info->i = BREG(0);						break;
		case CPUINFO_INT_REGISTER + TMS34010_B1:		info->i = BREG(1);						break;
		case CPUINFO_INT_REGISTER + TMS34010_B2:		info->i = BREG(2);						break;
		case CPUINFO_INT_REGISTER + TMS34010_B3:		info->i = BREG(3);						break;
		case CPUINFO_INT_REGISTER + TMS34010_B4:		info->i = BREG(4);						break;
		case CPUINFO_INT_REGISTER + TMS34010_B5:		info->i = BREG(5);						break;
		case CPUINFO_INT_REGISTER + TMS34010_B6:		info->i = BREG(6);						break;
		case CPUINFO_INT_REGISTER + TMS34010_B7:		info->i = BREG(7);						break;
		case CPUINFO_INT_REGISTER + TMS34010_B8:		info->i = BREG(8);						break;
		case CPUINFO_INT_REGISTER + TMS34010_B9:		info->i = BREG(9);						break;
		case CPUINFO_INT_REGISTER + TMS34010_B10:		info->i = BREG(10);						break;
		case CPUINFO_INT_REGISTER + TMS34010_B11:		info->i = BREG(11);						break;
		case CPUINFO_INT_REGISTER + TMS34010_B12:		info->i = BREG(12);						break;
		case CPUINFO_INT_REGISTER + TMS34010_B13:		info->i = BREG(13);						break;
		case CPUINFO_INT_REGISTER + TMS34010_B14:		info->i = BREG(14);						break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case CPUINFO_PTR_SET_INFO:						info->setinfo = tms34010_set_info;		break;
		case CPUINFO_PTR_GET_CONTEXT:					info->getcontext = tms34010_get_context; break;
		case CPUINFO_PTR_SET_CONTEXT:					info->setcontext = tms34010_set_context; break;
		case CPUINFO_PTR_INIT:							info->init = tms34010_init;				break;
		case CPUINFO_PTR_RESET:							info->reset = tms34010_reset;			break;
		case CPUINFO_PTR_EXIT:							info->exit = tms34010_exit;				break;
		case CPUINFO_PTR_EXECUTE:						info->execute = tms34010_execute;		break;
		case CPUINFO_PTR_BURN:							info->burn = NULL;						break;
		case CPUINFO_PTR_DISASSEMBLE:					info->disassemble = tms34010_dasm;		break;
		case CPUINFO_PTR_INSTRUCTION_COUNTER:			info->icount = &tms34010_ICount;		break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case CPUINFO_STR_NAME:							strcpy(info->s, "TMS34010");			break;
		case CPUINFO_STR_CORE_FAMILY:					strcpy(info->s, "Texas Instruments 340x0"); break;
		case CPUINFO_STR_CORE_VERSION:					strcpy(info->s, "1.0");					break;
		case CPUINFO_STR_CORE_FILE:						strcpy(info->s, __FILE__);				break;
		case CPUINFO_STR_CORE_CREDITS:					strcpy(info->s, "Copyright Alex Pasadyn/Zsolt Vasvari\nParts based on code by Aaron Giles"); break;

		case CPUINFO_STR_FLAGS:
			sprintf(info->s, "%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c",
				state.st & 0x80000000 ? 'N':'.',
				state.st & 0x40000000 ? 'C':'.',
				state.st & 0x20000000 ? 'Z':'.',
				state.st & 0x10000000 ? 'V':'.',
				state.st & 0x02000000 ? 'P':'.',
				state.st & 0x00200000 ? 'I':'.',
				state.st & 0x00000800 ? 'E':'.',
				state.st & 0x00000400 ? 'F':'.',
				state.st & 0x00000200 ? 'F':'.',
				state.st & 0x00000100 ? 'F':'.',
				state.st & 0x00000080 ? 'F':'.',
				state.st & 0x00000040 ? 'F':'.',
				state.st & 0x00000020 ? 'E':'.',
				state.st & 0x00000010 ? 'F':'.',
				state.st & 0x00000008 ? 'F':'.',
				state.st & 0x00000004 ? 'F':'.',
				state.st & 0x00000002 ? 'F':'.',
				state.st & 0x00000001 ? 'F':'.');
			break;

		case CPUINFO_STR_REGISTER + TMS34010_PC:		sprintf(info->s, "PC :%08X", state.pc); break;
		case CPUINFO_STR_REGISTER + TMS34010_SP:		sprintf(info->s, "SP :%08X", AREG(15)); break;
		case CPUINFO_STR_REGISTER + TMS34010_ST:		sprintf(info->s, "ST :%08X", state.st); break;
		case CPUINFO_STR_REGISTER + TMS34010_A0:		sprintf(info->s, "A0 :%08X", AREG( 0)); break;
		case CPUINFO_STR_REGISTER + TMS34010_A1:		sprintf(info->s, "A1 :%08X", AREG( 1)); break;
		case CPUINFO_STR_REGISTER + TMS34010_A2:		sprintf(info->s, "A2 :%08X", AREG( 2)); break;
		case CPUINFO_STR_REGISTER + TMS34010_A3:		sprintf(info->s, "A3 :%08X", AREG( 3)); break;
		case CPUINFO_STR_REGISTER + TMS34010_A4:		sprintf(info->s, "A4 :%08X", AREG( 4)); break;
		case CPUINFO_STR_REGISTER + TMS34010_A5:		sprintf(info->s, "A5 :%08X", AREG( 5)); break;
		case CPUINFO_STR_REGISTER + TMS34010_A6:		sprintf(info->s, "A6 :%08X", AREG( 6)); break;
		case CPUINFO_STR_REGISTER + TMS34010_A7:		sprintf(info->s, "A7 :%08X", AREG( 7)); break;
		case CPUINFO_STR_REGISTER + TMS34010_A8:		sprintf(info->s, "A8 :%08X", AREG( 8)); break;
		case CPUINFO_STR_REGISTER + TMS34010_A9:		sprintf(info->s, "A9 :%08X", AREG( 9)); break;
		case CPUINFO_STR_REGISTER + TMS34010_A10:		sprintf(info->s,"A10:%08X", AREG(10)); break;
		case CPUINFO_STR_REGISTER + TMS34010_A11:		sprintf(info->s,"A11:%08X", AREG(11)); break;
		case CPUINFO_STR_REGISTER + TMS34010_A12:		sprintf(info->s,"A12:%08X", AREG(12)); break;
		case CPUINFO_STR_REGISTER + TMS34010_A13:		sprintf(info->s,"A13:%08X", AREG(13)); break;
		case CPUINFO_STR_REGISTER + TMS34010_A14:		sprintf(info->s,"A14:%08X", AREG(14)); break;
		case CPUINFO_STR_REGISTER + TMS34010_B0:		sprintf(info->s, "B0 :%08X", BREG( 0)); break;
		case CPUINFO_STR_REGISTER + TMS34010_B1:		sprintf(info->s, "B1 :%08X", BREG( 1)); break;
		case CPUINFO_STR_REGISTER + TMS34010_B2:		sprintf(info->s, "B2 :%08X", BREG( 2)); break;
		case CPUINFO_STR_REGISTER + TMS34010_B3:		sprintf(info->s, "B3 :%08X", BREG( 3)); break;
		case CPUINFO_STR_REGISTER + TMS34010_B4:		sprintf(info->s, "B4 :%08X", BREG( 4)); break;
		case CPUINFO_STR_REGISTER + TMS34010_B5:		sprintf(info->s, "B5 :%08X", BREG( 5)); break;
		case CPUINFO_STR_REGISTER + TMS34010_B6:		sprintf(info->s, "B6 :%08X", BREG( 6)); break;
		case CPUINFO_STR_REGISTER + TMS34010_B7:		sprintf(info->s, "B7 :%08X", BREG( 7)); break;
		case CPUINFO_STR_REGISTER + TMS34010_B8:		sprintf(info->s, "B8 :%08X", BREG( 8)); break;
		case CPUINFO_STR_REGISTER + TMS34010_B9:		sprintf(info->s, "B9 :%08X", BREG( 9)); break;
		case CPUINFO_STR_REGISTER + TMS34010_B10:		sprintf(info->s,"B10:%08X", BREG(10)); break;
		case CPUINFO_STR_REGISTER + TMS34010_B11:		sprintf(info->s,"B11:%08X", BREG(11)); break;
		case CPUINFO_STR_REGISTER + TMS34010_B12:		sprintf(info->s,"B12:%08X", BREG(12)); break;
		case CPUINFO_STR_REGISTER + TMS34010_B13:		sprintf(info->s,"B13:%08X", BREG(13)); break;
		case CPUINFO_STR_REGISTER + TMS34010_B14:		sprintf(info->s,"B14:%08X", BREG(14)); break;
	}
}


#if (HAS_TMS34020)
/**************************************************************************
 * CPU-specific set_info
 **************************************************************************/

void tms34020_get_info(UINT32 _state, cpuinfo *info)
{
	switch (_state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case CPUINFO_INT_CONTEXT_SIZE:					info->i = TMS34020_STATE_SIZE;			break;
		case CPUINFO_INT_CLOCK_MULTIPLIER:				info->i = 1;							break;
		case CPUINFO_INT_CLOCK_DIVIDER:					info->i = 4;							break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case CPUINFO_PTR_GET_CONTEXT:					info->getcontext = tms34020_get_context; break;
		case CPUINFO_PTR_SET_CONTEXT:					info->setcontext = tms34020_set_context; break;
		case CPUINFO_PTR_RESET:							info->reset = tms34020_reset;			break;
		case CPUINFO_PTR_DISASSEMBLE:					info->disassemble = tms34020_dasm;		break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case CPUINFO_STR_NAME:							strcpy(info->s, "TMS34020");			break;

		default:										tms34010_get_info(_state, info);		break;
	}
}
#endif

#endif
