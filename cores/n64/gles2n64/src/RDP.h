#ifndef _GLN64_RDP_H
#define _GLN64_RDP_H

#include <stdint.h>

#include "../../Graphics/RDP/RDP_state.h"

#ifndef maxCMDMask
#define maxCMDMask (MAXCMD - 1)
#endif

#ifdef __cplusplus
extern "C" {
#endif

void RDP_Init(void);
void RDP_Half_1(uint32_t _c);
void RDP_ProcessRDPList();
void RDP_RepeatLastLoadBlock(void);
void RDP_SetScissor(uint32_t w0, uint32_t w1);

#ifdef __cplusplus
}
#endif

#endif

