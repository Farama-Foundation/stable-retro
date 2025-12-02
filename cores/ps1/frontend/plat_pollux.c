/*
 * (C) Gražvydas "notaz" Ignotas, 2009-2012
 *
 * This work is licensed under the terms of the GNU GPLv2 or later.
 * See the COPYING file in the top-level directory.
 *
 * GPH claims:
 *  Main Memory  : Wiz = 0       ~ 2A00000,      Caanoo = 0       ~ 5600000 (86M)
 *  Frame Buffer : Wiz = 2A00000 ~ 2E00000 (4M), Caanoo = 5600000 ~ 5700000 (1M)
 *  Sound Buffer : Wiz = 2E00000 ~ 3000000 (2M), Caanoo = 5700000 ~ 5800000 (1M)
 *  YUV Buffer   :                               Caanoo = 5800000 ~ 6000000 (8M)
 *  3D Buffer    : Wiz = 3000000 ~ 4000000 (16M),Caanoo = 6000000 ~ 8000000 (32M)
 *
 * Caanoo dram column (or row?) is 1024 bytes?
 *
 * pollux display array:
 * 27:24 |  23:18  |  17:11  |  10:6  |  5:0
 *  seg  | y[11:5] | x[11:6] | y[4:0] | x[5:0]
 *       |  blk_index[12:0]  |   block[10:0]
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <linux/fb.h>
#include <sys/mman.h>
#include <linux/soundcard.h>
#include <linux/input.h>

#include "libpicofe/plat.h"
#include "libpicofe/input.h"
#include "libpicofe/gp2x/in_gp2x.h"
#include "libpicofe/gp2x/soc_pollux.h"
#include "libpicofe/linux/in_evdev.h"
#include "libpicofe/menu.h"
#include "warm/warm.h"
#include "plugin_lib.h"
#include "pl_gun_ts.h"
#include "blit320.h"
#include "in_tsbutton.h"
#include "main.h"
#include "menu.h"
#include "plat.h"
#include "cspace.h"
#include "../libpcsxcore/psxmem_map.h"


static int fbdev = -1;
static void *fb_vaddrs[2];
static unsigned int fb_paddrs[2];
static int fb_work_buf;
static int have_warm;
#define FB_VRAM_SIZE (320*240*2*2*2) // 2 buffers with space for 24bpp mode
static unsigned int uppermem_pbase, vram_pbase;

static unsigned short *psx_vram;
static int psx_step, psx_width, psx_height, psx_bpp;
static int psx_offset_x, psx_offset_y, psx_src_width, psx_src_height;
static int fb_offset_x, fb_offset_y;

static void caanoo_init(void);
static void wiz_init(void);


static const struct in_default_bind in_evdev_defbinds[] = {
	{ KEY_UP,	IN_BINDTYPE_PLAYER12, DKEY_UP },
	{ KEY_DOWN,	IN_BINDTYPE_PLAYER12, DKEY_DOWN },
	{ KEY_LEFT,	IN_BINDTYPE_PLAYER12, DKEY_LEFT },
	{ KEY_RIGHT,	IN_BINDTYPE_PLAYER12, DKEY_RIGHT },
	{ BTN_TOP,	IN_BINDTYPE_PLAYER12, DKEY_TRIANGLE },
	{ BTN_THUMB,	IN_BINDTYPE_PLAYER12, DKEY_CROSS },
	{ BTN_THUMB2,	IN_BINDTYPE_PLAYER12, DKEY_CIRCLE },
	{ BTN_TRIGGER,	IN_BINDTYPE_PLAYER12, DKEY_SQUARE },
	{ BTN_BASE3,	IN_BINDTYPE_PLAYER12, DKEY_START },
	{ BTN_BASE4,	IN_BINDTYPE_PLAYER12, DKEY_SELECT },
	{ BTN_TOP2,	IN_BINDTYPE_PLAYER12, DKEY_L1 },
	{ BTN_PINKIE,	IN_BINDTYPE_PLAYER12, DKEY_R1 },
	{ BTN_BASE,	IN_BINDTYPE_EMU, SACTION_ENTER_MENU },
	{ 0, 0, 0 },
};

static const struct menu_keymap key_pbtn_map[] =
{
	{ KEY_UP,	PBTN_UP },
	{ KEY_DOWN,	PBTN_DOWN },
	{ KEY_LEFT,	PBTN_LEFT },
	{ KEY_RIGHT,	PBTN_RIGHT },
	/* Caanoo */
	{ BTN_THUMB2,	PBTN_MOK },
	{ BTN_THUMB,	PBTN_MBACK },
	{ BTN_TRIGGER,	PBTN_MA2 },
	{ BTN_TOP,	PBTN_MA3 },
	{ BTN_BASE,	PBTN_MENU },
	{ BTN_TOP2,	PBTN_L },
	{ BTN_PINKIE,	PBTN_R },
	/* "normal" keyboards */
	{ KEY_ENTER,	PBTN_MOK },
	{ KEY_ESC,	PBTN_MBACK },
	{ KEY_SEMICOLON,  PBTN_MA2 },
	{ KEY_APOSTROPHE, PBTN_MA3 },
	{ KEY_BACKSLASH,  PBTN_MENU },
	{ KEY_LEFTBRACE,  PBTN_L },
	{ KEY_RIGHTBRACE, PBTN_R },
};

static const struct in_pdata gp2x_evdev_pdata = {
	.defbinds = in_evdev_defbinds,
	.key_map = key_pbtn_map,
	.kmap_size = sizeof(key_pbtn_map) / sizeof(key_pbtn_map[0]),
};

static void *fb_flip(void)
{
	memregl[0x406C>>2] = memregl[0x446C>>2] = fb_paddrs[fb_work_buf];
	memregl[0x4058>>2] |= 0x10;
	memregl[0x4458>>2] |= 0x10;
	fb_work_buf ^= 1;
	return fb_vaddrs[fb_work_buf];
}

static void pollux_changemode(int bpp, int is_bgr)
{
	int code = 0, bytes = 2;
	unsigned int r;

	printf("changemode: %dbpp %s\n", bpp, is_bgr ? "bgr" : "rgb");

	memregl[0x4004>>2] = 0x00ef013f;
	memregl[0x4000>>2] |= 1 << 3;

	switch (bpp)
	{
		case 8:
			code = 0x443a;
			bytes = 1;
			break;
		case 16:
			code = is_bgr ? 0xc342 : 0x4432;
			bytes = 2;
			break;
		case 24:
			code = is_bgr ? 0xc653 : 0x4653;
			bytes = 3;
			break;
		default:
			printf("unhandled bpp request: %d\n", bpp);
			return;
	}

	// program both MLCs so that TV-out works
	memregl[0x405c>>2] = memregl[0x445c>>2] = bytes;
	memregl[0x4060>>2] = memregl[0x4460>>2] = 320 * bytes;

	r = memregl[0x4058>>2];
	r = (r & 0xffff) | (code << 16) | 0x10;
	memregl[0x4058>>2] = r;

	r = memregl[0x4458>>2];
	r = (r & 0xffff) | (code << 16) | 0x10;
	memregl[0x4458>>2] = r;
}

static int cpu_clock_wrapper(int mhz)
{
	// stupid pll share hack - must restart audio
	int pollux_cpu_clock_set(int cpu_clock);
	extern long SPUopen(void);
	extern long SPUclose(void);

	pollux_cpu_clock_set(mhz);
	SPUclose();
	SPUopen();

	return 0;
}

#define TIMER_BASE3 0x1980
#define TIMER_REG(x) memregl[(TIMER_BASE3 + x) >> 2]

static __attribute__((unused)) unsigned int timer_get(void)
{
	TIMER_REG(0x08) |= 0x48;  /* run timer, latch value */
	return TIMER_REG(0);
}

void plat_video_menu_enter(int is_rom_loaded)
{
	if (pl_vout_buf != NULL) {
		if (psx_bpp == 16)
			// have to do rgb conversion for menu bg
			bgr555_to_rgb565(pl_vout_buf, pl_vout_buf, 320*240*2);
		else
			memset(pl_vout_buf, 0, 320*240*2);
	}

	pollux_changemode(16, 0);
}

void plat_video_menu_begin(void)
{
}

void plat_video_menu_end(void)
{
	g_menuscreen_ptr = fb_flip();
}

void plat_video_menu_leave(void)
{
	if (psx_vram == NULL) {
		fprintf(stderr, "GPU plugin did not provide vram\n");
		exit(1);
	}

	if (gp2x_dev_id == GP2X_DEV_CAANOO)
		in_set_config_int(in_name_to_id("evdev:pollux-analog"),
			IN_CFG_ABS_DEAD_ZONE, analog_deadzone);

	memset(g_menuscreen_ptr, 0, 320*240 * psx_bpp/8);
	g_menuscreen_ptr = fb_flip();
	memset(g_menuscreen_ptr, 0, 320*240 * psx_bpp/8);
}

void *plat_prepare_screenshot(int *w, int *h, int *bpp)
{
	bgr555_to_rgb565(pl_vout_buf, pl_vout_buf, 320*240*2);
	*w = 320;
	*h = 240;
	*bpp = psx_bpp;
	return pl_vout_buf;
}

void plat_minimize(void)
{
}

static void spend_cycles(int loops)
{
	asm volatile (
		"   mov  r0,%0    ;\n"
		"0: subs r0,r0,#1 ;\n"
		"   bgt  0b"
		:: "r" (loops) : "cc", "r0");
}

#define DMA_BASE6 0x0300
#define DMA_REG(x) memregl[(DMA_BASE6 + x) >> 2]

/* this takes ~1.5ms, while ldm/stm ~1.95ms */
static void raw_blit_dma(int doffs, const void *vram, int w, int h,
			 int sstride, int bgr24)
{
	unsigned int pixel_offset = (unsigned short *)vram - psx_vram;
	unsigned int dst = fb_paddrs[fb_work_buf] +
			(fb_offset_y * 320 + fb_offset_x) * psx_bpp / 8;
	int spsx_line = pixel_offset / 1024 + psx_offset_y;
	int spsx_offset = (pixel_offset + psx_offset_x) & 0x3f8;
	unsigned int vram_byte_pos, vram_byte_step;
	int dst_stride = 320 * psx_bpp / 8;
	int len = psx_src_width * psx_bpp / 8;
	int i;

	//warm_cache_op_all(WOP_D_CLEAN);

	dst &= ~7;
	len &= ~7;

	if (DMA_REG(0x0c) & 0x90000) {
		printf("already runnig DMA?\n");
		DMA_REG(0x0c) = 0x100000;
	}
	if ((DMA_REG(0x2c) & 0x0f) < 5) {
		printf("DMA queue busy?\n");
		DMA_REG(0x24) = 1;
	}

	vram_byte_pos = vram_pbase;
	vram_byte_pos += (spsx_line & 511) * 2 * 1024 + spsx_offset * 2;
	vram_byte_step = psx_step * 2 * 1024;

	for (i = psx_src_height; i > 0;
	     i--, vram_byte_pos += vram_byte_step, dst += dst_stride)
	{
		while ((DMA_REG(0x2c) & 0x0f) < 4)
			spend_cycles(10);

		// XXX: it seems we must always set all regs, what is autoincrement there for?
		DMA_REG(0x20) = 1;		// queue wait cmd
		DMA_REG(0x10) = vram_byte_pos;	// DMA src
		DMA_REG(0x14) = dst;		// DMA dst
		DMA_REG(0x18) = len - 1;	// len
		DMA_REG(0x1c) = 0x80000;	// go
	}
}

#define make_flip_func(name, blitfunc)                                                  \
static void name(int doffs, const void *vram_, int w, int h, int sstride, int bgr24)    \
{                                                                                       \
        const unsigned short *vram = vram_;                                             \
        unsigned char *dst = (unsigned char *)g_menuscreen_ptr +                        \
                        (fb_offset_y * 320 + fb_offset_x) * psx_bpp / 8;                \
        int dst_stride = 320 * psx_bpp / 8;                                             \
        int len = psx_src_width * psx_bpp / 8;                                          \
        int i;                                                                          \
                                                                                        \
        vram += psx_offset_y * 1024 + psx_offset_x;                                     \
        vram = (void *)((long)vram & ~3);                                               \
        for (i = psx_src_height; i > 0; i--, vram += psx_step * 1024, dst += dst_stride)\
                blitfunc(dst, vram, len);                                               \
}

make_flip_func(raw_blit_soft, memcpy)
make_flip_func(raw_blit_soft_368, blit320_368)
make_flip_func(raw_blit_soft_512, blit320_512)
make_flip_func(raw_blit_soft_640, blit320_640)

void *plat_gvideo_set_mode(int *w_, int *h_, int *bpp_)
{
	int poff_w, poff_h, w_max;
	int w = *w_, h = *h_, bpp = *bpp_;

	if (!w || !h || !bpp)
		return NULL;

	printf("psx mode: %dx%d@%d\n", w, h, bpp);
	psx_width = w;
	psx_height = h;
	psx_bpp = bpp;

	switch (w + (bpp != 16) + !soft_scaling) {
	case 640:
		pl_plat_blit = raw_blit_soft_640;
		w_max = 640;
		break;
	case 512:
		pl_plat_blit = raw_blit_soft_512;
		w_max = 512;
		break;
	case 384:
	case 368:
		pl_plat_blit = raw_blit_soft_368;
		w_max = 368;
		break;
	default:
		pl_plat_blit = have_warm ? raw_blit_dma : raw_blit_soft;
		w_max = 320;
		break;
	}

	psx_step = 1;
	if (h > 256) {
		psx_step = 2;
		h /= 2;
	}

	poff_w = poff_h = 0;
	if (w > w_max) {
		poff_w = w / 2 - w_max / 2;
		w = w_max;
	}
	fb_offset_x = 0;
	if (w < 320)
		fb_offset_x = 320/2 - w / 2;
	if (h > 240) {
		poff_h = h / 2 - 240/2;
		h = 240;
	}
	fb_offset_y = 240/2 - h / 2;

	psx_offset_x = poff_w * psx_bpp/8 / 2;
	psx_offset_y = poff_h;
	psx_src_width = w;
	psx_src_height = h;

	if (fb_offset_x || fb_offset_y) {
		// not fullscreen, must clear borders
		memset(g_menuscreen_ptr, 0, 320*240 * psx_bpp/8);
		g_menuscreen_ptr = fb_flip();
		memset(g_menuscreen_ptr, 0, 320*240 * psx_bpp/8);
	}

	pollux_changemode(bpp, 1);

	pl_set_gun_rect(fb_offset_x, fb_offset_y, w > 320 ? 320 : w, h);

	// adjust for hud
	*w_ = 320;
	*h_ = fb_offset_y + psx_src_height;

	return g_menuscreen_ptr;
}

/* not really used, we do raw_flip */
void plat_gvideo_open(int is_pal)
{
}

void *plat_gvideo_flip(void)
{
	g_menuscreen_ptr = fb_flip();
	return g_menuscreen_ptr;
}

void plat_gvideo_close(void)
{
}

static void *pl_emu_mmap(unsigned long addr, size_t size,
	enum psxMapTag tag, int *can_retry_addr)
{
	unsigned int pbase;
	void *retval;
	int ret;

	*can_retry_addr = 1;
	if (!have_warm)
		goto basic_map;

	switch (tag) {
	case MAP_TAG_RAM:
		if (size > 0x400000) {
			fprintf(stderr, "unexpected ram map request: %08lx %x\n",
				addr, size);
			exit(1);
		}
		pbase = (uppermem_pbase + 0xffffff) & ~0xffffff;
		pbase += 0x400000;
		retval = (void *)addr;
		ret = warm_mmap_section(retval, pbase, size, WCB_C_BIT);
		if (ret != 0) {
			fprintf(stderr, "ram section map failed\n");
			exit(1);
		}
		goto out;
	case MAP_TAG_VRAM:
		if (size > 0x400000) {
			fprintf(stderr, "unexpected vram map request: %08lx %x\n",
				addr, size);
			exit(1);
		}
		if (addr == 0)
			addr = 0x60000000;
		vram_pbase = (uppermem_pbase + 0xffffff) & ~0xffffff;
		retval = (void *)addr;

		ret = warm_mmap_section(retval, vram_pbase, size, WCB_C_BIT);
		if (ret != 0) {
			fprintf(stderr, "vram section map failed\n");
			exit(1);
		}
		goto out;
	case MAP_TAG_LUTS:
		// mostly for Wiz to not run out of RAM
		if (size > 0x800000) {
			fprintf(stderr, "unexpected LUT map request: %08lx %x\n",
				addr, size);
			exit(1);
		}
		pbase = (uppermem_pbase + 0xffffff) & ~0xffffff;
		pbase += 0x800000;
		retval = (void *)addr;
		ret = warm_mmap_section(retval, pbase, size, WCB_C_BIT);
		if (ret != 0) {
			fprintf(stderr, "LUT section map failed\n");
			exit(1);
		}
		goto out;
	default:
		break;
	}

basic_map:
	retval = plat_mmap(addr, size, 0, 0);

out:
	if (tag == MAP_TAG_VRAM && retval)
		psx_vram = retval;
	return retval ? retval : MAP_FAILED;
}

static void pl_emu_munmap(void *ptr, size_t size, enum psxMapTag tag)
{
	switch (tag) {
	case MAP_TAG_RAM:
	case MAP_TAG_VRAM:
	case MAP_TAG_LUTS:
		warm_munmap_section(ptr, size);
		break;
	default:
		plat_munmap(ptr, size);
		break;
	}
}

void plat_init(void)
{
	const char *main_fb_name = "/dev/fb0";
	struct fb_fix_screeninfo fbfix;
	int ret;

	plat_target_init();

	fbdev = open(main_fb_name, O_RDWR);
	if (fbdev == -1) {
		fprintf(stderr, "%s: ", main_fb_name);
		perror("open");
		exit(1);
	}

	ret = ioctl(fbdev, FBIOGET_FSCREENINFO, &fbfix);
	if (ret == -1) {
		perror("ioctl(fbdev) failed");
		exit(1);
	}
	uppermem_pbase = fbfix.smem_start;

	printf("framebuffer: \"%s\" @ %08lx\n", fbfix.id, fbfix.smem_start);
	fb_paddrs[0] = fbfix.smem_start;
	fb_paddrs[1] = fb_paddrs[0] + 320*240*4; // leave space for 24bpp

	ret = warm_init();
	have_warm = (ret == 0);

	if (have_warm) {
		// map fb as write-through cached section
		fb_vaddrs[0] = (void *)0x7fe00000;
		ret = warm_mmap_section(fb_vaddrs[0], fb_paddrs[0],
			FB_VRAM_SIZE, WCB_C_BIT);
		if (ret != 0) {
			fprintf(stderr, "fb section map failed\n");
			fb_vaddrs[0] = NULL;

			// we could continue but it would just get messy
			exit(1);
		}
	}
	if (fb_vaddrs[0] == NULL) {
		fb_vaddrs[0] = mmap(0, FB_VRAM_SIZE, PROT_READ|PROT_WRITE,
				MAP_SHARED, memdev, fb_paddrs[0]);
		if (fb_vaddrs[0] == MAP_FAILED) {
			perror("mmap(fb_vaddrs) failed");
			exit(1);
		}

		memset(fb_vaddrs[0], 0, FB_VRAM_SIZE);
		warm_change_cb_range(WCB_C_BIT, 1, fb_vaddrs[0], FB_VRAM_SIZE);
	}
	printf("  mapped @%p\n", fb_vaddrs[0]);

	fb_vaddrs[1] = (char *)fb_vaddrs[0] + 320*240*4;

	memset(fb_vaddrs[0], 0, FB_VRAM_SIZE);

	pollux_changemode(16, 0);
	g_menuscreen_w = g_menuscreen_pp = 320;
	g_menuscreen_h = 240;
	g_menuscreen_ptr = fb_flip();

	/* setup DMA */
	DMA_REG(0x0c) = 0x20000; // pending IRQ clear

	in_tsbutton_init();
	in_evdev_init(&gp2x_evdev_pdata);
	if (gp2x_dev_id == GP2X_DEV_CAANOO)
		caanoo_init();
	else
		wiz_init();

	pl_plat_blit = have_warm ? raw_blit_dma : raw_blit_soft;

	psx_src_width = 320;
	psx_src_height = 240;
	psx_bpp = 16;

	pl_rearmed_cbs.screen_w = 320;
	pl_rearmed_cbs.screen_h = 240;

	plat_target_setup_input();

	plat_target.cpu_clock_set = cpu_clock_wrapper;

	psxMapHook = pl_emu_mmap;
	psxUnmapHook = pl_emu_munmap;
}

void plat_finish(void)
{
	warm_finish();
	memset(fb_vaddrs[0], 0, FB_VRAM_SIZE);
	munmap(fb_vaddrs[0], FB_VRAM_SIZE);
	close(fbdev);
	plat_target_finish();
}

/* Caanoo stuff, perhaps move later */
static const char * const caanoo_keys[KEY_MAX + 1] = {
	[0 ... KEY_MAX] = NULL,
	[KEY_UP]        = "Up",
	[KEY_LEFT]      = "Left",
	[KEY_RIGHT]     = "Right",
	[KEY_DOWN]      = "Down",
	[BTN_TRIGGER]   = "A",
	[BTN_THUMB]     = "X",
	[BTN_THUMB2]    = "B",
	[BTN_TOP]       = "Y",
	[BTN_TOP2]      = "L",
	[BTN_PINKIE]    = "R",
	[BTN_BASE]      = "Home",
	[BTN_BASE2]     = "Lock",
	[BTN_BASE3]     = "I",
	[BTN_BASE4]     = "II",
	[BTN_BASE5]     = "Push",
};

struct haptic_data {
	int count;
	struct {
		short time, strength;
	} actions[120];
};

#define HAPTIC_IOCTL_MAGIC	'I'
#define HAPTIC_PLAY_PATTERN	_IOW(HAPTIC_IOCTL_MAGIC, 4, struct haptic_data)
#define HAPTIC_INDIVIDUAL_MODE	_IOW(HAPTIC_IOCTL_MAGIC, 5, unsigned int)
#define HAPTIC_SET_VIB_LEVEL	_IOW(HAPTIC_IOCTL_MAGIC, 9, unsigned int)

static int hapticdev = -1;
static struct haptic_data haptic_seq[2];

static int haptic_read(const char *fname, struct haptic_data *data)
{
	int i, ret, v1, v2;
	char buf[128], *p;
	FILE *f;

	f = fopen(fname, "r");
	if (f == NULL) {
		fprintf(stderr, "fopen(%s)", fname);
		perror("");
		return -1;
	}

	for (i = 0; i < sizeof(data->actions) / sizeof(data->actions[0]); ) {
		p = fgets(buf, sizeof(buf), f);
		if (p == NULL)
			break;
		while (*p != 0 && *p == ' ')
			p++;
		if (*p == 0 || *p == ';' || *p == '#')
			continue;

		ret = sscanf(buf, "%d %d", &v1, &v2);
		if (ret != 2) {
			fprintf(stderr, "can't parse: %s", buf);
			continue;
		}

		data->actions[i].time = v1;
		data->actions[i].strength = v2;
		i++;
	}
	fclose(f);

	if (i == 0) {
		fprintf(stderr, "bad haptic file: %s\n", fname);
		return -1;
	}
	data->count = i;

	return 0;
}

static int haptic_init(void)
{
	int ret, i;

	ret = haptic_read("haptic_w.cfg", &haptic_seq[0]);
	if (ret != 0)
		return -1;
	ret = haptic_read("haptic_s.cfg", &haptic_seq[1]);
	if (ret != 0)
		return -1;

	hapticdev = open("/dev/isa1200", O_RDWR | O_NONBLOCK);
	if (hapticdev == -1) {
		perror("open(/dev/isa1200)");
		return -1;
	}

	i = 0;
	ret  = ioctl(hapticdev, HAPTIC_INDIVIDUAL_MODE, &i);	/* use 2 of them */
	i = 3;
	ret |= ioctl(hapticdev, HAPTIC_SET_VIB_LEVEL, &i);	/* max */
	if (ret != 0) {
		fprintf(stderr, "haptic ioctls failed\n");
		close(hapticdev);
		hapticdev = -1;
		return -1;
	}

	return 0;
}

void plat_trigger_vibrate(int pad, int low, int high)
{
	int is_strong;
	int ret;

	if (low == 0 && high == 0)
		return;
	is_strong = (high >= 0xf0);

	if (hapticdev == -2)
		return; // it's broken
	if (hapticdev < 0) {
		ret = haptic_init();
		if (ret < 0) {
			hapticdev = -2;
			return;
		}
	}

	ioctl(hapticdev, HAPTIC_PLAY_PATTERN, &haptic_seq[!!is_strong]);
}

static void caanoo_init(void)
{
	in_probe();
	in_set_config(in_name_to_id("evdev:pollux-analog"), IN_CFG_KEY_NAMES,
		      caanoo_keys, sizeof(caanoo_keys));
}

/* Wiz stuff */
static const struct in_default_bind in_gp2x_defbinds[] =
{
	/* MXYZ SACB RLDU */
	{ GP2X_BTN_UP,		IN_BINDTYPE_PLAYER12, DKEY_UP },
	{ GP2X_BTN_DOWN,	IN_BINDTYPE_PLAYER12, DKEY_DOWN },
	{ GP2X_BTN_LEFT,	IN_BINDTYPE_PLAYER12, DKEY_LEFT },
	{ GP2X_BTN_RIGHT,	IN_BINDTYPE_PLAYER12, DKEY_RIGHT },
	{ GP2X_BTN_X,		IN_BINDTYPE_PLAYER12, DKEY_CROSS },
	{ GP2X_BTN_B,		IN_BINDTYPE_PLAYER12, DKEY_CIRCLE },
	{ GP2X_BTN_A,		IN_BINDTYPE_PLAYER12, DKEY_SQUARE },
	{ GP2X_BTN_Y,		IN_BINDTYPE_PLAYER12, DKEY_TRIANGLE },
	{ GP2X_BTN_L,		IN_BINDTYPE_PLAYER12, DKEY_L1 },
	{ GP2X_BTN_R,		IN_BINDTYPE_PLAYER12, DKEY_R1 },
	{ GP2X_BTN_START,	IN_BINDTYPE_PLAYER12, DKEY_START },
	{ GP2X_BTN_SELECT,	IN_BINDTYPE_EMU, SACTION_ENTER_MENU },
	{ GP2X_BTN_VOL_UP,	IN_BINDTYPE_EMU, SACTION_VOLUME_UP },
	{ GP2X_BTN_VOL_DOWN,	IN_BINDTYPE_EMU, SACTION_VOLUME_DOWN },
	{ 0, 0, 0 },
};

// unused dummy for in_gp2x
volatile unsigned short *gp2x_memregs;

static void wiz_init(void)
{
	in_gp2x_init(in_gp2x_defbinds);
	in_probe();
}
