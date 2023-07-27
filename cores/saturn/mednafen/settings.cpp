/* Mednafen - Multi-system Emulator
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
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

#include <string.h>

#include <string>

#include "mednafen.h"
#include "settings.h"
#include <compat/msvc.h>
#include "libretro_settings.h"

extern char retro_base_directory[4096];

uint64_t MDFN_GetSettingUI(const char *name)
{
   if (!strcmp("ss.smpc.autortc.lang", name))
      return setting_smpc_autortc_lang;
   return 0;
}

int64 MDFN_GetSettingI(const char *name)
{
   if (!strcmp("ss.slstart", name))
      return setting_initial_scanline;
   if (!strcmp("ss.slstartp", name))
      return setting_initial_scanline_pal;
   if (!strcmp("ss.slend", name))
      return setting_last_scanline;
   if (!strcmp("ss.slendp", name))
      return setting_last_scanline_pal;
   return 0;
}

bool MDFN_GetSettingB(const char *name)
{
   /* LIBRETRO */
   if (!strcmp("ss.smpc.autortc", name))
      return int(setting_smpc_autortc);
   if (!strcmp("ss.bios_sanity", name))
      return true;
   /* FILESYS */
   if (!strcmp("filesys.untrusted_fip_check", name))
      return 0;
   return 0;
}

std::string MDFN_GetSettingS(const char *name)
{
   if (!strcmp("ss.cart.kof95_path", name))
      return std::string("mpr-18811-mx.ic1");
   if (!strcmp("ss.cart.ultraman_path", name))
      return std::string("mpr-19367-mx.ic1");
   if (!strcmp("ss.cart.satar4mp_path", name))
      return std::string("satar4mp.bin");
   return 0;
}
