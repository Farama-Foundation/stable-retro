/*
 * Copyright (c) 2015, Marcos Medeiros
 * Licensed under BSD 3-clause.
 */
#include "tms34010.h"
#include "tms34010_memacc.h"
#include "tms34010_ctrl.h"
#include "tms34010_mov.h"
#include "tms34010_arithm.h"
#include "tms34010_jump.h"
#include "tms34010_shift.h"
#include "tms34010_gfx.h"
#include "stddef.h"
#include "tiles_generic.h"

#ifdef TMS34010_DEBUGGER
#include <algorithm>
#include <iterator>
#include <QDebug>
#endif

namespace tms {


#define IO(reg) cpu->io_regs[reg]

const char *io_regs_names[32] = {
    "HESYNC", "HEBLNK", "HSBLNK", "HTOTAL", "VESYNC", "VEBLNK", "VSBLNK",
    "VTOTAL", "DPYCTL", "DPYSTRT", "DPYINT", "CONTROL", "HSTDATA", "HSTADRL",
    "HSTADRH", "HSTCTLL", "HSTCTLH", "INTENB", "INTPEND", "CONVSP", "CONVDP",
    "PSIZE", "PMASK", "RS_70", "RS_80", "RS_90", "RS_A0",
    "DPYTAP", "HCOUNT", "VCOUNT", "DPYADR", "REFCNT"
};

void scan(cpu_state *cpu, sdword nAction)
{
	if (nAction & ACB_DRIVER_DATA) {
		struct BurnArea ba;
		memset(&ba, 0, sizeof(ba));
		ba.Data	  = cpu;
		ba.nLen	  = STRUCT_SIZE_HELPER(cpu_state, shiftreg);
		ba.szName = "TMS34010 Regs";
		BurnAcb(&ba);
	}
}

void reset(cpu_state *cpu)
{
	cpu->pc = rdfield_32(VECT_RESET);
	cpu->icounter = 0;
	cpu->total_cycles = 0;
    cpu->st = 0x00000010;
    for (int i = 0; i < 15; i++) {
        cpu->a[i].value = 0;
        cpu->b[i].value = 0;
    }
    for (int i = 0; i < 32; i++) {
		cpu->io_regs[i] = 0;
    }

	memset(cpu->shiftreg, 0, 4096*2);

	cpu->timer_active = 0;
	cpu->timer_cyc = 0;
}

static void check_irq(cpu_state *cpu)
{
	if (cpu->io_regs[HSTCTLH] & 0x0100) {
		cpu->io_regs[HSTCTLH] &= ~0x0100;

		if (~cpu->io_regs[HSTCTLH] & 0x0200) {
			_push(_pc);
			_push(_st);
		}

		_st = 0x10;
		_pc = mem_read_d(0xfffffee0);
		CONSUME_CYCLES(16);
		return;
	}

	int irq = cpu->io_regs[INTPEND] & cpu->io_regs[INTENB];
    if (!(_st & ST_IE) || !irq)
        return;

    dword trap_addr = 0;
	if (irq & INTERRUPT_HOST) {
        trap_addr = 0xFFFFFEC0;
    }
    else if (irq & INTERRUPT_DISPLAY) {
        trap_addr = 0xFFFFFEA0;
    }
    else if (irq & INTERRUPT_EXTERN_1) {
        trap_addr = 0xFFFFFFC0;
    }
    else if (irq & INTERRUPT_EXTERN_2) {
        trap_addr = 0xFFFFFFA0;
    }
    if (trap_addr) {
        _push(_pc);
        _push(_st);
        _st = 0x10;
        _pc = mem_read_d(trap_addr) & 0xFFFFFFF0;
		CONSUME_CYCLES(16);
    }
}

void check_timer(cpu_state *cpu, int cyc)
{
	if (cpu->timer_active) {
		cpu->timer_cyc -= cyc;
		if (cpu->timer_cyc <= 0) {
			//bprintf(0, _T("timer hit @ %I64d\n"), TMS34010TotalCycles());
			cpu->timer_active = 0;
			cpu->timer_cyc = 0;

			if (cpu->timer_cb)
				cpu->timer_cb();
		}
	}
}

void timer_set_cb(cpu_state *cpu, void (*t_cb)())
{
	cpu->timer_cb = t_cb;
}

void timer_arm(cpu_state *cpu, int cycle)
{
	//bprintf(0, _T("timer arm @ %d   time now %I64d\n"), cycle, TMS34010TotalCycles());
	if (cpu->timer_active) {
		bprintf(0, _T("TMS34010: timer_arm() arm timer when timer pending!\n"));
	}
	cpu->timer_active = 1;
	cpu->timer_cyc = cycle;
}

#ifdef TMS34010_DEBUGGER
static void perform_trace(cpu_state *cpu)
{
    int pc_count = std::count(std::begin(cpu->history), std::end(cpu->history), cpu->pc);
    if (pc_count) {
        cpu->loop_counter++;
        return;
    }

    if (cpu->loop_counter) {
        cpu->trace << "\n\t(Loop for " << cpu->loop_counter << " instructions)\n\n";
    }
    cpu->loop_counter = 0;

    char pcbuf[16];
    snprintf(pcbuf, 16, "%08X\t", _pc);
    cpu->trace << pcbuf << new_dasm(_pc, nullptr) << std::endl;

    cpu->history[cpu->history_idx] = cpu->pc;
    cpu->history_idx = (cpu->history_idx + 1) % cpu->history.size();
}

void run(cpu_state *cpu, int cycles, bool stepping)
{
	cpu->cycles_start = cycles;
	cpu->icounter = cycles;
	cpu->stop = 0;

	if (cpu->io_regs[HSTCTLH] & 0x8000) {
		// halt
		cpu->icounter = 0;
		cpu->stop = 1;
	}

	check_timer(cpu, 0);

	while (cpu->icounter > 0 && !cpu->stop) {

        check_irq(cpu);

        if (!stepping) {
            if (std::find(std::begin(cpu->bpoints), std::end(cpu->bpoints), _pc) !=
                std::end(cpu->bpoints)) {
                cpu->reason = BREAKPOINT_FOUND;
                return;
            }
        }

        if (cpu->tracing && cpu->trace.is_open()) {
            perform_trace(cpu);
        }

        cpu->pc &= 0xFFFFFFF0;

        word opcode = mem_read(cpu->pc);
        cpu->last_pc = cpu->pc;
        cpu->pc += 16;
        opcode_table[(opcode >> 4) & 0xFFF](cpu, opcode);
        if (cpu->icounter == 0)
            return;
    }
    cpu->reason = ICOUNTER_EXPIRED;

	cycles = cycles - cpu->icounter;
	cpu->total_cycles += cycles;
	cpu->cycles_start = cpu->icounter = 0;
}

#else
int run(cpu_state *cpu, int cycles)
{
	cpu->cycles_start = cycles;
	cpu->icounter = cycles;
	cpu->stop = 0;
	if (cpu->io_regs[HSTCTLH] & 0x8000) {
		// halt
		cpu->icounter = 0;
		cpu->stop = 1;
	}

	check_timer(cpu, 0);

	while (cpu->icounter > 0 && !cpu->stop) {

        check_irq(cpu);
        cpu->pc &= 0xFFFFFFF0;
        word opcode = mem_read(cpu->pc);
        cpu->last_pc = cpu->pc;
        cpu->pc += 16;
		opcode_table[(opcode >> 4) & 0xFFF](cpu, opcode);
	}

	cycles = cycles - cpu->icounter;
	cpu->total_cycles += cycles;
	cpu->cycles_start = cpu->icounter = 0;

	return cycles;
}
#endif

void stop(cpu_state *cpu)
{
	cpu->stop = 1;
}

i64 total_cycles(cpu_state *cpu)
{
	return cpu->total_cycles + (cpu->cycles_start - cpu->icounter);
}

void new_frame(cpu_state *cpu)
{
	cpu->total_cycles = 0;
}

dword get_pc(cpu_state *cpu)
{
	return cpu->pc;
}

dword get_ppc(cpu_state *cpu)
{
	return cpu->last_pc;
}

void generate_irq(cpu_state *cpu, int num)
{
    cpu->io_regs[INTPEND] |= num;
}

void clear_irq(cpu_state *cpu, int num)
{
    cpu->io_regs[INTPEND] &= ~num;
}

void write_ioreg(cpu_state *cpu, dword addr, word value)
{
    const int reg = (addr >> 4) & 0x1F;
    cpu->io_regs[reg] = value;
    switch (reg) {
    case PSIZE:  cpu->pshift = log2((double)value); break;
    case CONVDP: cpu->convdp = 1 << (~value & 0x1F); break;
    case CONVSP: cpu->convsp = 1 << (~value & 0x1F); break;
    case INTPEND:
		if (!(value & INTERRUPT_DISPLAY)) {
			//bprintf(0, _T("clear VBL irq @ %I64d\n"), TMS34010TotalCycles());
			cpu->io_regs[INTPEND] &= ~INTERRUPT_DISPLAY;
		}
        break;
    case DPYSTRT:
		break;
	case HSTCTLL:
		//bprintf(0,_T("hstctll %x\n"), value);
		break;
    case VTOTAL:
    case HTOTAL:
    case HSBLNK:
    case HEBLNK:
    case VSBLNK:
    case VEBLNK:
#ifdef TMS34010_DEBUGGER
        qDebug() << QString().sprintf("vga crtc %dx%d - v:%d,h:%d",
            cpu->io_regs[HSBLNK]-cpu->io_regs[HEBLNK],
            cpu->io_regs[VSBLNK]-cpu->io_regs[VEBLNK],
            cpu->io_regs[VTOTAL],
            cpu->io_regs[HTOTAL]);
#endif
        break;
    }

}

dword read_ioreg(cpu_state *cpu, dword addr)
{
	int reg = (addr >> 4) & 0x1F;
	switch(reg) {
		case HCOUNT: {
			INT32 hc = TMS34010TotalCycles() % nScreenWidth;
			INT32 ht = cpu->io_regs[HTOTAL] + 1;
			hc = hc * ht / nScreenWidth;
			hc += cpu->io_regs[HEBLNK];
			return (hc > ht) ? (hc - ht) : hc;
		}
		case REFCNT:
			return (TMS34010TotalCycles() / 0x10) & 0xfffc;
	}

    return cpu->io_regs[reg];
}

int generate_scanline(cpu_state *cpu, int line, scanline_render_t render)
{
/*	if (line==0) {
		bprintf(0, _T("vtotal %X   veblnk %X   vsblnk %X.  dpyint %X (enable: %X)\n"), cpu->io_regs[VTOTAL], cpu->io_regs[VEBLNK], cpu->io_regs[VSBLNK], cpu->io_regs[DPYINT], cpu->io_regs[DPYCTL] & 0x8000);
	}
*/
	int enabled = cpu->io_regs[DPYCTL] & 0x8000;
    cpu->io_regs[VCOUNT] = line;

    if (enabled && line == cpu->io_regs[DPYINT]) {
		generate_irq(cpu, INTERRUPT_DISPLAY);
		//bprintf(0, _T("DO VBL irq @ %I64d\n"), TMS34010TotalCycles());
    }

    if (line == cpu->io_regs[VSBLNK]) {
        cpu->io_regs[DPYADR] = cpu->io_regs[DPYSTRT];
    }

    if (line >= cpu->io_regs[VEBLNK] && line <= cpu->io_regs[VSBLNK]) {
        word dpyadr = cpu->io_regs[DPYADR];
        if (!(cpu->io_regs[DPYCTL] & 0x0400))
            dpyadr ^= 0xFFFC;

        int rowaddr = dpyadr >> 4;
        int coladdr = ((dpyadr & 0x007C) << 4) | (cpu->io_regs[DPYTAP] & 0x3FFF);

        display_info info;
        info.coladdr = coladdr;
        info.rowaddr = rowaddr;
        info.heblnk = cpu->io_regs[HEBLNK];
        info.hsblnk = cpu->io_regs[HSBLNK];
        info.htotal = cpu->io_regs[HTOTAL];
        if (render) {
            render(line, &info);
        }
    }

    if (line >= cpu->io_regs[VEBLNK] && line < cpu->io_regs[VSBLNK]) {
        word dpyadr = cpu->io_regs[DPYADR];
        if ((dpyadr & 3) == 0) {
            dpyadr = ((dpyadr & 0xFFFC) - (cpu->io_regs[DPYCTL] & 0x03FC));
            dpyadr |= (cpu->io_regs[DPYSTRT] & 0x0003);
        } else {
            dpyadr = (dpyadr & 0xfffc) | ((dpyadr - 1) & 3);
        }
        cpu->io_regs[DPYADR] = dpyadr;
    }
    if (++line >= cpu->io_regs[VTOTAL])
        line = 0;
    return line;
}

wrfield_handler wrfield_table[32] = {
    &wrfield_32, &wrfield_1,  &wrfield_2,  &wrfield_3,  &wrfield_4,  &wrfield_5,  &wrfield_6,  &wrfield_7,
    &wrfield_8,  &wrfield_9,  &wrfield_10, &wrfield_11, &wrfield_12, &wrfield_13, &wrfield_14, &wrfield_15,
    &wrfield_16, &wrfield_17, &wrfield_18, &wrfield_19, &wrfield_20, &wrfield_21, &wrfield_22, &wrfield_23,
    &wrfield_24, &wrfield_25, &wrfield_26, &wrfield_27, &wrfield_28, &wrfield_29, &wrfield_30, &wrfield_31,
};

rdfield_handler rdfield_table[64] = {
    &rdfield_32,    &rdfield_1,     &rdfield_2,     &rdfield_3,     &rdfield_4,     &rdfield_5,     &rdfield_6,     &rdfield_7,
    &rdfield_8,     &rdfield_9,     &rdfield_10,    &rdfield_11,    &rdfield_12,    &rdfield_13,    &rdfield_14,    &rdfield_15,
    &rdfield_16,    &rdfield_17,    &rdfield_18,    &rdfield_19,    &rdfield_20,    &rdfield_21,    &rdfield_22,    &rdfield_23,
    &rdfield_24,    &rdfield_25,    &rdfield_26,    &rdfield_27,    &rdfield_28,    &rdfield_29,    &rdfield_30,    &rdfield_31,
    &rdfield_32,    &rdfield_1_sx,  &rdfield_2_sx,  &rdfield_3_sx,  &rdfield_4_sx,  &rdfield_5_sx,  &rdfield_6_sx,  &rdfield_7_sx,
    &rdfield_8_sx,  &rdfield_9_sx,  &rdfield_10_sx, &rdfield_11_sx, &rdfield_12_sx, &rdfield_13_sx, &rdfield_14_sx, &rdfield_15_sx,
    &rdfield_16_sx, &rdfield_17_sx, &rdfield_18_sx, &rdfield_19_sx, &rdfield_20_sx, &rdfield_21_sx, &rdfield_22_sx, &rdfield_23_sx,
    &rdfield_24_sx, &rdfield_25_sx, &rdfield_26_sx, &rdfield_27_sx, &rdfield_28_sx, &rdfield_29_sx, &rdfield_30_sx, &rdfield_31_sx,
};




}
