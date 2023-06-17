#ifndef AY8910_H
#define AY8910_H

#define MAX_8910 5
#define ALL_8910_CHANNELS -1

struct AY8910interface
{
	int num;	/* total number of 8910 in the machine */
	int baseclock;
	int mixing_level[MAX_8910];
	mem_read_handler portAread[MAX_8910];
	mem_read_handler portBread[MAX_8910];
	mem_write_handler portAwrite[MAX_8910];
	mem_write_handler portBwrite[MAX_8910];
	void (*handler[MAX_8910])(int irq);	/* IRQ handler for the YM2203 */
};

void AY8910_reset(int chip);

void AY8910_set_clock(int chip,int _clock);
void AY8910_set_volume(int chip,int channel,int volume);


void AY8910Write(int chip,int a,int data);
int AY8910Read(int chip);


READ_HANDLER( AY8910_read_port_0_r );
READ_HANDLER( AY8910_read_port_1_r );
READ_HANDLER( AY8910_read_port_2_r );
READ_HANDLER( AY8910_read_port_3_r );
READ_HANDLER( AY8910_read_port_4_r );
READ16_HANDLER( AY8910_read_port_0_lsb_r );
READ16_HANDLER( AY8910_read_port_1_lsb_r );
READ16_HANDLER( AY8910_read_port_2_lsb_r );
READ16_HANDLER( AY8910_read_port_3_lsb_r );
READ16_HANDLER( AY8910_read_port_4_lsb_r );
READ16_HANDLER( AY8910_read_port_0_msb_r );
READ16_HANDLER( AY8910_read_port_1_msb_r );
READ16_HANDLER( AY8910_read_port_2_msb_r );
READ16_HANDLER( AY8910_read_port_3_msb_r );
READ16_HANDLER( AY8910_read_port_4_msb_r );

WRITE_HANDLER( AY8910_control_port_0_w );
WRITE_HANDLER( AY8910_control_port_1_w );
WRITE_HANDLER( AY8910_control_port_2_w );
WRITE_HANDLER( AY8910_control_port_3_w );
WRITE_HANDLER( AY8910_control_port_4_w );
WRITE16_HANDLER( AY8910_control_port_0_lsb_w );
WRITE16_HANDLER( AY8910_control_port_1_lsb_w );
WRITE16_HANDLER( AY8910_control_port_2_lsb_w );
WRITE16_HANDLER( AY8910_control_port_3_lsb_w );
WRITE16_HANDLER( AY8910_control_port_4_lsb_w );
WRITE16_HANDLER( AY8910_control_port_0_msb_w );
WRITE16_HANDLER( AY8910_control_port_1_msb_w );
WRITE16_HANDLER( AY8910_control_port_2_msb_w );
WRITE16_HANDLER( AY8910_control_port_3_msb_w );
WRITE16_HANDLER( AY8910_control_port_4_msb_w );

WRITE_HANDLER( AY8910_write_port_0_w );
WRITE_HANDLER( AY8910_write_port_1_w );
WRITE_HANDLER( AY8910_write_port_2_w );
WRITE_HANDLER( AY8910_write_port_3_w );
WRITE_HANDLER( AY8910_write_port_4_w );
WRITE16_HANDLER( AY8910_write_port_0_lsb_w );
WRITE16_HANDLER( AY8910_write_port_1_lsb_w );
WRITE16_HANDLER( AY8910_write_port_2_lsb_w );
WRITE16_HANDLER( AY8910_write_port_3_lsb_w );
WRITE16_HANDLER( AY8910_write_port_4_lsb_w );
WRITE16_HANDLER( AY8910_write_port_0_msb_w );
WRITE16_HANDLER( AY8910_write_port_1_msb_w );
WRITE16_HANDLER( AY8910_write_port_2_msb_w );
WRITE16_HANDLER( AY8910_write_port_3_msb_w );
WRITE16_HANDLER( AY8910_write_port_4_msb_w );

int AY8910_sh_start(const struct MachineSound *msound);
void AY8910_sh_stop(void);
void AY8910_sh_reset(void);

/*********** An interface for SSG of YM2203 ***********/

/* When both of AY8910 and YM2203 or YM2608 or YM2610 are used.      */
/* It must be called AY8910_sh_start () before AY8910_sh_start_ym()  */

extern int ay8910_index_ym;

void AY8910_sh_stop_ym(void);
int AY8910_sh_start_ym(const struct MachineSound *msound);
#endif
