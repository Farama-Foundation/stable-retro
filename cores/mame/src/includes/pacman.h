/*************************************************************************

	Namco PuckMan

**************************************************************************/

/*----------- defined in vidhrdw/pacman.c -----------*/

PALETTE_INIT( pacman );
VIDEO_START( pacman );
VIDEO_UPDATE( pacman );

WRITE_HANDLER( pacman_videoram_w );
WRITE_HANDLER( pacman_colorram_w );
WRITE_HANDLER( pacman_flipscreen_w );
WRITE_HANDLER( mspactwin_videoram_w );


VIDEO_START( pengo );

WRITE_HANDLER( pengo_palettebank_w );
WRITE_HANDLER( pengo_colortablebank_w );
WRITE_HANDLER( pengo_gfxbank_w );


WRITE_HANDLER( vanvan_bgcolor_w );


VIDEO_START( s2650games );
VIDEO_UPDATE( s2650games );

extern data8_t *sprite_bank, *tiles_bankram;

WRITE_HANDLER( s2650games_videoram_w );
WRITE_HANDLER( s2650games_colorram_w );
WRITE_HANDLER( s2650games_scroll_w );
WRITE_HANDLER( s2650games_tilesbank_w );


VIDEO_START( jrpacman );

WRITE_HANDLER( jrpacman_videoram_w );
WRITE_HANDLER( jrpacman_charbank_w );
WRITE_HANDLER( jrpacman_spritebank_w );
WRITE_HANDLER( jrpacman_scroll_w );
WRITE_HANDLER( jrpacman_bgpriority_w );


/*----------- defined in machine/pacplus.c -----------*/

void pacplus_decode(void);


/*----------- defined in machine/jumpshot.c -----------*/

void jumpshot_decode(void);


/*----------- defined in machine/theglobp.c -----------*/

MACHINE_INIT( theglobp );
READ_HANDLER( theglobp_decrypt_rom );


/*----------- defined in machine/mspacman.c -----------*/

MACHINE_INIT( mspacman );
WRITE_HANDLER( mspacman_activate_rom );

/*----------- defined in machine/acitya.c -------------*/

MACHINE_INIT( acitya );
READ_HANDLER( acitya_decrypt_rom );
