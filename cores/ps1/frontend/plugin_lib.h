#ifndef __PLUGIN_LIB_H__
#define __PLUGIN_LIB_H__

#define THREAD_RENDERING_OFF   0
#define THREAD_RENDERING_SYNC  1
#define THREAD_RENDERING_ASYNC 2

enum {
	DKEY_SELECT = 0,
	DKEY_L3,
	DKEY_R3,
	DKEY_START,
	DKEY_UP,
	DKEY_RIGHT,
	DKEY_DOWN,
	DKEY_LEFT,
	DKEY_L2,
	DKEY_R2,
	DKEY_L1,
	DKEY_R1,
	DKEY_TRIANGLE,
	DKEY_CIRCLE,
	DKEY_CROSS,
	DKEY_SQUARE,
};
extern int in_type[8];
extern int multitap1;
extern int multitap2;
extern int in_analog_left[8][2];
extern int in_analog_right[8][2];
extern unsigned short in_keystate[8];
extern int in_mouse[8][2];

extern int in_adev[2], in_adev_axis[2][2];
extern int in_adev_is_nublike[2];
extern int in_enable_vibration;

extern void *pl_vout_buf;

extern int g_layer_x, g_layer_y;
extern int g_layer_w, g_layer_h;

void  pl_start_watchdog(void);
void *pl_prepare_screenshot(int *w, int *h, int *bpp);
void  pl_init(void);
void  pl_switch_dispmode(void);
void  pl_force_clear(void);

void  pl_timing_prepare(int is_pal);
void  pl_frame_limit(void);
void  pl_update_layer_size(int w, int h, int fw, int fh);

// for communication with gpulib
struct rearmed_cbs {
	void  (*pl_get_layer_pos)(int *x, int *y, int *w, int *h);
	int   (*pl_vout_open)(void);
	void  (*pl_vout_set_mode)(int w, int h, int raw_w, int raw_h, int bpp);
	void  (*pl_vout_flip)(const void *vram, int vram_offset, int bgr24,
			      int x, int y, int w, int h, int dims_changed);
	void  (*pl_vout_close)(void);
	void *(*mmap)(unsigned int size);
	void  (*munmap)(void *ptr, unsigned int size);
	// only used by some frontends
	void  (*pl_vout_set_raw_vram)(void *vram);
	void  (*pl_set_gpu_caps)(int caps);
	// emulation related
	void  (*gpu_state_change)(int what, int cycles);
	// some stats, for display by some plugins
	int flips_per_sec, cpu_usage;
	float vsps_cur; // currect vsync/s
	// these are for gles plugin
	unsigned int screen_w, screen_h;
	void *gles_display, *gles_surface;
	// gpu options
	int   frameskip;
	int   fskip_advice;
	int   fskip_force;
	int   fskip_dirty;
	unsigned int *gpu_frame_count;
	unsigned int *gpu_hcnt;
	unsigned int flip_cnt; // increment manually if not using pl_vout_flip
	unsigned int only_16bpp; // platform is 16bpp-only
	unsigned int thread_rendering;
	unsigned int dithering; // 0 off, 1 on, 2 force
	struct {
		int   allow_interlace; // 0 off, 1 on, 2 guess
		int   enhancement_enable;
		int   enhancement_no_main;
		int   enhancement_tex_adj;
	} gpu_neon;
	struct {
		int   dwActFixes;
		float fFrameRateHz;
		int   dwFrameRateTicks;
	} gpu_peops;
	struct {
		int old_renderer;
		int ilace_force;
		int lighting;
		int fast_lighting;
		int blending;
		int scale_hires;
	} gpu_unai;
	struct {
		int   dwActFixes;
		int   bDrawDither, iFilterType, iFrameTexType;
		int   iUseMask, bOpaquePass, bAdvancedBlend, bUseFastMdec;
		int   iVRamSize, iTexGarbageCollection;
	} gpu_peopsgl;
	// misc
	int gpu_caps;
	int screen_centering_type;
	int screen_centering_type_default;
	int screen_centering_x;
	int screen_centering_y;
	int screen_centering_h_adj;
	int show_overscan;
};

extern struct rearmed_cbs pl_rearmed_cbs;

enum centering_type { C_AUTO = 0, C_INGAME, C_BORDERLESS, C_MANUAL };

enum gpu_plugin_caps {
	GPU_CAP_OWNS_DISPLAY = (1 << 0),
	GPU_CAP_SUPPORTS_2X = (1 << 1),
};

// platform hooks
extern void (*pl_plat_clear)(void);
extern void (*pl_plat_blit)(int doffs, const void *src,
			    int w, int h, int sstride, int bgr24);
extern void (*pl_plat_hud_print)(int x, int y, const char *str, int bpp);

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(x) (sizeof(x) / sizeof(x[0]))
#endif

#endif /* __PLUGIN_LIB_H__ */
