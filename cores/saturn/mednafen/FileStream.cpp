// TODO/WIP

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

#include "mednafen.h"
#include "Stream.h"
#include "FileStream.h"

#include <stdarg.h>
#include <string.h>
#include <libretro.h>

FileStream::FileStream(const char *path, const int mode)
{
   fp = filestream_open(path, (mode == MODE_WRITE || mode == MODE_WRITE_INPLACE) ? RETRO_VFS_FILE_ACCESS_WRITE : RETRO_VFS_FILE_ACCESS_READ, RETRO_VFS_FILE_ACCESS_HINT_NONE);
}

FileStream::~FileStream()
{
   if (fp)
   {
      filestream_close(fp);
      fp = NULL;
   }
}

uint64_t FileStream::read(void *data, uint64_t count)
{
   if (!fp)
      return 0;
   return filestream_read(fp, data, count);
}

void FileStream::write(const void *data, uint64_t count)
{
   if (!fp)
      return;
   filestream_write(fp, data, count);
}

void FileStream::seek(int64_t offset, int whence)
{
   int seek_position = -1;
   if (!fp)
      return;
   switch (whence)
   {
      case SEEK_SET:
         seek_position = RETRO_VFS_SEEK_POSITION_START;
         break;
      case SEEK_CUR:
         seek_position = RETRO_VFS_SEEK_POSITION_CURRENT;
         break;
      case SEEK_END:
         seek_position = RETRO_VFS_SEEK_POSITION_END;
         break;
   }
   filestream_seek(fp, offset, seek_position);
}

uint64_t FileStream::tell(void)
{
   if (!fp)
      return -1;
   return filestream_tell(fp);
}

uint64_t FileStream::size(void)
{
   if (!fp)
      return -1;
   return filestream_get_size(fp);
}

void FileStream::truncate(uint64_t length)
{
}

void FileStream::flush(void)
{
}

void FileStream::close(void)
{
   if (!fp)
      return;
   filestream_close(fp);
   fp = NULL;
}
