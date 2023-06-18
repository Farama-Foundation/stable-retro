/*****************************************************************************/
/*                                                                           */
/* Module:  TIA Chip Sound Simulator                                         */
/* Purpose: To emulate the sound generation hardware of the Atari TIA chip.  */
/* Author:  Ron Fries                                                        */
/*                                                                           */
/* Revision History:                                                         */
/*    10-Sep-96 - V1.0 - Initial Release                                     */
/*    14-Jan-97 - V1.1 - Cleaned up sound output by eliminating counter      */
/*                       reset.                                              */
/*    30-Oct-98 - Modified for use in MESS by Dan Boris                      */
/*    28-Jul-01 - Added support for sample rates > TIA clock rate,           */
/*                through oversampling                                       */
/*                                                                           */
/*                                                                           */
/*                                                                           */
/*****************************************************************************/
/*                                                                           */
/*                 License Information and Copyright Notice                  */
/*                 ========================================                  */
/*                                                                           */
/* TiaSound is Copyright(c) 1996 by Ron Fries                                */
/*                                                                           */
/* This library is free software; you can redistribute it and/or modify it   */
/* under the terms of version 2 of the GNU Library General Public License    */
/* as published by the Free Software Foundation.                             */
/*                                                                           */
/* This library is distributed in the hope that it will be useful, but       */
/* WITHOUT ANY WARRANTY; without even the implied warranty of                */
/* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library */
/* General Public License for more details.                                  */
/* To obtain a copy of the GNU Library General Public License, write to the  */
/* Free Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.   */
/*                                                                           */
/* Any permitted reproduction of these routines, in whole or in part, must   */
/* bear this legend.                                                         */
/*                                                                           */
/*****************************************************************************/

#include "driver.h"
#include "tiaintf.h"
#include "tiasound.h"

/* number of bits to shift left AUDV0/AUDV1 registers for 16 bit volume */
#define AUDV_SHIFT  10

/* CONSTANT DEFINITIONS */

/* definitions for AUDCx (15, 16) */
#define SET_TO_1     0x00               /* 0000 */
#define POLY4        0x01               /* 0001 */
#define DIV31_POLY4  0x02               /* 0010 */
#define POLY5_POLY4  0x03               /* 0011 */
#define PURE         0x04               /* 0100 */
#define PURE2        0x05               /* 0101 */
#define DIV31_PURE   0x06               /* 0110 */
#define POLY5_2      0x07               /* 0111 */
#define POLY9        0x08               /* 1000 */
#define POLY5        0x09               /* 1001 */
#define DIV31_POLY5  0x0a               /* 1010 */
#define POLY5_POLY5  0x0b               /* 1011 */
#define DIV3_PURE    0x0c               /* 1100 */
#define DIV3_PURE2   0x0d               /* 1101 */
#define DIV93_PURE   0x0e               /* 1110 */
#define DIV3_POLY5   0x0f               /* 1111 */

#ifndef POLY9
#define POLY9       0x80                /* selects POLY9 or POLY17 */
#endif

#define DIV3_MASK    0x0c

#define AUDC0        0x15
#define AUDC1        0x16
#define AUDF0        0x17
#define AUDF1        0x18
#define AUDV0        0x19
#define AUDV1        0x1a

/* the size (in entries) of the 4 polynomial tables */
#define POLY4_SIZE  0x000f
#define POLY5_SIZE  0x001f
#define POLY9_SIZE  0x01ff

/* channel definitions */
#define CHAN1       0
#define CHAN2       1

#ifndef FALSE
#define FALSE       0
#endif
#ifndef TRUE
#define TRUE        1
#endif

/* LOCAL GLOBAL VARIABLE DEFINITIONS */

/* structures to hold the 6 tia sound control bytes */
static UINT8 AUDC[2];                   /* AUDCx (15, 16) */
static UINT8 AUDF[2];                   /* AUDFx (17, 18) */
static INT16 AUDV[2];                   /* AUDVx (19, 1A) */

static INT16 Outvol[2];                 /* last output volume for each channel */

static int tia_gain;					/* initialized in tia_sound_init() */

/* Initialze the bit patterns for the polynomials. */

/* The 4bit and 5bit patterns are the identical ones used in the tia chip. */
/* Though the patterns could be packed with 8 bits per byte, using only a */
/* single bit per byte keeps the math simple, which is important for */
/* efficient processing. */

/* HJB: poly bits are initialized at runtime */

static UINT8 Bit4[POLY4_SIZE];
static UINT8 Bit5[POLY5_SIZE];
static UINT8 Bit9[POLY9_SIZE];

/* I've treated the 'Div by 31' counter as another polynomial because of */
/* the way it operates.  It does not have a 50% duty cycle, but instead */
/* has a 13:18 ratio (of course, 13+18 = 31).  This could also be */
/* implemented by using counters. */

static UINT8 Div31[POLY5_SIZE] =
    {0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};


static UINT8 P4[2];                     /* Position pointer for the 4-bit POLY array */
static UINT8 P5[2];                     /* Position pointer for the 5-bit POLY array */
static UINT16 P9[2];                    /* Position pointer for the 9-bit POLY array */

static UINT8 Div_n_cnt[2];              /* Divide by n counter. one for each channel */
static UINT8 Div_n_max[2];              /* Divide by n maximum, one for each channel */


/* In my routines, I treat the sample output as another divide by N counter. */
/* For better accuracy, the Samp_n_cnt has a fixed binary decimal point */
/* which has 8 binary digits to the right of the decimal point. */

static UINT16 Samp_n_max;               /* Sample max, multiplied by 256 */
static UINT16 Samp_n_cnt;               /* Sample cnt. */

static int oversampling;				/* Added oversampling for sample_rate > clock_rate */

/*****************************************************************************/
/* Module:  tia_sound_w()                                                    */
/* Purpose: To process the latest control values stored in the AUDF, AUDC,   */
/*          and AUDV registers.  It pre-calculates as much information as    */
/*          possible for better performance.  This routine has not been      */
/*          optimized.                                                       */
/*                                                                           */
/* Author:  Ron Fries                                                        */
/* Date:    January 14, 1997                                                 */
/*                                                                           */
/* Inputs:  addr - the address of the parameter to be changed                */
/*          val - the new value to be placed in the specified address        */
/*                                                                           */
/* Outputs: Adjusts local globals - no return value                          */
/*                                                                           */
/*****************************************************************************/

WRITE_HANDLER( tia_sound_w )
{
    UINT16 new_val = 0;
    UINT8 chan;

    tia_sh_update();

    /* determine which address was changed */
    switch (offset)
    {
    case AUDC0:
        AUDC[0] = data & 0x0f;
        chan = 0;
        break;

    case AUDC1:
        AUDC[1] = data & 0x0f;
        chan = 1;
        break;

    case AUDF0:
        AUDF[0] = data & 0x1f;
        chan = 0;
        break;

    case AUDF1:
        AUDF[1] = data & 0x1f;
        chan = 1;
        break;

    case AUDV0:
		AUDV[0] = ((data & 0x0f) << AUDV_SHIFT) * tia_gain / TIA_DEFAULT_GAIN;
        chan = 0;
        break;

    case AUDV1:
		AUDV[1] = ((data & 0x0f) << AUDV_SHIFT) * tia_gain / TIA_DEFAULT_GAIN;
        chan = 1;
        break;

    default:
        chan = 255;
        break;
    }

    /* if the output value changed */
    if (chan != 255)
    {
        /* an AUDC value of 0 is a special case */
        if (AUDC[chan] == SET_TO_1)
        {
            /* indicate the clock is zero so no processing will occur */
            new_val = 0;

            /* and set the output to the selected volume */
            Outvol[chan] = AUDV[chan];
        }
        else
        {
            /* otherwise calculate the 'divide by N' value */
            new_val = AUDF[chan] + 1;

            /* if bits 2 & 3 are set, then multiply the 'div by n' count by 3 */
            if ((AUDC[chan] & DIV3_MASK) == DIV3_MASK)
            {
                new_val *= 3;
            }
        }

        /* only reset those channels that have changed */
        if (new_val != Div_n_max[chan])
        {
            /* reset the divide by n counters */
            Div_n_max[chan] = new_val;

            /* if the channel is now volume only or was volume only */
            if ((Div_n_cnt[chan] == 0) || (new_val == 0))
            {
                /* reset the counter (otherwise let it complete the previous) */
                Div_n_cnt[chan] = new_val;
            }
        }
    }
}


/*****************************************************************************/
/* Module:  tia_process()                                                    */
/* Purpose: To fill the output buffer with the sound output based on the     */
/*          tia chip parameters.  This routine has been optimized.           */
/*                                                                           */
/* Author:  Ron Fries                                                        */
/* Date:    September 10, 1996                                               */
/*                                                                           */
/* Inputs:  *buffer - pointer to the buffer where the audio output will      */
/*                    be placed                                              */
/*          n - size of the playback buffer                                  */
/*                                                                           */
/* Outputs: the buffer will be filled with n bytes of audio - no return val  */
/*                                                                           */
/*****************************************************************************/

void tia_process(int param, INT16 *buffer, int length)
{
    UINT8 audc0, audc1;
    UINT8 div_n_cnt0, div_n_cnt1;
    UINT8 p5_0, p5_1;
    INT16 audv0, audv1, outvol_0, outvol_1;

    audc0 = AUDC[0];
    audc1 = AUDC[1];
    audv0 = AUDV[0];
    audv1 = AUDV[1];

    /* make temporary local copy */
    p5_0 = P5[0];
    p5_1 = P5[1];
    outvol_0 = Outvol[0];
    outvol_1 = Outvol[1];
    div_n_cnt0 = Div_n_cnt[0];
    div_n_cnt1 = Div_n_cnt[1];

    /* loop until the buffer is filled */
    while (length > 0)
    {
        /* Process channel 0 */
        if (div_n_cnt0 > 1)
        {
            div_n_cnt0--;
        }
        else if (div_n_cnt0 == 1)
        {
            div_n_cnt0 = Div_n_max[0];

            /* the P5 counter has multiple uses, so we inc it here */
            p5_0++;
            if (p5_0 == POLY5_SIZE)
                p5_0 = 0;

            /* check clock modifier for clock tick */
            if ((audc0 & 0x02) == 0 ||
                ((audc0 & 0x01) == 0 && Div31[p5_0]) ||
                ((audc0 & 0x01) == 1 && Bit5[p5_0]))
            {
                if (audc0 & 0x04)       /* pure modified clock selected */
                {
                    if (outvol_0)       /* if the output was set */
                        outvol_0 = 0;   /* turn it off */
                    else
                        outvol_0 = audv0;   /* else turn it on */
                }
                else if (audc0 & 0x08)  /* check for p5/p9 */
                {
                    if (audc0 == POLY9) /* check for poly9 */
                    {
                        /* inc the poly9 counter */
                        P9[0]++;
                        if (P9[0] == POLY9_SIZE)
                            P9[0] = 0;

                        if (Bit9[P9[0]])
                            outvol_0 = audv0;
                        else
                            outvol_0 = 0;
                    }
                    else
                    /* must be poly5 */
                    {
                        if (Bit5[p5_0])
                            outvol_0 = audv0;
                        else
                            outvol_0 = 0;
                    }
                }
                else
                /* poly4 is the only remaining option */
                {
                    /* inc the poly4 counter */
                    P4[0]++;
                    if (P4[0] == POLY4_SIZE)
                        P4[0] = 0;

                    if (Bit4[P4[0]])
                        outvol_0 = audv0;
                    else
                        outvol_0 = 0;
                }
            }
        }


        /* Process channel 1 */
        if (div_n_cnt1 > 1)
        {
            div_n_cnt1--;
        }
        else if (div_n_cnt1 == 1)
        {
            div_n_cnt1 = Div_n_max[1];

            /* the P5 counter has multiple uses, so we inc it here */
            p5_1++;
            if (p5_1 == POLY5_SIZE)
                p5_1 = 0;

            /* check clock modifier for clock tick */
            if ((audc1 & 0x02) == 0 ||
                ((audc1 & 0x01) == 0 && Div31[p5_1]) ||
                ((audc1 & 0x01) == 1 && Bit5[p5_1]))
            {
                if (audc1 & 0x04)       /* pure modified clock selected */
                {
                    if (outvol_1)       /* if the output was set */
                        outvol_1 = 0;   /* turn it off */
                    else
                        outvol_1 = audv1;   /* else turn it on */
                }
                else if (audc1 & 0x08)  /* check for p5/p9 */
                {
                    if (audc1 == POLY9) /* check for poly9 */
                    {
                        /* inc the poly9 counter */
                        P9[1]++;
                        if (P9[1] == POLY9_SIZE)
                            P9[1] = 0;

                        if (Bit9[P9[1]])
                            outvol_1 = audv1;
                        else
                            outvol_1 = 0;
                    }
                    else
                        /* must be poly5 */
                    {
                        if (Bit5[p5_1])
                            outvol_1 = audv1;
                        else
                            outvol_1 = 0;
                    }
                }
                else
                    /* poly4 is the only remaining option */
                {
                    /* inc the poly4 counter */
                    P4[1]++;
                    if (P4[1] == POLY4_SIZE)
                        P4[1] = 0;

                    if (Bit4[P4[1]])
                        outvol_1 = audv1;
                    else
                        outvol_1 = 0;
                }
            }
        }

		if (!oversampling)
		{
			/* decrement the sample counter - value is 256 since the lower
			 * byte contains the fractional part */
			Samp_n_cnt -= 256;

			/* if the count down has reached zero */
			if (Samp_n_cnt < 256)
			{
				/* adjust the sample counter */
				Samp_n_cnt += Samp_n_max;

				/* calculate the latest output value and place in buffer */
				*buffer++ = outvol_0 + outvol_1;

				/* and indicate one less byte to process */
				length--;
			}
		}
		else
		{
			do
			{
				/* decrement the sample counter - value is 256 since the lower
				 * byte contains the fractional part */
				Samp_n_cnt -= 256;
				/* calculate the latest output value and place in buffer */
				*buffer++ = outvol_0 + outvol_1;
				length--;
			}
			while ((Samp_n_cnt >= 256) && (length > 0));

			/* adjust the sample counter if necessary */
			if (Samp_n_cnt < 256)
				Samp_n_cnt += Samp_n_max;
		}
    }

    /* save for next round */
    P5[0] = p5_0;
    P5[1] = p5_1;
    Outvol[0] = outvol_0;
    Outvol[1] = outvol_1;
    Div_n_cnt[0] = div_n_cnt0;
    Div_n_cnt[1] = div_n_cnt1;

}

static void poly_init(UINT8 *poly, int size, int left, int right, int add)
{
    int mask = (1 << size) - 1;
    int i, x = 0;

    for (i = 0; i < mask; i++)
    {
        *poly++ = x & 1;
        /* calculate next bit */
        x = ((x << left) + (x >> right) + add) & mask;
    }
}

/*****************************************************************************/
/* Module:  tia_sh_start()                                                   */
/* Purpose: to handle the power-up initialization functions                  */
/*          these functions should only be executed on a cold-restart        */
/*                                                                           */
/* Author:  Ron Fries                                                        */
/* Date:    September 10, 1996                                               */
/*                                                                           */
/* Inputs:  struct MachineSound *msound                                      */
/*          is a pointer to the struct TIAInterface parameters               */
/*                                                                           */
/* Outputs: returns zero on success                                          */
/*                                                                           */
/*****************************************************************************/

void tia_sound_init(int clock, int sample_rate, int gain)
{
	int chan;

	/* set the gain factor (normally use TIA_DEFAULT_GAIN) */
    tia_gain = gain;

	/* fill the polynomials */
    poly_init(Bit4, 4, 3, 1, 0x00004);
    poly_init(Bit5, 5, 3, 2, 0x00008);
    poly_init(Bit9, 9, 2, 7, 0x00080);

    /* calculate the sample 'divide by N' value based on the playback freq. */
    Samp_n_max = ((UINT16)(UINT32)clock << 8) / sample_rate;
    Samp_n_cnt = Samp_n_max;                     /* initialize all bits of the sample counter */

	if (Samp_n_max < 256) /* we need to use oversampling for sample_rate > clock_rate */
	                      /* at the Machine->sample_rate for now */
	{
    	Samp_n_max = ((UINT16)(UINT32)sample_rate << 8) / clock;
		Samp_n_cnt = Samp_n_max;
		oversampling = 1;
	}

    /* initialize the local globals */
    for (chan = CHAN1; chan <= CHAN2; chan++)
    {
        Outvol[chan] = 0;
        Div_n_cnt[chan] = 0;
        Div_n_max[chan] = 0;
        AUDC[chan] = 0;
        AUDF[chan] = 0;
        AUDV[chan] = 0;
        P4[chan] = 0;
        P5[chan] = 0;
        P9[chan] = 0;
    }
}
