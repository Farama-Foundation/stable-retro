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

#include <math.h>
#include "Render.h"
#include "Timing.h"

void RSP_GBI1_SpNoop(Gfx *gfx)
{
    SP_Timing(RSP_GBI1_SpNoop);

    if( (gfx+1)->words.cmd == 0x00 && gRSP.ucode >= 17 )
    {
        RSP_RDP_NOIMPL("Double SPNOOP, Skip remain ucodes, PC=%08X, Cmd1=%08X", __RSP.PC[__RSP.PCi], gfx->words.w1);
        RDP_GFX_PopDL();
        //if( gRSP.ucode < 17 ) TriggerDPInterrupt();
    }
}

void RSP_GBI0_Mtx(Gfx *gfx)
{   
    SP_Timing(RSP_GBI0_Mtx);

    uint32_t addr = RSPSegmentAddr((gfx->gbi0matrix.addr));

    if (addr + 64 > g_dwRamSize)
        return;

    LoadMatrix(addr);
    
    if (gfx->gbi0matrix.projection)
        CRender::g_pRender->SetProjection(matToLoad, gfx->gbi0matrix.push, gfx->gbi0matrix.load);
    else
        CRender::g_pRender->SetWorldView(matToLoad, gfx->gbi0matrix.push, gfx->gbi0matrix.load);
}


void RSP_GBI1_Reserved(Gfx *gfx)
{
    SP_Timing(RSP_GBI1_Reserved);
    RSP_RDP_NOIMPL("RDP: Reserved (0x%08x 0x%08x)", (gfx->words.w0), (gfx->words.w1));
}


void RSP_GBI1_MoveMem(Gfx *gfx)
{
    SP_Timing(RSP_GBI1_MoveMem);

    uint32_t type      = ((gfx->words.w0)>>16)&0xFF;
    uint32_t dwLength  = ((gfx->words.w0))&0xFFFF;
    uint32_t addr      = RSPSegmentAddr((gfx->words.w1));

    switch (type) 
    {
       case F3D_MV_VIEWPORT:
          ricegSPViewport(addr);
          break;
       case G_MV_LOOKATY:
          break;
       case G_MV_LOOKATX:
          break;
       case G_MV_L0:
       case G_MV_L1:
       case G_MV_L2:
       case G_MV_L3:
       case G_MV_L4:
       case G_MV_L5:
       case G_MV_L6:
       case G_MV_L7:
          {
             uint32_t dwLight = (type - G_MV_L0)/2;
             ricegSPLight(addr, dwLight);
          }
          break;
       case G_MV_TXTATT:
          break;
       case G_MV_MATRIX_1:
          RSP_GFX_Force_Matrix(addr);
          break;
       case G_MV_MATRIX_2:
          break;
       case G_MV_MATRIX_3:
          break;
       case G_MV_MATRIX_4:
          break;
       default:
          RSP_RDP_NOIMPL("MoveMem: Unknown Move Type, cmd=%08X, %08X", gfx->words.w0, gfx->words.w1);
          break;
    }
}

void RSP_GBI0_Vtx(Gfx *gfx)
{
    SP_Timing(RSP_GBI0_Vtx);

    int n = gfx->gbi0vtx.n + 1;
    int v0 = gfx->gbi0vtx.v0;
    uint32_t addr = RSPSegmentAddr((gfx->gbi0vtx.addr));

    if ((v0 + n) > 80)
    {
        TRACE3("Warning, invalid vertex positions, N=%d, v0=%d, Addr=0x%08X", n, v0, addr);
        n = 32 - v0;
    }

    // Check that address is valid...
    if ((addr + n*16) > g_dwRamSize)
    {
        TRACE1("Vertex Data: Address out of range (0x%08x)", addr);
    }
    else
    {
        ProcessVertexData(addr, v0, n);
        status.dwNumVertices += n;
        DisplayVertexInfo(addr, v0, n);
    }
}


void RSP_GBI0_DL(Gfx *gfx)
{
    SP_Timing(RSP_GBI0_DL);

    uint32_t addr = RSPSegmentAddr((gfx->gbi0dlist.addr)) & (g_dwRamSize-1);

    if( addr > g_dwRamSize )
    {
        RSP_RDP_NOIMPL("Error: DL addr = %08X out of range, PC=%08X", addr, __RSP.PC[__RSP.PCi] );
        addr &= (g_dwRamSize-1);
    }

    if( gfx->gbi0dlist.param == G_DL_PUSH )
        __RSP.PCi++;

    __RSP.PC[__RSP.PCi]        = addr;
    __RSP.countdown[__RSP.PCi] = MAX_DL_COUNT;
}


void RSP_GBI1_RDPHalf_Cont(Gfx *gfx)    
{
    SP_Timing(RSP_GBI1_RDPHalf_Cont);
}

void RSP_GBI1_RDPHalf_2(Gfx *gfx)       
{ 
    SP_Timing(RSP_GBI1_RDPHalf_2);
}

void RSP_GBI1_RDPHalf_1(Gfx *gfx)       
{
    SP_Timing(RSP_GBI1_RDPHalf_1);
}

void RSP_GBI1_Line3D(Gfx *gfx)
{
    status.primitiveType = PRIM_LINE3D;

    uint32_t dwPC = __RSP.PC[__RSP.PCi];

    bool bTrisAdded = false;

    if( gfx->ln3dtri2.v3 == 0 )
    {
        // Flying Dragon
        uint32_t dwV0     = gfx->ln3dtri2.v0/gRSP.vertexMult;
        uint32_t dwV1     = gfx->ln3dtri2.v1/gRSP.vertexMult;
        uint32_t dwWidth  = gfx->ln3dtri2.v2;
        //uint32_t dwFlag = gfx->ln3dtri2.v3/gRSP.vertexMult; 
        
        CRender::g_pRender->SetCombinerAndBlender();

        status.dwNumTrisRendered++;

        CRender::g_pRender->Line3D(dwV0, dwV1, dwWidth);
        SP_Timing(RSP_GBI1_Line3D);
        DP_Timing(RSP_GBI1_Line3D);
    }
    else
    {
        do {
            uint32_t dwV3  = gfx->ln3dtri2.v3/gRSP.vertexMult;        
            uint32_t dwV0  = gfx->ln3dtri2.v0/gRSP.vertexMult;
            uint32_t dwV1  = gfx->ln3dtri2.v1/gRSP.vertexMult;
            uint32_t dwV2  = gfx->ln3dtri2.v2/gRSP.vertexMult;

            // Do first tri
            if (IsTriangleVisible(dwV0, dwV1, dwV2))
            {
                DEBUG_DUMP_VERTEXES("Line3D 1/2", dwV0, dwV1, dwV2);
                if (!bTrisAdded && CRender::g_pRender->IsTextureEnabled())
                {
                    PrepareTextures();
                    InitVertexTextureConstants();
                }

                if( !bTrisAdded )
                {
                    CRender::g_pRender->SetCombinerAndBlender();
                }

                bTrisAdded = true;
                PrepareTriangle(dwV0, dwV1, dwV2);
            }

            // Do second tri
            if (IsTriangleVisible(dwV2, dwV3, dwV0))
            {
                DEBUG_DUMP_VERTEXES("Line3D 2/2", dwV0, dwV1, dwV2);
                if (!bTrisAdded && CRender::g_pRender->IsTextureEnabled())
                {
                    PrepareTextures();
                    InitVertexTextureConstants();
                }

                if( !bTrisAdded )
                {
                    CRender::g_pRender->SetCombinerAndBlender();
                }

                bTrisAdded = true;
                PrepareTriangle(dwV2, dwV3, dwV0);
            }

            gfx++;
            dwPC += 8;
        } while (gfx->words.cmd == (uint8_t)RSP_LINE3D);

        __RSP.PC[__RSP.PCi] = dwPC-8;

        if (bTrisAdded) 
            CRender::g_pRender->DrawTriangles();
    }
}


void RSP_GBI1_ClearGeometryMode(Gfx *gfx)
{
    SP_Timing(RSP_GBI1_ClearGeometryMode);
    uint32_t dwMask = ((gfx->words.w1));

    gSP.geometryMode &= ~dwMask;
    RSP_GFX_InitGeometryMode();
}



void RSP_GBI1_SetGeometryMode(Gfx *gfx)
{
    SP_Timing(RSP_GBI1_SetGeometryMode);
    uint32_t dwMask = ((gfx->words.w1));

    gSP.geometryMode |= dwMask;
    RSP_GFX_InitGeometryMode();
}


void RSP_GBI1_EndDL(Gfx *gfx)
{
    SP_Timing(RSP_GBI1_EndDL);
    RDP_GFX_PopDL();
}


void RSP_GBI1_SetOtherModeL(Gfx *gfx)
{
    SP_Timing(RSP_GBI1_SetOtherModeL);

    uint32_t dwShift = ((gfx->words.w0)>>8)&0xFF;
    uint32_t dwLength= ((gfx->words.w0)   )&0xFF;
    uint32_t dwData  = (gfx->words.w1);

    uint32_t dwMask = ((1<<dwLength)-1)<<dwShift;

    uint32_t modeL = gDP.otherMode.l;
    modeL = (modeL&(~dwMask)) | dwData;

    Gfx tempgfx;
    tempgfx.words.w0 = gDP.otherMode.h;
    tempgfx.words.w1 = modeL;
    DLParser_RDPSetOtherMode(&tempgfx);
}


void RSP_GBI1_SetOtherModeH(Gfx *gfx)
{
    SP_Timing(RSP_GBI1_SetOtherModeH);

    uint32_t dwShift = ((gfx->words.w0)>>8)&0xFF;
    uint32_t dwLength= ((gfx->words.w0)   )&0xFF;
    uint32_t dwData  = (gfx->words.w1);

    uint32_t dwMask = ((1<<dwLength)-1)<<dwShift;
    uint32_t dwModeH = gDP.otherMode.h;

    dwModeH = (dwModeH&(~dwMask)) | dwData;
    Gfx tempgfx;
    tempgfx.words.w0 = dwModeH;
    tempgfx.words.w1 = gDP.otherMode.l;
    DLParser_RDPSetOtherMode(&tempgfx );
}


void RSP_GBI1_Texture(Gfx *gfx)
{
    SP_Timing(RSP_GBI1_Texture);

    float fTextureScaleS = (float)(gfx->texture.scaleS) / (65536.0f * 32.0f);
    float fTextureScaleT = (float)(gfx->texture.scaleT) / (65536.0f * 32.0f);

    if( (((gfx->words.w1)>>16)&0xFFFF) == 0xFFFF )
        fTextureScaleS = 1/32.0f;
    else if( (((gfx->words.w1)>>16)&0xFFFF) == 0x8000 )
        fTextureScaleS = 1/64.0f;

    if( (((gfx->words.w1)    )&0xFFFF) == 0xFFFF )
        fTextureScaleT = 1/32.0f;
    else if( (((gfx->words.w1)    )&0xFFFF) == 0x8000 )
        fTextureScaleT = 1/64.0f;

    if( gRSP.ucode == 6 )
    {
        if( fTextureScaleS == 0 )   fTextureScaleS = 1.0f/32.0f;
        if( fTextureScaleT == 0 )   fTextureScaleT = 1.0f/32.0f;
    }

    CRender::g_pRender->SetTextureEnableAndScale(gfx->texture.tile, gfx->texture.enable_gbi0, fTextureScaleS, fTextureScaleT);

    /* What happens if these are 0? Interpret as 1.0f? */
}

extern void RSP_RDP_InsertMatrix(uint32_t word0, uint32_t word1);

void RSP_GBI1_MoveWord(Gfx *gfx)
{
   SP_Timing(RSP_GBI1_MoveWord);

   switch (gfx->gbi0moveword.type)
   {
      case G_MW_MATRIX:
         RSP_RDP_InsertMatrix(gfx);
         break;
      case G_MW_NUMLIGHT:
         {
            uint32_t dwNumLights = (((gfx->gbi0moveword.value)-0x80000000)/32)-1;

            gRSP.ambientLightIndex = dwNumLights;
            ricegSPNumLights(dwNumLights);
         }
         break;
      case G_MW_CLIP:
         {
            switch (gfx->gbi0moveword.offset)
            {
               case G_MWO_CLIP_RNX:
               case G_MWO_CLIP_RNY:
               case G_MWO_CLIP_RPX:
               case G_MWO_CLIP_RPY:
                  CRender::g_pRender->SetClipRatio(gfx->gbi0moveword.offset, gfx->gbi0moveword.value);
                  break;
               default:
                  break;
            }
         }
         break;
      case G_MW_SEGMENT:
         {
            uint32_t dwSegment = (gfx->gbi0moveword.offset >> 2) & 0xF;
            uint32_t dwBase    = (gfx->gbi0moveword.value)&0x00FFFFFF;

            gSP.segment[dwSegment] = dwBase;
         }
         break;
      case G_MW_FOG:
         {
            uint16_t wMult = (uint16_t)(((gfx->gbi0moveword.value) >> 16) & 0xFFFF);
            uint16_t wOff  = (uint16_t)(((gfx->gbi0moveword.value)      ) & 0xFFFF);

            float fMult    = (float)(short)wMult;
            float fOff     = (float)(short)wOff;

            float rng      = 128000.0f / fMult;
            float fMin     = 500.0f - (fOff*rng/256.0f);
            float fMax     = rng + fMin;

#if 0
            if( fMult <= 0 || fMin > fMax || fMax < 0 || fMin > 1000 )
#else
            if( fMult <= 0 || fMax < 0 )
#endif
            {
               // Hack
               fMin = 996;
               fMax = 1000;
               fMult = 0;
               fOff = 1;
            }

            SetFogMinMax(fMin, fMax, fMult, fOff);
         }
         break;
      case G_MW_LIGHTCOL:
         {
            uint32_t dwLight = gfx->gbi0moveword.offset / 0x20;
            uint32_t dwField = (gfx->gbi0moveword.offset & 0x7);

            switch (dwField)
            {
               case 0:
                  if (dwLight == gRSP.ambientLightIndex)
                     SetAmbientLight( ((gfx->gbi0moveword.value)>>8) );
                  else
                     ricegSPLightColor(dwLight, gfx->gbi0moveword.value);
                  break;

               case 4:
                  break;

               default:
                  break;
            }
         }
         break;
      case G_MW_POINTS:
         {
            uint32_t vtx   = gfx->gbi0moveword.offset/40;
            uint32_t where = gfx->gbi0moveword.offset - vtx*40;
            ricegSPModifyVertex(vtx, where, gfx->gbi0moveword.value);
         }
         break;
      case G_MW_PERSPNORM:
         //if( word1 != 0x1A ) DebuggerAppendMsg("PerspNorm: 0x%04x", (short)word1); 
         break;
      default:
         RSP_RDP_NOIMPL("Unknown MoveWord, %08X, %08X", gfx->words.w0, gfx->words.w1);
         break;
   }

}


void RSP_GBI1_PopMtx(Gfx *gfx)
{
    SP_Timing(RSP_GBI1_PopMtx);

    // Do any of the other bits do anything?
    // So far only Extreme-G seems to Push/Pop projection matrices

    if (gfx->gbi0popmatrix.projection)
        CRender::g_pRender->PopProjection();
    else
        CRender::g_pRender->PopWorldView();
}


void RSP_GBI1_CullDL(Gfx *gfx)
{
    SP_Timing(RSP_GBI1_CullDL);

    if( g_curRomInfo.bDisableCulling )
        return; //Disable Culling

    uint32_t dwVFirst = ((gfx->words.w0) & 0xFFF) / gRSP.vertexMult;
    uint32_t dwVLast  = (((gfx->words.w1)) & 0xFFF) / gRSP.vertexMult;

    // Mask into range
    dwVFirst &= 0x1f;
    dwVLast &= 0x1f;

    if( dwVLast < dwVFirst )
       return;
    if( !gRSP.bRejectVtx )
       return;

    for (uint32_t i = dwVFirst; i <= dwVLast; i++)
    {
        if (g_clipFlag[i] == 0)
        {
            LOG_UCODE("    Vertex %d is visible, continuing with display list processing", i);
            return;
        }
    }

    status.dwNumDListsCulled++;

    LOG_UCODE("    No vertices were visible, culling rest of display list");

    RDP_GFX_PopDL();
}



void RSP_GBI1_Tri1(Gfx *gfx)
{
    status.primitiveType = PRIM_TRI1;
    bool bTrisAdded = false;
    bool bTexturesAreEnabled = CRender::g_pRender->IsTextureEnabled();

    // While the next command pair is Tri1, add vertices
    uint32_t dwPC = __RSP.PC[__RSP.PCi];
    
    do
    {
        uint32_t dwV0 = gfx->tri1.v0/gRSP.vertexMult;
        uint32_t dwV1 = gfx->tri1.v1/gRSP.vertexMult;
        uint32_t dwV2 = gfx->tri1.v2/gRSP.vertexMult;

        if (IsTriangleVisible(dwV0, dwV1, dwV2))
        {
            if (!bTrisAdded)
            {
                if( bTexturesAreEnabled )
                {
                    PrepareTextures();
                    InitVertexTextureConstants();
                }
                CRender::g_pRender->SetCombinerAndBlender();
                bTrisAdded = true;
            }
            PrepareTriangle(dwV0, dwV1, dwV2);
        }

        gfx++;
        dwPC += 8;

    } while (gfx->words.cmd == (uint8_t)RSP_TRI1);

    __RSP.PC[__RSP.PCi] = dwPC-8;

    if (bTrisAdded) 
        CRender::g_pRender->DrawTriangles();
}


void RSP_GBI0_Tri4(Gfx *gfx)
{
    uint8_t *rdram_u8 = (uint8_t*)gfx_info.RDRAM;
    uint32_t w0 = gfx->words.w0;
    uint32_t w1 = gfx->words.w1;

    status.primitiveType = PRIM_TRI2;

    // While the next command pair is Tri2, add vertices
    uint32_t dwPC = __RSP.PC[__RSP.PCi];

    bool bTrisAdded = false;

    do
    {
       uint32_t dwFlag = (w0>>16)&0xFF;

       for( int i=0; i<4; i++)
       {
          uint32_t v0   = (w1>>(4+(i<<3))) & 0xF;
          uint32_t v1   = (w1>>(  (i<<3))) & 0xF;
          uint32_t v2   = (w0>>(  (i<<2))) & 0xF;
          bool bVisible = IsTriangleVisible(v0, v2, v1);

          if (bVisible)
          {
             DEBUG_DUMP_VERTEXES("Tri4_PerfectDark 1/2", v0, v1, v2);
             if (!bTrisAdded && CRender::g_pRender->IsTextureEnabled())
             {
                PrepareTextures();
                InitVertexTextureConstants();
             }

             if( !bTrisAdded )
                CRender::g_pRender->SetCombinerAndBlender();

             bTrisAdded = true;
             PrepareTriangle(v0, v2, v1);
          }
       }

       w0  = *(uint32_t *)(rdram_u8 + dwPC+0);
       w1  = *(uint32_t *)(rdram_u8 + dwPC+4);
       dwPC += 8;

    } while (((w0)>>24) == (uint8_t)RSP_TRI2);

    __RSP.PC[__RSP.PCi] = dwPC-8;

    if (bTrisAdded) 
        CRender::g_pRender->DrawTriangles();
    
    gRSP.DKRVtxCount=0;
}

//Nintro64 uses Sprite2d 


void RSP_RDP_Nothing(Gfx *gfx)
{
    SP_Timing(RSP_RDP_Nothing);

    if( options.bEnableHacks )
        return;
    
    __RSP.PCi = -1;
}


void RSP_RDP_InsertMatrix(Gfx *gfx)
{
    float fraction;

    UpdateCombinedMatrix();

    if ((gfx->words.w0) & 0x20)
    {
        int x = ((gfx->words.w0) & 0x1F) >> 1;
        int y = x >> 2;
        x &= 3;

        fraction = ((gfx->words.w1)>>16)/65536.0f;
        gRSPworldProject.m[y][x] = (float)(int)gRSPworldProject.m[y][x];
        gRSPworldProject.m[y][x] += fraction;

        fraction = ((gfx->words.w1)&0xFFFF)/65536.0f;
        gRSPworldProject.m[y][x+1] = (float)(int)gRSPworldProject.m[y][x+1];
        gRSPworldProject.m[y][x+1] += fraction;
    }
    else
    {
       int x = ((gfx->words.w0) & 0x1F) >> 1;
       int y = x >> 2;
       x &= 3;

       float integer = (float)(short)((gfx->words.w1)>>16);
       fraction = (float)fabs(gRSPworldProject.m[y][x] - (int)gRSPworldProject.m[y][x]);

       if(integer >= 0.0f)
          gRSPworldProject.m[y][x] = integer + fraction;
       else
          gRSPworldProject.m[y][x] = integer - fraction;


       integer = (float)(short)((gfx->words.w1)&0xFFFF);
       fraction = (float)fabs(gRSPworldProject.m[y][x+1] - (int)gRSPworldProject.m[y][x+1]);

       if(integer >= 0.0f)
          gRSPworldProject.m[y][x+1] = integer + fraction;
       else
          gRSPworldProject.m[y][x+1] = integer - fraction;
    }

    gRSP.bMatrixIsUpdated = false;
    gRSP.bCombinedMatrixIsUpdated = true;
}

