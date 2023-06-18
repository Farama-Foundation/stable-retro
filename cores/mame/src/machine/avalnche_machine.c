/***************************************************************************

	Atari Avalanche hardware

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "avalnche.h"

/***************************************************************************
  avalnche_input_r
***************************************************************************/

READ_HANDLER( avalnche_input_r )
{
	switch (offset & 0x03)
	{
		case 0x00:	 return input_port_0_r(offset);
		case 0x01:	 return input_port_1_r(offset);
		case 0x02:	 return input_port_2_r(offset);
		case 0x03:	 return 0; /* Spare */
	}
	return 0;
}

/***************************************************************************
  avalnche_output_w
***************************************************************************/

WRITE_HANDLER( avalnche_output_w )
{
	switch (offset & 0x07)
	{
		case 0x00:		/* 1 CREDIT LAMP */
	        set_led_status(0,data & 0x01);
			break;
		case 0x01:		/* ATTRACT */
			discrete_sound_w(4, (~data) & 0x01);
			break;
		case 0x02:		/* VIDEO INVERT */
			if (data & 0x01)
			{
				palette_set_color(0,0,0,0);
				palette_set_color(1,255,255,255);
			}
			else
			{
				palette_set_color(0,255,255,255);
				palette_set_color(1,0,0,0);
			}
			break;
		case 0x03:		/* 2 CREDIT LAMP */
	        set_led_status(1,data & 0x01);
			break;
		case 0x04:		/* AUD0 */
			discrete_sound_w(0, data & 0x01);
			break;
		case 0x05:		/* AUD1 */
			discrete_sound_w(1, data & 0x01);
			break;
		case 0x06:		/* AUD2 */
			discrete_sound_w(2, data & 0x01);
			break;
		case 0x07:		/* START LAMP (Serve button) */
	        set_led_status(2,data & 0x01);
			break;
	}
}

/***************************************************************************
  avalnche_noise_amplitude_w
***************************************************************************/

WRITE_HANDLER( avalnche_noise_amplitude_w )
{
	discrete_sound_w(3, data & 0x3f);
}

INTERRUPT_GEN( avalnche_interrupt )
{
		cpu_set_irq_line(0, IRQ_LINE_NMI, PULSE_LINE);
}
