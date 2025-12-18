#ifndef L3D_H
#define L3D_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#define L3D_LINE3D              0xB5

void L3D_Line3D( uint32_t w0, uint32_t w1 );
void L3D_Init(void);

#ifdef __cplusplus
}
#endif

#endif

