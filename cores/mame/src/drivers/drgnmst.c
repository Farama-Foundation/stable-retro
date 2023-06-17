/*

Dragon Master (c)1994 Unico

the hardware seems to fall somewhere between the
hardware playmark commonly used and the hardware
unico used for zero point etc.

*/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "cpu/pic16c5x/pic16c5x.h"


static data16_t drgnmst_snd_command;
static data16_t drgnmst_snd_flag;
static data8_t drgnmst_oki_control;
static data8_t drgnmst_oki_command;
static data8_t pic16c5x_port0;
static data8_t drgnmst_oki0_bank;
static data8_t drgnmst_oki1_bank;

data16_t *drgnmst_vidregs;
data16_t *drgnmst_rowscrollram;
data16_t *drgnmst_fg_videoram;
data16_t *drgnmst_bg_videoram;
data16_t *drgnmst_md_videoram;
data16_t *drgnmst_vidregs2;


WRITE16_HANDLER( drgnmst_fg_videoram_w );
WRITE16_HANDLER( drgnmst_bg_videoram_w );
WRITE16_HANDLER( drgnmst_md_videoram_w );
VIDEO_START(drgnmst);
VIDEO_UPDATE(drgnmst);
VIDEO_UPDATE(mastfury);




static WRITE16_HANDLER( drgnmst_snd_command_w )
{
	if (ACCESSING_LSB) {
		drgnmst_snd_command = (data & 0xff);
		cpu_yield();
	}
}

static WRITE16_HANDLER( drgnmst_snd_flag_w )
{
	/* Enables the following 68K write operation to latch through to the PIC */
	if (ACCESSING_LSB)
		drgnmst_snd_flag = 1;
}


static READ_HANDLER( pic16c5x_port0_r )
{
	return pic16c5x_port0;
}

static READ_HANDLER( drgnmst_snd_command_r )
{
	int data = 0;

	switch (drgnmst_oki_control & 0x1f)
	{
		case 0x12:	data = (OKIM6295_status_1_r(0) & 0x0f); break;
		case 0x16:	data = (OKIM6295_status_0_r(0) & 0x0f); break;
		case 0x0b:
		case 0x0f:	data = drgnmst_snd_command; break;
		default:	break;
	}

	return data;
}

static READ_HANDLER( drgnmst_snd_flag_r )
{
	if (drgnmst_snd_flag) {
		drgnmst_snd_flag = 0;
		return 0x40;
	}

	return 0x00;
}

static WRITE_HANDLER( drgnmst_pcm_banksel_w )
{
	/*	This is a 4 bit port.
		Each pair of bits is used in part of the OKI PCM ROM bank selectors.
		See the Port 2 write handler below (drgnmst_snd_control_w) for details.
	*/

	pic16c5x_port0 = data;
}

static WRITE_HANDLER( drgnmst_oki_w )
{
	drgnmst_oki_command = data;
}

static WRITE_HANDLER( drgnmst_snd_control_w )
{
	/*	This port controls communications to and from the 68K, both OKI
		devices, and part of the OKI PCM ROM bank selection.

		bit legend
		7w  ROM bank select for OKI-1, bit 2. Bank bits 1 & 0 are on Port 0
		6r  Flag from 68K to notify the PIC that a command is coming
		5w  ROM bank select for OKI-0, bit 2. Bank bits 1 & 0 are on Port 0
		4w  Set Port 1 to read sound to play command from 68K. (active low)
		3w  OKI enable comms? (active low)
		2w  OKI chip select? (0=OKI-1, 1=OKI-0)
		1w  Latch write data to OKI? (active low)
		0w  Activate read signal to OKI? (active low)

		The PCM ROM bank selects are 3 bits wide.
		2 bits for each OKI BANK selects are on Port 0, and the third most
		significant bit is here. The MSb is written here immediately after
		writing to Port 0 so we handle the bank switching here.
		The PIC16C55 only supports bank selections for:
		 OKI0 from 1 to 5  (Each bank selection switches the $20000-3ffff area)
		 OKI1 from 0 to 7  (Each bank selection switches the entire $40000 area)
		The OKI0 banks are pre-configured below in the driver init.
	*/

	int oki_new_bank;
	drgnmst_oki_control = data;


	oki_new_bank = ((pic16c5x_port0 & 0xc) >> 2) | ((drgnmst_oki_control & 0x80) >> 5);
	if (oki_new_bank != drgnmst_oki0_bank) {
		drgnmst_oki0_bank = oki_new_bank;
		if (drgnmst_oki0_bank) oki_new_bank--;
		OKIM6295_set_bank_base(0, (oki_new_bank * 0x40000));
	}
	oki_new_bank = ((pic16c5x_port0 & 0x3) >> 0) | ((drgnmst_oki_control & 0x20) >> 3);
	if (oki_new_bank != drgnmst_oki1_bank) {
		drgnmst_oki1_bank = oki_new_bank;
		OKIM6295_set_bank_base(1, (oki_new_bank * 0x40000));
	}

	switch(drgnmst_oki_control & 0x1f)
	{
		case 0x11:
/*					log_cb(RETRO_LOG_DEBUG, LOGPRE "Writing %02x to OKI1",drgnmst_oki_command);*/
/*					log_cb(RETRO_LOG_DEBUG, LOGPRE ", PortC=%02x, Code=%02x, Bank0=%01x, Bank1=%01x\n",drgnmst_oki_control,drgnmst_snd_command,drgnmst_oki0_bank,drgnmst_oki1_bank);*/
					OKIM6295_data_1_w(0, drgnmst_oki_command);
					break;
		case 0x15:
/*					log_cb(RETRO_LOG_DEBUG, LOGPRE "Writing %02x to OKI0",drgnmst_oki_command);*/
/*					log_cb(RETRO_LOG_DEBUG, LOGPRE ", PortC=%02x, Code=%02x, Bank0=%01x, Bank1=%01x\n",drgnmst_oki_control,drgnmst_snd_command,drgnmst_oki0_bank,drgnmst_oki1_bank);*/
					OKIM6295_data_0_w(0, drgnmst_oki_command);
					break;
		default:	break;
	}
}


static READ_HANDLER( PIC16C5X_T0_clk_r )
{
	return 0;
}



static MEMORY_READ16_START( drgnmst_readmem )
	{ 0x000000, 0x0fffff, MRA16_ROM },

	{ 0x800000, 0x800001, input_port_0_word_r },
	{ 0x800018, 0x800019, input_port_1_word_r },
	{ 0x80001a, 0x80001b, input_port_2_word_r },
	{ 0x80001c, 0x80001d, input_port_3_word_r },
	{ 0x800176, 0x800177, input_port_4_word_r },

	{ 0x900000, 0x903fff, MRA16_RAM },
	{ 0x904000, 0x907fff, MRA16_RAM },
	{ 0x908000, 0x90bfff, MRA16_RAM },
	{ 0x90c000, 0x90ffff, MRA16_RAM },

	{ 0x920000, 0x923fff, MRA16_RAM },
	{ 0x930000, 0x9307ff, MRA16_RAM },

	{ 0xff0000, 0xffffff, MRA16_RAM },
MEMORY_END

static MEMORY_WRITE16_START( drgnmst_writemem )
	{ 0x000000, 0x0fffff, MWA16_ROM },
	{ 0x800030, 0x800031, MWA16_RAM },

	{ 0x800100, 0x800121, MWA16_RAM, &drgnmst_vidregs },
	{ 0x800154, 0x800155, MWA16_RAM, &drgnmst_vidregs2 }, /* seems to be priority control */

	{ 0x800180, 0x800181, drgnmst_snd_command_w },
	{ 0x800188, 0x800189, drgnmst_snd_flag_w },

	{ 0x8001e0, 0x8001e1, MWA16_NOP },

	{ 0x900000, 0x903fff, paletteram16_xxxxRRRRGGGGBBBB_word_w, &paletteram16 },
	{ 0x904000, 0x907fff, drgnmst_md_videoram_w, &drgnmst_md_videoram  },
	{ 0x908000, 0x90bfff, drgnmst_bg_videoram_w, &drgnmst_bg_videoram },
	{ 0x90c000, 0x90ffff, drgnmst_fg_videoram_w, &drgnmst_fg_videoram },

	{ 0x920000, 0x923fff, MWA16_RAM, &drgnmst_rowscrollram }, /* rowscroll ram*/
	{ 0x930000, 0x9307ff, MWA16_RAM, &spriteram16, &spriteram_size	},	/* Sprites*/

	{ 0xff0000, 0xffffff, MWA16_RAM },
MEMORY_END

/* Masters Fury no PIC YM2151 sound */
static MEMORY_READ16_START( mastfury_readmem )
	{ 0x000000, 0x0fffff, MRA16_ROM },

	{ 0x800000, 0x800001, input_port_0_word_r },
	{ 0x800018, 0x800019, input_port_1_word_r },
	{ 0x80001a, 0x80001b, input_port_2_word_r },
	{ 0x80001c, 0x80001d, input_port_3_word_r },
	{ 0x800176, 0x800177, input_port_4_word_r },
	
	{ 0x800182, 0x800183, YM2151_status_port_0_lsb_r },
	{ 0x800188, 0x800189, OKIM6295_status_0_lsb_r },

	{ 0x900000, 0x903fff, MRA16_RAM },
	{ 0x904000, 0x907fff, MRA16_RAM },
	{ 0x908000, 0x90bfff, MRA16_RAM },
	{ 0x90c000, 0x90ffff, MRA16_RAM },

	{ 0x920000, 0x923fff, MRA16_RAM },
	{ 0x930000, 0x9307ff, MRA16_RAM },

	{ 0xff0000, 0xffffff, MRA16_RAM },
MEMORY_END

static MEMORY_WRITE16_START( mastfury_writemem )
	{ 0x000000, 0x0fffff, MWA16_ROM },
	{ 0x800030, 0x800031, MWA16_RAM },

	{ 0x800100, 0x800121, MWA16_RAM, &drgnmst_vidregs },
	{ 0x800154, 0x800155, MWA16_RAM, &drgnmst_vidregs2 }, /* seems to be priority control */

    { 0x800180, 0x800181, YM2151_register_port_0_lsb_w },
	{ 0x800182, 0x800183, YM2151_data_port_0_lsb_w },
	{ 0x800188, 0x800189, OKIM6295_data_0_lsb_w },
	
	{ 0x8001e0, 0x8001e1, MWA16_NOP },

	{ 0x900000, 0x903fff, paletteram16_xxxxRRRRGGGGBBBB_word_w, &paletteram16 },
	{ 0x904000, 0x907fff, drgnmst_md_videoram_w, &drgnmst_md_videoram  },
	{ 0x908000, 0x90bfff, drgnmst_bg_videoram_w, &drgnmst_bg_videoram },
	{ 0x90c000, 0x90ffff, drgnmst_fg_videoram_w, &drgnmst_fg_videoram },

	{ 0x920000, 0x923fff, MWA16_RAM, &drgnmst_rowscrollram }, /* rowscroll ram */
	{ 0x930000, 0x9307ff, MWA16_RAM, &spriteram16, &spriteram_size	},	/* Sprites */

	{ 0xff0000, 0xffffff, MWA16_RAM },
MEMORY_END

static MEMORY_READ_START( drgnmst_sound_readmem )
	{ PIC16C55_MEMORY_READ },
		/* $000 - 01F  Internal memory mapped registers */
		/* $000 - 1FF  Program ROM for PIC16C55. Note: code is 12bits wide */
		/*             View the ROM at $1000 in the debugger memory windows */
MEMORY_END

static MEMORY_WRITE_START( drgnmst_sound_writemem )
	{ PIC16C55_MEMORY_WRITE },
MEMORY_END

static PORT_READ_START( drgnmst_sound_readport )
	{ 0x00, 0x00, pic16c5x_port0_r },		/* 4 bit port */
	{ 0x01, 0x01, drgnmst_snd_command_r },
	{ 0x02, 0x02, drgnmst_snd_flag_r },
	{ PIC16C5x_T0, PIC16C5x_T0, PIC16C5X_T0_clk_r },
PORT_END

static PORT_WRITE_START( drgnmst_sound_writeport )
	{ 0x00, 0x00, drgnmst_pcm_banksel_w },	/* 4 bit port */
	{ 0x01, 0x01, drgnmst_oki_w },
	{ 0x02, 0x02, drgnmst_snd_control_w },
PORT_END



INPUT_PORTS_START( drgnmst )
	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER1 )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER1 )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER2 )

	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_DIPNAME( 0x0700, 0x0700, DEF_STR( Coinage ) )
/*	PORT_DIPSETTING(      0x0300, DEF_STR( Off ) ) */
	PORT_DIPSETTING(      0x0000, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(      0x0100, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0200, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0700, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0600, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x0500, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x0400, DEF_STR( 1C_4C ) )
	PORT_DIPNAME( 0x0800, 0x0000, "Continue" )
	PORT_DIPSETTING(      0x0800, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x1000, 0x0000, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(      0x1000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x2000, 0x2000, "Two credits to start" )
	PORT_DIPSETTING(      0x2000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x4000, 0x4000, DEF_STR ( Free_Play ) )
	PORT_DIPSETTING(      0x4000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x8000, 0x8000, "Game Pause" )
	PORT_DIPSETTING(      0x8000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )

	PORT_START
	PORT_DIPNAME( 0x0700, 0x0400, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(      0x0700, "Easiest" )
	PORT_DIPSETTING(      0x0600, "Easier" )
	PORT_DIPSETTING(      0x0500, "Easy" )
	PORT_DIPSETTING(      0x0400, "Normal" )
	PORT_DIPSETTING(      0x0300, "Medium" )
	PORT_DIPSETTING(      0x0200, "Hard" )
	PORT_DIPSETTING(      0x0100, "Harder" )
	PORT_DIPSETTING(      0x0000, "Hardest" )
	PORT_DIPNAME( 0x0800, 0x0800, "Language" )
	PORT_DIPSETTING(      0x0800, "English" )
	PORT_DIPSETTING(      0x0000, "Korea" )
	PORT_DIPNAME( 0x1000, 0x1000, "Game Time" )
	PORT_DIPSETTING(      0x1000, "Normal" )
	PORT_DIPSETTING(      0x0000, "Short" )
	PORT_DIPNAME( 0x2000, 0x2000, "Stage Skip" )
	PORT_DIPSETTING(      0x2000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x4000, 0x4000, "Spit Color" )
	PORT_DIPSETTING(      0x4000, "Grey" )
	PORT_DIPSETTING(      0x0000, "Red" )
	PORT_SERVICE( 0x8000, IP_ACTIVE_LOW )

	PORT_START
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_BUTTON5 | IPF_PLAYER1 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_BUTTON6 | IPF_PLAYER1 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_BUTTON7 | IPF_PLAYER1 )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON5 | IPF_PLAYER2 )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON6 | IPF_PLAYER2 )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_BUTTON7 | IPF_PLAYER2 )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END


static struct GfxLayout drgnmst_char8x8_layout =
{
	8,8,
	RGN_FRAC(1,1),
	4,
	{ 24,8,16, 0 },
	{ 0,1,2,3,4,5,6,7 },
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32 },
	8*32
};


static struct GfxLayout drgnmst_char16x16_layout =
{
	16,16,
	RGN_FRAC(1,2),
	4,
	{ 24, 8,16,0 },
	{ RGN_FRAC(1,2)+0,RGN_FRAC(1,2)+1,RGN_FRAC(1,2)+2,RGN_FRAC(1,2)+3,RGN_FRAC(1,2)+4,RGN_FRAC(1,2)+5,RGN_FRAC(1,2)+6,RGN_FRAC(1,2)+7,
		0,1,2,3,4,5,6,7 },
	{ 0*32,1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32,
	  8*32,9*32,10*32,11*32,12*32,13*32,14*32,15*32 },
	16*32
};



static struct GfxLayout drgnmst_char32x32_layout =
{
	32,32,
	RGN_FRAC(1,2),
	4,
	{ 24,8, 16,0 },
	{ 	RGN_FRAC(1,2)+0,RGN_FRAC(1,2)+1,RGN_FRAC(1,2)+2,RGN_FRAC(1,2)+3,RGN_FRAC(1,2)+4,RGN_FRAC(1,2)+5,RGN_FRAC(1,2)+6,RGN_FRAC(1,2)+7,
		0,1,2,3,4,5,6,7,
	    RGN_FRAC(1,2)+32,RGN_FRAC(1,2)+33,RGN_FRAC(1,2)+34,RGN_FRAC(1,2)+35,RGN_FRAC(1,2)+36,RGN_FRAC(1,2)+37,RGN_FRAC(1,2)+38,RGN_FRAC(1,2)+39,
		32,33,34,35,36,37,38,39 },

	{	 0*64, 1*64, 2*64, 3*64, 4*64, 5*64, 6*64, 7*64,
		 8*64, 9*64,10*64,11*64,12*64,13*64,14*64,15*64,
	 	16*64,17*64,18*64,19*64,20*64,21*64,22*64,23*64,
	 	24*64,25*64,26*64,27*64,28*64,29*64,30*64,31*64 },
	32*64
};


static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &drgnmst_char16x16_layout,   0,      0x200  }, /* sprite tiles */
	{ REGION_GFX2, 0, &drgnmst_char8x8_layout,     0x200,  0x200  }, /* fg tiles */
	{ REGION_GFX2, 0, &drgnmst_char16x16_layout,   0x0400, 0x200  }, /* md tiles */
	{ REGION_GFX2, 0, &drgnmst_char32x32_layout,   0x0600, 0x200  }, /* bg tiles */
	{ -1 } /* end of array */
};

static struct YM2151interface mastfury_ym2151_intf =
{
	1,
	3000000,
	{ YM3012_VOL(10,MIXER_PAN_LEFT,10,MIXER_PAN_RIGHT) },
	{ 0 }
};

static struct OKIM6295interface mastfury_okim6295_interface =
{
	1,										/* 1 chip */
	{ 32000000/32/132 },   /* Confirmed */
	{ REGION_SOUND1 },		/* memory region */
	{ 90 }
};

static struct OKIM6295interface dual_okim6295_interface =
{
	2,										/* 2 chips */
	{ 32000000/32/132, 32000000/32/132 },   /* Confirmed */
	{ REGION_SOUND1, REGION_SOUND2 },		/* memory region */
	{ 50, 50 }
};


static MACHINE_DRIVER_START( drgnmst )
	MDRV_CPU_ADD(M68000, 12000000) /* Confirmed */
	MDRV_CPU_MEMORY(drgnmst_readmem,drgnmst_writemem)
	MDRV_CPU_VBLANK_INT(irq2_line_hold,1)

	MDRV_CPU_ADD(PIC16C55, ((32000000/8)/PIC16C5x_CLOCK_DIVIDER))	/* Confirmed */
	MDRV_CPU_MEMORY(drgnmst_sound_readmem,drgnmst_sound_writemem)
	MDRV_CPU_PORTS(drgnmst_sound_readport,drgnmst_sound_writeport)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_GFXDECODE(gfxdecodeinfo)

	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(64*8, 32*8)
	MDRV_VISIBLE_AREA(8*8, 56*8-1, 2*8, 30*8-1)
	MDRV_PALETTE_LENGTH(0x2000)

	MDRV_VIDEO_START(drgnmst)
	MDRV_VIDEO_UPDATE(drgnmst)

	/* sound hardware */
	MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
	MDRV_SOUND_ADD(OKIM6295, dual_okim6295_interface)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( mastfury )
	MDRV_CPU_ADD(M68000, 12000000) /* Confirmed */
	MDRV_CPU_MEMORY(mastfury_readmem,mastfury_writemem)
	MDRV_CPU_VBLANK_INT(irq2_line_hold,1)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_GFXDECODE(gfxdecodeinfo)

	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(64*8, 32*8)
	MDRV_VISIBLE_AREA(8*8, 56*8-1, 2*8, 30*8-1)
	MDRV_PALETTE_LENGTH(0x2000)

	MDRV_VIDEO_START(drgnmst)
	MDRV_VIDEO_UPDATE(mastfury)

	/* sound hardware */
	MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
	MDRV_SOUND_ADD( YM2151,	 mastfury_ym2151_intf )
	MDRV_SOUND_ADD( OKIM6295, mastfury_okim6295_interface )
MACHINE_DRIVER_END

ROM_START( drgnmst )
	ROM_REGION( 0x100000, REGION_CPU1, 0 ) /* 68000 Code */
	ROM_LOAD16_BYTE( "dm1000e", 0x00000, 0x80000, CRC(29467dac) SHA1(42ca42340ffd9b04be23853ca4e936d0528a66ee) )
	ROM_LOAD16_BYTE( "dm1000o", 0x00001, 0x80000, CRC(ba48e9cf) SHA1(1107f927424107918bb10ff23f40c50579b23836) )

	ROM_REGION( 0x4000, REGION_CPU2, 0 ) /* PIC16C55 Code */
/*	ROM_LOAD( "pic16c55", PIC16C55_PGM_OFFSET, 0x400, CRC(531c9f8d) SHA1(8ec180b0566f2ce1e08f0347e5ad402c73b44049) )*/
	/* ROM will be copied here by the init code from the USER1 region */

	ROM_REGION( 0x1000, REGION_USER1, ROMREGION_DISPOSE )
	ROM_LOAD( "pic16c55.hex", 0x000, 0x0b7b, CRC(f17011e7) SHA1(8f3bd94ffb528f661eed77d89e5b772442d2f5a6) )

	ROM_REGION( 0x140000, REGION_SOUND1, 0 ) /* OKI-0 Samples */
	ROM_LOAD( "dm1001", 0x00000, 0x100000, CRC(63566f7f) SHA1(0fe6cb67a5d99cd54e46e9889ea121097756b9ef) )

	ROM_REGION( 0x200000, REGION_SOUND2, 0 ) /* OKI-1 Samples */
	ROM_LOAD( "dm1002", 0x00000, 0x200000, CRC(0f1a874e) SHA1(8efc39f8ff7e6e7138b19959bd083b9df002acca) )

	ROM_REGION( 0x800000, REGION_GFX1, 0 ) /* Sprites (16x16x4) */
	ROM_LOAD16_BYTE( "dm1003", 0x000000, 0x080000, CRC(0ca10e81) SHA1(abebd8437764110278c8b7e583d846db27e205ec) )
	ROM_CONTINUE(0x400000, 0x080000)
	ROM_CONTINUE(0x100000, 0x080000)
	ROM_CONTINUE(0x500000, 0x080000)
	ROM_LOAD16_BYTE( "dm1005", 0x000001, 0x080000, CRC(4c2b1db5) SHA1(35d799cd13540e2aca1d1164291fe4c9938ed0ce) )
	ROM_CONTINUE(0x400001, 0x080000)
	ROM_CONTINUE(0x100001, 0x080000)
	ROM_CONTINUE(0x500001, 0x080000)
	ROM_LOAD16_BYTE( "dm1004", 0x200000, 0x040000, CRC(1a9ac249) SHA1(c15c7399dcb24dcab05887e3711e5b31bb7f31e8) )
	ROM_CONTINUE(0x600000, 0x040000)
	ROM_LOAD16_BYTE( "dm1006", 0x200001, 0x040000, CRC(c46da6fc) SHA1(f2256f02c833bc1074681729bd2b95fa6f3350cf) )
	ROM_CONTINUE(0x600001, 0x040000)

	ROM_REGION( 0x200000, REGION_GFX2, 0 ) /* BG Tiles (8x8x4, 16x16x4 and 32x32x4) */
	ROM_LOAD16_BYTE( "dm1007", 0x000001, 0x100000, CRC(d5ad81c4) SHA1(03df467b218682a02245a6e8f500ab83de382448) )
	ROM_LOAD16_BYTE( "dm1008", 0x000000, 0x100000, CRC(b8572be3) SHA1(29aab76821e0a56033cf06b0a1890b11804da8d8) )
ROM_END

ROM_START( mastfury )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )        /* 68000 Code */
	ROM_LOAD16_BYTE( "master012.4m", 0x000000, 0x080000, CRC(020a3c50) SHA1(d6762b66f06fe91f3bff8cdcbff42c247df64671) )
	ROM_LOAD16_BYTE( "master013.4m", 0x000001, 0x080000, CRC(1e7dd287) SHA1(67764aa054731a0548f6c7d3b898597792d96eec) )

	ROM_REGION( 0x40000, REGION_SOUND1, 0 ) /* Samples */
	ROM_LOAD( "dmast96_voi1x", 0x00000, 0x40000, CRC(fc5161a1) SHA1(999e73e36df317aabefebf94444690f439d64559) )

	ROM_REGION( 0x800000, REGION_GFX1, 0 ) /* Sprites (16x16x4) */
	/* Some versions of the game use 2x32MBit ROMs instead.
	This set comes from a PCB marked "Dragon Master 96" but the PCB was missing program ROMs, 
	was Dragon Master 96 an alt title for the Chinese market? */
	ROM_LOAD16_BYTE( "dmast96_mvo1x", 0x000000, 0x080000, CRC(9a597497) SHA1(4f63e17629a00fa505e2165f7fa46f0c5ef2bc60) )
	ROM_CONTINUE(0x400000, 0x080000)
	ROM_CONTINUE(0x100000, 0x080000)
	ROM_CONTINUE(0x500000, 0x080000)
	ROM_LOAD16_BYTE( "dmast96_mvo3x", 0x000001, 0x080000, CRC(be01b829) SHA1(ab9858aadb0bba8415c674e27f9ea905de27871c) )
	ROM_CONTINUE(0x400001, 0x080000)
	ROM_CONTINUE(0x100001, 0x080000)
	ROM_CONTINUE(0x500001, 0x080000)
	ROM_LOAD16_BYTE( "dmast96_mvo2x", 0x200000, 0x080000, CRC(3eab296c) SHA1(d2add71e01aa6bd1b6539e72ed373bb71f65c437) )
	ROM_CONTINUE(0x600000, 0x080000)
	ROM_LOAD16_BYTE( "dmast96_mvo4x", 0x200001, 0x080000, CRC(d870b6ce) SHA1(e81c24eeaa5b857910436bfb6cac2b9fa05839e8) )
	ROM_CONTINUE(0x600001, 0x080000)

	ROM_REGION( 0x400000, REGION_GFX2, 0 ) /* BG Tiles (8x8x4, 16x16x4 and 32x32x4) */
	ROM_LOAD16_BYTE( "mf0016-3", 0x000000, 0x200000, CRC(0946bc61) SHA1(8b10c7f76daf21afb2aa6961100d83b1f6ca89bb) )
	ROM_LOAD16_BYTE( "mf0016-4", 0x000001, 0x200000, CRC(8f5b7c82) SHA1(5947c015c8a13539a3125c7ffe07cca0691b4348) )
ROM_END


static UINT8 drgnmst_asciitohex(UINT8 data)
{
	/* Convert ASCII data to HEX */

	if ((data >= 0x30) && (data < 0x3a)) data -= 0x30;
	data &= 0xdf;			/* remove case sensitivity */
	if ((data >= 0x41) && (data < 0x5b)) data -= 0x37;

	return data;
}


static DRIVER_INIT( drgnmst )
{
	data8_t *drgnmst_PICROM_HEX = memory_region(REGION_USER1);
	data8_t *drgnmst_PICROM = memory_region(REGION_CPU2);
	data8_t *drgnmst_PCM = memory_region(REGION_SOUND1);
	INT32   offs, data;
	UINT16  src_pos = 0;
	UINT16  dst_pos = 0;
	UINT8   data_hi, data_lo;


	drgnmst_snd_flag = 0;

	/* Configure the OKI-0 PCM data into a MAME friendly bank format */
	/* $00000-1ffff is the same through all banks */
	/* $20000-3ffff in each bank is actually the switched area */

	for (offs = 0x1ffff; offs >= 0; offs--)
	{
		drgnmst_PCM[0x120000 + offs] = drgnmst_PCM[0xa0000 + offs];
		drgnmst_PCM[0x100000 + offs] = drgnmst_PCM[0x00000 + offs];
		drgnmst_PCM[0x0e0000 + offs] = drgnmst_PCM[0x80000 + offs];
		drgnmst_PCM[0x0c0000 + offs] = drgnmst_PCM[0x00000 + offs];
		drgnmst_PCM[0x0a0000 + offs] = drgnmst_PCM[0x60000 + offs];
		drgnmst_PCM[0x080000 + offs] = drgnmst_PCM[0x00000 + offs];
		drgnmst_PCM[0x060000 + offs] = drgnmst_PCM[0x40000 + offs];
		drgnmst_PCM[0x040000 + offs] = drgnmst_PCM[0x00000 + offs];
	}

	/**** Convert the PIC16C55 ASCII HEX dump to pure HEX ****/
	do
	{
		if ((drgnmst_PICROM_HEX[src_pos + 0] == ':') &&
			(drgnmst_PICROM_HEX[src_pos + 1] == '1') &&
			(drgnmst_PICROM_HEX[src_pos + 2] == '0'))
			{
			src_pos += 9;

			for (offs = 0; offs < 32; offs += 2)
			{
				data_hi = drgnmst_asciitohex((drgnmst_PICROM_HEX[src_pos + offs + 0]));
				data_lo = drgnmst_asciitohex((drgnmst_PICROM_HEX[src_pos + offs + 1]));

				if ((data_hi <= 0x0f) && (data_lo <= 0x0f)) {
					data = (data_hi << 4) | (data_lo << 0);
					drgnmst_PICROM[PIC16C55_PGM_OFFSET + dst_pos] = data;
					dst_pos += 1;
				}
			}
			src_pos += 32;
		}

		/* Get the PIC16C55 Config register data */

		if ((drgnmst_PICROM_HEX[src_pos + 0] == ':') &&
			(drgnmst_PICROM_HEX[src_pos + 1] == '0') &&
			(drgnmst_PICROM_HEX[src_pos + 2] == '2') &&
			(drgnmst_PICROM_HEX[src_pos + 3] == '1'))
			{
			src_pos += 9;

			data_hi = drgnmst_asciitohex((drgnmst_PICROM_HEX[src_pos + 0]));
			data_lo = drgnmst_asciitohex((drgnmst_PICROM_HEX[src_pos + 1]));
			data =  (data_hi <<  4) | (data_lo << 0);
			data_hi = drgnmst_asciitohex((drgnmst_PICROM_HEX[src_pos + 2]));
			data_lo = drgnmst_asciitohex((drgnmst_PICROM_HEX[src_pos + 3]));
			data |= (data_hi << 12) | (data_lo << 8);

			pic16c5x_config(data);
			src_pos = 0x7fff;		/* Force Exit */
		}
		src_pos += 1;
	} while (src_pos < 0x0b7b);		/* 0x0b7b is the size of the HEX rom loaded */
}


GAME( 1994, drgnmst,  0, drgnmst,  drgnmst, drgnmst, ROT0, "Unico", "Dragon Master" )
GAME( 1996, mastfury, 0, mastfury, drgnmst, 0,       ROT0, "Unico", "Master's Fury" ) 


