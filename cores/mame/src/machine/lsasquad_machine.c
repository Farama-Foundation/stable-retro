#include "driver.h"
#include "cpu/z80/z80.h"
#include "random.h"


/* coin inputs are inverted in storming */
int lsasquad_invertcoin;


/***************************************************************************

 main <-> sound CPU communication

***************************************************************************/

static int sound_nmi_enable,pending_nmi,sound_cmd,sound_result;
int lsasquad_sound_pending;

static void nmi_callback(int param)
{
	if (sound_nmi_enable) cpu_set_irq_line(1,IRQ_LINE_NMI,PULSE_LINE);
	else pending_nmi = 1;
}

WRITE_HANDLER( lsasquad_sh_nmi_disable_w )
{
	sound_nmi_enable = 0;
}

WRITE_HANDLER( lsasquad_sh_nmi_enable_w )
{
	sound_nmi_enable = 1;
	if (pending_nmi)
	{
		cpu_set_irq_line(1,IRQ_LINE_NMI,PULSE_LINE);
		pending_nmi = 0;
	}
}

WRITE_HANDLER( lsasquad_sound_command_w )
{
	lsasquad_sound_pending |= 0x01;
	sound_cmd = data;
/*logerror("%04x: sound cmd %02x\n",activecpu_get_pc(),data);*/
	timer_set(TIME_NOW,data,nmi_callback);
}

READ_HANDLER( lsasquad_sh_sound_command_r )
{
	lsasquad_sound_pending &= ~0x01;
/*logerror("%04x: read sound cmd %02x\n",activecpu_get_pc(),sound_cmd);*/
	return sound_cmd;
}

WRITE_HANDLER( lsasquad_sh_result_w )
{
	lsasquad_sound_pending |= 0x02;
/*logerror("%04x: sound res %02x\n",activecpu_get_pc(),data);*/
	sound_result = data;
}

READ_HANDLER( lsasquad_sound_result_r )
{
	lsasquad_sound_pending &= ~0x02;
/*logerror("%04x: read sound res %02x\n",activecpu_get_pc(),sound_result);*/
	return sound_result;
}

READ_HANDLER( lsasquad_sound_status_r )
{
	/* bit 0: message pending for sound cpu */
	/* bit 1: message pending for main cpu */
	return lsasquad_sound_pending;
}

READ_HANDLER( daikaiju_sh_sound_command_r )
{
	lsasquad_sound_pending &= ~0x01;
	lsasquad_sound_pending |= 0x02;
/* logerror("%04x: read sound cmd %02x\n",activecpu_get_pc(),sound_cmd); */
	return sound_cmd;
}

READ_HANDLER( daikaiju_sound_status_r )
{
	/* bit 0: message pending for sound cpu */
	/* bit 1: message pending for main cpu */
	return lsasquad_sound_pending^3;
}



/***************************************************************************

 LSA Squad 68705 protection interface

 The following is ENTIRELY GUESSWORK!!!

***************************************************************************/

static unsigned char from_main,from_mcu;
static int mcu_sent = 0,main_sent = 0;

static unsigned char portA_in,portA_out,ddrA;

READ_HANDLER( lsasquad_68705_portA_r )
{
/*logerror("%04x: 68705 port A read %02x\n",activecpu_get_pc(),portA_in);*/
	return (portA_out & ddrA) | (portA_in & ~ddrA);
}

WRITE_HANDLER( lsasquad_68705_portA_w )
{
/*logerror("%04x: 68705 port A write %02x\n",activecpu_get_pc(),data);*/
	portA_out = data;
}

WRITE_HANDLER( lsasquad_68705_ddrA_w )
{
	ddrA = data;
}



/*
 *  Port B connections:
 *
 *  all bits are logical 1 when read (+5V pullup)
 *
 *  1   W  when 1->0, enables latch which brings the command from main CPU (read from port A)
 *  2   W  when 0->1, copies port A to the latch for the main CPU
 */

static unsigned char portB_in,portB_out,ddrB;

READ_HANDLER( lsasquad_68705_portB_r )
{
	return (portB_out & ddrB) | (portB_in & ~ddrB);
}

WRITE_HANDLER( lsasquad_68705_portB_w )
{
/*logerror("%04x: 68705 port B write %02x\n",activecpu_get_pc(),data);*/

	if ((ddrB & 0x02) && (~data & 0x02) && (portB_out & 0x02))
	{
		portA_in = from_main;
		if (main_sent) cpu_set_irq_line(2,0,CLEAR_LINE);
		main_sent = 0;
/*logerror("read command %02x from main cpu\n",portA_in);*/
	}
	if ((ddrB & 0x04) && (data & 0x04) && (~portB_out & 0x04))
	{
/*logerror("send command %02x to main cpu\n",portA_out);*/
		from_mcu = portA_out;
		mcu_sent = 1;
	}

	portB_out = data;
}

WRITE_HANDLER( lsasquad_68705_ddrB_w )
{
	ddrB = data;
}

WRITE_HANDLER( lsasquad_mcu_w )
{
/*logerror("%04x: mcu_w %02x\n",activecpu_get_pc(),data);*/
	from_main = data;
	main_sent = 1;
	cpu_set_irq_line(2,0,ASSERT_LINE);
}

READ_HANDLER( lsasquad_mcu_r )
{
/*logerror("%04x: mcu_r %02x\n",activecpu_get_pc(),from_mcu);*/
	mcu_sent = 0;
	return from_mcu;
}

READ_HANDLER( lsasquad_mcu_status_r )
{
	int res = input_port_3_r(0);

	/* bit 0 = when 1, mcu is ready to receive data from main cpu */
	/* bit 1 = when 0, mcu has sent data to the main cpu */
/*logerror("%04x: mcu_status_r\n",activecpu_get_pc());*/
	if (!main_sent) res |= 0x01;
	if (!mcu_sent) res |= 0x02;

	return res ^ lsasquad_invertcoin;
}

READ_HANDLER( daikaiju_mcu_status_r )
{
	int res = input_port_3_r(0);

	/* bit 0 = when 1, mcu is ready to receive data from main cpu */
	/* bit 1 = when 0, mcu has sent data to the main cpu */

	if (!main_sent)
		res |= 0x01;
	if (!mcu_sent)
		res |= 0x02;
	
	res |=((lsasquad_sound_pending & 0x02)^2)<<3; /* inverted flag */
	lsasquad_sound_pending &= ~0x02;
	return res;
}
