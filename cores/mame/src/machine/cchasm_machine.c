/***************************************************************************

	Cinematronics Cosmic Chasm hardware

***************************************************************************/

#include "driver.h"
#include "vidhrdw/vector.h"
#include "machine/z80fmly.h"
#include "cchasm.h"

#define M6840_CYCLE 1250  /* 800 kHz */

static int m6840_cr_select=2;
static int m6840_timerLSB[3];
static int m6840_timerMSB[3];
static int m6840_status;
static int m6840_cr[3];


static void cchasm_6840_irq(int state)
{
	cpu_set_irq_line(0, 4, state);
}

static void timer_2_timeout(int dummy)
{
	if (m6840_cr[1] & 0x40)
	{
		/* set interrupt flags */
		m6840_status |= 0x82;
		cchasm_6840_irq(ASSERT_LINE);
	}
}

/* from machine/mcr68.c modified for Cosmic Chasm (still very incomplete) */

READ16_HANDLER( cchasm_6840_r )
{
	/* From datasheet:
	   0 - nothing
	   2		1 - read status register
	   4		2 - read timer 1 counter
	   6		3 - read lsb buffer
	   8		4 - timer 2 counter
	   a		5 - read lsb buffer
	   c		6 - timer 3 counter
	   e		7 - lsb buffer


	*/

	switch (offset)
	{
	case 0x0:
		log_cb(RETRO_LOG_DEBUG, LOGPRE "Read from unimplemented port...\n");
		break;

	case 0x1:
		log_cb(RETRO_LOG_DEBUG, LOGPRE "Read status register\n");

		return m6840_status;
		break;

	case 0x2:
		log_cb(RETRO_LOG_DEBUG, LOGPRE "Read MSB of timer 1 (%d)\n",m6840_timerMSB[0]);
		return m6840_timerMSB[0];

	case 0x3:
		log_cb(RETRO_LOG_DEBUG, LOGPRE "Read LSB of timer 1\n");
		return m6840_timerLSB[0];

	case 0x4:
		log_cb(RETRO_LOG_DEBUG, LOGPRE "Read MSB of timer 2 %i\n",m6840_timerMSB[1]);
		return m6840_timerMSB[1];

	case 0x5:
		log_cb(RETRO_LOG_DEBUG, LOGPRE "Read LSB of timer 2 %i\n",m6840_timerLSB[1]);
		return m6840_timerLSB[1];

	case 0x6:
		log_cb(RETRO_LOG_DEBUG, LOGPRE "Read MSB of timer 3 (%d)\n",m6840_timerMSB[2]);
		return m6840_timerMSB[2];

	case 0x7:
		log_cb(RETRO_LOG_DEBUG, LOGPRE "Read LSB of timer 3\n");
		return m6840_timerLSB[2];
	}
	return 0;
}


WRITE16_HANDLER( cchasm_6840_w )
{

	/* From datasheet:


	   0		0 - write control reg 1/3 (reg 1 if lsb of Reg 2 is 1, else 3)
	   2		1 - write control reg 2
	   4		2 - write msb buffer reg
	   6		3 - write timer 1 latch
	   8		4 - msb buffer register
	   a		5 - write timer 2 latch
	   c		6 - msb buffer reg
	   e		7 - write timer 3 latch


	   So:

	   Write ff0100 to 6840 02	 = }
	   Write ff0000 to 6840 00	 = } Write 0 to control reg 1

	   Write ff0000 to 6840 02

	   Write ff0000 to 6840 0c
	   Write ff3500 to 6840 0e	 = } Write 0035 to timer 3 latch?

	   Write ff0a00 to 6840 00	 = } Write 0a to control reg 3

	*/

	if (ACCESSING_LSB)
	{
		data &= 0xff;

		switch (offset)
		{
		case 0x0: /* CR1/3 */
			m6840_cr[m6840_cr_select] = data;

			if (m6840_cr_select==0)
			{
				if ((data&0x1))
				{
					int i;
					log_cb(RETRO_LOG_DEBUG, LOGPRE "MC6840: Internal reset\n");
					for (i=0; i<3; i++) {
						m6840_timerLSB[i]=255;
						m6840_timerMSB[i]=255;

					}
				}
				else
					log_cb(RETRO_LOG_DEBUG, LOGPRE "MC6840: Timers go!\n");


			}

			else if (m6840_cr_select==2) {
				if (data&0x1)
					log_cb(RETRO_LOG_DEBUG, LOGPRE "MC6840: Divide by 8 prescaler selected\n");
			}

			/* Following bits apply to both registers */
			if (data&0x2)
				log_cb(RETRO_LOG_DEBUG, LOGPRE "MC6840: Internal clock selected on CR %d\n",m6840_cr_select);
			else
				log_cb(RETRO_LOG_DEBUG, LOGPRE "MC6840: External clock selected on CR %d\n",m6840_cr_select);

			if (data&0x4)
				log_cb(RETRO_LOG_DEBUG, LOGPRE "MC6840: Dual 8 bit count mode selected on CR %d\n",m6840_cr_select);
			else
				log_cb(RETRO_LOG_DEBUG, LOGPRE "MC6840: 16 bit count mode selected on CR %d\n",m6840_cr_select);

			log_cb(RETRO_LOG_DEBUG, LOGPRE " Write %02x to control register 1/3\n",data);
			break;
		case 0x1:
			m6840_cr[1] = data;
			if (data&0x1)
			{
				m6840_cr_select=0;
				log_cb(RETRO_LOG_DEBUG, LOGPRE "MC6840: Control register 1 selected\n");
			}
			else {
				m6840_cr_select=2;
				log_cb(RETRO_LOG_DEBUG, LOGPRE "MC6840: Control register 3 selected\n");
			}

			if (data&0x80)
				log_cb(RETRO_LOG_DEBUG, LOGPRE "MC6840: Cr2 Timer output enabled\n");

			if (data&0x40)
				log_cb(RETRO_LOG_DEBUG, LOGPRE "MC6840: Cr2 interrupt output enabled\n");

			log_cb(RETRO_LOG_DEBUG, LOGPRE " Write %02x to control register 2\n",data);
			break;
		case 0x2:
			m6840_timerMSB[0]=data;
			m6840_status &= ~0x01;
			log_cb(RETRO_LOG_DEBUG, LOGPRE " Write %02x to MSB of Timer 1\n",data);
			break;
		case 0x3:
			m6840_status &= ~0x01;
			m6840_timerLSB[0]=data;
			log_cb(RETRO_LOG_DEBUG, LOGPRE " Write %02x to LSB of Timer 1\n",data);
			break;
		case 0x4:
			m6840_status &= ~0x02;
			cchasm_6840_irq(CLEAR_LINE);
			m6840_timerMSB[1]=data;
			if ((m6840_cr[1] & 0x38) == 0)
				timer_set (TIME_IN_NSEC(M6840_CYCLE) * ((m6840_timerMSB[1]<<8) | m6840_timerLSB[1]), 0, timer_2_timeout);
			log_cb(RETRO_LOG_DEBUG, LOGPRE " Write %02x to MSB of Timer 2\n",data);
			break;
		case 0x5:
			m6840_status &= ~0x02;
			cchasm_6840_irq(CLEAR_LINE);
			m6840_timerLSB[1]=data;
			log_cb(RETRO_LOG_DEBUG, LOGPRE " Write %02x to LSB of Timer 2\n",data);
			break;
		case 0x6:
			m6840_status &= ~0x04;
			m6840_timerMSB[2]=data;
			log_cb(RETRO_LOG_DEBUG, LOGPRE " Write %02x to MSB of Timer 3\n",data);
			break;
		case 0x7:
			m6840_status &= ~0x04;
			m6840_timerLSB[2]=data;
			log_cb(RETRO_LOG_DEBUG, LOGPRE " Write %02x to LSB of Timer 3\n",data);
			break;
		}
	}
}

WRITE16_HANDLER( cchasm_led_w )
{
	/*logerror("LED write %x to %x\n", data, offset);*/
}
