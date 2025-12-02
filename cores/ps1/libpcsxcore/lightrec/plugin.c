#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <assert.h>

#if P_HAVE_MMAP
#include <sys/mman.h>
#endif

#include "lightrec.h"
#include "../cdrom.h"
#include "../gpu.h"
#include "../gte.h"
#include "../mdec.h"
#include "../psxdma.h"
#include "../psxhw.h"
#include "../psxmem.h"
#include "../r3000a.h"
#include "../psxinterpreter.h"
#include "../psxhle.h"
#include "../psxevents.h"

#include "../frontend/main.h"

#include "mem.h"
#include "plugin.h"

#if (defined(__arm__) || defined(__aarch64__)) && !defined(ALLOW_LIGHTREC_ON_ARM)
#error "Lightrec should not be used on ARM (please specify DYNAREC=ari64 to make)"
#endif

#define ARRAY_SIZE(x) (sizeof(x) ? sizeof(x) / sizeof((x)[0]) : 0)

#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
#	define LE32TOH(x)	__builtin_bswap32(x)
#	define HTOLE32(x)	__builtin_bswap32(x)
#	define LE16TOH(x)	__builtin_bswap16(x)
#	define HTOLE16(x)	__builtin_bswap16(x)
#else
#	define LE32TOH(x)	(x)
#	define HTOLE32(x)	(x)
#	define LE16TOH(x)	(x)
#	define HTOLE16(x)	(x)
#endif

#ifdef __GNUC__
#	define likely(x)       __builtin_expect(!!(x),1)
#	define unlikely(x)     __builtin_expect(!!(x),0)
#else
#	define likely(x)       (x)
#	define unlikely(x)     (x)
#endif

#ifndef LIGHTREC_PROG_NAME
#  ifdef __linux__
#    define LIGHTREC_PROG_NAME "/proc/self/exe"
#  else
#    define LIGHTREC_PROG_NAME "retroarch.exe"
#  endif
#endif

#ifndef GPUSTATUS_POLLING_THRESHOLD
#  define GPUSTATUS_POLLING_THRESHOLD 0
#endif

psxRegisters psxRegs;
Rcnt rcnts[4];

void* code_buffer;

static struct lightrec_state *lightrec_state;

static bool use_lightrec_interpreter;
static bool block_stepping;
//static bool use_pcsx_interpreter;
#define use_pcsx_interpreter 0
static bool ram_disabled;
static bool lightrec_debug, lightrec_very_debug;
static u32 lightrec_begin_cycles;

extern u32 lightrec_hacks;

static void lightrec_plugin_apply_config();
extern void lightrec_code_inv(void *ptr, uint32_t len);

enum my_cp2_opcodes {
	OP_CP2_RTPS		= 0x01,
	OP_CP2_NCLIP		= 0x06,
	OP_CP2_OP		= 0x0c,
	OP_CP2_DPCS		= 0x10,
	OP_CP2_INTPL		= 0x11,
	OP_CP2_MVMVA		= 0x12,
	OP_CP2_NCDS		= 0x13,
	OP_CP2_CDP		= 0x14,
	OP_CP2_NCDT		= 0x16,
	OP_CP2_NCCS		= 0x1b,
	OP_CP2_CC		= 0x1c,
	OP_CP2_NCS		= 0x1e,
	OP_CP2_NCT		= 0x20,
	OP_CP2_SQR		= 0x28,
	OP_CP2_DCPL		= 0x29,
	OP_CP2_DPCT		= 0x2a,
	OP_CP2_AVSZ3		= 0x2d,
	OP_CP2_AVSZ4		= 0x2e,
	OP_CP2_RTPT		= 0x30,
	OP_CP2_GPF		= 0x3d,
	OP_CP2_GPL		= 0x3e,
	OP_CP2_NCCT		= 0x3f,
};

static void (*cp2_ops[])(struct psxCP2Regs *) = {
	[OP_CP2_RTPS] = gteRTPS,
	[OP_CP2_NCLIP] = gteNCLIP,
	[OP_CP2_OP] = gteOP,
	[OP_CP2_DPCS] = gteDPCS,
	[OP_CP2_INTPL] = gteINTPL,
	[OP_CP2_MVMVA] = gteMVMVA,
	[OP_CP2_NCDS] = gteNCDS,
	[OP_CP2_CDP] = gteCDP,
	[OP_CP2_NCDT] = gteNCDT,
	[OP_CP2_NCCS] = gteNCCS,
	[OP_CP2_CC] = gteCC,
	[OP_CP2_NCS] = gteNCS,
	[OP_CP2_NCT] = gteNCT,
	[OP_CP2_SQR] = gteSQR,
	[OP_CP2_DCPL] = gteDCPL,
	[OP_CP2_DPCT] = gteDPCT,
	[OP_CP2_AVSZ3] = gteAVSZ3,
	[OP_CP2_AVSZ4] = gteAVSZ4,
	[OP_CP2_RTPT] = gteRTPT,
	[OP_CP2_GPF] = gteGPF,
	[OP_CP2_GPL] = gteGPL,
	[OP_CP2_NCCT] = gteNCCT,
};

static char cache_buf[64 * 1024];

static void cop2_op(struct lightrec_state *state, u32 func)
{
	struct lightrec_registers *regs = lightrec_get_registers(state);

	psxRegs.code = func;

	if (unlikely(!cp2_ops[func & 0x3f])) {
		fprintf(stderr, "Invalid CP2 function %u\n", func);
	} else {
		/* This works because regs->cp2c comes right after regs->cp2d,
		 * so it can be cast to a pcsxCP2Regs pointer. */
		cp2_ops[func & 0x3f]((psxCP2Regs *) regs->cp2d);
	}
}

static bool has_interrupt(void)
{
	struct lightrec_registers *regs = lightrec_get_registers(lightrec_state);

	return ((psxHu32(0x1070) & psxHu32(0x1074)) &&
		(regs->cp0[12] & 0x401) == 0x401) ||
		(regs->cp0[12] & regs->cp0[13] & 0x0300);
}

static void lightrec_tansition_to_pcsx(struct lightrec_state *state)
{
	psxRegs.cycle += lightrec_current_cycle_count(state) / 1024;
	lightrec_reset_cycle_count(state, 0);
}

static void lightrec_tansition_from_pcsx(struct lightrec_state *state)
{
	s32 cycles_left = psxRegs.next_interupt - psxRegs.cycle;

	if (block_stepping || cycles_left <= 0 || has_interrupt())
		lightrec_set_exit_flags(state, LIGHTREC_EXIT_CHECK_INTERRUPT);
	else {
		lightrec_set_target_cycle_count(state, cycles_left * 1024);
	}
}

static void hw_write_byte(struct lightrec_state *state,
			  u32 op, void *host, u32 mem, u32 val)
{
	lightrec_tansition_to_pcsx(state);

	psxHwWrite8(mem, val);

	lightrec_tansition_from_pcsx(state);
}

static void hw_write_half(struct lightrec_state *state,
			  u32 op, void *host, u32 mem, u32 val)
{
	lightrec_tansition_to_pcsx(state);

	psxHwWrite16(mem, val);

	lightrec_tansition_from_pcsx(state);
}

static void hw_write_word(struct lightrec_state *state,
			  u32 op, void *host, u32 mem, u32 val)
{
	lightrec_tansition_to_pcsx(state);

	psxHwWrite32(mem, val);

	lightrec_tansition_from_pcsx(state);
}

static u8 hw_read_byte(struct lightrec_state *state, u32 op, void *host, u32 mem)
{
	u8 val;

	lightrec_tansition_to_pcsx(state);

	val = psxHwRead8(mem);

	lightrec_tansition_from_pcsx(state);

	return val;
}

static u16 hw_read_half(struct lightrec_state *state,
			u32 op, void *host, u32 mem)
{
	u16 val;

	lightrec_tansition_to_pcsx(state);

	val = psxHwRead16(mem);

	lightrec_tansition_from_pcsx(state);

	return val;
}

static u32 hw_read_word(struct lightrec_state *state,
			u32 op, void *host, u32 mem)
{
	static u32 old_cycle, oldold_cycle, old_gpusr;
	u32 val, diff;

	lightrec_tansition_to_pcsx(state);

	val = psxHwRead32(mem);

	if (GPUSTATUS_POLLING_THRESHOLD > 0 && mem == 0x1f801814) {
		diff = psxRegs.cycle - old_cycle;

		if (diff > 0
		    && diff < GPUSTATUS_POLLING_THRESHOLD
		    && diff == old_cycle - oldold_cycle) {
			while (psxRegs.next_interupt > psxRegs.cycle && val == old_gpusr) {
				psxRegs.cycle += diff;
				val = psxHwRead32(mem);
			}
		}

		oldold_cycle = old_cycle;
		old_cycle = psxRegs.cycle;
		old_gpusr = val;
	}

	lightrec_tansition_from_pcsx(state);

	return val;
}

static struct lightrec_mem_map_ops hw_regs_ops = {
	.sb = hw_write_byte,
	.sh = hw_write_half,
	.sw = hw_write_word,
	.lb = hw_read_byte,
	.lh = hw_read_half,
	.lw = hw_read_word,
};

static u32 cache_ctrl;

static void cache_ctrl_write_word(struct lightrec_state *state,
				  u32 op, void *host, u32 mem, u32 val)
{
	cache_ctrl = val;
}

static u32 cache_ctrl_read_word(struct lightrec_state *state,
				u32 op, void *host, u32 mem)
{
	return cache_ctrl;
}

static struct lightrec_mem_map_ops cache_ctrl_ops = {
	.sw = cache_ctrl_write_word,
	.lw = cache_ctrl_read_word,
};

static struct lightrec_mem_map lightrec_map[] = {
	[PSX_MAP_KERNEL_USER_RAM] = {
		/* Kernel and user memory */
		.pc = 0x00000000,
		.length = 0x200000,
	},
	[PSX_MAP_BIOS] = {
		/* BIOS */
		.pc = 0x1fc00000,
		.length = 0x80000,
	},
	[PSX_MAP_SCRATCH_PAD] = {
		/* Scratch pad */
		.pc = 0x1f800000,
		.length = 0x400,
	},
	[PSX_MAP_PARALLEL_PORT] = {
		/* Parallel port */
		.pc = 0x1f000000,
		.length = 0x10000,
	},
	[PSX_MAP_HW_REGISTERS] = {
		/* Hardware registers */
		.pc = 0x1f801000,
		.length = 0x8000,
		.ops = &hw_regs_ops,
	},
	[PSX_MAP_CACHE_CONTROL] = {
		/* Cache control */
		.pc = 0x5ffe0130,
		.length = 4,
		.ops = &cache_ctrl_ops,
	},

	/* Mirrors of the kernel/user memory */
	[PSX_MAP_MIRROR1] = {
		.pc = 0x00200000,
		.length = 0x200000,
		.mirror_of = &lightrec_map[PSX_MAP_KERNEL_USER_RAM],
	},
	[PSX_MAP_MIRROR2] = {
		.pc = 0x00400000,
		.length = 0x200000,
		.mirror_of = &lightrec_map[PSX_MAP_KERNEL_USER_RAM],
	},
	[PSX_MAP_MIRROR3] = {
		.pc = 0x00600000,
		.length = 0x200000,
		.mirror_of = &lightrec_map[PSX_MAP_KERNEL_USER_RAM],
	},

	/* Mirror of the parallel port. Only used by the PS2/PS3 BIOS */
	[PSX_MAP_PPORT_MIRROR] = {
		.pc = 0x1fa00000,
		.length = 0x10000,
		.mirror_of = &lightrec_map[PSX_MAP_PARALLEL_PORT],
	},

	/* Code buffer */
	[PSX_MAP_CODE_BUFFER] = {
		.length = CODE_BUFFER_SIZE,
	},
};

static void lightrec_enable_ram(struct lightrec_state *state, bool enable)
{
	if (enable)
		memcpy(psxM, cache_buf, sizeof(cache_buf));
	else
		memcpy(cache_buf, psxM, sizeof(cache_buf));

	ram_disabled = !enable;
}

static bool lightrec_can_hw_direct(u32 kaddr, bool is_write, u8 size)
{
	if (is_write && size != 32) {
		// force32 so must go through handlers
		if (0x1f801000 <= kaddr && kaddr < 0x1f801024)
			return false;
		if ((kaddr & 0x1fffff80) == 0x1f801080) // dma
			return false;
	}

	switch (size) {
	case 8:
		switch (kaddr) {
		case 0x1f801040:
		case 0x1f801050:
		case 0x1f801800:
		case 0x1f801801:
		case 0x1f801802:
		case 0x1f801803:
			return false;
		default:
			return true;
		}
	case 16:
		switch (kaddr) {
		case 0x1f801040:
		case 0x1f801044:
		case 0x1f801048:
		case 0x1f80104a:
		case 0x1f80104e:
		case 0x1f801050:
		case 0x1f801054:
		case 0x1f80105a:
		case 0x1f80105e:
		case 0x1f801100:
		case 0x1f801104:
		case 0x1f801108:
		case 0x1f801110:
		case 0x1f801114:
		case 0x1f801118:
		case 0x1f801120:
		case 0x1f801124:
		case 0x1f801128:
			return false;
		case 0x1f801070:
		case 0x1f801074:
			return !is_write;
		default:
			return kaddr < 0x1f801c00 || kaddr >= 0x1f801e00;
		}
	default:
		switch (kaddr) {
		case 0x1f801040:
		case 0x1f801050:
		case 0x1f801100:
		case 0x1f801104:
		case 0x1f801108:
		case 0x1f801110:
		case 0x1f801114:
		case 0x1f801118:
		case 0x1f801120:
		case 0x1f801124:
		case 0x1f801128:
		case 0x1f801810:
		case 0x1f801814:
		case 0x1f801820:
		case 0x1f801824:
			return false;
		case 0x1f801070:
		case 0x1f801074:
		case 0x1f801088:
		case 0x1f801098:
		case 0x1f8010a8:
		case 0x1f8010b8:
		case 0x1f8010c8:
		case 0x1f8010e8:
		case 0x1f8010f4:
			return !is_write;
		default:
			return !is_write || kaddr < 0x1f801c00 || kaddr >= 0x1f801e00;
		}
	}
}

static const struct lightrec_ops lightrec_ops = {
	.cop2_op = cop2_op,
	.enable_ram = lightrec_enable_ram,
	.hw_direct = lightrec_can_hw_direct,
	.code_inv = LIGHTREC_CODE_INV ? lightrec_code_inv : NULL,
};

static int lightrec_plugin_init(void)
{
	lightrec_map[PSX_MAP_KERNEL_USER_RAM].address = psxM;
	lightrec_map[PSX_MAP_BIOS].address = psxR;
	lightrec_map[PSX_MAP_SCRATCH_PAD].address = psxH;
	lightrec_map[PSX_MAP_HW_REGISTERS].address = psxH + 0x1000;
	lightrec_map[PSX_MAP_PARALLEL_PORT].address = psxP;

	if (!LIGHTREC_CUSTOM_MAP) {
#if P_HAVE_MMAP
		code_buffer = mmap(0, CODE_BUFFER_SIZE,
				   PROT_EXEC | PROT_READ | PROT_WRITE,
				   MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
		if (code_buffer == MAP_FAILED)
			return -ENOMEM;
#else
		code_buffer = malloc(CODE_BUFFER_SIZE);
		if (!code_buffer)
			return -ENOMEM;
#endif
	}

	if (LIGHTREC_CUSTOM_MAP) {
		lightrec_map[PSX_MAP_MIRROR1].address = psxM + 0x200000;
		lightrec_map[PSX_MAP_MIRROR2].address = psxM + 0x400000;
		lightrec_map[PSX_MAP_MIRROR3].address = psxM + 0x600000;
	}

	lightrec_map[PSX_MAP_CODE_BUFFER].address = code_buffer;

	use_lightrec_interpreter = !!getenv("LIGHTREC_INTERPRETER");

#ifdef LIGHTREC_DEBUG
	char *cycles = getenv("LIGHTREC_BEGIN_CYCLES");

	lightrec_very_debug = !!getenv("LIGHTREC_VERY_DEBUG");
	lightrec_debug = lightrec_very_debug || !!getenv("LIGHTREC_DEBUG");

	if (cycles)
		lightrec_begin_cycles = (unsigned int) strtol(cycles, NULL, 0);
#endif

	lightrec_state = lightrec_init(LIGHTREC_PROG_NAME,
			lightrec_map, ARRAY_SIZE(lightrec_map),
			&lightrec_ops);

	// fprintf(stderr, "M=0x%lx, P=0x%lx, R=0x%lx, H=0x%lx\n",
	// 		(uintptr_t) psxM,
	// 		(uintptr_t) psxP,
	// 		(uintptr_t) psxR,
	// 		(uintptr_t) psxH);

#ifndef _WIN32
	signal(SIGPIPE, exit);
#endif
	lightrec_plugin_apply_config();
	return 0;
}

static u32 do_calculate_hash(const void *buffer, u32 count, u32 needle, bool le)
{
	unsigned int i;
	const u32 *data = (const u32 *) buffer;
	u32 hash = needle;

	count /= 4;
	for(i = 0; i < count; ++i) {
		hash += le ? LE32TOH(data[i]) : data[i];
		hash += (hash << 10);
		hash ^= (hash >> 6);
	}

	hash += (hash << 3);
	hash ^= (hash >> 11);
	hash += (hash << 15);

	return hash;
}

static u32 hash_calculate_le(const void *buffer, u32 count)
{
	return do_calculate_hash(buffer, count, 0xffffffff, true);
}

u32 hash_calculate(const void *buffer, u32 count)
{
	return do_calculate_hash(buffer, count, 0xffffffff, false);
}

static u32 hash_calculate_ram(const void *buffer, u32 ram_size)
{
	u32 hash;

	if (ram_disabled)
		hash = hash_calculate_le(cache_buf, sizeof(cache_buf));
	else
		hash = hash_calculate_le(buffer, sizeof(cache_buf));

	return do_calculate_hash(buffer + sizeof(cache_buf),
				 ram_size - sizeof(cache_buf),
				 hash, true);
}

static const char * const mips_regs[] = {
	"zero",
	"at",
	"v0", "v1",
	"a0", "a1", "a2", "a3",
	"t0", "t1", "t2", "t3", "t4", "t5", "t6", "t7",
	"s0", "s1", "s2", "s3", "s4", "s5", "s6", "s7",
	"t8", "t9",
	"k0", "k1",
	"gp", "sp", "fp", "ra",
	"lo", "hi",
};

static void print_for_big_ass_debugger(void)
{
	struct lightrec_registers *regs;
	unsigned int i;

	regs = lightrec_get_registers(lightrec_state);

	printf("CYCLE 0x%08x PC 0x%08x", psxRegs.cycle, psxRegs.pc);

	if (lightrec_very_debug)
		printf(" RAM 0x%08x SCRATCH 0x%08x HW 0x%08x",
				hash_calculate_ram(psxM, 0x200000),
				hash_calculate_le(psxH, 0x400),
				hash_calculate_le(psxH + 0x1000, 0x2000));

	printf(" CP0 0x%08x CP2D 0x%08x CP2C 0x%08x INT 0x%04x INTCYCLE 0x%08x GPU 0x%08x",
			hash_calculate(regs->cp0, sizeof(regs->cp0)),
			hash_calculate(regs->cp2d, sizeof(regs->cp2d)),
			hash_calculate(regs->cp2c, sizeof(regs->cp2c)),
			psxRegs.interrupt,
			hash_calculate(psxRegs.intCycle, sizeof(psxRegs.intCycle)),
			LE32TOH(HW_GPU_STATUS));

	if (lightrec_very_debug) {
		for (i = 0; i < 32; i++)
			printf(" CP2D%u 0x%08x", i, regs->cp2d[i]);
		for (i = 0; i < 32; i++)
			printf(" CP2C%u 0x%08x", i, regs->cp2c[i]);
	}

	if (lightrec_very_debug)
		for (i = 0; i < 34; i++)
			printf(" %s 0x%08x", mips_regs[i], regs->gpr[i]);
	else
		printf(" GPR 0x%08x",
		       hash_calculate(regs->gpr, sizeof(regs->gpr)));
	printf("\n");

	fflush(stdout);
}

static void lightrec_plugin_sync_regs_to_pcsx(bool need_cp2);
static void lightrec_plugin_sync_regs_from_pcsx(bool need_cp2);

static void lightrec_plugin_execute_internal(bool block_only)
{
	struct lightrec_registers *regs;
	u32 flags, cycles_pcsx;
	u32 old_pc = psxRegs.pc;

	regs = lightrec_get_registers(lightrec_state);
	gen_interupt((psxCP0Regs *)regs->cp0);
	if (!block_only && psxRegs.stop)
		return;

	cycles_pcsx = psxRegs.next_interupt - psxRegs.cycle;
	assert((s32)cycles_pcsx > 0);

	// step during early boot so that 0x80030000 fastboot hack works
	block_stepping = block_only;
	if (block_only)
		cycles_pcsx = 0;

	if (use_pcsx_interpreter) {
		psxInt.ExecuteBlock(&psxRegs, 0);
	} else {
		u32 cycles_lightrec = cycles_pcsx * 1024;
		if (unlikely(use_lightrec_interpreter)) {
			psxRegs.pc = lightrec_run_interpreter(lightrec_state,
							      psxRegs.pc,
							      cycles_lightrec);
		} else {
			psxRegs.pc = lightrec_execute(lightrec_state,
						      psxRegs.pc, cycles_lightrec);
		}

		lightrec_tansition_to_pcsx(lightrec_state);

		flags = lightrec_exit_flags(lightrec_state);

		if (flags & LIGHTREC_EXIT_SEGFAULT) {
			fprintf(stderr, "Exiting at cycle 0x%08x\n",
				psxRegs.cycle);
			if (lightrec_debug)
				print_for_big_ass_debugger();
			exit(1);
		}

		if (flags & LIGHTREC_EXIT_SYSCALL)
			psxException(R3000E_Syscall << 2, 0, (psxCP0Regs *)regs->cp0);
		if (flags & LIGHTREC_EXIT_BREAK)
			psxException(R3000E_Bp << 2, 0, (psxCP0Regs *)regs->cp0);
		else if (flags & LIGHTREC_EXIT_UNKNOWN_OP) {
			u32 op = intFakeFetch(psxRegs.pc);
			u32 hlec = op & 0x03ffffff;
			if ((op >> 26) == 0x3b && hlec < ARRAY_SIZE(psxHLEt) && Config.HLE) {
				lightrec_plugin_sync_regs_to_pcsx(0);
				psxHLEt[hlec]();
				lightrec_plugin_sync_regs_from_pcsx(0);
			}
			else
				psxException(R3000E_RI << 2, 0, (psxCP0Regs *)regs->cp0);
		}
	}

	if (lightrec_debug && psxRegs.cycle >= lightrec_begin_cycles && psxRegs.pc != old_pc) {
		print_for_big_ass_debugger();
	}

	if ((regs->cp0[13] & regs->cp0[12] & 0x300) && (regs->cp0[12] & 0x1)) {
		/* Handle software interrupts */
		regs->cp0[13] &= ~0x7c;
		psxException(regs->cp0[13], 0, (psxCP0Regs *)regs->cp0);
	}
}

static void lightrec_plugin_execute(psxRegisters *regs)
{
	while (!regs->stop)
		lightrec_plugin_execute_internal(lightrec_very_debug);
}

static void lightrec_plugin_execute_block(psxRegisters *regs,
	enum blockExecCaller caller)
{
	lightrec_plugin_execute_internal(true);
}

static void lightrec_plugin_clear(u32 addr, u32 size)
{
	if ((addr == 0 && size == UINT32_MAX)
	    || (lightrec_hacks & LIGHTREC_OPT_INV_DMA_ONLY))
		lightrec_invalidate_all(lightrec_state);
	else
		/* size * 4: PCSX uses DMA units */
		lightrec_invalidate(lightrec_state, addr, size * 4);
}

static void lightrec_plugin_notify(enum R3000Anote note, void *data)
{
	switch (note)
	{
	case R3000ACPU_NOTIFY_CACHE_ISOLATED:
	case R3000ACPU_NOTIFY_CACHE_UNISOLATED:
		/* not used, lightrec calls lightrec_enable_ram() instead */
		break;
	case R3000ACPU_NOTIFY_BEFORE_SAVE:
		/* non-null 'data' means this is HLE related sync */
		lightrec_plugin_sync_regs_to_pcsx(data == NULL);
		break;
	case R3000ACPU_NOTIFY_AFTER_LOAD:
		lightrec_plugin_sync_regs_from_pcsx(data == NULL);
		if (data == NULL)
			lightrec_invalidate_all(lightrec_state);
		break;
	}
}

static void lightrec_plugin_apply_config()
{
	static u32 cycles_per_op_old;
	u32 cycle_mult = Config.cycle_multiplier_override && Config.cycle_multiplier == CYCLE_MULT_DEFAULT
		? Config.cycle_multiplier_override : Config.cycle_multiplier;
	u32 cycles_per_op = cycle_mult * 1024 / 100;
	assert(cycles_per_op);

	if (cycles_per_op_old && cycles_per_op_old != cycles_per_op) {
		SysPrintf("lightrec: reinit block cache for cycles_per_op %.2f\n",
			cycles_per_op / 1024.f);
	}
	cycles_per_op_old = cycles_per_op;
	lightrec_set_cycles_per_opcode(lightrec_state, cycles_per_op);

	lightrec_set_unsafe_opt_flags(lightrec_state, lightrec_hacks);
	intApplyConfig();
}

static void lightrec_plugin_shutdown(void)
{
	lightrec_destroy(lightrec_state);

	if (!LIGHTREC_CUSTOM_MAP) {
#if P_HAVE_MMAP
		munmap(code_buffer, CODE_BUFFER_SIZE);
#else
		free(code_buffer);
#endif
	}
}

static void lightrec_plugin_reset(void)
{
	struct lightrec_registers *regs;

	regs = lightrec_get_registers(lightrec_state);

	/* Invalidate all blocks */
	lightrec_invalidate_all(lightrec_state);

	/* Reset registers */
	memset(regs, 0, sizeof(*regs));

	regs->cp0[12] = 0x10900000; // COP0 enabled | BEV = 1 | TS = 1
	regs->cp0[15] = 0x00000002; // PRevID = Revision ID, same as R3000A
}

static void lightrec_plugin_sync_regs_from_pcsx(bool need_cp2)
{
	struct lightrec_registers *regs;

	regs = lightrec_get_registers(lightrec_state);
	memcpy(regs->gpr, &psxRegs.GPR, sizeof(regs->gpr));
	memcpy(regs->cp0, &psxRegs.CP0, sizeof(regs->cp0));
	if (need_cp2)
		memcpy(regs->cp2d, &psxRegs.CP2, sizeof(regs->cp2d) + sizeof(regs->cp2c));
}

static void lightrec_plugin_sync_regs_to_pcsx(bool need_cp2)
{
	struct lightrec_registers *regs;

	regs = lightrec_get_registers(lightrec_state);
	memcpy(&psxRegs.GPR, regs->gpr, sizeof(regs->gpr));
	memcpy(&psxRegs.CP0, regs->cp0, sizeof(regs->cp0));
	if (need_cp2)
		memcpy(&psxRegs.CP2, regs->cp2d, sizeof(regs->cp2d) + sizeof(regs->cp2c));
}

R3000Acpu psxRec =
{
	lightrec_plugin_init,
	lightrec_plugin_reset,
	lightrec_plugin_execute,
	lightrec_plugin_execute_block,
	lightrec_plugin_clear,
	lightrec_plugin_notify,
	lightrec_plugin_apply_config,
	lightrec_plugin_shutdown,
};
