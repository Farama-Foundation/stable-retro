/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *   Mupen64plus - plugin.c                                                *
 *   Mupen64Plus homepage: http://code.google.com/p/mupen64plus/           *
 *   Copyright (C) 2002 Hacktarux                                          *
 *   Copyright (C) 2009 Richard Goedeken                                   *
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
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.          *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include <stdio.h>
#include <stdlib.h>

#include "plugin.h"

#include "r4300/r4300_core.h"
#include "../rdp/rdp_core.h"
#include "../rsp/rsp_core.h"

#include "../vi/vi_controller.h"

#include "api/callbacks.h"
#include "api/m64p_common.h"
#include "api/m64p_plugin.h"
#include "api/m64p_types.h"

#include "main/main.h"
#include "main/device.h"
#include "main/rom.h"
#include "dd/dd_rom.h"
#include "main/version.h"
#include "memory/memory.h"

static unsigned int dummy;

/* local functions */
static void EmptyFunc(void)
{
}

static m64p_error EmptyGetVersionFunc(m64p_plugin_type *PluginType, int *PluginVersion, int *APIVersion, const char **PluginNamePtr, int *Capabilities)
{
   return M64ERR_SUCCESS;
}
/* local data structures and functions */
#define DEFINE_GFX(X) \
    EXPORT m64p_error CALL X##PluginGetVersion(m64p_plugin_type *, int *, int *, const char **, int *); \
    EXPORT void CALL X##ChangeWindow(void); \
    EXPORT int  CALL X##InitiateGFX(GFX_INFO Gfx_Info); \
    EXPORT void CALL X##MoveScreen(int x, int y); \
    EXPORT void CALL X##ProcessDList(void); \
    EXPORT void CALL X##ProcessRDPList(void); \
    EXPORT void CALL X##RomClosed(void); \
    EXPORT int  CALL X##RomOpen(void); \
    EXPORT void CALL X##ShowCFB(void); \
    EXPORT void CALL X##UpdateScreen(void); \
    EXPORT void CALL X##ViStatusChanged(void); \
    EXPORT void CALL X##ViWidthChanged(void); \
    EXPORT void CALL X##ReadScreen2(void *dest, int *width, int *height, int front); \
    EXPORT void CALL X##SetRenderingCallback(void (*callback)(int)); \
    EXPORT void CALL X##ResizeVideoOutput(int width, int height); \
    EXPORT void CALL X##FBRead(unsigned int addr); \
    EXPORT void CALL X##FBWrite(unsigned int addr, unsigned int size); \
    EXPORT void CALL X##FBGetFrameBufferInfo(void *p); \
    \
    static const gfx_plugin_functions gfx_##X = { \
        X##PluginGetVersion, \
        X##ChangeWindow, \
        X##InitiateGFX, \
        X##MoveScreen, \
        X##ProcessDList, \
        X##ProcessRDPList, \
        X##RomClosed, \
        X##RomOpen, \
        X##ShowCFB, \
        X##UpdateScreen, \
        X##ViStatusChanged, \
        X##ViWidthChanged, \
        X##ReadScreen2, \
        X##SetRenderingCallback, \
        X##FBRead, \
        X##FBWrite, \
        X##FBGetFrameBufferInfo \
    }

DEFINE_GFX(angrylion);
#ifdef HAVE_RICE
DEFINE_GFX(rice);
#endif
#ifdef HAVE_GLN64
DEFINE_GFX(gln64);
#endif
#ifdef HAVE_GLIDE64
DEFINE_GFX(glide64);
#endif
#ifdef HAVE_PARALLEL
DEFINE_GFX(parallel);
#endif

gfx_plugin_functions gfx;
GFX_INFO gfx_info;

static m64p_error plugin_start_gfx(void)
{
   /* fill in the GFX_INFO data structure */
   if ((g_ddrom != NULL) && (g_ddrom_size != 0) && (g_rom == NULL) && (g_rom_size == 0))
   {
      //fill in 64DD IPL header
      gfx_info.HEADER = (unsigned char *) g_ddrom;
   }
   else
   {
      //fill in regular N64 ROM header
      gfx_info.HEADER = (unsigned char *) g_rom;
   }
   gfx_info.RDRAM = (unsigned char *) g_rdram;
   gfx_info.DMEM = (unsigned char *) g_dev.sp.mem;
   gfx_info.IMEM = (unsigned char *) g_dev.sp.mem + 0x1000;
   gfx_info.MI_INTR_REG = &(g_dev.r4300.mi.regs[MI_INTR_REG]);
   gfx_info.DPC_START_REG = &(g_dev.dp.dpc_regs[DPC_START_REG]);
   gfx_info.DPC_END_REG = &(g_dev.dp.dpc_regs[DPC_END_REG]);
   gfx_info.DPC_CURRENT_REG = &(g_dev.dp.dpc_regs[DPC_CURRENT_REG]);
   gfx_info.DPC_STATUS_REG = &(g_dev.dp.dpc_regs[DPC_STATUS_REG]);
   gfx_info.DPC_CLOCK_REG = &(g_dev.dp.dpc_regs[DPC_CLOCK_REG]);
   gfx_info.DPC_BUFBUSY_REG = &(g_dev.dp.dpc_regs[DPC_BUFBUSY_REG]);
   gfx_info.DPC_PIPEBUSY_REG = &(g_dev.dp.dpc_regs[DPC_PIPEBUSY_REG]);
   gfx_info.DPC_TMEM_REG = &(g_dev.dp.dpc_regs[DPC_TMEM_REG]);
   gfx_info.VI_STATUS_REG = &(g_dev.vi.regs[VI_STATUS_REG]);
   gfx_info.VI_ORIGIN_REG = &(g_dev.vi.regs[VI_ORIGIN_REG]);
   gfx_info.VI_WIDTH_REG = &(g_dev.vi.regs[VI_WIDTH_REG]);
   gfx_info.VI_INTR_REG = &(g_dev.vi.regs[VI_V_INTR_REG]);
   gfx_info.VI_V_CURRENT_LINE_REG = &(g_dev.vi.regs[VI_CURRENT_REG]);
   gfx_info.VI_TIMING_REG = &(g_dev.vi.regs[VI_BURST_REG]);
   gfx_info.VI_V_SYNC_REG = &(g_dev.vi.regs[VI_V_SYNC_REG]);
   gfx_info.VI_H_SYNC_REG = &(g_dev.vi.regs[VI_H_SYNC_REG]);
   gfx_info.VI_LEAP_REG = &(g_dev.vi.regs[VI_LEAP_REG]);
   gfx_info.VI_H_START_REG = &(g_dev.vi.regs[VI_H_START_REG]);
   gfx_info.VI_V_START_REG = &(g_dev.vi.regs[VI_V_START_REG]);
   gfx_info.VI_V_BURST_REG = &(g_dev.vi.regs[VI_V_BURST_REG]);
   gfx_info.VI_X_SCALE_REG = &(g_dev.vi.regs[VI_X_SCALE_REG]);
   gfx_info.VI_Y_SCALE_REG = &(g_dev.vi.regs[VI_Y_SCALE_REG]);
   gfx_info.CheckInterrupts = EmptyFunc;

   /* call the audio plugin */
   if (!gfx.initiateGFX(gfx_info))
   {
      printf("plugin_start_gfx fail.\n");
      return M64ERR_PLUGIN_FAIL;
   }

   printf("plugin_start_gfx success.\n");

   return M64ERR_SUCCESS;
}

/* INPUT */
extern m64p_error inputPluginGetVersion(m64p_plugin_type *PluginType, int *PluginVersion,
                                              int *APIVersion, const char **PluginNamePtr, int *Capabilities);
extern void inputInitiateControllers (CONTROL_INFO ControlInfo);
extern void inputControllerCommand(int Control, unsigned char *Command);
extern void inputInitiateControllers(CONTROL_INFO ControlInfo);
extern void inputReadController(int Control, unsigned char *Command);
extern int  inputRomOpen(void);
extern void inputRomClosed(void);

input_plugin_functions input = {
    inputPluginGetVersion,
    inputControllerCommand,
    NULL,
    inputInitiateControllers,
    inputReadController,
    inputRomClosed,
    inputRomOpen,
};

static CONTROL_INFO control_info;
CONTROL Controls[4];

static m64p_error plugin_start_input(void)
{
   int i;

   /* fill in the CONTROL_INFO data structure */
   control_info.Controls = Controls;
   for (i=0; i<4; i++)
   {
      Controls[i].Present = 0;
      Controls[i].RawData = 0;
      Controls[i].Plugin = PLUGIN_NONE;
   }

   /* call the input plugin */
   input.initiateControllers(control_info);

   return M64ERR_SUCCESS;
}

/* RSP */
#define DEFINE_RSP(X) \
    EXPORT m64p_error CALL X##PluginGetVersion(m64p_plugin_type *, int *, int *, const char **, int *); \
    EXPORT unsigned int CALL X##DoRspCycles(unsigned int Cycles); \
    EXPORT void CALL X##InitiateRSP(RSP_INFO Rsp_Info, unsigned int *CycleCount); \
    EXPORT void CALL X##RomClosed(void); \
    \
    static const rsp_plugin_functions rsp_##X = { \
        X##PluginGetVersion, \
        X##DoRspCycles, \
        X##InitiateRSP, \
        X##RomClosed \
    }

DEFINE_RSP(hle);
DEFINE_RSP(cxd4);
#ifdef HAVE_PARALLEL_RSP
DEFINE_RSP(parallelRSP);
#endif

rsp_plugin_functions rsp;
RSP_INFO rsp_info;

static m64p_error plugin_start_rsp(void)
{
   /* fill in the RSP_INFO data structure */
   rsp_info.RDRAM = (unsigned char *) g_rdram;
   rsp_info.DMEM = (unsigned char *) g_dev.sp.mem;
   rsp_info.IMEM = (unsigned char *) g_dev.sp.mem + 0x1000;
   rsp_info.MI_INTR_REG = &g_dev.r4300.mi.regs[MI_INTR_REG];
   rsp_info.SP_MEM_ADDR_REG = &g_dev.sp.regs[SP_MEM_ADDR_REG];
   rsp_info.SP_DRAM_ADDR_REG = &g_dev.sp.regs[SP_DRAM_ADDR_REG];
   rsp_info.SP_RD_LEN_REG = &g_dev.sp.regs[SP_RD_LEN_REG];
   rsp_info.SP_WR_LEN_REG = &g_dev.sp.regs[SP_WR_LEN_REG];
   rsp_info.SP_STATUS_REG = &g_dev.sp.regs[SP_STATUS_REG];
   rsp_info.SP_DMA_FULL_REG = &g_dev.sp.regs[SP_DMA_FULL_REG];
   rsp_info.SP_DMA_BUSY_REG = &g_dev.sp.regs[SP_DMA_BUSY_REG];
   rsp_info.SP_PC_REG = &g_dev.sp.regs2[SP_PC_REG];
   rsp_info.SP_SEMAPHORE_REG = &g_dev.sp.regs[SP_SEMAPHORE_REG];
   rsp_info.DPC_START_REG = &g_dev.dp.dpc_regs[DPC_START_REG];
   rsp_info.DPC_END_REG = &g_dev.dp.dpc_regs[DPC_END_REG];
   rsp_info.DPC_CURRENT_REG = &g_dev.dp.dpc_regs[DPC_CURRENT_REG];
   rsp_info.DPC_STATUS_REG = &g_dev.dp.dpc_regs[DPC_STATUS_REG];
   rsp_info.DPC_CLOCK_REG = &g_dev.dp.dpc_regs[DPC_CLOCK_REG];
   rsp_info.DPC_BUFBUSY_REG = &g_dev.dp.dpc_regs[DPC_BUFBUSY_REG];
   rsp_info.DPC_PIPEBUSY_REG = &g_dev.dp.dpc_regs[DPC_PIPEBUSY_REG];
   rsp_info.DPC_TMEM_REG = &g_dev.dp.dpc_regs[DPC_TMEM_REG];
   rsp_info.CheckInterrupts = EmptyFunc;
   rsp_info.ProcessDlistList = gfx.processDList;
   rsp_info.ProcessAlistList = NULL;
   rsp_info.ProcessRdpList = gfx.processRDPList;
   rsp_info.ShowCFB = gfx.showCFB;

   /* call the RSP plugin  */
   rsp.initiateRSP(rsp_info, NULL);

   return M64ERR_SUCCESS;
}

/* global functions */
void plugin_connect_all(enum gfx_plugin_type gfx_plugin, enum rsp_plugin_type rsp_plugin)
{
   switch (gfx_plugin)
   {
      case GFX_ANGRYLION:
#ifdef HAVE_THR_AL
         gfx = gfx_angrylion;
#endif
         break;
      case GFX_PARALLEL:
#ifdef HAVE_PARALLEL
         gfx = gfx_parallel;
#endif
         break;
      case GFX_RICE:
#if defined(HAVE_OPENGL) || defined(HAVE_OPENGLES)
         gfx = gfx_rice;
         break;
#endif
      case GFX_GLN64:
#if defined(HAVE_OPENGL) || defined(HAVE_OPENGLES)
         gfx = gfx_gln64;
         break;
#endif
      default:
#if defined(HAVE_OPENGL) || defined(HAVE_OPENGLES)
         gfx = gfx_glide64;
#elif defined(HAVE_THR_AL)
         gfx = gfx_angrylion;
#endif
         break;
   }

   switch (rsp_plugin)
   {
      case RSP_CXD4:
         rsp = rsp_cxd4;
         break;
#ifdef HAVE_PARALLEL_RSP
      case RSP_PARALLEL:
         rsp = rsp_parallelRSP;
         break;
#endif
      default:
         rsp = rsp_hle;
         break;
   }

   plugin_start_gfx();
   plugin_start_input();
   plugin_start_rsp();
}
