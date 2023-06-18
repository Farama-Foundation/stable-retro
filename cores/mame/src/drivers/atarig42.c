/***************************************************************************

	Atari G42 hardware

	driver by Aaron Giles

	Games supported:
		* Road Riot 4WD (1991)
		* Danger Express (1992)
		* Guardians of the 'Hood (1992)

	Known bugs:
		* ASIC65 for Road Riot not quite perfect

****************************************************************************

	Memory map (TBA)

***************************************************************************/


#include "driver.h"
#include "machine/atarigen.h"
#include "machine/asic65.h"
#include "sndhrdw/atarijsa.h"
#include "vidhrdw/atarirle.h"
#include "atarig42.h"
#include "bootstrap.h"
#include "inptport.h"


/*************************************
 *
 *	Statics
 *
 *************************************/

static UINT8 analog_data;
static data16_t *mo_command;

static int sloop_bank;
static int sloop_next_bank;
static int sloop_offset;
static int sloop_state;
static data16_t *sloop_base;



/*************************************
 *
 *	Initialization & interrupts
 *
 *************************************/

static void update_interrupts(void)
{
	int newstate = 0;

	if (atarigen_video_int_state)
		newstate = 4;
	if (atarigen_sound_int_state)
		newstate = 5;

	if (newstate)
		cpu_set_irq_line(0, newstate, ASSERT_LINE);
	else
		cpu_set_irq_line(0, 7, CLEAR_LINE);
}


static MACHINE_INIT( atarig42 )
{
	atarigen_eeprom_reset();
	atarigen_interrupt_reset(update_interrupts);
	atarigen_scanline_timer_reset(atarig42_scanline_update, 8);
	atarijsa_reset();
}



/*************************************
 *
 *	I/O read dispatch.
 *
 *************************************/

static READ16_HANDLER( special_port2_r )
{
	int temp = readinputport(2);
	if (atarigen_cpu_to_sound_ready) temp ^= 0x0020;
	if (atarigen_sound_to_cpu_ready) temp ^= 0x0010;
	temp ^= 0x0008;		/* A2D.EOC always high for now */
	return temp;
}


static WRITE16_HANDLER( a2d_select_w )
{
	analog_data = readinputport(4 + (offset != 0));
}


static READ16_HANDLER( a2d_data_r )
{
	return analog_data << 8;
}


static WRITE16_HANDLER( io_latch_w )
{
	/* upper byte */
	if (ACCESSING_MSB)
	{
		/* bit 14 controls the ASIC65 reset line */
		asic65_reset((~data >> 14) & 1);

		/* bits 13-11 are the MO control bits */
		atarirle_control_w(0, (data >> 11) & 7);
	}

	/* lower byte */
	if (ACCESSING_LSB)
	{
		/* bit 4 resets the sound CPU */
		cpu_set_reset_line(1, (data & 0x10) ? CLEAR_LINE : ASSERT_LINE);
		if (!(data & 0x10)) atarijsa_reset();

		/* bit 5 is /XRESET, probably related to the ASIC */

		/* bits 3 and 0 are coin counters */
	}
}


static WRITE16_HANDLER( mo_command_w )
{
	COMBINE_DATA(mo_command);
	atarirle_command_w(0, (data == 0) ? ATARIRLE_COMMAND_CHECKSUM : ATARIRLE_COMMAND_DRAW);
}



/*************************************
 *
 *	SLOOP banking -- Road Riot
 *
 *************************************/

static void roadriot_sloop_tweak(int offset)
{
/*
	sequence 1:

		touch $68000
		touch $68eee and $124/$678/$abc/$1024(bank) in the same instruction
		touch $69158/$6a690/$6e708/$71166

	sequence 2:

		touch $5edb4 to add 2 to the bank
		touch $5db0a to add 1 to the bank
		touch $5f042
		touch $69158/$6a690/$6e708/$71166
		touch $68000
		touch $5d532/$5d534
*/

	switch (offset)
	{
		/* standard 68000 -> 68eee -> (bank) addressing */
		case 0x68000/2:
			sloop_state = 1;
			break;
		case 0x68eee/2:
			if (sloop_state == 1)
				sloop_state = 2;
			break;
		case 0x00124/2:
			if (sloop_state == 2)
			{
				sloop_next_bank = 0;
				sloop_state = 3;
			}
			break;
		case 0x00678/2:
			if (sloop_state == 2)
			{
				sloop_next_bank = 1;
				sloop_state = 3;
			}
			break;
		case 0x00abc/2:
			if (sloop_state == 2)
			{
				sloop_next_bank = 2;
				sloop_state = 3;
			}
			break;
		case 0x01024/2:
			if (sloop_state == 2)
			{
				sloop_next_bank = 3;
				sloop_state = 3;
			}
			break;

		/* lock in the change? */
		case 0x69158/2:
			/* written if $ff8007 == 0 */
		case 0x6a690/2:
			/* written if $ff8007 == 1 */
		case 0x6e708/2:
			/* written if $ff8007 == 2 */
		case 0x71166/2:
			/* written if $ff8007 == 3 */
			if (sloop_state == 3)
				sloop_bank = sloop_next_bank;
			sloop_state = 0;
			break;

		/* bank offsets */
		case 0x5edb4/2:
			if (sloop_state == 0)
			{
				sloop_state = 10;
				sloop_offset = 0;
			}
			sloop_offset += 2;
			break;
		case 0x5db0a/2:
			if (sloop_state == 0)
			{
				sloop_state = 10;
				sloop_offset = 0;
			}
			sloop_offset += 1;
			break;

		/* apply the offset */
		case 0x5f042/2:
			if (sloop_state == 10)
			{
				sloop_bank = (sloop_bank + sloop_offset) & 3;
				sloop_offset = 0;
				sloop_state = 0;
			}
			break;

		/* unknown */
		case 0x5d532/2:
			break;
		case 0x5d534/2:
			break;

		default:
			break;
	}
}


static READ16_HANDLER( roadriot_sloop_data_r )
{
	roadriot_sloop_tweak(offset);
	if (offset < 0x78000/2)
		return sloop_base[offset];
	else
		return sloop_base[0x78000/2 + sloop_bank * 0x1000 + (offset & 0xfff)];
}


static WRITE16_HANDLER( roadriot_sloop_data_w )
{
	roadriot_sloop_tweak(offset);
}



/*************************************
 *
 *	SLOOP banking -- Guardians
 *
 *************************************/

static void guardians_sloop_tweak(int offset)
{
	static UINT32 last_accesses[8];

	if (offset >= 0x7f7c0/2)
	{
		last_accesses[0] = last_accesses[1];
		last_accesses[1] = last_accesses[2];
		last_accesses[2] = last_accesses[3];
		last_accesses[3] = last_accesses[4];
		last_accesses[4] = last_accesses[5];
		last_accesses[5] = last_accesses[6];
		last_accesses[6] = last_accesses[7];
		last_accesses[7] = offset;

		if (last_accesses[0] == 0x7f7c0/2 && last_accesses[1] == 0x7f7ce/2 && last_accesses[2] == 0x7f7c2/2 && last_accesses[3] == 0x7f7cc/2 && 
			last_accesses[4] == 0x7f7c4/2 && last_accesses[5] == 0x7f7ca/2 && last_accesses[6] == 0x7f7c6/2 && last_accesses[7] == 0x7f7c8/2)
			sloop_bank = 0;

		if (last_accesses[0] == 0x7f7d0/2 && last_accesses[1] == 0x7f7de/2 && last_accesses[2] == 0x7f7d2/2 && last_accesses[3] == 0x7f7dc/2 && 
			last_accesses[4] == 0x7f7d4/2 && last_accesses[5] == 0x7f7da/2 && last_accesses[6] == 0x7f7d6/2 && last_accesses[7] == 0x7f7d8/2)
			sloop_bank = 1;

		if (last_accesses[0] == 0x7f7e0/2 && last_accesses[1] == 0x7f7ee/2 && last_accesses[2] == 0x7f7e2/2 && last_accesses[3] == 0x7f7ec/2 && 
			last_accesses[4] == 0x7f7e4/2 && last_accesses[5] == 0x7f7ea/2 && last_accesses[6] == 0x7f7e6/2 && last_accesses[7] == 0x7f7e8/2)
			sloop_bank = 2;

		if (last_accesses[0] == 0x7f7f0/2 && last_accesses[1] == 0x7f7fe/2 && last_accesses[2] == 0x7f7f2/2 && last_accesses[3] == 0x7f7fc/2 && 
			last_accesses[4] == 0x7f7f4/2 && last_accesses[5] == 0x7f7fa/2 && last_accesses[6] == 0x7f7f6/2 && last_accesses[7] == 0x7f7f8/2)
			sloop_bank = 3;
	}
}


static READ16_HANDLER( guardians_sloop_data_r )
{
	guardians_sloop_tweak(offset);
	if (offset < 0x78000/2)
		return sloop_base[offset];
	else
		return sloop_base[0x78000/2 + sloop_bank * 0x1000 + (offset & 0xfff)];
}


static WRITE16_HANDLER( guardians_sloop_data_w )
{
	guardians_sloop_tweak(offset);
}



/*************************************
 *
 *	Main CPU memory handlers
 *
 *************************************/

static MEMORY_READ16_START( main_readmem )
	{ 0x000000, 0x080001, MRA16_ROM },
	{ 0xe00000, 0xe00001, input_port_0_word_r },
	{ 0xe00002, 0xe00003, input_port_1_word_r },
	{ 0xe00010, 0xe00011, special_port2_r },
	{ 0xe00012, 0xe00013, input_port_3_word_r },
	{ 0xe00020, 0xe00027, a2d_data_r },
	{ 0xe00030, 0xe00031, atarigen_sound_r },
	{ 0xe80000, 0xe80fff, MRA16_RAM },
	{ 0xf40000, 0xf40001, asic65_io_r },
	{ 0xf60000, 0xf60001, asic65_r },
	{ 0xfa0000, 0xfa0fff, atarigen_eeprom_r },
	{ 0xfc0000, 0xfc0fff, MRA16_RAM },
	{ 0xff0000, 0xffffff, MRA16_RAM },
MEMORY_END


static MEMORY_WRITE16_START( main_writemem )
	{ 0x000000, 0x080001, MWA16_ROM },
	{ 0xe00020, 0xe00027, a2d_select_w },
	{ 0xe00040, 0xe00041, atarigen_sound_w },
	{ 0xe00050, 0xe00051, io_latch_w },
	{ 0xe00060, 0xe00061, atarigen_eeprom_enable_w },
	{ 0xe03000, 0xe03001, atarigen_video_int_ack_w },
	{ 0xe03800, 0xe03801, watchdog_reset16_w },
	{ 0xe80000, 0xe80fff, MWA16_RAM },
	{ 0xf80000, 0xf80003, asic65_data_w },
	{ 0xfa0000, 0xfa0fff, atarigen_eeprom_w, &atarigen_eeprom, &atarigen_eeprom_size },
	{ 0xfc0000, 0xfc0fff, atarigen_666_paletteram_w, &paletteram16 },
	{ 0xff0000, 0xff0fff, atarirle_0_spriteram_w, &atarirle_0_spriteram },
	{ 0xff1000, 0xff1fff, MWA16_RAM },
	{ 0xff2000, 0xff5fff, atarigen_playfield_w, &atarigen_playfield },
	{ 0xff6000, 0xff6fff, atarigen_alpha_w, &atarigen_alpha },
	{ 0xff7000, 0xff7001, mo_command_w, &mo_command },
	{ 0xff7002, 0xffffff, MWA16_RAM },
MEMORY_END



/*************************************
 *
 *	Port definitions
 *
 *************************************/

INPUT_PORTS_START( roadriot )
	PORT_START		/* e00000 */
	PORT_BIT( 0x00ff, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER1 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0xf800, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START      /* e00002 */
	PORT_BIT( 0xffff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START		/* e00010 */
	PORT_BIT( 0x003f, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_SERVICE( 0x0040, IP_ACTIVE_LOW )
	PORT_BIT(  0x0080, IP_ACTIVE_LOW, IPT_VBLANK )
	PORT_BIT( 0xff00, IP_ACTIVE_LOW, IPT_UNUSED )

	JSA_III_PORT	/* audio board port */

	PORT_START		/* analog 0 */
	PORT_ANALOG( 0xff, 0x80, IPT_AD_STICK_X, 50, 10, 0, 255 )

	PORT_START      /* analog 1 */
	PORT_ANALOG( 0xff, 0x00, IPT_PEDAL, 100, 16, 0x00, 0xff )
INPUT_PORTS_END


INPUT_PORTS_START( guardian )
	PORT_START		/* e00000 */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_BUTTON5 | IPF_PLAYER3 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER3 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER3 )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER3 )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER3 )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER3 )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER3 )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_BUTTON5 | IPF_PLAYER1 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER1 )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER1 )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER1 )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER1 )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER1 )

	PORT_START      /* e00002 */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER1 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER3 )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER3 )
	PORT_BIT( 0x01c0, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_BUTTON5 | IPF_PLAYER2 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER2 )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER2 )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER2 )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER2 )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER2 )

	PORT_START		/* e00010 */
	PORT_BIT( 0x003f, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_SERVICE( 0x0040, IP_ACTIVE_LOW )
	PORT_BIT(  0x0080, IP_ACTIVE_LOW, IPT_VBLANK )
	PORT_BIT( 0xff00, IP_ACTIVE_LOW, IPT_UNUSED )

	JSA_III_PORT	/* audio board port */

	PORT_START		/* analog 0 */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START      /* analog 1 */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )
INPUT_PORTS_END

INPUT_PORTS_START( dangerex )
	PORT_START		/* e00000 */
	PORT_BIT( 0x003f, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_UNUSED ) /* Toggle 0 - D4 */
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_UNUSED ) /* Toggle 1 - D5 */
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_UNUSED ) /* Step SW - D6 */
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNUSED ) /* Freeze - D7 */
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER1 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER1 )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_PLAYER1 )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_PLAYER1 )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_PLAYER1 )

	PORT_START      /* e00002 */
	PORT_BIT( 0x000f, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_UNUSED ) /* Test Yellow - D4 */
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_UNUSED ) /* Test Blue - D5 */
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_UNUSED ) /* Test Blue - D6 */
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNUSED ) /* Test Blue - D7 */
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER2 )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_PLAYER2 )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_PLAYER2 )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_PLAYER2 )

	PORT_START		/* e00010 */
	PORT_BIT( 0x003f, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_SERVICE( 0x0040, IP_ACTIVE_LOW )
	PORT_BIT(  0x0080, IP_ACTIVE_LOW, IPT_VBLANK )
	PORT_BIT( 0xff00, IP_ACTIVE_LOW, IPT_UNUSED )

	JSA_III_PORT	/* audio board port */

	PORT_START		/* analog 0 */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START      /* analog 1 */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )
INPUT_PORTS_END



/*************************************
 *
 *	Graphics definitions
 *
 *************************************/

static struct GfxLayout pflayout =
{
	8,8,
	RGN_FRAC(1,3),
	5,
	{ 0, 0, 1, 2, 3 },
	{ RGN_FRAC(1,3)+0, RGN_FRAC(1,3)+4, 0, 4, RGN_FRAC(1,3)+8, RGN_FRAC(1,3)+12, 8, 12 },
	{ 0*8, 2*8, 4*8, 6*8, 8*8, 10*8, 12*8, 14*8 },
	16*8
};

static struct GfxLayout pftoplayout =
{
	8,8,
	RGN_FRAC(1,3),
	6,
	{ RGN_FRAC(2,3)+0, RGN_FRAC(2,3)+4, 0, 0, 0, 0 },
	{ 3, 2, 1, 0, 11, 10, 9, 8 },
	{ 0*8, 2*8, 4*8, 6*8, 8*8, 10*8, 12*8, 14*8 },
	16*8
};

static struct GfxLayout anlayout =
{
	8,8,
	RGN_FRAC(1,1),
	4,
	{ 0, 1, 2, 3 },
	{ 0, 4, 8, 12, 16, 20, 24, 28 },
	{ 0*8, 4*8, 8*8, 12*8, 16*8, 20*8, 24*8, 28*8 },
	32*8
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &pflayout, 0x000, 64 },
	{ REGION_GFX2, 0, &anlayout, 0x000, 16 },
	{ REGION_GFX1, 0, &pftoplayout, 0x000, 64 },
	{ -1 } /* end of array */
};



/*************************************
 *
 *	Machine driver
 *
 *************************************/

static MACHINE_DRIVER_START( atarig42 )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, ATARI_CLOCK_14MHz)
	MDRV_CPU_MEMORY(main_readmem,main_writemem)
	MDRV_CPU_VBLANK_INT(atarigen_video_int_gen,1)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)

	MDRV_MACHINE_INIT(atarig42)
	MDRV_NVRAM_HANDLER(atarigen)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_NEEDS_6BITS_PER_GUN | VIDEO_UPDATE_BEFORE_VBLANK)
	MDRV_SCREEN_SIZE(42*8, 30*8)
	MDRV_VISIBLE_AREA(0*8, 42*8-1, 0*8, 30*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(2048)

	MDRV_VIDEO_START(atarig42)
	MDRV_VIDEO_EOF(atarirle)
	MDRV_VIDEO_UPDATE(atarig42)

	/* sound hardware */
	MDRV_IMPORT_FROM(jsa_iii_mono)
MACHINE_DRIVER_END



/*************************************
 *
 *	ROM definition(s)
 *
 *************************************/

ROM_START( roadriot )
	ROM_REGION( 0x80004, REGION_CPU1, 0 )	/* 8*64k for 68000 code */
	ROM_LOAD16_BYTE( "rriot.8d", 0x00000, 0x20000, CRC(bf8aaafc) SHA1(1594d91b56609d49921c866d8f5796619e79217b) )
	ROM_LOAD16_BYTE( "rriot.8c", 0x00001, 0x20000, CRC(5dd2dd70) SHA1(8f6a0e809ec1f6feea8a18197a789086a7b9dd6a) )
	ROM_LOAD16_BYTE( "rriot.9d", 0x40000, 0x20000, CRC(6191653c) SHA1(97d1a84a585149e8f2c49cab7af22dc755dff350) )
	ROM_LOAD16_BYTE( "rriot.9c", 0x40001, 0x20000, CRC(0d34419a) SHA1(f16e9fb4cd537d727611cb7dd5537c030671fe1e) )

	ROM_REGION( 0x14000, REGION_CPU2, 0 )	/* 64k for 6502 code */
	ROM_LOAD( "rriots.12c", 0x10000, 0x4000, CRC(849dd26c) SHA1(05a0b2a5f7ee4437448b5f076d3066d96dec2320) )
	ROM_CONTINUE(           0x04000, 0xc000 )

	ROM_REGION( 0xc0000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "rriot.22d",    0x000000, 0x20000, CRC(b7451f92) SHA1(9fd17913630e457e406e596f2d86afff98787750) ) /* playfield, planes 0-1 */
	ROM_LOAD( "rriot.22c",    0x020000, 0x20000, CRC(90f3c6ee) SHA1(7607509e2d3b2080a918cfaf2879dbed6b79d029) )
	ROM_LOAD( "rriot20.21d",  0x040000, 0x20000, CRC(d40de62b) SHA1(fa6dfd20bdad7874ae33a1027a9bb0ea200f86ca) ) /* playfield, planes 2-3 */
	ROM_LOAD( "rriot20.21c",  0x060000, 0x20000, CRC(a7e836b1) SHA1(d41f1e4166ca757176c6976be2a953db5db05e48) )
	ROM_LOAD( "rriot.20d",    0x080000, 0x20000, CRC(a81ae93f) SHA1(b694ba5fab35f8fa505a02039ae62f7af3c7ae1d) ) /* playfield, planes 4-5 */
	ROM_LOAD( "rriot.20c",    0x0a0000, 0x20000, CRC(b8a6d15a) SHA1(43d2be9d40a84b2c01d80bbcac737eda04d55999) )

	ROM_REGION( 0x020000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "rriot.22j",    0x000000, 0x20000, CRC(0005bab0) SHA1(257e1b23eea117fe6701a67134b96d9d9fe10caf) ) /* alphanumerics */

	ROM_REGION16_BE( 0x200000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "rriot.2s", 0x000000, 0x20000, CRC(19590a94) SHA1(e375b7e01a8b1f366bb4e7750e33f0b6d9ae2042) )
	ROM_LOAD16_BYTE( "rriot.2p", 0x000001, 0x20000, CRC(c2bf3f69) SHA1(f822359070b1907973ee7ee35469f4a59f720830) )
	ROM_LOAD16_BYTE( "rriot.3s", 0x040000, 0x20000, CRC(bab110e4) SHA1(0c4e3521474249517e7832df1bc63aca6d6a6c91) )
	ROM_LOAD16_BYTE( "rriot.3p", 0x040001, 0x20000, CRC(791ad2c5) SHA1(4ef218fbf38a9c6b58c86f203843988df1c935f6) )
	ROM_LOAD16_BYTE( "rriot.4s", 0x080000, 0x20000, CRC(79cba484) SHA1(ce361a432f1fe627086bab3c1131118fd15056f1) )
	ROM_LOAD16_BYTE( "rriot.4p", 0x080001, 0x20000, CRC(86a2e257) SHA1(98d95d2e67fecc332f6c66358a1f8d58b168c74b) )
	ROM_LOAD16_BYTE( "rriot.5s", 0x0c0000, 0x20000, CRC(67d28478) SHA1(cfc9da6d20c65d11c2a59a38660a8da4d1cc219d) )
	ROM_LOAD16_BYTE( "rriot.5p", 0x0c0001, 0x20000, CRC(74638838) SHA1(bea0fb21ccb946e023c88791ce5a8dd92b44baec) )
	ROM_LOAD16_BYTE( "rriot.6s", 0x100000, 0x20000, CRC(24933c37) SHA1(516393aae51fc9634a5c6d5134be058d6067e114) )
	ROM_LOAD16_BYTE( "rriot.6p", 0x100001, 0x20000, CRC(054a163b) SHA1(1b0b129c093398bc5c14b3fdd87dfe149f555fac) )
	ROM_LOAD16_BYTE( "rriot.7s", 0x140000, 0x20000, CRC(31ff090a) SHA1(7b43ed37901c3f94cae90c84b3c8c689d7b64dd6) )
	ROM_LOAD16_BYTE( "rriot.7p", 0x140001, 0x20000, CRC(bbe5b69b) SHA1(9eaa551fba763824d36fc41bfe0e6d735a9e68c5) )
	ROM_LOAD16_BYTE( "rriot.8s", 0x180000, 0x20000, CRC(6c89d2c5) SHA1(0bf2990ce1cd5ec5b84f7e3171725540e6238408) )
	ROM_LOAD16_BYTE( "rriot.8p", 0x180001, 0x20000, CRC(40d9bde5) SHA1(aca6e07ea96e4618412d32fe4d4cd293ae82d940) )
	ROM_LOAD16_BYTE( "rriot.9s", 0x1c0000, 0x20000, CRC(eca3c595) SHA1(5d067b7c02675b1e6dd3c4046697a16f873f80af) )
	ROM_LOAD16_BYTE( "rriot.9p", 0x1c0001, 0x20000, CRC(88acdb53) SHA1(5bf2424ee75a25248a8ce38c8605b6780da4e323) )

	ROM_REGION( 0x100000, REGION_SOUND1, 0 )	/* 1MB for ADPCM samples */
	ROM_LOAD( "rriots.19e",  0x80000, 0x20000, CRC(2db638a7) SHA1(45da8088f7439beacc3056952a4d631d9633efa7) )
	ROM_LOAD( "rriots.17e",  0xa0000, 0x20000, CRC(e1dd7f9e) SHA1(6b9a240aa84d210d3052daab6ea26f9cd0e62dc1) )
	ROM_LOAD( "rriots.15e",  0xc0000, 0x20000, CRC(64d410bb) SHA1(877bccca7ff37a9dd8294bc1453487a2f516ca7d) )
	ROM_LOAD( "rriots.12e",  0xe0000, 0x20000, CRC(bffd01c8) SHA1(f6de000f61ea0c1ddb31ee5301506e5e966638c2) )

	ROM_REGION( 0x0600, REGION_PROMS, ROMREGION_DISPOSE )	/* microcode for growth renderer */
	ROM_LOAD( "089-1001.bin",  0x0000, 0x0200, CRC(5836cb5a) SHA1(2c797f6a1227d6e1fd7a12f99f0254072c8c266e) )
	ROM_LOAD( "089-1002.bin",  0x0200, 0x0200, CRC(44288753) SHA1(811582015264f85a32643196cdb331a41430318f) )
	ROM_LOAD( "089-1003.bin",  0x0400, 0x0200, CRC(1f571706) SHA1(26d5ea59163b3482ab1f8a26178d0849c5fd9692) )
ROM_END


ROM_START( guardian )
	ROM_REGION( 0x80004, REGION_CPU1, 0 )	/* 8*64k for 68000 code */
	ROM_LOAD16_BYTE( "2021.8e",  0x00000, 0x20000, CRC(efea1e02) SHA1(f0f1ef300f36953aff73b68ffe6d9950ac575f7d) )
	ROM_LOAD16_BYTE( "2020.8cd", 0x00001, 0x20000, CRC(a8f655ba) SHA1(2defe4d138613e248718a617d103794e051746f7) )
	ROM_LOAD16_BYTE( "2023.9e",  0x40000, 0x20000, CRC(cfa29316) SHA1(4e0e76304e29ee59bc2ce9a704e3f651dc9d473c) )
	ROM_LOAD16_BYTE( "2022.9cd", 0x40001, 0x20000, CRC(ed2abc91) SHA1(81531040d5663f6ab82e924210056e3737e17a8d) )

	ROM_REGION( 0x14000, REGION_CPU2, 0 )	/* 64k for 6502 code */
	ROM_LOAD( "0080-snd.12c", 0x10000, 0x4000, CRC(0388f805) SHA1(49c11313bc4192dbe294cf68b652cb19047889fd) )
	ROM_CONTINUE(             0x04000, 0xc000 )

	ROM_REGION( 0x180000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "0037a.23e",  0x000000, 0x80000, CRC(ca10b63e) SHA1(243a2a440e1bc9135d3dbe6553d39c54b9bdcd13) ) /* playfield, planes 0-1 */
	ROM_LOAD( "0038a.22e",  0x080000, 0x80000, CRC(cb1431a1) SHA1(d7b8f49a1e794ca2083e4bf0fa3870ce08caa53a) ) /* playfield, planes 2-3 */
	ROM_LOAD( "0039a.20e",  0x100000, 0x80000, CRC(2eee7188) SHA1(d3adbd7b20bc898fee35b6ba781e7775f82acd19) ) /* playfield, planes 4-5 */

	ROM_REGION( 0x020000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "0030.23k",   0x000000, 0x20000, CRC(0fd7baa1) SHA1(7802d732e5173291628ed498ad0fab71aeef4688) ) /* alphanumerics */

	ROM_REGION16_BE( 0x600000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "0041.2s",  0x000000, 0x80000, CRC(a2a5ae08) SHA1(d99f925bbc9a72432e13328ee8422fde615db90f) )
	ROM_LOAD16_BYTE( "0040.2p",  0x000001, 0x80000, CRC(ef95132e) SHA1(288de1d15956a612b7d19ceb2cf853490bf42b05) )
	ROM_LOAD16_BYTE( "0043.3s",  0x100000, 0x80000, CRC(6438b8e4) SHA1(ee1446209fbcab8b17c88c53b65e754a85f279d1) )
	ROM_LOAD16_BYTE( "0042.3p",  0x100001, 0x80000, CRC(46bf7c0d) SHA1(12414de2698178b158ec4ca0fbb176943c944cec) )
	ROM_LOAD16_BYTE( "0045.4s",  0x200000, 0x80000, CRC(4f4f2bee) SHA1(8276cdcd252d2d8fa41ab28e76a6bd72613c14ec) )
	ROM_LOAD16_BYTE( "0044.4p",  0x200001, 0x80000, CRC(20a4250b) SHA1(6a2e2596a9eef2792f7cdab648dd455b8e420a74) )
	ROM_LOAD16_BYTE( "0063a.6s", 0x300000, 0x80000, CRC(93933bcf) SHA1(a67d4839ffdb0eafbc2d68a60fb3bf1507c793cf) )
	ROM_LOAD16_BYTE( "0062a.6p", 0x300001, 0x80000, CRC(613e6f1d) SHA1(fd2ea18d245d0895e0bac6c5caa6d35fdd6a199f) )
	ROM_LOAD16_BYTE( "0065a.7s", 0x400000, 0x80000, CRC(6bcd1205) SHA1(c883c55f88d274ba8aa48c962406b253e1f8001e) )
	ROM_LOAD16_BYTE( "0064a.7p", 0x400001, 0x80000, CRC(7b4dce05) SHA1(36545917388e704f73a9b4d85189ec978d655b63) )
	ROM_LOAD16_BYTE( "0067a.9s", 0x500000, 0x80000, CRC(15845fba) SHA1(f7b670a8d48a5e9c261150914a06ef9a938a84e7) )
	ROM_LOAD16_BYTE( "0066a.9p", 0x500001, 0x80000, CRC(7130c575) SHA1(b3ea109981a1e5c631705b23dfad4a3a3daf7734) )

	ROM_REGION( 0x100000, REGION_SOUND1, 0 )	/* 1MB for ADPCM samples */
	ROM_LOAD( "0010-snd",  0x80000, 0x80000, CRC(bca27f40) SHA1(91a41eac116eb7d9a790abc590eb06328726d1c2) )

	ROM_REGION( 0x0600, REGION_PROMS, ROMREGION_DISPOSE )	/* microcode for growth renderer */
	ROM_LOAD( "092-1001.bin",  0x0000, 0x0200, CRC(b3251eeb) SHA1(5e83baa70aaa28f07f32657bf974fd87719972d3) )
	ROM_LOAD( "092-1002.bin",  0x0200, 0x0200, CRC(0c5314da) SHA1(a9c7ee3ab015c7f3ada4200acd2854eb9a5c74b0) )
	ROM_LOAD( "092-1003.bin",  0x0400, 0x0200, CRC(344b406a) SHA1(f4422f8c0d7004d0277a4fc77718d555f80fcf69) )
ROM_END

ROM_START( dangerex )
	ROM_REGION( 0x80004, REGION_CPU1, 0 )	/* 68000 code */
	ROM_LOAD16_BYTE( "dx8d-0h.8d", 0x00000, 0x20000, CRC(4957b65d) SHA1(de9f187b6496cf96d29c4b1b29887abc2bdf9bf0) )
	ROM_LOAD16_BYTE( "dx8c-0l.8c", 0x00001, 0x20000, CRC(aedcb497) SHA1(7e201b7db5c0ff661f782566a6b17299d514c77a) )
	ROM_LOAD16_BYTE( "dx9d-1h.9d", 0x40000, 0x20000, CRC(2eb943e2) SHA1(87dbf11720e2938bf5755b13231fc668ab3e0e05) )
	ROM_LOAD16_BYTE( "dx9c-1l.9c", 0x40001, 0x20000, CRC(79de4c91) SHA1(31de5e927aff4efcf4217da3c704ece2d393faf9) )

	ROM_REGION( 0x14000, REGION_CPU2, 0 ) /* 6502 code */
	ROM_LOAD( "dx12c-5.12c", 0x10000, 0x4000, CRC(d72621f7) SHA1(4bf5c98dd2434cc6ed1bddb6baf42f41cf138e1a) )
	ROM_CONTINUE(            0x04000, 0xc000 )

	ROM_REGION( 0xc0000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "dxc117-22d.22d",    0x000000, 0x20000, CRC(5532995a) SHA1(21e001c911adb91dbe43e895ae8582df65f2995d) ) /* playfield, planes 0-1 */
	ROM_LOAD( "dx82-22c.22c",      0x020000, 0x20000, CRC(9548599b) SHA1(d08bae8dabce0175f956631ddfbf091653af035e) )
	ROM_LOAD( "dxc116-20-21d.21d", 0x040000, 0x20000, CRC(ebbf0fd8) SHA1(4ceb026c4231b675215110c16c8f75551cdfa461) ) /* playfield, planes 2-3 */
	ROM_LOAD( "dxc81-22-21c.21c",  0x060000, 0x20000, CRC(24cb1d34) SHA1(4fc558c8dee3654abd164e5e4adddf9bfcbca3ae) )
	ROM_LOAD( "dxc115-20d.20d",    0x080000, 0x20000, CRC(2819ce54) SHA1(9a3c041d9046af41997dc1d9f41bf0e1be9489f9) ) /* playfield, planes 4-5 */
	ROM_LOAD( "dxc80-20c.20c",     0x0a0000, 0x20000, CRC(a8ffe459) SHA1(92a10694c38a4fbe3022662f4e8e4e214aab31c9) )

	ROM_REGION( 0x020000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "dxc187-22j.22j",   0x000000, 0x20000, CRC(7231ecc2) SHA1(8b1b0aed3a0d907630e120395b0a97fd9a1ef8cc) ) /* alphanumerics */

	ROM_REGION16_BE( 0x800000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "dx2s-0h.2s", 0x000000, 0x80000, CRC(89902ce2) SHA1(f910ad65f3780e28c9920c4185c1ea807ec478aa) )
	ROM_LOAD16_BYTE( "dx2p-0l.2p", 0x000001, 0x80000, CRC(dabe7e1c) SHA1(1f77bba57b7025333c27ee3d548d08ee960d63d6) )
	ROM_LOAD16_BYTE( "dx3s-1h.3s", 0x100000, 0x80000, CRC(ffeec3d1) SHA1(de40083ce3862f2b5d37f5f255f93b2b2487ed96) )
	ROM_LOAD16_BYTE( "dx3p-1l.3p", 0x100001, 0x80000, CRC(40b0a300) SHA1(3ec055bdc30e62c5e95541b15c53f1d439ccb5b4) )
	ROM_LOAD16_BYTE( "dx4s-2h.4s", 0x200000, 0x80000, CRC(1e4d0c50) SHA1(fbb5422f43e1c4f8787073c1cdaeb75121a67a35) )
	ROM_LOAD16_BYTE( "dx4p-2l.4p", 0x200001, 0x80000, CRC(00d586e1) SHA1(7e8419a5972a5e0fc372c72b5c5f8f3ff4294a2b) )
	ROM_LOAD16_BYTE( "dx5s-3h.5s", 0x300000, 0x80000, CRC(98e26315) SHA1(dbc567c3b9d00f827acb8ba2d5a79a7d2acc7ddd) )
	ROM_LOAD16_BYTE( "dx5p-3l.5p", 0x300001, 0x80000, CRC(e37b1413) SHA1(490911006f0e10319117dae3a87623d8244ea182) )
	ROM_LOAD16_BYTE( "dx6s-4h.6s", 0x400000, 0x80000, CRC(b2bc538c) SHA1(1bda4a6c40c9389573857879263c0f01b5f029d3) )
	ROM_LOAD16_BYTE( "dx6p-4l.6p", 0x400001, 0x80000, CRC(5e3aefb8) SHA1(492eb7f42bbeefbc377d3a491fe07b64c1c8a11c) )
	ROM_LOAD16_BYTE( "dx7s-5h.7s", 0x500000, 0x80000, CRC(396d706e) SHA1(7682287df2ad2283b5999c3df272baa9559b7bd5) )
	ROM_LOAD16_BYTE( "dx7p-5l.7p", 0x500001, 0x80000, CRC(102c827d) SHA1(6ae405032a07081841a5a3fb48422bd67d37372c) )
	ROM_LOAD16_BYTE( "dx8s-6h.8s", 0x600000, 0x80000, CRC(af3b1f90) SHA1(d514a7b5e9bb263bbb1164a5174fa6943c4e5fb4) )
	ROM_LOAD16_BYTE( "dx8p-6l.8p", 0x600001, 0x80000, CRC(a0e7311b) SHA1(9c4d443c727c3bd59f035bf27c4f7e74046d4c45) )
	ROM_LOAD16_BYTE( "dx9s-7h.7h", 0x700000, 0x80000, CRC(5bf0ce01) SHA1(22f971842371eb36b2dc6ae303ef3955dd9884c2) )
	ROM_LOAD16_BYTE( "dx9p-7l.7l", 0x700001, 0x80000, CRC(e6f1d9fa) SHA1(160b4c9a90bdc48c990e5d4a24b17a284c9b4da8) )

	ROM_REGION( 0x100000, REGION_SOUND1, 0 )
	ROM_LOAD( "dx19e-0.19e",  0x80000, 0x20000, CRC(e7d9d5fb) SHA1(756b1d59168855a707181dd6c437a59313da5a7c) )
	ROM_LOAD( "dx17e-1.17e",  0xa0000, 0x20000, CRC(ccb73a18) SHA1(3e853f7c7ab32b18fdb6529d37d28eb96c5365dc) )
	ROM_LOAD( "dx15e-2.15e",  0xc0000, 0x20000, CRC(11596234) SHA1(77eab7cb4ad83a50c23127b4fb1bbfd4aa2c6f8d) )
	ROM_LOAD( "dx12e-3.12e",  0xe0000, 0x20000, CRC(c0ffd43c) SHA1(dcd7e3cc5d46db0d0a7fe3806bddbca235492d35) )

	ROM_REGION( 0x0600, REGION_PROMS, ROMREGION_DISPOSE )	/* microcode for growth renderer - no_dump borrowed from guardian */
	ROM_LOAD( "092-1001.bin",  0x0000, 0x0200, CRC(b3251eeb) SHA1(5e83baa70aaa28f07f32657bf974fd87719972d3) )
	ROM_LOAD( "092-1002.bin",  0x0200, 0x0200, CRC(0c5314da) SHA1(a9c7ee3ab015c7f3ada4200acd2854eb9a5c74b0) )
	ROM_LOAD( "092-1003.bin",  0x0400, 0x0200, CRC(344b406a) SHA1(f4422f8c0d7004d0277a4fc77718d555f80fcf69) )
ROM_END


/*************************************
 *
 *	Driver initialization
 *
 *************************************/

static DRIVER_INIT( roadriot )
{
	static const UINT16 default_eeprom[] =
	{
		0x0001,0x01B7,0x01AF,0x01E4,0x0100,0x0130,0x0300,0x01CC,
		0x0700,0x01FE,0x0500,0x0102,0x0200,0x0108,0x011B,0x01C8,
		0x0100,0x0107,0x0120,0x0100,0x0125,0x0500,0x0177,0x0162,
		0x013A,0x010A,0x01B7,0x01AF,0x01E4,0x0100,0x0130,0x0300,
		0x01CC,0x0700,0x01FE,0x0500,0x0102,0x0200,0x0108,0x011B,
		0x01C8,0x0100,0x0107,0x0120,0x0100,0x0125,0x0500,0x0177,
		0x0162,0x013A,0x010A,0xE700,0x0164,0x0106,0x0100,0x0104,
		0x01B0,0x0146,0x012E,0x1A00,0x01C8,0x01D0,0x0118,0x0D00,
		0x0118,0x0100,0x01C8,0x01D0,0x0000
	};
	atarigen_eeprom_default = default_eeprom;
	atarijsa_init(1, 3, 2, 0x0040);
	atarijsa3_init_adpcm(REGION_SOUND1);

	atarig42_playfield_base = 0x400;
	atarig42_motion_object_base = 0x200;
	atarig42_motion_object_mask = 0x1ff;

	sloop_base = install_mem_read16_handler(0, 0x000000, 0x07ffff, roadriot_sloop_data_r);
	sloop_base = install_mem_write16_handler(0, 0x000000, 0x07ffff, roadriot_sloop_data_w);

	asic65_config(ASIC65_STANDARD);
/*
	Road Riot color MUX

	CRA10=!MGEP*!AN.VID7*AN.0				-- if (mopri < pfpri) && (!alpha)
	   +!AN.VID7*AN.0*MO.0					or if (mopix == 0) && (!alpha)

	CRA9=MGEP*!AN.VID7*AN.0*!MO.0			-- if (mopri >= pfpri) && (mopix != 0) && (!alpha)
	   +!AN.VID7*AN.0*PF.VID9				or if (pfpix & 0x200) && (!alpha)

	CRA8=MGEP*!AN.VID7*AN.0*!MO.0*MVID8		-- if (mopri >= pfpri) && (mopix != 0) && (mopix & 0x100) && (!alpha)
	   +!MGEP*!AN.VID7*AN.0*PF.VID8			or if (mopri < pfpri) && (pfpix & 0x100) && (!alpha)
	   +!AN.VID7*AN.0*MO.0*PF.VID8			or if (pfpix & 0x100) && (!alpha)

	CRMUXB=!AN.VID7*AN.0					-- if (!alpha)

	CRMUXA=!MGEP							-- if (mopri < pfpri)
	   +MO.0								or (mopix == 0)
	   +AN.VID7								or (alpha)
	   +!AN.0
*/
}


static DRIVER_INIT( guardian )
{
	static const UINT16 default_eeprom[] =
	{
		0x0001,0x01FD,0x01FF,0x01EF,0x0100,0x01CD,0x0300,0x0104,
		0x0700,0x0117,0x0F00,0x0133,0x1F00,0x0133,0x2400,0x0120,
		0x0600,0x0104,0x0300,0x010C,0x01A0,0x0100,0x0152,0x0179,
		0x012D,0x01BD,0x01FD,0x01FF,0x01EF,0x0100,0x01CD,0x0300,
		0x0104,0x0700,0x0117,0x0F00,0x0133,0x1F00,0x0133,0x2400,
		0x0120,0x0600,0x0104,0x0300,0x010C,0x01A0,0x0100,0x0152,
		0x0179,0x012D,0x01BD,0x8C00,0x0118,0x01AB,0x015A,0x0100,
		0x01D0,0x010B,0x01B8,0x01C7,0x01E2,0x0134,0x0100,0x010A,
		0x01BE,0x016D,0x0142,0x0100,0x0120,0x0109,0x0110,0x0141,
		0x0109,0x0100,0x0108,0x0134,0x0105,0x0148,0x1400,0x0000
	};
	atarigen_eeprom_default = default_eeprom;
	atarijsa_init(1, 3, 2, 0x0040);
	atarijsa3_init_adpcm(REGION_SOUND1);

	atarig42_playfield_base = 0x000;
	atarig42_motion_object_base = 0x400;
	atarig42_motion_object_mask = 0x3ff;

	/* it looks like they jsr to $80000 as some kind of protection */
	/* put an RTS there so we don't die */
	*(data16_t *)&memory_region(REGION_CPU1)[0x80000] = 0x4E75;

	sloop_base = install_mem_read16_handler(0, 0x000000, 0x07ffff, guardians_sloop_data_r);
	sloop_base = install_mem_write16_handler(0, 0x000000, 0x07ffff, guardians_sloop_data_w);

	asic65_config(ASIC65_GUARDIANS);
/*
	Guardians color MUX

	CRA10=MGEP*!AN.VID7*AN.0*!MO.0			-- if (mopri >= pfpri) && (!alpha) && (mopix != 0)

	CRA9=MGEP*!AN.VID7*AN.0*!MO.0*MVID9		-- if (mopri >= pfpri) && (!alpha) && (mopix != 0) && (mopix & 0x200)
	   +!MGEP*!AN.VID7*AN.0*PF.VID9			or if (mopri < pfpri) && (!alpha) && (pfpix & 0x200)
	   +!AN.VID7*AN.0*MO.0*PF.VID9			or if (mopix == 0) && (!alpha) && (pfpix & 0x200)

	CRA8=MGEP*!AN.VID7*AN.0*!MO.0*MVID8		-- if (mopri >= pfpri) && (!alpha) && (mopix != 0) && (mopix & 0x100)
	   +!MGEP*!AN.VID7*AN.0*PF.VID8			or if (mopri < pfpri) && (!alpha) && (pfpix & 0x100)
	   +!AN.VID7*AN.0*MO.0*PF.VID8			or if (mopix == 0) && (!alpha) && (pfpix & 0x100)

	CRMUXB=!AN.VID7*AN.0					-- if (!alpha)

	CRMUXA=!MGEP							-- if (mopri < pfpri)
	   +MO.0								or (mopix == 0)
	   +AN.VID7								or (alpha)
	   +!AN.0
*/
}

static DRIVER_INIT( dangerex )
{
	static const UINT16 default_eeprom[] =
	{
		0x0001,0x01FD,0x01FF,0x01EF,0x0100,0x01CD,0x0300,0x0104,
		0x0700,0x0117,0x0F00,0x0133,0x1F00,0x0133,0x2400,0x0120,
		0x0600,0x0104,0x0300,0x010C,0x01A0,0x0100,0x0152,0x0179,
		0x012D,0x01BD,0x01FD,0x01FF,0x01EF,0x0100,0x01CD,0x0300,
		0x0104,0x0700,0x0117,0x0F00,0x0133,0x1F00,0x0133,0x2400,
		0x0120,0x0600,0x0104,0x0300,0x010C,0x01A0,0x0100,0x0152,
		0x0179,0x012D,0x01BD,0x8C00,0x0118,0x01AB,0x015A,0x0100,
		0x01D0,0x010B,0x01B8,0x01C7,0x01E2,0x0134,0x0100,0x010A,
		0x01BE,0x016D,0x0142,0x0100,0x0120,0x0109,0x0110,0x0141,
		0x0109,0x0100,0x0108,0x0134,0x0105,0x0148,0x1400,0x0000
	};
	atarigen_eeprom_default = default_eeprom;
	atarijsa_init(1, 3, 2, 0x0040);
	atarijsa3_init_adpcm(REGION_SOUND1);

	atarig42_playfield_base = 0x000;
	atarig42_motion_object_base = 0x400;
	atarig42_motion_object_mask = 0x3ff;

	asic65_config(ASIC65_GUARDIANS);
/*
	Danger Express color MUX

	CRA10=MGEP*!AN.VID7*AN.0*!MO.0			-- if (mopri >= pfpri) && (!alpha) && (mopix != 0)

	CRA9=MGEP*!AN.VID7*AN.0*!MO.0*MVID9		-- if (mopri >= pfpri) && (!alpha) && (mopix != 0) && (mopix & 0x200)
	   +!MGEP*!AN.VID7*AN.0*PF.VID9			or if (mopri < pfpri) && (!alpha) && (pfpix & 0x200)
	   +!AN.VID7*AN.0*MO.0*PF.VID9			or if (mopix == 0) && (!alpha) && (pfpix & 0x200)

	CRA8=MGEP*!AN.VID7*AN.0*!MO.0*MVID8		-- if (mopri >= pfpri) && (!alpha) && (mopix != 0) && (mopix & 0x100)
	   +!MGEP*!AN.VID7*AN.0*PF.VID8			or if (mopri < pfpri) && (!alpha) && (pfpix & 0x100)
	   +!AN.VID7*AN.0*MO.0*PF.VID8			or if (mopix == 0) && (!alpha) && (pfpix & 0x100)

	CRMUXB=!AN.VID7*AN.0					-- if (!alpha)

	CRMUXA=!MGEP							-- if (mopri < pfpri)
	   +MO.0								or (mopix == 0)
	   +AN.VID7								or (alpha)
	   +!AN.0
*/
}


/*************************************
 *
 *	Game driver(s)
 *
 *************************************/

GAMEX( 1991, roadriot, 0,        atarig42, roadriot, roadriot, ROT0, "Atari Games", "Road Riot 4WD", GAME_UNEMULATED_PROTECTION )
GAME ( 1992, guardian, 0,        atarig42, guardian, guardian, ROT0, "Atari Games", "Guardians of the Hood" )
GAMEC( 1992, dangerex, 0,        atarig42, dangerex, dangerex, ROT0, "Atari Games", "Danger Express (prototype)", &generic_ctrl, &dangerex_bootstrap )
