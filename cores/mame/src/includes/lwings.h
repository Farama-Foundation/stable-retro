extern unsigned char *lwings_fgvideoram;
extern unsigned char *lwings_bg1videoram;

WRITE_HANDLER( lwings_fgvideoram_w );
WRITE_HANDLER( lwings_bg1videoram_w );
WRITE_HANDLER( lwings_bg1_scrollx_w );
WRITE_HANDLER( lwings_bg1_scrolly_w );
WRITE_HANDLER( trojan_bg2_scrollx_w );
WRITE_HANDLER( trojan_bg2_image_w );
VIDEO_START( lwings );
VIDEO_START( trojan );
VIDEO_START( avengers );
VIDEO_UPDATE( lwings );
VIDEO_UPDATE( trojan );
VIDEO_EOF( lwings );
