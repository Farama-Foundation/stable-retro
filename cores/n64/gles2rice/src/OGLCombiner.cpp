/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *   Mupen64plus - OGLCombiner.cpp                                         *
 *   Mupen64Plus homepage: http://code.google.com/p/mupen64plus/           *
 *   Copyright (C) 2003 Rice1964                                           *
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

#include "osal_opengl.h"

#include "OGLCombiner.h"
#include "OGLRender.h"
#include "OGLGraphicsContext.h"
#include "OGLDecodedMux.h"
#include "OGLTexture.h"

//========================================================================
uint32_t DirectX_OGL_BlendFuncMaps [] =
{
    GL_SRC_ALPHA,               //Nothing
    GL_ZERO,                    //BLEND_ZERO               = 1,
    GL_ONE,                     //BLEND_ONE                = 2,
    GL_SRC_COLOR,               //BLEND_SRCCOLOR           = 3,
    GL_ONE_MINUS_SRC_COLOR,     //BLEND_INVSRCCOLOR        = 4,
    GL_SRC_ALPHA,               //BLEND_SRCALPHA           = 5,
    GL_ONE_MINUS_SRC_ALPHA,     //BLEND_INVSRCALPHA        = 6,
    GL_DST_ALPHA,               //BLEND_DESTALPHA          = 7,
    GL_ONE_MINUS_DST_ALPHA,     //BLEND_INVDESTALPHA       = 8,
    GL_DST_COLOR,               //BLEND_DESTCOLOR          = 9,
    GL_ONE_MINUS_DST_COLOR,     //BLEND_INVDESTCOLOR       = 10,
    GL_SRC_ALPHA_SATURATE,      //BLEND_SRCALPHASAT        = 11,
    GL_SRC_ALPHA_SATURATE,      //BLEND_BOTHSRCALPHA       = 12,    
    GL_SRC_ALPHA_SATURATE,      //BLEND_BOTHINVSRCALPHA    = 13,
};

//========================================================================
COGLColorCombiner::COGLColorCombiner(CRender *pRender) :
    CColorCombiner(pRender),
    m_pOGLRender((OGLRender*)pRender),
    m_bSupportAdd(false), m_bSupportSubtract(false)
{
    m_pDecodedMux = new COGLDecodedMux;
    m_pDecodedMux->m_maxConstants = 0;
    m_pDecodedMux->m_maxTextures = 1;
}

COGLColorCombiner::~COGLColorCombiner()
{
    delete m_pDecodedMux;
    m_pDecodedMux = NULL;
}

bool COGLColorCombiner::Initialize(void)
{
    m_bSupportAdd = false;
    m_bSupportSubtract = false;
    m_supportedStages = 1;

    COGLGraphicsContext *pcontext = (COGLGraphicsContext *)(CGraphicsContext::g_pGraphicsContext);
    if( pcontext->IsExtensionSupported(OSAL_GL_ARB_TEXTURE_ENV_ADD) || pcontext->IsExtensionSupported("GL_EXT_texture_env_add") )
        m_bSupportAdd = true;

    if( pcontext->IsExtensionSupported("GL_EXT_blend_subtract") )
        m_bSupportSubtract = true;

    return true;
}

void COGLColorCombiner::DisableCombiner(void)
{
    m_pOGLRender->DisableMultiTexture();
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ZERO);
    
    if( m_bTexelsEnable )
    {
        COGLTexture* pTexture = g_textures[gRSP.curTile].m_pCOGLTexture;
        if( pTexture ) 
        {
            m_pOGLRender->EnableTexUnit(0, true);
            m_pOGLRender->BindTexture(pTexture->m_dwTextureName, 0);
            glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
            m_pOGLRender->SetAllTexelRepeatFlag();
        }
    }
    else
    {
        glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
        m_pOGLRender->EnableTexUnit(0, false);
    }
}

void COGLColorCombiner::InitCombinerCycleCopy(void)
{
    m_pOGLRender->DisableMultiTexture();
    m_pOGLRender->EnableTexUnit(0, true);
    COGLTexture* pTexture = g_textures[gRSP.curTile].m_pCOGLTexture;
    if( pTexture )
    {
        m_pOGLRender->BindTexture(pTexture->m_dwTextureName, 0);
        m_pOGLRender->SetTexelRepeatFlags(gRSP.curTile);
    }

    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
}

void COGLColorCombiner::InitCombinerCycleFill(void)
{
    m_pOGLRender->DisableMultiTexture();
    m_pOGLRender->EnableTexUnit(0, false);
}


void COGLColorCombiner::InitCombinerCycle12(void)
{
    m_pOGLRender->DisableMultiTexture();
    if (!m_bTexelsEnable)
    {
        glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
        m_pOGLRender->EnableTexUnit(0, false);
        return;
    }
}

void COGLBlender::NormalAlphaBlender(void)
{
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void COGLBlender::DisableAlphaBlender(void)
{
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ZERO);
}


void COGLBlender::BlendFunc(uint32_t srcFunc, uint32_t desFunc)
{
    glBlendFunc(DirectX_OGL_BlendFuncMaps[srcFunc], DirectX_OGL_BlendFuncMaps[desFunc]);
}

void COGLBlender::Enable()
{
    glEnable(GL_BLEND);
}

void COGLBlender::Disable()
{
    glDisable(GL_BLEND);
}

void COGLColorCombiner::InitCombinerBlenderForSimpleTextureDraw(uint32_t tile)
{
    m_pOGLRender->DisableMultiTexture();
    if( g_textures[tile].m_pCTexture )
    {
        m_pOGLRender->EnableTexUnit(0, true);
        glBindTexture(GL_TEXTURE_2D, ((COGLTexture*)(g_textures[tile].m_pCTexture))->m_dwTextureName);
    }
    m_pOGLRender->SetAllTexelRepeatFlag();

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR); // Linear Filtering
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR); // Linear Filtering

    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
    m_pOGLRender->SetAlphaTestEnable(false);
}

#ifdef DEBUGGER
extern const char *translatedCombTypes[];
void COGLColorCombiner::DisplaySimpleMuxString(void)
{
    TRACE0("\nSimplified Mux\n");
    m_pDecodedMux->DisplaySimpliedMuxString("Used");
}
#endif

