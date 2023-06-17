#define CHANNEL_DEBUG 0
#define VERBOSE 0

/*********************************************************

	Konami 054539 PCM Sound Chip

	A lot of information comes from Amuse.
	Big thanks to them.



CHANNEL_DEBUG enables the following keys:

	PAD.   : toggle debug mode
	PAD0   : toggle chip    (0 / 1)
	PAD4,6 : select channel (0 - 7)
	PAD8,2 : adjust gain    (00=0.0 10=1.0, 20=2.0, etc.)
	PAD5   : reset gain factor to 1.0

*********************************************************/

#include "driver.h"
#include "state.h"
#include "k054539.h"
#include <math.h>

/* Registers:
   00..ff: 20 bytes/channel, 8 channels
     00..02: pitch (lsb, mid, msb)
         03: volume (0=max, 0x40=-36dB)
         04: reverb volume (idem)
	 05: pan (1-f right, 10 middle, 11-1f left)
     06..07: reverb delay (0=max, current computation non-trusted)
     08..0a: loop (lsb, mid, msb)
     0c..0e: start (lsb, mid, msb) (and current position ?)

   100.1ff: effects?
     13f: pan of the analog input (1-1f)

   200..20f: 2 bytes/channel, 8 channels
     00: type (b2-3), reverse (b5)
	 01: loop (b0)

   214: keyon (b0-7 = channel 0-7)
   215: keyoff          ""
   22c: channel active? ""
   22d: data read/write port
   22e: rom/ram select (00..7f == rom banks, 80 = ram)
   22f: enable pcm (b0), disable register ram updating (b7)

   The chip has a 0x4000 bytes reverb buffer (the ram from 0x22e).
   The reverb delay is actually an offset in this buffer.  This driver
   uses some tricks (doubling the buffer size so that the longest
   reverbs don't fold over the sound to output, and adding a space at
   the end to fold back overflows in) to be able to do frame-based
   rendering instead of sample-based.
*/

#define MAX_K054539 2

static struct {
	const struct K054539interface *intf;
	double freq_ratio;
	double voltab[256];
	double pantab[0xf];

	struct K054539_chip {
		unsigned char regs[0x230];
		unsigned char *ram;
		int reverb_pos;

		int cur_ptr;
		int cur_limit;
		unsigned char *cur_zone;
		unsigned char *rom;
		UINT32 rom_size;
		UINT32 rom_mask;
		int stream;

		struct K054539_channel {
			UINT32 pos;
			UINT32 pfrac;
			INT32 val;
			INT32 pval;
		} channels[8];
	} chip[MAX_K054539];
} K054539_chips;

/***/
static double K054539_gain[MAX_K054539][8] =
{
	{1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0},
	{1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0}
};
static data8_t K054539_posreg_latch[MAX_K054539][8][3];
static int K054539_flags = K054539_RESET_FLAGS;

void K054539_init_flags(int flags)
{
	K054539_flags = flags;
}

void K054539_set_gain(int chip, int channel, double gain)
{
	if (gain >= 0) K054539_gain[chip][channel] = gain;
}
/***/

static int K054539_regupdate(int chip)
{
	return !(K054539_chips.chip[chip].regs[0x22f] & 0x80);
}

static void K054539_keyon(int chip, int channel)
{
	if(K054539_regupdate(chip))
		K054539_chips.chip[chip].regs[0x22c] |= 1 << channel;
}

static void K054539_keyoff(int chip, int channel)
{
	if(K054539_regupdate(chip))
		K054539_chips.chip[chip].regs[0x22c] &= ~(1 << channel);
}

static void K054539_update(int chip, INT16 **buffer, int length)
{
#define VOL_CAP 1.80

	static INT16 dpcm[16] = {
		0<<8, 1<<8, 4<<8, 9<<8, 16<<8, 25<<8, 36<<8, 49<<8,
		-64<<8, -49<<8, -36<<8, -25<<8, -16<<8, -9<<8, -4<<8, -1<<8
	};

	int ch, reverb_pos;
	short *rev_max;
	short *rbase, *rbuffer, *rev_top;
	unsigned char *samples;
	UINT32 rom_mask;

	unsigned char *base1, *base2;
	struct K054539_channel *chan;
	short *bufl, *bufr, *revb;
	int cur_pos, cur_pfrac, cur_val, cur_pval;
	int delta, rdelta, fdelta, pdelta;
	int vol, bval, pan, i;

	double gain, lvol, rvol, rbvol;

	reverb_pos = K054539_chips.chip[chip].reverb_pos;
	rbase = (short *)(K054539_chips.chip[chip].ram);
	rbuffer = rbase + reverb_pos;
	rev_max = rev_top = rbase + 0x4000;

	memset(buffer[0], 0, length*2);
	memset(buffer[1], 0, length*2);

	samples = K054539_chips.chip[chip].rom;
	rom_mask = K054539_chips.chip[chip].rom_mask;

	if(!(K054539_chips.chip[chip].regs[0x22f] & 1)) return;

	K054539_chips.chip[chip].reverb_pos = (reverb_pos + length) & 0x3fff;


	for(ch=0; ch<8; ch++)
		if(K054539_chips.chip[chip].regs[0x22c] & (1<<ch)) {
			base1 = K054539_chips.chip[chip].regs + 0x20*ch;
			base2 = K054539_chips.chip[chip].regs + 0x200 + 0x2*ch;
			chan = K054539_chips.chip[chip].channels + ch;
/***/
			delta = K054539_chips.freq_ratio * (base1[0x00] | (base1[0x01] << 8) | (base1[0x02] << 16));

			vol = base1[0x03];

			bval = vol + base1[0x04];
			if (bval > 255) bval = 255;

			pan = base1[0x05];
      /* DJ Main: 81-87 right, 88 middle, 89-8f left*/
      if (pan >= 0x81 && pan <= 0x8f)
        pan -= 0x81;
      else
        if (pan >= 0x11 && pan <= 0x1f)
          pan -= 0x11; 
        else
          pan = 0x18 - 0x11;

			gain = K054539_gain[chip][ch];

			lvol = K054539_chips.voltab[vol] * K054539_chips.pantab[pan] * gain;
			if (lvol > VOL_CAP) lvol = VOL_CAP;

			rvol = K054539_chips.voltab[vol] * K054539_chips.pantab[0xe - pan] * gain;
			if (rvol > VOL_CAP) rvol = VOL_CAP;

			rbvol= K054539_chips.voltab[bval] * gain / 2;
			if (rbvol > VOL_CAP) rbvol = VOL_CAP;

/*
	INT x FLOAT could be interpreted as INT x (int)FLOAT instead of (float)INT x FLOAT on some compilers
	causing precision loss. (rdelta - 0x2000) wraps around on zero reverb and the scale factor should
	actually be 1/freq_ratio because the target is an offset to the reverb buffer not sample source.
*/
			rdelta = (base1[6] | (base1[7] << 8)) >> 3;
/*			rdelta = (reverb_pos + (int)((rdelta - 0x2000) * K054539_chips.freq_ratio)) & 0x3fff;*/
			rdelta = (int)((double)rdelta / K054539_chips.freq_ratio + reverb_pos) & 0x3fff;
			revb = rbase + rdelta;

			cur_pos = (base1[0x0c] | (base1[0x0d] << 8) | (base1[0x0e] << 16)) & rom_mask;

			bufl = buffer[0];
			bufr = buffer[1];
/***/

			if(base2[0] & 0x20) {
				delta = -delta;
				fdelta = +0x10000;
				pdelta = -1;
			} else {
				fdelta = -0x10000;
				pdelta = +1;
			}

			if(cur_pos != chan->pos) {
				chan->pos = cur_pos;
				cur_pfrac = 0;
				cur_val = 0;
				cur_pval = 0;
			} else {
				cur_pfrac = chan->pfrac;
				cur_val = chan->val;
				cur_pval = chan->pval;
			}

#define UPDATE_CHANNELS																	\
			do {																		\
				*bufl++ += (INT16)(cur_val*lvol);										\
				*bufr++ += (INT16)(cur_val*rvol);										\
				*revb++ += (INT16)(cur_val*rbvol);										\
			} while(0)

			switch(base2[0] & 0xc) {
			case 0x0: { /* 8bit pcm*/
				for(i=0; i<length; i++) {
					cur_pfrac += delta;
					while(cur_pfrac & ~0xffff) {
						cur_pfrac += fdelta;
						cur_pos += pdelta;

						cur_pval = cur_val;
						cur_val = (INT16)(samples[cur_pos] << 8);
						if(cur_val == (INT16)0x8000) {
							if(base2[1] & 1) {
								cur_pos = (base1[0x08] | (base1[0x09] << 8) | (base1[0x0a] << 16)) & rom_mask;
								cur_val = (INT16)(samples[cur_pos] << 8);
								if(cur_val != (INT16)0x8000)
									continue;
							}
							K054539_keyoff(chip, ch);
							goto end_channel_0;
						}
					}

					UPDATE_CHANNELS;
				}
			end_channel_0:
				break;
			}
			case 0x4: { /* 16bit pcm lsb first*/
				pdelta <<= 1;

				for(i=0; i<length; i++) {
					cur_pfrac += delta;
					while(cur_pfrac & ~0xffff) {
						cur_pfrac += fdelta;
						cur_pos += pdelta;

						cur_pval = cur_val;
						cur_val = (INT16)(samples[cur_pos] | samples[cur_pos+1]<<8);
						if(cur_val == (INT16)0x8000) {
							if(base2[1] & 1) {
								cur_pos = (base1[0x08] | (base1[0x09] << 8) | (base1[0x0a] << 16)) & rom_mask;
								cur_val = (INT16)(samples[cur_pos] | samples[cur_pos+1]<<8);
								if(cur_val != (INT16)0x8000)
									continue;
							}
							K054539_keyoff(chip, ch);
							goto end_channel_4;
						}
					}

					UPDATE_CHANNELS;
				}
			end_channel_4:
				break;
			}
			case 0x8: { /* 4bit dpcm*/
				cur_pos <<= 1;
				cur_pfrac <<= 1;
				if(cur_pfrac & 0x10000) {
					cur_pfrac &= 0xffff;
					cur_pos |= 1;
				}

				for(i=0; i<length; i++) {
					cur_pfrac += delta;
					while(cur_pfrac & ~0xffff) {
						cur_pfrac += fdelta;
						cur_pos += pdelta;

						cur_pval = cur_val;
						cur_val = samples[cur_pos>>1];
						if(cur_val == 0x88) {
							if(base2[1] & 1) {
								cur_pos = ((base1[0x08] | (base1[0x09] << 8) | (base1[0x0a] << 16)) & rom_mask) << 1;
								cur_val = samples[cur_pos>>1];
								if(cur_val != 0x88)
									goto next_iter;
							}
							K054539_keyoff(chip, ch);
							goto end_channel_8;
						}
					next_iter:
						if(cur_pos & 1)
							cur_val >>= 4;
						else
							cur_val &= 15;
						cur_val = cur_pval + dpcm[cur_val];
                  MAME_CLAMP_SAMPLE(cur_val);
					}

					UPDATE_CHANNELS;
				}
			end_channel_8:
				cur_pfrac >>= 1;
				if(cur_pos & 1)
					cur_pfrac |= 0x8000;
				cur_pos >>= 1;
				break;
			}
			default:
  			log_cb(RETRO_LOG_DEBUG, LOGPRE "Unknown sample type %x for channel %d\n", base2[0] & 0xc, ch);
			break;
			}
			chan->pos = cur_pos;
			chan->pfrac = cur_pfrac;
			chan->pval = cur_pval;
			chan->val = cur_val;
			if(K054539_regupdate(chip)) {
				base1[0x0c] = cur_pos     & 0xff;
				base1[0x0d] = cur_pos>> 8 & 0xff;
				base1[0x0e] = cur_pos>>16 & 0xff;
			}

			if(revb > rev_max)
				rev_max = revb;
		}

	while(rev_max >= rev_top) {
		rev_max[-0x4000] += rev_max[0];
		rev_max[0] = 0;
		rev_max--;
	}

	/** drivers should be given the option to disable reverb when things go terribly wrong*/
	if(!(K054539_flags & K054539_DISABLE_REVERB))
	{
		for(i=0; i<length; i++) {
			short val = rbase[(i+reverb_pos) & 0x3fff];
			buffer[0][i] += val;
			buffer[1][i] += val;
		}
	}

	if(rbuffer+length > rev_top) {
		i = rev_top-rbuffer; /* delta*/
		memset(rbuffer, 0, i*2);
		memset(rbase, 0, (length-i)*2);
	} else
		memset(rbuffer, 0, length*2);

	#if CHANNEL_DEBUG
	{
		static char gc_msg[32] = "chip :                         ";
		static int gc_active=0, gc_chip=0, gc_pos[2]={0,0};
		double *gc_fptr;
		char *gc_cptr;
		double gc_f0;
		int gc_i, gc_j, gc_k, gc_l;

		if (code_pressed_memory(KEYCODE_DEL_PAD))
		{
			gc_active ^= 1;
			if (!gc_active) usrintf_showmessage_secs(0, " ");
		}

		if (gc_active)
		{
			if (code_pressed_memory(KEYCODE_0_PAD)) gc_chip ^= 1;

			gc_i = gc_pos[gc_chip];
			gc_j = 0;
			if (code_pressed_memory(KEYCODE_4_PAD)) { gc_i--; gc_j = 1; }
			if (code_pressed_memory(KEYCODE_6_PAD)) { gc_i++; gc_j = 1; }
			if (gc_j) { gc_i &= 7; gc_pos[gc_chip] = gc_i; }

			if (code_pressed_memory(KEYCODE_5_PAD))
				K054539_gain[gc_chip][gc_i] = 1.0;
			else
			{
				gc_fptr = &K054539_gain[gc_chip][gc_i];
				gc_f0 = *gc_fptr;
				gc_j = 0;
				if (code_pressed_memory(KEYCODE_2_PAD)) { gc_f0 -= 0.1; gc_j = 1; }
				if (code_pressed_memory(KEYCODE_8_PAD)) { gc_f0 += 0.1; gc_j = 1; }
				if (gc_j) { if (gc_f0 < 0) gc_f0 = 0; *gc_fptr = gc_f0; }
			}

			gc_fptr = &K054539_gain[gc_chip][0] + 8;
			gc_cptr = gc_msg + 7;
			for (gc_j=-8; gc_j; gc_j++)
			{
				gc_k = (int)(gc_fptr[gc_j] * 10);
				gc_l = gc_k / 10;
				gc_k = gc_k % 10;
				gc_cptr[0] = gc_l + '0';
				gc_cptr[1] = gc_k + '0';
				gc_cptr += 3;
			}
			gc_i = (gc_i + gc_i*2 + 6);
			gc_msg[4] = gc_chip + '0';
			gc_msg[gc_i  ] = '[';
			gc_msg[gc_i+3] = ']';
			usrintf_showmessage("%s", gc_msg);
			gc_msg[gc_i+3] = gc_msg[gc_i] = ' ';
		}
	}
	#endif
}


static void K054539_irq(int chip)
{
	if(K054539_chips.chip[chip].regs[0x22f] & 0x20)
		K054539_chips.intf->irq[chip] ();
}

static void K054539_init_chip(int chip, const struct MachineSound *msound)
{
	char buf[2][50];
	const char *bufp[2];
	int vol[2];
	int i, panleft, panright;

	memset(K054539_chips.chip[chip].regs, 0, sizeof(K054539_chips.chip[chip].regs));
	memset(K054539_posreg_latch, 0, sizeof(K054539_posreg_latch)); /***/
	K054539_flags |= K054539_UPDATE_AT_KEYON; /** make it default until proven otherwise*/

	/* Real size of 0x4000, the addon is to simplify the reverb buffer computations*/
	K054539_chips.chip[chip].ram = malloc(0x4000*2+48000/55*2+0x4000);
	K054539_chips.chip[chip].reverb_pos = 0;
	K054539_chips.chip[chip].cur_ptr = 0;
	memset(K054539_chips.chip[chip].ram, 0, 0x4000*2+48000/55*2+0x4000);

	K054539_chips.chip[chip].rom = memory_region(K054539_chips.intf->region[chip]);
	K054539_chips.chip[chip].rom_size = memory_region_length(K054539_chips.intf->region[chip]);
	K054539_chips.chip[chip].rom_mask = 0xffffffffU;
	for(i=0; i<32; i++)
		if((1U<<i) >= K054539_chips.chip[chip].rom_size) {
			K054539_chips.chip[chip].rom_mask = (1U<<i) - 1;
			break;
		}

	if(K054539_chips.intf->irq[chip])
		/* One or more of the registers must be the timer period*/
		/* And anyway, this particular frequency is probably wrong*/
		/* 480 hz is TRUSTED by gokuparo disco stage - the looping sample doesn't line up otherwise*/
		timer_pulse(TIME_IN_HZ(480), 0, K054539_irq);

	sprintf(buf[0], "%s.%d L", sound_name(msound), chip);
	sprintf(buf[1], "%s.%d R", sound_name(msound), chip);
	bufp[0] = buf[0];
	bufp[1] = buf[1];

	if (K054539_flags & K054539_REVERSE_STEREO) /***/
	{
		panleft  = MIXER_PAN_RIGHT;
		panright = MIXER_PAN_LEFT;
	}
	else
	{
		panleft  = MIXER_PAN_LEFT;
		panright = MIXER_PAN_RIGHT;
	}

	vol[0] = MIXER(K054539_chips.intf->mixing_level[chip][0], panleft);
	vol[1] = MIXER(K054539_chips.intf->mixing_level[chip][1], panright);

	K054539_chips.chip[chip].stream = stream_init_multi(2, bufp, vol, Machine->sample_rate, chip, K054539_update);

	state_save_register_UINT8("K054539", chip, "registers", K054539_chips.chip[chip].regs, 0x230);
	state_save_register_UINT8("K054539", chip, "ram",       K054539_chips.chip[chip].ram,  0x4000);
	state_save_register_int  ("K054539", chip, "cur_ptr",  &K054539_chips.chip[chip].cur_ptr);
}

static void K054539_stop_chip(int chip)
{
	free(K054539_chips.chip[chip].ram);
}

static void K054539_w(int chip, offs_t offset, data8_t data) /***/
{
#if 0
	int voice, reg;

	/* The K054539 has behavior like many other wavetable chips including
	   the Ensoniq 550x and Gravis GF-1: if a voice is active, writing
	   to it's current position is silently ignored.

	   Dadandaan depends on this or the vocals go wrong.
	   */
	if (offset < 8*0x20)
	{
		voice = offset / 0x20;
		reg = offset & ~0x20;

		if(K054539_chips.chip[chip].regs[0x22c] & (1<<voice))
		{
			if (reg >= 0xc && reg <= 0xe)
			{
				return;
			}
		}
	}
#endif

	int latch, offs, ch, pan;
	data8_t *regbase, *regptr, *posptr;

	regbase = K054539_chips.chip[chip].regs;
	latch = (K054539_flags & K054539_UPDATE_AT_KEYON) && (regbase[0x22f] & 1);

	if (latch && offset < 0x100)
	{
		offs = (offset & 0x1f) - 0xc;
		ch = offset >> 5;

		if (offs >= 0 && offs <= 2)
		{
			/* latch writes to the position index registers*/
			K054539_posreg_latch[chip][ch][offs] = data;
			return;
		}
	}

	else switch(offset)
	{
		case 0x13f:
			pan = data >= 0x11 && data <= 0x1f ? data - 0x11 : 0x18 - 0x11;
			if(K054539_chips.intf->apan[chip])
				K054539_chips.intf->apan[chip](K054539_chips.pantab[pan], K054539_chips.pantab[0xe - pan]);
		break;

		case 0x214:
			if (latch)
			{
				for(ch=0; ch<8; ch++)
				{
					if(data & (1<<ch))
					{
						posptr = &K054539_posreg_latch[chip][ch][0];
						regptr = regbase + (ch<<5) + 0xc;

						/* update the chip at key-on*/
						regptr[0] = posptr[0];
						regptr[1] = posptr[1];
						regptr[2] = posptr[2];

						K054539_keyon(chip, ch);
					}
				}
			}
			else
			{
				for(ch=0; ch<8; ch++)
					if(data & (1<<ch))
						K054539_keyon(chip, ch);
			}
		break;

		case 0x215:
			for(ch=0; ch<8; ch++)
				if(data & (1<<ch))
					K054539_keyoff(chip, ch);
		break;

		case 0x22d:
			if(regbase[0x22e] == 0x80)
				K054539_chips.chip[chip].cur_zone[K054539_chips.chip[chip].cur_ptr] = data;
			K054539_chips.chip[chip].cur_ptr++;
			if(K054539_chips.chip[chip].cur_ptr == K054539_chips.chip[chip].cur_limit)
				K054539_chips.chip[chip].cur_ptr = 0;
		break;

		case 0x22e:
			K054539_chips.chip[chip].cur_zone =
				data == 0x80 ? K054539_chips.chip[chip].ram :
				K054539_chips.chip[chip].rom + 0x20000*data;
			K054539_chips.chip[chip].cur_limit = data == 0x80 ? 0x4000 : 0x20000;
			K054539_chips.chip[chip].cur_ptr = 0;
		break;

		default:
#if 0
    if(regbase[offset] != data) {
				if((offset & 0xff00) == 0) {
					chanoff = offset & 0x1f;
					if(chanoff < 4 || chanoff == 5 ||
					   (chanoff >=8 && chanoff <= 0xa) ||
					   (chanoff >= 0xc && chanoff <= 0xe))
						break;
				}
				if(1 || ((offset >= 0x200) && (offset <= 0x210)))
					break;
				log_cb(RETRO_LOG_DEBUG, LOGPRE "K054539 %03x = %02x\n", offset, data);
			}
#endif      
		break;
	}

	regbase[offset] = data;
}

static void reset_zones(void)
{
	int chip;
	for(chip=0; chip<K054539_chips.intf->num; chip++) {
		int data = K054539_chips.chip[chip].regs[0x22e];
		K054539_chips.chip[chip].cur_zone =
			data == 0x80 ? K054539_chips.chip[chip].ram :
			K054539_chips.chip[chip].rom + 0x20000*data;
		K054539_chips.chip[chip].cur_limit = data == 0x80 ? 0x4000 : 0x20000;
	}
}

static data8_t K054539_r(int chip, offs_t offset)
{
	switch(offset) {
	case 0x22d:
		if(K054539_chips.chip[chip].regs[0x22f] & 0x10) {
			data8_t res = K054539_chips.chip[chip].cur_zone[K054539_chips.chip[chip].cur_ptr];
			K054539_chips.chip[chip].cur_ptr++;
			if(K054539_chips.chip[chip].cur_ptr == K054539_chips.chip[chip].cur_limit)
				K054539_chips.chip[chip].cur_ptr = 0;
			return res;
		} else
			return 0;
	case 0x22c:
		break;
	default:
		log_cb(RETRO_LOG_DEBUG, LOGPRE "K054539 read %03x\n", offset);
		break;
	}
	return K054539_chips.chip[chip].regs[offset];
}

int K054539_sh_start(const struct MachineSound *msound)
{
	int i;

	K054539_chips.intf = msound->sound_interface;

	if(Machine->sample_rate)
		K054539_chips.freq_ratio = (double)(K054539_chips.intf->clock) / (double)(Machine->sample_rate);
	else
		K054539_chips.freq_ratio = 1.0;

	/*
		I've tried various equations on volume control but none worked consistently.
		The upper four channels in most MW/GX games simply need a significant boost
		to sound right. For example, the bass and smash sound volumes in Violent Storm
		have roughly the same values and the voices in Tokimeki Puzzledama are given
		values smaller than those of the hihats. Needless to say the two K054539 chips
		in Mystic Warriors are completely out of balance. Rather than forcing a
		"one size fits all" function to the voltab the current invert exponential
		appraoch seems most appropriate.
	*/
	/* Factor the 1/4 for the number of channels in the volume (1/8 is too harsh, 1/2 gives clipping)*/
	/* vol=0 -> no attenuation, vol=0x40 -> -36dB*/
	for(i=0; i<256; i++)
		K054539_chips.voltab[i] = pow(10.0, (-36.0 * (double)i / (double)0x40) / 20.0) / 4.0;

	/* Pan table for the left channel*/
	/* Right channel is identical with inverted index*/
	/* Formula is such that pan[i]**2+pan[0xe-i]**2 = 1 (constant output power)*/
	/* and pan[0xe] = 1 (full panning)*/
	for(i=0; i<0xf; i++)
		K054539_chips.pantab[i] = sqrt(i) / sqrt(0xe);

	for(i=0; i<K054539_chips.intf->num; i++)
		K054539_init_chip(i, msound);

	state_save_register_func_postload(reset_zones);
	return 0;
}

void K054539_sh_stop(void)
{
	int i;
	for(i=0; i<K054539_chips.intf->num; i++)
		K054539_stop_chip(i);

	for (i=0; i<MAX_K054539*8; i++) ((double *)K054539_gain)[i] = 1.0;
	K054539_flags = K054539_RESET_FLAGS;
}

WRITE_HANDLER( K054539_0_w )
{
	K054539_w(0, offset, data);
}

READ_HANDLER( K054539_0_r )
{
	return K054539_r(0, offset);
}

WRITE_HANDLER( K054539_1_w )
{
	K054539_w(1, offset, data);
}

READ_HANDLER( K054539_1_r )
{
	return K054539_r(1, offset);
}
