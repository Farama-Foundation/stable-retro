/* Video emulation definitions */
VIDEO_START( dec0 );
VIDEO_START( dec0_nodma );
VIDEO_UPDATE( hbarrel );
VIDEO_UPDATE( baddudes );
VIDEO_UPDATE( birdtry );
VIDEO_UPDATE( robocop );
VIDEO_UPDATE( hippodrm );
VIDEO_UPDATE( slyspy );
VIDEO_UPDATE( midres );

extern data16_t *dec0_pf1_rowscroll,*dec0_pf2_rowscroll,*dec0_pf3_rowscroll;
extern data16_t *dec0_pf1_colscroll,*dec0_pf2_colscroll,*dec0_pf3_colscroll;
extern data16_t *dec0_pf1_data,*dec0_pf2_data,*dec0_pf3_data;

WRITE16_HANDLER( dec0_pf1_control_0_w );
WRITE16_HANDLER( dec0_pf1_control_1_w );
WRITE16_HANDLER( dec0_pf1_data_w );
WRITE16_HANDLER( dec0_pf2_control_0_w );
WRITE16_HANDLER( dec0_pf2_control_1_w );
WRITE16_HANDLER( dec0_pf2_data_w );
WRITE16_HANDLER( dec0_pf3_control_0_w );
WRITE16_HANDLER( dec0_pf3_control_1_w );
WRITE16_HANDLER( dec0_pf3_data_w );
WRITE16_HANDLER( dec0_priority_w );
WRITE16_HANDLER( dec0_update_sprites_w );

WRITE16_HANDLER( dec0_paletteram_rg_w );
WRITE16_HANDLER( dec0_paletteram_b_w );

READ_HANDLER( dec0_pf3_data_8bit_r );
WRITE_HANDLER( dec0_pf3_data_8bit_w );
WRITE_HANDLER( dec0_pf3_control_8bit_w );

/* Machine prototypes */
READ16_HANDLER( dec0_controls_r );
READ16_HANDLER( dec0_rotary_r );
READ16_HANDLER( midres_controls_r );
READ16_HANDLER( slyspy_controls_r );
READ16_HANDLER( slyspy_protection_r );
WRITE16_HANDLER( slyspy_state_w );
READ16_HANDLER( slyspy_state_r );
WRITE16_HANDLER( slyspy_240000_w );
WRITE16_HANDLER( slyspy_242000_w );
WRITE16_HANDLER( slyspy_246000_w );
WRITE16_HANDLER( slyspy_248000_w );
WRITE16_HANDLER( slyspy_24c000_w );
WRITE16_HANDLER( slyspy_24e000_w );

WRITE_HANDLER( robocop_bankswitch_w );

DRIVER_INIT( slyspy );
DRIVER_INIT( hippodrm );
DRIVER_INIT( robocop );
DRIVER_INIT( baddudes );
DRIVER_INIT( hbarrel );
DRIVER_INIT( hbarrelw );
DRIVER_INIT( birdtry );

extern void dec0_i8751_write(int data);
extern void dec0_i8751_reset(void);
READ_HANDLER( hippodrm_prot_r );
WRITE_HANDLER( hippodrm_prot_w );
READ_HANDLER( hippodrm_shared_r );
WRITE_HANDLER( hippodrm_shared_w );

extern data16_t *dec0_ram;
extern data8_t *robocop_shared_ram;
