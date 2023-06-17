/***************************************************************************
    Battle Cross
***************************************************************************/


/*----------------- video-related ----------------- */
static uint8_t battlex_scroll_lsb;
static uint8_t battlex_scroll_msb;
static uint8_t battlex_starfield_enabled;
static uint8_t battlex_in0_b4;


/*---------- defined in battlex_vidhrdw.c ----------*/

WRITE_HANDLER( battlex_palette_w );
WRITE_HANDLER( battlex_videoram_w );
WRITE_HANDLER( battlex_scroll_x_lsb_w );
WRITE_HANDLER( battlex_scroll_x_msb_w );
WRITE_HANDLER( battlex_scroll_starfield_w );
WRITE_HANDLER( battlex_flipscreen_w );
VIDEO_START( battlex );
VIDEO_START( dodgeman );
VIDEO_UPDATE( battlex );


/*-------------- defined in battlex.c --------------*/
INTERRUPT_GEN( battlex_interrupt );
READ_HANDLER( battlex_in0_b4_r );
