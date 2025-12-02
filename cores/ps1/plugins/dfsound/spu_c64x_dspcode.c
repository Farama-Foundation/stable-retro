/*
 * SPU processing offload to TI C64x DSP using bsp's c64_tools
 * (C) Gražvydas "notaz" Ignotas, 2015
 *
 *  Permission is hereby granted, free of charge, to any person obtaining a copy of
 *  this software and associated documentation files (the "Software"), to deal in
 *  the Software without restriction, including without limitation the rights to
 *  use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
 *  of the Software, and to permit persons to whom the Software is furnished to do
 *  so, subject to the following conditions:
 *
 *  The above copyright notice and this permission notice shall be included in all
 *  copies or substantial portions of the Software.
 *
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 *  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 *  SOFTWARE.
 */

#define SYSCALLS_C
#include <libc64_dsp/include/inc_overlay.h>
#include <stddef.h>

#include "spu.c"
#include "spu_c64x.h"

/* dummy deps, some bloat but avoids ifdef hell in SPU code.. */
static void thread_work_start(void) {}
static void thread_work_wait_sync(struct work_item *work, int force) {}
static void thread_sync_caches(void) {}
static int  thread_get_i_done(void) { return 0; }
struct out_driver *out_current;
void SetupSound(void) {}


static void enable_l2_cache(void)
{
 volatile uint32_t *L2CFG = (volatile uint32_t *)0x01840000;
 uint32_t *MARi = (void *)0x01848000;
 int i;

 // program Memory Attribute Registers
 // (old c64_tools has the defaults messed up)
 // 00000000-0fffffff - not configurable
 // 10000000-7fffffff - system
 for (i = 0x10; i < 0x80; i++)
  MARi[i] = 0;
 // 80000000-9fffffff - RAM
 for ( ; i < 0xa0; i++)
  MARi[i] = 1;
 // 0xa00000-ffffffff - reserved, etc
 for ( ; i < 0x100; i++)
  MARi[i] = 0;

 // enable L2 (1 for 32k, 2 for 64k)
 if (!(*L2CFG & 2)) {
  *L2CFG = 2;
  // wait the for the write
  *L2CFG;
 }
}

static void invalidate_cache(struct work_item *work)
{
 // see comment in writeout_cache()
 //syscalls.cache_inv(work, offsetof(typeof(*work), SSumLR), 1);
 syscalls.cache_inv(spu.s_chan, sizeof(spu.s_chan[0]) * 24, 1);
 syscalls.cache_inv(work->SSumLR,
   sizeof(work->SSumLR[0]) * 2 * work->ns_to, 1);
}

static void writeout_cache(struct work_item *work)
{
 int ns_to = work->ns_to;

 syscalls.cache_wb(work->SSumLR, sizeof(work->SSumLR[0]) * 2 * ns_to, 1);
 // have to invalidate now, otherwise there is a race between
 // DSP evicting dirty lines and ARM writing new data to this area
 syscalls.cache_inv(work, offsetof(typeof(*work), SSumLR), 1);
}

static void do_processing(void)
{
 int left, dirty = 0, had_rvb = 0;
 struct work_item *work;

 while (worker->active)
 {
  // i_ready is in first cacheline
  syscalls.cache_inv(worker, 64, 1);

  left = worker->i_ready - worker->i_done;
  if (left > 0) {
   dirty = 1;
   worker->active = ACTIVE_CNT;
   syscalls.cache_wb(&worker->active, 4, 1);

   work = &worker->i[worker->i_done & WORK_I_MASK];
   invalidate_cache(work);
   had_rvb |= work->rvb_addr;
   spu.spuCtrl = work->ctrl;
   do_channel_work(work);
   writeout_cache(work);

   worker->i_done++;
   syscalls.cache_wb(&worker->i_done, 4, 1);
   continue;
  }

  // nothing to do? Write out non-critical caches
  if (dirty) {
   syscalls.cache_wb(spu.spuMemC + 0x800, 0x800, 1);
   syscalls.cache_wb(spu.sb_thread, sizeof(spu.sb_thread_), 1);
   if (had_rvb) {
    left = 0x40000 - spu.rvb->StartAddr;
    syscalls.cache_wb(spu.spuMem + spu.rvb->StartAddr, left * 2, 1);
    had_rvb = 0;
   }
   dirty = 0;
   continue;
  }

  // this ->active loop thing is to avoid a race where we miss
  // new work and clear ->active just after ARM checks it
  worker->active--;
  syscalls.cache_wb(&worker->active, 4, 1);
 }
}

static unsigned int exec(dsp_component_cmd_t cmd,
  unsigned int arg1, unsigned int arg2,
  unsigned int *ret1, unsigned int *ret2)
{
 struct region_mem *mem = (void *)arg1;

 switch (cmd) {
  case CCMD_INIT:
   enable_l2_cache();
   InitADSR();

   spu.spuMemC = mem->spu_ram;
   spu.sb_thread = mem->sb_thread;
   spu.s_chan = mem->in.s_chan;
   spu.rvb = &mem->in.rvb;
   worker = &mem->worker;
   memcpy(&spu_config, &mem->in.spu_config, sizeof(spu_config));

   mem->sizeof_region_mem = sizeof(*mem);
   mem->offsetof_s_chan1 = offsetof(typeof(*mem), in.s_chan[1]);
   mem->offsetof_spos_3_20 = offsetof(typeof(*mem), worker.i[3].ch[20]);
   // seems to be unneeded, no write-alloc? but just in case..
   syscalls.cache_wb(&mem->sizeof_region_mem, 3 * 4, 1);
   break;

  case CCMD_DOIT:
   worker->active = ACTIVE_CNT;
   worker->boot_cnt++;
   syscalls.cache_inv(worker, 128, 1);
   syscalls.cache_wb(&worker->i_done, 128, 1);
   memcpy(&spu_config, &mem->in.spu_config, sizeof(spu_config));

   if (worker->ram_dirty)
    // it's faster to do it all than just a 512k buffer
    syscalls.cache_wbInvAll();

   do_processing();

   syscalls.cache_inv(&mem->sb_thread, sizeof(mem->sb_thread), 0);
   syscalls.cache_inv(&mem->in, sizeof(mem->in), 0);
   break;

  default:
   syscalls.printf("bad cmd: %x\n", cmd);
   break;
 }

 return 0;
}

#pragma DATA_SECTION(component_test_dsp, ".sec_com");
dsp_component_t component_test_dsp = {
 {
  NULL,       /* init */
  exec,
  NULL,       /* exec fastcall RPC */
  NULL,       /* exit */
 },

 COMPONENT_NAME,
};

DSP_COMPONENT_MAIN

// vim:shiftwidth=1:expandtab
