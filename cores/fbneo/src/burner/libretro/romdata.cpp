#include "burner.h"
#include "retro_common.h"
#include "retro_dirent.h"

#include <time.h>
#include <file/file_path.h>

#ifndef UINT32_MAX
#define UINT32_MAX	(UINT32)4294967295U
#endif

static TCHAR szRomset[MAX_PATH] = _T("");
static struct RomDataInfo RDI = { 0 };
RomDataInfo* pRDI = &RDI;

struct BurnRomInfo* pDataRomDesc = NULL;

TCHAR szRomdataName[MAX_PATH] = _T("");
static TCHAR szFullName[MAX_PATH] = { 0 }, szAltName[MAX_PATH] = { 0 }, szAltDesc[MAX_PATH] = { 0 };

std::vector<romdata_core_option> romdata_core_options;

static TCHAR CoreRomDataPaths[DIRS_MAX][MAX_PATH];

// Read in the config file for the whole application
INT32 CoreRomDataPathsLoad()
{
	TCHAR szConfig[MAX_PATH] = { 0 }, szLine[1024] = { 0 };
	FILE* h = NULL;

#ifdef _UNICODE
	setlocale(LC_ALL, "");
#endif

	for (INT32 i = 0; i < DIRS_MAX; i++)
		memset(CoreRomDataPaths[i], 0, MAX_PATH * sizeof(TCHAR));

	char* p = find_last_slash(szAppRomdatasPath);				// g_system_dir/fbneo/romdata/
	if ((NULL != p) && ('\0' == p[1])) p[0] = '\0';				// g_system_dir/fbneo/romdata

	strcpy(CoreRomDataPaths[0], szAppRomdatasPath);				// CoreRomDataPaths[0] = g_system_dir/fbneo/romdata

	snprintf(
		szConfig, MAX_PATH - 1, "%sromdata_path.opt",
		szAppPathDefPath
	);															// g_system_dir/fbneo/path/romdata_path.opt

	if (NULL == (h = _tfopen(szConfig, _T("rt"))))
	{
		memset(szConfig, 0, MAX_PATH * sizeof(TCHAR));
		snprintf(
			szConfig, MAX_PATH - 1, "%s%cromdata_path.opt",
			g_rom_dir, PATH_DEFAULT_SLASH_C()
		);														// g_rom_dir/romdata_path.opt

		if (NULL == (h = fopen(szConfig, "rt")))
			return 1;											// Only CoreRomDataPaths[0]
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

//		STR(CoreRomDataPaths[0]);								// g_system_dir/fbneo/romdata
		STR(CoreRomDataPaths[1]);
		STR(CoreRomDataPaths[2]);
		STR(CoreRomDataPaths[3]);
		STR(CoreRomDataPaths[4]);
		STR(CoreRomDataPaths[5]);
		STR(CoreRomDataPaths[6]);
		STR(CoreRomDataPaths[7]);
		STR(CoreRomDataPaths[8]);
		STR(CoreRomDataPaths[9]);
		STR(CoreRomDataPaths[10]);
		STR(CoreRomDataPaths[11]);
		STR(CoreRomDataPaths[12]);
		STR(CoreRomDataPaths[13]);
		STR(CoreRomDataPaths[14]);
		STR(CoreRomDataPaths[15]);
		STR(CoreRomDataPaths[16]);
		STR(CoreRomDataPaths[17]);
		STR(CoreRomDataPaths[18]);
		STR(CoreRomDataPaths[19]);
#undef STR
	}

	fclose(h);
	return 0;													// There may be more
}

static INT32 IsUTF8Text(const void* pBuffer, long size)
{
	INT32 nCode = 0;
	unsigned char* start = (unsigned char*)pBuffer;
	unsigned char* end   = (unsigned char*)pBuffer + size;

	while (start < end) {
		if (*start < 0x80) {        // (10000000) ASCII
			if (0 == nCode) nCode = 1;

			start++;
		} else if (*start < 0xc0) { // (11000000) Invalid UTF-8
			return 0;
		} else if (*start < 0xe0) { // (11100000) 2-byte UTF-8
			if (nCode < 2) nCode = 2;
			if (start >= end - 1) break;
			if (0x80 != (start[1] & 0xc0)) return 0;

			start += 2;
		} else if (*start < 0xf0) { // (11110000) 3-byte UTF-8
			if (nCode < 3) nCode = 3;
			if (start >= end - 2) break;
			if ((0x80 != (start[1] & 0xc0)) || (0x80 != (start[2] & 0xc0))) return 0;

			start += 3;
		} else {
			return 0;
		}
	}

	return nCode;
}

static INT32 IsDatUTF8BOM()
{
	FILE* fp = _tfopen(szRomdataName, _T("rb"));
	if (NULL == fp) return -1;

	// get dat size
	fseek(fp, 0, SEEK_END);
	INT32 nDatSize = ftell(fp);
	fseek(fp, 0, SEEK_SET);

	INT32 nRet = 0;
	char* pszTest = (char*)malloc(nDatSize + 1);

	if (NULL != pszTest) {
		memset(pszTest, 0, nDatSize + 1);
		fread(pszTest, nDatSize, 1, fp);
		nRet = IsUTF8Text(pszTest, nDatSize);
		free(pszTest);
		pszTest = NULL;
	}
	fclose(fp);

	return nRet;
}

#define DELIM_TOKENS_NAME	_T(" \t\r\n,%:|{}")
#define _strqtoken	strqtoken
#define _tcscmp		strcmp
#define _stscanf	sscanf

static INT32 LoadRomdata()
{
	RDI.nDescCount = -1;	// Failed

	INT32 nType = IsDatUTF8BOM();
	if (-1 == nType) return RDI.nDescCount;

	const TCHAR* szReadMode = (3 == nType) ? _T("rt, ccs=UTF-8") : _T("rt");
	FILE* fp = _tfopen(szRomdataName, szReadMode);
	if (NULL == fp) return RDI.nDescCount;

	TCHAR szBuf[MAX_PATH] = { 0 }, * pszBuf = NULL, * pszLabel = NULL, * pszInfo = NULL;

	memset(RDI.szExtraRom, 0, sizeof(RDI.szExtraRom));
	memset(szFullName,     0, sizeof(szFullName));

	while (!feof(fp)) {
		if (_fgetts(szBuf, MAX_PATH, fp) != NULL) {
			pszBuf   = szBuf;
			pszLabel = _strqtoken(pszBuf, DELIM_TOKENS_NAME);

			if (NULL == pszLabel) continue;
			if ((_T('#') == pszLabel[0]) || ((_T('/') == pszLabel[0]) && (_T('/') == pszLabel[1]))) continue;
			if (0 == _tcsicmp(_T("ZipName"), pszLabel)) {
				pszInfo = _strqtoken(NULL, DELIM_TOKENS_NAME);
				if (NULL == pszInfo) break;	// No romset specified
				if (NULL != pDataRomDesc)
					strcpy(RDI.szZipName, TCHARToANSI(pszInfo, NULL, 0));
			}
			if (0 == _tcsicmp(_T("DrvName"), pszLabel)) {
				pszInfo = _strqtoken(NULL, DELIM_TOKENS_NAME);
				if (NULL == pszInfo) break;	// No driver specified
				if (NULL != pDataRomDesc)
					strcpy(RDI.szDrvName, TCHARToANSI(pszInfo, NULL, 0));
			}
			if (0 == _tcsicmp(_T("ExtraRom"), pszLabel)) {
				pszInfo = _strqtoken(NULL, DELIM_TOKENS_NAME);
				if ((NULL != pszInfo) && (NULL != pDataRomDesc))
					strcpy(RDI.szExtraRom, TCHARToANSI(pszInfo, NULL, 0));
			}
			if (0 == _tcsicmp(_T("FullName"), pszLabel)) {
				pszInfo = _strqtoken(NULL, DELIM_TOKENS_NAME);
				if ((NULL != pszInfo) && (NULL != pDataRomDesc))
					strcpy(szFullName, TCHARToANSI(pszInfo, NULL, 0));
			}

			{
				// romname, len, crc, type
				struct BurnRomInfo ri = { 0 };
				ri.nLen  = UINT32_MAX;
				ri.nCrc  = UINT32_MAX;
				ri.nType = 0;

				pszInfo = _strqtoken(NULL, DELIM_TOKENS_NAME);
				if (NULL != pszInfo) {
					_stscanf(pszInfo, _T("%x"), &(ri.nLen));
					if (UINT32_MAX == ri.nLen) continue;

					pszInfo = _strqtoken(NULL, DELIM_TOKENS_NAME);
					if (NULL != pszInfo) {
						_stscanf(pszInfo, _T("%x"), &(ri.nCrc));
						if (UINT32_MAX == ri.nCrc) continue;

						while (NULL != (pszInfo = _strqtoken(NULL, DELIM_TOKENS_NAME))) {
							INT32 nValue = -1;

							if (0 == _tcscmp(pszInfo, _T("BRF_PRG"))) {
								ri.nType |= BRF_PRG;
								continue;
							}
							if (0 == _tcscmp(pszInfo, _T("BRF_GRA"))) {
								ri.nType |= BRF_GRA;
								continue;
							}
							if (0 == _tcscmp(pszInfo, _T("BRF_SND"))) {
								ri.nType |= BRF_SND;
								continue;
							}
							if (0 == _tcscmp(pszInfo, _T("BRF_ESS"))) {
								ri.nType |= BRF_ESS;
								continue;
							}
							if (0 == _tcscmp(pszInfo, _T("BRF_BIOS"))) {
								ri.nType |= BRF_BIOS;
								continue;
							}
							if (0 == _tcscmp(pszInfo, _T("BRF_SELECT"))) {
								ri.nType |= BRF_SELECT;
								continue;
							}
							if (0 == _tcscmp(pszInfo, _T("BRF_OPT"))) {
								ri.nType |= BRF_OPT;
								continue;
							}
							if (0 == _tcscmp(pszInfo, _T("BRF_NODUMP"))) {
								ri.nType |= BRF_NODUMP;
								continue;
							}
							if ((0 == _tcscmp(pszInfo, _T("CPS2_PRG_68K"))) ||
								(0 == _tcscmp(pszInfo, _T("CPS1_68K_PROGRAM_BYTESWAP")))) {			//  1
								ri.nType |= 1;
								continue;
							}
							if ((0 == _tcscmp(pszInfo, _T("CPS2_PRG_68K_SIMM"))) ||
								(0 == _tcscmp(pszInfo, _T("CPS1_68K_PROGRAM_NO_BYTESWAP")))) {		//  2
								ri.nType |= 2;
								continue;
							}
							if ((0 == _tcscmp(pszInfo, _T("CPS2_PRG_68K_XOR_TABLE"))) ||
								(0 == _tcscmp(pszInfo, _T("CPS1_Z80_PROGRAM")))) {					//  3
								ri.nType |= 3;
								continue;
							}
							if (0 == _tcscmp(pszInfo, _T("CPS1_TILES"))) {							//  4
								ri.nType |= 4;
								continue;
							}
							if ((0 == _tcscmp(pszInfo, _T("CPS2_GFX"))) ||
								(0 == _tcscmp(pszInfo, _T("CPS1_OKIM6295_SAMPLES")))) {				//  5
								ri.nType |= 5;
								continue;
							}
							if ((0 == _tcscmp(pszInfo, _T("CPS2_GFX_SIMM"))) ||
								(0 == _tcscmp(pszInfo, _T("CPS1_QSOUND_SAMPLES")))) {				//  6
								ri.nType |= 6;
								continue;
							}
							if ((0 == _tcscmp(pszInfo, _T("CPS2_GFX_SPLIT4"))) ||
								(0 == _tcscmp(pszInfo, _T("CPS1_PIC")))) {							//  7
								ri.nType |= 7;
								continue;
							}
							if ((0 == _tcscmp(pszInfo, _T("CPS2_GFX_SPLIT8"))) ||
								(0 == _tcscmp(pszInfo, _T("CPS1_EXTRA_TILES_SF2EBBL_400000")))) {	//  8
								ri.nType |= 8;
								continue;
							}
							if ((0 == _tcscmp(pszInfo, _T("CPS2_GFX_19XXJ"))) ||
								(0 == _tcscmp(pszInfo, _T("CPS1_EXTRA_TILES_400000")))) {			//  9
								ri.nType |= 9;
								continue;
							}
							if ((0 == _tcscmp(pszInfo, _T("CPS2_PRG_Z80"))) ||
								(0 == _tcscmp(pszInfo, _T("CPS1_EXTRA_TILES_SF2KORYU_400000")))) {	// 10
								ri.nType |= 10;
								continue;
							}
							if (0 == _tcscmp(pszInfo, _T("CPS1_EXTRA_TILES_SF2B_400000"))) {		// 11
								ri.nType |= 11;
								continue;
							}
							if ((0 == _tcscmp(pszInfo, _T("CPS2_QSND"))) ||
								(0 == _tcscmp(pszInfo, _T("CPS1_EXTRA_TILES_SF2MKOT_400000")))) {	// 12
								ri.nType |= 12;
								continue;
							}
							if (0 == _tcscmp(pszInfo, _T("CPS2_QSND_SIMM"))) {						// 13
								ri.nType |= 13;
								continue;
							}
							if (0 == _tcscmp(pszInfo, _T("CPS2_QSND_SIMM_BYTESWAP"))) {				// 14
								ri.nType |= 14;
								continue;
							}
							if (0 == _tcscmp(pszInfo, _T("CPS2_ENCRYPTION_KEY"))) {					// 15
								ri.nType |= 15;
								continue;
							}
							// megadrive
							if (0 == _tcscmp(pszInfo, _T("SEGA_MD_ROM_LOAD_NORMAL"))) {
								ri.nType |= 0x10;
								continue;
							}
							if (0 == _tcscmp(pszInfo, _T("SEGA_MD_ROM_LOAD16_WORD_SWAP"))) {
								ri.nType |= 0x20;
								continue;
							}
							if (0 == _tcscmp(pszInfo, _T("SEGA_MD_ROM_LOAD16_BYTE"))) {
								ri.nType |= 0x30;
								continue;
							}
							if (0 == _tcscmp(pszInfo, _T("SEGA_MD_ROM_LOAD16_WORD_SWAP_CONTINUE_040000_100000"))) {
								ri.nType |= 0x40;
								continue;
							}
							if (0 == _tcscmp(pszInfo, _T("SEGA_MD_ROM_LOAD_NORMAL_CONTINUE_020000_080000"))) {
								ri.nType |= 0x50;
								continue;
							}
							if (0 == _tcscmp(pszInfo, _T("SEGA_MD_ROM_OFFS_000000"))) {
								ri.nType |= 0x01;
								continue;
							}
							if (0 == _tcscmp(pszInfo, _T("SEGA_MD_ROM_OFFS_000001"))) {
								ri.nType |= 0x02;
								continue;
							}
							if (0 == _tcscmp(pszInfo, _T("SEGA_MD_ROM_OFFS_020000"))) {
								ri.nType |= 0x03;
								continue;
							}
							if (0 == _tcscmp(pszInfo, _T("SEGA_MD_ROM_OFFS_080000"))) {
								ri.nType |= 0x04;
								continue;
							}
							if (0 == _tcscmp(pszInfo, _T("SEGA_MD_ROM_OFFS_100000"))) {
								ri.nType |= 0x05;
								continue;
							}
							if (0 == _tcscmp(pszInfo, _T("SEGA_MD_ROM_OFFS_100001"))) {
								ri.nType |= 0x06;
								continue;
							}
							if (0 == _tcscmp(pszInfo, _T("SEGA_MD_ROM_OFFS_200000"))) {
								ri.nType |= 0x07;
								continue;
							}
							if (0 == _tcscmp(pszInfo, _T("SEGA_MD_ROM_OFFS_300000"))) {
								ri.nType |= 0x08;
								continue;
							}
							if (0 == _tcscmp(pszInfo, _T("SEGA_MD_ROM_RELOAD_200000_200000"))) {
								ri.nType |= 0x09;
								continue;
							}
							if (0 == _tcscmp(pszInfo, _T("SEGA_MD_ROM_RELOAD_100000_300000"))) {
								ri.nType |= 0x0a;
								continue;
							}
							_stscanf(pszInfo, _T("%x"), &nValue);
							if (-1 != nValue) {
								ri.nType |= (UINT32)nValue;
								continue;
							}
						}

						if (ri.nType > 0) {
							RDI.nDescCount++;

							if (NULL != pDataRomDesc) {
								pDataRomDesc[RDI.nDescCount].szName = (char*)malloc(512);

								strcpy(pDataRomDesc[RDI.nDescCount].szName, TCHARToANSI(pszLabel, NULL, 0));

								pDataRomDesc[RDI.nDescCount].nLen  = ri.nLen;
								pDataRomDesc[RDI.nDescCount].nCrc  = ri.nCrc;
								pDataRomDesc[RDI.nDescCount].nType = ri.nType;
							}
						}
					}
				}
			}
		}
	}
	fclose(fp);

	return RDI.nDescCount;
}

char* RomdataGetDrvName()
{
	INT32 nType = IsDatUTF8BOM();
	if (-1 == nType) return NULL;

	const TCHAR* szReadMode = (3 == nType) ? _T("rt, ccs=UTF-8") : _T("rt");

	FILE* fp = _tfopen(szRomdataName, szReadMode);
	if (NULL == fp) return NULL;

	memset(szRomset, 0, MAX_PATH * sizeof(TCHAR));

	TCHAR szBuf[MAX_PATH] = { 0 }, * pszBuf = NULL, * pszLabel = NULL, * pszInfo = NULL;

	while (!feof(fp)) {
		if (_fgetts(szBuf, MAX_PATH, fp) != NULL) {
			pszBuf = szBuf;

			pszLabel = _strqtoken(pszBuf, DELIM_TOKENS_NAME);
			if (NULL == pszLabel) continue;
			if ((_T('#') == pszLabel[0]) || ((_T('/') == pszLabel[0]) && (_T('/') == pszLabel[1]))) continue;

			if (0 == _tcsicmp(_T("DrvName"), pszLabel)) {
				pszInfo = _strqtoken(NULL, DELIM_TOKENS_NAME);
				if (NULL == pszInfo) break;	// No driver specified

				fclose(fp);
				_tcscpy(szRomset, TCHARToANSI(pszInfo, NULL, 0));

				return szRomset;
			}
		}
	}
	fclose(fp);

	return NULL;
}

INT32 create_variables_from_romdatas()
{
	INT32 nRet = 0;

	bool bRomdataMode = (NULL != pDataRomDesc);
	const char* pszDrvName = bRomdataMode ? RDI.szDrvName : BurnDrvGetTextA(DRV_NAME), * pszExt = ".dat";
	if (NULL == pszDrvName) return -2;

	romdata_core_options.clear();
	CoreRomDataPathsLoad();

	for (INT32 i = 0; i < DIRS_MAX; i++)
	{
		if (0 == _tcscmp("", CoreRomDataPaths[i])) continue;

		char* p = find_last_slash(CoreRomDataPaths[i]);
		if ((NULL != p) && ('\0' == p[1])) p[0] = '\0';

		TCHAR szFilePathSearch[MAX_PATH] = { 0 }, szDatPaths[MAX_PATH] = { 0 };
		snprintf(szFilePathSearch, MAX_PATH - 1, "%s%c", CoreRomDataPaths[i], PATH_DEFAULT_SLASH_C());

		// romdata_dirs/
		strcpy(szDatPaths, szFilePathSearch);

		struct RDIR* entry = retro_opendir_include_hidden(szFilePathSearch, true);
		if (!entry || retro_dirent_error(entry)) continue;

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
			snprintf(szFilePathSearch, MAX_PATH - 1, "%s%s", szDatPaths, name);

			fp = fopen(szFilePathSearch, "rb");
			if (NULL == fp) continue;

			// get dat size
			fseek(fp, 0, SEEK_END);
			INT32 nDatSize = ftell(fp);
			fseek(fp, 0, SEEK_SET);

			char* pszTest = (char*)malloc(nDatSize + 1);
			if (NULL == pszTest)
			{
				fclose(fp);
				continue;
			}

			memset(pszTest, 0, nDatSize + 1);
			fread(pszTest, nDatSize, 1, fp);
			INT32 nType = IsUTF8Text(pszTest, nDatSize);
			free(pszTest);
			pszTest = NULL;
			fclose(fp);

			const TCHAR* szReadMode = (3 == nType) ? _T("rt, ccs=UTF-8") : _T("rt");
			if (NULL == (fp = _tfopen(szFilePathSearch, szReadMode))) continue;

			TCHAR szBuf[MAX_PATH] = { 0 }, * pszBuf = NULL, * pszLabel = NULL, * pszInfo = NULL;
			bool bDrvOK = false, bDescOK = false;

			memset(szAltName, 0, MAX_PATH * sizeof(TCHAR));
			memset(szAltDesc, 0, MAX_PATH * sizeof(TCHAR));

			while (!feof(fp))
			{
				if (NULL != _fgetts(szBuf, MAX_PATH, fp))
				{
					pszBuf = szBuf;
					pszLabel = _strqtoken(pszBuf, DELIM_TOKENS_NAME);
					if ((NULL == pszLabel) || (_T('#') == pszLabel[0]) || ((_T('/') == pszLabel[0]) && (_T('/') == pszLabel[1]))) continue;

					if (0 == _tcsicmp(_T("DrvName"), pszLabel))
					{
						pszInfo = _strqtoken(NULL, DELIM_TOKENS_NAME);
						if ((NULL == pszInfo) || (0 != _tcsicmp(pszDrvName, pszInfo))) break;	// The driver specified by romdata does not match the current driver.
						_tcscpy(szAltName, pszInfo); bDrvOK = true;
					}
					if (0 == _tcsicmp(_T("FullName"), pszLabel))
					{
						const TCHAR* pDesc = (NULL == (pszInfo = _strqtoken(NULL, DELIM_TOKENS_NAME))) ? szFilePathSearch : pszInfo;
						_tcscpy(szAltDesc, pDesc); bDescOK = true;
					}
					if (bDrvOK && bDescOK)
					{
						char szKey[100] = { 0 };
						snprintf(szKey, 100 - 1, "fbneo-romdata-%s-%d", pszDrvName, nRet);

						romdata_core_options.push_back(romdata_core_option());
						romdata_core_option* romdata_option = &romdata_core_options.back();
						romdata_option->dat_path            = szFilePathSearch;
						romdata_option->option_name         = szKey;
						romdata_option->friendly_name       = szAltDesc;

						nRet++; break;
					}
				}
			}
			fclose(fp);
		}
		retro_closedir(entry);
	}

	return nRet;
}

#undef _stscanf
#undef _tcscmp
#undef _strqtoken
#undef DELIM_TOKENS_NAME

INT32 reset_romdatas_from_variables()
{
	struct retro_variable var = { 0 };

	for (INT32 romdata_idx = 0; romdata_idx < romdata_core_options.size(); romdata_idx++)
	{
		romdata_core_option* romdata_option = &romdata_core_options[romdata_idx];

		var.key = romdata_option->option_name.c_str();
		if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) == false || !var.value)
			continue;

		var.value = "disabled";

		if (environ_cb(RETRO_ENVIRONMENT_SET_VARIABLE, &var) == false)
			return -1;
	}

	return 0;
}

INT32 apply_romdatas_from_variables()
{
	if (!bPatchedRomsetsEnabled) {
		reset_romdatas_from_variables();
		return -2;
	}

	INT32 nIndex = 0, nCount = 0;
	struct retro_variable var = { 0 };

	// Calculate the number of selections.
	for (INT32 romdata_idx = 0; romdata_idx < romdata_core_options.size(); romdata_idx++)
	{
		romdata_core_option* romdata_option = &romdata_core_options[romdata_idx];
		var.key = romdata_option->option_name.c_str();

		if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) == false || !var.value)
			continue;

		if (0 == strcmp(var.value, "enabled")) nCount++;
	}

	// If nCount is 0, no romdata is enabled and we should return -1
	if (nCount == 0)
		return -1;

	// Generates a random value when multiple selections are made.
	if (nCount > 1)
		nIndex = rand() % nCount;

	nCount = 0;

	// Selects a romdata by default/random value.
	for (INT32 romdata_idx = 0; romdata_idx < romdata_core_options.size(); romdata_idx++)
	{
		romdata_core_option* romdata_option = &romdata_core_options[romdata_idx];
		var.key = romdata_option->option_name.c_str();

		if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) == false || !var.value)
			continue;

		if (0 == strcmp(var.value, "enabled"))
		{
			if (nIndex != nCount++) continue;

			RomDataExit();	// Reset the parasitized drvname.
			_tcscpy(szRomdataName, romdata_option->dat_path.c_str());
		}
	}

	// Reset
	for (INT32 romdata_idx = 0; romdata_idx < romdata_core_options.size(); romdata_idx++)
	{
		romdata_core_option* romdata_option = &romdata_core_options[romdata_idx];
		var.key = romdata_option->option_name.c_str();

		if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) == false || !var.value)
			continue;

		if (0 == strcmp(var.value, "enabled"))
		{
			var.value = "disabled";

			if (environ_cb(RETRO_ENVIRONMENT_SET_VARIABLE, &var) == false)
				return -1;
		}
	}

	return nIndex;
}

void RomDataInit()
{
	INT32 nLen = LoadRomdata();

	if ((-1 != nLen) && (NULL == pDataRomDesc)) {
		pDataRomDesc = (struct BurnRomInfo*)malloc((nLen + 1) * sizeof(BurnRomInfo));
		if (NULL != pDataRomDesc) {
			LoadRomdata();

			RDI.nDriverId = BurnDrvGetIndex(RDI.szDrvName);

			if (-1 != RDI.nDriverId) {
				BurnDrvSetZipName(RDI.szZipName, RDI.nDriverId);
			}
		}
	}
}

void RomDataSetFullName()
{
#if 0
	// At this point, the driver's default ZipName has been replaced with the ZipName from the rom data
	RDI.nDriverId = BurnDrvGetIndex(RDI.szZipName);

	if (-1 != RDI.nDriverId) {
		wchar_t* szOldName = BurnDrvGetFullNameW(RDI.nDriverId);
		memset(RDI.szOldName, '\0', sizeof(RDI.szOldName));

		if (0 != wcscmp(szOldName, RDI.szOldName)) {
			wcscpy(RDI.szOldName, szOldName);
		}

		BurnDrvSetFullNameW(RDI.szFullName, RDI.nDriverId);
	}
#endif
}

void RomDataExit()
{
	if (NULL != pDataRomDesc) {

		for (int i = 0; i < RDI.nDescCount + 1; i++) {
			free(pDataRomDesc[i].szName);
		}

		free(pDataRomDesc);
		pDataRomDesc = NULL;

		if (-1 != RDI.nDriverId) {
			BurnDrvSetZipName(RDI.szDrvName, RDI.nDriverId);
#if 0
			if (0 != wcscmp(BurnDrvGetFullNameW(RDI.nDriverId), RDI.szOldName)) {
				BurnDrvSetFullNameW(RDI.szOldName, RDI.nDriverId);
			}
#endif
			RDI.nDriverId = -1;
		}

		memset(&RDI, 0, sizeof(RomDataInfo));
		memset(szRomdataName, 0, sizeof(szRomdataName));

		RDI.nDescCount = -1;
	}
}
