/*************************************************************************

	BattleToads

	Common definitions

*************************************************************************/

/*----------- defined in vidhrdw/btoads.c -----------*/

extern data16_t *btoads_vram_fg0, *btoads_vram_fg1, *btoads_vram_fg_data;
extern data16_t *btoads_vram_bg0, *btoads_vram_bg1;
extern data16_t *btoads_sprite_scale;
extern data16_t *btoads_sprite_control;

VIDEO_START( btoads );
VIDEO_UPDATE( btoads );

WRITE16_HANDLER( btoads_misc_control_w );
WRITE16_HANDLER( btoads_display_control_w );

WRITE16_HANDLER( btoads_scroll0_w );
WRITE16_HANDLER( btoads_scroll1_w );

WRITE16_HANDLER( btoads_paletteram_w );
READ16_HANDLER( btoads_paletteram_r );

WRITE16_HANDLER( btoads_vram_bg0_w );
WRITE16_HANDLER( btoads_vram_bg1_w );
READ16_HANDLER( btoads_vram_bg0_r );
READ16_HANDLER( btoads_vram_bg1_r );

WRITE16_HANDLER( btoads_vram_fg_display_w );
WRITE16_HANDLER( btoads_vram_fg_draw_w );
READ16_HANDLER( btoads_vram_fg_display_r );
READ16_HANDLER( btoads_vram_fg_draw_r );

void btoads_to_shiftreg(UINT32 address, UINT16 *shiftreg);
void btoads_from_shiftreg(UINT32 address, UINT16 *shiftreg);
