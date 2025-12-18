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

#ifndef GFXCONTEXT_H
#define GFXCONTEXT_H

#include "typedefs.h"

enum ClearFlag
{
    CLEAR_COLOR_BUFFER           = 0x01,
    CLEAR_DEPTH_BUFFER           = 0x02,
    CLEAR_COLOR_AND_DEPTH_BUFFER = 0x03,
};


typedef struct
{
    uint32_t  addr;   //N64 RDRAM address
    uint32_t  size;   //N64 buffer size
    uint32_t  format; //N64 format
    uint32_t  width;
    uint32_t  height;
} TextureBufferShortInfo;


// This class basically provides an extra level of security for our
// multi-threaded code. Threads can Grab the CGraphicsContext to prevent
// other threads from changing/releasing any of the pointers while it is
// running.

class CGraphicsContext
{
    friend class CDeviceBuilder;
    
public:
    virtual bool Initialize(uint32_t dwWidth, uint32_t dwHeight);
    virtual bool ResizeInitialize(uint32_t dwWidth, uint32_t dwHeight);
    virtual void CleanUp();

    virtual void Clear(ClearFlag flags, uint32_t color, float depth) = 0;
    virtual void UpdateFrame(bool swapOnly) = 0;

    static void InitWindowInfo();
    static void InitDeviceParameters();

public:
protected:
    char                m_strDeviceStats[256];

    virtual ~CGraphicsContext();
    CGraphicsContext();
    
public:
    static CGraphicsContext *g_pGraphicsContext;
    static CGraphicsContext * Get(void);
    inline const char* GetDeviceStr() {return m_strDeviceStats;}
    static bool needCleanScene;
};

#endif

