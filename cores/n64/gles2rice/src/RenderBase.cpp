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

#include <stdint.h>
#include <math.h>
#include <vector>

#include <retro_miscellaneous.h>

#include "osal_preproc.h"
#include "float.h"
#include "DeviceBuilder.h"
#include "Render.h"
#include "Timing.h"

#include "../../Graphics/GBI.h"
#include "../../Graphics/RDP/gDP_state.h"

extern FiddledVtx * g_pVtxBase;

#define ENABLE_CLIP_TRI

#ifdef ENABLE_CLIP_TRI

void RSP_Vtx_Clipping(int i)
{
    g_clipFlag[i] = 0;
    g_clipFlag2[i] = 0;
    if( g_vecProjected[i].w > 0 )
    {
        /*
        if( gRSP.bRejectVtx )
        {
            if( g_vecProjected[i].x > 1 )   
            {
                g_clipFlag2[i] |= X_CLIP_MAX;
                if( g_vecProjected[i].x > gRSP.real_clip_ratio_posx )   
                    g_clipFlag[i] |= X_CLIP_MAX;
            }

            if( g_vecProjected[i].x < -1 )  
            {
                g_clipFlag2[i] |= X_CLIP_MIN;
                if( g_vecProjected[i].x < gRSP.real_clip_ratio_negx )   
                    g_clipFlag[i] |= X_CLIP_MIN;
            }

            if( g_vecProjected[i].y > 1 )   
            {
                g_clipFlag2[i] |= Y_CLIP_MAX;
                if( g_vecProjected[i].y > gRSP.real_clip_ratio_posy )   
                    g_clipFlag[i] |= Y_CLIP_MAX;
            }

            if( g_vecProjected[i].y < -1 )  
            {
                g_clipFlag2[i] |= Y_CLIP_MIN;
                if( g_vecProjected[i].y < gRSP.real_clip_ratio_negy )   
                    g_clipFlag[i] |= Y_CLIP_MIN;
            }

            //if( g_vecProjected[i].z > 1.0f )  
            //{
            //  g_clipFlag2[i] |= Z_CLIP_MAX;
            //  g_clipFlag[i] |= Z_CLIP_MAX;
            //}

            //if( gRSP.bNearClip && g_vecProjected[i].z < -1.0f )   
            //{
            //  g_clipFlag2[i] |= Z_CLIP_MIN;
            //  g_clipFlag[i] |= Z_CLIP_MIN;
            //}
        }
        else
        */
        {
            if( g_vecProjected[i].x > 1 )   g_clipFlag2[i] |= X_CLIP_MAX;
            if( g_vecProjected[i].x < -1 )  g_clipFlag2[i] |= X_CLIP_MIN;
            if( g_vecProjected[i].y > 1 )   g_clipFlag2[i] |= Y_CLIP_MAX;
            if( g_vecProjected[i].y < -1 )  g_clipFlag2[i] |= Y_CLIP_MIN;
            //if( g_vecProjected[i].z > 1.0f )  g_clipFlag2[i] |= Z_CLIP_MAX;
            //if( gRSP.bNearClip && g_vecProjected[i].z < -1.0f )   g_clipFlag2[i] |= Z_CLIP_MIN;
        }

    }
}

#else
inline void RSP_Vtx_Clipping(int i) {}
#endif

/*
 *  Global variables
 */
ALIGN(16,RSP_Options gRSP);
ALIGN(16,RDP_Options gRDP);

ALIGN(16,XVECTOR4 g_normal);
//static int norms[3];

ALIGN(16,XVECTOR4 g_vtxNonTransformed[MAX_VERTS]);
ALIGN(16,XVECTOR4 g_vecProjected[MAX_VERTS]);
ALIGN(16,XVECTOR4 g_vtxTransformed[MAX_VERTS]);

float       g_vtxProjected5[1000][5];
float       g_vtxProjected5Clipped[2000][5];

//uint32_t        g_dwVtxFlags[MAX_VERTS];            // Z_POS Z_NEG etc
VECTOR2     g_fVtxTxtCoords[MAX_VERTS];
uint32_t      g_dwVtxDifColor[MAX_VERTS];
uint32_t      g_clipFlag[MAX_VERTS];
uint32_t      g_clipFlag2[MAX_VERTS];
RenderTexture g_textures[MAX_TEXTURES];
float       g_fFogCoord[MAX_VERTS];

EXTERNAL_VERTEX g_vtxForExternal[MAX_VERTS];

TLITVERTEX          g_vtxBuffer[1000];
TLITVERTEX          g_clippedVtxBuffer[2000];
uint8_t               g_oglVtxColors[1000][4];
int                 g_clippedVtxCount=0;
TLITVERTEX          g_texRectTVtx[4];
unsigned short      g_vtxIndex[1000];
unsigned int        g_minIndex, g_maxIndex;

float               gRSPfFogMin;
float               gRSPfFogMax;
float               gRSPfFogDivider;

Light   gRSPlights[16];

ALIGN(16,Matrix  gRSPworldProjectTransported);
ALIGN(16,Matrix  gRSPworldProject);
ALIGN(16,Matrix  gRSPmodelViewTop);
ALIGN(16,Matrix  gRSPmodelViewTopTranspose);
ALIGN(16,Matrix  dkrMatrixTransposed);

N64Light        gRSPn64lights[16];


void (*ProcessVertexData)(uint32_t dwAddr, uint32_t dwV0, uint32_t dwNum)=NULL;

/* Multiply (x,y,z,0) by matrix m, then normalize */
#define Vec3TransformNormal(vec, m) \
   VECTOR3 temp; \
   temp.x = (vec.x * m._11) + (vec.y * m._21) + (vec.z * m._31); \
   temp.y = (vec.x * m._12) + (vec.y * m._22) + (vec.z * m._32); \
   temp.z = (vec.x * m._13) + (vec.y * m._23) + (vec.z * m._33); \
   float norm = sqrt(temp.x*temp.x+temp.y*temp.y+temp.z*temp.z); \
   if (norm == 0.0) { vec.x = 0.0; vec.y = 0.0; vec.z = 0.0;} else \
   { vec.x = temp.x/norm; vec.y = temp.y/norm; vec.z = temp.z/norm; }

float real255 = 255.0f;
float real128 = 128.0f;

void NormalizeNormalVec()
{
    float w = 1/sqrtf(g_normal.x*g_normal.x + g_normal.y*g_normal.y + g_normal.z*g_normal.z);
    g_normal.x *= w;
    g_normal.y *= w;
    g_normal.z *= w;
}


void InitRenderBase()
{
   ProcessVertexData = ProcessVertexDataNoSSE;

    gRSPfFogMin = gRSPfFogMax = 0.0f;
    windowSetting.fMultX = windowSetting.fMultY = 2.0f;
    windowSetting.vpLeftW = windowSetting.vpTopW = 0;
    windowSetting.vpRightW = windowSetting.vpWidthW = 640;
    windowSetting.vpBottomW = windowSetting.vpHeightW = 480;
    gRSP.maxZ = 0;
    gRSP.nVPLeftN = gRSP.nVPTopN = 0;
    gRSP.nVPRightN = 640;
    gRSP.nVPBottomN = 640;
    gRSP.nVPWidthN = 640;
    gRSP.nVPHeightN = 640;
    gRDP.scissor.left=gRDP.scissor.top=0;
    gRDP.scissor.right=gRDP.scissor.bottom=640;
    
    gRSP.bLightingEnable = gRSP.bTextureGen = false;
    gSP.numLights = 0;
    gRSP.curTile=gRSP.ambientLightColor=gRSP.ambientLightIndex= 0;
    gRSP.fAmbientLightR=gRSP.fAmbientLightG=gRSP.fAmbientLightB=0;
    gRSP.projectionMtxTop = gRSP.modelViewMtxTop = 0;
    gRDP.fogColor = gRDP.primitiveColor = gRDP.envColor = gRDP.primitiveDepth = gRDP.primLODMin = gRDP.primLODFrac = gRDP.LODFrac = 0;
    gRDP.fPrimitiveDepth = 0;
    gRSP.numVertices = 0;
    gRSP.maxVertexID = 0;
    gRSP.bCullFront=false;
    gRSP.bCullBack=true;
    gRSP.bFogEnabled=gRDP.bFogEnableInBlender=false;
    gRSP.bZBufferEnabled=true;
    gRSP.shadeMode=SHADE_SMOOTH;
    gRDP.keyR=gRDP.keyG=gRDP.keyB=gRDP.keyA=gRDP.keyRGB=gRDP.keyRGBA = 0;
    gRDP.fKeyA = 0;
    gRSP.DKRCMatrixIndex = gRSP.dwDKRVtxAddr = gRSP.dwDKRMatrixAddr = 0;
    gRSP.DKRBillBoard = false;

    gRSP.fTexScaleX = 1/32.0f;
    gRSP.fTexScaleY = 1/32.0f;
    gRSP.bTextureEnabled = false;

    gRSP.clip_ratio_left = 0;
    gRSP.clip_ratio_top = 0;
    gRSP.clip_ratio_right = 640;
    gRSP.clip_ratio_bottom = 480;
    gRSP.clip_ratio_negx = 1;
    gRSP.clip_ratio_negy = 1;
    gRSP.clip_ratio_posx = 1;
    gRSP.clip_ratio_posy = 1;
    gRSP.real_clip_scissor_left = 0;
    gRSP.real_clip_scissor_top = 0;
    gRSP.real_clip_scissor_right = 640;
    gRSP.real_clip_scissor_bottom = 480;
    windowSetting.clipping.left = 0;
    windowSetting.clipping.top = 0;
    windowSetting.clipping.right = 640;
    windowSetting.clipping.bottom = 480;
    windowSetting.clipping.width = 640;
    windowSetting.clipping.height = 480;
    windowSetting.clipping.needToClip = false;
    gRSP.real_clip_ratio_negx = 1;
    gRSP.real_clip_ratio_negy = 1;
    gRSP.real_clip_ratio_posx = 1;
    gRSP.real_clip_ratio_posy = 1;

    gRSP.DKRCMatrixIndex=0;
    gRSP.DKRVtxCount=0;
    gRSP.DKRBillBoard = false;
    gRSP.dwDKRVtxAddr=0;
    gRSP.dwDKRMatrixAddr=0;


    gSP.geometryMode    = 0;
    gDP.otherMode.l     = 0;
    gDP.otherMode.h     = 0;
    gRDP.fillColor      = 0xFFFFFFFF;
    gRDP.originalFillColor  =0;

    gRSP.ucode      = 1;
    gRSP.vertexMult = 10;
    gRSP.bNearClip  = false;
    gRSP.bRejectVtx = false;

    gRDP.texturesAreReloaded = false;
    gRDP.textureIsChanged = false;
    gRDP.colorsAreReloaded = false;

    memset(&gRDP.otherMode,0,sizeof(RDP_OtherMode));
    memset(&gRDP.tilesinfo,0,sizeof(TileAdditionalInfo)*8);
    memset(&gDP.tiles,0,sizeof(struct gDPTile)*8);

    for (int i=0; i<MAX_VERTS; i++)
    {
        g_clipFlag[i] = 0;
        g_vtxNonTransformed[i].w = 1;
    }

    memset(gRSPn64lights, 0, sizeof(N64Light)*16);
}

void SetFogMinMax(float fMin, float fMax, float fMul, float fOffset)
{
    if (fMin > fMax)
    {
        float temp = fMin;
        fMin = fMax;
        fMax = temp;
    }

    {
        gRSPfFogMin = MAX(0,fMin/500-1);
        gRSPfFogMax = fMax/500-1;
    }

    gRSPfFogDivider = 255/(gRSPfFogMax-gRSPfFogMin);
    CRender::g_pRender->SetFogMinMax(fMin, fMax);
}

void InitVertexTextureConstants()
{
    RenderTexture &tex0 = g_textures[gRSP.curTile];
    //CTexture *surf = tex0.m_pCTexture;
    TileAdditionalInfo *tile0 = &gRDP.tilesinfo[gRSP.curTile];

    float scaleX = gRSP.fTexScaleX;
    float scaleY = gRSP.fTexScaleY;

    gRSP.tex0scaleX = scaleX * tile0->fShiftScaleS/tex0.m_fTexWidth;
    gRSP.tex0scaleY = scaleY * tile0->fShiftScaleT/tex0.m_fTexHeight;

    gRSP.tex0OffsetX = tile0->fhilite_sl/tex0.m_fTexWidth;
    gRSP.tex0OffsetY = tile0->fhilite_tl/tex0.m_fTexHeight;

    if( CRender::g_pRender->IsTexel1Enable() )
    {
        RenderTexture &tex1 = g_textures[(gRSP.curTile+1)&7];
        //CTexture *surf = tex1.m_pCTexture;
        TileAdditionalInfo *tile1 = &gRDP.tilesinfo[(gRSP.curTile+1)&7];

        gRSP.tex1scaleX = scaleX * tile1->fShiftScaleS/tex1.m_fTexWidth;
        gRSP.tex1scaleY = scaleY * tile1->fShiftScaleT/tex1.m_fTexHeight;

        gRSP.tex1OffsetX = tile1->fhilite_sl/tex1.m_fTexWidth;
        gRSP.tex1OffsetY = tile1->fhilite_tl/tex1.m_fTexHeight;
    }

    gRSP.texGenXRatio = tile0->fShiftScaleS;
    gRSP.texGenYRatio = gRSP.fTexScaleX/gRSP.fTexScaleY*tex0.m_fTexWidth/tex0.m_fTexHeight*tile0->fShiftScaleT;
}

void TexGen(float &s, float &t)
{
    if (gSP.geometryMode & G_TEXTURE_GEN_LINEAR)
    {   
        s = acosf(g_normal.x) / 3.14159f;
        t = acosf(g_normal.y) / 3.14159f;
    }
    else
    {
        s = 0.5f * ( 1.0f + g_normal.x);
        t = 0.5f * ( 1.0f - g_normal.y);
    }
}

void ComputeLOD(void)
{
    TLITVERTEX &v0 = g_vtxBuffer[0];
    TLITVERTEX &v1 = g_vtxBuffer[1];
    RenderTexture &tex0 = g_textures[gRSP.curTile];

    float d,dt;
    float x = g_vtxProjected5[0][0] / g_vtxProjected5[0][4] - g_vtxProjected5[1][0] / g_vtxProjected5[1][4];
    float y = g_vtxProjected5[0][1] / g_vtxProjected5[0][4] - g_vtxProjected5[1][1] / g_vtxProjected5[1][4];

    x = windowSetting.vpWidthW*x/windowSetting.fMultX/2;
    y = windowSetting.vpHeightW*y/windowSetting.fMultY/2;
    d = sqrtf(x*x+y*y);

    float s0 = v0.tcord[0].u * tex0.m_fTexWidth;
    float t0 = v0.tcord[0].v * tex0.m_fTexHeight;
    float s1 = v1.tcord[0].u * tex0.m_fTexWidth;
    float t1 = v1.tcord[0].v * tex0.m_fTexHeight;

    dt = sqrtf((s0-s1)*(s0-s1)+(t0-t1)*(t0-t1));

    float lod = dt/d;
    float frac = log10f(lod)/log10f(2.0f);
    frac = (lod / powf(2.0f,floorf(frac)));
    frac = frac - floorf(frac);
    gRDP.LODFrac = (uint32_t)(frac*255);
    CRender::g_pRender->SetCombinerAndBlender();
}

bool bHalfTxtScale=false;
extern uint32_t lastSetTile;

#ifdef _MSC_VER
#define noinline __declspec(noinline)
#else
#define noinline __attribute__((noinline))
#endif

static noinline void InitVertex_scale_hack_check(uint32_t dwV)
{
    // Check for txt scale hack
    if( gDP.tiles[lastSetTile].size == G_IM_SIZ_32b || gDP.tiles[lastSetTile].size == G_IM_SIZ_4b )
    {
        int width = ((gDP.tiles[lastSetTile].uls  - gDP.tiles[lastSetTile].lrs+1)<<1);
        int height = ((gDP.tiles[lastSetTile].ult - gDP.tiles[lastSetTile].lrt+1)<<1);
        if( g_fVtxTxtCoords[dwV].x*gRSP.fTexScaleX == width || g_fVtxTxtCoords[dwV].y*gRSP.fTexScaleY == height )
            bHalfTxtScale=true;
    }
}

static noinline void InitVertex_notopengl_or_clipper_adjust(TLITVERTEX &v, uint32_t dwV)
{
    v.x = g_vecProjected[dwV].x*gRSP.vtxXMul+gRSP.vtxXAdd;
    v.y = g_vecProjected[dwV].y*gRSP.vtxYMul+gRSP.vtxYAdd;
    v.z = (g_vecProjected[dwV].z + 1.0f) * 0.5f;    // DirectX minZ=0, maxZ=1
    //v.z = g_vecProjected[dwV].z;  // DirectX minZ=0, maxZ=1
    v.rhw = g_vecProjected[dwV].w;

    if( gRSP.bProcessSpecularColor )
    {
        v.dcSpecular = CRender::g_pRender->PostProcessSpecularColor();
        if( gRSP.bFogEnabled )
        {
            v.dcSpecular &= 0x00FFFFFF;
            uint32_t  fogFct = 0xFF-(uint8_t)((g_fFogCoord[dwV]-gRSPfFogMin)*gRSPfFogDivider);
            v.dcSpecular |= (fogFct<<24);
        }
    }
    else if( gRSP.bFogEnabled )
    {
        uint32_t  fogFct = 0xFF-(uint8_t)((g_fFogCoord[dwV]-gRSPfFogMin)*gRSPfFogDivider);
        v.dcSpecular = (fogFct<<24);
    }
}

static noinline void InitVertex_texgen_correct(TLITVERTEX &v, uint32_t dwV)
{
    // Correction for texGen result
    float u0,u1,v0,v1;
    RenderTexture &tex0 = g_textures[gRSP.curTile];
    u0 = g_fVtxTxtCoords[dwV].x * 32 * 1024 * gRSP.fTexScaleX / tex0.m_fTexWidth;
    v0 = g_fVtxTxtCoords[dwV].y * 32 * 1024 * gRSP.fTexScaleY / tex0.m_fTexHeight;
    u0 *= (gRDP.tilesinfo[gRSP.curTile].fShiftScaleS);
    v0 *= (gRDP.tilesinfo[gRSP.curTile].fShiftScaleT);

    if( CRender::g_pRender->IsTexel1Enable() )
    {
        RenderTexture &tex1 = g_textures[(gRSP.curTile+1)&7];
        u1 = g_fVtxTxtCoords[dwV].x * 32 * 1024 * gRSP.fTexScaleX / tex1.m_fTexWidth;
        v1 = g_fVtxTxtCoords[dwV].y * 32 * 1024 * gRSP.fTexScaleY / tex1.m_fTexHeight;
        u1 *= gRDP.tilesinfo[(gRSP.curTile+1)&7].fShiftScaleS;
        v1 *= gRDP.tilesinfo[(gRSP.curTile+1)&7].fShiftScaleT;
        CRender::g_pRender->SetVertexTextureUVCoord(v, u0, v0, u1, v1);
    }
    else
    {
        CRender::g_pRender->SetVertexTextureUVCoord(v, u0, v0);
    }
}

static void multiply_subtract2(float *d, const float *m1, const float *m2, const float *s)
{
    int i;
    for (i = 0; i < 2; i++)
        d[i] = m1[i] * m2[i] - s[i];
}

void InitVertex(uint32_t dwV, uint32_t vtxIndex, bool bTexture)
{
    TLITVERTEX &v = g_vtxBuffer[vtxIndex];
    g_vtxProjected5[vtxIndex][0] = g_vtxTransformed[dwV].x;
    g_vtxProjected5[vtxIndex][1] = g_vtxTransformed[dwV].y;
    g_vtxProjected5[vtxIndex][2] = g_vtxTransformed[dwV].z;
    g_vtxProjected5[vtxIndex][3] = g_vtxTransformed[dwV].w;
    g_vtxProjected5[vtxIndex][4] = g_fFogCoord[dwV];

    g_vtxIndex[vtxIndex] = vtxIndex;

    if( options.bOGLVertexClipper == true )
    {
        InitVertex_notopengl_or_clipper_adjust(v, dwV);
    }

    v.dcDiffuse = g_dwVtxDifColor[dwV];
    if( gRDP.otherMode.key_en )
    {
        v.dcDiffuse &= 0x00FFFFFF;
        v.dcDiffuse |= (gRDP.keyA<<24);
    }
    else if( gRDP.otherMode.aa_en && gRDP.otherMode.clr_on_cvg==0 )
    {
        v.dcDiffuse |= 0xFF000000;
    }

    if( gRSP.bProcessDiffuseColor )
    {
        v.dcDiffuse = CRender::g_pRender->PostProcessDiffuseColor(v.dcDiffuse);
    }
    if( options.bWinFrameMode )
    {
        v.dcDiffuse = g_dwVtxDifColor[dwV];
    }

    g_oglVtxColors[vtxIndex][0] = v.r;
    g_oglVtxColors[vtxIndex][1] = v.g;
    g_oglVtxColors[vtxIndex][2] = v.b;
    g_oglVtxColors[vtxIndex][3] = v.a;

    if( bTexture )
    {
        // If the vert is already lit, then there is no normal (and hence we can't generate tex coord)
        // Only scale if not generated automatically
        if (gRSP.bTextureGen && gRSP.bLightingEnable)
            InitVertex_texgen_correct(v, dwV);
        else
        {
            TexCord tex0;
            multiply_subtract2(&tex0.u, &g_fVtxTxtCoords[dwV].x, &gRSP.tex0scaleX, &gRSP.tex0OffsetX);

            if( CRender::g_pRender->IsTexel1Enable() )
            {
                TexCord tex1;
                multiply_subtract2(&tex1.u, &g_fVtxTxtCoords[dwV].x, &gRSP.tex1scaleX, &gRSP.tex1OffsetX);

                CRender::g_pRender->SetVertexTextureUVCoord(v, tex0, tex1);
            }
            else
                CRender::g_pRender->SetVertexTextureUVCoord(v, tex0);
        }

        if(g_curRomInfo.bTextureScaleHack && !bHalfTxtScale)
            InitVertex_scale_hack_check(dwV);
    }
}

uint32_t LightVert(XVECTOR4 & norm, int vidx)
{
    float fCosT;
    /* Do ambient */
    float r = gRSP.fAmbientLightR;
    float g = gRSP.fAmbientLightG;
    float b = gRSP.fAmbientLightB;

    if( options.enableHackForGames != HACK_FOR_ZELDA_MM )
    {
        for (unsigned int l=0; l < gSP.numLights; l++)
        {
            fCosT = norm.x*gRSPlights[l].x + norm.y*gRSPlights[l].y + norm.z*gRSPlights[l].z; 

            if (fCosT > 0 )
            {
                r += gRSPlights[l].fr * fCosT;
                g += gRSPlights[l].fg * fCosT;
                b += gRSPlights[l].fb * fCosT;
            }
        }
    }
    else
    {
        XVECTOR4 v;
        unsigned int l;
        bool transformed = false;

        for (l=0; l < gSP.numLights; l++)
        {
            if( gRSPlights[l].range == 0 )
            {
                // Regular directional light
                fCosT = norm.x*gRSPlights[l].x + norm.y*gRSPlights[l].y + norm.z*gRSPlights[l].z; 

                if (fCosT > 0 )
                {
                    r += gRSPlights[l].fr * fCosT;
                    g += gRSPlights[l].fg * fCosT;
                    b += gRSPlights[l].fb * fCosT;
                }
            }
            else //if( (gRSPlights[l].col&0x00FFFFFF) != 0x00FFFFFF )
            {
                // Point light
                if( !transformed )
                {
                    Vec3Transform(&v, (XVECTOR3*)&g_vtxNonTransformed[vidx], &gRSPmodelViewTop);    // Convert to w=1
                    transformed = true;
                }

                XVECTOR3 dir(gRSPlights[l].x - v.x, gRSPlights[l].y - v.y, gRSPlights[l].z - v.z);
                //XVECTOR3 dir(v.x-gRSPlights[l].x, v.y-gRSPlights[l].y, v.z-gRSPlights[l].z);
                float d2 = sqrtf(dir.x*dir.x+dir.y*dir.y+dir.z*dir.z);
                dir.x /= d2;
                dir.y /= d2;
                dir.z /= d2;

                fCosT = norm.x*dir.x + norm.y*dir.y + norm.z*dir.z; 

                if (fCosT > 0 )
                {
                    //float f = d2/gRSPlights[l].range*50;
                    float f = d2/15000*50;
                    f = 1 - MIN(f,1);
                    fCosT *= f*f;

                    r += gRSPlights[l].fr * fCosT;
                    g += gRSPlights[l].fg * fCosT;
                    b += gRSPlights[l].fb * fCosT;
                }
            }
        }
    }

    if (r > 255) r = 255;
    if (g > 255) g = 255;
    if (b > 255) b = 255;
    return ((0xff000000)|(((uint32_t)r)<<16)|(((uint32_t)g)<<8)|((uint32_t)b));
}

uint32_t LightVertNew(XVECTOR4 & norm)
{
    unsigned int l;
    /* Do ambient */
    float r = gRSP.fAmbientLightR;
    float g = gRSP.fAmbientLightG;
    float b = gRSP.fAmbientLightB;


    for (l=0; l < gSP.numLights; l++)
    {
        float fCosT = norm.x*gRSPlights[l].tx + norm.y*gRSPlights[l].ty + norm.z*gRSPlights[l].tz; 

        if (fCosT > 0 )
        {
            r += gRSPlights[l].fr * fCosT;
            g += gRSPlights[l].fg * fCosT;
            b += gRSPlights[l].fb * fCosT;
        }
    }

    if (r > 255) r = 255;
    if (g > 255) g = 255;
    if (b > 255) b = 255;
    return ((0xff000000)|(((uint32_t)r)<<16)|(((uint32_t)g)<<8)|((uint32_t)b));
}


float zero = 0.0f;
float onef = 1.0f;
float fcosT;

void ReplaceAlphaWithFogFactor(int i)
{
    if( gSP.geometryMode & G_FOG )
    {
        // Use fog factor to replace vertex alpha
        if( g_vecProjected[i].z > 1 )
            *(((uint8_t*)&(g_dwVtxDifColor[i]))+3) = 0xFF;
        if( g_vecProjected[i].z < 0 )
            *(((uint8_t*)&(g_dwVtxDifColor[i]))+3) = 0;
        else
            *(((uint8_t*)&(g_dwVtxDifColor[i]))+3) = (uint8_t)(g_vecProjected[i].z*255);    
    }
}


// Bits
// +-+-+-
// xxyyzz
#define Z_NEG  0x01
#define Z_POS  0x02
#define Y_NEG  0x04
#define Y_POS  0x08
#define X_NEG  0x10
#define X_POS  0x20

// Assumes dwAddr has already been checked! 
// Don't inline - it's too big with the transform macros

void ProcessVertexDataNoSSE(uint32_t dwAddr, uint32_t dwV0, uint32_t dwNum)
{

    UpdateCombinedMatrix();

    // This function is called upon SPvertex
    // - do vertex matrix transform
    // - do vertex lighting
    // - do texture coordinate transform if needed
    // - calculate normal vector

    // Output:  - g_vecProjected[i]             -> transformed vertex x,y,z
    //          - g_vecProjected[i].w           -> saved vertex 1/w
    //          - g_dwVtxFlags[i]               -> flags
    //          - g_dwVtxDifColor[i]            -> vertex color
    //          - g_fVtxTxtCoords[i]            -> vertex texture coordinates

    uint8_t *rdram_u8 = (uint8_t*)gfx_info.RDRAM;
    FiddledVtx * pVtxBase = (FiddledVtx*)(rdram_u8 + dwAddr);
    g_pVtxBase = pVtxBase;

    for (uint32_t i = dwV0; i < dwV0 + dwNum; i++)
    {
        SP_Timing(RSP_GBI0_Vtx);

        FiddledVtx & vert = pVtxBase[i - dwV0];

        g_vtxNonTransformed[i].x = (float)vert.x;
        g_vtxNonTransformed[i].y = (float)vert.y;
        g_vtxNonTransformed[i].z = (float)vert.z;

        Vec3Transform(&g_vtxTransformed[i], (XVECTOR3*)&g_vtxNonTransformed[i], &gRSPworldProject); // Convert to w=1

        g_vecProjected[i].w = 1.0f / g_vtxTransformed[i].w;
        g_vecProjected[i].x = g_vtxTransformed[i].x * g_vecProjected[i].w;
        g_vecProjected[i].y = g_vtxTransformed[i].y * g_vecProjected[i].w;
        if ((g_curRomInfo.bPrimaryDepthHack || options.enableHackForGames == HACK_FOR_NASCAR ) && gRDP.otherMode.depth_source )
        {
            g_vecProjected[i].z = gRDP.fPrimitiveDepth;
            g_vtxTransformed[i].z = gRDP.fPrimitiveDepth*g_vtxTransformed[i].w;
        }
        else
        {
            g_vecProjected[i].z = g_vtxTransformed[i].z * g_vecProjected[i].w;
        }

        if( gRSP.bFogEnabled )
        {
            g_fFogCoord[i] = g_vecProjected[i].z;
            if( g_vecProjected[i].w < 0 || g_vecProjected[i].z < 0 || g_fFogCoord[i] < gRSPfFogMin )
                g_fFogCoord[i] = gRSPfFogMin;
        }

        RSP_Vtx_Clipping(i);

        if( gRSP.bLightingEnable )
        {
            g_normal.x = (float)vert.norma.nx;
            g_normal.y = (float)vert.norma.ny;
            g_normal.z = (float)vert.norma.nz;

            Vec3TransformNormal(g_normal, gRSPmodelViewTop);
            g_dwVtxDifColor[i] = LightVert(g_normal, i);
            *(((uint8_t*)&(g_dwVtxDifColor[i]))+3) = vert.rgba.a; // still use alpha from the vertex
        }
        else
        {
            if( (gSP.geometryMode & G_SHADE) == 0 && gRSP.ucode < 5 )  //Shade is disabled
            {
                //FLAT shade
                g_dwVtxDifColor[i] = gRDP.primitiveColor;
            }
            else
            {
                IColor &color = *(IColor*)&g_dwVtxDifColor[i];
                color.b = vert.rgba.r;
                color.g = vert.rgba.g;
                color.r = vert.rgba.b;
                color.a = vert.rgba.a;
            }
        }

        if( options.bWinFrameMode )
        {
            g_dwVtxDifColor[i] = COLOR_RGBA(vert.rgba.r, vert.rgba.g, vert.rgba.b, vert.rgba.a);
        }

        ReplaceAlphaWithFogFactor(i);

        // Update texture coords n.b. need to divide tu/tv by bogus scale on addition to buffer

        // If the vertex is already lit, then there is no normal (and hence we
        // can't generate tex coord)
        if (gRSP.bTextureGen && gRSP.bLightingEnable )
        {
            TexGen(g_fVtxTxtCoords[i].x, g_fVtxTxtCoords[i].y);
        }
        else
        {
            g_fVtxTxtCoords[i].x = (float)vert.tu;
            g_fVtxTxtCoords[i].y = (float)vert.tv; 
        }
    }
}

bool PrepareTriangle(uint32_t dwV0, uint32_t dwV1, uint32_t dwV2)
{
   SP_Timing(SP_Each_Triangle);

   bool textureFlag = (CRender::g_pRender->IsTextureEnabled() || gRSP.ucode == 6 );

   InitVertex(dwV0, gRSP.numVertices, textureFlag);
   InitVertex(dwV1, gRSP.numVertices+1, textureFlag);
   InitVertex(dwV2, gRSP.numVertices+2, textureFlag);

        if(gRSP.numVertices == 0 && g_curRomInfo.bEnableTxtLOD && gRDP.otherMode.text_lod)
        {
            if( CRender::g_pRender->IsTexel1Enable() && CRender::g_pRender->m_pColorCombiner->m_pDecodedMux->IsUsed(MUX_LODFRAC, MUX_MASK) )
            {
                ComputeLOD();
            }
            else
            {
                gRDP.LODFrac = 0;
            }
        }

   gRSP.numVertices += 3;
   status.dwNumTrisRendered++;

   return true;
}



// Returns true if it thinks the triangle is visible
// Returns false if it is clipped
bool IsTriangleVisible(uint32_t dwV0, uint32_t dwV1, uint32_t dwV2)
{
    //return true;  //fix me

    // Here we AND all the flags. If any of the bits is set for all
    // 3 vertices, it means that all three x, y or z lie outside of
    // the current viewing volume.
    // Currently disabled - still seems a bit dodgy
    if ((gRSP.bCullFront || gRSP.bCullBack) && gRDP.otherMode.zmode != 3)
    {
        XVECTOR4 & v0 = g_vecProjected[dwV0];
        XVECTOR4 & v1 = g_vecProjected[dwV1];
        XVECTOR4 & v2 = g_vecProjected[dwV2];

        // Only try to clip if the tri is onscreen. For some reason, this
        // method doesn't work well when the z value is outside of screenspace
        //if (v0.z < 1 && v1.z < 1 && v2.z < 1)
        {
           float V1 = v2.x - v0.x;
           float V2 = v2.y - v0.y;

           float W1 = v2.x - v1.x;
           float W2 = v2.y - v1.y;

           float fDirection = ((V1 * W2) - (V2 * W1)) * v1.w * v2.w * v0.w;

           if (fDirection < 0 && gRSP.bCullBack)
           {
              status.dwNumTrisClipped++;
              return false;
           }
           else if (fDirection > 0 && gRSP.bCullFront)
           {
              status.dwNumTrisClipped++;
              return false;
           }
        }
    }
    
#ifdef ENABLE_CLIP_TRI
    //if( gRSP.bRejectVtx && (g_clipFlag[dwV0]|g_clipFlag[dwV1]|g_clipFlag[dwV2]) ) 
    //  return;
    if( g_clipFlag2[dwV0]&g_clipFlag2[dwV1]&g_clipFlag2[dwV2] )
    {
        //DebuggerAppendMsg("Clipped");
        return false;
    }
#endif

    return true;
}


void SetPrimitiveColor(uint32_t dwCol, uint32_t LODMin, uint32_t LODFrac)
{
    gRDP.colorsAreReloaded = true;
    gRDP.primitiveColor = dwCol;
    gRDP.primLODMin = LODMin;
    gRDP.primLODFrac = LODFrac;
    if( gRDP.primLODFrac < gRDP.primLODMin )
    {
        gRDP.primLODFrac = gRDP.primLODMin;
    }

    gRDP.fvPrimitiveColor[0] = ((dwCol>>16)&0xFF)/255.0f;  // R
    gRDP.fvPrimitiveColor[1] = ((dwCol>>8)&0xFF)/255.0f;   // G
    gRDP.fvPrimitiveColor[2] = ((dwCol)&0xFF)/255.0f;      // B
    gRDP.fvPrimitiveColor[3] = ((dwCol>>24)&0xFF)/255.0f;  // A
}

void SetPrimitiveDepth(uint32_t z, uint32_t dwDZ)
{
    gRDP.primitiveDepth = z & 0x7FFF;
    gRDP.fPrimitiveDepth = (float)(gRDP.primitiveDepth)/(float)0x8000;

    //gRDP.fPrimitiveDepth = gRDP.fPrimitiveDepth*2-1;  
    /*
    z=0xFFFF    ->   1  the farthest
    z=0         ->  -1  the nearest
    */

    // TODO: How to use dwDZ?
}

void SetVertexXYZ(uint32_t vertex, float x, float y, float z)
{
    g_vecProjected[vertex].x = x;
    g_vecProjected[vertex].y = y;
    g_vecProjected[vertex].z = z;

    g_vtxTransformed[vertex].x = x*g_vtxTransformed[vertex].w;
    g_vtxTransformed[vertex].y = y*g_vtxTransformed[vertex].w;
    g_vtxTransformed[vertex].z = z*g_vtxTransformed[vertex].w;
}

void ProcessVertexDataDKR(uint32_t dwAddr, uint32_t dwV0, uint32_t dwNum)
{
    UpdateCombinedMatrix();

    uint8_t *rdram_u8 = (uint8_t*)gfx_info.RDRAM;
    long long pVtxBase = (long long) (rdram_u8 + dwAddr);
    g_pVtxBase = (FiddledVtx*)pVtxBase;

    Matrix &matWorldProject = gRSP.DKRMatrixes[gRSP.DKRCMatrixIndex];

    bool addbase=false;
    if ((!gRSP.DKRBillBoard) || (gRSP.DKRCMatrixIndex != 2) )
        addbase = false;
    else
        addbase = true;

    if( addbase && gRSP.DKRVtxCount == 0 && dwNum > 1 )
        gRSP.DKRVtxCount++;

    int nOff = 0;
    uint32_t end = dwV0 + dwNum;
    for (uint32_t i = dwV0; i < end; i++)
    {
        XVECTOR3 w;

        g_vtxNonTransformed[i].x = (float)*(short*)((pVtxBase+nOff + 0) ^ 2);
        g_vtxNonTransformed[i].y = (float)*(short*)((pVtxBase+nOff + 2) ^ 2);
        g_vtxNonTransformed[i].z = (float)*(short*)((pVtxBase+nOff + 4) ^ 2);

        Vec3Transform(&g_vtxTransformed[i], (XVECTOR3*)&g_vtxNonTransformed[i], &matWorldProject);  // Convert to w=1

        if( gRSP.DKRVtxCount == 0 && dwNum==1 )
        {
            gRSP.DKRBaseVec.x = g_vtxTransformed[i].x;
            gRSP.DKRBaseVec.y = g_vtxTransformed[i].y;
            gRSP.DKRBaseVec.z = g_vtxTransformed[i].z;
            gRSP.DKRBaseVec.w = g_vtxTransformed[i].w;
        }
        else if( addbase )
        {
            g_vtxTransformed[i].x += gRSP.DKRBaseVec.x;
            g_vtxTransformed[i].y += gRSP.DKRBaseVec.y;
            g_vtxTransformed[i].z += gRSP.DKRBaseVec.z;
            g_vtxTransformed[i].w  = gRSP.DKRBaseVec.w;
        }

        g_vecProjected[i].w = 1.0f / g_vtxTransformed[i].w;
        g_vecProjected[i].x = g_vtxTransformed[i].x * g_vecProjected[i].w;
        g_vecProjected[i].y = g_vtxTransformed[i].y * g_vecProjected[i].w;
        g_vecProjected[i].z = g_vtxTransformed[i].z * g_vecProjected[i].w;

        gRSP.DKRVtxCount++;

        if( gRSP.bFogEnabled )
        {
            g_fFogCoord[i] = g_vecProjected[i].z;
            if( g_vecProjected[i].w < 0 || g_vecProjected[i].z < 0 || g_fFogCoord[i] < gRSPfFogMin )
                g_fFogCoord[i] = gRSPfFogMin;
        }

        RSP_Vtx_Clipping(i);

        short wA = *(short*)((pVtxBase+nOff + 6) ^ 2);
        short wB = *(short*)((pVtxBase+nOff + 8) ^ 2);

        int8_t r = (int8_t)(wA >> 8);
        int8_t g = (int8_t)(wA);
        int8_t b = (int8_t)(wB >> 8);
        int8_t a = (int8_t)(wB);

        if (gRSP.bLightingEnable)
        {
            g_normal.x = (char)r; //norma.nx;
            g_normal.y = (char)g; //norma.ny;
            g_normal.z = (char)b; //norma.nz;

            Vec3TransformNormal(g_normal, matWorldProject)
                g_dwVtxDifColor[i] = LightVert(g_normal, i);
        }
        else   // Assign true vert colour after lighting/fogging
            g_dwVtxDifColor[i] = COLOR_RGBA(r, g, b, a);

        ReplaceAlphaWithFogFactor(i);

        g_fVtxTxtCoords[i].x = g_fVtxTxtCoords[i].y = 1;

        nOff += 10;
    }
}

extern uint32_t dwPDCIAddr;
extern uint32_t dwConkerVtxZAddr;

void ProcessVertexDataConker(uint32_t dwAddr, uint32_t dwV0, uint32_t dwNum)
{
    UpdateCombinedMatrix();

    uint8_t *rdram_u8 = (uint8_t*)gfx_info.RDRAM;
    FiddledVtx * pVtxBase = (FiddledVtx*)(rdram_u8 + dwAddr);
    g_pVtxBase = pVtxBase;

    for (uint32_t i = dwV0; i < dwV0 + dwNum; i++)
    {
        SP_Timing(RSP_GBI0_Vtx);

        FiddledVtx & vert = pVtxBase[i - dwV0];

        g_vtxNonTransformed[i].x = (float)vert.x;
        g_vtxNonTransformed[i].y = (float)vert.y;
        g_vtxNonTransformed[i].z = (float)vert.z;

        {
            Vec3Transform(&g_vtxTransformed[i], (XVECTOR3*)&g_vtxNonTransformed[i], &gRSPworldProject); // Convert to w=1
            g_vecProjected[i].w = 1.0f / g_vtxTransformed[i].w;
            g_vecProjected[i].x = g_vtxTransformed[i].x * g_vecProjected[i].w;
            g_vecProjected[i].y = g_vtxTransformed[i].y * g_vecProjected[i].w;
            g_vecProjected[i].z = g_vtxTransformed[i].z * g_vecProjected[i].w;
        }

        g_fFogCoord[i] = g_vecProjected[i].z;
        if( g_vecProjected[i].w < 0 || g_vecProjected[i].z < 0 || g_fFogCoord[i] < gRSPfFogMin )
            g_fFogCoord[i] = gRSPfFogMin;

        RSP_Vtx_Clipping(i);

        if( gRSP.bLightingEnable )
        {
            {
                uint32_t r= ((gRSP.ambientLightColor>>16)&0xFF);
                uint32_t g= ((gRSP.ambientLightColor>> 8)&0xFF);
                uint32_t b= ((gRSP.ambientLightColor    )&0xFF);
                for( uint32_t k=1; k<= gSP.numLights; k++)
                {
                    r += gRSPlights[k].r;
                    g += gRSPlights[k].g;
                    b += gRSPlights[k].b;
                }
                if( r>255 ) r=255;
                if( g>255 ) g=255;
                if( b>255 ) b=255;
                r *= vert.rgba.r ;
                g *= vert.rgba.g ;
                b *= vert.rgba.b ;
                r >>= 8;
                g >>= 8;
                b >>= 8;
                g_dwVtxDifColor[i] = 0xFF000000;
                g_dwVtxDifColor[i] |= (r<<16);
                g_dwVtxDifColor[i] |= (g<< 8);
                g_dwVtxDifColor[i] |= (b    );          
            }

            *(((uint8_t*)&(g_dwVtxDifColor[i]))+3) = vert.rgba.a; // still use alpha from the vertex
        }
        else
        {
            if( (gSP.geometryMode & G_SHADE) == 0 && gRSP.ucode < 5 )  //Shade is disabled
                g_dwVtxDifColor[i] = gRDP.primitiveColor;
            else    //FLAT shade
                g_dwVtxDifColor[i] = COLOR_RGBA(vert.rgba.r, vert.rgba.g, vert.rgba.b, vert.rgba.a);
        }

        if( options.bWinFrameMode )
        {
            //g_vecProjected[i].z = 0;
            g_dwVtxDifColor[i] = COLOR_RGBA(vert.rgba.r, vert.rgba.g, vert.rgba.b, vert.rgba.a);
        }

        ReplaceAlphaWithFogFactor(i);

        // Update texture coords n.b. need to divide tu/tv by bogus scale on addition to buffer
        //VECTOR2 & t = g_fVtxTxtCoords[i];

        // If the vert is already lit, then there is no normal (and hence we
        // can't generate tex coord)
        if (gRSP.bTextureGen && gRSP.bLightingEnable )
        {
                g_normal.x = (float)*(int8_t*)(rdram_u8 + (((i<<1)+0)^3)+dwConkerVtxZAddr);
                g_normal.y = (float)*(int8_t*)(rdram_u8 + (((i<<1)+1)^3)+dwConkerVtxZAddr);
                g_normal.z = (float)*(int8_t*)(rdram_u8 + (((i<<1)+2)^3)+dwConkerVtxZAddr);
                Vec3TransformNormal(g_normal, gRSPmodelViewTop);
                TexGen(g_fVtxTxtCoords[i].x, g_fVtxTxtCoords[i].y);
        }
        else
        {
            g_fVtxTxtCoords[i].x = (float)vert.tu;
            g_fVtxTxtCoords[i].y = (float)vert.tv; 
        }
    }
}


typedef struct
{
    short y;
    short x;
    short flag;
    short z;
} RS_Vtx_XYZ;

typedef union
{
    struct
    {
        uint8_t a;
        uint8_t b;
        uint8_t g;
        uint8_t r;
    };
    
    struct
    {
        char na;    // A
        char nz;    // B
        char ny;    // G
        char nx;    // R
    };
} RS_Vtx_Color;


void ProcessVertexData_Rogue_Squadron(uint32_t dwXYZAddr, uint32_t dwColorAddr, uint32_t dwXYZCmd, uint32_t dwColorCmd)
{
    UpdateCombinedMatrix();

    uint8_t *rdram_u8 = (uint8_t*)gfx_info.RDRAM;
    uint32_t dwV0 = 0;
    uint32_t dwNum = (dwXYZCmd&0xFF00)>>10;

    RS_Vtx_XYZ * pVtxXYZBase = (RS_Vtx_XYZ*)(rdram_u8 + dwXYZAddr);
    RS_Vtx_Color * pVtxColorBase = (RS_Vtx_Color*)(rdram_u8 + dwColorAddr);

    for (uint32_t i = dwV0; i < dwV0 + dwNum; i++)
    {
        RS_Vtx_XYZ & vertxyz = pVtxXYZBase[i - dwV0];
        RS_Vtx_Color & vertcolors = pVtxColorBase[i - dwV0];

        g_vtxNonTransformed[i].x = (float)vertxyz.x;
        g_vtxNonTransformed[i].y = (float)vertxyz.y;
        g_vtxNonTransformed[i].z = (float)vertxyz.z;

        {
            Vec3Transform(&g_vtxTransformed[i], (XVECTOR3*)&g_vtxNonTransformed[i], &gRSPworldProject); // Convert to w=1
            g_vecProjected[i].w = 1.0f / g_vtxTransformed[i].w;
            g_vecProjected[i].x = g_vtxTransformed[i].x * g_vecProjected[i].w;
            g_vecProjected[i].y = g_vtxTransformed[i].y * g_vecProjected[i].w;
            g_vecProjected[i].z = g_vtxTransformed[i].z * g_vecProjected[i].w;
        }

        g_fFogCoord[i] = g_vecProjected[i].z;
        if( g_vecProjected[i].w < 0 || g_vecProjected[i].z < 0 || g_fFogCoord[i] < gRSPfFogMin )
            g_fFogCoord[i] = gRSPfFogMin;

        RSP_Vtx_Clipping(i);

        if( gRSP.bLightingEnable )
        {
            g_normal.x = (float)vertcolors.nx;
            g_normal.y = (float)vertcolors.ny;
            g_normal.z = (float)vertcolors.nz;

            {
                Vec3TransformNormal(g_normal, gRSPmodelViewTop);
                g_dwVtxDifColor[i] = LightVert(g_normal, i);
            }
            *(((uint8_t*)&(g_dwVtxDifColor[i]))+3) = vertcolors.a;    // still use alpha from the vertex
        }
        else
        {
            if( (gSP.geometryMode & G_SHADE) == 0 && gRSP.ucode < 5 )  //Shade is disabled
            {
                g_dwVtxDifColor[i] = gRDP.primitiveColor;
            }
            else    //FLAT shade
            {
                g_dwVtxDifColor[i] = COLOR_RGBA(vertcolors.r, vertcolors.g, vertcolors.b, vertcolors.a);
            }
        }

        if( options.bWinFrameMode )
        {
            g_dwVtxDifColor[i] = COLOR_RGBA(vertcolors.r, vertcolors.g, vertcolors.b, vertcolors.a);
        }

        ReplaceAlphaWithFogFactor(i);

        /*
        // Update texture coords n.b. need to divide tu/tv by bogus scale on addition to buffer
        VECTOR2 & t = g_fVtxTxtCoords[i];

        // If the vert is already lit, then there is no normal (and hence we
        // can't generate tex coord)
        if (gRSP.bTextureGen && gRSP.bLightingEnable && g_textures[gRSP.curTile].m_bTextureEnable )
        {
            TexGen(g_fVtxTxtCoords[i].x, g_fVtxTxtCoords[i].y);
        }
        else
        {
            t.x = (float)vert.tu;
            t.y = (float)vert.tv; 
        }
        */
    }
}

void SetLightDirection(uint32_t dwLight, float x, float y, float z, float range)
{
    float w = 1;
    //gRSP.bLightIsUpdated = true;

    //gRSPlights[dwLight].ox = x;
    //gRSPlights[dwLight].oy = y;
    //gRSPlights[dwLight].oz = z;

    if (range == 0)
       w = (float)sqrt(x*x+y*y+z*z);

    gRSPlights[dwLight].x = x/w;
    gRSPlights[dwLight].y = y/w;
    gRSPlights[dwLight].z = z/w;
    gRSPlights[dwLight].range = range;
}

static float maxS0, maxT0;
static float maxS1, maxT1;
static bool validS0, validT0;
static bool validS1, validT1;

void LogTextureCoords(float fTex0S, float fTex0T, float fTex1S, float fTex1T)
{
    if( validS0 )
    {
        if( fTex0S<0 || fTex0S>maxS0 )  validS0 = false;
    }
    if( validT0 )
    {
        if( fTex0T<0 || fTex0T>maxT0 )  validT0 = false;
    }
    if( validS1 )
    {
        if( fTex1S<0 || fTex1S>maxS1 )  validS1 = false;
    }
    if( validT1 )
    {
        if( fTex1T<0 || fTex1T>maxT1 )  validT1 = false;
    }
}

bool CheckTextureCoords(int tex)
{
    if( tex==0 )
        return validS0&&validT0;

    return validS1&&validT1;
}

void ResetTextureCoordsLog(float maxs0, float maxt0, float maxs1, float maxt1)
{
    maxS0 = maxs0;
    maxT0 = maxt0;
    maxS1 = maxs1;
    maxT1 = maxt1;
    validS0 = validT0 = true;
    validS1 = validT1 = true;
}

void ForceMainTextureIndex(int dwTile) 
{
   // Hack
   if( dwTile == 1 && !(CRender::g_pRender->IsTexel0Enable()) && CRender::g_pRender->IsTexel1Enable() )
      gRSP.curTile = 0;
   else
      gRSP.curTile = dwTile;
}

float HackZ2(float z)
{
    z = (z+9)/10;
    return z;
}

float HackZ(float z)
{
   /* TODO - investigate
    * should we just do this instead?
    * z = HackZ2(z); 
    */
    return HackZ2(z);

    if( z < 0.1 && z >= 0 )
        z = (.1f+z)/2;
    else if( z < 0 )
        //return (10+z)/100;
        z = (expf(z)/20);
    return z;
}

void HackZ(std::vector<XVECTOR3>& points)
{
    int size = points.size();
    for( int i=0; i<size; i++)
    {
        XVECTOR3 &v = points[i];
        v.z = (float)HackZ(v.z);
    }
}

void HackZAll()
{
    if( CDeviceBuilder::m_deviceGeneralType == DIRECTX_DEVICE )
    {
        for( uint32_t i=0; i<gRSP.numVertices; i++)
            g_vtxBuffer[i].z = HackZ(g_vtxBuffer[i].z);
    }
    else
    {
        for( uint32_t i=0; i<gRSP.numVertices; i++)
        {
            float w = g_vtxProjected5[i][3];
            g_vtxProjected5[i][2] = HackZ(g_vtxProjected5[i][2]/w)*w;
        }
    }
}


extern XMATRIX reverseXY;
extern XMATRIX reverseY;

void UpdateCombinedMatrix()
{
    if( gRSP.bMatrixIsUpdated )
    {
        gRSPworldProject = gRSP.modelviewMtxs[gRSP.modelViewMtxTop] * gRSP.projectionMtxs[gRSP.projectionMtxTop];
        gRSP.bMatrixIsUpdated = false;
        gRSP.bCombinedMatrixIsUpdated = true;
    }

    if( gRSP.bCombinedMatrixIsUpdated )
    {
        if( options.enableHackForGames == HACK_REVERSE_XY_COOR )
            gRSPworldProject = gRSPworldProject * reverseXY;
        if( options.enableHackForGames == HACK_REVERSE_Y_COOR )
            gRSPworldProject = gRSPworldProject * reverseY;
        gRSP.bCombinedMatrixIsUpdated = false;
    }

    //if( gRSP.bWorldMatrixIsUpdated || gRSP.bLightIsUpdated )
    //{
    //  // Update lights with transported world matrix
    //  for( unsigned int l=0; l< gSP.numLights; l++)
    //  {
    //      Vec3TransformCoord(&gRSPlights[l].td, &gRSPlights[l].od, &gRSPmodelViewTopTranspose);
    //      Vec3Normalize(&gRSPlights[l].td,&gRSPlights[l].td);
    //  }

    //  gRSP.bWorldMatrixIsUpdated = false;
    //  gRSP.bLightIsUpdated = false;
    //}
}

