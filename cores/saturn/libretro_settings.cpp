#include "libretro_settings.h"

int setting_region = 0;
int setting_cart = 1; /*CART_BACKUP_MEM*/
bool setting_smpc_autortc = true;
int setting_smpc_autortc_lang = 0;
int setting_initial_scanline = 0;
int setting_initial_scanline_pal = 0;
int setting_last_scanline = 239;
int setting_last_scanline_pal = 287;
int setting_gun_crosshair = SETTING_GUN_CROSSHAIR_CROSS;
int setting_gun_input = SETTING_GUN_INPUT_LIGHTGUN;
bool setting_disc_test = false;
bool setting_multitap_port1;
bool setting_multitap_port2;
bool opposite_directions;
bool setting_midsync;
