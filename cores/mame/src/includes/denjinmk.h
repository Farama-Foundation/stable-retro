/*----------- defined in video/denjinmk.c -----------*/

extern UINT16 *denjinmk_back_data,*denjinmk_fore_data,*denjinmk_mid_data,*denjinmk_scrollram16,*denjinmk_textram;
extern UINT8 grainbow_pri_n;
extern UINT16 denjinmk_layer_disable;


void denjinmk_setgfxbank(UINT16 data);
WRITE16_HANDLER( denjinmk_background_w );
WRITE16_HANDLER( denjinmk_foreground_w );
WRITE16_HANDLER( denjinmk_midground_w );
WRITE16_HANDLER( denjinmk_text_w );


VIDEO_START( denjinmk );
VIDEO_UPDATE( denjinmk );
