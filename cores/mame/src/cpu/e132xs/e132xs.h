#ifndef E132XS_H
#define E132XS_H

/* Functions */
extern void e132xs_init(void);
extern void e132xs_reset(void *param);
extern void e132xs_exit(void);
extern int e132xs_execute(int cycles);
extern unsigned e132xs_get_context(void *regs);
extern void e132xs_set_context(void *regs);
extern unsigned e132xs_get_reg(int regnum);
extern void e132xs_set_reg(int regnum, unsigned val);
extern void e132xs_set_irq_line(int irqline, int state);
extern void e132xs_set_irq_callback(int (*callback)(int irqline));
extern const char *e132xs_info(void *context, int regnum);
extern unsigned e132xs_dasm(char *buffer, unsigned pc);

#ifdef MAME_DEBUG
extern unsigned dasm_e132xs(char *buffer, unsigned pc);
#endif


/* Variables */
extern int e132xs_ICount;

/* read byte */
#define READ_B(addr) (cpu_readmem32bedw(addr))
/* read half-word */
#define READ_HW(addr) (cpu_readmem32bedw_word(addr))
/* read word */
#define READ_W(addr) (cpu_readmem32bedw_dword(addr))

/* write byte */
#define WRITE_B(addr, val) (cpu_writemem32bedw(addr, val))
/* write half-word */
#define WRITE_HW(addr, val) (cpu_writemem32bedw_word(addr, val))
/* write word */
#define WRITE_W(addr, val) (cpu_writemem32bedw_dword(addr, val))

#define READ_OP(addr)	READ_HW(addr)
/*#define READ_OP(addr)	(cpu_readop16(addr))*/


#define PC_CODE			 0
#define SR_CODE			 1

#define X_CODE(val)		 ((val & 0x7000) >> 12)
#define E_BIT(val)		 ((val & 0x8000) >> 15)
#define S_BIT_CONST(val) ((val & 0x4000) >> 14)
#define DD(val)			 ((val & 0x3000) >> 12)


/* Extended DSP instructions */
#define EMUL			0x102
#define EMULU			0x104
#define EMULS			0x106
#define EMAC			0x10a
#define EMACD			0x10e
#define EMSUB			0x11a
#define EMSUBD			0x11e
#define EHMAC			0x02a
#define EHMACD			0x02e
#define EHCMULD			0x046
#define EHCMACD			0x04e
#define EHCSUMD			0x086
#define EHCFFTD			0x096
#define EHCFFTSD		0x296

/* Delay values */
#define NO_DELAY		0
#define DELAY_EXECUTE	1
#define DELAY_TAKEN		2

/* Trap numbers */
#define IO2					48
#define IO1					49
#define INT4				50
#define INT3				51
#define INT2				52
#define INT1				53
#define IO3					54
#define TIMER				55
#define RESERVED1			56
#define TRACE_EXCEPTION		57
#define PARITY_ERROR		58
#define EXTENDED_OVERFLOW	59
#define RANGE_ERROR			60
#define PRIVILEGE_ERROR		RANGE_ERROR
#define FRAME_ERROR			RANGE_ERROR
#define RESERVED2			61
#define RESET				62	/* reserved if not mapped @ MEM3*/
#define ERROR_ENTRY			63	/* for instruction code of all ones*/

/* Traps code */
#define	TRAPLE		4
#define	TRAPGT		5
#define	TRAPLT		6
#define	TRAPGE		7
#define	TRAPSE		8
#define	TRAPHT		9
#define	TRAPST		10
#define	TRAPHE		11
#define	TRAPE		12
#define	TRAPNE		13
#define	TRAPV		14
#define	TRAP		15

/* Entry point to get trap locations or emulated code associated */
#define	E132XS_ENTRY_MEM0	0
#define	E132XS_ENTRY_MEM1	1
#define	E132XS_ENTRY_MEM2	2
#define	E132XS_ENTRY_MEM3	3
#define	E132XS_ENTRY_IRAM	4

#endif /* E132XS_H */
