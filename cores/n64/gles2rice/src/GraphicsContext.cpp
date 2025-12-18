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
#include "m64p_plugin.h"
#include "m64p_vidext.h"

#include "FrameBuffer.h"
#include "OGLGraphicsContext.h"
#include "Video.h"

CGraphicsContext* CGraphicsContext::g_pGraphicsContext = NULL;
bool CGraphicsContext::needCleanScene = false;

CGraphicsContext * CGraphicsContext::Get(void)
{   
    return CGraphicsContext::g_pGraphicsContext;
}
    
CGraphicsContext::CGraphicsContext()
{
}
CGraphicsContext::~CGraphicsContext()
{
    g_pFrameBufferManager->CloseUp();
}

void CGraphicsContext::InitWindowInfo()
{
}

bool CGraphicsContext::Initialize(uint32_t dwWidth, uint32_t dwHeight)
{
    g_pFrameBufferManager->Initialize();
    return true;
}

bool CGraphicsContext::ResizeInitialize(uint32_t dwWidth, uint32_t dwHeight )
{
    return true;
}

void CGraphicsContext::CleanUp()
{
}

// This is a static function, will be called when the plugin DLL is initialized
void CGraphicsContext::InitDeviceParameters(void)
{
    // To initialize device parameters for OpenGL
    COGLGraphicsContext::InitDeviceParameters();
}

