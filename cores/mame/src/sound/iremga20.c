/*********************************************************

Irem GA20 PCM Sound Chip

It's not currently known whether this chip is stereo.


Revisions:

04-15-2002 Acho A. Tang
- rewrote channel mixing
- added prelimenary volume and sample rate emulation

05-30-2002 Acho A. Tang
- applied hyperbolic gain control to volume and used
  a musical-note style progression in sample rate
  calculation(still very inaccurate)

02-18-2004 R. Belmont
- sample rate calculation reverse-engineered.
  Thanks to Fujix, Yasuhiro Ogawa, the Guru, and Tormod
  for real PCB samples that made this possible.

*********************************************************/
#include <math.h>
#include "driver.h"
#include "iremga20.h"
#include "state.h"

/*AT*/
#define MAX_VOL 256

#define MIX_CH(CH) \
	if (play[CH]) \
	{ \
		eax = pos[CH]; \
		ebx = eax; \
		eax >>= 8; \
		eax = *(char *)(esi + eax); \
		eax *= vol[CH]; \
		ebx += rate[CH]; \
		pos[CH] = ebx; \
		ebx = (ebx < end[CH]); \
		play[CH] = ebx; \
		edx += eax; \
	}
/*ZT*/

static struct IremGA20_chip_def
{
	const struct IremGA20_interface *intf;
	unsigned char *rom;
	int rom_size;
	int channel;
	int mode;
	int regs[0x40];
} IremGA20_chip;

static struct IremGA20_channel_def
{
	unsigned long rate;
	unsigned long size;
	unsigned long start;
	unsigned long pos;
	unsigned long end;
	unsigned long volume;
	unsigned long pan;
	unsigned long effect;
	unsigned long play;
} IremGA20_channel[4];

static int sr_table[256];

void IremGA20_update( int param, INT16 **buffer, int length )
{
	unsigned long rate[4], pos[4], end[4], vol[4], play[4];
	unsigned long edi, ebp, esi;
	int eax, ebx, ecx, edx;

	if (!Machine->sample_rate) return;

	/* precache some values */
	for (ecx=0; ecx<4; ecx++)
	{
		rate[ecx] = IremGA20_channel[ecx].rate;
		pos[ecx] = IremGA20_channel[ecx].pos;
		end[ecx] = (IremGA20_channel[ecx].end - 0x20) << 8;
		vol[ecx] = IremGA20_channel[ecx].volume;
		play[ecx] = IremGA20_channel[ecx].play;
	}

	ecx = length << 1;
	esi = (unsigned long)IremGA20_chip.rom;
	edi = (unsigned long)buffer[0];
	ebp = (unsigned long)buffer[1];
	edi += ecx;
	ebp += ecx;
	ecx = -ecx;

	for (; ecx; ecx+=2)
	{
		edx = 0;

		MIX_CH(0);
		MIX_CH(1);
		MIX_CH(2);
		MIX_CH(3);

		edx >>= 2;
		*(short *)(edi + ecx) = (short)edx;
		*(short *)(ebp + ecx) = (short)edx;
	}

	/* update the regs now */
	for (ecx=0; ecx< 4; ecx++)
	{
		IremGA20_channel[ecx].pos = pos[ecx];
		IremGA20_channel[ecx].play = play[ecx];
	}
}
/*ZT*/

WRITE_HANDLER( IremGA20_w )
{
	int channel;

	/*logerror("GA20:  Offset %02x, data %04x\n",offset,data);*/

	if (!Machine->sample_rate)
		return;

	stream_update(IremGA20_chip.channel, 0);

	channel = offset >> 4;

	IremGA20_chip.regs[offset] = data;

	switch (offset & 0xf)
	{
	case 0: /* start address low */
		IremGA20_channel[channel].start = ((IremGA20_channel[channel].start)&0xff000) | (data<<4);
	break;

	case 2: /* start address high */
		IremGA20_channel[channel].start = ((IremGA20_channel[channel].start)&0x00ff0) | (data<<12);
	break;

	case 4: /* end address low */
		IremGA20_channel[channel].end = ((IremGA20_channel[channel].end)&0xff000) | (data<<4);
	break;

	case 6: /* end address high */
		IremGA20_channel[channel].end = ((IremGA20_channel[channel].end)&0x00ff0) | (data<<12);
	break;

	case 8:
		IremGA20_channel[channel].rate = (sr_table[data]<<8) / Machine->sample_rate;
	break;

	case 0xa: /*AT: gain control*/
		IremGA20_channel[channel].volume = (data * MAX_VOL) / (data + 10);
	break;

	case 0xc: /*AT: this is always written 2(enabling both channels?)*/
		IremGA20_channel[channel].play = data;
		IremGA20_channel[channel].pos = IremGA20_channel[channel].start << 8;
	break;
	}
}

READ_HANDLER( IremGA20_r )
{
	int channel;

	if (!Machine->sample_rate)
		return 0;

	stream_update(IremGA20_chip.channel, 0);

	channel = offset >> 4;

	switch (offset & 0xf)
	{
		case 0xe:	/* voice status.  bit 0 is 1 if active. (routine around 0xccc in rtypeleo) */
			return IremGA20_channel[channel].play ? 1 : 0;
			break;

		default:
			log_cb(RETRO_LOG_DEBUG, LOGPRE "GA20: read unk. register %d, channel %d\n", offset & 0xf, channel);
			break;
	}

	return 0;
}

static void IremGA20_reset( void )
{
	int i;

	for( i = 0; i < 4; i++ ) {
	IremGA20_channel[i].rate = 0;
	IremGA20_channel[i].size = 0;
	IremGA20_channel[i].start = 0;
	IremGA20_channel[i].pos = 0;
	IremGA20_channel[i].end = 0;
	IremGA20_channel[i].volume = 0;
	IremGA20_channel[i].pan = 0;
	IremGA20_channel[i].effect = 0;
	IremGA20_channel[i].play = 0;
	}
}

int IremGA20_sh_start(const struct MachineSound *msound)
{
	const char *names[2];
	char ch_names[2][40];
	int i;

	if (!Machine->sample_rate) return 0;

	/* Initialize our chip structure */
	IremGA20_chip.intf = msound->sound_interface;
	IremGA20_chip.mode = 0;
	IremGA20_chip.rom = memory_region(IremGA20_chip.intf->region);
	IremGA20_chip.rom_size = memory_region_length(IremGA20_chip.intf->region);

	/* Initialize our pitch table */
	for (i = 0; i < 255; i++)
	{
		sr_table[i] = (IremGA20_chip.intf->clock / (256-i) / 4);
	}

	/* change signedness of PCM samples in advance */
	for (i=0; i<IremGA20_chip.rom_size; i++)
		IremGA20_chip.rom[i] -= 0x80;

	IremGA20_reset();

	for ( i = 0; i < 0x40; i++ )
	IremGA20_chip.regs[i] = 0;

	for ( i = 0; i < 2; i++ ) {
	names[i] = ch_names[i];
	sprintf(ch_names[i],"%s Ch %d",sound_name(msound),i);
	}

	IremGA20_chip.channel = stream_init_multi( 2, names,
			IremGA20_chip.intf->mixing_level, Machine->sample_rate,
			0, IremGA20_update );

	state_save_register_UINT8("sound", 0, "IremGA20_channel", (UINT8*) IremGA20_channel, sizeof(IremGA20_channel));
	state_save_register_UINT8("sound", 0, "IremGA20_chip",    (UINT8*) &IremGA20_chip,   sizeof(IremGA20_chip));

	return 0;
}

void IremGA20_sh_stop( void )
{
}
