/***************************************************************************

							-= Seta Hardware =-

					driver by	Luca Elia (l.elia@tin.it)

					rewrite by Manbow-J(manbowj@hamal.freemail.ne.jp)

X1-010 (Seta Custom Sound Chip):

	The X1-010 is 16 Voices sound generator, each channel gets it's
	waveform from RAM (128 bytes per waveform, 8 bit unsigned data)
	or sampling PCM(8bit unsigned data).

Registers:
	8 registers per channel (mapped to the lower bytes of 16 words on the 68K)

	Reg:	Bits:		Meaning:

	0		7654 3---
			---- -2--	PCM/Waveform repeat flag (0:Ones 1:Repeat) (*1)
			---- --1-	Sound out select (0:PCM 1:Waveform)
			---- ---0	Key on / off

	1		7654 ----	PCM Volume 1 (L?)
			---- 3210	PCM Volume 2 (R?)
						Waveform No.

	2					PCM Frequency
						Waveform Pitch Lo

	3					Waveform Pitch Hi

	4					PCM Sample Start / 0x1000 			[Start/End in bytes]
						Waveform Envelope Time

	5					PCM Sample End 0x100 - (Sample End / 0x1000)	[PCM ROM is Max 1MB?]
						Waveform Envelope No.
	6					Reserved
	7					Reserved

	offset 0x0000 - 0x0fff	Wave form data
	offset 0x1000 - 0x1fff	Envelope data

	*1 : when 0 is specified, hardware interrupt is caused(allways return soon)

Hardcoded Values:

	PCM ROM region:		REGION_SOUND1

***************************************************************************/

#include "driver.h"
#include "seta.h"



#define LOG_SOUND 0

#define SETA_NUM_CHANNELS 16


#define	LOG_REGISTER_WRITE	0
#define	LOG_REGISTER_READ	0

#define FREQ_BASE_BITS		  8					/* Frequency fixed decimal shift bits*/
#define ENV_BASE_BITS		 16					/* wave form envelope fixed decimal shift bits*/
#define	VOL_BASE	(2*32*256/30)					/* Volume base*/

/* this structure defines the parameters for a channel */
typedef struct {
	unsigned char	status;
	unsigned char	volume;						/*        volume / wave form no.*/
	unsigned char	frequency;					/*     frequency / pitch lo*/
	unsigned char	pitch_hi;					/*      reserved / pitch hi*/
	unsigned char	start;						/* start address / envelope time*/
	unsigned char	end;						/*   end address / envelope no.*/
	unsigned char	reserve[2];
} X1_010_CHANNEL;

/* Variables only used here */
static int	rate;								/* Output sampling rate (Hz)*/
static int	stream;								/* Stream handle*/
static int	address;							/* address eor data*/
static int	sound_enable = 0;					/* sound output enable/disable*/
static UINT8	x1_010_reg[0x2000];				/* X1-010 Register & wave form area*/
static UINT8	HI_WORD_BUF[0x2000];			/* X1-010 16bit access ram check avoidance work*/
static UINT32	smp_offset[SETA_NUM_CHANNELS];
static UINT32	env_offset[SETA_NUM_CHANNELS];

static UINT32 base_clock;

/* mixer tables and internal buffers */
/*static short	*mixer_buffer = NULL;*/


/*--------------------------------------------------------------
 generate sound to the mix buffer
--------------------------------------------------------------*/
void seta_sh_update( int param, INT16 **buffer, int length )
{
	X1_010_CHANNEL	*reg;
	int		ch, i, volL, volR, freq;
	register INT8	*start, *end, data;
	register UINT8	*env;
	register UINT32	smp_offs, smp_step, env_offs, env_step, delta;

	/* mixer buffer zero clear*/
	memset( buffer[0], 0, length*sizeof(short) );
	memset( buffer[1], 0, length*sizeof(short) );

/*	if( sound_enable == 0 ) return;*/

	for( ch = 0; ch < SETA_NUM_CHANNELS; ch++ ) {
		reg = (X1_010_CHANNEL *)&(x1_010_reg[ch*sizeof(X1_010_CHANNEL)]);
		if( (reg->status&1) != 0 ) {							/* Key On*/
			INT16 *bufL = buffer[0];
			INT16 *bufR = buffer[1];
			if( (reg->status&2) == 0 ) {						/* PCM sampling*/
				start    = (INT8 *)(reg->start      *0x1000+memory_region(REGION_SOUND1));
				end      = (INT8 *)((0x100-reg->end)*0x1000+memory_region(REGION_SOUND1));
				volL     = ((reg->volume>>4)&0xf)*VOL_BASE;
				volR     = ((reg->volume>>0)&0xf)*VOL_BASE;
				smp_offs = smp_offset[ch];
				freq     = reg->frequency&0x1f;
				/* Meta Fox does not write the frequency register. Ever*/
				if( freq == 0 ) freq = 4;
				smp_step = (UINT32)((float)base_clock/8192.0
							*freq*(1<<FREQ_BASE_BITS)/(float)rate);
#if LOG_SOUND
				if( smp_offs == 0 ) {
					logerror( "Play sample %06X - %06X, channel %X volume %d freq %X step %X offset %X\n",
						start, end, ch, vol, freq, smp_step, smp_offs );
				}
#endif
				for( i = 0; i < length; i++ ) {
					delta = smp_offs>>FREQ_BASE_BITS;
					/* sample ended?*/
					if( start+delta >= end ) {
						reg->status &= 0xfe;					/* Key off*/
						break;
					}
					data = *(start+delta);
					*bufL++ += (data*volL/256);
					*bufR++ += (data*volR/256);
					smp_offs += smp_step;
				}
				smp_offset[ch] = smp_offs;
			} else {											/* Wave form*/
				start    = (INT8 *)&(x1_010_reg[reg->volume*128+0x1000]);
				smp_offs = smp_offset[ch];
				freq     = (reg->pitch_hi<<8)+reg->frequency;
				smp_step = (UINT32)((float)base_clock/128.0/1024.0/4.0*freq*(1<<FREQ_BASE_BITS)/(float)rate);

				env      = (UINT8 *)&(x1_010_reg[reg->end*128]);
				env_offs = env_offset[ch];
				env_step = (UINT32)((float)base_clock/128.0/1024.0/4.0*reg->start*(1<<ENV_BASE_BITS)/(float)rate);
#if LOG_SOUND
/* Print some more debug info */
				if( smp_offs == 0 ) {
					logerror( "Play waveform %X, channel %X volume %X freq %4X step %X offset %X\n",
						reg->volume, ch, reg->end, freq, smp_step, smp_offs );
				}
#endif
				for( i = 0; i < length; i++ ) {
					int vol;
					delta = env_offs>>ENV_BASE_BITS;
	 				/* Envelope one shot mode*/
					if( (reg->status&4) != 0 && delta >= 0x80 ) {
						reg->status &= 0xfe;					/* Key off*/
						break;
					}
					vol = *(env+(delta&0x7f));
					volL = ((vol>>4)&0xf)*VOL_BASE;
					volR = ((vol>>0)&0xf)*VOL_BASE;
					data  = *(start+((smp_offs>>FREQ_BASE_BITS)&0x7f));
					*bufL++ += (data*volL/256);
					*bufR++ += (data*volR/256);
					smp_offs += smp_step;
					env_offs += env_step;
				}
				smp_offset[ch] = smp_offs;
				env_offset[ch] = env_offs;
			}
		}
	}
}



int seta_sh_start( const struct MachineSound *msound )
{
	int i,j;
	struct x1_010_interface *intf = (struct x1_010_interface*)msound->sound_interface;
	char buf[2][40];
	const char *name[2];
	int mixed_vol,vol[2];

	base_clock	= intf->clock;
	rate		= Machine->sample_rate;
	address		= intf->adr;

	for( i = 0; i < SETA_NUM_CHANNELS; i++ ) {
		smp_offset[i] = 0;
		env_offset[i] = 0;
	}
	/* Print some more debug info */
	/*log_cb(RETRO_LOG_DEBUG, LOGPRE "masterclock = %d rate = %d\n", master_clock, rate );*/

	/* get stream channels */
	mixed_vol = intf->volume;
	/* stream setup */
	for (j = 0 ; j < 2 ; j++)
	{
		name[j]=buf[j];
		vol[j] = mixed_vol & 0xffff;
		mixed_vol>>=16;
		sprintf(buf[j],"%s Ch%d",sound_name(msound),j+1);
	}
	stream = stream_init_multi(2,name,vol,rate,0,seta_sh_update);

	return 0;
}


void seta_sh_stop( void )
{
}


void seta_sound_enable_w(int data)
{
	sound_enable = data;
}



/* Use these for 8 bit CPUs */


READ_HANDLER( seta_sound_r )
{
	offset ^= address;
	return x1_010_reg[offset];
}




WRITE_HANDLER( seta_sound_w )
{
	int channel, reg;
	offset ^= address;

	channel	= offset/sizeof(X1_010_CHANNEL);
	reg		= offset%sizeof(X1_010_CHANNEL);

	if( channel < SETA_NUM_CHANNELS && reg == 0
	 && (x1_010_reg[offset]&1) == 0 && (data&1) != 0 ) {
	 	smp_offset[channel] = 0;
	 	env_offset[channel] = 0;
	}
	log_cb(RETRO_LOG_DEBUG, LOGPRE "PC: %06X : offset %6X : data %2X\n", activecpu_get_pc(), offset, data );
	x1_010_reg[offset] = data;
}




/* Use these for 16 bit CPUs */

READ16_HANDLER( seta_sound_word_r )
{
	UINT16	ret;

	ret = HI_WORD_BUF[offset]<<8;
	ret += (seta_sound_r( offset )&0xff);
	log_cb(RETRO_LOG_DEBUG, LOGPRE  "Read X1-010 PC:%06X Offset:%04X Data:%04X\n", activecpu_get_pc(), offset, ret );
	return ret;
}

WRITE16_HANDLER( seta_sound_word_w )
{
	HI_WORD_BUF[offset] = (data>>8)&0xff;
	seta_sound_w( offset, data&0xff );
	log_cb(RETRO_LOG_DEBUG, LOGPRE  "Write X1-010 PC:%06X Offset:%04X Data:%04X\n", activecpu_get_pc(), offset, data );
}
