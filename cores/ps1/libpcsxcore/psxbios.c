/***************************************************************************
 *   Copyright (C) 2019 Ryan Schultz, PCSX-df Team, PCSX team, gameblabla, *
 *   dmitrysmagin, senquack                                                *
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
 *   51 Franklin Street, Fifth Floor, Boston, MA 02111-1307 USA.           *
 ***************************************************************************/

/* Gameblabla 2018-2019 :
 * Numerous changes to bios calls as well as improvements in order to conform to nocash's findings
 * for the PSX bios calls. Thanks senquack for helping out with some of the changes
 * and helping to spot issues and refine my patches.
 * */

/*
 * Internal simulated HLE BIOS.
 */

// TODO: implement all system calls, count the exact CPU cycles of system calls.

#include "psxbios.h"
#include "psxhw.h"
#include "gpu.h"
#include "sio.h"
#include "psxhle.h"
#include "psxinterpreter.h"
#include "psxevents.h"
#include "cdrom.h"
#include <stdarg.h>
#include <zlib.h>

#ifndef PSXBIOS_LOG
//#define PSXBIOS_LOG printf
#define PSXBIOS_LOG(...)
#endif
#ifndef PSXBIOS_EV_LOG
//#define PSXBIOS_EV_LOG printf
#define PSXBIOS_EV_LOG(...)
#endif

#define PTR_1 (void *)(size_t)1

char *biosA0n[256] = {
// 0x00
	"open",		"lseek",	"read",		"write",
	"close",	"ioctl",	"exit",		"sys_a0_07",
	"getc",		"putc",		"todigit",	"atof",
	"strtoul",	"strtol",	"abs",		"labs",
// 0x10
	"atoi",		"atol",		"atob",		"setjmp",
	"longjmp",	"strcat",	"strncat",	"strcmp",
	"strncmp",	"strcpy",	"strncpy",	"strlen",
	"index",	"rindex",	"strchr",	"strrchr",
// 0x20
	"strpbrk",	"strspn",	"strcspn",	"strtok",
	"strstr",	"toupper",	"tolower",	"bcopy",
	"bzero",	"bcmp",		"memcpy",	"memset",
	"memmove",	"memcmp",	"memchr",	"rand",
// 0x30
	"srand",	"qsort",	"strtod",	"malloc",
	"free",		"lsearch",	"bsearch",	"calloc",
	"realloc",	"InitHeap",	"_exit",	"getchar",
	"putchar",	"gets",		"puts",		"printf",
// 0x40
	"SystemErrorUnresolvedException", "LoadTest",		"Load",		"Exec",
	"FlushCache",		"InstallInterruptHandler",	"GPU_dw",	"mem2vram",
	"SendGPUStatus",	"GPU_cw",			"GPU_cwb",	"SendPackets",
	"sys_a0_4c",		"GetGPUStatus",				"GPU_sync",	"sys_a0_4f",
// 0x50
	"sys_a0_50",		"LoadExec",				"GetSysSp",		"sys_a0_53",
	"_96_init()",		"_bu_init()",			"_96_remove()",	"sys_a0_57",
	"sys_a0_58",		"sys_a0_59",			"sys_a0_5a",	"dev_tty_init",
	"dev_tty_open",		"sys_a0_5d",			"dev_tty_ioctl","dev_cd_open",
// 0x60
	"dev_cd_read",		"dev_cd_close",			"dev_cd_firstfile",	"dev_cd_nextfile",
	"dev_cd_chdir",		"dev_card_open",		"dev_card_read",	"dev_card_write",
	"dev_card_close",	"dev_card_firstfile",	"dev_card_nextfile","dev_card_erase",
	"dev_card_undelete","dev_card_format",		"dev_card_rename",	"dev_card_6f",
// 0x70
	"_bu_init",		"CdInit",	"CdRemove",		"sys_a0_73",
	"sys_a0_74",		"sys_a0_75",	"sys_a0_76",		"sys_a0_77",
	"_96_CdSeekL",		"sys_a0_79",	"sys_a0_7a",		"sys_a0_7b",
	"_96_CdGetStatus",	"sys_a0_7d",	"_96_CdRead",		"sys_a0_7f",
// 0x80
	"sys_a0_80",		"sys_a0_81",	"sys_a0_82",		"sys_a0_83",
	"sys_a0_84",		"_96_CdStop",	"sys_a0_86",		"sys_a0_87",
	"sys_a0_88",		"sys_a0_89",	"sys_a0_8a",		"sys_a0_8b",
	"sys_a0_8c",		"sys_a0_8d",	"sys_a0_8e",		"sys_a0_8f",
// 0x90
	"sys_a0_90",		"sys_a0_91",	"sys_a0_92",		"sys_a0_93",
	"sys_a0_94",		"CdReset",	"AddCDROMDevice",	"AddMemCardDevide",
	"DisableKernelIORedirection",		"EnableKernelIORedirection", "sys_a0_9a", "sys_a0_9b",
	"SetConf",			"GetConf",		"sys_a0_9e",		"SetMem",
// 0xa0
	"_boot",			"SystemError",	"EnqueueCdIntr",	"DequeueCdIntr",
	"sys_a0_a4",		"ReadSector",	"get_cd_status",	"bufs_cb_0",
	"bufs_cb_1",		"bufs_cb_2",	"bufs_cb_3",		"_card_info",
	"_card_load",		"_card_auto",	"bufs_cd_4",		"sys_a0_af",
// 0xb0
	"sys_a0_b0",		"sys_a0_b1",	"do_a_long_jmp",	"sys_a0_b3",
	"GetSystemInfo",
};

char *biosB0n[256] = {
// 0x00
	"SysMalloc",		"sys_b0_01",	"sys_b0_02",	"sys_b0_03",
	"sys_b0_04",		"sys_b0_05",	"sys_b0_06",	"DeliverEvent",
	"OpenEvent",		"CloseEvent",	"WaitEvent",	"TestEvent",
	"EnableEvent",		"DisableEvent",	"OpenTh",	"CloseTh",
// 0x10
	"ChangeTh",		"sys_b0_11",	"InitPAD",	"StartPAD",
	"StopPAD",		"PAD_init",	"PAD_dr",	"ReturnFromException",
	"ResetEntryInt",	"HookEntryInt",	"sys_b0_1a",	"sys_b0_1b",
	"sys_b0_1c",		"sys_b0_1d",	"sys_b0_1e",	"sys_b0_1f",
// 0x20
	"UnDeliverEvent",	"sys_b0_21",	"sys_b0_22",	"sys_b0_23",
	"sys_b0_24",		"sys_b0_25",	"sys_b0_26",	"sys_b0_27",
	"sys_b0_28",		"sys_b0_29",	"sys_b0_2a",	"sys_b0_2b",
	"sys_b0_2c",		"sys_b0_2d",	"sys_b0_2e",	"sys_b0_2f",
// 0x30
	"sys_b0_30",		"sys_b0_31",	"open",			"lseek",
	"read",				"write",		"close",		"ioctl",
	"exit",				"sys_b0_39",	"getc",			"putc",
	"getchar",			"putchar",		"gets",			"puts",
// 0x40
	"cd",				"format",		"firstfile",	"nextfile",
	"rename",			"delete",		"undelete",		"AddDevice",
	"RemoteDevice",		"PrintInstalledDevices", "InitCARD", "StartCARD",
	"StopCARD",			"sys_b0_4d",	"_card_write",	"_card_read",
// 0x50
	"_new_card",		"Krom2RawAdd",	"sys_b0_52",	"sys_b0_53",
	"_get_errno",		"_get_error",	"GetC0Table",	"GetB0Table",
	"_card_chan",		"sys_b0_59",	"sys_b0_5a",	"ChangeClearPAD",
	"_card_status",		"_card_wait",
};

char *biosC0n[256] = {
// 0x00
	"InitRCnt",			  "InitException",		"SysEnqIntRP",		"SysDeqIntRP",
	"get_free_EvCB_slot", "get_free_TCB_slot",	"ExceptionHandler",	"InstallExeptionHandler",
	"SysInitMemory",	  "SysInitKMem",		"ChangeClearRCnt",	"SystemError",
	"InitDefInt",		  "sys_c0_0d",			"sys_c0_0e",		"sys_c0_0f",
// 0x10
	"sys_c0_10",		  "sys_c0_11",			"InstallDevices",	"FlushStfInOutPut",
	"sys_c0_14",		  "_cdevinput",			"_cdevscan",		"_circgetc",
	"_circputc",		  "ioabort",			"sys_c0_1a",		"KernelRedirect",
	"PatchAOTable",
};

//#define r0 (psxRegs.GPR.n.r0)
#define at (psxRegs.GPR.n.at)
#define v0 (psxRegs.GPR.n.v0)
#define v1 (psxRegs.GPR.n.v1)
#define a0 (psxRegs.GPR.n.a0)
#define a1 (psxRegs.GPR.n.a1)
#define a2 (psxRegs.GPR.n.a2)
#define a3 (psxRegs.GPR.n.a3)
#define t0 (psxRegs.GPR.n.t0)
#define t1 (psxRegs.GPR.n.t1)
#define t2 (psxRegs.GPR.n.t2)
#define t3 (psxRegs.GPR.n.t3)
#define t4 (psxRegs.GPR.n.t4)
#define t5 (psxRegs.GPR.n.t5)
#define t6 (psxRegs.GPR.n.t6)
#define t7 (psxRegs.GPR.n.t7)
#define t8 (psxRegs.GPR.n.t8)
#define t9 (psxRegs.GPR.n.t9)
#define s0 (psxRegs.GPR.n.s0)
#define s1 (psxRegs.GPR.n.s1)
#define s2 (psxRegs.GPR.n.s2)
#define s3 (psxRegs.GPR.n.s3)
#define s4 (psxRegs.GPR.n.s4)
#define s5 (psxRegs.GPR.n.s5)
#define s6 (psxRegs.GPR.n.s6)
#define s7 (psxRegs.GPR.n.s7)
#define k0 (psxRegs.GPR.n.k0)
#define k1 (psxRegs.GPR.n.k1)
#define gp (psxRegs.GPR.n.gp)
#define sp (psxRegs.GPR.n.sp)
#define fp (psxRegs.GPR.n.fp)
#define ra (psxRegs.GPR.n.ra)
#define pc0 (psxRegs.pc)

#define Ra0 ((char *)PSXM(a0))
#define Ra1 ((char *)PSXM(a1))
#define Ra2 ((char *)PSXM(a2))
#define Ra3 ((char *)PSXM(a3))
#define Rv0 ((char *)PSXM(v0))
#define Rsp ((char *)PSXM(sp))

typedef struct {
	u32 class;
	u32 status;
	u32 spec;
	u32 mode;
	u32 fhandler;
	u32 unused[2];
} EvCB;

#define EvStUNUSED      0x0000
#define EvStDISABLED    0x1000
#define EvStACTIVE      0x2000
#define EvStALREADY     0x4000

#define EvMdCALL        0x1000
#define EvMdMARK        0x2000

typedef struct {
	u32 status;
	u32 mode;
	u32 reg[32];
	u32 epc;
	u32 hi, lo;
	u32 sr, cause;
	u32 unused[9];
} TCB;

typedef struct {
	u32 _pc0;
	u32 gp0;
	u32 t_addr;
	u32 t_size;
	u32 d_addr; // 10
	u32 d_size;
	u32 b_addr;
	u32 b_size; // 1c
	u32 S_addr;
	u32 s_size;
	u32 _sp, _fp, _gp, ret, base;
} EXEC;

struct DIRENTRY {
	char name[20];
	s32 attr;
	s32 size;
	u32 next;
	s32 head;
	char system[4];
};

typedef struct {
	char name[32];
	u32  mode;
	u32  offset;
	u32  size;
	u32  mcfile;
} FileDesc;

// todo: FileDesc layout is wrong
// todo: get rid of these globals
static FileDesc FDesc[32];
static char ffile[64];
static int nfile;
static char cdir[8*8+8];
static u32 floodchk;
static int card_io_delay;

// fixed RAM offsets, SCPH1001 compatible
#define A_TT_ExCB       0x0100
#define A_TT_PCB        0x0108
#define A_TT_TCB        0x0110
#define A_TT_EvCB       0x0120
#define A_A0_TABLE      0x0200
#define A_B0_TABLE      0x0874
#define A_C0_TABLE      0x0674
#define A_SYSCALL       0x0650
#define A_EXCEPTION     0x0c80
#define A_EXC_SP        0x6cf0
#define A_EEXIT_DEF     0x6cf4
#define A_CARD_ISLOT    0x7264  // 0 or 1, toggled by card vint handler
#define A_KMALLOC_PTR   0x7460
#define A_KMALLOC_SIZE  0x7464
#define A_KMALLOC_END   0x7468
#define A_PADCRD_CHN_E  0x74a8  // pad/card irq chain entry, see hleExcPadCard1()
#define A_PAD_IRQR_ENA  0x74b8  // pad read on vint irq (nocash 'pad_enable_flag')
#define A_CARD_IRQR_ENA 0x74bc  // same for card
#define A_PAD_INBUF     0x74c8  // 2x buffers for rx pad data
#define A_PAD_OUTBUF    0x74d0  // 2x buffers for tx pad data
#define A_PAD_IN_LEN    0x74d8
#define A_PAD_OUT_LEN   0x74e0
#define A_PAD_DR_DST    0x74c4
#define A_CARD_ACHAN    0x7500  // currently active port in 0xPortSlot format
#define A_CARD_HANDLER  0x7528  // ptr to irq handler
#define A_CARD_STATUS1  0x7568
#define A_CARD_STATUS2  0x7569
#define A_PAD_DR_BUF1   0x7570
#define A_PAD_DR_BUF2   0x7598
#define A_EEXIT_PTR     0x75d0
#define A_EXC_STACK     0x85d8  // exception stack top
#define A_RCNT_VBL_ACK  0x8600
#define A_PAD_ACK_VBL   0x8914  // enable vint ack by pad reading code
#define A_HEAP_BASE     0x9000
#define A_HEAP_SIZE     0x9004
#define A_HEAP_END      0x9008
#define A_HEAP_INIT_FLG 0x900c
#define A_RND_SEED      0x9010
#define A_HEAP_FRSTCHNK 0xb060
#define A_HEAP_CURCHNK  0xb064
#define A_CONF_TCB      0xb940
#define A_CONF_EvCB     0xb944
#define A_CONF_SP       0xb948
#define A_CD_EVENTS     0xb9b8
#define A_EXC_GP        0xf450

#define A_A0_TRAPS      0x1010
#define A_B0_TRAPS      0x2010
#define A_C0_TRAPS      0x3010
#define A_B0_5B_TRAP    0x43d0

#define CARD_HARDLER_WRITE  0x51F4
#define CARD_HARDLER_WRITEM 0x51F5 // fake, for psxBios_write()
#define CARD_HARDLER_READ   0x5688
#define CARD_HARDLER_READM  0x5689 // fake, for psxBios_read()
#define CARD_HARDLER_INFO   0x5B64

#define HLEOP(n) SWAPu32((0x3bu << 26) | (n));

static u8 loadRam8(u32 addr)
{
	assert(!(addr & 0x5f800000));
	return psxM[addr & 0x1fffff];
}

static u32 loadRam32(u32 addr)
{
	assert(!(addr & 0x5f800000));
	return SWAP32(*((u32 *)psxM + ((addr & 0x1fffff) >> 2)));
}

static void *castRam8ptr(u32 addr)
{
	assert(!(addr & 0x5f800000));
	return psxM + (addr & 0x1fffff);
}

static void *castRam32ptr(u32 addr)
{
	assert(!(addr & 0x5f800003));
	return psxM + (addr & 0x1ffffc);
}

static void *loadRam8ptr(u32 addr)
{
	return castRam8ptr(loadRam32(addr));
}

static void *loadRam32ptr(u32 addr)
{
	return castRam32ptr(loadRam32(addr));
}

static void storeRam8(u32 addr, u8 d)
{
	assert(!(addr & 0x5f800000));
	*((u8 *)psxM + (addr & 0x1fffff)) = d;
}

static void storeRam32(u32 addr, u32 d)
{
	assert(!(addr & 0x5f800000));
	*((u32 *)psxM + ((addr & 0x1fffff) >> 2)) = SWAP32(d);
}

static void mips_return(u32 val)
{
	v0 = val;
	pc0 = ra;
}

static void mips_return_void(void)
{
	pc0 = ra;
}

static void use_cycles(u32 cycle)
{
	psxRegs.cycle += cycle * 2;
}

static void mips_return_c(u32 val, u32 cycle)
{
	use_cycles(cycle);
	mips_return(val);
}

static void mips_return_void_c(u32 cycle)
{
	use_cycles(cycle);
	pc0 = ra;
}

static int returned_from_exception(void)
{
	// 0x80000080 means it took another exception just after return
	return pc0 == k0 || pc0 == 0x80000080
#ifdef LIGHTREC
		// lightrec doesn't return at 0x80000080, so look
		// for the next block too
		|| pc0 == A_EXCEPTION
#endif
		;
}

int psxBiosSoftcallEnded(void)
{
	return pc0 == 0x80001000 || returned_from_exception();
}

// TODO: get rid of this softCall() thing as recursive cpu calls cause
// complications with dynarecs
static inline void softCall(u32 pc) {
	u32 sra = ra;
	u32 ssr = psxRegs.CP0.n.SR;
	u32 lim = 0;
	pc0 = pc;
	ra = 0x80001000;
	psxRegs.CP0.n.SR &= ~0x404; // disable interrupts

	assert(psxRegs.cpuInRecursion <= 1);
	psxRegs.cpuInRecursion++;
	psxCpu->Notify(R3000ACPU_NOTIFY_AFTER_LOAD, PTR_1);

	while (!psxBiosSoftcallEnded() && ++lim < 0x100000)
		psxCpu->ExecuteBlock(&psxRegs, EXEC_CALLER_HLE);

	psxCpu->Notify(R3000ACPU_NOTIFY_BEFORE_SAVE, PTR_1);
	psxRegs.cpuInRecursion--;

	if (pc0 != 0x80001000)
		log_unhandled("%s @%x did not return (@%x cnt=%d)\n",
			__func__, pc, pc0, lim);
	ra = sra;
	psxRegs.CP0.n.SR |= ssr & 0x404;
}

static inline void softCallInException(u32 pc) {
	u32 sra = ra;
	u32 lim = 0;
	pc0 = pc;

	assert(ra != 0x80001000);
	if (ra == 0x80001000)
		return;
	ra = 0x80001000;

	psxRegs.cpuInRecursion++;
	psxCpu->Notify(R3000ACPU_NOTIFY_AFTER_LOAD, PTR_1);

	while (!psxBiosSoftcallEnded() && ++lim < 0x100000)
		psxCpu->ExecuteBlock(&psxRegs, EXEC_CALLER_HLE);

	psxCpu->Notify(R3000ACPU_NOTIFY_BEFORE_SAVE, PTR_1);
	psxRegs.cpuInRecursion--;

	if (pc0 != 0x80001000 && !psxBiosSoftcallEnded())
		log_unhandled("%s @%x did not return (@%x cnt=%d)\n",
			__func__, pc, pc0, lim);
	if (pc0 == 0x80001000)
		ra = sra;
}

static u32  OpenEvent(u32 class, u32 spec, u32 mode, u32 func);
static void EnableEvent(u32 ev, int do_log);
static u32  DeliverEvent(u32 class, u32 spec);
static u32  UnDeliverEvent(u32 class, u32 spec);
static void CloseEvent(u32 ev);

static int card_buf_io(int is_write, int port, void *buf, u32 size)
{
	char *mcdptr = port ? Mcd2Data : Mcd1Data;
	FileDesc *desc = &FDesc[2 + port];
	u32 offset = 8192 * desc->mcfile + desc->offset;

	PSXBIOS_LOG("card_%s_buf %d,%d: ofs=%x(%x) sz=%x (%s)\n",
		is_write ? "write" : "read", port, desc->mcfile,
		desc->offset, offset, size, mcdptr + 128 * desc->mcfile + 0xa);
	if (!(loadRam8(A_CARD_STATUS1 + port) & 1)) {
		PSXBIOS_LOG(" ...busy %x\n", loadRam8(A_CARD_STATUS1 + port));
		return -1;
	}
	UnDeliverEvent(0xf4000001, 0x0004);
	UnDeliverEvent(0xf4000001, 0x8000);
	UnDeliverEvent(0xf4000001, 0x2000);
	UnDeliverEvent(0xf4000001, 0x0100);

	if (offset >= 128*1024u) {
		log_unhandled("card offs %x(%x)\n", desc->offset, offset);
		DeliverEvent(0xf4000001, 0x8000); // ?
		return -1;
	}
	if (offset + size >= 128*1024u) {
		log_unhandled("card offs+size %x+%x\n", offset, size);
		size = 128*1024 - offset;
	}
	if (is_write) {
		memcpy(mcdptr + offset, buf, size);
		if (port == 0)
			SaveMcd(Config.Mcd1, Mcd1Data, offset, size);
		else
			SaveMcd(Config.Mcd2, Mcd2Data, offset, size);
	}
	else {
		size_t ram_offset = (s8 *)buf - psxM;
		memcpy(buf, mcdptr + offset, size);
		if (ram_offset < 0x200000)
			psxCpu->Clear(ram_offset, (size + 3) / 4);
	}
	desc->offset += size;
	if (desc->mode & 0x8000) { // async
		storeRam8(A_CARD_STATUS1 + port, is_write ? 4 : 2); // busy
		storeRam32(A_CARD_HANDLER,
			is_write ? CARD_HARDLER_WRITEM : CARD_HARDLER_READM);
		card_io_delay = 2 + size / 1024; // hack
		return 0;
	}
	return size;
}

/*                                           *
//                                           *
//                                           *
//               System calls A0             */

/* Internally redirects to "FileRead(fd,tempbuf,1)".*/
/* For some strange reason, the returned character is sign-expanded; */
/* So if a return value of FFFFFFFFh could mean either character FFh, or error. */
static void psxBios_getc(void) // 0x03, 0x35
{
	s8 buf[1] = { -1 };
	int ret = -1;

	PSXBIOS_LOG("psxBios_%s %d\n", biosA0n[0x03], a0);

	if (a0 == 1)
		ret = -1;
	else if (a0 == 2 || a0 == 3) {
		card_buf_io(0, a0 - 2, buf, 1);
		ret = buf[0];
	}

	mips_return_c(ret, 100);
}

/* Copy of psxBios_write, except size is 1. */
static void psxBios_putc(void) // 0x09, 0x3B
{
	u8 buf[1] = { (u8)a0 };
	int ret = -1;

	if (a1 != 1) // not stdout
		PSXBIOS_LOG("psxBios_%s '%c' %d\n", biosA0n[0x09], (char)a0, a1);

	if (a1 == 1) { // stdout
		if (Config.PsxOut) printf("%c", (char)a0);
	}
	else if (a1 == 2 || a1 == 3) {
		ret = card_buf_io(1, a1 - 2, buf, 1);
	}

	mips_return_c(ret, 100);
}

static u32 do_todigit(u32 c)
{
	c &= 0xFF;
	if (c >= 0x30 && c < 0x3A) {
		c -= 0x30;
	}
	else if (c > 0x60 && c < 0x7B) {
		c -= 0x20;
	}
	else if (c > 0x40 && c < 0x5B) {
		c = c - 0x41 + 10;
	}
	else if (c >= 0x80) {
		log_unhandled("todigit %02x\n", c);
		c = -1;
	}
	else
	{
		c = 0x0098967F;
	}
	use_cycles(40);
	return c;
}

static void psxBios_todigit(void) // 0x0a
{
	mips_return(do_todigit(a0));
	PSXBIOS_LOG("psxBios_%s '%c' -> %u\n", biosA0n[0x0a], a0, v0);
}

static void do_strtol(char *p, void *end_, u32 base, int can_neg) {
	s32 n = 0, f = 0, t;
	u32 *end = end_;

	use_cycles(12);
	if (p == INVALID_PTR) {
		mips_return(0);
		return;
	}

	for (; (0x09 <= *p && *p <= '\r') || *p == ' '; p++)
		use_cycles(9);
	if (can_neg) {
		for (; *p == '-'; f = 1, p++)
			use_cycles(4);
	}
	if (base == 0 || base > 36)
		base = 10;
	if (*p == '0') {
		switch (*p++) {
		case 'b': case 'B': base = 2; break;
		case 'x': case 'X': base = 16; break;
		}
	}
	else if (*p == 'o' || *p == 'O') {
		base = 8;
		p++;
	}

	for (; (t = do_todigit(*p)) < base; p++) {
		n = n * base + t;
		use_cycles(12);
	}

	n = (f ? -n : n);
	if (end != INVALID_PTR)
		*end = SWAP32(a0 + (p - Ra0));
	mips_return_c(n, 100);
}

static void psxBios_strtoul() { // 0x0c
	do_strtol(a0 ? Ra0 : INVALID_PTR, a1 ? Ra1 : INVALID_PTR, a2, 0);
	PSXBIOS_LOG("psxBios_%s %s (%x), %x, %x -> 0x%x\n",
		biosA0n[0x0c], a0 ? Ra0 : NULL, a0, a1, a2, v0);
}

static void psxBios_strtol() { // 0x0d
	do_strtol(a0 ? Ra0 : INVALID_PTR, a1 ? Ra1 : INVALID_PTR, a2, 1);
	PSXBIOS_LOG("psxBios_%s %s (%x), %x, %x -> 0x%x\n",
		biosA0n[0x0d], a0 ? Ra0 : NULL, a0, a1, a2, v0);
}

void psxBios_abs() { // 0x0e
	if ((s32)a0 < 0) v0 = -(s32)a0;
	else v0 = a0;
	pc0 = ra;
}

void psxBios_labs() { // 0x0f
	psxBios_abs();
}

void psxBios_atoi() { // 0x10
	s32 n = 0, f = 0;
	char *p = (char *)Ra0;

	if (p == INVALID_PTR) {
		mips_return(0);
		return;
	}

	for (;;p++) {
		switch (*p) {
			case ' ': case '\t': continue;
			case '-': f++;
			case '+': p++;
		}
		break;
	}

	while (*p >= '0' && *p <= '9') {
		n = n * 10 + *p++ - '0';
	}

	v0 = (f ? -n : n);
	pc0 = ra;
	PSXBIOS_LOG("psxBios_%s %s (%x) -> 0x%x\n", biosA0n[0x10], Ra0, a0, v0);
}

void psxBios_atol() { // 0x11
	psxBios_atoi();
}

struct jmp_buf_ {
	u32 ra_, sp_, fp_;
	u32 s[8];
	u32 gp_;
};

static void psxBios_setjmp() { // 0x13
	struct jmp_buf_ *jmp_buf = castRam32ptr(a0);
	int i;

	PSXBIOS_LOG("psxBios_%s %x\n", biosA0n[0x13], a0);

	jmp_buf->ra_ = SWAP32(ra);
	jmp_buf->sp_ = SWAP32(sp);
	jmp_buf->fp_ = SWAP32(fp);
	for (i = 0; i < 8; i++) // s0-s7
		jmp_buf->s[i] = SWAP32(psxRegs.GPR.r[16 + i]);
	jmp_buf->gp_ = SWAP32(gp);

	mips_return_c(0, 15);
}

static void longjmp_load(const struct jmp_buf_ *jmp_buf)
{
	int i;

	ra = SWAP32(jmp_buf->ra_);
	sp = SWAP32(jmp_buf->sp_);
	fp = SWAP32(jmp_buf->fp_);
	for (i = 0; i < 8; i++) // s0-s7
		psxRegs.GPR.r[16 + i] = SWAP32(jmp_buf->s[i]);
	gp = SWAP32(jmp_buf->gp_);;
}

void psxBios_longjmp() { // 0x14
	struct jmp_buf_ *jmp_buf = castRam32ptr(a0);

	PSXBIOS_LOG("psxBios_%s\n", biosA0n[0x14]);
	longjmp_load(jmp_buf);
	mips_return_c(a1, 15);
}

void psxBios_strcat() { // 0x15
	u8 *p2 = (u8 *)Ra1;
	u32 p1 = a0;

	PSXBIOS_LOG("psxBios_%s %s (%x), %s (%x)\n", biosA0n[0x15], Ra0, a0, Ra1, a1);
	if (a0 == 0 || a1 == 0 || p2 == INVALID_PTR)
	{
		mips_return_c(0, 6);
		return;
	}
	while (loadRam8(p1)) {
		use_cycles(4);
		p1++;
	}
	for (; *p2; p1++, p2++)
		storeRam8(p1, *p2);
	storeRam8(p1, 0);

	mips_return_c(a0, 22);
}

void psxBios_strncat() { // 0x16
	char *p1 = (char *)Ra0, *p2 = (char *)Ra1;
	s32 n = a2;

#ifdef PSXBIOS_LOG
	PSXBIOS_LOG("psxBios_%s: %s (%x), %s (%x), %d\n", biosA0n[0x16], Ra0, a0, Ra1, a1, a2);
#endif
	if (a0 == 0 || a1 == 0)
	{
		v0 = 0;
		pc0 = ra;
		return;
	}
	while (*p1++);
	--p1;
	while ((*p1++ = *p2++) != '\0') {
		if (--n < 0) {
			*--p1 = '\0';
			break;
		}
	}

	v0 = a0; pc0 = ra;
}

void psxBios_strcmp() { // 0x17
	char *p1 = (char *)Ra0, *p2 = (char *)Ra1;
	s32 n=0;
	if (a0 == 0 && a1 == 0)
	{
		v0 = 0;
		pc0 = ra;
		return;
	}
	else if (a0 == 0 && a1 != 0)
	{
		v0 = -1;
		pc0 = ra;
		return;
	}
	else if (a0 != 0 && a1 == 0)
	{
		v0 = 1;
		pc0 = ra;
		return;
	}
#ifdef PSXBIOS_LOG
	PSXBIOS_LOG("psxBios_%s: %s (%x), %s (%x)\n", biosA0n[0x17], Ra0, a0, Ra1, a1);
#endif

	while (*p1 == *p2++) {
		n++;
		if (*p1++ == '\0') {
			v1=n-1;
			a0+=n;
			a1+=n;
			v0 = 0;
			pc0 = ra;
			return;
		}
	}

	v0 = (*p1 - *--p2);
	v1 = n;
	a0+=n;
	a1+=n;
	pc0 = ra;
}

void psxBios_strncmp() { // 0x18
	char *p1 = (char *)Ra0, *p2 = (char *)Ra1;
	s32 n = a2;
	if (a0 == 0 && a1 == 0)
	{
		v0 = 0;
		pc0 = ra;
		return;
	}
	else if (a0 == 0 && a1 != 0)
	{
		v0 = -1;
		pc0 = ra;
		return;
	}
	else if (a0 != 0 && a1 == 0)
	{
		v0 = 1;
		pc0 = ra;
		return;
	}
#ifdef PSXBIOS_LOG
	PSXBIOS_LOG("psxBios_%s: %s (%x), %s (%x), %d\n", biosA0n[0x18], Ra0, a0, Ra1, a1, a2);
#endif

	while (--n >= 0 && *p1 == *p2++) {
		if (*p1++ == '\0') {
			v0 = 0;
			pc0 = ra;
			v1 = a2 - ((a2-n) - 1);
			a0 += (a2-n) - 1;
			a1 += (a2-n) - 1;
			a2 = n;
			return;
		}
	}

	v0 = (n < 0 ? 0 : *p1 - *--p2);
	pc0 = ra;
	v1 = a2 - ((a2-n) - 1);
	a0 += (a2-n) - 1;
	a1 += (a2-n) - 1;
	a2 = n;
}

void psxBios_strcpy() { // 0x19
	char *p1 = (char *)Ra0, *p2 = (char *)Ra1;
	PSXBIOS_LOG("psxBios_%s %x, %s (%x)\n", biosA0n[0x19], a0, p2, a1);
	if (a0 == 0 || a1 == 0)
	{
		v0 = 0;
		pc0 = ra;
		return;
	}
	while ((*p1++ = *p2++) != '\0');

	v0 = a0; pc0 = ra;
}

void psxBios_strncpy() { // 0x1a
	char *p1 = (char *)Ra0, *p2 = (char *)Ra1;
	s32 n = a2, i;
	if (a0 == 0 || a1 == 0)
	{
		v0 = 0;
		pc0 = ra;
		return;
	}
	for (i = 0; i < n; i++) {
		if ((*p1++ = *p2++) == '\0') {
			while (++i < n) {
				*p1++ = '\0';
			}
			v0 = a0; pc0 = ra;
			return;
		}
	}

	v0 = a0; pc0 = ra;
}

void psxBios_strlen() { // 0x1b
	char *p = (char *)Ra0;
	v0 = 0;
	if (a0 == 0)
	{
		pc0 = ra;
		return;
	}
	while (*p++) v0++;
	pc0 = ra;
}

void psxBios_index() { // 0x1c
	char *p = (char *)Ra0;
	if (a0 == 0)
	{
		v0 = 0;
		pc0 = ra;
		return;
	}

	do {
		if (*p == a1) {
			v0 = a0 + (p - (char *)Ra0);
			pc0 = ra;
			return;
		}
	} while (*p++ != '\0');

	v0 = 0; pc0 = ra;
}

void psxBios_rindex() { // 0x1d
	char *p = (char *)Ra0;

	v0 = 0;
	if (a0 == 0)
	{
		pc0 = ra;
		return;
	}
	do {
		if (*p == a1)
			v0 = a0 + (p - (char *)Ra0);
	} while (*p++ != '\0');

	pc0 = ra;
}

void psxBios_strchr() { // 0x1e
	psxBios_index();
}

void psxBios_strrchr() { // 0x1f
	psxBios_rindex();
}

void psxBios_strpbrk() { // 0x20
	char *p1 = (char *)Ra0, *p2 = (char *)Ra1, *scanp, c, sc;

	while ((c = *p1++) != '\0') {
		for (scanp = p2; (sc = *scanp++) != '\0';) {
			if (sc == c) {
				v0 = a0 + (p1 - 1 - (char *)Ra0);
				pc0 = ra;
				return;
			}
		}
	}

	// BUG: return a0 instead of NULL if not found
	v0 = a0; pc0 = ra;
}

void psxBios_strspn() { // 0x21
	char *p1, *p2;

	for (p1 = (char *)Ra0; *p1 != '\0'; p1++) {
		for (p2 = (char *)Ra1; *p2 != '\0' && *p2 != *p1; p2++);
		if (*p2 == '\0') break;
	}

	v0 = p1 - (char *)Ra0; pc0 = ra;
}

void psxBios_strcspn() { // 0x22
	char *p1, *p2;

	for (p1 = (char *)Ra0; *p1 != '\0'; p1++) {
		for (p2 = (char *)Ra1; *p2 != '\0' && *p2 != *p1; p2++);
		if (*p2 != '\0') break;
	}

	v0 = p1 - (char *)Ra0; pc0 = ra;
}

void psxBios_strtok() { // 0x23
	char *pcA0 = (char *)Ra0;
	char *pcRet = strtok(pcA0, (char *)Ra1);
	if (pcRet)
		v0 = a0 + pcRet - pcA0;
	else
		v0 = 0;
	pc0 = ra;
}

void psxBios_strstr() { // 0x24
	char *p = (char *)Ra0, *p1, *p2;
	PSXBIOS_LOG("psxBios_%s %s (%x), %s (%x)\n", biosA0n[0x24], p, a0, Ra1, a1);

	while (*p != '\0') {
		p1 = p;
		p2 = (char *)Ra1;

		while (*p1 != '\0' && *p2 != '\0' && *p1 == *p2) {
			p1++; p2++;
		}

		if (*p2 == '\0') {
			v0 = a0 + (p - (char *)Ra0);
			pc0 = ra;
			PSXBIOS_LOG(" -> %x\n", v0);
			return;
		}

		// bug: skips the whole matched substring + 1
		p = p1 + 1;
	}

	v0 = 0; pc0 = ra;
}

void psxBios_toupper() { // 0x25
	v0 = (s8)(a0 & 0xff);
	if (v0 >= 'a' && v0 <= 'z') v0 -= 'a' - 'A';
	pc0 = ra;
}

void psxBios_tolower() { // 0x26
	v0 = (s8)(a0 & 0xff);
	if (v0 >= 'A' && v0 <= 'Z') v0 += 'a' - 'A';
	pc0 = ra;
}

static void do_memset(u32 dst, u32 v, s32 len)
{
	u32 d = dst;
	s32 l = len;
	while (l-- > 0) {
		u8 *db = PSXM(d);
		if (db != INVALID_PTR)
			*db = v;
		d++;
	}
	psxCpu->Clear(dst, (len + 3) / 4);
}

static void do_memcpy(u32 dst, u32 src, s32 len)
{
	u32 d = dst, s = src;
	s32 l = len;
	while (l-- > 0) {
		const u8 *sb = PSXM(s);
		u8 *db = PSXM(d);
		if (db != INVALID_PTR && sb != INVALID_PTR)
			*db = *sb;
		d++;
		s++;
	}
	psxCpu->Clear(dst, (len + 3) / 4);
}

static void psxBios_memcpy();

static void psxBios_bcopy() { // 0x27 - memcpy with args swapped
	//PSXBIOS_LOG("psxBios_%s %x %x %x\n", biosA0n[0x27], a0, a1, a2);
	u32 ret = a0, cycles = 0;
	if (a0 == 0) // ...but it checks src this time
	{
		mips_return_c(0, 4);
		return;
	}
	v1 = a0;
	if ((s32)a2 > 0) {
		do_memcpy(a1, a0, a2);
		cycles = a2 * 6;
		a0 += a2;
		a1 += a2;
		a2 = 0;
	}
	mips_return_c(ret, cycles + 5);
}

static void psxBios_bzero() { // 0x28
	/* Same as memset here (See memset below) */
	u32 ret = a0, cycles;
	if (a0 == 0 || (s32)a1 <= 0)
	{
		mips_return_c(0, 6);
		return;
	}
	do_memset(a0, 0, a1);
	cycles = a1 * 4;
	a0 += a1;
	a1 = 0;
	// todo: many more cycles due to uncached bios mem
	mips_return_c(ret, cycles + 5);
}

void psxBios_bcmp() { // 0x29
	char *p1 = (char *)Ra0, *p2 = (char *)Ra1;

	if (a0 == 0 || a1 == 0) { v0 = 0; pc0 = ra; return; }

	while ((s32)a2-- > 0) {
		if (*p1++ != *p2++) {
			v0 = *p1 - *p2; // BUG: compare the NEXT byte
			pc0 = ra;
			return;
		}
	}

	v0 = 0; pc0 = ra;
}

static void psxBios_memcpy() { // 0x2a
	u32 ret = a0, cycles = 0;
	if (a0 == 0)
	{
		mips_return_c(0, 4);
		return;
	}
	v1 = a0;
	if ((s32)a2 > 0) {
		do_memcpy(a0, a1, a2);
		cycles = a2 * 6;
		a0 += a2;
		a1 += a2;
		a2 = 0;
	}
	mips_return_c(ret, cycles + 5);
}

static void psxBios_memset() { // 0x2b
	u32 ret = a0, cycles;
	if (a0 == 0 || (s32)a2 <= 0)
	{
		mips_return_c(0, 6);
		return;
	}
	do_memset(a0, a1, a2);
	cycles = a2 * 4;
	a0 += a2;
	a2 = 0;
	// todo: many more cycles due to uncached bios mem
	mips_return_c(ret, cycles + 5);
}

void psxBios_memmove() { // 0x2c
	u32 ret = a0, cycles = 0;
	if (a0 == 0)
	{
		mips_return_c(0, 4);
		return;
	}
	v1 = a0;
	if ((s32)a2 > 0 && a0 > a1 && a0 < a1 + a2) {
		u32 dst = a0, len = a2 + 1;
		a0 += a2;
		a1 += a2;
		while ((s32)a2 >= 0) { // BUG: copies one more byte here
			const u8 *sb = PSXM(a1);
			u8 *db = PSXM(a0);
			if (db != INVALID_PTR && sb != INVALID_PTR)
				*db = *sb;
			a0--;
			a1--;
			a2--;
		}
		psxCpu->Clear(dst, (len + 3) / 4);
		cycles = 10 + len * 8;
	} else if ((s32)a2 > 0) {
		do_memcpy(a0, a1, a2);
		cycles = a2 * 6;
		a0 += a2;
		a1 += a2;
		a2 = 0;
	}
	mips_return_c(ret, cycles + 5);
}

void psxBios_memcmp() { // 0x2d
	psxBios_bcmp();
}

void psxBios_memchr() { // 0x2e
	char *p = (char *)Ra0;

	if (a0 == 0 || a2 > 0x7FFFFFFF)
	{
		pc0 = ra;
		return;
	}

	while ((s32)a2-- > 0) {
		if (*p++ != (s8)a1) continue;
		v0 = a0 + (p - (char *)Ra0 - 1);
		pc0 = ra;
		return;
	}

	v0 = 0; pc0 = ra;
}

static void psxBios_rand() { // 0x2f
	u32 s = loadRam32(A_RND_SEED) * 1103515245 + 12345;
	storeRam32(A_RND_SEED, s);
	v1 = s;
	mips_return_c((s >> 16) & 0x7fff, 12+37);
}

static void psxBios_srand() { // 0x30
	storeRam32(A_RND_SEED, a0);
	mips_return_void_c(3);
}

static u32 qscmpfunc, qswidth;

static inline int qscmp(char *a, char *b) {
	u32 sa0 = a0;

	a0 = sa0 + (a - (char *)PSXM(sa0));
	a1 = sa0 + (b - (char *)PSXM(sa0));

	softCall(qscmpfunc);

	a0 = sa0;
	return (s32)v0;
}

static inline void qexchange(char *i, char *j) {
	char t;
	int n = qswidth;

	do {
		t = *i;
		*i++ = *j;
		*j++ = t;
	} while (--n);
}

static inline void q3exchange(char *i, char *j, char *k) {
	char t;
	int n = qswidth;

	do {
		t = *i;
		*i++ = *k;
		*k++ = *j;
		*j++ = t;
	} while (--n);
}

static void qsort_main(char *a, char *l) {
	char *i, *j, *lp, *hp;
	int c;
	unsigned int n;

start:
	if ((n = l - a) <= qswidth)
		return;
	n = qswidth * (n / (2 * qswidth));
	hp = lp = a + n;
	i = a;
	j = l - qswidth;
	while (TRUE) {
		if (i < lp) {
			if ((c = qscmp(i, lp)) == 0) {
				qexchange(i, lp -= qswidth);
				continue;
			}
			if (c < 0) {
				i += qswidth;
				continue;
			}
		}

loop:
		if (j > hp) {
			if ((c = qscmp(hp, j)) == 0) {
				qexchange(hp += qswidth, j);
				goto loop;
			}
			if (c > 0) {
				if (i == lp) {
					q3exchange(i, hp += qswidth, j);
					i = lp += qswidth;
					goto loop;
				}
				qexchange(i, j);
				j -= qswidth;
				i += qswidth;
				continue;
			}
			j -= qswidth;
			goto loop;
		}

		if (i == lp) {
			if (lp - a >= l - hp) {
				qsort_main(hp + qswidth, l);
				l = lp;
			} else {
				qsort_main(a, lp);
				a = hp + qswidth;
			}
			goto start;
		}

		q3exchange(j, lp -= qswidth, i);
		j = hp -= qswidth;
	}
}

void psxBios_qsort() { // 0x31
	qswidth = a2;
	qscmpfunc = a3;
	qsort_main((char *)Ra0, (char *)Ra0 + a1 * a2);

	pc0 = ra;
}

static int malloc_heap_grow(u32 size) {
	u32 heap_addr, heap_end, heap_addr_new;

	heap_addr = loadRam32(A_HEAP_BASE);
	heap_end = loadRam32(A_HEAP_END);
	heap_addr_new = heap_addr + size + 4;
	if (heap_addr_new >= heap_end)
		return -1;
	storeRam32(A_HEAP_BASE, heap_addr_new);
	storeRam32(heap_addr - 4, size | 1);
	storeRam32(heap_addr + size, ~1); // terminator
	return 0;
}

static void psxBios_malloc() { // 0x33
	u32 size = (a0 + 3) & ~3;
	u32 limit = 32*1024;
	u32 tries = 2, i;
	u32 ret;

	PSXBIOS_LOG("psxBios_%s %d\n", biosA0n[0x33], a0);

	if (!loadRam32(A_HEAP_INIT_FLG)) {
		u32 heap_addr = loadRam32(A_HEAP_BASE);
		storeRam32(heap_addr, ~1);
		storeRam32(A_HEAP_FRSTCHNK, heap_addr);
		storeRam32(A_HEAP_CURCHNK, heap_addr);
		storeRam32(A_HEAP_BASE, heap_addr + 4);
		if (malloc_heap_grow(size)) {
			PSXBIOS_LOG("malloc: init OOM\n");
			mips_return_c(0, 20);
			return;
		}
		storeRam32(A_HEAP_INIT_FLG, 1);
	}

	for (i = 0; tries > 0 && i < limit; i++)
	{
		u32 chunk = loadRam32(A_HEAP_CURCHNK);
		u32 chunk_hdr = loadRam32(chunk);
		u32 next_chunk = chunk + 4 + (chunk_hdr & ~3);
		u32 next_chunk_hdr = loadRam32(next_chunk);
		use_cycles(20);
		//printf(" c %08x %08x\n", chunk, chunk_hdr);
		if (chunk_hdr & 1) {
			// free chunk
			if (chunk_hdr > (size | 1)) {
				// split
				u32 p2size = (chunk_hdr & ~3) - size - 4;
				storeRam32(chunk + 4 + size, p2size | 1);
				chunk_hdr = size | 1;
			}
			if (chunk_hdr == (size | 1)) {
				storeRam32(chunk, size);
				break;
			}
			// chunk too small
			if (next_chunk_hdr == ~1) {
				// rm useless last free block
				storeRam32(A_HEAP_BASE, chunk + 4);
				storeRam32(chunk, ~1);
				continue;
			}
			if (next_chunk_hdr & 1) {
				// merge
				u32 msize = (chunk_hdr & ~3) + 4 + (next_chunk_hdr & ~3);
				storeRam32(chunk, msize | 1);
				continue;
			}
		}
		if (chunk_hdr == ~1) {
			// last chunk
			if (tries == 2)
				storeRam32(A_HEAP_CURCHNK, loadRam32(A_HEAP_FRSTCHNK));
			tries--;
		}
		else {
			// go to the next chunk
			storeRam32(A_HEAP_CURCHNK, next_chunk);
		}
	}

	if (i == limit) {
		PSXBIOS_LOG("malloc: limit OOM\n");
		ret = 0;
	}
	else if (tries == 0 && malloc_heap_grow(size)) {
		PSXBIOS_LOG("malloc: grow OOM s=%d end=%08x/%08x\n",
			size, loadRam32(A_HEAP_BASE), loadRam32(A_HEAP_END));
		ret = 0;
	}
	else {
		u32 chunk = loadRam32(A_HEAP_CURCHNK);
		storeRam32(chunk, loadRam32(chunk) & ~3);
		ret = chunk + 4;
	}

	PSXBIOS_LOG(" -> %08x\n", ret);
	mips_return_c(ret, 40);
}

static void psxBios_free() { // 0x34
	PSXBIOS_LOG("psxBios_%s %x (%d bytes)\n", biosA0n[0x34], a0, loadRam32(a0 - 4));
	storeRam32(a0 - 4, loadRam32(a0 - 4) | 1); // set chunk to free
	mips_return_void_c(5);
}

static void psxBios_calloc() { // 0x37
	u32 ret, size;
	PSXBIOS_LOG("psxBios_%s %x %x\n", biosA0n[0x37], a0, a1);

	a0 = size = a0 * a1;
	psxBios_malloc();
	ret = v0;
	if (ret) {
		a0 = ret; a1 = size;
		psxBios_bzero();
	}
	mips_return_c(ret, 21);
}

void psxBios_realloc() { // 0x38
	u32 block = a0;
	u32 size = a1;

	PSXBIOS_LOG("psxBios_%s %08x %d\n", biosA0n[0x38], a0, a1);

	a0 = block;
	/* If "old_buf" is zero, executes malloc(new_size), and returns r2=new_buf (or 0=failed). */
	if (block == 0)
	{
		psxBios_malloc();
	}
	/* Else, if "new_size" is zero, executes free(old_buf), and returns r2=garbage. */
	else if (size == 0)
	{
		psxBios_free();
	}
	/* Else, executes malloc(new_size), bcopy(old_buf,new_buf,new_size), and free(old_buf), and returns r2=new_buf (or 0=failed). */
	/* Note that it is not quite implemented this way here. */
	else
	{
		psxBios_free();
		a0 = size;
		psxBios_malloc();
	}
}


/* InitHeap(void *block , int n) */
static void psxBios_InitHeap() { // 0x39
	PSXBIOS_LOG("psxBios_%s %x %x\n", biosA0n[0x39], a0, a1);

	storeRam32(A_HEAP_BASE, a0);
	storeRam32(A_HEAP_SIZE, a1);
	storeRam32(A_HEAP_END, a0 + (a1 & ~3) + 4);
	storeRam32(A_HEAP_INIT_FLG, 0);
	storeRam32(a0, 0);

	mips_return_void_c(14);
}

void psxBios_getchar() { //0x3b
	v0 = getchar(); pc0 = ra;
}

static void psxBios_printf_psxout() { // 0x3f
	char tmp[1024];
	char tmp2[1024];
	u32 save[4] = { 0, };
	char *ptmp = tmp;
	int n=1, i=0, j;
	void *psp;

	psp = PSXM(sp);
	if (psp != INVALID_PTR) {
		memcpy(save, psp, 4 * 4);
		psxMu32ref(sp) = SWAP32((u32)a0);
		psxMu32ref(sp + 4) = SWAP32((u32)a1);
		psxMu32ref(sp + 8) = SWAP32((u32)a2);
		psxMu32ref(sp + 12) = SWAP32((u32)a3);
	}

	while (Ra0[i]) {
		switch (Ra0[i]) {
			case '%':
				j = 0;
				tmp2[j++] = '%';
_start:
				switch (Ra0[++i]) {
					case '.':
					case 'l':
						tmp2[j++] = Ra0[i]; goto _start;
					default:
						if (Ra0[i] >= '0' && Ra0[i] <= '9') {
							tmp2[j++] = Ra0[i];
							goto _start;
						}
						break;
				}
				tmp2[j++] = Ra0[i];
				tmp2[j] = 0;

				switch (Ra0[i]) {
					case 'f': case 'F':
						ptmp += sprintf(ptmp, tmp2, (float)psxMu32(sp + n * 4)); n++; break;
					case 'a': case 'A':
					case 'e': case 'E':
					case 'g': case 'G':
						ptmp += sprintf(ptmp, tmp2, (double)psxMu32(sp + n * 4)); n++; break;
					case 'p':
					case 'i': case 'u':
					case 'd': case 'D':
					case 'o': case 'O':
					case 'x': case 'X':
						ptmp += sprintf(ptmp, tmp2, (unsigned int)psxMu32(sp + n * 4)); n++; break;
					case 'c':
						ptmp += sprintf(ptmp, tmp2, (unsigned char)psxMu32(sp + n * 4)); n++; break;
					case 's':
						ptmp += sprintf(ptmp, tmp2, (char*)PSXM(psxMu32(sp + n * 4))); n++; break;
					case '%':
						*ptmp++ = Ra0[i]; break;
				}
				i++;
				break;
			default:
				*ptmp++ = Ra0[i++];
		}
	}
	*ptmp = 0;

	if (psp != INVALID_PTR)
		memcpy(psp, save, 4 * 4);

	if (Config.PsxOut)
		SysPrintf("%s", tmp);
}

void psxBios_printf() { // 0x3f
	psxBios_printf_psxout();
	pc0 = ra;
}

static void psxBios_cd() { // 0x40
	const char *p, *dir = Ra0;
	PSXBIOS_LOG("psxBios_%s %x(%s)\n", biosB0n[0x40], a0, dir);
	if (dir != INVALID_PTR) {
		if ((p = strchr(dir, ':')))
			dir = ++p;
		if (*dir == '\\')
			dir++;
		snprintf(cdir, sizeof(cdir), "%s", dir);
	}
	mips_return_c(1, 100);
}

static void psxBios_format() { // 0x41
	PSXBIOS_LOG("psxBios_%s %x(%s)\n", biosB0n[0x41], a0, Ra0);
	if (strcmp(Ra0, "bu00:") == 0 && Config.Mcd1[0] != '\0')
	{
		CreateMcd(Config.Mcd1);
		LoadMcd(1, Config.Mcd1);
		v0 = 1;
	}
	else if (strcmp(Ra0, "bu10:") == 0 && Config.Mcd2[0] != '\0')
	{
		CreateMcd(Config.Mcd2);
		LoadMcd(2, Config.Mcd2);
		v0 = 1;
	}
	else
	{
		v0 = 0;
	}
	pc0 = ra;
}

static void psxBios_SystemErrorUnresolvedException() {
	if (floodchk != 0x12340a40) { // prevent log flood
		SysPrintf("psxBios_%s called from %08x\n", biosA0n[0x40], ra);
		floodchk = 0x12340a40;
	}
	mips_return_void_c(1000);
}

static void FlushCache() {
	psxCpu->Notify(R3000ACPU_NOTIFY_CACHE_ISOLATED, NULL);
	psxCpu->Notify(R3000ACPU_NOTIFY_CACHE_UNISOLATED, NULL);
	k0 = 0xbfc0193c;
	// runs from uncached mem so tons of cycles
	use_cycles(500);
}

// you likely want to mask irqs before calling these
static u8 cdrom_sync(int do_ack)
{
	u8 r = 0;
	if (psxRegs.interrupt & (1u << PSXINT_CDR)) {
		if ((s32)(psxRegs.cycle - psxRegs.event_cycles[PSXINT_CDR]) < 0)
			psxRegs.cycle = psxRegs.event_cycles[PSXINT_CDR] + 1;
		irq_test(&psxRegs.CP0);
	}
	if (do_ack) {
		cdrWrite0(1);
		r = cdrRead3() & 0x1f;
		cdrWrite3(0x5f); // ack; clear params
	}
	return r;
}

static void cdrom_cmd_and_wait(u8 cmd, int arg_cnt, int resp_cnt, ...)
{
	va_list ap;

	cdrom_sync(0);
	cdrWrite0(0);
	va_start(ap, resp_cnt);
	while (arg_cnt-- > 0)
		cdrWrite2(va_arg(ap, u32));
	va_end(ap);
	cdrWrite1(cmd);

	if (resp_cnt > 0) {
		u8 r = cdrom_sync(1);
		assert(r == 3); (void)r;
		cdrRead1();
	}
	if (resp_cnt > 1) {
		u8 r = cdrom_sync(1);
		assert(r == 2); (void)r;
		cdrRead1();
	}
}

/*
 *	long Load(char *name, struct EXEC *header);
 */

void psxBios_Load() { // 0x42
	u8 time[3] = { 2, 0, 0x16 };
	EXE_HEADER eheader;
	char path[256];
	char *pa0, *p;
	void *pa1;

	pa0 = Ra0;
	pa1 = Ra1;
	PSXBIOS_LOG("psxBios_%s %x(%s), %x\n", biosA0n[0x42], a0, pa0, a1);
	if (pa0 == INVALID_PTR || pa1 == INVALID_PTR) {
		mips_return(0);
		return;
	}
	if ((p = strchr(pa0, ':')))
		pa0 = ++p;
	if (*pa0 == '\\')
		pa0++;
	if (cdir[0])
		snprintf(path, sizeof(path), "%s\\%s", cdir, (char *)pa0);
	else
		snprintf(path, sizeof(path), "%s", (char *)pa0);

	if (LoadCdromFile(path, &eheader, time) == 0) {
		memcpy(pa1, ((char*)&eheader)+16, sizeof(EXEC));
		psxCpu->Clear(a1, sizeof(EXEC) / 4);
		FlushCache();
		v0 = 1;
	} else v0 = 0;
	PSXBIOS_LOG(" -> %d\n", v0);

	pc0 = ra;

	// set the cdrom to a state of just after exe read
	psxRegs.CP0.n.SR &= ~0x404;
	cdrom_sync(1);
	cdrWrite0(1);
	cdrWrite2(0x1f); // unmask
	cdrom_cmd_and_wait(0x0e, 1, 1, 0x80u); // CdlSetmode
	cdrom_cmd_and_wait(0x02, 3, 1, time[0], time[1], time[2]); // CdlSetloc
	cdrom_cmd_and_wait(0x15, 0, 2); // CdlSeekL
	psxHwWrite16(0x1f801070, ~4);
	MTC0(&psxRegs, 12, psxRegs.CP0.n.SR | 0x404);
}

/*
 *	int Exec(struct EXEC *header , int argc , char **argv);
 */

void psxBios_Exec() { // 43
	EXEC *header = (EXEC *)castRam32ptr(a0);
	u32 ptr;
	s32 len;

	PSXBIOS_LOG("psxBios_%s: %x, %x, %x\n", biosA0n[0x43], a0, a1, a2);

	header->_sp = SWAP32(sp);
	header->_fp = SWAP32(fp);
	header->_sp = SWAP32(sp);
	header->_gp = SWAP32(gp);
	header->ret = SWAP32(ra);
	header->base = SWAP32(s0);

	ptr = SWAP32(header->b_addr);
	len = SWAP32(header->b_size);
	if (len != 0) do {
		storeRam32(ptr, 0);
		len -= 4; ptr += 4;
	} while (len > 0);

	if (header->S_addr != 0)
		sp = fp = SWAP32(header->S_addr) + SWAP32(header->s_size);

	gp = SWAP32(header->gp0);

	s0 = a0;

	a0 = a1;
	a1 = a2;

	ra = 0x8000;
	pc0 = SWAP32(header->_pc0);
}

static void psxBios_FlushCache() { // 44
	PSXBIOS_LOG("psxBios_%s\n", biosA0n[0x44]);
	FlushCache();
	mips_return_void();
}

void psxBios_GPU_dw() { // 0x46
	int size;
	u32 *ptr;

#ifdef PSXBIOS_LOG
	PSXBIOS_LOG("psxBios_%s\n", biosA0n[0x46]);
#endif

	GPU_writeData(0xa0000000);
	GPU_writeData((a1<<0x10)|(a0&0xffff));
	GPU_writeData((a3<<0x10)|(a2&0xffff));
	size = (a2*a3)/2;
	ptr = (u32*)PSXM(Rsp[4]);  //that is correct?
	while(size--)
	{
		GPU_writeData(SWAPu32(*ptr++));
	} 

	pc0 = ra;
}

static void gpu_sync() {
	// not implemented...
	// might be problematic to do because of Config.GpuListWalking
	if (psxRegs.interrupt & (1u << PSXINT_GPUDMA))
		log_unhandled("gpu_sync with active dma\n");
	mips_return_c(0, 21);
}

void psxBios_mem2vram() { // 0x47
	int size;
	gpuSyncPluginSR(); // flush
	GPU_writeData(0xa0000000);
	GPU_writeData((a1<<0x10)|(a0&0xffff));
	GPU_writeData((a3<<0x10)|(a2&0xffff));
	size = ((((a2 * a3) / 2) >> 4) << 16);
	GPU_writeStatus(0x04000002);
	psxHwWrite32(0x1f8010f4,0);
	psxHwWrite32(0x1f8010f0,psxHwRead32(0x1f8010f0)|0x800);
	psxHwWrite32(0x1f8010a0,Rsp[4]);//might have a buggy...
	psxHwWrite32(0x1f8010a4, size | 0x10);
	psxHwWrite32(0x1f8010a8,0x01000201);

	pc0 = ra;
}

void psxBios_SendGPU() { // 0x48
	GPU_writeStatus(a0);
	gpuSyncPluginSR();
	pc0 = ra;
}

void psxBios_GPU_cw() { // 0x49
	GPU_writeData(a0);
	gpuSyncPluginSR();
	use_cycles(13);
	gpu_sync();
}

void psxBios_GPU_cwb() { // 0x4a
	u32 *ptr = (u32*)Ra0;
	int size = a1;
	gpuSyncPluginSR();
	while(size--)
	{
		GPU_writeData(SWAPu32(*ptr++));
	}

	pc0 = ra;
}
   
void psxBios_GPU_SendPackets() { //4b:	
	gpuSyncPluginSR();
	GPU_writeStatus(0x04000002);
	psxHwWrite32(0x1f8010f4,0);
	psxHwWrite32(0x1f8010f0,psxHwRead32(0x1f8010f0)|0x800);
	psxHwWrite32(0x1f8010a0,a0);
	psxHwWrite32(0x1f8010a4,0);
	psxHwWrite32(0x1f8010a8,0x010000401);
	pc0 = ra;
}

void psxBios_sys_a0_4c() { // 0x4c GPU relate
	psxHwWrite32(0x1f8010a8,0x00000401);
	GPU_writeData(0x0400000);
	GPU_writeData(0x0200000);
	GPU_writeData(0x0100000);
	v0 = 0x1f801814;
	pc0 = ra;
}

void psxBios_GPU_GetGPUStatus() { // 0x4d
	v0 = GPU_readStatus();
	pc0 = ra;
}

#undef s_addr

void psxBios_LoadExec() { // 51
	EXEC *header = (EXEC*)PSXM(0xf000);
	u32 s_addr, s_size;

#ifdef PSXBIOS_LOG
	PSXBIOS_LOG("psxBios_%s: %s: %x,%x\n", biosA0n[0x51], Ra0, a1, a2);
#endif
	s_addr = a1; s_size = a2;

	a1 = 0xf000;
	psxBios_Load();

	header->S_addr = s_addr;
	header->s_size = s_size;

	a0 = 0xf000; a1 = 0; a2 = 0;
	psxBios_Exec();
}

static void psxBios__bu_init() { // 70
	PSXBIOS_LOG("psxBios_%s\n", biosA0n[0x70]);

	DeliverEvent(0xf0000011, 0x0004);
	DeliverEvent(0xf4000001, 0x0004);

	pc0 = ra;
}

static void write_chain(u32 *d, u32 next, u32 handler1, u32 handler2);
static void psxBios_SysEnqIntRP_(u32 priority, u32 chain_eptr);
static void psxBios_SysDeqIntRP_(u32 priority, u32 chain_rm_eptr);

static void psxBios_EnqueueCdIntr_(void)
{
	u32 *ram32 = (u32 *)psxM;

	// traps should already be installed by write_chain()
	ram32[0x91d0/4] = 0;
	ram32[0x91d4/4] = SWAP32(0xbfc0506c);
	ram32[0x91d8/4] = SWAP32(0xbfc04dec);
	psxBios_SysEnqIntRP_(0, 0x91d0);
	ram32[0x91e0/4] = 0;
	ram32[0x91e4/4] = SWAP32(0xbfc050a4);
	ram32[0x91e8/4] = SWAP32(0xbfc04fbc);
	psxBios_SysEnqIntRP_(0, 0x91e0);
	use_cycles(31);
}

static void setup_cd_irq_and_events(void)
{
	u16 specs[] = { 0x10, 0x20, 0x40, 0x80, 0x8000 };
	size_t i;

	psxBios_EnqueueCdIntr_();

	for (i = 0; i < sizeof(specs) / sizeof(specs[0]); i++) {
		u32 h = OpenEvent(0xf0000003, specs[i], EvMdMARK, 0);
		// no error checks
		storeRam32(A_CD_EVENTS + i * 4, h);
		EnableEvent(h, 0);
	}
}

static void psxBios_CdReset_() {
	psxRegs.CP0.n.SR &= ~0x404; // disable interrupts

	cdrom_sync(1);
	cdrWrite0(1);
	cdrWrite2(0x1f); // unmask
	cdrom_cmd_and_wait(0x0a, 0, 2); // CdlReset
	cdrom_cmd_and_wait(0x0e, 1, 1, 0x80u); // CdlSetmode

	// todo(?): should read something (iso root directory?)
	// from { 0, 2, 16 } to somewhere and pause

	mips_return(1);
	psxHwWrite16(0x1f801070, ~4);
	MTC0(&psxRegs, 12, psxRegs.CP0.n.SR | 0x404);
	DeliverEvent(0xf0000003, 0x0020);
}

static void psxBios_CdInit() { // 54, 71
	PSXBIOS_LOG("psxBios_%s\n", biosA0n[0x71]);
	setup_cd_irq_and_events();

	psxBios_CdReset_();

	// this function takes pretty much forever
	mips_return_c(0, 50000*11);
}

static void psxBios_DequeueCdIntr_() {
	psxBios_SysDeqIntRP_(0, 0x91d0);
	psxBios_SysDeqIntRP_(0, 0x91e0);
	use_cycles(16);
}

static void psxBios_CdReset() { // 95
	PSXBIOS_LOG("psxBios_%s\n", biosA0n[0x95]);
	psxBios_CdReset_();
}

static void psxBios_EnqueueCdIntr() { // a2
	PSXBIOS_LOG("psxBios_%s\n", biosA0n[0xa2]);
	psxBios_EnqueueCdIntr_();
	// return value comes from SysEnqIntRP() insternal call
}

static void psxBios_DequeueCdIntr() { // a3
	PSXBIOS_LOG("psxBios_%s\n", biosA0n[0xa3]);
	psxBios_DequeueCdIntr_();
}

static void psxBios_CdRemove() { // 56, 72
	PSXBIOS_LOG("psxBios_%s\n", biosA0n[0x72]);

	CloseEvent(loadRam32(A_CD_EVENTS + 0x00));
	CloseEvent(loadRam32(A_CD_EVENTS + 0x04));
	CloseEvent(loadRam32(A_CD_EVENTS + 0x08));
	CloseEvent(loadRam32(A_CD_EVENTS + 0x0c));
	CloseEvent(loadRam32(A_CD_EVENTS + 0x10));
	psxBios_DequeueCdIntr_();

	// EnterCriticalSection - should be done at the beginning,
	// but this way is much easier to implement
	a0 = 1;
	pc0 = A_SYSCALL;
	use_cycles(30);
}

static void setup_tt(u32 tcb_cnt, u32 evcb_cnt, u32 stack);

static void psxBios_SetConf() { // 9c
	PSXBIOS_LOG("psxBios_%s %x %x %x\n", biosA0n[0x9c], a0, a1, a2);
	setup_tt(a1, a0, a2);
	psxRegs.CP0.n.SR |= 0x401;
	mips_return_void_c(500);
}

static void psxBios_GetConf() { // 9d
	PSXBIOS_LOG("psxBios_%s %x %x %x\n", biosA0n[0x9d], a0, a1, a2);
	storeRam32(a0, loadRam32(A_CONF_EvCB));
	storeRam32(a1, loadRam32(A_CONF_TCB));
	storeRam32(a2, loadRam32(A_CONF_SP));
	mips_return_void_c(10);
}

void psxBios_SetMem() { // 9f
	u32 new = psxHu32(0x1060);

#ifdef PSXBIOS_LOG
	PSXBIOS_LOG("psxBios_%s: %x, %x\n", biosA0n[0x9f], a0, a1);
#endif

	switch(a0) {
		case 2:
			psxHu32ref(0x1060) = SWAP32(new);
			psxMu32ref(0x060) = a0;
			PSXBIOS_LOG("Change effective memory : %d MBytes\n",a0);
			break;

		case 8:
			psxHu32ref(0x1060) = SWAP32(new | 0x300);
			psxMu32ref(0x060) = a0;
			PSXBIOS_LOG("Change effective memory : %d MBytes\n",a0);

		default:
			PSXBIOS_LOG("Effective memory must be 2/8 MBytes\n");
		break;
	}

	pc0 = ra;
}

/* TODO FIXME : Not compliant. -1 indicates failure but using 1 for now. */
static void psxBios_get_cd_status() // a6
{
	PSXBIOS_LOG("psxBios_%s\n", biosA0n[0xa6]);
	v0 = 1;
	pc0 = ra;
}

static void psxBios_GetSystemInfo() { // b4
	u32 ret = 0;
	//PSXBIOS_LOG("psxBios_%s %x\n", biosA0n[0xb4], a0);
	SysPrintf("psxBios_%s %x\n", biosA0n[0xb4], a0);
	switch (a0) {
	case 0:
	case 1: ret = SWAP32(((u32 *)psxR)[0x100/4 + a0]); break;
	case 2: ret = 0xbfc0012c; break;
	case 5: ret = loadRam32(0x60) << 10; break;
	}
	mips_return_c(ret, 20);
}

/* System calls B0 */

static u32 psxBios_SysMalloc_(u32 size);

static void psxBios_SysMalloc() { // B 00
	u32 ret = psxBios_SysMalloc_(a0);

	PSXBIOS_LOG("psxBios_%s 0x%x -> %x\n", biosB0n[0x00], a0, ret);
	mips_return_c(ret, 33);
}

void psxBios_SetRCnt() { // 02
#ifdef PSXBIOS_LOG
	PSXBIOS_LOG("psxBios_%s\n", biosB0n[0x02]);
#endif

	a0&= 0x3;
	if (a0 != 3) {
		u32 mode=0;

		psxRcntWtarget(a0, a1);
		if (a2&0x1000) mode|= 0x050; // Interrupt Mode
		if (a2&0x0100) mode|= 0x008; // Count to 0xffff
		if (a2&0x0010) mode|= 0x001; // Timer stop mode
		if (a0 == 2) { if (a2&0x0001) mode|= 0x200; } // System Clock mode
		else         { if (a2&0x0001) mode|= 0x100; } // System Clock mode

		psxRcntWmode(a0, mode);
	}
	pc0 = ra;
}

void psxBios_GetRCnt() { // 03
#ifdef PSXBIOS_LOG
	PSXBIOS_LOG("psxBios_%s\n", biosB0n[0x03]);
#endif

	switch (a0 & 0x3)
	{
	case 0: v0 = psxRcntRcount0(); break;
	case 1: v0 = psxRcntRcount1(); break;
	case 2: v0 = psxRcntRcount2(); break;
	case 3: v0 = 0; break;
	}
	pc0 = ra;
}

void psxBios_StartRCnt() { // 04
#ifdef PSXBIOS_LOG
	PSXBIOS_LOG("psxBios_%s\n", biosB0n[0x04]);
#endif

	a0&= 0x3;
	if (a0 != 3) psxHu32ref(0x1074)|= SWAP32((u32)((1<<(a0+4))));
	else psxHu32ref(0x1074)|= SWAPu32(0x1);
	v0 = 1; pc0 = ra;
}

void psxBios_StopRCnt() { // 05
#ifdef PSXBIOS_LOG
	PSXBIOS_LOG("psxBios_%s\n", biosB0n[0x05]);
#endif

	a0&= 0x3;
	if (a0 != 3) psxHu32ref(0x1074)&= SWAP32((u32)(~(1<<(a0+4))));
	else psxHu32ref(0x1074)&= SWAPu32(~0x1);
	pc0 = ra;
}

void psxBios_ResetRCnt() { // 06
#ifdef PSXBIOS_LOG
	PSXBIOS_LOG("psxBios_%s\n", biosB0n[0x06]);
#endif

	a0&= 0x3;
	if (a0 != 3) {
		psxRcntWmode(a0, 0);
		psxRcntWtarget(a0, 0);
		psxRcntWcount(a0, 0);
	}
	pc0 = ra;
}

static u32 DeliverEvent(u32 class, u32 spec) {
	EvCB *ev, *ev_first = (EvCB *)loadRam32ptr(A_TT_EvCB);
	u32 evcb_len = loadRam32(A_TT_EvCB + 4);
	u32 ret = loadRam32(A_TT_EvCB) + evcb_len;
	u32 i, lim = evcb_len / 0x1c;

	//printf("%s %08x %x\n", __func__, class, spec);
	for (i = 0, ev = ev_first; i < lim; i++, ev++) {
		use_cycles(8);
		if (SWAP32(ev->status) != EvStACTIVE)
			continue;
		use_cycles(4);
		if (SWAP32(ev->class) != class)
			continue;
		use_cycles(4);
		if (SWAP32(ev->spec) != spec)
			continue;
		use_cycles(6);
		ret = SWAP32(ev->mode);
		if (ret == EvMdMARK) {
			if (ev->status != SWAP32(EvStALREADY))
				PSXBIOS_EV_LOG("DeliverEvent %08x %x (%08zx) set\n",
					class, spec, (ev - ev_first) | 0xf1000000u);
			ev->status = SWAP32(EvStALREADY);
			continue;
		}
		use_cycles(8);
		if (ret == EvMdCALL) {
			ret = SWAP32(ev->fhandler);
			PSXBIOS_EV_LOG("DeliverEvent %08x %x (%08zx) cb %x\n",
				class, spec, (ev - ev_first) | 0xf1000000u, ret);
			if (ret) {
				v0 = ret;
				softCall(ret);
				ret = v0;
			}
		}
	}
	floodchk = 0;
	use_cycles(29);
	return ret;
}

static u32 UnDeliverEvent(u32 class, u32 spec) {
	EvCB *ev = (EvCB *)loadRam32ptr(A_TT_EvCB);
	u32 evcb_len = loadRam32(A_TT_EvCB + 4);
	u32 ret = loadRam32(A_TT_EvCB) + evcb_len;
	u32 i, lim = evcb_len / 0x1c;

	for (i = 0; i < lim; i++, ev++) {
		use_cycles(8);
		if (SWAP32(ev->status) != EvStALREADY)
			continue;
		use_cycles(4);
		if (SWAP32(ev->class) != class)
			continue;
		use_cycles(4);
		if (SWAP32(ev->spec) != spec)
			continue;
		use_cycles(6);
		if (SWAP32(ev->mode) == EvMdMARK)
			ev->status = SWAP32(EvStACTIVE);
	}
	use_cycles(28);
	return ret;
}

static void psxBios_DeliverEvent() { // 07
	u32 ret;
	PSXBIOS_LOG("psxBios_%s %x %04x\n", biosB0n[0x07], a0, a1);

	ret = DeliverEvent(a0, a1);
	mips_return(ret);
}

static s32 get_free_EvCB_slot() {
	EvCB *ev = (EvCB *)loadRam32ptr(A_TT_EvCB);
	u32 i, lim = loadRam32(A_TT_EvCB + 4) / 0x1c;

	use_cycles(19);
	for (i = 0; i < lim; i++, ev++) {
		use_cycles(8);
		if (ev->status == SWAP32(EvStUNUSED))
			return i;
	}
	return -1;
}

static u32 OpenEvent(u32 class, u32 spec, u32 mode, u32 func) {
	u32 ret = get_free_EvCB_slot();
	if ((s32)ret >= 0) {
		EvCB *ev = (EvCB *)loadRam32ptr(A_TT_EvCB) + ret;
		ev->class = SWAP32(class);
		ev->status = SWAP32(EvStDISABLED);
		ev->spec = SWAP32(spec);
		ev->mode = SWAP32(mode);
		ev->fhandler = SWAP32(func);
		ret |= 0xf1000000u;
	}
	return ret;
}

static void psxBios_OpenEvent() { // 08
	u32 ret = OpenEvent(a0, a1, a2, a3);
	PSXBIOS_LOG("psxBios_%s (class:%x, spec:%04x, mode:%04x, func:%x) -> %x\n",
		biosB0n[0x08], a0, a1, a2, a3, ret);
	mips_return_c(ret, 36);
}

static void CloseEvent(u32 ev)
{
	u32 base = loadRam32(A_TT_EvCB);
	storeRam32(base + (ev & 0xffff) * sizeof(EvCB) + 4, EvStUNUSED);
}

static void psxBios_CloseEvent() { // 09
	PSXBIOS_LOG("psxBios_%s %x (%x)\n", biosB0n[0x09], a0,
		loadRam32(loadRam32(A_TT_EvCB) + (a0 & 0xffff) * sizeof(EvCB) + 4));
	CloseEvent(a0);
	mips_return_c(1, 10);
}

static void psxBios_WaitEvent() { // 0a
	u32 base = loadRam32(A_TT_EvCB);
	u32 status = loadRam32(base + (a0 & 0xffff) * sizeof(EvCB) + 4);
	PSXBIOS_LOG("psxBios_%s %x (status=%x)\n", biosB0n[0x0a], a0, status);

	use_cycles(15);
	if (status == EvStALREADY) {
		storeRam32(base + (a0 & 0xffff) * sizeof(EvCB) + 4, EvStACTIVE);
		mips_return(1);
		return;
	}
	if (status != EvStACTIVE)
	{
		mips_return_c(0, 2);
		return;
	}

	// retrigger this hlecall after the next emulation event
	pc0 -= 4;
	if ((s32)(psxRegs.next_interupt - psxRegs.cycle) > 0)
		psxRegs.cycle = psxRegs.next_interupt;
	psxBranchTest();
}

static void psxBios_TestEvent() { // 0b
	u32 base = loadRam32(A_TT_EvCB);
	u32 status = loadRam32(base + (a0 & 0xffff) * sizeof(EvCB) + 4);
	u32 ret = 0;

	if (psxRegs.cycle - floodchk > 16*1024u) { // prevent log flood
		PSXBIOS_LOG("psxBios_%s    %x %x\n", biosB0n[0x0b], a0, status);
		floodchk = psxRegs.cycle;
	}
	if (status == EvStALREADY) {
		storeRam32(base + (a0 & 0xffff) * sizeof(EvCB) + 4, EvStACTIVE);
		ret = 1;
	}

	mips_return_c(ret, 15);
}

static void EnableEvent(u32 ev, int do_log) {
	u32 base = loadRam32(A_TT_EvCB);
	u32 status = loadRam32(base + (ev & 0xffff) * sizeof(EvCB) + 4);
	if (do_log)
		PSXBIOS_LOG("psxBios_%s %x (%x)\n", biosB0n[0x0c], ev, status);
	if (status != EvStUNUSED)
		storeRam32(base + (ev & 0xffff) * sizeof(EvCB) + 4, EvStACTIVE);
}

static void psxBios_EnableEvent() { // 0c
	EnableEvent(a0, 1);
	mips_return_c(1, 15);
}

static void psxBios_DisableEvent() { // 0d
	u32 base = loadRam32(A_TT_EvCB);
	u32 status = loadRam32(base + (a0 & 0xffff) * sizeof(EvCB) + 4);
	PSXBIOS_LOG("psxBios_%s %x: %x\n", biosB0n[0x0d], a0, status);
	if (status != EvStUNUSED)
		storeRam32(base + (a0 & 0xffff) * sizeof(EvCB) + 4, EvStDISABLED);

	mips_return_c(1, 15);
}

/*
 *	long OpenTh(long (*func)(), unsigned long sp, unsigned long gp);
 */

void psxBios_OpenTh() { // 0e
	TCB *tcb = loadRam32ptr(A_TT_TCB);
	u32 limit = loadRam32(A_TT_TCB + 4) / 0xc0u;
	int th;

	for (th = 1; th < limit; th++)
	{
		if (tcb[th].status != SWAP32(0x4000)) break;

	}
	if (th == limit) {
		// Feb 2019 - Added out-of-bounds fix caught by cppcheck:
		// When no free TCB is found, return 0xffffffff according to Nocash doc.
#ifdef PSXBIOS_LOG
		PSXBIOS_LOG("\t%s() WARNING! No Free TCBs found!\n", __func__);
#endif
		mips_return_c(0xffffffff, 20);
		return;
	}
	PSXBIOS_LOG("psxBios_%s -> %x\n", biosB0n[0x0e], 0xff000000 + th);

	tcb[th].status  = SWAP32(0x4000);
	tcb[th].mode    = SWAP32(0x1000);
	tcb[th].epc     = SWAP32(a0);
	tcb[th].reg[30] = SWAP32(a1); // fp
	tcb[th].reg[29] = SWAP32(a1); // sp
	tcb[th].reg[28] = SWAP32(a2); // gp

	mips_return_c(0xff000000 + th, 34);
}

/*
 *	int CloseTh(long thread);
 */

static void psxBios_CloseTh() { // 0f
	u32 tcb = loadRam32(A_TT_TCB);
	u32 th = a0 & 0xffff;

	PSXBIOS_LOG("psxBios_%s %x\n", biosB0n[0x0f], a0);
	// in the usual bios fashion no checks, just write and return 1
	storeRam32(tcb + th * sizeof(TCB), 0x1000);

	mips_return_c(1, 11);
}

/*
 *	int ChangeTh(long thread);
 */

void psxBios_ChangeTh() { // 10
	u32 tcbBase = loadRam32(A_TT_TCB);
	u32 th = a0 & 0xffff;

	// this is quite spammy
	//PSXBIOS_LOG("psxBios_%s %x\n", biosB0n[0x10], th);

	// without doing any argument checks, just issue a syscall
	// (like the real bios does)
	a0 = 3;
	a1 = tcbBase + th * sizeof(TCB);
	pc0 = A_SYSCALL;
	use_cycles(15);
}

void psxBios_InitPAD() { // 0x12
	u32 i, *ram32 = (u32 *)psxM;
	PSXBIOS_LOG("psxBios_%s %x %x %x %x\n", biosB0n[0x12], a0, a1, a2, a3);

	// printf("%s", "PS-X Control PAD Driver  Ver 3.0");
	ram32[A_PAD_DR_DST/4] = 0;
	ram32[A_PAD_OUTBUF/4 + 0] = 0;
	ram32[A_PAD_OUTBUF/4 + 1] = 0;
	ram32[A_PAD_OUT_LEN/4 + 0] = 0;
	ram32[A_PAD_OUT_LEN/4 + 1] = 0;
	ram32[A_PAD_INBUF/4 + 0] = SWAP32(a0);
	ram32[A_PAD_INBUF/4 + 1] = SWAP32(a2);
	ram32[A_PAD_IN_LEN/4 + 0] = SWAP32(a1);
	ram32[A_PAD_IN_LEN/4 + 1] = SWAP32(a3);

	for (i = 0; i < a1; i++) {
		use_cycles(4);
		storeRam8(a0 + i, 0);
	}
	for (i = 0; i < a3; i++) {
		use_cycles(4);
		storeRam8(a2 + i, 0);
	}
	write_chain(ram32 + A_PADCRD_CHN_E/4, 0, 0x49bc, 0x4a4c);

	ram32[A_PAD_IRQR_ENA/4] = SWAP32(1);

	mips_return_c(1, 200);
}

void psxBios_StartPAD() { // 13
	PSXBIOS_LOG("psxBios_%s\n", biosB0n[0x13]);

	psxBios_SysDeqIntRP_(2, A_PADCRD_CHN_E);
	psxBios_SysEnqIntRP_(2, A_PADCRD_CHN_E);
	psxHwWrite16(0x1f801070, ~1);
	psxHwWrite16(0x1f801074, psxHu32(0x1074) | 1);
	storeRam32(A_PAD_ACK_VBL, 1);
	storeRam32(A_RCNT_VBL_ACK + (3 << 2), 0);
	psxRegs.CP0.n.SR |= 0x401;

	mips_return_c(1, 300);
}

void psxBios_StopPAD() { // 14
	PSXBIOS_LOG("psxBios_%s\n", biosB0n[0x14]);
	storeRam32(A_RCNT_VBL_ACK + (3 << 2), 1);
	psxBios_SysDeqIntRP_(2, A_PADCRD_CHN_E);
	psxRegs.CP0.n.SR |= 0x401;
	mips_return_void_c(200);
}

static void psxBios_PAD_init() { // 15
	u32 ret = 0;
	PSXBIOS_LOG("psxBios_%s\n", biosB0n[0x15]);
	if (a0 == 0x20000000 || a0 == 0x20000001)
	{
		u32 dst = a1;
		a0 = A_PAD_DR_BUF1; a1 = 0x22;
		a2 = A_PAD_DR_BUF2; a3 = 0x22;
		psxBios_InitPAD();
		psxBios_StartPAD();
		storeRam32(A_PAD_DR_DST, dst);
		ret = 2;
	}
	mips_return_c(ret, 100);
}

static u32 psxBios_PAD_dr_() {
	u8 *dst = loadRam32ptr(A_PAD_DR_DST);
	u8 *buf1 = castRam8ptr(A_PAD_DR_BUF1);
	u8 *buf2 = castRam8ptr(A_PAD_DR_BUF2);
	dst[0] = dst[1] = dst[2] = dst[3] = ~0;
	if (buf1[0] == 0 && (buf1[1] == 0x23 || buf1[1] == 0x41))
	{
		dst[0] = buf1[3], dst[1] = buf1[2];
		if (buf1[1] == 0x23) {
			dst[0] |= 0xc7, dst[1] |= 7;
			if (buf1[5] >= 0x10) dst[0] &= ~(1u << 6);
			if (buf1[6] >= 0x10) dst[0] &= ~(1u << 7);
		}
	}
	if (buf2[0] == 0 && (buf2[1] == 0x23 || buf2[1] == 0x41))
	{
		dst[2] = buf2[3], dst[3] = buf2[2];
		if (buf2[1] == 0x23) {
			dst[2] |= 0xc7, dst[3] |= 7;
			if (buf2[5] >= 0x10) dst[2] &= ~(1u << 6);
			if (buf2[6] >= 0x10) dst[2] &= ~(1u << 7);
		}
	}
	use_cycles(55);
	return SWAP32(*(u32 *)dst);
}

static void psxBios_PAD_dr() { // 16
	PSXBIOS_LOG("psxBios_%s\n", biosB0n[0x16]);
	u32 ret = psxBios_PAD_dr_();
	mips_return(ret);
}

static void psxBios_ReturnFromException() { // 17
	u32 tcbPtr = loadRam32(A_TT_PCB);
	const TCB *tcb = loadRam32ptr(tcbPtr);
	u32 sr;
	int i;

	for (i = 1; i < 32; i++)
		psxRegs.GPR.r[i] = SWAP32(tcb->reg[i]);
	psxRegs.GPR.n.lo = SWAP32(tcb->lo);
	psxRegs.GPR.n.hi = SWAP32(tcb->hi);
	sr = SWAP32(tcb->sr);

	//printf("%s %08x->%08x %u\n", __func__, pc0, tcb->epc, psxRegs.cycle);
	pc0 = k0 = SWAP32(tcb->epc);

	// the interpreter wants to know about sr changes, so do a MTC0
	sr = (sr & ~0x0f) | ((sr & 0x3c) >> 2);
	MTC0(&psxRegs, 12, sr);

	use_cycles(53);
	psxBranchTest();
}

void psxBios_ResetEntryInt() { // 18
	PSXBIOS_LOG("psxBios_%s\n", biosB0n[0x18]);

	storeRam32(A_EEXIT_PTR, A_EEXIT_DEF);
	mips_return_void_c(5);
}

void psxBios_HookEntryInt() { // 19
	PSXBIOS_LOG("psxBios_%s %x\n", biosB0n[0x19], a0);

	storeRam32(A_EEXIT_PTR, a0);
	mips_return_void_c(3);
}

static void psxBios_UnDeliverEvent() { // 0x20
	u32 ret;
	PSXBIOS_LOG("psxBios_%s %x %x\n", biosB0n[0x20], a0, a1);

	ret = UnDeliverEvent(a0, a1);
	mips_return(ret);
}

static void buopen(int mcd, char *ptr, char *cfg)
{
	int i;
	char *mcd_data = ptr;

	strcpy(FDesc[1 + mcd].name, Ra0+5);
	FDesc[1 + mcd].offset = 0;
	FDesc[1 + mcd].mode   = a1;

	for (i=1; i<16; i++) {
		const char *fptr = mcd_data + 128 * i;
		if ((*fptr & 0xF0) != 0x50) continue;
		if (strcmp(FDesc[1 + mcd].name, fptr+0xa)) continue;
		FDesc[1 + mcd].mcfile = i;
		v0 = 1 + mcd;
		PSXBIOS_LOG("open %s -> %d\n", fptr+0xa, v0);
		break;
	}
	if (a1 & 0x200 && v0 == -1) { /* FCREAT */
		for (i=1; i<16; i++) {
			int j, xor, nblk = a1 >> 16;
			char *pptr, *fptr2;
			char *fptr = mcd_data + 128 * i;

			if ((*fptr & 0xF0) != 0xa0) continue;

			FDesc[1 + mcd].mcfile = i;
			fptr[0] = 0x51;
			fptr[4] = 0x00;
			fptr[5] = 0x20 * nblk;
			fptr[6] = 0x00;
			fptr[7] = 0x00;
			strcpy(fptr+0xa, FDesc[1 + mcd].name);
			pptr = fptr2 = fptr;
			for(j=2; j<=nblk; j++) {
				int k;
				for(i++; i<16; i++) {
					fptr2 += 128;

					memset(fptr2, 0, 128);
					fptr2[0] = j < nblk ? 0x52 : 0x53;
					pptr[8] = i - 1;
					pptr[9] = 0;
					for (k=0, xor=0; k<127; k++) xor^= pptr[k];
					pptr[127] = xor;
					pptr = fptr2;
					break;
				}
				/* shouldn't this return ENOSPC if i == 16? */
			}
			pptr[8] = pptr[9] = 0xff;
			for (j=0, xor=0; j<127; j++) xor^= pptr[j];
			pptr[127] = xor;
			PSXBIOS_LOG("openC %s %d\n", ptr, nblk);
			v0 = 1 + mcd;
			/* just go ahead and resave them all */
			SaveMcd(cfg, ptr, 128, 128 * 15);
			break;
		}
		/* shouldn't this return ENOSPC if i == 16? */
	}
}

/*
 *	int open(char *name , int mode);
 */

void psxBios_open() { // 0x32
	void *pa0 = Ra0;

	PSXBIOS_LOG("psxBios_%s %s(%x) %x\n", biosB0n[0x32], Ra0, a0, a1);

	v0 = -1;

	if (pa0 != INVALID_PTR) {
		if (!strncmp(pa0, "bu00", 4)) {
			buopen(1, Mcd1Data, Config.Mcd1);
		}

		if (!strncmp(pa0, "bu10", 4)) {
			buopen(2, Mcd2Data, Config.Mcd2);
		}
	}

	pc0 = ra;
}

/*
 *	int lseek(int fd , int offset , int whence);
 */

void psxBios_lseek() { // 0x33
#ifdef PSXBIOS_LOG
	PSXBIOS_LOG("psxBios_%s: %x, %x, %x\n", biosB0n[0x33], a0, a1, a2);
#endif

	switch (a2) {
		case 0: // SEEK_SET
			FDesc[a0].offset = a1;
			v0 = a1;
//			DeliverEvent(0xf0000011, 0x0004);
//			DeliverEvent(0xf4000001, 0x0004);
			break;

		case 1: // SEEK_CUR
			FDesc[a0].offset+= a1;
			v0 = FDesc[a0].offset;
			break;
	}

	pc0 = ra;
}


/*
 *	int read(int fd , void *buf , int nbytes);
 */

static void psxBios_read() { // 0x34
	void *pa1 = Ra1;
	int ret = -1;

	PSXBIOS_LOG("psxBios_%s: %x, %x, %x\n", biosB0n[0x34], a0, a1, a2);

	if (pa1 == INVALID_PTR)
		;
	else if (a0 == 2 || a0 == 3)
		ret = card_buf_io(0, a0 - 2, pa1, a2);

	mips_return_c(ret, 100);
}

/*
 *	int write(int fd , void *buf , int nbytes);
 */

static void psxBios_write() { // 0x35/0x03
	void *pa1 = Ra1;
	int ret = -1;

	if (a0 != 1) // not stdout
		PSXBIOS_LOG("psxBios_%s: %x,%x,%x\n", biosB0n[0x35], a0, a1, a2);

	if (pa1 == INVALID_PTR)
		;
	else if (a0 == 1) { // stdout
		char *ptr = pa1;

		v0 = a2;
		if (Config.PsxOut) while (a2 > 0) {
			SysPrintf("%c", *ptr++); a2--;
		}
		pc0 = ra; return;
	}
	else if (a0 == 2 || a0 == 3)
		ret = card_buf_io(1, a0 - 2, pa1, a2);

	mips_return_c(ret, 100);
}

static void psxBios_write_psxout() {
	if (a0 == 1) { // stdout
		const char *ptr = Ra1;
		int len = a2;

		if (ptr != INVALID_PTR)
			while (len-- > 0)
				SysPrintf("%c", *ptr++);
	}
}

static void psxBios_putchar_psxout() { // 3d
	SysPrintf("%c", (char)a0);
}

static void psxBios_puts_psxout() { // 3e/3f
	SysPrintf("%s", Ra0);
}

/*
 *	int close(int fd);
 */

void psxBios_close() { // 0x36
#ifdef PSXBIOS_LOG
	PSXBIOS_LOG("psxBios_%s: %x\n", biosB0n[0x36], a0);
#endif

	v0 = a0;
	pc0 = ra;
}

void psxBios_putchar() { // 3d
	if (Config.PsxOut) SysPrintf("%c", (char)a0);
	pc0 = ra;
}

void psxBios_puts() { // 3e/3f
	if (Config.PsxOut) SysPrintf("%s", Ra0);
	pc0 = ra;
}

static void bufile(const u8 *mcd_data, u32 dir_) {
	struct DIRENTRY *dir = (struct DIRENTRY *)PSXM(dir_);
	const char *pfile = ffile + 5;
	const u8 *data = mcd_data;
	int i = 0, match = 0;
	int blocks = 1;
	u32 head = 0;

	v0 = 0;
	if (dir == INVALID_PTR)
		return;

	for (; nfile <= 15 && !match; nfile++) {
		const char *name;

		head = nfile * 0x40;
		data = mcd_data + 128 * nfile;
		name = (const char *)data + 0x0a;
		if ((data[0] & 0xF0) != 0x50) continue;
		/* Bug link files show up as free block. */
		if (!name[0]) continue;
		match = 1;
		for (i = 0; i < 20; i++) {
			if (pfile[i] == name[i] || pfile[i] == '?')
				dir->name[i] = name[i];
			else if (pfile[i] == '*') {
				int len = strlen(name + i);
				if (i + len > 20)
					len = 20 - i;
				memcpy(dir->name + i, name + i, len + 1);
				i += len;
				break;
			}
			else {
				match = 0;
				break;
			}
			if (!name[i])
				break;
		}
		PSXBIOS_LOG("%d : %s = %s + %s (match=%d)\n",
			nfile, dir->name, pfile, name, match);
	}
	for (; nfile <= 15; nfile++, blocks++) {
		const u8 *data2 = mcd_data + 128 * nfile;
		const char *name = (const char *)data2 + 0x0a;
		if ((data2[0] & 0xF0) != 0x50 || name[0])
			break;
	}
	if (match) {
		// nul char of full lenth name seems to overwrite .attr
		dir->attr = SWAP32(i < 20 ? data[0] & 0xf0 : 0); // ?
		dir->size = 8192 * blocks;
		dir->head = head;
		v0 = dir_;
	}
	PSXBIOS_LOG("  -> %x '%s' %x %x %x %x\n", v0, v0 ? dir->name : "",
		    dir->attr, dir->size, dir->next, dir->head);
}

/*
 *	struct DIRENTRY* firstfile(char *name,struct DIRENTRY *dir);
 */

static void psxBios_firstfile() { // 42
	char *pa0 = Ra0;

	PSXBIOS_LOG("psxBios_%s %s %x\n", biosB0n[0x42], pa0, a1);
	v0 = 0;

	if (pa0 != INVALID_PTR)
	{
		snprintf(ffile, sizeof(ffile), "%s", pa0);
		if (ffile[5] == 0)
			strcpy(ffile + 5, "*"); // maybe?
		nfile = 1;
		if (!strncmp(pa0, "bu00", 4)) {
			// firstfile() calls _card_read() internally, so deliver it's event
			DeliverEvent(0xf0000011, 0x0004);
			bufile((u8 *)Mcd1Data, a1);
		} else if (!strncmp(pa0, "bu10", 4)) {
			// firstfile() calls _card_read() internally, so deliver it's event
			DeliverEvent(0xf0000011, 0x0004);
			bufile((u8 *)Mcd2Data, a1);
		}
	}

	pc0 = ra;
}

/*
 *	struct DIRENTRY* nextfile(struct DIRENTRY *dir);
 */

void psxBios_nextfile() { // 43
	PSXBIOS_LOG("psxBios_%s %x\n", biosB0n[0x43], a0);

	v0 = 0;
	if (!strncmp(ffile, "bu00", 4))
		bufile((u8 *)Mcd1Data, a0);
	else if (!strncmp(ffile, "bu10", 4))
		bufile((u8 *)Mcd2Data, a0);

	pc0 = ra;
}

#define burename(mcd) { \
	for (i=1; i<16; i++) { \
		int namelen, j, xor = 0; \
		ptr = Mcd##mcd##Data + 128 * i; \
		if ((*ptr & 0xF0) != 0x50) continue; \
		if (strcmp(Ra0+5, ptr+0xa)) continue; \
		namelen = strlen(Ra1+5); \
		memcpy(ptr+0xa, Ra1+5, namelen); \
		memset(ptr+0xa+namelen, 0, 0x75-namelen); \
		for (j=0; j<127; j++) xor^= ptr[j]; \
		ptr[127] = xor; \
		SaveMcd(Config.Mcd##mcd, Mcd##mcd##Data, 128 * i + 0xa, 0x76); \
		v0 = 1; \
		break; \
	} \
}

/*
 *	int rename(char *old, char *new);
 */

void psxBios_rename() { // 44
	void *pa0 = Ra0;
	void *pa1 = Ra1;
	char *ptr;
	int i;

#ifdef PSXBIOS_LOG
	PSXBIOS_LOG("psxBios_%s: %s,%s\n", biosB0n[0x44], Ra0, Ra1);
#endif

	v0 = 0;

	if (pa0 != INVALID_PTR && pa1 != INVALID_PTR) {
		if (!strncmp(pa0, "bu00", 4) && !strncmp(pa1, "bu00", 4)) {
			burename(1);
		}

		if (!strncmp(pa0, "bu10", 4) && !strncmp(pa1, "bu10", 4)) {
			burename(2);
		}
	}

	pc0 = ra;
}


#define budelete(mcd) { \
	for (i=1; i<16; i++) { \
		ptr = Mcd##mcd##Data + 128 * i; \
		if ((*ptr & 0xF0) != 0x50) continue; \
		if (strcmp(Ra0+5, ptr+0xa)) continue; \
		*ptr = (*ptr & 0xf) | 0xA0; \
		SaveMcd(Config.Mcd##mcd, Mcd##mcd##Data, 128 * i, 1); \
		PSXBIOS_LOG("delete %s\n", ptr+0xa); \
		v0 = 1; \
		break; \
	} \
}

/*
 *	int delete(char *name);
 */

void psxBios_delete() { // 45
	void *pa0 = Ra0;
	char *ptr;
	int i;

#ifdef PSXBIOS_LOG
	PSXBIOS_LOG("psxBios_%s: %s\n", biosB0n[0x45], Ra0);
#endif

	v0 = 0;

	if (pa0 != INVALID_PTR) {
		if (!strncmp(pa0, "bu00", 4)) {
			budelete(1);
		}

		if (!strncmp(pa0, "bu10", 4)) {
			budelete(2);
		}
	}

	pc0 = ra;
}

void psxBios_InitCARD() { // 4a
	u8 *ram8 = (u8 *)psxM;
	u32 *ram32 = (u32 *)psxM;
	PSXBIOS_LOG("psxBios_%s: %x\n", biosB0n[0x4a], a0);
	write_chain(ram32 + A_PADCRD_CHN_E/4, 0, 0x49bc, 0x4a4c);
	//card_error = 0;
	ram8[A_CARD_ISLOT] = 0;
	ram8[A_CARD_STATUS1] = 1;
	ram8[A_CARD_STATUS2] = 1;
	// (maybe) todo: early_card_irq, etc

	ram32[A_PAD_IRQR_ENA/4] = SWAP32(a0);

	psxBios_FlushCache();
	mips_return_c(0, 34+13+15+6);
}

void psxBios_StartCARD() { // 4b
	PSXBIOS_LOG("psxBios_%s\n", biosB0n[0x4b]);
	psxBios_SysDeqIntRP_(2, A_PADCRD_CHN_E);
	psxBios_SysEnqIntRP_(2, A_PADCRD_CHN_E);

	psxHwWrite16(0x1f801074, psxHu32(0x1074) | 1);
	storeRam32(A_PAD_ACK_VBL, 1);
	storeRam32(A_RCNT_VBL_ACK + (3 << 2), 0);
	storeRam32(A_CARD_IRQR_ENA, 1);
	psxRegs.CP0.n.SR |= 0x401;

	mips_return_c(1, 200);
}

void psxBios_StopCARD() { // 4c
	PSXBIOS_LOG("psxBios_%s\n", biosB0n[0x4c]);
	storeRam32(A_RCNT_VBL_ACK + (3 << 2), 1);
	psxBios_SysDeqIntRP_(2, A_PADCRD_CHN_E);
	storeRam32(A_CARD_IRQR_ENA, 0);
	psxRegs.CP0.n.SR |= 0x401;
	mips_return_void_c(200);
}

void psxBios__card_write() { // 0x4e
	void *pa2 = Ra2;
	int port;

	PSXBIOS_LOG("psxBios_%s %02x,%x,%x\n", biosB0n[0x4e], a0, a1, a2);
	// function also accepts sector 400h (a bug),
	// but what actually happens then?
	if (a1 > 0x400)
	{
		/* Invalid sectors */
		v0 = 0; pc0 = ra;
		return;
	}
	storeRam32(A_CARD_ACHAN, a0);
	port = a0 >> 4;

	if (pa2 != INVALID_PTR && a1 < 0x400) {
		if (port == 0) {
			memcpy(Mcd1Data + a1 * 128, pa2, 128);
			SaveMcd(Config.Mcd1, Mcd1Data, a1 * 128, 128);
		} else {
			memcpy(Mcd2Data + a1 * 128, pa2, 128);
			SaveMcd(Config.Mcd2, Mcd2Data, a1 * 128, 128);
		}
	}

	storeRam8(A_CARD_STATUS1 + port, 4); // busy/write
	storeRam32(A_CARD_HANDLER, CARD_HARDLER_WRITE);

	v0 = 1; pc0 = ra;
}

static void psxBios__card_read() { // 0x4f
	void *pa2 = Ra2;
	int port;

	PSXBIOS_LOG("psxBios_%s %x,%x,%x\n", biosB0n[0x4f], a0, a1, a2);
	if (a1 > 0x400)
	{
		/* Invalid sectors */
		v0 = 0; pc0 = ra;
		return;
	}
	storeRam32(A_CARD_ACHAN, a0);
	port = a0 >> 4;

	if (pa2 != INVALID_PTR && a1 < 0x400) {
		if (port == 0) {
			memcpy(pa2, Mcd1Data + a1 * 128, 128);
		} else {
			memcpy(pa2, Mcd2Data + a1 * 128, 128);
		}
	}

	storeRam8(A_CARD_STATUS1 + port, 2); // busy/read
	storeRam32(A_CARD_HANDLER, CARD_HARDLER_READ);

	v0 = 1; pc0 = ra;
}

void psxBios__new_card() { // 0x50
#ifdef PSXBIOS_LOG
	PSXBIOS_LOG("psxBios_%s\n", biosB0n[0x50]);
#endif

	pc0 = ra;
}

/* According to a user, this allows Final Fantasy Tactics to save/load properly */
void psxBios__get_error(void) // 55
{
	PSXBIOS_LOG("psxBios_%s\n", biosB0n[0x55]);
	v0 = 0;
	pc0 = ra;
}

void psxBios_Krom2RawAdd() { // 0x51
	int i = 0;

	PSXBIOS_LOG("psxBios_%s\n", biosB0n[0x51]);
	const u32 table_8140[][2] = {
		{0x8140, 0x0000}, {0x8180, 0x0762}, {0x81ad, 0x0cc6}, {0x81b8, 0x0ca8},
		{0x81c0, 0x0f00}, {0x81c8, 0x0d98}, {0x81cf, 0x10c2}, {0x81da, 0x0e6a},
		{0x81e9, 0x13ce}, {0x81f0, 0x102c}, {0x81f8, 0x1590}, {0x81fc, 0x111c},
		{0x81fd, 0x1626}, {0x824f, 0x113a}, {0x8259, 0x20ee}, {0x8260, 0x1266},
		{0x827a, 0x24cc}, {0x8281, 0x1572}, {0x829b, 0x28aa}, {0x829f, 0x187e},
		{0x82f2, 0x32dc}, {0x8340, 0x2238}, {0x837f, 0x4362}, {0x8380, 0x299a},
		{0x8397, 0x4632}, {0x839f, 0x2c4c}, {0x83b7, 0x49f2}, {0x83bf, 0x2f1c},
		{0x83d7, 0x4db2}, {0x8440, 0x31ec}, {0x8461, 0x5dde}, {0x8470, 0x35ca},
		{0x847f, 0x6162}, {0x8480, 0x378c}, {0x8492, 0x639c}, {0x849f, 0x39a8},
		{0xffff, 0}
	};

	const u32 table_889f[][2] = {
		{0x889f, 0x3d68},  {0x8900, 0x40ec},  {0x897f, 0x4fb0},  {0x8a00, 0x56f4},
		{0x8a7f, 0x65b8},  {0x8b00, 0x6cfc},  {0x8b7f, 0x7bc0},  {0x8c00, 0x8304},
		{0x8c7f, 0x91c8},  {0x8d00, 0x990c},  {0x8d7f, 0xa7d0},  {0x8e00, 0xaf14},
		{0x8e7f, 0xbdd8},  {0x8f00, 0xc51c},  {0x8f7f, 0xd3e0},  {0x9000, 0xdb24},
		{0x907f, 0xe9e8},  {0x9100, 0xf12c},  {0x917f, 0xfff0},  {0x9200, 0x10734},
		{0x927f, 0x115f8}, {0x9300, 0x11d3c}, {0x937f, 0x12c00}, {0x9400, 0x13344},
		{0x947f, 0x14208}, {0x9500, 0x1494c}, {0x957f, 0x15810}, {0x9600, 0x15f54},
		{0x967f, 0x16e18}, {0x9700, 0x1755c}, {0x977f, 0x18420}, {0x9800, 0x18b64},
		{0xffff, 0}
	};

	if (a0 >= 0x8140 && a0 <= 0x84be) {
		while (table_8140[i][0] <= a0) i++;
		a0 -= table_8140[i - 1][0];
		v0 = 0xbfc66000 + (a0 * 0x1e + table_8140[i - 1][1]);
	} else if (a0 >= 0x889f && a0 <= 0x9872) {
		while (table_889f[i][0] <= a0) i++;
		a0 -= table_889f[i - 1][0];
		v0 = 0xbfc66000 + (a0 * 0x1e + table_889f[i - 1][1]);
	} else {
		v0 = 0xffffffff;
	}

	pc0 = ra;
}

void psxBios_GetC0Table() { // 56
	PSXBIOS_LOG("psxBios_%s\n", biosB0n[0x56]);
	log_unhandled("GetC0Table @%08x\n", ra);

	mips_return_c(A_C0_TABLE, 3);
}

void psxBios_GetB0Table() { // 57
	PSXBIOS_LOG("psxBios_%s\n", biosB0n[0x57]);
	log_unhandled("GetB0Table @%08x\n", ra);

	mips_return_c(A_B0_TABLE, 3);
}

static void psxBios__card_chan() { // 0x58
	// todo: should return active slot channel
	// (active - which was last processed by irq code)
	u32 ret = loadRam32(A_CARD_ACHAN);
	PSXBIOS_LOG("psxBios_%s -> %02x\n", biosB0n[0x58], ret);

	mips_return_c(ret, 8);
}

static void psxBios_ChangeClearPad() { // 5b
	u32 ret;
	PSXBIOS_LOG("psxBios_%s: %x\n", biosB0n[0x5b], a0);
	ret = loadRam32(A_PAD_ACK_VBL);
	storeRam32(A_PAD_ACK_VBL, a0);

	mips_return_c(ret, 6);
}

static void psxBios__card_status() { // 5c
	u8 s = loadRam8(A_CARD_STATUS1 + a0);
	PSXBIOS_LOG("psxBios_%s %x -> %x\n", biosB0n[0x5c], a0, s);

	mips_return_c(s, 5);
}

static void psxBios__card_wait() { // 5d
	u8 s = loadRam8(A_CARD_STATUS1 + a0);
	PSXBIOS_LOG("psxBios_%s %x -> %x\n", biosB0n[0x5d], a0, s);

	// todo
	if (!(s & 1))
		log_unhandled("%s %x\n", __func__, s);

	mips_return_c(s, 11);
}

static void psxBios__card_info() { // A ab
	PSXBIOS_LOG("psxBios_%s %02x\n", biosA0n[0xab], a0);
	u32 ret, port;
	storeRam32(A_CARD_ACHAN, a0);
	port = a0 >> 4;

	switch (port) {
	case 0x0:
	case 0x1:
		ret = 0x0004;
		if (McdDisable[port & 1])
			ret = 0x0100;
		break;
	default:
		PSXBIOS_LOG("psxBios_%s: UNKNOWN PORT 0x%x\n", biosA0n[0xab], a0);
		ret = 0x0302;
		break;
	}

	if (McdDisable[0] && McdDisable[1])
		ret = 0x0100;

	if (ret == 4) {
		// deliver from card_vint_handler()
		storeRam8(A_CARD_STATUS1 + port, 8); // busy/info
		storeRam32(A_CARD_HANDLER, CARD_HARDLER_INFO);
	} else {
		DeliverEvent(0xf4000001, ret);
		DeliverEvent(0xf0000011, 0x0004); // ?
	}
	mips_return(1);
}

static void psxBios__card_load() { // A ac
	PSXBIOS_LOG("psxBios_%s %02x\n", biosA0n[0xac], a0);

	storeRam32(A_CARD_ACHAN, a0);

//	DeliverEvent(0xf0000011, 0x0004);
	DeliverEvent(0xf4000001, 0x0004);

	mips_return(1);
}

static void card_vint_handler(void) {
	u8 select, status;
	u32 handler;

	if (card_io_delay) {
		card_io_delay--;
		return;
	}
	UnDeliverEvent(0xf0000011, 0x0004);
	UnDeliverEvent(0xf0000011, 0x8000);
	UnDeliverEvent(0xf0000011, 0x0100);
	UnDeliverEvent(0xf0000011, 0x0200);
	UnDeliverEvent(0xf0000011, 0x2000);

#if 0
	select = loadRam8(A_CARD_ISLOT);
	select = (select ^ 1) & 1;
	storeRam8(A_CARD_ISLOT, select);
#else
	select = loadRam8(A_CARD_ACHAN) >> 4;
	storeRam8(A_CARD_ISLOT, select);
#endif
	status = loadRam8(A_CARD_STATUS1 + select);
	if (status & 1)
		return; // done

	//psxBios_SysDeqIntRP_(0, 0x7540);
	//psxBios_SysDeqIntRP_(0, 0x7540);
	//card_state_machine = 0;
	//card_error_flag = 0;
	handler = loadRam32(A_CARD_HANDLER);
	switch (handler) {
	case CARD_HARDLER_INFO:
		DeliverEvent(0xf4000001, 4);
		DeliverEvent(0xf0000011, 4);
		storeRam8(A_CARD_STATUS1 + select, 1);
		storeRam32(A_CARD_HANDLER, 0);
		break;
	case CARD_HARDLER_WRITEM:
	case CARD_HARDLER_READM:
		DeliverEvent(0xf4000001, 4);
		// fallthrough
	case CARD_HARDLER_WRITE:
	case CARD_HARDLER_READ:
		DeliverEvent(0xf0000011, 4);
		storeRam8(A_CARD_STATUS1 + select, 1);
		storeRam32(A_CARD_HANDLER, 0);
		break;
	case 0:
		break;
	default:
		log_unhandled("%s: unhandled handler %x\n", __func__, handler);
		DeliverEvent(0xf0000011, 0x8000);
		storeRam8(A_CARD_STATUS1 + select, 1);
		storeRam32(A_CARD_HANDLER, 0);
	}
}

/* System calls C0 */

static void psxBios_InitRCnt() { // 00
	int i;
	PSXBIOS_LOG("psxBios_%s %x\n", biosC0n[0x00], a0);
	psxHwWrite16(0x1f801074, psxHu32(0x1074) & ~0x71);
	for (i = 0; i < 3; i++) {
		psxHwWrite16(0x1f801100 + i*0x10 + 4, 0);
		psxHwWrite16(0x1f801100 + i*0x10 + 8, 0);
		psxHwWrite16(0x1f801100 + i*0x10 + 0, 0);
	}
	for (i = 0; i < 4; i++)
		psxBios_SysEnqIntRP_(a0, 0x6d58 + i * 0x10);
	mips_return_c(0, 9);
}

static void psxBios_InitException() { // 01
	PSXBIOS_LOG("psxBios_%s %x\n", biosC0n[0x01], a0);
	psxBios_SysEnqIntRP_(a0, 0x6da8);
	mips_return_c(0, 9);
}

/*
 * int SysEnqIntRP(int index , long *queue);
 */

static void psxBios_SysEnqIntRP_(u32 priority, u32 chain_eptr) {
	u32 old, base = loadRam32(A_TT_ExCB);

	old = loadRam32(base + (priority << 3));
	storeRam32(base + (priority << 3), chain_eptr);
	storeRam32(chain_eptr, old);
	mips_return_c(0, 9);
}

static void psxBios_SysEnqIntRP() { // 02
	PSXBIOS_LOG("psxBios_%s %x %x\n", biosC0n[0x02], a0, a1);
	psxBios_SysEnqIntRP_(a0, a1);
}

/*
 * int SysDeqIntRP(int index , long *queue);
 */

static void psxBios_SysDeqIntRP_(u32 priority, u32 chain_rm_eptr) {
	u32 ptr, next, base = loadRam32(A_TT_ExCB);
	u32 lim = 0, ret = 0;

	// as in original: no arg checks of any kind, bug if a1 == 0
	ptr = loadRam32(base + (priority << 3));
	while (ptr) {
		next = loadRam32(ptr);
		if (ptr == chain_rm_eptr) {
			storeRam32(base + (priority << 3), next);
			ret = ptr;
			use_cycles(6);
			break;
		}
		while (next && next != chain_rm_eptr && lim++ < 100) {
			ptr = next;
			next = loadRam32(ptr);
			use_cycles(8);
		}
		if (next == chain_rm_eptr) {
			next = loadRam32(next);
			storeRam32(ptr, next);
			ret = ptr;
			use_cycles(6);
		}
		break;
	}
	if (lim == 100)
		PSXBIOS_LOG("bad chain %u %x\n", priority, base);

	mips_return_c(ret, 12);
}

static void psxBios_SysDeqIntRP() { // 03
	PSXBIOS_LOG("psxBios_%s %x %x\n", biosC0n[0x03], a0, a1);
	psxBios_SysDeqIntRP_(a0, a1);
}

static void psxBios_get_free_EvCB_slot() { // 04
	PSXBIOS_LOG("psxBios_%s\n", biosC0n[0x04]);
	s32 ret = get_free_EvCB_slot();
	mips_return_c(ret, 0);
}
	
static void psxBios_SysInitMemory_(u32 base, u32 size) {
	storeRam32(base, 0);
	storeRam32(A_KMALLOC_PTR, base);
	storeRam32(A_KMALLOC_SIZE, size);
	storeRam32(A_KMALLOC_END, base + (size & ~3) + 4);
}

// this should be much more complicated, but maybe that'll be enough
static u32 psxBios_SysMalloc_(u32 size) {
	u32 ptr = loadRam32(A_KMALLOC_PTR);

	size = (size + 3) & ~3;
	storeRam32(A_KMALLOC_PTR, ptr + 4 + size);
	storeRam32(ptr, size);
	return ptr + 4;
}

static void psxBios_SysInitMemory() { // 08
	PSXBIOS_LOG("psxBios_%s %x %x\n", biosC0n[0x08], a0, a1);

	psxBios_SysInitMemory_(a0, a1);
	mips_return_void_c(12);
}

static void psxBios_ChangeClearRCnt() { // 0a
	u32 ret;

	PSXBIOS_LOG("psxBios_%s: %x, %x\n", biosC0n[0x0a], a0, a1);

	ret = loadRam32(A_RCNT_VBL_ACK + (a0 << 2));
	storeRam32(A_RCNT_VBL_ACK + (a0 << 2), a1);
	mips_return_c(ret, 8);
}

static void psxBios_InitDefInt() { // 0c
	PSXBIOS_LOG("psxBios_%s %x\n", biosC0n[0x0c], a0);
	// should also clear the autoack table
	psxBios_SysEnqIntRP_(a0, 0x6d98);
	mips_return_c(0, 20 + 6*2);
}

void psxBios_dummy() {
	u32 pc = (pc0 & 0x1fffff) - 4;
	char **ntab = pc == 0xa0 ? biosA0n : pc == 0xb0 ? biosB0n
		: pc == 0xc0 ? biosC0n : NULL;
	PSXBIOS_LOG("unk %x call: %x ra=%x (%s)\n",
		pc, t1, ra, ntab ? ntab[t1 & 0xff] : "???");
	(void)pc; (void)ntab;
	mips_return_c(0, 100);
}

void (*biosA0[256])();
// C0 and B0 overlap (end of C0 is start of B0)
void (*biosC0[256+128])();
void (**biosB0)() = biosC0 + 128;

static void setup_mips_code()
{
	u32 *ptr;
	ptr = (u32 *)&psxM[A_SYSCALL];
	ptr[0x00/4] = SWAPu32(0x0000000c); // syscall 0
	ptr[0x04/4] = SWAPu32(0x03e00008); // jr    $ra
	ptr[0x08/4] = SWAPu32(0x00000000); // nop

	ptr = (u32 *)&psxM[A_EXCEPTION];
	memset(ptr, 0, 0xc0);              // nops (to be patched by games sometimes)
	ptr[0x10/4] = SWAPu32(0x8c1a0108); // lw    $k0, (0x108)   // PCB
	ptr[0x14/4] = SWAPu32(0x00000000); // nop
	ptr[0x18/4] = SWAPu32(0x8f5a0000); // lw    $k0, ($k0)     // TCB
	ptr[0x1c/4] = SWAPu32(0x00000000); // nop
	ptr[0x20/4] = SWAPu32(0x275a0008); // addiu $k0, $k0, 8    // regs
	ptr[0x24/4] = SWAPu32(0xaf5f007c); // sw    $ra, 0x7c($k0)
	ptr[0x28/4] = SWAPu32(0xaf410004); // sw    $at, 0x04($k0)
	ptr[0x2c/4] = SWAPu32(0xaf420008); // sw    $v0, 0x08($k0)
	ptr[0x30/4] = SWAPu32(0xaf43000c); // sw    $v1, 0x0c($k0)

	ptr[0x60/4] = SWAPu32(0x40037000); // mfc0  $v1, EPC
	ptr[0x64/4] = SWAPu32(0x40026800); // mfc0  $v0, Cause
	ptr[0x6c/4] = SWAPu32(0xaf430080); // sw    $v1, 0x80($k0)

	ptr[0xb0/4] = HLEOP(hleop_exception);
}

static const struct {
	u32 addr;
	enum hle_op op;
} chainfns[] = {
	{ 0xbfc050a4, hleop_exc0_0_1 },
	{ 0xbfc04fbc, hleop_exc0_0_2 },
	{ 0xbfc0506c, hleop_exc0_1_1 },
	{ 0xbfc04dec, hleop_exc0_1_2 },
	{     0x1a00, hleop_exc0_2_2 },
	{     0x19c8, hleop_exc1_0_1 },
	{     0x18bc, hleop_exc1_0_2 },
	{     0x1990, hleop_exc1_1_1 },
	{     0x1858, hleop_exc1_1_2 },
	{     0x1958, hleop_exc1_2_1 },
	{     0x17f4, hleop_exc1_2_2 },
	{     0x1920, hleop_exc1_3_1 },
	{     0x1794, hleop_exc1_3_2 },
	{     0x2458, hleop_exc3_0_2 },
	{     0x49bc, hleop_exc_padcard1 }, // hleExcPadCard1
	{     0x4a4c, hleop_exc_padcard2 },
};

static int chain_hle_op(u32 handler)
{
	size_t i;

	for (i = 0; i < sizeof(chainfns) / sizeof(chainfns[0]); i++)
		if (chainfns[i].addr == handler)
			return chainfns[i].op;
	return hleop_dummy;
}

static void write_chain(u32 *d, u32 next, u32 handler1, u32 handler2)
{
	d[0] = SWAPu32(next);
	d[1] = SWAPu32(handler1);
	d[2] = SWAPu32(handler2);

	// install the hle traps
	if (handler1) PSXMu32ref(handler1) = HLEOP(chain_hle_op(handler1));
	if (handler2) PSXMu32ref(handler2) = HLEOP(chain_hle_op(handler2));
}

static void setup_tt(u32 tcb_cnt, u32 evcb_cnt, u32 stack)
{
	u32 *ram32 = (u32 *)psxM;
	u32 s_excb = 0x20, s_evcb, s_pcb = 4, s_tcb;
	u32 p_excb, p_evcb, p_pcb, p_tcb;
	u32 i;

	PSXBIOS_LOG("setup: tcb %u, evcb %u\n", tcb_cnt, evcb_cnt);

	// the real bios doesn't care, but we just don't
	// want to crash in case of garbage parameters
	if (tcb_cnt > 1024) tcb_cnt = 1024;
	if (evcb_cnt > 1024) evcb_cnt = 1024;
	s_evcb = 0x1c * evcb_cnt;
	s_tcb = 0xc0 * tcb_cnt;

	memset(ram32 + 0xe000/4, 0, s_excb + s_evcb + s_pcb + s_tcb + 5*4);
	psxBios_SysInitMemory_(0xa000e000, 0x2000);
	p_excb = psxBios_SysMalloc_(s_excb);
	p_evcb = psxBios_SysMalloc_(s_evcb);
	p_pcb  = psxBios_SysMalloc_(s_pcb);
	p_tcb  = psxBios_SysMalloc_(s_tcb);

	// "table of tables". Some games modify it
	assert(A_TT_ExCB == 0x0100);
	ram32[0x0100/4] = SWAPu32(p_excb);  // ExCB - exception chains
	ram32[0x0104/4] = SWAPu32(s_excb);  // ExCB size
	ram32[0x0108/4] = SWAPu32(p_pcb);   // PCB - process control
	ram32[0x010c/4] = SWAPu32(s_pcb);   // PCB size
	ram32[0x0110/4] = SWAPu32(p_tcb);   // TCB - thread control
	ram32[0x0114/4] = SWAPu32(s_tcb);   // TCB size
	ram32[0x0120/4] = SWAPu32(p_evcb);  // EvCB - event control
	ram32[0x0124/4] = SWAPu32(s_evcb);  // EvCB size
	ram32[0x0140/4] = SWAPu32(0x8648);  // FCB - file control
	ram32[0x0144/4] = SWAPu32(0x02c0);  // FCB size
	ram32[0x0150/4] = SWAPu32(0x6ee0);  // DCB - device control
	ram32[0x0154/4] = SWAPu32(0x0320);  // DCB size

	storeRam32(p_excb + 0*4, 0x0000);   // chain0
	storeRam32(p_excb + 2*4, 0x6d88);   // chain1
	storeRam32(p_excb + 4*4, 0x0000);   // chain2
	storeRam32(p_excb + 6*4, 0x6d98);   // chain3

	storeRam32(p_pcb, p_tcb);
	storeRam32(p_tcb, 0x4000);          // first TCB
	for (i = 1; i < tcb_cnt; i++)
		storeRam32(p_tcb + sizeof(TCB) * i, 0x1000);

	psxBios_SysEnqIntRP_(0, 0x6da8);
	setup_cd_irq_and_events();

	storeRam32(A_CONF_EvCB, evcb_cnt);
	storeRam32(A_CONF_TCB, tcb_cnt);
	storeRam32(A_CONF_SP, stack);
}

static const u32 gpu_ctl_def[] = {
	0x00000000, 0x01000000, 0x03000000, 0x04000000,
	0x05000800, 0x06c60260, 0x0703fc10, 0x08000027
};

static const u32 gpu_data_def[] = {
	0xe100360b, 0xe2000000, 0xe3000800, 0xe4077e7f,
	0xe5001000, 0xe6000000,
	0x02000000, 0x00000000, 0x01ff03ff
};

// from 1f801d80
static const u16 spu_config[] = {
	0x3fff, 0x37ef, 0x5ebc, 0x5ebc, 0x0000, 0x0000, 0x0000, 0x00a0,
	0x0000, 0x0000, 0x0000, 0x0000, 0xffff, 0x00ff, 0x0000, 0x0000,
	0x0000, 0xe128, 0x0000, 0x0200, 0xf0f0, 0xc085, 0x0004, 0x0000,
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
	0x033d, 0x0231, 0x7e00, 0x5000, 0xb400, 0xb000, 0x4c00, 0xb000,
	0x6000, 0x5400, 0x1ed6, 0x1a31, 0x1d14, 0x183b, 0x1bc2, 0x16b2,
	0x1a32, 0x15ef, 0x15ee, 0x1055, 0x1334, 0x0f2d, 0x11f6, 0x0c5d,
	0x1056, 0x0ae1, 0x0ae0, 0x07a2, 0x0464, 0x0232, 0x8000, 0x8000
};

void psxBiosSetupBootState(void)
{
	boolean hle = Config.HLE;
	u32 *hw = (u32 *)psxH;
	int i;

	// see also SetBootRegs()
	if (hle) {
		v0 = 1; v1 = 4;
		a0 = 1; a2 = a3 = 0; a3 = 0x2a;
		t2 = 0x2d; t4 = 0x23; t5 = 0x2b; t6 = 0xa0010000;
		s0 = 0xa000b870;
		k0 = 0xbfc0d968; k1 = 0xf1c;
		ra = 0xf0001234; // just to easily detect attempts to return
		psxRegs.CP0.n.Cause = 0x20;
		psxRegs.CP0.n.EPC = 0xbfc0d964; // EnterCriticalSection syscall

		hw[0x1000/4] = SWAP32(0x1f000000);
		hw[0x1004/4] = SWAP32(0x1f802000);
		hw[0x1008/4] = SWAP32(0x0013243f);
		hw[0x100c/4] = SWAP32(0x00003022);
		hw[0x1010/4] = SWAP32(0x0013243f);
		hw[0x1014/4] = SWAP32(0x200931e1);
		hw[0x1018/4] = SWAP32(0x00020943);
		hw[0x101c/4] = SWAP32(0x00070777);
		hw[0x1020/4] = SWAP32(0x0000132c);
		hw[0x1060/4] = SWAP32(0x00000b88);
		hw[0x1070/4] = SWAP32(0x00000001);
		hw[0x1074/4] = SWAP32(0x0000000c);
		hw[0x2040/4] = SWAP32(0x00000900);
	}

	hw[0x10a0/4] = SWAP32(0x00ffffff);
	hw[0x10a8/4] = SWAP32(0x00000401);
	hw[0x10b0/4] = SWAP32(0x0008b000);
	hw[0x10b4/4] = SWAP32(0x00010200);
	hw[0x10e0/4] = SWAP32(0x000eccf4);
	hw[0x10e4/4] = SWAP32(0x00000400);
	hw[0x10e8/4] = SWAP32(0x00000002);
	hw[0x10f0/4] = SWAP32(0x00009099);
	hw[0x10f4/4] = SWAP32(0x8c8c0000);

	if (hle) {
		psxRcntWmode(0, 0);
		psxRcntWmode(1, 0);
		psxRcntWmode(2, 0);
	}

	// gpu
	for (i = 0; i < sizeof(gpu_ctl_def) / sizeof(gpu_ctl_def[0]); i++)
		GPU_writeStatus(gpu_ctl_def[i]);
	for (i = 0; i < sizeof(gpu_data_def) / sizeof(gpu_data_def[0]); i++)
		GPU_writeData(gpu_data_def[i]);

	// spu
	for (i = 0x1f801d80; i < sizeof(spu_config) / sizeof(spu_config[0]); i++)
		SPU_writeRegister(0x1f801d80 + i*2, spu_config[i], psxRegs.cycle);
}

static void hleExc0_0_1();
static void hleExc0_0_2();
static void hleExc0_1_1();
static void hleExc0_1_2();

#include "sjisfont.h"

void psxBiosInit() {
	u32 *ptr, *ram32, *rom32;
	char *romc;
	int i;
	uLongf len;

	psxRegs.biosBranchCheck = ~0;

	memset(psxM, 0, 0x10000);
	for(i = 0; i < 256; i++) {
		biosA0[i] = NULL;
		biosB0[i] = NULL;
		biosC0[i] = NULL;
	}
	biosA0[0x03] = biosB0[0x35] = psxBios_write_psxout;
	biosA0[0x3c] = biosB0[0x3d] = psxBios_putchar_psxout;
	biosA0[0x3e] = biosB0[0x3f] = psxBios_puts_psxout;
	// calls putchar() internally so no need to override
	//biosA0[0x3f] = psxBios_printf_psxout;

	if (!Config.HLE) {
		char verstr[0x24+1];
		rom32 = (u32 *)psxR;
		memcpy(verstr, psxR + 0x12c, 0x24);
		verstr[0x24] = 0;
		SysPrintf("BIOS: %08x, '%s', '%c'\n", SWAP32(rom32[0x100/4]),
			verstr, psxR[0x7ff52]);
		return;
	}

	for(i = 0; i < 256; i++) {
		if (biosA0[i] == NULL) biosA0[i] = psxBios_dummy;
		if (biosB0[i] == NULL) biosB0[i] = psxBios_dummy;
		if (biosC0[i] == NULL) biosC0[i] = psxBios_dummy;
	}

	biosA0[0x00] = psxBios_open;
	biosA0[0x01] = psxBios_lseek;
	biosA0[0x02] = psxBios_read;
	biosA0[0x03] = psxBios_write;
	biosA0[0x04] = psxBios_close;
	//biosA0[0x05] = psxBios_ioctl;
	//biosA0[0x06] = psxBios_exit;
	//biosA0[0x07] = psxBios_sys_a0_07;
	biosA0[0x08] = psxBios_getc;
	biosA0[0x09] = psxBios_putc;
	biosA0[0x0a] = psxBios_todigit;
	//biosA0[0x0b] = psxBios_atof;
	biosA0[0x0c] = psxBios_strtoul;
	biosA0[0x0d] = psxBios_strtol;
	biosA0[0x0e] = psxBios_abs;
	biosA0[0x0f] = psxBios_labs;
	biosA0[0x10] = psxBios_atoi;
	biosA0[0x11] = psxBios_atol;
	//biosA0[0x12] = psxBios_atob;
	biosA0[0x13] = psxBios_setjmp;
	biosA0[0x14] = psxBios_longjmp;
	biosA0[0x15] = psxBios_strcat;
	biosA0[0x16] = psxBios_strncat;
	biosA0[0x17] = psxBios_strcmp;
	biosA0[0x18] = psxBios_strncmp;
	biosA0[0x19] = psxBios_strcpy;
	biosA0[0x1a] = psxBios_strncpy;
	biosA0[0x1b] = psxBios_strlen;
	biosA0[0x1c] = psxBios_index;
	biosA0[0x1d] = psxBios_rindex;
	biosA0[0x1e] = psxBios_strchr;
	biosA0[0x1f] = psxBios_strrchr;
	biosA0[0x20] = psxBios_strpbrk;
	biosA0[0x21] = psxBios_strspn;
	biosA0[0x22] = psxBios_strcspn;
	biosA0[0x23] = psxBios_strtok;
	biosA0[0x24] = psxBios_strstr;
	biosA0[0x25] = psxBios_toupper;
	biosA0[0x26] = psxBios_tolower;
	biosA0[0x27] = psxBios_bcopy;
	biosA0[0x28] = psxBios_bzero;
	biosA0[0x29] = psxBios_bcmp;
	biosA0[0x2a] = psxBios_memcpy;
	biosA0[0x2b] = psxBios_memset;
	biosA0[0x2c] = psxBios_memmove;
	biosA0[0x2d] = psxBios_memcmp;
	biosA0[0x2e] = psxBios_memchr;
	biosA0[0x2f] = psxBios_rand;
	biosA0[0x30] = psxBios_srand;
	biosA0[0x31] = psxBios_qsort;
	//biosA0[0x32] = psxBios_strtod;
	biosA0[0x33] = psxBios_malloc;
	biosA0[0x34] = psxBios_free;
	//biosA0[0x35] = psxBios_lsearch;
	//biosA0[0x36] = psxBios_bsearch;
	biosA0[0x37] = psxBios_calloc;
	biosA0[0x38] = psxBios_realloc;
	biosA0[0x39] = psxBios_InitHeap;
	//biosA0[0x3a] = psxBios__exit;
	biosA0[0x3b] = psxBios_getchar;
	biosA0[0x3c] = psxBios_putchar;
	//biosA0[0x3d] = psxBios_gets;
	biosA0[0x3e] = psxBios_puts;
	biosA0[0x3f] = psxBios_printf;
	biosA0[0x40] = psxBios_SystemErrorUnresolvedException;
	//biosA0[0x41] = psxBios_LoadTest;
	biosA0[0x42] = psxBios_Load;
	biosA0[0x43] = psxBios_Exec;
	biosA0[0x44] = psxBios_FlushCache;
	//biosA0[0x45] = psxBios_InstallInterruptHandler;
	biosA0[0x46] = psxBios_GPU_dw;
	biosA0[0x47] = psxBios_mem2vram;
	biosA0[0x48] = psxBios_SendGPU;
	biosA0[0x49] = psxBios_GPU_cw;
	biosA0[0x4a] = psxBios_GPU_cwb;
	biosA0[0x4b] = psxBios_GPU_SendPackets;
	biosA0[0x4c] = psxBios_sys_a0_4c;
	biosA0[0x4d] = psxBios_GPU_GetGPUStatus;
	//biosA0[0x4e] = psxBios_GPU_sync;
	//biosA0[0x4f] = psxBios_sys_a0_4f;
	//biosA0[0x50] = psxBios_sys_a0_50;
	biosA0[0x51] = psxBios_LoadExec;
	//biosA0[0x52] = psxBios_GetSysSp;
	//biosA0[0x53] = psxBios_sys_a0_53;
	biosA0[0x54] = psxBios_CdInit;
	biosA0[0x55] = psxBios__bu_init;
	biosA0[0x56] = psxBios_CdRemove;
	//biosA0[0x57] = psxBios_sys_a0_57;
	//biosA0[0x58] = psxBios_sys_a0_58;
	//biosA0[0x59] = psxBios_sys_a0_59;
	//biosA0[0x5a] = psxBios_sys_a0_5a;
	//biosA0[0x5b] = psxBios_dev_tty_init;
	//biosA0[0x5c] = psxBios_dev_tty_open;
	//biosA0[0x5d] = psxBios_sys_a0_5d;
	//biosA0[0x5e] = psxBios_dev_tty_ioctl;
	//biosA0[0x5f] = psxBios_dev_cd_open;
	//biosA0[0x60] = psxBios_dev_cd_read;
	//biosA0[0x61] = psxBios_dev_cd_close;
	//biosA0[0x62] = psxBios_dev_cd_firstfile;
	//biosA0[0x63] = psxBios_dev_cd_nextfile;
	//biosA0[0x64] = psxBios_dev_cd_chdir;
	//biosA0[0x65] = psxBios_dev_card_open;
	//biosA0[0x66] = psxBios_dev_card_read;
	//biosA0[0x67] = psxBios_dev_card_write;
	//biosA0[0x68] = psxBios_dev_card_close;
	//biosA0[0x69] = psxBios_dev_card_firstfile;
	//biosA0[0x6a] = psxBios_dev_card_nextfile;
	//biosA0[0x6b] = psxBios_dev_card_erase;
	//biosA0[0x6c] = psxBios_dev_card_undelete;
	//biosA0[0x6d] = psxBios_dev_card_format;
	//biosA0[0x6e] = psxBios_dev_card_rename;
	//biosA0[0x6f] = psxBios_dev_card_6f;
	biosA0[0x70] = psxBios__bu_init;
	biosA0[0x71] = psxBios_CdInit;
	biosA0[0x72] = psxBios_CdRemove;
	//biosA0[0x73] = psxBios_sys_a0_73;
	//biosA0[0x74] = psxBios_sys_a0_74;
	//biosA0[0x75] = psxBios_sys_a0_75;
	//biosA0[0x76] = psxBios_sys_a0_76;
	//biosA0[0x77] = psxBios_sys_a0_77;
	//biosA0[0x78] = psxBios__96_CdSeekL;
	//biosA0[0x79] = psxBios_sys_a0_79;
	//biosA0[0x7a] = psxBios_sys_a0_7a;
	//biosA0[0x7b] = psxBios_sys_a0_7b;
	//biosA0[0x7c] = psxBios__96_CdGetStatus;
	//biosA0[0x7d] = psxBios_sys_a0_7d;
	//biosA0[0x7e] = psxBios__96_CdRead;
	//biosA0[0x7f] = psxBios_sys_a0_7f;
	//biosA0[0x80] = psxBios_sys_a0_80;
	//biosA0[0x81] = psxBios_sys_a0_81;
	//biosA0[0x82] = psxBios_sys_a0_82;
	//biosA0[0x83] = psxBios_sys_a0_83;
	//biosA0[0x84] = psxBios_sys_a0_84;
	//biosA0[0x85] = psxBios__96_CdStop;
	//biosA0[0x86] = psxBios_sys_a0_86;
	//biosA0[0x87] = psxBios_sys_a0_87;
	//biosA0[0x88] = psxBios_sys_a0_88;
	//biosA0[0x89] = psxBios_sys_a0_89;
	//biosA0[0x8a] = psxBios_sys_a0_8a;
	//biosA0[0x8b] = psxBios_sys_a0_8b;
	//biosA0[0x8c] = psxBios_sys_a0_8c;
	//biosA0[0x8d] = psxBios_sys_a0_8d;
	//biosA0[0x8e] = psxBios_sys_a0_8e;
	//biosA0[0x8f] = psxBios_sys_a0_8f;
	biosA0[0x90] = hleExc0_1_2;
	biosA0[0x91] = hleExc0_0_2;
	biosA0[0x92] = hleExc0_1_1;
	biosA0[0x93] = hleExc0_0_1;
	//biosA0[0x94] = psxBios_sys_a0_94;
	biosA0[0x95] = psxBios_CdReset;
	//biosA0[0x96] = psxBios_AddCDROMDevice;
	//biosA0[0x97] = psxBios_AddMemCardDevide;
	//biosA0[0x98] = psxBios_DisableKernelIORedirection;
	//biosA0[0x99] = psxBios_EnableKernelIORedirection;
	//biosA0[0x9a] = psxBios_sys_a0_9a;
	//biosA0[0x9b] = psxBios_sys_a0_9b;
	biosA0[0x9c] = psxBios_SetConf;
	biosA0[0x9d] = psxBios_GetConf;
	//biosA0[0x9e] = psxBios_sys_a0_9e;
	biosA0[0x9f] = psxBios_SetMem;
	//biosA0[0xa0] = psxBios__boot;
	//biosA0[0xa1] = psxBios_SystemError;
	biosA0[0xa2] = psxBios_EnqueueCdIntr;
	biosA0[0xa3] = psxBios_DequeueCdIntr;
	//biosA0[0xa4] = psxBios_sys_a0_a4;
	//biosA0[0xa5] = psxBios_ReadSector;
	biosA0[0xa6] = psxBios_get_cd_status;
	//biosA0[0xa7] = psxBios_bufs_cb_0;
	//biosA0[0xa8] = psxBios_bufs_cb_1;
	//biosA0[0xa9] = psxBios_bufs_cb_2;
	//biosA0[0xaa] = psxBios_bufs_cb_3;
	biosA0[0xab] = psxBios__card_info;
	biosA0[0xac] = psxBios__card_load;
	//biosA0[0axd] = psxBios__card_auto;
	//biosA0[0xae] = psxBios_bufs_cd_4;
	//biosA0[0xaf] = psxBios_sys_a0_af;
	//biosA0[0xb0] = psxBios_sys_a0_b0;
	//biosA0[0xb1] = psxBios_sys_a0_b1;
	//biosA0[0xb2] = psxBios_do_a_long_jmp
	//biosA0[0xb3] = psxBios_sys_a0_b3;
	biosA0[0xb4] = psxBios_GetSystemInfo;
//*******************B0 CALLS****************************
	biosB0[0x00] = psxBios_SysMalloc;
	//biosB0[0x01] = psxBios_sys_b0_01;
	biosB0[0x02] = psxBios_SetRCnt;
	biosB0[0x03] = psxBios_GetRCnt;
	biosB0[0x04] = psxBios_StartRCnt;
	biosB0[0x05] = psxBios_StopRCnt;
	biosB0[0x06] = psxBios_ResetRCnt;
	biosB0[0x07] = psxBios_DeliverEvent;
	biosB0[0x08] = psxBios_OpenEvent;
	biosB0[0x09] = psxBios_CloseEvent;
	biosB0[0x0a] = psxBios_WaitEvent;
	biosB0[0x0b] = psxBios_TestEvent;
	biosB0[0x0c] = psxBios_EnableEvent;
	biosB0[0x0d] = psxBios_DisableEvent;
	biosB0[0x0e] = psxBios_OpenTh;
	biosB0[0x0f] = psxBios_CloseTh;
	biosB0[0x10] = psxBios_ChangeTh;
	//biosB0[0x11] = psxBios_psxBios_b0_11;
	biosB0[0x12] = psxBios_InitPAD;
	biosB0[0x13] = psxBios_StartPAD;
	biosB0[0x14] = psxBios_StopPAD;
	biosB0[0x15] = psxBios_PAD_init;
	biosB0[0x16] = psxBios_PAD_dr;
	biosB0[0x17] = psxBios_ReturnFromException;
	biosB0[0x18] = psxBios_ResetEntryInt;
	biosB0[0x19] = psxBios_HookEntryInt;
	//biosB0[0x1a] = psxBios_sys_b0_1a;
	//biosB0[0x1b] = psxBios_sys_b0_1b;
	//biosB0[0x1c] = psxBios_sys_b0_1c;
	//biosB0[0x1d] = psxBios_sys_b0_1d;
	//biosB0[0x1e] = psxBios_sys_b0_1e;
	//biosB0[0x1f] = psxBios_sys_b0_1f;
	biosB0[0x20] = psxBios_UnDeliverEvent;
	//biosB0[0x21] = psxBios_sys_b0_21;
	//biosB0[0x22] = psxBios_sys_b0_22;
	//biosB0[0x23] = psxBios_sys_b0_23;
	//biosB0[0x24] = psxBios_sys_b0_24;
	//biosB0[0x25] = psxBios_sys_b0_25;
	//biosB0[0x26] = psxBios_sys_b0_26;
	//biosB0[0x27] = psxBios_sys_b0_27;
	//biosB0[0x28] = psxBios_sys_b0_28;
	//biosB0[0x29] = psxBios_sys_b0_29;
	//biosB0[0x2a] = psxBios_sys_b0_2a;
	//biosB0[0x2b] = psxBios_sys_b0_2b;
	//biosB0[0x2c] = psxBios_sys_b0_2c;
	//biosB0[0x2d] = psxBios_sys_b0_2d;
	//biosB0[0x2e] = psxBios_sys_b0_2e;
	//biosB0[0x2f] = psxBios_sys_b0_2f;
	//biosB0[0x30] = psxBios_sys_b0_30;
	//biosB0[0x31] = psxBios_sys_b0_31;
	biosB0[0x32] = psxBios_open;
	biosB0[0x33] = psxBios_lseek;
	biosB0[0x34] = psxBios_read;
	biosB0[0x35] = psxBios_write;
	biosB0[0x36] = psxBios_close;
	//biosB0[0x37] = psxBios_ioctl;
	//biosB0[0x38] = psxBios_exit;
	//biosB0[0x39] = psxBios_sys_b0_39;
	biosB0[0x3a] = psxBios_getc;
	biosB0[0x3b] = psxBios_putc;
	biosB0[0x3c] = psxBios_getchar;
	biosB0[0x3d] = psxBios_putchar;
	//biosB0[0x3e] = psxBios_gets;
	biosB0[0x3f] = psxBios_puts;
	biosB0[0x40] = psxBios_cd;
	biosB0[0x41] = psxBios_format;
	biosB0[0x42] = psxBios_firstfile;
	biosB0[0x43] = psxBios_nextfile;
	biosB0[0x44] = psxBios_rename;
	biosB0[0x45] = psxBios_delete;
	//biosB0[0x46] = psxBios_undelete;
	//biosB0[0x47] = psxBios_AddDevice;
	//biosB0[0x48] = psxBios_RemoteDevice;
	//biosB0[0x49] = psxBios_PrintInstalledDevices;
	biosB0[0x4a] = psxBios_InitCARD;
	biosB0[0x4b] = psxBios_StartCARD;
	biosB0[0x4c] = psxBios_StopCARD;
	//biosB0[0x4d] = psxBios_sys_b0_4d;
	biosB0[0x4e] = psxBios__card_write;
	biosB0[0x4f] = psxBios__card_read;
	biosB0[0x50] = psxBios__new_card;
	biosB0[0x51] = psxBios_Krom2RawAdd;
	//biosB0[0x52] = psxBios_sys_b0_52;
	//biosB0[0x53] = psxBios_sys_b0_53;
	//biosB0[0x54] = psxBios__get_errno;
	biosB0[0x55] = psxBios__get_error;
	biosB0[0x56] = psxBios_GetC0Table;
	biosB0[0x57] = psxBios_GetB0Table;
	biosB0[0x58] = psxBios__card_chan;
	//biosB0[0x59] = psxBios_sys_b0_59;
	//biosB0[0x5a] = psxBios_sys_b0_5a;
	biosB0[0x5b] = psxBios_ChangeClearPad;
	biosB0[0x5c] = psxBios__card_status;
	biosB0[0x5d] = psxBios__card_wait;
//*******************C0 CALLS****************************
	biosC0[0x00] = psxBios_InitRCnt;
	biosC0[0x01] = psxBios_InitException;
	biosC0[0x02] = psxBios_SysEnqIntRP;
	biosC0[0x03] = psxBios_SysDeqIntRP;
	biosC0[0x04] = psxBios_get_free_EvCB_slot;
	//biosC0[0x05] = psxBios_get_free_TCB_slot;
	//biosC0[0x06] = psxBios_ExceptionHandler;
	//biosC0[0x07] = psxBios_InstallExeptionHandler;
	biosC0[0x08] = psxBios_SysInitMemory;
	//biosC0[0x09] = psxBios_SysInitKMem;
	biosC0[0x0a] = psxBios_ChangeClearRCnt;
	//biosC0[0x0b] = psxBios_SystemError;
	biosC0[0x0c] = psxBios_InitDefInt;
	//biosC0[0x0d] = psxBios_sys_c0_0d;
	//biosC0[0x0e] = psxBios_sys_c0_0e;
	//biosC0[0x0f] = psxBios_sys_c0_0f;
	//biosC0[0x10] = psxBios_sys_c0_10;
	//biosC0[0x11] = psxBios_sys_c0_11;
	//biosC0[0x12] = psxBios_InstallDevices;
	//biosC0[0x13] = psxBios_FlushStfInOutPut;
	//biosC0[0x14] = psxBios_sys_c0_14;
	//biosC0[0x15] = psxBios__cdevinput;
	//biosC0[0x16] = psxBios__cdevscan;
	//biosC0[0x17] = psxBios__circgetc;
	//biosC0[0x18] = psxBios__circputc;
	//biosC0[0x19] = psxBios_ioabort;
	//biosC0[0x1a] = psxBios_sys_c0_1a
	//biosC0[0x1b] = psxBios_KernelRedirect;
	//biosC0[0x1c] = psxBios_PatchAOTable;
//************** THE END ***************************************
/**/

	memset(FDesc, 0, sizeof(FDesc));
	memset(cdir, 0, sizeof(cdir));
	floodchk = 0;

	// somewhat pretend to be a SCPH1001 BIOS
	// some games look for these and take an exception if they're missing
	rom32 = (u32 *)psxR;
	rom32[0x100/4] = SWAP32(0x19951204);
	rom32[0x104/4] = SWAP32(3);
	romc = (char *)psxR;
	strcpy(romc + 0x108, "PCSX authors");
	strcpy(romc + 0x12c, "CEX-3000 PCSX HLE"); // see psxBios_GetSystemInfo
	strcpy(romc + 0x7ff32, "System ROM Version 2.2 12/04/95 A");
	strcpy(romc + 0x7ff54, "GPL-2.0-or-later");

	// fonts
	len = 0x80000 - 0x66000;
	uncompress((Bytef *)(psxR + 0x66000), &len, font_8140, sizeof(font_8140));
	len = 0x80000 - 0x69d68;
	uncompress((Bytef *)(psxR + 0x69d68), &len, font_889f, sizeof(font_889f));

	// trap attempts to call bios directly
	rom32[0x00000/4] = HLEOP(hleop_dummy);
	rom32[0x00180/4] = HLEOP(hleop_dummy);
	rom32[0x3fffc/4] = HLEOP(hleop_dummy);
	rom32[0x65ffc/4] = HLEOP(hleop_dummy);
	rom32[0x7ff2c/4] = HLEOP(hleop_dummy);

	/*	Some games like R-Types, CTR, Fade to Black read from adress 0x00000000 due to uninitialized pointers.
		See Garbage Area at Address 00000000h in Nocash PSX Specfications for more information.
		Here are some examples of games not working with this fix in place :
		R-type won't get past the Irem logo if not implemented.
		Crash Team Racing will softlock after the Sony logo.
	*/

	ram32 = (u32 *)psxM;
	ram32[0x0000/4] = SWAPu32(0x00000003); // lui   $k0, 0  (overwritten by 3)
	ram32[0x0004/4] = SWAPu32(0x275a0000 + A_EXCEPTION); // addiu $k0, $k0, 0xc80
	ram32[0x0008/4] = SWAPu32(0x03400008); // jr    $k0
	ram32[0x000c/4] = SWAPu32(0x00000000); // nop

	ram32[0x0060/4] = SWAPu32(0x00000002); // ram size?
	ram32[0x0068/4] = SWAPu32(0x000000ff); // unknown

	ram32[0x0080/4] = SWAPu32(0x3c1a0000); // lui   $k0, 0  // exception vector
	ram32[0x0084/4] = SWAPu32(0x275a0000 + A_EXCEPTION); // addiu $k0, $k0, 0xc80
	ram32[0x0088/4] = SWAPu32(0x03400008); // jr    $k0
	ram32[0x008c/4] = SWAPu32(0x00000000); // nop

	ram32[0x00a0/4] = HLEOP(hleop_a0);
	ram32[0x00b0/4] = HLEOP(hleop_b0);
	ram32[0x00c0/4] = HLEOP(hleop_c0);

	setup_tt(4, 16, 0x801fff00);
	DeliverEvent(0xf0000003, 0x0010);

	ram32[0x6ee0/4] = SWAPu32(0x0000eff0); // DCB
	strcpy((char *)&ram32[0xeff0/4], "bu");

	// default exception handler chains
	// see also setup_cd_irq_and_events()
	write_chain(&ram32[0x91e0/4], 0x91d0, 0xbfc050a4, 0xbfc04fbc); // chain0.e0
	write_chain(&ram32[0x91d0/4], 0x6da8, 0xbfc0506c, 0xbfc04dec); // chain0.e1
	write_chain(&ram32[0x6da8/4],      0,          0,     0x1a00); // chain0.e2
	write_chain(&ram32[0x6d88/4], 0x6d78,     0x19c8,     0x18bc); // chain1.e0
	write_chain(&ram32[0x6d78/4], 0x6d68,     0x1990,     0x1858); // chain1.e1
	write_chain(&ram32[0x6d68/4], 0x6d58,     0x1958,     0x17f4); // chain1.e2
	write_chain(&ram32[0x6d58/4],      0,     0x1920,     0x1794); // chain1.e3
	write_chain(&ram32[0x6d98/4],      0,          0,     0x2458); // chain3.e0

	setup_mips_code();

	// fill the api jumptables with fake entries as some games patch them
	// (or rather the funcs listed there)
	// also trap the destination as some "Cheats Edition" thing overrides the
	// dispatcher with a wrapper and then jumps to the table entries directly
	ptr = (u32 *)&psxM[A_A0_TABLE];
	for (i = 0; i < 256; i++) {
		ptr[i] = SWAP32(A_A0_TRAPS + i*4);
		ram32[A_A0_TRAPS/4 + i] = HLEOP(hleop_a0t);
	}
	ptr = (u32 *)&psxM[A_B0_TABLE];
	for (i = 0; i < 256; i++) {
		ptr[i] = SWAP32(A_B0_TRAPS + i*4);
		ram32[A_B0_TRAPS/4 + i] = HLEOP(hleop_b0t);
	}
	// B(5b) is special because games patch (sometimes even jump to)
	// code at fixed offsets from it, nocash lists offsets:
	//  patch: +3d8, +4dc, +594, +62c, +9c8, +1988
	//  call:  +7a0=4b70, +884=4c54, +894=4c64
	ptr[0x5b] = SWAP32(A_B0_5B_TRAP);     // 0x43d0
	ram32[A_B0_5B_TRAP/4] = HLEOP(hleop_b0t);

	ram32[0x4b70/4] = SWAP32(0x03e00008); // jr $ra // setPadOutputBuf

	ram32[0x4c54/4] = SWAP32(0x240e0001); // mov $t6, 1
	ram32[0x4c58/4] = SWAP32(0x03e00008); // jr $ra
	ram32[0x4c5c/4] = SWAP32(0xac0e0000 + A_PAD_IRQR_ENA); // sw $t6, ...

	ram32[0x4c64/4] = SWAP32(0x03e00008); // jr $ra
	ram32[0x4c68/4] = SWAP32(0xac000000 + A_PAD_IRQR_ENA); // sw $0, ...

	ptr = (u32 *)&psxM[A_C0_TABLE];
	for (i = 0; i < 256/2; i++) {
		ptr[i] = SWAP32(A_C0_TRAPS + i*4);
		ram32[A_C0_TRAPS/4 + i] = HLEOP(hleop_c0t);
	}
	ptr[6] = SWAP32(A_EXCEPTION);

	// more HLE traps
	ram32[A_A0_TRAPS/4 - 1] = HLEOP(hleop_dummy);
	ram32[A_B0_TRAPS/4 - 1] = HLEOP(hleop_dummy);
	ram32[A_C0_TRAPS/4 - 1] = HLEOP(hleop_dummy);
	ram32[0x7ffc/4] = HLEOP(hleop_dummy);
	ram32[0x8000/4] = HLEOP(hleop_execret);

	ram32[A_EEXIT_PTR/4] = SWAP32(A_EEXIT_DEF);
	ram32[A_EXC_SP/4] = SWAP32(A_EXC_STACK);
	ram32[A_RCNT_VBL_ACK/4 + 0] = SWAP32(1);
	ram32[A_RCNT_VBL_ACK/4 + 1] = SWAP32(1);
	ram32[A_RCNT_VBL_ACK/4 + 2] = SWAP32(1);
	ram32[A_RCNT_VBL_ACK/4 + 3] = SWAP32(1);
	ram32[A_RND_SEED/4] = SWAPu32(0x24040001); // was 0xac20cc00
}

void psxBiosShutdown() {
}

void psxBiosCnfLoaded(u32 tcb_cnt, u32 evcb_cnt, u32 stack) {
	if (stack == 0)
		stack = 0x801FFF00;
	if (tcb_cnt != 4 || evcb_cnt != 16) {
		setup_tt(tcb_cnt, evcb_cnt, stack);
		DeliverEvent(0xf0000003, 0x0010);
	}
	storeRam32(A_CONF_SP, stack);
}

#define psxBios_PADpoll(pad) { \
	int i, more_data = 0; \
	PAD##pad##_startPoll(pad); \
	pad_buf##pad[1] = PAD##pad##_poll(0x42, &more_data); \
	pad_buf##pad[0] = more_data ? 0 : 0xff; \
	PAD##pad##_poll(0, &more_data); \
	i = 2; \
	while (more_data) { \
		pad_buf##pad[i++] = PAD##pad##_poll(0, &more_data); \
	} \
}

static void handle_chain_x_x_1(u32 enable, u32 irqbit)
{
	use_cycles(10);
	if (enable) {
		psxHwWrite16(0x1f801070, ~(1u << irqbit));
		psxBios_ReturnFromException();
	}
	else
		pc0 = ra;
}

// hleExc0_{0,1}* are usually removed by A(56)/A(72) on the game's startup,
// so this is only partially implemented
static void hleExc0_0_1() // A(93h) - CdromDmaIrqFunc2
{
	u32 cdrom_dma_ack_enable = 1; // a000b93c
	handle_chain_x_x_1(cdrom_dma_ack_enable, 3); // IRQ3 DMA
}

static void hleExc0_0_2() // A(91h) - CdromDmaIrqFunc1
{
	u32 ret = 0;
	//PSXBIOS_LOG("%s\n", __func__);

	if (psxHu32(0x1074) & psxHu32(0x1070) & 8) { // IRQ3 DMA
		psxHwWrite32(0x1f8010f4, (psxHu32(0x10f4) & 0xffffff) | 0x88000000);
		//if (--cdrom_irq_counter == 0) // 0xa0009180
		//	DeliverEvent(0xf0000003, 0x10);
		use_cycles(22);
		ret = 1;
	}
	mips_return_c(ret, 20);
}

static void hleExc0_1_1() // A(92h) - CdromIoIrqFunc2
{
	u32 cdrom_irq_ack_enable = 1; // a000b938
	handle_chain_x_x_1(cdrom_irq_ack_enable, 2); // IRQ2 cdrom
}

static void hleExc0_1_2() // A(90h) - CdromIoIrqFunc1
{
	u32 ret = 0;
	if (psxHu32(0x1074) & psxHu32(0x1070) & 4) { // IRQ2 cdrom
		PSXBIOS_LOG("%s TODO\n", __func__);
		ret = 1;
	}
	mips_return_c(ret, 20);
}

static void hleExc0_2_2_syscall() // not in any A/B/C table
{
	u32 tcbPtr = loadRam32(A_TT_PCB);
	TCB *tcb = loadRam32ptr(tcbPtr);
	u32 code = (SWAP32(tcb->cause) & 0x3c) >> 2;

	if (code != R3000E_Syscall) {
		if (code != 0) {
			DeliverEvent(0xf0000010, 0x1000);
			//psxBios_SystemErrorUnresolvedException();
		}
		mips_return_c(0, 17);
		return;
	}

	//printf("%s c=%d a0=%d\n", __func__, code, SWAP32(tcb->reg[4]));
	tcb->epc += SWAP32(4);
	switch (SWAP32(tcb->reg[4])) { // a0
		case 0: // noop
			break;

		case 1: { // EnterCritical - disable irqs
			u32 was_enabled = ((SWAP32(tcb->sr) & 0x404) == 0x404);
			tcb->reg[2] = SWAP32(was_enabled);
			tcb->sr &= SWAP32(~0x404);
			break;
		}
		case 2: // ExitCritical - enable irqs
			tcb->sr |= SWAP32(0x404);
			break;

		case 3: { // ChangeThreadSubFunction
			u32 tcbPtr = loadRam32(A_TT_PCB);
			storeRam32(tcbPtr, SWAP32(tcb->reg[5])); // a1
			break;
		}
		default:
			DeliverEvent(0xf0000010, 0x4000);
			break;
	}
	use_cycles(30);
	psxBios_ReturnFromException();
}

static void hleExc1_0_1(void)
{
	u32 vbl_irq_ack_enable = loadRam32(A_RCNT_VBL_ACK + 0x0c); // 860c
	handle_chain_x_x_1(vbl_irq_ack_enable, 0); // IRQ0 vblank
}

static void handle_chain_1_x_2(u32 ev_index, u32 irqbit)
{
	u32 ret = 0;
	if (psxHu32(0x1074) & psxHu32(0x1070) & (1u << irqbit)) {
		DeliverEvent(0xf2000000 + ev_index, 0x0002);
		ret = 1;
	}
	mips_return_c(ret, 22);
}

static void hleExc1_0_2(void)
{
	handle_chain_1_x_2(3, 0); // IRQ0 vblank
}

static void hleExc1_1_1(void)
{
	u32 rcnt_irq_ack_enable = loadRam32(A_RCNT_VBL_ACK + 0x08); // 8608
	handle_chain_x_x_1(rcnt_irq_ack_enable, 6); // IRQ6 rcnt2
}

static void hleExc1_1_2(void)
{
	handle_chain_1_x_2(2, 6); // IRQ6 rcnt2
}

static void hleExc1_2_1(void)
{
	u32 rcnt_irq_ack_enable = loadRam32(A_RCNT_VBL_ACK + 0x04); // 8604
	handle_chain_x_x_1(rcnt_irq_ack_enable, 5); // IRQ5 rcnt1
}

static void hleExc1_2_2(void)
{
	handle_chain_1_x_2(1, 5); // IRQ5 rcnt1
}

static void hleExc1_3_1(void)
{
	u32 rcnt_irq_ack_enable = loadRam32(A_RCNT_VBL_ACK + 0x00); // 8600
	handle_chain_x_x_1(rcnt_irq_ack_enable, 4); // IRQ4 rcnt0
}

static void hleExc1_3_2(void)
{
	handle_chain_1_x_2(0, 4); // IRQ4 rcnt0
}

static void hleExc3_0_2_defint(void)
{
	static const struct {
		u8 ev, irqbit;
	} tab[] = {
		{  3,  2 }, // cdrom
		{  9,  9 }, // spu
		{  2,  1 }, // gpu
		{ 10, 10 }, // io
		{ 11,  8 }, // sio
		{  1,  0 }, // vbl
		{  5,  4 }, // rcnt0
		{  6,  5 }, // rcnt1
		{  6,  6 }, // rcnt2 (bug)
		{  8,  7 }, // sio rx
		{  4,  3 }, // sio
	};
	size_t i;
	for (i = 0; i < sizeof(tab) / sizeof(tab[0]); i++) {
		if (psxHu32(0x1074) & psxHu32(0x1070) & (1u << tab[i].irqbit)) {
			DeliverEvent(0xf0000000 + tab[i].ev, 0x1000);
			use_cycles(7);
		}

	}
	mips_return_c(0, 11 + 7*11 + 7*11 + 12);
}

static void hleExcPadCard1(void)
{
	if (loadRam32(A_PAD_IRQR_ENA)) {
		u8 *pad_buf1 = loadRam8ptr(A_PAD_INBUF + 0);
		u8 *pad_buf2 = loadRam8ptr(A_PAD_INBUF + 4);

		psxBios_PADpoll(1);
		psxBios_PADpoll(2);
		use_cycles(100);
		if (loadRam32(A_PAD_DR_DST))
			psxBios_PAD_dr_();
	}
	if (loadRam32(A_PAD_ACK_VBL))
		psxHwWrite16(0x1f801070, ~1);
	if (loadRam32(A_CARD_IRQR_ENA))
		card_vint_handler();

	mips_return_c(0, 18);
}

static void hleExcPadCard2(void)
{
	u32 ret = psxHu32(0x1074) & psxHu32(0x1070) & 1;
	mips_return_c(ret, 15);
}

void psxBiosException() {
	u32 tcbPtr = loadRam32(A_TT_PCB);
	u32 *chains = loadRam32ptr(A_TT_ExCB);
	TCB *tcb = loadRam32ptr(tcbPtr);
	u32 ptr, *chain;
	int c, lim;
	int i;

	// save the regs
	// $at, $v0, $v1, $ra already saved by the mips code at A_EXCEPTION
	for (i = 4; i < 31; i++) {
		if (i == 26) // $k0
			continue;
		tcb->reg[i] = SWAP32(psxRegs.GPR.r[i]);
	}
	tcb->lo = SWAP32(psxRegs.GPR.n.lo);
	tcb->hi = SWAP32(psxRegs.GPR.n.hi);
	//tcb->epc = SWAP32(psxRegs.CP0.n.EPC); // done by asm
	tcb->sr = SWAP32(psxRegs.CP0.n.SR);
	tcb->cause = SWAP32(psxRegs.CP0.n.Cause);
	sp = fp = loadRam32(A_EXC_SP);
	gp = A_EXC_GP;
	use_cycles(46);
	assert(!psxRegs.cpuInRecursion);

	// do the chains (always 4)
	for (c = lim = 0; c < 4; c++) {
		if (chains[c * 2] == 0)
			continue;
		ptr = SWAP32(chains[c * 2]);
		for (; ptr && lim < 100; ptr = SWAP32(chain[0])) {
			chain = castRam32ptr(ptr);
			use_cycles(14);
			lim++;
			if (chain[2] == 0)
				continue;
			softCallInException(SWAP32(chain[2]));
			if (returned_from_exception())
				return;

			if (v0 == 0 || chain[1] == 0)
				continue;
			softCallInException(SWAP32(chain[1]));
			if (returned_from_exception())
				return;
		}
	}
	assert(lim < 100);

	// return from exception (custom or default)
	use_cycles(23);
	ptr = loadRam32(A_EEXIT_PTR);
	if (ptr != A_EEXIT_DEF) {
		const struct jmp_buf_ *jmp_buf = castRam32ptr(ptr);
		longjmp_load(jmp_buf);
		v0 = 1;
		pc0 = ra;
		return;
	}
	psxBios_ReturnFromException();
}

/* HLE */
static void hleDummy() {
	log_unhandled("hleDummy called @%08x ra=%08x\n", psxRegs.pc - 4, ra);
	psxRegs.pc = ra;
	psxRegs.cycle += 1000;

	psxBranchTest();
}

static void hleA0() {
	u32 call = t1 & 0xff;
	u32 entry = loadRam32(A_A0_TABLE + call * 4);

	use_cycles(4+7);
	if (call < 192 && entry != A_A0_TRAPS + call * 4) {
		PSXBIOS_LOG("custom A%02x %s(0x%x, )  addr=%08x ra=%08x\n",
			call, biosA0n[call], a0, entry, ra);
		softCall(entry);
		pc0 = ra;
		PSXBIOS_LOG(" -> %08x\n", v0);
	}
	else if (biosA0[call])
		biosA0[call]();

	//printf("A(%02x) -> %x\n", call, v0);
	psxBranchTest();
}

static void hleB0() {
	u32 call = t1 & 0xff;
	u32 entry = loadRam32(A_B0_TABLE + call * 4);
	int is_custom = 0;

	use_cycles(4+7);
	if (call == 0x5b)
		is_custom = entry != A_B0_5B_TRAP;
	else
		is_custom = entry != A_B0_TRAPS + call * 4;
	if (is_custom) {
		PSXBIOS_LOG("custom B%02x %s(0x%x, )  addr=%08x ra=%08x\n",
			call, biosB0n[call], a0, entry, ra);
		softCall(entry);
		pc0 = ra;
		PSXBIOS_LOG(" -> %08x\n", v0);
	}
	else if (biosB0[call])
		biosB0[call]();

	//printf("B(%02x) -> %x\n", call, v0);
	psxBranchTest();
}

static void hleC0() {
	u32 call = t1 & 0xff;
	u32 entry = loadRam32(A_C0_TABLE + call * 4);

	use_cycles(4+7);
	if (call < 128 && entry != A_C0_TRAPS + call * 4) {
		PSXBIOS_LOG("custom C%02x %s(0x%x, )  addr=%08x ra=%08x\n",
			call, biosC0n[call], a0, entry, ra);
		softCall(entry);
		pc0 = ra;
		PSXBIOS_LOG(" -> %08x\n", v0);
	}
	else if (biosC0[call])
		biosC0[call]();

	//printf("C(%02x) -> %x\n", call, v0);
	psxBranchTest();
}

static void hleA0t() {
	u32 call = (pc0 - A_A0_TRAPS) / 4 - 1;
	if (call >= 256u || !biosA0[call]) {
		log_unhandled("unexpected A trap @%08x ra=%08x\n", pc0 - 4, ra);
		mips_return_void_c(1000);
	}
	else
		biosA0[call]();

	//printf("A(%02x) -> %x\n", call, v0);
	psxBranchTest();
}

static void hleB0t() {
	u32 call = (pc0 - A_B0_TRAPS) / 4 - 1;
	if (pc0 - 4 == A_B0_5B_TRAP)
		call = 0x5b;
	if (call >= 256u || !biosB0[call]) {
		log_unhandled("unexpected B trap @%08x ra=%08x\n", pc0 - 4, ra);
		mips_return_void_c(1000);
	}
	else
		biosB0[call]();

	//printf("B(%02x) -> %x\n", call, v0);
	psxBranchTest();
}

static void hleC0t() {
	u32 call = (pc0 - A_C0_TRAPS) / 4 - 1;
	if (call >= 128u || !biosC0[call]) {
		log_unhandled("unexpected C trap @%08x ra=%08x\n", pc0 - 4, ra);
		mips_return_void_c(1000);
	}
	else
		biosC0[call]();

	//printf("C(%02x) -> %x\n", call, v0);
	psxBranchTest();
}

// currently not used
static void hleBootstrap() {
	CheckCdrom();
	LoadCdrom();
}

static void hleExecRet() {
	const EXEC *header = (EXEC *)PSXM(s0);

	PSXBIOS_LOG("ExecRet %x: %x\n", s0, header->ret);

	ra = SWAP32(header->ret);
	sp = SWAP32(header->_sp);
	fp = SWAP32(header->_fp);
	gp = SWAP32(header->_gp);
	s0 = SWAP32(header->base);

	v0 = 1;
	psxRegs.pc = ra;
}

void (* const psxHLEt[hleop_count_])() = {
	hleDummy, hleA0, hleB0, hleC0,
	hleBootstrap, hleExecRet, psxBiosException, hleDummy,
	hleExc0_0_1, hleExc0_0_2,
	hleExc0_1_1, hleExc0_1_2, hleExc0_2_2_syscall,
	hleExc1_0_1, hleExc1_0_2,
	hleExc1_1_1, hleExc1_1_2,
	hleExc1_2_1, hleExc1_2_2,
	hleExc1_3_1, hleExc1_3_2,
	hleExc3_0_2_defint,
	hleExcPadCard1, hleExcPadCard2,
	hleA0t, hleB0t, hleC0t,
};

void psxBiosCheckExe(u32 t_addr, u32 t_size, int loading_state)
{
	// lw      $v0, 0x10($sp)
	// nop
	// addiu   $v0, -1
	// sw      $v0, 0x10($sp)
	// lw      $v0, 0x10($sp)
	// nop
	// bne     $v0, $v1, not_timeout
	// nop
	// lui     $a0, ...
	static const u8 pattern[] = {
		0x10, 0x00, 0xA2, 0x8F, 0x00, 0x00, 0x00, 0x00,
		0xFF, 0xFF, 0x42, 0x24, 0x10, 0x00, 0xA2, 0xAF,
		0x10, 0x00, 0xA2, 0x8F, 0x00, 0x00, 0x00, 0x00,
		0x0C, 0x00, 0x43, 0x14, 0x00, 0x00, 0x00, 0x00,
	};
	u32 start = t_addr & 0x1ffffc;
	u32 end = (start + t_size) & 0x1ffffc;
	u32 buf[sizeof(pattern) / sizeof(u32)];
	const u32 *r32 = (u32 *)(psxM + start);
	u32 i, j;

	if (end <= start)
		return;
	if (!Config.HLE)
		return;

	memcpy(buf, pattern, sizeof(buf));
	for (i = 0; i < t_size / 4; i += j + 1) {
		for (j = 0; j < sizeof(buf) / sizeof(buf[0]); j++)
			if (r32[i + j] != buf[j])
				break;
		if (j != sizeof(buf) / sizeof(buf[0]))
			continue;

		if ((SWAP32(r32[i + j]) >> 16) != 0x3c04) // lui
			continue;
		if (!loading_state)
			SysPrintf("HLE vsync @%08x\n", start + i * 4);
		psxRegs.biosBranchCheck = (t_addr & 0xa01ffffc) + i * 4;
	}
}

void psxBiosCheckBranch(void)
{
#if 1
	// vsync HLE hack
	static u32 cycles_prev, v0_prev;
	u32 cycles_passed, waste_cycles;
	u32 loops, v0_expect = v0_prev - 1;
	if (v0 != 1)
		return;
	execI(&psxRegs);
	cycles_passed = psxRegs.cycle - cycles_prev;
	cycles_prev = psxRegs.cycle;
	v0_prev = v0;
	if (cycles_passed < 10 || cycles_passed > 50 || v0 != v0_expect)
		return;

	waste_cycles = schedule_timeslice(&psxRegs) - psxRegs.cycle;
	loops = waste_cycles / cycles_passed;
	if (loops > v0)
		loops = v0;
	v0 -= loops;
	psxRegs.cycle += loops * cycles_passed;
	//printf("c %4u %d\n", loops, cycles_passed);
#endif
}

#define bfreeze(ptr, size) { \
	if (Mode == 1) memcpy(&psxR[base], ptr, size); \
	if (Mode == 0) memcpy(ptr, &psxR[base], size); \
	base += size; \
}

#define bfreezes(ptr) bfreeze(ptr, sizeof(ptr))
#define bfreezel(ptr) bfreeze(ptr, sizeof(*(ptr)))

void psxBiosFreeze(int Mode) {
	u32 base = 0x40000;

	bfreezes(FDesc);
	bfreezes(ffile);
	bfreezel(&nfile);
	bfreezes(cdir);
	bfreezel(&card_io_delay);
}
