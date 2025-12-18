#ifndef _GRAPHICS_HLE_MICROCODE_FAST3D_H
#define _GRAPHICS_HLE_MICROCODE_FAST3D_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void F3D_EndDL(uint32_t w0, uint32_t w1);
void F3D_MoveMem(uint32_t w0, uint32_t w1);
void F3D_Reserved0(uint32_t w0, uint32_t w1);
void F3D_Reserved1(uint32_t w0, uint32_t w1);
void F3D_Reserved2(uint32_t w0, uint32_t w1);
void F3D_Reserved3(uint32_t w0, uint32_t w1);

#ifdef __cplusplus
}
#endif

#endif
