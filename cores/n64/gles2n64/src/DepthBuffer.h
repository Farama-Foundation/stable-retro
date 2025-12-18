#ifndef DEPTHBUFFER_H
#define DEPTHBUFFER_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define depthBuffer gln64depthBuffer

typedef struct DepthBuffer
{
    struct DepthBuffer *higher, *lower;

    uint32_t address, cleared;
} DepthBuffer;

typedef struct
{
    DepthBuffer *top, *bottom, *current;
    int numBuffers;
} DepthBufferInfo;

extern DepthBufferInfo depthBuffer;

void DepthBuffer_Init(void);
void DepthBuffer_Destroy(void);
void DepthBuffer_SetBuffer( uint32_t address );
void DepthBuffer_RemoveBuffer( uint32_t address );
DepthBuffer *DepthBuffer_FindBuffer( uint32_t address );

#ifdef __cplusplus
}
#endif

#endif

