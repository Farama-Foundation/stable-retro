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

#ifndef _TYPEDEFS_H_
#define _TYPEDEFS_H_

#include <stdint.h>

#include "osal_preproc.h"
#include "VectorMath.h"

typedef unsigned int COLOR;
typedef struct _COORDRECT
{
   int x1,y1;
   int x2,y2;
} COORDRECT;
#define COLOR_RGBA(r,g,b,a) (((r&0xFF)<<16) | ((g&0xFF)<<8) | ((b&0xFF)<<0) | ((a&0xFF)<<24))
#define SURFFMT_A8R8G8B8 21

#define RGBA_GETALPHA(rgb)      ((rgb) >> 24)
#define RGBA_GETRED(rgb)        (((rgb) >> 16) & 0xff)
#define RGBA_GETGREEN(rgb)      (((rgb) >> 8) & 0xff)
#define RGBA_GETBLUE(rgb)       ((rgb) & 0xff)

typedef XMATRIX Matrix;
typedef void* LPRICETEXTURE ;

typedef struct 
{
    uint32_t dwRGBA, dwRGBACopy;
    char x,y,z;         // Direction
    uint8_t pad;
} N64Light;


typedef struct
{
    unsigned int    dwFormat:3;
    unsigned int    dwSize:2;
    unsigned int    dwWidth:10;
    uint32_t          dwAddr;
    uint32_t          bpl;
} SetImgInfo;

typedef struct 
{
    int   hilite_sl;
    int   hilite_tl;
    int   hilite_sh;
    int   hilite_th;

    float   fhilite_sl;
    float   fhilite_tl;
    float   fhilite_sh;
    float   fhilite_th;

    uint32_t dwDXT;

    uint32_t dwPitch;

    float fShiftScaleS;
    float fShiftScaleT;

    uint32_t   lastTileCmd;
    bool  bSizeIsValid;

    bool bForceWrapS;
    bool bForceWrapT;
    bool bForceClampS;
    bool bForceClampT;

} TileAdditionalInfo;


typedef struct
{
    float u;
    float v;
} TexCord;

typedef struct VECTOR2
{
    float x;
    float y;
    VECTOR2( float newx, float newy )   {x=newx; y=newy;}
    VECTOR2()   {x=0; y=0;}
} VECTOR2;

typedef struct
{
    short x;
    short y;
} IVector2;

typedef struct 
{
    short x;
    short y;
    short z;
} IVector3;

typedef struct {
    float x,y,z;
    float rhw;
    union {
        COLOR  dcDiffuse;
        struct {
            uint8_t b;
            uint8_t g;
            uint8_t r;
            uint8_t a;
        };
    };
    COLOR  dcSpecular;
    TexCord tcord[2];
} TLITVERTEX, *LPTLITVERTEX;

typedef struct {
    float x,y,z;
    union {
        COLOR  dcDiffuse;
        struct {
            uint8_t b;
            uint8_t g;
            uint8_t r;
            uint8_t a;
        };
    };
    COLOR  dcSpecular;
    TexCord tcord[2];
} UTLITVERTEX, *LPUTLITVERTEX;

typedef struct {
    float x,y,z;
    float rhw;
    union {
        COLOR  dcDiffuse;
        struct {
            uint8_t b;
            uint8_t g;
            uint8_t r;
            uint8_t a;
        };
    };
    COLOR  dcSpecular;
} LITVERTEX, *LPLITVERTEX;



typedef struct {
    float   x,y,z;
    float   rhw;
    COLOR dcDiffuse;
} FILLRECTVERTEX, *LPFILLRECTVERTEX;

#include "COLOR.h"
#include "IColor.h"

typedef struct
{
    float x,y,z;
    float nx,ny,nz;
    union {
        COLOR  dcDiffuse;
        struct {
            uint8_t b;
            uint8_t g;
            uint8_t r;
            uint8_t a;
        };
    };
    float u,v;
}EXTERNAL_VERTEX, *LPSHADERVERTEX;


typedef struct
{
    union {
        struct {
            float x;
            float y;
            float z;
            float range;        // Range == 0  for directional light
                                // Range != 0  for point light, Zelda MM
        };
    };

    union {
        struct {
            uint8_t r;
            uint8_t g;
            uint8_t b;
            uint8_t a;
        };
        uint32_t col;
    };

    union {
        struct {
            float fr;
            float fg;
            float fb;
            float fa;
        };
        float fcolors[4];
    };

    union {
        struct {
            float tx;
            float ty;
            float tz;
            float tdummy;
        };
    };

    union {
        struct {
            float ox;
            float oy;
            float oz;
            float odummy;
        };
    };
} Light;

typedef struct
{
    char na;
    char nz;    // b
    char ny;    //g
    char nx;    //r
}NormalStruct;

typedef struct
{
    short y;
    short x;
    
    short flag;
    short z;
    
    short tv;
    short tu;
    
    union {
        struct {
            uint8_t a;
            uint8_t b;
            uint8_t g;
            uint8_t r;
        } rgba;
        NormalStruct norma;
    };
} FiddledVtx;

typedef struct
{
    short y;
    short x;
    
    uint8_t a;
    uint8_t b;
    short z;
    
    uint8_t g;
    uint8_t r;
    
} FiddledVtxDKR;

typedef struct 
{
    short y;
    short   x;
    uint16_t  cidx;
    short z;
    short t;
    short s;
} N64VtxPD;

class CTexture;
class COGLTexture;
class CDirectXTexture;
struct TxtrCacheEntry;

typedef struct {
    LPRICETEXTURE m_lpsTexturePtr;
    union {
        CTexture *          m_pCTexture;
        CDirectXTexture *   m_pCDirectXTexture;
        COGLTexture *       m_pCOGLTexture;
    };
    
    uint32_t m_dwTileWidth;
    uint32_t m_dwTileHeight;
    float m_fTexWidth;
    float m_fTexHeight;     // Float to avoid converts when processing verts
    TxtrCacheEntry *pTextureEntry;
} RenderTexture;


typedef struct
{
    unsigned int    dwFormat;
    unsigned int    dwSize;
    unsigned int    dwWidth;
    unsigned int    dwAddr;

    unsigned int    dwLastWidth;
    unsigned int    dwLastHeight;

    unsigned int    dwHeight;
    unsigned int    dwMemSize;

    bool                bCopied;
    unsigned int    dwCopiedAtFrame;

    unsigned int    dwCRC;
    unsigned int    lastUsedFrame;
    unsigned int    bUsedByVIAtFrame;
    unsigned int    lastSetAtUcode;
} RecentCIInfo;

typedef struct
{
    uint32_t      addr;
    uint32_t      FrameCount;
} RecentViOriginInfo;

typedef enum {
    SHADE_DISABLED,
    SHADE_FLAT,
    SHADE_SMOOTH,
} RenderShadeMode;

typedef enum {
    TEXTURE_UV_FLAG_WRAP,
    TEXTURE_UV_FLAG_MIRROR,
    TEXTURE_UV_FLAG_CLAMP,
} TextureUVFlag;

typedef struct
{
    TextureUVFlag   N64flag;
    uint32_t          realFlag;
} UVFlagMap;


typedef enum {
    FILTER_POINT,
    FILTER_LINEAR,
} TextureFilter;

typedef struct 
{
    TextureFilter   N64filter;
    uint32_t                  realFilter;
} TextureFilterMap;

typedef struct {
    const char* description;
    int number;
    uint32_t  setting;
} BufferSettingInfo;

typedef struct {
    const char* description;
    uint32_t setting;
} SettingInfo;

typedef union {
    uint8_t   g_Tmem8bit[0x1000];
    short   g_Tmem16bit[0x800];
    uint32_t  g_Tmem32bit[0x300];
    uint64_t  g_Tmem64bit[0x200];
} TmemType;


typedef struct {
    uint32_t dwFormat;
    uint32_t dwSize;
    uint32_t  bSetBy;

    uint32_t dwLoadAddress;
    uint32_t dwTotalWords;
    uint32_t dxt;
    bool  bSwapped;

    uint32_t dwWidth;
    uint32_t dwLine;

    int sl;
    int sh;
    int tl;
    int th;

    uint32_t dwTmem;
} TMEMLoadMapInfo;

#endif

