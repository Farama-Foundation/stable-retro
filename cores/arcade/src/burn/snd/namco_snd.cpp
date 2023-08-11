// copyright-holders:Nicola Salmoria,Aaron Giles

#include "burnint.h"
#include "namco_snd.h"

#define MAX_VOICES 8
#define MAX_VOLUME 16
#define INTERNAL_RATE	192000
#define MIXLEVEL	(1 << (16 - 4 - 4))
#define OUTPUT_LEVEL(n)		((n) * MIXLEVEL / chip->num_voices)
#define WAVEFORM_POSITION(n)	(((n) >> chip->f_fracbits) & 0x1f)

static INT32 enable_ram = 0; // allocate RAM?
UINT8* NamcoSoundProm = NULL;

typedef struct
{
	UINT32 frequency;
	UINT32 counter;
	INT32 volume[2];
	INT32 noise_sw;
	INT32 noise_state;
	INT32 noise_seed;
	UINT32 noise_counter;
	INT32 noise_hold;
	INT32 waveform_select;
} sound_channel;

static UINT8 *namco_soundregs = NULL;
static UINT8 *namco_wavedata = NULL;
static UINT8 *namco_waveformdata = NULL;
static INT32 namco_waveformdatasize = 0;

struct namco_sound
{
	sound_channel channel_list[MAX_VOICES];
	sound_channel *last_channel;

	INT32 wave_size;
	INT32 num_voices;
	INT32 sound_enable;
	INT32 namco_clock;
	INT32 sample_rate;
	INT32 f_fracbits;
	INT32 stereo;

	INT16 *waveform[MAX_VOLUME];
	
	double update_step;
	
	double gain[2];
	INT32 output_dir[2];
	INT32 bAdd;
};

static struct namco_sound *chip = NULL;

static void NamcoSoundUpdate_INT(INT16* buffer, INT32 length); // forwards
static void NamcoSoundUpdateStereo_INT(INT16* buffer, INT32 length); // forwards

// for stream-sync
static INT32 namco_buffered = 0;
static INT32 (*pCPUTotalCycles)() = NULL;
static UINT32 nDACCPUMHZ = 0;
static INT32 nPosition;
static INT16 *soundbuf;

// Streambuffer handling
static INT32 SyncInternal()
{
    if (!namco_buffered) return 0;
	return (INT32)(float)(nBurnSoundLen * (pCPUTotalCycles() / (nDACCPUMHZ / (nBurnFPS / 100.0000))));
}

static void UpdateStream(INT32 samples_len)
{
    if (!namco_buffered || !pBurnSoundOut) return;
    if (samples_len > nBurnSoundLen) samples_len = nBurnSoundLen;

	INT32 nSamplesNeeded = samples_len;

    nSamplesNeeded -= nPosition;
    if (nSamplesNeeded <= 0) return;

	INT16 *mix = soundbuf + 5 + (nPosition * 2); // * 2 (stereo stream)

	//bprintf(0, _T("Namco_sync: %d samples    frame %d\n"), nSamplesNeeded, nCurrentFrame);

	if (chip->stereo)
		NamcoSoundUpdateStereo_INT(mix, nSamplesNeeded);
	else
		NamcoSoundUpdate_INT(mix, nSamplesNeeded);

    nPosition += nSamplesNeeded;
}

void NamcoSoundSetStereo(INT32 stereomode)
{
	chip->stereo = stereomode;
}

void NamcoSoundSetBuffered(INT32 (*pCPUCyclesCB)(), INT32 nCpuMHZ)
{
    bprintf(0, _T("*** Using BUFFERED NamcoSnd-mode.\n"));
	nPosition = 0;

    namco_buffered = 1;

    pCPUTotalCycles = pCPUCyclesCB;
    nDACCPUMHZ = nCpuMHZ;
}

void NamcoSoundUpdate(INT16* pSoundBuf, INT32 Length)
{
#if defined FBNEO_DEBUG
	if (!DebugSnd_NamcoSndInitted) bprintf(PRINT_ERROR, _T("NamcoSoundUpdate called without init\n"));
#endif

	if (namco_buffered) {
		if (Length != nBurnSoundLen) {
			bprintf(0, _T("NamcoSoundUpdate() in buffered mode must be called once per frame!\n"));
			return;
		}
	} else {
		nPosition = 0;
	}

	INT16 *mix = soundbuf + 5 + (nPosition * 2);

	if (chip->stereo)
		NamcoSoundUpdateStereo_INT(mix, Length - nPosition); // fill to end
	else
		NamcoSoundUpdate_INT(mix, Length - nPosition); // fill to end

	INT16 *pBuf = soundbuf + 5;

	while (Length > 0) {
		if (chip->bAdd) {
			pSoundBuf[0] = BURN_SND_CLIP(pSoundBuf[0] + pBuf[0]);
			pSoundBuf[1] = BURN_SND_CLIP(pSoundBuf[1] + pBuf[1]);
		} else {
			pSoundBuf[0] = BURN_SND_CLIP(pBuf[0]);
			pSoundBuf[1] = BURN_SND_CLIP(pBuf[1]);
		}

		pBuf += 2;
		pSoundBuf += 2;
		Length--;
	}

	nPosition = 0;
}

static void update_namco_waveform(INT32 offset, UINT8 data)
{
	if (chip->wave_size == 1)
	{
		INT16 wdata;
		INT32 v;

		/* use full byte, first 4 high bits, then low 4 bits */
		for (v = 0; v < MAX_VOLUME; v++)
		{
			wdata = ((data >> 4) & 0x0f) - 8;
			chip->waveform[v][offset * 2] = OUTPUT_LEVEL(wdata * v);
			wdata = (data & 0x0f) - 8;
			chip->waveform[v][offset * 2 + 1] = OUTPUT_LEVEL(wdata * v);
		}
	}
	else
	{
		INT32 v;

		/* use only low 4 bits */
		for (v = 0; v < MAX_VOLUME; v++)
			chip->waveform[v][offset] = OUTPUT_LEVEL(((data & 0x0f) - 8) * v);
	}
}

static inline UINT32 namco_update_one(INT16 *buffer, INT32 length, const INT16 *wave, UINT32 counter, UINT32 freq)
{ // accumulator.
	while (length-- > 0)
	{
		INT32 nLeftSample = 0, nRightSample = 0;

		if ((chip->output_dir[BURN_SND_NAMCOSND_ROUTE_1] & BURN_SND_ROUTE_LEFT) == BURN_SND_ROUTE_LEFT) {
			nLeftSample += (INT32)(wave[WAVEFORM_POSITION(counter)] * chip->gain[BURN_SND_NAMCOSND_ROUTE_1]);
		}
		if ((chip->output_dir[BURN_SND_NAMCOSND_ROUTE_1] & BURN_SND_ROUTE_RIGHT) == BURN_SND_ROUTE_RIGHT) {
			nRightSample += (INT32)(wave[WAVEFORM_POSITION(counter)] * chip->gain[BURN_SND_NAMCOSND_ROUTE_1]);
		}

		nLeftSample = BURN_SND_CLIP(nLeftSample);
		nRightSample = BURN_SND_CLIP(nRightSample);

		*buffer = BURN_SND_CLIP(*buffer + nLeftSample); buffer++;
		*buffer = BURN_SND_CLIP(*buffer + nRightSample); buffer++;

		counter += (UINT32)((double)freq * chip->update_step);
	}

	return counter;
}

static inline UINT32 namco_stereo_update_one(INT16 *buffer, INT32 length, const INT16 *wave, UINT32 counter, UINT32 freq)
{ // stereo accumulator. confused? it does 2 passes of this, one for left, one for right, hence the buffer += 2.
	while (length-- > 0)
	{
		INT32 nSample = 0;

		// no route support here - no games use this currently (just volume/gain)
		nSample = (INT32)(wave[WAVEFORM_POSITION(counter)] * chip->gain[BURN_SND_NAMCOSND_ROUTE_1]);

		*buffer = BURN_SND_CLIP(*buffer + BURN_SND_CLIP(nSample));

		counter += (UINT32)((double)freq * chip->update_step);
		buffer += 2;
	}

	return counter;
}

static void NamcoSoundUpdate_INT(INT16* buffer, INT32 length)
{
	sound_channel *voice;

	memset(buffer, 0, length * 2 * sizeof(INT16));

	/* if no sound, we're done */
	if (chip->sound_enable == 0)
		return;

	/* loop over each voice and add its contribution */
	for (voice = chip->channel_list; voice < chip->last_channel; voice++)
	{
		INT16 *mix = buffer;
		INT32 v = voice->volume[0];

		if (voice->noise_sw)
		{
			INT32 f = voice->frequency & 0xff;

			/* only update if we have non-zero volume and frequency */
			if (v && f)
			{
				INT32 hold_time = 1 << (chip->f_fracbits - 16);
				INT32 hold = voice->noise_hold;
				UINT32 delta = f << 4;
				UINT32 c = voice->noise_counter;
				INT16 noise_data = OUTPUT_LEVEL(0x07 * (v >> 1));
				INT32 i;

				/* add our contribution */
				for (i = 0; i < length; i++)
				{
					INT32 cnt;

					if (voice->noise_state) {
						*mix = BURN_SND_CLIP(*mix + noise_data);
						mix++;
						*mix = BURN_SND_CLIP(*mix + noise_data);
						mix++;
					} else {
						*mix = BURN_SND_CLIP(*mix - noise_data);
						mix++;
						*mix = BURN_SND_CLIP(*mix - noise_data);
						mix++;
					}

					if (hold)
					{
						hold--;
						continue;
					}

					hold = 	hold_time;

					c += delta;
					cnt = (c >> 12);
					c &= (1 << 12) - 1;
					for( ;cnt > 0; cnt--)
					{
						if ((voice->noise_seed + 1) & 2) voice->noise_state ^= 1;
						if (voice->noise_seed & 1) voice->noise_seed ^= 0x28000;
						voice->noise_seed >>= 1;
					}
				}

				/* update the counter and hold time for this voice */
				voice->noise_counter = c;
				voice->noise_hold = hold;
			}
		}
		else
		{
			/* only update if we have non-zero volume and frequency */
			if (v && voice->frequency)
			{
				const INT16 *w = &chip->waveform[v][voice->waveform_select * 32];

				/* generate sound into buffer and update the counter for this voice */
				voice->counter = namco_update_one(mix, length, w, voice->counter, voice->frequency);
			}
		}
	}
}

static void NamcoSoundUpdateStereo_INT(INT16* buffer, INT32 length)
{
	sound_channel *voice;

	memset(buffer, 0, length * 2 * sizeof(INT16));

	/* if no sound, we're done */
	if (chip->sound_enable == 0)
		return;

	/* loop over each voice and add its contribution */
	for (voice = chip->channel_list; voice < chip->last_channel; voice++)
	{
		INT16 *lrmix = buffer;
		INT32 lv = voice->volume[0];
		INT32 rv = voice->volume[1];

		if (voice->noise_sw)
		{
			INT32 f = voice->frequency & 0xff;

			/* only update if we have non-zero volume and frequency */
			if ((lv || rv) && f)
			{
				INT32 hold_time = 1 << (chip->f_fracbits - 16);
				INT32 hold = voice->noise_hold;
				UINT32 delta = f << 4;
				UINT32 c = voice->noise_counter;
				INT16 l_noise_data = OUTPUT_LEVEL(0x07 * (lv >> 1));
				INT16 r_noise_data = OUTPUT_LEVEL(0x07 * (rv >> 1));
				INT32 i;

				/* add our contribution */
				for (i = 0; i < length; i++)
				{
					INT32 cnt;

					if (voice->noise_state)
					{
						*lrmix = BURN_SND_CLIP(*lrmix + l_noise_data);
						lrmix++;
						*lrmix = BURN_SND_CLIP(*lrmix + r_noise_data);
						lrmix++;
					}
					else
					{
						*lrmix = BURN_SND_CLIP(*lrmix - l_noise_data);
						lrmix++;
						*lrmix = BURN_SND_CLIP(*lrmix - r_noise_data);
						lrmix++;
					}

					if (hold)
					{
						hold--;
						continue;
					}

					hold =	hold_time;

					c += delta;
					cnt = (c >> 12);
					c &= (1 << 12) - 1;
					for( ;cnt > 0; cnt--)
					{
						if ((voice->noise_seed + 1) & 2) voice->noise_state ^= 1;
						if (voice->noise_seed & 1) voice->noise_seed ^= 0x28000;
						voice->noise_seed >>= 1;
					}
				}

				/* update the counter and hold time for this voice */
				voice->noise_counter = c;
				voice->noise_hold = hold;
			}
		}
		else
		{
			/* only update if we have non-zero frequency */
			if (voice->frequency)
			{
				/* save the counter for this voice */
				UINT32 c = voice->counter;

				/* only update if we have non-zero left volume */
				if (lv)
				{
					const INT16 *lw = &chip->waveform[lv][voice->waveform_select * 32];

					/* generate sound into the buffer */
					c = namco_stereo_update_one(lrmix + 0, length, lw, voice->counter, voice->frequency);
				}

				/* only update if we have non-zero right volume */
				if (rv)
				{
					const INT16 *rw = &chip->waveform[rv][voice->waveform_select * 32];

					/* generate sound into the buffer */
					c = namco_stereo_update_one(lrmix + 1, length, rw, voice->counter, voice->frequency);
				}

				/* update the counter for this voice */
				voice->counter = c;
			}
		}
	}
}

void NamcoSoundWrite(UINT32 offset, UINT8 data)
{
#if defined FBNEO_DEBUG
	if (!DebugSnd_NamcoSndInitted) bprintf(PRINT_ERROR, _T("NamcoSoundWrite called without init\n"));
#endif

	sound_channel *voice;
	INT32 ch;

	data &= 0x0f;
	if (namco_soundregs[offset] == data)
		return;

	if (namco_buffered) UpdateStream(SyncInternal());

	/* set the register */
	namco_soundregs[offset] = data;

	if (offset < 0x10)
		ch = (offset - 5) / 5;
	else if (offset == 0x10)
		ch = 0;
	else
		ch = (offset - 0x11) / 5;

	if (ch >= chip->num_voices)
		return;

	/* recompute the voice parameters */
	voice = chip->channel_list + ch;
	switch (offset - ch * 5)
	{
	case 0x05:
		voice->waveform_select = data & 7;
		break;

	case 0x10:
	case 0x11:
	case 0x12:
	case 0x13:
	case 0x14:
		/* the frequency has 20 bits */
		/* the first voice has extra frequency bits */
		voice->frequency = (ch == 0) ? namco_soundregs[0x10] : 0;
		voice->frequency += (namco_soundregs[ch * 5 + 0x11] << 4);
		voice->frequency += (namco_soundregs[ch * 5 + 0x12] << 8);
		voice->frequency += (namco_soundregs[ch * 5 + 0x13] << 12);
		voice->frequency += (namco_soundregs[ch * 5 + 0x14] << 16);	/* always 0 */
		break;

	case 0x15:
		voice->volume[0] = data;
		break;
	}
}

static void namcos1_sound_write(INT32 offset, INT32 data)
{
	/* verify the offset */
	if (offset > 63)
	{
	//	logerror("NAMCOS1 sound: Attempting to write past the 64 registers segment\n");
		return;
	}

	if (namco_soundregs[offset] == data)
		return;

	if (namco_buffered) UpdateStream(SyncInternal());

	/* set the register */
	namco_soundregs[offset] = data;

	INT32 ch = offset / 8;
	if (ch >= chip->num_voices)
		return;

	/* recompute the voice parameters */
	sound_channel *voice = chip->channel_list + ch;

	switch (offset - ch * 8)
	{
		case 0x00:
			voice->volume[0] = data & 0x0f;
			break;

		case 0x01:
			voice->waveform_select = (data >> 4) & 15;
		case 0x02:
		case 0x03:
			/* the frequency has 20 bits */
			voice->frequency = (namco_soundregs[ch * 8 + 0x01] & 15) << 16;	/* high bits are from here */
			voice->frequency += namco_soundregs[ch * 8 + 0x02] << 8;
			voice->frequency += namco_soundregs[ch * 8 + 0x03];
			break;

		case 0x04:
			voice->volume[1] = data & 0x0f;

			INT32 nssw = ((data & 0x80) >> 7);
			if (++voice == chip->last_channel)
				voice = chip->channel_list;
			voice->noise_sw = nssw;
			break;
	}
}

void namco_15xx_write(INT32 offset, UINT8 data)
{
	if (offset > 63)
	{
	//	logerror("NAMCO 15XX sound: Attempting to write past the 64 registers segment\n");
		return;
	}

	if (namco_soundregs[offset] == data)
		return;

	if (namco_buffered) UpdateStream(SyncInternal());

	/* set the register */
	namco_soundregs[offset] = data;

	INT32 ch = offset / 8;
	if (ch >= chip->num_voices)
		return;

	/* recompute the voice parameters */
	sound_channel *voice = chip->channel_list + ch;
	switch (offset - ch * 8)
	{
	case 0x03:
		voice->volume[0] = data & 0x0f;
		break;

	case 0x06:
		voice->waveform_select = (data >> 4) & 7;
	case 0x04:
	case 0x05:
		/* the frequency has 20 bits */
		voice->frequency = namco_soundregs[ch * 8 + 0x04];
		voice->frequency += namco_soundregs[ch * 8 + 0x05] << 8;
		voice->frequency += (namco_soundregs[ch * 8 + 0x06] & 15) << 16;    /* high bits are from here */
		break;
	}
}

void namco_15xx_sharedram_write(INT32 offset, UINT8 data)
{
	offset &= 0x3ff;

	if (offset < 0x40) {
		namco_15xx_write(offset, data);
	} else {
		namco_soundregs[offset] = data;
	}
}

UINT8 namco_15xx_sharedram_read(INT32 offset)
{
	return namco_soundregs[offset & 0x3ff];
}

void namco_15xx_sound_enable(INT32 value)
{
	chip->sound_enable = (value) ? 1 : 0;
}

void namcos1_custom30_write(INT32 offset, INT32 data)
{
#if defined FBNEO_DEBUG
	if (!DebugSnd_NamcoSndInitted) bprintf(PRINT_ERROR, _T("namcos1_custom30_write called without init\n"));
#endif

	if (offset < 0x100)
	{
		if (namco_wavedata[offset] != data)
		{
			namco_wavedata[offset] = data;

			/* update the decoded waveform table */
			update_namco_waveform(offset, data);
		}
	}
	else if (offset < 0x140) {
		namco_wavedata[offset] = data;
		namcos1_sound_write(offset - 0x100, data);
	} else
		namco_wavedata[offset] = data;
}

UINT8 namcos1_custom30_read(INT32 offset)
{
#if defined FBNEO_DEBUG
	if (!DebugSnd_NamcoSndInitted) bprintf(PRINT_ERROR, _T("namcos1_custom30_read called without init\n"));
#endif

	return namco_wavedata[offset];
}

static INT32 build_decoded_waveform()
{
	INT16 *p;
	INT32 size;
	INT32 offset;
	INT32 v;

	if (NamcoSoundProm != NULL)
		namco_wavedata = NamcoSoundProm;

	/* 20pacgal has waves in RAM but old sound system */
	if (NamcoSoundProm == NULL && chip->num_voices != 3)
	{
		chip->wave_size = 1;
		size = 32 * 16;		/* 32 samples, 16 waveforms */
	}
	else
	{
		chip->wave_size = 0;
		size = 32 * 8;		/* 32 samples, 8 waveforms */
	}

	namco_waveformdatasize = size * MAX_VOLUME * sizeof (INT16);

	p = (INT16*)BurnMalloc(namco_waveformdatasize);
	namco_waveformdata = (UINT8*)p; //strictly for savestates.

	memset(p, 0, namco_waveformdatasize);

	for (v = 0; v < MAX_VOLUME; v++)
	{
		chip->waveform[v] = p;
		p += size;
	}

	if (namco_wavedata == NULL) {
		enable_ram = 1;
		namco_wavedata = (UINT8*)BurnMalloc(0x400);
		memset(namco_wavedata, 0, 0x400);
	}

	/* We need waveform data. It fails if region is not specified. */
	if (namco_wavedata)
	{
		for (offset = 0; offset < 256; offset++)
			update_namco_waveform(offset, namco_wavedata[offset]);
	}

	return 0;
}

void NamcoSoundReset()
{
#if defined FBNEO_DEBUG
	if (!DebugSnd_NamcoSndInitted) bprintf(PRINT_ERROR, _T("NamcoSoundReset called without init\n"));
#endif

	sound_channel *voice;

	/* reset all the voices */
	for (voice = chip->channel_list; voice < chip->last_channel; voice++)
	{
		voice->frequency = 0;
		voice->volume[0] = voice->volume[1] = 0;
		voice->waveform_select = 0;
		voice->counter = 0;
		voice->noise_sw = 0;
		voice->noise_state = 0;
		voice->noise_seed = 1;
		voice->noise_counter = 0;
		voice->noise_hold = 0;
	}
}

void NamcoSoundInit(INT32 clock, INT32 num_voices, INT32 bAdd)
{
	DebugSnd_NamcoSndInitted = 1;
	
	INT32 clock_multiple;
	sound_channel *voice;
	
	chip = (struct namco_sound*)BurnMalloc(sizeof(*chip));
	memset(chip, 0, sizeof(*chip));

	namco_soundregs = (UINT8*)BurnMalloc(0x400);
	memset(namco_soundregs, 0, 0x400);

	soundbuf = (INT16*)BurnMalloc(0x1000); // for buffered updates

	chip->num_voices = num_voices;
	chip->last_channel = chip->channel_list + chip->num_voices;
	chip->stereo = 0;

	chip->bAdd = bAdd;

	/* adjust internal clock */
	chip->namco_clock = clock;
	for (clock_multiple = 0; chip->namco_clock < INTERNAL_RATE; clock_multiple++)
		chip->namco_clock *= 2;

	chip->f_fracbits = clock_multiple + 15;

	/* adjust output clock */
	chip->sample_rate = chip->namco_clock;

	/* build the waveform table */
	if (build_decoded_waveform()) return;
	
	/* start with sound enabled, many games don't have a sound enable register */
	chip->sound_enable = 1;

	/* reset all the voices */
	for (voice = chip->channel_list; voice < chip->last_channel; voice++)
	{
		voice->frequency = 0;
		voice->volume[0] = voice->volume[1] = 0;
		voice->waveform_select = 0;
		voice->counter = 0;
		voice->noise_sw = 0;
		voice->noise_state = 0;
		voice->noise_seed = 1;
		voice->noise_counter = 0;
		voice->noise_hold = 0;
	}

	chip->update_step = ((double)INTERNAL_RATE / nBurnSoundRate);

	chip->gain[BURN_SND_NAMCOSND_ROUTE_1] = 1.00;
	chip->gain[BURN_SND_NAMCOSND_ROUTE_2] = 1.00;
	chip->output_dir[BURN_SND_NAMCOSND_ROUTE_1] = BURN_SND_ROUTE_BOTH;
	chip->output_dir[BURN_SND_NAMCOSND_ROUTE_2] = BURN_SND_ROUTE_BOTH;
}

void NamcoSoundSetRoute(INT32 nIndex, double nVolume, INT32 nRouteDir)
{
#if defined FBNEO_DEBUG
	if (!DebugSnd_NamcoSndInitted) bprintf(PRINT_ERROR, _T("NamcoSoundSetRoute called without init\n"));
	if (nIndex < 0 || nIndex > 1) bprintf(PRINT_ERROR, _T("NamcoSoundSetRoute called with invalid index %i\n"), nIndex);
#endif

	chip->gain[nIndex] = nVolume;
	chip->output_dir[nIndex] = nRouteDir;
}

void NamcoSoundExit()
{
#if defined FBNEO_DEBUG
	if (!DebugSnd_NamcoSndInitted) bprintf(PRINT_ERROR, _T("NamcoSoundExit called without init\n"));
#endif

	if (!DebugSnd_NamcoSndInitted) {
		NamcoSoundProm = NULL; // sometimes drivers set this in MemIndex, if DrvInit fails, this must be cleared.
		namco_wavedata = NULL; // this is important.

		enable_ram = 0;

		return;
	}

	BurnFree(chip);
	BurnFree(namco_soundregs);
	if (enable_ram) {
		BurnFree(namco_wavedata);
	}
	BurnFree(namco_waveformdata);

	BurnFree(soundbuf);
    if (namco_buffered) {
        namco_buffered = 0;
        pCPUTotalCycles = NULL;
		nDACCPUMHZ = 0;
		nPosition = 0;
    }

	NamcoSoundProm = NULL;
	namco_wavedata = NULL; // this is important.

	enable_ram = 0;
	DebugSnd_NamcoSndInitted = 0;
}

void NamcoSoundScan(INT32 nAction, INT32 *pnMin)
{
	struct BurnArea ba;
	char szName[30];

	if ((nAction & ACB_DRIVER_DATA) == 0) {
		return;
	}

	if (pnMin != NULL) {
		*pnMin = 0x029707;
	}

	memset(&ba, 0, sizeof(ba));
	sprintf(szName, "NamcoSound");
	ba.Data		= &chip->channel_list;
	ba.nLen		= sizeof(chip->channel_list);
	ba.nAddress = 0;
	ba.szName	= szName;
	BurnAcb(&ba);

	memset(&ba, 0, sizeof(ba));
	sprintf(szName, "NamcoSoundWaveFormData");
	ba.Data		= namco_waveformdata;
	ba.nLen		= namco_waveformdatasize;
	ba.nAddress = 0;
	ba.szName	= szName;
	BurnAcb(&ba);

	if (enable_ram) {
		memset(&ba, 0, sizeof(ba));
		sprintf(szName, "NamcoSoundWaveData");
		ba.Data		= namco_wavedata;
		ba.nLen		= 0x400;
		ba.nAddress = 0;
		ba.szName	= szName;
		BurnAcb(&ba);
	}

	memset(&ba, 0, sizeof(ba));
	sprintf(szName, "NamcoSoundRegs");
	ba.Data		= namco_soundregs;
	ba.nLen		= 0x400;
	ba.nAddress = 0;
	ba.szName	= szName;
	BurnAcb(&ba);

	SCAN_VAR(chip->sound_enable);
}
