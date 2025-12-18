#ifndef GBI_H
#define GBI_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stdint.h>

// Microcode Types
#define F3D         0
#define F3DEX       1
#define F3DEX2      2
#define L3D         3
#define L3DEX       4
#define L3DEX2      5
#define S2DEX       6
#define S2DEX2      7
#define F3DPD       8
#define F3DDKR      9
#define F3DJFG		  10
#define F3DSWSE     11
#define F3DWRUS		12
#define F3DEX2CBFD	13
#define Turbo3D		14
#define ZSortp		15
#define NONE		16

#define F3DCBFD_MV_VIEWPORT     8
#define F3DCBFD_MV_LIGHT        10
#define F3DCBFD_MV_NORMAL       14

static const int numMicrocodeTypes = 11;

// Fixed point conversion factors
#define FIXED2FLOATRECIP1   0.5f
#define FIXED2FLOATRECIP2   0.25f
#define FIXED2FLOATRECIP3   0.125f
#define FIXED2FLOATRECIP4   0.0625f
#define FIXED2FLOATRECIP5   0.03125f
#define FIXED2FLOATRECIP6   0.015625f
#define FIXED2FLOATRECIP7   0.0078125f
#define FIXED2FLOATRECIP8   0.00390625f
#define FIXED2FLOATRECIP9   0.001953125f
#define FIXED2FLOATRECIP10  0.0009765625f
#define FIXED2FLOATRECIP11  0.00048828125f
#define FIXED2FLOATRECIP12  0.00024414063f
#define FIXED2FLOATRECIP13  0.00012207031f
#define FIXED2FLOATRECIP14  6.1035156e-05f
#define FIXED2FLOATRECIP15  3.0517578e-05f
#define FIXED2FLOATRECIP16  1.5258789e-05f

#define _FIXED2FLOAT( v, b ) ((float)v * FIXED2FLOATRECIP##b)

// Useful macros for decoding GBI command's parameters
#define _SHIFTL( v, s, w )  (((uint32_t)v & ((0x01 << w) - 1)) << s)
#define _SHIFTR( v, s, w )  (((uint32_t)v >> s) & ((0x01 << w) - 1))

// BG flags
#define G_BGLT_LOADBLOCK    0x0033
#define G_BGLT_LOADTILE     0xfff4

#define G_BG_FLAG_FLIPS     0x01
#define G_BG_FLAG_FLIPT     0x10

// Sprite object render modes
#define G_OBJRM_NOTXCLAMP       0x01
#define G_OBJRM_XLU             0x02    /* Ignored */
#define G_OBJRM_ANTIALIAS       0x04    /* Ignored */
#define G_OBJRM_BILERP          0x08
#define G_OBJRM_SHRINKSIZE_1    0x10
#define G_OBJRM_SHRINKSIZE_2    0x20
#define G_OBJRM_WIDEN           0x40

// Sprite texture loading types
#define G_OBJLT_TXTRBLOCK   0x00001033
#define G_OBJLT_TXTRTILE    0x00fc1034
#define G_OBJLT_TLUT        0x00000030


// These are all the constant flags
#define G_ZBUFFER               0x00000001
#define G_SHADE                 0x00000004
#define G_FOG                   0x00010000
#define G_LIGHTING              0x00020000
#define G_TEXTURE_GEN           0x00040000
#define G_TEXTURE_GEN_LINEAR    0x00080000
#define G_LOD                   0x00100000
#define G_POINT_LIGHTING		  0x00400000

#define G_MV_MMTX       2
#define G_MV_PMTX       6
#define G_MV_LIGHT      10
#define G_MV_POINT      12
#define G_MV_MATRIX     14
#define G_MV_NORMALES	14

#define G_MVO_LOOKATX   0
#define G_MVO_LOOKATY   24
#define G_MVO_L0        48
#define G_MVO_L1        72
#define G_MVO_L2        96
#define G_MVO_L3        120
#define G_MVO_L4        144
#define G_MVO_L5        168
#define G_MVO_L6        192
#define G_MVO_L7        216

#define G_MV_LOOKATY    0x82
#define G_MV_LOOKATX    0x84
#define G_MV_L0         0x86
#define G_MV_L1         0x88
#define G_MV_L2         0x8a
#define G_MV_L3         0x8c
#define G_MV_L4         0x8e
#define G_MV_L5         0x90
#define G_MV_L6         0x92
#define G_MV_L7         0x94
#define G_MV_TXTATT     0x96
#define G_MV_MATRIX_1   0x9E
#define G_MV_MATRIX_2   0x98
#define G_MV_MATRIX_3   0x9A
#define G_MV_MATRIX_4   0x9C

#define G_MW_MATRIX         0x00
#define G_MW_NUMLIGHT       0x02
#define G_MW_CLIP           0x04
#define G_MW_SEGMENT        0x06
#define G_MW_FOG            0x08
#define G_MW_LIGHTCOL       0x0A
#define G_MW_FORCEMTX       0x0C
#define G_MW_POINTS         0x0C
#define G_MW_PERSPNORM      0x0E
#define G_MV_COORDMOD       0x10    //Conker Bad Fur Day

#define G_MWO_NUMLIGHT      0x00
#define G_MWO_CLIP_RNX      0x04
#define G_MWO_CLIP_RNY      0x0c
#define G_MWO_CLIP_RPX      0x14
#define G_MWO_CLIP_RPY      0x1c
#define G_MWO_SEGMENT_0     0x00
#define G_MWO_SEGMENT_1     0x01
#define G_MWO_SEGMENT_2     0x02
#define G_MWO_SEGMENT_3     0x03
#define G_MWO_SEGMENT_4     0x04
#define G_MWO_SEGMENT_5     0x05
#define G_MWO_SEGMENT_6     0x06
#define G_MWO_SEGMENT_7     0x07
#define G_MWO_SEGMENT_8     0x08
#define G_MWO_SEGMENT_9     0x09
#define G_MWO_SEGMENT_A     0x0a
#define G_MWO_SEGMENT_B     0x0b
#define G_MWO_SEGMENT_C     0x0c
#define G_MWO_SEGMENT_D     0x0d
#define G_MWO_SEGMENT_E     0x0e
#define G_MWO_SEGMENT_F     0x0f
#define G_MWO_FOG           0x00

#define G_MWO_MATRIX_XX_XY_I    0x00
#define G_MWO_MATRIX_XZ_XW_I    0x04
#define G_MWO_MATRIX_YX_YY_I    0x08
#define G_MWO_MATRIX_YZ_YW_I    0x0C
#define G_MWO_MATRIX_ZX_ZY_I    0x10
#define G_MWO_MATRIX_ZZ_ZW_I    0x14
#define G_MWO_MATRIX_WX_WY_I    0x18
#define G_MWO_MATRIX_WZ_WW_I    0x1C
#define G_MWO_MATRIX_XX_XY_F    0x20
#define G_MWO_MATRIX_XZ_XW_F    0x24
#define G_MWO_MATRIX_YX_YY_F    0x28
#define G_MWO_MATRIX_YZ_YW_F    0x2C
#define G_MWO_MATRIX_ZX_ZY_F    0x30
#define G_MWO_MATRIX_ZZ_ZW_F    0x34
#define G_MWO_MATRIX_WX_WY_F    0x38
#define G_MWO_MATRIX_WZ_WW_F    0x3C
#define G_MWO_POINT_RGBA        0x10
#define G_MWO_POINT_ST          0x14
#define G_MWO_POINT_XYSCREEN    0x18
#define G_MWO_POINT_ZSCREEN     0x1C

#ifdef DEBUG
static const char *MWOPointText[] =
{
    "G_MWO_POINT_RGBA",
    "G_MWO_POINT_ST",
    "G_MWO_POINT_XYSCREEN",
    "G_MWO_POINT_ZSCREEN"
};

static const char *MWOMatrixText[] =
{
    "G_MWO_MATRIX_XX_XY_I", "G_MWO_MATRIX_XZ_XW_I", "G_MWO_MATRIX_YX_YY_I", "G_MWO_MATRIX_YZ_YW_I",
    "G_MWO_MATRIX_ZX_ZY_I", "G_MWO_MATRIX_ZZ_ZW_I", "G_MWO_MATRIX_WX_WY_I", "G_MWO_MATRIX_WZ_WW_I",
    "G_MWO_MATRIX_XX_XY_F", "G_MWO_MATRIX_XZ_XW_F", "G_MWO_MATRIX_YX_YY_F", "G_MWO_MATRIX_YZ_YW_F",
    "G_MWO_MATRIX_ZX_ZY_F", "G_MWO_MATRIX_ZZ_ZW_F", "G_MWO_MATRIX_WX_WY_F", "G_MWO_MATRIX_WZ_WW_F"
};
#endif

// These flags change between ucodes
extern uint32_t G_MTX_STACKSIZE;

extern uint32_t G_MTX_MODELVIEW;
extern uint32_t G_MTX_PROJECTION;
extern uint32_t G_MTX_MUL;
extern uint32_t G_MTX_LOAD;
extern uint32_t G_MTX_NOPUSH;
extern uint32_t G_MTX_PUSH;

extern uint32_t G_TEXTURE_ENABLE;
extern uint32_t G_SHADING_SMOOTH;
extern uint32_t G_CULL_FRONT;
extern uint32_t G_CULL_BACK;
extern uint32_t G_CULL_BOTH;
extern uint32_t G_CLIPPING;

extern uint32_t G_MV_VIEWPORT;

extern uint32_t G_MWO_aLIGHT_1, G_MWO_bLIGHT_1;
extern uint32_t G_MWO_aLIGHT_2, G_MWO_bLIGHT_2;
extern uint32_t G_MWO_aLIGHT_3, G_MWO_bLIGHT_3;
extern uint32_t G_MWO_aLIGHT_4, G_MWO_bLIGHT_4;
extern uint32_t G_MWO_aLIGHT_5, G_MWO_bLIGHT_5;
extern uint32_t G_MWO_aLIGHT_6, G_MWO_bLIGHT_6;
extern uint32_t G_MWO_aLIGHT_7, G_MWO_bLIGHT_7;
extern uint32_t G_MWO_aLIGHT_8, G_MWO_bLIGHT_8;

// Image formats
#define G_IM_FMT_RGBA   0
#define G_IM_FMT_YUV    1
#define G_IM_FMT_CI     2
#define G_IM_FMT_IA     3
#define G_IM_FMT_I      4
#define G_IM_FMT_CI_IA  5   //not real

// Image sizes
#define G_IM_SIZ_4b     0
#define G_IM_SIZ_8b     1
#define G_IM_SIZ_16b    2
#define G_IM_SIZ_32b    3
#define G_IM_SIZ_DD     5

#define G_TX_MIRROR     0x1
#define G_TX_CLAMP      0x2

#ifdef DEBUG
static const char *ImageFormatText[] =
{
    "G_IM_FMT_RGBA",
    "G_IM_FMT_YUV",
    "G_IM_FMT_CI",
    "G_IM_FMT_IA",
    "G_IM_FMT_I",
    "G_IM_FMT_INVALID",
    "G_IM_FMT_INVALID",
    "G_IM_FMT_INVALID"
};

static const char *ImageSizeText[] =
{
    "G_IM_SIZ_4b",
    "G_IM_SIZ_8b",
    "G_IM_SIZ_16b",
    "G_IM_SIZ_32b"
};

static const char *SegmentText[] =
{
    "G_MWO_SEGMENT_0", "G_MWO_SEGMENT_1", "G_MWO_SEGMENT_2", "G_MWO_SEGMENT_3",
    "G_MWO_SEGMENT_4", "G_MWO_SEGMENT_5", "G_MWO_SEGMENT_6", "G_MWO_SEGMENT_7",
    "G_MWO_SEGMENT_8", "G_MWO_SEGMENT_9", "G_MWO_SEGMENT_A", "G_MWO_SEGMENT_B",
    "G_MWO_SEGMENT_C", "G_MWO_SEGMENT_D", "G_MWO_SEGMENT_E", "G_MWO_SEGMENT_F"
};
#endif

#define G_NOOP                  0x00

#define G_IMMFIRST              -65

// These GBI commands are common to all ucodes
#define G_SETCIMG               0xFF    /*  -1 */
#define G_SETZIMG               0xFE    /*  -2 */
#define G_SETTIMG               0xFD    /*  -3 */
#define G_SETCOMBINE            0xFC    /*  -4 */
#define G_SETENVCOLOR           0xFB    /*  -5 */
#define G_SETPRIMCOLOR          0xFA    /*  -6 */
#define G_SETBLENDCOLOR         0xF9    /*  -7 */
#define G_SETFOGCOLOR           0xF8    /*  -8 */
#define G_SETFILLCOLOR          0xF7    /*  -9 */
#define G_FILLRECT              0xF6    /* -10 */
#define G_SETTILE               0xF5    /* -11 */
#define G_LOADTILE              0xF4    /* -12 */
#define G_LOADBLOCK             0xF3    /* -13 */
#define G_SETTILESIZE           0xF2    /* -14 */
#define G_LOADTLUT              0xF0    /* -16 */
#define G_RDPSETOTHERMODE       0xEF    /* -17 */
#define G_SETPRIMDEPTH          0xEE    /* -18 */
#define G_SETSCISSOR            0xED    /* -19 */
#define G_SETCONVERT            0xEC    /* -20 */
#define G_SETKEYR               0xEB    /* -21 */
#define G_SETKEYGB              0xEA    /* -22 */
#define G_RDPFULLSYNC           0xE9    /* -23 */
#define G_RDPTILESYNC           0xE8    /* -24 */
#define G_RDPPIPESYNC           0xE7    /* -25 */
#define G_RDPLOADSYNC           0xE6    /* -26 */
#define G_TEXRECTFLIP           0xE5    /* -27 */
#define G_TEXRECT               0xE4    /* -28 */

#define G_RDPNOOP               0xC0

#define G_TRI_FILL              0xC8    /* fill triangle:            11001000 */
#define G_TRI_FILL_ZBUFF        0xC9    /* fill, zbuff triangle:     11001001 */
#define G_TRI_TXTR              0xCA    /* texture triangle:         11001010 */
#define G_TRI_TXTR_ZBUFF        0xCB    /* texture, zbuff triangle:  11001011 */
#define G_TRI_SHADE             0xCC    /* shade triangle:           11001100 */
#define G_TRI_SHADE_ZBUFF       0xCD    /* shade, zbuff triangle:    11001101 */
#define G_TRI_SHADE_TXTR        0xCE    /* shade, texture triangle:  11001110 */
#define G_TRI_SHADE_TXTR_ZBUFF  0xCF    /* shade, txtr, zbuff trngl: 11001111 */

/*
 * G_SETOTHERMODE_L sft: shift count
 */
#define G_MDSFT_ALPHACOMPARE    0
#define G_MDSFT_ZSRCSEL         2
#define G_MDSFT_RENDERMODE      3
#define G_MDSFT_BLENDER         16

/*
 * G_SETOTHERMODE_H sft: shift count
 */
#define G_MDSFT_BLENDMASK       0   /* unsupported */
#define G_MDSFT_ALPHADITHER     4
#define G_MDSFT_RGBDITHER       6

#define G_MDSFT_COMBKEY         8
#define G_MDSFT_TEXTCONV        9
#define G_MDSFT_TEXTFILT        12
#define G_MDSFT_TEXTLUT         14
#define G_MDSFT_TEXTLOD         16
#define G_MDSFT_TEXTDETAIL      17
#define G_MDSFT_TEXTPERSP       19
#define G_MDSFT_CYCLETYPE       20
#define G_MDSFT_COLORDITHER     22  /* unsupported in HW 2.0 */
#define G_MDSFT_PIPELINE        23

/* G_SETOTHERMODE_H gPipelineMode */
#define G_PM_1PRIMITIVE         1
#define G_PM_NPRIMITIVE         0

/* G_SETOTHERMODE_H gSetCycleType */
#define G_CYC_1CYCLE            0
#define G_CYC_2CYCLE            1
#define G_CYC_COPY              2
#define G_CYC_FILL              3

/* G_SETOTHERMODE_H gSetTexturePersp */
#define G_TP_NONE               0
#define G_TP_PERSP              1

/* G_SETOTHERMODE_H gSetTextureDetail */
#define G_TD_CLAMP              0
#define G_TD_SHARPEN            1
#define G_TD_DETAIL             2

/* G_SETOTHERMODE_H gSetTextureLOD */
#define G_TL_TILE               0
#define G_TL_LOD                1

/* G_SETOTHERMODE_H gSetTextureLUT */
#define G_TT_NONE               0
#define G_TT_RGBA16             2
#define G_TT_IA16               3

/* G_SETOTHERMODE_H gSetTextureFilter */
#define G_TF_POINT              0
#define G_TF_AVERAGE            3
#define G_TF_BILERP             2

/* G_SETOTHERMODE_H gSetTextureConvert */
#define G_TC_CONV               0
#define G_TC_FILTCONV           5
#define G_TC_FILT               6

/* G_SETOTHERMODE_H gSetCombineKey */
#define G_CK_NONE               0
#define G_CK_KEY                1

/* G_SETOTHERMODE_H gSetColorDither */
#define G_CD_MAGICSQ            0
#define G_CD_BAYER              1
#define G_CD_NOISE              2

#define G_CD_DISABLE            3
#define G_CD_ENABLE             G_CD_NOISE  /* HW 1.0 compatibility mode */

/* G_SETOTHERMODE_H gSetAlphaDither */
#define G_AD_PATTERN            0
#define G_AD_NOTPATTERN         1
#define G_AD_NOISE              2
#define G_AD_DISABLE            3

/* G_SETOTHERMODE_L gSetAlphaCompare */
#define G_AC_NONE               0
#define G_AC_THRESHOLD          1
#define G_AC_DITHER             3

/* G_SETOTHERMODE_L gSetDepthSource */
#define G_ZS_PIXEL              0
#define G_ZS_PRIM               1

/* G_SETOTHERMODE_L gSetRenderMode */
#define AA_EN                   1
#define Z_CMP                   1
#define Z_UPD                   1
#define IM_RD                   1
#define CLR_ON_CVG              1
#define CVG_DST_CLAMP           0
#define CVG_DST_WRAP            1
#define CVG_DST_FULL            2
#define CVG_DST_SAVE            3
#define ZMODE_OPA               0
#define ZMODE_INTER             1
#define ZMODE_XLU               2
#define ZMODE_DEC               3
#define CVG_X_ALPHA             1
#define ALPHA_CVG_SEL           1
#define FORCE_BL                1
#define TEX_EDGE                0 // not used

#define G_SC_NON_INTERLACE      0
#define G_SC_EVEN_INTERLACE     2
#define G_SC_ODD_INTERLACE      3

#ifdef DEBUG
static const char *AAEnableText = "AA_EN";
static const char *DepthCompareText = "Z_CMP";
static const char *DepthUpdateText = "Z_UPD";
static const char *ClearOnCvgText = "CLR_ON_CVG";
static const char *CvgXAlphaText = "CVG_X_ALPHA";
static const char *AlphaCvgSelText = "ALPHA_CVG_SEL";
static const char *ForceBlenderText = "FORCE_BL";

static const char *AlphaCompareText[] =
{
    "G_AC_NONE", "G_AC_THRESHOLD", "G_AC_INVALID", "G_AC_DITHER"
};

static const char *DepthSourceText[] =
{
    "G_ZS_PIXEL", "G_ZS_PRIM"
};

static const char *AlphaDitherText[] =
{
    "G_AD_PATTERN", "G_AD_NOTPATTERN", "G_AD_NOISE", "G_AD_DISABLE"
};

static const char *ColorDitherText[] =
{
    "G_CD_MAGICSQ", "G_CD_BAYER", "G_CD_NOISE", "G_CD_DISABLE"
};

static const char *CombineKeyText[] =
{
    "G_CK_NONE", "G_CK_KEY"
};

static const char *TextureConvertText[] =
{
    "G_TC_CONV", "G_TC_INVALID", "G_TC_INVALID", "G_TC_INVALID", "G_TC_INVALID", "G_TC_FILTCONV", "G_TC_FILT", "G_TC_INVALID"
};

static const char *TextureFilterText[] =
{
    "G_TF_POINT", "G_TF_INVALID", "G_TF_BILERP", "G_TF_AVERAGE"
};

static const char *TextureLUTText[] =
{
    "G_TT_NONE", "G_TT_INVALID", "G_TT_RGBA16", "G_TT_IA16"
};

static const char *TextureLODText[] =
{
    "G_TL_TILE", "G_TL_LOD"
};

static const char *TextureDetailText[] =
{
    "G_TD_CLAMP", "G_TD_SHARPEN", "G_TD_DETAIL"
};

static const char *TexturePerspText[] =
{
    "G_TP_NONE", "G_TP_PERSP"
};

static const char *CycleTypeText[] =
{
    "G_CYC_1CYCLE", "G_CYC_2CYCLE", "G_CYC_COPY", "G_CYC_FILL"
};

static const char *PipelineModeText[] =
{
    "G_PM_NPRIMITIVE", "G_PM_1PRIMITIVE"
};

static const char *CvgDestText[] =
{
    "CVG_DST_CLAMP", "CVG_DST_WRAP", "CVG_DST_FULL", "CVG_DST_SAVE"
};

static const char *DepthModeText[] =
{
    "ZMODE_OPA", "ZMODE_INTER", "ZMODE_XLU", "ZMODE_DEC"
};

static const char *ScissorModeText[] =
{
    "G_SC_NON_INTERLACE", "G_SC_INVALID", "G_SC_EVEN_INTERLACE", "G_SC_ODD_INTERLACE"
};
#endif

/* Color combiner constants: */
#define G_CCMUX_COMBINED        0
#define G_CCMUX_TEXEL0          1
#define G_CCMUX_TEXEL1          2
#define G_CCMUX_PRIMITIVE       3
#define G_CCMUX_SHADE           4
#define G_CCMUX_ENVIRONMENT     5
#define G_CCMUX_CENTER          6
#define G_CCMUX_SCALE           6
#define G_CCMUX_COMBINED_ALPHA  7
#define G_CCMUX_TEXEL0_ALPHA    8
#define G_CCMUX_TEXEL1_ALPHA    9
#define G_CCMUX_PRIMITIVE_ALPHA 10
#define G_CCMUX_SHADE_ALPHA     11
#define G_CCMUX_ENV_ALPHA       12
#define G_CCMUX_LOD_FRACTION    13
#define G_CCMUX_PRIM_LOD_FRAC   14
#define G_CCMUX_NOISE           7
#define G_CCMUX_K4              7
#define G_CCMUX_K5              15
#define G_CCMUX_1               6
#define G_CCMUX_0               31

/* Alpha combiner constants: */
#define G_ACMUX_COMBINED        0
#define G_ACMUX_TEXEL0          1
#define G_ACMUX_TEXEL1          2
#define G_ACMUX_PRIMITIVE       3
#define G_ACMUX_SHADE           4
#define G_ACMUX_ENVIRONMENT     5
#define G_ACMUX_LOD_FRACTION    0
#define G_ACMUX_PRIM_LOD_FRAC   6
#define G_ACMUX_1               6
#define G_ACMUX_0               7

#ifdef DEBUG
static const char *saRGBText[] =
{
    "COMBINED",         "TEXEL0",           "TEXEL1",           "PRIMITIVE",
    "SHADE",            "ENVIRONMENT",      "NOISE",            "1",
    "0",                "0",                "0",                "0",
    "0",                "0",                "0",                "0"
};

static const char *sbRGBText[] =
{
    "COMBINED",         "TEXEL0",           "TEXEL1",           "PRIMITIVE",
    "SHADE",            "ENVIRONMENT",      "CENTER",           "K4",
    "0",                "0",                "0",                "0",
    "0",                "0",                "0",                "0"
};

static const char *mRGBText[] =
{
    "COMBINED",         "TEXEL0",           "TEXEL1",           "PRIMITIVE",
    "SHADE",            "ENVIRONMENT",      "SCALE",            "COMBINED_ALPHA",
    "TEXEL0_ALPHA",     "TEXEL1_ALPHA",     "PRIMITIVE_ALPHA",  "SHADE_ALPHA",
    "ENV_ALPHA",        "LOD_FRACTION",     "PRIM_LOD_FRAC",    "K5",
    "0",                "0",                "0",                "0",
    "0",                "0",                "0",                "0",
    "0",                "0",                "0",                "0",
    "0",                "0",                "0",                "0"
};

static const char *aRGBText[] =
{
    "COMBINED",         "TEXEL0",           "TEXEL1",           "PRIMITIVE",
    "SHADE",            "ENVIRONMENT",      "1",                "0",
};

static const char *saAText[] =
{
    "COMBINED",         "TEXEL0",           "TEXEL1",           "PRIMITIVE",
    "SHADE",            "ENVIRONMENT",      "1",                "0",
};

static const char *sbAText[] =
{
    "COMBINED",         "TEXEL0",           "TEXEL1",           "PRIMITIVE",
    "SHADE",            "ENVIRONMENT",      "1",                "0",
};

static const char *mAText[] =
{
    "LOD_FRACTION",     "TEXEL0",           "TEXEL1",           "PRIMITIVE",
    "SHADE",            "ENVIRONMENT",      "PRIM_LOD_FRAC",    "0",
};

static const char *aAText[] =
{
    "COMBINED",         "TEXEL0",           "TEXEL1",           "PRIMITIVE",
    "SHADE",            "ENVIRONMENT",      "1",                "0",
};
#endif

extern uint32_t G_RDPHALF_1, G_RDPHALF_2, G_RDPHALF_CONT;
extern uint32_t G_SPNOOP;
extern uint32_t G_SETOTHERMODE_H, G_SETOTHERMODE_L;
extern uint32_t G_DL, G_ENDDL, G_CULLDL, G_BRANCH_Z;
extern uint32_t G_LOAD_UCODE;
extern uint32_t G_MOVEMEM, G_MOVEWORD;
extern uint32_t G_MTX, G_POPMTX;
extern uint32_t G_GEOMETRYMODE, G_SETGEOMETRYMODE, G_CLEARGEOMETRYMODE;
extern uint32_t G_TEXTURE;
extern uint32_t G_DMA_IO, G_DMA_DL, G_DMA_TRI, G_DMA_MTX, G_DMA_VTX, G_DMA_TEX_OFFSET, G_DMA_OFFSETS;
extern uint32_t G_SPECIAL_1, G_SPECIAL_2, G_SPECIAL_3;
extern uint32_t G_VTX, G_MODIFYVTX, G_VTXCOLORBASE;
extern uint32_t G_TRI1, G_TRI2, G_TRI4;
extern uint32_t G_QUAD, G_LINE3D;
extern uint32_t G_RESERVED0, G_RESERVED1, G_RESERVED2, G_RESERVED3;
extern uint32_t G_SPRITE2D_BASE;
extern uint32_t G_BG_1CYC, G_BG_COPY;
extern uint32_t G_OBJ_RECTANGLE, G_OBJ_SPRITE, G_OBJ_MOVEMEM;
extern uint32_t G_SELECT_DL, G_OBJ_RENDERMODE, G_OBJ_RECTANGLE_R;
extern uint32_t G_OBJ_LOADTXTR, G_OBJ_LDTX_SPRITE, G_OBJ_LDTX_RECT, G_OBJ_LDTX_RECT_R;
extern uint32_t G_RDPHALF_0, G_TRI_UNKNOWN;

#define LIGHT_1 1
#define LIGHT_2 2
#define LIGHT_3 3
#define LIGHT_4 4
#define LIGHT_5 5
#define LIGHT_6 6
#define LIGHT_7 7
#define LIGHT_8 8

#define G_DL_PUSH       0x00
#define G_DL_NOPUSH     0x01

typedef struct
{
    int16_t y;
    int16_t x;

    uint16_t flag;
    int16_t z;

    int16_t t;
    int16_t s;

    union {
        struct
        {
            uint8_t a;
            uint8_t b;
            uint8_t g;
            uint8_t r;
        } color;
        struct
        {
            int8_t a;
            int8_t z;   // b
            int8_t y;   //g
            int8_t x;   //r
        } normal;
    };
} Vertex;

typedef struct
{
    int16_t y, x;
    uint16_t ci;
    int16_t z;
    int16_t t, s;
} PDVertex;


typedef struct
{
    uint8_t      v2, v1, v0, flag;
    int16_t     t0, s0;
    int16_t     t1, s1;
    int16_t     t2, s2;
} DKRTriangle;

typedef struct
{
    uint8_t pad0, b, g, r;
    uint8_t pad1, b2, g2, r2;
    int8_t pad2, z, y, x;
} Light;

// GBI commands
typedef void (*GBIFunc)( uint32_t w0, uint32_t w1 );


typedef struct
{
    uint32_t type;
    uint32_t NoN;
    uint32_t crc;
    const char *text;
} SpecialMicrocodeInfo;

typedef struct MicrocodeInfo
{
    uint32_t address, dataAddress;
    uint16_t dataSize;
    uint32_t type;
    uint32_t NoN;
    uint32_t crc;
    uint32_t *text;

    struct MicrocodeInfo *higher, *lower;
} MicrocodeInfo;

typedef struct
{
    GBIFunc cmd[256];

    uint32_t PCStackSize, numMicrocodes;
    MicrocodeInfo *current, *top, *bottom;
} GBIInfo;

extern GBIInfo GBI;

uint32_t GBI_GetCurrentMicrocodeType(void);
void GBI_MakeCurrent( MicrocodeInfo *current );
MicrocodeInfo *GBI_DetectMicrocode( uint32_t uc_start, uint32_t uc_dstart, uint16_t uc_dsize );
extern uint32_t last_good_ucode;
void GBI_Init(void);
void GBI_Destroy(void);

// Allows easier setting of GBI commands
#define GBI_SetGBI( command, value, function ) \
    command = value; \
    GBI.cmd[command] = function

#define GBI_InitFlags( ucode ) \
    G_MTX_STACKSIZE     = ucode##_MTX_STACKSIZE; \
    G_MTX_MODELVIEW     = ucode##_MTX_MODELVIEW; \
    G_MTX_PROJECTION    = ucode##_MTX_PROJECTION; \
    G_MTX_MUL           = ucode##_MTX_MUL; \
    G_MTX_LOAD          = ucode##_MTX_LOAD; \
    G_MTX_NOPUSH        = ucode##_MTX_NOPUSH; \
    G_MTX_PUSH          = ucode##_MTX_PUSH; \
\
    G_TEXTURE_ENABLE    = ucode##_TEXTURE_ENABLE; \
    G_SHADING_SMOOTH    = ucode##_SHADING_SMOOTH; \
    G_CULL_FRONT        = ucode##_CULL_FRONT; \
    G_CULL_BACK         = ucode##_CULL_BACK; \
    G_CULL_BOTH         = ucode##_CULL_BOTH; \
    G_CLIPPING          = ucode##_CLIPPING; \
\
    G_MV_VIEWPORT       = ucode##_MV_VIEWPORT; \
\
    G_MWO_aLIGHT_1      = ucode##_MWO_aLIGHT_1; \
    G_MWO_bLIGHT_1      = ucode##_MWO_bLIGHT_1; \
    G_MWO_aLIGHT_2      = ucode##_MWO_aLIGHT_2; \
    G_MWO_bLIGHT_2      = ucode##_MWO_bLIGHT_2; \
    G_MWO_aLIGHT_3      = ucode##_MWO_aLIGHT_3; \
    G_MWO_bLIGHT_3      = ucode##_MWO_bLIGHT_3; \
    G_MWO_aLIGHT_4      = ucode##_MWO_aLIGHT_4; \
    G_MWO_bLIGHT_4      = ucode##_MWO_bLIGHT_4; \
    G_MWO_aLIGHT_5      = ucode##_MWO_aLIGHT_5; \
    G_MWO_bLIGHT_5      = ucode##_MWO_bLIGHT_5; \
    G_MWO_aLIGHT_6      = ucode##_MWO_aLIGHT_6; \
    G_MWO_bLIGHT_6      = ucode##_MWO_bLIGHT_6; \
    G_MWO_aLIGHT_7      = ucode##_MWO_aLIGHT_7; \
    G_MWO_bLIGHT_7      = ucode##_MWO_bLIGHT_7; \
    G_MWO_aLIGHT_8      = ucode##_MWO_aLIGHT_8; \
    G_MWO_bLIGHT_8      = ucode##_MWO_bLIGHT_8;

#ifdef __cplusplus
}
#endif

#endif

