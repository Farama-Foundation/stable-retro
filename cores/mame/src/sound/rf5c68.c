/*********************************************************/
/*    ricoh RF5C68(or clone) PCM controller              */
/*********************************************************/

#include "driver.h"
#include <math.h>

#define PCM_NORMALIZE

enum
{
	RF_L_PAN = 0, RF_R_PAN = 1, RF_LR_PAN = 2
};

static RF5C68PCM    rpcm;
static int  reg_port;
static int emulation_rate;

static int buffer_len;
static int stream;

/*static unsigned char pcmbuf[0x10000]; */
static unsigned char *pcmbuf=NULL;

static struct RF5C68interface *intf;

static unsigned char wreg[0x10]; /* write data */

#define   BASE_SHIFT    (11+4)

#define    RF_ON     (1<<0)
#define    RF_START  (1<<1)


static void RF5C68Update( int num, INT16 **buffer, int length );

/************************************************/
/*    RF5C68 start                              */
/************************************************/
int RF5C68_sh_start( const struct MachineSound *msound )
{
	int i;
	int rate = Machine->sample_rate;
	struct RF5C68interface *inintf = msound->sound_interface;

	if (Machine->sample_rate == 0) return 0;

	if(pcmbuf==NULL) pcmbuf=(unsigned char *)malloc(0x10000);
	if(pcmbuf==NULL) return 1;

	memset(pcmbuf, 0xff, 0x10000);

	intf = inintf;
	buffer_len = rate / Machine->drv->frames_per_second;
	emulation_rate = buffer_len * Machine->drv->frames_per_second;

	rpcm.clock = intf->clock;
	for( i = 0; i < RF5C68_PCM_MAX; i++ )
	{
		rpcm.env[i] = 0;
		rpcm.pan[i] = 0;
		rpcm.start[i] = 0;
		rpcm.step[i] = 0;
		rpcm.flag[i] = 0;
	}
	for( i = 0; i < 0x10; i++ )  wreg[i] = 0;
	reg_port = 0;

	{
		char buf[2][40];
		const char *name[2];
		int  vol[2];
		name[0] = buf[0];
		name[1] = buf[1];
		sprintf( buf[0], "%s Left", sound_name(msound) );
		sprintf( buf[1], "%s Right", sound_name(msound) );
		vol[0] = (MIXER_PAN_LEFT<<8)  | (intf->volume&0xff);
		vol[1] = (MIXER_PAN_RIGHT<<8) | (intf->volume&0xff);

		stream = stream_init_multi( RF_LR_PAN, name, vol, rate, 0, RF5C68Update );
		if(stream == -1) return 1;
	}
	return 0;
}

/************************************************/
/*    RF5C68 stop                               */
/************************************************/
void RF5C68_sh_stop( void )
{
	if(pcmbuf!=NULL) free(pcmbuf);
	pcmbuf=NULL;
}

/************************************************/
/*    RF5C68 update                             */
/************************************************/

static INLINE int ILimit(int v, int max, int min) { return v > max ? max : (v < min ? min : v); }

static void RF5C68Update( int num, INT16 **buffer, int length )
{
	int i, j, tmp;
	unsigned int addr, old_addr;
	signed int ld, rd;
	INT16  *datap[2];

	datap[RF_L_PAN] = buffer[0];
	datap[RF_R_PAN] = buffer[1];

	memset( datap[RF_L_PAN], 0x00, length * sizeof(INT16) );
	memset( datap[RF_R_PAN], 0x00, length * sizeof(INT16) );

	for( i = 0; i < RF5C68_PCM_MAX; i++ )
	{
		if( (rpcm.flag[i]&(RF_START|RF_ON)) == (RF_START|RF_ON) )
		{
			/**** PCM setup ****/
			addr = (rpcm.addr[i]>>BASE_SHIFT)&0xffff;
			ld = (rpcm.pan[i]&0x0f);
			rd = ((rpcm.pan[i]>>4)&0x0f);
			/*       cen = (ld + rd) / 4; */
			/*       ld = (rpcm.env[i]&0xff) * (ld + cen); */
			/*       rd = (rpcm.env[i]&0xff) * (rd + cen); */
			ld = (rpcm.env[i]&0xff) * ld;
			rd = (rpcm.env[i]&0xff) * rd;

			for( j = 0; j < length; j++ )
			{
				old_addr = addr;
				addr = (rpcm.addr[i]>>BASE_SHIFT)&0xffff;
				for(; old_addr <= addr; old_addr++ )
				{
					/**** PCM end check ****/
					if( (((unsigned int)pcmbuf[old_addr])&0x00ff) == (unsigned int)0x00ff )
					{
						rpcm.addr[i] = rpcm.loop[i] + ((addr - old_addr)<<BASE_SHIFT);
						addr = (rpcm.addr[i]>>BASE_SHIFT)&0xffff;
						/**** PCM loop check ****/
						if( (((unsigned int)pcmbuf[addr])&0x00ff) == (unsigned int)0x00ff ) /* this causes looping problems */
						{
							rpcm.flag[i] = 0;
							break;
						}
						else
						{
							old_addr = addr;
						}
					}
#ifdef PCM_NORMALIZE
					tmp = rpcm.pcmd[i];
					rpcm.pcmd[i] = (pcmbuf[old_addr]&0x7f) * (-1 + (2 * ((pcmbuf[old_addr]>>7)&0x01)));
					rpcm.pcma[i] = (tmp - rpcm.pcmd[i]) / 2;
					rpcm.pcmd[i] += rpcm.pcma[i];
#endif
				}
				rpcm.addr[i] += rpcm.step[i];
				if( !rpcm.flag[i] )  break; /* skip PCM */
#ifndef PCM_NORMALIZE
				if( pcmbuf[addr]&0x80 )
				{
					rpcm.pcmx[0][i] = ((signed int)(pcmbuf[addr]&0x7f))*ld;
					rpcm.pcmx[1][i] = ((signed int)(pcmbuf[addr]&0x7f))*rd;
				}
				else
				{
					rpcm.pcmx[0][i] = ((signed int)(-(pcmbuf[addr]&0x7f)))*ld;
					rpcm.pcmx[1][i] = ((signed int)(-(pcmbuf[addr]&0x7f)))*rd;
				}
#else
				rpcm.pcmx[0][i] = rpcm.pcmd[i] * ld;
				rpcm.pcmx[1][i] = rpcm.pcmd[i] * rd;
#endif
				*(datap[RF_L_PAN] + j) = ILimit( ((int)*(datap[RF_L_PAN] + j) + ((rpcm.pcmx[0][i])>>4)), 32767, -32768 );
				*(datap[RF_R_PAN] + j) = ILimit( ((int)*(datap[RF_R_PAN] + j) + ((rpcm.pcmx[1][i])>>4)), 32767, -32768 );
			}
		}
	}
}

/************************************************/
/*    RF5C68 write register                     */
/************************************************/
WRITE_HANDLER( RF5C68_reg_w )
{
	int  i;
	int  val;

	wreg[offset] = data;			/* stock write data */
	/**** set PCM registers ****/

	switch( offset )
	{
		case 0x00:
			rpcm.env[reg_port] = data;		/* set env. */
			break;
		case 0x01:
			rpcm.pan[reg_port] = data;		/* set pan */
			break;
		case 0x02:
		case 0x03:
			/**** address step ****/
			val = (((((int)wreg[0x03])<<8)&0xff00) | (((int)wreg[0x02])&0x00ff));
			rpcm.step[reg_port] = (int)(
			(
			( 28456.0 / (float)emulation_rate ) *
			( val / (float)(0x0800) ) *
			(rpcm.clock / 8000000.0) *
			(1<<BASE_SHIFT)
			)
			);
			break;
		case 0x04:
		case 0x05:
			/**** loop address ****/
			rpcm.loop[reg_port] = (((((unsigned int)wreg[0x05])<<8)&0xff00)|(((unsigned int)wreg[0x04])&0x00ff))<<(BASE_SHIFT);
			break;
		case 0x06:
			/**** start address ****/
			rpcm.start[reg_port] = (((unsigned int)wreg[0x06])&0x00ff)<<(BASE_SHIFT + 8);
			rpcm.addr[reg_port] = rpcm.start[reg_port];
			break;
		case 0x07:
			reg_port = wreg[0x07]&0x07;
			if( data&0x80 )
			{
				rpcm.pcmx[0][reg_port] = 0;
				rpcm.pcmx[1][reg_port] = 0;
				rpcm.flag[reg_port] |= RF_START;
			}
			break;

		case 0x08:
			/**** pcm on/off ****/
			for( i = 0; i < RF5C68_PCM_MAX; i++ )
			{
				if( !(data&(1<<i)) )
				{
					rpcm.flag[i] |= RF_ON;	/* PCM on */
				}
				else
				{
					rpcm.flag[i] &= ~(RF_ON); /* PCM off */
				}
			}
			break;
	}
}

/************************************************/
/*    RF5C68 read memory                        */
/************************************************/
READ_HANDLER( RF5C68_r )
{
	unsigned int  bank;
	bank = wreg[0x7] & 0x40 ? rpcm.start[reg_port] >> BASE_SHIFT : (wreg[0x7] & 0xf) << 12;
	return pcmbuf[bank + offset];
}
/************************************************/
/*    RF5C68 write memory                       */
/************************************************/
WRITE_HANDLER( RF5C68_w )
{
	unsigned int  bank;
	bank = wreg[0x7] & 0x40 ? rpcm.start[reg_port] >> BASE_SHIFT : (wreg[0x7] & 0xf) << 12;
	pcmbuf[bank + offset] = data;
}


/**************** end of file ****************/
