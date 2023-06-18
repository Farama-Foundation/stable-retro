/***************************************************************************

  machine.c

  Functions to emulate general aspects of the machine (RAM, ROM, interrupts,
  I/O ports)

***************************************************************************/

#include "driver.h"
#include "cpu/z80/z80.h"
#include "vidhrdw/generic.h"



unsigned char *bublbobl_sharedram1,*bublbobl_sharedram2;
extern int bublbobl_video_enable;


READ_HANDLER( bublbobl_sharedram1_r )
{
	return bublbobl_sharedram1[offset];
}
READ_HANDLER( bublbobl_sharedram2_r )
{
	return bublbobl_sharedram2[offset];
}
WRITE_HANDLER( bublbobl_sharedram1_w )
{
	bublbobl_sharedram1[offset] = data;
}
WRITE_HANDLER( bublbobl_sharedram2_w )
{
	bublbobl_sharedram2[offset] = data;
}



WRITE_HANDLER( bublbobl_bankswitch_w )
{
	unsigned char *ROM = memory_region(REGION_CPU1);

	/* bits 0-2 select ROM bank */
	cpu_setbank(1,&ROM[0x10000 + 0x4000 * ((data ^ 4) & 7)]);

	/* bit 3 n.c. */

	/* bit 4 resets second Z80 */
	cpu_set_reset_line(1, (data & 0x10) ? CLEAR_LINE : ASSERT_LINE);

	/* bit 5 resets mcu */
	if (memory_region_length(REGION_CPU3) == 0x10000) { /* do we have an mcu?*/
		cpu_set_reset_line(3, (data & 0x20) ? CLEAR_LINE : ASSERT_LINE);
	}

	/* bit 6 enables display */
	bublbobl_video_enable = data & 0x40;

	/* bit 7 flips screen */
	flip_screen_set(data & 0x80);
}

WRITE_HANDLER( tokio_bankswitch_w )
{
	unsigned char *ROM = memory_region(REGION_CPU1);

	/* bits 0-2 select ROM bank */
	cpu_setbank(1,&ROM[0x10000 + 0x4000 * (data & 7)]);

	/* bits 3-7 unknown */
}

WRITE_HANDLER( tokio_videoctrl_w )
{
	/* bit 7 flips screen */
	flip_screen_set(data & 0x80);

	/* other bits unknown */
}

WRITE_HANDLER( bublbobl_nmitrigger_w )
{
	cpu_set_irq_line(1,IRQ_LINE_NMI,PULSE_LINE);
}

READ_HANDLER( tokio_fake_r )
{
  return 0xbf; /* ad-hoc value set to pass initial testing */
}



static int sound_nmi_enable,pending_nmi;

static void nmi_callback(int param)
{
	if (sound_nmi_enable) cpu_set_irq_line(2,IRQ_LINE_NMI,PULSE_LINE);
	else pending_nmi = 1;
}

WRITE_HANDLER( bublbobl_sound_command_w )
{
	soundlatch_w(offset,data);
	timer_set(TIME_NOW,data,nmi_callback);
}

WRITE_HANDLER( bublbobl_sh_nmi_disable_w )
{
	sound_nmi_enable = 0;
}

WRITE_HANDLER( bublbobl_sh_nmi_enable_w )
{
	sound_nmi_enable = 1;
	if (pending_nmi)
	{
		cpu_set_irq_line(2,IRQ_LINE_NMI,PULSE_LINE);
		pending_nmi = 0;
	}
}



/***************************************************************************

 Bubble Bobble 68705 protection interface

 The following is ENTIRELY GUESSWORK!!!

***************************************************************************/
INTERRUPT_GEN( bublbobl_m68705_interrupt )
{
	/* I don't know how to handle the interrupt line so I just toggle it every time. */
	if (cpu_getiloops() & 1)
		cpu_set_irq_line(3,0,CLEAR_LINE);
	else
		cpu_set_irq_line(3,0,ASSERT_LINE);
}



static unsigned char portA_in,portA_out,ddrA;

READ_HANDLER( bublbobl_68705_portA_r )
{
/*logerror("%04x: 68705 port A read %02x\n",activecpu_get_pc(),portA_in);*/
	return (portA_out & ddrA) | (portA_in & ~ddrA);
}

WRITE_HANDLER( bublbobl_68705_portA_w )
{
/*logerror("%04x: 68705 port A write %02x\n",activecpu_get_pc(),data);*/
	portA_out = data;
}

WRITE_HANDLER( bublbobl_68705_ddrA_w )
{
	ddrA = data;
}



/*
 *  Port B connections:
 *
 *  all bits are logical 1 when read (+5V pullup)
 *
 *  0   W  enables latch which holds data from main Z80 memory
 *  1   W  loads the latch which holds the low 8 bits of the address of
 *               the main Z80 memory location to access
 *  2   W  loads the latch which holds the high 4 bits of the address of
 *               the main Z80 memory location to access
 *         00-07 = read input ports
 *         0c-0f = access z80 memory at 0xfc00
 *  3   W  selects Z80 memory access direction (0 = write 1 = read)
 *  4   W  clocks main Z80 memory access (goes to a PAL)
 *  5   W  clocks a flip-flop which causes IRQ on the main Z80
 *  6   W  not used?
 *  7   W  not used?
 */

static unsigned char portB_in,portB_out,ddrB;

READ_HANDLER( bublbobl_68705_portB_r )
{
	return (portB_out & ddrB) | (portB_in & ~ddrB);
}

static int address,latch;

WRITE_HANDLER( bublbobl_68705_portB_w )
{
/*logerror("%04x: 68705 port B write %02x\n",activecpu_get_pc(),data);*/

	if ((ddrB & 0x01) && (~data & 0x01) && (portB_out & 0x01))
	{
		portA_in = latch;
	}
	if ((ddrB & 0x02) && (data & 0x02) && (~portB_out & 0x02)) /* positive edge trigger */
	{
		address = (address & 0xff00) | portA_out;
/*logerror("%04x: 68705 address %02x\n",activecpu_get_pc(),portA_out);*/
	}
	if ((ddrB & 0x04) && (data & 0x04) && (~portB_out & 0x04)) /* positive edge trigger */
	{
		address = (address & 0x00ff) | ((portA_out & 0x0f) << 8);
	}
	if ((ddrB & 0x10) && (~data & 0x10) && (portB_out & 0x10))
	{
		if (data & 0x08)	/* read */
		{
			if ((address & 0x0800) == 0x0000)
			{
/*logerror("%04x: 68705 read input port %02x\n",activecpu_get_pc(),address);*/
				latch = readinputport((address & 3) + 1);
			}
			else if ((address & 0x0c00) == 0x0c00)
			{
/*logerror("%04x: 68705 read %02x from address %04x\n",activecpu_get_pc(),bublbobl_sharedram2[address],address);*/
				latch = bublbobl_sharedram2[address & 0x03ff];
			}
			else
log_cb(RETRO_LOG_DEBUG, LOGPRE "%04x: 68705 unknown read address %04x\n",activecpu_get_pc(),address);
		}
		else	/* write */
		{
			if ((address & 0x0c00) == 0x0c00)
			{
/*logerror("%04x: 68705 write %02x to address %04x\n",activecpu_get_pc(),portA_out,address);*/
				bublbobl_sharedram2[address & 0x03ff] = portA_out;
			}
			else
log_cb(RETRO_LOG_DEBUG, LOGPRE "%04x: 68705 unknown write to address %04x\n",activecpu_get_pc(),address);
		}
	}
	if ((ddrB & 0x20) && (~data & 0x20) && (portB_out & 0x20))
	{
		/* hack to get random EXTEND letters (who is supposed to do this? 68705? PAL?) */
		bublbobl_sharedram2[0x7c] = rand()%6;

		cpu_irq_line_vector_w(0,0,bublbobl_sharedram2[0]);
		cpu_set_irq_line(0,0,HOLD_LINE);
	}
	if ((ddrB & 0x40) && (~data & 0x40) && (portB_out & 0x40))
	{
log_cb(RETRO_LOG_DEBUG, LOGPRE "%04x: 68705 unknown port B bit %02x\n",activecpu_get_pc(),data);
	}
	if ((ddrB & 0x80) && (~data & 0x80) && (portB_out & 0x80))
	{
log_cb(RETRO_LOG_DEBUG, LOGPRE "%04x: 68705 unknown port B bit %02x\n",activecpu_get_pc(),data);
	}

	portB_out = data;
}

WRITE_HANDLER( bublbobl_68705_ddrB_w )
{
	ddrB = data;
}
