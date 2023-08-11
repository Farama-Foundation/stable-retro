// ----------------------------------------------------------------------------------------------------------
// NEOCDSEL.CPP
#include "burner.h"
#include "png.h"
#include "neocdlist.h"

#include <process.h>

int				NeoCDList_Init();
static void		NeoCDList_InitListView();
static int		NeoCDList_CheckISO(HWND hList, TCHAR* pszFile);
static void		NeoCDList_iso9660_CheckDirRecord(HWND hList, TCHAR* pszFile,  FILE* fp, int nSector);
static int		NeoCDList_AddGame(TCHAR* pszFile, unsigned int nGameID);
static void		NeoCDList_ScanDir(HWND hList, TCHAR* pszDirectory);
static TCHAR*	NeoCDList_ParseCUE(TCHAR* pszFile);
static void		NeoCDList_ShowPreview(HWND hDlg, TCHAR* szFile, int nCtrID, int nFrameCtrID, float maxw, float maxh);
struct PNGRESOLUTION { int nWidth; int nHeight; };
static PNGRESOLUTION GetPNGResolution(TCHAR* szFile);

static INT_PTR	CALLBACK	NeoCDList_WndProc(HWND hDlg, UINT Msg, WPARAM wParam, LPARAM lParam);
static unsigned __stdcall NeoCDList_DoProc(void*);

static HWND		hNeoCDWnd			= NULL;
static HWND		hListView			= NULL;
static HWND		hProcParent			= NULL;
static bool		bProcessingList		= false;
static HANDLE   hProcessThread = NULL;
static unsigned ProcessThreadID = 0;

static HBRUSH hWhiteBGBrush;

bool bNeoCDListScanSub			= false;
bool bNeoCDListScanOnlyISO		= false;
TCHAR szNeoCDCoverDir[MAX_PATH] = _T("support/neocdz/");
TCHAR szNeoCDGamesDir[MAX_PATH] = _T("/neocdiso/");

static int nSelectedItem = -1;

struct GAMELIST {
	unsigned int nID;
	bool	bFoundCUE;
	TCHAR	szPathCUE[256];
	TCHAR	szPath[256];
	TCHAR	szISOFile[256];
	int		nAudioTracks;
	TCHAR	szTracks[99][256];
	TCHAR	szShortName[32];
	TCHAR	szPublisher[100];
};

static GAMELIST ngcd_list[100];
static int nListItems = 0;

// CD image stuff
const int nSectorLength		= 2352;

// Add game to List
static int NeoCDList_AddGame(TCHAR* pszFile, unsigned int nGameID)
{
	NGCDGAME* game;

	if(GetNeoGeoCDInfo(nGameID))
	{
		game = (NGCDGAME*)malloc(sizeof(NGCDGAME));
		memset(game, 0, sizeof(NGCDGAME));

		memcpy(game, GetNeoGeoCDInfo(nGameID), sizeof(NGCDGAME));

		TCHAR szNGCDID[12];
		_stprintf(szNGCDID, _T("%04X"), nGameID);

		LVITEM lvi;
		ZeroMemory(&lvi, sizeof(lvi));

		// NGCD-ID [Insert]
		lvi.iImage			= 0;
		lvi.iItem			= ListView_GetItemCount(hListView) + 1;
		lvi.mask			= LVIF_TEXT | LVIF_PARAM | LVIF_IMAGE;
		lvi.cchTextMax		= 255;
		TCHAR szTitle[256];
		_stprintf(szTitle, _T(" %s"), game->pszTitle);
		lvi.pszText			= szTitle;

		int nItem = ListView_InsertItem(hListView, &lvi);

		// TITLE
		ZeroMemory(&lvi, sizeof(lvi));

		lvi.iSubItem		= 1;
		lvi.iItem			= nItem;
		lvi.cchTextMax		= _tcslen(game->pszTitle);
		lvi.mask			= LVIF_TEXT | LVIF_IMAGE;
		lvi.pszText			= szNGCDID;

		ListView_SetItem(hListView, &lvi);

		ngcd_list[nListItems].nID = nGameID;

		_tcscpy(ngcd_list[nListItems].szPath, pszFile);
		_tcscpy(ngcd_list[nListItems].szShortName, game->pszName);
		_stprintf(ngcd_list[nListItems].szPublisher, _T("%s (%s)"), game->pszCompany, game->pszYear);

		nListItems++;

	} else {
		// error

		return 0;
	}

	if(game) {
		free(game);
		game = NULL;
	}
	return 1;
}

static void NeoCDList_InitListView()
{
	LVCOLUMN LvCol;
	ListView_SetExtendedListViewStyle(hListView, LVS_EX_FULLROWSELECT);

	memset(&LvCol, 0, sizeof(LvCol));
	LvCol.mask		= LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;

	LvCol.cx		= 445;
	LvCol.pszText	= FBALoadStringEx(hAppInst, IDS_NGCD_TITLE, true);
	SendMessage(hListView, LVM_INSERTCOLUMN , 0, (LPARAM)&LvCol);

	LvCol.cx		= 70;
	LvCol.pszText	= FBALoadStringEx(hAppInst, IDS_NGCD_NGCDID, true);
	SendMessage(hListView, LVM_INSERTCOLUMN , 1, (LPARAM)&LvCol);

	// Setup ListView Icons
//	HIMAGELIST hImageList = ImageList_Create(16, 16, ILC_MASK | ILC_COLOR16, 0, 1);
//	ListView_SetImageList(hListView, hImageList, LVSIL_SMALL);
//	ImageList_AddMasked(hImageList, LoadBitmap(hAppInst, MAKEINTRESOURCE(BMP_CD)), RGB(255, 0, 255));
}

static int NeoCDList_CheckDuplicates(HWND hList, unsigned int nID)
{
	int nItemCount = ListView_GetItemCount(hList);

	for(int nItem = 0; nItem < nItemCount; nItem++)
	{
		unsigned int nItemVal = 0;
		TCHAR szText[32];

		ListView_GetItemText(hList, nItem, 1, szText, 5);

		_stscanf(szText, _T("%x"), &nItemVal);

		if(nItemVal == nID) {
			ngcd_list[nItemCount].nAudioTracks = 0;
			return 1; // let's get out of here...
		}
	}

	return 0;
}

static void NeoCDList_iso9660_CheckDirRecord(HWND hList, TCHAR* pszFile,  FILE* fp, int nSector)
{
	//int		nFile				= 0;
	unsigned int	lBytesRead			= 0;
	//int		nLen				= 0;
	unsigned int	lOffset				= 0;
	bool	bNewSector			= false;
	bool	bRevisionQueve		= false;
	int		nRevisionQueveID	= 0;

	lOffset = (nSector * nSectorLength);

	UINT8 nLenDR;
	UINT8 Flags;
	UINT8 LEN_FI;
	UINT8* ExtentLoc = (UINT8*)malloc(8 + 32);
	UINT8* Data = (UINT8*)malloc(0x10b + 32);
	char *File = (char*)malloc(32 + 32);

	while(1)
	{
		iso9660_ReadOffset(&nLenDR, fp, lOffset, 1, sizeof(UINT8));

		if(nLenDR == 0x22) {
			lOffset		+= nLenDR;
			lBytesRead	+= nLenDR;
			continue;
		}

		if(nLenDR < 0x22)
		{
			if(bNewSector)
			{
				if(bRevisionQueve) {
					bRevisionQueve = false;

					// make sure we don't add duplicates to the list
					if(NeoCDList_CheckDuplicates(hList, nRevisionQueveID)) {
						return;
					}

					NeoCDList_AddGame(pszFile, nRevisionQueveID);
				}
				return;
			}

			nLenDR = 0;
			iso9660_ReadOffset(&nLenDR, fp, lOffset + 1, 1, sizeof(UINT8));

			if(nLenDR < 0x22) {
				lOffset += (nSectorLength - lBytesRead);
				lBytesRead = 0;
				bNewSector = true;
				continue;
			}
		}

		bNewSector = false;

		iso9660_ReadOffset(&Flags, fp, lOffset + 25, 1, sizeof(UINT8));

		if(!(Flags & (1 << 1)))
		{
			iso9660_ReadOffset(ExtentLoc, fp, lOffset + 2, 8, sizeof(UINT8));

			char szValue[32];
			sprintf(szValue, "%02x%02x%02x%02x", ExtentLoc[4], ExtentLoc[5], ExtentLoc[6], ExtentLoc[7]);

			unsigned int nValue = 0;
			sscanf(szValue, "%x", &nValue);

			iso9660_ReadOffset(Data, fp, nValue * nSectorLength, 0x10a, sizeof(UINT8));

			char szData[32];
			sprintf(szData, "%c%c%c%c%c%c%c", Data[0x100], Data[0x101], Data[0x102], Data[0x103], Data[0x104], Data[0x105], Data[0x106]);

			if(!strncmp(szData, "NEO-GEO", 7))
			{
				_tcscpy(ngcd_list[nListItems].szISOFile, pszFile);

				char id[32];
				sprintf(id, "%02X%02X",  Data[0x108], Data[0x109]);

				unsigned int nID = 0;
				sscanf(id, "%x", &nID);

				iso9660_ReadOffset(&LEN_FI, fp, lOffset + 32, 1, sizeof(UINT8));

				iso9660_ReadOffset((UINT8*)File, fp, lOffset + 33, LEN_FI, sizeof(char));
				File[LEN_FI] = 0;

				// Treasure of Caribbean (c) 1994 / (c) 2011 NCI
				if(nID == 0x0048 && Data[0x67] == 0x08) {
					nID = 0x1048;
				}

				// King of Fighters '94, The (1994)(SNK)(JP)
				// 10-6-1994 (P1.PRG)
				if(nID == 0x0055 && Data[0x67] == 0xDE) {
					// ...continue
				}

				// King of Fighters '94, The (1994)(SNK)(JP-US)
				// 11-21-1994 (P1.PRG)
				if(nID == 0x0055 && Data[0x67] == 0xE6) {
					// Change to custom revision id
					nID = 0x1055;
				}

				// King of Fighters '95, The (1995)(SNK)(JP-US)[!][NGCD-084 MT B01, B03-B06, NGCD-084E MT B01]
				// 9-11-1995 (P1.PRG)
				if(nID == 0x0084 && Data[0x6C] == 0xC0) {
					// ... continue
				}

				// King of Fighters '95, The (1995)(SNK)(JP-US)[!][NGCD-084 MT B10, NGCD-084E MT B03]
				// 10-5-1995 (P1.PRG)
				if(nID == 0x0084 && Data[0x6C] == 0xFF) {
					// Change to custom revision id
					nID = 0x1084;
				}

				// King of Fighters '96 NEOGEO Collection, The
				if(nID == 0x0229) {
					bRevisionQueve = false;

					// make sure we don't add duplicates to the list
					if(NeoCDList_CheckDuplicates(hList, nID)) {
						return;
					}

					NeoCDList_AddGame(pszFile, nID);
					break;
				}

				// King of Fighters '96, The
				if(nID == 0x0214) {
					bRevisionQueve		= true;
					nRevisionQueveID	= nID;
					lOffset		+= nLenDR;
					lBytesRead	+= nLenDR;
					continue;
				}

				// make sure we don't add duplicates to the list
				if(NeoCDList_CheckDuplicates(hList, nID)) {
					return;
				}

				NeoCDList_AddGame(pszFile, nID);

				//MessageBoxA(NULL, id, "", MB_OK);
				break;
			}
		}

		lOffset		+= nLenDR;
		lBytesRead	+= nLenDR;
	}

	if (ExtentLoc) {
		free(ExtentLoc);
		ExtentLoc = NULL;
	}

	if (Data) {
		free(Data);
		Data = NULL;
	}

	if (File) {
		free(File);
		File = NULL;
	}
}

// Check the specified ISO, and proceed accordingly
static int NeoCDList_CheckISO(HWND hList, TCHAR* pszFile)
{
	if(!pszFile) {
		// error
		return 0;
	}

	// Make sure we have a valid ISO file extension...
	if ( IsFileExt(pszFile, _T(".img")) || IsFileExt(pszFile, _T(".bin")) )
	{
		FILE* fp = _tfopen(pszFile, _T("rb"));
		if(fp)
		{
			// Read ISO and look for 68K ROM standard program header, ID is always there
			// Note: This function works very quick, doesn't compromise performance :)
			// it just read each sector first 264 bytes aproximately only.

			// Get ISO Size (bytes)
			fseek(fp, 0, SEEK_END);
			unsigned int lSize = 0;
			lSize = ftell(fp);
			fseek(fp, 0, SEEK_SET);

			// If it has at least 16 sectors proceed
			if(lSize > (nSectorLength * 16))
			{
				// Check for Standard ISO9660 Identifier
				UINT8 IsoCheck[32];

				// advance to sector 16 and PVD Field 2
				iso9660_ReadOffset(&IsoCheck[0], fp, nSectorLength * 16 + 1, 1, 5); // get Standard Identifier Field from PVD

				// Verify that we have indeed a valid ISO9660 MODE1/2352
				if(!memcmp(IsoCheck, "CD001", 5))
				{
					//bprintf(PRINT_NORMAL, _T("    Standard ISO9660 Identifier Found. \n"));
					iso9660_VDH vdh;

					// Get Volume Descriptor Header
					memset(&vdh, 0, sizeof(vdh));
					//memcpy(&vdh, iso9660_ReadOffset(fp, (2048 * 16), sizeof(vdh)), sizeof(vdh));
					iso9660_ReadOffset((UINT8*)&vdh, fp, 16 * nSectorLength, 1, sizeof(vdh));

					// Check for a valid Volume Descriptor Type
					if(vdh.vdtype == 0x01)
					{
#if 0
// This will fail on 64-bit due to differing variable sizes in the pvd struct
						// Get Primary Volume Descriptor
						iso9660_PVD pvd;
						memset(&pvd, 0, sizeof(pvd));
						//memcpy(&pvd, iso9660_ReadOffset(fp, (2048 * 16), sizeof(pvd)), sizeof(pvd));
						iso9660_ReadOffset((UINT8*)&pvd, fp, nSectorLength * 16, 1, sizeof(pvd));

						// ROOT DIRECTORY RECORD

						// Handle Path Table Location
						char szRootSector[32];
						unsigned int nRootSector = 0;

						sprintf(szRootSector, "%02X%02X%02X%02X",
							pvd.root_directory_record.location_of_extent[4],
							pvd.root_directory_record.location_of_extent[5],
							pvd.root_directory_record.location_of_extent[6],
							pvd.root_directory_record.location_of_extent[7]);

						// Convert HEX string to Decimal
						sscanf(szRootSector, "%X", &nRootSector);
#else
// Just read from the file directly at the correct offset (SECTOR_SIZE * 16 + 0x9e for the start of the root directory record)
						UINT8 buffer[32];
						char szRootSector[32];
						unsigned int nRootSector = 0;

						iso9660_ReadOffset(&buffer[0], fp, nSectorLength * 16 + 0x9e, 1, 8);

						sprintf(szRootSector, "%02x%02x%02x%02x", buffer[4], buffer[5], buffer[6], buffer[7]);

						sscanf(szRootSector, "%x", &nRootSector);
#endif

						// Process the Root Directory Records
						NeoCDList_iso9660_CheckDirRecord(hList, pszFile, fp, nRootSector);

						// Path Table Records are not processed, since NeoGeo CD should not have subdirectories
						// ...
					}
				} else {

					//bprintf(PRINT_NORMAL, _T("    Standard ISO9660 Identifier Not Found, cannot continue. \n"));
					return 0;
				}
			}
		} else {

			//bprintf(PRINT_NORMAL, _T("    Couldn't open %s \n"), GetIsoPath());
			return 0;
		}

		if(fp) fclose(fp);

	} else {

		//bprintf(PRINT_NORMAL, _T("    File doesn't have a valid ISO extension [ .iso / .ISO ] \n"));
		return 0;
	}

	return 1;
}

static int ScanDir_RecursionCount = 0;

// Scan the specified directory for ISO / CUE files, recurse if desired.
static void NeoCDList_ScanDir_Internal(HWND hList, TCHAR* pszDirectory)
{
	if (ScanDir_RecursionCount > 8) {
		bprintf(PRINT_ERROR, _T("*** Recursion too deep, can't traverse \"%s\"!\n"), pszDirectory);
		return;
	}

	ScanDir_RecursionCount++;

	WIN32_FIND_DATA ffdDirectory;
	HANDLE hDirectory = NULL;
	memset(&ffdDirectory, 0, sizeof(WIN32_FIND_DATA));

	// Scan directory for CUE
	TCHAR szSearch[512] = _T("\0");

	_stprintf(szSearch, _T("%s*.*"), pszDirectory);

	hDirectory = FindFirstFile(szSearch, &ffdDirectory);

	if (hDirectory != INVALID_HANDLE_VALUE)
	{
		do
		{
			// Directories only
			if (ffdDirectory.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			{
				if (!_tcscmp(ffdDirectory.cFileName, _T(".")) || !_tcscmp(ffdDirectory.cFileName, _T("..")))
				{
					// lets ignore " . " and " .. "
				}
				else if (bNeoCDListScanSub)
				{
					// Traverse & recurse this subdir
					TCHAR szNewDir[512] = _T("\0");
					_stprintf(szNewDir, _T("%s%s\\"), pszDirectory, ffdDirectory.cFileName);

					NeoCDList_ScanDir_Internal(hList, szNewDir);
				}
			}
			// Files only
			if (!(ffdDirectory.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
			{
				if (IsFileExt(ffdDirectory.cFileName, _T(".cue")))
				{
					// Found CUE, Parse it
					TCHAR szParse[512] = _T("\0");
					_stprintf(szParse, _T("%s%s"), pszDirectory, ffdDirectory.cFileName);

					TCHAR *pszISO = NeoCDList_ParseCUE( szParse );

					TCHAR szISO[512] =_T("\0");
					_stprintf(szISO, _T("%s%s"), pszDirectory, pszISO);
					free(pszISO);

					NeoCDList_CheckISO(hList, szISO);
				}
			}
		} while (FindNextFile(hDirectory, &ffdDirectory));

		FindClose(hDirectory);
	}

	ScanDir_RecursionCount--;
}

static void NeoCDList_ScanDir(HWND hList, TCHAR* pszDirectory)
{
	ScanDir_RecursionCount = 0;

	NeoCDList_ScanDir_Internal(hList, pszDirectory);
}

// This will parse the specified CUE file and return the ISO path, if found
static TCHAR* NeoCDList_ParseCUE(TCHAR* pszFile)
{
	//if(!pszFile) return NULL;

	TCHAR* szISO = NULL;
	szISO = (TCHAR*)malloc(sizeof(TCHAR) * 2048);
	if(!szISO) return NULL;

	// open file
	FILE* fp = NULL;
	fp = _tfopen(pszFile, _T("r"));

	if(!fp) {
		if (szISO)
		{
			free(szISO);
			return NULL;
		}
	}

	while(!feof(fp))
	{
		TCHAR szBuffer[2048];
		TCHAR szOriginal[2048];
		TCHAR* s;
		TCHAR* t;

		_fgetts(szBuffer, sizeof(szBuffer) - 1, fp);

		int nLength = 0;
		nLength = _tcslen(szBuffer);

		// Remove ASCII control characters from the string (including the 'space' character)
		while (szBuffer[nLength-1] < 32 && nLength > 0)
		{
			szBuffer[nLength-1] = 0;
			nLength--;
		}

		_tcscpy(szOriginal, szBuffer);

		if(!_tcsncmp(szBuffer, _T("FILE"), 4))
		{
			TCHAR* pEnd = _tcsrchr(szBuffer, '"');
			if (!pEnd)	{
				break;	// Invalid CUE format
			}

			*pEnd = 0;

			TCHAR* pStart = _tcschr(szBuffer, '"');

			if(!pStart)	{
				break;	// Invalid CUE format
			}

			if(!_tcsncmp(pEnd + 2, _T("BINARY"), 6))
			{
				_tcscpy(szISO,  pStart + 1);
				ngcd_list[nListItems].bFoundCUE = true;
				_tcscpy(ngcd_list[nListItems].szPathCUE,  pszFile);
				_tcscpy(ngcd_list[nListItems].szISOFile,  pStart + 1);
			}
		}
		// track info
		if ((t = LabelCheck(szBuffer, _T("TRACK"))) != 0) {
			s = t;

			// track number
			/*UINT8 track = */ _tcstol(s, &t, 10);

			s = t;

			// type of track

			if ((t = LabelCheck(s, _T("MODE1/2352"))) != 0) {
				//bprintf(0, _T(".cue: Track #%d, data.\n"), track);
				continue;
			}
			if ((t = LabelCheck(s, _T("AUDIO"))) != 0) {
				//bprintf(0, _T(".cue: Track #%d, AUDIO.\n"), track);
				ngcd_list[nListItems].nAudioTracks++;
				continue;
			}

			fclose(fp);
			return szISO;
		}
	}
	if(fp) fclose(fp);

	return szISO;
}

static PNGRESOLUTION GetPNGResolution(TCHAR* szFile)
{
	int width = 0;
	int height = 0;
	PNGRESOLUTION nResolution = { 0, 0 };
	png_structp png_ptr;
	png_infop info_ptr;
	char header[8];

	FILE *fp = _tfopen(szFile, _T("rb"));

	if (!fp) {
		return nResolution;
	}

	fread(header, 1, 8, fp);

	if (png_sig_cmp((png_const_bytep)header, 0, 8)) {
		return nResolution;
	}

	png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);

	if (!png_ptr) {
		return nResolution;
	}

	info_ptr = png_create_info_struct(png_ptr);
	if (!info_ptr) {
		return nResolution;
	}

	if (setjmp(png_jmpbuf(png_ptr))) {
		return nResolution;
	}

	png_init_io(png_ptr, fp);
	png_set_sig_bytes(png_ptr, 8);
	png_read_info(png_ptr, info_ptr);

	width	= png_get_image_width(png_ptr, info_ptr);
	height	= png_get_image_height(png_ptr, info_ptr);

	nResolution.nWidth = width;
	nResolution.nHeight = height;

	fclose(fp);

	return nResolution;
}

static void NeoCDList_ShowPreview(HWND hDlg, TCHAR* szFile, int nCtrID, int nFrameCtrID, float maxw, float maxh)
{
	PNGRESOLUTION PNGRes = { 0, 0 };
	if(!_tfopen(szFile, _T("rb")))
	{
		HRSRC hrsrc			= FindResource(NULL, MAKEINTRESOURCE(BMP_SPLASH), RT_BITMAP);
		HGLOBAL hglobal		= LoadResource(NULL, (HRSRC)hrsrc);

		BITMAPINFOHEADER* pbmih = (BITMAPINFOHEADER*)LockResource(hglobal);

		PNGRes.nWidth	= pbmih->biWidth;
		PNGRes.nHeight	= pbmih->biHeight;

		FreeResource(hglobal);

	} else {
		PNGRes = GetPNGResolution(szFile);
	}

	// ------------------------------------------------------
	// PROPER ASPECT RATIO CALCULATIONS

	float w = (float)PNGRes.nWidth;
	float h = (float)PNGRes.nHeight;

	//float maxw = 216; //
	//float maxh = 150; //

	// max WIDTH
	if(w > maxw) {
		float nh = maxw * (float)(h / w);
		float nw = maxw;

		// max HEIGHT
		if(nh > maxh) {
			nw = maxh * (float)(nw / nh);
			nh = maxh;
		}

		w = nw;
		h = nh;
	}

	// max HEIGHT
	if(h > maxh) {
		float nw = maxh * (float)(w / h);
		float nh = maxh;

		// max WIDTH
		if(nw > maxw) {
			nh = maxw * (float)(nh / nw);
			nw = maxw;
		}

		w = nw;
		h = nh;
	}

	// Proper centering of preview
	float x = ((maxw - w) / 2);
	float y = ((maxh - h) / 2);

	RECT rc = {0, 0, 0, 0};
	GetWindowRect(GetDlgItem(hDlg, nFrameCtrID), &rc);

	POINT pt;
	pt.x = rc.left;
	pt.y = rc.top;

	ScreenToClient(hDlg, &pt);

	// ------------------------------------------------------

	FILE* fp = _tfopen(szFile, _T("rb"));

	HBITMAP hCoverBmp = PNGLoadBitmap(hDlg, fp, (int)w, (int)h, 0);

	SetWindowPos(GetDlgItem(hDlg, nCtrID), NULL, (int)(pt.x + x), (int)(pt.y + y), 0, 0, SWP_NOSIZE | SWP_SHOWWINDOW);
	SendDlgItemMessage(hDlg, nCtrID, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)hCoverBmp);

	if(fp) fclose(fp);

}

static void NeoCDList_Clean()
{
	NeoCDList_ShowPreview(hNeoCDWnd, _T(""), IDC_NCD_FRONT_PIC, IDC_NCD_FRONT_PIC_FRAME, 216, 150);
	NeoCDList_ShowPreview(hNeoCDWnd, _T(""), IDC_NCD_BACK_PIC, IDC_NCD_BACK_PIC_FRAME, 216, 150);

	SetWindowText(GetDlgItem(hNeoCDWnd, IDC_NCD_TEXTSHORT), _T(""));
	SetWindowText(GetDlgItem(hNeoCDWnd, IDC_NCD_TEXTPUBLISHER), _T(""));
	SetWindowText(GetDlgItem(hNeoCDWnd, IDC_NCD_TEXTIMAGE), _T(""));
	SetWindowText(GetDlgItem(hNeoCDWnd, IDC_NCD_TEXTAUDIO), _T(""));

	hProcessThread = NULL;
	ProcessThreadID = 0;

	for(int x = 0; x < 100; x++) {
		for(int y = 0; y < 99; y++) {
			memset(&ngcd_list[x].szTracks[y], 0, sizeof(TCHAR) * 256);
		}
		ngcd_list[x].bFoundCUE = false;
	}
	memset(&ngcd_list, 0, (sizeof(GAMELIST) * 100));

	hProcParent			= NULL;
	bProcessingList		= false;

	nListItems = 0;

	nSelectedItem = -1;
}

static HWND hNeoCDList_CoverDlg = NULL;
TCHAR szBigCover[512] = _T("");

static INT_PTR CALLBACK NeoCDList_CoverWndProc(HWND hDlg, UINT Msg, WPARAM wParam, LPARAM /*lParam*/)
{
	if(Msg == WM_INITDIALOG)
	{
		hNeoCDList_CoverDlg = hDlg;

		NeoCDList_ShowPreview(hDlg, szBigCover, IDC_NCD_COVER_PREVIEW_PIC, IDC_NCD_COVER_PREVIEW_PIC, 580, 415);

		return TRUE;
	}

	if(Msg == WM_CLOSE)
	{
		EndDialog(hDlg, 0);
		hNeoCDList_CoverDlg	= NULL;
	}

	if(Msg == WM_COMMAND)
	{
		if (LOWORD(wParam) == WM_DESTROY) {
			SendMessage(hDlg, WM_CLOSE, 0, 0);
		}
	}

	return 0;
}

void NeoCDList_InitCoverDlg()
{
	FBADialogBox(hAppInst, MAKEINTRESOURCE(IDD_NCD_COVER_DLG), hNeoCDWnd, (DLGPROC)NeoCDList_CoverWndProc);
}

static INT_PTR CALLBACK NeoCDList_WndProc(HWND hDlg, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	if(Msg == WM_INITDIALOG)
	{
		hNeoCDWnd = hDlg;

		InitCommonControls();

		hListView = GetDlgItem(hDlg, IDC_NCD_LIST);

		NeoCDList_InitListView();

		HICON hIcon = LoadIcon(hAppInst, MAKEINTRESOURCE(IDI_APP));
		SendMessage(hDlg, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);		// Set the Game Selection dialog icon.

		hWhiteBGBrush	= CreateSolidBrush(RGB(0xFF,0xFF,0xFF));

		NeoCDList_ShowPreview(hNeoCDWnd, _T(""), IDC_NCD_FRONT_PIC, IDC_NCD_FRONT_PIC_FRAME, 216, 150);
		NeoCDList_ShowPreview(hNeoCDWnd, _T(""), IDC_NCD_BACK_PIC, IDC_NCD_BACK_PIC_FRAME, 216, 150);

		SetWindowText(GetDlgItem(hNeoCDWnd, IDC_NCD_TEXTSHORT), _T(""));
		SetWindowText(GetDlgItem(hNeoCDWnd, IDC_NCD_TEXTPUBLISHER), _T(""));
		SetWindowText(GetDlgItem(hNeoCDWnd, IDC_NCD_TEXTIMAGE), _T(""));
		SetWindowText(GetDlgItem(hNeoCDWnd, IDC_NCD_TEXTAUDIO), _T(""));

		CheckDlgButton(hNeoCDWnd, IDC_NCD_SSUBDIR_CHECK, bNeoCDListScanSub ? BST_CHECKED : BST_UNCHECKED);
		CheckDlgButton(hNeoCDWnd, IDC_NCD_SISO_ONLY_CHECK, bNeoCDListScanOnlyISO ? BST_CHECKED : BST_UNCHECKED);

		TreeView_SetItemHeight(hListView, 40);

		TCHAR szDialogTitle[200];
		_stprintf(szDialogTitle, FBALoadStringEx(hAppInst, IDS_NGCD_DIAG_TITLE, true), _T(APP_TITLE), _T(SEPERATOR_1), _T(SEPERATOR_1));
		SetWindowText(hDlg, szDialogTitle);

		hProcessThread = (HANDLE)_beginthreadex(NULL, 0, NeoCDList_DoProc, NULL, 0, &ProcessThreadID);

		WndInMid(hDlg, hScrnWnd);
		SetFocus(hListView);

		return TRUE;
	}

	if(Msg == WM_CLOSE)
	{
		NeoCDList_Clean();

		DeleteObject(hWhiteBGBrush);

		hNeoCDWnd	= NULL;
		hListView	= NULL;

		EndDialog(hDlg, 0);
	}

	if (Msg == WM_CTLCOLORSTATIC)
	{
		if ((HWND)lParam == GetDlgItem(hNeoCDWnd, IDC_NCD_LABELSHORT))	return (INT_PTR)hWhiteBGBrush;
		if ((HWND)lParam == GetDlgItem(hNeoCDWnd, IDC_NCD_LABELPUBLISHER))	return (INT_PTR)hWhiteBGBrush;
		if ((HWND)lParam == GetDlgItem(hNeoCDWnd, IDC_NCD_LABELIMAGE))	return (INT_PTR)hWhiteBGBrush;
		if ((HWND)lParam == GetDlgItem(hNeoCDWnd, IDC_NCD_LABELAUDIO))	return (INT_PTR)hWhiteBGBrush;
		if ((HWND)lParam == GetDlgItem(hNeoCDWnd, IDC_NCD_TEXTSHORT))	return (INT_PTR)hWhiteBGBrush;
		if ((HWND)lParam == GetDlgItem(hNeoCDWnd, IDC_NCD_TEXTPUBLISHER))	return (INT_PTR)hWhiteBGBrush;
		if ((HWND)lParam == GetDlgItem(hNeoCDWnd, IDC_NCD_TEXTIMAGE))	return (INT_PTR)hWhiteBGBrush;
		if ((HWND)lParam == GetDlgItem(hNeoCDWnd, IDC_NCD_TEXTAUDIO))	return (INT_PTR)hWhiteBGBrush;
	}

	if (Msg == WM_NOTIFY)
	{
		NMLISTVIEW* pNMLV	= (NMLISTVIEW*)lParam;

		// Game Selected
		if (pNMLV->hdr.code == LVN_ITEMCHANGED && pNMLV->hdr.idFrom == IDC_NCD_LIST)
		{
			int iCount		= SendMessage(hListView, LVM_GETITEMCOUNT, 0, 0);
			int iSelCount	= SendMessage(hListView, LVM_GETSELECTEDCOUNT, 0, 0);

			if(iCount == 0 || iSelCount == 0) return 1;

			TCHAR szID[] = _T("0000");

			int iItem = pNMLV->iItem;

			LVITEM LvItem;
			memset(&LvItem, 0, sizeof(LvItem));

			LvItem.iItem		= iItem;
			LvItem.mask			= LVIF_TEXT;
			LvItem.iSubItem		= 1;				// id column
			LvItem.pszText		= szID;
			LvItem.cchTextMax	= sizeof(szID) + 1;

			SendMessage(hListView, LVM_GETITEMTEXT, (WPARAM)iItem, (LPARAM)&LvItem);

			//MessageBox(NULL, LvItem.pszText, _T(""), MB_OK);

			SetWindowText(GetDlgItem(hNeoCDWnd, IDC_NCD_TEXTSHORT), _T(""));
			SetWindowText(GetDlgItem(hNeoCDWnd, IDC_NCD_TEXTPUBLISHER), _T(""));
			SetWindowText(GetDlgItem(hNeoCDWnd, IDC_NCD_TEXTIMAGE), _T(""));
			SetWindowText(GetDlgItem(hNeoCDWnd, IDC_NCD_TEXTAUDIO), _T(""));

			for(int nItem = 0; nItem < nListItems; nItem++)
			{
				unsigned int nID = 0;
				_stscanf(szID, _T("%x"), &nID);

				TCHAR szAudioTracks[] =_T("0");

				if(nID == ngcd_list[nItem].nID) {
					SetWindowText(GetDlgItem(hNeoCDWnd, IDC_NCD_TEXTSHORT), ngcd_list[nItem].szShortName);
					SetWindowText(GetDlgItem(hNeoCDWnd, IDC_NCD_TEXTPUBLISHER), ngcd_list[nItem].szPublisher);
					SetWindowText(GetDlgItem(hNeoCDWnd, IDC_NCD_TEXTIMAGE), ngcd_list[nItem].szISOFile);
					_stprintf(szAudioTracks, _T("%d"), ngcd_list[nItem].nAudioTracks);
					SetWindowText(GetDlgItem(hNeoCDWnd, IDC_NCD_TEXTAUDIO), szAudioTracks);

					TCHAR szFront[512];
					TCHAR szBack[512];

					_stprintf(szFront, _T("%s%s-front.png"), szNeoCDCoverDir, ngcd_list[nItem].szShortName );
					_stprintf(szBack, _T("%s%s.png"), szAppPreviewsPath, ngcd_list[nItem].szShortName );

					// Front / Back Cover preview
					NeoCDList_ShowPreview(hNeoCDWnd, szFront, IDC_NCD_FRONT_PIC, IDC_NCD_FRONT_PIC_FRAME, 216, 150);
					NeoCDList_ShowPreview(hNeoCDWnd, szBack, IDC_NCD_BACK_PIC, IDC_NCD_BACK_PIC_FRAME, 216, 150);

					nSelectedItem = nItem;
					break;
				}
			}


		}

		// Double Click
		if (pNMLV->hdr.code == NM_DBLCLK && pNMLV->hdr.idFrom == IDC_NCD_LIST)
		{
			if(nSelectedItem >= 0) {
				nCDEmuSelect = 0;
				if(ngcd_list[nSelectedItem].bFoundCUE) {
					_tcscpy(CDEmuImage, ngcd_list[nSelectedItem].szPathCUE);
				} else {
					_tcscpy(CDEmuImage, ngcd_list[nSelectedItem].szPath);
				}
			} else {
				return 0;
			}

			NeoCDList_Clean();

			hNeoCDWnd	= NULL;
			hListView	= NULL;

			EndDialog(hDlg, 0);

			BurnerLoadDriver(_T("neocdz"));
		}
	}

	if(Msg == WM_COMMAND)
	{
		if(HIWORD(wParam) == STN_CLICKED)
		{
			int nCtrlID = LOWORD(wParam);

			if(nCtrlID == IDC_NCD_FRONT_PIC)
			{
				if(nSelectedItem >= 0) {
					_stprintf(szBigCover, _T("%s%s-front.png"), szNeoCDCoverDir, ngcd_list[nSelectedItem].szShortName );

					if(!_tfopen(szBigCover, _T("rb"))) {
						szBigCover[0] = 0;
						return 0;
					}

					NeoCDList_InitCoverDlg();
				}
				return 0;
			}

			if(nCtrlID == IDC_NCD_BACK_PIC)
			{
				if(nSelectedItem >= 0) {
					_stprintf(szBigCover, _T("%s%s.png"), szAppPreviewsPath, ngcd_list[nSelectedItem].szShortName );

					if(!_tfopen(szBigCover, _T("rb"))) {
						szBigCover[0] = 0;
						return 0;
					}

					NeoCDList_InitCoverDlg();
				}
				return 0;
			}
		}

		if (LOWORD(wParam) == WM_DESTROY) {
			SendMessage(hDlg, WM_CLOSE, 0, 0);
		}

		if(HIWORD(wParam) == BN_CLICKED)
		{
			int nCtrlID	= LOWORD(wParam);

			switch(nCtrlID)
			{
				case IDC_NCD_PLAY_BUTTON:
				{
					if(nSelectedItem >= 0) {
						nCDEmuSelect = 0;
						if(ngcd_list[nSelectedItem].bFoundCUE) {
							_tcscpy(CDEmuImage, ngcd_list[nSelectedItem].szPathCUE);
						} else {
							_tcscpy(CDEmuImage, ngcd_list[nSelectedItem].szPath);
						}
					} else {
						break;
					}

					NeoCDList_Clean();

					DeleteObject(hWhiteBGBrush);

					hNeoCDWnd	= NULL;
					hListView	= NULL;

					EndDialog(hDlg, 0);

					BurnerLoadDriver(_T("neocdz"));
					break;
				}

				case IDC_NCD_SCAN_BUTTON:
				{
					if(bProcessingList) break;

					NeoCDList_Clean();
					hProcessThread = (HANDLE)_beginthreadex(NULL, 0, NeoCDList_DoProc, NULL, 0, &ProcessThreadID);
					SetFocus(hListView);
					break;
				}

				case IDC_NCD_SEL_DIR_BUTTON:
				{
					if(bProcessingList) break;

					NeoCDList_Clean();

					SupportDirCreateTab(IDC_SUPPORTDIR_EDIT8, hNeoCDWnd);
					hProcessThread = (HANDLE)_beginthreadex(NULL, 0, NeoCDList_DoProc, NULL, 0, &ProcessThreadID);
					SetFocus(hListView);
					break;
				}

				case IDC_NCD_SSUBDIR_CHECK:
				{
					if(BST_CHECKED == IsDlgButtonChecked(hDlg, IDC_NCD_SSUBDIR_CHECK)) {
						bNeoCDListScanSub = true;
					} else {
						bNeoCDListScanSub = false;
					}

					if(bProcessingList) break;

					NeoCDList_Clean();

					hProcessThread = (HANDLE)_beginthreadex(NULL, 0, NeoCDList_DoProc, NULL, 0, &ProcessThreadID);
					SetFocus(hListView);
					break;
				}
#if 0
				case IDC_NCD_SISO_ONLY_CHECK:
				{
					if(BST_CHECKED == IsDlgButtonChecked(hDlg, IDC_NCD_SISO_ONLY_CHECK)) {
						bNeoCDListScanOnlyISO = true;
					} else {
						bNeoCDListScanOnlyISO = false;
					}

					if(bProcessingList) break;

					NeoCDList_Clean();

					hProcessThread = (HANDLE)_beginthreadex(NULL, 0, NeoCDList_DoProc, NULL, 0, &ProcessThreadID);
					SetFocus(hListView);
					break;
				}
#endif
				case IDC_NCD_CANCEL_BUTTON:
				{
					NeoCDList_Clean();

					DeleteObject(hWhiteBGBrush);

					hNeoCDWnd	= NULL;
					hListView	= NULL;

					EndDialog(hDlg, 0);
					break;
				}
			}
		}
	}
	return 0;
}

static unsigned __stdcall NeoCDList_DoProc(void*)
{
	if(bProcessingList) return 0;

	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_HIGHEST);

	bProcessingList = true;
	ListView_DeleteAllItems(hListView);
	NeoCDList_ScanDir(hListView, szNeoCDGamesDir);

	bProcessingList = false;
	hProcessThread = NULL;
	ProcessThreadID = 0;

	return 0;
}

int NeoCDList_Init()
{
	return FBADialogBox(hAppInst, MAKEINTRESOURCE(IDD_NCD_DLG), hScrnWnd, (DLGPROC)NeoCDList_WndProc);
}
