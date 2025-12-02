/*
 * (C) Gražvydas "notaz" Ignotas, 2010-2015
 *
 * This work is licensed under the terms of any of these licenses
 * (at your option):
 *  - GNU GPL, version 2 or later.
 *  - GNU LGPL, version 2.1 or later.
 * See the COPYING file in the top-level directory.
 */

#define _GNU_SOURCE 1
#include <stdio.h>
#include <string.h>
#include <errno.h>
#ifndef NO_DYLIB
#include <dlfcn.h>
#endif
#include <zlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>

#include "main.h"
#include "menu.h"
#include "config.h"
#include "plugin.h"
#include "plugin_lib.h"
#include "plat.h"
#include "pcnt.h"
#include "cspace.h"
#include "libpicofe/plat.h"
#include "libpicofe/input.h"
#include "libpicofe/linux/in_evdev.h"
#include "libpicofe/plat.h"
#include "../libpcsxcore/misc.h"
#include "../libpcsxcore/cdrom.h"
#include "../libpcsxcore/cdrom-async.h"
#include "../libpcsxcore/cdriso.h"
#include "../libpcsxcore/cheat.h"
#include "../libpcsxcore/ppf.h"
#include "../libpcsxcore/new_dynarec/new_dynarec.h"
#include "../plugins/dfsound/spu_config.h"
#include "psemu_plugin_defs.h"
#include "compiler_features.h"
#include "arm_features.h"
#include "revision.h"

#define REARMED_BIRTHDAY_TIME 1293306830	/* 25 Dec 2010 */
#if defined(__linux__) && (!defined(__SIZEOF_POINTER__) || __SIZEOF_POINTER__ == 4)
#define STAT stat64
#else
#define STAT stat
#endif

#define array_size(x) (sizeof(x) / sizeof(x[0]))

typedef enum
{
	MA_NONE = 1,
	MA_MAIN_RESUME_GAME,
	MA_MAIN_SAVE_STATE,
	MA_MAIN_LOAD_STATE,
	MA_MAIN_RESET_GAME,
	MA_MAIN_LOAD_ROM,
	MA_MAIN_SWAP_CD,
	MA_MAIN_SWAP_CD_MULTI,
	MA_MAIN_RUN_BIOS,
	MA_MAIN_RUN_EXE,
	MA_MAIN_LOAD_CHEATS,
	MA_MAIN_CHEATS,
	MA_MAIN_CONTROLS,
	MA_MAIN_CREDITS,
	MA_MAIN_EXIT,
	MA_CTRL_PLAYER1,
	MA_CTRL_PLAYER2,
	MA_CTRL_ANALOG,
	MA_CTRL_EMU,
	MA_CTRL_DEV_FIRST,
	MA_CTRL_DEV_NEXT,
	MA_CTRL_NUBS_BTNS,
	MA_CTRL_DEADZONE,
	MA_CTRL_VIBRATION,
	MA_CTRL_DONE,
	MA_OPT_SAVECFG,
	MA_OPT_SAVECFG_GAME,
	MA_OPT_CPU_CLOCKS,
	MA_OPT_SPU_THREAD,
	MA_OPT_DISP_OPTS,
	MA_OPT_VARSCALER,
	MA_OPT_VARSCALER_C,
	MA_OPT_SCALER2,
	MA_OPT_HWFILTER,
	MA_OPT_SWFILTER,
	MA_OPT_GAMMA,
	MA_OPT_VOUT_MODE,
	MA_OPT_VOUT_FULL,
	MA_OPT_SCANLINES,
	MA_OPT_SCANLINE_LEVEL,
	MA_OPT_CENTERING,
	MA_OPT_OVERSCAN,
	MA_OPT_VSYNC,
} menu_id;

static int last_vout_w, last_vout_h, last_vout_bpp;
static int cpu_clock, cpu_clock_st, volume_boost;
static int frameskip = 1; // 0 - auto, 1 - off
static char last_selected_fname[MAXPATHLEN];
static int config_save_counter, region, in_type_sel1, in_type_sel2;
static int psx_clock;
static int memcard1_sel = -1, memcard2_sel = -1;
static int cd_buf_count;
extern int g_autostateld_opt;
static int menu_iopts[8];
int g_opts, g_scaler, g_gamma = 100;
int scanlines, scanline_level = 20;
int soft_scaling, analog_deadzone; // for Caanoo
int soft_filter;
int in_evdev_allow_abs_only attr_weak; // FIXME

#ifndef HAVE_PRE_ARMV7
#define DEFAULT_PSX_CLOCK (10000 / CYCLE_MULT_DEFAULT)
#define DEFAULT_PSX_CLOCK_S "57"
#else
#define DEFAULT_PSX_CLOCK 50
#define DEFAULT_PSX_CLOCK_S "50"
#endif

static const char *bioses[32];
static const char *gpu_plugins[16];
static const char *spu_plugins[16];
static const char *memcards[32];
static int bios_sel, gpu_plugsel, spu_plugsel;

#ifndef UI_FEATURES_H
#define MENU_SHOW_VOUTMODE 1
#define MENU_SHOW_SCALER2 0
#define MENU_SHOW_NUBS_BTNS 0
#define MENU_SHOW_VIBRATION 0
#define MENU_SHOW_DEADZONE 0
#define MENU_SHOW_MINIMIZE 0
#define MENU_SHOW_FULLSCREEN 1
#define MENU_SHOW_VOLUME 0
#endif
#ifndef MENU_SHOW_VARSCALER
#define MENU_SHOW_VARSCALER 0
#endif
#ifndef MENU_SHOW_VARSCALER_C
#define MENU_SHOW_VARSCALER_C 0
#endif

static int min(int x, int y) { return x < y ? x : y; }
static int max(int x, int y) { return x > y ? x : y; }

static int emu_check_save_file(int slot, int *time)
{
	char fname[MAXPATHLEN];
	struct stat status;
	int ret;
	
	ret = emu_check_state(slot);
	if (ret != 0 || time == NULL)
		return ret == 0 ? 1 : 0;

	ret = get_state_filename(fname, sizeof(fname), slot);
	if (ret != 0)
		return 1;

	ret = stat(fname, &status);
	if (ret != 0)
		return 1;

	if (status.st_mtime < REARMED_BIRTHDAY_TIME)
		return 1; // probably bad rtc like on some Caanoos

	*time = status.st_mtime;

	return 1;
}

static int emu_save_load_game(int load, int unused)
{
	int ret;

	if (load) {
		ret = emu_load_state(state_slot);

		// reflect hle/bios mode from savestate
		if (Config.HLE)
			bios_sel = 0;
		else if (bios_sel == 0 && bioses[1] != NULL)
			// XXX: maybe find the right bios instead
			bios_sel = 1;
	}
	else
		ret = emu_save_state(state_slot);

	return ret;
}

static void rm_namelist_entry(struct dirent **namelist,
	int count, const char *name)
{
	int i;

	for (i = 1; i < count; i++) {
		if (namelist[i] == NULL || namelist[i]->d_type == DT_DIR)
			continue;

		if (strcmp(name, namelist[i]->d_name) == 0) {
			free(namelist[i]);
			namelist[i] = NULL;
			break;
		}
	}
}

static int optional_cdimg_filter(struct dirent **namelist, int count,
	const char *basedir)
{
	const char *ext, *p;
	char buf[256], buf2[257];
	int i, d, ret, good_cue;
	struct STAT statf;
	FILE *f;

	if (count <= 1)
		return count;

	for (i = 1; i < count; i++) {
		if (namelist[i] == NULL || namelist[i]->d_type == DT_DIR)
			continue;

		ext = strrchr(namelist[i]->d_name, '.');
		if (ext == NULL) {
			// should not happen but whatever
			free(namelist[i]);
			namelist[i] = NULL;
			continue;
		}
		ext++;

		// first find .cue files and remove files they reference
		if (strcasecmp(ext, "cue") == 0)
		{
			snprintf(buf, sizeof(buf), "%s/%s", basedir,
				namelist[i]->d_name);

			f = fopen(buf, "r");
			if (f == NULL) {
				free(namelist[i]);
				namelist[i] = NULL;
				continue;
			}

			good_cue = 0;
			while (fgets(buf, sizeof(buf), f)) {
				ret = sscanf(buf, " FILE \"%256[^\"]\"", buf2);
				if (ret != 1)
					ret = sscanf(buf, " FILE %256s", buf2);
				if (ret != 1)
					continue;

				p = strrchr(buf2, '/');
				if (p == NULL)
					p = strrchr(buf2, '\\');
				if (p != NULL)
					p++;
				else
					p = buf2;

				snprintf(buf, sizeof(buf), "%s/%s", basedir, p);
				ret = STAT(buf, &statf);
				if (ret == 0) {
					rm_namelist_entry(namelist, count, p);
					good_cue = 1;
				}
			}
			fclose(f);

			if (!good_cue) {
				free(namelist[i]);
				namelist[i] = NULL;
			}
			continue;
		}

		p = strcasestr(namelist[i]->d_name, "track");
		if (p != NULL) {
			ret = strtoul(p + 5, NULL, 10);
			if (ret > 1) {
				free(namelist[i]);
				namelist[i] = NULL;
				continue;
			}
		}
	}

	// compact namelist
	for (i = d = 1; i < count; i++)
		if (namelist[i] != NULL)
			namelist[d++] = namelist[i];

	return d;
}

// propagate menu settings to the emu vars
static void menu_sync_config(void)
{
	static int allow_abs_only_old;

	Config.PsxAuto = 1;
	if (region > 0) {
		Config.PsxAuto = 0;
		Config.PsxType = region - 1;
	}
	Config.cycle_multiplier = 10000 / psx_clock;

	switch (in_type_sel1) {
	case 1:  in_type[0] = PSE_PAD_TYPE_ANALOGPAD; break;
	case 2:  in_type[0] = PSE_PAD_TYPE_GUNCON;    break;
	case 3:  in_type[0] = PSE_PAD_TYPE_GUN;       break;
	case 4:  in_type[0] = PSE_PAD_TYPE_NONE;      break;
	default: in_type[0] = PSE_PAD_TYPE_STANDARD;
	}
	switch (in_type_sel2) {
	case 1:  in_type[1] = PSE_PAD_TYPE_ANALOGPAD; break;
	case 2:  in_type[1] = PSE_PAD_TYPE_GUNCON;    break;
	case 3:  in_type[1] = PSE_PAD_TYPE_GUN;       break;
	case 4:  in_type[1] = PSE_PAD_TYPE_NONE;      break;
	default: in_type[1] = PSE_PAD_TYPE_STANDARD;
	}
	if (in_evdev_allow_abs_only != allow_abs_only_old) {
		in_probe();
		allow_abs_only_old = in_evdev_allow_abs_only;
	}

	spu_config.iVolume = 768 + 128 * volume_boost;
	pl_rearmed_cbs.frameskip = frameskip - 1;
	pl_timing_prepare(Config.PsxType);
}

static void menu_set_defconfig(void)
{
	emu_set_default_config();

	g_opts = 0;
	g_scaler = SCALE_4_3;
	g_gamma = 100;
	volume_boost = 0;
	frameskip = 1; // 1 - off
	analog_deadzone = 50;
	soft_scaling = 1;
	soft_filter = 0;
	scanlines = 0;
	scanline_level = 20;
	plat_target.vout_fullscreen = 0;
	psx_clock = DEFAULT_PSX_CLOCK;

	region = 0;
	in_type_sel1 = in_type_sel2 = 0;
	in_evdev_allow_abs_only = 0;

	menu_sync_config();
}

#define CE_CONFIG_STR(val) \
	{ #val, 0, Config.val }

#define CE_CONFIG_VAL(val) \
	{ #val, sizeof(Config.val), &Config.val }

#define CE_STR(val) \
	{ #val, 0, val }

#define CE_INTVAL(val) \
	{ #val, sizeof(val), &val }

#define CE_INTVAL_N(name, val) \
	{ name, sizeof(val), &val }

#define CE_INTVAL_P(val) \
	{ #val, sizeof(pl_rearmed_cbs.val), &pl_rearmed_cbs.val }

// 'versioned' var, used when defaults change
#define CE_CONFIG_STR_V(val, ver) \
	{ #val #ver, 0, Config.val }

#define CE_INTVAL_V(val, ver) \
	{ #val #ver, sizeof(val), &val }

#define CE_INTVAL_PV(val, ver) \
	{ #val #ver, sizeof(pl_rearmed_cbs.val), &pl_rearmed_cbs.val }

static const struct {
	const char *name;
	size_t len;
	void *val;
} config_data[] = {
	CE_CONFIG_STR(Bios),
	CE_CONFIG_STR_V(Gpu, 3),
	CE_CONFIG_STR(Spu),
//	CE_CONFIG_STR(Cdr),
	CE_CONFIG_VAL(Xa),
	CE_CONFIG_VAL(Mdec),
	CE_CONFIG_VAL(Cdda),
	CE_CONFIG_VAL(Debug),
	CE_CONFIG_VAL(PsxOut),
	CE_CONFIG_VAL(icache_emulation),
	CE_CONFIG_VAL(DisableStalls),
	CE_CONFIG_VAL(Cpu),
	CE_CONFIG_VAL(GpuListWalking),
	CE_CONFIG_VAL(FractionalFramerate),
	CE_CONFIG_VAL(PreciseExceptions),
	CE_CONFIG_VAL(TurboCD),
	CE_CONFIG_VAL(SlowBoot),
	CE_INTVAL(region),
	CE_INTVAL_V(g_scaler, 3),
	CE_INTVAL(g_gamma),
	CE_INTVAL(g_layer_x),
	CE_INTVAL(g_layer_y),
	CE_INTVAL(g_layer_w),
	CE_INTVAL(g_layer_h),
	CE_INTVAL(soft_filter),
	CE_INTVAL(scanlines),
	CE_INTVAL(scanline_level),
	CE_INTVAL(plat_target.vout_method),
	CE_INTVAL(plat_target.hwfilter),
	CE_INTVAL(plat_target.vout_fullscreen),
	CE_INTVAL(state_slot),
	CE_INTVAL(cpu_clock),
	CE_INTVAL(g_opts),
	CE_INTVAL(in_type_sel1),
	CE_INTVAL(in_type_sel2),
	CE_INTVAL(analog_deadzone),
	CE_INTVAL(memcard1_sel),
	CE_INTVAL(memcard2_sel),
	CE_INTVAL(g_autostateld_opt),
	CE_INTVAL(cd_buf_count),
	CE_INTVAL_N("adev0_axis0", in_adev_axis[0][0]),
	CE_INTVAL_N("adev0_axis1", in_adev_axis[0][1]),
	CE_INTVAL_N("adev1_axis0", in_adev_axis[1][0]),
	CE_INTVAL_N("adev1_axis1", in_adev_axis[1][1]),
	CE_INTVAL_N("adev0_is_nublike", in_adev_is_nublike[0]),
	CE_INTVAL_N("adev1_is_nublike", in_adev_is_nublike[1]),
	CE_INTVAL_V(frameskip, 4),
	CE_INTVAL_PV(dithering, 2),
	CE_INTVAL_P(gpu_peops.dwActFixes),
	CE_INTVAL_P(gpu_unai.old_renderer),
	CE_INTVAL_P(gpu_unai.ilace_force),
	CE_INTVAL_P(gpu_unai.lighting),
	CE_INTVAL_P(gpu_unai.fast_lighting),
	CE_INTVAL_P(gpu_unai.blending),
	CE_INTVAL_P(gpu_unai.scale_hires),
	CE_INTVAL_P(gpu_neon.allow_interlace),
	CE_INTVAL_P(gpu_neon.enhancement_enable),
	CE_INTVAL_P(gpu_neon.enhancement_no_main),
	CE_INTVAL_PV(gpu_neon.enhancement_tex_adj, 2),
	CE_INTVAL_P(gpu_peopsgl.bDrawDither),
	CE_INTVAL_P(gpu_peopsgl.iFilterType),
	CE_INTVAL_P(gpu_peopsgl.iFrameTexType),
	CE_INTVAL_P(gpu_peopsgl.iUseMask),
	CE_INTVAL_P(gpu_peopsgl.bOpaquePass),
	CE_INTVAL_P(gpu_peopsgl.bAdvancedBlend),
	CE_INTVAL_P(gpu_peopsgl.bUseFastMdec),
	CE_INTVAL_P(gpu_peopsgl.iVRamSize),
	CE_INTVAL_P(gpu_peopsgl.iTexGarbageCollection),
	CE_INTVAL_P(gpu_peopsgl.dwActFixes),
	CE_INTVAL_P(screen_centering_type),
	CE_INTVAL_P(screen_centering_x),
	CE_INTVAL_P(screen_centering_y),
	CE_INTVAL_P(screen_centering_h_adj),
	CE_INTVAL_P(show_overscan),
	CE_INTVAL(spu_config.iUseReverb),
	CE_INTVAL(spu_config.iXAPitch),
	CE_INTVAL(spu_config.iUseInterpolation),
	CE_INTVAL(spu_config.iTempo),
	CE_INTVAL(spu_config.iUseThread),
	CE_INTVAL(config_save_counter),
	CE_INTVAL(in_evdev_allow_abs_only),
	CE_INTVAL(volume_boost),
	CE_INTVAL(psx_clock),
	CE_INTVAL(ndrc_g.hacks),
	CE_INTVAL(in_enable_vibration),
};

static char *get_cd_label(void)
{
	static char trimlabel[33];
	int j;

	strncpy(trimlabel, CdromLabel, 32);
	trimlabel[32] = 0;
	for (j = 31; j >= 0; j--)
		if (trimlabel[j] == ' ')
			trimlabel[j] = 0;

	return trimlabel;
}

static void make_cfg_fname(char *buf, size_t size, int is_game)
{
	char id_buf[64];
	if (is_game) {
		snprintf(id_buf, sizeof(id_buf), "%.32s-%.9s.cfg",
			get_cd_label(), CdromId);
		emu_make_path(buf, size, CFG_DIR, id_buf);
	}
	else
		emu_make_path(buf, size, PCSX_DOT_DIR, cfgfile_basename);
}

static void keys_write_all(FILE *f);
static char *mystrip(char *str);

static void write_u32_value(FILE *f, u32 v)
{
	if (v > 7)
		fprintf(f, "0x");
	fprintf(f, "%x\n", v);
}

static int menu_write_config(int is_game)
{
	char cfgfile[MAXPATHLEN];
	FILE *f;
	int i;

	config_save_counter++;

	make_cfg_fname(cfgfile, sizeof(cfgfile), is_game);
	f = fopen(cfgfile, "w");
	if (f == NULL) {
		printf("menu_write_config: failed to open: %s\n", cfgfile);
		return -1;
	}

	cd_buf_count = cdra_get_buf_count();

	for (i = 0; i < ARRAY_SIZE(config_data); i++) {
		fprintf(f, "%s = ", config_data[i].name);
		switch (config_data[i].len) {
		case 0:
			fprintf(f, "%s\n", (char *)config_data[i].val);
			break;
		case 1:
			write_u32_value(f, *(u8 *)config_data[i].val);
			break;
		case 2:
			write_u32_value(f, *(u16 *)config_data[i].val);
			break;
		case 4:
			write_u32_value(f, *(u32 *)config_data[i].val);
			break;
		default:
			printf("menu_write_config: unhandled len %d for %s\n",
				 (int)config_data[i].len, config_data[i].name);
			break;
		}
	}

	keys_write_all(f);
	fclose(f);

	return 0;
}

static int menu_do_last_cd_img(int is_get)
{
	static const char *defaults[] = { "/media", "/mnt/sd", "/mnt" };
	char path[256];
	struct STAT st;
	FILE *f;
	int i, ret = -1;

	emu_make_path(path, sizeof(path), PCSX_DOT_DIR, "lastcdimg.txt");
	f = fopen(path, is_get ? "r" : "w");
	if (f == NULL) {
		ret = -1;
		goto out;
	}

	if (is_get) {
		ret = fread(last_selected_fname, 1, sizeof(last_selected_fname) - 1, f);
		last_selected_fname[ret] = 0;
		mystrip(last_selected_fname);
	}
	else
		fprintf(f, "%s\n", last_selected_fname);
	fclose(f);

out:
	if (is_get) {
		for (i = 0; last_selected_fname[0] == 0
		       || STAT(last_selected_fname, &st) != 0; i++)
		{
			if (i >= ARRAY_SIZE(defaults))
				break;
			strcpy(last_selected_fname, defaults[i]);
		}
	}

	return 0;
}

static void parse_str_val(char *cval, const char *src)
{
	char *tmp;
	strncpy(cval, src, MAXPATHLEN);
	cval[MAXPATHLEN - 1] = 0;
	tmp = strchr(cval, '\n');
	if (tmp == NULL)
		tmp = strchr(cval, '\r');
	if (tmp != NULL)
		*tmp = 0;
}

static void keys_load_all(const char *cfg);

int menu_load_config(int is_game)
{
	char cfgfile[MAXPATHLEN];
	int i, ret = -1;
	long size;
	char *cfg;
	FILE *f;

	make_cfg_fname(cfgfile, sizeof(cfgfile), is_game);
	f = fopen(cfgfile, "r");
	if (f == NULL) {
		printf("menu_load_config: failed to open: %s\n", cfgfile);
		goto fail;
	}

	fseek(f, 0, SEEK_END);
	size = ftell(f);
	if (size <= 0) {
		printf("bad size %ld: %s\n", size, cfgfile);
		goto fail;
	}

	cfg = malloc(size + 1);
	if (cfg == NULL)
		goto fail;

	fseek(f, 0, SEEK_SET);
	if (fread(cfg, 1, size, f) != size) {
		printf("failed to read: %s\n", cfgfile);
		goto fail_read;
	}
	cfg[size] = 0;

	for (i = 0; i < ARRAY_SIZE(config_data); i++) {
		char *tmp, *tmp2;
		u32 val;

		tmp = strstr(cfg, config_data[i].name);
		if (tmp == NULL)
			continue;
		tmp += strlen(config_data[i].name);
		if (strncmp(tmp, " = ", 3) != 0)
			continue;
		tmp += 3;

		if (config_data[i].len == 0) {
			parse_str_val(config_data[i].val, tmp);
			continue;
		}

		tmp2 = NULL;
		val = strtoul(tmp, &tmp2, 16);
		if (tmp2 == NULL || tmp == tmp2)
			continue; // parse failed

		switch (config_data[i].len) {
		case 1:
			*(u8 *)config_data[i].val = val;
			break;
		case 2:
			*(u16 *)config_data[i].val = val;
			break;
		case 4:
			*(u32 *)config_data[i].val = val;
			break;
		default:
			printf("menu_load_config: unhandled len %d for %s\n",
				 (int)config_data[i].len, config_data[i].name);
			break;
		}
	}

	if (!is_game) {
		char *tmp = strstr(cfg, "lastcdimg = ");
		if (tmp != NULL) {
			tmp += 12;
			parse_str_val(last_selected_fname, tmp);
		}
	}

	keys_load_all(cfg);
	cdra_set_buf_count(cd_buf_count);
	ret = 0;
fail_read:
	free(cfg);
fail:
	if (f != NULL)
		fclose(f);

	menu_sync_config();

	// sync plugins
	for (i = bios_sel = 0; bioses[i] != NULL; i++)
		if (strcmp(Config.Bios, bioses[i]) == 0)
			{ bios_sel = i; break; }

	for (i = gpu_plugsel = 0; gpu_plugins[i] != NULL; i++)
		if (strcmp(Config.Gpu, gpu_plugins[i]) == 0)
			{ gpu_plugsel = i; break; }

	for (i = spu_plugsel = 0; spu_plugins[i] != NULL; i++)
		if (strcmp(Config.Spu, spu_plugins[i]) == 0)
			{ spu_plugsel = i; break; }

	// memcard selections
	char mcd1_old[sizeof(Config.Mcd1)];
	char mcd2_old[sizeof(Config.Mcd2)];
	strcpy(mcd1_old, Config.Mcd1);
	strcpy(mcd2_old, Config.Mcd2);

	if ((unsigned int)memcard1_sel < ARRAY_SIZE(memcards)) {
		if (memcard1_sel == 0)
			strcpy(Config.Mcd1, "none");
		else if (memcards[memcard1_sel] != NULL)
			snprintf(Config.Mcd1, sizeof(Config.Mcd1), ".%s%s",
				MEMCARD_DIR, memcards[memcard1_sel]);
	}
	if ((unsigned int)memcard2_sel < ARRAY_SIZE(memcards)) {
		if (memcard2_sel == 0)
			strcpy(Config.Mcd2, "none");
		else if (memcards[memcard2_sel] != NULL)
			snprintf(Config.Mcd2, sizeof(Config.Mcd2), ".%s%s",
				MEMCARD_DIR, memcards[memcard2_sel]);
	}
	if (strcmp(mcd1_old, Config.Mcd1) || strcmp(mcd2_old, Config.Mcd2))
		LoadMcds(Config.Mcd1, Config.Mcd2);

	return ret;
}

static const char *filter_exts[] = {
	"bin", "img", "mdf", "iso", "cue", "z",
	#ifdef HAVE_CHD
	"chd",
	#endif
	"bz",  "znx", "pbp", "cbn", "ppf", NULL
};

// rrrr rggg gggb bbbb
static unsigned short fname2color(const char *fname)
{
	static const char *other_exts[] = {
		"ccd", "toc", "mds", "sub", "table", "index", "sbi"
	};
	const char *ext = strrchr(fname, '.');
	int i;

	if (ext == NULL)
		return 0xffff;
	ext++;
	for (i = 0; filter_exts[i] != NULL; i++)
		if (strcasecmp(ext, filter_exts[i]) == 0)
			return 0x7bff;
	for (i = 0; i < array_size(other_exts); i++)
		if (strcasecmp(ext, other_exts[i]) == 0)
			return 0xa514;
	return 0xffff;
}

static void draw_savestate_bg(int slot);

#define MENU_ALIGN_LEFT
#ifndef HAVE_PRE_ARMV7 // assume hires device
#define MENU_X2 1
#else
#define MENU_X2 0
#endif

#include "libpicofe/menu.c"

// a bit of black magic here
static void draw_savestate_bg(int slot)
{
	static const int psx_widths[8]  = { 256, 368, 320, 384, 512, 512, 640, 640 };
	int x, y, w, h;
	char fname[MAXPATHLEN];
	GPUFreeze_t *gpu;
	u16 *s, *d;
	gzFile f;
	int ret;
	u32 tmp;

	ret = get_state_filename(fname, sizeof(fname), slot);
	if (ret != 0)
		return;

	f = gzopen(fname, "rb");
	if (f == NULL)
		return;

	if ((ret = (int)gzseek(f, 0x29933d, SEEK_SET)) != 0x29933d) {
		fprintf(stderr, "gzseek failed: %d\n", ret);
		gzclose(f);
		return;
	}

	gpu = malloc(sizeof(*gpu));
	if (gpu == NULL) {
		gzclose(f);
		return;
	}

	ret = gzread(f, gpu, sizeof(*gpu));
	gzclose(f);
	if (ret != sizeof(*gpu)) {
		fprintf(stderr, "gzread failed\n");
		goto out;
	}

	memcpy(g_menubg_ptr, g_menubg_src_ptr, g_menuscreen_w * g_menuscreen_h * 2);

	if (gpu->ulStatus & 0x800000)
		goto out; // disabled

	x = gpu->ulControl[5] & 0x3ff;
	y = (gpu->ulControl[5] >> 10) & 0x1ff;
	w = psx_widths[(gpu->ulStatus >> 16) & 7];
	tmp = gpu->ulControl[7];
	h = ((tmp >> 10) & 0x3ff) - (tmp & 0x3ff);
	if (gpu->ulStatus & 0x80000) // doubleheight
		h *= 2;
	if (h <= 0 || h > 512)
		goto out;
	if (y > 512 - 64)
		y = 0;
	if (y + h > 512)
		h = 512 - y;
	s = (u16 *)gpu->psxVRam + y * 1024 + x;

	x = max(0, g_menuscreen_w - w) & ~3;
	y = max(0, g_menuscreen_h / 2 - h / 2);
	w = min(g_menuscreen_w, w);
	h = min(g_menuscreen_h, h);
	d = (u16 *)g_menubg_ptr + g_menuscreen_w * y + x;

	for (; h > 0; h--, d += g_menuscreen_w, s += 1024) {
		if (gpu->ulStatus & 0x200000)
			bgr888_to_rgb565(d, s, w * 3);
		else
			bgr555_to_rgb565(d, s, w * 2);

		// darken this so that menu text is visible
		if (g_menuscreen_w - w < 320)
			menu_darken_bg(d, d, w, 0);
	}

out:
	free(gpu);
}

// -------------- key config --------------

me_bind_action me_ctrl_actions[] =
{
	{ "UP      ", 1 << DKEY_UP},
	{ "DOWN    ", 1 << DKEY_DOWN },
	{ "LEFT    ", 1 << DKEY_LEFT },
	{ "RIGHT   ", 1 << DKEY_RIGHT },
	{ "TRIANGLE", 1 << DKEY_TRIANGLE },
	{ "CIRCLE  ", 1 << DKEY_CIRCLE },
	{ "CROSS   ", 1 << DKEY_CROSS },
	{ "SQUARE  ", 1 << DKEY_SQUARE },
	{ "L1      ", 1 << DKEY_L1 },
	{ "R1      ", 1 << DKEY_R1 },
	{ "L2      ", 1 << DKEY_L2 },
	{ "R2      ", 1 << DKEY_R2 },
	{ "L3      ", 1 << DKEY_L3 },
	{ "R3      ", 1 << DKEY_R3 },
	{ "START   ", 1 << DKEY_START },
	{ "SELECT  ", 1 << DKEY_SELECT },
	{ NULL,       0 }
};

me_bind_action emuctrl_actions[] =
{
	{ "Save State       ", 1 << SACTION_SAVE_STATE },
	{ "Load State       ", 1 << SACTION_LOAD_STATE },
	{ "Prev Save Slot   ", 1 << SACTION_PREV_SSLOT },
	{ "Next Save Slot   ", 1 << SACTION_NEXT_SSLOT },
	{ "Toggle Frameskip ", 1 << SACTION_TOGGLE_FSKIP },
	{ "Take Screenshot  ", 1 << SACTION_SCREENSHOT },
	{ "Show/Hide FPS    ", 1 << SACTION_TOGGLE_FPS },
#ifndef HAVE_PRE_ARMV7
	{ "Switch Renderer  ", 1 << SACTION_SWITCH_DISPMODE },
#endif
	{ "Fast Forward     ", 1 << SACTION_FAST_FORWARD },
#if MENU_SHOW_MINIMIZE
	{ "Minimize         ", 1 << SACTION_MINIMIZE },
#endif
#if MENU_SHOW_FULLSCREEN
	{ "Toggle fullscreen", 1 << SACTION_TOGGLE_FULLSCREEN },
#endif
	{ "Enter Menu       ", 1 << SACTION_ENTER_MENU },
	{ "Gun Trigger      ", 1 << SACTION_GUN_TRIGGER },
	{ "Gun A button     ", 1 << SACTION_GUN_A },
	{ "Gun B button     ", 1 << SACTION_GUN_B },
	{ "Gun Offscreen Trigger", 1 << SACTION_GUN_TRIGGER2 },
#if MENU_SHOW_VOLUME
	{ "Volume Up        ", 1 << SACTION_VOLUME_UP },
	{ "Volume Down      ", 1 << SACTION_VOLUME_DOWN },
#endif
	{ "Analog toggle    ", 1 << SACTION_ANALOG_TOGGLE },
	{ NULL,                0 }
};

static char *mystrip(char *str)
{
	int i, len;

	len = strlen(str);
	for (i = 0; i < len; i++)
		if (str[i] != ' ') break;
	if (i > 0) memmove(str, str + i, len - i + 1);

	len = strlen(str);
	for (i = len - 1; i >= 0; i--)
		if (str[i] != ' ' && str[i] != '\r' && str[i] != '\n') break;
	str[i+1] = 0;

	return str;
}

static void get_line(char *d, size_t size, const char *s)
{
	const char *pe;
	size_t len;

	for (pe = s; *pe != '\r' && *pe != '\n' && *pe != 0; pe++)
		;
	len = pe - s;
	if (len > size - 1)
		len = size - 1;
	strncpy(d, s, len);
	d[len] = 0;
}

static void keys_write_all(FILE *f)
{
	int d;

	for (d = 0; d < IN_MAX_DEVS; d++)
	{
		const int *binds = in_get_dev_binds(d);
		const char *name = in_get_dev_name(d, 0, 0);
		int k, count = 0;

		if (binds == NULL || name == NULL)
			continue;

		fprintf(f, "binddev = %s\n", name);
		in_get_config(d, IN_CFG_BIND_COUNT, &count);

		for (k = 0; k < count; k++)
		{
			int i, kbinds, mask;
			char act[32];

			act[0] = act[31] = 0;
			name = in_get_key_name(d, k);

			kbinds = binds[IN_BIND_OFFS(k, IN_BINDTYPE_PLAYER12)];
			for (i = 0; kbinds && i < ARRAY_SIZE(me_ctrl_actions) - 1; i++) {
				mask = me_ctrl_actions[i].mask;
				if (mask & kbinds) {
					strncpy(act, me_ctrl_actions[i].name, 31);
					fprintf(f, "bind %s = player1 %s\n", name, mystrip(act));
					kbinds &= ~mask;
				}
				mask = me_ctrl_actions[i].mask << 16;
				if (mask & kbinds) {
					strncpy(act, me_ctrl_actions[i].name, 31);
					fprintf(f, "bind %s = player2 %s\n", name, mystrip(act));
					kbinds &= ~mask;
				}
			}

			kbinds = binds[IN_BIND_OFFS(k, IN_BINDTYPE_EMU)];
			for (i = 0; kbinds && emuctrl_actions[i].name != NULL; i++) {
				mask = emuctrl_actions[i].mask;
				if (mask & kbinds) {
					strncpy(act, emuctrl_actions[i].name, 31);
					fprintf(f, "bind %s = %s\n", name, mystrip(act));
					kbinds &= ~mask;
				}
			}
		}

		for (k = 0; k < array_size(in_adev); k++)
		{
			if (in_adev[k] == d)
				fprintf(f, "bind_analog = %d\n", k);
		}
	}
}

static int parse_bind_val(const char *val, int *type)
{
	int i;

	*type = IN_BINDTYPE_NONE;
	if (val[0] == 0)
		return 0;
	
	if (strncasecmp(val, "player", 6) == 0)
	{
		int player, shift = 0;
		player = atoi(val + 6) - 1;

		if ((unsigned int)player > 1)
			return -1;
		if (player == 1)
			shift = 16;

		*type = IN_BINDTYPE_PLAYER12;
		for (i = 0; me_ctrl_actions[i].name != NULL; i++) {
			if (strncasecmp(me_ctrl_actions[i].name, val + 8, strlen(val + 8)) == 0)
				return me_ctrl_actions[i].mask << shift;
		}
	}
	for (i = 0; emuctrl_actions[i].name != NULL; i++) {
		if (strncasecmp(emuctrl_actions[i].name, val, strlen(val)) == 0) {
			*type = IN_BINDTYPE_EMU;
			return emuctrl_actions[i].mask;
		}
	}

	return -1;
}

static void keys_load_all(const char *cfg)
{
	char dev[256], key[128], *act;
	const char *p;
	int bind, bindtype;
	int ret, dev_id;

	p = cfg;
	while (p != NULL && (p = strstr(p, "binddev = ")) != NULL) {
		p += 10;

		// don't strip 'dev' because there are weird devices
		// with names with space at the end
		get_line(dev, sizeof(dev), p);

		dev_id = in_config_parse_dev(dev);
		if (dev_id < 0) {
			printf("input: can't handle dev: %s\n", dev);
			continue;
		}

		in_unbind_all(dev_id, -1, -1);
		while ((p = strstr(p, "bind"))) {
			if (strncmp(p, "binddev = ", 10) == 0)
				break;

			if (strncmp(p, "bind_analog", 11) == 0) {
				ret = sscanf(p, "bind_analog = %d", &bind);
				p += 11;
				if (ret != 1) {
					printf("input: parse error: %16s..\n", p);
					continue;
				}
				if ((unsigned int)bind >= array_size(in_adev)) {
					printf("input: analog id %d out of range\n", bind);
					continue;
				}
				in_adev[bind] = dev_id;
				continue;
			}

			p += 4;
			if (*p != ' ') {
				printf("input: parse error: %16s..\n", p);
				continue;
			}

			get_line(key, sizeof(key), p);
			act = strchr(key, '=');
			if (act == NULL) {
				printf("parse failed: %16s..\n", p);
				continue;
			}
			*act = 0;
			act++;
			mystrip(key);
			mystrip(act);

			bind = parse_bind_val(act, &bindtype);
			if (bind != -1 && bind != 0) {
				//printf("bind #%d '%s' %08x (%s)\n", dev_id, key, bind, act);
				in_config_bind_key(dev_id, key, bind, bindtype);
			}
			else
				lprintf("config: unhandled action \"%s\"\n", act);
		}
	}
	in_clean_binds();
}

static int key_config_loop_wrap(int id, int keys)
{
	int d;

	for (d = 0; d < IN_MAX_DEVS; d++)
		in_set_config_int(d, IN_CFG_ANALOG_MAP_ULDR, 0);
	switch (id) {
		case MA_CTRL_PLAYER1:
			key_config_loop(me_ctrl_actions, array_size(me_ctrl_actions) - 1, 0);
			break;
		case MA_CTRL_PLAYER2:
			key_config_loop(me_ctrl_actions, array_size(me_ctrl_actions) - 1, 1);
			break;
		case MA_CTRL_EMU:
			key_config_loop(emuctrl_actions, array_size(emuctrl_actions) - 1, -1);
			break;
		default:
			break;
	}
	for (d = 0; d < IN_MAX_DEVS; d++)
		in_set_config_int(d, IN_CFG_ANALOG_MAP_ULDR, 1);

	return 0;
}

static const char h_nubmode[] = "Maps nub-like analog controls to PSX ones better\n"
				"Might cause problems with real analog sticks";
static const char *adevnames[IN_MAX_DEVS + 2];
static int stick_sel[2];

static menu_entry e_menu_keyconfig_analog[] =
{
	mee_enum   ("Left stick (L3)",  0, stick_sel[0], adevnames),
	mee_range  ("  X axis",    0, in_adev_axis[0][0], 0, 7),
	mee_range  ("  Y axis",    0, in_adev_axis[0][1], 0, 7),
	mee_onoff_h("  nub mode",  0, in_adev_is_nublike[0], 1, h_nubmode),
	mee_enum   ("Right stick (R3)", 0, stick_sel[1], adevnames),
	mee_range  ("  X axis",    0, in_adev_axis[1][0], 0, 7),
	mee_range  ("  Y axis",    0, in_adev_axis[1][1], 0, 7),
	mee_onoff_h("  nub mode",  0, in_adev_is_nublike[1], 1, h_nubmode),
	mee_end,
};

static int key_config_analog(int id, int keys)
{
	int i, d, count, sel = 0;
	int sel2dev_map[IN_MAX_DEVS];

	memset(adevnames, 0, sizeof(adevnames));
	memset(sel2dev_map, 0xff, sizeof(sel2dev_map));
	memset(stick_sel, 0, sizeof(stick_sel));

	adevnames[0] = "None";
	i = 1;
	for (d = 0; d < IN_MAX_DEVS; d++)
	{
		const char *name = in_get_dev_name(d, 0, 1);
		if (name == NULL)
			continue;

		count = 0;
		in_get_config(d, IN_CFG_ABS_AXIS_COUNT, &count);
		if (count == 0)
			continue;

		if (in_adev[0] == d) stick_sel[0] = i;
		if (in_adev[1] == d) stick_sel[1] = i;
		sel2dev_map[i] = d;
		adevnames[i++] = name;
	}
	adevnames[i] = NULL;

	me_loop(e_menu_keyconfig_analog, &sel);

	in_adev[0] = sel2dev_map[stick_sel[0]];
	in_adev[1] = sel2dev_map[stick_sel[1]];

	return 0;
}

static const char *mgn_dev_name(int id, int *offs)
{
	const char *name = NULL;
	static int it = 0;

	if (id == MA_CTRL_DEV_FIRST)
		it = 0;

	for (; it < IN_MAX_DEVS; it++) {
		name = in_get_dev_name(it, 1, 1);
		if (name != NULL)
			break;
	}

	it++;
	return name;
}

static const char *mgn_saveloadcfg(int id, int *offs)
{
	return "";
}

static int mh_savecfg(int id, int keys)
{
	if (menu_write_config(id == MA_OPT_SAVECFG_GAME ? 1 : 0) == 0)
		menu_update_msg("config saved");
	else
		menu_update_msg("failed to write config");

	return 1;
}

static int mh_input_rescan(int id, int keys)
{
	//menu_sync_config();
	in_probe();
	menu_update_msg("rescan complete.");

	return 0;
}

static const char *men_in_type_sel[] = {
	"Standard (SCPH-1080)",
	"Analog (SCPH-1150)",
	"GunCon",
	"Konami Gun",
	"None",
	NULL
};
static const char h_nub_btns[] = "Experimental, keep this OFF if unsure. Select rescan after change.";
static const char h_notsgun[]  = "Don't trigger (shoot) when touching screen in gun games.";
static const char h_vibration[]= "Must select analog above and enable this ingame too.";

static menu_entry e_menu_keyconfig[] =
{
	mee_handler_id("Player 1",              MA_CTRL_PLAYER1,    key_config_loop_wrap),
	mee_handler_id("Player 2",              MA_CTRL_PLAYER2,    key_config_loop_wrap),
	mee_handler_id("Analog controls",       MA_CTRL_ANALOG,     key_config_analog),
	mee_handler_id("Emulator/Gun controls", MA_CTRL_EMU,        key_config_loop_wrap),
	mee_label     (""),
	mee_enum      ("Port 1 device",     0, in_type_sel1,    men_in_type_sel),
	mee_enum      ("Port 2 device",     0, in_type_sel2,    men_in_type_sel),
	mee_onoff_h   ("Nubs as buttons",   MA_CTRL_NUBS_BTNS,  in_evdev_allow_abs_only, 1, h_nub_btns),
	mee_onoff_h   ("Vibration",         MA_CTRL_VIBRATION,  in_enable_vibration, 1, h_vibration),
	mee_range     ("Analog deadzone",   MA_CTRL_DEADZONE,   analog_deadzone, 1, 99),
	mee_onoff_h   ("No TS Gun trigger", 0, g_opts, OPT_TSGUN_NOTRIGGER, h_notsgun),
	mee_cust_nosave("Save global config",       MA_OPT_SAVECFG,      mh_savecfg, mgn_saveloadcfg),
	mee_cust_nosave("Save cfg for loaded game", MA_OPT_SAVECFG_GAME, mh_savecfg, mgn_saveloadcfg),
	mee_handler   ("Rescan devices:",  mh_input_rescan),
	mee_label     (""),
	mee_label_mk  (MA_CTRL_DEV_FIRST, mgn_dev_name),
	mee_label_mk  (MA_CTRL_DEV_NEXT,  mgn_dev_name),
	mee_label_mk  (MA_CTRL_DEV_NEXT,  mgn_dev_name),
	mee_label_mk  (MA_CTRL_DEV_NEXT,  mgn_dev_name),
	mee_label_mk  (MA_CTRL_DEV_NEXT,  mgn_dev_name),
	mee_label_mk  (MA_CTRL_DEV_NEXT,  mgn_dev_name),
	mee_label_mk  (MA_CTRL_DEV_NEXT,  mgn_dev_name),
	mee_end,
};

static int menu_loop_keyconfig(int id, int keys)
{
	static int sel = 0;

//	me_enable(e_menu_keyconfig, MA_OPT_SAVECFG_GAME, ready_to_go && CdromId[0]);
	me_loop(e_menu_keyconfig, &sel);
	return 0;
}

// ------------ gfx options menu ------------

static const char *men_scaler[] = {
	"1x1", "integer scaled 2x", "scaled 4:3", "integer scaled 4:3", "fullscreen",
#if MENU_SHOW_VARSCALER_C
	"custom",
#endif
	NULL
};
static const char *men_soft_filter[] = { "None",
#ifdef HAVE_NEON32
	"scale2x", "eagle2x",
#endif
	NULL };
static const char *men_dummy[] = { NULL };
static const char *men_centering[] = { "Auto", "Ingame", "Borderless", "Force", NULL };
static const char *men_overscan[] = { "OFF", "Auto", "Hack", NULL };
static const char h_scaler[]    = "int. 2x  - scales w. or h. 2x if it fits on screen\n"
				  "int. 4:3 - uses integer if possible, else fractional";
static const char h_cscaler[]   = "Displays the scaler layer, you can resize it\n"
				  "using d-pad or move it using R+d-pad";
static const char h_soft_filter[] = "Works only if game uses low resolution modes";
static const char h_gamma[]     = "Gamma/brightness adjustment (default 100)";
#ifdef HAVE_NEON32
static const char *men_scanlines[] = { "OFF", "1", "2", "3", NULL };
static const char h_scanline_l[]  = "Scanline brightness, 0-100%";
#endif

static int menu_loop_cscaler(int id, int keys)
{
	void *saved_layer = NULL;
	size_t saved_layer_size = 0;
	int was_layer_clipped = 0;
	unsigned int inp;

	if (!pl_vout_buf)
		return -1;

	g_scaler = SCALE_CUSTOM;
	saved_layer_size = last_vout_w * last_vout_h * last_vout_bpp / 8;
	saved_layer = malloc(saved_layer_size);
	if (saved_layer)
		memcpy(saved_layer, pl_vout_buf, saved_layer_size);

	plat_gvideo_open(Config.PsxType);

	menu_draw_begin(0, 1);
	memset(g_menuscreen_ptr, 4, g_menuscreen_w * g_menuscreen_h * 2);
	menu_draw_end();

	for (;;)
	{
		if (saved_layer && last_vout_bpp == 16) {
			int top_x = max(0, -g_layer_x * last_vout_w / 800) + 1;
			int top_y = max(0, -g_layer_y * last_vout_h / 480) + 1;
			char text[128];
			memcpy(pl_vout_buf, saved_layer, saved_layer_size);
			snprintf(text, sizeof(text), "%d,%d %dx%d",
				g_layer_x, g_layer_y, g_layer_w, g_layer_h);
			basic_text_out16_nf(pl_vout_buf, last_vout_w,
				top_x, top_y, text);
			basic_text_out16_nf(pl_vout_buf, last_vout_w, 2,
				last_vout_h - 20, "d-pad: resize, R+d-pad: move");
			pl_vout_buf = plat_gvideo_flip();
		}

		inp = in_menu_wait(PBTN_UP|PBTN_DOWN|PBTN_LEFT|PBTN_RIGHT
				|PBTN_R|PBTN_MOK|PBTN_MBACK, NULL, 40);
		if (inp & PBTN_UP)    g_layer_y--;
		if (inp & PBTN_DOWN)  g_layer_y++;
		if (inp & PBTN_LEFT)  g_layer_x--;
		if (inp & PBTN_RIGHT) g_layer_x++;
		if (!(inp & PBTN_R)) {
			if (inp & PBTN_UP)    g_layer_h += 2;
			if (inp & PBTN_DOWN)  g_layer_h -= 2;
			if (inp & PBTN_LEFT)  g_layer_w += 2;
			if (inp & PBTN_RIGHT) g_layer_w -= 2;
		}
		if (inp & (PBTN_MOK|PBTN_MBACK))
			break;

		if (inp & (PBTN_UP|PBTN_DOWN|PBTN_LEFT|PBTN_RIGHT)) {
			int layer_clipped = 0;
			g_layer_x = max(-320, min(g_layer_x, 640));
			g_layer_y = max(-240, min(g_layer_y, 400));
			g_layer_w = max(160, g_layer_w);
			g_layer_h = max( 60, g_layer_h);
			if (g_layer_x < 0 || g_layer_x + g_layer_w > 800)
				layer_clipped = 1;
			if (g_layer_w > 800+400)
				g_layer_w = 800+400;
			if (g_layer_y < 0 || g_layer_y + g_layer_h > 480)
				layer_clipped = 1;
			if (g_layer_h > 480+360)
				g_layer_h = 480+360;
			// resize the layer
			plat_gvideo_open(Config.PsxType);
			if (layer_clipped || was_layer_clipped)
				pl_vout_buf = plat_gvideo_set_mode(&last_vout_w,
					&last_vout_h, &last_vout_bpp);
			was_layer_clipped = layer_clipped;
		}
	}

	plat_gvideo_close();
	free(saved_layer);

	return 0;
}

static menu_entry e_menu_gfx_options[] =
{
	mee_enum      ("PSX Screen centering",     MA_OPT_CENTERING, pl_rearmed_cbs.screen_centering_type, men_centering),
	mee_enum      ("Show overscan",            MA_OPT_OVERSCAN, pl_rearmed_cbs.show_overscan, men_overscan),
	mee_enum_h    ("Scaler",                   MA_OPT_VARSCALER, g_scaler, men_scaler, h_scaler),
	mee_enum      ("Video output mode",        MA_OPT_VOUT_MODE, plat_target.vout_method, men_dummy),
	mee_onoff     ("Fullscreen mode",          MA_OPT_VOUT_FULL, plat_target.vout_fullscreen, 1),
	mee_onoff     ("Software Scaling",         MA_OPT_SCALER2, soft_scaling, 1),
	mee_enum_h    ("Software Filter",          MA_OPT_SWFILTER, soft_filter, men_soft_filter, h_soft_filter),
	mee_enum      ("Hardware Filter",          MA_OPT_HWFILTER, plat_target.hwfilter, men_dummy),
#ifdef HAVE_NEON32
	mee_enum      ("Scanlines",                MA_OPT_SCANLINES, scanlines, men_scanlines),
	mee_range_h   ("Scanline brightness",      MA_OPT_SCANLINE_LEVEL, scanline_level, 0, 100, h_scanline_l),
#endif
	mee_range_h   ("Gamma adjustment",         MA_OPT_GAMMA, g_gamma, 1, 200, h_gamma),
	mee_onoff     ("OpenGL Vsync",             MA_OPT_VSYNC, g_opts, OPT_VSYNC),
	mee_cust_h    ("Setup custom scaler",      MA_OPT_VARSCALER_C, menu_loop_cscaler, NULL, h_cscaler),
	mee_end,
};

static int menu_loop_gfx_options(int id, int keys)
{
	static int sel = 0;

	me_loop(e_menu_gfx_options, &sel);

	return 0;
}

// ------------ bios/plugins ------------

static const char h_gpu_neon_enhanced[] =
	"Renders in double resolution at perf. cost\n"
	"(not available for high resolution games)";
static const char h_gpu_neon_enhanced_hack[] =
	"Speed hack for above option (glitches some games)";
static const char h_gpu_neon_enhanced_texadj[] =
	"Solves some Enh. res. texture issues, some perf hit";
static const char *men_gpu_interlace[] = { "Off", "On", "Auto", NULL };

static menu_entry e_menu_plugin_gpu_neon[] =
{
	mee_onoff_h   ("Enhanced resolution",        0, pl_rearmed_cbs.gpu_neon.enhancement_enable, 1, h_gpu_neon_enhanced),
	mee_onoff_h   ("Enhanced res. speed hack",   0, pl_rearmed_cbs.gpu_neon.enhancement_no_main, 1, h_gpu_neon_enhanced_hack),
	mee_onoff_h   ("Enh. res. texture adjust",   0, pl_rearmed_cbs.gpu_neon.enhancement_tex_adj, 1, h_gpu_neon_enhanced_texadj),
	mee_enum      ("Enable interlace mode",      0, pl_rearmed_cbs.gpu_neon.allow_interlace, men_gpu_interlace),
	mee_end,
};

static menu_entry e_menu_plugin_gpu_unai[] =
{
	mee_onoff     ("Old renderer",               0, pl_rearmed_cbs.gpu_unai.old_renderer, 1),
	mee_onoff     ("Skip every 2nd line",        0, pl_rearmed_cbs.gpu_unai.ilace_force, 1),
	mee_onoff     ("Lighting",                   0, pl_rearmed_cbs.gpu_unai.lighting, 1),
	mee_onoff     ("Fast lighting",              0, pl_rearmed_cbs.gpu_unai.fast_lighting, 1),
	mee_onoff     ("Blending",                   0, pl_rearmed_cbs.gpu_unai.blending, 1),
	mee_onoff     ("Downscale Hi-Res",           0, pl_rearmed_cbs.gpu_unai.scale_hires, 1),
	mee_end,
};

//static const char h_gpu_0[]            = "Needed for Chrono Cross";
static const char h_gpu_1[]            = "Capcom fighting games";
static const char h_gpu_2[]            = "Black screens in Lunar";
static const char h_gpu_3[]            = "Compatibility mode";
static const char h_gpu_6[]            = "Pandemonium 2";
//static const char h_gpu_7[]            = "Skip every second frame";
static const char h_gpu_8[]            = "Needed by Dark Forces";
static const char h_gpu_9[]            = "better g-colors, worse textures";
static const char h_gpu_10[]           = "Toggle busy flags after drawing";

static menu_entry e_menu_plugin_gpu_peops[] =
{
//	mee_onoff_h   ("Odd/even bit hack",          0, pl_rearmed_cbs.gpu_peops.dwActFixes, 1<<0, h_gpu_0),
	mee_onoff_h   ("Expand screen width",        0, pl_rearmed_cbs.gpu_peops.dwActFixes, 1<<1, h_gpu_1),
	mee_onoff_h   ("Ignore brightness color",    0, pl_rearmed_cbs.gpu_peops.dwActFixes, 1<<2, h_gpu_2),
	mee_onoff_h   ("Disable coordinate check",   0, pl_rearmed_cbs.gpu_peops.dwActFixes, 1<<3, h_gpu_3),
	mee_onoff_h   ("Lazy screen update",         0, pl_rearmed_cbs.gpu_peops.dwActFixes, 1<<6, h_gpu_6),
//	mee_onoff_h   ("Old frame skipping",         0, pl_rearmed_cbs.gpu_peops.dwActFixes, 1<<7, h_gpu_7),
	mee_onoff_h   ("Repeated flat tex triangles ",0,pl_rearmed_cbs.gpu_peops.dwActFixes, 1<<8, h_gpu_8),
	mee_onoff_h   ("Draw quads with triangles",  0, pl_rearmed_cbs.gpu_peops.dwActFixes, 1<<9, h_gpu_9),
	mee_onoff_h   ("Fake 'gpu busy' states",     0, pl_rearmed_cbs.gpu_peops.dwActFixes, 1<<10, h_gpu_10),
	mee_end,
};

static const char *men_peopsgl_texfilter[] = { "None", "Standard", "Extended",
	"Standard-sprites", "Extended-sprites", "Standard+sprites", "Extended+sprites", NULL };
static const char *men_peopsgl_fbtex[] = { "Emulated VRam", "Black", "Card", "Card+soft" };

static menu_entry e_menu_plugin_gpu_peopsgl[] =
{
	mee_onoff     ("Dithering",                  0, pl_rearmed_cbs.gpu_peopsgl.bDrawDither, 1),
	mee_enum      ("Texture Filtering",          0, pl_rearmed_cbs.gpu_peopsgl.iFilterType, men_peopsgl_texfilter),
	mee_enum      ("Framebuffer Textures",       0, pl_rearmed_cbs.gpu_peopsgl.iFrameTexType, men_peopsgl_fbtex),
	mee_onoff     ("Mask Detect",                0, pl_rearmed_cbs.gpu_peopsgl.iUseMask, 1),
	mee_onoff     ("Opaque Pass",                0, pl_rearmed_cbs.gpu_peopsgl.bOpaquePass, 1),
	mee_onoff     ("Advanced Blend",             0, pl_rearmed_cbs.gpu_peopsgl.bAdvancedBlend, 1),
	mee_onoff     ("Use Fast Mdec",              0, pl_rearmed_cbs.gpu_peopsgl.bUseFastMdec, 1),
	mee_range     ("Texture RAM size (MB)",      0, pl_rearmed_cbs.gpu_peopsgl.iVRamSize, 4, 128),
	mee_onoff     ("Texture garbage collection", 0, pl_rearmed_cbs.gpu_peopsgl.iTexGarbageCollection, 1),
	mee_label     ("Fixes/hacks:"),
	mee_onoff     ("FF7 cursor",                 0, pl_rearmed_cbs.gpu_peopsgl.dwActFixes, 1<<0),
	mee_onoff     ("Direct FB updates",          0, pl_rearmed_cbs.gpu_peopsgl.dwActFixes, 1<<1),
	mee_onoff     ("Black brightness",           0, pl_rearmed_cbs.gpu_peopsgl.dwActFixes, 1<<2),
	mee_onoff     ("Swap front detection",       0, pl_rearmed_cbs.gpu_peopsgl.dwActFixes, 1<<3),
	mee_onoff     ("Disable coord check",        0, pl_rearmed_cbs.gpu_peopsgl.dwActFixes, 1<<4),
	mee_onoff     ("No blue glitches (LoD)",     0, pl_rearmed_cbs.gpu_peopsgl.dwActFixes, 1<<5),
	mee_onoff     ("Soft FB access",             0, pl_rearmed_cbs.gpu_peopsgl.dwActFixes, 1<<6),
	mee_onoff     ("FF9 rect",                   0, pl_rearmed_cbs.gpu_peopsgl.dwActFixes, 1<<9),
	mee_onoff     ("No subtr. blending",         0, pl_rearmed_cbs.gpu_peopsgl.dwActFixes, 1<<10),
	mee_onoff     ("Lazy upload (DW7)",          0, pl_rearmed_cbs.gpu_peopsgl.dwActFixes, 1<<11),
	mee_onoff     ("Additional uploads",         0, pl_rearmed_cbs.gpu_peopsgl.dwActFixes, 1<<15),
	mee_end,
};

static const char *men_spu_interp[] = { "None", "Simple", "Gaussian", "Cubic", NULL };
static const char h_spu_volboost[]  = "Large values cause distortion";
static const char h_spu_tempo[]     = "Slows down audio if emu is too slow\n"
				      "This is inaccurate and breaks games";

static menu_entry e_menu_plugin_spu[] =
{
	mee_range_h   ("Volume boost",              0, volume_boost, -5, 30, h_spu_volboost),
	mee_onoff     ("Reverb",                    0, spu_config.iUseReverb, 1),
	mee_enum      ("Interpolation",             0, spu_config.iUseInterpolation, men_spu_interp),
	//mee_onoff     ("Adjust XA pitch",           0, spu_config.iXAPitch, 1),
	mee_onoff_h   ("Adjust tempo",              0, spu_config.iTempo, 1, h_spu_tempo),
	mee_end,
};

static int menu_loop_plugin_spu(int id, int keys)
{
	static int sel = 0;
	me_loop(e_menu_plugin_spu, &sel);
	return 0;
}

static const char *men_gpu_dithering[] = { "OFF", "ON", "Force", NULL };

static const char h_bios[]       = "HLE is simulated BIOS. BIOS selection is saved in\n"
				   "savestates and can't be changed there. Must save\n"
				   "config and reload the game for change to take effect";
static const char h_plugin_gpu[] = 
#if defined(BUILTIN_GPU_NEON)
				   "builtin_gpu is the NEON GPU, very fast and accurate\n"
#elif defined(BUILTIN_GPU_PEOPS)
				   "builtin_gpu is the P.E.Op.S GPU, slow but accurate\n"
#elif defined(BUILTIN_GPU_UNAI)
				   "builtin_gpu is the Unai GPU, very fast\n"
#endif
#ifndef NO_DYLIB
#if !defined(BUILTIN_GPU_NEON) && defined(GPU_NEON)
				   "gpu_neon is Exophase's NEON GPU, fast and accurate\n"
#endif
#ifndef BUILTIN_GPU_PEOPS
				   "gpu_peops is Pete's soft GPU, slow but accurate\n"
#endif
#ifndef BUILTIN_GPU_UNAI
				   "gpu_unai is the GPU renderer from PCSX4ALL\n"
#endif
#ifdef HAVE_GLES
				   "gpu_gles Pete's hw GPU, uses 3D chip but is glitchy\n"
#endif
				   "must save config and reload the game if changed"
#endif
				   ;
static const char h_plugin_spu[] = ""
#ifndef NO_DYLIB
				   "spunull effectively disables sound\n"
				   "must save config and reload the game if changed"
#endif
;
// static const char h_gpu_peops[]  = "Configure P.E.Op.S. SoftGL Driver V1.17";
// static const char h_gpu_peopsgl[]= "Configure P.E.Op.S. MesaGL Driver V1.78";
// static const char h_gpu_unai[]   = "Configure Unai/PCSX4ALL Team plugin (new)";
static const char h_spu[]        = "Configure built-in P.E.Op.S. Sound Driver V1.7";

static int menu_loop_pluginsel_options(int id, int keys)
{
	static int sel = 0;
	if (strcmp(gpu_plugins[gpu_plugsel], "gpu_peops.so") == 0)
		me_loop(e_menu_plugin_gpu_peops, &sel);
	else if (strcmp(gpu_plugins[gpu_plugsel], "gpu_unai.so") == 0)
		me_loop(e_menu_plugin_gpu_unai, &sel);
	else if (strcmp(gpu_plugins[gpu_plugsel], "gpu_gles.so") == 0)
		me_loop(e_menu_plugin_gpu_peopsgl, &sel);
	else if (strcmp(gpu_plugins[gpu_plugsel], "gpu_neon.so") == 0)
		me_loop(e_menu_plugin_gpu_neon, &sel);
	else
#if defined(BUILTIN_GPU_NEON)
		me_loop(e_menu_plugin_gpu_neon, &sel);
#elif defined(BUILTIN_GPU_PEOPS)
		me_loop(e_menu_plugin_gpu_peops, &sel);
#elif defined(BUILTIN_GPU_UNAI)
		me_loop(e_menu_plugin_gpu_unai, &sel);
#endif
	return 0;
}

static menu_entry e_menu_plugin_options[] =
{
	mee_enum_h    ("BIOS",                          0, bios_sel, bioses, h_bios),
	mee_enum      ("GPU Dithering",                 0, pl_rearmed_cbs.dithering, men_gpu_dithering),
	mee_enum_h    ("GPU plugin",                    0, gpu_plugsel, gpu_plugins, h_plugin_gpu),
	mee_enum_h    ("SPU plugin",                    0, spu_plugsel, spu_plugins, h_plugin_spu),
	mee_handler   ("Configure selected GPU plugin", menu_loop_pluginsel_options),
	mee_handler_h ("Configure built-in SPU plugin", menu_loop_plugin_spu, h_spu),
	mee_end,
};

static menu_entry e_menu_main2[];

static int menu_loop_plugin_options(int id, int keys)
{
	static int sel = 0;
	me_loop(e_menu_plugin_options, &sel);

	// sync BIOS/plugins
	snprintf(Config.Bios, sizeof(Config.Bios), "%s", bioses[bios_sel]);
	snprintf(Config.Gpu, sizeof(Config.Gpu), "%s", gpu_plugins[gpu_plugsel]);
	snprintf(Config.Spu, sizeof(Config.Spu), "%s", spu_plugins[spu_plugsel]);
	me_enable(e_menu_main2, MA_MAIN_RUN_BIOS, bios_sel != 0);

	return 0;
}

// ------------ adv options menu ------------

#ifndef DRC_DISABLE
static const char h_cfg_noch[]    = "Disables game-specific compatibility hacks";
static const char h_cfg_nosmc[]   = "Will cause crashes when loading, break memcards";
static const char h_cfg_gteunn[]  = "May cause graphical glitches";
static const char h_cfg_gteflgs[] = "Will cause graphical glitches";
#endif
static const char h_cfg_stalls[]  = "Will cause some games to run too fast";

static menu_entry e_menu_speed_hacks[] =
{
#ifndef DRC_DISABLE
	mee_onoff_h   ("Disable compat hacks",     0, ndrc_g.hacks, NDHACK_NO_COMPAT_HACKS, h_cfg_noch),
	mee_onoff_h   ("Disable SMC checks",       0, ndrc_g.hacks, NDHACK_NO_SMC_CHECK, h_cfg_nosmc),
	mee_onoff_h   ("Assume GTE regs unneeded", 0, ndrc_g.hacks, NDHACK_GTE_UNNEEDED, h_cfg_gteunn),
	mee_onoff_h   ("Disable GTE flags",        0, ndrc_g.hacks, NDHACK_GTE_NO_FLAGS, h_cfg_gteflgs),
#endif
	mee_onoff_h   ("Disable CPU/GTE stalls",   0, menu_iopts[0], 1, h_cfg_stalls),
	mee_end,
};

static int menu_loop_speed_hacks(int id, int keys)
{
	static int sel = 0;
	menu_iopts[0] = Config.DisableStalls;
	me_loop(e_menu_speed_hacks, &sel);
	Config.DisableStalls = menu_iopts[0];
	return 0;
}

static const char *men_autooo[]  = { "Auto", "Off", "On", NULL };

static const char h_cfg_cpul[]   = "Shows CPU usage in %";
static const char h_cfg_spu[]    = "Shows active SPU channels\n"
				   "(green: normal, red: fmod, blue: noise)";
static const char h_cfg_fl[]     = "Frame Limiter keeps the game from running too fast";
static const char h_cfg_xa[]     = "Disables XA sound, which can sometimes improve performance";
static const char h_cfg_cdda[]   = "Disable CD Audio for a performance boost\n"
				   "(proper .cue/.bin dump is needed otherwise)";
#ifndef DRC_DISABLE
static const char h_cfg_nodrc[]  = "Disable dynamic recompiler and use interpreter\n"
				   "Might be useful to overcome some dynarec bugs";
#endif
static const char h_cfg_shacks[] = "Breaks games but may give better performance";
static const char h_cfg_icache[] = "Support F1 games (only when dynarec is off)";
static const char h_cfg_exc[]    = "Emulate some PSX's debug hw like breakpoints\n"
				   "and exceptions (slow, interpreter only, keep off)";
static const char h_cfg_gpul[]   = "Try enabling this if the game misses some graphics\n"
				   "causes a performance hit";
static const char h_cfg_ffps[]   = "Instead of 50/60fps for PAL/NTSC use ~49.75/59.81\n"
				   "Closer to real hw but doesn't match modern displays.";
static const char h_cfg_tcd[]    = "Greatly reduce CD load times. Breaks some games.";
static const char h_cfg_psxclk[]  = "Over/under-clock the PSX, default is " DEFAULT_PSX_CLOCK_S "\n"
				    "(adjust this if the game is too slow/too fast/hangs)";

enum { AMO_XA, AMO_CDDA, AMO_IC, AMO_BP, AMO_CPU, AMO_GPUL, AMO_FFPS, AMO_TCD };

static menu_entry e_menu_adv_options[] =
{
	mee_onoff_h   ("Show CPU load",          0, g_opts, OPT_SHOWCPU, h_cfg_cpul),
	mee_onoff_h   ("Show SPU channels",      0, g_opts, OPT_SHOWSPU, h_cfg_spu),
	mee_onoff_h   ("Disable Frame Limiter",  0, g_opts, OPT_NO_FRAMELIM, h_cfg_fl),
	mee_onoff_h   ("Disable XA Decoding",    0, menu_iopts[AMO_XA],   1, h_cfg_xa),
	mee_onoff_h   ("Disable CD Audio",       0, menu_iopts[AMO_CDDA], 1, h_cfg_cdda),
	mee_onoff_h   ("ICache emulation",       0, menu_iopts[AMO_IC],   1, h_cfg_icache),
	mee_onoff_h   ("BP exception emulation", 0, menu_iopts[AMO_BP],   1, h_cfg_exc),
	mee_enum_h    ("GPU l-list slow walking",0, menu_iopts[AMO_GPUL], men_autooo, h_cfg_gpul),
	mee_enum_h    ("Fractional framerate",   0, menu_iopts[AMO_FFPS], men_autooo, h_cfg_ffps),
	mee_onoff_h   ("Turbo CD-ROM ",          0, menu_iopts[AMO_TCD], 1, h_cfg_tcd),
#ifdef USE_ASYNC_CDROM
	mee_range     ("CD-ROM read-ahead",      0, cd_buf_count, 0, 1024),
#endif
#if !defined(DRC_DISABLE) || defined(LIGHTREC)
	mee_onoff_h   ("Disable dynarec (slow!)",0, menu_iopts[AMO_CPU],  1, h_cfg_nodrc),
#endif
	mee_range_h   ("PSX CPU clock, %",       0, psx_clock, 1, 500, h_cfg_psxclk),
	mee_handler_h ("[Speed hacks]",             menu_loop_speed_hacks, h_cfg_shacks),
	mee_end,
};

static int menu_loop_adv_options(int id, int keys)
{
	static int sel = 0;
	static struct {
		boolean *opt;
		int *mopt;
	} opts[] = {
		{ &Config.Xa,      &menu_iopts[AMO_XA] },
		{ &Config.Cdda,    &menu_iopts[AMO_CDDA] },
		{ &Config.icache_emulation, &menu_iopts[AMO_IC] },
		{ &Config.PreciseExceptions, &menu_iopts[AMO_BP] },
		{ &Config.Cpu,     &menu_iopts[AMO_CPU] },
		{ &Config.TurboCD, &menu_iopts[AMO_TCD] },
	};
	int i;
	for (i = 0; i < ARRAY_SIZE(opts); i++)
		*opts[i].mopt = *opts[i].opt;
	menu_iopts[AMO_GPUL] = Config.GpuListWalking + 1;
	menu_iopts[AMO_FFPS] = Config.FractionalFramerate + 1;

	me_loop(e_menu_adv_options, &sel);

	for (i = 0; i < ARRAY_SIZE(opts); i++)
		*opts[i].opt = *opts[i].mopt;
	Config.GpuListWalking = menu_iopts[AMO_GPUL] - 1;
	Config.FractionalFramerate = menu_iopts[AMO_FFPS] - 1;
	cdra_set_buf_count(cd_buf_count);

	return 0;
}

// ------------ options menu ------------

static int mh_restore_defaults(int id, int keys)
{
	menu_set_defconfig();
	menu_update_msg("defaults restored");
	return 1;
}

static const char *men_region[]       = { "Auto", "NTSC", "PAL", NULL };
static const char *men_frameskip[]    = { "Auto", "Off", "1", "2", "3", NULL };
/*
static const char *men_confirm_save[] = { "OFF", "writes", "loads", "both", NULL };
static const char h_confirm_save[]    = "Ask for confirmation when overwriting save,\n"
					"loading state or both";
*/
static const char h_restore_def[]     = "Switches back to default / recommended\n"
					"configuration";
static const char h_frameskip[]       = "Warning: frameskip sometimes causes glitches\n";
static const char h_sputhr[]          = "Warning: has some known bugs\n";

static menu_entry e_menu_options[] =
{
//	mee_range     ("Save slot",                0, state_slot, 0, 9),
//	mee_enum_h    ("Confirm savestate",        0, dummy, men_confirm_save, h_confirm_save),
	mee_enum_h    ("Frameskip",                0, frameskip, men_frameskip, h_frameskip),
	mee_onoff     ("Show FPS",                 0, g_opts, OPT_SHOWFPS),
	mee_enum      ("Region",                   0, region, men_region),
	mee_range     ("CPU clock",                MA_OPT_CPU_CLOCKS, cpu_clock, 20, 5000),
#ifdef C64X_DSP
	mee_onoff_h   ("Use C64x DSP for sound",   MA_OPT_SPU_THREAD, spu_config.iUseThread, 1, h_sputhr),
#else
	mee_onoff_h   ("Threaded SPU",             MA_OPT_SPU_THREAD, spu_config.iUseThread, 1, h_sputhr),
#endif
	mee_handler_id("[Display]",                MA_OPT_DISP_OPTS, menu_loop_gfx_options),
	mee_handler   ("[BIOS/Plugins]",           menu_loop_plugin_options),
	mee_handler   ("[Advanced]",               menu_loop_adv_options),
	mee_cust_nosave("Save global config",      MA_OPT_SAVECFG,      mh_savecfg, mgn_saveloadcfg),
	mee_cust_nosave("Save cfg for loaded game",MA_OPT_SAVECFG_GAME, mh_savecfg, mgn_saveloadcfg),
	mee_handler_h ("Restore default config",   mh_restore_defaults, h_restore_def),
	mee_end,
};

static int menu_loop_options(int id, int keys)
{
	static int sel = 0;

	me_enable(e_menu_options, MA_OPT_CPU_CLOCKS, cpu_clock_st > 0);
	me_enable(e_menu_options, MA_OPT_SPU_THREAD, spu_config.iThreadAvail);
	me_enable(e_menu_options, MA_OPT_SAVECFG_GAME, ready_to_go && CdromId[0]);

	me_loop(e_menu_options, &sel);

	return 0;
}

// ------------ debug menu ------------

static void draw_frame_debug(GPUFreeze_t *gpuf, int x, int y)
{
	int w = min(g_menuscreen_w, 1024);
	int h = min(g_menuscreen_h, 512);
	u16 *d = g_menuscreen_ptr;
	u16 *s = (u16 *)gpuf->psxVRam + y * 1024 + x;
	char buff[64];
	int ty = 1;

	gpuf->ulFreezeVersion = 1;
	if (GPU_freeze != NULL)
		GPU_freeze(1, gpuf);

	for (; h > 0; h--, d += g_menuscreen_w, s += 1024)
		bgr555_to_rgb565(d, s, w * 2);

	smalltext_out16(4, 1, "build: "__DATE__ " " __TIME__ " " REV, 0xe7fc);
	snprintf(buff, sizeof(buff), "GPU sr: %08x", gpuf->ulStatus);
	smalltext_out16(4, (ty += me_sfont_h), buff, 0xe7fc);
	snprintf(buff, sizeof(buff), "PC/SP: %08x %08x", psxRegs.pc, psxRegs.GPR.n.sp);
	smalltext_out16(4, (ty += me_sfont_h), buff, 0xe7fc);
}

static void debug_menu_loop(void)
{
	int inp, df_x = 0, df_y = 0;
	GPUFreeze_t *gpuf;

	gpuf = malloc(sizeof(*gpuf));
	if (gpuf == NULL)
		return;

	while (1)
	{
		menu_draw_begin(0, 1);
		draw_frame_debug(gpuf, df_x, df_y);
		menu_draw_end();

		inp = in_menu_wait(PBTN_MOK|PBTN_MBACK|PBTN_MA2|PBTN_MA3|PBTN_L|PBTN_R |
					PBTN_UP|PBTN_DOWN|PBTN_LEFT|PBTN_RIGHT, NULL, 10);
		if      (inp & PBTN_MBACK) break;
		else if (inp & PBTN_UP)    { if (df_y > 0) df_y--; }
		else if (inp & PBTN_DOWN)  { if (df_y < 512 - g_menuscreen_h) df_y++; }
		else if (inp & PBTN_LEFT)  { if (df_x > 0) df_x -= 2; }
		else if (inp & PBTN_RIGHT) { if (df_x < 1024 - g_menuscreen_w) df_x += 2; }
	}

	free(gpuf);
}

// --------- memcard manager ---------

static void draw_mc_icon(int dx, int dy, const u16 *s)
{
	u16 *d;
	int x, y, l, p;
	
	d = (u16 *)g_menuscreen_ptr + g_menuscreen_w * dy + dx;

	for (y = 0; y < 16; y++, s += 16) {
		for (l = 0; l < 2; l++, d += g_menuscreen_w) {
			for (x = 0; x < 16; x++) {
				p = s[x];
				d[x*2] = d[x*2 + 1] = ((p & 0x7c00) >> 10)
					| ((p & 0x03e0) << 1) | ((p & 0x1f) << 11);
			}
		}
	}
}

static void draw_mc_bg(void)
{
	McdBlock *blocks1, *blocks2;
	int maxicons = 15;
	int i, y, row2;

	blocks1 = malloc(15 * sizeof(blocks1[0]));
	blocks2 = malloc(15 * sizeof(blocks1[0]));
	if (blocks1 == NULL || blocks2 == NULL)
		goto out;

	for (i = 0; i < 15; i++) {
		GetMcdBlockInfo(1, i + 1, &blocks1[i]);
		GetMcdBlockInfo(2, i + 1, &blocks2[i]);
	}

	menu_draw_begin(1, 1);

	memcpy(g_menuscreen_ptr, g_menubg_src_ptr, g_menuscreen_w * g_menuscreen_h * 2);

	y = g_menuscreen_h / 2 - 15 * 32 / 2;
	if (y < 0) {
		// doesn't fit..
		y = 0;
		maxicons = g_menuscreen_h / 32;
	}

	row2 = g_menuscreen_w / 2;
	for (i = 0; i < maxicons; i++) {
		draw_mc_icon(8, y + i * 32, (u16 *)blocks1[i].Icon);
		smalltext_out16(10+32, y + i * 32 + 8, blocks1[i].sTitle, 0xf71e);

		draw_mc_icon(row2 + 8, y + i * 32, (u16 *)blocks2[i].Icon);
		smalltext_out16(row2 + 10+32, y + i * 32 + 8, blocks2[i].sTitle, 0xf71e);
	}

	menu_darken_bg(g_menubg_ptr, g_menuscreen_ptr, g_menuscreen_w * g_menuscreen_h, 0);

	menu_draw_end();
out:
	free(blocks1);
	free(blocks2);
}

static void handle_memcard_sel(void)
{
	strcpy(Config.Mcd1, "none");
	if (memcard1_sel != 0)
		emu_make_path(Config.Mcd1, sizeof(Config.Mcd1), MEMCARD_DIR, memcards[memcard1_sel]);
	strcpy(Config.Mcd2, "none");
	if (memcard2_sel != 0)
		emu_make_path(Config.Mcd2, sizeof(Config.Mcd2), MEMCARD_DIR, memcards[memcard2_sel]);
	LoadMcds(Config.Mcd1, Config.Mcd2);
	draw_mc_bg();
}

static menu_entry e_memcard_options[] =
{
	mee_enum("Memory card 1", 0, memcard1_sel, memcards),
	mee_enum("Memory card 2", 0, memcard2_sel, memcards),
	mee_end,
};

static int menu_loop_memcards(int id, int keys)
{
	static int sel = 0;
	char *p;
	int i;

	memcard1_sel = memcard2_sel = 0;
	p = strrchr(Config.Mcd1, '/');
	if (p != NULL)
		for (i = 0; memcards[i] != NULL; i++)
			if (strcmp(p + 1, memcards[i]) == 0)
				{ memcard1_sel = i; break; }
	p = strrchr(Config.Mcd2, '/');
	if (p != NULL)
		for (i = 0; memcards[i] != NULL; i++)
			if (strcmp(p + 1, memcards[i]) == 0)
				{ memcard2_sel = i; break; }

	me_loop_d(e_memcard_options, &sel, handle_memcard_sel, NULL);

	memcpy(g_menubg_ptr, g_menubg_src_ptr, g_menuscreen_w * g_menuscreen_h * 2);

	return 0;
}

// ------------ cheats menu ------------

static void draw_cheatlist(int sel)
{
	int max_cnt, start, i, pos, active;

	max_cnt = g_menuscreen_h / me_sfont_h;
	start = max_cnt / 2 - sel;

	menu_draw_begin(1, 1);

	for (i = 0; i < NumCheats; i++) {
		pos = start + i;
		if (pos < 0) continue;
		if (pos >= max_cnt) break;
		active = Cheats[i].Enabled;
		smalltext_out16(14,                pos * me_sfont_h,
			active ? "ON " : "OFF", active ? 0xfff6 : 0xffff);
		smalltext_out16(14 + me_sfont_w*4, pos * me_sfont_h,
			Cheats[i].Descr, active ? 0xfff6 : 0xffff);
	}
	pos = start + i;
	if (pos < max_cnt)
		smalltext_out16(14, pos * me_sfont_h, "done", 0xffff);

	text_out16(5, max_cnt / 2 * me_sfont_h, ">");
	menu_draw_end();
}

static void menu_loop_cheats(void)
{
	static int menu_sel = 0;
	int inp;

	for (;;)
	{
		draw_cheatlist(menu_sel);
		inp = in_menu_wait(PBTN_UP|PBTN_DOWN|PBTN_LEFT|PBTN_RIGHT|PBTN_L|PBTN_R
				|PBTN_MOK|PBTN_MBACK, NULL, 33);
		if (inp & PBTN_UP  ) { menu_sel--; if (menu_sel < 0) menu_sel = NumCheats; }
		if (inp & PBTN_DOWN) { menu_sel++; if (menu_sel > NumCheats) menu_sel = 0; }
		if (inp &(PBTN_LEFT|PBTN_L))  { menu_sel-=10; if (menu_sel < 0) menu_sel = 0; }
		if (inp &(PBTN_RIGHT|PBTN_R)) { menu_sel+=10; if (menu_sel > NumCheats) menu_sel = NumCheats; }
		if (inp & PBTN_MOK) { // action
			if (menu_sel < NumCheats)
				Cheats[menu_sel].Enabled = !Cheats[menu_sel].Enabled;
			else 	break;
		}
		if (inp & PBTN_MBACK)
			break;
	}
}

// --------- main menu help ----------

static void menu_bios_warn(void)
{
	int inp;
	static const char msg[] =
		"You don't seem to have copied any BIOS\n"
		"files to\n%s\n\n"

		"While many games work fine with fake\n"
		"(HLE) BIOS, others (like MGS and FF8)\n"
		"require BIOS to work.\n"
		"After copying the file, you'll also need\n"
		"to select it in the emu's menu:\n"
		"options->[BIOS/Plugins]\n\n"
		"The file is usually named SCPH1001.BIN,\n"
		"but other not compressed files can be\n"
		"used too.\n\n"
		"Press %s or %s to continue";
	char tmp_msg[sizeof(msg) + 64];

	snprintf(tmp_msg, sizeof(tmp_msg), msg, Config.BiosDir,
		in_get_key_name(-1, -PBTN_MOK), in_get_key_name(-1, -PBTN_MBACK));
	while (1)
	{
		draw_menu_message(tmp_msg, NULL);

		inp = in_menu_wait(PBTN_MOK|PBTN_MBACK, NULL, 70);
		if (inp & (PBTN_MBACK|PBTN_MOK))
			return;
	}
}

// ------------ main menu ------------

static menu_entry e_menu_main[];

static void draw_frame_main(void)
{
	struct tm *tmp;
	time_t ltime;
	int capacity;
	char ltime_s[16];
	char buff[64];
	char *out;

	if (CdromId[0] != 0) {
		snprintf(buff, sizeof(buff), "%.32s/%.9s (running as %s, with %s)",
			 get_cd_label(), CdromId, Config.PsxType ? "PAL" : "NTSC",
			 Config.HLE ? "HLE" : "BIOS");
		smalltext_out16(4, 1, buff, 0x105f);
	}

	if (ready_to_go) {
		capacity = plat_target_bat_capacity_get();
		ltime = time(NULL);
		tmp = localtime(&ltime);
		strftime(ltime_s, sizeof(ltime_s), "%H:%M", tmp);
		if (capacity >= 0) {
			snprintf(buff, sizeof(buff), "%s %3d%%", ltime_s, capacity);
			out = buff;
		}
		else
			out = ltime_s;
		smalltext_out16(4, 1 + me_sfont_h, out, 0x105f);
	}
}

static void draw_frame_credits(void)
{
	smalltext_out16(4, 1, "build: " __DATE__ " " __TIME__ " " REV, 0xe7fc);
}

static const char credits_text[] = 
	"PCSX-ReARMed\n\n"
	"(C) 1999-2003 PCSX Team\n"
	"(C) 2005-2009 PCSX-df Team\n"
	"(C) 2009-2011 PCSX-Reloaded Team\n\n"
	"ARM recompiler (C) 2009-2011 Ari64\n"
	"ARM NEON GPU (c) 2011-2012 Exophase\n"
	"PEOpS GPU and SPU by Pete Bernert\n"
	"  and the P.E.Op.S. team\n"
	"PCSX4ALL plugin by PCSX4ALL team\n"
	"  Chui, Franxis, Unai\n\n"
	"integration, optimization and\n"
	"  frontend (C) 2010-2015 notaz\n";

static int reset_game(void)
{
	ClosePlugins();
	OpenPlugins();
	SysReset();
	if (Config.HLE) {
		if (LoadCdrom() == -1)
			return -1;
	}
	return 0;
}

static int reload_plugins(const char *cdimg)
{
	pl_vout_buf = NULL;

	ClosePlugins();

	set_cd_image(cdimg);
	LoadPlugins();
	pcnt_hook_plugins();
	if (OpenPlugins() == -1) {
		menu_update_msg("failed to open plugins");
		return -1;
	}
	plugin_call_rearmed_cbs();

	cdrIsoMultidiskCount = 1;
	CdromId[0] = '\0';
	CdromLabel[0] = '\0';

	return 0;
}

static int run_bios(void)
{
	boolean origSlowBoot = Config.SlowBoot;

	if (bios_sel == 0)
		return -1;

	ready_to_go = 0;
	if (reload_plugins(NULL) != 0)
		return -1;
	Config.SlowBoot = 1;
	SysReset();
	Config.SlowBoot = origSlowBoot;

	ready_to_go = 1;
	return 0;
}

static int run_exe(void)
{
	const char *exts[] = { "exe", NULL };
	const char *fname;

	fname = menu_loop_romsel(last_selected_fname,
		sizeof(last_selected_fname), exts, NULL);
	if (fname == NULL)
		return -1;

	ready_to_go = 0;
	if (reload_plugins(NULL) != 0)
		return -1;

	SysReset();
	if (Load(fname) != 0) {
		menu_update_msg("exe load failed, bad file?");
		printf("meh\n");
		return -1;
	}

	ready_to_go = 1;
	return 0;
}

static int run_cd_image(const char *fname)
{
	int autoload_state = g_autostateld_opt;
	size_t fname_len = strlen(fname);
	const char *ppfname = NULL;
	char fname2[256];

	// simle ppf handling, like game.chd.ppf
	if (4 < fname_len && fname_len < sizeof(fname2)
	    && strcasecmp(fname + fname_len - 4, ".ppf") == 0) {
		memcpy(fname2, fname, fname_len - 4);
		fname2[fname_len - 4] = 0;
		ppfname = fname;
		fname = fname2;
	}

	ready_to_go = 0;
	reload_plugins(fname);

	// always autodetect, menu_sync_config will override as needed
	Config.PsxAuto = 1;

	if (CheckCdrom() == -1) {
		// Only check the CD if we are starting the console with a CD
		ClosePlugins();
		menu_update_msg("unsupported/invalid CD image");
		return -1;
	}
	if (ppfname)
		BuildPPFCache(ppfname);

	SysReset();

	// Read main executable directly from CDRom and start it
	if (LoadCdrom() == -1) {
		ClosePlugins();
		menu_update_msg("failed to load CD image");
		return -1;
	}

	emu_on_new_cd(1);
	ready_to_go = 1;

	if (autoload_state) {
		unsigned int newest = 0;
		int time = 0, slot, newest_slot = -1;

		for (slot = 0; slot < 10; slot++) {
			if (emu_check_save_file(slot, &time)) {
				if ((unsigned int)time > newest) {
					newest = time;
					newest_slot = slot;
				}
			}
		}

		if (newest_slot >= 0) {
			lprintf("autoload slot %d\n", newest_slot);
			emu_load_state(newest_slot);
		}
		else {
			lprintf("no save to autoload.\n");
		}
	}

	return 0;
}

static int romsel_run(void)
{
	int prev_gpu, prev_spu;
	const char *fname;

	fname = menu_loop_romsel(last_selected_fname,
			sizeof(last_selected_fname), filter_exts,
			optional_cdimg_filter);
	if (fname == NULL)
		return -1;

	printf("selected file: %s\n", fname);

	ndrc_clear_full();

	if (run_cd_image(fname) != 0)
		return -1;

	prev_gpu = gpu_plugsel;
	prev_spu = spu_plugsel;
	if (menu_load_config(1) != 0)
		menu_load_config(0);

	// check for plugin changes, have to repeat
	// loading if game config changed plugins to reload them
	if (prev_gpu != gpu_plugsel || prev_spu != spu_plugsel) {
		printf("plugin change detected, reloading plugins..\n");
		if (run_cd_image(fname) != 0)
			return -1;
	}

	strcpy(last_selected_fname, fname);
	menu_do_last_cd_img(0);
	return 0;
}

static int swap_cd_image(void)
{
	const char *fname;

	fname = menu_loop_romsel(last_selected_fname,
			sizeof(last_selected_fname), filter_exts,
			optional_cdimg_filter);
	if (fname == NULL)
		return -1;

	printf("selected file: %s\n", fname);

	CdromId[0] = '\0';
	CdromLabel[0] = '\0';

	set_cd_image(fname);
	if (ReloadCdromPlugin() < 0) {
		menu_update_msg("failed to load cdr plugin");
		return -1;
	}
	if (cdra_open() < 0) {
		menu_update_msg("failed to open cdr plugin");
		return -1;
	}

	SetCdOpenCaseTime(time(NULL) + 2);
	LidInterrupt();

	strcpy(last_selected_fname, fname);
	return 0;
}

static int swap_cd_multidisk(void)
{
	cdrIsoMultidiskSelect++;
	CdromId[0] = '\0';
	CdromLabel[0] = '\0';

	cdra_close();
	if (cdra_open() < 0) {
		menu_update_msg("failed to open cdr plugin");
		return -1;
	}

	SetCdOpenCaseTime(time(NULL) + 2);
	LidInterrupt();

	return 0;
}

static void load_pcsx_cht(void)
{
	static const char *exts[] = { "cht", NULL };
	const char *fname;
	char msg[64];

	fname = menu_loop_romsel(last_selected_fname,
			sizeof(last_selected_fname), exts, NULL);
	if (fname == NULL)
		return;

	printf("selected cheat file: %s\n", fname);
	LoadCheats(fname);

	if (NumCheats == 0 && NumCodes == 0)
		menu_update_msg("failed to load cheats");
	else {
		snprintf(msg, sizeof(msg), "%d cheat(s) loaded", NumCheats + NumCodes);
		menu_update_msg(msg);
	}
	me_enable(e_menu_main, MA_MAIN_CHEATS, ready_to_go && NumCheats);
}

static int main_menu_handler(int id, int keys)
{
	switch (id)
	{
	case MA_MAIN_RESUME_GAME:
		if (ready_to_go)
			return 1;
		break;
	case MA_MAIN_SAVE_STATE:
		if (ready_to_go)
			return menu_loop_savestate(0);
		break;
	case MA_MAIN_LOAD_STATE:
		if (ready_to_go)
			return menu_loop_savestate(1);
		break;
	case MA_MAIN_RESET_GAME:
		if (ready_to_go && reset_game() == 0)
			return 1;
		break;
	case MA_MAIN_LOAD_ROM:
		if (romsel_run() == 0)
			return 1;
		break;
	case MA_MAIN_SWAP_CD:
		if (swap_cd_image() == 0)
			return 1;
		break;
	case MA_MAIN_SWAP_CD_MULTI:
		if (swap_cd_multidisk() == 0)
			return 1;
		break;
	case MA_MAIN_RUN_BIOS:
		if (run_bios() == 0)
			return 1;
		break;
	case MA_MAIN_RUN_EXE:
		if (run_exe() == 0)
			return 1;
		break;
	case MA_MAIN_CHEATS:
		menu_loop_cheats();
		break;
	case MA_MAIN_LOAD_CHEATS:
		load_pcsx_cht();
		break;
	case MA_MAIN_CREDITS:
		draw_menu_message(credits_text, draw_frame_credits);
		in_menu_wait(PBTN_MOK|PBTN_MBACK, NULL, 70);
		break;
	case MA_MAIN_EXIT:
		emu_core_ask_exit();
		return 1;
	default:
		lprintf("%s: something unknown selected\n", __FUNCTION__);
		break;
	}

	return 0;
}

static menu_entry e_menu_main2[] =
{
	mee_handler_id("Change CD image",    MA_MAIN_SWAP_CD,       main_menu_handler),
	mee_handler_id("Next multidisk CD",  MA_MAIN_SWAP_CD_MULTI, main_menu_handler),
	mee_handler_id("Run BIOS",           MA_MAIN_RUN_BIOS,      main_menu_handler),
	mee_handler_id("Run EXE",            MA_MAIN_RUN_EXE,       main_menu_handler),
	mee_handler   ("Memcard manager",    menu_loop_memcards),
	mee_handler_id("Load PCSX cheats..", MA_MAIN_LOAD_CHEATS,   main_menu_handler),
	mee_end,
};

static int main_menu2_handler(int id, int keys)
{
	static int sel = 0;

	me_enable(e_menu_main2, MA_MAIN_SWAP_CD,  ready_to_go);
	me_enable(e_menu_main2, MA_MAIN_SWAP_CD_MULTI, ready_to_go && cdrIsoMultidiskCount > 1);
	me_enable(e_menu_main2, MA_MAIN_RUN_BIOS, bios_sel != 0);
	me_enable(e_menu_main2, MA_MAIN_LOAD_CHEATS, ready_to_go);

	return me_loop_d(e_menu_main2, &sel, NULL, draw_frame_main);
}

static const char h_extra[] = "Change CD, manage memcards..\n";

static menu_entry e_menu_main[] =
{
	mee_label     (""),
	mee_label     (""),
	mee_handler_id("Resume game",        MA_MAIN_RESUME_GAME, main_menu_handler),
	mee_handler_id("Save State",         MA_MAIN_SAVE_STATE,  main_menu_handler),
	mee_handler_id("Load State",         MA_MAIN_LOAD_STATE,  main_menu_handler),
	mee_handler_id("Reset game",         MA_MAIN_RESET_GAME,  main_menu_handler),
	mee_handler_id("Load CD image",      MA_MAIN_LOAD_ROM,    main_menu_handler),
	mee_handler   ("Options",            menu_loop_options),
	mee_handler   ("Controls",           menu_loop_keyconfig),
	mee_handler_id("Cheats",             MA_MAIN_CHEATS,      main_menu_handler),
	mee_handler_h ("Extra stuff",        main_menu2_handler,  h_extra),
	mee_handler_id("Credits",            MA_MAIN_CREDITS,     main_menu_handler),
	mee_handler_id("Exit",               MA_MAIN_EXIT,        main_menu_handler),
	mee_end,
};

// ----------------------------

static void menu_leave_emu(void);

void menu_loop(void)
{
	static int warned_about_bios = 0;
	static int sel = 0;

	menu_leave_emu();

	if (config_save_counter == 0) {
		// assume first run
		if (bioses[1] != NULL) {
			// autoselect BIOS to make user's life easier
			snprintf(Config.Bios, sizeof(Config.Bios), "%s", bioses[1]);
			bios_sel = 1;
		}
		else if (!warned_about_bios) {
			menu_bios_warn();
			warned_about_bios = 1;
		}
	}

	me_enable(e_menu_main, MA_MAIN_RESUME_GAME, ready_to_go);
	me_enable(e_menu_main, MA_MAIN_SAVE_STATE,  ready_to_go && CdromId[0]);
	me_enable(e_menu_main, MA_MAIN_LOAD_STATE,  ready_to_go && CdromId[0]);
	me_enable(e_menu_main, MA_MAIN_RESET_GAME,  ready_to_go);
	me_enable(e_menu_main, MA_MAIN_CHEATS,      ready_to_go && NumCheats);

	in_set_config_int(0, IN_CFG_BLOCKING, 1);

	do {
		me_loop_d(e_menu_main, &sel, NULL, draw_frame_main);
	} while (!ready_to_go && !g_emu_want_quit);

	/* wait until menu, ok, back is released */
	while (in_menu_wait_any(NULL, 50) & (PBTN_MENU|PBTN_MOK|PBTN_MBACK))
		;

	in_set_config_int(0, IN_CFG_BLOCKING, 0);

	menu_prepare_emu();
}

static int qsort_strcmp(const void *p1, const void *p2)
{
	char * const *s1 = (char * const *)p1;
	char * const *s2 = (char * const *)p2;
	return strcasecmp(*s1, *s2);
}

static void scan_bios_plugins(void)
{
	char fname[MAXPATHLEN];
	struct dirent *ent;
	int bios_i, gpu_i, spu_i, mc_i;
	DIR *dir;

	bioses[0] = "HLE";
	gpu_plugins[0] = "builtin_gpu";
	spu_plugins[0] = "builtin_spu";
	memcards[0] = "(none)";
	bios_i = gpu_i = spu_i = mc_i = 1;

	snprintf(fname, sizeof(fname), "%s/", Config.BiosDir);
	dir = opendir(fname);
	if (dir == NULL) {
		perror("scan_bios_plugins bios opendir");
#ifndef NO_DYLIB
		goto do_plugins;
#else
		goto do_memcards;
#endif
	}

	while (1) {
		struct stat st;

		errno = 0;
		ent = readdir(dir);
		if (ent == NULL) {
			if (errno != 0)
				perror("readdir");
			break;
		}

		if (ent->d_type != DT_REG && ent->d_type != DT_LNK)
			continue;

		snprintf(fname, sizeof(fname), "%s/%s", Config.BiosDir, ent->d_name);
		if (stat(fname, &st) != 0
		    || (st.st_size != 512*1024 && st.st_size != 4*1024*1024)) {
			printf("bad BIOS file: %s\n", ent->d_name);
			continue;
		}

		if (bios_i < ARRAY_SIZE(bioses) - 1) {
			bioses[bios_i++] = strdup(ent->d_name);
			continue;
		}

		printf("too many BIOSes, dropping \"%s\"\n", ent->d_name);
	}

	closedir(dir);

#ifndef NO_DYLIB
do_plugins:
	snprintf(fname, sizeof(fname), "%s/", Config.PluginsDir);
	dir = opendir(fname);
	if (dir == NULL) {
		perror("scan_bios_plugins plugins opendir");
		goto do_memcards;
	}

	while (1) {
		void *h, *tmp;
		char *p;

		errno = 0;
		ent = readdir(dir);
		if (ent == NULL) {
			if (errno != 0)
				perror("readdir");
			break;
		}
		p = strstr(ent->d_name, ".so");
		if (p == NULL)
			continue;

		snprintf(fname, sizeof(fname), "%s/%s", Config.PluginsDir, ent->d_name);
		h = dlopen(fname, RTLD_LAZY | RTLD_LOCAL);
		if (h == NULL) {
			fprintf(stderr, "%s\n", dlerror());
			continue;
		}

		// now what do we have here?
		tmp = dlsym(h, "GPUinit");
		if (tmp) {
			dlclose(h);
			if (gpu_i < ARRAY_SIZE(gpu_plugins) - 1)
				gpu_plugins[gpu_i++] = strdup(ent->d_name);
			continue;
		}

		tmp = dlsym(h, "SPUinit");
		if (tmp) {
			dlclose(h);
			if (spu_i < ARRAY_SIZE(spu_plugins) - 1)
				spu_plugins[spu_i++] = strdup(ent->d_name);
			continue;
		}

		fprintf(stderr, "ignoring unidentified plugin: %s\n", fname);
		dlclose(h);
	}

	closedir(dir);
#endif

do_memcards:
	emu_make_path(fname, sizeof(fname), MEMCARD_DIR, NULL);
	dir = opendir(fname);
	if (dir == NULL) {
		perror("scan_bios_plugins memcards opendir");
		return;
	}

	while (1) {
		struct stat st;

		errno = 0;
		ent = readdir(dir);
		if (ent == NULL) {
			if (errno != 0)
				perror("readdir");
			break;
		}

		if (ent->d_type != DT_REG && ent->d_type != DT_LNK)
			continue;

		emu_make_path(fname, sizeof(fname), MEMCARD_DIR, ent->d_name);
		if (stat(fname, &st) != 0) {
			printf("bad memcard file: %s\n", ent->d_name);
			continue;
		}

		if (mc_i < ARRAY_SIZE(memcards) - 1) {
			memcards[mc_i++] = strdup(ent->d_name);
			continue;
		}

		printf("too many memcards, dropping \"%s\"\n", ent->d_name);
	}

	if (mc_i > 2)
		qsort(memcards + 1, mc_i - 1, sizeof(memcards[0]), qsort_strcmp);

	closedir(dir);
}

void menu_init(void)
{
	char buff[MAXPATHLEN];
	int i;

	cpu_clock_st = cpu_clock = plat_target_cpu_clock_get();

	scan_bios_plugins();
	menu_init_base();

	menu_set_defconfig();
	menu_load_config(0);
	menu_do_last_cd_img(1);
	last_vout_w = 320;
	last_vout_h = 240;
	last_vout_bpp = 16;

	g_menubg_src_ptr = calloc(g_menuscreen_w * g_menuscreen_h * 2, 1);
	g_menubg_ptr = calloc(g_menuscreen_w * g_menuscreen_h * 2, 1);
	if (g_menubg_src_ptr == NULL || g_menubg_ptr == NULL) {
		fprintf(stderr, "OOM\n");
		exit(1);
	}

	emu_make_data_path(buff, "skin/background.png", sizeof(buff));
	readpng(g_menubg_src_ptr, buff, READPNG_BG, g_menuscreen_w, g_menuscreen_h);

	i = plat_target.cpu_clock_set != NULL
		&& plat_target.cpu_clock_get != NULL && cpu_clock_st > 0;
	me_enable(e_menu_options, MA_OPT_CPU_CLOCKS, i);

	i = me_id2offset(e_menu_gfx_options, MA_OPT_VOUT_MODE);
	e_menu_gfx_options[i].data = plat_target.vout_methods;
	me_enable(e_menu_gfx_options, MA_OPT_VOUT_MODE,
		plat_target.vout_methods != NULL);

#ifndef SDL_OVERLAY_2X
	i = me_id2offset(e_menu_gfx_options, MA_OPT_VOUT_FULL);
	e_menu_gfx_options[i].data = plat_target.vout_methods;
	me_enable(e_menu_gfx_options, MA_OPT_VOUT_FULL, 0);
#endif

	i = me_id2offset(e_menu_gfx_options, MA_OPT_HWFILTER);
	e_menu_gfx_options[i].data = plat_target.hwfilters;
	me_enable(e_menu_gfx_options, MA_OPT_HWFILTER, plat_target.hwfilters != NULL);
	if (plat_target.hwfilters && !strcmp(plat_target.hwfilters[0], "linear"))
		e_menu_gfx_options[i].name = "OpenGL filter";
	else
		me_enable(e_menu_gfx_options, MA_OPT_VSYNC, 0);

	me_enable(e_menu_gfx_options, MA_OPT_GAMMA, plat_target.gamma_set != NULL);
#ifndef HAVE_NEON32
	me_enable(e_menu_gfx_options, MA_OPT_SWFILTER, 0);
#endif
	me_enable(e_menu_gfx_options, MA_OPT_VARSCALER, MENU_SHOW_VARSCALER);
	me_enable(e_menu_gfx_options, MA_OPT_VOUT_MODE, MENU_SHOW_VOUTMODE);
	me_enable(e_menu_gfx_options, MA_OPT_VARSCALER_C, MENU_SHOW_VARSCALER_C);
	me_enable(e_menu_gfx_options, MA_OPT_SCALER2, MENU_SHOW_SCALER2);
	me_enable(e_menu_keyconfig, MA_CTRL_NUBS_BTNS, MENU_SHOW_NUBS_BTNS);
	me_enable(e_menu_keyconfig, MA_CTRL_VIBRATION, MENU_SHOW_VIBRATION);
	me_enable(e_menu_keyconfig, MA_CTRL_DEADZONE, MENU_SHOW_DEADZONE);
}

void menu_notify_mode_change(int w, int h, int bpp)
{
	last_vout_w = w;
	last_vout_h = h;
	last_vout_bpp = bpp;
}

static void menu_leave_emu(void)
{
	if (GPU_close != NULL) {
		int ret = GPU_close();
		if (ret)
			fprintf(stderr, "Warning: GPU_close returned %d\n", ret);
	}

	plat_video_menu_enter(ready_to_go);

	memcpy(g_menubg_ptr, g_menubg_src_ptr, g_menuscreen_w * g_menuscreen_h * 2);
	if (pl_vout_buf != NULL && ready_to_go) {
		int x = max(0, g_menuscreen_w - last_vout_w);
		int y = max(0, g_menuscreen_h / 2 - last_vout_h / 2);
		int w = min(g_menuscreen_w, last_vout_w);
		int h = min(g_menuscreen_h, last_vout_h);
		u16 *d = (u16 *)g_menubg_ptr + g_menuscreen_w * y + x;
		char *s = pl_vout_buf;

		if (last_vout_bpp == 16) {
			for (; h > 0; h--, d += g_menuscreen_w, s += last_vout_w * 2)
				menu_darken_bg(d, s, w, 0);
		}
		else {
			for (; h > 0; h--, d += g_menuscreen_w, s += last_vout_w * 3) {
				rgb888_to_rgb565(d, s, w * 3);
				menu_darken_bg(d, d, w, 0);
			}
		}
	}

	if (ready_to_go)
		cpu_clock = plat_target_cpu_clock_get();
}

void menu_prepare_emu(void)
{
	R3000Acpu *prev_cpu = psxCpu;

	plat_video_menu_leave();

	#if !defined(DRC_DISABLE) || defined(LIGHTREC)
	psxCpu = (Config.Cpu == CPU_INTERPRETER) ? &psxInt : &psxRec;
	#else
	psxCpu = &psxInt;
	#endif
	if (psxCpu != prev_cpu) {
		prev_cpu->Notify(R3000ACPU_NOTIFY_BEFORE_SAVE, NULL);
		prev_cpu->Shutdown();
		psxCpu->Init();
		psxCpu->Notify(R3000ACPU_NOTIFY_AFTER_LOAD, NULL);
	}

	menu_sync_config();
	psxCpu->ApplyConfig();

	if (cpu_clock > 0)
		plat_target_cpu_clock_set(cpu_clock);

	// push config to GPU plugin
	plugin_call_rearmed_cbs();

	if (GPU_open != NULL) {
		int ret = GPU_open(&gpuDisp, "PCSX", NULL);
		if (ret)
			fprintf(stderr, "Warning: GPU_open returned %d\n", ret);
	}
}

void menu_update_msg(const char *msg)
{
	strncpy(menu_error_msg, msg, sizeof(menu_error_msg));
	menu_error_msg[sizeof(menu_error_msg) - 1] = 0;

	menu_error_time = plat_get_ticks_ms();
	lprintf("msg: %s\n", menu_error_msg);
}

void menu_finish(void)
{
	if (cpu_clock_st > 0)
		plat_target_cpu_clock_set(cpu_clock_st);
}
