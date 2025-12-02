/*
 * (C) notaz, 2010-2011
 *
 * This work is licensed under the terms of the GNU GPLv2 or later.
 * See the COPYING file in the top-level directory.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdint.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>
#include <assert.h>

#include "libpicofe/fonts.h"
#include "libpicofe/input.h"
#include "libpicofe/plat.h"
#include "libpicofe/arm/neon_scale2x.h"
#include "libpicofe/arm/neon_eagle2x.h"
#include "plugin_lib.h"
#include "menu.h"
#include "main.h"
#include "plat.h"
#include "pcnt.h"
#include "pl_gun_ts.h"
#include "cspace.h"
#include "psemu_plugin_defs.h"
#include "../libpcsxcore/new_dynarec/new_dynarec.h"
#include "../libpcsxcore/psxmem_map.h"
#include "../libpcsxcore/gpu.h"
#include "../libpcsxcore/r3000a.h"
#include "../libpcsxcore/psxcounters.h"
#include "arm_features.h"

#define HUD_HEIGHT 10

int in_type[8];
int multitap1;
int multitap2;
int in_analog_left[8][2] = {{ 127, 127 },{ 127, 127 },{ 127, 127 },{ 127, 127 },{ 127, 127 },{ 127, 127 },{ 127, 127 },{ 127, 127 }};
int in_analog_right[8][2] = {{ 127, 127 },{ 127, 127 },{ 127, 127 },{ 127, 127 },{ 127, 127 },{ 127, 127 },{ 127, 127 },{ 127, 127 }};
int in_adev[2] = { -1, -1 };
int in_adev_axis[2][2] =
#ifdef PANDORA
  {{ 0, 1 }, { 0, 1 }};
#else
  {{ 0, 1 }, { 2, 3 }};
#endif
int in_adev_is_nublike[2];
unsigned short in_keystate[8];
int in_mouse[8][2];
int in_enable_vibration;
void *tsdev;
void *pl_vout_buf;
int g_layer_x, g_layer_y, g_layer_w, g_layer_h;
static int pl_vout_w, pl_vout_h, pl_vout_bpp; /* output display/layer */
static int pl_vout_scale_w, pl_vout_scale_h;
static int psx_w, psx_h, psx_bpp;
static int vsync_cnt;
static int is_pal, frame_interval, frame_interval1024;
static int vsync_usec_time;

// platform hooks
void (*pl_plat_clear)(void);
void (*pl_plat_blit)(int doffs, const void *src, int w, int h,
		     int sstride, int bgr24);
void (*pl_plat_hud_print)(int x, int y, const char *str, int bpp);


static __attribute__((noinline)) int get_cpu_ticks(void)
{
	static unsigned long last_utime;
	static int fd;
	unsigned long utime, ret;
	char buf[128];

	if (fd == 0)
		fd = open("/proc/self/stat", O_RDONLY);
	lseek(fd, 0, SEEK_SET);
	buf[0] = 0;
	read(fd, buf, sizeof(buf));
	buf[sizeof(buf) - 1] = 0;

	sscanf(buf, "%*d %*s %*c %*d %*d %*d %*d %*d %*u %*u %*u %*u %*u %lu", &utime);
	ret = utime - last_utime;
	if (ret > 200)
		ret = 0;
	last_utime = utime;
	return ret;
}

static void hud_print(void *fb, int w, int x, int y, const char *text)
{
	if (pl_plat_hud_print)
		pl_plat_hud_print(x, y, text, pl_vout_bpp);
	else if (pl_vout_bpp == 16)
		basic_text_out16_nf(fb, w, x, y, text);
}

static void hud_printf(void *fb, int w, int x, int y, const char *texto, ...)
{
	va_list args;
	char    buffer[256];

	va_start(args, texto);
	vsnprintf(buffer, sizeof(buffer), texto, args);
	va_end(args);

	hud_print(fb, w, x, y, buffer);
}

static void print_msg(int h, int border)
{
	hud_print(pl_vout_buf, pl_vout_w, border + 2, h - HUD_HEIGHT, hud_msg);
}

static void print_fps(int h, int border)
{
	hud_printf(pl_vout_buf, pl_vout_w, border + 2, h - HUD_HEIGHT,
		"%2d %4.1f", pl_rearmed_cbs.flips_per_sec,
		pl_rearmed_cbs.vsps_cur);
}

static void print_cpu_usage(int x, int h)
{
	hud_printf(pl_vout_buf, pl_vout_w, x - 28,
		h - HUD_HEIGHT, "%3d", pl_rearmed_cbs.cpu_usage);
}

// draw 192x8 status of 24 sound channels
static __attribute__((noinline)) void draw_active_chans(int vout_w, int vout_h)
{
	extern void spu_get_debug_info(int *chans_out, int *run_chans,
		int *fmod_chans_out, int *noise_chans_out); // hack
	int live_chans, run_chans, fmod_chans, noise_chans;

	static const unsigned short colors[2] = { 0x1fe3, 0x0700 };
	unsigned short *dest = (unsigned short *)pl_vout_buf +
		pl_vout_w * (vout_h - HUD_HEIGHT) + pl_vout_w / 2 - 192/2;
	unsigned short *d, p;
	int c, x, y;

	if (pl_vout_buf == NULL || pl_vout_bpp != 16)
		return;

	spu_get_debug_info(&live_chans, &run_chans, &fmod_chans, &noise_chans);

	for (c = 0; c < 24; c++) {
		d = dest + c * 8;
		p = !(live_chans & (1<<c)) ? (run_chans & (1<<c) ? 0x01c0 : 0) :
		     (fmod_chans & (1<<c)) ? 0xf000 :
		     (noise_chans & (1<<c)) ? 0x001f :
		     colors[c & 1];
		for (y = 0; y < 8; y++, d += pl_vout_w)
			for (x = 0; x < 8; x++)
				d[x] = p;
	}
}

static void print_hud(int x, int w, int h)
{
	if (h < 192)
		return;

	if (h > pl_vout_h)
		h = pl_vout_h;

	if (g_opts & OPT_SHOWSPU)
		draw_active_chans(w, h);

	if (hud_msg[0] != 0)
		print_msg(h, x);
	else if (g_opts & OPT_SHOWFPS)
		print_fps(h, x);

	if (g_opts & OPT_SHOWCPU)
		print_cpu_usage(x + w, h);
}

/* update scaler target size according to user settings */
void pl_update_layer_size(int w, int h, int fw, int fh)
{
	float mult;
	int imult;

	switch (g_scaler) {
	case SCALE_1_1:
		g_layer_w = w; g_layer_h = h;
		break;

	case SCALE_2_2:
		g_layer_w = w; g_layer_h = h;
		if (w * 2 <= fw)
			g_layer_w = w * 2;
		if (h * 2 <= fh)
			g_layer_h = h * 2;
		break;

	case SCALE_4_3v2:
#ifdef PANDORA
		if (h <= fh && !(240 < h && h <= 360))
		{
#endif

		// 4:3 that prefers integer scaling
		imult = fh / h;
		if (imult < 1)
			imult = 1;
		g_layer_w = w * imult;
		g_layer_h = h * imult;
		mult = (float)g_layer_w / (float)g_layer_h;
		if (mult < 1.25f || mult > 1.666f)
			g_layer_w = 4.0f/3.0f * (float)g_layer_h;
		//printf("  -> %dx%d %.1f\n", g_layer_w, g_layer_h, mult);
		break;
#ifdef PANDORA
		}
#endif

	case SCALE_4_3:
		mult = 240.0f / (float)h * 4.0f / 3.0f;
		if (h > 256)
			mult *= 2.0f;
		g_layer_w = mult * (float)fh;
		g_layer_h = fh;
		//printf("  -> %dx%d %.1f\n", g_layer_w, g_layer_h, mult);
		break;

	case SCALE_FULLSCREEN:
		g_layer_w = fw;
		g_layer_h = fh;
		break;

	default:
		break;
	}

	if (g_scaler != SCALE_CUSTOM) {
		g_layer_x = fw / 2 - g_layer_w / 2;
		g_layer_y = fh / 2 - g_layer_h / 2;
	}
	if (g_layer_w > fw * 2) g_layer_w = fw * 2;
	if (g_layer_h > fh * 2) g_layer_h = fh * 2;
}

// XXX: this is platform specific really
static inline int resolution_ok(int w, int h)
{
	return w <= 1024 && h <= 512;
}

static void pl_vout_set_mode(int w, int h, int raw_w, int raw_h, int bpp)
{
	int vout_w, vout_h, vout_bpp;

	// special h handling, Wipeout likes to change it by 1-6
	static int vsync_cnt_ms_prev;
	if ((unsigned int)(vsync_cnt - vsync_cnt_ms_prev) < 5*60)
		h = (h + 7) & ~7;
	vsync_cnt_ms_prev = vsync_cnt;

	psx_w = raw_w;
	psx_h = raw_h;
	psx_bpp = bpp;
	vout_w = w;
	vout_h = h;
	vout_bpp = bpp;
	if (pl_rearmed_cbs.only_16bpp)
		vout_bpp = 16;

	assert(vout_h >= 192);

	pl_vout_scale_w = pl_vout_scale_h = 1;
#ifdef HAVE_NEON32
	if (soft_filter) {
		if (resolution_ok(w * 2, h * 2) && bpp == 16) {
			pl_vout_scale_w = 2;
			pl_vout_scale_h = 2;
		}
		else {
			// filter unavailable
			hud_msg[0] = 0;
		}
	}
	else if (scanlines != 0 && scanline_level != 100 && bpp == 16) {
		if (h <= 256)
			pl_vout_scale_h = 2;
	}
#endif
	vout_w *= pl_vout_scale_w;
	vout_h *= pl_vout_scale_h;

	pl_update_layer_size(vout_w, vout_h, g_menuscreen_w, g_menuscreen_h);

	pl_vout_buf = plat_gvideo_set_mode(&vout_w, &vout_h, &vout_bpp);
	if (pl_vout_buf == NULL && pl_plat_blit == NULL)
		fprintf(stderr, "failed to set mode %dx%d@%d\n",
			vout_w, vout_h, vout_bpp);
	else {
		pl_vout_w = vout_w;
		pl_vout_h = vout_h;
		pl_vout_bpp = vout_bpp;
	}

	menu_notify_mode_change(pl_vout_w, pl_vout_h, pl_vout_bpp);
}

static int flip_clear_counter;

void pl_force_clear(void)
{
	flip_clear_counter = 2;
}

static void pl_vout_flip(const void *vram_, int vram_ofs, int bgr24,
	int x, int y, int w, int h, int dims_changed)
{
	unsigned char *dest = pl_vout_buf;
	const unsigned char *vram = vram_;
	int dstride = pl_vout_w, h1 = h;
	int h_full = pl_vout_h;
	int enhres = w > psx_w;
	int xoffs = 0, doffs;
	int hwrapped;

	pcnt_start(PCNT_BLIT);

	if (vram == NULL) {
		// blanking
		if (pl_plat_clear)
			pl_plat_clear();
		else
			memset(pl_vout_buf, 0,
				dstride * h_full * pl_vout_bpp / 8);
		goto out_hud;
	}

	assert(x + w <= pl_vout_w);
	assert(y + h <= pl_vout_h);

	// offset
	xoffs = x * pl_vout_scale_w;
	doffs = xoffs + y * pl_vout_scale_h * dstride;

	if (dims_changed)
		flip_clear_counter = 3;

	if (flip_clear_counter > 0) {
		if (pl_plat_clear)
			pl_plat_clear();
		else
			memset(pl_vout_buf, 0,
				dstride * h_full * pl_vout_bpp / 8);
		flip_clear_counter--;
	}

	if (pl_plat_blit)
	{
		pl_plat_blit(doffs, vram + vram_ofs, w, h, 1024, bgr24);
		goto out_hud;
	}

	if (dest == NULL)
		goto out;

	dest += doffs * 2;

	if (bgr24)
	{
		hwrapped = (vram_ofs & 2047) + w * 3 - 2048;
		if (pl_rearmed_cbs.only_16bpp) {
			for (; h1-- > 0; dest += dstride * 2) {
				bgr888_to_rgb565(dest, vram + vram_ofs, w * 3);
				vram_ofs = (vram_ofs + 2048) & 0xfffff;
			}

			if (hwrapped > 0) {
				// this is super-rare so just fix-up
				vram_ofs = (vram_ofs - h * 2048) & 0xff800;
				dest -= dstride * 2 * h;
				dest += (w - hwrapped / 3) * 2;
				for (h1 = h; h1-- > 0; dest += dstride * 2) {
					bgr888_to_rgb565(dest, vram + vram_ofs, hwrapped);
					vram_ofs = (vram_ofs + 2048) & 0xfffff;
				}
			}
		}
		else {
			dest -= doffs * 2;
			dest += (doffs / 8) * 24;

			for (; h1-- > 0; dest += dstride * 3) {
				bgr888_to_rgb888(dest, vram + vram_ofs, w * 3);
				vram_ofs = (vram_ofs + 2048) & 0xfffff;
			}

			if (hwrapped > 0) {
				vram_ofs = (vram_ofs - h * 2048) & 0xff800;
				dest -= dstride * 3 * h;
				dest += w * 3 - hwrapped;
				for (h1 = h; h1-- > 0; dest += dstride * 3) {
					bgr888_to_rgb888(dest, vram + vram_ofs, hwrapped);
					vram_ofs = (vram_ofs + 2048) & 0xfffff;
				}
			}
		}
	}
#ifdef HAVE_NEON32
	else if (soft_filter == SOFT_FILTER_SCALE2X && pl_vout_scale_w == 2)
	{
		neon_scale2x_16_16((const void *)(vram + vram_ofs), (void *)dest, w,
			2048, dstride * 2, h);
	}
	else if (soft_filter == SOFT_FILTER_EAGLE2X && pl_vout_scale_w == 2)
	{
		neon_eagle2x_16_16((const void *)(vram + vram_ofs), (void *)dest, w,
			2048, dstride * 2, h);
	}
	else if (scanlines != 0 && scanline_level != 100)
	{
		int h2, l = scanline_level * 2048 / 100;
		int stride_0 = pl_vout_scale_h >= 2 ? 0 : 2048;

		h1 *= pl_vout_scale_h;
		while (h1 > 0)
		{
			for (h2 = scanlines; h2 > 0 && h1 > 0; h2--, h1--) {
				bgr555_to_rgb565(dest, vram + vram_ofs, w * 2);
				vram_ofs = (vram_ofs + stride_0) & 0xfffff;
				dest += dstride * 2;
			}

			for (h2 = scanlines; h2 > 0 && h1 > 0; h2--, h1--) {
				bgr555_to_rgb565_b(dest, vram + vram_ofs, w * 2, l);
				vram_ofs = (vram_ofs + 2048) & 0xfffff;
				dest += dstride * 2;
			}
		}
	}
#endif
	else
	{
		unsigned int vram_mask = enhres ? ~0 : 0xfffff;
		for (; h1-- > 0; dest += dstride * 2) {
			bgr555_to_rgb565(dest, vram + vram_ofs, w * 2);
			vram_ofs = (vram_ofs + 2048) & vram_mask;
		}

		hwrapped = (vram_ofs & 2047) + w * 2 - 2048;
		if (!enhres && hwrapped > 0) {
			vram_ofs = (vram_ofs - h * 2048) & 0xff800;
			dest -= dstride * 2 * h;
			dest += w * 2 - hwrapped;
			for (h1 = h; h1-- > 0; dest += dstride * 2) {
				bgr555_to_rgb565(dest, vram + vram_ofs, hwrapped);
				vram_ofs = (vram_ofs + 2048) & 0xfffff;
			}
		}
	}

out_hud:
	print_hud(xoffs, w * pl_vout_scale_w, (y + h) * pl_vout_scale_h);

out:
	pcnt_end(PCNT_BLIT);

	// let's flip now
	pl_vout_buf = plat_gvideo_flip();

	pl_rearmed_cbs.flip_cnt++;
}

static int pl_vout_open(void)
{
	struct timeval now;

	// force mode update on pl_vout_set_mode() call from gpulib/vout_pl
	pl_vout_buf = NULL;

	plat_gvideo_open(is_pal);

	gettimeofday(&now, 0);
	vsync_usec_time = now.tv_usec;
	while (vsync_usec_time >= frame_interval)
		vsync_usec_time -= frame_interval;

	return 0;
}

static void pl_vout_close(void)
{
	plat_gvideo_close();
}

static void pl_set_gpu_caps(int caps)
{
	pl_rearmed_cbs.gpu_caps = caps;
}

void *pl_prepare_screenshot(int *w, int *h, int *bpp)
{
	void *ret = plat_prepare_screenshot(w, h, bpp);
	if (ret != NULL)
		return ret;

	*w = pl_vout_w;
	*h = pl_vout_h;
	*bpp = pl_vout_bpp;

	return pl_vout_buf;
}

/* display/redering mode switcher */
static int dispmode_default(void)
{
	pl_rearmed_cbs.gpu_neon.enhancement_enable = 0;
	soft_filter = SOFT_FILTER_NONE;
	snprintf(hud_msg, sizeof(hud_msg), "default mode");
	return 1;
}

static int dispmode_doubleres(void)
{
	if (!(pl_rearmed_cbs.gpu_caps & GPU_CAP_SUPPORTS_2X)
	    || !resolution_ok(psx_w * 2, psx_h * 2) || psx_bpp != 16)
		return 0;

	dispmode_default();
	pl_rearmed_cbs.gpu_neon.enhancement_enable = 1;
	snprintf(hud_msg, sizeof(hud_msg), "double resolution");
	return 1;
}

#ifdef HAVE_NEON32
static int dispmode_scale2x(void)
{
	if (!resolution_ok(psx_w * 2, psx_h * 2) || psx_bpp != 16)
		return 0;

	dispmode_default();
	soft_filter = SOFT_FILTER_SCALE2X;
	snprintf(hud_msg, sizeof(hud_msg), "scale2x");
	return 1;
}

static int dispmode_eagle2x(void)
{
	if (!resolution_ok(psx_w * 2, psx_h * 2) || psx_bpp != 16)
		return 0;

	dispmode_default();
	soft_filter = SOFT_FILTER_EAGLE2X;
	snprintf(hud_msg, sizeof(hud_msg), "eagle2x");
	return 1;
}
#endif

static int (*dispmode_switchers[])(void) = {
	dispmode_default,
	dispmode_doubleres,
#ifdef HAVE_NEON32
	dispmode_scale2x,
	dispmode_eagle2x,
#endif
};

static int dispmode_current;

void pl_switch_dispmode(void)
{
	if (pl_rearmed_cbs.gpu_caps & GPU_CAP_OWNS_DISPLAY)
		return;

	while (1) {
		dispmode_current++;
		if (dispmode_current >=
		    sizeof(dispmode_switchers) / sizeof(dispmode_switchers[0]))
			dispmode_current = 0;
		if (dispmode_switchers[dispmode_current]())
			break;
	}
}

#ifndef MAEMO
/* adjust circle-like analog inputs to better match
 * more square-like analogs in PSX */
static void update_analog_nub_adjust(int *x_, int *y_)
{
	#define d 16
	static const int scale[] =
		{ 0 - d*2,  0 - d*2,  0 - d*2, 12 - d*2,
		 30 - d*2, 60 - d*2, 75 - d*2, 60 - d*2, 60 - d*2 };
	int x = abs(*x_);
	int y = abs(*y_);
	int scale_x = scale[y / 16];
	int scale_y = scale[x / 16];

	if (x) {
		x += d + (x * scale_x >> 8);
		if (*x_ < 0)
			x = -x;
	}
	if (y) {
		y += d + (y * scale_y >> 8);
		if (*y_ < 0)
			y = -y;
	}

	*x_ = x;
	*y_ = y;
	#undef d
}

static void update_analogs(void)
{
	int *nubp[2] = { in_analog_left[0], in_analog_right[0] };
	int vals[2];
	int i, a, v, ret;

	for (i = 0; i < 2; i++)
	{
		if (in_adev[i] < 0)
			continue;

		for (a = 0; a < 2; a++) {
			vals[a] = 0;

			ret = in_update_analog(in_adev[i], in_adev_axis[i][a], &v);
			if (ret == 0)
				vals[a] = 128 * v / IN_ABS_RANGE;
		}

		if (in_adev_is_nublike[i])
			update_analog_nub_adjust(&vals[0], &vals[1]);

		for (a = 0; a < 2; a++) {
			v = vals[a] + 127;
			if (v < 0) v = 0;
			else if (v > 255) v = 255;
			nubp[i][a] = v;
		}

	}
}

static void emu_set_action(enum sched_action action_)
{
	extern enum sched_action emu_action, emu_action_old;

	if (action_ == SACTION_NONE)
		emu_action_old = 0;
	else if (action_ != emu_action_old)
		psxRegs.stop++;
	emu_action = action_;
}

static void update_input(void)
{
	int actions[IN_BINDTYPE_COUNT] = { 0, };
	unsigned int emu_act;
	int in_state_gun;
	int i;

	in_update(actions);
	if (in_type[0] == PSE_PAD_TYPE_ANALOGJOY || in_type[0] == PSE_PAD_TYPE_ANALOGPAD)
		update_analogs();
	emu_act = actions[IN_BINDTYPE_EMU];
	in_state_gun = emu_act & SACTION_GUN_MASK;

	emu_act &= ~SACTION_GUN_MASK;
	if (emu_act) {
		int which = 0;
		for (; !(emu_act & 1); emu_act >>= 1, which++)
			;
		emu_act = which;
	}
	emu_set_action(emu_act);

	in_keystate[0] = actions[IN_BINDTYPE_PLAYER12] & 0xffff;
	in_keystate[1] = (actions[IN_BINDTYPE_PLAYER12] >> 16) & 0xffff;

	if (tsdev) for (i = 0; i < 2; i++) {
		int in = 0, x = 0, y = 0, trigger;;
		if (in_type[i] != PSE_PAD_TYPE_GUN
		    && in_type[i] != PSE_PAD_TYPE_GUNCON)
			continue;
		trigger = in_type[i] == PSE_PAD_TYPE_GUN
			? (1 << DKEY_SQUARE) : (1 << DKEY_CIRCLE);

		pl_gun_ts_update(tsdev, &x, &y, &in);
		in_analog_left[i][0] = 65536;
		in_analog_left[i][1] = 65536;
		if (in && !(in_state_gun & (1 << SACTION_GUN_TRIGGER2))) {
			in_analog_left[i][0] = x;
			in_analog_left[i][1] = y;
			if (!(g_opts & OPT_TSGUN_NOTRIGGER))
				in_state_gun |= (1 << SACTION_GUN_TRIGGER);
		}
		in_keystate[i] = 0;
		if (in_state_gun & ((1 << SACTION_GUN_TRIGGER)
					| (1 << SACTION_GUN_TRIGGER2)))
			in_keystate[i] |= trigger;
		if (in_state_gun & (1 << SACTION_GUN_A))
			in_keystate[i] |= (1 << DKEY_START);
		if (in_state_gun & (1 << SACTION_GUN_B))
			in_keystate[i] |= (1 << DKEY_CROSS);
	}
}
#else /* MAEMO */
extern void update_input(void);
#endif

void pl_gun_byte2(int port, unsigned char byte)
{
	if (!tsdev || in_type[port] != PSE_PAD_TYPE_GUN || !(byte & 0x10))
		return;
	if (in_analog_left[port][0] == 65536)
		return;

	psxScheduleIrq10(4, in_analog_left[port][0] * 1629 / 1024,
		in_analog_left[port][1] * psx_h / 1024);
}

#define MAX_LAG_FRAMES 3

#define tvdiff(tv, tv_old) \
	((tv.tv_sec - tv_old.tv_sec) * 1000000 + tv.tv_usec - tv_old.tv_usec)

/* called on every vsync */
void pl_frame_limit(void)
{
	static struct timeval tv_old, tv_expect;
	static int vsync_cnt_prev, drc_active_vsyncs;
	struct timeval now;
	int diff, usadj;

	if (g_emu_resetting)
		return;

	vsync_cnt++;

	/* doing input here because the pad is polled
	 * thousands of times per frame for some reason */
	update_input();

	pcnt_end(PCNT_ALL);
	gettimeofday(&now, 0);

	if (now.tv_sec != tv_old.tv_sec) {
		diff = tvdiff(now, tv_old);
		pl_rearmed_cbs.vsps_cur = 0.0f;
		if (0 < diff && diff < 2000000)
			pl_rearmed_cbs.vsps_cur = 1000000.0f * (vsync_cnt - vsync_cnt_prev) / diff;
		vsync_cnt_prev = vsync_cnt;

		if (g_opts & OPT_SHOWFPS)
			pl_rearmed_cbs.flips_per_sec = pl_rearmed_cbs.flip_cnt;
		pl_rearmed_cbs.flip_cnt = 0;
		if (g_opts & OPT_SHOWCPU)
			pl_rearmed_cbs.cpu_usage = get_cpu_ticks();

		if (hud_new_msg > 0) {
			hud_new_msg--;
			if (hud_new_msg == 0)
				hud_msg[0] = 0;
		}
		tv_old = now;
		//new_dynarec_print_stats();
	}
#ifdef PCNT
	static int ya_vsync_count;
	if (++ya_vsync_count == PCNT_FRAMES) {
		pcnt_print(pl_rearmed_cbs.vsps_cur);
		ya_vsync_count = 0;
	}
#endif

	// tv_expect uses usec*1024 units instead of usecs for better accuracy
	tv_expect.tv_usec += frame_interval1024;
	if (tv_expect.tv_usec >= (1000000 << 10)) {
		tv_expect.tv_usec -= (1000000 << 10);
		tv_expect.tv_sec++;
	}
	diff = (tv_expect.tv_sec - now.tv_sec) * 1000000 + (tv_expect.tv_usec >> 10) - now.tv_usec;

	if (diff > MAX_LAG_FRAMES * frame_interval || diff < -MAX_LAG_FRAMES * frame_interval) {
		//printf("pl_frame_limit reset, diff=%d, iv %d\n", diff, frame_interval);
		tv_expect = now;
		diff = 0;
		// try to align with vsync
		usadj = vsync_usec_time;
		while (usadj < tv_expect.tv_usec - frame_interval)
			usadj += frame_interval;
		tv_expect.tv_usec = usadj << 10;
	}

	if (!(g_opts & OPT_NO_FRAMELIM) && diff > frame_interval) {
		// yay for working usleep on pandora!
		//printf("usleep %d\n", diff - frame_interval / 2);
		usleep(diff - frame_interval);
	}

	if (pl_rearmed_cbs.frameskip) {
		if (diff < -frame_interval)
			pl_rearmed_cbs.fskip_advice = 1;
		else if (diff >= 0)
			pl_rearmed_cbs.fskip_advice = 0;

		// recompilation is not that fast and may cause frame skip on
		// loading screens and such, resulting in flicker or glitches
		if (ndrc_g.did_compile) {
			if (drc_active_vsyncs < 32)
				pl_rearmed_cbs.fskip_advice = 0;
			drc_active_vsyncs++;
		}
		else
			drc_active_vsyncs = 0;
		ndrc_g.did_compile = 0;
	}

	pcnt_start(PCNT_ALL);
}

void pl_timing_prepare(int is_pal_)
{
	double fps;
	pl_rearmed_cbs.fskip_advice = 0;
	pl_rearmed_cbs.flips_per_sec = 0;
	pl_rearmed_cbs.cpu_usage = 0;

	is_pal = is_pal_;
	fps = psxGetFps();
	frame_interval = (int)(1000000.0 / fps);
	frame_interval1024 = (int)(1000000.0 * 1024.0 / fps);

	// used by P.E.Op.S. frameskip code
	pl_rearmed_cbs.gpu_peops.fFrameRateHz = (float)fps;
	pl_rearmed_cbs.gpu_peops.dwFrameRateTicks =
		(100000*100 / (int)(pl_rearmed_cbs.gpu_peops.fFrameRateHz*100));
}

static void pl_get_layer_pos(int *x, int *y, int *w, int *h)
{
	*x = g_layer_x;
	*y = g_layer_y;
	*w = g_layer_w;
	*h = g_layer_h;
}

static void *pl_mmap(unsigned int size);
static void pl_munmap(void *ptr, unsigned int size);

struct rearmed_cbs pl_rearmed_cbs = {
	pl_get_layer_pos,
	pl_vout_open,
	pl_vout_set_mode,
	pl_vout_flip,
	pl_vout_close,

	.mmap = pl_mmap,
	.munmap = pl_munmap,
	.pl_set_gpu_caps = pl_set_gpu_caps,
	.gpu_state_change = gpu_state_change,
};

/* watchdog */
static void *watchdog_thread(void *unused)
{
	int vsync_cnt_old = 0;
	int seen_dead = 0;
	int sleep_time = 5;

	while (1)
	{
		sleep(sleep_time);

		if (psxRegs.stop) {
			seen_dead = 0;
			sleep_time = 5;
			continue;
		}
		if (vsync_cnt != vsync_cnt_old) {
			vsync_cnt_old = vsync_cnt;
			seen_dead = 0;
			sleep_time = 2;
			continue;
		}

		seen_dead++;
		sleep_time = 1;
		if (seen_dead > 1)
			fprintf(stderr, "watchdog: seen_dead %d\n", seen_dead);
		if (seen_dead > 4) {
			fprintf(stderr, "watchdog: lockup detected, aborting\n");
			fflush(stderr);
			// we can't do any cleanup here really, the main thread is
			// likely touching resources and would crash anyway
			abort();
		}
	}
}

void pl_start_watchdog(void)
{
#if defined(NDEBUG) && !defined(DRC_DBG)
	pthread_attr_t attr;
	pthread_t tid;
	int ret;
#ifdef __linux__
	int tpid = 0;
	char buf[256];
	FILE *f = fopen("/proc/self/status", "r");
	if (f) {
		while (fgets(buf, sizeof(buf), f))
			if (buf[0] == 'T' && sscanf(buf, "TracerPid: %d", &tpid) == 1)
				break;
		fclose(f);
	}
	if (tpid) {
		printf("no watchdog to tracer %d\n", tpid);
		return;
	}
#endif
	
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

	ret = pthread_create(&tid, &attr, watchdog_thread, NULL);
	if (ret != 0)
		fprintf(stderr, "could not start watchdog: %d\n", ret);
#endif
	(void)watchdog_thread;
}

static void *pl_emu_mmap(unsigned long addr, size_t size,
	enum psxMapTag tag, int *can_retry_addr)
{
	*can_retry_addr = 1;
	return plat_mmap(addr, size, 0, 0);
}

static void pl_emu_munmap(void *ptr, size_t size, enum psxMapTag tag)
{
	plat_munmap(ptr, size);
}

static void *pl_mmap(unsigned int size)
{
	int can_retry_addr;
	return psxMapHook(0, size, MAP_TAG_VRAM, &can_retry_addr);
}

static void pl_munmap(void *ptr, unsigned int size)
{
	psxUnmapHook(ptr, size, MAP_TAG_VRAM);
}

void pl_init(void)
{
	extern unsigned int hSyncCount; // from psxcounters
	extern unsigned int frame_counter;

	psx_w = psx_h = pl_vout_w = pl_vout_h = 256;
	psx_bpp = pl_vout_bpp = 16;

	tsdev = pl_gun_ts_init();

	pl_rearmed_cbs.gpu_hcnt = &hSyncCount;
	pl_rearmed_cbs.gpu_frame_count = &frame_counter;

	psxMapHook = pl_emu_mmap;
	psxUnmapHook = pl_emu_munmap;
}
