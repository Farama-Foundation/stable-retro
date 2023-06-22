#ifndef L3DEX2_H
#define L3DEX2_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#define L3DEX2_LINE3D               0x08

void L3DEX2_Line3D( uint32_t w0, uint32_t w1 );
void L3DEX2_Init();

#ifdef __cplusplus
}
#endif

#endif

