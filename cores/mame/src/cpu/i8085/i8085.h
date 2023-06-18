#ifndef I8085_H
#define I8085_H

#include "osd_cpu.h"

enum {
	I8085_PC=1, I8085_SP, I8085_AF ,I8085_BC, I8085_DE, I8085_HL,
	I8085_HALT, I8085_IM, I8085_IREQ, I8085_ISRV, I8085_VECTOR,
	I8085_TRAP_STATE, I8085_INTR_STATE,
	I8085_RST55_STATE, I8085_RST65_STATE, I8085_RST75_STATE};

#define I8085_INTR_LINE     0
#define I8085_RST55_LINE	1
#define I8085_RST65_LINE	2
#define I8085_RST75_LINE	3

extern int i8085_ICount;

extern void i8085_set_SID(int state);
extern void i8085_set_SOD_callback(void (*callback)(int state));
extern void i8085_init(void);
extern void i8085_reset(void *param);
extern void i8085_exit(void);
extern int i8085_execute(int cycles);
extern unsigned i8085_get_context(void *dst);
extern void i8085_set_context(void *src);
extern unsigned i8085_get_reg(int regnum);
extern void i8085_set_reg(int regnum, unsigned val);
extern void i8085_set_irq_line(int irqline, int state);
extern void i8085_set_irq_callback(int (*callback)(int irqline));
extern const char *i8085_info(void *context, int regnum);
extern unsigned i8085_dasm(char *buffer, unsigned pc);

/**************************************************************************
 * I8080 section
 **************************************************************************/
#if (HAS_8080)
#define I8080_PC                I8085_PC
#define I8080_SP				I8085_SP
#define I8080_BC				I8085_BC
#define I8080_DE				I8085_DE
#define I8080_HL				I8085_HL
#define I8080_AF				I8085_AF
#define I8080_HALT				I8085_HALT
#define I8080_IREQ				I8085_IREQ
#define I8080_ISRV				I8085_ISRV
#define I8080_VECTOR			I8085_VECTOR
#define I8080_TRAP_STATE		I8085_TRAP_STATE
#define I8080_INTR_STATE		I8085_INTR_STATE

#define I8080_REG_LAYOUT \
{	CPU_8080, \
	I8080_AF,I8080_BC,I8080_DE,I8080_HL,I8080_SP,I8080_PC, DBG_ROW, \
	I8080_HALT,I8080_IREQ,I8080_ISRV,I8080_VECTOR, I8080_TRAP_STATE,I8080_INTR_STATE, \
    DBG_END }

#define I8080_INTR_LINE         I8085_INTR_LINE

#define     i8080_ICount            i8085_ICount
extern void i8080_init(void);
extern void i8080_reset(void *param);
extern void i8080_exit(void);
extern int i8080_execute(int cycles);
extern unsigned i8080_get_context(void *dst);
extern void i8080_set_context(void *src);
extern unsigned i8080_get_reg(int regnum);
extern void i8080_set_reg(int regnum, unsigned val);
extern void i8080_set_irq_line(int irqline, int state);
extern void i8080_set_irq_callback(int (*callback)(int irqline));
extern const char *i8080_info(void *context, int regnum);
extern unsigned i8080_dasm(char *buffer, unsigned pc);
#endif

#ifdef	MAME_DEBUG
extern unsigned Dasm8085(char *buffer, unsigned pc);
#endif

#endif
