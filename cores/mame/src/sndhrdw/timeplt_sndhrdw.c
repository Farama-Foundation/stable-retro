/***************************************************************************

	This code is used by the following module:

	timeplt.c
	pooyan.c
	locomotn.c
	tutankhm.c
	rocnrope.c

***************************************************************************/

#include "driver.h"

static READ_HANDLER( timeplt_portB_r );
static WRITE_HANDLER( timeplt_filter_w );

MEMORY_READ_START( timeplt_sound_readmem )
	{ 0x0000, 0x1fff, MRA_ROM },
	{ 0x2000, 0x23ff, MRA_RAM },
	{ 0x3000, 0x33ff, MRA_RAM },
	{ 0x4000, 0x4000, AY8910_read_port_0_r },
	{ 0x6000, 0x6000, AY8910_read_port_1_r },
MEMORY_END

MEMORY_WRITE_START( timeplt_sound_writemem )
	{ 0x0000, 0x1fff, MWA_ROM },
	{ 0x2000, 0x23ff, MWA_RAM },
	{ 0x3000, 0x33ff, MWA_RAM },
	{ 0x4000, 0x4000, AY8910_write_port_0_w },
	{ 0x5000, 0x5000, AY8910_control_port_0_w },
	{ 0x6000, 0x6000, AY8910_write_port_1_w },
	{ 0x7000, 0x7000, AY8910_control_port_1_w },
	{ 0x8000, 0x8fff, timeplt_filter_w },
MEMORY_END


struct AY8910interface timeplt_ay8910_interface =
{
	2,				/* 2 chips */
	14318180/8,		/* 1.789772727 MHz */
	{ MIXERG(30,MIXER_GAIN_2x,MIXER_PAN_CENTER), MIXERG(30,MIXER_GAIN_2x,MIXER_PAN_CENTER) },
	{ soundlatch_r },
	{ timeplt_portB_r },
	{ 0 },
	{ 0 }
};


/* The timer clock which feeds the upper 4 bits of    					*/
/* AY-3-8910 port A is based on the same clock        					*/
/* feeding the sound CPU Z80.  It is a divide by      					*/
/* 5120, formed by a standard divide by 512,        					*/
/* followed by a divide by 10 using a 4 bit           					*/
/* bi-quinary count sequence. (See LS90 data sheet    					*/
/* for an example).                                   					*/
/*																		*/
/* Bit 4 comes from the output of the divide by 1024  					*/
/*       0, 1, 0, 1, 0, 1, 0, 1, 0, 1									*/
/* Bit 5 comes from the QC output of the LS90 producing a sequence of	*/
/* 		 0, 0, 1, 1, 0, 0, 1, 1, 1, 0									*/
/* Bit 6 comes from the QD output of the LS90 producing a sequence of	*/
/*		 0, 0, 0, 0, 1, 0, 0, 0, 0, 1									*/
/* Bit 7 comes from the QA output of the LS90 producing a sequence of	*/
/*		 0, 0, 0, 0, 0, 1, 1, 1, 1, 1			 						*/

static int timeplt_timer[10] =
{
	0x00, 0x10, 0x20, 0x30, 0x40, 0x90, 0xa0, 0xb0, 0xa0, 0xd0
};

static READ_HANDLER( timeplt_portB_r )
{
	/* need to protect from totalcycles overflow */
	static int last_totalcycles = 0;

	/* number of Z80 clock cycles to count */
	static int clock;

	int current_totalcycles;

	current_totalcycles = activecpu_gettotalcycles();
	clock = (clock + (current_totalcycles-last_totalcycles)) % 5120;

	last_totalcycles = current_totalcycles;

	return timeplt_timer[clock/512];
}


static void filter_w(int chip, int channel, int data)
{
	int C = 0;

	if (data & 1) C += 220000;	/* 220000pF = 0.220uF */
	if (data & 2) C +=  47000;	/*  47000pF = 0.047uF */
	set_RC_filter(3*chip + channel,1000,5100,0,C);
}

static WRITE_HANDLER( timeplt_filter_w )
{
	filter_w(0, 0, (offset >>  6) & 3);
	filter_w(0, 1, (offset >>  8) & 3);
	filter_w(0, 2, (offset >> 10) & 3);
	filter_w(1, 0, (offset >>  0) & 3);
	filter_w(1, 1, (offset >>  2) & 3);
	filter_w(1, 2, (offset >>  4) & 3);
}


WRITE_HANDLER( timeplt_sh_irqtrigger_w )
{
	static int last;

	if (last == 0 && data)
	{
		/* setting bit 0 low then high triggers IRQ on the sound CPU */
		cpu_set_irq_line_and_vector(1,0,HOLD_LINE,0xff);
	}

	last = data;
}
