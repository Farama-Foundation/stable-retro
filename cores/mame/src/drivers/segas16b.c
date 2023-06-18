#include "driver.h"
#include "system16.h"
#include "cpu/i8039/i8039.h"
#include "vidhrdw/segaic16.h"
#include "machine/fd1089.h"

WRITE_HANDLER( sys16_7751_audio_8255_w );
 READ_HANDLER( sys16_7751_audio_8255_r );
 READ_HANDLER( sys16_7751_sh_rom_r );
 READ_HANDLER( sys16_7751_sh_t1_r );
 READ_HANDLER( sys16_7751_sh_command_r );
WRITE_HANDLER( sys16_7751_sh_dac_w );
WRITE_HANDLER( sys16_7751_sh_busy_w );
WRITE_HANDLER( sys16_7751_sh_offset_a0_a3_w );
WRITE_HANDLER( sys16_7751_sh_offset_a4_a7_w );
WRITE_HANDLER( sys16_7751_sh_offset_a8_a11_w );
WRITE_HANDLER( sys16_7751_sh_rom_select_w );


static UINT8 disable_screen_blanking;
static read16_handler custom_io_r = NULL;
static write16_handler custom_io_w = NULL;

static MEMORY_READ_START( sound_readmem )
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0xe800, 0xe800, soundlatch_r },
	{ 0xf800, 0xffff, MRA_RAM },
MEMORY_END

static MEMORY_WRITE_START( sound_writemem )
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0xf800, 0xffff, MWA_RAM },
MEMORY_END

static PORT_READ_START( sound_readport )
	{ 0x01, 0x01, YM2151_status_port_0_r },
	{ 0xc0, 0xc0, soundlatch_r },
PORT_END

static PORT_WRITE_START( sound_writeport )
	{ 0x00, 0x00, YM2151_register_port_0_w },
	{ 0x01, 0x01, YM2151_data_port_0_w },
PORT_END

/* 7751 Sound */
static MEMORY_READ_START( sound_readmem_7751 )
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0xe800, 0xe800, soundlatch_r },
	{ 0xf800, 0xffff, MRA_RAM },
MEMORY_END

static PORT_READ_START( sound_readport_7751 )
	{ 0x01, 0x01, YM2151_status_port_0_r },
	/*{ 0x0e, 0x0e, sys16_7751_audio_8255_r }, */
	{ 0xc0, 0xc0, soundlatch_r },
PORT_END

static PORT_WRITE_START( sound_writeport_7751 )
	{ 0x00, 0x00, YM2151_register_port_0_w },
	{ 0x01, 0x01, YM2151_data_port_0_w },
	{ 0x80, 0x80, sys16_7751_audio_8255_w },
PORT_END

static MEMORY_READ_START( readmem_7751 )
	{ 0x0000, 0x03ff, MRA_ROM },
MEMORY_END

static MEMORY_WRITE_START( writemem_7751 )
	{ 0x0000, 0x03ff, MWA_ROM },
MEMORY_END

static PORT_READ_START( readport_7751 )
	{ I8039_t1, I8039_t1, sys16_7751_sh_t1_r },
	{ I8039_p2, I8039_p2, sys16_7751_sh_command_r },
	{ I8039_bus, I8039_bus, sys16_7751_sh_rom_r },
PORT_END

static PORT_WRITE_START( writeport_7751 )
	{ I8039_p1, I8039_p1, sys16_7751_sh_dac_w },
	{ I8039_p2, I8039_p2, sys16_7751_sh_busy_w },
	{ I8039_p4, I8039_p4, sys16_7751_sh_offset_a0_a3_w },
	{ I8039_p5, I8039_p5, sys16_7751_sh_offset_a4_a7_w },
	{ I8039_p6, I8039_p6, sys16_7751_sh_offset_a8_a11_w },
	{ I8039_p7, I8039_p7, sys16_7751_sh_rom_select_w },
PORT_END

/* 7759 */
static MEMORY_READ_START( sound_readmem_7759 )
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0xdfff, MRA_BANK1 },
	{ 0xe800, 0xe800, soundlatch_r },
	{ 0xf800, 0xffff, MRA_RAM },
MEMORY_END

static MEMORY_WRITE_START( sound_writemem_7759 )
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0x8000, 0xdfff, MWA_BANK1 },
	{ 0xf800, 0xffff, MWA_RAM },
MEMORY_END

static PORT_WRITE_START( sound_writeport_7759 )
	{ 0x00, 0x00, YM2151_register_port_0_w },
	{ 0x01, 0x01, YM2151_data_port_0_w },
	{ 0x40, 0x40, UPD7759_bank_w },
	{ 0x80, 0x80, upd7759_0_port_w },
PORT_END

WRITE_HANDLER( UPD7759_bank_w )
{
	int bankoffs=0;

	int size = memory_region_length(REGION_CPU2) - 0x10000;
	if (size > 0)
	/* banking depends on the ROM board */
	{
		upd7759_start_w(0, data & 0x80);
		upd7759_reset_w(0, data & 0x40);

		switch (sys16_soundbanktype)
		{
			case 0:

				bankoffs =  (data * 0x4000);
				break;

			case 1:

				/*
				D5 : /CS for ROM at A11
				D4 : /CS for ROM at A10
				D3 : /CS for ROM at A9
				D2 : /CS for ROM at A8
				D1 : A15 for all ROMs (Or ignored for 27256's)
				D0 : A14 for all ROMs
				*/
				if (!(data & 0x04)) bankoffs = 0x00000;
				if (!(data & 0x08)) bankoffs = 0x10000;
				if (!(data & 0x10)) bankoffs = 0x20000;
				if (!(data & 0x20)) bankoffs = 0x30000;
				bankoffs += (data & 0x03) * 0x4000;
				break;


			case 2: /* Cotton Fantasy Zone II etc etc */
				/*
				D5 : Unused
				D4 : Unused
				D3 : ROM select 0=A11, 1=A12
				D2 : A16 for all ROMs
				D1 : A15 for all ROMs
				D0 : A14 for all ROMs
				*/
				bankoffs = ((data & 0x08) >> 3) * 0x20000;
				bankoffs += (data & 0x07) * 0x4000;
				break;

			case 3:
				/*
				D5 : Unused
				D4 : A17 for all ROMs
				D3 : ROM select 0=A11, 1=A12
				D2 : A16 for all ROMs
				D1 : A15 for all ROMs
				D0 : A14 for all ROMs
				*/
				bankoffs = ((data & 0x08) >> 3) * 0x40000;
				bankoffs += ((data & 0x10) >> 4) * 0x20000;
				bankoffs += (data & 0x07) * 0x04000;
				break;
		}
		cpu_setbank(1, memory_region(REGION_CPU2) + 0x10000 + (bankoffs % size));
	}
}

static void sound_cause_a_nmi( int state )
{
	if (state) /* upd7759 callback */
		cpu_set_nmi_line(1, PULSE_LINE);
}

static WRITE16_HANDLER( sound_command_w )
{
	soundlatch_w( 0,data&0xff );
	cpu_set_irq_line( 1, 0, HOLD_LINE );
}

struct upd7759_interface sys16b_upd7759_interface =
{
	1,							/* number of chips */
	{ UPD7759_STANDARD_CLOCK }, /* clock */
	{ 60  },					/* volume */
	{ 0 },						/* memory regions */
	{ sound_cause_a_nmi }		/* drq callback (per chip, slave mode only) */
};

static READ16_HANDLER( standard_io_r )
{
	offset &= 0x1fff;
	switch (offset & (0x3000/2))
	{
		case 0x1000/2:
			return readinputport(offset & 3);

		case 0x2000/2:
			return  ((offset & 1) ? readinputport(4)  : readinputport(5));
	}
	logerror("CPU #0 PC %06x: standard_io_r - unknown read access to address %06x\n", activecpu_get_pc(), offset);
	return 0;
}

static WRITE16_HANDLER( standard_io_w )
{
	offset &= 0x1fff;
	switch (offset & (0x3000/2))
	{
		case 0x0000/2:
			/*
			D7 : 1 for most games, 0 for ddux, sdi, wb3
			D6 : 1= Screen flip, 0= Normal screen display
			D5 : 1= Display on, 0= Display off
			D4 : 0 for most games, 1 for eswat
			D3 : Output to lamp 2 (1= On, 0= Off)
			D2 : Output to lamp 1 (1= On, 0= Off)
			D1 : (Output to coin counter 2?)
			D0 : Output to coin counter 1
			 */
		segaic16_tilemap_set_flip(0, data & 0x40);
		segaic16_sprites_set_flip(0, data & 0x40);
		if (!disable_screen_blanking)
			segaic16_set_display_enable(data & 0x20);
		set_led_status(1, data & 0x08);
		set_led_status(0, data & 0x04);
		coin_counter_w(1, data & 0x02);
		coin_counter_w(0, data & 0x01);
		return;
	}
	logerror("%06X:standard_io_w - unknown write access to address %04X = %04X & %04X\n", activecpu_get_pc(), offset * 2, data, mem_mask ^ 0xffff);
}

static READ16_HANDLER( misc_io_r )
{
	if (custom_io_r)
		return (*custom_io_r)(offset, mem_mask);

	else
		return standard_io_r(offset, mem_mask);
}

static WRITE16_HANDLER( misc_io_w )
{
	if (custom_io_w)
		(*custom_io_w)(offset, data, mem_mask);
	else
		standard_io_w(offset, data, mem_mask);
}

static READ16_HANDLER( dunkshot_custom_io_r )
{

	switch (offset & (0x3000/2))
	{
		case 0x3000/2:


			switch ((offset/2) & 7)
			{
				case 0:	return (readinputport(6) << 4) >> (8 * (offset & 1));
				case 1:	return (readinputport(7) << 4) >> (8 * (offset & 1));
				case 2:	return (readinputport(8) << 4) >> (8 * (offset & 1));
				case 3:	return (readinputport(9) << 4) >> (8 * (offset & 1));
				case 4:	return (readinputport(10) << 4) >> (8 * (offset & 1));
				case 5:	return (readinputport(11) << 4) >> (8 * (offset & 1));
				case 6:	return (readinputport(12) << 4) >> (8 * (offset & 1));
				case 7:	return (readinputport(13) << 4) >> (8 * (offset & 1));
			}
			break;
	}
	return standard_io_r(offset, mem_mask);
}

/*************************************
 *
 *	SDI custom I/O
 *
 *************************************/

static READ16_HANDLER( sdi_custom_io_r )
{
	switch (offset & (0x3000/2))
	{

		case 0x3000/2:
			switch ((offset/2) & 3)
			{

				case 0:	return readinputport(6);
				case 1:	return readinputport(7);
				case 2:	return readinputport(8);
				case 3:	return readinputport(9);
			}
			break;
	}
	return standard_io_r(offset, mem_mask);
}



/*************************************
 *
 *	Sukeban Jansi Ryuko custom I/O
 *
 *************************************/

static READ16_HANDLER( sjryuko_custom_io_r )
{
	static const char *portname[] = { "MJ0", "MJ1", "MJ2", "MJ3", "MJ4", "MJ5" };

	switch (offset & (0x3000/2))
	{
		case 0x1000/2:
			switch (offset & 3)
			{
	/*
				case 1:
					if (readinputportbytag_safe(portname[mj_input_num], 0xff) != 0xff)
						return 0xff & ~(1 << mj_input_num);
					return 0xff;

				case 2:
					return readinputportbytag_safe(portname[mj_input_num], 0xff);
	*/
			}
			break;
	}
	return standard_io_r(offset, mem_mask);
}


static WRITE16_HANDLER( sjryuko_custom_io_w )
{
	static UINT8 last_val;

	switch (offset & (0x3000/2))
	{
	/*
		case 0x0000/2:
			if (((last_val ^ data) & 4) && (data & 4))
				mj_input_num = (mj_input_num + 1) % 6;
			break;
	*/
	}
	standard_io_w(offset, data, mem_mask);
}


READ16_HANDLER( segaic16_textram_r ){
	return segaic16_textram_0[offset];
}

READ16_HANDLER( segaic16_tileram_r ){
	return segaic16_tileram_0[offset];
}

READ16_HANDLER( segaic16_spriteram_r ){
	return segaic16_spriteram_0[offset];
}

static WRITE16_HANDLER( rom_5704_bank_w )
{
	if (ACCESSING_LSB)
		segaic16_tilemap_set_bank(0, offset & 1, data & 7);
}


INPUT_PORTS_START( aurail )
	SYS16_SERVICE
	SYS16_JOY1
	SYS16_UNUSED
	SYS16_JOY2

	SYS16_COINAGE/*DSW1 */
	PORT_START	/* DSW2 */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "2" )
	PORT_DIPSETTING(    0x0c, "3" )
	PORT_DIPSETTING(    0x08, "4" )
	PORT_DIPSETTING(    0x04, "5" )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x10, "Normal" )
	PORT_DIPSETTING(    0x00, "Hard" )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x20, "Normal" )
	PORT_DIPSETTING(    0x00, "Hard" )
	PORT_DIPNAME( 0x40, 0x40, "Controller select" )
	PORT_DIPSETTING(    0x40, "1 Player side" )
	PORT_DIPSETTING(    0x00, "2 Players side" )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
INPUT_PORTS_END

INPUT_PORTS_START( bullet )
	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_SERVICE_NO_TOGGLE( 0x04, IP_ACTIVE_LOW )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START3 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN3 )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_DOWN | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_UP | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_LEFT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICKRIGHT_DOWN | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICKRIGHT_UP | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICKRIGHT_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICKRIGHT_LEFT | IPF_8WAY | IPF_PLAYER1 )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_DOWN | IPF_8WAY | IPF_PLAYER3 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_UP | IPF_8WAY | IPF_PLAYER3 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_RIGHT | IPF_8WAY | IPF_PLAYER3 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_LEFT | IPF_8WAY | IPF_PLAYER3 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICKRIGHT_DOWN | IPF_8WAY | IPF_PLAYER3 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICKRIGHT_UP | IPF_8WAY | IPF_PLAYER3 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICKRIGHT_RIGHT | IPF_8WAY | IPF_PLAYER3 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICKRIGHT_LEFT | IPF_8WAY | IPF_PLAYER3 )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_DOWN | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_UP | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_LEFT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICKRIGHT_DOWN | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICKRIGHT_UP | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICKRIGHT_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICKRIGHT_LEFT | IPF_8WAY | IPF_PLAYER2 )


	SYS16_COINAGE /*DSW1 */

	PORT_START/*DSW2 */
	PORT_DIPNAME( 0x01, 0x01, "Players" )
	PORT_DIPSETTING(    0x01, "2" )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "2" )
	PORT_DIPSETTING(    0x0c, "3" )
	PORT_DIPSETTING(    0x08, "4" )
	PORT_DIPSETTING(    0x04, "5" )
INPUT_PORTS_END

INPUT_PORTS_START( cotton )

	SYS16_SERVICE
	SYS16_JOY1
	SYS16_UNUSED
	SYS16_JOY2

	SYS16_COINAGE /*DSW1*/
	PORT_START /*DSW2*/
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x06, 0x06, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x04, "2" )
	PORT_DIPSETTING(    0x06, "3" )
	PORT_DIPSETTING(    0x02, "4" )
	PORT_DIPSETTING(    0x00, "5" )
	PORT_DIPNAME( 0x18, 0x18, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x10, "Easy" )
	PORT_DIPSETTING(    0x18, "Normal" )
	PORT_DIPSETTING(    0x08, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
	/*"SW2:6" unused */
	/*"SW2:7" unused */
	/*"SW2:8" unused */
INPUT_PORTS_END


INPUT_PORTS_START( dunkshot )
PORT_START /*SERVICE*/
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 ) \
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 ) \
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_SERVICE, DEF_STR( Service_Mode ), KEYCODE_F2, IP_JOY_NONE ) \
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_SERVICE1 ) \
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START1 ) \
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START2 ) \
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START3 ) \
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START4 )

PORT_START/*JOY1*/
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER3 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER3 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER4 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER4 )

	SYS16_UNUSED

	SYS16_UNUSED /*JOY2*/

	SYS16_COINAGE /*DSW1*/

PORT_START/*DSW2*/
	PORT_DIPNAME( 0x01, 0x00, "Winner Advances" )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x0c, 0x0c, "VS Time" )
	PORT_DIPSETTING(    0x08, "2P 1:30/ 3P 2:00/ 4P 2:30" )
	PORT_DIPSETTING(    0x0c, "2P 2:00/ 3P 2:30/ 4P 3:00" )
	PORT_DIPSETTING(    0x04, "2P 2:30/ 3P 3:00/ 4P 3:30" )
	PORT_DIPSETTING(    0x00, "2P 3:00/ 3P 3:30/ 4P 4:00" )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Easy ) )
	PORT_DIPSETTING(    0x30, DEF_STR( Normal ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Hard ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Hardest ) )
	PORT_DIPNAME( 0x40, 0x40, "CPU Starts With +6 Pts." )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Allow_Continue ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )


	PORT_START				/* fake analog X */
	PORT_ANALOG( 0xff, 0x80, IPT_TRACKBALL_X | IPF_REVERSE, 75, 5, 0, 255 )

	PORT_START				/* fake analog Y */
	PORT_ANALOG( 0xff, 0x80, IPT_TRACKBALL_Y, 75, 5, 0, 255 )

	PORT_START				/* fake analog X */
	PORT_ANALOG( 0xff, 0x80, IPT_TRACKBALL_X | IPF_PLAYER2, 75, 5, 0, 255 )

	PORT_START				/* fake analog Y */
	PORT_ANALOG( 0xff, 0x80, IPT_TRACKBALL_Y | IPF_PLAYER2 | IPF_REVERSE, 75, 5, 0, 255 )

	PORT_START				/* fake analog X */
	PORT_ANALOG( 0xff, 0x80, IPT_TRACKBALL_X | IPF_PLAYER3 | IPF_REVERSE, 75, 5, 0, 255 )

	PORT_START				/* fake analog Y */
	PORT_ANALOG( 0xff, 0x80, IPT_TRACKBALL_Y | IPF_PLAYER3, 75, 5, 0, 255 )

	PORT_START				/* fake analog X */
	PORT_ANALOG( 0xff, 0x80, IPT_TRACKBALL_X | IPF_PLAYER4, 75, 5, 0, 255 )

	PORT_START				/* fake analog Y */
	PORT_ANALOG( 0xff, 0x80, IPT_TRACKBALL_Y | IPF_PLAYER4 | IPF_REVERSE, 75, 5, 0, 255 )
INPUT_PORTS_END


INPUT_PORTS_START( fantzn2x )
	SYS16_SERVICE
	SYS16_JOY1
	SYS16_UNUSED
	SYS16_JOY2

	SYS16_COINAGE /*DSW1 */
	PORT_START /*DSW2 */
		PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x08, "2" )
	PORT_DIPSETTING(    0x0c, "3" )
	PORT_DIPSETTING(    0x04, "4" )
	PORT_DIPSETTING(    0x00, "240 (Cheat)")
	PORT_BIT( 0x010, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x020, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_DIPNAME( 0xc0, 0xc0, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x80, "Easy" )
	PORT_DIPSETTING(    0xc0, "Normal" )
	PORT_DIPSETTING(    0x40, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
INPUT_PORTS_END

INPUT_PORTS_START( sdi )
PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 ) \
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 ) \
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_SERVICE, DEF_STR( Service_Mode ), KEYCODE_F2, IP_JOY_NONE ) \
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_SERVICE1 ) \
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START1 ) \
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START2 ) \
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON1 ) \
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )

	SYS16_UNUSED

PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_DOWN | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_UP | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_RIGHT | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_LEFT | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_DOWN | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_UP | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_LEFT | IPF_8WAY | IPF_PLAYER2 )

	SYS16_UNUSED

	SYS16_COINAGE

PORT_START
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Allow_Continue ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x08, "2" )
	PORT_DIPSETTING(    0x0c, "3" )
	PORT_DIPSETTING(    0x04, "4" )
	PORT_DIPSETTING(    0x00, "Free")
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Easy ) )
	PORT_DIPSETTING(    0x30, DEF_STR( Normal ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Hard ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Hardest ) )
	PORT_DIPNAME( 0xc0, 0xc0, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x80, "Every 50000" )
	PORT_DIPSETTING(    0xc0, "50000" )
	PORT_DIPSETTING(    0x40, "100000" )
	PORT_DIPSETTING(    0x00, DEF_STR( None ) )

	PORT_START				/* fake analog X */
	PORT_ANALOG( 0xff, 0x80, IPT_TRACKBALL_X | IPF_REVERSE, 75, 5, 0, 255 )

	PORT_START				/* fake analog Y */
	PORT_ANALOG( 0xff, 0x80, IPT_TRACKBALL_Y, 75, 5, 0, 255 )

	PORT_START				/* fake analog X */
	PORT_ANALOG( 0xff, 0x80, IPT_TRACKBALL_X | IPF_PLAYER2, 75, 5, 0, 255 )

	PORT_START				/* fake analog Y */
	PORT_ANALOG( 0xff, 0x80, IPT_TRACKBALL_Y | IPF_PLAYER2, 75, 5, 0, 255 )
INPUT_PORTS_END


static MEMORY_READ16_START( aurail_readmem )
	{ 0x000000, 0x0bffff, MRA16_ROM },
	{ 0x400000, 0x40ffff, segaic16_tileram_r },
	{ 0x410000, 0x410fff, segaic16_textram_r },
	{ 0x440000, 0x4407ff, segaic16_spriteram_r },
	{ 0x840000, 0x840fff, SYS16_MRA16_PALETTERAM },
	{ 0xc40000, 0xc43FFF, misc_io_r },
	{ 0xfc0000, 0xfcffff, MRA16_RAM},
	{ 0xff0000, 0xff3fff, SYS16_MRA16_WORKINGRAM },
	{ 0xff4000, 0xffffff, MRA16_RAM },
MEMORY_END

static MEMORY_WRITE16_START( aurail_writemem )
	{ 0x000000, 0x0bffff, MWA16_ROM },
	{ 0x400000, 0x40ffff, segaic16_tileram_0_w, &segaic16_tileram_0 },
	{ 0x410000, 0x410fff, segaic16_textram_0_w, &segaic16_textram_0 },
	{ 0x440000, 0x4407ff, SYS16_MWA16_SPRITERAM, &segaic16_spriteram_0 },
	{ 0x840000, 0x840fff, segaic16_paletteram_w, &paletteram16 },
	{ 0xc40000, 0xc43FFF, misc_io_w },
	{ 0xfc0000, 0xfcffff, rom_5704_bank_w },
	{ 0xfe0006, 0xfe0007, sound_command_w },
	{ 0xff0000, 0xff3fff, SYS16_MWA16_WORKINGRAM, &sys16_workingram },
	{ 0xff4000, 0xffffff, MWA16_RAM },
MEMORY_END

static MEMORY_READ16_START( bullet_readmem )
	{ 0x000000, 0x02ffff, MRA16_ROM },
	{ 0x400000, 0x40ffff, segaic16_tileram_r },
	{ 0x410000, 0x410fff, segaic16_textram_r },
	{ 0x440000, 0x4407ff, segaic16_spriteram_r },
	{ 0x840000, 0x840fff, SYS16_MRA16_PALETTERAM },
	{ 0xc40000, 0xc43fff, misc_io_r },
	{ 0xff0000, 0xff3fff, SYS16_MRA16_WORKINGRAM },
	{ 0xff4000, 0xffffff, MRA16_RAM },
MEMORY_END

static MEMORY_WRITE16_START( bullet_writemem )
	{ 0x000000, 0x02ffff, MWA16_ROM },
	{ 0x123406, 0x123407, sound_command_w }, /*sdib,defense */
	{ 0x400000, 0x40ffff, segaic16_tileram_0_w, &segaic16_tileram_0 },
	{ 0x410000, 0x410fff, segaic16_textram_0_w, &segaic16_textram_0 },
	{ 0x440000, 0x4407ff, SYS16_MWA16_SPRITERAM, &segaic16_spriteram_0 },
	{ 0x840000, 0x840fff, segaic16_paletteram_w, &paletteram16 },
	{ 0xc00006, 0xc00007, sound_command_w }, /*bullet */
	{ 0xc40000, 0xc43fff, misc_io_w },
	{ 0xff0000, 0xff3fff, SYS16_MWA16_WORKINGRAM, &sys16_workingram },
	{ 0xff4000, 0xffffff, MWA16_RAM },
MEMORY_END

static MEMORY_READ16_START( cotton_readmem )
	{ 0x000000, 0x0bffff, MRA16_ROM },
	{ 0x100000, 0x10FFFF, MRA16_RAM }, /*rom_5704_bank */
	{ 0x200000, 0x203FFF, SYS16_MRA16_WORKINGRAM }, /*workingram */
	{ 0x300000, 0x3007FF, segaic16_spriteram_r },
	{ 0x400000, 0x40FFFF, segaic16_tileram_r },
	{ 0x410000, 0x410FFF, segaic16_textram_r },
	{ 0x500000, 0x500FFF, SYS16_MRA16_PALETTERAM },
	{ 0x600000, 0x603FFF, misc_io_r },
MEMORY_END

static MEMORY_WRITE16_START( cotton_writemem )
	{ 0x000000, 0x0bffff, MWA16_ROM },
	{ 0x100000, 0x10FFFF, rom_5704_bank_w }, /*rom_5704_bank */
	{ 0x200000, 0x203FFF, SYS16_MWA16_WORKINGRAM, &sys16_workingram }, /*workingram */
	{ 0x300000, 0x3007FF, SYS16_MWA16_SPRITERAM, &segaic16_spriteram_0 },
	{ 0x400000, 0x40FFFF, segaic16_tileram_0_w, &segaic16_tileram_0 },
	{ 0x410000, 0x410FFF, segaic16_textram_0_w, &segaic16_textram_0 },
	{ 0x500000, 0x500FFF, segaic16_paletteram_w, &paletteram16 },
	{ 0x600000, 0x603FFF, misc_io_w  },
	{ 0xFF0006, 0xFF0007, sound_command_w },
MEMORY_END

static MEMORY_READ16_START( dunkshot_readmem )
	{ 0x000000, 0x02ffff, MRA16_ROM },
	{ 0x400000, 0x40ffff, segaic16_tileram_r },
	{ 0x410000, 0x410fff, segaic16_textram_r },
	{ 0x440000, 0x4407ff, segaic16_spriteram_r },
	{ 0x840000, 0x840fff, SYS16_MRA16_PALETTERAM },
	{ 0xc40000, 0xc43fff, misc_io_r },
	{ 0xff0000, 0xff3fff, SYS16_MRA16_WORKINGRAM },
	{ 0xff4000, 0xffffff, MRA16_RAM },
MEMORY_END

static MEMORY_WRITE16_START( dunkshot_writemem )
	{ 0x000000, 0x02ffff, MWA16_ROM },
	{ 0x400000, 0x40ffff, segaic16_tileram_0_w, &segaic16_tileram_0 },
	{ 0x410000, 0x410fff, segaic16_textram_0_w, &segaic16_textram_0 },
	{ 0x440000, 0x4407ff, SYS16_MWA16_SPRITERAM, &segaic16_spriteram_0 },
	{ 0x840000, 0x840fff, segaic16_paletteram_w, &paletteram16 },
	{ 0xFE0006, 0xFE0007, sound_command_w },
	{ 0xc40000, 0xc43fff, misc_io_w },
	{ 0xff0000, 0xff3fff, SYS16_MWA16_WORKINGRAM, &sys16_workingram },
	{ 0xff4000, 0xffffff, MWA16_RAM },
MEMORY_END


static MEMORY_READ16_START( fantzn2x_readmem )
	{ 0x000000, 0x0bffff, MRA16_ROM },
	{ 0x200000, 0x23ffff, SYS16_MRA16_WORKINGRAM },
	{ 0x400000, 0x40ffff, segaic16_tileram_r },
	{ 0x410000, 0x410fff, segaic16_textram_r },
	{ 0x440000, 0x440fff, segaic16_spriteram_r },
	{ 0x840000, 0x840fff, SYS16_MRA16_PALETTERAM },
	{ 0xc40000, 0xc43FFF, misc_io_r }, MEMORY_END

static MEMORY_WRITE16_START( fantzn2x_writemem )
	{ 0x000000, 0x0bffff, MWA16_ROM },
	{ 0x200000, 0x23ffff, SYS16_MWA16_WORKINGRAM, &sys16_workingram},
	{ 0x3f0000, 0x3fffff, rom_5704_bank_w }, /*rom_5704_bank */
	{ 0x400000, 0x40ffff, segaic16_tileram_0_w, &segaic16_tileram_0 },
	{ 0x410000, 0x410fff, segaic16_textram_0_w, &segaic16_textram_0 },
	{ 0x440000, 0x440fff, SYS16_MWA16_SPRITERAM, &segaic16_spriteram_0 },
	{ 0x840000, 0x840fff, segaic16_paletteram_w, &paletteram16 },
	{ 0xc40000, 0xc43FFF, misc_io_w },
	{ 0xfe0006, 0xfe0007, sound_command_w },
MEMORY_END

/*************************************************************************************************************************
/  Aurail, Sega System 16B
/  CPU: 68000
/  ROM Board type: 171-5704
/  Sega ID# for ROM board: 834-7702-06 AURAIL(ROM BD)
*/

ROM_START( aurail )
	ROM_REGION( 0xc0000, REGION_CPU1, 0 ) /* 68000 code */
	ROM_LOAD16_BYTE( "epr-13577.a7", 0x000000, 0x20000, CRC(6701b686) SHA1(ce1e99a516b34241bfe2fbe42d105970ad6e6ddb) )
	ROM_LOAD16_BYTE( "epr-13576.a5", 0x000001, 0x20000, CRC(1e428d94) SHA1(53c0c2d946822157974c8505fd301f8001fc3401) )
	ROM_LOAD16_BYTE( "epr-13447.a8", 0x080000, 0x20000, CRC(70a52167) SHA1(51512d4ee1e63902375b197cf04170744b099d88) )
	ROM_LOAD16_BYTE( "epr-13445.a6", 0x080001, 0x20000, CRC(28dfc3dd) SHA1(b1d6d3e31a48062a91cc9b7b6ff68bfde0a3ea1c) )

	ROM_REGION( 0xc0000, REGION_GFX1, 0 ) /* tiles */
	ROM_LOAD( "mpr-13450.a14", 0x00000, 0x20000, CRC(0fc4a7a8) SHA1(b46b22a6b0ead19551e67abfb3304c52b02b8d29) ) /* plane 1 */
	ROM_LOAD( "mpr-13465.b14", 0x20000, 0x20000, CRC(e08135e0) SHA1(cd748f4a8f67f562174fa9c6edb966c77b488e75) )
	ROM_LOAD( "mpr-13451.a15", 0x40000, 0x20000, CRC(1c49852f) SHA1(d08d6991c203400f685fada2445a22a7eceeee04) ) /* plane 2 */
	ROM_LOAD( "mpr-13466.b15", 0x60000, 0x20000, CRC(e14c6684) SHA1(f9b0d45e01a6df2b3875b493db9fa41bf37d79f0) )
	ROM_LOAD( "mpr-13452.a16", 0x80000, 0x20000, CRC(047bde5e) SHA1(e759baedcbb637a6c14af461b8a492554cadc9e4) ) /* plane 3 */
	ROM_LOAD( "mpr-13467.b16", 0xa0000, 0x20000, CRC(6309fec4) SHA1(f90c9679bade3cfbaa7949e412410c29d5bfa4d3) )

	ROM_REGION16_BE( 0x200000, REGION_GFX2, 0 ) /* sprites */
	ROM_LOAD16_BYTE( "mpr-13453.b1",  0x000001, 0x020000, CRC(5fa0a9f8) SHA1(d9d67cc006a608d48e37aa89359f1a9403172b00) )
	ROM_LOAD16_BYTE( "mpr-13457.b5",  0x000000, 0x020000, CRC(0d1b54da) SHA1(c7a6393f9e13adabe93b7a86aa7845c5f4d188f1) )
	ROM_LOAD16_BYTE( "mpr-13454.b2",  0x040001, 0x020000, CRC(5f6b33b1) SHA1(5d397bdaa2c7a9ce82cc9134bfeb78418dc613b0) )
	ROM_LOAD16_BYTE( "mpr-13458.b6",  0x040000, 0x020000, CRC(bad340c3) SHA1(e04ef028e44054d34831d3617c5a6348823cfebd) )
	ROM_LOAD16_BYTE( "mpr-13455.b3",  0x080001, 0x020000, CRC(4e80520b) SHA1(8147793ee0023ac18f633d756ecc53aef1980e95) )
	ROM_LOAD16_BYTE( "mpr-13459.b7",  0x080000, 0x020000, CRC(7e9165ac) SHA1(32114095f96fb0ae385c9bf31dd97a99ef182aaa) )
	ROM_LOAD16_BYTE( "mpr-13456.b4",  0x0c0001, 0x020000, CRC(5733c428) SHA1(865203ebee9d98e67324c46915d8835d0289ee0c) )
	ROM_LOAD16_BYTE( "mpr-13460.b8",  0x0c0000, 0x020000, CRC(66b8f9b3) SHA1(4bf003a9bed40673d88df51a314eb6bd10f0d039) )
	ROM_LOAD16_BYTE( "mpr-13440.a1",  0x100001, 0x020000, CRC(4f370b2b) SHA1(66beb4264d569520d80f8915e1c1fddbf61efb21) )
	ROM_LOAD16_BYTE( "mpr-13461.b10", 0x100000, 0x020000, CRC(f76014bf) SHA1(b44f0b6fb9dee927d81c62282aa946d8c35766ca) )
	ROM_LOAD16_BYTE( "mpr-13441.a2",  0x140001, 0x020000, CRC(37cf9cb4) SHA1(f51cee874ad8a824462d2475a23e7016ef64c6b4) )
	ROM_LOAD16_BYTE( "mpr-13462.b11", 0x140000, 0x020000, CRC(1061e7da) SHA1(bad3560f1ed6a5a79f4ecf85d3843b24ecf22d19) )
	ROM_LOAD16_BYTE( "mpr-13442.a3",  0x180001, 0x020000, CRC(049698ef) SHA1(dca1a78f0156cfac9acbfb6e47eb3897b579d2ec) )
	ROM_LOAD16_BYTE( "mpr-13463.b12", 0x180000, 0x020000, CRC(7dbcfbf1) SHA1(0b7be1de57f83b1213805489c6ebfc0f1e5fb4b0) )
	ROM_LOAD16_BYTE( "mpr-13443.a4",  0x1c0001, 0x020000, CRC(77a8989e) SHA1(0ad0877a9814fb7c2fb79062a50b1f9ce9420768) )
	ROM_LOAD16_BYTE( "mpr-13464.b13", 0x1c0000, 0x020000, CRC(551df422) SHA1(cf4cd2b66335853c7c6cce949e79c05e93a39666) )

	ROM_REGION( 0x50000, REGION_CPU2, 0 ) /* sound CPU */
	ROM_LOAD( "epr-13448.a10", 0x00000, 0x08000, CRC(b5183fb9) SHA1(c8372b57fa486256d49dc5851d6b17c92de593fb) )
	ROM_LOAD( "mpr-13449.a11", 0x10000, 0x20000, CRC(d3d9aaf9) SHA1(0fb3a8cb11033accceb3a43a691fb424cf8b9619) )

	ROM_REGION( 0x0100, REGION_USER1, 0 )
	ROM_LOAD( "315-5298.b9",  0x0000, 0x00eb, CRC(39b47212) SHA1(432b47aee5ecbf08a8a6dc2f8379c816feb86328) ) /* PLS153 */
ROM_END

/*************************************************************************************************************************
  Aurail, Sega System 16B
  CPU: FD1089B (317-0168)
  ROM Board type: 171-5704
*/

ROM_START( aurail1 )
	ROM_REGION( 0xc0000, REGION_CPU1, 0 ) /* 68000 code */
	ROM_LOAD16_BYTE( "epr-13469.a7", 0x000000, 0x20000, CRC(c628b69d) SHA1(839cefe5ab1c58fb3e6af8cba245194d5d672223) )
	ROM_LOAD16_BYTE( "epr-13468.a5", 0x000001, 0x20000, CRC(ce092218) SHA1(c93450234bc37060bc4b8bca34ea69154d99be6c) )
	ROM_LOAD16_BYTE( "epr-13447.a8", 0x080000, 0x20000, CRC(70a52167) SHA1(51512d4ee1e63902375b197cf04170744b099d88) )
	ROM_LOAD16_BYTE( "epr-13445.a6", 0x080001, 0x20000, CRC(28dfc3dd) SHA1(b1d6d3e31a48062a91cc9b7b6ff68bfde0a3ea1c) )

	ROM_REGION( 0xc0000, REGION_GFX1, 0 ) /* tiles */
	ROM_LOAD( "mpr-13450.a14", 0x00000, 0x20000, CRC(0fc4a7a8) SHA1(b46b22a6b0ead19551e67abfb3304c52b02b8d29) ) /* plane 1 */
	ROM_LOAD( "mpr-13465.b14", 0x20000, 0x20000, CRC(e08135e0) SHA1(cd748f4a8f67f562174fa9c6edb966c77b488e75) )
	ROM_LOAD( "mpr-13451.a15", 0x40000, 0x20000, CRC(1c49852f) SHA1(d08d6991c203400f685fada2445a22a7eceeee04) ) /* plane 2 */
	ROM_LOAD( "mpr-13466.b15", 0x60000, 0x20000, CRC(e14c6684) SHA1(f9b0d45e01a6df2b3875b493db9fa41bf37d79f0) )
	ROM_LOAD( "mpr-13452.a16", 0x80000, 0x20000, CRC(047bde5e) SHA1(e759baedcbb637a6c14af461b8a492554cadc9e4) ) /* plane 3 */
	ROM_LOAD( "mpr-13467.b16", 0xa0000, 0x20000, CRC(6309fec4) SHA1(f90c9679bade3cfbaa7949e412410c29d5bfa4d3) )

	ROM_REGION16_BE( 0x200000, REGION_GFX2, 0 ) /* sprites */
	ROM_LOAD16_BYTE( "mpr-13453.b1",  0x000001, 0x020000, CRC(5fa0a9f8) SHA1(d9d67cc006a608d48e37aa89359f1a9403172b00) )
	ROM_LOAD16_BYTE( "mpr-13457.b5",  0x000000, 0x020000, CRC(0d1b54da) SHA1(c7a6393f9e13adabe93b7a86aa7845c5f4d188f1) )
	ROM_LOAD16_BYTE( "mpr-13454.b2",  0x040001, 0x020000, CRC(5f6b33b1) SHA1(5d397bdaa2c7a9ce82cc9134bfeb78418dc613b0) )
	ROM_LOAD16_BYTE( "mpr-13458.b6",  0x040000, 0x020000, CRC(bad340c3) SHA1(e04ef028e44054d34831d3617c5a6348823cfebd) )
	ROM_LOAD16_BYTE( "mpr-13455.b3",  0x080001, 0x020000, CRC(4e80520b) SHA1(8147793ee0023ac18f633d756ecc53aef1980e95) )
	ROM_LOAD16_BYTE( "mpr-13459.b7",  0x080000, 0x020000, CRC(7e9165ac) SHA1(32114095f96fb0ae385c9bf31dd97a99ef182aaa) )
	ROM_LOAD16_BYTE( "mpr-13456.b4",  0x0c0001, 0x020000, CRC(5733c428) SHA1(865203ebee9d98e67324c46915d8835d0289ee0c) )
	ROM_LOAD16_BYTE( "mpr-13460.b8",  0x0c0000, 0x020000, CRC(66b8f9b3) SHA1(4bf003a9bed40673d88df51a314eb6bd10f0d039) )
	ROM_LOAD16_BYTE( "mpr-13440.a1",  0x100001, 0x020000, CRC(4f370b2b) SHA1(66beb4264d569520d80f8915e1c1fddbf61efb21) )
	ROM_LOAD16_BYTE( "mpr-13461.b10", 0x100000, 0x020000, CRC(f76014bf) SHA1(b44f0b6fb9dee927d81c62282aa946d8c35766ca) )
	ROM_LOAD16_BYTE( "mpr-13441.a2",  0x140001, 0x020000, CRC(37cf9cb4) SHA1(f51cee874ad8a824462d2475a23e7016ef64c6b4) )
	ROM_LOAD16_BYTE( "mpr-13462.b11", 0x140000, 0x020000, CRC(1061e7da) SHA1(bad3560f1ed6a5a79f4ecf85d3843b24ecf22d19) )
	ROM_LOAD16_BYTE( "mpr-13442.a3",  0x180001, 0x020000, CRC(049698ef) SHA1(dca1a78f0156cfac9acbfb6e47eb3897b579d2ec) )
	ROM_LOAD16_BYTE( "mpr-13463.b12", 0x180000, 0x020000, CRC(7dbcfbf1) SHA1(0b7be1de57f83b1213805489c6ebfc0f1e5fb4b0) )
	ROM_LOAD16_BYTE( "mpr-13443.a4",  0x1c0001, 0x020000, CRC(77a8989e) SHA1(0ad0877a9814fb7c2fb79062a50b1f9ce9420768) )
	ROM_LOAD16_BYTE( "mpr-13464.b13", 0x1c0000, 0x020000, CRC(551df422) SHA1(cf4cd2b66335853c7c6cce949e79c05e93a39666) )

	ROM_REGION( 0x50000, REGION_CPU2, 0 ) /* sound CPU */
	ROM_LOAD( "epr-13448.a10", 0x00000, 0x08000, CRC(b5183fb9) SHA1(c8372b57fa486256d49dc5851d6b17c92de593fb) )
	ROM_LOAD( "mpr-13449.a11", 0x10000, 0x20000, CRC(d3d9aaf9) SHA1(0fb3a8cb11033accceb3a43a691fb424cf8b9619) )
	ROM_RELOAD(                0x30000, 0x20000 )

	ROM_REGION( 0x2000, REGION_USER2, 0 ) /* decryption key */
	ROM_LOAD( "317-0168.key", 0x0000, 0x2000, CRC(fed38390) SHA1(b5f458bc70c069542be16d476645c153ed1d1b45) )

	ROM_REGION( 0x0100, REGION_USER1, 0 )
	ROM_LOAD( "315-5298.b9",  0x0000, 0x00eb, CRC(39b47212) SHA1(432b47aee5ecbf08a8a6dc2f8379c816feb86328) ) /* PLS153 */
ROM_END

ROM_START( aurail1d )
	ROM_REGION( 0xc0000, REGION_CPU1, 0 ) /* 68000 code */
	ROM_LOAD16_BYTE( "bootleg_epr-13469.a7", 0x000000, 0x20000, CRC(75ef3eec) SHA1(40749dd33d6218c116de3555eae4667c10c50a6e) )
	ROM_LOAD16_BYTE( "bootleg_epr-13468.a5", 0x000001, 0x20000, CRC(e46e4f55) SHA1(4be6a261f19ac0e14ca0b66c10f8af94bba27e63) )
	ROM_LOAD16_BYTE( "epr-13447.a8", 0x080000, 0x20000, CRC(70a52167) SHA1(51512d4ee1e63902375b197cf04170744b099d88) )
	ROM_LOAD16_BYTE( "epr-13445.a6", 0x080001, 0x20000, CRC(28dfc3dd) SHA1(b1d6d3e31a48062a91cc9b7b6ff68bfde0a3ea1c) )

	ROM_REGION( 0xc0000, REGION_GFX1, 0 ) /* tiles */
	ROM_LOAD( "mpr-13450.a14", 0x00000, 0x20000, CRC(0fc4a7a8) SHA1(b46b22a6b0ead19551e67abfb3304c52b02b8d29) ) /* plane 1 */
	ROM_LOAD( "mpr-13465.b14", 0x20000, 0x20000, CRC(e08135e0) SHA1(cd748f4a8f67f562174fa9c6edb966c77b488e75) )
	ROM_LOAD( "mpr-13451.a15", 0x40000, 0x20000, CRC(1c49852f) SHA1(d08d6991c203400f685fada2445a22a7eceeee04) ) /* plane 2 */
	ROM_LOAD( "mpr-13466.b15", 0x60000, 0x20000, CRC(e14c6684) SHA1(f9b0d45e01a6df2b3875b493db9fa41bf37d79f0) )
	ROM_LOAD( "mpr-13452.a16", 0x80000, 0x20000, CRC(047bde5e) SHA1(e759baedcbb637a6c14af461b8a492554cadc9e4) ) /* plane 3 */
	ROM_LOAD( "mpr-13467.b16", 0xa0000, 0x20000, CRC(6309fec4) SHA1(f90c9679bade3cfbaa7949e412410c29d5bfa4d3) )

	ROM_REGION16_BE( 0x200000, REGION_GFX2, 0 ) /* sprites */
	ROM_LOAD16_BYTE( "mpr-13453.b1",  0x000001, 0x020000, CRC(5fa0a9f8) SHA1(d9d67cc006a608d48e37aa89359f1a9403172b00) )
	ROM_LOAD16_BYTE( "mpr-13457.b5",  0x000000, 0x020000, CRC(0d1b54da) SHA1(c7a6393f9e13adabe93b7a86aa7845c5f4d188f1) )
	ROM_LOAD16_BYTE( "mpr-13454.b2",  0x040001, 0x020000, CRC(5f6b33b1) SHA1(5d397bdaa2c7a9ce82cc9134bfeb78418dc613b0) )
	ROM_LOAD16_BYTE( "mpr-13458.b6",  0x040000, 0x020000, CRC(bad340c3) SHA1(e04ef028e44054d34831d3617c5a6348823cfebd) )
	ROM_LOAD16_BYTE( "mpr-13455.b3",  0x080001, 0x020000, CRC(4e80520b) SHA1(8147793ee0023ac18f633d756ecc53aef1980e95) )
	ROM_LOAD16_BYTE( "mpr-13459.b7",  0x080000, 0x020000, CRC(7e9165ac) SHA1(32114095f96fb0ae385c9bf31dd97a99ef182aaa) )
	ROM_LOAD16_BYTE( "mpr-13456.b4",  0x0c0001, 0x020000, CRC(5733c428) SHA1(865203ebee9d98e67324c46915d8835d0289ee0c) )
	ROM_LOAD16_BYTE( "mpr-13460.b8",  0x0c0000, 0x020000, CRC(66b8f9b3) SHA1(4bf003a9bed40673d88df51a314eb6bd10f0d039) )
	ROM_LOAD16_BYTE( "mpr-13440.a1",  0x100001, 0x020000, CRC(4f370b2b) SHA1(66beb4264d569520d80f8915e1c1fddbf61efb21) )
	ROM_LOAD16_BYTE( "mpr-13461.b10", 0x100000, 0x020000, CRC(f76014bf) SHA1(b44f0b6fb9dee927d81c62282aa946d8c35766ca) )
	ROM_LOAD16_BYTE( "mpr-13441.a2",  0x140001, 0x020000, CRC(37cf9cb4) SHA1(f51cee874ad8a824462d2475a23e7016ef64c6b4) )
	ROM_LOAD16_BYTE( "mpr-13462.b11", 0x140000, 0x020000, CRC(1061e7da) SHA1(bad3560f1ed6a5a79f4ecf85d3843b24ecf22d19) )
	ROM_LOAD16_BYTE( "mpr-13442.a3",  0x180001, 0x020000, CRC(049698ef) SHA1(dca1a78f0156cfac9acbfb6e47eb3897b579d2ec) )
	ROM_LOAD16_BYTE( "mpr-13463.b12", 0x180000, 0x020000, CRC(7dbcfbf1) SHA1(0b7be1de57f83b1213805489c6ebfc0f1e5fb4b0) )
	ROM_LOAD16_BYTE( "mpr-13443.a4",  0x1c0001, 0x020000, CRC(77a8989e) SHA1(0ad0877a9814fb7c2fb79062a50b1f9ce9420768) )
	ROM_LOAD16_BYTE( "mpr-13464.b13", 0x1c0000, 0x020000, CRC(551df422) SHA1(cf4cd2b66335853c7c6cce949e79c05e93a39666) )

	ROM_REGION( 0x50000, REGION_CPU2, 0 ) /* sound CPU */
	ROM_LOAD( "epr-13448.a10", 0x00000, 0x08000, CRC(b5183fb9) SHA1(c8372b57fa486256d49dc5851d6b17c92de593fb) )
	ROM_LOAD( "mpr-13449.a11", 0x10000, 0x20000, CRC(d3d9aaf9) SHA1(0fb3a8cb11033accceb3a43a691fb424cf8b9619) )
	ROM_RELOAD(                0x30000, 0x20000 )

	ROM_REGION( 0x0100, REGION_USER1, 0 )
	ROM_LOAD( "315-5298.b9",  0x0000, 0x00eb, CRC(39b47212) SHA1(432b47aee5ecbf08a8a6dc2f8379c816feb86328) ) /* PLS153 */
ROM_END

/*************************************************************************************************************************
  Aurail, Sega System 16B
  CPU: FD1089A (317-0167)
  ROM Board type: 171-5704
  Sega ID# for ROM board: 834-7702

  S1  - -
  S2  ---
  S3  ---
  S4  - -
  S5  - -
  S6  ---
  S7  ---
  S8  - -
  S9  ---
  S10 - -
  S11 ---
  S12 - -
  S13 ---
  S14 - -
  S15 ---
  S16 - -
  S17 ---
  S18 - -
  S19 - -
*/
ROM_START( aurailj )
	ROM_REGION( 0xc0000, REGION_CPU1, 0 ) /* 68000 code */
	ROM_LOAD16_BYTE( "epr-13446.a7", 0x000000, 0x20000, CRC(d1f57b2a) SHA1(6d7c64ce7634e1661ab5833385055b360c313494) )
	ROM_LOAD16_BYTE( "epr-13444.a5", 0x000001, 0x20000, CRC(7a2b045f) SHA1(088b7031cc8ec9431d44f35216fd16a83ef4f0b3) )
	ROM_LOAD16_BYTE( "epr-13447.a8", 0x080000, 0x20000, CRC(70a52167) SHA1(51512d4ee1e63902375b197cf04170744b099d88) )
	ROM_LOAD16_BYTE( "epr-13445.a6", 0x080001, 0x20000, CRC(28dfc3dd) SHA1(b1d6d3e31a48062a91cc9b7b6ff68bfde0a3ea1c) )

	ROM_REGION( 0xc0000, REGION_GFX1, 0 ) /* tiles */
	ROM_LOAD( "mpr-13450.a14", 0x00000, 0x20000, CRC(0fc4a7a8) SHA1(b46b22a6b0ead19551e67abfb3304c52b02b8d29) ) /* plane 1 */
	ROM_LOAD( "mpr-13465.b14", 0x20000, 0x20000, CRC(e08135e0) SHA1(cd748f4a8f67f562174fa9c6edb966c77b488e75) )
	ROM_LOAD( "mpr-13451.a15", 0x40000, 0x20000, CRC(1c49852f) SHA1(d08d6991c203400f685fada2445a22a7eceeee04) ) /* plane 2 */
	ROM_LOAD( "mpr-13466.b15", 0x60000, 0x20000, CRC(e14c6684) SHA1(f9b0d45e01a6df2b3875b493db9fa41bf37d79f0) )
	ROM_LOAD( "mpr-13452.a16", 0x80000, 0x20000, CRC(047bde5e) SHA1(e759baedcbb637a6c14af461b8a492554cadc9e4) ) /* plane 3 */
	ROM_LOAD( "mpr-13467.b16", 0xa0000, 0x20000, CRC(6309fec4) SHA1(f90c9679bade3cfbaa7949e412410c29d5bfa4d3) )

	ROM_REGION16_BE( 0x200000, REGION_GFX2, 0 ) /* sprites */
	ROM_LOAD16_BYTE( "mpr-13453.b1",  0x000001, 0x020000, CRC(5fa0a9f8) SHA1(d9d67cc006a608d48e37aa89359f1a9403172b00) )
	ROM_LOAD16_BYTE( "mpr-13457.b5",  0x000000, 0x020000, CRC(0d1b54da) SHA1(c7a6393f9e13adabe93b7a86aa7845c5f4d188f1) )
	ROM_LOAD16_BYTE( "mpr-13454.b2",  0x040001, 0x020000, CRC(5f6b33b1) SHA1(5d397bdaa2c7a9ce82cc9134bfeb78418dc613b0) )
	ROM_LOAD16_BYTE( "mpr-13458.b6",  0x040000, 0x020000, CRC(bad340c3) SHA1(e04ef028e44054d34831d3617c5a6348823cfebd) )
	ROM_LOAD16_BYTE( "mpr-13455.b3",  0x080001, 0x020000, CRC(4e80520b) SHA1(8147793ee0023ac18f633d756ecc53aef1980e95) )
	ROM_LOAD16_BYTE( "mpr-13459.b7",  0x080000, 0x020000, CRC(7e9165ac) SHA1(32114095f96fb0ae385c9bf31dd97a99ef182aaa) )
	ROM_LOAD16_BYTE( "mpr-13456.b4",  0x0c0001, 0x020000, CRC(5733c428) SHA1(865203ebee9d98e67324c46915d8835d0289ee0c) )
	ROM_LOAD16_BYTE( "mpr-13460.b8",  0x0c0000, 0x020000, CRC(66b8f9b3) SHA1(4bf003a9bed40673d88df51a314eb6bd10f0d039) )
	ROM_LOAD16_BYTE( "mpr-13440.a1",  0x100001, 0x020000, CRC(4f370b2b) SHA1(66beb4264d569520d80f8915e1c1fddbf61efb21) )
	ROM_LOAD16_BYTE( "mpr-13461.b10", 0x100000, 0x020000, CRC(f76014bf) SHA1(b44f0b6fb9dee927d81c62282aa946d8c35766ca) )
	ROM_LOAD16_BYTE( "mpr-13441.a2",  0x140001, 0x020000, CRC(37cf9cb4) SHA1(f51cee874ad8a824462d2475a23e7016ef64c6b4) )
	ROM_LOAD16_BYTE( "mpr-13462.b11", 0x140000, 0x020000, CRC(1061e7da) SHA1(bad3560f1ed6a5a79f4ecf85d3843b24ecf22d19) )
	ROM_LOAD16_BYTE( "mpr-13442.a3",  0x180001, 0x020000, CRC(049698ef) SHA1(dca1a78f0156cfac9acbfb6e47eb3897b579d2ec) )
	ROM_LOAD16_BYTE( "mpr-13463.b12", 0x180000, 0x020000, CRC(7dbcfbf1) SHA1(0b7be1de57f83b1213805489c6ebfc0f1e5fb4b0) )
	ROM_LOAD16_BYTE( "mpr-13443.a4",  0x1c0001, 0x020000, CRC(77a8989e) SHA1(0ad0877a9814fb7c2fb79062a50b1f9ce9420768) )
	ROM_LOAD16_BYTE( "mpr-13464.b13", 0x1c0000, 0x020000, CRC(551df422) SHA1(cf4cd2b66335853c7c6cce949e79c05e93a39666) )

	ROM_REGION( 0x50000, REGION_CPU2, 0 ) /* sound CPU */
	ROM_LOAD( "epr-13448.a10", 0x00000, 0x08000, CRC(b5183fb9) SHA1(c8372b57fa486256d49dc5851d6b17c92de593fb) )
	ROM_LOAD( "mpr-13449.a11", 0x10000, 0x20000, CRC(d3d9aaf9) SHA1(0fb3a8cb11033accceb3a43a691fb424cf8b9619) )
	ROM_RELOAD(                0x30000, 0x20000 )

	ROM_REGION( 0x2000, REGION_USER2, 0 ) /* decryption key */
	ROM_LOAD( "317-0167.key", 0x0000, 0x2000, CRC(fed38390) SHA1(b5f458bc70c069542be16d476645c153ed1d1b45) )

	ROM_REGION( 0x0100, REGION_USER1, 0 )
	ROM_LOAD( "315-5298.b9",  0x0000, 0x00eb, CRC(39b47212) SHA1(432b47aee5ecbf08a8a6dc2f8379c816feb86328) ) /* PLS153 */
ROM_END

ROM_START( aurailjd )
	ROM_REGION( 0xc0000, REGION_CPU1, 0 ) /* 68000 code */
	ROM_LOAD16_BYTE( "bootleg_epr-13446.a7", 0x000000, 0x20000, CRC(25221510) SHA1(8c461db9438bc785c5f81e3cb3bcea9adbd2be9c) )
	ROM_LOAD16_BYTE( "bootleg_epr-13444.a5", 0x000001, 0x20000, CRC(56ba5356) SHA1(a8cf41f415776328b645a23f13999ff356284772) )
	ROM_LOAD16_BYTE( "epr-13447.a8", 0x080000, 0x20000, CRC(70a52167) SHA1(51512d4ee1e63902375b197cf04170744b099d88) )
	ROM_LOAD16_BYTE( "epr-13445.a6", 0x080001, 0x20000, CRC(28dfc3dd) SHA1(b1d6d3e31a48062a91cc9b7b6ff68bfde0a3ea1c) )

	ROM_REGION( 0xc0000, REGION_GFX1, 0 ) /* tiles */
	ROM_LOAD( "mpr-13450.a14", 0x00000, 0x20000, CRC(0fc4a7a8) SHA1(b46b22a6b0ead19551e67abfb3304c52b02b8d29) ) /* plane 1 */
	ROM_LOAD( "mpr-13465.b14", 0x20000, 0x20000, CRC(e08135e0) SHA1(cd748f4a8f67f562174fa9c6edb966c77b488e75) )
	ROM_LOAD( "mpr-13451.a15", 0x40000, 0x20000, CRC(1c49852f) SHA1(d08d6991c203400f685fada2445a22a7eceeee04) ) /* plane 2 */
	ROM_LOAD( "mpr-13466.b15", 0x60000, 0x20000, CRC(e14c6684) SHA1(f9b0d45e01a6df2b3875b493db9fa41bf37d79f0) )
	ROM_LOAD( "mpr-13452.a16", 0x80000, 0x20000, CRC(047bde5e) SHA1(e759baedcbb637a6c14af461b8a492554cadc9e4) ) /* plane 3 */
	ROM_LOAD( "mpr-13467.b16", 0xa0000, 0x20000, CRC(6309fec4) SHA1(f90c9679bade3cfbaa7949e412410c29d5bfa4d3) )

	ROM_REGION16_BE( 0x200000, REGION_GFX2, 0 ) /* sprites */
	ROM_LOAD16_BYTE( "mpr-13453.b1",  0x000001, 0x020000, CRC(5fa0a9f8) SHA1(d9d67cc006a608d48e37aa89359f1a9403172b00) )
	ROM_LOAD16_BYTE( "mpr-13457.b5",  0x000000, 0x020000, CRC(0d1b54da) SHA1(c7a6393f9e13adabe93b7a86aa7845c5f4d188f1) )
	ROM_LOAD16_BYTE( "mpr-13454.b2",  0x040001, 0x020000, CRC(5f6b33b1) SHA1(5d397bdaa2c7a9ce82cc9134bfeb78418dc613b0) )
	ROM_LOAD16_BYTE( "mpr-13458.b6",  0x040000, 0x020000, CRC(bad340c3) SHA1(e04ef028e44054d34831d3617c5a6348823cfebd) )
	ROM_LOAD16_BYTE( "mpr-13455.b3",  0x080001, 0x020000, CRC(4e80520b) SHA1(8147793ee0023ac18f633d756ecc53aef1980e95) )
	ROM_LOAD16_BYTE( "mpr-13459.b7",  0x080000, 0x020000, CRC(7e9165ac) SHA1(32114095f96fb0ae385c9bf31dd97a99ef182aaa) )
	ROM_LOAD16_BYTE( "mpr-13456.b4",  0x0c0001, 0x020000, CRC(5733c428) SHA1(865203ebee9d98e67324c46915d8835d0289ee0c) )
	ROM_LOAD16_BYTE( "mpr-13460.b8",  0x0c0000, 0x020000, CRC(66b8f9b3) SHA1(4bf003a9bed40673d88df51a314eb6bd10f0d039) )
	ROM_LOAD16_BYTE( "mpr-13440.a1",  0x100001, 0x020000, CRC(4f370b2b) SHA1(66beb4264d569520d80f8915e1c1fddbf61efb21) )
	ROM_LOAD16_BYTE( "mpr-13461.b10", 0x100000, 0x020000, CRC(f76014bf) SHA1(b44f0b6fb9dee927d81c62282aa946d8c35766ca) )
	ROM_LOAD16_BYTE( "mpr-13441.a2",  0x140001, 0x020000, CRC(37cf9cb4) SHA1(f51cee874ad8a824462d2475a23e7016ef64c6b4) )
	ROM_LOAD16_BYTE( "mpr-13462.b11", 0x140000, 0x020000, CRC(1061e7da) SHA1(bad3560f1ed6a5a79f4ecf85d3843b24ecf22d19) )
	ROM_LOAD16_BYTE( "mpr-13442.a3",  0x180001, 0x020000, CRC(049698ef) SHA1(dca1a78f0156cfac9acbfb6e47eb3897b579d2ec) )
	ROM_LOAD16_BYTE( "mpr-13463.b12", 0x180000, 0x020000, CRC(7dbcfbf1) SHA1(0b7be1de57f83b1213805489c6ebfc0f1e5fb4b0) )
	ROM_LOAD16_BYTE( "mpr-13443.a4",  0x1c0001, 0x020000, CRC(77a8989e) SHA1(0ad0877a9814fb7c2fb79062a50b1f9ce9420768) )
	ROM_LOAD16_BYTE( "mpr-13464.b13", 0x1c0000, 0x020000, CRC(551df422) SHA1(cf4cd2b66335853c7c6cce949e79c05e93a39666) )

	ROM_REGION( 0x50000, REGION_CPU2, 0 ) /* sound CPU */
	ROM_LOAD( "epr-13448.a10", 0x00000, 0x08000, CRC(b5183fb9) SHA1(c8372b57fa486256d49dc5851d6b17c92de593fb) )
	ROM_LOAD( "mpr-13449.a11", 0x10000, 0x20000, CRC(d3d9aaf9) SHA1(0fb3a8cb11033accceb3a43a691fb424cf8b9619) )
	ROM_RELOAD(                0x30000, 0x20000 )

	ROM_REGION( 0x0100, REGION_USER1, 0 )
	ROM_LOAD( "315-5298.b9",  0x0000, 0x00eb, CRC(39b47212) SHA1(432b47aee5ecbf08a8a6dc2f8379c816feb86328) ) /* PLS153 */
ROM_END


ROM_START( bulletd )
	ROM_REGION( 0x040000, REGION_CPU1, 0 ) /* 68000 code */
	ROM_LOAD16_BYTE( "bootleg_epr-11010.a4",  0x000000, 0x08000, CRC(c4b7cb63) SHA1(c35fceab2a03f23d9690432a582064d12de950f6) )
	ROM_LOAD16_BYTE( "bootleg_epr-11007.a1",  0x000001, 0x08000, CRC(2afa84c5) SHA1(97f74ba4b9f83314c9e1f61afe7db3d7fa7a9935) )
	ROM_LOAD16_BYTE( "epr11011.a5",  0x010000, 0x08000, CRC(7f446b9f) SHA1(0b92ab100c13bdcdd0f770da5da5e19cb79afde1) )
	ROM_LOAD16_BYTE( "epr11008.a2",  0x010001, 0x08000, CRC(34824d3b) SHA1(7a3134a71ad176b8a08a919c0acb75ae1e05743b) )
	ROM_LOAD16_BYTE( "epr11012.a6",  0x020000, 0x08000, CRC(3992f159) SHA1(50686b394693ab01cbd159ae661f326c8eee50b8) )
	ROM_LOAD16_BYTE( "epr11009.a3",  0x020001, 0x08000, CRC(df199999) SHA1(2669e923aa4f1bedc788401f44ad19c318658f00) )

	ROM_REGION( 0x30000, REGION_GFX1, ROMREGION_DISPOSE ) /* tiles */
	ROM_LOAD( "epr10994.b9",  0x00000, 0x10000, CRC(3035468a) SHA1(778366815a2a74188d72d64c5e1e95215bc4ca81) )
	ROM_LOAD( "epr10995.b10", 0x10000, 0x10000, CRC(6b97aff1) SHA1(323bafe43a703476f6f4e68b46ec86bb9152f88e) )
	ROM_LOAD( "epr10996.b11", 0x20000, 0x10000, CRC(501bddd6) SHA1(545273b1b874b3e68d23b0dcae81c8531bd98756) )

	ROM_REGION16_BE( 0x80000, REGION_GFX2, ROMREGION_ERASE00 ) /* sprites */
	ROM_LOAD16_BYTE( "epr10999.b1", 0x00001, 0x010000, CRC(119f0008) SHA1(6a39b537bb58ea19ed3b0322ebca37e6574289fd) )
	ROM_LOAD16_BYTE( "epr11003.b5", 0x00000, 0x010000, CRC(2f429089) SHA1(08bf9d9c15fafbcb26604ff30be367ecf25404b2) )
	ROM_LOAD16_BYTE( "epr11000.b2", 0x20001, 0x010000, CRC(f5482bbe) SHA1(d8482ba73622798b15e78ab2c123d0fd4c33480a) )
	ROM_LOAD16_BYTE( "epr11004.b6", 0x20000, 0x010000, CRC(8c886df0) SHA1(348f9111fe45fc94cb32b101d0a1a6a39ef1ec50) )
	ROM_LOAD16_BYTE( "epr11001.b3", 0x40001, 0x010000, CRC(65ea71e0) SHA1(79224c445ceaa1d13a3616e58e9d4eb595e920cb) )
	ROM_LOAD16_BYTE( "epr11005.b7", 0x40000, 0x010000, CRC(ea2f9d50) SHA1(db62584591d62780f81de651869bc74a61363793) )
	ROM_LOAD16_BYTE( "epr11002.b4", 0x60001, 0x010000, CRC(9e25042b) SHA1(cb0e20ca8ca1c42ad2a95b83ea8711b7ad8e42f5) )
	ROM_LOAD16_BYTE( "epr11006.b8", 0x60000, 0x010000, CRC(6b7384f2) SHA1(5201e3b5e4aeb4bc8f5b3ba3d8a9ffb3705eccf4) )

	ROM_REGION( 0x50000, REGION_CPU2, 0 ) /* sound CPU */
	ROM_LOAD( "epr10997.a7", 0x00000, 0x8000, CRC(5dd9cab5) SHA1(b9b27bbdc19feacb83cc5b33a74d910d86ac8f33) )
	ROM_LOAD( "epr10998.a8", 0x10000, 0x8000, CRC(f971a817) SHA1(502c95638e4fd5f87e5fc837cb44b39a5d62f4e4) )
ROM_END

ROM_START( cottond )
	ROM_REGION( 0x100000, REGION_CPU1, 0 ) /* 68000 code */
	ROM_LOAD16_BYTE( "bootleg_epr-13921a.a7", 0x000000, 0x20000, CRC(92947867) SHA1(6d5f1be45690bacac7093b442ed99c4de56d73a4) )
	ROM_LOAD16_BYTE( "bootleg_epr-13919a.a5", 0x000001, 0x20000, CRC(30f131fb) SHA1(5b35b4691d2436e82be3079634d8f7f259e46450) )
	ROM_LOAD16_BYTE( "bootleg_epr-13922a.a8", 0x080000, 0x20000, CRC(f0f75329) SHA1(e223b3b6e15ead11f93e353ddce5227f8b362d2e) )
	ROM_LOAD16_BYTE( "bootleg_epr-13920a.a6", 0x080001, 0x20000, CRC(a3721aab) SHA1(bfcd8e564f06520e51c61418246ef06e4a0036d7) )

	ROM_REGION( 0xc0000, REGION_GFX1, 0 ) /* tiles */
	ROM_LOAD( "opr-13862.a14", 0x00000, 0x20000, CRC(a47354b6) SHA1(ce52813b245f1d491a134d4bd5ab074e71d20129) )
	ROM_LOAD( "opr-13877.b14", 0x20000, 0x20000, CRC(d38424b5) SHA1(884ca190936aee2d2cac86491d4d0cdf4a45efe5) )
	ROM_LOAD( "opr-13863.a15", 0x40000, 0x20000, CRC(8c990026) SHA1(07b4510936376c171f3b31d87ac6154361eb0cbc) )
	ROM_LOAD( "opr-13878.b15", 0x60000, 0x20000, CRC(21c15b8a) SHA1(690d92420ec5465885e0f4870419992961420e33) )
	ROM_LOAD( "opr-13864.a16", 0x80000, 0x20000, CRC(d2b175bf) SHA1(897b7c794d0e7229ea5e9a682f64266a947a818f) )
	ROM_LOAD( "opr-13879.b16", 0xa0000, 0x20000, CRC(b9d62531) SHA1(e8c5e7b93339c00f75a3b66ce18f7838255577be) )

	ROM_REGION16_BE( 0x200000, REGION_GFX2, 0 ) /* sprites */
	ROM_LOAD16_BYTE( "opr-13869.b5", 0x000000, 0x20000, CRC(ab4b3468) SHA1(3071654a295152d609d2c2c1d4153b5ba3f174d5) )
	ROM_LOAD16_BYTE( "opr-13865.b1", 0x000001, 0x20000, CRC(7024f404) SHA1(4b2f9cdfdd97218797a3e386106e53f713b8650d) )
	ROM_LOAD16_BYTE( "opr-13870.b6", 0x040000, 0x20000, CRC(69b41ac3) SHA1(4c5a85e5a5ca9f8260557d4e97eb091dd857d63a) )
	ROM_LOAD16_BYTE( "opr-13866.b2", 0x040001, 0x20000, CRC(6169bba4) SHA1(a24a418ee7cd0c1109870a2e7a91e430671897ed) )
	ROM_LOAD16_BYTE( "opr-13871.b7", 0x080000, 0x20000, CRC(0801cf02) SHA1(3007bbbce2f327f4700e78e2b8672f4482189cd7) )
	ROM_LOAD16_BYTE( "opr-13867.b3", 0x080001, 0x20000, CRC(b014f02d) SHA1(46f5ed0b44cee03a6aec9ec57b506bb15bf35e47) )
	ROM_LOAD16_BYTE( "opr-13872.b8", 0x0c0000, 0x20000, CRC(f066f315) SHA1(bbeb24daaded994240d0cdb5cec2e662b677cb75) )
	ROM_LOAD16_BYTE( "opr-13868.b4", 0x0c0001, 0x20000, CRC(e62a7cd6) SHA1(1e6d06345f7b6cef2e887d9b9cd45e0155140c5e) )
	ROM_LOAD16_BYTE( "opr-13873.b10",0x100000, 0x20000, CRC(1bd145f3) SHA1(4744ffe9fbda453785345b46eb61b56730048f42) )
	ROM_LOAD16_BYTE( "opr-13852.a1", 0x100001, 0x20000, CRC(943aba8b) SHA1(d0dd1665a8d9495a92ae4e35d6b15b966e8d43cd) )
	ROM_LOAD16_BYTE( "opr-13874.b11",0x140000, 0x20000, CRC(4fd59bff) SHA1(2b4630e49b60593d668fe34d8faf712ac6928c14) )
	ROM_LOAD16_BYTE( "opr-13853.a2", 0x140001, 0x20000, CRC(7ea93200) SHA1(8e2d8cd48a12306772653f25bddc99ad0597a698) )
	ROM_LOAD16_BYTE( "opr-13894.b12",0x180000, 0x20000, CRC(e3d0bee2) SHA1(503a78123ca9d6f3405972bca281dcdaba929c99) )
	ROM_LOAD16_BYTE( "opr-13891.a3", 0x180001, 0x20000, CRC(c6b3c414) SHA1(0f0d936e77eb483be8865e8d968d78260e88ca99) )
	ROM_LOAD16_BYTE( "opr-13876.b13",0x1c0000, 0x20000, CRC(1c5ffad8) SHA1(13e5886ceece564cc71ba7f43a26d2b1782ccfc8) )
	ROM_LOAD16_BYTE( "opr-13855.a4", 0x1c0001, 0x20000, CRC(856f3ee2) SHA1(72346d887ff9738ebe93acb2e3f8cd80d494621e) )

	ROM_REGION( 0x50000, REGION_CPU2, 0 ) /* sound CPU */
	ROM_LOAD( "epr-13892.a10", 0x00000, 0x08000, CRC(fdfbe6ad) SHA1(9ebb94889c0e96e6af9cdced084804ca98612d61) )
	ROM_LOAD( "opr-13893.a11", 0x10000, 0x20000, CRC(384233df) SHA1(dfdf94697587a5ee45e97700f3741be54b90742b) )

	ROM_REGION( 0x0100, REGION_USER1, 0 )
	ROM_LOAD( "315-5298.b9",  0x0000, 0x00eb, CRC(39b47212) SHA1(432b47aee5ecbf08a8a6dc2f8379c816feb86328) ) /* PLS153 */
ROM_END

/*************************************************************************************************************************
  Dunk Shot, Sega System 16B
  CPU: FD1089A (317-0022)
  ROM Board type: 171-5358
       I/O board: 834-6180
      Main board: 837-6237
    Sega game ID: 833-6235-05 DUNK SHOT
       ROM board: 834-6236-04
*/
ROM_START( dunkshot )
	ROM_REGION( 0x30000, REGION_CPU1, 0 ) /* 68000 code */
	ROM_LOAD16_BYTE( "epr-10520c.a1", 0x000001, 0x8000, CRC(ba9c5d10) SHA1(370d0455903c1bce3b5ad2ffa1d02ccd6da78840) )
	ROM_LOAD16_BYTE( "epr-10523c.a4", 0x000000, 0x8000, CRC(106733c2) SHA1(d792b3d5073becbd9f440faff45692ab9e6309b9) )
	ROM_LOAD16_BYTE( "epr-10521.a2",  0x010001, 0x8000, CRC(e2d5f97a) SHA1(bf7b4a029580633fee65be89d5c9c83ff76a8484) ) /* == epr-10468.a2 */
	ROM_LOAD16_BYTE( "epr-10524.a5",  0x010000, 0x8000, CRC(22777314) SHA1(fbc35505a94c8d4bdb44ee058e9e2e9e9b377c5c) ) /* == epr-10471.a5 */
	ROM_LOAD16_BYTE( "epr-10522.a3",  0x020001, 0x8000, CRC(e5b5f754) SHA1(af02c46437e3cf62331753dc405211b7f90e3f62) )
	ROM_LOAD16_BYTE( "epr-10525.a6",  0x020000, 0x8000, CRC(7f41f334) SHA1(631f6113f3c0c47f2dd1ee0ea6e7db4321d7366d) )

	ROM_REGION( 0x18000, REGION_GFX1, 0 ) /* tiles */
	ROM_LOAD( "epr-10528.b9",  0x00000, 0x8000, CRC(a8a3762d) SHA1(af75df6eda0df903e2b3f9680cd128da4227961d) )
	ROM_LOAD( "epr-10529.b10", 0x08000, 0x8000, CRC(80cbff50) SHA1(3641ee337194d56d774bf1be91939d03f3c0f77b) )
	ROM_LOAD( "epr-10530.b11", 0x10000, 0x8000, CRC(2dbe1e52) SHA1(a6b74f88e2f47322fbde1f6682cae58caf79f6c8) )

	ROM_REGION16_BE( 0x80000, REGION_GFX2, 0 ) /* sprites */
	ROM_LOAD16_BYTE( "epr-10481.b5", 0x00000, 0x8000, CRC(feb04bc9) SHA1(233dc8e3b887a88ac114723d58a909a58f0ae771) )
	ROM_RELOAD(                      0x10000, 0x8000 )
	ROM_LOAD16_BYTE( "epr-10477.b1", 0x00001, 0x8000, CRC(f9d3b2cb) SHA1(b530fe16882c718122bfd1de098f39e54993de28) )
	ROM_RELOAD(                      0x10001, 0x8000 )
	ROM_LOAD16_BYTE( "epr-10482.b6", 0x20000, 0x8000, CRC(5bc07618) SHA1(f4c88f81b407d467f958181770ea4fd32aab3daf) )
	ROM_RELOAD(                      0x30000, 0x8000 )
	ROM_LOAD16_BYTE( "epr-10478.b2", 0x20001, 0x8000, CRC(5b5c5c92) SHA1(1c6f1cafa0788678c80ade11560f4a8d8bb7272a) )
	ROM_RELOAD(                      0x30001, 0x8000 )
	ROM_LOAD16_BYTE( "epr-10483.b7", 0x40000, 0x8000, CRC(7cab4f9e) SHA1(2310a9fe604f78d74d84bea301c95e6f0e6a6085) )
	ROM_RELOAD(                      0x50000, 0x8000 )
	ROM_LOAD16_BYTE( "epr-10479.b3", 0x40001, 0x8000, CRC(e84190a0) SHA1(23a8799adf81e1884a8c6b4c55397b8bca2f1850) )
	ROM_RELOAD(                      0x50001, 0x8000 )
	ROM_LOAD16_BYTE( "epr-10527.b8", 0x60000, 0x8000, CRC(39b1a242) SHA1(cf0c0768d006a18345b66dd389acba1e8192ec53) )
	ROM_RELOAD(                      0x70000, 0x8000 )
	ROM_LOAD16_BYTE( "epr-10526.b4", 0x60001, 0x8000, CRC(bf200754) SHA1(60900d80cfea147b011813dde558c1d39fdd274c) )
	ROM_RELOAD(                      0x70001, 0x8000 )

	ROM_REGION( 0x50000, REGION_CPU2, 0 ) /* sound CPU */
	ROM_LOAD( "epr-10473.a7",   0x00000, 0x08000, CRC(7f1f5a27) SHA1(7ff91b95c883b395ab4ff5e440d78e553a09e623) )
	ROM_LOAD( "epr-10474.a8",   0x10000, 0x08000, CRC(419a656e) SHA1(aa734ae835761badeb069f99acc5fded2a19b3a3) )
	ROM_LOAD( "epr-10475.a9",   0x20000, 0x08000, CRC(17d55e85) SHA1(0c414bafecbfaa82679cc155f15f5255c186358d) )
	ROM_LOAD( "epr-10476.a10",  0x30000, 0x08000, CRC(a6be0956) SHA1(fc4d6e25e0b46679f94fddbb1850fb0b02f8d84b) )

	ROM_REGION( 0x2000, REGION_USER2, 0 ) /* decryption key */
	ROM_LOAD( "317-0022.key", 0x0000, 0x2000, CRC(3f218333) SHA1(6f73801070a2c9748fc319cc95ab7a802f8ea7b6) )
ROM_END

ROM_START( dunkshota ) /* several ROMs had replacement? (different style to others) labels with 'T' markings, content identical. */
	ROM_REGION( 0x30000, REGION_CPU1, 0 ) /* 68000 code */
	ROM_LOAD16_BYTE( "epr-10520a.a1", 0x000001, 0x8000, CRC(16e213ba) SHA1(efddff17d1802ccbea9eac69cedef62fa6b9a640) )
	ROM_LOAD16_BYTE( "epr-10523a.a4", 0x000000, 0x8000, CRC(22e3f074) SHA1(acdb6952308957244355bedb9cc627314a0139ef) )
	ROM_LOAD16_BYTE( "epr-10521.a2",  0x010001, 0x8000, CRC(e2d5f97a) SHA1(bf7b4a029580633fee65be89d5c9c83ff76a8484) ) /* == epr-10468.a2 */
	ROM_LOAD16_BYTE( "epr-10524.a5",  0x010000, 0x8000, CRC(22777314) SHA1(fbc35505a94c8d4bdb44ee058e9e2e9e9b377c5c) ) /* == epr-10471.a5 */
	ROM_LOAD16_BYTE( "epr-10522.a3",  0x020001, 0x8000, CRC(e5b5f754) SHA1(af02c46437e3cf62331753dc405211b7f90e3f62) )
	ROM_LOAD16_BYTE( "epr-10525.a6",  0x020000, 0x8000, CRC(7f41f334) SHA1(631f6113f3c0c47f2dd1ee0ea6e7db4321d7366d) )

	ROM_REGION( 0x18000, REGION_GFX1, 0 ) /* tiles */
	ROM_LOAD( "epr-10528.b9",  0x00000, 0x8000, CRC(a8a3762d) SHA1(af75df6eda0df903e2b3f9680cd128da4227961d) )
	ROM_LOAD( "epr-10529.b10", 0x08000, 0x8000, CRC(80cbff50) SHA1(3641ee337194d56d774bf1be91939d03f3c0f77b) )
	ROM_LOAD( "epr-10530.b11", 0x10000, 0x8000, CRC(2dbe1e52) SHA1(a6b74f88e2f47322fbde1f6682cae58caf79f6c8) )

	ROM_REGION16_BE( 0x80000, REGION_GFX2, 0 ) /* sprites */
	ROM_LOAD16_BYTE( "epr-10481.b5", 0x00000, 0x8000, CRC(feb04bc9) SHA1(233dc8e3b887a88ac114723d58a909a58f0ae771) )
	ROM_RELOAD(                      0x10000, 0x8000 )
	ROM_LOAD16_BYTE( "epr-10477.b1", 0x00001, 0x8000, CRC(f9d3b2cb) SHA1(b530fe16882c718122bfd1de098f39e54993de28) )
	ROM_RELOAD(                      0x10001, 0x8000 )
	ROM_LOAD16_BYTE( "epr-10482.b6", 0x20000, 0x8000, CRC(5bc07618) SHA1(f4c88f81b407d467f958181770ea4fd32aab3daf) )
	ROM_RELOAD(                      0x30000, 0x8000 )
	ROM_LOAD16_BYTE( "epr-10478.b2", 0x20001, 0x8000, CRC(5b5c5c92) SHA1(1c6f1cafa0788678c80ade11560f4a8d8bb7272a) )
	ROM_RELOAD(                      0x30001, 0x8000 )
	ROM_LOAD16_BYTE( "epr-10483.b7", 0x40000, 0x8000, CRC(7cab4f9e) SHA1(2310a9fe604f78d74d84bea301c95e6f0e6a6085) )
	ROM_RELOAD(                      0x50000, 0x8000 )
	ROM_LOAD16_BYTE( "epr-10479.b3", 0x40001, 0x8000, CRC(e84190a0) SHA1(23a8799adf81e1884a8c6b4c55397b8bca2f1850) )
	ROM_RELOAD(                      0x50001, 0x8000 )
	ROM_LOAD16_BYTE( "epr-10527.b8", 0x60000, 0x8000, CRC(39b1a242) SHA1(cf0c0768d006a18345b66dd389acba1e8192ec53) )
	ROM_RELOAD(                      0x70000, 0x8000 )
	ROM_LOAD16_BYTE( "epr-10526.b4", 0x60001, 0x8000, CRC(bf200754) SHA1(60900d80cfea147b011813dde558c1d39fdd274c) )
	ROM_RELOAD(                      0x70001, 0x8000 )

	ROM_REGION( 0x50000, REGION_CPU2, 0 ) /* sound CPU */
	ROM_LOAD( "epr-10473.a7",   0x00000, 0x08000, CRC(7f1f5a27) SHA1(7ff91b95c883b395ab4ff5e440d78e553a09e623) )
	ROM_LOAD( "epr-10474.a8",   0x10000, 0x08000, CRC(419a656e) SHA1(aa734ae835761badeb069f99acc5fded2a19b3a3) )
	ROM_LOAD( "epr-10475.a9",   0x20000, 0x08000, CRC(17d55e85) SHA1(0c414bafecbfaa82679cc155f15f5255c186358d) )
	ROM_LOAD( "epr-10476.a10",  0x30000, 0x08000, CRC(a6be0956) SHA1(fc4d6e25e0b46679f94fddbb1850fb0b02f8d84b) )

	ROM_REGION( 0x2000, REGION_USER2, 0 ) /* decryption key */
	ROM_LOAD( "317-0022.key", 0x0000, 0x2000, CRC(3f218333) SHA1(6f73801070a2c9748fc319cc95ab7a802f8ea7b6) )
ROM_END

ROM_START( dunkshoto )
	ROM_REGION( 0x30000, REGION_CPU1, 0 ) /* 68000 code */
	ROM_LOAD16_BYTE( "epr-10467.a1", 0x000001, 0x8000, CRC(29774114) SHA1(3a88739213afd4ef7807ddbd3acdfddeb9636fd3) )
	ROM_LOAD16_BYTE( "epr-10470.a4", 0x000000, 0x8000, CRC(8c60761f) SHA1(aba009f482df7023b460ab20e50225ab5f6dff6d) )
	ROM_LOAD16_BYTE( "epr-10468.a2", 0x010001, 0x8000, CRC(e2d5f97a) SHA1(bf7b4a029580633fee65be89d5c9c83ff76a8484) )
	ROM_LOAD16_BYTE( "epr-10471.a5", 0x010000, 0x8000, CRC(22777314) SHA1(fbc35505a94c8d4bdb44ee058e9e2e9e9b377c5c) )
	ROM_LOAD16_BYTE( "epr-10469.a3", 0x020001, 0x8000, CRC(aa442b81) SHA1(24f455bc59147ccd948fd89e2048a118b5591d84) )
	ROM_LOAD16_BYTE( "epr-10472.a6", 0x020000, 0x8000, CRC(206027a6) SHA1(2b7d4754639d7023bc00f5e0fe9de4d2a971e487) )

	ROM_REGION( 0x18000, REGION_GFX1, 0 ) /* tiles */
	ROM_LOAD( "epr-10485.b9",  0x00000, 0x8000, CRC(f16dda29) SHA1(88f3aa5e6f827e124c5bba0978a6ccfde8cb6fe7) )
	ROM_LOAD( "epr-10486.b10", 0x08000, 0x8000, CRC(311d973c) SHA1(c4765917ba788ed45a801499f3d873a86c418eb8) )
	ROM_LOAD( "epr-10487.b11", 0x10000, 0x8000, CRC(a8fb179f) SHA1(8a748d537b3d327c41d6dac17342de9be068e53b) )

	ROM_REGION16_BE( 0x80000, REGION_GFX2, 0 ) /* sprites */
	ROM_LOAD16_BYTE( "epr-10481.b5", 0x00000, 0x8000, CRC(feb04bc9) SHA1(233dc8e3b887a88ac114723d58a909a58f0ae771) )
	ROM_RELOAD(                      0x10000, 0x8000 )
	ROM_LOAD16_BYTE( "epr-10477.b1", 0x00001, 0x8000, CRC(f9d3b2cb) SHA1(b530fe16882c718122bfd1de098f39e54993de28) )
	ROM_RELOAD(                      0x10001, 0x8000 )
	ROM_LOAD16_BYTE( "epr-10482.b6", 0x20000, 0x8000, CRC(5bc07618) SHA1(f4c88f81b407d467f958181770ea4fd32aab3daf) )
	ROM_RELOAD(                      0x30000, 0x8000 )
	ROM_LOAD16_BYTE( "epr-10478.b2", 0x20001, 0x8000, CRC(5b5c5c92) SHA1(1c6f1cafa0788678c80ade11560f4a8d8bb7272a) )
	ROM_RELOAD(                      0x30001, 0x8000 )
	ROM_LOAD16_BYTE( "epr-10483.b7", 0x40000, 0x8000, CRC(7cab4f9e) SHA1(2310a9fe604f78d74d84bea301c95e6f0e6a6085) )
	ROM_RELOAD(                      0x50000, 0x8000 )
	ROM_LOAD16_BYTE( "epr-10479.b3", 0x40001, 0x8000, CRC(e84190a0) SHA1(23a8799adf81e1884a8c6b4c55397b8bca2f1850) )
	ROM_RELOAD(                      0x50001, 0x8000 )
	ROM_LOAD16_BYTE( "epr-10484.b8", 0x60000, 0x8000, CRC(bcb5fcc9) SHA1(eb8d557c908e6265f96a5a7e832e19776a9e576c) )
	ROM_RELOAD(                      0x70000, 0x8000 )
	ROM_LOAD16_BYTE( "epr-10480.b4", 0x60001, 0x8000, CRC(5dffd9dd) SHA1(256b24613c952d89dbb9971c9091d5a8a7f363b0) )
	ROM_RELOAD(                      0x70001, 0x8000 )

	ROM_REGION( 0x50000, REGION_CPU2, 0 ) /* sound CPU */
	ROM_LOAD( "epr-10473.a7",   0x00000, 0x08000, CRC(7f1f5a27) SHA1(7ff91b95c883b395ab4ff5e440d78e553a09e623) )
	ROM_LOAD( "epr-10474.a8",   0x10000, 0x08000, CRC(419a656e) SHA1(aa734ae835761badeb069f99acc5fded2a19b3a3) )
	ROM_LOAD( "epr-10475.a9",   0x20000, 0x08000, CRC(17d55e85) SHA1(0c414bafecbfaa82679cc155f15f5255c186358d) )
	ROM_LOAD( "epr-10476.a10",  0x30000, 0x08000, CRC(a6be0956) SHA1(fc4d6e25e0b46679f94fddbb1850fb0b02f8d84b) )

	ROM_REGION( 0x2000, REGION_USER2, 0 ) /* decryption key */
	ROM_LOAD( "317-0022.key", 0x0000, 0x2000, CRC(3f218333) SHA1(6f73801070a2c9748fc319cc95ab7a802f8ea7b6) )
ROM_END

ROM_START( defense )
	ROM_REGION( 0x30000, REGION_CPU1, 0 ) /* 68000 code */
	ROM_LOAD16_BYTE( "epr-10917a.a4", 0x000000, 0x8000, CRC(d91ac47c) SHA1(298e4a9c1b00bd6090e7c4f42efd4c6e20b69d62) )
	ROM_LOAD16_BYTE( "epr-10915.a1",  0x000001, 0x8000, CRC(7344c510) SHA1(4bee5485204e8ec858726e403c55bb1592a991cc) )
	ROM_LOAD16_BYTE( "epr-10918a.a5", 0x010000, 0x8000, CRC(e41befcd) SHA1(bbac5f501d215b18d34e3d366fd39cf7fa38a25c) )
	ROM_LOAD16_BYTE( "epr-10916a.a2", 0x010001, 0x8000, CRC(7f58ba12) SHA1(38ca3ded758a005f6bb345ffe991a7d374ddeede) )
	ROM_LOAD16_BYTE( "epr-10829.a6",  0x020000, 0x8000, CRC(a431ab08) SHA1(95888af6fae598c40e7fefffbfd1f0b551e9f1be) )
	ROM_LOAD16_BYTE( "epr-10826.a3",  0x020001, 0x8000, CRC(2ed8e4b7) SHA1(23da16e29a475d6ec7ccec8cdd18a1dc78ae69cd) )

	ROM_REGION( 0x30000, REGION_GFX1, 0 ) /* tiles */
	ROM_LOAD( "10919.b9",  0x00000, 0x10000, CRC(23b88f82) SHA1(6c1336c17bdd8adc39bf4eca5b3348ac7b1e6559) )
	ROM_LOAD( "10920.b10", 0x10000, 0x10000, CRC(22b1fb4c) SHA1(f4721796155f13d472d735c646bc52f20f04debd) )
	ROM_LOAD( "10921.b11", 0x20000, 0x10000, CRC(7788f55d) SHA1(435273196a5e812f28a2224807e842ccadff9c10) )

	ROM_REGION16_BE( 0x60000, REGION_GFX2, 0 ) /* sprites */
	ROM_LOAD16_BYTE( "10760.b1", 0x00001, 0x010000, CRC(70de327b) SHA1(11dde9cefd993f5fb02baf5809fae6c1176a58a1) )
	ROM_LOAD16_BYTE( "10763.b5", 0x00000, 0x010000, CRC(99ec5cb5) SHA1(933a2216a2c772fc82499c739457865b1c75cdb8) )
	ROM_LOAD16_BYTE( "10761.b2", 0x20001, 0x010000, CRC(4e80f80d) SHA1(d168235bdf09317545c999676a4adf015df32366) )
	ROM_LOAD16_BYTE( "10764.b6", 0x20000, 0x010000, CRC(602da5d5) SHA1(d32cdde7d86c4561e7bfa547d7d7995ce9a43c24) )
	ROM_LOAD16_BYTE( "10762.b3", 0x40001, 0x010000, CRC(464b5f78) SHA1(b730964a54e6a63fa5b7cc2cbf9ec0ab650626d5) )
	ROM_LOAD16_BYTE( "10765.b7", 0x40000, 0x010000, CRC(0a73a057) SHA1(7f31124c67541a245e069e5b6aac59935d99a9a9) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* sound CPU */
	ROM_LOAD( "10775.a7", 0x0000, 0x8000, CRC(4cbd55a8) SHA1(8af2c52ab61338c8a9f1a74a05470dd3d5e0c42f) )

	ROM_REGION( 0x2000, REGION_USER2, 0 ) /* decryption key */
	ROM_LOAD( "317-0028.key", 0x0000, 0x2000, BAD_DUMP CRC(1514662f) SHA1(d6241cb2a70c5b0442ecc2f8a10307608e5a0870) )
ROM_END

ROM_START( fantzn2x )
	ROM_REGION( 0xc0000, REGION_CPU1, 0 ) /* 68000 code */
	ROM_LOAD16_BYTE( "fz2.a7", 0x00000, 0x20000, CRC(94c05f0b) SHA1(53da68a919776a46ae96dbc094ff941308d13613) )
	ROM_LOAD16_BYTE( "fz2.a5", 0x00001, 0x20000, CRC(f3526895) SHA1(3197956608138601192f111d3bcc26662a7d6ec1) )
	/* empty 0x40000 - 0x80000 */
	ROM_LOAD16_BYTE( "fz2.a8", 0x80000, 0x20000, CRC(b2ebb209) SHA1(bd40c90a372ab92a869bdd28d12cf52b45ecc80e) )
	ROM_LOAD16_BYTE( "fz2.a6", 0x80001, 0x20000, CRC(6833f546) SHA1(b4503cdb5bdb1322c34b9da3ff4227c740dad707) )

	ROM_REGION( 0x60000, REGION_GFX1, ROMREGION_DISPOSE ) /* tiles */
	ROM_LOAD( "fz2.a14", 0x00000, 0x20000, CRC(1c0a4537) SHA1(3abdf51ea81780309bcfaf12c04efdf7cb15a649) )
	ROM_LOAD( "fz2.a15", 0x20000, 0x20000, CRC(2b933344) SHA1(5b53ea8d58cc3d157aec6926db048359984e4276) )
	ROM_LOAD( "fz2.a16", 0x40000, 0x20000, CRC(e63281a1) SHA1(72379c579484c1ef7784a9598d373446ef0a472b) )

	ROM_REGION16_BE( 0x180000, REGION_GFX2, 0 ) /* sprites */
	ROM_LOAD16_BYTE( "fz2.b1",  0x000001, 0x20000, CRC(46bba615) SHA1(b291df4a83d7155eb7606f86ed733c24362a4db3) )
	ROM_LOAD16_BYTE( "fz2.b5",  0x000000, 0x20000, CRC(bebeee5d) SHA1(9e57e62c6b9136667aa90d7d423fc33ac6df4352) )
	ROM_LOAD16_BYTE( "fz2.b2",  0x040001, 0x20000, CRC(6681a7b6) SHA1(228df38601ba3895e9449921a64850941715b421) )
	ROM_LOAD16_BYTE( "fz2.b6",  0x040000, 0x20000, CRC(42d3241f) SHA1(c3240e3e1d7d398e74e76ba65adca6b06f0f67a9) )
	ROM_LOAD16_BYTE( "fz2.b3",  0x080001, 0x20000, CRC(5863926f) SHA1(0e591c4b85e5d572b3311bec2c1f6d2484204db6) )
	ROM_LOAD16_BYTE( "fz2.b7",  0x080000, 0x20000, CRC(cd830510) SHA1(8a32a1aa43f8af5e86f552f05da40b6e4ba12495) )
	ROM_LOAD16_BYTE( "fz2.b4",  0x0c0001, 0x20000, CRC(b98fa5b6) SHA1(c3f8891f81e80321e2ee5cc1f4d93b1867ed1868) )
	ROM_LOAD16_BYTE( "fz2.b8",  0x0c0000, 0x20000, CRC(e8248f68) SHA1(7876945d2baf1d7bdb9cc3a23be9f1a1681cede9) )
	ROM_LOAD16_BYTE( "fz2.a1",  0x100001, 0x20000, CRC(9d2f41f3) SHA1(54f5dc47d854cd26b108695f55263d8b8c29ce0e) )
	ROM_LOAD16_BYTE( "fz2.b10", 0x100000, 0x20000, CRC(7686ea33) SHA1(812a638f42500b30f80f9a3956c5eb4553cc35d0) )
	ROM_LOAD16_BYTE( "fz2.a2",  0x140001, 0x20000, CRC(3b4050b7) SHA1(8c7c8051c577a4b2ca54d7e60c100fbd5391551f) )
	ROM_LOAD16_BYTE( "fz2.b11", 0x140000, 0x20000, CRC(da8a95dc) SHA1(d44e1515008d4ee302f940ce7799fa9a790799e9) )

	ROM_REGION( 0x50000, REGION_CPU2, 0 ) /* sound CPU */
	ROM_LOAD( "fz2.a10", 0x00000, 0x08000, CRC(92c92924) SHA1(3c98cea8f42c316405b28ae03469c6876de5e806) )
	ROM_LOAD( "fz2.a11", 0x10000, 0x20000, CRC(8c641bb9) SHA1(920da63961d2f3457c80d4c5f6d4f405374bb23a) )
ROM_END


/*************************************************************************************************************************
  SDI, Sega System 16B
  CPU: FD1089A 317-0028
  ROM Board type: 171-5358
*/
ROM_START( sdib )
	ROM_REGION( 0x30000, REGION_CPU1, 0 ) /* 68000 code */
	/* the ic positions given (at the end of the line) can't be right?? */
	ROM_LOAD16_BYTE( "epr-10986a.a4", 0x000000, 0x8000, CRC(3e136215) SHA1(e90b55d03c515752cc2c647cb30f6f23a0a91b01) ) /* .a3?? */
	ROM_LOAD16_BYTE( "epr-10984a.a1", 0x000001, 0x8000, CRC(44bf3cf5) SHA1(fd7bcb25fbbfb01da49892c8b39c1f98e5acf9e6) )
	ROM_LOAD16_BYTE( "epr-10987a.a5", 0x010000, 0x8000, CRC(cfd79404) SHA1(5dec1d31cb0fe14a0dbe00df13322f8e2676774b) ) /* .a4?? */
	ROM_LOAD16_BYTE( "epr-10985a.a2", 0x010001, 0x8000, CRC(1c21a03f) SHA1(61d99ba59be2d97a91fa0e02c4a377e880cfbe13) )
	ROM_LOAD16_BYTE( "epr-10829.a6",  0x020000, 0x8000, CRC(a431ab08) SHA1(95888af6fae598c40e7fefffbfd1f0b551e9f1be) )
	ROM_LOAD16_BYTE( "epr-10826.a3",  0x020001, 0x8000, CRC(2ed8e4b7) SHA1(23da16e29a475d6ec7ccec8cdd18a1dc78ae69cd) )

	ROM_REGION( 0x30000, REGION_GFX1, 0 ) /* tiles */
	ROM_LOAD( "epr-10772.b9",  0x00000, 0x10000, CRC(182b6301) SHA1(bb6f1174f82611c801d2b4b7d3596bf71619e8a1) )
	ROM_LOAD( "epr-10773.b10", 0x10000, 0x10000, CRC(8f7129a2) SHA1(094a4065597d8d51fb2232546df1de9043fea731) )
	ROM_LOAD( "epr-10774.b11", 0x20000, 0x10000, CRC(4409411f) SHA1(84fd7128e8440d96b0384ae3c391a59bd37ecf9d) )

	ROM_REGION16_BE( 0x60000, REGION_GFX2, 0 ) /* sprites */
	ROM_LOAD16_BYTE( "10760.b1", 0x00001, 0x010000, CRC(70de327b) SHA1(11dde9cefd993f5fb02baf5809fae6c1176a58a1) )
	ROM_LOAD16_BYTE( "10763.b5", 0x00000, 0x010000, CRC(99ec5cb5) SHA1(933a2216a2c772fc82499c739457865b1c75cdb8) )
	ROM_LOAD16_BYTE( "10761.b2", 0x20001, 0x010000, CRC(4e80f80d) SHA1(d168235bdf09317545c999676a4adf015df32366) )
	ROM_LOAD16_BYTE( "10764.b6", 0x20000, 0x010000, CRC(602da5d5) SHA1(d32cdde7d86c4561e7bfa547d7d7995ce9a43c24) )
	ROM_LOAD16_BYTE( "10762.b3", 0x40001, 0x010000, CRC(464b5f78) SHA1(b730964a54e6a63fa5b7cc2cbf9ec0ab650626d5) )
	ROM_LOAD16_BYTE( "10765.b7", 0x40000, 0x010000, CRC(0a73a057) SHA1(7f31124c67541a245e069e5b6aac59935d99a9a9) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* sound CPU */
	ROM_LOAD( "10775.a7", 0x0000, 0x8000, CRC(4cbd55a8) SHA1(8af2c52ab61338c8a9f1a74a05470dd3d5e0c42f) )

	ROM_REGION( 0x2000, REGION_USER2, 0 ) /* decryption key */
	ROM_LOAD( "317-0028.key", 0x0000, 0x2000, BAD_DUMP CRC(1514662f) SHA1(d6241cb2a70c5b0442ecc2f8a10307608e5a0870) )
ROM_END

/*************************************************************************************************************************
  Sukeban Jansi Ryuko (JPN Ver.)
  CPU: FD1089B 317-5021 (16A/16B) (version uses i8751(317-5019) known to be exist)
  ROM Board type: 171-???

  (c)1988 White Board

  Sega System 16A/16B

  IC61:   839-0068 (16A)
  IC69:   315-5150 (16A)
*/
ROM_START( sjryuko )
	ROM_REGION( 0x20000, REGION_CPU1, 0 ) /* 68000 code */
	ROM_LOAD16_BYTE( "epr-12256.a4",  0x000000, 0x08000, CRC(5987ee1b) SHA1(70a4e8603491d60a687c10980db02e60f4239779) )
	ROM_LOAD16_BYTE( "epr-12253.a1",  0x000001, 0x08000, CRC(26a822df) SHA1(2ec21246f7bc1d4a25ec308853a6543805036df3) )
	ROM_LOAD16_BYTE( "epr-12257.a5",  0x010000, 0x08000, CRC(3a2acc3f) SHA1(8776c37b8092bece6928e68a86ed8f6cfbd0d5cf) )
	ROM_LOAD16_BYTE( "epr-12254.a2",  0x010001, 0x08000, CRC(7e908217) SHA1(509962c45dda7423ad081acdcac9ffa10807840a) )

	ROM_REGION( 0x18000, REGION_GFX1, 0 ) /* tiles */
	ROM_LOAD( "epr-12224-95.b9",  0x00000, 0x08000, CRC(eac17ba1) SHA1(6dfea3383b7c9c47bc0943a8d86fc89efcb85ae2) )
	ROM_LOAD( "epr-12225-94.b10", 0x08000, 0x08000, CRC(2310fc98) SHA1(c40ca62edbe5cfa2f84811426233412cd5bd398c) )
	ROM_LOAD( "epr-12226-93.b11", 0x10000, 0x08000, CRC(210e6999) SHA1(5707cc613060b0070a822850b9afab8293f64dd7) )
	/*(epr xxxxx - S16a location . S16b location */

	ROM_REGION16_BE( 0x80000, REGION_GFX2, ROMREGION_ERASE00 ) /* sprites */
	ROM_LOAD16_BYTE( "epr-12232-10.b1", 0x00001, 0x010000, CRC(0adec62b) SHA1(cd798a7994cea73bffe78feac4e692d755074b1d) )
	ROM_LOAD16_BYTE( "epr-12236-11.b5", 0x00000, 0x010000, CRC(286b9af8) SHA1(085251b8ce8b7fadf15b8ebd5872f0337adf142b) )
	ROM_LOAD16_BYTE( "epr-12233-17.b2", 0x20001, 0x010000, CRC(3e45969c) SHA1(804f3714c97877c6f0caf458f8af38e8d8179d73) )
	ROM_LOAD16_BYTE( "epr-12237-18.b6", 0x20000, 0x010000, CRC(e5058e96) SHA1(4a1f663c7c87fe7177a52017da3f2f55568bd863) )
	ROM_LOAD16_BYTE( "epr-12234-23.b3", 0x40001, 0x010000, CRC(8c8d54ef) SHA1(a8adee4f6ad8079af88cf471af42ace8ac8d093e) )
	ROM_LOAD16_BYTE( "epr-12238-24.b7", 0x40000, 0x010000, CRC(7ada3304) SHA1(e402442e73d93a1b174e3fcab6a97fb2d450994c) )
	ROM_LOAD16_BYTE( "epr-12235-29.b4", 0x60001, 0x010000, CRC(fa45d511) SHA1(41e343b039e8633b2469a5eaf5e4196b682f0d01) )
	ROM_LOAD16_BYTE( "epr-12239-30.b8", 0x60000, 0x010000, CRC(91f70c8b) SHA1(c3ac9cf248540d948f7845eb17ec95e1be8d00bb) )
	/*(EPR xxxxx - S16a location . S16b location */

	ROM_REGION( 0x50000, REGION_CPU2, 0 ) /* sound CPU */
	ROM_LOAD( "epr-12244.a7", 0x00000, 0x8000, CRC(cb2a47e5) SHA1(d7569a78522ec9104da7586ae7d17837599f92ee) )
	ROM_LOAD( "epr-12245.a8", 0x10000, 0x8000, CRC(66164134) SHA1(ef9dc24ee0817ffd16416243190f29dc80309966) )
	ROM_LOAD( "epr-12246.a9", 0x20000, 0x8000, CRC(f1242582) SHA1(f5734b31b449f3f09a0cacc712059644eedee006) )
	ROM_LOAD( "epr-12247.a10",0x30000, 0x8000, CRC(ef8a64c6) SHA1(525e0d968c72c6dd73df69b55b8626495d154649) )
	ROM_LOAD( "epr-12248.a11",0x40000, 0x8000, CRC(d1eabdab) SHA1(f255a66e082353768e8d2bb574e883a4a45f7670) )

	ROM_REGION( 0x2000, REGION_USER2, 0 ) /* decryption key */
	ROM_LOAD( "317-5021.key", 0x0000, 0x2000, CRC(8e40b2ab) SHA1(f3e2d70a17ac5270bec586cc67b2b8ba14bf53cf) )
ROM_END


/***************************************************************************/

static INTERRUPT_GEN( sys16_interrupt )
{
	if(sys16_custom_irq) sys16_custom_irq();
	cpu_set_irq_line(0, 4, HOLD_LINE); /* Interrupt vector 4, used by VBlank */
}

static MACHINE_DRIVER_START( system16b )

	/* basic machine hardware */
	MDRV_CPU_ADD_TAG("main", M68000, 10000000)
	MDRV_CPU_VBLANK_INT(sys16_interrupt,1)

	MDRV_CPU_ADD_TAG("sound", Z80, 5000000)
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
		MDRV_CPU_MEMORY(sound_readmem,sound_writemem)
	MDRV_CPU_PORTS(sound_readport,sound_writeport)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(1000000 * (262 - 224) / (262 * 60))

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(40*8, 28*8)
	MDRV_VISIBLE_AREA(0*8, 40*8-1, 0*8, 28*8-1)
	MDRV_GFXDECODE(sys16_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(2048*3)

	MDRV_VIDEO_START (system16b)
	MDRV_VIDEO_UPDATE(system16b)

	/* sound hardware */
	MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
	MDRV_SOUND_ADD_TAG("2151", YM2151, sys16_ym2151_interface)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( system16_7759 )
	/* basic machine hardware */
	MDRV_IMPORT_FROM(system16b)

	MDRV_CPU_MODIFY("sound")
		MDRV_CPU_MEMORY(sound_readmem_7759,sound_writemem_7759)
	MDRV_CPU_PORTS(sound_readport,sound_writeport_7759)

	/* sound hardware */
	MDRV_SOUND_ADD_TAG("7759", UPD7759, sys16b_upd7759_interface)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( system16_7751 )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(system16b)

	MDRV_CPU_MODIFY("sound")
	MDRV_CPU_MEMORY(sound_readmem_7751,sound_writemem)
	MDRV_CPU_PORTS(sound_readport_7751,sound_writeport_7751)

	MDRV_CPU_ADD(N7751, 6000000/15)
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
	MDRV_CPU_MEMORY(readmem_7751,writemem_7751)
	MDRV_CPU_PORTS(readport_7751,writeport_7751)

	/* sound hardware */
	MDRV_SOUND_ADD(DAC, sys16_7751_dac_interface)
MACHINE_DRIVER_END

/***************************************************************************/

static const UINT8 default_banklist[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 };
static const UINT8 alternate_banklist[] = { 255,255,255,255, 255,255,255,3, 255,255,255,2, 255,1,0,255 };

static MACHINE_INIT( generic_5704 )
{
	int i;
	segaic16_tilemap_reset(0);

	for (i = 0; i < 16; i++)
		segaic16_sprites_set_bank(0, i, default_banklist[i]);

	sys16_soundbanktype=2;
	disable_screen_blanking = 0;
}

static MACHINE_INIT( generic_5358 )
{
	int i;
	segaic16_tilemap_reset(0);

	for (i = 0; i < 16; i++)
		segaic16_sprites_set_bank(0, i, alternate_banklist[i]);

	sys16_soundbanktype=1;
	disable_screen_blanking = 0;
}

static DRIVER_INIT( FD1089A )
{
	fd1089a_decrypt();
}

static DRIVER_INIT( FD1089B )
{
	fd1089b_decrypt();
}

static DRIVER_INIT( dunkshot )
{
	custom_io_r = dunkshot_custom_io_r;
	fd1089a_decrypt();
}

static DRIVER_INIT( sdi )
{
	custom_io_r = sdi_custom_io_r;
	fd1089a_decrypt();
}

static DRIVER_INIT( sjryuko )
{
	custom_io_r = sjryuko_custom_io_r;
	custom_io_w = sjryuko_custom_io_w;
	fd1089b_decrypt();
}

static MACHINE_DRIVER_START( aurail )
	/* basic machine hardware */
	MDRV_IMPORT_FROM(system16_7759)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_MEMORY(aurail_readmem,aurail_writemem)
	MDRV_MACHINE_INIT(generic_5704)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( bullet )
	/* basic machine hardware */
	MDRV_IMPORT_FROM(system16_7759)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_MEMORY(bullet_readmem,bullet_writemem)
	MDRV_MACHINE_INIT(generic_5358)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( cotton )
	/* basic machine hardware */
	MDRV_IMPORT_FROM(system16_7759)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_MEMORY(cotton_readmem,cotton_writemem)
	MDRV_MACHINE_INIT(generic_5704)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( dunkshot )
	/* basic machine hardware */
	MDRV_IMPORT_FROM(system16_7759)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_MEMORY(dunkshot_readmem,dunkshot_writemem)
	MDRV_VIDEO_START(timscanr)
	MDRV_MACHINE_INIT(generic_5358)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( fantzn2x )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(system16_7759)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_MEMORY(fantzn2x_readmem,fantzn2x_writemem)
	MDRV_MACHINE_INIT(generic_5704)
MACHINE_DRIVER_END

/* init notes

   on boot order
   DRIVER_INIT
   MACHINE_INIT
   reset only MACHINE_INIT is called
*/

GAMEX( 1988, sjryuko,  sdi,      bullet,    aurail,    sjryuko,  ROT0,   "White Board",     "Sukeban Jansi Ryuko (set 2, System 16B, FD1089B 317-5021)",  GAME_NOT_WORKING )
/*all  games boot and work fine just need inputs done will consider doing it if someone that is japanese requests it */

/*          rom       parent     machine    inp        init */
GAME( 1987, dunkshot, 0,        dunkshot,  dunkshot,  dunkshot, ROT0,   "Sega",            "Dunk Shot (Rev C, FD1089A 317-0022)" )
GAME( 1987, dunkshota,dunkshot, dunkshot,  dunkshot,  dunkshot, ROT0,   "Sega",            "Dunk Shot (Rev A, FD1089A 317-0022)" )
GAME( 1987, dunkshoto,dunkshot, dunkshot,  dunkshot,  dunkshot, ROT0,   "Sega",            "Dunk Shot (FD1089A 317-0022)" )

GAME( 1987, defense,  sdi,      bullet,    sdi,       sdi,      ROT0,   "Sega",            "Defense (System 16B, FD1089A 317-0028)" )
GAME( 1987, sdib,     sdi,      bullet,    sdi,       sdi,      ROT0,   "Sega",            "SDI - Strategic Defense Initiative (System 16B, FD1089A 317-0028)" )

GAME( 1990, aurail,   0,        aurail,    aurail,    0,        ROT0,   "Sega / Westone",  "Aurail (set 3, US) (unprotected)" )
GAME( 1990, aurail1,  aurail,   aurail,    aurail,    FD1089B,  ROT0,   "Sega / Westone",  "Aurail (set 2, World) (FD1089B 317-0168)" )
GAME( 1990, aurail1d, aurail,   aurail,    aurail,    0,        ROT0,   "Sega / Westone",  "Aurail (set 2, World) (unprotected of FD1089B 317-0168 set)" )
GAME( 1990, aurailj,  aurail,   aurail,    aurail,    FD1089A,  ROT0,   "Sega / Westone",  "Aurail (set 1, Japan) (FD1089A 317-0167)" )
GAME( 1990, aurailjd, aurail,   aurail,    aurail,    0,        ROT0,   "Sega / Westone",  "Aurail (set 1, Japan) (unprotected of FD1089A 317-0167 set)" )

GAME( 1991, cottond,  cotton,   cotton,    cotton,    0,        ROT0,   "Sega / Success",  "Cotton (set 4, World) (unprotected of FD1094 317-0181a set)" )

GAME( 1987, bulletd,  0,        bullet,    bullet,    0,        ROT0,   "Sega",            "Bullet (unprotected of FD1094 317-0041 set)" )

GAME( 2008, fantzn2x, 0,        fantzn2x,  fantzn2x,  0,        ROT0,   "Sega / M2",       "Fantasy Zone II - The Tears of Opa-Opa (System 16C version)" )

