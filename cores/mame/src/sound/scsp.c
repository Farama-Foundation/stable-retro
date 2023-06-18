/*
	Sega/Yamaha YMF292-F (SCSP = Saturn Custom Sound Processor) emulation
	By ElSemi
	MAME/M1 conversion and cleanup by R. Belmont

	This chip has 32 voices.  Each voice can play a sample or be part of
	an FM construct.  Unlike traditional Yamaha FM chips, the base waveform
	for the FM still comes from the wavetable RAM.

	Unsupported:
		- FM mode (VF3 uses it, Hanagumi might late in the title song...)
		- on-board programmable DSP and related functionality

	ChangeLog:
	* November 25, 2003 (ES) Fixed buggy timers and envelope overflows.
                            (RB) Improved sample rates other than 44100, multiple
	             		 chips now works properly.
	* December 02, 2003 (ES) Added DISDL register support, improves mix.
*/

#include <math.h>
#include <string.h>
#include "driver.h"
#include "cpuintrf.h"

static signed short *bufferl;
static signed short *bufferr;

static int length;

signed int *buffertmpl,*buffertmpr;

static data32_t IrqTimA=0;
static data32_t IrqTimBC=0;
static data32_t IrqMidi=0;

static data8_t MidiOutW=0,MidiOutR=0;
static data8_t MidiStack[8];
static data8_t MidiW=0,MidiR=0;

static data32_t FNS_Table[0x400];

static float SDLT[8]={-1000000.0,-36.0,-30.0,-24.0,-18.0,-12.0,-6.0,0.0};
static int LPANTABLE[0x10000];
static int RPANTABLE[0x10000];

static int TimPris[3];
static int TimCnt[3];

/* DMA stuff*/
static UINT32 scsp_dmea;
static UINT16 scsp_drga;
#define	scsp_dgate		scsp_regs[0x416/2] & 0x4000
#define	scsp_ddir		scsp_regs[0x416/2] & 0x2000
#define scsp_dexe 		scsp_regs[0x416/2] & 0x1000
#define dma_transfer_end	((scsp_regs[0x424/2] & 0x10)>>4)|(((scsp_regs[0x426/2] & 0x10)>>4)<<1)|(((scsp_regs[0x428/2] & 0x10)>>4)<<2)
static UINT16 scsp_dtlg;
static void dma_scsp(void); 		/*SCSP DMA transfer function*/

#define SHIFT	12
#define FIX(v)	((data32_t) ((float) (1<<SHIFT)*(v)))


#define EG_SHIFT	8

/* include the LFO handling code*/
#include "scsplfo.c"

/*
	SCSP features 32 programmable slots
	that can generate FM and PCM (from ROM/RAM) sound
*/

/*SLOT PARAMETERS*/
#define KEYONEX(slot)		((slot->udata.data[0x0]>>0x0)&0x1000)
#define KEYONB(slot)		((slot->udata.data[0x0]>>0x0)&0x0800)
#define SBCTL(slot)		((slot->udata.data[0x0]>>0x9)&0x0003)
#define SSCTL(slot)		((slot->udata.data[0x0]>>0x7)&0x0003)
#define LPCTL(slot)		((slot->udata.data[0x0]>>0x5)&0x0003)
#define PCM8B(slot)		((slot->udata.data[0x0]>>0x0)&0x0010)

#define SA(slot)		(((slot->udata.data[0x0]&0xF)<<16)|(slot->udata.data[0x1]))

#define LSA(slot)		(slot->udata.data[0x2])

#define LEA(slot)		(slot->udata.data[0x3])

#define D2R(slot)		((slot->udata.data[0x4]>>0xB)&0x001F)
#define D1R(slot)		((slot->udata.data[0x4]>>0x6)&0x001F)
#define EGHOLD(slot)		((slot->udata.data[0x4]>>0x0)&0x0020)
#define AR(slot)		((slot->udata.data[0x4]>>0x0)&0x001F)

#define LPSLNK(slot)		((slot->udata.data[0x5]>>0x0)&0x4000)
#define KRS(slot)		((slot->udata.data[0x5]>>0xA)&0x000F)
#define DL(slot)		((slot->udata.data[0x5]>>0x5)&0x001F)
#define RR(slot)		((slot->udata.data[0x5]>>0x0)&0x001F)

#define STWINH(slot)		((slot->udata.data[0x6]>>0x0)&0x0200)
#define SDIR(slot)		((slot->udata.data[0x6]>>0x0)&0x0100)
#define TL(slot)		((slot->udata.data[0x6]>>0x0)&0x00FF)

#define MDL(slot)		((slot->udata.data[0x7]>>0xB)&0x0007)
#define MDXSL(slot)		((slot->udata.data[0x7]>>0x6)&0x003F)
#define MDYSL(slot)		((slot->udata.data[0x7]>>0x0)&0x003F)

#define OCT(slot)		((slot->udata.data[0x8]>>0xB)&0x000F)
#define FNS(slot)		((slot->udata.data[0x8]>>0x0)&0x03FF)

#define LFORE(slot)		((slot->udata.data[0x9]>>0x0)&0x8000)
#define LFOF(slot)		((slot->udata.data[0x9]>>0xA)&0x001F)
#define PLFOWS(slot)		((slot->udata.data[0x9]>>0x8)&0x0003)
#define PLFOS(slot)		((slot->udata.data[0x9]>>0x5)&0x0007)
#define ALFOWS(slot)		((slot->udata.data[0x9]>>0x3)&0x0003)
#define ALFOS(slot)		((slot->udata.data[0x9]>>0x0)&0x0007)

#define ISEL(slot)		((slot->udata.data[0xA]>>0x3)&0x000F)
#define IMXL(slot)		((slot->udata.data[0xA]>>0x0)&0x0007)

#define DISDL(slot)		((slot->udata.data[0xB]>>0xD)&0x0007)
#define DIPAN(slot)		((slot->udata.data[0xB]>>0x8)&0x001F)
#define EFSDL(slot)		((slot->udata.data[0xB]>>0x5)&0x0007)
#define EFPAN(slot)		((slot->udata.data[0xB]>>0x0)&0x001F)

static int ARTABLE[64],DRTABLE[64];
static double BaseTimes2[32]={7000,6500,6222.95,4978.37,4148.66,3556.01,3111.47,2489.21,2074.33,1778.00,1555.74,1244.63,1037.19,889.02,
					   777.87,622.31,518.59,444.54,388.93,311.16,259.32,222.27,194.47,155.60,129.66,111.16,97.23,77.82,64.85,55.60};
#define AR2DR	14.304187

typedef enum {ATTACK,DECAY1,DECAY2,RELEASE} _STATE;
struct _EG
{
	int volume;	/**/
	_STATE state;
	int step;
	/*step vals*/
	int AR;		/*Attack*/
	int D1R;	/*Decay1*/
	int D2R;	/*Decay2*/
	int RR;		/*Release*/

	int DL;		/*Decay level*/
	data8_t EGHOLD;
	data8_t LPLINK;
};

struct _SLOT
{
	union
	{
		data16_t data[0x10];	/*only 0x1a bytes used*/
		data8_t datab[0x20];
	} udata;
	data8_t active;	/*this slot is currently playing*/
	data8_t *base;		/*samples base address*/
	data32_t cur_addr;	/*current play address (24.8)*/
	data32_t step;		/*pitch step (24.8)*/
	struct _EG EG;			/*Envelope*/
	struct _LFO PLFO;		/*Phase LFO*/
	struct _LFO ALFO;		/*Amplitude LFO*/
	int slot;
};


#define MEM4B(scsp)		((scsp->udata.data[0]>>0x0)&0x0200)
#define DAC18B(scsp)		((scsp->udata.data[0]>>0x0)&0x0100)
#define MVOL(scsp)		((scsp->udata.data[0]>>0x0)&0x000F)
#define RBL(scsp)		((scsp->udata.data[1]>>0x7)&0x0003)
#define RBP(scsp)		((scsp->udata.data[1]>>0x0)&0x003F)
#define MOFULL(scsp)   		((scsp->udata.data[2]>>0x0)&0x1000)
#define MOEMPTY(scsp)		((scsp->udata.data[2]>>0x0)&0x0800)
#define MIOVF(scsp)		((scsp->udata.data[2]>>0x0)&0x0400)
#define MIFULL(scsp)		((scsp->udata.data[2]>>0x0)&0x0200)
#define MIEMPTY(scsp)		((scsp->udata.data[2]>>0x0)&0x0100)

#define SCILV0(scsp)    	((scsp->udata.data[0x24/2]>>0x0)&0xff)
#define SCILV1(scsp)    	((scsp->udata.data[0x26/2]>>0x0)&0xff)
#define SCILV2(scsp)    	((scsp->udata.data[0x28/2]>>0x0)&0xff)

#define SCIEX0	0
#define SCIEX1	1
#define SCIEX2	2
#define SCIMID	3
#define SCIDMA	4
#define SCIIRQ	5
#define SCITMA	6
#define SCITMB	7

struct _SCSP
{
	union
	{
		data16_t data[0x30/2];
		data8_t datab[0x30];
	} udata;
	struct _SLOT Slots[32];
	unsigned char *SCSPRAM;
	char Master;
	void (*Int68kCB)(int irq);
	int stream;
} SCSPs[MAX_SCSP],*SCSP=SCSPs;

static unsigned char DecodeSCI(unsigned char irq)
{
	unsigned char SCI=0;
	unsigned char v;
	v=(SCILV0((SCSP))&(1<<irq))?1:0;
	SCI|=v;
	v=(SCILV1((SCSP))&(1<<irq))?1:0;
	SCI|=v<<1;
	v=(SCILV2((SCSP))&(1<<irq))?1:0;
	SCI|=v<<2;
	return SCI;
}

void CheckPendingIRQ(void)
{
	data32_t pend=SCSP[0].udata.data[0x20/2];
	data32_t en=SCSP[0].udata.data[0x1e/2];
	if(MidiW!=MidiR)
	{
		SCSP[0].Int68kCB(IrqMidi);
		return;
	}
	if(!pend)
		return;
	if(pend&0x40)
		if(en&0x40)
		{
			SCSP[0].Int68kCB(IrqTimA);
			return;
		}
	if(pend&0x80)
		if(en&0x80)
		{
			SCSP[0].Int68kCB(IrqTimBC);
			return;
		}
	if(pend&0x100)
		if(en&0x100)
		{
			SCSP[0].Int68kCB(IrqTimBC);
			return;
		}

	SCSP[0].Int68kCB(0);
}

static int Get_AR(int base,int R)
{
	int Rate=base+(R<<1);
	if(Rate>63)	Rate=63;
	if(Rate<0) Rate=0;
	return ARTABLE[Rate];
}

static int Get_DR(int base,int R)
{
	int Rate=base+(R<<1);
	if(Rate>63)	Rate=63;
	if(Rate<0) Rate=0;
	return DRTABLE[Rate];
}

static int Get_RR(int base,int R)
{
	int Rate=base+(R<<1);
	if(Rate>63)	Rate=63;
	if(Rate<0) Rate=0;
	return ARTABLE[63-Rate];
}

static void Compute_EG(struct _SLOT *slot)
{
	int rate;

	rate=0;
	slot->EG.volume=0;
	slot->EG.AR=Get_AR(rate,AR(slot));
	slot->EG.D1R=Get_DR(rate,D1R(slot));
	slot->EG.D2R=Get_DR(rate,D2R(slot));
	slot->EG.RR=Get_RR(rate,RR(slot));
	slot->EG.DL=0x1f-DL(slot);
	slot->EG.EGHOLD=EGHOLD(slot);
}

static void SCSP_StopSlot(struct _SLOT *slot,int keyoff);

static int EG_Update(struct _SLOT *slot)
{
	switch(slot->EG.state)
	{
		case ATTACK:
			slot->EG.volume+=slot->EG.AR;
			if(slot->EG.volume>=(0x3ff<<EG_SHIFT))
			{
				slot->EG.state=DECAY1;
				if(slot->EG.D1R>=(1024<<EG_SHIFT)) /*Skip DECAY1, go directly to DECAY2*/
					slot->EG.state=DECAY2;
				slot->EG.volume=0x3ff<<EG_SHIFT;
			}
			if(slot->EG.EGHOLD)
				return 0x3ff<<(SHIFT-10);
			break;
		case DECAY1:
			slot->EG.volume-=slot->EG.D1R;
			if(slot->EG.volume>>(EG_SHIFT+5)>=slot->EG.DL)
				slot->EG.state=DECAY2;
			break;
		case DECAY2:
			if(D2R(slot)==0)
				return (slot->EG.volume>>EG_SHIFT)<<(SHIFT-10);
			slot->EG.volume-=slot->EG.D2R;
			if(slot->EG.volume<=0)
				slot->EG.volume=0;

			break;
		case RELEASE:
			slot->EG.volume-=slot->EG.RR;
			if(slot->EG.volume<=0)
			{
				SCSP_StopSlot(slot,0);
				slot->EG.volume=0;
				slot->EG.state=ATTACK;
			}
			break;
		default:
			return 1<<SHIFT;
	}
	return (slot->EG.volume>>EG_SHIFT)<<(SHIFT-10);
}

static data32_t SCSP_Step(struct _SLOT *slot)
{
	int octave=OCT(slot);
	int Fo=44100;
	int Fn;
	if(octave&8)
		Fo>>=(16-octave);
	else
		Fo<<=octave;
	Fn=Fo*(((FNS(slot))<<(SHIFT-10))|(1<<SHIFT));
	return Fn/(44100);
}

static void Compute_LFO(struct _SLOT *slot)
{
	if(PLFOS(slot)!=0)
		LFO_ComputeStep(&(slot->PLFO),LFOF(slot),PLFOWS(slot),PLFOS(slot),0);
	if(ALFOS(slot)!=0)
		LFO_ComputeStep(&(slot->ALFO),LFOF(slot),ALFOWS(slot),ALFOS(slot),1);
}

static void SCSP_StartSlot(struct _SLOT *slot)
{
	slot->active=1;
	slot->base=SCSP->SCSPRAM+SA(slot);
	slot->cur_addr=0;
	slot->step=SCSP_Step(slot);
	Compute_EG(slot);
	slot->EG.state=ATTACK;
	slot->EG.volume=0;
	Compute_LFO(slot);
}

static void SCSP_StopSlot(struct _SLOT *slot,int keyoff)
{
	slot->active=0;
	slot->udata.data[0]&=~0x800;
}

#define log2(n) (log((float) n)/log((float) 2))

static void SCSP_Init(int n, struct SCSPinterface *intf)
{
	int i;

	IrqTimA = IrqTimBC = IrqMidi = 0;
	MidiR=MidiW=0;
	MidiOutR=MidiOutW=0;

	/* get SCSP RAM*/
	for (i = 0; i < n; i++)
	{
		SCSP = &SCSPs[i];
		memset(SCSP,0,sizeof(SCSP));

		if (!i)
		{
			SCSP->Master=1;
		}
		else
		{
			SCSP->Master=0;
		}

		SCSPs[i].SCSPRAM = memory_region(intf->region[i]);
	}

	for(i=0;i<0x400;++i)
	{
		float fcent=1200.0*log2((float) ((1024.0+(float)i)/1024.0));
		fcent=pow(2.0,fcent/1200.0);
		FNS_Table[i]=(float) (1<<SHIFT) *fcent;
	}

	for(i=0;i<0x10000;++i)
	{
		int iTL =(i>>0x0)&0xff;
		int iPAN=(i>>0x8)&0x1f;
		int iSDL=(i>>0xD)&0x07;
		float TL=1.0;
		float SegaDB=0;
		float fSDL=1.0;
		float PAN=1.0;
		float LPAN,RPAN;

		if(iTL&0x01) SegaDB-=0.4;
		if(iTL&0x02) SegaDB-=0.8;
		if(iTL&0x04) SegaDB-=1.5;
		if(iTL&0x08) SegaDB-=3;
		if(iTL&0x10) SegaDB-=6;
		if(iTL&0x20) SegaDB-=12;
		if(iTL&0x40) SegaDB-=24;
		if(iTL&0x80) SegaDB-=48;

		TL=pow(10.0,SegaDB/20.0);

		SegaDB=0;
		if(iPAN&0x1) SegaDB-=3;
		if(iPAN&0x2) SegaDB-=6;
		if(iPAN&0x4) SegaDB-=12;
		if(iPAN&0x8) SegaDB-=24;

		if(iPAN==0xf) PAN=0.0;
		else PAN=pow(10.0,SegaDB/20.0);

		if(iPAN<0x10)
		{
			LPAN=PAN;
			RPAN=1.0;
		}
		else
		{
			RPAN=PAN;
			LPAN=1.0;
		}

		if(iSDL)
			fSDL=pow(10.0,(SDLT[iSDL])/20.0);
		else
			fSDL=0.0;

		LPANTABLE[i]=FIX((4.0*LPAN*TL*fSDL));
		RPANTABLE[i]=FIX((4.0*RPAN*TL*fSDL));
	}

	for(i=0;i<62;++i)
	{
		double t=BaseTimes2[i/2]/AR2DR;	/*In ms*/
		double step=(1023*1000.0)/((float) 44100*t);
		double scale=(double) (1<<EG_SHIFT);
		ARTABLE[i]=(int) (step*scale);
		step/=AR2DR;
		DRTABLE[i]=(int) (step*scale);
	}
	ARTABLE[62]=DRTABLE[62]=1024<<EG_SHIFT;
	ARTABLE[63]=DRTABLE[63]=1024<<EG_SHIFT;

	for(i=0;i<32;++i)
	{
		SCSPs[0].Slots[i].slot=i;
		SCSPs[1].Slots[i].slot=i;
	}

	LFO_Init();
	buffertmpl=(signed int*) auto_malloc(44100*sizeof(signed int));
	buffertmpr=(signed int*) auto_malloc(44100*sizeof(signed int));
	memset(buffertmpl,0,44100*sizeof(signed int));
	memset(buffertmpr,0,44100*sizeof(signed int));

	/* no "pend"*/
	SCSP[0].udata.data[0x20/2] = 0;
	SCSP[1].udata.data[0x20/2] = 0;
	TimCnt[0] = 0xffff;
	TimCnt[1] = 0xffff;
	TimCnt[2] = 0xffff;
}

static void SCSP_UpdateSlotReg(int s,int r)
{
	struct _SLOT *slot=SCSP->Slots+s;
	int sl;
	switch(r&0x3f)
	{
		case 0:
		case 1:
			if(KEYONEX(slot))
			{
				for(sl=0;sl<32;++sl)
				{
					struct _SLOT *s2=SCSP->Slots+sl;
					{
						if(KEYONB(s2) && !s2->active)
						{
							SCSP_StartSlot(s2);
						}
						if(!KEYONB(s2) && s2->active)
						{
							SCSP_StopSlot(s2,1);
						}
					}
				}
				slot->udata.data[0]&=~0x1000;
			}
			break;
		case 0x10:
		case 0x11:
			slot->step=SCSP_Step(slot);
			break;
		case 0xA:
		case 0xB:
			slot->EG.RR=Get_RR(0,RR(slot));
			slot->EG.DL=0x1f-DL(slot);
			break;
		case 0x12:
		case 0x13:
			Compute_LFO(slot);
			break;
	}
}

static void SCSP_UpdateReg(int reg)
{
	switch(reg&0x3f)
	{
		case 0x6:
		case 0x7:
			SCSP_MidiIn(0, SCSP->udata.data[0x6/2]&0xff, 0);
			break;
		case 0x12:
		case 0x13:
		case 0x14:
		case 0x15:
		case 0x16:
		case 0x17:
			break;
		case 0x18:
		case 0x19:
			if(SCSP->Master)
			{
				TimPris[0]=1<<((SCSP->udata.data[0x18/2]>>8)&0x7);
				TimCnt[0]=(SCSP->udata.data[0x18/2]&0xff)<<8;
			}
			break;
		case 0x1a:
		case 0x1b:
			if(SCSP->Master)
			{
				TimPris[1]=1<<((SCSP->udata.data[0x1A/2]>>8)&0x7);
				TimCnt[1]=(SCSP->udata.data[0x1A/2]&0xff)<<8;
			}
			break;
		case 0x1C:
		case 0x1D:
			if(SCSP->Master)
			{
				TimPris[2]=1<<((SCSP->udata.data[0x1C/2]>>8)&0x7);
				TimCnt[2]=(SCSP->udata.data[0x1C/2]&0xff)<<8;
			}
			break;
		case 0x22:	/*SCIRE*/
		case 0x23:

			if(SCSP->Master)
			{
				SCSP->udata.data[0x20/2]&=~SCSP->udata.data[0x22/2];
				CheckPendingIRQ();
			}
			break;
		case 0x24:
		case 0x25:
		case 0x26:
		case 0x27:
		case 0x28:
		case 0x29:
			if(SCSP->Master)
			{
				IrqTimA=DecodeSCI(SCITMA);
				IrqTimBC=DecodeSCI(SCITMB);
				IrqMidi=DecodeSCI(SCIMID);
			}
			break;
	}
}

static void SCSP_UpdateSlotRegR(int slot,int reg)
{

}

static void SCSP_UpdateRegR(int reg)
{
	switch(reg&0x3f)
	{
		case 4:
		case 5:
			{
				unsigned short v=SCSP->udata.data[0x5/2];
				v&=0xff00;
				v|=MidiStack[MidiR];
				if(MidiR!=MidiW)
				{
					++MidiR;
					MidiR&=7;
				}
				SCSP->udata.data[0x5/2]=v;
			}
			break;
		case 8:
		case 9:
			{
				unsigned char slot=SCSP->udata.data[0x8/2]>>11;
				unsigned int CA=SCSP->Slots[slot&0x1f].cur_addr>>(SHIFT+12);
				SCSP->udata.data[0x8/2]&=~(0x780);
				SCSP->udata.data[0x8/2]|=CA<<7;
			}
			break;
	}
}

static void SCSP_w16(unsigned int addr,unsigned short val)
{
	addr&=0xffff;
	if(addr<0x400)
	{
		int slot=addr/0x20;
		addr&=0x1f;
		*((unsigned short *) (SCSP->Slots[slot].udata.datab+(addr))) = val;
		if (Machine->sample_rate > 0)
		{
			SCSP_UpdateSlotReg(slot,addr&0x1f);
		}
	}
	else if(addr<0x600)
	{
		*((unsigned short *) (SCSP->udata.datab+((addr&0xff)))) = val;
		if (Machine->sample_rate > 0)
		{
			SCSP_UpdateReg(addr&0xff);
		}
	}
}

static unsigned short SCSP_r16(unsigned int addr)
{
	unsigned short v=0;
	addr&=0xffff;
	if(addr<0x400)
	{
		int slot=addr/0x20;
		addr&=0x1f;
		if (Machine->sample_rate > 0)
		{
			SCSP_UpdateSlotRegR(slot,addr&0x1f);
		}
		v=*((unsigned short *) (SCSP->Slots[slot].udata.datab+(addr)));
	}
	else if(addr<0x600)
	{
		if (Machine->sample_rate > 0)
		{
			SCSP_UpdateRegR(addr&0xff);
		}
		v= *((unsigned short *) (SCSP->udata.datab+((addr&0xff))));
	}
	return v;
}


#define REVSIGN(v) ((~v)+1)

void SCSP_TimersAddTicks(int ticks)
{
	if(TimCnt[0]<=0xff00)
	{
 		TimCnt[0] += ticks << (8-((SCSPs[0].udata.data[0x18/2]>>8)&0x7));
		if (TimCnt[0] > 0xFF00)
		{
			TimCnt[0] = 0xFFFF;
			SCSPs[0].udata.data[0x20/2]|=0x40;
		}
		SCSPs[0].udata.data[0x18/2]&=0xff00;
		SCSPs[0].udata.data[0x18/2]|=TimCnt[0]>>8;
	}

	if(TimCnt[1]<=0xff00)
	{
		TimCnt[1] += ticks << (8-((SCSPs[0].udata.data[0x1a/2]>>8)&0x7));
		if (TimCnt[1] > 0xFF00)
		{
			TimCnt[1] = 0xFFFF;
			SCSPs[0].udata.data[0x20/2]|=0x80;
		}
		SCSPs[0].udata.data[0x1a/2]&=0xff00;
		SCSPs[0].udata.data[0x1a/2]|=TimCnt[1]>>8;
	}

	if(TimCnt[2]<=0xff00)
	{
		TimCnt[2] += ticks << (8-((SCSPs[0].udata.data[0x1c/2]>>8)&0x7));
		if (TimCnt[2] > 0xFF00)
		{
			TimCnt[2] = 0xFFFF;
			SCSPs[0].udata.data[0x20/2]|=0x100;
		}
		SCSPs[0].udata.data[0x1c/2]&=0xff00;
		SCSPs[0].udata.data[0x1c/2]|=TimCnt[2]>>8;
	}
}

signed int *bufl1,*bufr1;
#define SCSPNAME(_8bit,lfo,alfo,loop) \
static void SCSP_Update##_8bit##lfo##alfo##loop(struct _SLOT *slot,unsigned int Enc,unsigned int nsamples)

#define SCSPTMPL(_8bit,lfo,alfo,loop) \
SCSPNAME(_8bit,lfo,alfo,loop)\
{\
	signed int sample;\
	unsigned int s;\
	data32_t addr;\
	for(s=0;s<nsamples;++s)\
	{\
		int step=slot->step;\
		if(!slot->active)\
			return;\
		if(lfo) \
		{\
			step=step*PLFO_Step(&(slot->PLFO));\
			step>>=SHIFT; \
		}\
		if(_8bit)\
		{\
			signed char *p=(signed char *) (slot->base+(slot->cur_addr>>SHIFT));\
			int s2;\
			signed int fpart;\
			fpart=slot->cur_addr&((1<<SHIFT)-1);\
			s2=(int) p[0]*((1<<SHIFT)-fpart)+(int) p[1]*fpart;\
			sample=(s2>>SHIFT)<<8;\
		}\
		else\
		{\
			signed short *p=(signed short *) (slot->base+((slot->cur_addr>>(SHIFT-1))&(~1)));\
			signed int fpart;\
			fpart=slot->cur_addr&((1<<SHIFT)-1);\
			sample=p[0];\
		}\
		slot->cur_addr+=step;\
		addr=slot->cur_addr>>SHIFT;\
		if(loop==0)\
		{\
			if(addr>LEA(slot))\
			{\
				SCSP_StopSlot(slot,0);\
			}\
		}\
		if(loop==1)\
		{\
			if(addr>LEA(slot))\
				slot->cur_addr=LSA(slot)<<SHIFT;\
		}\
		if(loop==2)\
		{\
			if(addr>LEA(slot))\
			{\
				slot->cur_addr=LEA(slot)<<SHIFT;\
				slot->step=REVSIGN(slot->step);\
			}\
			if(addr<LSA(slot) || (addr&0x80000000))\
				slot->cur_addr=LEA(slot)<<SHIFT;\
		}\
		if(loop==3)\
		{\
			if(addr>LEA(slot)) /*reached end, reverse till start*/ \
			{\
				slot->cur_addr=LEA(slot)<<SHIFT;\
				slot->step=REVSIGN(slot->step);\
			}\
			if(addr<LSA(slot) || (addr&0x80000000)) /*reached start or negative*/\
			{\
				slot->cur_addr=LSA(slot)<<SHIFT;\
				slot->step=REVSIGN(slot->step);\
			}\
		}\
		if(alfo)\
		{\
			sample=sample*ALFO_Step(&(slot->ALFO));\
			sample>>=SHIFT;\
		}\
		\
		sample=(sample*EG_Update(slot))>>SHIFT;\
	\
		*bufl1=*bufl1 + ((sample*LPANTABLE[Enc])>>SHIFT);\
		*bufr1=*bufr1 + ((sample*RPANTABLE[Enc])>>SHIFT);\
		++bufl1;\
		++bufr1;\
	}\
}

SCSPTMPL(0,0,0,0) SCSPTMPL(0,0,0,1) SCSPTMPL(0,0,0,2) SCSPTMPL(0,0,0,3)
SCSPTMPL(0,0,1,0) SCSPTMPL(0,0,1,1) SCSPTMPL(0,0,1,2) SCSPTMPL(0,0,1,3)
SCSPTMPL(0,1,0,0) SCSPTMPL(0,1,0,1) SCSPTMPL(0,1,0,2) SCSPTMPL(0,1,0,3)
SCSPTMPL(0,1,1,0) SCSPTMPL(0,1,1,1) SCSPTMPL(0,1,1,2) SCSPTMPL(0,1,1,3)
SCSPTMPL(1,0,0,0) SCSPTMPL(1,0,0,1) SCSPTMPL(1,0,0,2) SCSPTMPL(1,0,0,3)
SCSPTMPL(1,0,1,0) SCSPTMPL(1,0,1,1) SCSPTMPL(1,0,1,2) SCSPTMPL(1,0,1,3)
SCSPTMPL(1,1,0,0) SCSPTMPL(1,1,0,1) SCSPTMPL(1,1,0,2) SCSPTMPL(1,1,0,3)
SCSPTMPL(1,1,1,0) SCSPTMPL(1,1,1,1) SCSPTMPL(1,1,1,2) SCSPTMPL(1,1,1,3)

#undef SCSPTMPL
#define SCSPTMPL(_8bit,lfo,alfo,loop) \
 SCSP_Update##_8bit##lfo##alfo##loop ,


typedef void (*_SCSPUpdateModes)(struct _SLOT *,unsigned int,unsigned int);

_SCSPUpdateModes SCSPUpdateModes[]=
{
	SCSPTMPL(0,0,0,0) SCSPTMPL(0,0,0,1) SCSPTMPL(0,0,0,2) SCSPTMPL(0,0,0,3)
	SCSPTMPL(0,0,1,0) SCSPTMPL(0,0,1,1) SCSPTMPL(0,0,1,2) SCSPTMPL(0,0,1,3)
	SCSPTMPL(0,1,0,0) SCSPTMPL(0,1,0,1) SCSPTMPL(0,1,0,2) SCSPTMPL(0,1,0,3)
	SCSPTMPL(0,1,1,0) SCSPTMPL(0,1,1,1) SCSPTMPL(0,1,1,2) SCSPTMPL(0,1,1,3)
	SCSPTMPL(1,0,0,0) SCSPTMPL(1,0,0,1) SCSPTMPL(1,0,0,2) SCSPTMPL(1,0,0,3)
	SCSPTMPL(1,0,1,0) SCSPTMPL(1,0,1,1) SCSPTMPL(1,0,1,2) SCSPTMPL(1,0,1,3)
	SCSPTMPL(1,1,0,0) SCSPTMPL(1,1,0,1) SCSPTMPL(1,1,0,2) SCSPTMPL(1,1,0,3)
	SCSPTMPL(1,1,1,0) SCSPTMPL(1,1,1,1) SCSPTMPL(1,1,1,2) SCSPTMPL(1,1,1,3)

};


static void SCSP_DoMasterSamples(int chip, int nsamples)
{
	signed short *bufr,*bufl;
	int sl, s;

	SCSP = &SCSPs[chip];

	for(sl=0;sl<32;++sl)
	{
		bufr1=buffertmpr;
		bufl1=buffertmpl;

		if(SCSPs[chip].Slots[sl].active)
		{
			struct _SLOT *slot=SCSPs[chip].Slots+sl;
			unsigned short Enc=((TL(slot))<<0x0)|((DIPAN(slot))<<0x8)|((DISDL(slot))<<0xd);
			unsigned int mode=LPCTL(slot);
			if(PLFOS(slot))
				mode|=8;
			if(ALFOS(slot))
				mode|=4;

			if(PCM8B(slot))
				mode|=0x10;
			SCSPUpdateModes[mode](slot,Enc,nsamples);
		}
	}

	bufr=bufferr;
	bufl=bufferl;
	bufr1=buffertmpr;
	bufl1=buffertmpl;
	for(s=0;s<nsamples;++s)
	{
		signed int smpl=*bufl1>>2;
		signed int smpr=*bufr1>>2;

		if (!chip)
		{
			SCSP_TimersAddTicks(1);
			CheckPendingIRQ();
		}

      MAME_CLAMP_SAMPLE(smpl);
      MAME_CLAMP_SAMPLE(smpr);
		*bufl= smpl;
		*bufr= smpr;
		*bufl1=0;
		*bufr1=0;
		++bufl;
		++bufr;
		++bufl1;
		++bufr1;

	}
}

static void dma_scsp()
{
	static UINT16 tmp_dma[2], *scsp_regs;

	scsp_regs = (UINT16 *)SCSP->udata.datab;

	logerror("SCSP: DMA transfer START\n"
			 "DMEA: %04x DRGA: %04x DTLG: %04x\n"
			 "DGATE: %d  DDIR: %d\n",scsp_dmea,scsp_drga,scsp_dtlg,scsp_dgate ? 1 : 0,scsp_ddir ? 1 : 0);

	/* Copy the dma values in a temp storage for resuming later *
	 * (DMA *can't* overwrite his parameters).                  */
	if(!(scsp_ddir))
	{
		tmp_dma[0] = scsp_regs[0x412/2];
		tmp_dma[1] = scsp_regs[0x414/2];
		tmp_dma[2] = scsp_regs[0x416/2];
	}

	if(scsp_ddir)
	{
		for(;scsp_dtlg > 0;scsp_dtlg-=2)
		{
			cpu_writemem24bedw_word(scsp_dmea, cpu_readmem24bedw_word(0x100000|scsp_drga));
			scsp_dmea+=2;
			scsp_drga+=2;
		}
	}
	else
	{
		for(;scsp_dtlg > 0;scsp_dtlg-=2)
		{
  			cpu_writemem24bedw_word(0x100000|scsp_drga,cpu_readmem24bedw_word(scsp_dmea));
			scsp_dmea+=2;
			scsp_drga+=2;
		}
	}

	/*Resume the values*/
	if(!(scsp_ddir))
	{
	 	scsp_regs[0x412/2] = tmp_dma[0];
		scsp_regs[0x414/2] = tmp_dma[1];
		scsp_regs[0x416/2] = tmp_dma[2];
	}

	/*Job done,request a dma end irq*/
	if(scsp_regs[0x41e/2] & 0x10)
	cpu_set_irq_line(2,dma_transfer_end,HOLD_LINE);
}

int SCSP_IRQCB(int foo)
{
	CheckPendingIRQ();
	return -1;
}

static void SCSP_Update(int num, short **buf, int samples)
{
	bufferl = buf[0];
	bufferr = buf[1];
	length = samples;

	SCSP_DoMasterSamples(num, samples);
}

int SCSP_sh_start(const struct MachineSound *msound)
{
	char buf[2][40];
	const char *name[2];
	int vol[2];
	struct SCSPinterface *intf;
	int i;

	intf = msound->sound_interface;

	/* init the emulation*/
	SCSP_Init(MAX_SCSP, intf);

	/* set up the IRQ callbacks*/
	for (i = 0; i < intf->num; i++)
	{
		SCSPs[i].Int68kCB = intf->irq_callback[i];

		sprintf(buf[0], "SCSP %d R", i);
		sprintf(buf[1], "SCSP %d L", i);
		name[0] = buf[0];
		name[1] = buf[1];
		vol[1]=intf->mixing_level[i] >> 16;
		vol[0]=intf->mixing_level[i] & 0xffff;
		SCSPs[i].stream = stream_init_multi(2, name, vol, 44100, i, SCSP_Update);
	}

	SCSP = &SCSPs[0];

	return 0;
}

void SCSP_sh_stop(void)
{
}

READ16_HANDLER( SCSP_0_r )
{
	SCSP = &SCSPs[0];

	stream_update(SCSPs[0].stream, 0);

	return SCSP_r16(offset*2);
}

WRITE16_HANDLER( SCSP_0_w )
{
	UINT16 tmp, *scsp_regs;

	stream_update(SCSPs[0].stream, 0);

	SCSP = &SCSPs[0];
	tmp = SCSP_r16(offset*2);
	COMBINE_DATA(&tmp);
	SCSP_w16(offset*2, tmp);

	scsp_regs = (UINT16 *)SCSP->udata.datab;

	/* check DMA*/
	switch(offset*2)
	{
		case 0x412:
		/*DMEA [15:1]*/
		/*Sound memory address*/
		scsp_dmea = (((scsp_regs[0x414/2] & 0xf000)>>12)*0x10000) | (scsp_regs[0x412/2] & 0xfffe);
		break;
		case 0x414:
		/*DMEA [19:16]*/
		scsp_dmea = (((scsp_regs[0x414/2] & 0xf000)>>12)*0x10000) | (scsp_regs[0x412/2] & 0xfffe);
		/*DRGA [11:1]*/
		/*Register memory address*/
		scsp_drga = scsp_regs[0x414/2] & 0x0ffe;
		break;
		case 0x416:
		/*DGATE[14]*/
		/*DDIR[13]*/
		/*if 0 sound_mem -> reg*/
		/*if 1 sound_mem <- reg*/
		/*DEXE[12]*/
		/*starting bit*/
		/*DTLG[11:1]*/
		/*size of transfer*/
		scsp_dtlg = scsp_regs[0x416/2] & 0x0ffe;
		if(scsp_dexe)
		{
			dma_scsp();
			scsp_regs[0x416/2]^=0x1000;/*disable starting bit*/
		}
		break;
	}
}

READ16_HANDLER( SCSP_1_r )
{
	SCSP = &SCSPs[1];
	return SCSP_r16(offset*2);
}

WRITE16_HANDLER( SCSP_1_w )
{
	unsigned short tmp;

	SCSP = &SCSPs[1];
	tmp = SCSP_r16(offset*2);
	COMBINE_DATA(&tmp);
	SCSP_w16(offset*2, tmp);
}

WRITE16_HANDLER( SCSP_MidiIn )
{
	MidiStack[MidiW++]=data;
	MidiW&=7;
}

READ16_HANDLER( SCSP_MidiOutR )
{
	unsigned char val;

	val=MidiStack[MidiR++];
	MidiR&=7;
	return val;
}
