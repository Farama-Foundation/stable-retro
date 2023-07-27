#ifndef _MEDNAFEN_H
#define _MEDNAFEN_H

#include "mednafen-types.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define trio_sprintf sprintf /*compatibility with mednafen on libretro*/
#define trio_snprintf snprintf /*compatibility with mednafen on libretro*/

#define _(String) (String)

#include "math_ops.h"
#include "git.h"

#ifdef _WIN32
#define strcasecmp _stricmp
#endif

extern MDFNGI *MDFNGameInfo;

#include "settings.h"

void MDFN_DispMessage(const char *format, ...);

void MDFN_LoadGameCheats(void);
void MDFN_FlushGameCheats(void);

#include "mednafen-driver.h"

#include "mednafen-endian.h"

#endif
