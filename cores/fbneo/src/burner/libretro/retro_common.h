#ifndef __RETRO_COMMON__
#define __RETRO_COMMON__

#include <string>
#include <sstream>
#include <algorithm>
#include <vector>
#include "burner.h"
#include "retro_string.h"

#define SSTR( x ) static_cast< const std::ostringstream & >(( std::ostringstream() << std::dec << x ) ).str()

#define RETRO_GAME_TYPE_CV		1
#define RETRO_GAME_TYPE_GG		2
#define RETRO_GAME_TYPE_MD		3
#define RETRO_GAME_TYPE_MSX		4
#define RETRO_GAME_TYPE_PCE		5
#define RETRO_GAME_TYPE_SG1K	6
#define RETRO_GAME_TYPE_SGX		7
#define RETRO_GAME_TYPE_SMS		8
#define RETRO_GAME_TYPE_TG		9
#define RETRO_GAME_TYPE_SPEC	10
#define RETRO_GAME_TYPE_NES		11
#define RETRO_GAME_TYPE_FDS		12
#define RETRO_GAME_TYPE_NEOCD	13
#define RETRO_GAME_TYPE_NGP		14
#define RETRO_GAME_TYPE_CHF		15
#define RETRO_GAME_TYPE_SNES	16

#define PERCENT_VALUES \
	  {"25%",	NULL }, \
	  {"26%",	NULL }, \
	  {"27%",	NULL }, \
	  {"28%",	NULL }, \
	  {"29%",	NULL }, \
	  {"30%",	NULL }, \
	  {"31%",	NULL }, \
	  {"32%",	NULL }, \
	  {"33%",	NULL }, \
	  {"34%",	NULL }, \
	  {"35%",	NULL }, \
	  {"36%",	NULL }, \
	  {"37%",	NULL }, \
	  {"38%",	NULL }, \
	  {"39%",	NULL }, \
	  {"40%",	NULL }, \
	  {"41%",	NULL }, \
	  {"42%",	NULL }, \
	  {"43%",	NULL }, \
	  {"44%",	NULL }, \
	  {"45%",	NULL }, \
	  {"46%",	NULL }, \
	  {"47%",	NULL }, \
	  {"48%",	NULL }, \
	  {"49%",	NULL }, \
	  {"50%",	NULL }, \
	  {"51%",	NULL }, \
	  {"52%",	NULL }, \
	  {"53%",	NULL }, \
	  {"54%",	NULL }, \
	  {"55%",	NULL }, \
	  {"56%",	NULL }, \
	  {"57%",	NULL }, \
	  {"58%",	NULL }, \
	  {"59%",	NULL }, \
	  {"60%",	NULL }, \
	  {"61%",	NULL }, \
	  {"62%",	NULL }, \
	  {"63%",	NULL }, \
	  {"64%",	NULL }, \
	  {"65%",	NULL }, \
	  {"66%",	NULL }, \
	  {"67%",	NULL }, \
	  {"68%",	NULL }, \
	  {"69%",	NULL }, \
	  {"70%",	NULL }, \
	  {"71%",	NULL }, \
	  {"72%",	NULL }, \
	  {"73%",	NULL }, \
	  {"74%",	NULL }, \
	  {"75%",	NULL }, \
	  {"76%",	NULL }, \
	  {"77%",	NULL }, \
	  {"78%",	NULL }, \
	  {"79%",	NULL }, \
	  {"80%",	NULL }, \
	  {"81%",	NULL }, \
	  {"82%",	NULL }, \
	  {"83%",	NULL }, \
	  {"84%",	NULL }, \
	  {"85%",	NULL }, \
	  {"86%",	NULL }, \
	  {"87%",	NULL }, \
	  {"88%",	NULL }, \
	  {"89%",	NULL }, \
	  {"90%",	NULL }, \
	  {"91%",	NULL }, \
	  {"92%",	NULL }, \
	  {"93%",	NULL }, \
	  {"94%",	NULL }, \
	  {"95%",	NULL }, \
	  {"96%",	NULL }, \
	  {"97%",	NULL }, \
	  {"98%",	NULL }, \
	  {"99%",	NULL }, \
	  {"100%",	NULL }, \
	  {"105%",	NULL }, \
	  {"110%",	NULL }, \
	  {"115%",	NULL }, \
	  {"120%",	NULL }, \
	  {"125%",	NULL }, \
	  {"130%",	NULL }, \
	  {"135%",	NULL }, \
	  {"140%",	NULL }, \
	  {"145%",	NULL }, \
	  {"150%",	NULL }, \
	  {"155%",	NULL }, \
	  {"160%",	NULL }, \
	  {"165%",	NULL }, \
	  {"170%",	NULL }, \
	  {"175%",	NULL }, \
	  {"180%",	NULL }, \
	  {"185%",	NULL }, \
	  {"190%",	NULL }, \
	  {"195%",	NULL }, \
	  {"200%",	NULL }, \
	  {"210%",	NULL }, \
	  {"220%",	NULL }, \
	  {"230%",	NULL }, \
	  {"240%",	NULL }, \
	  {"250%",	NULL }, \
	  {"260%",	NULL }, \
	  {"270%",	NULL }, \
	  {"280%",	NULL }, \
	  {"290%",	NULL }, \
	  {"300%",	NULL }, \
	  {"310%",	NULL }, \
	  {"320%",	NULL }, \
	  {"330%",	NULL }, \
	  {"340%",	NULL }, \
	  {"350%",	NULL }, \
	  {"360%",	NULL }, \
	  {"370%",	NULL }, \
	  {"380%",	NULL }, \
	  {"390%",	NULL }, \
	  {"400%",	NULL }, \
	  { NULL,	NULL },

struct dipswitch_core_option_value
{
	struct GameInp *pgi;
	BurnDIPInfo bdi;
	std::string friendly_name;
	struct GameInp *cond_pgi;
	int nCondMask;
	int nCondSetting;
};

struct dipswitch_core_option
{
	std::string option_name;
	std::string friendly_name;
	std::string friendly_name_categorized;
	BurnDIPInfo default_bdi;
	std::vector<dipswitch_core_option_value> values;
};

struct cheat_core_option_value
{
	int nValue;
	std::string friendly_name;
};

struct cheat_core_option
{
	int num;
	std::string default_value;
	std::string option_name;
	std::string friendly_name;
	std::string friendly_name_categorized;
	std::vector<cheat_core_option_value> values;
};

struct ips_core_option
{
	std::string dat_path;						// Path to the ipses index file
	std::string option_name;					// key				= "fbneo-ips-<drvname>-<int>"
	std::string friendly_name;					// desc				= "Category/Patch Name"
												// desc_categorized	= NULL;
	std::string friendly_name_categorized;		// info				= "Patch Description"
												// info_categorized	= NULL;
												// category_key		= "ips"
												// value
												// default_value	= "disabled"
};

struct romdata_core_option
{
	std::string dat_path;						// Path to the romdatas index file
	std::string option_name;					// key				= "fbneo-romdata-<drvname>-<int>"
	std::string friendly_name;					// desc				= "Full name of the game"
												// desc_categorized	= NULL;
												// info				= "Specific to the running romset and your romdata database"
												// info_categorized	= NULL;
												// category_key		= "romdata"
												// value
												// default_value	= "disabled"
};

enum neogeo_bios_categories
{
	NEOGEO_MVS = 1<<0,
	NEOGEO_AES = 1<<1,
	NEOGEO_UNI = 1<<2,
	NEOGEO_EUR = 1<<3,
	NEOGEO_USA = 1<<4,
	NEOGEO_JAP = 1<<5,
};

struct RomBiosInfo {
	char* filename;
	uint32_t crc;
	uint8_t NeoSystem;
	char* friendly_name;
	uint32_t categories;
	uint32_t available;
};

extern retro_log_printf_t log_cb;
extern retro_environment_t environ_cb;
extern std::vector<dipswitch_core_option> dipswitch_core_options;
extern std::vector<cheat_core_option> cheat_core_options;
extern std::vector<ips_core_option> ips_core_options;
extern std::vector<romdata_core_option> romdata_core_options;
extern struct GameInp *pgi_reset;
extern struct GameInp *pgi_diag;
extern struct GameInp *pgi_debug_dip_1;
extern struct GameInp *pgi_debug_dip_2;
extern bool bIsNeogeoCartGame;
extern bool allow_neogeo_mode;
extern bool core_aspect_par;
extern bool bAllowDepth32;
extern bool bPatchedRomsetsEnabled;
extern bool bLibretroSupportsAudioBuffStatus;
extern bool bLowPassFilterEnabled;
extern UINT32 nVerticalMode;
extern UINT32 nNewWidth;
extern UINT32 nNewHeight;
extern UINT32 nFrameskip;
extern UINT32 nFrameskipType;
extern UINT32 nFrameskipThreshold;
extern UINT32 nMemcardMode;
extern UINT32 nLightgunCrosshairEmulation;
extern UINT8 NeoSystem;
extern INT32 g_audio_samplerate;
extern UINT8 *diag_input;
extern unsigned nGameType;
extern char g_rom_dir[MAX_PATH];
extern TCHAR szAppPathDefPath[MAX_PATH];
extern TCHAR szAppIpsesPath[MAX_PATH];
extern TCHAR szAppRomdatasPath[MAX_PATH];
extern UINT32 nDiagInputHoldCounter;

char* str_char_replace(char* destination, char c_find, char c_replace);
void set_neo_system_bios();
void set_neogeo_bios_availability(char *szName, uint32_t crc, bool ignoreCrc);
void evaluate_neogeo_bios_mode(const char* drvname);
void set_environment();
void check_variables(void);
int HandleMessage(enum retro_log_level level, TCHAR* szFormat, ...);
char* strqtoken(char* s, const char* delims);
char* TCHARToANSI(const TCHAR* pszInString, char* pszOutString, int /*nOutSize*/);
INT32 create_variables_from_ipses();
INT32 reset_ipses_from_variables();
INT32 apply_ipses_from_variables();
INT32 create_variables_from_romdatas();
INT32 reset_romdatas_from_variables();
INT32 apply_romdatas_from_variables();
#ifdef USE_CYCLONE
void SetSekCpuCore();
#endif

#endif
