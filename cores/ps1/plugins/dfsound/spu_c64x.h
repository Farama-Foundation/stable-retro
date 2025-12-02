#ifndef __P_SPU_C64X_H__
#define __P_SPU_C64X_H__

#include "externals.h"
#include "spu_config.h"

#define COMPONENT_NAME "pcsxr_spu"

enum {
 CCMD_INIT = 0x101,
 CCMD_DOIT = 0x102,
};

struct region_mem {
 unsigned char spu_ram[512 * 1024];
 sample_buf sb_thread[MAXCHAN+1];
 // careful not to lose ARM writes by DSP overwriting
 // with old data when it's writing out neighbor cachelines
 int _pad1[128/4 - ((sizeof(sample_buf) * (MAXCHAN+1) / 4) & (128/4 - 1))];
 struct spu_in {
  // these are not to be modified by DSP
  SPUCHAN s_chan[24 + 1];
  REVERBInfo rvb;
  SPUConfig spu_config;
 } in;
 int _pad2[128/4 - ((sizeof(struct spu_in) / 4) & (128/4 - 1))];
 struct spu_worker worker;
 // init/debug
 int sizeof_region_mem;
 int offsetof_s_chan1;
 int offsetof_spos_3_20;
};

#define ACTIVE_CNT 3

#endif /* __P_SPU_C64X_H__ */
