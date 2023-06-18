/*
C140.c

Simulator based on AMUSE sources.
The C140 sound chip is used by Namco SystemII, System21
This chip controls 24 channels of PCM.
16 bytes are associated with each channel.
Channels can be 8 bit signed PCM, or 12 bit signed PCM.

Timer behavior is not yet handled.

Unmapped registers:
	0x1f8:timer interval?	(Nx0.1 ms)
	0x1fa:irq ack? timer restart?
	0x1fe:timer switch?(0:off 1:on)
*/
/*
	2000.06.26	CAB		fixed compressed pcm playback
	2002.07.20	R.Belmont	added support for multiple banking types
*/


#include <math.h>
#include "driver.h"

#define MAX_VOICE 24

struct voice_registers
{
	UINT8 volume_right;
	UINT8 volume_left;
	UINT8 frequency_msb;
	UINT8 frequency_lsb;
	UINT8 bank;
	UINT8 mode;
	UINT8 start_msb;
	UINT8 start_lsb;
	UINT8 end_msb;
	UINT8 end_lsb;
	UINT8 loop_msb;
	UINT8 loop_lsb;
	UINT8 reserved[4];
};

static int sample_rate;
static int stream;
static int banking_type;
/* internal buffers */
static INT16 *mixer_buffer_left;
static INT16 *mixer_buffer_right;

static int baserate;
static void *pRom;
static UINT8 REG[0x200];

static INT16 pcmtbl[8];		/*2000.06.26 CAB*/

typedef struct
{
	long	ptoffset;
	long	pos;
	long	key;
	/*--work*/
	long	lastdt;
	long	prevdt;
	long	dltdt;
	/*--reg*/
	long	rvol;
	long	lvol;
	long	frequency;
	long	bank;
	long	mode;

	long	sample_start;
	long	sample_end;
	long	sample_loop;
} VOICE;

VOICE voi[MAX_VOICE];

static void init_voice( VOICE *v )
{
	v->key=0;
	v->ptoffset=0;
	v->rvol=0;
	v->lvol=0;
	v->frequency=0;
	v->bank=0;
	v->mode=0;
	v->sample_start=0;
	v->sample_end=0;
	v->sample_loop=0;
}

READ_HANDLER( C140_r )
{
	offset&=0x1ff;
	return REG[offset];
}

/*
   find_sample: compute the actual address of a sample given it's
   address and banking registers, as well as the board type.

   I suspect in "real life" this works like the Sega MultiPCM where the banking 
   is done by a small PAL or GAL external to the sound chip, which can be switched
   per-game or at least per-PCB revision as addressing range needs grow.
 */
static long find_sample( long adrs, long bank)
{
	long newadr = 0;

	adrs=(bank<<16)+adrs;

	switch (banking_type)
	{
		case C140_TYPE_SYSTEM2:
			/* System 2 banking*/
			newadr = ((adrs&0x200000)>>2)|(adrs&0x7ffff);
			break;

		case C140_TYPE_SYSTEM21_A:
			/* System 21 type A (simple) banking.*/
			/* similar to System 2's.*/
			newadr = ((adrs&0x300000)>>1)+(adrs&0x7ffff);
			break;

		case C140_TYPE_SYSTEM21_B:
			/* System 21 type B (chip select) banking*/

			/* get base address of sample inside the bank*/
			newadr = ((adrs&0x100000)>>2) + (adrs&0x3ffff); 

			/* now add the starting bank offsets based on the 2 */
			/* chip select bits.*/
			/* 0x40000 picks individual 512k ROMs*/
			if (adrs & 0x40000)
			{
				newadr += 0x80000;
			}
			
			/* and 0x200000 which group of chips...*/
			if (adrs & 0x200000)
			{
				newadr += 0x100000;
			}
			break;
	}

	return (newadr);
}
WRITE_HANDLER( C140_w )
{
	stream_update(stream, 0);

	offset&=0x1ff;

	REG[offset]=data;
	if( offset<0x180 )
	{
		VOICE *v = &voi[offset>>4];
		if( (offset&0xf)==0x5 )
		{
			if( data&0x80 )
			{
				const struct voice_registers *vreg = (struct voice_registers *) &REG[offset&0x1f0];
				v->key=1;
				v->ptoffset=0;
				v->pos=0;
				v->lastdt=0;
				v->prevdt=0;
				v->dltdt=0;
				v->bank = vreg->bank;
				v->mode = data;
				v->sample_loop = vreg->loop_msb*256 + vreg->loop_lsb;
				v->sample_start = vreg->start_msb*256 + vreg->start_lsb;
				v->sample_end = vreg->end_msb*256 + vreg->end_lsb;
			}
			else
			{
				v->key=0;
			}
		}
	}
}

static INLINE int limit(INT32 in)
{
	if(in>0x7fff)		return 0x7fff;
	else if(in<-0x8000)	return -0x8000;
	return in;
}

static void update_stereo(int ch, INT16 **buffer, int length)
{
	int		i,j;

	INT32	rvol,lvol;
	INT32	dt;
	INT32	sdt;
	INT32	st,ed,sz;

	INT8	*pSampleData;
	INT32	frequency,delta,offset,pos;
	INT32	cnt;
	INT32	lastdt,prevdt,dltdt;
	float	pbase=(float)baserate*2.0 / (float)sample_rate;

	INT16	*lmix, *rmix;

	if(length>sample_rate) length=sample_rate;

	/* zap the contents of the mixer buffer */
	memset(mixer_buffer_left, 0, length * sizeof(INT16));
	memset(mixer_buffer_right, 0, length * sizeof(INT16));

	/*--- audio update*/
	for( i=0;i<MAX_VOICE;i++ )
	{
		VOICE *v = &voi[i];
		const struct voice_registers *vreg = (struct voice_registers *)&REG[i*16];

		if( v->key )
		{
			frequency= vreg->frequency_msb*256 + vreg->frequency_lsb;

			/* Abort voice if no frequency value set */
			if(frequency==0) continue;

			/* Delta =  frequency * ((8MHz/374)*2 / sample rate) */
			delta=(long)((float)frequency * pbase);

			/* Calculate left/right channel volumes */
			lvol=(vreg->volume_left*32)/MAX_VOICE; /*32ch -> 24ch*/
			rvol=(vreg->volume_right*32)/MAX_VOICE;

			/* Set mixer buffer base pointers */
			lmix = mixer_buffer_left;
			rmix = mixer_buffer_right;

			/* Retrieve sample start/end and calculate size */
			st=v->sample_start;
			ed=v->sample_end;
			sz=ed-st;

			/* Retrieve base pointer to the sample data */
			pSampleData=(signed char*)((unsigned long)pRom + find_sample(st,v->bank));

			/* Fetch back previous data pointers */
			offset=v->ptoffset;
			pos=v->pos;
			lastdt=v->lastdt;
			prevdt=v->prevdt;
			dltdt=v->dltdt;

			/* Switch on data type */
			if(v->mode&8)
			{
				/*compressed PCM (maybe correct...)*/
				/* Loop for enough to fill sample buffer as requested */
				for(j=0;j<length;j++)
				{
					offset += delta;
					cnt = (offset>>16)&0x7fff;
					offset &= 0xffff;
					pos+=cnt;
					/*for(;cnt>0;cnt--)*/
					{
						/* Check for the end of the sample */
						if(pos >= sz)
						{
							/* Check if its a looping sample, either stop or loop */
							if(v->mode&0x10)
							{
								pos = (v->sample_loop - st);
							}
							else
							{
								v->key=0;
								break;
							}
						}

						/* Read the chosen sample byte */
						dt=pSampleData[pos];

						/* decompress to 13bit range */		/*2000.06.26 CAB*/
						sdt=dt>>3;				/*signed*/
						if(sdt<0)	sdt = (sdt<<(dt&7)) - pcmtbl[dt&7];
						else		sdt = (sdt<<(dt&7)) + pcmtbl[dt&7];

						prevdt=lastdt;
						lastdt=sdt;
						dltdt=(lastdt - prevdt);
					}

					/* Caclulate the sample value */
					dt=((dltdt*offset)>>16)+prevdt;

					/* Write the data to the sample buffers */
					*lmix++ +=(dt*lvol)>>(5+5);
					*rmix++ +=(dt*rvol)>>(5+5);
				}
			}
			else
			{
				/* linear 8bit signed PCM */

				for(j=0;j<length;j++)
				{
					offset += delta;
					cnt = (offset>>16)&0x7fff;
					offset &= 0xffff;
					pos += cnt;
					/* Check for the end of the sample */
					if(pos >= sz)
					{
						/* Check if its a looping sample, either stop or loop */
						if( v->mode&0x10 )
						{
							pos = (v->sample_loop - st);
						}
						else
						{
							v->key=0;
							break;
						}
					}

					if( cnt )
					{
						prevdt=lastdt;
						lastdt=pSampleData[pos];
						dltdt=(lastdt - prevdt);
					}

					/* Caclulate the sample value */
					dt=((dltdt*offset)>>16)+prevdt;

					/* Write the data to the sample buffers */
					*lmix++ +=(dt*lvol)>>5;
					*rmix++ +=(dt*rvol)>>5;
				}
			}

			/* Save positional data for next callback */
			v->ptoffset=offset;
			v->pos=pos;
			v->lastdt=lastdt;
			v->prevdt=prevdt;
			v->dltdt=dltdt;
		}
	}

	/* render to MAME's stream buffer */
	lmix = mixer_buffer_left;
	rmix = mixer_buffer_right;
	{
		INT16 *dest1 = buffer[0];
		INT16 *dest2 = buffer[1];
		for (i = 0; i < length; i++)
		{
			*dest1++ = limit(8*(*lmix++));
			*dest2++ = limit(8*(*rmix++));
		}
	}
}

int C140_sh_start( const struct MachineSound *msound )
{
	const struct C140interface *intf = msound->sound_interface;
	int vol[2];
	const char *stereo_names[2] = { "C140 Left", "C140 Right" };

	vol[0] = MIXER(intf->mixing_level,MIXER_PAN_LEFT);
	vol[1] = MIXER(intf->mixing_level,MIXER_PAN_RIGHT);

	sample_rate=baserate=intf->frequency;

	banking_type = intf->banking_type;

	stream = stream_init_multi(2,stereo_names,vol,sample_rate,0,update_stereo);

	pRom=memory_region(intf->region);

	/* make decompress pcm table */		/*2000.06.26 CAB*/
	{
		int i;
		INT32 segbase=0;
		for(i=0;i<8;i++)
		{
			pcmtbl[i]=segbase;	/*segment base value*/
			segbase += 16<<i;
		}
	}

	memset(REG,0,0x200 );
	{
		int i;
		for(i=0;i<MAX_VOICE;i++) init_voice( &voi[i] );
	}

	/* allocate a pair of buffers to mix into - 1 second's worth should be more than enough */
	mixer_buffer_left = malloc(2 * sizeof(INT16)*sample_rate );
	if( mixer_buffer_left )
	{
		mixer_buffer_right = mixer_buffer_left + sample_rate;
		return 0;
	}
	return 1;
}

void C140_sh_stop( void )
{
	free( mixer_buffer_left );
}
