/*************************************************************************

	3dfx Voodoo Graphics SST-1 emulator

	driver by Aaron Giles

**************************************************************************/

#include "driver.h"
#include "voodoo.h"
#include "cpu/mips/mips3.h"
#include <math.h>


/* math trickery */
#if !defined(_WIN32) || defined(__LIBRETRO__) /* HACK: No controlfp */
#define SETUP_FPU()
#define RESTORE_FPU()
#define TRUNC_TO_INT(f) (float) (floor(f))
#else
#include <float.h>
#define SETUP_FPU() { int oldfpu = _controlfp(_RC_CHOP/*_RC_DOWN*/ | _PC_24, _MCW_RC | _MCW_PC)
#define RESTORE_FPU() _controlfp(oldfpu, _MCW_RC | _MCW_PC); }
#define TRUNC_TO_INT(f) ((int)(f))
#endif


/* optimizations */
#define BILINEAR_FILTER			(1)
#define PER_PIXEL_LOD			(1)
#define ALPHA_BLENDING			(1)
#define FOGGING					(1)

/* debugging */
#define DISPLAY_SWAP_BUFFERS	(0)
#define DISPLAY_TEXTURE_MODES	(0)
#define DISPLAY_STATISTICS		(0)
#define DISPLAY_DEPTHBUF		(0)
#define LOG_RENDERERS			(0)
#define LOG_REGISTERS			(0)
#define LOG_COMMANDS			(0)
#define LOG_CMDFIFO				(0)
#define LOG_CMDFIFO_VERBOSE		(0)
#define TRACK_LOD				(0)

/* constants */
#define MAX_TMUS				(4)

#define FRAMEBUF_WIDTH			(1024)
#define FRAMEBUF_HEIGHT			(1024)

#define CMDFIFO_SIZE			(2*1024*1024)

#define FBZCOLORPATH_MASK		(0x0fffffff)
#define ALPHAMODE_MASK			(0xffffffff)
#define FOGMODE_MASK			(0x0000001f)
#define FBZMODE_MASK			(0x001fffff)
#define TEXTUREMODE0_MASK		(0xfffff8df)
#define TEXTUREMODE1_MASK		(0x00000000)



/* temporary holding for triangle setup */
struct setup_vert
{
	float x,y;
	float a,r,g,b;
	float z,wb;
	float w0,s0,t0;
	float w1,s1,t1;
};



/* core constants */
static UINT8 tmus;
static UINT8 voodoo2;
static offs_t texram_mask;

/* VRAM and various buffers */
static UINT16 *framebuf[2];
static UINT16 *depthbuf;
static UINT16 *frontbuf;
static UINT16 *backbuf;
static UINT16 **buffer_access[4];
static UINT8 *textureram[MAX_TMUS];
static UINT32 *cmdfifo;
static UINT32 cmdfifo_expected;

static UINT16 *pen_lookup;
static UINT8 *dither_lookup;
static float *depth_lookup;
static UINT8 *lod_lookup;

/* register pointers */
data32_t *voodoo_regs;
static float *fvoodoo_regs;

/* color DAC fake registers */
static UINT8 dac_regs[8];
static UINT8 dac_read;

static data32_t init_enable;

/* texel tables */
static UINT32 *texel_lookup[MAX_TMUS][16];
static UINT8 texel_lookup_dirty[MAX_TMUS][16];
static INT32 ncc_y[MAX_TMUS][2][16];
static INT32 ncc_ir[MAX_TMUS][2][4], ncc_ig[MAX_TMUS][2][4], ncc_ib[MAX_TMUS][2][4];
static INT32 ncc_qr[MAX_TMUS][2][4], ncc_qg[MAX_TMUS][2][4], ncc_qb[MAX_TMUS][2][4];

/* fog tables */
static UINT8 fog_blend[64];
static UINT8 fog_delta[64];

/* VBLANK and swapping */
static void *vblank_timer;
static int vblank_count;
static int swaps_pending;
static int vblanks_before_swap;
static int blocked_on_swap;
static int pending_fastfill;
static UINT32 pending_fastfill_zaColor;
static UINT32 pending_fastfill_color1;

/* fbzMode variables */
static struct rectangle *fbz_cliprect;
static struct rectangle fbz_noclip;
static struct rectangle fbz_clip;
static UINT8 fbz_chroma_key;
static UINT8 fbz_stipple_mask;
static UINT8 fbz_wbuffer_select;
static UINT8 fbz_depth_buffering;
static UINT8 fbz_depth_buffer_func;
static UINT8 fbz_dithering;
static UINT8 fbz_rgb_write;
static UINT8 fbz_depth_write;
static const UINT8 *fbz_dither_matrix;
static UINT16 **fbz_draw_buffer;
static UINT8 fbz_invert_y;

/* lfbMode variables */
static UINT8 lfb_write_format;
static UINT16 **lfb_write_buffer;
static UINT16 **lfb_read_buffer;
static UINT8 lfb_flipy;

/* videoDimensions variables */
static int video_width;
static int video_height;

/* fbiInit variables */
static UINT8 triple_buffer;
static UINT16 inverted_yorigin;

/* textureMode variables */
static UINT8 trex_perspective[3];
static UINT8 trex_minification[3];
static UINT8 trex_magnification[3];
static UINT8 trex_clamps[3];
static UINT8 trex_clampt[3];
static UINT8 trex_format[3];
static UINT8 trex_lodmin[3];
static UINT8 trex_lodmax[3];
static INT8 trex_lodbias[3];
static UINT32 trex_width[3];
static UINT32 trex_height[3];
static const UINT32 *trex_lod_offset[3];
static const UINT8 *trex_lod_width_shift[3];

/* triangle parameters */
struct tri_vertex { float x, y; };
static struct tri_vertex tri_va, tri_vb, tri_vc;
static INT32 tri_startr, tri_drdx, tri_drdy;	/* .16 */
static INT32 tri_startg, tri_dgdx, tri_dgdy;	/* .16 */
static INT32 tri_startb, tri_dbdx, tri_dbdy;	/* .16 */
static INT32 tri_starta, tri_dadx, tri_dady;	/* .16 */
static INT32 tri_startz, tri_dzdx, tri_dzdy;	/* .12 */
static float tri_startw, tri_dwdx, tri_dwdy;
static float tri_starts0, tri_ds0dx, tri_ds0dy;
static float tri_startt0, tri_dt0dx, tri_dt0dy;
static float tri_startw0, tri_dw0dx, tri_dw0dy;
static float tri_starts1, tri_ds1dx, tri_ds1dy;
static float tri_startt1, tri_dt1dx, tri_dt1dy;
static float tri_startw1, tri_dw1dx, tri_dw1dy;

/* triangle setup */
static int setup_count;
static struct setup_vert setup_verts[3];
static struct setup_vert setup_pending;

/* debugging/stats */
#if DISPLAY_STATISTICS
static UINT32 polycount, pixelcount, lastfps, framecount, totalframes;
#endif
static UINT16 modes_used;
static offs_t status_lastpc;
static int status_lastpc_count;

#if LOG_RENDERERS
#define RENDERER_LIST_MAX	1024
static UINT32 renderer_list[RENDERER_LIST_MAX][10];
static int renderer_listcount;
#endif

static const UINT8 dither_matrix_4x4[16] =
{
	 0,12, 3,14,
	 8, 4,11, 7,
	 2,15, 1,13,
	10, 6, 9, 5
};

static const UINT8 dither_matrix_2x2[16] =
{
	 0, 8, 0, 8,
	12, 4,12, 4,
	 0, 8, 0, 8,
	12, 4,12, 4
};

static const UINT8 lod_dither_matrix[16] =
{
	 0, 2, 1, 3,
	 1, 0, 3, 2,
	 2, 3, 0, 1,
	 3, 1, 2, 0
};

static const UINT32 lod_offset_table[4][16] =
{
	{ 0x00000, 0x10000, 0x14000, 0x15000, 0x15400, 0x15500, 0x15540, 0x15550, 0x15554 },
	{ 0x00000, 0x08000, 0x0a000, 0x0a800, 0x0aa00, 0x0aa80, 0x0aaa0, 0x0aaa8, 0x0aaa8 },
	{ 0x00000, 0x04000, 0x05000, 0x05400, 0x05500, 0x05540, 0x05550, 0x05550, 0x05550 },
	{ 0x00000, 0x02000, 0x02800, 0x02a00, 0x02a80, 0x02aa0, 0x02aa0, 0x02aa0, 0x02aa0 }
};

static const UINT8 lod_width_shift[8][16] =
{
	{ 8,7,6,5,4,3,2,1,0 },
	{ 8,7,6,5,4,3,2,1,0 },
	{ 7,6,5,4,3,2,1,0,0 },
	{ 8,7,6,5,4,3,2,1,0 },
	{ 6,5,4,3,2,1,0,0,0 },
	{ 8,7,6,5,4,3,2,1,0 },
	{ 5,4,3,2,1,0,0,0,0 },
	{ 8,7,6,5,4,3,2,1,0 }
};





static void generic_render_1tmu(void);
static void generic_render_2tmu(void);

static void render_0c000035_00045119_000b4779_0824101f(void);
static void render_0c000035_00045119_000b4779_0824109f(void);
static void render_0c000035_00045119_000b4779_082410df(void);
static void render_0c000035_00045119_000b4779_082418df(void);

static void render_0c600c09_00045119_000b4779_0824100f(void);
static void render_0c600c09_00045119_000b4779_0824180f(void);
static void render_0c600c09_00045119_000b4779_082418cf(void);
static void render_0c480035_00045119_000b4779_082418df(void);
static void render_0c480035_00045119_000b4379_082418df(void);

static void render_0c000035_00040400_000b4739_0c26180f(void);
static void render_0c582c35_00515110_000b4739_0c26180f(void);
static void render_0c000035_64040409_000b4739_0c26180f(void);
static void render_0c002c35_64515119_000b4799_0c26180f(void);
static void render_0c582c35_00515110_000b4739_0c2618cf(void);
static void render_0c002c35_40515119_000b4739_0c26180f(void);

static void fastfill(void);



#if DISPLAY_STATISTICS
#define ADD_TO_PIXEL_COUNT(a)	do { if ((a) > 0) pixelcount += (a); } while (0)
#else
#define ADD_TO_PIXEL_COUNT(a)
#endif


/*************************************
 *
 *	Register constants
 *
 *************************************/

/* 0x000 */
#define status			(0x000/4)
#define intrCtrl		(0x004/4)	/* Voodoo2 only */
#define vertexAx		(0x008/4)
#define vertexAy		(0x00c/4)
#define vertexBx		(0x010/4)
#define vertexBy		(0x014/4)
#define vertexCx		(0x018/4)
#define vertexCy		(0x01c/4)
#define startR			(0x020/4)
#define startG			(0x024/4)
#define startB			(0x028/4)
#define startZ			(0x02c/4)
#define startA			(0x030/4)
#define startS			(0x034/4)
#define startT			(0x038/4)
#define startW			(0x03c/4)

/* 0x040 */
#define dRdX			(0x040/4)
#define dGdX			(0x044/4)
#define dBdX			(0x048/4)
#define dZdX			(0x04c/4)
#define dAdX			(0x050/4)
#define dSdX			(0x054/4)
#define dTdX			(0x058/4)
#define dWdX			(0x05c/4)
#define dRdY			(0x060/4)
#define dGdY			(0x064/4)
#define dBdY			(0x068/4)
#define dZdY			(0x06c/4)
#define dAdY			(0x070/4)
#define dSdY			(0x074/4)
#define dTdY			(0x078/4)
#define dWdY			(0x07c/4)

/* 0x080 */
#define triangleCMD		(0x080/4)
#define fvertexAx		(0x088/4)
#define fvertexAy		(0x08c/4)
#define fvertexBx		(0x090/4)
#define fvertexBy		(0x094/4)
#define fvertexCx		(0x098/4)
#define fvertexCy		(0x09c/4)
#define fstartR			(0x0a0/4)
#define fstartG			(0x0a4/4)
#define fstartB			(0x0a8/4)
#define fstartZ			(0x0ac/4)
#define fstartA			(0x0b0/4)
#define fstartS			(0x0b4/4)
#define fstartT			(0x0b8/4)
#define fstartW			(0x0bc/4)

/* 0x0c0 */
#define fdRdX			(0x0c0/4)
#define fdGdX			(0x0c4/4)
#define fdBdX			(0x0c8/4)
#define fdZdX			(0x0cc/4)
#define fdAdX			(0x0d0/4)
#define fdSdX			(0x0d4/4)
#define fdTdX			(0x0d8/4)
#define fdWdX			(0x0dc/4)
#define fdRdY			(0x0e0/4)
#define fdGdY			(0x0e4/4)
#define fdBdY			(0x0e8/4)
#define fdZdY			(0x0ec/4)
#define fdAdY			(0x0f0/4)
#define fdSdY			(0x0f4/4)
#define fdTdY			(0x0f8/4)
#define fdWdY			(0x0fc/4)

/* 0x100 */
#define ftriangleCMD	(0x100/4)
#define fbzColorPath	(0x104/4)
#define fogMode			(0x108/4)
#define alphaMode		(0x10c/4)
#define fbzMode			(0x110/4)
#define lfbMode			(0x114/4)
#define clipLeftRight	(0x118/4)
#define clipLowYHighY	(0x11c/4)
#define nopCMD			(0x120/4)
#define fastfillCMD		(0x124/4)
#define swapbufferCMD	(0x128/4)
#define fogColor		(0x12c/4)
#define zaColor			(0x130/4)
#define chromaKey		(0x134/4)
#define chromaRange		(0x138/4)	/* Voodoo2 only */
#define userIntrCMD		(0x13c/4)	/* Voodoo2 only */

/* 0x140 */
#define stipple			(0x140/4)
#define color0			(0x144/4)
#define color1			(0x148/4)
#define fbiPixelsIn		(0x14c/4)
#define fbiChromaFail	(0x150/4)
#define fbiZfuncFail	(0x154/4)
#define fbiAfuncFail	(0x158/4)
#define fbiPixelsOut	(0x15c/4)
#define fogTable		(0x160/4)

/* 0x1c0 */
#define cmdFifoBaseAddr	(0x1e0/4)	/* Voodoo2 only */
#define cmdFifoBump		(0x1e4/4)	/* Voodoo2 only */
#define cmdFifoRdPtr	(0x1e8/4)	/* Voodoo2 only */
#define cmdFifoAMin		(0x1ec/4)	/* Voodoo2 only */
#define cmdFifoAMax		(0x1f0/4)	/* Voodoo2 only */
#define cmdFifoDepth	(0x1f4/4)	/* Voodoo2 only */
#define cmdFifoHoles	(0x1f8/4)	/* Voodoo2 only */

/* 0x200 */
#define fbiInit4		(0x200/4)
#define vRetrace		(0x204/4)
#define backPorch		(0x208/4)
#define videoDimensions	(0x20c/4)
#define fbiInit0		(0x210/4)
#define fbiInit1		(0x214/4)
#define fbiInit2		(0x218/4)
#define fbiInit3		(0x21c/4)
#define hSync			(0x220/4)
#define vSync			(0x224/4)
#define clutData		(0x228/4)
#define dacData			(0x22c/4)
#define maxRgbDelta		(0x230/4)
#define hBorder			(0x234/4)	/* Voodoo2 only */
#define vBorder			(0x238/4)	/* Voodoo2 only */
#define borderColor		(0x23c/4)	/* Voodoo2 only */

/* 0x240 */
#define hvRetrace		(0x240/4)	/* Voodoo2 only */
#define fbiInit5		(0x244/4)	/* Voodoo2 only */
#define fbiInit6		(0x248/4)	/* Voodoo2 only */
#define fbiInit7		(0x24c/4)	/* Voodoo2 only */
#define fbiSwapHistory	(0x258/4)	/* Voodoo2 only */
#define fbiTrianglesOut	(0x25c/4)	/* Voodoo2 only */
#define sSetupMode		(0x260/4)	/* Voodoo2 only */
#define sVx				(0x264/4)	/* Voodoo2 only */
#define sVy				(0x268/4)	/* Voodoo2 only */
#define sARGB			(0x26c/4)	/* Voodoo2 only */
#define sRed			(0x270/4)	/* Voodoo2 only */
#define sGreen			(0x274/4)	/* Voodoo2 only */
#define sBlue			(0x278/4)	/* Voodoo2 only */
#define sAlpha			(0x27c/4)	/* Voodoo2 only */

/* 0x280 */
#define sVz				(0x280/4)	/* Voodoo2 only */
#define sWb				(0x284/4)	/* Voodoo2 only */
#define sWtmu0			(0x288/4)	/* Voodoo2 only */
#define sS_W0			(0x28c/4)	/* Voodoo2 only */
#define sT_W0			(0x290/4)	/* Voodoo2 only */
#define sWtmu1			(0x294/4)	/* Voodoo2 only */
#define sS_Wtmu1		(0x298/4)	/* Voodoo2 only */
#define sT_Wtmu1		(0x29c/4)	/* Voodoo2 only */
#define sDrawTriCMD		(0x2a0/4)	/* Voodoo2 only */
#define sBeginTriCMD	(0x2a4/4)	/* Voodoo2 only */

/* 0x2c0 */
#define bltSrcBaseAddr	(0x2c0/4)	/* Voodoo2 only */
#define bltDstBaseAddr	(0x2c4/4)	/* Voodoo2 only */
#define bltXYStrides	(0x2c8/4)	/* Voodoo2 only */
#define bltSrcChromaRange (0x2cc/4)	/* Voodoo2 only */
#define bltDstChromaRange (0x2d0/4)	/* Voodoo2 only */
#define bltClipX		(0x2d4/4)	/* Voodoo2 only */
#define bltClipY		(0x2d8/4)	/* Voodoo2 only */
#define bltSrcXY		(0x2e0/4)	/* Voodoo2 only */
#define bltDstXY		(0x2e4/4)	/* Voodoo2 only */
#define bltSize			(0x2e8/4)	/* Voodoo2 only */
#define bltRop			(0x2ec/4)	/* Voodoo2 only */
#define bltColor		(0x2f0/4)	/* Voodoo2 only */
#define bltCommand		(0x2f8/4)	/* Voodoo2 only */
#define BltData			(0x2fc/4)	/* Voodoo2 only */

/* 0x300 */
#define textureMode		(0x300/4)
#define tLOD			(0x304/4)
#define tDetail			(0x308/4)
#define texBaseAddr		(0x30c/4)
#define texBaseAddr_1	(0x310/4)
#define texBaseAddr_2	(0x314/4)
#define texBaseAddr_3_8	(0x318/4)
#define trexInit0		(0x31c/4)
#define trexInit1		(0x320/4)
#define nccTable		(0x324/4)



/*************************************
 *
 *	Register string table for debug
 *
 *************************************/

static const char *voodoo_reg_name[] =
{
	/* 0x000 */
	"status",		"{intrCtrl}",	"vertexAx",		"vertexAy",
	"vertexBx",		"vertexBy",		"vertexCx",		"vertexCy",
	"startR",		"startG",		"startB",		"startZ",
	"startA",		"startS",		"startT",		"startW",
	/* 0x040 */
	"dRdX",			"dGdX",			"dBdX",			"dZdX",
	"dAdX",			"dSdX",			"dTdX",			"dWdX",
	"dRdY",			"dGdY",			"dBdY",			"dZdY",
	"dAdY",			"dSdY",			"dTdY",			"dWdY",
	/* 0x080 */
	"triangleCMD",	"reserved084",	"fvertexAx",	"fvertexAy",
	"fvertexBx",	"fvertexBy",	"fvertexCx",	"fvertexCy",
	"fstartR",		"fstartG",		"fstartB",		"fstartZ",
	"fstartA",		"fstartS",		"fstartT",		"fstartW",
	/* 0x0c0 */
	"fdRdX",		"fdGdX",		"fdBdX",		"fdZdX",
	"fdAdX",		"fdSdX",		"fdTdX",		"fdWdX",
	"fdRdY",		"fdGdY",		"fdBdY",		"fdZdY",
	"fdAdY",		"fdSdY",		"fdTdY",		"fdWdY",
	/* 0x100 */
	"ftriangleCMD",	"fbzColorPath",	"fogMode",		"alphaMode",
	"fbzMode",		"lfbMode",		"clipLeftRight","clipLowYHighY",
	"nopCMD",		"fastfillCMD",	"swapbufferCMD","fogColor",
	"zaColor",		"chromaKey",	"{chromaRange}","{userIntrCMD}",
	/* 0x140 */
	"stipple",		"color0",		"color1",		"fbiPixelsIn",
	"fbiChromaFail","fbiZfuncFail",	"fbiAfuncFail",	"fbiPixelsOut",
	"fogTable160",	"fogTable164",	"fogTable168",	"fogTable16c",	
	"fogTable170",	"fogTable174",	"fogTable178",	"fogTable17c",	
	/* 0x180 */
	"fogTable180",	"fogTable184",	"fogTable188",	"fogTable18c",
	"fogTable190",	"fogTable194",	"fogTable198",	"fogTable19c",
	"fogTable1a0",	"fogTable1a4",	"fogTable1a8",	"fogTable1ac",
	"fogTable1b0",	"fogTable1b4",	"fogTable1b8",	"fogTable1bc",
	/* 0x1c0 */
	"fogTable1c0",	"fogTable1c4",	"fogTable1c8",	"fogTable1cc",
	"fogTable1d0",	"fogTable1d4",	"fogTable1d8",	"fogTable1dc",
	"{cmdFifoBaseAddr}","{cmdFifoBump}","{cmdFifoRdPtr}","{cmdFifoAMin}",
	"{cmdFifoAMax}","{cmdFifoDepth}","{cmdFifoHoles}","reserved1fc",
	/* 0x200 */
	"fbiInit4",		"vRetrace",		"backPorch",	"videoDimensions",
	"fbiInit0",		"fbiInit1",		"fbiInit2",		"fbiInit3",
	"hSync",		"vSync",		"clutData",		"dacData",
	"maxRgbDelta",	"{hBorder}",	"{vBorder}",	"{borderColor}",
	/* 0x240 */
	"{hvRetrace}",	"{fbiInit5}",	"{fbiInit6}",	"{fbiInit7}",
	"reserved250",	"reserved254",	"{fbiSwapHistory}","{fbiTrianglesOut}",
	"{sSetupMode}",	"{sVx}",		"{sVy}",		"{sARGB}",
	"{sRed}",		"{sGreen}",		"{sBlue}",		"{sAlpha}",
	/* 0x280 */
	"{sVz}",		"{sWb}",		"{sWtmu0}",		"{sS/Wtmu0}",
	"{sT/Wtmu0}",	"{sWtmu1}",		"{sS/Wtmu1}",	"{sT/Wtmu1}",
	"{sDrawTriCMD}","{sBeginTriCMD}","reserved2a8",	"reserved2ac",
	"reserved2b0",	"reserved2b4",	"reserved2b8",	"reserved2bc",
	/* 0x2c0 */
	"{bltSrcBaseAddr}","{bltDstBaseAddr}","{bltXYStrides}","{bltSrcChromaRange}",
	"{bltDstChromaRange}","{bltClipX}","{bltClipY}","reserved2dc",
	"{bltSrcXY}",	"{bltDstXY}",	"{bltSize}",	"{bltRop}",
	"{bltColor}",	"reserved2f4",	"{bltCommand}",	"{bltData}",
	/* 0x300 */
	"textureMode",	"tLOD",			"tDetail",		"texBaseAddr",
	"texBaseAddr_1","texBaseAddr_2","texBaseAddr_3_8","trexInit0",
	"trexInit1",	"nccTable0.0",	"nccTable0.1",	"nccTable0.2",
	"nccTable0.3",	"nccTable0.4",	"nccTable0.5",	"nccTable0.6",
	/* 0x340 */
	"nccTable0.7",	"nccTable0.8",	"nccTable0.9",	"nccTable0.A",
	"nccTable0.B",	"nccTable1.0",	"nccTable1.1",	"nccTable1.2",
	"nccTable1.3",	"nccTable1.4",	"nccTable1.5",	"nccTable1.6",
	"nccTable1.7",	"nccTable1.8",	"nccTable1.9",	"nccTable1.A",
	/* 0x380 */
	"nccTable1.B"
};



/*************************************
 *
 *	Dither helper
 *
 *************************************/

static INLINE void dither_to_matrix(UINT32 color, UINT16 *matrix)
{
	UINT32 rawr = (((color >> 16) & 0xff) * 0xf8*2) / 0xff;
	UINT32 rawg = (((color >> 8) & 0xff) * 0xfc*2) / 0xff;
	UINT32 rawb = ((color & 0xff) * 0xf8*2) / 0xff;
	int i;
	
	for (i = 0; i < 16; i++)
	{
		UINT32 dither = fbz_dither_matrix[i];
		UINT32 newr = rawr + dither;
		UINT32 newg = rawg + dither/2;
		UINT32 newb = rawb + dither;
		
		matrix[i] = ((newr >> 4) << 11) | ((newg >> 3) << 5) | (newb >> 4);
	}
}



/*************************************
 *
 *	float to custom 16-bit float converter
 *
 *************************************/

static INLINE UINT16 float_to_depth(float val)
{
	INT32 ival, ex;
	
	if (val >= 1.0)
		return 0x0000;
	if (val < 0.0)
		return 0xffff;
	
	ival = (INT32)(val * (float)(1 << 28));
	if ((ival & 0xffff000) == 0)
		return 0xfffe;
	ex = 0;
	while (!(ival & 0x8000000)) ival <<= 1, ex++;
	return (ex << 12) | ((~ival >> 15) & 0xfff);
}



/*************************************
 *
 *	VBLANK callback/buffer swapping
 *
 *************************************/
 
#if (TRACK_LOD)
static int loglod = 0;
#endif

static void swap_buffers(void)
{
	UINT16 *temp;

#if DISPLAY_SWAP_BUFFERS
static int swaps;
swaps++;
usrintf_showmessage("Swaps = %d", swaps);
#endif
	
	force_partial_update(cpu_getscanline());
	temp = frontbuf;
	frontbuf = backbuf;
	backbuf = temp;
	
#if DISPLAY_TEXTURE_MODES
{
	char tempstr[100];
	int i;
	tempstr[0] = 0;
	for (i = 0; i < 16; i++)
		if (modes_used & (1 << i))
			sprintf(&tempstr[strlen(tempstr)], "%d ", i);
	usrintf_showmessage("%s", tempstr);
	modes_used = 0;
}
#endif
	
#if DISPLAY_STATISTICS
{
	int screen_area = (Machine->visible_area.max_x - Machine->visible_area.min_x + 1) * (Machine->visible_area.max_y - Machine->visible_area.min_y + 1);
	usrintf_showmessage("Polys:%d  Render:%d%%  FPS:%d",
			polycount, pixelcount * 100 / screen_area, lastfps);
	polycount = pixelcount = 0;
	framecount++;
}
#endif

	log_cb(RETRO_LOG_DEBUG, LOGPRE "---- swapbuffers\n");

	if (pending_fastfill)
	{
		UINT32 temp_zaColor = voodoo_regs[zaColor];
		UINT32 temp_color1 = voodoo_regs[color1];
		voodoo_regs[zaColor] = pending_fastfill_zaColor;
		voodoo_regs[color1] = pending_fastfill_color1;
		fastfill();
		voodoo_regs[zaColor] = temp_zaColor;
		voodoo_regs[color1] = temp_color1;
		pending_fastfill = 0;
	}

	cpu_trigger(13579);
	
#if (TRACK_LOD)
	loglod = keyboard_pressed(KEYCODE_L);
#endif
}


static void vblank_callback(int scanline)
{
	vblank_count++;
	
	log_cb(RETRO_LOG_DEBUG, LOGPRE "---- vblank\n");

	/* any pending swapbuffers */
	if (swaps_pending && vblank_count > vblanks_before_swap)
	{
		vblank_count = 0;
		swaps_pending--;
		swap_buffers();

		if (blocked_on_swap)
			blocked_on_swap = 0;
	}

	/* reset for next time */
	timer_adjust(vblank_timer, cpu_getscanlinetime(scanline), scanline, 0);
}



/*************************************
 *
 *	Video start
 *
 *************************************/

int voodoo_start_common(void)
{
	int i, j;

	fvoodoo_regs = (float *)voodoo_regs;
	
	/* allocate memory for the pen, dither, and depth lookups */
	pen_lookup = auto_malloc(sizeof(pen_lookup[0]) * 65536);
	dither_lookup = auto_malloc(sizeof(dither_lookup[0]) * 16*256);
	depth_lookup = auto_malloc(sizeof(depth_lookup[0]) * 65536);
	lod_lookup = auto_malloc(sizeof(lod_lookup[0]) * 65536);
	if (!pen_lookup || !dither_lookup || !depth_lookup)
		return 1;
	
	/* allocate memory for the frame a depth buffers */
	framebuf[0] = auto_malloc(sizeof(UINT16) * FRAMEBUF_WIDTH * FRAMEBUF_HEIGHT);
	framebuf[1] = auto_malloc(sizeof(UINT16) * FRAMEBUF_WIDTH * FRAMEBUF_HEIGHT);
	depthbuf = auto_malloc(sizeof(UINT16) * FRAMEBUF_WIDTH * FRAMEBUF_HEIGHT);
	if (!framebuf[0] || !framebuf[1] || !depthbuf)
		return 1;
	
	/* allocate memory for the cmdfifo */
	if (voodoo2)
	{
		cmdfifo = auto_malloc(CMDFIFO_SIZE);
		if (!cmdfifo)
			return 1;
	}
	
	/* allocate memory for texture RAM */
	for (i = 0; i < tmus; i++)
	{
		textureram[i] = auto_malloc(texram_mask + 1 + 65536);
		if (!textureram[i])
			return 1;
	}
	
	/* allocate memory for the lookup tables */
	for (j = 0; j < tmus; j++)
		for (i = 0; i < 16; i++)
		{
			texel_lookup[j][i] = auto_malloc(sizeof(UINT32) * ((i < 8) ? 256 : 65536));
			if (!texel_lookup[j][i])
				return 1;
		}
	
	/* initialize dithering tables */
	for (i = 0; i < 16; i++)
		for (j = 0; j < 256; j++)
		{
			UINT32 newval = (j * 0xf8*2) / 0xff + i;
			dither_lookup[i * 256 + j] = newval / 2;
		}
	
	/* initialize depth tables */
	for (i = 0; i < 65536; i++)
		((UINT32 *)depth_lookup)[i] = (i << 11) + (127 << 23);
	
	/* initialize LOD tables */
	for (i = 0; i < 65536; i++)
	{
		int val = (int)(4.0 * (log((float)(i / 256.0)) / log(2.0)));
		lod_lookup[i] = (val < 0) ? 0 : val;
	}
	
	/* init the palette */
	for (i = 1; i < 65535; i++)
	{
		int r = (i >> 11) & 0x1f;
		int g = (i >> 5) & 0x3f;
		int b = (i) & 0x1f;
		r = (r << 3) | (r >> 2);
		g = (g << 2) | (g >> 4);
		b = (b << 3) | (b >> 2);
		palette_set_color(i - 1, r, g, b);
		pen_lookup[i] = i - 1;
	}
	pen_lookup[0] = Machine->uifont->colortable[0];
	pen_lookup[65535] = Machine->uifont->colortable[1];
	
	/* allocate a vblank timer */
	vblank_timer = timer_alloc(vblank_callback);
	
	voodoo_reset();
	return 0;
}


static void reset_buffers(void)
{
	/* VRAM and various buffers */
	frontbuf = framebuf[0];
	backbuf = framebuf[1];
	buffer_access[0] = &frontbuf;
	buffer_access[1] = &backbuf;
	buffer_access[2] = &depthbuf;

	fbz_draw_buffer = &frontbuf;
	lfb_write_buffer = &frontbuf;
	lfb_read_buffer = &frontbuf;
}


void voodoo_reset(void)
{
	reset_buffers();

	/* color DAC fake registers */
	memset(dac_regs, 0, sizeof(dac_regs));
	init_enable = 0;

	/* initialize lookup tables */
	memset(texel_lookup_dirty, 1, sizeof(texel_lookup_dirty));
	memset(ncc_y, 0, sizeof(ncc_y));
	memset(ncc_ir, 0, sizeof(ncc_ir));
	memset(ncc_ig, 0, sizeof(ncc_ig));
	memset(ncc_ib, 0, sizeof(ncc_ib));
	memset(ncc_qr, 0, sizeof(ncc_qr));
	memset(ncc_qg, 0, sizeof(ncc_qg));
	memset(ncc_qb, 0, sizeof(ncc_qb));

	/* fog tables */
	memset(fog_blend, 0, sizeof(fog_blend));
	memset(fog_delta, 0, sizeof(fog_delta));
	
	/* VBLANK and swapping */
	vblank_count = 0;
	swaps_pending = 0;
	vblanks_before_swap = 0;
	blocked_on_swap = 0;
	pending_fastfill = 0;
	
	/* fbzMode variables */
	fbz_cliprect = &fbz_noclip;
	fbz_noclip.min_x = fbz_noclip.min_y = 0;
	fbz_noclip.max_x = fbz_noclip.max_y = (1024 << 4) - 1;
	fbz_clip = fbz_noclip;
	fbz_chroma_key = 0;
	fbz_stipple_mask = 0;
	fbz_wbuffer_select = 0;
	fbz_depth_buffering = 0;
	fbz_depth_buffer_func = 0;
	fbz_dithering = 0;
	fbz_rgb_write = 0;
	fbz_depth_write = 0;
	fbz_dither_matrix = dither_matrix_4x4;
	fbz_draw_buffer = &frontbuf;
	fbz_invert_y = 0;

	/* lfbMode variables */
	lfb_write_format = 0;
	lfb_write_buffer = &frontbuf;
	lfb_read_buffer = &frontbuf;
	lfb_flipy = 0;

	/* videoDimensions variables */
	video_width = Machine->visible_area.max_x + 1;
	video_height = Machine->visible_area.max_y + 1;

	/* fbiInit variables */
	triple_buffer = 0;
	inverted_yorigin = 0;

	/* textureMode variables */
	
	/* triangle setup */
	setup_count = 0;
}


VIDEO_START( voodoo_1x4mb )
{
	tmus = 1;
	voodoo2 = 0;
	texram_mask = 4 * 1024 * 1024 - 1;
	return voodoo_start_common();
}


VIDEO_START( voodoo_2x4mb )
{
	tmus = 2;
	voodoo2 = 0;
	texram_mask = 4 * 1024 * 1024 - 1;
	return voodoo_start_common();
}


VIDEO_START( voodoo2_1x4mb )
{
	tmus = 1;
	voodoo2 = 1;
	texram_mask = 4 * 1024 * 1024 - 1;
	return voodoo_start_common();
}


VIDEO_START( voodoo2_2x4mb )
{
	tmus = 2;
	voodoo2 = 1;
	texram_mask = 4 * 1024 * 1024 - 1;
	return voodoo_start_common();
}


VIDEO_STOP( voodoo )
{
#if LOG_RENDERERS
	int i;

	for (i = 0; i < renderer_listcount; i++)
		printf("%08X%08X: %08X %08X %08X %08X %08X %08X %08X\n",
			renderer_list[i][1],
			renderer_list[i][0],
			renderer_list[i][2],
			renderer_list[i][3],
			renderer_list[i][4],
			renderer_list[i][5],
			renderer_list[i][6],
			renderer_list[i][7],
			renderer_list[i][8]);
#endif
}



/*************************************
 *
 *	Video update
 *
 *************************************/

VIDEO_UPDATE( voodoo )
{
	int x, y;

#if DISPLAY_STATISTICS
	totalframes++;
	if (totalframes == (int)Machine->drv->frames_per_second)
	{
		lastfps = framecount;
		framecount = totalframes = 0;
	}
#endif

	log_cb(RETRO_LOG_DEBUG, LOGPRE "--- video update (%d-%d) ---\n", cliprect->min_y, cliprect->max_y);

#if (DISPLAY_DEPTHBUF)
	if (keyboard_pressed(KEYCODE_D))
	{
		for (y = cliprect->min_y; y <= cliprect->max_y; y++)
		{
			UINT16 *dest = (UINT16 *)bitmap->line[y];
			UINT16 *source = &depthbuf[1024 * y];
			for (x = cliprect->min_x; x <= cliprect->max_x; x++)
				*dest++ = pen_lookup[~*source++ & 0xf800];
		}
		return;
	}
#endif

	for (y = cliprect->min_y; y <= cliprect->max_y; y++)
	{
		UINT16 *dest = (UINT16 *)bitmap->line[y];
		UINT16 *source = &frontbuf[1024 * y];
		for (x = cliprect->min_x; x <= cliprect->max_x; x++)
			*dest++ = pen_lookup[*source++];
	}
}



/*************************************
 *
 *	Special PCI I/O
 *
 *************************************/

void voodoo_set_init_enable(data32_t newval)
{
	init_enable = newval;
}



/*************************************
 *
 *	RAMDAC accesses
 *
 *************************************/

static UINT8 ramdac_r(UINT8 reg)
{
	if (reg == 5)
	{
		/* this is just to make startup happy */
		switch (dac_regs[7])
		{
			case 0x01:	return 0x55;
			case 0x07:	return 0x71;
			case 0x0b:	return 0x79;
		}
	}
	return dac_regs[reg];
}


static void ramdac_w(UINT8 reg, UINT8 data)
{
	dac_regs[reg] = data;
}



/*************************************
 *
 *	Fast filler
 *
 *************************************/

static void fastfill(void)
{
	int sx = (voodoo_regs[clipLeftRight] >> 16) & 0x3ff;
	int ex = (voodoo_regs[clipLeftRight] >> 0) & 0x3ff;
	int sy = (voodoo_regs[clipLowYHighY] >> 16) & 0x3ff;
	int ey = (voodoo_regs[clipLowYHighY] >> 0) & 0x3ff;
	UINT16 *buffer = *fbz_draw_buffer;
	UINT16 dither[16];
	int x, y;

	/* frame buffer clear? */
	if (fbz_rgb_write)
	{
		/* determine dithering */
		dither_to_matrix(voodoo_regs[color1], dither);
	
		/* loop over y */
		for (y = sy; y < ey; y++)
		{
			UINT16 *dest = &buffer[(fbz_invert_y ? (inverted_yorigin - y) : y) * FRAMEBUF_WIDTH + sx];

			/* if not dithered, it's easy */
			if (!fbz_dithering)
			{
				UINT16 color = dither[0];
				for (x = sx; x < ex; x++)
					*dest++ = color;
			}
			
			/* dithered is a little trickier */
			else
			{
				UINT16 *dbase = dither + 4 * (y & 3);
				for (x = sx; x < ex; x++)
					*dest++ = dbase[x & 3];
			}
		}
	}
	
	/* depth buffer clear? */
	if (fbz_depth_write)
	{
		UINT16 color = voodoo_regs[zaColor];
		log_cb(RETRO_LOG_DEBUG, LOGPRE "FASTFILL depth = %04X\n", color);

		/* loop over y */
		for (y = sy; y < ey; y++)
		{
			UINT16 *dest = &depthbuf[y * FRAMEBUF_WIDTH + sx];
			for (x = sx; x < ex; x++)
				*dest++ = color;
		}
	}
}



/*************************************
 *
 *	Triangle renderer
 *
 *************************************/

#if LOG_RENDERERS
static void log_renderer(int pixels)
{
	UINT32 tmode1 = 0, tmode2 = 0;
	int i;
	
	if ((voodoo_regs[textureMode + 0x100] & 0x201000) != 0x201000)
	{
		tmode1 = voodoo_regs[textureMode + 0x200] & TEXTUREMODE0_MASK;
		if ((voodoo_regs[textureMode + 0x200] & 0x201000) != 0x201000)
			tmode2 = voodoo_regs[textureMode + 0x300] & TEXTUREMODE0_MASK;
	}

	for (i = 0; i < renderer_listcount; i++)
		if ((voodoo_regs[fbzColorPath]        & FBZCOLORPATH_MASK) == renderer_list[i][2] &&
			(voodoo_regs[fogMode]             & FOGMODE_MASK     ) == renderer_list[i][3] &&
			(voodoo_regs[alphaMode]           & ALPHAMODE_MASK   ) == renderer_list[i][4] &&
			(voodoo_regs[fbzMode]             & FBZMODE_MASK     ) == renderer_list[i][5] &&
			(voodoo_regs[textureMode + 0x100] & TEXTUREMODE0_MASK) == renderer_list[i][6] &&
			(tmode1                           & TEXTUREMODE0_MASK) == renderer_list[i][7] &&
			(tmode2                           & TEXTUREMODE0_MASK) == renderer_list[i][8])
		{
			UINT32 old = renderer_list[i][0];
			renderer_list[i][0] += pixels;
			if (renderer_list[i][0] < old)
				renderer_list[i][1]++;
			return;
		}

	renderer_list[renderer_listcount][0] = pixels;
	renderer_list[renderer_listcount][2] = voodoo_regs[fbzColorPath]        & FBZCOLORPATH_MASK;
	renderer_list[renderer_listcount][3] = voodoo_regs[fogMode]             & FOGMODE_MASK;
	renderer_list[renderer_listcount][4] = voodoo_regs[alphaMode]           & ALPHAMODE_MASK;
	renderer_list[renderer_listcount][5] = voodoo_regs[fbzMode]             & FBZMODE_MASK;
	renderer_list[renderer_listcount][6] = voodoo_regs[textureMode + 0x100] & TEXTUREMODE0_MASK;
	renderer_list[renderer_listcount][7] = tmode1                           & TEXTUREMODE0_MASK;
	renderer_list[renderer_listcount][8] = tmode2                           & TEXTUREMODE0_MASK;
	renderer_listcount++;
}
#else
#define log_renderer(a)
#endif


static void draw_triangle(void)
{
	int totalpix = voodoo_regs[fbiPixelsIn];
	UINT32 temp;
	
	voodoo_regs[fbiTrianglesOut] = (voodoo_regs[fbiTrianglesOut] + 1) & 0xffffff;
	
	if (LOG_COMMANDS)
		log_cb(RETRO_LOG_DEBUG, LOGPRE "%06X:FLOAT TRIANGLE command\n", activecpu_get_pc());

	SETUP_FPU();
	temp = voodoo_regs[fbzColorPath] & FBZCOLORPATH_MASK;
	if (temp == 0x0c000035)
	{
		temp = voodoo_regs[alphaMode] & ALPHAMODE_MASK;
		if (temp == 0x00045119)
		{
			temp = voodoo_regs[fbzMode] & FBZMODE_MASK;
			if (temp == 0x000b4779)
			{
				temp = voodoo_regs[0x100 + textureMode] & TEXTUREMODE0_MASK;
				if (temp == 0x0824101f)
					{ render_0c000035_00045119_000b4779_0824101f();	goto done; }	/* wg3dh */
				else if (temp == 0x0824109f)
					{ render_0c000035_00045119_000b4779_0824109f();	goto done; }	/* wg3dh */
				else if (temp == 0x082410df)
					{ render_0c000035_00045119_000b4779_082410df();	goto done; }	/* wg3dh */
				else if (temp == 0x082418df)
					{ render_0c000035_00045119_000b4779_082418df();	goto done; }	/* wg3dh */
			}
		}
		else if (temp == 0x00040400)
		{
			temp = voodoo_regs[fbzMode] & FBZMODE_MASK;
			if (temp == 0x000b4739)
			{
				temp = voodoo_regs[0x100 + textureMode] & TEXTUREMODE0_MASK;
				if (temp == 0x0c26180f)
					{ render_0c000035_00040400_000b4739_0c26180f();	goto done; }	/* blitz99 */
			}
		}
		else if (temp == 0x64040409)
		{
			temp = voodoo_regs[fbzMode] & FBZMODE_MASK;
			if (temp == 0x000b4739)
			{
				temp = voodoo_regs[0x100 + textureMode] & TEXTUREMODE0_MASK;
				if (temp == 0x0c26180f)
					{ render_0c000035_64040409_000b4739_0c26180f();	goto done; }	/* blitz99 */
			}
		}
	}
	else if (temp == 0x0c002c35)
	{
		temp = voodoo_regs[alphaMode] & ALPHAMODE_MASK;
		if (temp == 0x64515119)
		{
			temp = voodoo_regs[fbzMode] & FBZMODE_MASK;
			if (temp == 0x000b4799)
			{
				temp = voodoo_regs[0x100 + textureMode] & TEXTUREMODE0_MASK;
				if (temp == 0x0c26180f)
					{ render_0c002c35_64515119_000b4799_0c26180f();	goto done; }	/* blitz99 */
			}
		}
		else if (temp == 0x40515119)
		{
			temp = voodoo_regs[fbzMode] & FBZMODE_MASK;
			if (temp == 0x000b4739)
			{
				temp = voodoo_regs[0x100 + textureMode] & TEXTUREMODE0_MASK;
				if (temp == 0x0c26180f)
					{ render_0c002c35_40515119_000b4739_0c26180f();	goto done; }	/* blitz99 */
			}
		}
	}
	else if (temp == 0x0c582c35)
	{
		temp = voodoo_regs[alphaMode] & ALPHAMODE_MASK;
		if (temp == 0x00515110)
		{
			temp = voodoo_regs[fbzMode] & FBZMODE_MASK;
			if (temp == 0x000b4739)
			{
				temp = voodoo_regs[0x100 + textureMode] & TEXTUREMODE0_MASK;
				if (temp == 0x0c26180f)
					{ render_0c582c35_00515110_000b4739_0c26180f();	goto done; }	/* blitz99 */
				else if (temp == 0x0c2618cf)
					{ render_0c582c35_00515110_000b4739_0c2618cf();	goto done; }	/* blitz99 */
			}
		}
	}
	else if (temp == 0x0c600c09)
	{
		temp = voodoo_regs[alphaMode] & ALPHAMODE_MASK;
		if (temp == 0x00045119)
		{
			temp = voodoo_regs[fbzMode] & FBZMODE_MASK;
			if (temp == 0x000b4779)
			{
				temp = voodoo_regs[0x100 + textureMode] & TEXTUREMODE0_MASK;
				if (temp == 0x0824100f)
					{ render_0c600c09_00045119_000b4779_0824100f();	goto done; }	/* mace */
				else if (temp == 0x0824180f)
					{ render_0c600c09_00045119_000b4779_0824180f();	goto done; }	/* mace */
				else if (temp == 0x082418cf)
					{ render_0c600c09_00045119_000b4779_082418cf();	goto done; }	/* mace */
			}
		}
	}
	else if (temp == 0x0c480035)
	{
		temp = voodoo_regs[alphaMode] & ALPHAMODE_MASK;
		if (temp == 0x00045119)
		{
			temp = voodoo_regs[fbzMode] & FBZMODE_MASK;
			if (temp == 0x000b4779)
			{
				temp = voodoo_regs[0x100 + textureMode] & TEXTUREMODE0_MASK;
				if (temp == 0x082418df)
					{ render_0c480035_00045119_000b4779_082418df();	goto done; }	/* mace */
			}
			else if (temp == 0x000b4379)
			{
				temp = voodoo_regs[0x100 + textureMode] & TEXTUREMODE0_MASK;
				if (temp == 0x082418df)
					{ render_0c480035_00045119_000b4379_082418df();	goto done; }	/* mace */
			}
		}
	}
	if (tmus == 1)
		generic_render_1tmu();
	else
		generic_render_2tmu();

done:
	RESTORE_FPU();
	totalpix = voodoo_regs[fbiPixelsIn] - totalpix;
	if (totalpix < 0) totalpix += 0x1000000;
	log_renderer(totalpix);
#if DISPLAY_STATISTICS
	polycount++;
#endif
}


static void setup_and_draw_triangle(void)
{
	float dx1, dy1, dx2, dy2;
	float divisor;

	/* grab the X/Ys at least */
	tri_va.x = setup_verts[0].x;
	tri_va.y = setup_verts[0].y;
	tri_vb.x = setup_verts[1].x;
	tri_vb.y = setup_verts[1].y;
	tri_vc.x = setup_verts[2].x;
	tri_vc.y = setup_verts[2].y;
	
	/* compute the divisor */
	divisor = 1.0f / ((tri_va.x - tri_vb.x) * (tri_va.y - tri_vc.y) - (tri_va.x - tri_vc.x) * (tri_va.y - tri_vb.y));

	/* backface culling */
	if (voodoo_regs[sSetupMode] & 0x20000)
	{
		int culling_sign = (voodoo_regs[sSetupMode] >> 18) & 1;
		int divisor_sign = (divisor < 0);
		
		/* if doing strips and ping pong is enabled, apply the ping pong */
		if ((voodoo_regs[sSetupMode] & 0x90000) == 0x00000)
			culling_sign ^= (setup_count - 3) & 1;
		
		/* if our sign matches the culling sign, we're done for */
		if (divisor_sign == culling_sign)
			return;
	}

	/* compute the dx/dy values */
	dx1 = tri_va.y - tri_vc.y;
	dx2 = tri_va.y - tri_vb.y;
	dy1 = tri_va.x - tri_vb.x;
	dy2 = tri_va.x - tri_vc.x;

	/* set up appropriate bits */
	if (voodoo_regs[sSetupMode] & 0x0001)
	{
		tri_startr = (INT32)(setup_verts[0].r * 65536.0);
		tri_drdx = (INT32)(((setup_verts[0].r - setup_verts[1].r) * dx1 - (setup_verts[0].r - setup_verts[2].r) * dx2) * divisor * 65536.0);
		tri_drdy = (INT32)(((setup_verts[0].r - setup_verts[2].r) * dy1 - (setup_verts[0].r - setup_verts[1].r) * dy2) * divisor * 65536.0);
		tri_startg = (INT32)(setup_verts[0].g * 65536.0);
		tri_dgdx = (INT32)(((setup_verts[0].g - setup_verts[1].g) * dx1 - (setup_verts[0].g - setup_verts[2].g) * dx2) * divisor * 65536.0);
		tri_dgdy = (INT32)(((setup_verts[0].g - setup_verts[2].g) * dy1 - (setup_verts[0].g - setup_verts[1].g) * dy2) * divisor * 65536.0);
		tri_startb = (INT32)(setup_verts[0].b * 65536.0);
		tri_dbdx = (INT32)(((setup_verts[0].b - setup_verts[1].b) * dx1 - (setup_verts[0].b - setup_verts[2].b) * dx2) * divisor * 65536.0);
		tri_dbdy = (INT32)(((setup_verts[0].b - setup_verts[2].b) * dy1 - (setup_verts[0].b - setup_verts[1].b) * dy2) * divisor * 65536.0);
	}
	if (voodoo_regs[sSetupMode] & 0x0002)
	{
		tri_starta = (INT32)(setup_verts[0].a * 65536.0);
		tri_dadx = (INT32)(((setup_verts[0].a - setup_verts[1].a) * dx1 - (setup_verts[0].a - setup_verts[2].a) * dx2) * divisor * 65536.0);
		tri_dady = (INT32)(((setup_verts[0].a - setup_verts[2].a) * dy1 - (setup_verts[0].a - setup_verts[1].a) * dy2) * divisor * 65536.0);
	}
	if (voodoo_regs[sSetupMode] & 0x0004)
	{
		tri_startz = (INT32)(setup_verts[0].z * 4096.0);
		tri_dzdx = (INT32)(((setup_verts[0].z - setup_verts[1].z) * dx1 - (setup_verts[0].z - setup_verts[2].z) * dx2) * divisor * 4096.0);
		tri_dzdy = (INT32)(((setup_verts[0].z - setup_verts[2].z) * dy1 - (setup_verts[0].z - setup_verts[1].z) * dy2) * divisor * 4096.0);
	}
	if (voodoo_regs[sSetupMode] & 0x0008)
	{
		tri_startw = tri_startw0 = tri_startw1 = setup_verts[0].wb;
		tri_dwdx = tri_dw0dx = tri_dw1dx = ((setup_verts[0].wb - setup_verts[1].wb) * dx1 - (setup_verts[0].wb - setup_verts[2].wb) * dx2) * divisor;
		tri_dwdy = tri_dw0dy = tri_dw1dy = ((setup_verts[0].wb - setup_verts[2].wb) * dy1 - (setup_verts[0].wb - setup_verts[1].wb) * dy2) * divisor;
	}
	if (voodoo_regs[sSetupMode] & 0x0010)
	{
		tri_startw0 = tri_startw1 = setup_verts[0].w0;
		tri_dw0dx = tri_dw1dx = ((setup_verts[0].w0 - setup_verts[1].w0) * dx1 - (setup_verts[0].w0 - setup_verts[2].w0) * dx2) * divisor;
		tri_dw0dy = tri_dw1dy = ((setup_verts[0].w0 - setup_verts[2].w0) * dy1 - (setup_verts[0].w0 - setup_verts[1].w0) * dy2) * divisor;
	}
	if (voodoo_regs[sSetupMode] & 0x0020)
	{
		tri_starts0 = tri_starts1 = setup_verts[0].s0;
		tri_ds0dx = tri_ds1dx = ((setup_verts[0].s0 - setup_verts[1].s0) * dx1 - (setup_verts[0].s0 - setup_verts[2].s0) * dx2) * divisor;
		tri_ds0dy = tri_ds1dy = ((setup_verts[0].s0 - setup_verts[2].s0) * dy1 - (setup_verts[0].s0 - setup_verts[1].s0) * dy2) * divisor;
		tri_startt0 = tri_startt1 = setup_verts[0].t0;
		tri_dt0dx = tri_dt1dx = ((setup_verts[0].t0 - setup_verts[1].t0) * dx1 - (setup_verts[0].t0 - setup_verts[2].t0) * dx2) * divisor;
		tri_dt0dy = tri_dt1dy = ((setup_verts[0].t0 - setup_verts[2].t0) * dy1 - (setup_verts[0].t0 - setup_verts[1].t0) * dy2) * divisor;
	}
	if (voodoo_regs[sSetupMode] & 0x0040)
	{
		tri_startw1 = setup_verts[0].w1;
		tri_dw1dx = ((setup_verts[0].w1 - setup_verts[1].w1) * dx1 - (setup_verts[0].w1 - setup_verts[2].w1) * dx2) * divisor;
		tri_dw1dy = ((setup_verts[0].w1 - setup_verts[2].w1) * dy1 - (setup_verts[0].w1 - setup_verts[1].w1) * dy2) * divisor;
	}
	if (voodoo_regs[sSetupMode] & 0x0080)
	{
		tri_starts1 = setup_verts[0].s1;
		tri_ds1dx = ((setup_verts[0].s1 - setup_verts[1].s1) * dx1 - (setup_verts[0].s1 - setup_verts[2].s1) * dx2) * divisor;
		tri_ds1dy = ((setup_verts[0].s1 - setup_verts[2].s1) * dy1 - (setup_verts[0].s1 - setup_verts[1].s1) * dy2) * divisor;
		tri_startt1 = setup_verts[0].t1;
		tri_dt1dx = ((setup_verts[0].t1 - setup_verts[1].t1) * dx1 - (setup_verts[0].t1 - setup_verts[2].t1) * dx2) * divisor;
		tri_dt1dy = ((setup_verts[0].t1 - setup_verts[2].t1) * dy1 - (setup_verts[0].t1 - setup_verts[1].t1) * dy2) * divisor;
	}

	/* draw the triangle */
	draw_triangle();
}



/*************************************
 *
 *	MMIO register writes
 *
 *************************************/

static const UINT8 register_alias_map[0x40] =
{
	status,		0x004/4,	vertexAx,	vertexAy,
	vertexBx,	vertexBy,	vertexCx,	vertexCy,
	startR,		dRdX,		dRdY,		startG,
	dGdX,		dGdY,		startB,		dBdX,
	dBdY,		startZ,		dZdX,		dZdY,
	startA,		dAdX,		dAdY,		startS,
	dSdX,		dSdY,		startT,		dTdX,
	dTdY,		startW,		dWdX,		dWdY,
	
	triangleCMD,0x084/4,	fvertexAx,	fvertexAy,
	fvertexBx,	fvertexBy,	fvertexCx,	fvertexCy,
	fstartR,	fdRdX,		fdRdY,		fstartG,
	fdGdX,		fdGdY,		fstartB,	fdBdX,
	fdBdY,		fstartZ,	fdZdX,		fdZdY,
	fstartA,	fdAdX,		fdAdY,		fstartS,
	fdSdX,		fdSdY,		fstartT,	fdTdX,
	fdTdY,		fstartW,	fdWdX,		fdWdY
};


static int compute_expected_depth(void)
{
	UINT32 command = cmdfifo[voodoo_regs[cmdFifoRdPtr]/4];
	int i, count = 0;
	
	switch (command & 7)
	{
		/* packet type 0 */
		case 0:
			if (((command >> 3) & 7) == 4)
				return 2;
			return 1;
		
		/* packet type 1 */
		case 1:
			return 1 + (command >> 16);
		
		/* packet type 2 */
		case 2:
			for (i = 3; i <= 31; i++)
				if (command & (1 << i)) count++;
			return 1 + count;
		
		/* packet type 3 */
		case 3:
			count = 2;		/* X/Y */
			if (command & 0x10000000)
			{
				if (command & 0xc00) count++;		/* ARGB */
			}
			else
			{
				if (command & 0x400) count += 3;	/* RGB */
				if (command & 0x800) count++;		/* A */
			}
			if (command & 0x1000) count++;			/* Z */
			if (command & 0x2000) count++;			/* Wb */
			if (command & 0x4000) count++;			/* W0 */
			if (command & 0x8000) count += 2;		/* S0/T0 */
			if (command & 0x10000) count++;			/* W1 */
			if (command & 0x20000) count += 2;		/* S1/T1 */
			count *= (command >> 6) & 15;			/* numverts */
/*			if (command & 0xfc00000)				// smode != 0 */
/*				count++;*/
			return 1 + count + (command >> 29);
		
		/* packet type 4 */
		case 4:
			for (i = 15; i <= 28; i++)
				if (command & (1 << i)) count++;
			return 1 + count + (command >> 29);
		
		/* packet type 5 */
		case 5:
			return 2 + ((command >> 3) & 0x7ffff);
		
		default:
			log_cb(RETRO_LOG_DEBUG, LOGPRE "UNKNOWN PACKET TYPE %d\n", command & 7);
			return 1;
	}
	return 1;
}


static UINT32 execute_cmdfifo(void)
{
	UINT32 *src = &cmdfifo[voodoo_regs[cmdFifoRdPtr]/4];
	UINT32 command = *src++;
	int count, inc, code, i;
	offs_t target;

	switch (command & 7)
	{
		/* packet type 0 */
		case 0:
			target = (command >> 4) & 0x1fffffc;
			switch ((command >> 3) & 7)
			{
				case 0:		/* NOP */
					if (LOG_CMDFIFO) log_cb(RETRO_LOG_DEBUG, LOGPRE "  NOP\n");
					break;
				
				case 1:		/* JSR */
					if (LOG_CMDFIFO) log_cb(RETRO_LOG_DEBUG, LOGPRE "  JSR $%06X\n", target);
					voodoo_regs[cmdFifoAMin] = voodoo_regs[cmdFifoAMax] = target - 4;
					return target;
				
				case 2:		/* RET */
					if (LOG_CMDFIFO) log_cb(RETRO_LOG_DEBUG, LOGPRE "  RET $%06X\n", target);
					break;
				
				case 3:		/* JMP LOCAL FRAME BUFFER */
					if (LOG_CMDFIFO) log_cb(RETRO_LOG_DEBUG, LOGPRE "  JMP LOCAL FRAMEBUF $%06X\n", target);
					voodoo_regs[cmdFifoAMin] = voodoo_regs[cmdFifoAMax] = target - 4;
					return target;
				
				case 4:		/* JMP AGP */
					if (LOG_CMDFIFO) log_cb(RETRO_LOG_DEBUG, LOGPRE "  JMP AGP $%06X\n", target);
					voodoo_regs[cmdFifoAMin] = voodoo_regs[cmdFifoAMax] = target - 4;
					return target;
				
				default:
					log_cb(RETRO_LOG_DEBUG, LOGPRE "  INVALID JUMP COMMAND\n");
					break;
			}
			break;
		
		/* packet type 1 */
		case 1:
			count = command >> 16;
			inc = (command >> 15) & 1;
			target = (command >> 3) & 0xfff;
			
			if (LOG_CMDFIFO) log_cb(RETRO_LOG_DEBUG, LOGPRE "  PACKET TYPE 1: count=%d inc=%d reg=%04X\n", count, inc, target);
			for (i = 0; i < count; i++, target += inc)
				voodoo_regs_w(target, *src++, 0);
			break;
		
		/* packet type 2 */
		case 2:
			if (LOG_CMDFIFO) log_cb(RETRO_LOG_DEBUG, LOGPRE "  PACKET TYPE 2: mask=%X\n", (command >> 3) & 0x1ffffff);
			for (i = 3; i <= 31; i++)
				if (command & (1 << i))
					voodoo_regs_w(bltSrcBaseAddr + (i - 3), *src++, 0);
			break;
		
		/* packet type 3 */
		case 3:
			count = (command >> 6) & 15;
			code = (command >> 3) & 7;
			if (LOG_CMDFIFO) log_cb(RETRO_LOG_DEBUG, LOGPRE "  PACKET TYPE 3: count=%d code=%d mask=%03X\n", count, code, (command >> 10) & 0xfff);
			
			voodoo_regs[sSetupMode] = ((command >> 10) & 0xfff) | ((command >> 6) & 0xf0000);
			for (i = 0; i < count; i++)
			{
				setup_pending.x = TRUNC_TO_INT(*(float *)src++ * 16. + 0.5) * (1. / 16.);
				setup_pending.y = TRUNC_TO_INT(*(float *)src++ * 16. + 0.5) * (1. / 16.);

				if (command & 0x10000000)
				{
					if (voodoo_regs[sSetupMode] & 0x0003)
					{
						UINT32 argb = *src++;
						if (voodoo_regs[sSetupMode] & 0x0001)
						{
							setup_pending.r = (argb >> 16) & 0xff;
							setup_pending.g = (argb >> 8) & 0xff;
							setup_pending.b = argb & 0xff;
						}
						if (voodoo_regs[sSetupMode] & 0x0002)
							setup_pending.a = argb >> 24;
					}
				}
				else
				{
					if (voodoo_regs[sSetupMode] & 0x0001)
					{
						setup_pending.r = *(float *)src++;
						setup_pending.g = *(float *)src++;
						setup_pending.b = *(float *)src++;
					}
					if (voodoo_regs[sSetupMode] & 0x0002)
						setup_pending.a = *(float *)src++;
				}

				if (voodoo_regs[sSetupMode] & 0x0004)
					setup_pending.z = *(float *)src++;
				if (voodoo_regs[sSetupMode] & 0x0008)
					setup_pending.wb = *(float *)src++;
				if (voodoo_regs[sSetupMode] & 0x0010)
					setup_pending.w0 = *(float *)src++;
				if (voodoo_regs[sSetupMode] & 0x0020)
				{
					setup_pending.s0 = *(float *)src++;
					setup_pending.t0 = *(float *)src++;
				}
				if (voodoo_regs[sSetupMode] & 0x0040)
					setup_pending.w1 = *(float *)src++;
				if (voodoo_regs[sSetupMode] & 0x0080)
				{
					setup_pending.s1 = *(float *)src++;
					setup_pending.t1 = *(float *)src++;
				}

				if ((code == 1 && i == 0) || (code == 0 && i % 3 == 0))
				{
					setup_count = 1;
					setup_verts[0] = setup_verts[1] = setup_verts[2] = setup_pending;
				}
				else
				{
					if (!(voodoo_regs[sSetupMode] & 0x10000))	/* strip mode */
						setup_verts[0] = setup_verts[1];
					setup_verts[1] = setup_verts[2];
					setup_verts[2] = setup_pending;
					if (++setup_count >= 3)
						setup_and_draw_triangle();
				}
			}
			src += command >> 29;
			break;
		
		/* packet type 4 */
		case 4:
			target = (command >> 3) & 0xfff;

			if (LOG_CMDFIFO) log_cb(RETRO_LOG_DEBUG, LOGPRE "  PACKET TYPE 4: mask=%X reg=%04X pad=%d\n", (command >> 15) & 0x3fff, target, command >> 29);
			for (i = 15; i <= 28; i++)
				if (command & (1 << i))
					voodoo_regs_w(target + (i - 15), *src++, 0);
			src += command >> 29;
			break;
		
		/* packet type 5 */
		case 5:
			count = (command >> 3) & 0x7ffff;
			target = *src++ / 4;

			if ((command >> 30) == 2)
			{
				if (LOG_CMDFIFO) log_cb(RETRO_LOG_DEBUG, LOGPRE "  PACKET TYPE 5: LFB count=%d dest=%08X bd2=%X bdN=%X\n", count, target, (command >> 26) & 15, (command >> 22) & 15);
				for (i = 0; i < count; i++)
					voodoo_framebuf_w(target++, *src++, 0);
			}
			else if ((command >> 30) == 3)
			{
				if (LOG_CMDFIFO) log_cb(RETRO_LOG_DEBUG, LOGPRE "  PACKET TYPE 5: textureRAM count=%d dest=%08X bd2=%X bdN=%X\n", count, target, (command >> 26) & 15, (command >> 22) & 15);
				for (i = 0; i < count; i++)
					voodoo_textureram_w(target++, *src++, 0);
			}
			break;
		
		default:
			fprintf(stderr, "PACKET TYPE %d\n", command & 7);
			break;
	}

	/* by default just update the read pointer past all the data we consumed */
	return 4 * (src - &cmdfifo[0]);
}


WRITE32_HANDLER( voodoo2_regs_w )
{
	/* handle legacy writes */
	if (!(voodoo_regs[fbiInit7] & 0x100))
		voodoo_regs_w(offset, data, mem_mask);
	
	/* handle legacy writes that still work in FIFO mode */
	else if (!(offset & 0x80000))
	{
		if ((offset & 0x800c0) == 0x80000 && (voodoo_regs[fbiInit3] & 1))
			offset = register_alias_map[offset & 0x3f];
		else
			offset &= 0xff;
		
		/* only very particular commands go through in FIFO mode */
/*		if (offset == intrCtrl ||
			offset == backPorch ||
			offset == videoDimensions ||
			offset == dacData ||
			offset == hSync ||
			offset == vSync ||
			offset == maxRgbDelta ||
			offset == hBorder ||
			offset == vBorder ||
			offset == borderColor ||
			(offset >= cmdFifoBaseAddr && offset <= cmdFifoHoles))*/
		if (offset != swapbufferCMD)
			voodoo_regs_w(offset, data,mem_mask);
	}
	
	/* handle writes to the command FIFO */
	else
	{
		offs_t addr = ((voodoo_regs[cmdFifoBaseAddr] & 0x3ff) << 12) + ((offset & 0xffff) * 4);
		data32_t old_depth = voodoo_regs[cmdFifoDepth];
		
		/* swizzling */
		if (offset & 0x10000)
			data = (data >> 24) | ((data >> 8) & 0xff00) | ((data << 8) & 0xff0000) | (data << 24);
		cmdfifo[addr/4] = data;
		
		/* in-order, no holes */
		if (voodoo_regs[cmdFifoHoles] == 0 && addr == voodoo_regs[cmdFifoAMin] + 4)
		{
			voodoo_regs[cmdFifoAMin] = voodoo_regs[cmdFifoAMax] = addr;
			voodoo_regs[cmdFifoDepth]++;
		}
		
		/* out-of-order, but within the min-max range */
		else if (addr < voodoo_regs[cmdFifoAMax])
		{
			voodoo_regs[cmdFifoHoles]--;
			if (voodoo_regs[cmdFifoHoles] == 0)
			{
				voodoo_regs[cmdFifoDepth] += voodoo_regs[cmdFifoAMax] - voodoo_regs[cmdFifoAMin];
				voodoo_regs[cmdFifoAMin] = voodoo_regs[cmdFifoAMax];
			}
		}
		
		/* out-of-order, bumping max */
		else
		{
			voodoo_regs[cmdFifoAMax] = addr;
			voodoo_regs[cmdFifoHoles] += (voodoo_regs[cmdFifoAMax] - voodoo_regs[cmdFifoAMin]) / 4 - 1;
		}
		
		if (LOG_CMDFIFO_VERBOSE) 
		{
			if ((cmdfifo[voodoo_regs[cmdFifoRdPtr]/4] & 7) == 3)
				log_cb(RETRO_LOG_DEBUG, LOGPRE "CMDFIFO(%06X)=%f  (min=%06X max=%06X d=%d h=%d)\n", addr, *(float *)&data, voodoo_regs[cmdFifoAMin], voodoo_regs[cmdFifoAMax], voodoo_regs[cmdFifoDepth], voodoo_regs[cmdFifoHoles]);
			else if ((cmdfifo[voodoo_regs[cmdFifoRdPtr]/4] & 7) != 5)
				log_cb(RETRO_LOG_DEBUG, LOGPRE "CMDFIFO(%06X)=%08X  (min=%06X max=%06X d=%d h=%d)\n", addr, data, voodoo_regs[cmdFifoAMin], voodoo_regs[cmdFifoAMax], voodoo_regs[cmdFifoDepth], voodoo_regs[cmdFifoHoles]);
		}

		/* if we have data, process it */
		if (voodoo_regs[cmdFifoDepth])
		{
			/* if we didn't have data before, use the first word to compute the expected count */
			if (old_depth == 0)
			{
				cmdfifo_expected = compute_expected_depth();
				if (LOG_CMDFIFO_VERBOSE) log_cb(RETRO_LOG_DEBUG, LOGPRE "PACKET TYPE %d, expecting %d words\n", cmdfifo[voodoo_regs[cmdFifoRdPtr]/4] & 7, cmdfifo_expected);
			}
			
			/* if we got everything, execute */
			if (voodoo_regs[cmdFifoDepth] >= cmdfifo_expected)
			{
				voodoo_regs[cmdFifoRdPtr] = execute_cmdfifo();
				voodoo_regs[cmdFifoDepth] -= cmdfifo_expected;
			}
		}
	}
}


WRITE32_HANDLER( voodoo_regs_w )
{
	int chips = (offset >> 8) & 0x0f;
	if (chips == 0) chips = 0x0f;
	
	if ((offset & 0x800c0) == 0x80000 && (voodoo_regs[fbiInit3] & 1))
		offset = register_alias_map[offset & 0x3f];
	else
		offset &= 0xff;
	
	if (chips & 1)
		voodoo_regs[0x000 + offset] = data;
	if (chips & 2)
		voodoo_regs[0x100 + offset] = data;
	if (chips & 4)
		voodoo_regs[0x200 + offset] = data;
	if (chips & 8)
		voodoo_regs[0x300 + offset] = data;

	status_lastpc = ~0;

	if (LOG_REGISTERS)
	{
		if (offset < fvertexAx || offset > fdWdY)
			log_cb(RETRO_LOG_DEBUG, LOGPRE "%06X:voodoo %s(%d) write = %08X\n", activecpu_get_pc(), (offset < 0x384/4) ? voodoo_reg_name[offset] : "oob", chips, data);
		else
			log_cb(RETRO_LOG_DEBUG, LOGPRE "%06X:voodoo %s(%d) write = %f\n", activecpu_get_pc(), (offset < 0x384/4) ? voodoo_reg_name[offset] : "oob", chips, *(float *)&data);
	}
	
	switch (offset)
	{
		/* fixed-point vertex data */
		case vertexAx:
			if (chips & 1) tri_va.x = (float)(INT16)data * (1.0f / 16.0f);
			break;
		case vertexAy:
			if (chips & 1) tri_va.y = (float)(INT16)data * (1.0f / 16.0f);
			break;
		case vertexBx:
			if (chips & 1) tri_vb.x = (float)(INT16)data * (1.0f / 16.0f);
			break;
		case vertexBy:
			if (chips & 1) tri_vb.y = (float)(INT16)data * (1.0f / 16.0f);
			break;
		case vertexCx:
			if (chips & 1) tri_vc.x = (float)(INT16)data * (1.0f / 16.0f);
			break;
		case vertexCy:
			if (chips & 1) tri_vc.y = (float)(INT16)data * (1.0f / 16.0f);
			break;
		
		/* fixed point starting data */
		case startR:
			if (chips & 1) tri_startr = ((INT32)data << 8) >> 4;
			break;
		case startG:
			if (chips & 1) tri_startg = ((INT32)data << 8) >> 4;
			break;
		case startB:
			if (chips & 1) tri_startb = ((INT32)data << 8) >> 4;
			break;
		case startA:
			if (chips & 1) tri_starta = ((INT32)data << 8) >> 4;
			break;
		case startZ:
			if (chips & 1) tri_startz = (INT32)data;
			break;
		case startW:
			if (chips & 1) tri_startw = (float)(INT32)data * (1.0 / (float)(1 << 30));
			if (chips & 2) tri_startw0 = (float)(INT32)data * (1.0 / (float)(1 << 30));
			if (chips & 4) tri_startw1 = (float)(INT32)data * (1.0 / (float)(1 << 30));
			break;
		case startS:
			if (chips & 2) tri_starts0 = (float)(INT32)data * (1.0 / (float)(1 << 18));
			if (chips & 4) tri_starts1 = (float)(INT32)data * (1.0 / (float)(1 << 18));
			break;
		case startT:
			if (chips & 2) tri_startt0 = (float)(INT32)data * (1.0 / (float)(1 << 18));
			if (chips & 4) tri_startt1 = (float)(INT32)data * (1.0 / (float)(1 << 18));
			break;
		
		/* fixed point delta X data */
		case dRdX:
			if (chips & 1) tri_drdx = ((INT32)data << 8) >> 4;
			break;
		case dGdX:
			if (chips & 1) tri_dgdx = ((INT32)data << 8) >> 4;
			break;
		case dBdX:
			if (chips & 1) tri_dbdx = ((INT32)data << 8) >> 4;
			break;
		case dAdX:
			if (chips & 1) tri_dadx = ((INT32)data << 8) >> 4;
			break;
		case dZdX:
			if (chips & 1) tri_dzdx = (INT32)data;
			break;
		case dWdX:
			if (chips & 1) tri_dwdx = (float)(INT32)data * (1.0 / (float)(1 << 30));
			if (chips & 2) tri_dw0dx = (float)(INT32)data * (1.0 / (float)(1 << 30));
			if (chips & 4) tri_dw1dx = (float)(INT32)data * (1.0 / (float)(1 << 30));
			break;
		case dSdX:
			if (chips & 2) tri_ds0dx = (float)(INT32)data * (1.0 / (float)(1 << 18));
			if (chips & 4) tri_ds1dx = (float)(INT32)data * (1.0 / (float)(1 << 18));
			break;
		case dTdX:
			if (chips & 2) tri_dt0dx = (float)(INT32)data * (1.0 / (float)(1 << 18));
			if (chips & 4) tri_dt1dx = (float)(INT32)data * (1.0 / (float)(1 << 18));
			break;
		
		/* fixed point delta Y data */
		case dRdY:
			if (chips & 1) tri_drdy = ((INT32)data << 8) >> 4;
			break;
		case dGdY:
			if (chips & 1) tri_dgdy = ((INT32)data << 8) >> 4;
			break;
		case dBdY:
			if (chips & 1) tri_dbdy = ((INT32)data << 8) >> 4;
			break;
		case dAdY:
			if (chips & 1) tri_dady = ((INT32)data << 8) >> 4;
			break;
		case dZdY:
			if (chips & 1) tri_dzdy = (INT32)data;
			break;
		case dWdY:
			if (chips & 1) tri_dwdy = (float)(INT32)data * (1.0 / (float)(1 << 30));
			if (chips & 2) tri_dw0dy = (float)(INT32)data * (1.0 / (float)(1 << 30));
			if (chips & 4) tri_dw1dy = (float)(INT32)data * (1.0 / (float)(1 << 30));
			break;
		case dSdY:
			if (chips & 2) tri_ds0dy = (float)(INT32)data * (1.0 / (float)(1 << 18));
			if (chips & 4) tri_ds1dy = (float)(INT32)data * (1.0 / (float)(1 << 18));
			break;
		case dTdY:
			if (chips & 2) tri_dt0dy = (float)(INT32)data * (1.0 / (float)(1 << 18));
			if (chips & 4) tri_dt1dy = (float)(INT32)data * (1.0 / (float)(1 << 18));
			break;
		
		/* floating-point vertex data */
		case fvertexAx:
			if (chips & 1) tri_va.x = TRUNC_TO_INT(*(float *)&data * 16. + 0.5) * (1. / 16.);
			break;
		case fvertexAy:
			if (chips & 1) tri_va.y = TRUNC_TO_INT(*(float *)&data * 16. + 0.5) * (1. / 16.);
			break;
		case fvertexBx:
			if (chips & 1) tri_vb.x = TRUNC_TO_INT(*(float *)&data * 16. + 0.5) * (1. / 16.);
			break;
		case fvertexBy:
			if (chips & 1) tri_vb.y = TRUNC_TO_INT(*(float *)&data * 16. + 0.5) * (1. / 16.);
			break;
		case fvertexCx:
			if (chips & 1) tri_vc.x = TRUNC_TO_INT(*(float *)&data * 16. + 0.5) * (1. / 16.);
			break;
		case fvertexCy:
			if (chips & 1) tri_vc.y = TRUNC_TO_INT(*(float *)&data * 16. + 0.5) * (1. / 16.);
			break;
		
		/* floating-point starting data */
		case fstartR:
			if (chips & 1) tri_startr = (INT32)(*(float *)&data * 65536.0);
			break;
		case fstartG:
			if (chips & 1) tri_startg = (INT32)(*(float *)&data * 65536.0);
			break;
		case fstartB:
			if (chips & 1) tri_startb = (INT32)(*(float *)&data * 65536.0);
			break;
		case fstartA:
			if (chips & 1) tri_starta = (INT32)(*(float *)&data * 65536.0);
			break;
		case fstartZ:
			if (chips & 1) tri_startz = (INT32)(*(float *)&data * 4096.0);
			break;
		case fstartW:
			if (chips & 1) tri_startw = *(float *)&data;
			if (chips & 2) tri_startw0 = *(float *)&data;
			if (chips & 4) tri_startw1 = *(float *)&data;
			break;
		case fstartS:
			if (chips & 2) tri_starts0 = *(float *)&data;
			if (chips & 4) tri_starts1 = *(float *)&data;
			break;
		case fstartT:
			if (chips & 2) tri_startt0 = *(float *)&data;
			if (chips & 4) tri_startt1 = *(float *)&data;
			break;
		
		/* floating-point delta X data */
		case fdRdX:
			if (chips & 1) tri_drdx = (INT32)(*(float *)&data * 65536.0);
			break;
		case fdGdX:
			if (chips & 1) tri_dgdx = (INT32)(*(float *)&data * 65536.0);
			break;
		case fdBdX:
			if (chips & 1) tri_dbdx = (INT32)(*(float *)&data * 65536.0);
			break;
		case fdAdX:
			if (chips & 1) tri_dadx = (INT32)(*(float *)&data * 65536.0);
			break;
		case fdZdX:
			if (chips & 1) tri_dzdx = (INT32)(*(float *)&data * 4096.0);
			break;
		case fdWdX:
			if (chips & 1) tri_dwdx = *(float *)&data;
			if (chips & 2) tri_dw0dx = *(float *)&data;
			if (chips & 4) tri_dw1dx = *(float *)&data;
			break;
		case fdSdX:
			if (chips & 2) tri_ds0dx = *(float *)&data;
			if (chips & 4) tri_ds1dx = *(float *)&data;
			break;
		case fdTdX:
			if (chips & 2) tri_dt0dx = *(float *)&data;
			if (chips & 4) tri_dt1dx = *(float *)&data;
			break;
		
		/* floating-point delta Y data */
		case fdRdY:
			if (chips & 1) tri_drdy = (INT32)(*(float *)&data * 65536.0);
			break;
		case fdGdY:
			if (chips & 1) tri_dgdy = (INT32)(*(float *)&data * 65536.0);
			break;
		case fdBdY:
			if (chips & 1) tri_dbdy = (INT32)(*(float *)&data * 65536.0);
			break;
		case fdAdY:
			if (chips & 1) tri_dady = (INT32)(*(float *)&data * 65536.0);
			break;
		case fdZdY:
			if (chips & 1) tri_dzdy = (INT32)(*(float *)&data * 4096.0);
			break;
		case fdWdY:
			if (chips & 1) tri_dwdy = *(float *)&data;
			if (chips & 2) tri_dw0dy = *(float *)&data;
			if (chips & 4) tri_dw1dy = *(float *)&data;
			break;
		case fdSdY:
			if (chips & 2) tri_ds0dy = *(float *)&data;
			if (chips & 4) tri_ds1dy = *(float *)&data;
			break;
		case fdTdY:
			if (chips & 2) tri_dt0dy = *(float *)&data;
			if (chips & 4) tri_dt1dy = *(float *)&data;
			break;
		
		/* triangle setup (voodoo 2 only) */
		case sVx:
			setup_pending.x = TRUNC_TO_INT(*(float *)&data * 16. + 0.5) * (1. / 16.);
			break;
		case sVy:
			setup_pending.y = TRUNC_TO_INT(*(float *)&data * 16. + 0.5) * (1. / 16.);
			break;
		case sARGB:
			setup_pending.a = data >> 24;
			setup_pending.r = (data >> 16) & 0xff;
			setup_pending.g = (data >> 8) & 0xff;
			setup_pending.b = data & 0xff;
			break;
		case sWb:
			setup_pending.wb = *(float *)&data;
			break;
		case sWtmu0:
			setup_pending.w0 = *(float *)&data;
			break;
		case sS_W0:
			setup_pending.s0 = *(float *)&data;
			break;
		case sT_W0:
			setup_pending.t0 = *(float *)&data;
			break;
		case sWtmu1:
			setup_pending.w1 = *(float *)&data;
			break;
		case sS_Wtmu1:
			setup_pending.s1 = *(float *)&data;
			break;
		case sT_Wtmu1:
			setup_pending.t1 = *(float *)&data;
			break;
		case sAlpha:
			setup_pending.a = *(float *)&data;
			break;
		case sRed:
			setup_pending.r = *(float *)&data;
			break;
		case sGreen:
			setup_pending.g = *(float *)&data;
			break;
		case sBlue:
			setup_pending.b = *(float *)&data;
			break;
		case sDrawTriCMD:
			if (!(voodoo_regs[sSetupMode] & 0x10000))	/* strip mode */
				setup_verts[0] = setup_verts[1];
			setup_verts[1] = setup_verts[2];
			setup_verts[2] = setup_pending;
			if (++setup_count >= 3)
				setup_and_draw_triangle();
			break;
		case sBeginTriCMD:
			setup_count = 1;
			setup_verts[0] = setup_verts[1] = setup_verts[2] = setup_pending;
			break;
		
		case triangleCMD:
		case ftriangleCMD:
			draw_triangle();
			break;
		
		case fbzColorPath:
			/* bit 0-1 = RGB select (0=iteratedRGB, 1=TREX, 2=color1) */
			/* bit 2-3 = alpha select (0=iteratedA, 1=TREX, 2=color1) */
			/* bit 4 = cc_localselect mux control (0=iteratedRGB, 1=color0) */
			/* bit 5-6 = cca_localselect mux control (0=iteratedA, 1=color0, 2=iteratedZ) */
			/* bit 7 = cc_localselect_override mux control (0=cc_localselect, 1=texture alpha bit) */
			/* bit 8 = cc_zero_other mux control (0=c_other, 1=zero) */
			/* bit 9 = cc_sub_clocal mux control (0=zero, 1=c_local) */
			/* bit 10-12 = cc_mselect mux control (0=zero, 1=c_local, 2=a_other, 3=a_local, 4=texture alpha) */
			/* bit 13 = cc_reverse_blend control */
			/* bit 14 = cc_add_clocal control */
			/* bit 15 = cc_add_alocal control */
			/* bit 16 = cc_invert_output control */
			/* bit 17 = cca_zero_other mux control (0=a_other, 1=zero) */
			/* bit 18 = cca_sub_clocal mux control (0=zero, 1=a_local) */
			/* bit 19-21 = cc_mselect mux control (0=zero, 1=a_local, 2=a_other, 3=_alocal, 4=texture alpa) */
			/* bit 22 = cca_reverse_blend control */
			/* bit 23 = cca_add_clocal control */
			/* bit 24 = cca_add_alocal control */
			/* bit 25 = cca_invert_output control */
			/* bit 26 = parameter adjust (1=adjust for subpixel) */
			/* bit 27 = enable texture mapping (1=enable) */
			break;
		
		case fogMode:
			/* bit 0 = enable fog (1=enable) */
			/* bit 1 = fogadd control (0=fogColor, 1=zero) */
			/* bit 2 = fogmult control (0=ccu RGB, 1=zero) */
			/* bit 3 = fogalpha control (0=fog table, 1=iterated) */
			/* bit 4 = fogz control (0=fogalpha mux, 1=iterated z(27:20) */
			/* bit 5 = fogconstant control (0=fog multiplier output, 1=fogColor) */
			break;
	
		case alphaMode:
			/* bit 0 = enable alpha function (1=enable) */
			/* bit 1-3 = alpha function */
			/* bit 4 = enable alpha blending (1=enable) */
			/* bit 5 = enable anti-aliasing (1=enable) */
			/* bit 8-11 = source RGB alpha blending factor */
			/* bit 12-15 = dest RGB alpha blending factor */
			/* bit 16-19 = source alpha-channel alpha blending factor */
			/* bit 20-23 = dest alpha-channel alpha blending factor */
			/* bit 24-31 = alpha reference value */
			break;
	
		case fbzMode:
			/* bit 0 = enable clipping rectangle (1=enable) */
			/* bit 1 = enable chroma keying (1=enable) */
			/* bit 2 = enable stipple register masking (1=enable) */
			/* bit 3 = W-buffer select (0=use Z, 1=use W) */
			/* bit 4 = enable depth-buffering (1=enable) */
			/* bit 5-7 = depth-buffer function */
			/* bit 8 = enable dithering */
			/* bit 9 = RGB buffer write mask (0=disable writes to RGB buffer) */
			/* bit 10 = depth/alpha buffer write mask (0=disable writes) */
			/* bit 11 = dither algorithm (0=4x4 ordered, 1=2x2 ordered) */
			/* bit 12 = enable stipple pattern masking (1=enable) */
			/* bit 13 = enable alpha channel mask (1=enable) */
			/* bit 14-15 = draw buffer (0=front, 1=back) */
			/* bit 16 = enable depth-biasing (1=enable) */
			/* bit 17 = rendering commands Y origin (0=top of screen, 1=bottom) */
			/* bit 18 = enable alpha planes (1=enable) */
			/* bit 19 = enable alpha-blending dither subtraction (1=enable) */
			/* bit 20 = depth buffer compare select (0=normal, 1=zaColor) */

			/* extract parameters we can handle */
			fbz_cliprect = ((data >> 0) & 1) ? &fbz_clip : &fbz_noclip;
			fbz_chroma_key = (data >> 1) & 1;
			fbz_stipple_mask = (data >> 2) & 1;
			fbz_wbuffer_select = (data >> 3) & 1;
			fbz_depth_buffering = (data >> 4) & 1;
			fbz_depth_buffer_func = (data >> 5) & 7;
			fbz_dithering = (data >> 8) & 1;
			fbz_rgb_write = (data >> 9) & 1;
			fbz_depth_write = (data >> 10) & 1;
			fbz_dither_matrix = ((data >> 11) & 1) ? dither_matrix_2x2 : dither_matrix_4x4;
			fbz_draw_buffer = buffer_access[(data >> 14) & 3];
			fbz_invert_y = (data >> 17) & 1;
			if (!voodoo2)
				voodoo_regs[fbzMode] &= ~(1 << 21);
			break;
		
		case lfbMode:
			/* bit 0-3 = write format */
			/* bit 4-5 = write buffer select (0=front, 1=back) */
			/* bit 6-7 = read buffer select (0=front, 1=back, 2=depth/alpha) */
			/* bit 8 = enable pixel pipeline-processed writes */
			/* bit 9-10 = linear frame buffer RGBA lanes */
			/* bit 11 = 16-bit word swap LFB writes */
			/* bit 12 = byte swizzle LFB writes */
			/* bit 13 = LFB access Y origin (0=top is origin, 1=bottom) */
			/* bit 14 = LFB write access W select (0=LFB selected, 1=zacolor[15:0]) */
			/* bit 15 = 16-bit word swap LFB reads */
			/* bit 16 = byte swizzle LFB reads */

			/* extract parameters we can handle */
			lfb_write_format = data & 0x0f;
			lfb_write_buffer = buffer_access[(data >> 4) & 3];
			lfb_read_buffer = buffer_access[(data >> 6) & 3];
			lfb_flipy = (data >> 13) & 1;
			break;

		case clipLeftRight:
			fbz_clip.min_x = ((data >> 16) & 0x3ff) << 4;
			fbz_clip.max_x = ((data & 0x3ff) << 4);
			break;

		case clipLowYHighY:
			fbz_clip.min_y = ((data >> 16) & 0x3ff) << 4;
			fbz_clip.max_y = ((data & 0x3ff) << 4);
			break;
	
		case nopCMD:
			if (LOG_COMMANDS)
				log_cb(RETRO_LOG_DEBUG, LOGPRE "%06X:NOP command\n", activecpu_get_pc());
			break;
	
		case fastfillCMD:
			if (LOG_COMMANDS)
				log_cb(RETRO_LOG_DEBUG, LOGPRE "%06X:FASTFILL command\n", activecpu_get_pc());
			if (blocked_on_swap)
			{
				pending_fastfill = 1;
				pending_fastfill_zaColor = voodoo_regs[zaColor];
				pending_fastfill_color1 = voodoo_regs[color1];
			}
			else
				fastfill();
			break;
	
		case swapbufferCMD:
/*printf("%08X:swapbuffer %02X\n", activecpu_get_pc(), data);*/
			/* immediate? */
			if (!(data & 1))
				swap_buffers();
			
			/* deferred */
			else
			{
				swaps_pending++;
				vblanks_before_swap = (data >> 1) & 0x7f;
				blocked_on_swap = 1;
			}
		
			if (LOG_COMMANDS)
				log_cb(RETRO_LOG_DEBUG, LOGPRE "%06X:SWAPBUFFER command = %08X\n", activecpu_get_pc(), data);
			break;
	
		case fbiInit4:
			/* writes 0x00000001 */

			/* bit 0 = wait state cycles for PCI read accesses */
			break;
		
		case videoDimensions:
			if (data & 0x3ff)
				video_width = data & 0x3ff;
			if (data & 0x3ff0000)
				video_height = (data >> 16) & 0x3ff;
			set_visible_area(0, video_width - 1, 0, video_height - 1);
			timer_adjust(vblank_timer, cpu_getscanlinetime(video_height), video_height, 0);
			reset_buffers();
			break;
			
		case fbiInit0:
			/* writes 0x00000006 */
			/* writes 0x00000002 */
			/* writes 0x00000000 */
			/* writes 0x00000411 */
			
			/* bit 0 = VGA passtrough */
			/* bit 1 = FBI reset (1) */
			/* bit 2 = FBI FIFO reset (1) */
			/* bit 4 = Stall PCI enable for high water mark */
			/* bit 6-10 = PCI FIFO empty entries for low water mark (0x10) */
			break;
			
		case fbiInit1:
			/* writes 0x00000100 */
			/* writes 0x00201102 */
			
			/* bit 1 = wait state cycles for PCI write accesses */
			/* bit 8 = video timing reset (1) */
			/* bit 12 = software blanking enable (1=always blank) */
			/* bit 21-20 = video timing cvclk source select */
			break;
			
		case fbiInit2:
			/* writes 0x80000040 */
			
			/* bit 6 = enable generated dram OE signal */
			/* bit 31-23 = refresh load value (0x100) */
			
			triple_buffer = (data >> 4) & 1;
			break;
			
		case fbiInit3:
			/* writes 0x00114000 */
			
			/* bit 16-13 = FBI-to-TREX bus clock delay (0xa) */
			/* bit 21-17 = TREX-to-FBI bus FIFO full thresh (0x8) */
			/* bit 31-22 = Y origin swap subtraction value */
			
			inverted_yorigin = (data >> 22) & 0x3ff;
			break;
		
		case dacData:
			/* bit 0-7 = data to write */
			/* bit 8-10 = register number */
			/* bit 11 = write (0) or read (1) */
			if (data & 0x800)
			{
				dac_read = ramdac_r((data >> 8) & 7);
				if (LOG_REGISTERS)
					log_cb(RETRO_LOG_DEBUG, LOGPRE "-- dacData read reg %d; result = %02X\n", (data >> 8) & 7, dac_read);
			}
			else
			{
				ramdac_w((data >> 8) & 7, data);
				if (LOG_REGISTERS)
					log_cb(RETRO_LOG_DEBUG, LOGPRE "-- dacData write reg %d = %02X\n", (data >> 8) & 7, data & 0xff);
			}
			break;
		
		case trexInit0:
			/* writes 0x00005441 */
			break;
		
		case trexInit1:
			/* writes 0x0000f420 */
			break;
		
		case textureMode:
			/* bit 0 = enable perspective correction */
			/* bit 1 = minification filter (0=point, 1=bilinear) */
			/* bit 2 = magnification filter (0=point, 1=bilinear) */
			/* bit 3 = clamp when W is negative (0=disabled, 1=force S,T=0) */
			/* bit 4 = enable LOD dithering (0=no dither, 1=dither) */
			/* bit 5 = NCC table select */
			/* bit 6 = clamp S (0=wrap, 1=clamp) */
			/* bit 7 = clamp T (0=wrap, 1=clamp) */
			/* bit 8-11 = texture format */
			/* ----- color combine unit ----- */
			/* bit 12 = zero other (0=c_other, 1=zero) */
			/* bit 13 = subtract color local (0=zero, 1=c_local) */
			/* bit 14-16 = mux select (0=zero, 1=c_local, 2=a_other, 3=a_local, 4=LOD, 5=LOD_frac) */
			/* bit 17 = reverse blend (0=normal, 1=reverse) */
			/* bit 18 = add color local */
			/* bit 19 = add alphal local */
			/* bit 20 = invert output */
			/* ----- alpha combine unit ----- */
			/* bit 21 = zero other (0=c_other, 1=zero) */
			/* bit 22 = subtract color local (0=zero, 1=c_local) */
			/* bit 23-25 = mux select (0=zero, 1=c_local, 2=a_other, 3=a_local, 4=LOD, 5=LOD_frac) */
			/* bit 26 = reverse blend (0=normal, 1=reverse) */
			/* bit 27 = add color local */
			/* bit 28 = add alpha local */
			/* bit 29 = invert output */
			/* bit 30 = enable trilinear (0=point sampled/bilinear, 1=trilinear) */
			/* bit 31 = sequential 8-bit download (0=even 32-bit word addresses, 1=sequential addresses) */
			
			if (chips & 2)
			{
				trex_perspective[0] = (data >> 0) & 1;
				trex_minification[0] = (data >> 1) & 1;
				trex_magnification[0] = (data >> 2) & 1;
				trex_clamps[0] = (data >> 6) & 1;
				trex_clampt[0] = (data >> 7) & 1;
				trex_format[0] = (data >> 8) & 0x0f;
				if ((trex_format[0] & 7) == 1 && (data & 0x20))
					trex_format[0] += 6;
				modes_used |= 1 << trex_format[0];
			}
			if (chips & 4)
			{
				trex_perspective[1] = (data >> 0) & 1;
				trex_minification[1] = (data >> 1) & 1;
				trex_magnification[1] = (data >> 2) & 1;
				trex_clamps[1] = (data >> 6) & 1;
				trex_clampt[1] = (data >> 7) & 1;
				trex_format[1] = (data >> 8) & 0x0f;
				if ((trex_format[1] & 7) == 1 && (data & 0x20))
					trex_format[1] += 6;
			}
			if (chips & 8)
			{
				trex_perspective[2] = (data >> 0) & 1;
				trex_minification[2] = (data >> 1) & 1;
				trex_magnification[2] = (data >> 2) & 1;
				trex_clamps[2] = (data >> 6) & 1;
				trex_clampt[2] = (data >> 7) & 1;
				trex_format[2] = (data >> 8) & 0x0f;
				if ((trex_format[2] & 7) == 1 && (data & 0x20))
					trex_format[2] += 6;
			}
			break;
		
		case tLOD:
			/* bit 0-5 = minimum LOD (4.2 unsigned) */
			/* bit 6-11 = maximum LOD (4.2 unsigned) */
			/* bit 12-17 = LOD bias (4.2 signed) */
			/* bit 18 = LOD odd (0=even, 1=odd) */
			/* bit 19 = texture is split (0=all LOD levels, 1=odd or even only) */
			/* bit 20 = S dimension is wider */
			/* bit 21-22 = aspect ratio */
			/* bit 23 = LOD zero frac */
			/* bit 24 = use multiple texBaseAddr registers */
			/* bit 25 = byte swap incoming texture data */
			/* bit 26 = short swap incoming texture data */
			/* bit 27 = enable raw direct texture memory writes (1=enable) */
			if (chips & 2)
			{
				trex_lodmin[0] = (data >> 0) & 0x3f;
				trex_lodmax[0] = (data >> 6) & 0x3f;
				if (trex_lodmax[0] > (8 << 2)) trex_lodmax[0] = 8 << 2;
				trex_lodbias[0] = ((INT16)(data >> 2) >> 10);
				trex_width[0] = (data & 0x00100000) ? 256 : (256 >> ((data >> 21) & 3));
				trex_height[0] = !(data & 0x00100000) ? 256 : (256 >> ((data >> 21) & 3));

				trex_lod_offset[0] = &lod_offset_table[(data >> 21) & 3][0];
				trex_lod_width_shift[0] = &lod_width_shift[(data >> 20) & 7][0];
				if (LOG_REGISTERS)
					log_cb(RETRO_LOG_DEBUG, LOGPRE "%06X:trex[0] -- lodmin=%02X lodmax=%02X size=%dx%d\n", activecpu_get_pc(), trex_lodmin[0], trex_lodmax[0], trex_width[0], trex_height[0]);
			}
			if (chips & 4)
			{
				trex_lodmin[1] = (data >> 0) & 0x3f;
				trex_lodmax[1] = (data >> 6) & 0x3f;
				if (trex_lodmax[1] > (8 << 2)) trex_lodmax[1] = 8 << 2;
				trex_lodbias[1] = ((INT16)(data >> 2) >> 10);
				trex_width[1] = (data & 0x00100000) ? 256 : (256 >> ((data >> 21) & 3));
				trex_height[1] = !(data & 0x00100000) ? 256 : (256 >> ((data >> 21) & 3));

				trex_lod_offset[1] = &lod_offset_table[(data >> 21) & 3][0];
				trex_lod_width_shift[1] = &lod_width_shift[(data >> 20) & 7][0];
				if (LOG_REGISTERS)
					log_cb(RETRO_LOG_DEBUG, LOGPRE "%06X:trex[1] -- lodmin=%02X lodmax=%02X size=%dx%d\n", activecpu_get_pc(), trex_lodmin[1], trex_lodmax[1], trex_width[1], trex_height[1]);
			}
			if (chips & 8)
			{
				trex_lodmin[2] = (data >> 0) & 0x3f;
				trex_lodmax[2] = (data >> 6) & 0x3f;
				if (trex_lodmax[2] > (8 << 2)) trex_lodmax[2] = 8 << 2;
				trex_lodbias[2] = ((INT16)(data >> 2) >> 10);
				trex_width[2] = (data & 0x00100000) ? 256 : (256 >> ((data >> 21) & 3));
				trex_height[2] = !(data & 0x00100000) ? 256 : (256 >> ((data >> 21) & 3));

				trex_lod_offset[2] = &lod_offset_table[(data >> 21) & 3][0];
				trex_lod_width_shift[2] = &lod_width_shift[(data >> 20) & 7][0];
				if (LOG_REGISTERS)
					log_cb(RETRO_LOG_DEBUG, LOGPRE "%06X:trex[2] -- lodmin=%02X lodmax=%02X size=%dx%d\n",activecpu_get_pc(),  trex_lodmin[2], trex_lodmax[2], trex_width[2], trex_height[2]);
			}
			break;
		
		case nccTable+0:
		case nccTable+1:
		case nccTable+2:
		case nccTable+3:
			if (chips & 2)
			{
				int base = 4 * (offset - (nccTable+0));
				ncc_y[0][0][base+0] = (data >>  0) & 0xff;
				ncc_y[0][0][base+1] = (data >>  8) & 0xff;
				ncc_y[0][0][base+2] = (data >> 16) & 0xff;
				ncc_y[0][0][base+3] = (data >> 24) & 0xff;
				texel_lookup_dirty[0][1] = 1;
				texel_lookup_dirty[0][9] = 1;
			}
			if (chips & 4)
			{
				int base = 4 * (offset - (nccTable+0));
				ncc_y[1][0][base+0] = (data >>  0) & 0xff;
				ncc_y[1][0][base+1] = (data >>  8) & 0xff;
				ncc_y[1][0][base+2] = (data >> 16) & 0xff;
				ncc_y[1][0][base+3] = (data >> 24) & 0xff;
				texel_lookup_dirty[1][1] = 1;
				texel_lookup_dirty[1][9] = 1;
			}
			break;
		
		case nccTable+4:
		case nccTable+5:
		case nccTable+6:
		case nccTable+7:
			if (chips & 2)
			{
				if (data & 0x80000000)
				{
					texel_lookup[0][5][((data >> 23) & 0xfe) | (~offset & 1)] = 0xff000000 | data;
					texel_lookup_dirty[0][14] = 1;
				}
				else
				{
					int base = offset - (nccTable+4);
					ncc_ir[0][0][base] = (INT32)(data <<  5) >> 23;
					ncc_ig[0][0][base] = (INT32)(data << 14) >> 23;
					ncc_ib[0][0][base] = (INT32)(data << 23) >> 23;
					texel_lookup_dirty[0][1] = 1;
					texel_lookup_dirty[0][9] = 1;
				}
			}
			if (chips & 4)
			{
				if (data & 0x80000000)
				{
					texel_lookup[1][5][((data >> 23) & 0xfe) | (~offset & 1)] = 0xff000000 | data;
					texel_lookup_dirty[1][14] = 1;
				}
				else
				{
					int base = offset - (nccTable+4);
					ncc_ir[1][0][base] = (INT32)(data <<  5) >> 23;
					ncc_ig[1][0][base] = (INT32)(data << 14) >> 23;
					ncc_ib[1][0][base] = (INT32)(data << 23) >> 23;
					texel_lookup_dirty[1][1] = 1;
					texel_lookup_dirty[1][9] = 1;
				}
			}
			break;
		
		case nccTable+8:
		case nccTable+9:
		case nccTable+10:
		case nccTable+11:
			if (chips & 2)
			{
				if (data & 0x80000000)
				{
					texel_lookup[0][5][((data >> 23) & 0xfe) | (~offset & 1)] = 0xff000000 | data;
					texel_lookup_dirty[0][14] = 1;
				}
				else
				{
					int base = offset - (nccTable+8);
					ncc_qr[0][0][base] = (INT32)(data <<  5) >> 23;
					ncc_qg[0][0][base] = (INT32)(data << 14) >> 23;
					ncc_qb[0][0][base] = (INT32)(data << 23) >> 23;
					texel_lookup_dirty[0][1] = 1;
					texel_lookup_dirty[0][9] = 1;
				}
			}
			if (chips & 4)
			{
				if (data & 0x80000000)
				{
					texel_lookup[1][5][((data >> 23) & 0xfe) | (~offset & 1)] = 0xff000000 | data;
					texel_lookup_dirty[1][14] = 1;
				}
				else
				{
					int base = offset - (nccTable+8);
					ncc_qr[1][0][base] = (INT32)(data <<  5) >> 23;
					ncc_qg[1][0][base] = (INT32)(data << 14) >> 23;
					ncc_qb[1][0][base] = (INT32)(data << 23) >> 23;
					texel_lookup_dirty[1][1] = 1;
					texel_lookup_dirty[1][9] = 1;
				}
			}
			break;
		
		case nccTable+12:
		case nccTable+13:
		case nccTable+14:
		case nccTable+15:
			if (chips & 2)
			{
				int base = 4 * (offset - (nccTable+12));
				ncc_y[0][1][base+0] = (data >>  0) & 0xff;
				ncc_y[0][1][base+1] = (data >>  8) & 0xff;
				ncc_y[0][1][base+2] = (data >> 16) & 0xff;
				ncc_y[0][1][base+3] = (data >> 24) & 0xff;
				texel_lookup_dirty[0][7] = 1;
				texel_lookup_dirty[0][15] = 1;
			}
			if (chips & 4)
			{
				int base = 4 * (offset - (nccTable+12));
				ncc_y[1][1][base+0] = (data >>  0) & 0xff;
				ncc_y[1][1][base+1] = (data >>  8) & 0xff;
				ncc_y[1][1][base+2] = (data >> 16) & 0xff;
				ncc_y[1][1][base+3] = (data >> 24) & 0xff;
				texel_lookup_dirty[1][7] = 1;
				texel_lookup_dirty[1][15] = 1;
			}
			break;
		
		case nccTable+16:
		case nccTable+17:
		case nccTable+18:
		case nccTable+19:
			if (chips & 2)
			{
				int base = offset - (nccTable+16);
				ncc_ir[0][1][base] = (INT32)(data <<  5) >> 23;
				ncc_ig[0][1][base] = (INT32)(data << 14) >> 23;
				ncc_ib[0][1][base] = (INT32)(data << 23) >> 23;
				texel_lookup_dirty[0][7] = 1;
				texel_lookup_dirty[0][15] = 1;
			}
			if (chips & 4)
			{
				int base = offset - (nccTable+16);
				ncc_ir[1][1][base] = (INT32)(data <<  5) >> 23;
				ncc_ig[1][1][base] = (INT32)(data << 14) >> 23;
				ncc_ib[1][1][base] = (INT32)(data << 23) >> 23;
				texel_lookup_dirty[1][7] = 1;
				texel_lookup_dirty[1][15] = 1;
			}
			break;
		
		case nccTable+20:
		case nccTable+21:
		case nccTable+22:
		case nccTable+23:
			if (chips & 2)
			{
				int base = offset - (nccTable+20);
				ncc_qr[0][1][base] = (INT32)(data <<  5) >> 23;
				ncc_qg[0][1][base] = (INT32)(data << 14) >> 23;
				ncc_qb[0][1][base] = (INT32)(data << 23) >> 23;
				texel_lookup_dirty[0][7] = 1;
				texel_lookup_dirty[0][15] = 1;
			}
			if (chips & 4)
			{
				int base = offset - (nccTable+20);
				ncc_qr[1][1][base] = (INT32)(data <<  5) >> 23;
				ncc_qg[1][1][base] = (INT32)(data << 14) >> 23;
				ncc_qb[1][1][base] = (INT32)(data << 23) >> 23;
				texel_lookup_dirty[1][7] = 1;
				texel_lookup_dirty[1][15] = 1;
			}
			break;
		
		case fogTable+0:	case fogTable+1:	case fogTable+2:	case fogTable+3:
		case fogTable+4:	case fogTable+5:	case fogTable+6:	case fogTable+7:
		case fogTable+8:	case fogTable+9:	case fogTable+10:	case fogTable+11:
		case fogTable+12:	case fogTable+13:	case fogTable+14:	case fogTable+15:
		case fogTable+16:	case fogTable+17:	case fogTable+18:	case fogTable+19:
		case fogTable+20:	case fogTable+21:	case fogTable+22:	case fogTable+23:
		case fogTable+24:	case fogTable+25:	case fogTable+26:	case fogTable+27:
		case fogTable+28:	case fogTable+29:	case fogTable+30:	case fogTable+31:
		{
			int base = (offset - fogTable) * 2;
			fog_delta[base + 0] = (data >> 0) & 0xff;
			fog_blend[base + 0] = (data >> 8) & 0xff;
			fog_delta[base + 1] = (data >> 16) & 0xff;
			fog_blend[base + 1] = (data >> 24) & 0xff;
			break;
		}
		
		case bltCommand:
			fprintf(stderr, "WARNING: blt command %08X\n", data);
			break;
	}
}



/*************************************
 *
 *	MMIO register reads
 *
 *************************************/

READ32_HANDLER( voodoo_regs_r )
{
	data32_t result;
	
	if ((offset & 0x800c0) == 0x80000 && (voodoo_regs[fbiInit3] & 1))
		offset = register_alias_map[offset & 0x3f];
	else
		offset &= 0xff;
	
	result = voodoo_regs[offset];
	switch (offset)
	{
		case status:
		{
			result = 0;
			
			/* FIFO free space */
			if (!blocked_on_swap)
				result |= 0x3f;
			
			/* vertical retrace */
			result |= (cpu_getvblank()) << 6;
			
			/* FBI graphics engine busy */
			result |= blocked_on_swap << 7;
			
			/* TREX busy */
			result |= 0 << 8;
			
			/* SST-1 overall busy */
			result |= blocked_on_swap << 9;
			
			/* buffer displayed (0-2) */
			result |= (frontbuf == framebuf[1]) << 10;
			
			/* memory FIFO free space */
			if (!blocked_on_swap)
				result |= 0xffff << 12;
			
			/* swap buffers pending */
			result |= swaps_pending << 28;
			
			activecpu_eat_cycles(100);
			
			if (LOG_REGISTERS)
			{
				offs_t pc = activecpu_get_pc();
				if (pc == status_lastpc)
					status_lastpc_count++;
				else
				{
					if (status_lastpc_count)
						log_cb(RETRO_LOG_DEBUG, LOGPRE "%06X:voodoo status read = %08X (x%d)\n", activecpu_get_pc(), result, status_lastpc_count);
					status_lastpc_count = 0;
					status_lastpc = pc;
					log_cb(RETRO_LOG_DEBUG, LOGPRE "%06X:voodoo status read = %08X\n", activecpu_get_pc(), result);
				}
			}
			break;
		}
		
		case fbiInit2:
			/* bit 2 of the initEnable register maps this to dacRead */
			if (init_enable & 0x00000004)
				result = dac_read;

			if (LOG_REGISTERS)
				log_cb(RETRO_LOG_DEBUG, LOGPRE "%06X:voodoo fbiInit2 read = %08X\n", activecpu_get_pc(), result);
			break;
		
		case vRetrace:
			result = cpu_getscanline();
/*			if (LOG_REGISTERS)*/
/*				log_cb(RETRO_LOG_DEBUG, LOGPRE "%06X:voodoo vRetrace read = %08X\n", activecpu_get_pc(), result);*/
			break;
		
		/* reserved area in the TMU read by the Vegas startup sequence */
		case hvRetrace:
			result = 0x200 << 16;	/* should be between 0x7b and 0x267 */
			result |= 0x80;			/* should be between 0x17 and 0x103 */
			break;

		default:
			if (LOG_REGISTERS)
				log_cb(RETRO_LOG_DEBUG, LOGPRE "%06X:voodoo %s read = %08X\n", activecpu_get_pc(), (offset < 0x340/4) ? voodoo_reg_name[offset] : "oob", result);
			break;
	}
	return result;
}



/*************************************
 *
 *	LFB writes
 *
 *************************************/

static void lfbwrite_0(offs_t offset, data32_t data, data32_t mem_mask)
{
	UINT16 *buffer = *lfb_write_buffer;
	int y = offset / (FRAMEBUF_WIDTH/2);
	int x = (offset % (FRAMEBUF_WIDTH/2)) * 2;
	if (lfb_flipy)
		y = inverted_yorigin - y;
	if (ACCESSING_LSW32)
		buffer[y * FRAMEBUF_WIDTH + x] = data;
	if (ACCESSING_MSW32)
		buffer[y * FRAMEBUF_WIDTH + x + 1] = data >> 16;
/*	log_cb(RETRO_LOG_DEBUG, LOGPRE "%06X:LFB write mode 0 @ %08X = %08X & %08X\n", activecpu_get_pc(), offset, data, ~mem_mask);*/
}

static void lfbwrite_1(offs_t offset, data32_t data, data32_t mem_mask)
{
	UINT16 *buffer = *lfb_write_buffer;
	int y = offset / (FRAMEBUF_WIDTH/2);
	int x = (offset % (FRAMEBUF_WIDTH/2)) * 2;
	if (lfb_flipy)
		y = inverted_yorigin - y;
	if (ACCESSING_LSW32)
		buffer[y * FRAMEBUF_WIDTH + x] = ((data << 1) & 0xffe0) | (data & 0x001f);
	if (ACCESSING_MSW32)
		buffer[y * FRAMEBUF_WIDTH + x + 1] = ((data >> 15) & 0xffe0) | ((data >> 16) & 0x001f);
/*	log_cb(RETRO_LOG_DEBUG, LOGPRE "%06X:LFB write mode 1 @ %08X = %08X & %08X\n", activecpu_get_pc(), offset, data, ~mem_mask);*/
}

static void lfbwrite_2(offs_t offset, data32_t data, data32_t mem_mask)
{
	UINT16 *buffer = *lfb_write_buffer;
	int y = offset / (FRAMEBUF_WIDTH/2);
	int x = (offset % (FRAMEBUF_WIDTH/2)) * 2;
	if (lfb_flipy)
		y = inverted_yorigin - y;
	if (ACCESSING_LSW32)
		buffer[y * FRAMEBUF_WIDTH + x] = ((data << 1) & 0xffe0) | (data & 0x001f);
	if (ACCESSING_MSW32)
		buffer[y * FRAMEBUF_WIDTH + x + 1] = ((data >> 15) & 0xffe0) | ((data >> 16) & 0x001f);
/*	log_cb(RETRO_LOG_DEBUG, LOGPRE "%06X:LFB write mode 2 @ %08X = %08X & %08X\n", activecpu_get_pc(), offset, data, ~mem_mask);*/
}

static void lfbwrite_3(offs_t offset, data32_t data, data32_t mem_mask)
{
	log_cb(RETRO_LOG_DEBUG, LOGPRE "%06X:Unimplementd LFB write mode 3 @ %08X = %08X & %08X\n", activecpu_get_pc(), offset, data, ~mem_mask);
}

static void lfbwrite_4(offs_t offset, data32_t data, data32_t mem_mask)
{
	UINT16 *buffer = *lfb_write_buffer;
	int y = offset / FRAMEBUF_WIDTH;
	int x = offset % FRAMEBUF_WIDTH;
	if (lfb_flipy)
		y = inverted_yorigin - y;
	buffer[y * FRAMEBUF_WIDTH + x] = (((data >> 19) & 0x1f) << 11) | (((data >> 10) & 0x3f) << 5) | (((data >> 3) & 0x1f) << 0);
/*	log_cb(RETRO_LOG_DEBUG, LOGPRE "%06X:LFB write mode 4 @ %08X = %08X & %08X\n", activecpu_get_pc(), offset, data, ~mem_mask);*/
}

static void lfbwrite_5(offs_t offset, data32_t data, data32_t mem_mask)
{
	UINT16 *buffer = *lfb_write_buffer;
	int y = offset / FRAMEBUF_WIDTH;
	int x = offset % FRAMEBUF_WIDTH;
	if (lfb_flipy)
		y = inverted_yorigin - y;
	buffer[y * FRAMEBUF_WIDTH + x] = (((data >> 19) & 0x1f) << 11) | (((data >> 10) & 0x3f) << 5) | (((data >> 3) & 0x1f) << 0);
/*	log_cb(RETRO_LOG_DEBUG, LOGPRE "%06X:LFB write mode 5 @ %08X = %08X & %08X\n", activecpu_get_pc(), offset, data, ~mem_mask);*/
}

static void lfbwrite_6(offs_t offset, data32_t data, data32_t mem_mask)
{
	log_cb(RETRO_LOG_DEBUG, LOGPRE "%06X:Unimplementd LFB write mode 6 @ %08X = %08X & %08X\n", activecpu_get_pc(), offset, data, ~mem_mask);
}

static void lfbwrite_7(offs_t offset, data32_t data, data32_t mem_mask)
{
	log_cb(RETRO_LOG_DEBUG, LOGPRE "%06X:Unimplementd LFB write mode 7 @ %08X = %08X & %08X\n", activecpu_get_pc(), offset, data, ~mem_mask);
}

static void lfbwrite_8(offs_t offset, data32_t data, data32_t mem_mask)
{
	log_cb(RETRO_LOG_DEBUG, LOGPRE "%06X:Unimplementd LFB write mode 8 @ %08X = %08X & %08X\n", activecpu_get_pc(), offset, data, ~mem_mask);
}

static void lfbwrite_9(offs_t offset, data32_t data, data32_t mem_mask)
{
	log_cb(RETRO_LOG_DEBUG, LOGPRE "%06X:Unimplementd LFB write mode 9 @ %08X = %08X & %08X\n", activecpu_get_pc(), offset, data, ~mem_mask);
}

static void lfbwrite_a(offs_t offset, data32_t data, data32_t mem_mask)
{
	log_cb(RETRO_LOG_DEBUG, LOGPRE "%06X:Unimplementd LFB write mode a @ %08X = %08X & %08X\n", activecpu_get_pc(), offset, data, ~mem_mask);
}

static void lfbwrite_b(offs_t offset, data32_t data, data32_t mem_mask)
{
	log_cb(RETRO_LOG_DEBUG, LOGPRE "%06X:Unimplementd LFB write mode b @ %08X = %08X & %08X\n", activecpu_get_pc(), offset, data, ~mem_mask);
}

static void lfbwrite_c(offs_t offset, data32_t data, data32_t mem_mask)
{
	UINT16 *buffer = *lfb_write_buffer;
	int y = offset / FRAMEBUF_WIDTH;
	int x = offset % FRAMEBUF_WIDTH;
	if (lfb_flipy)
		y = inverted_yorigin - y;
	if (ACCESSING_LSW32)
		buffer[y * FRAMEBUF_WIDTH + x] = data;
	if (ACCESSING_MSW32)
		depthbuf[y * FRAMEBUF_WIDTH + x] = data >> 16;
/*	log_cb(RETRO_LOG_DEBUG, LOGPRE "%06X:LFB write mode c @ %08X = %08X & %08X\n", activecpu_get_pc(), offset, data, ~mem_mask);*/
}

static void lfbwrite_d(offs_t offset, data32_t data, data32_t mem_mask)
{
	UINT16 *buffer = *lfb_write_buffer;
	int y = offset / FRAMEBUF_WIDTH;
	int x = offset % FRAMEBUF_WIDTH;
	if (lfb_flipy)
		y = inverted_yorigin - y;
	if (ACCESSING_LSW32)
		buffer[y * FRAMEBUF_WIDTH + x] = ((data << 1) & 0xffc0) | (data & 0x001f);
	if (ACCESSING_MSW32)
		depthbuf[y * FRAMEBUF_WIDTH + x] = data >> 16;
/*	log_cb(RETRO_LOG_DEBUG, LOGPRE "%06X:LFB write mode d @ %08X = %08X & %08X\n", activecpu_get_pc(), offset, data, ~mem_mask);*/
}

static void lfbwrite_e(offs_t offset, data32_t data, data32_t mem_mask)
{
	UINT16 *buffer = *lfb_write_buffer;
	int y = offset / FRAMEBUF_WIDTH;
	int x = offset % FRAMEBUF_WIDTH;
	if (lfb_flipy)
		y = inverted_yorigin - y;
	if (ACCESSING_LSW32)
		buffer[y * FRAMEBUF_WIDTH + x] = ((data << 1) & 0xffc0) | (data & 0x001f);
	if (ACCESSING_MSW32)
		depthbuf[y * FRAMEBUF_WIDTH + x] = data >> 16;
/*	log_cb(RETRO_LOG_DEBUG, LOGPRE "%06X:LFB write mode e @ %08X = %08X & %08X\n", activecpu_get_pc(), offset, data, ~mem_mask);*/
}

static void lfbwrite_f(offs_t offset, data32_t data, data32_t mem_mask)
{
	int y = offset / (FRAMEBUF_WIDTH/2);
	int x = (offset % (FRAMEBUF_WIDTH/2)) * 2;
	if (lfb_flipy)
		y = inverted_yorigin - y;
	if (ACCESSING_LSW32)
		depthbuf[y * FRAMEBUF_WIDTH + x] = ((data << 1) & 0xffe0) | (data & 0x001f);
	if (ACCESSING_MSW32)
		depthbuf[y * FRAMEBUF_WIDTH + x + 1] = ((data >> 15) & 0xffe0) | ((data >> 16) & 0x001f);
/*	log_cb(RETRO_LOG_DEBUG, LOGPRE "%06X:LFB write mode f @ %08X = %08X & %08X\n", activecpu_get_pc(), offset, data, ~mem_mask);*/
}

static void (*lfbwrite[16])(offs_t offset, data32_t data, data32_t mem_mask) =
{
	lfbwrite_0,	lfbwrite_1,	lfbwrite_2,	lfbwrite_3,	
	lfbwrite_4,	lfbwrite_5,	lfbwrite_6,	lfbwrite_7,	
	lfbwrite_8,	lfbwrite_9,	lfbwrite_a,	lfbwrite_b,	
	lfbwrite_c,	lfbwrite_d,	lfbwrite_e,	lfbwrite_f
};

WRITE32_HANDLER( voodoo_framebuf_w )
{
	if (blocked_on_swap)
		cpu_spinuntil_trigger(13579);
	(*lfbwrite[lfb_write_format])(offset, data, mem_mask);
}



/*************************************
 *
 *	LFB reads
 *
 *************************************/

READ32_HANDLER( voodoo_framebuf_r )
{
	UINT16 *buffer = *lfb_read_buffer;
/*
	UINT32 result;
	if (lfb_flipy)
	{
		int y = offset / (1024/4);
		y = inverted_yorigin - y;
		offset = y * (1024/4) + (offset & 0x3ff/4);
	}
	result = buffer[offset * 2] | (buffer[offset * 2 + 1] << 16);
*/
	int y = offset / (FRAMEBUF_WIDTH/2);
	int x = (offset % (FRAMEBUF_WIDTH/2)) * 2;
	UINT32 result;
	if (lfb_flipy)
		y = inverted_yorigin - y;
	result = buffer[y * FRAMEBUF_WIDTH + x] | (buffer[y * FRAMEBUF_WIDTH + x + 1] << 16);

	log_cb(RETRO_LOG_DEBUG, LOGPRE "%06X:voodoo_framebuf_r[%06X] = %08X & %08X\n", activecpu_get_pc(), offset, result, ~mem_mask);
	return result;
} 



/*************************************
 *
 *	Texture RAM writes (no read access)
 *
 *************************************/

WRITE32_HANDLER( voodoo_textureram_w )
{
	int trex = (offset >> 19) & 0x03;
	int trex_base = 0x100 + 0x100 * trex;
	offs_t tbaseaddr = voodoo_regs[trex_base + texBaseAddr] * 8;
	int lod = (offset >> 13) & 0x3c;
	int t = (offset >> 7) & 0xff;
	int s = (offset << 1) & 0xfe;
	int twidth = trex_width[trex];
	int theight = trex_height[trex];

	if (trex >= tmus)
	{
		if (trex != 3)
			log_cb(RETRO_LOG_DEBUG, LOGPRE "TMU %d write\n", trex);
		return;
	}
	
/*	if (lod < trex_lodmin[trex] || lod > trex_lodmax[trex])*/
/*		return;*/

	/* swizzle the data */
	if (voodoo_regs[trex_base + tLOD] & 0x02000000)
		data = (data >> 24) | ((data >> 8) & 0xff00) | ((data << 8) & 0xff0000) | (data << 24);
	if (voodoo_regs[trex_base + tLOD] & 0x04000000)
		data = (data >> 16) | (data << 16);

if (s == 0 && t == 0)	
	log_cb(RETRO_LOG_DEBUG, LOGPRE "%06X:voodoo_textureram_w[%d,%06X,%d,%02X,%02X]", activecpu_get_pc(), trex, tbaseaddr & texram_mask, lod >> 2, s, t);
	while (lod != 0)
	{
		lod -= 4;
		
		if (trex_format[trex] < 8)
			tbaseaddr += twidth * theight;
		else
			tbaseaddr += 2 * twidth * theight;

		twidth >>= 1;
		if (twidth == 0)
			twidth = 1;

		theight >>= 1;
		if (theight == 0)
			theight = 1;
	}
	tbaseaddr &= texram_mask;
	
	if (trex_format[trex] < 8)
	{
		UINT8 *dest = textureram[trex];
		if (voodoo_regs[0x100/*trex_base -- breaks gauntleg */ + textureMode] & 0x80000000)
			tbaseaddr += t * twidth + ((s << 1) & 0xfc);
		else
			tbaseaddr += t * twidth + (s & 0xfc);
if (s == 0 && t == 0)	
	log_cb(RETRO_LOG_DEBUG, LOGPRE " -> %06X = %08X\n", tbaseaddr, data);
		dest[BYTE4_XOR_LE(tbaseaddr + 0)] = (data >> 0) & 0xff;
		dest[BYTE4_XOR_LE(tbaseaddr + 1)] = (data >> 8) & 0xff;
		dest[BYTE4_XOR_LE(tbaseaddr + 2)] = (data >> 16) & 0xff;
		dest[BYTE4_XOR_LE(tbaseaddr + 3)] = (data >> 24) & 0xff;
	}
	else
	{
		UINT16 *dest = (UINT16 *)textureram[trex];
		tbaseaddr /= 2;
		tbaseaddr += t * twidth + s;
if (s == 0 && t == 0)	
	log_cb(RETRO_LOG_DEBUG, LOGPRE " -> %06X = %08X\n", tbaseaddr*2, data);
		dest[BYTE_XOR_LE(tbaseaddr + 0)] = (data >> 0) & 0xffff;
		dest[BYTE_XOR_LE(tbaseaddr + 1)] = (data >> 16) & 0xffff;
	}
}



/*************************************
 *
 *	Texel lookups
 *
 *************************************/

static void init_texel_0(int which)
{
	/* format 0: 8-bit 3-3-2 */
	int r, g, b, i;
	for (i = 0; i < 256; i++)
	{
		r = (i >> 5) & 7;
		g = (i >> 2) & 7;
		b = i & 3;
		r = (r << 5) | (r << 2) | (r >> 1);
		g = (g << 5) | (g << 2) | (g >> 1);
		b = (b << 6) | (b << 4) | (b << 2) | b;
		texel_lookup[which][0][i] = 0xff000000 | (r << 16) | (g << 8) | b;
	}
}


static void init_texel_1(int which)
{
	/* format 1: 8-bit YIQ, NCC table 0 */
	int r, g, b, i;
	for (i = 0; i < 256; i++)
	{
		int vi = (i >> 2) & 0x03;
		int vq = (i >> 0) & 0x03;

		r = g = b = ncc_y[which][0][(i >> 4) & 0x0f];
		r += ncc_ir[which][0][vi] + ncc_qr[which][0][vq];
		g += ncc_ig[which][0][vi] + ncc_qg[which][0][vq];
		b += ncc_ib[which][0][vi] + ncc_qb[which][0][vq];

		if (r < 0) r = 0;
		else if (r > 255) r = 255;
		if (g < 0) g = 0;
		else if (g > 255) g = 255;
		if (b < 0) b = 0;
		else if (b > 255) b = 255;
		
		texel_lookup[which][1][i] = 0xff000000 | (r << 16) | (g << 8) | b;
	}
}


static void init_texel_2(int which)
{
	/* format 2: 8-bit alpha */
	int i;
	for (i = 0; i < 256; i++)
		texel_lookup[which][2][i] = (i << 24) | (i << 16) | (i << 8) | i;
}


static void init_texel_3(int which)
{
	/* format 3: 8-bit intensity */
	int i;
	for (i = 0; i < 256; i++)
		texel_lookup[which][3][i] = 0xff000000 | (i << 16) | (i << 8) | i;
}


static void init_texel_4(int which)
{
	/* format 4: 8-bit alpha, intensity (4-4) */
	int a, r, i;
	for (i = 0; i < 256; i++)
	{
		a = i >> 4;
		r = i & 15;
		a = (a << 4) | a;
		r = (r << 4) | r;
		texel_lookup[which][4][i] = (a << 24) | (r << 16) | (r << 8) | r;
	}
}


static void init_texel_5(int which)
{
	/* format 5: 8-bit palette -- updated dynamically */
}


static void init_texel_6(int which)
{
	/* format 6: 8-bit unused */
}


static void init_texel_7(int which)
{
	/* format 7: 8-bit YIQ, NCC table 1 (used internally, not really on the card) */
	int r, g, b, i;
	for (i = 0; i < 256; i++)
	{
		int vi = (i >> 2) & 0x03;
		int vq = (i >> 0) & 0x03;

		r = g = b = ncc_y[which][1][(i >> 4) & 0x0f];
		r += ncc_ir[which][1][vi] + ncc_qr[which][1][vq];
		g += ncc_ig[which][1][vi] + ncc_qg[which][1][vq];
		b += ncc_ib[which][1][vi] + ncc_qb[which][1][vq];

		if (r < 0) r = 0;
		else if (r > 255) r = 255;
		if (g < 0) g = 0;
		else if (g > 255) g = 255;
		if (b < 0) b = 0;
		else if (b > 255) b = 255;
		
		texel_lookup[which][7][i] = 0xff000000 | (r << 16) | (g << 8) | b;
	}
}


static void init_texel_8(int which)
{
	/* format 8: 16-bit ARGB (8-3-3-2) */
	int a, r, g, b, i;
	for (i = 0; i < 65536; i++)
	{
		a = i >> 8;
		r = (i >> 5) & 7;
		g = (i >> 2) & 7;
		b = i & 3;
		r = (r << 5) | (r << 2) | (r >> 1);
		g = (g << 5) | (g << 2) | (g >> 1);
		b = (b << 6) | (b << 4) | (b << 2) | b;
		texel_lookup[which][8][i] = (a << 24) | (r << 16) | (g << 8) | b;
	}
}


static void init_texel_9(int which)
{
	/* format 9: 16-bit YIQ, NCC table 0 */
	int a, r, g, b, i;
	for (i = 0; i < 65536; i++)
	{
		int vi = (i >> 2) & 0x03;
		int vq = (i >> 0) & 0x03;

		a = i >> 8;
		r = g = b = ncc_y[which][0][(i >> 4) & 0x0f];
		r += ncc_ir[which][0][vi] + ncc_qr[which][0][vq];
		g += ncc_ig[which][0][vi] + ncc_qg[which][0][vq];
		b += ncc_ib[which][0][vi] + ncc_qb[which][0][vq];

		if (r < 0) r = 0;
		else if (r > 255) r = 255;
		if (g < 0) g = 0;
		else if (g > 255) g = 255;
		if (b < 0) b = 0;
		else if (b > 255) b = 255;
		
		texel_lookup[which][9][i] = (a << 24) | (r << 16) | (g << 8) | b;
	}
}


static void init_texel_a(int which)
{
	/* format 10: 16-bit RGB (5-6-5) */
	int r, g, b, i;
	for (i = 0; i < 65536; i++)
	{
		r = (i >> 11) & 0x1f;
		g = (i >> 5) & 0x3f;
		b = i & 0x1f;
		r = (r << 3) | (r >> 2);
		g = (g << 2) | (g >> 4);
		b = (b << 3) | (b >> 2);
		texel_lookup[which][10][i] = 0xff000000 | (r << 16) | (g << 8) | b;
	}
}

		
static void init_texel_b(int which)
{
	/* format 11: 16-bit ARGB (1-5-5-5) */
	int a, r, g, b, i;
	for (i = 0; i < 65536; i++)
	{
		a = ((i >> 15) & 1) ? 0xff : 0x00;
		r = (i >> 10) & 0x1f;
		g = (i >> 5) & 0x1f;
		b = i & 0x1f;
		r = (r << 3) | (r >> 2);
		g = (g << 3) | (g >> 2);
		b = (b << 3) | (b >> 2);
		texel_lookup[which][11][i] = (a << 24) | (r << 16) | (g << 8) | b;
	}
}

		
static void init_texel_c(int which)
{
	/* format 12: 16-bit ARGB (4-4-4-4) */
	int a, r, g, b, i;
	for (i = 0; i < 65536; i++)
	{
		a = (i >> 12) & 0x0f;
		r = (i >> 8) & 0x0f;
		g = (i >> 4) & 0x0f;
		b = i & 0x0f;
		a = (a << 4) | a;
		r = (r << 4) | r;
		g = (g << 4) | g;
		b = (b << 4) | b;
		texel_lookup[which][12][i] = (a << 24) | (r << 16) | (g << 8) | b;
	}
}

		
static void init_texel_d(int which)
{
	/* format 13: 16-bit alpha, intensity */
	int a, r, i;
	for (i = 0; i < 65536; i++)
	{
		a = i >> 8;
		r = i & 0xff;
		texel_lookup[which][13][i] = (a << 24) | (r << 16) | (r << 8) | r;
	}
}


static void init_texel_e(int which)
{
	/* format 14: 16-bit alpha, palette */
	int a, i;
	for (i = 0; i < 65536; i++)
	{
		a = i >> 8;
		texel_lookup[which][14][i] = (a << 24) | (texel_lookup[which][5][i & 0xff] & 0x00ffffff);
	}
}


static void init_texel_f(int which)
{
	/* format 9: 16-bit YIQ, NCC table 1 */
	int a, r, g, b, i;
	for (i = 0; i < 65536; i++)
	{
		int vi = (i >> 2) & 0x03;
		int vq = (i >> 0) & 0x03;

		a = i >> 8;
		r = g = b = ncc_y[which][1][(i >> 4) & 0x0f];
		r += ncc_ir[which][1][vi] + ncc_qr[which][1][vq];
		g += ncc_ig[which][1][vi] + ncc_qg[which][1][vq];
		b += ncc_ib[which][1][vi] + ncc_qb[which][1][vq];

		if (r < 0) r = 0;
		else if (r > 255) r = 255;
		if (g < 0) g = 0;
		else if (g > 255) g = 255;
		if (b < 0) b = 0;
		else if (b > 255) b = 255;
		
		texel_lookup[which][15][i] = (a << 24) | (r << 16) | (g << 8) | b;
	}
}


static void (*update_texel_lookup[16])(int which) =
{
	init_texel_0,	init_texel_1,	init_texel_2,	init_texel_3,
	init_texel_4,	init_texel_5,	init_texel_6,	init_texel_7,
	init_texel_8,	init_texel_9,	init_texel_a,	init_texel_b,
	init_texel_c,	init_texel_d,	init_texel_e,	init_texel_f
};



/*************************************
 *
 *	Generate blitters
 *
 *************************************/

/* 
	WG3dh:
	
    816782: 0C000035 00000000 00045119 000B4779 082410DF
    629976: 0C000035 00000000 00045119 000B4779 0824109F
    497958: 0C000035 00000000 00045119 000B4779 0824101F
    141069: 0C000035 00000000 00045119 000B4779 082418DF
*/

#define NUM_TMUS			1

#define FBZCOLORPATH		0x0c000035
#define ALPHAMODE			0x00045119
#define FBZMODE				0x000b4779
#define TEXTUREMODE1		0x00000000

#define RENDERFUNC			render_0c000035_00045119_000b4779_0824101f
#define TEXTUREMODE0		0x0824101f
#include "voodblit.c"
#undef TEXTUREMODE0
#undef RENDERFUNC

#define RENDERFUNC			render_0c000035_00045119_000b4779_0824109f
#define TEXTUREMODE0		0x0824109f
#include "voodblit.c"
#undef TEXTUREMODE0
#undef RENDERFUNC

#define RENDERFUNC			render_0c000035_00045119_000b4779_082410df
#define TEXTUREMODE0		0x082410df
#include "voodblit.c"
#undef TEXTUREMODE0
#undef RENDERFUNC

#define RENDERFUNC			render_0c000035_00045119_000b4779_082418df
#define TEXTUREMODE0		0x082418df
#include "voodblit.c"
#undef TEXTUREMODE0
#undef RENDERFUNC

#undef FBZMODE
#undef ALPHAMODE
#undef FBZCOLORPATH
#undef TEXTUREMODE1

#undef NUM_TMUS


/*

	mace:
	
000000001173E00C: 0C000035 00000000 00045119 000B4779 082418DF (done)
000000000D3EB6D7: 0C600C09 00000000 00045119 000B4779 0824100F
0000000003E4A1D5: 0C600C09 00000000 00045119 000B4779 0824180F
0000000003AAEA07: 0C600C09 00000000 00045119 000B4779 082418CF
000000000389D5A8: 0C480035 00000000 00045119 000B4779 082418DF
000000000168ED9C: 0C480035 00000000 00045119 000B4379 082418DF
000000000142E146: 08602401 00000000 00045119 000B4779 082418DF

*/

#define NUM_TMUS			1

#define FBZCOLORPATH		0x0c600c09
#define ALPHAMODE			0x00045119
#define FBZMODE				0x000b4779
#define TEXTUREMODE1		0x00000000

#define RENDERFUNC			render_0c600c09_00045119_000b4779_0824100f
#define TEXTUREMODE0		0x0824100f
#include "voodblit.c"
#undef TEXTUREMODE0
#undef RENDERFUNC

#define RENDERFUNC			render_0c600c09_00045119_000b4779_0824180f
#define TEXTUREMODE0		0x0824180f
#include "voodblit.c"
#undef TEXTUREMODE0
#undef RENDERFUNC

#define RENDERFUNC			render_0c600c09_00045119_000b4779_082418cf
#define TEXTUREMODE0		0x082418cf
#include "voodblit.c"
#undef TEXTUREMODE0
#undef RENDERFUNC

#undef FBZCOLORPATH
#undef FBZMODE
#define FBZCOLORPATH		0x0c480035
#define TEXTUREMODE0		0x082418df

#define FBZMODE				0x000b4779
#define RENDERFUNC			render_0c480035_00045119_000b4779_082418df
#include "voodblit.c"
#undef FBZMODE
#undef RENDERFUNC

#define FBZMODE				0x000b4379
#define RENDERFUNC			render_0c480035_00045119_000b4379_082418df
#include "voodblit.c"
#undef FBZMODE
#undef RENDERFUNC

#undef TEXTUREMODE0
#undef RENDERFUNC
#undef ALPHAMODE
#undef FBZCOLORPATH
#undef TEXTUREMODE1

#undef NUM_TMUS


/*
	blitz99:

389BA0CA: 0C000035 00000000 00040400 000B4739 0C26180F 00000000 00000000
0667BB5A: 0C582C35 00000000 00515110 000B4739 0C26180F 00000000 00000000
0661E0A1: 0C000035 00000000 64040409 000B4739 0C26180F 00000000 00000000
048488C4: 0C002C35 00000000 64515119 000B4799 0C26180F 00000000 00000000
044A750D: 0C582C35 00000000 00515110 000B4739 0C2618CF 00000000 00000000
04351781: 0C002C35 00000000 40515119 000B4739 0C26180F 00000000 00000000
02A984D0: 0C002C35 00000000 40515119 000B47F9 0C26180F 00000000 00000000
0121D0A8: 0D422439 00000000 00040400 000B473B 0C2610C9 00000000 00000000

0000000039CEEE06: 0C000035 00000001 00040400 000B4739 0C26180F
0000000011F145DD: 0C582C35 00000000 00515110 000B4739 0C2618CF
000000000DEAA542: 0C582C35 00000000 00515110 000B4739 0C26180F
000000000C8D034E: 0C002C35 00000000 00515110 000B47F9 0C26180F
0000000005599D25: 0C000035 00000001 64040409 000B4739 0C26180F
00000000043FA611: 0C000035 00000000 00040400 000B4739 0C26180F
000000000422A43F: 0C002C35 00000001 40515119 000B4739 0C26180F
0000000003959E1E: 0C002C35 00000001 64515119 000B4799 0C26180F
000000000347D228: 0C000035 00000001 00040400 000B47F9 0C26180F
00000000033A5BC8: 0C002C35 00000001 40515119 000B47F9 0C26180F
0000000002F8A88F: 0D422439 00000000 00040400 000B473B 0C2610C9
*/

#define NUM_TMUS			1

#define RENDERFUNC			render_0c000035_00040400_000b4739_0c26180f
#define FBZCOLORPATH		0x0c000035
#define ALPHAMODE			0x00040400
#define FBZMODE				0x000b4739
#define TEXTUREMODE0		0x0c26180f
#define TEXTUREMODE1		0x00000000
#include "voodblit.c"
#undef TEXTUREMODE1
#undef TEXTUREMODE0
#undef FBZMODE
#undef ALPHAMODE
#undef FBZCOLORPATH
#undef RENDERFUNC

#define RENDERFUNC			render_0c582c35_00515110_000b4739_0c26180f
#define FBZCOLORPATH		0x0c582c35
#define ALPHAMODE			0x00515110
#define FBZMODE				0x000b4739
#define TEXTUREMODE0		0x0c26180f
#define TEXTUREMODE1		0x00000000
#include "voodblit.c"
#undef TEXTUREMODE1
#undef TEXTUREMODE0
#undef FBZMODE
#undef ALPHAMODE
#undef FBZCOLORPATH
#undef RENDERFUNC

#define RENDERFUNC			render_0c000035_64040409_000b4739_0c26180f
#define FBZCOLORPATH		0x0c000035
#define ALPHAMODE			0x64040409
#define FBZMODE				0x000b4739
#define TEXTUREMODE0		0x0c26180f
#define TEXTUREMODE1		0x00000000
#include "voodblit.c"
#undef TEXTUREMODE1
#undef TEXTUREMODE0
#undef FBZMODE
#undef ALPHAMODE
#undef FBZCOLORPATH
#undef RENDERFUNC

#define RENDERFUNC			render_0c002c35_64515119_000b4799_0c26180f
#define FBZCOLORPATH		0x0c002c35
#define ALPHAMODE			0x64515119
#define FBZMODE				0x000b4799
#define TEXTUREMODE0		0x0c26180f
#define TEXTUREMODE1		0x00000000
#include "voodblit.c"
#undef TEXTUREMODE1
#undef TEXTUREMODE0
#undef FBZMODE
#undef ALPHAMODE
#undef FBZCOLORPATH
#undef RENDERFUNC

#define RENDERFUNC			render_0c582c35_00515110_000b4739_0c2618cf
#define FBZCOLORPATH		0x0c582c35
#define ALPHAMODE			0x00515110
#define FBZMODE				0x000b4739
#define TEXTUREMODE0		0x0c2618cf
#define TEXTUREMODE1		0x00000000
#include "voodblit.c"
#undef TEXTUREMODE1
#undef TEXTUREMODE0
#undef FBZMODE
#undef ALPHAMODE
#undef FBZCOLORPATH
#undef RENDERFUNC

#define RENDERFUNC			render_0c002c35_40515119_000b4739_0c26180f
#define FBZCOLORPATH		0x0c002c35
#define ALPHAMODE			0x40515119
#define FBZMODE				0x000b4739
#define TEXTUREMODE0		0x0c26180f
#define TEXTUREMODE1		0x00000000
#include "voodblit.c"
#undef TEXTUREMODE1
#undef TEXTUREMODE0
#undef FBZMODE
#undef ALPHAMODE
#undef FBZCOLORPATH
#undef RENDERFUNC

#undef NUM_TMUS


/*
	Sfrush:

One of these is bad:
       175: 0C000035 00000000 00045119 000B4779 082418DF 00000000 00000000
       175: 0C480035 00000000 00045119 000B4779 082418DF 00000000 00000000
       703: 0C000035 00000000 00045119 000B477B 082410DB 00000000 00000000
       
       108: 0C600C09 00000001 00045119 000B4779 0824101F 0824101F 00000000
       688: 0C600C09 00000001 00045119 000B4779 00000000 00000000 00000000
         1: 0C600C09 00000001 00045119 000B4779 0824101F 082410DF 00000000
        47: 0C600C09 00000001 00045119 000B4779 082410DF 0824101F 00000000
        53: 0C600C09 00000001 00045119 000B4779 082418DF 0824101F 00000000
OK      41: 0C482435 00000001 00045119 000B4379 0824101F 0824101F 00000000
       225: 0C600C09 00000001 00045119 000B4779 0824101F 0824181F 00000000
        18: 0C600C09 00000001 00045119 000B4779 0824181F 0824181F 00000000
        13: 0C600C09 00000001 00045119 000B4779 082418DF 0824181F 00000000
        23: 0C000035 00000001 00045119 000B4779 082418DF 0824181F 00000000
         9: 0C000035 00000001 00045119 000B477B 082410DB 0824181F 00000000
       463: 0C000035 00000001 00045119 000B4779 082410DF 0824181F 00000000
         1: 0C480035 00000001 00045119 000B4779 082418DF 0824181F 00000000
         
       127: 0C000035 00000000 00045119 000B4779 082410DF 0824181F 00000000
        31: 0C480035 00000000 00045119 000B4779 082418DF 0824181F 00000000
       127: 0C000035 00000000 00045119 000B477B 082410DB 0824181F 00000000
       
-----------------------------------------------------------
	
       511: 00000002 00000000 00000000 00000300 00000000
         1: 08000001 00000000 00000000 00000300 00000800
         1: 08000001 00000000 00000000 00000200 08241800
     32353: 0C000035 00000000 00045119 000B4779 082418DF
      5437: 0C480035 00000000 00045119 000B4779 082418DF
     23867: 0C000035 00000000 00045119 000B477B 082410DB
     10655: 0C600C09 00000001 00045119 000B4779 0824101F
     13057: 0C600C09 00000001 00045119 000B4779 00000000
       949: 0C600C09 00000001 00045119 000B4779 082410DF
      2723: 0C600C09 00000001 00045119 000B4779 082418DF
       240: 0C482435 00000001 00045119 000B4379 0824101F
      4166: 0C600C09 00000001 00045119 000B4779 0824181F
       747: 0C000035 00000001 00045119 000B4779 082418DF
       427: 0C000035 00000001 00045119 000B477B 082410DB
     12063: 0C000035 00000001 00045119 000B4779 082410DF
        93: 0C480035 00000001 00045119 000B4779 082418DF
      3949: 0C000035 00000000 00045119 000B4779 082410DF
    470768: 0C600C09 00000000 00045119 000B4779 00000000
    418032: 0C600C09 00000000 00045119 000B4779 0824101F
    130673: 0C600C09 00000000 00045119 000B4779 0824181F
      1857: 0C480035 00000000 00045119 000B477B 00000000
      1891: 0C480035 00000000 00045119 000B4779 082410DF
     92962: 0C482435 00000000 00045119 000B4379 0824101F
    119123: 0C600C09 00000000 00045119 000B4779 082708DF
     33176: 0C600C09 00000000 00045119 000B4779 082418DF
     44448: 0C600C09 00000000 00045119 000B4779 082700DF
      1937: 0C600C09 00000000 00045119 000B4779 0827001F
     36352: 0C600C09 00000000 00045119 000B4779 082410DF
       328: 0C600C09 00000001 00045119 000B4779 082700DF
       659: 0C600C09 00000001 00045119 000B4779 082708DF
        67: 0C480035 00000000 00045119 000B4779 00000000
        
*/


/*

Carnevil:
0000000002B4E96A: 0C002425 00000000 00045119 00034679 0C26180F 00000000 00000000
0000000002885479: 0C002435 00000000 04045119 00034279 0C26180F 00000000 00000000
0000000001EE2400: 0C480015 00000000 0F045119 000346F9 0C2618C9 00000000 00000000
0000000001B92D31: 0D422439 00000000 00040400 000B4739 243210C9 00000000 00000000
000000000186F400: 0C000035 00000000 0A045119 000346F9 0C2618C9 00000000 00000000
00000000013C93EE: 0C482415 00000000 0A045119 000346F9 0C26180F 00000000 00000000
000000000139CF3C: 0C482415 00000000 40045119 00034679 0C2618C9 00000000 00000000
00000000013697FC: 0C486116 00000000 01045119 00034279 0C26180F 00000000 00000000
0000000000E4DE5A: 0C482415 00000000 0F045119 000346F9 0C2618C9 00000000 00000000
0000000000DF385B: 0C482415 00000000 04045119 00034279 0C26180F 00000000 00000000
0000000000D02EB3: 0D422409 00000000 00045119 00034679 0C26180F 00000000 00000000

00000000004F8328: 0C002435 00000000 40045119 000B4779 0C26180F 00000000 00000000
000000000000EDA8: 0C002435 00000000 40045119 000B43F9 0C26180F 00000000 00000000
000000000005B736: 0C002435 00000000 40045119 000342F9 0C26180F 00000000 00000000
0000000000A7E85E: 0D422439 00000000 00040400 000B473B 0C2610C9 00000000 00000000
000000000010DCC4: 0D420039 00000000 00040400 000B473B 0C2610C9 00000000 00000000
00000000006525EC: 0C002435 00000000 08045119 000346F9 0C26180F 00000000 00000000
00000000003EFE00: 0C000035 00000000 08045119 000346F9 0C2618C9 00000000 00000000
0000000000086AE4: 0C482415 00000000 05045119 00034679 0C26180F 00000000 00000000
000000000000B1EE: 0C482405 00000000 00045119 00034679 0C26180F 00000000 00000000
00000000003D9446: 0C482415 00000000 04045119 00034279 0C2618C9 00000000 00000000
0000000000BD3AB0: 0C480015 00000000 04045119 000346F9 0C2618C9 00000000 00000000
0000000000001027: 0542611A 00000000 00515119 00034679 0C26180F 00000000 00000000
00000000002C1360: 0C000035 00000000 04045119 00034679 0C2618C9 00000000 00000000
000000000007BAB0: 0C002435 00000000 04045119 00034679 0C2618C9 00000000 00000000
0000000000005360: 0C002425 00000000 04045119 00034679 0C26180F 00000000 00000000
00000000000190D8: 0C482415 00000000 10045119 00034679 0C2618C9 00000000 00000000
0000000000470C50: 0C482415 00000000 05045119 00034279 0C2618C9 00000000 00000000
00000000000A2800: 0C000035 00000000 04040409 00034679 0C2618C9 00000000 00000000
000000000001E814: 0C002435 00000000 04045119 00034279 0C2618C9 00000000 00000000
0000000000146C20: 0C000035 00000000 00045119 00034679 0C2618C9 00000000 00000000
00000000000B6B00: 0C002435 00000000 00045119 00034679 0C2618C9 00000000 00000000
000000000002A05E: 0C482415 00000000 05045119 000346F9 0C26180F 00000000 00000000
00000000001E1C98: 0C002435 00000000 40045119 00034679 0C26180F 00000000 00000000
00000000000198CA: 0C002435 00000000 40045119 000346F9 0C26180F 00000000 00000000

*/

#undef TEXTUREMODE0_MASK
#undef TEXTUREMODE1_MASK
#undef FBZMODE_MASK
#undef ALPHAMODE_MASK
#undef FBZCOLORPATH_MASK

#define FBZCOLORPATH_MASK	0x00000000
#define FBZCOLORPATH		0x00000000
#define ALPHAMODE_MASK		0x00000000
#define ALPHAMODE			0x00000000
#define FBZMODE_MASK		0x00000000
#define FBZMODE				0x00000000
#define TEXTUREMODE0_MASK	0x00000000
#define TEXTUREMODE0		0x00000000
#define TEXTUREMODE1_MASK	0x00000000
#define TEXTUREMODE1		0x00000000

#define RENDERFUNC			generic_render_1tmu
#define NUM_TMUS			1

#include "voodblit.c"

#undef NUM_TMUS
#undef RENDERFUNC
#define RENDERFUNC			generic_render_2tmu
#define NUM_TMUS			2

#include "voodblit.c"

#undef NUM_TMUS
#undef RENDERFUNC

#undef TEXTUREMODE0
#undef TEXTUREMODE0_MASK
#undef TEXTUREMODE1
#undef TEXTUREMODE1_MASK
#undef FBZMODE
#undef FBZMODE_MASK
#undef ALPHAMODE
#undef ALPHAMODE_MASK
#undef FBZCOLORPATH
#undef FBZCOLORPATH_MASK
