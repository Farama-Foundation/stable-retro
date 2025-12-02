#!/bin/sh
# Adds the reference to the Glibc documentation and to the Linux manual page
# to the file foo.texi.
# Usage: add-links.sh foo.texi

case "$1" in
  *.texi)
    f="$1"
    g="${f%.texi}"
    glibc_inforef=
    glibc_link=
    if test -f _index.html \
       || { wget -O _index1.html https://www.gnu.org/software/libc/manual/html_node/Function-Index.html \
            && wget -O _index2.html https://www.gnu.org/software/libc/manual/html_node/Variable-Index.html \
            && cat _index1.html _index2.html > _index.html; \
          }; then
      glibc_page=`sed -n -e "s|^.*<a href=\"\([^\"]*\)\"><code>$g</code></a>.*|\1|p" < _index.html | sed -s 's/#.*//'`
      if test -n "$glibc_page"; then
        glibc_page_title=`wget -O - "https://www.gnu.org/software/libc/manual/html_node/$glibc_page" 2>/dev/null | sed -n -e 's/^.*<h[1-4][^>]*>[0-9. ]*\(.*\)<\/h.*/\1/p' | sed -e 's|<[^>]*>||g' -e 's|,||g' | head -n 1`
        if test -n "$glibc_page_title"; then
          glibc_node=`echo "$glibc_page" | sed -e 's/\.html$//' -e 's/_/\\\\u/g' -e 's/\\u00\([0-7]\)/\\x\1/g' -e 's/-/ /g'`
          # Avoid the broken built-in 'printf' in the dash shell.
          glibc_node=`/usr/bin/printf "$glibc_node" 2>/dev/null || printf "$glibc_node"`
          glibc_inforef="@ref{$glibc_node,,$glibc_page_title,libc}"
          glibc_link="@url{https://www.gnu.org/software/libc/manual/html_node/$glibc_page}"
        else
          echo "failed to determine page title of https://www.gnu.org/software/libc/manual/html_node/$glibc_page"
        fi
      else
        echo $f not documented in glibc manual
      fi
    else
      echo "failed to fetch glibc index"
    fi
    manpages_link=
    if wget https://www.kernel.org/doc/man-pages/online/pages/man3/${g}.3.html >/dev/null 2>&1; then
      manpages_link="@uref{https://www.kernel.org/doc/man-pages/online/pages/man3/${g}.3.html,,man ${g}}"
    else
      if wget https://www.kernel.org/doc/man-pages/online/pages/man2/${g}.2.html >/dev/null 2>&1; then
        manpages_link="@uref{https://www.kernel.org/doc/man-pages/online/pages/man2/${g}.2.html,,man ${g}}"
      else
        echo $f not documented among the man pages
      fi
    fi
    if test -n "$glibc_link"; then
      if test -n "$manpages_link"; then
        documentation="Documentation:\n\
@itemize\n\
@item\n\
@ifinfo\n\
$glibc_inforef,\n\
@end ifinfo\n\
@ifnotinfo\n\
$glibc_link,\n\
@end ifnotinfo\n\
@item\n\
$manpages_link.\n\
@end itemize"
      else
        documentation="Documentation:@*\n\
@ifinfo\n\
$glibc_inforef.\n\
@end ifinfo\n\
@ifnotinfo\n\
$glibc_link.\n\
@end ifnotinfo"
      fi
    else
      if test -n "$manpages_link"; then
        documentation="Documentation:@* $manpages_link"
      else
        documentation=
      fi
    fi
    if test -n "$documentation"; then
      mv ${f} ${f}~
      if cat ${f}~ | sed -e '/^Documentation:/,/^$/d' | sed -e "s|^Gnulib module|$documentation\n\
\n\
Gnulib module|" > ${f}; then
        echo $f updated
      else
        echo $f update failed
        mv ${f}~ ${f}
      fi
    fi
    ;;
  *) echo "invalid argument: $1" 1>&2 ;;
esac
