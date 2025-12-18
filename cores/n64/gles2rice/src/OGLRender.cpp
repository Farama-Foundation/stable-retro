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

#include "OGLES2FragmentShaders.h"
#include "OGLRender.h"
#include "OGLGraphicsContext.h"
#include "OGLTexture.h"
#include "TextureManager.h"

// FIXME: Use OGL internal L/T and matrix stack
// FIXME: Use OGL lookupAt function
// FIXME: Use OGL DisplayList

UVFlagMap OGLXUVFlagMaps[] =
{
    {TEXTURE_UV_FLAG_WRAP, GL_REPEAT},
    {TEXTURE_UV_FLAG_MIRROR, GL_MIRRORED_REPEAT},
    {TEXTURE_UV_FLAG_CLAMP, GL_CLAMP},
};

//===================================================================
OGLRender::OGLRender()
{
    m_bSupportClampToEdge = false;
    for( int i=0; i<8; i++ )
    {
        m_curBoundTex[i]=0;
        m_texUnitEnabled[i]= false;
    }

    m_bEnableMultiTexture = false;
}

OGLRender::~OGLRender()
{
    ClearDeviceObjects();
}

bool OGLRender::InitDeviceObjects()
{
    // enable Z-buffer by default
    ZBufferEnable(true);
    return true;
}

bool OGLRender::ClearDeviceObjects()
{
    return true;
}

void OGLRender::Initialize(void)
{
    glViewportWrapper(0, 0, windowSetting.uDisplayWidth, windowSetting.uDisplayHeight, true);

    OGLXUVFlagMaps[TEXTURE_UV_FLAG_MIRROR].realFlag = GL_MIRRORED_REPEAT;
    m_bSupportClampToEdge = true;
    OGLXUVFlagMaps[TEXTURE_UV_FLAG_CLAMP].realFlag = GL_CLAMP_TO_EDGE;

    glVertexAttribPointer(VS_POSITION,4,GL_FLOAT,GL_FALSE,sizeof(float)*5,&(g_vtxProjected5[0][0]));


    glVertexAttribPointer(VS_TEXCOORD0,2,GL_FLOAT,GL_FALSE, sizeof( TLITVERTEX ), &(g_vtxBuffer[0].tcord[0].u));
    glVertexAttribPointer(VS_TEXCOORD1,2,GL_FLOAT,GL_FALSE, sizeof( TLITVERTEX ), &(g_vtxBuffer[0].tcord[1].u));

    glVertexAttribPointer(VS_FOG,1,GL_FLOAT,GL_FALSE,sizeof(float)*5,&(g_vtxProjected5[0][4]));

    glVertexAttribPointer(VS_COLOR, 4, GL_UNSIGNED_BYTE,GL_TRUE, sizeof(uint8_t)*4, &(g_oglVtxColors[0][0]) );
}
//===================================================================
TextureFilterMap OglTexFilterMap[2]=
{
    {FILTER_POINT, GL_NEAREST},
    {FILTER_LINEAR, GL_LINEAR},
};

void OGLRender::ApplyTextureFilter()
{
    static uint32_t minflag=0xFFFF, magflag=0xFFFF;
    static uint32_t mtex;

    if( m_texUnitEnabled[0] )
    {
        if( mtex != m_curBoundTex[0] )
        {
            mtex = m_curBoundTex[0];
            minflag = m_dwMinFilter;
            magflag = m_dwMagFilter;
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, OglTexFilterMap[m_dwMinFilter].realFilter);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, OglTexFilterMap[m_dwMagFilter].realFilter);
        }
        else
        {
            if( minflag != (unsigned int)m_dwMinFilter )
            {
                minflag = m_dwMinFilter;
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, OglTexFilterMap[m_dwMinFilter].realFilter);
            }
            if( magflag != (unsigned int)m_dwMagFilter )
            {
                magflag = m_dwMagFilter;
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, OglTexFilterMap[m_dwMagFilter].realFilter);
            }   
        }
    }
}

void OGLRender::SetShadeMode(RenderShadeMode mode)
{
}

void OGLRender::ZBufferEnable(bool bZBuffer)
{
    gRSP.bZBufferEnabled = bZBuffer;
    if( g_curRomInfo.bForceDepthBuffer )
        bZBuffer = true;
    if( bZBuffer )
    {
        glDepthMask(GL_TRUE);
        //glEnable(GL_DEPTH_TEST);
        glDepthFunc( GL_LEQUAL );
    }
    else
    {
        glDepthMask(GL_FALSE);
        //glDisable(GL_DEPTH_TEST);
        glDepthFunc( GL_ALWAYS );
    }
}

void OGLRender::ClearBuffer(bool cbuffer, bool zbuffer)
{
    uint32_t flag = 0;
    float depth   = ((gRDP.originalFillColor&0xFFFF)>>2)/(float)0x3FFF;

    if( cbuffer )
       flag |= GL_COLOR_BUFFER_BIT;
    if( zbuffer )
       flag |= GL_DEPTH_BUFFER_BIT;
    glClearDepth(depth);
    glClear(flag);
}

void OGLRender::ClearZBuffer(float depth)
{
    uint32_t flag=GL_DEPTH_BUFFER_BIT;
    glClearDepth(depth);
    glClear(flag);
}

void OGLRender::SetZCompare(bool bZCompare)
{
    if( g_curRomInfo.bForceDepthBuffer )
        bZCompare = true;

    gRSP.bZBufferEnabled = bZCompare;
    if( bZCompare == true )
    {
        //glEnable(GL_DEPTH_TEST);
        glDepthFunc( GL_LEQUAL );
    }
    else
    {
        //glDisable(GL_DEPTH_TEST);
        glDepthFunc( GL_ALWAYS );
    }
}

void OGLRender::SetZUpdate(bool bZUpdate)
{
    if( g_curRomInfo.bForceDepthBuffer )
        bZUpdate = true;

    if( bZUpdate )
    {
        //glEnable(GL_DEPTH_TEST);
        glDepthMask(GL_TRUE);
    }
    else
        glDepthMask(GL_FALSE);
}

void OGLRender::ApplyZBias(int bias)
{
    static int old_bias;
    float f1 = bias > 0 ? -3.0f : 0.0f;  // z offset = -3.0 * max(abs(dz/dx),abs(dz/dy)) per pixel delta z slope
    float f2 = bias > 0 ? -3.0f : 0.0f;  // z offset += -3.0 * 1 bit

    if (bias == old_bias)
        return;
    old_bias = bias;

    if (bias > 0)
    {
        glEnable(GL_POLYGON_OFFSET_FILL);  // enable z offsets
    }
    else
    {
        glDisable(GL_POLYGON_OFFSET_FILL);  // disable z offsets
    }
    glPolygonOffset(f1, f2);  // set bias functions
}

void OGLRender::SetZBias(int bias)
{
#if defined(DEBUGGER)
    if( pauseAtNext == true )
      DebuggerAppendMsg("Set zbias = %d", bias);
#endif
    // set member variable and apply the setting in opengl
    m_dwZBias = bias;
    ApplyZBias(bias);
}

void OGLRender::SetAlphaRef(uint32_t dwAlpha)
{
    if (m_dwAlpha != dwAlpha)
    {
        m_dwAlpha = dwAlpha;
    }
}

void OGLRender::ForceAlphaRef(uint32_t dwAlpha)
{
    m_dwAlpha = dwAlpha;
}

void OGLRender::SetFillMode(FillMode mode)
{
}

void OGLRender::SetCullMode(bool bCullFront, bool bCullBack)
{
    CRender::SetCullMode(bCullFront, bCullBack);
    if( bCullFront && bCullBack )
    {
        glCullFace(GL_FRONT_AND_BACK);
        glEnable(GL_CULL_FACE);
    }
    else if( bCullFront )
    {
        glCullFace(GL_FRONT);
        glEnable(GL_CULL_FACE);
    }
    else if( bCullBack )
    {
        glCullFace(GL_BACK);
        glEnable(GL_CULL_FACE);
    }
    else
    {
        glDisable(GL_CULL_FACE);
    }
}

bool OGLRender::SetCurrentTexture(int tile, CTexture *handler,uint32_t dwTileWidth, uint32_t dwTileHeight, TxtrCacheEntry *pTextureEntry)
{
    RenderTexture &texture = g_textures[tile];
    texture.pTextureEntry = pTextureEntry;

    if( handler!= NULL  && texture.m_lpsTexturePtr != handler->GetTexture() )
    {
        texture.m_pCTexture = handler;
        texture.m_lpsTexturePtr = handler->GetTexture();

        texture.m_dwTileWidth = dwTileWidth;
        texture.m_dwTileHeight = dwTileHeight;

        if( handler->m_bIsEnhancedTexture )
        {
            texture.m_fTexWidth = (float)pTextureEntry->pTexture->m_dwCreatedTextureWidth;
            texture.m_fTexHeight = (float)pTextureEntry->pTexture->m_dwCreatedTextureHeight;
        }
        else
        {
            texture.m_fTexWidth = (float)handler->m_dwCreatedTextureWidth;
            texture.m_fTexHeight = (float)handler->m_dwCreatedTextureHeight;
        }
    }
    
    return true;
}

bool OGLRender::SetCurrentTexture(int tile, TxtrCacheEntry *pEntry)
{
    if (pEntry != NULL && pEntry->pTexture != NULL)
    {   
        SetCurrentTexture( tile, pEntry->pTexture,  pEntry->ti.WidthToCreate, pEntry->ti.HeightToCreate, pEntry);
        return true;
    }
    else
    {
        SetCurrentTexture( tile, NULL, 64, 64, NULL );
        return false;
    }
    return true;
}

void OGLRender::SetAddressUAllStages(uint32_t dwTile, TextureUVFlag dwFlag)
{
    SetTextureUFlag(dwFlag, dwTile);
}

void OGLRender::SetAddressVAllStages(uint32_t dwTile, TextureUVFlag dwFlag)
{
    SetTextureVFlag(dwFlag, dwTile);
}

void OGLRender::SetTexWrapS(int unitno,GLuint flag)
{
    static GLuint mflag;
    static GLuint mtex;
#ifdef DEBUGGER
    if( unitno != 0 )
    {
        DebuggerAppendMsg("Check me, unitno != 0 in base ogl");
    }
#endif
    if( m_curBoundTex[0] != mtex || mflag != flag )
    {
        mtex = m_curBoundTex[0];
        mflag = flag;
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, flag);
    }
}
void OGLRender::SetTexWrapT(int unitno,GLuint flag)
{
    static GLuint mflag;
    static GLuint mtex;
    if( m_curBoundTex[0] != mtex || mflag != flag )
    {
        mtex = m_curBoundTex[0];
        mflag = flag;
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, flag);
    }
}

void OGLRender::SetTextureUFlag(TextureUVFlag dwFlag, uint32_t dwTile)
{
    TileUFlags[dwTile] = dwFlag;
    if( dwTile == gRSP.curTile )    // For basic OGL, only support the 1st texel
    {
        COGLTexture* pTexture = g_textures[gRSP.curTile].m_pCOGLTexture;
        if( pTexture )
        {
            EnableTexUnit(0, true);
            BindTexture(pTexture->m_dwTextureName, 0);
        }
        SetTexWrapS(0, OGLXUVFlagMaps[dwFlag].realFlag);
    }
}
void OGLRender::SetTextureVFlag(TextureUVFlag dwFlag, uint32_t dwTile)
{
    TileVFlags[dwTile] = dwFlag;
    if( dwTile == gRSP.curTile )    // For basic OGL, only support the 1st texel
    {
        COGLTexture* pTexture = g_textures[gRSP.curTile].m_pCOGLTexture;
        if( pTexture ) 
        {
            EnableTexUnit(0, true);
            BindTexture(pTexture->m_dwTextureName, 0);
        }
        SetTexWrapT(0, OGLXUVFlagMaps[dwFlag].realFlag);
    }
}

// Basic render drawing functions

bool OGLRender::RenderTexRect()
{
    glViewportWrapper(0, 0, windowSetting.uDisplayWidth, windowSetting.uDisplayHeight, true);

    GLboolean cullface = glIsEnabled(GL_CULL_FACE);
    glDisable(GL_CULL_FACE);

    float depth = -(g_texRectTVtx[3].z*2-1);

    GLfloat colour[] = {
            (GLfloat)g_texRectTVtx[3].r, (GLfloat)g_texRectTVtx[3].g, (GLfloat)g_texRectTVtx[3].b, (GLfloat)g_texRectTVtx[3].a,
            (GLfloat)g_texRectTVtx[2].r, (GLfloat)g_texRectTVtx[2].g, (GLfloat)g_texRectTVtx[2].b, (GLfloat)g_texRectTVtx[2].a,
            (GLfloat)g_texRectTVtx[1].r, (GLfloat)g_texRectTVtx[1].g, (GLfloat)g_texRectTVtx[1].b, (GLfloat)g_texRectTVtx[1].a,
            (GLfloat)g_texRectTVtx[0].r, (GLfloat)g_texRectTVtx[0].g, (GLfloat)g_texRectTVtx[0].b, (GLfloat)g_texRectTVtx[0].a
    };

    GLfloat tex[] = {
            g_texRectTVtx[3].tcord[0].u,g_texRectTVtx[3].tcord[0].v,
            g_texRectTVtx[2].tcord[0].u,g_texRectTVtx[2].tcord[0].v,
            g_texRectTVtx[1].tcord[0].u,g_texRectTVtx[1].tcord[0].v,
            g_texRectTVtx[0].tcord[0].u,g_texRectTVtx[0].tcord[0].v
    };

    GLfloat tex2[] = {
       g_texRectTVtx[3].tcord[1].u,g_texRectTVtx[3].tcord[1].v,
       g_texRectTVtx[2].tcord[1].u,g_texRectTVtx[2].tcord[1].v,
       g_texRectTVtx[1].tcord[1].u,g_texRectTVtx[1].tcord[1].v,
       g_texRectTVtx[0].tcord[1].u,g_texRectTVtx[0].tcord[1].v

    };

    float w = windowSetting.uDisplayWidth / 2.0f, h = windowSetting.uDisplayHeight / 2.0f, inv = 1.0f;

    GLfloat vertices[] = {
            -inv + g_texRectTVtx[3].x / w, inv - g_texRectTVtx[3].y / h, depth, 1,
            -inv + g_texRectTVtx[2].x / w, inv - g_texRectTVtx[2].y / h, depth, 1,
            -inv + g_texRectTVtx[1].x / w, inv - g_texRectTVtx[1].y / h, depth, 1,
            -inv + g_texRectTVtx[0].x / w, inv - g_texRectTVtx[0].y / h, depth, 1
    };

    glVertexAttribPointer(VS_COLOR, 4, GL_FLOAT,GL_TRUE, 0, &colour );
    glVertexAttribPointer(VS_POSITION,4,GL_FLOAT,GL_FALSE,0,&vertices);
    glVertexAttribPointer(VS_TEXCOORD0,2,GL_FLOAT,GL_FALSE, 0, &tex);
    glVertexAttribPointer(VS_TEXCOORD1,2,GL_FLOAT,GL_FALSE, 0, &tex2);
    glDrawArrays(GL_TRIANGLE_FAN,0,4);

    //Restore old pointers
    glVertexAttribPointer(VS_COLOR, 4, GL_UNSIGNED_BYTE,GL_TRUE, sizeof(uint8_t)*4, &(g_oglVtxColors[0][0]) );
    glVertexAttribPointer(VS_POSITION,4,GL_FLOAT,GL_FALSE,sizeof(float)*5,&(g_vtxProjected5[0][0]));
    glVertexAttribPointer(VS_TEXCOORD0,2,GL_FLOAT,GL_FALSE, sizeof( TLITVERTEX ), &(g_vtxBuffer[0].tcord[0].u));
    glVertexAttribPointer(VS_TEXCOORD1,2,GL_FLOAT,GL_FALSE, sizeof( TLITVERTEX ), &(g_vtxBuffer[0].tcord[1].u));

    if( cullface ) glEnable(GL_CULL_FACE);

    return true;
}

bool OGLRender::RenderFillRect(uint32_t dwColor, float depth)
{
    float a = (dwColor>>24)/255.0f;
    float r = ((dwColor>>16)&0xFF)/255.0f;
    float g = ((dwColor>>8)&0xFF)/255.0f;
    float b = (dwColor&0xFF)/255.0f;
    glViewportWrapper(0, 0, windowSetting.uDisplayWidth, windowSetting.uDisplayHeight, true);

    GLboolean cullface = glIsEnabled(GL_CULL_FACE);
    glDisable(GL_CULL_FACE);

    GLfloat colour[] = {
            r,g,b,a,
            r,g,b,a,
            r,g,b,a,
            r,g,b,a};

    float w = windowSetting.uDisplayWidth / 2.0f, h = windowSetting.uDisplayHeight / 2.0f, inv = 1.0f;

    GLfloat vertices[] = {
            -inv + m_fillRectVtx[0].x / w, inv - m_fillRectVtx[1].y / h, depth, 1,
            -inv + m_fillRectVtx[1].x / w, inv - m_fillRectVtx[1].y / h, depth, 1,
            -inv + m_fillRectVtx[1].x / w, inv - m_fillRectVtx[0].y / h, depth, 1,
            -inv + m_fillRectVtx[0].x / w, inv - m_fillRectVtx[0].y / h, depth, 1
    };

    glVertexAttribPointer(VS_COLOR, 4, GL_FLOAT,GL_FALSE, 0, &colour );
    glVertexAttribPointer(VS_POSITION,4,GL_FLOAT,GL_FALSE,0,&vertices);
    glDisableVertexAttribArray(VS_TEXCOORD0);
    glDisableVertexAttribArray(VS_TEXCOORD1);

    glDrawArrays(GL_TRIANGLE_FAN,0,4);

    //Restore old pointers
    glVertexAttribPointer(VS_COLOR, 4, GL_UNSIGNED_BYTE,GL_TRUE, sizeof(uint8_t)*4, &(g_oglVtxColors[0][0]) );
    glVertexAttribPointer(VS_POSITION,4,GL_FLOAT,GL_FALSE,sizeof(float)*5,&(g_vtxProjected5[0][0]));
    glEnableVertexAttribArray(VS_TEXCOORD0);
    glEnableVertexAttribArray(VS_TEXCOORD1);

    if( cullface ) glEnable(GL_CULL_FACE);

    return true;
}

bool OGLRender::RenderLine3D()
{
    return true;
}

extern FiddledVtx * g_pVtxBase;

// This is so weird that I can not do vertex transform by myself. I have to use
// OpenGL internal transform
bool OGLRender::RenderFlushTris()
{
   if( !gRDP.bFogEnableInBlender && gRSP.bFogEnabled )
   {
      TurnFogOnOff(false);
   }

    ApplyZBias(m_dwZBias);  // set the bias factors

    glViewportWrapper(windowSetting.vpLeftW, windowSetting.uDisplayHeight - windowSetting.vpTopW - windowSetting.vpHeightW, windowSetting.vpWidthW, windowSetting.vpHeightW, false);

    //if options.bOGLVertexClipper == false )
    {
        glDrawElements( GL_TRIANGLES, gRSP.numVertices, GL_UNSIGNED_SHORT, g_vtxIndex );
    }

    if( !gRDP.bFogEnableInBlender && gRSP.bFogEnabled )
    {
       TurnFogOnOff(true);
    }
    return true;
}

void OGLRender::DrawSimple2DTexture(float x0, float y0, float x1, float y1, float u0, float v0, float u1, float v1, COLOR dif, COLOR spe, float z, float rhw)
{
    if( status.bVIOriginIsUpdated == true && currentRomOptions.screenUpdateSetting==SCREEN_UPDATE_AT_1ST_PRIMITIVE )
    {
        status.bVIOriginIsUpdated=false;
        CGraphicsContext::Get()->UpdateFrame(false);
    }

    StartDrawSimple2DTexture(x0, y0, x1, y1, u0, v0, u1, v1, dif, spe, z, rhw);

    GLboolean cullface = glIsEnabled(GL_CULL_FACE);
    glDisable(GL_CULL_FACE);

    glViewportWrapper(0, 0, windowSetting.uDisplayWidth, windowSetting.uDisplayHeight, true);

    float a = (g_texRectTVtx[0].dcDiffuse >>24)/255.0f;
    float r = ((g_texRectTVtx[0].dcDiffuse>>16)&0xFF)/255.0f;
    float g = ((g_texRectTVtx[0].dcDiffuse>>8)&0xFF)/255.0f;
    float b = (g_texRectTVtx[0].dcDiffuse&0xFF)/255.0f;

    GLfloat colour[] = {
            r,g,b,a,
            r,g,b,a,
            r,g,b,a,
            r,g,b,a,
            r,g,b,a,
            r,g,b,a
    };

    GLfloat tex[] = {
            g_texRectTVtx[0].tcord[0].u,g_texRectTVtx[0].tcord[0].v,
            g_texRectTVtx[1].tcord[0].u,g_texRectTVtx[1].tcord[0].v,
            g_texRectTVtx[2].tcord[0].u,g_texRectTVtx[2].tcord[0].v,

            g_texRectTVtx[0].tcord[0].u,g_texRectTVtx[0].tcord[0].v,
            g_texRectTVtx[2].tcord[0].u,g_texRectTVtx[2].tcord[0].v,
            g_texRectTVtx[3].tcord[0].u,g_texRectTVtx[3].tcord[0].v,
    };

    GLfloat tex2[] = {
       g_texRectTVtx[0].tcord[1].u,g_texRectTVtx[0].tcord[1].v,
       g_texRectTVtx[1].tcord[1].u,g_texRectTVtx[1].tcord[1].v,
       g_texRectTVtx[2].tcord[1].u,g_texRectTVtx[2].tcord[1].v,

       g_texRectTVtx[0].tcord[1].u,g_texRectTVtx[0].tcord[1].v,
       g_texRectTVtx[2].tcord[1].u,g_texRectTVtx[2].tcord[1].v,
       g_texRectTVtx[3].tcord[1].u,g_texRectTVtx[3].tcord[1].v,

    };

     float w = windowSetting.uDisplayWidth / 2.0f, h = windowSetting.uDisplayHeight / 2.0f, inv = 1.0f;

    GLfloat vertices[] = {
            -inv + g_texRectTVtx[0].x/ w, inv - g_texRectTVtx[0].y/ h, -g_texRectTVtx[0].z,1,
            -inv + g_texRectTVtx[1].x/ w, inv - g_texRectTVtx[1].y/ h, -g_texRectTVtx[1].z,1,
            -inv + g_texRectTVtx[2].x/ w, inv - g_texRectTVtx[2].y/ h, -g_texRectTVtx[2].z,1,

            -inv + g_texRectTVtx[0].x/ w, inv - g_texRectTVtx[0].y/ h, -g_texRectTVtx[0].z,1,
            -inv + g_texRectTVtx[2].x/ w, inv - g_texRectTVtx[2].y/ h, -g_texRectTVtx[2].z,1,
            -inv + g_texRectTVtx[3].x/ w, inv - g_texRectTVtx[3].y/ h, -g_texRectTVtx[3].z,1
    };

    glVertexAttribPointer(VS_COLOR, 4, GL_FLOAT,GL_FALSE, 0, &colour );
    glVertexAttribPointer(VS_POSITION,4,GL_FLOAT,GL_FALSE,0,&vertices);
    glVertexAttribPointer(VS_TEXCOORD0,2,GL_FLOAT,GL_FALSE, 0, &tex);
    glVertexAttribPointer(VS_TEXCOORD1,2,GL_FLOAT,GL_FALSE, 0, &tex2);
    glDrawArrays(GL_TRIANGLES,0,6);

    //Restore old pointers
    glVertexAttribPointer(VS_COLOR, 4, GL_UNSIGNED_BYTE,GL_TRUE, sizeof(uint8_t)*4, &(g_oglVtxColors[0][0]) );
    glVertexAttribPointer(VS_POSITION,4,GL_FLOAT,GL_FALSE,sizeof(float)*5,&(g_vtxProjected5[0][0]));
    glVertexAttribPointer(VS_TEXCOORD0,2,GL_FLOAT,GL_FALSE, sizeof( TLITVERTEX ), &(g_vtxBuffer[0].tcord[0].u));
    glVertexAttribPointer(VS_TEXCOORD1,2,GL_FLOAT,GL_FALSE, sizeof( TLITVERTEX ), &(g_vtxBuffer[0].tcord[1].u));

    if( cullface ) glEnable(GL_CULL_FACE);
}

void OGLRender::DrawSimpleRect(int nX0, int nY0, int nX1, int nY1, uint32_t dwColor, float depth, float rhw)
{
    StartDrawSimpleRect(nX0, nY0, nX1, nY1, dwColor, depth, rhw);

    GLboolean cullface = glIsEnabled(GL_CULL_FACE);
    glDisable(GL_CULL_FACE);

    float a = (dwColor>>24)/255.0f;
    float r = ((dwColor>>16)&0xFF)/255.0f;
    float g = ((dwColor>>8)&0xFF)/255.0f;
    float b = (dwColor&0xFF)/255.0f;

    GLfloat colour[] = {
            r,g,b,a,
            r,g,b,a,
            r,g,b,a,
            r,g,b,a};
    float w = windowSetting.uDisplayWidth / 2.0f, h = windowSetting.uDisplayHeight / 2.0f, inv = 1.0f;

    GLfloat vertices[] = {
            -inv + m_simpleRectVtx[1].x / w, inv - m_simpleRectVtx[0].y / h, -depth, 1,
            -inv + m_simpleRectVtx[1].x / w, inv - m_simpleRectVtx[1].y / h, -depth, 1,
            -inv + m_simpleRectVtx[0].x / w, inv - m_simpleRectVtx[1].y / h, -depth, 1,
            -inv + m_simpleRectVtx[0].x / w, inv - m_simpleRectVtx[0].y / h, -depth, 1
    };

    glVertexAttribPointer(VS_COLOR, 4, GL_FLOAT,GL_FALSE, 0, &colour );
    glVertexAttribPointer(VS_POSITION,4,GL_FLOAT,GL_FALSE,0,&vertices);
    glDisableVertexAttribArray(VS_TEXCOORD0);
    glDisableVertexAttribArray(VS_TEXCOORD1);

    glDrawArrays(GL_TRIANGLE_FAN,0,4);

    //Restore old pointers
    glVertexAttribPointer(VS_COLOR, 4, GL_UNSIGNED_BYTE,GL_TRUE, sizeof(uint8_t)*4, &(g_oglVtxColors[0][0]) );
    glVertexAttribPointer(VS_POSITION,4,GL_FLOAT,GL_FALSE,sizeof(float)*5,&(g_vtxProjected5[0][0]));
    glEnableVertexAttribArray(VS_TEXCOORD0);
    glEnableVertexAttribArray(VS_TEXCOORD1);


    if( cullface ) glEnable(GL_CULL_FACE);
}

#if 0
void OGLRender::InitCombinerBlenderForSimpleRectDraw(uint32_t tile)
{
    //glEnable(GL_CULL_FACE);
    EnableTexUnit(0, false);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    //glEnable(GL_ALPHA_TEST);
}
#endif

COLOR OGLRender::PostProcessDiffuseColor(COLOR curDiffuseColor)
{
    uint32_t color = curDiffuseColor;
    uint32_t colorflag = m_pColorCombiner->m_pDecodedMux->m_dwShadeColorChannelFlag;
    uint32_t alphaflag = m_pColorCombiner->m_pDecodedMux->m_dwShadeAlphaChannelFlag;
    if( colorflag+alphaflag != MUX_0 )
    {
        if( (colorflag & 0xFFFFFF00) == 0 && (alphaflag & 0xFFFFFF00) == 0 )
            color = (m_pColorCombiner->GetConstFactor(colorflag, alphaflag, curDiffuseColor));
        else
            color = (CalculateConstFactor(colorflag, alphaflag, curDiffuseColor));
    }

    //return (color<<8)|(color>>24);
    return color;
}

COLOR OGLRender::PostProcessSpecularColor()
{
    return 0;
}

void OGLRender::SetViewportRender()
{
    glViewportWrapper(windowSetting.vpLeftW, windowSetting.uDisplayHeight - windowSetting.vpTopW - windowSetting.vpHeightW, windowSetting.vpWidthW, windowSetting.vpHeightW, true);
}

void OGLRender::RenderReset()
{
    CRender::RenderReset();
}

void OGLRender::SetAlphaTestEnable(bool bAlphaTestEnable)
{
#ifdef DEBUGGER
    if( bAlphaTestEnable && debuggerEnableAlphaTest )
#else
        COGL_FragmentProgramCombiner* frag = (COGL_FragmentProgramCombiner*)m_pColorCombiner;
        frag->SetAlphaTestState(bAlphaTestEnable);
#endif
}

void OGLRender::BindTexture(GLuint texture, int unitno)
{
#ifdef DEBUGGER
    if( unitno != 0 )
    {
        DebuggerAppendMsg("Check me, base ogl bind texture, unit no != 0");
    }
#endif
    if( m_curBoundTex[0] != texture )
    {
        glBindTexture(GL_TEXTURE_2D,texture);
        m_curBoundTex[0] = texture;
    }
}

void OGLRender::DisBindTexture(GLuint texture, int unitno)
{
    //EnableTexUnit(0, false);
    //glBindTexture(GL_TEXTURE_2D, 0);  //Not to bind any texture
}

void OGLRender::EnableTexUnit(int unitno, bool flag)
{
#ifdef DEBUGGER
    if( unitno != 0 )
        DebuggerAppendMsg("Check me, in the base OpenGL render, unitno!=0");
#endif
    if( m_texUnitEnabled[0] != flag )
        m_texUnitEnabled[0] = flag;
}

void OGLRender::TexCoord2f(float u, float v)
{
    glTexCoord2f(u, v);
}

void OGLRender::TexCoord(TLITVERTEX &vtxInfo)
{
    glTexCoord2f(vtxInfo.tcord[0].u, vtxInfo.tcord[0].v);
}

void OGLRender::UpdateScissor()
{
    if( options.bEnableHacks && g_CI.dwWidth == 0x200 && gRDP.scissor.right == 0x200 && g_CI.dwWidth>(*gfx_info.VI_WIDTH_REG & 0xFFF) )
    {
        // Hack for RE2
        uint32_t width = *gfx_info.VI_WIDTH_REG & 0xFFF;
        uint32_t height = (gRDP.scissor.right*gRDP.scissor.bottom)/width;
        glEnable(GL_SCISSOR_TEST);
        glScissor(0, int(height * windowSetting.fMultY),
            int(width*windowSetting.fMultX), int(height*windowSetting.fMultY) );
    }
    else
    {
        UpdateScissorWithClipRatio();
    }
}

void OGLRender::ApplyRDPScissor(bool force)
{
    if( !force && status.curScissor == RDP_SCISSOR )
        return;

    if( options.bEnableHacks && g_CI.dwWidth == 0x200 && gRDP.scissor.right == 0x200 && g_CI.dwWidth>(*gfx_info.VI_WIDTH_REG & 0xFFF) )
    {
        // Hack for RE2
        uint32_t width = *gfx_info.VI_WIDTH_REG & 0xFFF;
        uint32_t height = (gRDP.scissor.right*gRDP.scissor.bottom)/width;
        glEnable(GL_SCISSOR_TEST);
        glScissor(0, int(height * windowSetting.fMultY),
            int(width*windowSetting.fMultX), int(height*windowSetting.fMultY) );
    }
    else
    {
        glScissor(int(gRDP.scissor.left*windowSetting.fMultX), int((windowSetting.uViHeight-gRDP.scissor.bottom)*windowSetting.fMultY),
            int((gRDP.scissor.right-gRDP.scissor.left)*windowSetting.fMultX), int((gRDP.scissor.bottom-gRDP.scissor.top)*windowSetting.fMultY ));
    }

    status.curScissor = RDP_SCISSOR;
}

void OGLRender::ApplyScissorWithClipRatio(bool force)
{
    if( !force && status.curScissor == RSP_SCISSOR )
        return;

    glEnable(GL_SCISSOR_TEST);
    glScissor(windowSetting.clipping.left, int((windowSetting.uViHeight-gRSP.real_clip_scissor_bottom)*windowSetting.fMultY),
        windowSetting.clipping.width, windowSetting.clipping.height);

    status.curScissor = RSP_SCISSOR;
}

void OGLRender::SetFogMinMax(float fMin, float fMax)
{
}

void OGLRender::TurnFogOnOff(bool flag)
{
    ((COGL_FragmentProgramCombiner*)m_pColorCombiner)->SetFogState(flag);
}

void OGLRender::SetFogEnable(bool bEnable)
{
    gRSP.bFogEnabled = bEnable;
    
    // If force fog
    if(options.fogMethod == 2)
        gRSP.bFogEnabled = true;

    ((COGL_FragmentProgramCombiner*)m_pColorCombiner)->SetFogState(gRSP.bFogEnabled);
}

void OGLRender::SetFogColor(uint32_t r, uint32_t g, uint32_t b, uint32_t a)
{
    gRDP.fogColor = COLOR_RGBA(r, g, b, a); 
    gRDP.fvFogColor[0] = r/255.0f;      //r
    gRDP.fvFogColor[1] = g/255.0f;      //g
    gRDP.fvFogColor[2] = b/255.0f;      //b
    gRDP.fvFogColor[3] = a/255.0f;      //a
}

void OGLRender::DisableMultiTexture()
{
    glActiveTexture(GL_TEXTURE1);
    EnableTexUnit(1, false);
    glActiveTexture(GL_TEXTURE0);
    EnableTexUnit(0, false);
    glActiveTexture(GL_TEXTURE0);
    EnableTexUnit(0, true);
}

void OGLRender::EndRendering(void)
{
    if( CRender::gRenderReferenceCount > 0 ) 
        CRender::gRenderReferenceCount--;
}

void OGLRender::glViewportWrapper(GLint x, GLint y, GLsizei width, GLsizei height, bool flag)
{
    static GLint mx=0,my=0;
    static GLsizei m_width=0, m_height=0;
    static bool mflag=true;

    if( x!=mx || y!=my || width!=m_width || height!=m_height || mflag!=flag)
    {
        mx=x;
        my=y;
        m_width=width;
        m_height=height;
        mflag=flag;
        glViewport(x,y,width,height);
    }
}

