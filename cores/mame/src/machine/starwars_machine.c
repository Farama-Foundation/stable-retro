/***************************************************************************

	Atari Star Wars hardware

	This file is Copyright 1997, Steve Baines.
	Modified by Frank Palazzolo for sound support

***************************************************************************/

#include "driver.h"
#include "cpuintrf.h"
#include "starwars.h"
#include "vidhrdw/avgdvg.h"


/* Control select values for ADC_R */
#define kPitch		0
#define kYaw		1
#define kThrust		2

/* Constants for mathbox operations */
#define NOP			0x00
#define LAC			0x01
#define READ_ACC	0x02
#define M_HALT		0x04
#define INC_BIC		0x08
#define CLEAR_ACC	0x10
#define LDC			0x20
#define LDB			0x40
#define LDA			0x80

/* Debugging flag */
#define MATHDEBUG	0


/* Local variables */
static UINT8 control_num = kPitch;

static int MPA; /* PROM address counter */
static int BIC; /* Block index counter  */
static int PRN; /* Pseudo-random number */

static int div_result;
static int divisor, dividend;

/* Store decoded PROM elements */
static UINT8 PROM_STR[1024]; /* Storage for instruction strobe only */
static UINT8 PROM_MAS[1024]; /* Storage for direct address only */
static UINT8 PROM_AM[1024]; /* Storage for address mode select only */


/* Local function prototypes */
static void run_mbox(void);



/*************************************
 *
 *	Output latch
 *
 *************************************/

WRITE_HANDLER( starwars_out_w )
{
	unsigned char *RAM = memory_region(REGION_CPU1);

	switch (offset)
	{
		case 0:		/* Coin counter 1 */
			coin_counter_w(0, data);
			break;

		case 1:		/* Coin counter 2 */
			coin_counter_w(1, data);
			break;

		case 2:		/* LED 3 */
			set_led_status(2, ~data & 0x80);
			break;

		case 3:		/* LED 2 */
			set_led_status(1, ~data & 0x80);
			break;

		case 4:		/* bank switch */
			if (data & 0x80)
			{
				cpu_setbank(1, &RAM[0x10000]);
			    if (starwars_is_esb) cpu_setbank(2, &RAM[0x1c000]);
			}
			else
			{
				cpu_setbank(1, &RAM[0x06000]);
				if (starwars_is_esb) cpu_setbank(2, &RAM[0x0a000]);
			}
			break;

		case 5:		/* reset PRNG */
			PRN = 0;
			break;

		case 6:		/* LED 1 */
			set_led_status(0, ~data & 0x80);
			break;

		case 7:
			log_cb(RETRO_LOG_DEBUG, LOGPRE "recall\n"); /* what's that? */
			break;
	}
}



/*************************************
 *
 *	Input port 1
 *
 *************************************/

READ_HANDLER( starwars_input_1_r )
{
	int x = readinputport(1);

	/* Kludge to enable Starwars Mathbox Self-test                  */
	/* The mathbox looks like it's running, from this address... :) */
	if (activecpu_get_pc() == 0xf978 || activecpu_get_pc() == 0xf655)
		x |= 0x80;

	/* set the AVG done flag */
	if (avgdvg_done())
		x |= 0x40;
	else
		x &= ~0x40;

	return x;
}



/*************************************
 *
 *	ADC input and control
 *
 *************************************/

READ_HANDLER( starwars_adc_r )
{
	/* pitch */
	if (control_num == kPitch)
		return readinputport(4);

	/* yaw */
	else if (control_num == kYaw)
		return readinputport(5);

	/* default to unused thrust */
	else
		return 0;
}


WRITE_HANDLER( starwars_adc_select_w )
{
	control_num = offset;
}



/*************************************
 *
 *	Mathbox initialization
 *
 *************************************/

void swmathbox_init(void)
{
	UINT8 *src = memory_region(REGION_PROMS);
	int cnt, val;

	for (cnt = 0; cnt < 1024; cnt++)
	{
		/* translate PROMS into 16 bit code */
		val  = (src[0x0c00 + cnt]      ) & 0x000f; /* Set LS nibble */
		val |= (src[0x0800 + cnt] <<  4) & 0x00f0;
		val |= (src[0x0400 + cnt] <<  8) & 0x0f00;
		val |= (src[0x0000 + cnt] << 12) & 0xf000; /* Set MS nibble */

		/* perform pre-decoding */
		PROM_STR[cnt] = (val >> 8) & 0x00ff;
		PROM_MAS[cnt] =  val       & 0x007f;
		PROM_AM[cnt]  = (val >> 7) & 0x0001;
	}
}



/*************************************
 *
 *	Mathbox reset
 *
 *************************************/

void swmathbox_reset(void)
{
	MPA = BIC = 0;
	PRN = 0;
}



/*************************************
 *
 *	Mathbox execution
 *
 *************************************/

void run_mbox(void)
{
	static short ACC, A, B, C;

	UINT8 *RAM = memory_region(REGION_CPU1);
	int RAMWORD = 0;
	int MA_byte;
	int tmp;
	int M_STOP = 100000; /* Limit on number of instructions allowed before halt */
	int MA;
	int IP15_8, IP7, IP6_0; /* Instruction PROM values */


	log_cb(RETRO_LOG_DEBUG, LOGPRE "Running Mathbox...\n");

	/* loop until finished */
	while (M_STOP > 0)
	{
		/* fetch the current instruction data */
		IP15_8 = PROM_STR[MPA];
		IP7    = PROM_AM[MPA];
		IP6_0  = PROM_MAS[MPA];

#if (MATHDEBUG)
		log_cb(RETRO_LOG_DEBUG, LOGPRE "\n(MPA:%x), Strobe: %x, IP7: %d, IP6_0:%x\n",MPA, IP15_8, IP7, IP6_0);
		log_cb(RETRO_LOG_DEBUG, LOGPRE "(BIC: %x), A: %x, B: %x, C: %x, ACC: %x\n",BIC,A,B,C,ACC);
#endif

		/* construct the current RAM address */
		if (IP7 == 0)
			MA = (IP6_0 & 3) | ((BIC & 0x01ff) << 2);	/* MA10-2 set to BIC8-0 */
		else
			MA = IP6_0;

		/* convert RAM offset to eight bit addressing (2kx8 rather than 1k*16)
			and apply base address offset */

		MA_byte = 0x5000 + (MA << 1);
		RAMWORD = (RAM[MA_byte + 1] & 0x00ff) | ((RAM[MA_byte] & 0x00ff) << 8);

		log_cb(RETRO_LOG_DEBUG, LOGPRE "MATH ADDR: %x, CPU ADDR: %x, RAMWORD: %x\n", MA, MA_byte, RAMWORD);

		/*
		 * RAMWORD is the sixteen bit Math RAM value for the selected address
		 * MA_byte is the base address of this location as seen by the main CPU
		 * IP is the 16 bit instruction word from the PROM. IP7_0 have already
		 * been used in the address selection stage
		 * IP15_8 provide the instruction strobes
		 */

		/* 0x01 - LAC */
		if (IP15_8 & LAC)
			ACC = RAMWORD;

		/* 0x02 - READ_ACC */
		if (IP15_8 & READ_ACC)
		{
			RAM[MA_byte+1] = (ACC & 0x00ff);
			RAM[MA_byte  ] = (ACC & 0xff00) >> 8;
		}

		/* 0x04 - M_HALT */
		if (IP15_8 & M_HALT)
			M_STOP = 0;

		/* 0x08 - INC_BIC */
		if (IP15_8 & INC_BIC)
			BIC = (BIC + 1) & 0x1ff; /* Restrict to 9 bits */

		/* 0x10 - CLEAR_ACC */
		if (IP15_8 & CLEAR_ACC)
			ACC = 0;

		/* 0x20 - LDC */
		if (IP15_8 & LDC)
		{
			C = RAMWORD;
			/* TODO: this next line is accurate to the schematics, but doesn't seem to work right */
			/* ACC=ACC+(  ( (long)((A-B)*C) )>>14  ); */
			/* round the result - this fixes bad trench vectors in Star Wars */
			ACC += ((((long)((A - B) * C)) >> 13) + 1) >> 1;
		}

		/* 0x40 - LDB */
		if (IP15_8 & LDB)
			B = RAMWORD;

		/* 0x80 - LDA */
		if (IP15_8 & LDA)
			A = RAMWORD;

		/*
		 * Now update the PROM address counter
		 * Done like this because the top two bits are not part of the counter
		 * This means that each of the four pages should wrap around rather than
		 * leaking from one to another.  It may not matter, but I've put it in anyway
		 */
		tmp = MPA + 1;
		MPA = (MPA & 0x0300) | (tmp & 0x00ff); /* New MPA value */

		M_STOP--; /* Decrease count */
	}
}



/*************************************
 *
 *	Pseudo-RNG read
 *
 *************************************/

READ_HANDLER( swmathbx_prng_r )
{
	PRN = (int)((PRN + 0x2364) ^ 2); /* This is a total bodge for now, but it works!*/
	return PRN;
}



/*************************************
 *
 *	Mathbox divider
 *
 *************************************/

READ_HANDLER( swmathbx_reh_r )
{
	return (div_result & 0xff00) >> 8;
}


READ_HANDLER( swmathbx_rel_r )
{
	return div_result & 0x00ff;
}


WRITE_HANDLER( swmathbx_w )
{
	data &= 0xff;	/* ASG 971002 -- make sure we only get bytes here */
	switch (offset)
	{
		case 0:	/* mw0 */
			MPA = data << 2;	/* Set starting PROM address */
			run_mbox();			/* and run the Mathbox */
			break;

		case 1:	/* mw1 */
			BIC = (BIC & 0x00ff) | ((data & 0x01) << 8);
			break;

		case 2:	/* mw2 */
			BIC = (BIC & 0x0100) | data;
			break;

		case 4: /* dvsrh */
			divisor = (divisor & 0x00ff) | (data << 8);
			break;

		case 5: /* dvsrl */
			/* Note: Divide is triggered by write to low byte.  This is */
			/*       dependant on the proper 16 bit write order in the  */
			/*       6809 emulation (high bytes, then low byte).        */
			/*       If the Tie fighters look corrupt, he byte order of */
			/*       the 16 bit writes in the 6809 are backwards        */

			divisor = (divisor & 0xff00) | data;

			if (dividend >= 2 * divisor)
				div_result = 0x7fff;
			else
				div_result = (int)(((long)dividend << 14) / (long)divisor);
			break;

		case 6: /* dvddh */
			dividend = (dividend & 0x00ff) | (data << 8);
			break;

		case 7: /* dvddl */
			dividend = (dividend & 0xff00) | (data);
			break;

		default:
			break;
	}
}
