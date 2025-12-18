/*
Copyright (C) 2002 Rice1964

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

#ifndef _DECODEDMUX_H_
#define _DECODEDMUX_H_

#include <string.h>
#include <stdio.h>

#include "typedefs.h"
#include "CombinerDefs.h"

#define CM_IGNORE 0
#define CM_IGNORE_BYTE 0xFF

typedef enum 
{
    N64Cycle0RGB   = 0,
    N64Cycle0Alpha = 1,
    N64Cycle1RGB   = 2,
    N64Cycle1Alpha = 3,
} N64StageNumberType;

typedef union
{
    struct
    {
        uint32_t dwMux0;
        uint32_t dwMux1;
    };
    uint64_t Mux64;
} MuxType;

typedef struct
{
    MuxType ori_mux;
    MuxType simple_mux;
} SimpleMuxMapType;

class DecodedMux
{
public:
    union
    {
        struct
        {
            uint8_t aRGB0;
            uint8_t bRGB0;
            uint8_t cRGB0;
            uint8_t dRGB0;
            
            uint8_t aA0;
            uint8_t bA0;
            uint8_t cA0;
            uint8_t dA0;
            
            uint8_t aRGB1;
            uint8_t bRGB1;
            uint8_t cRGB1;
            uint8_t dRGB1;
            
            uint8_t aA1;
            uint8_t bA1;
            uint8_t cA1;
            uint8_t dA1;
        };
        uint8_t  m_bytes[16];
        uint32_t m_dWords[4];
        N64CombinerType m_n64Combiners[4];
    };
    
    union
    {
        struct
        {
            uint32_t m_dwMux0;
            uint32_t m_dwMux1;
        };
        uint64_t m_u64Mux;
    };

    CombinerFormatType splitType[4];
    CombinerFormatType mType;
    
    uint32_t m_dwShadeColorChannelFlag;
    uint32_t m_dwShadeAlphaChannelFlag;
    uint32_t m_ColorTextureFlag[2];   // I may use a texture to represent a constant color
                                    // when there are more constant colors are used than    
                                    // the system can support

    bool m_bShadeIsUsed[2];     // 0 for color channel, 1 for alpha channel
    bool m_bTexel0IsUsed;
    bool m_bTexel1IsUsed;

    int  m_maxConstants;    // OpenGL 1.1 does not really support a constant color in combiner
                            // must use shade for constants;
    int  m_maxTextures;     // 1 or 2


    void Decode(uint32_t dwMux0, uint32_t dwMux1);
    virtual void Hack(void);
    bool IsUsed(uint8_t fac, uint8_t mask);
    bool IsUsedInAlphaChannel(uint8_t fac, uint8_t mask);
    bool IsUsedInColorChannel(uint8_t fac, uint8_t mask);
    bool IsUsedInCycle(uint8_t fac, int cycle, CombineChannel channel, uint8_t mask);
    bool IsUsedInCycle(uint8_t fac, int cycle, uint8_t mask);
    uint32_t GetCycle(int cycle, CombineChannel channel);
    uint32_t GetCycle(int cycle);
    CombinerFormatType GetCombinerFormatType(uint32_t cycle);
    void Display(bool simplified, FILE *fp);
    static char* FormatStr(uint8_t val, char *buf);
    void CheckCombineInCycle1(void);
    virtual void Simplify(void);
    virtual void Reformat(bool do_complement);
    virtual void To_AB_Add_CD_Format(void); // Use by TNT,Geforce
    virtual void To_AB_Add_C_Format(void);  // Use by ATI Radeon
    
    virtual void MergeShadeWithConstants(void);
    virtual void MergeShadeWithConstantsInChannel(CombineChannel channel);
    virtual void MergeConstants(void);
    virtual void UseShadeForConstant(void);
    virtual void UseTextureForConstant(void);

    void ConvertComplements();
    int HowManyConstFactors();
    int HowManyTextures();
    void MergeConstFactors();
    virtual void SplitComplexStages();  // Only used if the combiner supports more than 1 stages
    void ConvertLODFracTo0();
    void ReplaceVal(uint8_t val1, uint8_t val2, int cycle, uint8_t mask);
    void Replace1Val(uint8_t &val1, const uint8_t val2, uint8_t mask)
    {
        val1 &= (~mask);
        val1 |= val2;
    }
    int CountTexels(void);
    int Count(uint8_t val, int cycle, uint8_t mask);

#ifdef DEBUGGER
    void DisplayMuxString(const char *prompt);
    void DisplaySimpliedMuxString(const char *prompt);
    void DisplayConstantsWithShade(uint32_t flag,CombineChannel channel);
#else
    void DisplayMuxString(const char *prompt) {}
    void DisplaySimpliedMuxString(const char *prompt){}
    void DisplayConstantsWithShade(uint32_t flag,CombineChannel channel){}
    void LogMuxString(const char *prompt, FILE *fp);
    void LogSimpliedMuxString(const char *prompt, FILE *fp);
    void LogConstantsWithShade(uint32_t flag,CombineChannel channel, FILE *fp);
#endif

    virtual DecodedMux& operator=(const DecodedMux& mux)
    {
        m_dWords[0] = mux.m_dWords[0];
        m_dWords[1] = mux.m_dWords[1];
        m_dWords[2] = mux.m_dWords[2];
        m_dWords[3] = mux.m_dWords[3];
        m_u64Mux = mux.m_u64Mux;
        splitType[0] = mux.splitType[0];
        splitType[1] = mux.splitType[1];
        splitType[2] = mux.splitType[2];
        splitType[3] = mux.splitType[3];
        mType = mux.mType;

        m_dwShadeColorChannelFlag = mux.m_dwShadeColorChannelFlag;
        m_dwShadeAlphaChannelFlag = mux.m_dwShadeAlphaChannelFlag;

        m_bShadeIsUsed[0] = mux.m_bShadeIsUsed[0];
        m_bShadeIsUsed[1] = mux.m_bShadeIsUsed[1];
        m_bTexel0IsUsed = mux.m_bTexel0IsUsed;
        m_bTexel1IsUsed = mux.m_bTexel1IsUsed;

        m_maxConstants = mux.m_maxConstants;
        m_maxTextures = mux.m_maxTextures;
        m_ColorTextureFlag[0] = mux.m_ColorTextureFlag[0];
        m_ColorTextureFlag[1] = mux.m_ColorTextureFlag[1];

        return *this;
    }

    static inline bool IsConstFactor(uint8_t val)
    {
        uint8_t v = val&MUX_MASK;
        return( v == MUX_0 || v == MUX_1 || v == MUX_PRIM || v == MUX_ENV || v == MUX_LODFRAC || v == MUX_PRIMLODFRAC );
    }

    DecodedMux()
    {
        memset(m_bytes, 0, sizeof(m_bytes));
        mType=CM_FMT_TYPE_NOT_CHECKED;
        for( int i=0; i<4; i++ )
        {
            splitType[i] = CM_FMT_TYPE_NOT_CHECKED;
        }
        m_maxConstants = 1;
        m_maxTextures = 2;
    }
    
    virtual ~DecodedMux() {}
};

class DecodedMuxForPixelShader : public DecodedMux
{
public:
    virtual void Simplify(void);
    void SplitComplexStages() {};
};

class DecodedMuxForSemiPixelShader : public DecodedMux
{
public:
    void Reset(void);
};

class DecodedMuxForOGL14V2 : public DecodedMuxForPixelShader
{
public:
    virtual void Simplify(void);
    void UseTextureForConstant(void);
};

typedef struct 
{
    bool bFurtherFormatForOGL2;
    bool bUseShadeForConstants;
    bool bUseTextureForConstants;
    bool bUseMoreThan2TextureForConstants;
    bool bReformatToAB_CD;
    bool bAllowHack;
    bool bAllowComplimentary;
    bool bCheckCombineInCycle1;
    bool bSetLODFracTo0;
    bool bMergeShadeWithConstants;
    bool bSplitComplexStage;
    bool bReformatAgainWithTwoTexels;
} MuxConverterOptions;

#endif


