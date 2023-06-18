/**********************************************************************************************

     TMS5110 simulator (modified from TMS5220 by Jarek Burczynski)

     Written for MAME by Frank Palazzolo
     With help from Neill Corlett
     Additional tweaking by Aaron Giles

***********************************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "driver.h"
#include "tms5110.h"


/* Pull in the ROM tables */
#include "tms5110r.c"


/* these contain data that describes the 64 bits FIFO */
#define FIFO_SIZE 64
static UINT8 fifo[FIFO_SIZE];
static UINT8 fifo_head;
static UINT8 fifo_tail;
static UINT8 fifo_count;

/* these contain global status bits */
static UINT8 PDC;
static UINT8 CTL_pins;
static UINT8 speaking_now;
static UINT8 speak_delay_frames;
static UINT8 talk_status;


static int (*M0_callback)(void);

/* these contain data describing the current and previous voice frames */
static UINT16 old_energy;
static UINT16 old_pitch;
static int old_k[10];

static UINT16 new_energy;
static UINT16 new_pitch;
static int new_k[10];


/* these are all used to contain the current state of the sound generation */
static UINT16 current_energy;
static UINT16 current_pitch;
static int current_k[10];

static UINT16 target_energy;
static UINT16 target_pitch;
static int target_k[10];

static UINT8 interp_count;       /* number of interp periods (0-7) */
static UINT8 sample_count;       /* sample number within interp (0-24) */
static int pitch_count;

static int u[11];
static int x[10];

static INT8 randbit;


/* Static function prototypes */
static int parse_frame(int removeit);


#define DEBUG_5110	0


/**********************************************************************************************

     tms5110_reset -- resets the TMS5110

***********************************************************************************************/

void tms5110_reset(void)
{
    /* initialize the FIFO */
    memset(fifo, 0, sizeof(fifo));
    fifo_head = fifo_tail = fifo_count = 0;

    /* initialize the chip state */
    speaking_now = speak_delay_frames = talk_status = 0;
    CTL_pins = 0;

    /* initialize the energy/pitch/k states */
    old_energy = new_energy = current_energy = target_energy = 0;
    old_pitch = new_pitch = current_pitch = target_pitch = 0;
    memset(old_k, 0, sizeof(old_k));
    memset(new_k, 0, sizeof(new_k));
    memset(current_k, 0, sizeof(current_k));
    memset(target_k, 0, sizeof(target_k));

    /* initialize the sample generators */
    interp_count = sample_count = pitch_count = 0;
    randbit = 0;
    memset(u, 0, sizeof(u));
    memset(x, 0, sizeof(x));
}



/******************************************************************************************

     tms5110_set_M0_callback -- set M0 callback for the TMS5110

******************************************************************************************/

void tms5110_set_M0_callback(int (*func)(void))
{
    M0_callback = func;
}


/******************************************************************************************

     FIFO_data_write -- handle bit data write to the TMS5110 (as a result of toggling M0 pin)

******************************************************************************************/
static void FIFO_data_write(int data)
{
	/* add this byte to the FIFO */
	if (fifo_count < FIFO_SIZE)
	{
		fifo[fifo_tail] = (data&1); /* set bit to 1 or 0 */

		fifo_tail = (fifo_tail + 1) % FIFO_SIZE;
		fifo_count++;

		log_cb(RETRO_LOG_DEBUG, LOGPRE "Added bit to FIFO (size=%2d)\n", fifo_count);
	}
	else
	{
		log_cb(RETRO_LOG_DEBUG, LOGPRE "Ran out of room in the FIFO!\n");
	}
}

/******************************************************************************************

     extract_bits -- extract a specific number of bits from the FIFO

******************************************************************************************/

static int extract_bits(int count)
{
    int val = 0;

    while (count--)
    {
        val = (val << 1) | (fifo[fifo_head] & 1);
        fifo_count--;
        fifo_head = (fifo_head + 1) % FIFO_SIZE;
    }
    return val;
}

static void request_bits(int no)
{
int i;
	for (i=0; i<no; i++)
	{
		if (M0_callback)
		{
			int data = (*M0_callback)();
			FIFO_data_write(data);
		}
		else
			log_cb(RETRO_LOG_DEBUG, LOGPRE "-->ERROR: TMS5110 missing M0 callback function\n");
	}
}

static void perform_dummy_read(void)
{
	if (M0_callback)
	{
		int data = (*M0_callback)();
	        log_cb(RETRO_LOG_DEBUG, LOGPRE "TMS5110 performing dummy read; value read = %1i\n", data&1);
	}
    	else
	        log_cb(RETRO_LOG_DEBUG, LOGPRE "-->ERROR: TMS5110 missing M0 callback function\n");
}

/**********************************************************************************************

     tms5110_status_read -- read status from the TMS5110

        bit 0 = TS - Talk Status is active (high) when the VSP is processing speech data.
                Talk Status goes active at the initiation of a SPEAK command.
                It goes inactive (low) when the stop code (Energy=1111) is processed, or
                immediately(?????? not TMS5110) by a RESET command.
		TMS5110 datasheets mention this is only available as a result of executing
                TEST TALK command.


***********************************************************************************************/

int tms5110_status_read(void)
{

    log_cb(RETRO_LOG_DEBUG, LOGPRE "Status read: TS=%d\n", talk_status);

    return (talk_status << 0); /*CTL1 = still talking ? */
}



/**********************************************************************************************

     tms5110_ready_read -- returns the ready state of the TMS5110

***********************************************************************************************/

int tms5110_ready_read(void)
{
    return (fifo_count < FIFO_SIZE-1);
}



/**********************************************************************************************

     tms5110_process -- fill the buffer with a specific number of samples

***********************************************************************************************/

void tms5110_process(INT16 *buffer, unsigned int size)
{
    int buf_count=0;
    int i, interp_period;

/* tryagain: */

    /* if we're not speaking, fill with nothingness */
    if (!speaking_now)
        goto empty;

    /* if we're to speak, but haven't started */
    if (!talk_status)
    {

/*"perform dummy read" is not mentioned in the datasheet */
/* However Bagman speech roms data are organized in such way so bit at address 0
** is NOT a speech data. Bit at address 1 is data speech. Seems that the
** tms5110 is performing dummy read before starting to execute a SPEAK command.
*/
		perform_dummy_read();

        /* parse but don't remove the first frame, and set the status to 1 */
        parse_frame(1);
        talk_status = 1;
    }
#if 0
    /* apply some delay before we actually consume data */
    if (speak_delay_frames)
    {
    	if (size <= speak_delay_frames)
    	{
    		speak_delay_frames -= size;
    		size = 0;
    	}
    	else
    	{
    		size -= speak_delay_frames;
    		speak_delay_frames = 0;
    	}
    }
#endif

    /* loop until the buffer is full or we've stopped speaking */
    while ((size > 0) && speaking_now)
    {
        int current_val;

        /* if we're ready for a new frame */
        if ((interp_count == 0) && (sample_count == 0))
        {
            /* Parse a new frame */
            if (!parse_frame(1))
                break;

            /* Set old target as new start of frame */
            current_energy = old_energy;
            current_pitch = old_pitch;
            for (i = 0; i < 10; i++)
                current_k[i] = old_k[i];

            /* is this a zero energy frame? */
            if (current_energy == 0)
            {
                /*logerror("processing frame: zero energy\n");*/
                target_energy = 0;
                target_pitch = current_pitch;
                for (i = 0; i < 10; i++)
                    target_k[i] = current_k[i];
            }
            /* is this a stop frame? */
            else if (current_energy == (energytable[15] >> 6))
            {
                log_cb(RETRO_LOG_DEBUG, LOGPRE "processing frame: stop frame\n");
                current_energy = energytable[0] >> 6;
                target_energy = current_energy;
                speaking_now = talk_status = 0;
                interp_count = sample_count = pitch_count = 0;

                /* try to fetch commands again */
                /*goto tryagain;*/
            }
            else
            {
                /* is this the ramp down frame? */
                if (new_energy == (energytable[15] >> 6))
                {
                    /*logerror("processing frame: ramp down\n");*/
                    target_energy = 0;
                    target_pitch = current_pitch;
                    for (i = 0; i < 10; i++)
                        target_k[i] = current_k[i];
                }
                /* Reset the step size */
                else
                {
                    /*logerror("processing frame: Normal\n");*/
                    /*logerror("*** Energy = %d\n",current_energy);*/
                    /*logerror("proc: %d %d\n",last_fbuf_head,fbuf_head);*/

                    target_energy = new_energy;
                    target_pitch = new_pitch;

                    for (i = 0; i < 4; i++)
                        target_k[i] = new_k[i];
                    if (current_pitch == 0)
                        for (i = 4; i < 10; i++)
                        {
                            target_k[i] = current_k[i] = 0;
                        }
                    else
                        for (i = 4; i < 10; i++)
                            target_k[i] = new_k[i];
                }
            }
        }
        else if (interp_count == 0)
        {
            /* Update values based on step values */
            /*logerror("\n");*/

            interp_period = sample_count / 25;
            current_energy += (target_energy - current_energy) / interp_coeff[interp_period];
            if (old_pitch != 0)
                current_pitch += (target_pitch - current_pitch) / interp_coeff[interp_period];

            /*logerror("*** Energy = %d\n",current_energy);*/

            for (i = 0; i < 10; i++)
            {
                current_k[i] += (target_k[i] - current_k[i]) / interp_coeff[interp_period];
            }
        }

        if (old_energy == 0)
        {
            /* generate silent samples here */
            current_val = 0x00;
        }
        else if (old_pitch == 0)
        {
            /* generate unvoiced samples here */
            randbit = (rand() % 2) * 2 - 1;
            current_val = (randbit * current_energy) / 4;
        }
        else
        {
            /* generate voiced samples here */
            if (pitch_count < sizeof(chirptable))
                current_val = (chirptable[pitch_count] * current_energy) / 256;
            else
                current_val = 0x00;
        }

        /* Lattice filter here */

        u[10] = current_val;

        for (i = 9; i >= 0; i--)
        {
            u[i] = u[i+1] - ((current_k[i] * x[i]) / 32768);
        }
        for (i = 9; i >= 1; i--)
        {
            x[i] = x[i-1] + ((current_k[i-1] * u[i-1]) / 32768);
        }

        x[0] = u[0];

        /* clipping, just like the chip */

        if (u[0] > 511)
            buffer[buf_count] = 127<<8;
        else if (u[0] < -512)
            buffer[buf_count] = -128<<8;
        else
            buffer[buf_count] = u[0] << 6;

        /* Update all counts */

        size--;
        sample_count = (sample_count + 1) % 200;

        if (current_pitch != 0)
            pitch_count = (pitch_count + 1) % current_pitch;
        else
            pitch_count = 0;

        interp_count = (interp_count + 1) % 25;
        buf_count++;
    }

empty:

    while (size > 0)
    {
        buffer[buf_count] = 0x00;
        buf_count++;
        size--;
    }
}




/******************************************************************************************

     CTL_set -- set CTL pins named CTL1, CTL2, CTL4 and CTL8

******************************************************************************************/

void tms5110_CTL_set(int data)
{
	CTL_pins = data & 0xf;
}


/******************************************************************************************

     PDC_set -- set Processor Data Clock. Execute CTL_pins command on hi-lo transition.

******************************************************************************************/

void tms5110_PDC_set(int data)
{
	if (PDC != (data & 0x1) )
	{
		PDC = data & 0x1;
		if (PDC == 0) /* toggling 1->0 processes command on CTL_pins */
		{
			/* only real commands we handle now are SPEAK and RESET */

			switch (CTL_pins & 0xe) /*CTL1 - don't care*/
			{
			case TMS5110_CMD_SPEAK:
				speaking_now = 1;
				/*speak_delay_frames = 10;*/

				/*should FIFO be cleared now ?????*/

				break;

        		case TMS5110_CMD_RESET:
				speaking_now = 0;
				talk_status = 0;
				break;

			default:
				break;
			}
		}
	}
}



/******************************************************************************************

     parse_frame -- parse a new frame's worth of data; returns 0 if not enough bits in buffer

******************************************************************************************/

static int parse_frame(int removeit)
{
    int old_head, old_count;
    int bits, indx, i, rep_flag;

    /* remember previous frame */
    old_energy = new_energy;
    old_pitch = new_pitch;
    for (i = 0; i < 10; i++)
        old_k[i] = new_k[i];

    /* clear out the new frame */
    new_energy = 0;
    new_pitch = 0;
    for (i = 0; i < 10; i++)
        new_k[i] = 0;

    /* if the previous frame was a stop frame, don't do anything */
    if (old_energy == (energytable[15] >> 6))
        return 1;

    /* remember the original FIFO counts, in case we don't have enough bits */
    old_count = fifo_count;
    old_head = fifo_head;

    /* count the total number of bits available */
    bits = fifo_count;

    /* attempt to extract the energy index */
    bits -= 4;
    if (bits < 0)
    {
        request_bits( -bits ); /* toggle M0 to receive needed bits */
	bits = 0;
    }
    indx = extract_bits(4);
    new_energy = energytable[indx] >> 6;

	/* if the index is 0 or 15, we're done */
	if (indx == 0 || indx == 15)
	{
		log_cb(RETRO_LOG_DEBUG, LOGPRE "  (4-bit energy=%d frame)\n",new_energy);

		/* clear fifo if stop frame encountered */
		if (indx == 15)
		{
			log_cb(RETRO_LOG_DEBUG, LOGPRE "  (4-bit energy=%d STOP frame)\n",new_energy);
			fifo_head = fifo_tail = fifo_count = 0;
			removeit = 1;
            speaking_now = talk_status = 0;
		}
		goto done;
	}

    /* attempt to extract the repeat flag */
    bits -= 1;
    if (bits < 0)
    {
        request_bits( -bits ); /* toggle M0 to receive needed bits */
	bits = 0;
    }
    rep_flag = extract_bits(1);

    /* attempt to extract the pitch */
    bits -= 5;
    if (bits < 0)
    {
        request_bits( -bits ); /* toggle M0 to receive needed bits */
        bits = 0;
    }
    indx = extract_bits(5);
    new_pitch = pitchtable[indx] / 256;

    /* if this is a repeat frame, just copy the k's */
    if (rep_flag)
    {
        for (i = 0; i < 10; i++)
            new_k[i] = old_k[i];

        log_cb(RETRO_LOG_DEBUG, LOGPRE "  (10-bit energy=%d pitch=%d rep=%d frame)\n", new_energy, new_pitch, rep_flag);
        goto done;
    }

    /* if the pitch index was zero, we need 4 k's */
    if (indx == 0)
    {
        /* attempt to extract 4 K's */
        bits -= 18;
        if (bits < 0)
        {
		request_bits( -bits ); /* toggle M0 to receive needed bits */
		bits = 0;
        }
        new_k[0] = k1table[extract_bits(5)];
        new_k[1] = k2table[extract_bits(5)];
        new_k[2] = k3table[extract_bits(4)];
        new_k[3] = k4table[extract_bits(4)];

        log_cb(RETRO_LOG_DEBUG, LOGPRE "  (28-bit energy=%d pitch=%d rep=%d 4K frame)\n", new_energy, new_pitch, rep_flag);
        goto done;
    }

    /* else we need 10 K's */
    bits -= 39;
    if (bits < 0)
    {
	        request_bits( -bits ); /* toggle M0 to receive needed bits */
		bits = 0;
    }
    new_k[0] = k1table[extract_bits(5)];
    new_k[1] = k2table[extract_bits(5)];
    new_k[2] = k3table[extract_bits(4)];
    new_k[3] = k4table[extract_bits(4)];
    new_k[4] = k5table[extract_bits(4)];
    new_k[5] = k6table[extract_bits(4)];
    new_k[6] = k7table[extract_bits(4)];
    new_k[7] = k8table[extract_bits(3)];
    new_k[8] = k9table[extract_bits(3)];
    new_k[9] = k10table[extract_bits(3)];

    log_cb(RETRO_LOG_DEBUG, LOGPRE "  (49-bit energy=%d pitch=%d rep=%d 10K frame)\n", new_energy, new_pitch, rep_flag);

done:

    log_cb(RETRO_LOG_DEBUG, LOGPRE "Parsed a frame successfully - %d bits remaining\n", bits);

#if 0
    /* if we're not to remove this one, restore the FIFO */
    if (!removeit)
    {
        fifo_count = old_count;
        fifo_head = old_head;
    }
#endif

    return 1;

}



#if 0
/*This is an example word TEN taken from the TMS5110A datasheet*/
static unsigned int example_word_TEN[619]={
/* 1*/1,0,0,0,	0,	0,0,0,0,0,	1,1,0,0,0,	0,0,0,1,0,	0,1,1,1,	0,1,0,1,
/* 2*/1,0,0,0,	0,	0,0,0,0,0,	1,0,0,1,0,	0,0,1,1,0,	0,0,1,1,	0,1,0,1,
/* 3*/1,1,0,0,	0,	1,0,0,0,0,	1,0,1,0,0,	0,1,0,1,0,	0,1,0,0,	1,0,1,0,	1,0,0,0,	1,0,0,1,	0,1,0,1,	0,0,1,	0,1,0,	0,1,1,
/* 4*/1,1,1,0,	0,	0,1,1,1,1,	1,0,1,0,1,	0,1,1,1,0,	0,1,0,1,	0,1,1,1,	0,1,1,1,	1,0,1,1,	1,0,1,0,	0,1,1,	0,1,0,	0,1,1,
/* 5*/1,1,1,0,	0,	1,0,0,0,0,	1,0,1,0,0,	0,1,1,1,0,	0,1,0,1,	1,0,1,0,	1,0,0,0,	1,1,0,0,	1,0,1,1,	1,0,0,	0,1,0,	0,1,1,
/* 6*/1,1,1,0,	0,	1,0,0,0,1,	1,0,1,0,1,	0,1,1,0,1,	0,1,1,0,	0,1,1,1,	0,1,1,1,	1,0,1,0,	1,0,1,0,	1,1,0,	0,0,1,	1,0,0,
/* 7*/1,1,1,0,	0,	1,0,0,1,0,	1,0,1,1,1,	0,1,1,1,0,	0,1,1,1,	0,1,1,1,	0,1,0,1,	0,1,1,0,	1,0,0,1,	1,1,0,	0,1,0,	0,1,1,
/* 8*/1,1,1,0,	1,	1,0,1,0,1,
/* 9*/1,1,1,0,	0,	1,1,0,0,1,	1,0,1,1,1,	0,1,0,1,1,	1,0,1,1,	0,1,1,1,	0,1,0,0,	1,0,0,0,	1,0,0,0,	1,1,0,	0,1,1,	0,1,1,
/*10*/1,1,0,1,	0,	1,1,0,1,0,	1,0,1,0,1,	0,1,1,0,1,	1,0,1,1,	0,1,0,1,	0,1,0,0,	1,0,0,0,	1,0,1,0,	1,1,0,	0,1,0,	1,0,0,
/*11*/1,0,1,1,	0,	1,1,0,1,1,	1,0,0,1,1,	1,0,0,1,0,	0,1,1,0,	0,0,1,1,	0,1,0,1,	1,0,0,1,	1,0,1,0,	1,0,0,	0,1,1,	0,1,1,
/*12*/1,0,0,0,	0,	1,1,1,0,0,	1,0,0,1,1,	0,0,1,1,0,	0,1,0,0,	0,1,1,0,	1,1,0,0,	0,1,0,1,	1,0,0,0,	1,0,0,	0,1,0,	1,0,1,
/*13*/0,1,1,1,	1,	1,1,1,0,1,
/*14*/0,1,1,1,	0,	1,1,1,1,0,	1,0,0,1,1,	0,0,1,1,1,	0,1,0,1,	0,1,0,1,	1,1,0,0,	0,1,1,1,	1,0,0,0,	1,0,0,	0,1,0,	1,0,1,
/*15*/0,1,1,0,	0,	1,1,1,1,0,	1,0,1,0,1,	0,0,1,1,0,	0,1,0,0,	0,0,1,1,	1,1,0,0,	1,0,0,1,	0,1,1,1,	1,0,1,	0,1,0,	1,0,1,
/*16*/1,1,1,1
};
#endif
