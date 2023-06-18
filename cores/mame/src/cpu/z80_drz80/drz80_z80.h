#ifndef DRZ80_Z80_H
#define DRZ80_Z80_H

#include "cpuintrf.h"
#include "osd_cpu.h"

extern int drz80_ICount;

#define DRZ80_IGNORE_INT  -1    /* Ignore interrupt */
#define DRZ80_NMI_INT 	-2	/* Execute NMI */
#define DRZ80_IRQ_INT 	-1000	/* Execute IRQ */

extern void drz80_init(void);
extern void drz80_reset (void *param);
extern void drz80_exit (void);
extern int drz80_execute(int cycles);
extern void drz80_burn(int cycles);
extern unsigned drz80_get_context (void *dst);
extern void drz80_set_context (void *src);
extern const void *drz80_get_cycle_table (int which);
extern void drz80_set_cycle_table (int which, void *new_tbl);
extern unsigned drz80_get_pc (void);
extern void drz80_set_pc (unsigned val);
extern unsigned drz80_get_sp (void);
extern void drz80_set_sp (unsigned val);
extern unsigned drz80_get_reg (int regnum);
extern void drz80_set_reg (int regnum, unsigned val);
extern void drz80_set_irq_line(int irqline, int state);
extern void drz80_set_irq_callback(int (*irq_callback)(int));
extern void drz80_state_save(void *file);
extern void drz80_state_load(void *file);
extern const char *drz80_info(void *context, int regnum);
extern unsigned drz80_dasm(char *buffer, unsigned pc);

#ifdef __cplusplus
extern "C" {
#endif
extern void Interrupt(void); /* required for DrZ80 int hack */
#ifdef __cplusplus
} /* End of extern "C" */
#endif

#endif
