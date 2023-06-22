#include <string.h>
#include <stdlib.h>
#include "OpenGL.h"
#include "ShaderCombiner.h"
#include "Common.h"
#include "Textures.h"
#include "Config.h"

ShaderProgram *scProgramRoot = NULL;
ShaderProgram *scProgramCurrent = NULL;
int scProgramChanged = 0;
int scProgramCount = 0;

GLint _vertex_shader = 0;

const char *_frag_header = "                                \n"
#if !defined(HAVE_OPENGLES2) // Desktop GL fix
"#version 120                                               \n"
"#define highp                                              \n"
"#define lowp                                               \n"
"#define mediump                                            \n"
#endif
"uniform sampler2D uTex0;                                   \n"\
"uniform sampler2D uTex1;                                   \n"\
"uniform sampler2D uTexNoise;                               \n"\
"uniform lowp vec4 uEnvColor;                               \n"\
"uniform lowp vec4 uPrimColor;                              \n"\
"uniform lowp vec4 uFogColor;                               \n"\
"uniform highp float uAlphaRef;                             \n"\
"uniform lowp float uPrimLODFrac;                           \n"\
"uniform lowp float uK4;                                    \n"\
"uniform lowp float uK5;                                    \n"\
"                                                           \n"\
"varying lowp float vFactor;                                \n"\
"varying lowp vec4 vShadeColor;                             \n"\
"varying mediump vec2 vTexCoord0;                           \n"\
"varying mediump vec2 vTexCoord1;                           \n"\
"                                                           \n"\
"void main()                                                \n"\
"{                                                          \n"\
"lowp vec4 lFragColor;                                      \n";


const char *_vert = "                                       \n"
#if !defined(HAVE_OPENGLES2) // Desktop GL fix
"#version 120                                               \n"
"#define highp                                              \n"
"#define lowp                                               \n"
"#define mediump                                            \n"
#endif
"attribute highp vec4 	aPosition;                          \n"\
"attribute lowp vec4 	aColor;                             \n"\
"attribute highp vec2   aTexCoord0;                         \n"\
"attribute highp vec2   aTexCoord1;                         \n"\
"                                                           \n"\
"uniform bool		    uEnableFog;                         \n"\
"uniform float			 uFogScale, uFogOffset;         \n"\
"uniform float 			uRenderState;                       \n"\
"                                                           \n"\
"uniform mediump vec2 	uTexScale;                          \n"\
"uniform mediump vec2 	uTexOffset[2];                      \n"\
"uniform mediump vec2 	uCacheShiftScale[2];                \n"\
"uniform mediump vec2 	uCacheScale[2];                     \n"\
"uniform mediump vec2 	uCacheOffset[2];                    \n"\
"                                                           \n"\
"varying lowp float     vFactor;                            \n"\
"varying lowp vec4 		vShadeColor;                        \n"\
"varying mediump vec2 	vTexCoord0;                         \n"\
"varying mediump vec2 	vTexCoord1;                         \n"\
"                                                           \n"\
"void main()                                                \n"\
"{                                                          \n"\
"gl_Position = aPosition;                                   \n"\
"vShadeColor = aColor;                                      \n"\
"                                                           \n"\
"if (uRenderState == 1.0)                                   \n"\
"{                                                          \n"\
"vTexCoord0 = (aTexCoord0 * (uTexScale[0] *                 \n"\
"           uCacheShiftScale[0]) + (uCacheOffset[0] -       \n"\
"           uTexOffset[0])) * uCacheScale[0];               \n"\
"vTexCoord1 = (aTexCoord0 * (uTexScale[1] *                 \n"\
"           uCacheShiftScale[1]) + (uCacheOffset[1] -       \n"\
"           uTexOffset[1])) * uCacheScale[1];               \n"\
"}                                                          \n"\
"else                                                       \n"\
"{                                                          \n"\
"vTexCoord0 = aTexCoord0;                                   \n"\
"vTexCoord1 = aTexCoord1;                                   \n"\
"}                                                          \n"\
"                                                           \n";

const char * _vertfog = "                                   \n"\
"if (uEnableFog)                                            \n"\
"{                                                          \n"\
"vFactor = max(-1.0, aPosition.z / aPosition.w)             \n"\
"   * uFogScale + uFogOffset;                          \n"\
"vFactor = clamp(vFactor, 0.0, 1.0);                        \n"\
"}                                                          \n";

const char * _vertzhack = "                                 \n"\
"if (uRenderState == 1.0)                                   \n"\
"{                                                          \n"\
"gl_Position.z = (gl_Position.z + gl_Position.w*9.0) * 0.1; \n"\
"}                                                          \n";


static const char * _color_param_str(int param)
{
   switch(param)
   {
      case COMBINED:          return "lFragColor.rgb";
      case TEXEL0:            return "lTex0.rgb";
      case TEXEL1:            return "lTex1.rgb";
      case PRIMITIVE:         return "uPrimColor.rgb";
      case SHADE:             return "vShadeColor.rgb";
      case ENVIRONMENT:       return "uEnvColor.rgb";
      case CENTER:            return "vec3(0.0)";
      case SCALE:             return "vec3(0.0)";
      case COMBINED_ALPHA:    return "vec3(lFragColor.a)";
      case TEXEL0_ALPHA:      return "vec3(lTex0.a)";
      case TEXEL1_ALPHA:      return "vec3(lTex1.a)";
      case PRIMITIVE_ALPHA:   return "vec3(uPrimColor.a)";
      case SHADE_ALPHA:       return "vec3(vShadeColor.a)";
      case ENV_ALPHA:         return "vec3(uEnvColor.a)";
      case LOD_FRACTION:      return "vec3(0.0)";
      case PRIM_LOD_FRAC:     return "vec3(uPrimLODFrac)";
      case NOISE:             return "lNoise.rgb";
      case K4:                return "vec3(uK4)";
      case K5:                return "vec3(uK5)";
      case ONE:               return "vec3(1.0)";
      case ZERO:              return "vec3(0.0)";
      default:
                              return "vec3(0.0)";
   }
}

static const char * _alpha_param_str(int param)
{
   switch(param)
   {
      case COMBINED:          return "lFragColor.a";
      case TEXEL0:            return "lTex0.a";
      case TEXEL1:            return "lTex1.a";
      case PRIMITIVE:         return "uPrimColor.a";
      case SHADE:             return "vShadeColor.a";
      case ENVIRONMENT:       return "uEnvColor.a";
      case CENTER:            return "0.0";
      case SCALE:             return "0.0";
      case COMBINED_ALPHA:    return "lFragColor.a";
      case TEXEL0_ALPHA:      return "lTex0.a";
      case TEXEL1_ALPHA:      return "lTex1.a";
      case PRIMITIVE_ALPHA:   return "uPrimColor.a";
      case SHADE_ALPHA:       return "vShadeColor.a";
      case ENV_ALPHA:         return "uEnvColor.a";
      case LOD_FRACTION:      return "0.0";
      case PRIM_LOD_FRAC:     return "uPrimLODFrac";
      case NOISE:             return "lNoise.a";
      case K4:                return "uK4";
      case K5:                return "uK5";
      case ONE:               return "1.0";
      case ZERO:              return "0.0";
      default:
                              return "0.0";
   }
}

static int program_compare(ShaderProgram *prog, DecodedMux *dmux, uint32_t flags)
{
   if (prog)
      return ((prog->combine.mux == dmux->combine.mux) && (prog->flags == flags));
   return 1;
}

static void glcompiler_error(GLint shader)
{
   int len, i;
   char* log;

   glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &len);
   log = (char*) malloc(len + 1);
   glGetShaderInfoLog(shader, len, &i, log);
   log[len] = 0;
   LOG(LOG_ERROR, "COMPILE ERROR: %s \n", log);
   free(log);
}

static void gllinker_error(GLint program)
{
   int len, i;
   char* log;

   glGetProgramiv(program, GL_INFO_LOG_LENGTH, &len);
   log = (char*)malloc(len + 1);
   glGetProgramInfoLog(program, len, &i, log);
   log[len] = 0;
   LOG(LOG_ERROR, "LINK ERROR: %s \n", log);
   free(log);
}

static void locate_attributes(ShaderProgram *p)
{
   glBindAttribLocation(p->program, SC_POSITION,   "aPosition");
   glBindAttribLocation(p->program, SC_COLOR,      "aColor");
   glBindAttribLocation(p->program, SC_TEXCOORD0,  "aTexCoord0");
   glBindAttribLocation(p->program, SC_TEXCOORD1,  "aTexCoord1");
}

#define LocateUniform(A) \
    p->uniforms.A.loc = glGetUniformLocation(p->program, #A);

static void locate_uniforms(ShaderProgram *p)
{
   LocateUniform(uTex0);
   LocateUniform(uTex1);
   LocateUniform(uTexNoise);
   LocateUniform(uEnvColor);
   LocateUniform(uPrimColor);
   LocateUniform(uPrimLODFrac);
   LocateUniform(uK4);
   LocateUniform(uK5);
   LocateUniform(uFogColor);
   LocateUniform(uEnableFog);
   LocateUniform(uRenderState);
   LocateUniform(uFogScale);
   LocateUniform(uFogOffset);
   LocateUniform(uAlphaRef);
   LocateUniform(uTexScale);
   LocateUniform(uTexOffset[0]);
   LocateUniform(uTexOffset[1]);
   LocateUniform(uCacheShiftScale[0]);
   LocateUniform(uCacheShiftScale[1]);
   LocateUniform(uCacheScale[0]);
   LocateUniform(uCacheScale[1]);
   LocateUniform(uCacheOffset[0]);
   LocateUniform(uCacheOffset[1]);
}

static void force_uniforms(void)
{
   SC_ForceUniform1i(uTex0, 0);
   SC_ForceUniform1i(uTex1, 1);
   SC_ForceUniform1i(uTexNoise, 2);
   SC_ForceUniform4fv(uEnvColor, &gDP.envColor.r);
   SC_ForceUniform4fv(uPrimColor, &gDP.primColor.r);
   SC_ForceUniform1f(uPrimLODFrac, gDP.primColor.l);
   SC_ForceUniform1f(uK4, gDP.convert.k4);
   SC_ForceUniform1f(uK5, gDP.convert.k5);
   SC_ForceUniform4fv(uFogColor, &gDP.fogColor.r);
   SC_ForceUniform1i(uEnableFog, ((gSP.geometryMode & G_FOG)));
   SC_ForceUniform1f(uRenderState, (float) OGL.renderState);
   SC_ForceUniform1f(uFogScale, (float) gSP.fog.multiplier / 256.0f);
   SC_ForceUniform1f(uFogOffset, (float) gSP.fog.offset / 255.0f);
   SC_ForceUniform1f(uAlphaRef, (gDP.otherMode.cvgXAlpha) ? 0.5f : gDP.blendColor.a);
   SC_ForceUniform2f(uTexScale, gSP.texture.scales, gSP.texture.scalet);

   if (gSP.textureTile[0])
      SC_ForceUniform2f(uTexOffset[0], gSP.textureTile[0]->fuls, gSP.textureTile[0]->fult);
   else
      SC_ForceUniform2f(uTexOffset[0], 0.0f, 0.0f);

   if (gSP.textureTile[1])
      SC_ForceUniform2f(uTexOffset[1], gSP.textureTile[1]->fuls, gSP.textureTile[1]->fult);
   else
      SC_ForceUniform2f(uTexOffset[1], 0.0f, 0.0f);

   if (cache.current[0])
   {
      SC_ForceUniform2f(uCacheShiftScale[0], cache.current[0]->shiftScaleS, cache.current[0]->shiftScaleT);
      SC_ForceUniform2f(uCacheScale[0], cache.current[0]->scaleS, cache.current[0]->scaleT);
      SC_ForceUniform2f(uCacheOffset[0], cache.current[0]->offsetS, cache.current[0]->offsetT);
   }
   else
   {
      SC_ForceUniform2f(uCacheShiftScale[0], 1.0f, 1.0f);
      SC_ForceUniform2f(uCacheScale[0], 1.0f, 1.0f);
      SC_ForceUniform2f(uCacheOffset[0], 0.0f, 0.0f);
   }

   if (cache.current[1])
   {
      SC_ForceUniform2f(uCacheShiftScale[1], cache.current[1]->shiftScaleS, cache.current[1]->shiftScaleT);
      SC_ForceUniform2f(uCacheScale[1], cache.current[1]->scaleS, cache.current[1]->scaleT);
      SC_ForceUniform2f(uCacheOffset[1], cache.current[1]->offsetS, cache.current[1]->offsetT);
   }
   else
   {
      SC_ForceUniform2f(uCacheShiftScale[1], 1.0f, 1.0f);
      SC_ForceUniform2f(uCacheScale[1], 1.0f, 1.0f);
      SC_ForceUniform2f(uCacheOffset[1], 0.0f, 0.0f);
   }
}

static ShaderProgram *ShaderCombiner_Compile(DecodedMux *dmux, int flags)
{
   int i, j;
   GLint success, len[1];
   char frag[4096], *src[1], *buffer;
   ShaderProgram *prog;

   buffer = (char*)frag;
   prog = (ShaderProgram*) malloc(sizeof(ShaderProgram));

   prog->left = prog->right = NULL;
   prog->usesT0 = prog->usesT1 = prog->usesCol = prog->usesNoise = 0;
   prog->combine = dmux->combine;
   prog->flags = flags;
   prog->vertex = _vertex_shader;

   for(i = 0; i < ((flags & SC_2CYCLE) ? 4 : 2); i++)
   {
      //make sure were not ignoring cycle:
      if ((dmux->flags&(1<<i)) == 0)
      {
         {
            prog->usesT0 |= (dmux->decode[i].sa == TEXEL0 || dmux->decode[i].sa == TEXEL0_ALPHA);
            prog->usesT1 |= (dmux->decode[i].sa == TEXEL1 || dmux->decode[i].sa == TEXEL1_ALPHA);
            prog->usesCol |= (dmux->decode[i].sa == SHADE || dmux->decode[i].sa == SHADE_ALPHA);
            prog->usesNoise |= (dmux->decode[i].sa == NOISE);

            prog->usesT0 |= (dmux->decode[i].sb == TEXEL0 || dmux->decode[i].sb == TEXEL0_ALPHA);
            prog->usesT1 |= (dmux->decode[i].sb == TEXEL1 || dmux->decode[i].sb == TEXEL1_ALPHA);
            prog->usesCol |= (dmux->decode[i].sb == SHADE || dmux->decode[i].sb == SHADE_ALPHA);
            prog->usesNoise |= (dmux->decode[i].sb == NOISE);

            prog->usesT0 |= (dmux->decode[i].m == TEXEL0 || dmux->decode[i].m == TEXEL0_ALPHA);
            prog->usesT1 |= (dmux->decode[i].m == TEXEL1 || dmux->decode[i].m == TEXEL1_ALPHA);
            prog->usesCol |= (dmux->decode[i].m == SHADE || dmux->decode[i].m == SHADE_ALPHA);
            prog->usesNoise |= (dmux->decode[i].m == NOISE);

            prog->usesT0 |= (dmux->decode[i].a == TEXEL0 || dmux->decode[i].a == TEXEL0_ALPHA);
            prog->usesT1 |= (dmux->decode[i].a == TEXEL1 || dmux->decode[i].a == TEXEL1_ALPHA);
            prog->usesCol |= (dmux->decode[i].a == SHADE || dmux->decode[i].a == SHADE_ALPHA);
            prog->usesNoise |= (dmux->decode[i].a == NOISE);
         }
      }
   }

   buffer += sprintf(buffer, "%s", _frag_header);
   if (prog->usesT0)
      buffer += sprintf(buffer, "lowp vec4 lTex0 = texture2D(uTex0, vTexCoord0); \n");
   if (prog->usesT1)
      buffer += sprintf(buffer, "lowp vec4 lTex1 = texture2D(uTex1, vTexCoord1); \n");
   if (prog->usesNoise)
      buffer += sprintf(buffer, "lowp vec4 lNoise = texture2D(uTexNoise, (1.0 / 1024.0) * gl_FragCoord.st); \n");

   for(i = 0; i < ((flags & SC_2CYCLE) ? 2 : 1); i++)
   {
      if ((dmux->flags&(1<<(i*2))) == 0)
      {
         buffer += sprintf(buffer, "lFragColor.rgb = (%s - %s) * %s + %s; \n",
               _color_param_str(dmux->decode[i*2].sa),
               _color_param_str(dmux->decode[i*2].sb),
               _color_param_str(dmux->decode[i*2].m),
               _color_param_str(dmux->decode[i*2].a)
               );
      }

      if ((dmux->flags&(1<<(i*2+1))) == 0)
      {
         buffer += sprintf(buffer, "lFragColor.a = (%s - %s) * %s + %s; \n",
               _alpha_param_str(dmux->decode[i*2+1].sa),
               _alpha_param_str(dmux->decode[i*2+1].sb),
               _alpha_param_str(dmux->decode[i*2+1].m),
               _alpha_param_str(dmux->decode[i*2+1].a)
               );
      }
      buffer += sprintf(buffer, "gl_FragColor = lFragColor; \n");
   };

   //fog
   if (flags&SC_FOGENABLED)
   {
      buffer += sprintf(buffer, "gl_FragColor = mix(gl_FragColor, uFogColor, vFactor); \n");
   }

   //alpha function
   if (flags&SC_ALPHAENABLED)
   {
      if (flags&SC_ALPHAGREATER)
         buffer += sprintf(buffer, "if (gl_FragColor.a < uAlphaRef) %s;\n", config.hackAlpha ? "gl_FragColor.a = 0" : "discard");
      else
         buffer += sprintf(buffer, "if (gl_FragColor.a <= uAlphaRef) %s;\n", config.hackAlpha ? "gl_FragColor.a = 0" : "discard");
   }
   buffer += sprintf(buffer, "} \n\n");
   *buffer = 0;

   prog->program = glCreateProgram();

   //Compile:

   src[0] = frag;
   len[0] = MIN(4096, strlen(frag));
   prog->fragment = glCreateShader(GL_FRAGMENT_SHADER);

   glShaderSource(prog->fragment, 1, (const char**) src, len);
   glCompileShader(prog->fragment);


   glGetShaderiv(prog->fragment, GL_COMPILE_STATUS, &success);
   if (!success)
      glcompiler_error(prog->fragment);

   //link
   locate_attributes(prog);
   glAttachShader(prog->program, prog->fragment);
   glAttachShader(prog->program, prog->vertex);
   glLinkProgram(prog->program);
   glGetProgramiv(prog->program, GL_LINK_STATUS, &success);
   if (!success)
      gllinker_error(prog->program);

   //remove fragment shader:
   glDeleteShader(prog->fragment);

   locate_uniforms(prog);
   return prog;
}

void ShaderCombiner_UpdateBlendColor(void)
{
   SC_SetUniform1f(uAlphaRef, (gDP.otherMode.cvgXAlpha) ? 0.5f : gDP.blendColor.a);
}

void ShaderCombiner_UpdateEnvColor(void)
{
   SC_SetUniform4fv(uEnvColor, &gDP.envColor.r);
}

void ShaderCombiner_UpdateFogColor(void)
{
   SC_SetUniform4fv(uFogColor, &gDP.fogColor.r );
}

void ShaderCombiner_UpdateConvertColor(void)
{
   SC_SetUniform1f(uK4, gDP.convert.k4);
   SC_SetUniform1f(uK5, gDP.convert.k5);
}

void ShaderCombiner_UpdatePrimColor(void)
{
   SC_SetUniform4fv(uPrimColor, &gDP.primColor.r);
   SC_SetUniform1f(uPrimLODFrac, gDP.primColor.l);
}

void ShaderCombiner_UpdateKeyColor(void)
{
}

void ShaderCombiner_UpdateLightParameters(void)
{
}

void ShaderCombiner_Init(void)
{
   /* compile vertex shader: */
   GLint success;
   const char *src[1];
   char buff[4096], *str;
   str = buff;

   str += sprintf(str, "%s", _vert);
   str += sprintf(str, "%s", _vertfog);
   if (config.zHack)
      str += sprintf(str, "%s", _vertzhack);

   str += sprintf(str, "}\n\n");

   src[0] = buff;
   _vertex_shader = glCreateShader(GL_VERTEX_SHADER);
   glShaderSource(_vertex_shader, 1, (const char**) src, NULL);
   glCompileShader(_vertex_shader);
   glGetShaderiv(_vertex_shader, GL_COMPILE_STATUS, &success);
   if (!success)
      glcompiler_error(_vertex_shader);

   gDP.otherMode.cycleType = G_CYC_1CYCLE;
}

static void Combiner_DeletePrograms(ShaderProgram *prog)
{
   if (prog)
   {
      Combiner_DeletePrograms(prog->left);
      Combiner_DeletePrograms(prog->right);
      glDeleteProgram(prog->program);
      //glDeleteShader(prog->fragment);
      free(prog);
      scProgramCount--;
   }
}

void ShaderCombiner_Destroy(void)
{
   Combiner_DeletePrograms(scProgramRoot);
   glDeleteShader(_vertex_shader);
   scProgramCount = scProgramChanged = 0;
   scProgramRoot = scProgramCurrent = NULL;
}

void ShaderCombiner_Set(DecodedMux *dmux, int flags)
{
   ShaderProgram *root, *prog;

   /* if already bound: */
   if (scProgramCurrent)
   {
      if (program_compare(scProgramCurrent, dmux, flags))
      {
         scProgramChanged = 0;
         return;
      }
   }

   //traverse binary tree for cached programs
   scProgramChanged = 1;

   root = (ShaderProgram*)scProgramRoot;
   prog = (ShaderProgram*)root;
   while(!program_compare(prog, dmux, flags))
   {
      root = prog;
      if (prog->combine.mux < dmux->combine.mux)
         prog = prog->right;
      else
         prog = prog->left;
   }

   //build new program
   if (!prog)
   {
      scProgramCount++;
      prog = ShaderCombiner_Compile(dmux, flags);
      if (!root)
         scProgramRoot = prog;
      else if (root->combine.mux < dmux->combine.mux)
         root->right = prog;
      else
         root->left = prog;

   }

   prog->lastUsed = OGL.frame_dl;
   scProgramCurrent = prog;
   glUseProgram(prog->program);
   force_uniforms();
}
