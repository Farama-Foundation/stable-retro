// ------------ gfx options menu ------------

static const char *men_scaling_opts[] = { "OFF", "software", "hardware", NULL };
static const char *men_filter_opts[] = { "nearest", "smoother", "bilinear 1", "bilinear 2", NULL };

static const char h_scale[] = "hardware scaling might not work on some devices";
static const char h_stype[] = "scaler algorithm for software scaling";

#define MENU_OPTIONS_GFX \
	mee_enum_h    ("Horizontal scaling",     MA_OPT_SCALING, currentConfig.scaling, men_scaling_opts, h_scale), \
	mee_enum_h    ("Vertical scaling",       MA_OPT_VSCALING, currentConfig.vscaling, men_scaling_opts, h_scale), \
	mee_enum_h    ("Scaler type",            MA_OPT3_FILTERING, currentConfig.filter, men_filter_opts, h_stype), \

#define MENU_OPTIONS_ADV

void linux_menu_init(void)
{
}

