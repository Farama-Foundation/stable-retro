extern data8_t *taxidrvr_vram0,*taxidrvr_vram1,*taxidrvr_vram2,*taxidrvr_vram3;
extern data8_t *taxidrvr_vram4,*taxidrvr_vram5,*taxidrvr_vram6,*taxidrvr_vram7;
extern data8_t *taxidrvr_scroll;
extern int taxidrvr_bghide;

WRITE_HANDLER( taxidrvr_spritectrl_w );

VIDEO_UPDATE( taxidrvr );
