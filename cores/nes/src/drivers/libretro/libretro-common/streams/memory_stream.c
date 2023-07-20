/* Copyright  (C) 2010-2016 The RetroArch team
 *
 * ---------------------------------------------------------------------------------------
 * The following license statement only applies to this file (memory_stream.c).
 * ---------------------------------------------------------------------------------------
 *
 * Permission is hereby granted, free of charge,
 * to any person obtaining a copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../include/streams/memory_stream.h"

static uint8_t* g_buffer     = NULL;
static size_t g_size         = 0;
static size_t last_file_size = 0;

struct memstream
{
   uint8_t *buf;
   size_t size;
   size_t ptr;
   size_t max_ptr;
   unsigned writing;
};

static void memstream_update_pos(memstream_t *stream)
{
   if (stream->ptr > stream->max_ptr)
      stream->max_ptr = stream->ptr;
}

void memstream_set_buffer(uint8_t *buffer, size_t size)
{
   g_buffer = buffer;
   g_size = size;
}

size_t memstream_get_last_size(void)
{
   return last_file_size;
}

static void memstream_init(memstream_t *stream,
      uint8_t *buffer, size_t max_size, unsigned writing)
{
   if (!stream)
      return;

   stream->buf     = buffer;
   stream->size    = max_size;
   stream->ptr     = 0;
   stream->max_ptr = 0;
   stream->writing = writing;
}

memstream_t *memstream_open(unsigned writing)
{
	memstream_t *stream;
   if (!g_buffer || !g_size)
      return NULL;

   stream = (memstream_t*)calloc(1, sizeof(*stream));
   memstream_init(stream, g_buffer, g_size, writing);

   g_buffer = NULL;
   g_size = 0;
   return stream;
}

void memstream_close(memstream_t *stream)
{
   if (!stream)
      return;

   last_file_size = stream->writing ? stream->max_ptr : stream->size;
   free(stream);
}

size_t memstream_read(memstream_t *stream, void *data, size_t bytes)
{
   size_t avail = 0;

   if (!stream)
      return 0;

   avail = stream->size - stream->ptr;
   if (bytes > avail)
      bytes = avail;

   memcpy(data, stream->buf + stream->ptr, bytes);
   stream->ptr += bytes;
   memstream_update_pos(stream);
   return bytes;
}

size_t memstream_write(memstream_t *stream, const void *data, size_t bytes)
{
   size_t avail = 0;

   if (!stream)
      return 0;

   avail = stream->size - stream->ptr;
   if (bytes > avail)
      bytes = avail;

   memcpy(stream->buf + stream->ptr, data, bytes);
   stream->ptr += bytes;
   memstream_update_pos(stream);
   return bytes;
}

int memstream_seek(memstream_t *stream, int offset, int whence)
{
   size_t ptr;

   switch (whence)
   {
      case SEEK_SET:
         ptr = offset;
         break;
      case SEEK_CUR:
         ptr = stream->ptr + offset;
         break;
      case SEEK_END:
         ptr = (stream->writing ? stream->max_ptr : stream->size) + offset;
         break;
      default:
         return -1;
   }

   if (ptr <= stream->size)
   {
      stream->ptr = ptr;
      return 0;
   }

   return -1;
}

void memstream_rewind(memstream_t *stream)
{
   memstream_seek(stream, 0L, SEEK_SET);
}

size_t memstream_pos(memstream_t *stream)
{
   return stream->ptr;
}

char *memstream_gets(memstream_t *stream, char *buffer, size_t len)
{
   return NULL;
}

int memstream_getc(memstream_t *stream)
{
   int ret = 0;
   if (stream->ptr >= stream->size)
      return EOF;
   ret = stream->buf[stream->ptr++];

   memstream_update_pos(stream);

   return ret;
}

void memstream_putc(memstream_t *stream, int c)
{
   if (stream->ptr < stream->size)
      stream->buf[stream->ptr++] = c;

   memstream_update_pos(stream);
}
