#!/bin/sh
# Prints information for maintaining config.rpath.

# The caller should set the environment variables
# top_srcdir, srcdir, CONFIG_SHELL, CC, GCC, LDFLAGS, LD, LIBDIRSTEM, with_gnu_ld, host.

if test -f $top_srcdir/build-aux/config.rpath; then
  build_aux_dir=$top_srcdir/build-aux
else
  build_aux_dir=$top_srcdir/../build-aux
fi

echo "=============== rpathcfg for $host ==============="
echo
echo "--------------- config.rpath output ---------------"
${CONFIG_SHELL-/bin/sh} $build_aux_dir/config.rpath "$host" | sed -e 's/^acl_cv_//'
echo "--------------- experimentally determined ---------------"
builddir=`pwd`
global_top_auxdir=`cd "$top_srcdir"/build-aux && pwd`
export global_top_auxdir

tstdir='tst/dir'
test -d $tstdir || { mkdir tst && mkdir $tstdir; }

rm -rf tstprefix $tstdir/build tstlib

eval `${CONFIG_SHELL-/bin/sh} $build_aux_dir/config.rpath "$host" | grep '^acl_cv_wl='`
wl="$acl_cv_wl"

# Static library suffix (normally "a").
mkdir tstprefix
mkdir $tstdir/build
(cd $srcdir/rpathx && tar cf - *) | (cd $tstdir/build && tar xf -)
(cd $tstdir/build
 ${CONFIG_SHELL-/bin/sh} ./configure --disable-shared --prefix=$builddir/tstprefix --libdir=$builddir/tstprefix/${LIBDIRSTEM-lib} > configure.log 2>&1
 make > make.log 2>&1
 make install > install.log 2>&1
)
rm -rf $tstdir/build
libext=
cd tstprefix/lib
for f in *; do
  case $f in
    *.la) ;;
    *[0-9]) ;;
    *) libext=`echo $f | sed -e 's/^.*\.//'`;;
  esac
  if test -n "$libext"; then
    break
  fi
done
cd ../..
rm -rf tstprefix

# Shared library suffix (normally "so").
mkdir tstprefix
mkdir $tstdir/build
(cd $srcdir/rpathx && tar cf - *) | (cd $tstdir/build && tar xf -)
(cd $tstdir/build
 ${CONFIG_SHELL-/bin/sh} ./configure --disable-static --prefix=$builddir/tstprefix --libdir=$builddir/tstprefix/${LIBDIRSTEM-lib} > configure.log 2>&1
 make > make.log 2>&1
 make install > install.log 2>&1
)
rm -rf $tstdir/build
shlibext=
cd tstprefix/lib || exit 1
for f in *; do
  case $f in
    *.la) ;;
    *[0-9]) ;;
    *) shlibext=`echo $f | sed -e 's/^.*\.//'`;;
  esac
  if test -n "$shlibext"; then
    break
  fi
done
cd ../..
rm -rf tstprefix

# Prepare hardcoding tests.
mkdir tstprefix
mkdir $tstdir/build
(cd $srcdir/rpathx && tar cf - *) | (cd $tstdir/build && tar xf -)
(cd $tstdir/build
 ${CONFIG_SHELL-/bin/sh} ./configure --disable-static --prefix=$builddir/tstprefix --libdir=$builddir/tstprefix/${LIBDIRSTEM-lib} > configure.log 2>&1
 make > make.log 2>&1
 make install > install.log 2>&1
)
rm -rf $tstdir/build

# Flag to hardcode \$libdir into a binary during linking.
# This must work even if \$libdir does not exist.
hardcode_libdir_flag_spec=
for spec in \
    '-L$libdir' \
    '-R$libdir' \
    '-rpath $libdir' '${wl}-rpath ${wl}$libdir' \
    '${wl}+b ${wl}$libdir' \
    '${wl}-R $libdir:/usr/lib:/lib' \
    '${wl}-blibpath:$libdir:/usr/lib:/lib' \
    '${wl}-bnolibpath ${wl}-blibpath:$libdir:/usr/lib:/lib' \
  ; do
  mv tstprefix/lib tstlib
  libdir=`pwd`/tstprefix/lib
  eval flag=\"$spec\"
  echo 1>&2
  echo "$CC $LDFLAGS $srcdir/rpathlx/usex.c tstlib/librpathx.$shlibext $flag -o a.out" 1>&2
  $CC $LDFLAGS $srcdir/rpathlx/usex.c tstlib/librpathx.$shlibext $flag -o a.out
  if test $? = 0; then
    mv tstlib tstprefix/lib
    echo "ok, running created a.out." 1>&2
    if ./a.out; then
      hardcode_libdir_flag_spec="$hardcode_libdir_flag_spec$spec
"
    fi
  else
    mv tstlib tstprefix/lib
  fi
  rm -f a.out
done

# Whether we need a single -rpath flag with a separated argument.
hardcode_libdir_separator=
if test -n "$hardcode_libdir_flag_spec"; then
  spec=`echo "$hardcode_libdir_flag_spec" | sed -e '2,$d'`
  # Try with multiple -rpath flags.
  mv tstprefix/lib tstlib
  libdir=`pwd`/tstprefix/lib
  eval flag1=\"$spec\"
  libdir=/tmp
  eval flag2=\"$spec\"
  echo 1>&2
  echo "$CC $LDFLAGS $srcdir/rpathlx/usex.c tstlib/librpathx.$shlibext $flag1 $flag2 -o a.out" 1>&2
  $CC $LDFLAGS $srcdir/rpathlx/usex.c tstlib/librpathx.$shlibext $flag1 $flag2 -o a.out
  if test $? = 0; then
    mv tstlib tstprefix/lib
    echo "ok, running created a.out." 1>&2
    if ./a.out; then
      hardcode_libdir_separator=NONE
    fi
  else
    mv tstlib tstprefix/lib
  fi
  rm -f a.out
  if test -z "$hardcode_libdir_separator"; then
    # Try with a single -rpath flag.
    mv tstprefix/lib tstlib
    libdir=`pwd`/tstprefix/lib:/tmp
    eval flag=\"$spec\"
    echo 1>&2
    echo "$CC $LDFLAGS $srcdir/rpathlx/usex.c tstlib/librpathx.$shlibext $flag -o a.out" 1>&2
    $CC $LDFLAGS $srcdir/rpathlx/usex.c tstlib/librpathx.$shlibext $flag -o a.out
    if test $? = 0; then
      mv tstlib tstprefix/lib
      echo "ok, running created a.out." 1>&2
      if ./a.out; then
        hardcode_libdir_separator=:
      fi
    else
      mv tstlib tstprefix/lib
    fi
    rm -f a.out
  fi
  if test -z "$hardcode_libdir_separator"; then
    echo "hardcode_libdir_separator test failed!" 1>&2
  else
    if test "$hardcode_libdir_separator" = NONE; then
      hardcode_libdir_separator=
    fi
  fi
fi

# Set to yes if using DIR/libNAME.so during linking hardcodes DIR into the
# resulting binary.
hardcode_direct=no
echo 1>&2
echo "$CC $LDFLAGS $srcdir/rpathlx/usex.c tstprefix/lib/librpathx.$shlibext -o a.out" 1>&2
$CC $LDFLAGS $srcdir/rpathlx/usex.c tstprefix/lib/librpathx.$shlibext -o a.out
if test $? = 0; then
  echo "ok, running created a.out." 1>&2
  if ./a.out; then
    hardcode_direct=yes
  fi
fi
rm -f a.out

# Set to yes if using the -LDIR flag during linking hardcodes DIR into the
# resulting binary.
hardcode_minus_L=no
echo 1>&2
echo "$CC $LDFLAGS $srcdir/rpathlx/usex.c -Ltstprefix/lib -lrpathx -o a.out" 1>&2
$CC $LDFLAGS $srcdir/rpathlx/usex.c -Ltstprefix/lib -lrpathx -o a.out
if test $? = 0; then
  echo "ok, running created a.out." 1>&2
  if ./a.out; then
    hardcode_minus_L=yes
  fi
fi
rm -f a.out

# Clean up.
rm -rf tstprefix $tstdir

sed_quote_subst='s/\(["`$\\]\)/\\\1/g'
escaped_hardcode_libdir_flag_spec=`echo "X$hardcode_libdir_flag_spec" | sed -e 's/^X//' -e "$sed_quote_subst"`
escaped_sys_lib_search_path_spec=`echo "X$sys_lib_search_path_spec" | sed -e 's/^X//' -e "$sed_quote_subst"`
escaped_sys_lib_dlsearch_path_spec=`echo "X$sys_lib_dlsearch_path_spec" | sed -e 's/^X//' -e "$sed_quote_subst"`

cat <<EOF

# Static library suffix (normally "a").
libext="$libext"

# Shared library suffix (normally "so").
shlibext="$shlibext"

# Flag to hardcode \$libdir into a binary during linking.
# This must work even if \$libdir does not exist.
hardcode_libdir_flag_spec="$escaped_hardcode_libdir_flag_spec"

# Whether we need a single -rpath flag with a separated argument.
hardcode_libdir_separator="$hardcode_libdir_separator"

# Set to yes if using DIR/libNAME.so during linking hardcodes DIR into the
# resulting binary.
hardcode_direct="$hardcode_direct"

# Set to yes if using the -LDIR flag during linking hardcodes DIR into the
# resulting binary.
hardcode_minus_L="$hardcode_minus_L"

EOF
