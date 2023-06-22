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

#ifndef _RICE_RENDER_BASE_H
#define _RICE_RENDER_BASE_H

#include "osal_preproc.h"
#include "Debugger.h"
#include "RSP_Parser.h"
#include "Video.h"

#include "../../Graphics/RSP/gSP_state.h"

/*
 *  Global variables defined in this file were moved out of Render class
 *  to make them be accessed faster
 */

#define RICE_MATRIX_STACK   60
#define MAX_TEXTURES         8

enum FillMode
{
    RICE_FILLMODE_WINFRAME,
    RICE_FILLMODE_SOLID,
};

enum { MAX_VERTS = 80 };        // F3DLP.Rej supports up to 80 verts!

void myVec3Transform(float *vecout, float *vecin, float* m);

// All these arrays are moved out of the class CRender
// to be accessed in faster speed
extern ALIGN(16, XVECTOR4 g_vtxTransformed[MAX_VERTS]);
extern ALIGN(16, XVECTOR4 g_vecProjected[MAX_VERTS]);
extern float        g_vtxProjected5[1000][5];
extern float        g_vtxProjected5Clipped[2000][5];
extern VECTOR2      g_fVtxTxtCoords[MAX_VERTS];
extern uint32_t       g_dwVtxDifColor[MAX_VERTS];
//extern uint32_t     g_dwVtxFlags[MAX_VERTS];            // Z_POS Z_NEG etc

extern RenderTexture g_textures[MAX_TEXTURES];

extern TLITVERTEX       g_vtxBuffer[1000];
extern unsigned short   g_vtxIndex[1000];

extern TLITVERTEX       g_clippedVtxBuffer[2000];
extern int              g_clippedVtxCount;

extern uint8_t            g_oglVtxColors[1000][4];
extern uint32_t           g_clipFlag[MAX_VERTS];
extern uint32_t           g_clipFlag2[MAX_VERTS];
extern float            g_fFogCoord[MAX_VERTS];

extern TLITVERTEX       g_texRectTVtx[4];

extern EXTERNAL_VERTEX  g_vtxForExternal[MAX_VERTS];


//#define INIT_VERTEX_METHOD_2

/*
 *  Global variables
 */

/************************************************************************/
/*      Don't move                                                      */
/************************************************************************/

extern Light              gRSPlights[16];
extern ALIGN(16, Matrix   gRSPworldProjectTransported);
extern ALIGN(16, Matrix   gRSPworldProject);
extern N64Light           gRSPn64lights[16];
extern ALIGN(16, Matrix   gRSPmodelViewTop);
extern ALIGN(16, Matrix   gRSPmodelViewTopTranspose);
extern float              gRSPfFogMin;
extern float              gRSPfFogMax;
extern float              gRSPfFogDivider;

/************************************************************************/
/*      Don't move                                                      */
/************************************************************************/
typedef struct 
{
    /************************************************************************/
    /*      Don't move                                                      */
    /************************************************************************/
    union {     
        struct {
            float   fAmbientLightR;
            float   fAmbientLightG;
            float   fAmbientLightB;
            float   fAmbientLightA;
        };
        float fAmbientColors[4];
    };
    /************************************************************************/
    /*      Don't move above                                                */
    /************************************************************************/
    bool    bTextureEnabled;
    uint32_t  curTile;
    float   fTexScaleX;
    float   fTexScaleY;

    RenderShadeMode shadeMode;
    bool    bCullFront;
    bool    bCullBack;
    bool    bLightingEnable;
    bool    bTextureGen;
    bool    bFogEnabled;
    bool    bZBufferEnabled;

    uint32_t  ambientLightColor;
    uint32_t  ambientLightIndex;

    uint32_t  projectionMtxTop;
    uint32_t  modelViewMtxTop;

    uint32_t  numVertices;
    uint32_t  maxVertexID;

    int     nVPLeftN, nVPTopN, nVPRightN, nVPBottomN, nVPWidthN, nVPHeightN, maxZ;
    int     clip_ratio_negx,    clip_ratio_negy,    clip_ratio_posx,    clip_ratio_posy;
    int     clip_ratio_left,    clip_ratio_top, clip_ratio_right,   clip_ratio_bottom;
    int     real_clip_scissor_left, real_clip_scissor_top,  real_clip_scissor_right,    real_clip_scissor_bottom;
    float   real_clip_ratio_negx,   real_clip_ratio_negy,   real_clip_ratio_posx,   real_clip_ratio_posy;

    Matrix  projectionMtxs[RICE_MATRIX_STACK];
    Matrix  modelviewMtxs[RICE_MATRIX_STACK];

    bool    bWorldMatrixIsUpdated;
    bool    bMatrixIsUpdated;
    bool    bCombinedMatrixIsUpdated;
    bool    bLightIsUpdated;

    int     DKRCMatrixIndex;
    int     DKRVtxCount;
    bool    DKRBillBoard;
    uint32_t  dwDKRVtxAddr;
    uint32_t  dwDKRMatrixAddr;
    Matrix  DKRMatrixes[4];
    XVECTOR4  DKRBaseVec;

    int     ucode;
    int     vertexMult; 
    bool    bNearClip;
    bool    bRejectVtx;

    bool    bProcessDiffuseColor;
    bool    bProcessSpecularColor;

    float   vtxXMul;
    float   vtxXAdd;
    float   vtxYMul;
    float   vtxYAdd;

    // Texture coordinates computation constants
    float   tex0scaleX;
    float   tex0scaleY;
    float   tex1scaleX;
    float   tex1scaleY;
    float   tex0OffsetX;
    float   tex0OffsetY;
    float   tex1OffsetX;
    float   tex1OffsetY;
    float   texGenYRatio;
    float   texGenXRatio;

} RSP_Options;

extern ALIGN(16, RSP_Options gRSP);

typedef struct
{
    uint32_t  keyR;
    uint32_t  keyG;
    uint32_t  keyB;
    uint32_t  keyA;
    uint32_t  keyRGB;
    uint32_t  keyRGBA;
    float   fKeyA;
    
    bool    bFogEnableInBlender;

    uint32_t  fogColor;
    uint32_t  primitiveColor;
    uint32_t  envColor;
    uint32_t  primitiveDepth;
    uint32_t  primLODMin;
    uint32_t  primLODFrac;
    uint32_t  LODFrac;

    float   fPrimitiveDepth;
    float   fvFogColor[4];
    float   fvPrimitiveColor[4];
    float   fvEnvColor[4];

    uint32_t  fillColor;
    uint32_t  originalFillColor;

    RDP_OtherMode otherMode;

    TileAdditionalInfo    tilesinfo[8];
    ScissorType scissor;

    bool    textureIsChanged;
    bool    texturesAreReloaded;
    bool    colorsAreReloaded;
} RDP_Options;

extern ALIGN(16, RDP_Options gRDP);

/*
*   Global functions
*/
void InitRenderBase();
void SetFogMinMax(float fMin, float fMax, float fMul, float fOffset);
void InitVertex(uint32_t dwV, uint32_t vtxIndex, bool bTexture);
void InitVertexTextureConstants();
bool PrepareTriangle(uint32_t dwV0, uint32_t dwV1, uint32_t dwV2);
bool IsTriangleVisible(uint32_t dwV0, uint32_t dwV1, uint32_t dwV2);
extern void (*ProcessVertexData)(uint32_t dwAddr, uint32_t dwV0, uint32_t dwNum);
void ProcessVertexDataNoSSE(uint32_t dwAddr, uint32_t dwV0, uint32_t dwNum);
void ProcessVertexDataNEON(uint32_t dwAddr, uint32_t dwV0, uint32_t dwNum);
void ProcessVertexDataExternal(uint32_t dwAddr, uint32_t dwV0, uint32_t dwNum);
void SetPrimitiveColor(uint32_t dwCol, uint32_t LODMin, uint32_t LODFrac);
void SetPrimitiveDepth(uint32_t z, uint32_t dwDZ);
void SetVertexXYZ(uint32_t vertex, float x, float y, float z);
void ricegSPModifyVertex(uint32_t vtx, uint32_t where,  uint32_t val);
void ProcessVertexDataDKR(uint32_t dwAddr, uint32_t dwV0, uint32_t dwNum);
void ricegSPLightColor(uint32_t dwLight, uint32_t dwCol);
void SetLightDirection(uint32_t dwLight, float x, float y, float z, float range);
void ForceMainTextureIndex(int dwTile); 
void UpdateCombinedMatrix();

void ClipVertexes();
void ClipVertexesOpenGL();
void ClipVertexesForRect();

void LogTextureCoords(float fTex0S, float fTex0T, float fTex1S, float fTex1T);
bool CheckTextureCoords(int tex);
void ResetTextureCoordsLog(float maxs0, float maxt0, float maxs1, float maxt1);

inline float ViewPortTranslatef_x(float x) { return ( (x+1) * windowSetting.vpWidthW/2) + windowSetting.vpLeftW; }
inline float ViewPortTranslatef_y(float y) { return ( (1-y) * windowSetting.vpHeightW/2) + windowSetting.vpTopW; }
inline float ViewPortTranslatei_x(int x) { return x*windowSetting.fMultX; }
inline float ViewPortTranslatei_y(int y) { return y*windowSetting.fMultY; }
inline float ViewPortTranslatei_x(float x) { return x*windowSetting.fMultX; }
inline float ViewPortTranslatei_y(float y) { return y*windowSetting.fMultY; }

inline float GetPrimitiveDepth() { return gRDP.fPrimitiveDepth; }
inline uint32_t GetPrimitiveColor() { return gRDP.primitiveColor; }
inline float* GetPrimitiveColorfv() { return gRDP.fvPrimitiveColor; }
inline uint32_t GetLODFrac() { return gRDP.LODFrac; }
inline uint32_t GetEnvColor() { return gRDP.envColor; }
inline float* GetEnvColorfv() { return gRDP.fvEnvColor; }

inline void SetAmbientLight(uint32_t color) 
{ 
    gRSP.ambientLightColor = color; 
    gRSP.fAmbientLightR = (float)RGBA_GETRED(gRSP.ambientLightColor);
    gRSP.fAmbientLightG = (float)RGBA_GETGREEN(gRSP.ambientLightColor);
    gRSP.fAmbientLightB = (float)RGBA_GETBLUE(gRSP.ambientLightColor);
    LIGHT_DUMP(TRACE1("Set Ambient Light: %08X", color));
}

inline void SetLighting(bool bLighting) { gRSP.bLightingEnable = bLighting; }

// Generate texture coords?
inline void SetTextureGen(bool bTextureGen) { gRSP.bTextureGen = bTextureGen; }
inline COLOR GetVertexDiffuseColor(uint32_t ver) { return g_dwVtxDifColor[ver]; }
inline void SetScreenMult(float fMultX, float fMultY) { windowSetting.fMultX = fMultX; windowSetting.fMultY = fMultY; }
inline COLOR GetLightCol(uint32_t dwLight) { return gRSPlights[dwLight].col; }

void ricegSPNumLights(int32_t n);

#endif

