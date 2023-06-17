/*
	vlm5030.c

	VLM5030 emulator

	Written by Tatsuyuki Satoh
	Based on TMS5220 simulator (tms5220.c)

  note:
	memory read cycle(==sampling rate) = 122.9u(440clock)
	interpolator (LC8109 = 2.5ms)      = 20 * samples(125us)
	frame time (20ms)                  =  4 * interpolator
	9bit DAC is composed of 5bit Physical and 3bitPWM.

  todo:
	Noise Generator circuit without 'rand()' function.

----------- command format (Analytical result) ----------

1)end of speech (8bit)
:00000011:

2)silent some frame (8bit)
:????SS01:

SS : number of silent frames
   00 = 2 frame
   01 = 4 frame
   10 = 6 frame
   11 = 8 frame

3)-speech frame (48bit)
function:   6th  :  5th   :   4th  :   3rd  :   2nd  : 1st    :
end     :   ---  :  ---   :   ---  :   ---  :   ---  :00000011:
silent  :   ---  :  ---   :   ---  :   ---  :   ---  :0000SS01:
speech  :11111122:22233334:44455566:67778889:99AAAEEE:EEPPPPP0:

EEEEE  : energy : volume 0=off,0x1f=max
PPPPP  : pitch  : 0=noize , 1=fast,0x1f=slow
111111 : K1     : 48=off
22222  : K2     : 0=off,1=+min,0x0f=+max,0x10=off,0x11=+max,0x1f=-min
                : 16 == special function??
3333   : K3     : 0=off,1=+min,0x07=+max,0x08=-max,0x0f=-min
4444   : K4     :
555    : K5     : 0=off,1=+min,0x03=+max,0x04=-max,0x07=-min
666    : K6     :
777    : K7     :
888    : K8     :
999    : K9     :
AAA    : K10    :

 ---------- chirp table information ----------

DAC PWM cycle == 88system clock , (11clock x 8 pattern) = 40.6KHz
one chirp     == 5 x PWM cycle == 440systemclock(8,136Hz)

chirp  0   : volume 10- 8 : with filter
chirp  1   : volume  8- 6 : with filter
chirp  2   : volume  6- 4 : with filter
chirp  3   : volume   4   : no filter ??
chirp  4- 5: volume  4- 2 : with filter
chirp  6-11: volume  2- 0 : with filter
chirp 12-..: vokume   0   : silent

 ---------- digial output information ----------
 when ME pin = high , some status output to A0..15 pins

  A0..8   : DAC output value (abs)
  A9      : DAC sign flag , L=minus,H=Plus
  A10     : energy reload flag (pitch pulse)
  A11..15 : unknown

  [DAC output value(signed 6bit)] = A9 ? A0..8 : -(A0..8)

*/
#include "driver.h"
#include "state.h"
#include "vlm5030.h"

/* interpolator per frame   */
#define FR_SIZE 4
/* samples per interpolator */
#define IP_SIZE_SLOWER  (240/FR_SIZE)
#define IP_SIZE_SLOW    (200/FR_SIZE)
#define IP_SIZE_NORMAL  (160/FR_SIZE)
#define IP_SIZE_FAST    (120/FR_SIZE)
#define IP_SIZE_FASTER  ( 80/FR_SIZE)

static const struct VLM5030interface *intf;

static int channel;
static int schannel;

/* need to save state */

static UINT8 *VLM5030_rom;
static int VLM5030_address_mask;
static UINT16 VLM5030_address;
static UINT8 pin_BSY;
static UINT8 pin_ST;
static UINT8 pin_VCU;
static UINT8 pin_RST;
static UINT8 latch_data;
static UINT16 vcu_addr_h;
static UINT8 VLM5030_parameter;
static UINT8 VLM5030_phase;

/* state of option paramter */
static int VLM5030_frame_size;
static int pitch_offset;
static UINT8 interp_step;

static UINT8 interp_count;       /* number of interp periods    */
static UINT8 sample_count;       /* sample number within interp */
static UINT8 pitch_count;

/* these contain data describing the current and previous voice frames */
static UINT16 old_energy;
static UINT8 old_pitch;
static INT16  old_k[10];
static UINT16 target_energy;
static UINT8 target_pitch;
static INT16 target_k[10];

static UINT16 new_energy;
static UINT8 new_pitch;
static INT16 new_k[10];

/* these are all used to contain the current state of the sound generation */
static unsigned int current_energy;
static unsigned int current_pitch;
static int current_k[10];

static INT32 x[10];

/* phase value */
enum {
	PH_RESET,
	PH_IDLE,
	PH_SETUP,
	PH_WAIT,
	PH_RUN,
	PH_STOP,
	PH_END
};

/*
  speed parameter
SPC SPB SPA
 1   0   1  more slow (05h)     : 42ms   (150%) : 60sample
 1   1   x  slow      (06h,07h) : 34ms   (125%) : 50sample
 x   0   0  normal    (00h,04h) : 25.6ms (100%) : 40samplme
 0   0   1  fast      (01h)     : 20.2ms  (75%) : 30sample
 0   1   x  more fast (02h,03h) : 12.2ms  (50%) : 20sample
*/
static const int VLM5030_speed_table[8] =
{
 IP_SIZE_NORMAL,
 IP_SIZE_FAST,
 IP_SIZE_FASTER,
 IP_SIZE_FASTER,
 IP_SIZE_NORMAL,
 IP_SIZE_SLOWER,
 IP_SIZE_SLOW,
 IP_SIZE_SLOW
};

static const char VLM_NAME[] = "VLM5030";

/* ROM Tables */

/* This is the energy lookup table */

/* sampled from real chip */
static unsigned short energytable[0x20] =
{
	  0,  2,  4,  6, 10, 12, 14, 18, /*  0-7  */
	 22, 26, 30, 34, 38, 44, 48, 54, /*  8-15 */
	 62, 68, 76, 84, 94,102,114,124, /* 16-23 */
	136,150,164,178,196,214,232,254  /* 24-31 */
};

/* This is the pitch lookup table */
static const unsigned char pitchtable [0x20]=
{
   1,                               /* 0     : random mode */
   22,                              /* 1     : start=22    */
   23, 24, 25, 26, 27, 28, 29, 30,  /*  2- 9 : 1step       */
   32, 34, 36, 38, 40, 42, 44, 46,  /* 10-17 : 2step       */
   50, 54, 58, 62, 66, 70, 74, 78,  /* 18-25 : 4step       */
   86, 94, 102,110,118,126          /* 26-31 : 8step       */
};

static const INT16 K1_table[] = {
  -24898,  -25672,  -26446,  -27091,  -27736,  -28252,  -28768,  -29155,
  -29542,  -29929,  -30316,  -30574,  -30832,  -30961,  -31219,  -31348,
  -31606,  -31735,  -31864,  -31864,  -31993,  -32122,  -32122,  -32251,
  -32251,  -32380,  -32380,  -32380,  -32509,  -32509,  -32509,  -32509,
   24898,   23995,   22963,   21931,   20770,   19480,   18061,   16642,
   15093,   13416,   11610,    9804,    7998,    6063,    3999,    1935,
       0,   -1935,   -3999,   -6063,   -7998,   -9804,  -11610,  -13416,
  -15093,  -16642,  -18061,  -19480,  -20770,  -21931,  -22963,  -23995
};
static const INT16 K2_table[] = {
       0,   -3096,   -6321,   -9417,  -12513,  -15351,  -18061,  -20770,
  -23092,  -25285,  -27220,  -28897,  -30187,  -31348,  -32122,  -32638,
       0,   32638,   32122,   31348,   30187,   28897,   27220,   25285,
   23092,   20770,   18061,   15351,   12513,    9417,    6321,    3096
};
static const INT16 K3_table[] = {
       0,   -3999,   -8127,  -12255,  -16384,  -20383,  -24511,  -28639,
   32638,   28639,   24511,   20383,   16254,   12255,    8127,    3999
};
static const INT16 K5_table[] = {
       0,   -8127,  -16384,  -24511,   32638,   24511,   16254,    8127
};

static int get_bits(int sbit,int bits)
{
	int offset = VLM5030_address + (sbit>>3);
	int data;

	data = VLM5030_rom[offset&VLM5030_address_mask] +
	       (((int)VLM5030_rom[(offset+1)&VLM5030_address_mask])*256);
	data >>= (sbit&7);
	data &= (0xff>>(8-bits));

	return data;
}

/* get next frame */
static int parse_frame (void)
{
	unsigned char cmd;
	int i;

	/* remember previous frame */
	old_energy = new_energy;
	old_pitch = new_pitch;
	for(i=0;i<=9;i++)
		old_k[i] = new_k[i];

	/* command byte check */
	cmd = VLM5030_rom[VLM5030_address&VLM5030_address_mask];
	if( cmd & 0x01 )
	{	/* extend frame */
		new_energy = new_pitch = 0;
		for(i=0;i<=9;i++)
			new_k[i] = 0;
		VLM5030_address++;
		if( cmd & 0x02 )
		{	/* end of speech */

			log_cb(RETRO_LOG_DEBUG, LOGPRE "VLM5030 %04X end \n",VLM5030_address );
			return 0;
		}
		else
		{	/* silent frame */
			int nums = ( (cmd>>2)+1 )*2;
			log_cb(RETRO_LOG_DEBUG, LOGPRE "VLM5030 %04X silent %d frame\n",VLM5030_address,nums );
			return nums * FR_SIZE;
		}
	}
	/* pitch */
	new_pitch  = ( pitchtable[get_bits( 1,5)] + pitch_offset )&0xff;
	/* energy */
	new_energy = energytable[get_bits( 6,5)];

	/* 10 K's */
	new_k[9] = K5_table[get_bits(11,3)];
	new_k[8] = K5_table[get_bits(14,3)];
	new_k[7] = K5_table[get_bits(17,3)];
	new_k[6] = K5_table[get_bits(20,3)];
	new_k[5] = K5_table[get_bits(23,3)];
	new_k[4] = K5_table[get_bits(26,3)];
	new_k[3] = K3_table[get_bits(29,4)];
	new_k[2] = K3_table[get_bits(33,4)];
	new_k[1] = K2_table[get_bits(37,5)];
	new_k[0] = K1_table[get_bits(42,6)];

	VLM5030_address+=6;
	log_cb(RETRO_LOG_DEBUG, LOGPRE "VLM5030 %04X voice \n",VLM5030_address );
	return FR_SIZE;
}

/* decode and buffering data */
static void vlm5030_update_callback(int num,INT16 *buffer, int length)
{
	int buf_count=0;
	int interp_effect;
	int i;
	int u[11];

	/* running */
	if( VLM5030_phase == PH_RUN || VLM5030_phase == PH_STOP )
	{
		/* playing speech */
		while (length > 0)
		{
			int current_val;

			/* check new interpolator or  new frame */
			if( sample_count == 0 )
			{
				if( VLM5030_phase == PH_STOP )
				{
					VLM5030_phase = PH_END;
					sample_count = 1;
					goto phase_stop; /* continue to end phase */
				}
				sample_count = VLM5030_frame_size;
				/* interpolator changes */
				if ( interp_count == 0 )
				{
					/* change to new frame */
					interp_count = parse_frame(); /* with change phase */
					if ( interp_count == 0 )
					{	/* end mark found */
						interp_count = FR_SIZE;
						sample_count = VLM5030_frame_size; /* end -> stop time */
						VLM5030_phase = PH_STOP;
					}
					/* Set old target as new start of frame */
					current_energy = old_energy;
					current_pitch = old_pitch;
					for(i=0;i<=9;i++)
						current_k[i] = old_k[i];
					/* is this a zero energy frame? */
					if (current_energy == 0)
					{
						/*printf("processing frame: zero energy\n");*/
						target_energy = 0;
						target_pitch = current_pitch;
						for(i=0;i<=9;i++)
							target_k[i] = current_k[i];
					}
					else
					{
						/*printf("processing frame: Normal\n");*/
						/*printf("*** Energy = %d\n",current_energy);*/
						/*printf("proc: %d %d\n",last_fbuf_head,fbuf_head);*/
						target_energy = new_energy;
						target_pitch = new_pitch;
						for(i=0;i<=9;i++)
							target_k[i] = new_k[i];
					}
				}
				/* next interpolator */
				/* Update values based on step values 25% , 50% , 75% , 100% */
				interp_count -= interp_step;
				/* 3,2,1,0 -> 1,2,3,4 */
				interp_effect = FR_SIZE - (interp_count%FR_SIZE);
				current_energy = old_energy + (target_energy - old_energy) * interp_effect / FR_SIZE;
				if (old_pitch > 1)
					current_pitch = old_pitch + (target_pitch - old_pitch) * interp_effect / FR_SIZE;
				for (i = 0; i <= 9 ; i++)
					current_k[i] = old_k[i] + (target_k[i] - old_k[i]) * interp_effect / FR_SIZE;
			}
			/* calcrate digital filter */
			if (old_energy == 0)
			{
				/* generate silent samples here */
				current_val = 0x00;
			}
			else if (old_pitch <= 1)
			{	/* generate unvoiced samples here */
				current_val = (rand()&1) ? current_energy : -current_energy;
			}
			else
			{
				/* generate voiced samples here */
				current_val = ( pitch_count == 0) ? current_energy : 0;
			}

			/* Lattice filter here */
			u[10] = current_val;
			for (i = 9; i >= 0; i--)
				u[i] = u[i+1] - ((current_k[i] * x[i]) / 32768);
			for (i = 9; i >= 1; i--)
				x[i] = x[i-1] + ((current_k[i-1] * u[i-1]) / 32768);
			x[0] = u[0];

			/* clipping, buffering */
			if (u[0] > 511)
				buffer[buf_count] = 511<<6;
			else if (u[0] < -511)
				buffer[buf_count] = -511<<6;
			else
				buffer[buf_count] = (u[0] << 6);
			buf_count++;

			/* sample count */
			sample_count--;
			/* pitch */
			pitch_count++;
			if (pitch_count >= current_pitch )
				pitch_count = 0;
			/* size */
			length--;
		}
/*		return;*/
	}
	/* stop phase */
phase_stop:
	switch( VLM5030_phase )
	{
	case PH_SETUP:
		if( sample_count <= length)
		{
			sample_count = 0;
			log_cb(RETRO_LOG_DEBUG, LOGPRE "VLM5030 BSY=H\n" );
			/* pin_BSY = 1; */
			VLM5030_phase = PH_WAIT;
		}
		else
		{
			sample_count -= length;
		}
		break;
	case PH_END:
		if( sample_count <= length)
		{
			sample_count = 0;
			log_cb(RETRO_LOG_DEBUG, LOGPRE "VLM5030 BSY=L\n" );
			pin_BSY = 0;
			VLM5030_phase = PH_IDLE;
		}
		else
		{
			sample_count -= length;
		}
	}
	/* silent buffering */
	while (length > 0)
	{
		buffer[buf_count++] = 0x00;
		length--;
	}
}

/* realtime update */
static void VLM5030_update(void)
{
	stream_update(channel,0);
}

/* setup parameteroption when RST=H */
static void VLM5030_setup_parameter(UINT8 param)
{
	/* latch parameter value */
	VLM5030_parameter = param;

	/* bit 0,1 : 4800bps / 9600bps , interporator step */
	if(param&2) /* bit 1 = 1 , 9600bps */
		interp_step = 4; /* 9600bps : no interporator */
	else if(param&1) /* bit1 = 0 & bit0 = 1 , 4800bps */
		interp_step = 2; /* 4800bps : 2 interporator */
	else	/* bit1 = bit0 = 0 : 2400bps */
		interp_step = 1; /* 2400bps : 4 interporator */

	/* bit 3,4,5 : speed (frame size) */
	VLM5030_frame_size = VLM5030_speed_table[(param>>3) &7];

	/* bit 6,7 : low / high pitch */
	if(param&0x80)	/* bit7=1 , high pitch */
		pitch_offset = -8;
	else if(param&0x40)	/* bit6=1 , low pitch */
		pitch_offset = 8;
	else
		pitch_offset = 0;
}

#ifdef _STATE_H
static void VLM5030_resotore_state(void)
{
	int interp_effect = FR_SIZE - (interp_count%FR_SIZE);
	int i;

	/* restore parameter data */
	VLM5030_setup_parameter(VLM5030_parameter);

	/* restore current energy,pitch & filter */
	current_energy = old_energy + (target_energy - old_energy) * interp_effect / FR_SIZE;
	if (old_pitch > 1)
		current_pitch = old_pitch + (target_pitch - old_pitch) * interp_effect / FR_SIZE;
	for (i = 0; i <= 9 ; i++)
		current_k[i] = old_k[i] + (target_k[i] - old_k[i]) * interp_effect / FR_SIZE;
}
#endif

static void VLM5030_reset(void)
{
	VLM5030_phase = PH_RESET;
	VLM5030_address = 0;
	vcu_addr_h = 0;
	pin_BSY = 0;

	old_energy = old_pitch = 0;
	new_energy = new_pitch = 0;
	current_energy = current_pitch = 0;
	target_energy = target_pitch = 0;
	memset(old_k, 0, sizeof(old_k));
	memset(new_k, 0, sizeof(new_k));
	memset(current_k, 0, sizeof(current_k));
	memset(target_k, 0, sizeof(target_k));
	interp_count = sample_count = pitch_count = 0;
	memset(x, 0, sizeof(x));
	/* reset parameters */
	VLM5030_setup_parameter(0x00);
}

/* set speech rom address */
void VLM5030_set_rom(void *speech_rom)
{
	VLM5030_rom = (UINT8 *)speech_rom;
}

/* get BSY pin level */
int VLM5030_BSY(void)
{
	VLM5030_update();
	return pin_BSY;
}

/* latch contoll data */
WRITE_HANDLER( VLM5030_data_w )
{
	latch_data = (UINT8)data;
}

/* set RST pin level : reset / set table address A8-A15 */
void VLM5030_RST (int pin )
{
	if( pin_RST )
	{
		if( !pin )
		{	/* H -> L : latch parameters */
			pin_RST = 0;
			VLM5030_setup_parameter(latch_data);
		}
	}
	else
	{
		if( pin )
		{	/* L -> H : reset chip */
			pin_RST = 1;
			if( pin_BSY )
			{
				VLM5030_reset();
			}
		}
	}
}

/* set VCU pin level : ?? unknown */
void VLM5030_VCU(int pin)
{
	/* direct mode / indirect mode */
	pin_VCU = pin;
	return;
}

/* set ST pin level  : set table address A0-A7 / start speech */
void VLM5030_ST(int pin )
{
	int table;

	if( pin_ST != pin )
	{
		/* pin level is change */
		if( !pin )
		{	/* H -> L */
			pin_ST = 0;

			if( pin_VCU )
			{	/* direct access mode & address High */
				vcu_addr_h = ((int)latch_data<<8) + 0x01;
			}
			else
			{
				/* start speech */
				if (Machine->sample_rate == 0)
				{
					pin_BSY = 0;
					return;
				}
				/* check access mode */
				if( vcu_addr_h )
				{	/* direct access mode */
					VLM5030_address = (vcu_addr_h&0xff00) + latch_data;
					vcu_addr_h = 0;
				}
				else
				{	/* indirect accedd mode */
					table = (latch_data&0xfe) + (((int)latch_data&1)<<8);
					VLM5030_address = (((int)VLM5030_rom[table&VLM5030_address_mask])<<8)
					                |        VLM5030_rom[(table+1)&VLM5030_address_mask];
        /* show unsupported parameter message */
        if( interp_step != 1)
          log_cb(RETRO_LOG_DEBUG, LOGPRE "No %d %dBPS parameter",table/2,interp_step*2400);
				}
				VLM5030_update();
				/* log_cb(RETRO_LOG_DEBUG, LOGPRE "VLM5030 %02X start adr=%04X\n",table/2,VLM5030_address ); */
				/* reset process status */
				sample_count = VLM5030_frame_size;
				interp_count = FR_SIZE;
				/* clear filter */
				/* start after 3 sampling cycle */
				VLM5030_phase = PH_RUN;
			}
		}
		else
		{	/* L -> H */
			pin_ST = 1;
			/* setup speech , BSY on after 30ms? */
			VLM5030_phase = PH_SETUP;
			sample_count = 1; /* wait time for busy on */
			pin_BSY = 1; /* */
		}
	}
}

/* start VLM5030 with sound rom              */
/* speech_rom == 0 -> use sampling data mode */
int VLM5030_sh_start(const struct MachineSound *msound)
{
	int emulation_rate;

	intf = msound->sound_interface;

	emulation_rate = intf->baseclock / 440;

	/* reset input pins */
	pin_RST = pin_ST = pin_VCU= 0;
	latch_data = 0;

	VLM5030_reset();
	VLM5030_phase = PH_IDLE;

	VLM5030_rom = memory_region(intf->memory_region);
	/* memory size */
	if( intf->memory_size == 0)
		VLM5030_address_mask = memory_region_length(intf->memory_region)-1;
	else
		VLM5030_address_mask = intf->memory_size-1;

	channel = stream_init(VLM_NAME,intf->volume,emulation_rate,0,vlm5030_update_callback);
	if (channel == -1) return 1;

	schannel = mixer_allocate_channel(intf->volume);

#ifdef _STATE_H
	/* don't restore "UINT8 *VLM5030_rom" when use VLM5030_set_rom() */

	state_save_register_UINT16 (VLM_NAME,0,"address", &VLM5030_address, 1);
	state_save_register_UINT8  (VLM_NAME,0,"busy"   , &pin_BSY        , 1);
	state_save_register_UINT8  (VLM_NAME,0,"start"  , &pin_ST         , 1);
	state_save_register_UINT8  (VLM_NAME,0,"vcu"    , &pin_VCU        , 1);
	state_save_register_UINT8  (VLM_NAME,0,"reset"  , &pin_RST        , 1);
	state_save_register_UINT8  (VLM_NAME,0,"data"   , &latch_data     , 1);
	state_save_register_UINT16 (VLM_NAME,0,"vcu_addr", &vcu_addr_h    , 1);
	state_save_register_UINT8  (VLM_NAME,0,"parameter", &VLM5030_parameter, 1);
	state_save_register_UINT8  (VLM_NAME,0,"phase"   , &VLM5030_phase , 1);
	state_save_register_UINT8  (VLM_NAME,0,"interporator"  , &interp_count , 1);
	state_save_register_UINT8  (VLM_NAME,0,"sample count"  , &sample_count , 1);
	state_save_register_UINT8  (VLM_NAME,0,"pitch count"   , &pitch_count , 1);
	state_save_register_UINT16 (VLM_NAME,0,"old energy"    , &old_energy  , 1);
	state_save_register_UINT8  (VLM_NAME,0,"old pitch"     , &old_pitch   , 1);
	state_save_register_INT16  (VLM_NAME,0,"old K"         , old_k        ,10);
	state_save_register_UINT16 (VLM_NAME,0,"tartget energy", &target_energy, 1);
	state_save_register_UINT8  (VLM_NAME,0,"tartget pitch" , &target_pitch , 1);
	state_save_register_INT16  (VLM_NAME,0,"tartget K"     , target_k      ,10);
	state_save_register_INT32  (VLM_NAME,0,"x"             , x             ,10);
	state_save_register_func_postload(VLM5030_resotore_state);
#endif
	return 0;
}

/* update VLM5030 */
void VLM5030_sh_update( void )
{
	VLM5030_update();
}

/* stop VLM5030 */
void VLM5030_sh_stop( void )
{
}
