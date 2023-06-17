/*************************************************************************

	Atari Blasteroids hardware

*************************************************************************/

/*----------- defined in vidhrdw/blstroid.c -----------*/

VIDEO_START( blstroid );
VIDEO_UPDATE( blstroid );

void blstroid_scanline_update(int scanline);

extern data16_t *blstroid_priorityram;
