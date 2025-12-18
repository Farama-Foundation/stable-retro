#include <stdint.h>
#include <stddef.h>

#define CRC32_POLYNOMIAL     0x04C11DB7

unsigned int CRCTable[ 256 ];

static uint32_t Reflect(uint32_t ref, char ch )
{
   char i;
   uint32_t value = 0;
   /* Swap bit 0 for bit 7.
    * bit 1 for bit 6, etc.
    */
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
   uint32_t crc = 0xffffffff;
   uint8_t *p = (uint8_t*) buffer;
   while (count--)
      crc = (crc >> 8) ^ CRCTable[(crc & 0xFF) ^ *p++];
   return ~crc;
}
