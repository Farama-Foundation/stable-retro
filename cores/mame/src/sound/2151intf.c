/***************************************************************************

  2151intf.c

  Support interface YM2151(OPM)

***************************************************************************/

#include "driver.h"
#include "fm.h"
#include "ym2151.h"


/* for stream system */
static int stream[MAX_2151];

static const struct YM2151interface *intf;

static int FMMode;
#define CHIP_YM2151_DAC 4	/* use Tatsuyuki's FM.C */
#define CHIP_YM2151_ALT 5	/* use Jarek's YM2151.C */


#define YM2151_NUMBUF 2

#if (HAS_YM2151)

static void *Timer[MAX_2151][2];

/* IRQ Handler */
static void IRQHandler(int n,int irq)
{
	if(intf->irqhandler[n]) intf->irqhandler[n](irq);
}

static void timer_callback_2151(int param)
{
	int n=param&0x7f;
	int c=param>>7;

	YM2151TimerOver(n,c);
}

/* TimerHandler from fm.c */
static void TimerHandler(int n,int c,int count,double stepTime)
{
	if( count == 0 )
	{	/* Reset FM Timer */
	 	timer_enable (Timer[n][c], 0);
	}
	else
	{	/* Start FM Timer */
		double timeSec = (double)count * stepTime;
		if (!timer_enable(Timer[n][c], 1))
		 	timer_adjust (Timer[n][c], timeSec, (c<<7)|n, 0);
	}
}

#endif

/* update request from fm.c */
void YM2151UpdateRequest(int chip)
{
	stream_update(stream[chip],0);
}

static int my_YM2151_sh_start(const struct MachineSound *msound,int mode)
{
	int i,j;
	int rate = Machine->sample_rate;
	char buf[YM2151_NUMBUF][40];
	const char *name[YM2151_NUMBUF];
	int mixed_vol,vol[YM2151_NUMBUF];

	if( rate == 0 ) rate = 1000;	/* kludge to prevent nasty crashes */

	intf = msound->sound_interface;

	if( mode ) FMMode = CHIP_YM2151_ALT;
	else       FMMode = CHIP_YM2151_DAC;

	switch(FMMode)
	{
#if (HAS_YM2151)
	case CHIP_YM2151_DAC:	/* Tatsuyuki's */
		/* stream system initialize */
		for (i = 0;i < intf->num;i++)
		{
			mixed_vol = intf->volume[i];
			/* stream setup */
			for (j = 0 ; j < YM2151_NUMBUF ; j++)
			{
				name[j]=buf[j];
				vol[j] = mixed_vol & 0xffff;
				mixed_vol>>=16;
				sprintf(buf[j],"%s #%d Ch%d",sound_name(msound),i,j+1);
			}
			stream[i] = stream_init_multi(YM2151_NUMBUF,
				name,vol,rate,i,OPMUpdateOne);
		}
		/* Set Timer handler */
		for (i = 0; i < intf->num; i++)
		{
			Timer[i][0] = timer_alloc(timer_callback_2151);
			Timer[i][1] = timer_alloc(timer_callback_2151);
		}
		if (OPMInit(intf->num,intf->baseclock,Machine->sample_rate,TimerHandler,IRQHandler) == 0)
		{
			/* set port handler */
			for (i = 0; i < intf->num; i++)
				OPMSetPortHander(i,intf->portwritehandler[i]);
			return 0;
		}
		/* error */
		return 1;
#endif
#if (HAS_YM2151_ALT)
	case CHIP_YM2151_ALT:	/* Jarek's */

        rate = intf->baseclock/64;

		/* stream system initialize */
		for (i = 0;i < intf->num;i++)
		{
			/* stream setup */
			mixed_vol = intf->volume[i];
			for (j = 0 ; j < YM2151_NUMBUF ; j++)
			{
				name[j]=buf[j];
				vol[j] = mixed_vol & 0xffff;
				mixed_vol>>=16;
				sprintf(buf[j],"%s #%d Ch%d",sound_name(msound),i,j+1);
			}
			stream[i] = stream_init_multi(YM2151_NUMBUF,
				name,vol,rate,i,YM2151UpdateOne);
		}

		if (YM2151Init(intf->num,intf->baseclock,rate) == 0)
		{
			for (i = 0; i < intf->num; i++)
			{
				YM2151SetIrqHandler(i,intf->irqhandler[i]);
				YM2151SetPortWriteHandler(i,intf->portwritehandler[i]);
			}
			return 0;
		}
		return 1;
#endif
	}
	return 1;
}

#if (HAS_YM2151)
int YM2151_sh_start(const struct MachineSound *msound)
{
	return my_YM2151_sh_start(msound,0);
}
#endif
#if (HAS_YM2151_ALT)
int YM2151_sh_start(const struct MachineSound *msound)
{
	return my_YM2151_sh_start(msound,1);
}
#endif

void YM2151_sh_stop(void)
{
	switch(FMMode)
	{
#if (HAS_YM2151)
	case CHIP_YM2151_DAC:
		OPMShutdown();
		break;
#endif
#if (HAS_YM2151_ALT)
	case CHIP_YM2151_ALT:
		YM2151Shutdown();
		break;
#endif
	}
}

void YM2151_sh_reset(void)
{
	int i;

	for (i = 0;i < intf->num;i++)
	switch(FMMode)
	{
#if (HAS_YM2151)
	case CHIP_YM2151_DAC:
		OPMResetChip(i);
		break;
#endif
#if (HAS_YM2151_ALT)
	case CHIP_YM2151_ALT:
		YM2151ResetChip(i);
		break;
#endif
	}

}

static int lastreg0,lastreg1,lastreg2;

READ_HANDLER( YM2151_status_port_0_r )
{
	switch(FMMode)
	{
#if (HAS_YM2151)
	case CHIP_YM2151_DAC:
		return YM2151Read(0,1);
#endif
#if (HAS_YM2151_ALT)
	case CHIP_YM2151_ALT:
		return YM2151ReadStatus(0);
#endif
	}
	return 0;
}

READ_HANDLER( YM2151_status_port_1_r )
{
	switch(FMMode)
	{
#if (HAS_YM2151)
	case CHIP_YM2151_DAC:
		return YM2151Read(1,1);
#endif
#if (HAS_YM2151_ALT)
	case CHIP_YM2151_ALT:
		return YM2151ReadStatus(1);
#endif
	}
	return 0;
}

READ_HANDLER( YM2151_status_port_2_r )
{
	switch(FMMode)
	{
#if (HAS_YM2151)
	case CHIP_YM2151_DAC:
		return YM2151Read(2,1);
#endif
#if (HAS_YM2151_ALT)
	case CHIP_YM2151_ALT:
		return YM2151ReadStatus(2);
#endif
	}
	return 0;
}

WRITE_HANDLER( YM2151_register_port_0_w )
{
	lastreg0 = data;
}
WRITE_HANDLER( YM2151_register_port_1_w )
{
	lastreg1 = data;
}
WRITE_HANDLER( YM2151_register_port_2_w )
{
	lastreg2 = data;
}

WRITE_HANDLER( YM2151_data_port_0_w )
{
	switch(FMMode)
	{
#if (HAS_YM2151)
	case CHIP_YM2151_DAC:
		YM2151Write(0,0,lastreg0);
		YM2151Write(0,1,data);
		break;
#endif
#if (HAS_YM2151_ALT)
	case CHIP_YM2151_ALT:
		YM2151UpdateRequest(0);
		YM2151WriteReg(0,lastreg0,data);
		break;
#endif
	}
}

WRITE_HANDLER( YM2151_data_port_1_w )
{
	switch(FMMode)
	{
#if (HAS_YM2151)
	case CHIP_YM2151_DAC:
		YM2151Write(1,0,lastreg1);
		YM2151Write(1,1,data);
		break;
#endif
#if (HAS_YM2151_ALT)
	case CHIP_YM2151_ALT:
		YM2151UpdateRequest(1);
		YM2151WriteReg(1,lastreg1,data);
		break;
#endif
	}
}

WRITE_HANDLER( YM2151_data_port_2_w )
{
	switch(FMMode)
	{
#if (HAS_YM2151)
	case CHIP_YM2151_DAC:
		YM2151Write(2,0,lastreg2);
		YM2151Write(2,1,data);
		break;
#endif
#if (HAS_YM2151_ALT)
	case CHIP_YM2151_ALT:
		YM2151UpdateRequest(2);
		YM2151WriteReg(2,lastreg2,data);
		break;
#endif
	}
}

WRITE_HANDLER( YM2151_word_0_w )
{
	if (offset)
		YM2151_data_port_0_w(0,data);
	else
		YM2151_register_port_0_w(0,data);
}

WRITE_HANDLER( YM2151_word_1_w )
{
	if (offset)
		YM2151_data_port_1_w(0,data);
	else
		YM2151_register_port_1_w(0,data);
}

READ16_HANDLER( YM2151_status_port_0_lsb_r )
{
	return YM2151_status_port_0_r(0);
}

READ16_HANDLER( YM2151_status_port_1_lsb_r )
{
	return YM2151_status_port_1_r(0);
}

READ16_HANDLER( YM2151_status_port_2_lsb_r )
{
	return YM2151_status_port_2_r(0);
}


WRITE16_HANDLER( YM2151_register_port_0_lsb_w )
{
	if (ACCESSING_LSB)
		YM2151_register_port_0_w(0, data & 0xff);
}

WRITE16_HANDLER( YM2151_register_port_1_lsb_w )
{
	if (ACCESSING_LSB)
		YM2151_register_port_1_w(0, data & 0xff);
}

WRITE16_HANDLER( YM2151_register_port_2_lsb_w )
{
	if (ACCESSING_LSB)
		YM2151_register_port_2_w(0, data & 0xff);
}

WRITE16_HANDLER( YM2151_data_port_0_lsb_w )
{
	if (ACCESSING_LSB)
		YM2151_data_port_0_w(0, data & 0xff);
}

WRITE16_HANDLER( YM2151_data_port_1_lsb_w )
{
	if (ACCESSING_LSB)
		YM2151_data_port_1_w(0, data & 0xff);
}

WRITE16_HANDLER( YM2151_data_port_2_lsb_w )
{
	if (ACCESSING_LSB)
		YM2151_data_port_2_w(0, data & 0xff);
}
