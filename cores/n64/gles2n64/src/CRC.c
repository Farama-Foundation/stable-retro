#include <stdint.h>

#define CRC32_POLYNOMIAL 0x04C11DB7
unsigned int CRCTable[ 256 ];

uint32_t Reflect(uint32_t ref, char ch )
{
   int i;
   uint32_t value = 0;
   // Swap bit 0 for bit 7
   // bit 1 for bit 6, etc.
   for (i = 1; i < (ch + 1); i++)
   {
      if(ref & 1)
         value |= 1 << (ch - i);
      ref >>= 1;
   }
   return value;
}

void CRC_BuildTable(void)
{
   int i, j;
   uint32_t crc;

   for (i = 0; i < 256; i++)
   {
      crc = Reflect( i, 8 ) << 24;
      for (j = 0; j < 8; j++)
         crc = (crc << 1) ^ (crc & (1 << 31) ? CRC32_POLYNOMIAL : 0);
      CRCTable[i] = Reflect( crc, 32 );
   }
}

uint32_t CRC_Calculate(void *buffer, uint32_t count)
{
   uint8_t *p;
   uint32_t crc = 0xffffffff;
   p = (uint8_t*) buffer;
   while (count--)
      crc = (crc >> 8) ^ CRCTable[(crc & 0xFF) ^ *p++];
   return ~crc;
}

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

uint32_t Hash_Calculate(uint32_t hash, const void *buffer, uint32_t count)
{
   unsigned int i;
   const uint32_t *data = (const uint32_t *)buffer;

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
