/***************************************************************************

	Taito Qix hardware

	driver by John Butler, Ed Mueller, Aaron Giles

***************************************************************************/

#include "driver.h"
#include "qix.h"
#include "6821pia.h"
#include "cpu/m6800/m6800.h"
#include "cpu/m6805/m6805.h"
#include "cpu/m6809/m6809.h"


/* Globals */
UINT8 *qix_sharedram;
UINT8 *qix_68705_port_out;
UINT8 *qix_68705_ddr;


/* Local variables */
static UINT8 qix_68705_port_in[3];
static UINT8 qix_coinctrl;


/* Prototypes */
static READ_HANDLER( qixmcu_coin_r );
static WRITE_HANDLER( qixmcu_coinctrl_w );
static WRITE_HANDLER( qixmcu_coin_w );

static WRITE_HANDLER( qix_dac_w );
static WRITE_HANDLER( sync_pia_4_porta_w );

static WRITE_HANDLER( qix_inv_flag_w );

static WRITE_HANDLER( qix_coinctl_w );
static WRITE_HANDLER( slither_coinctl_w );

static void qix_pia_sint(int state);
static void qix_pia_dint(int state);

static WRITE_HANDLER( slither_76489_0_w );
static WRITE_HANDLER( slither_76489_1_w );

static READ_HANDLER( slither_trak_lr_r );
static READ_HANDLER( slither_trak_ud_r );



/***************************************************************************

	Qix has 6 PIAs on board:

	From the ROM I/O schematic:

	PIA 0 = U11: (mapped to $9400 on the data CPU)
		port A = external input (input_port_0)
		port B = external input (input_port_1) (coin)

	PIA 1 = U20: (mapped to $9800/$9900 on the data CPU)
		port A = external input (???)
		port B = external input (???)

	PIA 2 = U30: (mapped to $9c00 on the data CPU)
		port A = external input (???)
		port B = external input (???)


	From the data/sound processor schematic:

	PIA 3 = U20: (mapped to $9000 on the data CPU)
		port A = data CPU to sound CPU communication
		port B = some kind of sound control, 2 4-bit values
		CA1 = interrupt signal from sound CPU
		CA2 = interrupt signal to sound CPU
		CB1 = VS input signal (vertical sync)
		CB2 = INV output signal (cocktail flip)
		IRQA = /DINT1 signal
		IRQB = /DINT1 signal

	PIA 4 = U8: (mapped to $4000 on the sound CPU)
		port A = sound CPU to data CPU communication
		port B = DAC value (port B)
		CA1 = interrupt signal from data CPU
		CA2 = interrupt signal to data CPU
		IRQA = /SINT1 signal
		IRQB = /SINT1 signal

	PIA 5 = U7: (never actually used, mapped to $2000 on the sound CPU)
		port A = unused
		port B = sound CPU to TMS5220 communication
		CA1 = interrupt signal from TMS5220
		CA2 = write signal to TMS5220
		CB1 = ready signal from TMS5220
		CB2 = read signal to TMS5220
		IRQA = /SINT2 signal
		IRQB = /SINT2 signal

***************************************************************************/

static struct pia6821_interface qix_pia_0_intf =
{
	/*inputs : A/B,CA/B1,CA/B2 */ input_port_0_r, input_port_1_r, 0, 0, 0, 0,
	/*outputs: A/B,CA/B2       */ 0, 0, 0, 0,
	/*irqs   : A/B             */ 0, 0
};

static struct pia6821_interface qix_pia_1_intf =
{
	/*inputs : A/B,CA/B1,CA/B2 */ input_port_2_r, input_port_3_r, 0, 0, 0, 0,
	/*outputs: A/B,CA/B2       */ 0, 0, 0, 0,
	/*irqs   : A/B             */ 0, 0
};

static struct pia6821_interface qix_pia_2_intf =
{
	/*inputs : A/B,CA/B1,CA/B2 */ input_port_4_r, 0, 0, 0, 0, 0,
	/*outputs: A/B,CA/B2       */ 0, qix_coinctl_w, 0, 0,
	/*irqs   : A/B             */ 0, 0
};

static struct pia6821_interface qix_pia_3_intf =
{
	/*inputs : A/B,CA/B1,CA/B2 */ 0, 0, 0, 0, 0, 0,
	/*outputs: A/B,CA/B2       */ sync_pia_4_porta_w, 0, pia_4_ca1_w, qix_inv_flag_w,
	/*irqs   : A/B             */ qix_pia_dint, qix_pia_dint
};

static struct pia6821_interface qix_pia_4_intf =
{
	/*inputs : A/B,CA/B1,CA/B2 */ pia_4_porta_r, 0, 0, 0, 0, 0,
	/*outputs: A/B,CA/B2       */ pia_3_porta_w, qix_dac_w, pia_3_ca1_w, 0,
	/*irqs   : A/B             */ qix_pia_sint, qix_pia_sint
};

static struct pia6821_interface qix_pia_5_intf =
{
	/*inputs : A/B,CA/B1,CA/B2 */ 0, 0, 0, 0, 0, 0,
	/*outputs: A/B,CA/B2       */ pia_3_porta_w, qix_dac_w, pia_3_ca1_w, 0,
	/*irqs   : A/B             */ 0, 0
};



/***************************************************************************

	Games with an MCU need to handle coins differently, and provide
	communication with the MCU

***************************************************************************/

static struct pia6821_interface qixmcu_pia_0_intf =
{
	/*inputs : A/B,CA/B1,CA/B2 */ input_port_0_r, qixmcu_coin_r, 0, 0, 0, 0,
	/*outputs: A/B,CA/B2       */ 0, qixmcu_coin_w, 0, 0,
	/*irqs   : A/B             */ 0, 0
};

static struct pia6821_interface qixmcu_pia_2_intf =
{
	/*inputs : A/B,CA/B1,CA/B2 */ input_port_4_r, 0, 0, 0, 0, 0,
	/*outputs: A/B,CA/B2       */ 0, qixmcu_coinctrl_w, 0, 0,
	/*irqs   : A/B             */ 0, 0
};



/***************************************************************************

	Slither uses 2 SN76489's for sound instead of the 6802+DAC; these
	are accessed via the PIAs.

***************************************************************************/

static struct pia6821_interface slither_pia_1_intf =
{
	/*inputs : A/B,CA/B1,CA/B2 */ slither_trak_lr_r, 0, 0, 0, 0, 0,
	/*outputs: A/B,CA/B2       */ 0, slither_76489_0_w, 0, 0,
	/*irqs   : A/B             */ 0, 0
};

static struct pia6821_interface slither_pia_2_intf =
{
	/*inputs : A/B,CA/B1,CA/B2 */ slither_trak_ud_r, 0, 0, 0, 0, 0,
	/*outputs: A/B,CA/B2       */ 0, slither_76489_1_w, 0, 0,
	/*irqs   : A/B             */ 0, 0
};

static struct pia6821_interface slither_pia_3_intf =
{
	/*inputs : A/B,CA/B1,CA/B2 */ input_port_2_r, 0, 0, 0, 0, 0,
	/*outputs: A/B,CA/B2       */ 0, slither_coinctl_w, 0, qix_inv_flag_w,
	/*irqs   : A/B             */ qix_pia_dint, qix_pia_dint
};



/*************************************
 *
 *	Machine initialization
 *
 *************************************/

MACHINE_INIT( qix )
{
	/* set a timer for the first scanline */
	timer_set(cpu_getscanlinetime(0), 0, qix_scanline_callback);

	/* configure and reset the PIAs */
	pia_unconfig();
	pia_config(0, PIA_STANDARD_ORDERING, &qix_pia_0_intf);
	pia_config(1, PIA_STANDARD_ORDERING, &qix_pia_1_intf);
	pia_config(2, PIA_STANDARD_ORDERING, &qix_pia_2_intf);
	pia_config(3, PIA_STANDARD_ORDERING, &qix_pia_3_intf);
	pia_config(4, PIA_STANDARD_ORDERING, &qix_pia_4_intf);
	pia_config(5, PIA_STANDARD_ORDERING, &qix_pia_5_intf);
	pia_reset();
}


MACHINE_INIT( qixmcu )
{
	/* set a timer for the first scanline */
	timer_set(cpu_getscanlinetime(0), 0, qix_scanline_callback);

	/* configure and reset the PIAs */
	pia_unconfig();
	pia_config(0, PIA_STANDARD_ORDERING, &qixmcu_pia_0_intf);
	pia_config(1, PIA_STANDARD_ORDERING, &qix_pia_1_intf);
	pia_config(2, PIA_STANDARD_ORDERING, &qixmcu_pia_2_intf);
	pia_config(3, PIA_STANDARD_ORDERING, &qix_pia_3_intf);
	pia_config(4, PIA_STANDARD_ORDERING, &qix_pia_4_intf);
	pia_config(5, PIA_STANDARD_ORDERING, &qix_pia_5_intf);
	pia_reset();

	/* reset the coin counter register */
	qix_coinctrl = 0x00;
}


MACHINE_INIT( slither )
{
	/* set a timer for the first scanline */
	timer_set(cpu_getscanlinetime(0), 0, qix_scanline_callback);

	/* configure and reset the PIAs */
	pia_unconfig();
	pia_config(0, PIA_STANDARD_ORDERING, &qix_pia_0_intf);
	pia_config(1, PIA_STANDARD_ORDERING, &slither_pia_1_intf);
	pia_config(2, PIA_STANDARD_ORDERING, &slither_pia_2_intf);
	pia_config(3, PIA_STANDARD_ORDERING, &slither_pia_3_intf);
	pia_reset();
}



/*************************************
 *
 *	VSYNC interrupt handling
 *
 *************************************/

static void vblank_stop(int param)
{
	pia_3_cb1_w(0, 0);
}


INTERRUPT_GEN( qix_vblank_start )
{
	pia_3_cb1_w(0, 1);
	timer_set(cpu_getscanlinetime(0), 0, vblank_stop);
}



/*************************************
 *
 *	Shared RAM
 *
 *************************************/

READ_HANDLER( qix_sharedram_r )
{
	return qix_sharedram[offset];
}


WRITE_HANDLER( qix_sharedram_w )
{
	qix_sharedram[offset] = data;
}



/*************************************
 *
 *	Zoo Keeper bankswitching
 *
 *************************************/

WRITE_HANDLER( zoo_bankswitch_w )
{
	UINT8 *RAM = memory_region(REGION_CPU2);

	if (data & 0x04)
		cpu_setbank(1, &RAM[0x10000]);
	else
		cpu_setbank(1, &RAM[0xa000]);
}



/*************************************
 *
 *	Data CPU FIRQ generation/ack
 *
 *************************************/

WRITE_HANDLER( qix_data_firq_w )
{
	cpu_set_irq_line(0, M6809_FIRQ_LINE, ASSERT_LINE);
}


WRITE_HANDLER( qix_data_firq_ack_w )
{
	cpu_set_irq_line(0, M6809_FIRQ_LINE, CLEAR_LINE);
}


READ_HANDLER( qix_data_firq_r )
{
	cpu_set_irq_line(0, M6809_FIRQ_LINE, ASSERT_LINE);
	return 0xff;
}


READ_HANDLER( qix_data_firq_ack_r )
{
	cpu_set_irq_line(0, M6809_FIRQ_LINE, CLEAR_LINE);
	return 0xff;
}



/*************************************
 *
 *	Video CPU FIRQ generation/ack
 *
 *************************************/

WRITE_HANDLER( qix_video_firq_w )
{
	cpu_set_irq_line(1, M6809_FIRQ_LINE, ASSERT_LINE);
}


WRITE_HANDLER( qix_video_firq_ack_w )
{
	cpu_set_irq_line(1, M6809_FIRQ_LINE, CLEAR_LINE);
}


READ_HANDLER( qix_video_firq_r )
{
	cpu_set_irq_line(1, M6809_FIRQ_LINE, ASSERT_LINE);
	return 0xff;
}


READ_HANDLER( qix_video_firq_ack_r )
{
	cpu_set_irq_line(1, M6809_FIRQ_LINE, CLEAR_LINE);
	return 0xff;
}



/*************************************
 *
 *	Sound PIA interfaces
 *
 *************************************/

static WRITE_HANDLER( qix_dac_w )
{
	DAC_data_w(0, data);
}


static void deferred_pia_4_porta_w(int data)
{
	pia_4_porta_w(0, data);
}


static WRITE_HANDLER( sync_pia_4_porta_w )
{
	/* we need to synchronize this so the sound CPU doesn't drop anything important */
	timer_set(TIME_NOW, data, deferred_pia_4_porta_w);
}



/*************************************
 *
 *	IRQ generation
 *
 *************************************/

static void qix_pia_dint(int state)
{
	/* DINT is connected to the data CPU's IRQ line */
	cpu_set_irq_line(0, M6809_IRQ_LINE, state ? ASSERT_LINE : CLEAR_LINE);
}


static void qix_pia_sint(int state)
{
	/* SINT is connected to the sound CPU's IRQ line */
	cpu_set_irq_line(2, M6802_IRQ_LINE, state ? ASSERT_LINE : CLEAR_LINE);
}



/*************************************
 *
 *	68705 Communication
 *
 *************************************/

READ_HANDLER( qixmcu_coin_r )
{
	return qix_68705_port_out[0];
}


static WRITE_HANDLER( qixmcu_coin_w )
{
	/* this is a callback called by pia_0_w(), so I don't need to synchronize */
	/* the CPUs - they have already been synchronized by qix_pia_0_w() */
	qix_68705_port_in[0] = data;
}


static WRITE_HANDLER( qixmcu_coinctrl_w )
{
	if (data & 0x04)
	{
		cpu_set_irq_line(3, M6809_IRQ_LINE, ASSERT_LINE);
		/* spin for a while to let the 68705 write the result */
		cpu_spinuntil_time(TIME_IN_USEC(50));
	}
	else
		cpu_set_irq_line(3, M6809_IRQ_LINE, CLEAR_LINE);

	/* this is a callback called by pia_0_w(), so I don't need to synchronize */
	/* the CPUs - they have already been synchronized by qix_pia_0_w() */
	qix_coinctrl = data;
}



/*************************************
 *
 *	68705 Port Inputs
 *
 *************************************/

READ_HANDLER( qix_68705_portA_r )
{
	UINT8 ddr = qix_68705_ddr[0];
	UINT8 out = qix_68705_port_out[0];
	UINT8 in = qix_68705_port_in[0];
	return (out & ddr) | (in & ~ddr);
}


READ_HANDLER( qix_68705_portB_r )
{
	UINT8 ddr = qix_68705_ddr[1];
	UINT8 out = qix_68705_port_out[1];
	UINT8 in = (readinputport(1) & 0x0f) | ((readinputport(1) & 0x80) >> 3);
	return (out & ddr) | (in & ~ddr);
}


READ_HANDLER( qix_68705_portC_r )
{
	UINT8 ddr = qix_68705_ddr[2];
	UINT8 out = qix_68705_port_out[2];
	UINT8 in = (~qix_coinctrl & 0x08) | ((readinputport(1) & 0x70) >> 4);
	return (out & ddr) | (in & ~ddr);
}



/*************************************
 *
 *	68705 Port Outputs
 *
 *************************************/

WRITE_HANDLER( qix_68705_portA_w )
{
	qix_68705_port_out[0] = data;
}


WRITE_HANDLER( qix_68705_portB_w )
{
	qix_68705_port_out[1] = data;
	coin_lockout_w(0, (~data >> 6) & 1);
	coin_counter_w(0, (data >> 7) & 1);
}


WRITE_HANDLER( qix_68705_portC_w )
{
	qix_68705_port_out[2] = data;
}



/*************************************
 *
 *	Data CPU PIA 0 synchronization
 *
 *************************************/

static void pia_0_w_callback(int param)
{
	pia_0_w(param >> 8, param & 0xff);
}


WRITE_HANDLER( qix_pia_0_w )
{
	/* make all the CPUs synchronize, and only AFTER that write the command to the PIA */
	/* otherwise the 68705 will miss commands */
	timer_set(TIME_NOW, data | (offset << 8), pia_0_w_callback);
}



/*************************************
 *
 *	PIA/Protection(?) workarounds
 *
 *************************************/

WRITE_HANDLER( zookeep_pia_0_w )
{
	/* Hack: Kram and Zoo Keeper for some reason (protection?) leave the port A */
	/* DDR set to 0xff, so they cannot read the player 1 controls. Here we force */
	/* the DDR to 0, so the controls work correctly. */
	if (offset == 0)
		data = 0;
	qix_pia_0_w(offset, data);
}


WRITE_HANDLER( zookeep_pia_2_w )
{
	/* Hack: Zoo Keeper for some reason (protection?) leaves the port A */
	/* DDR set to 0xff, so they cannot read the player 2 controls. Here we force */
	/* the DDR to 0, so the controls work correctly. */
	if (offset == 0)
		data = 0;
	pia_2_w(offset, data);
}



/*************************************
 *
 *	Cocktail flip
 *
 *************************************/

static WRITE_HANDLER( qix_inv_flag_w )
{
	qix_cocktail_flip = data;
}



/*************************************
 *
 *	Coin I/O for games without coin CPU
 *
 *************************************/

static WRITE_HANDLER( qix_coinctl_w )
{
	coin_lockout_w(0, (~data >> 2) & 1);
	coin_counter_w(0, (data >> 1) & 1);
}


static WRITE_HANDLER( slither_coinctl_w )
{
	coin_lockout_w(0, (~data >> 6) & 1);
	coin_counter_w(0, (data >> 5) & 1);
}



/*************************************
 *
 *	Slither SN76489 I/O
 *
 *************************************/

static WRITE_HANDLER( slither_76489_0_w )
{
	/* write to the sound chip */
	SN76496_0_w(0, data);

	/* clock the ready line going back into CB1 */
	pia_1_cb1_w(0, 0);
	pia_1_cb1_w(0, 1);
}


static WRITE_HANDLER( slither_76489_1_w )
{
	/* write to the sound chip */
	SN76496_1_w(0, data);

	/* clock the ready line going back into CB1 */
	pia_2_cb1_w(0, 0);
	pia_2_cb1_w(0, 1);
}



/*************************************
 *
 *	Slither trackball I/O
 *
 *************************************/

static READ_HANDLER( slither_trak_lr_r )
{
	return readinputport(qix_cocktail_flip ? 6 : 4);
}


static READ_HANDLER( slither_trak_ud_r )
{
	return readinputport(qix_cocktail_flip ? 5 : 3);
}
