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


#ifndef __RICE_RDP_GFX_H__
#define __RICE_RDP_GFX_H__

#include "../../Graphics/GBI.h"
#include "../../Graphics/RSP/gSP_state.h"

#define RSP_SPNOOP              0   // handle 0 gracefully 
#define RSP_MTX                 1
#define RSP_RESERVED0           2   // unknown 
#define RSP_MOVEMEM             3   // move a block of memory (up to 4 words) to dmem 
#define RSP_VTX                 4
#define RSP_RESERVED1           5   // unknown 
#define RSP_DL                  6
#define RSP_RESERVED2           7   // unknown 
#define RSP_RESERVED3           8   // unknown 
#define RSP_SPRITE2D            9   // sprite command 
#define RSP_SPRITE2D_BASE       9   // sprite command


#define RSP_1ST                 0xBF
#define RSP_TRI1                (RSP_1ST-0)
#define RSP_CULLDL              (RSP_1ST-1)
#define RSP_POPMTX              (RSP_1ST-2)
#define RSP_MOVEWORD            (RSP_1ST-3)
#define RSP_TEXTURE             (RSP_1ST-4)
#define RSP_SETOTHERMODE_H      (RSP_1ST-5)
#define RSP_SETOTHERMODE_L      (RSP_1ST-6)
#define RSP_ENDDL               (RSP_1ST-7)
#define RSP_SETGEOMETRYMODE     (RSP_1ST-8)
#define RSP_CLEARGEOMETRYMODE   (RSP_1ST-9)
#define RSP_LINE3D              (RSP_1ST-10)
#define RSP_RDPHALF_1           (RSP_1ST-11)
#define RSP_RDPHALF_2           (RSP_1ST-12)
#define RSP_RDPHALF_CONT        (RSP_1ST-13)

#define RSP_MODIFYVTX           (RSP_1ST-13)
#define RSP_TRI2                (RSP_1ST-14)
#define RSP_BRANCH_Z            (RSP_1ST-15)
#define RSP_LOAD_UCODE          (RSP_1ST-16)

#define RSP_SPRITE2D_SCALEFLIP    (RSP_1ST-1)
#define RSP_SPRITE2D_DRAW         (RSP_1ST-2)

#define RSP_ZELDAVTX                1
#define RSP_ZELDAMODIFYVTX          2
#define RSP_ZELDACULLDL             3
#define RSP_ZELDABRANCHZ            4
#define RSP_ZELDATRI1               5
#define RSP_ZELDATRI2               6
#define RSP_ZELDALINE3D             7

// 4 is something like a conditional DL
#define RSP_DMATRI  0x05
#define G_DLINMEM   0x07

// RSP_SETOTHERMODE_H gPipelineMode 
#define RSP_PIPELINE_MODE_1PRIMITIVE        (1 << RSP_SETOTHERMODE_SHIFT_PIPELINE)
#define RSP_PIPELINE_MODE_NPRIMITIVE        (0 << RSP_SETOTHERMODE_SHIFT_PIPELINE)

// RSP_SETOTHERMODE_H gSetTextureLUT 
#define TLUT_FMT_NONE           (0 << G_MDSFT_TEXTLUT)
#define TLUT_FMT_UNKNOWN        (1 << G_MDSFT_TEXTLUT)
#define TLUT_FMT_RGBA16         (2 << G_MDSFT_TEXTLUT)
#define TLUT_FMT_IA16           (3 << G_MDSFT_TEXTLUT)

// RSP_SETOTHERMODE_H gSetTextureFilter 
#define RDP_TFILTER_POINT       (0 << G_MDSFT_TEXTFILT)
#define RDP_TFILTER_AVERAGE     (3 << G_MDSFT_TEXTFILT)
#define RDP_TFILTER_BILERP      (2 << G_MDSFT_TEXTFILT)

// RSP_SETOTHERMODE_L gSetAlphaCompare 
#define RDP_ALPHA_COMPARE_NONE          (0 << RSP_SETOTHERMODE_SHIFT_ALPHACOMPARE)
#define RDP_ALPHA_COMPARE_THRESHOLD     (1 << RSP_SETOTHERMODE_SHIFT_ALPHACOMPARE)
#define RDP_ALPHA_COMPARE_DITHER        (3 << RSP_SETOTHERMODE_SHIFT_ALPHACOMPARE)

// RSP_SETOTHERMODE_L gSetRenderMode 
#define Z_COMPARE           0x0010
#define Z_UPDATE            0x0020

// Texturing macros

#define RDP_TXT_LOADTILE    7
#define RDP_TXT_RENDERTILE  0

#define RDP_TXT_NOMIRROR    0
#define RDP_TXT_WRAP        0
#define RDP_TXT_MIRROR      0x1
#define RDP_TXT_CLAMP       0x2
#define RDP_TXT_NOMASK      0
#define RDP_TXT_NOLOD       0

// MOVEMEM indices
//
// Each of these indexes an entry in a dmem table
// which points to a 1-4 word block of dmem in
// which to store a 1-4 word DMA.

# define RSP_GBI2_MV_MEM__VIEWPORT  8
# define RSP_GBI2_MV_MEM__LIGHT     10
# define RSP_GBI2_MV_MEM__POINT     12
# define RSP_GBI2_MV_MEM__MATRIX    14      // NOTE: this is in moveword table

typedef struct 
{
    uint32_t  type;
    uint32_t  flags;

    uint32_t  ucode_boot;
    uint32_t  ucode_boot_size;

    uint32_t  ucode;
    uint32_t  ucode_size;

    uint32_t  ucode_data;
    uint32_t  ucode_data_size;

    uint32_t  dram_stack;
    uint32_t  dram_stack_size;

    uint32_t  output_buff;
    uint32_t  output_buff_size;

    uint32_t  data_ptr;
    uint32_t  data_size;

    uint32_t  yield_data_ptr;
    uint32_t  yield_data_size;
} OSTask_t;

typedef union {
    OSTask_t        t;
    uint64_t  force_structure_alignment;
} OSTask;

#define MAX_DL_COUNT        1000000

typedef struct {
    bool    used;
    uint32_t  crc_size;
    uint32_t  crc_800;
    uint32_t  ucode;
    uint32_t  minor_ver;
    uint32_t  variant;
    char    rspstr[200];

    uint32_t  ucStart;
    uint32_t  ucSize;
    uint32_t  ucDStart;
    uint32_t  ucDSize;
    uint32_t  ucCRC;
    uint32_t  ucDWORD1;
    uint32_t  ucDWORD2;
    uint32_t  ucDWORD3;
    uint32_t  ucDWORD4;
} UcodeInfo;


typedef struct
{
    uint32_t      ucode;
    uint32_t      crc_size;
    uint32_t      crc_800;
    const unsigned char * ucode_name;
    bool        non_nearclip;
    bool        reject;
} UcodeData;

struct TileDescriptor
{
    // Set by SetTile
    unsigned int dwFormat   :3;     // e.g. RGBA, YUV etc
    unsigned int dwSize     :2;     // e.g 4/8/16/32bpp
    unsigned int dwLine     :9;     // Ummm...
    unsigned int dwPalette  :4;     // 0..15 - a palette index?
    uint32_t dwTMem;                  // Texture memory location

    unsigned int bClampS    :1;
    unsigned int bClampT    :1;
    unsigned int bMirrorS   :1;
    unsigned int bMirrorT   :1;

    unsigned int dwMaskS    :4;
    unsigned int dwMaskT    :4;
    unsigned int dwShiftS   :4;
    unsigned int dwShiftT   :4;

    // Set by SetTileSize
    unsigned int sl     :10;    // Upper left S     - 8:3
    unsigned int tl     :10;    // Upper Left T     - 8:3
    unsigned int sh     :10;    // Lower Right S
    unsigned int th     :10;    // Lower Right T
};

enum LoadType
{
    BY_NEVER_SET,
    BY_LOAD_BLOCK,
    BY_LOAD_TILE,
    BY_LOAD_TLUT,
};

struct LoadCmdInfo
{
    LoadType    loadtype;
    unsigned int sl     :10;    // Upper left S     - 8:3
    unsigned int tl     :10;    // Upper Left T     - 8:3
    unsigned int sh     :10;    // Lower Right S
    unsigned int th     :10;    // Lower Right T
    unsigned int dxt    :12;
};

typedef struct {    // This is in Intel format
    uint32_t SourceImagePointer;
    uint32_t TlutPointer;

    short SubImageWidth;
    short Stride;

    char  SourceImageBitSize;
    char  SourceImageType;
    short SubImageHeight;

    short SourceImageOffsetT;
    short SourceImageOffsetS;

    char  dummy[4]; 
} SpriteStruct;         //Converted Sprint struct in Intel format

typedef struct{
    short px;
    short py;
    float scaleX;
    float scaleY;
    uint8_t  flipX; 
    uint8_t  flipY;
    SpriteStruct *spritePtr;
} Sprite2DInfo;


typedef struct
{
    unsigned int    c2_m2b:2;
    unsigned int    c1_m2b:2;
    unsigned int    c2_m2a:2;
    unsigned int    c1_m2a:2;
    unsigned int    c2_m1b:2;
    unsigned int    c1_m1b:2;
    unsigned int    c2_m1a:2;
    unsigned int    c1_m1a:2;
} RDP_BlenderSetting;

typedef struct
{
    union
    {
        struct
        {
            // Low bits
            unsigned int        alpha_compare : 2;          // 0..1
            unsigned int        depth_source : 1;           // 2..2

        //  unsigned int        render_mode : 13;           // 3..15
            unsigned int        aa_en : 1;                  // 3
            unsigned int        z_cmp : 1;                  // 4
            unsigned int        z_upd : 1;                  // 5
            unsigned int        im_rd : 1;                  // 6
            unsigned int        clr_on_cvg : 1;             // 7

            unsigned int        cvg_dst : 2;                // 8..9
            unsigned int        zmode : 2;                  // 10..11

            unsigned int        cvg_x_alpha : 1;            // 12
            unsigned int        alpha_cvg_sel : 1;          // 13
            unsigned int        force_bl : 1;               // 14
            unsigned int        tex_edge : 1;               // 15 - Not used

            unsigned int        blender : 16;               // 16..31

            // High bits
            unsigned int        blend_mask : 4;             // 0..3 - not supported
            unsigned int        alpha_dither : 2;           // 4..5
            unsigned int        rgb_dither : 2;             // 6..7
            
            unsigned int        key_en : 1;                 // 8..8
            unsigned int        text_conv : 3;              // 9..11
            unsigned int        text_filt : 2;              // 12..13
            unsigned int        text_tlut : 2;              // 14..15

            unsigned int        text_lod : 1;               // 16..16
            unsigned int        text_sharpen : 1;           // 17..18
            unsigned int        text_detail : 1;            // 17..18
            unsigned int        text_persp : 1;             // 19..19
            unsigned int        cycle_type : 2;             // 20..21
            unsigned int        reserved : 1;               // 22..22 - not supported
            unsigned int        atomic_prim : 1;            // 23..23

            unsigned int        pad : 8;                    // 24..31 - padding

        };
        uint64_t          _u64;
        uint32_t          _u32[2];
    };
} RDP_OtherMode;


typedef enum 
{ 
    CMD_SETTILE, 
    CMD_SETTILE_SIZE, 
    CMD_LOADBLOCK, 
    CMD_LOADTILE, 
    CMD_LOADTLUT, 
    CMD_SET_TEXTURE,
    CMD_LOAD_OBJ_TXTR,
} SetTileCmdType;


// The display list PC stack. Before this was an array of 10
// items, but this way we can nest as deeply as necessary. 

typedef struct 
{
    uint32_t pc;
    int countdown;
} DListStack;

typedef struct
{
    int x0, y0, x1, y1, mode;
    int left, top, right, bottom;
} ScissorType;

// Mask down to 0x003FFFFF?
#define RSPSegmentAddr(seg) ( gSP.segment[((seg)>>24)&0x0F] + ((seg)&0x00FFFFFF) )

extern uint16_t g_wRDPTlut[];
extern const char *textluttype[4];

extern const char *pszImgFormat[8];
extern const char *pszImgSize[4];
extern uint8_t pnImgSize[4];
extern const char *textlutname[4];

extern SetImgInfo g_CI;
extern SetImgInfo g_ZI;
extern SetImgInfo g_TI;
extern TmemType g_Tmem;

void DLParser_Init();
void RDP_GFX_Reset();
void RDP_Cleanup();
void DLParser_Process(OSTask * pTask);
void RDP_DLParser_Process(void);

void PrepareTextures();
void RDP_InitRenderState();
void DisplayVertexInfo(uint32_t dwAddr, uint32_t dwV0, uint32_t dwN);

void ricegSPLight(uint32_t dwAddr, uint32_t dwLight);
void ricegSPViewport(uint32_t v);

void RDP_NOIMPL_WARN(const char* op);
void RSP_GFX_Force_Matrix(uint32_t dwAddr);
void RSP_GFX_InitGeometryMode();
void RSP_SetUcode(int ucode, uint32_t ucStart, uint32_t ucDStart, uint32_t cdSize);
uint32_t CalcalateCRC(uint32_t* srcPtr, uint32_t srcSize);
void RDP_GFX_PopDL();

extern Matrix matToLoad;
void LoadMatrix(uint32_t addr);

unsigned int ComputeCRC32(unsigned int crc, const uint8_t *buf, unsigned int len);

void TriggerDPInterrupt();
void TriggerSPInterrupt();
uint32_t DLParser_CheckUcode(uint32_t ucStart, uint32_t ucDStart, uint32_t ucSize, uint32_t ucDSize);

bool IsUsedAsDI(uint32_t addr);

#if defined(DEBUGGER)
  void __cdecl LOG_UCODE(const char* szFormat, ...);
#else
  inline void LOG_UCODE(...) {}
#endif

#endif  // __RICE_RDP_GFX_H__

