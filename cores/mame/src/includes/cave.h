/***************************************************************************


***************************************************************************/

/* Variables defined in vidhrdw */

extern int cave_spritetype;
extern int cave_kludge;

extern data16_t *cave_videoregs;

extern data16_t *cave_vram_0, *cave_vctrl_0;
extern data16_t *cave_vram_1, *cave_vctrl_1;
extern data16_t *cave_vram_2, *cave_vctrl_2;
extern data16_t *cave_vram_3, *cave_vctrl_3;

/* Functions defined in vidhrdw */

WRITE16_HANDLER( cave_vram_0_w );
WRITE16_HANDLER( cave_vram_1_w );
WRITE16_HANDLER( cave_vram_2_w );
WRITE16_HANDLER( cave_vram_3_w );

WRITE16_HANDLER( cave_vram_0_8x8_w );
WRITE16_HANDLER( cave_vram_1_8x8_w );
WRITE16_HANDLER( cave_vram_2_8x8_w );
WRITE16_HANDLER( cave_vram_3_8x8_w );

PALETTE_INIT( ddonpach );
PALETTE_INIT( dfeveron );
PALETTE_INIT( mazinger );
PALETTE_INIT( sailormn );
PALETTE_INIT( pwrinst2 );

VIDEO_START( cave_1_layer );
VIDEO_START( cave_2_layers );
VIDEO_START( cave_3_layers );
VIDEO_START( cave_4_layers );

VIDEO_START( sailormn_3_layers );


VIDEO_UPDATE( cave );

void cave_get_sprite_info(void);

void sailormn_tilebank_w( int bank );
