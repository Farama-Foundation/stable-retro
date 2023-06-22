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

#include <stdlib.h>
#include "osal_opengl.h"

#include "DeviceBuilder.h"
#include "FrameBuffer.h"
#include "OGLCombiner.h"
#include "OGLExtRender.h"
#include "OGLGraphicsContext.h"
#include "OGLTexture.h"
#include "OGLES2FragmentShaders.h"

//========================================================================
CDeviceBuilder* CDeviceBuilder::m_pInstance=NULL;
SupportedDeviceType CDeviceBuilder::m_deviceType = DIRECTX_DEVICE;
SupportedDeviceType CDeviceBuilder::m_deviceGeneralType = DIRECTX_DEVICE;

CDeviceBuilder* CDeviceBuilder::GetBuilder(void)
{
    if( m_pInstance == NULL )
        CreateBuilder(m_deviceType);
    
    return m_pInstance;
}

void CDeviceBuilder::SelectDeviceType(SupportedDeviceType type)
{
    if( type != m_deviceType && m_pInstance != NULL )
        DeleteBuilder();

    CDeviceBuilder::m_deviceType        = type;
    CDeviceBuilder::m_deviceGeneralType = OGL_DEVICE;
}

SupportedDeviceType CDeviceBuilder::GetDeviceType(void)
{
    return CDeviceBuilder::m_deviceType;
}

SupportedDeviceType CDeviceBuilder::GetGeneralDeviceType(void)
{
    return CDeviceBuilder::m_deviceGeneralType;
}

CDeviceBuilder* CDeviceBuilder::CreateBuilder(SupportedDeviceType type)
{
    if( m_pInstance == NULL )
       m_pInstance = new OGLDeviceBuilder();

    return m_pInstance;
}

void CDeviceBuilder::DeleteBuilder(void)
{
    delete m_pInstance;
    m_pInstance = NULL;
}

CDeviceBuilder::CDeviceBuilder() :
    m_pRender(NULL),
    m_pGraphicsContext(NULL),
    m_pColorCombiner(NULL),
    m_pAlphaBlender(NULL)
{
}

CDeviceBuilder::~CDeviceBuilder()
{
    DeleteGraphicsContext();
    DeleteRender();
    DeleteColorCombiner();
    DeleteAlphaBlender();
}

void CDeviceBuilder::DeleteGraphicsContext(void)
{
    if( m_pGraphicsContext != NULL )
    {
        delete m_pGraphicsContext;
        CGraphicsContext::g_pGraphicsContext = m_pGraphicsContext = NULL;
    }

    if (g_pFrameBufferManager)
       free(g_pFrameBufferManager);
    g_pFrameBufferManager = NULL;
}

void CDeviceBuilder::DeleteRender(void)
{
    if( m_pRender != NULL )
    {
        delete m_pRender;
        CRender::g_pRender = m_pRender = NULL;
        CRender::gRenderReferenceCount = 0;
    }
}

void CDeviceBuilder::DeleteColorCombiner(void)
{
    if( m_pColorCombiner != NULL )
    {
        delete m_pColorCombiner;
        m_pColorCombiner = NULL;
    }
}

void CDeviceBuilder::DeleteAlphaBlender(void)
{
    if( m_pAlphaBlender != NULL )
    {
        delete m_pAlphaBlender;
        m_pAlphaBlender = NULL;
    }
}


//========================================================================

CGraphicsContext * OGLDeviceBuilder::CreateGraphicsContext(void)
{
    if( m_pGraphicsContext == NULL )
    {
        m_pGraphicsContext = new COGLGraphicsContext();
        CGraphicsContext::g_pGraphicsContext = m_pGraphicsContext;
    }

    g_pFrameBufferManager = new FrameBufferManager;
    return m_pGraphicsContext;
}

CRender * OGLDeviceBuilder::CreateRender(void)
{
    if( m_pRender == NULL )
    {
        COGLGraphicsContext &context = *((COGLGraphicsContext*)CGraphicsContext::g_pGraphicsContext);
        m_pRender = new COGLExtRender();

        CRender::g_pRender = m_pRender;
    }

    return m_pRender;
}

CTexture * OGLDeviceBuilder::CreateTexture(uint32_t dwWidth, uint32_t dwHeight, TextureUsage usage)
{
    COGLTexture *txtr = new COGLTexture(dwWidth, dwHeight, usage);
    if( txtr->m_pTexture == NULL )
    {
        delete txtr;
        TRACE0("Cannot create new texture, out of video memory");
        return NULL;
    }

    return txtr;
}

CColorCombiner * OGLDeviceBuilder::CreateColorCombiner(CRender *pRender)
{
    if( m_pColorCombiner == NULL )
    {
       m_deviceType = (SupportedDeviceType)options.OpenglRenderSetting;

       m_pColorCombiner = new COGL_FragmentProgramCombiner(pRender);
       DebugMessage(M64MSG_VERBOSE, "OpenGL Combiner: Fragment Program");
    }

    return m_pColorCombiner;
}

CBlender * OGLDeviceBuilder::CreateAlphaBlender(CRender *pRender)
{
    if( m_pAlphaBlender == NULL )
        m_pAlphaBlender = new COGLBlender(pRender);

    return m_pAlphaBlender;
}

