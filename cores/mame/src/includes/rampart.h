/*************************************************************************

	Atari Rampart hardware

*************************************************************************/


/*----------- defined in vidhrdw/rampart.c -----------*/

WRITE16_HANDLER( rampart_bitmap_w );

VIDEO_START( rampart );
VIDEO_UPDATE( rampart );

int rampart_bitmap_init(int _xdim, int _ydim);
void rampart_bitmap_render(struct mame_bitmap *bitmap, const struct rectangle *cliprect);

extern data16_t *rampart_bitmap;
