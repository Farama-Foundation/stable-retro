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

#ifndef _RICE_RENDER_H
#define _RICE_RENDER_H

#include "Blender.h"
#include "Combiner.h"
#include "Config.h"
#include "Debugger.h"
#include "RenderBase.h"
#include "RSP_Parser.h"
#include "RSP_S2DEX.h"

enum TextureChannel 
{
    TXT_RGB,
    TXT_ALPHA,
    TXT_RGBA,
};

class CRender
{
protected:
    CRender();

    TextureUVFlag TileUFlags[8];
    TextureUVFlag TileVFlags[8];

public:

    float m_fScreenViewportMultX;
    float m_fScreenViewportMultY;


    uint32_t  m_dwTexturePerspective;
    bool    m_bAlphaTestEnable;

    bool    m_bZUpdate;
    bool    m_bZCompare;
    uint32_t  m_dwZBias;

    TextureFilter   m_dwMinFilter;
    TextureFilter   m_dwMagFilter;

    uint32_t  m_dwAlpha;

    uint64_t      m_Mux;
    bool    m_bBlendModeValid;

    CColorCombiner *m_pColorCombiner;
    CBlender *m_pAlphaBlender;
    
    
    virtual ~CRender();
    
    inline bool IsTexel0Enable() {return m_pColorCombiner->m_bTex0Enabled;}
    inline bool IsTexel1Enable() {return m_pColorCombiner->m_bTex1Enabled;}
    inline bool IsTextureEnabled() { return (m_pColorCombiner->m_bTex0Enabled||m_pColorCombiner->m_bTex1Enabled); }

    inline RenderTexture& GetCurrentTexture() { return g_textures[gRSP.curTile]; }
    inline RenderTexture& GetTexture(uint32_t dwTile) { return g_textures[dwTile]; }
    void SetViewport(int nLeft, int nTop, int nRight, int nBottom, int maxZ);
    virtual void SetViewportRender() {}
    virtual void SetClipRatio(uint32_t type, uint32_t value);
    virtual void UpdateScissor() {}
    virtual void ApplyRDPScissor(bool force) {}
    virtual void UpdateClipRectangle();
    virtual void UpdateScissorWithClipRatio();
    virtual void ApplyScissorWithClipRatio(bool force) {}

    void SetTextureEnableAndScale(int dwTile, bool enable, float fScaleX, float fScaleY);
    
    virtual void SetFogEnable(bool bEnable) 
    { 
        gRSP.bFogEnabled = bEnable;
    }
    virtual void SetFogMinMax(float fMin, float fMax) = 0;
    virtual void TurnFogOnOff(bool flag)=0;

    virtual void SetFogColor(uint32_t r, uint32_t g, uint32_t b, uint32_t a) 
    { 
        gRDP.fogColor = COLOR_RGBA(r, g, b, a); 
    }
    uint32_t GetFogColor() { return gRDP.fogColor; }

    void SetProjection(const Matrix & mat, bool bPush, bool bReplace);
    void SetWorldView(const Matrix & mat, bool bPush, bool bReplace);
    inline int GetProjectMatrixLevel(void) { return gRSP.projectionMtxTop; }
    inline int GetWorldViewMatrixLevel(void) { return gRSP.modelViewMtxTop; }

    inline void PopProjection()
    {
        if (gRSP.projectionMtxTop > 0)
            gRSP.projectionMtxTop--;
        else
            TRACE0("Popping past projection stack limits");
    }

    void PopWorldView();
    Matrix & GetWorldProjectMatrix(void);
    void SetWorldProjectMatrix(Matrix &mtx);
    
    void ResetMatrices();

    inline RenderShadeMode GetShadeMode() { return gRSP.shadeMode; }

    void SetVtxTextureCoord(uint32_t dwV, float tu, float tv)
    {
        g_fVtxTxtCoords[dwV].x = tu;
        g_fVtxTxtCoords[dwV].y = tv;
    }

    virtual void RenderReset();
    virtual void SetCombinerAndBlender();
    virtual void SetMux(uint32_t dwMux0, uint32_t dwMux1);
    virtual void SetCullMode(bool bCullFront, bool bCullBack) { gRSP.bCullFront = bCullFront; gRSP.bCullBack = bCullBack; }

    virtual void BeginRendering(void) {CRender::gRenderReferenceCount++;}       // For DirectX only
    virtual void EndRendering(void) 
    {
        if( CRender::gRenderReferenceCount > 0 )
            CRender::gRenderReferenceCount--;
    }

    virtual void ClearBuffer(bool cbuffer, bool zbuffer)=0;
    virtual void ClearZBuffer(float depth)=0;
    virtual void ClearBuffer(bool cbuffer, bool zbuffer, COORDRECT &rect) 
    {
        ClearBuffer(cbuffer, zbuffer);
    }
    virtual void ZBufferEnable(bool bZBuffer)=0;
    virtual void SetZCompare(bool bZCompare)=0;
    virtual void SetZUpdate(bool bZUpdate)=0;
    virtual void SetZBias(int bias)=0;
    virtual void SetAlphaTestEnable(bool bAlphaTestEnable)=0;

    void SetTextureFilter(uint32_t dwFilter);
    virtual void ApplyTextureFilter() {}
    
    virtual void SetShadeMode(RenderShadeMode mode)=0;

    virtual void SetAlphaRef(uint32_t dwAlpha)=0;
    virtual void ForceAlphaRef(uint32_t dwAlpha)=0;

    virtual void InitOtherModes(void);

    void SetVertexTextureUVCoord(TLITVERTEX &v, float fTex0S, float fTex0T, float fTex1S, float fTex1T);
    void SetVertexTextureUVCoord(TLITVERTEX &v, float fTex0S, float fTex0T);
    void SetVertexTextureUVCoord(TLITVERTEX &v, const TexCord &fTex0, const TexCord &fTex1);
    void SetVertexTextureUVCoord(TLITVERTEX &v, const TexCord &fTex0);
    virtual COLOR PostProcessDiffuseColor(COLOR curDiffuseColor)=0;
    virtual COLOR PostProcessSpecularColor()=0;
    
    bool DrawTriangles();
    virtual bool RenderFlushTris()=0;

    bool TexRect(int nX0, int nY0, int nX1, int nY1, float fS0, float fT0, float fScaleS, float fScaleT, bool colorFlag, uint32_t difcolor);
    bool TexRectFlip(int nX0, int nY0, int nX1, int nY1, float fS0, float fT0, float fS1, float fT1);
    bool FillRect(int nX0, int nY0, int nX1, int nY1, uint32_t dwColor);
    bool Line3D(uint32_t dwV0, uint32_t dwV1, uint32_t dwWidth);

    virtual void SetAddressUAllStages(uint32_t dwTile, TextureUVFlag dwFlag); // For DirectX only, fix me
    virtual void SetAddressVAllStages(uint32_t dwTile, TextureUVFlag dwFlag); // For DirectX only, fix me
    virtual void SetTextureUFlag(TextureUVFlag dwFlag, uint32_t tile)=0;
    virtual void SetTextureVFlag(TextureUVFlag dwFlag, uint32_t tile)=0;
    virtual void SetTexelRepeatFlags(uint32_t dwTile);
    virtual void SetAllTexelRepeatFlag();
    
    virtual bool SetCurrentTexture(int tile, TxtrCacheEntry *pTextureEntry)=0;
    virtual bool SetCurrentTexture(int tile, CTexture *handler, uint32_t dwTileWidth, uint32_t dwTileHeight, TxtrCacheEntry *pTextureEntry) = 0;

    virtual bool InitDeviceObjects()=0;
    virtual bool ClearDeviceObjects()=0;
    virtual void Initialize(void);
    virtual void CleanUp(void);
    
    virtual void SetFillMode(FillMode mode)=0;

    void LoadSprite2D(Sprite2DInfo &info, uint32_t ucode);
    void LoadObjBGCopy(uObjBg &info);
    void LoadObjBG1CYC(uObjScaleBg &info);
    void LoadObjSprite(uObjTxSprite &info, bool useTIAddr);

    void LoadFrameBuffer(bool useVIreg, uint32_t left, uint32_t top, uint32_t width, uint32_t height);
    void LoadTextureFromMemory(void *buf, uint32_t left, uint32_t top, uint32_t width, uint32_t height, uint32_t pitch, uint32_t format);
    void LoadTxtrBufIntoTexture(void);
    void DrawSprite2D(Sprite2DInfo &info, uint32_t ucode);
    void DrawSpriteR(uObjTxSprite &sprite, bool initCombiner, uint32_t tile, uint32_t left, uint32_t top, uint32_t width, uint32_t height);
    void DrawSprite(uObjTxSprite &sprite, bool rectR);
    void DrawObjBGCopy(uObjBg &info);
    virtual void DrawSpriteR_Render(){};
    virtual void DrawSimple2DTexture(float x0, float y0, float x1, float y1, float u0, float v0, float u1, float v1, COLOR dif, COLOR spe, float z, float rhw)=0;
    void DrawFrameBuffer(bool useVIreg, uint32_t left, uint32_t top, uint32_t width, uint32_t height);
    void DrawObjBG1CYC(uObjScaleBg &bg, bool scaled);

    static CRender * g_pRender;
    static int gRenderReferenceCount;
    static CRender * GetRender(void);
    static bool IsAvailable();


protected:
    bool            m_savedZBufferFlag;
    uint32_t          m_savedMinFilter;
    uint32_t          m_savedMagFilter;

    // FillRect
    virtual bool    RenderFillRect(uint32_t dwColor, float depth)=0;
    VECTOR2         m_fillRectVtx[2];
    
    // Line3D
    virtual bool    RenderLine3D()=0;

    LITVERTEX       m_line3DVtx[2];
    VECTOR2         m_line3DVector[4];
    
    // TexRect
    virtual bool    RenderTexRect()=0;

    TexCord         m_texRectTex1UV[2];
    TexCord         m_texRectTex2UV[2];

    // DrawSimple2DTexture
    virtual void    StartDrawSimple2DTexture(float x0, float y0, float x1, float y1, float u0, float v0, float u1, float v1, COLOR dif, COLOR spe, float z, float rhw);

    // DrawSimpleRect
    virtual void    StartDrawSimpleRect(int nX0, int nY0, int nX1, int nY1, uint32_t dwColor, float depth, float rhw);
    VECTOR2         m_simpleRectVtx[2];

    bool            RemapTextureCoordinate(float s0, float s1, uint32_t tileWidth, uint32_t mask, float textureWidth,
                                            float &u0, float &u1);

};

#define ffloor(a) (((int(a))<=(a))?(float)(int(a)):((float)(int(a))-1))

#endif  //_RICE_RENDER_H

