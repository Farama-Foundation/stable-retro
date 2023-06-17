/***************************************************************************

							-= Kaneko 16 Bit Games =-

***************************************************************************/

/* Tile Layers: */

extern data16_t *kaneko16_vram_0,    *kaneko16_vram_1,    *kaneko16_layers_0_regs;
extern data16_t *kaneko16_vscroll_0, *kaneko16_vscroll_1;
extern data16_t *kaneko16_vram_2,    *kaneko16_vram_3,    *kaneko16_layers_1_regs;
extern data16_t *kaneko16_vscroll_2, *kaneko16_vscroll_3;

WRITE16_HANDLER( kaneko16_vram_0_w );
WRITE16_HANDLER( kaneko16_vram_1_w );
WRITE16_HANDLER( kaneko16_vram_2_w );
WRITE16_HANDLER( kaneko16_vram_3_w );

WRITE16_HANDLER( kaneko16_layers_0_regs_w );
WRITE16_HANDLER( kaneko16_layers_1_regs_w );


/* Sprites: */

extern int kaneko16_sprite_type;
extern data16_t kaneko16_sprite_xoffs, kaneko16_sprite_flipx;
extern data16_t kaneko16_sprite_yoffs, kaneko16_sprite_flipy;
extern data16_t *kaneko16_sprites_regs;

READ16_HANDLER ( kaneko16_sprites_regs_r );
WRITE16_HANDLER( kaneko16_sprites_regs_w );

void kaneko16_draw_sprites(struct mame_bitmap *bitmap, const struct rectangle *cliprect, int pri);

/* Pixel Layer: */

extern data16_t *kaneko16_bg15_select, *kaneko16_bg15_reg;

READ16_HANDLER ( kaneko16_bg15_select_r );
WRITE16_HANDLER( kaneko16_bg15_select_w );

READ16_HANDLER ( kaneko16_bg15_reg_r );
WRITE16_HANDLER( kaneko16_bg15_reg_w );

PALETTE_INIT( berlwall );


/* Priorities: */

typedef struct
{
	int VIEW2_2_pri;

	int tile[4];
	int sprite[4];
}	kaneko16_priority_t;

extern kaneko16_priority_t kaneko16_priority;


/* Machine */

VIDEO_START( kaneko16_sprites );
VIDEO_START( kaneko16_1xVIEW2 );
VIDEO_START( kaneko16_2xVIEW2 );
VIDEO_START( berlwall );
VIDEO_START( sandscrp_1xVIEW2 );
VIDEO_START( wingforce_1xVIEW2 );


VIDEO_UPDATE( kaneko16 );

MACHINE_INIT( kaneko16 );


/* in drivers/galpani2.c */

void galpani2_mcu_run(void);

/* in vidhrdw/galpani2.c */

extern data16_t *galpani2_bg8_0,         *galpani2_bg8_1;
extern data16_t *galpani2_palette_0,     *galpani2_palette_1;
extern data16_t *galpani2_bg8_regs_0,    *galpani2_bg8_regs_1;
extern data16_t *galpani2_bg8_0_scrollx, *galpani2_bg8_1_scrollx;
extern data16_t *galpani2_bg8_0_scrolly, *galpani2_bg8_1_scrolly;

extern data16_t *galpani2_bg15;

PALETTE_INIT( galpani2 );
VIDEO_START( galpani2 );
VIDEO_UPDATE( galpani2 );

WRITE16_HANDLER( galpani2_palette_0_w );
WRITE16_HANDLER( galpani2_palette_1_w );

READ16_HANDLER ( galpani2_bg8_regs_0_r );
READ16_HANDLER ( galpani2_bg8_regs_1_r );
WRITE16_HANDLER( galpani2_bg8_regs_0_w );
WRITE16_HANDLER( galpani2_bg8_regs_1_w );
WRITE16_HANDLER( galpani2_bg8_0_w );
WRITE16_HANDLER( galpani2_bg8_1_w );

WRITE16_HANDLER( galpani2_bg15_w );
