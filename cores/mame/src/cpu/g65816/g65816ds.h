#ifndef HEADER__G65816DS
#define HEADER__G65816DS
/* ======================================================================== */
/* =============================== COPYRIGHT ============================== */
/* ======================================================================== */
/*

G65C816 CPU Emulator V0.92

Copyright (c) 2000 Karl Stenerud
All rights reserved.

Permission is granted to use this source code for non-commercial purposes.
To use this code for commercial purposes, you must get permission from the
author (Karl Stenerud) at karl@higashiyama-unet.ocn.ne.jp.


*/

int g65816_disassemble(char* buff, unsigned int pc, unsigned int pb, int m_flag, int x_flag);

unsigned int g65816_read_8_disassembler(unsigned int address);

#include "cpuintrf.h"
#include "memory.h"
#include "driver.h"
#include "state.h"
#include "mamedbg.h"
#define g65816_read_8_disassembler(addr)				cpu_readmem24(addr)


#endif /* HEADER__G65816DS */
