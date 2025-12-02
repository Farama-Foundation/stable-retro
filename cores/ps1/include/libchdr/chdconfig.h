#ifndef __CHDCONFIG_H__
#define __CHDCONFIG_H__

/* this overrides deps/libchdr/include/libchdr/chdconfig.h */
#define WANT_SUBCODE            1
#define NEED_CACHE_HUNK         1

#if defined(__x86_64__) || defined(__aarch64__)
#define WANT_RAW_DATA_SECTOR    1
#define VERIFY_BLOCK_CRC        1
#else
// assume some slower hw so no ecc that most (all?) games don't need
#endif

#endif
