/* Variables defined in vidhrdw: */

extern data8_t *paradise_vram_0,*paradise_vram_1,*paradise_vram_2;
extern int paradise_sprite_inc;

/* Functions defined in vidhrdw: */

WRITE_HANDLER( paradise_vram_0_w );
WRITE_HANDLER( paradise_vram_1_w );
WRITE_HANDLER( paradise_vram_2_w );

WRITE_HANDLER( paradise_flipscreen_w );
WRITE_HANDLER( tgtball_flipscreen_w );
WRITE_HANDLER( paradise_palette_w );
WRITE_HANDLER( paradise_pixmap_w );

WRITE_HANDLER( paradise_priority_w );
WRITE_HANDLER( paradise_palbank_w );

VIDEO_START( paradise );
VIDEO_UPDATE( paradise );
VIDEO_UPDATE( torus );
VIDEO_UPDATE( madball );
