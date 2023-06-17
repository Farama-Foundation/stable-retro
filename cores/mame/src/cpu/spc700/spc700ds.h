#ifndef HEADER__SPC700DS
#define HEADER__SPC700DS
/* ======================================================================== */
/* =============================== COPYRIGHT ============================== */
/* ======================================================================== */
/*

Sony SPC700 CPU Emulator V1.0

Copyright (c) 2000 Karl Stenerud
All rights reserved.

Permission is granted to use this source code for non-commercial purposes.
To use this code for commercial purposes, you must get permission from the
author (Karl Stenerud) at karl@higashiyama-unet.ocn.ne.jp.


*/

int spc700_disassemble(char* buff, unsigned int pc);

unsigned int spc700_read_8_disassembler(unsigned int address);

#include "cpuintrf.h"
#include "memory.h"
#include "driver.h"
#include "state.h"
#include "mamedbg.h"
#define spc700_read_8_disassembler(addr)				cpu_readmem16(addr)


#endif /* HEADER__SPC700DS */
