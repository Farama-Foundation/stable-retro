#ifndef REALBRK_H
#define REALBRK_H

VIDEO_START(realbrk);
VIDEO_UPDATE(realbrk);

WRITE16_HANDLER( realbrk_vram_0_w );
WRITE16_HANDLER( realbrk_vram_1_w );
WRITE16_HANDLER( realbrk_vram_2_w );
WRITE16_HANDLER( realbrk_vregs_w );
WRITE16_HANDLER( realbrk_flipscreen_w );

extern data16_t *realbrk_vram_0, *realbrk_vram_1, *realbrk_vram_2, *realbrk_vregs;

#endif
