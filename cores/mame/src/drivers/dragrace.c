/***************************************************************************

Atari Drag Race Driver

***************************************************************************/

#include "driver.h"

extern VIDEO_START( dragrace );
extern VIDEO_UPDATE( dragrace );

extern UINT8* dragrace_playfield_ram;
extern UINT8* dragrace_position_ram;

static unsigned dragrace_misc_flags = 0;

static int dragrace_gear[2];


static void dragrace_frame_callback(int dummy)
{
	int i;

	for (i = 0; i < 2; i++)
	{
		switch (readinputport(5 + i))
		{
		case 0x01: dragrace_gear[i] = 1; break;
		case 0x02: dragrace_gear[i] = 2; break;
		case 0x04: dragrace_gear[i] = 3; break;
		case 0x08: dragrace_gear[i] = 4; break;
		case 0x10: dragrace_gear[i] = 0; break;
		}
	}
}


static MACHINE_INIT( dragrace )
{
	timer_pulse(cpu_getscanlinetime(0), 0, dragrace_frame_callback);
}

static void dragrace_update_misc_flags(void)
{
	/* 0x0900 = set 3SPEED1			0x00000001
	 * 0x0901 = set 4SPEED1			0x00000002
	 * 0x0902 = set 5SPEED1			0x00000004
	 * 0x0903 = set 6SPEED1			0x00000008
	 * 0x0904 = set 7SPEED1			0x00000010
	 * 0x0905 = set EXPLOSION1		0x00000020
	 * 0x0906 = set SCREECH1		0x00000040
	 * 0x0920 - 0x0927 = clear 0x0900 - 0x0907

	 * 0x0909 = set KLEXPL1			0x00000200
	 * 0x090b = set MOTOR1			0x00000800
	 * 0x090c = set ATTRACT			0x00001000
	 * 0x090d = set LOTONE			0x00002000
	 * 0x090f = set Player 1 Start Lamp	0x00008000
	 * 0x0928 - 0x092f = clear 0x0908 - 0x090f

	 * 0x0910 = set 3SPEED2			0x00010000
	 * 0x0911 = set 4SPEED2			0x00020000
	 * 0x0912 = set 5SPEED2			0x00040000
	 * 0x0913 = set 6SPEED2			0x00080000
	 * 0x0914 = set 7SPEED2			0x00100000
	 * 0x0915 = set EXPLOSION2		0x00200000
	 * 0x0916 = set SCREECH2		0x00400000
	 * 0x0930 = clear 0x0910 - 0x0917

	 * 0x0919 = set KLEXPL2			0x02000000
	 * 0x091b = set MOTOR2			0x08000000
	 * 0x091d = set HITONE			0x20000000
	 * 0x091f = set Player 2 Start Lamp	0x80000000
	 * 0x0938 = clear 0x0918 - 0x091f
	 */
	set_led_status(0, dragrace_misc_flags & 0x00008000);
	set_led_status(1, dragrace_misc_flags & 0x80000000);

	discrete_sound_w(0x06,  ~dragrace_misc_flags & 0x0000001f);		/* Speed1 data**/
	discrete_sound_w(0x04, (dragrace_misc_flags & 0x00000020) ? 1: 0);	/* Explosion1 enable*/
	discrete_sound_w(0x00, (dragrace_misc_flags & 0x00000040) ? 1: 0);	/* Screech1 enable*/
	discrete_sound_w(0x0a, (dragrace_misc_flags & 0x00000200) ? 1: 0);	/* KLEXPL1 enable*/
	discrete_sound_w(0x08, (dragrace_misc_flags & 0x00000800) ? 1: 0);	/* Motor1 enable*/

	discrete_sound_w(0x07, (~dragrace_misc_flags & 0x001f0000) >> 0x10);	/* Speed2 data**/
	discrete_sound_w(0x05, (dragrace_misc_flags & 0x00200000) ? 1: 0);	/* Explosion2 enable*/
	discrete_sound_w(0x01, (dragrace_misc_flags & 0x00400000) ? 1: 0);	/* Screech2 enable*/
	discrete_sound_w(0x0b, (dragrace_misc_flags & 0x02000000) ? 1: 0);	/* KLEXPL2 enable*/
	discrete_sound_w(0x09, (dragrace_misc_flags & 0x08000000) ? 1: 0);	/* Motor2 enable*/

	discrete_sound_w(0x0c, (dragrace_misc_flags & 0x00001000) ? 1: 0);	/* Attract enable*/
	discrete_sound_w(0x02, (dragrace_misc_flags & 0x00002000) ? 1: 0);	/* LoTone enable*/
	discrete_sound_w(0x03, (dragrace_misc_flags & 0x20000000) ? 1: 0);	/* HiTone enable*/
}

WRITE_HANDLER( dragrace_misc_w )
{
	/* Set/clear individual bit */
	UINT32 mask = 1 << offset;
	if (data & 0x01)
		dragrace_misc_flags |= mask;
	else
		dragrace_misc_flags &= (~mask);
	log_cb(RETRO_LOG_DEBUG, LOGPRE "Set   %#6x, Mask=%#10x, Flag=%#10x, Data=%x\n", 0x0900+offset, mask, dragrace_misc_flags, data & 0x01);
	dragrace_update_misc_flags();
	}

WRITE_HANDLER( dragrace_misc_clear_w )
{
	/* Clear 8 bits */
	UINT32 mask = 0xff << (((offset >> 3) & 0x03) * 8);
	dragrace_misc_flags &= (~mask);
	log_cb(RETRO_LOG_DEBUG, LOGPRE "Clear %#6x, Mask=%#10x, Flag=%#10x, Data=%x\n", 0x0920+offset, mask, dragrace_misc_flags, data & 0x01);
	dragrace_update_misc_flags();
}

READ_HANDLER( dragrace_input_r )
{
	int val = readinputport(2);

	UINT8 maskA = 1 << (offset % 8);
	UINT8 maskB = 1 << (offset / 8);

	int i;

	for (i = 0; i < 2; i++)
	{
		int in = readinputport(i);

		if (dragrace_gear[i] != 0)
		{
			in &= ~(1 << dragrace_gear[i]);
		}

		if (in & maskA)
		{
			val |= 1 << i;
		}
	}

	return (val & maskB) ? 0xFF : 0x7F;
}


READ_HANDLER( dragrace_steering_r )
{
	int bitA[2];
	int bitB[2];

	int i;

	for (i = 0; i < 2; i++)
	{
		int dial = readinputport(3 + i);

		bitA[i] = ((dial + 1) / 2) & 1;
		bitB[i] = ((dial + 0) / 2) & 1;
	}

	return
		(bitA[0] << 0) | (bitB[0] << 1) |
		(bitA[1] << 2) | (bitB[1] << 3);
}


READ_HANDLER( dragrace_scanline_r )
{
	return (cpu_getscanline() ^ 0xf0) | 0x0f;
}


static MEMORY_READ_START( dragrace_readmem )
	{ 0x0080, 0x00ff, MRA_RAM },
	{ 0x0800, 0x083f, dragrace_input_r },
	{ 0x0c00, 0x0c00, dragrace_steering_r },
	{ 0x0d00, 0x0d00, dragrace_scanline_r },
	{ 0x1000, 0x1fff, MRA_ROM }, /* program */
	{ 0xf800, 0xffff, MRA_ROM }, /* program mirror */
MEMORY_END

static MEMORY_WRITE_START( dragrace_writemem )
	{ 0x0080, 0x00ff, MWA_RAM },
	{ 0x0900, 0x091f, dragrace_misc_w },
	{ 0x0920, 0x093f, dragrace_misc_clear_w },
	{ 0x0a00, 0x0aff, MWA_RAM, &dragrace_playfield_ram },
	{ 0x0b00, 0x0bff, MWA_RAM, &dragrace_position_ram },
	{ 0x0e00, 0x0e00, MWA_NOP }, /* watchdog (disabled in service mode) */
	{ 0x1000, 0x1fff, MWA_ROM }, /* program */
	{ 0xf800, 0xffff, MWA_ROM }, /* program mirror */
MEMORY_END


INPUT_PORTS_START( dragrace )
	PORT_START /* IN0 */
	PORT_BITX( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1, "Player 1 Gas",  IP_KEY_DEFAULT, IP_JOY_DEFAULT )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNUSED ) /* player 1 gear 1 */
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNUSED ) /* player 1 gear 2 */
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNUSED ) /* player 1 gear 3 */
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNUSED ) /* player 1 gear 4 */
	PORT_SERVICE( 0x20, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0xc0, 0x80, "Extended Play" )
	PORT_DIPSETTING( 0x00, "6.9 seconds" )
	PORT_DIPSETTING( 0x80, "5.9 seconds" )
	PORT_DIPSETTING( 0x40, "4.9 seconds" )
	PORT_DIPSETTING( 0xc0, "Never" )

	PORT_START /* IN1 */
	PORT_BITX( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2, "Player 2 Gas",  IP_KEY_DEFAULT, IP_JOY_DEFAULT )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNUSED ) /* player 2 gear 1 */
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNUSED ) /* player 2 gear 2 */
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNUSED ) /* player 2 gear 3 */
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNUSED ) /* player 2 gear 4 */
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_DIPNAME( 0xc0, 0x80, "Number Of Heats" )
	PORT_DIPSETTING( 0xc0, "3" )
	PORT_DIPSETTING( 0x80, "4" )
	PORT_DIPSETTING( 0x00, "5" )

	PORT_START /* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_UNUSED ) /* IN0 connects here */
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_UNUSED ) /* IN1 connects here */
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_DIPNAME( 0xc0, 0x40, DEF_STR( Coinage ) )
	PORT_DIPSETTING( 0xc0, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING( 0x40, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING( 0x80, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING( 0x00, DEF_STR( Free_Play ) )

	PORT_START /* IN3 */
	PORT_ANALOG( 0xff, 0x00, IPT_DIAL_V | IPF_PLAYER1, 25, 10, 0, 0 )

	PORT_START /* IN4 */
	PORT_ANALOG( 0xff, 0x00, IPT_DIAL_V | IPF_PLAYER2, 25, 10, 0, 0 )

	PORT_START /* IN5 */
	PORT_BITX(0x01, IP_ACTIVE_HIGH, IPT_BUTTON2 | IPF_PLAYER1, "Player 1 Gear 1",  IP_KEY_DEFAULT, IP_JOY_DEFAULT )
	PORT_BITX(0x02, IP_ACTIVE_HIGH, IPT_BUTTON3 | IPF_PLAYER1, "Player 1 Gear 2",  IP_KEY_DEFAULT, IP_JOY_DEFAULT )
	PORT_BITX(0x04, IP_ACTIVE_HIGH, IPT_BUTTON4 | IPF_PLAYER1, "Player 1 Gear 3",  IP_KEY_DEFAULT, IP_JOY_DEFAULT )
	PORT_BITX(0x08, IP_ACTIVE_HIGH, IPT_BUTTON5 | IPF_PLAYER1, "Player 1 Gear 4",  IP_KEY_DEFAULT, IP_JOY_DEFAULT )
	PORT_BITX(0x10, IP_ACTIVE_HIGH, IPT_BUTTON6 | IPF_PLAYER1, "Player 1 Neutral", IP_KEY_DEFAULT, IP_JOY_DEFAULT )

	PORT_START /* IN6 */
	PORT_BITX(0x01, IP_ACTIVE_HIGH, IPT_BUTTON2 | IPF_PLAYER2, "Player 2 Gear 1",  IP_KEY_DEFAULT, IP_JOY_DEFAULT )
	PORT_BITX(0x02, IP_ACTIVE_HIGH, IPT_BUTTON3 | IPF_PLAYER2, "Player 2 Gear 2",  IP_KEY_DEFAULT, IP_JOY_DEFAULT )
	PORT_BITX(0x04, IP_ACTIVE_HIGH, IPT_BUTTON4 | IPF_PLAYER2, "Player 2 Gear 3",  IP_KEY_DEFAULT, IP_JOY_DEFAULT )
	PORT_BITX(0x08, IP_ACTIVE_HIGH, IPT_BUTTON5 | IPF_PLAYER2, "Player 2 Gear 4",  IP_KEY_DEFAULT, IP_JOY_DEFAULT )
	PORT_BITX(0x10, IP_ACTIVE_HIGH, IPT_BUTTON6 | IPF_PLAYER2, "Player 2 Neutral", IP_KEY_DEFAULT, IP_JOY_DEFAULT )
	PORT_START
	PORT_ADJUSTER( 81, "Motor 1 RPM" )

	PORT_START
	PORT_ADJUSTER( 85, "Motor 2 RPM" )
INPUT_PORTS_END


static struct GfxLayout dragrace_tile_layout1 =
{
	16, 16,   /* width, height */
	0x40,     /* total         */
	1,        /* planes        */
	{ 0 },    /* plane offsets */
	{
		0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
		0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87
	},
	{
		0x00, 0x08, 0x10, 0x18, 0x20, 0x28, 0x30, 0x38,
		0x40, 0x48, 0x50, 0x58, 0x60, 0x68, 0x70, 0x78
	},
	0x100      /* increment */
};


static struct GfxLayout dragrace_tile_layout2 =
{
	16, 16,   /* width, height */
	0x20,     /* total         */
	2,        /* planes        */
	{         /* plane offsets */
		0x0000, 0x2000
	},
	{
		0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
		0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87
	},
	{
		0x00, 0x08, 0x10, 0x18, 0x20, 0x28, 0x30, 0x38,
		0x40, 0x48, 0x50, 0x58, 0x60, 0x68, 0x70, 0x78
	},
	0x100      /* increment */
};


static struct GfxDecodeInfo dragrace_gfx_decode_info[] =
{
	{ REGION_GFX1, 0, &dragrace_tile_layout1, 0, 4 },
	{ REGION_GFX2, 0, &dragrace_tile_layout2, 8, 2 },
	{ -1 } /* end of array */
};


static PALETTE_INIT( dragrace )
{
	palette_set_color(0, 0xFF, 0xFF, 0xFF);   /* 2 color tiles */
	palette_set_color(1, 0x00, 0x00, 0x00);
	palette_set_color(2, 0x00, 0x00, 0x00);
	palette_set_color(3, 0xFF, 0xFF, 0xFF);
	palette_set_color(4, 0x00, 0x00, 0x00);
	palette_set_color(5, 0x00, 0x00, 0x00);
	palette_set_color(6, 0xFF, 0xFF, 0xFF);
	palette_set_color(7, 0xFF, 0xFF, 0xFF);
	palette_set_color(8, 0xFF, 0xFF, 0xFF);   /* 4 color tiles */
	palette_set_color(9, 0xB0, 0xB0, 0xB0);
	palette_set_color(10,0x5F, 0x5F, 0x5F);
	palette_set_color(11,0x00, 0x00, 0x00);
	palette_set_color(12,0xFF, 0xFF, 0xFF);
	palette_set_color(13,0x5F, 0x5F, 0x5F);
	palette_set_color(14,0xB0, 0xB0, 0xB0);
	palette_set_color(15,0x00, 0x00, 0x00);
}


/************************************************************************/
/* dragrace Sound System Analog emulation                                  */
/************************************************************************/

const struct discrete_lfsr_desc dragrace_lfsr = {
	16,			/* Bit Length */
	0,			/* Reset Value */
	0,			/* Use Bit 0 as XOR input 0 */
	14,			/* Use Bit 14 as XOR input 1 */
	DISC_LFSR_XNOR,		/* Feedback stage1 is XNOR */
	DISC_LFSR_OR,		/* Feedback stage2 is just stage 1 output OR with external feed */
	DISC_LFSR_REPLACE,	/* Feedback stage3 replaces the shifted register contents */
	0x000001,		/* Everything is shifted into the first bit only */
	0,			/* Output is already inverted by XNOR */
	15			/* Output bit */
};

/* Nodes - Inputs */
#define DRAGRACE_SCREECH1_EN	NODE_01
#define DRAGRACE_SCREECH2_EN	NODE_02
#define DRAGRACE_LOTONE_EN	NODE_03
#define DRAGRACE_HITONE_EN	NODE_04
#define DRAGRACE_EXPLODE1_EN	NODE_05
#define DRAGRACE_EXPLODE2_EN	NODE_06
#define DRAGRACE_MOTOR1_DATA	NODE_07
#define DRAGRACE_MOTOR2_DATA	NODE_08
#define DRAGRACE_MOTOR1_EN	NODE_80
#define DRAGRACE_MOTOR2_EN	NODE_81
#define DRAGRACE_KLEXPL1_EN	NODE_82
#define DRAGRACE_KLEXPL2_EN	NODE_83
#define DRAGRACE_ATTRACT_EN	NODE_09
/* Nodes - Sounds */
#define DRAGRACE_NOISE		NODE_10
#define DRAGRACE_SCREECH1_SND	NODE_11
#define DRAGRACE_SCREECH2_SND	NODE_12
#define DRAGRACE_LOTONE_SND	NODE_13
#define DRAGRACE_HITONE_SND	NODE_14
#define DRAGRACE_TONE_SND	NODE_15
#define DRAGRACE_EXPLODE1_SND	NODE_16
#define DRAGRACE_EXPLODE2_SND	NODE_17
#define DRAGRACE_MOTOR1_SND	NODE_18
#define DRAGRACE_MOTOR2_SND	NODE_19

static DISCRETE_SOUND_START(dragrace_sound_interface)
	/************************************************/
	/* dragrace  Effects Relataive Gain Table       */
	/*                                              */
	/* Effect  V-ampIn  Gain ratio        Relative  */
	/* LoTone   3.8     10/32               593.8   */
	/* HiTone   3.8     10/32               593.8   */
	/* Screech  3.8     10/330               57.6   */
	/* Motor    3.8     10/32.67            581.6   */
	/* Explode  5.0     10/25              1000.0   */
	/************************************************/


	/************************************************/
	/* Input register mapping for dragrace          */
	/************************************************/
	/*                    NODE           ADDR  MASK    INIT */
	DISCRETE_INPUT(DRAGRACE_SCREECH1_EN, 0x00, 0x000f, 0.0)
	DISCRETE_INPUT(DRAGRACE_SCREECH2_EN, 0x01, 0x000f, 0.0)
	DISCRETE_INPUT(DRAGRACE_LOTONE_EN,   0x02, 0x000f, 0.0)
	DISCRETE_INPUT(DRAGRACE_HITONE_EN,   0x03, 0x000f, 0.0)
	DISCRETE_INPUT(DRAGRACE_EXPLODE1_EN, 0x04, 0x000f, 0.0)
	DISCRETE_INPUT(DRAGRACE_EXPLODE2_EN, 0x05, 0x000f, 0.0)
	DISCRETE_INPUT(DRAGRACE_MOTOR1_DATA, 0x06, 0x000f, 0.0)
	DISCRETE_INPUT(DRAGRACE_MOTOR2_DATA, 0x07, 0x000f, 0.0)
	DISCRETE_INPUT(DRAGRACE_MOTOR1_EN,   0x08, 0x000f, 0.0)
	DISCRETE_INPUT(DRAGRACE_MOTOR2_EN,   0x09, 0x000f, 0.0)
	DISCRETE_INPUT(DRAGRACE_KLEXPL1_EN,  0x0a, 0x000f, 0.0)
	DISCRETE_INPUT(DRAGRACE_KLEXPL2_EN,  0x0b, 0x000f, 0.0)
	DISCRETE_INPUT(DRAGRACE_ATTRACT_EN,  0x0c, 0x000f, 0.0)

	/************************************************/
	/* Motor sound circuit is based on a 556 VCO    */
	/* with the input frequency set by the MotorSND */
	/* latch (4 bit). This freqency is then used to */
	/* driver a modulo 12 counter, with div6, 4 & 3 */
	/* summed as the output of the circuit.         */
	/* VCO Output is Sq wave = 27-382Hz             */
	/*  F1 freq - (Div6)                            */
	/*  F2 freq = (Div4)                            */
	/*  F3 freq = (Div3) 33.3% duty, 33.3 deg phase */
	/* To generate the frequency we take the freq.  */
	/* diff. and /15 to get all the steps between   */
	/* 0 - 15.  Then add the low frequency and send */
	/* that value to a squarewave generator.        */
	/* Also as the frequency changes, it ramps due  */
	/* to a 2.2uf capacitor on the R-ladder.        */
	/* Note the VCO freq. is controlled by a 250k   */
	/* pot.  The freq. used here is for the pot set */
	/* to 125k.  The low freq is allways the same.  */
	/* This adjusts the high end.                   */
	/* 0k = 214Hz.   250k = 4416Hz                  */
	/* NOTE: freqs are ripped from Sprint for now.  */
	/************************************************/
	DISCRETE_RCFILTER(NODE_20, 1, DRAGRACE_MOTOR1_DATA, 119898, 2.2e-6)
	DISCRETE_ADJUSTMENT(NODE_21, 1, (214.0-27.0)/12/31, (4416.0-27.0)/12/31, DISC_LOGADJ, 7)
	DISCRETE_MULTIPLY(NODE_22, 1, NODE_20, NODE_21)

	DISCRETE_MULTADD(NODE_23, 1, NODE_22, 2, 27.0/6)	/* F1 = /12*2 = /6 */
	DISCRETE_SQUAREWAVE(NODE_24, 1, NODE_23, (581.6/3), 50.0, 0, 0)
	DISCRETE_RCFILTER(NODE_25, 1, NODE_24, 10000, 1e-7)

	DISCRETE_MULTADD(NODE_26, 1, NODE_22, 3, 27.0/4)	/* F2 = /12*3 = /4 */
	DISCRETE_SQUAREWAVE(NODE_27, 1, NODE_26, (581.6/3), 50.0, 0, 0)
	DISCRETE_RCFILTER(NODE_28, 1, NODE_27, 10000, 1e-7)

	DISCRETE_MULTADD(NODE_29, 1, NODE_22, 4, 27.0/3)	/* F3 = /12*4 = /3 */
	DISCRETE_SQUAREWAVE(NODE_30, 1, NODE_29, (581.6/3), 100.0/3, 0, 360.0/3)
	DISCRETE_RCFILTER(NODE_31, 1, NODE_30, 10000, 1e-7)

	DISCRETE_ADDER3(DRAGRACE_MOTOR1_SND, DRAGRACE_MOTOR1_EN, NODE_25, NODE_28, NODE_31)

	/************************************************/
	/* Car2 motor sound is basically the same as    */
	/* Car1.  But I shifted the frequencies up for  */
	/* it to sound different from car1.             */
	/************************************************/
	DISCRETE_RCFILTER(NODE_40, 1, DRAGRACE_MOTOR2_DATA, 119898, 2.2e-6)
	DISCRETE_ADJUSTMENT(NODE_41, 1, (214.0-27.0)/12/31, (4416.0-27.0)/12/31, DISC_LOGADJ, 8)
	DISCRETE_MULTIPLY(NODE_42, 1, NODE_40, NODE_41)

	DISCRETE_MULTADD(NODE_43, 1, NODE_42, 2, 27.0/6)	/* F1 = /12*2 = /6 */
	DISCRETE_SQUAREWAVE(NODE_44, 1, NODE_43, (581.6/3), 50.0, 0, 0)
	DISCRETE_RCFILTER(NODE_45, 1, NODE_44, 10000, 1e-7)

	DISCRETE_MULTADD(NODE_46, 1, NODE_42, 3, 27.0/4)	/* F2 = /12*3 = /4 */
	DISCRETE_SQUAREWAVE(NODE_47, 1, NODE_46, (581.6/3), 50.0, 0, 0)
	DISCRETE_RCFILTER(NODE_48, 1, NODE_47, 10000, 1e-7)

	DISCRETE_MULTADD(NODE_49, 1, NODE_42, 4, 27.0/3)	/* F3 = /12*4 = /3 */
	DISCRETE_SQUAREWAVE(NODE_50, 1, NODE_49, (581.6/3), 100.0/3, 0, 360.0/3)
	DISCRETE_RCFILTER(NODE_51, 1, NODE_50, 10000, 1e-7)

	DISCRETE_ADDER3(DRAGRACE_MOTOR2_SND, DRAGRACE_MOTOR2_EN, NODE_45, NODE_48, NODE_51)

	/************************************************/
	/* Explosion circuit is built around a noise    */
	/* generator built from 2 shift registers that  */
	/* are clocked by the 1V signal.                */
	/* 1V = HSYNC/2                                 */
	/*    = 15750/2                                 */
	/* Output is integrated to apply decay.         */
	/************************************************/
	DISCRETE_LFSR_NOISE(DRAGRACE_NOISE, 1, DRAGRACE_ATTRACT_EN, 15750.0/2, 1.0, 0, 0, &dragrace_lfsr)

	DISCRETE_RAMP(NODE_61, DRAGRACE_EXPLODE1_EN, DRAGRACE_EXPLODE1_EN, (1000.0-0.0)/1, 1000.0, 0.0, 1000.0)
	DISCRETE_MULTIPLY(NODE_62, 1, DRAGRACE_NOISE, NODE_61)
	DISCRETE_RCFILTER(DRAGRACE_EXPLODE1_SND, DRAGRACE_KLEXPL1_EN, NODE_62, 1500, 2.2e-7)

	DISCRETE_RAMP(NODE_66, DRAGRACE_EXPLODE2_EN, DRAGRACE_EXPLODE2_EN, (1000.0-0.0)/1, 1000.0, 0.0, 1000.0)
	DISCRETE_MULTIPLY(NODE_67, 1, DRAGRACE_NOISE, NODE_66)
	DISCRETE_RCFILTER(DRAGRACE_EXPLODE2_SND, DRAGRACE_KLEXPL2_EN, NODE_67, 1500, 2.2e-7)

	/************************************************/
	/* Skid circuits takes the noise output from    */
	/* the crash circuit and applies +ve feedback   */
	/* to cause oscillation. There is also an RC    */
	/* filter on the input to the feedback cct.     */
	/* RC is 1K & 10uF                              */
	/* Feedback cct is modelled by using the RC out */
	/* as the frequency input on a VCO,             */
	/* breadboarded freq range as:                  */
	/*  0 = 940Hz, 34% duty                         */
	/*  1 = 630Hz, 29% duty                         */
	/*  the duty variance is so small we ignore it  */
	/************************************************/
	DISCRETE_INVERT(NODE_70, DRAGRACE_NOISE)
	DISCRETE_MULTADD(NODE_71, 1, NODE_70, 940.0-630.0, ((940.0-630.0)/2)+630.0)
	DISCRETE_RCFILTER(NODE_72, 1, NODE_71, 1000, 1e-5)
	DISCRETE_SQUAREWAVE(NODE_73, 1, NODE_72, 407.8, 31.5, 0, 0.0)
	DISCRETE_ONOFF(DRAGRACE_SCREECH1_SND, DRAGRACE_SCREECH1_EN, NODE_73)
	DISCRETE_ONOFF(DRAGRACE_SCREECH2_SND, DRAGRACE_SCREECH2_EN, NODE_73)

	/************************************************/
	/* LoTone = 32V = 15750Hz/2/32                  */
	/* HiTone =  4V = 15750Hz/2/4                   */
	/************************************************/
	DISCRETE_SQUAREWFIX(DRAGRACE_LOTONE_SND, DRAGRACE_LOTONE_EN, 15750.0/2/32,  593.8, 50.0, 0, 0.0)
	DISCRETE_SQUAREWFIX(DRAGRACE_HITONE_SND, DRAGRACE_HITONE_EN, 15750.0/2/4,   593.8, 50.0, 0, 0.0)
	DISCRETE_ADDER2(DRAGRACE_TONE_SND, 1, DRAGRACE_LOTONE_SND, DRAGRACE_HITONE_SND)

	/************************************************/
	/* Combine all 5 sound sources.                 */
	/* Add some final gain to get to a good sound   */
	/* level.                                       */
	/************************************************/
	DISCRETE_ADDER4(NODE_90, DRAGRACE_ATTRACT_EN, DRAGRACE_TONE_SND, DRAGRACE_MOTOR1_SND, DRAGRACE_EXPLODE1_SND, DRAGRACE_SCREECH1_SND)
	DISCRETE_ADDER4(NODE_91, DRAGRACE_ATTRACT_EN, DRAGRACE_TONE_SND, DRAGRACE_MOTOR2_SND, DRAGRACE_EXPLODE2_SND, DRAGRACE_SCREECH2_SND)
	DISCRETE_GAIN(NODE_92, NODE_90, 65534.0/(593.8+581.6+1000.0+57.6))
	DISCRETE_GAIN(NODE_93, NODE_91, 65534.0/(593.8+581.6+1000.0+57.6))

	DISCRETE_OUTPUT(NODE_92, MIXER(100,MIXER_PAN_LEFT))
	DISCRETE_OUTPUT(NODE_93, MIXER(100,MIXER_PAN_RIGHT))
DISCRETE_SOUND_END


static MACHINE_DRIVER_START( dragrace )

	/* basic machine hardware */
	MDRV_CPU_ADD(M6800, 12096000 / 12)
	MDRV_CPU_MEMORY(dragrace_readmem, dragrace_writemem)
	MDRV_CPU_VBLANK_INT(irq0_line_hold, 4)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION((int) ((22. * 1000000) / (262. * 60) + 0.5))

	MDRV_MACHINE_INIT(dragrace)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(256, 240)
	MDRV_VISIBLE_AREA(0, 255, 0, 239)
	MDRV_GFXDECODE(dragrace_gfx_decode_info)
	MDRV_PALETTE_LENGTH(16)
	MDRV_PALETTE_INIT(dragrace)
	MDRV_VIDEO_START(dragrace)
	MDRV_VIDEO_UPDATE(dragrace)

	/* sound hardware */
	MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
	MDRV_SOUND_ADD_TAG("discrete", DISCRETE, dragrace_sound_interface)
MACHINE_DRIVER_END


ROM_START( dragrace )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "8513.c1", 0x1000, 0x0800, CRC(543bbb30) SHA1(646a41d1124c8365f07a93de38af007895d7d263) )
	ROM_LOAD( "8514.a1", 0x1800, 0x0800, CRC(ad218690) SHA1(08ba5f4fa4c75d8dad1a7162888d44b3349cbbe4) )
	ROM_RELOAD(          0xF800, 0x0800 )

	ROM_REGION( 0x800, REGION_GFX1, ROMREGION_DISPOSE )   /* 2 color tiles */
	ROM_LOAD( "8519dr.j0", 0x000, 0x200, CRC(aa221ba0) SHA1(450acbf349d77a790a25f3e303c31b38cc426a38) )
	ROM_LOAD( "8521dr.k0", 0x200, 0x200, CRC(0cb33f12) SHA1(d50cb55391aec03e064eecad1624d50d4c30ccab) )
	ROM_LOAD( "8520dr.r0", 0x400, 0x200, CRC(ee1ae6a7) SHA1(83491095260c8b7c616ff17ec1e888d05620f166) )

	ROM_REGION( 0x800, REGION_GFX2, ROMREGION_DISPOSE )   /* 4 color tiles */
	ROM_LOAD( "8515dr.e0", 0x000, 0x200, CRC(9510a59e) SHA1(aea0782b919279efe55a07007bd55a16f7f59239) )
	ROM_LOAD( "8517dr.h0", 0x200, 0x200, CRC(8b5bff1f) SHA1(fdcd719c66bff7c4b9f3d56d1e635259dd8add61) )
	ROM_LOAD( "8516dr.l0", 0x400, 0x200, CRC(d1e74af1) SHA1(f55a3bfd7d152ac9af128697f55c9a0c417779f5) )
	ROM_LOAD( "8518dr.n0", 0x600, 0x200, CRC(b1369028) SHA1(598a8779982d532c9f34345e793a79fcb29cac62) )
ROM_END


GAME( 1977, dragrace, 0, dragrace, dragrace, 0, 0, "Atari", "Drag Race" )
