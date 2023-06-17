/*************************************************************************

	Atari Skydiver hardware

*************************************************************************/

/*----------- defined in vidhrdw/skydiver.c -----------*/

extern data8_t *skydiver_videoram;

MACHINE_INIT( skydiver );
WRITE_HANDLER( skydiver_videoram_w );
WRITE_HANDLER( skydiver_wram_w );	/* the signal is WRAM, presumably Work RAM */
READ_HANDLER( skydiver_wram_r );
WRITE_HANDLER( skydiver_start_lamp_1_w );
WRITE_HANDLER( skydiver_start_lamp_2_w );
WRITE_HANDLER( skydiver_lamp_s_w );
WRITE_HANDLER( skydiver_lamp_k_w );
WRITE_HANDLER( skydiver_lamp_y_w );
WRITE_HANDLER( skydiver_lamp_d_w );
WRITE_HANDLER( skydiver_lamp_i_w );
WRITE_HANDLER( skydiver_lamp_v_w );
WRITE_HANDLER( skydiver_lamp_e_w );
WRITE_HANDLER( skydiver_lamp_r_w );
WRITE_HANDLER( skydiver_width_w );
WRITE_HANDLER( skydiver_coin_lockout_w );
VIDEO_START( skydiver );
VIDEO_UPDATE( skydiver );
