/****************************************************************************

Royal Mahjong (c) 1982 Falcon
and many other Dyna/Dynax games running in similar bare-bones hardware

driver by Zsolt Vasvari and Nicola Salmoria


Hardware specs are the same for all Dyna/Dynax games:

CPU:	Z80
Sound:	AY-3-8910
OSC:	18.432MHz and 8MHz

Mahjong If doesn't use a Z80, it probably uses a Toshiba TLCS-90 which is
similar to Z80 but with different opcodes

-------------------------------------------------------------------------------------------
Year + Game					Board			Company				Notes
-------------------------------------------------------------------------------------------
82 Royal Mahjong			FRM-03			Falcon
86 Watashiha Suzumechan						Dyna Electronics
86 Don Den Mahjong			D039198L-0		Dyna Electronics
87 Mahjong Diplomat			D0706088L1-0	Dynax
87 Tonton					D0908288L1-0	Dynax
87 Mahjong Studio 101		D1708228L1		Dynax
89 Mahjong Derringer		D2203018L		Dynax				Larger palette
90 Mahjong If				D29?			Dynax				Larger palette, unsupported CPU
-------------------------------------------------------------------------------------------

TODO:
- dip switches and inputs in dondenmj, suzume, mjderngr...

- suzume: coins are not recognized, but the input does work in service mode.

- there's something fishy with the bank switching in tontonb/mjdiplob

- majs101b: service mode doesn't work


Stephh's notes (based on the games Z80 code and some tests) :

1) 'royalmah'

  - COIN1 doesn't work correctly, the screen goes black instead of showing the
    credits, and you can start a game but the "phantom" credit is not subtracted;
    with NVRAM support, this means the game would always boot to a black screen.
  - The doesn't seem to be any possibility to play a 2 players game
    (but the inputs are mapped so you can test them in the "test mode").
    P1 IN4 doesn't seem to be needed outside the "test mode" either.

2) 'tontonb'

  - The doesn't seem to be any possibility to play a 2 players game
    (but the inputs are mapped so you can test them in the "test mode")
    P1 IN4 doesn't seem to be needed outside the "test mode" either.

  - I've DELIBERATELY mapped DSW3 before DSW2 to try to spot the common
    things with the other Dynax mahjong games ! Please don't change this !

  - When "Special Combinaisons" Dip Switch is ON, there is a marker in
    front of a random combinaison. It's value is *2 then.

3) 'mjdiplob'

  - The doesn't seem to be any possibility to play a 2 players game
    (but the inputs are mapped so you can test them in the "test mode")
    P1 IN4 doesn't seem to be needed outside the "test mode" either.

  - When "Special Combinaisons" Dip Switch is ON, there is a marker in
    front of a random combinaison. It's value remains *1 though.
    Could it be a leftover from another game ('tontonb' for exemple) ?

****************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"


static int palette_base;


PALETTE_INIT( royalmah )
{
	int i;

	for (i = 0;i < Machine->drv->total_colors;i++)
	{
		int bit0,bit1,bit2,r,g,b;


		/* red component */
		bit0 = (*color_prom >> 0) & 0x01;
		bit1 = (*color_prom >> 1) & 0x01;
		bit2 = (*color_prom >> 2) & 0x01;
		r = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;
		/* green component */
		bit0 = (*color_prom >> 3) & 0x01;
		bit1 = (*color_prom >> 4) & 0x01;
		bit2 = (*color_prom >> 5) & 0x01;
		g = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;
		/* blue component */
		bit0 = 0;
		bit1 = (*color_prom >> 6) & 0x01;
		bit2 = (*color_prom >> 7) & 0x01;
		b = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;

		palette_set_color(i,r,g,b);
		color_prom++;
	}
}

/* 0 B01234 G01234 R01234 */
PALETTE_INIT( mjderngr )
{
	int i;

	for (i = 0;i < Machine->drv->total_colors;i++)
	{
		int x =	(color_prom[i]<<8) + color_prom[0x200+i];
		/* The bits are in reverse order! */
		int r = BITSWAP8((x >>  0) & 0x1f, 7,6,5, 0,1,2,3,4 );
		int g = BITSWAP8((x >>  5) & 0x1f, 7,6,5, 0,1,2,3,4 );
		int b = BITSWAP8((x >> 10) & 0x1f, 7,6,5, 0,1,2,3,4 );
		r =  (r << 3) | (r >> 2);
		g =  (g << 3) | (g >> 2);
		b =  (b << 3) | (b >> 2);
		palette_set_color(i,r,g,b);
	}
}


WRITE_HANDLER( royalmah_videoram_w )
{
	int i;
	UINT8 x, y;
	UINT8 col1, col2;


	videoram[offset] = data;

	col1 = videoram[offset & 0x3fff];
	col2 = videoram[offset | 0x4000];

	y = (offset >> 6);
	x = (offset & 0x3f) << 2;

	for (i = 0; i < 4; i++)
	{
		int col = ((col1 & 0x01) >> 0) | ((col1 & 0x10) >> 3) | ((col2 & 0x01) << 2) | ((col2 & 0x10) >> 1);

		plot_pixel(tmpbitmap, (x+i) ^ 0xff, y ^ 0xff, 16*palette_base + col );

		col1 >>= 1;
		col2 >>= 1;
	}
}


WRITE_HANDLER( royalmah_palbank_w )
{
	/* bit 1 = coin counter */
	coin_counter_w(0,data & 2);

	/* bit 2 always set? */

	/* bit 3 = palette bank */
	set_vh_global_attribute(&palette_base,(data & 0x08) >> 3);
}

WRITE_HANDLER( mjderngr_coin_w )
{
	/* bit 1 = coin counter */
	coin_counter_w(0,data & 2);

	/* bit 2 always set? */
}

WRITE_HANDLER( mjderngr_palbank_w )
{
	set_vh_global_attribute(&palette_base,data);
}


VIDEO_UPDATE( royalmah )
{
	if (get_vh_global_attribute_changed())
	{
		int offs;

		/* redraw bitmap */

		for (offs = 0; offs < videoram_size; offs++)
		{
			royalmah_videoram_w(offs, videoram[offs]);
		}
	}
	copybitmap(bitmap,tmpbitmap,0,0,0,0,&Machine->visible_area,TRANSPARENCY_NONE,0);
}




static WRITE_HANDLER( royalmah_rom_w )
{
	/* using this handler will avoid all the entries in the error log that are the result of
	   the RLD and RRD instructions this games uses to print text on the screen */
}


static int royalmah_input_port_select;
static int majs101b_dsw_select;

static WRITE_HANDLER( royalmah_input_port_select_w )
{
	royalmah_input_port_select = data;
}

static READ_HANDLER( royalmah_player_1_port_r )
{
	int ret = (input_port_0_r(offset) & 0xc0) | 0x3f;

	if ((royalmah_input_port_select & 0x01) == 0)  ret &= input_port_0_r(offset);
	if ((royalmah_input_port_select & 0x02) == 0)  ret &= input_port_1_r(offset);
	if ((royalmah_input_port_select & 0x04) == 0)  ret &= input_port_2_r(offset);
	if ((royalmah_input_port_select & 0x08) == 0)  ret &= input_port_3_r(offset);
	if ((royalmah_input_port_select & 0x10) == 0)  ret &= input_port_4_r(offset);

	return ret;
}

static READ_HANDLER( royalmah_player_2_port_r )
{
	int ret = (input_port_5_r(offset) & 0xc0) | 0x3f;

	if ((royalmah_input_port_select & 0x01) == 0)  ret &= input_port_5_r(offset);
	if ((royalmah_input_port_select & 0x02) == 0)  ret &= input_port_6_r(offset);
	if ((royalmah_input_port_select & 0x04) == 0)  ret &= input_port_7_r(offset);
	if ((royalmah_input_port_select & 0x08) == 0)  ret &= input_port_8_r(offset);
	if ((royalmah_input_port_select & 0x10) == 0)  ret &= input_port_9_r(offset);

	return ret;
}



static READ_HANDLER ( majs101b_dsw_r )
{
	switch (majs101b_dsw_select)
	{
		case 0x00: return readinputport(13);	/* DSW3 */
		case 0x20: return readinputport(14);	/* DSW4 */
		case 0x40: return readinputport(12);	/* DSW2 */
	}
	return 0;
}


static data8_t suzume_bank;

static READ_HANDLER ( suzume_dsw_r )
{
	if (suzume_bank & 0x40)
	{
		return suzume_bank;
	}
	else
	{
		switch (suzume_bank)
		{
			case 0x08: return readinputport(14);	/* DSW4 */
			case 0x10: return readinputport(13);	/* DSW3 */
			case 0x18: return readinputport(12);	/* DSW2 */
		}
		return 0;
	}
}

static WRITE_HANDLER ( suzume_bank_w )
{
	data8_t *rom = memory_region(REGION_CPU1);
	int address;

	suzume_bank = data;

log_cb(RETRO_LOG_DEBUG, LOGPRE "%04x: bank %02x\n",activecpu_get_pc(),data);

	/* bits 6, 4 and 3 used for something input related? */

	address = 0x10000 + (data & 0x07) * 0x8000;
	cpu_setbank(1,&rom[address]);
}


static WRITE_HANDLER ( tontonb_bank_w )
{
	data8_t *rom = memory_region(REGION_CPU1);
	int address;

log_cb(RETRO_LOG_DEBUG, LOGPRE "%04x: bank %02x\n",activecpu_get_pc(),data);

	if (data == 0) return;	/* tontonb fix?*/

	data &= 0x0f;

	address = 0x10000 + data * 0x8000;

	cpu_setbank(1,&rom[address]);
}


/* bits 5 and 6 seem to affect which Dip Switch to read in 'majs101b' */
static WRITE_HANDLER ( dynax_bank_w )
{
	data8_t *rom = memory_region(REGION_CPU1);
	int address;

/*logerror("%04x: bank %02x\n",activecpu_get_pc(),data);*/

	majs101b_dsw_select = data & 0x60;

	data &= 0x1f;

	address = 0x10000 + data * 0x8000;

	cpu_setbank(1,&rom[address]);
}





static MEMORY_READ_START( readmem )
	{ 0x0000, 0x6fff, MRA_ROM },
	{ 0x7000, 0x7fff, MRA_RAM },
	{ 0x8000, 0xffff, MRA_BANK1 },	/* banked ROMs not present in royalmah*/
MEMORY_END

static MEMORY_WRITE_START( writemem )
	{ 0x0000, 0x6fff, royalmah_rom_w },
	{ 0x7000, 0x7fff, MWA_RAM, &generic_nvram, &generic_nvram_size },
	{ 0x8000, 0xffff, royalmah_videoram_w, &videoram, &videoram_size },
MEMORY_END


static PORT_READ_START( royalmah_readport )
	{ 0x01, 0x01, AY8910_read_port_0_r },
	{ 0x10, 0x10, input_port_11_r },
	{ 0x11, 0x11, input_port_10_r },
PORT_END

static PORT_WRITE_START( royalmah_writeport )
	{ 0x02, 0x02, AY8910_write_port_0_w },
	{ 0x03, 0x03, AY8910_control_port_0_w },
	{ 0x10, 0x10, royalmah_palbank_w },
	{ 0x11, 0x11, royalmah_input_port_select_w },
PORT_END

static PORT_READ_START( suzume_readport )
	{ 0x01, 0x01, AY8910_read_port_0_r },
	{ 0x10, 0x10, input_port_11_r },
	{ 0x11, 0x11, input_port_10_r },
	{ 0x80, 0x80, suzume_dsw_r },
PORT_END

static PORT_WRITE_START( suzume_writeport )
	{ 0x02, 0x02, AY8910_write_port_0_w },
	{ 0x03, 0x03, AY8910_control_port_0_w },
	{ 0x10, 0x10, royalmah_palbank_w },
	{ 0x11, 0x11, royalmah_input_port_select_w },
	{ 0x81, 0x81, suzume_bank_w },
PORT_END

static PORT_READ_START( dondenmj_readport )
	{ 0x01, 0x01, AY8910_read_port_0_r },
	{ 0x10, 0x10, input_port_11_r },
	{ 0x11, 0x11, input_port_10_r },
	{ 0x85, 0x85, input_port_12_r },	/* DSW2*/
	{ 0x86, 0x86, input_port_13_r },	/* DSW3*/
PORT_END

static PORT_WRITE_START( dondenmj_writeport )
	{ 0x02, 0x02, AY8910_write_port_0_w },
	{ 0x03, 0x03, AY8910_control_port_0_w },
	{ 0x10, 0x10, royalmah_palbank_w },
	{ 0x11, 0x11, royalmah_input_port_select_w },
	{ 0x87, 0x87, dynax_bank_w },
PORT_END

static PORT_READ_START( mjdiplob_readport )
	{ 0x01, 0x01, AY8910_read_port_0_r },
	{ 0x10, 0x10, input_port_11_r },
	{ 0x11, 0x11, input_port_10_r },
	{ 0x62, 0x62, input_port_12_r },	/* DSW2*/
	{ 0x63, 0x63, input_port_13_r },	/* DSW3*/
PORT_END

static PORT_WRITE_START( mjdiplob_writeport )
	{ 0x02, 0x02, AY8910_write_port_0_w },
	{ 0x03, 0x03, AY8910_control_port_0_w },
	{ 0x10, 0x10, royalmah_palbank_w },
	{ 0x11, 0x11, royalmah_input_port_select_w },
	{ 0x61, 0x61, tontonb_bank_w },
PORT_END

static PORT_READ_START( tontonb_readport )
	{ 0x01, 0x01, AY8910_read_port_0_r },
	{ 0x10, 0x10, input_port_11_r },
	{ 0x11, 0x11, input_port_10_r },
	{ 0x46, 0x46, input_port_13_r },	/* DSW2*/
	{ 0x47, 0x47, input_port_12_r },	/* DSW3*/
PORT_END

static PORT_WRITE_START( tontonb_writeport )
	{ 0x02, 0x02, AY8910_write_port_0_w },
	{ 0x03, 0x03, AY8910_control_port_0_w },
	{ 0x10, 0x10, royalmah_palbank_w },
	{ 0x11, 0x11, royalmah_input_port_select_w },
	{ 0x44, 0x44, tontonb_bank_w },
PORT_END

static PORT_READ_START( majs101b_readport )
	{ 0x01, 0x01, AY8910_read_port_0_r },
	{ 0x10, 0x10, input_port_11_r },
	{ 0x11, 0x11, input_port_10_r },
	{ 0x00, 0x00, majs101b_dsw_r },
PORT_END

static PORT_WRITE_START( majs101b_writeport )
	{ 0x02, 0x02, AY8910_write_port_0_w },
	{ 0x03, 0x03, AY8910_control_port_0_w },
	{ 0x10, 0x10, royalmah_palbank_w },
	{ 0x11, 0x11, royalmah_input_port_select_w },
	{ 0x00, 0x00, dynax_bank_w },
PORT_END

static PORT_READ_START( mjderngr_readport )
	{ 0x01, 0x01, AY8910_read_port_0_r },
/*	{ 0x10, 0x10, input_port_11_r },*/
	{ 0x11, 0x11, input_port_10_r },
	{ 0x40, 0x40, input_port_13_r },	/* DSW2*/
	{ 0x4c, 0x4c, input_port_12_r },	/* DSW3*/
PORT_END

static PORT_WRITE_START( mjderngr_writeport )
	{ 0x02, 0x02, AY8910_write_port_0_w },
	{ 0x03, 0x03, AY8910_control_port_0_w },
	{ 0x10, 0x10, mjderngr_coin_w },	/* palette bank is set separately*/
	{ 0x11, 0x11, royalmah_input_port_select_w },
	{ 0x20, 0x20, dynax_bank_w },
	{ 0x60, 0x60, mjderngr_palbank_w },
PORT_END



INPUT_PORTS_START( royalmah )
	PORT_START	/* P1 IN0 */
	PORT_BITX(0x01, IP_ACTIVE_LOW, 0, "P1 A",            KEYCODE_A,         IP_JOY_NONE )
	PORT_BITX(0x02, IP_ACTIVE_LOW, 0, "P1 E",            KEYCODE_E,         IP_JOY_NONE )
	PORT_BITX(0x04, IP_ACTIVE_LOW, 0, "P1 I",            KEYCODE_I,         IP_JOY_NONE )
	PORT_BITX(0x08, IP_ACTIVE_LOW, 0, "P1 M",            KEYCODE_M,         IP_JOY_NONE )
	PORT_BITX(0x10, IP_ACTIVE_LOW, 0, "P1 Kan",          KEYCODE_LCONTROL,  IP_JOY_NONE )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BITX(0x40, IP_ACTIVE_LOW, 0, "P1 Credit Clear", KEYCODE_7,  IP_JOY_NONE )
	PORT_BITX(0x80, IP_ACTIVE_LOW, 0, "P2 Credit Clear", KEYCODE_8,  IP_JOY_NONE )

	PORT_START	/* P1 IN1 */
	PORT_BITX(0x01, IP_ACTIVE_LOW, 0, "P1 B",            KEYCODE_B,         IP_JOY_NONE )
	PORT_BITX(0x02, IP_ACTIVE_LOW, 0, "P1 F",            KEYCODE_F,         IP_JOY_NONE )
	PORT_BITX(0x04, IP_ACTIVE_LOW, 0, "P1 J",            KEYCODE_J,         IP_JOY_NONE )
	PORT_BITX(0x08, IP_ACTIVE_LOW, 0, "P1 N",            KEYCODE_N,         IP_JOY_NONE )
	PORT_BITX(0x10, IP_ACTIVE_LOW, 0, "P1 Reach",        KEYCODE_LSHIFT,    IP_JOY_NONE )
	PORT_BITX(0x20, IP_ACTIVE_LOW, 0, "P1 Bet",          KEYCODE_3,         IP_JOY_NONE )
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* P1 IN2 */
	PORT_BITX(0x01, IP_ACTIVE_LOW, 0, "P1 C",            KEYCODE_C,         IP_JOY_NONE )
	PORT_BITX(0x02, IP_ACTIVE_LOW, 0, "P1 G",            KEYCODE_G,         IP_JOY_NONE )
	PORT_BITX(0x04, IP_ACTIVE_LOW, 0, "P1 K",            KEYCODE_K,         IP_JOY_NONE )
	PORT_BITX(0x08, IP_ACTIVE_LOW, 0, "P1 Chi",          KEYCODE_SPACE,     IP_JOY_NONE )
	PORT_BITX(0x10, IP_ACTIVE_LOW, 0, "P1 Ron",          KEYCODE_Z,         IP_JOY_NONE )
	PORT_BIT( 0xe0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* P1 IN3 */
	PORT_BITX(0x01, IP_ACTIVE_LOW, 0, "P1 D",            KEYCODE_D,         IP_JOY_NONE )
	PORT_BITX(0x02, IP_ACTIVE_LOW, 0, "P1 H",            KEYCODE_H,         IP_JOY_NONE )
	PORT_BITX(0x04, IP_ACTIVE_LOW, 0, "P1 L",            KEYCODE_L,         IP_JOY_NONE )
	PORT_BITX(0x08, IP_ACTIVE_LOW, 0, "P1 Pon",          KEYCODE_LALT,      IP_JOY_NONE )
	PORT_BIT( 0xf0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* P1 IN4 */
	PORT_BITX(0x01, IP_ACTIVE_LOW, 0, "P1 Last Chance",  KEYCODE_RALT,      IP_JOY_NONE )
	PORT_BITX(0x02, IP_ACTIVE_LOW, 0, "P1 Take Score",   KEYCODE_RCONTROL,  IP_JOY_NONE )
	PORT_BITX(0x04, IP_ACTIVE_LOW, 0, "P1 Double Up",    KEYCODE_RSHIFT,    IP_JOY_NONE )
	PORT_BITX(0x08, IP_ACTIVE_LOW, 0, "P1 Flip Flop",    KEYCODE_X,         IP_JOY_NONE )
	PORT_BITX(0x10, IP_ACTIVE_LOW, 0, "P1 Big",          KEYCODE_ENTER,     IP_JOY_NONE )
	PORT_BITX(0x20, IP_ACTIVE_LOW, 0, "P1 Small",        KEYCODE_BACKSPACE, IP_JOY_NONE )
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* P2 IN0 */
	PORT_BITX(0x01, IP_ACTIVE_LOW, 0, "P2 A",            IP_KEY_DEFAULT,    IP_JOY_NONE )
	PORT_BITX(0x02, IP_ACTIVE_LOW, 0, "P2 E",            IP_KEY_DEFAULT,    IP_JOY_NONE )
	PORT_BITX(0x04, IP_ACTIVE_LOW, 0, "P2 I",            IP_KEY_DEFAULT,    IP_JOY_NONE )
	PORT_BITX(0x08, IP_ACTIVE_LOW, 0, "P2 M",            IP_KEY_DEFAULT,    IP_JOY_NONE )
	PORT_BITX(0x10, IP_ACTIVE_LOW, 0, "P2 Kan",          IP_KEY_DEFAULT,    IP_JOY_NONE )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN1 )	/* "COIN2"*/
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN2 )	/* "COIN1", but not working*/

	PORT_START	/* P2 IN1 */
	PORT_BITX(0x01, IP_ACTIVE_LOW, 0, "P2 B",            IP_KEY_DEFAULT,    IP_JOY_NONE )
	PORT_BITX(0x02, IP_ACTIVE_LOW, 0, "P2 F",            IP_KEY_DEFAULT,    IP_JOY_NONE )
	PORT_BITX(0x04, IP_ACTIVE_LOW, 0, "P2 J",            IP_KEY_DEFAULT,    IP_JOY_NONE )
	PORT_BITX(0x08, IP_ACTIVE_LOW, 0, "P2 N",            IP_KEY_DEFAULT,    IP_JOY_NONE )
	PORT_BITX(0x10, IP_ACTIVE_LOW, 0, "P2 Reach",        IP_KEY_DEFAULT,    IP_JOY_NONE )
	PORT_BITX(0x20, IP_ACTIVE_LOW, 0, "P2 Bet",          KEYCODE_4,         IP_JOY_NONE )
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* P2 IN2 */
	PORT_BITX(0x01, IP_ACTIVE_LOW, 0, "P2 C",            IP_KEY_DEFAULT,    IP_JOY_NONE )
	PORT_BITX(0x02, IP_ACTIVE_LOW, 0, "P2 G",            IP_KEY_DEFAULT,    IP_JOY_NONE )
	PORT_BITX(0x04, IP_ACTIVE_LOW, 0, "P2 K",            IP_KEY_DEFAULT,    IP_JOY_NONE )
	PORT_BITX(0x08, IP_ACTIVE_LOW, 0, "P2 Chi",          IP_KEY_DEFAULT,    IP_JOY_NONE )
	PORT_BITX(0x10, IP_ACTIVE_LOW, 0, "P2 Ron",          IP_KEY_DEFAULT,    IP_JOY_NONE )
	PORT_BIT( 0xe0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* P2 IN3 */
	PORT_BITX(0x01, IP_ACTIVE_LOW, 0, "P2 D",            IP_KEY_DEFAULT,    IP_JOY_NONE )
	PORT_BITX(0x02, IP_ACTIVE_LOW, 0, "P2 H",            IP_KEY_DEFAULT,    IP_JOY_NONE )
	PORT_BITX(0x04, IP_ACTIVE_LOW, 0, "P2 L",            IP_KEY_DEFAULT,    IP_JOY_NONE )
	PORT_BITX(0x08, IP_ACTIVE_LOW, 0, "P2 Pon",          IP_KEY_DEFAULT,    IP_JOY_NONE )
	PORT_BIT( 0xf0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* P2 IN4 */
	PORT_BITX(0x01, IP_ACTIVE_LOW, 0, "P2 Last Chance",  IP_KEY_DEFAULT,    IP_JOY_NONE )
	PORT_BITX(0x02, IP_ACTIVE_LOW, 0, "P2 Take Score",   IP_KEY_DEFAULT,    IP_JOY_NONE )
	PORT_BITX(0x04, IP_ACTIVE_LOW, 0, "P2 Double Up",    IP_KEY_DEFAULT,    IP_JOY_NONE )
	PORT_BITX(0x08, IP_ACTIVE_LOW, 0, "P2 Flip Flop",    IP_KEY_DEFAULT,    IP_JOY_NONE )
	PORT_BITX(0x10, IP_ACTIVE_LOW, 0, "P2 Big",          IP_KEY_DEFAULT,    IP_JOY_NONE )
	PORT_BITX(0x20, IP_ACTIVE_LOW, 0, "P2 Small",        IP_KEY_DEFAULT,    IP_JOY_NONE )
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* IN10 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_SERVICE1 )	/* "Note" ("Paper Money") = 10 Credits */
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_SERVICE3 )	/* Memory Reset */
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_SERVICE2 )	/* Analizer (Statistics) */
	PORT_SERVICE( 0x08, IP_ACTIVE_HIGH )
	PORT_BIT( 0xf0, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START	/* DSW  (inport $10) */
	PORT_DIPNAME( 0x0f, 0x0f, "Pay Out Rate" )
	PORT_DIPSETTING(    0x0f, "96%" )
	PORT_DIPSETTING(    0x0e, "93%" )
	PORT_DIPSETTING(    0x0d, "90%" )
	PORT_DIPSETTING(    0x0c, "87%" )
	PORT_DIPSETTING(    0x0b, "84%" )
	PORT_DIPSETTING(    0x0a, "81%" )
	PORT_DIPSETTING(    0x09, "78%" )
	PORT_DIPSETTING(    0x08, "75%" )
	PORT_DIPSETTING(    0x07, "71%" )
	PORT_DIPSETTING(    0x06, "68%" )
	PORT_DIPSETTING(    0x05, "65%" )
	PORT_DIPSETTING(    0x04, "62%" )
	PORT_DIPSETTING(    0x03, "59%" )
	PORT_DIPSETTING(    0x02, "56%" )
	PORT_DIPSETTING(    0x01, "53%" )
	PORT_DIPSETTING(    0x00, "50%" )
	PORT_DIPNAME( 0x30, 0x30, "Maximum Bet" )
	PORT_DIPSETTING(    0x00, "1" )
	PORT_DIPSETTING(    0x10, "5" )
	PORT_DIPSETTING(    0x20, "10" )
	PORT_DIPSETTING(    0x30, "20" )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x80, DEF_STR( On ) )
INPUT_PORTS_END

INPUT_PORTS_START( tontonb )
	PORT_START	/* P1 IN0 */
	PORT_BITX(0x01, IP_ACTIVE_LOW, 0, "P1 A",            KEYCODE_A,         IP_JOY_NONE )
	PORT_BITX(0x02, IP_ACTIVE_LOW, 0, "P1 E",            KEYCODE_E,         IP_JOY_NONE )
	PORT_BITX(0x04, IP_ACTIVE_LOW, 0, "P1 I",            KEYCODE_I,         IP_JOY_NONE )
	PORT_BITX(0x08, IP_ACTIVE_LOW, 0, "P1 M",            KEYCODE_M,         IP_JOY_NONE )
	PORT_BITX(0x10, IP_ACTIVE_LOW, 0, "P1 Kan",          KEYCODE_LCONTROL,  IP_JOY_NONE )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BITX(0x40, IP_ACTIVE_LOW, 0, "Payout",          KEYCODE_7,  IP_JOY_NONE )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* P1 IN1 */
	PORT_BITX(0x01, IP_ACTIVE_LOW, 0, "P1 B",            KEYCODE_B,         IP_JOY_NONE )
	PORT_BITX(0x02, IP_ACTIVE_LOW, 0, "P1 F",            KEYCODE_F,         IP_JOY_NONE )
	PORT_BITX(0x04, IP_ACTIVE_LOW, 0, "P1 J",            KEYCODE_J,         IP_JOY_NONE )
	PORT_BITX(0x08, IP_ACTIVE_LOW, 0, "P1 N",            KEYCODE_N,         IP_JOY_NONE )
	PORT_BITX(0x10, IP_ACTIVE_LOW, 0, "P1 Reach",        KEYCODE_LSHIFT,    IP_JOY_NONE )
	PORT_BITX(0x20, IP_ACTIVE_LOW, 0, "P1 Bet",          KEYCODE_3,         IP_JOY_NONE )
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* P1 IN2 */
	PORT_BITX(0x01, IP_ACTIVE_LOW, 0, "P1 C",            KEYCODE_C,         IP_JOY_NONE )
	PORT_BITX(0x02, IP_ACTIVE_LOW, 0, "P1 G",            KEYCODE_G,         IP_JOY_NONE )
	PORT_BITX(0x04, IP_ACTIVE_LOW, 0, "P1 K",            KEYCODE_K,         IP_JOY_NONE )
	PORT_BITX(0x08, IP_ACTIVE_LOW, 0, "P1 Chi",          KEYCODE_SPACE,     IP_JOY_NONE )
	PORT_BITX(0x10, IP_ACTIVE_LOW, 0, "P1 Ron",          KEYCODE_Z,         IP_JOY_NONE )
	PORT_BIT( 0xe0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* P1 IN3 */
	PORT_BITX(0x01, IP_ACTIVE_LOW, 0, "P1 D",            KEYCODE_D,         IP_JOY_NONE )
	PORT_BITX(0x02, IP_ACTIVE_LOW, 0, "P1 H",            KEYCODE_H,         IP_JOY_NONE )
	PORT_BITX(0x04, IP_ACTIVE_LOW, 0, "P1 L",            KEYCODE_L,         IP_JOY_NONE )
	PORT_BITX(0x08, IP_ACTIVE_LOW, 0, "P1 Pon",          KEYCODE_LALT,      IP_JOY_NONE )
	PORT_BIT( 0xf0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* P1 IN4 */
	PORT_BITX(0x01, IP_ACTIVE_LOW, 0, "P1 Last Chance",  KEYCODE_RALT,      IP_JOY_NONE )
	PORT_BITX(0x02, IP_ACTIVE_LOW, 0, "P1 Take Score",   KEYCODE_RCONTROL,  IP_JOY_NONE )
	PORT_BITX(0x04, IP_ACTIVE_LOW, 0, "P1 Double Up",    KEYCODE_RSHIFT,    IP_JOY_NONE )
	PORT_BITX(0x08, IP_ACTIVE_LOW, 0, "P1 Flip Flop",    KEYCODE_X,         IP_JOY_NONE )
	PORT_BITX(0x10, IP_ACTIVE_LOW, 0, "P1 Big",          KEYCODE_ENTER,     IP_JOY_NONE )
	PORT_BITX(0x20, IP_ACTIVE_LOW, 0, "P1 Small",        KEYCODE_BACKSPACE, IP_JOY_NONE )
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* P2 IN0 */
	PORT_BITX(0x01, IP_ACTIVE_LOW, 0, "P2 A",            IP_KEY_DEFAULT,    IP_JOY_NONE )
	PORT_BITX(0x02, IP_ACTIVE_LOW, 0, "P2 E",            IP_KEY_DEFAULT,    IP_JOY_NONE )
	PORT_BITX(0x04, IP_ACTIVE_LOW, 0, "P2 I",            IP_KEY_DEFAULT,    IP_JOY_NONE )
	PORT_BITX(0x08, IP_ACTIVE_LOW, 0, "P2 M",            IP_KEY_DEFAULT,    IP_JOY_NONE )
	PORT_BITX(0x10, IP_ACTIVE_LOW, 0, "P2 Kan",          IP_KEY_DEFAULT,    IP_JOY_NONE )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* P2 IN1 */
	PORT_BITX(0x01, IP_ACTIVE_LOW, 0, "P2 B",            IP_KEY_DEFAULT,    IP_JOY_NONE )
	PORT_BITX(0x02, IP_ACTIVE_LOW, 0, "P2 F",            IP_KEY_DEFAULT,    IP_JOY_NONE )
	PORT_BITX(0x04, IP_ACTIVE_LOW, 0, "P2 J",            IP_KEY_DEFAULT,    IP_JOY_NONE )
	PORT_BITX(0x08, IP_ACTIVE_LOW, 0, "P2 N",            IP_KEY_DEFAULT,    IP_JOY_NONE )
	PORT_BITX(0x10, IP_ACTIVE_LOW, 0, "P2 Reach",        IP_KEY_DEFAULT,    IP_JOY_NONE )
	PORT_BITX(0x20, IP_ACTIVE_LOW, 0, "P2 Bet",          KEYCODE_4,         IP_JOY_NONE )
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* P2 IN2 */
	PORT_BITX(0x01, IP_ACTIVE_LOW, 0, "P2 C",            IP_KEY_DEFAULT,    IP_JOY_NONE )
	PORT_BITX(0x02, IP_ACTIVE_LOW, 0, "P2 G",            IP_KEY_DEFAULT,    IP_JOY_NONE )
	PORT_BITX(0x04, IP_ACTIVE_LOW, 0, "P2 K",            IP_KEY_DEFAULT,    IP_JOY_NONE )
	PORT_BITX(0x08, IP_ACTIVE_LOW, 0, "P2 Chi",          IP_KEY_DEFAULT,    IP_JOY_NONE )
	PORT_BITX(0x10, IP_ACTIVE_LOW, 0, "P2 Ron",          IP_KEY_DEFAULT,    IP_JOY_NONE )
	PORT_BIT( 0xe0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* P2 IN3 */
	PORT_BITX(0x01, IP_ACTIVE_LOW, 0, "P2 D",            IP_KEY_DEFAULT,    IP_JOY_NONE )
	PORT_BITX(0x02, IP_ACTIVE_LOW, 0, "P2 H",            IP_KEY_DEFAULT,    IP_JOY_NONE )
	PORT_BITX(0x04, IP_ACTIVE_LOW, 0, "P2 L",            IP_KEY_DEFAULT,    IP_JOY_NONE )
	PORT_BITX(0x08, IP_ACTIVE_LOW, 0, "P2 Pon",          IP_KEY_DEFAULT,    IP_JOY_NONE )
	PORT_BIT( 0xf0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* P2 IN4 */
	PORT_BITX(0x01, IP_ACTIVE_LOW, 0, "P2 Last Chance",  IP_KEY_DEFAULT,    IP_JOY_NONE )
	PORT_BITX(0x02, IP_ACTIVE_LOW, 0, "P2 Take Score",   IP_KEY_DEFAULT,    IP_JOY_NONE )
	PORT_BITX(0x04, IP_ACTIVE_LOW, 0, "P2 Double Up",    IP_KEY_DEFAULT,    IP_JOY_NONE )
	PORT_BITX(0x08, IP_ACTIVE_LOW, 0, "P2 Flip Flop",    IP_KEY_DEFAULT,    IP_JOY_NONE )
	PORT_BITX(0x10, IP_ACTIVE_LOW, 0, "P2 Big",          IP_KEY_DEFAULT,    IP_JOY_NONE )
	PORT_BITX(0x20, IP_ACTIVE_LOW, 0, "P2 Small",        IP_KEY_DEFAULT,    IP_JOY_NONE )
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* IN10 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_SERVICE1 )	/* "Note" ("Paper Money") = 10 Credits */
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_SERVICE3 )	/* Memory Reset */
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_SERVICE2 )	/* Analizer (Statistics) */
	PORT_SERVICE( 0x08, IP_ACTIVE_HIGH )
	PORT_BIT( 0xf0, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START	/* DSW1 (inport $10 -> 0x73b0) */
	PORT_DIPNAME( 0x0f, 0x0f, "Pay Out Rate" )
	PORT_DIPSETTING(    0x0f, "96%" )
	PORT_DIPSETTING(    0x0e, "93%" )
	PORT_DIPSETTING(    0x0d, "90%" )
	PORT_DIPSETTING(    0x0c, "87%" )
	PORT_DIPSETTING(    0x0b, "84%" )
	PORT_DIPSETTING(    0x0a, "81%" )
	PORT_DIPSETTING(    0x09, "78%" )
	PORT_DIPSETTING(    0x08, "75%" )
	PORT_DIPSETTING(    0x07, "71%" )
	PORT_DIPSETTING(    0x06, "68%" )
	PORT_DIPSETTING(    0x05, "65%" )
	PORT_DIPSETTING(    0x04, "62%" )
	PORT_DIPSETTING(    0x03, "59%" )
	PORT_DIPSETTING(    0x02, "56%" )
	PORT_DIPSETTING(    0x01, "53%" )
	PORT_DIPSETTING(    0x00, "50%" )
	PORT_DIPNAME( 0x30, 0x30, "Maximum Bet" )
	PORT_DIPSETTING(    0x00, "1" )
	PORT_DIPSETTING(    0x10, "5" )
	PORT_DIPSETTING(    0x20, "10" )
	PORT_DIPSETTING(    0x30, "20" )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Unknown ) )		/* affects videoram - flip screen ?*/
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, "Debug Mode ?" )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START	/* DSW3 (inport $47 -> 0x73b1) */
	PORT_DIPNAME( 0x03, 0x03, "Winnings" )			/* check code at 0x0e6d*/
	PORT_DIPSETTING(    0x00, "32 24 16 12 8 4 2 1" )	/* table at 0x4e7d*/
	PORT_DIPSETTING(    0x03, "50 30 15 8 5 3 2 1" )	/* table at 0x4e4d*/
	PORT_DIPSETTING(    0x02, "100 50 25 10 5 3 2 1" )	/* table at 0x4e5d*/
	PORT_DIPSETTING(    0x01, "200 100 50 10 5 3 2 1" )	/* table at 0x4e6d*/
	PORT_DIPNAME( 0x04, 0x00, DEF_STR( Unknown ) )		/* check code at 0x5184*/
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x04, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Unknown ) )		/* stores something at 0x76ff*/
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x00, DEF_STR( Unknown ) )		/* check code at 0x1482, 0x18c2, 0x1a1d, 0x1a83, 0x2d2f and 0x2d85*/
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x10, DEF_STR( On ) )
	PORT_DIPNAME( 0x60, 0x60, "Maximum Payout ?" )		/* check code at 0x1ab7*/
	PORT_DIPSETTING(    0x00, "100" )
	PORT_DIPSETTING(    0x20, "200" )
	PORT_DIPSETTING(    0x40, "300" )
	PORT_DIPSETTING(    0x60, "500" )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Unknown ) )		/* check code at 0x18c2, 0x1a1d, 0x2d2f and 0x2d85*/
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x80, DEF_STR( On ) )

	PORT_START	/* DSW2 (inport $46 -> 0x73b2) */
	PORT_DIPNAME( 0x01, 0x00, "Special Combinaisons" )	/* see notes*/
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Unknown ) )		/* check code at 0x07c5*/
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x02, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x00, DEF_STR( Unknown ) )		/* check code at 0x5375*/
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x04, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Unknown ) )		/* check code at 0x5241*/
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x00, DEF_STR( Unknown ) )		/* untested ?*/
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x10, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x00, DEF_STR( Unknown ) )		/* check code at 0x13aa*/
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x20, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, "Full Tests" )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x80, DEF_STR( On ) )
INPUT_PORTS_END

INPUT_PORTS_START( mjdiplob )
	PORT_START	/* P1 IN0 */
	PORT_BITX(0x01, IP_ACTIVE_LOW, 0, "P1 A",            KEYCODE_A,         IP_JOY_NONE )
	PORT_BITX(0x02, IP_ACTIVE_LOW, 0, "P1 E",            KEYCODE_E,         IP_JOY_NONE )
	PORT_BITX(0x04, IP_ACTIVE_LOW, 0, "P1 I",            KEYCODE_I,         IP_JOY_NONE )
	PORT_BITX(0x08, IP_ACTIVE_LOW, 0, "P1 M",            KEYCODE_M,         IP_JOY_NONE )
	PORT_BITX(0x10, IP_ACTIVE_LOW, 0, "P1 Kan",          KEYCODE_LCONTROL,  IP_JOY_NONE )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BITX(0x40, IP_ACTIVE_LOW, 0, "Payout",          KEYCODE_7,  IP_JOY_NONE )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* P1 IN1 */
	PORT_BITX(0x01, IP_ACTIVE_LOW, 0, "P1 B",            KEYCODE_B,         IP_JOY_NONE )
	PORT_BITX(0x02, IP_ACTIVE_LOW, 0, "P1 F",            KEYCODE_F,         IP_JOY_NONE )
	PORT_BITX(0x04, IP_ACTIVE_LOW, 0, "P1 J",            KEYCODE_J,         IP_JOY_NONE )
	PORT_BITX(0x08, IP_ACTIVE_LOW, 0, "P1 N",            KEYCODE_N,         IP_JOY_NONE )
	PORT_BITX(0x10, IP_ACTIVE_LOW, 0, "P1 Reach",        KEYCODE_LSHIFT,    IP_JOY_NONE )
	PORT_BITX(0x20, IP_ACTIVE_LOW, 0, "P1 Bet",          KEYCODE_3,         IP_JOY_NONE )
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* P1 IN2 */
	PORT_BITX(0x01, IP_ACTIVE_LOW, 0, "P1 C",            KEYCODE_C,         IP_JOY_NONE )
	PORT_BITX(0x02, IP_ACTIVE_LOW, 0, "P1 G",            KEYCODE_G,         IP_JOY_NONE )
	PORT_BITX(0x04, IP_ACTIVE_LOW, 0, "P1 K",            KEYCODE_K,         IP_JOY_NONE )
	PORT_BITX(0x08, IP_ACTIVE_LOW, 0, "P1 Chi",          KEYCODE_SPACE,     IP_JOY_NONE )
	PORT_BITX(0x10, IP_ACTIVE_LOW, 0, "P1 Ron",          KEYCODE_Z,         IP_JOY_NONE )
	PORT_BIT( 0xe0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* P1 IN3 */
	PORT_BITX(0x01, IP_ACTIVE_LOW, 0, "P1 D",            KEYCODE_D,         IP_JOY_NONE )
	PORT_BITX(0x02, IP_ACTIVE_LOW, 0, "P1 H",            KEYCODE_H,         IP_JOY_NONE )
	PORT_BITX(0x04, IP_ACTIVE_LOW, 0, "P1 L",            KEYCODE_L,         IP_JOY_NONE )
	PORT_BITX(0x08, IP_ACTIVE_LOW, 0, "P1 Pon",          KEYCODE_LALT,      IP_JOY_NONE )
	PORT_BIT( 0xf0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* P1 IN4 */
	PORT_BITX(0x01, IP_ACTIVE_LOW, 0, "P1 Last Chance",  KEYCODE_RALT,      IP_JOY_NONE )
	PORT_BITX(0x02, IP_ACTIVE_LOW, 0, "P1 Take Score",   KEYCODE_RCONTROL,  IP_JOY_NONE )
	PORT_BITX(0x04, IP_ACTIVE_LOW, 0, "P1 Double Up",    KEYCODE_RSHIFT,    IP_JOY_NONE )
	PORT_BITX(0x08, IP_ACTIVE_LOW, 0, "P1 Flip Flop",    KEYCODE_X,         IP_JOY_NONE )
	PORT_BITX(0x10, IP_ACTIVE_LOW, 0, "P1 Big",          KEYCODE_ENTER,     IP_JOY_NONE )
	PORT_BITX(0x20, IP_ACTIVE_LOW, 0, "P1 Small",        KEYCODE_BACKSPACE, IP_JOY_NONE )
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* P2 IN0 */
	PORT_BITX(0x01, IP_ACTIVE_LOW, 0, "P2 A",            IP_KEY_DEFAULT,    IP_JOY_NONE )
	PORT_BITX(0x02, IP_ACTIVE_LOW, 0, "P2 E",            IP_KEY_DEFAULT,    IP_JOY_NONE )
	PORT_BITX(0x04, IP_ACTIVE_LOW, 0, "P2 I",            IP_KEY_DEFAULT,    IP_JOY_NONE )
	PORT_BITX(0x08, IP_ACTIVE_LOW, 0, "P2 M",            IP_KEY_DEFAULT,    IP_JOY_NONE )
	PORT_BITX(0x10, IP_ACTIVE_LOW, 0, "P2 Kan",          IP_KEY_DEFAULT,    IP_JOY_NONE )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* P2 IN1 */
	PORT_BITX(0x01, IP_ACTIVE_LOW, 0, "P2 B",            IP_KEY_DEFAULT,    IP_JOY_NONE )
	PORT_BITX(0x02, IP_ACTIVE_LOW, 0, "P2 F",            IP_KEY_DEFAULT,    IP_JOY_NONE )
	PORT_BITX(0x04, IP_ACTIVE_LOW, 0, "P2 J",            IP_KEY_DEFAULT,    IP_JOY_NONE )
	PORT_BITX(0x08, IP_ACTIVE_LOW, 0, "P2 N",            IP_KEY_DEFAULT,    IP_JOY_NONE )
	PORT_BITX(0x10, IP_ACTIVE_LOW, 0, "P2 Reach",        IP_KEY_DEFAULT,    IP_JOY_NONE )
	PORT_BITX(0x20, IP_ACTIVE_LOW, 0, "P2 Bet",          KEYCODE_4,         IP_JOY_NONE )
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* P2 IN2 */
	PORT_BITX(0x01, IP_ACTIVE_LOW, 0, "P2 C",            IP_KEY_DEFAULT,    IP_JOY_NONE )
	PORT_BITX(0x02, IP_ACTIVE_LOW, 0, "P2 G",            IP_KEY_DEFAULT,    IP_JOY_NONE )
	PORT_BITX(0x04, IP_ACTIVE_LOW, 0, "P2 K",            IP_KEY_DEFAULT,    IP_JOY_NONE )
	PORT_BITX(0x08, IP_ACTIVE_LOW, 0, "P2 Chi",          IP_KEY_DEFAULT,    IP_JOY_NONE )
	PORT_BITX(0x10, IP_ACTIVE_LOW, 0, "P2 Ron",          IP_KEY_DEFAULT,    IP_JOY_NONE )
	PORT_BIT( 0xe0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* P2 IN3 */
	PORT_BITX(0x01, IP_ACTIVE_LOW, 0, "P2 D",            IP_KEY_DEFAULT,    IP_JOY_NONE )
	PORT_BITX(0x02, IP_ACTIVE_LOW, 0, "P2 H",            IP_KEY_DEFAULT,    IP_JOY_NONE )
	PORT_BITX(0x04, IP_ACTIVE_LOW, 0, "P2 L",            IP_KEY_DEFAULT,    IP_JOY_NONE )
	PORT_BITX(0x08, IP_ACTIVE_LOW, 0, "P2 Pon",          IP_KEY_DEFAULT,    IP_JOY_NONE )
	PORT_BIT( 0xf0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* P2 IN4 */
	PORT_BITX(0x01, IP_ACTIVE_LOW, 0, "P2 Last Chance",  IP_KEY_DEFAULT,    IP_JOY_NONE )
	PORT_BITX(0x02, IP_ACTIVE_LOW, 0, "P2 Take Score",   IP_KEY_DEFAULT,    IP_JOY_NONE )
	PORT_BITX(0x04, IP_ACTIVE_LOW, 0, "P2 Double Up",    IP_KEY_DEFAULT,    IP_JOY_NONE )
	PORT_BITX(0x08, IP_ACTIVE_LOW, 0, "P2 Flip Flop",    IP_KEY_DEFAULT,    IP_JOY_NONE )
	PORT_BITX(0x10, IP_ACTIVE_LOW, 0, "P2 Big",          IP_KEY_DEFAULT,    IP_JOY_NONE )
	PORT_BITX(0x20, IP_ACTIVE_LOW, 0, "P2 Small",        IP_KEY_DEFAULT,    IP_JOY_NONE )
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* IN10 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_SERVICE1 )	/* "Note" ("Paper Money") = 10 Credits */
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_SERVICE3 )	/* Memory Reset */
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_SERVICE2 )	/* Analizer (Statistics) */
	PORT_SERVICE( 0x08, IP_ACTIVE_HIGH )
	PORT_BIT( 0xf0, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START	/* DSW1 (inport $10 -> 0x76fa) */
	PORT_DIPNAME( 0x0f, 0x0f, "Pay Out Rate" )
	PORT_DIPSETTING(    0x0f, "96%" )
	PORT_DIPSETTING(    0x0e, "93%" )
	PORT_DIPSETTING(    0x0d, "90%" )
	PORT_DIPSETTING(    0x0c, "87%" )
	PORT_DIPSETTING(    0x0b, "84%" )
	PORT_DIPSETTING(    0x0a, "81%" )
	PORT_DIPSETTING(    0x09, "78%" )
	PORT_DIPSETTING(    0x08, "75%" )
	PORT_DIPSETTING(    0x07, "71%" )
	PORT_DIPSETTING(    0x06, "68%" )
	PORT_DIPSETTING(    0x05, "65%" )
	PORT_DIPSETTING(    0x04, "62%" )
	PORT_DIPSETTING(    0x03, "59%" )
	PORT_DIPSETTING(    0x02, "56%" )
	PORT_DIPSETTING(    0x01, "53%" )
	PORT_DIPSETTING(    0x00, "50%" )
	PORT_DIPNAME( 0x30, 0x30, "Maximum Bet" )
	PORT_DIPSETTING(    0x00, "1" )
	PORT_DIPSETTING(    0x10, "5" )
	PORT_DIPSETTING(    0x20, "10" )
	PORT_DIPSETTING(    0x30, "20" )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Unknown ) )		/* affects videoram - flip screen ?*/
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, "Debug Mode ?" )		/* check code at 0x0b94 and 0x0de2*/
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START	/* DSW2 (inport $62 -> 0x76fb) */
	PORT_DIPNAME( 0x03, 0x03, "Winnings" )			/* check code at 0x09cd*/
	PORT_DIPSETTING(    0x00, "32 24 16 12 8 4 2 1" )	/* table at 0x4b82*/
	PORT_DIPSETTING(    0x03, "50 30 15 8 5 3 2 1" )	/* table at 0x4b52*/
	PORT_DIPSETTING(    0x02, "100 50 25 10 5 3 2 1" )	/* table at 0x4b62*/
	PORT_DIPSETTING(    0x01, "200 100 50 10 5 3 2 1" )	/* table at 0x4b72*/
	PORT_DIPNAME( 0x04, 0x00, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x04, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	PORT_DIPNAME( 0x30, 0x30, "Maximum Payout ?" )		/* check code at 0x166c*/
	PORT_DIPSETTING(    0x00, "100" )
	PORT_DIPSETTING(    0x10, "200" )
	PORT_DIPSETTING(    0x20, "300" )
	PORT_DIPSETTING(    0x30, "500" )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Unknown ) )		/* check code at 0x2c64*/
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Unknown ) )		/* check code at 0x2c64*/
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x80, DEF_STR( On ) )

	PORT_START	/* DSW3 (inport $63 -> 0x76fc) */
	PORT_DIPNAME( 0x01, 0x00, "Special Combinaisons" )	/* see notes*/
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x02, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x00, DEF_STR( Unknown ) )		/* check code at 0x531f and 0x5375*/
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x04, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Unknown ) )		/* check code at 0x5240*/
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x00, DEF_STR( Unknown ) )		/* check code at 0x2411*/
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x10, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x00, DEF_STR( Unknown ) )		/* check code at 0x2411 and 0x4beb*/
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x20, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Unknown ) )		/* check code at 0x24ff, 0x25f2, 0x3fcf and 0x45d7*/
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, "Full Tests" )			/* seems to hang after the last animation*/
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
INPUT_PORTS_END

INPUT_PORTS_START( majs101b )
	PORT_START	/* P1 IN0 */
	PORT_BITX(0x01, IP_ACTIVE_LOW, 0, "P1 A",            KEYCODE_A,         IP_JOY_NONE )
	PORT_BITX(0x02, IP_ACTIVE_LOW, 0, "P1 E",            KEYCODE_E,         IP_JOY_NONE )
	PORT_BITX(0x04, IP_ACTIVE_LOW, 0, "P1 I",            KEYCODE_I,         IP_JOY_NONE )
	PORT_BITX(0x08, IP_ACTIVE_LOW, 0, "P1 M",            KEYCODE_M,         IP_JOY_NONE )
	PORT_BITX(0x10, IP_ACTIVE_LOW, 0, "P1 Kan",          KEYCODE_LCONTROL,  IP_JOY_NONE )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BITX(0x40, IP_ACTIVE_LOW, 0, "Payout",          KEYCODE_7,  IP_JOY_NONE )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* P1 IN1 */
	PORT_BITX(0x01, IP_ACTIVE_LOW, 0, "P1 B",            KEYCODE_B,         IP_JOY_NONE )
	PORT_BITX(0x02, IP_ACTIVE_LOW, 0, "P1 F",            KEYCODE_F,         IP_JOY_NONE )
	PORT_BITX(0x04, IP_ACTIVE_LOW, 0, "P1 J",            KEYCODE_J,         IP_JOY_NONE )
	PORT_BITX(0x08, IP_ACTIVE_LOW, 0, "P1 N",            KEYCODE_N,         IP_JOY_NONE )
	PORT_BITX(0x10, IP_ACTIVE_LOW, 0, "P1 Reach",        KEYCODE_LSHIFT,    IP_JOY_NONE )
	PORT_BITX(0x20, IP_ACTIVE_LOW, 0, "P1 Bet",          KEYCODE_3,         IP_JOY_NONE )
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* P1 IN2 */
	PORT_BITX(0x01, IP_ACTIVE_LOW, 0, "P1 C",            KEYCODE_C,         IP_JOY_NONE )
	PORT_BITX(0x02, IP_ACTIVE_LOW, 0, "P1 G",            KEYCODE_G,         IP_JOY_NONE )
	PORT_BITX(0x04, IP_ACTIVE_LOW, 0, "P1 K",            KEYCODE_K,         IP_JOY_NONE )
	PORT_BITX(0x08, IP_ACTIVE_LOW, 0, "P1 Chi",          KEYCODE_SPACE,     IP_JOY_NONE )
	PORT_BITX(0x10, IP_ACTIVE_LOW, 0, "P1 Ron",          KEYCODE_Z,         IP_JOY_NONE )
	PORT_BIT( 0xe0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* P1 IN3 */
	PORT_BITX(0x01, IP_ACTIVE_LOW, 0, "P1 D",            KEYCODE_D,         IP_JOY_NONE )
	PORT_BITX(0x02, IP_ACTIVE_LOW, 0, "P1 H",            KEYCODE_H,         IP_JOY_NONE )
	PORT_BITX(0x04, IP_ACTIVE_LOW, 0, "P1 L",            KEYCODE_L,         IP_JOY_NONE )
	PORT_BITX(0x08, IP_ACTIVE_LOW, 0, "P1 Pon",          KEYCODE_LALT,      IP_JOY_NONE )
	PORT_BIT( 0xf0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* P1 IN4 */
	PORT_BITX(0x01, IP_ACTIVE_LOW, 0, "P1 Last Chance",  KEYCODE_RALT,      IP_JOY_NONE )
	PORT_BITX(0x02, IP_ACTIVE_LOW, 0, "P1 Take Score",   KEYCODE_RCONTROL,  IP_JOY_NONE )
	PORT_BITX(0x04, IP_ACTIVE_LOW, 0, "P1 Double Up",    KEYCODE_RSHIFT,    IP_JOY_NONE )
	PORT_BITX(0x08, IP_ACTIVE_LOW, 0, "P1 Flip Flop",    KEYCODE_X,         IP_JOY_NONE )
	PORT_BITX(0x10, IP_ACTIVE_LOW, 0, "P1 Big",          KEYCODE_ENTER,     IP_JOY_NONE )
	PORT_BITX(0x20, IP_ACTIVE_LOW, 0, "P1 Small",        KEYCODE_BACKSPACE, IP_JOY_NONE )
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* P2 IN0 */
	PORT_BITX(0x01, IP_ACTIVE_LOW, 0, "P2 A",            IP_KEY_DEFAULT,    IP_JOY_NONE )
	PORT_BITX(0x02, IP_ACTIVE_LOW, 0, "P2 E",            IP_KEY_DEFAULT,    IP_JOY_NONE )
	PORT_BITX(0x04, IP_ACTIVE_LOW, 0, "P2 I",            IP_KEY_DEFAULT,    IP_JOY_NONE )
	PORT_BITX(0x08, IP_ACTIVE_LOW, 0, "P2 M",            IP_KEY_DEFAULT,    IP_JOY_NONE )
	PORT_BITX(0x10, IP_ACTIVE_LOW, 0, "P2 Kan",          IP_KEY_DEFAULT,    IP_JOY_NONE )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* P2 IN1 */
	PORT_BITX(0x01, IP_ACTIVE_LOW, 0, "P2 B",            IP_KEY_DEFAULT,    IP_JOY_NONE )
	PORT_BITX(0x02, IP_ACTIVE_LOW, 0, "P2 F",            IP_KEY_DEFAULT,    IP_JOY_NONE )
	PORT_BITX(0x04, IP_ACTIVE_LOW, 0, "P2 J",            IP_KEY_DEFAULT,    IP_JOY_NONE )
	PORT_BITX(0x08, IP_ACTIVE_LOW, 0, "P2 N",            IP_KEY_DEFAULT,    IP_JOY_NONE )
	PORT_BITX(0x10, IP_ACTIVE_LOW, 0, "P2 Reach",        IP_KEY_DEFAULT,    IP_JOY_NONE )
	PORT_BITX(0x20, IP_ACTIVE_LOW, 0, "P2 Bet",          KEYCODE_4,         IP_JOY_NONE )
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* P2 IN2 */
	PORT_BITX(0x01, IP_ACTIVE_LOW, 0, "P2 C",            IP_KEY_DEFAULT,    IP_JOY_NONE )
	PORT_BITX(0x02, IP_ACTIVE_LOW, 0, "P2 G",            IP_KEY_DEFAULT,    IP_JOY_NONE )
	PORT_BITX(0x04, IP_ACTIVE_LOW, 0, "P2 K",            IP_KEY_DEFAULT,    IP_JOY_NONE )
	PORT_BITX(0x08, IP_ACTIVE_LOW, 0, "P2 Chi",          IP_KEY_DEFAULT,    IP_JOY_NONE )
	PORT_BITX(0x10, IP_ACTIVE_LOW, 0, "P2 Ron",          IP_KEY_DEFAULT,    IP_JOY_NONE )
	PORT_BIT( 0xe0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* P2 IN3 */
	PORT_BITX(0x01, IP_ACTIVE_LOW, 0, "P2 D",            IP_KEY_DEFAULT,    IP_JOY_NONE )
	PORT_BITX(0x02, IP_ACTIVE_LOW, 0, "P2 H",            IP_KEY_DEFAULT,    IP_JOY_NONE )
	PORT_BITX(0x04, IP_ACTIVE_LOW, 0, "P2 L",            IP_KEY_DEFAULT,    IP_JOY_NONE )
	PORT_BITX(0x08, IP_ACTIVE_LOW, 0, "P2 Pon",          IP_KEY_DEFAULT,    IP_JOY_NONE )
	PORT_BIT( 0xf0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* P2 IN4 */
	PORT_BITX(0x01, IP_ACTIVE_LOW, 0, "P2 Last Chance",  IP_KEY_DEFAULT,    IP_JOY_NONE )
	PORT_BITX(0x02, IP_ACTIVE_LOW, 0, "P2 Take Score",   IP_KEY_DEFAULT,    IP_JOY_NONE )
	PORT_BITX(0x04, IP_ACTIVE_LOW, 0, "P2 Double Up",    IP_KEY_DEFAULT,    IP_JOY_NONE )
	PORT_BITX(0x08, IP_ACTIVE_LOW, 0, "P2 Flip Flop",    IP_KEY_DEFAULT,    IP_JOY_NONE )
	PORT_BITX(0x10, IP_ACTIVE_LOW, 0, "P2 Big",          IP_KEY_DEFAULT,    IP_JOY_NONE )
	PORT_BITX(0x20, IP_ACTIVE_LOW, 0, "P2 Small",        IP_KEY_DEFAULT,    IP_JOY_NONE )
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* IN10 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_SERVICE1 )	/* "Note" ("Paper Money") = 10 Credits */
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_SERVICE3 )	/* Memory Reset */
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_SERVICE2 )	/* Analizer (Statistics) */
	PORT_SERVICE( 0x08, IP_ACTIVE_HIGH )
	PORT_BIT( 0xf0, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START	/* DSW1 (inport $10 -> 0x76fd) */
	PORT_DIPNAME( 0x0f, 0x0f, "Pay Out Rate" )
	PORT_DIPSETTING(    0x0f, "96%" )
	PORT_DIPSETTING(    0x0e, "93%" )
	PORT_DIPSETTING(    0x0d, "90%" )
	PORT_DIPSETTING(    0x0c, "87%" )
	PORT_DIPSETTING(    0x0b, "84%" )
	PORT_DIPSETTING(    0x0a, "81%" )
	PORT_DIPSETTING(    0x09, "78%" )
	PORT_DIPSETTING(    0x08, "75%" )
	PORT_DIPSETTING(    0x07, "71%" )
	PORT_DIPSETTING(    0x06, "68%" )
	PORT_DIPSETTING(    0x05, "65%" )
	PORT_DIPSETTING(    0x04, "62%" )
	PORT_DIPSETTING(    0x03, "59%" )
	PORT_DIPSETTING(    0x02, "56%" )
	PORT_DIPSETTING(    0x01, "53%" )
	PORT_DIPSETTING(    0x00, "50%" )
	PORT_DIPNAME( 0x30, 0x30, "Maximum Bet" )
	PORT_DIPSETTING(    0x00, "1" )
	PORT_DIPSETTING(    0x10, "5" )
	PORT_DIPSETTING(    0x20, "10" )
	PORT_DIPSETTING(    0x30, "20" )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, "Debug Mode ?" )		/* check code at 0x1635*/
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START	/* DSW2 (inport $00 (after out 0,$40) -> 0x76fa) */
	PORT_DIPNAME( 0x03, 0x03, "Winnings" )			/* check code at 0x14e4*/
	PORT_DIPSETTING(    0x00, "32 24 16 12 8 4 2 1" )	/* table at 0x1539*/
	PORT_DIPSETTING(    0x03, "50 30 15 8 5 3 2 1" )	/* table at 0x1509*/
	PORT_DIPSETTING(    0x02, "100 50 25 10 5 3 2 1" )	/* table at 0x1519*/
	PORT_DIPSETTING(    0x01, "200 100 50 10 5 3 2 1" )	/* table at 0x1529*/
	PORT_DIPNAME( 0x04, 0x00, DEF_STR( Unknown ) )		/* check code at 0x1220, 0x128d, 0x13b1, 0x13cb and 0x2692*/
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x04, DEF_STR( On ) )
	PORT_DIPNAME( 0x38, 0x00, "Maximum Payout ?" )		/* check code at 0x12c1*/
	PORT_DIPSETTING(    0x20, "200" )
	PORT_DIPSETTING(    0x10, "300" )
	PORT_DIPSETTING(    0x30, "400" )
	PORT_DIPSETTING(    0x08, "500" )
	PORT_DIPSETTING(    0x28, "600" )
	PORT_DIPSETTING(    0x18, "700" )
	PORT_DIPSETTING(    0x00, "1000" )
/*	PORT_DIPSETTING(    0x38, "1000" )*/
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Unknown ) )		/* check code at 0x1333*/
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, "Background" )
	PORT_DIPSETTING(    0x00, "Black" )
	PORT_DIPSETTING(    0x80, "Gray" )

	PORT_START	/* DSW3 (inport $00 (after out 0,$00) -> 0x76fc) */
	PORT_DIPNAME( 0x01, 0x00, "Special Combinaisons" )	/* see notes*/
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Unknown ) )		/* check code at 0x1cf9*/
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x02, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x00, DEF_STR( Unknown ) )		/* check code at 0x21a9, 0x21dc and 0x2244*/
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x04, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Unknown ) )		/* check code at 0x2b7f*/
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x00, DEF_STR( Unknown ) )		/* check code at 0x50ba*/
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x10, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x00, DEF_STR( Unknown ) )		/* check code at 0x1f65*/
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x20, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Unknown ) )		/* check code at 0x6412*/
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )		/* check code at 0x2cb2 and 0x2d02*/
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START	/* DSW4 (inport $00 (after out 0,$20) -> 0x76fb) */
	PORT_DIPNAME( 0x03, 0x00, DEF_STR( Unknown ) )		/* stored at 0x702f - check code at 0x1713,*/
	PORT_DIPSETTING(    0x00, "0" )				/* 0x33d1, 0x3408, 0x3415, 0x347c, 0x3492, 0x350d,*/
	PORT_DIPSETTING(    0x01, "1" )				/* 0x4af9, 0x4b1f and 0x61f6*/
	PORT_DIPSETTING(    0x02, "2" )
	PORT_DIPSETTING(    0x03, "3" )
	PORT_DIPNAME( 0x0c, 0x00, "Difficulty ?" )		/* check code at 0x4b5c and 0x6d72*/
	PORT_DIPSETTING(    0x00, "Easy" )				/* 0x05 - 0x03, 0x02, 0x02, 0x01*/
	PORT_DIPSETTING(    0x04, "Normal" )			/* 0x0a - 0x05, 0x02, 0x02, 0x01*/
	PORT_DIPSETTING(    0x08, "Hard" )				/* 0x0f - 0x06, 0x03, 0x02, 0x01*/
	PORT_DIPSETTING(    0x0c, "Hardest" )			/* 0x14 - 0x0a, 0x06, 0x02, 0x01*/
	PORT_DIPNAME( 0x30, 0x00, DEF_STR( Unknown ) )		/* check code at 0x228e*/
	PORT_DIPSETTING(    0x00, "0x00" )
	PORT_DIPSETTING(    0x10, "0x10" )
	PORT_DIPSETTING(    0x20, "0x20" )
	PORT_DIPSETTING(    0x30, "0x30" )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Unknown ) )		/* check code at 0x11e4*/
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, "Full Tests" )			/* check code at 0x006d*/
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
INPUT_PORTS_END



static struct AY8910interface ay8910_interface =
{
	1,	/* 1 chip */
	18432000/12,	/* 1.5 MHz ? */
	{ 33 },
	{ royalmah_player_1_port_r },
	{ royalmah_player_2_port_r },
	{ 0 },
	{ 0 }
};



static MACHINE_DRIVER_START( royalmah )

	/* basic machine hardware */
	MDRV_CPU_ADD(Z80, 3000000)        /* 3.00 MHz ? */
	MDRV_CPU_MEMORY(readmem,writemem)
	MDRV_CPU_PORTS(royalmah_readport,royalmah_writeport)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,1)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)

	MDRV_NVRAM_HANDLER(generic_0fill)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(256, 256)
	MDRV_VISIBLE_AREA(0, 255, 0, 255)
	MDRV_PALETTE_LENGTH(32)

	MDRV_PALETTE_INIT(royalmah)
	MDRV_VIDEO_START(generic)
	MDRV_VIDEO_UPDATE(royalmah)

	/* sound hardware */
	MDRV_SOUND_ADD(AY8910, ay8910_interface)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( dondenmj )

	/* basic machine hardware */
	MDRV_CPU_ADD_TAG("main", Z80, 8000000/2)	/* 4 MHz ? */
	MDRV_CPU_MEMORY(readmem,writemem)
	MDRV_CPU_PORTS(dondenmj_readport,dondenmj_writeport)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,1)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_NVRAM_HANDLER(generic_0fill)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(256, 256)
	MDRV_VISIBLE_AREA(0, 255, 0, 255)
	MDRV_PALETTE_LENGTH(32)

	MDRV_PALETTE_INIT(royalmah)
	MDRV_VIDEO_START(generic)
	MDRV_VIDEO_UPDATE(royalmah)

	/* sound hardware */
	MDRV_SOUND_ADD(AY8910, ay8910_interface)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( suzume )
	/* basic machine hardware */
	MDRV_IMPORT_FROM(dondenmj)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_PORTS(suzume_readport,suzume_writeport)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( tontonb )
	/* basic machine hardware */
	MDRV_IMPORT_FROM(dondenmj)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_PORTS(tontonb_readport,tontonb_writeport)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( mjdiplob )
	/* basic machine hardware */
	MDRV_IMPORT_FROM(dondenmj)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_PORTS(mjdiplob_readport,mjdiplob_writeport)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( majs101b )
	/* basic machine hardware */
	MDRV_IMPORT_FROM(dondenmj)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_PORTS(majs101b_readport,majs101b_writeport)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( mjderngr )
	/* basic machine hardware */
	MDRV_IMPORT_FROM(dondenmj)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_PORTS(mjderngr_readport,mjderngr_writeport)

	MDRV_PALETTE_LENGTH(512)

	MDRV_PALETTE_INIT(mjderngr)
MACHINE_DRIVER_END


/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( royalmah )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "rom1",       0x0000, 0x1000, CRC(69b37a62) SHA1(7792528754b0df4e11f4ebe33380b713ac7351a3) )
	ROM_LOAD( "rom2",       0x1000, 0x1000, CRC(0c8351b6) SHA1(9e6b48fd39dd98478d1e3557df839b09652c4349) )
	ROM_LOAD( "rom3",       0x2000, 0x1000, CRC(b7736596) SHA1(4b8bc175d945e695b767b9fb2227ffc1cd4b0547) )
	ROM_LOAD( "rom4",       0x3000, 0x1000, CRC(e3c7c15c) SHA1(a335374cc0f5b1d8e689cc304d006dd97f3e35e7) )
	ROM_LOAD( "rom5",       0x4000, 0x1000, CRC(16c09c73) SHA1(ea712f9ca3200ca27434e4200187b488e24f4c65) )
	ROM_LOAD( "rom6",       0x5000, 0x1000, CRC(92687327) SHA1(4fafba5881dca2a147616d94dd055eba6aa3c653) )

	ROM_REGION( 0x0020, REGION_PROMS, ROMREGION_DISPOSE )
	ROM_LOAD( "f-rom.bpr",  0x0000, 0x0020, CRC(d3007282) SHA1(e4d863ab193e49208ed0f59dcddb1da0492314f6) )
ROM_END


ROM_START( suzume )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD( "p1.bin",     0x00000, 0x1000, CRC(e9706967) SHA1(2e3d78178623de6552c9036da90e02f240d94055) )
	ROM_LOAD( "p2.bin",     0x01000, 0x1000, CRC(dd48cd62) SHA1(1ce7b515fabae5054f0ac284a9ed5760f59d18fa) )
	ROM_LOAD( "p3.bin",     0x02000, 0x1000, CRC(10a05c23) SHA1(f13ba660bc5eff9057b1ab46f564f586c76e945d) )
	ROM_LOAD( "p4.bin",     0x03000, 0x1000, CRC(267eaf52) SHA1(56e2f5d7080463dc0f11a2751590ac2b79eb02c5) )
	ROM_LOAD( "p5.bin",     0x04000, 0x1000, CRC(2fde346b) SHA1(7f45aa4427b4cb6bf6cc5919d397b25d53e133f3) )
	ROM_LOAD( "p6.bin",     0x05000, 0x1000, CRC(57f42ac7) SHA1(209b2f62a64ddf544578f144d9ec83478603c8b2) )
	/* bank switched ROMs follow */
	ROM_LOAD( "1.1a",       0x10000, 0x08000, CRC(f670dd47) SHA1(d0236021ae4dd5a10603dde038eb777feeff016f) )	/* 0*/
	ROM_LOAD( "2.1c",       0x18000, 0x08000, CRC(140b11aa) SHA1(6f6a96135434324dcb486596920cb785fe2bf1a2) )	/* 1*/
	ROM_LOAD( "3.1d",       0x20000, 0x08000, CRC(3d437b61) SHA1(175308086e1d7ab566c82dcaeef9f50690edf92a) )	/* 2*/
	ROM_LOAD( "4.1e",       0x28000, 0x08000, CRC(9da8952e) SHA1(956d16b82ff8fe733a7b3135d082e18ea5167dfe) )	/* 3*/
	ROM_LOAD( "5.1h",       0x30000, 0x08000, CRC(04a6f41a) SHA1(37117faf6bc823770413faa7618387ca6f16fa34) )	/* 4*/

	ROM_REGION( 0x0020, REGION_PROMS, ROMREGION_DISPOSE )
	ROM_LOAD( "ic6k.bin",   0x0000, 0x0020, CRC(97e1defe) SHA1(b5002218b2292f7623dd9a205ce183dedeec03f1) )
ROM_END

ROM_START( dondenmj )
	ROM_REGION( 0x90000, REGION_CPU1, 0 )
	ROM_LOAD( "dn5.1h",     0x00000, 0x08000, CRC(3080252e) SHA1(e039087afc36a0c594da093ea599b81a1d757139) )
	/* bank switched ROMs follow */
	ROM_LOAD( "dn1.1e",     0x18000, 0x08000, CRC(1cd9c48a) SHA1(12bc519889dacea59ae49672ad5313fff3a99f12) )	/* 1*/
	ROM_LOAD( "dn2.1d",     0x20000, 0x04000, CRC(7a72929d) SHA1(7955f41883fa53876172bac417955ed0b5eb43f4) )	/* 2*/
	ROM_LOAD( "dn3.2h",     0x30000, 0x08000, CRC(b09d2897) SHA1(0cde3e16ca333be01a5ab3a232f2ea602faec7a2) )	/* 4*/
	ROM_LOAD( "dn4.2e",     0x50000, 0x08000, CRC(67d7dcd6) SHA1(6b708a29de1f4738eb2d4e667327d9433ff7216c) )	/* 8*/

	ROM_REGION( 0x0020, REGION_PROMS, ROMREGION_DISPOSE )
	ROM_LOAD( "ic6k.bin",   0x0000, 0x0020, CRC(97e1defe) SHA1(b5002218b2292f7623dd9a205ce183dedeec03f1) )
ROM_END

ROM_START( mjdiplob )
	ROM_REGION( 0x90000, REGION_CPU1, 0 )
	ROM_LOAD( "071.4l",     0x00000, 0x10000, CRC(81a6d6b0) SHA1(c6169e6d5f35304a0c3efcc2175c3213650f179c) )
	/* bank switched ROMs follow */
	ROM_RELOAD(             0x10000, 0x10000 )				/* 0,1*/
	ROM_LOAD( "072.4k",     0x20000, 0x10000, CRC(a992bb85) SHA1(e60231e04831dac122d1d49a68641ee47b57faaf) )	/* 2,3*/
	ROM_LOAD( "073.4j",     0x30000, 0x10000, CRC(562ed64f) SHA1(42b4a7e5a8de4dde83c12d7b9facf561bc872978) )	/* 4,5*/
	ROM_LOAD( "074.4h",     0x40000, 0x10000, CRC(1eba0140) SHA1(0d0b95be338d7450ad3b24cc47e24e94f86dcefe) )	/* 6,7*/

	ROM_REGION( 0x0020, REGION_PROMS, ROMREGION_DISPOSE )
	ROM_LOAD( "ic6k.bin",   0x0000, 0x0020, CRC(c1e427df) SHA1(9a9980d93dff4b87a940398b18277acaf946eeab) )
ROM_END

ROM_START( tontonb )
	ROM_REGION( 0x90000, REGION_CPU1, 0 )
	ROM_LOAD( "091.5e",   	0x00000, 0x10000, CRC(d8d67b59) SHA1(7e7a85df738f80fc031cda8a104ac9c7b3e24785) )
	/* bank switched ROMs follow */
	ROM_RELOAD(             0x10000, 0x10000 )				/* 0,1*/
	/**/													/* 2,3 unused*/
	ROM_LOAD( "093.5b",   	0x30000, 0x10000, CRC(24b6be55) SHA1(11390d6ed55d7d0b7b84c6d36d4ac5330a06abba) )	/* 4,5*/
	/**/													/* 6,7 unused*/
	ROM_LOAD( "092.5c",   	0x50000, 0x10000, CRC(7ff2738b) SHA1(89a49f89705f499439dc024fc70c87141a84780b) )	/* 8,9*/

	ROM_REGION( 0x0020, REGION_PROMS, ROMREGION_DISPOSE )
	ROM_LOAD( "ic6k.bin",   0x0000, 0x0020, CRC(97e1defe) SHA1(b5002218b2292f7623dd9a205ce183dedeec03f1) )
ROM_END

ROM_START( majs101b )
	ROM_REGION( 0x90000, REGION_CPU1, 0 )
	ROM_LOAD( "171.3e",     0x00000, 0x10000, CRC(fa3c553b) SHA1(fda212559c4d55610a12ad2927afe21f9069c7b6) )
	/* bank switched ROMs follow */
	/**/													/* 0,1 unused*/
	ROM_RELOAD(             0x20000, 0x10000 )				/* 2,3*/
	ROM_LOAD( "172.3f",     0x30000, 0x20000, CRC(7da39a63) SHA1(34d07978a326c83e5b51ce19619d52a75a501795) )	/* 4,5,6,7*/
	ROM_LOAD( "173.3h",     0x50000, 0x20000, CRC(7a9e71ae) SHA1(ce1bde6e05f81b7dbb14015514397ed72f8dd92a) )	/* 8,9,a,b*/
	ROM_LOAD( "174.3j",     0x70000, 0x10000, CRC(972c2cc9) SHA1(ba78d29d1723783dbd0e8c754d2422caad5ab367) )	/* c,d*/

	ROM_REGION( 0x0020, REGION_PROMS, ROMREGION_DISPOSE )
	ROM_LOAD( "ic6k.bin",   0x0000, 0x0020, CRC(c1e427df) SHA1(9a9980d93dff4b87a940398b18277acaf946eeab) )
ROM_END

ROM_START( mjderngr )
	ROM_REGION( 0xb0000, REGION_CPU1, 0 )
	ROM_LOAD( "2201.1a",    0x00000, 0x08000, CRC(54ec531d) SHA1(c5d9c575f6bdc499bae35123d7ad5bd4869b6ed9) )
	/* bank switched ROMs follow */
	ROM_CONTINUE(           0x10000, 0x08000 )				/* 0*/
	ROM_LOAD( "2202.1b",    0x30000, 0x10000, CRC(edcf97f2) SHA1(8143f41d511fa01bd86faf829eb2c139292d705f) )	/* 4,5*/
	ROM_LOAD( "2203.1d",    0x50000, 0x10000, CRC(a33368c0) SHA1(e216b65d7ed59d7cbf2b5d078799915d707b5291) )	/* 8,9*/
	ROM_LOAD( "2204.1e",    0x70000, 0x20000, CRC(ed5fde4b) SHA1(d55487ae1007d43b71f06ae5c407c75db7054515) )	/* c,d,e,f*/
	ROM_LOAD( "2205.1f",    0x90000, 0x20000, CRC(cfb8075d) SHA1(31f613a1a9b5f4295b552aeeddb760605ce2ac70) )	/* 0x10,0x11,0x12,0x13*/

	ROM_REGION( 0x400, REGION_PROMS, ROMREGION_DISPOSE )
	ROM_LOAD( "ic3g.bin",   0x000, 0x200, CRC(d43f4c7c) SHA1(117d2e4e8d5bea3e5dc903a4b87bd71786ae009c) )
	ROM_LOAD( "ic4g.bin",   0x200, 0x200, CRC(30cf7831) SHA1(b4593d51c6ceb301279a01a98665e4be8a3c403d) )
ROM_END

ROM_START( mjifb )
	ROM_REGION( 0xd0000, REGION_CPU1, 0 )
	ROM_LOAD( "2911.1b",    0x00000, 0x08000, CRC(138a31a1) SHA1(7e77c63a968206b8e61aaa423e19a766e4142554) )
	/* bank switched ROMs follow */
	ROM_CONTINUE(           0x10000, 0x08000 )
	ROM_LOAD( "2902.1c",    0x30000, 0x10000, CRC(0ce02a98) SHA1(69f6bca9af8548038401839047a304a4aa97cfe6) )
	ROM_LOAD( "2903.1d",    0x50000, 0x20000, CRC(90c44965) SHA1(6904bfa7475f9de921bc2abcfc337b3daf7e0fad) )
	ROM_LOAD( "2904.1e",    0x70000, 0x20000, CRC(2791abfa) SHA1(a8fd1a7e1cf4441b447a4605ad2f1c13775f92da) )
	ROM_LOAD( "2905.1f",    0x90000, 0x20000, CRC(b7a73cf7) SHA1(d93111e6d5f84e331f8198d8c595e3500abed133) )
	ROM_LOAD( "2906.1g",    0xb0000, 0x20000, CRC(ad469345) SHA1(914ea4c77a540467da779ea78c52e66b05c30475) )

	ROM_REGION( 0x400, REGION_PROMS, ROMREGION_DISPOSE )
	ROM_LOAD( "d29-2.4d",   0x000, 0x200, CRC(78252f6a) SHA1(1869147bc6b7573c2543bdf6b17d6c3c1debdddb) )
	ROM_LOAD( "d29-1.4c",   0x200, 0x200, CRC(4aaec8cf) SHA1(fbe1c3729d078a422ffe68dfde495fcb9f329cdd) )
ROM_END



GAME( 1982, royalmah, 0, royalmah, royalmah, 0, ROT0, "Falcon", "Royal Mahjong (Japan)" )
GAMEX(1986, suzume,   0, suzume,   majs101b, 0, ROT0, "Dyna Electronics", "Watashiha Suzumechan (Japan)", GAME_NOT_WORKING )
GAME( 1986, dondenmj, 0, dondenmj, majs101b, 0, ROT0, "Dyna Electronics", "Don Den Mahjong [BET] (Japan)" )
GAME( 1987, mjdiplob, 0, mjdiplob, mjdiplob, 0, ROT0, "Dynax", "Mahjong Diplomat [BET] (Japan)" )
GAME( 1987, tontonb,  0, tontonb,  tontonb,  0, ROT0, "Dynax", "Tonton [BET] (Japan)" )
GAME( 1988, majs101b, 0, majs101b, majs101b, 0, ROT0, "Dynax", "Mahjong Studio 101 [BET] (Japan)" )
GAME( 1989, mjderngr, 0, mjderngr, majs101b, 0, ROT0, "Dynax", "Mahjong Derringer (Japan)" )
GAMEX(1990, mjifb,    0, mjderngr, majs101b, 0, ROT0, "Dynax", "Mahjong If [BET] (Japan)", GAME_NOT_WORKING )
