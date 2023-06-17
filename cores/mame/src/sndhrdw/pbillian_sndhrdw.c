#include "driver.h"

static int channel;
static signed char *samplebuf;

int pbillian_sh_start(const struct MachineSound *msound)
{
	int i;
	unsigned char *ROM = memory_region(REGION_SOUND1);
	channel = mixer_allocate_channel(50);
	mixer_set_name(channel,"Samples");
	samplebuf = auto_malloc(memory_region_length(REGION_SOUND1));
	for(i=0;i<memory_region_length(REGION_SOUND1);i++)samplebuf[i]=ROM[i]-0x80;
	return 0;
}

#define pb_play_s(start,end) mixer_play_sample(channel,samplebuf + (start<<8),	(end-start)<<8,5000,0) /* 5khz ?*/

WRITE_HANDLER(data_41a_w)
{
	/* 
	 i/o port $41a wrties are sample related
	 playback is done probably using mcu (missing dump)
	 It's just a guess for now. 
	 Value from port $41a can be offset in some  table,
	 offset in sample rom , some mixed value of sample offset/length
	 or freq
	
	 Code works  (somehow) only for prebillian 
	 
	*/
	
	switch (data)
	{
		case 0x00:	pb_play_s(0x00,0x06);break;
		case 0x1c:	pb_play_s(0x1c,0x2d);break;
		case 0xad:  pb_play_s(0x2d,0x2f);break; /*code at 0x0a51,0x1e21,0x2b17  'ball in pocket' .. etc*/
		case 0x2f:	pb_play_s(0x2f,0x38);break;
		case 0x38:	pb_play_s(0x38,0x42);break;
		case 0xc2:	pb_play_s(0x42,0x46);break; /*code at 0x1f42,0x2bc3,*/
		case 0x46:	pb_play_s(0x46,0x57);break;
		case 0x57:	pb_play_s(0x57,0x59);break;
		case 0x59:	pb_play_s(0x59,0x5f);break;
		case 0xdf:  pb_play_s(0x5f,0x62);break; /*code at 0x1eb2*/
		case 0x62:	pb_play_s(0x62,0x6d);break;
		default: log_cb(RETRO_LOG_DEBUG, LOGPRE "[41a] W %x at %x\n",data,activecpu_get_previouspc());
	}
}	
