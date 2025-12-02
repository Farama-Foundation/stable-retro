## Makefile for the autoconf-lib-link/tests subdirectory of GNU gettext
## Copyright (C) 1995-1997, 2001-2003, 2006, 2009-2021 Free Software
## Foundation, Inc.
##
## This program is free software: you can redistribute it and/or modify
## it under the terms of the GNU General Public License as published by
## the Free Software Foundation; either version 3 of the License, or
## (at your option) any later version.
##
## This program is distributed in the hope that it will be useful,
## but WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
## GNU General Public License for more details.
##
## You should have received a copy of the GNU General Public License
## along with this program.  If not, see <https://www.gnu.org/licenses/>.

## Process this file with automake to produce Makefile.in.

AUTOMAKE_OPTIONS = 1.11 gnits no-dependencies color-tests
MOSTLYCLEANFILES =

TESTS = rpath-1a rpath-1b \
	rpath-2aaa rpath-2aab            rpath-2aad \
	rpath-2aba rpath-2abb            rpath-2abd \
	rpath-2baa rpath-2bab            rpath-2bad \
	rpath-2bba rpath-2bbb            rpath-2bbd \
	rpath-3aaa rpath-3aab            rpath-3aad \
	                                 rpath-3aah \
	rpath-3aba rpath-3abb            rpath-3abd \
	                                 rpath-3abh \
	rpath-3baa rpath-3bab            rpath-3bad \
	                                 rpath-3bah \
	rpath-3bba rpath-3bbb            rpath-3bbd \
	                                 rpath-3bbh

EXTRA_DIST = $(TESTS) \
	rpathx/configure.ac rpathx/aclocal.m4 rpathx/configure \
	rpathx/Makefile.am rpathx/Makefile.in rpathx/rpathx.c \
	rpathy/configure.ac rpathy/aclocal.m4 rpathy/configure \
	rpathy/Makefile.am rpathy/Makefile.in rpathy/rpathy.c \
	rpathz/configure.ac rpathz/aclocal.m4 rpathz/configure \
	rpathz/Makefile.am rpathz/Makefile.in rpathz/rpathz.c \
	rpathlx/configure.ac rpathlx/aclocal.m4 rpathlx/configure \
	rpathlx/Makefile.am rpathlx/Makefile.in rpathlx/usex.c \
	rpathly/configure.ac rpathly/aclocal.m4 rpathly/configure \
	rpathly/Makefile.am rpathly/Makefile.in rpathly/usey.c \
	rpathlyx/configure.ac rpathlyx/aclocal.m4 rpathlyx/configure \
	rpathlyx/Makefile.am rpathlyx/Makefile.in rpathlyx/usey.c \
	rpathlz/configure.ac rpathlz/aclocal.m4 rpathlz/configure \
	rpathlz/Makefile.am rpathlz/Makefile.in rpathlz/usez.c \
	rpathlzyx/configure.ac rpathlzyx/aclocal.m4 rpathlzyx/configure \
	rpathlzyx/Makefile.am rpathlzyx/Makefile.in rpathlzyx/usez.c \
	rpath-1 rpath-2_a rpath-2_b rpath-3_a rpath-3_b README \
	rpath-3aae rpath-3aag rpath-3abe rpath-3abg \
	rpathcfg.sh

TESTS_ENVIRONMENT = top_srcdir=$(srcdir)/../.. srcdir=$(srcdir) \
		    OBJEXT='@OBJEXT@' EXEEXT='@EXEEXT@' \
		    LIBTOOL='@LIBTOOL@' \
		    CC='@CC@' CFLAGS='@CFLAGS@' \
		    CPPFLAGS='@CPPFLAGS@' LDFLAGS='@LDFLAGS@' \
		    LIBDIRSTEM='@LIBDIRSTEM@' \
		    CONFIG_SHELL='$(SHELL)' \
		    $(SHELL)

# Help maintaining config.rpath.
rpathcfg: rpathcfg.sh
	top_srcdir=$(srcdir)/../.. srcdir=$(srcdir) \
	CONFIG_SHELL='$(SHELL)' \
	CC='@CC@' GCC='@GCC@' \
	LDFLAGS='@LDFLAGS@' \
	LD='@LD@' with_gnu_ld='@with_gnu_ld@' \
	host='@host@' \
	$(SHELL) $(srcdir)/rpathcfg.sh > rpathcfg 2> rpathcfg.log
MOSTLYCLEANFILES += rpathcfg rpathcfg.log

# Clean up the intermediate directory created by running the tests.
distclean-local:
	rm -rf tst
