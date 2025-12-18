#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void CRC_BuildTable(void);

uint32_t CRC_Calculate(void *buffer, uint32_t count);
uint32_t Hash_CalculatePalette(void *buffer, uint32_t count);
uint32_t Hash_Calculate(uint32_t hash, const void *buffer, uint32_t count);

#ifdef __cplusplus
}
#endif
