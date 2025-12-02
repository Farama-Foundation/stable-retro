#!/bin/sh
#
# Copyright (C) 2002-2021 Free Software Foundation, Inc.
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>.
#

# Usage: MODULES.html.sh [--git-urls] > MODULES.html

# Extend the PATH so that gnulib-tool is found.
PATH=`dirname "$0"`:$PATH; export PATH

POSIX2001_URL='https://pubs.opengroup.org/onlinepubs/009695399'
POSIX2008_URL='https://pubs.opengroup.org/onlinepubs/9699919799'

repo_url_prefix=
repo_url_suffix=
if test $# != 0; then
  case "$1" in
    --git-urls)
      # Generate URLs to the official gnulib git repository.
      repo_url_prefix='https://git.sv.gnu.org/gitweb/?p=gnulib.git;a=blob_plain;f='
      repo_url_suffix=''
      ;;
  esac
fi
# For sed replacements: Escape the '&'.
repo_url_suffix_repl=`echo "$repo_url_suffix" | sed -e 's,[&],\\\&,'`

sed_lt='s,<,\&lt;,g'
sed_gt='s,>,\&gt;,g'
sed_escape_dot='s,\.,\\.,g'
sed_escape_slash='s,/,\\/,g'
trnl='\012'
sed_alt1='s,$,\\|,'
sed_alt2='s,^\\|,\\(,'
sed_alt3='s,\\|\\|$,\\),'
posix_headers=`echo '
aio
arpa/inet
assert
complex
cpio
ctype
dirent
dlfcn
errno
fcntl
fenv
float
fmtmsg
fnmatch
ftw
glob
grp
iconv
inttypes
iso646
langinfo
libgen
limits
locale
math
monetary
mqueue
ndbm
net/if
netdb
netinet/in
netinet/tcp
nl_types
poll
pthread
pwd
regex
sched
search
semaphore
setjmp
signal
spawn
stdarg
stdbool
stddef
stdint
stdio
stdlib
string
strings
stropts
sys/ipc
sys/mman
sys/msg
sys/resource
sys/select
sys/sem
sys/shm
sys/socket
sys/stat
sys/statvfs
sys/time
sys/times
sys/types
sys/uio
sys/un
sys/utsname
sys/wait
syslog
tar
termios
tgmath
time
trace
ulimit
unistd
utime
utmpx
wchar
wctype
wordexp
' | sed -e "$sed_alt1" | tr -d "$trnl" | sed -e "$sed_alt2" -e "$sed_alt3"`
posix2001_headers=`echo '
sys/timeb
ucontext
' | sed -e "$sed_alt1" | tr -d "$trnl" | sed -e "$sed_alt2" -e "$sed_alt3"`
posix_functions=`echo '
FD_CLR
FD_ISSET
FD_SET
FD_ZERO
_Exit
_exit
_longjmp
_setjmp
_tolower
_toupper
a64l
abort
abs
accept
access
acos
acosf
acosh
acoshf
acoshl
acosl
aio_cancel
aio_error
aio_fsync
aio_read
aio_return
aio_suspend
aio_write
alarm
alphasort
asctime
asctime_r
asin
asinf
asinh
asinhf
asinhl
asinl
assert
atan
atan2
atan2f
atan2l
atanf
atanh
atanhf
atanhl
atanl
atexit
atof
atoi
atol
atoll
basename
bind
bsearch
btowc
cabs
cabsf
cabsl
cacos
cacosf
cacosh
cacoshf
cacoshl
cacosl
calloc
carg
cargf
cargl
casin
casinf
casinh
casinhf
casinhl
casinl
catan
catanf
catanh
catanhf
catanhl
catanl
catclose
catgets
catopen
cbrt
cbrtf
cbrtl
ccos
ccosf
ccosh
ccoshf
ccoshl
ccosl
ceil
ceilf
ceill
cexp
cexpf
cexpl
cfgetispeed
cfgetospeed
cfsetispeed
cfsetospeed
chdir
chmod
chown
cimag
cimagf
cimagl
clearerr
clock
clock_getcpuclockid
clock_getres
clock_gettime
clock_nanosleep
clock_settime
clog
clogf
clogl
close
closedir
closelog
confstr
conj
conjf
conjl
connect
copysign
copysignf
copysignl
cos
cosf
cosh
coshf
coshl
cosl
cpow
cpowf
cpowl
cproj
cprojf
cprojl
creal
crealf
creall
creat
crypt
csin
csinf
csinh
csinhf
csinhl
csinl
csqrt
csqrtf
csqrtl
ctan
ctanf
ctanh
ctanhf
ctanhl
ctanl
ctermid
ctime
ctime_r
daylight
dbm_clearerr
dbm_close
dbm_delete
dbm_error
dbm_fetch
dbm_firstkey
dbm_nextkey
dbm_open
dbm_store
difftime
dirfd
dirname
div
dlclose
dlerror
dlopen
dlsym
dprintf
drand48
dup
dup2
duplocale
encrypt
endgrent
endhostent
endnetent
endprotoent
endpwent
endservent
endutxent
environ
erand48
erf
erfc
erfcf
erfcl
erff
erfl
errno
execl
execle
execlp
execv
execve
execvp
exit
exp
exp2
exp2f
exp2l
expf
expl
expm1
expm1f
expm1l
fabs
fabsf
fabsl
faccessat
fattach
fchdir
fchmod
fchmodat
fchown
fchownat
fclose
fcntl
fdatasync
fdetach
fdim
fdimf
fdiml
fdopen
fdopendir
feclearexcept
fegetenv
fegetexceptflag
fegetround
feholdexcept
feof
feraiseexcept
ferror
fesetenv
fesetexceptflag
fesetround
fetestexcept
feupdateenv
fexecve
fflush
ffs
fgetc
fgetpos
fgets
fgetwc
fgetws
fileno
flock
flockfile
floor
floorf
floorl
fma
fmaf
fmal
fmax
fmaxf
fmaxl
fmemopen
fmin
fminf
fminl
fmod
fmodf
fmodl
fmtmsg
fnmatch
fopen
fork
fpathconf
fpclassify
fprintf
fputc
fputs
fputwc
fputws
fread
free
freeaddrinfo
freelocale
freopen
frexp
frexpf
frexpl
fscanf
fseek
fseeko
fsetpos
fstat
fstatat
fstatvfs
fsync
ftell
ftello
ftok
ftruncate
ftrylockfile
ftw
funlockfile
futimens
fwide
fwprintf
fwrite
fwscanf
gai_strerror
getaddrinfo
getc
getc_unlocked
getchar
getchar_unlocked
getcwd
getdate
getdate_err
getdelim
getegid
getenv
geteuid
getgid
getgrent
getgrgid
getgrgid_r
getgrnam
getgrnam_r
getgroups
gethostent
gethostid
gethostname
getitimer
getline
getlogin
getlogin_r
getmsg
getnameinfo
getnetbyaddr
getnetbyname
getnetent
getopt
getpeername
getpgid
getpgrp
getpid
getpmsg
getppid
getpriority
getprotobyname
getprotobynumber
getprotoent
getpwent
getpwnam
getpwnam_r
getpwuid
getpwuid_r
getrlimit
getrusage
gets
getservbyname
getservbyport
getservent
getsid
getsockname
getsockopt
getsubopt
gettimeofday
getuid
getutxent
getutxid
getutxline
getwc
getwchar
glob
globfree
gmtime
gmtime_r
grantpt
hcreate
hdestroy
hsearch
htonl
htons
hypot
hypotf
hypotl
iconv
iconv_close
iconv_open
if_freenameindex
if_indextoname
if_nameindex
if_nametoindex
ilogb
ilogbf
ilogbl
imaxabs
imaxdiv
inet_addr
inet_ntoa
inet_ntop
inet_pton
initstate
insque
ioctl
isalnum
isalnum_l
isalpha
isalpha_l
isascii
isastream
isatty
isblank
isblank_l
iscntrl
iscntrl_l
isdigit
isdigit_l
isfinite
isgraph
isgraph_l
isgreater
isgreaterequal
isinf
isless
islessequal
islessgreater
islower
islower_l
isnan
isnormal
isprint
isprint_l
ispunct
ispunct_l
isspace
isspace_l
isunordered
isupper
isupper_l
iswalnum
iswalnum_l
iswalpha
iswalpha_l
iswblank
iswblank_l
iswcntrl
iswcntrl_l
iswctype
iswctype_l
iswdigit
iswdigit_l
iswgraph
iswgraph_l
iswlower
iswlower_l
iswprint
iswprint_l
iswpunct
iswpunct_l
iswspace
iswspace_l
iswupper
iswupper_l
iswxdigit
iswxdigit_l
isxdigit
isxdigit_l
j0
j1
jn
jrand48
kill
killpg
l64a
labs
lchown
lcong48
ldexp
ldexpf
ldexpl
ldiv
lfind
lgamma
lgammaf
lgammal
link
linkat
lio_listio
listen
llabs
lldiv
llrint
llrintf
llrintl
llround
llroundf
llroundl
localeconv
localtime
localtime_r
lockf
log
log10
log10f
log10l
log1p
log1pf
log1pl
log2
log2f
log2l
logb
logbf
logbl
logf
logl
longjmp
lrand48
lrint
lrintf
lrintl
lround
lroundf
lroundl
lsearch
lseek
lstat
malloc
mblen
mbrlen
mbrtowc
mbsinit
mbsnrtowcs
mbsrtowcs
mbstowcs
mbtowc
memccpy
memchr
memcmp
memcpy
memmove
memset
mkdir
mkdirat
mkdtemp
mkfifo
mkfifoat
mknod
mknodat
mkstemp
mktime
mlock
mlockall
mmap
modf
modff
modfl
mprotect
mq_close
mq_getattr
mq_notify
mq_open
mq_receive
mq_send
mq_setattr
mq_timedreceive
mq_timedsend
mq_unlink
mrand48
msgctl
msgget
msgrcv
msgsnd
msync
munlock
munlockall
munmap
nan
nanf
nanl
nanosleep
nearbyint
nearbyintf
nearbyintl
newlocale
nextafter
nextafterf
nextafterl
nexttoward
nexttowardf
nexttowardl
nftw
nice
nl_langinfo
nl_langinfo_l
nrand48
ntohl
ntohs
open
open_memstream
open_wmemstream
openat
opendir
openlog
optarg
opterr
optind
optopt
pathconf
pause
pclose
perror
pipe
poll
popen
posix_fadvise
posix_fallocate
posix_madvise
posix_mem_offset
posix_memalign
posix_openpt
posix_spawn
posix_spawn_file_actions_addclose
posix_spawn_file_actions_adddup2
posix_spawn_file_actions_addopen
posix_spawn_file_actions_destroy
posix_spawn_file_actions_init
posix_spawnattr_destroy
posix_spawnattr_getflags
posix_spawnattr_getpgroup
posix_spawnattr_getschedparam
posix_spawnattr_getschedpolicy
posix_spawnattr_getsigdefault
posix_spawnattr_getsigmask
posix_spawnattr_init
posix_spawnattr_setflags
posix_spawnattr_setpgroup
posix_spawnattr_setschedparam
posix_spawnattr_setschedpolicy
posix_spawnattr_setsigdefault
posix_spawnattr_setsigmask
posix_spawnp
posix_trace_attr_destroy
posix_trace_attr_getclockres
posix_trace_attr_getcreatetime
posix_trace_attr_getgenversion
posix_trace_attr_getinherited
posix_trace_attr_getlogfullpolicy
posix_trace_attr_getlogsize
posix_trace_attr_getmaxdatasize
posix_trace_attr_getmaxsystemeventsize
posix_trace_attr_getmaxusereventsize
posix_trace_attr_getname
posix_trace_attr_getstreamfullpolicy
posix_trace_attr_getstreamsize
posix_trace_attr_init
posix_trace_attr_setinherited
posix_trace_attr_setlogfullpolicy
posix_trace_attr_setlogsize
posix_trace_attr_setmaxdatasize
posix_trace_attr_setname
posix_trace_attr_setstreamfullpolicy
posix_trace_attr_setstreamsize
posix_trace_clear
posix_trace_close
posix_trace_create
posix_trace_create_withlog
posix_trace_event
posix_trace_eventid_equal
posix_trace_eventid_get_name
posix_trace_eventid_open
posix_trace_eventset_add
posix_trace_eventset_del
posix_trace_eventset_empty
posix_trace_eventset_fill
posix_trace_eventset_ismember
posix_trace_eventtypelist_getnext_id
posix_trace_eventtypelist_rewind
posix_trace_flush
posix_trace_get_attr
posix_trace_get_filter
posix_trace_get_status
posix_trace_getnext_event
posix_trace_open
posix_trace_rewind
posix_trace_set_filter
posix_trace_shutdown
posix_trace_start
posix_trace_stop
posix_trace_timedgetnext_event
posix_trace_trid_eventid_open
posix_trace_trygetnext_event
posix_typed_mem_get_info
posix_typed_mem_open
pow
powf
powl
pread
printf
pselect
psiginfo
psignal
pthread_atfork
pthread_attr_destroy
pthread_attr_getdetachstate
pthread_attr_getguardsize
pthread_attr_getinheritsched
pthread_attr_getschedparam
pthread_attr_getschedpolicy
pthread_attr_getscope
pthread_attr_getstack
pthread_attr_getstacksize
pthread_attr_init
pthread_attr_setdetachstate
pthread_attr_setguardsize
pthread_attr_setinheritsched
pthread_attr_setschedparam
pthread_attr_setschedpolicy
pthread_attr_setscope
pthread_attr_setstack
pthread_attr_setstacksize
pthread_barrier_destroy
pthread_barrier_init
pthread_barrier_wait
pthread_barrierattr_destroy
pthread_barrierattr_getpshared
pthread_barrierattr_init
pthread_barrierattr_setpshared
pthread_cancel
pthread_cleanup_pop
pthread_cleanup_push
pthread_cond_broadcast
pthread_cond_destroy
pthread_cond_init
pthread_cond_signal
pthread_cond_timedwait
pthread_cond_wait
pthread_condattr_destroy
pthread_condattr_getclock
pthread_condattr_getpshared
pthread_condattr_init
pthread_condattr_setclock
pthread_condattr_setpshared
pthread_create
pthread_detach
pthread_equal
pthread_exit
pthread_getconcurrency
pthread_getcpuclockid
pthread_getschedparam
pthread_getspecific
pthread_join
pthread_key_create
pthread_key_delete
pthread_kill
pthread_mutex_consistent
pthread_mutex_destroy
pthread_mutex_getprioceiling
pthread_mutex_init
pthread_mutex_lock
pthread_mutex_setprioceiling
pthread_mutex_timedlock
pthread_mutex_trylock
pthread_mutex_unlock
pthread_mutexattr_destroy
pthread_mutexattr_getprioceiling
pthread_mutexattr_getprotocol
pthread_mutexattr_getpshared
pthread_mutexattr_getrobust
pthread_mutexattr_gettype
pthread_mutexattr_init
pthread_mutexattr_setprioceiling
pthread_mutexattr_setprotocol
pthread_mutexattr_setpshared
pthread_mutexattr_setrobust
pthread_mutexattr_settype
pthread_once
pthread_rwlock_destroy
pthread_rwlock_init
pthread_rwlock_rdlock
pthread_rwlock_timedrdlock
pthread_rwlock_timedwrlock
pthread_rwlock_tryrdlock
pthread_rwlock_trywrlock
pthread_rwlock_unlock
pthread_rwlock_wrlock
pthread_rwlockattr_destroy
pthread_rwlockattr_getpshared
pthread_rwlockattr_init
pthread_rwlockattr_setpshared
pthread_self
pthread_setcancelstate
pthread_setcanceltype
pthread_setconcurrency
pthread_setschedparam
pthread_setschedprio
pthread_setspecific
pthread_sigmask
pthread_spin_destroy
pthread_spin_init
pthread_spin_lock
pthread_spin_trylock
pthread_spin_unlock
pthread_testcancel
ptsname
putc
putc_unlocked
putchar
putchar_unlocked
putenv
putmsg
putpmsg
puts
pututxline
putwc
putwchar
pwrite
qsort
raise
rand
rand_r
random
read
readdir
readdir_r
readlink
readlinkat
readv
realloc
realpath
recv
recvfrom
recvmsg
regcomp
regerror
regexec
regfree
remainder
remainderf
remainderl
remove
remque
remquo
remquof
remquol
rename
renameat
rewind
rewinddir
rint
rintf
rintl
rmdir
round
roundf
roundl
scalbln
scalblnf
scalblnl
scalbn
scalbnf
scalbnl
scandir
scanf
sched_get_priority_max
sched_get_priority_min
sched_getparam
sched_getscheduler
sched_rr_get_interval
sched_setparam
sched_setscheduler
sched_yield
seed48
seekdir
select
sem_close
sem_destroy
sem_getvalue
sem_init
sem_open
sem_post
sem_timedwait
sem_trywait
sem_unlink
sem_wait
semctl
semget
semop
send
sendmsg
sendto
setbuf
setegid
setenv
seteuid
setgid
setgrent
sethostent
setitimer
setjmp
setkey
setlocale
setlogmask
setnetent
setpgid
setpgrp
setpriority
setprotoent
setpwent
setregid
setreuid
setrlimit
setservent
setsid
setsockopt
setstate
setuid
setutxent
setvbuf
shm_open
shm_unlink
shmat
shmctl
shmdt
shmget
shutdown
sigaction
sigaddset
sigaltstack
sigdelset
sigemptyset
sigfillset
sighold
sigignore
siginterrupt
sigismember
siglongjmp
signal
signbit
signgam
sigpause
sigpending
sigprocmask
sigqueue
sigrelse
sigset
sigsetjmp
sigsuspend
sigtimedwait
sigwait
sigwaitinfo
sin
sinf
sinh
sinhf
sinhl
sinl
sleep
snprintf
sockatmark
socket
socketpair
sprintf
sqrt
sqrtf
sqrtl
srand
srand48
srandom
sscanf
stat
statvfs
stderr
stdin
stdout
stpcpy
stpncpy
strcasecmp
strcasecmp_l
strcat
strchr
strcmp
strcoll
strcoll_l
strcpy
strcspn
strdup
strerror
strerror_l
strerror_r
strfmon
strfmon_l
strftime
strftime_l
strlen
strncasecmp
strncasecmp_l
strncat
strncmp
strncpy
strndup
strnlen
strpbrk
strptime
strrchr
strsignal
strspn
strstr
strtod
strtof
strtoimax
strtok
strtok_r
strtol
strtold
strtoll
strtoul
strtoull
strtoumax
strxfrm
strxfrm_l
swab
swprintf
swscanf
symlink
symlinkat
sync
sysconf
syslog
system
tan
tanf
tanh
tanhf
tanhl
tanl
tcdrain
tcflow
tcflush
tcgetattr
tcgetpgrp
tcgetsid
tcsendbreak
tcsetattr
tcsetpgrp
tdelete
telldir
tempnam
tfind
tgamma
tgammaf
tgammal
time
timer_create
timer_delete
timer_getoverrun
timer_gettime
timer_settime
times
timezone
tmpfile
tmpnam
toascii
tolower
tolower_l
toupper
toupper_l
towctrans
towctrans_l
towlower
towlower_l
towupper
towupper_l
trunc
truncate
truncf
truncl
tsearch
ttyname
ttyname_r
twalk
tzname
tzset
ulimit
umask
uname
ungetc
ungetwc
unlink
unlinkat
unlockpt
unsetenv
uselocale
utime
utimensat
utimes
va_arg
va_copy
va_end
va_start
vdprintf
vfprintf
vfscanf
vfwprintf
vfwscanf
vprintf
vscanf
vsnprintf
vsprintf
vsscanf
vswprintf
vswscanf
vwprintf
vwscanf
wait
waitid
waitpid
wcpcpy
wcpncpy
wcrtomb
wcscasecmp
wcscasecmp_l
wcscat
wcschr
wcscmp
wcscoll
wcscoll_l
wcscpy
wcscspn
wcsdup
wcsftime
wcslen
wcsncasecmp
wcsncasecmp_l
wcsncat
wcsncmp
wcsncpy
wcsnlen
wcsnrtombs
wcspbrk
wcsrchr
wcsrtombs
wcsspn
wcsstr
wcstod
wcstof
wcstoimax
wcstok
wcstol
wcstold
wcstoll
wcstombs
wcstoul
wcstoull
wcstoumax
wcswidth
wcsxfrm
wcsxfrm_l
wctob
wctomb
wctrans
wctrans_l
wctype
wctype_l
wcwidth
wmemchr
wmemcmp
wmemcpy
wmemmove
wmemset
wordexp
wordfree
wprintf
write
writev
wscanf
y0
y1
yn
' | sed -e "$sed_alt1" | tr -d "$trnl" | sed -e "$sed_alt2" -e "$sed_alt3"`
posix2001_functions=`echo '
bcmp
bcopy
bsd_signal
bzero
ecvt
fcvt
ftime
gcvt
getcontext
gethostbyaddr
gethostbyname
getwd
h_errno
index
makecontext
mktemp
pread
pthread_attr_getstackaddr
pthread_attr_setstackaddr
rindex
scalb
setcontext
swapcontext
ualarm
usleep
vfork
wcswcs
' | sed -e "$sed_alt1" | tr -d "$trnl" | sed -e "$sed_alt2" -e "$sed_alt3"`

indent=""
seen_modules=
seen_files=

# func_exit STATUS
# exit with status
func_exit ()
{
  (exit $1); exit $1
}

# func_tmpdir
# creates a temporary directory.
# Sets variable
# - tmp             pathname of freshly created temporary directory
func_tmpdir ()
{
  # Use the environment variable TMPDIR, falling back to /tmp. This allows
  # users to specify a different temporary directory, for example, if their
  # /tmp is filled up or too small.
  : ${TMPDIR=/tmp}
  {
    # Use the mktemp program if available. If not available, hide the error
    # message.
    tmp=`(umask 077 && mktemp -d "$TMPDIR/MDXXXXXX") 2>/dev/null` &&
    test -n "$tmp" && test -d "$tmp"
  } ||
  {
    # Use a simple mkdir command. It is guaranteed to fail if the directory
    # already exists.  $RANDOM is bash specific and expands to empty in shells
    # other than bash, ksh and zsh.  Its use does not increase security;
    # rather, it minimizes the probability of failure in a very cluttered /tmp
    # directory.
    tmp=$TMPDIR/MD$$-$RANDOM
    (umask 077 && mkdir "$tmp")
  } ||
  {
    echo "$0: cannot create a temporary directory in $TMPDIR" >&2
    func_exit 1
  }
}

# func_append var value
# appends the given value to the shell variable var.
if ( foo=bar; foo+=baz && test "$foo" = barbaz ) >/dev/null 2>&1; then
  # Use bash's += operator. It reduces complexity of appending repeatedly to
  # a single variable from O(n^2) to O(n).
  func_append ()
  {
    eval "$1+=\"\$2\""
  }
else
  func_append ()
  {
    eval "$1=\"\$$1\$2\""
  }
fi

# func_echo line
# outputs line with indentation.
func_echo ()
{
  echo "${indent}$*"
}

# func_indent
# increases the indentation.
func_indent ()
{
  indent="$indent  "
}

# func_unindent
# decreases the indentation.
func_unindent ()
{
  indent=`echo "$indent" | sed -e 's/  $//'`
}

# func_begin tag [attribute...]
# opens a HTML tag.
func_begin ()
{
  func_echo "<$*>"
  func_indent
}

# func_end tag
# closes a HTML tag.
func_end ()
{
  func_unindent
  func_echo "</$1>"
}

# func_wrap tag [attribute...]
# encloses $element in a HTML tag, without line breaks.
func_wrap ()
{
  element="<$*>${element}</$1>"
}

# func_section_wrap sectiontag
# encloses $element in a HTML tag referring to the given tag.
func_section_wrap ()
{
  if test -n "$in_toc"; then
    func_wrap A "HREF=\"#$1\""
  else
    func_wrap A "NAME=\"$1\""
  fi
}

# func_begin_table
func_begin_table ()
{
  func_begin TABLE
  if test -z "$in_toc"; then
    func_begin TR
    func_echo "<TH ALIGN=LEFT>modules/"
    func_echo "<TH ALIGN=LEFT>lib/"
    func_echo "<TH ALIGN=LEFT>lib/"
    func_echo "<TH ALIGN=LEFT>m4/"
    func_echo "<TH ALIGN=LEFT>&nbsp;"
    func_end TR
    func_begin TR
    func_echo "<TH ALIGN=LEFT>Module"
    func_echo "<TH ALIGN=LEFT>Header"
    func_echo "<TH ALIGN=LEFT>Implementation"
    func_echo "<TH ALIGN=LEFT>Autoconf macro"
    func_echo "<TH ALIGN=LEFT>Depends on"
    func_end TR
  fi
}

# func_module module
func_module ()
{
    sed_remove_trailing_empty_line='${
/^$/d
}'

  if test -n "$in_toc"; then

    func_begin TR "WIDTH=\"100%\""

    element='<A HREF="#module='$1'">'$1'</A>'
    func_echo "<TD ALIGN=LEFT VALIGN=TOP WIDTH=\"20%\">$element"

    # Rendering the description:
    # - Change the symbol() syntax as suitable for documentation, removing the
    #   parentheses (as per GNU standards, section "GNU Manuals").
    # - Flag the remaining symbol() constructs as errors.
    # - Change 'xxx' to <CODE>xxx</CODE>.
    sed_extract_element='
      '$sed_lt'
      '$sed_gt'
      '$sed_remove_trailing_empty_line'
      s,^, ,
      s,$, ,
      s,\([^a-zA-Z_]\)'$posix_functions'() \(function\|macro\),\1<A HREF="'$POSIX2008_URL'/functions/\2.html">\2</A> \3,g
      s,\([^a-zA-Z_]\)'$posix_functions' \(function\|macro\),\1<A HREF="'$POSIX2008_URL'/functions/\2.html">\2</A> \3,g
      s,\([^a-zA-Z_]\)'$posix_functions'(),\1<A HREF="'$POSIX2008_URL'/functions/\2.html">\2</A> <SPAN STYLE="color:#FF0000;">what?? If you mean a function\, please say so.</SPAN>,g
      s,\([^a-zA-Z_]\)'$posix2001_functions'() \(function\|macro\),\1<A HREF="'$POSIX2001_URL'/functions/\2.html">\2</A> \3,g
      s,\([^a-zA-Z_]\)'$posix2001_functions' \(function\|macro\),\1<A HREF="'$POSIX2001_URL'/functions/\2.html">\2</A> \3,g
      s,\([^a-zA-Z_]\)'$posix2001_functions'(),\1<A HREF="'$POSIX2001_URL'/functions/\2.html">\2</A> <SPAN STYLE="color:#FF0000;">what?? If you mean a function\, please say so.</SPAN>,g
      s,\([^a-zA-Z_]\)\([a-zA-Z_][a-zA-Z0-9_]*\)() \(function\|macro\),\1\2 \3,g
      s,\([^a-zA-Z_]\)\([a-zA-Z_][a-zA-Z0-9_]*\)(),\1\2 <SPAN STYLE="color:#FF0000;">what?? If you mean a function\, please say so.</SPAN>,g
      s, '"'"'\([a-zA-Z0-9_ -]*\)'"'"'\([^a-zA-Z0-9_]\), <CODE>\1</CODE>\2,g
      s,^ ,,
      s, $,,
    '
    element=`gnulib-tool --extract-description $1 \
             | LC_ALL=C sed -e "$sed_extract_element"`
    func_echo "<TD ALIGN=LEFT VALIGN=TOP WIDTH=\"80%\">$element"

    func_end TR

  else

    func_begin TR

    element='<A NAME="module='$1'"></A><A HREF="'$repo_url_prefix'modules/'$1$repo_url_suffix'">'$1'</A>'
    func_echo "<TD ALIGN=LEFT VALIGN=TOP>$element"

    includes=`gnulib-tool --extract-include-directive $1`
    files=`gnulib-tool --extract-filelist $1 \
           | grep -v '^m4/gnulib-common\.m4$'`
    sed_extract_element='
      '$sed_lt'
      '$sed_gt'
      '$sed_remove_trailing_empty_line'
      s,^#include "\(.*\)"$,#include "<A HREF="'$repo_url_prefix'lib/\1'$repo_url_suffix_repl'">\1</A>",
      s,^#include &lt;'$posix_headers'\.h&gt;$,#include \&lt;<A HREF="'$POSIX2008_URL'/basedefs/\1.h.html">\1.h</A>\&gt;,
      s,<A HREF="'$POSIX2008_URL'/basedefs/\([a-zA-Z0-9_]*\)/\([a-zA-Z0-9_]*\)\.h\.html">,<A HREF="'$POSIX2008_URL'/basedefs/\1_\2.h.html">,
      s,^#include &lt;'$posix2001_headers'\.h&gt;$,#include \&lt;<A HREF="'$POSIX2001_URL'/basedefs/\1.h.html">\1.h</A>\&gt;,
      s/$/<BR>/
    '
    element=`echo "$includes" \
             | LC_ALL=C sed -e "$sed_extract_element" | tr -d "$trnl" \
             | sed -e 's/<BR>$//'`
    test -n "$element" || element='---'
    func_echo "<TD ALIGN=LEFT VALIGN=TOP>$element"

    sed_choose_unconditional_nonstandard_include='s,^#include "\(.*\)"$,\1,p'
    includefile=`echo "$includes" \
                 | sed -n -e "$sed_choose_unconditional_nonstandard_include" \
                 | sed -e "$sed_escape_dot" | tr -d "$trnl"`
    sed_choose_lib_files='s,^lib/\(.*\)$,\1,p'
    sed_extract_include='
      \|^'"$includefile"'$|d
      s,^\(.*\)$,<A HREF="'$repo_url_prefix'lib/\1'$repo_url_suffix_repl'">\1</A>,
      s/$/<BR>/
    '
    element=`echo "$files" \
             | sed -e '/^$/d' \
             | sed -n -e "$sed_choose_lib_files" \
             | sed -e "$sed_extract_include" \
             | tr -d "$trnl" | sed -e 's/<BR>$//'`
    test -n "$element" || element='---'
    func_echo "<TD ALIGN=LEFT VALIGN=TOP>$element"

    sed_choose_m4_files='s,^m4/\(.*\)$,\1,p'
    sed_extract_repo_url='
      /^onceonly/d
      s,^\(.*\)$,<A HREF="'$repo_url_prefix'm4/\1'$repo_url_suffix_repl'">\1</A>,
    '
    element=`(echo "$files" \
              | sed -e "$sed_remove_trailing_empty_line" \
              | sed -n -e "$sed_choose_m4_files" \
              | sed -e "$sed_extract_repo_url"; \
              gnulib-tool --extract-autoconf-snippet $1 \
              | sed -e "$sed_remove_trailing_empty_line") \
              | sed -e 's/$/<BR>/' | tr -d "$trnl" | sed -e 's/<BR>$//'`
    test -n "$element" || element='---'
    func_echo "<TD ALIGN=LEFT VALIGN=TOP>$element"

    element=`gnulib-tool --extract-dependencies $1 \
             | sed -e "$sed_remove_trailing_empty_line" \
                   -e 's/$/<BR>/' | tr -d "$trnl" | sed -e 's/<BR>$//'`
    test -n "$element" || element='---'
    func_echo "<TD ALIGN=LEFT VALIGN=TOP>$element"

    func_end TR

    func_append seen_modules " $1"
    func_append seen_files " $files"
  fi
}

# func_end_table
func_end_table ()
{
  func_end TABLE
}

# func_all_modules
func_all_modules ()
{
  element="Support for obsolete systems lacking ANSI C 89"
  func_section_wrap ansic_sup_obsolete
  func_wrap H2
  func_echo "$element"

  func_begin_table
  func_module stdlib
  func_module strtol
  func_module strtoul
  func_module memcmp
  func_module memcpy
  func_module memmove
  func_module memset
  func_module strcspn
  func_module strpbrk
  func_end_table

  func_echo 'These modules are not listed among dependencies below, for simplicity.'
  func_echo 'If your package requires portability to old, obsolete systems, you need to list these modules explicitly among the modules to import through gnulib-tool.'

  element="Support for systems lacking ANSI C 89"
  func_section_wrap ansic_sup
  func_wrap H2
  func_echo "$element"

  func_begin_table
  func_module atexit
  func_module ctime
  func_module localtime
  func_module strtod
  func_module strerror
  func_module strerror-override
  func_module mktime
  func_end_table

  element="Enhancements for ANSI C 89 functions"
  func_section_wrap ansic_enh
  func_wrap H2
  func_echo "$element"

  element="Diagnostics <assert.h>"
  element=`printf "%s" "$element" | sed -e "$sed_lt" -e "$sed_gt"`
  func_section_wrap ansic_enh_assert_diagnostics
  func_wrap H3
  func_echo "$element"

  func_begin_table
  func_module assert
  func_module assure
  func_module verify
  func_end_table

  element="Mathematics <math.h>"
  element=`printf "%s" "$element" | sed -e "$sed_lt" -e "$sed_gt"`
  func_section_wrap ansic_enh_math
  func_wrap H3
  func_echo "$element"

  func_begin_table
  func_module fpieee
  func_end_table

  element="Input/output <stdio.h>"
  element=`printf "%s" "$element" | sed -e "$sed_lt" -e "$sed_gt"`
  func_section_wrap ansic_enh_stdio
  func_wrap H3
  func_echo "$element"

  func_begin_table
  func_module fflush
  func_module fseterr
  func_module tmpfile
  func_end_table

  element="Memory management functions <stdlib.h>"
  element=`printf "%s" "$element" | sed -e "$sed_lt" -e "$sed_gt"`
  func_section_wrap ansic_enh_stdlib_memory
  func_wrap H3
  func_echo "$element"

  func_begin_table
  func_module calloc-gnu
  func_module eealloc
  func_module free-posix
  func_module malloc-gnu
  func_module realloc-gnu
  func_module reallocarray
  func_module pagealign_alloc
  func_end_table

  element="Sorting functions <stdlib.h>"
  element=`printf "%s" "$element" | sed -e "$sed_lt" -e "$sed_gt"`
  func_section_wrap ansic_enh_stdlib_sorting
  func_wrap H3
  func_echo "$element"

  func_begin_table
  func_module array-mergesort
  func_module mpsort
  func_module qsort_r
  func_end_table

  element="Date and time <time.h>"
  element=`printf "%s" "$element" | sed -e "$sed_lt" -e "$sed_gt"`
  func_section_wrap ansic_enh_time_datetime
  func_wrap H3
  func_echo "$element"

  func_begin_table
  func_module fprintftime
  func_module nstrftime
  func_module strftime-fixes
  func_module time_rz
  func_module year2038
  func_end_table

  element="Extra functions based on ANSI C 89"
  func_section_wrap ansic_ext
  func_wrap H2
  func_echo "$element"

  element="Memory management functions <stdlib.h>"
  element=`printf "%s" "$element" | sed -e "$sed_lt" -e "$sed_gt"`
  func_section_wrap ansic_ext_stdlib_memory
  func_wrap H3
  func_echo "$element"

  func_begin_table
  func_module xsize
  func_module xalloc
  func_module xalloc-die
  func_module alloca
  func_module alloca-opt
  func_module malloca
  func_module xmalloca
  func_module xmemdup0
  func_module safe-alloc
  func_end_table

  element="Integer arithmetic functions <stdlib.h>"
  element=`printf "%s" "$element" | sed -e "$sed_lt" -e "$sed_gt"`
  func_section_wrap ansic_ext_stdlib_arith
  func_wrap H3
  func_echo "$element"

  func_begin_table
  func_module count-leading-zeros
  func_module count-one-bits
  func_module count-trailing-zeros
  func_module ffs
  func_module ffsl
  func_module ffsll
  func_module gcd
  func_module minmax
  func_end_table

  element="Environment variables <stdlib.h>"
  element=`printf "%s" "$element" | sed -e "$sed_lt" -e "$sed_gt"`
  func_section_wrap ansic_ext_stdlib_env
  func_wrap H3
  func_echo "$element"

  func_begin_table
  func_module putenv
  func_module secure_getenv
  func_module setenv
  func_module unsetenv
  func_module xsetenv
  func_end_table

  element="Character handling <ctype.h>"
  element=`printf "%s" "$element" | sed -e "$sed_lt" -e "$sed_gt"`
  func_section_wrap ansic_ext_ctype
  func_wrap H3
  func_echo "$element"

  func_begin_table
  func_module c-ctype
  func_end_table

  element="String handling <string.h>"
  element=`printf "%s" "$element" | sed -e "$sed_lt" -e "$sed_gt"`
  func_section_wrap ansic_ext_string
  func_wrap H3
  func_echo "$element"

  func_begin_table
  func_module bcopy
  func_module explicit_bzero
  func_module memchr
  func_module memchr2
  func_module memcmp2
  func_module memmem
  func_module memmem-simple
  func_module mempcpy
  func_module memrchr
  func_module amemxfrm
  func_module rawmemchr
  func_module stpcpy
  func_module stpncpy
  func_module c-strcase
  func_module strcase
  func_module c-strcaseeq
  func_module c-strcasestr
  func_module strcasestr
  func_module strcasestr-simple
  func_module strchrnul
  func_module streq
  func_module strerror_r-posix
  func_module strnlen
  func_module strnlen1
  func_module strndup
  func_module strsep
  func_module strstr
  func_module strstr-simple
  func_module c-strstr
  func_module astrxfrm
  func_module trim
  func_module fstrcmp
  func_module xstrndup
  func_end_table

  element="Mathematics <math.h>"
  element=`printf "%s" "$element" | sed -e "$sed_lt" -e "$sed_gt"`
  func_section_wrap ansic_ext_math
  func_wrap H3
  func_echo "$element"

  func_begin_table
  func_module printf-frexp
  func_end_table

  element="Numeric conversion functions <stdlib.h>"
  element=`printf "%s" "$element" | sed -e "$sed_lt" -e "$sed_gt"`
  func_section_wrap ansic_ext_stdlib_conv
  func_wrap H3
  func_echo "$element"

  func_begin_table
  func_module c-strtod
  func_module c-strtold
  func_module xstrtod
  func_module xstrtol
  func_module xstrtol-error
  func_module xstrtoll
  func_module xstrtold
  func_end_table

  element="Date and time <time.h>"
  element=`printf "%s" "$element" | sed -e "$sed_lt" -e "$sed_gt"`
  func_section_wrap ansic_ext_time_datetime
  func_wrap H3
  func_echo "$element"

  func_begin_table
  func_module mktime-internal
  func_module parse-datetime
  func_module timegm
  func_module tzset
  func_end_table

  element="Input/Output <stdio.h>"
  element=`printf "%s" "$element" | sed -e "$sed_lt" -e "$sed_gt"`
  func_section_wrap ansic_ext_stdio
  func_wrap H3
  func_echo "$element"

  func_begin_table
  func_module unlocked-io
  func_module fwriteerror
  func_module vasnprintf
  func_module vasprintf
  func_module xprintf
  func_module xvasprintf
  func_end_table

  element="Signal handling <signal.h>"
  element=`printf "%s" "$element" | sed -e "$sed_lt" -e "$sed_gt"`
  func_section_wrap ansic_ext_signal
  func_wrap H3
  func_echo "$element"

  func_begin_table
  func_module fatal-signal
  func_module raise
  func_module strsignal
  func_end_table

  element="Command-line arguments"
  element=`printf "%s" "$element" | sed -e "$sed_lt" -e "$sed_gt"`
  func_section_wrap ansic_ext_argv
  func_wrap H3
  func_echo "$element"

  func_begin_table
  func_module argmatch
  func_module argv-iter
  func_module version-etc
  func_module version-etc-fsf
  func_module long-options
  func_end_table

  element="Container data structures"
  element=`printf "%s" "$element" | sed -e "$sed_lt" -e "$sed_gt"`
  func_section_wrap ansic_ext_container
  func_wrap H3
  func_echo "$element"

  func_begin_table
  func_module list
  func_module xlist
  func_module array-list
  func_module carray-list
  func_module linked-list
  func_module avltree-list
  func_module rbtree-list
  func_module linkedhash-list
  func_module avltreehash-list
  func_module rbtreehash-list
  func_module sublist
  func_module xsublist
  func_module oset
  func_module xoset
  func_module array-oset
  func_module avltree-oset
  func_module rbtree-oset
  func_end_table

  element="Cryptographic computations (low-level)"
  element=`printf "%s" "$element" | sed -e "$sed_lt" -e "$sed_gt"`
  func_section_wrap ansic_ext_crypto
  func_wrap H3
  func_echo "$element"

  func_begin_table
  func_module crypto/arcfour
  func_module crypto/arctwo
  func_module crypto/des
  func_module crypto/hmac-md5
  func_module crypto/hmac-sha1
  func_module crypto/md2
  func_module crypto/md4
  func_module crypto/md5
  func_module crypto/rijndael
  func_module crypto/sha1
  func_module crypto/sha256
  func_module crypto/sha512
  func_module crypto/sm3
  func_end_table

  element="Cryptographic computations (high-level)"
  element=`printf "%s" "$element" | sed -e "$sed_lt" -e "$sed_gt"`
  func_section_wrap ansic_ext_crypto2
  func_wrap H3
  func_echo "$element"

  func_begin_table
  func_module crypto/gc
  func_module crypto/gc-arcfour
  func_module crypto/gc-arctwo
  func_module crypto/gc-camellia
  func_module crypto/gc-des
  func_module crypto/gc-hmac-md5
  func_module crypto/gc-hmac-sha1
  func_module crypto/gc-md2
  func_module crypto/gc-md4
  func_module crypto/gc-md5
  func_module crypto/gc-pbkdf2
  func_module crypto/gc-pbkdf2-sha1
  func_module crypto/gc-random
  func_module crypto/gc-rijndael
  func_module crypto/gc-sha1
  func_module crypto/gc-sm3
  func_end_table

  element="Compiler warning management"
  element=`printf "%s" "$element" | sed -e "$sed_lt" -e "$sed_gt"`
  func_section_wrap ansic_ext_compwarn
  func_wrap H3
  func_echo "$element"

  func_begin_table
  func_module ignore-value
  func_end_table

  element="Misc"
  element=`printf "%s" "$element" | sed -e "$sed_lt" -e "$sed_gt"`
  func_section_wrap ansic_ext_misc
  func_wrap H3
  func_echo "$element"

  func_begin_table
  func_module base32
  func_module base64
  func_module check-version
  func_module crc
  func_module diffseq
  func_module execinfo
  func_module getline
  func_module getdelim
  func_module getnline
  func_module getndelim2
  func_module linebuffer
  func_module memxor
  func_module obstack
  func_module obstack-printf
  func_module obstack-printf-posix
  func_module hash-pjw
  func_module hash-pjw-bare
  func_module hash
  func_module readline
  func_module readtokens
  func_module readtokens0
  func_module stack
  func_module strverscmp
  func_module filevercmp
  func_end_table

  element="Support for systems lacking ISO C 99"
  func_section_wrap isoc_sup
  func_wrap H2
  func_echo "$element"

  element="Core language properties"
  element=`printf "%s" "$element" | sed -e "$sed_lt" -e "$sed_gt"`
  func_section_wrap isoc_core_properties
  func_wrap H3
  func_echo "$element"

  func_begin_table
  func_module alignof
  func_module c99
  func_module flexmember
  func_module fpucw
  func_module func
  func_module inline
  func_module longlong
  func_module snippet/unused-parameter
  func_module va-args
  func_module vararrays
  func_module vla
  func_end_table

  element="Variable arguments <stdarg.h>"
  element=`printf "%s" "$element" | sed -e "$sed_lt" -e "$sed_gt"`
  func_section_wrap isoc_sup_stdarg
  func_wrap H3
  func_echo "$element"

  func_begin_table
  func_module stdarg
  func_end_table

  element="Boolean type and values <stdbool.h>"
  element=`printf "%s" "$element" | sed -e "$sed_lt" -e "$sed_gt"`
  func_section_wrap isoc_sup_stdbool
  func_wrap H3
  func_echo "$element"

  func_begin_table
  func_module stdbool
  func_end_table

  element="Basic types <stddef.h>"
  element=`printf "%s" "$element" | sed -e "$sed_lt" -e "$sed_gt"`
  func_section_wrap isoc_sup_stddef
  func_wrap H3
  func_echo "$element"

  func_begin_table
  func_module stddef
  func_end_table

  element="Integer types and values <stdint.h>"
  element=`printf "%s" "$element" | sed -e "$sed_lt" -e "$sed_gt"`
  func_section_wrap isoc_sup_stdint
  func_wrap H3
  func_echo "$element"

  func_begin_table
  func_module size_max
  func_module stdint
  func_end_table

  element="Input/output <stdio.h>"
  element=`printf "%s" "$element" | sed -e "$sed_lt" -e "$sed_gt"`
  func_section_wrap isoc_sup_stdio
  func_wrap H3
  func_echo "$element"

  func_begin_table
  func_module stdio
  func_module snprintf
  func_module vsnprintf
  func_end_table

  element="Process control, Numeric conversion functions <stdlib.h>"
  element=`printf "%s" "$element" | sed -e "$sed_lt" -e "$sed_gt"`
  func_section_wrap isoc_sup_stdlib_procconv
  func_wrap H3
  func_echo "$element"

  func_begin_table
  func_module _Exit
  func_module atoll
  func_module strtoll
  func_module strtoull
  func_end_table

  element="Unibyte characters <ctype.h>"
  element=`printf "%s" "$element" | sed -e "$sed_lt" -e "$sed_gt"`
  func_section_wrap isoc_sup_ctype
  func_wrap H3
  func_echo "$element"

  func_begin_table
  func_module ctype
  func_end_table

  element="Functions for greatest-width integer types <inttypes.h>"
  element=`printf "%s" "$element" | sed -e "$sed_lt" -e "$sed_gt"`
  func_section_wrap isoc_sup_inttypes
  func_wrap H3
  func_echo "$element"

  func_begin_table
  func_module imaxabs
  func_module imaxdiv
  func_module inttypes
  func_module strtoimax
  func_module strtoumax
  func_end_table

  element="String handling <string.h>"
  element=`printf "%s" "$element" | sed -e "$sed_lt" -e "$sed_gt"`
  func_section_wrap isoc_sup_string
  func_wrap H3
  func_echo "$element"

  func_begin_table
  func_module strncat
  func_end_table

  element="Extended multibyte and wide character utilities <wchar.h>"
  element=`printf "%s" "$element" | sed -e "$sed_lt" -e "$sed_gt"`
  func_section_wrap isoc_sup_wchar
  func_wrap H3
  func_echo "$element"

  func_begin_table
  func_module wchar
  func_module btowc
  func_module wctob
  func_module mbsinit
  func_module mbrlen
  func_module mbrtowc
  func_module mbsrtowcs
  func_module wcrtomb
  func_module wcsrtombs
  func_end_table

  element="Wide character classification and mapping utilities <wctype.h>"
  element=`printf "%s" "$element" | sed -e "$sed_lt" -e "$sed_gt"`
  func_section_wrap isoc_sup_wctype
  func_wrap H3
  func_echo "$element"

  func_begin_table
  func_module wctype
  func_end_table

  element="Characteristics of floating types <float.h>"
  element=`printf "%s" "$element" | sed -e "$sed_lt" -e "$sed_gt"`
  func_section_wrap isoc_sup_float
  func_wrap H3
  func_echo "$element"

  func_begin_table
  func_module float
  func_end_table

  element="Mathematics <math.h>"
  element=`printf "%s" "$element" | sed -e "$sed_lt" -e "$sed_gt"`
  func_section_wrap isoc_sup_math
  func_wrap H3
  func_echo "$element"

  func_begin_table
  func_module acos
  func_module acosl
  func_module asin
  func_module asinl
  func_module atan
  func_module atan2
  func_module atanl
  func_module cbrt
  func_module ceil
  func_module ceilf
  func_module ceill
  func_module copysign
  func_module cos
  func_module cosh
  func_module cosl
  func_module erf
  func_module erfc
  func_module exp
  func_module expl
  func_module fabs
  func_module floor
  func_module floorf
  func_module floorl
  func_module fmod
  func_module frexp
  func_module frexp-nolibm
  func_module frexpl
  func_module frexpl-nolibm
  func_module hypot
  func_module isfinite
  func_module isinf
  func_module isnan
  func_module isnanf
  func_module isnanf-nolibm
  func_module isnand
  func_module isnand-nolibm
  func_module isnanl
  func_module isnanl-nolibm
  func_module j0
  func_module j1
  func_module jn
  func_module ldexp
  func_module ldexpl
  func_module lgamma
  func_module log
  func_module log10
  func_module log1p
  func_module logb
  func_module logl
  func_module math
  func_module mathl
  func_module modf
  func_module nextafter
  func_module pow
  func_module remainder
  func_module rint
  func_module round
  func_module roundf
  func_module roundl
  func_module signbit
  func_module sin
  func_module sinh
  func_module sinl
  func_module sqrt
  func_module sqrtl
  func_module tan
  func_module tanh
  func_module tanl
  func_module trunc
  func_module truncf
  func_module truncl
  func_module y0
  func_module y1
  func_module yn
  func_end_table

  element="Enhancements for ISO C 99 functions"
  func_section_wrap isoc_enh
  func_wrap H2
  func_echo "$element"

  element="Input/output <stdio.h>"
  element=`printf "%s" "$element" | sed -e "$sed_lt" -e "$sed_gt"`
  func_section_wrap isoc_enh_stdio
  func_wrap H3
  func_echo "$element"

  func_begin_table
  func_module printf-safe
  func_end_table

  element="Extra functions based on ISO C 99"
  func_section_wrap isoc_ext
  func_wrap H2
  func_echo "$element"

  element="Mathematics <math.h>"
  element=`printf "%s" "$element" | sed -e "$sed_lt" -e "$sed_gt"`
  func_section_wrap isoc_ext_math
  func_wrap H3
  func_echo "$element"

  func_begin_table
  func_module printf-frexpl
  func_end_table

  element="Numeric conversion functions <stdlib.h>"
  element=`printf "%s" "$element" | sed -e "$sed_lt" -e "$sed_gt"`
  func_section_wrap isoc_ext_stdlib_conv
  func_wrap H3
  func_echo "$element"

  func_begin_table
  func_module c-dtoastr
  func_module c-ldtoastr
  func_module dtoastr
  func_module ftoastr
  func_module intprops
  func_module inttostr
  func_module ldtoastr
  func_module xstrtoimax
  func_module xstrtoumax
  func_end_table

  element="Extended multibyte and wide character utilities <wchar.h>"
  element=`printf "%s" "$element" | sed -e "$sed_lt" -e "$sed_gt"`
  func_section_wrap isoc_ext_wchar_mb
  func_wrap H3
  func_echo "$element"

  func_begin_table
  func_module mbchar
  func_module mbiter
  func_module mbuiter
  func_module mbfile
  func_end_table

  element="Support for systems lacking ISO C 11"
  func_section_wrap c11_sup
  func_wrap H2
  func_echo "$element"

  element="Core language properties"
  element=`printf "%s" "$element" | sed -e "$sed_lt" -e "$sed_gt"`
  func_section_wrap c11_core_properties
  func_wrap H3
  func_echo "$element"

  func_begin_table
  func_module noreturn
  func_module std-gnu11
  func_module stdalign
  func_end_table

  element="Support for standard extensions to ISO C 11"
  func_section_wrap c11_ext
  func_wrap H2
  func_echo "$element"

  func_begin_table
  func_module attribute
  func_module builtin-expect
  func_module ieee754-h
  func_module limits-h
  func_end_table

  element="Support for GNU multiple precision arithmetic"
  func_section_wrap gmp
  func_wrap H2
  func_echo "$element"

  func_begin_table
  func_module libgmp
  func_end_table

  element="Support for sharing code with the GNU C Library"
  func_section_wrap glibc
  func_wrap H2
  func_echo "$element"

  func_begin_table
  func_module libc-config
  func_end_table

  element="Support for obsolete systems lacking POSIX:2008"
  func_section_wrap posix_sup_obsolete
  func_wrap H2
  func_echo "$element"

  func_begin_table
  func_module strdup
  func_end_table

  func_echo 'These modules are not listed among dependencies below, for simplicity.'
  func_echo 'If your package requires portability to old, obsolete systems, you need to list these modules explicitly among the modules to import through gnulib-tool.'

  element="Support for systems lacking POSIX:2008"
  func_section_wrap posix_sup
  func_wrap H2
  func_echo "$element"

  func_begin_table
  func_module accept
  func_module alphasort
  func_module arpa_inet
  func_module bind
  func_module calloc-posix
  func_module chown
  func_module close
  func_module connect
  func_module dirent
  func_module dprintf
  func_module dprintf-posix
  func_module dup2
  func_module duplocale
  func_module environ
  func_module errno
  func_module fchdir
  func_module fclose
  func_module fcntl-h
  func_module fcntl
  func_module fdatasync
  func_module flock
  func_module fopen
  func_module fprintf-posix
  func_module freopen
  func_module fseek
  func_module fseeko
  func_module fsync
  func_module ftell
  func_module ftello
  func_module ftruncate
  func_module futimens
  func_module getaddrinfo
  func_module getcwd
  func_module getcwd-lgpl
  func_module getgroups
  func_module gethostname
  func_module getlogin
  func_module getlogin_r
  func_module getopt-posix
  func_module getpeername
  func_module getsockname
  func_module getsockopt
  func_module getsubopt
  func_module gettimeofday
  func_module grantpt
  func_module hostent
  func_module iconv-h
  func_module iconv_open
  func_module inet_ntop
  func_module inet_pton
  func_module ioctl
  func_module isblank
  func_module langinfo
  func_module link
  func_module linkat
  func_module listen
  func_module locale
  func_module lseek
  func_module lstat
  func_module malloc-posix
  func_module mbsnrtowcs
  func_module mkdir
  func_module mkdtemp
  func_module mkfifo
  func_module mknod
  func_module mkstemp
  func_module net_if
  func_module netdb
  func_module netinet_in
  func_module nl_langinfo
  func_module open
  func_module perror
  func_module poll
  func_module popen
  func_module posix_openpt
  func_module posix_spawn
  func_module posix_spawnattr_destroy
  func_module posix_spawnattr_getflags
  func_module posix_spawnattr_getpgroup
  func_module posix_spawnattr_getschedparam
  func_module posix_spawnattr_getschedpolicy
  func_module posix_spawnattr_getsigdefault
  func_module posix_spawnattr_getsigmask
  func_module posix_spawnattr_init
  func_module posix_spawnattr_setflags
  func_module posix_spawnattr_setpgroup
  func_module posix_spawnattr_setschedparam
  func_module posix_spawnattr_setschedpolicy
  func_module posix_spawnattr_setsigdefault
  func_module posix_spawnattr_setsigmask
  func_module posix_spawn_file_actions_addclose
  func_module posix_spawn_file_actions_adddup2
  func_module posix_spawn_file_actions_addopen
  func_module posix_spawn_file_actions_destroy
  func_module posix_spawn_file_actions_init
  func_module posix_spawn-internal
  func_module posix_spawnp
  func_module pread
  func_module printf-posix
  func_module pselect
  func_module pthread
  func_module ptsname
  func_module pwrite
  func_module readlink
  func_module realloc-posix
  func_module recv
  func_module recvfrom
  func_module remove
  func_module scandir
  func_module sched
  func_module select
  func_module send
  func_module sendto
  func_module servent
  func_module setsockopt
  func_module shutdown
  func_module signal-h
  func_module sleep
  func_module snprintf-posix
  func_module socket
  func_module spawn
  func_module sprintf-posix
  func_module stat
  func_module strdup-posix
  func_module string
  func_module strings
  func_module tempname
  func_module time
  func_module time_r
  func_module times
  func_module timespec
  func_module nanosleep
  func_module pthread_sigmask
  func_module regex
  func_module rename
  func_module renameat
  func_module rmdir
  func_module search
  func_module sigaction
  func_module sigprocmask
  func_module socklen
  func_module ssize_t
  func_module strptime
  func_module strtok_r
  func_module sys_select
  func_module sys_socket
  func_module sys_stat
  func_module sys_time
  func_module sys_times
  func_module sys_uio
  func_module sys_utsname
  func_module sys_wait
  func_module truncate
  func_module tsearch
  func_module ttyname_r
  func_module uname
  func_module unistd
  func_module unlink
  func_module unlockpt
  func_module utime
  func_module utime-h
  func_module utimensat
  func_module vasnprintf-posix
  func_module vasprintf-posix
  func_module vdprintf
  func_module vdprintf-posix
  func_module vfprintf-posix
  func_module vprintf-posix
  func_module vsnprintf-posix
  func_module vsprintf-posix
  func_module wcsnrtombs
  func_module wcwidth
  func_module windows-stat-inodes
  func_module windows-stat-override
  func_module windows-stat-timespec
  func_module write
  func_end_table

  element="Compatibility checks for POSIX:2008 functions"
  func_section_wrap posix_compat
  func_wrap H2
  func_echo "$element"

  func_begin_table
  func_module clock-time
  func_module d-ino
  func_module d-type
  func_module link-follow
  func_module rmdir-errno
  func_module timer-time
  func_module unlink-busy
  func_module winsz-ioctl
  func_module winsz-termios
  func_end_table

  element="Enhancements for POSIX:2008 functions"
  func_section_wrap posix_enh
  func_wrap H2
  func_echo "$element"

  func_begin_table
  func_module chdir-long
  func_module dirent-safer
  func_module dirname
  func_module dirname-lgpl
  func_module getopt-gnu
  func_module iconv_open-utf
  func_module unistd-safer
  func_module fnmatch
  func_module fnmatch-gnu
  func_module glob
  func_module exclude
  func_end_table

  element="Extra functions based on POSIX:2008"
  func_section_wrap posix_ext
  func_wrap H2
  func_echo "$element"

  element="Input/output"
  element=`printf "%s" "$element" | sed -e "$sed_lt" -e "$sed_gt"`
  func_section_wrap posix_ext_stdio
  func_wrap H3
  func_echo "$element"

  func_begin_table
  func_module xprintf-posix
  func_module xvasprintf-posix
  func_end_table

  element="Numeric conversion functions"
  element=`printf "%s" "$element" | sed -e "$sed_lt" -e "$sed_gt"`
  func_section_wrap posix_ext_conv
  func_wrap H3
  func_echo "$element"

  func_begin_table
  func_module human
  func_end_table

  element="File system functions"
  element=`printf "%s" "$element" | sed -e "$sed_lt" -e "$sed_gt"`
  func_section_wrap posix_ext_filesys
  func_wrap H3
  func_echo "$element"

  func_begin_table
  func_module acl
  func_module acl-permissions
  func_module areadlink
  func_module areadlink-with-size
  func_module areadlinkat
  func_module areadlinkat-with-size
  func_module backupfile
  func_module backup-rename
  func_module canonicalize
  func_module canonicalize-lgpl
  func_module clean-temp
  func_module concat-filename
  func_module copy-file
  func_module fsusage
  func_module dirfd
  func_module double-slash-root
  func_module euidaccess
  func_module faccessat
  func_module fdopendir
  func_module fdutimensat
  func_module file-type
  func_module fileblocks
  func_module filemode
  func_module filename
  func_module filenamecat
  func_module filenamecat-lgpl
  func_module fts
  func_module isdir
  func_module largefile
  func_module lchmod
  func_module lchown
  func_module mkancesdirs
  func_module mkfifoat
  func_module mkdir-p
  func_module mkostemp
  func_module mkostemps
  func_module mkstemps
  func_module modechange
  func_module mountlist
  func_module openat
  func_module openat-die
  func_module pathmax
  func_module qacl
  func_module qcopy-acl
  func_module qset-acl
  func_module read-file
  func_module readlinkat
  func_module renameatu
  func_module same
  func_module save-cwd
  func_module savedir
  func_module savewd
  func_module stat-macros
  func_module stat-time
  func_module symlink
  func_module symlinkat
  func_module sys_file
  func_module sys_ioctl
  func_module tmpdir
  func_module unlinkdir
  func_module utimecmp
  func_module utimens
  func_module write-any-file
  func_module xconcat-filename
  func_module xgetcwd
  func_module xreadlink
  func_module xreadlinkat
  func_end_table

  element="File system as inode set"
  element=`printf "%s" "$element" | sed -e "$sed_lt" -e "$sed_gt"`
  func_section_wrap posix_ext_inodeset
  func_wrap H3
  func_echo "$element"

  func_begin_table
  func_module cycle-check
  func_module dev-ino
  func_module file-set
  func_module hash-triple
  func_module i-ring
  func_module same-inode
  func_end_table

  element="File descriptor based Input/Output"
  element=`printf "%s" "$element" | sed -e "$sed_lt" -e "$sed_gt"`
  func_section_wrap posix_ext_filedesc
  func_wrap H3
  func_echo "$element"

  func_begin_table
  func_module binary-io
  func_module copy-file-range
  func_module dup3
  func_module fcntl-safer
  func_module fd-safer-flag
  func_module full-read
  func_module full-write
  func_module getdtablesize
  func_module isapipe
  func_module openat-safer
  func_module pipe-posix
  func_module pipe2
  func_module pipe2-safer
  func_module safe-read
  func_module safe-write
  func_end_table

  element="File stream based Input/Output"
  element=`printf "%s" "$element" | sed -e "$sed_lt" -e "$sed_gt"`
  func_section_wrap posix_ext_filestream
  func_wrap H3
  func_echo "$element"

  func_begin_table
  func_module close-stream
  func_module closein
  func_module closeout
  func_module fbufmode
  func_module fopen-safer
  func_module fpending
  func_module fpurge
  func_module freadable
  func_module freadahead
  func_module freading
  func_module freadptr
  func_module freadseek
  func_module freopen-safer
  func_module fwritable
  func_module fwriting
  func_module getpass
  func_module getpass-gnu
  func_module popen-safer
  func_module stdlib-safer
  func_module tmpfile-safer
  func_module xfreopen
  func_end_table

  element="Users and groups"
  element=`printf "%s" "$element" | sed -e "$sed_lt" -e "$sed_gt"`
  func_section_wrap posix_ext_uidgid
  func_wrap H3
  func_echo "$element"

  func_begin_table
  func_module getugroups
  func_module group-member
  func_module idcache
  func_module mgetgroups
  func_module userspec
  func_end_table

  element="Security"
  element=`printf "%s" "$element" | sed -e "$sed_lt" -e "$sed_gt"`
  func_section_wrap posix_ext_security
  func_wrap H3
  func_echo "$element"

  func_begin_table
  func_module idpriv-drop
  func_module idpriv-droptemp
  func_module priv-set
  func_end_table

  element="Date and time"
  element=`printf "%s" "$element" | sed -e "$sed_lt" -e "$sed_gt"`
  func_section_wrap posix_ext_datetime
  func_wrap H3
  func_echo "$element"

  func_begin_table
  func_module gethrxtime
  func_module gettime
  func_module posixtm
  func_module settime
  func_module usleep
  func_module xnanosleep
  func_end_table

  element="Networking functions"
  element=`printf "%s" "$element" | sed -e "$sed_lt" -e "$sed_gt"`
  func_section_wrap posix_ext_net
  func_wrap H3
  func_echo "$element"

  func_begin_table
  func_module accept4
  func_module xgethostname
  func_module canon-host
  func_module sockets
  func_end_table

  element="Multithreading"
  element=`printf "%s" "$element" | sed -e "$sed_lt" -e "$sed_gt"`
  func_section_wrap posix_ext_thread
  func_wrap H3
  func_echo "$element"

  func_begin_table
  func_module threadlib
  func_module lock
  func_module tls
  func_module thread
  func_module yield
  func_module cond
  func_module openmp
  func_end_table

  element="Signal handling"
  element=`printf "%s" "$element" | sed -e "$sed_lt" -e "$sed_gt"`
  func_section_wrap posix_ext_signal
  func_wrap H3
  func_echo "$element"

  func_begin_table
  func_module c-stack
  func_module libsigsegv
  func_module sig2str
  func_module sigpipe
  func_module sigpipe-die
  func_end_table

  element="Internationalization functions"
  element=`printf "%s" "$element" | sed -e "$sed_lt" -e "$sed_gt"`
  func_section_wrap posix_ext_i18n
  func_wrap H3
  func_echo "$element"

  func_begin_table
  func_module gettext
  func_module gettext-h
  func_module propername
  func_module iconv
  func_module striconv
  func_module xstriconv
  func_module striconveh
  func_module xstriconveh
  func_module striconveha
  func_module localcharset
  func_module hard-locale
  func_module localename
  func_module mbmemcasecmp
  func_module mbmemcasecoll
  func_module mbslen
  func_module mbsnlen
  func_module mbschr
  func_module mbsrchr
  func_module mbsstr
  func_module mbscasecmp
  func_module mbsncasecmp
  func_module mbspcasecmp
  func_module mbscasestr
  func_module mbscspn
  func_module mbspbrk
  func_module mbsspn
  func_module mbssep
  func_module mbstok_r
  func_module mbswidth
  func_module memcasecmp
  func_module memcoll
  func_module xmemcoll
  func_module unicodeio
  func_module rpmatch
  func_module yesno
  func_module bison-i18n
  func_end_table

  element="Unicode string functions"
  element=`printf "%s" "$element" | sed -e "$sed_lt" -e "$sed_gt"`
  func_section_wrap posix_ext_unicode
  func_wrap H3
  func_echo "$element"

  func_begin_table
  func_module libunistring
  func_module libunistring-optional
  func_module unitypes
  func_module ucs4-utf8
  func_module ucs4-utf16
  func_module utf8-ucs4-unsafe
  func_module utf16-ucs4-unsafe
  func_module utf8-ucs4
  func_module utf16-ucs4
  func_module unistr/base
  func_module unistr/u8-to-u16
  func_module unistr/u8-to-u32
  func_module unistr/u16-to-u8
  func_module unistr/u16-to-u32
  func_module unistr/u32-to-u8
  func_module unistr/u32-to-u16
  func_module unistr/u8-check
  func_module unistr/u16-check
  func_module unistr/u32-check
  func_module unistr/u8-chr
  func_module unistr/u16-chr
  func_module unistr/u32-chr
  func_module unistr/u8-cmp
  func_module unistr/u16-cmp
  func_module unistr/u32-cmp
  func_module unistr/u8-cmp2
  func_module unistr/u16-cmp2
  func_module unistr/u32-cmp2
  func_module unistr/u8-cpy
  func_module unistr/u16-cpy
  func_module unistr/u32-cpy
  func_module unistr/u8-cpy-alloc
  func_module unistr/u16-cpy-alloc
  func_module unistr/u32-cpy-alloc
  func_module unistr/u8-endswith
  func_module unistr/u16-endswith
  func_module unistr/u32-endswith
  func_module unistr/u8-mblen
  func_module unistr/u16-mblen
  func_module unistr/u32-mblen
  func_module unistr/u8-mbsnlen
  func_module unistr/u16-mbsnlen
  func_module unistr/u32-mbsnlen
  func_module unistr/u8-mbtouc-unsafe
  func_module unistr/u16-mbtouc-unsafe
  func_module unistr/u32-mbtouc-unsafe
  func_module unistr/u8-mbtouc
  func_module unistr/u16-mbtouc
  func_module unistr/u32-mbtouc
  func_module unistr/u8-mbtoucr
  func_module unistr/u16-mbtoucr
  func_module unistr/u32-mbtoucr
  func_module unistr/u8-move
  func_module unistr/u16-move
  func_module unistr/u32-move
  func_module unistr/u8-next
  func_module unistr/u16-next
  func_module unistr/u32-next
  func_module unistr/u8-prev
  func_module unistr/u16-prev
  func_module unistr/u32-prev
  func_module unistr/u8-set
  func_module unistr/u16-set
  func_module unistr/u32-set
  func_module unistr/u8-startswith
  func_module unistr/u16-startswith
  func_module unistr/u32-startswith
  func_module unistr/u8-stpcpy
  func_module unistr/u16-stpcpy
  func_module unistr/u32-stpcpy
  func_module unistr/u8-stpncpy
  func_module unistr/u16-stpncpy
  func_module unistr/u32-stpncpy
  func_module unistr/u8-strcat
  func_module unistr/u16-strcat
  func_module unistr/u32-strcat
  func_module unistr/u8-strchr
  func_module unistr/u16-strchr
  func_module unistr/u32-strchr
  func_module unistr/u8-strcmp
  func_module unistr/u16-strcmp
  func_module unistr/u32-strcmp
  func_module unistr/u8-strcoll
  func_module unistr/u16-strcoll
  func_module unistr/u32-strcoll
  func_module unistr/u8-strcpy
  func_module unistr/u16-strcpy
  func_module unistr/u32-strcpy
  func_module unistr/u8-strcspn
  func_module unistr/u16-strcspn
  func_module unistr/u32-strcspn
  func_module unistr/u8-strdup
  func_module unistr/u16-strdup
  func_module unistr/u32-strdup
  func_module unistr/u8-strlen
  func_module unistr/u16-strlen
  func_module unistr/u32-strlen
  func_module unistr/u8-strmblen
  func_module unistr/u16-strmblen
  func_module unistr/u32-strmblen
  func_module unistr/u8-strmbtouc
  func_module unistr/u16-strmbtouc
  func_module unistr/u32-strmbtouc
  func_module unistr/u8-strncat
  func_module unistr/u16-strncat
  func_module unistr/u32-strncat
  func_module unistr/u8-strncmp
  func_module unistr/u16-strncmp
  func_module unistr/u32-strncmp
  func_module unistr/u8-strncpy
  func_module unistr/u16-strncpy
  func_module unistr/u32-strncpy
  func_module unistr/u8-strnlen
  func_module unistr/u16-strnlen
  func_module unistr/u32-strnlen
  func_module unistr/u8-strpbrk
  func_module unistr/u16-strpbrk
  func_module unistr/u32-strpbrk
  func_module unistr/u8-strrchr
  func_module unistr/u16-strrchr
  func_module unistr/u32-strrchr
  func_module unistr/u8-strspn
  func_module unistr/u16-strspn
  func_module unistr/u32-strspn
  func_module unistr/u8-strstr
  func_module unistr/u16-strstr
  func_module unistr/u32-strstr
  func_module unistr/u8-strtok
  func_module unistr/u16-strtok
  func_module unistr/u32-strtok
  func_module unistr/u8-uctomb
  func_module unistr/u16-uctomb
  func_module unistr/u32-uctomb
  func_module uniconv/base
  func_module uniconv/u8-conv-from-enc
  func_module uniconv/u16-conv-from-enc
  func_module uniconv/u32-conv-from-enc
  func_module uniconv/u8-conv-to-enc
  func_module uniconv/u16-conv-to-enc
  func_module uniconv/u32-conv-to-enc
  func_module uniconv/u8-strconv-from-enc
  func_module uniconv/u16-strconv-from-enc
  func_module uniconv/u32-strconv-from-enc
  func_module uniconv/u8-strconv-to-enc
  func_module uniconv/u16-strconv-to-enc
  func_module uniconv/u32-strconv-to-enc
  func_module uniconv/u8-strconv-from-locale
  func_module uniconv/u16-strconv-from-locale
  func_module uniconv/u32-strconv-from-locale
  func_module uniconv/u8-strconv-to-locale
  func_module uniconv/u16-strconv-to-locale
  func_module uniconv/u32-strconv-to-locale
  func_module unistdio/base
  func_module unistdio/u-printf-args
  func_module unistdio/ulc-asnprintf
  func_module unistdio/ulc-asprintf
  func_module unistdio/ulc-fprintf
  func_module unistdio/ulc-printf-parse
  func_module unistdio/ulc-snprintf
  func_module unistdio/ulc-sprintf
  func_module unistdio/ulc-vasnprintf
  func_module unistdio/ulc-vasprintf
  func_module unistdio/ulc-vfprintf
  func_module unistdio/ulc-vsnprintf
  func_module unistdio/ulc-vsprintf
  func_module unistdio/u8-asnprintf
  func_module unistdio/u8-asprintf
  func_module unistdio/u8-printf-parse
  func_module unistdio/u8-snprintf
  func_module unistdio/u8-sprintf
  func_module unistdio/u8-vasnprintf
  func_module unistdio/u8-vasprintf
  func_module unistdio/u8-vsnprintf
  func_module unistdio/u8-vsprintf
  func_module unistdio/u8-u8-asnprintf
  func_module unistdio/u8-u8-asprintf
  func_module unistdio/u8-u8-snprintf
  func_module unistdio/u8-u8-sprintf
  func_module unistdio/u8-u8-vasnprintf
  func_module unistdio/u8-u8-vasprintf
  func_module unistdio/u8-u8-vsnprintf
  func_module unistdio/u8-u8-vsprintf
  func_module unistdio/u16-asnprintf
  func_module unistdio/u16-asprintf
  func_module unistdio/u16-printf-parse
  func_module unistdio/u16-snprintf
  func_module unistdio/u16-sprintf
  func_module unistdio/u16-vasnprintf
  func_module unistdio/u16-vasprintf
  func_module unistdio/u16-vsnprintf
  func_module unistdio/u16-vsprintf
  func_module unistdio/u16-u16-asnprintf
  func_module unistdio/u16-u16-asprintf
  func_module unistdio/u16-u16-snprintf
  func_module unistdio/u16-u16-sprintf
  func_module unistdio/u16-u16-vasnprintf
  func_module unistdio/u16-u16-vasprintf
  func_module unistdio/u16-u16-vsnprintf
  func_module unistdio/u16-u16-vsprintf
  func_module unistdio/u32-asnprintf
  func_module unistdio/u32-asprintf
  func_module unistdio/u32-printf-parse
  func_module unistdio/u32-snprintf
  func_module unistdio/u32-sprintf
  func_module unistdio/u32-vasnprintf
  func_module unistdio/u32-vasprintf
  func_module unistdio/u32-vsnprintf
  func_module unistdio/u32-vsprintf
  func_module unistdio/u32-u32-asnprintf
  func_module unistdio/u32-u32-asprintf
  func_module unistdio/u32-u32-snprintf
  func_module unistdio/u32-u32-sprintf
  func_module unistdio/u32-u32-vasnprintf
  func_module unistdio/u32-u32-vasprintf
  func_module unistdio/u32-u32-vsnprintf
  func_module unistdio/u32-u32-vsprintf
  func_module uniname/base
  func_module uniname/uniname
  func_module unictype/base
  func_module unictype/bidiclass-byname
  func_module unictype/bidiclass-name
  func_module unictype/bidiclass-longname
  func_module unictype/bidiclass-of
  func_module unictype/bidiclass-test
  func_module unictype/bidiclass-all
  func_module unictype/block-list
  func_module unictype/block-of
  func_module unictype/block-test
  func_module unictype/block-all
  func_module unictype/category-C
  func_module unictype/category-Cc
  func_module unictype/category-Cf
  func_module unictype/category-Cn
  func_module unictype/category-Co
  func_module unictype/category-Cs
  func_module unictype/category-L
  func_module unictype/category-Ll
  func_module unictype/category-Lm
  func_module unictype/category-Lo
  func_module unictype/category-Lt
  func_module unictype/category-Lu
  func_module unictype/category-M
  func_module unictype/category-Mc
  func_module unictype/category-Me
  func_module unictype/category-Mn
  func_module unictype/category-N
  func_module unictype/category-Nd
  func_module unictype/category-Nl
  func_module unictype/category-No
  func_module unictype/category-P
  func_module unictype/category-Pc
  func_module unictype/category-Pd
  func_module unictype/category-Pe
  func_module unictype/category-Pf
  func_module unictype/category-Pi
  func_module unictype/category-Po
  func_module unictype/category-Ps
  func_module unictype/category-S
  func_module unictype/category-Sc
  func_module unictype/category-Sk
  func_module unictype/category-Sm
  func_module unictype/category-So
  func_module unictype/category-Z
  func_module unictype/category-Zl
  func_module unictype/category-Zp
  func_module unictype/category-Zs
  func_module unictype/category-and
  func_module unictype/category-and-not
  func_module unictype/category-byname
  func_module unictype/category-name
  func_module unictype/category-none
  func_module unictype/category-of
  func_module unictype/category-or
  func_module unictype/category-test
  func_module unictype/category-test-withtable
  func_module unictype/category-all
  func_module unictype/combining-class
  func_module unictype/ctype-alnum
  func_module unictype/ctype-alpha
  func_module unictype/ctype-blank
  func_module unictype/ctype-cntrl
  func_module unictype/ctype-digit
  func_module unictype/ctype-graph
  func_module unictype/ctype-lower
  func_module unictype/ctype-print
  func_module unictype/ctype-punct
  func_module unictype/ctype-space
  func_module unictype/ctype-upper
  func_module unictype/ctype-xdigit
  func_module unictype/decimal-digit
  func_module unictype/digit
  func_module unictype/mirror
  func_module unictype/numeric
  func_module unictype/property-alphabetic
  func_module unictype/property-ascii-hex-digit
  func_module unictype/property-bidi-arabic-digit
  func_module unictype/property-bidi-arabic-right-to-left
  func_module unictype/property-bidi-block-separator
  func_module unictype/property-bidi-boundary-neutral
  func_module unictype/property-bidi-common-separator
  func_module unictype/property-bidi-control
  func_module unictype/property-bidi-embedding-or-override
  func_module unictype/property-bidi-eur-num-separator
  func_module unictype/property-bidi-eur-num-terminator
  func_module unictype/property-bidi-european-digit
  func_module unictype/property-bidi-hebrew-right-to-left
  func_module unictype/property-bidi-left-to-right
  func_module unictype/property-bidi-non-spacing-mark
  func_module unictype/property-bidi-other-neutral
  func_module unictype/property-bidi-pdf
  func_module unictype/property-bidi-segment-separator
  func_module unictype/property-bidi-whitespace
  func_module unictype/property-byname
  func_module unictype/property-case-ignorable
  func_module unictype/property-cased
  func_module unictype/property-changes-when-casefolded
  func_module unictype/property-changes-when-casemapped
  func_module unictype/property-changes-when-lowercased
  func_module unictype/property-changes-when-titlecased
  func_module unictype/property-changes-when-uppercased
  func_module unictype/property-combining
  func_module unictype/property-composite
  func_module unictype/property-currency-symbol
  func_module unictype/property-dash
  func_module unictype/property-decimal-digit
  func_module unictype/property-default-ignorable-code-point
  func_module unictype/property-deprecated
  func_module unictype/property-diacritic
  func_module unictype/property-extender
  func_module unictype/property-format-control
  func_module unictype/property-grapheme-base
  func_module unictype/property-grapheme-extend
  func_module unictype/property-grapheme-link
  func_module unictype/property-hex-digit
  func_module unictype/property-hyphen
  func_module unictype/property-id-continue
  func_module unictype/property-id-start
  func_module unictype/property-ideographic
  func_module unictype/property-ids-binary-operator
  func_module unictype/property-ids-trinary-operator
  func_module unictype/property-ignorable-control
  func_module unictype/property-iso-control
  func_module unictype/property-join-control
  func_module unictype/property-left-of-pair
  func_module unictype/property-line-separator
  func_module unictype/property-logical-order-exception
  func_module unictype/property-lowercase
  func_module unictype/property-math
  func_module unictype/property-non-break
  func_module unictype/property-not-a-character
  func_module unictype/property-numeric
  func_module unictype/property-other-alphabetic
  func_module unictype/property-other-default-ignorable-code-point
  func_module unictype/property-other-grapheme-extend
  func_module unictype/property-other-id-continue
  func_module unictype/property-other-id-start
  func_module unictype/property-other-lowercase
  func_module unictype/property-other-math
  func_module unictype/property-other-uppercase
  func_module unictype/property-paired-punctuation
  func_module unictype/property-paragraph-separator
  func_module unictype/property-pattern-syntax
  func_module unictype/property-pattern-white-space
  func_module unictype/property-private-use
  func_module unictype/property-punctuation
  func_module unictype/property-quotation-mark
  func_module unictype/property-radical
  func_module unictype/property-sentence-terminal
  func_module unictype/property-soft-dotted
  func_module unictype/property-space
  func_module unictype/property-terminal-punctuation
  func_module unictype/property-test
  func_module unictype/property-titlecase
  func_module unictype/property-unassigned-code-value
  func_module unictype/property-unified-ideograph
  func_module unictype/property-uppercase
  func_module unictype/property-variation-selector
  func_module unictype/property-white-space
  func_module unictype/property-xid-continue
  func_module unictype/property-xid-start
  func_module unictype/property-zero-width
  func_module unictype/property-all
  func_module unictype/scripts
  func_module unictype/scripts-all
  func_module unictype/syntax-c-ident
  func_module unictype/syntax-c-whitespace
  func_module unictype/syntax-java-ident
  func_module unictype/syntax-java-whitespace
  func_module uniwidth/base
  func_module uniwidth/u8-strwidth
  func_module uniwidth/u8-width
  func_module uniwidth/u16-strwidth
  func_module uniwidth/u16-width
  func_module uniwidth/u32-strwidth
  func_module uniwidth/u32-width
  func_module uniwidth/width
  func_module uniwbrk/base
  func_module uniwbrk/ulc-wordbreaks
  func_module uniwbrk/u8-wordbreaks
  func_module uniwbrk/u16-wordbreaks
  func_module uniwbrk/u32-wordbreaks
  func_module uniwbrk/wordbreak-property
  func_module unilbrk/base
  func_module unilbrk/tables
  func_module unilbrk/ulc-common
  func_module unilbrk/u8-possible-linebreaks
  func_module unilbrk/u16-possible-linebreaks
  func_module unilbrk/u32-possible-linebreaks
  func_module unilbrk/ulc-possible-linebreaks
  func_module unilbrk/u8-width-linebreaks
  func_module unilbrk/u16-width-linebreaks
  func_module unilbrk/u32-width-linebreaks
  func_module unilbrk/ulc-width-linebreaks
  func_module uninorm/base
  func_module uninorm/canonical-decomposition
  func_module uninorm/composition
  func_module uninorm/decomposing-form
  func_module uninorm/decomposition
  func_module uninorm/filter
  func_module uninorm/nfc
  func_module uninorm/nfd
  func_module uninorm/nfkc
  func_module uninorm/nfkd
  func_module uninorm/u8-normalize
  func_module uninorm/u16-normalize
  func_module uninorm/u32-normalize
  func_module uninorm/u8-normcmp
  func_module uninorm/u16-normcmp
  func_module uninorm/u32-normcmp
  func_module uninorm/u8-normcoll
  func_module uninorm/u16-normcoll
  func_module uninorm/u32-normcoll
  func_module uninorm/u8-normxfrm
  func_module uninorm/u16-normxfrm
  func_module uninorm/u32-normxfrm
  func_module unicase/base
  func_module unicase/empty-prefix-context
  func_module unicase/empty-suffix-context
  func_module unicase/locale-language
  func_module unicase/tolower
  func_module unicase/totitle
  func_module unicase/toupper
  func_module unicase/ulc-casecmp
  func_module unicase/ulc-casecoll
  func_module unicase/ulc-casexfrm
  func_module unicase/u8-casecmp
  func_module unicase/u16-casecmp
  func_module unicase/u32-casecmp
  func_module unicase/u8-casecoll
  func_module unicase/u16-casecoll
  func_module unicase/u32-casecoll
  func_module unicase/u8-casefold
  func_module unicase/u16-casefold
  func_module unicase/u32-casefold
  func_module unicase/u8-casexfrm
  func_module unicase/u16-casexfrm
  func_module unicase/u32-casexfrm
  func_module unicase/u8-ct-casefold
  func_module unicase/u16-ct-casefold
  func_module unicase/u32-ct-casefold
  func_module unicase/u8-ct-tolower
  func_module unicase/u16-ct-tolower
  func_module unicase/u32-ct-tolower
  func_module unicase/u8-ct-totitle
  func_module unicase/u16-ct-totitle
  func_module unicase/u32-ct-totitle
  func_module unicase/u8-ct-toupper
  func_module unicase/u16-ct-toupper
  func_module unicase/u32-ct-toupper
  func_module unicase/u8-is-cased
  func_module unicase/u16-is-cased
  func_module unicase/u32-is-cased
  func_module unicase/u8-is-casefolded
  func_module unicase/u16-is-casefolded
  func_module unicase/u32-is-casefolded
  func_module unicase/u8-is-lowercase
  func_module unicase/u16-is-lowercase
  func_module unicase/u32-is-lowercase
  func_module unicase/u8-is-titlecase
  func_module unicase/u16-is-titlecase
  func_module unicase/u32-is-titlecase
  func_module unicase/u8-is-uppercase
  func_module unicase/u16-is-uppercase
  func_module unicase/u8-prefix-context
  func_module unicase/u16-prefix-context
  func_module unicase/u32-prefix-context
  func_module unicase/u8-suffix-context
  func_module unicase/u16-suffix-context
  func_module unicase/u32-suffix-context
  func_module unicase/u8-tolower
  func_module unicase/u16-tolower
  func_module unicase/u32-tolower
  func_module unicase/u8-totitle
  func_module unicase/u16-totitle
  func_module unicase/u32-totitle
  func_module unicase/u8-toupper
  func_module unicase/u16-toupper
  func_module unicase/u32-toupper
  func_end_table

  element="Executing programs"
  element=`printf "%s" "$element" | sed -e "$sed_lt" -e "$sed_gt"`
  func_section_wrap posix_ext_exec
  func_wrap H3
  func_echo "$element"

  func_begin_table
  func_module cloexec
  func_module findprog
  func_module findprog-lgpl
  func_module wait-process
  func_module execute
  func_module spawn-pipe
  func_module pipe-filter-gi
  func_module pipe-filter-ii
  func_module sh-quote
  func_end_table

  element="Java"
  element=`printf "%s" "$element" | sed -e "$sed_lt" -e "$sed_gt"`
  func_section_wrap posix_ext_java
  func_wrap H3
  func_echo "$element"

  func_begin_table
  func_module classpath
  func_module javacomp-script
  func_module javacomp
  func_module javaexec-script
  func_module javaexec
  func_module javaversion
  #func_module gcj
  func_end_table

  element="C#"
  element=`printf "%s" "$element" | sed -e "$sed_lt" -e "$sed_gt"`
  func_section_wrap posix_ext_csharp
  func_wrap H3
  func_echo "$element"

  func_begin_table
  func_module csharpcomp-script
  func_module csharpcomp
  func_module csharpexec-script
  func_module csharpexec
  func_end_table

  element="Misc"
  element=`printf "%s" "$element" | sed -e "$sed_lt" -e "$sed_gt"`
  func_section_wrap posix_ext_misc
  func_wrap H3
  func_echo "$element"

  func_begin_table
  func_module argp
  func_module argp-version-etc
  func_module argz
  func_module bitrotate
  func_module byteswap
  func_module dfa
  func_module exitfail
  func_module error
  func_module extensions
  func_module forkpty
  func_module getdomainname
  func_module xgetdomainname
  func_module getentropy
  func_module getloadavg
  func_module getpagesize
  func_module getprogname
  func_module getrandom
  func_module getusershell
  func_module lib-symbol-visibility
  func_module login_tty
  func_module nproc
  func_module openpty
  func_module parse-duration
  func_module physmem
  func_module posixver
  func_module progname
  func_module ptsname_r
  func_module pty
  func_module quotearg
  func_module quote
  func_module readutmp
  func_module random_r
  func_module selinux-h
  func_module selinux-at
  func_module sysexits
  func_module u64
  func_module verror
  func_end_table

  element="Support for building libraries and executables"
  func_section_wrap build_lib
  func_wrap H2
  func_echo "$element"

  func_begin_table
  func_module absolute-header
  func_module snippet/arg-nonnull
  func_module config-h
  func_module configmake
  func_module dummy
  func_module gperf
  func_module havelib
  func_module include_next
  func_module ldd
  func_module lib-ignore
  func_module lib-msvc-compat
  func_module lib-symbol-versions
  func_module snippet/link-warning
  func_module manywarnings
  func_module no-c++
  func_module relocatable-lib
  func_module relocatable-lib-lgpl
  func_module relocatable-prog
  func_module relocatable-prog-wrapper
  func_module relocatable-script
  func_module snippet/warn-on-use
  func_module warnings
  func_end_table

  element="Support for building documentation"
  func_section_wrap build_doc
  func_wrap H2
  func_echo "$element"

  func_begin_table
  func_module agpl-3.0
  func_module fdl
  func_module fdl-1.3
  func_module gendocs
  func_module gpl-2.0
  func_module gpl-3.0
  func_module lgpl-2.1
  func_module lgpl-3.0
  func_module pmccabe2html
  func_module regexprops-generic
  func_end_table

  element="Support for maintaining and releasing projects"
  func_section_wrap maintain
  func_wrap H2
  func_echo "$element"

  func_begin_table
  func_module announce-gen
  func_module autobuild
  func_module do-release-commit-and-tag
  func_module git-version-gen
  func_module gitlog-to-changelog
  func_module gnu-web-doc-update
  func_module gnumakefile
  func_module gnupload
  func_module maintainer-makefile
  func_module mktempd
  func_module non-recursive-gnulib-prefix-hack
  func_module readme-release
  func_module test-framework-sh
  func_module update-copyright
  func_module useless-if-before-free
  func_module vc-list-files
  func_module vcs-to-changelog
  func_end_table

  element="Misc"
  func_section_wrap misc
  func_wrap H2
  func_echo "$element"

  func_begin_table
  func_module gnu-make
  func_module host-os
  func_module nocrash
  func_module perl
  func_module posix-shell
  func_module uptime
  func_end_table
}


func_tmpdir
trap 'exit_status=$?
      if test "$signal" != 0; then
        echo "caught signal $signal" >&2
      fi
      rm -rf "$tmp"
      exit $exit_status' 0
for signal in 1 2 3 13 15; do
  trap '{ signal='$signal'; func_exit 1; }' $signal
done
signal=0

echo '<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 3.2//EN">'
func_begin HTML

func_begin HEAD

element="Gnulib Module List"
func_wrap TITLE
func_echo "$element"

modules=`gnulib-tool --list`
modules=`for m in $modules; do printf "%s, " $m; done | sed -e 's/, $//'`
func_echo '<META NAME="keywords" CONTENT="'"${modules}"'">'

func_end HEAD

func_begin BODY

element="Gnulib Module List"
func_wrap H1
func_echo "$element"

func_echo 'This is a list of the modules which make up gnulib, with dependencies.'

in_toc=yes
func_all_modules

in_toc=
func_all_modules


gnulib-tool --list > "$tmp/all-modules"
missed_modules=`for module in $seen_modules; do echo $module; done \
                  | LC_ALL=C sort -u \
                  | LC_ALL=C join -v 2 - "$tmp/all-modules"`

if test -n "$missed_modules"; then

  element="Unclassified modules - please update MODULES.html.sh"
  element=`printf "%s" "$element" | sed -e "$sed_lt" -e "$sed_gt"`
  func_wrap H2
  func_echo "$element"

  func_begin_table
  for module in $missed_modules; do
    func_module $module
  done
  func_end_table

fi

{ find lib -type f -print; find m4 -type f -print; } | LC_ALL=C sort | sed -e '/\/\./d' -e /README/d -e /ChangeLog/d -e /Makefile/d -e /TODO/d -e '/tags$/d' -e '/TAGS$/d' -e '/~$/d' > "$tmp/all-files"
missed_files=`for file in $seen_files; do echo $file; done \
                | LC_ALL=C sort -u \
                | LC_ALL=C join -v 2 - "$tmp/all-files"`

if test -n "$missed_files"; then

  element="Lone files - please create new modules containing them"
  element=`printf "%s" "$element" | sed -e "$sed_lt" -e "$sed_gt"`
  func_wrap H2
  func_echo "$element"

  func_echo '<PRE>'
  echo "$missed_files" | sed -e 's,^\(.*\)$,<A HREF="'"$repo_url_prefix"'\1'"$repo_url_suffix_repl"'">\1</A>,'
  echo '</PRE>'

fi

element="Future developments"
element=`printf "%s" "$element" | sed -e "$sed_lt" -e "$sed_gt"`
func_wrap H2
func_echo "$element"

func_echo 'Ideally a module could consist of:'
func_begin UL
func_echo '<LI>A header file: lib/<VAR>module</VAR>.h'
func_echo '<LI>One or more implementation files: lib/<VAR>module</VAR>.c et al.'
func_echo '<LI>One or more autoconf macro files: m4/<VAR>module</VAR>.m4 et al.'
func_echo '<LI>A configure.ac fragment, Makefile.am fragment, dependency list: modules/<VAR>module</VAR>'
func_echo '<LI>A testsuite: source files in tests/ and metainformation (a configure.ac fragment, Makefile.am fragment, dependency list) in modules/<VAR>module</VAR>-tests'
func_echo '<LI>Some documentation'
func_echo '<LI>A POT file and some PO files'
func_end UL

func_echo '<HR>'
git_checkout_date=`if test -d .git; then
                     git log -n 1 --date=iso --format=fuller | sed -n -e 's/^CommitDate: //p';
                   else
                     sed -n -e 's/^\([0-9][0-9][0-9][0-9]-[0-9][0-9]-[0-9][0-9]\).*/\1/p' -e 1q ChangeLog;
                   fi`
pretty_date=`LC_ALL=C date +"%e %B %Y" --date="$git_checkout_date"`
func_echo "Generated by <CODE>MODULES.html.sh</CODE> from a git checkout as of ${pretty_date}."

func_end BODY

func_end HTML

rm -rf "$tmp"
# Undo the effect of the previous 'trap' command.
trap '' 0
trap 'func_exit $?' 1 2 3 13 15

exit 0

# Local Variables:
# indent-tabs-mode: nil
# whitespace-check-buffer-indent: nil
# End:
