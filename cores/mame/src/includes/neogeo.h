/*************************************************************************

	SNK NeoGeo hardware

*************************************************************************/

/*----------- defined in drivers/neogeo.c -----------*/

extern unsigned int neogeo_frame_counter;
extern unsigned int neogeo_frame_counter_speed;
extern int neogeo_has_trackball;

void neogeo_set_cpu1_second_bank(UINT32 bankaddress);
void neogeo_init_cpu2_setbank(void);
void neogeo_register_main_savestate(void);

/*----------- defined in machine/neogeo.c -----------*/

extern data16_t *neogeo_ram16;
extern data16_t *neogeo_sram16;

extern int mcd_action;
extern int mcd_number;
extern int memcard_status;
extern int memcard_number;
extern int memcard_manager;
extern UINT8 *neogeo_memcard;

extern data8_t *neogeo_game_vectors;

MACHINE_INIT( neogeo );
DRIVER_INIT( neogeo );

WRITE16_HANDLER( neogeo_sram16_lock_w );
WRITE16_HANDLER( neogeo_sram16_unlock_w );
READ16_HANDLER( neogeo_sram16_r );
WRITE16_HANDLER( neogeo_sram16_w );

NVRAM_HANDLER( neogeo );

READ16_HANDLER( neogeo_memcard16_r );
WRITE16_HANDLER( neogeo_memcard16_w );
int neogeo_memcard_load(int);
void neogeo_memcard_save(void);
void neogeo_memcard_eject(void);
int neogeo_memcard_create(int);


/*----------- defined in machine/neocrypt.c -----------*/
extern int neogeo_rng;
extern int neogeo_fix_bank_type;

void kof99_neogeo_gfx_decrypt(int extra_xor);
void kof2000_neogeo_gfx_decrypt(int extra_xor);
void cmc50_neogeo_gfx_decrypt(int extra_xor);
void neogeo_cmc50_m1_decrypt( void );
void neo_pcm2_snk_1999(int value);
void neo_pcm2_swap(int value);
void kog_px_decrypt(void);
void neogeo_bootleg_cx_decrypt(void);
void neogeo_bootleg_sx_decrypt(int value);
void decrypt_cthd2003(void);
void patch_cthd2003(void);
void decrypt_ct2k3sp(void);
void kof2002_decrypt_68k(void);
void kf2k2mp_decrypt(void);
void kof2km2_px_decrypt(void);
void decrypt_kof2k4se_68k(void);
void install_kof10th_protection(void);
void decrypt_kof10th(void);
void decrypt_kf10thep(void);
void decrypt_kf2k5uni(void);
void kof2003_decrypt_68k(void);
void install_pvc_protection(void);
void kf2k3bl_install_protection(void);
void kf2k3pl_px_decrypt( void );
void kf2k3pl_install_protection(void);
void kf2k3upl_px_decrypt(void);
void kf2k3upl_install_protection(void);
void kof99_decrypt_68k(void);
void kof99_install_protection(void);
void garou_decrypt_68k(void);
void garou_install_protection(void);
void garouo_decrypt_68k(void);
void garouo_install_protection(void);
void mslug3_decrypt_68k(void);
void mslug3_install_protection(void);
void kof2000_decrypt_68k(void);
void kof2000_install_protection(void);
void kof98_decrypt_68k(void);
void install_kof98_protection(void);
void matrim_decrypt_68k(void);
void svcboot_px_decrypt( void );
void svcboot_cx_decrypt( void );
void samsho5_decrypt_68k(void);
void samsh5sp_decrypt_68k(void);
void mslug5_decrypt_68k(void);
void kof2003_px_decrypt( void );
void kof2003_sx_decrypt( void );
void kf2k3pcb_gfx_decrypt(void);
void kf2k3pcb_decrypt_68k(void);
void kf2k3pcb_decrypt_s1data(void);
void kof2003biosdecode(void);
void svcchaos_px_decrypt(void);
void svcchaos_vx_decrypt(void);
void svcpcb_gfx_decrypt(void);
void svcpcb_s1data_decrypt(void);
void svcplus_px_decrypt( void );
void svcplus_px_hack( void );
void svcplusa_px_decrypt( void );
void svcsplus_px_decrypt( void );
void svcsplus_px_hack( void );


/*----------- defined in vidhrdw/neogeo.c -----------*/

VIDEO_START( neogeo_mvs );

WRITE16_HANDLER( neogeo_setpalbank0_16_w );
WRITE16_HANDLER( neogeo_setpalbank1_16_w );
READ16_HANDLER( neogeo_paletteram16_r );
WRITE16_HANDLER( neogeo_paletteram16_w );

WRITE16_HANDLER( neogeo_vidram16_offset_w );
READ16_HANDLER( neogeo_vidram16_data_r );
WRITE16_HANDLER( neogeo_vidram16_data_w );
WRITE16_HANDLER( neogeo_vidram16_modulo_w );
READ16_HANDLER( neogeo_vidram16_modulo_r );
WRITE16_HANDLER( neo_board_fix_16_w );
WRITE16_HANDLER( neo_game_fix_16_w );
WRITE16_HANDLER (neogeo_select_bios_vectors);
WRITE16_HANDLER (neogeo_select_game_vectors);

VIDEO_UPDATE( neogeo );
VIDEO_UPDATE( neogeo_raster );
void neogeo_vh_raster_partial_refresh(struct mame_bitmap *bitmap,int current_line);
