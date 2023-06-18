/* drivers/tatsumi.c */
extern data8_t* tatsumi_rom_sprite_lookup1;
extern data8_t* tatsumi_rom_sprite_lookup2;
extern data8_t* tatsumi_rom_clut0;
extern data8_t* tatsumi_rom_clut1;

extern data16_t *roundup5_d0000_ram, *roundup5_e0000_ram;
extern data16_t *cyclwarr_videoram0, *cyclwarr_videoram1;
extern data8_t *roundup5_unknown0, *roundup5_unknown1, *roundup5_unknown2;

/* machine/tatsumi.c */
READ_HANDLER( apache3_bank_r );
WRITE_HANDLER( apache3_bank_w );
WRITE16_HANDLER( apache3_irq_ack_w );
READ_HANDLER( apache3_v30_v20_r );
WRITE_HANDLER( apache3_v30_v20_w );
READ_HANDLER( roundup_v30_z80_r );
WRITE_HANDLER( roundup_v30_z80_w );
WRITE_HANDLER(apache3_palette_w);
READ_HANDLER( tatsumi_v30_68000_r );
WRITE_HANDLER( tatsumi_v30_68000_w ) ;
READ16_HANDLER(apache3_z80_r);
WRITE16_HANDLER(apache3_z80_w);
READ_HANDLER( apache3_adc_r );
WRITE_HANDLER( apache3_adc_w );
WRITE16_HANDLER(cyclwarr_control_w);
READ16_HANDLER(cyclwarr_control_r);
WRITE_HANDLER( roundup5_control_w );
WRITE16_HANDLER( apache3_a0000_w );
WRITE16_HANDLER( roundup5_d0000_w );
WRITE16_HANDLER( roundup5_e0000_w );

READ_HANDLER(tatsumi_hack_ym2151_r);
READ_HANDLER(tatsumi_hack_oki_r);

extern data16_t *tatsumi_68k_ram;
extern data8_t *apache3_z80_ram;
extern data16_t tatsumi_control_word;
extern data16_t apache3_a0000[16];

void tatsumi_reset(void);

/* vidhrdw/tatsumi.c */
WRITE_HANDLER(roundup5_palette_w);
WRITE16_HANDLER(tatsumi_sprite_control_w);
WRITE_HANDLER( roundup5_text_w );
WRITE_HANDLER( roundup5_crt_w );
READ16_HANDLER( cyclwarr_videoram0_r );
WRITE16_HANDLER( cyclwarr_videoram0_w );
READ16_HANDLER( cyclwarr_videoram1_r );
WRITE16_HANDLER( cyclwarr_videoram1_w );
READ_HANDLER(roundup5_vram_r);
WRITE_HANDLER(roundup5_vram_w);

extern data16_t* tatsumi_sprite_control_ram;
extern data16_t *cyclwarr_videoram0, *cyclwarr_videoram1;
extern data16_t *roundup_r_ram, *roundup_p_ram, *roundup_l_ram;

VIDEO_START( apache3 );
VIDEO_START( bigfight );
VIDEO_START( roundup5 );
VIDEO_START( cyclwarr );
VIDEO_UPDATE( roundup5 );
VIDEO_UPDATE( apache3 );
VIDEO_UPDATE( bigfight );
VIDEO_UPDATE( cyclwarr );


