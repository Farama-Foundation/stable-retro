/**********************************************************************************************

     TMS5220 simulator

     Written for MAME by Frank Palazzolo
     With help from Neill Corlett
     Additional tweaking by Aaron Giles

***********************************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "driver.h"
#include "tms5220.h"


/* Pull in the ROM tables */
#include "tms5220r.c"

/*
  Changes by R. Nabet
   * added Speech ROM support
   * modified code so that the beast only start speaking at the start of next frame, like the data
     sheet says
*/
#define USE_OBSOLETE_HACK 0


#ifndef TRUE
	#define TRUE 1
#endif
#ifndef FALSE
	#define FALSE 0
#endif


/* these contain data that describes the 128-bit data FIFO */
#define FIFO_SIZE 16
static UINT8 fifo[FIFO_SIZE];
static UINT8 fifo_head;
static UINT8 fifo_tail;
static UINT8 fifo_count;
static UINT8 fifo_bits_taken;


/* these contain global status bits */
/*
	R Nabet : speak_external is only set when a speak external command is going on.
	tms5220_speaking is set whenever a speak or speak external command is going on.
	Note that we really need to do anything in tms5220_process and play samples only when
	tms5220_speaking is true.  Else, we can play nothing as well, which is a
	speed-up...
*/
static UINT8 tms5220_speaking;	/* Speak or Speak External command in progress */
static UINT8 speak_external;	/* Speak External command in progress */
#if USE_OBSOLETE_HACK
static UINT8 speak_delay_frames;
#endif
static UINT8 talk_status; 		/* tms5220 is really currently speaking */
static UINT8 first_frame;		/* we have just started speaking, and we are to parse the first frame */
static UINT8 last_frame;		/* we are doing the frame of sound */
static UINT8 buffer_low;		/* FIFO has less than 8 bytes in it */
static UINT8 buffer_empty;		/* FIFO is empty*/
static UINT8 irq_pin;			/* state of the IRQ pin (output) */

static void (*irq_func)(int state); /* called when the state of the IRQ pin changes */


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

static UINT8 interp_count;		/* number of interp periods (0-7) */
static UINT8 sample_count;		/* sample number within interp (0-24) */
static int pitch_count;

static int u[11];
static int x[10];

static INT8 randbit;


/* Static function prototypes */
static void process_command(void);
static int extract_bits(int count);
static int parse_frame(int the_first_frame);
static void check_buffer_low(void);
static void set_interrupt_state(int state);


#define DEBUG_5220	0



/* R Nabet : These have been added to emulate speech Roms */
static int (*read_callback)(int count) = NULL;
static void (*load_address_callback)(int data) = NULL;
static void (*read_and_branch_callback)(void) = NULL;
static int schedule_dummy_read;			/* set after each load address, so that next read operation
										  is preceded by a dummy read */

static UINT8 data_register;				/* data register, used by read command */
static int RDB_flag;					/* whether we should read data register or status register */

/* flag for tms0285 emulation */
/* The tms0285 is an early variant of the tms5220 used in the ti-99/4(a)
computer.  The exact relationship of this chip with tms5200 & tms5220 is
unknown, but it seems to use slightly different tables for LPC parameters. */
static tms5220_variant variant;

/**********************************************************************************************

     tms5220_reset -- resets the TMS5220

***********************************************************************************************/

void tms5220_reset(void)
{
	/* initialize the FIFO */
	/*memset(fifo, 0, sizeof(fifo));*/
	fifo_head = fifo_tail = fifo_count = fifo_bits_taken = 0;

	/* initialize the chip state */
	/* Note that we do not actually clear IRQ on start-up : IRQ is even raised if buffer_empty or buffer_low are 0 */
	tms5220_speaking = speak_external = talk_status = first_frame = last_frame = irq_pin = 0;
#if USE_OBSOLETE_HACK
	speak_delay_frames = 0;
#endif
	if (irq_func) irq_func(0);
	buffer_empty = buffer_low = 1;

	RDB_flag = FALSE;

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

	if (load_address_callback)
		(*load_address_callback)(0);

	schedule_dummy_read = TRUE;
}



/**********************************************************************************************

     tms5220_set_irq -- sets the interrupt handler

***********************************************************************************************/

void tms5220_set_irq(void (*func)(int))
{
    irq_func = func;
}


/**********************************************************************************************

     tms5220_set_read -- sets the speech ROM read handler

***********************************************************************************************/

void tms5220_set_read(int (*func)(int))
{
	read_callback = func;
}


/**********************************************************************************************

     tms5220_set_load_address -- sets the speech ROM load address handler

***********************************************************************************************/

void tms5220_set_load_address(void (*func)(int))
{
	load_address_callback = func;
}


/**********************************************************************************************

     tms5220_set_read_and_branch -- sets the speech ROM read and branch handler

***********************************************************************************************/

void tms5220_set_read_and_branch(void (*func)(void))
{
	read_and_branch_callback = func;
}


/**********************************************************************************************

     tms5220_set_variant -- sets the tms5220 core to emulate its buggy forerunner, the tms0285

***********************************************************************************************/

void tms5220_set_variant(tms5220_variant new_variant)
{
	variant = new_variant;
}


/**********************************************************************************************

     tms5220_data_write -- handle a write to the TMS5220

***********************************************************************************************/

void tms5220_data_write(int data)
{
    /* add this byte to the FIFO */
    if (fifo_count < FIFO_SIZE)
    {
        fifo[fifo_tail] = data;
        fifo_tail = (fifo_tail + 1) % FIFO_SIZE;
        fifo_count++;

		/* if we were speaking, then we're no longer empty */
		if (speak_external)
			buffer_empty = 0;

        log_cb(RETRO_LOG_DEBUG, LOGPRE "Added byte to FIFO (size=%2d)\n", fifo_count);
    }
    else
    {
        log_cb(RETRO_LOG_DEBUG, LOGPRE "Ran out of room in the FIFO!\n");
    }

    /* update the buffer low state */
    check_buffer_low();

	if (! speak_external)
		/* R Nabet : we parse commands at once.  It is necessary for such commands as read. */
		process_command (/*data*/);
}


/**********************************************************************************************

     tms5220_status_read -- read status or data from the TMS5220

	  From the data sheet:
        bit 0 = TS - Talk Status is active (high) when the VSP is processing speech data.
                Talk Status goes active at the initiation of a Speak command or after nine
                bytes of data are loaded into the FIFO following a Speak External command. It
                goes inactive (low) when the stop code (Energy=1111) is processed, or
                immediately by a buffer empty condition or a reset command.
        bit 1 = BL - Buffer Low is active (high) when the FIFO buffer is more than half empty.
                Buffer Low is set when the "Last-In" byte is shifted down past the half-full
                boundary of the stack. Buffer Low is cleared when data is loaded to the stack
                so that the "Last-In" byte lies above the half-full boundary and becomes the
                ninth data byte of the stack.
        bit 2 = BE - Buffer Empty is active (high) when the FIFO buffer has run out of data
                while executing a Speak External command. Buffer Empty is set when the last bit
                of the "Last-In" byte is shifted out to the Synthesis Section. This causes
                Talk Status to be cleared. Speed is terminated at some abnormal point and the
                Speak External command execution is terminated.

***********************************************************************************************/

int tms5220_status_read(void)
{
	if (RDB_flag)
	{	/* if last command was read, return data register */
		RDB_flag = FALSE;
		return(data_register);
	}
	else
	{	/* read status */

		/* clear the interrupt pin */
		set_interrupt_state(0);

		log_cb(RETRO_LOG_DEBUG, LOGPRE "Status read: TS=%d BL=%d BE=%d\n", talk_status, buffer_low, buffer_empty);

		return (talk_status << 7) | (buffer_low << 6) | (buffer_empty << 5);
	}
}



/**********************************************************************************************

     tms5220_ready_read -- returns the ready state of the TMS5220

***********************************************************************************************/

int tms5220_ready_read(void)
{
    return (fifo_count < FIFO_SIZE-1);
}


/**********************************************************************************************

     tms5220_ready_read -- returns the number of cycles until ready is asserted

***********************************************************************************************/

int tms5220_cycles_to_ready(void)
{
	int answer;


	if (tms5220_ready_read())
		answer = 0;
	else
	{
		int val;

		answer = 200-sample_count+1;

		/* total number of bits available in current byte is (8 - fifo_bits_taken) */
		/* if more than 4 are available, we need to check the energy */
		if (fifo_bits_taken < 4)
		{
			/* read energy */
			val = (fifo[fifo_head] >> fifo_bits_taken) & 0xf;
			if (val == 0)
				/* 0 -> silence frame: we will only read 4 bits, and we will
				therefore need to read another frame before the FIFO is not
				full any more */
				answer += 200;
			/* 15 -> stop frame, we will only read 4 bits, but the FIFO will
			we cleared */
			/* otherwise, we need to parse the repeat flag (1 bit) and the
			pitch (6 bits), so everything will be OK. */
		}
	}

	return answer;
}


/**********************************************************************************************

     tms5220_int_read -- returns the interrupt state of the TMS5220

***********************************************************************************************/

int tms5220_int_read(void)
{
    return irq_pin;
}



/**********************************************************************************************

     tms5220_process -- fill the buffer with a specific number of samples

***********************************************************************************************/

void tms5220_process(INT16 *buffer, unsigned int size)
{
    int buf_count=0;
    int i, interp_period;

tryagain:

    /* if we're not speaking, parse commands */
	/*while (!speak_external && fifo_count > 0)
		process_command();*/

    /* if we're empty and still not speaking, fill with nothingness */
	if ((!tms5220_speaking) && (!last_frame))
        goto empty;

    /* if we're to speak, but haven't started, wait for the 9th byte */
	if (!talk_status && speak_external)
    {
        if (fifo_count < 9)
           goto empty;

        talk_status = 1;
		first_frame = 1;	/* will cause the first frame to be parsed */
		buffer_empty = 0;
	}

#if 0
	/* we are to speak, yet we fill with 0s until start of next frame */
	if (first_frame)
	{
		while ((size > 0) && ((sample_count != 0) || (interp_count != 0)))
		{
			sample_count = (sample_count + 1) % 200;
			interp_count = (interp_count + 1) % 25;
			buffer[buf_count] = 0x00;	/* should be (-1 << 8) ??? (cf note in data sheet, p 10, table 4) */
			buf_count++;
			size--;
		}
	}
#endif

#if USE_OBSOLETE_HACK
    /* apply some delay before we actually consume data; Victory requires this */
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
	while ((size > 0) && talk_status)
    {
        int current_val;

        /* if we're ready for a new frame */
        if ((interp_count == 0) && (sample_count == 0))
        {
            /* Parse a new frame */
			if (!parse_frame(first_frame))
				break;
			first_frame = 0;

            /* Set old target as new start of frame */
            current_energy = old_energy;
            current_pitch = old_pitch;
            for (i = 0; i < 10; i++)
                current_k[i] = old_k[i];

            /* is this a zero energy frame? */
            if (current_energy == 0)
            {
                /*printf("processing frame: zero energy\n");*/
                target_energy = 0;
                target_pitch = current_pitch;
                for (i = 0; i < 10; i++)
                    target_k[i] = current_k[i];
            }

            /* is this a stop frame? */
            else if (current_energy == (energytable[15] >> 6))
            {
                /*printf("processing frame: stop frame\n");*/
                current_energy = energytable[0] >> 6;
                target_energy = current_energy;
				/*interp_count = sample_count =*/ pitch_count = 0;
				last_frame = 0;
				if (tms5220_speaking)
					/* new speech command in progress */
					first_frame = 1;
				else
				{
					/* really stop speaking */
					talk_status = 0;

					/* generate an interrupt if necessary */
					set_interrupt_state(1);
				}

                /* try to fetch commands again */
                goto tryagain;
            }
            else
            {
                /* is this the ramp down frame? */
                if (new_energy == (energytable[15] >> 6))
                {
                    /*printf("processing frame: ramp down\n");*/
                    target_energy = 0;
                    target_pitch = current_pitch;
                    for (i = 0; i < 10; i++)
                        target_k[i] = current_k[i];
                }
                /* Reset the step size */
                else
                {
                    /*printf("processing frame: Normal\n");*/
                    /*printf("*** Energy = %d\n",current_energy);*/
                    /*printf("proc: %d %d\n",last_fbuf_head,fbuf_head);*/

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
            /*printf("\n");*/

            interp_period = sample_count / 25;
            current_energy += (target_energy - current_energy) / interp_coeff[interp_period];
            if (old_pitch != 0)
                current_pitch += (target_pitch - current_pitch) / interp_coeff[interp_period];

            /*printf("*** Energy = %d\n",current_energy);*/

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
		sample_count = (sample_count + 1) % 200;
		interp_count = (interp_count + 1) % 25;
		buffer[buf_count] = 0x00;	/* should be (-1 << 8) ??? (cf note in data sheet, p 10, table 4) */
        buf_count++;
        size--;
    }
}



/**********************************************************************************************

     process_command -- extract a byte from the FIFO and interpret it as a command

***********************************************************************************************/

static void process_command(void)
{
    unsigned char cmd;

    /* if there are stray bits, ignore them */
	if (fifo_bits_taken)
	{
		fifo_bits_taken = 0;
        fifo_count--;
        fifo_head = (fifo_head + 1) % FIFO_SIZE;
    }

    /* grab a full byte from the FIFO */
    if (fifo_count > 0)
    {
		cmd = fifo[fifo_head];
		fifo_count--;
		fifo_head = (fifo_head + 1) % FIFO_SIZE;

		/* parse the command */
		switch (cmd & 0x70)
		{
		case 0x10 : /* read byte */
			if (schedule_dummy_read)
			{
				schedule_dummy_read = FALSE;
				if (read_callback)
					(*read_callback)(1);
			}
			if (read_callback)
				data_register = (*read_callback)(8);	/* read one byte from speech ROM... */
			RDB_flag = TRUE;
			break;

		case 0x30 : /* read and branch */
			log_cb(RETRO_LOG_DEBUG, LOGPRE "read and branch command received\n");
			RDB_flag = FALSE;
			if (read_and_branch_callback)
				(*read_and_branch_callback)();
			break;

		case 0x40 : /* load address */
			/* tms5220 data sheet says that if we load only one 4-bit nibble, it won't work.
			  This code does not care about this. */
			if (load_address_callback)
				(*load_address_callback)(cmd & 0x0f);
			schedule_dummy_read = TRUE;
			break;

		case 0x50 : /* speak */
			if (schedule_dummy_read)
			{
				schedule_dummy_read = FALSE;
				if (read_callback)
					(*read_callback)(1);
			}
			tms5220_speaking = 1;
			speak_external = 0;
			if (! last_frame)
			{
				first_frame = 1;
			}
			talk_status = 1;  /* start immediately */
			break;

		case 0x60 : /* speak external */
			tms5220_speaking = speak_external = 1;
#if USE_OBSOLETE_HACK
            speak_delay_frames = 10;
#endif

			RDB_flag = FALSE;

            /* according to the datasheet, this will cause an interrupt due to a BE condition */
            if (!buffer_empty)
            {
                buffer_empty = 1;
                set_interrupt_state(1);
            }

			talk_status = 0;	/* wait to have 8 bytes in buffer before starting */
			break;

		case 0x70 : /* reset */
			if (schedule_dummy_read)
			{
				schedule_dummy_read = FALSE;
				if (read_callback)
					(*read_callback)(1);
			}
			tms5220_reset();
			break;
        }
    }

    /* update the buffer low state */
    check_buffer_low();
}



/**********************************************************************************************

     extract_bits -- extract a specific number of bits from the FIFO

***********************************************************************************************/

static int extract_bits(int count)
{
    int val = 0;

	if (speak_external)
	{
		/* extract from FIFO */
    	while (count--)
    	{
        	val = (val << 1) | ((fifo[fifo_head] >> fifo_bits_taken) & 1);
        	fifo_bits_taken++;
        	if (fifo_bits_taken >= 8)
        	{
        	    fifo_count--;
        	    fifo_head = (fifo_head + 1) % FIFO_SIZE;
        	    fifo_bits_taken = 0;
        	}
    	}
    }
	else
	{
		/* extract from speech ROM */
		if (read_callback)
			val = (* read_callback)(count);
	}

    return val;
}



/**********************************************************************************************

     parse_frame -- parse a new frame's worth of data; returns 0 if not enough bits in buffer

***********************************************************************************************/

static int parse_frame(int the_first_frame)
{
	int bits = 0;	/* number of bits in FIFO (speak external only) */
	int indx, i, rep_flag;

	if (! the_first_frame)
	{
    /* remember previous frame */
    old_energy = new_energy;
    old_pitch = new_pitch;
    for (i = 0; i < 10; i++)
        old_k[i] = new_k[i];
	}

    /* clear out the new frame */
    new_energy = 0;
    new_pitch = 0;
    for (i = 0; i < 10; i++)
        new_k[i] = 0;

    /* if the previous frame was a stop frame, don't do anything */
	if ((! the_first_frame) && (old_energy == (energytable[15] >> 6)))
		/*return 1;*/
	{
		buffer_empty = 1;
		return 1;
	}

	if (speak_external)
    	/* count the total number of bits available */
		bits = fifo_count * 8 - fifo_bits_taken;

    /* attempt to extract the energy index */
	if (speak_external)
	{
    bits -= 4;
    if (bits < 0)
        goto ranout;
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
			fifo_head = fifo_tail = fifo_count = fifo_bits_taken = 0;
			speak_external = tms5220_speaking = 0;
			last_frame = 1;
		}
		goto done;
	}

    /* attempt to extract the repeat flag */
	if (speak_external)
	{
    bits -= 1;
    if (bits < 0)
        goto ranout;
	}
    rep_flag = extract_bits(1);

    /* attempt to extract the pitch */
	if (speak_external)
	{
    bits -= 6;
    if (bits < 0)
        goto ranout;
	}
    indx = extract_bits(6);
    new_pitch = pitchtable[indx] / 256;

    /* if this is a repeat frame, just copy the k's */
    if (rep_flag)
    {
        for (i = 0; i < 10; i++)
            new_k[i] = old_k[i];

        log_cb(RETRO_LOG_DEBUG, LOGPRE "  (11-bit energy=%d pitch=%d rep=%d frame)\n", new_energy, new_pitch, rep_flag);
        goto done;
    }

    /* if the pitch index was zero, we need 4 k's */
    if (indx == 0)
    {
        /* attempt to extract 4 K's */
		if (speak_external)
		{
        bits -= 18;
        if (bits < 0)
            goto ranout;
		}
        new_k[0] = k1table[extract_bits(5)];
        new_k[1] = k2table[extract_bits(5)];
        new_k[2] = k3table[extract_bits(4)];
		if (variant == variant_tms0285)
			new_k[3] = k3table[extract_bits(4)];	/* ??? */
		else
			new_k[3] = k4table[extract_bits(4)];

        log_cb(RETRO_LOG_DEBUG, LOGPRE "  (29-bit energy=%d pitch=%d rep=%d 4K frame)\n", new_energy, new_pitch, rep_flag);
        goto done;
    }

    /* else we need 10 K's */
	if (speak_external)
	{
    bits -= 39;
    if (bits < 0)
        goto ranout;
	}

    new_k[0] = k1table[extract_bits(5)];
    new_k[1] = k2table[extract_bits(5)];
    new_k[2] = k3table[extract_bits(4)];
	if (variant == variant_tms0285)
		new_k[3] = k3table[extract_bits(4)];	/* ??? */
	else
		new_k[3] = k4table[extract_bits(4)];
    new_k[4] = k5table[extract_bits(4)];
    new_k[5] = k6table[extract_bits(4)];
    new_k[6] = k7table[extract_bits(4)];
    new_k[7] = k8table[extract_bits(3)];
    new_k[8] = k9table[extract_bits(3)];
    new_k[9] = k10table[extract_bits(3)];

    log_cb(RETRO_LOG_DEBUG, LOGPRE "  (50-bit energy=%d pitch=%d rep=%d 10K frame)\n", new_energy, new_pitch, rep_flag);

done:
		if (speak_external)
			log_cb(RETRO_LOG_DEBUG, LOGPRE "Parsed a frame successfully in FIFO - %d bits remaining\n", bits);
		else
			log_cb(RETRO_LOG_DEBUG, LOGPRE "Parsed a frame successfully in ROM\n");

	if (the_first_frame)
	{
		/* if this is the first frame, no previous frame to take as a starting point */
		old_energy = new_energy;
		old_pitch = new_pitch;
		for (i = 0; i < 10; i++)
			old_k[i] = new_k[i];
    }

    /* update the buffer_low status */
    check_buffer_low();
    return 1;

ranout:

    log_cb(RETRO_LOG_DEBUG, LOGPRE "Ran out of bits on a parse!\n");

    /* this is an error condition; mark the buffer empty and turn off speaking */
    buffer_empty = 1;
	talk_status = speak_external = tms5220_speaking = the_first_frame = last_frame = 0;
    fifo_count = fifo_head = fifo_tail = 0;

	RDB_flag = FALSE;

    /* generate an interrupt if necessary */
    set_interrupt_state(1);
    return 0;
}



/**********************************************************************************************

     check_buffer_low -- check to see if the buffer low flag should be on or off

***********************************************************************************************/

static void check_buffer_low(void)
{
    /* did we just become low? */
    if (fifo_count <= 8)
    {
        /* generate an interrupt if necessary */
        if (!buffer_low)
            set_interrupt_state(1);
        buffer_low = 1;

        log_cb(RETRO_LOG_DEBUG, LOGPRE "Buffer low set\n");
    }

    /* did we just become full? */
    else
    {
        buffer_low = 0;

        log_cb(RETRO_LOG_DEBUG, LOGPRE "Buffer low cleared\n");
    }
}



/**********************************************************************************************

     set_interrupt_state -- generate an interrupt

***********************************************************************************************/

static void set_interrupt_state(int state)
{
    if (irq_func && state != irq_pin)
    	irq_func(state);
    irq_pin = state;
}
