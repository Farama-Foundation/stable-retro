/***************************************************************************

Free Kick  - (c) 1987 Sega / Nihon System (made by Nihon, licensed to Sega)

Driver by Tomasz Slanina  dox@space.pl
based on initial work made by David Haywood

Notes:
- Quite interestingly, Free Kick's sound ROM contains a Z80 program, but
  there isn't a sound CPU and that program isn't executed. Instead, the main
  CPU reads the sound program through an 8255 PPI and plays sounds directly.

TODO:
- Gigas cocktail mode / flipscreen

****************************************************************************

 currently only the freekick bootleg roms are included
 the fk bootleg roms are the same as one of the other sets + an extra
 64k ram dump from protection device.

 main program rom is unused, is it a dummy or just something to active the
 protection device?

 we don't have the game rom data / program from inside the counter run cpu,
 hopefully somebody can work out how to dump it before all the boards die
 since it appears to be battery backed

 ---

 $78d        - checksum calculations
 $d290,$d291 - level (color set , level number  - BCD)
 $d292       - lives

 To enter Test Mode - press Button1 durning RESET (code at $79d)

*/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "machine/8255ppi.h"


extern data8_t *freek_videoram;

VIDEO_START(freekick);
VIDEO_UPDATE(gigas);
VIDEO_UPDATE(pbillrd);
VIDEO_UPDATE(freekick);
WRITE_HANDLER( freek_videoram_w );

static int oigas_inval,oigas_outval,oigas_cnt;/*oigas*/
static int romaddr;

static WRITE_HANDLER( snd_rom_addr_l_w )
{
	romaddr = (romaddr & 0xff00) | data;
}

static WRITE_HANDLER( snd_rom_addr_h_w )
{
	romaddr = (romaddr & 0x00ff) | (data << 8);
}

static READ_HANDLER( snd_rom_r )
{
	return memory_region(REGION_USER1)[romaddr & 0x7fff];
}

static ppi8255_interface ppi8255_intf =
{
	2, 										/* 1 chips */
	{ NULL,             input_port_0_r },	/* Port A read */
	{ NULL,             input_port_1_r },	/* Port B read */
	{ snd_rom_r,        input_port_2_r },	/* Port C read */
	{ snd_rom_addr_l_w, NULL },				/* Port A write */
	{ snd_rom_addr_h_w, NULL },				/* Port B write */
	{ NULL,             NULL },				/* Port C write */
};

MACHINE_INIT( freekckb )
{
	ppi8255_init(&ppi8255_intf);
}


static WRITE_HANDLER( flipscreen_w )
{
	/* flip Y/X could be the other way round... */
	if (offset)
		flip_screen_y_set(~data & 1);
	else
		flip_screen_x_set(~data & 1);
}

static WRITE_HANDLER( coin_w )
{
	coin_counter_w(offset,~data & 1);
}


static int spinner;

static WRITE_HANDLER( spinner_select_w )
{
	spinner = data & 1;
}

static READ_HANDLER( spinner_r )
{
	return readinputport(5 + spinner);
}

static READ_HANDLER( gigas_spinner_r )
{
	return readinputport( spinner );
}



static WRITE_HANDLER( pbillrd_bankswitch_w )
{
	cpu_setbank(1,memory_region(REGION_CPU1) + 0x10000 + 0x4000 * (data & 1));
}


static int nmi_en;

static WRITE_HANDLER( nmi_enable_w )
{
	nmi_en = data & 1;
}

static INTERRUPT_GEN( freekick_irqgen )
{
	if (nmi_en) cpu_set_irq_line(0,IRQ_LINE_NMI,PULSE_LINE);
}

static MEMORY_READ_START( gigas_readmem )
	{ 0x0000, 0xbfff, MRA_ROM },
	{ 0xc000, 0xdfff, MRA_RAM },
	{ 0xe000, 0xe000, input_port_2_r },
	{ 0xe800, 0xe800, input_port_3_r },
	{ 0xf000, 0xf000, input_port_4_r },
	{ 0xf800, 0xf800, input_port_5_r },
MEMORY_END

static MEMORY_WRITE_START( gigas_writemem )
	{ 0x0000, 0xbfff, MWA_ROM },
	{ 0xc000, 0xcfff, MWA_RAM },
	{ 0xd000, 0xd7ff, freek_videoram_w, &freek_videoram },
	{ 0xd800, 0xd8ff, MWA_RAM, &spriteram, &spriteram_size },
	{ 0xd900, 0xdfff, MWA_RAM },
	{ 0xe000, 0xe001, MWA_NOP },/* probably not flipscreen*/
	{ 0xe002, 0xe003, coin_w },
	{ 0xe004, 0xe004, nmi_enable_w },
	{ 0xe005, 0xe005, MWA_NOP},
	{ 0xf000, 0xf000, MWA_NOP }, /*bankswitch ?*/
	{ 0xfc00, 0xfc00, SN76496_0_w },
	{ 0xfc01, 0xfc01, SN76496_1_w },
	{ 0xfc02, 0xfc02, SN76496_2_w },
	{ 0xfc03, 0xfc03, SN76496_3_w },
MEMORY_END

static PORT_READ_START( gigas_readport )
	{ 0x00, 0x00, gigas_spinner_r },
	{ 0x01, 0x01, MRA_NOP }, /*unused dip 3*/
PORT_END

static PORT_WRITE_START( gigas_writeport )
	{ 0x00, 0x00 ,spinner_select_w },
PORT_END



static WRITE_HANDLER(oigas_5_w)
{
	if(data>0xc0&&data<0xe0)oigas_cnt=1;
	switch(oigas_cnt)
	{
	  case 1: oigas_inval=data<<8;break;
	  case 2: oigas_inval|=data;break;
	}
}

static READ_HANDLER(oigas_3_r)
{
	switch(++oigas_cnt)
	{
	  case 2: return ~(oigas_inval>>8);
	  case 3: return ~(oigas_inval&0xff);
	  case 4: switch(oigas_inval)
	  {
	    case 0xc500: oigas_outval=0x17ef;break;
	    case 0xc520:
	    case 0xc540: oigas_outval=0x19c1;break;
	    case 0xc560: oigas_outval=0x1afc;break;
	    case 0xc580:
	    case 0xc5a0:
	    case 0xc5c0: oigas_outval=0x1f28;break;
	    case 0xc680: oigas_outval=0x2e8a;break;
	    case 0xc5e0:
	    case 0xc600:
	    case 0xc620:
	    case 0xc640:
	    case 0xc660: oigas_outval=0x25cc;break;
	    case 0xc6c0:
	    case 0xc6e0: oigas_outval=0x09d7;break;
	    case 0xc6a0: oigas_outval=0x3168;break;
	    case 0xc720: oigas_outval=0x2207;break;
     	    case 0xc700: oigas_outval=0x0e34;break;
	    case 0xc710: oigas_outval=0x0fdd;break;
	    case 0xc4f0: oigas_outval=0x05b6;break;
	    case 0xc4e0: oigas_outval=0xae1e;break;
	   }
	   return oigas_outval>>8;
	   case 5: oigas_cnt=0;return oigas_outval&0xff;
	}
	return 0;
}

static READ_HANDLER(oigas_2_r)
{
	return 1;
}

static PORT_READ_START( oigas_readport )
	{ 0x00, 0x00, gigas_spinner_r },
	{ 0x01, 0x01, IORP_NOP }, /*unused dip 3*/
	{ 0x02, 0x02, oigas_2_r },
	{ 0x03, 0x03, oigas_3_r },
PORT_END

static PORT_WRITE_START( oigas_writeport )
	{ 0x00, 0x00, spinner_select_w },
	{ 0x05, 0x05, oigas_5_w },
PORT_END




static MEMORY_READ_START( pbillrd_readmem )
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0xbfff, MRA_BANK1 },
	{ 0xc000, 0xdfff, MRA_RAM },
	{ 0xe000, 0xe000, input_port_0_r },
	{ 0xe800, 0xe800, input_port_1_r },
	{ 0xf000, 0xf000, input_port_2_r },
	{ 0xf800, 0xf800, input_port_3_r },
MEMORY_END

static MEMORY_WRITE_START( pbillrd_writemem )
	{ 0x0000, 0xbfff, MWA_ROM },
	{ 0xc000, 0xcfff, MWA_RAM },
	{ 0xd000, 0xd7ff, freek_videoram_w, &freek_videoram },
	{ 0xd800, 0xd8ff, MWA_RAM, &spriteram, &spriteram_size },
	{ 0xd900, 0xdfff, MWA_RAM },
	{ 0xe000, 0xe001, flipscreen_w },
	{ 0xe002, 0xe003, coin_w },
	{ 0xe004, 0xe004, nmi_enable_w },
	{ 0xf000, 0xf000, pbillrd_bankswitch_w },
	{ 0xfc00, 0xfc00, SN76496_0_w },
	{ 0xfc01, 0xfc01, SN76496_1_w },
	{ 0xfc02, 0xfc02, SN76496_2_w },
	{ 0xfc03, 0xfc03, SN76496_3_w },
MEMORY_END

static MEMORY_READ_START( freekckb_readmem )
	{ 0x0000, 0xcfff, MRA_ROM },
	{ 0xd000, 0xdfff, MRA_RAM },
	{ 0xe000, 0xe7ff, MRA_RAM },	/* tilemap*/
	{ 0xe800, 0xe8ff, MRA_RAM },	/* sprites*/
	{ 0xec00, 0xec03, ppi8255_0_r },
	{ 0xf000, 0xf003, ppi8255_1_r },
	{ 0xf800, 0xf800, input_port_3_r },
	{ 0xf801, 0xf801, input_port_4_r },
	{ 0xf802, 0xf802, MRA_NOP },	/*MUST return bit 0 = 0, otherwise game resets*/
	{ 0xf803, 0xf803, spinner_r },
MEMORY_END

static MEMORY_WRITE_START( freekckb_writemem )
	{ 0x0000, 0xcfff, MWA_ROM },
	{ 0xd000, 0xdfff, MWA_RAM },
	{ 0xe000, 0xe7ff, freek_videoram_w, &freek_videoram },
	{ 0xe800, 0xe8ff, MWA_RAM, &spriteram, &spriteram_size },
	{ 0xec00, 0xec03, ppi8255_0_w },
	{ 0xf000, 0xf003, ppi8255_1_w },
	{ 0xf800, 0xf801, flipscreen_w },
	{ 0xf802, 0xf803, coin_w },
	{ 0xf804, 0xf804, nmi_enable_w },
	{ 0xf806, 0xf806, spinner_select_w },
	{ 0xfc00, 0xfc00, SN76496_0_w },
	{ 0xfc01, 0xfc01, SN76496_1_w },
	{ 0xfc02, 0xfc02, SN76496_2_w },
	{ 0xfc03, 0xfc03, SN76496_3_w },
MEMORY_END


static int ff_data;

static READ_HANDLER (freekick_ff_r)
{
	return ff_data;
}

static WRITE_HANDLER (freekick_ff_w)
{
	ff_data = data;
}

static PORT_READ_START( freekckb_readport )
	{ 0xff, 0xff, freekick_ff_r },
PORT_END

static PORT_WRITE_START( freekckb_writeport )
	{ 0xff, 0xff, freekick_ff_w },
PORT_END


INPUT_PORTS_START( gigas )

	PORT_START
	PORT_ANALOG( 0xff, 0x00, IPT_DIAL | IPF_REVERSE, 30, 15, 0, 0 )

	PORT_START
	PORT_ANALOG( 0xff, 0x00, IPT_DIAL | IPF_REVERSE | IPF_COCKTAIL, 30, 15, 0, 0 )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN1 )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN2 )

	PORT_START
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x01, "3" )
	PORT_DIPSETTING(    0x00, "5" )
	PORT_DIPNAME( 0x06, 0x06, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x06, "20000, 60000, Every 60000" )
	PORT_DIPSETTING(    0x02, "20000, 60000" )
	PORT_DIPSETTING(    0x04, "30000, 80000, Every 80000" )
	PORT_DIPSETTING(    0x00, "20000" )
	PORT_DIPNAME( 0x18, 0x18, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x18, "Easy" )    /* level 1 */
	PORT_DIPSETTING(    0x10, "Normal" )
	PORT_DIPSETTING(    0x08, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" ) /* level 4 */
  PORT_DIPNAME( 0x20, 0x20, "Allow Continue" )
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START
	PORT_DIPNAME( 0x0f, 0x0f, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x0e, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x05, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 3C_2C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 4C_3C ) )
	PORT_DIPSETTING(    0x0f, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 4C_5C ) )
	PORT_DIPSETTING(    0x0a, DEF_STR( 3C_4C ) )
	PORT_DIPSETTING(    0x09, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x02, "3 Coins/5 Credits" )
	PORT_DIPSETTING(    0x07, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 2C_5C ) )
	PORT_DIPSETTING(    0x0b, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x0d, DEF_STR( 1C_5C ) )
	PORT_DIPNAME( 0xf0, 0xf0, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0xe0, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x50, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x60, DEF_STR( 3C_2C ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 4C_3C ) )
	PORT_DIPSETTING(    0xf0, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 4C_5C ) )
	PORT_DIPSETTING(    0xa0, DEF_STR( 3C_4C ) )
	PORT_DIPSETTING(    0x90, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x20, "3 Coins/5 Credits" )
	PORT_DIPSETTING(    0x70, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 2C_5C ) )
	PORT_DIPSETTING(    0xb0, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0xd0, DEF_STR( 1C_5C ) )


INPUT_PORTS_END


INPUT_PORTS_START( gigasm2 )

	PORT_START
	PORT_ANALOG( 0xff, 0x00, IPT_DIAL | IPF_REVERSE, 30, 15, 0, 0 )

	PORT_START
	PORT_ANALOG( 0xff, 0x00, IPT_DIAL | IPF_REVERSE | IPF_COCKTAIL, 30, 15, 0, 0 )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN1 )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN2 )

	PORT_START
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x01, "3" )
	PORT_DIPSETTING(    0x00, "5" )
	PORT_DIPNAME( 0x06, 0x06, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x06, "20000, 60000, Every 60000" )
	PORT_DIPSETTING(    0x02, "20000, 60000" )
	PORT_DIPSETTING(    0x04, "30000, 90000, Every 90000" )
	PORT_DIPSETTING(    0x00, "20000" )
	PORT_DIPNAME( 0x18, 0x18, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x18, "Easy" )    /* level 1 */
	PORT_DIPSETTING(    0x10, "Normal" )
	PORT_DIPSETTING(    0x08, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" ) /* level 4 */
  PORT_DIPNAME( 0x20, 0x20, "Allow Continue" )
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START
	PORT_DIPNAME( 0x0f, 0x0f, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x0e, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x05, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 3C_2C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 4C_3C ) )
	PORT_DIPSETTING(    0x0f, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 4C_5C ) )
	PORT_DIPSETTING(    0x0a, DEF_STR( 3C_4C ) )
	PORT_DIPSETTING(    0x09, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x02, "3 Coins/5 Credits" )
	PORT_DIPSETTING(    0x07, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 2C_5C ) )
	PORT_DIPSETTING(    0x0b, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x0d, DEF_STR( 1C_5C ) )
	PORT_DIPNAME( 0xf0, 0xf0, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0xe0, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x50, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x60, DEF_STR( 3C_2C ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 4C_3C ) )
	PORT_DIPSETTING(    0xf0, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 4C_5C ) )
	PORT_DIPSETTING(    0xa0, DEF_STR( 3C_4C ) )
	PORT_DIPSETTING(    0x90, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x20, "3 Coins/5 Credits" )
	PORT_DIPSETTING(    0x70, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 2C_5C ) )
	PORT_DIPSETTING(    0xb0, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0xd0, DEF_STR( 1C_5C ) )



INPUT_PORTS_END

INPUT_PORTS_START( pbillrd )
	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN1 )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN2 )

	PORT_START
	PORT_DIPNAME( 0x01, 0x01, "Balls" )
	PORT_DIPSETTING(    0x01, "3" )
	PORT_DIPSETTING(    0x00, "5" )
	PORT_DIPNAME( 0x06, 0x06, "Bonus Ball" )
	PORT_DIPSETTING(    0x06, "10000, 30000, 50000"  )
	PORT_DIPSETTING(    0x02, "20000, 60000" )
	PORT_DIPSETTING(    0x04, "30000, 80000" )
	PORT_DIPSETTING(    0x00, "20000" )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, "Allow continue" )
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x20, 0x00, "Shot" )
	PORT_DIPSETTING(    0x00, "2" )
	PORT_DIPSETTING(    0x20, "3" )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x80, DEF_STR( On ) )

	PORT_START
	PORT_DIPNAME( 0x0f, 0x0f, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x0e, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x05, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 3C_2C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 4C_3C ) )
	PORT_DIPSETTING(    0x0f, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 4C_5C ) )
	PORT_DIPSETTING(    0x0a, DEF_STR( 3C_4C ) )
	PORT_DIPSETTING(    0x09, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x02, "3 Coins/5 Credits" )
	PORT_DIPSETTING(    0x07, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 2C_5C ) )
	PORT_DIPSETTING(    0x0b, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x0d, DEF_STR( 1C_5C ) )
	PORT_DIPNAME( 0xf0, 0xf0, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0xe0, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x50, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x60, DEF_STR( 3C_2C ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 4C_3C ) )
	PORT_DIPSETTING(    0xf0, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 4C_5C ) )
	PORT_DIPSETTING(    0xa0, DEF_STR( 3C_4C ) )
	PORT_DIPSETTING(    0x90, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x20, "3 Coins/5 Credits" )
	PORT_DIPSETTING(    0x70, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 2C_5C ) )
	PORT_DIPSETTING(    0xb0, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0xd0, DEF_STR( 1C_5C ) )
INPUT_PORTS_END

INPUT_PORTS_START( freekckb )
	PORT_START
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x01, "3" )
	PORT_DIPSETTING(    0x00, "5" )
	PORT_DIPNAME( 0x06, 0x06, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x06, "2-3-4-5-60000 Points" )
	PORT_DIPSETTING(    0x02, "3-4-5-6-7-80000 Points" )
	PORT_DIPSETTING(    0x04, "20000 & 60000 Points" )
	PORT_DIPSETTING(    0x00, "ONLY 20000 Points" )
	PORT_DIPNAME( 0x18, 0x18, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x18, "Easy" )    /* level 1 */
	PORT_DIPSETTING(    0x10, "Normal" )
	PORT_DIPSETTING(    0x08, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" ) /* level 4 */
    PORT_DIPNAME( 0x20, 0x20, "Allow Continue" )
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START
	PORT_DIPNAME( 0x0f, 0x0f, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x0e, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x05, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 3C_2C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 4C_3C ) )
	PORT_DIPSETTING(    0x0f, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 4C_5C ) )
	PORT_DIPSETTING(    0x0a, DEF_STR( 3C_4C ) )
	PORT_DIPSETTING(    0x09, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x02, "3 Coins/5 Credits" )
	PORT_DIPSETTING(    0x07, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 2C_5C ) )
	PORT_DIPSETTING(    0x0b, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x0d, DEF_STR( 1C_5C ) )
	PORT_DIPNAME( 0xf0, 0xf0, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(    0xe0, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x50, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x60, DEF_STR( 3C_2C ) )
	PORT_DIPSETTING(    0xf0, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0xa0, DEF_STR( 3C_4C ) )
	PORT_DIPSETTING(    0x90, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x20, "3 Coins/5 Credits" )
	PORT_DIPSETTING(    0x70, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 2C_5C ) )
	PORT_DIPSETTING(    0xb0, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0xd0, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0xc0, "1 Coin/10 Credits" )
	PORT_DIPSETTING(    0x40, "1 Coin/25 Credits" )
	PORT_DIPSETTING(    0x80, "1 Coin/50 Credits" )

	PORT_START
	PORT_DIPNAME( 0x01, 0x00, "Manufacturer" )
	PORT_DIPSETTING(    0x00, "Nihon System" )
	PORT_DIPSETTING(    0x01, "Sega/Nihon System" )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, "Coin Slots" )
	PORT_DIPSETTING(    0x00, "1" )
	PORT_DIPSETTING(    0x80, "2" )

	PORT_START
    PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN1 )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN2 )

	PORT_START
	PORT_ANALOG( 0xff, 0x00, IPT_DIAL | IPF_REVERSE, 30, 15, 0, 0 )

	PORT_START
	PORT_ANALOG( 0xff, 0x00, IPT_DIAL | IPF_REVERSE | IPF_COCKTAIL, 30, 15, 0, 0 )
INPUT_PORTS_END

INPUT_PORTS_START( countrun )
	PORT_START
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x01, "3" )
	PORT_DIPSETTING(    0x00, "2" )
	PORT_DIPNAME( 0x06, 0x06, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x06, "20000, 60000 and every 60000 Points" )
	PORT_DIPSETTING(    0x02, "30000, 80000 and every 80000 Points" )
	PORT_DIPSETTING(    0x04, "20000 & 60000 Points" )
	PORT_DIPSETTING(    0x00, "ONLY 20000 Points" )
	PORT_DIPNAME( 0x18, 0x18, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x18, "Easy" )    /* level 1 */
	PORT_DIPSETTING(    0x10, "Normal" )
	PORT_DIPSETTING(    0x08, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" ) /* level 4 */
        PORT_DIPNAME( 0x20, 0x20, "Allow_Continue" )
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START
	PORT_DIPNAME( 0x0f, 0x0f, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x0e, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x05, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 3C_2C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 4C_3C ) )
	PORT_DIPSETTING(    0x0f, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 4C_5C ) )
	PORT_DIPSETTING(    0x0a, DEF_STR( 3C_4C ) )
	PORT_DIPSETTING(    0x09, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x02, "3 Coins/5 Credits" )
	PORT_DIPSETTING(    0x07, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 2C_5C ) )
	PORT_DIPSETTING(    0x0b, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x0d, DEF_STR( 1C_5C ) )
	PORT_DIPNAME( 0xf0, 0xf0, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(    0xe0, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x50, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x60, DEF_STR( 3C_2C ) )
	PORT_DIPSETTING(    0xf0, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0xa0, DEF_STR( 3C_4C ) )
	PORT_DIPSETTING(    0x90, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x20, "3 Coins/5 Credits" )
	PORT_DIPSETTING(    0x70, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 2C_5C ) )
	PORT_DIPSETTING(    0xb0, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0xd0, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0xc0, "1 Coin/10 Credits" )
	PORT_DIPSETTING(    0x40, "1 Coin/25 Credits" )
	PORT_DIPSETTING(    0x80, "1 Coin/50 Credits" )

	PORT_START
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, "Coin Slots" )
	PORT_DIPSETTING(    0x00, "1" )
	PORT_DIPSETTING(    0x80, "2" )

        PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN1 )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN2 )
	
	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN1 )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN2 )

	PORT_START
	PORT_ANALOG( 0xff, 0x00, IPT_DIAL | IPF_REVERSE, 30, 15, 0, 0 )

	PORT_START
	PORT_ANALOG( 0xff, 0x00, IPT_DIAL | IPF_REVERSE | IPF_COCKTAIL, 30, 15, 0, 0 )
INPUT_PORTS_END


static struct GfxLayout charlayout =
{
	8,8,
	RGN_FRAC(1,3),
	3,
	{ RGN_FRAC(2,3), RGN_FRAC(1,3), RGN_FRAC(0,3) },
	{ 0,1,2,3, 4,5,6,7 },
	{ 0*8,1*8,2*8,3*8,4*8,5*8,6*8,7*8 },
	8*8
};

static struct GfxLayout spritelayout =
{
	16,16,
	RGN_FRAC(1,3),
	3,
	{ RGN_FRAC(0,3),RGN_FRAC(2,3),RGN_FRAC(1,3) },
	{ 0, 1, 2, 3, 4, 5, 6, 7,
	128+0,128+1,128+2,128+3,128+4,128+5,128+6,128+7
	},
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
	  8*8, 9*8, 10*8, 11*8,12*8,13*8,14*8,15*8
	},
	16*16
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &charlayout,   0x000, 32 },
	{ REGION_GFX2, 0, &spritelayout, 0x100, 32 },
	{ -1 }
};



static struct SN76496interface sn76496_interface =
{
	4,	/* 4 chips */
	{ 12000000/4, 12000000/4, 12000000/4, 12000000/4 },	/* 3 MHz??? */
	{ 50, 50, 50, 50 }
};



static MACHINE_DRIVER_START( pbillrd )
	MDRV_CPU_ADD(Z80, 12000000/2)	/* 6 MHz? */
	MDRV_CPU_MEMORY(pbillrd_readmem,pbillrd_writemem)
	MDRV_CPU_PERIODIC_INT(irq0_line_pulse,60*3) /*??*/
	MDRV_CPU_VBLANK_INT(freekick_irqgen,1)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_GFXDECODE(gfxdecodeinfo)

	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 2*8, 30*8-1)
	MDRV_PALETTE_LENGTH(0x200)
	MDRV_PALETTE_INIT(RRRR_GGGG_BBBB)

	MDRV_VIDEO_START(freekick)
	MDRV_VIDEO_UPDATE(pbillrd)

	/* sound hardware */
	MDRV_SOUND_ADD(SN76496, sn76496_interface)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( freekckb )
	MDRV_CPU_ADD(Z80, 12000000/2)	/* 6 MHz? */
	MDRV_CPU_MEMORY(freekckb_readmem,freekckb_writemem)
	MDRV_CPU_PORTS(freekckb_readport,freekckb_writeport)
	MDRV_CPU_PERIODIC_INT(irq0_line_pulse,60*3) /*??*/
	MDRV_CPU_VBLANK_INT(freekick_irqgen,1)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)
	MDRV_MACHINE_INIT(freekckb)

	MDRV_GFXDECODE(gfxdecodeinfo)

	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 2*8, 30*8-1)
	MDRV_PALETTE_LENGTH(0x200)
	MDRV_PALETTE_INIT(RRRR_GGGG_BBBB)

	MDRV_VIDEO_START(freekick)
	MDRV_VIDEO_UPDATE(freekick)

	/* sound hardware */
	MDRV_SOUND_ADD(SN76496, sn76496_interface)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( gigas )
	MDRV_CPU_ADD_TAG("main",Z80, 18432000/6)	/*confirmed*/
	MDRV_CPU_MEMORY(gigas_readmem,gigas_writemem)
	MDRV_CPU_PORTS(gigas_readport,gigas_writeport)
	MDRV_CPU_PERIODIC_INT(irq0_line_pulse,60*3)
	MDRV_CPU_VBLANK_INT(freekick_irqgen,1)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_GFXDECODE(gfxdecodeinfo)

	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 2*8, 30*8-1)
	MDRV_PALETTE_LENGTH(0x200)
	MDRV_PALETTE_INIT(RRRR_GGGG_BBBB)

	MDRV_VIDEO_START(freekick)
	MDRV_VIDEO_UPDATE(gigas)

	/* sound hardware */
	MDRV_SOUND_ADD(SN76496, sn76496_interface)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( oigas )
	MDRV_IMPORT_FROM(gigas)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_PORTS(oigas_readport,oigas_writeport)
MACHINE_DRIVER_END

/* roms */

ROM_START( pbillrd )
	ROM_REGION( 0x18000, REGION_CPU1, 0 ) /* Z80 Code */
	ROM_LOAD( "pb.18",       0x00000, 0x4000, CRC(9e6275ac) SHA1(482e845e7fb4190da483155bd908ad470373cd5c) )
	ROM_LOAD( "pb.7",        0x04000, 0x4000, CRC(dd438431) SHA1(07a950e38b3f627ecf95e5831e5480abb337a010) )
	ROM_CONTINUE(            0x10000, 0x4000 )
	ROM_LOAD( "pb.9",        0x14000, 0x4000, CRC(089ce80a) SHA1(779be9ba2277a26fbebf4acf9e2f5319a934b0f5) )

	ROM_REGION( 0xc000, REGION_GFX1, ROMREGION_DISPOSE ) /* GFX */
	ROM_LOAD( "pb.4",        0x000000, 0x04000, CRC(2f4d4dd3) SHA1(ee4facabf591c235c270db4f4d3f612b8c474e57) )
	ROM_LOAD( "pb.5",        0x004000, 0x04000, CRC(9dfccbd3) SHA1(66ad8882f36630312b488d5d67ae554477574c31) )
	ROM_LOAD( "pb.6",        0x008000, 0x04000, CRC(b5c3f6f6) SHA1(586b47587619a766cf977b74978550aff41a58cc) )

	ROM_REGION( 0x6000, REGION_GFX2, ROMREGION_DISPOSE ) /* GFX */
	ROM_LOAD( "10619.3r",    0x000000, 0x02000, CRC(3296b9d9) SHA1(51393306f74394de96c4097b6244e8eb36114dac) )
	ROM_LOAD( "10620.3n",    0x002000, 0x02000, CRC(ee76b079) SHA1(99abe2c5b1889d20bc3f5720b168690e3979fb2f) )
	ROM_LOAD( "10621.3m",    0x004000, 0x02000, CRC(3dca8e4b) SHA1(ca0416d8faba0bb5e6b8c0a8fc227b57caa75f71) )

	ROM_REGION( 0x0600, REGION_PROMS, 0 )
    ROM_LOAD( "82s129.3a", 0x0000, 0x0100, CRC(44802169) SHA1(f181d80185e0f87ee906d2b40e3a5deb6f563aa2) )
	ROM_LOAD( "82s129.4d", 0x0100, 0x0100, CRC(69ca07cc) SHA1(38ab08174633b53d70a38aacb40059a25cf12069) )
	ROM_LOAD( "82s129.4a", 0x0200, 0x0100, CRC(145f950a) SHA1(b007d0c1cc9545e0e241b39b79a48593d457f826) )
	ROM_LOAD( "82s129.3d", 0x0300, 0x0100, CRC(43d24e17) SHA1(de5c9391574781dcd8f244794010e8eddffa1c1e) )
	ROM_LOAD( "82s129.3b", 0x0400, 0x0100, CRC(7fdc872c) SHA1(98572560aa524490489d4202dba292a5af9f15e7) )
	ROM_LOAD( "82s129.3c", 0x0500, 0x0100, CRC(cc1657e5) SHA1(358f20dce376c2389009f9673ce38b297af863f6) )
ROM_END

ROM_START( pbillrds )
	ROM_REGION( 0x18000, REGION_CPU1, 0 ) /* Z80 Code */
	ROM_LOAD( "10627.10n",   0x00000, 0x4000, CRC(2335e6dd) SHA1(82352b6f4abea88aad3a96ca63cccccb6e278f48) )	/* encrypted */
	ROM_LOAD( "10625.8r",    0x04000, 0x4000, CRC(8977c724) SHA1(f00835a04dc6fa7d8c1e382dace515f2aa7d6f44) )	/* encrypted */
	ROM_CONTINUE(            0x10000, 0x4000 )
	ROM_LOAD( "10626.8n",    0x14000, 0x4000, CRC(51d725e6) SHA1(d7007c983530780e7fa3686cb7a6d7c382c802fa) )	/* encrypted */

	ROM_REGION( 0xc000, REGION_GFX1, ROMREGION_DISPOSE ) /* GFX */
	ROM_LOAD( "10622.3h",    0x000000, 0x04000, CRC(23b864ac) SHA1(5a13ad6f2278761967269eed8c07077293c921d6) )
	ROM_LOAD( "10623.3h",    0x004000, 0x04000, CRC(3dbfb790) SHA1(81a2645b7b3addf8f5b83043c967647cea476002) )
	ROM_LOAD( "10624.3g",    0x008000, 0x04000, CRC(b80032a9) SHA1(20096bdae1aad8913d8d7b1045912ea5ae7fce6f) )

	ROM_REGION( 0x6000, REGION_GFX2, ROMREGION_DISPOSE ) /* GFX */
	ROM_LOAD( "10619.3r",    0x000000, 0x02000, CRC(3296b9d9) SHA1(51393306f74394de96c4097b6244e8eb36114dac) )
	ROM_LOAD( "10620.3n",    0x002000, 0x02000, CRC(ee76b079) SHA1(99abe2c5b1889d20bc3f5720b168690e3979fb2f) )
	ROM_LOAD( "10621.3m",    0x004000, 0x02000, CRC(3dca8e4b) SHA1(ca0416d8faba0bb5e6b8c0a8fc227b57caa75f71) )

	ROM_REGION( 0x0600, REGION_PROMS, 0 )
    ROM_LOAD( "82s129.3a", 0x0000, 0x0100, CRC(44802169) SHA1(f181d80185e0f87ee906d2b40e3a5deb6f563aa2) )
	ROM_LOAD( "82s129.4d", 0x0100, 0x0100, CRC(69ca07cc) SHA1(38ab08174633b53d70a38aacb40059a25cf12069) )
	ROM_LOAD( "82s129.4a", 0x0200, 0x0100, CRC(145f950a) SHA1(b007d0c1cc9545e0e241b39b79a48593d457f826) )
	ROM_LOAD( "82s129.3d", 0x0300, 0x0100, CRC(43d24e17) SHA1(de5c9391574781dcd8f244794010e8eddffa1c1e) )
	ROM_LOAD( "82s129.3b", 0x0400, 0x0100, CRC(7fdc872c) SHA1(98572560aa524490489d4202dba292a5af9f15e7) )
	ROM_LOAD( "82s129.3c", 0x0500, 0x0100, CRC(cc1657e5) SHA1(358f20dce376c2389009f9673ce38b297af863f6) )
ROM_END

ROM_START( freekick )
    ROM_REGION( 0x0d000*2, REGION_CPU1, 0 ) /* Z80 Code, internal program RAM is 52K in custom cpu module */
	/* Custom CPU (pack) No. NS6201-A 1987.10 FREE KICK */
	ROM_LOAD( "ns6201-a_1987.10_free_kick.cpu", 0x00000, 0x0d000, CRC(6d172850) SHA1(ac461bff9da263681085920ad6acd778241dedd3) )
	
	ROM_REGION( 0x08000, REGION_USER1, 0 ) /* sound data */
	ROM_LOAD( "11.1e",       0x00000, 0x08000, CRC(a6030ba9) SHA1(f363100f54a7a80701a6395c7539b8daa60db054) )

	ROM_REGION( 0xc000, REGION_GFX1, ROMREGION_DISPOSE ) /* GFX */
	ROM_LOAD( "12.1h",       0x000000, 0x04000, CRC(fb82e486) SHA1(bc672272dc32b2aa64e991992172c44bea1ca65c) )
	ROM_LOAD( "13.1j",       0x004000, 0x04000, CRC(3ad78ee2) SHA1(033285d4ab7d6f46abf4c1bd4671c874738f0ac1) )
	ROM_LOAD( "14.1l",       0x008000, 0x04000, CRC(0185695f) SHA1(126994c69de157fc7c452ccc7f1a767f5085da27) )

	ROM_REGION( 0xc000, REGION_GFX2, ROMREGION_DISPOSE ) /* GFX */
	ROM_LOAD( "15.1m",       0x000000, 0x04000, CRC(0fa7c13c) SHA1(24b0ca73b0e35474e2392d8e729bcd44b80f9135) )
	ROM_LOAD( "16.1p",       0x004000, 0x04000, CRC(2b996e89) SHA1(c6900449d27e89c3b444fb028694fdcda8e79322) )
	ROM_LOAD( "17.1r",       0x008000, 0x04000, CRC(e7894def) SHA1(5c97b7cce43d1e51c709603a0d2394b8119764bd) )

	ROM_REGION( 0x0600, REGION_PROMS, 0 ) /* use the Japanese set proms, others seemed bad */
    ROM_LOAD( "8j.bpr",    0x0000, 0x0100, CRC(53a6bc21) SHA1(d4beedc226004c1aa9b6aae29bee9c8a9b0fff7c) )
	ROM_LOAD( "7j.bpr",    0x0100, 0x0100, CRC(38dd97d8) SHA1(468a0f87a704982dc1bce1ca21f9bb252ac241a0) )
	ROM_LOAD( "8k.bpr",    0x0200, 0x0100, CRC(18e66087) SHA1(54857526179b738862d11ce87e9d0edcb7878488) )
	ROM_LOAD( "7k.bpr",    0x0300, 0x0100, CRC(bc21797a) SHA1(4d6cf05e51b7ef9147eeff051c3728764021cfdb) )
	ROM_LOAD( "8h.bpr",    0x0400, 0x0100, CRC(8aac5fd0) SHA1(07a179603c0167c1f998b2337d66be95db9911cc) )
	ROM_LOAD( "7h.bpr",    0x0500, 0x0100, CRC(a507f941) SHA1(97619959ee4c366cb010525636ab5eefe5a3127a) )
ROM_END

ROM_START( freekckb )
	ROM_REGION( 0x10000, REGION_CPU1, 0 ) /* Z80 Code */
	ROM_LOAD( "freekbl8.q7", 0x00000, 0x10000, CRC(4208cfe5) SHA1(21628cbe8a217fbae30a6c24c9cc4c790fe45d65) ) /* this was on the bootleg, would normally be battery backed inside cpu?*/

	ROM_REGION( 0x08000, REGION_USER1, 0 ) /* sound data */
	ROM_LOAD( "11.1e",       0x00000, 0x08000, CRC(a6030ba9) SHA1(f363100f54a7a80701a6395c7539b8daa60db054) )		/* freekbl1.e2*/

	ROM_REGION( 0xc000, REGION_GFX1, ROMREGION_DISPOSE ) /* GFX */
	ROM_LOAD( "12.1h",       0x000000, 0x04000, CRC(fb82e486) SHA1(bc672272dc32b2aa64e991992172c44bea1ca65c) )	/* freekbl2.f2*/
	ROM_LOAD( "13.1j",       0x004000, 0x04000, CRC(3ad78ee2) SHA1(033285d4ab7d6f46abf4c1bd4671c874738f0ac1) )	/* freekbl3.j2*/
	ROM_LOAD( "14.1l",       0x008000, 0x04000, CRC(0185695f) SHA1(126994c69de157fc7c452ccc7f1a767f5085da27) )	/* freekbl4.k2*/

	ROM_REGION( 0xc000, REGION_GFX2, ROMREGION_DISPOSE ) /* GFX */
	ROM_LOAD( "15.1m",       0x000000, 0x04000, CRC(0fa7c13c) SHA1(24b0ca73b0e35474e2392d8e729bcd44b80f9135) )	/* freekbl5.m2*/
	ROM_LOAD( "16.1p",       0x004000, 0x04000, CRC(2b996e89) SHA1(c6900449d27e89c3b444fb028694fdcda8e79322) )	/* freekbl6.n2*/
	ROM_LOAD( "17.1r",       0x008000, 0x04000, CRC(e7894def) SHA1(5c97b7cce43d1e51c709603a0d2394b8119764bd) )	/* freekbl7.r2*/

	ROM_REGION( 0x0600, REGION_PROMS, 0 )
    ROM_LOAD( "8j.bpr",    0x0000, 0x0100, CRC(53a6bc21) SHA1(d4beedc226004c1aa9b6aae29bee9c8a9b0fff7c) )
	ROM_LOAD( "7j.bpr",    0x0100, 0x0100, CRC(38dd97d8) SHA1(468a0f87a704982dc1bce1ca21f9bb252ac241a0) )
	ROM_LOAD( "8k.bpr",    0x0200, 0x0100, CRC(18e66087) SHA1(54857526179b738862d11ce87e9d0edcb7878488) )
	ROM_LOAD( "7k.bpr",    0x0300, 0x0100, CRC(bc21797a) SHA1(4d6cf05e51b7ef9147eeff051c3728764021cfdb) )
	ROM_LOAD( "8h.bpr",    0x0400, 0x0100, CRC(8aac5fd0) SHA1(07a179603c0167c1f998b2337d66be95db9911cc) )
	ROM_LOAD( "7h.bpr",    0x0500, 0x0100, CRC(a507f941) SHA1(97619959ee4c366cb010525636ab5eefe5a3127a) )
ROM_END

/*
The original Counter Run set doesn't work, it's missing the main CPU code which is inside a custom CPU "block"
just like the original Freekick boards. Hopefully an original Counter Run board can be found and dumped (using
the same method as used for Freekick) before they all die, batteries don't last for ever :-(
*/

ROM_START( countrun )
	ROM_REGION( 0x10000, REGION_CPU1, 0 ) /* Z80 Code */
	/*  Custom CPU (pack) No. NS6201-A 1988.3 COUNTER RUN*/
	ROM_LOAD( "countrun.cpu", 0x00000, 0x10000, NO_DUMP ) /* missing*/

	ROM_REGION( 0x08000, REGION_USER1, 0 ) /* sound data */
	ROM_LOAD( "c-run.e1", 0x00000, 0x08000, CRC(2c3b6f8f) SHA1(ee7d71e6d8bb7138d5d029a10a95471d387b5f29) )

	ROM_REGION( 0xc000, REGION_GFX1, ROMREGION_DISPOSE ) /* GFX */
	ROM_LOAD( "c-run.h1", 0x000000, 0x04000, CRC(3385b7b5) SHA1(3f8f96f2a5406369dd56a9fe9f509ebee4a0179a) ) /* rom 2*/
	ROM_LOAD( "c-run.j1", 0x004000, 0x04000, CRC(58dc148d) SHA1(3b2e5c6ced885d945f6c02fbab7c6d40db78c66a) ) /* rom 3*/
	ROM_LOAD( "c-run.l1", 0x008000, 0x04000, CRC(3201f1e9) SHA1(72bd35600bf6e38741730f39bfd2a19f359bfb93) ) /* rom 4*/

	ROM_REGION( 0xc000, REGION_GFX2, ROMREGION_DISPOSE ) /* GFX */
	ROM_LOAD( "c-run.m1", 0x000000, 0x04000, CRC(1efab3b4) SHA1(7ce39cecf2809d3a7cbca5c6dffee738ba6f7b11) ) /* rom 5*/
	ROM_LOAD( "c-run.p1", 0x004000, 0x04000, CRC(d0bf8d42) SHA1(b8d1bd155dba065475c84db768f14a3562fe21e0) ) /* rom 6*/
	ROM_LOAD( "c-run.r1", 0x008000, 0x04000, CRC(4bb4a3e3) SHA1(179696464fce548ec333eec233025840fdb1eac2) ) /* rom 7*/

	ROM_REGION( 0x0600, REGION_PROMS, 0 )
	ROM_LOAD( "prom5.bpr",    0x0000, 0x0100, CRC(63c114ad) SHA1(db881c4ff92cb04a94988587503346a44eb89b69) )
	ROM_LOAD( "prom2.bpr",    0x0100, 0x0100, CRC(d16f95cc) SHA1(041bb84576bd8492c1ad3e492d8cb3e04d316527) )
	ROM_LOAD( "prom4.bpr",    0x0200, 0x0100, CRC(217db2c1) SHA1(f2af1a74b0ce56290b1c119e1a9707287132194a) )
	ROM_LOAD( "prom1.bpr",    0x0300, 0x0100, CRC(8d983949) SHA1(d7331900d18a53ceb133f8a8848d3c108e03323a) )
	ROM_LOAD( "prom6.bpr",    0x0400, 0x0100, CRC(33e87550) SHA1(951ce0dc975b799c1056ce8eb005256cbb43a112) )
	ROM_LOAD( "prom3.bpr",    0x0500, 0x0100, CRC(c77d0077) SHA1(4cbbf625ad5e45d00ca6aebe9566538ff0a3348d) )
ROM_END

ROM_START( countrnb )
	ROM_REGION( 0x10000, REGION_CPU1, 0 ) /* Z80 Code */
	ROM_LOAD( "rom_cpu.bin", 0x00000, 0x10000, CRC(f65639ae) SHA1(faa81607858d49559098c887ac847722df955a76) ) 
	
	ROM_REGION( 0x08000, REGION_USER1, 0 ) /* sound data */
	ROM_LOAD( "c-run.e1", 0x00000, 0x08000, CRC(2c3b6f8f) SHA1(ee7d71e6d8bb7138d5d029a10a95471d387b5f29) )

	ROM_REGION( 0xc000, REGION_GFX1, ROMREGION_DISPOSE ) /* GFX */
	ROM_LOAD( "c-run.h1", 0x000000, 0x04000, CRC(3385b7b5) SHA1(3f8f96f2a5406369dd56a9fe9f509ebee4a0179a) ) /* rom 2*/
	ROM_LOAD( "c-run.j1", 0x004000, 0x04000, CRC(58dc148d) SHA1(3b2e5c6ced885d945f6c02fbab7c6d40db78c66a) ) /* rom 3*/
	ROM_LOAD( "c-run.l1", 0x008000, 0x04000, CRC(3201f1e9) SHA1(72bd35600bf6e38741730f39bfd2a19f359bfb93) ) /* rom 4*/

	ROM_REGION( 0xc000, REGION_GFX2, ROMREGION_DISPOSE ) /* GFX */
	ROM_LOAD( "c-run.m1", 0x000000, 0x04000, CRC(1efab3b4) SHA1(7ce39cecf2809d3a7cbca5c6dffee738ba6f7b11) ) /* rom 5*/
	ROM_LOAD( "c-run.p1", 0x004000, 0x04000, CRC(d0bf8d42) SHA1(b8d1bd155dba065475c84db768f14a3562fe21e0) ) /* rom 6*/
	ROM_LOAD( "c-run.r1", 0x008000, 0x04000, CRC(4bb4a3e3) SHA1(179696464fce548ec333eec233025840fdb1eac2) ) /* rom 7*/

	ROM_REGION( 0x0600, REGION_PROMS, 0 )
	ROM_LOAD( "prom5.bpr",    0x0000, 0x0100, CRC(63c114ad) SHA1(db881c4ff92cb04a94988587503346a44eb89b69) )
	ROM_LOAD( "prom2.bpr",    0x0100, 0x0100, CRC(d16f95cc) SHA1(041bb84576bd8492c1ad3e492d8cb3e04d316527) )
	ROM_LOAD( "prom4.bpr",    0x0200, 0x0100, CRC(217db2c1) SHA1(f2af1a74b0ce56290b1c119e1a9707287132194a) )
	ROM_LOAD( "prom1.bpr",    0x0300, 0x0100, CRC(8d983949) SHA1(d7331900d18a53ceb133f8a8848d3c108e03323a) )
	ROM_LOAD( "prom6.bpr",    0x0400, 0x0100, CRC(33e87550) SHA1(951ce0dc975b799c1056ce8eb005256cbb43a112) )
	ROM_LOAD( "prom3.bpr",    0x0500, 0x0100, CRC(c77d0077) SHA1(4cbbf625ad5e45d00ca6aebe9566538ff0a3348d) )
ROM_END

ROM_START( countrb2 )
	ROM_REGION( 0x10000, REGION_CPU1, 0 ) /* Z80 Code */
	ROM_LOAD( "crunbl.8", 0x00000, 0x10000, CRC(318f95d9) SHA1(f2386b9d26d1bc98728aad9e257363b381043dc9) ) /* encrypted? bad? its strange anyway*/

	ROM_REGION( 0x01000, REGION_CPU2, 0 )
	ROM_LOAD( "68705.uc",  0x00000, 0x01000, NO_DUMP )

	ROM_REGION( 0x08000, REGION_USER1, 0 ) /* sound data */
	ROM_LOAD( "c-run.e1", 0x00000, 0x08000, CRC(2c3b6f8f) SHA1(ee7d71e6d8bb7138d5d029a10a95471d387b5f29) )

	ROM_REGION( 0xc000, REGION_GFX1, ROMREGION_DISPOSE ) /* GFX */
	ROM_LOAD( "c-run.h1", 0x000000, 0x04000, CRC(3385b7b5) SHA1(3f8f96f2a5406369dd56a9fe9f509ebee4a0179a) ) /* rom 2*/
	ROM_LOAD( "c-run.j1", 0x004000, 0x04000, CRC(58dc148d) SHA1(3b2e5c6ced885d945f6c02fbab7c6d40db78c66a) ) /* rom 3*/
	ROM_LOAD( "c-run.l1", 0x008000, 0x04000, CRC(3201f1e9) SHA1(72bd35600bf6e38741730f39bfd2a19f359bfb93) ) /* rom 4*/

	ROM_REGION( 0xc000, REGION_GFX2, ROMREGION_DISPOSE ) /* GFX */
	ROM_LOAD( "c-run.m1", 0x000000, 0x04000, CRC(1efab3b4) SHA1(7ce39cecf2809d3a7cbca5c6dffee738ba6f7b11) ) /* rom 5*/
	ROM_LOAD( "c-run.p1", 0x004000, 0x04000, CRC(d0bf8d42) SHA1(b8d1bd155dba065475c84db768f14a3562fe21e0) ) /* rom 6*/
	ROM_LOAD( "c-run.r1", 0x008000, 0x04000, CRC(4bb4a3e3) SHA1(179696464fce548ec333eec233025840fdb1eac2) ) /* rom 7*/

	ROM_REGION( 0x0600, REGION_PROMS, 0 )
	ROM_LOAD( "prom5.bpr",    0x0000, 0x0100, CRC(63c114ad) SHA1(db881c4ff92cb04a94988587503346a44eb89b69) )
	ROM_LOAD( "prom2.bpr",    0x0100, 0x0100, CRC(d16f95cc) SHA1(041bb84576bd8492c1ad3e492d8cb3e04d316527) )
	ROM_LOAD( "prom4.bpr",    0x0200, 0x0100, CRC(217db2c1) SHA1(f2af1a74b0ce56290b1c119e1a9707287132194a) )
	ROM_LOAD( "prom1.bpr",    0x0300, 0x0100, CRC(8d983949) SHA1(d7331900d18a53ceb133f8a8848d3c108e03323a) )
	ROM_LOAD( "prom6.bpr",    0x0400, 0x0100, CRC(33e87550) SHA1(951ce0dc975b799c1056ce8eb005256cbb43a112) )
	ROM_LOAD( "prom3.bpr",    0x0500, 0x0100, CRC(c77d0077) SHA1(4cbbf625ad5e45d00ca6aebe9566538ff0a3348d) )
ROM_END

ROM_START( gigasm2b )
	ROM_REGION( 0x10000*2, REGION_CPU1, 0 )
	ROM_LOAD( "8.rom", 0x10000, 0x4000, CRC(c00a4a6c) SHA1(0d1bb849c9bfe4e92ad70e4ef19da494c0bd7ba8) )
	ROM_CONTINUE(      0x00000, 0x4000 )
	ROM_LOAD( "7.rom", 0x14000, 0x4000, CRC(92bd9045) SHA1(e4d8a94deeb795bb284ca0bd211ed40ed498b172) )
	ROM_CONTINUE(      0x04000, 0x4000 )
	ROM_LOAD( "9.rom", 0x18000, 0x4000, CRC(a3ef809c) SHA1(6d4098658aa124e10e5edb8e8e3abe0aa26741a1) )
	ROM_CONTINUE(      0x08000, 0x4000 )

	ROM_REGION( 0xc000, REGION_GFX1, ROMREGION_DISPOSE ) /* GFX */
	ROM_LOAD( "4.rom",       0x00000, 0x04000, CRC(20b3405f) SHA1(32120d7b40e74648eb4ac4ab3ad3d2125033f6b1) )
	ROM_LOAD( "5.rom",       0x04000, 0x04000, CRC(d04ecfa8) SHA1(10bfb1d075da768f31a8c34cdfe1a1bf01e89f94) )
	ROM_LOAD( "6.rom",       0x08000, 0x04000, CRC(33776801) SHA1(29952818038a08c98b95ac801b8929cf1647049c) )

	ROM_REGION( 0xc000, REGION_GFX2, ROMREGION_DISPOSE ) /* GFX */
	ROM_LOAD( "1.rom",       0x00000, 0x04000, CRC(f64cbd1e) SHA1(f8d9b110cdac6ef524e35bec9a5d406651cd7bab) )
	ROM_LOAD( "3.rom",       0x04000, 0x04000, CRC(c228df19) SHA1(584f269f7de2d531f2b038b4b7318f813c329f7f) )
	ROM_LOAD( "2.rom",       0x08000, 0x04000, CRC(a6ad9ce2) SHA1(db0338385208df9e9cf43efc11383412dec493e6) )

	ROM_REGION( 0x0600, REGION_PROMS, 0 )
	ROM_LOAD( "1.pr",    0x0000, 0x0100, CRC(a784e71f) SHA1(1741ce98d719bad6cc5ea42337ef897f2435bbab) )
	ROM_LOAD( "6.pr",    0x0100, 0x0100, CRC(376df30c) SHA1(cc95920cd1c133da1becc7d92f4b187b56a90ec7) )
	ROM_LOAD( "5.pr",    0x0200, 0x0100, CRC(4edff5bd) SHA1(305efc7ad7f86635489a655e214e216ac02b904d) )
	ROM_LOAD( "4.pr",    0x0300, 0x0100, CRC(fe201a4e) SHA1(15f8ecfcf6c63ffbf9777bec9b203c319ba1b96c) )
	ROM_LOAD( "2.pr",    0x0400, 0x0100, CRC(5796cc4a) SHA1(39576c4e48fd7ac52fc652a1ae0573db3d878878) )
	ROM_LOAD( "3.pr",    0x0500, 0x0100, CRC(28b5ee4c) SHA1(e21b9c38f433dca1e8894619b1d9f0389a81b48a) )
ROM_END

ROM_START( gigasb )
	ROM_REGION( 0x10000*2, REGION_CPU1, 0 )
	ROM_LOAD( "g-7",   0x10000, 0x4000, CRC(daf4e88d) SHA1(391dff914ce8e9b7975fc8827c066d7db16c4171) )
	ROM_CONTINUE(      0x00000, 0x4000 )
	ROM_LOAD( "g-8",   0x14000, 0x8000, CRC(4ab4c1f1) SHA1(63d8f489c7a8271e99a66d97e6eb0eb252cb2b67) )
	ROM_CONTINUE(      0x04000, 0x8000 )

	ROM_REGION( 0xc000, REGION_GFX1, ROMREGION_DISPOSE ) /* GFX */
	ROM_LOAD( "g-4",     0x00000, 0x04000, CRC(8ed78981) SHA1(1f2c0584fcc6d04b042638c7b9a7e21fc560ca3d) )
	ROM_LOAD( "g-5",     0x04000, 0x04000, CRC(0645ec2d) SHA1(ecf8b1ce98f845b5b32e7fc959cea7679a149d74) )
	ROM_LOAD( "g-6",     0x08000, 0x04000, CRC(99e9cb27) SHA1(d141d6caa077e3cd182eb64cf803613ac17e7d09) )

	ROM_REGION( 0xc000, REGION_GFX2, ROMREGION_DISPOSE ) /* GFX */
	ROM_LOAD( "g-1",     0x00000, 0x04000, CRC(d78fae6e) SHA1(a7bf3b213f2a3a51b964959bd45003351670575a) )
	ROM_LOAD( "g-3",     0x04000, 0x04000, CRC(37df4a4c) SHA1(ab996db636d89845474529ba2573307046fb96ee) )
	ROM_LOAD( "g-2",     0x08000, 0x04000, CRC(3a46e354) SHA1(ebd6a5db4c9cdfc6fabe6b412a704aaf03c32d7c) )

	ROM_REGION( 0x0600, REGION_PROMS, 0 )
	ROM_LOAD( "1.pr",    0x0000, 0x0100, CRC(a784e71f) SHA1(1741ce98d719bad6cc5ea42337ef897f2435bbab) )
	ROM_LOAD( "6.pr",    0x0100, 0x0100, CRC(376df30c) SHA1(cc95920cd1c133da1becc7d92f4b187b56a90ec7) )
	ROM_LOAD( "5.pr",    0x0200, 0x0100, CRC(4edff5bd) SHA1(305efc7ad7f86635489a655e214e216ac02b904d) )
	ROM_LOAD( "4.pr",    0x0300, 0x0100, CRC(fe201a4e) SHA1(15f8ecfcf6c63ffbf9777bec9b203c319ba1b96c) )
	ROM_LOAD( "2.pr",    0x0400, 0x0100, CRC(5796cc4a) SHA1(39576c4e48fd7ac52fc652a1ae0573db3d878878) )
	ROM_LOAD( "3.pr",    0x0500, 0x0100, CRC(28b5ee4c) SHA1(e21b9c38f433dca1e8894619b1d9f0389a81b48a) )
ROM_END

ROM_START( oigas )
	ROM_REGION( 0x10000*2, REGION_CPU1, 0 )
	ROM_LOAD( "rom.7",   0x10000, 0x4000, CRC(e5bc04cc) SHA1(ffbd416313a9e49d2f9a7268d5ef48a8b641e480) )
	ROM_CONTINUE(        0x00000, 0x4000)
	ROM_LOAD( "rom.8",   0x04000, 0x8000, CRC(c199060d) SHA1(de8f1e0f941533abbbed25b595b1d51fadbb428d) )

	ROM_REGION( 0x0800, REGION_CPU2, 0 )
	ROM_LOAD( "8748.bin",		0x0000, 0x0800, NO_DUMP )	/* missing */

	ROM_REGION( 0xc000, REGION_GFX1, ROMREGION_DISPOSE ) /* GFX */
	ROM_LOAD( "g-4",     0x00000, 0x04000, CRC(8ed78981) SHA1(1f2c0584fcc6d04b042638c7b9a7e21fc560ca3d) )
	ROM_LOAD( "g-5",     0x04000, 0x04000, CRC(0645ec2d) SHA1(ecf8b1ce98f845b5b32e7fc959cea7679a149d74) )
	ROM_LOAD( "g-6",     0x08000, 0x04000, CRC(99e9cb27) SHA1(d141d6caa077e3cd182eb64cf803613ac17e7d09) )

	ROM_REGION( 0xc000, REGION_GFX2, ROMREGION_DISPOSE ) /* GFX */
	ROM_LOAD( "g-1",     0x00000, 0x04000, CRC(d78fae6e) SHA1(a7bf3b213f2a3a51b964959bd45003351670575a) )
	ROM_LOAD( "g-3",     0x04000, 0x04000, CRC(37df4a4c) SHA1(ab996db636d89845474529ba2573307046fb96ee) )
	ROM_LOAD( "g-2",     0x08000, 0x04000, CRC(3a46e354) SHA1(ebd6a5db4c9cdfc6fabe6b412a704aaf03c32d7c) )

	ROM_REGION( 0x0600, REGION_PROMS, 0 )
	ROM_LOAD( "1.pr",    0x0000, 0x0100, CRC(a784e71f) SHA1(1741ce98d719bad6cc5ea42337ef897f2435bbab) )
	ROM_LOAD( "6.pr",    0x0100, 0x0100, CRC(376df30c) SHA1(cc95920cd1c133da1becc7d92f4b187b56a90ec7) )
	ROM_LOAD( "5.pr",    0x0200, 0x0100, CRC(4edff5bd) SHA1(305efc7ad7f86635489a655e214e216ac02b904d) )
	ROM_LOAD( "4.pr",    0x0300, 0x0100, CRC(fe201a4e) SHA1(15f8ecfcf6c63ffbf9777bec9b203c319ba1b96c) )
	ROM_LOAD( "2.pr",    0x0400, 0x0100, CRC(5796cc4a) SHA1(39576c4e48fd7ac52fc652a1ae0573db3d878878) )
	ROM_LOAD( "3.pr",    0x0500, 0x0100, CRC(28b5ee4c) SHA1(e21b9c38f433dca1e8894619b1d9f0389a81b48a) )
ROM_END

static DRIVER_INIT(gigas)
{
	unsigned char *rom = memory_region(REGION_CPU1);
	int diff = memory_region_length(REGION_CPU1) / 2;
	memory_set_opcode_base(0,rom+diff);
}

GAMEX(1986, gigasb,   0,        gigas,    gigas,    gigas, ROT270, "bootleg", "Gigas (bootleg)", GAME_NO_COCKTAIL )
GAMEX(1986, oigas,    gigasb,   oigas,    gigas,    gigas, ROT270, "bootleg", "Oigas (bootleg)", GAME_NO_COCKTAIL )
GAMEX(1986, gigasm2b, 0,        gigas,    gigasm2,  gigas, ROT270, "bootleg", "Gigas Mark II (bootleg)", GAME_NO_COCKTAIL )
GAME( 1987, pbillrd,  0,        pbillrd,  pbillrd,  0,     ROT0,   "Nihon System", "Perfect Billiard" )
GAMEX(1987, pbillrds, pbillrd,  pbillrd,  pbillrd,  0,     ROT0,   "Nihon System", "Perfect Billiard (Sega)", GAME_UNEMULATED_PROTECTION )	/* encrypted*/
GAME( 1987, freekick, 0,        freekckb, freekckb, 0,     ROT270, "Nihon System (Sega license)", "Free Kick" )
GAME( 1987, freekckb, freekick, freekckb, freekckb, 0,     ROT270, "bootleg", "Free Kick (bootleg)" )
GAMEX(1988, countrun, 0,        freekckb, countrun, 0,     ROT0,   "Nihon System (Sega license)", "Counter Run", GAME_NOT_WORKING )
GAME( 1988, countrnb, countrun, freekckb, countrun, 0,     ROT0,   "bootleg", "Counter Run (bootleg set 1)" )
GAMEX(1988, countrb2, countrun, freekckb, countrun, 0,     ROT0,   "bootleg", "Counter Run (bootleg set 2)", GAME_NOT_WORKING )
