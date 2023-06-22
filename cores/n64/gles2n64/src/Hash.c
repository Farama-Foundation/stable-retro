#include <stdint.h>

uint32_t Hash_CalculatePalette(void *buffer, uint32_t count)
{
   unsigned int i;
   uint16_t *data = (uint16_t *) buffer;
   uint32_t hash = 0xffffffff;
   count /= 4;
   for(i = 0; i < count; ++i) {
      hash += data[i << 2];
      hash += (hash << 10);
      hash ^= (hash >> 6);
   }
   hash += (hash << 3);
   hash ^= (hash >> 11);
   hash += (hash << 15);
   return hash;
}

uint32_t Hash_Calculate(uint32_t hash, void *buffer, uint32_t count)
{
   unsigned int i;
   uint32_t *data = (uint32_t *) buffer;
   count /= 4;
   for(i = 0; i < count; ++i) {
      hash += data[i];
      hash += (hash << 10);
      hash ^= (hash >> 6);
   }
   hash += (hash << 3);
   hash ^= (hash >> 11);
   hash += (hash << 15);
   return hash;
}
