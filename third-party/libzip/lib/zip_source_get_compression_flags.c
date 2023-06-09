/*
  zip_source_get_compression_flags.c -- get compression flags for entry
  Copyright (C) 2017 Dieter Baron and Thomas Klausner

  This file is part of libzip, a library to manipulate ZIP archives.
  The authors can be contacted at <libzip@nih.at>

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions
  are met:
  1. Redistributions of source code must retain the above copyright
     notice, this list of conditions and the following disclaimer.
  2. Redistributions in binary form must reproduce the above copyright
     notice, this list of conditions and the following disclaimer in
     the documentation and/or other materials provided with the
     distribution.
  3. The names of the authors may not be used to endorse or promote
     products derived from this software without specific prior
     written permission.

  THIS SOFTWARE IS PROVIDED BY THE AUTHORS ``AS IS'' AND ANY EXPRESS
  OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
  WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
  ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY
  DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
  DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
  GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
  IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
  OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
  IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/


#include "zipint.h"

#define ZIP_COMPRESSION_BITFLAG_MAX	3

zip_int8_t
zip_source_get_compression_flags(zip_source_t *src)
{
    while (src) {
	if ((src->supports & ZIP_SOURCE_MAKE_COMMAND_BITMASK(ZIP_SOURCE_GET_COMPRESSION_FLAGS))) {
	    zip_int64_t ret = _zip_source_call(src, NULL, 0, ZIP_SOURCE_GET_COMPRESSION_FLAGS);
	    if (ret < 0) {
		return -1;
	    }
	    if (ret > ZIP_COMPRESSION_BITFLAG_MAX) {
		zip_error_set(&src->error, ZIP_ER_INTERNAL, 0);
		return -1;
	    }
	    return (zip_int8_t)ret;
	}
	src = src->src;
    }

    return 0;
}
