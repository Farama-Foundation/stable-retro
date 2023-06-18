VIDEO_START( gcpinbal );
VIDEO_EOF( gcpinbal );
VIDEO_UPDATE( gcpinbal );

WRITE16_HANDLER( gcpinbal_spritectrl_w );
WRITE16_HANDLER( gcpinbal_spriteflip_w );
READ16_HANDLER ( gcpinbal_tilemaps_word_r );
WRITE16_HANDLER( gcpinbal_tilemaps_word_w );

extern data16_t *gcpinbal_tilemapram;
extern data16_t *gcpinbal_ioc_ram;
