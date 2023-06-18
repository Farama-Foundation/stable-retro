/* Variables defined in vidhrdw: */

extern data8_t *thedeep_vram_0, *thedeep_vram_1;
extern data8_t *thedeep_scroll, *thedeep_scroll2;

/* Functions defined in vidhrdw: */

WRITE_HANDLER( thedeep_vram_0_w );
WRITE_HANDLER( thedeep_vram_1_w );

PALETTE_INIT( thedeep );
VIDEO_START( thedeep );
VIDEO_UPDATE( thedeep );
