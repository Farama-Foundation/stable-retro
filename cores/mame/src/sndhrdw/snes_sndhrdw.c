/***************************************************************************

  snes.c

  File to handle the sound emulation of the Nintendo Super NES.

  Anthony Kruize
  Based on the original MESS driver by Lee Hammerton (aka Savoury Snax)

  Just a shell for the moment.

***************************************************************************/
#include "driver.h"
#include "includes/snes.h"

#define IPLROM_SIZE 64

static struct
{
	INT8  vol_left;
	INT8  vol_right;
	INT8  evol_left;
	INT8  evol_right;
	UINT8 key_on;
	UINT8 key_off;
	UINT8 flags;
	UINT8 end_block;
	UINT8 pitch_mod;
	UINT8 noise_on;
	UINT16 dir_offset;
	UINT8 echo_on;
	UINT8 echo_fback;
	UINT8 echo_offset;
	UINT8 echo_delay;
	UINT8 echo_coeff[8];
	struct
	{
		INT8   vol_left;
		INT8   vol_right;
		UINT16 pitch;
		UINT16 source_no;
		UINT8  adsr1;
		UINT8  adsr2;
		UINT8  gain;
		UINT8  envx;
		UINT8  outx;
		UINT8  key;
		INT16  sample[65536];
		INT16  prevnyb[2];
		UINT8  loop;
		UINT16 length;
		UINT16 pos;
	} voice[8];
} snes_dsp;

static struct
{
	UINT8 enabled;
	UINT16 counter;
	void *timer;
} timers[3];
static int channel;
UINT8 spc_usefakeapu = 0;				/* Fake the APU behaviour. */
static UINT8 spc_showrom = 1;			/* Is the IPL ROM visible or not */
static UINT8 spc_iplrom[IPLROM_SIZE];	/* Storage for the IPL rom */
UINT8 fakeapu_port[4] = { 0xaa, 0xbb, 0x00, 0x00 };

static void snes_spc_timer( int t )
{
	timers[t].counter++;
	if( timers[t].counter >= spc_ram[0xfa + t] ) /* minus =*/
	{
		timers[t].counter = 0;
		spc_ram[0xfd + t]++;
		if( spc_ram[0xfd + t] > 0xf )
		{
			spc_ram[0xfd + t] = 0;
		}
	}
}

int snes_sh_start( const struct MachineSound *driver )
{
	UINT8 ii;
	const char *names[2] = { "SNES", "SNES" };
	const int volume[2] = { MIXER( 50, MIXER_PAN_LEFT ), MIXER( 50, MIXER_PAN_RIGHT ) };

	/* If sound is disabled then we'd better use the SPC skipper */
	if( Machine->sample_rate == 0 )
	{
		spc_usefakeapu = 1;
		return 0;
	}

	/* Copy the IPL ROM into storage */
	spc_ram = (UINT8 *)memory_region( REGION_CPU2 );
	memcpy( spc_iplrom, &spc_ram[0xffc0], sizeof(spc_iplrom) );

	/* Sort out the ports */
	for( ii = 0; ii < 4; ii++ )
	{
		spc_port_in[ii] = 0;
		spc_port_out[ii] = 0;
	}

	/* Init DSP */
	for( ii = 0; ii < 8; ii++ )
	{
		snes_dsp.voice[ii].key = 0;
		snes_dsp.voice[ii].length = 0;
		snes_dsp.voice[ii].vol_left = 0;
		snes_dsp.voice[ii].vol_right = 0;
		snes_dsp.voice[ii].pitch = 0;
		snes_dsp.voice[ii].source_no = 0;
		snes_dsp.voice[ii].outx = 0;
		snes_dsp.voice[ii].loop = 0;
		snes_dsp.voice[ii].pos = 0;
	}

	channel = stream_init_multi( 2, names, volume, Machine->sample_rate, 0, snes_sh_update );

	/* Initialize the timers */
	timers[0].timer = timer_alloc( snes_spc_timer );
	timer_adjust( timers[0].timer, TIME_IN_USEC(125), 0, TIME_IN_USEC(125) );
	timer_enable( timers[0].timer, 0 );
	timers[1].timer = timer_alloc( snes_spc_timer );
	timer_adjust( timers[1].timer, TIME_IN_USEC(125), 1, TIME_IN_USEC(125) );
	timer_enable( timers[1].timer, 0 );
	timers[2].timer = timer_alloc( snes_spc_timer );
	timer_adjust( timers[2].timer, TIME_IN_USEC(15.6), 2, TIME_IN_USEC(15.6) );
	timer_enable( timers[2].timer, 0 );

	return 0;
}

void snes_sh_update( int param, INT16 **buffer, int length )
{
	INT16 left, right;

	while( length-- > 0 )
	{
		/* no sound for now */
		left = 0;
		right = 0;

		/* Update the buffers */
		*(buffer[0]++) = left;
		*(buffer[1]++) = right;
	}
}

/***************************
 *       I/O for DSP       *
 ***************************/

static void snes_dsp_decode_sample( UINT8 chnl )
	{
	/* FIXME: Need to fill this in! */
	}

READ_HANDLER( snes_dsp_io_r )
{
	/* FIXME: Need to fill this in! */
	return 0xff;
}

WRITE_HANDLER( snes_dsp_io_w )
{
	switch( offset )
	{
		case DSP_V0_VOLL:	/* Voice volume (left) */
		case DSP_V1_VOLL:
		case DSP_V2_VOLL:
		case DSP_V3_VOLL:
		case DSP_V4_VOLL:
		case DSP_V5_VOLL:
		case DSP_V6_VOLL:
		case DSP_V7_VOLL:
			snes_dsp.voice[(offset & 0xf0) >> 4].vol_left = data;
			break;
		case DSP_V0_VOLR:	/* Voice volume (right) */
		case DSP_V1_VOLR:
		case DSP_V2_VOLR:
		case DSP_V3_VOLR:
		case DSP_V4_VOLR:
		case DSP_V5_VOLR:
		case DSP_V6_VOLR:
		case DSP_V7_VOLR:
			snes_dsp.voice[(offset & 0xf0) >> 4].vol_right = data;
			break;
		case DSP_V0_PITCHL:	/* Voice pitch (low) */
		case DSP_V1_PITCHL:
		case DSP_V2_PITCHL:
		case DSP_V3_PITCHL:
		case DSP_V4_PITCHL:
		case DSP_V5_PITCHL:
		case DSP_V6_PITCHL:
		case DSP_V7_PITCHL:
			snes_dsp.voice[(offset & 0xf0) >> 4].pitch = (snes_dsp.voice[(offset & 0xf0) >> 4].pitch & 0xff00) | data;
			break;
		case DSP_V0_PITCHH:	/* Voice pitch (high) */
		case DSP_V1_PITCHH:
		case DSP_V2_PITCHH:
		case DSP_V3_PITCHH:
		case DSP_V4_PITCHH:
		case DSP_V5_PITCHH:
		case DSP_V6_PITCHH:
		case DSP_V7_PITCHH:
			snes_dsp.voice[(offset & 0xf0) >> 4].pitch = (snes_dsp.voice[(offset & 0xf0) >> 4].pitch & 0xff) | ((data & 0x3f) << 8);
			break;
		case DSP_V0_SRCN:	/* Voice source number */
		case DSP_V1_SRCN:
		case DSP_V2_SRCN:
		case DSP_V3_SRCN:
		case DSP_V4_SRCN:
		case DSP_V5_SRCN:
		case DSP_V6_SRCN:
		case DSP_V7_SRCN:
			snes_dsp.voice[(offset & 0xf0) >> 4].source_no = data * 4;
			break;
		case DSP_V0_ADSR1:	/* Voice ADSR 1 */
		case DSP_V1_ADSR1:
		case DSP_V2_ADSR1:
		case DSP_V3_ADSR1:
		case DSP_V4_ADSR1:
		case DSP_V5_ADSR1:
		case DSP_V6_ADSR1:
		case DSP_V7_ADSR1:
			snes_dsp.voice[(offset & 0xf0) >> 4].adsr1 = data;
			break;
		case DSP_V0_ADSR2:	/* Voice ADSR 2 */
		case DSP_V1_ADSR2:
		case DSP_V2_ADSR2:
		case DSP_V3_ADSR2:
		case DSP_V4_ADSR2:
		case DSP_V5_ADSR2:
		case DSP_V6_ADSR2:
		case DSP_V7_ADSR2:
			snes_dsp.voice[(offset & 0xf0) >> 4].adsr2 = data;
			break;
		case DSP_V0_GAIN:	/* Voice envelope gain */
		case DSP_V1_GAIN:
		case DSP_V2_GAIN:
		case DSP_V3_GAIN:
		case DSP_V4_GAIN:
		case DSP_V5_GAIN:
		case DSP_V6_GAIN:
		case DSP_V7_GAIN:
			snes_dsp.voice[(offset & 0xf0) >> 4].gain = data;
			break;
		case DSP_V0_ENVX:	/* Current envelope value */
		case DSP_V1_ENVX:
		case DSP_V2_ENVX:
		case DSP_V3_ENVX:
		case DSP_V4_ENVX:
		case DSP_V5_ENVX:
		case DSP_V6_ENVX:
		case DSP_V7_ENVX:
			snes_dsp.voice[(offset & 0xf0) >> 4].envx = data;
			break;
		case DSP_V0_OUTX:	/* Wave height value */
		case DSP_V1_OUTX:
		case DSP_V2_OUTX:
		case DSP_V3_OUTX:
		case DSP_V4_OUTX:
		case DSP_V5_OUTX:
		case DSP_V6_OUTX:
		case DSP_V7_OUTX:
			snes_dsp.voice[(offset & 0xf0) >> 4].outx = data;
			break;
		case DSP_MVOLL:		/* Master volume (left) */
			snes_dsp.vol_left = data;
			break;
		case DSP_MVOLR:		/* Master volume (right) */
			snes_dsp.vol_right = data;
			break;
		case DSP_EVOLL:		/* Echo volume (left) */
			snes_dsp.evol_left = data;
			break;
		case DSP_EVOLR:		/* Echo volume (right) */
			snes_dsp.evol_right = data;
			break;
		case DSP_KON:		/* Key On */
			snes_dsp.key_on = data;
			if( data & 0x1 )
			{
				snes_dsp.voice[0].key = 1;
				snes_dsp_decode_sample( 0 );
			}
			if( data & 0x2 )
			{
				snes_dsp.voice[1].key = 1;
				snes_dsp_decode_sample( 1 );
			}
			if( data & 0x4 )
			{
				snes_dsp.voice[2].key = 1;
				snes_dsp_decode_sample( 2 );
			}
			if( data & 0x8 )
			{
				snes_dsp.voice[3].key = 1;
				snes_dsp_decode_sample( 3 );
			}
			if( data & 0x10 )
			{
				snes_dsp.voice[4].key = 1;
				snes_dsp_decode_sample( 4 );
			}
			if( data & 0x20 )
			{
				snes_dsp.voice[5].key = 1;
				snes_dsp_decode_sample( 5 );
			}
			if( data & 0x40 )
			{
				snes_dsp.voice[6].key = 1;
				snes_dsp_decode_sample( 6 );
			}
			if( data & 0x80 )
			{
				snes_dsp.voice[7].key = 1;
				snes_dsp_decode_sample( 7 );
			}
			break;
		case DSP_KOF:		/* Key Off */
			snes_dsp.key_off = data;
			if( data & 0x1 ) snes_dsp.voice[0].key = 0;
			if( data & 0x2 ) snes_dsp.voice[1].key = 0;
			if( data & 0x4 ) snes_dsp.voice[2].key = 0;
			if( data & 0x8 ) snes_dsp.voice[3].key = 0;
			if( data & 0x10 ) snes_dsp.voice[4].key = 0;
			if( data & 0x20 ) snes_dsp.voice[5].key = 0;
			if( data & 0x40 ) snes_dsp.voice[6].key = 0;
			if( data & 0x80 ) snes_dsp.voice[7].key = 0;
			break;
		case DSP_FLG:		/* Flags (reset,mute,ecen,nck) */
			snes_dsp.flags = data;
			break;
		case DSP_EFB:		/* Echo feedback */
			snes_dsp.echo_fback = data;
			break;
		case DSP_PMOD:		/* Pitch modulation */
			snes_dsp.pitch_mod = data;
			break;
		case DSP_NON:		/* Noise on/off */
			snes_dsp.noise_on = data;
			break;
		case DSP_EON:		/* Echo on/off */
			snes_dsp.echo_on = data;
			break;
		case DSP_DIR:		/* Directory offset address */
			snes_dsp.dir_offset = data * 0x100;
			break;
		case DSP_ESA:		/* Echo offset address */
			snes_dsp.echo_offset = data;
			break;
		case DSP_EDL:		/* Echo delay */
			snes_dsp.echo_delay = data & 0xf;
			break;

		default:
			log_cb(RETRO_LOG_DEBUG, LOGPRE  "DSP: write to unknown offset %X. Data = %X\n", offset, data );
			break;
	}
}

/***************************
 *     I/O for SPC700      *
 ***************************/
READ_HANDLER( spc_io_r )
{
	switch( offset )	/* Offset is from 0x00f0 */
	{
		case 0x2:		/* Register address */
			return spc_ram[0xf2];
		case 0x3:		/* Register data */
			return snes_dsp_io_r( spc_ram[0xf2] );
		case 0x4:		/* Port 0 */
		case 0x5:		/* Port 1 */
		case 0x6:		/* Port 2 */
		case 0x7:		/* Port 3 */
			return spc_port_in[offset - 4];
		case 0xA:		/* Timer 0 */
		case 0xB:		/* Timer 1 */
		case 0xC:		/* Timer 2 */
			break;
		case 0xD:		/* Counter 0 */
		case 0xE:		/* Counter 1 */
		case 0xF:		/* Counter 2 */
		{
			UINT8 value = spc_ram[0xf0 + offset] & 0xf;
			spc_ram[0xf0 + offset] = 0;
			return value;
	}
	}
	return 0xff;
}

WRITE_HANDLER( spc_io_w )
{
	switch( offset )	/* Offset is from 0x00f0 */
	{
		case 0x1:		/* Control */
			if( data & 0x1 && !timers[0].enabled )
			{
				timers[0].counter = 0;
				spc_ram[0xfd] = 0;
			}
			if( data & 0x2 && !timers[1].enabled )
			{
				timers[1].counter = 0;
				spc_ram[0xfe] = 0;
			}
			if( data & 0x4 && !timers[2].enabled )
			{
				timers[2].counter = 0;
				spc_ram[0xff] = 0;
			}
			timers[0].enabled = data & 0x1;
			timer_enable( timers[0].timer, timers[0].enabled );
			timers[1].enabled = (data & 0x2) >> 1;
			timer_enable( timers[1].timer, timers[1].enabled );
			timers[2].enabled = (data & 0x4) >> 2;
			timer_enable( timers[2].timer, timers[2].enabled );
			if( data & 0x10 )
			{
				spc_port_in[0] = spc_port_out[0] = 0;
				spc_port_in[1] = spc_port_out[1] = 0;
			}
			if( data & 0x20 )
			{
				spc_port_in[2] = spc_port_out[2] = 0;
				spc_port_in[3] = spc_port_out[3] = 0;
			}
			spc_showrom = (data & 0x80) >> 7;
			break;
		case 0x2:		/* Register address */
			break;
		case 0x3:		/* Register data */
			snes_dsp_io_w( spc_ram[0xf2], data );
			break;
		case 0x4:		/* Port 0 */
		case 0x5:		/* Port 1 */
		case 0x6:		/* Port 2 */
		case 0x7:		/* Port 3 */
			spc_port_out[offset - 4] = data;
			break;
		case 0xA:		/* Timer 0 */
		case 0xB:		/* Timer 1 */
		case 0xC:		/* Timer 2 */
		case 0xD:		/* Counter 0 */
		case 0xE:		/* Counter 1 */
		case 0xF:		/* Counter 2 */
			return;
	}
			spc_ram[0xf0 + offset] = data;
}

READ_HANDLER( spc_bank_r )
{
	if( spc_showrom )
	{
		return spc_iplrom[offset];
	}
	else
	{
		return spc_ram[0xffc0 + offset];
	}
}

WRITE_HANDLER( spc_bank_w )
{
	spc_ram[0xffc0 + offset] = data;
}

/*******************************************************************
 *                       I/O for Fake APU                          *
 *                                                                 *
 * When sound is disabled the SPC700 is stopped so we need to      *
 * simulate the behaviour of the ROM in the SPC700 as best we can. *
 *******************************************************************/
WRITE_HANDLER( fakespc_port_w )
{
	if( offset == 0 )
	{
		fakeapu_port[2]++;
		fakeapu_port[3]++;
	}

	fakeapu_port[offset] = data;
}

READ_HANDLER( fakespc_port_r )
{
/*  G65816_PC=1, G65816_S, G65816_P, G65816_A, G65816_X, G65816_Y,
 *  G65816_PB, G65816_DB, G65816_D, G65816_E,
 *  G65816_NMI_STATE, G65816_IRQ_STATE
 */

	static UINT8 portcount[2] = {0,0};
	UINT8 retVal = 0;

	switch( offset )
	{
		case 0:
		case 1:
		{
			switch( portcount[0] )
			{
				case 0:
					retVal = fakeapu_port[offset];
					break;
				case 1:
					retVal = activecpu_get_reg(4) & 0xFF;
					break;
				case 2:
					retVal = (activecpu_get_reg(4) >> 8) & 0xFF;
					break;
				case 3:
					retVal = activecpu_get_reg(5) & 0xFF;
					break;
				case 4:
					retVal = (activecpu_get_reg(5) >> 8) & 0xFF;
					break;
				case 5:
					retVal = activecpu_get_reg(6) & 0xFF;
					break;
				case 6:
					retVal = (activecpu_get_reg(6) >> 8) & 0xFF;
					break;
				case 7:
					retVal = 0xAA;
					break;
				case 8:
					retVal = 0xBB;
					break;
				case 9:
				case 10:
					retVal = rand() & 0xFF;
					break;
			}
			portcount[0]++;
			if( portcount[0] > 10 )
				portcount[0] = 0;
			return retVal;
		} break;
		case 2:
		case 3:
		{
			switch( portcount[1] )
			{
				case 0:
					retVal = fakeapu_port[offset];
					break;
				case 1:
					retVal = activecpu_get_reg(4) & 0xFF;
					break;
				case 2:
					retVal = (activecpu_get_reg(4) >> 8) & 0xFF;
					break;
				case 3:
					retVal = activecpu_get_reg(5) & 0xFF;
					break;
				case 4:
					retVal = (activecpu_get_reg(5) >> 8) & 0xFF;
					break;
				case 5:
					retVal = activecpu_get_reg(5) & 0xFF;
					break;
				case 6:
					retVal = (activecpu_get_reg(5) >> 8) & 0xFF;
					break;
				case 7:
				case 8:
					retVal = rand() & 0xFF;
					break;
			}
			portcount[1]++;
			if( portcount[1] > 8 )
				portcount[1] = 0;
			return retVal;
		} break;
	}

	return fakeapu_port[offset];
}
