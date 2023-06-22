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

#ifndef _OGL_CONTEXT_H_
#define _OGL_CONTEXT_H_

#include "typedefs.h"
#include "GraphicsContext.h"

class COGLGraphicsContext : public CGraphicsContext
{
    friend class OGLRender;
    friend class COGLRenderTexture;
public:
    virtual ~COGLGraphicsContext();

    bool Initialize(uint32_t dwWidth, uint32_t dwHeight);
    bool ResizeInitialize(uint32_t dwWidth, uint32_t dwHeight);
    void CleanUp();
    void Clear(ClearFlag dwFlags, uint32_t color, float depth);

    void UpdateFrame(bool swapOnly);

    bool IsExtensionSupported(const char* pExtName);
    static void InitDeviceParameters();

protected:
    friend class OGLDeviceBuilder;
    COGLGraphicsContext();
    void InitState(void);
    void InitOGLExtension(void);

    // Optional OGL extension features;
    bool    m_bSupportLODBias;

    // Nvidia OGL only features
    bool    m_bSupportTextureLOD;
    
    const unsigned char*    m_pVendorStr;
    const unsigned char*    m_pRenderStr;
    const unsigned char*    m_pExtensionStr;
    const char* m_pWglExtensionStr;
    const unsigned char*    m_pVersionStr;
};

#endif



