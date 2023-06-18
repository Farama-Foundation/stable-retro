/***************************************************************************

  machine.c

  Functions to emulate general aspects of the machine (RAM, ROM, interrupts,
  I/O ports)

***************************************************************************/

#include "driver.h"
#include "cpu/z80/z80.h"
#include "includes/berzerk.h"


static int irq_enabled;
static int nmi_enabled;
static int int_count;


MACHINE_INIT( berzerk )
{
	memory_set_unmap_value(0xff);

	irq_enabled = 0;
	nmi_enabled = 0;
	int_count = 0;
}


WRITE_HANDLER( berzerk_irq_enable_w )
{
	irq_enabled = data;
}

WRITE_HANDLER( berzerk_nmi_enable_w )
{
	nmi_enabled = 1;
}

WRITE_HANDLER( berzerk_nmi_disable_w )
{
	nmi_enabled = 0;
}

READ_HANDLER( berzerk_nmi_enable_r )
{
	nmi_enabled = 1;
	return 0;
}

READ_HANDLER( berzerk_nmi_disable_r )
{
	nmi_enabled = 0;
	return 0;
}

READ_HANDLER( berzerk_led_on_r )
{
	set_led_status(0,1);

	return 0;
}

READ_HANDLER( berzerk_led_off_r )
{
	set_led_status(0,0);

	return 0;
}

INTERRUPT_GEN( berzerk_interrupt )
{
	int_count++;

	if (int_count & 0x03)
	{
		if (nmi_enabled) cpu_set_irq_line(0, IRQ_LINE_NMI, PULSE_LINE);
	}
	else
	{
		if (irq_enabled) cpu_set_irq_line_and_vector(0, 0, HOLD_LINE, 0xfc);
	}
}
