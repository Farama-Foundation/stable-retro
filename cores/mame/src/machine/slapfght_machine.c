/***************************************************************************

  machine.c

  Functions to emulate general aspects of the machine (RAM, ROM, interrupts,
  I/O ports)

***************************************************************************/

#include "driver.h"


unsigned char *slapfight_dpram;
size_t slapfight_dpram_size;

int slapfight_status;
int getstar_sequence_index;
int getstar_sh_intenabled;

static int slapfight_status_state;
extern unsigned char *getstar_e803;

static unsigned char mcu_val;
extern unsigned char *slapfight_scrollx_lo,*slapfight_scrollx_hi,*slapfight_scrolly;

static unsigned char from_main,from_mcu;
static int mcu_sent = 0,main_sent = 0;
static unsigned char portA_in,portA_out,ddrA;
static unsigned char portB_in,portB_out,ddrB;
static unsigned char portC_in,portC_out,ddrC;




/* Perform basic machine initialisation */
MACHINE_INIT( slapfight )
{
	/* MAIN CPU */

	slapfight_status_state=0;
	slapfight_status = 0xc7;

	getstar_sequence_index = 0;
	getstar_sh_intenabled = 0;	/* disable sound cpu interrupts */

	/* SOUND CPU */
	cpu_set_reset_line(1,ASSERT_LINE);

	/* MCU */
	mcu_val = 0;
}

/* Interrupt handlers cpu & sound */

WRITE_HANDLER( slapfight_dpram_w )
{
    slapfight_dpram[offset]=data;
}

READ_HANDLER( slapfight_dpram_r )
{
    return slapfight_dpram[offset];
}



/* Slapfight CPU input/output ports

  These ports seem to control memory access

*/

/* Reset and hold sound CPU */
WRITE_HANDLER( slapfight_port_00_w )
{
	cpu_set_reset_line(1,ASSERT_LINE);
	getstar_sh_intenabled = 0;
}

/* Release reset on sound CPU */
WRITE_HANDLER( slapfight_port_01_w )
{
	cpu_set_reset_line(1,CLEAR_LINE);
}

/* Disable and clear hardware interrupt */
WRITE_HANDLER( slapfight_port_06_w )
{
	interrupt_enable_w(0,0);
}

/* Enable hardware interrupt */
WRITE_HANDLER( slapfight_port_07_w )
{
	interrupt_enable_w(0,1);
}

WRITE_HANDLER( slapfight_port_08_w )
{
	unsigned char *RAM = memory_region(REGION_CPU1);

	cpu_setbank(1,&RAM[0x10000]);
}

WRITE_HANDLER( slapfight_port_09_w )
{
	unsigned char *RAM = memory_region(REGION_CPU1);

	cpu_setbank(1,&RAM[0x14000]);
}


/* Status register */
READ_HANDLER( slapfight_port_00_r )
{
	int states[3]={ 0xc7, 0x55, 0x00 };

	slapfight_status = states[slapfight_status_state];

	slapfight_status_state++;
	if (slapfight_status_state > 2) slapfight_status_state = 0;

	return slapfight_status;
}

READ_HANDLER( slapfight_68705_portA_r )
{
	return (portA_out & ddrA) | (portA_in & ~ddrA);
}

WRITE_HANDLER( slapfight_68705_portA_w )
{
	portA_out = data;
}

WRITE_HANDLER( slapfight_68705_ddrA_w )
{
	ddrA = data;
}

READ_HANDLER( slapfight_68705_portB_r )
{
	return (portB_out & ddrB) | (portB_in & ~ddrB);
}

WRITE_HANDLER( slapfight_68705_portB_w )
{
	if ((ddrB & 0x02) && (~data & 0x02) && (portB_out & 0x02))
	{
		portA_in = from_main;

		if (main_sent)
			cpu_set_irq_line(2,0,CLEAR_LINE);

		main_sent = 0;
	}
	if ((ddrB & 0x04) && (data & 0x04) && (~portB_out & 0x04))
	{
		from_mcu = portA_out;
		mcu_sent = 1;
	}
	if ((ddrB & 0x08) && (~data & 0x08) && (portB_out & 0x08))
	{
		*slapfight_scrollx_lo = portA_out;
	}
	if ((ddrB & 0x10) && (~data & 0x10) && (portB_out & 0x10))
	{
		*slapfight_scrollx_hi = portA_out;
	}

	portB_out = data;
}

WRITE_HANDLER( slapfight_68705_ddrB_w )
{
	ddrB = data;
}

READ_HANDLER( slapfight_68705_portC_r )
{
	portC_in = 0;

	if (main_sent)
		portC_in |= 0x01;
	if (!mcu_sent)
		portC_in |= 0x02;

	return (portC_out & ddrC) | (portC_in & ~ddrC);
}

WRITE_HANDLER( slapfight_68705_portC_w )
{
	portC_out = data;
}

WRITE_HANDLER( slapfight_68705_ddrC_w )
{
	ddrC = data;
}

WRITE_HANDLER( slapfight_mcu_w )
{
	from_main = data;
	main_sent = 1;
    cpu_set_irq_line(2,0,ASSERT_LINE);
}

READ_HANDLER( slapfight_mcu_r )
{
	mcu_sent = 0;
	return from_mcu;
}

READ_HANDLER( slapfight_mcu_status_r )
{
	int res = 0;

	if (!main_sent)
		res |= 0x02;
	if (!mcu_sent)
		res |= 0x04;

	return res;
}


/*
 Reads at e803 expect a sequence of values such that:
 - first value is different from successive
 - third value is (first+5)^0x56
 I don't know what writes to this address do (connected to port 0 reads?).
*/
READ_HANDLER( getstar_e803_r )
{
unsigned char seq[] = { 0, 1, ((0+5)^0x56) };
unsigned char val;

	val = seq[getstar_sequence_index];
	getstar_sequence_index = (getstar_sequence_index+1)%3;
	return val;
}

/* Enable hardware interrupt of sound cpu */
WRITE_HANDLER( getstar_sh_intenable_w )
{
	getstar_sh_intenabled = 1;
	log_cb(RETRO_LOG_DEBUG, LOGPRE "cpu #1 PC=%d: %d written to a0e0\n",activecpu_get_pc(),data);
}



/* Generate interrups only if they have been enabled */
INTERRUPT_GEN( getstar_interrupt )
{
	if (getstar_sh_intenabled)
		cpu_set_irq_line(1, IRQ_LINE_NMI, PULSE_LINE);
}

WRITE_HANDLER( getstar_port_04_w )
{
/*	cpu_halt(0,0);*/
}


READ_HANDLER( tigerh_68705_portA_r )
{
	return (portA_out & ddrA) | (portA_in & ~ddrA);
}

WRITE_HANDLER( tigerh_68705_portA_w )
{
	portA_out = data;/*?*/
	from_mcu = portA_out;
	mcu_sent = 1;
}

WRITE_HANDLER( tigerh_68705_ddrA_w )
{
	ddrA = data;
}

READ_HANDLER( tigerh_68705_portB_r )
{
	return (portB_out & ddrB) | (portB_in & ~ddrB);
}

WRITE_HANDLER( tigerh_68705_portB_w )
{

	if ((ddrB & 0x02) && (~data & 0x02) && (portB_out & 0x02))
	{
		portA_in = from_main;
		if (main_sent) cpu_set_irq_line(2,0,CLEAR_LINE);
		main_sent = 0;
	}
	if ((ddrB & 0x04) && (data & 0x04) && (~portB_out & 0x04))
	{
		from_mcu = portA_out;
		mcu_sent = 1;
	}

	portB_out = data;
}

WRITE_HANDLER( tigerh_68705_ddrB_w )
{
	ddrB = data;
}


READ_HANDLER( tigerh_68705_portC_r )
{
	portC_in = 0;
	if (!main_sent) portC_in |= 0x01;
	if (mcu_sent) portC_in |= 0x02;
	return (portC_out & ddrC) | (portC_in & ~ddrC);
}

WRITE_HANDLER( tigerh_68705_portC_w )
{
	portC_out = data;
}

WRITE_HANDLER( tigerh_68705_ddrC_w )
{
	ddrC = data;
}

WRITE_HANDLER( tigerh_mcu_w )
{
	from_main = data;
	main_sent = 1;
	mcu_sent=0;
	cpu_set_irq_line(2,0,ASSERT_LINE);
}

READ_HANDLER( tigerh_mcu_r )
{
	mcu_sent = 0;
	return from_mcu;
}

READ_HANDLER( tigerh_mcu_status_r )
{
	int res = 0;
	if (!main_sent) res |= 0x02;
	if (!mcu_sent) res |= 0x04;
	return res;
}
