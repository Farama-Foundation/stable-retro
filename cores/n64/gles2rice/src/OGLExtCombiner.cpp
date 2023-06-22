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

#include <algorithm>
#include "osal_opengl.h"

#include "OGLExtCombiner.h"
#include "OGLExtRender.h"
#include "OGLDecodedMux.h"
#include "OGLTexture.h"

#define GL_MODULATE_ADD_ATI        0x8744
#define GL_MODULATE_SUBTRACT_ATI   0x8746

//========================================================================
COGLColorCombiner4::COGLColorCombiner4(CRender *pRender)
        :COGLColorCombiner(pRender), m_maxTexUnits(0), m_lastIndex(-1),
        m_dwLastMux0(0), m_dwLastMux1(0)
{
    delete m_pDecodedMux;
    m_pDecodedMux = new COGLExtDecodedMux;
}

//////////////////////////////////////////////////////////////////////////
bool COGLColorCombiner4::Initialize(void)
{
    m_maxTexUnits = 1;

    return true;
}

//========================================================================
void COGLColorCombiner4::InitCombinerCycleFill(void)
{
    for( int i=0; i<m_supportedStages; i++ )
    {
        glActiveTexture(GL_TEXTURE0+i);
        m_pOGLRender->EnableTexUnit(i, false);
    }
}

//////////////////////////////////////////////////////////////////////////
void COGLColorCombiner4::InitCombinerCycle12(void)
{
#ifdef DEBUGGER
    if( debuggerDropCombiners )
    {
        UpdateCombiner(m_pDecodedMux->m_dwMux0,m_pDecodedMux->m_dwMux1);
        m_vCompiledSettings.clear();
        m_dwLastMux0 = m_dwLastMux1 = 0;
        debuggerDropCombiners = false;
    }
#endif

    m_pOGLRender->EnableMultiTexture();

    bool combinerIsChanged = false;

    if( m_pDecodedMux->m_dwMux0 != m_dwLastMux0 || m_pDecodedMux->m_dwMux1 != m_dwLastMux1 || m_lastIndex < 0 )
    {
        combinerIsChanged = true;
        m_lastIndex = FindCompiledMux();
        if( m_lastIndex < 0 )       // Can not found
        {
            m_lastIndex = ParseDecodedMux();
#ifdef DEBUGGER
            DisplaySimpleMuxString();
#endif
        }

        m_dwLastMux0 = m_pDecodedMux->m_dwMux0;
        m_dwLastMux1 = m_pDecodedMux->m_dwMux1;
    }
    

    if( m_bCycleChanged || combinerIsChanged || gRDP.texturesAreReloaded || gRDP.colorsAreReloaded )
    {
        if( m_bCycleChanged || combinerIsChanged )
        {
            GenerateCombinerSettingConstants(m_lastIndex);
            GenerateCombinerSetting(m_lastIndex);
        }
        else if( gRDP.colorsAreReloaded )
        {
            GenerateCombinerSettingConstants(m_lastIndex);
        }

        m_pOGLRender->SetAllTexelRepeatFlag();

        gRDP.colorsAreReloaded = false;
        gRDP.texturesAreReloaded = false;
    }
    else
    {
        m_pOGLRender->SetAllTexelRepeatFlag();
    }
}

//////////////////////////////////////////////////////////////////////////
int COGLColorCombiner4::ParseDecodedMux()
{
    return 0;
}

int COGLColorCombiner4::ParseDecodedMux2Units()
{
    OGLExtCombinerSaveType res;
    for( int k=0; k<8; k++ )
        res.units[k].tex = -1;

    res.numOfUnits = 2;

    for( int i=0; i<res.numOfUnits*2; i++ ) // Set combiner for each texture unit
    {
        // For each texture unit, set both RGB and Alpha channel
        // Keep in mind that the m_pDecodeMux has been reformatted and simplified very well

        OGLExtCombinerType &unit = res.units[i/2];
        OGLExt1CombType &comb = unit.Combs[i%2];

        CombinerFormatType type = m_pDecodedMux->splitType[i];
        N64CombinerType &m = m_pDecodedMux->m_n64Combiners[i];

        comb.arg0 = comb.arg1 = comb.arg2 = MUX_0;

        switch( type )
        {
        case CM_FMT_TYPE_NOT_USED:
            comb.arg0 = MUX_COMBINED;
            unit.ops[i%2] = GL_REPLACE;
            break;
        case CM_FMT_TYPE_D:                 // = A
            comb.arg0 = m.d;
            unit.ops[i%2] = GL_REPLACE;
            break;
        default:
            comb.arg0 = m.a;
            comb.arg1 = m.b;
            comb.arg2 = m.c;
            unit.ops[i%2] = GL_INTERPOLATE;
            break;
        }
    }

    if( m_pDecodedMux->splitType[2] == CM_FMT_TYPE_NOT_USED && m_pDecodedMux->splitType[3] == CM_FMT_TYPE_NOT_USED && !m_bTex1Enabled )
    {
        res.numOfUnits = 1;
    }

    res.units[0].tex = 0;
    res.units[1].tex = 1;

    return SaveParsedResult(res);
}

const char* COGLColorCombiner4::GetOpStr(GLenum op)
{
    switch( op )
    {
    case GL_REPLACE:
        return "REPLACE";
    case GL_MODULATE_ADD_ATI:
        return "MULADD";
    default:
        return "SUB";
    }
}

int COGLColorCombiner4::SaveParsedResult(OGLExtCombinerSaveType &result)
{
    result.dwMux0 = m_pDecodedMux->m_dwMux0;
    result.dwMux1 = m_pDecodedMux->m_dwMux1;

    for( int n=0; n<result.numOfUnits; n++ )
    {
        for( int i=0; i<3; i++ )
        {
            result.units[n].glRGBArgs[i] = 0;
            result.units[n].glRGBFlags[i] = 0;
            result.units[n].glAlphaArgs[i] = 0;
            result.units[n].glAlphaFlags[i] = 0;
            if( result.units[n].rgbComb.args[i] != CM_IGNORE_BYTE )
            {
                result.units[n].glRGBArgs[i] = MapRGBArgs(result.units[n].rgbComb.args[i]);
                result.units[n].glRGBFlags[i] = MapRGBArgFlags(result.units[n].rgbComb.args[i]);
            }
            if( result.units[n].alphaComb.args[i] != CM_IGNORE_BYTE )
            {
                result.units[n].glAlphaArgs[i] = MapAlphaArgs(result.units[n].alphaComb.args[i]);
                result.units[n].glAlphaFlags[i] = MapAlphaArgFlags(result.units[n].alphaComb.args[i]);
            }
        }
    }

    m_vCompiledSettings.push_back(result);
    m_lastIndex = m_vCompiledSettings.size()-1;

#ifdef DEBUGGER
    if( logCombiners )
    {
        DisplaySimpleMuxString();
    }
#endif

    return m_lastIndex;
}

bool isGLtex(GLint val)
{
    if( val >= GL_TEXTURE0 && val <= GL_TEXTURE7 )
        return true;
    else
        return false;
}

#ifdef DEBUGGER
extern const char *translatedCombTypes[];
void COGLColorCombiner4::DisplaySimpleMuxString(void)
{
    char buf0[30], buf1[30], buf2[30];
    OGLExtCombinerSaveType &result = m_vCompiledSettings[m_lastIndex];

    COGLColorCombiner::DisplaySimpleMuxString();
    DebuggerAppendMsg("OpenGL 1.2: %d Stages", result.numOfUnits);      
    for( int i=0; i<result.numOfUnits; i++ )
    {
        DebuggerAppendMsg("//aRGB%d:\t%s: %s, %s, %s\n", i,GetOpStr(result.units[i].rgbOp), DecodedMux::FormatStr(result.units[i].rgbArg0,buf0), DecodedMux::FormatStr(result.units[i].rgbArg1,buf1), DecodedMux::FormatStr(result.units[i].rgbArg2,buf2));     
        DebuggerAppendMsg("//aAlpha%d:\t%s: %s, %s, %s\n", i,GetOpStr(result.units[i].alphaOp), DecodedMux::FormatStr(result.units[i].alphaArg0,buf0), DecodedMux::FormatStr(result.units[i].alphaArg1,buf1), DecodedMux::FormatStr(result.units[i].alphaArg2,buf2));       
    }
    TRACE0("\n\n");
}
#endif


//////////////////////////////////////////////////////////////////////////
int COGLColorCombiner4::FindCompiledMux()
{
#ifdef DEBUGGER
    if( debuggerDropCombiners )
    {
        m_vCompiledSettings.clear();
        //m_dwLastMux0 = m_dwLastMux1 = 0;
        debuggerDropCombiners = false;
    }
#endif
    for( uint32_t i=0; i<m_vCompiledSettings.size(); i++ )
    {
        if( m_vCompiledSettings[i].dwMux0 == m_pDecodedMux->m_dwMux0 && m_vCompiledSettings[i].dwMux1 == m_pDecodedMux->m_dwMux1 )
            return (int)i;
    }

    return -1;
}

//========================================================================

GLint COGLColorCombiner4::RGBArgsMap4[] =
{
    GL_TEXTURE0,                //MUX_TEXEL0,
    GL_TEXTURE0,                //MUX_T0_ALPHA,
};

//========================================================================

GLint COGLColorCombiner4::MapRGBArgs(uint8_t arg)
{
    return RGBArgsMap4[arg&MUX_MASK];
}

GLint COGLColorCombiner4::MapRGBArgFlags(uint8_t arg)
{
    if( (arg & MUX_ALPHAREPLICATE) && (arg & MUX_COMPLEMENT) )
    {
        return GL_ONE_MINUS_SRC_ALPHA;
    }
    else if( (arg & MUX_ALPHAREPLICATE) )
    {
        return GL_SRC_ALPHA;
    }
    else if(arg & MUX_COMPLEMENT) 
    {
        return GL_ONE_MINUS_SRC_COLOR;
    }
    else
        return GL_SRC_COLOR;
}

GLint COGLColorCombiner4::MapAlphaArgs(uint8_t arg)
{
    return RGBArgsMap4[arg&MUX_MASK];
}

GLint COGLColorCombiner4::MapAlphaArgFlags(uint8_t arg)
{
    if(arg & MUX_COMPLEMENT) 
    {
        return GL_ONE_MINUS_SRC_ALPHA;
    }
    else
        return GL_SRC_ALPHA;
}

//========================================================================

void ApplyFor1Unit(OGLExtCombinerType &unit)
{
}

//////////////////////////////////////////////////////////////////////////

void COGLColorCombiner4::GenerateCombinerSetting(int index)
{
    OGLExtCombinerSaveType &res = m_vCompiledSettings[index];

    // Texture unit 0
    COGLTexture* pTexture = NULL;
    COGLTexture* pTexture1 = NULL;

    if( m_bTex0Enabled || m_bTex1Enabled || gRDP.otherMode.cycle_type  == G_CYC_COPY )
    {
        if( m_bTex0Enabled || gRDP.otherMode.cycle_type  == G_CYC_COPY )
        {
            pTexture = g_textures[gRSP.curTile].m_pCOGLTexture;
            if( pTexture )  m_pOGLRender->BindTexture(pTexture->m_dwTextureName, 0);
        }
        if( m_bTex1Enabled )
        {
            pTexture1 = g_textures[(gRSP.curTile+1)&7].m_pCOGLTexture;
            if( pTexture1 ) m_pOGLRender->BindTexture(pTexture1->m_dwTextureName, 1);
        }
    }



    for( int i=0; i<res.numOfUnits; i++ )
    {
        glActiveTexture(GL_TEXTURE0 + i);
        m_pOGLRender->EnableTexUnit(i, true);
        glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);
        ApplyFor1Unit(res.units[i]);
    }

    if( res.numOfUnits < m_maxTexUnits )
    {
        for( int i=res.numOfUnits; i<m_maxTexUnits; i++ )
        {
            glActiveTexture(GL_TEXTURE0 + i);
            m_pOGLRender->DisBindTexture(0, i);
            m_pOGLRender->EnableTexUnit(i, false);
        }
    }
}


void COGLColorCombiner4::GenerateCombinerSettingConstants(int index)
{
    OGLExtCombinerSaveType &res = m_vCompiledSettings[index];

    float *fv;
    float tempf[4];

    bool isUsed = true;

    if( res.primIsUsed )
    {
        fv = GetPrimitiveColorfv(); // CONSTANT COLOR
    }
    else if( res.envIsUsed )
    {
        fv = GetEnvColorfv();   // CONSTANT COLOR
    }
    else if( res.lodFracIsUsed )
    {
        float frac = gRDP.LODFrac / 255.0f;
        tempf[0] = tempf[1] = tempf[2] = tempf[3] = frac;
        fv = &tempf[0];
    }
    else
    {
        isUsed = false;
    }

    if( isUsed )
    {
        for( int i=0; i<res.numOfUnits; i++ )
        {
            glActiveTexture(GL_TEXTURE0 + i);
            glTexEnvfv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR,fv);
        }
    }
}


GLenum GeneralToGLMaps[]=
{
    GL_REPLACE,             //CM_REPLACE,
    GL_MODULATE_ADD_ATI,    //CM_MULTIPLYADD,       
};
