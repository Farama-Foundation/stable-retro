/*
  tms9900.h

  C Header file for TMS9900 core
*/

#ifndef TMS9900_H
#define TMS9900_H

#include <stdio.h>
#include "driver.h"
#include "osd_cpu.h"

#define TMS9900_ID      0 /* original processor, 1976 (huh... it had some multi-chip ancestors, */
                          /* the 9x0 series)*/
#define TMS9940_ID      1 /* embedded version, 1979 */
#define TMS9980_ID      2 /* 8-bit variant of tms9900.  Two distinct chips actually : tms9980a, */
                          /* and tms9981 with an extra clock and simplified power supply */
#define TMS9985_ID      3 /* 9980 with on-chip 16-bit RAM and decrementer, c. 1978 (never released) */
#define TMS9989_ID      4 /* improved 9980, used in bombs, missiles, and other *nice* hardware */
#define TMS9995_ID      5 /* tms9985-like, with many improvements */
#define TMS99105A_ID    6 /* late variant, widely improved, 1981 */
#define TMS99110A_ID    7 /* same as above, with floating point support, c. 1981 */



enum {
	TMS9900_PC=1, TMS9900_WP, TMS9900_STATUS, TMS9900_IR
#ifdef MAME_DEBUG
	,
	TMS9900_R0, TMS9900_R1, TMS9900_R2, TMS9900_R3,
	TMS9900_R4, TMS9900_R5, TMS9900_R6, TMS9900_R7,
	TMS9900_R8, TMS9900_R9, TMS9900_R10, TMS9900_R11,
	TMS9900_R12, TMS9900_R13, TMS9900_R14, TMS9900_R15
#endif
};

#if (HAS_TMS9900)

extern	int tms9900_ICount;

extern void tms9900_init(void);
extern void tms9900_reset(void *param);
extern int tms9900_execute(int cycles);
extern void tms9900_exit(void);
extern unsigned tms9900_get_context(void *dst);
extern void tms9900_set_context(void *src);
extern unsigned tms9900_get_reg(int regnum);
extern void tms9900_set_reg(int regnum, unsigned val);
extern void tms9900_set_irq_line(int irqline, int state);
extern void tms9900_set_irq_callback(int (*callback)(int irqline));
extern const char *tms9900_info(void *context, int regnum);
extern unsigned tms9900_dasm(char *buffer, unsigned pc);

#endif

#if (HAS_TMS9940)

extern	int tms9940_ICount;

extern void tms9940_init(void);
extern void tms9940_reset(void *param);
extern int tms9940_execute(int cycles);
extern void tms9940_exit(void);
extern unsigned tms9940_get_context(void *dst);
extern void tms9940_set_context(void *src);
extern unsigned tms9940_get_reg(int regnum);
extern void tms9940_set_reg(int regnum, unsigned val);
extern void tms9940_set_irq_line(int irqline, int state);
extern void tms9940_set_irq_callback(int (*callback)(int irqline));
extern const char *tms9940_info(void *context, int regnum);
extern unsigned tms9940_dasm(char *buffer, unsigned pc);

#endif

#if (HAS_TMS9980)

extern	int tms9980a_ICount;

extern void tms9980a_init(void);
extern void tms9980a_reset(void *param);
extern int tms9980a_execute(int cycles);
extern void tms9980a_exit(void);
extern unsigned tms9980a_get_context(void *dst);
extern void tms9980a_set_context(void *src);
extern unsigned tms9980a_get_reg(int regnum);
extern void tms9980a_set_reg(int regnum, unsigned val);
extern void tms9980a_set_irq_line(int irqline, int state);
extern void tms9980a_set_irq_callback(int (*callback)(int irqline));
extern const char *tms9980a_info(void *context, int regnum);
extern unsigned tms9980a_dasm(char *buffer, unsigned pc);

#endif

#if (HAS_TMS9985)

extern	int tms9985_ICount;

extern void tms9985_init(void);
extern void tms9985_reset(void *param);
extern int tms9985_execute(int cycles);
extern void tms9985_exit(void);
extern unsigned tms9985_get_context(void *dst);
extern void tms9985_set_context(void *src);
extern unsigned tms9985_get_reg(int regnum);
extern void tms9985_set_reg(int regnum, unsigned val);
extern void tms9985_set_irq_line(int irqline, int state);
extern void tms9985_set_irq_callback(int (*callback)(int irqline));
extern const char *tms9985_info(void *context, int regnum);
extern unsigned tms9985_dasm(char *buffer, unsigned pc);

#endif

#if (HAS_TMS9989)

extern	int tms9989_ICount;

extern void tms9989_init(void);
extern void tms9989_reset(void *param);
extern int tms9989_execute(int cycles);
extern void tms9989_exit(void);
extern unsigned tms9989_get_context(void *dst);
extern void tms9989_set_context(void *src);
extern unsigned tms9989_get_reg(int regnum);
extern void tms9989_set_reg(int regnum, unsigned val);
extern void tms9989_set_irq_line(int irqline, int state);
extern void tms9989_set_irq_callback(int (*callback)(int irqline));
extern const char *tms9989_info(void *context, int regnum);
extern unsigned tms9989_dasm(char *buffer, unsigned pc);

#endif

#if (HAS_TMS9995)

extern	int tms9995_ICount;

extern void tms9995_init(void);
extern void tms9995_reset(void *param);
extern int tms9995_execute(int cycles);
extern void tms9995_exit(void);
extern unsigned tms9995_get_context(void *dst);
extern void tms9995_set_context(void *src);
extern unsigned tms9995_get_reg(int regnum);
extern void tms9995_set_reg(int regnum, unsigned val);
extern void tms9995_set_irq_line(int irqline, int state);
extern void tms9995_set_irq_callback(int (*callback)(int irqline));
extern const char *tms9995_info(void *context, int regnum);
extern unsigned tms9995_dasm(char *buffer, unsigned pc);

/*
  structure with the parameters tms9995_reset wants.
*/
typedef struct tms9995reset_param
{
	/* auto_wait_state : a non-zero value makes tms9995 generate a wait state automatically on each
	   memory access */
	int auto_wait_state;
} tms9995reset_param;

#endif

#if (HAS_TMS99105A)

extern	int tms99105a_ICount;

extern void tms99105a_init(void);
extern void tms99105a_reset(void *param);
extern int tms99105a_execute(int cycles);
extern void tms99105a_exit(void);
extern unsigned tms99105a_get_context(void *dst);
extern void tms99105a_set_context(void *src);
extern unsigned tms99105a_get_reg(int regnum);
extern void tms99105a_set_reg(int regnum, unsigned val);
extern void tms99105a_set_irq_line(int irqline, int state);
extern void tms99105a_set_irq_callback(int (*callback)(int irqline));
extern const char *tms99105a_info(void *context, int regnum);
extern unsigned tms99105a_dasm(char *buffer, unsigned pc);

#endif

#if (HAS_TMS99110A)

extern	int tms99110A_ICount;

extern void tms99110a_init(void);
extern void tms99110a_reset(void *param);
extern int tms99110a_execute(int cycles);
extern void tms99110a_exit(void);
extern unsigned tms99110a_get_context(void *dst);
extern void tms99110a_set_context(void *src);
extern unsigned tms99110a_get_reg(int regnum);
extern void tms99110a_set_reg(int regnum, unsigned val);
extern void tms99110a_set_irq_line(int irqline, int state);
extern void tms99110a_set_irq_callback(int (*callback)(int irqline));
extern const char *tms99110a_info(void *context, int regnum);
extern unsigned tms99110a_dasm(char *buffer, unsigned pc);

#endif

#ifdef MAME_DEBUG
extern unsigned Dasm9900 (char *buffer, unsigned pc);
#endif

#endif
