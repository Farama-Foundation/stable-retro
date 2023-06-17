/*************************************************************************

	Driver for Midway MCR games

**************************************************************************/

/* constants */
#define MAIN_OSC_MCR_I		19968000


/*----------- defined in machine/mcr.c -----------*/

extern INT16 spyhunt_scrollx, spyhunt_scrolly;
extern double mcr68_timing_factor;

extern Z80_DaisyChain mcr_daisy_chain[];
extern UINT8 mcr_cocktail_flip;

extern struct GfxLayout mcr_bg_layout;
extern struct GfxLayout mcr_sprite_layout;

MACHINE_INIT( mcr );
MACHINE_INIT( mcr68 );
MACHINE_INIT( zwackery );

INTERRUPT_GEN( mcr_interrupt );
INTERRUPT_GEN( mcr68_interrupt );

WRITE_HANDLER( mcr_control_port_w );
WRITE_HANDLER( mcrmono_control_port_w );
WRITE_HANDLER( mcr_scroll_value_w );

WRITE16_HANDLER( mcr68_6840_upper_w );
WRITE16_HANDLER( mcr68_6840_lower_w );
READ16_HANDLER( mcr68_6840_upper_r );
READ16_HANDLER( mcr68_6840_lower_r );


/*----------- defined in vidhrdw/mcr12.c -----------*/

extern INT8 mcr12_sprite_xoffs;
extern INT8 mcr12_sprite_xoffs_flip;

VIDEO_START( mcr1 );
VIDEO_START( mcr2 );
VIDEO_START( twotigra );
VIDEO_START( journey );

WRITE_HANDLER( mcr1_videoram_w );
WRITE_HANDLER( mcr2_videoram_w );
WRITE_HANDLER( twotigra_videoram_w );

VIDEO_UPDATE( mcr1 );
VIDEO_UPDATE( mcr2 );
VIDEO_UPDATE( journey );


/*----------- defined in vidhrdw/mcr3.c -----------*/

extern UINT8 spyhunt_sprite_color_mask;
extern INT16 spyhunt_scrollx, spyhunt_scrolly;
extern INT16 spyhunt_scroll_offset;

extern UINT8 *spyhunt_alpharam;

WRITE_HANDLER( mcr3_paletteram_w );
WRITE_HANDLER( mcr3_videoram_w );
WRITE_HANDLER( spyhunt_videoram_w );
WRITE_HANDLER( spyhunt_alpharam_w );

void mcr3_update_sprites(struct mame_bitmap *bitmap, const struct rectangle *cliprect, int color_mask, int code_xor, int dx, int dy);

VIDEO_START( mcr3 );
VIDEO_START( mcrmono );
VIDEO_START( spyhunt );
VIDEO_START( dotron );

PALETTE_INIT( spyhunt );

VIDEO_UPDATE( mcr3 );
VIDEO_UPDATE( spyhunt );
VIDEO_UPDATE( dotron );


/*----------- defined in vidhrdw/mcr68.c -----------*/

extern UINT8 mcr68_sprite_clip;
extern INT8 mcr68_sprite_xoffset;

WRITE16_HANDLER( mcr68_paletteram_w );
WRITE16_HANDLER( mcr68_videoram_w );

VIDEO_START( mcr68 );
VIDEO_UPDATE( mcr68 );

WRITE16_HANDLER( zwackery_paletteram_w );
WRITE16_HANDLER( zwackery_videoram_w );
WRITE16_HANDLER( zwackery_spriteram_w );

PALETTE_INIT( zwackery );
VIDEO_START( zwackery );
VIDEO_UPDATE( zwackery );
