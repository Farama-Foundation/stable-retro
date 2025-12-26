/*
    Copyright 2016-2019 Arisotura

    This file is part of melonDS.

    melonDS is free software: you can redistribute it and/or modify it under
    the terms of the GNU General Public License as published by the Free
    Software Foundation, either version 3 of the License, or (at your option)
    any later version.

    melonDS is distributed in the hope that it will be useful, but WITHOUT ANY
    WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
    FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with melonDS. If not, see http://www.gnu.org/licenses/.
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "Config.h"
#include "Platform.h"


namespace Config
{
    char BIOS9Path[1024];
    char BIOS7Path[1024];
    char FirmwarePath[1024];
    int DLDIEnable;
    char DLDISDPath[1024];

    char FirmwareUsername[64];
    int FirmwareLanguage;
    int FirmwareFavouriteColour;
    int FirmwareBirthdayMonth;
    int FirmwareBirthdayDay;
    char FirmwareMessage[1024];
    bool FirmwareOverrideSettings;

    char DSiBIOS9Path[1024];
    char DSiBIOS7Path[1024];
    char DSiFirmwarePath[1024];
    char DSiNANDPath[1024];
    int DSiSDEnable;
    char DSiSDPath[1024];

    int RandomizeMAC;

#ifdef JIT_ENABLED
    int JIT_Enable = true;
    int JIT_MaxBlockSize = 12;
    int JIT_BranchOptimisations = true;
    int JIT_LiteralOptimisations = true;
    int JIT_FastMemory = false;
#else
    // Needed for savestate
    int JIT_Enable = false;
#endif

    int AudioBitrate = 0;
    int AudioInterp = 0;
    int ConsoleType = 0;
    int DirectBoot = 0;

    ConfigEntry ConfigFile[] =
    {
#ifdef JIT_ENABLED
        {"JIT_Enable", 0, &JIT_Enable, 0, NULL, 0},
        {"JIT_MaxBlockSize", 0, &JIT_MaxBlockSize, 10, NULL, 0},
        {"JIT_BranchOptimisations", 0, &JIT_BranchOptimisations, 1, NULL, 0},
        {"JIT_LiteralOptimisations", 0, &JIT_LiteralOptimisations, 1, NULL, 0},
#endif

        {"", -1, NULL, 0, NULL, 0}
    };

    extern ConfigEntry PlatformConfigFile[];


    void Load()
    {

    }

    void Save()
    {

    }
}
