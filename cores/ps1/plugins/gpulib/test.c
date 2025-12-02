#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "gpu.h"

static inline unsigned int pcnt_get(void)
{
	unsigned int val;
#ifdef __ARM_ARCH_7A__
	asm volatile("mrc p15, 0, %0, c9, c13, 0" : "=r"(val));
#else
	val = 0;
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
#endif
}

const unsigned char cmd_lengths[256] =
{
	0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	3, 3, 3, 3, 6, 6, 6, 6, 4, 4, 4, 4, 8, 8, 8, 8, // 20
	5, 5, 5, 5, 8, 8, 8, 8, 7, 7, 7, 7, 11, 11, 11, 11,
	2, 2, 2, 2, 0, 0, 0, 0, 3, 3, 3, 3, 3, 3, 3, 3, // 40
	3, 3, 3, 3, 0, 0, 0, 0, 4, 4, 4, 4, 4, 4, 4, 4,
	2, 2, 2, 2, 3, 3, 3, 3, 1, 1, 1, 1, 2, 2, 2, 2, // 60
	1, 1, 1, 1, 2, 2, 2, 2, 1, 1, 1, 1, 2, 2, 2, 2,
	3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 80
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // a0
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // c0
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // e0
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

struct psx_gpu gpu __attribute__((aligned(64)));

typedef struct
{
	uint16_t vram[1024 * 512];
	uint32_t gpu_register[15];
	uint32_t status;
} gpu_dump_struct;

static gpu_dump_struct state;

int main(int argc, char *argv[])
{
  unsigned int start_cycles;
  uint32_t *list;
  int size, dummy;
  FILE *state_file;
  FILE *list_file;
  FILE *out_file;

  if (argc != 3 && argc != 4)
  {
    printf("usage:\n%s <state> <list> [vram_out]\n", argv[0]);
    return 1;
  }

  state_file = fopen(argv[1], "rb");
  fread(&state, 1, sizeof(gpu_dump_struct), state_file);
  fclose(state_file);
  
  list_file = fopen(argv[2], "rb");
  fseek(list_file, 0, SEEK_END);
  size = ftell(list_file);
  fseek(list_file, 0, SEEK_SET);

  list = (uint32_t *)malloc(size);
  fread(list, 1, size, list_file);
  fclose(list_file);
 
  pcnt_init();
  renderer_init();
  memcpy(gpu.vram, state.vram, 1024*512*2);
  if ((state.gpu_register[8] & 0x24) == 0x24)
    renderer_set_interlace(1, !(state.status >> 31));

  start_cycles = pcnt_get();

  do_cmd_list(list, size / 4, &dummy, &dummy);
  renderer_flush_queues();

  printf("%u\n", pcnt_get() - start_cycles);

  if (argc >= 4) {
    out_file = fopen(argv[3], "wb");
    fwrite(gpu.vram, 1, sizeof(gpu.vram), out_file);
    fclose(out_file);
  }

  return 0;
}
