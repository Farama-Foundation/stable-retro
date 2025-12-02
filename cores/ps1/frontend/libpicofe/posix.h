#ifndef LIBPICOFE_POSIX_H
#define LIBPICOFE_POSIX_H

/* define POSIX stuff: dirent, scandir, getcwd, mkdir */
#if defined(__FreeBSD__) || defined(__MACH__) || defined(__linux__) || defined(__MINGW32__) || defined(__PSP__) || defined(__PS2__)

#include <dirent.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

#ifdef __MINGW32__
#warning hacks!
#define mkdir(pathname,mode) mkdir(pathname)
#define d_type d_ino
#define DT_REG 0
#define DT_DIR 1
#define DT_LNK 2
#define DT_UNKNOWN -1
#define readlink(p,d,s)		-1
#endif

#else

#error "must provide posix"

#endif

#endif // LIBPICOFE_POSIX_H
