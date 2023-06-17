/***************************************************************************

  Fast Freddie/Jump Coaster hardware
  driver by Zsolt Vasvari

***************************************************************************/

/* defined in vihdrdw/fastfred.h */
extern data8_t *fastfred_videoram;
extern data8_t *fastfred_spriteram;
extern size_t fastfred_spriteram_size;
extern data8_t *fastfred_attributesram;

PALETTE_INIT( fastfred );
VIDEO_START( fastfred );
WRITE_HANDLER( fastfred_videoram_w );
WRITE_HANDLER( fastfred_attributes_w );
WRITE_HANDLER( fastfred_charbank1_w );
WRITE_HANDLER( fastfred_charbank2_w );
WRITE_HANDLER( fastfred_colorbank1_w );
WRITE_HANDLER( fastfred_colorbank2_w );
WRITE_HANDLER( fastfred_background_color_w );
WRITE_HANDLER( fastfred_flip_screen_x_w );
WRITE_HANDLER( fastfred_flip_screen_y_w );
VIDEO_UPDATE( fastfred );

extern data8_t *imago_fg_videoram;
VIDEO_START( imago );
VIDEO_UPDATE( imago );
WRITE_HANDLER( imago_fg_videoram_w );
WRITE_HANDLER( imago_charbank_w );
