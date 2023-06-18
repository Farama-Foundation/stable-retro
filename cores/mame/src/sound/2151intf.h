#ifndef YM2151INTF_H
#define YM2151INTF_H

#define MAX_2151 2

#ifndef YM3012_VOL
/* YM2151interface->volume macro */
#define YM3012_VOL(LVol,LPan,RVol,RPan) (MIXER(LVol,LPan)|(MIXER(RVol,RPan) << 16))
#endif

struct YM2151interface
{
	int num;
	int baseclock;
	int volume[MAX_2151]; /* use YM3012_VOL() macro to fill this field */
	void (*irqhandler[MAX_2151])(int irq);
	mem_write_handler portwritehandler[MAX_2151];
};

READ_HANDLER( YM2151_status_port_0_r );
READ_HANDLER( YM2151_status_port_1_r );
READ_HANDLER( YM2151_status_port_2_r );

WRITE_HANDLER( YM2151_register_port_0_w );
WRITE_HANDLER( YM2151_register_port_1_w );
WRITE_HANDLER( YM2151_register_port_2_w );

WRITE_HANDLER( YM2151_data_port_0_w );
WRITE_HANDLER( YM2151_data_port_1_w );
WRITE_HANDLER( YM2151_data_port_2_w );

WRITE_HANDLER( YM2151_word_0_w );
WRITE_HANDLER( YM2151_word_1_w );

READ16_HANDLER( YM2151_status_port_0_lsb_r );
READ16_HANDLER( YM2151_status_port_1_lsb_r );
READ16_HANDLER( YM2151_status_port_2_lsb_r );

WRITE16_HANDLER( YM2151_register_port_0_lsb_w );
WRITE16_HANDLER( YM2151_register_port_1_lsb_w );
WRITE16_HANDLER( YM2151_register_port_2_lsb_w );

WRITE16_HANDLER( YM2151_data_port_0_lsb_w );
WRITE16_HANDLER( YM2151_data_port_1_lsb_w );
WRITE16_HANDLER( YM2151_data_port_2_lsb_w );

int YM2151_sh_start(const struct MachineSound *msound);
void YM2151_sh_stop(void);
void YM2151_sh_reset(void);

void YM2151UpdateRequest(int chip);
#endif
