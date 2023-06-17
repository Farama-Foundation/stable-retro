/***************************************************************************

 Lasso and similar hardware

***************************************************************************/

/* defined in vidhrdw/ */
extern data8_t *lasso_videoram;
extern data8_t *lasso_colorram;
extern data8_t *lasso_spriteram;
extern size_t lasso_spriteram_size;
extern data8_t *lasso_bitmap_ram;
extern data8_t *wwjgtin_track_scroll;

WRITE_HANDLER( lasso_videoram_w );
WRITE_HANDLER( lasso_colorram_w );
WRITE_HANDLER( lasso_backcolor_w );
WRITE_HANDLER( lasso_video_control_w );
WRITE_HANDLER( wwjgtin_video_control_w );
WRITE_HANDLER( pinbo_video_control_w );
WRITE_HANDLER( wwjgtin_lastcolor_w );

PALETTE_INIT( lasso );
PALETTE_INIT( wwjgtin );

VIDEO_START( lasso );
VIDEO_START( wwjgtin );
VIDEO_START( pinbo );

VIDEO_UPDATE( lasso );
VIDEO_UPDATE( chameleo );
VIDEO_UPDATE( wwjgtin );
