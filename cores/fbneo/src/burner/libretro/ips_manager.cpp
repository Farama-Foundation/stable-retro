#include "burner.h"
#include "retro_common.h"
#include "retro_dirent.h"

#include <file/file_path.h>

#define NUM_LANGUAGES		12
#define MAX_ACTIVE_PATCHES	1024

#ifndef MAX_PATH
#define MAX_PATH			260
#endif

// Game patching

#define UTF8_SIGNATURE	"\xef\xbb\xbf"
#define IPS_SIGNATURE	"PATCH"
#define IPS_TAG_EOF		"EOF"
#define IPS_EXT			".ips"

#define BYTE3_TO_UINT(bp)						\
	(((UINT32)(bp)[0] << 16) & 0x00FF0000) |	\
	(((UINT32)(bp)[1] <<  8) & 0x0000FF00) |	\
	(( UINT32)(bp)[2]        & 0x000000FF)

#define BYTE2_TO_UINT(bp)				\
	(((UINT32)(bp)[0] << 8) & 0xFF00) |	\
	(( UINT32)(bp)[1]       & 0x00FF)

bool bDoIpsPatch = false;
UINT32 nIpsDrvDefine = 0, nIpsMemExpLen[SND2_ROM + 1] = { 0 };
TCHAR szAppIpsPath[MAX_PATH] = { 0 };

std::vector<ips_core_option> ips_core_options;

static INT32 nRomOffset = 0, nActiveArray = 0, nStandalone, nDefineNum = 0;
static TCHAR szAltFile[MAX_PATH] = { 0 }, szAltName[MAX_PATH] = { 0 }, * pszAltDesc = NULL, ** pszIpsActivePatches = NULL;
static TCHAR CoreIpsPaths[DIRS_MAX][MAX_PATH];

static const TCHAR szLanguageCodes[NUM_LANGUAGES][6] = {
	_T("en_US"),	_T("zh_CN"),	_T("zh_TW"),	_T("ja_JP"),	_T("ko_KR"),	_T("fr_FR"),
	_T("es_ES"),	_T("it_IT"),	_T("de_DE"),	_T("pt_BR"),	_T("pl_PL"),	_T("hu_HU")
};

// Read in the config file for the whole application
INT32 CoreIpsPathsLoad()
{
	TCHAR szConfig[MAX_PATH] = { 0 }, szLine[1024] = { 0 };
	FILE* h = NULL;

#ifdef _UNICODE
	setlocale(LC_ALL, "");
#endif

	for (INT32 i = 0; i < DIRS_MAX; i++)
		memset(CoreIpsPaths[i], 0, MAX_PATH * sizeof(TCHAR));

	char* p = find_last_slash(szAppIpsesPath);						// g_system_dir/fbneo/ips/
	if ((NULL != p) && ('\0' == p[1])) p[0] = '\0';					// g_system_dir/fbneo/ips

	_tcscpy(CoreIpsPaths[0], szAppIpsesPath);						// CoreIpsPaths[0] = g_system_dir/fbneo/ips

	snprintf(
		szConfig, MAX_PATH - 1, "%sips_path.opt",
		szAppPathDefPath
	);																// g_system_dir/fbneo/path/ips_path.opt

	if (NULL == (h = _tfopen(szConfig, _T("rt"))))
	{
		memset(szConfig, 0, MAX_PATH * sizeof(TCHAR));
		snprintf(
			szConfig, MAX_PATH - 1, "%s%cips_path.opt",
			g_rom_dir, PATH_DEFAULT_SLASH_C()
		);															// g_rom_dir/ips_path.opt

		if (NULL == (h = _tfopen(szConfig, _T("rt"))))
			return 1;												// Only CoreIpsPaths[0]
	}

	// Go through each line of the config file
	while (_fgetts(szLine, 1024, h)) {
		int nLen = _tcslen(szLine);

		// Get rid of the linefeed at the end
		if (nLen > 0 && szLine[nLen - 1] == 10) {
			szLine[nLen - 1] = 0;
			nLen--;
		}

#define STR(x) { TCHAR* szValue = LabelCheck(szLine,_T(#x) _T(" "));	\
  if (szValue) _tcscpy(x,szValue); }

//		STR(CoreIpsPaths[0]);										// g_system_dir/fbneo/ips
		STR(CoreIpsPaths[1]);
		STR(CoreIpsPaths[2]);
		STR(CoreIpsPaths[3]);
		STR(CoreIpsPaths[4]);
		STR(CoreIpsPaths[5]);
		STR(CoreIpsPaths[6]);
		STR(CoreIpsPaths[7]);
		STR(CoreIpsPaths[8]);
		STR(CoreIpsPaths[9]);
		STR(CoreIpsPaths[10]);
		STR(CoreIpsPaths[11]);
		STR(CoreIpsPaths[12]);
		STR(CoreIpsPaths[13]);
		STR(CoreIpsPaths[14]);
		STR(CoreIpsPaths[15]);
		STR(CoreIpsPaths[16]);
		STR(CoreIpsPaths[17]);
		STR(CoreIpsPaths[18]);
		STR(CoreIpsPaths[19]);
#undef STR
	}

	fclose(h);
	return 0;														// There may be more
}

static INT32 GetLanguageCode()
{
	INT32 nLangcode = 0;

	environ_cb(RETRO_ENVIRONMENT_GET_LANGUAGE, &nLangcode);

	switch (nLangcode)
	{
		case RETRO_LANGUAGE_CHINESE_SIMPLIFIED:
			nLangcode =  1; break;
		case RETRO_LANGUAGE_CHINESE_TRADITIONAL:
			nLangcode =  2; break;
		case RETRO_LANGUAGE_JAPANESE:
			nLangcode =  3; break;
		case RETRO_LANGUAGE_KOREAN:
			nLangcode =  4; break;
		case RETRO_LANGUAGE_FRENCH:
			nLangcode =  5; break;
		case RETRO_LANGUAGE_SPANISH:
			nLangcode =  6; break;
		case RETRO_LANGUAGE_ITALIAN:
			nLangcode =  7; break;
		case RETRO_LANGUAGE_GERMAN:
			nLangcode =  8; break;
		case RETRO_LANGUAGE_PORTUGUESE_BRAZIL:
			nLangcode =  9; break;
		case RETRO_LANGUAGE_POLISH:
			nLangcode = 10; break;
		case RETRO_LANGUAGE_HUNGARIAN:
			nLangcode = 11; break;
		default:
			nLangcode =  0; break;
			break;
	}

	return nLangcode;
}

static TCHAR* GetPatchDescByLangcode(FILE* fp, int nLang)
{
	TCHAR* result = NULL;
	char* desc = NULL, langtag[10] = { 0 };

	sprintf(langtag, "[%s]", TCHARToANSI(szLanguageCodes[nLang], NULL, 0));

	fseek(fp, 0, SEEK_SET);

	while (!feof(fp))
	{
		char s[4096] = { 0 };

		if (fgets(s, sizeof(s), fp) != NULL)
		{
			if (strncmp(langtag, s, strlen(langtag)) != 0)
				continue;

			while (fgets(s, sizeof(s), fp) != NULL)
			{
				char* p = NULL;

				if (*s == '[')
				{
					if (desc)
					{
						pszAltDesc = (char*)malloc(strlen(desc) + 1);
						result = (NULL != pszAltDesc) ? strcpy(pszAltDesc, desc) : NULL;
						free(desc);
						desc = NULL;
						return result;
					}
					else
						return NULL;
				}

				for (p = s; *p; p++)
				{
					if (*p == '\r' || *p == '\n')
					{
						*p = '\0'; break;
					}
				}

				if (desc)
				{
					char* p1;
					int len = strlen(desc);

					len += strlen(s) + 2;
					p1 = (char*)malloc(len + 1);
					sprintf(p1, "%s\r\n%s", desc, s);
					if (desc) {
						free(desc);
					}
					desc = p1;
				}
				else
				{
					desc = (char*)malloc(strlen(s) + 1);
					if (desc != NULL)
						strcpy(desc, s);
				}
			}
		}
	}

	if (desc)
	{
		pszAltDesc = (char*)malloc(strlen(desc) + 1);
		result = (NULL != pszAltDesc) ? strcpy(pszAltDesc, desc) : NULL;
		free(desc);
		desc = NULL;
		return result;
	}
	else
		return NULL;
}

INT32 create_variables_from_ipses()
{
	INT32 nRet = 0;
	const char* pszDrvName = BurnDrvGetTextA(DRV_NAME), * pszExt = ".dat";
	if (NULL == pszDrvName) return -2;

	ips_core_options.clear();
	CoreIpsPathsLoad();

	for (INT32 i = 0; i < DIRS_MAX; i++)
	{
		if (NULL == CoreIpsPaths[i]) continue;

		char* p = find_last_slash(CoreIpsPaths[i]);
		if ((NULL != p) && ('\0' == p[1])) p[0] = '\0';

		char szFilePathSearch[MAX_PATH] = { 0 }, szPatchPaths[MAX_PATH] = { 0 };
		snprintf(
			szFilePathSearch, MAX_PATH - 1, "%s%c%s%c",
			CoreIpsPaths[i], PATH_DEFAULT_SLASH_C(), pszDrvName, PATH_DEFAULT_SLASH_C()
		);

		// ips_dirs/drvname_dir/
		strcpy(szPatchPaths, szFilePathSearch);

		struct RDIR* entry = retro_opendir_include_hidden(szFilePathSearch, true);

		if (!entry || retro_dirent_error(entry)) continue;

		INT32 nLangcode = GetLanguageCode();
		FILE* fp = NULL;

		while (retro_readdir(entry))
		{
			const char* name = retro_dirent_get_name(entry);

			if (
				retro_dirent_is_dir(entry, NULL) ||
				(0 != strcmp(pszExt, strrchr(name, '.'))) ||
				(0 == strcmp(name, ".")) || (0 == strcmp(name, ".."))
				)
				continue;

			memset(szFilePathSearch, 0, MAX_PATH * sizeof(TCHAR));

			// ips_dirs/drvname_dir/xx.dat
			snprintf(
				szFilePathSearch, MAX_PATH - 1, "%s%s",
				szPatchPaths, name
			);

			if (NULL == (fp = fopen(szFilePathSearch, "r"))) continue;
			strcpy(szAltFile, szFilePathSearch);

			TCHAR* pszPatchDesc = GetPatchDescByLangcode(fp, nLangcode);
			// If not available - try English first
			if (NULL == pszPatchDesc) pszPatchDesc = GetPatchDescByLangcode(fp, 0);
			// Simplified Chinese is the reference language (should always be available!!)
			if (NULL == pszPatchDesc) pszPatchDesc = GetPatchDescByLangcode(fp, 1);
			if (NULL == pszPatchDesc)
			{
				pszPatchDesc = (TCHAR*)malloc(1024);
				memset(pszPatchDesc, 0, 1024 * sizeof(TCHAR));
				_tcscpy(pszPatchDesc, name);
			}

			p = NULL;

			for (UINT32 x = 0; x < _tcslen(pszPatchDesc); x++) {
				if ((pszPatchDesc[x] == '\r') || (pszPatchDesc[x] == '\n')) break;

				szAltName[x] = pszPatchDesc[x];
				p = pszPatchDesc + x;
			}

			while (NULL != pszPatchDesc)
			{
				p++;
				if ((p[0] != '\r') && (p[0] != '\n')) break;
			}

			char szKey[100] = { 0 };
			sprintf(szKey, "fbneo-ips-%s-%d", pszDrvName, nRet);

			ips_core_options.push_back(ips_core_option());
			ips_core_option* ips_option = &ips_core_options.back();
			ips_option->dat_path        = szAltFile;
			ips_option->option_name     = szKey;
			ips_option->friendly_name   = szAltName;
			std::string option_name     = (0 != strcmp("", p)) ? p : RETRO_IPS_DEF_INFO;
			std::replace(option_name.begin(), option_name.end(), '\r', ' ');
			ips_option->friendly_name_categorized = option_name;

			if (NULL != pszPatchDesc) free(pszPatchDesc);

			memset(szAltFile, 0, MAX_PATH * sizeof(TCHAR));
			memset(szAltName, 0, MAX_PATH * sizeof(TCHAR));

			fclose(fp);
			nRet++;
		}
	}
	return nRet;
}

INT32 reset_ipses_from_variables()
{
	struct retro_variable var = { 0 };

	for (INT32 ips_idx = 0; ips_idx < ips_core_options.size(); ips_idx++)
	{
		ips_core_option* ips_option = &ips_core_options[ips_idx];
		var.key = ips_option->option_name.c_str();

		if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) == false || !var.value)
			continue;

		var.value = "disabled";

		if (environ_cb(RETRO_ENVIRONMENT_SET_VARIABLE, &var) == false)
			return -1;
	}

	return 0;
}

INT32 apply_ipses_from_variables()
{
	if (!bPatchedRomsetsEnabled) {
		reset_ipses_from_variables(); 
		return -2;
	}

	IpsPatchExit();

	struct retro_variable var = { 0 };

	for (INT32 ips_idx = 0; ips_idx < ips_core_options.size(); ips_idx++)
	{
		ips_core_option* ips_option = &ips_core_options[ips_idx];
		var.key = ips_option->option_name.c_str();

		if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) == false || !var.value)
			continue;

		if (0 == strcmp(var.value, "enabled")) nActiveArray++;
	}

	const INT32 nActive = nActiveArray; nActiveArray = 0;

	if (nActive > 0) pszIpsActivePatches = (TCHAR**)malloc(nActive * sizeof(TCHAR*));

	for (INT32 ips_idx = 0; ips_idx < ips_core_options.size(); ips_idx++)
	{
		ips_core_option* ips_option = &ips_core_options[ips_idx];
		var.key = ips_option->option_name.c_str();

		if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) == false || !var.value)
			continue;

		if (0 == strcmp(var.value, "enabled"))
		{
			if (NULL != pszIpsActivePatches)
			{
				pszIpsActivePatches[nActiveArray] = (TCHAR*)malloc(MAX_PATH * sizeof(TCHAR));
				memset(pszIpsActivePatches[nActiveArray], 0, MAX_PATH * sizeof(TCHAR));
				_tcscpy(pszIpsActivePatches[nActiveArray], ips_option->dat_path.c_str());
				nActiveArray++;
			}

			var.value = "disabled";

			if (environ_cb(RETRO_ENVIRONMENT_SET_VARIABLE, &var) == false)
				return -1;
		}
	}

	return nActiveArray;
}

static INT32 GetIpsNumActivePatches()
{
	if (NULL == pszIpsActivePatches)
		return 0;

	return nActiveArray;
}

static void PatchFile(const char* ips_path, UINT8* base, bool readonly)
{
	char buf[6];
	FILE* f = NULL;
	INT32 Offset = 0, Size = 0;
	UINT8* mem8 = NULL;

	if (NULL == (f = fopen(ips_path, "rb"))) return;

	memset(buf, 0, sizeof(buf));
	fread(buf, 1, 5, f);

	if (strcmp(buf, IPS_SIGNATURE))
	{
		if (f) fclose(f);

		return;
	}
	else
	{
		UINT8 ch = 0;
		INT32 bRLE = 0;

		while (!feof(f)) {
			// read patch address offset
			fread(buf, 1, 3, f);
			buf[3] = 0;

			if (0 == strcmp(buf, IPS_TAG_EOF))
				break;

			Offset = BYTE3_TO_UINT(buf);

			// read patch length
			fread(buf, 1, 2, f);
			Size = BYTE2_TO_UINT(buf);

			bRLE = (0 == Size);
			if (bRLE) {
				fread(buf, 1, 2, f);
				Size = BYTE2_TO_UINT(buf);
				ch = fgetc(f);
			}

			while (Size--)
			{
				if (!readonly)
					mem8 = base + Offset + nRomOffset;
				
				Offset++;

				if (readonly)
				{
					if (!bRLE)
						fgetc(f);
				}
				else
					*mem8 = bRLE ? ch : fgetc(f);
			}
		}
	}

	// Avoid memory out-of-bounds due to ips offset greater than rom length.
	if (readonly && (0 == nIpsMemExpLen[EXP_FLAG]))	// Unspecified length.
		nIpsMemExpLen[LOAD_ROM] = Offset;

	fclose(f);
}

static char* stristr_int(const char* str1, const char* str2)
{
	const char* p1 = str1;
	const char* p2 = str2;
	const char* r = (!*p2) ? str1 : NULL;

	while (*p1 && *p2)
	{
		if (tolower((unsigned char)*p1) == tolower((unsigned char)*p2)) {
			if (!r)
				r = p1;

			p2++;
		}
		else
		{
			p2 = str2;

			if (r)
				p1 = r + 1;

			if (tolower((unsigned char)*p1) == tolower((unsigned char)*p2))
			{
				r = p1; p2++;
			}
			else
				r = NULL;
		}

		p1++;
	}

	return (*p2) ? NULL : (char*)r;
}

static UINT32 hexto32(const char *s)
{
	UINT32 val = 0;
	char c;

	while ((c = *s++))
	{
		UINT8 v = ((c & 0xf) + (c >> 6)) | ((c >> 3) & 0x08);
		val = (val << 4) | (UINT32)v;
	}

	return val;
}

// strqtoken() - functionally identicle to strtok() w/ special treatment for
// quoted strings.  -dink april 2023
char *strqtoken(char *s, const char *delims)
{
	static char* prev_str = NULL;
	char* token = NULL;

	if (!s)
		s = prev_str;

	s += strspn(s, delims);

	if (s[0] == '\0')
	{
		prev_str = s;
		return NULL;
	}

	if (s[0] == '\"') { // time to grab quoted string!
		token = ++s;

		if ((s = strpbrk(token, "\"")))
			*(s++) = '\0';
	}
	else
		token = s;

	if ((s = strpbrk(s, delims)))
	{
		*(s++) = '\0';
		prev_str = s;
	}
	else
		// we're at the end of the road
		prev_str = (char*)memchr((void *)token, '\0', MAX_PATH);

	return token;
}

static void DoPatchGame(const char* patch_name, char* game_name, UINT32 crc, UINT8* base, bool readonly)
{
	char s[MAX_PATH] = { 0 }, * p = NULL, * rom_name = NULL, * ips_name = NULL, * ips_offs = NULL, * ips_crc = NULL;
	UINT32 nIps_crc = 0;
	FILE* fp = NULL;

	if (NULL != (fp = fopen(patch_name, "rb")))
	{
		bool bTarget = false;

		while (!feof(fp))
		{
			if (NULL != fgets(s, sizeof(s), fp))
			{
				p = s;

				// skip UTF-8 sig
				if (strncmp(p, UTF8_SIGNATURE, strlen(UTF8_SIGNATURE)) == 0)
					p += strlen(UTF8_SIGNATURE);

				if (p[0] == '[')	// reached info-section of .dat file, time to leave.
					break;

				// Can support linetypes: (space or tab)
				// "rom name.bin" "patch file.ips" CRC(abcd1234)
				// romname.bin patchfile CRC(abcd1234)
				#define DELIM_TOKENS_NAME	" \t\r\n"
				#define DELIM_TOKENS		" \t\r\n()"

				rom_name = strqtoken(p, DELIM_TOKENS_NAME);

				if (!rom_name)
					continue;
				if (*rom_name == '#') {
					if (0 == strcmp(rom_name, "#define")) {
						if (nDefineNum > 0) break;
						nStandalone++;
					}
					continue;
				}

				ips_name = strqtoken(NULL, DELIM_TOKENS_NAME);
				if (!ips_name)
					continue;

				nIps_crc = nRomOffset = 0; // Reset to 0

				if (NULL != (ips_offs = strqtoken(NULL, DELIM_TOKENS)))
				{	// Parameters of the offset increment
					if (     0 == strcmp(ips_offs, "IPS_OFFSET_016")) nRomOffset = 0x1000000;
					else if (0 == strcmp(ips_offs, "IPS_OFFSET_032")) nRomOffset = 0x2000000;
					else if (0 == strcmp(ips_offs, "IPS_OFFSET_048")) nRomOffset = 0x3000000;
					else if (0 == strcmp(ips_offs, "IPS_OFFSET_064")) nRomOffset = 0x4000000;
					else if (0 == strcmp(ips_offs, "IPS_OFFSET_080")) nRomOffset = 0x5000000;
					else if (0 == strcmp(ips_offs, "IPS_OFFSET_096")) nRomOffset = 0x6000000;
					else if (0 == strcmp(ips_offs, "IPS_OFFSET_112")) nRomOffset = 0x7000000;
					else if (0 == strcmp(ips_offs, "IPS_OFFSET_128")) nRomOffset = 0x8000000;
					else if (0 == strcmp(ips_offs, "IPS_OFFSET_144")) nRomOffset = 0x9000000;

					if (nRomOffset != 0)	// better get next token (crc)
						ips_offs = strqtoken(NULL, DELIM_TOKENS);
				}

				if (ips_offs != NULL && stristr_int(ips_offs, "crc"))
				{
					ips_crc = strqtoken(NULL, DELIM_TOKENS);

					if (ips_crc)
						nIps_crc = hexto32(ips_crc);

				}

				char *has_ext = stristr_int(ips_name, ".ips");

				if (_stricmp(rom_name, game_name))	// name don't match?
					if (nIps_crc != crc)			// crc don't match?
						continue;					// not our file. next!

				bTarget = true;
				char ips_path[MAX_PATH * 2] = { 0 }, ips_dir[MAX_PATH] = { 0 }, * pszTmp = NULL;

				strcpy(ips_dir, patch_name);												// ips_dir/drvname_dir/ips.dat
				if (NULL != (pszTmp = strrchr(ips_dir, PATH_DEFAULT_SLASH_C())))
					pszTmp[1] = '\0';														// ips_dir/drvname_dir/
				sprintf(ips_path, "%s%s%s", ips_dir, ips_name, (has_ext) ? "" : IPS_EXT);	// ips_dir/drvname_dir/(sub_dir/)ipses.ips

				PatchFile(ips_path, base, readonly);
			}
		}
		fclose(fp);

		if (!bTarget && (0 == nIpsMemExpLen[EXP_FLAG]))
			nIpsMemExpLen[LOAD_ROM] = 0;	// Must be reset to 0!
	}
}

static UINT32 GetIpsDefineExpValue(char* szTmp)
{
	if (NULL == (szTmp = strqtoken(NULL, " \t\r\n")))
		return 0U;

	INT32 nRet = 0;

	if      (0 == strcmp(szTmp, "EXP_VALUE_001")) nRet = 0x0010000;
	else if (0 == strcmp(szTmp, "EXP_VALUE_002")) nRet = 0x0020000;
	else if (0 == strcmp(szTmp, "EXP_VALUE_003")) nRet = 0x0030000;
	else if (0 == strcmp(szTmp, "EXP_VALUE_004")) nRet = 0x0040000;
	else if (0 == strcmp(szTmp, "EXP_VALUE_005")) nRet = 0x0050000;
	else if (0 == strcmp(szTmp, "EXP_VALUE_006")) nRet = 0x0060000;
	else if (0 == strcmp(szTmp, "EXP_VALUE_007")) nRet = 0x0070000;
	else if (0 == strcmp(szTmp, "EXP_VALUE_008")) nRet = 0x0080000;
	else if (0 == strcmp(szTmp, "EXP_VALUE_010")) nRet = 0x0100000;
	else if (0 == strcmp(szTmp, "EXP_VALUE_020")) nRet = 0x0200000;
	else if (0 == strcmp(szTmp, "EXP_VALUE_030")) nRet = 0x0300000;
	else if (0 == strcmp(szTmp, "EXP_VALUE_040")) nRet = 0x0400000;
	else if (0 == strcmp(szTmp, "EXP_VALUE_050")) nRet = 0x0500000;
	else if (0 == strcmp(szTmp, "EXP_VALUE_060")) nRet = 0x0600000;
	else if (0 == strcmp(szTmp, "EXP_VALUE_070")) nRet = 0x0700000;
	else if (0 == strcmp(szTmp, "EXP_VALUE_080")) nRet = 0x0800000;
	else if (0 == strcmp(szTmp, "EXP_VALUE_100")) nRet = 0x1000000;
	else if (0 == strcmp(szTmp, "EXP_VALUE_200")) nRet = 0x2000000;
	else if (0 == strcmp(szTmp, "EXP_VALUE_300")) nRet = 0x3000000;
	else if (0 == strcmp(szTmp, "EXP_VALUE_400")) nRet = 0x4000000;
	else if (0 == strcmp(szTmp, "EXP_VALUE_500")) nRet = 0x5000000;
	else if (0 == strcmp(szTmp, "EXP_VALUE_600")) nRet = 0x6000000;
	else if (0 == strcmp(szTmp, "EXP_VALUE_700")) nRet = 0x7000000;
	else if (0 == strcmp(szTmp, "EXP_VALUE_800")) nRet = 0x8000000;
	else if (EOF != (sscanf(szTmp, "%x", &nRet))) return nRet;

	return nRet;
}

// Run once to get the definition & definition values of the DAT files.
// Suppress CPU usage caused by multiple runs.
// Two entry points: cmdline Launch & SelOkay.
void GetIpsDrvDefine()
{
	if (!bDoIpsPatch)
		return;

	nStandalone = nDefineNum = nIpsDrvDefine = 0;
	memset(nIpsMemExpLen, 0, sizeof(nIpsMemExpLen));

	INT32 nCount         = GetIpsNumActivePatches();
	INT32 nActivePatches = (0 == nCount) ? 1 : nCount;

	for (INT32 i = 0; i < nActivePatches; i++)
	{
		char ips_data[MAX_PATH] = { 0 };
		strcpy(ips_data, (0 == nCount) ? szAppIpsPath : pszIpsActivePatches[i]);

		char str[MAX_PATH] = { 0 }, * ptr = NULL, * tmp = NULL;
		FILE* fp = NULL;

		if (NULL != (fp = fopen(ips_data, "rb"))) {
			while (!feof(fp))
			{
				if (NULL != fgets(str, sizeof(str), fp)) {
					ptr = str;

					// skip UTF-8 sig
					if (0 == strncmp(ptr, UTF8_SIGNATURE, strlen(UTF8_SIGNATURE)))
						ptr += strlen(UTF8_SIGNATURE);

					if (NULL == (tmp = strqtoken(ptr, " \t\r\n")))
						continue;
					if (0 != strcmp(tmp, "#define"))
						break;
					if (NULL == (tmp = strqtoken(NULL, " \t\r\n")))
						break;

					UINT32 nNewValue = 0;

					if (0 == strcmp(tmp, "IPS_NOT_PROTECT")) {
						nIpsDrvDefine |= IPS_NOT_PROTECT;
						continue;
					}
					if (0 == strcmp(tmp, "IPS_PGM_SPRHACK")) {
						nIpsDrvDefine |= IPS_PGM_SPRHACK;
						continue;
					}
					if (0 == strcmp(tmp, "IPS_PGM_MAPHACK")) {
						nIpsDrvDefine |= IPS_PGM_MAPHACK;
						continue;
					}
					if (0 == strcmp(tmp, "IPS_PGM_SNDOFFS")) {
						nIpsDrvDefine |= IPS_PGM_SNDOFFS;
						continue;
					}
					if (0 == strcmp(tmp, "IPS_SNES_VRAMHK")) {
						nIpsDrvDefine |= IPS_SNES_VRAMHK;
						continue;
					}
					if (0 == strcmp(tmp, "IPS_LOAD_EXPAND")) {
						nIpsDrvDefine |= IPS_LOAD_EXPAND;
						nIpsMemExpLen[EXP_FLAG] = 1;
						if ((nNewValue = GetIpsDefineExpValue(tmp)) > nIpsMemExpLen[LOAD_ROM])
							nIpsMemExpLen[LOAD_ROM] = nNewValue;
						continue;
					}
					if (0 == strcmp(tmp, "IPS_EXTROM_INCL")) {
						nIpsDrvDefine |= IPS_EXTROM_INCL;
						if ((nNewValue = GetIpsDefineExpValue(tmp)) > nIpsMemExpLen[EXTR_ROM])
							nIpsMemExpLen[EXTR_ROM] = nNewValue;
						continue;
					}
					if (0 == strcmp(tmp, "IPS_PRG1_EXPAND")) {
						nIpsDrvDefine |= IPS_PRG1_EXPAND;
						if ((nNewValue = GetIpsDefineExpValue(tmp)) > nIpsMemExpLen[PRG1_ROM])
							nIpsMemExpLen[PRG1_ROM] = nNewValue;
						continue;
					}
					if (0 == strcmp(tmp, "IPS_PRG2_EXPAND")) {
						nIpsDrvDefine |= IPS_PRG2_EXPAND;
						if ((nNewValue = GetIpsDefineExpValue(tmp)) > nIpsMemExpLen[PRG2_ROM])
							nIpsMemExpLen[PRG2_ROM] = nNewValue;
						continue;
					}
					if (0 == strcmp(tmp, "IPS_GRA1_EXPAND")) {
						nIpsDrvDefine |= IPS_GRA1_EXPAND;
						if ((nNewValue = GetIpsDefineExpValue(tmp)) > nIpsMemExpLen[GRA1_ROM])
							nIpsMemExpLen[GRA1_ROM] = nNewValue;
						continue;
					}
					if (0 == strcmp(tmp, "IPS_GRA2_EXPAND")) {
						nIpsDrvDefine |= IPS_GRA2_EXPAND;
						if ((nNewValue = GetIpsDefineExpValue(tmp)) > nIpsMemExpLen[GRA2_ROM])
							nIpsMemExpLen[GRA2_ROM] = nNewValue;
						continue;
					}
					if (0 == strcmp(tmp, "IPS_GRA3_EXPAND")) {
						nIpsDrvDefine |= IPS_GRA3_EXPAND;
						if ((nNewValue = GetIpsDefineExpValue(tmp)) > nIpsMemExpLen[GRA3_ROM])
							nIpsMemExpLen[GRA3_ROM] = nNewValue;
						continue;
					}
					if (0 == strcmp(tmp, "IPS_ACPU_EXPAND")) {
						nIpsDrvDefine |= IPS_ACPU_EXPAND;
						if ((nNewValue = GetIpsDefineExpValue(tmp)) > nIpsMemExpLen[ACPU_ROM])
							nIpsMemExpLen[ACPU_ROM] = nNewValue;
						continue;
					}
					if (0 == strcmp(tmp, "IPS_SND1_EXPAND")) {
						nIpsDrvDefine |= IPS_SND1_EXPAND;
						if ((nNewValue = GetIpsDefineExpValue(tmp)) > nIpsMemExpLen[SND1_ROM])
							nIpsMemExpLen[SND1_ROM] = nNewValue;
						continue;
					}
					if (0 == strcmp(tmp, "IPS_SND2_EXPAND")) {
						nIpsDrvDefine |= IPS_SND2_EXPAND;
						if ((nNewValue = GetIpsDefineExpValue(tmp)) > nIpsMemExpLen[SND2_ROM])
							nIpsMemExpLen[SND2_ROM] = nNewValue;
						continue;
					}
				}
			}

			fclose(fp);
		}
	}
	nStandalone = nDefineNum = 0;
}

void IpsApplyPatches(UINT8* base, char* rom_name, UINT32 crc, bool readonly)
{
	if (!bDoIpsPatch)
		return;

	INT32 nCount         = GetIpsNumActivePatches();
	INT32 nActivePatches = (0 == nCount) ? 1 : nCount;

	// Two concurrent #define DAT files are not allowed.
	nStandalone = nDefineNum = 0;
	for (INT32 i = 0; i < nActivePatches; i++)
	{
		char ips_data[MAX_PATH] = { 0 };
		strcpy(ips_data, (0 == nCount) ? szAppIpsPath : pszIpsActivePatches[i]);
		if (nStandalone > 0)
		{
			nDefineNum  = 1;
			nStandalone = 0;
		}
		DoPatchGame(ips_data, rom_name, crc, base, readonly);
	}
	nStandalone = nDefineNum = 0;
}

void IpsPatchInit()
{
	bDoIpsPatch = true;
	GetIpsDrvDefine();
}

void IpsPatchExit()
{
	INT32 nCount = GetIpsNumActivePatches();

	if (nCount > 0)
	{
		for (INT32 i = 0; i < nCount; i++)
		{
			free(pszIpsActivePatches[i]);
			pszIpsActivePatches[i] = NULL;
		}
		free(pszIpsActivePatches);
		pszIpsActivePatches = NULL;
	}

	memset(nIpsMemExpLen, 0, sizeof(nIpsMemExpLen));

	nIpsDrvDefine = 0;
	nActiveArray  = 0;
	bDoIpsPatch   = false;
}
