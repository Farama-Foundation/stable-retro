#include <math.h>

#include "../Glitch64/glide.h"
#include "Combine.h"
#include "rdp.h"
#include "../../Graphics/GBI.h"
#include "../../Graphics/RSP/gSP_funcs_C.h"
#include "../../Graphics/RSP/RSP_state.h"

typedef struct DRAWOBJECT_t
{
  float objX;
  float objY;
  float scaleW;
  float scaleH;
  int16_t imageW;
  int16_t imageH;

  uint16_t  imageStride;
  uint16_t  imageAdrs;
  uint8_t  imageFmt;
  uint8_t  imageSiz;
  uint8_t  imagePal;
  uint8_t  imageFlags;
} DRAWOBJECT;

/* forward decls */

static void uc6_draw_polygons (VERTEX v[4]);
static void uc6_read_object_data (DRAWOBJECT *d);
static void uc6_init_tile(const DRAWOBJECT *d);
extern int dzdx;
extern int deltaZ;

void cull_trianglefaces(VERTEX **v, unsigned iterations, bool do_update, bool do_cull, int32_t wd);
void glide64gSPLightVertex(void *data);
void glide64gSPDMATriangles(uint32_t tris, uint32_t n);
void glide64gSPSetDMAOffsets(uint32_t mtxoffset, uint32_t vtxoffset);
void glide64gSPDMAMatrix(uint32_t matrix, uint8_t index, uint8_t multiply);
void glide64gSPDMAVertex(uint32_t v, uint32_t n, uint32_t v0);
void glide64gSPCBFDVertex(uint32_t a, uint32_t n, uint32_t v0);
void glide64gSPObjLoadTxtr(uint32_t tx);
void glide64gSPCoordMod(uint32_t w0, uint32_t w1);
void glide64gSPLightCBFD(uint32_t l, int32_t n);
void glide64gSPSetVertexNormaleBase(uint32_t base);
/* end forward decls */

