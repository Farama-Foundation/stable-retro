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
#include <stdarg.h>

#include <sys/types.h>

#include <string>

#include <boolean.h>

#include "mednafen.h"
#include "general.h"
#include "state.h"

#include "hash/md5.h"

using namespace std;

static bool IsAbsolutePath(const char *path)
{
   if (
#ifdef _WIN32
         path[0] == '\\' ||
#endif
         path[0] == '/'
      )
         return true;

#if defined(_WIN32) || defined(DOS)
   if((path[0] >= 'a' && path[0] <= 'z') || (path[0] >= 'A' && path[0] <= 'Z'))
   {
      if(path[1] == ':')
         return true;
   }
#endif

 return(false);
}

bool MDFN_IsFIROPSafe(const std::string &path)
{
 // We could make this more OS-specific, but it shouldn't hurt to try to weed out usage of characters that are path
 // separators in one OS but not in another, and we'd also run more of a risk of missing a special path separator case
 // in some OS.

 if(!MDFN_GetSettingB("filesys.untrusted_fip_check"))
  return(true);

 if(path.find('\0') != string::npos)
  return(false);

 if(path.find(':') != string::npos)
  return(false);

 if(path.find('\\') != string::npos)
  return(false);

 if(path.find('/') != string::npos)
  return(false);

 return(true);
}

void MDFN_GetFilePathComponents(const std::string &file_path, 
      std::string *dir_path_out, std::string *file_base_out, 
      std::string *file_ext_out)
{
 size_t final_ds;		                  // in file_path
 string file_name;
 size_t fn_final_dot;		            // in local var file_name
 string dir_path, file_base, file_ext; // Temporary output

#ifdef _WIN32
 final_ds = file_path.find_last_of('\\');

 size_t alt_final_ds = file_path.find_last_of('/');

 if(final_ds == string::npos || (alt_final_ds != string::npos && alt_final_ds > final_ds))
    final_ds = alt_final_ds;
#else
 final_ds = file_path.find_last_of('/');
#endif

 if(final_ds == string::npos)
 {
  dir_path = string(".");
  file_name = file_path;
 }
 else
 {
  dir_path = file_path.substr(0, final_ds);
  file_name = file_path.substr(final_ds + 1);
 }

 fn_final_dot = file_name.find_last_of('.');

 if(fn_final_dot != string::npos)
 {
  file_base = file_name.substr(0, fn_final_dot);
  file_ext = file_name.substr(fn_final_dot);
 }
 else
 {
  file_base = file_name;
  file_ext = string("");
 }

 if(dir_path_out)
  *dir_path_out = dir_path;

 if(file_base_out)
  *file_base_out = file_base;

 if(file_ext_out)
  *file_ext_out = file_ext;
}

std::string MDFN_EvalFIP(const std::string &dir_path, const std::string &rel_path, bool skip_safety_check)
{
   char slash;
#ifdef _WIN32
   slash = '\\';
#else
   slash = '/';
#endif

   if(!skip_safety_check && !MDFN_IsFIROPSafe(rel_path))
      throw MDFN_Error(0, _("Referenced path \"%s\" is potentially unsafe.  See \"filesys.untrusted_fip_check\" setting.\n"), rel_path.c_str());

   if(IsAbsolutePath(rel_path.c_str()))
      return(rel_path);
   else
      return(dir_path + slash + rel_path);
}

const char * GetFNComponent(const char *str)
{
   const char *tp1;

#ifdef _WIN32
   tp1 = ((char *)strrchr(str,'\\'));

   const char *tp3;

   tp3 = ((char *)strrchr(str,'/'));

   if (tp1<tp3)
      tp1 = tp3;
#else
   tp1 = ((char *)strrchr(str,'/'));
#endif

   if (tp1)
      return (tp1+1);
   else
      return (str);
}

// Remove whitespace from beginning of string
void MDFN_ltrim(std::string &string)
{
 size_t len = string.length();
 size_t di, si;
 bool InWhitespace = true;

 di = si = 0;

 while(si < len)
 {
  if(InWhitespace && (string[si] == ' ' || string[si] == '\r' || string[si] == '\n' || string[si] == '\t' || string[si] == 0x0b))
  {

  }
  else
  {
   InWhitespace = false;
   string[di] = string[si];
   di++;
  }
  si++;
 }

 string.resize(di);
}

// Remove whitespace from end of string
void MDFN_rtrim(std::string &string)
{
 size_t len = string.length();

 if(len)
 {
  size_t x = len;
  size_t new_len = len;

  do
  {
   x--;

   if(!(string[x] == ' ' || string[x] == '\r' || string[x] == '\n' || string[x] == '\t' || string[x] == 0x0b))
    break;
 
   new_len--;
  } while(x);

  string.resize(new_len);
 }
}


void MDFN_trim(std::string &string)
{
 MDFN_rtrim(string);
 MDFN_ltrim(string);
}

