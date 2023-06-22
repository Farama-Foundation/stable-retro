/*
Copyright (C) 2003 Rice1964

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

#define M64P_PLUGIN_PROTOTYPES 1
#include <stdint.h>

#include <retro_miscellaneous.h>

#include "osal_preproc.h"
#include "m64p_plugin.h"

#include "ConvertImage.h"
#include "DeviceBuilder.h"
#include "FrameBuffer.h"
#include "Render.h"

#include <algorithm>

extern FiddledVtx * g_pVtxBase;
CRender * CRender::g_pRender=NULL;
int CRender::gRenderReferenceCount=0;

XMATRIX reverseXY(-1,0,0,0,0,-1,0,0,0,0,1,0,0,0,0,1);
XMATRIX reverseY(1,0,0,0,0,-1,0,0,0,0,1,0,0,0,0,1);
extern char* right (const char * src, int nchars);

#if defined(WIN32)
  #define strcasecmp _stricmp
#endif

//========================================================================
CRender * CRender::GetRender(void)
{
   return CRender::g_pRender;
}
bool CRender::IsAvailable()
{
    return CRender::g_pRender != NULL;
}

CRender::CRender() :
    m_fScreenViewportMultX(2.0f),
    m_fScreenViewportMultY(2.0f),

    m_dwTexturePerspective(false),
    m_bAlphaTestEnable(false),

    m_bZUpdate(false),
    m_bZCompare(false),
    m_dwZBias(0),
    
    m_dwMinFilter(FILTER_POINT),
    m_dwMagFilter(FILTER_POINT),
    m_dwAlpha(0xFF),
    m_Mux(0),
    m_bBlendModeValid(false)
{
    InitRenderBase();

    for( int i=0; i<MAX_TEXTURES; i++ )
    {
        g_textures[i].m_lpsTexturePtr = NULL;
        g_textures[i].m_pCTexture = NULL;
        
        g_textures[i].m_dwTileWidth = 64;       // Value doesn't really matter, as tex not set
        g_textures[i].m_dwTileHeight = 64;
        g_textures[i].m_fTexWidth = 64.0f;      // Value doesn't really matter, as tex not set
        g_textures[i].m_fTexHeight = 64.0f;
        g_textures[i].pTextureEntry = NULL;

        TileUFlags[i] = TileVFlags[i] = TEXTURE_UV_FLAG_CLAMP;
    }


    //for( int i=0; i<MAX_VERTS; i++)
    //{
    //  g_dwVtxFlags[i] = 0;
    //}
    
    m_pColorCombiner = CDeviceBuilder::GetBuilder()->CreateColorCombiner(this);
    m_pColorCombiner->Initialize();

    m_pAlphaBlender = CDeviceBuilder::GetBuilder()->CreateAlphaBlender(this);

}

CRender::~CRender()
{
    if( m_pColorCombiner != NULL )
    {
        CDeviceBuilder::GetBuilder()->DeleteColorCombiner();
        m_pColorCombiner = NULL;
    }
    
    if( m_pAlphaBlender != NULL )
    {
        CDeviceBuilder::GetBuilder()->DeleteAlphaBlender();
        m_pAlphaBlender = NULL;
    }
}

void CRender::ResetMatrices()
{
    Matrix mat;

    mat.m[0][1] = mat.m[0][2] = mat.m[0][3] =
    mat.m[1][0] = mat.m[1][2] = mat.m[1][3] =
    mat.m[2][0] = mat.m[2][1] = mat.m[2][3] =
    mat.m[3][0] = mat.m[3][1] = mat.m[3][2] = 0.0f;

    mat.m[0][0] = mat.m[1][1] = mat.m[2][2] = mat.m[3][3] = 1.0f;

    gRSP.projectionMtxTop = 0;
    gRSP.modelViewMtxTop = 0;
    gRSP.projectionMtxs[0] = mat;
    gRSP.modelviewMtxs[0] = mat;

    gRSP.bMatrixIsUpdated = true;
    gRSP.bWorldMatrixIsUpdated = true;
    UpdateCombinedMatrix();
}

void CRender::SetProjection(const Matrix & mat, bool bPush, bool bReplace) 
{
    if (bPush)
    {
        if (gRSP.projectionMtxTop < (RICE_MATRIX_STACK-1))
            gRSP.projectionMtxTop++;

        if (bReplace)   // Load projection matrix
            gRSP.projectionMtxs[gRSP.projectionMtxTop] = mat;
        else
            gRSP.projectionMtxs[gRSP.projectionMtxTop] = mat * gRSP.projectionMtxs[gRSP.projectionMtxTop-1];
        
    }
    else
    {
        if (bReplace)   // Load projection matrix
            gRSP.projectionMtxs[gRSP.projectionMtxTop] = mat;
        else
            gRSP.projectionMtxs[gRSP.projectionMtxTop] = mat * gRSP.projectionMtxs[gRSP.projectionMtxTop];
    }
    
    gRSP.bMatrixIsUpdated = true;
}

bool mtxPopUpError = false;
void CRender::SetWorldView(const Matrix & mat, bool bPush, bool bReplace)
{
    if (bPush)
    {
        if (gRSP.modelViewMtxTop < (RICE_MATRIX_STACK-1))
            gRSP.modelViewMtxTop++;

        // We should store the current projection matrix...
        if (bReplace)
        {
            // Load projection matrix
            gRSP.modelviewMtxs[gRSP.modelViewMtxTop] = mat;

            //GSI: Hack needed to show heart in OOT & MM
            //it renders at Z coordinate = 0.0f that gets clipped away.
            //so we translate them a bit along Z to make them stick

            if( options.enableHackForGames == HACK_FOR_ZELDA || options.enableHackForGames == HACK_FOR_ZELDA_MM) 
            {
               if(gRSP.modelviewMtxs[gRSP.modelViewMtxTop]._43 == 0.0f
                     && gRSP.modelviewMtxs[gRSP.modelViewMtxTop]._42 != 0.0f
                     && gRSP.modelviewMtxs[gRSP.modelViewMtxTop]._42 <= 94.5f
                     && gRSP.modelviewMtxs[gRSP.modelViewMtxTop]._42 >= -94.5f)
                  gRSP.modelviewMtxs[gRSP.modelViewMtxTop]._43 -= 10.1f;
            }
        }
        else            // Multiply projection matrix
            gRSP.modelviewMtxs[gRSP.modelViewMtxTop] = mat * gRSP.modelviewMtxs[gRSP.modelViewMtxTop-1];
    }
    else    // NoPush
    {
        if (bReplace)   // Load projection matrix
            gRSP.modelviewMtxs[gRSP.modelViewMtxTop] = mat;
        else            // Multiply projection matrix
            gRSP.modelviewMtxs[gRSP.modelViewMtxTop] = mat * gRSP.modelviewMtxs[gRSP.modelViewMtxTop];
    }

    gRSPmodelViewTop = gRSP.modelviewMtxs[gRSP.modelViewMtxTop];
    if( options.enableHackForGames == HACK_REVERSE_XY_COOR )
        gRSPmodelViewTop = gRSPmodelViewTop * reverseXY;
    if( options.enableHackForGames == HACK_REVERSE_Y_COOR )
        gRSPmodelViewTop = gRSPmodelViewTop * reverseY;
    MatrixTranspose(&gRSPmodelViewTopTranspose, &gRSPmodelViewTop);

    gRSP.bMatrixIsUpdated = true;
    gRSP.bWorldMatrixIsUpdated = true;
}


void CRender::PopWorldView()
{
    if (gRSP.modelViewMtxTop > 0)
    {
        gRSP.modelViewMtxTop--;
        gRSPmodelViewTop = gRSP.modelviewMtxs[gRSP.modelViewMtxTop];
        if( options.enableHackForGames == HACK_REVERSE_XY_COOR )
            gRSPmodelViewTop = gRSPmodelViewTop * reverseXY;
        if( options.enableHackForGames == HACK_REVERSE_Y_COOR )
            gRSPmodelViewTop = gRSPmodelViewTop * reverseY;
        MatrixTranspose(&gRSPmodelViewTopTranspose, &gRSPmodelViewTop);
        gRSP.bMatrixIsUpdated = true;
        gRSP.bWorldMatrixIsUpdated = true;
    }
    else
    {
        mtxPopUpError = true;
    }
}


Matrix & CRender::GetWorldProjectMatrix(void)
{
    return gRSPworldProject;
}

void CRender::SetWorldProjectMatrix(Matrix &mtx)
{
    gRSPworldProject = mtx;

    gRSP.bMatrixIsUpdated = false;
    gRSP.bCombinedMatrixIsUpdated = true;
}

void CRender::SetMux(uint32_t dwMux0, uint32_t dwMux1)
{
    uint64_t tempmux = (((uint64_t)dwMux0) << 32) | (uint64_t)dwMux1;
    if( m_Mux != tempmux )
    {
        m_Mux = tempmux;
        m_bBlendModeValid = false;
        m_pColorCombiner->UpdateCombiner(dwMux0, dwMux1);
    }
}


void CRender::SetCombinerAndBlender()
{
    InitOtherModes();

    if( g_curRomInfo.bDisableBlender )
        m_pAlphaBlender->DisableAlphaBlender();
    else if( currentRomOptions.bNormalBlender )
        m_pAlphaBlender->NormalAlphaBlender();
    else
        m_pAlphaBlender->InitBlenderMode();

    m_pColorCombiner->InitCombinerMode();
}

void CRender::RenderReset()
{
    UpdateClipRectangle();
    ResetMatrices();
    SetZBias(0);
    gRSP.numVertices = 0;
    gRSP.maxVertexID = 0;
    gRSP.curTile = 0;
    gRSP.fTexScaleX = 1/32.0f;;
    gRSP.fTexScaleY = 1/32.0f;
}

bool CRender::FillRect(int nX0, int nY0, int nX1, int nY1, uint32_t dwColor)
{
    if (g_CI.dwSize != G_IM_SIZ_16b && frameBufferOptions.bIgnore) 
        return true;

    if (status.bHandleN64RenderTexture && !status.bDirectWriteIntoRDRAM)
        status.bFrameBufferIsDrawn = true;

    if(status.bVIOriginIsUpdated == true && currentRomOptions.screenUpdateSetting==SCREEN_UPDATE_AT_1ST_PRIMITIVE)
    {
        status.bVIOriginIsUpdated=false;
        CGraphicsContext::Get()->UpdateFrame(false);
    }

  if (status.bCIBufferIsRendered && status.bVIOriginIsUpdated == true && currentRomOptions.screenUpdateSetting==SCREEN_UPDATE_BEFORE_SCREEN_CLEAR )
  {
      if ((nX0==0 && nY0 == 0 && (nX1 == (int) g_CI.dwWidth || nX1 == (int) g_CI.dwWidth-1)) ||
          (nX0==gRDP.scissor.left && nY0 == gRDP.scissor.top  && (nX1 == gRDP.scissor.right || nX1 == gRDP.scissor.right-1)) ||
          ((nX0+nX1 == (int)g_CI.dwWidth || nX0+nX1 == (int)g_CI.dwWidth-1 || nX0+nX1 == gRDP.scissor.left+gRDP.scissor.right || nX0+nX1 == gRDP.scissor.left+gRDP.scissor.right-1) && (nY0 == gRDP.scissor.top || nY0 == 0 || nY0+nY1 == gRDP.scissor.top+gRDP.scissor.bottom || nY0+nY1 == gRDP.scissor.top+gRDP.scissor.bottom-1)))
      {
          status.bVIOriginIsUpdated=false;
          CGraphicsContext::Get()->UpdateFrame(false);
      }
  }


    SetFillMode(RICE_FILLMODE_SOLID);

    bool res=true;

    /*
    // I don't know why this does not work for OpenGL
    if( gRDP.otherMode.cycle_type == G_CYC_FILL && nX0 == 0 && nY0 == 0 && ((nX1==windowSetting.uViWidth && nY1==windowSetting.uViHeight)||(nX1==windowSetting.uViWidth-1 && nY1==windowSetting.uViHeight-1)) )
    {
        CGraphicsContext::g_pGraphicsContext->Clear(CLEAR_COLOR_BUFFER,dwColor, 0xFF000000, 1.0f);
    }
    else
    */
    {
        //bool m_savedZBufferFlag = gRSP.bZBufferEnabled;   // Save ZBuffer state
        ZBufferEnable( false );

        m_fillRectVtx[0].x = ViewPortTranslatei_x(nX0);
        m_fillRectVtx[0].y = ViewPortTranslatei_y(nY0);
        m_fillRectVtx[1].x = ViewPortTranslatei_x(nX1);
        m_fillRectVtx[1].y = ViewPortTranslatei_y(nY1);

        SetCombinerAndBlender();

        if( gRDP.otherMode.cycle_type  >= G_CYC_COPY )
        {
            ZBufferEnable(false);
        }
        else
        {
            //dwColor = PostProcessDiffuseColor(0);
            dwColor = PostProcessDiffuseColor(gRDP.primitiveColor);
        }

        float depth = (gRDP.otherMode.depth_source == 1 ? gRDP.fPrimitiveDepth : 0 );

        ApplyRDPScissor(false);
        TurnFogOnOff(false);
        res = RenderFillRect(dwColor, depth);
        TurnFogOnOff(gRSP.bFogEnabled);

        if( gRDP.otherMode.cycle_type  >= G_CYC_COPY )
        {
            ZBufferEnable(gRSP.bZBufferEnabled);
        }
    }

    if( options.bWinFrameMode )
        SetFillMode(RICE_FILLMODE_WINFRAME);

    return res;
}


bool CRender::Line3D(uint32_t dwV0, uint32_t dwV1, uint32_t dwWidth)
{
    if( !status.bCIBufferIsRendered )
        g_pFrameBufferManager->ActiveTextureBuffer();

    m_line3DVtx[0].z = (g_vecProjected[dwV0].z + 1.0f) * 0.5f;
    m_line3DVtx[1].z = (g_vecProjected[dwV1].z + 1.0f) * 0.5f;

    if( m_line3DVtx[0].z != m_line3DVtx[1].z )  
        return false;

    if( status.bHandleN64RenderTexture && !status.bDirectWriteIntoRDRAM )
        status.bFrameBufferIsDrawn = true;

    if( status.bHandleN64RenderTexture ) 
    {
        g_pRenderTextureInfo->maxUsedHeight = g_pRenderTextureInfo->N64Height;
        if( status.bHandleN64RenderTexture && !status.bDirectWriteIntoRDRAM )   
        {
            status.bFrameBufferIsDrawn = true;
            status.bFrameBufferDrawnByTriangles = true;
        }
    }

    m_line3DVtx[0].x = ViewPortTranslatef_x(g_vecProjected[dwV0].x);
    m_line3DVtx[0].y = ViewPortTranslatef_y(g_vecProjected[dwV0].y);
    m_line3DVtx[0].rhw = g_vecProjected[dwV0].w;
    m_line3DVtx[0].dcDiffuse = PostProcessDiffuseColor(g_dwVtxDifColor[dwV0]);
    m_line3DVtx[0].dcSpecular = PostProcessSpecularColor();

    m_line3DVtx[1].x = ViewPortTranslatef_x(g_vecProjected[dwV1].x);
    m_line3DVtx[1].y = ViewPortTranslatef_y(g_vecProjected[dwV1].y);
    m_line3DVtx[1].rhw = g_vecProjected[dwV1].w;
    m_line3DVtx[1].dcDiffuse = PostProcessDiffuseColor(g_dwVtxDifColor[dwV1]);
    m_line3DVtx[1].dcSpecular = m_line3DVtx[0].dcSpecular;

    float width = dwWidth*0.5f+1.5f;

    if( m_line3DVtx[0].y == m_line3DVtx[1].y )
    {
        m_line3DVector[0].x = m_line3DVector[1].x = m_line3DVtx[0].x;
        m_line3DVector[2].x = m_line3DVector[3].x = m_line3DVtx[1].x;

        m_line3DVector[0].y = m_line3DVector[2].y = m_line3DVtx[0].y-width/2*windowSetting.fMultY;
        m_line3DVector[1].y = m_line3DVector[3].y = m_line3DVtx[0].y+width/2*windowSetting.fMultY;
    }
    else
    {
        m_line3DVector[0].y = m_line3DVector[1].y = m_line3DVtx[0].y;
        m_line3DVector[2].y = m_line3DVector[3].y = m_line3DVtx[1].y;

        m_line3DVector[0].x = m_line3DVector[2].x = m_line3DVtx[0].x-width/2*windowSetting.fMultX;
        m_line3DVector[1].x = m_line3DVector[3].x = m_line3DVtx[0].x+width/2*windowSetting.fMultX;
    }

    SetCombinerAndBlender();

    return RenderLine3D();
}

bool CRender::RemapTextureCoordinate
    (float t0, float t1, uint32_t tileWidth, uint32_t mask, float textureWidth, float &u0, float &u1)
{
    int s0 = (int)t0;
    int s1 = (int)t1;
    int width = mask>0 ? (1<<mask) : tileWidth;
    if( width == 0 ) return false;

    int divs0 = s0/width; if( divs0*width > s0 )    divs0--;
    int divs1 = s1/width; if( divs1*width > s1 )    divs1--;

    if( divs0 == divs1 )
    {
        s0 -= divs0*width;
        s1 -= divs1*width;
        //if( s0 > s1 ) 
        //  s0++;
        //else if( s1 > s0 )    
        //  s1++;
        u0 = s0/textureWidth;
        u1 = s1/textureWidth;

        return true;
    }
    else if( divs0+1 == divs1 && s0%width==0 && s1%width == 0 )
    {
        u0 = 0;
        u1 = tileWidth/textureWidth;
        return true;
    }
    else if( divs0 == divs1+1 && s0%width==0 && s1%width == 0 )
    {
        u1 = 0;
        u0 = tileWidth/textureWidth;
        return true;
    }
    else
    {
        //if( s0 > s1 ) 
        //{
            //s0++;
        //  u0 = s0/textureWidth;
        //}
        //else if( s1 > s0 )    
        //{
            //s1++;
        //  u1 = s1/textureWidth;
        //}

        return false;
    }
}

bool CRender::TexRect(int nX0, int nY0, int nX1, int nY1, float fS0, float fT0, float fScaleS, float fScaleT, bool colorFlag, uint32_t diffuseColor)
{
    if( options.enableHackForGames == HACK_FOR_DUKE_NUKEM )
    {
        colorFlag = true;
        diffuseColor = 0;
    }

    if( status.bVIOriginIsUpdated == true && currentRomOptions.screenUpdateSetting==SCREEN_UPDATE_AT_1ST_PRIMITIVE )
    {
        status.bVIOriginIsUpdated=false;
        CGraphicsContext::Get()->UpdateFrame(false);
    }

    if( options.enableHackForGames == HACK_FOR_BANJO_TOOIE )
    {
        // Hack for Banjo shadow in Banjo Tooie
        if( g_TI.dwWidth == g_CI.dwWidth && g_TI.dwFormat == G_IM_FMT_CI && g_TI.dwSize == G_IM_SIZ_8b )
        {
           // Skip the text rect
           if( nX0 == fS0 && nY0 == fT0 )//&& nX0 > 90 && nY0 > 130 && nX1-nX0 > 80 && nY1-nY0 > 20 )
              return true;
        }
    }

    if( status.bN64IsDrawingTextureBuffer )
    {
        if( frameBufferOptions.bIgnore || ( frameBufferOptions.bIgnoreRenderTextureIfHeightUnknown && newRenderTextureInfo.knownHeight == 0 ) )
            return true;
    }

    PrepareTextures();

    if( status.bHandleN64RenderTexture && g_pRenderTextureInfo->CI_Info.dwSize == G_IM_SIZ_8b ) 
        return true;

    if( !IsTextureEnabled() &&  gRDP.otherMode.cycle_type  != G_CYC_COPY )
    {
        FillRect(nX0, nY0, nX1, nY1, gRDP.primitiveColor);
        return true;
    }


    if( IsUsedAsDI(g_CI.dwAddr) && !status.bHandleN64RenderTexture )
        status.bFrameBufferIsDrawn = true;

    if( status.bHandleN64RenderTexture && !status.bDirectWriteIntoRDRAM )   status.bFrameBufferIsDrawn = true;

    if( options.bEnableHacks )
    {
        // Goldeneye HACK
        if( nY1 - nY0 < 2 ) 
            nY1 = nY1+2;

        //// Text edge hack
        else if( gRDP.otherMode.cycle_type == G_CYC_1CYCLE && fScaleS == 1 && fScaleT == 1 && 
            (int)g_textures[gRSP.curTile].m_dwTileWidth == nX1-nX0+1 && (int)g_textures[gRSP.curTile].m_dwTileHeight == nY1-nY0+1 &&
            g_textures[gRSP.curTile].m_dwTileWidth%2 == 0 && g_textures[gRSP.curTile].m_dwTileHeight%2 == 0 )
        {
            nY1++;
            nX1++;
        }
        else if( g_curRomInfo.bIncTexRectEdge )
        {
            nX1++;
            nY1++;
        }
    }


    // Scale to Actual texture coords
    // The two cases are to handle the oversized textures hack on voodoos

    SetCombinerAndBlender();
    

    if( gRDP.otherMode.cycle_type  >= G_CYC_COPY || !gRDP.otherMode.z_cmp )
        ZBufferEnable(false);

    bool accurate = currentRomOptions.bAccurateTextureMapping;

    RenderTexture &tex0 = g_textures[gRSP.curTile];
    TileAdditionalInfo *tile0 = &gRDP.tilesinfo[gRSP.curTile];
    gDPTile *tile0info = &gDP.tiles[gRSP.curTile];

    float widthDiv = tex0.m_fTexWidth;
    float heightDiv = tex0.m_fTexHeight;
    float t0u0;
    if( options.enableHackForGames == HACK_FOR_ALL_STAR_BASEBALL || options.enableHackForGames == HACK_FOR_MLB )
        t0u0 = (fS0 -tile0->fhilite_sl);
    else
        t0u0 = (fS0 * tile0->fShiftScaleS -tile0->fhilite_sl);
    
    float t0u1;
    if( accurate && gRDP.otherMode.cycle_type >= G_CYC_COPY )
        t0u1 = t0u0 + (fScaleS * (nX1 - nX0 - 1))*tile0->fShiftScaleS;
    else
        t0u1 = t0u0 + (fScaleS * (nX1 - nX0))*tile0->fShiftScaleS;


    if( status.UseLargerTile[0] )
    {
        m_texRectTex1UV[0].u = (t0u0+status.LargerTileRealLeft[0])/widthDiv;
        m_texRectTex1UV[1].u = (t0u1+status.LargerTileRealLeft[0])/widthDiv;
    }
    else
    {
        m_texRectTex1UV[0].u = t0u0/widthDiv;
        m_texRectTex1UV[1].u = t0u1/widthDiv;
        if( accurate && !tile0info->mirrors && RemapTextureCoordinate(t0u0, t0u1, tex0.m_dwTileWidth, tile0info->masks, widthDiv, m_texRectTex1UV[0].u, m_texRectTex1UV[1].u) )
            SetTextureUFlag(TEXTURE_UV_FLAG_CLAMP, gRSP.curTile);
    }

    float t0v0;
    if( options.enableHackForGames == HACK_FOR_ALL_STAR_BASEBALL || options.enableHackForGames == HACK_FOR_MLB )
        t0v0 = (fT0 -tile0->fhilite_tl);
    else
        t0v0 = (fT0 * tile0->fShiftScaleT -tile0->fhilite_tl);
    
    float t0v1;
    if ( accurate && gRDP.otherMode.cycle_type >= G_CYC_COPY)
        t0v1 = t0v0 + (fScaleT * (nY1 - nY0-1))*tile0->fShiftScaleT;
    else
        t0v1 = t0v0 + (fScaleT * (nY1 - nY0))*tile0->fShiftScaleT;

    m_texRectTex1UV[0].v = t0v0/heightDiv;
    m_texRectTex1UV[1].v = t0v1/heightDiv;
    if( accurate && !tile0info->mirrort && RemapTextureCoordinate(t0v0, t0v1, tex0.m_dwTileHeight, tile0info->maskt, heightDiv, m_texRectTex1UV[0].v, m_texRectTex1UV[1].v) )
        SetTextureVFlag(TEXTURE_UV_FLAG_CLAMP, gRSP.curTile);
    
    COLOR speColor = PostProcessSpecularColor();
    COLOR difColor;
    if( colorFlag )
        difColor = PostProcessDiffuseColor(diffuseColor);
    else
        //difColor = PostProcessDiffuseColor(0);
        difColor = PostProcessDiffuseColor(gRDP.primitiveColor);

    g_texRectTVtx[0].x = ViewPortTranslatei_x(nX0);
    g_texRectTVtx[0].y = ViewPortTranslatei_y(nY0);
    g_texRectTVtx[0].dcDiffuse = difColor;
    g_texRectTVtx[0].dcSpecular = speColor;

    g_texRectTVtx[1].x = ViewPortTranslatei_x(nX1);
    g_texRectTVtx[1].y = ViewPortTranslatei_y(nY0);
    g_texRectTVtx[1].dcDiffuse = difColor;
    g_texRectTVtx[1].dcSpecular = speColor;

    g_texRectTVtx[2].x = ViewPortTranslatei_x(nX1);
    g_texRectTVtx[2].y = ViewPortTranslatei_y(nY1);
    g_texRectTVtx[2].dcDiffuse = difColor;
    g_texRectTVtx[2].dcSpecular = speColor;

    g_texRectTVtx[3].x = ViewPortTranslatei_x(nX0);
    g_texRectTVtx[3].y = ViewPortTranslatei_y(nY1);
    g_texRectTVtx[3].dcDiffuse = difColor;
    g_texRectTVtx[3].dcSpecular = speColor;

    float depth = (gRDP.otherMode.depth_source == 1 ? gRDP.fPrimitiveDepth : 0 );

    g_texRectTVtx[0].z = g_texRectTVtx[1].z = g_texRectTVtx[2].z = g_texRectTVtx[3].z = depth;
    g_texRectTVtx[0].rhw = g_texRectTVtx[1].rhw = g_texRectTVtx[2].rhw = g_texRectTVtx[3].rhw = 1;

    if( IsTexel1Enable() )
    {
        RenderTexture &tex1 = g_textures[(gRSP.curTile+1)&7];
        TileAdditionalInfo *tile1 = &gRDP.tilesinfo[(gRSP.curTile+1)&7];
        gDPTile *tile1info = &gDP.tiles[(gRSP.curTile+1)&7];

        widthDiv = tex1.m_fTexWidth;
        heightDiv = tex1.m_fTexHeight;

        float t0u0 = fS0 * tile1->fShiftScaleS -tile1->fhilite_sl;
        float t0v0 = fT0 * tile1->fShiftScaleT -tile1->fhilite_tl;
        float t0u1;
        float t0v1;
        if ( accurate && gRDP.otherMode.cycle_type >= G_CYC_COPY)
        {
            t0u1 = t0u0 + (fScaleS * (nX1 - nX0 - 1))*tile1->fShiftScaleS;
            t0v1 = t0v0 + (fScaleT * (nY1 - nY0 - 1))*tile1->fShiftScaleT;
        }
        else
        {
            t0u1 = t0u0 + (fScaleS * (nX1 - nX0))*tile1->fShiftScaleS;
            t0v1 = t0v0 + (fScaleT * (nY1 - nY0))*tile1->fShiftScaleT;
        }

        if( status.UseLargerTile[1] )
        {
            m_texRectTex2UV[0].u = (t0u0+status.LargerTileRealLeft[1])/widthDiv;
            m_texRectTex2UV[1].u = (t0u1+status.LargerTileRealLeft[1])/widthDiv;
        }
        else
        {
            m_texRectTex2UV[0].u = t0u0/widthDiv;
            m_texRectTex2UV[1].u = t0u1/widthDiv;
            if( accurate && !tile1info->mirrors && RemapTextureCoordinate(t0u0, t0u1, tex1.m_dwTileWidth, tile1info->masks, widthDiv, m_texRectTex2UV[0].u, m_texRectTex2UV[1].u) )
                SetTextureUFlag(TEXTURE_UV_FLAG_CLAMP, (gRSP.curTile+1)&7);
        }

        m_texRectTex2UV[0].v = t0v0/heightDiv;
        m_texRectTex2UV[1].v = t0v1/heightDiv;

        if( accurate && !tile1info->mirrort && RemapTextureCoordinate(t0v0, t0v1, tex1.m_dwTileHeight, tile1info->maskt, heightDiv, m_texRectTex2UV[0].v, m_texRectTex2UV[1].v) )
            SetTextureVFlag(TEXTURE_UV_FLAG_CLAMP, (gRSP.curTile+1)&7);

        SetVertexTextureUVCoord(g_texRectTVtx[0], m_texRectTex1UV[0].u, m_texRectTex1UV[0].v, m_texRectTex2UV[0].u, m_texRectTex2UV[0].v);
        SetVertexTextureUVCoord(g_texRectTVtx[1], m_texRectTex1UV[1].u, m_texRectTex1UV[0].v, m_texRectTex2UV[1].u, m_texRectTex2UV[0].v);
        SetVertexTextureUVCoord(g_texRectTVtx[2], m_texRectTex1UV[1].u, m_texRectTex1UV[1].v, m_texRectTex2UV[1].u, m_texRectTex2UV[1].v);
        SetVertexTextureUVCoord(g_texRectTVtx[3], m_texRectTex1UV[0].u, m_texRectTex1UV[1].v, m_texRectTex2UV[0].u, m_texRectTex2UV[1].v);
    }
    else
    {
        SetVertexTextureUVCoord(g_texRectTVtx[0], m_texRectTex1UV[0].u, m_texRectTex1UV[0].v);
        SetVertexTextureUVCoord(g_texRectTVtx[1], m_texRectTex1UV[1].u, m_texRectTex1UV[0].v);
        SetVertexTextureUVCoord(g_texRectTVtx[2], m_texRectTex1UV[1].u, m_texRectTex1UV[1].v);
        SetVertexTextureUVCoord(g_texRectTVtx[3], m_texRectTex1UV[0].u, m_texRectTex1UV[1].v);
    }


    bool res;
    TurnFogOnOff(false);
    if( TileUFlags[gRSP.curTile]==TEXTURE_UV_FLAG_CLAMP && TileVFlags[gRSP.curTile]==TEXTURE_UV_FLAG_CLAMP && options.forceTextureFilter == FORCE_DEFAULT_FILTER )
    {
        TextureFilter dwFilter = m_dwMagFilter;
        m_dwMagFilter = m_dwMinFilter = FILTER_LINEAR;
        ApplyTextureFilter();
        ApplyRDPScissor(false);
        res = RenderTexRect();
        m_dwMagFilter = m_dwMinFilter = dwFilter;
        ApplyTextureFilter();
    }
    else if( fScaleS >= 1 && fScaleT >= 1 && options.forceTextureFilter == FORCE_DEFAULT_FILTER )
    {
        TextureFilter dwFilter = m_dwMagFilter;
        m_dwMagFilter = m_dwMinFilter = FILTER_POINT;
        ApplyTextureFilter();
        ApplyRDPScissor(false);
        res = RenderTexRect();
        m_dwMagFilter = m_dwMinFilter = dwFilter;
        ApplyTextureFilter();
    }
    else
    {
        ApplyRDPScissor(false);
        res = RenderTexRect();
    }
    TurnFogOnOff(gRSP.bFogEnabled);

    if( gRDP.otherMode.cycle_type  >= G_CYC_COPY || !gRDP.otherMode.z_cmp  )
        ZBufferEnable(gRSP.bZBufferEnabled);

    return res;
}


bool CRender::TexRectFlip(int nX0, int nY0, int nX1, int nY1, float fS0, float fT0, float fS1, float fT1)
{
    if( status.bHandleN64RenderTexture && !status.bDirectWriteIntoRDRAM )   
    {
        status.bFrameBufferIsDrawn = true;
        status.bFrameBufferDrawnByTriangles = true;
    }

    PrepareTextures();

    // Save ZBuffer state
    m_savedZBufferFlag = gRSP.bZBufferEnabled;
    if( gRDP.otherMode.depth_source == 0 ) 
        ZBufferEnable( false );

    float widthDiv = g_textures[gRSP.curTile].m_fTexWidth;
    float heightDiv = g_textures[gRSP.curTile].m_fTexHeight;

    //Tile &tile0 = gRDP.tiles[gRSP.curTile];

    float t0u0 = fS0 / widthDiv;
    float t0v0 = fT0 / heightDiv;

    float t0u1 = (fS1 - fS0)/ widthDiv + t0u0;
    float t0v1 = (fT1 - fT0)/ heightDiv + t0v0;

    float depth = (gRDP.otherMode.depth_source == 1 ? gRDP.fPrimitiveDepth : 0 );

    if( t0u0 >= 0 && t0u1 <= 1 && t0u1 >= t0u0 ) SetTextureUFlag(TEXTURE_UV_FLAG_CLAMP, gRSP.curTile);
    if( t0v0 >= 0 && t0v1 <= 1 && t0v1 >= t0v0 ) SetTextureVFlag(TEXTURE_UV_FLAG_CLAMP, gRSP.curTile);

    SetCombinerAndBlender();

    COLOR speColor = PostProcessSpecularColor();
    COLOR difColor = PostProcessDiffuseColor(gRDP.primitiveColor);

    // Same as TexRect, but with texcoords 0,2 swapped
    g_texRectTVtx[0].x = ViewPortTranslatei_x(nX0);
    g_texRectTVtx[0].y = ViewPortTranslatei_y(nY0);
    g_texRectTVtx[0].dcDiffuse = difColor;
    g_texRectTVtx[0].dcSpecular = speColor;

    g_texRectTVtx[1].x = ViewPortTranslatei_x(nX1);
    g_texRectTVtx[1].y = ViewPortTranslatei_y(nY0);
    g_texRectTVtx[1].dcDiffuse = difColor;
    g_texRectTVtx[1].dcSpecular = speColor;

    g_texRectTVtx[2].x = ViewPortTranslatei_x(nX1);
    g_texRectTVtx[2].y = ViewPortTranslatei_y(nY1);
    g_texRectTVtx[2].dcDiffuse = difColor;
    g_texRectTVtx[2].dcSpecular = speColor;

    g_texRectTVtx[3].x = ViewPortTranslatei_x(nX0);
    g_texRectTVtx[3].y = ViewPortTranslatei_y(nY1);
    g_texRectTVtx[3].dcDiffuse = difColor;
    g_texRectTVtx[3].dcSpecular = speColor;

    g_texRectTVtx[0].z = g_texRectTVtx[1].z = g_texRectTVtx[2].z = g_texRectTVtx[3].z = depth;
    g_texRectTVtx[0].rhw = g_texRectTVtx[1].rhw = g_texRectTVtx[2].rhw = g_texRectTVtx[3].rhw = 1.0f;
    
    SetVertexTextureUVCoord(g_texRectTVtx[0], t0u0, t0v0);
    SetVertexTextureUVCoord(g_texRectTVtx[1], t0u0, t0v1);
    SetVertexTextureUVCoord(g_texRectTVtx[2], t0u1, t0v1);
    SetVertexTextureUVCoord(g_texRectTVtx[3], t0u1, t0v0);

    TurnFogOnOff(false);
    ApplyRDPScissor(false);
    bool res = RenderTexRect();

    TurnFogOnOff(gRSP.bFogEnabled);

    // Restore state
    ZBufferEnable( m_savedZBufferFlag );

    return res;
}


void CRender::StartDrawSimple2DTexture(float x0, float y0, float x1, float y1, float u0, float v0, float u1, float v1, COLOR dif, COLOR spe, float z, float rhw)
{
    g_texRectTVtx[0].x = ViewPortTranslatei_x(x0);  // << Error here, shouldn't divid by 4
    g_texRectTVtx[0].y = ViewPortTranslatei_y(y0);
    g_texRectTVtx[0].dcDiffuse = dif;
    g_texRectTVtx[0].dcSpecular = spe;
    g_texRectTVtx[0].tcord[0].u = u0;
    g_texRectTVtx[0].tcord[0].v = v0;


    g_texRectTVtx[1].x = ViewPortTranslatei_x(x1);
    g_texRectTVtx[1].y = ViewPortTranslatei_y(y0);
    g_texRectTVtx[1].dcDiffuse = dif;
    g_texRectTVtx[1].dcSpecular = spe;
    g_texRectTVtx[1].tcord[0].u = u1;
    g_texRectTVtx[1].tcord[0].v = v0;

    g_texRectTVtx[2].x = ViewPortTranslatei_x(x1);
    g_texRectTVtx[2].y = ViewPortTranslatei_y(y1);
    g_texRectTVtx[2].dcDiffuse = dif;
    g_texRectTVtx[2].dcSpecular = spe;
    g_texRectTVtx[2].tcord[0].u = u1;
    g_texRectTVtx[2].tcord[0].v = v1;

    g_texRectTVtx[3].x = ViewPortTranslatei_x(x0);
    g_texRectTVtx[3].y = ViewPortTranslatei_y(y1);
    g_texRectTVtx[3].dcDiffuse = dif;
    g_texRectTVtx[3].dcSpecular = spe;
    g_texRectTVtx[3].tcord[0].u = u0;
    g_texRectTVtx[3].tcord[0].v = v1;

    RenderTexture &txtr = g_textures[0];
    if( txtr.pTextureEntry && txtr.pTextureEntry->txtrBufIdx > 0 )
    {
        RenderTextureInfo &info = gRenderTextureInfos[txtr.pTextureEntry->txtrBufIdx-1];
        g_texRectTVtx[0].tcord[0].u *= info.scaleX;
        g_texRectTVtx[0].tcord[0].v *= info.scaleY;
        g_texRectTVtx[1].tcord[0].u *= info.scaleX;
        g_texRectTVtx[1].tcord[0].v *= info.scaleY;
        g_texRectTVtx[2].tcord[0].u *= info.scaleX;
        g_texRectTVtx[2].tcord[0].v *= info.scaleY;
        g_texRectTVtx[3].tcord[0].u *= info.scaleX;
        g_texRectTVtx[3].tcord[0].v *= info.scaleY;
    }

    g_texRectTVtx[0].z = g_texRectTVtx[1].z = g_texRectTVtx[2].z = g_texRectTVtx[3].z = z;
    g_texRectTVtx[0].rhw = g_texRectTVtx[1].rhw = g_texRectTVtx[2].rhw = g_texRectTVtx[3].rhw = rhw;
}

void CRender::StartDrawSimpleRect(int nX0, int nY0, int nX1, int nY1, uint32_t dwColor, float depth, float rhw)
{
    m_simpleRectVtx[0].x = ViewPortTranslatei_x(nX0);
    m_simpleRectVtx[1].x = ViewPortTranslatei_x(nX1);
    m_simpleRectVtx[0].y = ViewPortTranslatei_y(nY0);
    m_simpleRectVtx[1].y = ViewPortTranslatei_y(nY1);
}

void CRender::SetAddressUAllStages(uint32_t dwTile, TextureUVFlag dwFlag)
{
}

void CRender::SetAddressVAllStages(uint32_t dwTile, TextureUVFlag dwFlag)
{
}

void CRender::SetAllTexelRepeatFlag()
{
    if( IsTextureEnabled() )
    {
        if( IsTexel0Enable() || gRDP.otherMode.cycle_type  == G_CYC_COPY )
            SetTexelRepeatFlags(gRSP.curTile);
        if( IsTexel1Enable() )
            SetTexelRepeatFlags((gRSP.curTile+1)&7);
    }
}


void CRender::SetTexelRepeatFlags(uint32_t dwTile)
{
    TileAdditionalInfo *tile = &gRDP.tilesinfo[dwTile];
    gDPTile        *tileinfo = &gDP.tiles[dwTile];

    if( tile->bForceClampS )
        SetTextureUFlag(TEXTURE_UV_FLAG_CLAMP, dwTile);
    else if( tile->bForceWrapS )
            SetTextureUFlag(TEXTURE_UV_FLAG_WRAP, dwTile);
    else if( tileinfo->masks == 0 || tileinfo->clamps )
    {
        if( gRDP.otherMode.cycle_type  >= G_CYC_COPY )
            SetTextureUFlag(TEXTURE_UV_FLAG_WRAP, dwTile);  // Can not clamp in COPY/FILL mode
        else
            SetTextureUFlag(TEXTURE_UV_FLAG_CLAMP, dwTile);
    }
    else if (tileinfo->mirrors )
        SetTextureUFlag(TEXTURE_UV_FLAG_MIRROR, dwTile);
    else                                
        SetTextureUFlag(TEXTURE_UV_FLAG_WRAP, dwTile);
    
    if( tile->bForceClampT )
        SetTextureVFlag(TEXTURE_UV_FLAG_CLAMP, dwTile);
    else if( tile->bForceWrapT )
        SetTextureVFlag(TEXTURE_UV_FLAG_WRAP, dwTile);
    else if( tileinfo->maskt == 0 || tileinfo->clampt)
    {
        if( gRDP.otherMode.cycle_type  >= G_CYC_COPY )
            SetTextureVFlag(TEXTURE_UV_FLAG_WRAP, dwTile);  // Can not clamp in COPY/FILL mode
        else
            SetTextureVFlag(TEXTURE_UV_FLAG_CLAMP, dwTile);
    }
    else if (tileinfo->mirrort )
        SetTextureVFlag(TEXTURE_UV_FLAG_MIRROR, dwTile);
    else                                
        SetTextureVFlag(TEXTURE_UV_FLAG_WRAP, dwTile);
}

void CRender::Initialize(void)
{
    ClearDeviceObjects();
    InitDeviceObjects();
}

void CRender::CleanUp(void)
{
    m_pColorCombiner->CleanUp();
    ClearDeviceObjects();
}

void myVec3Transform(float *vecout, float *vecin, float* m)
{
    float w = m[3]*vecin[0]+m[7]*vecin[1]+m[11]*vecin[2]+m[15];
    vecout[0] = (m[0]*vecin[0]+m[4]*vecin[1]+m[8]*vecin[2]+m[12])/w;
    vecout[1] = (m[1]*vecin[0]+m[5]*vecin[1]+m[9]*vecin[2]+m[13])/w;
    vecout[2] = (m[2]*vecin[0]+m[6]*vecin[1]+m[10]*vecin[2]+m[14])/w;
}

void CRender::SetTextureEnableAndScale(int dwTile, bool bEnable, float fScaleX, float fScaleY)
{
    gRSP.bTextureEnabled = bEnable;

    if( bEnable )
    {
        if( gRSP.curTile != (unsigned int)dwTile )
            gRDP.textureIsChanged = true;

        gRSP.curTile    = dwTile;
        gRSP.fTexScaleX = fScaleX;
        gRSP.fTexScaleY = fScaleY;

        if( fScaleX == 0 || fScaleY == 0 )
        {
            gRSP.fTexScaleX = 1/32.0f;
            gRSP.fTexScaleY = 1/32.0f;
        }
    }
}

void CRender::SetViewport(int nLeft, int nTop, int nRight, int nBottom, int maxZ)
{
    if( status.bHandleN64RenderTexture )
        return;

    static float MultX=0, MultY=0;

    if( gRSP.nVPLeftN == nLeft && gRSP.nVPTopN == nTop &&
        gRSP.nVPRightN == nRight && gRSP.nVPBottomN == nBottom &&
        MultX==windowSetting.fMultX && MultY==windowSetting.fMultY)
    {
        // no changes
        return;
    }

    MultX=windowSetting.fMultX;
    MultY=windowSetting.fMultY;

    gRSP.maxZ = maxZ;
    gRSP.nVPLeftN = nLeft;
    gRSP.nVPTopN = nTop;
    gRSP.nVPRightN = nRight;
    gRSP.nVPBottomN = nBottom;
    gRSP.nVPWidthN = nRight - nLeft + 1;
    gRSP.nVPHeightN = nBottom - nTop + 1;

    UpdateClipRectangle();
    SetViewportRender();
}

extern bool bHalfTxtScale;

bool CRender::DrawTriangles()
{
    if( !status.bCIBufferIsRendered )
        g_pFrameBufferManager->ActiveTextureBuffer();

    if( status.bVIOriginIsUpdated == true && currentRomOptions.screenUpdateSetting==SCREEN_UPDATE_AT_1ST_PRIMITIVE )
    {
        status.bVIOriginIsUpdated=false;
        CGraphicsContext::Get()->UpdateFrame(false);
    }

    // Hack for Pilotwings 64 (U) [!].v64
    static bool skipNext=false;
    if( options.enableHackForGames == HACK_FOR_PILOT_WINGS )
    {
        if( IsUsedAsDI(g_CI.dwAddr) && gRDP.otherMode.z_cmp+gRDP.otherMode.z_upd > 0 )
        {
            TRACE0("Warning: using Flushtris to write Z-Buffer" );
            gRSP.numVertices = 0;
            gRSP.maxVertexID = 0;
            skipNext = true;
            return true;
        }
        else if( skipNext )
        {
            skipNext = false;
            gRSP.numVertices = 0;
            gRSP.maxVertexID = 0;
            return true;
        }   
    }

    if( status.bN64IsDrawingTextureBuffer && frameBufferOptions.bIgnore )
    {
        gRSP.numVertices = 0;
        gRSP.maxVertexID = 0;
        return true;
    }

    extern bool bConkerHideShadow;
    if( options.enableHackForGames == HACK_FOR_CONKER && bConkerHideShadow )
    {
        gRSP.numVertices = 0;
        gRSP.maxVertexID = 0;
        return true;
    }

    if( IsUsedAsDI(g_CI.dwAddr) && !status.bHandleN64RenderTexture )
    {
        status.bFrameBufferIsDrawn = true;
    }

    /*
    if( status.bHandleN64RenderTexture && g_pRenderTextureInfo->CI_Info.dwSize == G_IM_SIZ_8b ) 
    {
        gRSP.numVertices = 0;
        gRSP.maxVertexID = 0;
        return true;
    }
    */

    if (gRSP.numVertices == 0) 
        return true;

    if( status.bHandleN64RenderTexture )
    {
        g_pRenderTextureInfo->maxUsedHeight = g_pRenderTextureInfo->N64Height;
        if( !status.bDirectWriteIntoRDRAM ) 
        {
            status.bFrameBufferIsDrawn = true;
            status.bFrameBufferDrawnByTriangles = true;
        }
    }

    if( !gRDP.bFogEnableInBlender && gRSP.bFogEnabled )
    {
        TurnFogOnOff(false);
    }

    for( int t=0; t<2; t++ )
    {
        float halfscaleS = 1;

        // This will get rid of the thin black lines
        if( t==0 && !(m_pColorCombiner->m_bTex0Enabled) )
        {
            continue;
        }
        else
        {
           if( (    gDP.tiles[gRSP.curTile].size == G_IM_SIZ_32b && 
                    options.enableHackForGames == HACK_FOR_RUMBLE ) ||
                 (bHalfTxtScale && g_curRomInfo.bTextureScaleHack ) ||
                 (
                  options.enableHackForGames == HACK_FOR_POLARISSNOCROSS && 
                  gDP.tiles[7].format == G_IM_FMT_CI && 
                  gDP.tiles[7].size == G_IM_SIZ_8b   &&
                  gDP.tiles[0].format == G_IM_FMT_CI &&
                  gDP.tiles[0].size == G_IM_SIZ_8b   && 
                  gRSP.curTile == 0 )
             )
            {
                halfscaleS = 0.5;
            }
        }

        if( t==1 && !(m_pColorCombiner->m_bTex1Enabled) )
            break;

        if( halfscaleS < 1 )
        {
            for( uint32_t i=0; i<gRSP.numVertices; i++ )
            {
                if( t == 0 )
                {
                    g_vtxBuffer[i].tcord[t].u += gRSP.tex0OffsetX;
                    g_vtxBuffer[i].tcord[t].u /= 2;
                    g_vtxBuffer[i].tcord[t].u -= gRSP.tex0OffsetX;
                    g_vtxBuffer[i].tcord[t].v += gRSP.tex0OffsetY;
                    g_vtxBuffer[i].tcord[t].v /= 2;
                    g_vtxBuffer[i].tcord[t].v -= gRSP.tex0OffsetY;
                }
                else
                {
                    g_vtxBuffer[i].tcord[t].u += gRSP.tex1OffsetX;
                    g_vtxBuffer[i].tcord[t].u /= 2;
                    g_vtxBuffer[i].tcord[t].u -= gRSP.tex1OffsetX;
                    g_vtxBuffer[i].tcord[t].v += gRSP.tex1OffsetY;
                    g_vtxBuffer[i].tcord[t].v /= 2;
                    g_vtxBuffer[i].tcord[t].v -= gRSP.tex1OffsetY;
                }
            }
        }

        /*
        // The code here is disabled because it could cause incorrect texture repeating flag 
        // for later DrawTriangles
        bool clampS=true;
        bool clampT=true;

        for( uint32_t i=0; i<gRSP.numVertices; i++ )
        {
            float w = g_vtxProjected5[i][3]; 
            if( w < 0 || g_vtxBuffer[i].tcord[t].u > 1.0 || g_vtxBuffer[i].tcord[t].u < 0.0  )
            {
                clampS = false;
                break;
            }
        }

        for( uint32_t i=0; i<gRSP.numVertices; i++ )
        {
            float w = g_vtxProjected5[i][3]; 
            if( w < 0 || g_vtxBuffer[i].tcord[t].v > 1.0 || g_vtxBuffer[i].tcord[t].v < 0.0  )
            {
                clampT = false;
                break;
            }
        }

        if( clampS )
        {
            SetTextureUFlag(TEXTURE_UV_FLAG_CLAMP, gRSP.curTile+t);
        }
        if( clampT )
        {
            SetTextureVFlag(TEXTURE_UV_FLAG_CLAMP, gRSP.curTile+t);
        }
        */
    }

    if( status.bHandleN64RenderTexture && g_pRenderTextureInfo->CI_Info.dwSize == G_IM_SIZ_8b )
    {
        ZBufferEnable(false);
    }

    ApplyScissorWithClipRatio(false);

    if( g_curRomInfo.bZHack )
    {
        extern void HackZAll();
        HackZAll();
    }

    bool res = RenderFlushTris();
    g_clippedVtxCount = 0;

    gRSP.numVertices = 0;   // Reset index
    gRSP.maxVertexID = 0;

    if( !gRDP.bFogEnableInBlender && gRSP.bFogEnabled )
        TurnFogOnOff(true);

    return res;
}

inline int ReverseCITableLookup(uint32_t *pTable, int size, uint32_t val)
{
    for( int i=0; i<size; i++)
    {
        if( pTable[i] == val )
            return i;
    }

    TRACE0("Cannot find value in CI table");
    return 0;
}

extern RenderTextureInfo gRenderTextureInfos[];
void SetVertexTextureUVCoord(TexCord &dst, const TexCord &src, int tile, TxtrCacheEntry *pEntry)
{
    RenderTexture &txtr = g_textures[tile];
    RenderTextureInfo &info = gRenderTextureInfos[pEntry->txtrBufIdx-1];
    float s = src.u;
    float t = src.v;

    uint32_t addrOffset = g_TI.dwAddr-info.CI_Info.dwAddr;
    uint32_t extraTop = (addrOffset>>(info.CI_Info.dwSize-1)) /info.CI_Info.dwWidth;
    uint32_t extraLeft = (addrOffset>>(info.CI_Info.dwSize-1))%info.CI_Info.dwWidth;

    if( pEntry->txtrBufIdx > 0  )
    {
        // Loading from render_texture or back buffer
        s += (extraLeft+pEntry->ti.LeftToLoad)/txtr.m_fTexWidth;
        t += (extraTop+pEntry->ti.TopToLoad)/txtr.m_fTexHeight;

        s *= info.scaleX;
        t *= info.scaleY;
    }

    dst.u = s;
    dst.v = t;
}

void CRender::SetVertexTextureUVCoord(TLITVERTEX &v, const TexCord &fTex0)
{
    RenderTexture &txtr = g_textures[0];
    if( txtr.pTextureEntry && txtr.pTextureEntry->txtrBufIdx > 0 )
    {
        ::SetVertexTextureUVCoord(v.tcord[0], fTex0, 0, txtr.pTextureEntry);
    }
    else
    {
        v.tcord[0] = fTex0;
    }
}

void CRender::SetVertexTextureUVCoord(TLITVERTEX &v, float fTex0S, float fTex0T)
{
    TexCord t = { fTex0S, fTex0T };
    SetVertexTextureUVCoord(v, t);
}

void CRender::SetVertexTextureUVCoord(TLITVERTEX &v, const TexCord &fTex0_, const TexCord &fTex1_)
{
    TexCord fTex0 = fTex0_;
    TexCord fTex1 = fTex1_;

    if( (options.enableHackForGames == HACK_FOR_ZELDA||options.enableHackForGames == HACK_FOR_ZELDA_MM) && m_Mux == 0x00262a60150c937fLL && gRSP.curTile == 0 )
    {
        // Hack for Zelda Sun
        gDPTile *t0 = &gDP.tiles[0];
        gDPTile *t1 = &gDP.tiles[1];
        if( t0->format == G_IM_FMT_I && t0->size == G_IM_SIZ_8b && t0->width == 64 &&
            t1->format == G_IM_FMT_I && t1->size == G_IM_SIZ_8b && t1->width == 64 &&
            t0->height == t1->height )
        {
            fTex0.u /= 2;
            fTex0.v /= 2;
            fTex1.u /= 2;
            fTex1.v /= 2;
        }
    }

    RenderTexture &txtr0 = g_textures[0];
    if( txtr0.pTextureEntry && txtr0.pTextureEntry->txtrBufIdx > 0 )
        ::SetVertexTextureUVCoord(v.tcord[0], fTex0, 0, txtr0.pTextureEntry);
    else
        v.tcord[0] = fTex0;

    RenderTexture &txtr1 = g_textures[1];
    if( txtr1.pTextureEntry && txtr1.pTextureEntry->txtrBufIdx > 0 )
        ::SetVertexTextureUVCoord(v.tcord[1], fTex1, 1, txtr1.pTextureEntry);
    else
        v.tcord[1] = fTex1;
}

void CRender::SetVertexTextureUVCoord(TLITVERTEX &v, float fTex0S, float fTex0T, float fTex1S, float fTex1T)
{
    TexCord t0 = { fTex0S, fTex0T };
    TexCord t1 = { fTex1S, fTex1T };
    SetVertexTextureUVCoord(v, t0, t1);
}

void CRender::SetClipRatio(uint32_t type, uint32_t w1)
{
   bool modified = false;
   switch(type)
   {
      case G_MWO_CLIP_RNX:
         if( gRSP.clip_ratio_negx != (short)w1 )
         {
            gRSP.clip_ratio_negx = (short)w1;
            modified = true;
         }
         break;
      case G_MWO_CLIP_RNY:
         if( gRSP.clip_ratio_negy != (short)w1 )
         {
            gRSP.clip_ratio_negy = (short)w1;
            modified = true;
         }
         break;
      case G_MWO_CLIP_RPX:
         if( gRSP.clip_ratio_posx != -(short)w1 )
         {
            gRSP.clip_ratio_posx = -(short)w1;
            modified = true;
         }
         break;
      case G_MWO_CLIP_RPY:
         if( gRSP.clip_ratio_posy != -(short)w1 )
         {
            gRSP.clip_ratio_posy = -(short)w1;
            modified = true;
         }
         break;
   }

   if( modified )
      UpdateClipRectangle();
}

void CRender::UpdateClipRectangle()
{
    if( status.bHandleN64RenderTexture )
    {
        //windowSetting.fMultX = windowSetting.fMultY = 1;
        windowSetting.vpLeftW = 0;
        windowSetting.vpTopW = 0;
        windowSetting.vpRightW = newRenderTextureInfo.bufferWidth;
        windowSetting.vpBottomW = newRenderTextureInfo.bufferHeight;
        windowSetting.vpWidthW = newRenderTextureInfo.bufferWidth;
        windowSetting.vpHeightW = newRenderTextureInfo.bufferHeight;

        gRSP.vtxXMul = windowSetting.vpWidthW/2.0f;
        gRSP.vtxXAdd = gRSP.vtxXMul + windowSetting.vpLeftW;
        gRSP.vtxYMul = -windowSetting.vpHeightW/2.0f;
        gRSP.vtxYAdd = windowSetting.vpHeightW/2.0f + windowSetting.vpTopW;

        // Update clip rectangle by setting scissor

        int halfx = newRenderTextureInfo.bufferWidth/2;
        int halfy = newRenderTextureInfo.bufferHeight/2;
        int centerx = halfx;
        int centery = halfy;

        gRSP.clip_ratio_left = centerx - halfx * gRSP.clip_ratio_negx;
        gRSP.clip_ratio_top = centery - halfy * gRSP.clip_ratio_negy;
        gRSP.clip_ratio_right = centerx + halfx * gRSP.clip_ratio_posx;
        gRSP.clip_ratio_bottom = centery + halfy * gRSP.clip_ratio_posy;
    }
    else
    {
        windowSetting.vpLeftW = int(gRSP.nVPLeftN * windowSetting.fMultX);
        windowSetting.vpTopW = int(gRSP.nVPTopN  * windowSetting.fMultY);
        windowSetting.vpRightW = int(gRSP.nVPRightN* windowSetting.fMultX);
        windowSetting.vpBottomW = int(gRSP.nVPBottomN* windowSetting.fMultY);
        windowSetting.vpWidthW = int((gRSP.nVPRightN - gRSP.nVPLeftN + 1) * windowSetting.fMultX);
        windowSetting.vpHeightW = int((gRSP.nVPBottomN - gRSP.nVPTopN + 1) * windowSetting.fMultY);

        gRSP.vtxXMul = windowSetting.vpWidthW/2.0f;
        gRSP.vtxXAdd = gRSP.vtxXMul + windowSetting.vpLeftW;
        gRSP.vtxYMul = -windowSetting.vpHeightW/2.0f;
        gRSP.vtxYAdd = windowSetting.vpHeightW/2.0f + windowSetting.vpTopW;

        // Update clip rectangle by setting scissor

        int halfx = gRSP.nVPWidthN/2;
        int halfy = gRSP.nVPHeightN/2;
        int centerx = gRSP.nVPLeftN+halfx;
        int centery = gRSP.nVPTopN+halfy;

        gRSP.clip_ratio_left = centerx - halfx * gRSP.clip_ratio_negx;
        gRSP.clip_ratio_top = centery - halfy * gRSP.clip_ratio_negy;
        gRSP.clip_ratio_right = centerx + halfx * gRSP.clip_ratio_posx;
        gRSP.clip_ratio_bottom = centery + halfy * gRSP.clip_ratio_posy;
    }

    UpdateScissorWithClipRatio();
}

void CRender::UpdateScissorWithClipRatio()
{
    gRSP.real_clip_scissor_left   = MAX(gRDP.scissor.left, gRSP.clip_ratio_left);
    gRSP.real_clip_scissor_top    = MAX(gRDP.scissor.top, gRSP.clip_ratio_top);
    gRSP.real_clip_scissor_right  = MIN(gRDP.scissor.right,gRSP.clip_ratio_right);
    gRSP.real_clip_scissor_bottom = MIN(gRDP.scissor.bottom, gRSP.clip_ratio_bottom);

    gRSP.real_clip_scissor_left   = MAX(gRSP.real_clip_scissor_left, 0);
    gRSP.real_clip_scissor_top    = MAX(gRSP.real_clip_scissor_top, 0);
    gRSP.real_clip_scissor_right  = MIN(gRSP.real_clip_scissor_right,windowSetting.uViWidth-1);
    gRSP.real_clip_scissor_bottom = MIN(gRSP.real_clip_scissor_bottom, windowSetting.uViHeight-1);

    WindowSettingStruct &w = windowSetting;
    w.clipping.left = (uint32_t)(gRSP.real_clip_scissor_left*windowSetting.fMultX);
    w.clipping.top  = (uint32_t)(gRSP.real_clip_scissor_top*windowSetting.fMultY);
    w.clipping.bottom = (uint32_t)(gRSP.real_clip_scissor_bottom*windowSetting.fMultY);
    w.clipping.right = (uint32_t)(gRSP.real_clip_scissor_right*windowSetting.fMultX);

    if( w.clipping.left > 0 || w.clipping.top > 0 || w.clipping.right < (uint32_t)windowSetting.uDisplayWidth-1 ||
        w.clipping.bottom < (uint32_t)windowSetting.uDisplayHeight-1 )
        w.clipping.needToClip = true;
    else
        w.clipping.needToClip = false;

    w.clipping.width = (uint32_t)((gRSP.real_clip_scissor_right-gRSP.real_clip_scissor_left+1)*windowSetting.fMultX);
    w.clipping.height = (uint32_t)((gRSP.real_clip_scissor_bottom-gRSP.real_clip_scissor_top+1)*windowSetting.fMultY);

    float halfx = gRSP.nVPWidthN/2.0f;
    float halfy = gRSP.nVPHeightN/2.0f;
    float centerx = gRSP.nVPLeftN+halfx;
    float centery = gRSP.nVPTopN+halfy;

    gRSP.real_clip_ratio_negx = (gRSP.real_clip_scissor_left - centerx)/halfx;
    gRSP.real_clip_ratio_negy = (gRSP.real_clip_scissor_top - centery)/halfy;
    gRSP.real_clip_ratio_posx = (gRSP.real_clip_scissor_right - centerx)/halfx;
    gRSP.real_clip_ratio_posy = (gRSP.real_clip_scissor_bottom - centery)/halfy;

    ApplyScissorWithClipRatio(true);
}


// Set other modes not covered by color combiner or alpha blender
void CRender::InitOtherModes(void)
{
    ApplyTextureFilter();

    //
    // I can't think why the hand in Mario's menu screen is rendered with an opaque rendermode,
    // and no alpha threshold. We set the alpha reference to 1 to ensure that the transparent pixels
    // don't get rendered. I hope this doesn't fuck anything else up.
    //
    if ( gRDP.otherMode.alpha_compare == 0 )
    {
        if ( gRDP.otherMode.cvg_x_alpha && (gRDP.otherMode.alpha_cvg_sel || gRDP.otherMode.aa_en ) )
        {
            ForceAlphaRef(128); // Strange, I have to use value=2 for pixel shader combiner for Nvidia FX5200
                                // for other video cards, value=1 is good enough.
            SetAlphaTestEnable(true);
        }
        else
            SetAlphaTestEnable(false);
    }
    else if ( gRDP.otherMode.alpha_compare == 3 )
    {
        //RDP_ALPHA_COMPARE_DITHER
        SetAlphaTestEnable(false);
    }
    else
    {
       // Use CVG for pixel alpha
        if( (gRDP.otherMode.alpha_cvg_sel ) && !gRDP.otherMode.cvg_x_alpha )
            SetAlphaTestEnable(false);
        else
        {
            // RDP_ALPHA_COMPARE_THRESHOLD || RDP_ALPHA_COMPARE_DITHER
            if( m_dwAlpha==0 )
                ForceAlphaRef(1);
            else
                ForceAlphaRef(m_dwAlpha);
            SetAlphaTestEnable(true);
        }
    }

    if( options.enableHackForGames == HACK_FOR_SOUTH_PARK_RALLY && m_Mux == 0x00121824ff33ffffLL &&
        gRSP.bCullFront && gRDP.otherMode.aa_en && gRDP.otherMode.z_cmp && gRDP.otherMode.z_upd )
    {
        SetZCompare(false);
    }


    if( gRDP.otherMode.cycle_type  >= G_CYC_COPY )
    {
        // Disable zbuffer for COPY and FILL mode
        SetZCompare(false);
    }
    else
    {
        SetZCompare(gRDP.otherMode.z_cmp);
        SetZUpdate(gRDP.otherMode.z_upd);
    }

    /*
    if( options.enableHackForGames == HACK_FOR_SOUTH_PARK_RALLY && m_Mux == 0x00121824ff33ffff &&
        gRSP.bCullFront && gRDP.otherMode.z_cmp && gRDP.otherMode.z_upd )//&& gRDP.otherMode.aa_en )
    {
        SetZCompare(false);
        SetZUpdate(false);
    }
    */
}


void CRender::SetTextureFilter(uint32_t dwFilter)
{
    if( options.forceTextureFilter == FORCE_DEFAULT_FILTER )
    {
        switch(dwFilter)
        {
            case RDP_TFILTER_AVERAGE:   //?
            case RDP_TFILTER_BILERP:
                m_dwMinFilter = m_dwMagFilter = FILTER_LINEAR;
                break;
            default:
                m_dwMinFilter = m_dwMagFilter = FILTER_POINT;
                break;
        }
    }
    else
    {
        switch( options.forceTextureFilter )
        {
        case FORCE_POINT_FILTER:
            m_dwMinFilter = m_dwMagFilter = FILTER_POINT;
            break;
        case FORCE_LINEAR_FILTER:
            m_dwMinFilter = m_dwMagFilter = FILTER_LINEAR;
            break;
        }
    }

    ApplyTextureFilter();
}
