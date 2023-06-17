/*************************************************************************

	Sega Z80-3D system

*************************************************************************/

/*----------- defined in machine/turbo.c -----------*/

extern UINT8 turbo_opa, turbo_opb, turbo_opc;
extern UINT8 turbo_ipa, turbo_ipb, turbo_ipc;
extern UINT8 turbo_fbpla, turbo_fbcol;

extern UINT8 subroc3d_col, subroc3d_ply, subroc3d_chofs;

extern UINT8 buckrog_fchg, buckrog_mov, buckrog_obch;

MACHINE_INIT( turbo );
MACHINE_INIT( subroc3d );
MACHINE_INIT( buckrog );

READ_HANDLER( turbo_8279_r );
WRITE_HANDLER( turbo_8279_w );

READ_HANDLER( turbo_collision_r );
WRITE_HANDLER( turbo_collision_clear_w );
WRITE_HANDLER( turbo_coin_and_lamp_w );

void turbo_rom_decode(void);

void turbo_update_tachometer(void);
void turbo_update_segments(void);

READ_HANDLER( buckrog_cpu2_command_r );
READ_HANDLER( buckrog_port_2_r );
READ_HANDLER( buckrog_port_3_r );


/*----------- defined in vidhrdw/turbo.c -----------*/

extern UINT8 *sega_sprite_position;
extern UINT8 turbo_collision;

PALETTE_INIT( turbo );
VIDEO_START( turbo );
VIDEO_EOF( turbo );
VIDEO_UPDATE( turbo );

PALETTE_INIT( subroc3d );
VIDEO_START( subroc3d );
VIDEO_UPDATE( subroc3d );

PALETTE_INIT( buckrog );
VIDEO_START( buckrog );
VIDEO_UPDATE( buckrog );

WRITE_HANDLER( buckrog_led_display_w );
WRITE_HANDLER( buckrog_bitmap_w );
