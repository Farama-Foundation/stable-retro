/*************************************************************************

	Atari Cloak & Dagger hardware

*************************************************************************/

/*----------- defined in vidhrdw/ccastles.c -----------*/

WRITE_HANDLER( cloak_paletteram_w );
READ_HANDLER( graph_processor_r );
WRITE_HANDLER( graph_processor_w );
WRITE_HANDLER( cloak_clearbmp_w );

VIDEO_START( cloak );
VIDEO_UPDATE( cloak );
