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

#include "osal_opengl.h"

#define M64P_PLUGIN_PROTOTYPES 1
#include "m64p_plugin.h"
#include "Config.h"
#include "Debugger.h"
#include "OGLGraphicsContext.h"
#include "TextureManager.h"
#include "Video.h"
#include "version.h"
#include "../../libretro/libretro_private.h"

COGLGraphicsContext::COGLGraphicsContext() :
    m_bSupportLODBias(false),
    m_bSupportTextureLOD(false),
    m_pVendorStr(NULL),
    m_pRenderStr(NULL),
    m_pExtensionStr(NULL),
    m_pVersionStr(NULL)
{
}


COGLGraphicsContext::~COGLGraphicsContext()
{
}

bool COGLGraphicsContext::Initialize(uint32_t dwWidth, uint32_t dwHeight)
{
    DebugMessage(M64MSG_INFO, "Initializing OpenGL Device Context.");
    CGraphicsContext::Initialize(dwWidth, dwHeight);

    int depthBufferDepth = options.OpenglDepthBufferSetting;
    int colorBufferDepth = 32;
    int bVerticalSync = windowSetting.bVerticalSync;

    if (options.colorQuality == TEXTURE_FMT_A4R4G4B4)
        colorBufferDepth = 16;

    InitState();
    InitOGLExtension();
    sprintf(m_strDeviceStats, "%.60s - %.128s : %.60s", m_pVendorStr, m_pRenderStr, m_pVersionStr);
    TRACE0(m_strDeviceStats);
    DebugMessage(M64MSG_INFO, "Using OpenGL: %s", m_strDeviceStats);

    Clear(CLEAR_COLOR_AND_DEPTH_BUFFER, 0xFF000000, 1.0f);    // Clear buffers
    UpdateFrame(false);
    Clear(CLEAR_COLOR_AND_DEPTH_BUFFER, 0xFF000000, 1.0f);
    UpdateFrame(false);
    
    return true;
}

bool COGLGraphicsContext::ResizeInitialize(uint32_t dwWidth, uint32_t dwHeight)
{
    CGraphicsContext::Initialize(dwWidth, dwHeight);

    int depthBufferDepth = options.OpenglDepthBufferSetting;
    int colorBufferDepth = 32;
    int bVerticalSync = windowSetting.bVerticalSync;

    if (options.colorQuality == TEXTURE_FMT_A4R4G4B4)
        colorBufferDepth = 16;

    InitState();

    Clear(CLEAR_COLOR_AND_DEPTH_BUFFER, 0xFF000000, 1.0f);    // Clear buffers
    UpdateFrame(false);
    Clear(CLEAR_COLOR_AND_DEPTH_BUFFER, 0xFF000000, 1.0f);
    UpdateFrame(false);
    
    return true;
}

void COGLGraphicsContext::InitState(void)
{
    m_pRenderStr = glGetString(GL_RENDERER);
    m_pExtensionStr = glGetString(GL_EXTENSIONS);
    m_pVersionStr = glGetString(GL_VERSION);
    m_pVendorStr = glGetString(GL_VENDOR);

    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClearDepth(1.0f);

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_BLEND);

    glFrontFace(GL_CCW);
    glDisable(GL_CULL_FACE);

    glDepthFunc(GL_LEQUAL);
    glEnable(GL_DEPTH_TEST);

    glEnable(GL_BLEND);
    glDepthRange(-1.0f, 1.0f);
}

void COGLGraphicsContext::InitOGLExtension(void)
{
}

bool COGLGraphicsContext::IsExtensionSupported(const char* pExtName)
{
   if (strstr((const char*)m_pExtensionStr, pExtName) != NULL)
   {
      DebugMessage(M64MSG_VERBOSE, "OpenGL Extension '%s' is supported.", pExtName);
      return true;
   }

   DebugMessage(M64MSG_VERBOSE, "OpenGL Extension '%s' is NOT supported.", pExtName);
   return false;
}

void COGLGraphicsContext::CleanUp()
{
}


void COGLGraphicsContext::Clear(ClearFlag dwFlags, uint32_t color, float depth)
{
    uint32_t flag = 0;
    float r       = ((color>>16)&0xFF)/255.0f;
    float g       = ((color>> 8)&0xFF)/255.0f;
    float b       = ((color    )&0xFF)/255.0f;
    float a       = ((color>>24)&0xFF)/255.0f;

    if (dwFlags & CLEAR_COLOR_BUFFER)
       flag |= GL_COLOR_BUFFER_BIT;
    if (dwFlags & CLEAR_DEPTH_BUFFER)
       flag |= GL_DEPTH_BUFFER_BIT;

    glClearColor(r, g, b, a);
    glClearDepth(depth);
    glClear(flag);  //Clear color buffer and depth buffer
}

void COGLGraphicsContext::UpdateFrame(bool swapOnly)
{
    status.gFrameCount++;

    glFlush();

    // if emulator defined a render callback function, call it before buffer swap
    if (renderCallback)
        (*renderCallback)(status.bScreenIsDrawn);

   retro_return(true);
   
    glDepthMask(GL_TRUE);
    glClearDepth(1.0f);
    if (!g_curRomInfo.bForceScreenClear)
        glClear(GL_DEPTH_BUFFER_BIT);
    else
        needCleanScene = true;

    status.bScreenIsDrawn = false;
}

// This is a static function, will be called when the plugin DLL is initialized
void COGLGraphicsContext::InitDeviceParameters()
{
}
