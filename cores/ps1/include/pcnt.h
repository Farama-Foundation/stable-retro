#ifndef __PCNT_H__
#define __PCNT_H__

enum pcounters {
	PCNT_ALL,
	PCNT_GPU,
	PCNT_SPU,
	PCNT_BLIT,
	PCNT_GTE,
	PCNT_TEST,
	PCNT_CNT
};

#ifdef PCNT

#if defined(__ARM_ARCH_7A__) || defined(ARM1176)
#define PCNT_DIV 1000
#else
#include <sys/time.h>
#define PCNT_DIV 1
#endif

static const char *pcnt_names[PCNT_CNT] = { "", "gpu", "spu", "blit", "gte", "test" };

#define PCNT_FRAMES 10

extern unsigned int pcounters[PCNT_CNT];
extern unsigned int pcounter_starts[PCNT_CNT];

#define pcnt_start(id) \
	pcounter_starts[id] = pcnt_get()

#define pcnt_end(id) \
	pcounters[id] += pcnt_get() - pcounter_starts[id]

void pcnt_hook_plugins(void);

static inline void pcnt_print(float fps)
{
	static int print_counter;
	unsigned int total, rem;
	int i;

	for (i = 0; i < PCNT_CNT; i++)
		pcounters[i] /= PCNT_DIV * PCNT_FRAMES;

	rem = total = pcounters[PCNT_ALL];
	for (i = 1; i < PCNT_CNT; i++)
		rem -= pcounters[i];
	if (!total)
		total++;

	if (--print_counter < 0) {
		printf("     ");
		for (i = 1; i < PCNT_CNT; i++)
			printf("%5s ", pcnt_names[i]);
		printf("%5s\n", "rem");
		print_counter = 30;
	}

	printf("%4.1f ", fps);
#if 0
	static float pcounters_all[PCNT_CNT+1];
	static int pcounter_samples;
	pcounter_samples++;

	for (i = 1; i < PCNT_CNT; i++) {
		pcounters_all[i] += pcounters[i];
		printf("%5.0f ", pcounters_all[i] / pcounter_samples);
	}
	pcounters_all[i] += rem;
	printf("%5.0f\n", pcounters_all[i] / pcounter_samples);
#else
	for (i = 1; i < PCNT_CNT; i++)
		printf("%5u ", pcounters[i]);
	printf("%5u (", rem);
	for (i = 1; i < PCNT_CNT; i++)
		printf("%2u ", pcounters[i] * 100 / total);
	printf("%2u) %u\n", rem * 100 / total, total);
#endif
	memset(pcounters, 0, sizeof(pcounters));
}

static inline unsigned int pcnt_get(void)
{
	unsigned int val;
#ifdef __ARM_ARCH_7A__
	__asm__ volatile("mrc p15, 0, %0, c9, c13, 0"
			 : "=r"(val));
#elif defined(ARM1176)
	__asm__ volatile("mrc p15, 0, %0, c15, c12, 1"
			 : "=r"(val));
#else
	// all slow on ARM :(
	//struct timespec tv;
	//clock_gettime(CLOCK_MONOTONIC_RAW, &tv);
	//val = tv.tv_sec * 1000000000 + tv.tv_nsec;
	struct timeval tv;
	gettimeofday(&tv, NULL);
	val = tv.tv_sec * 1000000 + tv.tv_usec;
#endif
	return val;
}

static inline void pcnt_init(void)
{
#ifdef __ARM_ARCH_7A__
	int v;
	asm volatile("mrc p15, 0, %0, c9, c12, 0" : "=r"(v));
	v |= 5; // master enable, ccnt reset
	v &= ~8; // ccnt divider 0
	asm volatile("mcr p15, 0, %0, c9, c12, 0" :: "r"(v));
	// enable cycle counter
	asm volatile("mcr p15, 0, %0, c9, c12, 1" :: "r"(1<<31));
#elif defined(ARM1176)
	int v;
	asm volatile("mrc p15, 0, %0, c15, c12, 0" : "=r"(v));
	v |= 5; // master enable, ccnt reset
	v &= ~8; // ccnt divider 0
	asm volatile("mcr p15, 0, %0, c15, c12, 0" :: "r"(v));
#endif
}

void pcnt_gte_start(int op);
void pcnt_gte_end(int op);

#else

#define pcnt_start(id)
#define pcnt_end(id)
#define pcnt_hook_plugins()
#define pcnt_print(fps)

#endif

#endif /* __PCNT_H__ */
