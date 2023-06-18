#ifndef S2650_H
#define S2650_H

#include "osd_cpu.h"

enum {
	S2650_PC=1, S2650_PS, S2650_R0, S2650_R1, S2650_R2, S2650_R3,
	S2650_R1A, S2650_R2A, S2650_R3A,
	S2650_HALT, S2650_IRQ_STATE, S2650_SI, S2650_FO
};

/* fake control port   M/~IO=0 D/~C=0 E/~NE=0 */
#define S2650_CTRL_PORT 0x100

/* fake data port      M/~IO=0 D/~C=1 E/~NE=0 */
#define S2650_DATA_PORT 0x101

/* extended i/o ports  M/~IO=0 D/~C=x E/~NE=1 */
#define S2650_EXT_PORT	0xff

/* Fake Sense Line */
#define S2650_SENSE_PORT 0x102

extern int s2650_ICount;

extern void s2650_init(void);
extern void s2650_reset(void *param);
extern void s2650_exit(void);
extern int s2650_execute(int cycles);
extern unsigned s2650_get_context(void *dst);
extern void s2650_set_context(void *src);
extern unsigned s2650_get_reg(int regnum);
extern void s2650_set_reg(int regnum, unsigned val);
extern void s2650_set_irq_line(int irqline, int state);
extern void s2650_set_irq_callback(int (*callback)(int irqline));
extern void s2650_state_save(void *file);
extern void s2650_state_load(void *file);
extern const char *s2650_info(void *context, int regnum);
extern unsigned s2650_dasm(char *buffer, unsigned pc);

extern void s2650_set_flag(int state);
extern int s2650_get_flag(void);
extern void s2650_set_sense(int state);
extern int s2650_get_sense(void);

#ifdef  MAME_DEBUG
extern int Dasm2650(char *buff, int PC);
#endif

#endif
