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

#ifndef _UCODE_DEFS_H_
#define _UCODE_DEFS_H_

typedef struct {
    union {
        uint32_t w0;
        struct {
            uint32_t arg0:24;
            uint32_t cmd:8;
        };
    };
    uint32_t w1;
} Gwords;

typedef struct {
    uint32_t w0;
    uint32_t v2:8;
    uint32_t v1:8;
    uint32_t v0:8;
    uint32_t flag:8;
} GGBI0_Tri1;

typedef struct {
    uint32_t v0:8;
    uint32_t v1:8;
    uint32_t v2:8;
    uint32_t cmd:8;
    uint32_t pad:24;
    uint32_t flag:8;
} GGBI2_Tri1;

typedef struct {
    uint32_t :1;
    uint32_t v3:7;
    uint32_t :1;
    uint32_t v4:7;
    uint32_t :1;
    uint32_t v5:7;
    uint32_t cmd:8;
    uint32_t :1;
    uint32_t v0:7;
    uint32_t :1;
    uint32_t v1:7;
    uint32_t :1;
    uint32_t v2:7;
    uint32_t flag:8;
} GGBI2_Tri2;

typedef struct {
    uint32_t w0;
    uint32_t v2:8;
    uint32_t v1:8;
    uint32_t v0:8;
    uint32_t v3:8;
} GGBI0_Ln3DTri2;

typedef struct {
    uint32_t v5:8;
    uint32_t v4:8;
    uint32_t v3:8;
    uint32_t cmd:8;

    uint32_t v2:8;
    uint32_t v1:8;
    uint32_t v0:8;
    uint32_t flag:8;
} GGBI1_Tri2;

typedef struct {
    uint32_t v3:8;
    uint32_t v4:8;
    uint32_t v5:8;
    uint32_t cmd:8;

    uint32_t v0:8;
    uint32_t v1:8;
    uint32_t v2:8;
    uint32_t flag:8;
} GGBI2_Line3D;

typedef struct {
    uint32_t len:16;
    uint32_t v0:4;
    uint32_t n:4;
    uint32_t cmd:8;
    uint32_t addr;
} GGBI0_Vtx;

typedef struct {
    uint32_t len:10;
    uint32_t n:6;
    uint32_t :1;
    uint32_t v0:7;
    uint32_t cmd:8;
    uint32_t addr;
} GGBI1_Vtx;

typedef struct {
    uint32_t vend:8;
    uint32_t :4;
    uint32_t n:8;
    uint32_t :4;
    uint32_t cmd:8;
    uint32_t addr;
} GGBI2_Vtx;

typedef struct {
    uint32_t    width:12;
    uint32_t    :7;
    uint32_t    siz:2;
    uint32_t    fmt:3;
    uint32_t    cmd:8;
    uint32_t    addr;
} GSetImg;

typedef struct {
    uint32_t    prim_level:8;
    uint32_t    prim_min_level:8;
    uint32_t    pad:8;
    uint32_t    cmd:8;

    union {
        uint32_t    color;
        struct {
            uint32_t fillcolor:16;
            uint32_t fillcolor2:16;
        };
        struct {
            uint32_t a:8;
            uint32_t b:8;
            uint32_t g:8;
            uint32_t r:8;
        };
    };
} GSetColor;

typedef struct {
    uint32_t    :16;
    uint32_t    param:8;
    uint32_t    cmd:8;
    uint32_t    addr;
} GGBI0_Dlist;

typedef struct {
    uint32_t    len:16;
    uint32_t    projection:1;
    uint32_t    load:1;
    uint32_t    push:1;
    uint32_t    :5;
    uint32_t    cmd:8;
    uint32_t    addr;
} GGBI0_Matrix;

typedef struct {
    uint32_t    :24;
    uint32_t    cmd:8;
    uint32_t    projection:1;
    uint32_t    :31;
} GGBI0_PopMatrix;

typedef struct {
    union {
        struct {
            uint32_t    param:8;
            uint32_t    len:16;
            uint32_t    cmd:8;
        };
        struct {
            uint32_t    nopush:1;
            uint32_t    load:1;
            uint32_t    projection:1;
            uint32_t    :5;
            uint32_t    len2:16;
            uint32_t    cmd2:8;
        };
    };
    uint32_t    addr;
} GGBI2_Matrix;

typedef struct {
    uint32_t    type:8;
    uint32_t    offset:16;
    uint32_t    cmd:8;
    uint32_t    value;
} GGBI0_MoveWord;

typedef struct {
    uint32_t    offset:16;
    uint32_t    type:8;
    uint32_t    cmd:8;
    uint32_t    value;
} GGBI2_MoveWord;

typedef struct {
    uint32_t    enable_gbi0:1;
    uint32_t    enable_gbi2:1;
    uint32_t    :6;
    uint32_t    tile:3;
    uint32_t    level:3;
    uint32_t    :10;
    uint32_t    cmd:8;
    uint32_t    scaleT:16;
    uint32_t    scaleS:16;
} GTexture;

typedef struct {
    uint32_t    tl:12;
    uint32_t    sl:12;
    uint32_t    cmd:8;

    uint32_t    th:12;
    uint32_t    sh:12;
    uint32_t    tile:3;
    uint32_t    pad:5;
} Gloadtile;

typedef struct {
    uint32_t    tmem:9;
    uint32_t    line:9;
    uint32_t    pad0:1;
    uint32_t    siz:2;
    uint32_t    fmt:3;
    uint32_t    cmd:8;

    uint32_t    shifts:4;
    uint32_t    masks:4;
    uint32_t    ms:1;
    uint32_t    cs:1;
    uint32_t    shiftt:4;
    uint32_t    maskt:4;
    uint32_t    mt:1;
    uint32_t    ct:1;
    uint32_t    palette:4;
    uint32_t    tile:3;
    uint32_t    pad1:5;
} Gsettile;

typedef union {
    Gwords          words;
    GGBI0_Tri1      tri1;
    GGBI0_Ln3DTri2  ln3dtri2;
    GGBI1_Tri2      gbi1tri2;
    GGBI2_Tri1      gbi2tri1;
    GGBI2_Tri2      gbi2tri2;
    GGBI2_Line3D    gbi2line3d;
    GGBI0_Vtx       gbi0vtx;
    GGBI1_Vtx       gbi1vtx;
    GGBI2_Vtx       gbi2vtx;
    GSetImg         setimg;
    GSetColor       setcolor;
    GGBI0_Dlist     gbi0dlist;
    GGBI0_Matrix    gbi0matrix;
    GGBI0_PopMatrix gbi0popmatrix;
    GGBI2_Matrix    gbi2matrix;
    GGBI0_MoveWord  gbi0moveword;
    GGBI2_MoveWord  gbi2moveword;
    GTexture        texture;
    Gloadtile       loadtile;
    Gsettile        settile;
    /*
    Gdma        dma;
    Gsegment    segment;
    GsetothermodeH  setothermodeH;
    GsetothermodeL  setothermodeL;
    Gtexture    texture;
    Gperspnorm  perspnorm;
    Gsetcombine setcombine;
    Gfillrect   fillrect;
    Gsettile    settile;
    Gloadtile   loadtile;
    Gsettilesize    settilesize;
    Gloadtlut   loadtlut;
    */
    int64_t   force_structure_alignment;
} Gfx;

typedef union {
    struct {
        uint32_t    w0;
        uint32_t    w1;
        uint32_t    w2;
        uint32_t    w3;
    };
    struct {
        uint32_t    yl:12;  /* Y coordinate of upper left   */
        uint32_t    xl:12;  /* X coordinate of upper left   */
        uint32_t    cmd:8;  /* command          */

        uint32_t    yh:12;  /* Y coordinate of lower right  */
        uint32_t    xh:12;  /* X coordinate of lower right  */
        uint32_t    tile:3; /* Tile descriptor index    */
        uint32_t    pad1:5; /* Padding          */

        uint32_t    t:16;   /* T texture coord at top left  */
        uint32_t    s:16;   /* S texture coord at top left  */

        uint32_t    dtdy:16;/* Change in T per change in Y  */
        uint32_t    dsdx:16;/* Change in S per change in X  */
    };
} Gtexrect;

#endif

