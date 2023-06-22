#ifndef GBI_H
#define GBI_H

#ifdef __cplusplus
extern "C" {
#endif

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
#define F3DWRUS     10
#define F3DCBFD     11
#define NONE        12


#define F3DCBFD_MV_VIEWPORT     8
#define F3DCBFD_MV_LIGHT        10
#define F3DCBFD_MV_NORMAL       14

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

/* Useful macros for decoding GBI command's parameters */
#define _SHIFTL( v, s, w )  (((uint32_t)v & ((0x01 << w) - 1)) << s)
#define _SHIFTR( v, s, w )  (((uint32_t)v >> s) & ((0x01 << w) - 1))

/* BG flags */
#define G_BGLT_LOADBLOCK    0x0033
#define G_BGLT_LOADTILE     0xfff4

#define G_BG_FLAG_FLIPS     0x01
#define G_BG_FLAG_FLIPT     0x10

/* Sprite object render modes */
#define G_OBJRM_NOTXCLAMP       0x01
#define G_OBJRM_XLU             0x02    /* Ignored */
#define G_OBJRM_ANTIALIAS       0x04    /* Ignored */
#define G_OBJRM_BILERP          0x08
#define G_OBJRM_SHRINKSIZE_1    0x10
#define G_OBJRM_SHRINKSIZE_2    0x20
#define G_OBJRM_WIDEN           0x40

/* Sprite texture loading types */
#define G_OBJLT_TXTRBLOCK   0x00001033
#define G_OBJLT_TXTRTILE    0x00fc1034
#define G_OBJLT_TLUT        0x00000030

/* These are all the constant flags */
#define G_ZBUFFER               0x00000001  /* Enables Z buffer calculations.
                                             *
                                             * Other Z buffer-related parameters
                                             * for the framebuffer must also be
                                             * set. */
#define G_TEXTURE_ENABLE        0x00000002  /* Microcode use only  */
#define G_SHADE                 0x00000004  /* Calculate vertex color.
                                             * 
                                             * Enables calculation of 
                                             * the vertex color for
                                             * a triangle. */

#define G_SHADING_SMOOTH        0x00000200  /* Enables Gouraud shading.
                                             * 
                                             * When this is not enabled, 
                                             * flat shading is used for the 
                                             * triangle based on the color
                                             * of one vertex (see gSP1Triangle).
                                             * */
#define G_CULL_FRONT            0x00001000  /* Enables front-face culling.
                                             *
                                             * This does not support F3DLX.Rej
                                             * but it does support F3DLX2.Rej.
                                             */
#define G_CULL_BACK             0x00002000  /* Enables back-face culling. */
#define G_CULL_BOTH             0x00003000  /* Enables both back-face and
                                             * front-face culling.
                                             *
                                             * This does not support F3DLX.Rej
                                             * but it does support F3DLX2.Rej. */
#define G_FOG                   0x00010000  /* Enables generation of vertex
                                             * alpha coordinate fog parameters. */
#define G_LIGHTING              0x00020000  /* Enables lighting calculations. */
#define G_TEXTURE_GEN           0x00040000  /* Enables automatic generation of
                                             * the texture's S/T coordinates.
                                             *
                                             * Spherical mapping based on the 
                                             * normal vector is used.
                                             */
#define G_TEXTURE_GEN_LINEAR    0x00080000  /* Enables automatic generation of
                                             * the texture's S/T coordinates. */
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

/* MOVEMEM indices
 *
 * each of these indexes an entry
 * in a DMEM table which points to
 * a 1-4 word block of DMEM in which
 * to store a 1-4 word DMA. */
#define F3D_MV_VIEWPORT 0x80
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
#define G_MV_MATRIX_1   0x9E /* NOTE: this is in moveword table */
#define G_MV_MATRIX_2   0x98
#define G_MV_MATRIX_3   0x9A
#define G_MV_MATRIX_4   0x9C

/* MOVEWORD indices
 *
 * each of these indexes an entry
 * in a DMEM table which points to
 * a word in DMEM where an immediate
 * word will be stored. */
#define G_MW_MATRIX         0x00
#define G_MW_NUMLIGHT       0x02
#define G_MW_CLIP           0x04
#define G_MW_SEGMENT        0x06
#define G_MW_FOG            0x08
#define G_MW_LIGHTCOL       0x0A
#define G_MW_FORCEMTX       0x0C
#define G_MW_POINTS         0x0C
#define G_MW_PERSPNORM      0x0E
#define G_MV_COORDMOD       0x10    /* Conker Bad Fur Day */

/* These are offsets from the address in the DMEM table */
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

#define F3D_MWO_aLIGHT_1        0x00
#define F3D_MWO_bLIGHT_1        0x04
#define F3D_MWO_aLIGHT_2        0x20
#define F3D_MWO_bLIGHT_2        0x24
#define F3D_MWO_aLIGHT_3        0x40
#define F3D_MWO_bLIGHT_3        0x44
#define F3D_MWO_aLIGHT_4        0x60
#define F3D_MWO_bLIGHT_4        0x64
#define F3D_MWO_aLIGHT_5        0x80
#define F3D_MWO_bLIGHT_5        0x84
#define F3D_MWO_aLIGHT_6        0xa0
#define F3D_MWO_bLIGHT_6        0xa4
#define F3D_MWO_aLIGHT_7        0xc0
#define F3D_MWO_bLIGHT_7        0xc4
#define F3D_MWO_aLIGHT_8        0xe0
#define F3D_MWO_bLIGHT_8        0xe4


#define F3DEX2_MWO_aLIGHT_1     0x00
#define F3DEX2_MWO_bLIGHT_1     0x04
#define F3DEX2_MWO_aLIGHT_2     0x18
#define F3DEX2_MWO_bLIGHT_2     0x1c
#define F3DEX2_MWO_aLIGHT_3     0x30
#define F3DEX2_MWO_bLIGHT_3     0x34
#define F3DEX2_MWO_aLIGHT_4     0x48
#define F3DEX2_MWO_bLIGHT_4     0x4c
#define F3DEX2_MWO_aLIGHT_5     0x60
#define F3DEX2_MWO_bLIGHT_5     0x64
#define F3DEX2_MWO_aLIGHT_6     0x78
#define F3DEX2_MWO_bLIGHT_6     0x7c
#define F3DEX2_MWO_aLIGHT_7     0x90
#define F3DEX2_MWO_bLIGHT_7     0x94
#define F3DEX2_MWO_aLIGHT_8     0xa8
#define F3DEX2_MWO_bLIGHT_8     0xac

#define F3DEX2_RDPHALF_2        0xF1
#define F3DEX2_SETOTHERMODE_H   0xE3
#define F3DEX2_SETOTHERMODE_L   0xE2
#define F3DEX2_RDPHALF_1        0xE1
#define F3DEX2_SPNOOP           0xE0
#define F3DEX2_ENDDL            0xDF
#define F3DEX2_DL               0xDE
#define F3DEX2_LOAD_UCODE       0xDD
#define F3DEX2_MOVEMEM          0xDC
#define F3DEX2_MOVEWORD         0xDB
#define F3DEX2_MTX              0xDA
#define F3DEX2_GEOMETRYMODE     0xD9
#define F3DEX2_POPMTX           0xD8
#define F3DEX2_TEXTURE          0xD7
#define F3DEX2_DMA_IO           0xD6
#define F3DEX2_SPECIAL_1        0xD5
#define F3DEX2_SPECIAL_2        0xD4
#define F3DEX2_SPECIAL_3        0xD3

#define F3DEX2_VTX              0x01
#define F3DEX2_MODIFYVTX        0x02
#define F3DEX2_CULLDL           0x03
#define F3DEX2_BRANCH_Z         0x04
#define F3DEX2_TRI1             0x05
#define F3DEX2_TRI2             0x06
#define F3DEX2_QUAD             0x07
#define F3DEX2_LINE3D           0x08

#define F3DEX2_MV_VIEWPORT      8

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

/* Image formats */
#define G_IM_FMT_RGBA   0
#define G_IM_FMT_YUV    1
#define G_IM_FMT_CI     2
#define G_IM_FMT_IA     3
#define G_IM_FMT_I      4
#define G_IM_FMT_CI_IA  5   /* not real */

/* Image sizes */
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

/* RDP commands */
/* These GBI commands are common to all ucodes */
#define G_RDPNOOP               0xC0
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

#define G_TRI_FILL              0xC8    /* fill triangle:            11001000 */
#define G_TRI_FILL_ZBUFF        0xC9    /* fill, zbuff triangle:     11001001 */
#define G_TRI_TXTR              0xCA    /* texture triangle:         11001010 */
#define G_TRI_TXTR_ZBUFF        0xCB    /* texture, zbuff triangle:  11001011 */
#define G_TRI_SHADE             0xCC    /* shade triangle:           11001100 */
#define G_TRI_SHADE_ZBUFF       0xCD    /* shade, zbuff triangle:    11001101 */
#define G_TRI_SHADE_TXTR        0xCE    /* shade, texture triangle:  11001110 */
#define G_TRI_SHADE_TXTR_ZBUFF  0xCF    /* shade, txtr, zbuff trngl: 11001111 */

#define G_SETOTHERMODE_H	     0xe3
#define G_SETOTHERMODE_L	     0xe2

/* G_SETOTHERMODE_L sft: shift count */
#define G_MDSFT_ALPHACOMPARE    0
#define G_MDSFT_ZSRCSEL         2
#define G_MDSFT_RENDERMODE      3
#define G_MDSFT_BLENDER         16

/* G_SETOTHERMODE_H sft: shift count */
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
#define CLR_ON_CVG              0x80
#define CVG_DST_CLAMP           0
#define CVG_DST_WRAP            1
#define CVG_DST_FULL            2
#define CVG_DST_SAVE            3
#define ZMODE_OPA               0
#define ZMODE_INTER             1
#define ZMODE_XLU               2
#define ZMODE_DEC               3
#define CVG_X_ALPHA             1
#define FORCE_BL                0x4000
#define ALPHA_CVG_SEL           0x2000
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

#define LIGHT_1 1
#define LIGHT_2 2
#define LIGHT_3 3
#define LIGHT_4 4
#define LIGHT_5 5
#define LIGHT_6 6
#define LIGHT_7 7
#define LIGHT_8 8

/* Flags to inhibit pushing of the display list (on branch) */
#define G_DL_PUSH          0x00
#define G_DL_NOPUSH        0x01

/* Blender */
#define BLEND_FOG_ASHADE   0xc800
#define BLEND_FOG_APRIME   0xc400
#define BLEND_XLU          0x0040

#define ZMODE_DECAL        0xc00
#define ZLUT_SIZE          0x40000

#define X_CLIP_MAX  0x01
#define X_CLIP_MIN  0x02
#define Y_CLIP_MAX  0x04
#define Y_CLIP_MIN  0x08
#define Z_CLIP_MAX  0x10
#define Z_CLIP_MIN  0x20

/* G_MTX: parameter flags */
# define G_MTX_MODELVIEW	0x00	/* matrix types */
# define G_MTX_PROJECTION	0x04
# define G_MTX_MUL		   0x00	/* concat or load */
# define G_MTX_LOAD		   0x02
# define G_MTX_NOPUSH		0x00	/* push or not */
# define G_MTX_PUSH		   0x01

#define RDRAM_UWORD(addr)   (*(uint32_t *)((addr)+ (uint8_t*)gfx_info.RDRAM))
#define RDRAM_SWORD(addr)   (*(int32_t *)((addr)+ (uint8_t*)gfx_info.RDRAM))
#define RDRAM_UHALF(addr)   (*(uint16_t *)(((addr)^2) + (uint8_t*)gfx_info.RDRAM))
#define RDRAM_SHALF(addr)   (*(int16_t *)(((addr)^2)+ (uint8_t*)gfx_info.RDRAM))
#define RDRAM_UBYTE(addr)   (*(uint8_t *)(((addr)^3)+ (uint8_t*)gfx_info.RDRAM))
#define RDRAM_SBYTE(addr)   (*(int8_t *)(((addr)^3) + (uint8_t*)gfx_info.RDRAM))
#define pRDRAM_UWORD(addr)  ((uint32_t *)((addr) + (uint8_t*)gfx_info.RDRAM))
#define pRDRAM_SWORD(addr)  ((int32_t *)((addr)+ (uint8_t*)gfx_info.RDRAM))
#define pRDRAM_UHALF(addr)  ((uint16_t *)(((addr)^2) + (uint8_t*)gfx_info.RDRAM))
#define pRDRAM_SHALF(addr)  ((int16_t*)(((addr)^2)  + (uint8_t*)gfx_info.RDRAM))
#define pRDRAM_UBYTE(addr)  ((uint8_t *)(((addr)^3) + (uint8_t*)gfx_info.RDRAM))
#define pRDRAM_SBYTE(addr)  ((int8_t *)(((addr)^3)) + (uint8_t*)gfx_info.RDRAM)

#ifdef __cplusplus
}
#endif

#endif

