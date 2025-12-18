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

// A few ucode used in DKR and Others Special games

#include <algorithm>

#include <retro_miscellaneous.h>

#include "../../Graphics/GBI.h"
#include "../../Graphics/RSP/RSP_state.h"

#include "Render.h"
#include "Timing.h"
#include "osal_preproc.h"

uint32_t dwConkerVtxZAddr=0;

static void RDP_GFX_DumpVtxInfoDKR(uint32_t dwAddr, uint32_t dwV0, uint32_t dwN);

void RDP_GFX_DLInMem(Gfx *gfx)
{
   uint32_t dwLimit = ((gfx->words.w0) >> 16) & 0xFF;
   uint32_t dwPush  = G_DL_PUSH; //((gfx->words.w0) >> 16) & 0xFF;
   uint32_t dwAddr  = 0x00000000 | (gfx->words.w1); //RSPSegmentAddr((gfx->words.w1));

   LOG_UCODE("    Address=0x%08x Push: 0x%02x", dwAddr, dwPush);

   switch (dwPush)
   {
      case G_DL_PUSH:
         LOG_UCODE("    Pushing DisplayList 0x%08x", dwAddr);
         __RSP.PCi++;
         __RSP.PC[__RSP.PCi]        = dwAddr;
         __RSP.countdown[__RSP.PCi] = dwLimit;

         break;
      case G_DL_NOPUSH:
         LOG_UCODE("    Jumping to DisplayList 0x%08x", dwAddr);
         __RSP.PC[__RSP.PCi]        = dwAddr;
         __RSP.countdown[__RSP.PCi] = dwLimit;
         break;
   }

   LOG_UCODE("");
   LOG_UCODE("\\/ \\/ \\/ \\/ \\/ \\/ \\/ \\/ \\/ \\/ \\/ \\/ \\/ \\/ \\/");
   LOG_UCODE("#############################################");
}

extern Matrix ALIGN(16, dkrMatrixTransposed);
void RSP_Mtx_DKR(Gfx *gfx)
{   
    uint32_t dwAddr = RSPSegmentAddr((gfx->words.w1));
    uint32_t dwCommand = ((gfx->words.w0)>>16)&0xFF;
    //uint32_t dwLength  = ((gfx->words.w0))    &0xFFFF;

    dwAddr = (gfx->words.w1)+RSPSegmentAddr(gRSP.dwDKRMatrixAddr);

    //gRSP.DKRCMatrixIndex = ((gfx->words.w0)>>22)&3;
    bool mul=false;
    int index = 0;
    switch( dwCommand )
    {
    case 0xC0:  // DKR
        gRSP.DKRCMatrixIndex = index = 3;
        break;
    case 0x80:  // DKR
        gRSP.DKRCMatrixIndex = index = 2;
        break;
    case 0x40:  // DKR
        gRSP.DKRCMatrixIndex = index = 1;
        break;
    case 0x20:  // DKR
        gRSP.DKRCMatrixIndex = index = 0;
        break;
    case 0x00:
        gRSP.DKRCMatrixIndex = index = 0;
        break;
    case 0x01:
        //mul = true;
        gRSP.DKRCMatrixIndex = index = 1;
        break;
    case 0x02:
        //mul = true;
        gRSP.DKRCMatrixIndex = index = 2;
        break;
    case 0x03:
        //mul = true;
        gRSP.DKRCMatrixIndex = index = 3;
        break;
    case 0x81:
        index = 1;
        mul = true;
        break;
    case 0x82:
        index = 2;
        mul = true;
        break;
    case 0x83:
        index = 3;
        mul = true;
        break;
    default:
        DebuggerAppendMsg("Fix me, mtx DKR, cmd=%08X", dwCommand);
        break;
    }

    // Load matrix from dwAddr
    Matrix &mat = gRSP.DKRMatrixes[index];
    LoadMatrix(dwAddr);

    if( mul )
        mat = matToLoad*gRSP.DKRMatrixes[0];
    else
        mat = matToLoad;
}

void RSP_Vtx_DKR(Gfx *gfx)
{
    uint32_t dwAddr = RSPSegmentAddr((gfx->words.w1));
    uint32_t dwV0 = (((gfx->words.w0) >> 9 )&0x1F);
    uint32_t dwN  = (((gfx->words.w0) >>19 )&0x1F)+1;

    if( gfx->words.w0 & 0x00010000 )
    {
        if( gRSP.DKRBillBoard )
            gRSP.DKRVtxCount = 1;
    }
    else
    {
        gRSP.DKRVtxCount = 0;
    }

    dwV0 += gRSP.DKRVtxCount;

    if (dwV0 >= 32)
        dwV0 = 31;
    
    if ((dwV0 + dwN) > 32)
    {
        WARNING(TRACE0("Warning, attempting to load into invalid vertex positions"));
        dwN = 32 - dwV0;
    }

    
    //if( dwAddr == 0 || dwAddr < 0x2000)
    {
        dwAddr = (gfx->words.w1)+RSPSegmentAddr(gRSP.dwDKRVtxAddr);
    }

    // Check that address is valid...
    if ((dwAddr + (dwN*16)) > g_dwRamSize)
    {
        WARNING(TRACE1("ProcessVertexData: Address out of range (0x%08x)", dwAddr));
    }
    else
    {
        ProcessVertexDataDKR(dwAddr, dwV0, dwN);

        status.dwNumVertices += dwN;

        RDP_GFX_DumpVtxInfoDKR(dwAddr, dwV0, dwN);
    }
}


void RSP_Vtx_Gemini(Gfx *gfx)
{
    uint32_t dwAddr = RSPSegmentAddr((gfx->words.w1));
    uint32_t dwV0 =  (((gfx->words.w0)>>9)&0x1F);
    uint32_t dwN  = (((gfx->words.w0) >>19 )&0x1F);

    if (dwV0 >= 32)
        dwV0 = 31;

    if ((dwV0 + dwN) > 32)
    {
        TRACE0("Warning, attempting to load into invalid vertex positions");
        dwN = 32 - dwV0;
    }


    //if( dwAddr == 0 || dwAddr < 0x2000)
    {
        dwAddr = (gfx->words.w1)+RSPSegmentAddr(gRSP.dwDKRVtxAddr);
    }

    // Check that address is valid...
    if ((dwAddr + (dwN*16)) > g_dwRamSize)
    {
        TRACE1("ProcessVertexData: Address out of range (0x%08x)", dwAddr);
    }
    else
    {
        ProcessVertexDataDKR(dwAddr, dwV0, dwN);

        status.dwNumVertices += dwN;

        RDP_GFX_DumpVtxInfoDKR(dwAddr, dwV0, dwN);
    }
}

// DKR verts are extra 4 bytes
void RDP_GFX_DumpVtxInfoDKR(uint32_t dwAddr, uint32_t dwV0, uint32_t dwN)
{
}

void DLParser_Set_Addr_Ucode6(Gfx *gfx)
{
    gRSP.dwDKRMatrixAddr = (gfx->words.w0)&0x00FFFFFF;
    gRSP.dwDKRVtxAddr = (gfx->words.w1)&0x00FFFFFF;
    gRSP.DKRVtxCount=0;
}



void RSP_Vtx_WRUS(Gfx *gfx)
{
    uint32_t dwAddr = RSPSegmentAddr((gfx->words.w1));
    uint32_t dwLength = ((gfx->words.w0))&0xFFFF;

    uint32_t dwN= (dwLength + 1) / 0x210;
    //uint32_t dwN= (dwLength >> 9);
    //uint32_t dwV0 = (((gfx->words.w0)>>16)&0x3f)/5;
    uint32_t dwV0 = (((gfx->words.w0)>>16)&0xFF)/5;

    LOG_UCODE("    Address 0x%08x, v0: %d, Num: %d, Length: 0x%04x", dwAddr, dwV0, dwN, dwLength);

    if (dwV0 >= 32)
        dwV0 = 31;
    
    if ((dwV0 + dwN) > 32)
    {
        TRACE0("Warning, attempting to load into invalid vertex positions");
        dwN = 32 - dwV0;
    }

    ProcessVertexData(dwAddr, dwV0, dwN);

    status.dwNumVertices += dwN;

    DisplayVertexInfo(dwAddr, dwV0, dwN);
}

void RSP_Vtx_ShadowOfEmpire(Gfx *gfx)
{
    uint32_t dwAddr = RSPSegmentAddr((gfx->words.w1));
    uint32_t dwLength = ((gfx->words.w0))&0xFFFF;

    uint32_t dwN= (((gfx->words.w0) >> 4) & 0xFFF) / 33 + 1;
    uint32_t dwV0 = 0;

    LOG_UCODE("    Address 0x%08x, v0: %d, Num: %d, Length: 0x%04x", dwAddr, dwV0, dwN, dwLength);

    if (dwV0 >= 32)
        dwV0 = 31;

    if ((dwV0 + dwN) > 32)
    {
        TRACE0("Warning, attempting to load into invalid vertex positions");
        dwN = 32 - dwV0;
    }

    ProcessVertexData(dwAddr, dwV0, dwN);

    status.dwNumVertices += dwN;

    DisplayVertexInfo(dwAddr, dwV0, dwN);
}


void RSP_DL_In_MEM_DKR(Gfx *gfx)
{
   // This cmd is likely to execute number of ucode at the given address
   uint32_t dwAddr = (gfx->words.w1);//RSPSegmentAddr((gfx->words.w1));
   __RSP.PCi++;
   __RSP.PC[__RSP.PCi]        = dwAddr;
   __RSP.countdown[__RSP.PCi] = (((gfx->words.w0)>>16)&0xFF);
}

void TexRectToN64FrameBuffer_YUV_16b(uint32_t x0, uint32_t y0, uint32_t width, uint32_t height)
{
   /* Convert YUV image at TImg and Copy the texture into the N64 RDRAM framebuffer memory */

   uint32_t n64CIaddr = g_CI.dwAddr;
   uint32_t n64CIwidth = g_CI.dwWidth;

   for (uint32_t y = 0; y < height; y++)
   {
      uint32_t x;
      uint8_t *rdram_u8 = (uint8_t*)gfx_info.RDRAM;
      uint32_t* pN64Src = (uint32_t*)(rdram_u8 + (g_TI.dwAddr&(g_dwRamSize-1)))+y*(g_TI.dwWidth>>1);
      uint16_t* pN64Dst = (uint16_t*)(rdram_u8 + (n64CIaddr&(g_dwRamSize-1)))+(y+y0)*n64CIwidth;

      for (x = 0; x < width; x+=2)
      {
         uint32_t val    = *pN64Src++;
         int y0          = (uint8_t)val&0xFF;
         int v           = (uint8_t)(val>>8)&0xFF;
         int y1          = (uint8_t)(val>>16)&0xFF;
         int u           = (uint8_t)(val>>24)&0xFF;

         pN64Dst[x+x0]   = YUVtoRGBA16(y0,u,v);
         pN64Dst[x+x0+1] = YUVtoRGBA16(y1,u,v);
      }
   }
}

extern uObjMtxReal gObjMtxReal;
void DLParser_OgreBatter64BG(Gfx *gfx)
{

   PrepareTextures();

   CTexture *ptexture = g_textures[0].m_pCTexture;
   TexRectToN64FrameBuffer_16b( (uint32_t)gObjMtxReal.X, (uint32_t)gObjMtxReal.Y, ptexture->m_dwWidth, ptexture->m_dwHeight, gRSP.curTile);
}

void DLParser_Bomberman2TextRect(Gfx *gfx)
{
    // Bomberman 64 - The Second Attack! (U) [!]
    // The 0x02 cmd, list a TexRect cmd

    if( options.enableHackForGames == HACK_FOR_OGRE_BATTLE && gDP.tiles[7].format == G_IM_FMT_YUV )
    {
        TexRectToN64FrameBuffer_YUV_16b( (uint32_t)gObjMtxReal.X, (uint32_t)gObjMtxReal.Y, 16, 16);
        //DLParser_OgreBatter64BG((gfx->words.w0), (gfx->words.w1));
        return;
    }

    uint8_t *rdram_u8 = (uint8_t*)gfx_info.RDRAM;
    uint32_t dwAddr = RSPSegmentAddr((gfx->words.w1));
    uObjSprite *info = (uObjSprite*)(rdram_u8 + dwAddr);

    uint32_t dwTile   = gRSP.curTile;

    PrepareTextures();
    
    //CRender::g_pRender->SetCombinerAndBlender();

    uObjTxSprite drawinfo;
    memcpy( &(drawinfo.sprite), info, sizeof(uObjSprite));
    CRender::g_pRender->DrawSpriteR(drawinfo, false, dwTile, 0, 0, drawinfo.sprite.imageW/32, drawinfo.sprite.imageH/32);
}

void RSP_MoveWord_DKR(Gfx *gfx)
{
    SP_Timing(RSP_GBI1_MoveWord);
    uint32_t dwNumLights;

    switch ((gfx->words.w0) & 0xFF)
    {
       case G_MW_NUMLIGHT:
          dwNumLights = (gfx->words.w1)&0x7;
          LOG_UCODE("    RSP_MOVE_WORD_NUMLIGHT: Val:%d", dwNumLights);

          gRSP.ambientLightIndex = dwNumLights;
          ricegSPNumLights(dwNumLights);
          //gRSP.DKRBillBoard = (gfx->words.w1)&0x1 ? true : false;
          gRSP.DKRBillBoard = (gfx->words.w1)&0x7 ? true : false;
          break;
       case G_MW_LIGHTCOL:
          gRSP.DKRCMatrixIndex = ((gfx->words.w1)>>6)&7;
          //gRSP.DKRCMatrixIndex = ((gfx->words.w1)>>6)&3;
          break;
       default:
          RSP_GBI1_MoveWord(gfx);
          break;
    }
}

extern void ricegSPCIVertex(uint32_t v, uint32_t n, uint32_t v0);
extern void ricegSPDMATriangles( uint32_t tris, uint32_t n );

void RSP_DMA_Tri_DKR(Gfx *gfx)
{
   uint32_t flag   = ((gfx->words.w0) & 0xFF0000) >> 16;

   if (flag & 1) 
      CRender::g_pRender->SetCullMode(false,true);
   else
      CRender::g_pRender->SetCullMode(false,false);

   ricegSPDMATriangles(
         gfx->words.w1,
         (((gfx->words.w0) &  0xFFF0) >>4 )
         );

   gRSP.DKRVtxCount=0;
}

uint32_t dwPDCIAddr = 0;


void RSP_Vtx_PD(Gfx *gfx)
{
   SP_Timing(RSP_GBI0_Vtx);

   uint32_t v    = RSPSegmentAddr((gfx->words.w1));
   uint32_t v0   = ((gfx->words.w0)  >> 16) &0x0F;
   uint32_t n    = (((gfx->words.w0) >> 20) &0x0F)+1;

   ricegSPCIVertex(v, n, v0);
   status.dwNumVertices += n;
}

void RSP_Set_Vtx_CI_PD(Gfx *gfx)
{
    // Color index buf address
    dwPDCIAddr = RSPSegmentAddr((gfx->words.w1));
}

void RSP_Tri4_PD(Gfx *gfx)
{
    uint8_t *rdram_u8 = (uint8_t*)gfx_info.RDRAM;
    uint32_t w0 = gfx->words.w0;
    uint32_t w1 = gfx->words.w1;

    status.primitiveType = PRIM_TRI2;

    // While the next command pair is Tri2, add vertices
    uint32_t dwPC = __RSP.PC[__RSP.PCi];

    bool bTrisAdded = false;

    do {
        uint32_t dwFlag = (w0>>16)&0xFF;
        LOG_UCODE("    PD Tri4: 0x%08x 0x%08x Flag: 0x%02x", w0, w1, dwFlag);

        for( uint32_t i=0; i<4; i++)
        {
            uint32_t v0 = (w1>>(4+(i<<3))) & 0xF;
            uint32_t v1 = (w1>>(  (i<<3))) & 0xF;
            uint32_t v2 = (w0>>(  (i<<2))) & 0xF;
            bool bVisible = IsTriangleVisible(v0, v2, v1);
            LOG_UCODE("       (%d, %d, %d) %s", v0, v1, v2, bVisible ? "": "(clipped)");
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

        w0 = *(uint32_t *)(rdram_u8 + dwPC+0);
        w1 = *(uint32_t *)(rdram_u8 + dwPC+4);
        dwPC += 8;

    } while ((w0>>24) == (uint8_t)RSP_TRI2);

    __RSP.PC[__RSP.PCi] = dwPC-8;

    if (bTrisAdded) 
        CRender::g_pRender->DrawTriangles();

    DEBUG_TRIANGLE(TRACE0("Pause at PD Tri4"));
}


void DLParser_Tri4_Conker(Gfx *gfx)
{
   uint8_t *rdram_u8 = (uint8_t*)gfx_info.RDRAM;
    uint32_t w0 = gfx->words.w0;
    uint32_t w1 = gfx->words.w1;

    status.primitiveType = PRIM_TRI2;

    // While the next command pair is Tri2, add vertices
    uint32_t dwPC = __RSP.PC[__RSP.PCi];

    bool bTrisAdded = false;

    do {
        LOG_UCODE("    Conker Tri4: 0x%08x 0x%08x", w0, w1);
        uint32_t idx[12];
        idx[0] = (w1   )&0x1F;
        idx[1] = (w1>> 5)&0x1F;
        idx[2] = (w1>>10)&0x1F;
        idx[3] = (w1>>15)&0x1F;
        idx[4] = (w1>>20)&0x1F;
        idx[5] = (w1>>25)&0x1F;

        idx[6] = (w0    )&0x1F;
        idx[7] = (w0>> 5)&0x1F;
        idx[8] = (w0>>10)&0x1F;

        idx[ 9] = (((w0>>15)&0x7)<<2)|(w1>>30);
        idx[10] = (w0>>18)&0x1F;
        idx[11] = (w0>>23)&0x1F;

        for( uint32_t i=0; i<4; i++)
        {
            uint32_t v0=idx[i*3  ];
            uint32_t v1=idx[i*3+1];
            uint32_t v2=idx[i*3+2];
            bool bVisible = IsTriangleVisible(v0, v1, v2);
            LOG_UCODE("       (%d, %d, %d) %s", v0, v1, v2, bVisible ? "": "(clipped)");
            if (bVisible)
            {
                DEBUG_DUMP_VERTEXES("Tri4 Conker:", v0, v1, v2);
                if (!bTrisAdded && CRender::g_pRender->IsTextureEnabled())
                {
                    PrepareTextures();
                    InitVertexTextureConstants();
                }

                if( !bTrisAdded )
                    CRender::g_pRender->SetCombinerAndBlender();

                bTrisAdded = true;
                PrepareTriangle(v0, v1, v2);
            }
        }

        w0 = *(uint32_t *)(rdram_u8 + dwPC+0);
        w1 = *(uint32_t *)(rdram_u8 + dwPC+4);
        dwPC += 8;

    } while ((w0>>28) == 1);

    __RSP.PC[__RSP.PCi] = dwPC-8;

    if (bTrisAdded) 
        CRender::g_pRender->DrawTriangles();

    DEBUG_TRIANGLE(TRACE0("Pause at Conker Tri4"));
}

void RDP_GFX_Force_Vertex_Z_Conker(uint32_t dwAddr)
{
    dwConkerVtxZAddr = dwAddr;
}



void DLParser_MoveMem_Conker(Gfx *gfx)
{
    uint32_t dwType    = ((gfx->words.w0)     ) & 0xFE;
    uint32_t dwAddr = RSPSegmentAddr((gfx->words.w1));

    if( dwType == RSP_GBI2_MV_MEM__MATRIX )
    {
        LOG_UCODE("    DLParser_MoveMem_Conker");
        RDP_GFX_Force_Vertex_Z_Conker(dwAddr);
    }
    else if( dwType == RSP_GBI2_MV_MEM__LIGHT )
    {
        LOG_UCODE("    MoveMem Light Conker");
        uint32_t dwOffset2 = ((gfx->words.w0) >> 5) & 0x3FFF;
        uint32_t dwLight=0xFF;
        if( dwOffset2 >= 0x30 )
        {
            dwLight = (dwOffset2 - 0x30)/0x30;
            LOG_UCODE("    Light %d:", dwLight);
            ricegSPLight(dwAddr, dwLight);
        }
        else
        {
            // fix me
            //TRACE0("Check me in DLParser_MoveMem_Conker - MoveMem Light");
        }
    }
    else
    {
        RSP_GBI2_MoveMem(gfx);
    }
}

extern void ProcessVertexDataConker(uint32_t dwAddr, uint32_t dwV0, uint32_t dwNum);
void RSP_Vtx_Conker(Gfx *gfx)
{
    uint32_t dwAddr = RSPSegmentAddr((gfx->words.w1));
    uint32_t dwVEnd    = (((gfx->words.w0)   )&0xFFF)/2;
    uint32_t dwN      = (((gfx->words.w0)>>12)&0xFFF);
    uint32_t dwV0     = dwVEnd - dwN;

    LOG_UCODE("    Vtx: Address 0x%08x, vEnd: %d, v0: %d, Num: %d", dwAddr, dwVEnd, dwV0, dwN);

    ProcessVertexDataConker(dwAddr, dwV0, dwN);
    status.dwNumVertices += dwN;
    DisplayVertexInfo(dwAddr, dwV0, dwN);
}


void DLParser_MoveWord_Conker(Gfx *gfx)
{
    uint32_t dwType   = ((gfx->words.w0) >> 16) & 0xFF;

    if( dwType != G_MW_NUMLIGHT )
    {
        RSP_GBI2_MoveWord(gfx);
    }
    else
    {
        uint32_t dwNumLights = ((gfx->words.w1)/48);
        LOG_UCODE("Conker RSP_MOVE_WORD_NUMLIGHT: %d", dwNumLights);
        gRSP.ambientLightIndex = dwNumLights+1;
        ricegSPNumLights(dwNumLights);
    }
}

void DLParser_Ucode8_0x0(Gfx *gfx)
{
    LOG_UCODE("DLParser_Ucode8_0x0");

    if( (gfx->words.w0) == 0 && (gfx->words.w1) )
    {
        uint32_t newaddr = RSPSegmentAddr((gfx->words.w1));

        if( newaddr && newaddr < g_dwRamSize)
        {
            if( __RSP.PCi < MAX_DL_STACK_SIZE-1 )
            {
                __RSP.PCi++;
                __RSP.PC[__RSP.PCi]        = newaddr+8; // Always skip the first 2 entries
                __RSP.countdown[__RSP.PCi] = MAX_DL_COUNT;
            }
        }
    }
    else
    {
        LOG_UCODE("DLParser_Ucode8_0x0, skip 0x%08X, 0x%08x", (gfx->words.w0), (gfx->words.w1));
        __RSP.PC[__RSP.PCi] += 8;
    }
}


uint32_t Rogue_Squadron_Vtx_XYZ_Cmd;
uint32_t Rogue_Squadron_Vtx_XYZ_Addr;
uint32_t Rogue_Squadron_Vtx_Color_Cmd;
uint32_t Rogue_Squadron_Vtx_Color_Addr;
uint32_t GSBlkAddrSaves[100][2];

void ProcessVertexData_Rogue_Squadron(uint32_t dwXYZAddr, uint32_t dwColorAddr, uint32_t dwXYZCmd, uint32_t dwColorCmd);

void DLParser_RS_Color_Buffer(Gfx *gfx)
{
    uint32_t dwPC = __RSP.PC[__RSP.PCi]-8;
    uint32_t dwAddr = RSPSegmentAddr((gfx->words.w1));

    if( dwAddr > g_dwRamSize )
    {
        TRACE0("DL, addr is wrong");
        dwAddr = (gfx->words.w1)&(g_dwRamSize-1);
    }

    Rogue_Squadron_Vtx_Color_Cmd = (gfx->words.w0);
    Rogue_Squadron_Vtx_Color_Addr = dwAddr;

    LOG_UCODE("Vtx_Color at PC=%08X: 0x%08x 0x%08x\n", dwPC-8, (gfx->words.w0), (gfx->words.w1));

    ProcessVertexData_Rogue_Squadron(Rogue_Squadron_Vtx_XYZ_Addr, Rogue_Squadron_Vtx_Color_Addr, Rogue_Squadron_Vtx_XYZ_Cmd, Rogue_Squadron_Vtx_Color_Cmd);

}


void DLParser_RS_Vtx_Buffer(Gfx *gfx)
{
    uint32_t dwPC = __RSP.PC[__RSP.PCi]-8;
    uint32_t dwAddr = RSPSegmentAddr((gfx->words.w1));
    if( dwAddr > g_dwRamSize )
    {
        TRACE0("DL, addr is wrong");
        dwAddr = (gfx->words.w1)&(g_dwRamSize-1);
    }

    LOG_UCODE("Vtx_XYZ at PC=%08X: 0x%08x 0x%08x\n", dwPC-8, (gfx->words.w0), (gfx->words.w1));
    Rogue_Squadron_Vtx_XYZ_Cmd = (gfx->words.w0);
    Rogue_Squadron_Vtx_XYZ_Addr = dwAddr;
}


void DLParser_RS_Block(Gfx *gfx)
{
    uint32_t dwPC = __RSP.PC[__RSP.PCi]-8;
    LOG_UCODE("ucode 0x80 at PC=%08X: 0x%08x 0x%08x\n", dwPC, (gfx->words.w0), (gfx->words.w1));
}

void DLParser_RS_MoveMem(Gfx *gfx)
{
    //uint32_t dwPC = __RSP.PC[__RSP.PCi];
    //uint32_t cmd1 = ((dwPC)&0x00FFFFFF)|0x80000000;
    RSP_GBI1_MoveMem(gfx);
    __RSP.PC[__RSP.PCi] += 16;

}

void DLParser_RS_0xbe(Gfx *gfx)
{
   uint8_t *rdram_u8 = (uint8_t*)gfx_info.RDRAM;
    uint32_t dwPC    = __RSP.PC[__RSP.PCi]-8;
    LOG_UCODE("ucode %02X, skip 1", ((gfx->words.w0)>>24));
    LOG_UCODE("\tPC=%08X: 0x%08x 0x%08x", dwPC, (gfx->words.w0), (gfx->words.w1));
    dwPC+=8;
    uint32_t dwCmd2 = *(uint32_t *)(rdram_u8 + dwPC);
    uint32_t dwCmd3 = *(uint32_t *)(rdram_u8 + dwPC+4);
    LOG_UCODE("\tPC=%08X: 0x%08x 0x%08x\n", dwPC, dwCmd2, dwCmd3);
    __RSP.PC[__RSP.PCi] += 8;

}


void DLParser_Ucode8_EndDL(Gfx *gfx)
{
    RDP_GFX_PopDL();
}

void DLParser_Ucode8_DL(Gfx *gfx)   // DL Function Call
{
   uint8_t *rdram_u8 = (uint8_t*)gfx_info.RDRAM;

    uint32_t dwAddr = RSPSegmentAddr((gfx->words.w1));
    uint32_t dwCmd2 = *(uint32_t *)(rdram_u8 + dwAddr);
    uint32_t dwCmd3 = *(uint32_t *)(rdram_u8 + dwAddr+4);

    if( dwAddr > g_dwRamSize )
    {
        TRACE0("DL, addr is wrong");
        dwAddr = (gfx->words.w1)&(g_dwRamSize-1);
    }

    if( __RSP.PCi < MAX_DL_STACK_SIZE-1 )
    {
        __RSP.PCi++;
        __RSP.PC[__RSP.PCi]        = dwAddr+16;
        __RSP.countdown[__RSP.PCi] = MAX_DL_COUNT;
    }
    else
    {
        DebuggerAppendMsg("Error, __RSP.PCi overflow");
        RDP_GFX_PopDL();
    }

    GSBlkAddrSaves[__RSP.PCi][0]=GSBlkAddrSaves[__RSP.PCi][1]=0;
    if( (dwCmd2>>24) == 0x80 )
    {
        GSBlkAddrSaves[__RSP.PCi][0] = dwCmd2;
        GSBlkAddrSaves[__RSP.PCi][1] = dwCmd3;
    }
}

void DLParser_Ucode8_JUMP(Gfx *gfx) // DL Function Call
{
   uint8_t *rdram_u8 = (uint8_t*)gfx_info.RDRAM;
    if( ((gfx->words.w0)&0x00FFFFFF) == 0 )
    {

        uint32_t dwAddr = RSPSegmentAddr((gfx->words.w1));

        if( dwAddr > g_dwRamSize )
        {
            TRACE0("DL, addr is wrong");
            dwAddr = (gfx->words.w1)&(g_dwRamSize-1);
        }

        __RSP.PC[__RSP.PCi] = dwAddr+8; // Jump to new address
    }
    else
    {
        uint32_t dwPC = __RSP.PC[__RSP.PCi]-8;
        LOG_UCODE("ucode 0x07 at PC=%08X: 0x%08x 0x%08x\n", dwPC, (gfx->words.w0), (gfx->words.w1));
    }
}

void DLParser_Ucode8_Unknown(Gfx *gfx)
{
    uint32_t dwPC = __RSP.PC[__RSP.PCi]-8;
    LOG_UCODE("ucode %02X at PC=%08X: 0x%08x 0x%08x\n", ((gfx->words.w0)>>24), dwPC, (gfx->words.w0), (gfx->words.w1));
}

void DLParser_Unknown_Skip1(Gfx *gfx)
{
    uint32_t dwPC = __RSP.PC[__RSP.PCi]-8;
    LOG_UCODE("ucode %02X, skip 1", ((gfx->words.w0)>>24));
    gfx++;
    LOG_UCODE("\tPC=%08X: 0x%08x 0x%08x", dwPC, (gfx->words.w0), (gfx->words.w1));
    dwPC+=8;
    gfx++;
    LOG_UCODE("\tPC=%08X: 0x%08x 0x%08x\n", dwPC, (gfx->words.w0), (gfx->words.w1));
    __RSP.PC[__RSP.PCi] += 8;
}

void DLParser_Unknown_Skip2(Gfx *gfx)
{
    uint32_t dwPC = __RSP.PC[__RSP.PCi]-8;
    LOG_UCODE("ucode %02X, skip 2", ((gfx->words.w0)>>24));
    gfx++;
    LOG_UCODE("\tPC=%08X: 0x%08x 0x%08x", dwPC, (gfx->words.w0), (gfx->words.w1));
    dwPC+=8;
    gfx++;
    LOG_UCODE("\tPC=%08X: 0x%08x 0x%08x", dwPC, (gfx->words.w0), (gfx->words.w1));
    dwPC+=8;
    gfx++;
    LOG_UCODE("\tPC=%08X: 0x%08x 0x%08x\n", dwPC, (gfx->words.w0), (gfx->words.w1));
    __RSP.PC[__RSP.PCi] += 16;
}

void DLParser_Unknown_Skip3(Gfx *gfx)
{
    uint32_t dwPC = __RSP.PC[__RSP.PCi]-8;
    LOG_UCODE("ucode %02X, skip 3", ((gfx->words.w0)>>24));
    gfx++;
    LOG_UCODE("\tPC=%08X: 0x%08x 0x%08x", dwPC, (gfx->words.w0), (gfx->words.w1));
    dwPC+=8;
    gfx++;
    LOG_UCODE("\tPC=%08X: 0x%08x 0x%08x", dwPC, (gfx->words.w0), (gfx->words.w1));
    dwPC+=8;
    gfx++;
    LOG_UCODE("\tPC=%08X: 0x%08x 0x%08x", dwPC, (gfx->words.w0), (gfx->words.w1));
    dwPC+=8;
    gfx++;
    LOG_UCODE("\tPC=%08X: 0x%08x 0x%08x\n", dwPC, (gfx->words.w0), (gfx->words.w1));
    __RSP.PC[__RSP.PCi] += 24;
}

void DLParser_Unknown_Skip4(Gfx *gfx)
{
    uint32_t dwPC = __RSP.PC[__RSP.PCi]-8;
    LOG_UCODE("ucode %02X, skip 4", ((gfx->words.w0)>>24));
    gfx++;
    LOG_UCODE("\tPC=%08X: 0x%08x 0x%08x", dwPC, (gfx->words.w0), (gfx->words.w1));
    dwPC+=8;
    gfx++;
    LOG_UCODE("\tPC=%08X: 0x%08x 0x%08x", dwPC, (gfx->words.w0), (gfx->words.w1));
    dwPC+=8;
    gfx++;
    LOG_UCODE("\tPC=%08X: 0x%08x 0x%08x", dwPC, (gfx->words.w0), (gfx->words.w1));
    dwPC+=8;
    gfx++;
    LOG_UCODE("\tPC=%08X: 0x%08x 0x%08x", dwPC, (gfx->words.w0), (gfx->words.w1));
    dwPC+=8;
    gfx++;
    LOG_UCODE("\tPC=%08X: 0x%08x 0x%08x\n", dwPC, (gfx->words.w0), (gfx->words.w1));
    __RSP.PC[__RSP.PCi] += 32;
}

void DLParser_Ucode8_0x05(Gfx *gfx)
{
    // Be careful, 0x05 is variable length ucode
    /*
    0028E4E0: 05020088, 04D0000F - Reserved1
    0028E4E8: 6BDC0306, 00000000 - G_NOTHING
    0028E4F0: 05010130, 01B0000F - Reserved1
    0028E4F8: 918A01CA, 1EC5FF3B - G_NOTHING
    0028E500: 05088C68, F5021809 - Reserved1
    0028E508: 04000405, 00000000 - RSP_VTX
    0028E510: 102ECE60, 202F2AA0 - G_NOTHING
    0028E518: 05088C90, F5021609 - Reserved1
    0028E520: 04050405, F0F0F0F0 - RSP_VTX
    0028E528: 102ED0C0, 202F2D00 - G_NOTHING
    0028E530: B5000000, 00000000 - RSP_LINE3D
    0028E538: 8028E640, 8028E430 - G_NOTHING
    0028E540: 00000000, 00000000 - RSP_SPNOOP
    */

    if((gfx->words.w1) == 0)
        return;

    DLParser_Unknown_Skip4(gfx);
}

void DLParser_Ucode8_0xb4(Gfx *gfx)
{
    if(((gfx->words.w0)&0xFF) == 0x06)
        DLParser_Unknown_Skip3(gfx);
    else if(((gfx->words.w0)&0xFF) == 0x04)
        DLParser_Unknown_Skip1(gfx);
    else if(((gfx->words.w0)&0xFFF) == 0x600)
        DLParser_Unknown_Skip3(gfx);
    else
    {
        DLParser_Unknown_Skip3(gfx);
    }
}

void DLParser_Ucode8_0xb5(Gfx *gfx)
{
   uint8_t *rdram_u8 = (uint8_t*)gfx_info.RDRAM;
   uint32_t dwPC = __RSP.PC[__RSP.PCi]-8;
   LOG_UCODE("ucode 0xB5 at PC=%08X: 0x%08x 0x%08x\n", dwPC-8, (gfx->words.w0), (gfx->words.w1));

   uint32_t dwCmd2 = *(uint32_t *)(rdram_u8 + dwPC+8);
   uint32_t dwCmd3 = *(uint32_t *)(rdram_u8 + dwPC+12);
   LOG_UCODE("     : 0x%08x 0x%08x\n", dwCmd2, dwCmd3);

   //if( dwCmd2 == 0 && dwCmd3 == 0 )
   {
      DLParser_Ucode8_EndDL(gfx); // Check me
      return;
   }

   __RSP.PC[__RSP.PCi] += 8;
   return;


   if( GSBlkAddrSaves[__RSP.PCi][0] == 0 || GSBlkAddrSaves[__RSP.PCi][1] == 0 )
   {
      DLParser_Ucode8_EndDL(gfx); // Check me
      return;
   }

   if( ((dwCmd2>>24)!=0x80 && (dwCmd2>>24)!=0x00 ) || ((dwCmd3>>24)!=0x80 && (dwCmd3>>24)!=0x00 ) )
   {
      DLParser_Ucode8_EndDL(gfx); // Check me
      return;
   }

   if( (dwCmd2>>24)!= (dwCmd3>>24) )
   {
      DLParser_Ucode8_EndDL(gfx); // Check me
      return;
   }


   if( (dwCmd2>>24)==0x80 && (dwCmd3>>24)==0x80 )
   {
      if( dwCmd2 < dwCmd3  )
      {
         // All right, the next block is not ucode, but data
         DLParser_Ucode8_EndDL(gfx); // Check me
         return;
      }

      uint32_t dwCmd4 = *(uint32_t *)(rdram_u8 + (dwCmd2&0x00FFFFFF));
      uint32_t dwCmd5 = *(uint32_t *)(rdram_u8 + (dwCmd2&0x00FFFFFF)+4);
      uint32_t dwCmd6 = *(uint32_t *)(rdram_u8 + (dwCmd3&0x00FFFFFF));
      uint32_t dwCmd7 = *(uint32_t *)(rdram_u8 + (dwCmd3&0x00FFFFFF)+4);
      if( (dwCmd4>>24) != 0x80 || (dwCmd5>>24) != 0x80 || (dwCmd6>>24) != 0x80 || (dwCmd7>>24) != 0x80 || dwCmd4 < dwCmd5 || dwCmd6 < dwCmd7 )
      {
         // All right, the next block is not ucode, but data
         DLParser_Ucode8_EndDL(gfx); // Check me
         return;
      }

      __RSP.PC[__RSP.PCi] += 8;
      return;
   }
   else if( (dwCmd2>>24)==0x00 && (dwCmd3>>24)==0x00 )
   {
      DLParser_Ucode8_EndDL(gfx); // Check me
      return;
   }
   else if( (dwCmd2>>24)==0x00 && (dwCmd3>>24)==0x00 )
   {
      dwCmd2 = *(uint32_t *)(rdram_u8 + dwPC+16);
      dwCmd3 = *(uint32_t *)(rdram_u8 + dwPC+20);
      if( (dwCmd2>>24)==0x80 && (dwCmd3>>24)==0x80 && dwCmd2 < dwCmd3 )
      {
         // All right, the next block is not ucode, but data
         DLParser_Ucode8_EndDL(gfx); // Check me
         return;
      }
      else
      {
         __RSP.PC[__RSP.PCi] += 8;
         return;
      }
   }
}

void DLParser_Ucode8_0xbc(Gfx *gfx)
{
    if( ((gfx->words.w0)&0xFFF) == 0x58C )
    {
        DLParser_Ucode8_DL(gfx);
    }
    else
    {
        uint32_t dwPC = __RSP.PC[__RSP.PCi]-8;
        LOG_UCODE("ucode 0xBC at PC=%08X: 0x%08x 0x%08x\n", dwPC, (gfx->words.w0), (gfx->words.w1));
    }
}

void DLParser_Ucode8_0xbd(Gfx *gfx)
{
    /*
    00359A68: BD000000, DB5B0077 - RSP_POPMTX
    00359A70: C8C0A000, 00240024 - RDP_TriFill
    00359A78: 01000100, 00000000 - RSP_MTX
    00359A80: BD000501, DB5B0077 - RSP_POPMTX
    00359A88: C8C0A000, 00240024 - RDP_TriFill
    00359A90: 01000100, 00000000 - RSP_MTX
    00359A98: BD000A02, DB5B0077 - RSP_POPMTX
    00359AA0: C8C0A000, 00240024 - RDP_TriFill
    00359AA8: 01000100, 00000000 - RSP_MTX
    00359AB0: BD000F04, EB6F0087 - RSP_POPMTX
    00359AB8: C8C0A000, 00280028 - RDP_TriFill
    00359AC0: 01000100, 00000000 - RSP_MTX
    00359AC8: BD001403, DB5B0077 - RSP_POPMTX
    00359AD0: C8C0A000, 00240024 - RDP_TriFill
    00359AD8: 01000100, 00000000 - RSP_MTX
    00359AE0: B5000000, 00000000 - RSP_LINE3D
    00359AE8: 1A000000, 16000200 - G_NOTHING
     */

    if( (gfx->words.w1) != 0 )
    {
        DLParser_Unknown_Skip2(gfx);
        return;
    }

    uint32_t dwPC = __RSP.PC[__RSP.PCi];
    LOG_UCODE("ucode 0xbd at PC=%08X: 0x%08x 0x%08x\n", dwPC-8, (gfx->words.w0), (gfx->words.w1));
}

void DLParser_Ucode8_0xbf(Gfx *gfx)
{
    if( ((gfx->words.w0)&0xFF) == 0x02 )
        DLParser_Unknown_Skip3(gfx);
    else
        DLParser_Unknown_Skip1(gfx);
}

void PD_LoadMatrix_0xb4(uint32_t addr)
{
   unsigned i, j;
   uint32_t data[16];
   uint8_t *rdram_u8  = (uint8_t*)gfx_info.RDRAM;
   const float fRecip = 1.0f / 65536.0f;

   data[0]  =  *(uint32_t*)(rdram_u8 +addr+4+ 0);
   data[1]  =  *(uint32_t*)(rdram_u8 +addr+4+ 8);
   data[2]  =  *(uint32_t*)(rdram_u8 +addr+4+16);
   data[3]  =  *(uint32_t*)(rdram_u8 +addr+4+24);

   data[8]  =  *(uint32_t*)(rdram_u8 +addr+4+32);
   data[9]  =  *(uint32_t*)(rdram_u8 +addr+4+40);
   data[10] =  *(uint32_t*)(rdram_u8 +addr+4+48);
   data[11] =  *(uint32_t*)(rdram_u8 +addr+4+56);

   data[4]  =  *(uint32_t*)(rdram_u8 +addr+4+ 0+64);
   data[5]  =  *(uint32_t*)(rdram_u8 +addr+4+ 8+64);
   data[6]  =  *(uint32_t*)(rdram_u8 +addr+4+16+64);
   data[7]  =  *(uint32_t*)(rdram_u8 +addr+4+24+64);

   data[12] =  *(uint32_t*)(rdram_u8 +addr+4+32+64);
   data[13] =  *(uint32_t*)(rdram_u8 +addr+4+40+64);
   data[14] =  *(uint32_t*)(rdram_u8 +addr+4+48+64);
   data[15] =  *(uint32_t*)(rdram_u8 +addr+4+56+64);

   for (i = 0; i < 4; i++)
   {
      for (j = 0; j < 4; j++) 
      {
         int  hi = *(short *)((unsigned char*)data + (((i<<3)+(j<<1)     )^0x2));
         int  lo = *(uint16_t*)((unsigned char*)data + (((i<<3)+(j<<1) + 32)^0x2));
         matToLoad.m[i][j] = (float)((hi<<16) | lo) * fRecip;
      }
   }
}   

void DLParser_RDPHalf_1_0xb4_GoldenEye(Gfx *gfx)        
{
   SP_Timing(RSP_GBI1_RDPHalf_1);
   if( ((gfx->words.w1)>>24) == 0xce )
   {
      uint8_t *rdram_u8 = (uint8_t*)gfx_info.RDRAM;
      PrepareTextures();
      CRender::g_pRender->SetCombinerAndBlender();

      uint32_t dwPC  = __RSP.PC[__RSP.PCi];       // This points to the next instruction

      //PD_LoadMatrix_0xb4(dwPC + 8*16 - 8);

      uint32_t dw1   = *(uint32_t *)(rdram_u8 + dwPC+8*0+4);
      uint32_t dw8   = *(uint32_t *)(rdram_u8 + dwPC+8*7+4);
      uint32_t dw9   = *(uint32_t *)(rdram_u8 + dwPC+8*8+4);

      uint32_t r     = (dw8>>16)&0xFF;
      uint32_t g     = (dw8    )&0xFF;
      uint32_t b     = (dw9>>16)&0xFF;
      uint32_t a     = (dw9    )&0xFF;
      uint32_t color = COLOR_RGBA(r, g, b, a);

      //int x0 = 0;
      //int x1 = gRDP.scissor.right;
      int x0 = gRSP.nVPLeftN;
      int x1 = gRSP.nVPRightN;
      int y0 = int(dw1&0xFFFF)/4;
      int y1 = int(dw1>>16)/4;

      float xscale = g_textures[0].m_pCTexture->m_dwWidth / (float)(x1-x0);
      float yscale = g_textures[0].m_pCTexture->m_dwHeight / (float)(y1-y0);
      //float fs0 = (short)(dw3&0xFFFF)/32768.0f*g_textures[0].m_pCTexture->m_dwWidth;
      //float ft0 = (short)(dw3>>16)/32768.0f*256;
      CRender::g_pRender->TexRect(x0, y0, x1, y1, 0, 0, xscale, yscale, true, color);

      __RSP.PC[__RSP.PCi] += 312;
   }
}

void DLParser_RSP_DL_WorldDriver(Gfx *gfx)
{
    uint32_t dwAddr = RSPSegmentAddr((gfx->words.w1));
    if( dwAddr > g_dwRamSize )
    {
        RSP_RDP_NOIMPL("Error: DL addr = %08X out of range, PC=%08X", dwAddr, __RSP.PC[__RSP.PCi] );
        dwAddr &= (g_dwRamSize-1);
    }

    LOG_UCODE("    WorldDriver DisplayList 0x%08x", dwAddr);
    __RSP.PCi++;
    __RSP.PC[__RSP.PCi]        = dwAddr;
    __RSP.countdown[__RSP.PCi] = MAX_DL_COUNT;

    LOG_UCODE("Level=%d", __RSP.PCi+1);
    LOG_UCODE("^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^");
}

void DLParser_RSP_Pop_DL_WorldDriver(Gfx *gfx)
{
    RDP_GFX_PopDL();
}

void DLParser_RSP_Last_Legion_0x80(Gfx *gfx)
{
    __RSP.PC[__RSP.PCi] += 16;
    LOG_UCODE("DLParser_RSP_Last_Legion_0x80");
}

void DLParser_RSP_Last_Legion_0x00(Gfx *gfx)
{
   uint8_t *rdram_u8 = (uint8_t*)gfx_info.RDRAM;
   LOG_UCODE("DLParser_RSP_Last_Legion_0x00");
   __RSP.PC[__RSP.PCi] += 16;

   if( (gfx->words.w0) == 0 && (gfx->words.w1) )
   {
      uint32_t newaddr = RSPSegmentAddr((gfx->words.w1));
      if( newaddr >= g_dwRamSize )
      {
         RDP_GFX_PopDL();
         return;
      }

      uint32_t pc1 = *(uint32_t *)(rdram_u8 + newaddr+8*1+4);
      uint32_t pc2 = *(uint32_t *)(rdram_u8 + newaddr+8*4+4);
      pc1 = RSPSegmentAddr(pc1);
      pc2 = RSPSegmentAddr(pc2);

      if( pc1 && pc1 != 0xffffff && pc1 < g_dwRamSize)
      {
         // Need to call both DL
         __RSP.PCi++;
         __RSP.PC[__RSP.PCi] = pc1;
         __RSP.countdown[__RSP.PCi] = MAX_DL_COUNT;
      }

      if( pc2 && pc2 != 0xffffff && pc2 < g_dwRamSize )
      {
         __RSP.PCi++;
         __RSP.PC[__RSP.PCi] = pc2;
         __RSP.countdown[__RSP.PCi] = MAX_DL_COUNT;
      }
   }
   else if( (gfx->words.w1) == 0 )
   {
      RDP_GFX_PopDL();
   }
   else
   {
      // (gfx->words.w0) != 0
      RSP_RDP_Nothing(gfx);
      RDP_GFX_PopDL();
   }
}

void DLParser_TexRect_Last_Legion(Gfx *gfx)
{
   uint8_t *rdram_u8 = (uint8_t*)gfx_info.RDRAM;
    if( !status.bCIBufferIsRendered )
        g_pFrameBufferManager->ActiveTextureBuffer();

    status.primitiveType = PRIM_TEXTRECT;

    // This command used 128bits, and not 64 bits. This means that we have to look one 
    // Command ahead in the buffer, and update the PC.
    uint32_t dwPC = __RSP.PC[__RSP.PCi];       // This points to the next instruction
    uint32_t dwCmd2 = *(uint32_t *)(rdram_u8 + dwPC);
    uint32_t dwCmd3 = *(uint32_t *)(rdram_u8 + dwPC+4);

    __RSP.PC[__RSP.PCi] += 8;


    LOG_UCODE("0x%08x: %08x %08x", dwPC, *(uint32_t *)(rdram_u8 + dwPC+0), *(uint32_t *)(rdram_u8 + dwPC+4));

    uint32_t dwXH     = (((gfx->words.w0)>>12)&0x0FFF)/4;
    uint32_t dwYH     = (((gfx->words.w0)    )&0x0FFF)/4;
    uint32_t tileno   = ((gfx->words.w1)>>24)&0x07;
    uint32_t dwXL     = (((gfx->words.w1)>>12)&0x0FFF)/4;
    uint32_t dwYL     = (((gfx->words.w1)    )&0x0FFF)/4;


    if( (int)dwXL >= gRDP.scissor.right || (int)dwYL >= gRDP.scissor.bottom || (int)dwXH < gRDP.scissor.left || (int)dwYH < gRDP.scissor.top )
    {
        // Clipping
        return;
    }

    uint16_t uS  = (uint16_t)(  dwCmd2>>16)&0xFFFF;
    uint16_t uT  = (uint16_t)(  dwCmd2    )&0xFFFF;
    short s16S = *(short*)(&uS);
    short s16T = *(short*)(&uT);

    uint16_t uDSDX   = (uint16_t)((  dwCmd3>>16)&0xFFFF);
    uint16_t uDTDY   = (uint16_t)((  dwCmd3    )&0xFFFF);
    short s16DSDX  = *(short*)(&uDSDX);
    short s16DTDY  = *(short*)(&uDTDY);

    uint32_t curTile = gRSP.curTile;
    ForceMainTextureIndex(tileno);

    float fS0 = s16S / 32.0f;
    float fT0 = s16T / 32.0f;

    float fDSDX = s16DSDX / 1024.0f;
    float fDTDY = s16DTDY / 1024.0f;

    uint32_t cycletype = gRDP.otherMode.cycle_type;

    if (cycletype == G_CYC_COPY)
    {
        fDSDX /= 4.0f;  // In copy mode 4 pixels are copied at once.
        dwXH++;
        dwYH++;
    }
    else if (cycletype == G_CYC_FILL)
    {
        dwXH++;
        dwYH++;
    }

    if( fDSDX == 0 )  fDSDX = 1;
    if( fDTDY == 0 )  fDTDY = 1;

    float fS1 = fS0 + (fDSDX * (dwXH - dwXL));
    float fT1 = fT0 + (fDTDY * (dwYH - dwYL));

    LOG_UCODE("    Tile:%d Screen(%d,%d) -> (%d,%d)", tileno, dwXL, dwYL, dwXH, dwYH);
    LOG_UCODE("           Tex:(%#5f,%#5f) -> (%#5f,%#5f) (DSDX:%#5f DTDY:%#5f)",
        fS0, fT0, fS1, fT1, fDSDX, fDTDY);
    LOG_UCODE("");

    float t0u0 = (fS0-gRDP.tilesinfo[tileno].hilite_sl) * gRDP.tilesinfo[tileno].fShiftScaleS;
    float t0v0 = (fT0-gRDP.tilesinfo[tileno].hilite_tl) * gRDP.tilesinfo[tileno].fShiftScaleT;
    float t0u1 = t0u0 + (fDSDX * (dwXH - dwXL))*gRDP.tilesinfo[tileno].fShiftScaleS;
    float t0v1 = t0v0 + (fDTDY * (dwYH - dwYL))*gRDP.tilesinfo[tileno].fShiftScaleT;

    if( dwXL==0 && dwYL==0 && dwXH==windowSetting.fViWidth-1 && dwYH==windowSetting.fViHeight-1 &&
        t0u0 == 0 && t0v0==0 && t0u1==0 && t0v1==0 )
    {
        //Using TextRect to clear the screen
    }
    else
    {
        if( status.bHandleN64RenderTexture && //status.bDirectWriteIntoRDRAM && 
            g_pRenderTextureInfo->CI_Info.dwFormat == gDP.tiles[tileno].format && 
            g_pRenderTextureInfo->CI_Info.dwSize == gDP.tiles[tileno].size && 
            gDP.tiles[tileno].format == G_IM_FMT_CI && gDP.tiles[tileno].size == G_IM_SIZ_8b )
        {
            if( options.enableHackForGames == HACK_FOR_YOSHI )
            {
                // Hack for Yoshi background image
                PrepareTextures();
                TexRectToFrameBuffer_8b(dwXL, dwYL, dwXH, dwYH, t0u0, t0v0, t0u1, t0v1, tileno);
            }
            else
            {
                if( frameBufferOptions.bUpdateCIInfo )
                {
                    PrepareTextures();
                    TexRectToFrameBuffer_8b(dwXL, dwYL, dwXH, dwYH, t0u0, t0v0, t0u1, t0v1, tileno);
                }

                if( !status.bDirectWriteIntoRDRAM )
                {
                    CRender::g_pRender->TexRect(dwXL, dwYL, dwXH, dwYH, fS0, fT0, fDSDX, fDTDY, false, 0xFFFFFFFF);

                    status.dwNumTrisRendered += 2;
                }
            }
        }
        else
        {
            CRender::g_pRender->TexRect(dwXL, dwYL, dwXH, dwYH, fS0, fT0, fDSDX, fDTDY, false, 0xFFFFFFFF);
            status.bFrameBufferDrawnByTriangles = true;

            status.dwNumTrisRendered += 2;
        }
    }

    if( status.bHandleN64RenderTexture ) 
        g_pRenderTextureInfo->maxUsedHeight = MAX(g_pRenderTextureInfo->maxUsedHeight,(int)dwYH);

    ForceMainTextureIndex(curTile);
}
