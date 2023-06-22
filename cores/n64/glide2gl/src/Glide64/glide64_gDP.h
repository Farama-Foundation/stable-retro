#include <stdint.h>

#include <retro_inline.h>

#include "../../../Graphics/GBI.h"
#include "Util.h"
#include "TexLoad.h"

#ifdef __cplusplus
extern "C" {
#endif

extern int CI_SET;
extern uint32_t swapped_addr;

void glide64gDPSetTextureImage(int32_t fmt, int32_t siz,
   int32_t width, int32_t addr);
void glide64gDPLoadBlock( uint32_t tile, uint32_t ul_s, uint32_t ul_t,
      uint32_t lr_s, uint32_t dxt );
void glide64gDPLoadTile(uint32_t tile, uint32_t ul_s, uint32_t ul_t,
      uint32_t lr_s, uint32_t lr_t);

#ifdef __cplusplus
}
#endif
