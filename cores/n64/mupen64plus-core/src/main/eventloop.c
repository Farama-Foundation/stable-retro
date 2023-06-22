/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *   Mupen64plus - eventloop.c                                             *
 *   Mupen64Plus homepage: http://code.google.com/p/mupen64plus/           *
 *   Copyright (C) 2008-2009 Richard Goedeken                              *
 *   Copyright (C) 2008 Ebenblues Nmn Okaygo Tillin9                       *
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

#include <stdlib.h>

#define M64P_CORE_PROTOTYPES 1
#include "main.h"
#include "eventloop.h"
#include "util.h"
#include "api/callbacks.h"
#include "api/config.h"
#include "api/m64p_config.h"
#include "plugin/plugin.h"
#include "r4300/reset.h"

/* version number for CoreEvents config section */
#define CONFIG_PARAM_VERSION 1.00

static int GamesharkActive = 0;

int event_gameshark_active(void)
{
    return GamesharkActive;
}

void event_set_gameshark(int active)
{
   /* if boolean value doesn't change then just return */
   if (!active == !GamesharkActive)
      return;

   /* set the button state */
   GamesharkActive = 0;

   if (active)
      GamesharkActive = 1;

   /* notify front-end application that 
    * gameshark button state has changed */
   StateChanged(M64CORE_INPUT_GAMESHARK, GamesharkActive);
}

