/***************************************************************************

Konami games memory map (preliminary)

Based on drivers from Juno First emulator by Chris Hardy (chrish@kcbbs.gen.nz)

Track'n'Field

MAIN BOARD:
0000-17ff RAM
1800-183f Sprite RAM Pt 1
1C00-1C3f Sprite RAM Pt 2
3800-3bff Color RAM
3000-33ff Video RAM
6000-ffff ROM
1200-12ff IO

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "cpu/m6809/m6809.h"


extern void konami1_decode(void);


extern UINT8 *trackfld_scroll;
extern UINT8 *trackfld_scroll2;

extern WRITE_HANDLER( trackfld_videoram_w );
extern WRITE_HANDLER( trackfld_colorram_w );
extern WRITE_HANDLER( trackfld_flipscreen_w );

extern PALETTE_INIT( trackfld );
extern VIDEO_START( trackfld );
extern VIDEO_UPDATE( trackfld );

extern WRITE_HANDLER( konami_sh_irqtrigger_w );
extern READ_HANDLER( trackfld_sh_timer_r );
extern READ_HANDLER( trackfld_speech_r );
extern WRITE_HANDLER( trackfld_sound_w );
extern READ_HANDLER( hyprolyb_speech_r );
extern WRITE_HANDLER( hyprolyb_ADPCM_data_w );

extern struct SN76496interface konami_sn76496_interface;
extern struct DACinterface konami_dac_interface;
extern struct ADPCMinterface hyprolyb_adpcm_interface;


/* handle fake button for speed cheat */
static READ_HANDLER( konami_IN1_r )
{
	int res;
	static int cheat = 0;
	static int bits[] = { 0xee, 0xff, 0xbb, 0xaa };

	res = readinputport(1);

	if ((res & 0x80) == 0)
	{
		res |= 0x55;
		res &= bits[cheat];
		cheat = (cheat+1)%4;
	}
	return res;
}

/*
 Track'n'Field has 1k of battery backed RAM which can be erased by setting a dipswitch
*/
static UINT8 *nvram;
static size_t nvram_size;
static int we_flipped_the_switch;

static NVRAM_HANDLER( trackfld )
{
	if (read_or_write)
	{
		mame_fwrite(file,nvram,nvram_size);

		if (we_flipped_the_switch)
		{
			struct InputPort *in;


			/* find the dip switch which resets the high score table, and set it */
			/* back to off. */
			in = Machine->input_ports;

			while (in->type != IPT_END)
			{
				if (in->name != NULL && in->name != IP_NAME_DEFAULT &&
						strcmp(in->name,"World Records") == 0)
				{
					if (in->default_value == 0)
						in->default_value = in->mask;
					break;
				}

				in++;
			}

			we_flipped_the_switch = 0;
		}
	}
	else
	{
		if (file)
		{
			mame_fread(file,nvram,nvram_size);
			we_flipped_the_switch = 0;
		}
		else
		{
			struct InputPort *in;


			/* find the dip switch which resets the high score table, and set it on */
			in = Machine->input_ports;

			while (in->type != IPT_END)
			{
				if (in->name != NULL && in->name != IP_NAME_DEFAULT &&
						strcmp(in->name,"World Records") == 0)
				{
					if (in->default_value == in->mask)
					{
						in->default_value = 0;
						we_flipped_the_switch = 1;
					}
					break;
				}

				in++;
			}
		}
	}
}

static NVRAM_HANDLER( mastkin )
{
	if (read_or_write)
		mame_fwrite(file,nvram,nvram_size);
	else
	{
		if (file)
			mame_fread(file,nvram,nvram_size);
	}
}

static WRITE_HANDLER( coin_w )
{
	coin_counter_w(offset,data & 1);
}


static MEMORY_READ_START( readmem )
	{ 0x1200, 0x1200, input_port_4_r }, /* DIP 2 */
	{ 0x1280, 0x1280, input_port_0_r }, /* IO Coin */
/*	{ 0x1281, 0x1281, input_port_1_r },*/  /* P1 IO /*/ 
	{ 0x1281, 0x1281, konami_IN1_r },	/* P1 IO and handle fake button for cheating */
	{ 0x1282, 0x1282, input_port_2_r }, /* P2 IO */
	{ 0x1283, 0x1283, input_port_3_r }, /* DIP 1 */
	{ 0x1800, 0x1fff, MRA_RAM },
	{ 0x2800, 0x3fff, MRA_RAM },
	{ 0x6000, 0xffff, MRA_ROM },
MEMORY_END

static MEMORY_WRITE_START( writemem )
	{ 0x1000, 0x1000, watchdog_reset_w },
	{ 0x1080, 0x1080, trackfld_flipscreen_w },
	{ 0x1081, 0x1081, konami_sh_irqtrigger_w },  /* cause interrupt on audio CPU */
	{ 0x1083, 0x1084, coin_w },
	{ 0x1087, 0x1087, interrupt_enable_w },
	{ 0x1100, 0x1100, soundlatch_w },
	{ 0x1800, 0x183f, MWA_RAM, &spriteram_2 },
	{ 0x1840, 0x185f, MWA_RAM, &trackfld_scroll },
	{ 0x1860, 0x1bff, MWA_RAM },
	{ 0x1c00, 0x1c3f, MWA_RAM, &spriteram, &spriteram_size },
	{ 0x1c40, 0x1c5f, MWA_RAM, &trackfld_scroll2 },
	{ 0x1c60, 0x1fff, MWA_RAM },
	{ 0x2800, 0x2bff, MWA_RAM },
	{ 0x2c00, 0x2fff, MWA_RAM, &nvram, &nvram_size },
	{ 0x3000, 0x37ff, trackfld_videoram_w, &videoram },
	{ 0x3800, 0x3fff, trackfld_colorram_w, &colorram },
	{ 0x6000, 0xffff, MWA_ROM },
MEMORY_END

static MEMORY_READ_START( mastkin_readmem )
	{ 0x1200, 0x1200, input_port_4_r }, /* DIP 2 */
	{ 0x1280, 0x1280, input_port_0_r }, /* IO Coin */
	{ 0x1281, 0x1281, input_port_1_r }, /* P1 IO */
/*	{ 0x1282, 0x1282, input_port_2_r },*/ /* unused */ 
	{ 0x1283, 0x1283, input_port_3_r }, /* DIP 1 */
	{ 0x1800, 0x1fff, MRA_RAM },
	{ 0x2800, 0x3fff, MRA_RAM },
	{ 0x6000, 0xffff, MRA_ROM },
MEMORY_END

static MEMORY_WRITE_START( mastkin_writemem )
	{ 0x1000, 0x1000, watchdog_reset_w },
	{ 0x10b0, 0x10b0, trackfld_flipscreen_w },
	{ 0x10b1, 0x10b1, konami_sh_irqtrigger_w },
	{ 0x1083, 0x1084, coin_w },
	{ 0x1087, 0x1087, interrupt_enable_w },
	{ 0x1100, 0x1100, soundlatch_w },
	{ 0x1800, 0x183f, MWA_RAM, &spriteram_2 },
	{ 0x1840, 0x185f, MWA_RAM, &trackfld_scroll },
	{ 0x1860, 0x1bff, MWA_RAM },
	{ 0x1c00, 0x1c3f, MWA_RAM, &spriteram, &spriteram_size },
	{ 0x1c40, 0x1c5f, MWA_RAM, &trackfld_scroll2 },
	{ 0x1c60, 0x1fff, MWA_RAM },
	{ 0x2800, 0x2bff, MWA_RAM },
	{ 0x2c00, 0x2fff, MWA_RAM, &nvram, &nvram_size },
	{ 0x3000, 0x37ff, trackfld_videoram_w, &videoram },
	{ 0x3800, 0x3fff, trackfld_colorram_w, &colorram },
	{ 0x6000, 0xffff, MWA_ROM },
MEMORY_END

static MEMORY_READ_START( sound_readmem )
	{ 0x0000, 0x3fff, MRA_ROM },
	{ 0x4000, 0x43ff, MRA_RAM },
	{ 0x6000, 0x6000, soundlatch_r },
	{ 0x8000, 0x8000, trackfld_sh_timer_r },
	{ 0xe002, 0xe002, trackfld_speech_r },
MEMORY_END

static MEMORY_WRITE_START( sound_writemem )
	{ 0x0000, 0x3fff, MWA_ROM },
	{ 0x4000, 0x43ff, MWA_RAM },
	{ 0xa000, 0xa000, SN76496_0_w },	/* Loads the snd command into the snd latch */
	{ 0xc000, 0xc000, MWA_NOP },		/* This address triggers the SN chip to read the data port. */
	{ 0xe000, 0xe000, DAC_0_data_w },
/* There are lots more addresses which are used for setting a two bit volume
	controls for speech and music

	Currently these are un-supported by Mame
*/
	{ 0xe001, 0xe001, MWA_NOP }, /* watch dog ? */
	{ 0xe004, 0xe004, VLM5030_data_w },
	{ 0xe000, 0xefff, trackfld_sound_w, }, /* e003 speech control */
MEMORY_END

static MEMORY_READ_START( hyprolyb_sound_readmem )
	{ 0x0000, 0x3fff, MRA_ROM },
	{ 0x4000, 0x43ff, MRA_RAM },
	{ 0x6000, 0x6000, soundlatch_r },
	{ 0x8000, 0x8000, trackfld_sh_timer_r },
	{ 0xe002, 0xe002, hyprolyb_speech_r },
MEMORY_END

static MEMORY_WRITE_START( hyprolyb_sound_writemem )
	{ 0x0000, 0x3fff, MWA_ROM },
	{ 0x4000, 0x43ff, MWA_RAM },
	{ 0xa000, 0xa000, SN76496_0_w },	/* Loads the snd command into the snd latch */
	{ 0xc000, 0xc000, MWA_NOP },		/* This address triggers the SN chip to read the data port. */
	{ 0xe000, 0xe000, DAC_0_data_w },
/* There are lots more addresses which are used for setting a two bit volume
	controls for speech and music

	Currently these are un-supported by Mame
*/
	{ 0xe001, 0xe001, MWA_NOP }, /* watch dog ? */
	{ 0xe004, 0xe004, hyprolyb_ADPCM_data_w },
	{ 0xe000, 0xefff, MWA_NOP },
MEMORY_END



INPUT_PORTS_START( trackfld )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START3 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
/*	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )*/
	/* Fake button to press buttons 1 and 3 impossibly fast. Handle via konami_IN1_r */
	PORT_BITX(0x80, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_CHEAT | IPF_PLAYER1, "Run Like Hell Cheat", IP_KEY_DEFAULT, IP_JOY_DEFAULT )

	PORT_START      /* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER3 /*| IPF_COCKTAIL*/ )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER3 /*| IPF_COCKTAIL*/ )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER3 /*| IPF_COCKTAIL*/ )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START4 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER4 /*| IPF_COCKTAIL*/ )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER4 /*| IPF_COCKTAIL*/ )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER4 /*| IPF_COCKTAIL*/ )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* DSW0 */
	PORT_DIPNAME( 0x0f, 0x0f, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x05, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 3C_2C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 4C_3C ) )
	PORT_DIPSETTING(    0x0f, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 3C_4C ) )
	PORT_DIPSETTING(    0x07, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x0e, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 2C_5C ) )
	PORT_DIPSETTING(    0x0d, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x0b, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0x0a, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(    0x09, DEF_STR( 1C_7C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0xf0, 0xf0, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x50, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 3C_2C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 4C_3C ) )
	PORT_DIPSETTING(    0xf0, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 3C_4C ) )
	PORT_DIPSETTING(    0x70, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0xe0, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x60, DEF_STR( 2C_5C ) )
	PORT_DIPSETTING(    0xd0, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0xb0, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0xa0, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(    0x90, DEF_STR( 1C_7C ) )
	PORT_DIPSETTING(    0x00, "Disabled" )
/* 0x00 disables Coin 2. It still accepts coins and makes the sound, but
   it doesn't give you any credit */

	PORT_START      /* DSW1 */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x01, "1" )
	PORT_DIPSETTING(    0x00, "2" )
	PORT_DIPNAME( 0x02, 0x00, "After Last Event" )
	PORT_DIPSETTING(    0x02, "Game Over" )
	PORT_DIPSETTING(    0x00, "Game Continues" )
	PORT_DIPNAME( 0x04, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Cocktail ))
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x08, "None" )
	PORT_DIPSETTING(    0x00, "100000" )
	PORT_DIPNAME( 0x10, 0x10, "World Records" )
	PORT_DIPSETTING(    0x10, "Don't Erase" )
	PORT_DIPSETTING(    0x00, "Erase on Reset" )
	PORT_DIPNAME( 0x60, 0x60, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x60, "Easy" )
	PORT_DIPSETTING(    0x40, "Normal" )
	PORT_DIPSETTING(    0x20, "Hard" )
	PORT_DIPSETTING(    0x00, "Difficult" )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
INPUT_PORTS_END

INPUT_PORTS_START( mastkin )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* DSW0 */
	PORT_DIPNAME( 0x01, 0x00, "Allow Continue" )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, "Timer Speed" )
	PORT_DIPSETTING(    0x02, "Normal" )
	PORT_DIPSETTING(    0x00, "Fast" )
	PORT_DIPNAME( 0x0c, 0x04, DEF_STR( Difficulty ) )	/* "Damage"*/
	PORT_DIPSETTING(    0x0c, "Easy" )				/*   0x03*/
	PORT_DIPSETTING(    0x04, "Normal" )			/*   0x07*/
	PORT_DIPSETTING(    0x08, "Hard" )				/*   0x0b*/
	PORT_DIPSETTING(    0x00, "Very Hard" )			/*   0x0f*/
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x10, "4" )
	PORT_DIPSETTING(    0x00, "5" )
	PORT_DIPNAME( 0x20, 0x00, "Internal speed" )		/* Check code at 0x8576*/
	PORT_DIPSETTING(    0x20, "Slow" )				/*   0x0c00*/
	PORT_DIPSETTING(    0x00, "Fast" )				/*   0x0a00*/
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED )		/* Stored at 0x284e but not read back*/
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )		/* Cocktail Mode, not used*/

	PORT_START      /* DSW1 */
	PORT_DIPNAME( 0x0f, 0x00, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x0a, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x09, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x05, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 2C_2C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x07, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 2C_4C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_5C ) )
/*	PORT_DIPSETTING(    0x0b, DEF_STR( 1C_1C ) )*/
/*	PORT_DIPSETTING(    0x0c, DEF_STR( 1C_1C ) )*/
/*	PORT_DIPSETTING(    0x0d, DEF_STR( 1C_1C ) )*/
/*	PORT_DIPSETTING(    0x0e, DEF_STR( 1C_1C ) )*/
/*	PORT_DIPSETTING(    0x0f, DEF_STR( 1C_1C ) )*/
	PORT_DIPNAME( 0xf0, 0x00, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0xa0, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x90, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x50, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x60, DEF_STR( 2C_2C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x70, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 2C_4C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 1C_5C ) )
/*	PORT_DIPSETTING(    0xb0, DEF_STR( 1C_1C ) )*/
/*	PORT_DIPSETTING(    0xc0, DEF_STR( 1C_1C ) )*/
/*	PORT_DIPSETTING(    0xd0, DEF_STR( 1C_1C ) )*/
/*	PORT_DIPSETTING(    0xe0, DEF_STR( 1C_1C ) )*/
/*	PORT_DIPSETTING(    0xf0, DEF_STR( 1C_1C ) )*/
INPUT_PORTS_END



static struct GfxLayout charlayout =
{
	8,8,
	RGN_FRAC(1,1),
	4,
	{ 0, 1, 2, 3 },
	{ 0*4, 1*4, 2*4, 3*4, 4*4, 5*4, 6*4, 7*4 },
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32 },
	32*8
};

static struct GfxLayout spritelayout =
{
	16,16,
	RGN_FRAC(1,2),
	4,
	{ RGN_FRAC(1,2)+4, RGN_FRAC(1,2)+0, 4, 0 },
	{ 0, 1, 2, 3, 8*8+0, 8*8+1, 8*8+2, 8*8+3,
			16*8+0, 16*8+1, 16*8+2, 16*8+3, 24*8+0, 24*8+1, 24*8+2, 24*8+3 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
			32*8, 33*8, 34*8, 35*8, 36*8, 37*8, 38*8, 39*8 },
	64*8
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &charlayout,       0, 16 },
	{ REGION_GFX2, 0, &spritelayout, 16*16, 16 },
	{ -1 } /* end of array */
};


struct VLM5030interface trackfld_vlm5030_interface =
{
	3580000,    /* master clock  */
	100,        /* volume        */
	REGION_SOUND1,	/* memory region  */
	0           /* memory size    */
};



static MACHINE_DRIVER_START( trackfld )

	/* basic machine hardware */
	MDRV_CPU_ADD_TAG("main", M6809, 2048000)        /* 1.400 MHz ??? */
	MDRV_CPU_MEMORY(readmem,writemem)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,1)

	MDRV_CPU_ADD(Z80,14318180/4)
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)	/* Z80 Clock is derived from a 14.31818 MHz crystal */
	MDRV_CPU_MEMORY(sound_readmem,sound_writemem)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_NVRAM_HANDLER(trackfld)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 2*8, 30*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(32)
	MDRV_COLORTABLE_LENGTH(16*16+16*16)

	MDRV_PALETTE_INIT(trackfld)
	MDRV_VIDEO_START(trackfld)
	MDRV_VIDEO_UPDATE(trackfld)

	/* sound hardware */
	MDRV_SOUND_ADD(DAC, konami_dac_interface)
	MDRV_SOUND_ADD(SN76496, konami_sn76496_interface)
	MDRV_SOUND_ADD(VLM5030, trackfld_vlm5030_interface)
MACHINE_DRIVER_END

/* same as the original, but uses ADPCM instead of VLM5030 */
/* also different memory handlers do handle that */
static MACHINE_DRIVER_START( hyprolyb )

	/* basic machine hardware */
	MDRV_CPU_ADD(M6809, 2048000)        /* 1.400 MHz ??? */
	MDRV_CPU_MEMORY(readmem,writemem)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,1)

	MDRV_CPU_ADD(Z80,14318180/4)
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)	/* Z80 Clock is derived from a 14.31818 MHz crystal */
	MDRV_CPU_MEMORY(hyprolyb_sound_readmem,hyprolyb_sound_writemem)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_NVRAM_HANDLER(trackfld)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 2*8, 30*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(32)
	MDRV_COLORTABLE_LENGTH(16*16+16*16)

	MDRV_PALETTE_INIT(trackfld)
	MDRV_VIDEO_START(trackfld)
	MDRV_VIDEO_UPDATE(trackfld)

	/* sound hardware */
	MDRV_SOUND_ADD(DAC, konami_dac_interface)
	MDRV_SOUND_ADD(SN76496, konami_sn76496_interface)
	MDRV_SOUND_ADD(ADPCM, hyprolyb_adpcm_interface)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( mastkin )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(trackfld)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_MEMORY(mastkin_readmem,mastkin_writemem)

	MDRV_NVRAM_HANDLER(mastkin)
MACHINE_DRIVER_END



/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( trackfld )
	ROM_REGION( 2*0x10000, REGION_CPU1, 0 )     /* 64k for code + 64k for decrypted opcodes */
	ROM_LOAD( "a01_e01.bin",  0x6000, 0x2000, CRC(2882f6d4) SHA1(f7ddae2c5412a2849efd7f9629e92a5b0328e7cb) )
	ROM_LOAD( "a02_e02.bin",  0x8000, 0x2000, CRC(1743b5ee) SHA1(31301031a525f893c31461f634350f01a9492ef4) )
	ROM_LOAD( "a03_k03.bin",  0xa000, 0x2000, CRC(6c0d1ee9) SHA1(380ab2162153a61910a6fe5b6d091ca9451ad4fd) )
	ROM_LOAD( "a04_e04.bin",  0xc000, 0x2000, CRC(21d6c448) SHA1(6c42cc76302485954a31520bdd08469fa948c72f) )
	ROM_LOAD( "a05_e05.bin",  0xe000, 0x2000, CRC(f08c7b7e) SHA1(50e65d9b0ea37d2afb2dfdf1f3e1378e3290bc81) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for the audio CPU */
	ROM_LOAD( "c2_d13.bin",   0x0000, 0x2000, CRC(95bf79b6) SHA1(ea9135acd7ad162c19c5cdde356e69792d61b675) )

	ROM_REGION( 0x6000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "h16_e12.bin",  0x0000, 0x2000, CRC(50075768) SHA1(dfff92c0f59dd3d8d3d6256944bfd48792cef6a9) )
	ROM_LOAD( "h15_e11.bin",  0x2000, 0x2000, CRC(dda9e29f) SHA1(0f41cde82bb60c3f1591ee14dc3cff4642bbddc1) )
	ROM_LOAD( "h14_e10.bin",  0x4000, 0x2000, CRC(c2166a5c) SHA1(5ba25900e653ce4edcf35f1fbce758a327a715ce) )

	ROM_REGION( 0x8000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "c11_d06.bin",  0x0000, 0x2000, CRC(82e2185a) SHA1(1da9ea20e7af0b49c62fb39834a7ec686491af04) )
	ROM_LOAD( "c12_d07.bin",  0x2000, 0x2000, CRC(800ff1f1) SHA1(33d73b18903e3e6bfb30f1a06db4b8105d4040d8) )
	ROM_LOAD( "c13_d08.bin",  0x4000, 0x2000, CRC(d9faf183) SHA1(4448b6242790783d37acf50704d597af5878c2ab) )
	ROM_LOAD( "c14_d09.bin",  0x6000, 0x2000, CRC(5886c802) SHA1(884a12a8f63600da4f23b29be6dbaacef37add20) )

	ROM_REGION( 0x0220, REGION_PROMS, 0 )
	ROM_LOAD( "tfprom.1",     0x0000, 0x0020, CRC(d55f30b5) SHA1(4d6a851f4886778307f75771645078b97ad55f5f) ) /* palette */
	ROM_LOAD( "tfprom.3",     0x0020, 0x0100, CRC(d2ba4d32) SHA1(894b5cedf01ba9225a0d6215291857e455b84903) ) /* sprite lookup table */
	ROM_LOAD( "tfprom.2",     0x0120, 0x0100, CRC(053e5861) SHA1(6740a62cf7b6938a4f936a2fed429704612060a5) ) /* char lookup table */

	ROM_REGION( 0x10000, REGION_SOUND1, 0 )	/* 64k for speech rom */
	ROM_LOAD( "c9_d15.bin",   0x0000, 0x2000, CRC(f546a56b) SHA1(caee3d8546eb7a75ce2a578c6a1a630246aec6b8) )
ROM_END

ROM_START( trackflc )
	ROM_REGION( 2*0x10000, REGION_CPU1, 0 )     /* 64k for code + 64k for decrypted opcodes */
	ROM_LOAD( "f01.1a",       0x6000, 0x2000, CRC(4e32b360) SHA1(cafd4b9ef5548d31d894610dfd2288425d29ed58) )
	ROM_LOAD( "f02.2a",       0x8000, 0x2000, CRC(4e7ebf07) SHA1(266110e5195ab1e374724536b82ec4da35123dc7) )
	ROM_LOAD( "l03.3a",       0xa000, 0x2000, CRC(fef4c0ea) SHA1(c34a0f001de8c06fdb617e20dc335ad99e15df05) )
	ROM_LOAD( "f04.4a",       0xc000, 0x2000, CRC(73940f2d) SHA1(31e0db23ebcf634605f8c232606079ad75e27a66) )
	ROM_LOAD( "f05.5a",       0xe000, 0x2000, CRC(363fd761) SHA1(2b4868813b62c2b7d122e2cb238803eb4687b002) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for the audio CPU */
	ROM_LOAD( "c2_d13.bin",   0x0000, 0x2000, CRC(95bf79b6) SHA1(ea9135acd7ad162c19c5cdde356e69792d61b675) )

	ROM_REGION( 0x6000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "h16_e12.bin",  0x0000, 0x2000, CRC(50075768) SHA1(dfff92c0f59dd3d8d3d6256944bfd48792cef6a9) )
	ROM_LOAD( "h15_e11.bin",  0x2000, 0x2000, CRC(dda9e29f) SHA1(0f41cde82bb60c3f1591ee14dc3cff4642bbddc1) )
	ROM_LOAD( "h14_e10.bin",  0x4000, 0x2000, CRC(c2166a5c) SHA1(5ba25900e653ce4edcf35f1fbce758a327a715ce) )

	ROM_REGION( 0x8000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "c11_d06.bin",  0x0000, 0x2000, CRC(82e2185a) SHA1(1da9ea20e7af0b49c62fb39834a7ec686491af04) )
	ROM_LOAD( "c12_d07.bin",  0x2000, 0x2000, CRC(800ff1f1) SHA1(33d73b18903e3e6bfb30f1a06db4b8105d4040d8) )
	ROM_LOAD( "c13_d08.bin",  0x4000, 0x2000, CRC(d9faf183) SHA1(4448b6242790783d37acf50704d597af5878c2ab) )
	ROM_LOAD( "c14_d09.bin",  0x6000, 0x2000, CRC(5886c802) SHA1(884a12a8f63600da4f23b29be6dbaacef37add20) )

	ROM_REGION( 0x0220, REGION_PROMS, 0 )
	ROM_LOAD( "tfprom.1",     0x0000, 0x0020, CRC(d55f30b5) SHA1(4d6a851f4886778307f75771645078b97ad55f5f) ) /* palette */
	ROM_LOAD( "tfprom.3",     0x0020, 0x0100, CRC(d2ba4d32) SHA1(894b5cedf01ba9225a0d6215291857e455b84903) ) /* sprite lookup table */
	ROM_LOAD( "tfprom.2",     0x0120, 0x0100, CRC(053e5861) SHA1(6740a62cf7b6938a4f936a2fed429704612060a5) ) /* char lookup table */

	ROM_REGION( 0x10000, REGION_SOUND1, 0 )	/* 64k for speech rom */
	ROM_LOAD( "c9_d15.bin",   0x0000, 0x2000, CRC(f546a56b) SHA1(caee3d8546eb7a75ce2a578c6a1a630246aec6b8) )
ROM_END

ROM_START( hyprolym )
	ROM_REGION( 2*0x10000, REGION_CPU1, 0 )     /* 64k for code + 64k for decrypted opcodes */
	ROM_LOAD( "hyprolym.a01", 0x6000, 0x2000, CRC(82257fb7) SHA1(4a5038292e582d5c3b5f2d82b01c57ccb24f3095) )
	ROM_LOAD( "hyprolym.a02", 0x8000, 0x2000, CRC(15b83099) SHA1(79827590d74f20c9a95723e06b05af2b15c34f5f) )
	ROM_LOAD( "hyprolym.a03", 0xa000, 0x2000, CRC(e54cc960) SHA1(7c448c174675271d548ffcf0297ec7a2ae646985) )
	ROM_LOAD( "hyprolym.a04", 0xc000, 0x2000, CRC(d099b1e8) SHA1(0472991ad6caef41ec6b8ec8bf3d9d07584a57cc) )
	ROM_LOAD( "hyprolym.a05", 0xe000, 0x2000, CRC(974ff815) SHA1(11512df2008a79ba44bbb84bd70885f187113211) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )     /* 64k for the audio CPU */
	ROM_LOAD( "c2_d13.bin",   0x0000, 0x2000, CRC(95bf79b6) SHA1(ea9135acd7ad162c19c5cdde356e69792d61b675) )

	ROM_REGION( 0x6000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "hyprolym.h16", 0x0000, 0x2000, CRC(768bb63d) SHA1(effc46615c389245e5a4aac18292e1d764ff0e46) )
	ROM_LOAD( "hyprolym.h15", 0x2000, 0x2000, CRC(3af0e2a8) SHA1(450f35fd7e45ecc88ee80bf57499b2e9f06f6487) )
	ROM_LOAD( "h14_e10.bin",  0x4000, 0x2000, CRC(c2166a5c) SHA1(5ba25900e653ce4edcf35f1fbce758a327a715ce) )

	ROM_REGION( 0x8000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "c11_d06.bin",  0x0000, 0x2000, CRC(82e2185a) SHA1(1da9ea20e7af0b49c62fb39834a7ec686491af04) )
	ROM_LOAD( "c12_d07.bin",  0x2000, 0x2000, CRC(800ff1f1) SHA1(33d73b18903e3e6bfb30f1a06db4b8105d4040d8) )
	ROM_LOAD( "c13_d08.bin",  0x4000, 0x2000, CRC(d9faf183) SHA1(4448b6242790783d37acf50704d597af5878c2ab) )
	ROM_LOAD( "c14_d09.bin",  0x6000, 0x2000, CRC(5886c802) SHA1(884a12a8f63600da4f23b29be6dbaacef37add20) )

	ROM_REGION( 0x0220, REGION_PROMS, 0 )
	ROM_LOAD( "tfprom.1",     0x0000, 0x0020, CRC(d55f30b5) SHA1(4d6a851f4886778307f75771645078b97ad55f5f) ) /* palette */
	ROM_LOAD( "tfprom.3",     0x0020, 0x0100, CRC(d2ba4d32) SHA1(894b5cedf01ba9225a0d6215291857e455b84903) ) /* sprite lookup table */
	ROM_LOAD( "tfprom.2",     0x0120, 0x0100, CRC(053e5861) SHA1(6740a62cf7b6938a4f936a2fed429704612060a5) ) /* char lookup table */

	ROM_REGION( 0x10000, REGION_SOUND1, 0 )	/* 64k for speech rom */
	ROM_LOAD( "c9_d15.bin",   0x0000, 0x2000, CRC(f546a56b) SHA1(caee3d8546eb7a75ce2a578c6a1a630246aec6b8) )
ROM_END

ROM_START( hyprolyb )
	ROM_REGION( 2*0x10000, REGION_CPU1, 0 )     /* 64k for code + 64k for decrypted opcodes */
	ROM_LOAD( "a1.1",         0x6000, 0x2000, CRC(9aee2d5a) SHA1(81f151459f1113b5f2f76ddc140bf86676f778e4) )
	ROM_LOAD( "hyprolym.a02", 0x8000, 0x2000, CRC(15b83099) SHA1(79827590d74f20c9a95723e06b05af2b15c34f5f) )
	ROM_LOAD( "a3.3",         0xa000, 0x2000, CRC(2d6fc308) SHA1(1ff95384670e40d560703f2238998a8e154aa4cf) )
	ROM_LOAD( "hyprolym.a04", 0xc000, 0x2000, CRC(d099b1e8) SHA1(0472991ad6caef41ec6b8ec8bf3d9d07584a57cc) )
	ROM_LOAD( "hyprolym.a05", 0xe000, 0x2000, CRC(974ff815) SHA1(11512df2008a79ba44bbb84bd70885f187113211) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )     /* 64k for the audio CPU */
	ROM_LOAD( "c2_d13.bin",   0x0000, 0x2000, CRC(95bf79b6) SHA1(ea9135acd7ad162c19c5cdde356e69792d61b675) )

	ROM_REGION( 0x10000, REGION_CPU3, 0 )	/*  64k for the 6802 which plays ADPCM samples */
	/* this bootleg uses a 6802 to "emulate" the VLM5030 speech chip */
	/* I didn't bother to emulate the 6802, I just play the samples. */
	ROM_LOAD( "2764.1",       0x8000, 0x2000, CRC(a4cddeb8) SHA1(057981ad3b04239662bb19342e9ec14b0dab2351) )
	ROM_LOAD( "2764.2",       0xa000, 0x2000, CRC(e9919365) SHA1(bd11d6e3ee2c6e698159c2768e315389d666107f) )
	ROM_LOAD( "2764.3",       0xc000, 0x2000, CRC(c3ec42e1) SHA1(048a95726c4f031552e629c3788952c1bc5e7251) )
	ROM_LOAD( "2764.4",       0xe000, 0x2000, CRC(76998389) SHA1(499189b0e20296af88712199b93b958655083608) )

	ROM_REGION( 0x6000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "hyprolym.h16", 0x0000, 0x2000, CRC(768bb63d) SHA1(effc46615c389245e5a4aac18292e1d764ff0e46) )
	ROM_LOAD( "hyprolym.h15", 0x2000, 0x2000, CRC(3af0e2a8) SHA1(450f35fd7e45ecc88ee80bf57499b2e9f06f6487) )
	ROM_LOAD( "h14_e10.bin",  0x4000, 0x2000, CRC(c2166a5c) SHA1(5ba25900e653ce4edcf35f1fbce758a327a715ce) )

	ROM_REGION( 0x8000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "c11_d06.bin",  0x0000, 0x2000, CRC(82e2185a) SHA1(1da9ea20e7af0b49c62fb39834a7ec686491af04) )
	ROM_LOAD( "c12_d07.bin",  0x2000, 0x2000, CRC(800ff1f1) SHA1(33d73b18903e3e6bfb30f1a06db4b8105d4040d8) )
	ROM_LOAD( "c13_d08.bin",  0x4000, 0x2000, CRC(d9faf183) SHA1(4448b6242790783d37acf50704d597af5878c2ab) )
	ROM_LOAD( "c14_d09.bin",  0x6000, 0x2000, CRC(5886c802) SHA1(884a12a8f63600da4f23b29be6dbaacef37add20) )

	ROM_REGION( 0x0220, REGION_PROMS, 0 )
	ROM_LOAD( "tfprom.1",     0x0000, 0x0020, CRC(d55f30b5) SHA1(4d6a851f4886778307f75771645078b97ad55f5f) ) /* palette */
	ROM_LOAD( "tfprom.3",     0x0020, 0x0100, CRC(d2ba4d32) SHA1(894b5cedf01ba9225a0d6215291857e455b84903) ) /* sprite lookup table */
	ROM_LOAD( "tfprom.2",     0x0120, 0x0100, CRC(053e5861) SHA1(6740a62cf7b6938a4f936a2fed429704612060a5) ) /* char lookup table */
ROM_END

ROM_START( mastkin )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )     /* 64k for code */
	ROM_LOAD( "mk3",          0x8000, 0x2000, CRC(9f80d6ae) SHA1(724321d8c3e32d679f8170dfef6555d0179f9d20) )
	ROM_LOAD( "mk4",          0xa000, 0x2000, CRC(99f361e7) SHA1(8706e5c393325c5a89d32388991bc48fa4102779) )
	ROM_LOAD( "mk5",          0xe000, 0x2000, CRC(143d76ce) SHA1(5e5c450e891a11980fb514453f28ffc74a2730ae) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for the audio CPU */
	ROM_LOAD( "mk1",          0x0000, 0x2000, CRC(95bf79b6) SHA1(ea9135acd7ad162c19c5cdde356e69792d61b675) )

	ROM_REGION( 0x6000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "mk12",         0x0000, 0x2000, CRC(8b1a19cf) SHA1(9f75f69828eeaeb2d0dcf20fb80425546124b21e) )
	ROM_LOAD( "mk11",         0x2000, 0x2000, CRC(1a56d24d) SHA1(e64b8a9bdbcf6d2d583ded0750d5f48721785459) )
	ROM_LOAD( "mk10",         0x4000, 0x2000, CRC(e7d05634) SHA1(e7532749fe9b955ba221517807888b34a7754db7) )

	ROM_REGION( 0x8000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "mk6",          0x0000, 0x2000, CRC(18fbe047) SHA1(d2c6aeb1dfd9751f4db386944d144e293001b497) )
	ROM_LOAD( "mk7",          0x2000, 0x2000, CRC(47dee791) SHA1(9c2d5c2ef1e2e8f329160a1c536119b078803347) )
	ROM_LOAD( "mk8",          0x4000, 0x2000, CRC(9c091ead) SHA1(fce50c9d260f20873289921926bd632d6d49ef15) )
	ROM_LOAD( "mk9",          0x6000, 0x2000, CRC(5c8ed3fe) SHA1(a878fcd547aad5388fef9fe2825c1122444c216d) )

	ROM_REGION( 0x0220, REGION_PROMS, 0 )
	ROM_LOAD( "prom.1",       0x0000, 0x0020, NO_DUMP ) /* palette */
	ROM_LOAD( "prom.3",       0x0020, 0x0100, NO_DUMP ) /* sprite lookup table */
	ROM_LOAD( "prom.2",       0x0120, 0x0100, NO_DUMP ) /* char lookup table */

	ROM_REGION( 0x10000, REGION_SOUND1, 0 )	/* 64k for speech rom */
	ROM_LOAD( "mk2",          0x0000, 0x2000, CRC(f546a56b) SHA1(caee3d8546eb7a75ce2a578c6a1a630246aec6b8) )
ROM_END

ROM_START( whizquiz )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )     /* 64k for code */
	ROM_LOAD( "ic9_a1.bin",   0xe000, 0x2000, CRC(608e1ff3) SHA1(f3350a3367df59ec1780bb22c7a6a227e7b10d5e) )	/* encrypted? */

	ROM_REGION( 0x40000, REGION_USER1, 0 )     /* questions data */
	ROM_LOAD( "ic1_q06.bin",  0x00000, 0x8000, CRC(c62f25b1) SHA1(22694716b2675dd0c725ce788bb0ffe7a1808cf6) )
	ROM_LOAD( "ic2_q28.bin",  0x08000, 0x8000, CRC(2bd00476) SHA1(88ed9d26909873c52273290686b4783563edfb61) )
	ROM_LOAD( "ic3_q27.bin",  0x10000, 0x8000, CRC(46d28aaf) SHA1(af19b166eabdab59712eb755ae3d83545ea7db62) )
	ROM_LOAD( "ic4_q23.bin",  0x18000, 0x8000, CRC(3f46f702) SHA1(f41a9ea5a47f2677cea8ad55847860a955521374) )
	ROM_LOAD( "ic5_q26.bin",  0x20000, 0x8000, CRC(9d130515) SHA1(bfc32219d4d4eaca4efa02c3c46125144c8cd286) )
	ROM_LOAD( "ic6_q09.bin",  0x28000, 0x8000, CRC(636f89b4) SHA1(0b9b471e52fff343f9c7e7b1212f03aba52839f2) )
	ROM_LOAD( "ic7_q15.bin",  0x30000, 0x8000, CRC(b35332b1) SHA1(18c5cf3cc6fb6d1fe6d672d745d22b2498d8324e) )
	ROM_LOAD( "ic8_q19.bin",  0x38000, 0x8000, CRC(8d152da0) SHA1(8404256775b6236d80869f5023d912aa9ebb6582) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for the audio CPU */
	ROM_LOAD( "02c.bin",      0x0000, 0x2000, CRC(3daca93a) SHA1(743c2b787aeb2c893ea476efc95d92e33b9bd159) )

	ROM_REGION( 0x6000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "16h.bin",      0x0000, 0x2000, CRC(e6728bda) SHA1(8bd029af5136b0ed6c0087989c69f0b1c23305fb) )
	ROM_LOAD( "15h.bin",      0x2000, 0x2000, CRC(9c067ef4) SHA1(2a66beee4fa76d40ca18637c0061b196d3873df3) )
	ROM_LOAD( "14h.bin",      0x4000, 0x2000, CRC(3bbad920) SHA1(f5c491f37aa6855181c62fe6bb2975c7d011cc72) )

	ROM_REGION( 0x4000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "11c.bin",      0x0000, 0x2000, CRC(87d060d4) SHA1(22da2dfaf71d78a4789ca34c27571733ab65ea30) )
	ROM_LOAD( "14c.bin",      0x2000, 0x2000, CRC(5bff1607) SHA1(20c4b74c93511f9cafd6e3f2d048baad3a3a8aa4) )

	ROM_REGION( 0x0220, REGION_PROMS, 0 )
	ROM_LOAD( "prom.1",       0x0000, 0x0020, NO_DUMP ) /* palette */
	ROM_LOAD( "prom.3",       0x0020, 0x0100, NO_DUMP ) /* sprite lookup table */
	ROM_LOAD( "prom.2",       0x0120, 0x0100, NO_DUMP ) /* char lookup table */
ROM_END


static DRIVER_INIT( trackfld )
{
	konami1_decode();
}

static DRIVER_INIT( mastkin )
{
	UINT8 *prom = memory_region(REGION_PROMS);
	int i;

	/* We don't have the colour PROMs. */
	/* So we try and put together a palette that works. */
	/* It is not the original, as no one seems to have screenshots */
	/* or flyers or video of the original machine in action. */
	/* I suspect the sky was originally red, but it looks odd */
	/* so I've gone with light blue. */
	prom[0] = 0;	/* ?? */
	prom[1] = 246;	/* Monster horn + part of fire breath + part of bullets + arms of queen */
	prom[2] = 246;	/* Your skin + bat head */
	prom[3] = 12;	/* Front of your trousers + death explosion */
	prom[4] = 255;	/* Sword (half) */
	prom[5] = 110;	/* Your hair + enemy face + top of your boots */
	prom[6] = 12;	/* Back of your body + bat body */
	prom[7] = 28;	/* Back of your trousers */
	prom[8] = 11;	/* Front of enemy trousers */
	prom[9] = 246;	/* Your hands and forehead */
	prom[10] = 11;	/* Your front of body + bat wings + enemy hair + enemy feet */
	prom[11] = 10;	/* Your boots + enemy arms + hilt of your sword */
	prom[12] = 4;	/* Queen's eye + flame from monsters */
	prom[13] = 2;	/* Your eyes + patch on arm + body of enemy + back of enemy trousers */
	prom[14] = 14;	/* Your shoulders */
	prom[15] = 254;	/* Your sword (middle) + spot on enemy neck */
	prom[16] = 0;	/* Title screen background + slight part of sticks + minor dirt lines */
	prom[17] = 10;	/* Highlight on columns + top of sticks + title text + score digits + timer lines + lives handle */
	prom[18] = 11;	/* Title columns left + highlight on bricks + major dirt lines + high score text */
	prom[19] = 2;	/* Title columns main + title dirt + major bricks + major sticks + major dirt lines + house tops in background */
	prom[20] = 1;	/* Column shadows + other highlight on bricks + background house colour + more dirt lines */
	prom[21] = 90;	/* The Masters Of text + little stones on road + top of mountains + high score digits */
	prom[22] = 16;	/* Written By text + hills + road minus stones */
	prom[23] = 25;	/* More stones on road + lines on hill + square around Written By text */
	prom[24] = 20;	/* Brick grouting + progress meter highlight + score text */
	prom[25] = 11;	/* Brick wall edge highlight + sword highlight on lives */
	prom[26] = 2;	/* Top of brick wall + highlight on lives */
	prom[27] = 2;	/* Credit text + progress highlight */
	prom[28] = 11;	/* Bottom of life meter + hilt of lives */
	prom[29] = 240;	/* Sky + bottom of life meter */
	prom[30] = 11;	/* Surround of timer */
	prom[31] = 4;	/* Top of life meter */

	/* build a fake lookup table since we don't have the color PROMs */
	for (i = 0; i < 0x0200; i++)
	{
		if ((i & 0x0f) == 0)
			prom[i + 0x20] = 0;
		else
			prom[i + 0x20] = (i + i / 16) & 0x0f;
	}
}


GAME( 1983, trackfld, 0,        trackfld, trackfld, trackfld, ROT0, "Konami", "Track and Field" )
GAME( 1983, trackflc, trackfld, trackfld, trackfld, trackfld, ROT0, "Konami (Centuri license)", "Track and Field (Centuri)" )
GAME( 1983, hyprolym, trackfld, trackfld, trackfld, trackfld, ROT0, "Konami", "Hyper Olympic" )
GAME( 1983, hyprolyb, trackfld, hyprolyb, trackfld, trackfld, ROT0, "bootleg", "Hyper Olympic (bootleg)" )
GAMEX(1985, whizquiz, 0,        trackfld, trackfld, mastkin,  ROT0, "Zilec-Zenitone", "Whiz Quiz", GAME_NOT_WORKING )
GAMEX(1988, mastkin,  0,        mastkin,  mastkin,  mastkin,  ROT0, "Du Tech", "The Masters of Kin", GAME_WRONG_COLORS )
