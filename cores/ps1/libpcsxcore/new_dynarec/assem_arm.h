#define HOST_IMM8 1
#define HAVE_CMOV_IMM 1
#define HAVE_CONDITIONAL_CALL 1

/* ARM calling convention:
   r0-r3, r12: caller-save
   r4-r11: callee-save */

/* GCC register naming convention:
   r10 = sl (base)
   r11 = fp (frame pointer)
   r12 = ip (scratch)
   r13 = sp (stack pointer)
   r14 = lr (link register)
   r15 = pc (program counter) */

#define HOST_REGS 13
#define HOST_CCREG 10
#define EXCLUDE_REG 11

// Note: FP is set to &dynarec_local when executing generated code.
// Thus the local variables are actually global and not on the stack.
#define FP 11
#define LR 14
#define HOST_TEMPREG 14

#ifndef __MACH__
#define CALLER_SAVE_REGS 0x100f
#else
#define CALLER_SAVE_REGS 0x120f
#endif
#define PREFERRED_REG_FIRST 4
#define PREFERRED_REG_LAST  9

#define DRC_DBG_REGMASK CALLER_SAVE_REGS

extern char *invc_ptr;

// note: max due to branch encoding: arm 32M, arm64 128M
#define TARGET_SIZE_2 24 // 2^24 = 16 megabytes

struct tramp_insns
{
  u_int ldrpc;
};

