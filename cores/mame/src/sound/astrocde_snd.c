/***********************************************************

     Astrocade custom 'IO' chip sound chip driver
	 Frank Palazzolo

	 Portions copied from the Pokey emulator by Ron Fries

	 First Release:
	 	09/20/98

	 Issues:
	 	Noise generators need work
		Can do lots of speed optimizations

***********************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "driver.h"
#include "cpu/z80/z80.h"


static const struct astrocade_interface *intf;

static int emulation_rate;
static int div_by_N_factor;
static int buffer_len;

static INT16 *astrocade_buffer[MAX_ASTROCADE_CHIPS];

static int sample_pos[MAX_ASTROCADE_CHIPS];

static int current_count_A[MAX_ASTROCADE_CHIPS];
static int current_count_B[MAX_ASTROCADE_CHIPS];
static int current_count_C[MAX_ASTROCADE_CHIPS];
static int current_count_V[MAX_ASTROCADE_CHIPS];
static int current_count_N[MAX_ASTROCADE_CHIPS];

static int current_state_A[MAX_ASTROCADE_CHIPS];
static int current_state_B[MAX_ASTROCADE_CHIPS];
static int current_state_C[MAX_ASTROCADE_CHIPS];
static int current_state_V[MAX_ASTROCADE_CHIPS];

static int current_size_A[MAX_ASTROCADE_CHIPS];
static int current_size_B[MAX_ASTROCADE_CHIPS];
static int current_size_C[MAX_ASTROCADE_CHIPS];
static int current_size_V[MAX_ASTROCADE_CHIPS];
static int current_size_N[MAX_ASTROCADE_CHIPS];

static int channel;

/* Registers */

static int master_osc[MAX_ASTROCADE_CHIPS];
static int freq_A[MAX_ASTROCADE_CHIPS];
static int freq_B[MAX_ASTROCADE_CHIPS];
static int freq_C[MAX_ASTROCADE_CHIPS];
static int vol_A[MAX_ASTROCADE_CHIPS];
static int vol_B[MAX_ASTROCADE_CHIPS];
static int vol_C[MAX_ASTROCADE_CHIPS];
static int vibrato[MAX_ASTROCADE_CHIPS];
static int vibrato_speed[MAX_ASTROCADE_CHIPS];
static int mux[MAX_ASTROCADE_CHIPS];
static int noise_am[MAX_ASTROCADE_CHIPS];
static int vol_noise4[MAX_ASTROCADE_CHIPS];
static int vol_noise8[MAX_ASTROCADE_CHIPS];

static int randbyte = 0;
static int randbit = 1;

static void astrocade_update(int num, int newpos)
{
	INT16 *buffer = astrocade_buffer[num];

	int pos = sample_pos[num];
	int i, data, data16, noise_plus_osc, vib_plus_osc;

	for(i=pos; i<newpos; i++)
	{
		if (current_count_N[i] == 0)
		{
			randbyte = rand() & 0xff;
		}

		current_size_V[num] = 32768*vibrato_speed[num]/div_by_N_factor;

		if (!mux[num])
		{
			if (current_state_V[num] == -1)
				vib_plus_osc = (master_osc[num]-vibrato[num])&0xff;
			else
				vib_plus_osc = master_osc[num];
			current_size_A[num] = vib_plus_osc*freq_A[num]/div_by_N_factor;
			current_size_B[num] = vib_plus_osc*freq_B[num]/div_by_N_factor;
			current_size_C[num] = vib_plus_osc*freq_C[num]/div_by_N_factor;
		}
		else
		{
			noise_plus_osc = ((master_osc[num]-(vol_noise8[num]&randbyte)))&0xff;
			current_size_A[num] = noise_plus_osc*freq_A[num]/div_by_N_factor;
			current_size_B[num] = noise_plus_osc*freq_B[num]/div_by_N_factor;
			current_size_C[num] = noise_plus_osc*freq_C[num]/div_by_N_factor;
			current_size_N[num] = 2*noise_plus_osc/div_by_N_factor;
		}

		data = (current_state_A[num]*vol_A[num] +
			    current_state_B[num]*vol_B[num] +
			    current_state_C[num]*vol_C[num]);

		if (noise_am[num])
		{
			randbit = rand() & 1;
			data = data + randbit*vol_noise4[num];
		}

		data16 = data<<8;
		buffer[pos++] = data16;

		if (current_count_A[num] >= current_size_A[num])
		{
			current_state_A[num] = -current_state_A[num];
			current_count_A[num] = 0;
		}
		else
			current_count_A[num]++;

		if (current_count_B[num] >= current_size_B[num])
		{
			current_state_B[num] = -current_state_B[num];
			current_count_B[num] = 0;
		}
		else
			current_count_B[num]++;

		if (current_count_C[num] >= current_size_C[num])
		{
			current_state_C[num] = -current_state_C[num];
			current_count_C[num] = 0;
		}
		else
			current_count_C[num]++;

		if (current_count_V[num] >= current_size_V[num])
		{
			current_state_V[num] = -current_state_V[num];
			current_count_V[num] = 0;
		}
		else
			current_count_V[num]++;

		if (current_count_N[num] >= current_size_N[num])
		{
			current_count_N[num] = 0;
		}
		else
			current_count_N[num]++;
	}
	sample_pos[num]    = pos;
}

int astrocade_sh_start(const struct MachineSound *msound)
{
	int i;

	intf = msound->sound_interface;

	if (Machine->sample_rate == 0)
	{
		return 0;
	}

	buffer_len = Machine->sample_rate / Machine->drv->frames_per_second;

	emulation_rate = buffer_len * Machine->drv->frames_per_second;
	div_by_N_factor = intf->baseclock/emulation_rate;

	channel = mixer_allocate_channels(intf->num,intf->volume);
	/* reserve buffer */
	for (i = 0;i < intf->num;i++)
	{
		if ((astrocade_buffer[i] = malloc(sizeof(INT16)*buffer_len)) == 0)
		{
			while (--i >= 0) free(astrocade_buffer[i]);
			return 1;
		}
		/* reset state */
		sample_pos[i] = 0;
		current_count_A[i] = 0;
		current_count_B[i] = 0;
		current_count_C[i] = 0;
		current_count_V[i] = 0;
		current_count_N[i] = 0;
		current_state_A[i] = 1;
		current_state_B[i] = 1;
		current_state_C[i] = 1;
		current_state_V[i] = 1;
	}

	return 0;
}

void astrocade_sh_stop(void)
{
	int i;

	for (i = 0;i < intf->num;i++){
		free(astrocade_buffer[i]);
	}
}

void astrocade_sound_w(int num, int offset, int data)
{
	int i, bvalue, temp_vib;

	/* update */
	astrocade_update(num,sound_scalebufferpos(buffer_len));

	switch(offset)
	{
		case 0:  /* Master Oscillator */
			log_cb(RETRO_LOG_DEBUG, LOGPRE "Master Osc Write: %02x\n",data);
      master_osc[num] = data+1;
		break;

		case 1:  /* Tone A Frequency */
			log_cb(RETRO_LOG_DEBUG, LOGPRE "Tone A Write:        %02x\n",data);
			freq_A[num] = data+1;
		break;

		case 2:  /* Tone B Frequency */
			log_cb(RETRO_LOG_DEBUG, LOGPRE "Tone B Write:           %02x\n",data);
			freq_B[num] = data+1;
		break;

		case 3:  /* Tone C Frequency */
			log_cb(RETRO_LOG_DEBUG, LOGPRE "Tone C Write:              %02x\n",data);
			freq_C[num] = data+1;
		break;

		case 4:  /* Vibrato Register */
			log_cb(RETRO_LOG_DEBUG, LOGPRE "Vibrato Depth:                %02x\n",data&0x3f);
			log_cb(RETRO_LOG_DEBUG, LOGPRE "Vibrato Speed:                %02x\n",data>>6);
			vibrato[num] = data & 0x3f;

			temp_vib = (data>>6) & 0x03;
			vibrato_speed[num] = 1;
			for(i=0;i<temp_vib;i++)
				vibrato_speed[num] <<= 1;

		break;

		case 5:  /* Tone C Volume, Noise Modulation Control */
			vol_C[num] = data & 0x0f;
			mux[num] = (data>>4) & 0x01;
			noise_am[num] = (data>>5) & 0x01;
			log_cb(RETRO_LOG_DEBUG, LOGPRE "Tone C Vol:                      %02x\n",vol_C[num]);
			log_cb(RETRO_LOG_DEBUG, LOGPRE "Mux Source:                      %02x\n",mux[num]);
			log_cb(RETRO_LOG_DEBUG, LOGPRE "Noise Am:                        %02x\n",noise_am[num]);
		break;

		case 6:  /* Tone A & B Volume */
			vol_B[num] = (data>>4) & 0x0f;
			vol_A[num] = data & 0x0f;
			log_cb(RETRO_LOG_DEBUG, LOGPRE "Tone A Vol:                         %02x\n",vol_A[num]);
			log_cb(RETRO_LOG_DEBUG, LOGPRE "Tone B Vol:                         %02x\n",vol_B[num]);
		break;

		case 7:  /* Noise Volume Register */
			vol_noise8[num] = data;
			vol_noise4[num] = (data>>4) & 0x0f;
			log_cb(RETRO_LOG_DEBUG, LOGPRE "Noise Vol:                             %02x\n",vol_noise8[num]);
			log_cb(RETRO_LOG_DEBUG, LOGPRE "Noise Vol (4):                         %02x\n",vol_noise4[num]);
		break;

		case 8:  /* Sound Block Transfer */

			bvalue = (activecpu_get_reg(Z80_BC) >> 8) & 0x07;

			astrocade_sound_w(num, bvalue, data);

		break;
	}
}

WRITE_HANDLER( astrocade_sound1_w )
{
	astrocade_sound_w(0, offset, data);
}

WRITE_HANDLER( astrocade_sound2_w )
{
	astrocade_sound_w(1, offset, data);
}


void astrocade_sh_update(void)
{
	int num;

	if (Machine->sample_rate == 0 ) return;

	for (num = 0;num < intf->num;num++)
	{
		astrocade_update(num, buffer_len);
		/* reset position , step , count */
		sample_pos[num] = 0;
		/* play sound */
		mixer_play_streamed_sample_16(channel+num,astrocade_buffer[num],2*buffer_len,emulation_rate);
	}
}
