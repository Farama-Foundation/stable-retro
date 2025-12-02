# Generate argz.c and argz.in.h from glibc sources.

glibc_dir = ../glibc
glibc_dir = /mirror/d/glibc

argz_names = \
  append addsep ctsep insert next stringify count \
  extract create delete replace
argz_files = $(patsubst %, $(glibc_dir)/string/argz-%.c, $(argz_names))

define print-header
  printf '%s\n'								\
"/* Functions for dealing with '\0' separated arg vectors."		\
"   Copyright (C) 1995-1998, 2000-2002, 2006, 2009-2021 Free Software
"   Foundation, Inc."\
"   This file is part of the GNU C Library."				\
""									\
"   This program is free software; you can redistribute it and/or modify"\
"   it under the terms of the GNU General Public License as published by"\
"   the Free Software Foundation; either version 2, or (at your option)"\
"   any later version."							\
""									\
"   This program is distributed in the hope that it will be useful,"	\
"   but WITHOUT ANY WARRANTY; without even the implied warranty of"	\
"   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the"	\
"   GNU General Public License for more details."			\
""									\
"   You should have received a copy of the GNU General Public License along"\
"   with this program; if not, see <https://www.gnu.org/licenses/>.  */"	\
""									\
"#include <config.h>"							\
""									\
"#include <argz.h>"							\
"#include <errno.h>"							\
"#include <stdlib.h>"							\
"#include <string.h>"
endef

targets = argz.c argz.in.h

all: $(targets)

argz.c: $(argz_files)
	($(print-header);						\
	 for i in $^; do						\
	   perl -pe 's/__(argz_|st|mem)/$$1/g' $$i			\
	     | perl -0x0 -pe 's,/\*(.|\n)+?\*/\n,,'			\
	     | grep -vE '^(#include|INTDEF|weak_alias|libc_hidden_def)'; \
	 done) > $@-t && mv $@-t $@

argz.in.h: $(glibc_dir)/string/argz.h
	perl -pe 's/__(restrict|const|st|mem)/$$1/g;'			\
	    -e 's/\s*__THROW//;'					\
	    -e 's/\s*__attribute_pure__//;'				\
	  $<								\
	  | perl -ne							\
	    '/^(#include <features\.h>|__(?:BEGIN|END)_DECLS)/ or print' \
	  | perl -0x3b -pe 's/extern \S+ \*?__argz_(.|\n)*?\)\n*;//'	\
	  | perl -pe 's/__(argz_next)/$$1/g;'				\
	  > $@-t && mv $@-t $@

clean:
	rm -f $(targets)
