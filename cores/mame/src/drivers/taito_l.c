/*
  Taito L-System

  Monoprocessor games (1 main z80, no sound z80)
  - Plotting
  - Puzznic
  - Palamedes
  - Cachat / Tube-It
  - American Horseshoes
  - Play Girls
  - Play Girls 2
  - Cuby Bop

  Dual processor games
  - Kuri Kinton
  - Evil Stone

  Triple processor games (2 main z80, 1 sound z80)
  - Fighting hawk
  - Raimais
  - Champion Wrestler

Notes:
- the system uses RAM based characters, which aren't really supported by the
  TileMap system, so we have to tilemap_mark_all_tiles_dirty() to compensate
- kurikina has some debug dip switches (invulnerability, slow motion) so might
  be a prototype. It also doesn't have service mode (or has it disabled).

TODO:
- champwr ADPCM interface is not entirely understood, it involves also addresses
  0xd000 and 0xe000, and maybe also YM2203 port B.
- slowdowns in fhawk, probably the interrupts have to be generated at a
  different time.
- plgirls doesn't work without a kludge because of an interrupt issue. This
  happens because the program enables interrupts before setting IM2, so the
  interrupt vector is interpreted as IM0, which is obviously bogus.
- The  puzznic protection is worked around,  but I'm not happy with it
  (the 68705-returned values are wrong, I'm sure of that).
- A bunch of control registers are simply ignored
- The source of   irqs 0 and  1 is  unknown, while  2 is vblank  (0 is
  usually   ignored  by the  program,    1   leads  to  reading    the
  ports... maybe vbl-in, vbl-out and hblank ?).
- Text Plane colours are only right in Cuby Bop once you've started a game
  & reset
- Scrolling in Cuby Bop's Game seems incorrect.

*/


#include "driver.h"
#include "cpu/z80/z80.h"
#include "sndhrdw/taitosnd.h"

VIDEO_EOF( taitol );
VIDEO_START( taitol );
VIDEO_UPDATE( taitol );

void taitol_chardef14_m(int offset);
void taitol_chardef15_m(int offset);
void taitol_chardef16_m(int offset);
void taitol_chardef17_m(int offset);
void taitol_chardef1c_m(int offset);
void taitol_chardef1d_m(int offset);
void taitol_chardef1e_m(int offset);
void taitol_chardef1f_m(int offset);
void taitol_bg18_m(int offset);
void taitol_bg19_m(int offset);
void taitol_char1a_m(int offset);
void taitol_obj1b_m(int offset);

WRITE_HANDLER( taitol_control_w );
READ_HANDLER( taitol_control_r );
WRITE_HANDLER( horshoes_bankg_w );
WRITE_HANDLER( taitol_bankc_w );
READ_HANDLER( taitol_bankc_r );



static void (*rambank_modify_notifiers[12])(int) =
{
	taitol_chardef14_m,	/* 14*/
	taitol_chardef15_m,	/* 15*/
	taitol_chardef16_m,	/* 16*/
	taitol_chardef17_m,	/* 17*/

	taitol_bg18_m,		/* 18*/
	taitol_bg19_m,		/* 19*/
	taitol_char1a_m,	/* 1a*/
	taitol_obj1b_m,		/* 1b*/

	taitol_chardef1c_m,	/* 1c*/
	taitol_chardef1d_m,	/* 1d*/
	taitol_chardef1e_m,	/* 1e*/
	taitol_chardef1f_m,	/* 1f*/
};

static void (*current_notifier[4])(int);
static unsigned char *current_base[4];

static int cur_rombank, cur_rombank2, cur_rambank[4];
static int irq_adr_table[3];
static int irq_enable = 0;

unsigned char *taitol_rambanks;

static unsigned char *palette_ram;
static unsigned char *empty_ram;
static unsigned char *shared_ram;

static mem_read_handler porte0_r;
static mem_read_handler porte1_r;
static mem_read_handler portf0_r;
static mem_read_handler portf1_r;

static void palette_notifier(int addr)
{
	unsigned char *p = palette_ram + (addr & ~1);
	unsigned char byte0 = *p++;
	unsigned char byte1 = *p;

	unsigned int b = (byte1 & 0xf) * 0x11;
	unsigned int g = ((byte0 & 0xf0)>>4) * 0x11;
	unsigned int r = (byte0 & 0xf) * 0x11;

	/*	addr &= 0x1ff;*/

	if(addr > 0x200)
	{
log_cb(RETRO_LOG_DEBUG, LOGPRE "Large palette ? %03x (%04x)\n", addr, activecpu_get_pc());
	}
	else
	{
		/*		r = g = b = ((addr & 0x1e) != 0)*255;*/
		palette_set_color(addr/2, r, g, b);
	}
}

static void machine_init(void)
{
	int i;

	taitol_rambanks = auto_malloc(0x1000*12);
	palette_ram = auto_malloc(0x1000);
	empty_ram = auto_malloc(0x1000);

	for(i=0;i<3;i++)
		irq_adr_table[i] = 0;

	irq_enable = 0;

	for(i=0;i<4;i++)
	{
		cur_rambank[i] = 0x80;
		current_base[i] = palette_ram;
		current_notifier[i] = palette_notifier;
		cpu_setbank(2+i, current_base[i]);
	}
	cur_rombank = cur_rombank2 = 0;
	cpu_setbank(1, memory_region(REGION_CPU1) + 0x10000);

	for(i=0;i<512;i++)
	{
		decodechar(Machine->gfx[2], i, taitol_rambanks,
				   Machine->drv->gfxdecodeinfo[2].gfxlayout);
		decodechar(Machine->gfx[2], i+512, taitol_rambanks + 0x4000,
				   Machine->drv->gfxdecodeinfo[2].gfxlayout);
	}
}


static MACHINE_INIT( fhawk )
{
	machine_init();
	porte0_r = 0;
	porte1_r = 0;
	portf0_r = 0;
	portf1_r = 0;
}

static MACHINE_INIT( raimais )
{
	machine_init();
	porte0_r = 0;
	porte1_r = 0;
	portf0_r = 0;
	portf1_r = 0;
}

static MACHINE_INIT( champwr )
{
	machine_init();
	porte0_r = 0;
	porte1_r = 0;
	portf0_r = 0;
	portf1_r = 0;
}


static MACHINE_INIT( kurikint )
{
	machine_init();
	porte0_r = 0;
	porte1_r = 0;
	portf0_r = 0;
	portf1_r = 0;
}

static MACHINE_INIT( evilston )
{
	machine_init();
	porte0_r = 0;
	porte1_r = 0;
	portf0_r = 0;
	portf1_r = 0;
}

static MACHINE_INIT( puzznic )
{
	machine_init();
	porte0_r = input_port_0_r;
	porte1_r = input_port_1_r;
	portf0_r = input_port_2_r;
	portf1_r = input_port_3_r;
}

static MACHINE_INIT( plotting )
{
	machine_init();
	porte0_r = input_port_0_r;
	porte1_r = input_port_1_r;
	portf0_r = input_port_2_r;
	portf1_r = input_port_3_r;
}

static MACHINE_INIT( palamed )
{
	machine_init();
	porte0_r = input_port_0_r;
	porte1_r = 0;
	portf0_r = input_port_1_r;
	portf1_r = 0;
}

static MACHINE_INIT( cachat )
{
	machine_init();
	porte0_r = input_port_0_r;
	porte1_r = 0;
	portf0_r = input_port_1_r;
	portf1_r = 0;
}

static MACHINE_INIT( horshoes )
{
	machine_init();
	porte0_r = input_port_0_r;
	porte1_r = input_port_1_r;
	portf0_r = input_port_2_r;
	portf1_r = input_port_3_r;
}



static INTERRUPT_GEN( vbl_interrupt )
{
	/* kludge to make plgirls boot */
	if (cpunum_get_reg(0,Z80_IM) != 2) return;

	/* What is really generating interrupts 0 and 1 is still to be found*/

	if (cpu_getiloops() == 1 && (irq_enable & 1))
		cpu_set_irq_line_and_vector(0, 0, HOLD_LINE, irq_adr_table[0]);
	else if (cpu_getiloops() == 2 && (irq_enable & 2))
		cpu_set_irq_line_and_vector(0, 0, HOLD_LINE, irq_adr_table[1]);
	else if (cpu_getiloops() == 0 && (irq_enable & 4))
		cpu_set_irq_line_and_vector(0, 0, HOLD_LINE, irq_adr_table[2]);
}

static WRITE_HANDLER( irq_adr_w )
{
/*logerror("irq_adr_table[%d] = %02x\n",offset,data);*/
	irq_adr_table[offset] = data;
}

static READ_HANDLER( irq_adr_r )
{
	return irq_adr_table[offset];
}

static WRITE_HANDLER( irq_enable_w )
{
/*logerror("irq_enable = %02x\n",data);*/
	irq_enable = data;
}

static READ_HANDLER( irq_enable_r )
{
	return irq_enable;
}


static WRITE_HANDLER( rombankswitch_w )
{
	static int high = 0;
	if(cur_rombank != data)
	{
		if(data>high)
		{
			high = data;
			log_cb(RETRO_LOG_DEBUG, LOGPRE "New rom size : %x\n", (high+1)*0x2000);
		}

/*		log_cb(RETRO_LOG_DEBUG, LOGPRE "robs %d, %02x (%04x)\n", offset, data, activecpu_get_pc());*/
		cur_rombank = data;
		cpu_setbank(1, memory_region(REGION_CPU1)+0x10000+0x2000*cur_rombank);
	}
}

static WRITE_HANDLER( rombank2switch_w )
{
	static int high = 0;

	data &= 0xf;

	if(cur_rombank2 != data)
	{
		if(data>high)
		{
			high = data;
			log_cb(RETRO_LOG_DEBUG, LOGPRE "New rom2 size : %x\n", (high+1)*0x4000);
		}

/*		log_cb(RETRO_LOG_DEBUG, LOGPRE "robs2 %02x (%04x)\n", data, activecpu_get_pc());*/

		cur_rombank2 = data;
		cpu_setbank(6, memory_region(REGION_CPU3)+0x10000+0x4000*cur_rombank2);
	}
}

static READ_HANDLER( rombankswitch_r )
{
	return cur_rombank;
}

static READ_HANDLER( rombank2switch_r )
{
	return cur_rombank2;
}

static WRITE_HANDLER( rambankswitch_w )
{
	if(cur_rambank[offset]!=data)
	{
		cur_rambank[offset]=data;
/*logerror("rabs %d, %02x (%04x)\n", offset, data, activecpu_get_pc());*/
		if(data>=0x14 && data<=0x1f)
		{
			data -= 0x14;
			current_notifier[offset] = rambank_modify_notifiers[data];
			current_base[offset] = taitol_rambanks+0x1000*data;
		}
		else if (data == 0x80)
		{
			current_notifier[offset] = palette_notifier;
			current_base[offset] = palette_ram;
		}
		else
		{
log_cb(RETRO_LOG_DEBUG, LOGPRE "unknown rambankswitch %d, %02x (%04x)\n", offset, data, activecpu_get_pc());
			current_notifier[offset] = 0;
			current_base[offset] = empty_ram;
		}
		cpu_setbank(2+offset, current_base[offset]);
	}
}

static READ_HANDLER( rambankswitch_r )
{
	return cur_rambank[offset];
}

static WRITE_HANDLER( bank0_w )
{
	if(current_base[0][offset]!=data)
	{
		current_base[0][offset] = data;
		if(current_notifier[0])
			current_notifier[0](offset);
	}
}

static WRITE_HANDLER( bank1_w )
{
	if(current_base[1][offset]!=data)
	{
		current_base[1][offset] = data;
		if(current_notifier[1])
			current_notifier[1](offset);
	}
}

static WRITE_HANDLER( bank2_w )
{
	if(current_base[2][offset]!=data)
	{
		current_base[2][offset] = data;
		if(current_notifier[2])
			current_notifier[2](offset);
	}
}

static WRITE_HANDLER( bank3_w )
{
	if(current_base[3][offset]!=data)
	{
		current_base[3][offset] = data;
		if(current_notifier[3])
			current_notifier[3](offset);
	}
}

static WRITE_HANDLER( control2_w )
{
	coin_lockout_w(0,~data & 0x01);
	coin_lockout_w(1,~data & 0x02);
	coin_counter_w(0,data & 0x04);
	coin_counter_w(1,data & 0x08);
}

static int extport;

static READ_HANDLER( portA_r )
{
	if (extport == 0) return porte0_r(0);
	else return porte1_r(0);
}

static READ_HANDLER( portB_r )
{
	if (extport == 0) return portf0_r(0);
	else return portf1_r(0);
}

static READ_HANDLER( ym2203_data0_r )
{
	extport = 0;
	return YM2203_read_port_0_r(offset);
}

static READ_HANDLER( ym2203_data1_r )
{
	extport = 1;
	return YM2203_read_port_0_r(offset);
}

static int *mcu_reply;
static int mcu_pos = 0, mcu_reply_len = 0;
static int last_data_adr, last_data;

static int puzznic_mcu_reply[] = { 0x50, 0x1f, 0xb6, 0xba, 0x06, 0x03, 0x47, 0x05, 0x00 };

static WRITE_HANDLER( mcu_data_w )
{
	last_data = data;
	last_data_adr = activecpu_get_pc();
/*	log_cb(RETRO_LOG_DEBUG, LOGPRE "mcu write %02x (%04x)\n", data, activecpu_get_pc());*/
	switch(data)
	{
	case 0x43:
		mcu_pos = 0;
		mcu_reply = puzznic_mcu_reply;
		mcu_reply_len = sizeof(puzznic_mcu_reply);
		break;
	}
}

static WRITE_HANDLER( mcu_control_w )
{
/*	log_cb(RETRO_LOG_DEBUG, LOGPRE "mcu control %02x (%04x)\n", data, activecpu_get_pc());*/
}

static READ_HANDLER( mcu_data_r )
{
/*	log_cb(RETRO_LOG_DEBUG, LOGPRE "mcu read (%04x) [%02x, %04x]\n", activecpu_get_pc(), last_data, last_data_adr);*/
	if(mcu_pos==mcu_reply_len)
		return 0;

	return mcu_reply[mcu_pos++];
}

static READ_HANDLER( mcu_control_r )
{
/*	log_cb(RETRO_LOG_DEBUG, LOGPRE "mcu control read (%04x)\n", activecpu_get_pc());*/
	return 0x1;
}

#if 0
static WRITE_HANDLER( sound_w )
{
	log_cb(RETRO_LOG_DEBUG, LOGPRE "Sound_w %02x (%04x)\n", data, activecpu_get_pc());
}
#endif

static READ_HANDLER( shared_r )
{
	return shared_ram[offset];
}

static WRITE_HANDLER( shared_w )
{
	shared_ram[offset] = data;
}

static int mux_ctrl = 0;

static READ_HANDLER( mux_r )
{
	switch(mux_ctrl)
	{
	case 0:
		return input_port_0_r(0);
	case 1:
		return input_port_1_r(0);
	case 2:
		return input_port_2_r(0);
	case 3:
		return input_port_3_r(0);
	case 7:
		return input_port_4_r(0);
	default:
		log_cb(RETRO_LOG_DEBUG, LOGPRE "Mux read from unknown port %d (%04x)\n", mux_ctrl, activecpu_get_pc());
		return 0xff;
	}
}

static WRITE_HANDLER( mux_w )
{
	switch(mux_ctrl)
	{
	case 4:
		control2_w(0, data);
		break;
	default:
		log_cb(RETRO_LOG_DEBUG, LOGPRE "Mux write to unknown port %d, %02x (%04x)\n", mux_ctrl, data, activecpu_get_pc());
	}
}

static WRITE_HANDLER( mux_ctrl_w )
{
	mux_ctrl = data;
}




static int champwr_adpcm_start;

static WRITE_HANDLER( champwr_adpcm_lo_w )
{
	champwr_adpcm_start = (champwr_adpcm_start & 0xff00ff) | (data << 8);
}

static WRITE_HANDLER( champwr_adpcm_hi_w )
{
	UINT8 *rom = memory_region(REGION_SOUND1);
	int romlen = memory_region_length(REGION_SOUND1);
	int length;
	int i;

	champwr_adpcm_start = ((champwr_adpcm_start & 0x00ffff) | (data << 16)) & (romlen-1);
	i = champwr_adpcm_start + 0x20;
	while (i < romlen && (rom[i] || rom[i+1] || rom[i+2] || rom[i+3]))
		i += 4;
	length = i - champwr_adpcm_start;

	ADPCM_play(0,champwr_adpcm_start,length*2);
}



static int trackx,tracky;

static READ_HANDLER( horshoes_tracky_reset_r )
{
	/* reset the trackball counter */
	tracky = readinputport(4);
	return 0;
}

static READ_HANDLER( horshoes_trackx_reset_r )
{
	/* reset the trackball counter */
	trackx = readinputport(5);
	return 0;
}

static READ_HANDLER( horshoes_tracky_lo_r )
{
	return (readinputport(4) - tracky) & 0xff;
}

static READ_HANDLER( horshoes_tracky_hi_r )
{
	return (readinputport(4) - tracky) >> 8;
}

static READ_HANDLER( horshoes_trackx_lo_r )
{
	return (readinputport(5) - trackx) & 0xff;
}

static READ_HANDLER( horshoes_trackx_hi_r )
{
	return (readinputport(5) - trackx) >> 8;
}




#define COMMON_BANKS_READ \
	{ 0x0000, 0x5fff, MRA_ROM },			\
	{ 0x6000, 0x7fff, MRA_BANK1 },			\
	{ 0xc000, 0xcfff, MRA_BANK2 },			\
	{ 0xd000, 0xdfff, MRA_BANK3 },			\
	{ 0xe000, 0xefff, MRA_BANK4 },			\
	{ 0xf000, 0xfdff, MRA_BANK5 },			\
	{ 0xfe00, 0xfe03, taitol_bankc_r },		\
	{ 0xfe04, 0xfe04, taitol_control_r },	\
	{ 0xff00, 0xff02, irq_adr_r },			\
	{ 0xff03, 0xff03, irq_enable_r },		\
	{ 0xff04, 0xff07, rambankswitch_r },	\
	{ 0xff08, 0xff08, rombankswitch_r }

#define COMMON_BANKS_WRITE \
	{ 0x0000, 0x7fff, MWA_ROM },			\
	{ 0xc000, 0xcfff, bank0_w },			\
	{ 0xd000, 0xdfff, bank1_w },			\
	{ 0xe000, 0xefff, bank2_w },			\
	{ 0xf000, 0xfdff, bank3_w },			\
	{ 0xfe00, 0xfe03, taitol_bankc_w },		\
	{ 0xfe04, 0xfe04, taitol_control_w },	\
	{ 0xff00, 0xff02, irq_adr_w },			\
	{ 0xff03, 0xff03, irq_enable_w },		\
	{ 0xff04, 0xff07, rambankswitch_w },	\
	{ 0xff08, 0xff08, rombankswitch_w }

#define COMMON_SINGLE_READ \
	{ 0xa000, 0xa000, YM2203_status_port_0_r },	\
	{ 0xa001, 0xa001, ym2203_data0_r },			\
	{ 0xa003, 0xa003, ym2203_data1_r },			\
	{ 0x8000, 0x9fff, MRA_RAM }

#define COMMON_SINGLE_WRITE \
	{ 0xa000, 0xa000, YM2203_control_port_0_w },	\
	{ 0xa001, 0xa001, YM2203_write_port_0_w },		\
	{ 0x8000, 0x9fff, MWA_RAM }



static MEMORY_READ_START( fhawk_readmem )
	COMMON_BANKS_READ,
	{ 0x8000, 0x9fff, MRA_RAM },
	{ 0xa000, 0xbfff, MRA_RAM },
MEMORY_END

static MEMORY_WRITE_START( fhawk_writemem )
	COMMON_BANKS_WRITE,
	{ 0x8000, 0x9fff, MWA_RAM, &shared_ram },
	{ 0xa000, 0xbfff, MWA_RAM },
MEMORY_END

static MEMORY_READ_START( fhawk_2_readmem )
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0xbfff, MRA_BANK6 },
	{ 0xc800, 0xc800, MRA_NOP },
	{ 0xc801, 0xc801, taitosound_comm_r },
	{ 0xe000, 0xffff, shared_r },
	{ 0xd000, 0xd000, input_port_0_r },
	{ 0xd001, 0xd001, input_port_1_r },
	{ 0xd002, 0xd002, input_port_2_r },
	{ 0xd003, 0xd003, input_port_3_r },
	{ 0xd007, 0xd007, input_port_4_r },
MEMORY_END

static MEMORY_WRITE_START( fhawk_2_writemem )
	{ 0x0000, 0xbfff, MWA_ROM },
	{ 0xc000, 0xc000, rombank2switch_w },
	{ 0xc800, 0xc800, taitosound_port_w },
	{ 0xc801, 0xc801, taitosound_comm_w },
	{ 0xd000, 0xd000, MWA_NOP },	/* Direct copy of input port 0*/
	{ 0xd004, 0xd004, control2_w },
	{ 0xd005, 0xd006, MWA_NOP },	/* Always 0*/
	{ 0xe000, 0xffff, shared_w },
MEMORY_END

static MEMORY_READ_START( fhawk_3_readmem )
	{ 0x0000, 0x3fff, MRA_ROM },
	{ 0x4000, 0x7fff, MRA_BANK7 },
	{ 0x8000, 0x9fff, MRA_RAM },
	{ 0xe000, 0xe000, MRA_NOP },
	{ 0xe001, 0xe001, taitosound_slave_comm_r },
	{ 0xf000, 0xf000, YM2203_status_port_0_r },
MEMORY_END

static MEMORY_WRITE_START( fhawk_3_writemem )
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0x8000, 0x9fff, MWA_RAM },
	{ 0xe000, 0xe000, taitosound_slave_port_w },
	{ 0xe001, 0xe001, taitosound_slave_comm_w },
	{ 0xf000, 0xf000, YM2203_control_port_0_w },
	{ 0xf001, 0xf001, YM2203_write_port_0_w },
MEMORY_END

static MEMORY_READ_START( raimais_readmem )
	COMMON_BANKS_READ,
	{ 0x8000, 0x87ff, MRA_RAM },
	{ 0x8800, 0x8800, mux_r },
	{ 0x8801, 0x8801, MRA_NOP },	/* Watchdog or interrupt ack (value ignored)*/
	{ 0x8c00, 0x8c00, MRA_NOP },
	{ 0x8c01, 0x8c01, taitosound_comm_r },
	{ 0xa000, 0xbfff, MRA_RAM },
MEMORY_END
static MEMORY_WRITE_START( raimais_writemem )
	COMMON_BANKS_WRITE,
	{ 0x8000, 0x87ff, MWA_RAM, &shared_ram },
	{ 0x8800, 0x8800, mux_w },
	{ 0x8801, 0x8801, mux_ctrl_w },
	{ 0x8c00, 0x8c00, taitosound_port_w },
	{ 0x8c01, 0x8c01, taitosound_comm_w },
	{ 0xa000, 0xbfff, MWA_RAM },
MEMORY_END

static MEMORY_READ_START( raimais_2_readmem )
	{ 0x0000, 0xbfff, MRA_ROM },
	{ 0xc000, 0xdfff, MRA_RAM },
	{ 0xe000, 0xe7ff, shared_r },
MEMORY_END

static MEMORY_WRITE_START( raimais_2_writemem )
	{ 0x0000, 0xbfff, MWA_ROM },
	{ 0xc000, 0xdfff, MWA_RAM },
	{ 0xe000, 0xe7ff, shared_w },
MEMORY_END


static MEMORY_READ_START( raimais_3_readmem )
	{ 0x0000, 0x3fff, MRA_ROM },
	{ 0x4000, 0x7fff, MRA_BANK7 },
	{ 0xc000, 0xdfff, MRA_RAM },
	{ 0xe000, 0xe000, YM2610_status_port_0_A_r },
	{ 0xe001, 0xe001, YM2610_read_port_0_r },
	{ 0xe002, 0xe002, YM2610_status_port_0_B_r },
	{ 0xe200, 0xe200, MRA_NOP },
	{ 0xe201, 0xe201, taitosound_slave_comm_r },
MEMORY_END

static WRITE_HANDLER( sound_bankswitch_w )
{
	unsigned char *RAM = memory_region(REGION_CPU2);
	int banknum = (data - 1) & 3;

	cpu_setbank (7, &RAM [0x10000 + (banknum * 0x4000)]);
}

static MEMORY_WRITE_START( raimais_3_writemem )
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0xc000, 0xdfff, MWA_RAM },
	{ 0xe000, 0xe000, YM2610_control_port_0_A_w },
	{ 0xe001, 0xe001, YM2610_data_port_0_A_w },
	{ 0xe002, 0xe002, YM2610_control_port_0_B_w },
	{ 0xe003, 0xe003, YM2610_data_port_0_B_w },
	{ 0xe200, 0xe200, taitosound_slave_port_w },
	{ 0xe201, 0xe201, taitosound_slave_comm_w },
	{ 0xe400, 0xe403, MWA_NOP }, /* pan */
	{ 0xe600, 0xe600, MWA_NOP }, /* ? */
	{ 0xee00, 0xee00, MWA_NOP }, /* ? */
	{ 0xf000, 0xf000, MWA_NOP }, /* ? */
	{ 0xf200, 0xf200, sound_bankswitch_w },
MEMORY_END


static MEMORY_READ_START( champwr_readmem )
	COMMON_BANKS_READ,
	{ 0x8000, 0x9fff, MRA_RAM },
	{ 0xa000, 0xbfff, MRA_RAM },
MEMORY_END


static MEMORY_WRITE_START( champwr_writemem )
	COMMON_BANKS_WRITE,
	{ 0x8000, 0x9fff, MWA_RAM },
	{ 0xa000, 0xbfff, MWA_RAM, &shared_ram },
MEMORY_END

static MEMORY_READ_START( champwr_2_readmem )
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0xbfff, MRA_BANK6 },
	{ 0xc000, 0xdfff, shared_r },
	{ 0xe000, 0xe000, input_port_0_r },
	{ 0xe001, 0xe001, input_port_1_r },
	{ 0xe002, 0xe002, input_port_2_r },
	{ 0xe003, 0xe003, input_port_3_r },
	{ 0xe007, 0xe007, input_port_4_r },
	{ 0xe008, 0xe00f, MRA_NOP },
	{ 0xe800, 0xe800, MRA_NOP },
	{ 0xe801, 0xe801, taitosound_comm_r },
	{ 0xf000, 0xf000, rombank2switch_r },
MEMORY_END

static MEMORY_WRITE_START( champwr_2_writemem )
	{ 0x0000, 0xbfff, MWA_ROM },
	{ 0xc000, 0xdfff, shared_w },
	{ 0xe000, 0xe000, MWA_NOP },	/* Watchdog*/
	{ 0xe004, 0xe004, control2_w },
	{ 0xe800, 0xe800, taitosound_port_w },
	{ 0xe801, 0xe801, taitosound_comm_w },
	{ 0xf000, 0xf000, rombank2switch_w },
MEMORY_END

static MEMORY_READ_START( champwr_3_readmem )
	{ 0x0000, 0x3fff, MRA_ROM },
	{ 0x4000, 0x7fff, MRA_BANK7 },
	{ 0x8000, 0x8fff, MRA_RAM },
	{ 0x9000, 0x9000, YM2203_status_port_0_r },
	{ 0xa000, 0xa000, MRA_NOP },
	{ 0xa001, 0xa001, taitosound_slave_comm_r },
MEMORY_END

static MEMORY_WRITE_START( champwr_3_writemem )
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0x8000, 0x8fff, MWA_RAM },
	{ 0x9000, 0x9000, YM2203_control_port_0_w },
	{ 0x9001, 0x9001, YM2203_write_port_0_w },
	{ 0xa000, 0xa000, taitosound_slave_port_w },
	{ 0xa001, 0xa001, taitosound_slave_comm_w },
	{ 0xb000, 0xb000, champwr_adpcm_hi_w },
	{ 0xc000, 0xc000, champwr_adpcm_lo_w },
	{ 0xd000, 0xd000, MWA_NOP },	/* ADPCM related */
	{ 0xe000, 0xe000, MWA_NOP },	/* ADPCM related */
MEMORY_END



static MEMORY_READ_START( kurikint_readmem )
	COMMON_BANKS_READ,
	{ 0x8000, 0x9fff, MRA_RAM },
	{ 0xa000, 0xa7ff, MRA_RAM },
	{ 0xa800, 0xa800, mux_r },
	{ 0xa801, 0xa801, MRA_NOP },	/* Watchdog or interrupt ack (value ignored)*/
MEMORY_END

static MEMORY_WRITE_START( kurikint_writemem )
	COMMON_BANKS_WRITE,
	{ 0x8000, 0x9fff, MWA_RAM },
	{ 0xa000, 0xa7ff, MWA_RAM, &shared_ram },
	{ 0xa800, 0xa800, mux_w },
	{ 0xa801, 0xa801, mux_ctrl_w },
MEMORY_END

static MEMORY_READ_START( kurikint_2_readmem )
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0xc000, 0xdfff, MRA_RAM },
	{ 0xe000, 0xe7ff, shared_r },
	{ 0xe800, 0xe800, YM2203_status_port_0_r },
#if 0
	{ 0xd000, 0xd000, input_port_0_r },
	{ 0xd001, 0xd001, input_port_1_r },
	{ 0xd002, 0xd002, input_port_2_r },
	{ 0xd003, 0xd003, input_port_3_r },
	{ 0xd007, 0xd007, input_port_4_r },
#endif
MEMORY_END

static MEMORY_WRITE_START( kurikint_2_writemem )
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0xc000, 0xdfff, MWA_RAM },
	{ 0xe000, 0xe7ff, shared_w },
	{ 0xe800, 0xe800, YM2203_control_port_0_w },
	{ 0xe801, 0xe801, YM2203_write_port_0_w },
#if 0
	{ 0xc000, 0xc000, rombank2switch_w },
#endif
MEMORY_END



static MEMORY_READ_START( puzznic_readmem )
	COMMON_BANKS_READ,
	COMMON_SINGLE_READ,
	{ 0xa800, 0xa800, MRA_NOP },	/* Watchdog*/
	{ 0xb000, 0xb7ff, MRA_RAM },	/* Wrong, used to overcome protection*/
	{ 0xb800, 0xb800, mcu_data_r },
	{ 0xb801, 0xb801, mcu_control_r },
MEMORY_END

static MEMORY_WRITE_START( puzznic_writemem )
	COMMON_BANKS_WRITE,
	COMMON_SINGLE_WRITE,
	{ 0xb000, 0xb7ff, MWA_RAM },	/* Wrong, used to overcome protection*/
	{ 0xb800, 0xb800, mcu_data_w },
	{ 0xb801, 0xb801, mcu_control_w },
	{ 0xbc00, 0xbc00, MWA_NOP },	/* Control register, function unknown*/
MEMORY_END


static MEMORY_READ_START( plotting_readmem )
	COMMON_BANKS_READ,
	COMMON_SINGLE_READ,
MEMORY_END

static MEMORY_WRITE_START( plotting_writemem )
	COMMON_BANKS_WRITE,
	COMMON_SINGLE_WRITE,
	{ 0xa800, 0xa800, MWA_NOP },	/* Watchdog or interrupt ack*/
	{ 0xb800, 0xb800, MWA_NOP },	/* Control register, function unknown*/
MEMORY_END


static MEMORY_READ_START( palamed_readmem )
	COMMON_BANKS_READ,
	COMMON_SINGLE_READ,
	{ 0xa800, 0xa800, input_port_2_r },
	{ 0xa801, 0xa801, input_port_3_r },
	{ 0xa802, 0xa802, input_port_4_r },
	{ 0xb001, 0xb001, MRA_NOP },	/* Watchdog or interrupt ack*/
MEMORY_END

static MEMORY_WRITE_START( palamed_writemem )
	COMMON_BANKS_WRITE,
	COMMON_SINGLE_WRITE,
	{ 0xa803, 0xa803, MWA_NOP },	/* Control register, function unknown*/
	{ 0xb000, 0xb000, MWA_NOP },	/* Control register, function unknown (copy of 8822)*/
MEMORY_END


static MEMORY_READ_START( cachat_readmem )
	COMMON_BANKS_READ,
	COMMON_SINGLE_READ,
	{ 0xa800, 0xa800, input_port_2_r },
	{ 0xa801, 0xa801, input_port_3_r },
	{ 0xa802, 0xa802, input_port_4_r },
	{ 0xb001, 0xb001, MRA_NOP },	/* Watchdog or interrupt ack (value ignored)*/
	{ 0xfff8, 0xfff8, rombankswitch_r },
MEMORY_END

static MEMORY_WRITE_START( cachat_writemem )
	COMMON_BANKS_WRITE,
	COMMON_SINGLE_WRITE,
	{ 0xa803, 0xa803, MWA_NOP },	/* Control register, function unknown*/
	{ 0xb000, 0xb000, MWA_NOP },	/* Control register, function unknown*/
	{ 0xfff8, 0xfff8, rombankswitch_w },
MEMORY_END


static MEMORY_READ_START( horshoes_readmem )
	COMMON_BANKS_READ,
	COMMON_SINGLE_READ,
	{ 0xa800, 0xa800, horshoes_tracky_lo_r },
	{ 0xa802, 0xa802, horshoes_tracky_reset_r },
	{ 0xa803, 0xa803, horshoes_trackx_reset_r },
	{ 0xa804, 0xa804, horshoes_tracky_hi_r },
	{ 0xa808, 0xa808, horshoes_trackx_lo_r },
	{ 0xa80c, 0xa80c, horshoes_trackx_hi_r },
	{ 0xb801, 0xb801, MRA_NOP },	/* Watchdog or interrupt ack*/
MEMORY_END

static MEMORY_WRITE_START( horshoes_writemem )
	COMMON_BANKS_WRITE,
	COMMON_SINGLE_WRITE,
	{ 0xb802, 0xb802, horshoes_bankg_w },
	{ 0xbc00, 0xbc00, MWA_NOP },
MEMORY_END

static MEMORY_READ_START( evilston_readmem )
	COMMON_BANKS_READ,
	{ 0x8000, 0x9fff, MRA_RAM },
	{ 0xa000, 0xa7ff, MRA_RAM },
	{ 0xa800, 0xa800, input_port_0_r },
	{ 0xa801, 0xa801, input_port_1_r },
	{ 0xa802, 0xa802, input_port_2_r },
	{ 0xa803, 0xa803, input_port_3_r },
	{ 0xa807, 0xa807, input_port_4_r },
MEMORY_END



static WRITE_HANDLER (evilston_snd_w)
{
	shared_ram[0x7fe]=data&0x7f;
	cpu_set_irq_line(1,IRQ_LINE_NMI,PULSE_LINE);
}




static MEMORY_WRITE_START( evilston_writemem )
	COMMON_BANKS_WRITE,
	{ 0x8000, 0x9fff, MWA_RAM },
	{ 0xa000, 0xa7ff, MWA_RAM,&shared_ram},/*shared2_w },*/
	{ 0xa800, 0xa800, MWA_RAM },/*watchdog ?*/
	{ 0xa804, 0xa804, MWA_RAM}, /*coin couters/locks ?*/

MEMORY_END

static MEMORY_READ_START( evilston_2_readmem )
	{ 0x0000, 0xbfff, MRA_ROM },
	{ 0xc000, 0xdfff, MRA_RAM },
	{ 0xe000, 0xe7ff, shared_r},/*shared_r },*/
	{ 0xe800, 0xe800, YM2203_status_port_0_r },
	{ 0xf000, 0xf7ff, MRA_BANK7 },
MEMORY_END

static MEMORY_WRITE_START( evilston_2_writemem )
	{ 0x0000, 0xbfff, MWA_ROM },
	{ 0xc000, 0xdfff, MWA_RAM },
	{ 0xe000, 0xe7ff, shared_w },
	{ 0xe800, 0xe800, YM2203_control_port_0_w },
	{ 0xe801, 0xe801, YM2203_write_port_0_w },
MEMORY_END



/***********************************************************
			 INPUT PORTS, DIPs
***********************************************************/

#define TAITO_COINAGE_WORLD_8 \
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Coin_A ) ) \
	PORT_DIPSETTING(    0x00, DEF_STR( 4C_1C ) ) \
	PORT_DIPSETTING(    0x10, DEF_STR( 3C_1C ) ) \
	PORT_DIPSETTING(    0x20, DEF_STR( 2C_1C ) ) \
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_1C ) ) \
	PORT_DIPNAME( 0xc0, 0xc0, DEF_STR( Coin_B ) ) \
	PORT_DIPSETTING(    0xc0, DEF_STR( 1C_2C ) ) \
	PORT_DIPSETTING(    0x80, DEF_STR( 1C_3C ) ) \
	PORT_DIPSETTING(    0x40, DEF_STR( 1C_4C ) ) \
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_6C ) )

#define TAITO_COINAGE_JAPAN_8 \
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Coin_A ) ) \
	PORT_DIPSETTING(    0x10, DEF_STR( 2C_1C ) ) \
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_1C ) ) \
	PORT_DIPSETTING(    0x00, DEF_STR( 2C_3C ) ) \
	PORT_DIPSETTING(    0x20, DEF_STR( 1C_2C ) ) \
	PORT_DIPNAME( 0xc0, 0xc0, DEF_STR( Coin_B ) ) \
	PORT_DIPSETTING(    0x40, DEF_STR( 2C_1C ) ) \
	PORT_DIPSETTING(    0xc0, DEF_STR( 1C_1C ) ) \
	PORT_DIPSETTING(    0x00, DEF_STR( 2C_3C ) ) \
	PORT_DIPSETTING(    0x80, DEF_STR( 1C_2C ) )

#define TAITO_COINAGE_JAPAN_NEW_8 \
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Coin_A ) ) \
	PORT_DIPSETTING(    0x00, DEF_STR( 3C_1C ) ) \
	PORT_DIPSETTING(    0x10, DEF_STR( 2C_1C ) ) \
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_1C ) ) \
	PORT_DIPSETTING(    0x20, DEF_STR( 1C_2C ) ) \
	PORT_DIPNAME( 0xc0, 0xc0, DEF_STR( Coin_B ) ) \
	PORT_DIPSETTING(    0x00, DEF_STR( 3C_1C ) ) \
	PORT_DIPSETTING(    0x40, DEF_STR( 2C_1C ) ) \
	PORT_DIPSETTING(    0xc0, DEF_STR( 1C_1C ) ) \
	PORT_DIPSETTING(    0x80, DEF_STR( 1C_2C ) )

#define TAITO_COINAGE_US_8 \
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Coinage ) ) \
	PORT_DIPSETTING(    0x00, DEF_STR( 4C_1C ) ) \
	PORT_DIPSETTING(    0x10, DEF_STR( 3C_1C ) ) \
	PORT_DIPSETTING(    0x20, DEF_STR( 2C_1C ) ) \
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_1C ) ) \
	PORT_DIPNAME( 0xc0, 0xc0, "Price to Continue" ) \
	PORT_DIPSETTING(    0x00, DEF_STR( 3C_1C ) ) \
	PORT_DIPSETTING(    0x40, DEF_STR( 2C_1C ) ) \
	PORT_DIPSETTING(    0x80, DEF_STR( 1C_1C ) ) \
	PORT_DIPSETTING(    0xc0, "Same as Start" )

#define TAITO_DIFFICULTY_8 \
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Difficulty ) ) \
	PORT_DIPSETTING(    0x02, "Easy" ) \
	PORT_DIPSETTING(    0x03, "Medium" ) \
	PORT_DIPSETTING(    0x01, "Hard" ) \
	PORT_DIPSETTING(    0x00, "Hardest" )

#define TAITO_L_PLAYERS_INPUT( player ) \
	PORT_START \
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | player ) \
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | player ) \
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | player ) \
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | player ) \
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | player ) \
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | player ) \
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN ) \
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

#define TAITO_L_SYSTEM_INPUT( type, impulse ) \
	PORT_START \
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_TILT ) \
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_SERVICE1 ) \
	PORT_BIT_IMPULSE( 0x04, type, IPT_COIN1, impulse ) \
	PORT_BIT_IMPULSE( 0x08, type, IPT_COIN2, impulse ) \
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN ) \
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN ) \
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START1 ) \
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START2 )

#define TAITO_L_DSWA_2_4 \
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Flip_Screen ) ) \
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) ) \
	PORT_DIPSETTING(    0x00, DEF_STR( On ) ) \
	PORT_SERVICE( 0x04, IP_ACTIVE_LOW ) \
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Demo_Sounds ) ) \
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) ) \
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )

INPUT_PORTS_START( fhawk )
	PORT_START
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Cocktail ) )
	TAITO_L_DSWA_2_4
	TAITO_COINAGE_WORLD_8

	PORT_START
	TAITO_DIFFICULTY_8
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unused ) )  /* all in manual*/
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x30, "3" )
	PORT_DIPSETTING(    0x20, "4" )
	PORT_DIPSETTING(    0x10, "5" )
	PORT_DIPSETTING(    0x00, "6" )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	TAITO_L_PLAYERS_INPUT( IPF_PLAYER1 )

	TAITO_L_PLAYERS_INPUT( IPF_PLAYER2 )

	TAITO_L_SYSTEM_INPUT( IP_ACTIVE_LOW, 4 )
INPUT_PORTS_END

INPUT_PORTS_START( fhawkj )
	PORT_START
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Cocktail ) )
	TAITO_L_DSWA_2_4
	TAITO_COINAGE_JAPAN_8

	PORT_START
	TAITO_DIFFICULTY_8
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unused ) )  /* all in manual*/
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x30, "3" )
	PORT_DIPSETTING(    0x20, "4" )
	PORT_DIPSETTING(    0x10, "5" )
	PORT_DIPSETTING(    0x00, "6" )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	TAITO_L_PLAYERS_INPUT( IPF_PLAYER1 )

	TAITO_L_PLAYERS_INPUT( IPF_PLAYER2 )

	TAITO_L_SYSTEM_INPUT( IP_ACTIVE_LOW, 4 )
INPUT_PORTS_END

INPUT_PORTS_START( raimais )
	PORT_START
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Cocktail ) )
	TAITO_L_DSWA_2_4
	TAITO_COINAGE_JAPAN_8

	PORT_START
	TAITO_DIFFICULTY_8
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x08, "80k and 160k" )
	PORT_DIPSETTING(    0x0c, "80k only" )
	PORT_DIPSETTING(    0x04, "160k only" )
	PORT_DIPSETTING(    0x00, "None" )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x30, "3" )
	PORT_DIPSETTING(    0x20, "4" )
	PORT_DIPSETTING(    0x10, "5" )
	PORT_DIPSETTING(    0x00, "6" )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x00, "Allow Continue" )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	TAITO_L_PLAYERS_INPUT( IPF_PLAYER1 )

	TAITO_L_PLAYERS_INPUT( IPF_PLAYER2 )

	TAITO_L_SYSTEM_INPUT( IP_ACTIVE_HIGH, 1 )
INPUT_PORTS_END

#define CHAMPWR_DSWB \
	PORT_START \
	TAITO_DIFFICULTY_8 \
	PORT_DIPNAME( 0x0c, 0x0c, "Time" ) \
	PORT_DIPSETTING(    0x08, "2 minutes" ) \
	PORT_DIPSETTING(    0x0c, "3 minutes" ) \
	PORT_DIPSETTING(    0x04, "4 minutes" ) \
	PORT_DIPSETTING(    0x00, "5 minutes" ) \
	PORT_DIPNAME( 0x30, 0x30, "1 minute Lenght" ) \
	PORT_DIPSETTING(    0x00, "30 sec" ) \
	PORT_DIPSETTING(    0x10, "40 sec" ) \
	PORT_DIPSETTING(    0x30, "50 sec" ) \
	PORT_DIPSETTING(    0x20, "60 sec" ) \
	PORT_DIPNAME( 0x40, 0x40, "Allow Continue" ) \
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) ) \
	PORT_DIPSETTING(    0x40, DEF_STR( On ) ) \
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unused ) ) \
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) ) \
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

#define CHAMPWR_INPUTS \
 	PORT_START \
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 ) \
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 ) \
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_START1 ) \
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START2 ) \
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 ) \
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 ) \
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_TILT ) \
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_SERVICE1 ) \
 \
	PORT_START \
	PORT_BIT_IMPULSE( 0x01, IP_ACTIVE_LOW, IPT_COIN2, 1 ) \
	PORT_BIT_IMPULSE( 0x02, IP_ACTIVE_LOW, IPT_COIN1, 1 ) \
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN ) \
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN ) \
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN ) \
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN ) \
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN ) \
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN ) \
 \
	PORT_START \
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 ) \
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 ) \
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 ) \
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 ) \
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 ) \
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 ) \
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 ) \
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 )

INPUT_PORTS_START( champwr )
	PORT_START
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unused ) )  /* all 2 in manual*/
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	TAITO_L_DSWA_2_4
	TAITO_COINAGE_WORLD_8

	CHAMPWR_DSWB

	CHAMPWR_INPUTS
INPUT_PORTS_END

INPUT_PORTS_START( champwrj )
	PORT_START
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	TAITO_L_DSWA_2_4
	TAITO_COINAGE_JAPAN_8

	CHAMPWR_DSWB

	CHAMPWR_INPUTS
INPUT_PORTS_END

INPUT_PORTS_START( champwru )
	PORT_START
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	TAITO_L_DSWA_2_4
	TAITO_COINAGE_US_8

	CHAMPWR_DSWB

	CHAMPWR_INPUTS
INPUT_PORTS_END

#define KURIKIN_DSWB \
	PORT_START \
	TAITO_DIFFICULTY_8 \
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) ) \
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) ) \
	PORT_DIPSETTING(    0x00, DEF_STR( On ) ) \
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) ) \
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) ) \
	PORT_DIPSETTING(    0x00, DEF_STR( On ) ) \
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) ) \
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) ) \
	PORT_DIPSETTING(    0x00, DEF_STR( On ) ) \
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) ) \
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) ) \
	PORT_DIPSETTING(    0x00, DEF_STR( On ) ) \
	PORT_DIPNAME( 0x40, 0x40, "Bosses' messages" ) \
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) ) \
	PORT_DIPSETTING(    0x40, DEF_STR( On ) ) \
	PORT_DIPNAME( 0x80, 0x80, "Allow Continue" ) \
	PORT_DIPSETTING(    0x80, "5 Times" ) \
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

INPUT_PORTS_START( kurikint )
	PORT_START
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Cocktail ) )
	TAITO_L_DSWA_2_4
	TAITO_COINAGE_WORLD_8

	KURIKIN_DSWB

	TAITO_L_PLAYERS_INPUT( IPF_PLAYER1 )

	TAITO_L_PLAYERS_INPUT( IPF_PLAYER2 )

	TAITO_L_SYSTEM_INPUT( IP_ACTIVE_HIGH, 4 )
INPUT_PORTS_END

INPUT_PORTS_START( kurikinj )
	PORT_START
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Cocktail ) )
	TAITO_L_DSWA_2_4
	TAITO_COINAGE_JAPAN_8

	KURIKIN_DSWB

	TAITO_L_PLAYERS_INPUT( IPF_PLAYER1 )

	TAITO_L_PLAYERS_INPUT( IPF_PLAYER2 )

	TAITO_L_SYSTEM_INPUT( IP_ACTIVE_HIGH, 4 )
INPUT_PORTS_END

INPUT_PORTS_START( kurikina )
	PORT_START
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x02, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	TAITO_COINAGE_WORLD_8

	PORT_START
	PORT_BITX(    0x01, 0x01, IPT_DIPSWITCH_NAME | IPF_CHEAT, "Level select", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_BITX(    0x02, 0x02, IPT_DIPSWITCH_NAME | IPF_CHEAT, "Invulnerability", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x08, "Easy" )
	PORT_DIPSETTING(    0x0c, "Medium" )
	PORT_DIPSETTING(    0x04, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, "Bosses' messages" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x20, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_BITX(    0x80, 0x80, IPT_DIPSWITCH_NAME | IPF_CHEAT, "Slow Motion", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	TAITO_L_PLAYERS_INPUT( IPF_PLAYER1 )

	TAITO_L_PLAYERS_INPUT( IPF_PLAYER2 )

	TAITO_L_SYSTEM_INPUT( IP_ACTIVE_HIGH, 4 )
INPUT_PORTS_END

INPUT_PORTS_START( puzznic )
	PORT_START
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Cocktail ) )
	TAITO_L_DSWA_2_4
	/* There is no Coin B in the Manuals */
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 1C_2C ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START
	/* Difficulty controls the Timer Speed (how many seconds are there in a minute) */
	TAITO_DIFFICULTY_8
	PORT_DIPNAME( 0x0c, 0x0c, "Retries" )
	PORT_DIPSETTING(    0x00, "0" )
	PORT_DIPSETTING(    0x04, "1" )
	PORT_DIPSETTING(    0x0c, "2" )
	PORT_DIPSETTING(    0x08, "3" )
	PORT_DIPNAME( 0x10, 0x10, "Bombs" )
	PORT_DIPSETTING(    0x10, "0" )
	PORT_DIPSETTING(    0x00, "2" )
	PORT_DIPNAME( 0x20, 0x20, "Girls" )
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Yes ) )
	PORT_DIPNAME( 0xc0, 0xc0, "Terms of Replay" )
	PORT_DIPSETTING(    0x40, "Stage one step back/Timer continuous" )
	PORT_DIPSETTING(    0xc0, "Stage reset to start/Timer continuous" )
	PORT_DIPSETTING(    0x80, "Stage reset to start/Timer reset to start" )
/*	PORT_DIPSETTING(    0x00, "No Use" )*/

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )

	PORT_START /* Not read yet. There is no Coin_B in manual */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END

INPUT_PORTS_START( plotting )
	PORT_START
	PORT_DIPNAME( 0x01, 0x01, "Max Players" )
	PORT_DIPSETTING(    0x00, "1" )
	PORT_DIPSETTING(    0x01, "2" )
	TAITO_L_DSWA_2_4
	TAITO_COINAGE_WORLD_8

	PORT_START
	TAITO_DIFFICULTY_8
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x30, 0x30, "Misses" )
	PORT_DIPSETTING(    0x20, "1" )
	PORT_DIPSETTING(    0x30, "2" )
	PORT_DIPSETTING(    0x10, "3" )
	PORT_DIPSETTING(    0x00, "4" )
	PORT_DIPNAME( 0x40, 0x40, "Allow Continue" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x80, DEF_STR( On ) )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_TILT )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
INPUT_PORTS_END

INPUT_PORTS_START( palamed )
	PORT_START
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	TAITO_L_DSWA_2_4
	TAITO_COINAGE_JAPAN_NEW_8

	PORT_START
	/* Difficulty controls how faster falls the dice lines */
	TAITO_DIFFICULTY_8
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, "Versus Mode" )
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Yes ) )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_TILT )
	PORT_BIT_IMPULSE( 0x04, IP_ACTIVE_LOW, IPT_COIN1, 1 )
	PORT_BIT_IMPULSE( 0x08, IP_ACTIVE_LOW, IPT_COIN2, 1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END

INPUT_PORTS_START( cachat )
	PORT_START
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x02, DEF_STR( On ) )
	PORT_SERVICE( 0x04, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	TAITO_COINAGE_JAPAN_NEW_8

	PORT_START
	TAITO_DIFFICULTY_8
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_TILT )
	PORT_BIT_IMPULSE( 0x04, IP_ACTIVE_LOW, IPT_COIN1, 1 )
	PORT_BIT_IMPULSE( 0x08, IP_ACTIVE_LOW, IPT_COIN2, 1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END

INPUT_PORTS_START( tubeit )
	PORT_START
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x02, DEF_STR( On ) )
	PORT_SERVICE( 0x04, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	TAITO_COINAGE_WORLD_8

	PORT_START
	TAITO_DIFFICULTY_8
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_TILT )
	PORT_BIT_IMPULSE( 0x04, IP_ACTIVE_LOW, IPT_COIN1, 1 )
	PORT_BIT_IMPULSE( 0x08, IP_ACTIVE_LOW, IPT_COIN2, 1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END

INPUT_PORTS_START( horshoes )
	PORT_START
	PORT_DIPNAME( 0x01, 0x01, "Beer Frame Message" )
	PORT_DIPSETTING(    0x01, "Break Time" )
	PORT_DIPSETTING(    0x00, "Beer Frame" )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_SERVICE( 0x04, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	TAITO_COINAGE_US_8	/* According to the "United States Version" manual listing */

	PORT_START
	/* Not for sure, the CPU seems to play better when set to Hardest */
	TAITO_DIFFICULTY_8
	PORT_DIPNAME( 0x0c, 0x0c, "Time" )
	PORT_DIPSETTING(    0x08, "20 sec" )
	PORT_DIPSETTING(    0x0c, "30 sec" )
	PORT_DIPSETTING(    0x04, "40 sec" )
	PORT_DIPSETTING(    0x00, "60 sec" )
	PORT_DIPNAME( 0x10, 0x10, "Innings" )
	PORT_DIPSETTING(    0x10, "3 per Credit" )
	PORT_DIPSETTING(    0x00, "9 per Credit" )
	PORT_DIPNAME( 0x20, 0x20, "Bonus Advantage" )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, "Scoring Speed" )
	PORT_DIPSETTING(    0x40, "Normal" )
	PORT_DIPSETTING(    0x00, "Fast" )
	PORT_DIPNAME( 0x80, 0x80, "Grip/Angle Select" )
	PORT_DIPSETTING(    0x80, "2 Buttons" )
	PORT_DIPSETTING(    0x00, "1 Button" )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_TILT )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )

	PORT_START
	PORT_ANALOG( 0xffff, 0x0000, IPT_TRACKBALL_Y | IPF_REVERSE, 50, 30, 0, 0 )

	PORT_START
	PORT_ANALOG( 0xffff, 0x0000, IPT_TRACKBALL_X, 50, 30, 0, 0 )
INPUT_PORTS_END

INPUT_PORTS_START( plgirls )
	PORT_START
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x01, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_SERVICE( 0x04, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x38, 0x38, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x18, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x38, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x28, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 1C_4C ) )
/*	PORT_DIPSETTING(    0x30, DEF_STR( 1C_1C ) )*/
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START
	/* Difficulty controls the Ball Speed */
	TAITO_DIFFICULTY_8
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_TILT )
	PORT_BIT_IMPULSE( 0x04, IP_ACTIVE_LOW, IPT_COIN1, 1 )
	PORT_BIT_IMPULSE( 0x08, IP_ACTIVE_LOW, IPT_COIN2, 1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END

INPUT_PORTS_START( plgirls2 )
	PORT_START
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_SERVICE( 0x04, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	TAITO_COINAGE_JAPAN_8

	PORT_START
	/* Difficulty controls the number of hits requiered to destroy enemies */
	TAITO_DIFFICULTY_8
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x18, 0x18, "Life" )
	PORT_DIPSETTING(    0x10, "3/2/3" )
	PORT_DIPSETTING(    0x18, "4/3/4" )
	PORT_DIPSETTING(    0x08, "5/4/5" )
	PORT_DIPSETTING(    0x00, "6/5/6" )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_TILT )
	PORT_BIT_IMPULSE( 0x04, IP_ACTIVE_LOW, IPT_COIN1, 1 )
	PORT_BIT_IMPULSE( 0x08, IP_ACTIVE_LOW, IPT_COIN2, 1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END

INPUT_PORTS_START( cubybop )
	PORT_START
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_SERVICE( 0x04, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	TAITO_COINAGE_JAPAN_NEW_8

	PORT_START
	TAITO_DIFFICULTY_8
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_TILT )
	PORT_BIT_IMPULSE( 0x04, IP_ACTIVE_LOW, IPT_COIN1, 1 )
	PORT_BIT_IMPULSE( 0x08, IP_ACTIVE_LOW, IPT_COIN2, 1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END

INPUT_PORTS_START( evilston )

	PORT_START
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Cocktail ) )
	TAITO_L_DSWA_2_4
	TAITO_COINAGE_WORLD_8

	PORT_START
	TAITO_DIFFICULTY_8
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x30, "3" )
	PORT_DIPSETTING(    0x20, "2" )
	PORT_DIPSETTING(    0x10, "1" )
	PORT_DIPSETTING(    0x00, "4" )
  PORT_DIPNAME( 0xc0, 0x00, "Language" )
  PORT_DIPSETTING(    0x00, "English" )
  PORT_DIPSETTING(    0x80, "English" )
  PORT_DIPSETTING(    0x40, "English" )
  PORT_DIPSETTING(    0xc0, "Japanese" )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START1  )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START2  )

	PORT_START

	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_TILT )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT_IMPULSE( 0x04, IP_ACTIVE_LOW, IPT_COIN1, 4 )
	PORT_BIT_IMPULSE( 0x08, IP_ACTIVE_LOW, IPT_COIN2, 4 )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_4WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_4WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_4WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_COCKTAIL | IPF_4WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_COCKTAIL | IPF_4WAY )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_COCKTAIL | IPF_4WAY )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_COCKTAIL | IPF_4WAY )

INPUT_PORTS_END



static struct GfxLayout bg1_layout =
{
	8, 8,
	RGN_FRAC(1,2),
	4,
	{ RGN_FRAC(1,2)+0, RGN_FRAC(1,2)+4, 0, 4 },
	{ 3, 2, 1, 0, 8+3, 8+2, 8+1, 8+0 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16 },
	8*8*2
};

static struct GfxLayout bg2_layout =
{
	8, 8,
	RGN_FRAC(1,1),
	4,
	{ 8, 12, 0, 4 },
	{ 3, 2, 1, 0, 19, 18, 17, 16 },
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32 },
	8*8*4
};

#define O 8*8*2
#define O2 2*O
static struct GfxLayout sp1_layout =
{
	16, 16,
	RGN_FRAC(1,2),
	4,
	{ RGN_FRAC(1,2)+0, RGN_FRAC(1,2)+4, 0, 4 },
	{ 3, 2, 1, 0, 8+3, 8+2, 8+1, 8+0, O+3, O+2, O+1, O+0, O+8+3, O+8+2, O+8+1, O+8+0 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16, O2+0*16, O2+1*16, O2+2*16, O2+3*16, O2+4*16, O2+5*16, O2+6*16, O2+7*16 },
	8*8*2*4
};
#undef O
#undef O2

#define O 8*8*4
#define O2 2*O
static struct GfxLayout sp2_layout =
{
	16, 16,
	RGN_FRAC(1,1),
	4,
	{ 8, 12, 0, 4 },
	{ 3, 2, 1, 0, 19, 18, 17, 16, O+3, O+2, O+1, O+0, O+19, O+18, O+17, O+16 },
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32, O2+0*32, O2+1*32, O2+2*32, O2+3*32, O2+4*32, O2+5*32, O2+6*32, O2+7*32 },
	8*8*4*4
};
#undef O
#undef O2

static struct GfxLayout char_layout =
{
	8, 8,
	1024,
	4,
	{ 8, 12, 0, 4 },
	{ 3, 2, 1, 0, 19, 18, 17, 16},
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32 },
	8*8*4
};

static struct GfxDecodeInfo gfxdecodeinfo1[] =
{
	{ REGION_GFX1, 0, &bg1_layout, 0, 16 },
	{ REGION_GFX1, 0, &sp1_layout, 0, 16 },
	{ 0,           0, &char_layout,  0, 16 },  /* Ram-based*/
	{ -1 }
};

static struct GfxDecodeInfo gfxdecodeinfo2[] =
{
	{ REGION_GFX1, 0, &bg2_layout, 0, 16 },
	{ REGION_GFX1, 0, &sp2_layout, 0, 16 },
	{ 0,           0, &char_layout,  0, 16 },  /* Ram-based*/
	{ -1 }
};



static void irqhandler(int irq)
{
	cpu_set_irq_line(1,0,irq ? ASSERT_LINE : CLEAR_LINE);
}

static WRITE_HANDLER( portA_w )
{
	static int cur_bank = 0;

	if (cur_bank != (data & 0x03) )
	{
		int bankaddress;
		unsigned char *RAM = memory_region(REGION_CPU2);

		cur_bank = data & 0x03;
		bankaddress = 0x10000 + (cur_bank-1) * 0x4000;
		cpu_setbank(7,&RAM[bankaddress]);
		/*logerror ("YM2203 bank change val=%02x  pc=%04x\n",cur_bank, activecpu_get_pc() );*/
	}
}

static struct YM2203interface ym2203_interface_triple =
{
	1,			/* 1 chip */
	3000000,	/* ??? */
	{ YM2203_VOL(80,20) },
	{ 0 },
	{ 0 },
	{ portA_w },
	{ 0 },
	{ irqhandler }
};

static struct ADPCMinterface adpcm_interface =
{
	1,			/* 1 channel */
	8000,		/* 8000Hz playback? */
	REGION_SOUND1,	/* memory region */
	{ 80 } 	/* volume */
};


static struct YM2610interface ym2610_interface =
{
	1,	/* 1 chip */
	8000000,	/* 8 MHz */
	{ 25 },
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 },
	{ irqhandler },
	{ REGION_SOUND1 },
	{ REGION_SOUND1 },
	{ YM3012_VOL(100,MIXER_PAN_LEFT,100,MIXER_PAN_RIGHT) }
};

static struct YM2203interface ym2203_interface_double =
{
	1,			/* 1 chip */
	3000000,	/* ??? */
	{ YM2203_VOL(80,20) },
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 }
};

static struct YM2203interface ym2203_interface_evilston =
{
	1,			/* 1 chip */
	3000000,	/* ??? */
	{ YM2203_VOL(80,0) },
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 }
};



static struct YM2203interface ym2203_interface_single =
{
	1,			/* 1 chip */
	3000000,	/* ??? */
	{ YM2203_VOL(80,20) },
	{ portA_r },
	{ portB_r },
	{ 0 },
	{ 0 },
	{ 0 }
};


static MACHINE_DRIVER_START( fhawk )

	/* basic machine hardware */
	MDRV_CPU_ADD_TAG("cpu1", Z80, 6665280)	/* ? xtal is 13.33056 */
	MDRV_CPU_MEMORY(fhawk_readmem,fhawk_writemem)
	MDRV_CPU_VBLANK_INT(vbl_interrupt,3)

	MDRV_CPU_ADD_TAG("sound", Z80, 4000000)	/* ? xtal is 13.33056 */
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
	MDRV_CPU_MEMORY(fhawk_3_readmem,fhawk_3_writemem)

	MDRV_CPU_ADD_TAG("cpu2", Z80, 4000000)	/* ? xtal is 13.33056 */
	MDRV_CPU_MEMORY(fhawk_2_readmem,fhawk_2_writemem)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,3)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(100)

	MDRV_MACHINE_INIT(fhawk)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(40*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 40*8-1, 2*8, 30*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo2)
	MDRV_PALETTE_LENGTH(256)

	MDRV_VIDEO_START(taitol)
	MDRV_VIDEO_EOF(taitol)
	MDRV_VIDEO_UPDATE(taitol)

	/* sound hardware */
	MDRV_SOUND_ADD_TAG("2203", YM2203, ym2203_interface_triple)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( champwr )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(fhawk)
	MDRV_CPU_MODIFY("cpu1")
	MDRV_CPU_MEMORY(champwr_readmem,champwr_writemem)

	MDRV_CPU_MODIFY("sound")
	MDRV_CPU_MEMORY(champwr_3_readmem,champwr_3_writemem)

	MDRV_CPU_MODIFY("cpu2")
	MDRV_CPU_MEMORY(champwr_2_readmem,champwr_2_writemem)

	MDRV_MACHINE_INIT(champwr)

	/* sound hardware */
	MDRV_SOUND_ADD(ADPCM, adpcm_interface)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( raimais )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(fhawk)
	MDRV_CPU_MODIFY("cpu1")
	MDRV_CPU_MEMORY(raimais_readmem,raimais_writemem)

	MDRV_CPU_MODIFY("sound")
	MDRV_CPU_MEMORY(raimais_3_readmem,raimais_3_writemem)

	MDRV_CPU_MODIFY("cpu2")
	MDRV_CPU_MEMORY(raimais_2_readmem,raimais_2_writemem)

	MDRV_MACHINE_INIT(raimais)

	/* sound hardware */
	MDRV_SOUND_REPLACE("2203", YM2610, ym2610_interface)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( kurikint )

	/* basic machine hardware */
	MDRV_CPU_ADD(Z80, 6000000)	/* ? xtal is 13.33056 */
	MDRV_CPU_MEMORY(kurikint_readmem,kurikint_writemem)
	MDRV_CPU_VBLANK_INT(vbl_interrupt,3)

	MDRV_CPU_ADD(Z80, 6000000)	/* ? xtal is 13.33056 */
	MDRV_CPU_MEMORY(kurikint_2_readmem,kurikint_2_writemem)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,1)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(100)

	MDRV_MACHINE_INIT(kurikint)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(40*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 40*8-1, 2*8, 30*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo2)
	MDRV_PALETTE_LENGTH(256)

	MDRV_VIDEO_START(taitol)
	MDRV_VIDEO_EOF(taitol)
	MDRV_VIDEO_UPDATE(taitol)

	/* sound hardware */
	MDRV_SOUND_ADD(YM2203, ym2203_interface_double)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( kurikina )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(kurikint)

	/* video hardware */
	MDRV_GFXDECODE(gfxdecodeinfo1)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( plotting )

	/* basic machine hardware */
	MDRV_CPU_ADD_TAG("main", Z80, 6000000)	/* ? xtal is 13.33056 */
	MDRV_CPU_MEMORY(plotting_readmem,plotting_writemem)
	MDRV_CPU_VBLANK_INT(vbl_interrupt,3)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_MACHINE_INIT(plotting)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(40*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 40*8-1, 2*8, 30*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo1)
	MDRV_PALETTE_LENGTH(256)

	MDRV_VIDEO_START(taitol)
	MDRV_VIDEO_EOF(taitol)
	MDRV_VIDEO_UPDATE(taitol)

	/* sound hardware */
	MDRV_SOUND_ADD(YM2203, ym2203_interface_single)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( puzznic )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(plotting)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_MEMORY(puzznic_readmem,puzznic_writemem)

	MDRV_MACHINE_INIT(puzznic)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( horshoes )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(plotting)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_MEMORY(horshoes_readmem,horshoes_writemem)

	MDRV_MACHINE_INIT(horshoes)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( palamed )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(plotting)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_MEMORY(palamed_readmem,palamed_writemem)

	MDRV_MACHINE_INIT(palamed)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( cachat )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(plotting)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_MEMORY(cachat_readmem,cachat_writemem)

	MDRV_MACHINE_INIT(cachat)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( evilston )

	/* basic machine hardware */
	MDRV_CPU_ADD(Z80, 6000000)
	MDRV_CPU_MEMORY(evilston_readmem,evilston_writemem)
	MDRV_CPU_VBLANK_INT(vbl_interrupt,3)

	MDRV_CPU_ADD(Z80, 6000000)
	MDRV_CPU_MEMORY(evilston_2_readmem,evilston_2_writemem)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,1)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(100)

	MDRV_MACHINE_INIT(evilston)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(40*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 40*8-1, 2*8, 30*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo2)
	MDRV_PALETTE_LENGTH(256)

	MDRV_VIDEO_START(taitol)
	MDRV_VIDEO_EOF(taitol)
	MDRV_VIDEO_UPDATE(taitol)

	/* sound hardware */
	MDRV_SOUND_ADD(YM2203, ym2203_interface_evilston)
MACHINE_DRIVER_END




ROM_START( raimais )
	ROM_REGION( 0xb0000, REGION_CPU1, 0 )
	ROM_LOAD( "b36-08-1.bin", 0x00000, 0x20000, CRC(6cc8f79f) SHA1(17b4903f87e6d5447c8557c2baca1728f86245dc) )
	ROM_RELOAD(               0x10000, 0x20000 )
	ROM_LOAD( "b36-09.bin",   0x30000, 0x20000, CRC(9c466e43) SHA1(2466a3f1f8124323008c9925f90e9a1d2edf1564) )

	ROM_REGION( 0x1c000, REGION_CPU2, 0 )	/* sound (sndhrdw/rastan.c wants it as #2 */
	ROM_LOAD( "b36-06.bin",   0x00000, 0x4000, CRC(29bbc4f8) SHA1(39a68729c6180c5f6cdf604e692018e7d6bf5591) )
	ROM_CONTINUE(             0x10000, 0xc000 )

	ROM_REGION( 0x10000, REGION_CPU3, 0 )
	ROM_LOAD( "b36-07.bin",   0x00000, 0x10000, CRC(4f3737e6) SHA1(ff5f5d4ca5485441d03c8cb01a6a096941ab02eb) )

	ROM_REGION( 0x100000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "b36-01.bin",   0x00000, 0x80000, CRC(89355cb2) SHA1(433e929fe8b488af84e88486d9679468a3d9677a) )
	ROM_LOAD( "b36-02.bin",   0x80000, 0x80000, CRC(e71da5db) SHA1(aa47ae02c359264c0a1f09ecc583eefd1ef1dfa4) )

	ROM_REGION( 0x80000, REGION_SOUND1, 0 )
	ROM_LOAD( "b36-03.bin",   0x00000, 0x80000, CRC(96166516) SHA1(a6748218188cbd1b037f6c0845416665c0d55a7b) )
ROM_END

ROM_START( fhawk )
	ROM_REGION( 0xb0000, REGION_CPU1, 0 )
	ROM_LOAD( "b70-11.bin", 0x00000, 0x20000, CRC(7d9f7583) SHA1(d8fa7c66a81fb356fa9c72f377bfc31b1837eafb) )
	ROM_RELOAD(             0x10000, 0x20000 )
	ROM_LOAD( "b70-03.bin", 0x30000, 0x80000, CRC(42d5a9b8) SHA1(10714fe95c372cec12376e615a9abe213aff12bc) )

	ROM_REGION( 0x1c000, REGION_CPU2, 0 )	/* sound (sndhrdw/rastan.c wants it as #2 */
	ROM_LOAD( "b70-09.bin", 0x00000, 0x4000, CRC(85cccaa2) SHA1(5459cd8df9d94e1938008cfc17d4ebac98004bfc) )
	ROM_CONTINUE(           0x10000, 0xc000 )

	ROM_REGION( 0x30000, REGION_CPU3, 0 )
	ROM_LOAD( "b70-08.bin", 0x00000, 0x20000, CRC(4d795f48) SHA1(58040d8ccbd0572cf6aef6ea9dd646b9338a03a0) )
	ROM_RELOAD(             0x10000, 0x20000 )

	ROM_REGION( 0x100000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "b70-01.bin", 0x00000, 0x80000, CRC(fcdf67e2) SHA1(08a6a04a45c4adb4f5b4b0b83e90b2e5fe5cb0b1) )
	ROM_LOAD( "b70-02.bin", 0x80000, 0x80000, CRC(35f7172e) SHA1(f257e9db470bb6dcca491b89cb666ef6d2546887) )
ROM_END

ROM_START( fhawkj )
	ROM_REGION( 0xb0000, REGION_CPU1, 0 )
	ROM_LOAD( "b70-07.bin", 0x00000, 0x20000, CRC(939114af) SHA1(66218536dcb3b34ffa01d3c9c2fee365d91cfe00) )
	ROM_RELOAD(             0x10000, 0x20000 )
	ROM_LOAD( "b70-03.bin", 0x30000, 0x80000, CRC(42d5a9b8) SHA1(10714fe95c372cec12376e615a9abe213aff12bc) )

	ROM_REGION( 0x1c000, REGION_CPU2, 0 )	/* sound (sndhrdw/rastan.c wants it as #2 */
	ROM_LOAD( "b70-09.bin", 0x00000, 0x4000, CRC(85cccaa2) SHA1(5459cd8df9d94e1938008cfc17d4ebac98004bfc) )
	ROM_CONTINUE(           0x10000, 0xc000 )

	ROM_REGION( 0x30000, REGION_CPU3, 0 )
	ROM_LOAD( "b70-08.bin", 0x00000, 0x20000, CRC(4d795f48) SHA1(58040d8ccbd0572cf6aef6ea9dd646b9338a03a0) )
	ROM_RELOAD(             0x10000, 0x20000 )

	ROM_REGION( 0x100000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "b70-01.bin", 0x00000, 0x80000, CRC(fcdf67e2) SHA1(08a6a04a45c4adb4f5b4b0b83e90b2e5fe5cb0b1) )
	ROM_LOAD( "b70-02.bin", 0x80000, 0x80000, CRC(35f7172e) SHA1(f257e9db470bb6dcca491b89cb666ef6d2546887) )
ROM_END

ROM_START( champwr )
	ROM_REGION( 0xf0000, REGION_CPU1, 0 )
	ROM_LOAD( "c01-13.rom", 0x00000, 0x20000, CRC(7ef47525) SHA1(79789fa3bcaeb6666c108d2e4b69a1f9341b2f4a) )
	ROM_RELOAD(             0x10000, 0x20000 )
	ROM_LOAD( "c01-04.rom", 0x30000, 0x20000, CRC(358bd076) SHA1(beb20a09370d05de719dde596eadca8fecb14ce5) )

	ROM_REGION( 0x1c000, REGION_CPU2, 0 )	/* sound (sndhrdw/rastan.c wants it as #2 */
	ROM_LOAD( "c01-08.rom", 0x00000, 0x4000, CRC(810efff8) SHA1(dd4fc046095e0e815e8e1fd96d258da0d6bba298) )
	ROM_CONTINUE(           0x10000, 0xc000 )

	ROM_REGION( 0x30000, REGION_CPU3, 0 )
	ROM_LOAD( "c01-07.rom", 0x00000, 0x20000, CRC(5117c98f) SHA1(16b3a443eb113d2591833884a1b0ff297d8c00a4) )
	ROM_RELOAD(             0x10000, 0x20000 )

	ROM_REGION( 0x180000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "c01-01.rom", 0x000000, 0x80000, CRC(f302e6e9) SHA1(456b046932c1ee29c890b8e87d417c4bb508c06a) )
	ROM_LOAD( "c01-02.rom", 0x080000, 0x80000, CRC(1e0476c4) SHA1(b7922e5196990ad4382f367ec80b5c72e75f9d35) )
	ROM_LOAD( "c01-03.rom", 0x100000, 0x80000, CRC(2a142dbc) SHA1(5d0e40ec266d3abcff4237c5c609355c65b4fa33) )

	ROM_REGION( 0x20000, REGION_SOUND1, 0 )	/* ADPCM samples */
	ROM_LOAD( "c01-05.rom", 0x00000, 0x20000, CRC(22efad4a) SHA1(54fb33dfba5059dee16fa8b5a33b0b2d62a78373) )
ROM_END

ROM_START( champwru )
	ROM_REGION( 0xf0000, REGION_CPU1, 0 )
	ROM_LOAD( "c01-12.rom", 0x00000, 0x20000, CRC(09f345b3) SHA1(f3f9a7dab0b3f87b6919a7b37cb52245e112cb08) )
	ROM_RELOAD(             0x10000, 0x20000 )
	ROM_LOAD( "c01-04.rom", 0x30000, 0x20000, CRC(358bd076) SHA1(beb20a09370d05de719dde596eadca8fecb14ce5) )

	ROM_REGION( 0x1c000, REGION_CPU2, 0 )	/* sound (sndhrdw/rastan.c wants it as #2 */
	ROM_LOAD( "c01-08.rom", 0x00000, 0x4000, CRC(810efff8) SHA1(dd4fc046095e0e815e8e1fd96d258da0d6bba298) )
	ROM_CONTINUE(           0x10000, 0xc000 )

	ROM_REGION( 0x30000, REGION_CPU3, 0 )
	ROM_LOAD( "c01-07.rom", 0x00000, 0x20000, CRC(5117c98f) SHA1(16b3a443eb113d2591833884a1b0ff297d8c00a4) )
	ROM_RELOAD(             0x10000, 0x20000 )

	ROM_REGION( 0x180000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "c01-01.rom", 0x000000, 0x80000, CRC(f302e6e9) SHA1(456b046932c1ee29c890b8e87d417c4bb508c06a) )
	ROM_LOAD( "c01-02.rom", 0x080000, 0x80000, CRC(1e0476c4) SHA1(b7922e5196990ad4382f367ec80b5c72e75f9d35) )
	ROM_LOAD( "c01-03.rom", 0x100000, 0x80000, CRC(2a142dbc) SHA1(5d0e40ec266d3abcff4237c5c609355c65b4fa33) )

	ROM_REGION( 0x20000, REGION_SOUND1, 0 )	/* ADPCM samples */
	ROM_LOAD( "c01-05.rom", 0x00000, 0x20000, CRC(22efad4a) SHA1(54fb33dfba5059dee16fa8b5a33b0b2d62a78373) )
ROM_END

ROM_START( champwrj )
	ROM_REGION( 0xf0000, REGION_CPU1, 0 )
	ROM_LOAD( "c01-06.bin", 0x00000, 0x20000, CRC(90fa1409) SHA1(7904488d567ce5d8705b2d2c8a4b4aae310cc28b) )
	ROM_RELOAD(             0x10000, 0x20000 )
	ROM_LOAD( "c01-04.rom", 0x30000, 0x20000, CRC(358bd076) SHA1(beb20a09370d05de719dde596eadca8fecb14ce5) )

	ROM_REGION( 0x1c000, REGION_CPU2, 0 )	/* sound (sndhrdw/rastan.c wants it as #2 */
	ROM_LOAD( "c01-08.rom", 0x00000, 0x4000, CRC(810efff8) SHA1(dd4fc046095e0e815e8e1fd96d258da0d6bba298) )
	ROM_CONTINUE(           0x10000, 0xc000 )

	ROM_REGION( 0x30000, REGION_CPU3, 0 )
	ROM_LOAD( "c01-07.rom", 0x00000, 0x20000, CRC(5117c98f) SHA1(16b3a443eb113d2591833884a1b0ff297d8c00a4) )
	ROM_RELOAD(             0x10000, 0x20000 )

	ROM_REGION( 0x180000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "c01-01.rom", 0x000000, 0x80000, CRC(f302e6e9) SHA1(456b046932c1ee29c890b8e87d417c4bb508c06a) )
	ROM_LOAD( "c01-02.rom", 0x080000, 0x80000, CRC(1e0476c4) SHA1(b7922e5196990ad4382f367ec80b5c72e75f9d35) )
	ROM_LOAD( "c01-03.rom", 0x100000, 0x80000, CRC(2a142dbc) SHA1(5d0e40ec266d3abcff4237c5c609355c65b4fa33) )

	ROM_REGION( 0x20000, REGION_SOUND1, 0 )	/* ADPCM samples */
	ROM_LOAD( "c01-05.rom", 0x00000, 0x20000, CRC(22efad4a) SHA1(54fb33dfba5059dee16fa8b5a33b0b2d62a78373) )
ROM_END


ROM_START( kurikint )
	ROM_REGION( 0xb0000, REGION_CPU1, 0 )
	ROM_LOAD( "b42-09.2",    0x00000, 0x20000, CRC(e97c4394) SHA1(fdeb15315166f7615d4039d5dc9c28d53cee86f2) )
	ROM_RELOAD(              0x10000, 0x20000 )
	ROM_LOAD( "b42-06.6",    0x30000, 0x20000, CRC(fa15fd65) SHA1(a810d7315878212e4e5344a24addf117ea6baeab) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )
	ROM_LOAD( "b42-07.22",   0x00000, 0x10000, CRC(0f2719c0) SHA1(f870335a75f236f0059522f9a577dee7ca3acb2f) )

	ROM_REGION( 0x100000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "b42-01.1",    0x00000, 0x80000, CRC(7d1a1fec) SHA1(28311b07673686c18988400d0254533a454f07f4) )
	ROM_LOAD( "b42-02.5",    0x80000, 0x80000, CRC(1a52e65c) SHA1(20a1fc4d02b5928fb01444079692e23d178c6297) )
ROM_END

ROM_START( kurikinu )
	ROM_REGION( 0xb0000, REGION_CPU1, 0 )
	ROM_LOAD( "b42-08.2",    0x00000, 0x20000, CRC(7075122e) SHA1(55f5f0cf3b91b7b408f9c05c91f9839c43b49c5f) )
	ROM_RELOAD(              0x10000, 0x20000 )
	ROM_LOAD( "b42-06.6",    0x30000, 0x20000, CRC(fa15fd65) SHA1(a810d7315878212e4e5344a24addf117ea6baeab) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )
	ROM_LOAD( "b42-07.22",   0x00000, 0x10000, CRC(0f2719c0) SHA1(f870335a75f236f0059522f9a577dee7ca3acb2f) )

	ROM_REGION( 0x100000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "b42-01.1",    0x00000, 0x80000, CRC(7d1a1fec) SHA1(28311b07673686c18988400d0254533a454f07f4) )
	ROM_LOAD( "b42-02.5",    0x80000, 0x80000, CRC(1a52e65c) SHA1(20a1fc4d02b5928fb01444079692e23d178c6297) )
ROM_END

ROM_START( kurikinj )
	ROM_REGION( 0xb0000, REGION_CPU1, 0 )
	ROM_LOAD( "b42_05.2",    0x00000, 0x20000, CRC(077222b8) SHA1(953fb3444f6bb0dbe0323a0fd8fc3067b106a4f6) )
	ROM_RELOAD(              0x10000, 0x20000 )
	ROM_LOAD( "b42-06.6",    0x30000, 0x20000, CRC(fa15fd65) SHA1(a810d7315878212e4e5344a24addf117ea6baeab) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )
	ROM_LOAD( "b42-07.22",   0x00000, 0x10000, CRC(0f2719c0) SHA1(f870335a75f236f0059522f9a577dee7ca3acb2f) )

	ROM_REGION( 0x100000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "b42-01.1",    0x00000, 0x80000, CRC(7d1a1fec) SHA1(28311b07673686c18988400d0254533a454f07f4) )
	ROM_LOAD( "b42-02.5",    0x80000, 0x80000, CRC(1a52e65c) SHA1(20a1fc4d02b5928fb01444079692e23d178c6297) )
ROM_END

ROM_START( kurikina )
	ROM_REGION( 0xb0000, REGION_CPU1, 0 )
	ROM_LOAD( "kk_ic2.rom",  0x00000, 0x20000, CRC(908603f2) SHA1(f810f2501458224e9264a984f22547cc8ccc2b0e) )
	ROM_RELOAD(              0x10000, 0x20000 )
	ROM_LOAD( "kk_ic6.rom",  0x30000, 0x20000, CRC(a4a957b1) SHA1(bbdb5b71ab613a8c89f7a0300abd85408951dc7e) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )
	ROM_LOAD( "b42-07.22",   0x00000, 0x10000, CRC(0f2719c0) SHA1(f870335a75f236f0059522f9a577dee7ca3acb2f) )

	ROM_REGION( 0x100000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "kk_1-1l.rom", 0x00000, 0x20000, CRC(df1d4fcd) SHA1(300cad3636ea9648595c3f4bba3ca737f95f7354) )
	ROM_LOAD( "kk_2-2l.rom", 0x20000, 0x20000, CRC(fca7f647) SHA1(0571e8fc2eda9f139e81d6d191368fb99764f797) )
	ROM_LOAD( "kk_5-3l.rom", 0x40000, 0x20000, CRC(d080fde1) SHA1(e5011cdf35bf5d39f4786e6d60d2b35a79560dfa) )
	ROM_LOAD( "kk_7-4l.rom", 0x60000, 0x20000, CRC(f5bf6829) SHA1(4c1b4c6f451ed823730762f67c2e716789cddb10) )
	ROM_LOAD( "kk_3-1h.rom", 0x80000, 0x20000, CRC(71af848e) SHA1(1e4d050c9191a8645f324de84767662ed80165b6) )
	ROM_LOAD( "kk_4-2h.rom", 0xa0000, 0x20000, CRC(cebb5bac) SHA1(6c1e3cdea353bd835b49b95af0bb718e2b46ecfe) )
	ROM_LOAD( "kk_6-3h.rom", 0xc0000, 0x20000, CRC(322e3752) SHA1(7592b5dc7945c96f53aeb5c328c54c0dcba3809a) )
	ROM_LOAD( "kk_8-4h.rom", 0xe0000, 0x20000, CRC(117bde99) SHA1(fe0f56b6c840e35870639c4de129443e14720a7b) )
ROM_END


ROM_START( plotting )
	ROM_REGION( 0x20000, REGION_CPU1, 0 )
	ROM_LOAD( "plot01.bin", 0x00000, 0x10000, CRC(5b30bc25) SHA1(df8839a90da9e5122d75b6faaf97f59499dbd316) )
	ROM_RELOAD(             0x10000, 0x10000 )

	ROM_REGION( 0x20000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "plot07.bin", 0x00000, 0x10000, CRC(6e0bad2a) SHA1(73996688cd058a2f56f61ea60144b9c673919a58) )
	ROM_LOAD( "plot08.bin", 0x10000, 0x10000, CRC(fb5f3ca4) SHA1(0c335acceea50133a6899f9e368cff5f61b55a96) )
ROM_END

ROM_START( puzznic )
	ROM_REGION( 0x30000, REGION_CPU1, 0 )
	ROM_LOAD( "u11.rom",  0x00000, 0x20000, CRC(a4150b6c) SHA1(27719b8993735532cd59f4ed5693ff3143ee2336) )
	ROM_RELOAD(           0x10000, 0x20000 )

	ROM_REGION( 0x0800, REGION_CPU2, 0 )	/* 2k for the microcontroller */
	ROM_LOAD( "mc68705p", 0x0000, 0x0800, NO_DUMP )

	ROM_REGION( 0x40000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "u10.rom",  0x00000, 0x20000, CRC(4264056c) SHA1(d2d8a170ae0f361093a5384935238605a59e5938) )
	ROM_LOAD( "u09.rom",  0x20000, 0x20000, CRC(3c115f8b) SHA1(8d518be01b7c4d6d993d5d9b62aab719a5c8baca) )
ROM_END

ROM_START( horshoes )
	ROM_REGION( 0x30000, REGION_CPU1, 0 )
	ROM_LOAD( "c47.03", 0x00000, 0x20000, CRC(37e15b20) SHA1(85baa0ee553e4c9fed38294ba8912f18f519e62f) )
	ROM_RELOAD(         0x10000, 0x20000 )

	ROM_REGION( 0x80000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "c47.02", 0x00000, 0x10000, CRC(35f96526) SHA1(e7f9b33d82b050aff49f991aa12db436421caa5b) )
	ROM_CONTINUE (      0x20000, 0x10000 )
	ROM_LOAD( "c47.04", 0x40000, 0x10000, CRC(aeac7121) SHA1(cf67688cde14d452da6d9cbd7a81593f4048ce77) )
	ROM_CONTINUE (      0x60000, 0x10000 )
	ROM_LOAD( "c47.01", 0x10000, 0x10000, CRC(031c73d8) SHA1(deef972fbf226701f9a6469ae3934129dc52ce9c) )
	ROM_CONTINUE (      0x30000, 0x10000 )
	ROM_LOAD( "c47.05", 0x50000, 0x10000, CRC(b2a3dafe) SHA1(5ffd3e296272ef3f31432005c827f057aac79497) )
	ROM_CONTINUE (      0x70000, 0x10000 )
ROM_END

ROM_START( palamed )
	ROM_REGION( 0x30000, REGION_CPU1, 0 )
	ROM_LOAD( "c63.02", 0x00000, 0x20000, CRC(55a82bb2) SHA1(f157ad770351d4b8d8f8c061c4e330d6391fc624) )
	ROM_RELOAD(         0x10000, 0x20000 )

	ROM_REGION( 0x40000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "c63.04", 0x00000, 0x20000, CRC(c7bbe460) SHA1(1c1f186d0b0b2e383f82c53ae93b975a75f50f9c) )
	ROM_LOAD( "c63.03", 0x20000, 0x20000, CRC(fcd86e44) SHA1(bdd0750ed6e93cc49f09f4ccb05b0c4a44cb9c23) )
ROM_END

ROM_START( cachat )
	ROM_REGION( 0x30000, REGION_CPU1, 0 )
	ROM_LOAD( "cac6",  0x00000, 0x20000, CRC(8105cf5f) SHA1(e6dd22165436c247db887a04c3e69c9e2505bb33) )
	ROM_RELOAD(        0x10000, 0x20000 )

	ROM_REGION( 0x80000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "cac9",  0x00000, 0x20000, CRC(bc462914) SHA1(3eede8940cabadf563acb63059bfc2d13253b29f) )
	ROM_LOAD( "cac10", 0x20000, 0x20000, CRC(ecc64b31) SHA1(04ce97cdcdbdbd38602011f5ed27fe9182fb500a) )
	ROM_LOAD( "cac7",  0x40000, 0x20000, CRC(7fb71578) SHA1(34cfa1383ea1f3cbf45eaf6b989a1248cdef1bb9) )
	ROM_LOAD( "cac8",  0x60000, 0x20000, CRC(d2a63799) SHA1(71b024b239834ef068b7fc20cd49aae7853e0f7c) )
ROM_END

ROM_START( tubeit )
	ROM_REGION( 0x30000, REGION_CPU1, 0 )
	ROM_LOAD( "t-i_02.6", 0x00000, 0x20000, CRC(54730669) SHA1(a44ebd31a8588a133a7552a39fa8d52ba1985e45) )
	ROM_RELOAD(         0x10000, 0x20000 )

	ROM_REGION( 0x80000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "t-i_03.7", 0x40000, 0x40000, CRC(e1c3fed0) SHA1(cd68dbf61ed820f4aa50c630e7cb778aafb433c2) )
	ROM_LOAD( "t-i_04.9", 0x00000, 0x40000, CRC(b4a6e31d) SHA1(e9abab8f19c78207f25a62104bcae1e391cbd2c0) )
ROM_END

ROM_START( cubybop )
	ROM_REGION( 0x50000, REGION_CPU1, 0 )
	ROM_LOAD( "cb06.6", 0x00000, 0x40000, CRC(66b89a85) SHA1(2ba26d71fd1aa8e64584a5908a1d797666718d49) )
	ROM_RELOAD(         0x10000, 0x40000 )

	ROM_REGION( 0x100000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "cb09.9",  0x00000, 0x40000, CRC(5f831e59) SHA1(db319a6c1058200274d687163b4df2f78a2bf879) )
	ROM_LOAD( "cb10.10", 0x40000, 0x40000, CRC(430510fc) SHA1(95c0a0ebd0485a15090f302e5d2f4da8204baf7c) )
	ROM_LOAD( "cb07.7",  0x80000, 0x40000, CRC(3582de99) SHA1(51620cc9044aef8e5ed0335b7d5d6d67a7857005) )
	ROM_LOAD( "cb08.8",  0xc0000, 0x40000, CRC(09e18a51) SHA1(18db47d1d84f9be892bc796116c7ef7d0c1ee59f) )
ROM_END

ROM_START( plgirls )
	ROM_REGION( 0x50000, REGION_CPU1, 0 )
	ROM_LOAD( "pg03.ic6",    0x00000, 0x40000, CRC(6ca73092) SHA1(f5679f047a29b936046c0d3677489df553ad7b41) )
	ROM_RELOAD(              0x10000, 0x40000 )

	ROM_REGION( 0x80000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "pg02.ic9",    0x00000, 0x40000, CRC(3cf05ca9) SHA1(502c45a5330dda1b2fbf7d3d0c9bc6e889ff07d8) )
	ROM_LOAD( "pg01.ic7",    0x40000, 0x40000, CRC(79e41e74) SHA1(aa8efbeeee47f84e19b639821a89a7bcd67fe7a9) )
ROM_END

ROM_START( plgirls2 )
	ROM_REGION( 0x50000, REGION_CPU1, 0 )
	ROM_LOAD( "pg2_1j.ic6",  0x00000, 0x40000, CRC(f924197a) SHA1(ecaaefd1b3715ba60608e05d58be67e3c71f653a) )
	ROM_RELOAD(              0x10000, 0x40000 )

	ROM_REGION( 0x100000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "cho-l.ic9",   0x00000, 0x80000, CRC(956384ec) SHA1(94a2b95f340e96bdccbeafd373f0dea90b8328dd) )
	ROM_LOAD( "cho-h.ic7",   0x80000, 0x80000, CRC(992f99b1) SHA1(c79f1014d73654740f7823812f92376d65d6b15d) )
ROM_END

ROM_START( evilston )
	ROM_REGION( 0xb0000, REGION_CPU1, 0 )
	ROM_LOAD( "c67-03.ic2",    0x00000, 0x20000, CRC(53419982) SHA1(ecc338e2237d26c5ff25b756d371b26b23beed1e) )
	ROM_RELOAD(              0x10000, 0x20000 )
	ROM_LOAD( "c67-04.ic6",    0x30000, 0x20000, CRC(55d57e19) SHA1(8815bcaafe7ee056314b4131e3fb7963854dd6ba) )

	ROM_REGION( 0x80000, REGION_CPU2, 0 )
	ROM_LOAD( "c67-05.22",   0x00000, 0x20000, CRC(94d3a642) SHA1(af20aa5bb60a45c05eb1deba23ba30e6640ca235) )

	ROM_REGION( 0x100000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "c67-01.ic1",    0x00000, 0x80000, CRC(2f351bf4) SHA1(0fb37abf3413cd11baece1c9bbca5a51b0f28938) )
	ROM_LOAD( "c67-02.ic5",    0x80000, 0x80000, CRC(eb4f895c) SHA1(2c902572fe5a5d4442e4dd29e8a85cb40c384140) )
ROM_END




/* bits 7..0 => bits 0..7*/
static DRIVER_INIT( plotting )
{
	unsigned char tab[256];
	unsigned char *p;
	int i;

	for(i=0;i<256;i++)
	{
		int j, v=0;
		for(j=0;j<8;j++)
			if(i & (1<<j))
				v |= 1<<(7-j);
		tab[i] = v;
	}
	p = memory_region(REGION_CPU1);
	for(i=0;i<0x20000;i++)
	{
		*p = tab[*p];
		p++;
	}
}

static DRIVER_INIT( evilston )
{
	unsigned char *ROM = memory_region(REGION_CPU2);
	ROM[0x72]=0x45;	/* reti -> retn  ('dead' loop @ $1104 )*/
	install_mem_write_handler( 0, 0xa7fe, 0xa7fe, evilston_snd_w);
}


GAME( 1988, raimais,  0,        raimais,  raimais,  0,        ROT0,   "Taito Corporation", "Raimais (Japan)" )
GAME( 1988, fhawk,    0,        fhawk,    fhawk,    0,        ROT270, "Taito Corporation Japan", "Fighting Hawk (World)" )
GAME( 1988, fhawkj,   fhawk,    fhawk,    fhawkj,   0,        ROT270, "Taito Corporation", "Fighting Hawk (Japan)" )
GAME( 1989, champwr,  0,        champwr,  champwr,  0,        ROT0,   "Taito Corporation Japan", "Champion Wrestler (World)" )
GAME( 1989, champwru, champwr,  champwr,  champwru, 0,        ROT0,   "Taito America Corporation", "Champion Wrestler (US)" )
GAME( 1989, champwrj, champwr,  champwr,  champwrj, 0,        ROT0,   "Taito Corporation", "Champion Wrestler (Japan)" )
GAME( 1988, kurikint, 0,        kurikint, kurikint, 0,        ROT0,   "Taito Corporation Japan", "Kuri Kinton (World)" )
GAME( 1988, kurikinu, kurikint, kurikint, kurikinj, 0,        ROT0,   "Taito America Corporation", "Kuri Kinton (US)" )
GAME( 1988, kurikinj, kurikint, kurikint, kurikinj, 0,        ROT0,   "Taito Corporation", "Kuri Kinton (Japan)" )
GAME( 1988, kurikina, kurikint, kurikina, kurikina, 0,        ROT0,   "Taito Corporation Japan", "Kuri Kinton (World, prototype[Q])" )
GAME( 1989, plotting, 0,        plotting, plotting, plotting, ROT0,   "Taito Corporation Japan", "Plotting (World)" )
GAME( 1989, puzznic,  0,        puzznic,  puzznic,  0,        ROT0,   "Taito Corporation", "Puzznic (Japan)" )
GAME( 1990, horshoes, 0,        horshoes, horshoes, 0,        ROT270, "Taito America Corporation", "American Horseshoes (US)" )
GAME( 1990, palamed,  0,        palamed,  palamed,  0,        ROT0,   "Taito Corporation", "Palamedes (Japan)" )
GAME( 1993, cachat,   0,        cachat,   cachat,   0,        ROT0,   "Taito Corporation", "Cachat (Japan)" )
GAME( 1993, tubeit,   cachat,   cachat,   tubeit,   0,        ROT0,   "Taito Corporation", "Tube-It" )  /* No (c) message*/
GAME( 199?, cubybop,  0,        cachat,   cubybop,  0,        ROT0,   "Taito Corporation", "Cuby Bop (Location Test)" ) /* No (c) message*/

GAME( 1992, plgirls,  0,        cachat,   plgirls,  0,        ROT270, "Hot-B.", "Play Girls" )
GAME( 1993, plgirls2, 0,        cachat,   plgirls2, 0,        ROT270, "Hot-B.", "Play Girls 2" )

GAME( 1990, evilston, 0,        evilston, evilston, evilston, ROT270, "Spacy Industrial, Ltd.", "Evil Stone" )
