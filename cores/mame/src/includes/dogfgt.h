extern data8_t *dogfgt_bgvideoram;

WRITE_HANDLER( dogfgt_plane_select_w );
READ_HANDLER( dogfgt_bitmapram_r );
WRITE_HANDLER( dogfgt_bitmapram_w );
WRITE_HANDLER( dogfgt_bgvideoram_w );
WRITE_HANDLER( dogfgt_scroll_w );
WRITE_HANDLER( dogfgt_1800_w );

PALETTE_INIT( dogfgt );
VIDEO_START( dogfgt );
VIDEO_UPDATE( dogfgt );
