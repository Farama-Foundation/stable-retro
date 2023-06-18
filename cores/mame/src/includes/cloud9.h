/*************************************************************************

	Atari Cloud 9 (prototype) hardware

*************************************************************************/

/*----------- defined in vidhrdw/cloud9.c -----------*/

WRITE_HANDLER( cloud9_paletteram_w );
READ_HANDLER( cloud9_bitmap_regs_r );
WRITE_HANDLER( cloud9_bitmap_regs_w );
WRITE_HANDLER( cloud9_bitmap_w );

VIDEO_UPDATE( cloud9 );
VIDEO_START( cloud9 );


extern unsigned char *cloud9_vram2;
extern unsigned char *cloud9_bitmap_regs;
extern unsigned char *cloud9_auto_inc_x;
extern unsigned char *cloud9_auto_inc_y;
extern unsigned char *cloud9_both_banks;
extern unsigned char *cloud9_vram_bank;
extern unsigned char *cloud9_color_bank;
