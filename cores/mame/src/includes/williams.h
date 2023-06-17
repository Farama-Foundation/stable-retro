/*************************************************************************

	Driver for early Williams games

**************************************************************************/


/*----------- defined in machine/wmsyunit.c -----------*/

/* Generic old-Williams PIA interfaces */
extern struct pia6821_interface williams_pia_0_intf;
extern struct pia6821_interface williams_muxed_pia_0_intf;
extern struct pia6821_interface williams_dual_muxed_pia_0_intf;
extern struct pia6821_interface williams_49way_pia_0_intf;
extern struct pia6821_interface williams_49way_muxed_pia_0_intf;
extern struct pia6821_interface williams_pia_1_intf;
extern struct pia6821_interface williams_snd_pia_intf;

/* Game-specific old-Williams PIA interfaces */
extern struct pia6821_interface defender_pia_0_intf;
extern struct pia6821_interface stargate_pia_0_intf;
extern struct pia6821_interface lottofun_pia_0_intf;
extern struct pia6821_interface sinistar_snd_pia_intf;
extern struct pia6821_interface playball_pia_1_intf;
extern struct pia6821_interface spdball_pia_3_intf;

/* Generic later-Williams PIA interfaces */
extern struct pia6821_interface williams2_muxed_pia_0_intf;
extern struct pia6821_interface williams2_pia_1_intf;
extern struct pia6821_interface williams2_snd_pia_intf;

/* Game-specific later-Williams PIA interfaces */
extern struct pia6821_interface mysticm_pia_0_intf;
extern struct pia6821_interface tshoot_pia_0_intf;
extern struct pia6821_interface tshoot_snd_pia_intf;
extern struct pia6821_interface joust2_pia_1_intf;

/* banking variables */
extern UINT8 *williams_bank_base;
extern UINT8 *defender_bank_base;
extern const UINT32 *defender_bank_list;
extern UINT8 williams2_bank;

/* switches controlled by $c900 */
extern UINT16 sinistar_clip;
extern UINT8 williams_cocktail;

/* initialization */
MACHINE_INIT( defender );
MACHINE_INIT( williams );
MACHINE_INIT( williams2 );
MACHINE_INIT( joust2 );

/* banking */
WRITE_HANDLER( williams_vram_select_w );
WRITE_HANDLER( defender_bank_select_w );
WRITE_HANDLER( blaster_bank_select_w );
WRITE_HANDLER( blaster_vram_select_w );
WRITE_HANDLER( williams2_bank_select_w );

/* misc */
WRITE_HANDLER( williams2_7segment_w );

/* Mayday protection */
extern UINT8 *mayday_protection;
READ_HANDLER( mayday_protection_r );


/*----------- defined in vidhrdw/wmsyunit.c -----------*/

extern UINT8 *williams_videoram;
extern UINT8 *williams2_paletteram;

/* blitter variables */
extern UINT8 *williams_blitterram;
extern UINT8 williams_blitter_xor;
extern UINT8 williams_blitter_remap;
extern UINT8 williams_blitter_clip;

/* tilemap variables */
extern UINT8 williams2_tilemap_mask;
extern const UINT8 *williams2_row_to_palette;
extern UINT8 williams2_M7_flip;
extern INT8  williams2_videoshift;
extern UINT8 williams2_special_bg_color;

/* later-Williams video control variables */
extern UINT8 *williams2_blit_inhibit;
extern UINT8 *williams2_xscroll_low;
extern UINT8 *williams2_xscroll_high;

/* Blaster extra variables */
extern UINT8 *blaster_color_zero_flags;
extern UINT8 *blaster_color_zero_table;
extern UINT8 *blaster_video_bits;


WRITE_HANDLER( defender_videoram_w );
WRITE_HANDLER( williams_videoram_w );
WRITE_HANDLER( williams2_videoram_w );
WRITE_HANDLER( williams_blitter_w );
WRITE_HANDLER( blaster_remap_select_w );
WRITE_HANDLER( blaster_palette_0_w );
READ_HANDLER( williams_video_counter_r );

VIDEO_START( williams );
VIDEO_UPDATE( williams );
VIDEO_UPDATE( williams2 );

VIDEO_START( blaster );
VIDEO_START( williams2 );

WRITE_HANDLER( williams2_fg_select_w );
WRITE_HANDLER( williams2_bg_select_w );
