/* Driver Info

Action Hollywood (c)1995 TCH / Proyesel
Kick Goal (c)1995 TCH / Proyesel
Top Driving (c)1995 Proyesel

prelim driver by David Haywood


todo:

Sound - Not possible without PIC dump?

*/

/* Notes

68k interrupts
lev 1 : 0x64 : 0000 0000 - x
lev 2 : 0x68 : 0000 0000 - x
lev 3 : 0x6c : 0000 0000 - x
lev 4 : 0x70 : 0000 0000 - x
lev 5 : 0x74 : 0000 0000 - x
lev 6 : 0x78 : 0000 0510 - vblank?
lev 7 : 0x7c : 0000 0000 - x

*/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "machine/eeprom.h"
#include "sound/adpcm.h"



data16_t *kickgoal_fgram, *kickgoal_bgram, *kickgoal_bg2ram, *kickgoal_scrram;
data16_t *topdrive_fgram, *topdrive_bgram, *topdrive_bg2ram, *topdrive_scrram;


WRITE16_HANDLER( kickgoal_fgram_w  );
WRITE16_HANDLER( kickgoal_bgram_w  );
WRITE16_HANDLER( kickgoal_bg2ram_w );

WRITE16_HANDLER( topdrive_fgram_w  );
WRITE16_HANDLER( topdrive_bgram_w  );
WRITE16_HANDLER( topdrive_bg2ram_w );

VIDEO_START( actionhw );
VIDEO_UPDATE( actionhw );
VIDEO_START( kickgoal );
VIDEO_UPDATE( kickgoal );
VIDEO_START( topdrive );
VIDEO_UPDATE( topdrive );

#define oki_time_base 0x08
static int snd_new, snd_sam[4];


static struct EEPROM_interface eeprom_interface =
{
	6,				/* address bits */
	16,				/* data bits */
	"*110",			/*  read command */
	"*101",			/* write command */
	0,				/* erase command */
	"*10000xxxx",	/* lock command */
	"*10011xxxx"	/* unlock command */
};

static NVRAM_HANDLER( kickgoal )
{
	if (read_or_write) EEPROM_save(file);
	else
	{
		EEPROM_init(&eeprom_interface);
		if (file) EEPROM_load(file);
	}
}


static READ16_HANDLER( kickgoal_eeprom_r )
{
	return EEPROM_read_bit();
}


static WRITE16_HANDLER( kickgoal_eeprom_w )
{
	if (ACCESSING_LSB)
	{
		switch (offset)
		{
			case 0:
				EEPROM_set_cs_line((data & 0x0001) ? CLEAR_LINE : ASSERT_LINE);
				break;
			case 1:
				EEPROM_set_clock_line((data & 0x0001) ? ASSERT_LINE : CLEAR_LINE);
				break;
			case 2:
				EEPROM_write_bit(data & 0x0001);
				break;
		}
	}
}

WRITE16_HANDLER( actionhw_snd_w )
{
	log_cb(RETRO_LOG_DEBUG, LOGPRE "PC:%06x Writing %04x to Sound CPU - mask %04x\n",activecpu_get_previouspc(),data,mem_mask);

	if (!ACCESSING_LSB) data >>= 8;

	switch (data)
	{
		case 0xfc:	OKIM6295_set_bank_base(0, (0 * 0x40000)); break;
		case 0xfd:	OKIM6295_set_bank_base(0, (2 * 0x40000)); break;
		case 0xfe:	OKIM6295_set_bank_base(0, (1 * 0x40000)); break;
		case 0xff:	OKIM6295_set_bank_base(0, (3 * 0x40000)); break;
		case 0x78:	OKIM6295_data_0_w(0,data);
					snd_sam[0]=00; snd_sam[1]=00; snd_sam[2]=00; snd_sam[3]=00;
					break;
		default:	if (snd_new) /* Play new sample */
					{
						if ((data & 0x80) && (snd_sam[3] != snd_new))
						{
							log_cb(RETRO_LOG_DEBUG, LOGPRE "About to play sample %02x at vol %02x\n",snd_new,data);
							if ((OKIM6295_status_0_r(0) & 0x08) != 0x08)
							{
							log_cb(RETRO_LOG_DEBUG, LOGPRE "Playing sample %02x at vol %02x\n",snd_new,data);
								OKIM6295_data_0_w(0,snd_new);
								OKIM6295_data_0_w(0,data);
							}
							snd_new = 00;
						}
						if ((data & 0x40) && (snd_sam[2] != snd_new))
						{
							log_cb(RETRO_LOG_DEBUG, LOGPRE "About to play sample %02x at vol %02x\n",snd_new,data);
							if ((OKIM6295_status_0_r(0) & 0x04) != 0x04)
							{
							log_cb(RETRO_LOG_DEBUG, LOGPRE "Playing sample %02x at vol %02x\n",snd_new,data);
								OKIM6295_data_0_w(0,snd_new);
								OKIM6295_data_0_w(0,data);
							}
							snd_new = 00;
						}
						if ((data & 0x20) && (snd_sam[1] != snd_new))
						{
							log_cb(RETRO_LOG_DEBUG, LOGPRE "About to play sample %02x at vol %02x\n",snd_new,data);
							if ((OKIM6295_status_0_r(0) & 0x02) != 0x02)
							{
							log_cb(RETRO_LOG_DEBUG, LOGPRE "Playing sample %02x at vol %02x\n",snd_new,data);
								OKIM6295_data_0_w(0,snd_new);
								OKIM6295_data_0_w(0,data);
							}
							snd_new = 00;
						}
						if ((data & 0x10) && (snd_sam[0] != snd_new))
						{
							log_cb(RETRO_LOG_DEBUG, LOGPRE "About to play sample %02x at vol %02x\n",snd_new,data);
							if ((OKIM6295_status_0_r(0) & 0x01) != 0x01)
							{
							log_cb(RETRO_LOG_DEBUG, LOGPRE "Playing sample %02x at vol %02x\n",snd_new,data);
								OKIM6295_data_0_w(0,snd_new);
								OKIM6295_data_0_w(0,data);
							}
							snd_new = 00;
						}
						break;
					}
					else if (data > 0x80) /* New sample command */
					{
						log_cb(RETRO_LOG_DEBUG, LOGPRE "Next sample %02x\n",data);
						snd_new = data;
						break;
					}
					else /* Turn a channel off */
					{
						log_cb(RETRO_LOG_DEBUG, LOGPRE "Turning channel %02x off\n",data);
						OKIM6295_data_0_w(0,data);
						if (data & 0x40) snd_sam[3] = 00;
						if (data & 0x20) snd_sam[2] = 00;
						if (data & 0x10) snd_sam[1] = 00;
						if (data & 0x08) snd_sam[0] = 00;
						snd_new = 00;
						break;
					}
	}
}

WRITE16_HANDLER( topdrive_snd_w )
{
	/*
		In theory this would be sample banking (it writes a value of 01 on startup)
		however all samples addresses in header are sequential, and data after
		the last used sample doesn't appear to be sound data anyway.
		Furthermore no other values are ever written here
	*/	
}

/* Memory Maps ****************************************************************

it doesn't seem able to read from fg/bg/spr/pal ram

*/

static MEMORY_READ16_START( kickgoal_readmem )
	{ 0x000000, 0x0fffff, MRA16_ROM },
	{ 0x800000, 0x800001, input_port_0_word_r },
	{ 0x800002, 0x800003, input_port_1_word_r },
	{ 0x900006, 0x900007, kickgoal_eeprom_r },
	{ 0xa00000, 0xa03fff, MRA16_RAM }, /* FG Layer */
	{ 0xa04000, 0xa07fff, MRA16_RAM }, /* Higher BG Layer */
	{ 0xa08000, 0xa0bfff, MRA16_RAM }, /* Lower BG Layer */
    { 0xa0c000, 0xa0ffff, MRA16_RAM }, /* more tilemap? */
	{ 0xc00000, 0xc007ff, MRA16_RAM }, /* Palette */
	{ 0xff0000, 0xffffff, MRA16_RAM },
MEMORY_END

static MEMORY_WRITE16_START( kickgoal_writemem )
	{ 0x000000, 0x0fffff, MWA16_ROM },
/*	{ 0x800004, 0x800005, soundlatch_word_w }, */
	{ 0x800004, 0x800005, actionhw_snd_w },
	{ 0x900000, 0x900005, kickgoal_eeprom_w },
	{ 0xa00000, 0xa03fff, kickgoal_fgram_w,  &kickgoal_fgram  }, /* FG Layer */
	{ 0xa04000, 0xa07fff, kickgoal_bgram_w,  &kickgoal_bgram  }, /* Higher BG Layer */
	{ 0xa08000, 0xa0bfff, kickgoal_bg2ram_w, &kickgoal_bg2ram }, /* Lower BG Layer */
    { 0xa0c000, 0xa0ffff, MWA16_RAM }, /* more tilemap? */
	{ 0xa10000, 0xa1000f, MWA16_RAM, &kickgoal_scrram }, /* Scroll Registers */
	{ 0xb00000, 0xb007ff, MWA16_RAM, &spriteram16, &spriteram_size  }, /* Sprites */
	{ 0xc00000, 0xc007ff, paletteram16_xxxxBBBBGGGGRRRR_word_w, &paletteram16 }, /* Palette */
	{ 0xff0000, 0xffffff, MWA16_RAM },
MEMORY_END


static MEMORY_READ16_START( topdrive_readmem )
    { 0x000000, 0x0fffff, MRA16_ROM },
	{ 0x800000, 0x800001, input_port_0_word_r },
	{ 0x800002, 0x800003, input_port_1_word_r },
/*	{ 0x800006, 0x800007, }, /* accessed in service menu, wheel maybe? */
	{ 0x900006, 0x900007, kickgoal_eeprom_r },
	{ 0xa00000, 0xa03fff, MRA16_RAM }, /* FG Layer */
	{ 0xa00400, 0xa01fff, MRA16_RAM },
	{ 0xa02000, 0xa03fff, MRA16_RAM }, /* buffer for scroll regs? or layer configs? */
	{ 0xa04000, 0xa043ff, MRA16_RAM }, /* Higher BG Layer */
	{ 0xa04400, 0xa07fff, MRA16_RAM },
	{ 0xa08000, 0xa083ff, MRA16_RAM }, /* Lower BG Layer */
	{ 0xa08400, 0xa0bfff, MRA16_RAM },
	{ 0xa0c000, 0xa0c3ff, MRA16_RAM }, /* seems to be a buffer for data that gets put at 0xa00000? */
	{ 0xa0c400, 0xa0ffff, MRA16_RAM },
	{ 0xc00000, 0xc007ff, MRA16_RAM }, /* Palette */ 
	{ 0xe00003, 0xe00003, OKIM6295_status_0_msb_r }, /* msb for this game */
	{ 0xf00000, 0xf2ffff, MRA16_RAM },
	{ 0xff0000, 0xffffff, MRA16_RAM },
MEMORY_END

static MEMORY_WRITE16_START( topdrive_writemem )
    { 0x000000, 0x0fffff, MWA16_ROM },
	{ 0x900000, 0x900005, kickgoal_eeprom_w },
	{ 0xa00000, 0xa03fff, topdrive_fgram_w, &topdrive_fgram }, /* FG Layer */
	{ 0xa00400, 0xa01fff, MWA16_RAM },
	{ 0xa02000, 0xa03fff, MWA16_RAM }, /* buffer for scroll regs? or layer configs? */
	{ 0xa04000, 0xa043ff, topdrive_bgram_w, &topdrive_bgram }, /* Higher BG Layer */
	{ 0xa04400, 0xa07fff, MWA16_RAM },
	{ 0xa08000, 0xa083ff, topdrive_bg2ram_w, &topdrive_bg2ram }, /* Lower BG Layer */
	{ 0xa08400, 0xa0bfff, MWA16_RAM },
	{ 0xa0c000, 0xa0c3ff, MWA16_RAM }, /* seems to be a buffer for data that gets put at 0xa00000? */
	{ 0xa0c400, 0xa0ffff, MWA16_RAM },
	{ 0xa10000, 0xa1000f, MWA16_RAM, &topdrive_scrram }, /* Scroll Registers */
	{ 0xb00000, 0xb007ff, MWA16_RAM, &spriteram16, &spriteram_size }, /* Sprites */
	{ 0xc00000, 0xc007ff, paletteram16_xBBBBBGGGGGRRRRR_word_w, &paletteram16 }, /* Palette */ 
	{ 0xe00003, 0xe00003, OKIM6295_data_0_msb_w }, /* msb for this game */
	{ 0xe00004, 0xe00005, topdrive_snd_w },
	{ 0xf00000, 0xf2ffff, MWA16_RAM },
	{ 0xff0000, 0xffffff, MWA16_RAM },
MEMORY_END

/* INPUT ports ***************************************************************/

INPUT_PORTS_START( kickgoal )
	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER1 )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BITX(0x0800, IP_ACTIVE_LOW, IPT_SERVICE, DEF_STR( Service_Mode ), KEYCODE_F2, IP_JOY_NONE )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END

INPUT_PORTS_START( topdrive )
	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER1 )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BITX(0x0800, IP_ACTIVE_LOW, IPT_SERVICE, DEF_STR( Service_Mode ), KEYCODE_F2, IP_JOY_NONE )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END

/* GFX Decodes ***************************************************************/

static struct GfxLayout fg816_charlayout =
{
	8,16,
	RGN_FRAC(1,4),
	4,
	{ RGN_FRAC(3,4), RGN_FRAC(2,4), RGN_FRAC(1,4), RGN_FRAC(0,4) },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
			8*8, 9*8, 10*8, 11*8, 12*8, 13*8, 14*8, 15*8 },
	16*8
};

static struct GfxLayout bg1632_charlayout =
{
	16,32,
	RGN_FRAC(1,4),
	4,
	{ RGN_FRAC(3,4), RGN_FRAC(2,4), RGN_FRAC(1,4), RGN_FRAC(0,4) },
	{ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 },
	{ 0*16, 0*16, 1*16, 1*16, 2*16,  2*16,  3*16,  3*16,  4*16,  4*16,  5*16,  5*16,  6*16,  6*16,  7*16, 7*16,
	  8*16, 8*16, 9*16, 9*16, 10*16, 10*16, 11*16, 11*16, 12*16, 12*16, 13*16, 13*16, 14*16, 14*16, 15*16, 15*16 },
	16*16
};

static struct GfxLayout bg3264_charlayout =
{
	32,64,
	RGN_FRAC(1,4),
	4,
	{ RGN_FRAC(3,4), RGN_FRAC(2,4), RGN_FRAC(1,4), RGN_FRAC(0,4) },
	{ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15,
	  16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31 },
	{ 0*32,  0*32,  1*32,  1*32,  2*32,  2*32,  3*32,  3*32,  4*32,  4*32,  5*32,  5*32,  6*32,  6*32,  7*32,  7*32,
	  8*32,  8*32,  9*32,  9*32,  10*32, 10*32, 11*32, 11*32, 12*32, 12*32, 13*32, 13*32, 14*32, 14*32, 15*32, 15*32,
	  16*32, 16*32, 17*32, 17*32, 18*32, 18*32, 19*32, 19*32, 20*32, 20*32, 21*32, 21*32, 22*32, 22*32, 23*32, 23*32,
	  24*32, 24*32, 25*32, 25*32, 26*32, 26*32, 27*32, 27*32, 28*32, 28*32, 29*32, 29*32, 30*32, 30*32, 31*32, 31*32 },
	32*32
};


static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &fg816_charlayout,   0x000, 0x40 },
	{ REGION_GFX1, 0, &bg1632_charlayout,  0x000, 0x40 },
	{ REGION_GFX1, 0, &bg3264_charlayout,  0x000, 0x40 },
	{ -1 } /* end of array */
};

static struct GfxLayout actionhw_fg88_alt_charlayout =
{
	8,8,
	RGN_FRAC(1,4),
	4,
	{ RGN_FRAC(3,4), RGN_FRAC(2,4), RGN_FRAC(1,4), RGN_FRAC(0,4) },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8,  2*8,  3*8,  4*8, 5*8, 6*8,  7*8 },
	8*8
};


static struct GfxLayout actionhw_bg1616_charlayout =
{
	16,16,
	RGN_FRAC(1,4),
	4,
	{ RGN_FRAC(3,4), RGN_FRAC(2,4), RGN_FRAC(1,4), RGN_FRAC(0,4) },
	{ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 },
	{ 0*16,  1*16,  2*16,  3*16,   4*16,   5*16,   6*16,   7*16,
	  8*16, 9*16, 10*16, 11*16, 12*16, 13*16, 14*16, 15*16 },
	16*16
};



static struct GfxDecodeInfo actionhw_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &actionhw_fg88_alt_charlayout,   0x000, 0x40 },
	{ REGION_GFX1, 0, &actionhw_bg1616_charlayout,  0x000, 0x40 },
	{ -1 } /* end of array */
};

static struct GfxLayout topdrive_bg1616_charlayout =
{
	16,16,
	RGN_FRAC(1,4),
	4,
	{ RGN_FRAC(3,4), RGN_FRAC(2,4), RGN_FRAC(1,4), RGN_FRAC(0,4) },
	{ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 },
	{ 0*16,  1*16,  2*16,  3*16,   4*16,   5*16,   6*16,   7*16,
	  8*16, 9*16, 10*16, 11*16, 12*16, 13*16, 14*16, 15*16 },
	16*16
};

static struct GfxDecodeInfo topdrive_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &topdrive_bg1616_charlayout,  0x000, 0x40 },
	{ -1 } /* end of array */
};

static struct OKIM6295interface okim6295_interface =
{
	1,                 /* 1 chip */
	{ 12000000/8/165 },/* Frequency */
	{ REGION_SOUND1 },
	{ 80 }
};

static struct OKIM6295interface topdrive_okim6295_interface =
{
	1,				/* 1 chip */
    { 1000000/165 },
	{ REGION_SOUND1 },	/* memory region */
	{ 100 }
};


/* MACHINE drivers ***********************************************************/

static MACHINE_DRIVER_START( kickgoal )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 12000000)	/* 12 MHz */
	MDRV_CPU_MEMORY(kickgoal_readmem,kickgoal_writemem)
	MDRV_CPU_VBLANK_INT(irq6_line_hold,1)

	/* pic16c57? */

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_NVRAM_HANDLER(kickgoal)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(64*8, 64*8)
	MDRV_VISIBLE_AREA(9*8, 55*8-1, 4*8, 60*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(1024)

	MDRV_VIDEO_START(kickgoal)
	MDRV_VIDEO_UPDATE(kickgoal)

	/* sound hardware */
/*	MDRV_SOUND_ADD(OKIM6295, okim6295_interface) */
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( actionhw )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 12000000)	/* 12 MHz */
	MDRV_CPU_MEMORY(kickgoal_readmem,kickgoal_writemem)
	MDRV_CPU_VBLANK_INT(irq6_line_hold,1)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_NVRAM_HANDLER(kickgoal) /* 93C46 really */

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(64*8, 64*8)
	MDRV_VISIBLE_AREA(10*8+2, 54*8-1+2, 0*8, 30*8-1)
	MDRV_GFXDECODE(actionhw_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(1024)

	MDRV_VIDEO_START(actionhw)
	MDRV_VIDEO_UPDATE(actionhw)

	/* sound hardware */
	MDRV_SOUND_ADD(OKIM6295, okim6295_interface)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( topdrive )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 12000000)	/* 12 MHz */
	MDRV_CPU_MEMORY(topdrive_readmem, topdrive_writemem)
	MDRV_CPU_VBLANK_INT(irq2_line_hold,1)

	MDRV_FRAMES_PER_SECOND(58.75)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_NVRAM_HANDLER(kickgoal) /* 93C46 really */

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(64*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 48*8-1, 0*8, 30*8-1)
	MDRV_GFXDECODE(topdrive_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(0x400)

	MDRV_VIDEO_START(topdrive)
	MDRV_VIDEO_UPDATE(topdrive)

	/* sound hardware */
	MDRV_SOUND_ADD(OKIM6295, topdrive_okim6295_interface)
MACHINE_DRIVER_END

/* Rom Loading ***************************************************************/

ROM_START( kickgoal )
	ROM_REGION( 0x80000, REGION_CPU1, 0 )	/* 68000 code */
	ROM_LOAD16_BYTE( "ic6",   0x000000, 0x40000, CRC(498ca792) SHA1(c638c3a1755870010c5961b58bcb02458ff4e238) )
	ROM_LOAD16_BYTE( "ic5",   0x000001, 0x40000, CRC(d528740a) SHA1(d56a71004aabc839b0833a6bf383e5ef9d4948fa) )

	ROM_REGION( 0x0800, REGION_CPU2, 0 )	/* sound? (missing) */
	ROM_LOAD( "pic16c57",     0x0000, 0x0800, NO_DUMP )

	ROM_REGION( 0x200000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "ic33",   0x000000, 0x80000, CRC(5038f52a) SHA1(22ed0e2c8a99056e73cff912731626689996a276) )
	ROM_LOAD( "ic34",   0x080000, 0x80000, CRC(06e7094f) SHA1(e41b893ef91d541d2623d76ce6c69ecf4218c16d) )
	ROM_LOAD( "ic35",   0x100000, 0x80000, CRC(ea010563) SHA1(5e474db372550e9d33f933ab00881a9b29a712d1) )
	ROM_LOAD( "ic36",   0x180000, 0x80000, CRC(b6a86860) SHA1(73ab43830d5e62154bc8953615cdb397c7a742aa) )

	ROM_REGION( 0x080000, REGION_SOUND1, 0 )	/* OKIM6295 samples */
	ROM_LOAD( "ic13",   0x00000, 0x080000, CRC(c6cb56e9) SHA1(835773b3f0647d3c553180bcf10e57ad44d68353) ) /* BAD ADDRESS LINES (mask=010000) */
ROM_END

ROM_START( actionhw )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )	/* 68000 code */
	ROM_LOAD16_BYTE( "2.ic6",  0x000000, 0x80000, CRC(2b71d58c) SHA1(3e58531fa56d41a3c7944e3beab4850907564a89) )
	ROM_LOAD16_BYTE( "1.ic5",  0x000001, 0x80000, CRC(136b9711) SHA1(553f9fdd99bb9ce2e1492d0755633075e59ba587) )

	ROM_REGION( 0x1000, REGION_CPU2, 0 )	/* sound? (missing) */
	ROM_LOAD( "pic16c57",     0x0000, 0x0800, NO_DUMP )

	ROM_REGION( 0x400000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "4.ic29",  0x000000, 0x80000, CRC(df076744) SHA1(4b2c8e21a201e1491e4ba3cda8d71b51e0943431) )
	ROM_LOAD( "5.ic33",  0x080000, 0x80000, CRC(8551fdd4) SHA1(f29bdfb75af7607534de171d7b3927419c00377c) )
	ROM_LOAD( "6.ic30",  0x100000, 0x80000, CRC(5cb005a5) SHA1(d3a5ab8f9a520bfaa53fdf6145142ccba416fbb8) )
	ROM_LOAD( "7.ic34",  0x180000, 0x80000, CRC(c2f7d284) SHA1(b3c3d6aa932c813affd667344ea5ddefa55f219b) )
	ROM_LOAD( "8.ic31",  0x200000, 0x80000, CRC(50dffa47) SHA1(33da3b2cabb7b0e480158d343e876563bd0f0930) )
	ROM_LOAD( "9.ic35",  0x280000, 0x80000, CRC(c1ea0370) SHA1(c836611e478d2bf9ae2a5d7e7665982c2b731189) )
	ROM_LOAD( "10.ic32", 0x300000, 0x80000, CRC(5ee5db3e) SHA1(c79f84548ce5311acac478c5180330bf56485863) )
	ROM_LOAD( "11.ic36", 0x380000, 0x80000, CRC(8d376b1e) SHA1(37f16b3237d9813a8d153ab5640252e7643f3b99) )

	/* $00000-$20000 stays the same in all sound banks, */
	/* the second half of the bank is the area that gets switched */
	ROM_REGION( 0x100000, REGION_SOUND1, 0 )    /* OKIM6295 samples */
	ROM_LOAD( "3.ic13",      0x00000, 0x40000, CRC(b8f6705d) SHA1(55116e14aba6dac7334e26f704b3e6b0b9f856c2) )
	ROM_CONTINUE(            0x60000, 0x20000 )
	ROM_CONTINUE(            0xa0000, 0x20000 )
	ROM_COPY( REGION_SOUND1, 0x00000, 0x40000, 0x20000)
	ROM_COPY( REGION_SOUND1, 0x00000, 0x80000, 0x20000)
	ROM_COPY( REGION_SOUND1, 0x00000, 0xc0000, 0x20000) /* Last bank used in Test Mode */
ROM_END

ROM_START( topdrive )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_BYTE( "2-27c040.bin", 0x00000, 0x80000, CRC(37798c4e) SHA1(708a64b416bd2104fbc4b72a37bfeae33bbab454) )
	ROM_LOAD16_BYTE( "1-27c040.bin", 0x00001, 0x80000, CRC(e2dc5096) SHA1(82b22e03be225ab7f20eff6314383a9f28d52294) )

	ROM_REGION( 0x400000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "4-27c040.bin",  0x000000, 0x80000, CRC(a81ca7f7) SHA1(cc2030a9bea90b694adbf222389766945ce9552b) )
	ROM_LOAD( "5-27c040.bin",  0x080000, 0x80000, CRC(a756d2b2) SHA1(59ddef858850b0f6c5865d555d6402c41cc3cb6c) )
	ROM_LOAD( "6-27c040.bin",  0x100000, 0x80000, CRC(90c778a2) SHA1(8122ee085e388bb1f7952edb6a99dffc466f2e2c) )
	ROM_LOAD( "7-27c040.bin",  0x180000, 0x80000, CRC(db219087) SHA1(c79145555678971db29e91a24d69738da7d8f07f) )
	ROM_LOAD( "8-27c040.bin",  0x200000, 0x80000, CRC(0e5f4419) SHA1(4fc8173001e2b412f4a7b0b5160c853436bbb139) )
	ROM_LOAD( "9-27c040.bin",  0x280000, 0x80000, CRC(159a7426) SHA1(6851fbc1fe11ae72a86d35011730d2df641e8fc5) )
	ROM_LOAD( "10-27c040.bin", 0x300000, 0x80000, CRC(54c1617a) SHA1(7bb4faaa54581f080f19f98e78fa9cae899f4c2a) )
	ROM_LOAD( "11-27c040.bin", 0x380000, 0x80000, CRC(6b3c3c73) SHA1(8ac76abdc4676cfcd9dc66a4c7b55010de099133) )

	ROM_REGION( 0x80000, REGION_SOUND1, 0 )
	ROM_LOAD( "3-27c040.bin",      0x00000, 0x80000, CRC(2894b89b) SHA1(cf884042edd2fc05e04d21ccd36f5183f9a7ec5c) )
ROM_END

/* GAME drivers **************************************************************/

DRIVER_INIT( kickgoal )
{
	data16_t *rom = (data16_t *)memory_region(REGION_CPU1);

	/* fix "bug" that prevents game from writing to EEPROM */
	rom[0x12b0/2] = 0x0001;
}


GAMEX( 1995, actionhw,0, actionhw, kickgoal, 0,         ROT0, "TCH", "Action Hollywood", GAME_IMPERFECT_SOUND )
GAMEX( 1995, kickgoal,0, kickgoal, kickgoal, kickgoal,  ROT0, "TCH", "Kick Goal", GAME_NO_SOUND )
GAMEX( 1995, topdrive,0, topdrive, topdrive, kickgoal,  ROT0, "Proyesel", "Top Driving (version 1.1)", GAME_IMPERFECT_SOUND )

