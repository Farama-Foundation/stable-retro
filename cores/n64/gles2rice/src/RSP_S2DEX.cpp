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


// This file implements the S2DEX microcode. Yoshi's Story is using this microcode.
#include <stdint.h>
#include "UcodeDefs.h"
#include "Render.h"
#include "Timing.h"

#include "../../Graphics/RSP/RSP_state.h"

uObjTxtr *gObjTxtr = NULL;
uObjTxtrTLUT *gObjTlut = NULL;
uint32_t gObjTlutAddr = 0;
uObjMtx *gObjMtx = NULL;
uObjSubMtx *gSubObjMtx = NULL;
uObjMtxReal gObjMtxReal = {1, 0, 0, 1, 0, 0, 0, 0};
Matrix g_MtxReal(1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1);

uint32_t g_TxtLoadBy = CMD_LOAD_OBJ_TXTR;


// Yoshi's Story uses this - 0x02
void RSP_S2DEX_BG_COPY(Gfx *gfx)
{
    SP_Timing(DP_Minimal16);
    DP_Timing(DP_Minimal16);

    uint8_t *rdram_u8 = (uint8_t*)gfx_info.RDRAM;
    uint32_t dwAddr = RSPSegmentAddr((gfx->words.w1));
    uObjBg *sbgPtr = (uObjBg*)(rdram_u8 + dwAddr);
    CRender::g_pRender->LoadObjBGCopy(*sbgPtr);
    CRender::g_pRender->DrawObjBGCopy(*sbgPtr);
}

// Yoshi's Story uses this - 0x03
void RSP_S2DEX_OBJ_RECTANGLE(Gfx *gfx)
{
   uint8_t *rdram_u8 = (uint8_t*)gfx_info.RDRAM;
    uint32_t dwAddr = RSPSegmentAddr((gfx->words.w1));
    uObjSprite *ptr = (uObjSprite*)(rdram_u8 + dwAddr);

    uObjTxSprite objtx;
    memcpy(&objtx.sprite,ptr,sizeof(uObjSprite));

    if( g_TxtLoadBy == CMD_LOAD_OBJ_TXTR )
    {
        memcpy(&(objtx.txtr.block),&(gObjTxtr->block),sizeof(uObjTxtr));
        CRender::g_pRender->LoadObjSprite(objtx, true);
    }
    else
    {
        PrepareTextures();
    }
    CRender::g_pRender->DrawSprite(objtx, false);
}

// Yoshi's Story uses this - 0x04
void RSP_S2DEX_OBJ_SPRITE(Gfx *gfx)
{
   uint8_t *rdram_u8 = (uint8_t*)gfx_info.RDRAM;
    uint32_t dwAddr = RSPSegmentAddr((gfx->words.w1));
    uObjSprite *info = (uObjSprite*)(rdram_u8 + dwAddr);

    uint32_t dwTile   = gRSP.curTile;
    status.bAllowLoadFromTMEM = false;  // Because we need to use TLUT loaded by the ObjTlut command
    PrepareTextures();
    status.bAllowLoadFromTMEM = true;

    uObjTxSprite drawinfo;
    memcpy( &(drawinfo.sprite), info, sizeof(uObjSprite));
    CRender::g_pRender->DrawSpriteR(drawinfo, false, dwTile, 0, 0, drawinfo.sprite.imageW/32, drawinfo.sprite.imageH/32);
}

// Yoshi's Story uses this - 0xb0
void RSP_S2DEX_SELECT_DL(Gfx *gfx)
{
    //static BOOL bWarned = FALSE;
    //if (!bWarned)
    {
        RSP_RDP_NOIMPL("RDP: RSP_S2DEX_SELECT_DL (0x%08x 0x%08x)", (gfx->words.w0), (gfx->words.w1));
        //bWarned = TRUE;
    }
}

void RSP_S2DEX_OBJ_RENDERMODE(Gfx *gfx)
{
    /*
    static BOOL bWarned = FALSE;
    //if (!bWarned)
    {
    RSP_RDP_NOIMPL("RDP: RSP_S2DEX_OBJ_RENDERMODE (0x%08x 0x%08x)", (gfx->words.w0), (gfx->words.w1));
    bWarned = TRUE;
    }
    */
}

// Yoshi's Story uses this - 0xb1
void RSP_GBI1_Tri2(Gfx *gfx);
void RSP_S2DEX_OBJ_RENDERMODE_2(Gfx *gfx)
{
    if( ((gfx->words.w0)&0xFFFFFF) != 0 || ((gfx->words.w1)&0xFFFFFF00) != 0 )
    {
        // This is a TRI2 cmd
        RSP_GBI1_Tri2(gfx);
        return;
    }

    RSP_S2DEX_OBJ_RENDERMODE(gfx);
}

void ObjMtxTranslate(float &x, float &y)
{
    float x1 = gObjMtxReal.A*x + gObjMtxReal.B*y + gObjMtxReal.X;
    float y1 = gObjMtxReal.C*x + gObjMtxReal.D*y + gObjMtxReal.Y;

    x = x1;
    y = y1;
}

void RSP_S2DEX_SPObjLoadTxtr(Gfx *gfx)
{
   uint8_t *rdram_u8 = (uint8_t*)gfx_info.RDRAM;
    gObjTxtr = (uObjTxtr*)(rdram_u8 + (RSPSegmentAddr((gfx->words.w1))&(g_dwRamSize-1)));
    if( gObjTxtr->block.type == S2DEX_OBJLT_TLUT )
    {
        gObjTlut = (uObjTxtrTLUT*)gObjTxtr;
        gObjTlutAddr = (uint32_t)(RSPSegmentAddr(gObjTlut->image));
        
        // Copy tlut
        int size = gObjTlut->pnum+1;
        int offset = gObjTlut->phead-0x100;

        if( offset+size>0x100)
        {
            size = 0x100 - offset;
        }

        uint32_t addr = (gObjTlutAddr);

        for( int i=offset; i<offset+size; i++ )
        {
            g_wRDPTlut[i^1] = RDRAM_UHALF(addr);
            addr += 2;
        }
    }
    else
    {
        // Loading ObjSprite
        g_TxtLoadBy = CMD_LOAD_OBJ_TXTR;
    }
}

// Yoshi's Story uses this - 0xc2
void RSP_S2DEX_SPObjLoadTxSprite(Gfx *gfx)
{
    uint8_t *rdram_u8 = (uint8_t*)gfx_info.RDRAM;
    uObjTxSprite* ptr = (uObjTxSprite*)(rdram_u8 + (RSPSegmentAddr((gfx->words.w1))&(g_dwRamSize-1)));
    gObjTxtr = (uObjTxtr*)ptr;
    
    //Now draw the sprite
    CRender::g_pRender->LoadObjSprite(*ptr, false);
    CRender::g_pRender->DrawSpriteR(*ptr, true, 0, 0, 0, 0, 0);
}


// Yoshi's Story uses this - 0xc3
void RSP_S2DEX_SPObjLoadTxRect(Gfx *gfx)
{
    uint8_t *rdram_u8 = (uint8_t*)gfx_info.RDRAM;
    uObjTxSprite* ptr = (uObjTxSprite*)(rdram_u8 + (RSPSegmentAddr((gfx->words.w1))&(g_dwRamSize-1)));
    gObjTxtr = (uObjTxtr*)ptr;
    
    //Now draw the sprite
    CRender::g_pRender->LoadObjSprite(*ptr, false);
    CRender::g_pRender->DrawSprite(*ptr, false);
}

// Yoshi's Story uses this - 0xc4
void RSP_S2DEX_SPObjLoadTxRectR(Gfx *gfx)
{
   uint8_t *rdram_u8 = (uint8_t*)gfx_info.RDRAM;
    uObjTxSprite* ptr = (uObjTxSprite*)(rdram_u8 + (RSPSegmentAddr((gfx->words.w1))&(g_dwRamSize-1)));
    gObjTxtr = (uObjTxtr*)ptr;
    
    //Now draw the sprite
    CRender::g_pRender->LoadObjSprite(*ptr, false);
    CRender::g_pRender->DrawSprite(*ptr, true);
}

void DLParser_TexRect(Gfx *gfx);
// Yoshi's Story uses this - 0xe4
void RSP_S2DEX_RDPHALF_0(Gfx *gfx)
{
    //RDP: RSP_S2DEX_RDPHALF_0 (0xe449c0a8 0x003b40a4)
    //0x001d3c88: e449c0a8 003b40a4 RDP_TEXRECT 
    //0x001d3c90: b4000000 00000000 RSP_RDPHALF_1
    //0x001d3c98: b3000000 04000400 RSP_RDPHALF_2

   uint8_t *rdram_u8     = (uint8_t*)gfx_info.RDRAM;
    uint32_t dwPC        = __RSP.PC[__RSP.PCi];       // This points to the next instruction
    uint32_t dwNextUcode = *(uint32_t *)(rdram_u8 + dwPC);

    if( (dwNextUcode>>24) != S2DEX_SELECT_DL )
    {
        // Pokemon Puzzle League
        if( (dwNextUcode>>24) == 0xB4 )
            DLParser_TexRect(gfx);
        else
        {
            RSP_RDP_NOIMPL("RDP: RSP_S2DEX_RDPHALF_0 (0x%08x 0x%08x)", (gfx->words.w0), (gfx->words.w1));
        }
    }
    else
    {
        RSP_RDP_NOIMPL("RDP: RSP_S2DEX_RDPHALF_0 (0x%08x 0x%08x)", (gfx->words.w0), (gfx->words.w1));
    }
}

// Yoshi's Story uses this - 0x05
void RSP_S2DEX_OBJ_MOVEMEM(Gfx *gfx)
{
    uint32_t dwCommand = ((gfx->words.w0)>>16)&0xFF;
    uint32_t dwLength  = ((gfx->words.w0))    &0xFFFF;
    uint32_t dwAddr = RSPSegmentAddr((gfx->words.w1));

    if( dwAddr >= g_dwRamSize )
    {
        TRACE0("ObjMtx: memory ptr is invalid");
    }

    if( dwLength == 0 && dwCommand == 23 )
    {
       uint8_t *rdram_u8 = (uint8_t*)gfx_info.RDRAM;
        gObjMtx = (uObjMtx *)(rdram_u8 + dwAddr);
        gObjMtxReal.A = gObjMtx->A/65536.0f;
        gObjMtxReal.B = gObjMtx->B/65536.0f;
        gObjMtxReal.C = gObjMtx->C/65536.0f;
        gObjMtxReal.D = gObjMtx->D/65536.0f;
        gObjMtxReal.X = float(gObjMtx->X>>2);
        gObjMtxReal.Y = float(gObjMtx->Y>>2);
        gObjMtxReal.BaseScaleX = gObjMtx->BaseScaleX/1024.0f;
        gObjMtxReal.BaseScaleY = gObjMtx->BaseScaleY/1024.0f;
    }
    else if( dwLength == 2 && dwCommand == 7 )
    {
       uint8_t *rdram_u8 = (uint8_t*)gfx_info.RDRAM;
        gSubObjMtx = (uObjSubMtx*)(rdram_u8 + dwAddr);
        gObjMtxReal.X = float(gSubObjMtx->X>>2);
        gObjMtxReal.Y = float(gSubObjMtx->Y>>2);
        gObjMtxReal.BaseScaleX = gSubObjMtx->BaseScaleX/1024.0f;
        gObjMtxReal.BaseScaleY = gSubObjMtx->BaseScaleY/1024.0f;
    }

    g_MtxReal._11 = gObjMtxReal.A;
    g_MtxReal._12 = gObjMtxReal.C;
    g_MtxReal._13 = 0;
    g_MtxReal._14 = 0;//gObjMtxReal.X;

    g_MtxReal._21 = gObjMtxReal.B;
    g_MtxReal._22 = gObjMtxReal.D;
    g_MtxReal._23 = 0;
    g_MtxReal._24 = 0;//gObjMtxReal.Y;

    g_MtxReal._31 = 0;
    g_MtxReal._32 = 0;
    g_MtxReal._33 = 1.0;
    g_MtxReal._34 = 0;

    g_MtxReal._41 = gObjMtxReal.X;
    g_MtxReal._42 = gObjMtxReal.Y;
    g_MtxReal._43 = 0;
    g_MtxReal._44 = 1.0;
}

// Yoshi's Story uses this - 0x01
extern void RSP_GBI0_Mtx(Gfx *gfx);

void RSP_S2DEX_BG_1CYC(Gfx *gfx)
{
    SP_Timing(DP_Minimal16);
    DP_Timing(DP_Minimal16);

    uint8_t *rdram_u8 = (uint8_t*)gfx_info.RDRAM;
    uint32_t dwAddr = RSPSegmentAddr((gfx->words.w1));
    uObjScaleBg *sbgPtr = (uObjScaleBg *)(rdram_u8 + dwAddr);
    CRender::g_pRender->LoadObjBG1CYC(*sbgPtr);
    CRender::g_pRender->DrawObjBG1CYC(*sbgPtr, true);
}

void RSP_S2DEX_BG_1CYC_2(Gfx *gfx)
{
    if( ((gfx->words.w0)&0x00FFFFFF) != 0 )
    {
        RSP_GBI0_Mtx(gfx);
        return;
    }

    RSP_S2DEX_BG_1CYC(gfx);
}


// Yoshi's Story uses this - 0xb2
void RSP_S2DEX_OBJ_RECTANGLE_R(Gfx *gfx)
{
    uint8_t *rdram_u8 = (uint8_t*)gfx_info.RDRAM;
    uint32_t dwAddr = RSPSegmentAddr((gfx->words.w1));
    uObjSprite *ptr = (uObjSprite*)(rdram_u8 + dwAddr);

    uObjTxSprite objtx;
    memcpy(&objtx.sprite, ptr, sizeof(uObjSprite));

    //Now draw the sprite
    if( g_TxtLoadBy == CMD_LOAD_OBJ_TXTR )
    {
        memcpy(&(objtx.txtr.block), &(gObjTxtr->block), sizeof(uObjTxtr));
        //CRender::g_pRender->LoadObjSprite(*ptr, true);
        CRender::g_pRender->LoadObjSprite(objtx, true);
    }
    else
    {
        PrepareTextures();
    }
    //CRender::g_pRender->DrawSprite(*ptr, true);
    CRender::g_pRender->DrawSprite(objtx, true);
}

