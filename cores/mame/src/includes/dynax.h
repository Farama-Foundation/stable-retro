/***************************************************************************

						-= Dynax / Nakanihon Games =-

***************************************************************************/

/***************************************************************************


								Interrupts


***************************************************************************/

/* Variables defined in drivers: */

extern UINT8 dynax_blitter_irq;

/* Functions defined in drivers: */

void sprtmtch_update_irq(void);

/***************************************************************************


								Video Blitter(s)


***************************************************************************/

/* Functions defined in vidhrdw: */

WRITE_HANDLER( dynax_blitter_rev2_w );

WRITE_HANDLER( dynax_blit_pen_w );
WRITE_HANDLER( dynax_blit_backpen_w );
WRITE_HANDLER( dynax_blit_palette01_w );
WRITE_HANDLER( dynax_blit_palette2_w );
WRITE_HANDLER( dynax_blit_palbank_w );
WRITE_HANDLER( dynax_blit_dest_w );
WRITE_HANDLER( hanamai_layer_half_w );
WRITE_HANDLER( hnoridur_layer_half2_w );
WRITE_HANDLER( hanamai_priority_w );
WRITE_HANDLER( mjdialq2_blit_dest_w );
WRITE_HANDLER( dynax_layer_enable_w );
WRITE_HANDLER( mjdialq2_layer_enable_w );
WRITE_HANDLER( dynax_flipscreen_w );

WRITE_HANDLER( dynax_extra_scrollx_w );
WRITE_HANDLER( dynax_extra_scrolly_w );

VIDEO_START( hanamai );
VIDEO_START( hnoridur );
VIDEO_START( mcnpshnt );
VIDEO_START( sprtmtch );
VIDEO_START( mjdialq2 );

VIDEO_UPDATE( hanamai );
VIDEO_UPDATE( hnoridur );
VIDEO_UPDATE( sprtmtch );
VIDEO_UPDATE( mjdialq2 );

PALETTE_INIT( sprtmtch );
