/***************************************************************************

	Atari Gauntlet hardware

	driver by Aaron Giles

	Games supported:
		* Gauntlet (1985) [14 sets]
		* Gauntlet 2-player Version (1985) [6 sets]
		* Gauntlet II (1986) [2 sets]
		* Gauntlet II 2-player Version (1986) [3 sets]
		* Vindicators Part II (1988) [3 sets]

	Known bugs:
		* none at this time

****************************************************************************

	Memory map

****************************************************************************

	========================================================================
	MAIN CPU
	========================================================================
	000000-037FFF   R     xxxxxxxx xxxxxxxx   Program ROM
	038000-03FFFF   R     xxxxxxxx xxxxxxxx   Slapstic-protected ROM
	040000-07FFFF   R     xxxxxxxx xxxxxxxx   Program ROM
	800000-801FFF   R/W   xxxxxxxx xxxxxxxx   Program RAM
	802000-802FFF   R/W   -------- xxxxxxxx   EEPROM
	803000          R     -------- xxxxxxxx   Input port 1
	803002          R     -------- xxxxxxxx   Input port 2
	803004          R     -------- xxxxxxxx   Input port 3
	803006          R     -------- xxxxxxxx   Input port 4
	803008          R     -------- -xxxx---   Status port
	                R     -------- -x------      (VBLANK)
	                R     -------- --x-----      (Sound command buffer full)
	                R     -------- ---x----      (Sound response buffer full)
	                R     -------- ----x---      (Self test)
	80300E          R     -------- xxxxxxxx   Sound response read
	803100            W   -------- --------   Watchdog reset
	80312E            W   -------- -------x   Sound CPU reset
	803140            W   -------- --------   VBLANK IRQ acknowledge
	803150            W   -------- --------   EEPROM enable
	803170            W   -------- xxxxxxxx   Sound command write
	900000-901FFF   R/W   xxxxxxxx xxxxxxxx   Playfield RAM (64x64 tiles)
	                R/W   x------- --------      (Horizontal flip)
	                R/W   -xxx---- --------      (Palette select)
	                R/W   ----xxxx xxxxxxxx      (Tile index)
	902000-903FFF   R/W   xxxxxxxx xxxxxxxx   Motion object RAM (1024 entries x 4 words)
	                R/W   -xxxxxxx xxxxxxxx      (0: Tile index)
	                R/W   xxxxxxxx x-------      (1024: X position)
	                R/W   -------- ----xxxx      (1024: Palette select)
	                R/W   xxxxxxxx x-------      (2048: Y position)
	                R/W   -------- -x------      (2048: Horizontal flip)
	                R/W   -------- --xxx---      (2048: Number of X tiles - 1)
	                R/W   -------- -----xxx      (2048: Number of Y tiles - 1)
	                R/W   ------xx xxxxxxxx      (3072: Link to next object)
	904000-904FFF   R/W   xxxxxxxx xxxxxxxx   Spare video RAM
	905000-905FFF   R/W   xxxxxxxx xxxxxxxx   Alphanumerics RAM (64x32 tiles)
	                R/W   x------- --------      (Opaque/transparent)
	                R/W   -xxxxx-- --------      (Palette select)
	                R/W   ------xx xxxxxxxx      (Tile index)
	905F6E          R/W   xxxxxxxx x-----xx   Playfield Y scroll/tile bank select
	                R/W   xxxxxxxx x-------      (Playfield Y scroll)
	                R/W   -------- ------xx      (Playfield tile bank select)
	910000-9101FF   R/W   xxxxxxxx xxxxxxxx   Alphanumercs palette RAM (256 entries)
	                R/W   xxxx---- --------      (Intensity)
	                R/W   ----xxxx --------      (Red)
	                R/W   -------- xxxx----      (Green)
	                R/W   -------- ----xxxx      (Blue)
	910200-9103FF   R/W   xxxxxxxx xxxxxxxx   Motion object palette RAM (256 entries)
	910400-9105FF   R/W   xxxxxxxx xxxxxxxx   Playfield palette RAM (256 entries)
	910600-9107FF   R/W   xxxxxxxx xxxxxxxx   Extra palette RAM (256 entries)
	930000            W   xxxxxxxx x-------   Playfield X scroll
	========================================================================
	Interrupts:
		IRQ4 = VBLANK
		IRQ6 = sound CPU communications
	========================================================================


	========================================================================
	SOUND CPU
	========================================================================
	0000-0FFF   R/W   xxxxxxxx   Program RAM
	1000          W   xxxxxxxx   Sound response write
	1010        R     xxxxxxxx   Sound command read
	1020        R     ----xxxx   Coin inputs
	            R     ----x---      (Coin 1)
	            R     -----x--      (Coin 2)
	            R     ------x-      (Coin 3)
	            R     -------x      (Coin 4)
	1020          W   xxxxxxxx   Mixer control
	              W   xxx-----      (TMS5220 volume)
	              W   ---xx---      (POKEY volume)
	              W   -----xxx      (YM2151 volume)
	1030        R     xxxx----   Sound status read
	            R     x-------      (Sound command buffer full)
	            R     -x------      (Sound response buffer full)
	            R     --x-----      (TMS5220 ready)
	            R     ---x----      (Self test)
	1030          W   x-------   YM2151 reset
	1031          W   x-------   TMS5220 data strobe
	1032          W   x-------   TMS5220 reset
	1033          W   x-------   TMS5220 frequency
	1800-180F   R/W   xxxxxxxx   POKEY communications
	1810-1811   R/W   xxxxxxxx   YM2151 communications
	1820          W   xxxxxxxx   TMS5220 data latch
	1830        R/W   --------   IRQ acknowledge
	4000-FFFF   R     xxxxxxxx   Program ROM
	========================================================================
	Interrupts:
		IRQ = timed interrupt
		NMI = latch on sound command
	========================================================================

****************************************************************************/


#include "driver.h"
#include "machine/atarigen.h"
#include "gauntlet.h"



/*************************************
 *
 *	Statics
 *
 *************************************/

static UINT8 speech_val;
static UINT8 last_speech_write;
static data16_t sound_reset_val;



/*************************************
 *
 *	Initialization & interrupts
 *
 *************************************/

static void update_interrupts(void)
{
	int newstate = 0;

	if (atarigen_video_int_state)
		newstate |= 4;
	if (atarigen_sound_int_state)
		newstate |= 6;

	if (newstate)
		cpu_set_irq_line(0, newstate, ASSERT_LINE);
	else
		cpu_set_irq_line(0, 7, CLEAR_LINE);
}


static void scanline_update(int scanline)
{
	/* sound IRQ is on 32V */
	if (scanline & 32)
		atarigen_6502_irq_gen();
	else
		atarigen_6502_irq_ack_r(0);
}


static MACHINE_INIT( gauntlet )
{
	last_speech_write = 0x80;
	sound_reset_val = 1;

	atarigen_eeprom_reset();
	atarigen_slapstic_reset();
	atarigen_interrupt_reset(update_interrupts);
	atarigen_scanline_timer_reset(scanline_update, 32);
	atarigen_sound_io_reset(1);
}



/*************************************
 *
 *	Controller reads
 *
 *************************************/

static int fake_inputs(int real_port, int fake_port)
{
	int result = readinputport(real_port);
	int fake = readinputport(fake_port);

	if (fake & 0x01)			/* up */
	{
		if (fake & 0x04)		/* up and left */
			result &= ~0x20;
		else if (fake & 0x08)	/* up and right */
			result &= ~0x10;
		else					/* up only */
			result &= ~0x30;
	}
	else if (fake & 0x02)		/* down */
	{
		if (fake & 0x04)		/* down and left */
			result &= ~0x80;
		else if (fake & 0x08)	/* down and right */
			result &= ~0x40;
		else					/* down only */
			result &= ~0xc0;
	}
	else if (fake & 0x04)		/* left only */
		result &= ~0x60;
	else if (fake & 0x08)		/* right only */
		result &= ~0x90;

	return result;
}


static READ16_HANDLER( vindctr2_port01_r )
{
	return fake_inputs(0 + offset, 6 + offset);
}


static READ16_HANDLER( port4_r )
{
	int temp = readinputport(4);
	if (atarigen_cpu_to_sound_ready) temp ^= 0x0020;
	if (atarigen_sound_to_cpu_ready) temp ^= 0x0010;
	return temp;
}



/*************************************
 *
 *	Sound reset
 *
 *************************************/

static WRITE16_HANDLER( sound_reset_w )
{
	if (ACCESSING_LSB)
	{
		int oldword = sound_reset_val;
		COMBINE_DATA(&sound_reset_val);

		if ((oldword ^ sound_reset_val) & 1)
		{
			cpu_set_reset_line(1, (sound_reset_val & 1) ? CLEAR_LINE : ASSERT_LINE);
			atarigen_sound_reset();
		}
	}
}



/*************************************
 *
 *	Sound I/O inputs
 *
 *************************************/

static READ_HANDLER( switch_6502_r )
{
	int temp = 0x30;

	if (atarigen_cpu_to_sound_ready) temp ^= 0x80;
	if (atarigen_sound_to_cpu_ready) temp ^= 0x40;
	if (tms5220_ready_r()) temp ^= 0x20;
	if (!(readinputport(4) & 0x0008)) temp ^= 0x10;

	return temp;
}



/*************************************
 *
 *	Sound TMS5220 write
 *
 *************************************/

static WRITE_HANDLER( tms5220_w )
{
	speech_val = data;
}



/*************************************
 *
 *	Sound control write
 *
 *************************************/

static WRITE_HANDLER( sound_ctl_w )
{
	switch (offset & 7)
	{
		case 0:	/* music reset, bit D7, low reset */
			break;

		case 1:	/* speech write, bit D7, active low */
			if (((data ^ last_speech_write) & 0x80) && (data & 0x80))
				tms5220_data_w(0, speech_val);
			last_speech_write = data;
			break;

		case 2:	/* speech reset, bit D7, active low */
			if (((data ^ last_speech_write) & 0x80) && (data & 0x80))
				tms5220_reset();
			break;

		case 3:	/* speech squeak, bit D7 */
			data = 5 | ((data >> 6) & 2);
			tms5220_set_frequency(ATARI_CLOCK_14MHz/2 / (16 - data));
			break;
	}
}



/*************************************
 *
 *	Sound mixer write
 *
 *************************************/

static WRITE_HANDLER( mixer_w )
{
	atarigen_set_ym2151_vol((data & 7) * 100 / 7);
	atarigen_set_pokey_vol(((data >> 3) & 3) * 100 / 3);
	atarigen_set_tms5220_vol(((data >> 5) & 7) * 100 / 7);
}



/*************************************
 *
 *	Main CPU memory handlers
 *
 *************************************/

static MEMORY_READ16_START( main_readmem )
	{ 0x000000, 0x07ffff, MRA16_ROM },
	{ 0x800000, 0x801fff, MRA16_RAM },
	{ 0x802000, 0x802fff, atarigen_eeprom_r },
	{ 0x803000, 0x803001, input_port_0_word_r },
	{ 0x803002, 0x803003, input_port_1_word_r },
	{ 0x803004, 0x803005, input_port_2_word_r },
	{ 0x803006, 0x803007, input_port_3_word_r },
	{ 0x803008, 0x803009, port4_r },
	{ 0x80300e, 0x80300f, atarigen_sound_r },
	{ 0x900000, 0x905fff, MRA16_RAM },
	{ 0x910000, 0x9107ff, MRA16_RAM },
MEMORY_END


static MEMORY_WRITE16_START( main_writemem )
	{ 0x000000, 0x07ffff, MWA16_ROM },
	{ 0x800000, 0x801fff, MWA16_RAM },
	{ 0x802000, 0x802fff, atarigen_eeprom_w, &atarigen_eeprom, &atarigen_eeprom_size },
	{ 0x803100, 0x803101, watchdog_reset16_w },
	{ 0x80312e, 0x80312f, sound_reset_w },
	{ 0x803140, 0x803141, atarigen_video_int_ack_w },
	{ 0x803150, 0x803151, atarigen_eeprom_enable_w },
	{ 0x803170, 0x803171, atarigen_sound_w },
	{ 0x900000, 0x901fff, atarigen_playfield_w, &atarigen_playfield },
	{ 0x902000, 0x903fff, atarimo_0_spriteram_w, &atarimo_0_spriteram },
	{ 0x904000, 0x904fff, MWA16_RAM },
	{ 0x905f6e, 0x905f6f, gauntlet_yscroll_w, &atarigen_yscroll },
	{ 0x905000, 0x905f7f, atarigen_alpha_w, &atarigen_alpha },
	{ 0x905f80, 0x905fff, atarimo_0_slipram_w, &atarimo_0_slipram },
	{ 0x910000, 0x9107ff, paletteram16_IIIIRRRRGGGGBBBB_word_w, &paletteram16 },
	{ 0x930000, 0x930001, gauntlet_xscroll_w, &atarigen_xscroll },
MEMORY_END



/*************************************
 *
 *	Sound CPU memory handlers
 *
 *************************************/

static MEMORY_READ_START( sound_readmem )
	{ 0x0000, 0x0fff, MRA_RAM },
	{ 0x1010, 0x101f, atarigen_6502_sound_r },
	{ 0x1020, 0x102f, input_port_5_r },
	{ 0x1030, 0x103f, switch_6502_r },
	{ 0x1800, 0x180f, pokey1_r },
	{ 0x1811, 0x1811, YM2151_status_port_0_r },
	{ 0x1830, 0x183f, atarigen_6502_irq_ack_r },
	{ 0x4000, 0xffff, MRA_ROM },
MEMORY_END


static MEMORY_WRITE_START( sound_writemem )
	{ 0x0000, 0x0fff, MWA_RAM },
	{ 0x1000, 0x100f, atarigen_6502_sound_w },
	{ 0x1020, 0x102f, mixer_w },
	{ 0x1030, 0x103f, sound_ctl_w },
	{ 0x1800, 0x180f, pokey1_w },
	{ 0x1810, 0x1810, YM2151_register_port_0_w },
	{ 0x1811, 0x1811, YM2151_data_port_0_w },
	{ 0x1820, 0x182f, tms5220_w },
	{ 0x1830, 0x183f, atarigen_6502_irq_ack_w },
	{ 0x4000, 0xffff, MWA_ROM },
MEMORY_END



/*************************************
 *
 *	Port definitions
 *
 *************************************/

INPUT_PORTS_START( gauntlet )
	PORT_START	/* 803000 */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x000c, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER1 | IPF_8WAY )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER1 | IPF_8WAY )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER1 | IPF_8WAY )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER1 | IPF_8WAY )
	PORT_BIT( 0xff00, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* 803002 */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x000c, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER2 | IPF_8WAY )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER2 | IPF_8WAY )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER2 | IPF_8WAY )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER2 | IPF_8WAY )
	PORT_BIT( 0xff00, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* 803004 */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_START3 )
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER3 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER3 )
	PORT_BIT( 0x000c, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER3 | IPF_8WAY )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER3 | IPF_8WAY )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER3 | IPF_8WAY )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER3 | IPF_8WAY )
	PORT_BIT( 0xff00, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* 803006 */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_START4 )
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER4 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER4 )
	PORT_BIT( 0x000c, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER4 | IPF_8WAY )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER4 | IPF_8WAY )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER4 | IPF_8WAY )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER4 | IPF_8WAY )
	PORT_BIT( 0xff00, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* 803008 */
	PORT_BIT( 0x0007, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_SERVICE( 0x0008, IP_ACTIVE_LOW )
	PORT_BIT( 0x0030, IP_ACTIVE_HIGH, IPT_SPECIAL )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW,  IPT_VBLANK )
	PORT_BIT( 0xff80, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START	/* 1020 (sound) */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN4 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0xf0, IP_ACTIVE_LOW, IPT_UNUSED )
INPUT_PORTS_END


INPUT_PORTS_START( vindctr2 )
	PORT_START	/* 803000 */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER1 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER1 )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_UP    | IPF_PLAYER1 | IPF_2WAY )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_JOYSTICKRIGHT_UP   | IPF_PLAYER1 | IPF_2WAY )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_DOWN  | IPF_PLAYER1 | IPF_2WAY )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_JOYSTICKRIGHT_DOWN | IPF_PLAYER1 | IPF_2WAY )
	PORT_BIT( 0xff00, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* 803002 */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER2 )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_UP    | IPF_PLAYER2 | IPF_2WAY )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_JOYSTICKRIGHT_UP   | IPF_PLAYER2 | IPF_2WAY )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_DOWN  | IPF_PLAYER2 | IPF_2WAY )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_JOYSTICKRIGHT_DOWN | IPF_PLAYER2 | IPF_2WAY )
	PORT_BIT( 0xff00, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* 803004 */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0xfffc, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* 803006 */
	PORT_BIT( 0xffff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* 803008 */
	PORT_BIT( 0x0007, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_SERVICE( 0x0008, IP_ACTIVE_LOW )
	PORT_BIT( 0x0030, IP_ACTIVE_HIGH, IPT_SPECIAL )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW,  IPT_VBLANK )
	PORT_BIT( 0xff80, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START	/* 1020 (sound) */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN4 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0xf0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* single joystick */
	PORT_BIT( 0x0001, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP | IPF_8WAY | IPF_CHEAT | IPF_PLAYER1 )
	PORT_BIT( 0x0002, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_CHEAT | IPF_PLAYER1 )
	PORT_BIT( 0x0004, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_CHEAT | IPF_PLAYER1 )
	PORT_BIT( 0x0008, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_CHEAT | IPF_PLAYER1 )

	PORT_START	/* single joystick */
	PORT_BIT( 0x0001, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP | IPF_8WAY | IPF_CHEAT | IPF_PLAYER2 )
	PORT_BIT( 0x0002, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_CHEAT | IPF_PLAYER2 )
	PORT_BIT( 0x0004, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_CHEAT | IPF_PLAYER2 )
	PORT_BIT( 0x0008, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_CHEAT | IPF_PLAYER2 )
INPUT_PORTS_END



/*************************************
 *
 *	Graphics definitions
 *
 *************************************/

static struct GfxLayout anlayout =
{
	8,8,
	RGN_FRAC(1,1),
	2,
	{ 0, 4 },
	{ 0, 1, 2, 3, 8, 9, 10, 11 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16 },
	8*16
};


static struct GfxLayout pfmolayout =
{
	8,8,
	RGN_FRAC(1,4),
	4,
	{ RGN_FRAC(3,4), RGN_FRAC(2,4), RGN_FRAC(1,4), RGN_FRAC(0,4) },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8
};


static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX2, 0, &pfmolayout,  256, 32 },
	{ REGION_GFX1, 0, &anlayout,      0, 64 },
	{ -1 }
};



/*************************************
 *
 *	Sound definitions
 *
 *************************************/

static struct YM2151interface ym2151_interface =
{
	1,
	ATARI_CLOCK_14MHz/4,
	{ YM3012_VOL(48,MIXER_PAN_LEFT,48,MIXER_PAN_RIGHT) },
	{ 0 }
};


static struct POKEYinterface pokey_interface =
{
	1,
	ATARI_CLOCK_14MHz/8,
	{ 32 },
};


static struct TMS5220interface tms5220_interface =
{
	ATARI_CLOCK_14MHz/2/11,	/* potentially ATARI_CLOCK_14MHz/2/9 as well */
	80,
	0
};



/*************************************
 *
 *	Machine driver
 *
 *************************************/

static MACHINE_DRIVER_START( gauntlet )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68010, ATARI_CLOCK_14MHz/2)
	MDRV_CPU_MEMORY(main_readmem,main_writemem)
	MDRV_CPU_VBLANK_INT(atarigen_video_int_gen,1)

	MDRV_CPU_ADD(M6502, ATARI_CLOCK_14MHz/8)
	MDRV_CPU_MEMORY(sound_readmem,sound_writemem)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)

	MDRV_MACHINE_INIT(gauntlet)
	MDRV_NVRAM_HANDLER(atarigen)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_NEEDS_6BITS_PER_GUN | VIDEO_UPDATE_BEFORE_VBLANK)
	MDRV_SCREEN_SIZE(42*8, 30*8)
	MDRV_VISIBLE_AREA(0*8, 42*8-1, 0*8, 30*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(1024)

	MDRV_VIDEO_START(gauntlet)
	MDRV_VIDEO_UPDATE(gauntlet)

	/* sound hardware */
	MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
	MDRV_SOUND_ADD(YM2151, ym2151_interface)
	MDRV_SOUND_ADD(POKEY, pokey_interface)
	MDRV_SOUND_ADD(TMS5220, tms5220_interface)
MACHINE_DRIVER_END



/*************************************
 *
 *	ROM definition(s)
 *
 *************************************/

ROM_START( gaunts )
	ROM_REGION( 0x80000, REGION_CPU1, 0 )	/* 8*64k for 68000 code */
	ROM_LOAD16_BYTE( "1507-9a.037",  0x000000, 0x008000, CRC(b5183228) SHA1(5cf433acf1463076576ce7c29298c609b0bd9705) )
	ROM_LOAD16_BYTE( "1508-9b.037",  0x000001, 0x008000, CRC(afd3c501) SHA1(99a7bb6c05fc4a865a44887a5ca9dc5e710397d9) )
	ROM_LOAD16_BYTE( "205-10a.037",  0x038000, 0x004000, CRC(6d99ed51) SHA1(a7bc18f32908451859ba5cdf1a5c97ecc5fe325f) )
	ROM_LOAD16_BYTE( "206-10b.037",  0x038001, 0x004000, CRC(545ead91) SHA1(7fad5a63c6443249bb6dad5b2a1fd08ca5f11e10) )
	ROM_LOAD16_BYTE( "1509-7a.037",  0x040000, 0x008000, CRC(69e50ae9) SHA1(bd2c9420dc0db1492db8dfbc49afeae92554efb1) )
	ROM_LOAD16_BYTE( "1510-7b.037",  0x040001, 0x008000, CRC(54e2692c) SHA1(7a4d9c33a3abecef40ac33260fb05260c742868c) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for 6502 code */
	ROM_LOAD( "120-16r.037",  0x004000, 0x004000, CRC(6ee7f3cc) SHA1(b86676340b06f07c164690862c1f6f75f30c080b) )
	ROM_LOAD( "119-16s.037",  0x008000, 0x008000, CRC(fa19861f) SHA1(7568b4ab526bd5849f7ef70dfa6d1ef1f30c0abc) )

	ROM_REGION( 0x04000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "104-6p.037",   0x000000, 0x002000, CRC(9e2a5b59) SHA1(3bbaa92e4612b0a26837ca67cfa4e36d0f2295fa) )

	ROM_REGION( 0x40000, REGION_GFX2, ROMREGION_DISPOSE | ROMREGION_INVERT )
	ROM_LOAD( "111-1a.037",   0x000000, 0x008000, CRC(91700f33) SHA1(fac1ce700c4cd46b643307998df781d637f193aa) )
	ROM_LOAD( "112-1b.037",   0x008000, 0x008000, CRC(869330be) SHA1(5dfaaf54ee2b3c0eaf35e8c17558313db9791616) )
	ROM_LOAD( "113-1l.037",   0x010000, 0x008000, CRC(d497d0a8) SHA1(bb715bcec7f783dd04151e2e3b221a72133bf17d) )
	ROM_LOAD( "114-1mn.037",  0x018000, 0x008000, CRC(29ef9882) SHA1(91e1465af6505b35cd97434c13d2b4d40a085946) )
	ROM_LOAD( "115-2a.037",   0x020000, 0x008000, CRC(9510b898) SHA1(e6c8c7af1898d548f0f01e4ff37c2c7b22c0b5c2) )
	ROM_LOAD( "116-2b.037",   0x028000, 0x008000, CRC(11e0ac5b) SHA1(729b7561d59d94ef33874a134b97bcd37573dfa6) )
	ROM_LOAD( "117-2l.037",   0x030000, 0x008000, CRC(29a5db41) SHA1(94f4f5dd39e724570a0f54af176ad018497697fd) )
	ROM_LOAD( "118-2mn.037",  0x038000, 0x008000, CRC(8bf3b263) SHA1(683d900ab7591ee661218be2406fb375a12e435c) )
ROM_END


ROM_START( gauntlet )
	ROM_REGION( 0x80000, REGION_CPU1, 0 )	/* 8*64k for 68000 code */
	ROM_LOAD16_BYTE( "1307-9a.037",  0x000000, 0x008000, CRC(46fe8743) SHA1(d5fa19e028a2f43658330c67c10e0c811d332780) )
	ROM_LOAD16_BYTE( "1308-9b.037",  0x000001, 0x008000, CRC(276e15c4) SHA1(7467b2ec21b1b4fcc18ff9387ce891495f4b064c) )
	ROM_LOAD16_BYTE( "205-10a.037",  0x038000, 0x004000, CRC(6d99ed51) SHA1(a7bc18f32908451859ba5cdf1a5c97ecc5fe325f) )
	ROM_LOAD16_BYTE( "206-10b.037",  0x038001, 0x004000, CRC(545ead91) SHA1(7fad5a63c6443249bb6dad5b2a1fd08ca5f11e10) )
	ROM_LOAD16_BYTE( "1409-7a.037",  0x040000, 0x008000, CRC(6fb8419c) SHA1(299fee0368f6027bacbb57fb469e817e64e0e41d) )
	ROM_LOAD16_BYTE( "1410-7b.037",  0x040001, 0x008000, CRC(931bd2a0) SHA1(d69b45758d1c252a93dbc2263efa9de1f972f62e) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for 6502 code */
	ROM_LOAD( "120-16r.037",  0x004000, 0x004000, CRC(6ee7f3cc) SHA1(b86676340b06f07c164690862c1f6f75f30c080b) )
	ROM_LOAD( "119-16s.037",  0x008000, 0x008000, CRC(fa19861f) SHA1(7568b4ab526bd5849f7ef70dfa6d1ef1f30c0abc) )

	ROM_REGION( 0x04000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "104-6p.037",   0x000000, 0x002000, CRC(9e2a5b59) SHA1(3bbaa92e4612b0a26837ca67cfa4e36d0f2295fa) )

	ROM_REGION( 0x40000, REGION_GFX2, ROMREGION_DISPOSE | ROMREGION_INVERT )
	ROM_LOAD( "111-1a.037",   0x000000, 0x008000, CRC(91700f33) SHA1(fac1ce700c4cd46b643307998df781d637f193aa) )
	ROM_LOAD( "112-1b.037",   0x008000, 0x008000, CRC(869330be) SHA1(5dfaaf54ee2b3c0eaf35e8c17558313db9791616) )
	ROM_LOAD( "113-1l.037",   0x010000, 0x008000, CRC(d497d0a8) SHA1(bb715bcec7f783dd04151e2e3b221a72133bf17d) )
	ROM_LOAD( "114-1mn.037",  0x018000, 0x008000, CRC(29ef9882) SHA1(91e1465af6505b35cd97434c13d2b4d40a085946) )
	ROM_LOAD( "115-2a.037",   0x020000, 0x008000, CRC(9510b898) SHA1(e6c8c7af1898d548f0f01e4ff37c2c7b22c0b5c2) )
	ROM_LOAD( "116-2b.037",   0x028000, 0x008000, CRC(11e0ac5b) SHA1(729b7561d59d94ef33874a134b97bcd37573dfa6) )
	ROM_LOAD( "117-2l.037",   0x030000, 0x008000, CRC(29a5db41) SHA1(94f4f5dd39e724570a0f54af176ad018497697fd) )
	ROM_LOAD( "118-2mn.037",  0x038000, 0x008000, CRC(8bf3b263) SHA1(683d900ab7591ee661218be2406fb375a12e435c) )
ROM_END


ROM_START( gauntj )
	ROM_REGION( 0x80000, REGION_CPU1, 0 )	/* 8*64k for 68000 code */
	ROM_LOAD16_BYTE( "1307-9a.037",  0x000000, 0x008000, CRC(46fe8743) SHA1(d5fa19e028a2f43658330c67c10e0c811d332780) )
	ROM_LOAD16_BYTE( "1308-9b.037",  0x000001, 0x008000, CRC(276e15c4) SHA1(7467b2ec21b1b4fcc18ff9387ce891495f4b064c) )
	ROM_LOAD16_BYTE( "205-10a.037",  0x038000, 0x004000, CRC(6d99ed51) SHA1(a7bc18f32908451859ba5cdf1a5c97ecc5fe325f) )
	ROM_LOAD16_BYTE( "206-10b.037",  0x038001, 0x004000, CRC(545ead91) SHA1(7fad5a63c6443249bb6dad5b2a1fd08ca5f11e10) )
	ROM_LOAD16_BYTE( "1309-7a.037",  0x040000, 0x008000, CRC(e8ba39d8) SHA1(9ad68617df0ae655b5e1e40ed7b6d205f4c0443d) )
	ROM_LOAD16_BYTE( "1310-7b.037",  0x040001, 0x008000, CRC(a204d997) SHA1(c8fe0ea04ce35bc83fe5abd16e0a3df8f5456bfe) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for 6502 code */
	ROM_LOAD( "120-16r.037",  0x004000, 0x004000, CRC(6ee7f3cc) SHA1(b86676340b06f07c164690862c1f6f75f30c080b) )
	ROM_LOAD( "119-16s.037",  0x008000, 0x008000, CRC(fa19861f) SHA1(7568b4ab526bd5849f7ef70dfa6d1ef1f30c0abc) )

	ROM_REGION( 0x04000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "104-6p.037",   0x000000, 0x002000, CRC(9e2a5b59) SHA1(3bbaa92e4612b0a26837ca67cfa4e36d0f2295fa) )

	ROM_REGION( 0x40000, REGION_GFX2, ROMREGION_DISPOSE | ROMREGION_INVERT )
	ROM_LOAD( "111-1a.037",   0x000000, 0x008000, CRC(91700f33) SHA1(fac1ce700c4cd46b643307998df781d637f193aa) )
	ROM_LOAD( "112-1b.037",   0x008000, 0x008000, CRC(869330be) SHA1(5dfaaf54ee2b3c0eaf35e8c17558313db9791616) )
	ROM_LOAD( "113-1l.037",   0x010000, 0x008000, CRC(d497d0a8) SHA1(bb715bcec7f783dd04151e2e3b221a72133bf17d) )
	ROM_LOAD( "114-1mn.037",  0x018000, 0x008000, CRC(29ef9882) SHA1(91e1465af6505b35cd97434c13d2b4d40a085946) )
	ROM_LOAD( "115-2a.037",   0x020000, 0x008000, CRC(9510b898) SHA1(e6c8c7af1898d548f0f01e4ff37c2c7b22c0b5c2) )
	ROM_LOAD( "116-2b.037",   0x028000, 0x008000, CRC(11e0ac5b) SHA1(729b7561d59d94ef33874a134b97bcd37573dfa6) )
	ROM_LOAD( "117-2l.037",   0x030000, 0x008000, CRC(29a5db41) SHA1(94f4f5dd39e724570a0f54af176ad018497697fd) )
	ROM_LOAD( "118-2mn.037",  0x038000, 0x008000, CRC(8bf3b263) SHA1(683d900ab7591ee661218be2406fb375a12e435c) )
ROM_END


ROM_START( gauntj12 )
	ROM_REGION( 0x80000, REGION_CPU1, 0 )	/* 8*64k for 68000 code */
	ROM_LOAD16_BYTE( "1207-9a.037",  0x000000, 0x008000, CRC(6dc0610d) SHA1(6f810a8ac1c753b2fd24e6b008f0cdf82e9e0831) )
	ROM_LOAD16_BYTE( "1208-9b.037",  0x000001, 0x008000, CRC(faa306eb) SHA1(48c5632a365b4c3df8f424d06229f10b608edfa5) )
	ROM_LOAD16_BYTE( "205-10a.037",  0x038000, 0x004000, CRC(6d99ed51) SHA1(a7bc18f32908451859ba5cdf1a5c97ecc5fe325f) )
	ROM_LOAD16_BYTE( "206-10b.037",  0x038001, 0x004000, CRC(545ead91) SHA1(7fad5a63c6443249bb6dad5b2a1fd08ca5f11e10) )
	ROM_LOAD16_BYTE( "1109-7a.037",  0x040000, 0x008000, CRC(500194fb) SHA1(348f8702cd9ca4552c5e61f9386f916ff2da9b20) )
	ROM_LOAD16_BYTE( "1110-7b.037",  0x040001, 0x008000, CRC(b2969076) SHA1(d7508ac30e17ba93cd01000fc3132543762c6430) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for 6502 code */
	ROM_LOAD( "120-16r.037",  0x004000, 0x004000, CRC(6ee7f3cc) SHA1(b86676340b06f07c164690862c1f6f75f30c080b) )
	ROM_LOAD( "119-16s.037",  0x008000, 0x008000, CRC(fa19861f) SHA1(7568b4ab526bd5849f7ef70dfa6d1ef1f30c0abc) )

	ROM_REGION( 0x04000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "104-6p.037",   0x000000, 0x002000, CRC(9e2a5b59) SHA1(3bbaa92e4612b0a26837ca67cfa4e36d0f2295fa) )

	ROM_REGION( 0x40000, REGION_GFX2, ROMREGION_DISPOSE | ROMREGION_INVERT )
	ROM_LOAD( "111-1a.037",   0x000000, 0x008000, CRC(91700f33) SHA1(fac1ce700c4cd46b643307998df781d637f193aa) )
	ROM_LOAD( "112-1b.037",   0x008000, 0x008000, CRC(869330be) SHA1(5dfaaf54ee2b3c0eaf35e8c17558313db9791616) )
	ROM_LOAD( "113-1l.037",   0x010000, 0x008000, CRC(d497d0a8) SHA1(bb715bcec7f783dd04151e2e3b221a72133bf17d) )
	ROM_LOAD( "114-1mn.037",  0x018000, 0x008000, CRC(29ef9882) SHA1(91e1465af6505b35cd97434c13d2b4d40a085946) )
	ROM_LOAD( "115-2a.037",   0x020000, 0x008000, CRC(9510b898) SHA1(e6c8c7af1898d548f0f01e4ff37c2c7b22c0b5c2) )
	ROM_LOAD( "116-2b.037",   0x028000, 0x008000, CRC(11e0ac5b) SHA1(729b7561d59d94ef33874a134b97bcd37573dfa6) )
	ROM_LOAD( "117-2l.037",   0x030000, 0x008000, CRC(29a5db41) SHA1(94f4f5dd39e724570a0f54af176ad018497697fd) )
	ROM_LOAD( "118-2mn.037",  0x038000, 0x008000, CRC(8bf3b263) SHA1(683d900ab7591ee661218be2406fb375a12e435c) )
ROM_END


ROM_START( gauntg )
	ROM_REGION( 0x80000, REGION_CPU1, 0 )	/* 8*64k for 68000 code */
	ROM_LOAD16_BYTE( "1007-9a.037",  0x000000, 0x008000, CRC(6a224cea) SHA1(1d9205a1587a39b3bc6da1813e380a8babee2994) )
	ROM_LOAD16_BYTE( "1008-9b.037",  0x000001, 0x008000, CRC(fa391dab) SHA1(7dcb67fa969b437fe2474daeb3c7c3652df2ff5d) )
	ROM_LOAD16_BYTE( "205-10a.037",  0x038000, 0x004000, CRC(6d99ed51) SHA1(a7bc18f32908451859ba5cdf1a5c97ecc5fe325f) )
	ROM_LOAD16_BYTE( "206-10b.037",  0x038001, 0x004000, CRC(545ead91) SHA1(7fad5a63c6443249bb6dad5b2a1fd08ca5f11e10) )
	ROM_LOAD16_BYTE( "1009-7a.037",  0x040000, 0x008000, CRC(75d1f966) SHA1(4f04d9ab082f6984bf11b83ce20a109a923652cd) )
	ROM_LOAD16_BYTE( "1010-7b.037",  0x040001, 0x008000, CRC(28a4197b) SHA1(20668f17b53dfef3044581ee340fbc04df33d419) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for 6502 code */
	ROM_LOAD( "120-16r.037",  0x004000, 0x004000, CRC(6ee7f3cc) SHA1(b86676340b06f07c164690862c1f6f75f30c080b) )
	ROM_LOAD( "119-16s.037",  0x008000, 0x008000, CRC(fa19861f) SHA1(7568b4ab526bd5849f7ef70dfa6d1ef1f30c0abc) )

	ROM_REGION( 0x04000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "104-6p.037",   0x000000, 0x002000, CRC(9e2a5b59) SHA1(3bbaa92e4612b0a26837ca67cfa4e36d0f2295fa) )

	ROM_REGION( 0x40000, REGION_GFX2, ROMREGION_DISPOSE | ROMREGION_INVERT )
	ROM_LOAD( "111-1a.037",   0x000000, 0x008000, CRC(91700f33) SHA1(fac1ce700c4cd46b643307998df781d637f193aa) )
	ROM_LOAD( "112-1b.037",   0x008000, 0x008000, CRC(869330be) SHA1(5dfaaf54ee2b3c0eaf35e8c17558313db9791616) )
	ROM_LOAD( "113-1l.037",   0x010000, 0x008000, CRC(d497d0a8) SHA1(bb715bcec7f783dd04151e2e3b221a72133bf17d) )
	ROM_LOAD( "114-1mn.037",  0x018000, 0x008000, CRC(29ef9882) SHA1(91e1465af6505b35cd97434c13d2b4d40a085946) )
	ROM_LOAD( "115-2a.037",   0x020000, 0x008000, CRC(9510b898) SHA1(e6c8c7af1898d548f0f01e4ff37c2c7b22c0b5c2) )
	ROM_LOAD( "116-2b.037",   0x028000, 0x008000, CRC(11e0ac5b) SHA1(729b7561d59d94ef33874a134b97bcd37573dfa6) )
	ROM_LOAD( "117-2l.037",   0x030000, 0x008000, CRC(29a5db41) SHA1(94f4f5dd39e724570a0f54af176ad018497697fd) )
	ROM_LOAD( "118-2mn.037",  0x038000, 0x008000, CRC(8bf3b263) SHA1(683d900ab7591ee661218be2406fb375a12e435c) )
ROM_END


ROM_START( gauntr9 )
	ROM_REGION( 0x80000, REGION_CPU1, 0 )	/* 8*64k for 68000 code */
	ROM_LOAD16_BYTE( "907-9a.037",   0x000000, 0x008000, CRC(c13a6399) SHA1(569c8eac81ec7d0ea451b73888efd5dce4d4906d) )
	ROM_LOAD16_BYTE( "908-9b.037",   0x000001, 0x008000, CRC(417607d9) SHA1(b168773d5868adc9b8d860f32d847bb525d9069f) )
	ROM_LOAD16_BYTE( "105-10a.037",  0x038000, 0x004000, CRC(4642cd95) SHA1(96ff5a28a8ccd80d1a09bd1c5ce038ce5b400ac7) )
	ROM_LOAD16_BYTE( "106-10b.037",  0x038001, 0x004000, CRC(c8df945e) SHA1(71d675aaed7e128bd5fd9b137ddd1b1751ecf681) )
	ROM_LOAD16_BYTE( "909-7a.037",   0x040000, 0x008000, CRC(fb1cdc1c) SHA1(d26b1941a1f903e0df36c880c0955be9b5126083) )
	ROM_LOAD16_BYTE( "910-7b.037",   0x040001, 0x008000, CRC(f188e7b3) SHA1(1b696dbf9fdae24e462015738561b2cc7aac2a9f) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for 6502 code */
	ROM_LOAD( "120-16r.037",  0x004000, 0x004000, CRC(6ee7f3cc) SHA1(b86676340b06f07c164690862c1f6f75f30c080b) )
	ROM_LOAD( "119-16s.037",  0x008000, 0x008000, CRC(fa19861f) SHA1(7568b4ab526bd5849f7ef70dfa6d1ef1f30c0abc) )

	ROM_REGION( 0x04000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "104-6p.037",   0x000000, 0x002000, CRC(9e2a5b59) SHA1(3bbaa92e4612b0a26837ca67cfa4e36d0f2295fa) )

	ROM_REGION( 0x40000, REGION_GFX2, ROMREGION_DISPOSE | ROMREGION_INVERT )
	ROM_LOAD( "111-1a.037",   0x000000, 0x008000, CRC(91700f33) SHA1(fac1ce700c4cd46b643307998df781d637f193aa) )
	ROM_LOAD( "112-1b.037",   0x008000, 0x008000, CRC(869330be) SHA1(5dfaaf54ee2b3c0eaf35e8c17558313db9791616) )
	ROM_LOAD( "113-1l.037",   0x010000, 0x008000, CRC(d497d0a8) SHA1(bb715bcec7f783dd04151e2e3b221a72133bf17d) )
	ROM_LOAD( "114-1mn.037",  0x018000, 0x008000, CRC(29ef9882) SHA1(91e1465af6505b35cd97434c13d2b4d40a085946) )
	ROM_LOAD( "115-2a.037",   0x020000, 0x008000, CRC(9510b898) SHA1(e6c8c7af1898d548f0f01e4ff37c2c7b22c0b5c2) )
	ROM_LOAD( "116-2b.037",   0x028000, 0x008000, CRC(11e0ac5b) SHA1(729b7561d59d94ef33874a134b97bcd37573dfa6) )
	ROM_LOAD( "117-2l.037",   0x030000, 0x008000, CRC(29a5db41) SHA1(94f4f5dd39e724570a0f54af176ad018497697fd) )
	ROM_LOAD( "118-2mn.037",  0x038000, 0x008000, CRC(8bf3b263) SHA1(683d900ab7591ee661218be2406fb375a12e435c) )
ROM_END


ROM_START( gauntgr8 )
	ROM_REGION( 0x80000, REGION_CPU1, 0 )	/* 8*64k for 68000 code */
	ROM_LOAD16_BYTE( "807-9a.037",   0x000000, 0x008000, CRC(671c0bc2) SHA1(73c8249bac8a131b2fb93fc4ac7235b3f329b987) )
	ROM_LOAD16_BYTE( "808-9b.037",   0x000001, 0x008000, CRC(f2842af4) SHA1(8ecaec141f21b26647b2f2fd224c92b8a36acbad) )
	ROM_LOAD16_BYTE( "105-10a.037",  0x038000, 0x004000, CRC(4642cd95) SHA1(96ff5a28a8ccd80d1a09bd1c5ce038ce5b400ac7) )
	ROM_LOAD16_BYTE( "106-10b.037",  0x038001, 0x004000, CRC(c8df945e) SHA1(71d675aaed7e128bd5fd9b137ddd1b1751ecf681) )
	ROM_LOAD16_BYTE( "809-7a.037",   0x040000, 0x008000, CRC(05642d60) SHA1(c008325635e086b7c0bb259c40b44d204eaf4392) )
	ROM_LOAD16_BYTE( "810-7b.037",   0x040001, 0x008000, CRC(36d295e3) SHA1(536e5dfb12b1ead92140edc4a36f44914e77677e) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for 6502 code */
	ROM_LOAD( "120-16r.037",  0x004000, 0x004000, CRC(6ee7f3cc) SHA1(b86676340b06f07c164690862c1f6f75f30c080b) )
	ROM_LOAD( "119-16s.037",  0x008000, 0x008000, CRC(fa19861f) SHA1(7568b4ab526bd5849f7ef70dfa6d1ef1f30c0abc) )

	ROM_REGION( 0x04000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "104-6p.037",   0x000000, 0x002000, CRC(9e2a5b59) SHA1(3bbaa92e4612b0a26837ca67cfa4e36d0f2295fa) )

	ROM_REGION( 0x40000, REGION_GFX2, ROMREGION_DISPOSE | ROMREGION_INVERT )
	ROM_LOAD( "111-1a.037",   0x000000, 0x008000, CRC(91700f33) SHA1(fac1ce700c4cd46b643307998df781d637f193aa) )
	ROM_LOAD( "112-1b.037",   0x008000, 0x008000, CRC(869330be) SHA1(5dfaaf54ee2b3c0eaf35e8c17558313db9791616) )
	ROM_LOAD( "113-1l.037",   0x010000, 0x008000, CRC(d497d0a8) SHA1(bb715bcec7f783dd04151e2e3b221a72133bf17d) )
	ROM_LOAD( "114-1mn.037",  0x018000, 0x008000, CRC(29ef9882) SHA1(91e1465af6505b35cd97434c13d2b4d40a085946) )
	ROM_LOAD( "115-2a.037",   0x020000, 0x008000, CRC(9510b898) SHA1(e6c8c7af1898d548f0f01e4ff37c2c7b22c0b5c2) )
	ROM_LOAD( "116-2b.037",   0x028000, 0x008000, CRC(11e0ac5b) SHA1(729b7561d59d94ef33874a134b97bcd37573dfa6) )
	ROM_LOAD( "117-2l.037",   0x030000, 0x008000, CRC(29a5db41) SHA1(94f4f5dd39e724570a0f54af176ad018497697fd) )
	ROM_LOAD( "118-2mn.037",  0x038000, 0x008000, CRC(8bf3b263) SHA1(683d900ab7591ee661218be2406fb375a12e435c) )
ROM_END


ROM_START( gauntr7 )
	ROM_REGION( 0x80000, REGION_CPU1, 0 )	/* 8*64k for 68000 code */
	ROM_LOAD16_BYTE( "207-9a.037",   0x000000, 0x008000, CRC(fd871f81) SHA1(111615cb3990fe2121ed5b3dd0c28054c98ef665) )
	ROM_LOAD16_BYTE( "208-9b.037",   0x000001, 0x008000, CRC(bcb2fb1d) SHA1(62f2acf81d8094617e4fcaa427e47c5940d85ad2) )
	ROM_LOAD16_BYTE( "105-10a.037",  0x038000, 0x004000, CRC(4642cd95) SHA1(96ff5a28a8ccd80d1a09bd1c5ce038ce5b400ac7) )
	ROM_LOAD16_BYTE( "106-10b.037",  0x038001, 0x004000, CRC(c8df945e) SHA1(71d675aaed7e128bd5fd9b137ddd1b1751ecf681) )
	ROM_LOAD16_BYTE( "709-7a.037",   0x040000, 0x008000, CRC(73e1ad79) SHA1(11c17f764cbbe87acca05c9e6179010b09c5a856) )
	ROM_LOAD16_BYTE( "710-7b.037",   0x040001, 0x008000, CRC(fd248cea) SHA1(85db2c3b31fa8d9c8a048f553c3b195b2ff43586) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for 6502 code */
	ROM_LOAD( "120-16r.037",  0x004000, 0x004000, CRC(6ee7f3cc) SHA1(b86676340b06f07c164690862c1f6f75f30c080b) )
	ROM_LOAD( "119-16s.037",  0x008000, 0x008000, CRC(fa19861f) SHA1(7568b4ab526bd5849f7ef70dfa6d1ef1f30c0abc) )

	ROM_REGION( 0x04000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "104-6p.037",   0x000000, 0x002000, CRC(9e2a5b59) SHA1(3bbaa92e4612b0a26837ca67cfa4e36d0f2295fa) )

	ROM_REGION( 0x40000, REGION_GFX2, ROMREGION_DISPOSE | ROMREGION_INVERT )
	ROM_LOAD( "111-1a.037",   0x000000, 0x008000, CRC(91700f33) SHA1(fac1ce700c4cd46b643307998df781d637f193aa) )
	ROM_LOAD( "112-1b.037",   0x008000, 0x008000, CRC(869330be) SHA1(5dfaaf54ee2b3c0eaf35e8c17558313db9791616) )
	ROM_LOAD( "113-1l.037",   0x010000, 0x008000, CRC(d497d0a8) SHA1(bb715bcec7f783dd04151e2e3b221a72133bf17d) )
	ROM_LOAD( "114-1mn.037",  0x018000, 0x008000, CRC(29ef9882) SHA1(91e1465af6505b35cd97434c13d2b4d40a085946) )
	ROM_LOAD( "115-2a.037",   0x020000, 0x008000, CRC(9510b898) SHA1(e6c8c7af1898d548f0f01e4ff37c2c7b22c0b5c2) )
	ROM_LOAD( "116-2b.037",   0x028000, 0x008000, CRC(11e0ac5b) SHA1(729b7561d59d94ef33874a134b97bcd37573dfa6) )
	ROM_LOAD( "117-2l.037",   0x030000, 0x008000, CRC(29a5db41) SHA1(94f4f5dd39e724570a0f54af176ad018497697fd) )
	ROM_LOAD( "118-2mn.037",  0x038000, 0x008000, CRC(8bf3b263) SHA1(683d900ab7591ee661218be2406fb375a12e435c) )
ROM_END


ROM_START( gauntgr6 )
	ROM_REGION( 0x80000, REGION_CPU1, 0 )	/* 8*64k for 68000 code */
	ROM_LOAD16_BYTE( "307-9a.037",   0x000000, 0x008000, CRC(759827c9) SHA1(d267e2416365814cd9a2b2c587edc8334031b77f) )
	ROM_LOAD16_BYTE( "308-9b.037",   0x000001, 0x008000, CRC(d71262d1) SHA1(cc7f64f75d325b0531c3ee509d3eb1159a149b81) )
	ROM_LOAD16_BYTE( "105-10a.037",  0x038000, 0x004000, CRC(4642cd95) SHA1(96ff5a28a8ccd80d1a09bd1c5ce038ce5b400ac7) )
	ROM_LOAD16_BYTE( "106-10b.037",  0x038001, 0x004000, CRC(c8df945e) SHA1(71d675aaed7e128bd5fd9b137ddd1b1751ecf681) )
	ROM_LOAD16_BYTE( "609-7a.037",   0x040000, 0x008000, CRC(cd3381de) SHA1(15ec837f9dc55575b0da7169d36da991dc9b3c41) )
	ROM_LOAD16_BYTE( "610-7b.037",   0x040001, 0x008000, CRC(2cff932a) SHA1(13567150fabfe9878d902d6580edcc84100b10b2) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for 6502 code */
	ROM_LOAD( "120-16r.037",  0x004000, 0x004000, CRC(6ee7f3cc) SHA1(b86676340b06f07c164690862c1f6f75f30c080b) )
	ROM_LOAD( "119-16s.037",  0x008000, 0x008000, CRC(fa19861f) SHA1(7568b4ab526bd5849f7ef70dfa6d1ef1f30c0abc) )

	ROM_REGION( 0x04000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "104-6p.037",   0x000000, 0x002000, CRC(9e2a5b59) SHA1(3bbaa92e4612b0a26837ca67cfa4e36d0f2295fa) )

	ROM_REGION( 0x40000, REGION_GFX2, ROMREGION_DISPOSE | ROMREGION_INVERT )
	ROM_LOAD( "111-1a.037",   0x000000, 0x008000, CRC(91700f33) SHA1(fac1ce700c4cd46b643307998df781d637f193aa) )
	ROM_LOAD( "112-1b.037",   0x008000, 0x008000, CRC(869330be) SHA1(5dfaaf54ee2b3c0eaf35e8c17558313db9791616) )
	ROM_LOAD( "113-1l.037",   0x010000, 0x008000, CRC(d497d0a8) SHA1(bb715bcec7f783dd04151e2e3b221a72133bf17d) )
	ROM_LOAD( "114-1mn.037",  0x018000, 0x008000, CRC(29ef9882) SHA1(91e1465af6505b35cd97434c13d2b4d40a085946) )
	ROM_LOAD( "115-2a.037",   0x020000, 0x008000, CRC(9510b898) SHA1(e6c8c7af1898d548f0f01e4ff37c2c7b22c0b5c2) )
	ROM_LOAD( "116-2b.037",   0x028000, 0x008000, CRC(11e0ac5b) SHA1(729b7561d59d94ef33874a134b97bcd37573dfa6) )
	ROM_LOAD( "117-2l.037",   0x030000, 0x008000, CRC(29a5db41) SHA1(94f4f5dd39e724570a0f54af176ad018497697fd) )
	ROM_LOAD( "118-2mn.037",  0x038000, 0x008000, CRC(8bf3b263) SHA1(683d900ab7591ee661218be2406fb375a12e435c) )
ROM_END


ROM_START( gauntr5 )
	ROM_REGION( 0x80000, REGION_CPU1, 0 )	/* 8*64k for 68000 code */
	ROM_LOAD16_BYTE( "207-9a.037",   0x000000, 0x008000, CRC(fd871f81) SHA1(111615cb3990fe2121ed5b3dd0c28054c98ef665) )
	ROM_LOAD16_BYTE( "208-9b.037",   0x000001, 0x008000, CRC(bcb2fb1d) SHA1(62f2acf81d8094617e4fcaa427e47c5940d85ad2) )
	ROM_LOAD16_BYTE( "105-10a.037",  0x038000, 0x004000, CRC(4642cd95) SHA1(96ff5a28a8ccd80d1a09bd1c5ce038ce5b400ac7) )
	ROM_LOAD16_BYTE( "106-10b.037",  0x038001, 0x004000, CRC(c8df945e) SHA1(71d675aaed7e128bd5fd9b137ddd1b1751ecf681) )
	ROM_LOAD16_BYTE( "409-7a.037",   0x040000, 0x008000, CRC(c57377b3) SHA1(4e7bf488240ec85ed4efd76a69d77f0308459ee5) )
	ROM_LOAD16_BYTE( "510-7b.037",   0x040001, 0x008000, CRC(1cac2071) SHA1(e8038c00e17dea6df6bd251505e525e3ef1a4c80) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for 6502 code */
	ROM_LOAD( "120-16r.037",  0x004000, 0x004000, CRC(6ee7f3cc) SHA1(b86676340b06f07c164690862c1f6f75f30c080b) )
	ROM_LOAD( "119-16s.037",  0x008000, 0x008000, CRC(fa19861f) SHA1(7568b4ab526bd5849f7ef70dfa6d1ef1f30c0abc) )

	ROM_REGION( 0x04000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "104-6p.037",   0x000000, 0x002000, CRC(9e2a5b59) SHA1(3bbaa92e4612b0a26837ca67cfa4e36d0f2295fa) )

	ROM_REGION( 0x40000, REGION_GFX2, ROMREGION_DISPOSE | ROMREGION_INVERT )
	ROM_LOAD( "111-1a.037",   0x000000, 0x008000, CRC(91700f33) SHA1(fac1ce700c4cd46b643307998df781d637f193aa) )
	ROM_LOAD( "112-1b.037",   0x008000, 0x008000, CRC(869330be) SHA1(5dfaaf54ee2b3c0eaf35e8c17558313db9791616) )
	ROM_LOAD( "113-1l.037",   0x010000, 0x008000, CRC(d497d0a8) SHA1(bb715bcec7f783dd04151e2e3b221a72133bf17d) )
	ROM_LOAD( "114-1mn.037",  0x018000, 0x008000, CRC(29ef9882) SHA1(91e1465af6505b35cd97434c13d2b4d40a085946) )
	ROM_LOAD( "115-2a.037",   0x020000, 0x008000, CRC(9510b898) SHA1(e6c8c7af1898d548f0f01e4ff37c2c7b22c0b5c2) )
	ROM_LOAD( "116-2b.037",   0x028000, 0x008000, CRC(11e0ac5b) SHA1(729b7561d59d94ef33874a134b97bcd37573dfa6) )
	ROM_LOAD( "117-2l.037",   0x030000, 0x008000, CRC(29a5db41) SHA1(94f4f5dd39e724570a0f54af176ad018497697fd) )
	ROM_LOAD( "118-2mn.037",  0x038000, 0x008000, CRC(8bf3b263) SHA1(683d900ab7591ee661218be2406fb375a12e435c) )
ROM_END


ROM_START( gauntr4 )
	ROM_REGION( 0x80000, REGION_CPU1, 0 )	/* 8*64k for 68000 code */
	ROM_LOAD16_BYTE( "207-9a.037",   0x000000, 0x008000, CRC(fd871f81) SHA1(111615cb3990fe2121ed5b3dd0c28054c98ef665) )
	ROM_LOAD16_BYTE( "208-9b.037",   0x000001, 0x008000, CRC(bcb2fb1d) SHA1(62f2acf81d8094617e4fcaa427e47c5940d85ad2) )
	ROM_LOAD16_BYTE( "105-10a.037",  0x038000, 0x004000, CRC(4642cd95) SHA1(96ff5a28a8ccd80d1a09bd1c5ce038ce5b400ac7) )
	ROM_LOAD16_BYTE( "106-10b.037",  0x038001, 0x004000, CRC(c8df945e) SHA1(71d675aaed7e128bd5fd9b137ddd1b1751ecf681) )
	ROM_LOAD16_BYTE( "409-7a.037",   0x040000, 0x008000, CRC(c57377b3) SHA1(4e7bf488240ec85ed4efd76a69d77f0308459ee5) )
	ROM_LOAD16_BYTE( "410-7b.037",   0x040001, 0x008000, CRC(6b971a27) SHA1(1ceb64ac5d0cb68abc05618637e183f3f87381c7) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for 6502 code */
	ROM_LOAD( "120-16r.037",  0x004000, 0x004000, CRC(6ee7f3cc) SHA1(b86676340b06f07c164690862c1f6f75f30c080b) )
	ROM_LOAD( "119-16s.037",  0x008000, 0x008000, CRC(fa19861f) SHA1(7568b4ab526bd5849f7ef70dfa6d1ef1f30c0abc) )

	ROM_REGION( 0x04000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "104-6p.037",   0x000000, 0x002000, CRC(9e2a5b59) SHA1(3bbaa92e4612b0a26837ca67cfa4e36d0f2295fa) )

	ROM_REGION( 0x40000, REGION_GFX2, ROMREGION_DISPOSE | ROMREGION_INVERT )
	ROM_LOAD( "111-1a.037",   0x000000, 0x008000, CRC(91700f33) SHA1(fac1ce700c4cd46b643307998df781d637f193aa) )
	ROM_LOAD( "112-1b.037",   0x008000, 0x008000, CRC(869330be) SHA1(5dfaaf54ee2b3c0eaf35e8c17558313db9791616) )
	ROM_LOAD( "113-1l.037",   0x010000, 0x008000, CRC(d497d0a8) SHA1(bb715bcec7f783dd04151e2e3b221a72133bf17d) )
	ROM_LOAD( "114-1mn.037",  0x018000, 0x008000, CRC(29ef9882) SHA1(91e1465af6505b35cd97434c13d2b4d40a085946) )
	ROM_LOAD( "115-2a.037",   0x020000, 0x008000, CRC(9510b898) SHA1(e6c8c7af1898d548f0f01e4ff37c2c7b22c0b5c2) )
	ROM_LOAD( "116-2b.037",   0x028000, 0x008000, CRC(11e0ac5b) SHA1(729b7561d59d94ef33874a134b97bcd37573dfa6) )
	ROM_LOAD( "117-2l.037",   0x030000, 0x008000, CRC(29a5db41) SHA1(94f4f5dd39e724570a0f54af176ad018497697fd) )
	ROM_LOAD( "118-2mn.037",  0x038000, 0x008000, CRC(8bf3b263) SHA1(683d900ab7591ee661218be2406fb375a12e435c) )
ROM_END


ROM_START( gauntgr3 )
	ROM_REGION( 0x80000, REGION_CPU1, 0 )	/* 8*64k for 68000 code */
	ROM_LOAD16_BYTE( "307-9a.037",   0x000000, 0x008000, CRC(759827c9) SHA1(d267e2416365814cd9a2b2c587edc8334031b77f) )
	ROM_LOAD16_BYTE( "308-9b.037",   0x000001, 0x008000, CRC(d71262d1) SHA1(cc7f64f75d325b0531c3ee509d3eb1159a149b81) )
	ROM_LOAD16_BYTE( "105-10a.037",  0x038000, 0x004000, CRC(4642cd95) SHA1(96ff5a28a8ccd80d1a09bd1c5ce038ce5b400ac7) )
	ROM_LOAD16_BYTE( "106-10b.037",  0x038001, 0x004000, CRC(c8df945e) SHA1(71d675aaed7e128bd5fd9b137ddd1b1751ecf681) )
	ROM_LOAD16_BYTE( "309-7a.037",   0x040000, 0x008000, CRC(7f03696b) SHA1(be1ffc8aa1bd8230c69247716a5a1c3a83dda040) )
	ROM_LOAD16_BYTE( "310-7b.037",   0x040001, 0x008000, CRC(8d7197fc) SHA1(c1233973ee2210743ed759d44f6e6b24784d8556) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for 6502 code */
	ROM_LOAD( "120-16r.037",  0x004000, 0x004000, CRC(6ee7f3cc) SHA1(b86676340b06f07c164690862c1f6f75f30c080b) )
	ROM_LOAD( "119-16s.037",  0x008000, 0x008000, CRC(fa19861f) SHA1(7568b4ab526bd5849f7ef70dfa6d1ef1f30c0abc) )

	ROM_REGION( 0x04000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "104-6p.037",   0x000000, 0x002000, CRC(9e2a5b59) SHA1(3bbaa92e4612b0a26837ca67cfa4e36d0f2295fa) )

	ROM_REGION( 0x40000, REGION_GFX2, ROMREGION_DISPOSE | ROMREGION_INVERT )
	ROM_LOAD( "111-1a.037",   0x000000, 0x008000, CRC(91700f33) SHA1(fac1ce700c4cd46b643307998df781d637f193aa) )
	ROM_LOAD( "112-1b.037",   0x008000, 0x008000, CRC(869330be) SHA1(5dfaaf54ee2b3c0eaf35e8c17558313db9791616) )
	ROM_LOAD( "113-1l.037",   0x010000, 0x008000, CRC(d497d0a8) SHA1(bb715bcec7f783dd04151e2e3b221a72133bf17d) )
	ROM_LOAD( "114-1mn.037",  0x018000, 0x008000, CRC(29ef9882) SHA1(91e1465af6505b35cd97434c13d2b4d40a085946) )
	ROM_LOAD( "115-2a.037",   0x020000, 0x008000, CRC(9510b898) SHA1(e6c8c7af1898d548f0f01e4ff37c2c7b22c0b5c2) )
	ROM_LOAD( "116-2b.037",   0x028000, 0x008000, CRC(11e0ac5b) SHA1(729b7561d59d94ef33874a134b97bcd37573dfa6) )
	ROM_LOAD( "117-2l.037",   0x030000, 0x008000, CRC(29a5db41) SHA1(94f4f5dd39e724570a0f54af176ad018497697fd) )
	ROM_LOAD( "118-2mn.037",  0x038000, 0x008000, CRC(8bf3b263) SHA1(683d900ab7591ee661218be2406fb375a12e435c) )
ROM_END


ROM_START( gauntr2 )
	ROM_REGION( 0x80000, REGION_CPU1, 0 )	/* 8*64k for 68000 code */
	ROM_LOAD16_BYTE( "207-9a.037",   0x000000, 0x008000, CRC(fd871f81) SHA1(111615cb3990fe2121ed5b3dd0c28054c98ef665) )
	ROM_LOAD16_BYTE( "208-9b.037",   0x000001, 0x008000, CRC(bcb2fb1d) SHA1(62f2acf81d8094617e4fcaa427e47c5940d85ad2) )
	ROM_LOAD16_BYTE( "105-10a.037",  0x038000, 0x004000, CRC(4642cd95) SHA1(96ff5a28a8ccd80d1a09bd1c5ce038ce5b400ac7) )
	ROM_LOAD16_BYTE( "106-10b.037",  0x038001, 0x004000, CRC(c8df945e) SHA1(71d675aaed7e128bd5fd9b137ddd1b1751ecf681) )
	ROM_LOAD16_BYTE( "209-7a.037",   0x040000, 0x008000, CRC(d810a7dc) SHA1(a9b41c11c93a28e6672d91e3107c757fe1ca48dc) )
	ROM_LOAD16_BYTE( "210-7b.037",   0x040001, 0x008000, CRC(fbba7290) SHA1(bbf629e7a803b5e39e29930808a34e8a118b1806) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for 6502 code */
	ROM_LOAD( "120-16r.037",  0x004000, 0x004000, CRC(6ee7f3cc) SHA1(b86676340b06f07c164690862c1f6f75f30c080b) )
	ROM_LOAD( "119-16s.037",  0x008000, 0x008000, CRC(fa19861f) SHA1(7568b4ab526bd5849f7ef70dfa6d1ef1f30c0abc) )

	ROM_REGION( 0x04000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "104-6p.037",   0x000000, 0x002000, CRC(9e2a5b59) SHA1(3bbaa92e4612b0a26837ca67cfa4e36d0f2295fa) )

	ROM_REGION( 0x40000, REGION_GFX2, ROMREGION_DISPOSE | ROMREGION_INVERT )
	ROM_LOAD( "111-1a.037",   0x000000, 0x008000, CRC(91700f33) SHA1(fac1ce700c4cd46b643307998df781d637f193aa) )
	ROM_LOAD( "112-1b.037",   0x008000, 0x008000, CRC(869330be) SHA1(5dfaaf54ee2b3c0eaf35e8c17558313db9791616) )
	ROM_LOAD( "113-1l.037",   0x010000, 0x008000, CRC(d497d0a8) SHA1(bb715bcec7f783dd04151e2e3b221a72133bf17d) )
	ROM_LOAD( "114-1mn.037",  0x018000, 0x008000, CRC(29ef9882) SHA1(91e1465af6505b35cd97434c13d2b4d40a085946) )
	ROM_LOAD( "115-2a.037",   0x020000, 0x008000, CRC(9510b898) SHA1(e6c8c7af1898d548f0f01e4ff37c2c7b22c0b5c2) )
	ROM_LOAD( "116-2b.037",   0x028000, 0x008000, CRC(11e0ac5b) SHA1(729b7561d59d94ef33874a134b97bcd37573dfa6) )
	ROM_LOAD( "117-2l.037",   0x030000, 0x008000, CRC(29a5db41) SHA1(94f4f5dd39e724570a0f54af176ad018497697fd) )
	ROM_LOAD( "118-2mn.037",  0x038000, 0x008000, CRC(8bf3b263) SHA1(683d900ab7591ee661218be2406fb375a12e435c) )
ROM_END


ROM_START( gauntr1 )
	ROM_REGION( 0x80000, REGION_CPU1, 0 )	/* 8*64k for 68000 code */
	ROM_LOAD16_BYTE( "107-9a.037",   0x000000, 0x008000, CRC(a5885e14) SHA1(aa49a3bd8352179532d1cbbb27badb6fbe7d3394) )
	ROM_LOAD16_BYTE( "108-9b.037",   0x000001, 0x008000, CRC(0087f1ab) SHA1(d16a44a5ad4faf26df63b91fac813111c9302713) )
	ROM_LOAD16_BYTE( "105-10a.037",  0x038000, 0x004000, CRC(4642cd95) SHA1(96ff5a28a8ccd80d1a09bd1c5ce038ce5b400ac7) )
	ROM_LOAD16_BYTE( "106-10b.037",  0x038001, 0x004000, CRC(c8df945e) SHA1(71d675aaed7e128bd5fd9b137ddd1b1751ecf681) )
	ROM_LOAD16_BYTE( "109-7a.037",   0x040000, 0x008000, CRC(55d87198) SHA1(5ed1b543b9f245680b4eda5e46e524931d1c8804) )
	ROM_LOAD16_BYTE( "110-7b.037",   0x040001, 0x008000, CRC(f84ad06d) SHA1(2a7eacfbd98a27cb82f451944943f5bd21b5ae46) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for 6502 code */
	ROM_LOAD( "120-16r.037",  0x004000, 0x004000, CRC(6ee7f3cc) SHA1(b86676340b06f07c164690862c1f6f75f30c080b) )
	ROM_LOAD( "119-16s.037",  0x008000, 0x008000, CRC(fa19861f) SHA1(7568b4ab526bd5849f7ef70dfa6d1ef1f30c0abc) )

	ROM_REGION( 0x04000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "104-6p.037",   0x000000, 0x002000, CRC(9e2a5b59) SHA1(3bbaa92e4612b0a26837ca67cfa4e36d0f2295fa) )

	ROM_REGION( 0x40000, REGION_GFX2, ROMREGION_DISPOSE | ROMREGION_INVERT )
	ROM_LOAD( "111-1a.037",   0x000000, 0x008000, CRC(91700f33) SHA1(fac1ce700c4cd46b643307998df781d637f193aa) )
	ROM_LOAD( "112-1b.037",   0x008000, 0x008000, CRC(869330be) SHA1(5dfaaf54ee2b3c0eaf35e8c17558313db9791616) )
	ROM_LOAD( "113-1l.037",   0x010000, 0x008000, CRC(d497d0a8) SHA1(bb715bcec7f783dd04151e2e3b221a72133bf17d) )
	ROM_LOAD( "114-1mn.037",  0x018000, 0x008000, CRC(29ef9882) SHA1(91e1465af6505b35cd97434c13d2b4d40a085946) )
	ROM_LOAD( "115-2a.037",   0x020000, 0x008000, CRC(9510b898) SHA1(e6c8c7af1898d548f0f01e4ff37c2c7b22c0b5c2) )
	ROM_LOAD( "116-2b.037",   0x028000, 0x008000, CRC(11e0ac5b) SHA1(729b7561d59d94ef33874a134b97bcd37573dfa6) )
	ROM_LOAD( "117-2l.037",   0x030000, 0x008000, CRC(29a5db41) SHA1(94f4f5dd39e724570a0f54af176ad018497697fd) )
	ROM_LOAD( "118-2mn.037",  0x038000, 0x008000, CRC(8bf3b263) SHA1(683d900ab7591ee661218be2406fb375a12e435c) )
ROM_END


ROM_START( gaunt2p )
	ROM_REGION( 0x80000, REGION_CPU1, 0 )	/* 8*64k for 68000 code */
	ROM_LOAD16_BYTE( "507-9a.041",   0x000000, 0x008000, CRC(8784133f) SHA1(98017427d84209405bb15d95a47bda5e1bd69f45) )
	ROM_LOAD16_BYTE( "508-9b.041",   0x000001, 0x008000, CRC(2843bde3) SHA1(15e480c5245fd407f0fd5f0a3f3189ff18de88b3) )
	ROM_LOAD16_BYTE( "205-10a.037",  0x038000, 0x004000, CRC(6d99ed51) SHA1(a7bc18f32908451859ba5cdf1a5c97ecc5fe325f) )
	ROM_LOAD16_BYTE( "206-10b.037",  0x038001, 0x004000, CRC(545ead91) SHA1(7fad5a63c6443249bb6dad5b2a1fd08ca5f11e10) )
	ROM_LOAD16_BYTE( "609-7a.041",   0x040000, 0x008000, CRC(5b4ee415) SHA1(dd9faba778710a86780b51d13deef1c9ebce0d44) )
	ROM_LOAD16_BYTE( "610-7b.041",   0x040001, 0x008000, CRC(41f5c9e2) SHA1(791609520686ad48aaa76db1b3192ececf0d4e91) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for 6502 code */
	ROM_LOAD( "120-16r.037",  0x004000, 0x004000, CRC(6ee7f3cc) SHA1(b86676340b06f07c164690862c1f6f75f30c080b) )
	ROM_LOAD( "119-16s.037",  0x008000, 0x008000, CRC(fa19861f) SHA1(7568b4ab526bd5849f7ef70dfa6d1ef1f30c0abc) )

	ROM_REGION( 0x04000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "104-6p.037",   0x000000, 0x002000, CRC(9e2a5b59) SHA1(3bbaa92e4612b0a26837ca67cfa4e36d0f2295fa) )

	ROM_REGION( 0x40000, REGION_GFX2, ROMREGION_DISPOSE | ROMREGION_INVERT )
	ROM_LOAD( "111-1a.037",   0x000000, 0x008000, CRC(91700f33) SHA1(fac1ce700c4cd46b643307998df781d637f193aa) )
	ROM_LOAD( "112-1b.037",   0x008000, 0x008000, CRC(869330be) SHA1(5dfaaf54ee2b3c0eaf35e8c17558313db9791616) )
	ROM_LOAD( "113-1l.037",   0x010000, 0x008000, CRC(d497d0a8) SHA1(bb715bcec7f783dd04151e2e3b221a72133bf17d) )
	ROM_LOAD( "114-1mn.037",  0x018000, 0x008000, CRC(29ef9882) SHA1(91e1465af6505b35cd97434c13d2b4d40a085946) )
	ROM_LOAD( "115-2a.037",   0x020000, 0x008000, CRC(9510b898) SHA1(e6c8c7af1898d548f0f01e4ff37c2c7b22c0b5c2) )
	ROM_LOAD( "116-2b.037",   0x028000, 0x008000, CRC(11e0ac5b) SHA1(729b7561d59d94ef33874a134b97bcd37573dfa6) )
	ROM_LOAD( "117-2l.037",   0x030000, 0x008000, CRC(29a5db41) SHA1(94f4f5dd39e724570a0f54af176ad018497697fd) )
	ROM_LOAD( "118-2mn.037",  0x038000, 0x008000, CRC(8bf3b263) SHA1(683d900ab7591ee661218be2406fb375a12e435c) )
ROM_END


ROM_START( gaunt2pj )
	ROM_REGION( 0x80000, REGION_CPU1, 0 )	/* 8*64k for 68000 code */
	ROM_LOAD16_BYTE( "507-9a.041",   0x000000, 0x008000, CRC(8784133f) SHA1(98017427d84209405bb15d95a47bda5e1bd69f45) )
	ROM_LOAD16_BYTE( "508-9b.041",   0x000001, 0x008000, CRC(2843bde3) SHA1(15e480c5245fd407f0fd5f0a3f3189ff18de88b3) )
	ROM_LOAD16_BYTE( "205-10a.037",  0x038000, 0x004000, CRC(6d99ed51) SHA1(a7bc18f32908451859ba5cdf1a5c97ecc5fe325f) )
	ROM_LOAD16_BYTE( "206-10b.037",  0x038001, 0x004000, CRC(545ead91) SHA1(7fad5a63c6443249bb6dad5b2a1fd08ca5f11e10) )
	ROM_LOAD16_BYTE( "509-7a.041",   0x040000, 0x008000, CRC(fb2ef226) SHA1(8527d32b535f7c96b238af47ad808636e9d328f3) )
	ROM_LOAD16_BYTE( "510-7b.041",   0x040001, 0x008000, CRC(a69be8da) SHA1(5b88a63d30e2e916d5b0ff6ac37969d92c031abc) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for 6502 code */
	ROM_LOAD( "120-16r.037",  0x004000, 0x004000, CRC(6ee7f3cc) SHA1(b86676340b06f07c164690862c1f6f75f30c080b) )
	ROM_LOAD( "119-16s.037",  0x008000, 0x008000, CRC(fa19861f) SHA1(7568b4ab526bd5849f7ef70dfa6d1ef1f30c0abc) )

	ROM_REGION( 0x04000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "104-6p.037",   0x000000, 0x002000, CRC(9e2a5b59) SHA1(3bbaa92e4612b0a26837ca67cfa4e36d0f2295fa) )

	ROM_REGION( 0x40000, REGION_GFX2, ROMREGION_DISPOSE | ROMREGION_INVERT )
	ROM_LOAD( "111-1a.037",   0x000000, 0x008000, CRC(91700f33) SHA1(fac1ce700c4cd46b643307998df781d637f193aa) )
	ROM_LOAD( "112-1b.037",   0x008000, 0x008000, CRC(869330be) SHA1(5dfaaf54ee2b3c0eaf35e8c17558313db9791616) )
	ROM_LOAD( "113-1l.037",   0x010000, 0x008000, CRC(d497d0a8) SHA1(bb715bcec7f783dd04151e2e3b221a72133bf17d) )
	ROM_LOAD( "114-1mn.037",  0x018000, 0x008000, CRC(29ef9882) SHA1(91e1465af6505b35cd97434c13d2b4d40a085946) )
	ROM_LOAD( "115-2a.037",   0x020000, 0x008000, CRC(9510b898) SHA1(e6c8c7af1898d548f0f01e4ff37c2c7b22c0b5c2) )
	ROM_LOAD( "116-2b.037",   0x028000, 0x008000, CRC(11e0ac5b) SHA1(729b7561d59d94ef33874a134b97bcd37573dfa6) )
	ROM_LOAD( "117-2l.037",   0x030000, 0x008000, CRC(29a5db41) SHA1(94f4f5dd39e724570a0f54af176ad018497697fd) )
	ROM_LOAD( "118-2mn.037",  0x038000, 0x008000, CRC(8bf3b263) SHA1(683d900ab7591ee661218be2406fb375a12e435c) )
ROM_END


ROM_START( gaunt2pg )
	ROM_REGION( 0x80000, REGION_CPU1, 0 )	/* 8*64k for 68000 code */
	ROM_LOAD16_BYTE( "407-9a.041",   0x000000, 0x008000, CRC(cde72140) SHA1(6cf4254e90a32ee36f5fbfa44b69fca82f68d2bc) )
	ROM_LOAD16_BYTE( "408-9b.041",   0x000001, 0x008000, CRC(4ab1af62) SHA1(46915a6822551004f3670678691a4ffb6d187914) )
	ROM_LOAD16_BYTE( "205-10a.037",  0x038000, 0x004000, CRC(6d99ed51) SHA1(a7bc18f32908451859ba5cdf1a5c97ecc5fe325f) )
	ROM_LOAD16_BYTE( "206-10b.037",  0x038001, 0x004000, CRC(545ead91) SHA1(7fad5a63c6443249bb6dad5b2a1fd08ca5f11e10) )
	ROM_LOAD16_BYTE( "409-7a.041",   0x040000, 0x008000, CRC(44e01459) SHA1(f49de4445550aa72be73fff3ed4c70ecd21fc2ea) )
	ROM_LOAD16_BYTE( "410-7b.041",   0x040001, 0x008000, CRC(b58d96d3) SHA1(621b3f26cc5f681fa0b15bdbc1a94e9fdd098423) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for 6502 code */
	ROM_LOAD( "120-16r.037",  0x004000, 0x004000, CRC(6ee7f3cc) SHA1(b86676340b06f07c164690862c1f6f75f30c080b) )
	ROM_LOAD( "119-16s.037",  0x008000, 0x008000, CRC(fa19861f) SHA1(7568b4ab526bd5849f7ef70dfa6d1ef1f30c0abc) )

	ROM_REGION( 0x04000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "104-6p.037",   0x000000, 0x002000, CRC(9e2a5b59) SHA1(3bbaa92e4612b0a26837ca67cfa4e36d0f2295fa) )

	ROM_REGION( 0x40000, REGION_GFX2, ROMREGION_DISPOSE | ROMREGION_INVERT )
	ROM_LOAD( "111-1a.037",   0x000000, 0x008000, CRC(91700f33) SHA1(fac1ce700c4cd46b643307998df781d637f193aa) )
	ROM_LOAD( "112-1b.037",   0x008000, 0x008000, CRC(869330be) SHA1(5dfaaf54ee2b3c0eaf35e8c17558313db9791616) )
	ROM_LOAD( "113-1l.037",   0x010000, 0x008000, CRC(d497d0a8) SHA1(bb715bcec7f783dd04151e2e3b221a72133bf17d) )
	ROM_LOAD( "114-1mn.037",  0x018000, 0x008000, CRC(29ef9882) SHA1(91e1465af6505b35cd97434c13d2b4d40a085946) )
	ROM_LOAD( "115-2a.037",   0x020000, 0x008000, CRC(9510b898) SHA1(e6c8c7af1898d548f0f01e4ff37c2c7b22c0b5c2) )
	ROM_LOAD( "116-2b.037",   0x028000, 0x008000, CRC(11e0ac5b) SHA1(729b7561d59d94ef33874a134b97bcd37573dfa6) )
	ROM_LOAD( "117-2l.037",   0x030000, 0x008000, CRC(29a5db41) SHA1(94f4f5dd39e724570a0f54af176ad018497697fd) )
	ROM_LOAD( "118-2mn.037",  0x038000, 0x008000, CRC(8bf3b263) SHA1(683d900ab7591ee661218be2406fb375a12e435c) )
ROM_END


ROM_START( gaun2pr3 )
	ROM_REGION( 0x80000, REGION_CPU1, 0 )	/* 8*64k for 68000 code */
	ROM_LOAD16_BYTE( "207-9a.041",   0x000000, 0x008000, CRC(0e1af1b4) SHA1(7091d3ff15dce33959e3c2268843c8d4f4140097) )
	ROM_LOAD16_BYTE( "208-9b.041",   0x000001, 0x008000, CRC(bf51a238) SHA1(2110e6aa4a8076b1ed29432876138590102a7408) )
	ROM_LOAD16_BYTE( "205-10a.037",  0x038000, 0x004000, CRC(6d99ed51) SHA1(a7bc18f32908451859ba5cdf1a5c97ecc5fe325f) )
	ROM_LOAD16_BYTE( "206-10b.037",  0x038001, 0x004000, CRC(545ead91) SHA1(7fad5a63c6443249bb6dad5b2a1fd08ca5f11e10) )
	ROM_LOAD16_BYTE( "309-7a.041",   0x040000, 0x008000, CRC(5acbcd2b) SHA1(b0acf6f3639d84faf11645ab54d07127259bcb65) )
	ROM_LOAD16_BYTE( "310-7b.041",   0x040001, 0x008000, CRC(1889ab77) SHA1(eb06138ec385b6936147587dd3254ce8ef68c2ba) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for 6502 code */
	ROM_LOAD( "120-16r.037",  0x004000, 0x004000, CRC(6ee7f3cc) SHA1(b86676340b06f07c164690862c1f6f75f30c080b) )
	ROM_LOAD( "119-16s.037",  0x008000, 0x008000, CRC(fa19861f) SHA1(7568b4ab526bd5849f7ef70dfa6d1ef1f30c0abc) )

	ROM_REGION( 0x04000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "104-6p.037",   0x000000, 0x002000, CRC(9e2a5b59) SHA1(3bbaa92e4612b0a26837ca67cfa4e36d0f2295fa) )

	ROM_REGION( 0x40000, REGION_GFX2, ROMREGION_DISPOSE | ROMREGION_INVERT )
	ROM_LOAD( "111-1a.037",   0x000000, 0x008000, CRC(91700f33) SHA1(fac1ce700c4cd46b643307998df781d637f193aa) )
	ROM_LOAD( "112-1b.037",   0x008000, 0x008000, CRC(869330be) SHA1(5dfaaf54ee2b3c0eaf35e8c17558313db9791616) )
	ROM_LOAD( "113-1l.037",   0x010000, 0x008000, CRC(d497d0a8) SHA1(bb715bcec7f783dd04151e2e3b221a72133bf17d) )
	ROM_LOAD( "114-1mn.037",  0x018000, 0x008000, CRC(29ef9882) SHA1(91e1465af6505b35cd97434c13d2b4d40a085946) )
	ROM_LOAD( "115-2a.037",   0x020000, 0x008000, CRC(9510b898) SHA1(e6c8c7af1898d548f0f01e4ff37c2c7b22c0b5c2) )
	ROM_LOAD( "116-2b.037",   0x028000, 0x008000, CRC(11e0ac5b) SHA1(729b7561d59d94ef33874a134b97bcd37573dfa6) )
	ROM_LOAD( "117-2l.037",   0x030000, 0x008000, CRC(29a5db41) SHA1(94f4f5dd39e724570a0f54af176ad018497697fd) )
	ROM_LOAD( "118-2mn.037",  0x038000, 0x008000, CRC(8bf3b263) SHA1(683d900ab7591ee661218be2406fb375a12e435c) )
ROM_END


ROM_START( gaun2pj2 )
	ROM_REGION( 0x80000, REGION_CPU1, 0 )	/* 8*64k for 68000 code */
	ROM_LOAD16_BYTE( "207-9a.041",   0x000000, 0x008000, CRC(0e1af1b4) SHA1(7091d3ff15dce33959e3c2268843c8d4f4140097) )
	ROM_LOAD16_BYTE( "208-9b.041",   0x000001, 0x008000, CRC(bf51a238) SHA1(2110e6aa4a8076b1ed29432876138590102a7408) )
	ROM_LOAD16_BYTE( "205-10a.037",  0x038000, 0x004000, CRC(6d99ed51) SHA1(a7bc18f32908451859ba5cdf1a5c97ecc5fe325f) )
	ROM_LOAD16_BYTE( "206-10b.037",  0x038001, 0x004000, CRC(545ead91) SHA1(7fad5a63c6443249bb6dad5b2a1fd08ca5f11e10) )
	ROM_LOAD16_BYTE( "209-7a.041",   0x040000, 0x008000, CRC(ddc9b56f) SHA1(fef9ae612c074b9297be3318acaa4a0565dad258) )
	ROM_LOAD16_BYTE( "210-7b.041",   0x040001, 0x008000, CRC(ffe78a4f) SHA1(0a50b3a9ae4c90270e00abd4808082fb9996cb0f) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for 6502 code */
	ROM_LOAD( "120-16r.037",  0x004000, 0x004000, CRC(6ee7f3cc) SHA1(b86676340b06f07c164690862c1f6f75f30c080b) )
	ROM_LOAD( "119-16s.037",  0x008000, 0x008000, CRC(fa19861f) SHA1(7568b4ab526bd5849f7ef70dfa6d1ef1f30c0abc) )

	ROM_REGION( 0x04000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "104-6p.037",   0x000000, 0x002000, CRC(9e2a5b59) SHA1(3bbaa92e4612b0a26837ca67cfa4e36d0f2295fa) )

	ROM_REGION( 0x40000, REGION_GFX2, ROMREGION_DISPOSE | ROMREGION_INVERT )
	ROM_LOAD( "111-1a.037",   0x000000, 0x008000, CRC(91700f33) SHA1(fac1ce700c4cd46b643307998df781d637f193aa) )
	ROM_LOAD( "112-1b.037",   0x008000, 0x008000, CRC(869330be) SHA1(5dfaaf54ee2b3c0eaf35e8c17558313db9791616) )
	ROM_LOAD( "113-1l.037",   0x010000, 0x008000, CRC(d497d0a8) SHA1(bb715bcec7f783dd04151e2e3b221a72133bf17d) )
	ROM_LOAD( "114-1mn.037",  0x018000, 0x008000, CRC(29ef9882) SHA1(91e1465af6505b35cd97434c13d2b4d40a085946) )
	ROM_LOAD( "115-2a.037",   0x020000, 0x008000, CRC(9510b898) SHA1(e6c8c7af1898d548f0f01e4ff37c2c7b22c0b5c2) )
	ROM_LOAD( "116-2b.037",   0x028000, 0x008000, CRC(11e0ac5b) SHA1(729b7561d59d94ef33874a134b97bcd37573dfa6) )
	ROM_LOAD( "117-2l.037",   0x030000, 0x008000, CRC(29a5db41) SHA1(94f4f5dd39e724570a0f54af176ad018497697fd) )
	ROM_LOAD( "118-2mn.037",  0x038000, 0x008000, CRC(8bf3b263) SHA1(683d900ab7591ee661218be2406fb375a12e435c) )
ROM_END


ROM_START( gaun2pg1 )
	ROM_REGION( 0x80000, REGION_CPU1, 0 )	/* 8*64k for 68000 code */
	ROM_LOAD16_BYTE( "107-9a.041",   0x000000, 0x008000, CRC(3faf74d8) SHA1(366256fb42e9d3a548c6545f6fa718beb766ba16) )
	ROM_LOAD16_BYTE( "108-9b.041",   0x000001, 0x008000, CRC(f1e6d815) SHA1(9bda05ee05c1f49078a152aa30a1fafa108f1c93) )
	ROM_LOAD16_BYTE( "205-10a.037",  0x038000, 0x004000, CRC(6d99ed51) SHA1(a7bc18f32908451859ba5cdf1a5c97ecc5fe325f) )
	ROM_LOAD16_BYTE( "206-10b.037",  0x038001, 0x004000, CRC(545ead91) SHA1(7fad5a63c6443249bb6dad5b2a1fd08ca5f11e10) )
	ROM_LOAD16_BYTE( "109-7a.041",   0x040000, 0x008000, CRC(56d0c5b8) SHA1(6534c810c2b863f3712fd35cc4f7f8d1e2330a6f) )
	ROM_LOAD16_BYTE( "110-7b.041",   0x040001, 0x008000, CRC(3b9ae397) SHA1(a605c39bdd994941756be97f71a76973b68833bc) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for 6502 code */
	ROM_LOAD( "120-16r.037",  0x004000, 0x004000, CRC(6ee7f3cc) SHA1(b86676340b06f07c164690862c1f6f75f30c080b) )
	ROM_LOAD( "119-16s.037",  0x008000, 0x008000, CRC(fa19861f) SHA1(7568b4ab526bd5849f7ef70dfa6d1ef1f30c0abc) )

	ROM_REGION( 0x04000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "104-6p.037",   0x000000, 0x002000, CRC(9e2a5b59) SHA1(3bbaa92e4612b0a26837ca67cfa4e36d0f2295fa) )

	ROM_REGION( 0x40000, REGION_GFX2, ROMREGION_DISPOSE | ROMREGION_INVERT )
	ROM_LOAD( "111-1a.037",   0x000000, 0x008000, CRC(91700f33) SHA1(fac1ce700c4cd46b643307998df781d637f193aa) )
	ROM_LOAD( "112-1b.037",   0x008000, 0x008000, CRC(869330be) SHA1(5dfaaf54ee2b3c0eaf35e8c17558313db9791616) )
	ROM_LOAD( "113-1l.037",   0x010000, 0x008000, CRC(d497d0a8) SHA1(bb715bcec7f783dd04151e2e3b221a72133bf17d) )
	ROM_LOAD( "114-1mn.037",  0x018000, 0x008000, CRC(29ef9882) SHA1(91e1465af6505b35cd97434c13d2b4d40a085946) )
	ROM_LOAD( "115-2a.037",   0x020000, 0x008000, CRC(9510b898) SHA1(e6c8c7af1898d548f0f01e4ff37c2c7b22c0b5c2) )
	ROM_LOAD( "116-2b.037",   0x028000, 0x008000, CRC(11e0ac5b) SHA1(729b7561d59d94ef33874a134b97bcd37573dfa6) )
	ROM_LOAD( "117-2l.037",   0x030000, 0x008000, CRC(29a5db41) SHA1(94f4f5dd39e724570a0f54af176ad018497697fd) )
	ROM_LOAD( "118-2mn.037",  0x038000, 0x008000, CRC(8bf3b263) SHA1(683d900ab7591ee661218be2406fb375a12e435c) )
ROM_END


ROM_START( gaunt2 )
	ROM_REGION( 0x80000, REGION_CPU1, 0 )	/* 8*64k for 68000 code */
	ROM_LOAD16_BYTE( "1307-9a.037",  0x000000, 0x008000, CRC(46fe8743) SHA1(d5fa19e028a2f43658330c67c10e0c811d332780) )
	ROM_LOAD16_BYTE( "1308-9b.037",  0x000001, 0x008000, CRC(276e15c4) SHA1(7467b2ec21b1b4fcc18ff9387ce891495f4b064c) )
	ROM_LOAD16_BYTE( "1105-10a.043", 0x038000, 0x004000, CRC(45dfda47) SHA1(a9a03150f5a0ad6ce62c5cfdffb4a9f54340590c) )
	ROM_LOAD16_BYTE( "1106-10b.043", 0x038001, 0x004000, CRC(343c029c) SHA1(d2df4e5b036500dcc537a1e0025abb2a8c730bdd) )
	ROM_LOAD16_BYTE( "1109-7a.043",  0x040000, 0x008000, CRC(58a0a9a3) SHA1(7f51184840e3c96574836b8a00bfb4a7a5f508d0) )
	ROM_LOAD16_BYTE( "1110-7b.043",  0x040001, 0x008000, CRC(658f0da8) SHA1(dfce027ea50188659907be698aeb26f9d8bfab23) )
	ROM_LOAD16_BYTE( "1121-6a.043",  0x050000, 0x008000, CRC(ae301bba) SHA1(3d93236aaffe6ef692e5073b1828633e8abf0ce4) )
	ROM_LOAD16_BYTE( "1122-6b.043",  0x050001, 0x008000, CRC(e94aaa8a) SHA1(378c582c360440b808820bcd3be78ec6e8800c34) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for 6502 code */
	ROM_LOAD( "1120-16r.043", 0x004000, 0x004000, CRC(5c731006) SHA1(045ad571db34ef870b1bf003e77eea403204f55b) )
	ROM_LOAD( "1119-16s.043", 0x008000, 0x008000, CRC(dc3591e7) SHA1(6d0d8493609974bd5a63be858b045fe4db35d8df) )

	ROM_REGION( 0x04000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "1104-6p.043",  0x000000, 0x002000, CRC(1343cf6f) SHA1(4a9542bc8ede305e7e8f860eb4b47ca2f3017275) )

	ROM_REGION( 0x60000, REGION_GFX2, ROMREGION_DISPOSE | ROMREGION_INVERT )
	ROM_LOAD( "1111-1a.043",  0x000000, 0x008000, CRC(09df6e23) SHA1(726984275c6a338c12ec0c4cc449f92f4a7a138c) )
	ROM_LOAD( "112-1b.037",   0x008000, 0x008000, CRC(869330be) SHA1(5dfaaf54ee2b3c0eaf35e8c17558313db9791616) )
	ROM_LOAD( "1123-1c.043",  0x010000, 0x004000, CRC(e4c98f01) SHA1(a24bece3196d13c38e4acdbf62783860253ba67d) )
	ROM_RELOAD(               0x014000, 0x004000 )
	ROM_LOAD( "1113-1l.043",  0x018000, 0x008000, CRC(33cb476e) SHA1(e0757ee0120de2d38be44f8dc8702972c35b87b3) )
	ROM_LOAD( "114-1mn.037",  0x020000, 0x008000, CRC(29ef9882) SHA1(91e1465af6505b35cd97434c13d2b4d40a085946) )
	ROM_LOAD( "1124-1p.043",  0x028000, 0x004000, CRC(c4857879) SHA1(3b4ce96da0d178b4bc2d05b5b51b42c7ec461113) )
	ROM_RELOAD(               0x02c000, 0x004000 )
	ROM_LOAD( "1115-2a.043",  0x030000, 0x008000, CRC(f71e2503) SHA1(244e108668eaef6b64c6ff733b08b9ee6b7a2d2b) )
	ROM_LOAD( "116-2b.037",   0x038000, 0x008000, CRC(11e0ac5b) SHA1(729b7561d59d94ef33874a134b97bcd37573dfa6) )
	ROM_LOAD( "1125-2c.043",  0x040000, 0x004000, CRC(d9c2c2d1) SHA1(185e38c75c06b6ca131a17ee3a46098279bfe17e) )
	ROM_RELOAD(               0x044000, 0x004000 )
	ROM_LOAD( "1117-2l.043",  0x048000, 0x008000, CRC(9e30b2e9) SHA1(e9b513089eaf3bec269058b437fefe7075a3fd6f) )
	ROM_LOAD( "118-2mn.037",  0x050000, 0x008000, CRC(8bf3b263) SHA1(683d900ab7591ee661218be2406fb375a12e435c) )
	ROM_LOAD( "1126-2p.043",  0x058000, 0x004000, CRC(a32c732a) SHA1(abe801dff7bb3f2712e2189c2b91f172d941fccd) )
	ROM_RELOAD(               0x05c000, 0x004000 )
ROM_END


ROM_START( gaunt2g )
	ROM_REGION( 0x80000, REGION_CPU1, 0 )	/* 8*64k for 68000 code */
	ROM_LOAD16_BYTE( "1007-9a.037",  0x000000, 0x008000, CRC(6a224cea) SHA1(1d9205a1587a39b3bc6da1813e380a8babee2994) )
	ROM_LOAD16_BYTE( "1008-9b.037",  0x000001, 0x008000, CRC(fa391dab) SHA1(7dcb67fa969b437fe2474daeb3c7c3652df2ff5d) )
	ROM_LOAD16_BYTE( "1105-10a.043", 0x038000, 0x004000, CRC(45dfda47) SHA1(a9a03150f5a0ad6ce62c5cfdffb4a9f54340590c) )
	ROM_LOAD16_BYTE( "1106-10b.043", 0x038001, 0x004000, CRC(343c029c) SHA1(d2df4e5b036500dcc537a1e0025abb2a8c730bdd) )
	ROM_LOAD16_BYTE( "2209-7a.043",  0x040000, 0x008000, CRC(577f4101) SHA1(0923613b913d5ea832ff109c90ecd17111269c0a) )
	ROM_LOAD16_BYTE( "2210-7b.043",  0x040001, 0x008000, CRC(03254cf4) SHA1(2453f600590c09255652009fced539fd3acb6db4) )
	ROM_LOAD16_BYTE( "2221-6a.043",  0x050000, 0x008000, CRC(c8adcf1a) SHA1(511077782e3ab97adbc9f3adb8cb5247cbda7d89) )
	ROM_LOAD16_BYTE( "2222-6b.043",  0x050001, 0x008000, CRC(7788ff84) SHA1(1615873fcff048ce6b8413904814caf6679cf501) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for 6502 code */
	ROM_LOAD( "1120-16r.043", 0x004000, 0x004000, CRC(5c731006) SHA1(045ad571db34ef870b1bf003e77eea403204f55b) )
	ROM_LOAD( "1119-16s.043", 0x008000, 0x008000, CRC(dc3591e7) SHA1(6d0d8493609974bd5a63be858b045fe4db35d8df) )

	ROM_REGION( 0x04000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "1104-6p.043",  0x000000, 0x002000, CRC(1343cf6f) SHA1(4a9542bc8ede305e7e8f860eb4b47ca2f3017275) )

	ROM_REGION( 0x60000, REGION_GFX2, ROMREGION_DISPOSE | ROMREGION_INVERT )
	ROM_LOAD( "1111-1a.043",  0x000000, 0x008000, CRC(09df6e23) SHA1(726984275c6a338c12ec0c4cc449f92f4a7a138c) )
	ROM_LOAD( "112-1b.037",   0x008000, 0x008000, CRC(869330be) SHA1(5dfaaf54ee2b3c0eaf35e8c17558313db9791616) )
	ROM_LOAD( "1123-1c.043",  0x010000, 0x004000, CRC(e4c98f01) SHA1(a24bece3196d13c38e4acdbf62783860253ba67d) )
	ROM_RELOAD(               0x014000, 0x004000 )
	ROM_LOAD( "1113-1l.043",  0x018000, 0x008000, CRC(33cb476e) SHA1(e0757ee0120de2d38be44f8dc8702972c35b87b3) )
	ROM_LOAD( "114-1mn.037",  0x020000, 0x008000, CRC(29ef9882) SHA1(91e1465af6505b35cd97434c13d2b4d40a085946) )
	ROM_LOAD( "1124-1p.043",  0x028000, 0x004000, CRC(c4857879) SHA1(3b4ce96da0d178b4bc2d05b5b51b42c7ec461113) )
	ROM_RELOAD(               0x02c000, 0x004000 )
	ROM_LOAD( "1115-2a.043",  0x030000, 0x008000, CRC(f71e2503) SHA1(244e108668eaef6b64c6ff733b08b9ee6b7a2d2b) )
	ROM_LOAD( "116-2b.037",   0x038000, 0x008000, CRC(11e0ac5b) SHA1(729b7561d59d94ef33874a134b97bcd37573dfa6) )
	ROM_LOAD( "1125-2c.043",  0x040000, 0x004000, CRC(d9c2c2d1) SHA1(185e38c75c06b6ca131a17ee3a46098279bfe17e) )
	ROM_RELOAD(               0x044000, 0x004000 )
	ROM_LOAD( "1117-2l.043",  0x048000, 0x008000, CRC(9e30b2e9) SHA1(e9b513089eaf3bec269058b437fefe7075a3fd6f) )
	ROM_LOAD( "118-2mn.037",  0x050000, 0x008000, CRC(8bf3b263) SHA1(683d900ab7591ee661218be2406fb375a12e435c) )
	ROM_LOAD( "1126-2p.043",  0x058000, 0x004000, CRC(a32c732a) SHA1(abe801dff7bb3f2712e2189c2b91f172d941fccd) )
	ROM_RELOAD(               0x05c000, 0x004000 )
ROM_END


ROM_START( gaunt22p )
	ROM_REGION( 0x80000, REGION_CPU1, 0 )	/* 8*64k for 68000 code */
	ROM_LOAD16_BYTE( "1307-9a.037",  0x000000, 0x008000, CRC(46fe8743) SHA1(d5fa19e028a2f43658330c67c10e0c811d332780) )
	ROM_LOAD16_BYTE( "1308-9b.037",  0x000001, 0x008000, CRC(276e15c4) SHA1(7467b2ec21b1b4fcc18ff9387ce891495f4b064c) )
	ROM_LOAD16_BYTE( "1105-10a.043", 0x038000, 0x004000, CRC(45dfda47) SHA1(a9a03150f5a0ad6ce62c5cfdffb4a9f54340590c) )
	ROM_LOAD16_BYTE( "1106-10b.043", 0x038001, 0x004000, CRC(343c029c) SHA1(d2df4e5b036500dcc537a1e0025abb2a8c730bdd) )
	ROM_LOAD16_BYTE( "2109-7a.044",  0x040000, 0x008000, CRC(1102ab96) SHA1(a8a5b30b93af668d3fc44df537b62028e31b0c31) )
	ROM_LOAD16_BYTE( "2110-7b.044",  0x040001, 0x008000, CRC(d2203a2b) SHA1(8744055067a5dcc5a8803be79ed1a18f0e3bcd2e) )
	ROM_LOAD16_BYTE( "2121-6a.044",  0x050000, 0x008000, CRC(753982d7) SHA1(eedad2672865ae868a4838dcf4d836ea9e72f546) )
	ROM_LOAD16_BYTE( "2122-6b.044",  0x050001, 0x008000, CRC(879149ea) SHA1(fa5bb34f9547052e9bcdf2c581352f51a3e8dd3d) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for 6502 code */
	ROM_LOAD( "1120-16r.043", 0x004000, 0x004000, CRC(5c731006) SHA1(045ad571db34ef870b1bf003e77eea403204f55b) )
	ROM_LOAD( "1119-16s.043", 0x008000, 0x008000, CRC(dc3591e7) SHA1(6d0d8493609974bd5a63be858b045fe4db35d8df) )

	ROM_REGION( 0x04000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "1104-6p.043",  0x000000, 0x002000, CRC(1343cf6f) SHA1(4a9542bc8ede305e7e8f860eb4b47ca2f3017275) )

	ROM_REGION( 0x60000, REGION_GFX2, ROMREGION_DISPOSE | ROMREGION_INVERT )
	ROM_LOAD( "1111-1a.043",  0x000000, 0x008000, CRC(09df6e23) SHA1(726984275c6a338c12ec0c4cc449f92f4a7a138c) )
	ROM_LOAD( "112-1b.037",   0x008000, 0x008000, CRC(869330be) SHA1(5dfaaf54ee2b3c0eaf35e8c17558313db9791616) )
	ROM_LOAD( "1123-1c.043",  0x010000, 0x004000, CRC(e4c98f01) SHA1(a24bece3196d13c38e4acdbf62783860253ba67d) )
	ROM_RELOAD(               0x014000, 0x004000 )
	ROM_LOAD( "1113-1l.043",  0x018000, 0x008000, CRC(33cb476e) SHA1(e0757ee0120de2d38be44f8dc8702972c35b87b3) )
	ROM_LOAD( "114-1mn.037",  0x020000, 0x008000, CRC(29ef9882) SHA1(91e1465af6505b35cd97434c13d2b4d40a085946) )
	ROM_LOAD( "1124-1p.043",  0x028000, 0x004000, CRC(c4857879) SHA1(3b4ce96da0d178b4bc2d05b5b51b42c7ec461113) )
	ROM_RELOAD(               0x02c000, 0x004000 )
	ROM_LOAD( "1115-2a.043",  0x030000, 0x008000, CRC(f71e2503) SHA1(244e108668eaef6b64c6ff733b08b9ee6b7a2d2b) )
	ROM_LOAD( "116-2b.037",   0x038000, 0x008000, CRC(11e0ac5b) SHA1(729b7561d59d94ef33874a134b97bcd37573dfa6) )
	ROM_LOAD( "1125-2c.043",  0x040000, 0x004000, CRC(d9c2c2d1) SHA1(185e38c75c06b6ca131a17ee3a46098279bfe17e) )
	ROM_RELOAD(               0x044000, 0x004000 )
	ROM_LOAD( "1117-2l.043",  0x048000, 0x008000, CRC(9e30b2e9) SHA1(e9b513089eaf3bec269058b437fefe7075a3fd6f) )
	ROM_LOAD( "118-2mn.037",  0x050000, 0x008000, CRC(8bf3b263) SHA1(683d900ab7591ee661218be2406fb375a12e435c) )
	ROM_LOAD( "1126-2p.043",  0x058000, 0x004000, CRC(a32c732a) SHA1(abe801dff7bb3f2712e2189c2b91f172d941fccd) )
	ROM_RELOAD(               0x05c000, 0x004000 )
ROM_END


ROM_START( gaun22p1 )
	ROM_REGION( 0x80000, REGION_CPU1, 0 )	/* 8*64k for 68000 code */
	ROM_LOAD16_BYTE( "1307-9a.037",  0x000000, 0x008000, CRC(46fe8743) SHA1(d5fa19e028a2f43658330c67c10e0c811d332780) )
	ROM_LOAD16_BYTE( "1308-9b.037",  0x000001, 0x008000, CRC(276e15c4) SHA1(7467b2ec21b1b4fcc18ff9387ce891495f4b064c) )
	ROM_LOAD16_BYTE( "1105-10a.043", 0x038000, 0x004000, CRC(45dfda47) SHA1(a9a03150f5a0ad6ce62c5cfdffb4a9f54340590c) )
	ROM_LOAD16_BYTE( "1106-10b.043", 0x038001, 0x004000, CRC(343c029c) SHA1(d2df4e5b036500dcc537a1e0025abb2a8c730bdd) )
	ROM_LOAD16_BYTE( "1109-7a.044",  0x040000, 0x008000, CRC(31f805eb) SHA1(21fd30bd5379b39cbf4faae02509a07c9eb8b139) )
	ROM_LOAD16_BYTE( "1110-7b.044",  0x040001, 0x008000, CRC(5285c0e2) SHA1(034a8f537160bebfdc1546679d2d01572ed34176) )
	ROM_LOAD16_BYTE( "1121-6a.044",  0x050000, 0x008000, CRC(d1f3b32a) SHA1(bf31abef2ef1c05044e0167b27ce27139427d9a5) )
	ROM_LOAD16_BYTE( "1122-6b.044",  0x050001, 0x008000, CRC(3485785f) SHA1(a2dc463ca87d7a600a8f5f99967a648e00d6acc8) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for 6502 code */
	ROM_LOAD( "1120-16r.043", 0x004000, 0x004000, CRC(5c731006) SHA1(045ad571db34ef870b1bf003e77eea403204f55b) )
	ROM_LOAD( "1119-16s.043", 0x008000, 0x008000, CRC(dc3591e7) SHA1(6d0d8493609974bd5a63be858b045fe4db35d8df) )

	ROM_REGION( 0x04000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "1104-6p.043",  0x000000, 0x002000, CRC(1343cf6f) SHA1(4a9542bc8ede305e7e8f860eb4b47ca2f3017275) )

	ROM_REGION( 0x60000, REGION_GFX2, ROMREGION_DISPOSE | ROMREGION_INVERT )
	ROM_LOAD( "1111-1a.043",  0x000000, 0x008000, CRC(09df6e23) SHA1(726984275c6a338c12ec0c4cc449f92f4a7a138c) )
	ROM_LOAD( "112-1b.037",   0x008000, 0x008000, CRC(869330be) SHA1(5dfaaf54ee2b3c0eaf35e8c17558313db9791616) )
	ROM_LOAD( "1123-1c.043",  0x010000, 0x004000, CRC(e4c98f01) SHA1(a24bece3196d13c38e4acdbf62783860253ba67d) )
	ROM_RELOAD(               0x014000, 0x004000 )
	ROM_LOAD( "1113-1l.043",  0x018000, 0x008000, CRC(33cb476e) SHA1(e0757ee0120de2d38be44f8dc8702972c35b87b3) )
	ROM_LOAD( "114-1mn.037",  0x020000, 0x008000, CRC(29ef9882) SHA1(91e1465af6505b35cd97434c13d2b4d40a085946) )
	ROM_LOAD( "1124-1p.043",  0x028000, 0x004000, CRC(c4857879) SHA1(3b4ce96da0d178b4bc2d05b5b51b42c7ec461113) )
	ROM_RELOAD(               0x02c000, 0x004000 )
	ROM_LOAD( "1115-2a.043",  0x030000, 0x008000, CRC(f71e2503) SHA1(244e108668eaef6b64c6ff733b08b9ee6b7a2d2b) )
	ROM_LOAD( "116-2b.037",   0x038000, 0x008000, CRC(11e0ac5b) SHA1(729b7561d59d94ef33874a134b97bcd37573dfa6) )
	ROM_LOAD( "1125-2c.043",  0x040000, 0x004000, CRC(d9c2c2d1) SHA1(185e38c75c06b6ca131a17ee3a46098279bfe17e) )
	ROM_RELOAD(               0x044000, 0x004000 )
	ROM_LOAD( "1117-2l.043",  0x048000, 0x008000, CRC(9e30b2e9) SHA1(e9b513089eaf3bec269058b437fefe7075a3fd6f) )
	ROM_LOAD( "118-2mn.037",  0x050000, 0x008000, CRC(8bf3b263) SHA1(683d900ab7591ee661218be2406fb375a12e435c) )
	ROM_LOAD( "1126-2p.043",  0x058000, 0x004000, CRC(a32c732a) SHA1(abe801dff7bb3f2712e2189c2b91f172d941fccd) )
	ROM_RELOAD(               0x05c000, 0x004000 )
ROM_END


ROM_START( gaun22pg )
	ROM_REGION( 0x80000, REGION_CPU1, 0 )	/* 8*64k for 68000 code */
	ROM_LOAD16_BYTE( "1007-9a.037",  0x000000, 0x008000, CRC(6a224cea) SHA1(1d9205a1587a39b3bc6da1813e380a8babee2994) )
	ROM_LOAD16_BYTE( "1008-9b.037",  0x000001, 0x008000, CRC(fa391dab) SHA1(7dcb67fa969b437fe2474daeb3c7c3652df2ff5d) )
	ROM_LOAD16_BYTE( "1105-10a.043", 0x038000, 0x004000, CRC(45dfda47) SHA1(a9a03150f5a0ad6ce62c5cfdffb4a9f54340590c) )
	ROM_LOAD16_BYTE( "1106-10b.043", 0x038001, 0x004000, CRC(343c029c) SHA1(d2df4e5b036500dcc537a1e0025abb2a8c730bdd) )
	ROM_LOAD16_BYTE( "2209-7a.044",  0x040000, 0x008000, CRC(9da52ecd) SHA1(b6ce6ee66fb4febafc8c1075241546b630d2d9f2) )
	ROM_LOAD16_BYTE( "2210-7b.044",  0x040001, 0x008000, CRC(63d0f6a7) SHA1(fd967418d334d98f5d20747931f22fc83fc3e43b) )
	ROM_LOAD16_BYTE( "2221-6a.044",  0x050000, 0x008000, CRC(8895b31b) SHA1(16d3d6675b68559a0c3b2d2101a2fb6bea5600c6) )
	ROM_LOAD16_BYTE( "2222-6b.044",  0x050001, 0x008000, CRC(a4456cc7) SHA1(cb50cee59e7a0eecad0d33d8b8eb4adf0d413e77) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for 6502 code */
	ROM_LOAD( "1120-16r.043", 0x004000, 0x004000, CRC(5c731006) SHA1(045ad571db34ef870b1bf003e77eea403204f55b) )
	ROM_LOAD( "1119-16s.043", 0x008000, 0x008000, CRC(dc3591e7) SHA1(6d0d8493609974bd5a63be858b045fe4db35d8df) )

	ROM_REGION( 0x04000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "1104-6p.043",  0x000000, 0x002000, CRC(1343cf6f) SHA1(4a9542bc8ede305e7e8f860eb4b47ca2f3017275) )

	ROM_REGION( 0x60000, REGION_GFX2, ROMREGION_DISPOSE | ROMREGION_INVERT )
	ROM_LOAD( "1111-1a.043",  0x000000, 0x008000, CRC(09df6e23) SHA1(726984275c6a338c12ec0c4cc449f92f4a7a138c) )
	ROM_LOAD( "112-1b.037",   0x008000, 0x008000, CRC(869330be) SHA1(5dfaaf54ee2b3c0eaf35e8c17558313db9791616) )
	ROM_LOAD( "1123-1c.043",  0x010000, 0x004000, CRC(e4c98f01) SHA1(a24bece3196d13c38e4acdbf62783860253ba67d) )
	ROM_RELOAD(               0x014000, 0x004000 )
	ROM_LOAD( "1113-1l.043",  0x018000, 0x008000, CRC(33cb476e) SHA1(e0757ee0120de2d38be44f8dc8702972c35b87b3) )
	ROM_LOAD( "114-1mn.037",  0x020000, 0x008000, CRC(29ef9882) SHA1(91e1465af6505b35cd97434c13d2b4d40a085946) )
	ROM_LOAD( "1124-1p.043",  0x028000, 0x004000, CRC(c4857879) SHA1(3b4ce96da0d178b4bc2d05b5b51b42c7ec461113) )
	ROM_RELOAD(               0x02c000, 0x004000 )
	ROM_LOAD( "1115-2a.043",  0x030000, 0x008000, CRC(f71e2503) SHA1(244e108668eaef6b64c6ff733b08b9ee6b7a2d2b) )
	ROM_LOAD( "116-2b.037",   0x038000, 0x008000, CRC(11e0ac5b) SHA1(729b7561d59d94ef33874a134b97bcd37573dfa6) )
	ROM_LOAD( "1125-2c.043",  0x040000, 0x004000, CRC(d9c2c2d1) SHA1(185e38c75c06b6ca131a17ee3a46098279bfe17e) )
	ROM_RELOAD(               0x044000, 0x004000 )
	ROM_LOAD( "1117-2l.043",  0x048000, 0x008000, CRC(9e30b2e9) SHA1(e9b513089eaf3bec269058b437fefe7075a3fd6f) )
	ROM_LOAD( "118-2mn.037",  0x050000, 0x008000, CRC(8bf3b263) SHA1(683d900ab7591ee661218be2406fb375a12e435c) )
	ROM_LOAD( "1126-2p.043",  0x058000, 0x004000, CRC(a32c732a) SHA1(abe801dff7bb3f2712e2189c2b91f172d941fccd) )
	ROM_RELOAD(               0x05c000, 0x004000 )
ROM_END


ROM_START( vindctr2 )
	ROM_REGION( 0x80000, REGION_CPU1, 0 )	/* 8*64k for 68000 code */
	ROM_LOAD16_BYTE( "1186-9a.059",  0x000000, 0x008000, CRC(af138263) SHA1(acb1b7f497b83c9950d51776e620adee347b48a7) )
	ROM_LOAD16_BYTE( "1187-9b.059",  0x000001, 0x008000, CRC(44baff64) SHA1(3cb3af1e93208ac139e90482d329e2368fde66d5) )
	ROM_LOAD16_BYTE( "1196-10a.059", 0x038000, 0x004000, CRC(c92bf6dd) SHA1(bdd179d6fae9565823917baefae17ace71be8191) )
	ROM_LOAD16_BYTE( "1197-10b.059", 0x038001, 0x004000, CRC(d7ace347) SHA1(9842cec069b11bd77908801be4c454571a8f04c2) )
	ROM_LOAD16_BYTE( "3188-7a.059",  0x040000, 0x008000, CRC(10f558d2) SHA1(b9ea79a7f3cbd0122d861180631a601ff77fae00) )
	ROM_LOAD16_BYTE( "3189-7b.059",  0x040001, 0x008000, CRC(302e24b6) SHA1(b138138ae397a0e911b0502d6622fff1f1419716) )
	ROM_LOAD16_BYTE( "2190-6a.059",  0x050000, 0x008000, CRC(e7dc2b74) SHA1(55da5d0293d3ff41bdeaaa9b52d153bfb88bfcad) )
	ROM_LOAD16_BYTE( "2191-6b.059",  0x050001, 0x008000, CRC(ed8ed86e) SHA1(8fedb1c25d3f4069df68118266faf0a74561a6d7) )
	ROM_LOAD16_BYTE( "2192-5a.059",  0x060000, 0x008000, CRC(eec2c93d) SHA1(d35e871ccbbccb35e35813b2cf9bf8821c000440) )
	ROM_LOAD16_BYTE( "2193-5b.059",  0x060001, 0x008000, CRC(3fbee9aa) SHA1(5802291e4a71cece4175ef1d2cecdaabfc096c3d) )
	ROM_LOAD16_BYTE( "1194-3a.059",  0x070000, 0x008000, CRC(e6bcf458) SHA1(0492ebca7baa5ee456b739628200c094cdf4879e) )
	ROM_LOAD16_BYTE( "1195-3b.059",  0x070001, 0x008000, CRC(b9bf245d) SHA1(ba190518fd7f630976d97b00af7e28a113a33ce1) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for 6502 code */
	ROM_LOAD( "1160-16s.059", 0x004000, 0x004000, CRC(eef0a003) SHA1(4b1c0810e8c60e364051ed867fed0dc3a0b3a872) )
	ROM_LOAD( "1161-16r.059", 0x008000, 0x008000, CRC(68c74337) SHA1(13a9333e0b58ce771774632ecdfa8ca9c9664e57) )

	ROM_REGION( 0x04000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "1198-6p.059",  0x000000, 0x004000, CRC(f99b631a) SHA1(7a2430b6810c77b0f717d6e9d71823eadbcf6013) )

	ROM_REGION( 0xc0000, REGION_GFX2, ROMREGION_DISPOSE | ROMREGION_INVERT )
	ROM_LOAD( "1162-1a.059",  0x000000, 0x008000, CRC(dd3833ad) SHA1(e78a44b5f2033b618b5879a8a39bfdf428b5e4c7) )
	ROM_LOAD( "1166-1b.059",  0x008000, 0x008000, CRC(e2db50a0) SHA1(953e621f7312340dcbda9e4a727ebeba69ba7d4e) )
	ROM_LOAD( "1170-1c.059",  0x010000, 0x008000, CRC(f050ab43) SHA1(72fbba20b6c8a1838842084c07157cdc2fd923c1) )
	ROM_LOAD( "1174-1d.059",  0x018000, 0x008000, CRC(b6704bd1) SHA1(0876e51e54a0f876f637f934d0ed2808d67a3515) )
	ROM_LOAD( "1178-1ef.059", 0x020000, 0x008000, CRC(d3006f05) SHA1(00e08b9b11eca017fd6ee0dea6f1818fcfddd830) )
	ROM_LOAD( "1182-1j.059",  0x028000, 0x008000, CRC(9046e985) SHA1(0cc0cd67faa467dcdf6b90c106a3662ff9e5fe41) )

	ROM_LOAD( "1163-1l.059",  0x030000, 0x008000, CRC(d505b04a) SHA1(cabf61f74146fbe84c7db368f014e17237126056) )
	ROM_LOAD( "1167-1mn.059", 0x038000, 0x008000, CRC(1869c76d) SHA1(c2ed2b94726a0a97925d0c05ad65fe8c05bac01b) )
	ROM_LOAD( "1171-1p.059",  0x040000, 0x008000, CRC(1b229c2b) SHA1(b8bf5e17d8b73bdf04bbb9ca553ce8e69c8f71db) )
	ROM_LOAD( "1175-1r.059",  0x048000, 0x008000, CRC(73c41aca) SHA1(c401f5d1664c9a86231feda0ba110f586632a1a2) )
	ROM_LOAD( "1179-1st.059", 0x050000, 0x008000, CRC(9b7cb0ef) SHA1(7febc479ddf52a5b72eba2abc9e12d3e48e804ff) )
	ROM_LOAD( "1183-1u.059",  0x058000, 0x008000, CRC(393bba42) SHA1(1c7eb448d7a4862d16bef7aa1419e8db99fb6815) )

	ROM_LOAD( "1164-2a.059",  0x060000, 0x008000, CRC(50e76162) SHA1(7aaf55c4d0ba44609c29d222babe2fb4990d0004) )
	ROM_LOAD( "1168-2b.059",  0x068000, 0x008000, CRC(35c78469) SHA1(1b3ab6e826ec2a8c8bef1d35a8ed2c46651336a6) )
	ROM_LOAD( "1172-2c.059",  0x070000, 0x008000, CRC(314ac268) SHA1(2a3b2be3b548d60489265bf78a4ab135c2bff692) )
	ROM_LOAD( "1176-2d.059",  0x078000, 0x008000, CRC(061d79db) SHA1(adf94aa01547df578039567126ca9ea53be33c37) )
	ROM_LOAD( "1180-2ef.059", 0x080000, 0x008000, CRC(89c1fe16) SHA1(e58fbe710f11529151814892e380ba0fa3296995) )
	ROM_LOAD( "1184-2j.059",  0x088000, 0x008000, CRC(541209d3) SHA1(d862f1759c1e56d61e60e0760f7743b10f65e765) )

	ROM_LOAD( "1165-2l.059",  0x090000, 0x008000, CRC(9484ba65) SHA1(ad5e3589c4bcc7be814e2dc274de0fe9d321e37c) )
	ROM_LOAD( "1169-2mn.059", 0x098000, 0x008000, CRC(132d3337) SHA1(4e50f35773ab19a0319a6fbe81e87ef69d7d0ee8) )
	ROM_LOAD( "1173-2p.059",  0x0a0000, 0x008000, CRC(98de2426) SHA1(2f3df9abef8a5ae3c09346d70ce96e65b728ffaf) )
	ROM_LOAD( "1177-2r.059",  0x0a8000, 0x008000, CRC(9d0824f8) SHA1(db921fea0ffd6c07af3affe7e3cf9282d48e6eee) )
	ROM_LOAD( "1181-2st.059", 0x0b0000, 0x008000, CRC(9e62b27c) SHA1(2df265abe412613beb6bee0b6179232b4c45d5fc) )
	ROM_LOAD( "1185-2u.059",  0x0b8000, 0x008000, CRC(9d62f6b7) SHA1(0d0f94dd81958c41674096d326ad1662284209e6) )
ROM_END


ROM_START( vindc2r2 )
	ROM_REGION( 0x80000, REGION_CPU1, 0 )	/* 8*64k for 68000 code */
	ROM_LOAD16_BYTE( "1186-9a.059",  0x000000, 0x008000, CRC(af138263) SHA1(acb1b7f497b83c9950d51776e620adee347b48a7) )
	ROM_LOAD16_BYTE( "1187-9b.059",  0x000001, 0x008000, CRC(44baff64) SHA1(3cb3af1e93208ac139e90482d329e2368fde66d5) )
	ROM_LOAD16_BYTE( "1196-10a.059", 0x038000, 0x004000, CRC(c92bf6dd) SHA1(bdd179d6fae9565823917baefae17ace71be8191) )
	ROM_LOAD16_BYTE( "1197-10b.059", 0x038001, 0x004000, CRC(d7ace347) SHA1(9842cec069b11bd77908801be4c454571a8f04c2) )
	ROM_LOAD16_BYTE( "2188-7a.059",  0x040000, 0x008000, CRC(d4e0ef1f) SHA1(833b81565cac694739050b652e61c64f45866973) )
	ROM_LOAD16_BYTE( "2189-7b.059",  0x040001, 0x008000, CRC(dcbbe2aa) SHA1(c8ddaaac9b440d706820fcbdb96059cfeb7a3b5c) )
	ROM_LOAD16_BYTE( "2190-6a.059",  0x050000, 0x008000, CRC(e7dc2b74) SHA1(55da5d0293d3ff41bdeaaa9b52d153bfb88bfcad) )
	ROM_LOAD16_BYTE( "2191-6b.059",  0x050001, 0x008000, CRC(ed8ed86e) SHA1(8fedb1c25d3f4069df68118266faf0a74561a6d7) )
	ROM_LOAD16_BYTE( "2192-5a.059",  0x060000, 0x008000, CRC(eec2c93d) SHA1(d35e871ccbbccb35e35813b2cf9bf8821c000440) )
	ROM_LOAD16_BYTE( "2193-5b.059",  0x060001, 0x008000, CRC(3fbee9aa) SHA1(5802291e4a71cece4175ef1d2cecdaabfc096c3d) )
	ROM_LOAD16_BYTE( "1194-3a.059",  0x070000, 0x008000, CRC(e6bcf458) SHA1(0492ebca7baa5ee456b739628200c094cdf4879e) )
	ROM_LOAD16_BYTE( "1195-3b.059",  0x070001, 0x008000, CRC(b9bf245d) SHA1(ba190518fd7f630976d97b00af7e28a113a33ce1) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for 6502 code */
	ROM_LOAD( "1160-16s.059", 0x004000, 0x004000, CRC(eef0a003) SHA1(4b1c0810e8c60e364051ed867fed0dc3a0b3a872) )
	ROM_LOAD( "1161-16r.059", 0x008000, 0x008000, CRC(68c74337) SHA1(13a9333e0b58ce771774632ecdfa8ca9c9664e57) )

	ROM_REGION( 0x04000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "1198-6p.059",  0x000000, 0x004000, CRC(f99b631a) SHA1(7a2430b6810c77b0f717d6e9d71823eadbcf6013) )

	ROM_REGION( 0xc0000, REGION_GFX2, ROMREGION_DISPOSE | ROMREGION_INVERT )
	ROM_LOAD( "1162-1a.059",  0x000000, 0x008000, CRC(dd3833ad) SHA1(e78a44b5f2033b618b5879a8a39bfdf428b5e4c7) )
	ROM_LOAD( "1166-1b.059",  0x008000, 0x008000, CRC(e2db50a0) SHA1(953e621f7312340dcbda9e4a727ebeba69ba7d4e) )
	ROM_LOAD( "1170-1c.059",  0x010000, 0x008000, CRC(f050ab43) SHA1(72fbba20b6c8a1838842084c07157cdc2fd923c1) )
	ROM_LOAD( "1174-1d.059",  0x018000, 0x008000, CRC(b6704bd1) SHA1(0876e51e54a0f876f637f934d0ed2808d67a3515) )
	ROM_LOAD( "1178-1ef.059", 0x020000, 0x008000, CRC(d3006f05) SHA1(00e08b9b11eca017fd6ee0dea6f1818fcfddd830) )
	ROM_LOAD( "1182-1j.059",  0x028000, 0x008000, CRC(9046e985) SHA1(0cc0cd67faa467dcdf6b90c106a3662ff9e5fe41) )

	ROM_LOAD( "1163-1l.059",  0x030000, 0x008000, CRC(d505b04a) SHA1(cabf61f74146fbe84c7db368f014e17237126056) )
	ROM_LOAD( "1167-1mn.059", 0x038000, 0x008000, CRC(1869c76d) SHA1(c2ed2b94726a0a97925d0c05ad65fe8c05bac01b) )
	ROM_LOAD( "1171-1p.059",  0x040000, 0x008000, CRC(1b229c2b) SHA1(b8bf5e17d8b73bdf04bbb9ca553ce8e69c8f71db) )
	ROM_LOAD( "1175-1r.059",  0x048000, 0x008000, CRC(73c41aca) SHA1(c401f5d1664c9a86231feda0ba110f586632a1a2) )
	ROM_LOAD( "1179-1st.059", 0x050000, 0x008000, CRC(9b7cb0ef) SHA1(7febc479ddf52a5b72eba2abc9e12d3e48e804ff) )
	ROM_LOAD( "1183-1u.059",  0x058000, 0x008000, CRC(393bba42) SHA1(1c7eb448d7a4862d16bef7aa1419e8db99fb6815) )

	ROM_LOAD( "1164-2a.059",  0x060000, 0x008000, CRC(50e76162) SHA1(7aaf55c4d0ba44609c29d222babe2fb4990d0004) )
	ROM_LOAD( "1168-2b.059",  0x068000, 0x008000, CRC(35c78469) SHA1(1b3ab6e826ec2a8c8bef1d35a8ed2c46651336a6) )
	ROM_LOAD( "1172-2c.059",  0x070000, 0x008000, CRC(314ac268) SHA1(2a3b2be3b548d60489265bf78a4ab135c2bff692) )
	ROM_LOAD( "1176-2d.059",  0x078000, 0x008000, CRC(061d79db) SHA1(adf94aa01547df578039567126ca9ea53be33c37) )
	ROM_LOAD( "1180-2ef.059", 0x080000, 0x008000, CRC(89c1fe16) SHA1(e58fbe710f11529151814892e380ba0fa3296995) )
	ROM_LOAD( "1184-2j.059",  0x088000, 0x008000, CRC(541209d3) SHA1(d862f1759c1e56d61e60e0760f7743b10f65e765) )

	ROM_LOAD( "1165-2l.059",  0x090000, 0x008000, CRC(9484ba65) SHA1(ad5e3589c4bcc7be814e2dc274de0fe9d321e37c) )
	ROM_LOAD( "1169-2mn.059", 0x098000, 0x008000, CRC(132d3337) SHA1(4e50f35773ab19a0319a6fbe81e87ef69d7d0ee8) )
	ROM_LOAD( "1173-2p.059",  0x0a0000, 0x008000, CRC(98de2426) SHA1(2f3df9abef8a5ae3c09346d70ce96e65b728ffaf) )
	ROM_LOAD( "1177-2r.059",  0x0a8000, 0x008000, CRC(9d0824f8) SHA1(db921fea0ffd6c07af3affe7e3cf9282d48e6eee) )
	ROM_LOAD( "1181-2st.059", 0x0b0000, 0x008000, CRC(9e62b27c) SHA1(2df265abe412613beb6bee0b6179232b4c45d5fc) )
	ROM_LOAD( "1185-2u.059",  0x0b8000, 0x008000, CRC(9d62f6b7) SHA1(0d0f94dd81958c41674096d326ad1662284209e6) )
ROM_END


ROM_START( vindc2r1 )
	ROM_REGION( 0x80000, REGION_CPU1, 0 )	/* 8*64k for 68000 code */
	ROM_LOAD16_BYTE( "1186-9a.059",  0x000000, 0x008000, CRC(af138263) SHA1(acb1b7f497b83c9950d51776e620adee347b48a7) )
	ROM_LOAD16_BYTE( "1187-9b.059",  0x000001, 0x008000, CRC(44baff64) SHA1(3cb3af1e93208ac139e90482d329e2368fde66d5) )
	ROM_LOAD16_BYTE( "1196-10a.059", 0x038000, 0x004000, CRC(c92bf6dd) SHA1(bdd179d6fae9565823917baefae17ace71be8191) )
	ROM_LOAD16_BYTE( "1197-10b.059", 0x038001, 0x004000, CRC(d7ace347) SHA1(9842cec069b11bd77908801be4c454571a8f04c2) )
	ROM_LOAD16_BYTE( "1188-7a.059",  0x040000, 0x008000, CRC(52294cad) SHA1(38bb965cee41e2baf33082cb3bcf6adb78607b66) )
	ROM_LOAD16_BYTE( "1189-7b.059",  0x040001, 0x008000, CRC(577a705f) SHA1(9d0f7d4282bb96c192927a8ae02f842742dc4a64) )
	ROM_LOAD16_BYTE( "1190-6a.059",  0x050000, 0x008000, CRC(7be01bb1) SHA1(7ba55b5e2ce778caa4bd0598aeb611235315cfbc) )
	ROM_LOAD16_BYTE( "1191-6b.059",  0x050001, 0x008000, CRC(91922a02) SHA1(9b64221f53251af84d80ce12d7d36dcd958c07e7) )
	ROM_LOAD16_BYTE( "1192-5a.059",  0x060000, 0x008000, CRC(e4f59d72) SHA1(aa0249d4decc32eadc37878c716c41dc280a087c) )
	ROM_LOAD16_BYTE( "1193-5b.059",  0x060001, 0x008000, CRC(e901c618) SHA1(1d24dd825bde93f72c996b16a7e7bbbbdbbfdeee) )
	ROM_LOAD16_BYTE( "1194-3a.059",  0x070000, 0x008000, CRC(e6bcf458) SHA1(0492ebca7baa5ee456b739628200c094cdf4879e) )
	ROM_LOAD16_BYTE( "1195-3b.059",  0x070001, 0x008000, CRC(b9bf245d) SHA1(ba190518fd7f630976d97b00af7e28a113a33ce1) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for 6502 code */
	ROM_LOAD( "1160-16s.059", 0x004000, 0x004000, CRC(eef0a003) SHA1(4b1c0810e8c60e364051ed867fed0dc3a0b3a872) )
	ROM_LOAD( "1161-16r.059", 0x008000, 0x008000, CRC(68c74337) SHA1(13a9333e0b58ce771774632ecdfa8ca9c9664e57) )

	ROM_REGION( 0x04000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "1198-6p.059",  0x000000, 0x004000, CRC(f99b631a) SHA1(7a2430b6810c77b0f717d6e9d71823eadbcf6013) )

	ROM_REGION( 0xc0000, REGION_GFX2, ROMREGION_DISPOSE | ROMREGION_INVERT )
	ROM_LOAD( "1162-1a.059",  0x000000, 0x008000, CRC(dd3833ad) SHA1(e78a44b5f2033b618b5879a8a39bfdf428b5e4c7) )
	ROM_LOAD( "1166-1b.059",  0x008000, 0x008000, CRC(e2db50a0) SHA1(953e621f7312340dcbda9e4a727ebeba69ba7d4e) )
	ROM_LOAD( "1170-1c.059",  0x010000, 0x008000, CRC(f050ab43) SHA1(72fbba20b6c8a1838842084c07157cdc2fd923c1) )
	ROM_LOAD( "1174-1d.059",  0x018000, 0x008000, CRC(b6704bd1) SHA1(0876e51e54a0f876f637f934d0ed2808d67a3515) )
	ROM_LOAD( "1178-1ef.059", 0x020000, 0x008000, CRC(d3006f05) SHA1(00e08b9b11eca017fd6ee0dea6f1818fcfddd830) )
	ROM_LOAD( "1182-1j.059",  0x028000, 0x008000, CRC(9046e985) SHA1(0cc0cd67faa467dcdf6b90c106a3662ff9e5fe41) )

	ROM_LOAD( "1163-1l.059",  0x030000, 0x008000, CRC(d505b04a) SHA1(cabf61f74146fbe84c7db368f014e17237126056) )
	ROM_LOAD( "1167-1mn.059", 0x038000, 0x008000, CRC(1869c76d) SHA1(c2ed2b94726a0a97925d0c05ad65fe8c05bac01b) )
	ROM_LOAD( "1171-1p.059",  0x040000, 0x008000, CRC(1b229c2b) SHA1(b8bf5e17d8b73bdf04bbb9ca553ce8e69c8f71db) )
	ROM_LOAD( "1175-1r.059",  0x048000, 0x008000, CRC(73c41aca) SHA1(c401f5d1664c9a86231feda0ba110f586632a1a2) )
	ROM_LOAD( "1179-1st.059", 0x050000, 0x008000, CRC(9b7cb0ef) SHA1(7febc479ddf52a5b72eba2abc9e12d3e48e804ff) )
	ROM_LOAD( "1183-1u.059",  0x058000, 0x008000, CRC(393bba42) SHA1(1c7eb448d7a4862d16bef7aa1419e8db99fb6815) )

	ROM_LOAD( "1164-2a.059",  0x060000, 0x008000, CRC(50e76162) SHA1(7aaf55c4d0ba44609c29d222babe2fb4990d0004) )
	ROM_LOAD( "1168-2b.059",  0x068000, 0x008000, CRC(35c78469) SHA1(1b3ab6e826ec2a8c8bef1d35a8ed2c46651336a6) )
	ROM_LOAD( "1172-2c.059",  0x070000, 0x008000, CRC(314ac268) SHA1(2a3b2be3b548d60489265bf78a4ab135c2bff692) )
	ROM_LOAD( "1176-2d.059",  0x078000, 0x008000, CRC(061d79db) SHA1(adf94aa01547df578039567126ca9ea53be33c37) )
	ROM_LOAD( "1180-2ef.059", 0x080000, 0x008000, CRC(89c1fe16) SHA1(e58fbe710f11529151814892e380ba0fa3296995) )
	ROM_LOAD( "1184-2j.059",  0x088000, 0x008000, CRC(541209d3) SHA1(d862f1759c1e56d61e60e0760f7743b10f65e765) )

	ROM_LOAD( "1165-2l.059",  0x090000, 0x008000, CRC(9484ba65) SHA1(ad5e3589c4bcc7be814e2dc274de0fe9d321e37c) )
	ROM_LOAD( "1169-2mn.059", 0x098000, 0x008000, CRC(132d3337) SHA1(4e50f35773ab19a0319a6fbe81e87ef69d7d0ee8) )
	ROM_LOAD( "1173-2p.059",  0x0a0000, 0x008000, CRC(98de2426) SHA1(2f3df9abef8a5ae3c09346d70ce96e65b728ffaf) )
	ROM_LOAD( "1177-2r.059",  0x0a8000, 0x008000, CRC(9d0824f8) SHA1(db921fea0ffd6c07af3affe7e3cf9282d48e6eee) )
	ROM_LOAD( "1181-2st.059", 0x0b0000, 0x008000, CRC(9e62b27c) SHA1(2df265abe412613beb6bee0b6179232b4c45d5fc) )
	ROM_LOAD( "1185-2u.059",  0x0b8000, 0x008000, CRC(9d62f6b7) SHA1(0d0f94dd81958c41674096d326ad1662284209e6) )
ROM_END



/*************************************
 *
 *	Driver initialization
 *
 *************************************/

static void common_init(int slapstic, int vindctr2)
{
	atarigen_eeprom_default = NULL;
	atarigen_slapstic_init(0, 0x038000, slapstic);

	/* swap the top and bottom halves of the main CPU ROM images */
	atarigen_swap_mem(memory_region(REGION_CPU1) + 0x000000, memory_region(REGION_CPU1) + 0x008000, 0x8000);
	atarigen_swap_mem(memory_region(REGION_CPU1) + 0x040000, memory_region(REGION_CPU1) + 0x048000, 0x8000);
	atarigen_swap_mem(memory_region(REGION_CPU1) + 0x050000, memory_region(REGION_CPU1) + 0x058000, 0x8000);
	atarigen_swap_mem(memory_region(REGION_CPU1) + 0x060000, memory_region(REGION_CPU1) + 0x068000, 0x8000);
	atarigen_swap_mem(memory_region(REGION_CPU1) + 0x070000, memory_region(REGION_CPU1) + 0x078000, 0x8000);

	/* indicate whether or not we are vindicators 2 */
	vindctr2_screen_refresh = vindctr2;
}


static DRIVER_INIT( gauntlet )
{
	common_init(104, 0);
}


static DRIVER_INIT( gaunt2p )
{
	common_init(107, 0);
}


static DRIVER_INIT( gauntlet2 )
{
	common_init(106, 0);
}


static DRIVER_INIT( vindctr2 )
{
	UINT8 *gfx2_base = memory_region(REGION_GFX2);
	UINT8 *data = malloc(0x8000);
	int i;

	common_init(118, 1);

	/* install our special ports */
	install_mem_read16_handler(0, 0x803000, 0x803003, vindctr2_port01_r);

	/* highly strange -- the address bits on the chip at 2J (and only that
	   chip) are scrambled -- this is verified on the schematics! */
	if (data)
	{
		memcpy(data, &gfx2_base[0x88000], 0x8000);
		for (i = 0; i < 0x8000; i++)
		{
			int srcoffs = (i & 0x4000) | ((i << 11) & 0x3800) | ((i >> 3) & 0x07ff);
			gfx2_base[0x88000 + i] = data[srcoffs];
		}
		free(data);
	}
}



/*************************************
 *
 *	Game driver(s)
 *
 *************************************/

GAME( 1985, gauntlet, 0,        gauntlet, gauntlet, gauntlet,  ROT0, "Atari Games", "Gauntlet (rev 14)" )
GAME( 1985, gaunts,   gauntlet, gauntlet, gauntlet, gauntlet,  ROT0, "Atari Games", "Gauntlet (Spanish, rev 15)" )
GAME( 1985, gauntj,   gauntlet, gauntlet, gauntlet, gauntlet,  ROT0, "Atari Games", "Gauntlet (Japanese, rev 13)" )
GAME( 1985, gauntg,   gauntlet, gauntlet, gauntlet, gauntlet,  ROT0, "Atari Games", "Gauntlet (German, rev 10)" )
GAME( 1985, gauntj12, gauntlet, gauntlet, gauntlet, gauntlet,  ROT0, "Atari Games", "Gauntlet (Japanese, rev 12)" )
GAME( 1985, gauntr9,  gauntlet, gauntlet, gauntlet, gauntlet,  ROT0, "Atari Games", "Gauntlet (rev 9)" )
GAME( 1985, gauntgr8, gauntlet, gauntlet, gauntlet, gauntlet,  ROT0, "Atari Games", "Gauntlet (German, rev 8)" )
GAME( 1985, gauntr7,  gauntlet, gauntlet, gauntlet, gauntlet,  ROT0, "Atari Games", "Gauntlet (rev 7)" )
GAME( 1985, gauntgr6, gauntlet, gauntlet, gauntlet, gauntlet,  ROT0, "Atari Games", "Gauntlet (German, rev 6)" )
GAME( 1985, gauntr5,  gauntlet, gauntlet, gauntlet, gauntlet,  ROT0, "Atari Games", "Gauntlet (rev 5)" )
GAME( 1985, gauntr4,  gauntlet, gauntlet, gauntlet, gauntlet,  ROT0, "Atari Games", "Gauntlet (rev 4)" )
GAME( 1985, gauntgr3, gauntlet, gauntlet, gauntlet, gauntlet,  ROT0, "Atari Games", "Gauntlet (German, rev 3)" )
GAME( 1985, gauntr2,  gauntlet, gauntlet, gauntlet, gauntlet,  ROT0, "Atari Games", "Gauntlet (rev 2)" )
GAME( 1985, gauntr1,  gauntlet, gauntlet, gauntlet, gauntlet,  ROT0, "Atari Games", "Gauntlet (rev 1)" )

GAME( 1985, gaunt2p,  gauntlet, gauntlet, gauntlet, gaunt2p,   ROT0, "Atari Games", "Gauntlet (2 Players, rev 6)" )
GAME( 1985, gaunt2pj, gauntlet, gauntlet, gauntlet, gaunt2p,   ROT0, "Atari Games", "Gauntlet (2 Players, Japanese, rev 5)" )
GAME( 1985, gaunt2pg, gauntlet, gauntlet, gauntlet, gaunt2p,   ROT0, "Atari Games", "Gauntlet (2 Players, German, rev 4)" )
GAME( 1985, gaun2pr3, gauntlet, gauntlet, gauntlet, gaunt2p,   ROT0, "Atari Games", "Gauntlet (2 Players, rev 3)" )
GAME( 1985, gaun2pj2, gauntlet, gauntlet, gauntlet, gaunt2p,   ROT0, "Atari Games", "Gauntlet (2 Players, Japanese, rev 2)" )
GAME( 1985, gaun2pg1, gauntlet, gauntlet, gauntlet, gaunt2p,   ROT0, "Atari Games", "Gauntlet (2 Players, German, rev 1)" )

GAME( 1986, gaunt2,   0,        gauntlet, gauntlet, gauntlet2, ROT0, "Atari Games", "Gauntlet II" )
GAME( 1986, gaunt2g,  gaunt2,   gauntlet, gauntlet, gauntlet2, ROT0, "Atari Games", "Gauntlet II (German)" )

GAME( 1986, gaunt22p, gaunt2,   gauntlet, gauntlet, gauntlet2, ROT0, "Atari Games", "Gauntlet II (2 Players, rev 2)" )
GAME( 1986, gaun22p1, gaunt2,   gauntlet, gauntlet, gauntlet2, ROT0, "Atari Games", "Gauntlet II (2 Players, rev 1)" )
GAME( 1986, gaun22pg, gaunt2,   gauntlet, gauntlet, gauntlet2, ROT0, "Atari Games", "Gauntlet II (2 Players, German)" )

GAME( 1988, vindctr2, 0,        gauntlet, vindctr2, vindctr2,  ROT0, "Atari Games", "Vindicators Part II (rev 3)" )
GAME( 1988, vindc2r2, vindctr2, gauntlet, vindctr2, vindctr2,  ROT0, "Atari Games", "Vindicators Part II (rev 2)" )
GAME( 1988, vindc2r1, vindctr2, gauntlet, vindctr2, vindctr2,  ROT0, "Atari Games", "Vindicators Part II (rev 1)" )
