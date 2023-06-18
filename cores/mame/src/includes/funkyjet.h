VIDEO_START( funkyjet );
VIDEO_UPDATE( funkyjet );

WRITE16_HANDLER( funkyjet_pf2_data_w );
WRITE16_HANDLER( funkyjet_pf1_data_w );
WRITE16_HANDLER( funkyjet_control_0_w );

extern data16_t *funkyjet_pf1_data;
extern data16_t *funkyjet_pf2_data;
extern data16_t *funkyjet_pf1_row;
