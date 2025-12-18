#ifndef SHADERCOMBINER_H
#define SHADERCOMBINER_H

#include "OpenGL.h"

#include "../../Graphics/RDP/gDP_state.h"

#include "Combiner.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SC_POSITION             0
#define SC_COLOR                1
#define SC_TEXCOORD0            2
#define SC_TEXCOORD1            3

#define SC_SetUniform1i(A, B)       glUniform1i(scProgramCurrent->uniforms.A.loc, B)
#define SC_SetUniform1f(A, B)       glUniform1f(scProgramCurrent->uniforms.A.loc, B)
#define SC_SetUniform4fv(A, B)      glUniform4fv(scProgramCurrent->uniforms.A.loc, 1, B)
#define SC_SetUniform2f(A, B, C)    glUniform2f(scProgramCurrent->uniforms.A.loc, B, C)
#define SC_ForceUniform1i(A, B)     glUniform1i(scProgramCurrent->uniforms.A.loc, B)
#define SC_ForceUniform1f(A, B)     glUniform1f(scProgramCurrent->uniforms.A.loc, B)
#define SC_ForceUniform4fv(A, B)    glUniform4fv(scProgramCurrent->uniforms.A.loc, 1, B)
#define SC_ForceUniform2f(A, B, C)  glUniform2f(scProgramCurrent->uniforms.A.loc, B, C)

struct iUniform {GLint loc; int val;};
struct fUniform {GLint loc; float val;};
struct fv2Uniform {GLint loc; float val[2];};
struct fv4Uniform {GLint loc; float val[4];};

typedef struct
{
    struct iUniform uTex0, uTex1, uTexNoise;
    struct iUniform uEnableFog;
    struct fUniform uFogScale, uFogOffset, uAlphaRef, uPrimLODFrac, uRenderState, uK4, uK5;
    struct fv4Uniform uEnvColor, uPrimColor, uFogColor;
    struct fv2Uniform  uTexScale, uTexOffset[2], uCacheShiftScale[2],
        uCacheScale[2], uCacheOffset[2];
} UniformLocation;

typedef struct ShaderProgram
{
    GLint       program;
    GLint       fragment;
    GLint       vertex;
    int         usesT0;       //uses texcoord0 attrib
    int         usesT1;       //uses texcoord1 attrib
    int         usesCol;      //uses color attrib
    int         usesNoise;    //requires noise texture

    UniformLocation uniforms;
    struct gDPCombine      combine;
    uint32_t             flags;
    struct ShaderProgram   *left, *right;
    uint32_t             lastUsed;
} ShaderProgram;

extern ShaderProgram    *scProgramRoot;
extern ShaderProgram    *scProgramCurrent;
extern int              scProgramChanged;
extern int              scProgramCount;

void ShaderCombiner_UpdateBlendColor(void);
void ShaderCombiner_UpdateEnvColor(void);
void ShaderCombiner_UpdateFogColor(void);
void ShaderCombiner_UpdatePrimColor(void);
void ShaderCombiner_UpdateKeyColor(void);
void ShaderCombiner_UpdateLightParameters(void);
void ShaderCombiner_UpdateConvertColor(void);

#ifdef __cplusplus
}
#endif

#endif

