/*
 * Platform interface functions for PSP picodrive frontend
 *
 * (C) 2020 kub
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>

#include "../common/emu.h"
#include "../libpicofe/menu.h"
#include "../libpicofe/plat.h"

#include <pspiofilemgr.h>
#include <pspthreadman.h>
#include <pspdisplay.h>
#include <psputils.h>
#include <psppower.h>
#include <pspgu.h>
#include <pspaudio.h>

#include "psp.h"
#include "emu.h"
#include "asm_utils.h"

#include <pico/pico_int.h>

/* graphics buffer management in VRAM:
 * -	VRAM_FB0, VRAM_FB1	frame buffers
 * -	VRAM_DEPTH		Z buffer (unused)
 * -	VRAM_BUF0, VRAM_BUF1	emulator render buffers
 * Emulator screen output is using the MD screen resolutions and is rendered
 * to VRAM_BUFx and subsequently projected (that is, scaled and blitted) into
 * the associated frame buffer (in PSP output resolution). Additional emulator
 * output is then directly rendered to that frame buffer.
 * The emulator menu is rendered directly into the frame buffers, using the
 * native PSP resolution.
 * Menu background uses native resolution and is copied and shaded from a frame
 * buffer, or read in from a file if no emulator screen output is present.
 */

/* System level intialization */
int plat_target_init(void)
{
	psp_init();

	/* buffer resolutions */
	g_menuscreen_w = 480, g_menuscreen_h  = 272, g_menuscreen_pp = 512;
	g_screen_width = 328, g_screen_height = 256, g_screen_ppitch = 512;
	g_menubg_src_w = 480, g_menubg_src_h  = 272, g_menubg_src_pp = 512;

	/* buffer settings for menu display on startup */
	g_screen_ptr = VRAM_CACHED_STUFF + (psp_screen - VRAM_FB0);
	g_menuscreen_ptr = psp_screen;
	g_menubg_ptr = malloc(512*272*2);

	return 0;
}

/* System level deinitialization */
void plat_target_finish(void)
{
	psp_finish();
}

/* display a completed frame buffer and prepare a new render buffer */
void plat_video_flip(void)
{
	g_menubg_src_ptr = psp_screen;
	psp_video_flip(currentConfig.EmuOpt & EOPT_VSYNC, 1);
	g_screen_ptr = VRAM_CACHED_STUFF + (psp_screen - VRAM_FB0);
	plat_video_set_buffer(g_screen_ptr);
}

/* wait for start of vertical blanking */
void plat_video_wait_vsync(void)
{
	sceDisplayWaitVblankStart();
}

/* switch from emulation display to menu display */
void plat_video_menu_enter(int is_rom_loaded)
{
}

/* start rendering a menu screen */
void plat_video_menu_begin(void)
{
	g_menuscreen_ptr = psp_screen;
}

/* display a completed menu screen */
void plat_video_menu_end(void)
{
	plat_video_wait_vsync();
	psp_video_flip(0, 0);
}

/* terminate menu display */
void plat_video_menu_leave(void)
{
}

/* Preliminary initialization needed at program start */
void plat_early_init(void)
{
}

/* base directory for configuration and save files */
int plat_get_root_dir(char *dst, int len)
{
 	if (len > 0) *dst = 0;
	return 0;
}

/* base directory for emulator resources */
int plat_get_skin_dir(char *dst, int len)
{
	if (len > 5)
		strcpy(dst, "skin/");
	else if (len > 0)
		*dst = 0;
	return strlen(dst);
}

/* top directory for rom images */
int plat_get_data_dir(char *dst, int len)
{
	if (len > 5)
		strcpy(dst, "ms0:/");
	else if (len > 0)
		*dst = 0;
	return strlen(dst);
}

/* check if path is a directory */
int plat_is_dir(const char *path)
{
	SceIoStat st;
	int ret = sceIoGetstat(path, &st);
	return (ret >= 0 && (st.st_mode & FIO_S_IFDIR));
}

/* current time in ms */
unsigned int plat_get_ticks_ms(void)
{
	/* approximate /= 1000 */
	unsigned long long v64;
	v64 = (unsigned long long)plat_get_ticks_us() * 4294968;
	return v64 >> 32;
}

/* current time in us */
unsigned int plat_get_ticks_us(void)
{
	return sceKernelGetSystemTimeLow();
}

/* sleep for some time in ms */
void plat_sleep_ms(int ms)
{
	psp_msleep(ms);
}

/* sleep for some time in us */
void plat_wait_till_us(unsigned int us_to)
{
	unsigned int tval;
	int diff;

	tval = sceKernelGetSystemTimeLow();
	diff = (int)us_to - (int)tval;
	if (diff >= 512 && diff < 100*1024)
		sceKernelDelayThread(diff);
}

/* wait until some event occurs, or timeout */
int plat_wait_event(int *fds_hnds, int count, int timeout_ms)
{
	return 0;	// unused
}

/* memory mapping functions */
void *plat_mmap(unsigned long addr, size_t size, int need_exec, int is_fixed)
{
	return malloc(size);
}

void *plat_mremap(void *ptr, size_t oldsize, size_t newsize)
{
	return realloc(ptr, newsize);
}

void plat_munmap(void *ptr, size_t size)
{
	free(ptr);
}

void *plat_mem_get_for_drc(size_t size)
{
	return NULL;
}

int plat_mem_set_exec(void *ptr, size_t size)
{
	return 0;
}

/* get current CPU clock */
static int plat_cpu_clock_get(void)
{
	return scePowerGetCpuClockFrequencyInt();
}

/* set CPU clock */
static int plat_cpu_clock_set(int clock)
{
	if (clock < 33) clock = 33;
	if (clock > 333) clock = 333;

	return scePowerSetClockFrequency(clock, clock, clock/2);
}

/* get battery state in percent */
static int plat_bat_capacity_get(void)
{
	return scePowerGetBatteryLifePercent();
}

struct plat_target plat_target = {
	.cpu_clock_get = plat_cpu_clock_get,
	.cpu_clock_set = plat_cpu_clock_set,
	.bat_capacity_get = plat_bat_capacity_get,
//	.gamma_set = plat_gamma_set,
//	.hwfilter_set = plat_hwfilter_set,
//	.hwfilters = plat_hwfilters,
};

#ifndef DT_DIR
/* replacement libc stuff */

int alphasort(const struct dirent **a, const struct dirent **b)
{
	return strcoll ((*a)->d_name, (*b)->d_name);
}

int scandir(const char *dir, struct dirent ***namelist_out,
		int(*filter)(const struct dirent *),
		int(*compar)(const struct dirent **, const struct dirent **))
{
	int ret = -1, dir_uid = -1, name_alloc = 4, name_count = 0;
	struct dirent **namelist = NULL, *ent;
	SceIoDirent sce_ent;

	namelist = malloc(sizeof(*namelist) * name_alloc);
	if (namelist == NULL) { lprintf("%s:%i: OOM\n", __FILE__, __LINE__); goto fail; }

	// try to read first..
	dir_uid = sceIoDopen(dir);
	if (dir_uid >= 0)
	{
		/* it is very important to clear SceIoDirent to be passed to sceIoDread(), */
		/* or else it may crash, probably misinterpreting something in it. */
		memset(&sce_ent, 0, sizeof(sce_ent));
		ret = sceIoDread(dir_uid, &sce_ent);
		if (ret < 0)
		{
			lprintf("sceIoDread(\"%s\") failed with %i\n", dir, ret);
			goto fail;
		}
	}
	else
		lprintf("sceIoDopen(\"%s\") failed with %i\n", dir, dir_uid);

	while (ret > 0)
	{
		ent = malloc(sizeof(*ent));
		if (ent == NULL) { lprintf("%s:%i: OOM\n", __FILE__, __LINE__); goto fail; }
		ent->d_stat = sce_ent.d_stat;
		ent->d_stat.st_attr &= FIO_SO_IFMT; // serves as d_type
		strncpy(ent->d_name, sce_ent.d_name, sizeof(ent->d_name));
		ent->d_name[sizeof(ent->d_name)-1] = 0;
		if (filter == NULL || filter(ent))
		     namelist[name_count++] = ent;
		else free(ent);

		if (name_count >= name_alloc)
		{
			void *tmp;
			name_alloc *= 2;
			tmp = realloc(namelist, sizeof(*namelist) * name_alloc);
			if (tmp == NULL) { lprintf("%s:%i: OOM\n", __FILE__, __LINE__); goto fail; }
			namelist = tmp;
		}

		memset(&sce_ent, 0, sizeof(sce_ent));
		ret = sceIoDread(dir_uid, &sce_ent);
	}

	// sort
	if (compar != NULL && name_count > 3)
		qsort(&namelist[2], name_count - 2, sizeof(namelist[0]), (int (*)()) compar);

	// all done.
	ret = name_count;
	*namelist_out = namelist;
	goto end;

fail:
	if (namelist != NULL)
	{
		while (name_count--)
			free(namelist[name_count]);
		free(namelist);
	}
end:
	if (dir_uid >= 0) sceIoDclose(dir_uid);
	return ret;
}

/* stubs for libflac (embedded in libchdr) */
#include <utime.h>
#include <malloc.h>

int chown(const char *pathname, uid_t owner, gid_t group) { return -1; }
int chmod(const char *pathname, mode_t mode) { return -1; }
int utime(const char *filename, const struct utimbuf *times) { return -1; }
int posix_memalign(void **memptr, size_t alignment, size_t size)
	{ *memptr = memalign(alignment, size); return 0; }
#endif

int _flush_cache (char *addr, const int size, const int op)
{
	//sceKernelDcacheWritebackAll();
	sceKernelDcacheWritebackRange(addr, size);
	sceKernelIcacheInvalidateRange(addr, size);
	return 0;
}
