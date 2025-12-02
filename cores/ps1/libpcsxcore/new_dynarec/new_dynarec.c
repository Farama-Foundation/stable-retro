/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *   Mupen64plus - new_dynarec.c                                           *
 *   Copyright (C) 2009-2011 Ari64                                         *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.          *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include <stdlib.h>
#include <stdint.h> //include for uint64_t
#include <assert.h>
#include <errno.h>
#include <sys/mman.h>
#include <unistd.h>
#ifdef __MACH__
#include <libkern/OSCacheControl.h>
#endif
#ifdef _3DS
#include <3ds_utils.h>
#endif
#ifdef HAVE_LIBNX
#include <switch.h>
static Jit g_jit;
#endif

#include "new_dynarec_config.h"
#include "../psxhle.h"
#include "../psxinterpreter.h"
#include "../psxcounters.h"
#include "../gte.h"
#include "emu_if.h" // emulator interface
#include "linkage_offsets.h"
#include "compiler_features.h"
#include "arm_features.h"

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(x) (sizeof(x) / sizeof(x[0]))
#endif
#ifndef min
#define min(a, b) ((b) < (a) ? (b) : (a))
#endif
#ifndef max
#define max(a, b) ((b) > (a) ? (b) : (a))
#endif

//#define DISASM
//#define ASSEM_PRINT
//#define ASSEM_PRINT_ADDRS
//#define REGMAP_PRINT // with DISASM only
//#define INV_DEBUG_W
//#define STAT_PRINT

#ifdef ASSEM_PRINT
#define assem_debug printf
#else
#define assem_debug(...)
#endif
#ifdef ASSEM_PRINT_ADDRS
#define log_addr(a) (a)
#else
// for diff-able output
#define log_addr(a) ((u_long)(a) <= 1024u ? (void *)(a) : (void *)0xadd0l)
#endif
//#define inv_debug printf
#define inv_debug(...)

#define SysPrintf_lim(...) do { \
  if (err_print_count++ < 64u) \
    SysPrintf(__VA_ARGS__); \
} while (0)

// from linkage_*
extern int cycle_count; // ... until end of the timeslice, counts -N -> 0 (CCREG)
extern int last_count;  // last absolute target, often = next_interupt

extern int reg_cop2d[], reg_cop2c[];

extern void *hash_table_ptr;
extern uintptr_t ram_offset;
extern uintptr_t mini_ht[32][2];

#ifdef __i386__
#include "assem_x86.h"
#endif
#ifdef __x86_64__
#include "assem_x64.h"
#endif
#ifdef __arm__
#include "assem_arm.h"
#endif
#ifdef __aarch64__
#include "assem_arm64.h"
#endif

#define RAM_SIZE 0x200000
#define MAX_OUTPUT_BLOCK_SIZE 262144
#define EXPIRITY_OFFSET (MAX_OUTPUT_BLOCK_SIZE * 2)
#define PAGE_COUNT 1024

#if defined(HAVE_CONDITIONAL_CALL) && !defined(DESTRUCTIVE_SHIFT)
#define INVALIDATE_USE_COND_CALL
#endif

#ifdef VITA
// apparently Vita has a 16MB limit, so either we cut tc in half,
// or use this hack (it's a hack because tc size was designed to be power-of-2)
#define TC_REDUCE_BYTES 4096
#else
#define TC_REDUCE_BYTES 0
#endif

struct ndrc_globals ndrc_g;

struct ndrc_tramp
{
  struct tramp_insns ops[2048 / sizeof(struct tramp_insns)];
  const void *f[2048 / sizeof(void *)];
};

struct ndrc_mem
{
  u_char translation_cache[(1 << TARGET_SIZE_2) - TC_REDUCE_BYTES];
  struct ndrc_tramp tramp;
};

static struct ndrc_mem *ndrc;
#ifndef BASE_ADDR_DYNAMIC
// reserve .bss space with upto 64k page size in mind
static char ndrc_bss[((sizeof(*ndrc) + 65535) & ~65535) + 65536];
#endif
#ifdef TC_WRITE_OFFSET
# ifdef __GLIBC__
# include <sys/types.h>
# include <sys/stat.h>
# include <fcntl.h>
# endif
static long ndrc_write_ofs;
#define NDRC_WRITE_OFFSET(x) (void *)((char *)(x) + ndrc_write_ofs)
#else
#define NDRC_WRITE_OFFSET(x) (x)
#endif

// stubs
enum stub_type {
  CC_STUB = 1,
  //FP_STUB = 2,
  LOADB_STUB = 3,
  LOADH_STUB = 4,
  LOADW_STUB = 5,
  //LOADD_STUB = 6,
  LOADBU_STUB = 7,
  LOADHU_STUB = 8,
  STOREB_STUB = 9,
  STOREH_STUB = 10,
  STOREW_STUB = 11,
  //STORED_STUB = 12,
  STORELR_STUB = 13,
  INVCODE_STUB = 14,
  OVERFLOW_STUB = 15,
  ALIGNMENT_STUB = 16,
};

// regmap_pre[i]    - regs before [i] insn starts; dirty things here that
//                    don't match .regmap will be written back
// [i].regmap_entry - regs that must be set up if someone jumps here
// [i].regmap       - regs [i] insn will read/(over)write
// branch_regs[i].* - same as above but for branches, takes delay slot into account
struct regstat
{
  signed char regmap_entry[HOST_REGS];
  signed char regmap[HOST_REGS];
  u_int wasdirty;
  u_int dirty;
  u_int wasconst;                // before; for example 'lw r2, (r2)' wasconst is true
  u_int isconst;                 //  ... but isconst is false when r2 is known (hr)
  u_int loadedconst;             // host regs that have constants loaded
  u_int noevict;                 // can't evict this hr (alloced by current op)
  //u_int waswritten;              // MIPS regs that were used as store base before
  uint64_t u;
};

struct ht_entry
{
  u_int vaddr[2];
  void *tcaddr[2];
};

struct code_stub
{
  enum stub_type type;
  void *addr;
  void *retaddr;
  u_int a;
  uintptr_t b;
  uintptr_t c;
  u_int d;
  u_int e;
};

struct link_entry
{
  void *addr;
  u_int target;
  u_int internal;
};

struct block_info
{
  struct block_info *next;
  const void *source;
  const void *copy;
  u_int start; // vaddr of the block start
  u_int len;   // of the whole block source
  u_int tc_offs;
  //u_int tc_len;
  u_int reg_sv_flags;
  u_char is_dirty;
  u_char inv_near_misses;
  u_short jump_in_cnt;
  struct {
    u_int vaddr;
    void *addr;
  } jump_in[0];
};

struct jump_info
{
  int alloc;
  int count;
  struct {
    u_int target_vaddr;
    void *stub;
  } e[0];
};

static struct decoded_insn
{
  u_char itype;
  u_char opcode;   // bits 31-26
  u_char opcode2;  // (depends on opcode)
  u_char rs1;
  u_char rs2;
  u_char rt1;
  u_char rt2;
  u_char use_lt1:1;
  u_char bt:1;
  u_char ooo:1;
  u_char is_ds:1;
  u_char is_jump:1;
  u_char is_ujump:1;
  u_char is_load:1;
  u_char is_store:1;
  u_char is_delay_load:1; // is_load + MFC/CFC
  u_char is_exception:1;  // unconditional, also interp. fallback
  u_char may_except:1;    // might generate an exception
  u_char ls_type:2;       // load/store type (ls_width_type LS_*)
} dops[MAXBLOCK];

enum ls_width_type {
  LS_8 = 0, LS_16, LS_32, LS_LR
};

static struct compile_info
{
  int imm;
  u_int ba;
  int ccadj;
  signed char min_free_regs;
  signed char addr;
  signed char reserved[2];
} cinfo[MAXBLOCK];

  static u_char *out;
  static char invalid_code[0x100000];
  static struct ht_entry hash_table[65536];
  static struct block_info *blocks[PAGE_COUNT];
  static struct jump_info *jumps[PAGE_COUNT];
  static u_int start;
  static u_int *source;
  static uint64_t gte_rs[MAXBLOCK]; // gte: 32 data and 32 ctl regs
  static uint64_t gte_rt[MAXBLOCK];
  static uint64_t gte_unneeded[MAXBLOCK];
  unsigned int ndrc_smrv_regs[32]; // speculated MIPS register values
  static u_int smrv_strong; // mask or regs that are likely to have correct values
  static u_int smrv_weak; // same, but somewhat less likely
  static u_int smrv_strong_next; // same, but after current insn executes
  static u_int smrv_weak_next;
  static uint64_t unneeded_reg[MAXBLOCK];
  static uint64_t branch_unneeded_reg[MAXBLOCK];
  // see 'struct regstat' for a description
  static signed char regmap_pre[MAXBLOCK][HOST_REGS];
  // contains 'real' consts at [i] insn, but may differ from what's actually
  // loaded in host reg as 'final' value is always loaded, see get_final_value()
  static uint32_t current_constmap[HOST_REGS];
  static uint32_t constmap[MAXBLOCK][HOST_REGS];
  static struct regstat regs[MAXBLOCK];
  static struct regstat branch_regs[MAXBLOCK];
  static int slen;
  static void *instr_addr[MAXBLOCK];
  static struct link_entry link_addr[MAXBLOCK];
  static int linkcount;
  static struct code_stub stubs[MAXBLOCK*3];
  static int stubcount;
  static u_int literals[1024][2];
  static int literalcount;
  static int is_delayslot;
  static char shadow[1048576]  __attribute__((aligned(16)));
  static void *copy;
  static u_int expirep;
  static u_int stop_after_jal;
  static u_int ni_count;
  static u_int err_print_count;
  static u_int f1_hack;
  static u_int vsync_hack;
#ifdef STAT_PRINT
  static int stat_bc_direct;
  static int stat_bc_pre;
  static int stat_bc_restore;
  static int stat_ht_lookups;
  static int stat_jump_in_lookups;
  static int stat_restore_tries;
  static int stat_restore_compares;
  static int stat_inv_addr_calls;
  static int stat_inv_hits;
  static int stat_blocks;
  static int stat_links;
  #define stat_inc(s) s++
  #define stat_dec(s) s--
  #define stat_clear(s) s = 0
#else
  #define stat_inc(s)
  #define stat_dec(s)
  #define stat_clear(s)
#endif

  #define HACK_ENABLED(x) ((ndrc_g.hacks | ndrc_g.hacks_pergame) & (x))

  /* registers that may be allocated */
  /* 1-31 gpr */
#define LOREG 32 // lo
#define HIREG 33 // hi
//#define FSREG 34 // FPU status (FCSR)
//#define CSREG 35 // Coprocessor status
#define CCREG 36 // Cycle count
#define INVCP 37 // Pointer to invalid_code
//#define MMREG 38 // Pointer to memory_map
#define ROREG 39 // ram offset (if psxM != 0x80000000)
#define TEMPREG 40
#define FTEMP 40 // Load/store temporary register (was fpu)
#define PTEMP 41 // Prefetch temporary register
//#define TLREG 42 // TLB mapping offset
#define RHASH 43 // Return address hash
#define RHTBL 44 // Return address hash table address
#define RTEMP 45 // JR/JALR address register
#define MAXREG 45
#define AGEN1 46 // Address generation temporary register (pass5b_preallocate2)
//#define AGEN2 47 // Address generation temporary register

  /* instruction types */
#define NOP 0     // No operation
#define LOAD 1    // Load
#define STORE 2   // Store
#define LOADLR 3  // Unaligned load
#define STORELR 4 // Unaligned store
#define MOV 5     // Move (hi/lo only)
#define ALU 6     // Arithmetic/logic
#define MULTDIV 7 // Multiply/divide
#define SHIFT 8   // Shift by register
#define SHIFTIMM 9// Shift by immediate
#define IMM16 10  // 16-bit immediate
#define RJUMP 11  // Unconditional jump to register
#define UJUMP 12  // Unconditional jump
#define CJUMP 13  // Conditional branch (BEQ/BNE/BGTZ/BLEZ)
#define SJUMP 14  // Conditional branch (regimm format)
#define COP0 15   // Coprocessor 0
#define RFE 16
#define SYSCALL 22// SYSCALL,BREAK
#define OTHER 23  // Other/unknown - do nothing
#define HLECALL 26// PCSX fake opcodes for HLE
#define COP2 27   // Coprocessor 2 move
#define C2LS 28   // Coprocessor 2 load/store
#define C2OP 29   // Coprocessor 2 operation
#define INTCALL 30// Call interpreter to handle rare corner cases

  /* branch codes */
#define TAKEN 1
#define NOTTAKEN 2

#define DJT_1 (void *)1l // no function, just a label in assem_debug log
#define DJT_2 (void *)2l

// asm linkage
void dyna_linker();
void cc_interrupt();
void jump_syscall   (u_int u0, u_int u1, u_int pc);
void jump_syscall_ds(u_int u0, u_int u1, u_int pc);
void jump_break   (u_int u0, u_int u1, u_int pc);
void jump_break_ds(u_int u0, u_int u1, u_int pc);
void jump_overflow   (u_int u0, u_int u1, u_int pc);
void jump_overflow_ds(u_int u0, u_int u1, u_int pc);
void jump_addrerror   (u_int cause, u_int addr, u_int pc);
void jump_addrerror_ds(u_int cause, u_int addr, u_int pc);
void jump_to_new_pc();
void new_dyna_leave();

void *ndrc_get_addr_ht(u_int vaddr, struct ht_entry *ht);
void ndrc_write_invalidate_one(u_int addr);
static void ndrc_write_invalidate_many(u_int addr, u_int end);

static int new_recompile_block(u_int addr);
static void invalidate_block(struct block_info *block);
static void exception_assemble(int i, const struct regstat *i_regs, int ccadj_);

// Needed by assembler
static void wb_register(signed char r, const signed char regmap[], u_int dirty);
static void wb_dirtys(const signed char i_regmap[], u_int i_dirty);
static void wb_needed_dirtys(const signed char i_regmap[], u_int i_dirty, int addr);
static void load_all_regs(const signed char i_regmap[]);
static void load_needed_regs(const signed char i_regmap[], const signed char next_regmap[]);
static void load_regs_entry(int t);
static void load_all_consts(const signed char regmap[], u_int dirty, int i);
static u_int get_host_reglist(const signed char *regmap);

static int get_final_value(int hr, int i, u_int *value);
static void add_stub(enum stub_type type, void *addr, void *retaddr,
  u_int a, uintptr_t b, uintptr_t c, u_int d, u_int e);
static void add_stub_r(enum stub_type type, void *addr, void *retaddr,
  int i, int addr_reg, const struct regstat *i_regs, int ccadj, u_int reglist);
static void add_to_linker(void *addr, u_int target, int ext);
static void *get_direct_memhandler(void *table, u_int addr,
  enum stub_type type, uintptr_t *addr_host);
static void cop2_do_stall_check(u_int op, int i, const struct regstat *i_regs, u_int reglist);
static void pass_args(int a0, int a1);
static void emit_far_jump(const void *f);
static void emit_far_call(const void *f);

#ifdef VITA
#include <psp2/kernel/sysmem.h>
static int sceBlock;
// note: this interacts with RetroArch's Vita bootstrap code: bootstrap/vita/sbrk.c
extern int getVMBlock();
int _newlib_vm_size_user = sizeof(*ndrc);
#endif

static void mprotect_w_x(void *start, void *end, int is_x)
{
#ifdef NO_WRITE_EXEC
  #if defined(VITA)
  // *Open* enables write on all memory that was
  // allocated by sceKernelAllocMemBlockForVM()?
  if (is_x)
    sceKernelCloseVMDomain();
  else
    sceKernelOpenVMDomain();
  #elif defined(HAVE_LIBNX)
  Result rc;
  // check to avoid the full flush in jitTransitionToExecutable()
  if (g_jit.type != JitType_CodeMemory) {
    if (is_x)
      rc = jitTransitionToExecutable(&g_jit);
    else
      rc = jitTransitionToWritable(&g_jit);
    if (R_FAILED(rc))
      ;//SysPrintf("jitTransition %d %08x\n", is_x, rc);
  }
  #elif defined(TC_WRITE_OFFSET)
  // separated rx and rw areas are always available
  #else
  u_long mstart = (u_long)start & ~4095ul;
  u_long mend = (u_long)end;
  if (mprotect((void *)mstart, mend - mstart,
               PROT_READ | (is_x ? PROT_EXEC : PROT_WRITE)) != 0)
    SysPrintf("mprotect(%c) failed: %s\n", is_x ? 'x' : 'w', strerror(errno));
  #endif
#endif
}

void new_dyna_clear_cache(void *start, void *end)
{
#if defined(__arm__) || defined(__aarch64__)
  size_t len = (char *)end - (char *)start;
  #if   defined(__BLACKBERRY_QNX__)
  msync(start, len, MS_SYNC | MS_CACHE_ONLY | MS_INVALIDATE_ICACHE);
  #elif defined(__MACH__)
  sys_cache_control(kCacheFunctionPrepareForExecution, start, len);
  #elif defined(VITA)
  sceKernelSyncVMDomain(sceBlock, start, len);
  #elif defined(_3DS)
  // tuned for old3ds' 16k:16k cache (in it's mostly clean state...)
  if ((char *)end - (char *)start <= 2*1024)
    ctr_clear_cache_range(start, end);
  else
    ctr_clear_cache();
  #elif defined(HAVE_LIBNX)
  if (g_jit.type == JitType_CodeMemory) {
    armDCacheClean(start, len);
    armICacheInvalidate((char *)start - ndrc_write_ofs, len);
    // as of v4.2.1 libnx lacks isb
    __asm__ volatile("isb" ::: "memory");
  }
  #elif defined(__aarch64__)
  // __clear_cache() doesn't handle differing cacheline sizes on big.LITTLE and
  // leaves it to the kernel to virtualize ctr_el0, which some old kernels don't do
  clear_cache_arm64(start, end);
  #else
  __clear_cache(start, end);
  #endif
  (void)len;
#endif
}

static void start_tcache_write(void *start, void *end)
{
  mprotect_w_x(start, end, 0);
}

static void end_tcache_write(void *start, void *end)
{
#ifdef NDRC_THREAD
  if (!ndrc_g.thread.dirty_start || (size_t)ndrc_g.thread.dirty_start > (size_t)start)
    ndrc_g.thread.dirty_start = start;
  if ((size_t)ndrc_g.thread.dirty_end < (size_t)end)
    ndrc_g.thread.dirty_end = end;
#endif
  new_dyna_clear_cache(start, end);

  mprotect_w_x(start, end, 1);
}

static void *start_block(void)
{
  u_char *end = out + MAX_OUTPUT_BLOCK_SIZE;
  if (end > ndrc->translation_cache + sizeof(ndrc->translation_cache))
    end = ndrc->translation_cache + sizeof(ndrc->translation_cache);
  start_tcache_write(NDRC_WRITE_OFFSET(out), NDRC_WRITE_OFFSET(end));
  return out;
}

static void end_block(void *start)
{
  end_tcache_write(NDRC_WRITE_OFFSET(start), NDRC_WRITE_OFFSET(out));
}

#ifdef NDRC_CACHE_FLUSH_ALL

static int needs_clear_cache;

static void mark_clear_cache(void *target)
{
  if (!needs_clear_cache) {
    start_tcache_write(NDRC_WRITE_OFFSET(ndrc), NDRC_WRITE_OFFSET(ndrc + 1));
    needs_clear_cache = 1;
  }
}

static void do_clear_cache(void)
{
  if (needs_clear_cache) {
    end_tcache_write(NDRC_WRITE_OFFSET(ndrc), NDRC_WRITE_OFFSET(ndrc + 1));
    needs_clear_cache = 0;
  }
}

#else

// also takes care of w^x mappings when patching code
static u_int needs_clear_cache[1<<(TARGET_SIZE_2-17)];

static void mark_clear_cache(void *target)
{
  uintptr_t offset = (u_char *)target - ndrc->translation_cache;
  u_int mask = 1u << ((offset >> 12) & 31);
  if (!(needs_clear_cache[offset >> 17] & mask)) {
    char *start = (char *)NDRC_WRITE_OFFSET((uintptr_t)target & ~4095l);
    start_tcache_write(start, start + 4095);
    needs_clear_cache[offset >> 17] |= mask;
  }
}

// Clearing the cache is rather slow on ARM Linux, so mark the areas
// that need to be cleared, and then only clear these areas once.
static void do_clear_cache(void)
{
  int i, j;
  for (i = 0; i < (1<<(TARGET_SIZE_2-17)); i++)
  {
    u_int bitmap = needs_clear_cache[i];
    if (!bitmap)
      continue;
    for (j = 0; j < 32; j++)
    {
      u_char *start, *end;
      if (!(bitmap & (1u << j)))
        continue;

      start = ndrc->translation_cache + i*131072 + j*4096;
      end = start + 4095;
      for (j++; j < 32; j++) {
        if (!(bitmap & (1u << j)))
          break;
        end += 4096;
      }
      end_tcache_write(NDRC_WRITE_OFFSET(start), NDRC_WRITE_OFFSET(end));
    }
    needs_clear_cache[i] = 0;
  }
}

#endif // NDRC_CACHE_FLUSH_ALL

#define NO_CYCLE_PENALTY_THR 12

static int cycle_multiplier_active;

static int CLOCK_ADJUST(int x)
{
  int m = cycle_multiplier_active;
  int s = (x >> 31) | 1;
  return (x * m + s * 50) / 100;
}

static int ds_writes_rjump_rs(int i)
{
  return dops[i].rs1 != 0
   && (dops[i].rs1 == dops[i+1].rt1 || dops[i].rs1 == dops[i+1].rt2
    || dops[i].rs1 == dops[i].rt1); // overwrites itself - same effect
}

// psx addr mirror masking (for invalidation)
static u_int pmmask(u_int vaddr)
{
  vaddr &= ~0xe0000000;
  if (vaddr < 0x01000000)
    vaddr &= ~0x00e00000; // RAM mirrors
  return vaddr;
}

static u_int get_page(u_int vaddr)
{
  u_int page = pmmask(vaddr) >> 12;
  if (page >= PAGE_COUNT / 2)
    page = PAGE_COUNT / 2 + (page & (PAGE_COUNT / 2 - 1));
  return page;
}

// get a page for looking for a block that has vaddr
// (needed because the block may start in previous page)
static u_int get_page_prev(u_int vaddr)
{
  assert(MAXBLOCK <= (1 << 12));
  u_int page = get_page(vaddr);
  if (page & 511)
    page--;
  return page;
}

static struct ht_entry *hash_table_get_p(struct ht_entry *ht, u_int vaddr)
{
  return &ht[((vaddr >> 16) ^ vaddr) & 0xFFFF];
}

static struct ht_entry *hash_table_get(u_int vaddr)
{
  return hash_table_get_p(hash_table, vaddr);
}

#define HASH_TABLE_BAD 0xbac

static void hash_table_clear(void)
{
  struct ht_entry *ht_bin;
  int i, j;
  for (i = 0; i < ARRAY_SIZE(hash_table); i++) {
    for (j = 0; j < ARRAY_SIZE(hash_table[i].vaddr); j++) {
      hash_table[i].vaddr[j] = ~0;
      hash_table[i].tcaddr[j] = (void *)(uintptr_t)HASH_TABLE_BAD;
    }
  }
  // don't allow ~0 to hit
  ht_bin = hash_table_get(~0);
  for (j = 0; j < ARRAY_SIZE(ht_bin->vaddr); j++)
    ht_bin->vaddr[j] = 1;
}

static void hash_table_add(u_int vaddr, void *tcaddr)
{
  struct ht_entry *ht_bin = hash_table_get(vaddr);
  assert(tcaddr);
  ht_bin->vaddr[1] = ht_bin->vaddr[0];
  ht_bin->tcaddr[1] = ht_bin->tcaddr[0];
  ht_bin->vaddr[0] = vaddr;
  ht_bin->tcaddr[0] = tcaddr;
}

static void hash_table_remove(int vaddr)
{
  //printf("remove hash: %x\n",vaddr);
  struct ht_entry *ht_bin = hash_table_get(vaddr);
  if (ht_bin->vaddr[1] == vaddr) {
    ht_bin->vaddr[1] = ~0;
    ht_bin->tcaddr[1] = (void *)(uintptr_t)HASH_TABLE_BAD;
  }
  if (ht_bin->vaddr[0] == vaddr) {
    ht_bin->vaddr[0] = ht_bin->vaddr[1];
    ht_bin->tcaddr[0] = ht_bin->tcaddr[1];
    ht_bin->vaddr[1] = ~0;
    ht_bin->tcaddr[1] = (void *)(uintptr_t)HASH_TABLE_BAD;
  }
}

static void mini_ht_clear(void)
{
#ifdef USE_MINI_HT
  int i;
  for (i = 0; i < ARRAY_SIZE(mini_ht) - 1; i++) {
    mini_ht[i][0] = ~0;
    mini_ht[i][1] = HASH_TABLE_BAD;
  }
  mini_ht[i][0] = 1;
  mini_ht[i][1] = HASH_TABLE_BAD;
#endif
}

static void mark_invalid_code(u_int vaddr, u_int len, char invalid)
{
  u_int vaddr_m = vaddr & 0x1fffffff;
  u_int i, j;
  for (i = vaddr_m & ~0xfff; i < vaddr_m + len; i += 0x1000) {
    // ram mirrors, but should not hurt bios
    for (j = 0; j < 0x800000; j += 0x200000) {
      invalid_code[(i|j) >> 12] =
      invalid_code[(i|j|0x80000000u) >> 12] =
      invalid_code[(i|j|0xa0000000u) >> 12] = invalid;
    }
  }
  if (!invalid && vaddr + len > inv_code_start && vaddr <= inv_code_end)
    inv_code_start = inv_code_end = ~0;
}

static int doesnt_expire_soon(u_char *tcaddr)
{
  u_int diff = (u_int)(tcaddr - out) & ((1u << TARGET_SIZE_2) - 1u);
  return diff > EXPIRITY_OFFSET + MAX_OUTPUT_BLOCK_SIZE;
}

static attr_unused void check_for_block_changes(u_int start, u_int end)
{
  u_int start_page = get_page_prev(start);
  u_int end_page = get_page(end - 1);
  u_int page;

  for (page = start_page; page <= end_page; page++) {
    struct block_info *block;
    for (block = blocks[page]; block != NULL; block = block->next) {
      if (block->is_dirty)
        continue;
      if (memcmp(block->source, block->copy, block->len)) {
        printf("bad block %08x-%08x %016llx %016llx @%08x\n",
          block->start, block->start + block->len,
          *(long long *)block->source, *(long long *)block->copy, psxRegs.pc);
        fflush(stdout);
        abort();
      }
    }
  }
}

static void *try_restore_block(u_int vaddr, u_int start_page, u_int end_page)
{
  void *found_clean = NULL;
  u_int i, page;

  stat_inc(stat_restore_tries);
  for (page = start_page; page <= end_page; page++) {
    struct block_info *block;
    for (block = blocks[page]; block != NULL; block = block->next) {
      if (vaddr < block->start)
        break;
      if (!block->is_dirty || vaddr >= block->start + block->len)
        continue;
      for (i = 0; i < block->jump_in_cnt; i++)
        if (block->jump_in[i].vaddr == vaddr)
          break;
      if (i == block->jump_in_cnt)
        continue;
      assert(block->source && block->copy);
      stat_inc(stat_restore_compares);
      if (memcmp(block->source, block->copy, block->len))
        continue;

      block->is_dirty = block->inv_near_misses = 0;
      found_clean = block->jump_in[i].addr;
      hash_table_add(vaddr, found_clean);
      mark_invalid_code(block->start, block->len, 0);
      stat_inc(stat_bc_restore);
      inv_debug("INV: restored %08x %p (%d)\n", vaddr, found_clean, block->jump_in_cnt);
      return found_clean;
    }
  }
  return NULL;
}

// this doesn't normally happen
static noinline u_int generate_exception(u_int pc)
{
  //if (execBreakCheck(&psxRegs, pc))
  //  return psxRegs.pc;

  // generate an address or bus error
  psxRegs.CP0.n.Cause &= 0x300;
  psxRegs.CP0.n.EPC = pc;
  if (pc & 3) {
    psxRegs.CP0.n.Cause |= R3000E_AdEL << 2;
    psxRegs.CP0.n.BadVAddr = pc;
#ifdef DRC_DBG
    last_count -= 2;
#endif
  } else
    psxRegs.CP0.n.Cause |= R3000E_IBE << 2;
  return (psxRegs.pc = 0x80000080);
}

// Get address from virtual address
// This is called from the recompiled JR/JALR instructions
static void noinline *get_addr(struct ht_entry *ht, const u_int vaddr,
  enum ndrc_compile_mode compile_mode)
{
  u_int start_page = get_page_prev(vaddr);
  u_int i, page, end_page = get_page(vaddr);
  void *found_clean = NULL;

  stat_inc(stat_jump_in_lookups);
  for (page = start_page; page <= end_page; page++) {
    const struct block_info *block;
    for (block = blocks[page]; block != NULL; block = block->next) {
      if (vaddr < block->start)
        break;
      if (block->is_dirty || vaddr >= block->start + block->len)
        continue;
      for (i = 0; i < block->jump_in_cnt; i++)
        if (block->jump_in[i].vaddr == vaddr)
          break;
      if (i == block->jump_in_cnt)
        continue;
      found_clean = block->jump_in[i].addr;
      hash_table_add(vaddr, found_clean);
      return found_clean;
    }
  }
  found_clean = try_restore_block(vaddr, start_page, end_page);
  if (found_clean)
    return found_clean;

  if (compile_mode == ndrc_cm_no_compile)
    return NULL;
#ifdef NDRC_THREAD
  if (ndrc_g.thread.handle && compile_mode == ndrc_cm_compile_live) {
    psxRegs.pc = vaddr;
    return new_dyna_leave;
  }
  if (!ndrc_g.thread.handle)
#endif
  memcpy(ndrc_smrv_regs, psxRegs.GPR.r, sizeof(ndrc_smrv_regs));

  int r = new_recompile_block(vaddr);
  if (likely(r == 0))
    return ndrc_get_addr_ht(vaddr, ht);

  if (compile_mode == ndrc_cm_compile_live)
    return ndrc_get_addr_ht(generate_exception(vaddr), ht);

  return NULL;
}

// Look up address in hash table first
void *ndrc_get_addr_ht_param(struct ht_entry *ht, unsigned int vaddr,
  enum ndrc_compile_mode compile_mode)
{
  //check_for_block_changes(vaddr, vaddr + MAXBLOCK);
  const struct ht_entry *ht_bin = hash_table_get_p(ht, vaddr);
  u_int vaddr_a = vaddr & ~3;
  stat_inc(stat_ht_lookups);
  if (ht_bin->vaddr[0] == vaddr_a) return ht_bin->tcaddr[0];
  if (ht_bin->vaddr[1] == vaddr_a) return ht_bin->tcaddr[1];
  return get_addr(ht, vaddr, compile_mode);
}

// "usual" addr lookup for indirect branches, etc
// to be used by currently running code only
void *ndrc_get_addr_ht(u_int vaddr, struct ht_entry *ht)
{
  return ndrc_get_addr_ht_param(ht, vaddr, ndrc_cm_compile_live);
}

static void clear_all_regs(signed char regmap[])
{
  memset(regmap, -1, sizeof(regmap[0]) * HOST_REGS);
}

// get_reg: get allocated host reg from mips reg
// returns -1 if no such mips reg was allocated
#if defined(__arm__) && defined(HAVE_ARMV6) && HOST_REGS == 13 && EXCLUDE_REG == 11

extern signed char get_reg(const signed char regmap[], signed char r);

#else

static signed char get_reg(const signed char regmap[], signed char r)
{
  int hr;
  for (hr = 0; hr < HOST_REGS; hr++) {
    if (hr == EXCLUDE_REG)
      continue;
    if (regmap[hr] == r)
      return hr;
  }
  return -1;
}

#endif

// get reg suitable for writing
static signed char get_reg_w(const signed char regmap[], signed char r)
{
  return r == 0 ? -1 : get_reg(regmap, r);
}

// get reg as mask bit (1 << hr)
static u_int get_regm(const signed char regmap[], signed char r)
{
  return (1u << (get_reg(regmap, r) & 31)) & ~(1u << 31);
}

static signed char get_reg_temp(const signed char regmap[])
{
  int hr;
  for (hr = 0; hr < HOST_REGS; hr++) {
    if (hr == EXCLUDE_REG)
      continue;
    if (regmap[hr] == (signed char)-1)
      return hr;
  }
  return -1;
}

// Find a register that is available for two consecutive cycles
static signed char get_reg2(signed char regmap1[], const signed char regmap2[], int r)
{
  int hr;
  for (hr=0;hr<HOST_REGS;hr++) if(hr!=EXCLUDE_REG&&regmap1[hr]==r&&regmap2[hr]==r) return hr;
  return -1;
}

// reverse reg map: mips -> host
#define RRMAP_SIZE 64
static void make_rregs(const signed char regmap[], signed char rrmap[RRMAP_SIZE],
  u_int *regs_can_change)
{
  u_int r, hr, hr_can_change = 0;
  memset(rrmap, -1, RRMAP_SIZE);
  for (hr = 0; hr < HOST_REGS; )
  {
    r = regmap[hr];
    rrmap[r & (RRMAP_SIZE - 1)] = hr;
    // only add mips $1-$31+$lo, others shifted out
    hr_can_change |= (uint64_t)1 << (hr + ((r - 1) & 32));
    hr++;
    if (hr == EXCLUDE_REG)
      hr++;
  }
  hr_can_change |= 1u << (rrmap[33] & 31);
  hr_can_change |= 1u << (rrmap[CCREG] & 31);
  hr_can_change &= ~(1u << 31);
  *regs_can_change = hr_can_change;
}

// same as get_reg, but takes rrmap
static signed char get_rreg(signed char rrmap[RRMAP_SIZE], signed char r)
{
  assert(0 <= r && r < RRMAP_SIZE);
  return rrmap[r];
}

static int count_free_regs(const signed char regmap[])
{
  int count=0;
  int hr;
  for(hr=0;hr<HOST_REGS;hr++)
  {
    if(hr!=EXCLUDE_REG) {
      if(regmap[hr]<0) count++;
    }
  }
  return count;
}

static void dirty_reg(struct regstat *cur, signed char reg)
{
  int hr;
  if (!reg) return;
  hr = get_reg(cur->regmap, reg);
  if (hr >= 0)
    cur->dirty |= 1<<hr;
}

static void set_const(struct regstat *cur, signed char reg, uint32_t value)
{
  int hr;
  if (!reg) return;
  hr = get_reg(cur->regmap, reg);
  if (hr >= 0) {
    cur->isconst |= 1<<hr;
    current_constmap[hr] = value;
  }
}

static void clear_const(struct regstat *cur, signed char reg)
{
  int hr;
  if (!reg) return;
  hr = get_reg(cur->regmap, reg);
  if (hr >= 0)
    cur->isconst &= ~(1<<hr);
}

static int is_const(const struct regstat *cur, signed char reg)
{
  int hr;
  if (reg < 0) return 0;
  if (!reg) return 1;
  hr = get_reg(cur->regmap, reg);
  if (hr >= 0)
    return (cur->isconst>>hr)&1;
  return 0;
}

static uint32_t get_const(const struct regstat *cur, signed char reg)
{
  int hr;
  if (!reg) return 0;
  hr = get_reg(cur->regmap, reg);
  if (hr >= 0)
    return current_constmap[hr];

  SysPrintf("Unknown constant in r%d\n", reg);
  abort();
}

// Least soon needed registers
// Look at the next ten instructions and see which registers
// will be used.  Try not to reallocate these.
static void lsn(u_char hsn[], int i)
{
  int j;
  int b=-1;
  for(j=0;j<9;j++)
  {
    if(i+j>=slen) {
      j=slen-i-1;
      break;
    }
    if (dops[i+j].is_ujump)
    {
      // Don't go past an unconditonal jump
      j++;
      break;
    }
  }
  for(;j>=0;j--)
  {
    if(dops[i+j].rs1) hsn[dops[i+j].rs1]=j;
    if(dops[i+j].rs2) hsn[dops[i+j].rs2]=j;
    if(dops[i+j].rt1) hsn[dops[i+j].rt1]=j;
    if(dops[i+j].rt2) hsn[dops[i+j].rt2]=j;
    if(dops[i+j].itype==STORE || dops[i+j].itype==STORELR) {
      // Stores can allocate zero
      hsn[dops[i+j].rs1]=j;
      hsn[dops[i+j].rs2]=j;
    }
    if (ram_offset && (dops[i+j].is_load || dops[i+j].is_store))
      hsn[ROREG] = j;
    // On some architectures stores need invc_ptr
    #if defined(HOST_IMM8)
    if (dops[i+j].is_store)
      hsn[INVCP] = j;
    #endif
    if(i+j>=0&&(dops[i+j].itype==UJUMP||dops[i+j].itype==CJUMP||dops[i+j].itype==SJUMP))
    {
      hsn[CCREG]=j;
      b=j;
    }
  }
  if(b>=0)
  {
    if(cinfo[i+b].ba>=start && cinfo[i+b].ba<(start+slen*4))
    {
      // Follow first branch
      int t=(cinfo[i+b].ba-start)>>2;
      j=7-b;if(t+j>=slen) j=slen-t-1;
      for(;j>=0;j--)
      {
        if(dops[t+j].rs1) if(hsn[dops[t+j].rs1]>j+b+2) hsn[dops[t+j].rs1]=j+b+2;
        if(dops[t+j].rs2) if(hsn[dops[t+j].rs2]>j+b+2) hsn[dops[t+j].rs2]=j+b+2;
        //if(dops[t+j].rt1) if(hsn[dops[t+j].rt1]>j+b+2) hsn[dops[t+j].rt1]=j+b+2;
        //if(dops[t+j].rt2) if(hsn[dops[t+j].rt2]>j+b+2) hsn[dops[t+j].rt2]=j+b+2;
      }
    }
    // TODO: preferred register based on backward branch
  }
  // Delay slot should preferably not overwrite branch conditions or cycle count
  if (i > 0 && dops[i-1].is_jump) {
    if(dops[i-1].rs1) if(hsn[dops[i-1].rs1]>1) hsn[dops[i-1].rs1]=1;
    if(dops[i-1].rs2) if(hsn[dops[i-1].rs2]>1) hsn[dops[i-1].rs2]=1;
    hsn[CCREG]=1;
    // ...or hash tables
    hsn[RHASH]=1;
    hsn[RHTBL]=1;
  }
  // Coprocessor load/store needs FTEMP, even if not declared
  if(dops[i].itype==C2LS) {
    hsn[FTEMP]=0;
  }
  // Load/store L/R also uses FTEMP as a temporary register
  if (dops[i].itype == LOADLR || dops[i].itype == STORELR) {
    hsn[FTEMP]=0;
  }
  // Don't remove the miniht registers
  if(dops[i].itype==UJUMP||dops[i].itype==RJUMP)
  {
    hsn[RHASH]=0;
    hsn[RHTBL]=0;
  }
}

// We only want to allocate registers if we're going to use them again soon
static int needed_again(int r, int i)
{
  int j;
  int b=-1;
  int rn=10;

  if (i > 0 && dops[i-1].is_ujump)
  {
    if(cinfo[i-1].ba<start || cinfo[i-1].ba>start+slen*4-4)
      return 0; // Don't need any registers if exiting the block
  }
  for(j=0;j<9;j++)
  {
    if(i+j>=slen) {
      j=slen-i-1;
      break;
    }
    if (dops[i+j].is_ujump)
    {
      // Don't go past an unconditonal jump
      j++;
      break;
    }
    if (dops[i+j].is_exception)
    {
      break;
    }
  }
  for(;j>=1;j--)
  {
    if(dops[i+j].rs1==r) rn=j;
    if(dops[i+j].rs2==r) rn=j;
    if((unneeded_reg[i+j]>>r)&1) rn=10;
    if(i+j>=0&&(dops[i+j].itype==UJUMP||dops[i+j].itype==CJUMP||dops[i+j].itype==SJUMP))
    {
      b=j;
    }
  }
  if(rn<10) return 1;
  (void)b;
  return 0;
}

// Try to match register allocations at the end of a loop with those
// at the beginning
static int loop_reg(int i, int r, int hr)
{
  int j,k;
  for(j=0;j<9;j++)
  {
    if(i+j>=slen) {
      j=slen-i-1;
      break;
    }
    if (dops[i+j].is_ujump)
    {
      // Don't go past an unconditonal jump
      j++;
      break;
    }
  }
  k=0;
  if(i>0){
    if(dops[i-1].itype==UJUMP||dops[i-1].itype==CJUMP||dops[i-1].itype==SJUMP)
      k--;
  }
  for(;k<j;k++)
  {
    assert(r < 64);
    if((unneeded_reg[i+k]>>r)&1) return hr;
    if(i+k>=0&&(dops[i+k].itype==UJUMP||dops[i+k].itype==CJUMP||dops[i+k].itype==SJUMP))
    {
      if(cinfo[i+k].ba>=start && cinfo[i+k].ba<(start+i*4))
      {
        int t=(cinfo[i+k].ba-start)>>2;
        int reg=get_reg(regs[t].regmap_entry,r);
        if(reg>=0) return reg;
        //reg=get_reg(regs[t+1].regmap_entry,r);
        //if(reg>=0) return reg;
      }
    }
  }
  return hr;
}


// Allocate every register, preserving source/target regs
static void alloc_all(struct regstat *cur,int i)
{
  int hr;

  for(hr=0;hr<HOST_REGS;hr++) {
    if(hr!=EXCLUDE_REG) {
      if((cur->regmap[hr]!=dops[i].rs1)&&(cur->regmap[hr]!=dops[i].rs2)&&
         (cur->regmap[hr]!=dops[i].rt1)&&(cur->regmap[hr]!=dops[i].rt2))
      {
        cur->regmap[hr]=-1;
        cur->dirty&=~(1<<hr);
      }
      // Don't need zeros
      if(cur->regmap[hr]==0)
      {
        cur->regmap[hr]=-1;
        cur->dirty&=~(1<<hr);
      }
    }
  }
}

#ifndef NDEBUG
static int host_tempreg_in_use;

static void host_tempreg_acquire(void)
{
  assert(!host_tempreg_in_use);
  host_tempreg_in_use = 1;
}

static void host_tempreg_release(void)
{
  host_tempreg_in_use = 0;
}
#else
static void host_tempreg_acquire(void) {}
static void host_tempreg_release(void) {}
#endif

#ifdef ASSEM_PRINT
extern void gen_interupt();
extern void do_insn_cmp();
#define FUNCNAME(f) { f, " " #f }
static const struct {
  void *addr;
  const char *name;
} function_names[] = {
  FUNCNAME(cc_interrupt),
  FUNCNAME(gen_interupt),
  FUNCNAME(ndrc_get_addr_ht),
  FUNCNAME(ndrc_get_addr_ht_param),
  FUNCNAME(jump_handler_read8),
  FUNCNAME(jump_handler_read16),
  FUNCNAME(jump_handler_read32),
  FUNCNAME(jump_handler_write8),
  FUNCNAME(jump_handler_write16),
  FUNCNAME(jump_handler_write32),
  FUNCNAME(ndrc_write_invalidate_one),
  FUNCNAME(ndrc_write_invalidate_many),
  FUNCNAME(jump_to_new_pc),
  FUNCNAME(jump_break),
  FUNCNAME(jump_break_ds),
  FUNCNAME(jump_syscall),
  FUNCNAME(jump_syscall_ds),
  FUNCNAME(jump_overflow),
  FUNCNAME(jump_overflow_ds),
  FUNCNAME(jump_addrerror),
  FUNCNAME(jump_addrerror_ds),
  FUNCNAME(new_dyna_leave),
  FUNCNAME(pcsx_mtc0),
  FUNCNAME(pcsx_mtc0_ds),
  FUNCNAME(execI),
#ifdef __aarch64__
  FUNCNAME(do_memhandler_pre),
  FUNCNAME(do_memhandler_post),
#endif
#ifdef DRC_DBG
# ifdef __aarch64__
  FUNCNAME(do_insn_cmp_arm64),
# else
  FUNCNAME(do_insn_cmp),
# endif
#endif
};

static const char *func_name(const void *a)
{
  int i;
  for (i = 0; i < sizeof(function_names)/sizeof(function_names[0]); i++)
    if (function_names[i].addr == a)
      return function_names[i].name;
  return "";
}

static const char *fpofs_name(u_int ofs)
{
  u_int *p = (u_int *)&dynarec_local + ofs/sizeof(u_int);
  static char buf[64];
  switch (ofs) {
  #define ofscase(x) case LO_##x: return " ; " #x
  ofscase(next_interupt);
  ofscase(cycle_count);
  ofscase(last_count);
  ofscase(stop);
  ofscase(address);
  ofscase(lo);
  ofscase(hi);
  ofscase(PC);
  ofscase(cycle);
  ofscase(mem_rtab);
  ofscase(mem_wtab);
  ofscase(psxH_ptr);
  ofscase(invc_ptr);
  ofscase(ram_offset);
  ofscase(hash_table_ptr);
  #undef ofscase
  }
  buf[0] = 0;
  if      (psxRegs.GPR.r <= p && p < &psxRegs.GPR.r[32])
    snprintf(buf, sizeof(buf), " ; r%d", (int)(p - psxRegs.GPR.r));
  else if (psxRegs.CP0.r <= p && p < &psxRegs.CP0.r[32])
    snprintf(buf, sizeof(buf), " ; cp0 $%d", (int)(p - psxRegs.CP0.r));
  else if (psxRegs.CP2D.r <= p && p < &psxRegs.CP2D.r[32])
    snprintf(buf, sizeof(buf), " ; cp2d $%d", (int)(p - psxRegs.CP2D.r));
  else if (psxRegs.CP2C.r <= p && p < &psxRegs.CP2C.r[32])
    snprintf(buf, sizeof(buf), " ; cp2c $%d", (int)(p - psxRegs.CP2C.r));
  return buf;
}
#else
#define func_name(x) ""
#define fpofs_name(x) ""
#endif

#ifdef __i386__
#include "assem_x86.c"
#endif
#ifdef __x86_64__
#include "assem_x64.c"
#endif
#ifdef __arm__
#include "assem_arm.c"
#endif
#ifdef __aarch64__
#include "assem_arm64.c"
#endif

static void *get_trampoline(const void *f)
{
  struct ndrc_tramp *tramp = NDRC_WRITE_OFFSET(&ndrc->tramp);
  size_t i;

  for (i = 0; i < ARRAY_SIZE(tramp->f); i++) {
    if (tramp->f[i] == f || tramp->f[i] == NULL)
      break;
  }
  if (i == ARRAY_SIZE(tramp->f)) {
    SysPrintf("trampoline table is full, last func %p\n", f);
    abort();
  }
  if (tramp->f[i] == NULL) {
    start_tcache_write(&tramp->f[i], &tramp->f[i + 1]);
    tramp->f[i] = f;
    end_tcache_write(&tramp->f[i], &tramp->f[i + 1]);
#ifdef HAVE_LIBNX
    // invalidate the RX mirror (unsure if necessary, but just in case...)
    armDCacheFlush(&ndrc->tramp.f[i], sizeof(ndrc->tramp.f[i]));
#endif
  }
  return &ndrc->tramp.ops[i];
}

static void emit_far_jump(const void *f)
{
  if (can_jump_or_call(f)) {
    emit_jmp(f);
    return;
  }

  f = get_trampoline(f);
  emit_jmp(f);
}

static void emit_far_call(const void *f)
{
  if (can_jump_or_call(f)) {
    emit_call(f);
    return;
  }

  f = get_trampoline(f);
  emit_call(f);
}

// Check if an address is already compiled
// but don't return addresses which are about to expire from the cache
static void *check_addr(u_int vaddr)
{
  struct ht_entry *ht_bin = hash_table_get(vaddr);
  size_t i;
  for (i = 0; i < ARRAY_SIZE(ht_bin->vaddr); i++) {
    if (ht_bin->vaddr[i] == vaddr)
      if (doesnt_expire_soon(ht_bin->tcaddr[i]))
        return ht_bin->tcaddr[i];
  }

  // refactor to get_addr_nocompile?
  u_int start_page = get_page_prev(vaddr);
  u_int page, end_page = get_page(vaddr);

  stat_inc(stat_jump_in_lookups);
  for (page = start_page; page <= end_page; page++) {
    const struct block_info *block;
    for (block = blocks[page]; block != NULL; block = block->next) {
      if (vaddr < block->start)
        break;
      if (block->is_dirty || vaddr >= block->start + block->len)
        continue;
      if (!doesnt_expire_soon(ndrc->translation_cache + block->tc_offs))
        continue;
      for (i = 0; i < block->jump_in_cnt; i++)
        if (block->jump_in[i].vaddr == vaddr)
          break;
      if (i == block->jump_in_cnt)
        continue;

      // Update existing entry with current address
      void *addr = block->jump_in[i].addr;
      if (ht_bin->vaddr[0] == vaddr) {
        ht_bin->tcaddr[0] = addr;
        return addr;
      }
      if (ht_bin->vaddr[1] == vaddr) {
        ht_bin->tcaddr[1] = addr;
        return addr;
      }
      // Insert into hash table with low priority.
      // Don't evict existing entries, as they are probably
      // addresses that are being accessed frequently.
      if (ht_bin->vaddr[0] == -1) {
        ht_bin->vaddr[0] = vaddr;
        ht_bin->tcaddr[0] = addr;
      }
      else if (ht_bin->vaddr[1] == -1) {
        ht_bin->vaddr[1] = vaddr;
        ht_bin->tcaddr[1] = addr;
      }
      return addr;
    }
  }
  return NULL;
}

static void blocks_clear(struct block_info **head)
{
  struct block_info *cur, *next;

  if ((cur = *head)) {
    *head = NULL;
    while (cur) {
      next = cur->next;
      free(cur);
      cur = next;
    }
  }
}

static int blocks_remove_matching_addrs(struct block_info **head,
  u_int base_offs, int shift)
{
  struct block_info *next;
  int hit = 0;
  while (*head) {
    if ((((*head)->tc_offs ^ base_offs) >> shift) == 0) {
      inv_debug("EXP: rm block %08x (tc_offs %x)\n", (*head)->start, (*head)->tc_offs);
      invalidate_block(*head);
      next = (*head)->next;
      free(*head);
      *head = next;
      stat_dec(stat_blocks);
      hit = 1;
    }
    else
    {
      head = &((*head)->next);
    }
  }
  return hit;
}

// This is called when we write to a compiled block (see do_invstub)
static void unlink_jumps_vaddr_range(u_int start, u_int end)
{
  u_int page, start_page = get_page(start), end_page = get_page(end - 1);
  int i;

  for (page = start_page; page <= end_page; page++) {
    struct jump_info *ji = jumps[page];
    if (ji == NULL)
      continue;
    for (i = 0; i < ji->count; ) {
      if (ji->e[i].target_vaddr < start || ji->e[i].target_vaddr >= end) {
        i++;
        continue;
      }

      inv_debug("INV: rm link to %08x (tc_offs %zx)\n", ji->e[i].target_vaddr,
        (u_char *)ji->e[i].stub - ndrc->translation_cache);
      void *host_addr = find_extjump_insn(ji->e[i].stub);
      mark_clear_cache(host_addr);
      set_jump_target(host_addr, ji->e[i].stub); // point back to dyna_linker stub

      stat_dec(stat_links);
      ji->count--;
      if (i < ji->count) {
        ji->e[i] = ji->e[ji->count];
        continue;
      }
      i++;
    }
  }
}

static void unlink_jumps_tc_range(struct jump_info *ji, u_int base_offs, int shift)
{
  int i;
  if (ji == NULL)
    return;
  for (i = 0; i < ji->count; ) {
    u_int tc_offs = (u_char *)ji->e[i].stub - ndrc->translation_cache;
    if (((tc_offs ^ base_offs) >> shift) != 0) {
      i++;
      continue;
    }

    inv_debug("EXP: rm link to %08x (tc_offs %x)\n", ji->e[i].target_vaddr, tc_offs);
    stat_dec(stat_links);
    ji->count--;
    if (i < ji->count) {
      ji->e[i] = ji->e[ji->count];
      continue;
    }
    i++;
  }
}

static void invalidate_block(struct block_info *block)
{
  u_int i;

  block->is_dirty = 1;
  unlink_jumps_vaddr_range(block->start, block->start + block->len);
  for (i = 0; i < block->jump_in_cnt; i++)
    hash_table_remove(block->jump_in[i].vaddr);
}

static int invalidate_range(u_int start, u_int end,
  u32 *inv_start_ret, u32 *inv_end_ret)
{
  struct block_info *last_block = NULL;
  u_int start_page = get_page_prev(start);
  u_int end_page = get_page(end - 1);
  u_int start_m = pmmask(start);
  u_int end_m = pmmask(end - 1);
  u_int inv_start, inv_end;
  u_int blk_start_m, blk_end_m;
  u_int page;
  int hit = 0;

  // additional area without code (to supplement invalid_code[]), [start, end)
  // avoids excessive ndrc_write_invalidate*() calls
  inv_start = start_m & ~0xfff;
  inv_end = end_m | 0xfff;

  for (page = start_page; page <= end_page; page++) {
    struct block_info *block;
    for (block = blocks[page]; block != NULL; block = block->next) {
      if (block->is_dirty)
        continue;
      last_block = block;
      blk_end_m = pmmask(block->start + block->len);
      if (blk_end_m <= start_m) {
        inv_start = max(inv_start, blk_end_m);
        continue;
      }
      blk_start_m = pmmask(block->start);
      if (end_m <= blk_start_m) {
        inv_end = min(inv_end, blk_start_m - 1);
        continue;
      }
      if (!block->source) // "hack" block - leave it alone
        continue;

      hit++;
      invalidate_block(block);
      stat_inc(stat_inv_hits);
    }
  }

  if (!hit && last_block && last_block->source) {
    // could be some leftover unused block, uselessly trapping writes
    last_block->inv_near_misses++;
    if (last_block->inv_near_misses > 128) {
      invalidate_block(last_block);
      stat_inc(stat_inv_hits);
      hit++;
    }
  }
  if (hit) {
    do_clear_cache();
    mini_ht_clear();
  }

  if (inv_start <= (start_m & ~0xfff) && inv_end >= (start_m | 0xfff))
    // the whole page is empty now
    mark_invalid_code(start, 1, 1);

  if (inv_start_ret) *inv_start_ret = inv_start | (start & 0xe0000000);
  if (inv_end_ret) *inv_end_ret = inv_end | (end & 0xe0000000);
  return hit;
}

void new_dynarec_invalidate_range(unsigned int start, unsigned int end)
{
  invalidate_range(start, end, NULL, NULL);
}

// check if the range may need invalidation (must be thread-safe)
int new_dynarec_quick_check_range(unsigned int start, unsigned int end)
{
  u_int start_page = get_page_prev(start);
  u_int end_page = get_page(end - 1);
  u_int page;

  if (inv_code_start <= start && end <= inv_code_end)
    return 0;
  for (page = start_page; page <= end_page; page++) {
    if (blocks[page]) {
      //SysPrintf("quick hit %x-%x\n", start, end);
      return 1;
    }
  }
  return 0;
}

static void ndrc_write_invalidate_many(u_int start, u_int end)
{
  // this check is done by the caller
  //if (inv_code_start<=addr&&addr<=inv_code_end) { rhits++; return; }
  int ret = invalidate_range(start, end, &inv_code_start, &inv_code_end);
#ifdef INV_DEBUG_W
  int invc = invalid_code[start >> 12];
  u_int len = end - start;
  if (ret)
    printf("INV ADDR: %08x/%02x hit %d blocks\n", start, len, ret);
  else
    printf("INV ADDR: %08x/%02x miss, inv %08x-%08x invc %d->%d\n", start, len,
      inv_code_start, inv_code_end, invc, invalid_code[start >> 12]);
  check_for_block_changes(start, end);
#endif
  stat_inc(stat_inv_addr_calls);
  (void)ret;
}

void ndrc_write_invalidate_one(u_int addr)
{
  ndrc_write_invalidate_many(addr, addr + 4);
}

// This is called when loading a save state.
// Anything could have changed, so invalidate everything.
void new_dynarec_invalidate_all_pages(void)
{
  struct block_info *block;
  u_int page;
  for (page = 0; page < ARRAY_SIZE(blocks); page++) {
    for (block = blocks[page]; block != NULL; block = block->next) {
      if (block->is_dirty)
        continue;
      if (!block->source) // hack block?
        continue;
      invalidate_block(block);
    }
  }

  do_clear_cache();
  mini_ht_clear();
}

// Add an entry to jump_out after making a link
// stub should point to stub code by emit_extjump()
static void ndrc_add_jump_out(u_int vaddr, void *stub)
{
  inv_debug("ndrc_add_jump_out: %p -> %x\n", stub, vaddr);
  u_int page = get_page(vaddr);
  struct jump_info *ji;

  stat_inc(stat_links);
  check_extjump2(stub);
  ji = jumps[page];
  if (ji == NULL) {
    ji = malloc(sizeof(*ji) + sizeof(ji->e[0]) * 16);
    ji->alloc = 16;
    ji->count = 0;
  }
  else if (ji->count >= ji->alloc) {
    ji->alloc += 16;
    ji = realloc(ji, sizeof(*ji) + sizeof(ji->e[0]) * ji->alloc);
  }
  jumps[page] = ji;
  ji->e[ji->count].target_vaddr = vaddr;
  ji->e[ji->count].stub = stub;
  ji->count++;
}

void ndrc_patch_link(u_int vaddr, void *insn, void *stub, void *target)
{
  void *insn_end = (char *)insn + 4;

  //start_tcache_write(insn, insn_end);
  mprotect_w_x(insn, insn_end, 0);

  assert(target != stub);
  set_jump_target_far1(insn, target);
  ndrc_add_jump_out(vaddr, stub);

#if defined(__aarch64__) || defined(NO_WRITE_EXEC)
  // arm64: no syscall concerns, dyna_linker lacks stale detection
  // w^x: have to do costly permission switching anyway
  new_dyna_clear_cache(NDRC_WRITE_OFFSET(insn), NDRC_WRITE_OFFSET(insn_end));
#endif
  //end_tcache_write(insn, insn_end);
  mprotect_w_x(insn, insn_end, 1);
}

/* Register allocation */

static void alloc_set(struct regstat *cur, int reg, int hr)
{
  cur->regmap[hr] = reg;
  cur->dirty &= ~(1u << hr);
  cur->isconst &= ~(1u << hr);
  cur->noevict |= 1u << hr;
}

static void evict_alloc_reg(struct regstat *cur, int i, int reg, int preferred_hr)
{
  u_char hsn[MAXREG+1];
  int j, r, hr;
  memset(hsn, 10, sizeof(hsn));
  lsn(hsn, i);
  //printf("hsn(%x): %d %d %d %d %d %d %d\n",start+i*4,hsn[cur->regmap[0]&63],hsn[cur->regmap[1]&63],hsn[cur->regmap[2]&63],hsn[cur->regmap[3]&63],hsn[cur->regmap[5]&63],hsn[cur->regmap[6]&63],hsn[cur->regmap[7]&63]);
  if(i>0) {
    // Don't evict the cycle count at entry points, otherwise the entry
    // stub will have to write it.
    if(dops[i].bt&&hsn[CCREG]>2) hsn[CCREG]=2;
    if (i>1 && hsn[CCREG] > 2 && dops[i-2].is_jump) hsn[CCREG]=2;
    for(j=10;j>=3;j--)
    {
      // Alloc preferred register if available
      if (!((cur->noevict >> preferred_hr) & 1)
          && hsn[cur->regmap[preferred_hr]] == j)
      {
        alloc_set(cur, reg, preferred_hr);
        return;
      }
      for(r=1;r<=MAXREG;r++)
      {
        if(hsn[r]==j&&r!=dops[i-1].rs1&&r!=dops[i-1].rs2&&r!=dops[i-1].rt1&&r!=dops[i-1].rt2) {
          for(hr=0;hr<HOST_REGS;hr++) {
            if (hr == EXCLUDE_REG || ((cur->noevict >> hr) & 1))
              continue;
            if(hr!=HOST_CCREG||j<hsn[CCREG]) {
              if(cur->regmap[hr]==r) {
                alloc_set(cur, reg, hr);
                return;
              }
            }
          }
        }
      }
    }
  }
  for(j=10;j>=0;j--)
  {
    for(r=1;r<=MAXREG;r++)
    {
      if(hsn[r]==j) {
        for(hr=0;hr<HOST_REGS;hr++) {
          if (hr == EXCLUDE_REG || ((cur->noevict >> hr) & 1))
            continue;
          if(cur->regmap[hr]==r) {
            alloc_set(cur, reg, hr);
            return;
          }
        }
      }
    }
  }
  SysPrintf("This shouldn't happen (evict_alloc_reg)\n");
  abort();
}

// Note: registers are allocated clean (unmodified state)
// if you intend to modify the register, you must call dirty_reg().
static void alloc_reg(struct regstat *cur,int i,signed char reg)
{
  int r,hr;
  int preferred_reg = PREFERRED_REG_FIRST
    + reg % (PREFERRED_REG_LAST - PREFERRED_REG_FIRST + 1);
  if (reg == CCREG) preferred_reg = HOST_CCREG;
  if (reg == PTEMP || reg == FTEMP) preferred_reg = 12;
  assert(PREFERRED_REG_FIRST != EXCLUDE_REG && EXCLUDE_REG != HOST_REGS);
  assert(reg >= 0);

  // Don't allocate unused registers
  if((cur->u>>reg)&1) return;

  // see if it's already allocated
  if ((hr = get_reg(cur->regmap, reg)) >= 0) {
    cur->noevict |= 1u << hr;
    return;
  }

  // Keep the same mapping if the register was already allocated in a loop
  preferred_reg = loop_reg(i,reg,preferred_reg);

  // Try to allocate the preferred register
  if (cur->regmap[preferred_reg] == -1) {
    alloc_set(cur, reg, preferred_reg);
    return;
  }
  r=cur->regmap[preferred_reg];
  assert(r < 64);
  if((cur->u>>r)&1) {
    alloc_set(cur, reg, preferred_reg);
    return;
  }

  // Clear any unneeded registers
  // We try to keep the mapping consistent, if possible, because it
  // makes branches easier (especially loops).  So we try to allocate
  // first (see above) before removing old mappings.  If this is not
  // possible then go ahead and clear out the registers that are no
  // longer needed.
  for(hr=0;hr<HOST_REGS;hr++)
  {
    r=cur->regmap[hr];
    if(r>=0) {
      assert(r < 64);
      if((cur->u>>r)&1) {cur->regmap[hr]=-1;break;}
    }
  }

  // Try to allocate any available register, but prefer
  // registers that have not been used recently.
  if (i > 0) {
    for (hr = PREFERRED_REG_FIRST; ; ) {
      if (cur->regmap[hr] < 0) {
        int oldreg = regs[i-1].regmap[hr];
        if (oldreg < 0 || (oldreg != dops[i-1].rs1 && oldreg != dops[i-1].rs2
             && oldreg != dops[i-1].rt1 && oldreg != dops[i-1].rt2))
        {
          alloc_set(cur, reg, hr);
          return;
        }
      }
      hr++;
      if (hr == EXCLUDE_REG)
        hr++;
      if (hr == HOST_REGS)
        hr = 0;
      if (hr == PREFERRED_REG_FIRST)
        break;
    }
  }

  // Try to allocate any available register
  for (hr = PREFERRED_REG_FIRST; ; ) {
    if (cur->regmap[hr] < 0) {
      alloc_set(cur, reg, hr);
      return;
    }
    hr++;
    if (hr == EXCLUDE_REG)
      hr++;
    if (hr == HOST_REGS)
      hr = 0;
    if (hr == PREFERRED_REG_FIRST)
      break;
  }

  // Ok, now we have to evict someone
  // Pick a register we hopefully won't need soon
  evict_alloc_reg(cur, i, reg, preferred_reg);
}

// Allocate a temporary register.  This is done without regard to
// dirty status or whether the register we request is on the unneeded list
// Note: This will only allocate one register, even if called multiple times
static void alloc_reg_temp(struct regstat *cur,int i,signed char reg)
{
  int r,hr;

  // see if it's already allocated
  for (hr = 0; hr < HOST_REGS; hr++)
  {
    if (hr != EXCLUDE_REG && cur->regmap[hr] == reg) {
      cur->noevict |= 1u << hr;
      return;
    }
  }

  // Try to allocate any available register
  for(hr=HOST_REGS-1;hr>=0;hr--) {
    if(hr!=EXCLUDE_REG&&cur->regmap[hr]==-1) {
      alloc_set(cur, reg, hr);
      return;
    }
  }

  // Find an unneeded register
  for(hr=HOST_REGS-1;hr>=0;hr--)
  {
    r=cur->regmap[hr];
    if(r>=0) {
      assert(r < 64);
      if((cur->u>>r)&1) {
        if(i==0||((unneeded_reg[i-1]>>r)&1)) {
          alloc_set(cur, reg, hr);
          return;
        }
      }
    }
  }

  // Ok, now we have to evict someone
  // Pick a register we hopefully won't need soon
  evict_alloc_reg(cur, i, reg, 0);
}

static void mov_alloc(struct regstat *current,int i)
{
  if (dops[i].rs1 == HIREG || dops[i].rs1 == LOREG) {
    alloc_cc(current,i); // for stalls
    dirty_reg(current,CCREG);
  }

  // Note: Don't need to actually alloc the source registers
  //alloc_reg(current,i,dops[i].rs1);
  alloc_reg(current,i,dops[i].rt1);

  clear_const(current,dops[i].rs1);
  clear_const(current,dops[i].rt1);
  dirty_reg(current,dops[i].rt1);
}

static void shiftimm_alloc(struct regstat *current,int i)
{
  if(dops[i].opcode2<=0x3) // SLL/SRL/SRA
  {
    if(dops[i].rt1) {
      if(dops[i].rs1&&needed_again(dops[i].rs1,i)) alloc_reg(current,i,dops[i].rs1);
      else dops[i].use_lt1=!!dops[i].rs1;
      alloc_reg(current,i,dops[i].rt1);
      dirty_reg(current,dops[i].rt1);
      if(is_const(current,dops[i].rs1)) {
        int v=get_const(current,dops[i].rs1);
        if(dops[i].opcode2==0x00) set_const(current,dops[i].rt1,v<<cinfo[i].imm);
        if(dops[i].opcode2==0x02) set_const(current,dops[i].rt1,(u_int)v>>cinfo[i].imm);
        if(dops[i].opcode2==0x03) set_const(current,dops[i].rt1,v>>cinfo[i].imm);
      }
      else clear_const(current,dops[i].rt1);
    }
  }
  else
  {
    clear_const(current,dops[i].rs1);
    clear_const(current,dops[i].rt1);
  }

  if(dops[i].opcode2>=0x38&&dops[i].opcode2<=0x3b) // DSLL/DSRL/DSRA
  {
    assert(0);
  }
  if(dops[i].opcode2==0x3c) // DSLL32
  {
    assert(0);
  }
  if(dops[i].opcode2==0x3e) // DSRL32
  {
    assert(0);
  }
  if(dops[i].opcode2==0x3f) // DSRA32
  {
    assert(0);
  }
}

static void shift_alloc(struct regstat *current,int i)
{
  if(dops[i].rt1) {
      if(dops[i].rs1) alloc_reg(current,i,dops[i].rs1);
      if(dops[i].rs2) alloc_reg(current,i,dops[i].rs2);
      alloc_reg(current,i,dops[i].rt1);
      if(dops[i].rt1==dops[i].rs2) {
        alloc_reg_temp(current,i,-1);
        cinfo[i].min_free_regs=1;
      }
    clear_const(current,dops[i].rs1);
    clear_const(current,dops[i].rs2);
    clear_const(current,dops[i].rt1);
    dirty_reg(current,dops[i].rt1);
  }
}

static void alu_alloc(struct regstat *current,int i)
{
  if(dops[i].opcode2>=0x20&&dops[i].opcode2<=0x23) { // ADD/ADDU/SUB/SUBU
    if(dops[i].rt1) {
      if(dops[i].rs1&&dops[i].rs2) {
        alloc_reg(current,i,dops[i].rs1);
        alloc_reg(current,i,dops[i].rs2);
      }
      else {
        if(dops[i].rs1&&needed_again(dops[i].rs1,i)) alloc_reg(current,i,dops[i].rs1);
        if(dops[i].rs2&&needed_again(dops[i].rs2,i)) alloc_reg(current,i,dops[i].rs2);
      }
      alloc_reg(current,i,dops[i].rt1);
    }
    if (dops[i].may_except) {
      alloc_cc_optional(current, i); // for exceptions
      alloc_reg_temp(current, i, -1);
      cinfo[i].min_free_regs = 1;
    }
  }
  else if(dops[i].opcode2==0x2a||dops[i].opcode2==0x2b) { // SLT/SLTU
    if(dops[i].rt1) {
      alloc_reg(current,i,dops[i].rs1);
      alloc_reg(current,i,dops[i].rs2);
      alloc_reg(current,i,dops[i].rt1);
    }
  }
  else if(dops[i].opcode2>=0x24&&dops[i].opcode2<=0x27) { // AND/OR/XOR/NOR
    if(dops[i].rt1) {
      if(dops[i].rs1&&dops[i].rs2) {
        alloc_reg(current,i,dops[i].rs1);
        alloc_reg(current,i,dops[i].rs2);
      }
      else
      {
        if(dops[i].rs1&&needed_again(dops[i].rs1,i)) alloc_reg(current,i,dops[i].rs1);
        if(dops[i].rs2&&needed_again(dops[i].rs2,i)) alloc_reg(current,i,dops[i].rs2);
      }
      alloc_reg(current,i,dops[i].rt1);
    }
  }
  clear_const(current,dops[i].rs1);
  clear_const(current,dops[i].rs2);
  clear_const(current,dops[i].rt1);
  dirty_reg(current,dops[i].rt1);
}

static void imm16_alloc(struct regstat *current,int i)
{
  if(dops[i].rs1&&needed_again(dops[i].rs1,i)) alloc_reg(current,i,dops[i].rs1);
  else dops[i].use_lt1=!!dops[i].rs1;
  if(dops[i].rt1) alloc_reg(current,i,dops[i].rt1);
  if(dops[i].opcode==0x0a||dops[i].opcode==0x0b) { // SLTI/SLTIU
    clear_const(current,dops[i].rs1);
    clear_const(current,dops[i].rt1);
  }
  else if(dops[i].opcode>=0x0c&&dops[i].opcode<=0x0e) { // ANDI/ORI/XORI
    if(is_const(current,dops[i].rs1)) {
      int v=get_const(current,dops[i].rs1);
      if(dops[i].opcode==0x0c) set_const(current,dops[i].rt1,v&cinfo[i].imm);
      if(dops[i].opcode==0x0d) set_const(current,dops[i].rt1,v|cinfo[i].imm);
      if(dops[i].opcode==0x0e) set_const(current,dops[i].rt1,v^cinfo[i].imm);
    }
    else clear_const(current,dops[i].rt1);
  }
  else if(dops[i].opcode==0x08||dops[i].opcode==0x09) { // ADDI/ADDIU
    if(is_const(current,dops[i].rs1)) {
      int v=get_const(current,dops[i].rs1);
      set_const(current,dops[i].rt1,v+cinfo[i].imm);
    }
    else clear_const(current,dops[i].rt1);
    if (dops[i].may_except) {
      alloc_cc_optional(current, i); // for exceptions
      alloc_reg_temp(current, i, -1);
      cinfo[i].min_free_regs = 1;
    }
  }
  else {
    set_const(current,dops[i].rt1,cinfo[i].imm<<16); // LUI
  }
  dirty_reg(current,dops[i].rt1);
}

static void load_alloc(struct regstat *current,int i)
{
  int need_temp = 0;
  clear_const(current,dops[i].rt1);
  //if(dops[i].rs1!=dops[i].rt1&&needed_again(dops[i].rs1,i)) clear_const(current,dops[i].rs1); // Does this help or hurt?
  if(!dops[i].rs1) current->u&=~1LL; // Allow allocating r0 if it's the source register
  if (needed_again(dops[i].rs1, i))
    alloc_reg(current, i, dops[i].rs1);
  if (ram_offset)
    alloc_reg(current, i, ROREG);
  if (dops[i].may_except) {
    alloc_cc_optional(current, i); // for exceptions
    need_temp = 1;
  }
  if(dops[i].rt1&&!((current->u>>dops[i].rt1)&1)) {
    alloc_reg(current,i,dops[i].rt1);
    assert(get_reg_w(current->regmap, dops[i].rt1)>=0);
    dirty_reg(current,dops[i].rt1);
    // LWL/LWR need a temporary register for the old value
    if(dops[i].opcode==0x22||dops[i].opcode==0x26)
    {
      alloc_reg(current,i,FTEMP);
      need_temp = 1;
    }
  }
  else
  {
    // Load to r0 or unneeded register (dummy load)
    // but we still need a register to calculate the address
    if(dops[i].opcode==0x22||dops[i].opcode==0x26)
      alloc_reg(current,i,FTEMP); // LWL/LWR need another temporary
    need_temp = 1;
  }
  if (need_temp) {
    alloc_reg_temp(current, i, -1);
    cinfo[i].min_free_regs = 1;
  }
}

// this may eat up to 7 registers
static void store_alloc(struct regstat *current, int i)
{
  clear_const(current,dops[i].rs2);
  if(!(dops[i].rs2)) current->u&=~1LL; // Allow allocating r0 if necessary
  if(needed_again(dops[i].rs1,i)) alloc_reg(current,i,dops[i].rs1);
  alloc_reg(current,i,dops[i].rs2);
  if (ram_offset)
    alloc_reg(current, i, ROREG);
  #if defined(HOST_IMM8)
  // On CPUs without 32-bit immediates we need a pointer to invalid_code
  alloc_reg(current, i, INVCP);
  #endif
  if (dops[i].opcode == 0x2a || dops[i].opcode == 0x2e) { // SWL/SWL
    alloc_reg(current,i,FTEMP);
  }
  if (dops[i].may_except)
    alloc_cc_optional(current, i); // for exceptions
  // We need a temporary register for address generation
  alloc_reg_temp(current,i,-1);
  cinfo[i].min_free_regs=1;
}

static void c2ls_alloc(struct regstat *current, int i)
{
  clear_const(current,dops[i].rt1);
  if(needed_again(dops[i].rs1,i)) alloc_reg(current,i,dops[i].rs1);
  alloc_reg(current,i,FTEMP);
  if (ram_offset)
    alloc_reg(current, i, ROREG);
  #if defined(HOST_IMM8)
  // On CPUs without 32-bit immediates we need a pointer to invalid_code
  if (dops[i].opcode == 0x3a) // SWC2
    alloc_reg(current,i,INVCP);
  #endif
  if (dops[i].may_except)
    alloc_cc_optional(current, i); // for exceptions
  // We need a temporary register for address generation
  alloc_reg_temp(current,i,-1);
  cinfo[i].min_free_regs=1;
}

#ifndef multdiv_alloc
static void multdiv_alloc(struct regstat *current,int i)
{
  //  case 0x18: MULT
  //  case 0x19: MULTU
  //  case 0x1A: DIV
  //  case 0x1B: DIVU
  clear_const(current,dops[i].rs1);
  clear_const(current,dops[i].rs2);
  alloc_cc(current,i); // for stalls
  dirty_reg(current,CCREG);
  current->u &= ~(1ull << HIREG);
  current->u &= ~(1ull << LOREG);
  alloc_reg(current, i, HIREG);
  alloc_reg(current, i, LOREG);
  dirty_reg(current, HIREG);
  dirty_reg(current, LOREG);
  if ((dops[i].opcode2 & 0x3e) == 0x1a || (dops[i].rs1 && dops[i].rs2)) // div(u)
  {
    alloc_reg(current, i, dops[i].rs1);
    alloc_reg(current, i, dops[i].rs2);
  }
  // else multiply by zero is zero
}
#endif

static void cop0_alloc(struct regstat *current,int i)
{
  if(dops[i].opcode2==0) // MFC0
  {
    if(dops[i].rt1) {
      clear_const(current,dops[i].rt1);
      alloc_reg(current,i,dops[i].rt1);
      dirty_reg(current,dops[i].rt1);
    }
  }
  else if(dops[i].opcode2==4) // MTC0
  {
    if (((source[i]>>11)&0x1e) == 12) {
      alloc_cc(current, i);
      dirty_reg(current, CCREG);
    }
    if(dops[i].rs1){
      clear_const(current,dops[i].rs1);
      alloc_reg(current,i,dops[i].rs1);
      alloc_all(current,i);
    }
    else {
      alloc_all(current,i); // FIXME: Keep r0
      current->u&=~1LL;
      alloc_reg(current,i,0);
    }
    cinfo[i].min_free_regs = HOST_REGS;
  }
}

static void rfe_alloc(struct regstat *current, int i)
{
  alloc_all(current, i);
  cinfo[i].min_free_regs = HOST_REGS;
}

static void cop2_alloc(struct regstat *current,int i)
{
  if (dops[i].opcode2 < 3) // MFC2/CFC2
  {
    alloc_cc(current,i); // for stalls
    dirty_reg(current,CCREG);
    if(dops[i].rt1){
      clear_const(current,dops[i].rt1);
      alloc_reg(current,i,dops[i].rt1);
      dirty_reg(current,dops[i].rt1);
    }
  }
  else if (dops[i].opcode2 > 3) // MTC2/CTC2
  {
    if(dops[i].rs1){
      clear_const(current,dops[i].rs1);
      alloc_reg(current,i,dops[i].rs1);
    }
    else {
      current->u&=~1LL;
      alloc_reg(current,i,0);
    }
  }
  alloc_reg_temp(current,i,-1);
  cinfo[i].min_free_regs=1;
}

static void c2op_alloc(struct regstat *current,int i)
{
  alloc_cc(current,i); // for stalls
  dirty_reg(current,CCREG);
  alloc_reg_temp(current,i,-1);
}

static void syscall_alloc(struct regstat *current,int i)
{
  alloc_cc(current,i);
  dirty_reg(current,CCREG);
  alloc_all(current,i);
  cinfo[i].min_free_regs=HOST_REGS;
  current->isconst=0;
}

static void delayslot_alloc(struct regstat *current,int i)
{
  switch(dops[i].itype) {
    case UJUMP:
    case CJUMP:
    case SJUMP:
    case RJUMP:
    case SYSCALL:
    case HLECALL:
    case IMM16:
      imm16_alloc(current,i);
      break;
    case LOAD:
    case LOADLR:
      load_alloc(current,i);
      break;
    case STORE:
    case STORELR:
      store_alloc(current,i);
      break;
    case ALU:
      alu_alloc(current,i);
      break;
    case SHIFT:
      shift_alloc(current,i);
      break;
    case MULTDIV:
      multdiv_alloc(current,i);
      break;
    case SHIFTIMM:
      shiftimm_alloc(current,i);
      break;
    case MOV:
      mov_alloc(current,i);
      break;
    case COP0:
      cop0_alloc(current,i);
      break;
    case RFE:
      rfe_alloc(current,i);
      break;
    case COP2:
      cop2_alloc(current,i);
      break;
    case C2LS:
      c2ls_alloc(current,i);
      break;
    case C2OP:
      c2op_alloc(current,i);
      break;
  }
}

static void add_stub(enum stub_type type, void *addr, void *retaddr,
  u_int a, uintptr_t b, uintptr_t c, u_int d, u_int e)
{
  assert(stubcount < ARRAY_SIZE(stubs));
  stubs[stubcount].type = type;
  stubs[stubcount].addr = addr;
  stubs[stubcount].retaddr = retaddr;
  stubs[stubcount].a = a;
  stubs[stubcount].b = b;
  stubs[stubcount].c = c;
  stubs[stubcount].d = d;
  stubs[stubcount].e = e;
  stubcount++;
}

static void add_stub_r(enum stub_type type, void *addr, void *retaddr,
  int i, int addr_reg, const struct regstat *i_regs, int ccadj, u_int reglist)
{
  add_stub(type, addr, retaddr, i, addr_reg, (uintptr_t)i_regs, ccadj, reglist);
}

// Write out a single register
static void wb_register(signed char r, const signed char regmap[], u_int dirty)
{
  int hr;
  for(hr=0;hr<HOST_REGS;hr++) {
    if(hr!=EXCLUDE_REG) {
      if(regmap[hr]==r) {
        if((dirty>>hr)&1) {
          assert(regmap[hr]<64);
          emit_storereg(r,hr);
        }
        break;
      }
    }
  }
}

static void wb_valid(signed char pre[],signed char entry[],u_int dirty_pre,u_int dirty,uint64_t u)
{
  //if(dirty_pre==dirty) return;
  int hr, r;
  for (hr = 0; hr < HOST_REGS; hr++) {
    r = pre[hr];
    if (r < 1 || r > 33 || ((u >> r) & 1))
      continue;
    if (((dirty_pre & ~dirty) >> hr) & 1)
      emit_storereg(r, hr);
  }
}

// trashes r2
static void pass_args(int a0, int a1)
{
  if(a0==1&&a1==0) {
    // must swap
    emit_mov(a0,2); emit_mov(a1,1); emit_mov(2,0);
  }
  else if(a0!=0&&a1==0) {
    emit_mov(a1,1);
    if (a0>=0) emit_mov(a0,0);
  }
  else {
    if(a0>=0&&a0!=0) emit_mov(a0,0);
    if(a1>=0&&a1!=1) emit_mov(a1,1);
  }
}

static void alu_assemble(int i, const struct regstat *i_regs, int ccadj_)
{
  if(dops[i].opcode2>=0x20&&dops[i].opcode2<=0x23) { // ADD/ADDU/SUB/SUBU
    int do_oflow = dops[i].may_except; // ADD/SUB with exceptions enabled
    if (dops[i].rt1 || do_oflow) {
      int do_exception_check = 0;
      signed char s1, s2, t, tmp;
      t = get_reg_w(i_regs->regmap, dops[i].rt1);
      tmp = get_reg_temp(i_regs->regmap);
      if (do_oflow)
        assert(tmp >= 0);
      if (t < 0 && do_oflow)
        t = tmp;
      if (t >= 0) {
        s1 = get_reg(i_regs->regmap, dops[i].rs1);
        s2 = get_reg(i_regs->regmap, dops[i].rs2);
        if (dops[i].rs1 && dops[i].rs2) {
          assert(s1>=0);
          assert(s2>=0);
          if (dops[i].opcode2 & 2) {
            if (do_oflow) {
              emit_subs(s1, s2, tmp);
              do_exception_check = 1;
            }
            else
              emit_sub(s1,s2,t);
          }
          else {
            if (do_oflow) {
              emit_adds(s1, s2, tmp);
              do_exception_check = 1;
            }
            else
              emit_add(s1,s2,t);
          }
        }
        else if(dops[i].rs1) {
          if(s1>=0) emit_mov(s1,t);
          else emit_loadreg(dops[i].rs1,t);
        }
        else if(dops[i].rs2) {
          if (s2 < 0) {
            emit_loadreg(dops[i].rs2, t);
            s2 = t;
          }
          if (dops[i].opcode2 & 2) {
            if (do_oflow) {
              emit_negs(s2, tmp);
              do_exception_check = 1;
            }
            else
              emit_neg(s2, t);
          }
          else if (s2 != t)
            emit_mov(s2, t);
        }
        else
          emit_zeroreg(t);
      }
      if (do_exception_check) {
        void *jaddr = out;
        emit_jo(0);
        if (t >= 0 && tmp != t)
          emit_mov(tmp, t);
        add_stub_r(OVERFLOW_STUB, jaddr, out, i, 0, i_regs, ccadj_, 0);
      }
    }
  }
  else if(dops[i].opcode2==0x2a||dops[i].opcode2==0x2b) { // SLT/SLTU
    if(dops[i].rt1) {
      signed char s1l,s2l,t;
      {
        t=get_reg_w(i_regs->regmap, dops[i].rt1);
        //assert(t>=0);
        if(t>=0) {
          s1l=get_reg(i_regs->regmap,dops[i].rs1);
          s2l=get_reg(i_regs->regmap,dops[i].rs2);
          if(dops[i].rs2==0) // rx<r0
          {
            if(dops[i].opcode2==0x2a&&dops[i].rs1!=0) { // SLT
              assert(s1l>=0);
              emit_shrimm(s1l,31,t);
            }
            else // SLTU (unsigned can not be less than zero, 0<0)
              emit_zeroreg(t);
          }
          else if(dops[i].rs1==0) // r0<rx
          {
            assert(s2l>=0);
            if(dops[i].opcode2==0x2a) // SLT
              emit_set_gz32(s2l,t);
            else // SLTU (set if not zero)
              emit_set_nz32(s2l,t);
          }
          else{
            assert(s1l>=0);assert(s2l>=0);
            if(dops[i].opcode2==0x2a) // SLT
              emit_set_if_less32(s1l,s2l,t);
            else // SLTU
              emit_set_if_carry32(s1l,s2l,t);
          }
        }
      }
    }
  }
  else if(dops[i].opcode2>=0x24&&dops[i].opcode2<=0x27) { // AND/OR/XOR/NOR
    if(dops[i].rt1) {
      signed char s1l,s2l,tl;
      tl=get_reg_w(i_regs->regmap, dops[i].rt1);
      {
        if(tl>=0) {
          s1l=get_reg(i_regs->regmap,dops[i].rs1);
          s2l=get_reg(i_regs->regmap,dops[i].rs2);
          if(dops[i].rs1&&dops[i].rs2) {
            assert(s1l>=0);
            assert(s2l>=0);
            if(dops[i].opcode2==0x24) { // AND
              emit_and(s1l,s2l,tl);
            } else
            if(dops[i].opcode2==0x25) { // OR
              emit_or(s1l,s2l,tl);
            } else
            if(dops[i].opcode2==0x26) { // XOR
              emit_xor(s1l,s2l,tl);
            } else
            if(dops[i].opcode2==0x27) { // NOR
              emit_or(s1l,s2l,tl);
              emit_not(tl,tl);
            }
          }
          else
          {
            if(dops[i].opcode2==0x24) { // AND
              emit_zeroreg(tl);
            } else
            if(dops[i].opcode2==0x25||dops[i].opcode2==0x26) { // OR/XOR
              if(dops[i].rs1){
                if(s1l>=0) emit_mov(s1l,tl);
                else emit_loadreg(dops[i].rs1,tl); // CHECK: regmap_entry?
              }
              else
              if(dops[i].rs2){
                if(s2l>=0) emit_mov(s2l,tl);
                else emit_loadreg(dops[i].rs2,tl); // CHECK: regmap_entry?
              }
              else emit_zeroreg(tl);
            } else
            if(dops[i].opcode2==0x27) { // NOR
              if(dops[i].rs1){
                if(s1l>=0) emit_not(s1l,tl);
                else {
                  emit_loadreg(dops[i].rs1,tl);
                  emit_not(tl,tl);
                }
              }
              else
              if(dops[i].rs2){
                if(s2l>=0) emit_not(s2l,tl);
                else {
                  emit_loadreg(dops[i].rs2,tl);
                  emit_not(tl,tl);
                }
              }
              else emit_movimm(-1,tl);
            }
          }
        }
      }
    }
  }
}

static void imm16_assemble(int i, const struct regstat *i_regs, int ccadj_)
{
  if (dops[i].opcode==0x0f) { // LUI
    if(dops[i].rt1) {
      signed char t;
      t=get_reg_w(i_regs->regmap, dops[i].rt1);
      //assert(t>=0);
      if(t>=0) {
        if(!((i_regs->isconst>>t)&1))
          emit_movimm(cinfo[i].imm<<16,t);
      }
    }
  }
  if(dops[i].opcode==0x08||dops[i].opcode==0x09) { // ADDI/ADDIU
    int is_addi = dops[i].may_except;
    if (dops[i].rt1 || is_addi) {
      signed char s, t, tmp;
      t=get_reg_w(i_regs->regmap, dops[i].rt1);
      s=get_reg(i_regs->regmap,dops[i].rs1);
      if(dops[i].rs1) {
        tmp = get_reg_temp(i_regs->regmap);
        if (is_addi) {
          assert(tmp >= 0);
          if (t < 0) t = tmp;
        }
        if(t>=0) {
          if(!((i_regs->isconst>>t)&1)) {
            int sum, do_exception_check = 0;
            if (s < 0) {
              if(i_regs->regmap_entry[t]!=dops[i].rs1) emit_loadreg(dops[i].rs1,t);
              if (is_addi) {
                emit_addimm_and_set_flags3(t, cinfo[i].imm, tmp);
                do_exception_check = 1;
              }
              else
                emit_addimm(t, cinfo[i].imm, t);
            } else {
              if (!((i_regs->wasconst >> s) & 1)) {
                if (is_addi) {
                  emit_addimm_and_set_flags3(s, cinfo[i].imm, tmp);
                  do_exception_check = 1;
                }
                else
                  emit_addimm(s, cinfo[i].imm, t);
              }
              else {
                int oflow = add_overflow(constmap[i][s], cinfo[i].imm, sum);
                if (is_addi && oflow)
                  do_exception_check = 2;
                else
                  emit_movimm(sum, t);
              }
            }
            if (do_exception_check) {
              void *jaddr = out;
              if (do_exception_check == 2)
                emit_jmp(0);
              else {
                emit_jo(0);
                if (tmp != t)
                  emit_mov(tmp, t);
              }
              add_stub_r(OVERFLOW_STUB, jaddr, out, i, 0, i_regs, ccadj_, 0);
            }
          }
        }
      } else {
        if(t>=0) {
          if(!((i_regs->isconst>>t)&1))
            emit_movimm(cinfo[i].imm,t);
        }
      }
    }
  }
  else if(dops[i].opcode==0x0a||dops[i].opcode==0x0b) { // SLTI/SLTIU
    if(dops[i].rt1) {
      //assert(dops[i].rs1!=0); // r0 might be valid, but it's probably a bug
      signed char sl,t;
      t=get_reg_w(i_regs->regmap, dops[i].rt1);
      sl=get_reg(i_regs->regmap,dops[i].rs1);
      //assert(t>=0);
      if(t>=0) {
        if(dops[i].rs1>0) {
            if(dops[i].opcode==0x0a) { // SLTI
              if(sl<0) {
                if(i_regs->regmap_entry[t]!=dops[i].rs1) emit_loadreg(dops[i].rs1,t);
                emit_slti32(t,cinfo[i].imm,t);
              }else{
                emit_slti32(sl,cinfo[i].imm,t);
              }
            }
            else { // SLTIU
              if(sl<0) {
                if(i_regs->regmap_entry[t]!=dops[i].rs1) emit_loadreg(dops[i].rs1,t);
                emit_sltiu32(t,cinfo[i].imm,t);
              }else{
                emit_sltiu32(sl,cinfo[i].imm,t);
              }
            }
        }else{
          // SLTI(U) with r0 is just stupid,
          // nonetheless examples can be found
          if(dops[i].opcode==0x0a) // SLTI
            if(0<cinfo[i].imm) emit_movimm(1,t);
            else emit_zeroreg(t);
          else // SLTIU
          {
            if(cinfo[i].imm) emit_movimm(1,t);
            else emit_zeroreg(t);
          }
        }
      }
    }
  }
  else if(dops[i].opcode>=0x0c&&dops[i].opcode<=0x0e) { // ANDI/ORI/XORI
    if(dops[i].rt1) {
      signed char sl,tl;
      tl=get_reg_w(i_regs->regmap, dops[i].rt1);
      sl=get_reg(i_regs->regmap,dops[i].rs1);
      if(tl>=0 && !((i_regs->isconst>>tl)&1)) {
        if(dops[i].opcode==0x0c) //ANDI
        {
          if(dops[i].rs1) {
            if(sl<0) {
              if(i_regs->regmap_entry[tl]!=dops[i].rs1) emit_loadreg(dops[i].rs1,tl);
              emit_andimm(tl,cinfo[i].imm,tl);
            }else{
              if(!((i_regs->wasconst>>sl)&1))
                emit_andimm(sl,cinfo[i].imm,tl);
              else
                emit_movimm(constmap[i][sl]&cinfo[i].imm,tl);
            }
          }
          else
            emit_zeroreg(tl);
        }
        else
        {
          if(dops[i].rs1) {
            if(sl<0) {
              if(i_regs->regmap_entry[tl]!=dops[i].rs1) emit_loadreg(dops[i].rs1,tl);
            }
            if(dops[i].opcode==0x0d) { // ORI
              if(sl<0) {
                emit_orimm(tl,cinfo[i].imm,tl);
              }else{
                if(!((i_regs->wasconst>>sl)&1))
                  emit_orimm(sl,cinfo[i].imm,tl);
                else
                  emit_movimm(constmap[i][sl]|cinfo[i].imm,tl);
              }
            }
            if(dops[i].opcode==0x0e) { // XORI
              if(sl<0) {
                emit_xorimm(tl,cinfo[i].imm,tl);
              }else{
                if(!((i_regs->wasconst>>sl)&1))
                  emit_xorimm(sl,cinfo[i].imm,tl);
                else
                  emit_movimm(constmap[i][sl]^cinfo[i].imm,tl);
              }
            }
          }
          else {
            emit_movimm(cinfo[i].imm,tl);
          }
        }
      }
    }
  }
}

static void shiftimm_assemble(int i, const struct regstat *i_regs)
{
  if(dops[i].opcode2<=0x3) // SLL/SRL/SRA
  {
    if(dops[i].rt1) {
      signed char s,t;
      t=get_reg_w(i_regs->regmap, dops[i].rt1);
      s=get_reg(i_regs->regmap,dops[i].rs1);
      //assert(t>=0);
      if(t>=0&&!((i_regs->isconst>>t)&1)){
        if(dops[i].rs1==0)
        {
          emit_zeroreg(t);
        }
        else
        {
          if(s<0&&i_regs->regmap_entry[t]!=dops[i].rs1) emit_loadreg(dops[i].rs1,t);
          if(cinfo[i].imm) {
            if(dops[i].opcode2==0) // SLL
            {
              emit_shlimm(s<0?t:s,cinfo[i].imm,t);
            }
            if(dops[i].opcode2==2) // SRL
            {
              emit_shrimm(s<0?t:s,cinfo[i].imm,t);
            }
            if(dops[i].opcode2==3) // SRA
            {
              emit_sarimm(s<0?t:s,cinfo[i].imm,t);
            }
          }else{
            // Shift by zero
            if(s>=0 && s!=t) emit_mov(s,t);
          }
        }
      }
      //emit_storereg(dops[i].rt1,t); //DEBUG
    }
  }
  if(dops[i].opcode2>=0x38&&dops[i].opcode2<=0x3b) // DSLL/DSRL/DSRA
  {
    assert(0);
  }
  if(dops[i].opcode2==0x3c) // DSLL32
  {
    assert(0);
  }
  if(dops[i].opcode2==0x3e) // DSRL32
  {
    assert(0);
  }
  if(dops[i].opcode2==0x3f) // DSRA32
  {
    assert(0);
  }
}

#ifndef shift_assemble
static void shift_assemble(int i, const struct regstat *i_regs)
{
  signed char s,t,shift;
  if (dops[i].rt1 == 0)
    return;
  assert(dops[i].opcode2<=0x07); // SLLV/SRLV/SRAV
  t = get_reg(i_regs->regmap, dops[i].rt1);
  s = get_reg(i_regs->regmap, dops[i].rs1);
  shift = get_reg(i_regs->regmap, dops[i].rs2);
  if (t < 0)
    return;

  if(dops[i].rs1==0)
    emit_zeroreg(t);
  else if(dops[i].rs2==0) {
    assert(s>=0);
    if(s!=t) emit_mov(s,t);
  }
  else {
    host_tempreg_acquire();
    emit_andimm(shift,31,HOST_TEMPREG);
    switch(dops[i].opcode2) {
    case 4: // SLLV
      emit_shl(s,HOST_TEMPREG,t);
      break;
    case 6: // SRLV
      emit_shr(s,HOST_TEMPREG,t);
      break;
    case 7: // SRAV
      emit_sar(s,HOST_TEMPREG,t);
      break;
    default:
      assert(0);
    }
    host_tempreg_release();
  }
}

#endif

enum {
  MTYPE_8000 = 0,
  MTYPE_8020,
  MTYPE_0000,
  MTYPE_A000,
  MTYPE_1F80,
};

static int get_ptr_mem_type(u_int a)
{
  if(a < 0x00200000) {
    if(a<0x1000&&((start>>20)==0xbfc||(start>>24)==0xa0))
      // return wrong, must use memhandler for BIOS self-test to pass
      // 007 does similar stuff from a00 mirror, weird stuff
      return MTYPE_8000;
    return MTYPE_0000;
  }
  if(0x1f800000 <= a && a < 0x1f801000)
    return MTYPE_1F80;
  if(0x80200000 <= a && a < 0x80800000)
    return MTYPE_8020;
  if(0xa0000000 <= a && a < 0xa0200000)
    return MTYPE_A000;
  return MTYPE_8000;
}

static int get_ro_reg(const struct regstat *i_regs, int host_tempreg_free)
{
  int r = get_reg(i_regs->regmap, ROREG);
  if (r < 0 && host_tempreg_free) {
    host_tempreg_acquire();
    emit_loadreg(ROREG, r = HOST_TEMPREG);
  }
  if (r < 0)
    abort();
  return r;
}

static void *emit_fastpath_cmp_jump(int i, const struct regstat *i_regs,
  int addr, int *offset_reg, int *addr_reg_override, int ccadj_)
{
  void *jaddr = NULL;
  int type = 0;
  int mr = dops[i].rs1;
  assert(addr >= 0);
  *offset_reg = -1;
  if(((smrv_strong|smrv_weak)>>mr)&1) {
    type=get_ptr_mem_type(ndrc_smrv_regs[mr]);
    //printf("set %08x @%08x r%d %d\n", ndrc_smrv_regs[mr], start+i*4, mr, type);
  }
  else {
    // use the mirror we are running on
    type=get_ptr_mem_type(start);
    //printf("set nospec   @%08x r%d %d\n", start+i*4, mr, type);
  }

  if (dops[i].may_except) {
    // alignment check
    u_int op = dops[i].opcode;
    int mask = ((op & 0x37) == 0x21 || op == 0x25) ? 1 : 3; // LH/SH/LHU
    void *jaddr2;
    emit_testimm(addr, mask);
    jaddr2 = out;
    emit_jne(0);
    add_stub_r(ALIGNMENT_STUB, jaddr2, out, i, addr, i_regs, ccadj_, 0);
  }

  if(type==MTYPE_8020) { // RAM 80200000+ mirror
    host_tempreg_acquire();
    emit_andimm(addr,~0x00e00000,HOST_TEMPREG);
    addr=*addr_reg_override=HOST_TEMPREG;
    type=0;
  }
  else if(type==MTYPE_0000) { // RAM 0 mirror
    host_tempreg_acquire();
    emit_orimm(addr,0x80000000,HOST_TEMPREG);
    addr=*addr_reg_override=HOST_TEMPREG;
    type=0;
  }
  else if(type==MTYPE_A000) { // RAM A mirror
    host_tempreg_acquire();
    emit_andimm(addr,~0x20000000,HOST_TEMPREG);
    addr=*addr_reg_override=HOST_TEMPREG;
    type=0;
  }
  else if(type==MTYPE_1F80) { // scratchpad
    if (psxH == (void *)0x1f800000) {
      host_tempreg_acquire();
      emit_xorimm(addr,0x1f800000,HOST_TEMPREG);
      emit_cmpimm(HOST_TEMPREG,0x1000);
      host_tempreg_release();
      jaddr=out;
      emit_jc(0);
    }
    else {
      // do the usual RAM check, jump will go to the right handler
      type=0;
    }
  }

  if (type == 0) // need ram check
  {
    emit_cmpimm(addr,RAM_SIZE);
    jaddr = out;
    #ifdef CORTEX_A8_BRANCH_PREDICTION_HACK
    // Hint to branch predictor that the branch is unlikely to be taken
    if (dops[i].rs1 >= 28)
      emit_jno_unlikely(0);
    else
    #endif
      emit_jno(0);
    if (ram_offset != 0)
      *offset_reg = get_ro_reg(i_regs, 0);
  }

  return jaddr;
}

// return memhandler, or get directly accessable address and return 0
static void *get_direct_memhandler(void *table, u_int addr,
  enum stub_type type, uintptr_t *addr_host)
{
  uintptr_t msb = 1ull << (sizeof(uintptr_t)*8 - 1);
  uintptr_t l1, l2 = 0;
  l1 = ((uintptr_t *)table)[addr>>12];
  if (!(l1 & msb)) {
    uintptr_t v = l1 << 1;
    *addr_host = v + addr;
    return NULL;
  }
  else {
    l1 <<= 1;
    if (type == LOADB_STUB || type == LOADBU_STUB || type == STOREB_STUB)
      l2 = ((uintptr_t *)l1)[0x1000/4 + 0x1000/2 + (addr&0xfff)];
    else if (type == LOADH_STUB || type == LOADHU_STUB || type == STOREH_STUB)
      l2 = ((uintptr_t *)l1)[0x1000/4 + (addr&0xfff)/2];
    else
      l2 = ((uintptr_t *)l1)[(addr&0xfff)/4];
    if (!(l2 & msb)) {
      uintptr_t v = l2 << 1;
      *addr_host = v + (addr&0xfff);
      return NULL;
    }
    return (void *)(l2 << 1);
  }
}

static u_int get_host_reglist(const signed char *regmap)
{
  u_int reglist = 0, hr;
  for (hr = 0; hr < HOST_REGS; hr++) {
    if (hr != EXCLUDE_REG && regmap[hr] >= 0)
      reglist |= 1 << hr;
  }
  return reglist;
}

static u_int reglist_exclude(u_int reglist, int r1, int r2)
{
  if (r1 >= 0)
    reglist &= ~(1u << r1);
  if (r2 >= 0)
    reglist &= ~(1u << r2);
  return reglist;
}

// find a temp caller-saved register not in reglist (so assumed to be free)
static int reglist_find_free(u_int reglist)
{
  u_int free_regs = ~reglist & CALLER_SAVE_REGS;
  if (free_regs == 0)
    return -1;
  return __builtin_ctz(free_regs);
}

static void do_load_word(int a, int rt, int offset_reg)
{
  if (offset_reg >= 0)
    emit_ldr_dualindexed(offset_reg, a, rt);
  else
    emit_readword_indexed(0, a, rt);
}

static void do_store_word(int a, int ofs, int rt, int offset_reg, int preseve_a)
{
  if (offset_reg < 0) {
    emit_writeword_indexed(rt, ofs, a);
    return;
  }
  if (ofs != 0)
    emit_addimm(a, ofs, a);
  emit_str_dualindexed(offset_reg, a, rt);
  if (ofs != 0 && preseve_a)
    emit_addimm(a, -ofs, a);
}

static void do_store_hword(int a, int ofs, int rt, int offset_reg, int preseve_a)
{
  if (offset_reg < 0) {
    emit_writehword_indexed(rt, ofs, a);
    return;
  }
  if (ofs != 0)
    emit_addimm(a, ofs, a);
  emit_strh_dualindexed(offset_reg, a, rt);
  if (ofs != 0 && preseve_a)
    emit_addimm(a, -ofs, a);
}

static void do_store_byte(int a, int rt, int offset_reg)
{
  if (offset_reg >= 0)
    emit_strb_dualindexed(offset_reg, a, rt);
  else
    emit_writebyte_indexed(rt, 0, a);
}

static void load_assemble(int i, const struct regstat *i_regs, int ccadj_)
{
  int addr = cinfo[i].addr;
  int s,tl;
  int offset;
  void *jaddr=0;
  int memtarget=0,c=0;
  int offset_reg = -1;
  int fastio_reg_override = -1;
  u_int reglist=get_host_reglist(i_regs->regmap);
  tl=get_reg_w(i_regs->regmap, dops[i].rt1);
  s=get_reg(i_regs->regmap,dops[i].rs1);
  offset=cinfo[i].imm;
  if(i_regs->regmap[HOST_CCREG]==CCREG) reglist&=~(1<<HOST_CCREG);
  if(s>=0) {
    c=(i_regs->wasconst>>s)&1;
    if (c) {
      memtarget=((signed int)(constmap[i][s]+offset))<(signed int)0x80000000+RAM_SIZE;
    }
  }
  //printf("load_assemble: c=%d\n",c);
  //if(c) printf("load_assemble: const=%lx\n",(long)constmap[i][s]+offset);
  if(tl<0 && ((!c||(((u_int)constmap[i][s]+offset)>>16)==0x1f80) || dops[i].rt1==0)) {
      // could be FIFO, must perform the read
      // ||dummy read
      assem_debug("(forced read)\n");
      tl = get_reg_temp(i_regs->regmap); // may be == addr
      assert(tl>=0);
  }
  assert(addr >= 0);
 if(tl>=0) {
  //printf("load_assemble: c=%d\n",c);
  //if(c) printf("load_assemble: const=%lx\n",(long)constmap[i][s]+offset);
  reglist&=~(1<<tl);
  if(!c) {
    #ifdef R29_HACK
    // Strmnnrmn's speed hack
    if(dops[i].rs1!=29||start<0x80001000||start>=0x80000000+RAM_SIZE)
    #endif
    {
      jaddr = emit_fastpath_cmp_jump(i, i_regs, addr,
                &offset_reg, &fastio_reg_override, ccadj_);
    }
  }
  else if (ram_offset && memtarget) {
    offset_reg = get_ro_reg(i_regs, 0);
  }
  int dummy=(dops[i].rt1==0)||(tl!=get_reg_w(i_regs->regmap, dops[i].rt1)); // ignore loads to r0 and unneeded reg
  switch (dops[i].opcode) {
  case 0x20: // LB
    if(!c||memtarget) {
      if(!dummy) {
        int a = addr;
        if (fastio_reg_override >= 0)
          a = fastio_reg_override;

        if (offset_reg >= 0)
          emit_ldrsb_dualindexed(offset_reg, a, tl);
        else
          emit_movsbl_indexed(0, a, tl);
      }
      if(jaddr)
        add_stub_r(LOADB_STUB,jaddr,out,i,addr,i_regs,ccadj_,reglist);
    }
    else
      inline_readstub(LOADB_STUB,i,constmap[i][s]+offset,i_regs->regmap,dops[i].rt1,ccadj_,reglist);
    break;
  case 0x21: // LH
    if(!c||memtarget) {
      if(!dummy) {
        int a = addr;
        if (fastio_reg_override >= 0)
          a = fastio_reg_override;
        if (offset_reg >= 0)
          emit_ldrsh_dualindexed(offset_reg, a, tl);
        else
          emit_movswl_indexed(0, a, tl);
      }
      if(jaddr)
        add_stub_r(LOADH_STUB,jaddr,out,i,addr,i_regs,ccadj_,reglist);
    }
    else
      inline_readstub(LOADH_STUB,i,constmap[i][s]+offset,i_regs->regmap,dops[i].rt1,ccadj_,reglist);
    break;
  case 0x23: // LW
    if(!c||memtarget) {
      if(!dummy) {
        int a = addr;
        if (fastio_reg_override >= 0)
          a = fastio_reg_override;
        do_load_word(a, tl, offset_reg);
      }
      if(jaddr)
        add_stub_r(LOADW_STUB,jaddr,out,i,addr,i_regs,ccadj_,reglist);
    }
    else
      inline_readstub(LOADW_STUB,i,constmap[i][s]+offset,i_regs->regmap,dops[i].rt1,ccadj_,reglist);
    break;
  case 0x24: // LBU
    if(!c||memtarget) {
      if(!dummy) {
        int a = addr;
        if (fastio_reg_override >= 0)
          a = fastio_reg_override;

        if (offset_reg >= 0)
          emit_ldrb_dualindexed(offset_reg, a, tl);
        else
          emit_movzbl_indexed(0, a, tl);
      }
      if(jaddr)
        add_stub_r(LOADBU_STUB,jaddr,out,i,addr,i_regs,ccadj_,reglist);
    }
    else
      inline_readstub(LOADBU_STUB,i,constmap[i][s]+offset,i_regs->regmap,dops[i].rt1,ccadj_,reglist);
    break;
  case 0x25: // LHU
    if(!c||memtarget) {
      if(!dummy) {
        int a = addr;
        if (fastio_reg_override >= 0)
          a = fastio_reg_override;
        if (offset_reg >= 0)
          emit_ldrh_dualindexed(offset_reg, a, tl);
        else
          emit_movzwl_indexed(0, a, tl);
      }
      if(jaddr)
        add_stub_r(LOADHU_STUB,jaddr,out,i,addr,i_regs,ccadj_,reglist);
    }
    else
      inline_readstub(LOADHU_STUB,i,constmap[i][s]+offset,i_regs->regmap,dops[i].rt1,ccadj_,reglist);
    break;
  default:
    assert(0);
  }
 } // tl >= 0
 if (fastio_reg_override == HOST_TEMPREG || offset_reg == HOST_TEMPREG)
   host_tempreg_release();
}

#ifndef loadlr_assemble
static void loadlr_assemble(int i, const struct regstat *i_regs, int ccadj_)
{
  int addr = cinfo[i].addr;
  int s,tl,temp,temp2;
  int offset;
  void *jaddr=0;
  int memtarget=0,c=0;
  int offset_reg = -1;
  int fastio_reg_override = -1;
  u_int reglist=get_host_reglist(i_regs->regmap);
  tl=get_reg_w(i_regs->regmap, dops[i].rt1);
  s=get_reg(i_regs->regmap,dops[i].rs1);
  temp=get_reg_temp(i_regs->regmap);
  temp2=get_reg(i_regs->regmap,FTEMP);
  offset=cinfo[i].imm;
  reglist|=1<<temp;
  assert(addr >= 0);
  if(s>=0) {
    c=(i_regs->wasconst>>s)&1;
    if(c) {
      memtarget=((signed int)(constmap[i][s]+offset))<(signed int)0x80000000+RAM_SIZE;
    }
  }
  if(!c) {
    emit_shlimm(addr,3,temp);
    if (dops[i].opcode==0x22||dops[i].opcode==0x26) {
      emit_andimm(addr,0xFFFFFFFC,temp2); // LWL/LWR
    }else{
      emit_andimm(addr,0xFFFFFFF8,temp2); // LDL/LDR
    }
    jaddr = emit_fastpath_cmp_jump(i, i_regs, temp2,
              &offset_reg, &fastio_reg_override, ccadj_);
  }
  else {
    if (ram_offset && memtarget) {
      offset_reg = get_ro_reg(i_regs, 0);
    }
    if (dops[i].opcode==0x22||dops[i].opcode==0x26) {
      emit_movimm(((constmap[i][s]+offset)<<3)&24,temp); // LWL/LWR
    }else{
      emit_movimm(((constmap[i][s]+offset)<<3)&56,temp); // LDL/LDR
    }
  }
  if (dops[i].opcode==0x22||dops[i].opcode==0x26) { // LWL/LWR
    if(!c||memtarget) {
      int a = temp2;
      if (fastio_reg_override >= 0)
        a = fastio_reg_override;
      do_load_word(a, temp2, offset_reg);
      if (fastio_reg_override == HOST_TEMPREG || offset_reg == HOST_TEMPREG)
        host_tempreg_release();
      if(jaddr) add_stub_r(LOADW_STUB,jaddr,out,i,temp2,i_regs,ccadj_,reglist);
    }
    else
      inline_readstub(LOADW_STUB,i,(constmap[i][s]+offset)&0xFFFFFFFC,i_regs->regmap,FTEMP,ccadj_,reglist);
    if(dops[i].rt1) {
      assert(tl>=0);
      emit_andimm(temp,24,temp);
      if (dops[i].opcode==0x22) // LWL
        emit_xorimm(temp,24,temp);
      host_tempreg_acquire();
      emit_movimm(-1,HOST_TEMPREG);
      if (dops[i].opcode==0x26) {
        emit_shr(temp2,temp,temp2);
        emit_bic_lsr(tl,HOST_TEMPREG,temp,tl);
      }else{
        emit_shl(temp2,temp,temp2);
        emit_bic_lsl(tl,HOST_TEMPREG,temp,tl);
      }
      host_tempreg_release();
      emit_or(temp2,tl,tl);
    }
    //emit_storereg(dops[i].rt1,tl); // DEBUG
  }
  if (dops[i].opcode==0x1A||dops[i].opcode==0x1B) { // LDL/LDR
    assert(0);
  }
}
#endif

static void do_invstub(int n)
{
  literal_pool(20);
  assem_debug("do_invstub %x\n", start + stubs[n].e*4);
  u_int reglist = stubs[n].a;
  u_int addrr = stubs[n].b;
  int ofs_start = stubs[n].c;
  int ofs_end = stubs[n].d;
  int len = ofs_end - ofs_start;
  u_int rightr = 0;

  set_jump_target(stubs[n].addr, out);
  save_regs(reglist);
  if (addrr != 0 || ofs_start != 0)
    emit_addimm(addrr, ofs_start, 0);
  emit_readword(&inv_code_start, 2);
  emit_readword(&inv_code_end, 3);
  if (len != 0)
    emit_addimm(0, len + 4, (rightr = 1));
  emit_cmp(0, 2);
  emit_cmpcs(3, rightr);
  void *jaddr = out;
  emit_jc(0);
  void *func = (len != 0)
    ? (void *)ndrc_write_invalidate_many
    : (void *)ndrc_write_invalidate_one;
  emit_far_call(func);
  set_jump_target(jaddr, out);
  restore_regs(reglist);
  emit_jmp(stubs[n].retaddr);
}

static void do_store_smc_check(int i, const struct regstat *i_regs, u_int reglist, int addr)
{
  if (HACK_ENABLED(NDHACK_NO_SMC_CHECK))
    return;
  // this can't be used any more since we started to check exact
  // block boundaries in invalidate_range()
  //if (i_regs->waswritten & (1<<dops[i].rs1))
  //  return;
  // (naively) assume nobody will run code from stack
  if (dops[i].rs1 == 29)
    return;

  int j, imm_maxdiff = 32, imm_min = cinfo[i].imm, imm_max = cinfo[i].imm, count = 1;
  if (i < slen - 1 && dops[i+1].is_store && dops[i+1].rs1 == dops[i].rs1
      && abs(cinfo[i+1].imm - cinfo[i].imm) <= imm_maxdiff)
    return;
  for (j = i - 1; j >= 0; j--) {
    if (!dops[j].is_store || dops[j].rs1 != dops[i].rs1
        || abs(cinfo[j].imm - cinfo[j+1].imm) > imm_maxdiff)
      break;
    count++;
    if (imm_min > cinfo[j].imm)
      imm_min = cinfo[j].imm;
    if (imm_max < cinfo[j].imm)
      imm_max = cinfo[j].imm;
  }
#if defined(HOST_IMM8)
  int ir = get_reg(i_regs->regmap, INVCP);
  assert(ir >= 0);
  host_tempreg_acquire();
  emit_ldrb_indexedsr12_reg(ir, addr, HOST_TEMPREG);
#else
  emit_cmpmem_indexedsr12_imm(invalid_code, addr, 1);
  #error not handled
#endif
  (void)count;
#ifdef INVALIDATE_USE_COND_CALL
  if (count == 1) {
    emit_cmpimm(HOST_TEMPREG, 1);
    emit_callne(invalidate_addr_reg[addr]);
    host_tempreg_release();
    return;
  }
#endif
  void *jaddr = emit_cbz(HOST_TEMPREG, 0);
  host_tempreg_release();
  imm_min -= cinfo[i].imm;
  imm_max -= cinfo[i].imm;
  add_stub(INVCODE_STUB, jaddr, out, reglist|(1<<HOST_CCREG),
    addr, imm_min, imm_max, i);
}

// determines if code overwrite checking is needed only
// (also true non-existent 0x20000000 mirror that shouldn't matter)
#define is_ram_addr(a) !((a) & 0x5f800000)

static void store_assemble(int i, const struct regstat *i_regs, int ccadj_)
{
  int s,tl;
  int addr = cinfo[i].addr;
  int offset;
  void *jaddr=0;
  enum stub_type type=0;
  int memtarget=0,c=0;
  int offset_reg = -1;
  int fastio_reg_override = -1;
  u_int addr_const = ~0;
  u_int reglist=get_host_reglist(i_regs->regmap);
  tl=get_reg(i_regs->regmap,dops[i].rs2);
  s=get_reg(i_regs->regmap,dops[i].rs1);
  offset=cinfo[i].imm;
  if(s>=0) {
    c=(i_regs->wasconst>>s)&1;
    if (c) {
      addr_const = constmap[i][s] + offset;
      memtarget = ((signed int)addr_const) < (signed int)(0x80000000 + RAM_SIZE);
    }
  }
  assert(tl>=0);
  assert(addr >= 0);
  if(i_regs->regmap[HOST_CCREG]==CCREG) reglist&=~(1<<HOST_CCREG);
  reglist |= 1u << addr;
  if (!c) {
    jaddr = emit_fastpath_cmp_jump(i, i_regs, addr,
              &offset_reg, &fastio_reg_override, ccadj_);
  }
  else if (ram_offset && memtarget) {
    offset_reg = get_ro_reg(i_regs, 0);
  }

  switch (dops[i].opcode) {
  case 0x28: // SB
    if(!c||memtarget) {
      int a = addr;
      if (fastio_reg_override >= 0)
        a = fastio_reg_override;
      do_store_byte(a, tl, offset_reg);
    }
    type = STOREB_STUB;
    break;
  case 0x29: // SH
    if(!c||memtarget) {
      int a = addr;
      if (fastio_reg_override >= 0)
        a = fastio_reg_override;
      do_store_hword(a, 0, tl, offset_reg, 1);
    }
    type = STOREH_STUB;
    break;
  case 0x2B: // SW
    if(!c||memtarget) {
      int a = addr;
      if (fastio_reg_override >= 0)
        a = fastio_reg_override;
      do_store_word(a, 0, tl, offset_reg, 1);
    }
    type = STOREW_STUB;
    break;
  default:
    assert(0);
  }
  if (fastio_reg_override == HOST_TEMPREG || offset_reg == HOST_TEMPREG)
    host_tempreg_release();
  if (jaddr) {
    // PCSX store handlers don't check invcode again
    add_stub_r(type,jaddr,out,i,addr,i_regs,ccadj_,reglist);
  }
  if (!c || is_ram_addr(addr_const))
    do_store_smc_check(i, i_regs, reglist, addr);
  if (c && !memtarget)
    inline_writestub(type, i, addr_const, i_regs->regmap, dops[i].rs2, ccadj_, reglist);
  // basic current block modification detection..
  // not looking back as that should be in mips cache already
  // (see Spyro2 title->attract mode)
  if (start + i*4 < addr_const && addr_const < start + slen*4) {
    SysPrintf_lim("write to %08x hits block %08x, pc=%08x\n",
      addr_const, start, start+i*4);
    assert(i_regs->regmap==regs[i].regmap); // not delay slot
    if(i_regs->regmap==regs[i].regmap) {
      load_all_consts(regs[i].regmap_entry,regs[i].wasdirty,i);
      wb_dirtys(regs[i].regmap_entry,regs[i].wasdirty);
      emit_readptr(&hash_table_ptr, 1);
      emit_movimm(start+i*4+4, 0);
      emit_writeword(0, &psxRegs.pc);
      emit_addimm(HOST_CCREG, 2, HOST_CCREG);
      emit_far_call(ndrc_get_addr_ht);
      emit_jmpreg(0);
    }
  }
}

static void storelr_assemble(int i, const struct regstat *i_regs, int ccadj_)
{
  int addr = cinfo[i].addr;
  int s,tl;
  int offset;
  void *jaddr=0;
  void *case1, *case23, *case3;
  void *done0, *done1, *done2;
  int memtarget=0,c=0;
  int offset_reg = -1;
  u_int addr_const = ~0;
  u_int reglist = get_host_reglist(i_regs->regmap);
  tl=get_reg(i_regs->regmap,dops[i].rs2);
  s=get_reg(i_regs->regmap,dops[i].rs1);
  offset=cinfo[i].imm;
  if(s>=0) {
    c = (i_regs->isconst >> s) & 1;
    if (c) {
      addr_const = constmap[i][s] + offset;
      memtarget = ((signed int)addr_const) < (signed int)(0x80000000 + RAM_SIZE);
    }
  }
  assert(tl>=0);
  assert(addr >= 0);
  reglist |= 1u << addr;
  if(!c) {
    emit_cmpimm(addr, RAM_SIZE);
    jaddr=out;
    emit_jno(0);
  }
  else
  {
    if(!memtarget||!dops[i].rs1) {
      jaddr=out;
      emit_jmp(0);
    }
  }
  if (ram_offset)
    offset_reg = get_ro_reg(i_regs, 0);

  emit_testimm(addr,2);
  case23=out;
  emit_jne(0);
  emit_testimm(addr,1);
  case1=out;
  emit_jne(0);
  // 0
  if (dops[i].opcode == 0x2A) { // SWL
    // Write msb into least significant byte
    if (dops[i].rs2) emit_rorimm(tl, 24, tl);
    do_store_byte(addr, tl, offset_reg);
    if (dops[i].rs2) emit_rorimm(tl, 8, tl);
  }
  else if (dops[i].opcode == 0x2E) { // SWR
    // Write entire word
    do_store_word(addr, 0, tl, offset_reg, 1);
  }
  done0 = out;
  emit_jmp(0);
  // 1
  set_jump_target(case1, out);
  if (dops[i].opcode == 0x2A) { // SWL
    // Write two msb into two least significant bytes
    if (dops[i].rs2) emit_rorimm(tl, 16, tl);
    do_store_hword(addr, -1, tl, offset_reg, 1);
    if (dops[i].rs2) emit_rorimm(tl, 16, tl);
  }
  else if (dops[i].opcode == 0x2E) { // SWR
    // Write 3 lsb into three most significant bytes
    do_store_byte(addr, tl, offset_reg);
    if (dops[i].rs2) emit_rorimm(tl, 8, tl);
    do_store_hword(addr, 1, tl, offset_reg, 1);
    if (dops[i].rs2) emit_rorimm(tl, 24, tl);
  }
  done1=out;
  emit_jmp(0);
  // 2,3
  set_jump_target(case23, out);
  emit_testimm(addr,1);
  case3 = out;
  emit_jne(0);
  // 2
  if (dops[i].opcode==0x2A) { // SWL
    // Write 3 msb into three least significant bytes
    if (dops[i].rs2) emit_rorimm(tl, 8, tl);
    do_store_hword(addr, -2, tl, offset_reg, 1);
    if (dops[i].rs2) emit_rorimm(tl, 16, tl);
    do_store_byte(addr, tl, offset_reg);
    if (dops[i].rs2) emit_rorimm(tl, 8, tl);
  }
  else if (dops[i].opcode == 0x2E) { // SWR
    // Write two lsb into two most significant bytes
    do_store_hword(addr, 0, tl, offset_reg, 1);
  }
  done2 = out;
  emit_jmp(0);
  // 3
  set_jump_target(case3, out);
  if (dops[i].opcode == 0x2A) { // SWL
    do_store_word(addr, -3, tl, offset_reg, 1);
  }
  else if (dops[i].opcode == 0x2E) { // SWR
    do_store_byte(addr, tl, offset_reg);
  }
  set_jump_target(done0, out);
  set_jump_target(done1, out);
  set_jump_target(done2, out);
  if (offset_reg == HOST_TEMPREG)
    host_tempreg_release();
  if (!c || !memtarget)
    add_stub_r(STORELR_STUB,jaddr,out,i,addr,i_regs,ccadj_,reglist);
  if (!c || is_ram_addr(addr_const))
    do_store_smc_check(i, i_regs, reglist, addr);
}

static void cop0_assemble(int i, const struct regstat *i_regs, int ccadj_)
{
  if(dops[i].opcode2==0) // MFC0
  {
    signed char t=get_reg_w(i_regs->regmap, dops[i].rt1);
    u_int copr=(source[i]>>11)&0x1f;
    if(t>=0&&dops[i].rt1!=0) {
      emit_readword(&psxRegs.CP0.r[copr],t);
    }
  }
  else if(dops[i].opcode2==4) // MTC0
  {
    int s = get_reg(i_regs->regmap, dops[i].rs1);
    int cc = get_reg(i_regs->regmap, CCREG);
    char copr=(source[i]>>11)&0x1f;
    assert(s>=0);
    wb_register(dops[i].rs1,i_regs->regmap,i_regs->dirty);
    if (copr == 12 || copr == 13) {
      emit_readword(&last_count,HOST_TEMPREG);
      if (cc != HOST_CCREG)
        emit_loadreg(CCREG, HOST_CCREG);
      emit_add(HOST_CCREG, HOST_TEMPREG, HOST_CCREG);
      emit_addimm(HOST_CCREG, ccadj_ + 2, HOST_CCREG);
      emit_writeword(HOST_CCREG, &psxRegs.cycle);
      if (is_delayslot) {
        // burn cycles to cause cc_interrupt, which will
        // reschedule next_interupt. Relies on CCREG from above.
        assem_debug("MTC0 DS %d\n", copr);
        emit_writeword(HOST_CCREG,&last_count);
        emit_movimm(0,HOST_CCREG);
        emit_storereg(CCREG,HOST_CCREG);
        emit_loadreg(dops[i].rs1, 2);
        emit_movimm(copr, 1);
        emit_addimm_ptr(FP, (u_char *)&psxRegs - (u_char *)&dynarec_local, 0);
        emit_far_call(pcsx_mtc0_ds);
        emit_loadreg(dops[i].rs1,s);
        return;
      }
      emit_movimm(start+i*4+4,HOST_TEMPREG);
      emit_writeword(HOST_TEMPREG,&psxRegs.pc);
    }
    if (s != 2)
      emit_mov(s, 2);
    emit_movimm(copr, 1);
    emit_addimm_ptr(FP, (u_char *)&psxRegs - (u_char *)&dynarec_local, 0);
    emit_far_call(pcsx_mtc0);
    if (copr == 12 || copr == 13) {
      emit_readword(&psxRegs.cycle,HOST_CCREG);
      emit_readword(&last_count,HOST_TEMPREG);
      emit_sub(HOST_CCREG,HOST_TEMPREG,HOST_CCREG);
      //emit_writeword(HOST_TEMPREG,&last_count);
      assert(!is_delayslot);
      emit_readword(&psxRegs.pc, 0);
      emit_movimm(start+i*4+4, HOST_TEMPREG);
      emit_cmp(HOST_TEMPREG, 0);
      void *jaddr = out;
      emit_jeq(0);
      emit_readptr(&hash_table_ptr, 1);
      emit_far_call(ndrc_get_addr_ht);
      emit_jmpreg(0);
      set_jump_target(jaddr, out);
      emit_addimm(HOST_CCREG, -ccadj_ - 2, HOST_CCREG);
      if (cc != HOST_CCREG)
        emit_storereg(CCREG, HOST_CCREG);
    }
    emit_loadreg(dops[i].rs1,s);
  }
}

static void rfe_assemble(int i, const struct regstat *i_regs)
{
  emit_readword(&psxRegs.CP0.n.SR, 0);
  emit_andimm(0, 0x3c, 1);
  emit_andimm(0, ~0xf, 0);
  emit_orrshr_imm(1, 2, 0);
  emit_writeword(0, &psxRegs.CP0.n.SR);
}

static int cop2_is_stalling_op(int i, int *cycles)
{
  if (dops[i].itype == COP2 || dops[i].itype == C2LS) {
    *cycles = 0;
    return 1;
  }
  if (dops[i].itype == C2OP) {
    *cycles = gte_cycletab[source[i] & 0x3f];
    return 1;
  }
  // ... what about MTC2/CTC2/LWC2?
  return 0;
}

#if 0
static void log_gte_stall(int stall, u_int cycle)
{
  if ((u_int)stall <= 44)
    printf("x    stall %2d %u\n", stall, cycle + last_count);
}

static void emit_log_gte_stall(int i, int stall, u_int reglist)
{
  save_regs(reglist);
  if (stall > 0)
    emit_movimm(stall, 0);
  else
    emit_mov(HOST_TEMPREG, 0);
  emit_addimm(HOST_CCREG, cinfo[i].ccadj, 1);
  emit_far_call(log_gte_stall);
  restore_regs(reglist);
}
#endif

static void cop2_do_stall_check(u_int op, int i, const struct regstat *i_regs, u_int reglist)
{
  int j = i, cycles, other_gte_op_cycles = -1, stall = -MAXBLOCK, cycles_passed;
  int rtmp = reglist_find_free(reglist);

  if (HACK_ENABLED(NDHACK_NO_STALLS))
    return;
  if (get_reg(i_regs->regmap, CCREG) != HOST_CCREG) {
    // happens occasionally... cc evicted? Don't bother then
    //printf("no cc %08x\n", start + i*4);
    return;
  }
  if (!dops[i].bt) {
    for (j = i - 1; j >= 0; j--) {
      //if (dops[j].is_ds) break;
      if (cop2_is_stalling_op(j, &other_gte_op_cycles) || dops[j].bt)
        break;
      if (j > 0 && cinfo[j - 1].ccadj > cinfo[j].ccadj)
        break;
    }
    j = max(j, 0);
  }
  cycles_passed = cinfo[i].ccadj - cinfo[j].ccadj;
  if (other_gte_op_cycles >= 0)
    stall = other_gte_op_cycles - cycles_passed;
  else if (cycles_passed >= 44)
    stall = 0; // can't possibly stall
  if (stall == -MAXBLOCK && rtmp >= 0) {
    // unknown stall, do the expensive runtime check
    assem_debug("; cop2_do_stall_check\n");
    // busy - (cc + adj) -> busy - adj - cc
    host_tempreg_acquire();
    emit_readword(&psxRegs.gteBusyCycle, rtmp);
    emit_addimm(rtmp, -cinfo[i].ccadj, rtmp);
    emit_sub(rtmp, HOST_CCREG, HOST_TEMPREG);
    emit_cmpimm(HOST_TEMPREG, 44);
    emit_cmovb_reg(rtmp, HOST_CCREG);
    //emit_log_gte_stall(i, 0, reglist);
    host_tempreg_release();
  }
  else if (stall > 0) {
    //emit_log_gte_stall(i, stall, reglist);
    emit_addimm(HOST_CCREG, stall, HOST_CCREG);
  }

  // save gteBusyCycle, if needed
  cycles = gte_cycletab[op];
  if (cycles == 0)
    return;
  other_gte_op_cycles = -1;
  for (j = i + 1; j < slen; j++) {
    if (cop2_is_stalling_op(j, &other_gte_op_cycles))
      break;
    if (dops[j].is_jump) {
      // check ds
      if (j + 1 < slen && cop2_is_stalling_op(j + 1, &other_gte_op_cycles))
        j++;
      break;
    }
  }
  if (other_gte_op_cycles >= 0)
    // will handle stall when assembling that op
    return;
  cycles_passed = cinfo[min(j, slen -1)].ccadj - cinfo[i].ccadj;
  if (cycles_passed >= cycles)
    return;
  assem_debug("; save gteBusyCycle\n");
  host_tempreg_acquire();
  emit_addimm(HOST_CCREG, cinfo[i].ccadj + cycles, HOST_TEMPREG);
  emit_writeword(HOST_TEMPREG, &psxRegs.gteBusyCycle);
  host_tempreg_release();
}

static int is_mflohi(int i)
{
  return (dops[i].itype == MOV && (dops[i].rs1 == HIREG || dops[i].rs1 == LOREG));
}

static int check_multdiv(int i, int *cycles)
{
  if (dops[i].itype != MULTDIV)
    return 0;
  if (dops[i].opcode2 == 0x18 || dops[i].opcode2 == 0x19) // MULT(U)
    *cycles = 11; // approx from 7 11 14
  else
    *cycles = 37;
  return 1;
}

static void multdiv_prepare_stall(int i, const struct regstat *i_regs, int ccadj_)
{
  int j, found = 0, c = 0;
  if (HACK_ENABLED(NDHACK_NO_STALLS))
    return;
  if (get_reg(i_regs->regmap, CCREG) != HOST_CCREG) {
    // happens occasionally... cc evicted? Don't bother then
    return;
  }
  for (j = i + 1; j < slen; j++) {
    if (dops[j].bt)
      break;
    if ((found = is_mflohi(j)))
      break;
    if (dops[j].is_jump) {
      // check ds
      if (j + 1 < slen && (found = is_mflohi(j + 1)))
        j++;
      break;
    }
  }
  if (found)
    // handle all in multdiv_do_stall()
    return;
  check_multdiv(i, &c);
  assert(c > 0);
  assem_debug("; muldiv prepare stall %d\n", c);
  host_tempreg_acquire();
  emit_addimm(HOST_CCREG, ccadj_ + c, HOST_TEMPREG);
  emit_writeword(HOST_TEMPREG, &psxRegs.muldivBusyCycle);
  host_tempreg_release();
}

static void multdiv_do_stall(int i, const struct regstat *i_regs)
{
  int j, known_cycles = 0;
  u_int reglist = get_host_reglist(i_regs->regmap);
  int rtmp = get_reg_temp(i_regs->regmap);
  if (rtmp < 0)
    rtmp = reglist_find_free(reglist);
  if (HACK_ENABLED(NDHACK_NO_STALLS))
    return;
  if (get_reg(i_regs->regmap, CCREG) != HOST_CCREG || rtmp < 0) {
    // happens occasionally... cc evicted? Don't bother then
    //printf("no cc/rtmp %08x\n", start + i*4);
    return;
  }
  if (!dops[i].bt) {
    for (j = i - 1; j >= 0; j--) {
      if (dops[j].is_ds) break;
      if (check_multdiv(j, &known_cycles))
        break;
      if (is_mflohi(j))
        // already handled by this op
        return;
      if (dops[j].bt || (j > 0 && cinfo[j - 1].ccadj > cinfo[j].ccadj))
        break;
    }
    j = max(j, 0);
  }
  if (known_cycles > 0) {
    known_cycles -= cinfo[i].ccadj - cinfo[j].ccadj;
    assem_debug("; muldiv stall resolved %d\n", known_cycles);
    if (known_cycles > 0)
      emit_addimm(HOST_CCREG, known_cycles, HOST_CCREG);
    return;
  }
  assem_debug("; muldiv stall unresolved\n");
  host_tempreg_acquire();
  emit_readword(&psxRegs.muldivBusyCycle, rtmp);
  emit_addimm(rtmp, -cinfo[i].ccadj, rtmp);
  emit_sub(rtmp, HOST_CCREG, HOST_TEMPREG);
  emit_cmpimm(HOST_TEMPREG, 37);
  emit_cmovb_reg(rtmp, HOST_CCREG);
  //emit_log_gte_stall(i, 0, reglist);
  host_tempreg_release();
}

static void cop2_get_dreg(u_int copr,signed char tl,signed char temp)
{
  switch (copr) {
    case 1:
    case 3:
    case 5:
    case 8:
    case 9:
    case 10:
    case 11:
      emit_readword(&reg_cop2d[copr],tl);
      emit_signextend16(tl,tl);
      emit_writeword(tl,&reg_cop2d[copr]); // hmh
      break;
    case 7:
    case 16:
    case 17:
    case 18:
    case 19:
      emit_readword(&reg_cop2d[copr],tl);
      emit_andimm(tl,0xffff,tl);
      emit_writeword(tl,&reg_cop2d[copr]);
      break;
    case 15:
      emit_readword(&reg_cop2d[14],tl); // SXY2
      emit_writeword(tl,&reg_cop2d[copr]);
      break;
    case 28:
    case 29:
      c2op_mfc2_29_assemble(tl,temp);
      break;
    default:
      emit_readword(&reg_cop2d[copr],tl);
      break;
  }
}

static void cop2_put_dreg(u_int copr,signed char sl,signed char temp)
{
  switch (copr) {
    case 15:
      emit_readword(&reg_cop2d[13],temp);  // SXY1
      emit_writeword(sl,&reg_cop2d[copr]);
      emit_writeword(temp,&reg_cop2d[12]); // SXY0
      emit_readword(&reg_cop2d[14],temp);  // SXY2
      emit_writeword(sl,&reg_cop2d[14]);
      emit_writeword(temp,&reg_cop2d[13]); // SXY1
      break;
    case 28:
      emit_andimm(sl,0x001f,temp);
      emit_shlimm(temp,7,temp);
      emit_writeword(temp,&reg_cop2d[9]);
      emit_andimm(sl,0x03e0,temp);
      emit_shlimm(temp,2,temp);
      emit_writeword(temp,&reg_cop2d[10]);
      emit_andimm(sl,0x7c00,temp);
      emit_shrimm(temp,3,temp);
      emit_writeword(temp,&reg_cop2d[11]);
      emit_writeword(sl,&reg_cop2d[28]);
      break;
    case 30:
      emit_xorsar_imm(sl,sl,31,temp);
#if defined(HAVE_ARMV5) || defined(__aarch64__)
      emit_clz(temp,temp);
#else
      emit_movs(temp,HOST_TEMPREG);
      emit_movimm(0,temp);
      emit_jeq((int)out+4*4);
      emit_addpl_imm(temp,1,temp);
      emit_lslpls_imm(HOST_TEMPREG,1,HOST_TEMPREG);
      emit_jns((int)out-2*4);
#endif
      emit_writeword(sl,&reg_cop2d[30]);
      emit_writeword(temp,&reg_cop2d[31]);
      break;
    case 31:
      break;
    default:
      emit_writeword(sl,&reg_cop2d[copr]);
      break;
  }
}

static void c2ls_assemble(int i, const struct regstat *i_regs, int ccadj_)
{
  int s,tl;
  int ar;
  int offset;
  int memtarget=0,c=0;
  void *jaddr2=NULL;
  enum stub_type type;
  int offset_reg = -1;
  int fastio_reg_override = -1;
  u_int addr_const = ~0;
  u_int reglist=get_host_reglist(i_regs->regmap);
  u_int copr=(source[i]>>16)&0x1f;
  s=get_reg(i_regs->regmap,dops[i].rs1);
  tl=get_reg(i_regs->regmap,FTEMP);
  offset=cinfo[i].imm;
  assert(tl>=0);

  if(i_regs->regmap[HOST_CCREG]==CCREG)
    reglist&=~(1<<HOST_CCREG);

  // get the address
  ar = cinfo[i].addr;
  assert(ar >= 0);
  if (dops[i].opcode==0x3a) { // SWC2
    reglist |= 1<<ar;
  }
  if (s >= 0) {
    c = (i_regs->isconst >> s) & 1;
    if (c) {
      addr_const = constmap[i][s] + offset;
      memtarget = ((signed int)addr_const) < (signed int)(0x80000000 + RAM_SIZE);
    }
  }

  cop2_do_stall_check(0, i, i_regs, reglist);

  if (dops[i].opcode==0x3a) { // SWC2
    cop2_get_dreg(copr,tl,-1);
    type=STOREW_STUB;
  }
  else
    type=LOADW_STUB;

  if(c&&!memtarget) {
    jaddr2=out;
    emit_jmp(0); // inline_readstub/inline_writestub?
  }
  else {
    if(!c) {
      jaddr2 = emit_fastpath_cmp_jump(i, i_regs, ar,
                &offset_reg, &fastio_reg_override, ccadj_);
    }
    else if (ram_offset && memtarget) {
      offset_reg = get_ro_reg(i_regs, 0);
    }
    switch (dops[i].opcode) {
    case 0x32: { // LWC2
      int a = ar;
      if (fastio_reg_override >= 0)
        a = fastio_reg_override;
      do_load_word(a, tl, offset_reg);
      break;
    }
    case 0x3a: { // SWC2
      #ifdef DESTRUCTIVE_SHIFT
      if(!offset&&!c&&s>=0) emit_mov(s,ar);
      #endif
      int a = ar;
      if (fastio_reg_override >= 0)
        a = fastio_reg_override;
      do_store_word(a, 0, tl, offset_reg, 1);
      break;
    }
    default:
      assert(0);
    }
  }
  if (fastio_reg_override == HOST_TEMPREG || offset_reg == HOST_TEMPREG)
    host_tempreg_release();
  if(jaddr2)
    add_stub_r(type,jaddr2,out,i,ar,i_regs,ccadj_,reglist);
  if (dops[i].opcode == 0x3a && (!c || is_ram_addr(addr_const))) // SWC2
    do_store_smc_check(i, i_regs, reglist, ar);
  if (dops[i].opcode == 0x32) { // LWC2
    host_tempreg_acquire();
    cop2_put_dreg(copr,tl,HOST_TEMPREG);
    host_tempreg_release();
  }
}

static void cop2_assemble(int i, const struct regstat *i_regs)
{
  u_int copr = (source[i]>>11) & 0x1f;
  signed char temp = get_reg_temp(i_regs->regmap);

  if (!HACK_ENABLED(NDHACK_NO_STALLS)) {
    u_int reglist = reglist_exclude(get_host_reglist(i_regs->regmap), temp, -1);
    if (dops[i].opcode2 == 0 || dops[i].opcode2 == 2) { // MFC2/CFC2
      signed char tl = get_reg(i_regs->regmap, dops[i].rt1);
      reglist = reglist_exclude(reglist, tl, -1);
    }
    cop2_do_stall_check(0, i, i_regs, reglist);
  }
  if (dops[i].opcode2==0) { // MFC2
    signed char tl=get_reg_w(i_regs->regmap, dops[i].rt1);
    if(tl>=0&&dops[i].rt1!=0)
      cop2_get_dreg(copr,tl,temp);
  }
  else if (dops[i].opcode2==4) { // MTC2
    signed char sl=get_reg(i_regs->regmap,dops[i].rs1);
    cop2_put_dreg(copr,sl,temp);
  }
  else if (dops[i].opcode2==2) // CFC2
  {
    signed char tl=get_reg_w(i_regs->regmap, dops[i].rt1);
    if(tl>=0&&dops[i].rt1!=0)
      emit_readword(&reg_cop2c[copr],tl);
  }
  else if (dops[i].opcode2==6) // CTC2
  {
    signed char sl=get_reg(i_regs->regmap,dops[i].rs1);
    switch(copr) {
      case 4:
      case 12:
      case 20:
      case 26:
      case 27:
      case 29:
      case 30:
        emit_signextend16(sl,temp);
        break;
      case 31:
        c2op_ctc2_31_assemble(sl,temp);
        break;
      default:
        temp=sl;
        break;
    }
    emit_writeword(temp,&reg_cop2c[copr]);
    assert(sl>=0);
  }
}

static void do_unalignedwritestub(int n)
{
  assem_debug("do_unalignedwritestub %x\n",start+stubs[n].a*4);
  literal_pool(256);
  set_jump_target(stubs[n].addr, out);

  int i=stubs[n].a;
  struct regstat *i_regs=(struct regstat *)stubs[n].c;
  int addr=stubs[n].b;
  u_int reglist=stubs[n].e;
  signed char *i_regmap=i_regs->regmap;
  int temp2=get_reg(i_regmap,FTEMP);
  int rt;
  rt=get_reg(i_regmap,dops[i].rs2);
  assert(rt>=0);
  assert(addr>=0);
  assert(dops[i].opcode==0x2a||dops[i].opcode==0x2e); // SWL/SWR only implemented
  reglist|=(1<<addr);
  reglist&=~(1<<temp2);

  // don't bother with it and call write handler
  save_regs(reglist);
  pass_args(addr,rt);
  int cc=get_reg(i_regmap,CCREG);
  if(cc<0)
    emit_loadreg(CCREG,2);
  emit_addimm(cc<0?2:cc,(int)stubs[n].d+1,2);
  emit_movimm(start + i*4,3);
  emit_writeword(3,&psxRegs.pc);
  emit_far_call((dops[i].opcode==0x2a?jump_handle_swl:jump_handle_swr));
  emit_addimm(0,-((int)stubs[n].d+1),cc<0?2:cc);
  if(cc<0)
    emit_storereg(CCREG,2);
  restore_regs(reglist);
  emit_jmp(stubs[n].retaddr); // return address
}

static void do_overflowstub(int n)
{
  assem_debug("do_overflowstub %x\n", start + (u_int)stubs[n].a * 4);
  literal_pool(24);
  int i = stubs[n].a;
  struct regstat *i_regs = (struct regstat *)stubs[n].c;
  int ccadj = stubs[n].d;
  set_jump_target(stubs[n].addr, out);
  wb_dirtys(regs[i].regmap, regs[i].dirty);
  exception_assemble(i, i_regs, ccadj);
}

static void do_alignmentstub(int n)
{
  assem_debug("do_alignmentstub %x\n", start + (u_int)stubs[n].a * 4);
  literal_pool(24);
  int i = stubs[n].a;
  struct regstat *i_regs = (struct regstat *)stubs[n].c;
  int ccadj = stubs[n].d;
  int is_store = dops[i].itype == STORE || dops[i].opcode == 0x3A; // SWC2
  int cause = (dops[i].opcode & 3) << 28;
  cause |= is_store ? (R3000E_AdES << 2) : (R3000E_AdEL << 2);
  set_jump_target(stubs[n].addr, out);
  wb_dirtys(regs[i].regmap, regs[i].dirty);
  if (stubs[n].b != 1)
    emit_mov(stubs[n].b, 1); // faulting address
  emit_movimm(cause, 0);
  exception_assemble(i, i_regs, ccadj);
}

#ifndef multdiv_assemble
void multdiv_assemble(int i,struct regstat *i_regs)
{
  printf("Need multdiv_assemble for this architecture.\n");
  abort();
}
#endif

static void mov_assemble(int i, const struct regstat *i_regs)
{
  //if(dops[i].opcode2==0x10||dops[i].opcode2==0x12) { // MFHI/MFLO
  //if(dops[i].opcode2==0x11||dops[i].opcode2==0x13) { // MTHI/MTLO
  if(dops[i].rt1) {
    signed char sl,tl;
    tl=get_reg_w(i_regs->regmap, dops[i].rt1);
    //assert(tl>=0);
    if(tl>=0) {
      sl=get_reg(i_regs->regmap,dops[i].rs1);
      if(sl>=0) emit_mov(sl,tl);
      else emit_loadreg(dops[i].rs1,tl);
    }
  }
  if (dops[i].rs1 == HIREG || dops[i].rs1 == LOREG) // MFHI/MFLO
    multdiv_do_stall(i, i_regs);
}

// call interpreter, exception handler, things that change pc/regs/cycles ...
static void call_c_cpu_handler(int i, const struct regstat *i_regs, int ccadj_, u_int pc, void *func)
{
  signed char ccreg=get_reg(i_regs->regmap,CCREG);
  assert(ccreg==HOST_CCREG);
  assert(!is_delayslot);
  (void)ccreg;

  emit_movimm(pc,3); // Get PC
  emit_readword(&last_count,2);
  emit_writeword(3,&psxRegs.pc);
  emit_addimm(HOST_CCREG,ccadj_,HOST_CCREG);
  emit_add(2,HOST_CCREG,2);
  emit_writeword(2,&psxRegs.cycle);
  emit_addimm_ptr(FP,(u_char *)&psxRegs - (u_char *)&dynarec_local,0);
  emit_far_call(func);
  emit_far_jump(jump_to_new_pc);
}

static void exception_assemble(int i, const struct regstat *i_regs, int ccadj_)
{
  // 'break' tends to be littered around to catch things like
  // division by 0 and is almost never executed, so don't emit much code here
  void *func;
  if (dops[i].itype == ALU || dops[i].itype == IMM16)
    func = is_delayslot ? jump_overflow_ds : jump_overflow;
  else if (dops[i].itype == LOAD || dops[i].itype == STORE)
    func = is_delayslot ? jump_addrerror_ds : jump_addrerror;
  else if (dops[i].opcode2 == 0x0C)
    func = is_delayslot ? jump_syscall_ds : jump_syscall;
  else
    func = is_delayslot ? jump_break_ds : jump_break;
  if (get_reg(i_regs->regmap, CCREG) != HOST_CCREG) // evicted
    emit_loadreg(CCREG, HOST_CCREG);
  emit_movimm(start + i*4, 2); // pc
  emit_addimm(HOST_CCREG, ccadj_ + CLOCK_ADJUST(1), HOST_CCREG);
  emit_far_jump(func);
}

static void hlecall_bad()
{
  assert(0);
}

static void hlecall_assemble(int i, const struct regstat *i_regs, int ccadj_)
{
  void *hlefunc = hlecall_bad;
  uint32_t hleCode = source[i] & 0x03ffffff;
  if (hleCode < ARRAY_SIZE(psxHLEt))
    hlefunc = psxHLEt[hleCode];

  call_c_cpu_handler(i, i_regs, ccadj_, start + i*4+4, hlefunc);
}

static void intcall_assemble(int i, const struct regstat *i_regs, int ccadj_)
{
  call_c_cpu_handler(i, i_regs, ccadj_, start + i*4, execI);
}

static void speculate_mov(int rs,int rt)
{
  if (rt != 0) {
    smrv_strong_next |= 1 << rt;
    ndrc_smrv_regs[rt] = ndrc_smrv_regs[rs];
  }
}

static void speculate_mov_weak(int rs,int rt)
{
  if (rt != 0) {
    smrv_weak_next |= 1 << rt;
    ndrc_smrv_regs[rt] = ndrc_smrv_regs[rs];
  }
}

static void speculate_register_values(int i)
{
  if(i==0) {
    // gp,sp are likely to stay the same throughout the block
    smrv_strong_next=(1<<28)|(1<<29)|(1<<30);
    smrv_weak_next=~smrv_strong_next;
    //printf(" llr %08x\n", ndrc_smrv_regs[4]);
  }
  smrv_strong=smrv_strong_next;
  smrv_weak=smrv_weak_next;
  switch(dops[i].itype) {
    case ALU:
      if     ((smrv_strong>>dops[i].rs1)&1) speculate_mov(dops[i].rs1,dops[i].rt1);
      else if((smrv_strong>>dops[i].rs2)&1) speculate_mov(dops[i].rs2,dops[i].rt1);
      else if((smrv_weak>>dops[i].rs1)&1) speculate_mov_weak(dops[i].rs1,dops[i].rt1);
      else if((smrv_weak>>dops[i].rs2)&1) speculate_mov_weak(dops[i].rs2,dops[i].rt1);
      else {
        smrv_strong_next&=~(1<<dops[i].rt1);
        smrv_weak_next&=~(1<<dops[i].rt1);
      }
      break;
    case SHIFTIMM:
      smrv_strong_next&=~(1<<dops[i].rt1);
      smrv_weak_next&=~(1<<dops[i].rt1);
      // fallthrough
    case IMM16:
      if(dops[i].rt1&&is_const(&regs[i],dops[i].rt1)) {
        int hr = get_reg_w(regs[i].regmap, dops[i].rt1);
        u_int value;
        if(hr>=0) {
          if(get_final_value(hr,i,&value))
               ndrc_smrv_regs[dops[i].rt1]=value;
          else ndrc_smrv_regs[dops[i].rt1]=constmap[i][hr];
          smrv_strong_next|=1<<dops[i].rt1;
        }
      }
      else {
        if     ((smrv_strong>>dops[i].rs1)&1) speculate_mov(dops[i].rs1,dops[i].rt1);
        else if((smrv_weak>>dops[i].rs1)&1) speculate_mov_weak(dops[i].rs1,dops[i].rt1);
      }
      break;
    case LOAD:
      if(start<0x2000&&(dops[i].rt1==26||(ndrc_smrv_regs[dops[i].rt1]>>24)==0xa0)) {
        // special case for BIOS
        ndrc_smrv_regs[dops[i].rt1]=0xa0000000;
        smrv_strong_next|=1<<dops[i].rt1;
        break;
      }
      // fallthrough
    case SHIFT:
    case LOADLR:
    case MOV:
      smrv_strong_next&=~(1<<dops[i].rt1);
      smrv_weak_next&=~(1<<dops[i].rt1);
      break;
    case COP0:
    case COP2:
      if(dops[i].opcode2==0||dops[i].opcode2==2) { // MFC/CFC
        smrv_strong_next&=~(1<<dops[i].rt1);
        smrv_weak_next&=~(1<<dops[i].rt1);
      }
      break;
    case C2LS:
      if (dops[i].opcode==0x32) { // LWC2
        smrv_strong_next&=~(1<<dops[i].rt1);
        smrv_weak_next&=~(1<<dops[i].rt1);
      }
      break;
  }
#if 0
  int r=4;
  printf("x %08x %08x %d %d c %08x %08x\n",ndrc_smrv_regs[r],start+i*4,
    ((smrv_strong>>r)&1),(smrv_weak>>r)&1,regs[i].isconst,regs[i].wasconst);
#endif
}

static void ujump_assemble(int i, const struct regstat *i_regs);
static void rjump_assemble(int i, const struct regstat *i_regs);
static void cjump_assemble(int i, const struct regstat *i_regs);
static void sjump_assemble(int i, const struct regstat *i_regs);

static int assemble(int i, const struct regstat *i_regs, int ccadj_)
{
  int ds = 0;
  switch (dops[i].itype) {
    case ALU:
      alu_assemble(i, i_regs, ccadj_);
      break;
    case IMM16:
      imm16_assemble(i, i_regs, ccadj_);
      break;
    case SHIFT:
      shift_assemble(i, i_regs);
      break;
    case SHIFTIMM:
      shiftimm_assemble(i, i_regs);
      break;
    case LOAD:
      load_assemble(i, i_regs, ccadj_);
      break;
    case LOADLR:
      loadlr_assemble(i, i_regs, ccadj_);
      break;
    case STORE:
      store_assemble(i, i_regs, ccadj_);
      break;
    case STORELR:
      storelr_assemble(i, i_regs, ccadj_);
      break;
    case COP0:
      cop0_assemble(i, i_regs, ccadj_);
      break;
    case RFE:
      rfe_assemble(i, i_regs);
      break;
    case COP2:
      cop2_assemble(i, i_regs);
      break;
    case C2LS:
      c2ls_assemble(i, i_regs, ccadj_);
      break;
    case C2OP:
      c2op_assemble(i, i_regs);
      break;
    case MULTDIV:
      multdiv_assemble(i, i_regs);
      multdiv_prepare_stall(i, i_regs, ccadj_);
      break;
    case MOV:
      mov_assemble(i, i_regs);
      break;
    case SYSCALL:
      exception_assemble(i, i_regs, ccadj_);
      break;
    case HLECALL:
      hlecall_assemble(i, i_regs, ccadj_);
      break;
    case INTCALL:
      intcall_assemble(i, i_regs, ccadj_);
      break;
    case UJUMP:
      ujump_assemble(i, i_regs);
      ds = 1;
      break;
    case RJUMP:
      rjump_assemble(i, i_regs);
      ds = 1;
      break;
    case CJUMP:
      cjump_assemble(i, i_regs);
      ds = 1;
      break;
    case SJUMP:
      sjump_assemble(i, i_regs);
      ds = 1;
      break;
    case NOP:
    case OTHER:
      // not handled, just skip
      break;
    default:
      assert(0);
  }
  return ds;
}

static void ds_assemble(int i, const struct regstat *i_regs)
{
  speculate_register_values(i);
  is_delayslot = 1;
  switch (dops[i].itype) {
    case SYSCALL:
    case HLECALL:
    case INTCALL:
    case UJUMP:
    case RJUMP:
    case CJUMP:
    case SJUMP:
      SysPrintf("Jump in the delay slot.  This is probably a bug.\n");
      break;
    default:
      assemble(i, i_regs, cinfo[i].ccadj);
  }
  is_delayslot = 0;
}

// Is the branch target a valid internal jump?
static int internal_branch(int addr)
{
  if(addr&1) return 0; // Indirect (register) jump
  if(addr>=start && addr<start+slen*4-4)
  {
    return 1;
  }
  return 0;
}

static void wb_invalidate(signed char pre[],signed char entry[],uint64_t dirty,uint64_t u)
{
  int hr;
  for(hr=0;hr<HOST_REGS;hr++) {
    if(hr!=EXCLUDE_REG) {
      if(pre[hr]!=entry[hr]) {
        if(pre[hr]>=0) {
          if((dirty>>hr)&1) {
            if(get_reg(entry,pre[hr])<0) {
              assert(pre[hr]<64);
              if(!((u>>pre[hr])&1))
                emit_storereg(pre[hr],hr);
            }
          }
        }
      }
    }
  }
  // Move from one register to another (no writeback)
  for(hr=0;hr<HOST_REGS;hr++) {
    if(hr!=EXCLUDE_REG) {
      if(pre[hr]!=entry[hr]) {
        if(pre[hr]>=0&&pre[hr]<TEMPREG) {
          int nr;
          if((nr=get_reg(entry,pre[hr]))>=0) {
            emit_mov(hr,nr);
          }
        }
      }
    }
  }
}

// Load the specified registers
// This only loads the registers given as arguments because
// we don't want to load things that will be overwritten
static inline void load_reg(signed char entry[], signed char regmap[], int rs)
{
  int hr = get_reg(regmap, rs);
  if (hr >= 0 && entry[hr] != regmap[hr])
    emit_loadreg(regmap[hr], hr);
}

static void load_regs(signed char entry[], signed char regmap[], int rs1, int rs2)
{
  load_reg(entry, regmap, rs1);
  if (rs1 != rs2)
    load_reg(entry, regmap, rs2);
}

// Load registers prior to the start of a loop
// so that they are not loaded within the loop
static void loop_preload(signed char pre[],signed char entry[])
{
  int hr;
  for (hr = 0; hr < HOST_REGS; hr++) {
    int r = entry[hr];
    if (r >= 0 && pre[hr] != r && get_reg(pre, r) < 0) {
      assem_debug("loop preload:\n");
      if (r < TEMPREG)
        emit_loadreg(r, hr);
    }
  }
}

// Generate address for load/store instruction
// goes to AGEN (or temp) for writes, FTEMP for LOADLR and cop1/2 loads
// AGEN is assigned by pass5b_preallocate2
static void address_generation(int i, const struct regstat *i_regs, signed char entry[])
{
  if (dops[i].is_load || dops[i].is_store) {
    int ra = -1;
    int agr = AGEN1 + (i&1);
    if(dops[i].itype==LOAD) {
      if (!dops[i].may_except)
        ra = get_reg_w(i_regs->regmap, dops[i].rt1); // reuse dest for agen
      if (ra < 0)
        ra = get_reg_temp(i_regs->regmap);
    }
    if(dops[i].itype==LOADLR) {
      ra=get_reg(i_regs->regmap,FTEMP);
    }
    if(dops[i].itype==STORE||dops[i].itype==STORELR) {
      ra=get_reg(i_regs->regmap,agr);
      if(ra<0) ra=get_reg_temp(i_regs->regmap);
    }
    if(dops[i].itype==C2LS) {
      if (dops[i].opcode == 0x32) // LWC2
        ra=get_reg(i_regs->regmap,FTEMP);
      else { // SWC2
        ra=get_reg(i_regs->regmap,agr);
        if(ra<0) ra=get_reg_temp(i_regs->regmap);
      }
    }
    int rs = get_reg(i_regs->regmap, dops[i].rs1);
    //if(ra>=0)
    {
      int offset = cinfo[i].imm;
      int add_offset = offset != 0;
      int c = rs >= 0 && ((i_regs->wasconst >> rs) & 1);
      if(dops[i].rs1==0) {
        // Using r0 as a base address
        assert(ra >= 0);
        if(!entry||entry[ra]!=agr) {
          if (dops[i].opcode==0x22||dops[i].opcode==0x26) {
            emit_movimm(offset&0xFFFFFFFC,ra); // LWL/LWR
          }else{
            emit_movimm(offset,ra);
          }
        } // else did it in the previous cycle
        cinfo[i].addr = ra;
        add_offset = 0;
      }
      else if (rs < 0) {
        assert(ra >= 0);
        if (!entry || entry[ra] != dops[i].rs1)
          emit_loadreg(dops[i].rs1, ra);
        cinfo[i].addr = ra;
        //if(!entry||entry[ra]!=dops[i].rs1)
        //  printf("poor load scheduling!\n");
      }
      else if(c) {
        if(dops[i].rs1!=dops[i].rt1||dops[i].itype!=LOAD) {
          assert(ra >= 0);
          if(!entry||entry[ra]!=agr) {
            if (dops[i].opcode==0x22||dops[i].opcode==0x26) {
              emit_movimm((constmap[i][rs]+offset)&0xFFFFFFFC,ra); // LWL/LWR
            }else{
              emit_movimm(constmap[i][rs]+offset,ra);
              regs[i].loadedconst|=1<<ra;
            }
          } // else did it in the previous cycle
          cinfo[i].addr = ra;
        }
        else // else load_consts already did it
          cinfo[i].addr = rs;
        add_offset = 0;
      }
      else
        cinfo[i].addr = rs;
      if (add_offset) {
        assert(ra >= 0);
        if(rs>=0) {
          emit_addimm(rs,offset,ra);
        }else{
          emit_addimm(ra,offset,ra);
        }
        cinfo[i].addr = ra;
      }
    }
    assert(cinfo[i].addr >= 0);
  }
  // Preload constants for next instruction
  if (dops[i+1].is_load || dops[i+1].is_store) {
    int agr,ra;
    // Actual address
    agr=AGEN1+((i+1)&1);
    ra=get_reg(i_regs->regmap,agr);
    if(ra>=0) {
      int rs=get_reg(regs[i+1].regmap,dops[i+1].rs1);
      int offset=cinfo[i+1].imm;
      int c=(regs[i+1].wasconst>>rs)&1;
      if(c&&(dops[i+1].rs1!=dops[i+1].rt1||dops[i+1].itype!=LOAD)) {
        if (dops[i+1].opcode==0x22||dops[i+1].opcode==0x26) {
          emit_movimm((constmap[i+1][rs]+offset)&0xFFFFFFFC,ra); // LWL/LWR
        }else if (dops[i+1].opcode==0x1a||dops[i+1].opcode==0x1b) {
          emit_movimm((constmap[i+1][rs]+offset)&0xFFFFFFF8,ra); // LDL/LDR
        }else{
          emit_movimm(constmap[i+1][rs]+offset,ra);
          regs[i+1].loadedconst|=1<<ra;
        }
      }
      else if(dops[i+1].rs1==0) {
        // Using r0 as a base address
        if (dops[i+1].opcode==0x22||dops[i+1].opcode==0x26) {
          emit_movimm(offset&0xFFFFFFFC,ra); // LWL/LWR
        }else if (dops[i+1].opcode==0x1a||dops[i+1].opcode==0x1b) {
          emit_movimm(offset&0xFFFFFFF8,ra); // LDL/LDR
        }else{
          emit_movimm(offset,ra);
        }
      }
    }
  }
}

static int get_final_value(int hr, int i, u_int *value)
{
  int reg=regs[i].regmap[hr];
  while(i<slen-1) {
    if(regs[i+1].regmap[hr]!=reg) break;
    if(!((regs[i+1].isconst>>hr)&1)) break;
    if(dops[i+1].bt) break;
    i++;
  }
  if(i<slen-1) {
    if (dops[i].is_jump) {
      *value=constmap[i][hr];
      return 1;
    }
    if(!dops[i+1].bt) {
      if (dops[i+1].is_jump) {
        // Load in delay slot, out-of-order execution
        if(dops[i+2].itype==LOAD&&dops[i+2].rs1==reg&&dops[i+2].rt1==reg&&((regs[i+1].wasconst>>hr)&1))
        {
          // Precompute load address
          *value=constmap[i][hr]+cinfo[i+2].imm;
          return 1;
        }
      }
      if(dops[i+1].itype==LOAD&&dops[i+1].rs1==reg&&dops[i+1].rt1==reg)
      {
        // Precompute load address
        *value=constmap[i][hr]+cinfo[i+1].imm;
        //printf("c=%x imm=%lx\n",(long)constmap[i][hr],cinfo[i+1].imm);
        return 1;
      }
    }
  }
  *value=constmap[i][hr];
  //printf("c=%lx\n",(long)constmap[i][hr]);
  if(i==slen-1) return 1;
  assert(reg < 64);
  return !((unneeded_reg[i+1]>>reg)&1);
}

// Load registers with known constants
static void load_consts(signed char pre[],signed char regmap[],int i)
{
  int hr,hr2;
  // propagate loaded constant flags
  if(i==0||dops[i].bt)
    regs[i].loadedconst=0;
  else {
    for (hr = 0; hr < HOST_REGS; hr++) {
      if (hr == EXCLUDE_REG || regmap[hr] < 0 || pre[hr] != regmap[hr])
        continue;
      if ((((regs[i-1].isconst & regs[i-1].loadedconst) >> hr) & 1)
          && regmap[hr] == regs[i-1].regmap[hr])
      {
        regs[i].loadedconst |= 1u << hr;
      }
    }
  }
  // Load 32-bit regs
  for(hr=0;hr<HOST_REGS;hr++) {
    if(hr!=EXCLUDE_REG&&regmap[hr]>=0) {
      //if(entry[hr]!=regmap[hr]) {
      if(!((regs[i].loadedconst>>hr)&1)) {
        assert(regmap[hr]<64);
        if(((regs[i].isconst>>hr)&1)&&regmap[hr]>0) {
          u_int value, similar=0;
          if(get_final_value(hr,i,&value)) {
            // see if some other register has similar value
            for(hr2=0;hr2<HOST_REGS;hr2++) {
              if(hr2!=EXCLUDE_REG&&((regs[i].loadedconst>>hr2)&1)) {
                if(is_similar_value(value,constmap[i][hr2])) {
                  similar=1;
                  break;
                }
              }
            }
            if(similar) {
              u_int value2;
              if(get_final_value(hr2,i,&value2)) // is this needed?
                emit_movimm_from(value2,hr2,value,hr);
              else
                emit_movimm(value,hr);
            }
            else if(value==0) {
              emit_zeroreg(hr);
            }
            else {
              emit_movimm(value,hr);
            }
          }
          regs[i].loadedconst|=1<<hr;
        }
      }
    }
  }
}

static void load_all_consts(const signed char regmap[], u_int dirty, int i)
{
  int hr;
  // Load 32-bit regs
  for(hr=0;hr<HOST_REGS;hr++) {
    if(hr!=EXCLUDE_REG&&regmap[hr]>=0&&((dirty>>hr)&1)) {
      assert(regmap[hr] < 64);
      if(((regs[i].isconst>>hr)&1)&&regmap[hr]>0) {
        int value=constmap[i][hr];
        if(value==0) {
          emit_zeroreg(hr);
        }
        else {
          emit_movimm(value,hr);
        }
      }
    }
  }
}

// Write out all dirty registers (except cycle count)
#ifndef wb_dirtys
static void wb_dirtys(const signed char i_regmap[], u_int i_dirty)
{
  int hr;
  for(hr=0;hr<HOST_REGS;hr++) {
    if(hr!=EXCLUDE_REG) {
      if(i_regmap[hr]>0) {
        if(i_regmap[hr]!=CCREG) {
          if((i_dirty>>hr)&1) {
            assert(i_regmap[hr]<64);
            emit_storereg(i_regmap[hr],hr);
          }
        }
      }
    }
  }
}
#endif

// Write out dirty registers that we need to reload (pair with load_needed_regs)
// This writes the registers not written by store_regs_bt
static void wb_needed_dirtys(const signed char i_regmap[], u_int i_dirty, int addr)
{
  int hr;
  int t=(addr-start)>>2;
  for(hr=0;hr<HOST_REGS;hr++) {
    if(hr!=EXCLUDE_REG) {
      if(i_regmap[hr]>0) {
        if(i_regmap[hr]!=CCREG) {
          if(i_regmap[hr]==regs[t].regmap_entry[hr] && ((regs[t].dirty>>hr)&1)) {
            if((i_dirty>>hr)&1) {
              assert(i_regmap[hr]<64);
              emit_storereg(i_regmap[hr],hr);
            }
          }
        }
      }
    }
  }
}

// Load all registers (except cycle count)
#ifndef load_all_regs
static void load_all_regs(const signed char i_regmap[])
{
  int hr;
  for(hr=0;hr<HOST_REGS;hr++) {
    if(hr!=EXCLUDE_REG) {
      if(i_regmap[hr]==0) {
        emit_zeroreg(hr);
      }
      else
      if(i_regmap[hr]>0 && i_regmap[hr]<TEMPREG && i_regmap[hr]!=CCREG)
      {
        emit_loadreg(i_regmap[hr],hr);
      }
    }
  }
}
#endif

// Load all current registers also needed by next instruction
static void load_needed_regs(const signed char i_regmap[], const signed char next_regmap[])
{
  signed char regmap_sel[HOST_REGS];
  int hr;
  for (hr = 0; hr < HOST_REGS; hr++) {
    regmap_sel[hr] = -1;
    if (hr != EXCLUDE_REG)
      if (next_regmap[hr] == i_regmap[hr] || get_reg(next_regmap, i_regmap[hr]) >= 0)
        regmap_sel[hr] = i_regmap[hr];
  }
  load_all_regs(regmap_sel);
}

// Load all regs, storing cycle count if necessary
static void load_regs_entry(int t)
{
  if(dops[t].is_ds) emit_addimm(HOST_CCREG,CLOCK_ADJUST(1),HOST_CCREG);
  else if(cinfo[t].ccadj) emit_addimm(HOST_CCREG,-cinfo[t].ccadj,HOST_CCREG);
  if(regs[t].regmap_entry[HOST_CCREG]!=CCREG) {
    emit_storereg(CCREG,HOST_CCREG);
  }
  load_all_regs(regs[t].regmap_entry);
}

// Store dirty registers prior to branch
static void store_regs_bt(signed char i_regmap[],uint64_t i_dirty,int addr)
{
  if(internal_branch(addr))
  {
    int t=(addr-start)>>2;
    int hr;
    for(hr=0;hr<HOST_REGS;hr++) {
      if(hr!=EXCLUDE_REG) {
        if(i_regmap[hr]>0 && i_regmap[hr]!=CCREG) {
          if(i_regmap[hr]!=regs[t].regmap_entry[hr] || !((regs[t].dirty>>hr)&1)) {
            if((i_dirty>>hr)&1) {
              assert(i_regmap[hr]<64);
              if(!((unneeded_reg[t]>>i_regmap[hr])&1))
                emit_storereg(i_regmap[hr],hr);
            }
          }
        }
      }
    }
  }
  else
  {
    // Branch out of this block, write out all dirty regs
    wb_dirtys(i_regmap,i_dirty);
  }
}

// Load all needed registers for branch target
static void load_regs_bt(signed char i_regmap[],uint64_t i_dirty,int addr)
{
  //if(addr>=start && addr<(start+slen*4))
  if(internal_branch(addr))
  {
    int t=(addr-start)>>2;
    int hr;
    // Store the cycle count before loading something else
    if(i_regmap[HOST_CCREG]!=CCREG) {
      assert(i_regmap[HOST_CCREG]==-1);
    }
    if(regs[t].regmap_entry[HOST_CCREG]!=CCREG) {
      emit_storereg(CCREG,HOST_CCREG);
    }
    // Load 32-bit regs
    for(hr=0;hr<HOST_REGS;hr++) {
      if(hr!=EXCLUDE_REG&&regs[t].regmap_entry[hr]>=0&&regs[t].regmap_entry[hr]<TEMPREG) {
        if(i_regmap[hr]!=regs[t].regmap_entry[hr]) {
          if(regs[t].regmap_entry[hr]==0) {
            emit_zeroreg(hr);
          }
          else if(regs[t].regmap_entry[hr]!=CCREG)
          {
            emit_loadreg(regs[t].regmap_entry[hr],hr);
          }
        }
      }
    }
  }
}

static int match_bt(signed char i_regmap[],uint64_t i_dirty,int addr)
{
  if(addr>=start && addr<start+slen*4-4)
  {
    int t=(addr-start)>>2;
    int hr;
    if(regs[t].regmap_entry[HOST_CCREG]!=CCREG) return 0;
    for(hr=0;hr<HOST_REGS;hr++)
    {
      if(hr!=EXCLUDE_REG)
      {
        if(i_regmap[hr]!=regs[t].regmap_entry[hr])
        {
          if(regs[t].regmap_entry[hr]>=0&&(regs[t].regmap_entry[hr]|64)<TEMPREG+64)
          {
            return 0;
          }
          else
          if((i_dirty>>hr)&1)
          {
            if(i_regmap[hr]<TEMPREG)
            {
              if(!((unneeded_reg[t]>>i_regmap[hr])&1))
                return 0;
            }
            else if(i_regmap[hr]>=64&&i_regmap[hr]<TEMPREG+64)
            {
              assert(0);
            }
          }
        }
        else // Same register but is it 32-bit or dirty?
        if(i_regmap[hr]>=0)
        {
          if(!((regs[t].dirty>>hr)&1))
          {
            if((i_dirty>>hr)&1)
            {
              if(!((unneeded_reg[t]>>i_regmap[hr])&1))
              {
                //printf("%x: dirty no match\n",addr);
                return 0;
              }
            }
          }
        }
      }
    }
    // Delay slots are not valid branch targets
    //if(t>0&&(dops[t-1].is_jump) return 0;
    // Delay slots require additional processing, so do not match
    if(dops[t].is_ds) return 0;
  }
  else
  {
    int hr;
    for(hr=0;hr<HOST_REGS;hr++)
    {
      if(hr!=EXCLUDE_REG)
      {
        if(i_regmap[hr]>=0)
        {
          if(hr!=HOST_CCREG||i_regmap[hr]!=CCREG)
          {
            if((i_dirty>>hr)&1)
            {
              return 0;
            }
          }
        }
      }
    }
  }
  return 1;
}

#ifdef DRC_DBG
static void drc_dbg_emit_do_cmp(int i, int ccadj_)
{
  extern void do_insn_cmp();
  //extern int cycle;
  u_int hr, reglist = get_host_reglist(regs[i].regmap);
  reglist |= get_host_reglist(regs[i].regmap_entry);
  reglist &= DRC_DBG_REGMASK;

  assem_debug("//do_insn_cmp %08x\n", start+i*4);
  save_regs(reglist);
  // write out changed consts to match the interpreter
  if (i > 0 && !dops[i].bt) {
    for (hr = 0; hr < HOST_REGS; hr++) {
      int reg = regs[i].regmap_entry[hr]; // regs[i-1].regmap[hr];
      if (hr == EXCLUDE_REG || reg <= 0)
        continue;
      if (!((regs[i-1].isconst >> hr) & 1))
        continue;
      if (i > 1 && reg == regs[i-2].regmap[hr] && constmap[i-1][hr] == constmap[i-2][hr])
        continue;
      emit_movimm(constmap[i-1][hr],0);
      emit_storereg(reg, 0);
    }
  }
  if (dops[i].opcode == 0x0f) { // LUI
    emit_movimm(cinfo[i].imm << 16, 0);
    emit_storereg(dops[i].rt1, 0);
  }
  emit_movimm(start+i*4,0);
  emit_writeword(0,&psxRegs.pc);
  int cc = get_reg(regs[i].regmap_entry, CCREG);
  if (cc < 0)
    emit_loadreg(CCREG, cc = 0);
  emit_addimm(cc, ccadj_, 0);
  emit_writeword(0, &psxRegs.cycle);
  emit_far_call(do_insn_cmp);
  //emit_readword(&cycle,0);
  //emit_addimm(0,2,0);
  //emit_writeword(0,&cycle);
  (void)get_reg2;
  restore_regs(reglist);
  assem_debug("\\\\do_insn_cmp\n");
}
static void drc_dbg_emit_wb_dirtys(int i, const struct regstat *i_regs)
{
  // write-out non-consts, consts are likely different because of get_final_value()
  if (i_regs->dirty & ~i_regs->loadedconst) {
    assem_debug("/ drc_dbg_wb\n");
    wb_dirtys(i_regs->regmap, i_regs->dirty & ~i_regs->loadedconst);
    assem_debug("\\ drc_dbg_wb\n");
  }
}
#else
#define drc_dbg_emit_do_cmp(x,y)
#define drc_dbg_emit_wb_dirtys(x,y)
#endif

// Used when a branch jumps into the delay slot of another branch
static void ds_assemble_entry(int i)
{
  int t = (cinfo[i].ba - start) >> 2;
  int ccadj_ = -CLOCK_ADJUST(1);
  if (!instr_addr[t])
    instr_addr[t] = out;
  assem_debug("Assemble delay slot at %x\n",cinfo[i].ba);
  assem_debug("<->\n");
  drc_dbg_emit_do_cmp(t, ccadj_);
  if(regs[t].regmap_entry[HOST_CCREG]==CCREG&&regs[t].regmap[HOST_CCREG]!=CCREG)
    wb_register(CCREG,regs[t].regmap_entry,regs[t].wasdirty);
  load_regs(regs[t].regmap_entry,regs[t].regmap,dops[t].rs1,dops[t].rs2);
  address_generation(t,&regs[t],regs[t].regmap_entry);
  if (ram_offset && (dops[t].is_load || dops[t].is_store))
    load_reg(regs[t].regmap_entry,regs[t].regmap,ROREG);
  if (dops[t].is_store)
    load_reg(regs[t].regmap_entry,regs[t].regmap,INVCP);
  is_delayslot=0;
  switch (dops[t].itype) {
    case SYSCALL:
    case HLECALL:
    case INTCALL:
    case UJUMP:
    case RJUMP:
    case CJUMP:
    case SJUMP:
      SysPrintf("Jump in the delay slot.  This is probably a bug.\n");
      break;
    default:
      assemble(t, &regs[t], ccadj_);
  }
  store_regs_bt(regs[t].regmap,regs[t].dirty,cinfo[i].ba+4);
  load_regs_bt(regs[t].regmap,regs[t].dirty,cinfo[i].ba+4);
  if(internal_branch(cinfo[i].ba+4))
    assem_debug("branch: internal\n");
  else
    assem_debug("branch: external\n");
  assert(internal_branch(cinfo[i].ba+4));
  add_to_linker(out,cinfo[i].ba+4,internal_branch(cinfo[i].ba+4));
  emit_jmp(0);
}

// Load 2 immediates optimizing for small code size
static void emit_mov2imm_compact(int imm1,u_int rt1,int imm2,u_int rt2)
{
  emit_movimm(imm1,rt1);
  emit_movimm_from(imm1,rt1,imm2,rt2);
}

static void do_cc(int i, const signed char i_regmap[], int *adj,
  int addr, int taken, int invert)
{
  int count, count_plus2;
  void *jaddr;
  void *idle=NULL;
  int t=0;
  if(dops[i].itype==RJUMP)
  {
    *adj=0;
  }
  //if(cinfo[i].ba>=start && cinfo[i].ba<(start+slen*4))
  if(internal_branch(cinfo[i].ba))
  {
    t=(cinfo[i].ba-start)>>2;
    if(dops[t].is_ds) *adj=-CLOCK_ADJUST(1); // Branch into delay slot adds an extra cycle
    else *adj=cinfo[t].ccadj;
  }
  else
  {
    *adj=0;
  }
  count = cinfo[i].ccadj;
  count_plus2 = count + CLOCK_ADJUST(2);
  if(taken==TAKEN && i==(cinfo[i].ba-start)>>2 && source[i+1]==0) {
    // Idle loop
    if(count&1) emit_addimm_and_set_flags(2*(count+2),HOST_CCREG);
    idle=out;
    //emit_subfrommem(&idlecount,HOST_CCREG); // Count idle cycles
    emit_andimm(HOST_CCREG,3,HOST_CCREG);
    jaddr=out;
    emit_jmp(0);
  }
  else if(*adj==0||invert) {
    int cycles = count_plus2;
    // faster loop HACK
#if 0
    if (t&&*adj) {
      int rel=t-i;
      if(-NO_CYCLE_PENALTY_THR<rel&&rel<0)
        cycles=*adj+count+2-*adj;
    }
#endif
    emit_addimm_and_set_flags(cycles, HOST_CCREG);
    jaddr = out;
    emit_jns(0);
  }
  else
  {
    emit_cmpimm(HOST_CCREG, -count_plus2);
    jaddr = out;
    emit_jns(0);
  }
  add_stub(CC_STUB,jaddr,idle?idle:out,(*adj==0||invert||idle)?0:count_plus2,i,addr,taken,0);
}

static void do_ccstub(int n)
{
  literal_pool(256);
  assem_debug("do_ccstub %x\n",start+(u_int)stubs[n].b*4);
  set_jump_target(stubs[n].addr, out);
  int i = stubs[n].b;
  int r_pc = -1;
  if (stubs[n].d != TAKEN) {
    wb_dirtys(branch_regs[i].regmap,branch_regs[i].dirty);
  }
  else {
    if(internal_branch(cinfo[i].ba))
      wb_needed_dirtys(branch_regs[i].regmap,branch_regs[i].dirty,cinfo[i].ba);
  }
  if(stubs[n].c!=-1)
  {
    // Save PC as return address
    emit_movimm(stubs[n].c, (r_pc = 0));
  }
  else
  {
    // Return address depends on which way the branch goes
    if(dops[i].itype==CJUMP||dops[i].itype==SJUMP)
    {
      int s1l=get_reg(branch_regs[i].regmap,dops[i].rs1);
      int s2l=get_reg(branch_regs[i].regmap,dops[i].rs2);
      if(dops[i].rs1==0)
      {
        s1l=s2l;
        s2l=-1;
      }
      else if(dops[i].rs2==0)
      {
        s2l=-1;
      }
      assert(s1l>=0);
      #ifdef DESTRUCTIVE_WRITEBACK
      if(dops[i].rs1) {
        if((branch_regs[i].dirty>>s1l)&&1)
          emit_loadreg(dops[i].rs1,s1l);
      }
      else {
        if((branch_regs[i].dirty>>s1l)&1)
          emit_loadreg(dops[i].rs2,s1l);
      }
      if(s2l>=0)
        if((branch_regs[i].dirty>>s2l)&1)
          emit_loadreg(dops[i].rs2,s2l);
      #endif
      int hr=0;
      int addr=-1,alt=-1,ntaddr=-1;
      while(hr<HOST_REGS)
      {
        if(hr!=EXCLUDE_REG && hr!=HOST_CCREG &&
           branch_regs[i].regmap[hr]!=dops[i].rs1 &&
           branch_regs[i].regmap[hr]!=dops[i].rs2 )
        {
          addr=hr++;break;
        }
        hr++;
      }
      while(hr<HOST_REGS)
      {
        if(hr!=EXCLUDE_REG && hr!=HOST_CCREG &&
           branch_regs[i].regmap[hr]!=dops[i].rs1 &&
           branch_regs[i].regmap[hr]!=dops[i].rs2 )
        {
          alt=hr++;break;
        }
        hr++;
      }
      if ((dops[i].opcode & 0x3e) == 6) // BLEZ/BGTZ needs another register
      {
        while(hr<HOST_REGS)
        {
          if(hr!=EXCLUDE_REG && hr!=HOST_CCREG &&
             branch_regs[i].regmap[hr]!=dops[i].rs1 &&
             branch_regs[i].regmap[hr]!=dops[i].rs2 )
          {
            ntaddr=hr;break;
          }
          hr++;
        }
        assert(hr<HOST_REGS);
      }
      if (dops[i].opcode == 4) // BEQ
      {
        #ifdef HAVE_CMOV_IMM
        if(s2l>=0) emit_cmp(s1l,s2l);
        else emit_test(s1l,s1l);
        emit_cmov2imm_e_ne_compact(cinfo[i].ba,start+i*4+8,addr);
        #else
        emit_mov2imm_compact(cinfo[i].ba,addr,start+i*4+8,alt);
        if(s2l>=0) emit_cmp(s1l,s2l);
        else emit_test(s1l,s1l);
        emit_cmovne_reg(alt,addr);
        #endif
      }
      else if (dops[i].opcode == 5) // BNE
      {
        #ifdef HAVE_CMOV_IMM
        if(s2l>=0) emit_cmp(s1l,s2l);
        else emit_test(s1l,s1l);
        emit_cmov2imm_e_ne_compact(start+i*4+8,cinfo[i].ba,addr);
        #else
        emit_mov2imm_compact(start+i*4+8,addr,cinfo[i].ba,alt);
        if(s2l>=0) emit_cmp(s1l,s2l);
        else emit_test(s1l,s1l);
        emit_cmovne_reg(alt,addr);
        #endif
      }
      else if (dops[i].opcode == 6) // BLEZ
      {
        //emit_movimm(cinfo[i].ba,alt);
        //emit_movimm(start+i*4+8,addr);
        emit_mov2imm_compact(cinfo[i].ba,alt,start+i*4+8,addr);
        emit_cmpimm(s1l,1);
        emit_cmovl_reg(alt,addr);
      }
      else if (dops[i].opcode == 7) // BGTZ
      {
        //emit_movimm(cinfo[i].ba,addr);
        //emit_movimm(start+i*4+8,ntaddr);
        emit_mov2imm_compact(cinfo[i].ba,addr,start+i*4+8,ntaddr);
        emit_cmpimm(s1l,1);
        emit_cmovl_reg(ntaddr,addr);
      }
      else if (dops[i].itype == SJUMP) // BLTZ/BGEZ
      {
        //emit_movimm(cinfo[i].ba,alt);
        //emit_movimm(start+i*4+8,addr);
        if (dops[i].rs1) {
          emit_mov2imm_compact(cinfo[i].ba,
            (dops[i].opcode2 & 1) ? addr : alt, start + i*4 + 8,
            (dops[i].opcode2 & 1) ? alt : addr);
          emit_test(s1l,s1l);
          emit_cmovs_reg(alt,addr);
        }
        else
          emit_movimm((dops[i].opcode2 & 1) ? cinfo[i].ba : start + i*4 + 8, addr);
      }
      r_pc = addr;
    }
    else
    if(dops[i].itype==RJUMP)
    {
      r_pc = get_reg(branch_regs[i].regmap, dops[i].rs1);
      if (ds_writes_rjump_rs(i)) {
        r_pc = get_reg(branch_regs[i].regmap, RTEMP);
      }
    }
    else {SysPrintf("Unknown branch type in do_ccstub\n");abort();}
  }
  emit_writeword(r_pc, &psxRegs.pc);
  // Update cycle count
  assert(branch_regs[i].regmap[HOST_CCREG]==CCREG||branch_regs[i].regmap[HOST_CCREG]==-1);
  if(stubs[n].a) emit_addimm(HOST_CCREG,(int)stubs[n].a,HOST_CCREG);
  emit_far_call(cc_interrupt);
  if(stubs[n].a) emit_addimm(HOST_CCREG,-(int)stubs[n].a,HOST_CCREG);
  if(stubs[n].d==TAKEN) {
    if(internal_branch(cinfo[i].ba))
      load_needed_regs(branch_regs[i].regmap,regs[(cinfo[i].ba-start)>>2].regmap_entry);
    else if(dops[i].itype==RJUMP) {
      if(get_reg(branch_regs[i].regmap,RTEMP)>=0)
        emit_readword(&psxRegs.pc,get_reg(branch_regs[i].regmap,RTEMP));
      else
        emit_loadreg(dops[i].rs1,get_reg(branch_regs[i].regmap,dops[i].rs1));
    }
  }else if(stubs[n].d==NOTTAKEN) {
    if(i<slen-2) load_needed_regs(branch_regs[i].regmap,regmap_pre[i+2]);
    else load_all_regs(branch_regs[i].regmap);
  }else{
    load_all_regs(branch_regs[i].regmap);
  }
  if (stubs[n].retaddr)
    emit_jmp(stubs[n].retaddr);
  else
    do_jump_vaddr(stubs[n].e);
}

static void add_to_linker(void *addr, u_int target, int is_internal)
{
  assert(linkcount < ARRAY_SIZE(link_addr));
  link_addr[linkcount].addr = addr;
  link_addr[linkcount].target = target;
  link_addr[linkcount].internal = is_internal;
  linkcount++;
}

static void ujump_assemble_write_ra(int i)
{
  int rt;
  unsigned int return_address;
  rt=get_reg(branch_regs[i].regmap,31);
  //assem_debug("branch(%d): eax=%d ecx=%d edx=%d ebx=%d ebp=%d esi=%d edi=%d\n",i,branch_regs[i].regmap[0],branch_regs[i].regmap[1],branch_regs[i].regmap[2],branch_regs[i].regmap[3],branch_regs[i].regmap[5],branch_regs[i].regmap[6],branch_regs[i].regmap[7]);
  //assert(rt>=0);
  return_address=start+i*4+8;
  if(rt>=0) {
    #ifdef USE_MINI_HT
    if(internal_branch(return_address)&&dops[i+1].rt1!=31) {
      int temp=-1; // note: must be ds-safe
      #ifdef HOST_TEMPREG
      temp=HOST_TEMPREG;
      #endif
      if(temp>=0) do_miniht_insert(return_address,rt,temp);
      else emit_movimm(return_address,rt);
    }
    else
    #endif
    {
      #ifdef REG_PREFETCH
      if(temp>=0)
      {
        if(i_regmap[temp]!=PTEMP) emit_movimm((uintptr_t)hash_table_get(return_address),temp);
      }
      #endif
      if (!((regs[i].loadedconst >> rt) & 1))
        emit_movimm(return_address, rt); // PC into link register
      #ifdef IMM_PREFETCH
      emit_prefetch(hash_table_get(return_address));
      #endif
    }
  }
}

static void ujump_assemble(int i, const struct regstat *i_regs)
{
  if(i==(cinfo[i].ba-start)>>2) assem_debug("idle loop\n");
  address_generation(i+1,i_regs,regs[i].regmap_entry);
  #ifdef REG_PREFETCH
  int temp=get_reg(branch_regs[i].regmap,PTEMP);
  if(dops[i].rt1==31&&temp>=0)
  {
    signed char *i_regmap=i_regs->regmap;
    int return_address=start+i*4+8;
    if(get_reg(branch_regs[i].regmap,31)>0)
    if(i_regmap[temp]==PTEMP) emit_movimm((uintptr_t)hash_table_get(return_address),temp);
  }
  #endif
  if (dops[i].rt1 == 31)
    ujump_assemble_write_ra(i); // writeback ra for DS
  ds_assemble(i+1,i_regs);
  uint64_t bc_unneeded=branch_regs[i].u;
  bc_unneeded|=1|(1LL<<dops[i].rt1);
  wb_invalidate(regs[i].regmap,branch_regs[i].regmap,regs[i].dirty,bc_unneeded);
  load_reg(regs[i].regmap,branch_regs[i].regmap,CCREG);
  int cc,adj;
  cc=get_reg(branch_regs[i].regmap,CCREG);
  assert(cc==HOST_CCREG);
  store_regs_bt(branch_regs[i].regmap,branch_regs[i].dirty,cinfo[i].ba);
  #ifdef REG_PREFETCH
  if(dops[i].rt1==31&&temp>=0) emit_prefetchreg(temp);
  #endif
  do_cc(i,branch_regs[i].regmap,&adj,cinfo[i].ba,TAKEN,0);
  if(adj) emit_addimm(cc, cinfo[i].ccadj + CLOCK_ADJUST(2) - adj, cc);
  load_regs_bt(branch_regs[i].regmap,branch_regs[i].dirty,cinfo[i].ba);
  if(internal_branch(cinfo[i].ba))
    assem_debug("branch: internal\n");
  else
    assem_debug("branch: external\n");
  if (internal_branch(cinfo[i].ba) && dops[(cinfo[i].ba-start)>>2].is_ds) {
    ds_assemble_entry(i);
  }
  else {
    add_to_linker(out,cinfo[i].ba,internal_branch(cinfo[i].ba));
    emit_jmp(0);
  }
}

static void rjump_assemble_write_ra(int i)
{
  int rt,return_address;
  rt=get_reg_w(branch_regs[i].regmap, dops[i].rt1);
  //assem_debug("branch(%d): eax=%d ecx=%d edx=%d ebx=%d ebp=%d esi=%d edi=%d\n",i,branch_regs[i].regmap[0],branch_regs[i].regmap[1],branch_regs[i].regmap[2],branch_regs[i].regmap[3],branch_regs[i].regmap[5],branch_regs[i].regmap[6],branch_regs[i].regmap[7]);
  assert(rt>=0);
  return_address=start+i*4+8;
  #ifdef REG_PREFETCH
  if(temp>=0)
  {
    if(i_regmap[temp]!=PTEMP) emit_movimm((uintptr_t)hash_table_get(return_address),temp);
  }
  #endif
  if (!((regs[i].loadedconst >> rt) & 1))
    emit_movimm(return_address, rt); // PC into link register
  #ifdef IMM_PREFETCH
  emit_prefetch(hash_table_get(return_address));
  #endif
}

static void rjump_assemble(int i, const struct regstat *i_regs)
{
  int temp;
  int rs,cc;
  rs=get_reg(branch_regs[i].regmap,dops[i].rs1);
  assert(rs>=0);
  if (ds_writes_rjump_rs(i)) {
    // Delay slot abuse, make a copy of the branch address register
    temp=get_reg(branch_regs[i].regmap,RTEMP);
    assert(temp>=0);
    assert(regs[i].regmap[temp]==RTEMP);
    emit_mov(rs,temp);
    rs=temp;
  }
  address_generation(i+1,i_regs,regs[i].regmap_entry);
  #ifdef REG_PREFETCH
  if(dops[i].rt1==31)
  {
    if((temp=get_reg(branch_regs[i].regmap,PTEMP))>=0) {
      signed char *i_regmap=i_regs->regmap;
      int return_address=start+i*4+8;
      if(i_regmap[temp]==PTEMP) emit_movimm((uintptr_t)hash_table_get(return_address),temp);
    }
  }
  #endif
  #ifdef USE_MINI_HT
  if(dops[i].rs1==31) {
    int rh=get_reg(regs[i].regmap,RHASH);
    if(rh>=0) do_preload_rhash(rh);
  }
  #endif
  if (dops[i].rt1 != 0)
    rjump_assemble_write_ra(i);
  ds_assemble(i+1,i_regs);
  uint64_t bc_unneeded=branch_regs[i].u;
  bc_unneeded|=1|(1LL<<dops[i].rt1);
  bc_unneeded&=~(1LL<<dops[i].rs1);
  wb_invalidate(regs[i].regmap,branch_regs[i].regmap,regs[i].dirty,bc_unneeded);
  load_regs(regs[i].regmap,branch_regs[i].regmap,dops[i].rs1,CCREG);
  cc=get_reg(branch_regs[i].regmap,CCREG);
  assert(cc==HOST_CCREG);
  (void)cc;
  #ifdef USE_MINI_HT
  int rh=get_reg(branch_regs[i].regmap,RHASH);
  int ht=get_reg(branch_regs[i].regmap,RHTBL);
  if(dops[i].rs1==31) {
    if(regs[i].regmap[rh]!=RHASH) do_preload_rhash(rh);
    do_preload_rhtbl(ht);
    do_rhash(rs,rh);
  }
  #endif
  store_regs_bt(branch_regs[i].regmap,branch_regs[i].dirty,-1);
  #ifdef DESTRUCTIVE_WRITEBACK
  if((branch_regs[i].dirty>>rs)&1) {
    if(dops[i].rs1!=dops[i+1].rt1&&dops[i].rs1!=dops[i+1].rt2) {
      emit_loadreg(dops[i].rs1,rs);
    }
  }
  #endif
  #ifdef REG_PREFETCH
  if(dops[i].rt1==31&&temp>=0) emit_prefetchreg(temp);
  #endif
  #ifdef USE_MINI_HT
  if(dops[i].rs1==31) {
    do_miniht_load(ht,rh);
  }
  #endif
  //do_cc(i,branch_regs[i].regmap,&adj,-1,TAKEN);
  //if(adj) emit_addimm(cc,2*(cinfo[i].ccadj+2-adj),cc); // ??? - Shouldn't happen
  //assert(adj==0);
  emit_addimm_and_set_flags(cinfo[i].ccadj + CLOCK_ADJUST(2), HOST_CCREG);
  add_stub(CC_STUB,out,NULL,0,i,-1,TAKEN,rs);
  if (dops[i+1].itype == RFE)
    // special case for RFE
    emit_jmp(0);
  else
    emit_jns(0);
  //load_regs_bt(branch_regs[i].regmap,branch_regs[i].dirty,-1);
  #ifdef USE_MINI_HT
  if(dops[i].rs1==31) {
    do_miniht_jump(rs,rh,ht);
  }
  else
  #endif
  {
    do_jump_vaddr(rs);
  }
  #ifdef CORTEX_A8_BRANCH_PREDICTION_HACK
  if(dops[i].rt1!=31&&i<slen-2&&(((u_int)out)&7)) emit_mov(13,13);
  #endif
}

static void vsync_hack_assemble(int i, int ld_ofs, int cc)
{
  int sp = get_reg(branch_regs[i].regmap, 29);
  int ro = get_reg(branch_regs[i].regmap, ROREG);
  int cycles = CLOCK_ADJUST(9+5) * 16;
  void *t_exit[3], *loop_target, *t_loop_break;
  int j;
  if (sp < 0 || (ram_offset && ro < 0))
    return;
  assem_debug("; vsync hack\n");
  host_tempreg_acquire();
  emit_cmpimm(cc, -cycles);
  t_exit[0] = out;
  emit_jge(0);
  emit_cmpimm(sp, RAM_SIZE);
  t_exit[1] = out;
  emit_jno(0);
  if (ro >= 0) {
    emit_addimm(sp, ld_ofs, HOST_TEMPREG);
    emit_ldr_dualindexed(ro, HOST_TEMPREG, HOST_TEMPREG);
  }
  else
    emit_readword_indexed(ld_ofs, sp, HOST_TEMPREG);
  emit_cmpimm(HOST_TEMPREG, 17);
  t_exit[2] = out;
  emit_jl(0);

  assem_debug("1:\n");
  loop_target = out;
  emit_addimm(HOST_TEMPREG, -16, HOST_TEMPREG);
  emit_addimm(cc, cycles, cc);
  emit_cmpimm(HOST_TEMPREG, 17);
  t_loop_break = out;
  emit_jl(DJT_2);
  emit_cmpimm(cc, -cycles);
  emit_jl(loop_target);

  assem_debug("2:\n");
  set_jump_target(t_loop_break, out);
  do_store_word(sp, ld_ofs, HOST_TEMPREG, ro, 1);

  for (j = 0; j < ARRAY_SIZE(t_exit); j++)
    set_jump_target(t_exit[j], out);
  host_tempreg_release();
}

static void cjump_assemble(int i, const struct regstat *i_regs)
{
  const signed char *i_regmap = i_regs->regmap;
  int cc;
  int match;
  match=match_bt(branch_regs[i].regmap,branch_regs[i].dirty,cinfo[i].ba);
  assem_debug("match=%d\n",match);
  int s1l,s2l;
  int unconditional=0,nop=0;
  int invert=0;
  int internal=internal_branch(cinfo[i].ba);
  if(i==(cinfo[i].ba-start)>>2) assem_debug("idle loop\n");
  if(!match) invert=1;
  if (vsync_hack && (vsync_hack >> 16) == i) invert=1;
  #ifdef CORTEX_A8_BRANCH_PREDICTION_HACK
  if(i>(cinfo[i].ba-start)>>2) invert=1;
  #endif
  #ifdef __aarch64__
  invert=1; // because of near cond. branches
  #endif

  if(dops[i].ooo) {
    s1l=get_reg(branch_regs[i].regmap,dops[i].rs1);
    s2l=get_reg(branch_regs[i].regmap,dops[i].rs2);
  }
  else {
    s1l=get_reg(i_regmap,dops[i].rs1);
    s2l=get_reg(i_regmap,dops[i].rs2);
  }
  if(dops[i].rs1==0&&dops[i].rs2==0)
  {
    if(dops[i].opcode&1) nop=1;
    else unconditional=1;
    //assert(dops[i].opcode!=5);
    //assert(dops[i].opcode!=7);
    //assert(dops[i].opcode!=0x15);
    //assert(dops[i].opcode!=0x17);
  }
  else if(dops[i].rs1==0)
  {
    s1l=s2l;
    s2l=-1;
  }
  else if(dops[i].rs2==0)
  {
    s2l=-1;
  }

  if(dops[i].ooo) {
    // Out of order execution (delay slot first)
    //printf("OOOE\n");
    address_generation(i+1,i_regs,regs[i].regmap_entry);
    ds_assemble(i+1,i_regs);
    int adj;
    uint64_t bc_unneeded=branch_regs[i].u;
    bc_unneeded&=~((1LL<<dops[i].rs1)|(1LL<<dops[i].rs2));
    bc_unneeded|=1;
    wb_invalidate(regs[i].regmap,branch_regs[i].regmap,regs[i].dirty,bc_unneeded);
    load_regs(regs[i].regmap,branch_regs[i].regmap,dops[i].rs1,dops[i].rs2);
    load_reg(regs[i].regmap,branch_regs[i].regmap,CCREG);
    cc=get_reg(branch_regs[i].regmap,CCREG);
    assert(cc==HOST_CCREG);
    if(unconditional)
      store_regs_bt(branch_regs[i].regmap,branch_regs[i].dirty,cinfo[i].ba);
    //do_cc(i,branch_regs[i].regmap,&adj,unconditional?cinfo[i].ba:-1,unconditional);
    //assem_debug("cycle count (adj)\n");
    if(unconditional) {
      do_cc(i,branch_regs[i].regmap,&adj,cinfo[i].ba,TAKEN,0);
      if(i!=(cinfo[i].ba-start)>>2 || source[i+1]!=0) {
        if(adj) emit_addimm(cc, cinfo[i].ccadj + CLOCK_ADJUST(2) - adj, cc);
        load_regs_bt(branch_regs[i].regmap,branch_regs[i].dirty,cinfo[i].ba);
        if(internal)
          assem_debug("branch: internal\n");
        else
          assem_debug("branch: external\n");
        if (internal && dops[(cinfo[i].ba-start)>>2].is_ds) {
          ds_assemble_entry(i);
        }
        else {
          add_to_linker(out,cinfo[i].ba,internal);
          emit_jmp(0);
        }
        #ifdef CORTEX_A8_BRANCH_PREDICTION_HACK
        if(((u_int)out)&7) emit_addnop(0);
        #endif
      }
    }
    else if(nop) {
      emit_addimm_and_set_flags(cinfo[i].ccadj + CLOCK_ADJUST(2), cc);
      void *jaddr=out;
      emit_jns(0);
      add_stub(CC_STUB,jaddr,out,0,i,start+i*4+8,NOTTAKEN,0);
    }
    else {
      void *taken = NULL, *nottaken = NULL, *nottaken1 = NULL;
      do_cc(i,branch_regs[i].regmap,&adj,-1,0,invert);
      if(adj&&!invert) emit_addimm(cc, cinfo[i].ccadj + CLOCK_ADJUST(2) - adj, cc);

      //printf("branch(%d): eax=%d ecx=%d edx=%d ebx=%d ebp=%d esi=%d edi=%d\n",i,branch_regs[i].regmap[0],branch_regs[i].regmap[1],branch_regs[i].regmap[2],branch_regs[i].regmap[3],branch_regs[i].regmap[5],branch_regs[i].regmap[6],branch_regs[i].regmap[7]);
      assert(s1l>=0);
      if(dops[i].opcode==4) // BEQ
      {
        if(s2l>=0) emit_cmp(s1l,s2l);
        else emit_test(s1l,s1l);
        if(invert){
          nottaken=out;
          emit_jne(DJT_1);
        }else{
          add_to_linker(out,cinfo[i].ba,internal);
          emit_jeq(0);
        }
      }
      if(dops[i].opcode==5) // BNE
      {
        if(s2l>=0) emit_cmp(s1l,s2l);
        else emit_test(s1l,s1l);
        if(invert){
          nottaken=out;
          emit_jeq(DJT_1);
        }else{
          add_to_linker(out,cinfo[i].ba,internal);
          emit_jne(0);
        }
      }
      if(dops[i].opcode==6) // BLEZ
      {
        emit_cmpimm(s1l,1);
        if(invert){
          nottaken=out;
          emit_jge(DJT_1);
        }else{
          add_to_linker(out,cinfo[i].ba,internal);
          emit_jl(0);
        }
      }
      if(dops[i].opcode==7) // BGTZ
      {
        emit_cmpimm(s1l,1);
        if(invert){
          nottaken=out;
          emit_jl(DJT_1);
        }else{
          add_to_linker(out,cinfo[i].ba,internal);
          emit_jge(0);
        }
      }
      if(invert) {
        if(taken) set_jump_target(taken, out);
        if (vsync_hack && (vsync_hack >> 16) == i)
          vsync_hack_assemble(i, vsync_hack & 0xffff, cc);
        #ifdef CORTEX_A8_BRANCH_PREDICTION_HACK
        if (match && (!internal || !dops[(cinfo[i].ba-start)>>2].is_ds)) {
          if(adj) {
            emit_addimm(cc,-adj,cc);
            add_to_linker(out,cinfo[i].ba,internal);
          }else{
            emit_addnop(13);
            add_to_linker(out,cinfo[i].ba,internal*2);
          }
          emit_jmp(0);
        }else
        #endif
        {
          if(adj) emit_addimm(cc,-adj,cc);
          store_regs_bt(branch_regs[i].regmap,branch_regs[i].dirty,cinfo[i].ba);
          load_regs_bt(branch_regs[i].regmap,branch_regs[i].dirty,cinfo[i].ba);
          if(internal)
            assem_debug("branch: internal\n");
          else
            assem_debug("branch: external\n");
          if (internal && dops[(cinfo[i].ba - start) >> 2].is_ds) {
            ds_assemble_entry(i);
          }
          else {
            add_to_linker(out,cinfo[i].ba,internal);
            emit_jmp(0);
          }
        }
        set_jump_target(nottaken, out);
      }

      if(nottaken1) set_jump_target(nottaken1, out);
      if(adj) {
        if(!invert) emit_addimm(cc,adj,cc);
      }
    } // (!unconditional)
  } // if(ooo)
  else
  {
    // In-order execution (branch first)
    void *taken = NULL, *nottaken = NULL, *nottaken1 = NULL;
    if(!unconditional&&!nop) {
      //printf("branch(%d): eax=%d ecx=%d edx=%d ebx=%d ebp=%d esi=%d edi=%d\n",i,branch_regs[i].regmap[0],branch_regs[i].regmap[1],branch_regs[i].regmap[2],branch_regs[i].regmap[3],branch_regs[i].regmap[5],branch_regs[i].regmap[6],branch_regs[i].regmap[7]);
      assert(s1l>=0);
      if((dops[i].opcode&0x2f)==4) // BEQ
      {
        if(s2l>=0) emit_cmp(s1l,s2l);
        else emit_test(s1l,s1l);
        nottaken=out;
        emit_jne(DJT_2);
      }
      if((dops[i].opcode&0x2f)==5) // BNE
      {
        if(s2l>=0) emit_cmp(s1l,s2l);
        else emit_test(s1l,s1l);
        nottaken=out;
        emit_jeq(DJT_2);
      }
      if((dops[i].opcode&0x2f)==6) // BLEZ
      {
        emit_cmpimm(s1l,1);
        nottaken=out;
        emit_jge(DJT_2);
      }
      if((dops[i].opcode&0x2f)==7) // BGTZ
      {
        emit_cmpimm(s1l,1);
        nottaken=out;
        emit_jl(DJT_2);
      }
    } // if(!unconditional)
    int adj;
    uint64_t ds_unneeded=branch_regs[i].u;
    ds_unneeded&=~((1LL<<dops[i+1].rs1)|(1LL<<dops[i+1].rs2));
    ds_unneeded|=1;
    // branch taken
    if(!nop) {
      if(taken) set_jump_target(taken, out);
      assem_debug("1:\n");
      wb_invalidate(regs[i].regmap,branch_regs[i].regmap,regs[i].dirty,ds_unneeded);
      // load regs
      load_regs(regs[i].regmap,branch_regs[i].regmap,dops[i+1].rs1,dops[i+1].rs2);
      address_generation(i+1,&branch_regs[i],0);
      if (ram_offset)
        load_reg(regs[i].regmap,branch_regs[i].regmap,ROREG);
      load_regs(regs[i].regmap,branch_regs[i].regmap,CCREG,INVCP);
      ds_assemble(i+1,&branch_regs[i]);
      drc_dbg_emit_wb_dirtys(i+1, &branch_regs[i]);
      cc=get_reg(branch_regs[i].regmap,CCREG);
      if(cc==-1) {
        emit_loadreg(CCREG,cc=HOST_CCREG);
        // CHECK: Is the following instruction (fall thru) allocated ok?
      }
      assert(cc==HOST_CCREG);
      store_regs_bt(branch_regs[i].regmap,branch_regs[i].dirty,cinfo[i].ba);
      do_cc(i,i_regmap,&adj,cinfo[i].ba,TAKEN,0);
      assem_debug("cycle count (adj)\n");
      if(adj) emit_addimm(cc, cinfo[i].ccadj + CLOCK_ADJUST(2) - adj, cc);
      load_regs_bt(branch_regs[i].regmap,branch_regs[i].dirty,cinfo[i].ba);
      if(internal)
        assem_debug("branch: internal\n");
      else
        assem_debug("branch: external\n");
      if (internal && dops[(cinfo[i].ba - start) >> 2].is_ds) {
        ds_assemble_entry(i);
      }
      else {
        add_to_linker(out,cinfo[i].ba,internal);
        emit_jmp(0);
      }
    }
    // branch not taken
    if(!unconditional) {
      if(nottaken1) set_jump_target(nottaken1, out);
      set_jump_target(nottaken, out);
      assem_debug("2:\n");
      wb_invalidate(regs[i].regmap,branch_regs[i].regmap,regs[i].dirty,ds_unneeded);
      // load regs
      load_regs(regs[i].regmap,branch_regs[i].regmap,dops[i+1].rs1,dops[i+1].rs2);
      address_generation(i+1,&branch_regs[i],0);
      if (ram_offset)
        load_reg(regs[i].regmap,branch_regs[i].regmap,ROREG);
      load_regs(regs[i].regmap,branch_regs[i].regmap,CCREG,INVCP);
      ds_assemble(i+1,&branch_regs[i]);
      cc=get_reg(branch_regs[i].regmap,CCREG);
      if (cc == -1) {
        // Cycle count isn't in a register, temporarily load it then write it out
        emit_loadreg(CCREG,HOST_CCREG);
        emit_addimm_and_set_flags(cinfo[i].ccadj + CLOCK_ADJUST(2), HOST_CCREG);
        void *jaddr=out;
        emit_jns(0);
        add_stub(CC_STUB,jaddr,out,0,i,start+i*4+8,NOTTAKEN,0);
        emit_storereg(CCREG,HOST_CCREG);
      }
      else{
        cc=get_reg(i_regmap,CCREG);
        assert(cc==HOST_CCREG);
        emit_addimm_and_set_flags(cinfo[i].ccadj + CLOCK_ADJUST(2), cc);
        void *jaddr=out;
        emit_jns(0);
        add_stub(CC_STUB,jaddr,out,0,i,start+i*4+8,NOTTAKEN,0);
      }
    }
  }
}

static void sjump_assemble(int i, const struct regstat *i_regs)
{
  const signed char *i_regmap = i_regs->regmap;
  int cc;
  int match;
  match=match_bt(branch_regs[i].regmap,branch_regs[i].dirty,cinfo[i].ba);
  assem_debug("smatch=%d ooo=%d\n", match, dops[i].ooo);
  int s1l;
  int unconditional=0,nevertaken=0;
  int invert=0;
  int internal=internal_branch(cinfo[i].ba);
  if(i==(cinfo[i].ba-start)>>2) assem_debug("idle loop\n");
  if(!match) invert=1;
  #ifdef CORTEX_A8_BRANCH_PREDICTION_HACK
  if(i>(cinfo[i].ba-start)>>2) invert=1;
  #endif
  #ifdef __aarch64__
  invert=1; // because of near cond. branches
  #endif

  //if(dops[i].opcode2>=0x10) return; // FIXME (BxxZAL)
  //assert(dops[i].opcode2<0x10||dops[i].rs1==0); // FIXME (BxxZAL)

  if(dops[i].ooo) {
    s1l=get_reg(branch_regs[i].regmap,dops[i].rs1);
  }
  else {
    s1l=get_reg(i_regmap,dops[i].rs1);
  }
  if(dops[i].rs1==0)
  {
    if(dops[i].opcode2&1) unconditional=1;
    else nevertaken=1;
    // These are never taken (r0 is never less than zero)
    //assert(dops[i].opcode2!=0);
    //assert(dops[i].opcode2!=2);
    //assert(dops[i].opcode2!=0x10);
    //assert(dops[i].opcode2!=0x12);
  }

  if(dops[i].ooo) {
    // Out of order execution (delay slot first)
    //printf("OOOE\n");
    address_generation(i+1,i_regs,regs[i].regmap_entry);
    ds_assemble(i+1,i_regs);
    int adj;
    uint64_t bc_unneeded=branch_regs[i].u;
    bc_unneeded&=~((1LL<<dops[i].rs1)|(1LL<<dops[i].rs2));
    bc_unneeded|=1;
    wb_invalidate(regs[i].regmap,branch_regs[i].regmap,regs[i].dirty,bc_unneeded);
    load_regs(regs[i].regmap,branch_regs[i].regmap,dops[i].rs1,dops[i].rs1);
    load_reg(regs[i].regmap,branch_regs[i].regmap,CCREG);
    if(dops[i].rt1==31) {
      int rt,return_address;
      rt=get_reg(branch_regs[i].regmap,31);
      //assem_debug("branch(%d): eax=%d ecx=%d edx=%d ebx=%d ebp=%d esi=%d edi=%d\n",i,branch_regs[i].regmap[0],branch_regs[i].regmap[1],branch_regs[i].regmap[2],branch_regs[i].regmap[3],branch_regs[i].regmap[5],branch_regs[i].regmap[6],branch_regs[i].regmap[7]);
      if(rt>=0) {
        // Save the PC even if the branch is not taken
        return_address=start+i*4+8;
        emit_movimm(return_address,rt); // PC into link register
        #ifdef IMM_PREFETCH
        if(!nevertaken) emit_prefetch(hash_table_get(return_address));
        #endif
      }
    }
    cc=get_reg(branch_regs[i].regmap,CCREG);
    assert(cc==HOST_CCREG);
    if(unconditional)
      store_regs_bt(branch_regs[i].regmap,branch_regs[i].dirty,cinfo[i].ba);
    //do_cc(i,branch_regs[i].regmap,&adj,unconditional?cinfo[i].ba:-1,unconditional);
    assem_debug("cycle count (adj)\n");
    if(unconditional) {
      do_cc(i,branch_regs[i].regmap,&adj,cinfo[i].ba,TAKEN,0);
      if(i!=(cinfo[i].ba-start)>>2 || source[i+1]!=0) {
        if(adj) emit_addimm(cc, cinfo[i].ccadj + CLOCK_ADJUST(2) - adj, cc);
        load_regs_bt(branch_regs[i].regmap,branch_regs[i].dirty,cinfo[i].ba);
        if(internal)
          assem_debug("branch: internal\n");
        else
          assem_debug("branch: external\n");
        if (internal && dops[(cinfo[i].ba - start) >> 2].is_ds) {
          ds_assemble_entry(i);
        }
        else {
          add_to_linker(out,cinfo[i].ba,internal);
          emit_jmp(0);
        }
        #ifdef CORTEX_A8_BRANCH_PREDICTION_HACK
        if(((u_int)out)&7) emit_addnop(0);
        #endif
      }
    }
    else if(nevertaken) {
      emit_addimm_and_set_flags(cinfo[i].ccadj + CLOCK_ADJUST(2), cc);
      void *jaddr=out;
      emit_jns(0);
      add_stub(CC_STUB,jaddr,out,0,i,start+i*4+8,NOTTAKEN,0);
    }
    else {
      void *nottaken = NULL;
      do_cc(i,branch_regs[i].regmap,&adj,-1,0,invert);
      if(adj&&!invert) emit_addimm(cc, cinfo[i].ccadj + CLOCK_ADJUST(2) - adj, cc);
      {
        assert(s1l>=0);
        if ((dops[i].opcode2 & 1) == 0) // BLTZ/BLTZAL
        {
          emit_test(s1l,s1l);
          if(invert){
            nottaken=out;
            emit_jns(DJT_1);
          }else{
            add_to_linker(out,cinfo[i].ba,internal);
            emit_js(0);
          }
        }
        else // BGEZ/BGEZAL
        {
          emit_test(s1l,s1l);
          if(invert){
            nottaken=out;
            emit_js(DJT_1);
          }else{
            add_to_linker(out,cinfo[i].ba,internal);
            emit_jns(0);
          }
        }
      }

      if(invert) {
        #ifdef CORTEX_A8_BRANCH_PREDICTION_HACK
        if (match && (!internal || !dops[(cinfo[i].ba - start) >> 2].is_ds)) {
          if(adj) {
            emit_addimm(cc,-adj,cc);
            add_to_linker(out,cinfo[i].ba,internal);
          }else{
            emit_addnop(13);
            add_to_linker(out,cinfo[i].ba,internal*2);
          }
          emit_jmp(0);
        }else
        #endif
        {
          if(adj) emit_addimm(cc,-adj,cc);
          store_regs_bt(branch_regs[i].regmap,branch_regs[i].dirty,cinfo[i].ba);
          load_regs_bt(branch_regs[i].regmap,branch_regs[i].dirty,cinfo[i].ba);
          if(internal)
            assem_debug("branch: internal\n");
          else
            assem_debug("branch: external\n");
          if (internal && dops[(cinfo[i].ba - start) >> 2].is_ds) {
            ds_assemble_entry(i);
          }
          else {
            add_to_linker(out,cinfo[i].ba,internal);
            emit_jmp(0);
          }
        }
        set_jump_target(nottaken, out);
      }

      if(adj) {
        if(!invert) emit_addimm(cc,adj,cc);
      }
    } // (!unconditional)
  } // if(ooo)
  else
  {
    // In-order execution (branch first)
    //printf("IOE\n");
    void *nottaken = NULL;
    if (!unconditional && !nevertaken) {
      assert(s1l >= 0);
      emit_test(s1l, s1l);
    }
    if (dops[i].rt1 == 31) {
      int rt, return_address;
      rt = get_reg(branch_regs[i].regmap,31);
      if(rt >= 0) {
        // Save the PC even if the branch is not taken
        return_address = start + i*4+8;
        emit_movimm(return_address, rt); // PC into link register
        #ifdef IMM_PREFETCH
        emit_prefetch(hash_table_get(return_address));
        #endif
      }
    }
    if (!unconditional && !nevertaken) {
      nottaken = out;
      if (!(dops[i].opcode2 & 1)) // BLTZ/BLTZAL
        emit_jns(DJT_1);
      else                        // BGEZ/BGEZAL
        emit_js(DJT_1);
    }
    int adj;
    uint64_t ds_unneeded=branch_regs[i].u;
    ds_unneeded&=~((1LL<<dops[i+1].rs1)|(1LL<<dops[i+1].rs2));
    ds_unneeded|=1;
    // branch taken
    if(!nevertaken) {
      //assem_debug("1:\n");
      wb_invalidate(regs[i].regmap,branch_regs[i].regmap,regs[i].dirty,ds_unneeded);
      // load regs
      load_regs(regs[i].regmap,branch_regs[i].regmap,dops[i+1].rs1,dops[i+1].rs2);
      address_generation(i+1,&branch_regs[i],0);
      if (ram_offset)
        load_reg(regs[i].regmap,branch_regs[i].regmap,ROREG);
      load_regs(regs[i].regmap,branch_regs[i].regmap,CCREG,INVCP);
      ds_assemble(i+1,&branch_regs[i]);
      cc=get_reg(branch_regs[i].regmap,CCREG);
      if(cc==-1) {
        emit_loadreg(CCREG,cc=HOST_CCREG);
        // CHECK: Is the following instruction (fall thru) allocated ok?
      }
      assert(cc==HOST_CCREG);
      store_regs_bt(branch_regs[i].regmap,branch_regs[i].dirty,cinfo[i].ba);
      do_cc(i,i_regmap,&adj,cinfo[i].ba,TAKEN,0);
      assem_debug("cycle count (adj)\n");
      if(adj) emit_addimm(cc, cinfo[i].ccadj + CLOCK_ADJUST(2) - adj, cc);
      load_regs_bt(branch_regs[i].regmap,branch_regs[i].dirty,cinfo[i].ba);
      if(internal)
        assem_debug("branch: internal\n");
      else
        assem_debug("branch: external\n");
      if (internal && dops[(cinfo[i].ba - start) >> 2].is_ds) {
        ds_assemble_entry(i);
      }
      else {
        add_to_linker(out,cinfo[i].ba,internal);
        emit_jmp(0);
      }
    }
    // branch not taken
    if(!unconditional) {
      if (!nevertaken) {
        assert(nottaken);
        set_jump_target(nottaken, out);
      }
      assem_debug("1:\n");
      wb_invalidate(regs[i].regmap,branch_regs[i].regmap,regs[i].dirty,ds_unneeded);
      load_regs(regs[i].regmap,branch_regs[i].regmap,dops[i+1].rs1,dops[i+1].rs2);
      address_generation(i+1,&branch_regs[i],0);
      if (ram_offset)
        load_reg(regs[i].regmap,branch_regs[i].regmap,ROREG);
      load_regs(regs[i].regmap,branch_regs[i].regmap,CCREG,INVCP);
      ds_assemble(i+1,&branch_regs[i]);
      cc=get_reg(branch_regs[i].regmap,CCREG);
      if (cc == -1) {
        // Cycle count isn't in a register, temporarily load it then write it out
        emit_loadreg(CCREG,HOST_CCREG);
        emit_addimm_and_set_flags(cinfo[i].ccadj + CLOCK_ADJUST(2), HOST_CCREG);
        void *jaddr=out;
        emit_jns(0);
        add_stub(CC_STUB,jaddr,out,0,i,start+i*4+8,NOTTAKEN,0);
        emit_storereg(CCREG,HOST_CCREG);
      }
      else{
        cc=get_reg(i_regmap,CCREG);
        assert(cc==HOST_CCREG);
        emit_addimm_and_set_flags(cinfo[i].ccadj + CLOCK_ADJUST(2), cc);
        void *jaddr=out;
        emit_jns(0);
        add_stub(CC_STUB,jaddr,out,0,i,start+i*4+8,NOTTAKEN,0);
      }
    }
  }
}

static void check_regmap(signed char *regmap)
{
#ifndef NDEBUG
  int i,j;
  for (i = 0; i < HOST_REGS; i++) {
    if (regmap[i] < 0)
      continue;
    for (j = i + 1; j < HOST_REGS; j++)
      assert(regmap[i] != regmap[j]);
  }
#endif
}

#ifdef DISASM
#include <inttypes.h>
static char insn[MAXBLOCK][10];

#define set_mnemonic(i_, n_) \
  strcpy(insn[i_], n_)

void print_regmap(const char *name, const signed char *regmap)
{
  char buf[5];
  int i, l;
  fputs(name, stdout);
  for (i = 0; i < HOST_REGS; i++) {
    l = 0;
    if (regmap[i] >= 0)
      l = snprintf(buf, sizeof(buf), "$%d", regmap[i]);
    for (; l < 3; l++)
      buf[l] = ' ';
    buf[l] = 0;
    printf(" r%d=%s", i, buf);
  }
  fputs("\n", stdout);
}

  /* disassembly */
void disassemble_inst(int i)
{
    if (dops[i].bt) printf("*"); else printf(" ");
    switch(dops[i].itype) {
      case UJUMP:
        printf (" %x: %s %8x\n",start+i*4,insn[i],cinfo[i].ba);break;
      case CJUMP:
        printf (" %x: %s r%d,r%d,%8x\n",start+i*4,insn[i],dops[i].rs1,dops[i].rs2,i?start+i*4+4+((signed int)((unsigned int)source[i]<<16)>>14):cinfo[i].ba);break;
      case SJUMP:
        printf (" %x: %s r%d,%8x\n",start+i*4,insn[i],dops[i].rs1,start+i*4+4+((signed int)((unsigned int)source[i]<<16)>>14));break;
      case RJUMP:
        if (dops[i].opcode2 == 9 && dops[i].rt1 != 31)
          printf (" %x: %s r%d,r%d\n",start+i*4,insn[i],dops[i].rt1,dops[i].rs1);
        else
          printf (" %x: %s r%d\n",start+i*4,insn[i],dops[i].rs1);
        break;
      case IMM16:
        if(dops[i].opcode==0xf) //LUI
          printf (" %x: %s r%d,%4x0000\n",start+i*4,insn[i],dops[i].rt1,cinfo[i].imm&0xffff);
        else
          printf (" %x: %s r%d,r%d,%d\n",start+i*4,insn[i],dops[i].rt1,dops[i].rs1,cinfo[i].imm);
        break;
      case LOAD:
      case LOADLR:
        printf (" %x: %s r%d,r%d+%x\n",start+i*4,insn[i],dops[i].rt1,dops[i].rs1,cinfo[i].imm);
        break;
      case STORE:
      case STORELR:
        printf (" %x: %s r%d,r%d+%x\n",start+i*4,insn[i],dops[i].rs2,dops[i].rs1,cinfo[i].imm);
        break;
      case ALU:
      case SHIFT:
        printf (" %x: %s r%d,r%d,r%d\n",start+i*4,insn[i],dops[i].rt1,dops[i].rs1,dops[i].rs2);
        break;
      case MULTDIV:
        printf (" %x: %s r%d,r%d\n",start+i*4,insn[i],dops[i].rs1,dops[i].rs2);
        break;
      case SHIFTIMM:
        printf (" %x: %s r%d,r%d,%d\n",start+i*4,insn[i],dops[i].rt1,dops[i].rs1,cinfo[i].imm);
        break;
      case MOV:
        if((dops[i].opcode2&0x1d)==0x10)
          printf (" %x: %s r%d\n",start+i*4,insn[i],dops[i].rt1);
        else if((dops[i].opcode2&0x1d)==0x11)
          printf (" %x: %s r%d\n",start+i*4,insn[i],dops[i].rs1);
        else
          printf (" %x: %s\n",start+i*4,insn[i]);
        break;
      case COP0:
        if(dops[i].opcode2==0)
          printf (" %x: %s r%d,cpr0[%d]\n",start+i*4,insn[i],dops[i].rt1,(source[i]>>11)&0x1f); // MFC0
        else if(dops[i].opcode2==4)
          printf (" %x: %s r%d,cpr0[%d]\n",start+i*4,insn[i],dops[i].rs1,(source[i]>>11)&0x1f); // MTC0
        else printf (" %x: %s\n",start+i*4,insn[i]);
        break;
      case COP2:
        if(dops[i].opcode2<3)
          printf (" %x: %s r%d,cpr2[%d]\n",start+i*4,insn[i],dops[i].rt1,(source[i]>>11)&0x1f); // MFC2
        else if(dops[i].opcode2>3)
          printf (" %x: %s r%d,cpr2[%d]\n",start+i*4,insn[i],dops[i].rs1,(source[i]>>11)&0x1f); // MTC2
        else printf (" %x: %s\n",start+i*4,insn[i]);
        break;
      case C2LS:
        printf (" %x: %s cpr2[%d],r%d+%x\n",start+i*4,insn[i],(source[i]>>16)&0x1f,dops[i].rs1,cinfo[i].imm);
        break;
      case INTCALL:
        printf (" %x: %s (INTCALL)\n",start+i*4,insn[i]);
        break;
      default:
        //printf (" %s %8x\n",insn[i],source[i]);
        printf (" %x: %s\n",start+i*4,insn[i]);
    }
    #ifndef REGMAP_PRINT
    return;
    #endif
    printf("D: %x  WD: %x  U: %"PRIx64"  hC: %x  hWC: %x  hLC: %x\n",
      regs[i].dirty, regs[i].wasdirty, unneeded_reg[i],
      regs[i].isconst, regs[i].wasconst, regs[i].loadedconst);
    print_regmap("pre:   ", regmap_pre[i]);
    print_regmap("entry: ", regs[i].regmap_entry);
    print_regmap("map:   ", regs[i].regmap);
    if (dops[i].is_jump) {
      print_regmap("bentry:", branch_regs[i].regmap_entry);
      print_regmap("bmap:  ", branch_regs[i].regmap);
    }
}
#else
#define set_mnemonic(i_, n_)
static void disassemble_inst(int i) {}
#endif // DISASM

#define DRC_TEST_VAL 0x74657374

static noinline void new_dynarec_test(void)
{
  int (*testfunc)(void);
  void *beginning;
  int ret[2];
  size_t i;

  // check structure linkage
  if ((u_char *)rcnts - (u_char *)&psxRegs != sizeof(psxRegs))
  {
    SysPrintf("linkage_arm* miscompilation/breakage detected.\n");
  }

  SysPrintf("(%p) testing if we can run recompiled code @%p...\n",
    new_dynarec_test, out);

  for (i = 0; i < ARRAY_SIZE(ret); i++) {
    out = ndrc->translation_cache;
    beginning = start_block();
    ((volatile u_int *)NDRC_WRITE_OFFSET(out))[0]++; // make the cache dirty
    emit_movimm(DRC_TEST_VAL + i, 0); // test
    emit_ret();
    literal_pool(0);
    end_block(beginning);
    testfunc = beginning;
    ret[i] = testfunc();
  }

  if (ret[0] == DRC_TEST_VAL && ret[1] == DRC_TEST_VAL + 1)
    SysPrintf("test passed.\n");
  else
    SysPrintf("test failed, will likely crash soon (r=%08x %08x)\n", ret[0], ret[1]);
  out = ndrc->translation_cache;
}

static int get_cycle_multiplier(void)
{
  return Config.cycle_multiplier_override && Config.cycle_multiplier == CYCLE_MULT_DEFAULT
     ? Config.cycle_multiplier_override : Config.cycle_multiplier;
}

// clear the state completely, instead of just marking
// things invalid like invalidate_all_pages() does
void new_dynarec_clear_full(void)
{
  int n;
  out = ndrc->translation_cache;
  memset(invalid_code,1,sizeof(invalid_code));
  memset(shadow,0,sizeof(shadow));
  hash_table_clear();
  mini_ht_clear();
  copy=shadow;
  expirep = EXPIRITY_OFFSET;
  literalcount=0;
  stop_after_jal=0;
  ni_count=0;
  err_print_count=0;
  inv_code_start=inv_code_end=~0;
  hack_addr=0;
  f1_hack=0;
  for (n = 0; n < ARRAY_SIZE(blocks); n++)
    blocks_clear(&blocks[n]);
  for (n = 0; n < ARRAY_SIZE(jumps); n++) {
    free(jumps[n]);
    jumps[n] = NULL;
  }
  stat_clear(stat_blocks);
  stat_clear(stat_links);

  if (ndrc_g.cycle_multiplier_old != Config.cycle_multiplier
      || ndrc_g.hacks_old != (ndrc_g.hacks | ndrc_g.hacks_pergame))
  {
    SysPrintf("ndrc config: mul=%d, ha=%x, pex=%d\n",
      get_cycle_multiplier(), ndrc_g.hacks, Config.PreciseExceptions);
  }
  ndrc_g.cycle_multiplier_old = Config.cycle_multiplier;
  ndrc_g.hacks_old = ndrc_g.hacks | ndrc_g.hacks_pergame;
}

static int pgsize(void)
{
  long ret = -1;
#ifdef _SC_PAGESIZE
  ret = sysconf(_SC_PAGESIZE);
#endif
  if (ret < 1)
    ret = 4096;
  return ret;
}

void new_dynarec_init(void)
{
  int align = pgsize() - 1;
  SysPrintf("Init new dynarec, ndrc size %x, pgsize %d\n",
    (int)sizeof(*ndrc), align + 1);

#ifdef BASE_ADDR_DYNAMIC
  #ifdef VITA
  sceBlock = getVMBlock(); //sceKernelAllocMemBlockForVM("code", sizeof(*ndrc));
  if (sceBlock <= 0)
    SysPrintf("getVMBlock failed: %x\n", sceBlock);
  int ret = sceKernelGetMemBlockBase(sceBlock, (void **)&ndrc);
  if (ret)
    SysPrintf("sceKernelGetMemBlockBase: %x\n", ret);
  ret = sceKernelOpenVMDomain();
  if (ret)
    SysPrintf("sceKernelOpenVMDomain: %x\n", ret);
  #elif defined(_MSC_VER)
  ndrc = VirtualAlloc(NULL, sizeof(*ndrc), MEM_COMMIT | MEM_RESERVE,
    PAGE_EXECUTE_READWRITE);
  #elif defined(HAVE_LIBNX)
  Result rc = jitCreate(&g_jit, sizeof(*ndrc));
  if (R_FAILED(rc))
    SysPrintf("jitCreate failed: %08x\n", rc);
  SysPrintf("jitCreate: RX: %p RW: %p type: %d\n", g_jit.rx_addr, g_jit.rw_addr, g_jit.type);
  jitTransitionToWritable(&g_jit);
  ndrc = g_jit.rx_addr;
  ndrc_write_ofs = (char *)g_jit.rw_addr - (char *)ndrc;
  memset(NDRC_WRITE_OFFSET(&ndrc->tramp), 0, sizeof(ndrc->tramp));
  #else
  uintptr_t desired_addr = 0;
  int prot = PROT_READ | PROT_WRITE | PROT_EXEC;
  int flags = MAP_PRIVATE | MAP_ANONYMOUS;
  int fd = -1;
  #ifdef __ELF__
  extern char _end;
  desired_addr = ((uintptr_t)&_end + 0xffffff) & ~0xffffffl;
  #endif
  #ifdef TC_WRITE_OFFSET
  // mostly for testing
  fd = open("/dev/shm/pcsxr", O_CREAT | O_RDWR, 0600);
  ftruncate(fd, sizeof(*ndrc));
  void *mw = mmap(NULL, sizeof(*ndrc), PROT_READ | PROT_WRITE,
                  (flags = MAP_SHARED), fd, 0);
  assert(mw != MAP_FAILED);
  #endif
  #if defined(NO_WRITE_EXEC) || defined(TC_WRITE_OFFSET)
  prot = PROT_READ | PROT_EXEC;
  #endif
  ndrc = mmap((void *)desired_addr, sizeof(*ndrc), prot, flags, fd, 0);
  if (ndrc == MAP_FAILED) {
    SysPrintf("mmap() failed: %s\n", strerror(errno));
    abort();
  }
  #ifdef TC_WRITE_OFFSET
  ndrc_write_ofs = (char *)mw - (char *)ndrc;
  #endif
  #endif
#else
  ndrc = (struct ndrc_mem *)((size_t)(ndrc_bss + align) & ~align);
  #ifndef NO_WRITE_EXEC
  // not all systems allow execute in data segment by default
  // size must be 4K aligned for 3DS?
  if (mprotect(ndrc, sizeof(*ndrc),
               PROT_READ | PROT_WRITE | PROT_EXEC) != 0)
    SysPrintf("mprotect(%p) failed: %s\n", ndrc, strerror(errno));
  #endif
  #ifdef TC_WRITE_OFFSET
  #error "misconfiguration detected"
  #endif
#endif
  out = ndrc->translation_cache;
  new_dynarec_clear_full();
  hash_table_ptr = hash_table;
#ifdef HOST_IMM8
  // Copy this into local area so we don't have to put it in every literal pool
  invc_ptr=invalid_code;
#endif
  arch_init();
  new_dynarec_test();
  ram_offset = (uintptr_t)psxM - 0x80000000;
  if (ram_offset != 0)
    SysPrintf("RAM is not directly mapped\n");
  SysPrintf("Mapped (RAM/scrp/ROM/LUTs/TC):\n");
  SysPrintf("%p/%p/%p/%p/%p\n", psxM, psxH, psxR, mem_rtab, out);
}

void new_dynarec_cleanup(void)
{
  int n;
#ifdef BASE_ADDR_DYNAMIC
  #ifdef VITA
  // sceBlock is managed by retroarch's bootstrap code
  //sceKernelFreeMemBlock(sceBlock);
  //sceBlock = -1;
  #elif defined(HAVE_LIBNX)
  jitClose(&g_jit);
  ndrc = NULL;
  #else
  if (munmap(ndrc, sizeof(*ndrc)) < 0)
    SysPrintf("munmap() failed\n");
  ndrc = NULL;
  #endif
#endif
  for (n = 0; n < ARRAY_SIZE(blocks); n++)
    blocks_clear(&blocks[n]);
  for (n = 0; n < ARRAY_SIZE(jumps); n++) {
    free(jumps[n]);
    jumps[n] = NULL;
  }
  stat_clear(stat_blocks);
  stat_clear(stat_links);
  new_dynarec_print_stats();
}

static u_int *get_source_start(u_int addr, u_int *limit)
{
  if (addr < 0x00800000u
      || (0x80000000u <= addr && addr < 0x80800000u)
      || (0xa0000000u <= addr && addr < 0xa0800000u))
  {
    // used for BIOS calls mostly?
    *limit = (addr & 0xa0600000) + 0x00200000;
    return (u_int *)(psxM + (addr & 0x1fffff));
  }
  else if (
    (0x9fc00000u <= addr && addr < 0x9fc80000u) ||
    (0xbfc00000u <= addr && addr < 0xbfc80000u))
  {
    // BIOS. The multiplier should be much higher as it's uncached 8bit mem
    // XXX: disabled as this introduces differences from the interpreter
    // and lightrec multipliers making emu variations act inconsistently
    //if (!HACK_ENABLED(NDHACK_OVERRIDE_CYCLE_M))
    //  cycle_multiplier_active = 200;

    *limit = (addr & 0xfff00000) | 0x80000;
    return (u_int *)((u_char *)psxR + (addr&0x7ffff));
  }
  return NULL;
}

static u_int scan_for_ret(u_int addr)
{
  u_int limit = 0;
  u_int *mem;

  mem = get_source_start(addr, &limit);
  if (mem == NULL)
    return addr;

  if (limit > addr + 0x1000)
    limit = addr + 0x1000;
  for (; addr < limit; addr += 4, mem++) {
    if (*mem == 0x03e00008) // jr $ra
      return addr + 8;
  }
  return addr;
}

struct savestate_block {
  uint32_t addr;
  uint32_t regflags;
};

static int addr_cmp(const void *p1_, const void *p2_)
{
  const struct savestate_block *p1 = p1_, *p2 = p2_;
  return p1->addr - p2->addr;
}

int new_dynarec_save_blocks(void *save, int size)
{
  struct savestate_block *sblocks = save;
  int maxcount = size / sizeof(sblocks[0]);
  struct savestate_block tmp_blocks[1024];
  struct block_info *block;
  int p, s, d, o, bcnt;
  u_int addr;

  o = 0;
  for (p = 0; p < ARRAY_SIZE(blocks); p++) {
    bcnt = 0;
    for (block = blocks[p]; block != NULL; block = block->next) {
      if (block->is_dirty)
        continue;
      tmp_blocks[bcnt].addr = block->start;
      tmp_blocks[bcnt].regflags = block->reg_sv_flags;
      bcnt++;
    }
    if (bcnt < 1)
      continue;
    qsort(tmp_blocks, bcnt, sizeof(tmp_blocks[0]), addr_cmp);

    addr = tmp_blocks[0].addr;
    for (s = d = 0; s < bcnt; s++) {
      if (tmp_blocks[s].addr < addr)
        continue;
      if (d == 0 || tmp_blocks[d-1].addr != tmp_blocks[s].addr)
        tmp_blocks[d++] = tmp_blocks[s];
      addr = scan_for_ret(tmp_blocks[s].addr);
    }

    if (o + d > maxcount)
      d = maxcount - o;
    memcpy(&sblocks[o], tmp_blocks, d * sizeof(sblocks[0]));
    o += d;
  }

  return o * sizeof(sblocks[0]);
}

void new_dynarec_load_blocks(const void *save, int size)
{
  const struct savestate_block *sblocks = save;
  int count = size / sizeof(sblocks[0]);
  struct block_info *block;
  u_int regs_save[32];
  u_int page;
  uint32_t f;
  int i, b;

  // restore clean blocks, if any
  for (page = 0, b = i = 0; page < ARRAY_SIZE(blocks); page++) {
    for (block = blocks[page]; block != NULL; block = block->next, b++) {
      if (!block->is_dirty)
        continue;
      assert(block->source && block->copy);
      if (memcmp(block->source, block->copy, block->len))
        continue;

      // see try_restore_block
      block->is_dirty = 0;
      mark_invalid_code(block->start, block->len, 0);
      i++;
    }
  }
  inv_debug("load_blocks: %d/%d clean blocks\n", i, b);

  // change GPRs for speculation to at least partially work..
  memcpy(regs_save, &psxRegs.GPR, sizeof(regs_save));
  for (i = 1; i < 32; i++)
    psxRegs.GPR.r[i] = 0x80000000;

  for (b = 0; b < count; b++) {
    for (f = sblocks[b].regflags, i = 0; f; f >>= 1, i++) {
      if (f & 1)
        psxRegs.GPR.r[i] = 0x1f800000;
    }

    ndrc_get_addr_ht_param(hash_table, sblocks[b].addr, ndrc_cm_compile_offline);

    for (f = sblocks[b].regflags, i = 0; f; f >>= 1, i++) {
      if (f & 1)
        psxRegs.GPR.r[i] = 0x80000000;
    }
  }

  memcpy(&psxRegs.GPR, regs_save, sizeof(regs_save));
}

void new_dynarec_print_stats(void)
{
#ifdef STAT_PRINT
  printf("cc %3d,%3d,%3d lu%6d,%3d,%3d c%3d inv%3d,%3d tc_offs %zu b %u,%u\n",
    stat_bc_pre, stat_bc_direct, stat_bc_restore,
    stat_ht_lookups, stat_jump_in_lookups, stat_restore_tries,
    stat_restore_compares, stat_inv_addr_calls, stat_inv_hits,
    out - ndrc->translation_cache, stat_blocks, stat_links);
  stat_bc_direct = stat_bc_pre = stat_bc_restore =
  stat_ht_lookups = stat_jump_in_lookups = stat_restore_tries =
  stat_restore_compares = stat_inv_addr_calls = stat_inv_hits = 0;
#endif
}

static void force_intcall(int i)
{
  memset(&dops[i], 0, sizeof(dops[i]));
  dops[i].itype = INTCALL;
  dops[i].rs1 = CCREG;
  dops[i].is_exception = dops[i].may_except = 1;
  cinfo[i].ba = -1;
}

static noinline void do_vsync(int i)
{
  // lui a0, x; addiu a0, x; jal puts
  u32 addr = (cinfo[i].imm << 16) + (signed short)cinfo[i+1].imm;
  char *str = NULL;
  int j, t, jals_cnt = 0;

  if (!is_ram_addr(addr))
      return;
  str = (char *)psxM + (addr & 0x1fffff);
  if (!str || strncmp(str, "VSync: timeout", 14))
    return;
  // jal clearPad, jal clearRCnt; j return; nop
  for (j = i+2; j < slen; j++) {
    if (dops[j].itype == SHIFTIMM || dops[j].itype == IMM16 || dops[j].itype == ALU)
      continue;
    if (dops[j].opcode == 0x03) {
      jals_cnt++; continue;
    }
    break;
  }
  if (j >= slen || jals_cnt != 3 || dops[j++].opcode != 0x02)
    return;
  for (; j < slen; j++)
    if (dops[j].itype != SHIFTIMM && dops[j].itype != IMM16)
      break;
  if (j >= slen || dops[j].opcode != 0x23) // lw x, condition
    return;
  j += 2;
  if (dops[j].opcode != 0 || dops[j].opcode2 != 0x2A) // slt x, y
    return;
  if (dops[++j].opcode != 0x05) // bnez x, loop
    return;
  t = (cinfo[j].ba - start) / 4;
  if (t < 0 || t >= slen)
    return;
  // lw x, d(sp)
  if (dops[t].opcode != 0x23 || dops[t].rs1 != 29 || (u32)cinfo[t].imm >= 1024)
    return;
  if (dops[t+2].opcode != 0x09 || cinfo[t+2].imm != -1) // addiu x, -1
    return;
  SysPrintf("vsync @%08x\n", start + t*4);
  vsync_hack = (j << 16) | (cinfo[t].imm & 0xffff);
}

static int apply_hacks(void)
{
  int i;
  vsync_hack = 0;
  if (HACK_ENABLED(NDHACK_NO_COMPAT_HACKS))
    return 0;
  /* special hack(s) */
  for (i = 0; i < slen - 4; i++)
  {
    // lui a4, 0xf200; jal <rcnt_read>; addu a0, 2; slti v0, 28224
    if (source[i] == 0x3c04f200 && dops[i+1].itype == UJUMP
        && source[i+2] == 0x34840002 && dops[i+3].opcode == 0x0a
        && cinfo[i+3].imm == 0x6e40 && dops[i+3].rs1 == 2)
    {
      SysPrintf("PE2 hack @%08x\n", start + (i+3)*4);
      dops[i + 3].itype = NOP;
    }
    // see also: psxBiosCheckExe()
    if (i > 1 && dops[i].opcode == 0x0f && dops[i].rt1 == 4
        && dops[i+1].opcode == 0x09 && dops[i+1].rt1 == 4 && dops[i+1].rs1 == 4
        && dops[i+2].opcode == 0x03)
    {
      do_vsync(i);
    }
  }
  if (source[0] == 0x3c05edb8 && source[1] == 0x34a58320)
  {
    // lui a1, 0xEDB8; ori a1, 0x8320
    SysPrintf("F1 2000 hack @%08x\n", start);
    cycle_multiplier_active = 100;
  }
  i = slen;
  if (i > 10 && source[i-1] == 0 && source[i-2] == 0x03e00008
      && source[i-4] == 0x8fbf0018 && source[i-6] == 0x00c0f809
      && dops[i-7].itype == STORE)
  {
    i = i-8;
    if (dops[i].itype == IMM16)
      i--;
    // swl r2, 15(r6); swr r2, 12(r6); sw r6, *; jalr r6
    if (dops[i].itype == STORELR && dops[i].rs1 == 6
      && dops[i-1].itype == STORELR && dops[i-1].rs1 == 6)
    {
      SysPrintf("F1 hack from %08x, old dst %08x\n", start, hack_addr);
      f1_hack = 1;
      return 1;
    }
  }
#if 0 // alt vsync, not used
  if (Config.HLE)
  {
    if (start <= psxRegs.biosBranchCheck && psxRegs.biosBranchCheck < start + i*4)
    {
      i = (psxRegs.biosBranchCheck - start) / 4u + 23;
      if (dops[i].is_jump && !dops[i+1].bt)
      {
        force_intcall(i);
        dops[i+1].is_ds = 0;
      }
    }
  }
#endif
  return 0;
}

static int is_ld_use_hazard(const struct decoded_insn *op_ld,
  const struct decoded_insn *op)
{
  if (op_ld->rt1 == 0 || (op_ld->rt1 != op->rs1 && op_ld->rt1 != op->rs2))
    return 0;
  if (op_ld->itype == LOADLR && op->itype == LOADLR)
    return op_ld->rt1 == op_ld->rs1;
  return op->itype != CJUMP && op->itype != SJUMP;
}

static void disassemble_one(int i, u_int src)
{
    unsigned int type, op, op2, op3;
    enum ls_width_type ls_type = LS_32;
    memset(&dops[i], 0, sizeof(dops[i]));
    memset(&cinfo[i], 0, sizeof(cinfo[i]));
    cinfo[i].ba = -1;
    cinfo[i].addr = -1;
    dops[i].opcode = op = src >> 26;
    op2 = 0;
    type = INTCALL;
    set_mnemonic(i, "???");
    switch(op)
    {
      case 0x00: set_mnemonic(i, "special");
        op2 = src & 0x3f;
        switch(op2)
        {
          case 0x00: set_mnemonic(i, "SLL"); type=SHIFTIMM; break;
          case 0x02: set_mnemonic(i, "SRL"); type=SHIFTIMM; break;
          case 0x03: set_mnemonic(i, "SRA"); type=SHIFTIMM; break;
          case 0x04: set_mnemonic(i, "SLLV"); type=SHIFT; break;
          case 0x06: set_mnemonic(i, "SRLV"); type=SHIFT; break;
          case 0x07: set_mnemonic(i, "SRAV"); type=SHIFT; break;
          case 0x08: set_mnemonic(i, "JR"); type=RJUMP; break;
          case 0x09: set_mnemonic(i, "JALR"); type=RJUMP; break;
          case 0x0C: set_mnemonic(i, "SYSCALL"); type=SYSCALL; break;
          case 0x0D: set_mnemonic(i, "BREAK"); type=SYSCALL; break;
          case 0x10: set_mnemonic(i, "MFHI"); type=MOV; break;
          case 0x11: set_mnemonic(i, "MTHI"); type=MOV; break;
          case 0x12: set_mnemonic(i, "MFLO"); type=MOV; break;
          case 0x13: set_mnemonic(i, "MTLO"); type=MOV; break;
          case 0x18: set_mnemonic(i, "MULT"); type=MULTDIV; break;
          case 0x19: set_mnemonic(i, "MULTU"); type=MULTDIV; break;
          case 0x1A: set_mnemonic(i, "DIV"); type=MULTDIV; break;
          case 0x1B: set_mnemonic(i, "DIVU"); type=MULTDIV; break;
          case 0x20: set_mnemonic(i, "ADD"); type=ALU; break;
          case 0x21: set_mnemonic(i, "ADDU"); type=ALU; break;
          case 0x22: set_mnemonic(i, "SUB"); type=ALU; break;
          case 0x23: set_mnemonic(i, "SUBU"); type=ALU; break;
          case 0x24: set_mnemonic(i, "AND"); type=ALU; break;
          case 0x25: set_mnemonic(i, "OR"); type=ALU; break;
          case 0x26: set_mnemonic(i, "XOR"); type=ALU; break;
          case 0x27: set_mnemonic(i, "NOR"); type=ALU; break;
          case 0x2A: set_mnemonic(i, "SLT"); type=ALU; break;
          case 0x2B: set_mnemonic(i, "SLTU"); type=ALU; break;
        }
        break;
      case 0x01: set_mnemonic(i, "regimm");
        type = SJUMP;
        op2 = (src >> 16) & 0x1f;
        switch(op2)
        {
          case 0x10: set_mnemonic(i, "BLTZAL"); break;
          case 0x11: set_mnemonic(i, "BGEZAL"); break;
          default:
            if (op2 & 1)
              set_mnemonic(i, "BGEZ");
            else
              set_mnemonic(i, "BLTZ");
        }
        break;
      case 0x02: set_mnemonic(i, "J"); type=UJUMP; break;
      case 0x03: set_mnemonic(i, "JAL"); type=UJUMP; break;
      case 0x04: set_mnemonic(i, "BEQ"); type=CJUMP; break;
      case 0x05: set_mnemonic(i, "BNE"); type=CJUMP; break;
      case 0x06: set_mnemonic(i, "BLEZ"); type=CJUMP; break;
      case 0x07: set_mnemonic(i, "BGTZ"); type=CJUMP; break;
      case 0x08: set_mnemonic(i, "ADDI"); type=IMM16; break;
      case 0x09: set_mnemonic(i, "ADDIU"); type=IMM16; break;
      case 0x0A: set_mnemonic(i, "SLTI"); type=IMM16; break;
      case 0x0B: set_mnemonic(i, "SLTIU"); type=IMM16; break;
      case 0x0C: set_mnemonic(i, "ANDI"); type=IMM16; break;
      case 0x0D: set_mnemonic(i, "ORI"); type=IMM16; break;
      case 0x0E: set_mnemonic(i, "XORI"); type=IMM16; break;
      case 0x0F: set_mnemonic(i, "LUI"); type=IMM16; break;
      case 0x10: set_mnemonic(i, "COP0");
        op2 = (src >> 21) & 0x1f;
	if (op2 & 0x10) {
          op3 = src & 0x1f;
          switch (op3)
          {
            case 0x01: case 0x02: case 0x06: case 0x08: type = INTCALL; break;
            case 0x10: set_mnemonic(i, "RFE"); type=RFE; break;
            default:   type = OTHER; break;
          }
          break;
        }
        switch(op2)
        {
          u32 rd;
          case 0x00:
            set_mnemonic(i, "MFC0");
            rd = (src >> 11) & 0x1F;
            if (!(0x00000417u & (1u << rd)))
              type = COP0;
            break;
          case 0x04: set_mnemonic(i, "MTC0"); type=COP0; break;
          case 0x02:
          case 0x06: type = INTCALL; break;
          default:   type = OTHER; break;
        }
        break;
      case 0x11: set_mnemonic(i, "COP1");
        op2 = (src >> 21) & 0x1f;
        break;
      case 0x12: set_mnemonic(i, "COP2");
        op2 = (src >> 21) & 0x1f;
        if (op2 & 0x10) {
          type = OTHER;
          if (gte_handlers[src & 0x3f] != NULL) {
#ifdef DISASM
            if (gte_regnames[src & 0x3f] != NULL)
              strcpy(insn[i], gte_regnames[src & 0x3f]);
            else
              snprintf(insn[i], sizeof(insn[i]), "COP2 %x", src & 0x3f);
#endif
            type = C2OP;
          }
        }
        else switch(op2)
        {
          case 0x00: set_mnemonic(i, "MFC2"); type=COP2; break;
          case 0x02: set_mnemonic(i, "CFC2"); type=COP2; break;
          case 0x04: set_mnemonic(i, "MTC2"); type=COP2; break;
          case 0x06: set_mnemonic(i, "CTC2"); type=COP2; break;
        }
        break;
      case 0x13: set_mnemonic(i, "COP3");
        op2 = (src >> 21) & 0x1f;
        break;
      case 0x20: set_mnemonic(i, "LB"); type=LOAD; ls_type = LS_8; break;
      case 0x21: set_mnemonic(i, "LH"); type=LOAD; ls_type = LS_16; break;
      case 0x22: set_mnemonic(i, "LWL"); type=LOADLR; ls_type = LS_LR; break;
      case 0x23: set_mnemonic(i, "LW"); type=LOAD; ls_type = LS_32; break;
      case 0x24: set_mnemonic(i, "LBU"); type=LOAD; ls_type = LS_8; break;
      case 0x25: set_mnemonic(i, "LHU"); type=LOAD; ls_type = LS_16; break;
      case 0x26: set_mnemonic(i, "LWR"); type=LOADLR; ls_type = LS_LR; break;
      case 0x28: set_mnemonic(i, "SB"); type=STORE; ls_type = LS_8; break;
      case 0x29: set_mnemonic(i, "SH"); type=STORE; ls_type = LS_16; break;
      case 0x2A: set_mnemonic(i, "SWL"); type=STORELR; ls_type = LS_LR; break;
      case 0x2B: set_mnemonic(i, "SW"); type=STORE; ls_type = LS_32; break;
      case 0x2E: set_mnemonic(i, "SWR"); type=STORELR; ls_type = LS_LR; break;
      case 0x32: set_mnemonic(i, "LWC2"); type=C2LS; ls_type = LS_32; break;
      case 0x3A: set_mnemonic(i, "SWC2"); type=C2LS; ls_type = LS_32; break;
      case 0x3B:
        if (Config.HLE && (src & 0x03ffffff) < ARRAY_SIZE(psxHLEt)) {
          set_mnemonic(i, "HLECALL");
          type = HLECALL;
        }
        break;
      default:
        break;
    }
    if (type == INTCALL)
      SysPrintf_lim("NI %08x @%08x (%08x)\n", src, start + i*4, start);
    dops[i].itype = type;
    dops[i].opcode2 = op2;
    dops[i].ls_type = ls_type;
    /* Get registers/immediates */
    dops[i].use_lt1=0;
    gte_rs[i]=gte_rt[i]=0;
    dops[i].rs1 = 0;
    dops[i].rs2 = 0;
    dops[i].rt1 = 0;
    dops[i].rt2 = 0;
    switch(type) {
      case LOAD:
        dops[i].rs1 = (src >> 21) & 0x1f;
        dops[i].rt1 = (src >> 16) & 0x1f;
        cinfo[i].imm = (short)src;
        break;
      case STORE:
      case STORELR:
        dops[i].rs1 = (src >> 21) & 0x1f;
        dops[i].rs2 = (src >> 16) & 0x1f;
        cinfo[i].imm = (short)src;
        break;
      case LOADLR:
        // LWL/LWR only load part of the register,
        // therefore the target register must be treated as a source too
        dops[i].rs1 = (src >> 21) & 0x1f;
        dops[i].rs2 = (src >> 16) & 0x1f;
        dops[i].rt1 = (src >> 16) & 0x1f;
        cinfo[i].imm = (short)src;
        break;
      case IMM16:
        if (op==0x0f) dops[i].rs1=0; // LUI instruction has no source register
        else dops[i].rs1 = (src >> 21) & 0x1f;
        dops[i].rs2 = 0;
        dops[i].rt1 = (src >> 16) & 0x1f;
        if(op>=0x0c&&op<=0x0e) { // ANDI/ORI/XORI
          cinfo[i].imm = (unsigned short)src;
        }else{
          cinfo[i].imm = (short)src;
        }
        break;
      case UJUMP:
        // The JAL instruction writes to r31.
        if (op&1) {
          dops[i].rt1=31;
        }
        dops[i].rs2=CCREG;
        break;
      case RJUMP:
        dops[i].rs1 = (src >> 21) & 0x1f;
        // The JALR instruction writes to rd.
        if (op2&1) {
          dops[i].rt1 = (src >> 11) & 0x1f;
        }
        dops[i].rs2=CCREG;
        break;
      case CJUMP:
        dops[i].rs1 = (src >> 21) & 0x1f;
        dops[i].rs2 = (src >> 16) & 0x1f;
        if(op&2) { // BGTZ/BLEZ
          dops[i].rs2=0;
        }
        break;
      case SJUMP:
        dops[i].rs1 = (src >> 21) & 0x1f;
        dops[i].rs2 = CCREG;
        if (op2 == 0x10 || op2 == 0x11) { // BxxAL
          dops[i].rt1 = 31;
          // NOTE: If the branch is not taken, r31 is still overwritten
        }
        break;
      case ALU:
        dops[i].rs1=(src>>21)&0x1f; // source
        dops[i].rs2=(src>>16)&0x1f; // subtract amount
        dops[i].rt1=(src>>11)&0x1f; // destination
        break;
      case MULTDIV:
        dops[i].rs1=(src>>21)&0x1f; // source
        dops[i].rs2=(src>>16)&0x1f; // divisor
        dops[i].rt1=HIREG;
        dops[i].rt2=LOREG;
        break;
      case MOV:
        if(op2==0x10) dops[i].rs1=HIREG; // MFHI
        if(op2==0x11) dops[i].rt1=HIREG; // MTHI
        if(op2==0x12) dops[i].rs1=LOREG; // MFLO
        if(op2==0x13) dops[i].rt1=LOREG; // MTLO
        if((op2&0x1d)==0x10) dops[i].rt1=(src>>11)&0x1f; // MFxx
        if((op2&0x1d)==0x11) dops[i].rs1=(src>>21)&0x1f; // MTxx
        break;
      case SHIFT:
        dops[i].rs1=(src>>16)&0x1f; // target of shift
        dops[i].rs2=(src>>21)&0x1f; // shift amount
        dops[i].rt1=(src>>11)&0x1f; // destination
        break;
      case SHIFTIMM:
        dops[i].rs1=(src>>16)&0x1f;
        dops[i].rs2=0;
        dops[i].rt1=(src>>11)&0x1f;
        cinfo[i].imm=(src>>6)&0x1f;
        break;
      case COP0:
        if(op2==0) dops[i].rt1=(src>>16)&0x1F; // MFC0
        if(op2==4) dops[i].rs1=(src>>16)&0x1F; // MTC0
        if(op2==4&&((src>>11)&0x1e)==12) dops[i].rs2=CCREG;
        break;
      case COP2:
        if(op2<3) dops[i].rt1=(src>>16)&0x1F; // MFC2/CFC2
        if(op2>3) dops[i].rs1=(src>>16)&0x1F; // MTC2/CTC2
        int gr=(src>>11)&0x1F;
        switch(op2)
        {
          case 0x00: gte_rs[i]=1ll<<gr; break; // MFC2
          case 0x04: gte_rt[i]=1ll<<gr; break; // MTC2
          case 0x02: gte_rs[i]=1ll<<(gr+32); break; // CFC2
          case 0x06: gte_rt[i]=1ll<<(gr+32); break; // CTC2
        }
        break;
      case C2LS:
        dops[i].rs1=(src>>21)&0x1F;
        cinfo[i].imm=(short)src;
        if(op==0x32) gte_rt[i]=1ll<<((src>>16)&0x1F); // LWC2
        else gte_rs[i]=1ll<<((src>>16)&0x1F); // SWC2
        break;
      case C2OP:
        gte_rs[i]=gte_reg_reads[src&0x3f];
        gte_rt[i]=gte_reg_writes[src&0x3f];
        gte_rt[i]|=1ll<<63; // every op changes flags
        if((src&0x3f)==GTE_MVMVA) {
          int v = (src >> 15) & 3;
          gte_rs[i]&=~0xe3fll;
          if(v==3) gte_rs[i]|=0xe00ll;
          else gte_rs[i]|=3ll<<(v*2);
        }
        break;
      case SYSCALL:
      case HLECALL:
      case INTCALL:
        dops[i].rs1=CCREG;
        break;
      default:
        break;
    }
}

static noinline void pass1a_disassemble(u_int pagelimit)
{
  int i, j, done = 0;
  int ds_next = 0;

  for (i = 0; !done; i++)
  {
    int force_j_to_interpreter = 0;
    unsigned int type, op, op2;

    disassemble_one(i, source[i]);
    dops[i].is_ds = ds_next; ds_next = 0;
    type = dops[i].itype;
    op = dops[i].opcode;
    op2 = dops[i].opcode2;

    /* Calculate branch target addresses */
    if(type==UJUMP)
      cinfo[i].ba=((start+i*4+4)&0xF0000000)|(((unsigned int)source[i]<<6)>>4);
    else if(type==CJUMP&&dops[i].rs1==dops[i].rs2&&(op&1))
      cinfo[i].ba=start+i*4+8; // Ignore never taken branch
    else if(type==SJUMP&&dops[i].rs1==0&&!(op2&1))
      cinfo[i].ba=start+i*4+8; // Ignore never taken branch
    else if(type==CJUMP||type==SJUMP)
      cinfo[i].ba=start+i*4+4+((signed int)((unsigned int)source[i]<<16)>>14);

    /* simplify always (not)taken branches */
    if (type == CJUMP && dops[i].rs1 == dops[i].rs2) {
      dops[i].rs1 = dops[i].rs2 = 0;
      if (!(op & 1)) {
        dops[i].itype = type = UJUMP;
        dops[i].rs2 = CCREG;
      }
    }
    else if (type == SJUMP && dops[i].rs1 == 0 && (op2 & 1))
      dops[i].itype = type = UJUMP;

    dops[i].is_jump  = type == RJUMP || type == UJUMP || type == CJUMP || type == SJUMP;
    dops[i].is_ujump = type == RJUMP || type == UJUMP;
    dops[i].is_load  = type == LOAD || type == LOADLR || op == 0x32; // LWC2
    dops[i].is_delay_load = (dops[i].is_load || (source[i] & 0xf3d00000) == 0x40000000); // MFC/CFC
    dops[i].is_store = type == STORE || type == STORELR || op == 0x3a; // SWC2
    dops[i].is_exception = type == SYSCALL || type == HLECALL || type == INTCALL;
    dops[i].may_except = dops[i].is_exception || (type == ALU && (op2 == 0x20 || op2 == 0x22)) || op == 8;
    ds_next = dops[i].is_jump;

    if (((op & 0x37) == 0x21 || op == 0x25) // LH/SH/LHU
        && ((cinfo[i].imm & 1) || Config.PreciseExceptions))
      dops[i].may_except = 1;
    if (((op & 0x37) == 0x23 || (op & 0x37) == 0x32) // LW/SW/LWC2/SWC2
        && ((cinfo[i].imm & 3) || Config.PreciseExceptions))
      dops[i].may_except = 1;

    /* rare messy cases to just pass over to the interpreter */
    if (i > 0 && dops[i-1].is_jump) {
      j = i - 1;
      // branch in delay slot?
      if (dops[i].is_jump) {
        // don't handle first branch and call interpreter if it's hit
        SysPrintf_lim("branch in DS @%08x (%08x)\n", start + i*4, start);
        force_j_to_interpreter = 1;
      }
      // load delay detection through a branch
      else if (dops[i].is_delay_load && dops[i].rt1 != 0) {
        const struct decoded_insn *dop = NULL;
        int t = -1;
        if (cinfo[i-1].ba != -1) {
          t = (cinfo[i-1].ba - start) / 4;
          if (t < 0 || t > i) {
            u_int limit = 0;
            u_int *mem = get_source_start(cinfo[i-1].ba, &limit);
            if (mem != NULL) {
              disassemble_one(MAXBLOCK - 1, mem[0]);
              dop = &dops[MAXBLOCK - 1];
            }
          }
          else
            dop = &dops[t];
        }
        if ((dop && is_ld_use_hazard(&dops[i], dop))
            || (!dop && Config.PreciseExceptions)) {
          // jump target wants DS result - potential load delay effect
          SysPrintf_lim("load delay in DS @%08x (%08x)\n", start + i*4, start);
          force_j_to_interpreter = 1;
          if (0 <= t && t < i)
            dops[t + 1].bt = 1; // expected return from interpreter
        }
        else if(i>=2&&dops[i-2].rt1==2&&dops[i].rt1==2&&dops[i].rs1!=2&&dops[i].rs2!=2&&dops[i-1].rs1!=2&&dops[i-1].rs2!=2&&
              !(i>=3&&dops[i-3].is_jump)) {
          // v0 overwrite like this is a sign of trouble, bail out
          SysPrintf_lim("v0 overwrite @%08x (%08x)\n", start + i*4, start);
          force_j_to_interpreter = 1;
        }
      }
    }
    else if (i > 0 && dops[i-1].is_delay_load
             && is_ld_use_hazard(&dops[i-1], &dops[i])
             && (i < 2 || !dops[i-2].is_ujump)) {
      SysPrintf_lim("load delay @%08x (%08x)\n", start + i*4, start);
      for (j = i - 1; j > 0 && dops[j-1].is_delay_load; j--)
        if (dops[j-1].rt1 != dops[i-1].rt1)
          break;
      force_j_to_interpreter = 1;
    }
    if (force_j_to_interpreter) {
      force_intcall(j);
      done = 2;
      i = j; // don't compile the problematic branch/load/etc
    }
    if (dops[i].is_exception && i > 0 && dops[i-1].is_jump) {
      SysPrintf_lim("exception in DS @%08x (%08x)\n", start + i*4, start);
      i--;
      force_intcall(i);
      done = 2;
    }
    if (i >= 2) {
      if ((source[i-2] & 0xffe0f800) == 0x40806000 // MTC0 $12
          || (dops[i-2].is_jump && dops[i-2].rt1 == 31)) // call
      dops[i].bt = 1;
    }
    if (i >= 1 && (source[i-1] & 0xffe0f800) == 0x40806800) // MTC0 $13
      dops[i].bt = 1;

    /* Is this the end of the block? */
    if (i > 0 && dops[i-1].is_ujump) {
      // Don't recompile stuff that's already compiled
      if (check_addr(start + i*4+4)) {
        done = 1;
        continue;
      }
      // Don't get too close to the limit
      if (i > MAXBLOCK - 64)
        done = 2;
      if (dops[i-1].opcode2 == 0x08 || dops[i-1].rs1 == 31) // JR; JALR x, lr
        done = 2;
      else if (dops[i-1].itype != RJUMP && dops[i-1].rt1 == 0) { // not JAL(R)
        int found_bbranch = 0, t = (cinfo[i-1].ba - start) / 4;
        if ((u_int)(t - i) < 64 && start + (t+64)*4 < pagelimit) {
          // scan for a branch back to i+1
          for (j = t; j < t + 64; j++) {
            int tmpop = source[j] >> 26;
            if (tmpop == 1 || ((tmpop & ~3) == 4)) {
              int t2 = j + 1 + (int)(signed short)source[j];
              if (t2 == i + 1) {
                //printf("blk expand %08x<-%08x\n", start + (i+1)*4, start + j*4);
                found_bbranch = 1;
                break;
              }
            }
          }
        }
        if (!found_bbranch)
          done = 2;
      }
      else {
        // jal(r) - continue or perf may suffer for platforms without
        // runtime block linking (like in crash3)
        if (stop_after_jal)
          done = 2;
      }
    }
    if (dops[i].itype == HLECALL)
      done = 1;
    else if (dops[i].itype == INTCALL) {
      ni_count++;
      done = 2;
    }
    else if (dops[i].is_exception)
      done = 2;
    if (done == 2) {
      // Does the block continue due to a branch?
      for (j = i-1; j >= 0; j--) {
        if (cinfo[j].ba == start+i*4) done=j=0; // Branch into delay slot
        if (cinfo[j].ba == start+i*4+4) done=j=0;
        if (cinfo[j].ba == start+i*4+8) done=j=0;
      }
    }
    //assert(i<MAXBLOCK-1);
    if(start+i*4==pagelimit-4) done=1;
    assert(start+i*4<pagelimit);
    if (i == MAXBLOCK - 2)
      done = 1;
  }
  if (ni_count > 32 && !stop_after_jal) {
    stop_after_jal = 1;
    SysPrintf("Disabled speculative precompilation\n");
  }
  while (i > 0 && dops[i-1].is_jump)
    i--;
  assert(i > 0);
  assert(!dops[i-1].is_jump);
  slen = i;
}

static noinline void pass1b_bt(void)
{
  int i;
  for (i = 0; i < slen; i++)
    if (dops[i].is_jump && start <= cinfo[i].ba && cinfo[i].ba < start+slen*4)
      // Internal branch, flag target
      dops[(cinfo[i].ba - start) >> 2].bt = 1;
}

// Basic liveness analysis for MIPS registers
static noinline void pass2b_unneeded_regs(int istart, int iend, int r)
{
  int i;
  uint64_t u,gte_u,b,gte_b;
  uint64_t temp_u,temp_gte_u=0;
  uint64_t gte_u_unknown=0;
  if (HACK_ENABLED(NDHACK_GTE_UNNEEDED))
    gte_u_unknown=~0ll;
  if(iend==slen-1) {
    u=1;
    gte_u=gte_u_unknown;
  }else{
    //u=unneeded_reg[iend+1];
    u=1;
    gte_u=gte_unneeded[iend+1];
  }

  for (i=iend;i>=istart;i--)
  {
    //printf("unneeded registers i=%d (%d,%d) r=%d\n",i,istart,iend,r);
    if(dops[i].is_jump)
    {
      if(cinfo[i].ba<start || cinfo[i].ba>=(start+slen*4))
      {
        // Branch out of this block, flush all regs
        u=1;
        gte_u=gte_u_unknown;
        branch_unneeded_reg[i]=u;
        // Merge in delay slot
        u|=(1LL<<dops[i+1].rt1)|(1LL<<dops[i+1].rt2);
        u&=~((1LL<<dops[i+1].rs1)|(1LL<<dops[i+1].rs2));
        u|=1;
        gte_u|=gte_rt[i+1];
        gte_u&=~gte_rs[i+1];
      }
      else
      {
        if(cinfo[i].ba<=start+i*4) {
          // Backward branch
          if(dops[i].is_ujump)
          {
            // Unconditional branch
            temp_u=1;
            temp_gte_u=0;
          } else {
            // Conditional branch (not taken case)
            temp_u=unneeded_reg[i+2];
            temp_gte_u&=gte_unneeded[i+2];
          }
          // Merge in delay slot
          temp_u|=(1LL<<dops[i+1].rt1)|(1LL<<dops[i+1].rt2);
          temp_u&=~((1LL<<dops[i+1].rs1)|(1LL<<dops[i+1].rs2));
          temp_u|=1;
          temp_gte_u|=gte_rt[i+1];
          temp_gte_u&=~gte_rs[i+1];
          temp_u|=(1LL<<dops[i].rt1)|(1LL<<dops[i].rt2);
          temp_u&=~((1LL<<dops[i].rs1)|(1LL<<dops[i].rs2));
          temp_u|=1;
          temp_gte_u|=gte_rt[i];
          temp_gte_u&=~gte_rs[i];
          unneeded_reg[i]=temp_u;
          gte_unneeded[i]=temp_gte_u;
          // Only go three levels deep.  This recursion can take an
          // excessive amount of time if there are a lot of nested loops.
          if(r<2) {
            pass2b_unneeded_regs((cinfo[i].ba-start)>>2, i-1, r+1);
          }else{
            unneeded_reg[(cinfo[i].ba-start)>>2]=1;
            gte_unneeded[(cinfo[i].ba-start)>>2]=gte_u_unknown;
          }
        } /*else*/ if(1) {
          if (dops[i].is_ujump)
          {
            // Unconditional branch
            u=unneeded_reg[(cinfo[i].ba-start)>>2];
            gte_u=gte_unneeded[(cinfo[i].ba-start)>>2];
            branch_unneeded_reg[i]=u;
            // Merge in delay slot
            u|=(1LL<<dops[i+1].rt1)|(1LL<<dops[i+1].rt2);
            u&=~((1LL<<dops[i+1].rs1)|(1LL<<dops[i+1].rs2));
            u|=1;
            gte_u|=gte_rt[i+1];
            gte_u&=~gte_rs[i+1];
          } else {
            // Conditional branch
            b=unneeded_reg[(cinfo[i].ba-start)>>2];
            gte_b=gte_unneeded[(cinfo[i].ba-start)>>2];
            branch_unneeded_reg[i]=b;
            // Branch delay slot
            b|=(1LL<<dops[i+1].rt1)|(1LL<<dops[i+1].rt2);
            b&=~((1LL<<dops[i+1].rs1)|(1LL<<dops[i+1].rs2));
            b|=1;
            gte_b|=gte_rt[i+1];
            gte_b&=~gte_rs[i+1];
            u&=b;
            gte_u&=gte_b;
            if(i<slen-1) {
              branch_unneeded_reg[i]&=unneeded_reg[i+2];
            } else {
              branch_unneeded_reg[i]=1;
            }
          }
        }
      }
    }
    //u=1; // DEBUG
    // Written registers are unneeded
    u|=1LL<<dops[i].rt1;
    u|=1LL<<dops[i].rt2;
    gte_u|=gte_rt[i];
    // Accessed registers are needed
    u&=~(1LL<<dops[i].rs1);
    u&=~(1LL<<dops[i].rs2);
    gte_u&=~gte_rs[i];
    if(gte_rs[i]&&dops[i].rt1&&(unneeded_reg[i+1]&(1ll<<dops[i].rt1)))
      gte_u|=gte_rs[i]&gte_unneeded[i+1]; // MFC2/CFC2 to dead register, unneeded
    if (dops[i].may_except || dops[i].itype == RFE)
    {
      // SYSCALL instruction, etc or conditional exception
      u=1;
    }
    // Source-target dependencies
    // R0 is always unneeded
    u|=1;
    // Save it
    unneeded_reg[i]=u;
    gte_unneeded[i]=gte_u;
    /*
    printf("ur (%d,%d) %x: ",istart,iend,start+i*4);
    printf("U:");
    int r;
    for(r=1;r<=CCREG;r++) {
      if((unneeded_reg[i]>>r)&1) {
        if(r==HIREG) printf(" HI");
        else if(r==LOREG) printf(" LO");
        else printf(" r%d",r);
      }
    }
    printf("\n");
    */
  }
}

static noinline void pass2a_unneeded(void)
{
  int i, j;
  for (i = 0; i < slen; i++)
  {
    // remove redundant alignment checks
    if (dops[i].may_except && (dops[i].is_load || dops[i].is_store)
        && dops[i].rt1 != dops[i].rs1 && !dops[i].is_ds)
    {
      int base = dops[i].rs1, lsb = cinfo[i].imm, ls_type = dops[i].ls_type;
      int mask = ls_type == LS_32 ? 3 : 1;
      lsb &= mask;
      for (j = i + 1; j < slen; j++) {
        if (dops[j].bt || dops[j].is_jump)
          break;
        if ((dops[j].is_load || dops[j].is_store) && dops[j].rs1 == base
            && dops[j].ls_type == ls_type && (cinfo[j].imm & mask) == lsb)
          dops[j].may_except = 0;
        if (dops[j].rt1 == base)
          break;
      }
    }
    // rm redundant stack loads (unoptimized code, assuming no io mem access through sp)
    if (i > 0 && dops[i].is_load && dops[i].rs1 == 29 && dops[i].ls_type == LS_32
        && dops[i-1].is_store && dops[i-1].rs1 == 29 && dops[i-1].ls_type == LS_32
        && dops[i-1].rs2 == dops[i].rt1 && !dops[i-1].is_ds && i < slen - 1
        && dops[i+1].rs1 != dops[i].rt1 && dops[i+1].rs2 != dops[i].rt1
        && !dops[i].bt && cinfo[i].imm == cinfo[i-1].imm)
    {
      cinfo[i].imm = 0;
      memset(&dops[i], 0, sizeof(dops[i]));
      dops[i].itype = NOP;
    }
  }
}

static noinline void pass3_register_alloc(u_int addr)
{
  struct regstat current; // Current register allocations/status
  clear_all_regs(current.regmap_entry);
  clear_all_regs(current.regmap);
  current.wasdirty = current.dirty = 0;
  current.u = unneeded_reg[0];
  alloc_reg(&current, 0, CCREG);
  dirty_reg(&current, CCREG);
  current.wasconst = 0;
  current.isconst = 0;
  current.loadedconst = 0;
  current.noevict = 0;
  //current.waswritten = 0;
  int ds=0;
  int cc=0;
  int hr;
  int i, j;

  if (addr & 1) {
    // First instruction is delay slot
    cc=-1;
    dops[1].bt=1;
    ds=1;
    unneeded_reg[0]=1;
  }

  for(i=0;i<slen;i++)
  {
    if(dops[i].bt)
    {
      for(hr=0;hr<HOST_REGS;hr++)
      {
        // Is this really necessary?
        if(current.regmap[hr]==0) current.regmap[hr]=-1;
      }
      current.isconst=0;
      //current.waswritten=0;
    }

    memcpy(regmap_pre[i],current.regmap,sizeof(current.regmap));
    regs[i].wasconst=current.isconst;
    regs[i].wasdirty=current.dirty;
    regs[i].dirty=0;
    regs[i].u=0;
    regs[i].isconst=0;
    regs[i].loadedconst=0;
    if (!dops[i].is_jump) {
      if(i+1<slen) {
        current.u=unneeded_reg[i+1]&~((1LL<<dops[i].rs1)|(1LL<<dops[i].rs2));
        current.u|=1;
      } else {
        current.u=1;
      }
    } else {
      if(i+1<slen) {
        current.u=branch_unneeded_reg[i]&~((1LL<<dops[i+1].rs1)|(1LL<<dops[i+1].rs2));
        current.u&=~((1LL<<dops[i].rs1)|(1LL<<dops[i].rs2));
        current.u|=1;
      } else {
        SysPrintf("oops, branch at end of block with no delay slot @%08x\n", start + i*4);
        abort();
      }
    }
    assert(dops[i].is_ds == ds);
    if(ds) {
      ds=0; // Skip delay slot, already allocated as part of branch
      // ...but we need to alloc it in case something jumps here
      if(i+1<slen) {
        current.u=branch_unneeded_reg[i-1]&unneeded_reg[i+1];
      }else{
        current.u=branch_unneeded_reg[i-1];
      }
      current.u&=~((1LL<<dops[i].rs1)|(1LL<<dops[i].rs2));
      current.u|=1;
      struct regstat temp;
      memcpy(&temp,&current,sizeof(current));
      temp.wasdirty=temp.dirty;
      // TODO: Take into account unconditional branches, as below
      delayslot_alloc(&temp,i);
      memcpy(regs[i].regmap,temp.regmap,sizeof(temp.regmap));
      regs[i].wasdirty=temp.wasdirty;
      regs[i].dirty=temp.dirty;
      regs[i].isconst=0;
      regs[i].wasconst=0;
      current.isconst=0;
      // Create entry (branch target) regmap
      for(hr=0;hr<HOST_REGS;hr++)
      {
        int r=temp.regmap[hr];
        if(r>=0) {
          if(r!=regmap_pre[i][hr]) {
            regs[i].regmap_entry[hr]=-1;
          }
          else
          {
              assert(r < 64);
              if((current.u>>r)&1) {
                regs[i].regmap_entry[hr]=-1;
                regs[i].regmap[hr]=-1;
                //Don't clear regs in the delay slot as the branch might need them
                //current.regmap[hr]=-1;
              }else
                regs[i].regmap_entry[hr]=r;
          }
        } else {
          // First instruction expects CCREG to be allocated
          if(i==0&&hr==HOST_CCREG)
            regs[i].regmap_entry[hr]=CCREG;
          else
            regs[i].regmap_entry[hr]=-1;
        }
      }
    }
    else { // Not delay slot
      current.noevict = 0;
      switch(dops[i].itype) {
        case UJUMP:
          //current.isconst=0; // DEBUG
          //current.wasconst=0; // DEBUG
          //regs[i].wasconst=0; // DEBUG
          clear_const(&current,dops[i].rt1);
          alloc_cc(&current,i);
          dirty_reg(&current,CCREG);
          if (dops[i].rt1==31) {
            alloc_reg(&current,i,31);
            dirty_reg(&current,31);
            //assert(dops[i+1].rs1!=31&&dops[i+1].rs2!=31);
            //assert(dops[i+1].rt1!=dops[i].rt1);
            #ifdef REG_PREFETCH
            alloc_reg(&current,i,PTEMP);
            #endif
          }
          dops[i].ooo=1;
          delayslot_alloc(&current,i+1);
          //current.isconst=0; // DEBUG
          ds=1;
          break;
        case RJUMP:
          //current.isconst=0;
          //current.wasconst=0;
          //regs[i].wasconst=0;
          clear_const(&current,dops[i].rs1);
          clear_const(&current,dops[i].rt1);
          alloc_cc(&current,i);
          dirty_reg(&current,CCREG);
          if (!ds_writes_rjump_rs(i)) {
            alloc_reg(&current,i,dops[i].rs1);
            if (dops[i].rt1!=0) {
              alloc_reg(&current,i,dops[i].rt1);
              dirty_reg(&current,dops[i].rt1);
              #ifdef REG_PREFETCH
              alloc_reg(&current,i,PTEMP);
              #endif
            }
            #ifdef USE_MINI_HT
            if(dops[i].rs1==31) { // JALR
              alloc_reg(&current,i,RHASH);
              alloc_reg(&current,i,RHTBL);
            }
            #endif
            delayslot_alloc(&current,i+1);
          } else {
            // The delay slot overwrites our source register,
            // allocate a temporary register to hold the old value.
            current.isconst=0;
            current.wasconst=0;
            regs[i].wasconst=0;
            delayslot_alloc(&current,i+1);
            current.isconst=0;
            alloc_reg(&current,i,RTEMP);
          }
          //current.isconst=0; // DEBUG
          dops[i].ooo=1;
          ds=1;
          break;
        case CJUMP:
          //current.isconst=0;
          //current.wasconst=0;
          //regs[i].wasconst=0;
          clear_const(&current,dops[i].rs1);
          clear_const(&current,dops[i].rs2);
          if((dops[i].opcode&0x3E)==4) // BEQ/BNE
          {
            alloc_cc(&current,i);
            dirty_reg(&current,CCREG);
            if(dops[i].rs1) alloc_reg(&current,i,dops[i].rs1);
            if(dops[i].rs2) alloc_reg(&current,i,dops[i].rs2);
            if((dops[i].rs1&&(dops[i].rs1==dops[i+1].rt1||dops[i].rs1==dops[i+1].rt2))||
               (dops[i].rs2&&(dops[i].rs2==dops[i+1].rt1||dops[i].rs2==dops[i+1].rt2))) {
              // The delay slot overwrites one of our conditions.
              // Allocate the branch condition registers instead.
              current.isconst=0;
              current.wasconst=0;
              regs[i].wasconst=0;
              if(dops[i].rs1) alloc_reg(&current,i,dops[i].rs1);
              if(dops[i].rs2) alloc_reg(&current,i,dops[i].rs2);
            }
            else
            {
              dops[i].ooo=1;
              delayslot_alloc(&current,i+1);
            }
          }
          else
          if((dops[i].opcode&0x3E)==6) // BLEZ/BGTZ
          {
            alloc_cc(&current,i);
            dirty_reg(&current,CCREG);
            alloc_reg(&current,i,dops[i].rs1);
            if(dops[i].rs1&&(dops[i].rs1==dops[i+1].rt1||dops[i].rs1==dops[i+1].rt2)) {
              // The delay slot overwrites one of our conditions.
              // Allocate the branch condition registers instead.
              current.isconst=0;
              current.wasconst=0;
              regs[i].wasconst=0;
              if(dops[i].rs1) alloc_reg(&current,i,dops[i].rs1);
            }
            else
            {
              dops[i].ooo=1;
              delayslot_alloc(&current,i+1);
            }
          }
          else
          // Don't alloc the delay slot yet because we might not execute it
          if((dops[i].opcode&0x3E)==0x14) // BEQL/BNEL
          {
            current.isconst=0;
            current.wasconst=0;
            regs[i].wasconst=0;
            alloc_cc(&current,i);
            dirty_reg(&current,CCREG);
            alloc_reg(&current,i,dops[i].rs1);
            alloc_reg(&current,i,dops[i].rs2);
          }
          else
          if((dops[i].opcode&0x3E)==0x16) // BLEZL/BGTZL
          {
            current.isconst=0;
            current.wasconst=0;
            regs[i].wasconst=0;
            alloc_cc(&current,i);
            dirty_reg(&current,CCREG);
            alloc_reg(&current,i,dops[i].rs1);
          }
          ds=1;
          //current.isconst=0;
          break;
        case SJUMP:
          clear_const(&current,dops[i].rs1);
          clear_const(&current,dops[i].rt1);
          {
            alloc_cc(&current,i);
            dirty_reg(&current,CCREG);
            alloc_reg(&current,i,dops[i].rs1);
            if (dops[i].rt1 == 31) { // BLTZAL/BGEZAL
              alloc_reg(&current,i,31);
              dirty_reg(&current,31);
            }
            if ((dops[i].rs1 &&
                 (dops[i].rs1==dops[i+1].rt1||dops[i].rs1==dops[i+1].rt2)) // The delay slot overwrites the branch condition.
               ||(dops[i].rt1 == 31 && dops[i].rs1 == 31) // overwrites it's own condition
               ||(dops[i].rt1==31&&(dops[i+1].rs1==31||dops[i+1].rs2==31||dops[i+1].rt1==31||dops[i+1].rt2==31))) { // DS touches $ra
              // Allocate the branch condition registers instead.
              current.isconst=0;
              current.wasconst=0;
              regs[i].wasconst=0;
              if(dops[i].rs1) alloc_reg(&current,i,dops[i].rs1);
            }
            else
            {
              dops[i].ooo=1;
              delayslot_alloc(&current,i+1);
            }
          }
          ds=1;
          //current.isconst=0;
          break;
        case IMM16:
          imm16_alloc(&current,i);
          break;
        case LOAD:
        case LOADLR:
          load_alloc(&current,i);
          break;
        case STORE:
        case STORELR:
          store_alloc(&current,i);
          break;
        case ALU:
          alu_alloc(&current,i);
          break;
        case SHIFT:
          shift_alloc(&current,i);
          break;
        case MULTDIV:
          multdiv_alloc(&current,i);
          break;
        case SHIFTIMM:
          shiftimm_alloc(&current,i);
          break;
        case MOV:
          mov_alloc(&current,i);
          break;
        case COP0:
          cop0_alloc(&current,i);
          break;
        case RFE:
          rfe_alloc(&current,i);
          break;
        case COP2:
          cop2_alloc(&current,i);
          break;
        case C2LS:
          c2ls_alloc(&current,i);
          break;
        case C2OP:
          c2op_alloc(&current,i);
          break;
        case SYSCALL:
        case HLECALL:
        case INTCALL:
          syscall_alloc(&current,i);
          break;
      }

      // Create entry (branch target) regmap
      for(hr=0;hr<HOST_REGS;hr++)
      {
        int r,or;
        r=current.regmap[hr];
        if(r>=0) {
          if(r!=regmap_pre[i][hr]) {
            // TODO: delay slot (?)
            or=get_reg(regmap_pre[i],r); // Get old mapping for this register
            if(or<0||r>=TEMPREG){
              regs[i].regmap_entry[hr]=-1;
            }
            else
            {
              // Just move it to a different register
              regs[i].regmap_entry[hr]=r;
              // If it was dirty before, it's still dirty
              if((regs[i].wasdirty>>or)&1) dirty_reg(&current,r);
            }
          }
          else
          {
            // Unneeded
            if(r==0){
              regs[i].regmap_entry[hr]=0;
            }
            else
            {
              assert(r<64);
              if((current.u>>r)&1) {
                regs[i].regmap_entry[hr]=-1;
                //regs[i].regmap[hr]=-1;
                current.regmap[hr]=-1;
              }else
                regs[i].regmap_entry[hr]=r;
            }
          }
        } else {
          // Branches expect CCREG to be allocated at the target
          if(regmap_pre[i][hr]==CCREG)
            regs[i].regmap_entry[hr]=CCREG;
          else
            regs[i].regmap_entry[hr]=-1;
        }
      }
      memcpy(regs[i].regmap,current.regmap,sizeof(current.regmap));
    }

#if 0 // see do_store_smc_check()
    if(i>0&&(dops[i-1].itype==STORE||dops[i-1].itype==STORELR||(dops[i-1].itype==C2LS&&dops[i-1].opcode==0x3a))&&(u_int)cinfo[i-1].imm<0x800)
      current.waswritten|=1<<dops[i-1].rs1;
    current.waswritten&=~(1<<dops[i].rt1);
    current.waswritten&=~(1<<dops[i].rt2);
    if((dops[i].itype==STORE||dops[i].itype==STORELR||(dops[i].itype==C2LS&&dops[i].opcode==0x3a))&&(u_int)cinfo[i].imm>=0x800)
      current.waswritten&=~(1<<dops[i].rs1);
#endif

    /* Branch post-alloc */
    if(i>0)
    {
      current.wasdirty=current.dirty;
      switch(dops[i-1].itype) {
        case UJUMP:
          memcpy(&branch_regs[i-1],&current,sizeof(current));
          branch_regs[i-1].isconst=0;
          branch_regs[i-1].wasconst=0;
          branch_regs[i-1].u=branch_unneeded_reg[i-1]&~((1LL<<dops[i-1].rs1)|(1LL<<dops[i-1].rs2));
          alloc_cc(&branch_regs[i-1],i-1);
          dirty_reg(&branch_regs[i-1],CCREG);
          if(dops[i-1].rt1==31) { // JAL
            alloc_reg(&branch_regs[i-1],i-1,31);
            dirty_reg(&branch_regs[i-1],31);
          }
          memcpy(&branch_regs[i-1].regmap_entry,&branch_regs[i-1].regmap,sizeof(current.regmap));
          memcpy(constmap[i],constmap[i-1],sizeof(constmap[i]));
          break;
        case RJUMP:
          memcpy(&branch_regs[i-1],&current,sizeof(current));
          branch_regs[i-1].isconst=0;
          branch_regs[i-1].wasconst=0;
          branch_regs[i-1].u=branch_unneeded_reg[i-1]&~((1LL<<dops[i-1].rs1)|(1LL<<dops[i-1].rs2));
          alloc_cc(&branch_regs[i-1],i-1);
          dirty_reg(&branch_regs[i-1],CCREG);
          alloc_reg(&branch_regs[i-1],i-1,dops[i-1].rs1);
          if(dops[i-1].rt1!=0) { // JALR
            alloc_reg(&branch_regs[i-1],i-1,dops[i-1].rt1);
            dirty_reg(&branch_regs[i-1],dops[i-1].rt1);
          }
          #ifdef USE_MINI_HT
          if(dops[i-1].rs1==31) { // JALR
            alloc_reg(&branch_regs[i-1],i-1,RHASH);
            alloc_reg(&branch_regs[i-1],i-1,RHTBL);
          }
          #endif
          memcpy(&branch_regs[i-1].regmap_entry,&branch_regs[i-1].regmap,sizeof(current.regmap));
          memcpy(constmap[i],constmap[i-1],sizeof(constmap[i]));
          break;
        case CJUMP:
          if((dops[i-1].opcode&0x3E)==4) // BEQ/BNE
          {
            alloc_cc(&current,i-1);
            dirty_reg(&current,CCREG);
            if((dops[i-1].rs1&&(dops[i-1].rs1==dops[i].rt1||dops[i-1].rs1==dops[i].rt2))||
               (dops[i-1].rs2&&(dops[i-1].rs2==dops[i].rt1||dops[i-1].rs2==dops[i].rt2))) {
              // The delay slot overwrote one of our conditions
              // Delay slot goes after the test (in order)
              current.u=branch_unneeded_reg[i-1]&~((1LL<<dops[i].rs1)|(1LL<<dops[i].rs2));
              current.u|=1;
              delayslot_alloc(&current,i);
              current.isconst=0;
            }
            else
            {
              current.u=branch_unneeded_reg[i-1]&~((1LL<<dops[i-1].rs1)|(1LL<<dops[i-1].rs2));
              // Alloc the branch condition registers
              if(dops[i-1].rs1) alloc_reg(&current,i-1,dops[i-1].rs1);
              if(dops[i-1].rs2) alloc_reg(&current,i-1,dops[i-1].rs2);
            }
            memcpy(&branch_regs[i-1],&current,sizeof(current));
            branch_regs[i-1].isconst=0;
            branch_regs[i-1].wasconst=0;
            memcpy(&branch_regs[i-1].regmap_entry,&current.regmap,sizeof(current.regmap));
            memcpy(constmap[i],constmap[i-1],sizeof(constmap[i]));
          }
          else
          if((dops[i-1].opcode&0x3E)==6) // BLEZ/BGTZ
          {
            alloc_cc(&current,i-1);
            dirty_reg(&current,CCREG);
            if(dops[i-1].rs1==dops[i].rt1||dops[i-1].rs1==dops[i].rt2) {
              // The delay slot overwrote the branch condition
              // Delay slot goes after the test (in order)
              current.u=branch_unneeded_reg[i-1]&~((1LL<<dops[i].rs1)|(1LL<<dops[i].rs2));
              current.u|=1;
              delayslot_alloc(&current,i);
              current.isconst=0;
            }
            else
            {
              current.u=branch_unneeded_reg[i-1]&~(1LL<<dops[i-1].rs1);
              // Alloc the branch condition register
              alloc_reg(&current,i-1,dops[i-1].rs1);
            }
            memcpy(&branch_regs[i-1],&current,sizeof(current));
            branch_regs[i-1].isconst=0;
            branch_regs[i-1].wasconst=0;
            memcpy(&branch_regs[i-1].regmap_entry,&current.regmap,sizeof(current.regmap));
            memcpy(constmap[i],constmap[i-1],sizeof(constmap[i]));
          }
          break;
        case SJUMP:
          {
            alloc_cc(&current,i-1);
            dirty_reg(&current,CCREG);
            if(dops[i-1].rs1==dops[i].rt1||dops[i-1].rs1==dops[i].rt2) {
              // The delay slot overwrote the branch condition
              // Delay slot goes after the test (in order)
              current.u=branch_unneeded_reg[i-1]&~((1LL<<dops[i].rs1)|(1LL<<dops[i].rs2));
              current.u|=1;
              delayslot_alloc(&current,i);
              current.isconst=0;
            }
            else
            {
              current.u=branch_unneeded_reg[i-1]&~(1LL<<dops[i-1].rs1);
              // Alloc the branch condition register
              alloc_reg(&current,i-1,dops[i-1].rs1);
            }
            memcpy(&branch_regs[i-1],&current,sizeof(current));
            branch_regs[i-1].isconst=0;
            branch_regs[i-1].wasconst=0;
            memcpy(&branch_regs[i-1].regmap_entry,&current.regmap,sizeof(current.regmap));
            memcpy(constmap[i],constmap[i-1],sizeof(constmap[i]));
          }
          break;
      }

      if (dops[i-1].is_ujump)
      {
        if(dops[i-1].rt1==31) // JAL/JALR
        {
          // Subroutine call will return here, don't alloc any registers
          current.dirty=0;
          clear_all_regs(current.regmap);
          alloc_reg(&current,i,CCREG);
          dirty_reg(&current,CCREG);
        }
        else if(i+1<slen)
        {
          // Internal branch will jump here, match registers to caller
          current.dirty=0;
          clear_all_regs(current.regmap);
          alloc_reg(&current,i,CCREG);
          dirty_reg(&current,CCREG);
          for(j=i-1;j>=0;j--)
          {
            if(cinfo[j].ba==start+i*4+4) {
              memcpy(current.regmap,branch_regs[j].regmap,sizeof(current.regmap));
              current.dirty=branch_regs[j].dirty;
              break;
            }
          }
          while(j>=0) {
            if(cinfo[j].ba==start+i*4+4) {
              for(hr=0;hr<HOST_REGS;hr++) {
                if(current.regmap[hr]!=branch_regs[j].regmap[hr]) {
                  current.regmap[hr]=-1;
                }
                current.dirty&=branch_regs[j].dirty;
              }
            }
            j--;
          }
        }
      }
    }

    // Count cycles in between branches
    cinfo[i].ccadj = CLOCK_ADJUST(cc);
    if (i > 0 && (dops[i-1].is_jump || dops[i].is_exception))
    {
      cc=0;
    }
#if !defined(DRC_DBG)
    else if(dops[i].itype==C2OP&&gte_cycletab[source[i]&0x3f]>2)
    {
      // this should really be removed since the real stalls have been implemented,
      // but doing so causes sizeable perf regression against the older version
      u_int gtec = gte_cycletab[source[i] & 0x3f];
      cc += HACK_ENABLED(NDHACK_NO_STALLS) ? gtec/2 : 2;
    }
    else if(i>1&&dops[i].itype==STORE&&dops[i-1].itype==STORE&&dops[i-2].itype==STORE&&!dops[i].bt)
    {
      cc+=4;
    }
    else if(dops[i].itype==C2LS)
    {
      // same as with C2OP
      cc += HACK_ENABLED(NDHACK_NO_STALLS) ? 4 : 2;
    }
#endif
    else
    {
      cc++;
    }

    if(!dops[i].is_ds) {
      regs[i].dirty=current.dirty;
      regs[i].isconst=current.isconst;
      memcpy(constmap[i],current_constmap,sizeof(constmap[i]));
    }
    for(hr=0;hr<HOST_REGS;hr++) {
      if(hr!=EXCLUDE_REG&&regs[i].regmap[hr]>=0) {
        if(regmap_pre[i][hr]!=regs[i].regmap[hr]) {
          regs[i].wasconst&=~(1<<hr);
        }
      }
    }
    //regs[i].waswritten=current.waswritten;
  }
}

static noinline void pass4_cull_unused_regs(void)
{
  u_int last_needed_regs[4] = {0,0,0,0};
  u_int nr=0;
  int i;

  for (i=slen-1;i>=0;i--)
  {
    int hr;
    __builtin_prefetch(regs[i-2].regmap);
    if(dops[i].is_jump)
    {
      if(cinfo[i].ba<start || cinfo[i].ba>=(start+slen*4))
      {
        // Branch out of this block, don't need anything
        nr=0;
      }
      else
      {
        // Internal branch
        // Need whatever matches the target
        nr=0;
        int t=(cinfo[i].ba-start)>>2;
        for(hr=0;hr<HOST_REGS;hr++)
        {
          if(regs[i].regmap_entry[hr]>=0) {
            if(regs[i].regmap_entry[hr]==regs[t].regmap_entry[hr]) nr|=1<<hr;
          }
        }
      }
      // Conditional branch may need registers for following instructions
      if (!dops[i].is_ujump)
      {
        if(i<slen-2) {
          nr |= last_needed_regs[(i+2) & 3];
          for(hr=0;hr<HOST_REGS;hr++)
          {
            if(regmap_pre[i+2][hr]>=0&&get_reg(regs[i+2].regmap_entry,regmap_pre[i+2][hr])<0) nr&=~(1<<hr);
            //if((regmap_entry[i+2][hr])>=0) if(!((nr>>hr)&1)) printf("%x-bogus(%d=%d)\n",start+i*4,hr,regmap_entry[i+2][hr]);
          }
        }
      }
      // Don't need stuff which is overwritten
      //if(regs[i].regmap[hr]!=regmap_pre[i][hr]) nr&=~(1<<hr);
      //if(regs[i].regmap[hr]<0) nr&=~(1<<hr);
      // Merge in delay slot
      if (dops[i+1].rt1) nr &= ~get_regm(regs[i].regmap, dops[i+1].rt1);
      if (dops[i+1].rt2) nr &= ~get_regm(regs[i].regmap, dops[i+1].rt2);
      nr |= get_regm(regmap_pre[i], dops[i+1].rs1);
      nr |= get_regm(regmap_pre[i], dops[i+1].rs2);
      nr |= get_regm(regs[i].regmap_entry, dops[i+1].rs1);
      nr |= get_regm(regs[i].regmap_entry, dops[i+1].rs2);
      if (ram_offset && (dops[i+1].is_load || dops[i+1].is_store)) {
        nr |= get_regm(regmap_pre[i], ROREG);
        nr |= get_regm(regs[i].regmap_entry, ROREG);
      }
      if (dops[i+1].is_store) {
        nr |= get_regm(regmap_pre[i], INVCP);
        nr |= get_regm(regs[i].regmap_entry, INVCP);
      }
    }
    else if (dops[i].is_exception)
    {
      // SYSCALL instruction, etc
      nr=0;
    }
    else // Non-branch
    {
      if(i<slen-1) {
        for(hr=0;hr<HOST_REGS;hr++) {
          if(regmap_pre[i+1][hr]>=0&&get_reg(regs[i+1].regmap_entry,regmap_pre[i+1][hr])<0) nr&=~(1<<hr);
          if(regs[i].regmap[hr]!=regmap_pre[i+1][hr]) nr&=~(1<<hr);
          if(regs[i].regmap[hr]!=regmap_pre[i][hr]) nr&=~(1<<hr);
          if(regs[i].regmap[hr]<0) nr&=~(1<<hr);
        }
      }
    }
    // Overwritten registers are not needed
    if (dops[i].rt1) nr &= ~get_regm(regs[i].regmap, dops[i].rt1);
    if (dops[i].rt2) nr &= ~get_regm(regs[i].regmap, dops[i].rt2);
    nr &= ~get_regm(regs[i].regmap, FTEMP);
    // Source registers are needed
    nr |= get_regm(regmap_pre[i], dops[i].rs1);
    nr |= get_regm(regmap_pre[i], dops[i].rs2);
    nr |= get_regm(regs[i].regmap_entry, dops[i].rs1);
    nr |= get_regm(regs[i].regmap_entry, dops[i].rs2);
    if (ram_offset && (dops[i].is_load || dops[i].is_store)) {
      nr |= get_regm(regmap_pre[i], ROREG);
      nr |= get_regm(regs[i].regmap_entry, ROREG);
    }
    if (dops[i].is_store) {
      nr |= get_regm(regmap_pre[i], INVCP);
      nr |= get_regm(regs[i].regmap_entry, INVCP);
    }

    if (i > 0 && !dops[i].bt && regs[i].wasdirty)
    for(hr=0;hr<HOST_REGS;hr++)
    {
      // Don't store a register immediately after writing it,
      // may prevent dual-issue.
      // But do so if this is a branch target, otherwise we
      // might have to load the register before the branch.
      if((regs[i].wasdirty>>hr)&1) {
        if((regmap_pre[i][hr]>0&&!((unneeded_reg[i]>>regmap_pre[i][hr])&1))) {
          if(dops[i-1].rt1==regmap_pre[i][hr]) nr|=1<<hr;
          if(dops[i-1].rt2==regmap_pre[i][hr]) nr|=1<<hr;
        }
        if((regs[i].regmap_entry[hr]>0&&!((unneeded_reg[i]>>regs[i].regmap_entry[hr])&1))) {
          if(dops[i-1].rt1==regs[i].regmap_entry[hr]) nr|=1<<hr;
          if(dops[i-1].rt2==regs[i].regmap_entry[hr]) nr|=1<<hr;
        }
      }
    }
    // Cycle count is needed at branches.  Assume it is needed at the target too.
    if (i == 0 || dops[i].bt || dops[i].may_except || dops[i].itype == CJUMP) {
      if(regmap_pre[i][HOST_CCREG]==CCREG) nr|=1<<HOST_CCREG;
      if(regs[i].regmap_entry[HOST_CCREG]==CCREG) nr|=1<<HOST_CCREG;
    }
    // Save it
    last_needed_regs[i & 3] = nr;

    // Deallocate unneeded registers
    for(hr=0;hr<HOST_REGS;hr++)
    {
      if(!((nr>>hr)&1)) {
        if(regs[i].regmap_entry[hr]!=CCREG) regs[i].regmap_entry[hr]=-1;
        if(dops[i].is_jump)
        {
          int map1 = 0, map2 = 0, temp = 0; // or -1 ??
          if (dops[i+1].is_load || dops[i+1].is_store)
            map1 = ROREG;
          if (dops[i+1].is_store)
            map2 = INVCP;
          if(dops[i+1].itype==LOADLR || dops[i+1].itype==STORELR || dops[i+1].itype==C2LS)
            temp = FTEMP;
          if(regs[i].regmap[hr]!=dops[i].rs1 && regs[i].regmap[hr]!=dops[i].rs2 &&
             regs[i].regmap[hr]!=dops[i].rt1 && regs[i].regmap[hr]!=dops[i].rt2 &&
             regs[i].regmap[hr]!=dops[i+1].rt1 && regs[i].regmap[hr]!=dops[i+1].rt2 &&
             regs[i].regmap[hr]!=dops[i+1].rs1 && regs[i].regmap[hr]!=dops[i+1].rs2 &&
             regs[i].regmap[hr]!=temp && regs[i].regmap[hr]!=PTEMP &&
             regs[i].regmap[hr]!=RHASH && regs[i].regmap[hr]!=RHTBL &&
             regs[i].regmap[hr]!=RTEMP && regs[i].regmap[hr]!=CCREG &&
             regs[i].regmap[hr]!=map1 && regs[i].regmap[hr]!=map2)
          {
            regs[i].regmap[hr]=-1;
            regs[i].isconst&=~(1<<hr);
            regs[i].dirty&=~(1<<hr);
            regs[i+1].wasdirty&=~(1<<hr);
            if(branch_regs[i].regmap[hr]!=dops[i].rs1 && branch_regs[i].regmap[hr]!=dops[i].rs2 &&
               branch_regs[i].regmap[hr]!=dops[i].rt1 && branch_regs[i].regmap[hr]!=dops[i].rt2 &&
               branch_regs[i].regmap[hr]!=dops[i+1].rt1 && branch_regs[i].regmap[hr]!=dops[i+1].rt2 &&
               branch_regs[i].regmap[hr]!=dops[i+1].rs1 && branch_regs[i].regmap[hr]!=dops[i+1].rs2 &&
               branch_regs[i].regmap[hr]!=temp && branch_regs[i].regmap[hr]!=PTEMP &&
               branch_regs[i].regmap[hr]!=RHASH && branch_regs[i].regmap[hr]!=RHTBL &&
               branch_regs[i].regmap[hr]!=RTEMP && branch_regs[i].regmap[hr]!=CCREG &&
               branch_regs[i].regmap[hr]!=map1 && branch_regs[i].regmap[hr]!=map2)
            {
              branch_regs[i].regmap[hr]=-1;
              branch_regs[i].regmap_entry[hr]=-1;
              if (!dops[i].is_ujump)
              {
                if (i < slen-2) {
                  regmap_pre[i+2][hr]=-1;
                  regs[i+2].wasconst&=~(1<<hr);
                }
              }
            }
          }
        }
        else
        {
          // Non-branch
          if(i>0)
          {
            int map1 = -1, map2 = -1, temp=-1;
            if (dops[i].is_load || dops[i].is_store)
              map1 = ROREG;
            if (dops[i].is_store)
              map2 = INVCP;
            if (dops[i].itype==LOADLR || dops[i].itype==STORELR || dops[i].itype==C2LS)
              temp = FTEMP;
            if(regs[i].regmap[hr]!=dops[i].rt1 && regs[i].regmap[hr]!=dops[i].rt2 &&
               regs[i].regmap[hr]!=dops[i].rs1 && regs[i].regmap[hr]!=dops[i].rs2 &&
               regs[i].regmap[hr]!=temp && regs[i].regmap[hr]!=map1 && regs[i].regmap[hr]!=map2 &&
               //(dops[i].itype!=SPAN||regs[i].regmap[hr]!=CCREG)
               regs[i].regmap[hr] != CCREG)
            {
              if(i<slen-1&&!dops[i].is_ds) {
                assert(regs[i].regmap[hr]<64);
                if(regmap_pre[i+1][hr]!=-1 || regs[i].regmap[hr]>0)
                if(regmap_pre[i+1][hr]!=regs[i].regmap[hr])
                {
                  SysPrintf_lim("fail: %x (%d %d!=%d)\n",
                    start+i*4, hr, regmap_pre[i+1][hr], regs[i].regmap[hr]);
                  assert(regmap_pre[i+1][hr]==regs[i].regmap[hr]);
                }
                regmap_pre[i+1][hr]=-1;
                if(regs[i+1].regmap_entry[hr]==CCREG) regs[i+1].regmap_entry[hr]=-1;
                regs[i+1].wasconst&=~(1<<hr);
              }
              regs[i].regmap[hr]=-1;
              regs[i].isconst&=~(1<<hr);
              regs[i].dirty&=~(1<<hr);
              regs[i+1].wasdirty&=~(1<<hr);
            }
          }
        }
      } // if needed
    } // for hr
  }
}

// If a register is allocated during a loop, try to allocate it for the
// entire loop, if possible.  This avoids loading/storing registers
// inside of the loop.
static noinline void pass5a_preallocate1(void)
{
  int i, j, hr;
  signed char f_regmap[HOST_REGS];
  clear_all_regs(f_regmap);
  for(i=0;i<slen-1;i++)
  {
    if(dops[i].itype==UJUMP||dops[i].itype==CJUMP||dops[i].itype==SJUMP)
    {
      if(cinfo[i].ba>=start && cinfo[i].ba<(start+i*4))
      if(dops[i+1].itype==NOP||dops[i+1].itype==MOV||dops[i+1].itype==ALU
      ||dops[i+1].itype==SHIFTIMM||dops[i+1].itype==IMM16||dops[i+1].itype==LOAD
      ||dops[i+1].itype==STORE||dops[i+1].itype==STORELR
      ||dops[i+1].itype==SHIFT
      ||dops[i+1].itype==COP2||dops[i+1].itype==C2LS||dops[i+1].itype==C2OP)
      {
        int t=(cinfo[i].ba-start)>>2;
        if(t > 0 && !dops[t-1].is_jump) // loop_preload can't handle jumps into delay slots
        if(t<2||(dops[t-2].itype!=UJUMP&&dops[t-2].itype!=RJUMP)||dops[t-2].rt1!=31) // call/ret assumes no registers allocated
        for(hr=0;hr<HOST_REGS;hr++)
        {
          if(regs[i].regmap[hr]>=0) {
            if(f_regmap[hr]!=regs[i].regmap[hr]) {
              // dealloc old register
              int n;
              for(n=0;n<HOST_REGS;n++)
              {
                if(f_regmap[n]==regs[i].regmap[hr]) {f_regmap[n]=-1;}
              }
              // and alloc new one
              f_regmap[hr]=regs[i].regmap[hr];
            }
          }
          if(branch_regs[i].regmap[hr]>=0) {
            if(f_regmap[hr]!=branch_regs[i].regmap[hr]) {
              // dealloc old register
              int n;
              for(n=0;n<HOST_REGS;n++)
              {
                if(f_regmap[n]==branch_regs[i].regmap[hr]) {f_regmap[n]=-1;}
              }
              // and alloc new one
              f_regmap[hr]=branch_regs[i].regmap[hr];
            }
          }
          if(dops[i].ooo) {
            if(count_free_regs(regs[i].regmap)<=cinfo[i+1].min_free_regs)
              f_regmap[hr]=branch_regs[i].regmap[hr];
          }else{
            if(count_free_regs(branch_regs[i].regmap)<=cinfo[i+1].min_free_regs)
              f_regmap[hr]=branch_regs[i].regmap[hr];
          }
          // Avoid dirty->clean transition
          #ifdef DESTRUCTIVE_WRITEBACK
          if(t>0) if(get_reg(regmap_pre[t],f_regmap[hr])>=0) if((regs[t].wasdirty>>get_reg(regmap_pre[t],f_regmap[hr]))&1) f_regmap[hr]=-1;
          #endif
          // This check is only strictly required in the DESTRUCTIVE_WRITEBACK
          // case above, however it's always a good idea.  We can't hoist the
          // load if the register was already allocated, so there's no point
          // wasting time analyzing most of these cases.  It only "succeeds"
          // when the mapping was different and the load can be replaced with
          // a mov, which is of negligible benefit.  So such cases are
          // skipped below.
          if(f_regmap[hr]>0) {
            if(regs[t].regmap[hr]==f_regmap[hr]||(regs[t].regmap_entry[hr]<0&&get_reg(regmap_pre[t],f_regmap[hr])<0)) {
              int r=f_regmap[hr];
              for(j=t;j<=i;j++)
              {
                //printf("Test %x -> %x, %x %d/%d\n",start+i*4,cinfo[i].ba,start+j*4,hr,r);
                if(r<34&&((unneeded_reg[j]>>r)&1)) break;
                assert(r < 64);
                if(regs[j].regmap[hr]==f_regmap[hr]&&f_regmap[hr]<TEMPREG) {
                  //printf("Hit %x -> %x, %x %d/%d\n",start+i*4,cinfo[i].ba,start+j*4,hr,r);
                  int k;
                  if(regs[i].regmap[hr]==-1&&branch_regs[i].regmap[hr]==-1) {
                    if(get_reg(regs[i].regmap,f_regmap[hr])>=0) break;
                    if(get_reg(regs[i+2].regmap,f_regmap[hr])>=0) break;
                    k=i;
                    while(k>1&&regs[k-1].regmap[hr]==-1) {
                      if(count_free_regs(regs[k-1].regmap)<=cinfo[k-1].min_free_regs) {
                        //printf("no free regs for store %x\n",start+(k-1)*4);
                        break;
                      }
                      if(get_reg(regs[k-1].regmap,f_regmap[hr])>=0) {
                        //printf("no-match due to different register\n");
                        break;
                      }
                      if (dops[k-2].is_jump) {
                        //printf("no-match due to branch\n");
                        break;
                      }
                      // call/ret fast path assumes no registers allocated
                      if(k>2&&(dops[k-3].itype==UJUMP||dops[k-3].itype==RJUMP)&&dops[k-3].rt1==31) {
                        break;
                      }
                      k--;
                    }
                    if(regs[k-1].regmap[hr]==f_regmap[hr]&&regmap_pre[k][hr]==f_regmap[hr]) {
                      //printf("Extend r%d, %x ->\n",hr,start+k*4);
                      while(k<i) {
                        regs[k].regmap_entry[hr]=f_regmap[hr];
                        regs[k].regmap[hr]=f_regmap[hr];
                        regmap_pre[k+1][hr]=f_regmap[hr];
                        regs[k].wasdirty&=~(1<<hr);
                        regs[k].dirty&=~(1<<hr);
                        regs[k].wasdirty|=(1<<hr)&regs[k-1].dirty;
                        regs[k].dirty|=(1<<hr)&regs[k].wasdirty;
                        regs[k].wasconst&=~(1<<hr);
                        regs[k].isconst&=~(1<<hr);
                        k++;
                      }
                    }
                    else {
                      //printf("Fail Extend r%d, %x ->\n",hr,start+k*4);
                      break;
                    }
                    assert(regs[i-1].regmap[hr]==f_regmap[hr]);
                    if(regs[i-1].regmap[hr]==f_regmap[hr]&&regmap_pre[i][hr]==f_regmap[hr]) {
                      //printf("OK fill %x (r%d)\n",start+i*4,hr);
                      regs[i].regmap_entry[hr]=f_regmap[hr];
                      regs[i].regmap[hr]=f_regmap[hr];
                      regs[i].wasdirty&=~(1<<hr);
                      regs[i].dirty&=~(1<<hr);
                      regs[i].wasdirty|=(1<<hr)&regs[i-1].dirty;
                      regs[i].dirty|=(1<<hr)&regs[i-1].dirty;
                      regs[i].wasconst&=~(1<<hr);
                      regs[i].isconst&=~(1<<hr);
                      branch_regs[i].regmap_entry[hr]=f_regmap[hr];
                      branch_regs[i].wasdirty&=~(1<<hr);
                      branch_regs[i].wasdirty|=(1<<hr)&regs[i].dirty;
                      branch_regs[i].regmap[hr]=f_regmap[hr];
                      branch_regs[i].dirty&=~(1<<hr);
                      branch_regs[i].dirty|=(1<<hr)&regs[i].dirty;
                      branch_regs[i].wasconst&=~(1<<hr);
                      branch_regs[i].isconst&=~(1<<hr);
                      if (!dops[i].is_ujump) {
                        regmap_pre[i+2][hr]=f_regmap[hr];
                        regs[i+2].wasdirty&=~(1<<hr);
                        regs[i+2].wasdirty|=(1<<hr)&regs[i].dirty;
                      }
                    }
                  }
                  for(k=t;k<j;k++) {
                    // Alloc register clean at beginning of loop,
                    // but may dirty it in pass 6
                    regs[k].regmap_entry[hr]=f_regmap[hr];
                    regs[k].regmap[hr]=f_regmap[hr];
                    regs[k].dirty&=~(1<<hr);
                    regs[k].wasconst&=~(1<<hr);
                    regs[k].isconst&=~(1<<hr);
                    if (dops[k].is_jump) {
                      branch_regs[k].regmap_entry[hr]=f_regmap[hr];
                      branch_regs[k].regmap[hr]=f_regmap[hr];
                      branch_regs[k].dirty&=~(1<<hr);
                      branch_regs[k].wasconst&=~(1<<hr);
                      branch_regs[k].isconst&=~(1<<hr);
                      if (!dops[k].is_ujump) {
                        regmap_pre[k+2][hr]=f_regmap[hr];
                        regs[k+2].wasdirty&=~(1<<hr);
                      }
                    }
                    else
                    {
                      regmap_pre[k+1][hr]=f_regmap[hr];
                      regs[k+1].wasdirty&=~(1<<hr);
                    }
                  }
                  if(regs[j].regmap[hr]==f_regmap[hr])
                    regs[j].regmap_entry[hr]=f_regmap[hr];
                  break;
                }
                if(j==i) break;
                if(regs[j].regmap[hr]>=0)
                  break;
                if(get_reg(regs[j].regmap,f_regmap[hr])>=0) {
                  //printf("no-match due to different register\n");
                  break;
                }
                if (dops[j].is_ujump)
                {
                  // Stop on unconditional branch
                  break;
                }
                if(dops[j].itype==CJUMP||dops[j].itype==SJUMP)
                {
                  if(dops[j].ooo) {
                    if(count_free_regs(regs[j].regmap)<=cinfo[j+1].min_free_regs)
                      break;
                  }else{
                    if(count_free_regs(branch_regs[j].regmap)<=cinfo[j+1].min_free_regs)
                      break;
                  }
                  if(get_reg(branch_regs[j].regmap,f_regmap[hr])>=0) {
                    //printf("no-match due to different register (branch)\n");
                    break;
                  }
                }
                if(count_free_regs(regs[j].regmap)<=cinfo[j].min_free_regs) {
                  //printf("No free regs for store %x\n",start+j*4);
                  break;
                }
                assert(f_regmap[hr]<64);
              }
            }
          }
        }
      }
    }else{
      // Non branch or undetermined branch target
      for(hr=0;hr<HOST_REGS;hr++)
      {
        if(hr!=EXCLUDE_REG) {
          if(regs[i].regmap[hr]>=0) {
            if(f_regmap[hr]!=regs[i].regmap[hr]) {
              // dealloc old register
              int n;
              for(n=0;n<HOST_REGS;n++)
              {
                if(f_regmap[n]==regs[i].regmap[hr]) {f_regmap[n]=-1;}
              }
              // and alloc new one
              f_regmap[hr]=regs[i].regmap[hr];
            }
          }
        }
      }
      // Try to restore cycle count at branch targets
      if(dops[i].bt) {
        for(j=i;j<slen-1;j++) {
          if(regs[j].regmap[HOST_CCREG]!=-1) break;
          if(count_free_regs(regs[j].regmap)<=cinfo[j].min_free_regs) {
            //printf("no free regs for store %x\n",start+j*4);
            break;
          }
        }
        if(regs[j].regmap[HOST_CCREG]==CCREG) {
          int k=i;
          //printf("Extend CC, %x -> %x\n",start+k*4,start+j*4);
          while(k<j) {
            regs[k].regmap_entry[HOST_CCREG]=CCREG;
            regs[k].regmap[HOST_CCREG]=CCREG;
            regmap_pre[k+1][HOST_CCREG]=CCREG;
            regs[k+1].wasdirty|=1<<HOST_CCREG;
            regs[k].dirty|=1<<HOST_CCREG;
            regs[k].wasconst&=~(1<<HOST_CCREG);
            regs[k].isconst&=~(1<<HOST_CCREG);
            k++;
          }
          regs[j].regmap_entry[HOST_CCREG]=CCREG;
        }
        // Work backwards from the branch target
        if(j>i&&f_regmap[HOST_CCREG]==CCREG)
        {
          //printf("Extend backwards\n");
          int k;
          k=i;
          while(regs[k-1].regmap[HOST_CCREG]==-1) {
            if(count_free_regs(regs[k-1].regmap)<=cinfo[k-1].min_free_regs) {
              //printf("no free regs for store %x\n",start+(k-1)*4);
              break;
            }
            k--;
          }
          if(regs[k-1].regmap[HOST_CCREG]==CCREG) {
            //printf("Extend CC, %x ->\n",start+k*4);
            while(k<=i) {
              regs[k].regmap_entry[HOST_CCREG]=CCREG;
              regs[k].regmap[HOST_CCREG]=CCREG;
              regmap_pre[k+1][HOST_CCREG]=CCREG;
              regs[k+1].wasdirty|=1<<HOST_CCREG;
              regs[k].dirty|=1<<HOST_CCREG;
              regs[k].wasconst&=~(1<<HOST_CCREG);
              regs[k].isconst&=~(1<<HOST_CCREG);
              k++;
            }
          }
          else {
            //printf("Fail Extend CC, %x ->\n",start+k*4);
          }
        }
      }
      if(dops[i].itype!=STORE&&dops[i].itype!=STORELR&&dops[i].itype!=SHIFT&&
         dops[i].itype!=NOP&&dops[i].itype!=MOV&&dops[i].itype!=ALU&&dops[i].itype!=SHIFTIMM&&
         dops[i].itype!=IMM16&&dops[i].itype!=LOAD)
      {
        memcpy(f_regmap,regs[i].regmap,sizeof(f_regmap));
      }
    }
  }
}

// This allocates registers (if possible) one instruction prior
// to use, which can avoid a load-use penalty on certain CPUs.
static noinline void pass5b_preallocate2(void)
{
  int i, hr, limit = min(slen - 1, MAXBLOCK - 2);
  for (i = 0; i < limit; i++)
  {
    if (!i || !dops[i-1].is_jump)
    {
      if(!dops[i+1].bt)
      {
        int j, can_steal = 1;
        for (j = i; j < i + 2; j++) {
          int free_regs = 0;
          if (cinfo[j].min_free_regs == 0)
            continue;
          for (hr = 0; hr < HOST_REGS; hr++)
            if (hr != EXCLUDE_REG && regs[j].regmap[hr] < 0)
              free_regs++;
          if (free_regs <= cinfo[j].min_free_regs) {
            can_steal = 0;
            break;
          }
        }
        if (!can_steal)
          continue;
        if(dops[i].itype==ALU||dops[i].itype==MOV||dops[i].itype==LOAD||dops[i].itype==SHIFTIMM||dops[i].itype==IMM16
           ||(dops[i].itype==COP2&&dops[i].opcode2<3))
        {
          if(dops[i+1].rs1) {
            if((hr=get_reg(regs[i+1].regmap,dops[i+1].rs1))>=0)
            {
              if(regs[i].regmap[hr]<0&&regs[i+1].regmap_entry[hr]<0)
              {
                regs[i].regmap[hr]=regs[i+1].regmap[hr];
                regmap_pre[i+1][hr]=regs[i+1].regmap[hr];
                regs[i+1].regmap_entry[hr]=regs[i+1].regmap[hr];
                regs[i].isconst&=~(1<<hr);
                regs[i].isconst|=regs[i+1].isconst&(1<<hr);
                constmap[i][hr]=constmap[i+1][hr];
                regs[i+1].wasdirty&=~(1<<hr);
                regs[i].dirty&=~(1<<hr);
              }
            }
          }
          if(dops[i+1].rs2) {
            if((hr=get_reg(regs[i+1].regmap,dops[i+1].rs2))>=0)
            {
              if(regs[i].regmap[hr]<0&&regs[i+1].regmap_entry[hr]<0)
              {
                regs[i].regmap[hr]=regs[i+1].regmap[hr];
                regmap_pre[i+1][hr]=regs[i+1].regmap[hr];
                regs[i+1].regmap_entry[hr]=regs[i+1].regmap[hr];
                regs[i].isconst&=~(1<<hr);
                regs[i].isconst|=regs[i+1].isconst&(1<<hr);
                constmap[i][hr]=constmap[i+1][hr];
                regs[i+1].wasdirty&=~(1<<hr);
                regs[i].dirty&=~(1<<hr);
              }
            }
          }
          // Preload target address for load instruction (non-constant)
          if(dops[i+1].itype==LOAD&&dops[i+1].rs1&&get_reg(regs[i+1].regmap,dops[i+1].rs1)<0) {
            if((hr=get_reg_w(regs[i+1].regmap, dops[i+1].rt1))>=0)
            {
              if(regs[i].regmap[hr]<0&&regs[i+1].regmap_entry[hr]<0)
              {
                regs[i].regmap[hr]=dops[i+1].rs1;
                regmap_pre[i+1][hr]=dops[i+1].rs1;
                regs[i+1].regmap_entry[hr]=dops[i+1].rs1;
                regs[i].isconst&=~(1<<hr);
                regs[i].isconst|=regs[i+1].isconst&(1<<hr);
                constmap[i][hr]=constmap[i+1][hr];
                regs[i+1].wasdirty&=~(1<<hr);
                regs[i].dirty&=~(1<<hr);
              }
            }
          }
          // Load source into target register
          if(dops[i+1].use_lt1&&get_reg(regs[i+1].regmap,dops[i+1].rs1)<0) {
            if((hr=get_reg_w(regs[i+1].regmap, dops[i+1].rt1))>=0)
            {
              if(regs[i].regmap[hr]<0&&regs[i+1].regmap_entry[hr]<0)
              {
                regs[i].regmap[hr]=dops[i+1].rs1;
                regmap_pre[i+1][hr]=dops[i+1].rs1;
                regs[i+1].regmap_entry[hr]=dops[i+1].rs1;
                regs[i].isconst&=~(1<<hr);
                regs[i].isconst|=regs[i+1].isconst&(1<<hr);
                constmap[i][hr]=constmap[i+1][hr];
                regs[i+1].wasdirty&=~(1<<hr);
                regs[i].dirty&=~(1<<hr);
              }
            }
          }
          // Address for store instruction (non-constant)
          if (dops[i+1].is_store) { // SB/SH/SW/SWC2
            if(get_reg(regs[i+1].regmap,dops[i+1].rs1)<0) {
              hr=get_reg2(regs[i].regmap,regs[i+1].regmap,-1);
              if(hr<0) hr=get_reg_temp(regs[i+1].regmap);
              else {
                regs[i+1].regmap[hr]=AGEN1+((i+1)&1);
                regs[i+1].isconst&=~(1<<hr);
                regs[i+1].dirty&=~(1<<hr);
                regs[i+2].wasdirty&=~(1<<hr);
              }
              assert(hr>=0);
              #if 0 // what is this for? double allocs $0 in ps1_rom.bin
              if(regs[i].regmap[hr]<0&&regs[i+1].regmap_entry[hr]<0)
              {
                regs[i].regmap[hr]=dops[i+1].rs1;
                regmap_pre[i+1][hr]=dops[i+1].rs1;
                regs[i+1].regmap_entry[hr]=dops[i+1].rs1;
                regs[i].isconst&=~(1<<hr);
                regs[i].isconst|=regs[i+1].isconst&(1<<hr);
                constmap[i][hr]=constmap[i+1][hr];
                regs[i+1].wasdirty&=~(1<<hr);
                regs[i].dirty&=~(1<<hr);
              }
              #endif
            }
          }
          if (dops[i+1].itype == LOADLR || dops[i+1].opcode == 0x32) { // LWC2
            if(get_reg(regs[i+1].regmap,dops[i+1].rs1)<0) {
              int nr;
              hr=get_reg(regs[i+1].regmap,FTEMP);
              assert(hr>=0);
              if(regs[i].regmap[hr]<0&&regs[i+1].regmap_entry[hr]<0)
              {
                regs[i].regmap[hr]=dops[i+1].rs1;
                regmap_pre[i+1][hr]=dops[i+1].rs1;
                regs[i+1].regmap_entry[hr]=dops[i+1].rs1;
                regs[i].isconst&=~(1<<hr);
                regs[i].isconst|=regs[i+1].isconst&(1<<hr);
                constmap[i][hr]=constmap[i+1][hr];
                regs[i+1].wasdirty&=~(1<<hr);
                regs[i].dirty&=~(1<<hr);
              }
              else if((nr=get_reg2(regs[i].regmap,regs[i+1].regmap,-1))>=0)
              {
                // move it to another register
                regs[i+1].regmap[hr]=-1;
                regmap_pre[i+2][hr]=-1;
                regs[i+1].regmap[nr]=FTEMP;
                regmap_pre[i+2][nr]=FTEMP;
                regs[i].regmap[nr]=dops[i+1].rs1;
                regmap_pre[i+1][nr]=dops[i+1].rs1;
                regs[i+1].regmap_entry[nr]=dops[i+1].rs1;
                regs[i].isconst&=~(1<<nr);
                regs[i+1].isconst&=~(1<<nr);
                regs[i].dirty&=~(1<<nr);
                regs[i+1].wasdirty&=~(1<<nr);
                regs[i+1].dirty&=~(1<<nr);
                regs[i+2].wasdirty&=~(1<<nr);
              }
            }
          }
          if(dops[i+1].itype==LOAD||dops[i+1].itype==LOADLR||dops[i+1].itype==STORE||dops[i+1].itype==STORELR/*||dops[i+1].itype==C2LS*/) {
            hr = -1;
            if(dops[i+1].itype==LOAD)
              hr=get_reg_w(regs[i+1].regmap, dops[i+1].rt1);
            if (dops[i+1].itype == LOADLR || dops[i+1].opcode == 0x32) // LWC2
              hr=get_reg(regs[i+1].regmap,FTEMP);
            if (dops[i+1].is_store) {
              hr=get_reg(regs[i+1].regmap,AGEN1+((i+1)&1));
              if(hr<0) hr=get_reg_temp(regs[i+1].regmap);
            }
            if(hr>=0&&regs[i].regmap[hr]<0) {
              int rs=get_reg(regs[i+1].regmap,dops[i+1].rs1);
              if(rs>=0&&((regs[i+1].wasconst>>rs)&1)) {
                regs[i].regmap[hr]=AGEN1+((i+1)&1);
                regmap_pre[i+1][hr]=AGEN1+((i+1)&1);
                regs[i+1].regmap_entry[hr]=AGEN1+((i+1)&1);
                regs[i].isconst&=~(1<<hr);
                regs[i+1].wasdirty&=~(1<<hr);
                regs[i].dirty&=~(1<<hr);
              }
            }
          }
        }
      }
    }
  }
}

// Write back dirty registers as soon as we will no longer modify them,
// so that we don't end up with lots of writes at the branches.
static noinline void pass6_clean_registers(int istart, int iend, int wr)
{
  static u_int wont_dirty[MAXBLOCK];
  static u_int will_dirty[MAXBLOCK];
  int i;
  int r;
  u_int will_dirty_i,will_dirty_next,temp_will_dirty;
  u_int wont_dirty_i,wont_dirty_next,temp_wont_dirty;
  if(iend==slen-1) {
    will_dirty_i=will_dirty_next=0;
    wont_dirty_i=wont_dirty_next=0;
  }else{
    will_dirty_i=will_dirty_next=will_dirty[iend+1];
    wont_dirty_i=wont_dirty_next=wont_dirty[iend+1];
  }
  for (i=iend;i>=istart;i--)
  {
    signed char rregmap_i[RRMAP_SIZE];
    u_int hr_candirty = 0;
    assert(HOST_REGS < 32);
    make_rregs(regs[i].regmap, rregmap_i, &hr_candirty);
    __builtin_prefetch(regs[i-1].regmap);
    if(dops[i].is_jump)
    {
      signed char branch_rregmap_i[RRMAP_SIZE];
      u_int branch_hr_candirty = 0;
      make_rregs(branch_regs[i].regmap, branch_rregmap_i, &branch_hr_candirty);
      if(cinfo[i].ba<start || cinfo[i].ba>=(start+slen*4))
      {
        // Branch out of this block, flush all regs
        will_dirty_i = 0;
        will_dirty_i |= 1u << (get_rreg(branch_rregmap_i, dops[i].rt1) & 31);
        will_dirty_i |= 1u << (get_rreg(branch_rregmap_i, dops[i].rt2) & 31);
        will_dirty_i |= 1u << (get_rreg(branch_rregmap_i, dops[i+1].rt1) & 31);
        will_dirty_i |= 1u << (get_rreg(branch_rregmap_i, dops[i+1].rt2) & 31);
        will_dirty_i |= 1u << (get_rreg(branch_rregmap_i, CCREG) & 31);
        will_dirty_i &= branch_hr_candirty;
        if (dops[i].is_ujump)
        {
          // Unconditional branch
          wont_dirty_i = 0;
          // Merge in delay slot (will dirty)
          will_dirty_i |= 1u << (get_rreg(rregmap_i, dops[i].rt1) & 31);
          will_dirty_i |= 1u << (get_rreg(rregmap_i, dops[i].rt2) & 31);
          will_dirty_i |= 1u << (get_rreg(rregmap_i, dops[i+1].rt1) & 31);
          will_dirty_i |= 1u << (get_rreg(rregmap_i, dops[i+1].rt2) & 31);
          will_dirty_i |= 1u << (get_rreg(rregmap_i, CCREG) & 31);
          will_dirty_i &= hr_candirty;
        }
        else
        {
          // Conditional branch
          wont_dirty_i = wont_dirty_next;
          // Merge in delay slot (will dirty)
          // (the original code had no explanation why these 2 are commented out)
          //will_dirty_i |= 1u << (get_rreg(rregmap_i, dops[i].rt1) & 31);
          //will_dirty_i |= 1u << (get_rreg(rregmap_i, dops[i].rt2) & 31);
          will_dirty_i |= 1u << (get_rreg(rregmap_i, dops[i+1].rt1) & 31);
          will_dirty_i |= 1u << (get_rreg(rregmap_i, dops[i+1].rt2) & 31);
          will_dirty_i |= 1u << (get_rreg(rregmap_i, CCREG) & 31);
          will_dirty_i &= hr_candirty;
        }
        // Merge in delay slot (wont dirty)
        wont_dirty_i |= 1u << (get_rreg(rregmap_i, dops[i].rt1) & 31);
        wont_dirty_i |= 1u << (get_rreg(rregmap_i, dops[i].rt2) & 31);
        wont_dirty_i |= 1u << (get_rreg(rregmap_i, dops[i+1].rt1) & 31);
        wont_dirty_i |= 1u << (get_rreg(rregmap_i, dops[i+1].rt2) & 31);
        wont_dirty_i |= 1u << (get_rreg(rregmap_i, CCREG) & 31);
        wont_dirty_i |= 1u << (get_rreg(branch_rregmap_i, dops[i].rt1) & 31);
        wont_dirty_i |= 1u << (get_rreg(branch_rregmap_i, dops[i].rt2) & 31);
        wont_dirty_i |= 1u << (get_rreg(branch_rregmap_i, dops[i+1].rt1) & 31);
        wont_dirty_i |= 1u << (get_rreg(branch_rregmap_i, dops[i+1].rt2) & 31);
        wont_dirty_i |= 1u << (get_rreg(branch_rregmap_i, CCREG) & 31);
        wont_dirty_i &= ~(1u << 31);
        if(wr) {
          #ifndef DESTRUCTIVE_WRITEBACK
          branch_regs[i].dirty&=wont_dirty_i;
          #endif
          branch_regs[i].dirty|=will_dirty_i;
        }
      }
      else
      {
        // Internal branch
        if(cinfo[i].ba<=start+i*4) {
          // Backward branch
          if (dops[i].is_ujump)
          {
            // Unconditional branch
            temp_will_dirty=0;
            temp_wont_dirty=0;
            // Merge in delay slot (will dirty)
            temp_will_dirty |= 1u << (get_rreg(branch_rregmap_i, dops[i].rt1) & 31);
            temp_will_dirty |= 1u << (get_rreg(branch_rregmap_i, dops[i].rt2) & 31);
            temp_will_dirty |= 1u << (get_rreg(branch_rregmap_i, dops[i+1].rt1) & 31);
            temp_will_dirty |= 1u << (get_rreg(branch_rregmap_i, dops[i+1].rt2) & 31);
            temp_will_dirty |= 1u << (get_rreg(branch_rregmap_i, CCREG) & 31);
            temp_will_dirty &= branch_hr_candirty;
            temp_will_dirty |= 1u << (get_rreg(rregmap_i, dops[i].rt1) & 31);
            temp_will_dirty |= 1u << (get_rreg(rregmap_i, dops[i].rt2) & 31);
            temp_will_dirty |= 1u << (get_rreg(rregmap_i, dops[i+1].rt1) & 31);
            temp_will_dirty |= 1u << (get_rreg(rregmap_i, dops[i+1].rt2) & 31);
            temp_will_dirty |= 1u << (get_rreg(rregmap_i, CCREG) & 31);
            temp_will_dirty &= hr_candirty;
          } else {
            // Conditional branch (not taken case)
            temp_will_dirty=will_dirty_next;
            temp_wont_dirty=wont_dirty_next;
            // Merge in delay slot (will dirty)
            temp_will_dirty |= 1u << (get_rreg(branch_rregmap_i, dops[i].rt1) & 31);
            temp_will_dirty |= 1u << (get_rreg(branch_rregmap_i, dops[i].rt2) & 31);
            temp_will_dirty |= 1u << (get_rreg(branch_rregmap_i, dops[i+1].rt1) & 31);
            temp_will_dirty |= 1u << (get_rreg(branch_rregmap_i, dops[i+1].rt2) & 31);
            temp_will_dirty |= 1u << (get_rreg(branch_rregmap_i, CCREG) & 31);
            temp_will_dirty &= branch_hr_candirty;
            //temp_will_dirty |= 1u << (get_rreg(rregmap_i, dops[i].rt1) & 31);
            //temp_will_dirty |= 1u << (get_rreg(rregmap_i, dops[i].rt2) & 31);
            temp_will_dirty |= 1u << (get_rreg(rregmap_i, dops[i+1].rt1) & 31);
            temp_will_dirty |= 1u << (get_rreg(rregmap_i, dops[i+1].rt2) & 31);
            temp_will_dirty |= 1u << (get_rreg(rregmap_i, CCREG) & 31);
            temp_will_dirty &= hr_candirty;
          }
          // Merge in delay slot (wont dirty)
          temp_wont_dirty |= 1u << (get_rreg(rregmap_i, dops[i].rt1) & 31);
          temp_wont_dirty |= 1u << (get_rreg(rregmap_i, dops[i].rt2) & 31);
          temp_wont_dirty |= 1u << (get_rreg(rregmap_i, dops[i+1].rt1) & 31);
          temp_wont_dirty |= 1u << (get_rreg(rregmap_i, dops[i+1].rt2) & 31);
          temp_wont_dirty |= 1u << (get_rreg(rregmap_i, CCREG) & 31);
          temp_wont_dirty |= 1u << (get_rreg(branch_rregmap_i, dops[i].rt1) & 31);
          temp_wont_dirty |= 1u << (get_rreg(branch_rregmap_i, dops[i].rt2) & 31);
          temp_wont_dirty |= 1u << (get_rreg(branch_rregmap_i, dops[i+1].rt1) & 31);
          temp_wont_dirty |= 1u << (get_rreg(branch_rregmap_i, dops[i+1].rt2) & 31);
          temp_wont_dirty |= 1u << (get_rreg(branch_rregmap_i, CCREG) & 31);
          temp_wont_dirty &= ~(1u << 31);
          // Deal with changed mappings
          if(i<iend) {
            for(r=0;r<HOST_REGS;r++) {
              if(r!=EXCLUDE_REG) {
                if(regs[i].regmap[r]!=regmap_pre[i][r]) {
                  temp_will_dirty&=~(1<<r);
                  temp_wont_dirty&=~(1<<r);
                  if(regmap_pre[i][r]>0 && regmap_pre[i][r]<34) {
                    temp_will_dirty|=((unneeded_reg[i]>>regmap_pre[i][r])&1)<<r;
                    temp_wont_dirty|=((unneeded_reg[i]>>regmap_pre[i][r])&1)<<r;
                  } else {
                    temp_will_dirty|=1<<r;
                    temp_wont_dirty|=1<<r;
                  }
                }
              }
            }
          }
          if(wr) {
            will_dirty[i]=temp_will_dirty;
            wont_dirty[i]=temp_wont_dirty;
            pass6_clean_registers((cinfo[i].ba-start)>>2,i-1,0);
          }else{
            // Limit recursion.  It can take an excessive amount
            // of time if there are a lot of nested loops.
            will_dirty[(cinfo[i].ba-start)>>2]=0;
            wont_dirty[(cinfo[i].ba-start)>>2]=-1;
          }
        }
        /*else*/ if(1)
        {
          if (dops[i].is_ujump)
          {
            // Unconditional branch
            will_dirty_i=0;
            wont_dirty_i=0;
          //if(cinfo[i].ba>start+i*4) { // Disable recursion (for debugging)
            for(r=0;r<HOST_REGS;r++) {
              if(r!=EXCLUDE_REG) {
                if(branch_regs[i].regmap[r]==regs[(cinfo[i].ba-start)>>2].regmap_entry[r]) {
                  will_dirty_i|=will_dirty[(cinfo[i].ba-start)>>2]&(1<<r);
                  wont_dirty_i|=wont_dirty[(cinfo[i].ba-start)>>2]&(1<<r);
                }
                if(branch_regs[i].regmap[r]>=0) {
                  will_dirty_i|=((unneeded_reg[(cinfo[i].ba-start)>>2]>>branch_regs[i].regmap[r])&1)<<r;
                  wont_dirty_i|=((unneeded_reg[(cinfo[i].ba-start)>>2]>>branch_regs[i].regmap[r])&1)<<r;
                }
              }
            }
          //}
            // Merge in delay slot
            will_dirty_i |= 1u << (get_rreg(branch_rregmap_i, dops[i].rt1) & 31);
            will_dirty_i |= 1u << (get_rreg(branch_rregmap_i, dops[i].rt2) & 31);
            will_dirty_i |= 1u << (get_rreg(branch_rregmap_i, dops[i+1].rt1) & 31);
            will_dirty_i |= 1u << (get_rreg(branch_rregmap_i, dops[i+1].rt2) & 31);
            will_dirty_i |= 1u << (get_rreg(branch_rregmap_i, CCREG) & 31);
            will_dirty_i &= branch_hr_candirty;
            will_dirty_i |= 1u << (get_rreg(rregmap_i, dops[i].rt1) & 31);
            will_dirty_i |= 1u << (get_rreg(rregmap_i, dops[i].rt2) & 31);
            will_dirty_i |= 1u << (get_rreg(rregmap_i, dops[i+1].rt1) & 31);
            will_dirty_i |= 1u << (get_rreg(rregmap_i, dops[i+1].rt2) & 31);
            will_dirty_i |= 1u << (get_rreg(rregmap_i, CCREG) & 31);
            will_dirty_i &= hr_candirty;
          } else {
            // Conditional branch
            will_dirty_i=will_dirty_next;
            wont_dirty_i=wont_dirty_next;
          //if(cinfo[i].ba>start+i*4) // Disable recursion (for debugging)
            for(r=0;r<HOST_REGS;r++) {
              if(r!=EXCLUDE_REG) {
                signed char target_reg=branch_regs[i].regmap[r];
                if(target_reg==regs[(cinfo[i].ba-start)>>2].regmap_entry[r]) {
                  will_dirty_i&=will_dirty[(cinfo[i].ba-start)>>2]&(1<<r);
                  wont_dirty_i|=wont_dirty[(cinfo[i].ba-start)>>2]&(1<<r);
                }
                else if(target_reg>=0) {
                  will_dirty_i&=((unneeded_reg[(cinfo[i].ba-start)>>2]>>target_reg)&1)<<r;
                  wont_dirty_i|=((unneeded_reg[(cinfo[i].ba-start)>>2]>>target_reg)&1)<<r;
                }
              }
            }
            // Merge in delay slot
            will_dirty_i |= 1u << (get_rreg(branch_rregmap_i, dops[i].rt1) & 31);
            will_dirty_i |= 1u << (get_rreg(branch_rregmap_i, dops[i].rt2) & 31);
            will_dirty_i |= 1u << (get_rreg(branch_rregmap_i, dops[i+1].rt1) & 31);
            will_dirty_i |= 1u << (get_rreg(branch_rregmap_i, dops[i+1].rt2) & 31);
            will_dirty_i |= 1u << (get_rreg(branch_rregmap_i, CCREG) & 31);
            will_dirty_i &= branch_hr_candirty;
            //will_dirty_i |= 1u << (get_rreg(rregmap_i, dops[i].rt1) & 31);
            //will_dirty_i |= 1u << (get_rreg(rregmap_i, dops[i].rt2) & 31);
            will_dirty_i |= 1u << (get_rreg(rregmap_i, dops[i+1].rt1) & 31);
            will_dirty_i |= 1u << (get_rreg(rregmap_i, dops[i+1].rt2) & 31);
            will_dirty_i |= 1u << (get_rreg(rregmap_i, CCREG) & 31);
            will_dirty_i &= hr_candirty;
          }
          // Merge in delay slot (won't dirty)
          wont_dirty_i |= 1u << (get_rreg(rregmap_i, dops[i].rt1) & 31);
          wont_dirty_i |= 1u << (get_rreg(rregmap_i, dops[i].rt2) & 31);
          wont_dirty_i |= 1u << (get_rreg(rregmap_i, dops[i+1].rt1) & 31);
          wont_dirty_i |= 1u << (get_rreg(rregmap_i, dops[i+1].rt2) & 31);
          wont_dirty_i |= 1u << (get_rreg(rregmap_i, CCREG) & 31);
          wont_dirty_i |= 1u << (get_rreg(branch_rregmap_i, dops[i].rt1) & 31);
          wont_dirty_i |= 1u << (get_rreg(branch_rregmap_i, dops[i].rt2) & 31);
          wont_dirty_i |= 1u << (get_rreg(branch_rregmap_i, dops[i+1].rt1) & 31);
          wont_dirty_i |= 1u << (get_rreg(branch_rregmap_i, dops[i+1].rt2) & 31);
          wont_dirty_i |= 1u << (get_rreg(branch_rregmap_i, CCREG) & 31);
          wont_dirty_i &= ~(1u << 31);
          if(wr) {
            #ifndef DESTRUCTIVE_WRITEBACK
            branch_regs[i].dirty&=wont_dirty_i;
            #endif
            branch_regs[i].dirty|=will_dirty_i;
          }
        }
      }
    }
    else if (dops[i].is_exception)
    {
      // SYSCALL instruction, etc
      will_dirty_i=0;
      wont_dirty_i=0;
    }
    will_dirty_next=will_dirty_i;
    wont_dirty_next=wont_dirty_i;
    will_dirty_i |= 1u << (get_rreg(rregmap_i, dops[i].rt1) & 31);
    will_dirty_i |= 1u << (get_rreg(rregmap_i, dops[i].rt2) & 31);
    will_dirty_i |= 1u << (get_rreg(rregmap_i, CCREG) & 31);
    will_dirty_i &= hr_candirty;
    wont_dirty_i |= 1u << (get_rreg(rregmap_i, dops[i].rt1) & 31);
    wont_dirty_i |= 1u << (get_rreg(rregmap_i, dops[i].rt2) & 31);
    wont_dirty_i |= 1u << (get_rreg(rregmap_i, CCREG) & 31);
    wont_dirty_i &= ~(1u << 31);
    if (i > istart && !dops[i].is_jump) {
      // Don't store a register immediately after writing it,
      // may prevent dual-issue.
      wont_dirty_i |= 1u << (get_rreg(rregmap_i, dops[i-1].rt1) & 31);
      wont_dirty_i |= 1u << (get_rreg(rregmap_i, dops[i-1].rt2) & 31);
    }
    // Save it
    will_dirty[i]=will_dirty_i;
    wont_dirty[i]=wont_dirty_i;
    // Mark registers that won't be dirtied as not dirty
    if(wr) {
        regs[i].dirty|=will_dirty_i;
        #ifndef DESTRUCTIVE_WRITEBACK
        regs[i].dirty&=wont_dirty_i;
        if(dops[i].is_jump)
        {
          if (i < iend-1 && !dops[i].is_ujump) {
            for(r=0;r<HOST_REGS;r++) {
              if(r!=EXCLUDE_REG) {
                if(regs[i].regmap[r]==regmap_pre[i+2][r]) {
                  regs[i+2].wasdirty&=wont_dirty_i|~(1<<r);
                }else {/*printf("i: %x (%d) mismatch(+2): %d\n",start+i*4,i,r);assert(!((wont_dirty_i>>r)&1));*/}
              }
            }
          }
        }
        else
        {
          if(i<iend) {
            for(r=0;r<HOST_REGS;r++) {
              if(r!=EXCLUDE_REG) {
                if(regs[i].regmap[r]==regmap_pre[i+1][r]) {
                  regs[i+1].wasdirty&=wont_dirty_i|~(1<<r);
                }else {/*printf("i: %x (%d) mismatch(+1): %d\n",start+i*4,i,r);assert(!((wont_dirty_i>>r)&1));*/}
              }
            }
          }
        }
        #endif
    }
    // Deal with changed mappings
    temp_will_dirty=will_dirty_i;
    temp_wont_dirty=wont_dirty_i;
    for(r=0;r<HOST_REGS;r++) {
      if(r!=EXCLUDE_REG) {
        int nr;
        if(regs[i].regmap[r]==regmap_pre[i][r]) {
          if(wr) {
            #ifndef DESTRUCTIVE_WRITEBACK
            regs[i].wasdirty&=wont_dirty_i|~(1<<r);
            #endif
            regs[i].wasdirty|=will_dirty_i&(1<<r);
          }
        }
        else if(regmap_pre[i][r]>=0&&(nr=get_rreg(rregmap_i,regmap_pre[i][r]))>=0) {
          // Register moved to a different register
          will_dirty_i&=~(1<<r);
          wont_dirty_i&=~(1<<r);
          will_dirty_i|=((temp_will_dirty>>nr)&1)<<r;
          wont_dirty_i|=((temp_wont_dirty>>nr)&1)<<r;
          if(wr) {
            #ifndef DESTRUCTIVE_WRITEBACK
            regs[i].wasdirty&=wont_dirty_i|~(1<<r);
            #endif
            regs[i].wasdirty|=will_dirty_i&(1<<r);
          }
        }
        else {
          will_dirty_i&=~(1<<r);
          wont_dirty_i&=~(1<<r);
          if(regmap_pre[i][r]>0 && regmap_pre[i][r]<34) {
            will_dirty_i|=((unneeded_reg[i]>>regmap_pre[i][r])&1)<<r;
            wont_dirty_i|=((unneeded_reg[i]>>regmap_pre[i][r])&1)<<r;
          } else {
            wont_dirty_i|=1<<r;
            /*printf("i: %x (%d) mismatch: %d\n",start+i*4,i,r);assert(!((will_dirty>>r)&1));*/
          }
        }
      }
    }
  }
}

static noinline void pass10_expire_blocks(void)
{
  u_int step = MAX_OUTPUT_BLOCK_SIZE / PAGE_COUNT / 2;
  // not sizeof(ndrc->translation_cache) due to vita hack
  u_int step_mask = ((1u << TARGET_SIZE_2) - 1u) & ~(step - 1u);
  u_int end = (out - ndrc->translation_cache + EXPIRITY_OFFSET) & step_mask;
  u_int base_shift = __builtin_ctz(MAX_OUTPUT_BLOCK_SIZE);
  int hit;

  for (; expirep != end; expirep = ((expirep + step) & step_mask))
  {
    u_int base_offs = expirep & ~(MAX_OUTPUT_BLOCK_SIZE - 1);
    u_int block_i = expirep / step & (PAGE_COUNT - 1);
    u_int phase = (expirep >> (base_shift - 1)) & 1u;
    if (!(expirep & (MAX_OUTPUT_BLOCK_SIZE / 2 - 1))) {
      inv_debug("EXP: base_offs %x/%lx phase %u\n", base_offs,
        (long)(out - ndrc->translation_cache), phase);
    }

    if (!phase) {
      hit = blocks_remove_matching_addrs(&blocks[block_i], base_offs, base_shift);
      if (hit) {
        do_clear_cache();
        mini_ht_clear();
      }
    }
    else
      unlink_jumps_tc_range(jumps[block_i], base_offs, base_shift);
  }
}

static struct block_info *new_block_info(u_int start, u_int len,
  const void *source, const void *copy, u_char *beginning, u_short jump_in_count)
{
  struct block_info **b_pptr;
  struct block_info *block;
  u_int page = get_page(start);

  block = malloc(sizeof(*block) + jump_in_count * sizeof(block->jump_in[0]));
  assert(block);
  assert(jump_in_count > 0);
  block->source = source;
  block->copy = copy;
  block->start = start;
  block->len = len;
  block->reg_sv_flags = 0;
  block->tc_offs = beginning - ndrc->translation_cache;
  //block->tc_len = out - beginning;
  block->is_dirty = 0;
  block->inv_near_misses = 0;
  block->jump_in_cnt = jump_in_count;

  // insert sorted by start mirror-unmasked vaddr
  for (b_pptr = &blocks[page]; ; b_pptr = &((*b_pptr)->next)) {
    if (*b_pptr == NULL || (*b_pptr)->start >= start) {
      block->next = *b_pptr;
      *b_pptr = block;
      break;
    }
  }
  stat_inc(stat_blocks);
  return block;
}

static int noinline new_recompile_block(u_int addr)
{
  u_int pagelimit = 0;
  u_int state_rflags = 0;
  int i;

  assem_debug("NOTCOMPILED: addr = %x -> %p\n", addr, log_addr(out));

  if (addr & 3) {
    if (addr != hack_addr) {
      SysPrintf_lim("game crash @%08x, ra=%08x\n", addr, psxRegs.GPR.n.ra);
      hack_addr = addr;
    }
    return -1;
  }

  // this is just for speculation
  for (i = 1; i < 32; i++) {
    if ((psxRegs.GPR.r[i] & 0xffff0000) == 0x1f800000)
      state_rflags |= 1 << i;
  }

  start = addr;
  ndrc_g.did_compile++;
  if (Config.HLE && start == 0x80001000) // hlecall
  {
    void *beginning = start_block();

    emit_movimm(start,0);
    emit_writeword(0,&psxRegs.pc);
    emit_far_jump(new_dyna_leave);
    literal_pool(0);
    end_block(beginning);
    struct block_info *block = new_block_info(start, 4, NULL, NULL, beginning, 1);
    block->jump_in[0].vaddr = start;
    block->jump_in[0].addr = beginning;
    return 0;
  }
  else if (f1_hack && hack_addr == 0) {
    void *beginning = start_block();
    emit_movimm(start, 0);
    emit_writeword(0, &hack_addr);
    emit_readword(&psxRegs.GPR.n.sp, 0);
    emit_readptr(&mem_rtab, 1);
    emit_shrimm(0, 12, 2);
    emit_readptr_dualindexedx_ptrlen(1, 2, 1);
    emit_addimm(0, 0x18, 0);
    emit_adds_ptr(1, 1, 1);
    emit_ldr_dualindexed(1, 0, 0);
    emit_readptr(&hash_table_ptr, 1);
    emit_writeword(0, &psxRegs.GPR.r[26]); // lw k0, 0x18(sp)
    emit_far_call(ndrc_get_addr_ht);
    emit_jmpreg(0); // jr k0
    literal_pool(0);
    end_block(beginning);

    struct block_info *block = new_block_info(start, 4, NULL, NULL, beginning, 1);
    block->jump_in[0].vaddr = start;
    block->jump_in[0].addr = beginning;
    SysPrintf("F1 hack to   %08x\n", start);
    return 0;
  }

  cycle_multiplier_active = get_cycle_multiplier();

  source = get_source_start(start, &pagelimit);
  if (source == NULL) {
    if (addr != hack_addr) {
      SysPrintf_lim("Compile at bogus memory address: %08x, ra=%x\n",
        addr, psxRegs.GPR.n.ra);
      hack_addr = addr;
    }
    //abort();
    return -1;
  }

  /* Pass 1: disassemble */
  /* Pass 2: register dependencies, branch targets */
  /* Pass 3: register allocation */
  /* Pass 4: branch dependencies */
  /* Pass 5: pre-alloc */
  /* Pass 6: optimize clean/dirty state */
  /* Pass 7: flag 32-bit registers */
  /* Pass 8: assembly */
  /* Pass 9: linker */
  /* Pass 10: garbage collection / free memory */

  /* Pass 1 disassembly */

  pass1a_disassemble(pagelimit);
  pass1b_bt();

  int clear_hack_addr = apply_hacks();

  /* Pass 2 - unneeded, register dependencies */

  pass2a_unneeded();
  pass2b_unneeded_regs(0, slen-1, 0);

  /* Pass 3 - Register allocation */

  pass3_register_alloc(addr);

  /* Pass 4 - Cull unused host registers */

  pass4_cull_unused_regs();

  /* Pass 5 - Pre-allocate registers */

  pass5a_preallocate1();
  pass5b_preallocate2();

  /* Pass 6 - Optimize clean/dirty state */
  pass6_clean_registers(0, slen-1, 1);

  /* Pass 7 */
  for (i=slen-1;i>=0;i--)
  {
    if(dops[i].itype==CJUMP||dops[i].itype==SJUMP)
    {
      // Conditional branch
      if((source[i]>>16)!=0x1000&&i<slen-2) {
        // Mark this address as a branch target since it may be called
        // upon return from interrupt
        dops[i+2].bt=1;
      }
    }
  }

  /* Pass 8 - Assembly */
  linkcount=0;stubcount=0;
  is_delayslot=0;
  u_int dirty_pre=0;
  void *beginning=start_block();
  void *instr_addr0_override = NULL;
  int ds = 0;

  if ((Config.HLE && start == 0x80000080) || start == 0x80030000) {
    instr_addr0_override = out;
    emit_movimm(start, 0);
    if (start == 0x80030000) {
      // for BiosBootBypass() to work
      // io address var abused as a "already been here" flag
      emit_readword(&address, 1);
      emit_writeword(0, &psxRegs.pc);
      emit_writeword(0, &address);
      emit_cmp(0, 1);
    }
    else {
      emit_readword(&psxRegs.cpuInRecursion, 1);
      emit_writeword(0, &psxRegs.pc);
      emit_test(1, 1);
    }
    #ifdef __aarch64__
    emit_jeq(out + 4*2);
    emit_far_jump(new_dyna_leave);
    #else
    emit_jne(new_dyna_leave);
    #endif
  }
  for(i=0;i<slen;i++)
  {
    __builtin_prefetch(regs[i+1].regmap);
    check_regmap(regmap_pre[i]);
    check_regmap(regs[i].regmap_entry);
    check_regmap(regs[i].regmap);
    //if(ds) printf("ds: ");
    disassemble_inst(i);
    if(ds) {
      ds=0; // Skip delay slot
      if(dops[i].bt) assem_debug("OOPS - branch into delay slot\n");
      instr_addr[i] = NULL;
    } else {
      speculate_register_values(i);
      #ifndef DESTRUCTIVE_WRITEBACK
      if (i < 2 || !dops[i-2].is_ujump)
      {
        wb_valid(regmap_pre[i],regs[i].regmap_entry,dirty_pre,regs[i].wasdirty,unneeded_reg[i]);
      }
      if((dops[i].itype==CJUMP||dops[i].itype==SJUMP)) {
        dirty_pre=branch_regs[i].dirty;
      }else{
        dirty_pre=regs[i].dirty;
      }
      #endif
      // write back
      if (i < 2 || !dops[i-2].is_ujump)
      {
        wb_invalidate(regmap_pre[i],regs[i].regmap_entry,regs[i].wasdirty,unneeded_reg[i]);
        loop_preload(regmap_pre[i],regs[i].regmap_entry);
      }
      // branch target entry point
      instr_addr[i] = out;
      assem_debug("<->\n");
      drc_dbg_emit_do_cmp(i, cinfo[i].ccadj);
      if (clear_hack_addr) {
        emit_movimm(0, 0);
        emit_writeword(0, &hack_addr);
        clear_hack_addr = 0;
      }

      // load regs
      if(regs[i].regmap_entry[HOST_CCREG]==CCREG&&regs[i].regmap[HOST_CCREG]!=CCREG)
        wb_register(CCREG,regs[i].regmap_entry,regs[i].wasdirty);
      load_regs(regs[i].regmap_entry,regs[i].regmap,dops[i].rs1,dops[i].rs2);
      address_generation(i,&regs[i],regs[i].regmap_entry);
      load_consts(regmap_pre[i],regs[i].regmap,i);
      if(dops[i].is_jump)
      {
        // Load the delay slot registers if necessary
        if(dops[i+1].rs1!=dops[i].rs1&&dops[i+1].rs1!=dops[i].rs2&&(dops[i+1].rs1!=dops[i].rt1||dops[i].rt1==0))
          load_regs(regs[i].regmap_entry,regs[i].regmap,dops[i+1].rs1,dops[i+1].rs1);
        if(dops[i+1].rs2!=dops[i+1].rs1&&dops[i+1].rs2!=dops[i].rs1&&dops[i+1].rs2!=dops[i].rs2&&(dops[i+1].rs2!=dops[i].rt1||dops[i].rt1==0))
          load_regs(regs[i].regmap_entry,regs[i].regmap,dops[i+1].rs2,dops[i+1].rs2);
        if (ram_offset && (dops[i+1].is_load || dops[i+1].is_store))
          load_reg(regs[i].regmap_entry,regs[i].regmap,ROREG);
        if (dops[i+1].is_store)
          load_reg(regs[i].regmap_entry,regs[i].regmap,INVCP);
      }
      else if(i+1<slen)
      {
        // Preload registers for following instruction
        if(dops[i+1].rs1!=dops[i].rs1&&dops[i+1].rs1!=dops[i].rs2)
          if(dops[i+1].rs1!=dops[i].rt1&&dops[i+1].rs1!=dops[i].rt2)
            load_regs(regs[i].regmap_entry,regs[i].regmap,dops[i+1].rs1,dops[i+1].rs1);
        if(dops[i+1].rs2!=dops[i+1].rs1&&dops[i+1].rs2!=dops[i].rs1&&dops[i+1].rs2!=dops[i].rs2)
          if(dops[i+1].rs2!=dops[i].rt1&&dops[i+1].rs2!=dops[i].rt2)
            load_regs(regs[i].regmap_entry,regs[i].regmap,dops[i+1].rs2,dops[i+1].rs2);
      }
      // TODO: if(is_ooo(i)) address_generation(i+1);
      if (!dops[i].is_jump || dops[i].itype == CJUMP)
        load_reg(regs[i].regmap_entry,regs[i].regmap,CCREG);
      if (ram_offset && (dops[i].is_load || dops[i].is_store))
        load_reg(regs[i].regmap_entry,regs[i].regmap,ROREG);
      if (dops[i].is_store)
        load_reg(regs[i].regmap_entry,regs[i].regmap,INVCP);

      ds = assemble(i, &regs[i], cinfo[i].ccadj);

      drc_dbg_emit_wb_dirtys(i, &regs[i]);
      if (dops[i].is_ujump)
        literal_pool(1024);
      else
        literal_pool_jumpover(256);
    }
  }

  assert(slen > 0);
  if (slen > 0 && dops[slen-1].itype == INTCALL) {
    // no ending needed for this block since INTCALL never returns
  }
  // If the block did not end with an unconditional branch,
  // add a jump to the next instruction.
  else if (i > 1) {
    if (!dops[i-2].is_ujump) {
      assert(!dops[i-1].is_jump);
      assert(i==slen);
      if(dops[i-2].itype!=CJUMP&&dops[i-2].itype!=SJUMP) {
        store_regs_bt(regs[i-1].regmap,regs[i-1].dirty,start+i*4);
        if(regs[i-1].regmap[HOST_CCREG]!=CCREG)
          emit_loadreg(CCREG,HOST_CCREG);
        emit_addimm(HOST_CCREG, cinfo[i-1].ccadj + CLOCK_ADJUST(1), HOST_CCREG);
      }
      else
      {
        store_regs_bt(branch_regs[i-2].regmap,branch_regs[i-2].dirty,start+i*4);
        assert(branch_regs[i-2].regmap[HOST_CCREG]==CCREG);
      }
      add_to_linker(out,start+i*4,0);
      emit_jmp(0);
    }
  }
  else
  {
    assert(i>0);
    assert(!dops[i-1].is_jump);
    store_regs_bt(regs[i-1].regmap,regs[i-1].dirty,start+i*4);
    if(regs[i-1].regmap[HOST_CCREG]!=CCREG)
      emit_loadreg(CCREG,HOST_CCREG);
    emit_addimm(HOST_CCREG, cinfo[i-1].ccadj + CLOCK_ADJUST(1), HOST_CCREG);
    add_to_linker(out,start+i*4,0);
    emit_jmp(0);
  }

  // Stubs
  for(i = 0; i < stubcount; i++)
  {
    switch(stubs[i].type)
    {
      case LOADB_STUB:
      case LOADH_STUB:
      case LOADW_STUB:
      case LOADBU_STUB:
      case LOADHU_STUB:
        do_readstub(i);break;
      case STOREB_STUB:
      case STOREH_STUB:
      case STOREW_STUB:
        do_writestub(i);break;
      case CC_STUB:
        do_ccstub(i);break;
      case INVCODE_STUB:
        do_invstub(i);break;
      case STORELR_STUB:
        do_unalignedwritestub(i);break;
      case OVERFLOW_STUB:
        do_overflowstub(i); break;
      case ALIGNMENT_STUB:
        do_alignmentstub(i); break;
      default:
        assert(0);
    }
  }

  if (instr_addr0_override)
    instr_addr[0] = instr_addr0_override;

#if 0
  /* check for improper expiration */
  for (i = 0; i < ARRAY_SIZE(jumps); i++) {
    int j;
    if (!jumps[i])
      continue;
    for (j = 0; j < jumps[i]->count; j++)
      assert(jumps[i]->e[j].stub < beginning || (u_char *)jumps[i]->e[j].stub > out);
  }
#endif

  /* Pass 9 - Linker */
  for(i=0;i<linkcount;i++)
  {
    assem_debug("link: %p -> %08x\n",
      log_addr(link_addr[i].addr), link_addr[i].target);
    literal_pool(64);
    if (!link_addr[i].internal)
    {
      void *stub = out;
      void *addr = check_addr(link_addr[i].target);
      emit_extjump(link_addr[i].addr, link_addr[i].target);
      if (addr) {
        set_jump_target(link_addr[i].addr, addr);
        ndrc_add_jump_out(link_addr[i].target,stub);
      }
      else
        set_jump_target(link_addr[i].addr, stub);
    }
    else
    {
      // Internal branch
      int target=(link_addr[i].target-start)>>2;
      assert(target>=0&&target<slen);
      assert(instr_addr[target]);
      //#ifdef CORTEX_A8_BRANCH_PREDICTION_HACK
      //set_jump_target_fillslot(link_addr[i].addr,instr_addr[target],link_addr[i].ext>>1);
      //#else
      set_jump_target(link_addr[i].addr, instr_addr[target]);
      //#endif
    }
  }

  u_int source_len = slen*4;
  if (dops[slen-1].itype == INTCALL && source_len > 4)
    // no need to treat the last instruction as compiled
    // as interpreter fully handles it
    source_len -= 4;

  if ((u_char *)copy + source_len > (u_char *)shadow + sizeof(shadow))
    copy = shadow;

  // External Branch Targets (jump_in)
  int jump_in_count = 1;
  assert(instr_addr[0]);
  for (i = 1; i < slen; i++)
  {
    if (dops[i].bt && instr_addr[i])
      jump_in_count++;
  }

  struct block_info *block =
    new_block_info(start, slen * 4, source, copy, beginning, jump_in_count);
  block->reg_sv_flags = state_rflags;

  int jump_in_i = 0;
  for (i = 0; i < slen; i++)
  {
    if ((i == 0 || dops[i].bt) && instr_addr[i])
    {
      assem_debug("%p (%d) <- %8x\n", log_addr(instr_addr[i]), i, start + i*4);
      u_int vaddr = start + i*4;

      literal_pool(256);
      void *entry = out;
      load_regs_entry(i);
      if (entry == out)
        entry = instr_addr[i];
      else
        emit_jmp(instr_addr[i]);

      block->jump_in[jump_in_i].vaddr = vaddr;
      block->jump_in[jump_in_i].addr = entry;
      jump_in_i++;
    }
  }
  assert(jump_in_i == jump_in_count);
  hash_table_add(block->jump_in[0].vaddr, block->jump_in[0].addr);
  // Write out the literal pool if necessary
  literal_pool(0);
  #ifdef CORTEX_A8_BRANCH_PREDICTION_HACK
  // Align code
  if(((u_int)out)&7) emit_addnop(13);
  #endif
  assert(out - (u_char *)beginning < MAX_OUTPUT_BLOCK_SIZE);
  //printf("shadow buffer: %p-%p\n",copy,(u_char *)copy+slen*4);
  memcpy(copy, source, source_len);
  copy += source_len;

  end_block(beginning);

  // If we're within 256K of the end of the buffer,
  // start over from the beginning. (Is 256K enough?)
  if (out > ndrc->translation_cache + sizeof(ndrc->translation_cache) - MAX_OUTPUT_BLOCK_SIZE)
    out = ndrc->translation_cache;

  // Trap writes to any of the pages we compiled
  mark_invalid_code(start, slen*4, 0);

  /* Pass 10 - Free memory by expiring oldest blocks */

  pass10_expire_blocks();

#ifdef ASSEM_PRINT
  fflush(stdout);
#endif
  stat_inc(stat_bc_direct);
  return 0;
}

// vim:shiftwidth=2:expandtab
