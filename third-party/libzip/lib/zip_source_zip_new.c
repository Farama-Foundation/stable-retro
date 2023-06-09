/*
  zip_source_zip_new.c -- prepare data structures for zip_fopen/zip_source_zip
  Copyright (C) 2012-2017 Dieter Baron and Thomas Klausner

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


#include <stdlib.h>

#include "zipint.h"


zip_source_t *
_zip_source_zip_new(zip_t *za, zip_t *srcza, zip_uint64_t srcidx, zip_flags_t flags, zip_uint64_t start, zip_uint64_t len, const char *password)
{
    zip_source_t *src, *s2;
    struct zip_stat st;
    bool partial_data, needs_crc, needs_decrypt, needs_decompress;

    if (za == NULL)
	return NULL;

    if (srcza == NULL || srcidx >= srcza->nentry) {
	zip_error_set(&za->error, ZIP_ER_INVAL, 0);
	return NULL;
    }

    if ((flags & ZIP_FL_UNCHANGED) == 0
	&& (ZIP_ENTRY_DATA_CHANGED(srcza->entry+srcidx) || srcza->entry[srcidx].deleted)) {
	zip_error_set(&za->error, ZIP_ER_CHANGED, 0);
	return NULL;
    }

    if (zip_stat_index(srcza, srcidx, flags|ZIP_FL_UNCHANGED, &st) < 0) {
	zip_error_set(&za->error, ZIP_ER_INTERNAL, 0);
	return NULL;
    }

    if (flags & ZIP_FL_ENCRYPTED)
	flags |= ZIP_FL_COMPRESSED;

    if ((start > 0 || len > 0) && (flags & ZIP_FL_COMPRESSED)) {
	zip_error_set(&za->error, ZIP_ER_INVAL, 0);
	return NULL;
    }

    /* overflow or past end of file */
    if ((start > 0 || len > 0) && (start + len < start || start + len > st.size)) {
	zip_error_set(&za->error, ZIP_ER_INVAL, 0);
	return NULL;
    }

    if (len == 0) {
	len = st.size - start;
    }

    partial_data = len < st.size;
    needs_decrypt = ((flags & ZIP_FL_ENCRYPTED) == 0) && (st.encryption_method != ZIP_EM_NONE);
    needs_decompress = ((flags & ZIP_FL_COMPRESSED) == 0) && (st.comp_method != ZIP_CM_STORE);
    /* when reading the whole file, check for CRC errors */
    needs_crc = ((flags & ZIP_FL_COMPRESSED) == 0 || st.comp_method == ZIP_CM_STORE) && !partial_data;

    if (needs_decrypt) {
        if (password == NULL) {
            password = za->default_password;
        }
	if (password == NULL) {
	    zip_error_set(&za->error, ZIP_ER_NOPASSWD, 0);
	    return NULL;
	}
    }

    if (st.comp_size == 0) {
	return zip_source_buffer(za, NULL, 0, 0);
    }

    if (partial_data && !needs_decrypt && !needs_decompress) {
	struct zip_stat st2;

	st2.size = len;
	st2.comp_size = len;
	st2.comp_method = ZIP_CM_STORE;
	st2.mtime = st.mtime;
	st2.valid = ZIP_STAT_SIZE|ZIP_STAT_COMP_SIZE|ZIP_STAT_COMP_METHOD|ZIP_STAT_MTIME;

	if ((src = _zip_source_window_new(srcza->src, start, len, &st2, 0, srcza, srcidx, &za->error)) == NULL) {
	    return NULL;
	}
    }
    else {
	zip_dirent_t *de;

	if ((de = _zip_get_dirent(srcza, srcidx, flags, &za->error)) == NULL) {
	    return NULL;
	}
	if ((src = _zip_source_window_new(srcza->src, 0, st.comp_size, &st, (de->bitflags >> 1) & 3, srcza, srcidx, &za->error)) == NULL) {
	    return NULL;
	}
    }

    if (_zip_source_set_source_archive(src, srcza) < 0) {
	zip_source_free(src);
	return NULL;
    }

    /* creating a layered source calls zip_keep() on the lower layer, so we free it */

    if (needs_decrypt) {
	zip_encryption_implementation enc_impl;

	if ((enc_impl = _zip_get_encryption_implementation(st.encryption_method, ZIP_CODEC_DECODE)) == NULL) {
	    zip_error_set(&za->error, ZIP_ER_ENCRNOTSUPP, 0);
	    return NULL;
	}

	s2 = enc_impl(za, src, st.encryption_method, 0, password);
	zip_source_free(src);
	if (s2 == NULL) {
	    return NULL;
	}
	src = s2;
    }
    if (needs_decompress) {
	s2 = zip_source_decompress(za, src, st.comp_method);
	zip_source_free(src);
	if (s2 == NULL) {
	    return NULL;
	}
	src = s2;
    }
    if (needs_crc) {
	s2 = zip_source_crc(za, src, 1);
	zip_source_free(src);
	if (s2 == NULL) {
	    return NULL;
	}
	src = s2;
    }

    if (partial_data && (needs_decrypt || needs_decompress)) {
	s2 = zip_source_window(za, src, start, len);
	zip_source_free(src);
	if (s2 == NULL) {
	    return NULL;
	}
	src = s2;
    }

    return src;
}
