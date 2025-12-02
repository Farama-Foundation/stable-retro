/*
 * (C) Gražvydas "notaz" Ignotas, 2011
 *
 * This work is licensed under the terms of any of these licenses
 * (at your option):
 *  - GNU GPL, version 2 or later.
 *  - GNU LGPL, version 2.1 or later.
 * See the COPYING file in the top-level directory.
 */

#ifndef __GPULIB_GPU_H__
#define __GPULIB_GPU_H__

#include <stdint.h>

//#define RAW_FB_DISPLAY

#define gpu_log(gpu, fmt, ...) \
  printf("%d:%03d: " fmt, *(gpu)->state.frame_count, *(gpu)->state.hcnt, ##__VA_ARGS__)

//#define log_anomaly gpu_log
#define log_anomaly(...)

#ifdef __cplusplus
extern "C" {
#endif

#define CMD_BUFFER_LEN          1024

#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
#define HTOLE32(x) __builtin_bswap32(x)
#define HTOLE16(x) __builtin_bswap16(x)
#define LE32TOH(x) __builtin_bswap32(x)
#define LE16TOH(x) __builtin_bswap16(x)
#else
#define HTOLE32(x) (x)
#define HTOLE16(x) (x)
#define LE32TOH(x) (x)
#define LE16TOH(x) (x)
#endif

#undef BIT
#define BIT(x) (1u << (x))

#define PSX_GPU_STATUS_DHEIGHT		BIT(19)
#define PSX_GPU_STATUS_PAL		BIT(20)
#define PSX_GPU_STATUS_RGB24		BIT(21)
#define PSX_GPU_STATUS_INTERLACE	BIT(22)
#define PSX_GPU_STATUS_BLANKING		BIT(23)
#define PSX_GPU_STATUS_IMG			BIT(27)
#define PSX_GPU_STATUS_DMA(x)		((x) << 29)
#define PSX_GPU_STATUS_DMA_MASK		(BIT(29) | BIT(30))

struct psx_gpu {
  uint32_t regs[16];
  uint16_t *vram;
  uint32_t status;
  uint32_t gp0;
  uint32_t ex_regs[8];
  struct {
    int hres, vres;
    int x, y, w, h;
    int x1, x2;
    int y1, y2;
    int src_x, src_y;
  } screen;
  struct {
    int x, y, w, h;
    short int offset, is_read;
  } dma, dma_start;
  int cmd_len;
  uint32_t zero;
  struct {
    uint32_t fb_dirty:1;
    uint32_t old_interlace:1;
    uint32_t allow_interlace:2;
    uint32_t blanked:1;
    uint32_t enhancement_enable:1;
    uint32_t enhancement_active:1;
    uint32_t enhancement_was_active:1;
    uint32_t downscale_enable:1;
    uint32_t downscale_active:1;
    uint32_t dims_changed:1;
    uint32_t show_overscan:2;
    uint32_t *frame_count;
    uint32_t *hcnt; /* hsync count */
    struct {
      uint32_t addr;
      uint32_t cycles;
      uint32_t frame;
      uint32_t hcnt;
    } last_list;
    uint32_t last_vram_read_frame;
    uint32_t w_out_old, h_out_old, status_vo_old;
    short screen_centering_type;
    short screen_centering_type_default;
    short screen_centering_x;
    short screen_centering_y;
    int screen_centering_h_adj;
  } state;
  struct {
    int32_t set:3; /* -1 auto, 0 off, 1-3 fixed */
    int32_t cnt:3; /* amount skipped in a row */
    uint32_t active:1;
    uint32_t allow:1;
    uint32_t frame_ready:1;
    const int *advice;
    const int *force;
    int *dirty;
    uint32_t last_flip_frame;
    uint32_t pending_fill[3];
  } frameskip;
  uint32_t scratch_ex_regs[8]; // for threaded rendering
  uint32_t cmd_buffer[CMD_BUFFER_LEN];
  void *(*get_enhancement_bufer)
    (int *x, int *y, int *w, int *h, int *vram_h);
  uint16_t *(*get_downscale_buffer)
    (int *x, int *y, int *w, int *h, int *vram_h);
  void *(*mmap)(unsigned int size);
  void  (*munmap)(void *ptr, unsigned int size);
  void  (*gpu_state_change)(int what, int cycles); // psx_gpu_state
};

extern struct psx_gpu gpu;

extern const unsigned char cmd_lengths[256];

int do_cmd_list(uint32_t *list, int count,
	int *cycles_sum, int *cycles_last, int *last_cmd);

struct rearmed_cbs;

int  renderer_init(void);
void renderer_finish(void);
void renderer_sync_ecmds(uint32_t * ecmds);
void renderer_update_caches(int x, int y, int w, int h, int state_changed);
void renderer_flush_queues(void);
void renderer_set_interlace(int enable, int is_odd);
void renderer_set_config(const struct rearmed_cbs *config);
void renderer_notify_res_change(void);
void renderer_notify_update_lace(int updated);
void renderer_sync(void);
void renderer_notify_scanout_change(int x, int y);

int  vout_init(void);
int  vout_finish(void);
int  vout_update(void);
void vout_blank(void);
void vout_set_config(const struct rearmed_cbs *config);

int  prim_try_simplify_quad_t (void *simplified, const void *prim);
int  prim_try_simplify_quad_gt(void *simplified, const void *prim);

/* listing these here for correct linkage if rasterizer uses c++ */
struct GPUFreeze;

long GPUinit(void);
long GPUshutdown(void);
void GPUwriteDataMem(uint32_t *mem, int count);
long GPUdmaChain(uint32_t *rambase, uint32_t addr,
		uint32_t *progress_addr, int32_t *cycles_last_cmd);
void GPUwriteData(uint32_t data);
void GPUreadDataMem(uint32_t *mem, int count);
uint32_t GPUreadData(void);
uint32_t GPUreadStatus(void);
void GPUwriteStatus(uint32_t data);
long GPUfreeze(uint32_t type, struct GPUFreeze *freeze);
void GPUupdateLace(void);
long GPUopen(unsigned long *disp, char *cap, char *cfg);
long GPUclose(void);
void GPUvBlank(int is_vblank, int lcf);
void GPUgetScreenInfo(int *y, int *base_hres);
void GPUrearmedCallbacks(const struct rearmed_cbs *cbs_);

#ifdef __cplusplus
}
#endif

#endif /* __GPULIB_GPU_H__ */
