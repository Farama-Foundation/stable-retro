#ifndef __SEGAIC_H
#define __SEGAIC_H

/* system24temp_ functions / variables are from shared rewrite files,
   once the rest of the rewrite is complete they can be removed, I
   just made a copy & renamed them for now to avoid any conflicts
*/

/*#include "vmixer.h"*/

WRITE16_HANDLER (system24temp_sys16_paletteram1_w);

/* 
   Tilemaps
   System24
*/ 
int sys24_tile_vh_start(UINT16 tile_mask);
void sys24_tile_update(void);
void sys24_tile_draw(struct mame_bitmap *bitmap, const struct rectangle *cliprect, int layer, int pri, int flags);

READ16_HANDLER(sys24_tile_r);
READ16_HANDLER(sys24_char_r);
WRITE16_HANDLER(sys24_tile_w);
WRITE16_HANDLER(sys24_char_w);

/*
   Sprites
   System24
*/
int sys24_sprite_vh_start(void);
void sys24_sprite_draw(struct mame_bitmap *bitmap, const struct rectangle *cliprect, const int *spri);

WRITE16_HANDLER (sys24_sprite_w);
READ16_HANDLER (sys24_sprite_r);

/* 
    Programmable mixers
    System24
*/
int sys24_mixer_vh_start(void);
int sys24_mixer_get_reg(int reg);

WRITE16_HANDLER (sys24_mixer_w);
READ16_HANDLER (sys24_mixer_r);

#endif
