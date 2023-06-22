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

#include "Render.h"
#include "Timing.h"

void RSP_GBI2_Vtx(Gfx *gfx)
{
    uint32_t addr = RSPSegmentAddr((gfx->gbi2vtx.addr));
    int vend    = gfx->gbi2vtx.vend/2;
    int n       = gfx->gbi2vtx.n;
    int v0      = vend - n;

    if( vend > 64 )
    {
        DebuggerAppendMsg("Warning, attempting to load into invalid vertex positions, v0=%d, n=%d", v0, n);
        return;
    }

    if ((addr + (n*16)) > g_dwRamSize)
    {
        DebuggerAppendMsg("ProcessVertexData: Address out of range (0x%08x)", addr);
    }
    else
    {
        ProcessVertexData(addr, v0, n);
        status.dwNumVertices += n;
        DisplayVertexInfo(addr, v0, n);
    }
}

void RSP_GBI2_EndDL(Gfx *gfx)
{
    SP_Timing(RSP_GBI1_EndDL);

    RDP_GFX_PopDL();
}

void RSP_GBI2_CullDL(Gfx *gfx)
{
    SP_Timing(RSP_GBI1_CullDL);

    if( g_curRomInfo.bDisableCulling )
        return; //Disable Culling

    uint32_t dwVFirst = (((gfx->words.w0)) & 0xfff) / gRSP.vertexMult;
    uint32_t dwVLast  = (((gfx->words.w1)) & 0xfff) / gRSP.vertexMult;

    // Mask into range
    dwVFirst &= 0x1f;
    dwVLast &= 0x1f;

    if( dwVLast < dwVFirst ) return;
    if( !gRSP.bRejectVtx )   return;

    for (uint32_t i = dwVFirst; i <= dwVLast; i++)
    {
        //if (g_dwVtxFlags[i] == 0)
        if (g_clipFlag[i] == 0)
        {
            LOG_UCODE("    Vertex %d is visible, returning", i);
            return;
        }
    }

    status.dwNumDListsCulled++;

    LOG_UCODE("    No vertices were visible, culling");

    RDP_GFX_PopDL();
}

void RSP_GBI2_MoveWord(Gfx *gfx)
{
    SP_Timing(RSP_GBI1_MoveWord);

    switch (gfx->gbi2moveword.type)
    {
    case G_MW_MATRIX:
        RSP_RDP_InsertMatrix(gfx);
        break;
    case G_MW_NUMLIGHT:
        {
            uint32_t dwNumLights = gfx->gbi2moveword.value/24;
            gRSP.ambientLightIndex = dwNumLights;
            ricegSPNumLights(dwNumLights);
        }
        break;

    case G_MW_CLIP:
        {
            switch (gfx->gbi2moveword.offset)
            {
               case G_MWO_CLIP_RNX:
               case G_MWO_CLIP_RNY:
               case G_MWO_CLIP_RPX:
               case G_MWO_CLIP_RPY:
                  CRender::g_pRender->SetClipRatio(gfx->gbi2moveword.offset, gfx->gbi2moveword.value);
               default:
                  break;
            }
        }
        break;

    case G_MW_SEGMENT:
        {
            uint32_t dwSeg       = gfx->gbi2moveword.offset / 4;
            uint32_t dwAddr      = gfx->gbi2moveword.value & 0x00FFFFFF;           // Hack - convert to physical

            gSP.segment[dwSeg] = dwAddr;
        }
        break;
    case G_MW_FOG:
        {
            uint16_t wMult = (uint16_t)((gfx->gbi2moveword.value >> 16) & 0xFFFF);
            uint16_t wOff  = (uint16_t)((gfx->gbi2moveword.value      ) & 0xFFFF);

            float fMult    = (float)(short)wMult;
            float fOff     = (float)(short)wOff;

            float rng      = 128000.0f / fMult;
            float fMin     = 500.0f - (fOff*rng/256.0f);
            float fMax     = rng + fMin;

            if( fMult <= 0 || fMax < 0 )
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
            uint32_t dwLight = gfx->gbi2moveword.offset / 0x18;
            uint32_t dwField = (gfx->gbi2moveword.offset & 0x7);

            switch (dwField)
            {
               case 0:
                  if (dwLight == gRSP.ambientLightIndex)
                     SetAmbientLight( (gfx->gbi2moveword.value>>8) );
                  else
                     ricegSPLightColor(dwLight, gfx->gbi2moveword.value);
                  break;

               case 4:
                  break;

               default:
                  break;
            }


        }
        break;

    case G_MW_PERSPNORM:
        LOG_UCODE("     RSP_MOVE_WORD_PERSPNORM 0x%04x", (short)gfx->words.w1);
        break;

    case G_MW_POINTS:
        LOG_UCODE("     2nd cmd of Force Matrix");
        break;

    default:
        break;
    }
}

void RSP_GBI2_Tri1(Gfx *gfx)
{
    if( gfx->words.w0 == 0x05000017 && gfx->gbi2tri1.flag == 0x80 )
    {
        // The ObjLoadTxtr / Tlut cmd for Evangelion.v64
        RSP_S2DEX_SPObjLoadTxtr(gfx);
        DebuggerAppendMsg("Fix me, SPObjLoadTxtr as RSP_GBI2_Tri2");
    }
    else
    {
        status.primitiveType = PRIM_TRI1;
        bool bTrisAdded = false;
        bool bTexturesAreEnabled = CRender::g_pRender->IsTextureEnabled();

        // While the next command pair is Tri1, add vertices
        uint32_t dwPC = __RSP.PC[__RSP.PCi];

        do
        {
            uint32_t dwV2 = gfx->gbi2tri1.v2/gRSP.vertexMult;
            uint32_t dwV1 = gfx->gbi2tri1.v1/gRSP.vertexMult;
            uint32_t dwV0 = gfx->gbi2tri1.v0/gRSP.vertexMult;

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

        } while( gfx->words.cmd == (uint8_t)RSP_ZELDATRI1);

        __RSP.PC[__RSP.PCi] = dwPC-8;

        if (bTrisAdded) 
            CRender::g_pRender->DrawTriangles();
    }
}

void RSP_GBI2_Tri2(Gfx *gfx)
{
    if( gfx->words.w0 == 0x0600002f && gfx->gbi2tri2.flag == 0x80 )
    {
        // The ObjTxSprite cmd for Evangelion.v64
        RSP_S2DEX_SPObjLoadTxSprite(gfx);
        DebuggerAppendMsg("Fix me, SPObjLoadTxSprite as RSP_GBI2_Tri2");
    }
    else
    {
        status.primitiveType = PRIM_TRI2;
        bool bTrisAdded = false;

        // While the next command pair is Tri2, add vertices
        uint32_t dwPC = __RSP.PC[__RSP.PCi];
        bool bTexturesAreEnabled = CRender::g_pRender->IsTextureEnabled();

        do {
            uint32_t dwV2 = gfx->gbi2tri2.v2;
            uint32_t dwV1 = gfx->gbi2tri2.v1;
            uint32_t dwV0 = gfx->gbi2tri2.v0;

            uint32_t dwV5 = gfx->gbi2tri2.v5;
            uint32_t dwV4 = gfx->gbi2tri2.v4;
            uint32_t dwV3 = gfx->gbi2tri2.v3;

            // Do first tri
            if (IsTriangleVisible(dwV0, dwV1, dwV2))
            {
                DEBUG_DUMP_VERTEXES("ZeldaTri2 1/2", dwV0, dwV1, dwV2);
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

            // Do second tri
            if (IsTriangleVisible(dwV3, dwV4, dwV5))
            {
                DEBUG_DUMP_VERTEXES("ZeldaTri2 2/2", dwV3, dwV4, dwV5);
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

                PrepareTriangle(dwV3, dwV4, dwV5);
            }
            
            gfx++;
            dwPC += 8;

        } while ( gfx->words.cmd == (uint8_t)RSP_ZELDATRI2 );//&& status.dwNumTrisRendered < 50);


        __RSP.PC[__RSP.PCi] = dwPC-8;

        if (bTrisAdded) 
            CRender::g_pRender->DrawTriangles();
    }
}

void RSP_GBI2_Line3D(Gfx *gfx)
{
    if( gfx->words.w0 == 0x0700002f && (gfx->words.w1>>24) == 0x80 )
    {
        // The ObjTxSprite cmd for Evangelion.v64
        RSP_S2DEX_SPObjLoadTxRect(gfx);
    }
    else
    {
        status.primitiveType = PRIM_TRI3;
        uint32_t dwPC        = __RSP.PC[__RSP.PCi];

        bool bTrisAdded = false;

        do {
            uint32_t dwV0 = gfx->gbi2line3d.v0/gRSP.vertexMult;
            uint32_t dwV1 = gfx->gbi2line3d.v1/gRSP.vertexMult;
            uint32_t dwV2 = gfx->gbi2line3d.v2/gRSP.vertexMult;

            uint32_t dwV3 = gfx->gbi2line3d.v3/gRSP.vertexMult;
            uint32_t dwV4 = gfx->gbi2line3d.v4/gRSP.vertexMult;
            uint32_t dwV5 = gfx->gbi2line3d.v5/gRSP.vertexMult;

            // Do first tri
            if (IsTriangleVisible(dwV0, dwV1, dwV2))
            {
                DEBUG_DUMP_VERTEXES("ZeldaTri3 1/2", dwV0, dwV1, dwV2);
                if (!bTrisAdded && CRender::g_pRender->IsTextureEnabled())
                {
                    PrepareTextures();
                    InitVertexTextureConstants();
                }

                if( !bTrisAdded )
                    CRender::g_pRender->SetCombinerAndBlender();

                bTrisAdded = true;
                PrepareTriangle(dwV0, dwV1, dwV2);
            }

            // Do second tri
            if (IsTriangleVisible(dwV3, dwV4, dwV5))
            {
                if (!bTrisAdded && CRender::g_pRender->IsTextureEnabled())
                {
                    PrepareTextures();
                    InitVertexTextureConstants();
                }

                if( !bTrisAdded )
                    CRender::g_pRender->SetCombinerAndBlender();

                bTrisAdded = true;
                PrepareTriangle(dwV3, dwV4, dwV5);
            }
            
            gfx++;
            dwPC += 8;

        } while ( gfx->words.cmd == (uint8_t)RSP_LINE3D);

        __RSP.PC[__RSP.PCi] = dwPC-8;


        if (bTrisAdded) 
            CRender::g_pRender->DrawTriangles();
    }
}

void RSP_GBI2_Texture(Gfx *gfx)
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

    CRender::g_pRender->SetTextureEnableAndScale(gfx->texture.tile, gfx->texture.enable_gbi2, fTextureScaleS, fTextureScaleT);

    /*
    if( g_curRomInfo.bTextureScaleHack )
    {
    // Hack, need to verify, refer to N64 programming manual
    // that if scale = 0.5 (1/64), vtx s,t are also doubled

        if( ((word1>>16)&0xFFFF) == 0x8000 )
        {
            fTextureScaleS = 1/128.0f;
            if( ((word1)&0xFFFF) == 0xFFFF )
            {
                fTextureScaleT = 1/64.0f;
            }
        }

        if( ((word1    )&0xFFFF) == 0x8000 )
        {
            fTextureScaleT = 1/128.0f;
            if( ((word1>>16)&0xFFFF) == 0xFFFF )
            {
                fTextureScaleS = 1/64.0f;
            }
        }
    }
    */

    CRender::g_pRender->SetTextureEnableAndScale(gfx->texture.tile, gfx->texture.enable_gbi2, fTextureScaleS, fTextureScaleT);
}



void RSP_GBI2_PopMtx(Gfx *gfx)
{
   /* G_MTX_PROJECTION might be Zelda-specific */

   SP_Timing(RSP_GBI1_PopMtx);

   uint8_t nCommand = (uint8_t)(gfx->words.w0 & 0xFF);

   LOG_UCODE("        PopMtx: 0x%02x (%s)",
         nCommand, 
         (nCommand & G_MTX_PROJECTION) ? "Projection" : "ModelView");


   /*  if (nCommand & G_MTX_PROJECTION)
       {
       CRender::g_pRender->PopProjection();
       }
       else*/
   {
      CRender::g_pRender->PopWorldView();
   }

}


#define RSP_ZELDA_ZBUFFER             0x00000001      // Guess
#define RSP_ZELDA_CULL_BACK           0x00000200
#define RSP_ZELDA_CULL_FRONT          0x00000400
#define RSP_ZELDA_FOG                 0x00010000
#define RSP_ZELDA_LIGHTING            0x00020000
#define RSP_ZELDA_TEXTURE_GEN         0x00040000
#define RSP_ZELDA_TEXTURE_GEN_LINEAR  0x00080000
#define RSP_ZELDA_SHADING_SMOOTH      0x00200000

void RSP_GBI2_GeometryMode(Gfx *gfx)
{
    SP_Timing(RSP_GBI2_GeometryMode);

    uint32_t dwAnd = ((gfx->words.w0)) & 0x00FFFFFF;
    uint32_t dwOr  = ((gfx->words.w1)) & 0x00FFFFFF;

    gSP.geometryMode &= dwAnd;
    gSP.geometryMode |= dwOr;


    bool bCullFront     = (gSP.geometryMode & RSP_ZELDA_CULL_FRONT) ? true : false;
    bool bCullBack      = (gSP.geometryMode & RSP_ZELDA_CULL_BACK) ? true : false;
    
    //bool bShade         = (gSP.geometryMode & G_SHADE) ? true : false;
    //bool bFlatShade     = (gSP.geometryMode & RSP_ZELDA_SHADING_SMOOTH) ? true : false;
    bool bFlatShade     = (gSP.geometryMode & RSP_ZELDA_TEXTURE_GEN_LINEAR) ? true : false;
    if( options.enableHackForGames == HACK_FOR_TIGER_HONEY_HUNT )
        bFlatShade      = false;
    
    bool bFog           = (gSP.geometryMode & RSP_ZELDA_FOG) ? true : false;
    bool bTextureGen    = (gSP.geometryMode & RSP_ZELDA_TEXTURE_GEN) ? true : false;

    bool bLighting      = (gSP.geometryMode & RSP_ZELDA_LIGHTING) ? true : false;
    bool bZBuffer       = (gSP.geometryMode & RSP_ZELDA_ZBUFFER)  ? true : false; 

    CRender::g_pRender->SetCullMode(bCullFront, bCullBack);
    
    //if (bFlatShade||!bShade)  CRender::g_pRender->SetShadeMode( SHADE_FLAT );
    if (bFlatShade) CRender::g_pRender->SetShadeMode( SHADE_FLAT );
    else            CRender::g_pRender->SetShadeMode( SHADE_SMOOTH );
    
    SetTextureGen(bTextureGen);

    SetLighting( bLighting );
    CRender::g_pRender->ZBufferEnable( bZBuffer );
    CRender::g_pRender->SetFogEnable( bFog );
}


int dlistMtxCount=0;
extern uint32_t dwConkerVtxZAddr;

void RSP_GBI2_Mtx(Gfx *gfx)
{   
    SP_Timing(RSP_GBI0_Mtx);
    dwConkerVtxZAddr = 0;   // For Conker BFD

    uint32_t addr = RSPSegmentAddr((gfx->gbi2matrix.addr));

    if( gfx->gbi2matrix.param == 0 && gfx->gbi2matrix.len == 0 )
    {
        DLParser_Bomberman2TextRect(gfx);
        return;
    }

    if (addr + 64 > g_dwRamSize)
    {
        DebuggerAppendMsg("ZeldaMtx: Address invalid (0x%08x)", addr);
        return;
    }

    LoadMatrix(addr);

    // So far only Extreme-G seems to Push/Pop projection matrices  
    if (gfx->gbi2matrix.projection)
        CRender::g_pRender->SetProjection(matToLoad, gfx->gbi2matrix.nopush==0, gfx->gbi2matrix.load);
    else
    {
        CRender::g_pRender->SetWorldView(matToLoad, gfx->gbi2matrix.nopush==0, gfx->gbi2matrix.load);

        if( options.enableHackForGames == HACK_FOR_SOUTH_PARK_RALLY )
        {
            dlistMtxCount++;
            if( dlistMtxCount == 2 )
            {
                CRender::g_pRender->ClearZBuffer(1.0f);
            }
        }
    }
}

void RSP_GBI2_MoveMem(Gfx *gfx)
{
   SP_Timing(RSP_GBI1_MoveMem);

   uint32_t addr = RSPSegmentAddr((gfx->words.w1));
   uint32_t type    = ((gfx->words.w0)     ) & 0xFE;

   //uint32_t dwLen = ((gfx->words.w0) >> 16) & 0xFF;
   //uint32_t dwOffset = ((gfx->words.w0) >> 8) & 0xFFFF;

   switch (type)
   {
      case RSP_GBI2_MV_MEM__VIEWPORT:
         ricegSPViewport(addr);
         break;
      case RSP_GBI2_MV_MEM__LIGHT:
         {
            int8_t *rdram_s8 = (int8_t*)gfx_info.RDRAM;
            uint32_t dwOffset2 = ((gfx->words.w0) >> 5) & 0x3FFF;
            switch (dwOffset2)
            {
               case 0x00:
                  {
                     int8_t * pcBase = rdram_s8 + addr;
                     LOG_UCODE("    RSP_GBI1_MV_MEM_LOOKATX %f %f %f",
                           (float)pcBase[8 ^ 0x3],
                           (float)pcBase[9 ^ 0x3],
                           (float)pcBase[10 ^ 0x3]);

                  }
                  break;
               case 0x18:
                  {
                     int8_t * pcBase = rdram_s8 + addr;
                     LOG_UCODE("    RSP_GBI1_MV_MEM_LOOKATY %f %f %f",
                           (float)pcBase[8 ^ 0x3],
                           (float)pcBase[9 ^ 0x3],
                           (float)pcBase[10 ^ 0x3]);
                  }
                  break;
               default:        //0x30/48/60
                  {
                     uint32_t dwLight = (dwOffset2 - 0x30)/0x18;
                     ricegSPLight(addr, dwLight);
                  }
                  break;
            }
            break;

         }
      case RSP_GBI2_MV_MEM__MATRIX:
         LOG_UCODE("Force Matrix: addr=%08X", addr);
         RSP_GFX_Force_Matrix(addr);
         break;
      case G_MVO_L0:
      case G_MVO_L1:
      case G_MVO_L2:
      case G_MVO_L3:
      case G_MVO_L4:
      case G_MVO_L5:
      case G_MVO_L6:
      case G_MVO_L7:
         LOG_UCODE("Zelda Move Light");
         RDP_NOIMPL_WARN("Zelda Move Light");
         break;

      case RSP_GBI2_MV_MEM__POINT:
         LOG_UCODE("Zelda Move Point");
         void RDP_NOIMPL_WARN(const char* op);
         RDP_NOIMPL_WARN("Zelda Move Point");
         break;

      case G_MVO_LOOKATX:
         if( (gfx->words.w0) == 0xDC170000 && ((gfx->words.w1)&0xFF000000) == 0x80000000 )
         {
            // Ucode for Evangelion.v64, the ObjMatrix cmd
            RSP_S2DEX_OBJ_MOVEMEM(gfx);
         }
         break;
      case G_MVO_LOOKATY:
         RSP_RDP_NOIMPL("Not implemented ZeldaMoveMem LOOKATY, Cmd0=0x%08X, Cmd1=0x%08X", gfx->words.w0, gfx->words.w1);
         break;
      case 0x02:
         if( (gfx->words.w0) == 0xDC070002 && ((gfx->words.w1)&0xFF000000) == 0x80000000 )
         {
            RSP_S2DEX_OBJ_MOVEMEM(gfx);
            break;
         }
      default:
         LOG_UCODE("ZeldaMoveMem Type: Unknown");
         RSP_RDP_NOIMPL("Unknown ZeldaMoveMem Type, type=0x%X, Addr=%08X", type, addr);
         break;
   }
}

void RSP_GBI2_DL(Gfx *gfx)
{
    SP_Timing(RSP_GBI0_DL);

    uint32_t dwPush = ((gfx->words.w0) >> 16) & 0xFF;
    uint32_t dwAddr = RSPSegmentAddr((gfx->words.w1));

    if( dwAddr > g_dwRamSize )
    {
        RSP_RDP_NOIMPL("Error: DL addr = %08X out of range, PC=%08X", dwAddr, __RSP.PC[__RSP.PCi] );
        dwAddr &= (g_dwRamSize-1);
    }

    switch (dwPush)
    {
       case G_DL_PUSH:
          LOG_UCODE("    Pushing ZeldaDisplayList 0x%08x", dwAddr);
          __RSP.PCi++;
          __RSP.PC[__RSP.PCi]        = dwAddr;
          __RSP.countdown[__RSP.PCi] = MAX_DL_COUNT;

          break;
       case G_DL_NOPUSH:
          LOG_UCODE("    Jumping to ZeldaDisplayList 0x%08x", dwAddr);
          if( __RSP.PC[__RSP.PCi] == dwAddr+8 )    //Is this a loop
          {
             //Hack for Gauntlet Legends
             __RSP.PC[__RSP.PCi] = dwAddr+8;
          }
          else
             __RSP.PC[__RSP.PCi]     = dwAddr;
          __RSP.countdown[__RSP.PCi] = MAX_DL_COUNT;
          break;
    }
}


void RSP_GBI2_SetOtherModeL(Gfx *gfx)
{
    SP_Timing(RSP_GBI1_SetOtherModeL);

    uint32_t dwShift = ((gfx->words.w0)>>8)&0xFF;
    uint32_t dwLength= ((gfx->words.w0)   )&0xFF;
    uint32_t dwData  = (gfx->words.w1);

    // Mask is constructed slightly differently
    uint32_t dwMask = (uint32_t)((int32_t)(0x80000000)>>dwLength)>>dwShift;
    dwData &= dwMask;

    uint32_t modeL = gDP.otherMode.l;
    modeL = (modeL&(~dwMask)) | dwData;

    Gfx tempgfx;
    tempgfx.words.w0 = gDP.otherMode.h;
    tempgfx.words.w1 = modeL;
    DLParser_RDPSetOtherMode(&tempgfx );
}


void RSP_GBI2_SetOtherModeH(Gfx *gfx)
{
    SP_Timing(RSP_GBI1_SetOtherModeH);

    uint32_t dwLength= (((gfx->words.w0))&0xFF)+1;
    uint32_t dwShift = 32 - (((gfx->words.w0)>>8)&0xFF) - dwLength;
    uint32_t dwData  = (gfx->words.w1);

    uint32_t dwMask2 = ((1<<dwLength)-1)<<dwShift;
    uint32_t dwModeH = gDP.otherMode.h;
    dwModeH = (dwModeH&(~dwMask2)) | dwData;

    Gfx tempgfx;
    tempgfx.words.w0 = dwModeH;
    tempgfx.words.w1 = gDP.otherMode.l;
    DLParser_RDPSetOtherMode(&tempgfx );
}


void RSP_GBI2_SubModule(Gfx *gfx)
{
    SP_Timing(RSP_GBI2_SubModule);
}

