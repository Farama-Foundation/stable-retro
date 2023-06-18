#ifndef YM2203INTF_H
#define YM2203INTF_H

#include "ay8910.h"

#define MAX_2203 4

#define YM2203interface AY8910interface

/* volume level for YM2203 */
#define YM2203_VOL(FM_VOLUME,SSG_VOLUME) (((FM_VOLUME)<<16)+(SSG_VOLUME))

READ_HANDLER( YM2203_status_port_0_r );
READ_HANDLER( YM2203_status_port_1_r );
READ_HANDLER( YM2203_status_port_2_r );
READ_HANDLER( YM2203_status_port_3_r );
READ_HANDLER( YM2203_status_port_4_r );

READ_HANDLER( YM2203_read_port_0_r );
READ_HANDLER( YM2203_read_port_1_r );
READ_HANDLER( YM2203_read_port_2_r );
READ_HANDLER( YM2203_read_port_3_r );
READ_HANDLER( YM2203_read_port_4_r );

WRITE_HANDLER( YM2203_control_port_0_w );
WRITE_HANDLER( YM2203_control_port_1_w );
WRITE_HANDLER( YM2203_control_port_2_w );
WRITE_HANDLER( YM2203_control_port_3_w );
WRITE_HANDLER( YM2203_control_port_4_w );

WRITE_HANDLER( YM2203_write_port_0_w );
WRITE_HANDLER( YM2203_write_port_1_w );
WRITE_HANDLER( YM2203_write_port_2_w );
WRITE_HANDLER( YM2203_write_port_3_w );
WRITE_HANDLER( YM2203_write_port_4_w );

WRITE_HANDLER( YM2203_word_0_w );
WRITE_HANDLER( YM2203_word_1_w );

int YM2203_sh_start(const struct MachineSound *msound);
void YM2203_sh_stop(void);
void YM2203_sh_reset(void);

void YM2203UpdateRequest(int chip);

#endif
