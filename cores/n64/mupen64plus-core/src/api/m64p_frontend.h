/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *   Mupen64plus-core - m64p_frontend.h                                    *
 *   Mupen64Plus homepage: http://code.google.com/p/mupen64plus/           *
 *   Copyright (C) 2009 Richard Goedeken                                   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.          *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

/* This header file defines typedefs for function pointers to Core functions
 * designed for use by the front-end user interface.
 */

#ifndef M64P_FRONTEND_H
#define M64P_FRONTEND_H

#include "m64p_types.h"

#ifdef __cplusplus
extern "C" {
#endif


/* pointer types to the callback functions in the front-end application */
typedef void (*ptr_DebugCallback)(void *Context, int level, const char *message);
typedef void (*ptr_StateCallback)(void *Context, m64p_core_param param_type, int new_value);
#if defined(M64P_CORE_PROTOTYPES)
EXPORT void CALL DebugCallback(void *Context, int level, const char *message);
EXPORT void CALL StateCallback(void *Context, m64p_core_param param_type, int new_value);
#endif

/* CoreStartup()
 *
 * This function initializes libmupen64plus for use by allocating memory,
 * creating data structures, and loading the configuration file.
 */
typedef m64p_error (*ptr_CoreStartup)(int, const char *, const char *, void *, ptr_DebugCallback, void *, ptr_StateCallback);
#if defined(M64P_CORE_PROTOTYPES)
EXPORT m64p_error CALL CoreStartup(int, const char *, const char *, void *, ptr_DebugCallback, void *, ptr_StateCallback);
#endif

/* CoreShutdown()
 *
 * This function saves the configuration file, then destroys data structures
 * and releases memory allocated by the core library.
 */
typedef m64p_error (*ptr_CoreShutdown)(void);
#if defined(M64P_CORE_PROTOTYPES)
EXPORT m64p_error CALL CoreShutdown(void);
#endif

/* CoreDoCommand()
 *
 * This function sends a command to the emulator core.
 */
typedef m64p_error (*ptr_CoreDoCommand)(m64p_command, int, void *);
#if defined(M64P_CORE_PROTOTYPES)
EXPORT m64p_error CALL CoreDoCommand(m64p_command, int, void *);
#endif

/* CoreGetRomSettings()
 *
 * This function will retrieve the ROM settings from the mupen64plus INI file for
 * the ROM image corresponding to the given CRC values.
 */
typedef m64p_error (*ptr_CoreGetRomSettings)(m64p_rom_settings *, int, int, int);
#if defined(M64P_CORE_PROTOTYPES)
EXPORT m64p_error CALL CoreGetRomSettings(m64p_rom_settings *, int, int, int);
#endif

/* CoreSetAudioInterfaceBackend()
 *
 * This function allows the frontend to specify the audio backend to be used by
 * the AI controller. If any of the function pointers in the structure are NULL,
 * a dummy backend will be used (eg no sound).
 */
typedef m64p_error (*ptr_CoreSetAudioInterfaceBackend)(const struct m64p_audio_backend *);
#if defined(M64P_CORE_PROTOTYPES)
EXPORT m64p_error CALL CoreSetAudioInterfaceBackend(const struct m64p_audio_backend *);
#endif

EXPORT m64p_error CALL CoreAddCheat(const char *CheatName, m64p_cheat_code *CodeList, int NumCodes);

EXPORT m64p_error CALL CoreCheatEnabled(const char *CheatName, int Enabled);

EXPORT m64p_error CALL CoreCheatClearAll(void);

#ifdef __cplusplus
}
#endif

#endif /* #define M64P_FRONTEND_H */

