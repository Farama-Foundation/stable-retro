#ifndef __GPU_UNAI_GPU_PORT_H__
#define __GPU_UNAI_GPU_PORT_H__

#include <stddef.h>
#include <string.h>

#define INLINE static inline

#define GPU_init	GPUinit
#define GPU_shutdown	GPUshutdown
//#define GPU_freeze	GPUfreeze
#define GPU_writeDataMem GPUwriteDataMem
#define GPU_dmaChain	GPUdmaChain
#define GPU_writeData	GPUwriteData
#define GPU_readDataMem	GPUreadDataMem
#define GPU_readData	GPUreadData
#define GPU_readStatus	GPUreadStatus
#define GPU_writeStatus	GPUwriteStatus
#define GPU_updateLace	GPUupdateLace

extern "C" {

#define u32 unsigned int
#define s32 signed int

bool GPUinit(void);
void GPUshutdown(void);
void GPUwriteDataMem(u32* dmaAddress, s32 dmaCount);
long GPUdmaChain(u32* baseAddr, u32 dmaVAddr);
void GPUwriteData(u32 data);
void GPUreadDataMem(u32* dmaAddress, s32 dmaCount);
u32  GPUreadData(void);
u32  GPUreadStatus(void);
void GPUwriteStatus(u32 data);

#undef u32
#undef s32

}

#endif /* __GPU_UNAI_GPU_PORT_H__ */
