#ifndef M64P_RI_SAFE_RDRAM_H
#define M64P_RI_SAFE_RDRAM_H

#include <stdint.h>

inline uint8_t rdram_safe_read_byte(const void *rdram, uint32_t addr)
{
    addr &= 0x3ffffffu;
    return (addr < 0x800000u) ? ((const uint8_t*)rdram)[addr] : 0;
}

inline void rdram_safe_write_byte(void *rdram, uint32_t addr, uint8_t value)
{
    addr &= 0x3ffffffu;
    if (addr < 0x800000u)
        ((uint8_t*)rdram)[addr] = value;
}

inline uint32_t rdram_safe_read_word(const void *rdram, uint32_t addr)
{
    addr = (addr & 0x3ffffffu) >> 2;
    return (addr < 0x200000u) ? ((const uint32_t*)rdram)[addr] : 0u;
}

inline void rdram_safe_write_word(void *rdram, uint32_t addr, uint32_t value)
{
    addr = (addr & 0x3ffffffu) >> 2;
    if (addr < 0x200000u)
        ((uint32_t*)rdram)[addr] = value;
}

inline void rdram_safe_masked_write_word(void *rdram, uint32_t addr, uint32_t value, uint32_t mask)
{
    addr = (addr & 0x3ffffffu) >> 2;
    if (addr >= 0x200000u) return;
    uint32_t *word = &((uint32_t*)rdram)[addr];
    *word = (*word & ~mask) | (value & mask);
}

#endif
