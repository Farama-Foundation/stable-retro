#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <sys/time.h>

#define L1CACHESIZE (16*1024)
#define L2CACHESIZE (128*1024)
#define KILOBYTE (1024)
#define MEGABYTE (1024*1024)

#define TESTSIZE (40*MEGABYTE)

#define TILEWIDTH (32)
#define TINYWIDTH (8)

#if 1
#define CANDIDATE memcpy
#define CANDIDATE_RETURN_TYPE void *
#elif 1
#define CANDIDATE memset
#define CANDIDATE_RETURN_TYPE void *
#elif 1
#define CANDIDATE memcmp
#define CANDIDATE_RETURN_TYPE int
#endif


/* Just used for cancelling out the overheads */
static CANDIDATE_RETURN_TYPE control(const void *s1, const void *s2, size_t n)
{
    return 0;
}

static uint64_t gettime(void)
{
    struct timeval tv;

    gettimeofday (&tv, NULL);
    return tv.tv_sec * 1000000 + tv.tv_usec;
}

static uint32_t bench_L(CANDIDATE_RETURN_TYPE (*test)(), char *a, char *b, size_t len, size_t times)
{
    int i, j, x = 0, q = 0;
    volatile int qx;
    for (i = times; i >= 0; i--)
    {
        /* Ensure the destination is in cache (if it gets flushed out, source gets reloaded anyway) */
        for (j = 0; j < len; j += 32)
            q += a[j];
        q += a[len-1];
        x = (x + 1) & 63;
        test(a + x, b + 63 - x, len);
    }
    qx = q;
    return len * times;
}

static uint32_t bench_M(CANDIDATE_RETURN_TYPE (*test)(), char *a, char *b, size_t len, size_t times)
{
    int i, x = 0;
    for (i = times; i >= 0; i--)
    {
        x = (x + 1) & 63;
        test(a + x, b + 63 - x, len);
    }
    return len * times;
}

static uint32_t bench_T(CANDIDATE_RETURN_TYPE (*test)(), char *a, char *b, size_t times)
{
    uint32_t total = 0;
    int i, x = 0;

    srand (0);
    for (i = times; i >= 0; i--)
    {
        int w = (rand () % (TILEWIDTH * 2)) + 1;
        if (x + w > MEGABYTE)
            x = 0;
        test(a + x, b + x, w);
        x += w;
        total += w;
    }
    return total;
}

static uint32_t bench_R(CANDIDATE_RETURN_TYPE (*test)(), char *a, char *b, size_t times)
{
    uint32_t total = 0;
    int i;

    srand (0);
    for (i = times; i >= 0; i--)
    {
        int w = (rand () % (TILEWIDTH * 2)) + 1;
        int ax = (rand() % (MEGABYTE - TILEWIDTH * 2));
        int bx = (rand() % (MEGABYTE - TILEWIDTH * 2));
        test(a + ax, b + bx, w);
        total += w;
    }
    return total;
}

static uint32_t bench_RW(CANDIDATE_RETURN_TYPE (*test)(), char *a, char *b, size_t w, size_t times)
{
    uint32_t total = 0;
    int i;

    srand (0);
    for (i = times; i >= 0; i--)
    {
        int ax = (rand() % (MEGABYTE - 1024));
        int bx = (rand() % (MEGABYTE - 1024));
        test(a + ax, b + bx, w);
        total += w;
    }
    return total;
}

static uint32_t bench_RT(CANDIDATE_RETURN_TYPE (*test)(), char *a, char *b, size_t times)
{
    uint32_t total = 0;
    int i;

    srand (0);
    for (i = times; i >= 0; i--)
    {
        int w = (rand () % (TINYWIDTH * 2)) + 1;
        int ax = (rand() % (MEGABYTE - TINYWIDTH * 2));
        int bx = (rand() % (MEGABYTE - TINYWIDTH * 2));
        test(a + ax, b + bx, w);
        total += w;
    }
    return total;
}

int main(int argc, char *argv[])
{
    static __attribute__((aligned(32))) char l1bufa[L1CACHESIZE/2-KILOBYTE];
    static __attribute__((aligned(32))) char l1bufb[L1CACHESIZE/2-KILOBYTE];
    static __attribute__((aligned(32))) char l2bufa[L2CACHESIZE/2-KILOBYTE];
    static __attribute__((aligned(32))) char l2bufb[L2CACHESIZE/2-KILOBYTE];
    static __attribute__((aligned(32))) char membufa[MEGABYTE];
    static __attribute__((aligned(32))) char membufb[MEGABYTE];
    size_t s, d, n;
    uint64_t t1, t2, t3;
    uint32_t byte_cnt;
    size_t iterations;

    srand(0);

    if (argc != 2)
    {
        fprintf(stderr, "Syntax: %s <iterations>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    iterations = atoi(argv[1]);

    memset(l1bufa, 0x5A, sizeof l1bufa);
    memset(l1bufb, 0x5A, sizeof l1bufb);
    memset(l2bufa, 0x5A, sizeof l2bufa);
    memset(l2bufb, 0x5A, sizeof l2bufb);
    memset(membufa, 0x5A, sizeof membufa);
    memset(membufb, 0x5A, sizeof membufb);

    // This code was useful for correctness checking.
    // The "my" prefix was used during development to enable the test harness to function
    // even when the local implementations were buggy.
#if 0
    void *mymemset(void *s, int c, size_t n);
    void *mymemcpy(void * restrict s1, const void * restrict s2, size_t n);
    void *mymemmove(void *s1, const void *s2, size_t n);
    int   mymemcmp(const void *s1, const void *s2, size_t n);

// These defines are used to prove that the test harness is correct - to test the local
// implementations, comment out the #define
#define mymemset memset
#define mymemcmp memcmp
#define mymemcpy memcpy
    /* Check mymemset */
    for (d = 0; d < 64; d++)
    {
        for (n = 0; n < 192; n++)
        {
            memset(l1bufa+d, 0xA5, n);
            mymemset(l1bufa+d, 0x5A, n);
            if (memcmp(l1bufa, l1bufb, sizeof l1bufa) != 0)
            {
                printf("memset failed (insufficient) with d = %d, n = %d\n", d, n);
                for (int x = 0; x < sizeof l1bufa; x++)
                    if (l1bufa[x] != 0x5A)
                        printf("Offset %d is wrong\n", x);
            }
            mymemset(l1bufa+d, 0xA5, n);
            memset(l1bufa+d, 0x5A, n);
            if (memcmp(l1bufa, l1bufb, sizeof l1bufa) != 0)
            {
                printf("memset failed (excessive) with d = %d, n = %d\n", d, n);
                for (int x = 0; x < sizeof l1bufa; x++)
                    if (l1bufa[x] != 0x5A)
                        printf("Offset %d is wrong\n", x);
            }
        }
    }

    /* Check memcmp */
    {
#define SIGNOF(x) (((x)>0)-((x)<0))
        uint32_t a = 0x00010200, b = 0x00020100;
        int d1,d2;
        if ((d1=SIGNOF(memcmp(l1bufa, l1bufb, sizeof l1bufa))) != (d2=SIGNOF(mymemcmp(l1bufa, l1bufb, sizeof l1bufa))))
            printf("memcmp failed (0: %d %d)\n", d1, d2);
        if ((d1=SIGNOF(memcmp(&a, &b, 4))) != (d2=SIGNOF(mymemcmp(&a, &b, 4))))
            printf("memcmp failed (1: %d %d)\n", d1, d2);
        if ((d1=SIGNOF(memcmp(&b, &a, 4))) != (d2=SIGNOF(mymemcmp(&b, &a, 4))))
            printf("memcmp failed (2: %d %d)\n", d1, d2);

        /*
        for (size_t i = 32-(((int) l1bufa)&31); i < 32-(((int) l1bufa)&31) + 32; i++)
        {
            for (size_t len = 0; len < 256; len++)
            {
                mymemcpy(l1bufb+0, l1bufa+i, len);
            }
            for (size_t len = 0; len < 256; len++)
            {
                mymemcpy(l1bufb+1, l1bufa+i, len);
            }
            for (size_t len = 0; len < 256; len++)
            {
                mymemcpy(l1bufb+2, l1bufa+i, len);
            }
            for (size_t len = 0; len < 256; len++)
            {
                mymemcpy(l1bufb+30, l1bufa+i, len);
            }
            for (size_t len = 0; len < 256; len++)
            {
                mymemcpy(l1bufb+31, l1bufa+i, len);
            }
        }
        */

        memset(l2bufa, 0, sizeof l1bufa);
        for (size_t i = 0; i < sizeof l1bufa; i += 4)
            *(uint32_t*)(l1bufa+i) = rand();
        for (size_t i = 0; i < 64; i++)
        {
            printf("%u\n", i);
            for (size_t j = 0; j < 64; j++)
                for (size_t len = 0; len < 2048; len++)
                {
                    int myresult;
                    int trueresult;
                    memset(l1bufb, 0, sizeof l1bufb);
                    mymemcpy(l1bufb+j, l1bufa+i, len);
                    if (memcmp(l1bufb+j, l1bufa+i, len) != 0)
                    {
                        printf("memcpy failed (data: %u %u %u)\n", i, j, len);
                        printf("should be");
                        for (size_t x = 0; x < len; x++)
                            printf(" %02X%s", l1bufa[i+x] & 0xFF, l1bufa[i+x] != l1bufb[j+x] ? "*" : "");
                        printf("\nbut is   ");
                        for (size_t x = 0; x < len; x++)
                            printf(" %02X%s", l1bufb[j+x] & 0xFF, l1bufa[i+x] != l1bufb[j+x] ? "*" : "");
                        printf("\n");
                    }
                    else if ((myresult = mymemcmp(l1bufb+j, l1bufa+i, len)) != 0)
                    {
                        printf("memcmp failed (%u %u %u) was %08x (%c0), should be =0\n", i, j, len, myresult, "<=>"[SIGNOF(myresult) + 1]);
                        myresult = mymemcmp(l1bufb+j, l1bufa+i, len);
                    }
                    for (size_t k = 0; k + 1 < len && k + 1 < 20; k++)
                    {
                        size_t k2 = len - 2 - k;
                        l1bufb[j+k] ^= 0x80;
                        l1bufb[j+k+1] ^= 0x80;

                        myresult = mymemcmp(l1bufb+j, l1bufa+i, len);
                        trueresult = memcmp(l1bufb+j, l1bufa+i, len);
                        if (SIGNOF(myresult) != SIGNOF(trueresult))
                        {
                            printf("memcmp failed (%u %u %u with diff at %u was %08x (%c0), should be %c0\n",
                                    i, j, len, k,
                                    myresult,
                                    "<=>"[SIGNOF(myresult) + 1],
                                    "<=>"[SIGNOF(trueresult) + 1]);
                            myresult = mymemcmp(l1bufb+j, l1bufa+i, len);
                        }
                        l1bufb[j+k] ^= 0x80;
                        l1bufb[j+k+1] ^= 0x80;
                        l1bufb[j+k2] ^= 0x80;
                        l1bufb[j+k2+1] ^= 0x80;
                        myresult = mymemcmp(l1bufb+j, l1bufa+i, len);
                        trueresult = memcmp(l1bufb+j, l1bufa+i, len);
                        if (SIGNOF(myresult) != SIGNOF(trueresult))
                        {
                            printf("memcmp failed (%u %u %u with diff at %u was %08x (%c0), should be %c0\n",
                                    i, j, len, k2,
                                    myresult,
                                    "<=>"[SIGNOF(myresult) + 1],
                                    "<=>"[SIGNOF(trueresult) + 1]);
                            myresult = mymemcmp(l1bufb+j, l1bufa+i, len);
                        }
                        l1bufb[j+k2] ^= 0x80;
                        l1bufb[j+k2+1] ^= 0x80;
                    }
                    if (memcmp(l1bufb, l2bufa, j) != 0)
                        printf("memcpy failed (before: %u %u %u)\n", i, j, len);
                    if (memcmp(l1bufb+j+len, l2bufa, sizeof l1bufa -j-len) != 0)
                        printf("memcpy failed (after: %u %u %u)\n", i, j, len);
                }
        }
    }
#endif

    // This code is for benchmarking
#if 1
    printf("L1,     L2,     M,      T,      R,      RT\n");

    while (iterations--)
    {
        memcpy(l1bufa, l1bufb, sizeof l1bufa);
        memcpy(l1bufb, l1bufa, sizeof l1bufa);

        t1 = gettime();
        bench_L(control, l1bufa, l1bufb, sizeof l1bufa - 64, TESTSIZE / (sizeof l1bufa - 64));
        t2 = gettime();
        byte_cnt = bench_L(CANDIDATE, l1bufa, l1bufb, sizeof l1bufa - 64, TESTSIZE / (sizeof l1bufa - 64));
        t3 = gettime();
        printf("%6.2f, ", ((double)byte_cnt) / ((t3 - t2) - (t2 - t1)));
        fflush(stdout);

        memcpy(l2bufa, l2bufb, sizeof l2bufa);
        memcpy(l2bufb, l2bufa, sizeof l2bufa);

        t1 = gettime();
        bench_L(control, l2bufa, l2bufb, sizeof l2bufa - 64, TESTSIZE / (sizeof l2bufa - 64));
        t2 = gettime();
        byte_cnt = bench_L(CANDIDATE, l2bufa, l2bufb, sizeof l2bufa - 64, TESTSIZE / (sizeof l2bufa - 64));
        t3 = gettime();
        printf("%6.2f, ", ((double)byte_cnt) / ((t3 - t2) - (t2 - t1)));
        fflush(stdout);

        memcpy(membufa, membufb, sizeof membufa);
        memcpy(membufb, membufa, sizeof membufa);

        t1 = gettime();
        bench_M(control, membufa, membufb, sizeof membufa - 64, TESTSIZE / (sizeof membufa - 64));
        t2 = gettime();
        byte_cnt = bench_M(CANDIDATE, membufa, membufb, sizeof membufa - 64, TESTSIZE / (sizeof membufa - 64));
        t3 = gettime();
        printf("%6.2f, ", ((double)byte_cnt) / ((t3 - t2) - (t2 - t1)));
        fflush(stdout);

        memcpy(membufa, membufb, sizeof membufa);
        memcpy(membufb, membufa, sizeof membufa);

        t1 = gettime();
        bench_T(control, membufa, membufb, TESTSIZE / (TILEWIDTH*2));
        t2 = gettime();
        byte_cnt = bench_T(CANDIDATE, membufa, membufb, TESTSIZE / (TILEWIDTH*2));
        t3 = gettime();
        printf("%6.2f, ", ((double)byte_cnt) / ((t3 - t2) - (t2 - t1)));
        fflush(stdout);

        memcpy(membufa, membufb, sizeof membufa);
        memcpy(membufb, membufa, sizeof membufa);

        t1 = gettime();
        bench_R(control, membufa, membufb, TESTSIZE / (TILEWIDTH*2));
        t2 = gettime();
        byte_cnt = bench_R(CANDIDATE, membufa, membufb, TESTSIZE / (TILEWIDTH*2));
        t3 = gettime();
        printf("%6.2f, ", ((double)byte_cnt) / ((t3 - t2) - (t2 - t1)));
        fflush(stdout);

        memcpy(membufa, membufb, sizeof membufa);
        memcpy(membufb, membufa, sizeof membufa);

        t1 = gettime();
        bench_RT(control, membufa, membufb, TESTSIZE / (TILEWIDTH*2));
        t2 = gettime();
        byte_cnt = bench_RT(CANDIDATE, membufa, membufb, TESTSIZE / (TILEWIDTH*2));
        t3 = gettime();
        printf("%6.2f\n", ((double)byte_cnt) / ((t3 - t2) - (t2 - t1)));
        fflush(stdout);
    }
#elif 0
    const char *sep = "";
    for (int w = 1; w <= 100; w++)
    {
        printf("%sW%d", sep, w);
        sep = ", ";
    }
    printf("\n");

    while (iterations--)
    {
        sep = "";
        for (int w = 1; w <= 100; w++)
        {
            memcpy(membufa, membufb, sizeof membufa);
            memcpy(membufb, membufa, sizeof membufa);

            t1 = gettime();
            bench_RW(control, membufa, membufb, w, TESTSIZE / w);
            t2 = gettime();
            byte_cnt = bench_RW(CANDIDATE, membufa, membufb, w, TESTSIZE / w);
            t3 = gettime();
            printf("%s%6.2f", sep, ((double)byte_cnt) / ((t3 - t2) - (t2 - t1)));
            sep = ", ";
            fflush(stdout);
        }
        printf("\n");
    }
#endif
}
