/*************************************************************************

	Atari Batman hardware

*************************************************************************/

/*----------- defined in vidhrdw/batman.c -----------*/

extern UINT8 batman_alpha_tile_bank;
extern INT32 mm2_startup;

VIDEO_START( batman );
VIDEO_UPDATE( batman );
VIDEO_START( mm2 );
VIDEO_UPDATE( mm2 );

void batman_scanline_update(int scanline);
