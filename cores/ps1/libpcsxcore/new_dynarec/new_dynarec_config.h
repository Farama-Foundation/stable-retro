
#ifdef __arm__
#define CORTEX_A8_BRANCH_PREDICTION_HACK 1
#endif

#define USE_MINI_HT 1
//#define REG_PREFETCH 1

// options:
//#define NO_WRITE_EXEC 1
//#define BASE_ADDR_DYNAMIC 1
//#define TC_WRITE_OFFSET 1
//#define NDRC_CACHE_FLUSH_ALL 1

#if defined(__MACH__) || defined(HAVE_LIBNX)
#define NO_WRITE_EXEC 1
#endif
#if defined(VITA) || defined(HAVE_LIBNX)
#define BASE_ADDR_DYNAMIC 1
#endif
#if defined(HAVE_LIBNX)
#define TC_WRITE_OFFSET 1
#endif
#if defined(_3DS)
#define NDRC_CACHE_FLUSH_ALL 1
#endif
