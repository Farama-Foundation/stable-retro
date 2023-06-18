/*
 * Sega System 32 Multi/Model 1 custom PCM chip (315-5560) emulation.
 *
 * by R. Belmont.  Info from AMUSE, Hoot, and the YMF278B (OPL4).
 * This chip is sort of a dry run for the PCM section of the YMF278B,
 * and it has many obvious similarities to that chip.
 *
 * voice registers:
 * 0: pan
 * 1: sample to play (PCM chip uses table to figure out)
 * 2: LSB of pitch
 * 3: MSB of pitch
 * 4: voice control: top bit = 1 for key on, 0 for key off
 * 5: bit 0: loop, bits 1-7 = volume attenuate (0=max, 7f=min)
 * 6: LFO (OutRunners engine, singing man in Daytona)
 * 7: LFO
 *
 * The first sample ROM contains a variable length table with 12
 * bytes per instrument/sample.  The end of the table is marked
 * by 12 bytes of 0xFF.  This is very similar to the YMF278B.
 *
 * The first 3 bytes are the offset into the file (big endian).
 * The next 2 are the loop start offset into the file (big endian)
 * The next 2 are the 2's complement of the total sample size (big endian)
 * The next byte is unknown.
 * The next 3 are envelope attack / decay / release parameters (not yet emulated)
 *
 */

#include <string.h>
#include <stdlib.h>
#include <math.h>
#include "driver.h"
#include "cpuintrf.h"
#include "multipcm.h"
#include "state.h"

#define MULTIPCM_CLOCKDIV    	(360.0)
#define MULTIPCM_ONE		(18)

static int ctbl[] =
{
	0, 1, 2, 3, 4, 5, 6 , -1,	/* voice number mapping*/
	7, 8, 9, 10,11,12,13, -1,
	14,15,16,17,18,19,20, -1,
	21,22,23,24,25,26,27, -1,
};

static int decaytbl[16] =	/* decay times*/
{
     0,   300,   800,  1400,
  2100,  3000,  4000,  5200,
  6600,  8200, 10000, 12000,
 14500, 17500, 21000, 25000
};

static long voltbl[128];	/* pre-calculated volume table*/
static long pantbl[16];		/* pre-calculated panning table*/

/* sample info struct*/
typedef struct PCM_t
{
	INT32	st;
	INT32	size;
	INT32	loop;
	UINT8	env[4];
} PCMInfoT;

/* voice structure*/
typedef struct Voice_t
{
	INT8    active;		/* active flag*/
	INT8	loop;	        /* loop flag*/
	INT32   end;		/* length of sample*/
	INT32	loopst;		/* loop start offset*/
	int     pan;		/* panning*/
	INT32 	vol;		/* volume*/
	INT8    *pSamp;		/* pointer to start of sample data*/

	INT32	ptdelta;	/* pitch step*/
	INT32	ptoffset;	/* fixed point offset*/
	INT32	ptsum;		/* fixed point sum*/
  	int 	relamt;		/* release amount*/
  	int 	relcount;	/* release counter*/
	INT8	relstage;	/* release stage*/
} VoiceT;

/* chip structure*/
typedef struct MultiPCM_t
{
	unsigned char registers[28][8];	/* 8 registers per voice?*/
	int type;			/* chip type: 0=model1, 1=multi32*/
	int bankL, bankR;
	long banksize;

	VoiceT Voices[28];
	int curreg, curvoice;
	signed char *romptr;
	double freq_ratio;

	long dlttbl[0x1001];		/* pre-calculated pitch table*/

	PCMInfoT samples[512];

} MultiPCMT;

static MultiPCMT mpcm[MAX_MULTIPCM];

static void MultiPCM_postload(void)
{
	int i, j, inum;

	/* recalculate all the proper voice pointers*/
	for (i = 0; i < 2; i++)
	{
		for (j = 0; j < 28; j++)
		{
			inum = mpcm[i].registers[j][1] | ((mpcm[i].registers[j][2]&0x1)<<8);
		  	mpcm[i].Voices[j].pSamp = &mpcm[i].romptr[mpcm[i].samples[inum].st];
		}
	}
}

static void MultiPCM_update(int chip, INT16 **buffer, int length )
{
	INT16  *datap[2];
	int i, j;
	signed long lvol, rvol, mlvol, mrvol;
	signed char *pSamp;
	long	cnt, ptsum, ptoffset, ptdelta, end;
	VoiceT	*vptr;
	float decTemp;
	char relstage;
	int relcount, relamt;
	float invrelamt;

	datap[0] = buffer[0];
	datap[1] = buffer[1];

	memset(datap[0], 0, sizeof(short)*length);
	memset(datap[1], 0, sizeof(short)*length);

	for (j = 0; j < 28; j++)
	{
		vptr = &mpcm[chip].Voices[j];

		/* is voice playing?*/
		if ((vptr->active) || (vptr->relstage))
		{	/* only calculate volume once per voice per update cycle*/
			rvol = pantbl[vptr->pan];
			lvol = pantbl[15-vptr->pan];

			mrvol = rvol = (rvol * vptr->vol)>>8;
			mlvol = lvol = (lvol * vptr->vol)>>8;

			decTemp = 1.0f;

			/* copy our "working set" into locals*/
			ptsum = vptr->ptsum;
			ptoffset = vptr->ptoffset;
			ptdelta = vptr->ptdelta;
			end = vptr->end;
			pSamp = vptr->pSamp;
			relstage = vptr->relstage;
			relcount = vptr->relcount;
			relamt = vptr->relamt;
			invrelamt = 1.f / (float)relamt;

			for (i = 0; i < length; i++)
			{
				cnt = ptsum >> MULTIPCM_ONE;
				ptsum &= ((1<<MULTIPCM_ONE)-1);
				ptoffset += cnt;

				if (ptoffset >= end)
				{
					if (vptr->loop)
					{
						ptoffset = vptr->loopst;
					}
					else
					{
						vptr->active = 0;
						i = length;
						continue;
					}
				}

				if (relstage)
				{
					relcount++;
					if (relcount > relamt)
					{
						relstage = 0;
						vptr->relstage = 0;
					}

					decTemp = 1.0f - (relcount * invrelamt);

					lvol = mlvol * decTemp;
					rvol = mrvol * decTemp;
				}

				ptsum += ptdelta;

				datap[0][i] += ((pSamp[ptoffset] * lvol)>>2);
				datap[1][i] += ((pSamp[ptoffset] * rvol)>>2);
			}

			/* copy back the working values we need to keep*/
			vptr->ptsum = ptsum;
			vptr->ptoffset = ptoffset;
			vptr->relcount = relcount;
		}
	}
}

int MultiPCM_sh_start(const struct MachineSound *msound)
{
	int i, j, chip;
	double unity = (double)(1<<MULTIPCM_ONE);
	unsigned char* phdr;
	long nowadrs;
	long idx;
	char buf[2][40];
	const char *name[2];
	int vol[2];
	struct MultiPCM_interface *intf = msound->sound_interface;

	/* make volume table*/
	double	max=255.0;
	double	db=(48.0/128);

	for (i = 0; i < 128; i++)
	{
		voltbl[i]=max;
		max /= pow(10.0,db/20.0);
	}
	voltbl[127]=0;

	/* make pan table*/
	for(i=0; i<16; i++)
	{
		pantbl[i]=(long)( (255/sqrt(15)) * sqrt(i));
	}

	for (chip = 0; chip < intf->chips; chip++)
	{
		mpcm[chip].type = intf->type[chip];
		mpcm[chip].banksize = intf->banksize[chip];

		mpcm[chip].curreg = mpcm[chip].curvoice = 0;

		mpcm[chip].romptr = (signed char *)memory_region(intf->region[chip]);

		mpcm[chip].freq_ratio = ((float)intf->clock[chip] / (float)MULTIPCM_CLOCKDIV) / (float)Machine->sample_rate;

		for (i = 0; i < 28; i++)
		{
			mpcm[chip].Voices[i].active = 0;
			mpcm[chip].Voices[i].ptsum = 0;
			mpcm[chip].Voices[i].ptoffset = 0;
			mpcm[chip].Voices[i].loop = 0;
			mpcm[chip].Voices[i].loopst = 0;
			mpcm[chip].Voices[i].end = 0;
			mpcm[chip].Voices[i].pan = 0;
			mpcm[chip].Voices[i].vol = 0;
			mpcm[chip].Voices[i].relamt = 0;
			mpcm[chip].Voices[i].relcount = 0;
			mpcm[chip].Voices[i].relstage = 0;
		}

		sprintf(buf[0], "%s %d L", sound_name(msound), chip);
		sprintf(buf[1], "%s %d R", sound_name(msound), chip);
		name[0] = buf[0];
		name[1] = buf[1];
		vol[0]=intf->mixing_level[chip] >> 16;
		vol[1]=intf->mixing_level[chip] & 0xffff;
		stream_init_multi(2, name, vol, Machine->sample_rate, chip, MultiPCM_update);

		/* make pitch delta table (1 octave)*/
		for(i=0; i<0x1001; i++)
		{
			mpcm[chip].dlttbl[i] = (long)(mpcm[chip].freq_ratio * unity * (1.0 + ((double)i / 4096.0)));
		}

		/* precalculate the PCM data for a small speedup*/
		phdr = (unsigned char *)mpcm[chip].romptr;
		for(i = 0; i < 511; i++)
		{
			idx = i*12;
			nowadrs = (phdr[idx + 0]<<16) + (phdr[idx + 1]<<8) + (phdr[idx + 2]);

			if((nowadrs == 0)||(nowadrs==0xffffff))
			{	/* invalid entry*/
				mpcm[chip].samples[i].st=0;
				mpcm[chip].samples[i].size=0;
			}
			else
			{
				mpcm[chip].samples[i].st = nowadrs;
				mpcm[chip].samples[i].loop = (phdr[idx + 3]<<8) + (phdr[idx + 4]);
				mpcm[chip].samples[i].size = 0xffff - ((phdr[idx + 5]<<8) + (phdr[idx + 6]));
				mpcm[chip].samples[i].env[0] = phdr[idx + 8];
				mpcm[chip].samples[i].env[1] = phdr[idx + 9];
				mpcm[chip].samples[i].env[2] = phdr[idx + 10];
			}
		}
	}

	/* set up the save state info */
	for (i = 0; i < MAX_MULTIPCM; i++)
	{
		int v;
		char mname[20];

		sprintf(mname, "MultiPCM %d", i);

		state_save_register_int(mname, i, "bankL", &mpcm[i].bankL);
		state_save_register_int(mname, i, "bankR", &mpcm[i].bankR);

		for (v = 0; v < 28; v++)
		{
			char mname2[32];

			sprintf(mname2, "MultiPCM %d v %d", i, v);

			for (j = 0; j < 8; j++)
			{
				char sname[20];

				sprintf(sname, "rawreg %d", j);

				state_save_register_UINT8(mname2, 1, sname, &mpcm[i].registers[v][j], 1);
			}
			state_save_register_INT8(mname2, 1, "active", &mpcm[i].Voices[v].active, 1);
			state_save_register_INT8(mname2, 1, "loop", &mpcm[i].Voices[v].loop, 1);
			state_save_register_INT32(mname2, 1, "end", &mpcm[i].Voices[v].end, 1);
			state_save_register_INT32(mname2, 1, "lpstart", &mpcm[i].Voices[v].loopst, 1);
			state_save_register_int(mname2, 1, "pan", &mpcm[i].Voices[v].pan);
			state_save_register_INT32(mname2, 1, "vol", &mpcm[i].Voices[v].vol, 1);
			state_save_register_INT32(mname2, 1, "ptdelta", &mpcm[i].Voices[v].ptdelta, 1);
			state_save_register_INT32(mname2, 1, "ptoffset", &mpcm[i].Voices[v].ptoffset, 1);
			state_save_register_INT32(mname2, 1, "ptsum", &mpcm[i].Voices[v].ptsum, 1);
			state_save_register_int(mname2, 1, "relamt", &mpcm[i].Voices[v].relamt);
			state_save_register_INT8(mname2, 1, "relstage", &mpcm[i].Voices[v].relstage, 1);
		}

		state_save_register_int(mname, i, "curreg", &mpcm[i].curreg);
		state_save_register_int(mname, i, "curvoice", &mpcm[i].curvoice);
	}

	state_save_register_func_postload(MultiPCM_postload);

	return 0;
}

void MultiPCM_sh_stop(void)
{
}

/* write register */
static void MultiPCM_reg_w(int chip, int offset, unsigned char data)
{
	int ppp, inum;
	signed short pitch;
	long pt_abs, pt_oct, st;
	int vnum;
	MultiPCMT *cptr;
	VoiceT *vptr;

	cptr = &mpcm[chip];

	switch (offset)
	{
		case 0:	      	/* data / status*/
			if ((cptr->curvoice > 27) || (cptr->curvoice < 0))
			{
				/*logerror("MPCM: unknown write to voice > 28\n");*/
				return;
			}

			vnum = mpcm[chip].curvoice;
			cptr->registers[vnum][cptr->curreg] = data;

			vptr = &mpcm[chip].Voices[vnum];

			switch (cptr->curreg)
			{
			 	case 0:	/* panning*/
					ppp = (cptr->registers[vnum][0]>>4)&0xf;
					if (ppp >= 8)
					{
						ppp = -(16-ppp);
					}
					vptr->pan = ppp + 8;
					break;

				case 1:	/* sample*/
					break;

				case 2:	/* pitch LSB*/
				/* MUST fall through to update pitch also!*/
				case 3: /* pitch MSB*/
					/* compute frequency divisor*/
					pitch = (cptr->registers[vnum][3]<<8) + cptr->registers[vnum][2];
					pt_abs = (double)abs(pitch);
					pt_oct = pt_abs>>12;
					if(pitch < 0)
					{
						vptr->ptdelta = cptr->dlttbl[0x1000 - (pt_abs&0xfff)];
						vptr->ptdelta >>= (pt_oct+1);
					}
					else
					{
						vptr->ptdelta = cptr->dlttbl[pt_abs&0xfff];
						vptr->ptdelta <<= pt_oct;
					}
					break;

				case 4:	/* key on/off*/
					if (data & 0x80)
					{
						inum = cptr->registers[vnum][1];

						/* calc decay amount*/
						vptr->relamt = decaytbl[(0x0f - cptr->samples[inum].env[2])];

						/* compute start and end pointers*/
						st = cptr->samples[inum].st;

						/* perform banking*/
						if (st >= 0x100000)
						{
							if (cptr->type == 1)	/* multiPCM*/
							{
								log_cb(RETRO_LOG_DEBUG, LOGPRE "MPCM: key on chip %d voice %d\n", chip, vnum);
								log_cb(RETRO_LOG_DEBUG, LOGPRE "regs %02x %02x %02x %02x %02x %02x %02x %02x\n", cptr->registers[vnum][0],
									cptr->registers[vnum][1],cptr->registers[vnum][2],cptr->registers[vnum][3],
									cptr->registers[vnum][4],cptr->registers[vnum][5],
									cptr->registers[vnum][6],cptr->registers[vnum][7]);

								if (vptr->pan < 8)
								{
									st = (st & 0xfffff) + cptr->banksize * cptr->bankL;
								}
								else
								{
									st = (st & 0xfffff) + cptr->banksize * cptr->bankR;
								}
							}
							else	/* model 1*/
							{
								st = (st & 0xfffff) + cptr->banksize * cptr->bankL;
							}
						}

						vptr->pSamp = &cptr->romptr[st];
						vptr->end = cptr->samples[inum].size;
						vptr->loopst = cptr->samples[inum].loop;

						vptr->ptoffset = 0;
						vptr->ptsum = 0;
						vptr->active = 1;
						vptr->relstage = 0;
					}
					else
					{
						log_cb(RETRO_LOG_DEBUG, LOGPRE "MPCM: key off chip %d voice %d\n", chip, vnum);
						vptr->active = 0;
						vptr->relcount = 0;
						if ((vptr->loop) && (vptr->pSamp))
						{
							vptr->relstage = 1;
						}
					}
				 	break;

				case 5:	/* volume/loop*/
					vptr->vol = voltbl[(cptr->registers[vnum][5]>>1)&0x7f];
					vptr->loop = (cptr->registers[vnum][5]&0x1) || !vptr->loopst;
					break;

				case 6: /* ??? LFO? reverb?*/
				case 7:
					log_cb(RETRO_LOG_DEBUG, LOGPRE "write %x to reg %d, voice %d\n", data, cptr->curreg, vnum);
					break;

				default:
					log_cb(RETRO_LOG_DEBUG, LOGPRE "write %x to reg %d, voice %d\n", data, cptr->curreg, vnum);
					break;
			}
			break;

		case 1:		/* voice select*/
			cptr->curvoice = ctbl[data&0x1f];
			break;

		case 2:		/* register select*/
			cptr->curreg = data;
			if (cptr->curreg > 7)
				cptr->curreg = 7;
			break;
	}
}

/* read register */

static unsigned char MultiPCM_reg_r(int chip, int offset)
{
	unsigned char retval = 0;

	switch (offset)
	{
	case 0:
		retval = 0;	/* always return READY*/
		break;

	default:
		/*logerror("read from unknown MPCM register %ld\n", offset);*/
		break;
	}

	return retval;
}

/* MAME/M1 access functions */

READ_HANDLER( MultiPCM_reg_0_r )
{
	return MultiPCM_reg_r(0, offset);
}

WRITE_HANDLER( MultiPCM_reg_0_w )
{
	MultiPCM_reg_w(0, offset, data);
}

READ_HANDLER( MultiPCM_reg_1_r )
{
	return MultiPCM_reg_r(1, offset);
}

WRITE_HANDLER( MultiPCM_reg_1_w )
{
	MultiPCM_reg_w(1, offset, data);
}

WRITE_HANDLER( MultiPCM_bank_0_w )
{
	if (mpcm[0].type == MULTIPCM_MODE_STADCROSS)	/* multi32 with mono bankswitching GAL*/
	{
		mpcm[0].bankL = mpcm[0].bankR = (data&0x7);
	}
	else if (mpcm[0].type == MULTIPCM_MODE_MULTI32)	/* multi32*/
	{
		mpcm[0].bankL = (data>>3)&0x7;
		mpcm[0].bankR = data & 0x7;
	}
	else
	{
		mpcm[0].bankL = data&0x1f;
	}
}

WRITE_HANDLER( MultiPCM_bank_1_w )
{
	if (mpcm[1].type == MULTIPCM_MODE_STADCROSS)	/* multi32 with mono bankswitching GAL*/
	{
		mpcm[1].bankL = mpcm[1].bankR = (data&0x7);
	}
	else if (mpcm[1].type == MULTIPCM_MODE_MULTI32)	/* multi32*/
	{
		mpcm[1].bankL = (data>>3)&0x7;
		mpcm[1].bankR = data & 0x7;
	}
	else
	{
		mpcm[1].bankL = data&0x1f;
	}
}
