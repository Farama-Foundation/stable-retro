
static const char *men_hscaling_opts[] = { "OFF", "4:3", "wide", "fullscreen", NULL };
static const char *men_vscaling_opts[] = { "OFF", "4:3", "fullscreen", NULL };
static const char *men_filter_opts[] = { "nearest", "bilinear", NULL };

#define MENU_OPTIONS_GFX \
	mee_enum    ("Horizontal scaling", MA_OPT_SCALING,    currentConfig.scaling, men_hscaling_opts), \
	mee_enum    ("Vertical scaling",   MA_OPT_VSCALING,   currentConfig.vscaling, men_vscaling_opts), \
	mee_enum    ("Scaler type",        MA_OPT3_FILTERING, currentConfig.filter, men_filter_opts), \
	mee_range   ("Gamma adjustment",   MA_OPT3_GAMMAA,    currentConfig.gamma, -4, 16), \
	mee_range   ("Black level",        MA_OPT3_BLACKLVL,  currentConfig.gamma2, 0,  2), \
	mee_onoff   ("Wait for vsync",     MA_OPT3_VSYNC,     currentConfig.EmuOpt, EOPT_VSYNC), \

#define MENU_OPTIONS_ADV

static menu_entry e_menu_sms_options[];

void psp_menu_init(void)
{
	me_enable(e_menu_sms_options, MA_SMSOPT_GHOSTING, 0);
}
