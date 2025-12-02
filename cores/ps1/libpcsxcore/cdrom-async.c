/***************************************************************************
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 ***************************************************************************/

#include <stdalign.h>
#include <stdlib.h>
#include <string.h>
#include "system.h"
#include "plugins.h"
#include "cdriso.h"
#include "cdrom.h"
#include "cdrom-async.h"

#if 0
#define acdrom_dbg printf
#else
#define acdrom_dbg(...)
#endif

#ifdef USE_ASYNC_CDROM

static void *g_cd_handle;

#ifndef HAVE_CDROM

static void *rcdrom_open(const char *name, u32 *total_lba, u32 *have_sub) { return NULL; }
static void rcdrom_close(void *stream) {}
static int  rcdrom_getTN(void *stream, u8 *tn) { return -1; }
static int  rcdrom_getTD(void *stream, u32 total_lba, u8 track, u8 *rt) { return -1; }
static int  rcdrom_getStatus(void *stream, struct CdrStat *stat) { return -1; }
static int  rcdrom_readSector(void *stream, unsigned int lba, void *b) { return -1; }
static int  rcdrom_readSub(void *stream, unsigned int lba, void *b) { return -1; }
static int  rcdrom_isMediaInserted(void *stream) { return 0; }

#endif

#ifdef USE_C11_THREADS
#include <threads.h>

static int c11_threads_cb_wrapper(void *cb)
{
   ((void (*)(void *))cb)(NULL);

   return 0;
}

#define slock_new() ({ \
        mtx_t *lock = malloc(sizeof(*lock)); \
        if (lock) mtx_init(lock, mtx_plain); \
        lock; \
})

#define scond_new() ({ \
        cnd_t *cnd = malloc(sizeof(*cnd)); \
        if (cnd) cnd_init(cnd); \
        cnd; \
})

#define pcsxr_sthread_create(cb, unused) ({ \
        thrd_t *thd = malloc(sizeof(*thd)); \
        if (thd) \
                thrd_create(thd, c11_threads_cb_wrapper, cb); \
        thd; \
})

#define sthread_join(thrd) ({ \
        thrd_join(*thrd, NULL); \
        free(thrd); \
})

#define slock_free(lock) free(lock)
#define slock_lock(lock) mtx_lock(lock)
#define slock_unlock(lock) mtx_unlock(lock)
#define scond_free(cond) free(cond)
#define scond_wait(cond, lock) cnd_wait(cond, lock)
#define scond_signal(cond) cnd_signal(cond)
#define slock_t mtx_t
#define scond_t cnd_t
#define sthread_t thrd_t
#else
#include "../frontend/libretro-rthreads.h"
#endif

#include "retro_timers.h"

struct cached_buf {
   u32 lba;
   u8 buf[CD_FRAMESIZE_RAW];
   u8 buf_sub[SUB_FRAMESIZE];
};
static struct {
   sthread_t *thread;
   slock_t *read_lock;
   slock_t *buf_lock;
   scond_t *cond;
   struct cached_buf *buf_cache;
   u32 buf_cnt, thread_exit, do_prefetch, prefetch_failed, have_subchannel;
   u32 total_lba, prefetch_lba;
   int check_eject_delay;

   // single sector cache, not touched by the thread
   alignas(64) u8 buf_local[CD_FRAMESIZE_RAW_ALIGNED];
} acdrom;

static void lbacache_do(u32 lba)
{
   alignas(64) unsigned char buf[CD_FRAMESIZE_RAW_ALIGNED];
   unsigned char msf[3], buf_sub[SUB_FRAMESIZE];
   u32 i = lba % acdrom.buf_cnt;
   int ret;

   lba2msf(lba + 150, &msf[0], &msf[1], &msf[2]);
   slock_lock(acdrom.read_lock);
   if (g_cd_handle)
      ret = rcdrom_readSector(g_cd_handle, lba, buf);
   else
      ret = ISOreadTrack(msf, buf);
   if (acdrom.have_subchannel) {
      if (g_cd_handle)
         ret |= rcdrom_readSub(g_cd_handle, lba, buf_sub);
      else
         ret |= ISOreadSub(msf, buf_sub);
   }

   slock_lock(acdrom.buf_lock);
   slock_unlock(acdrom.read_lock);
   acdrom_dbg("c  %d:%02d:%02d %2d m%d f%d\n", msf[0], msf[1], msf[2], ret,
         buf[12+3], ((buf[12+4+2] >> 5) & 1) + 1);
   if (ret) {
      acdrom.do_prefetch = 0;
      acdrom.prefetch_failed = 1;
      slock_unlock(acdrom.buf_lock);
      SysPrintf("prefetch: read failed for lba %d: %d\n", lba, ret);
      return;
   }
   acdrom.prefetch_failed = 0;
   acdrom.check_eject_delay = 100;

   if (lba != acdrom.buf_cache[i].lba) {
      acdrom.buf_cache[i].lba = lba;
      memcpy(acdrom.buf_cache[i].buf, buf, sizeof(acdrom.buf_cache[i].buf));
      if (acdrom.have_subchannel)
         memcpy(acdrom.buf_cache[i].buf_sub, buf_sub, sizeof(buf_sub));
   }
   slock_unlock(acdrom.buf_lock);
   if (g_cd_handle)
      retro_sleep(0); // why does the main thread stall without this?
}

static int lbacache_get(unsigned int lba, void *buf, void *sub_buf)
{
   unsigned int i;
   int ret = 0;

   i = lba % acdrom.buf_cnt;
   slock_lock(acdrom.buf_lock);
   if (lba == acdrom.buf_cache[i].lba) {
      if (!buf)
         buf = acdrom.buf_local;
      memcpy(buf, acdrom.buf_cache[i].buf, CD_FRAMESIZE_RAW);
      if (sub_buf)
         memcpy(sub_buf, acdrom.buf_cache[i].buf_sub, SUB_FRAMESIZE);
      ret = 1;
   }
   slock_unlock(acdrom.buf_lock);
   return ret;
}

// note: This has races on some vars but that's ok, main thread can deal
// with it. Only unsafe buffer accesses and simultaneous reads are prevented.
static void cdra_prefetch_thread(void *unused)
{
   u32 buf_cnt, lba, lba_to;

   slock_lock(acdrom.buf_lock);
   while (!acdrom.thread_exit)
   {
#ifdef __GNUC__
      __asm__ __volatile__("":::"memory"); // barrier
#endif
      if (!acdrom.do_prefetch)
         scond_wait(acdrom.cond, acdrom.buf_lock);
      if (!acdrom.do_prefetch || acdrom.thread_exit)
         continue;

      buf_cnt = acdrom.buf_cnt;
      lba = acdrom.prefetch_lba;
      lba_to = lba + buf_cnt;
      if (lba_to > acdrom.total_lba)
         lba_to = acdrom.total_lba;
      for (; lba < lba_to; lba++) {
         if (lba != acdrom.buf_cache[lba % buf_cnt].lba)
            break;
      }
      if (lba == lba_to || lba >= acdrom.total_lba) {
         // caching complete
         acdrom.do_prefetch = 0;
         continue;
      }

      slock_unlock(acdrom.buf_lock);
      lbacache_do(lba);
      slock_lock(acdrom.buf_lock);
   }
   slock_unlock(acdrom.buf_lock);
}

void cdra_stop_thread(void)
{
   acdrom.thread_exit = 1;
   if (acdrom.buf_lock) {
      slock_lock(acdrom.buf_lock);
      acdrom.do_prefetch = 0;
      if (acdrom.cond)
         scond_signal(acdrom.cond);
      slock_unlock(acdrom.buf_lock);
   }
   if (acdrom.thread) {
      sthread_join(acdrom.thread);
      acdrom.thread = NULL;
   }
   if (acdrom.cond) { scond_free(acdrom.cond); acdrom.cond = NULL; }
   if (acdrom.buf_lock) { slock_free(acdrom.buf_lock); acdrom.buf_lock = NULL; }
   if (acdrom.read_lock) { slock_free(acdrom.read_lock); acdrom.read_lock = NULL; }
   free(acdrom.buf_cache);
   acdrom.buf_cache = NULL;
}

// the thread is optional, if anything fails we can do direct reads
static void cdra_start_thread(void)
{
   cdra_stop_thread();
   acdrom.thread_exit = acdrom.prefetch_lba = acdrom.do_prefetch = 0;
   acdrom.prefetch_failed = 0;
   if (acdrom.buf_cnt == 0)
      return;
   acdrom.buf_cache = calloc(acdrom.buf_cnt, sizeof(acdrom.buf_cache[0]));
   acdrom.buf_lock = slock_new();
   acdrom.read_lock = slock_new();
   acdrom.cond = scond_new();
   if (acdrom.buf_cache && acdrom.buf_lock && acdrom.read_lock && acdrom.cond)
   {
      int i;
      acdrom.thread = pcsxr_sthread_create(cdra_prefetch_thread, PCSXRT_CDR);
      for (i = 0; i < acdrom.buf_cnt; i++)
         acdrom.buf_cache[i].lba = ~0;
   }
   if (acdrom.thread) {
      SysPrintf("cdrom precache: %d buffers%s\n",
            acdrom.buf_cnt, acdrom.have_subchannel ? " +sub" : "");
   }
   else {
      SysPrintf("cdrom precache thread init failed.\n");
      cdra_stop_thread();
   }
}

int cdra_init(void)
{
   return ISOinit();
}

void cdra_shutdown(void)
{
   cdra_close();
}

int cdra_open(void)
{
   const char *name = GetIsoFile();
   u8 buf_sub[SUB_FRAMESIZE];
   int ret = -1, ret2;

   acdrom_dbg("%s %s\n", __func__, name);
   acdrom.have_subchannel = 0;
   if (!name[0] || !strncmp(name, "cdrom:", 6)) {
      g_cd_handle = rcdrom_open(name, &acdrom.total_lba, &acdrom.have_subchannel);
      if (!!g_cd_handle)
         ret = 0;
   }

   // try ISO even if it's cdrom:// as it might work through libretro vfs
   if (name[0] && ret < 0) {
      ret = ISOopen(name);
      if (ret == 0) {
         u8 msf[3];
         ISOgetTD(0, msf);
         acdrom.total_lba = MSF2SECT(msf[0], msf[1], msf[2]);
         msf[0] = 0; msf[1] = 2; msf[2] = 16;
         ret2 = ISOreadSub(msf, buf_sub);
         acdrom.have_subchannel = (ret2 == 0);
      }
   }
   if (ret == 0)
      cdra_start_thread();
   return ret;
}

void cdra_close(void)
{
   acdrom_dbg("%s\n", __func__);
   cdra_stop_thread();
   if (g_cd_handle) {
      rcdrom_close(g_cd_handle);
      g_cd_handle = NULL;
   }
   else
      ISOclose();
}

int cdra_getTN(unsigned char *tn)
{
   int ret;
   if (g_cd_handle)
      ret = rcdrom_getTN(g_cd_handle, tn);
   else
      ret = ISOgetTN(tn);
   acdrom_dbg("%s -> %d %d\n", __func__, tn[0], tn[1]);
   return ret;
}

int cdra_getTD(int track, unsigned char *rt)
{
   int ret;
   if (g_cd_handle)
      ret = rcdrom_getTD(g_cd_handle, acdrom.total_lba, track, rt);
   else
      ret = ISOgetTD(track, rt);
   //acdrom_dbg("%s %d -> %d:%02d:%02d\n", __func__, track, rt[2], rt[1], rt[0]);
   return ret;
}

int cdra_prefetch(unsigned char m, unsigned char s, unsigned char f)
{
   u32 lba = MSF2SECT(m, s, f);
   int ret = 1;
   if (acdrom.cond) {
      acdrom.prefetch_lba = lba;
      acdrom.do_prefetch = 1;
      scond_signal(acdrom.cond);
   }
   if (acdrom.buf_cache && !acdrom.prefetch_failed) {
     u32 c = acdrom.buf_cnt;
     if (c)
        ret = acdrom.buf_cache[lba % c].lba == lba;
     acdrom_dbg("p  %d:%02d:%02d %d\n", m, s, f, ret);
   }
   return ret;
}

static int cdra_do_read(const unsigned char *time, int cdda,
      void *buf, void *buf_sub)
{
   u32 lba = MSF2SECT(time[0], time[1], time[2]);
   int hit = 0, ret = -1, read_locked = 0;
   do
   {
      if (acdrom.buf_lock) {
         hit = lbacache_get(lba, buf, buf_sub);
         if (hit)
            break;
      }
      if (acdrom.read_lock) {
         // maybe still prefetching
         slock_lock(acdrom.read_lock);
         read_locked = 1;
         hit = lbacache_get(lba, buf, buf_sub);
         if (hit) {
            hit = 2;
            break;
         }
      }
      acdrom.do_prefetch = 0;
      if (!buf)
         buf = acdrom.buf_local;
      if (g_cd_handle) {
         if (buf_sub)
            ret = rcdrom_readSub(g_cd_handle, lba, buf_sub);
         else
            ret = rcdrom_readSector(g_cd_handle, lba, buf);
      }
      else if (buf_sub)
         ret = ISOreadSub(time, buf_sub);
      else if (cdda)
         ret = ISOreadCDDA(time, buf);
      else
         ret = ISOreadTrack(time, buf);
      if (ret)
         SysPrintf("cdrom read failed for lba %d: %d\n", lba, ret);
   }
   while (0);
   if (read_locked)
      slock_unlock(acdrom.read_lock);
   if (hit)
      ret = 0;
   acdrom.check_eject_delay = ret ? 0 : 100;
   acdrom_dbg("f%c %d:%02d:%02d %d%s\n",
      buf_sub ? 's' : (cdda ? 'c' : 'd'),
      time[0], time[1], time[2], hit, ret ? " ERR" : "");
   return ret;
}

// time: msf in non-bcd format
int cdra_readTrack(const unsigned char *time)
{
   if (!acdrom.thread && !g_cd_handle) {
      // just forward to ISOreadTrack to avoid extra copying
      return ISOreadTrack(time, NULL);
   }
   return cdra_do_read(time, 0, NULL, NULL);
}

int cdra_readCDDA(const unsigned char *time, void *buffer)
{
   return cdra_do_read(time, 1, buffer, NULL);
}

int cdra_readSub(const unsigned char *time, void *buffer)
{
   if (!acdrom.thread && !g_cd_handle)
      return ISOreadSub(time, buffer);
   if (!acdrom.have_subchannel)
      return -1;
   acdrom_dbg("s  %d:%02d:%02d\n", time[0], time[1], time[2]);
   return cdra_do_read(time, 0, NULL, buffer);
}

// pointer to cached buffer from last cdra_readTrack() call
void *cdra_getBuffer(void)
{
   //acdrom_dbg("%s\n", __func__);
   if (!acdrom.thread && !g_cd_handle)
      return ISOgetBuffer();
   return acdrom.buf_local + 12;
}

int cdra_getStatus(struct CdrStat *stat)
{
   int ret;
   CDR__getStatus(stat);
   if (g_cd_handle)
      ret = rcdrom_getStatus(g_cd_handle, stat);
   else
      ret = ISOgetStatus(stat);
   return ret;
}

int cdra_is_physical(void)
{
   return !!g_cd_handle;
}

int cdra_check_eject(int *inserted)
{
   if (!g_cd_handle || acdrom.do_prefetch || acdrom.check_eject_delay-- > 0)
      return 0;
   acdrom.check_eject_delay = 100;
   *inserted = rcdrom_isMediaInserted(g_cd_handle); // 1-2ms
   return 1;
}

void cdra_set_buf_count(int newcount)
{
   if (acdrom.buf_cnt == newcount)
      return;
   cdra_stop_thread();
   acdrom.buf_cnt = newcount;
   cdra_start_thread();
}

int cdra_get_buf_count(void)
{
   return acdrom.buf_cnt;
}

int cdra_get_buf_cached_approx(void)
{
   u32 buf_cnt = acdrom.buf_cnt, lba = acdrom.prefetch_lba;
   u32 total = acdrom.total_lba;
   u32 left = buf_cnt;
   int buf_use = 0;

   if (left > total)
      left = total;
   for (; lba < total && left > 0; lba++, left--)
      if (lba == acdrom.buf_cache[lba % buf_cnt].lba)
         buf_use++;
   for (lba = 0; left > 0; lba++, left--)
      if (lba == acdrom.buf_cache[lba % buf_cnt].lba)
         buf_use++;

   return buf_use;
}
#else

// phys. CD-ROM without a cache is unusable so not implemented
#ifdef HAVE_CDROM
#error "HAVE_CDROM requires USE_ASYNC_CDROM"
#endif

// just forward to cdriso
int cdra_init(void)
{
   return ISOinit();
}

void cdra_shutdown(void)
{
   ISOshutdown();
}

int cdra_open(void)
{
   return ISOopen(GetIsoFile());
}

void cdra_close(void)
{
   ISOclose();
}

int cdra_getTN(unsigned char *tn)
{
   return ISOgetTN(tn);
}

int cdra_getTD(int track, unsigned char *rt)
{
   return ISOgetTD(track, rt);
}

int cdra_prefetch(unsigned char m, unsigned char s, unsigned char f)
{
   return 1; // always hit
}

// time: msf in non-bcd format
int cdra_readTrack(const unsigned char *time)
{
   return ISOreadTrack(time, NULL);
}

int cdra_readCDDA(const unsigned char *time, void *buffer)
{
   return ISOreadCDDA(time, buffer);
}

int cdra_readSub(const unsigned char *time, void *buffer)
{
   return ISOreadSub(time, buffer);
}

// pointer to cached buffer from last cdra_readTrack() call
void *cdra_getBuffer(void)
{
   return ISOgetBuffer();
}

int cdra_getStatus(struct CdrStat *stat)
{
   return ISOgetStatus(stat);
}

int cdra_is_physical(void) { return 0; }
int cdra_check_eject(int *inserted) { return 0; }
void cdra_stop_thread(void) {}
void cdra_set_buf_count(int newcount) {}
int  cdra_get_buf_count(void) { return 0; }
int  cdra_get_buf_cached_approx(void) { return 0; }

#endif

// vim:sw=3:ts=3:expandtab
