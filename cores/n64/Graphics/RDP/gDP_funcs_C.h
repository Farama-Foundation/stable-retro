#ifndef _GDP_FUNCS_C_H
#define _GDP_FUNCS_C_H

#include <stdint.h>

#include "gDP_funcs_prot.h"

#define gDPSetScissor(mode, ulx, uly, lrx, lry) GDPSetScissorC(mode, ulx, uly, lrx, lry)
#define gDPLoadBlock(tile, ul_s, ul_t, lr_s, lr_t) GDPLoadBlockC(tile, ul_s, ul_t, lr_s, lr_t)

void GDPSetScissorC(uint32_t mode, float ulx, float uly, float lrx, float lry );
void GDPLoadBlockC(uint32_t tile, uint32_t ul_s, uint32_t ul_t,
      uint32_t lr_s, uint32_t dxt );

#endif
