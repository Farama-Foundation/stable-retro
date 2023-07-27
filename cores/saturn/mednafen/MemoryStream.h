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

// TODO/WIP

#ifndef _MEMORY_STREAM_H
#define _MEMORY_STREAM_H

#include "Stream.h"

class MemoryStream : public Stream
{
 public:

 MemoryStream();
 MemoryStream(uint64 size_hint);
 MemoryStream(Stream *stream);	// Will create a MemoryStream equivalent of the contents of "stream", and then "delete stream".
				// Will only work if stream->tell() == 0, or if "stream" is seekable.
				// stream will be deleted even if this constructor throws.

 MemoryStream(const MemoryStream *zs);
 MemoryStream & operator=(const MemoryStream *zs);

 virtual ~MemoryStream();

 virtual uint8 *map(void);
 virtual void unmap(void);

 virtual uint64 read(void *data, uint64 count);
 virtual void write(const void *data, uint64 count);
 virtual void seek(int64 offset, int whence);
 virtual void truncate(uint64_t length);
 virtual void flush(void);
 virtual uint64_t tell(void);
 virtual uint64_t size(void);
 virtual void close(void);

 virtual int get_line(std::string &str);

 private:
 uint8 *data_buffer;
 uint64 data_buffer_size;
 uint64 data_buffer_alloced;

 int64 position;

 void grow_if_necessary(uint64 new_required_size);
};

#endif
