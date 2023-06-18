/***************************************************************************

	Midway DCS Audio Board

****************************************************************************/

MACHINE_DRIVER_EXTERN( dcs_audio );
MACHINE_DRIVER_EXTERN( dcs_audio_uart );
MACHINE_DRIVER_EXTERN( dcs2_audio );
MACHINE_DRIVER_EXTERN( dcs2_audio_2104 );

void dcs_init(void);
void dcs2_init(offs_t polling_offset);
void dcs_set_auto_ack(int state);

void dcs_set_fifo_callbacks(UINT16 (*fifo_data_r)(void), UINT16 (*fifo_status_r)(void));
void dcs_set_io_callbacks(void (*output_full_cb)(int), void (*input_empty_cb)(int));

int dcs_data_r(void);
void dcs_ack_w(void);
int dcs_data2_r(void);
int dcs_control_r(void);

void dcs_data_w(int data);
void dcs_reset_w(int state);
