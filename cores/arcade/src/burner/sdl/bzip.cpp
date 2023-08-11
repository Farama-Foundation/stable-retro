// Burner Zip module
#include "burner.h"

int nBzipError = 0;                                                             // non-zero if there is a problem with the opened romset

static TCHAR* szBzipName[BZIP_MAX] = { NULL, };                                 // Zip files to search through

struct RomFind { int nState; int nZip; int nPos; };                             // State is non-zero if found. 1 = found totally okay.
static struct RomFind* RomFind = NULL;
static int              nRomCount = 0; static int nTotalSize = 0;
static struct ZipEntry* List = NULL; static int nListCount = 0; // List of entries for current zip file
static int              nCurrentZip = -1;                       // Zip which is currently open


StringSet BzipText;                                                                                     // Text which describes any problems with loading the zip
StringSet BzipDetail;                                                                                   // Text which describes in detail any problems with loading the zip

static void BzipListFree()
{
	if (List)
	{
		for (int i = 0; i < nListCount; i++)
		{
			if (List[i].szName)
			{
				free(List[i].szName);
				List[i].szName = NULL;
			}
		}
		free(List);
		List = NULL;
	}

	nListCount = 0;
}

static char* GetFilenameA(char* szFull)
{
	int nLen = strlen(szFull);

	if (nLen <= 0)
	{
		return szFull;
	}
	for (int i = nLen - 1; i >= 0; i--)
	{
		if (szFull[i] == '\\' || szFull[i] == '/')
		{
			return szFull + i + 1;
		}
	}

	return szFull;
}

static TCHAR* GetFilenameW(TCHAR* szFull)
{
	int nLen = _tcslen(szFull);

	if (nLen <= 0)
	{
		return szFull;
	}
	for (int i = nLen - 1; i >= 0; i--)
	{
		if (szFull[i] == _T('\\') || szFull[i] == _T('/'))
		{
			return szFull + i + 1;
		}
	}

	return szFull;
}

static int FileExists(TCHAR* szName)
{
	FILE* file = fopen(szName, "r");

	if (file)
	{
		fclose(file);
		return 1;
	}
	return 0;
}

static int RomArchiveExists(TCHAR* szName)
{
	TCHAR szFileName[MAX_PATH];
	int   ret = 0;

	_stprintf(szFileName, _T("%s.zip"), szName);
	ret = FileExists(szFileName);

	if (ret)
	{
		return ret;
	}

#ifdef INCLUDE_7Z_SUPPORT
	_stprintf(szFileName, _T("%s.7z"), szName);
	ret = FileExists(szFileName);
#endif

	return ret;
}

static int FindRomByName(TCHAR* szName)
{
	struct ZipEntry* pl;
	int i;

	// Find the rom named szName in the List
	for (i = 0, pl = List; i < nListCount; i++, pl++)
	{
		TCHAR szCurrentName[MAX_PATH];
		if (pl->szName)
		{
			if (_tcsicmp(szName, GetFilenameW(ANSIToTCHAR(pl->szName, szCurrentName, MAX_PATH))) == 0)
			{
				return i;
			}
		}
	}
	return -1;                                                                                                           // couldn't find the rom
}

static int FindRomByCrc(unsigned int nCrc)
{
	struct ZipEntry* pl;
	int i;

	// Find the rom named szName in the List
	for (i = 0, pl = List; i < nListCount; i++, pl++)
	{
		if (nCrc == pl->nCrc)
		{
			return i;
		}
	}

	return -1;                                                                                                           // couldn't find the rom
}

// Find rom number i from the pBzipDriver game
static int FindRom(int i)
{
	struct BurnRomInfo ri;
	int nRet;

	memset(&ri, 0, sizeof(ri));

	nRet = BurnDrvGetRomInfo(&ri, i);
	if (nRet != 0)                                                                                               // Failure: no such rom
	{
		return -2;
	}

	if (ri.nCrc)                                                                                                 // Search by crc first
	{
		nRet = FindRomByCrc(ri.nCrc);
		if (nRet >= 0)
		{
			return nRet;
		}
	}

	for (int nAka = 0; nAka < 0x10000; nAka++)                                   // Failing that, search for possible names
	{
		char* szPossibleName = NULL;

		nRet = BurnDrvGetRomName(&szPossibleName, i, nAka);
		if (nRet)                                                                                                         // No more rom names
		{
			break;
		}
		nRet = FindRomByName(ANSIToTCHAR(szPossibleName, NULL, 0));
		if (nRet >= 0)
		{
			return nRet;
		}
	}

	return -1;                                                                                                           // Couldn't find the rom
}

static int RomDescribe(StringSet* pss, struct BurnRomInfo* pri)
{
	if (nBzipError == 0)
	{
		nBzipError |= 0x8000;

		pss->Add(_T("Invalid zip..."));
	}
	pss->Add(_T("The "));
	if (pri->nType & BRF_ESS)
	{
		pss->Add(_T("essential "));
	}
	if (pri->nType & BRF_BIOS)
	{
		pss->Add(_T("BIOS "));
	}
	if (pri->nType & BRF_PRG)
	{
		pss->Add(_T("program "));
	}
	if (pri->nType & BRF_GRA)
	{
		pss->Add(_T("graphics "));
	}
	if (pri->nType & BRF_SND)
	{
		pss->Add(_T("sound "));
	}
	pss->Add(_T("ROM "));

	return 0;
}

static int CheckRomsBoot()
{
	int nRet = 0;

	for (int i = 0; i < nRomCount; i++)
	{
		struct BurnRomInfo ri;
		int nState;

		memset(&ri, 0, sizeof(ri));
		BurnDrvGetRomInfo(&ri, i);                                // Find information about the wanted rom
		nState = RomFind[i].nState;                               // Get the state of the rom in the zip file

		if (nState != 1 && ri.nType && ri.nCrc)
		{
			if (!(ri.nType & BRF_OPT) && !(ri.nType & BRF_NODUMP))
			{
				return 1;
			}
			nRet = 2;
		}
	}

	return nRet;
}

static int GetBZipError(int nState)
{
	switch (nState)
	{
	case 1:                                                                      // OK
		return 0x0000;

	case 0:                                                                      // Not present
		return 0x0001;

	case 3:                                                                      // Incomplete
		return 0x0001;

	default:                                                                     // CRC wrong or too large
		return 0x0100;
	}

	return 0x0100;
}

// Check the roms to see if they code, graphics etc are complete
static int CheckRoms()
{
	for (int i = 0; i < nRomCount; i++)
	{
		struct BurnRomInfo ri;

		memset(&ri, 0, sizeof(ri));
		BurnDrvGetRomInfo(&ri, i);                                                                // Find information about the wanted rom
		if (/*ri.nCrc &&*/ (ri.nType & BRF_OPT) == 0 && (ri.nType & BRF_NODUMP) == 0)
		{
			int nState = RomFind[i].nState;                                                // Get the state of the rom in the zip file
			int nError = GetBZipError(nState);

			if (nState == 0 && ri.nType)                                                   // (A type of 0 means empty slot - no rom)
			{
				char* szName = "Unknown";
				RomDescribe(&BzipDetail, &ri);
				BurnDrvGetRomName(&szName, i, 0);
				BzipDetail.Add(_T("%hs was not found.\n"), szName);
			}

			if (nError == 0)
			{
				nBzipError |= 0x2000;
			}

			if (ri.nType & BRF_ESS)                                                                // essential rom - without it the game may not run at all
			{
				nBzipError |= nError << 0;
			}
			if (ri.nType & BRF_PRG)                                                                // rom which contains program information
			{
				nBzipError |= nError << 1;
			}
			if (ri.nType & BRF_GRA)                                                                // rom which contains graphics information
			{
				nBzipError |= nError << 2;
			}
			if (ri.nType & BRF_SND)                                                                // rom which contains sound information
			{
				nBzipError |= nError << 3;
			}
		}
	}

	if (nBzipError & 0x0F0F)
	{
		nBzipError |= 0x4000;
	}

	return 0;
}

// ----------------------------------------------------------------------------

static int __cdecl BzipBurnLoadRom(unsigned char* Dest, int* pnWrote, int i)
{
#if defined (BUILD_WIN32)
	MSG Msg;
#endif

	struct BurnRomInfo ri;
	int   nWantZip = 0;
	TCHAR szText[128];
	char* pszRomName = NULL;
	int   nRet = 0;

	if (i < 0 || i >= nRomCount)
	{
		return 1;
	}

	ri.nLen = 0;
	BurnDrvGetRomInfo(&ri, i);                                                                   // Get info

	// show what we're doing
	BurnDrvGetRomName(&pszRomName, i, 0);
	if (pszRomName == NULL)
	{
		pszRomName = "unknown";
	}
	_stprintf(szText, _T("Loading"));
	if (ri.nType & (BRF_PRG | BRF_GRA | BRF_SND | BRF_BIOS))
	{
		if (ri.nType & BRF_BIOS)
		{
			_stprintf(szText + _tcslen(szText), _T(" %s"), _T("BIOS "));
		}
		if (ri.nType & BRF_PRG)
		{
			_stprintf(szText + _tcslen(szText), _T(" %s"), _T("program "));
		}
		if (ri.nType & BRF_GRA)
		{
			_stprintf(szText + _tcslen(szText), _T(" %s"), _T("graphics "));
		}
		if (ri.nType & BRF_SND)
		{
			_stprintf(szText + _tcslen(szText), _T(" %s"), _T("sound "));
		}
		_stprintf(szText + _tcslen(szText), _T("(%hs)..."), pszRomName);
	}
	else
	{
		_stprintf(szText + _tcslen(szText), _T(" %hs..."), pszRomName);
	}
	ProgressUpdateBurner(ri.nLen ? 1.0 / ((double)nTotalSize / ri.nLen) : 0, szText, 0);

#if defined (BUILD_WIN32)
	// Check for messages:
	while (PeekMessage(&Msg, NULL, 0, 0, PM_REMOVE))
	{
		DispatchMessage(&Msg);
	}
#endif

	if (RomFind[i].nState == 0)                                                          // Rom not found in zip at all
	{
		TCHAR szTemp[128] = _T("");
		_stprintf(szTemp, "%s (not found)\n", szText);
		fprintf(stderr, szTemp);
		AppError(szTemp, 1);
		return 1;
	}

	nWantZip = RomFind[i].nZip;                                                          // Which zip file it is in
	if (nCurrentZip != nWantZip)                                                         // If we haven't got the right zip file currently open
	{
		ZipClose();
		nCurrentZip = -1;
		if (ZipOpen(TCHARToANSI(szBzipName[nWantZip], NULL, 0)))
		{
			return 1;
		}
		nCurrentZip = nWantZip;
	}

	// Read in file and return how many bytes we read
	if (ZipLoadFile(Dest, ri.nLen, pnWrote, RomFind[i].nPos))
	{
		// Error loading from the zip file
		TCHAR szTemp[128] = _T("");
		_stprintf(szTemp, _T("%s reading %.30hs from %.30s"), nRet == 2 ? _T("CRC error") : _T("Error"), pszRomName, GetFilenameW(szBzipName[nCurrentZip]));
		fprintf(stderr, szTemp);
		AppError(szTemp, 1);
		return 1;
	}

	fprintf(stderr, "%s (OK)\n", szText);
	return 0;
}

// ----------------------------------------------------------------------------

int BzipStatus()
{
	if (!(nBzipError & 0x0F0F))
	{
		return BZIP_STATUS_OK;
	}
	if (nBzipError & 1)
	{
		return BZIP_STATUS_ERROR;
	}

	return BZIP_STATUS_BADDATA;
}

int BzipOpen(bool bootApp)
{
	int nMemLen;                                                                                         // Zip name number

	nTotalSize = 0;
	nBzipError = 0;

	if (szBzipName == NULL)
	{
		return 1;
	}

	BzipClose();                                                                                         // Make sure nothing is open

	if (!bootApp)                                                                                        // reset information strings
	{
		BzipText.Reset();
		BzipDetail.Reset();
	}

	// Count the number of roms needed
	for (nRomCount = 0;; nRomCount++)
	{
		if (BurnDrvGetRomInfo(NULL, nRomCount))
		{
			break;
		}
	}
	if (nRomCount <= 0)
	{
		return 1;
	}

	// Create an array for holding lookups for each rom -> zip entries
	nMemLen = nRomCount * sizeof(struct RomFind);
	RomFind = (struct RomFind*)malloc(nMemLen);
	if (RomFind == NULL)
	{
		return 1;
	}
	memset(RomFind, 0, nMemLen);

	for (int y = 0; y < BZIP_MAX; y++)
	{
		if (szBzipName[y])
		{
			free(szBzipName[y]);
			szBzipName[y] = NULL;
		}
	}

	// Locate each zip file
	for (int y = 0, z = 0; y < BZIP_MAX && z < BZIP_MAX; y++)
	{
		char* szName = NULL;
		bool  bFound = false;

		if (BurnDrvGetZipName(&szName, y))
		{
			break;
		}

		for (int d = 0; d < DIRS_MAX; d++) // Traverse the user-configured rom paths
		{
			TCHAR szFullName[MAX_PATH];
			if (strlen(szAppRomPaths[d]) == 0) 
				continue;
			
			_stprintf(szFullName, _T("%s%hs"), szAppRomPaths[d], szName);

			if (RomArchiveExists(szFullName))                  // Check existence of the rom zip/7z archive file

			{
				bFound = true;
				szBzipName[z] = (TCHAR*)malloc(MAX_PATH * sizeof(TCHAR));
				_tcscpy(szBzipName[z], szFullName);
				_stprintf(szBzipName[z], _T("%s%hs"), szAppRomPaths[d], szName);


				if (!bootApp)
				{
					BzipText.Add(_T("Found %s;\n"), szBzipName[z]);
				}

				z++;
				if (z >= BZIP_MAX)
				{
					break;
				}
			}
		}

		if (!bootApp && !bFound)
		{
			BzipText.Add(_T("Not found %s;\n"), szBzipName[z]);
		}
	}

	// Locate the ROM data in the zip files
	for (int z = 0; z < BZIP_MAX; z++)
	{
		if (szBzipName[z] == NULL)
		{
			continue;
		}

		if (ZipOpen(TCHARToANSI(szBzipName[z], NULL, 0)) == 0)                            // Open the rom zip file
		{
			nCurrentZip = z;

			ZipGetList(&List, &nListCount);                                                                        // Get the list of entries

			for (int i = 0; i < nRomCount; i++)
			{
				struct BurnRomInfo ri;
				int nFind;

				if (RomFind[i].nState == 1)                                                                         // Already found this and it's okay
				{
					continue;
				}

				memset(&ri, 0, sizeof(ri));

				nFind = FindRom(i);

				if (nFind < 0)                                                                                                      // Couldn't find this rom at all
				{
					continue;
				}

				RomFind[i].nZip = z;                                                                                      // Remember which zip file it is in
				RomFind[i].nPos = nFind;
				RomFind[i].nState = 1;                                                                                      // Set to found okay

				BurnDrvGetRomInfo(&ri, i);                                                                                  // Get info about the rom

				if ((ri.nType & BRF_OPT) == 0 && (ri.nType & BRF_NODUMP) == 0)
				{
					nTotalSize += ri.nLen;
				}

				if (List[nFind].nLen == ri.nLen)
				{
					if (ri.nCrc)                                                                                     // If we know the CRC
					{
						if (List[nFind].nCrc != ri.nCrc)                                                              // Length okay, but CRC wrong
						{
							RomFind[i].nState = 2;
						}
					}
				}
				else
				{
					if (List[nFind].nLen < ri.nLen)
					{
						RomFind[i].nState = 3;                                                                                // Too small
					}
					else
					{
						RomFind[i].nState = 4;                                                                                // Too big
					}
				}

				if (!bootApp)
				{
					if (RomFind[i].nState != 1)
					{
						RomDescribe(&BzipDetail, &ri);

						if (RomFind[i].nState == 2)
						{
							BzipDetail.Add(_T("%hs has a CRC of %.8X. (It should be %.8X.)\n"), GetFilenameA(List[nFind].szName), List[nFind].nCrc, ri.nCrc);
						}
						if (RomFind[i].nState == 3)
						{
							BzipDetail.Add(_T("%hs is %dk which is incomplete. (It should be %dkB.)\n"), GetFilenameA(List[nFind].szName), List[nFind].nLen >> 10, ri.nLen >> 10);
						}
						if (RomFind[i].nState == 4)
						{
							BzipDetail.Add(_T("%hs is %dk which is too big. (It should be %dkB.)\n"), GetFilenameA(List[nFind].szName), List[nFind].nLen >> 10, ri.nLen >> 10);
						}
					}
				}
			}

			BzipListFree();
		}

		ZipClose();                                                                                                                       // Close the last zip file if open
		nCurrentZip = -1;
	}

	if (!bootApp)
	{
		// Check the roms to see if the code, graphics etc are complete
		CheckRoms();

		if (nBzipError & 0x2000)
		{
			if (!(nBzipError & 0x0F0F))
			{
				BzipText.Add(_T("Rom Found ok\n"));
			}
			else
			{
				BzipText.Add(_T("Rom Found Problem\n"));
			}

			if (nBzipError & 0x0101)
			{
				if (nBzipError & 0x0001)
				{
					BzipText.Add(_T("Rom Missing\n"));
				}
				else
				{
					BzipText.Add(_T("Rom is bad!!!\n"));
				}
			}
			if (nBzipError & 0x0202)
			{
				if (nBzipError & 0x0002)
				{
					BzipText.Add(_T("Essential rom data is missing; the game probably won't run.\n"));
				}
				else
				{
					BzipText.Add(_T("Some essential roms are different. \n"));
				}
			}
			if (nBzipError & 0x0404)
			{
				if (nBzipError & 0x0004)
				{
					BzipText.Add(_T("Graphical data is missing.\n "));
				}
				else
				{
					BzipText.Add(_T("Some graphics roms are different. \n"));
				}
			}
			if (nBzipError & 0x0808)
			{
				if (nBzipError & 0x0008)
				{
					BzipText.Add(_T("Sound data is missing. \n"));
				}
				else
				{
					BzipText.Add(_T("Some sound roms are different. \n"));
				}
			}

			// Catch non-categorised ROMs
			if ((nBzipError & 0x0F0F) == 0)
			{
				if (nBzipError & 0x0010)
				{
					if (nBzipError & 0x1000)
					{
						BzipText.Add(_T("Data is missing.\n"));
					}
					else
					{
						BzipText.Add(_T("Data is Different.\n"));
					}
				}
			}
		}
		else
		{
			BzipText.Add(_T("Data is BAD.\n"));
		}

		// check for hard drive images (assumed max one per game)
		char* szHDDNameTmp = NULL;
		BurnDrvGetHDDName(&szHDDNameTmp, 0, 0);

		if (szHDDNameTmp)
		{
			char* szHddFolderName = NULL;

			if (BurnDrvGetTextA(DRV_PARENT))
			{
				szHddFolderName = BurnDrvGetTextA(DRV_PARENT);
			}
			else
			{
				szHddFolderName = BurnDrvGetTextA(DRV_NAME);
			}

			char szHDDPath[MAX_PATH];
			sprintf(szHDDPath, "%s%s/%s", _TtoA(szAppHDDPath), szHddFolderName, szHDDNameTmp);

			FILE* test = fopen(szHDDPath, "rb");
			if (test)
			{
				fclose(test);
			}
			else
			{
				BzipText.Add(_T("HDD image is missing!.\n"));

				nBzipError = 1;
			}
		}

		BurnExtLoadRom = BzipBurnLoadRom;                                                                         // Okay to call our function to load each rom
	}
	else
	{
		// check for hard drive images (assumed max one per game)
		char* szHDDNameTmp = NULL;
		BurnDrvGetHDDName(&szHDDNameTmp, 0, 0);

		if (szHDDNameTmp)
		{
			char* szHddFolderName = NULL;

			if (BurnDrvGetTextA(DRV_PARENT))
			{
				szHddFolderName = BurnDrvGetTextA(DRV_PARENT);
			}
			else
			{
				szHddFolderName = BurnDrvGetTextA(DRV_NAME);
			}

			char szHDDPath[MAX_PATH];
			sprintf(szHDDPath, "%s%s/%s", _TtoA(szAppHDDPath), szHddFolderName, szHDDNameTmp);

			FILE* test = fopen(szHDDPath, "rb");
			if (test)
			{
				fclose(test);
			}
			else
			{
				return 1;
			}
		}

		return CheckRomsBoot();
	}

	return 0;
}

int BzipClose()
{
	ZipClose();
	nCurrentZip = -1;                                                                                                    // Close the last zip file if open

	BurnExtLoadRom = NULL;                                                                                               // Can't call our function to load each rom anymore
	nBzipError = 0;                                                                                                  // reset romset errors

	if (RomFind)
	{
		free(RomFind);
		RomFind = NULL;
	}
	nRomCount = 0;

	for (int z = 0; z < BZIP_MAX; z++)
	{
		if (szBzipName[z])
		{
			free(szBzipName[z]);
			szBzipName[z] = NULL;
		}
	}

	return 0;
}
