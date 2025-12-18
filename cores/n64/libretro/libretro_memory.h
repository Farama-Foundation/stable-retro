#ifndef M64P_LIBRETRO_MEMORY_H
#define M64P_LIBRETRO_MEMORY_H

#include <stdint.h>

typedef struct _save_memory_data
{
   uint8_t eeprom[0x800];
   uint8_t mempack[4][0x8000];
   uint8_t sram[0x8000];
   uint8_t flashram[0x20000];
   uint8_t disk[0x0435B0C0];
} save_memory_data;

extern save_memory_data saved_memory;

#endif
