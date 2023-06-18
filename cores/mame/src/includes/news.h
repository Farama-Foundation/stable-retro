extern data8_t *news_fgram;
extern data8_t *news_bgram;

WRITE_HANDLER( news_fgram_w );
WRITE_HANDLER( news_bgram_w );
WRITE_HANDLER( news_bgpic_w );
VIDEO_START( news );
VIDEO_UPDATE( news );
