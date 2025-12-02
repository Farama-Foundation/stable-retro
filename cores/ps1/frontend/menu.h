#ifndef __MENU_H__
#define __MENU_H__

void menu_init(void);
void menu_prepare_emu(void);
void menu_loop(void);
void menu_finish(void);

void menu_notify_mode_change(int w, int h, int bpp);
int  menu_load_config(int is_game);

enum g_opts_opts {
	OPT_SHOWFPS = 1 << 0,
	OPT_SHOWCPU = 1 << 1,
	OPT_NO_FRAMELIM = 1 << 2,
	OPT_SHOWSPU = 1 << 3,
	OPT_TSGUN_NOTRIGGER = 1 << 4,
	OPT_VSYNC = 1 << 5,
};

enum g_scaler_opts {
	SCALE_1_1,
	SCALE_2_2,
	SCALE_4_3,
	SCALE_4_3v2,
	SCALE_FULLSCREEN,
	SCALE_CUSTOM,
};

enum g_soft_filter_opts {
	SOFT_FILTER_NONE,
	SOFT_FILTER_SCALE2X,
	SOFT_FILTER_EAGLE2X,
};

extern int g_opts, g_scaler, g_gamma;
extern int scanlines, scanline_level;
extern int soft_scaling, analog_deadzone;
extern int soft_filter;

extern int g_menuscreen_w;
extern int g_menuscreen_h;

#endif /* __MENU_H__ */
