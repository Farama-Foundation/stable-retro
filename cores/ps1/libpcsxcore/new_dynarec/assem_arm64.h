#define HOST_IMM8 1

/* calling convention:
   x0 -x17: caller-save
   x18    : caller-save (platform reg)
   x19-x29: callee-save */

#define HOST_REGS 29
#define EXCLUDE_REG -1

#define SP 31
#define WZR SP
#define XZR SP

#define LR 30
#define HOST_TEMPREG LR

// Note: FP is set to &dynarec_local when executing generated code.
// Thus the local variables are actually global and not on the stack.
#define FP 29
#define rFP x29

#define HOST_CCREG 28
#define rCC w28

#define CALLER_SAVE_REGS 0x0007ffff
#define PREFERRED_REG_FIRST 19
#define PREFERRED_REG_LAST  27

#define DRC_DBG_REGMASK 3 // others done by do_insn_cmp_arm64
#define do_insn_cmp do_insn_cmp_arm64

// stack space
#define SSP_CALLEE_REGS (8*12) // new_dyna_start caller's
#define SSP_CALLER_REGS (8*20)
#define SSP_ALL (SSP_CALLEE_REGS+SSP_CALLER_REGS)

#define TARGET_SIZE_2 24 // 2^24 = 16 megabytes

#ifndef __ASSEMBLER__

extern char *invc_ptr;

struct tramp_insns
{
  u_int ldr;
  u_int br;
};

static void clear_cache_arm64(char *start, char *end);

void do_memhandler_pre();
void do_memhandler_post();

#endif // !__ASSEMBLY__
