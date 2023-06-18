#ifndef LOG_H
#define LOG_H

#include <libretro.h>

/******************************************************************************

	Shared libretro log interface
    set in mame2003.c

******************************************************************************/

#define LOGPRE          "[MAME 2003+] "

extern retro_log_printf_t log_cb;


/* logerror is a holdover from the MAME log system. MAME 0.78 was evidently
   trying to standardize on logerror but the standardization was not complete,
   with a dozen variations on indicating loging verbosity via defines,
   some code still using printf, etc.

   Due to the fact that the logging in MAME was not internally consistent,
   and that MAME's logging levels don't necessarily correspond to libretro's
   levels of RETRO_LOG_DEBUG, RETRO_LOG_INFO, RETRO_LOG_WARN, and RETRO_LOG_ERROR,
   there is manual correction to the logging levels needed in some cases.

   TODO: Remove logerror once the conversion to libretro logging is "done.
*/

#ifdef __GNUC__
static INLINE void CLIB_DECL logerror(const char *text,...) __attribute__ ((format (printf, 1, 2)));
#endif

static INLINE void CLIB_DECL logerror(const char *text,...)
{
    va_list args;
    va_start (args, text);
    log_cb(RETRO_LOG_DEBUG, text, args);
    va_end (args);
}

#endif /* LOG_H */
