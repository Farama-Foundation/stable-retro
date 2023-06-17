/*************************************************************************

	Model Racing Dribbling hardware

*************************************************************************/

/*----------- defined in drivers/dribling.c -----------*/

extern data8_t dribling_abca;


/*----------- defined in vidhrdw/dribling.c -----------*/

PALETTE_INIT( dribling );
WRITE_HANDLER( dribling_colorram_w );
VIDEO_UPDATE( dribling );
