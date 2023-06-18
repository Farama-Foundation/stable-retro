/* appoooh.h */

extern unsigned char *spriteram,*spriteram_2;

/* vidhrdw */
extern unsigned char *appoooh_fg_videoram,*appoooh_fg_colorram;
extern unsigned char *appoooh_bg_videoram,*appoooh_bg_colorram;
WRITE_HANDLER( appoooh_fg_videoram_w );
WRITE_HANDLER( appoooh_fg_colorram_w );
WRITE_HANDLER( appoooh_bg_videoram_w );
WRITE_HANDLER( appoooh_bg_colorram_w );
PALETTE_INIT( appoooh );
WRITE_HANDLER( appoooh_scroll_w );
WRITE_HANDLER( appoooh_out_w );
VIDEO_START( appoooh );
VIDEO_UPDATE( appoooh );
