// license:BSD-3-Clause
// copyright-holders:R. Belmont

/*
C140.c

Simulator based on AMUSE sources.
The C140 sound chip is used by Namco System 2 and System 21
The 219 ASIC (which incorporates a modified C140) is used by Namco NA-1 and NA-2
This chip controls 24 channels (C140) or 16 (219) of PCM.
16 bytes are associated with each channel.
Channels can be 8 bit signed PCM, or 12 bit signed PCM.

Timer behavior is not yet handled.

Unmapped registers:
    0x1f8:timer interval?   (Nx0.1 ms)
    0x1fa:irq ack? timer restart?
    0x1fe:timer switch?(0:off 1:on)

--------------

    ASIC "219" notes

    On the 219 ASIC used on NA-1 and NA-2, the high registers have the following
    meaning instead:
    0x1f7: bank for voices 0-3
    0x1f1: bank for voices 4-7
    0x1f3: bank for voices 8-11
    0x1f5: bank for voices 12-15

    Some games (bkrtmaq, xday2) write to 0x1fd for voices 12-15 instead.  Probably the bank registers
    mirror at 1f8, in which case 1ff is also 0-3, 1f9 is also 4-7, 1fb is also 8-11, and 1fd is also 12-15.

    Each bank is 0x20000 (128k), and the voice addresses on the 219 are all multiplied by 2.
    Additionally, the 219's base pitch is the same as the C352's (42667).  But these changes
    are IMO not sufficient to make this a separate file - all the other registers are
    fully compatible.

    Finally, the 219 only has 16 voices.
*/
/*
    2000.06.26  CAB     fixed compressed pcm playback
    2002.07.20  R.Belmont   added support for multiple banking types
    2006.01.08  R.Belmont   added support for NA-1/2 "219" derivative
*/


#include "burnint.h"
#include "c140.h"

// --- Future NOTE: if asic219 DOES NOT WORK, this is why!! (line below) -dink
#define BYTE_XOR_BE(x) (x^1)

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

static INT32 m_sample_rate;
static INT32 m_banking_type;

/* internal buffers */
static INT16 *m_mixer_buffer_left;
static INT16 *m_mixer_buffer_right;

static INT32 m_baserate;
static INT8 *m_pRom;
static UINT8 m_REG[0x200];

static INT16 m_pcmtbl[8];        //2000.06.26 CAB

static C140_VOICE m_voi[C140_MAX_VOICE];

#include "stream.h"
static Stream stream;

void c140_update_INT(INT16 **streams, INT32 samples_len); //forward

//**************************************************************************
//  LIVE DEVICE
//**************************************************************************

static void init_voice( C140_VOICE *v )
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

/*
   find_sample: compute the actual address of a sample given it's
   address and banking registers, as well as the board type.

   I suspect in "real life" this works like the Sega MultiPCM where the banking
   is done by a small PAL or GAL external to the sound chip, which can be switched
   per-game or at least per-PCB revision as addressing range needs grow.
*/
static long find_sample(long adrs, long bank, INT32 voice)
{
	long newadr = 0;

	static const INT16 asic219banks[4] = { 0x1f7, 0x1f1, 0x1f3, 0x1f5 };

	adrs=(bank<<16)+adrs;

	switch (m_banking_type)
	{
		case C140_TYPE_SYSTEM2:
			// System 2 banking
			newadr = ((adrs&0x200000)>>2)|(adrs&0x7ffff);
			break;

		case C140_TYPE_SYSTEM21:
			// System 21 banking.
			// similar to System 2's.
			newadr = ((adrs&0x300000)>>1)+(adrs&0x7ffff);
			break;

		case C140_TYPE_ASIC219:
			// ASIC219's banking is fairly simple
			newadr = ((m_REG[asic219banks[voice/4]]&0x3) * 0x20000) + adrs;
			break;
	}

	return (newadr);
}


//-------------------------------------------------
//  device_start - device-specific startup
//-------------------------------------------------

void c140_init(INT32 clock, INT32 devtype, UINT8 *c140_rom)
{
	m_sample_rate = m_baserate = clock;

	m_banking_type = devtype;

	m_pRom = (INT8 *)c140_rom;

	/* make decompress pcm table */     //2000.06.26 CAB
	{
		memset(m_pcmtbl, 0, sizeof(m_pcmtbl));

		INT32 segbase=0;
		for(INT32 i = 0; i < 8; i++)
		{
			m_pcmtbl[i] = segbase;    //segment base value
			segbase += 16<<i;
		}
	}

	c140_reset();

	/* allocate a pair of buffers to mix into - 1 second's worth should be more than enough */
	m_mixer_buffer_left = (INT16*)BurnMalloc(2 * sizeof(INT16) * m_sample_rate);
	m_mixer_buffer_right = m_mixer_buffer_left + m_sample_rate;
	memset(m_mixer_buffer_left, 0, 2 * sizeof(INT16) * m_sample_rate);

	// init stream/resampler
	stream.init(m_sample_rate, nBurnSoundRate, 2, 1, c140_update_INT);
    stream.set_volume(1.00);

}

void c140_set_sync(INT32 (*pCPUCyclesCB)(), INT32 nCPUMhz)
{
	stream.set_buffered(pCPUCyclesCB, nCPUMhz);
}

void c140_exit()
{
	if (m_mixer_buffer_left) {
		BurnFree(m_mixer_buffer_left);
		m_mixer_buffer_left = m_mixer_buffer_right = NULL;
	}
	stream.exit();
}

void c140_reset()
{
	memset(m_REG, 0, sizeof(m_REG));

	for (INT32 i = 0; i < C140_MAX_VOICE; i++) {
		init_voice(&m_voi[i]);
	}
}

void c140_scan(INT32 nAction, INT32 *)
{
	SCAN_VAR(m_REG);
	SCAN_VAR(m_voi);

	if (nAction & ACB_WRITE && ~nAction & ACB_RUNAHEAD) {
		memset(m_mixer_buffer_left, 0, 2 * sizeof(INT16) * m_sample_rate);
	}
}

static inline int limit(INT32 in)
{
	if(in>0x7fff)       return 0x7fff;
	else if(in<-0x8000) return -0x8000;
	return in;
}

//-------------------------------------------------
//  sound_stream_update - handle a stream update
//-------------------------------------------------

void c140_update_INT(INT16 **streams, INT32 samples_len)
{
	INT32   rvol,lvol;
	INT32   dt;
	INT32   sdt;
	INT32   st,ed,sz;

	INT8    *pSampleData;
	INT32   frequency,delta,offset,pos;
	INT32   cnt, voicecnt;
	INT32   lastdt,prevdt,dltdt;
	float   pbase=(float)m_baserate*2.0 / (float)m_sample_rate;

	INT16   *lmix, *rmix;

	INT32 nSamplesNeeded = samples_len;

	lmix = m_mixer_buffer_left;
	rmix = m_mixer_buffer_right;

	/* zap the contents of the mixer buffer */
	memset(lmix, 0, nSamplesNeeded * sizeof(INT16));
	memset(rmix, 0, nSamplesNeeded * sizeof(INT16));

	/* get the number of voices to update */
	voicecnt = (m_banking_type == C140_TYPE_ASIC219) ? 16 : 24;

	//--- audio update
	for (INT32 i = 0; i < voicecnt; i++)
	{
		C140_VOICE *v = &m_voi[i];
		const struct voice_registers *vreg = (struct voice_registers *)&m_REG[i*16];

		if( v->key )
		{
			frequency = vreg->frequency_msb*256 + vreg->frequency_lsb;

			/* Abort voice if no frequency value set */
			if(frequency==0) continue;

			/* Delta =  frequency * ((8MHz/374)*2 / sample rate) */
			delta=(long)((float)frequency * pbase);

			/* Calculate left/right channel volumes */
			lvol=(vreg->volume_left*32)/C140_MAX_VOICE; //32ch -> 24ch
			rvol=(vreg->volume_right*32)/C140_MAX_VOICE;

			/* Set mixer outputs base pointers */
			lmix = m_mixer_buffer_left;
			rmix = m_mixer_buffer_right;

			/* Retrieve sample start/end and calculate size */
			st=v->sample_start;
			ed=v->sample_end;
			sz=ed-st;

			/* Retrieve base pointer to the sample data */
			pSampleData = m_pRom + find_sample(st, v->bank, i);

			/* Fetch back previous data pointers */
			offset=v->ptoffset;
			pos=v->pos;
			lastdt=v->lastdt;
			prevdt=v->prevdt;
			dltdt=v->dltdt;

			/* Switch on data type - compressed PCM is only for C140 */
			if ((v->mode&8) && (m_banking_type != C140_TYPE_ASIC219))
			{
				//compressed PCM (maybe correct...)
				/* Loop for enough to fill sample buffer as requested */
				for(INT32 j=0;j<(nSamplesNeeded);j++)
				{
					offset += delta;
					cnt = (offset>>16)&0x7fff;
					offset &= 0xffff;
					pos+=cnt;
					//for(;cnt>0;cnt--)
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

						/* decompress to 13bit range */     //2000.06.26 CAB
						sdt=dt>>3;              //signed
						if(sdt<0)   sdt = (sdt<<(dt&7)) - m_pcmtbl[dt&7];
						else        sdt = (sdt<<(dt&7)) + m_pcmtbl[dt&7];

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
				for(INT32 j=0;j<(nSamplesNeeded);j++)
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

						if (m_banking_type == C140_TYPE_ASIC219)
						{
							lastdt = pSampleData[BYTE_XOR_BE(pos)];

							// Sign + magnitude format
							if ((v->mode & 0x01) && (lastdt & 0x80))
								lastdt = -(lastdt & 0x7f);

							// Sign flip
							if (v->mode & 0x40)
								lastdt = -lastdt;
						}
						else
						{
							lastdt=pSampleData[pos];
						}

						dltdt = (lastdt - prevdt);
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
	lmix = m_mixer_buffer_left;
	rmix = m_mixer_buffer_right;
	{
		INT16 *dest1 = streams[0];
		INT16 *dest2 = streams[1];
		for (INT32 i = 0; i < samples_len; i++)
		{
			INT32 val;

			val = 8 * (*lmix++);
			*dest1++ = limit(val);
			val = 8 * (*rmix++);
			*dest2++ = limit(val);
		}
	}
}

void c140_update(INT16 *output, INT32 samples_len)
{
	if (samples_len != nBurnSoundLen) {
		bprintf(0, _T("c140_update(): once per frame, please!\n"));
		return;
	}

	stream.render(output, samples_len);
}


UINT8 c140_read(UINT16 offset)
{
	stream.update();
	offset &= 0x1ff;
	if ((offset & 0xf) == 0x5 && offset < ((m_banking_type == C140_TYPE_ASIC219) ? 0x100 : 0x180)) {
		C140_VOICE const &v = m_voi[offset >> 4];
		return (v.key ? 0x40 : 0x00) | (m_REG[offset] & 0x3f);
	}
	return m_REG[offset];
}


void c140_write(UINT16 offset, UINT8 data)
{
	stream.update();
	offset &= 0x1ff;

	// mirror the bank registers on the 219, fixes bkrtmaq (and probably xday2 based on notes in the HLE)
	if ((offset >= 0x1f8) && (m_banking_type == C140_TYPE_ASIC219))
	{
		offset -= 8;
	}

	m_REG[offset]=data;

	if (offset < 0x180)
	{
		C140_VOICE *v = &m_voi[offset>>4];

		if ((offset&0xf) == 0x5)
		{
			if (data&0x80)
			{
				const struct voice_registers *vreg = (struct voice_registers *) &m_REG[offset&0x1f0];
				v->key=1;
				v->ptoffset=0;
				v->pos=0;
				v->lastdt=0;
				v->prevdt=0;
				v->dltdt=0;
				v->bank = vreg->bank;
				v->mode = data;

				// on the 219 asic, addresses are in words
				if (m_banking_type == C140_TYPE_ASIC219)
				{
					v->sample_loop = (vreg->loop_msb*256 + vreg->loop_lsb)*2;
					v->sample_start = (vreg->start_msb*256 + vreg->start_lsb)*2;
					v->sample_end = (vreg->end_msb*256 + vreg->end_lsb)*2;

					#if 0
					logerror("219: play v %d mode %02x start %x loop %x end %x\n",
						offset>>4, v->mode,
						find_sample(v->sample_start, v->bank, offset>>4),
						find_sample(v->sample_loop, v->bank, offset>>4),
						find_sample(v->sample_end, v->bank, offset>>4));
					#endif
				}
				else
				{
					v->sample_loop = vreg->loop_msb*256 + vreg->loop_lsb;
					v->sample_start = vreg->start_msb*256 + vreg->start_lsb;
					v->sample_end = vreg->end_msb*256 + vreg->end_lsb;
				}
			}
			else
			{
				v->key=0;
			}
		}
	}
}


void c140_set_base(void *base)
{
	m_pRom = (INT8 *)base;
}

