void pleiads_sound_update(INT16 *buffer, INT32 length);
void pleiads_sound_control_a_w(INT32 address, UINT8 data);
void pleiads_sound_control_b_w(INT32 address, UINT8 data);
void pleiads_sound_control_c_w(INT32 address, UINT8 data);
void pleiads_sound_reset();
void naughtyb_sound_reset();
void popflame_sound_reset();
void pleiads_sound_init(INT32 naughtybpopflamer);
void pleiads_sound_deinit();
void pleiads_sound_scan(INT32 nAction, INT32 *pnMin);

