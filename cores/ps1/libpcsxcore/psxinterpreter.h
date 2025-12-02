#ifndef __PSXINTERPRETER_H__
#define __PSXINTERPRETER_H__

struct psxRegisters;
struct psxCP2Regs;

// get an opcode without triggering exceptions or affecting cache
u32 intFakeFetch(u32 pc);

// called by "new_dynarec"
void execI(struct psxRegisters *regs);
void intApplyConfig();
void MTC0(struct psxRegisters *regs, int reg, u32 val);
void gteNULL(struct psxCP2Regs *regs);
extern void (*psxCP2[64])(struct psxCP2Regs *regs);

#endif // __PSXINTERPRETER_H__
