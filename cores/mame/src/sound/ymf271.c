/*
	Yamaha YMF271-F "OPX" emulator v0.1
	By R. Belmont.  
	Based in part on YMF278B emulator by R. Belmont and O. Galibert.

	Copyright (c) 2003 R. Belmont.

	This software is dual-licensed: it may be used in MAME and properly licensed
	MAME derivatives under the terms of the MAME license.  For use outside of
	MAME and properly licensed derivatives, it is available under the 
	terms of the GNU Lesser General Public License (LGPL), version 2.1.
	You may read the LGPL at http://www.gnu.org/licenses/lgpl.html
*/

#include <math.h>
#include "driver.h"
#include "cpuintrf.h"

#define VERBOSE		(1)

#define CLOCK (44100 * 384)	/* = 16.9344 MHz*/

#define log2(n) (log((float) n)/log((float) 2))

typedef struct
{
	INT8  extout;
	INT16 lfoFreq;
	INT8  lfowave;
	INT8  pms, ams;
	INT8  detune;
	INT8  multiple;
	INT8  tl;
	INT8  keyscale;
	INT8  ar;
	INT8  decay1rate, decay2rate;
	INT8  decay1lvl;
	INT8  relrate;
	INT32 fns;
	INT8  block;
	INT8  feedback;
	INT8  waveform;
	INT8  accon;
	INT8  algorithm;
	INT8  ch0lvl, ch1lvl, ch2lvl, ch3lvl;	

	UINT32 startaddr;
	UINT32 loopaddr;
	UINT32 endaddr;
	INT8   fs, srcnote, srcb;

	UINT32 step;
	UINT32 stepptr;

	INT8 active;
	INT8 bits;

} YMF271Slot;

typedef struct
{
	INT8 sync, pfm;
} YMF271Group;

typedef struct
{
	YMF271Slot slots[48];
	YMF271Group groups[12];

	INT32 timerA, timerB;
	INT32 timerAVal, timerBVal;
	INT32 irqstate;
	INT8  status;
	INT8  enable;

	void *timA, *timB;

	INT8  reg0, reg1, reg2, reg3, pcmreg, timerreg;
	UINT32 ext_address;
	UINT8 ext_read;

	const UINT8 *rom;
	read8_handler ext_mem_read;
	write8_handler ext_mem_write;
	void (*irq_callback)(int);
} YMF271Chip;

/* slot mapping assists*/
static int fm_tab[] = { 0, 1, 2, -1, 3, 4, 5, -1, 6, 7, 8, -1, 9, 10, 11, -1 };
static int pcm_tab[] = { 0, 4, 8, -1, 12, 16, 20, -1, 24, 28, 32, -1, 36, 40, 44, -1 };

static YMF271Chip YMF271[MAX_YMF271];

static INT32 volume[256*4];			/* precalculated attenuation values with some marging for enveloppe and pan levels*/

static void ymf271_pcm_update(int num, INT16 **outputs, int length)
{
	int i, j;
	INT32 mix[48000*2];
	INT32 *mixp;
	INT16 sample = 0;
	YMF271Chip *chip = &YMF271[num];
	YMF271Slot *slot;
	const UINT8 *rombase;

	memset(mix, 0, sizeof(mix[0])*length*2);

	rombase = chip->rom;

	for (j = 0; j < 48; j++)
	{
		slot = &chip->slots[j];
		mixp = &mix[0];
		/* PCM*/
		if (slot->active && slot->waveform == 7)
		{
			for (i = 0; i < length; i++)
			{
				if (slot->bits == 8)
				{
					sample = rombase[slot->startaddr + (slot->stepptr>>16)]<<8;
				}
				else
				{
					if (slot->stepptr & 1)
						sample = rombase[slot->startaddr + (slot->stepptr>>17)*3 + 2]<<8 | ((rombase[slot->startaddr + (slot->stepptr>>17)*3 + 1] << 4) & 0xf0);
					else
						sample = rombase[slot->startaddr + (slot->stepptr>>17)*3]<<8 | (rombase[slot->startaddr + (slot->stepptr>>17)*3 + 1] & 0xf0);
				}

				*mixp++ += (sample * volume[slot->tl])>>16;
				*mixp++ += (sample * volume[slot->tl])>>16;

				slot->stepptr += slot->step << slot-> multiple;
				if ((slot->stepptr>>16) > slot->endaddr)
				{
					/* kill non-frac*/
					slot->stepptr &= 0xffff;
					slot->stepptr |= (slot->loopaddr<<16);
				}
			}
		}
	}

	mixp = &mix[0];
	for (i = 0; i < length; i++)
	{
		outputs[0][i] = (*mixp++)>>4;
		outputs[1][i] = (*mixp++)>>4;
	}
}

static void ymf271_write_fm(YMF271Chip *chip, int grp, int adr, int data)
{
	int slotnum;
	YMF271Slot *slot;

	slotnum = 12*grp;
	slotnum += fm_tab[adr & 0xf];
	slot = &chip->slots[slotnum];

	switch ((adr>>4)&0xf)
	{
		case 0:
			slot->extout = (data>>3)&0xf;

			if (data & 1)
			{
				slot->active = 1;

				/* key on*/
				slot->step = 0;
				slot->stepptr = 0;
/*				log_cb(RETRO_LOG_DEBUG, LOGPRE "start %x end %x loop %x\n", slot->startaddr, slot->endaddr, slot->loopaddr);*/
				if (slot->waveform != 7)
				{
/*					log_cb(RETRO_LOG_DEBUG, LOGPRE "UNSUPPORTED FM! on slot %d\n", slotnum);*/
				}
				else
				{
					int step, oct; 

/*					log_cb(RETRO_LOG_DEBUG, LOGPRE "oct %d fns %x fs %x srcnote %x srcb %x TL %x\n", slot->block, slot->fns, slot->fs, slot->srcnote, slot->srcb, slot->tl);*/

					oct = slot->block;
					if (oct & 8)
					{
						oct |= -8;
					}

					step = ((slot->fns/2) | 1024) << (oct + 7);
					slot->step = (UINT32) ((((INT64)step)*(44100/4)) / (Machine->sample_rate  << slot->fs ) );

/*					log_cb(RETRO_LOG_DEBUG, LOGPRE "step %x\n", slot->step);*/
				}
			}
			else
			{
				if (slot->active)
				{
					slot->active = 0;
				}
			}


			break;

		case 1:
			slot->lfoFreq = data;
			break;

		case 2:
			slot->lfowave = data & 3;
			slot->pms = (data >> 3) & 0x7;
			slot->ams = (data >> 6) & 0x7;
			break;

		case 3:
			slot->multiple = data & 0xf;
			slot->detune = (data >> 4) & 0x7;
			break;

		case 4:
			slot->tl = data & 0x7f;
			break;

		case 5:
			slot->ar = data & 0x1f;
			slot->keyscale = (data>>5)&0x7;
			break;

		case 6:
			slot->decay1rate = data & 0x1f;
			break;

		case 7:
			slot->decay2rate = data & 0x1f;
			break;

		case 8:
			slot->relrate = data & 0xf;
			slot->decay1lvl = (data >> 4) & 0xf;
			break;

		case 9:
			slot->fns &= ~0xff;
			slot->fns |= data;
			break;

		case 10:
			slot->fns &= ~0xff00;
			slot->fns |= (data & 0xf)<<8;
			slot->block = (data>>4)&0xf;
			break;

		case 11:
			slot->waveform = data & 0x7;
			slot->feedback = (data >> 4) & 0x7;
			slot->accon = (data & 0x80) ? 1 : 0;
			break;

		case 12:
			slot->algorithm = data & 0xf;
			break;

		case 13:
			slot->ch0lvl = data>>4;
			slot->ch1lvl = data & 0xf;
			break;

		case 14:
			slot->ch2lvl = data>>4;
			slot->ch3lvl = data & 0xf;
			break;

	}
}

static void ymf271_write_pcm(YMF271Chip *chip, int data)
{
	int slotnum;
	YMF271Slot *slot;

	slotnum = pcm_tab[chip->pcmreg&0xf];
	slot = &chip->slots[slotnum];

	switch ((chip->pcmreg>>4)&0xf)
	{
		case 0:
			slot->startaddr &= ~0xff;
			slot->startaddr |= data;
			break;
		case 1:
			slot->startaddr &= ~0xff00;
			slot->startaddr |= data<<8;
			break;
		case 2:
			slot->startaddr &= ~0xff0000;
			slot->startaddr |= data<<16;
			break;
		case 3:
			slot->endaddr &= ~0xff;
			slot->endaddr |= data;
			break;
		case 4:
			slot->endaddr &= ~0xff00;
			slot->endaddr |= data<<8;
			break;
		case 5:
			slot->endaddr &= ~0xff0000;
			slot->endaddr |= data<<16;
			break;
		case 6:
			slot->loopaddr &= ~0xff;
			slot->loopaddr |= data;
			break;
		case 7:
			slot->loopaddr &= ~0xff00;
			slot->loopaddr |= data<<8;
			break;
		case 8:
			slot->loopaddr &= ~0xff0000;
			slot->loopaddr |= data<<16;
			break;
		case 9:
			slot->fs = data & 0x3;
			slot->bits = (data & 0x4) ? 12 : 8;
			slot->srcnote = (data >> 3) & 0x3;
			slot->srcb = (data >> 5) & 0x7;
			break;
	}
}

static void ymf271_timer_a_tick(int num)
{
	YMF271Chip *chip;
	
	chip = &YMF271[num];	

	chip->status |= 1;

	if (chip->enable & 4)
	{
		chip->irqstate |= 1;
		if (chip->irq_callback) chip->irq_callback(1);
	}
}

static void ymf271_timer_b_tick(int num)
{
	YMF271Chip *chip;
	
	chip = &YMF271[num];	

	chip->status |= 2;

	if (chip->enable & 8)
	{
		chip->irqstate |= 2;
		if (chip->irq_callback) chip->irq_callback(1);
	}
}

static UINT8 ymf271_read_ext_memory(int chipnum, UINT32 address)
{
	if( YMF271[chipnum].ext_mem_read ) {
		return YMF271[chipnum].ext_mem_read(address);
	} else {
		if( address < 0x800000)
			return YMF271[chipnum].rom[address];
	}
	return 0xff;
}

static void ymf271_write_ext_memory(int chipnum, UINT32 address, UINT8 data)
{
	if( YMF271[chipnum].ext_mem_write ) {
		YMF271[chipnum].ext_mem_write(address, data);
	}
}

static void ymf271_write_timer(int chipnum, int data)
{
	int slotnum;
	YMF271Chip *chip;
	YMF271Group *group;
	double period;

	chip = &YMF271[chipnum];

	slotnum = fm_tab[chip->timerreg & 0xf];
	group = &chip->groups[slotnum];

	if ((chip->timerreg & 0xf0) == 0)
	{
		group->sync = data & 0x3;
		group->pfm = data >> 7;
	}
	else
	{
		switch (chip->timerreg)
		{
			case 0x10:
				chip->timerA &= ~0xff;
				chip->timerA |= data;
				break;

/*			case 0x11: */
/*				chip->timerA &= ~0x300; */
/*				chip->timerA |= (data & 0x3)<<8; */
/*				break; */

			case 0x12:
				chip->timerB = data;
				break;

			case 0x13:
				if (data & 1)
				{	/* timer A load*/
					chip->timerAVal = chip->timerA;
				}
				if (data & 2)
				{	/* timer B load*/
					chip->timerBVal = chip->timerB;
				}
				if (data & 4)
				{
					/* timer A IRQ enable*/
					chip->enable |= 4;
				}
				if (data & 8)
				{
					/* timer B IRQ enable*/
					chip->enable |= 8;
				}
				if (data & 0x10)
				{	/* timer A reset*/
					chip->irqstate &= ~1;
					chip->status &= ~1;

					if (chip->irq_callback) chip->irq_callback(0);

					period = (double)(256.0 - chip->timerAVal ) * ( 384.0 * 4.0 / (double)CLOCK);

					timer_adjust(chip->timA, TIME_IN_SEC(period), chipnum, TIME_IN_SEC(period));
				}
				if (data & 0x20)
				{	/* timer B reset*/
					chip->irqstate &= ~2;
					chip->status &= ~2;

					if (chip->irq_callback) chip->irq_callback(0);

					period = 6144.0 * (256.0 - (double)chip->timerBVal) / (double)CLOCK;

					timer_adjust(chip->timB, TIME_IN_SEC(period), chipnum, TIME_IN_SEC(period));
				}
				break;
			case 0x14:
				chip->ext_address &= ~0xff;
				chip->ext_address |= data;
				break;
			case 0x15:
				chip->ext_address &= ~0xff00;
				chip->ext_address |= data << 8;
				break;
			case 0x16:
				chip->ext_address &= ~0xff0000;
				chip->ext_address |= (data & 0x7f) << 16;
				chip->ext_read = (data & 0x80) ? 1 : 0;
				if( !chip->ext_read )
					chip->ext_address = (chip->ext_address + 1) & 0x7fffff;
				break;
			case 0x17:
				ymf271_write_ext_memory( chipnum, chip->ext_address, data );
				chip->ext_address = (chip->ext_address + 1) & 0x7fffff;
				break;
		}
	}
}

static void ymf271_w(int chipnum, int offset, int data)
{
	YMF271Chip *chip = &YMF271[chipnum];

	switch (offset)
	{
		case 0:
			chip->reg0 = data;
			break;
		case 1:
			ymf271_write_fm(chip, 0, chip->reg0, data);
			break;
		case 2:
			chip->reg1 = data;
			break;
		case 3:
			ymf271_write_fm(chip, 1, chip->reg1, data);
			break;
		case 4:
			chip->reg2 = data;
			break;
		case 5:
			ymf271_write_fm(chip, 2, chip->reg2, data);
			break;
		case 6:
			chip->reg3 = data;
			break;
		case 7:
			ymf271_write_fm(chip, 3, chip->reg3, data);
			break;
		case 8:
			chip->pcmreg = data;
			break;
		case 9:
			ymf271_write_pcm(chip, data); 
			break;
		case 0xc:
			chip->timerreg = data;
			break;
		case 0xd:
			ymf271_write_timer(chipnum, data);
			break;
	}
}

static int ymf271_r(int chipnum, int offset)
{
	YMF271Chip *chip = &YMF271[chipnum];
	UINT8 value;
	switch(offset)
	{
		case 0:
			return chip->status;

		case 2:
			value = ymf271_read_ext_memory( chipnum, chip->ext_address );
			chip->ext_address = (chip->ext_address + 1) & 0x7fffff;
			return value;
	}

	return 0;
}

static void ymf271_init(int i, UINT8 *rom, void (*cb)(int), read8_handler ext_read, write8_handler ext_write)
{
	memset(&YMF271[i], 0, sizeof(YMF271Chip));

	YMF271[i].timA = timer_alloc(ymf271_timer_a_tick);
	YMF271[i].timB = timer_alloc(ymf271_timer_b_tick);
	
	YMF271[i].rom = rom;
	YMF271[i].irq_callback = cb;
	
	YMF271[i].ext_mem_read = ext_read;
	YMF271[i].ext_mem_write = ext_write;
}

int YMF271_sh_start( const struct MachineSound *msound )
{
	char buf[2][40];
	const char *name[2];
	int  vol[2];
	struct YMF271interface *intf;
	int i;

	intf = msound->sound_interface;

	for(i=0; i<intf->num; i++)
	{
		sprintf(buf[0], "YMF271 %d L", i);
		sprintf(buf[1], "YMF271 %d R", i);
		name[0] = buf[0];
		name[1] = buf[1];
		vol[0]=intf->mixing_level[i] >> 16;
		vol[1]=intf->mixing_level[i] & 0xffff;
ymf271_init(i, memory_region(intf->region[0]), intf->irq_callback[i], intf->ext_read[i], intf->ext_write[i]);
		stream_init_multi(2, name, vol, Machine->sample_rate, i, ymf271_pcm_update);
	}

	/* Volume table, 1 = -0.375dB, 8 = -3dB, 256 = -96dB*/
	for(i = 0; i < 256; i++)
		volume[i] = 65536*pow(2.0, (-0.375/6)*i);
	for(i = 256; i < 256*4; i++)
		volume[i] = 0;

	return 0;
}

void YMF271_sh_stop( void )
{
}

READ_HANDLER( YMF271_0_r )
{
	return ymf271_r(0, offset);
}

WRITE_HANDLER( YMF271_0_w )
{
	ymf271_w(0, offset, data);
}

READ_HANDLER( YMF271_1_r )
{
	return ymf271_r(1, offset);
}

WRITE_HANDLER( YMF271_1_w )
{
	ymf271_w(1, offset, data);
}
