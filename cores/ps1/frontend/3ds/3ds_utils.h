#ifndef _3DS_UTILS_H
#define _3DS_UTILS_H

#ifndef USE_CTRULIB_2
#error CTRULIB_2 is required
#endif

#define MEMOP_PROT      6
#define MEMOP_MAP       4
#define MEMOP_UNMAP     5

#define DEBUG_HOLD() do{printf("%s@%s:%d.\n",__FUNCTION__, __FILE__, __LINE__);fflush(stdout);wait_for_input();}while(0)

void wait_for_input(void);
void ctr_clear_cache(void);
void ctr_clear_cache_range(void *start, void *end);
void ctr_invalidate_icache(void); // only icache
int ctr_get_tlbdesc(void *ptr);

int svcCustomBackdoor(void *callback, void *a0, void *a1, void *a2);
int svcConvertVAToPA(const void *VA, int writeCheck);

extern __attribute__((weak)) int  __ctr_svchax;

#endif // _3DS_UTILS_H
