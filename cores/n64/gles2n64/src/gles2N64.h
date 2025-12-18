#ifndef GLN64_H
#define GLN64_H

#ifdef __cplusplus
extern "C" {
#endif

#include "m64p_config.h"
#include "stdio.h"

//#define DEBUG

#define PLUGIN_NAME     "gles2n64"
#define PLUGIN_VERSION  0x000005
#define PLUGIN_API_VERSION 0x020200

#define renderCallback gln64RenderCallback

extern void (*renderCallback)();

#ifdef __cplusplus
}
#endif

#endif

