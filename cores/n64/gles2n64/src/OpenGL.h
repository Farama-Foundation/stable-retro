#ifndef OPENGL_H
#define OPENGL_H

#ifdef __cplusplus
extern "C" {
#endif

#include <boolean.h>
#include <glsm/glsmsym.h>
#include "gSP.h"

#include "../../Graphics/RSP/gSP_state.h"

#define RS_NONE         0
#define RS_TRIANGLE     1
#define RS_RECT         2
#define RS_TEXTUREDRECT 3
#define RS_LINE         4


#define SCREEN_UPDATE_AT_VI_UPDATE              1
#define SCREEN_UPDATE_AT_VI_CHANGE              2
#define SCREEN_UPDATE_AT_CI_CHANGE              3
#define SCREEN_UPDATE_AT_1ST_CI_CHANGE          4
#define SCREEN_UPDATE_BEFORE_SCREEN_CLEAR       6
#define SCREEN_UPDATE_AT_VI_UPDATE_AND_DRAWN    7


#define BLEND_NOOP              0x0000
#define BLEND_NOOP5             0xcc48  // Fog * 0 + Mem * 1
#define BLEND_NOOP4             0xcc08  // Fog * 0 + In * 1
#define BLEND_FOG_ASHADE        0xc800
#define BLEND_FOG_3             0xc000  // Fog * AIn + In * 1-A
#define BLEND_FOG_MEM           0xc440  // Fog * AFog + Mem * 1-A
#define BLEND_FOG_APRIM         0xc400  // Fog * AFog + In * 1-A
#define BLEND_BLENDCOLOR        0x8c88
#define BLEND_BI_AFOG           0x8400  // Bl * AFog + In * 1-A
#define BLEND_BI_AIN            0x8040  // Bl * AIn + Mem * 1-A
#define BLEND_MEM               0x4c40  // Mem*0 + Mem*(1-0)?!
#define BLEND_FOG_MEM_3         0x44c0  // Mem * AFog + Fog * 1-A
#define BLEND_NOOP3             0x0c48  // In * 0 + Mem * 1
#define BLEND_PASS              0x0c08  // In * 0 + In * 1
#define BLEND_FOG_MEM_IN_MEM    0x0440  // In * AFog + Mem * 1-A
#define BLEND_FOG_MEM_FOG_MEM   0x04c0  // In * AFog + Fog * 1-A
#define BLEND_OPA               0x0044  //  In * AIn + Mem * AMem
#define BLEND_XLU               0x0040
#define BLEND_MEM_ALPHA_IN      0x4044  //  Mem * AIn + Mem * AMem

#define INDEXMAP_SIZE 64
#define VERTBUFF_SIZE 256
#define ELEMBUFF_SIZE 1024

typedef struct
{
    float x, y, z, w;
    struct
    {
        float r, g, b, a;
    } color, secondaryColor;
    float s0, t0, s1, t1;
} GLVertex;

typedef struct triangles_t
{
   struct SPVertex    vertices[VERTBUFF_SIZE];
   GLubyte     elements[ELEMBUFF_SIZE];
   int         num;
} triangles_t;

typedef struct
{
    bool    screenUpdate;

    int     frame_dl;
    int     frame_prevdl;
    int     mustRenderDlist;
    int     renderingToTexture;

    uint32_t heightOffset;
    float   scaleX, scaleY;


    struct triangles_t triangles;

    unsigned int    renderState;

    GLVertex rect[4];
} GLInfo;

struct TexturedRectParams
{
   float ulx, uly, lrx, lry;
   float uls, ult, lrs, lrt;
   bool flip;
};

extern GLInfo OGL;

bool OGL_Start();
void OGL_Stop();

void OGL_AddTriangle(int v0, int v1, int v2);
void OGL_DrawTriangles(void);
void OGL_DrawDMATriangles(uint32_t _numVtx);
void OGL_DrawTriangle(struct SPVertex *vertices, int v0, int v1, int v2);
void OGL_DrawLLETriangle(uint32_t _numVtx);
void OGL_DrawLine(int v0, int v1, float width);
void OGL_DrawRect(int ulx, int uly, int lrx, int lry, float *color);
void OGL_DrawTexturedRect(const struct TexturedRectParams *_params);

void OGL_UpdateFrameTime();
void OGL_UpdateScale();

void OGL_ClearDepthBuffer(bool _fullsize);
void OGL_ClearColorBuffer(float *color);
void OGL_ResizeWindow(int x, int y, int width, int height);
void OGL_SwapBuffers();
void OGL_ReadScreen( void *dest, int *width, int *height );

int  OGL_CheckError();
int  OGL_IsExtSupported( const char *extension );

float OGL_GetScaleX(void);

float OGL_GetScaleY(void);

uint32_t OGL_GetHeightOffset(void);

uint32_t OGL_GetScreenWidth(void);

uint32_t OGL_GetScreenHeight(void);

#ifdef __cplusplus
}
#endif

#endif

