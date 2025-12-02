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

#include <dlfcn.h>
#include <stddef.h>
#include <unistd.h>

#include <inc_libc64_mini.h>
#include "spu_c64x.h"

static struct {
 void *handle;
 int  (*dsp_open)(void);
 dsp_mem_region_t (*dsp_shm_alloc)(dsp_cache_t _type, sU32 _numBytes);
 int  (*dsp_shm_free)(dsp_mem_region_t _mem);
 void (*dsp_close)(void);
 int  (*dsp_component_load)(const char *_path, const char *_name, dsp_component_id_t *_id);
 int  (*dsp_cache_inv_virt)(void *_virtAddr, sU32 _size);
 int  (*dsp_rpc_send)(const dsp_msg_t *_msgTo);
 int  (*dsp_rpc_recv)(dsp_msg_t *_msgFrom);
 int  (*dsp_rpc)(const dsp_msg_t *_msgTo, dsp_msg_t *_msgFrom);
 void (*dsp_logbuf_print)(void);

 dsp_mem_region_t region;
 dsp_component_id_t compid;
 unsigned int stale_caches:1;
 unsigned int req_sent:1;
} f;

static noinline void dsp_fault(void)
{
 dsp_msg_t msg;

 f.dsp_cache_inv_virt(worker, sizeof(*worker));
 printf("dsp crash/fault/corruption:\n");
 printf("state rdy/reap/done: %u %u %u\n",
   worker->i_ready, worker->i_reaped, worker->i_done);
 printf("active/boot: %u %u\n",
   worker->active, worker->boot_cnt);

 if (f.req_sent) {
  f.dsp_rpc_recv(&msg);
  f.req_sent = 0;
 }
 f.dsp_logbuf_print();
 spu_config.iUseThread = 0;
}

static void thread_work_start(void)
{
 struct region_mem *mem;
 dsp_msg_t msg;
 int ret;

 // make sure new work is written out
 __sync_synchronize();

 // this should be safe, as dsp checks for new work even
 // after it decrements ->active
 // cacheline: i_done, active
 f.dsp_cache_inv_virt(&worker->i_done, 64);
 if (worker->active == ACTIVE_CNT)
  return;

 // to start the DSP, dsp_rpc_send() must be used,
 // but before that, previous request must be finished
 if (f.req_sent) {
  if (worker->boot_cnt == worker->last_boot_cnt) {
   // hopefully still booting
   //printf("booting?\n");
   return;
  }

  ret = f.dsp_rpc_recv(&msg);
  if (ret != 0) {
   fprintf(stderr, "dsp_rpc_recv failed: %d\n", ret);
   f.dsp_logbuf_print();
   f.req_sent = 0;
   spu_config.iUseThread = 0;
   return;
  }
 }

 f.dsp_cache_inv_virt(&worker->i_done, 64);
 worker->last_boot_cnt = worker->boot_cnt;
 worker->ram_dirty = spu.bMemDirty;
 spu.bMemDirty = 0;

 mem = (void *)f.region.virt_addr;
 memcpy(&mem->in.spu_config, &spu_config, sizeof(mem->in.spu_config));

 DSP_MSG_INIT(&msg, f.compid, CCMD_DOIT, f.region.phys_addr, 0);
 ret = f.dsp_rpc_send(&msg);
 if (ret != 0) {
  fprintf(stderr, "dsp_rpc_send failed: %d\n", ret);
  f.dsp_logbuf_print();
  spu_config.iUseThread = 0;
  return;
 }
 f.req_sent = 1;

#if 0
 f.dsp_rpc_recv(&msg);
 f.req_sent = 0;
#endif
}

static int thread_get_i_done(void)
{
 f.dsp_cache_inv_virt(&worker->i_done, sizeof(worker->i_done));
 return worker->i_done;
}

static void thread_work_wait_sync(struct work_item *work, int force)
{
 int limit = 1000;
 int ns_to;

 if ((unsigned int)(worker->i_done - worker->i_reaped) > WORK_MAXCNT) {
  dsp_fault();
  return;
 }

 while (worker->i_done == worker->i_reaped && limit-- > 0) {
  if (!f.req_sent) {
   printf("dsp: req not sent?\n");
   break;
  }

  if (worker->boot_cnt != worker->last_boot_cnt && !worker->active) {
   printf("dsp: broken sync\n");
   worker->last_boot_cnt = ~0;
   break;
  }

  usleep(500);
  f.dsp_cache_inv_virt(&worker->i_done, 64);
 }

 ns_to = work->ns_to;
 f.dsp_cache_inv_virt(work->SSumLR, sizeof(work->SSumLR[0]) * 2 * ns_to);
 preload(work->SSumLR);
 preload(work->SSumLR + 64/4);

 f.stale_caches = 1; // sb, spuMem

 if (limit == 0)
  printf("dsp: wait timeout\n");

 // still in results loop?
 if (worker->i_reaped != worker->i_done - 1)
  return;

 if (f.req_sent && (force || worker->i_done == worker->i_ready)) {
  dsp_msg_t msg;
  int ret;

  ret = f.dsp_rpc_recv(&msg);
  if (ret != 0) {
   fprintf(stderr, "dsp_rpc_recv failed: %d\n", ret);
   f.dsp_logbuf_print();
   spu_config.iUseThread = 0;
  }
  f.req_sent = 0;
 }
}

static void thread_sync_caches(void)
{
 if (f.stale_caches) {
  f.dsp_cache_inv_virt(spu.sb_thread, sizeof(spu.sb_thread[0]) * (MAXCHAN+1));
  f.dsp_cache_inv_virt(spu.spuMemC + 0x800, 0x800);
  if (spu.rvb->StartAddr) {
   int left = 0x40000 - spu.rvb->StartAddr;
   f.dsp_cache_inv_virt(spu.spuMem + spu.rvb->StartAddr, left * 2);
  }
  f.stale_caches = 0;
 }
}

static void init_spu_thread(void)
{
 dsp_msg_t init_msg, msg_in;
 struct region_mem *mem;
 int ret;

 spu.sb_thread = spu.sb_thread_;
 if (f.handle == NULL) {
  const char lib[] = "libc64.so.1";
  int failed = 0;

  f.handle = dlopen(lib, RTLD_NOW);
  if (f.handle == NULL) {
   fprintf(stderr, "can't load %s: %s\n", lib, dlerror());
   goto fail_open;
  }
  #define LDS(name) \
    failed |= (f.name = dlsym(f.handle, #name)) == NULL
  LDS(dsp_open);
  LDS(dsp_close);
  LDS(dsp_shm_alloc);
  LDS(dsp_shm_free);
  LDS(dsp_cache_inv_virt);
  LDS(dsp_component_load);
  LDS(dsp_rpc_send);
  LDS(dsp_rpc_recv);
  LDS(dsp_rpc);
  LDS(dsp_logbuf_print);
  #undef LDS
  if (failed) {
   fprintf(stderr, "missing symbol(s) in %s\n", lib);
   dlclose(f.handle);
   f.handle = NULL;
   goto fail_open;
  }
 }

 ret = f.dsp_open();
 if (ret != 0) {
  fprintf(stderr, "dsp_open failed: %d\n", ret);
  goto fail_open;
 }

 ret = f.dsp_component_load(NULL, COMPONENT_NAME, &f.compid);
 if (ret != 0) {
  fprintf(stderr, "dsp_component_load failed: %d\n", ret);
  goto fail_cload;
 }

 f.region = f.dsp_shm_alloc(DSP_CACHE_R, sizeof(*mem)); // writethrough
 if (f.region.size < sizeof(*mem) || f.region.virt_addr == 0) {
  fprintf(stderr, "dsp_shm_alloc failed\n");
  goto fail_mem;
 }
 mem = (void *)f.region.virt_addr;

 memcpy(&mem->in.spu_config, &spu_config, sizeof(mem->in.spu_config));

 DSP_MSG_INIT(&init_msg, f.compid, CCMD_INIT, f.region.phys_addr, 0);
 ret = f.dsp_rpc(&init_msg, &msg_in);
 if (ret != 0) {
  fprintf(stderr, "dsp_rpc failed: %d\n", ret);
  goto fail_init;
 }

 if (mem->sizeof_region_mem != sizeof(*mem)) {
  fprintf(stderr, "error: size mismatch 1: %d vs %zd\n",
    mem->sizeof_region_mem, sizeof(*mem));
  goto fail_init;
 }
 if (mem->offsetof_s_chan1 != offsetof(typeof(*mem), in.s_chan[1])) {
  fprintf(stderr, "error: size mismatch 2: %d vs %zd\n",
    mem->offsetof_s_chan1, offsetof(typeof(*mem), in.s_chan[1]));
  goto fail_init;
 }
 if (mem->offsetof_spos_3_20 != offsetof(typeof(*mem), worker.i[3].ch[20])) {
  fprintf(stderr, "error: size mismatch 3: %d vs %zd\n",
    mem->offsetof_spos_3_20, offsetof(typeof(*mem), worker.i[3].ch[20]));
  goto fail_init;
 }

 // override default allocations
 free(spu.spuMemC);
 spu.spuMemC = mem->spu_ram;
 spu.sb_thread = mem->sb_thread;
 free(spu.s_chan);
 spu.s_chan = mem->in.s_chan;
 free(spu.rvb);
 spu.rvb = &mem->in.rvb;
 worker = &mem->worker;

 printf("spu: C64x DSP ready (id=%d).\n", (int)f.compid);
 f.dsp_logbuf_print();

 spu_config.iThreadAvail = 1;
 (void)do_channel_work; // used by DSP instead
 return;

fail_init:
 f.dsp_shm_free(f.region);
fail_mem:
 // no component unload func?
fail_cload:
 f.dsp_logbuf_print();
 f.dsp_close();
fail_open:
 printf("spu: C64x DSP init failed.\n");
 spu_config.iUseThread = spu_config.iThreadAvail = 0;
 worker = NULL;
}

static void exit_spu_thread(void)
{
 dsp_msg_t msg;

 if (worker == NULL)
  return;

 if (f.req_sent) {
  f.dsp_rpc_recv(&msg);
  f.req_sent = 0;
 }

 f.dsp_logbuf_print();
 f.dsp_shm_free(f.region);
 f.dsp_close();

 spu.spuMemC = NULL;
 spu.sb_thread = spu.sb_thread_;
 spu.s_chan = NULL;
 spu.rvb = NULL;
 worker = NULL;
}

/* debug: "access" shared mem from gdb */
#if 0
struct region_mem *dbg_dsp_mem;

void dbg_dsp_mem_update(void)
{
 struct region_mem *mem;

 if (dbg_dsp_mem == NULL)
  dbg_dsp_mem = malloc(sizeof(*dbg_dsp_mem));
 if (dbg_dsp_mem == NULL)
  return;

 mem = (void *)f.region.virt_addr;
 f.dsp_cache_inv_virt(mem, sizeof(*mem));
 memcpy(dbg_dsp_mem, mem, sizeof(*dbg_dsp_mem));
}
#endif

// vim:shiftwidth=1:expandtab
