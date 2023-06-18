/***************************************************************************

  ay8910.c


  Emulation of the AY-3-8910 / YM2149 sound chip.

  Based on various code snippets by Ville Hallik, Michael Cuddy,
  Tatsuyuki Satoh, Fabrice Frances, Nicola Salmoria.

***************************************************************************/

#include "driver.h"
#include "ay8910.h"

#define MAX_OUTPUT 0x7fff

#define STEP 0x8000


int ay8910_index_ym;
static int num = 0, ym_num = 0;

struct AY8910
{
	int Channel;
	int SampleRate;
	mem_read_handler PortAread;
	mem_read_handler PortBread;
	mem_write_handler PortAwrite;
	mem_write_handler PortBwrite;
	int register_latch;
	unsigned char Regs[16];
	int lastEnable;
	unsigned int UpdateStep;
	int PeriodA,PeriodB,PeriodC,PeriodN,PeriodE;
	int CountA,CountB,CountC,CountN,CountE;
	unsigned int VolA,VolB,VolC,VolE;
	unsigned char EnvelopeA,EnvelopeB,EnvelopeC;
	unsigned char OutputA,OutputB,OutputC,OutputN;
	signed char CountEnv;
	unsigned char Hold,Alternate,Attack,Holding;
	int RNG;
	unsigned int VolTable[32];
};

/* register id's */
#define AY_AFINE	(0)
#define AY_ACOARSE	(1)
#define AY_BFINE	(2)
#define AY_BCOARSE	(3)
#define AY_CFINE	(4)
#define AY_CCOARSE	(5)
#define AY_NOISEPER	(6)
#define AY_ENABLE	(7)
#define AY_AVOL		(8)
#define AY_BVOL		(9)
#define AY_CVOL		(10)
#define AY_EFINE	(11)
#define AY_ECOARSE	(12)
#define AY_ESHAPE	(13)

#define AY_PORTA	(14)
#define AY_PORTB	(15)


static struct AY8910 AYPSG[MAX_8910];		/* array of PSG's */



void _AYWriteReg(int n, int r, int v)
{
	struct AY8910 *PSG = &AYPSG[n];
	int old;


	PSG->Regs[r] = v;

	/* A note about the period of tones, noise and envelope: for speed reasons,*/
	/* we count down from the period to 0, but careful studies of the chip     */
	/* output prove that it instead counts up from 0 until the counter becomes */
	/* greater or equal to the period. This is an important difference when the*/
	/* program is rapidly changing the period to modulate the sound.           */
	/* To compensate for the difference, when the period is changed we adjust  */
	/* our internal counter.                                                   */
	/* Also, note that period = 0 is the same as period = 1. This is mentioned */
	/* in the YM2203 data sheets. However, this does NOT apply to the Envelope */
	/* period. In that case, period = 0 is half as period = 1. */
	switch( r )
	{
	case AY_AFINE:
	case AY_ACOARSE:
		PSG->Regs[AY_ACOARSE] &= 0x0f;
		old = PSG->PeriodA;
		PSG->PeriodA = (PSG->Regs[AY_AFINE] + 256 * PSG->Regs[AY_ACOARSE]) * PSG->UpdateStep;
		if (PSG->PeriodA == 0) PSG->PeriodA = PSG->UpdateStep;
		PSG->CountA += PSG->PeriodA - old;
		if (PSG->CountA <= 0) PSG->CountA = 1;
		break;
	case AY_BFINE:
	case AY_BCOARSE:
		PSG->Regs[AY_BCOARSE] &= 0x0f;
		old = PSG->PeriodB;
		PSG->PeriodB = (PSG->Regs[AY_BFINE] + 256 * PSG->Regs[AY_BCOARSE]) * PSG->UpdateStep;
		if (PSG->PeriodB == 0) PSG->PeriodB = PSG->UpdateStep;
		PSG->CountB += PSG->PeriodB - old;
		if (PSG->CountB <= 0) PSG->CountB = 1;
		break;
	case AY_CFINE:
	case AY_CCOARSE:
		PSG->Regs[AY_CCOARSE] &= 0x0f;
		old = PSG->PeriodC;
		PSG->PeriodC = (PSG->Regs[AY_CFINE] + 256 * PSG->Regs[AY_CCOARSE]) * PSG->UpdateStep;
		if (PSG->PeriodC == 0) PSG->PeriodC = PSG->UpdateStep;
		PSG->CountC += PSG->PeriodC - old;
		if (PSG->CountC <= 0) PSG->CountC = 1;
		break;
	case AY_NOISEPER:
		PSG->Regs[AY_NOISEPER] &= 0x1f;
		old = PSG->PeriodN;
		PSG->PeriodN = PSG->Regs[AY_NOISEPER] * PSG->UpdateStep;
		if (PSG->PeriodN == 0) PSG->PeriodN = PSG->UpdateStep;
		PSG->CountN += PSG->PeriodN - old;
		if (PSG->CountN <= 0) PSG->CountN = 1;
		break;
	case AY_ENABLE:
		if ((PSG->lastEnable == -1) ||
		    ((PSG->lastEnable & 0x40) != (PSG->Regs[AY_ENABLE] & 0x40)))
		{
			/* write out 0xff if port set to input */
			if (PSG->PortAwrite)
				(*PSG->PortAwrite)(0, (PSG->Regs[AY_ENABLE] & 0x40) ? PSG->Regs[AY_PORTA] : 0xff);
		}

		if ((PSG->lastEnable == -1) ||
		    ((PSG->lastEnable & 0x80) != (PSG->Regs[AY_ENABLE] & 0x80)))
		{
			/* write out 0xff if port set to input */
			if (PSG->PortBwrite)
				(*PSG->PortBwrite)(0, (PSG->Regs[AY_ENABLE] & 0x80) ? PSG->Regs[AY_PORTB] : 0xff);
		}

		PSG->lastEnable = PSG->Regs[AY_ENABLE];
		break;
	case AY_AVOL:
		PSG->Regs[AY_AVOL] &= 0x1f;
		PSG->EnvelopeA = PSG->Regs[AY_AVOL] & 0x10;
		PSG->VolA = PSG->EnvelopeA ? PSG->VolE : PSG->VolTable[PSG->Regs[AY_AVOL] ? PSG->Regs[AY_AVOL]*2+1 : 0];
		break;
	case AY_BVOL:
		PSG->Regs[AY_BVOL] &= 0x1f;
		PSG->EnvelopeB = PSG->Regs[AY_BVOL] & 0x10;
		PSG->VolB = PSG->EnvelopeB ? PSG->VolE : PSG->VolTable[PSG->Regs[AY_BVOL] ? PSG->Regs[AY_BVOL]*2+1 : 0];
		break;
	case AY_CVOL:
		PSG->Regs[AY_CVOL] &= 0x1f;
		PSG->EnvelopeC = PSG->Regs[AY_CVOL] & 0x10;
		PSG->VolC = PSG->EnvelopeC ? PSG->VolE : PSG->VolTable[PSG->Regs[AY_CVOL] ? PSG->Regs[AY_CVOL]*2+1 : 0];
		break;
	case AY_EFINE:
	case AY_ECOARSE:
		old = PSG->PeriodE;
		PSG->PeriodE = ((PSG->Regs[AY_EFINE] + 256 * PSG->Regs[AY_ECOARSE])) * PSG->UpdateStep;
		if (PSG->PeriodE == 0) PSG->PeriodE = PSG->UpdateStep / 2;
		PSG->CountE += PSG->PeriodE - old;
		if (PSG->CountE <= 0) PSG->CountE = 1;
		break;
	case AY_ESHAPE:
		/* envelope shapes:
		C AtAlH
		0 0 x x  \___

		0 1 x x  /___

		1 0 0 0  \\\\

		1 0 0 1  \___

		1 0 1 0  \/\/
		          ___
		1 0 1 1  \

		1 1 0 0  /*//*/
		          ___
		1 1 0 1  /

		1 1 1 0  /\/\

		1 1 1 1  /___

		The envelope counter on the AY-3-8910 has 16 steps. On the YM2149 it
		has twice the steps, happening twice as fast. Since the end result is
		just a smoother curve, we always use the YM2149 behaviour.
		*/
		PSG->Regs[AY_ESHAPE] &= 0x0f;
		PSG->Attack = (PSG->Regs[AY_ESHAPE] & 0x04) ? 0x1f : 0x00;
		if ((PSG->Regs[AY_ESHAPE] & 0x08) == 0)
		{
			/* if Continue = 0, map the shape to the equivalent one which has Continue = 1 */
			PSG->Hold = 1;
			PSG->Alternate = PSG->Attack;
		}
		else
		{
			PSG->Hold = PSG->Regs[AY_ESHAPE] & 0x01;
			PSG->Alternate = PSG->Regs[AY_ESHAPE] & 0x02;
		}
		PSG->CountE = PSG->PeriodE;
		PSG->CountEnv = 0x1f;
		PSG->Holding = 0;
		PSG->VolE = PSG->VolTable[PSG->CountEnv ^ PSG->Attack];
		if (PSG->EnvelopeA) PSG->VolA = PSG->VolE;
		if (PSG->EnvelopeB) PSG->VolB = PSG->VolE;
		if (PSG->EnvelopeC) PSG->VolC = PSG->VolE;
		break;
	case AY_PORTA:
		if (PSG->Regs[AY_ENABLE] & 0x40)
		{
			if (PSG->PortAwrite)
				(*PSG->PortAwrite)(0, PSG->Regs[AY_PORTA]);
			else
				log_cb(RETRO_LOG_DEBUG, LOGPRE "PC %04x: warning - write %02x to 8910 #%d Port A\n",activecpu_get_pc(),PSG->Regs[AY_PORTA],n);
		}
		else
		{
			log_cb(RETRO_LOG_DEBUG, LOGPRE "warning: write to 8910 #%d Port A set as input - ignored\n",n);
		}
		break;
	case AY_PORTB:
		if (PSG->Regs[AY_ENABLE] & 0x80)
		{
			if (PSG->PortBwrite)
				(*PSG->PortBwrite)(0, PSG->Regs[AY_PORTB]);
			else
				log_cb(RETRO_LOG_DEBUG, LOGPRE "PC %04x: warning - write %02x to 8910 #%d Port B\n",activecpu_get_pc(),PSG->Regs[AY_PORTB],n);
		}
		else
		{
			log_cb(RETRO_LOG_DEBUG, LOGPRE "warning: write to 8910 #%d Port B set as input - ignored\n",n);
		}
		break;
	}
}


/* write a register on AY8910 chip number 'n' */
void AYWriteReg(int chip, int r, int v)
{
	struct AY8910 *PSG = &AYPSG[chip];


	if (r > 15) return;
	if (r < 14)
	{
		if (r == AY_ESHAPE || PSG->Regs[r] != v)
		{
			/* update the output buffer before changing the register */
			stream_update(PSG->Channel,0);
		}
	}

	_AYWriteReg(chip,r,v);
}



unsigned char AYReadReg(int n, int r)
{
	struct AY8910 *PSG = &AYPSG[n];


	if (r > 15) return 0;

	switch (r)
	{
	case AY_PORTA:
		if ((PSG->Regs[AY_ENABLE] & 0x40) != 0)
			log_cb(RETRO_LOG_DEBUG, LOGPRE "warning: read from 8910 #%d Port A set as output\n",n);
		/*
		   even if the port is set as output, we still need to return the external
		   data. Some games, like kidniki, need this to work.
		 */
		if (PSG->PortAread) PSG->Regs[AY_PORTA] = (*PSG->PortAread)(0);
		else log_cb(RETRO_LOG_DEBUG, LOGPRE "PC %04x: warning - read 8910 #%d Port A\n",activecpu_get_pc(),n);
		break;
	case AY_PORTB:
		if ((PSG->Regs[AY_ENABLE] & 0x80) != 0)
			log_cb(RETRO_LOG_DEBUG, LOGPRE "warning: read from 8910 #%d Port B set as output\n",n);
		if (PSG->PortBread) PSG->Regs[AY_PORTB] = (*PSG->PortBread)(0);
		else log_cb(RETRO_LOG_DEBUG, LOGPRE "PC %04x: warning - read 8910 #%d Port B\n",activecpu_get_pc(),n);
		break;
	}
	return PSG->Regs[r];
}


void AY8910Write(int chip,int a,int data)
{
	struct AY8910 *PSG = &AYPSG[chip];

	if (a & 1)
	{	/* Data port */
		AYWriteReg(chip,PSG->register_latch,data);
	}
	else
	{	/* Register port */
		PSG->register_latch = data & 0x0f;
	}
}

int AY8910Read(int chip)
{
	struct AY8910 *PSG = &AYPSG[chip];

	return AYReadReg(chip,PSG->register_latch);
}


/* AY8910 interface */
READ_HANDLER( AY8910_read_port_0_r ) { return AY8910Read(0); }
READ_HANDLER( AY8910_read_port_1_r ) { return AY8910Read(1); }
READ_HANDLER( AY8910_read_port_2_r ) { return AY8910Read(2); }
READ_HANDLER( AY8910_read_port_3_r ) { return AY8910Read(3); }
READ_HANDLER( AY8910_read_port_4_r ) { return AY8910Read(4); }
READ16_HANDLER( AY8910_read_port_0_lsb_r ) { return AY8910Read(0); }
READ16_HANDLER( AY8910_read_port_1_lsb_r ) { return AY8910Read(1); }
READ16_HANDLER( AY8910_read_port_2_lsb_r ) { return AY8910Read(2); }
READ16_HANDLER( AY8910_read_port_3_lsb_r ) { return AY8910Read(3); }
READ16_HANDLER( AY8910_read_port_4_lsb_r ) { return AY8910Read(4); }
READ16_HANDLER( AY8910_read_port_0_msb_r ) { return AY8910Read(0) << 8; }
READ16_HANDLER( AY8910_read_port_1_msb_r ) { return AY8910Read(1) << 8; }
READ16_HANDLER( AY8910_read_port_2_msb_r ) { return AY8910Read(2) << 8; }
READ16_HANDLER( AY8910_read_port_3_msb_r ) { return AY8910Read(3) << 8; }
READ16_HANDLER( AY8910_read_port_4_msb_r ) { return AY8910Read(4) << 8; }

WRITE_HANDLER( AY8910_control_port_0_w ) { AY8910Write(0,0,data); }
WRITE_HANDLER( AY8910_control_port_1_w ) { AY8910Write(1,0,data); }
WRITE_HANDLER( AY8910_control_port_2_w ) { AY8910Write(2,0,data); }
WRITE_HANDLER( AY8910_control_port_3_w ) { AY8910Write(3,0,data); }
WRITE_HANDLER( AY8910_control_port_4_w ) { AY8910Write(4,0,data); }
WRITE16_HANDLER( AY8910_control_port_0_lsb_w ) { if (ACCESSING_LSB) AY8910Write(0,0,data & 0xff); }
WRITE16_HANDLER( AY8910_control_port_1_lsb_w ) { if (ACCESSING_LSB) AY8910Write(1,0,data & 0xff); }
WRITE16_HANDLER( AY8910_control_port_2_lsb_w ) { if (ACCESSING_LSB) AY8910Write(2,0,data & 0xff); }
WRITE16_HANDLER( AY8910_control_port_3_lsb_w ) { if (ACCESSING_LSB) AY8910Write(3,0,data & 0xff); }
WRITE16_HANDLER( AY8910_control_port_4_lsb_w ) { if (ACCESSING_LSB) AY8910Write(4,0,data & 0xff); }
WRITE16_HANDLER( AY8910_control_port_0_msb_w ) { if (ACCESSING_MSB) AY8910Write(0,0,data >> 8); }
WRITE16_HANDLER( AY8910_control_port_1_msb_w ) { if (ACCESSING_MSB) AY8910Write(1,0,data >> 8); }
WRITE16_HANDLER( AY8910_control_port_2_msb_w ) { if (ACCESSING_MSB) AY8910Write(2,0,data >> 8); }
WRITE16_HANDLER( AY8910_control_port_3_msb_w ) { if (ACCESSING_MSB) AY8910Write(3,0,data >> 8); }
WRITE16_HANDLER( AY8910_control_port_4_msb_w ) { if (ACCESSING_MSB) AY8910Write(4,0,data >> 8); }

WRITE_HANDLER( AY8910_write_port_0_w ) { AY8910Write(0,1,data); }
WRITE_HANDLER( AY8910_write_port_1_w ) { AY8910Write(1,1,data); }
WRITE_HANDLER( AY8910_write_port_2_w ) { AY8910Write(2,1,data); }
WRITE_HANDLER( AY8910_write_port_3_w ) { AY8910Write(3,1,data); }
WRITE_HANDLER( AY8910_write_port_4_w ) { AY8910Write(4,1,data); }
WRITE16_HANDLER( AY8910_write_port_0_lsb_w ) { if (ACCESSING_LSB) AY8910Write(0,1,data & 0xff); }
WRITE16_HANDLER( AY8910_write_port_1_lsb_w ) { if (ACCESSING_LSB) AY8910Write(1,1,data & 0xff); }
WRITE16_HANDLER( AY8910_write_port_2_lsb_w ) { if (ACCESSING_LSB) AY8910Write(2,1,data & 0xff); }
WRITE16_HANDLER( AY8910_write_port_3_lsb_w ) { if (ACCESSING_LSB) AY8910Write(3,1,data & 0xff); }
WRITE16_HANDLER( AY8910_write_port_4_lsb_w ) { if (ACCESSING_LSB) AY8910Write(4,1,data & 0xff); }
WRITE16_HANDLER( AY8910_write_port_0_msb_w ) { if (ACCESSING_MSB) AY8910Write(0,1,data >> 8); }
WRITE16_HANDLER( AY8910_write_port_1_msb_w ) { if (ACCESSING_MSB) AY8910Write(1,1,data >> 8); }
WRITE16_HANDLER( AY8910_write_port_2_msb_w ) { if (ACCESSING_MSB) AY8910Write(2,1,data >> 8); }
WRITE16_HANDLER( AY8910_write_port_3_msb_w ) { if (ACCESSING_MSB) AY8910Write(3,1,data >> 8); }
WRITE16_HANDLER( AY8910_write_port_4_msb_w ) { if (ACCESSING_MSB) AY8910Write(4,1,data >> 8); }



static void AY8910Update(int chip,INT16 **buffer,int length)
{
	struct AY8910 *PSG = &AYPSG[chip];
	INT16 *buf1,*buf2,*buf3;
	int outn;

	buf1 = buffer[0];
	buf2 = buffer[1];
	buf3 = buffer[2];


	/* The 8910 has three outputs, each output is the mix of one of the three */
	/* tone generators and of the (single) noise generator. The two are mixed */
	/* BEFORE going into the DAC. The formula to mix each channel is: */
	/* (ToneOn | ToneDisable) & (NoiseOn | NoiseDisable). */
	/* Note that this means that if both tone and noise are disabled, the output */
	/* is 1, not 0, and can be modulated changing the volume. */


	/* If the channels are disabled, set their output to 1, and increase the */
	/* counter, if necessary, so they will not be inverted during this update. */
	/* Setting the output to 1 is necessary because a disabled channel is locked */
	/* into the ON state (see above); and it has no effect if the volume is 0. */
	/* If the volume is 0, increase the counter, but don't touch the output. */
	if (PSG->Regs[AY_ENABLE] & 0x01)
	{
		if (PSG->CountA <= length*STEP) PSG->CountA += length*STEP;
		PSG->OutputA = 1;
	}
	else if (PSG->Regs[AY_AVOL] == 0)
	{
		/* note that I do count += length, NOT count = length + 1. You might think */
		/* it's the same since the volume is 0, but doing the latter could cause */
		/* interferencies when the program is rapidly modulating the volume. */
		if (PSG->CountA <= length*STEP) PSG->CountA += length*STEP;
	}
	if (PSG->Regs[AY_ENABLE] & 0x02)
	{
		if (PSG->CountB <= length*STEP) PSG->CountB += length*STEP;
		PSG->OutputB = 1;
	}
	else if (PSG->Regs[AY_BVOL] == 0)
	{
		if (PSG->CountB <= length*STEP) PSG->CountB += length*STEP;
	}
	if (PSG->Regs[AY_ENABLE] & 0x04)
	{
		if (PSG->CountC <= length*STEP) PSG->CountC += length*STEP;
		PSG->OutputC = 1;
	}
	else if (PSG->Regs[AY_CVOL] == 0)
	{
		if (PSG->CountC <= length*STEP) PSG->CountC += length*STEP;
	}

	/* for the noise channel we must not touch OutputN - it's also not necessary */
	/* since we use outn. */
	if ((PSG->Regs[AY_ENABLE] & 0x38) == 0x38)	/* all off */
		if (PSG->CountN <= length*STEP) PSG->CountN += length*STEP;

	outn = (PSG->OutputN | PSG->Regs[AY_ENABLE]);


	/* buffering loop */
	while (length)
	{
		int vola,volb,volc;
		int left;


		/* vola, volb and volc keep track of how long each square wave stays */
		/* in the 1 position during the sample period. */
		vola = volb = volc = 0;

		left = STEP;
		do
		{
			int nextevent;


			if (PSG->CountN < left) nextevent = PSG->CountN;
			else nextevent = left;

			if (outn & 0x08)
			{
				if (PSG->OutputA) vola += PSG->CountA;
				PSG->CountA -= nextevent;
				/* PeriodA is the half period of the square wave. Here, in each */
				/* loop I add PeriodA twice, so that at the end of the loop the */
				/* square wave is in the same status (0 or 1) it was at the start. */
				/* vola is also incremented by PeriodA, since the wave has been 1 */
				/* exactly half of the time, regardless of the initial position. */
				/* If we exit the loop in the middle, OutputA has to be inverted */
				/* and vola incremented only if the exit status of the square */
				/* wave is 1. */
				while (PSG->CountA <= 0)
				{
					PSG->CountA += PSG->PeriodA;
					if (PSG->CountA > 0)
					{
						PSG->OutputA ^= 1;
						if (PSG->OutputA) vola += PSG->PeriodA;
						break;
					}
					PSG->CountA += PSG->PeriodA;
					vola += PSG->PeriodA;
				}
				if (PSG->OutputA) vola -= PSG->CountA;
			}
			else
			{
				PSG->CountA -= nextevent;
				while (PSG->CountA <= 0)
				{
					PSG->CountA += PSG->PeriodA;
					if (PSG->CountA > 0)
					{
						PSG->OutputA ^= 1;
						break;
					}
					PSG->CountA += PSG->PeriodA;
				}
			}

			if (outn & 0x10)
			{
				if (PSG->OutputB) volb += PSG->CountB;
				PSG->CountB -= nextevent;
				while (PSG->CountB <= 0)
				{
					PSG->CountB += PSG->PeriodB;
					if (PSG->CountB > 0)
					{
						PSG->OutputB ^= 1;
						if (PSG->OutputB) volb += PSG->PeriodB;
						break;
					}
					PSG->CountB += PSG->PeriodB;
					volb += PSG->PeriodB;
				}
				if (PSG->OutputB) volb -= PSG->CountB;
			}
			else
			{
				PSG->CountB -= nextevent;
				while (PSG->CountB <= 0)
				{
					PSG->CountB += PSG->PeriodB;
					if (PSG->CountB > 0)
					{
						PSG->OutputB ^= 1;
						break;
					}
					PSG->CountB += PSG->PeriodB;
				}
			}

			if (outn & 0x20)
			{
				if (PSG->OutputC) volc += PSG->CountC;
				PSG->CountC -= nextevent;
				while (PSG->CountC <= 0)
				{
					PSG->CountC += PSG->PeriodC;
					if (PSG->CountC > 0)
					{
						PSG->OutputC ^= 1;
						if (PSG->OutputC) volc += PSG->PeriodC;
						break;
					}
					PSG->CountC += PSG->PeriodC;
					volc += PSG->PeriodC;
				}
				if (PSG->OutputC) volc -= PSG->CountC;
			}
			else
			{
				PSG->CountC -= nextevent;
				while (PSG->CountC <= 0)
				{
					PSG->CountC += PSG->PeriodC;
					if (PSG->CountC > 0)
					{
						PSG->OutputC ^= 1;
						break;
					}
					PSG->CountC += PSG->PeriodC;
				}
			}

			PSG->CountN -= nextevent;
			if (PSG->CountN <= 0)
			{
				/* Is noise output going to change? */
				if ((PSG->RNG + 1) & 2)	/* (bit0^bit1)? */
				{
					PSG->OutputN = ~PSG->OutputN;
					outn = (PSG->OutputN | PSG->Regs[AY_ENABLE]);
				}

				/* The Random Number Generator of the 8910 is a 17-bit shift */
				/* register. The input to the shift register is bit0 XOR bit3 */
				/* (bit0 is the output). This was verified on AY-3-8910 and YM2149 chips. */

				/* The following is a fast way to compute bit17 = bit0^bit3. */
				/* Instead of doing all the logic operations, we only check */
				/* bit0, relying on the fact that after three shifts of the */
				/* register, what now is bit3 will become bit0, and will */
				/* invert, if necessary, bit14, which previously was bit17. */
				if (PSG->RNG & 1) PSG->RNG ^= 0x24000; /* This version is called the "Galois configuration". */
				PSG->RNG >>= 1;
				PSG->CountN += PSG->PeriodN;
			}

			left -= nextevent;
		} while (left > 0);

		/* update envelope */
		if (PSG->Holding == 0)
		{
			PSG->CountE -= STEP;
			if (PSG->CountE <= 0)
			{
				do
				{
					PSG->CountEnv--;
					PSG->CountE += PSG->PeriodE;
				} while (PSG->CountE <= 0);

				/* check envelope current position */
				if (PSG->CountEnv < 0)
				{
					if (PSG->Hold)
					{
						if (PSG->Alternate)
							PSG->Attack ^= 0x1f;
						PSG->Holding = 1;
						PSG->CountEnv = 0;
					}
					else
					{
						/* if CountEnv has looped an odd number of times (usually 1), */
						/* invert the output. */
						if (PSG->Alternate && (PSG->CountEnv & 0x20))
 							PSG->Attack ^= 0x1f;

						PSG->CountEnv &= 0x1f;
					}
				}

				PSG->VolE = PSG->VolTable[PSG->CountEnv ^ PSG->Attack];
				/* reload volume */
				if (PSG->EnvelopeA) PSG->VolA = PSG->VolE;
				if (PSG->EnvelopeB) PSG->VolB = PSG->VolE;
				if (PSG->EnvelopeC) PSG->VolC = PSG->VolE;
			}
		}

		*(buf1++) = (vola * PSG->VolA) / STEP;
		*(buf2++) = (volb * PSG->VolB) / STEP;
		*(buf3++) = (volc * PSG->VolC) / STEP;

		length--;
	}
}


void AY8910_set_clock(int chip,int clock)
{
	struct AY8910 *PSG = &AYPSG[chip];

	/* the step clock for the tone and noise generators is the chip clock    */
	/* divided by 8; for the envelope generator of the AY-3-8910, it is half */
	/* that much (clock/16), but the envelope of the YM2149 goes twice as    */
	/* fast, therefore again clock/8.                                        */
	/* Here we calculate the number of steps which happen during one sample  */
	/* at the given sample rate. No. of events = sample rate / (clock/8).    */
	/* STEP is a multiplier used to turn the fraction into a fixed point     */
	/* number.                                                               */
	PSG->UpdateStep = ((double)STEP * PSG->SampleRate * 8 + clock/2) / clock;
}


void AY8910_set_volume(int chip,int channel,int volume)
{
	struct AY8910 *PSG = &AYPSG[chip];
	int ch;

	for (ch = 0; ch < 3; ch++)
		if (channel == ch || channel == ALL_8910_CHANNELS)
			mixer_set_volume(PSG->Channel + ch, volume);
}


static void build_mixer_table(int chip)
{
	struct AY8910 *PSG = &AYPSG[chip];
	int i;
	double out;


	/* calculate the volume->voltage conversion table */
	/* The AY-3-8910 has 16 levels, in a logarithmic scale (3dB per step) */
	/* The YM2149 still has 16 levels for the tone generators, but 32 for */
	/* the envelope generator (1.5dB per step). */
	out = MAX_OUTPUT;
	for (i = 31;i > 0;i--)
	{
		PSG->VolTable[i] = out + 0.5;	/* round to nearest */

		out /= 1.188502227;	/* = 10 ^ (1.5/20) = 1.5dB */
	}
	PSG->VolTable[0] = 0;
}



void AY8910_reset(int chip)
{
	int i;
	struct AY8910 *PSG = &AYPSG[chip];

	PSG->register_latch = 0;
	PSG->RNG = 1;
	PSG->OutputA = 0;
	PSG->OutputB = 0;
	PSG->OutputC = 0;
	PSG->OutputN = 0xff;
	PSG->lastEnable = -1;	/* force a write */
	for (i = 0;i < AY_PORTA;i++)
		_AYWriteReg(chip,i,0);	/* AYWriteReg() uses the timer system; we cannot */
								/* call it at this time because the timer system */
								/* has not been initialized. */
}

void AY8910_sh_reset(void)
{
	int i;

	for (i = 0;i < num + ym_num;i++)
		AY8910_reset(i);
}

static int AY8910_init(const char *chip_name,int chip,
		int clock,int volume,int sample_rate,
		mem_read_handler portAread,mem_read_handler portBread,
		mem_write_handler portAwrite,mem_write_handler portBwrite)
{
	int i;
	struct AY8910 *PSG = &AYPSG[chip];
	char buf[3][40];
	const char *name[3];
	int vol[3];


/* causes crashes with YM2610 games - overflow?*/
/*	if (options.use_filter)*/
/*		sample_rate = clock/8;*/

	memset(PSG,0,sizeof(struct AY8910));
	PSG->SampleRate = sample_rate;
	PSG->PortAread = portAread;
	PSG->PortBread = portBread;
	PSG->PortAwrite = portAwrite;
	PSG->PortBwrite = portBwrite;
	for (i = 0;i < 3;i++)
	{
		vol[i] = volume;
		name[i] = buf[i];
		sprintf(buf[i],"%s #%d Ch %c",chip_name,chip,'A'+i);
	}
	PSG->Channel = stream_init_multi(3,name,vol,sample_rate,chip,AY8910Update);

	if (PSG->Channel == -1)
		return 1;

	AY8910_set_clock(chip,clock);

	return 0;
}


int AY8910_sh_start(const struct MachineSound *msound)
{
	int chip;
	const struct AY8910interface *intf = msound->sound_interface;

	num = intf->num;

	for (chip = 0;chip < num;chip++)
	{
		if (AY8910_init(sound_name(msound),chip+ym_num,intf->baseclock,
				intf->mixing_level[chip] & 0xffff,
				Machine->sample_rate,
				intf->portAread[chip],intf->portBread[chip],
				intf->portAwrite[chip],intf->portBwrite[chip]) != 0)
			return 1;
		build_mixer_table(chip+ym_num);
	}
	return 0;
}

void AY8910_sh_stop(void)
{
	num = 0;
}

int AY8910_sh_start_ym(const struct MachineSound *msound)
{
	int chip;
	const struct AY8910interface *intf = msound->sound_interface;

	ym_num = intf->num;
	ay8910_index_ym = num;

	for (chip = 0;chip < ym_num;chip++)
	{
		if (AY8910_init(sound_name(msound),chip+num,intf->baseclock,
				intf->mixing_level[chip] & 0xffff,
				Machine->sample_rate,
				intf->portAread[chip],intf->portBread[chip],
				intf->portAwrite[chip],intf->portBwrite[chip]) != 0)
			return 1;
		build_mixer_table(chip+num);
	}
	return 0;
}

void AY8910_sh_stop_ym(void)
{
	ym_num = 0;
}
