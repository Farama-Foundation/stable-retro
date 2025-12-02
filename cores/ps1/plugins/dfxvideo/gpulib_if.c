/***************************************************************************
    copyright            : (C) 2001 by Pete Bernert, 2011 notaz

 ***************************************************************************/
/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version. See also the license.txt file for *
 *   additional informations.                                              *
 *                                                                         *
 ***************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../gpulib/gpu.h"
#include "../../include/arm_features.h"

#if defined(__GNUC__) && (__GNUC__ >= 6 || (defined(__clang_major__) && __clang_major__ >= 10))
#pragma GCC diagnostic ignored "-Wmisleading-indentation"
#endif

#ifdef THREAD_RENDERING
#include "../gpulib/gpulib_thread_if.h"
#define do_cmd_list real_do_cmd_list
#define renderer_init real_renderer_init
#define renderer_finish real_renderer_finish
#define renderer_sync_ecmds real_renderer_sync_ecmds
#define renderer_update_caches real_renderer_update_caches
#define renderer_flush_queues real_renderer_flush_queues
#define renderer_set_interlace real_renderer_set_interlace
#define renderer_set_config real_renderer_set_config
#define renderer_notify_res_change real_renderer_notify_res_change
#define renderer_notify_update_lace real_renderer_notify_update_lace
#define renderer_sync real_renderer_sync
#define ex_regs scratch_ex_regs
#endif

#define u32 uint32_t

#define INFO_TW        0
#define INFO_DRAWSTART 1
#define INFO_DRAWEND   2
#define INFO_DRAWOFF   3

#define SHADETEXBIT(x) ((x>>24) & 0x1)
#define SEMITRANSBIT(x) ((x>>25) & 0x1)
#define PSXRGB(r,g,b) ((g<<10)|(b<<5)|r)

#define DATAREGISTERMODES unsigned short

#define DR_NORMAL        0
#define DR_VRAMTRANSFER  1

#define GPUSTATUS_READYFORVRAM        0x08000000

// byteswappings

#define SWAP16(x) __builtin_bswap16(x)
#define SWAP32(x) __builtin_bswap32(x)

#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__

// big endian config
#define HOST2LE32(x) SWAP32(x)
#define HOST2BE32(x) (x)
#define LE2HOST32(x) SWAP32(x)
#define BE2HOST32(x) (x)

#define HOST2LE16(x) SWAP16(x)
#define HOST2BE16(x) (x)
#define LE2HOST16(x) SWAP16(x)
#define BE2HOST16(x) (x)

#else

// little endian config
#define HOST2LE32(x) (x)
#define HOST2BE32(x) SWAP32(x)
#define LE2HOST32(x) (x)
#define BE2HOST32(x) SWAP32(x)

#define HOST2LE16(x) (x)
#define HOST2BE16(x) SWAP16(x)
#define LE2HOST16(x) (x)
#define BE2HOST16(x) SWAP16(x)

#endif

#define GETLEs16(X) ((int16_t)GETLE16((uint16_t *)(X)))

#define GETLE16(X) LE2HOST16(*(uint16_t *)(X))
#define GETLE32_(X) LE2HOST32(*(uint32_t *)(X))
#define PUTLE16(X, Y) do{*((uint16_t *)(X))=HOST2LE16((uint16_t)(Y));}while(0)
#define PUTLE32_(X, Y) do{*((uint32_t *)(X))=HOST2LE32((uint32_t)(Y));}while(0)
#if defined(__arm__) && !defined(HAVE_ARMV6)
// for (very) old ARMs with no unaligned loads?
#define GETLE32(X) (*(uint16_t *)(X)|((uint32_t)((uint16_t *)(X))[1]<<16))
#define PUTLE32(X, Y) do{uint16_t *p_=(uint16_t *)(X);uint32_t y_=Y;p_[0]=y_;p_[1]=y_>>16;}while(0)
#else
#define GETLE32 GETLE32_
#define PUTLE32 PUTLE32_
#endif

/////////////////////////////////////////////////////////////////////////////

typedef struct VRAMLOADTTAG
{
 short x;
 short y;
 short Width;
 short Height;
 short RowsRemaining;
 short ColsRemaining;
 unsigned short *ImagePtr;
} VRAMLoad_t;

/////////////////////////////////////////////////////////////////////////////

typedef struct PSXPOINTTAG
{
 int32_t x;
 int32_t y;
} PSXPoint_t;

typedef struct PSXSPOINTTAG
{
 short x;
 short y;
} PSXSPoint_t;

typedef struct PSXRECTTAG
{
 short x0;
 short x1;
 short y0;
 short y1;
} PSXRect_t;

// linux defines for some windows stuff

#define FALSE 0
#define TRUE 1
#define BOOL unsigned short
#define LOWORD(l)           ((unsigned short)(l))
#define HIWORD(l)           ((unsigned short)(((uint32_t)(l) >> 16) & 0xFFFF))
#define max(a,b)            (((a) > (b)) ? (a) : (b))
#define min(a,b)            (((a) < (b)) ? (a) : (b))
#define DWORD uint32_t
#ifndef __int64
#define __int64 long long int
#endif

typedef struct RECTTAG
{
 int left;
 int top;
 int right;
 int bottom;
}RECT;

/////////////////////////////////////////////////////////////////////////////

typedef struct TWINTAG
{
 PSXRect_t  Position;
 int xmask, ymask;
} TWin_t;

/////////////////////////////////////////////////////////////////////////////

typedef struct PSXDISPLAYTAG
{
 PSXPoint_t  DisplayModeNew;
 PSXPoint_t  DisplayMode;
 PSXPoint_t  DisplayPosition;
 PSXPoint_t  DisplayEnd;

 int32_t        Double;
 int32_t        Height;
 int32_t        PAL;
 int32_t        InterlacedNew;
 int32_t        Interlaced;
 int32_t        RGB24New;
 int32_t        RGB24;
 PSXSPoint_t DrawOffset;
 int32_t        Disabled;
 PSXRect_t   Range;

} PSXDisplay_t;

/////////////////////////////////////////////////////////////////////////////

// draw.c

extern int32_t           GlobalTextAddrX,GlobalTextAddrY,GlobalTextTP;
extern int32_t           GlobalTextABR,GlobalTextPAGE;
extern short          ly0,lx0,ly1,lx1,ly2,lx2,ly3,lx3;
extern long           lLowerpart;
extern BOOL           bCheckMask;
extern unsigned short sSetMask;
extern unsigned long  lSetMask;
extern short          g_m1;
extern short          g_m2;
extern short          g_m3;
extern short          DrawSemiTrans;

// prim.c

extern BOOL           bUsingTWin;
extern TWin_t         TWin;
extern void (*primTableJ[256])(unsigned char *);
extern void (*primTableSkip[256])(unsigned char *);
extern unsigned short  usMirror;
extern int            iDither;
extern uint32_t  dwCfgFixes;
extern uint32_t  dwActFixes;
extern int            iUseFixes;
extern int            iUseDither;
extern BOOL           bDoVSyncUpdate;
extern int32_t           drawX;
extern int32_t           drawY;
extern int32_t           drawW;
extern int32_t           drawH;

// gpu.h

#define OPAQUEON   10
#define OPAQUEOFF  11

#define KEY_RESETTEXSTORE 1
#define KEY_SHOWFPS       2
#define KEY_RESETOPAQUE   4
#define KEY_RESETDITHER   8
#define KEY_RESETFILTER   16
#define KEY_RESETADVBLEND 32
#define KEY_BADTEXTURES   128
#define KEY_CHECKTHISOUT  256

#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
#define RED(x) (x & 0xff)
#define BLUE(x) ((x>>16) & 0xff)
#define GREEN(x) ((x>>8) & 0xff)
#define COLOR(x) (x & 0xffffff)
#else
#define RED(x) ((x>>24) & 0xff)
#define BLUE(x) ((x>>8) & 0xff)
#define GREEN(x) ((x>>16) & 0xff)
#define COLOR(x) SWAP32(x & 0xffffff)
#endif

PSXDisplay_t      PSXDisplay;
unsigned char  *psxVub;
unsigned short *psxVuw;
unsigned short *psxVuw_eom;

long              lGPUstatusRet;
uint32_t          lGPUInfoVals[16];

VRAMLoad_t        VRAMWrite;
VRAMLoad_t        VRAMRead;

DATAREGISTERMODES DataWriteMode;
DATAREGISTERMODES DataReadMode;

BOOL           bCheckMask = FALSE;
unsigned short sSetMask = 0;
unsigned long  lSetMask = 0;
long           lLowerpart;

#if defined(__GNUC__) && __GNUC__ >= 6
#pragma GCC diagnostic ignored "-Wmisleading-indentation"
#endif

#include "soft.c"
#include "prim.c"

/////////////////////////////////////////////////////////////////////////////

static void set_vram(void *vram)
{
 psxVub=vram;
 psxVuw=(unsigned short *)psxVub;
 psxVuw_eom=psxVuw+1024*512;                           // pre-calc of end of vram
}

int renderer_init(void)
{
 set_vram(gpu.vram);

 PSXDisplay.RGB24        = FALSE;                      // init some stuff
 PSXDisplay.Interlaced   = FALSE;
 PSXDisplay.DrawOffset.x = 0;
 PSXDisplay.DrawOffset.y = 0;
 PSXDisplay.DisplayMode.x= 320;
 PSXDisplay.DisplayMode.y= 240;
 PSXDisplay.Disabled     = FALSE;
 PSXDisplay.Range.x0=0;
 PSXDisplay.Range.x1=0;
 PSXDisplay.Double = 1;

 DataWriteMode = DR_NORMAL;
 lGPUstatusRet = 0x14802000;

 return 0;
}

void renderer_finish(void)
{
}

void renderer_notify_res_change(void)
{
}

void renderer_notify_scanout_change(int x, int y)
{
}

#include "../gpulib/gpu_timing.h"
extern const unsigned char cmd_lengths[256];

int do_cmd_list(uint32_t *list, int list_len,
 int *cycles_sum_out, int *cycles_last, int *last_cmd)
{
  int cpu_cycles_sum = 0, cpu_cycles = *cycles_last;
  unsigned int cmd = 0, len;
  uint32_t *list_start = list;
  uint32_t *list_end = list + list_len;

  for (; list < list_end; list += 1 + len)
  {
    short *slist = (void *)list;
    cmd = GETLE32(list) >> 24;
    len = cmd_lengths[cmd];
    if (list + 1 + len > list_end) {
      cmd = -1;
      break;
    }

#ifndef TEST
    if (0x80 <= cmd && cmd < 0xe0)
      break; // image i/o, forward to upper layer
    else if ((cmd & 0xf8) == 0xe0)
      gpu.ex_regs[cmd & 7] = GETLE32(list);
#endif

    primTableJ[cmd]((void *)list);

    switch(cmd)
    {
      case 0x48 ... 0x4F:
      {
        u32 num_vertexes = 2;
        u32 *list_position = &(list[3]);

        while(1)
        {
          gput_sum(cpu_cycles_sum, cpu_cycles, gput_line(0));

          if(list_position >= list_end) {
            cmd = -1;
            goto breakloop;
          }

          if((*list_position & HOST2LE32(0xf000f000)) == HOST2LE32(0x50005000))
            break;

          list_position++;
          num_vertexes++;
        }

        len += (num_vertexes - 2);
        break;
      }

      case 0x58 ... 0x5F:
      {
        u32 num_vertexes = 2;
        u32 *list_position = &(list[4]);

        while(1)
        {
          gput_sum(cpu_cycles_sum, cpu_cycles, gput_line(0));

          if(list_position >= list_end) {
            cmd = -1;
            goto breakloop;
          }

          if((*list_position & HOST2LE32(0xf000f000)) == HOST2LE32(0x50005000))
            break;

          list_position += 2;
          num_vertexes++;
        }

        len += (num_vertexes - 2) * 2;
        break;
      }

#ifdef TEST
      case 0xA0:          //  sys -> vid
      {
        u32 load_width = LE2HOST16(slist[4]);
        u32 load_height = LE2HOST16(slist[5]);
        u32 load_size = load_width * load_height;

        len += load_size / 2;
        break;
      }
#endif

      // timing
      case 0x02:
        gput_sum(cpu_cycles_sum, cpu_cycles,
            gput_fill(LE2HOST16(slist[4]) & 0x3ff, LE2HOST16(slist[5]) & 0x1ff));
        break;
      case 0x20 ... 0x23: gput_sum(cpu_cycles_sum, cpu_cycles, gput_poly_base());    break;
      case 0x24 ... 0x27: gput_sum(cpu_cycles_sum, cpu_cycles, gput_poly_base_t());  break;
      case 0x28 ... 0x2B: gput_sum(cpu_cycles_sum, cpu_cycles, gput_quad_base());    break;
      case 0x2C ... 0x2F: gput_sum(cpu_cycles_sum, cpu_cycles, gput_quad_base_t());  break;
      case 0x30 ... 0x33: gput_sum(cpu_cycles_sum, cpu_cycles, gput_poly_base_g());  break;
      case 0x34 ... 0x37: gput_sum(cpu_cycles_sum, cpu_cycles, gput_poly_base_gt()); break;
      case 0x38 ... 0x3B: gput_sum(cpu_cycles_sum, cpu_cycles, gput_quad_base_g());  break;
      case 0x3C ... 0x3F: gput_sum(cpu_cycles_sum, cpu_cycles, gput_quad_base_gt()); break;
      case 0x40 ... 0x47: gput_sum(cpu_cycles_sum, cpu_cycles, gput_line(0));        break;
      case 0x50 ... 0x57: gput_sum(cpu_cycles_sum, cpu_cycles, gput_line(0));        break;
      case 0x60 ... 0x63:
        gput_sum(cpu_cycles_sum, cpu_cycles,
            gput_sprite(LE2HOST16(slist[4]) & 0x3ff, LE2HOST16(slist[5]) & 0x1ff));
        break;
      case 0x64 ... 0x67:
        gput_sum(cpu_cycles_sum, cpu_cycles,
            gput_sprite(LE2HOST16(slist[6]) & 0x3ff, LE2HOST16(slist[7]) & 0x1ff));
        break;
      case 0x68 ... 0x6B: gput_sum(cpu_cycles_sum, cpu_cycles, gput_sprite(1, 1));   break;
      case 0x70 ... 0x73:
      case 0x74 ... 0x77: gput_sum(cpu_cycles_sum, cpu_cycles, gput_sprite(8, 8));   break;
      case 0x78 ... 0x7B:
      case 0x7C ... 0x7F: gput_sum(cpu_cycles_sum, cpu_cycles, gput_sprite(16, 16)); break;
    }
  }

breakloop:
  gpu.ex_regs[1] &= ~0x1ff;
  gpu.ex_regs[1] |= lGPUstatusRet & 0x1ff;

  *cycles_sum_out += cpu_cycles_sum;
  *cycles_last = cpu_cycles;
  *last_cmd = cmd;
  return list - list_start;
}

void renderer_sync_ecmds(uint32_t *ecmds_)
{
#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
  // the funcs below expect LE
  uint32_t i, ecmds[8];
  for (i = 1; i <= 6; i++)
    ecmds[i] = HTOLE32(ecmds_[i]);
#else
  uint32_t *ecmds = ecmds_;
#endif
  cmdTexturePage((unsigned char *)&ecmds[1]);
  cmdTextureWindow((unsigned char *)&ecmds[2]);
  cmdDrawAreaStart((unsigned char *)&ecmds[3]);
  cmdDrawAreaEnd((unsigned char *)&ecmds[4]);
  cmdDrawOffset((unsigned char *)&ecmds[5]);
  cmdSTP((unsigned char *)&ecmds[6]);
}

void renderer_update_caches(int x, int y, int w, int h, int state_changed)
{
}

void renderer_flush_queues(void)
{
}

void renderer_set_interlace(int enable, int is_odd)
{
}

void renderer_sync(void)
{
}

void renderer_notify_update_lace(int updated)
{
}

#include "../../frontend/plugin_lib.h"

void renderer_set_config(const struct rearmed_cbs *cbs)
{
 iUseDither = cbs->dithering;
 dwActFixes = cbs->gpu_peops.dwActFixes;
 if (cbs->pl_set_gpu_caps)
  cbs->pl_set_gpu_caps(0);
 set_vram(gpu.vram);
}

// vim:ts=2:shiftwidth=2:expandtab
