#ifndef _BURNER_LIBRETRO_H
#define _BURNER_LIBRETRO_H

#include "gameinp.h"
#include "cd_interface.h"

char* TCHARToANSI(const TCHAR* pszInString, char* pszOutString, int /*nOutSize*/);
extern void InpDIPSWResetDIPs (void);
extern TCHAR szAppBurnVer[16];

#endif
