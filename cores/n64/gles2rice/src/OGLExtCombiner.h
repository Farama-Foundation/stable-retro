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

#ifndef _OGLEXT_COMBINER_H_
#define _OGLEXT_COMBINER_H_

#include <vector>

#include "osal_opengl.h"

#include "OGLCombiner.h"

typedef union 
{
    struct
    {
        uint8_t   arg0;
        uint8_t   arg1;
        uint8_t   arg2;
    };
    uint8_t args[3];
} OGLExt1CombType;

typedef struct
{
    union
    {
        struct
        {
            GLenum  rgbOp;
            GLenum  alphaOp;
        };

        GLenum ops[2];
    };

    union
    {
        struct
        {
            uint8_t   rgbArg0;
            uint8_t   rgbArg1;
            uint8_t   rgbArg2;
            uint8_t   alphaArg0;
            uint8_t   alphaArg1;
            uint8_t   alphaArg2;
        };

        struct
        {
            OGLExt1CombType rgbComb;
            OGLExt1CombType alphaComb;
        };

        OGLExt1CombType Combs[2];
    };

    union
    {
        struct
        {
            GLint   rgbArg0gl;
            GLint   rgbArg1gl;
            GLint   rgbArg2gl;
        };

        GLint glRGBArgs[3];
    };

    union
    {
        struct
        {
            GLint   rgbFlag0gl;
            GLint   rgbFlag1gl;
            GLint   rgbFlag2gl;
        };

        GLint glRGBFlags[3];
    };

    union
    {
        struct
        {
            GLint   alphaArg0gl;
            GLint   alphaArg1gl;
            GLint   alphaArg2gl;
        };

        GLint glAlphaArgs[3];
    };

    union
    {
        struct
        {
            GLint   alphaFlag0gl;
            GLint   alphaFlag1gl;
            GLint   alphaFlag2gl;
        };

        GLint glAlphaFlags[3];
    };

    int     tex;
    bool    textureIsUsed;
    //float scale;      //Will not be used
} OGLExtCombinerType;

typedef struct
{
    uint32_t  dwMux0;
    uint32_t  dwMux1;
    OGLExtCombinerType units[8];
    int     numOfUnits;
    uint32_t  constantColor;

    // For 1.4 v2 combiner
    bool    primIsUsed;
    bool    envIsUsed;
    bool    lodFracIsUsed;
} OGLExtCombinerSaveType;


//========================================================================
// OpenGL 1.4 combiner which support Texture Crossbar feature
class COGLColorCombiner4 : public COGLColorCombiner
{
public:
    bool Initialize(void);
protected:
    friend class OGLDeviceBuilder;
    void InitCombinerCycle12(void);
    void InitCombinerCycleFill(void);
    virtual void GenerateCombinerSetting(int index);
    virtual void GenerateCombinerSettingConstants(int index);
    virtual int ParseDecodedMux();

    COGLColorCombiner4(CRender *pRender);
    ~COGLColorCombiner4() {};

    GLint m_maxTexUnits;
    int m_lastIndex;
    uint32_t m_dwLastMux0;
    uint32_t m_dwLastMux1;

#ifdef DEBUGGER
    void DisplaySimpleMuxString(void);
#endif

protected:
    virtual int SaveParsedResult(OGLExtCombinerSaveType &result);
    static GLint MapRGBArgFlags(uint8_t arg);
    static GLint MapAlphaArgFlags(uint8_t arg);
    static const char* GetOpStr(GLenum op);
    static GLint RGBArgsMap4[];
    std::vector<OGLExtCombinerSaveType>  m_vCompiledSettings;

private:
    virtual int ParseDecodedMux2Units();
    virtual int FindCompiledMux();

    virtual GLint MapRGBArgs(uint8_t arg);
    virtual GLint MapAlphaArgs(uint8_t arg);
};

#endif

