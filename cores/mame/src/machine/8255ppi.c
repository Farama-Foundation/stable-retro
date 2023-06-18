/* INTEL 8255 PPI I/O chip */


/* NOTE: When port is input, then data present on the ports
   outputs is 0xff */

/* KT 10/01/2000 - Added bit set/reset feature for control port */
/*               - Added more accurate port i/o data handling */
/*               - Added output reset when control mode is programmed */



#include "driver.h"
#include "machine/8255ppi.h"


static int num;

typedef struct
{
	mem_read_handler portAread;
	mem_read_handler portBread;
	mem_read_handler portCread;
	mem_write_handler portAwrite;
	mem_write_handler portBwrite;
	mem_write_handler portCwrite;
	int groupA_mode;
	int groupB_mode;
	int in_mask[3];	/* input mask */
	int out_mask[3];/* output mask */
	int latch[3];	/* data written to ports */
} ppi8255;

static ppi8255 chips[MAX_8255];

static void set_mode(int which, int data, int call_handlers);


void ppi8255_init( ppi8255_interface *intfce )
{
	int i;

	num = intfce->num;

	for (i = 0; i < num; i++)
	{
		chips[i].portAread = intfce->portAread[i];
		chips[i].portBread = intfce->portBread[i];
		chips[i].portCread = intfce->portCread[i];
		chips[i].portAwrite = intfce->portAwrite[i];
		chips[i].portBwrite = intfce->portBwrite[i];
		chips[i].portCwrite = intfce->portCwrite[i];

		set_mode(i, 0x1b, 0);	/* Mode 0, all ports set to input */
	}
}


int ppi8255_r(int which, int offset)
{
	ppi8255 *chip = &chips[which];
	int result = 0;

	/* some bounds checking */
	if (which > num)
	{
		log_cb(RETRO_LOG_DEBUG, LOGPRE "Attempting to access an unmapped 8255 chip.  PC: %04X\n", activecpu_get_pc());
		return 0xff;
	}

	if (offset > 3)
	{
		log_cb(RETRO_LOG_DEBUG, LOGPRE "Attempting to access an invalid 8255 register.  PC: %04X\n", activecpu_get_pc());
		return 0xff;
	}

	switch(offset)
	{
		case 0: /* Port A read */
			if (chip->in_mask[0])
			{
				if (chip->portAread)
					result = (*chip->portAread)(0) & chip->in_mask[0];
				else
					log_cb(RETRO_LOG_DEBUG, LOGPRE "8255 chip %d: Port A is being read (mask %02x) but has no handler.  PC: %08X\n", which, chip->in_mask[0], activecpu_get_pc());
			}
			result |= chip->latch[0] & chip->out_mask[0] & ~chip->in_mask[0];
			break;

		case 1: /* Port B read */
			if (chip->in_mask[1])
			{
				if (chip->portBread)
					result = (*chip->portBread)(0) & chip->in_mask[1];
				else
					log_cb(RETRO_LOG_DEBUG, LOGPRE "8255 chip %d: Port B is being read (mask %02x) but has no handler.  PC: %08X\n", which, chip->in_mask[1], activecpu_get_pc());
			}
			result |= chip->latch[1] & chip->out_mask[1] & ~chip->in_mask[1];
			break;

		case 2: /* Port C read */
			if (chip->in_mask[2])
			{
				if (chip->portCread)
					result = (*chip->portCread)(0) & chip->in_mask[2];
				else
					log_cb(RETRO_LOG_DEBUG, LOGPRE "8255 chip %d: Port C is being read (mask %02x) but has no handler.  PC: %08X\n", which, chip->in_mask[2], activecpu_get_pc());
			}
			result |= chip->latch[2] & chip->out_mask[2] & ~chip->in_mask[2];
			break;

		case 3: /* Control word */
			result = 0xff;
			break;
	}

	return result;
}



#define PPI8255_PORT_A_WRITE()							\
{														\
	int write_data = (chip->latch[0] & chip->out_mask[0]) |	(0xff & ~chip->out_mask[0]);	\
														\
	if (chip->portAwrite)								\
		(*chip->portAwrite)(0, write_data);				\
	else												\
		log_cb(RETRO_LOG_DEBUG, LOGPRE "8255 chip %d: Port A is being written to (mask %02x) but has no handler.  PC: %08X - %02X\n", which, chip->out_mask[0], activecpu_get_pc(), write_data);	\
}

#define PPI8255_PORT_B_WRITE()							\
{														\
	int write_data = (chip->latch[1] & chip->out_mask[1]) |	(0xff & ~chip->out_mask[1]);	\
														\
	if (chip->portBwrite)								\
		(*chip->portBwrite)(0, write_data);				\
	else												\
		log_cb(RETRO_LOG_DEBUG, LOGPRE "8255 chip %d: Port B is being written to (mask %02x) but has no handler.  PC: %08X - %02X\n", which, chip->out_mask[1], activecpu_get_pc(), write_data);	\
}

#define PPI8255_PORT_C_WRITE()							\
{														\
	int write_data = (chip->latch[2] & chip->out_mask[2]) |	(0xff & ~chip->out_mask[2]);	\
														\
	if (chip->portCwrite)								\
		(*chip->portCwrite)(0, write_data);				\
	else												\
		log_cb(RETRO_LOG_DEBUG, LOGPRE "8255 chip %d: Port C is being written to (mask %02x) but has no handler.  PC: %08X - %02X\n", which, chip->out_mask[2], activecpu_get_pc(), write_data);	\
}


void ppi8255_w(int which, int offset, int data)
{
	ppi8255	*chip = &chips[which];

	/* Some bounds checking */
	if (which > num)
	{
		log_cb(RETRO_LOG_DEBUG, LOGPRE "Attempting to access an unmapped 8255 chip.  PC: %04X\n", activecpu_get_pc());
		return;
	}

	if (offset > 3)
	{
		log_cb(RETRO_LOG_DEBUG, LOGPRE "Attempting to access an invalid 8255 register.  PC: %04X\n", activecpu_get_pc());
		return;
	}

	switch( offset )
	{
		case 0: /* Port A write */
			chip->latch[0] = data;
			PPI8255_PORT_A_WRITE();
			break;

		case 1: /* Port B write */
			chip->latch[1] = data;
			PPI8255_PORT_B_WRITE();
			break;

		case 2: /* Port C write */
			chip->latch[2] = data;
			PPI8255_PORT_C_WRITE();
			break;

		case 3: /* Control word */
			if (data & 0x80)
			{
				set_mode(which, data & 0x7f, 1);
			}
			else
			{
				/* bit set/reset */
				int bit;

				bit = (data >> 1) & 0x07;

				if (data & 1)
					chip->latch[2] |= (1<<bit);		/* set bit */
				else
					chip->latch[2] &= ~(1<<bit);	/* reset bit */

				PPI8255_PORT_C_WRITE();
			}
	}
}


void ppi8255_set_portAread(int which, mem_read_handler portAread)
{
	chips[which].portAread = portAread;
}

void ppi8255_set_portBread(int which, mem_read_handler portBread)
{
	chips[which].portBread = portBread;
}

void ppi8255_set_portCread(int which, mem_read_handler portCread)
{
	chips[which].portCread = portCread;
}


void ppi8255_set_portAwrite(int which, mem_write_handler portAwrite)
{
	chips[which].portAwrite = portAwrite;
}

void ppi8255_set_portBwrite(int which, mem_write_handler portBwrite)
{
	chips[which].portBwrite = portBwrite;
}

void ppi8255_set_portCwrite(int which, mem_write_handler portCwrite)
{
	chips[which].portCwrite = portCwrite;
}


static void set_mode(int which, int data, int call_handlers)
{
	ppi8255 *chip = &chips[which];

	chip->groupA_mode = (data >> 5) & 3;
	chip->groupB_mode = (data >> 2) & 1;

	if ((chip->groupA_mode == 1) || (chip->groupB_mode == 1))
	{
		log_cb(RETRO_LOG_DEBUG, LOGPRE "8255 chip %d: Setting an unsupported mode %02X.  PC: %04X\n", which, data & 0x62, activecpu_get_pc());
		return;
	}

	/* Group A mode 0 */
	if (chip->groupA_mode == 0)
	{
		/* Port A direction */
		if (data & 0x10)
			chip->in_mask[0] = 0xff, chip->out_mask[0] = 0x00;	/* input */
		else
			chip->in_mask[0] = 0x00, chip->out_mask[0] = 0xff; 	/* output */

		/* Port C upper direction */
		if (data & 0x08)
			chip->in_mask[2] |= 0xf0, chip->out_mask[2] &= ~0xf0;	/* input */
		else
			chip->in_mask[2] &= ~0xf0, chip->out_mask[2] |= 0xf0;	/* output */
	}

	/* Group A mode 2/3 */
	else
	{
		chip->in_mask[0] = 0xff;
		chip->out_mask[0] = 0xff;

		chip->in_mask[2] = 0xf7;
		chip->out_mask[2] = 0xff;
	}

	/* Group B mode 0 */
	{
		/* Port B direction */
		if (data & 0x02)
			chip->in_mask[1] = 0xff, chip->out_mask[1] = 0x00;	/* input */
		else
			chip->in_mask[1] = 0x00, chip->out_mask[1] = 0xff; 	/* output */

		/* Port C lower direction */
		if (data & 0x01)
			chip->in_mask[2] |= 0x0f, chip->out_mask[2] &= ~0x0f;	/* input */
		else
			chip->in_mask[2] &= ~0x0f, chip->out_mask[2] |= 0x0f;	/* output */
	}

	/* KT: 25-Dec-99 - 8255 resets latches when mode set */
	chip->latch[0] = chip->latch[1] = chip->latch[2] = 0;

	if (call_handlers)
	{
		if (chip->portAwrite) PPI8255_PORT_A_WRITE();
		if (chip->portBwrite) PPI8255_PORT_B_WRITE();
		if (chip->portCwrite) PPI8255_PORT_C_WRITE();
	}
}


/* Helpers */
READ_HANDLER( ppi8255_0_r ) { return ppi8255_r( 0, offset ); }
READ_HANDLER( ppi8255_1_r ) { return ppi8255_r( 1, offset ); }
READ_HANDLER( ppi8255_2_r ) { return ppi8255_r( 2, offset ); }
READ_HANDLER( ppi8255_3_r ) { return ppi8255_r( 3, offset ); }
READ_HANDLER( ppi8255_4_r ) { return ppi8255_r( 4, offset ); }
READ_HANDLER( ppi8255_5_r ) { return ppi8255_r( 5, offset ); }
READ_HANDLER( ppi8255_6_r ) { return ppi8255_r( 6, offset ); }
READ_HANDLER( ppi8255_7_r ) { return ppi8255_r( 7, offset ); }
WRITE_HANDLER( ppi8255_0_w ) { ppi8255_w( 0, offset, data ); }
WRITE_HANDLER( ppi8255_1_w ) { ppi8255_w( 1, offset, data ); }
WRITE_HANDLER( ppi8255_2_w ) { ppi8255_w( 2, offset, data ); }
WRITE_HANDLER( ppi8255_3_w ) { ppi8255_w( 3, offset, data ); }
WRITE_HANDLER( ppi8255_4_w ) { ppi8255_w( 4, offset, data ); }
WRITE_HANDLER( ppi8255_5_w ) { ppi8255_w( 5, offset, data ); }
WRITE_HANDLER( ppi8255_6_w ) { ppi8255_w( 6, offset, data ); }
WRITE_HANDLER( ppi8255_7_w ) { ppi8255_w( 7, offset, data ); }
