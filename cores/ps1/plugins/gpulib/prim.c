#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../../include/compiler_features.h"
#include "gpu.h"

// retain neon's ability to sample textures pixel-perfectly
#ifdef GPU_NEON
#define STRICT
#endif

struct vert_t
{
  union {
    struct {
      int16_t x, y;
    };
    uint32_t xy;
  };
  union {
    struct {
      uint8_t u, v;
      int16_t clut;
    };
    uint32_t uvclut;
  };
};

// gt ~ gouraud textured
struct vert_gt
{
  uint32_t rgb;
  struct vert_t t;
};

struct quad_t
{
  uint32_t rgb_c;
  struct vert_t v[4];
};

struct quad_gt
{
  struct vert_gt v[4];
};

struct sprite
{
  uint32_t rgb_c;
  union {
    struct {
      int16_t x, y;
    };
    uint32_t xy;
  };
  union {
    struct {
      uint8_t u, v;
      int16_t clut;
    };
    uint32_t uvclut;
  };
  int16_t w, h;
};

// debug
#if 0
static void log_quad_t(const struct quad_t *q, int ret)
{
#if 1
  printf("quad_t %08x", q->rgb_c);
  int i;
  for (i = 0; i < 4; i++)
    printf(" | %3d,%3d %3d,%3d",
        q->v[i].x, q->v[i].y, q->v[i].u, q->v[i].v);
  printf(" -> %d\n", ret);
#endif
}

static void log_quad_gt(const struct vert_gt *v, int ret)
{
#if 1
  printf("quad_gt %02x", v[0].rgb >> 24);
  int i;
  for (i = 0; i < 4; i++)
    printf(" | %3d,%3d %3d,%3d %06x",
        v[i].t.x, v[i].t.y, v[i].t.u, v[i].t.v, v[i].rgb & 0xffffff);
  printf(" -> %d\n", ret);
#endif
}

int prim_try_simplify_quad_t_(void *simplified, const void *prim_);
int prim_try_simplify_quad_t(void *simplified, const void *prim_)
{
  struct quad_t prim = *(struct quad_t *)prim_;
  int ret = prim_try_simplify_quad_t_(simplified, prim_);
  #define prim_try_simplify_quad_t prim_try_simplify_quad_t_
  ///if (!ret)
    log_quad_t(&prim, ret);
  return ret;
}

int prim_try_simplify_quad_gt_(void *simplified, const void *prim_);
int prim_try_simplify_quad_gt(void *simplified, const void *prim_)
{
  struct quad_gt prim = *(struct quad_gt *)prim_;
  int ret = prim_try_simplify_quad_gt_(simplified, prim_);
  #define prim_try_simplify_quad_gt prim_try_simplify_quad_gt_
  ///if (!ret)
    log_quad_gt(prim.v, ret);
  return ret;
}
#endif // debug

static noinline int simplify_quad_t(void *simplified, const struct vert_t *v,
  int xd, int ud, int yd, int vd, uint32_t rgb_c, uint16_t clut)
{
  struct sprite *s = simplified;
  int ret = 1;
  rgb_c &= HTOLE32(0x03ffffff);
  rgb_c |= HTOLE32(0x64000000);
  xd = abs(xd);
  ud = abs(ud);
  s[0].rgb_c = rgb_c;
  s[0].xy = v->xy;
  s[0].u = v->u;
  s[0].v = v->v;
  s[0].clut = clut;
  s[0].w = HTOLE16(xd);
  s[0].h = HTOLE16(yd);
#ifndef STRICT
  if (xd != ud) {
    int mid = xd / 2;
    s[0].w = HTOLE16(mid);
    s[1].rgb_c = rgb_c;
    s[1].x = HTOLE16(LE16TOH(s[0].x) + mid);
    s[1].y = s[0].y;
    s[1].u = s[0].u + mid + ud - xd;
    s[1].v = s[0].v;
    s[1].clut = clut;
    s[1].w = HTOLE16(xd - mid);
    s[1].h = s[0].h;
    ret = 2;
  }
  if (yd != vd) {
    int i, mid = yd / 2, y = LE16TOH(s[0].y);
    memcpy(s + ret, s, sizeof(s[0]) * ret);
    for (i = 0; i < ret; i++) {
      s[i].h = HTOLE16(mid);
      s[ret+i].y = HTOLE16(y + mid);
      s[ret+i].h = HTOLE16(yd - mid);
      s[ret+i].v = s[0].v + mid + vd - yd;
    }
    ret *= 2;
  }
#endif
  return ret;
}

// this is split to reduce gcc spilling
static noinline int prim_try_simplify_quad_t2(void *simplified,
  const struct vert_t *v, uint32_t rgb_c)
{
  do {
    int yd = LE16TOH(v[2].y) - LE16TOH(v[0].y);
    int xd, ud, vd;
    if (yd < 0)
      break;
    xd = LE16TOH(v[1].x) - LE16TOH(v[0].x);
    ud = LE16TOH(v[1].u) - LE16TOH(v[0].u);
    vd = LE16TOH(v[2].v) - LE16TOH(v[0].v);
#ifdef STRICT
    if (xd != ud || yd != vd)
#else
    if (abs(xd - ud) > 1 || abs(yd - vd) > 1)
#endif
      break;
    return simplify_quad_t(simplified, xd < 0 ? &v[1] : &v[0],
             xd, ud, yd, vd, rgb_c, v[0].clut);
  }
  while (0);
  return 0;
}

static noinline int prim_try_simplify_quad_gt2(void *simplified,
  const struct vert_gt *v)
{
  do {
    int yd = LE16TOH(v[2].t.y) - LE16TOH(v[0].t.y);
    int xd, ud, vd;
    if (yd < 0)
      break;
    xd = LE16TOH(v[1].t.x) - LE16TOH(v[0].t.x);
    ud = LE16TOH(v[1].t.u) - LE16TOH(v[0].t.u);
    vd = LE16TOH(v[2].t.v) - LE16TOH(v[0].t.v);
#ifdef STRICT
    if (xd != ud || yd != vd)
#else
    if (abs(xd - ud) > 1 || abs(yd - vd) > 1)
#endif
      break;
    if (!(v[0].rgb & HTOLE32(1 << 24))) { // modulation/"lighting"
      uint32_t i, xor = 0, rgb0 = v[0].rgb;
      for (i = 1; i < 4; i++)
        xor |= rgb0 ^ v[i].rgb;
      if (xor & HTOLE32(0xf8f8f8))
        break;
    }
    return simplify_quad_t(simplified, xd < 0 ? &v[1].t : &v[0].t,
        xd, ud, yd, vd, v[0].rgb, v[0].t.clut);
  }
  while (0);
  return 0;
}

// 2c-2f
int prim_try_simplify_quad_t(void *simplified, const void *prim_)
{
  const struct quad_t *prim = prim_;
  const struct vert_t *v = prim->v;
  int ret = 0;
  do {
    if (v[0].y != v[1].y || v[0].x != v[2].x || v[2].y != v[3].y || v[1].x != v[3].x)
      break;
    if (v[0].v != v[1].v || v[0].u != v[2].u || v[2].v != v[3].v || v[1].u != v[3].u)
      break;
    ret = prim_try_simplify_quad_t2(simplified, v, prim->rgb_c);
  }
  while (0);
  return ret;
}

// 3c-3f
int prim_try_simplify_quad_gt(void *simplified, const void *prim)
{
  const struct vert_gt *v = prim;
  int ret = 0;
  do {
    if (v[0].t.y != v[1].t.y || v[0].t.x != v[2].t.x || v[2].t.y != v[3].t.y || v[1].t.x != v[3].t.x)
      break;
    if (v[0].t.v != v[1].t.v || v[0].t.u != v[2].t.u || v[2].t.v != v[3].t.v || v[1].t.u != v[3].t.u)
      break;
    ret = prim_try_simplify_quad_gt2(simplified, v);
  }
  while (0);
  return ret;
}

// vim:shiftwidth=2:expandtab
