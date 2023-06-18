/* vidhrdw/bosco.c */
extern data8_t *bosco_videoram;
extern data8_t *bosco_radarattr;
extern unsigned char *bosco_sharedram;
READ_HANDLER( bosco_sharedram1_r );
WRITE_HANDLER( bosco_sharedram1_w );
READ_HANDLER( bosco_sharedram2_r );
WRITE_HANDLER( bosco_sharedram2_w );
READ_HANDLER( bosco_videoram_r );
WRITE_HANDLER( bosco_videoram_w );
WRITE_HANDLER( bosco_scrollx_w );
WRITE_HANDLER( bosco_scrolly_w );
WRITE_HANDLER( bosco_starcontrol_w );
WRITE_HANDLER( bosco_starclr_w );
WRITE_HANDLER( bosco_starblink_w );
VIDEO_START( bosco );
VIDEO_UPDATE( bosco );
PALETTE_INIT( bosco );
VIDEO_EOF( bosco );	/* update starfield */

/* vidhrdw/galaga.c */
extern data8_t *galaga_videoram;
extern data8_t *galaga_ram1,*galaga_ram2,*galaga_ram3;
READ_HANDLER( galaga_sharedram1_r );
WRITE_HANDLER( galaga_sharedram1_w );
READ_HANDLER( galaga_sharedram2_r );
WRITE_HANDLER( galaga_sharedram2_w );
READ_HANDLER( galaga_sharedram3_r );
WRITE_HANDLER( galaga_sharedram3_w );
PALETTE_INIT( galaga );
READ_HANDLER( galaga_videoram_r );
WRITE_HANDLER( galaga_videoram_w );
WRITE_HANDLER( galaga_starcontrol_w );
WRITE_HANDLER( gatsbee_bank_w );
VIDEO_START( galaga );
VIDEO_UPDATE( galaga );
VIDEO_EOF( galaga );	/* update starfield */

/* XEVIOUS */
extern data8_t *xevious_fg_videoram,*xevious_fg_colorram;
extern data8_t *xevious_bg_videoram,*xevious_bg_colorram;
extern data8_t *xevious_sr1,*xevious_sr2,*xevious_sr3;
extern unsigned char *xevious_sharedram;
READ_HANDLER( xevious_sharedram0_r );
WRITE_HANDLER( xevious_sharedram0_w );
READ_HANDLER( xevious_sharedram1_r );
WRITE_HANDLER( xevious_sharedram1_w );
READ_HANDLER( xevious_sharedram2_r );
WRITE_HANDLER( xevious_sharedram2_w );
READ_HANDLER( xevious_sharedram3_r );
WRITE_HANDLER( xevious_sharedram3_w );
READ_HANDLER( xevious_fg_videoram_r );
READ_HANDLER( xevious_fg_colorram_r );
READ_HANDLER( xevious_bg_videoram_r );
READ_HANDLER( xevious_bg_colorram_r );
WRITE_HANDLER( xevious_fg_videoram_w );
WRITE_HANDLER( xevious_fg_colorram_w );
WRITE_HANDLER( xevious_bg_videoram_w );
WRITE_HANDLER( xevious_bg_colorram_w );
WRITE_HANDLER( xevious_vh_latch_w );
WRITE_HANDLER( xevious_bs_w );
READ_HANDLER( xevious_bb_r );
VIDEO_START( xevious );
PALETTE_INIT( xevious );
VIDEO_UPDATE( xevious );


/* BATTLES */
void battles_customio_init(void);
READ_HANDLER( battles_customio0_r );
READ_HANDLER( battles_customio_data0_r );
READ_HANDLER( battles_customio3_r );
READ_HANDLER( battles_customio_data3_r );
READ_HANDLER( battles_input_port_r );

WRITE_HANDLER( battles_customio0_w );
WRITE_HANDLER( battles_customio_data0_w );
WRITE_HANDLER( battles_customio3_w );
WRITE_HANDLER( battles_customio_data3_w );
WRITE_HANDLER( battles_CPU4_coin_w );
WRITE_HANDLER( battles_noise_sound_w );

INTERRUPT_GEN( battles_interrupt_4 );

PALETTE_INIT( battles );



/* DIG DUG */
extern data8_t *digdug_videoram,*digdug_objram, *digdug_posram, *digdug_flpram;
extern unsigned char *digdug_sharedram;
READ_HANDLER( digdug_sharedram0_r );
WRITE_HANDLER( digdug_sharedram0_w );
READ_HANDLER( digdug_sharedram1_r );
WRITE_HANDLER( digdug_sharedram1_w );
READ_HANDLER( digdug_sharedram2_r );
WRITE_HANDLER( digdug_sharedram2_w );
READ_HANDLER( digdug_sharedram3_r );
WRITE_HANDLER( digdug_sharedram3_w );
READ_HANDLER( digdug_videoram_r );
WRITE_HANDLER( digdug_videoram_w );
WRITE_HANDLER( digdug_PORT_w );
VIDEO_START( digdug );
VIDEO_UPDATE( digdug );
PALETTE_INIT( digdug );
