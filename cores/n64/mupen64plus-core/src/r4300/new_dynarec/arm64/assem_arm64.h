#ifndef M64P_R4300_ASSEM_ARM64_H
#define M64P_R4300_ASSEM_ARM64_H

#define HOST_REGS 29
#define HOST_CCREG 20 /* callee-save */
#define HOST_BTREG 19 /* callee-save */
#define EXCLUDE_REG 29 /* FP */

#define HOST_IMM8 1
//#define HAVE_CMOV_IMM 1
#define CORTEX_A8_BRANCH_PREDICTION_HACK 1
#define USE_MINI_HT 1
//#define REG_PREFETCH 1
//#define HAVE_CONDITIONAL_CALL 1
#define RAM_OFFSET 1

/* ARM calling convention:
   x0-x18: caller-save
   x19-x28: callee-save */

#define ARG1_REG 0
#define ARG2_REG 1
#define ARG3_REG 2
#define ARG4_REG 3

/* GCC register naming convention:
   x16 = ip0 (scratch)
   x17 = ip1 (scratch)
   x29 = fp (frame pointer)
   x30 = lr (link register)
   x31 = sp (stack pointer) */

#define FP 29
#define LR 30
#define WZR 31
#define XZR WZR
#define HOST_TEMPREG 30

// Note: FP is set to &dynarec_local when executing generated code.
// Thus the local variables are actually global and not on the stack.

#define BASE_ADDR ((intptr_t)(&extra_memory))
#define TARGET_SIZE_2 25 // 2^25 = 32 megabytes
#define JUMP_TABLE_SIZE (0)

void jump_vaddr(void);
void jump_vaddr_x0(void);
void jump_vaddr_x1(void);
void jump_vaddr_x2(void);
void jump_vaddr_x3(void);
void jump_vaddr_x4(void);
void jump_vaddr_x5(void);
void jump_vaddr_x6(void);
void jump_vaddr_x7(void);
void jump_vaddr_x8(void);
void jump_vaddr_x9(void);
void jump_vaddr_x10(void);
void jump_vaddr_x11(void);
void jump_vaddr_x12(void);
void jump_vaddr_x13(void);
void jump_vaddr_x14(void);
void jump_vaddr_x15(void);
void jump_vaddr_x16(void);
void jump_vaddr_x17(void);
void jump_vaddr_x18(void);
void jump_vaddr_x19(void);
void jump_vaddr_x20(void);
void jump_vaddr_x21(void);
void jump_vaddr_x22(void);
void jump_vaddr_x23(void);
void jump_vaddr_x24(void);
void jump_vaddr_x25(void);
void jump_vaddr_x26(void);
void jump_vaddr_x27(void);
void jump_vaddr_x28(void);
void invalidate_addr_x0(void);
void invalidate_addr_x1(void);
void invalidate_addr_x2(void);
void invalidate_addr_x3(void);
void invalidate_addr_x4(void);
void invalidate_addr_x5(void);
void invalidate_addr_x6(void);
void invalidate_addr_x7(void);
void invalidate_addr_x8(void);
void invalidate_addr_x9(void);
void invalidate_addr_x10(void);
void invalidate_addr_x11(void);
void invalidate_addr_x12(void);
void invalidate_addr_x13(void);
void invalidate_addr_x14(void);
void invalidate_addr_x15(void);
void invalidate_addr_x16(void);
void invalidate_addr_x17(void);
void invalidate_addr_x18(void);
void invalidate_addr_x19(void);
void invalidate_addr_x20(void);
void invalidate_addr_x21(void);
void invalidate_addr_x22(void);
void invalidate_addr_x23(void);
void invalidate_addr_x24(void);
void invalidate_addr_x25(void);
void invalidate_addr_x26(void);
void invalidate_addr_x27(void);
void invalidate_addr_x28(void);
void indirect_jump_indexed(void);
void indirect_jump(void);
void verify_code(void);
void verify_code_vm(void);
void verify_code_ds(void);
void cc_interrupt(void);
void do_interrupt(void);
void fp_exception(void);
void fp_exception_ds(void);
void jump_syscall(void);
void jump_eret(void);
void read_nomem_new(void);
void read_nomemb_new(void);
void read_nomemh_new(void);
void read_nomemd_new(void);
void write_nomem_new(void);
void write_nomemb_new(void);
void write_nomemh_new(void);
void write_nomemd_new(void);
void write_rdram_new(void);
void write_rdramb_new(void);
void write_rdramh_new(void);
void write_rdramd_new(void);
void breakpoint(void);

extern char *invc_ptr;
extern char extra_memory[33554432];
extern int cycle_count;
extern int last_count;
extern int branch_target;
extern uint64_t ram_offset;
extern uint64_t readmem_dword;
extern struct precomp_instr fake_pc;
extern void *dynarec_local;
extern uint64_t memory_map[1048576];
extern uint64_t mini_ht[32][2];
extern u_int rounding_modes[4];
extern u_char restore_candidate[512];

#endif /* M64P_R4300_ASSEM_ARM64_H */
