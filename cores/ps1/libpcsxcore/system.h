/***************************************************************************
 *   Copyright (C) 2007 Ryan Schultz, PCSX-df Team, PCSX team              *
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
 *   51 Franklin Street, Fifth Floor, Boston, MA 02111-1307 USA.           *
 ***************************************************************************/

#ifndef __SYSTEM_H__
#define __SYSTEM_H__

#ifdef __cplusplus
extern "C" {
#endif

int SysInit();							// Init mem and plugins
void SysReset();						// Resets mem
void SysPrintf(const char *fmt, ...)
#if defined(__GNUC__) && defined(__x86_64__) // some platforms have int32_t as long
	 __attribute__((format(printf, 1, 2)))
#endif
	 ;
void SysMessage(const char *fmt, ...)	// Message used to print msg to users
#if defined(__GNUC__) && defined(__x86_64__)
	 __attribute__((format(printf, 1, 2)))
#endif
	 ;
void *SysLoadLibrary(const char *lib);	// Loads Library
void *SysLoadSym(void *lib, const char *sym);	// Loads Symbol from Library
const char *SysLibError();				// Gets previous error loading sysbols
void SysCloseLibrary(void *lib);		// Closes Library
void SysRunGui();						// Returns to the Gui
void SysClose();						// Close mem and plugins

// log if the game does something we don't handle (well)
//#define log_unhandled printf
#define log_unhandled(...)

#ifdef __cplusplus
}
#endif
#endif
