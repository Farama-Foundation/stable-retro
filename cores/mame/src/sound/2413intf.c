/****************************************************************

	MAME / MESS functions

****************************************************************/

#include "driver.h"
#include "ym2413.h"

/*#define YM2413ISA*/
#ifdef YM2413ISA
	#include <pc.h>
#endif


/* for stream system */
static int stream[MAX_2413];

static const struct YM2413interface *intf;

/*
void YM2413DAC_update(int chip,INT16 *buffer,int length)
{
	static int out = 0;

	if ( ym2413[chip].reg[0x0F] & 0x01 )
	{
		out = ((ym2413[chip].reg[0x10] & 0xF0) << 7);
	}
	while (length--) *(buffer++) = out;
}
*/
int YM2413_sh_start (const struct MachineSound *msound)
{
	char buf[2][40];
	const char *name[2];
	int volume[2];

	int i;
	int rate = Machine->sample_rate;

	intf = msound->sound_interface;
	if( intf->num > MAX_2413 ) return 1;

	rate = intf->baseclock/72;


	/* emulator create */
	if ( YM2413Init(intf->num, intf->baseclock, rate) != 0)
		return 1;

	for (i = 0; i < intf->num; i++)
	{
		/* stream setup */

		int vol = intf->mixing_level[i];

		name[0]=buf[0];
		name[1]=buf[1];
		volume[0] = vol & 0xffff;
		vol>>=16;
		volume[1] = vol & 0xffff;

		/* YM2413 has two separate outputs:
		**        1 for melody output
		**        1 for rhythm output (percussion)
        */
		sprintf(buf[0],"%s #%d MO",sound_name(msound),i);
		sprintf(buf[1],"%s #%d RO",sound_name(msound),i);

		/* stream system initialize */
		stream[i] = stream_init_multi(2,
				name,volume,rate,i,YM2413UpdateOne);

		YM2413SetUpdateHandler(i, stream_update, stream[i]);
	}
	return 0;




#if 0
	int i, tst;
	char name[40];

	num = intf->num;

	tst = YM3812_sh_start (msound);
	if (tst)
		return 1;

	for (i=0;i<num;i++)
	{
		ym2413_reset (i);
		sprintf(name,"YM-2413 DAC #%d",i);

		ym2413[i].DAC_stream = stream_init(name,intf->mixing_level[i],
		                       Machine->sample_rate, i, YM2413DAC_update);

		if (ym2413[i].DAC_stream == -1)
			return 1;
	}
	return 0;
#endif

}

void YM2413_sh_stop (void)
{
	YM2413Shutdown();
}

void YM2413_sh_reset (void)
{
	int i;

	for (i=0;i<intf->num;i++)
	{
		YM2413ResetChip(i);
	}
}


#ifdef YM2413ISA
WRITE_HANDLER( YM2413_register_port_0_w ) {
int i,a;
	outportb(0x308,data); /* ym2413_write (0, 0, data);*/
	/*add delay*/
	for (i=0; i<0x20; i++)
		a = inportb(0x80);

 } /* 1st chip */
#else
WRITE_HANDLER( YM2413_register_port_0_w ) { YM2413Write (0, 0, data); } /* 1st chip */
#endif
WRITE_HANDLER( YM2413_register_port_1_w ) { YM2413Write (1, 0, data); } /* 2nd chip */
WRITE_HANDLER( YM2413_register_port_2_w ) { YM2413Write (2, 0, data); } /* 3rd chip */
WRITE_HANDLER( YM2413_register_port_3_w ) { YM2413Write (3, 0, data); } /* 4th chip */

#ifdef YM2413ISA
WRITE_HANDLER( YM2413_data_port_0_w ) {
int i,a;
	outportb(0x309,data);/* YM2413Write (0, 1, data);*/
	/*add delay*/
	for (i=0; i<0x40; i++)
		a = inportb(0x80);
 } /* 1st chip */
#else
WRITE_HANDLER( YM2413_data_port_0_w ) { YM2413Write (0, 1, data); } /* 1st chip */
#endif
WRITE_HANDLER( YM2413_data_port_1_w ) { YM2413Write (1, 1, data); } /* 2nd chip */
WRITE_HANDLER( YM2413_data_port_2_w ) { YM2413Write (2, 1, data); } /* 3rd chip */
WRITE_HANDLER( YM2413_data_port_3_w ) { YM2413Write (3, 1, data); } /* 4th chip */

WRITE16_HANDLER( YM2413_register_port_0_lsb_w ) { if (ACCESSING_LSB) YM2413_register_port_0_w(offset,data & 0xff); }
WRITE16_HANDLER( YM2413_register_port_1_lsb_w ) { if (ACCESSING_LSB) YM2413_register_port_1_w(offset,data & 0xff); }
WRITE16_HANDLER( YM2413_register_port_2_lsb_w ) { if (ACCESSING_LSB) YM2413_register_port_2_w(offset,data & 0xff); }
WRITE16_HANDLER( YM2413_register_port_3_lsb_w ) { if (ACCESSING_LSB) YM2413_register_port_3_w(offset,data & 0xff); }
WRITE16_HANDLER( YM2413_data_port_0_lsb_w ) { if (ACCESSING_LSB) YM2413_data_port_0_w(offset,data & 0xff); }
WRITE16_HANDLER( YM2413_data_port_1_lsb_w ) { if (ACCESSING_LSB) YM2413_data_port_1_w(offset,data & 0xff); }
WRITE16_HANDLER( YM2413_data_port_2_lsb_w ) { if (ACCESSING_LSB) YM2413_data_port_2_w(offset,data & 0xff); }
WRITE16_HANDLER( YM2413_data_port_3_lsb_w ) { if (ACCESSING_LSB) YM2413_data_port_3_w(offset,data & 0xff); }
