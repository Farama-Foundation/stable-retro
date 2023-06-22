/*
* Glide64 - Glide video plugin for Nintendo 64 emulators.
* Copyright (c) 2002  Dave2001
* Copyright (c) 2003-2009  Sergey 'Gonetz' Lipski
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include <stdint.h>
#ifdef _WIN32
#include <windows.h>
#else // _WIN32
#include <string.h>
#include <stdlib.h>
#endif // _WIN32
#include <math.h>

#include "glide.h"
#include "glitchmain.h"
#include "../../libretro/libretro_private.h"

#include "../../Graphics/RDP/RDP_state.h"

float glide64_pow(float a, float b);

typedef struct _shader_program_key
{
   int index;

   int color_combiner;
   int alpha_combiner;
   int texture0_combiner;
   int texture1_combiner;
   int texture0_combinera;
   int texture1_combinera;
   int fog_enabled;
   int chroma_enabled;
   int dither_enabled;
   int three_point_filter0;
   int three_point_filter1;
   GLuint program_object;
   int texture0_location;
   int texture1_location;
   int vertexOffset_location;
   int textureSizes_location;
   int exactSizes_location;
   int fogModeEndScale_location;
   int fogColor_location;
   int alphaRef_location;
   int chroma_color_location;
   int lambda_location;

   int constant_color_location;
   int ccolor0_location;
   int ccolor1_location;
} shader_program_key;

static int fct[4], source0[4], operand0[4], source1[4], operand1[4], source2[4], operand2[4];
static int fcta[4],sourcea0[4],operanda0[4],sourcea1[4],operanda1[4],sourcea2[4],operanda2[4];
static int alpha_ref, alpha_func;
bool alpha_test = 0;

static shader_program_key *shader_programs = NULL;
static shader_program_key *current_shader  = NULL;

static int number_of_programs = 0;
static int color_combiner_key;
static int alpha_combiner_key;
static int texture0_combiner_key;
static int texture1_combiner_key;
static int texture0_combinera_key;
static int texture1_combinera_key;

float texture_env_color[4];
float ccolor[2][4];
static float chroma_color[4];
int fog_enabled;
static int chroma_enabled;
static int chroma_other_color;
static int chroma_other_alpha;
static int dither_enabled;

float fogStart,fogEnd;

int need_lambda[2];
float lambda_color[2][4];

// shaders variables
int need_to_compile;

static char *fragment_shader;
static GLuint vertex_shader_object;
GLuint program_object_default;
static int first_color = 1;
static int first_alpha = 1;
static int first_texture0 = 1;
static int first_texture1 = 1;
static int tex0_combiner_ext = 0;
static int tex1_combiner_ext = 0;
static int c_combiner_ext = 0;
static int a_combiner_ext = 0;

#if defined(HAVE_OPENGLES2) // Desktop GL fix
#define GLSL_VERSION "100"
#else
#define GLSL_VERSION "120"
#endif

#define SHADER_HEADER \
"#version " GLSL_VERSION          "\n"

#define SHADER_VARYING \
"varying highp vec4 vFrontColor;  \n" \
"varying highp vec4 vTexCoord[4]; \n"

static const char* fragment_shader_header =
SHADER_HEADER
#if defined(HAVE_OPENGLES2) // Desktop GL fix
"precision lowp float;          \n"
#else
"#define highp                  \n"
#endif
"uniform sampler2D texture0;    \n"
"uniform sampler2D texture1;    \n"
"uniform vec4 exactSizes;     \n"  //textureSizes doesn't contain the correct sizes, use this one instead for offset calculations
"uniform vec4 constant_color;   \n"
"uniform vec4 ccolor0;          \n"
"uniform vec4 ccolor1;          \n"
"uniform vec4 chroma_color;     \n"
"uniform float lambda;          \n"
"uniform vec3 fogColor;         \n"
"uniform float alphaRef;        \n"
"#define TEX0             texture2D(texture0, vTexCoord[0].xy) \n" \
"#define TEX0_OFFSET(off) texture2D(texture0, vTexCoord[0].xy - off/exactSizes.xy) \n" \
"#define TEX1             texture2D(texture1, vTexCoord[1].xy) \n" \
"#define TEX1_OFFSET(off) texture2D(texture1, vTexCoord[1].xy - off/exactSizes.zw) \n" \

"// START JINC2 CONSTANTS AND FUNCTIONS // \n"
"#define JINC2_WINDOW_SINC 0.44 \n"
"#define JINC2_SINC 0.82 \n"
"#define JINC2_AR_STRENGTH 0.8 \n"
"const   float halfpi            = 1.5707963267948966192313216916398;   \n"
"const   float pi                = 3.1415926535897932384626433832795;   \n"
"const   float wa                = JINC2_WINDOW_SINC*pi;    \n"
"const   float wb                = JINC2_SINC*pi;       \n"

"// Calculates the distance between two points  \n"
"float d(vec2 pt1, vec2 pt2)    \n"
"{  \n"
"  vec2 v = pt2 - pt1;  \n"
"  return sqrt(dot(v,v));   \n"
"}  \n"

"vec3 min4(vec3 a, vec3 b, vec3 c, vec3 d)  \n"
"{  \n"
"    return min(a, min(b, min(c, d)));  \n"
"}  \n"

"vec3 max4(vec3 a, vec3 b, vec3 c, vec3 d)  \n"
"{  \n"
 "   return max(a, max(b, max(c, d)));  \n"
"}  \n"

"vec4 min4(vec4 a, vec4 b, vec4 c, vec4 d)  \n"
"{  \n"
"    return min(a, min(b, min(c, d)));  \n"
"}  \n"

"vec4 max4(vec4 a, vec4 b, vec4 c, vec4 d)  \n"
"{  \n"
 "   return max(a, max(b, max(c, d)));  \n"
"}  \n"

"vec4 resampler(vec4 x) \n"
"{  \n"
"   vec4 res;   \n"
"   res = (x==vec4(0.0, 0.0, 0.0, 0.0)) ?  vec4(wa*wb)  :  sin(x*wa)*sin(x*wb)/(x*x);   \n"
"   return res; \n"
"}  \n"
"// END JINC2 CONSTANTS AND FUNCTIONS // \n"

SHADER_VARYING
"\n"
"void test_chroma(vec4 ctexture1); \n"
"\n"
"\n"
"void main()\n"
"{\n"
"  vec2 offset; \n"
"  vec4 c0,c1,c2; \n"
;

// using gl_FragCoord is terribly slow on ATI and varying variables don't work for some unknown
// reason, so we use the unused components of the texture2 coordinates
static const char* fragment_shader_dither =
"  highp float temp=abs(sin((vTexCoord[2].a)+sin((vTexCoord[2].a)+(vTexCoord[2].b))))*170.0; \n"
"  if ((fract(temp)+fract(temp/2.0)+fract(temp/4.0))>1.5) discard; \n"
;

static const char* fragment_shader_default =
"  gl_FragColor = TEX0; \n"
;
static const char* fragment_shader_readtex0color =
"  vec4 readtex0 = TEX0; \n"
;
static const char* fragment_shader_readtex0color_3point =
"  offset=fract(vTexCoord[0].xy*exactSizes.xy-vec2(0.5,0.5)); \n"
"  offset-=step(1.0,offset.x+offset.y); \n"
"  c0=TEX0_OFFSET(offset); \n"
"  c1=TEX0_OFFSET(vec2(offset.x-sign(offset.x),offset.y)); \n"
"  c2=TEX0_OFFSET(vec2(offset.x,offset.y-sign(offset.y))); \n"
"  vec4 readtex0 =c0+abs(offset.x)*(c1-c0)+abs(offset.y)*(c2-c0); \n"
;

static const char* fragment_shader_readtex0color_jinc2 =
"    vec4 color;    \n"
"    vec4 weights[4];   \n"

"    vec2 dx = vec2(1.0, 0.0);  \n"
"    vec2 dy = vec2(0.0, 1.0);  \n"

"    vec2 pc = vTexCoord[0].xy * exactSizes.xy;    \n"

"    vec2 tc = (floor(pc-vec2(0.5,0.5))+vec2(0.5,0.5)); \n"

"    weights[0] = resampler(vec4(d(pc, tc    -dx    -dy), d(pc, tc           -dy), d(pc, tc    +dx    -dy), d(pc, tc+2.0*dx    -dy)));  \n"
"    weights[1] = resampler(vec4(d(pc, tc    -dx       ), d(pc, tc              ), d(pc, tc    +dx       ), d(pc, tc+2.0*dx       )));  \n"
"    weights[2] = resampler(vec4(d(pc, tc    -dx    +dy), d(pc, tc           +dy), d(pc, tc    +dx    +dy), d(pc, tc+2.0*dx    +dy)));  \n"
"    weights[3] = resampler(vec4(d(pc, tc    -dx+2.0*dy), d(pc, tc       +2.0*dy), d(pc, tc    +dx+2.0*dy), d(pc, tc+2.0*dx+2.0*dy)));  \n"

"    dx = dx/exactSizes.xy;   \n"
"    dy = dy/exactSizes.xy;   \n"
"    tc = tc/exactSizes.xy;   \n"

"    vec4 c00 = texture2D(texture0, tc    -dx    -dy).xyzw;  \n"
"    vec4 c10 = texture2D(texture0, tc           -dy).xyzw;  \n"
"    vec4 c20 = texture2D(texture0, tc    +dx    -dy).xyzw;  \n"
"    vec4 c30 = texture2D(texture0, tc+2.0*dx    -dy).xyzw;  \n"
"    vec4 c01 = texture2D(texture0, tc    -dx       ).xyzw;  \n"
"    vec4 c11 = texture2D(texture0, tc              ).xyzw;  \n"
"    vec4 c21 = texture2D(texture0, tc    +dx       ).xyzw;  \n"
"    vec4 c31 = texture2D(texture0, tc+2.0*dx       ).xyzw;  \n"
"    vec4 c02 = texture2D(texture0, tc    -dx    +dy).xyzw;  \n"
"    vec4 c12 = texture2D(texture0, tc           +dy).xyzw;  \n"
"    vec4 c22 = texture2D(texture0, tc    +dx    +dy).xyzw;  \n"
"    vec4 c32 = texture2D(texture0, tc+2.0*dx    +dy).xyzw;  \n"
"    vec4 c03 = texture2D(texture0, tc    -dx+2.0*dy).xyzw;  \n"
"    vec4 c13 = texture2D(texture0, tc       +2.0*dy).xyzw;  \n"
"    vec4 c23 = texture2D(texture0, tc    +dx+2.0*dy).xyzw;  \n"
"    vec4 c33 = texture2D(texture0, tc+2.0*dx+2.0*dy).xyzw;  \n"

"    //  Get min/max samples    \n"
"    vec4 min_sample = min4(c11, c21, c12, c22);    \n"
"    vec4 max_sample = max4(c11, c21, c12, c22);    \n"

"    color = vec4(dot(weights[0], vec4(c00.x, c10.x, c20.x, c30.x)), dot(weights[0], vec4(c00.y, c10.y, c20.y, c30.y)), dot(weights[0], vec4(c00.z, c10.z, c20.z, c30.z)), dot(weights[0], vec4(c00.w, c10.w, c20.w, c30.w))); \n"
"    color+= vec4(dot(weights[1], vec4(c01.x, c11.x, c21.x, c31.x)), dot(weights[1], vec4(c01.y, c11.y, c21.y, c31.y)), dot(weights[1], vec4(c01.z, c11.z, c21.z, c31.z)), dot(weights[1], vec4(c01.w, c11.w, c21.w, c31.w))); \n"
"    color+= vec4(dot(weights[2], vec4(c02.x, c12.x, c22.x, c32.x)), dot(weights[2], vec4(c02.y, c12.y, c22.y, c32.y)), dot(weights[2], vec4(c02.z, c12.z, c22.z, c32.z)), dot(weights[2], vec4(c02.w, c12.w, c22.w, c32.w))); \n"
"    color+= vec4(dot(weights[3], vec4(c03.x, c13.x, c23.x, c33.x)), dot(weights[3], vec4(c03.y, c13.y, c23.y, c33.y)), dot(weights[3], vec4(c03.z, c13.z, c23.z, c33.z)), dot(weights[3], vec4(c03.w, c13.w, c23.w, c33.w))); \n"
"    color = color/(dot(weights[0], vec4(1,1,1,1)) + dot(weights[1], vec4(1,1,1,1)) + dot(weights[2], vec4(1,1,1,1)) + dot(weights[3], vec4(1,1,1,1))); \n"

"    // Anti-ringing    \n"
"    vec4 aux = color;  \n"
"    color = clamp(color, min_sample, max_sample);  \n"
"    color = mix(aux, color, JINC2_AR_STRENGTH);    \n"

"    // final sum and weight normalization  \n"
"    vec4 readtex1 = vec4(color); \n"
;

static const char* fragment_shader_readtex1color =
"  vec4 readtex1 = TEX1; \n"
;

static const char* fragment_shader_readtex1color_3point =
"  offset=fract(vTexCoord[1].xy*exactSizes.zw-vec2(0.5,0.5)); \n"
"  offset-=step(1.0,offset.x+offset.y); \n"
"  c0=TEX1_OFFSET(offset); \n"
"  c1=TEX1_OFFSET(vec2(offset.x-sign(offset.x),offset.y)); \n"
"  c2=TEX1_OFFSET(vec2(offset.x,offset.y-sign(offset.y))); \n"
"  vec4 readtex1 =c0+abs(offset.x)*(c1-c0)+abs(offset.y)*(c2-c0); \n";

static const char* fragment_shader_readtex1color_jinc2 =
"    vec4 color;    \n"
"    vec4 weights[4];   \n"

"    vec2 dx = vec2(1.0, 0.0);  \n"
"    vec2 dy = vec2(0.0, 1.0);  \n"

"    vec2 pc = vTexCoord[1].xy * exactSizes.zw;    \n"

"    vec2 tc = (floor(pc-vec2(0.5,0.5))+vec2(0.5,0.5)); \n"

"    weights[0] = resampler(vec4(d(pc, tc    -dx    -dy), d(pc, tc           -dy), d(pc, tc    +dx    -dy), d(pc, tc+2.0*dx    -dy)));  \n"
"    weights[1] = resampler(vec4(d(pc, tc    -dx       ), d(pc, tc              ), d(pc, tc    +dx       ), d(pc, tc+2.0*dx       )));  \n"
"    weights[2] = resampler(vec4(d(pc, tc    -dx    +dy), d(pc, tc           +dy), d(pc, tc    +dx    +dy), d(pc, tc+2.0*dx    +dy)));  \n"
"    weights[3] = resampler(vec4(d(pc, tc    -dx+2.0*dy), d(pc, tc       +2.0*dy), d(pc, tc    +dx+2.0*dy), d(pc, tc+2.0*dx+2.0*dy)));  \n"

"    dx = dx/exactSizes.zw;   \n"
"    dy = dy/exactSizes.zw;   \n"
"    tc = tc/exactSizes.zw;   \n"

"    vec4 c00 = texture2D(texture1, tc    -dx    -dy).xyzw;  \n"
"    vec4 c10 = texture2D(texture1, tc           -dy).xyzw;  \n"
"    vec4 c20 = texture2D(texture1, tc    +dx    -dy).xyzw;  \n"
"    vec4 c30 = texture2D(texture1, tc+2.0*dx    -dy).xyzw;  \n"
"    vec4 c01 = texture2D(texture1, tc    -dx       ).xyzw;  \n"
"    vec4 c11 = texture2D(texture1, tc              ).xyzw;  \n"
"    vec4 c21 = texture2D(texture1, tc    +dx       ).xyzw;  \n"
"    vec4 c31 = texture2D(texture1, tc+2.0*dx       ).xyzw;  \n"
"    vec4 c02 = texture2D(texture1, tc    -dx    +dy).xyzw;  \n"
"    vec4 c12 = texture2D(texture1, tc           +dy).xyzw;  \n"
"    vec4 c22 = texture2D(texture1, tc    +dx    +dy).xyzw;  \n"
"    vec4 c32 = texture2D(texture1, tc+2.0*dx    +dy).xyzw;  \n"
"    vec4 c03 = texture2D(texture1, tc    -dx+2.0*dy).xyzw;  \n"
"    vec4 c13 = texture2D(texture1, tc       +2.0*dy).xyzw;  \n"
"    vec4 c23 = texture2D(texture1, tc    +dx+2.0*dy).xyzw;  \n"
"    vec4 c33 = texture2D(texture1, tc+2.0*dx+2.0*dy).xyzw;  \n"

"    //  Get min/max samples    \n"
"    vec4 min_sample = min4(c11, c21, c12, c22);    \n"
"    vec4 max_sample = max4(c11, c21, c12, c22);    \n"

"    color = vec4(dot(weights[0], vec4(c00.x, c10.x, c20.x, c30.x)), dot(weights[0], vec4(c00.y, c10.y, c20.y, c30.y)), dot(weights[0], vec4(c00.z, c10.z, c20.z, c30.z)), dot(weights[0], vec4(c00.w, c10.w, c20.w, c30.w))); \n"
"    color+= vec4(dot(weights[1], vec4(c01.x, c11.x, c21.x, c31.x)), dot(weights[1], vec4(c01.y, c11.y, c21.y, c31.y)), dot(weights[1], vec4(c01.z, c11.z, c21.z, c31.z)), dot(weights[1], vec4(c01.w, c11.w, c21.w, c31.w))); \n"
"    color+= vec4(dot(weights[2], vec4(c02.x, c12.x, c22.x, c32.x)), dot(weights[2], vec4(c02.y, c12.y, c22.y, c32.y)), dot(weights[2], vec4(c02.z, c12.z, c22.z, c32.z)), dot(weights[2], vec4(c02.w, c12.w, c22.w, c32.w))); \n"
"    color+= vec4(dot(weights[3], vec4(c03.x, c13.x, c23.x, c33.x)), dot(weights[3], vec4(c03.y, c13.y, c23.y, c33.y)), dot(weights[3], vec4(c03.z, c13.z, c23.z, c33.z)), dot(weights[3], vec4(c03.w, c13.w, c23.w, c33.w))); \n"
"    color = color/(dot(weights[0], vec4(1,1,1,1)) + dot(weights[1], vec4(1,1,1,1)) + dot(weights[2], vec4(1,1,1,1)) + dot(weights[3], vec4(1,1,1,1))); \n"

"    // Anti-ringing    \n"
"    vec4 aux = color;  \n"
"    color = clamp(color, min_sample, max_sample);  \n"
"    color = mix(aux, color, JINC2_AR_STRENGTH);    \n"

"    // final sum and weight normalization  \n"
"    vec4 readtex1 = vec4(color); \n"
;

static const char* fragment_shader_fog =
"  float fog;  \n"
"  fog = vTexCoord[0].b;  \n"
"  gl_FragColor.rgb = mix(fogColor, gl_FragColor.rgb, fog); \n"
;

static const char* fragment_shader_end =
"if(gl_FragColor.a <= alphaRef) {discard;}   \n"
"}\n"
;

static const char* vertex_shader =
SHADER_HEADER
#if !defined(HAVE_OPENGLES2) // Desktop GL fix
"#define highp                         \n"
#endif
"#define Z_MAX 65536.0                 \n"
"attribute highp vec4 aPosition;         \n"
"attribute highp vec4 aColor;          \n"
"attribute highp vec4 aMultiTexCoord0; \n"
"attribute highp vec4 aMultiTexCoord1; \n"
"attribute float aFog;                 \n"
"uniform vec3 vertexOffset;            \n" //Moved some calculations from grDrawXXX to shader
"uniform vec4 textureSizes;            \n"
"uniform vec3 fogModeEndScale;         \n" //0 = Mode, 1 = gl_Fog.end, 2 = gl_Fog.scale
SHADER_VARYING
"\n"
"void main()\n"
"{\n"
"  highp float q = aPosition.w;                                                     \n"
"  highp float invertY = vertexOffset.z;                                          \n" //Usually 1.0 but -1.0 when rendering to a texture (see inverted_culling grRenderBuffer)
"  gl_Position.x = (aPosition.x - vertexOffset.x) / vertexOffset.x;           \n"
"  gl_Position.y = invertY *-(aPosition.y - vertexOffset.y) / vertexOffset.y; \n"
"  gl_Position.z = aPosition.z / Z_MAX;                                       \n"
"  gl_Position.w = 1.0;                                                     \n"
"  gl_Position /= q;                                                        \n"
"  vFrontColor = aColor.bgra;                                             \n"
"\n"
"  vTexCoord[0] = vec4(aMultiTexCoord0.xy / q / textureSizes.xy,0,1);     \n"
"  vTexCoord[1] = vec4(aMultiTexCoord1.xy / q / textureSizes.zw,0,1);     \n"
"\n"
"  float fogV = (1.0 / mix(q,aFog,fogModeEndScale[0])) / 255.0;             \n"
//"  //if(fogMode == 2) {                                                     \n"
//"  //  fogV = 1.0 / aFog / 255                                              \n"
//"  //}                                                                      \n"
"\n"
"  float f = (fogModeEndScale[1] - fogV) * fogModeEndScale[2];              \n"
"  f = clamp(f, 0.0, 1.0);                                                  \n"
"  vTexCoord[0].b = f;                                                    \n"
"  vTexCoord[2].b = aPosition.x;                                            \n"
"  vTexCoord[2].a = aPosition.y;                                            \n"
"}                                                                          \n"
;

static char fragment_shader_color_combiner[1024*2];
static char fragment_shader_alpha_combiner[1024*2];
static char fragment_shader_texture1[1024*2];
static char fragment_shader_texture0[1024*2];
static char fragment_shader_chroma[1024*2];
static char shader_log[2048];

void check_compile(GLuint shader)
{
   GLint success;
   glGetShaderiv(shader,GL_COMPILE_STATUS,&success);

   if (!success)
   {
      char log[1024];
      glGetShaderInfoLog(shader,1024,NULL,log);
      if (log_cb)
         log_cb(RETRO_LOG_ERROR, log);
   }
}

void check_link(GLuint program)
{
   GLint success;
   glGetProgramiv(program,GL_LINK_STATUS,&success);

   if (!success)
   {
      char log[1024];
      glGetProgramInfoLog(program,1024,NULL,log);
      if (log_cb)
         log_cb(RETRO_LOG_ERROR, log);
   }
}

static void append_shader_program(shader_program_key *shader)
{
   int curr_index;
   int                   index = number_of_programs;

   if (current_shader)
      curr_index = current_shader->index;

   shader->index = index;

   if (!shader_programs)
      shader_programs = (shader_program_key*)malloc(sizeof(shader_program_key));
   else
   {
      shader_program_key *new_ptr = (shader_program_key*)
         realloc(shader_programs, (index + 1) * sizeof(shader_program_key));
      if (!new_ptr)
         return;

      shader_programs = new_ptr;
   }

   if (current_shader)
      current_shader = &shader_programs[curr_index];

   shader_programs[index] = *shader;

   ++number_of_programs;
}

static void shader_bind_attributes(shader_program_key *shader)
{
   GLuint prog = shader->program_object;

   glBindAttribLocation(prog, POSITION_ATTR,   "aPosition");
   glBindAttribLocation(prog, COLOUR_ATTR,     "aColor");
   glBindAttribLocation(prog, TEXCOORD_0_ATTR, "aMultiTexCoord0");
   glBindAttribLocation(prog, TEXCOORD_1_ATTR, "aMultiTexCoord1");
   glBindAttribLocation(prog, FOG_ATTR,        "aFog");
}

static void use_shader_program(shader_program_key *shader)
{
   current_shader = &shader_programs[shader->index];
   glUseProgram(shader->program_object);
}

static void shader_find_uniforms(shader_program_key *shader)
{
   GLuint prog = shader->program_object;

   /* vertex shader uniforms */
   shader->vertexOffset_location    = glGetUniformLocation(prog, "vertexOffset");
   shader->textureSizes_location    = glGetUniformLocation(prog, "textureSizes");
   shader->fogModeEndScale_location = glGetUniformLocation(prog, "fogModeEndScale");

   /* fragment shader uniforms */
   shader->texture0_location       = glGetUniformLocation(prog, "texture0");
   shader->texture1_location       = glGetUniformLocation(prog, "texture1");
   shader->exactSizes_location     = glGetUniformLocation(prog, "exactSizes");
   shader->constant_color_location = glGetUniformLocation(prog, "constant_color");
   shader->ccolor0_location        = glGetUniformLocation(prog, "ccolor0");
   shader->ccolor1_location        = glGetUniformLocation(prog, "ccolor1");
   shader->chroma_color_location   = glGetUniformLocation(prog, "chroma_color");
   shader->lambda_location         = glGetUniformLocation(prog, "lambda");
   shader->fogColor_location       = glGetUniformLocation(prog, "fogColor");
   shader->alphaRef_location       = glGetUniformLocation(prog, "alphaRef");
}

static void finish_shader_program_setup(shader_program_key *shader)
{
   GLuint fragshader = glCreateShader(GL_FRAGMENT_SHADER);

   glShaderSource(fragshader, 1, (const GLchar**)&fragment_shader, NULL);
   glCompileShader(fragshader);
   check_compile(fragshader);

   shader->program_object = glCreateProgram();
   glAttachShader(shader->program_object, vertex_shader_object);
   glAttachShader(shader->program_object, fragshader);

   shader_bind_attributes(shader);

   glLinkProgram(shader->program_object);
   check_link(shader->program_object);
   glUseProgram(shader->program_object);

   shader_find_uniforms(shader);
   append_shader_program(shader);
}

void init_combiner(void)
{
   shader_program_key shader;

   if (shader_programs)
      free(shader_programs);

   number_of_programs = 0;
   shader_programs    = NULL;
   current_shader     = NULL;
   fragment_shader    = (char*)malloc(4096*2);
   need_to_compile    = true;

   /* default shader */
   memset(&shader, 0, sizeof(shader));

   strcpy(fragment_shader, fragment_shader_header);
   strcat(fragment_shader, fragment_shader_default);
   strcat(fragment_shader, fragment_shader_end);

   vertex_shader_object = glCreateShader(GL_VERTEX_SHADER);
   glShaderSource(vertex_shader_object, 1, (const GLchar**)&vertex_shader, NULL);
   glCompileShader(vertex_shader_object);
   check_compile(vertex_shader_object);

   finish_shader_program_setup(&shader);
   program_object_default = shader.program_object;

   use_shader_program(&shader);

   glUniform1i(shader.texture0_location, 0);
   glUniform1i(shader.texture1_location, 1);

   strcpy(fragment_shader_color_combiner, "");
   strcpy(fragment_shader_alpha_combiner, "");
   strcpy(fragment_shader_texture1, "vec4 ctexture1 = texture2D(texture0, vec2(vTexCoord[0])); \n");
   strcpy(fragment_shader_texture0, "");

   first_color = 1;
   first_alpha = 1;
   first_texture0 = 1;
   first_texture1 = 1;
   need_to_compile = 0;
   fog_enabled = 0;
   chroma_enabled = 0;
   dither_enabled = 0;
}

static void compile_chroma_shader(void)
{
   strcpy(fragment_shader_chroma, "\nvoid test_chroma(vec4 ctexture1)\n{\n");

   switch(chroma_other_alpha)
   {
      case GR_COMBINE_OTHER_ITERATED:
         strcat(fragment_shader_chroma, "float alpha = vFrontColor.a; \n");
         break;
      case GR_COMBINE_OTHER_TEXTURE:
         strcat(fragment_shader_chroma, "float alpha = ctexture1.a; \n");
         break;
      case GR_COMBINE_OTHER_CONSTANT:
         strcat(fragment_shader_chroma, "float alpha = constant_color.a; \n");
         break;
   }

   switch(chroma_other_color)
   {
      case GR_COMBINE_OTHER_ITERATED:
         strcat(fragment_shader_chroma, "vec4 color = vec4(vec3(vFrontColor),alpha); \n");
         break;
      case GR_COMBINE_OTHER_TEXTURE:
         strcat(fragment_shader_chroma, "vec4 color = vec4(vec3(ctexture1),alpha); \n");
         break;
      case GR_COMBINE_OTHER_CONSTANT:
         strcat(fragment_shader_chroma, "vec4 color = vec4(vec3(constant_color),alpha); \n");
         break;
   }

   strcat(fragment_shader_chroma, "if (color.rgb == chroma_color.rgb) discard; \n");
   strcat(fragment_shader_chroma, "}");
}


static void update_uniforms(const shader_program_key *prog)
{
   GLfloat v0, v2;
   glUniform1i(prog->texture0_location, 0);
   glUniform1i(prog->texture1_location, 1);

   v2 = 1.0f;
   glUniform3f(
      prog->vertexOffset_location,
      (GLfloat)width / 2.f,
      (GLfloat)height / 2.f,
      v2
   );
   glUniform4f(
      prog->textureSizes_location,
      (float)tex_width[0],
      (float)tex_height[0],
      (float)tex_width[1],
      (float)tex_height[1]
   );
   glUniform4f(
      prog->exactSizes_location,
      (float)tex_exactWidth[0],
      (float)tex_exactHeight[0],
      (float)tex_exactWidth[1],
      (float)tex_exactHeight[1]
   );

   v0 = fog_enabled != 2 ? 0.0f : 1.0f;
   v2 /= (fogEnd - fogStart);
   glUniform3f(prog->fogModeEndScale_location, v0, fogEnd,  v2);

   if(prog->fogColor_location != -1)
      glUniform3f(prog->fogColor_location, g_gdp.fog_color.r / 255.0f, g_gdp.fog_color.g / 255.0f, g_gdp.fog_color.b / 255.0f);

   glUniform1f(prog->alphaRef_location,alpha_test ? alpha_ref/255.0f : -1.0f);

   glUniform4f(prog->constant_color_location, texture_env_color[0], texture_env_color[1],
         texture_env_color[2], texture_env_color[3]);

   glUniform4f(prog->ccolor0_location, ccolor[0][0], ccolor[0][1], ccolor[0][2], ccolor[0][3]);

   glUniform4f(prog->ccolor1_location, ccolor[1][0], ccolor[1][1], ccolor[1][2], ccolor[1][3]);

   glUniform4f(prog->chroma_color_location, chroma_color[0], chroma_color[1],
         chroma_color[2], chroma_color[3]);

   set_lambda();
}

void compile_shader(void)
{
   shader_program_key shader;
   int i;

   need_to_compile = 0;

   for( i = 0; i < number_of_programs; i++)
   {
      shader_program_key *program = &shader_programs[i];
      if(program->color_combiner == color_combiner_key &&
            program->alpha_combiner == alpha_combiner_key &&
            program->texture0_combiner == texture0_combiner_key &&
            program->texture1_combiner == texture1_combiner_key &&
            program->texture0_combinera == texture0_combinera_key &&
            program->texture1_combinera == texture1_combinera_key &&
            program->fog_enabled == fog_enabled &&
            program->chroma_enabled == chroma_enabled &&
            program->dither_enabled == dither_enabled &&
            program->three_point_filter0 == three_point_filter[0] &&
            program->three_point_filter1 == three_point_filter[1])
      {
         use_shader_program(program);
         update_uniforms(program);
         return;
      }
   }

   shader.color_combiner        = color_combiner_key;
   shader.alpha_combiner        = alpha_combiner_key;
   shader.texture0_combiner     = texture0_combiner_key;
   shader.texture1_combiner     = texture1_combiner_key;
   shader.texture0_combinera    = texture0_combinera_key;
   shader.texture1_combinera    = texture1_combinera_key;
   shader.fog_enabled           = fog_enabled;
   shader.chroma_enabled        = chroma_enabled;
   shader.dither_enabled        = dither_enabled;
   shader.three_point_filter0   = three_point_filter[0];
   shader.three_point_filter1   = three_point_filter[1];
   shader.program_object        = 0;
   shader.texture0_location     = 0;
   shader.texture1_location     = 0;
   shader.vertexOffset_location = 0;
   shader.textureSizes_location = 0;
   shader.exactSizes_location   = 0;
   shader.fogModeEndScale_location   = 0;
   shader.fogColor_location     = 0;
   shader.alphaRef_location     = 0;
   shader.chroma_color_location = 0;
   shader.lambda_location       = 0;
   shader.constant_color_location = 0;
   shader.ccolor0_location      = 0;
   shader.ccolor1_location      = 0;

   strcpy(fragment_shader, fragment_shader_header);

   if (dither_enabled)
      strcat(fragment_shader, fragment_shader_dither);

   strcat(fragment_shader, three_point_filter[0] ? fragment_shader_readtex0color_3point : fragment_shader_readtex0color);
   strcat(fragment_shader, three_point_filter[1] ? fragment_shader_readtex1color_3point : fragment_shader_readtex1color);
   strcat(fragment_shader, fragment_shader_texture0);
   strcat(fragment_shader, fragment_shader_texture1);
   strcat(fragment_shader, fragment_shader_color_combiner);
   strcat(fragment_shader, fragment_shader_alpha_combiner);

   if (fog_enabled)
      strcat(fragment_shader, fragment_shader_fog);

   if (chroma_enabled)
   {
      strcat(fragment_shader, fragment_shader_chroma);
      strcat(fragment_shader_texture1, "test_chroma(ctexture1); \n");
      compile_chroma_shader();
   }

   strcat(fragment_shader, fragment_shader_end);

   finish_shader_program_setup(&shader);

   update_uniforms(&shader);
}

void free_combiners(void)
{
   if (shader_programs)
   {
      shader_program_key *s = shader_programs;

      while (number_of_programs--)
      {
         if (glIsProgram(s->program_object))
            glDeleteProgram(s->program_object);
      }

      free(shader_programs);
   }

   if (fragment_shader)
      free(fragment_shader);

   shader_programs = NULL;
   current_shader  = NULL;
   fragment_shader = NULL;

   number_of_programs = 0;
}

void set_copy_shader(void)
{
   int texture0_location;
   int alphaRef_location;

   glUseProgram(program_object_default);
   texture0_location = glGetUniformLocation(program_object_default, "texture0");
   glUniform1i(texture0_location, 0);

   alphaRef_location = glGetUniformLocation(program_object_default, "alphaRef");
   if(alphaRef_location != -1)
      glUniform1f(alphaRef_location,alpha_test ? alpha_ref/255.0f : -1.0f);
}

void set_depth_shader(void)
{
}

void set_lambda(void)
{
   glUniform1f(current_shader->lambda_location, lambda);
}

void grConstantColorValue( uint32_t value )
{
   texture_env_color[0] = ((value >> 24) & 0xFF) / 255.0f;
   texture_env_color[1] = ((value >> 16) & 0xFF) / 255.0f;
   texture_env_color[2] = ((value >>  8) & 0xFF) / 255.0f;
   texture_env_color[3] = (value & 0xFF) / 255.0f;

   glUniform4f(current_shader->constant_color_location, texture_env_color[0], texture_env_color[1],
         texture_env_color[2], texture_env_color[3]);
}

static void writeGLSLColorOther(int other)
{
   switch(other)
   {
      case GR_COMBINE_OTHER_ITERATED:
         strcat(fragment_shader_color_combiner, "vec4 color_other = vFrontColor; \n");
         break;
      case GR_COMBINE_OTHER_TEXTURE:
         strcat(fragment_shader_color_combiner, "vec4 color_other = ctexture1; \n");
         break;
      case GR_COMBINE_OTHER_CONSTANT:
         strcat(fragment_shader_color_combiner, "vec4 color_other = constant_color; \n");
         break;
   }
}

static void writeGLSLColorLocal(int local)
{
   switch(local)
   {
      case GR_COMBINE_LOCAL_ITERATED:
         strcat(fragment_shader_color_combiner, "vec4 color_local = vFrontColor; \n");
         break;
      case GR_COMBINE_LOCAL_CONSTANT:
         strcat(fragment_shader_color_combiner, "vec4 color_local = constant_color; \n");
         break;
   }
}

static void writeGLSLColorFactor(int factor, int local, int need_local, int other, int need_other)
{
   switch(factor)
   {
      case GR_COMBINE_FACTOR_ZERO:
         strcat(fragment_shader_color_combiner, "vec4 color_factor = vec4(0.0); \n");
         break;
      case GR_COMBINE_FACTOR_LOCAL:
         if(need_local) writeGLSLColorLocal(local);
         strcat(fragment_shader_color_combiner, "vec4 color_factor = color_local; \n");
         break;
      case GR_COMBINE_FACTOR_OTHER_ALPHA:
         if(need_other) writeGLSLColorOther(other);
         strcat(fragment_shader_color_combiner, "vec4 color_factor = vec4(color_other.a); \n");
         break;
      case GR_COMBINE_FACTOR_LOCAL_ALPHA:
         if(need_local) writeGLSLColorLocal(local);
         strcat(fragment_shader_color_combiner, "vec4 color_factor = vec4(color_local.a); \n");
         break;
      case GR_COMBINE_FACTOR_TEXTURE_ALPHA:
         strcat(fragment_shader_color_combiner, "vec4 color_factor = vec4(ctexture1.a); \n");
         break;
      case GR_COMBINE_FACTOR_TEXTURE_RGB:
         strcat(fragment_shader_color_combiner, "vec4 color_factor = ctexture1; \n");
         break;
      case GR_COMBINE_FACTOR_ONE:
         strcat(fragment_shader_color_combiner, "vec4 color_factor = vec4(1.0); \n");
         break;
      case GR_COMBINE_FACTOR_ONE_MINUS_LOCAL:
         if(need_local) writeGLSLColorLocal(local);
         strcat(fragment_shader_color_combiner, "vec4 color_factor = vec4(1.0) - color_local; \n");
         break;
      case GR_COMBINE_FACTOR_ONE_MINUS_OTHER_ALPHA:
         if(need_other) writeGLSLColorOther(other);
         strcat(fragment_shader_color_combiner, "vec4 color_factor = vec4(1.0) - vec4(color_other.a); \n");
         break;
      case GR_COMBINE_FACTOR_ONE_MINUS_LOCAL_ALPHA:
         if(need_local) writeGLSLColorLocal(local);
         strcat(fragment_shader_color_combiner, "vec4 color_factor = vec4(1.0) - vec4(color_local.a); \n");
         break;
      case GR_COMBINE_FACTOR_ONE_MINUS_TEXTURE_ALPHA:
         strcat(fragment_shader_color_combiner, "vec4 color_factor = vec4(1.0) - vec4(ctexture1.a); \n");
         break;
   }
}

void grColorCombine(
      int32_t function, int32_t factor,
      int32_t local, int32_t other,
      int32_t invert )
{
   static int last_function = 0;
   static int last_factor = 0;
   static int last_local = 0;
   static int last_other = 0;

   if(last_function == function && last_factor == factor &&
         last_local == local && last_other == other && first_color == 0 && !c_combiner_ext)
      return;
   first_color = 0;
   c_combiner_ext = 0;

   last_function = function;
   last_factor = factor;
   last_local = local;
   last_other = other;

   color_combiner_key = function | (factor << 4) | (local << 8) | (other << 10);
   chroma_other_color = other;

   strcpy(fragment_shader_color_combiner, "");
   switch(function)
   {
      case GR_COMBINE_FUNCTION_ZERO:
         strcat(fragment_shader_color_combiner, "gl_FragColor = vec4(0.0); \n");
         break;
      case GR_COMBINE_FUNCTION_LOCAL:
         writeGLSLColorLocal(local);
         strcat(fragment_shader_color_combiner, "gl_FragColor = color_local; \n");
         break;
      case GR_COMBINE_FUNCTION_LOCAL_ALPHA:
         writeGLSLColorLocal(local);
         strcat(fragment_shader_color_combiner, "gl_FragColor = vec4(color_local.a); \n");
         break;
      case GR_COMBINE_FUNCTION_SCALE_OTHER:
         writeGLSLColorOther(other);
         writeGLSLColorFactor(factor,local,1,other,0);
         strcat(fragment_shader_color_combiner, "gl_FragColor = color_factor * color_other; \n");
         break;
      case GR_COMBINE_FUNCTION_SCALE_OTHER_ADD_LOCAL:
         writeGLSLColorLocal(local);
         writeGLSLColorOther(other);
         writeGLSLColorFactor(factor,local,0,other,0);
         strcat(fragment_shader_color_combiner, "gl_FragColor = color_factor * color_other + color_local; \n");
         break;
      case GR_COMBINE_FUNCTION_SCALE_OTHER_ADD_LOCAL_ALPHA:
         writeGLSLColorLocal(local);
         writeGLSLColorOther(other);
         writeGLSLColorFactor(factor,local,0,other,0);
         strcat(fragment_shader_color_combiner, "gl_FragColor = color_factor * color_other + vec4(color_local.a); \n");
         break;
      case GR_COMBINE_FUNCTION_SCALE_OTHER_MINUS_LOCAL:
         writeGLSLColorLocal(local);
         writeGLSLColorOther(other);
         writeGLSLColorFactor(factor,local,0,other,0);
         strcat(fragment_shader_color_combiner, "gl_FragColor = color_factor * (color_other - color_local); \n");
         break;
      case GR_COMBINE_FUNCTION_SCALE_OTHER_MINUS_LOCAL_ADD_LOCAL:
         writeGLSLColorLocal(local);
         writeGLSLColorOther(other);
         writeGLSLColorFactor(factor,local,0,other,0);
         strcat(fragment_shader_color_combiner, "gl_FragColor = color_factor * (color_other - color_local) + color_local; \n");
         break;
      case GR_COMBINE_FUNCTION_SCALE_OTHER_MINUS_LOCAL_ADD_LOCAL_ALPHA:
         writeGLSLColorLocal(local);
         writeGLSLColorOther(other);
         writeGLSLColorFactor(factor,local,0,other,0);
         strcat(fragment_shader_color_combiner, "gl_FragColor = color_factor * (color_other - color_local) + vec4(color_local.a); \n");
         break;
      case GR_COMBINE_FUNCTION_SCALE_MINUS_LOCAL_ADD_LOCAL:
         writeGLSLColorLocal(local);
         writeGLSLColorFactor(factor,local,0,other,1);
         strcat(fragment_shader_color_combiner, "gl_FragColor = color_factor * (-color_local) + color_local; \n");
         break;
      case GR_COMBINE_FUNCTION_SCALE_MINUS_LOCAL_ADD_LOCAL_ALPHA:
         writeGLSLColorLocal(local);
         writeGLSLColorFactor(factor,local,0,other,1);
         strcat(fragment_shader_color_combiner, "gl_FragColor = color_factor * (-color_local) + vec4(color_local.a); \n");
         break;
      default:
         strcpy(fragment_shader_color_combiner, fragment_shader_default);
   }

   need_to_compile = 1;
}

static void writeGLSLAlphaOther(int other)
{
   switch(other)
   {
      case GR_COMBINE_OTHER_ITERATED:
         strcat(fragment_shader_alpha_combiner, "float alpha_other = vFrontColor.a; \n");
         break;
      case GR_COMBINE_OTHER_TEXTURE:
         strcat(fragment_shader_alpha_combiner, "float alpha_other = ctexture1.a; \n");
         break;
      case GR_COMBINE_OTHER_CONSTANT:
         strcat(fragment_shader_alpha_combiner, "float alpha_other = constant_color.a; \n");
         break;
   }
}

static void writeGLSLAlphaLocal(int local)
{
   switch(local)
   {
      case GR_COMBINE_LOCAL_ITERATED:
         strcat(fragment_shader_alpha_combiner, "float alpha_local = vFrontColor.a; \n");
         break;
      case GR_COMBINE_LOCAL_CONSTANT:
         strcat(fragment_shader_alpha_combiner, "float alpha_local = constant_color.a; \n");
         break;
   }
}

static void writeGLSLAlphaFactor(int factor, int local, int need_local, int other, int need_other)
{
   switch(factor)
   {
      case GR_COMBINE_FACTOR_ZERO:
         strcat(fragment_shader_alpha_combiner, "float alpha_factor = 0.0; \n");
         break;
      case GR_COMBINE_FACTOR_LOCAL:
         if(need_local) writeGLSLAlphaLocal(local);
         strcat(fragment_shader_alpha_combiner, "float alpha_factor = alpha_local; \n");
         break;
      case GR_COMBINE_FACTOR_OTHER_ALPHA:
         if(need_other) writeGLSLAlphaOther(other);
         strcat(fragment_shader_alpha_combiner, "float alpha_factor = alpha_other; \n");
         break;
      case GR_COMBINE_FACTOR_LOCAL_ALPHA:
         if(need_local) writeGLSLAlphaLocal(local);
         strcat(fragment_shader_alpha_combiner, "float alpha_factor = alpha_local; \n");
         break;
      case GR_COMBINE_FACTOR_TEXTURE_ALPHA:
         strcat(fragment_shader_alpha_combiner, "float alpha_factor = ctexture1.a; \n");
         break;
      case GR_COMBINE_FACTOR_ONE:
         strcat(fragment_shader_alpha_combiner, "float alpha_factor = 1.0; \n");
         break;
      case GR_COMBINE_FACTOR_ONE_MINUS_LOCAL:
         if(need_local) writeGLSLAlphaLocal(local);
         strcat(fragment_shader_alpha_combiner, "float alpha_factor = 1.0 - alpha_local; \n");
         break;
      case GR_COMBINE_FACTOR_ONE_MINUS_OTHER_ALPHA:
         if(need_other) writeGLSLAlphaOther(other);
         strcat(fragment_shader_alpha_combiner, "float alpha_factor = 1.0 - alpha_other; \n");
         break;
      case GR_COMBINE_FACTOR_ONE_MINUS_LOCAL_ALPHA:
         if(need_local) writeGLSLAlphaLocal(local);
         strcat(fragment_shader_alpha_combiner, "float alpha_factor = 1.0 - alpha_local; \n");
         break;
      case GR_COMBINE_FACTOR_ONE_MINUS_TEXTURE_ALPHA:
         strcat(fragment_shader_alpha_combiner, "float alpha_factor = 1.0 - ctexture1.a; \n");
         break;
   }
}

void grAlphaCombine(
      int32_t function, int32_t factor,
      int32_t local, int32_t other,
      int32_t invert
      )
{
   static int last_function = 0;
   static int last_factor = 0;
   static int last_local = 0;
   static int last_other = 0;

   if(last_function == function && last_factor == factor &&
         last_local == local && last_other == other && first_alpha == 0 && !a_combiner_ext) return;
   first_alpha = 0;
   a_combiner_ext = 0;

   last_function = function;
   last_factor = factor;
   last_local = local;
   last_other = other;

   alpha_combiner_key = function | (factor << 4) | (local << 8) | (other << 10);
   chroma_other_alpha = other;

   strcpy(fragment_shader_alpha_combiner, "");

   switch(function)
   {
      case GR_COMBINE_FUNCTION_ZERO:
         strcat(fragment_shader_alpha_combiner, "gl_FragColor.a = 0.0; \n");
         break;
      case GR_COMBINE_FUNCTION_LOCAL:
         writeGLSLAlphaLocal(local);
         strcat(fragment_shader_alpha_combiner, "gl_FragColor.a = alpha_local; \n");
         break;
      case GR_COMBINE_FUNCTION_LOCAL_ALPHA:
         writeGLSLAlphaLocal(local);
         strcat(fragment_shader_alpha_combiner, "gl_FragColor.a = alpha_local; \n");
         break;
      case GR_COMBINE_FUNCTION_SCALE_OTHER:
         writeGLSLAlphaOther(other);
         writeGLSLAlphaFactor(factor,local,1,other,0);
         strcat(fragment_shader_alpha_combiner, "gl_FragColor.a = alpha_factor * alpha_other; \n");
         break;
      case GR_COMBINE_FUNCTION_SCALE_OTHER_ADD_LOCAL:
         writeGLSLAlphaLocal(local);
         writeGLSLAlphaOther(other);
         writeGLSLAlphaFactor(factor,local,0,other,0);
         strcat(fragment_shader_alpha_combiner, "gl_FragColor.a = alpha_factor * alpha_other + alpha_local; \n");
         break;
      case GR_COMBINE_FUNCTION_SCALE_OTHER_ADD_LOCAL_ALPHA:
         writeGLSLAlphaLocal(local);
         writeGLSLAlphaOther(other);
         writeGLSLAlphaFactor(factor,local,0,other,0);
         strcat(fragment_shader_alpha_combiner, "gl_FragColor.a = alpha_factor * alpha_other + alpha_local; \n");
         break;
      case GR_COMBINE_FUNCTION_SCALE_OTHER_MINUS_LOCAL:
         writeGLSLAlphaLocal(local);
         writeGLSLAlphaOther(other);
         writeGLSLAlphaFactor(factor,local,0,other,0);
         strcat(fragment_shader_alpha_combiner, "gl_FragColor.a = alpha_factor * (alpha_other - alpha_local); \n");
         break;
      case GR_COMBINE_FUNCTION_SCALE_OTHER_MINUS_LOCAL_ADD_LOCAL:
         writeGLSLAlphaLocal(local);
         writeGLSLAlphaOther(other);
         writeGLSLAlphaFactor(factor,local,0,other,0);
         strcat(fragment_shader_alpha_combiner, "gl_FragColor.a = alpha_factor * (alpha_other - alpha_local) + alpha_local; \n");
         break;
      case GR_COMBINE_FUNCTION_SCALE_OTHER_MINUS_LOCAL_ADD_LOCAL_ALPHA:
         writeGLSLAlphaLocal(local);
         writeGLSLAlphaOther(other);
         writeGLSLAlphaFactor(factor,local,0,other,0);
         strcat(fragment_shader_alpha_combiner, "gl_FragColor.a = alpha_factor * (alpha_other - alpha_local) + alpha_local; \n");
         break;
      case GR_COMBINE_FUNCTION_SCALE_MINUS_LOCAL_ADD_LOCAL:
         writeGLSLAlphaLocal(local);
         writeGLSLAlphaFactor(factor,local,0,other,1);
         strcat(fragment_shader_alpha_combiner, "gl_FragColor.a = alpha_factor * (-alpha_local) + alpha_local; \n");
         break;
      case GR_COMBINE_FUNCTION_SCALE_MINUS_LOCAL_ADD_LOCAL_ALPHA:
         writeGLSLAlphaLocal(local);
         writeGLSLAlphaFactor(factor,local,0,other,1);
         strcat(fragment_shader_alpha_combiner, "gl_FragColor.a = alpha_factor * (-alpha_local) + alpha_local; \n");
         break;
   }

   need_to_compile = 1;
}

static void writeGLSLTextureColorFactorTMU0(int num_tex, int factor)
{
   switch(factor)
   {
      case GR_COMBINE_FACTOR_ZERO:
         strcat(fragment_shader_texture0, "vec4 texture0_color_factor = vec4(0.0); \n");
         break;
      case GR_COMBINE_FACTOR_LOCAL:
         strcat(fragment_shader_texture0, "vec4 texture0_color_factor = readtex0; \n");
         break;
      case GR_COMBINE_FACTOR_OTHER_ALPHA:
         strcat(fragment_shader_texture0, "vec4 texture0_color_factor = vec4(0.0); \n");
         break;
      case GR_COMBINE_FACTOR_LOCAL_ALPHA:
         strcat(fragment_shader_texture0, "vec4 texture0_color_factor = vec4(readtex0.a); \n");
         break;
      case GR_COMBINE_FACTOR_DETAIL_FACTOR:
         strcat(fragment_shader_texture0, "vec4 texture0_color_factor = vec4(lambda); \n");
         break;
      case GR_COMBINE_FACTOR_ONE:
         strcat(fragment_shader_texture0, "vec4 texture0_color_factor = vec4(1.0); \n");
         break;
      case GR_COMBINE_FACTOR_ONE_MINUS_LOCAL:
         strcat(fragment_shader_texture0, "vec4 texture0_color_factor = vec4(1.0) - readtex0; \n");
         break;
      case GR_COMBINE_FACTOR_ONE_MINUS_OTHER_ALPHA:
         strcat(fragment_shader_texture0, "vec4 texture0_color_factor = vec4(1.0) - vec4(0.0); \n");
         break;
      case GR_COMBINE_FACTOR_ONE_MINUS_LOCAL_ALPHA:
         strcat(fragment_shader_texture0, "vec4 texture0_color_factor = vec4(1.0) - vec4(readtex0.a); \n");
         break;
      case GR_COMBINE_FACTOR_ONE_MINUS_DETAIL_FACTOR:
         strcat(fragment_shader_texture0, "vec4 texture0_color_factor = vec4(1.0) - vec4(lambda); \n");
         break;
   }
}

static void writeGLSLTextureColorFactorTMU1(int num_tex, int factor)
{
   switch(factor)
   {
      case GR_COMBINE_FACTOR_ZERO:
         strcat(fragment_shader_texture1, "vec4 texture1_color_factor = vec4(0.0); \n");
         break;
      case GR_COMBINE_FACTOR_LOCAL:
         strcat(fragment_shader_texture1, "vec4 texture1_color_factor = readtex1; \n");
         break;
      case GR_COMBINE_FACTOR_OTHER_ALPHA:
         strcat(fragment_shader_texture1, "vec4 texture1_color_factor = vec4(ctexture0.a); \n");
         break;
      case GR_COMBINE_FACTOR_LOCAL_ALPHA:
         strcat(fragment_shader_texture1, "vec4 texture1_color_factor = vec4(readtex1.a); \n");
         break;
      case GR_COMBINE_FACTOR_DETAIL_FACTOR:
         strcat(fragment_shader_texture1, "vec4 texture1_color_factor = vec4(lambda); \n");
         break;
      case GR_COMBINE_FACTOR_ONE:
         strcat(fragment_shader_texture1, "vec4 texture1_color_factor = vec4(1.0); \n");
         break;
      case GR_COMBINE_FACTOR_ONE_MINUS_LOCAL:
         strcat(fragment_shader_texture1, "vec4 texture1_color_factor = vec4(1.0) - readtex1; \n");
         break;
      case GR_COMBINE_FACTOR_ONE_MINUS_OTHER_ALPHA:
         strcat(fragment_shader_texture1, "vec4 texture1_color_factor = vec4(1.0) - vec4(ctexture0.a); \n");
         break;
      case GR_COMBINE_FACTOR_ONE_MINUS_LOCAL_ALPHA:
         strcat(fragment_shader_texture1, "vec4 texture1_color_factor = vec4(1.0) - vec4(readtex1.a); \n");
         break;
      case GR_COMBINE_FACTOR_ONE_MINUS_DETAIL_FACTOR:
         strcat(fragment_shader_texture1, "vec4 texture1_color_factor = vec4(1.0) - vec4(lambda); \n");
         break;
   }
}

static void writeGLSLTextureAlphaFactorTMU0(int num_tex, int factor)
{
   switch(factor)
   {
      case GR_COMBINE_FACTOR_ZERO:
         strcat(fragment_shader_texture0, "float texture0_alpha_factor = 0.0; \n");
         break;
      case GR_COMBINE_FACTOR_LOCAL:
         strcat(fragment_shader_texture0, "float texture0_alpha_factor = readtex0.a; \n");
         break;
      case GR_COMBINE_FACTOR_OTHER_ALPHA:
         strcat(fragment_shader_texture0, "float texture0_alpha_factor = 0.0; \n");
         break;
      case GR_COMBINE_FACTOR_LOCAL_ALPHA:
         strcat(fragment_shader_texture0, "float texture0_alpha_factor = readtex0.a; \n");
         break;
      case GR_COMBINE_FACTOR_DETAIL_FACTOR:
         strcat(fragment_shader_texture0, "float texture0_alpha_factor = lambda; \n");
         break;
      case GR_COMBINE_FACTOR_ONE:
         strcat(fragment_shader_texture0, "float texture0_alpha_factor = 1.0; \n");
         break;
      case GR_COMBINE_FACTOR_ONE_MINUS_LOCAL:
         strcat(fragment_shader_texture0, "float texture0_alpha_factor = 1.0 - readtex0.a; \n");
         break;
      case GR_COMBINE_FACTOR_ONE_MINUS_OTHER_ALPHA:
         strcat(fragment_shader_texture0, "float texture0_alpha_factor = 1.0 - 0.0; \n");
         break;
      case GR_COMBINE_FACTOR_ONE_MINUS_LOCAL_ALPHA:
         strcat(fragment_shader_texture0, "float texture0_alpha_factor = 1.0 - readtex0.a; \n");
         break;
      case GR_COMBINE_FACTOR_ONE_MINUS_DETAIL_FACTOR:
         strcat(fragment_shader_texture0, "float texture0_alpha_factor = 1.0 - lambda; \n");
         break;
   }
}

static void writeGLSLTextureAlphaFactorTMU1(int num_tex, int factor)
{
   switch(factor)
   {
      case GR_COMBINE_FACTOR_ZERO:
         strcat(fragment_shader_texture1, "float texture1_alpha_factor = 0.0; \n");
         break;
      case GR_COMBINE_FACTOR_LOCAL:
         strcat(fragment_shader_texture1, "float texture1_alpha_factor = readtex1.a; \n");
         break;
      case GR_COMBINE_FACTOR_OTHER_ALPHA:
         strcat(fragment_shader_texture1, "float texture1_alpha_factor = ctexture0.a; \n");
         break;
      case GR_COMBINE_FACTOR_LOCAL_ALPHA:
         strcat(fragment_shader_texture1, "float texture1_alpha_factor = readtex1.a; \n");
         break;
      case GR_COMBINE_FACTOR_DETAIL_FACTOR:
         strcat(fragment_shader_texture1, "float texture1_alpha_factor = lambda; \n");
         break;
      case GR_COMBINE_FACTOR_ONE:
         strcat(fragment_shader_texture1, "float texture1_alpha_factor = 1.0; \n");
         break;
      case GR_COMBINE_FACTOR_ONE_MINUS_LOCAL:
         strcat(fragment_shader_texture1, "float texture1_alpha_factor = 1.0 - readtex1.a; \n");
         break;
      case GR_COMBINE_FACTOR_ONE_MINUS_OTHER_ALPHA:
         strcat(fragment_shader_texture1, "float texture1_alpha_factor = 1.0 - ctexture0.a; \n");
         break;
      case GR_COMBINE_FACTOR_ONE_MINUS_LOCAL_ALPHA:
         strcat(fragment_shader_texture1, "float texture1_alpha_factor = 1.0 - readtex1.a; \n");
         break;
      case GR_COMBINE_FACTOR_ONE_MINUS_DETAIL_FACTOR:
         strcat(fragment_shader_texture1, "float texture1_alpha_factor = 1.0 - lambda; \n");
         break;
   }
}

void
grTexCombine(
             int32_t tmu,
             int32_t rgb_function,
             int32_t rgb_factor,
             int32_t alpha_function,
             int32_t alpha_factor,
             int32_t rgb_invert,
             int32_t alpha_invert
             )
{
   int num_tex = 0;

   if (tmu == GR_TMU0)
      num_tex = 1;

   ccolor[tmu][0] = ccolor[tmu][1] = ccolor[tmu][2] = ccolor[tmu][3] = 0;

   if(num_tex == 0)
   {
      static int last_function = 0;
      static int last_factor = 0;
      static int last_afunction = 0;
      static int last_afactor = 0;
      static int last_rgb_invert = 0;

      if(last_function == rgb_function && last_factor == rgb_factor &&
            last_afunction == alpha_function && last_afactor == alpha_factor &&
            last_rgb_invert == rgb_invert && first_texture0 == 0 && !tex0_combiner_ext) return;
      first_texture0 = 0;
      tex0_combiner_ext = 0;

      last_function = rgb_function;
      last_factor = rgb_factor;
      last_afunction = alpha_function;
      last_afactor = alpha_factor;
      last_rgb_invert= rgb_invert;
      texture0_combiner_key = rgb_function | (rgb_factor << 4) |
         (alpha_function << 8) | (alpha_factor << 12) |
         (rgb_invert << 16);
      texture0_combinera_key = 0;
      strcpy(fragment_shader_texture0, "");

      switch(rgb_function)
      {
         case GR_COMBINE_FUNCTION_ZERO:
            strcat(fragment_shader_texture0, "vec4 ctexture0 = vec4(0.0); \n");
            break;
         case GR_COMBINE_FUNCTION_LOCAL:
            strcat(fragment_shader_texture0, "vec4 ctexture0 = readtex0; \n");
            break;
         case GR_COMBINE_FUNCTION_LOCAL_ALPHA:
            strcat(fragment_shader_texture0, "vec4 ctexture0 = vec4(readtex0.a); \n");
            break;
         case GR_COMBINE_FUNCTION_SCALE_OTHER:
            writeGLSLTextureColorFactorTMU0(num_tex, rgb_factor);
            strcat(fragment_shader_texture0, "vec4 ctexture0 = texture0_color_factor * vec4(0.0); \n");
            break;
         case GR_COMBINE_FUNCTION_SCALE_OTHER_ADD_LOCAL:
            writeGLSLTextureColorFactorTMU0(num_tex, rgb_factor);
            strcat(fragment_shader_texture0, "vec4 ctexture0 = texture0_color_factor * vec4(0.0) + readtex0; \n");
            break;
         case GR_COMBINE_FUNCTION_SCALE_OTHER_ADD_LOCAL_ALPHA:
            writeGLSLTextureColorFactorTMU0(num_tex, rgb_factor);
            strcat(fragment_shader_texture0, "vec4 ctexture0 = texture0_color_factor * vec4(0.0) + vec4(readtex0.a); \n");
            break;
         case GR_COMBINE_FUNCTION_SCALE_OTHER_MINUS_LOCAL:
            writeGLSLTextureColorFactorTMU0(num_tex, rgb_factor);
            strcat(fragment_shader_texture0, "vec4 ctexture0 = texture0_color_factor * (vec4(0.0) - readtex0); \n");
            break;
         case GR_COMBINE_FUNCTION_SCALE_OTHER_MINUS_LOCAL_ADD_LOCAL:
            writeGLSLTextureColorFactorTMU0(num_tex, rgb_factor);
            strcat(fragment_shader_texture0, "vec4 ctexture0 = texture0_color_factor * (vec4(0.0) - readtex0) + readtex0; \n");
            break;
         case GR_COMBINE_FUNCTION_SCALE_OTHER_MINUS_LOCAL_ADD_LOCAL_ALPHA:
            writeGLSLTextureColorFactorTMU0(num_tex, rgb_factor);
            strcat(fragment_shader_texture0, "vec4 ctexture0 = texture0_color_factor * (vec4(0.0) - readtex0) + vec4(readtex0.a); \n");
            break;
         case GR_COMBINE_FUNCTION_SCALE_MINUS_LOCAL_ADD_LOCAL:
            writeGLSLTextureColorFactorTMU0(num_tex, rgb_factor);
            strcat(fragment_shader_texture0, "vec4 ctexture0 = texture0_color_factor * (-readtex0) + readtex0; \n");
            break;
         case GR_COMBINE_FUNCTION_SCALE_MINUS_LOCAL_ADD_LOCAL_ALPHA:
            writeGLSLTextureColorFactorTMU0(num_tex, rgb_factor);
            strcat(fragment_shader_texture0, "vec4 ctexture0 = texture0_color_factor * (-readtex0) + vec4(readtex0.a); \n");
            break;
         default:
            strcat(fragment_shader_texture0, "vec4 ctexture0 = readtex0; \n");
      }

      if (rgb_invert)
         strcat(fragment_shader_texture0, "ctexture0 = vec4(1.0) - ctexture0; \n");

      switch(alpha_function)
      {
         case GR_COMBINE_FACTOR_ZERO:
            strcat(fragment_shader_texture0, "ctexture0.a = 0.0; \n");
            break;
         case GR_COMBINE_FUNCTION_LOCAL:
            strcat(fragment_shader_texture0, "ctexture0.a = readtex0.a; \n");
            break;
         case GR_COMBINE_FUNCTION_LOCAL_ALPHA:
            strcat(fragment_shader_texture0, "ctexture0.a = readtex0.a; \n");
            break;
         case GR_COMBINE_FUNCTION_SCALE_OTHER:
            writeGLSLTextureAlphaFactorTMU0(num_tex, alpha_factor);
            strcat(fragment_shader_texture0, "ctexture0.a = texture0_alpha_factor * 0.0; \n");
            break;
         case GR_COMBINE_FUNCTION_SCALE_OTHER_ADD_LOCAL:
            writeGLSLTextureAlphaFactorTMU0(num_tex, alpha_factor);
            strcat(fragment_shader_texture0, "ctexture0.a = texture0_alpha_factor * 0.0 + readtex0.a; \n");
            break;
         case GR_COMBINE_FUNCTION_SCALE_OTHER_ADD_LOCAL_ALPHA:
            writeGLSLTextureAlphaFactorTMU0(num_tex, alpha_factor);
            strcat(fragment_shader_texture0, "ctexture0.a = texture0_alpha_factor * 0.0 + readtex0.a; \n");
            break;
         case GR_COMBINE_FUNCTION_SCALE_OTHER_MINUS_LOCAL:
            writeGLSLTextureAlphaFactorTMU0(num_tex, alpha_factor);
            strcat(fragment_shader_texture0, "ctexture0.a = texture0_alpha_factor * (0.0 - readtex0.a); \n");
            break;
         case GR_COMBINE_FUNCTION_SCALE_OTHER_MINUS_LOCAL_ADD_LOCAL:
            writeGLSLTextureAlphaFactorTMU0(num_tex, alpha_factor);
            strcat(fragment_shader_texture0, "ctexture0.a = texture0_alpha_factor * (0.0 - readtex0.a) + readtex0.a; \n");
            break;
         case GR_COMBINE_FUNCTION_SCALE_OTHER_MINUS_LOCAL_ADD_LOCAL_ALPHA:
            writeGLSLTextureAlphaFactorTMU0(num_tex, alpha_factor);
            strcat(fragment_shader_texture0, "ctexture0.a = texture0_alpha_factor * (0.0 - readtex0.a) + readtex0.a; \n");
            break;
         case GR_COMBINE_FUNCTION_SCALE_MINUS_LOCAL_ADD_LOCAL:
            writeGLSLTextureAlphaFactorTMU0(num_tex, alpha_factor);
            strcat(fragment_shader_texture0, "ctexture0.a = texture0_alpha_factor * (-readtex0.a) + readtex0.a; \n");
            break;
         case GR_COMBINE_FUNCTION_SCALE_MINUS_LOCAL_ADD_LOCAL_ALPHA:
            writeGLSLTextureAlphaFactorTMU0(num_tex, alpha_factor);
            strcat(fragment_shader_texture0, "ctexture0.a = texture0_alpha_factor * (-readtex0.a) + readtex0.a; \n");
            break;
         default:
            strcat(fragment_shader_texture0, "ctexture0.a = readtex0.a; \n");
      }

      if (alpha_invert)
         strcat(fragment_shader_texture0, "ctexture0.a = 1.0 - ctexture0.a; \n");

      glUniform4f(current_shader->ccolor0_location, 0, 0, 0, 0);
   }
   else
   {
      static int last_function = 0;
      static int last_factor = 0;
      static int last_afunction = 0;
      static int last_afactor = 0;
      static int last_rgb_invert = 0;

      if(last_function == rgb_function && last_factor == rgb_factor &&
            last_afunction == alpha_function && last_afactor == alpha_factor &&
            last_rgb_invert == rgb_invert && first_texture1 == 0 && !tex1_combiner_ext) return;
      first_texture1 = 0;
      tex1_combiner_ext = 0;

      last_function = rgb_function;
      last_factor = rgb_factor;
      last_afunction = alpha_function;
      last_afactor = alpha_factor;
      last_rgb_invert = rgb_invert;

      texture1_combiner_key = rgb_function | (rgb_factor << 4) |
         (alpha_function << 8) | (alpha_factor << 12) |
         (rgb_invert << 16);
      texture1_combinera_key = 0;
      strcpy(fragment_shader_texture1, "");

      switch(rgb_function)
      {
         case GR_COMBINE_FUNCTION_ZERO:
            strcat(fragment_shader_texture1, "vec4 ctexture1 = vec4(0.0); \n");
            break;
         case GR_COMBINE_FUNCTION_LOCAL:
            strcat(fragment_shader_texture1, "vec4 ctexture1 = readtex1; \n");
            break;
         case GR_COMBINE_FUNCTION_LOCAL_ALPHA:
            strcat(fragment_shader_texture1, "vec4 ctexture1 = vec4(readtex1.a); \n");
            break;
         case GR_COMBINE_FUNCTION_SCALE_OTHER:
            writeGLSLTextureColorFactorTMU1(num_tex, rgb_factor);
            strcat(fragment_shader_texture1, "vec4 ctexture1 = texture1_color_factor * ctexture0; \n");
            break;
         case GR_COMBINE_FUNCTION_SCALE_OTHER_ADD_LOCAL:
            writeGLSLTextureColorFactorTMU1(num_tex, rgb_factor);
            strcat(fragment_shader_texture1, "vec4 ctexture1 = texture1_color_factor * ctexture0 + readtex1; \n");
            break;
         case GR_COMBINE_FUNCTION_SCALE_OTHER_ADD_LOCAL_ALPHA:
            writeGLSLTextureColorFactorTMU1(num_tex, rgb_factor);
            strcat(fragment_shader_texture1, "vec4 ctexture1 = texture1_color_factor * ctexture0 + vec4(readtex1.a); \n");
            break;
         case GR_COMBINE_FUNCTION_SCALE_OTHER_MINUS_LOCAL:
            writeGLSLTextureColorFactorTMU1(num_tex, rgb_factor);
            strcat(fragment_shader_texture1, "vec4 ctexture1 = texture1_color_factor * (ctexture0 - readtex1); \n");
            break;
         case GR_COMBINE_FUNCTION_SCALE_OTHER_MINUS_LOCAL_ADD_LOCAL:
            writeGLSLTextureColorFactorTMU1(num_tex, rgb_factor);
            strcat(fragment_shader_texture1, "vec4 ctexture1 = texture1_color_factor * (ctexture0 - readtex1) + readtex1; \n");
            break;
         case GR_COMBINE_FUNCTION_SCALE_OTHER_MINUS_LOCAL_ADD_LOCAL_ALPHA:
            writeGLSLTextureColorFactorTMU1(num_tex, rgb_factor);
            strcat(fragment_shader_texture1, "vec4 ctexture1 = texture1_color_factor * (ctexture0 - readtex1) + vec4(readtex1.a); \n");
            break;
         case GR_COMBINE_FUNCTION_SCALE_MINUS_LOCAL_ADD_LOCAL:
            writeGLSLTextureColorFactorTMU1(num_tex, rgb_factor);
            strcat(fragment_shader_texture1, "vec4 ctexture1 = texture1_color_factor * (-readtex1) + readtex1; \n");
            break;
         case GR_COMBINE_FUNCTION_SCALE_MINUS_LOCAL_ADD_LOCAL_ALPHA:
            writeGLSLTextureColorFactorTMU1(num_tex, rgb_factor);
            strcat(fragment_shader_texture1, "vec4 ctexture1 = texture1_color_factor * (-readtex1) + vec4(readtex1.a); \n");
            break;
         default:
            strcat(fragment_shader_texture1, "vec4 ctexture1 = readtex1; \n");
      }

      if (rgb_invert)
         strcat(fragment_shader_texture1, "ctexture1 = vec4(1.0) - ctexture1; \n");

      switch(alpha_function)
      {
         case GR_COMBINE_FACTOR_ZERO:
            strcat(fragment_shader_texture1, "ctexture1.a = 0.0; \n");
            break;
         case GR_COMBINE_FUNCTION_LOCAL:
            strcat(fragment_shader_texture1, "ctexture1.a = readtex1.a; \n");
            break;
         case GR_COMBINE_FUNCTION_LOCAL_ALPHA:
            strcat(fragment_shader_texture1, "ctexture1.a = readtex1.a; \n");
            break;
         case GR_COMBINE_FUNCTION_SCALE_OTHER:
            writeGLSLTextureAlphaFactorTMU1(num_tex, alpha_factor);
            strcat(fragment_shader_texture1, "ctexture1.a = texture1_alpha_factor * ctexture0.a; \n");
            break;
         case GR_COMBINE_FUNCTION_SCALE_OTHER_ADD_LOCAL:
            writeGLSLTextureAlphaFactorTMU1(num_tex, alpha_factor);
            strcat(fragment_shader_texture1, "ctexture1.a = texture1_alpha_factor * ctexture0.a + readtex1.a; \n");
            break;
         case GR_COMBINE_FUNCTION_SCALE_OTHER_ADD_LOCAL_ALPHA:
            writeGLSLTextureAlphaFactorTMU1(num_tex, alpha_factor);
            strcat(fragment_shader_texture1, "ctexture1.a = texture1_alpha_factor * ctexture0.a + readtex1.a; \n");
            break;
         case GR_COMBINE_FUNCTION_SCALE_OTHER_MINUS_LOCAL:
            writeGLSLTextureAlphaFactorTMU1(num_tex, alpha_factor);
            strcat(fragment_shader_texture1, "ctexture1.a = texture1_alpha_factor * (ctexture0.a - readtex1.a); \n");
            break;
         case GR_COMBINE_FUNCTION_SCALE_OTHER_MINUS_LOCAL_ADD_LOCAL:
            writeGLSLTextureAlphaFactorTMU1(num_tex, alpha_factor);
            strcat(fragment_shader_texture1, "ctexture1.a = texture1_alpha_factor * (ctexture0.a - readtex1.a) + readtex1.a; \n");
            break;
         case GR_COMBINE_FUNCTION_SCALE_OTHER_MINUS_LOCAL_ADD_LOCAL_ALPHA:
            writeGLSLTextureAlphaFactorTMU1(num_tex, alpha_factor);
            strcat(fragment_shader_texture1, "ctexture1.a = texture1_alpha_factor * (ctexture0.a - readtex1.a) + readtex1.a; \n");
            break;
         case GR_COMBINE_FUNCTION_SCALE_MINUS_LOCAL_ADD_LOCAL:
            writeGLSLTextureAlphaFactorTMU1(num_tex, alpha_factor);
            strcat(fragment_shader_texture1, "ctexture1.a = texture1_alpha_factor * (-readtex1.a) + readtex1.a; \n");
            break;
         case GR_COMBINE_FUNCTION_SCALE_MINUS_LOCAL_ADD_LOCAL_ALPHA:
            writeGLSLTextureAlphaFactorTMU1(num_tex, alpha_factor);
            strcat(fragment_shader_texture1, "ctexture1.a = texture1_alpha_factor * (-readtex1.a) + readtex1.a; \n");
            break;
         default:
            strcat(fragment_shader_texture1, "ctexture1.a = ctexture0.a; \n");
      }

      if (alpha_invert)
         strcat(fragment_shader_texture1, "ctexture1.a = 1.0 - ctexture1.a; \n");

      glUniform4f(current_shader->ccolor1_location, 0, 0, 0, 0);
   }

   need_to_compile = 1;
}

void grAlphaTestReferenceValue(uint8_t value)
{
   alpha_ref = value;
}

void grAlphaTestFunction( int32_t function, uint8_t value, int set_alpha_ref)
{
   alpha_func = function;
   alpha_test = (function == GR_CMP_ALWAYS) ? false : true;
   alpha_ref = (set_alpha_ref) ? value : alpha_ref;
}

void grFogMode( int32_t mode, uint32_t fogcolor)
{
   fog_enabled = mode;

   need_to_compile = 1;
}

// chroma

void grChromakeyMode( int32_t mode )
{
   switch(mode)
   {
      case GR_CHROMAKEY_DISABLE:
         chroma_enabled = 0;
         break;
      case GR_CHROMAKEY_ENABLE:
         chroma_enabled = 1;
         break;
   }
   need_to_compile = 1;
}

void grChromakeyValue( uint32_t value )
{
   chroma_color[0] = ((value >> 24) & 0xFF) / 255.0f;
   chroma_color[1] = ((value >> 16) & 0xFF) / 255.0f;
   chroma_color[2] = ((value >>  8) & 0xFF) / 255.0f;
   chroma_color[3] = 1.0;//(value & 0xFF) / 255.0f;

   glUniform4f(current_shader->chroma_color_location, chroma_color[0], chroma_color[1],
         chroma_color[2], chroma_color[3]);
}

void grStipplePattern(uint32_t stipple)
{
}

void grStippleMode( int32_t mode )
{
   switch(mode)
   {
      case GR_STIPPLE_DISABLE:
         dither_enabled = 0;
         break;
      case GR_STIPPLE_PATTERN:
      case GR_STIPPLE_ROTATE:
         dither_enabled = 1;
         break;
   }
   need_to_compile = 1;
}

void  grColorCombineExt(uint32_t a, uint32_t a_mode,
      uint32_t b, uint32_t b_mode,
      uint32_t c, int32_t c_invert,
      uint32_t d, int32_t d_invert,
      uint32_t shift, int32_t invert)
{
   color_combiner_key = 0x80000000 | (a & 0x1F) | ((a_mode & 3) << 5) |
      ((b & 0x1F) << 7) | ((b_mode & 3) << 12) |
      ((c & 0x1F) << 14) | ((c_invert & 1) << 19) |
      ((d & 0x1F) << 20) | ((d_invert & 1) << 25);
   c_combiner_ext = 1;
   strcpy(fragment_shader_color_combiner, "");

   switch(a)
   {
      case GR_CMBX_ZERO:
         strcat(fragment_shader_color_combiner, "vec4 cs_a = vec4(0.0); \n");
         break;
      case GR_CMBX_TEXTURE_ALPHA:
         strcat(fragment_shader_color_combiner, "vec4 cs_a = vec4(ctexture1.a); \n");
         break;
      case GR_CMBX_CONSTANT_ALPHA:
         strcat(fragment_shader_color_combiner, "vec4 cs_a = vec4(constant_color.a); \n");
         break;
      case GR_CMBX_CONSTANT_COLOR:
         strcat(fragment_shader_color_combiner, "vec4 cs_a = constant_color; \n");
         break;
      case GR_CMBX_ITALPHA:
         strcat(fragment_shader_color_combiner, "vec4 cs_a = vec4(vFrontColor.a); \n");
         break;
      case GR_CMBX_ITRGB:
         strcat(fragment_shader_color_combiner, "vec4 cs_a = vFrontColor; \n");
         break;
      case GR_CMBX_TEXTURE_RGB:
         strcat(fragment_shader_color_combiner, "vec4 cs_a = ctexture1; \n");
         break;
      default:
         strcat(fragment_shader_color_combiner, "vec4 cs_a = vec4(0.0); \n");
   }

   switch(a_mode)
   {
      case GR_FUNC_MODE_ZERO:
         strcat(fragment_shader_color_combiner, "vec4 c_a = vec4(0.0); \n");
         break;
      case GR_FUNC_MODE_X:
         strcat(fragment_shader_color_combiner, "vec4 c_a = cs_a; \n");
         break;
      case GR_FUNC_MODE_ONE_MINUS_X:
         strcat(fragment_shader_color_combiner, "vec4 c_a = vec4(1.0) - cs_a; \n");
         break;
      case GR_FUNC_MODE_NEGATIVE_X:
         strcat(fragment_shader_color_combiner, "vec4 c_a = -cs_a; \n");
         break;
      default:
         strcat(fragment_shader_color_combiner, "vec4 c_a = vec4(0.0); \n");
   }

   switch(b)
   {
      case GR_CMBX_ZERO:
         strcat(fragment_shader_color_combiner, "vec4 cs_b = vec4(0.0); \n");
         break;
      case GR_CMBX_TEXTURE_ALPHA:
         strcat(fragment_shader_color_combiner, "vec4 cs_b = vec4(ctexture1.a); \n");
         break;
      case GR_CMBX_CONSTANT_ALPHA:
         strcat(fragment_shader_color_combiner, "vec4 cs_b = vec4(constant_color.a); \n");
         break;
      case GR_CMBX_CONSTANT_COLOR:
         strcat(fragment_shader_color_combiner, "vec4 cs_b = constant_color; \n");
         break;
      case GR_CMBX_ITALPHA:
         strcat(fragment_shader_color_combiner, "vec4 cs_b = vec4(vFrontColor.a); \n");
         break;
      case GR_CMBX_ITRGB:
         strcat(fragment_shader_color_combiner, "vec4 cs_b = vFrontColor; \n");
         break;
      case GR_CMBX_TEXTURE_RGB:
         strcat(fragment_shader_color_combiner, "vec4 cs_b = ctexture1; \n");
         break;
      default:
         strcat(fragment_shader_color_combiner, "vec4 cs_b = vec4(0.0); \n");
   }

   switch(b_mode)
   {
      case GR_FUNC_MODE_ZERO:
         strcat(fragment_shader_color_combiner, "vec4 c_b = vec4(0.0); \n");
         break;
      case GR_FUNC_MODE_X:
         strcat(fragment_shader_color_combiner, "vec4 c_b = cs_b; \n");
         break;
      case GR_FUNC_MODE_ONE_MINUS_X:
         strcat(fragment_shader_color_combiner, "vec4 c_b = vec4(1.0) - cs_b; \n");
         break;
      case GR_FUNC_MODE_NEGATIVE_X:
         strcat(fragment_shader_color_combiner, "vec4 c_b = -cs_b; \n");
         break;
      default:
         strcat(fragment_shader_color_combiner, "vec4 c_b = vec4(0.0); \n");
   }

   switch(c)
   {
      case GR_CMBX_ZERO:
         strcat(fragment_shader_color_combiner, "vec4 c_c = vec4(0.0); \n");
         break;
      case GR_CMBX_TEXTURE_ALPHA:
         strcat(fragment_shader_color_combiner, "vec4 c_c = vec4(ctexture1.a); \n");
         break;
      case GR_CMBX_ALOCAL:
         strcat(fragment_shader_color_combiner, "vec4 c_c = vec4(c_b.a); \n");
         break;
      case GR_CMBX_AOTHER:
         strcat(fragment_shader_color_combiner, "vec4 c_c = vec4(c_a.a); \n");
         break;
      case GR_CMBX_B:
         strcat(fragment_shader_color_combiner, "vec4 c_c = cs_b; \n");
         break;
      case GR_CMBX_CONSTANT_ALPHA:
         strcat(fragment_shader_color_combiner, "vec4 c_c = vec4(constant_color.a); \n");
         break;
      case GR_CMBX_CONSTANT_COLOR:
         strcat(fragment_shader_color_combiner, "vec4 c_c = constant_color; \n");
         break;
      case GR_CMBX_ITALPHA:
         strcat(fragment_shader_color_combiner, "vec4 c_c = vec4(vFrontColor.a); \n");
         break;
      case GR_CMBX_ITRGB:
         strcat(fragment_shader_color_combiner, "vec4 c_c = vFrontColor; \n");
         break;
      case GR_CMBX_TEXTURE_RGB:
         strcat(fragment_shader_color_combiner, "vec4 c_c = ctexture1; \n");
         break;
      default:
         strcat(fragment_shader_color_combiner, "vec4 c_c = vec4(0.0); \n");
   }

   if(c_invert)
      strcat(fragment_shader_color_combiner, "c_c = vec4(1.0) - c_c; \n");

   switch(d)
   {
      case GR_CMBX_ZERO:
         strcat(fragment_shader_color_combiner, "vec4 c_d = vec4(0.0); \n");
         break;
      case GR_CMBX_ALOCAL:
         strcat(fragment_shader_color_combiner, "vec4 c_d = vec4(c_b.a); \n");
         break;
      case GR_CMBX_B:
         strcat(fragment_shader_color_combiner, "vec4 c_d = cs_b; \n");
         break;
      case GR_CMBX_TEXTURE_RGB:
         strcat(fragment_shader_color_combiner, "vec4 c_d = ctexture1; \n");
         break;
      case GR_CMBX_ITRGB:
         strcat(fragment_shader_color_combiner, "vec4 c_d = vFrontColor; \n");
         break;
      default:
         strcat(fragment_shader_color_combiner, "vec4 c_d = vec4(0.0); \n");
   }

   if(d_invert)
      strcat(fragment_shader_color_combiner, "c_d = vec4(1.0) - c_d; \n");

   strcat(fragment_shader_color_combiner, "gl_FragColor = (c_a + c_b) * c_c + c_d; \n");

   need_to_compile = 1;
}

void grAlphaCombineExt(uint32_t a, uint32_t a_mode,
      uint32_t b, uint32_t b_mode,
      uint32_t c, int32_t c_invert,
      uint32_t d, int32_t d_invert,
      uint32_t shift, int32_t invert)
{
   alpha_combiner_key = 0x80000000 | (a & 0x1F) | ((a_mode & 3) << 5) |
      ((b & 0x1F) << 7) | ((b_mode & 3) << 12) |
      ((c & 0x1F) << 14) | ((c_invert & 1) << 19) |
      ((d & 0x1F) << 20) | ((d_invert & 1) << 25);
   a_combiner_ext = 1;
   strcpy(fragment_shader_alpha_combiner, "");

   switch(a)
   {
      case GR_CMBX_ZERO:
         strcat(fragment_shader_alpha_combiner, "float as_a = 0.0; \n");
         break;
      case GR_CMBX_TEXTURE_ALPHA:
         strcat(fragment_shader_alpha_combiner, "float as_a = ctexture1.a; \n");
         break;
      case GR_CMBX_CONSTANT_ALPHA:
         strcat(fragment_shader_alpha_combiner, "float as_a = constant_color.a; \n");
         break;
      case GR_CMBX_ITALPHA:
         strcat(fragment_shader_alpha_combiner, "float as_a = vFrontColor.a; \n");
         break;
      default:
         strcat(fragment_shader_alpha_combiner, "float as_a = 0.0; \n");
   }

   switch(a_mode)
   {
      case GR_FUNC_MODE_ZERO:
         strcat(fragment_shader_alpha_combiner, "float a_a = 0.0; \n");
         break;
      case GR_FUNC_MODE_X:
         strcat(fragment_shader_alpha_combiner, "float a_a = as_a; \n");
         break;
      case GR_FUNC_MODE_ONE_MINUS_X:
         strcat(fragment_shader_alpha_combiner, "float a_a = 1.0 - as_a; \n");
         break;
      case GR_FUNC_MODE_NEGATIVE_X:
         strcat(fragment_shader_alpha_combiner, "float a_a = -as_a; \n");
         break;
      default:
         strcat(fragment_shader_alpha_combiner, "float a_a = 0.0; \n");
   }

   switch(b)
   {
      case GR_CMBX_ZERO:
         strcat(fragment_shader_alpha_combiner, "float as_b = 0.0; \n");
         break;
      case GR_CMBX_TEXTURE_ALPHA:
         strcat(fragment_shader_alpha_combiner, "float as_b = ctexture1.a; \n");
         break;
      case GR_CMBX_CONSTANT_ALPHA:
         strcat(fragment_shader_alpha_combiner, "float as_b = constant_color.a; \n");
         break;
      case GR_CMBX_ITALPHA:
         strcat(fragment_shader_alpha_combiner, "float as_b = vFrontColor.a; \n");
         break;
      default:
         strcat(fragment_shader_alpha_combiner, "float as_b = 0.0; \n");
   }

   switch(b_mode)
   {
      case GR_FUNC_MODE_ZERO:
         strcat(fragment_shader_alpha_combiner, "float a_b = 0.0; \n");
         break;
      case GR_FUNC_MODE_X:
         strcat(fragment_shader_alpha_combiner, "float a_b = as_b; \n");
         break;
      case GR_FUNC_MODE_ONE_MINUS_X:
         strcat(fragment_shader_alpha_combiner, "float a_b = 1.0 - as_b; \n");
         break;
      case GR_FUNC_MODE_NEGATIVE_X:
         strcat(fragment_shader_alpha_combiner, "float a_b = -as_b; \n");
         break;
      default:
         strcat(fragment_shader_alpha_combiner, "float a_b = 0.0; \n");
   }

   switch(c)
   {
      case GR_CMBX_ZERO:
         strcat(fragment_shader_alpha_combiner, "float a_c = 0.0; \n");
         break;
      case GR_CMBX_TEXTURE_ALPHA:
         strcat(fragment_shader_alpha_combiner, "float a_c = ctexture1.a; \n");
         break;
      case GR_CMBX_ALOCAL:
         strcat(fragment_shader_alpha_combiner, "float a_c = as_b; \n");
         break;
      case GR_CMBX_AOTHER:
         strcat(fragment_shader_alpha_combiner, "float a_c = as_a; \n");
         break;
      case GR_CMBX_B:
         strcat(fragment_shader_alpha_combiner, "float a_c = as_b; \n");
         break;
      case GR_CMBX_CONSTANT_ALPHA:
         strcat(fragment_shader_alpha_combiner, "float a_c = constant_color.a; \n");
         break;
      case GR_CMBX_ITALPHA:
         strcat(fragment_shader_alpha_combiner, "float a_c = vFrontColor.a; \n");
         break;
      default:
         strcat(fragment_shader_alpha_combiner, "float a_c = 0.0; \n");
   }

   if(c_invert)
      strcat(fragment_shader_alpha_combiner, "a_c = 1.0 - a_c; \n");

   switch(d)
   {
      case GR_CMBX_ZERO:
         strcat(fragment_shader_alpha_combiner, "float a_d = 0.0; \n");
         break;
      case GR_CMBX_TEXTURE_ALPHA:
         strcat(fragment_shader_alpha_combiner, "float a_d = ctexture1.a; \n");
         break;
      case GR_CMBX_ALOCAL:
         strcat(fragment_shader_alpha_combiner, "float a_d = as_b; \n");
         break;
      case GR_CMBX_B:
         strcat(fragment_shader_alpha_combiner, "float a_d = as_b; \n");
         break;
      default:
         strcat(fragment_shader_alpha_combiner, "float a_d = 0.0; \n");
   }

   if(d_invert)
      strcat(fragment_shader_alpha_combiner, "a_d = 1.0 - a_d; \n");

   strcat(fragment_shader_alpha_combiner, "gl_FragColor.a = (a_a + a_b) * a_c + a_d; \n");

   need_to_compile = 1;
}

void
grTexColorCombineExt(int32_t       tmu,
      uint32_t a, uint32_t a_mode,
      uint32_t b, uint32_t b_mode,
      uint32_t c, int32_t c_invert,
      uint32_t d, int32_t d_invert,
      uint32_t shift, int32_t invert)
{
   int num_tex = 0;

   if (tmu == GR_TMU0)
      num_tex = 1;

   if(num_tex == 0)
   {
      texture0_combiner_key = 0x80000000 | (a & 0x1F) | ((a_mode & 3) << 5) |
         ((b & 0x1F) << 7) | ((b_mode & 3) << 12) |
         ((c & 0x1F) << 14) | ((c_invert & 1) << 19) |
         ((d & 0x1F) << 20) | ((d_invert & 1) << 25);
      tex0_combiner_ext = 1;
      strcpy(fragment_shader_texture0, "");

      switch(a)
      {
         case GR_CMBX_ZERO:
            strcat(fragment_shader_texture0, "vec4 ctex0s_a = vec4(0.0); \n");
            break;
         case GR_CMBX_ITALPHA:
            strcat(fragment_shader_texture0, "vec4 ctex0s_a = vec4(vFrontColor.a); \n");
            break;
         case GR_CMBX_ITRGB:
            strcat(fragment_shader_texture0, "vec4 ctex0s_a = vFrontColor; \n");
            break;
         case GR_CMBX_LOCAL_TEXTURE_ALPHA:
            strcat(fragment_shader_texture0, "vec4 ctex0s_a = vec4(readtex0.a); \n");
            break;
         case GR_CMBX_LOCAL_TEXTURE_RGB:
            strcat(fragment_shader_texture0, "vec4 ctex0s_a = readtex0; \n");
            break;
         case GR_CMBX_OTHER_TEXTURE_ALPHA:
            strcat(fragment_shader_texture0, "vec4 ctex0s_a = vec4(0.0); \n");
            break;
         case GR_CMBX_OTHER_TEXTURE_RGB:
            strcat(fragment_shader_texture0, "vec4 ctex0s_a = vec4(0.0); \n");
            break;
         case GR_CMBX_TMU_CCOLOR:
            strcat(fragment_shader_texture0, "vec4 ctex0s_a = ccolor0; \n");
            break;
         case GR_CMBX_TMU_CALPHA:
            strcat(fragment_shader_texture0, "vec4 ctex0s_a = vec4(ccolor0.a); \n");
            break;
         default:
            strcat(fragment_shader_texture0, "vec4 ctex0s_a = vec4(0.0); \n");
      }

      switch(a_mode)
      {
         case GR_FUNC_MODE_ZERO:
            strcat(fragment_shader_texture0, "vec4 ctex0_a = vec4(0.0); \n");
            break;
         case GR_FUNC_MODE_X:
            strcat(fragment_shader_texture0, "vec4 ctex0_a = ctex0s_a; \n");
            break;
         case GR_FUNC_MODE_ONE_MINUS_X:
            strcat(fragment_shader_texture0, "vec4 ctex0_a = vec4(1.0) - ctex0s_a; \n");
            break;
         case GR_FUNC_MODE_NEGATIVE_X:
            strcat(fragment_shader_texture0, "vec4 ctex0_a = -ctex0s_a; \n");
            break;
         default:
            strcat(fragment_shader_texture0, "vec4 ctex0_a = vec4(0.0); \n");
      }

      switch(b)
      {
         case GR_CMBX_ZERO:
            strcat(fragment_shader_texture0, "vec4 ctex0s_b = vec4(0.0); \n");
            break;
         case GR_CMBX_ITALPHA:
            strcat(fragment_shader_texture0, "vec4 ctex0s_b = vec4(vFrontColor.a); \n");
            break;
         case GR_CMBX_ITRGB:
            strcat(fragment_shader_texture0, "vec4 ctex0s_b = vFrontColor; \n");
            break;
         case GR_CMBX_LOCAL_TEXTURE_ALPHA:
            strcat(fragment_shader_texture0, "vec4 ctex0s_b = vec4(readtex0.a); \n");
            break;
         case GR_CMBX_LOCAL_TEXTURE_RGB:
            strcat(fragment_shader_texture0, "vec4 ctex0s_b = readtex0; \n");
            break;
         case GR_CMBX_OTHER_TEXTURE_ALPHA:
            strcat(fragment_shader_texture0, "vec4 ctex0s_b = vec4(0.0); \n");
            break;
         case GR_CMBX_OTHER_TEXTURE_RGB:
            strcat(fragment_shader_texture0, "vec4 ctex0s_b = vec4(0.0); \n");
            break;
         case GR_CMBX_TMU_CALPHA:
            strcat(fragment_shader_texture0, "vec4 ctex0s_b = vec4(ccolor0.a); \n");
            break;
         case GR_CMBX_TMU_CCOLOR:
            strcat(fragment_shader_texture0, "vec4 ctex0s_b = ccolor0; \n");
            break;
         default:
            strcat(fragment_shader_texture0, "vec4 ctex0s_b = vec4(0.0); \n");
      }

      switch(b_mode)
      {
         case GR_FUNC_MODE_ZERO:
            strcat(fragment_shader_texture0, "vec4 ctex0_b = vec4(0.0); \n");
            break;
         case GR_FUNC_MODE_X:
            strcat(fragment_shader_texture0, "vec4 ctex0_b = ctex0s_b; \n");
            break;
         case GR_FUNC_MODE_ONE_MINUS_X:
            strcat(fragment_shader_texture0, "vec4 ctex0_b = vec4(1.0) - ctex0s_b; \n");
            break;
         case GR_FUNC_MODE_NEGATIVE_X:
            strcat(fragment_shader_texture0, "vec4 ctex0_b = -ctex0s_b; \n");
            break;
         default:
            strcat(fragment_shader_texture0, "vec4 ctex0_b = vec4(0.0); \n");
      }

      switch(c)
      {
         case GR_CMBX_ZERO:
            strcat(fragment_shader_texture0, "vec4 ctex0_c = vec4(0.0); \n");
            break;
         case GR_CMBX_B:
            strcat(fragment_shader_texture0, "vec4 ctex0_c = ctex0s_b; \n");
            break;
         case GR_CMBX_DETAIL_FACTOR:
            strcat(fragment_shader_texture0, "vec4 ctex0_c = vec4(lambda); \n");
            break;
         case GR_CMBX_ITRGB:
            strcat(fragment_shader_texture0, "vec4 ctex0_c = vFrontColor; \n");
            break;
         case GR_CMBX_ITALPHA:
            strcat(fragment_shader_texture0, "vec4 ctex0_c = vec4(vFrontColor.a); \n");
            break;
         case GR_CMBX_LOCAL_TEXTURE_ALPHA:
            strcat(fragment_shader_texture0, "vec4 ctex0_c = vec4(readtex0.a); \n");
            break;
         case GR_CMBX_LOCAL_TEXTURE_RGB:
            strcat(fragment_shader_texture0, "vec4 ctex0_c = readtex0; \n");
            break;
         case GR_CMBX_OTHER_TEXTURE_ALPHA:
            strcat(fragment_shader_texture0, "vec4 ctex0_c = vec4(0.0); \n");
            break;
         case GR_CMBX_OTHER_TEXTURE_RGB:
            strcat(fragment_shader_texture0, "vec4 ctex0_c = vec4(0.0); \n");
            break;
         case GR_CMBX_TMU_CALPHA:
            strcat(fragment_shader_texture0, "vec4 ctex0_c = vec4(ccolor0.a); \n");
            break;
         case GR_CMBX_TMU_CCOLOR:
            strcat(fragment_shader_texture0, "vec4 ctex0_c = ccolor0; \n");
            break;
         default:
            strcat(fragment_shader_texture0, "vec4 ctex0_c = vec4(0.0); \n");
      }

      if(c_invert)
         strcat(fragment_shader_texture0, "ctex0_c = vec4(1.0) - ctex0_c; \n");

      switch(d)
      {
         case GR_CMBX_ZERO:
            strcat(fragment_shader_texture0, "vec4 ctex0_d = vec4(0.0); \n");
            break;
         case GR_CMBX_B:
            strcat(fragment_shader_texture0, "vec4 ctex0_d = ctex0s_b; \n");
            break;
         case GR_CMBX_ITRGB:
            strcat(fragment_shader_texture0, "vec4 ctex0_d = vFrontColor; \n");
            break;
         case GR_CMBX_LOCAL_TEXTURE_ALPHA:
            strcat(fragment_shader_texture0, "vec4 ctex0_d = vec4(readtex0.a); \n");
            break;
         default:
            strcat(fragment_shader_texture0, "vec4 ctex0_d = vec4(0.0); \n");
      }

      if(d_invert)
         strcat(fragment_shader_texture0, "ctex0_d = vec4(1.0) - ctex0_d; \n");

      strcat(fragment_shader_texture0, "vec4 ctexture0 = (ctex0_a + ctex0_b) * ctex0_c + ctex0_d; \n");
   }
   else
   {
      texture1_combiner_key = 0x80000000 | (a & 0x1F) | ((a_mode & 3) << 5) |
         ((b & 0x1F) << 7) | ((b_mode & 3) << 12) |
         ((c & 0x1F) << 14) | ((c_invert & 1) << 19) |
         ((d & 0x1F) << 20) | ((d_invert & 1) << 25);
      tex1_combiner_ext = 1;
      strcpy(fragment_shader_texture1, "");

      switch(a)
      {
         case GR_CMBX_ZERO:
            strcat(fragment_shader_texture1, "vec4 ctex1s_a = vec4(0.0); \n");
            break;
         case GR_CMBX_ITALPHA:
            strcat(fragment_shader_texture1, "vec4 ctex1s_a = vec4(vFrontColor.a); \n");
            break;
         case GR_CMBX_ITRGB:
            strcat(fragment_shader_texture1, "vec4 ctex1s_a = vFrontColor; \n");
            break;
         case GR_CMBX_LOCAL_TEXTURE_ALPHA:
            strcat(fragment_shader_texture1, "vec4 ctex1s_a = vec4(readtex1.a); \n");
            break;
         case GR_CMBX_LOCAL_TEXTURE_RGB:
            strcat(fragment_shader_texture1, "vec4 ctex1s_a = readtex1; \n");
            break;
         case GR_CMBX_OTHER_TEXTURE_ALPHA:
            strcat(fragment_shader_texture1, "vec4 ctex1s_a = vec4(ctexture0.a); \n");
            break;
         case GR_CMBX_OTHER_TEXTURE_RGB:
            strcat(fragment_shader_texture1, "vec4 ctex1s_a = ctexture0; \n");
            break;
         case GR_CMBX_TMU_CCOLOR:
            strcat(fragment_shader_texture1, "vec4 ctex1s_a = ccolor1; \n");
            break;
         case GR_CMBX_TMU_CALPHA:
            strcat(fragment_shader_texture1, "vec4 ctex1s_a = vec4(ccolor1.a); \n");
            break;
         default:
            strcat(fragment_shader_texture1, "vec4 ctex1s_a = vec4(0.0); \n");
      }

      switch(a_mode)
      {
         case GR_FUNC_MODE_ZERO:
            strcat(fragment_shader_texture1, "vec4 ctex1_a = vec4(0.0); \n");
            break;
         case GR_FUNC_MODE_X:
            strcat(fragment_shader_texture1, "vec4 ctex1_a = ctex1s_a; \n");
            break;
         case GR_FUNC_MODE_ONE_MINUS_X:
            strcat(fragment_shader_texture1, "vec4 ctex1_a = vec4(1.0) - ctex1s_a; \n");
            break;
         case GR_FUNC_MODE_NEGATIVE_X:
            strcat(fragment_shader_texture1, "vec4 ctex1_a = -ctex1s_a; \n");
            break;
         default:
            strcat(fragment_shader_texture1, "vec4 ctex1_a = vec4(0.0); \n");
      }

      switch(b)
      {
         case GR_CMBX_ZERO:
            strcat(fragment_shader_texture1, "vec4 ctex1s_b = vec4(0.0); \n");
            break;
         case GR_CMBX_ITALPHA:
            strcat(fragment_shader_texture1, "vec4 ctex1s_b = vec4(vFrontColor.a); \n");
            break;
         case GR_CMBX_ITRGB:
            strcat(fragment_shader_texture1, "vec4 ctex1s_b = vFrontColor; \n");
            break;
         case GR_CMBX_LOCAL_TEXTURE_ALPHA:
            strcat(fragment_shader_texture1, "vec4 ctex1s_b = vec4(readtex1.a); \n");
            break;
         case GR_CMBX_LOCAL_TEXTURE_RGB:
            strcat(fragment_shader_texture1, "vec4 ctex1s_b = readtex1; \n");
            break;
         case GR_CMBX_OTHER_TEXTURE_ALPHA:
            strcat(fragment_shader_texture1, "vec4 ctex1s_b = vec4(ctexture0.a); \n");
            break;
         case GR_CMBX_OTHER_TEXTURE_RGB:
            strcat(fragment_shader_texture1, "vec4 ctex1s_b = ctexture0; \n");
            break;
         case GR_CMBX_TMU_CALPHA:
            strcat(fragment_shader_texture1, "vec4 ctex1s_b = vec4(ccolor1.a); \n");
            break;
         case GR_CMBX_TMU_CCOLOR:
            strcat(fragment_shader_texture1, "vec4 ctex1s_b = ccolor1; \n");
            break;
         default:
            strcat(fragment_shader_texture1, "vec4 ctex1s_b = vec4(0.0); \n");
      }

      switch(b_mode)
      {
         case GR_FUNC_MODE_ZERO:
            strcat(fragment_shader_texture1, "vec4 ctex1_b = vec4(0.0); \n");
            break;
         case GR_FUNC_MODE_X:
            strcat(fragment_shader_texture1, "vec4 ctex1_b = ctex1s_b; \n");
            break;
         case GR_FUNC_MODE_ONE_MINUS_X:
            strcat(fragment_shader_texture1, "vec4 ctex1_b = vec4(1.0) - ctex1s_b; \n");
            break;
         case GR_FUNC_MODE_NEGATIVE_X:
            strcat(fragment_shader_texture1, "vec4 ctex1_b = -ctex1s_b; \n");
            break;
         default:
            strcat(fragment_shader_texture1, "vec4 ctex1_b = vec4(0.0); \n");
      }

      switch(c)
      {
         case GR_CMBX_ZERO:
            strcat(fragment_shader_texture1, "vec4 ctex1_c = vec4(0.0); \n");
            break;
         case GR_CMBX_B:
            strcat(fragment_shader_texture1, "vec4 ctex1_c = ctex1s_b; \n");
            break;
         case GR_CMBX_DETAIL_FACTOR:
            strcat(fragment_shader_texture1, "vec4 ctex1_c = vec4(lambda); \n");
            break;
         case GR_CMBX_ITRGB:
            strcat(fragment_shader_texture1, "vec4 ctex1_c = vFrontColor; \n");
            break;
         case GR_CMBX_ITALPHA:
            strcat(fragment_shader_texture1, "vec4 ctex1_c = vec4(vFrontColor.a); \n");
            break;
         case GR_CMBX_LOCAL_TEXTURE_ALPHA:
            strcat(fragment_shader_texture1, "vec4 ctex1_c = vec4(readtex1.a); \n");
            break;
         case GR_CMBX_LOCAL_TEXTURE_RGB:
            strcat(fragment_shader_texture1, "vec4 ctex1_c = readtex1; \n");
            break;
         case GR_CMBX_OTHER_TEXTURE_ALPHA:
            strcat(fragment_shader_texture1, "vec4 ctex1_c = vec4(ctexture0.a); \n");
            break;
         case GR_CMBX_OTHER_TEXTURE_RGB:
            strcat(fragment_shader_texture1, "vec4 ctex1_c = ctexture0; \n");
            break;
         case GR_CMBX_TMU_CALPHA:
            strcat(fragment_shader_texture1, "vec4 ctex1_c = vec4(ccolor1.a); \n");
            break;
         case GR_CMBX_TMU_CCOLOR:
            strcat(fragment_shader_texture1, "vec4 ctex1_c = ccolor1; \n");
            break;
         default:
            strcat(fragment_shader_texture1, "vec4 ctex1_c = vec4(0.0); \n");
      }

      if(c_invert)
         strcat(fragment_shader_texture1, "ctex1_c = vec4(1.0) - ctex1_c; \n");

      switch(d)
      {
         case GR_CMBX_ZERO:
            strcat(fragment_shader_texture1, "vec4 ctex1_d = vec4(0.0); \n");
            break;
         case GR_CMBX_B:
            strcat(fragment_shader_texture1, "vec4 ctex1_d = ctex1s_b; \n");
            break;
         case GR_CMBX_ITRGB:
            strcat(fragment_shader_texture1, "vec4 ctex1_d = vFrontColor; \n");
            break;
         case GR_CMBX_LOCAL_TEXTURE_ALPHA:
            strcat(fragment_shader_texture1, "vec4 ctex1_d = vec4(readtex1.a); \n");
            break;
         default:
            strcat(fragment_shader_texture1, "vec4 ctex1_d = vec4(0.0); \n");
      }

      if(d_invert)
         strcat(fragment_shader_texture1, "ctex1_d = vec4(1.0) - ctex1_d; \n");

      strcat(fragment_shader_texture1, "vec4 ctexture1 = (ctex1_a + ctex1_b) * ctex1_c + ctex1_d; \n");
   }

   need_to_compile = 1;
}

void
grTexAlphaCombineExt(int32_t       tmu,
      uint32_t a, uint32_t a_mode,
      uint32_t b, uint32_t b_mode,
      uint32_t c, int32_t c_invert,
      uint32_t d, int32_t d_invert,
      uint32_t shift, int32_t invert,
      uint32_t     ccolor_value)
{
   int num_tex = 0;

   if (tmu == GR_TMU0)
      num_tex = 1;

   ccolor[num_tex][0] = ((ccolor_value >> 24) & 0xFF) / 255.0f;
   ccolor[num_tex][1] = ((ccolor_value >> 16) & 0xFF) / 255.0f;
   ccolor[num_tex][2] = ((ccolor_value >>  8) & 0xFF) / 255.0f;
   ccolor[num_tex][3] = (ccolor_value & 0xFF) / 255.0f;

   if(num_tex == 0)
   {
      texture0_combinera_key = 0x80000000 | (a & 0x1F) | ((a_mode & 3) << 5) |
         ((b & 0x1F) << 7) | ((b_mode & 3) << 12) |
         ((c & 0x1F) << 14) | ((c_invert & 1) << 19) |
         ((d & 0x1F) << 20) | ((d_invert & 1) << 25);

      switch(a)
      {
         case GR_CMBX_ITALPHA:
            strcat(fragment_shader_texture0, "ctex0s_a.a = vFrontColor.a; \n");
            break;
         case GR_CMBX_LOCAL_TEXTURE_ALPHA:
            strcat(fragment_shader_texture0, "ctex0s_a.a = readtex0.a; \n");
            break;
         case GR_CMBX_OTHER_TEXTURE_ALPHA:
            strcat(fragment_shader_texture0, "ctex0s_a.a = 0.0; \n");
            break;
         case GR_CMBX_TMU_CALPHA:
            strcat(fragment_shader_texture0, "ctex0s_a.a = ccolor0.a; \n");
            break;
         default:
            strcat(fragment_shader_texture0, "ctex0s_a.a = 0.0; \n");
      }

      switch(a_mode)
      {
         case GR_FUNC_MODE_ZERO:
            strcat(fragment_shader_texture0, "ctex0_a.a = 0.0; \n");
            break;
         case GR_FUNC_MODE_X:
            strcat(fragment_shader_texture0, "ctex0_a.a = ctex0s_a.a; \n");
            break;
         case GR_FUNC_MODE_ONE_MINUS_X:
            strcat(fragment_shader_texture0, "ctex0_a.a = 1.0 - ctex0s_a.a; \n");
            break;
         case GR_FUNC_MODE_NEGATIVE_X:
            strcat(fragment_shader_texture0, "ctex0_a.a = -ctex0s_a.a; \n");
            break;
         default:
            strcat(fragment_shader_texture0, "ctex0_a.a = 0.0; \n");
      }

      switch(b)
      {
         case GR_CMBX_ITALPHA:
            strcat(fragment_shader_texture0, "ctex0s_b.a = vFrontColor.a; \n");
            break;
         case GR_CMBX_LOCAL_TEXTURE_ALPHA:
            strcat(fragment_shader_texture0, "ctex0s_b.a = readtex0.a; \n");
            break;
         case GR_CMBX_OTHER_TEXTURE_ALPHA:
            strcat(fragment_shader_texture0, "ctex0s_b.a = 0.0; \n");
            break;
         case GR_CMBX_TMU_CALPHA:
            strcat(fragment_shader_texture0, "ctex0s_b.a = ccolor0.a; \n");
            break;
         default:
            strcat(fragment_shader_texture0, "ctex0s_b.a = 0.0; \n");
      }

      switch(b_mode)
      {
         case GR_FUNC_MODE_ZERO:
            strcat(fragment_shader_texture0, "ctex0_b.a = 0.0; \n");
            break;
         case GR_FUNC_MODE_X:
            strcat(fragment_shader_texture0, "ctex0_b.a = ctex0s_b.a; \n");
            break;
         case GR_FUNC_MODE_ONE_MINUS_X:
            strcat(fragment_shader_texture0, "ctex0_b.a = 1.0 - ctex0s_b.a; \n");
            break;
         case GR_FUNC_MODE_NEGATIVE_X:
            strcat(fragment_shader_texture0, "ctex0_b.a = -ctex0s_b.a; \n");
            break;
         default:
            strcat(fragment_shader_texture0, "ctex0_b.a = 0.0; \n");
      }

      switch(c)
      {
         case GR_CMBX_ZERO:
            strcat(fragment_shader_texture0, "ctex0_c.a = 0.0; \n");
            break;
         case GR_CMBX_B:
            strcat(fragment_shader_texture0, "ctex0_c.a = ctex0s_b.a; \n");
            break;
         case GR_CMBX_DETAIL_FACTOR:
            strcat(fragment_shader_texture0, "ctex0_c.a = lambda; \n");
            break;
         case GR_CMBX_ITALPHA:
            strcat(fragment_shader_texture0, "ctex0_c.a = vFrontColor.a; \n");
            break;
         case GR_CMBX_LOCAL_TEXTURE_ALPHA:
            strcat(fragment_shader_texture0, "ctex0_c.a = readtex0.a; \n");
            break;
         case GR_CMBX_OTHER_TEXTURE_ALPHA:
            strcat(fragment_shader_texture0, "ctex0_c.a = 0.0; \n");
            break;
         case GR_CMBX_TMU_CALPHA:
            strcat(fragment_shader_texture0, "ctex0_c.a = ccolor0.a; \n");
            break;
         default:
            strcat(fragment_shader_texture0, "ctex0_c.a = 0.0; \n");
      }

      switch(d)
      {
         case GR_CMBX_ZERO:
            strcat(fragment_shader_texture0, "ctex0_d.a = 0.0; \n");
            break;
         case GR_CMBX_B:
            strcat(fragment_shader_texture0, "ctex0_d.a = ctex0s_b.a; \n");
            break;
         case GR_CMBX_ITALPHA:
            strcat(fragment_shader_texture0, "ctex0_d.a = vFrontColor.a; \n");
            break;
         case GR_CMBX_ITRGB:
            strcat(fragment_shader_texture0, "ctex0_d.a = vFrontColor.a; \n");
            break;
         case GR_CMBX_LOCAL_TEXTURE_ALPHA:
            strcat(fragment_shader_texture0, "ctex0_d.a = readtex0.a; \n");
            break;
         default:
            strcat(fragment_shader_texture0, "ctex0_d.a = 0.0; \n");
      }

      if(c_invert)
         strcat(fragment_shader_texture0, "ctex0_c.a = 1.0 - ctex0_c.a; \n");

      if(d_invert)
         strcat(fragment_shader_texture0, "ctex0_d.a = 1.0 - ctex0_d.a; \n");

      strcat(fragment_shader_texture0, "ctexture0.a = (ctex0_a.a + ctex0_b.a) * ctex0_c.a + ctex0_d.a; \n");

      glUniform4f(current_shader->ccolor0_location, ccolor[0][0], ccolor[0][1], ccolor[0][2], ccolor[0][3]);
   }
   else
   {
      texture1_combinera_key = 0x80000000 | (a & 0x1F) | ((a_mode & 3) << 5) |
         ((b & 0x1F) << 7) | ((b_mode & 3) << 12) |
         ((c & 0x1F) << 14) | ((c_invert & 1) << 19) |
         ((d & 0x1F) << 20) | ((d_invert & 1) << 25);

      switch(a)
      {
         case GR_CMBX_ITALPHA:
            strcat(fragment_shader_texture1, "ctex1s_a.a = vFrontColor.a; \n");
            break;
         case GR_CMBX_LOCAL_TEXTURE_ALPHA:
            strcat(fragment_shader_texture1, "ctex1s_a.a = readtex1.a; \n");
            break;
         case GR_CMBX_OTHER_TEXTURE_ALPHA:
            strcat(fragment_shader_texture1, "ctex1s_a.a = ctexture0.a; \n");
            break;
         case GR_CMBX_TMU_CALPHA:
            strcat(fragment_shader_texture1, "ctex1s_a.a = ccolor1.a; \n");
            break;
         default:
            strcat(fragment_shader_texture1, "ctex1s_a.a = 0.0; \n");
      }

      switch(a_mode)
      {
         case GR_FUNC_MODE_ZERO:
            strcat(fragment_shader_texture1, "ctex1_a.a = 0.0; \n");
            break;
         case GR_FUNC_MODE_X:
            strcat(fragment_shader_texture1, "ctex1_a.a = ctex1s_a.a; \n");
            break;
         case GR_FUNC_MODE_ONE_MINUS_X:
            strcat(fragment_shader_texture1, "ctex1_a.a = 1.0 - ctex1s_a.a; \n");
            break;
         case GR_FUNC_MODE_NEGATIVE_X:
            strcat(fragment_shader_texture1, "ctex1_a.a = -ctex1s_a.a; \n");
            break;
         default:
            strcat(fragment_shader_texture1, "ctex1_a.a = 0.0; \n");
      }

      switch(b)
      {
         case GR_CMBX_ITALPHA:
            strcat(fragment_shader_texture1, "ctex1s_b.a = vFrontColor.a; \n");
            break;
         case GR_CMBX_LOCAL_TEXTURE_ALPHA:
            strcat(fragment_shader_texture1, "ctex1s_b.a = readtex1.a; \n");
            break;
         case GR_CMBX_OTHER_TEXTURE_ALPHA:
            strcat(fragment_shader_texture1, "ctex1s_b.a = ctexture0.a; \n");
            break;
         case GR_CMBX_TMU_CALPHA:
            strcat(fragment_shader_texture1, "ctex1s_b.a = ccolor1.a; \n");
            break;
         default:
            strcat(fragment_shader_texture1, "ctex1s_b.a = 0.0; \n");
      }

      switch(b_mode)
      {
         case GR_FUNC_MODE_ZERO:
            strcat(fragment_shader_texture1, "ctex1_b.a = 0.0; \n");
            break;
         case GR_FUNC_MODE_X:
            strcat(fragment_shader_texture1, "ctex1_b.a = ctex1s_b.a; \n");
            break;
         case GR_FUNC_MODE_ONE_MINUS_X:
            strcat(fragment_shader_texture1, "ctex1_b.a = 1.0 - ctex1s_b.a; \n");
            break;
         case GR_FUNC_MODE_NEGATIVE_X:
            strcat(fragment_shader_texture1, "ctex1_b.a = -ctex1s_b.a; \n");
            break;
         default:
            strcat(fragment_shader_texture1, "ctex1_b.a = 0.0; \n");
      }

      switch(c)
      {
         case GR_CMBX_ZERO:
            strcat(fragment_shader_texture1, "ctex1_c.a = 0.0; \n");
            break;
         case GR_CMBX_B:
            strcat(fragment_shader_texture1, "ctex1_c.a = ctex1s_b.a; \n");
            break;
         case GR_CMBX_DETAIL_FACTOR:
            strcat(fragment_shader_texture1, "ctex1_c.a = lambda; \n");
            break;
         case GR_CMBX_ITALPHA:
            strcat(fragment_shader_texture1, "ctex1_c.a = vFrontColor.a; \n");
            break;
         case GR_CMBX_LOCAL_TEXTURE_ALPHA:
            strcat(fragment_shader_texture1, "ctex1_c.a = readtex1.a; \n");
            break;
         case GR_CMBX_OTHER_TEXTURE_ALPHA:
            strcat(fragment_shader_texture1, "ctex1_c.a = ctexture0.a; \n");
            break;
         case GR_CMBX_TMU_CALPHA:
            strcat(fragment_shader_texture1, "ctex1_c.a = ccolor1.a; \n");
            break;
         default:
            strcat(fragment_shader_texture1, "ctex1_c.a = 0.0; \n");
      }

      switch(d)
      {
         case GR_CMBX_ZERO:
            strcat(fragment_shader_texture1, "ctex1_d.a = 0.0; \n");
            break;
         case GR_CMBX_B:
            strcat(fragment_shader_texture1, "ctex1_d.a = ctex1s_b.a; \n");
            break;
         case GR_CMBX_ITALPHA:
            strcat(fragment_shader_texture1, "ctex1_d.a = vFrontColor.a; \n");
            break;
         case GR_CMBX_ITRGB:
            strcat(fragment_shader_texture1, "ctex1_d.a = vFrontColor.a; \n");
            break;
         case GR_CMBX_LOCAL_TEXTURE_ALPHA:
            strcat(fragment_shader_texture1, "ctex1_d.a = readtex1.a; \n");
            break;
         default:
            strcat(fragment_shader_texture1, "ctex1_d.a = 0.0; \n");
      }

      if(c_invert)
         strcat(fragment_shader_texture1, "ctex1_c.a = 1.0 - ctex1_c.a; \n");

      if(d_invert)
         strcat(fragment_shader_texture1, "ctex1_d.a = 1.0 - ctex1_d.a; \n");

      strcat(fragment_shader_texture1, "ctexture1.a = (ctex1_a.a + ctex1_b.a) * ctex1_c.a + ctex1_d.a; \n");

      glUniform4f(current_shader->ccolor1_location, ccolor[1][0], ccolor[1][1], ccolor[1][2], ccolor[1][3]);
   }

   need_to_compile = 1;
}

void grAlphaBlendFunction(GLenum rgb_sf, GLenum rgb_df, GLenum alpha_sf, GLenum alpha_df)
{
   glEnable(GL_BLEND);
   glBlendFuncSeparate(rgb_sf, rgb_df, alpha_sf, alpha_df);
}
