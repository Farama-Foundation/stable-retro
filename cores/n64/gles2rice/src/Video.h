/*
Copyright (C) 2002 Rice1964

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

#ifndef _DLLINTERFACE_H_
#define _DLLINTERFACE_H_

#define M64P_PLUGIN_PROTOTYPES 1
#include "typedefs.h"
#include "m64p_config.h"
#include "m64p_plugin.h"
#include "m64p_vidext.h"

typedef struct {
    float   fViWidth, fViHeight;
    unsigned short        uViWidth, uViHeight;
    unsigned short        uDisplayWidth, uDisplayHeight;
    
    bool    bVerticalSync;

    float   fMultX, fMultY;
    int     vpLeftW, vpTopW, vpRightW, vpBottomW, vpWidthW, vpHeightW;

    struct {
        uint32_t      left;
        uint32_t      top;
        uint32_t      right;
        uint32_t      bottom;
        uint32_t      width;
        uint32_t      height;
        bool        needToClip;
    } clipping;

    int     timer;
    float   fps;    // frame per second
    float   dps;    // dlist per second
    uint32_t  lastSecFrameCount;
    uint32_t  lastSecDlistCount;
}WindowSettingStruct;

typedef enum 
{
    PRIM_TRI1,
    PRIM_TRI2,
    PRIM_TRI3,
    PRIM_DMA_TRI,
    PRIM_LINE3D,
    PRIM_TEXTRECT,
    PRIM_TEXTRECTFLIP,
    PRIM_FILLRECT,
} PrimitiveType;

typedef enum 
{
    RSP_SCISSOR,
    RDP_SCISSOR,
    UNKNOWN_SCISSOR,
} CurScissorType;

typedef struct {
    bool    bGameIsRunning;
    uint32_t  dwTvSystem;
    float   fRatio;

    bool    frameReadByCPU;
    bool    frameWriteByCPU;

    uint32_t  SPCycleCount;       // Count how many CPU cycles SP used in this DLIST
    uint32_t  DPCycleCount;       // Count how many CPU cycles DP used in this DLIST

    uint32_t  dwNumTrisRendered;
    uint32_t  dwNumDListsCulled;
    uint32_t  dwNumTrisClipped;
    uint32_t  dwNumVertices;
    uint32_t  dwBiggestVertexIndex;

    uint32_t  gDlistCount;
    uint32_t  gFrameCount;
    uint32_t  gUcodeCount;
    uint32_t  gRDPTime;
    bool    ToResize;
    uint32_t  gNewResizeWidth, gNewResizeHeight;
    bool    bDisableFPS;

    bool    bUseModifiedUcodeMap;
    bool    ucodeHasBeenSet;
    bool    bUcodeIsKnown;

    uint32_t  curRenderBuffer;
    uint32_t  curDisplayBuffer;
    uint32_t  curVIOriginReg;
    CurScissorType  curScissor;

    PrimitiveType primitiveType;

    uint32_t  lastPurgeTimeTime;      // Time textures were last purged

    bool    UseLargerTile[2];       // This is a speed up for large tile loading,
    uint32_t  LargerTileRealLeft[2];  // works only for TexRect, LoadTile, large width, large pitch

    bool    bVIOriginIsUpdated;
    bool    bCIBufferIsRendered;
    int     leftRendered,topRendered,rightRendered,bottomRendered;

    bool    isMMXSupported;

    bool    isMMXEnabled;

    bool    toShowCFB;

    bool    bAllowLoadFromTMEM;

    // Frame buffer simulation related status variables
    bool    bN64FrameBufferIsUsed;      // Frame buffer is used in the frame
    bool    bN64IsDrawingTextureBuffer; // The current N64 game is rendering into render_texture, to create self-rendering texture
    bool    bHandleN64RenderTexture;    // Do we need to handle of the N64 render_texture stuff?
    bool    bDirectWriteIntoRDRAM;      // When drawing into render_texture, this value =
                                        // = true   don't render, but write real N64 graphic value into RDRAM
                                        // = false  rendering into render_texture of DX or OGL, the render_texture
                                        //          will be copied into RDRAM at the end
    bool    bFrameBufferIsDrawn;        // flag to mark if the frame buffer is ever drawn
    bool    bFrameBufferDrawnByTriangles;   // flag to tell if the buffer is even drawn by Triangle cmds

    bool    bScreenIsDrawn;

} PluginStatus;

#define MI_INTR_DP          0x00000020  
#define MI_INTR_SP          0x00000001  

extern PluginStatus status;

#ifdef __cplusplus
extern "C" {
#endif

extern GFX_INFO gfx_info;

#ifdef __cplusplus
}
#endif

extern WindowSettingStruct windowSetting;

extern uint32_t   g_dwRamSize;

#define renderCallback ricerenderCallback

/* global functions provided by Video.cpp */
extern char generalText[];
extern void (*renderCallback)(int);
void DebugMessage(int level, const char *message, ...);

void SetVIScales();
extern void _VIDEO_DisplayTemporaryMessage2(const char *msg, ...);
extern void _VIDEO_DisplayTemporaryMessage(const char *msg);
extern void XBOX_Debugger_Log(const char *Message, ...);

#endif

