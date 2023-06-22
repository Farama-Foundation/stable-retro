#ifndef _GDP_FUNCS_H
#define _GDP_FUNCS_H

#include <stdint.h>

#include "gDP_funcs_prot.h"

#define gDPSetScissor(mode, ulx, uly, lrx, lry) GDPSetScissor(mode, ulx, uly, lrx, lry)
#define gDPLoadBlock(tile, ul_s, ul_t, lr_s, lr_t) GDPLoadBlock(tile, ul_s, ul_t, lr_s, lr_t)

void GDPSetScissor(uint32_t mode, float ulx, float uly, float lrx, float lry );
void GDPLoadBlock(uint32_t tile, uint32_t ul_s, uint32_t ul_t,
      uint32_t lr_s, uint32_t dxt );

#endif
