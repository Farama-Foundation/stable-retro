/*
* Glide64 - Glide video plugin for Nintendo 64 emulators.
* Copyright (c) 2002  Dave2001
* Copyright (c) 2003-2009  Sergey 'Gonetz' Lipski
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

//****************************************************************
//
// Glide64 - Glide Plugin for Nintendo 64 emulators
// Project started on December 29th, 2001
//
// Authors:
// Dave2001, original author, founded the project in 2001, left it in 2002
// Gugaman, joined the project in 2002, left it in 2002
// Sergey 'Gonetz' Lipski, joined the project in 2002, main author since fall of 2002
// Hiroshi 'KoolSmoky' Morii, joined the project in 2007
//
//****************************************************************
//
// To modify Glide64:
// * Write your name and (optional)email, commented by your work, so I know who did it, and so that you can find which parts you modified when it comes time to send it to me.
// * Do NOT send me the whole project or file that you modified.  Take out your modified code sections, and tell me where to put them.  If people sent the whole thing, I would have many different versions, but no idea how to combine them all.
//
//****************************************************************

/**********************************************************************************
Common gfx plugin spec, version #1.3 maintained by zilmar (zilmar@emulation64.com)

All questions or suggestions should go through the mailing list.
http://www.egroups.com/group/Plugin64-Dev
***********************************************************************************

Notes:
------

Setting the approprate bits in the MI_INTR_REG and calling CheckInterrupts which
are both passed to the DLL in InitiateGFX will generate an Interrupt from with in
the plugin.

The Setting of the RSP flags and generating an SP interrupt  should not be done in
the plugin

**********************************************************************************/

#ifndef _GFX_H_INCLUDED__
#define _GFX_H_INCLUDED__

#include <stdint.h>
#include <stdio.h>
#include <boolean.h>
#include "m64p_types.h"
#include "m64p_plugin.h"
#include "m64p_config.h"
#include "m64p_vidext.h"

#define VIDEO_PLUGIN_API_VERSION	0x020100

void WriteLog(m64p_msg_level level, const char *msg, ...);

#ifndef MAX
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#endif

#ifndef MIN
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#endif 

#ifdef MSB_FIRST
    #define BYTE_ADDR_XOR        0
    #define WORD_ADDR_XOR        0
    #define BYTE4_XOR_BE(a)     (a)
#else
    #define BYTE_ADDR_XOR        3
    #define WORD_ADDR_XOR        1
    #define BYTE4_XOR_BE(a)     ((a) ^ 3)                
#endif

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <stdarg.h>

#define _ENDUSER_RELEASE_

#ifndef _ENDUSER_RELEASE_
#define BRIGHT_RED			// Keep enabled, option in dialog
#endif

#define COLORED_DEBUGGER	// ;) pretty colors

// rdram mask at 0x400000 bytes (bah, not right for majora's mask)
//#define BMASK	0x7FFFFF
extern uint32_t BMASK;

extern uint32_t update_screen_count;
extern uint32_t resolutions[0x18][2];

#ifdef LOGGING
#define LOG(...) WriteLog(M64MSG_INFO, __VA_ARGS__)
#define VLOG(...) WriteLog(M64MSG_VERBOSE, __VA_ARGS__)
#define WARNLOG(...) WriteLog(M64MSG_WARNING, __VA_ARGS__)
#define ERRLOG(...) WriteLog(M64MSG_ERROR, __VA_ARGS__)
#else
#define LOG(...)
#define VLOG(...)
#define WARNLOG(...)
#define ERRLOG(...)
#endif

#define OPEN_RDP_LOG()
#define CLOSE_RDP_LOG()
#define LRDP(x)

#define LRDP(x)
#define FRDP(x, ...)


#ifdef RDP_ERROR_LOG
/* FIXME */
#define RDP_E(x) 
#else
#define OPEN_RDP_E_LOG()
#define CLOSE_RDP_E_LOG()
#define RDP_E(x)
#define FRDP_E(x, ...)
#endif

extern int romopen;
extern int debugging;

extern int exception;
extern GFX_INFO gfx_info;

/* Plugin types */
#define PLUGIN_TYPE_GFX				2

/***** Structures *****/
typedef struct
{
   uint16_t Version;        /* Set to 0x0103 */
   uint16_t Type;           /* Set to PLUGIN_TYPE_GFX */
   char Name[100];          /* Name of the DLL */

   /* If DLL supports memory these memory options then set them to TRUE or FALSE
      if it does not support it */
   int NormalMemory;       /* a normal uint8_t array */
   int MemoryBswaped;      /* a normal uint8_t array where the memory has been pre
                          bswap on a dword (32 bits) boundry */
} PLUGIN_INFO;

extern bool no_dlist;

#endif //_GFX_H_INCLUDED__
