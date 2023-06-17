/*
	Sega Multi System 32 hardware

 preliminary support by Jason Lo aka fbff

 based on earlier work by R.Belmont and David Haywood which was
 in turn based on the Modeler emulator

 Main ToDo's:

 convert from using the 16-bit V60 to the 32-bit V70 (I'm doing this
  later as I couldn't get it to boot with the V70 for now and the gfx
  hardware is easier to work with this way)


*/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "machine/eeprom.h"
#include "machine/random.h"

#define MASTER_CLOCK		32215900
#define MULTI32_CLOCK		40000000

#define MAX_COLOURS (16384)

int multi32;

static unsigned char irq_status;
static data16_t *system32_shared_ram;
extern data16_t *system32_mixerregs[2];  /* mixer registers*/

static data16_t *sys32_protram;
static data16_t *system32_workram;
extern data16_t sys32_tilebank_external;
extern data16_t sys32_displayenable;

/* Video Hardware */
extern int system32_temp_kludge;
extern data16_t *sys32_spriteram16;
extern data16_t *sys32_txtilemap_ram;
extern data16_t *sys32_ramtile_ram;
extern data16_t *scrambled_paletteram16[2];
static data16_t *paletteram16_b;

extern int system32_mixerShift;
extern int system32_screen_mode;
extern int system32_screen_old_mode;
extern int system32_allow_high_resolution;

extern int sys32_brightness[2][3];

WRITE16_HANDLER( sys32_videoram_w );
WRITE16_HANDLER( sys32_ramtile_w );
WRITE16_HANDLER( sys32_spriteram_w );
READ16_HANDLER ( sys32_videoram_r );
WRITE32_HANDLER( sys32_videoram_long_w );
READ32_HANDLER ( sys32_videoram_long_r );
VIDEO_START( system32 );
VIDEO_UPDATE( system32 );

extern int system32_use_default_eeprom;

static data16_t controlB[256];
static data16_t control[256];

static void irq_raise(int level)
{
	irq_status |= (1 << level);
	cpu_set_irq_line(0, 0, ASSERT_LINE);
}

static int irq_callback(int irqline)
{
	int i;
	for(i=7; i>=0; i--)
		if(irq_status & (1 << i)) {
			return i;
		}
	return 0;
}

static WRITE16_HANDLER(irq_ack_w)
{
	if(ACCESSING_MSB) {
		irq_status &= data >> 8;
		if(!irq_status)
			cpu_set_irq_line(0, 0, CLEAR_LINE);
	}
}

static void irq_init(void)
{
	irq_status = 0;
	cpu_set_irq_line(0, 0, CLEAR_LINE);
	cpu_set_irq_callback(0, irq_callback);
}

static NVRAM_HANDLER( system32 )
{
	if (read_or_write)
		EEPROM_save(file);
	else {
		EEPROM_init(&eeprom_interface_93C46);

		if (file)
			EEPROM_load(file);
	}
}

static READ16_HANDLER(system32_eeprom_r)
{
	return (EEPROM_read_bit() << 7) | input_port_0_r(0);
}

static WRITE16_HANDLER(system32_eeprom_w)
{
	if(ACCESSING_LSB) {
		EEPROM_write_bit(data & 0x80);
		EEPROM_set_cs_line((data & 0x20) ? CLEAR_LINE : ASSERT_LINE);
		EEPROM_set_clock_line((data & 0x40) ? ASSERT_LINE : CLEAR_LINE);
	}
}


static READ16_HANDLER(sys32_read_ff)
{
	return 0xffff;
}

static READ16_HANDLER(sys32_read_random)
{
	return mame_rand(); /* new random.c random number code, see clouds in ga2*/
}

extern int sys32_brightness[2][3];

void multi32_set_colour (int offset, int monitor)
{
	int data;
	int r,g,b;
	int r2,g2,b2;
	UINT16 r_bright, g_bright, b_bright;

	/* Although the hardware writes to all 65536 colours on both monitors, the
	   games do not use more than 16384 colours per monitor.  We discard any
	   colours that are written above MAX_COLOURS(16384).
	*/

	if (offset<MAX_COLOURS) {
		if (monitor) {
			data = paletteram16_b[offset];
		}
		else data = paletteram16[offset];

		r = (data >> 0) & 0x0f;
		g = (data >> 4) & 0x0f;
		b = (data >> 8) & 0x0f;

		r2 = (data >> 13) & 0x1;
		g2 = (data >> 13) & 0x1;
		b2 = (data >> 13) & 0x1;

		r = (r << 4) | (r2 << 3);
		g = (g << 4) | (g2 << 3);
		b = (b << 4) | (b2 << 3);

		/* there might be better ways of doing this ... but for now its functional ;-)*/
		r_bright = sys32_brightness[monitor][0]; r_bright &= 0x3f;
		g_bright = sys32_brightness[monitor][1]; g_bright &= 0x3f;
		b_bright = sys32_brightness[monitor][2]; b_bright &= 0x3f;

		if ((r_bright & 0x20)) { r = (r * (r_bright&0x1f))>>5; } else { r = r+(((0xf8-r) * (r_bright&0x1f))>>5); }
		if ((g_bright & 0x20)) { g = (g * (g_bright&0x1f))>>5; } else { g = g+(((0xf8-g) * (g_bright&0x1f))>>5); }
		if ((b_bright & 0x20)) { b = (b * (b_bright&0x1f))>>5; } else { b = b+(((0xf8-b) * (b_bright&0x1f))>>5); }

		palette_set_color((monitor*MAX_COLOURS)+offset,r,g,b);
	}
}

static WRITE16_HANDLER( multi32_paletteram16_xBBBBBGGGGGRRRRR_scrambled_word_w )
{
	int r,g,b;
	int r2,g2,b2;

	if (offset<MAX_COLOURS) {
		COMBINE_DATA(&scrambled_paletteram16[0][offset]); /* it expects to read back the same values?*/

		/* rearrange the data to normal format ... */

		r = (data >>1) & 0xf;
		g = (data >>6) & 0xf;
		b = (data >>11) & 0xf;

		r2 = (data >>0) & 0x1;
		g2 = (data >>5) & 0x1;
		b2 = (data >> 10) & 0x1;

		data = (data & 0x8000) | r | g<<4 | b << 8 | r2 << 12 | g2 << 13 | b2 << 14;

		COMBINE_DATA(&paletteram16[offset]);

		multi32_set_colour(offset, 0);
	}
}

static WRITE16_HANDLER( multi32_paletteram16_xBGRBBBBGGGGRRRR_word_w )
{
	if (offset<MAX_COLOURS) {
		COMBINE_DATA(&paletteram16[offset]);

	/* some games use 8-bit writes to some palette regions*/
	/* (especially for the text layer palettes)*/

		multi32_set_colour(offset, 0);
	}
}

/* --------------------------------------- Monitor B ---------------------------------*/

static WRITE16_HANDLER( multi32_paletteram16_xBBBBBGGGGGRRRRR_scrambled_word_b_w )
{
	int r,g,b;
	int r2,g2,b2;

	if (offset<MAX_COLOURS) {
		COMBINE_DATA(&scrambled_paletteram16[1][offset]); /* it expects to read back the same values?*/

		/* rearrange the data to normal format ... */

		r = (data >>1) & 0xf;
		g = (data >>6) & 0xf;
		b = (data >>11) & 0xf;

		r2 = (data >>0) & 0x1;
		g2 = (data >>5) & 0x1;
		b2 = (data >> 10) & 0x1;

		data = (data & 0x8000) | r | g<<4 | b << 8 | r2 << 12 | g2 << 13 | b2 << 14;


		COMBINE_DATA(&paletteram16_b[offset]);

		multi32_set_colour(offset, 1);
	}
}

static WRITE16_HANDLER( multi32_paletteram16_xBGRBBBBGGGGRRRR_word_b_w )
{
	if (offset<MAX_COLOURS) {
		COMBINE_DATA(&paletteram16_b[offset]);

		/* some games use 8-bit writes to some palette regions*/
		/* (especially for the text layer palettes)*/

		multi32_set_colour(offset, 1);
	}
}

extern int analogRead[8];
extern int analogSwitch;

static READ16_HANDLER( multi32_io_analog_r )
{
/*
	{ 0xc00050, 0xc00057, system32_io_analog_r },

	 Read the value of each analog control port, one bit at a time, 8 times.
	 Analog Input Set B is requested by the hardware using "analogSwitch"
*/
	int retdata;
	if (offset<=3) {
		retdata = analogRead[offset*2+analogSwitch] & 0x80;
		analogRead[offset*2+analogSwitch] <<= 1;
		return retdata;
	}

	switch(offset)
	{
	default:
		log_cb(RETRO_LOG_DEBUG, LOGPRE "multi32_io_analog [%d:%06x]: read %02x (mask %x)\n", cpu_getactivecpu(), activecpu_get_pc(), offset, mem_mask);
		return 0xffff;
		break;
	}
}

static WRITE16_HANDLER( multi32_io_analog_w )
{
	COMBINE_DATA(&control[offset]);

	if (offset<=3) {
		if (analogSwitch) analogRead[offset*2+1]=readinputport(offset*2+5);
		else analogRead[offset*2]=readinputport(offset*2+4);
	}
}

static READ16_HANDLER( multi32_io_r )
{
/* I/O Control port at 0xc00000

	{ 0xc00000, 0xc00001, input_port_1_word_r },
	{ 0xc00002, 0xc00003, input_port_2_word_r },
	{ 0xc00004, 0xc00007, sys32_read_ff },
	{ 0xc00008, 0xc00009, input_port_3_word_r },
	{ 0xc0000a, 0xc0000b, system32_eeprom_r },
	{ 0xc0000c, 0xc0004f, sys32_read_ff },
*/
	switch(offset) {
	case 0x00:
		return readinputport(0x01);
	case 0x01:
		return readinputport(0x02);
	case 0x02:
		return 0xffff;
	case 0x03:
		/* f1lap*/
		return 0xffff;
	case 0x04:
		return readinputport(0x03);
	case 0x05:
		return (EEPROM_read_bit() << 7) | readinputport(0x00);
	case 0x06:
		return 0xffff;
	case 0x07:
		/* scross*/
		return sys32_tilebank_external;
	case 0x0e:
		/* f1lap*/
		return 0xffff;
	default:
		log_cb(RETRO_LOG_DEBUG, LOGPRE "Port A1 %d [%d:%06x]: read (mask %x)\n", offset, cpu_getactivecpu(), activecpu_get_pc(), mem_mask);
		return 0xffff;
	}
}

static WRITE16_HANDLER( multi32_io_w )
{
/* I/O Control port at 0xc00000

	{ 0xc00006, 0xc00007, system32_eeprom_w },
	{ 0xc0000c, 0xc0000d, jp_v60_write_cab },
	{ 0xc00008, 0xc0000d, MWA16_RAM }, // Unknown c00008=f1lap , c0000c=titlef
	{ 0xc0000e, 0xc0000f, MWA16_RAM, &sys32_tilebank_external }, // tilebank per layer on multi32
	{ 0xc0001c, 0xc0001d, MWA16_RAM, &sys32_displayenable },
	{ 0xc0001e, 0xc0001f, MWA16_RAM }, // Unknown
*/

	COMBINE_DATA(&control[offset]);

	switch(offset) {
	case 0x03:
		if(ACCESSING_LSB) {
			EEPROM_write_bit(data & 0x80);
			EEPROM_set_cs_line((data & 0x20) ? CLEAR_LINE : ASSERT_LINE);
			EEPROM_set_clock_line((data & 0x40) ? ASSERT_LINE : CLEAR_LINE);
		}
		break;
	case 0x04:
		/* f1lap*/
		break;
	case 0x06:
		/* jp_v60_write_cab / titlef*/
		break;
	case 0x07:
		/* Multi32: tilebank per layer*/
		COMBINE_DATA(&sys32_tilebank_external);
		break;
	case 0x0e:
		COMBINE_DATA(&sys32_displayenable);
		break;
	case 0x0f:
		/* orunners unknown*/
		break;
	default:
		log_cb(RETRO_LOG_DEBUG, LOGPRE "Port A1 %d [%d:%06x]: write %02x (mask %x)\n", offset, cpu_getactivecpu(), activecpu_get_pc(), data, mem_mask);
		break;
	}
}

static READ16_HANDLER( multi32_io_2_r )
{
/* I/O Control port at 0xc00060

	{ 0xc00060, 0xc00061, input_port_4_word_r },
	{ 0xc00062, 0xc00063, input_port_5_word_r },
	{ 0xc00064, 0xc00065, input_port_6_word_r },
	{ 0xc00066, 0xc000ff, sys32_read_ff },
*/
	switch(offset) {
	case 0x00:
		return readinputport(0x04);
	case 0x01:
		return readinputport(0x05);
	case 0x02:
		return readinputport(0x06);
	default:
		log_cb(RETRO_LOG_DEBUG, LOGPRE "Port A2 %d [%d:%06x]: read (mask %x)\n", offset, cpu_getactivecpu(), activecpu_get_pc(), mem_mask);
		return 0xffff;
	}
}

static WRITE16_HANDLER( multi32_io_2_w )
{
/* I/O Control port at 0xc00060

	{ 0xc00060, 0xc00061, MWA16_RAM }, // Analog switch
	{ 0xc00074, 0xc00075, MWA16_RAM }, // Unknown
*/

	switch(offset) {
	case 0x00:
		/* Used by the hardware to switch the analog input ports to set B*/
		analogSwitch=data;
		break;
	case 0x0a:
		/* orunners unknown*/
		break;
	default:
		log_cb(RETRO_LOG_DEBUG, LOGPRE "Port A2 %d [%d:%06x]: write %02x (mask %x)\n", offset, cpu_getactivecpu(), activecpu_get_pc(), data, mem_mask);
		break;
	}
}

static READ16_HANDLER( multi32_io_B_r )
{
	switch(offset) {
	case 0:
		/* orunners (mask ff00)*/
		return readinputport(0X0c); /* orunners Monitor B Shift Up, Shift Down buttons*/
	case 1:
		/* orunners (mask ff00)*/
		return readinputport(0X0d); /* orunners Monitor B DJ Music, Music Up, Music Down buttons*/
	case 2:
		return 0x00;
	case 3:
		/* orunners (mask ff00)*/
		return 0x00;
	case 4:
		/* harddunk (mask ff00) will not exit test mode if not 0xff*/
		return readinputport(0X0e); /* orunners Monitor B Service, Test, Coin and Start buttons*/
	case 5:
		/* orunners (mask ff00) locks up*/
		return (EEPROM_read_bit() << 7) | readinputport(0x00);
	case 7:
		/* orunners (mask ff00)*/
		return 0xffff;
	case 14:
		/* harddunk (mask ff00)*/
		return 0xffff;
	default:
		log_cb(RETRO_LOG_DEBUG, LOGPRE "Port B %d [%d:%06x]: read (mask %x)\n", offset, cpu_getactivecpu(), activecpu_get_pc(), mem_mask);
		return 0xffff;
	}
}

static WRITE16_HANDLER( multi32_io_B_w )
{
	COMBINE_DATA(&controlB[offset]);
	switch(offset) {

	case 0x03:
		/* titlef value=00*/
		break;
	case 0x06:
		/* orunners value=00, 08, 34*/
		break;
	case 0x07:
		if(ACCESSING_LSB) {
			EEPROM_write_bit(data & 0x80);
			EEPROM_set_cs_line((data & 0x20) ? CLEAR_LINE : ASSERT_LINE);
			EEPROM_set_clock_line((data & 0x40) ? ASSERT_LINE : CLEAR_LINE);
		}
		break;
	case 0x0e:
		/* orunners value=86 (displayenable?)*/
		break;
	case 0x0f:
		/* orunners value=c8*/
		break;

	default:
		log_cb(RETRO_LOG_DEBUG, LOGPRE "Port B %d [%d:%06x]: write %02x (mask %x)\n", offset, cpu_getactivecpu(), activecpu_get_pc(), data, mem_mask);
		break;
	}
}

static WRITE16_HANDLER( random_number_16_w )
{
/* printf("%06X:random_seed_w(%04X) = %04X & %04X\n", activecpu_get_pc(), offset*2, data, mem_mask  ^ 0xffff);*/
}

static READ16_HANDLER( random_number_16_r )
{
       return rand();
}

static MEMORY_READ16_START( multi32_readmem )
	{ 0x000000, 0x1fffff, MRA16_ROM },
	{ 0x200000, 0x23ffff, MRA16_RAM }, /* work RAM*/
	{ 0x300000, 0x31ffff, sys32_videoram_r }, /* Tile Ram*/
	{ 0x400000, 0x41ffff, MRA16_RAM }, /* sprite RAM*/
	{ 0x500000, 0x50000d, MRA16_RAM },	/* Unknown*/
/*	{ 0x500002, 0x500003, jp_v60_read_cab },*/

	{ 0x600000, 0x6100ff, MRA16_RAM }, /* Palette + mixer registers (Monitor A)*/
	{ 0x680000, 0x69004f, MRA16_RAM }, /* Palette + mixer registers (Monitor B)*/

	{ 0x700000, 0x701fff, MRA16_RAM },	/* shared RAM*/
	{ 0x800000, 0x80000f, MRA16_RAM },	/* Unknown*/
	{ 0x80007e, 0x80007f, MRA16_RAM },	/* Unknown f1lap*/
	{ 0x801000, 0x801003, MRA16_RAM },	/* Unknown*/
	{ 0xa00000, 0xa00001, MRA16_RAM }, /* Unknown dbzvrvs*/

	{ 0xc00000, 0xc0003f, multi32_io_r },
	{ 0xc00050, 0xc0005f, multi32_io_analog_r },
	{ 0xc00060, 0xc0007f, multi32_io_2_r },
	{ 0xc80000, 0xc8007f, multi32_io_B_r },

	{ 0xd80000, 0xdfffff, random_number_16_r },
	{ 0xe00000, 0xe0000f, MRA16_RAM },   /* Unknown*/
	{ 0xe80000, 0xe80003, MRA16_RAM }, /* Unknown*/
	{ 0xf00000, 0xffffff, MRA16_BANK1 }, /* High rom mirror*/
MEMORY_END

static MEMORY_WRITE16_START( multi32_writemem )
	{ 0x000000, 0x1fffff, MWA16_ROM },
	{ 0x200000, 0x23ffff, MWA16_RAM, &system32_workram },
	{ 0x300000, 0x31ffff, sys32_videoram_w },
	{ 0x400000, 0x41ffff, sys32_spriteram_w, &sys32_spriteram16 }, /* Sprites*/
	{ 0x500000, 0x50000d, MWA16_RAM },	/* Unknown*/

	{ 0x600000, 0x607fff, multi32_paletteram16_xBBBBBGGGGGRRRRR_scrambled_word_w, &scrambled_paletteram16[0] },	/* magic data-line-scrambled mirror of palette RAM * we need to shuffle data written then?*/
	{ 0x608000, 0x60ffff, multi32_paletteram16_xBGRBBBBGGGGRRRR_word_w, &paletteram16 }, /* Palettes*/
	{ 0x610000, 0x6100ff, MWA16_RAM, &system32_mixerregs[0] }, /* mixer chip registers*/

	{ 0x680000, 0x687fff, multi32_paletteram16_xBBBBBGGGGGRRRRR_scrambled_word_b_w, &scrambled_paletteram16[1] },	/* magic data-line-scrambled mirror of palette RAM * we need to shuffle data written then?*/
	{ 0x688000, 0x68ffff, multi32_paletteram16_xBGRBBBBGGGGRRRR_word_b_w, &paletteram16_b }, /* Monitor B palette*/
	{ 0x690000, 0x69004f, MWA16_RAM, &system32_mixerregs[1] }, /* monitor B mixer registers*/

	{ 0x700000, 0x701fff, MWA16_RAM, &system32_shared_ram }, /* Shared ram with the z80*/
	{ 0x800000, 0x80000f, MWA16_RAM },	/* Unknown*/
	{ 0x80007e, 0x80007f, MWA16_RAM },	/* Unknown f1lap*/
	{ 0x801000, 0x801003, MWA16_RAM },	/* Unknown*/
	{ 0x81002a, 0x81002b, MWA16_RAM },	/* Unknown dbzvrvs*/
	{ 0x810100, 0x810101, MWA16_RAM },	/* Unknown dbzvrvs*/
	{ 0xa00000, 0xa00fff, MWA16_RAM, &sys32_protram },	/* protection RAM*/

	{ 0xc00000, 0xc0003f, multi32_io_w },
	{ 0xc00050, 0xc0005f, multi32_io_analog_w },
	{ 0xc00060, 0xc0007f, multi32_io_2_w },
	{ 0xc80000, 0xc8007f, multi32_io_B_w },

	{ 0xd00000, 0xd00005, MWA16_RAM }, /* Unknown*/
	{ 0xd00006, 0xd00007, irq_ack_w },
	{ 0xd00008, 0xd0000b, MWA16_RAM }, /* Unknown*/
	{ 0xd80000, 0xdfffff, random_number_16_w },
	{ 0xe00000, 0xe0000f, MWA16_RAM },   /* Unknown*/
	{ 0xe80000, 0xe80003, MWA16_RAM }, /* Unknown*/
	{ 0xf00000, 0xffffff, MWA16_ROM },
MEMORY_END


static MACHINE_INIT( system32 )
{
	cpu_setbank(1, memory_region(REGION_CPU1));
	irq_init();

	/* force it to select lo-resolution on reset */
	system32_allow_high_resolution = 0;
	system32_screen_mode = 0;
	system32_screen_old_mode = 1;
}

static INTERRUPT_GEN( system32_interrupt )
{
	if(cpu_getiloops())
		irq_raise(1);
	else
		irq_raise(0);
}

static void irq_handler(int irq)
{
	cpu_set_irq_line( 1, 0 , irq ? ASSERT_LINE : CLEAR_LINE );
}

static struct GfxLayout s32_bgcharlayout =
{
	16,16,
	RGN_FRAC(1,1),
	4,
	{ 0,1,2,3 },
	{ 0, 4, 16, 20, 8, 12, 24, 28,
	   32, 36, 48, 52, 40, 44, 56, 60  },
	{ 0*64, 1*64, 2*64, 3*64, 4*64, 5*64, 6*64, 7*64,
	  8*64, 9*64,10*64,11*64,12*64,13*64,14*64,15*64 },
	16*64
};

static struct GfxLayout s32_fgcharlayout =
{
	8,8,
	RGN_FRAC(1,1),
	4,
	{ 0,1,2,3 },
	{ 0, 4, 8, 12, 16, 20, 24, 28 },
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32 },
	16*16
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &s32_bgcharlayout,   0x00, 0x3ff  },
	{ REGION_GFX3, 0, &s32_fgcharlayout,   0x00, 0x3ff  },
	{ -1 } /* end of array */
};

static UINT8 *sys32_SoundMemBank;

static READ_HANDLER( system32_bank_r )
{
	return sys32_SoundMemBank[offset];
}

static READ_HANDLER( sys32_shared_snd_r )
{
	data8_t *RAM = (data8_t *)system32_shared_ram;

	return RAM[offset];
}

static WRITE_HANDLER( sys32_shared_snd_w )
{
	data8_t *RAM = (data8_t *)system32_shared_ram;

	RAM[offset] = data;
}

static MEMORY_READ_START( multi32_sound_readmem )
	{ 0x0000, 0x9fff, MRA_ROM },
	{ 0xa000, 0xbfff, system32_bank_r },
	{ 0xc000, 0xdfff, MultiPCM_reg_0_r },
	{ 0xe000, 0xffff, sys32_shared_snd_r },
MEMORY_END

static MEMORY_WRITE_START( multi32_sound_writemem )
	{ 0x0000, 0x9fff, MWA_ROM },
	{ 0xc000, 0xdfff, MultiPCM_reg_0_w },
	{ 0xe000, 0xffff, sys32_shared_snd_w },
MEMORY_END

static WRITE_HANDLER( sys32_soundbank_w )
{
	unsigned char *RAM = memory_region(REGION_CPU2);
	int Bank;

	Bank = data * 0x2000;

	sys32_SoundMemBank = &RAM[Bank+0x10000];
}

static PORT_READ_START( multi32_sound_readport )
	{ 0x80, 0x80, YM2612_status_port_0_A_r },
PORT_END

static PORT_WRITE_START( multi32_sound_writeport )
	{ 0x80, 0x80, YM2612_control_port_0_A_w },
	{ 0x81, 0x81, YM2612_data_port_0_A_w },
	{ 0x82, 0x82, YM2612_control_port_0_B_w },
	{ 0x83, 0x83, YM2612_data_port_0_B_w },
	{ 0xa0, 0xa0, sys32_soundbank_w },
	{ 0xb0, 0xb0, MultiPCM_bank_0_w },
	{ 0xc1, 0xc1, IOWP_NOP },
PORT_END

struct YM2612interface mul32_ym3438_interface =
{
	1,
	MASTER_CLOCK/4,
	{ 60,60 },
	{ 0 },	{ 0 },	{ 0 },	{ 0 },
	{ irq_handler }
};

static struct MultiPCM_interface mul32_multipcm_interface =
{
	1,		/* 1 chip*/
	{ MASTER_CLOCK/4 },	/* clock*/
	{ MULTIPCM_MODE_MULTI32 },	/* banking mode*/
	{ (512*1024) },	/* bank size*/
	{ REGION_SOUND1 },	/* sample region*/
	{ YM3012_VOL(100, MIXER_PAN_CENTER, 100, MIXER_PAN_CENTER) }
};

static struct MultiPCM_interface scross_multipcm_interface =
{
	1,		/* 1 chip*/
	{ MASTER_CLOCK/4 },	/* clock*/
	{ MULTIPCM_MODE_STADCROSS },	/* banking mode*/
	{ (512*1024) },	/* bank size*/
	{ REGION_SOUND1 },	/* sample region*/
	{ YM3012_VOL(100, MIXER_PAN_CENTER, 100, MIXER_PAN_CENTER) }
};

static MACHINE_DRIVER_START( base )

	/* basic machine hardware */
	MDRV_CPU_ADD(V60, MULTI32_CLOCK/2)
	MDRV_CPU_MEMORY(multi32_readmem,multi32_writemem)
	MDRV_CPU_VBLANK_INT(system32_interrupt,2)

	MDRV_CPU_ADD(Z80, MASTER_CLOCK/4)
	MDRV_CPU_MEMORY(multi32_sound_readmem, multi32_sound_writemem)
	MDRV_CPU_PORTS(multi32_sound_readport, multi32_sound_writeport)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(100 /*DEFAULT_60HZ_VBLANK_DURATION*/)

	MDRV_MACHINE_INIT(system32)
	MDRV_NVRAM_HANDLER(system32)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_NEEDS_6BITS_PER_GUN | VIDEO_UPDATE_AFTER_VBLANK | VIDEO_RGB_DIRECT | VIDEO_HAS_SHADOWS ) /* RGB_DIRECT will be needed for alpha*/
	MDRV_SCREEN_SIZE(52*8*2, 28*8*2)
	MDRV_VISIBLE_AREA(0*8, 52*8*2-1, 0*8, 28*8*2-1)

	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(32768)

	MDRV_VIDEO_START(system32)
	MDRV_VIDEO_UPDATE(system32)

	MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
	MDRV_SOUND_ADD(YM3438, mul32_ym3438_interface)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( multi32 )
	MDRV_IMPORT_FROM(base)
	MDRV_SOUND_ADD(MULTIPCM, mul32_multipcm_interface)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( scross )
	MDRV_IMPORT_FROM(base)
	MDRV_SOUND_ADD(MULTIPCM, scross_multipcm_interface)
MACHINE_DRIVER_END

static DRIVER_INIT(orunners)
{
	multi32=1;
	system32_temp_kludge = 0;
	system32_mixerShift = 4;
}

static DRIVER_INIT(titlef)
{
	multi32=1;
	system32_temp_kludge = 0;
	system32_mixerShift = 4;
}

static DRIVER_INIT(harddunk)
{
	multi32=1;
	system32_temp_kludge = 0;
	system32_mixerShift = 5;
}

#define SYSTEM32_PLAYER_INPUTS(_n_, _b1_, _b2_, _b3_, _b4_) \
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_##_b1_         | IPF_PLAYER##_n_ ) \
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_##_b2_         | IPF_PLAYER##_n_ ) \
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_##_b3_         | IPF_PLAYER##_n_ ) \
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_##_b4_         | IPF_PLAYER##_n_ ) \
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER##_n_ ) \
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER##_n_ ) \
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER##_n_ ) \
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER##_n_ )


INPUT_PORTS_START( orunners )
	PORT_START	/* port 0*/
	PORT_BIT( 0x7f, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_SPECIAL )	/* EEPROM data */

	PORT_START	/* port 1*/
	PORT_BITX( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1, "P1 Shift Up", IP_KEY_DEFAULT, IP_JOY_DEFAULT )
	PORT_BITX( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2, "P1 Shift Down", IP_KEY_DEFAULT, IP_JOY_DEFAULT )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* port 2*/
	PORT_BITX( 0x01, IP_ACTIVE_LOW, IPT_BUTTON3, "P1 DJ Music", IP_KEY_DEFAULT, IP_JOY_DEFAULT )
	PORT_BITX( 0x02, IP_ACTIVE_LOW, IPT_BUTTON4, "P1 Music Up", IP_KEY_DEFAULT, IP_JOY_DEFAULT )
	PORT_BITX( 0x04, IP_ACTIVE_LOW, IPT_BUTTON5, "P1 Music Down", IP_KEY_DEFAULT, IP_JOY_DEFAULT )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* port 3*/
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_SERVICE, "Test", KEYCODE_F2, IP_JOY_NONE )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* port 4*/
	PORT_ANALOG( 0xff, 0x80, IPT_AD_STICK_X | IPF_CENTER | IPF_PLAYER1, 30, 10, 0x00, 0xff)

	PORT_START	/* port 5*/
	PORT_BIT( 0x00, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* port 6*/
	PORT_ANALOG( 0xff, 0x00, IPT_PEDAL | IPF_PLAYER1, 30, 10, 0x00, 0xff)

	PORT_START	/* port 7*/
	PORT_BIT( 0x00, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* port 8*/
	PORT_ANALOG( 0xff, 0x00, IPT_PEDAL2 | IPF_PLAYER1, 30, 10, 0x00, 0xff)

	PORT_START	/* port 9*/
	PORT_ANALOG( 0xff, 0x00, IPT_PEDAL | IPF_PLAYER2, 30, 10, 0x00, 0xff)

	PORT_START	/* port A*/
	PORT_ANALOG( 0xff, 0x80, IPT_AD_STICK_X | IPF_CENTER | IPF_PLAYER2, 30, 10, 0x00, 0xff)

	PORT_START	/* port B*/
	PORT_ANALOG( 0xff, 0x00, IPT_PEDAL2 | IPF_PLAYER2, 30, 10, 0x00, 0xff)

	PORT_START	/* port C*/
	PORT_BITX( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2, "P2 Shift Up", IP_KEY_DEFAULT, IP_JOY_DEFAULT )
	PORT_BITX( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2, "P2 Shift Down", IP_KEY_DEFAULT, IP_JOY_DEFAULT )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* port D*/
	PORT_BITX( 0x01, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2, "P2 DJ Music", IP_KEY_DEFAULT, IP_JOY_DEFAULT )
	PORT_BITX( 0x02, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER2, "P2 Music Up", IP_KEY_DEFAULT, IP_JOY_DEFAULT )
	PORT_BITX( 0x04, IP_ACTIVE_LOW, IPT_BUTTON5 | IPF_PLAYER2, "P2 Music Down", IP_KEY_DEFAULT, IP_JOY_DEFAULT )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* port E*/
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_SERVICE2 )
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_SERVICE, "Test", KEYCODE_F2, IP_JOY_NONE )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* port F*/
	PORT_DIPNAME( 0x03, 0x01, "Monitors" )
	PORT_DIPSETTING(    0x01, "A only" )
	PORT_DIPSETTING(    0x03, "A and B" )
	PORT_DIPSETTING(    0x02, "B only" )
INPUT_PORTS_END

INPUT_PORTS_START( titlef )
	PORT_START	/* 0xc0000a - port 0*/
	PORT_BIT( 0x7f, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_SPECIAL )	/* EEPROM data */

	PORT_START	/* 0xc00000 - port 1*/
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN        | IPF_PLAYER1)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN        | IPF_PLAYER1)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN        | IPF_PLAYER1)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN        | IPF_PLAYER1)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_DOWN  | IPF_PLAYER1)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_UP    | IPF_PLAYER1)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_RIGHT | IPF_PLAYER1)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_LEFT  | IPF_PLAYER1)

	PORT_START	/* 0xc00002 - port 2*/
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN        | IPF_PLAYER1)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN        | IPF_PLAYER1)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN        | IPF_PLAYER1)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN        | IPF_PLAYER1)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICKRIGHT_DOWN  | IPF_PLAYER1)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICKRIGHT_UP    | IPF_PLAYER1)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICKRIGHT_RIGHT | IPF_PLAYER1)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICKRIGHT_LEFT  | IPF_PLAYER1)

	PORT_START	/* 0xc00008 - port 3*/
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_SERVICE, "Test", KEYCODE_F2, IP_JOY_NONE )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* 0xc00060 - port 4*/
	PORT_START	/* 0xc00062 - port 5*/
	PORT_START	/* 0xc00064 - port 6*/
	PORT_START	/* port 7*/
	PORT_START	/* port 8*/
	PORT_START	/* port 9*/
	PORT_START	/* port A*/
	PORT_START	/* port B*/

	PORT_START	/* port C*/
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN        | IPF_PLAYER2)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN        | IPF_PLAYER2)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN        | IPF_PLAYER2)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN        | IPF_PLAYER2)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_DOWN  | IPF_PLAYER2)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_UP    | IPF_PLAYER2)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_RIGHT | IPF_PLAYER2)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_LEFT  | IPF_PLAYER2)

	PORT_START	/* port D*/
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN        | IPF_PLAYER2)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN        | IPF_PLAYER2)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN        | IPF_PLAYER2)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN        | IPF_PLAYER2)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICKRIGHT_DOWN  | IPF_PLAYER2)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICKRIGHT_UP    | IPF_PLAYER2)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICKRIGHT_RIGHT | IPF_PLAYER2)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICKRIGHT_LEFT  | IPF_PLAYER2)

	PORT_START	/* port E*/
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_SERVICE, "Test1", KEYCODE_F2, IP_JOY_NONE )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START3 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START4 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* port F*/
	PORT_DIPNAME( 0x03, 0x01, "Monitors" )
	PORT_DIPSETTING(    0x01, "A only" )
	PORT_DIPSETTING(    0x03, "A and B" )
	PORT_DIPSETTING(    0x02, "B only" )
INPUT_PORTS_END

INPUT_PORTS_START( harddunk )
	PORT_START	/* 0xc0000a - port 0*/
	PORT_BIT( 0x7f, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_SPECIAL )	/* EEPROM data */

	PORT_START	/* 0xc00000 - port 1*/
	SYSTEM32_PLAYER_INPUTS(1, BUTTON1, BUTTON2, BUTTON3, BUTTON4)

	PORT_START	/* 0xc00002 - port 2*/
	SYSTEM32_PLAYER_INPUTS(2, BUTTON1, BUTTON2, BUTTON3, BUTTON4)

	PORT_START	/* 0xc00008 - port 3*/
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_SERVICE2 )
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_SERVICE, "Test", KEYCODE_F2, IP_JOY_NONE )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* port 4*/
	SYSTEM32_PLAYER_INPUTS(3, BUTTON1, BUTTON2, BUTTON3, BUTTON4)

	PORT_START	/* port 5*/
	SYSTEM32_PLAYER_INPUTS(6, BUTTON1, BUTTON2, BUTTON3, BUTTON4)

	PORT_START	/* port 6*/
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START3 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START6 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* port 7*/
	PORT_START	/* port 8*/
	PORT_START	/* port 9*/
	PORT_START	/* port A*/
	PORT_START	/* port B*/

	PORT_START	/* port C*/
	SYSTEM32_PLAYER_INPUTS(4, BUTTON1, BUTTON2, BUTTON3, BUTTON4)

	PORT_START	/* port D*/
	SYSTEM32_PLAYER_INPUTS(5, BUTTON1, BUTTON2, BUTTON3, BUTTON4)

	PORT_START	/* port E*/
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_SERVICE, "Test1", KEYCODE_F2, IP_JOY_NONE )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START4 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START5 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* port F*/
	PORT_DIPNAME( 0x03, 0x01, "Monitors" )
	PORT_DIPSETTING(    0x01, "A only" )
	PORT_DIPSETTING(    0x03, "A and B" )
	PORT_DIPSETTING(    0x02, "B only" )
INPUT_PORTS_END

INPUT_PORTS_START( scross )
	PORT_START	/* 0xc0000a - port 0*/
	PORT_BIT( 0x7f, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_SPECIAL )	/* EEPROM data */

	PORT_START	/* 0xc00000 - port 1*/
	PORT_BITX( 0x01, IP_ACTIVE_LOW, IPT_BUTTON2, "P1 Attack", IP_KEY_DEFAULT, IP_JOY_DEFAULT )
	PORT_BITX( 0x02, IP_ACTIVE_HIGH, IPT_BUTTON3, "P1 Wheelie", IP_KEY_DEFAULT, IP_JOY_DEFAULT )
	PORT_BITX( 0x04, IP_ACTIVE_LOW, IPT_BUTTON4, "P1 Brake", IP_KEY_DEFAULT, IP_JOY_DEFAULT )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* 0xc00002 - port 2*/

	PORT_START	/* 0xc00008 - port 3*/
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_SERVICE, "Test", KEYCODE_F2, IP_JOY_NONE )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* port 4*/
	PORT_ANALOG( 0xff, 0x7f, IPT_AD_STICK_X | IPF_CENTER | IPF_REVERSE | IPF_PLAYER1, 30, 10, 0x00, 0xff)

	PORT_START	/* port 5*/

	PORT_START	/* port 6*/
	PORT_ANALOG( 0xff, 0x00, IPT_PEDAL | IPF_PLAYER1, 30, 10, 0x00, 0xff)

	PORT_START	/* port 7*/
	PORT_START	/* port 8*/
	PORT_ANALOG( 0xff, 0x7f, IPT_AD_STICK_X | IPF_CENTER | IPF_REVERSE | IPF_PLAYER2, 30, 10, 0x00, 0xff)

	PORT_START	/* port 9*/

	PORT_START	/* port A*/
	PORT_ANALOG( 0xff, 0x00, IPT_PEDAL | IPF_PLAYER2, 30, 10, 0x00, 0xff)

	PORT_START	/* port B*/

	PORT_START	/* port C*/
	PORT_BITX( 0x01, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2, "P2 Attack", IP_KEY_DEFAULT, IP_JOY_DEFAULT )
	PORT_BITX( 0x02, IP_ACTIVE_HIGH, IPT_BUTTON3 | IPF_PLAYER2, "P2 Wheelie", IP_KEY_DEFAULT, IP_JOY_DEFAULT )
	PORT_BITX( 0x04, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER2, "P2 Brake", IP_KEY_DEFAULT, IP_JOY_DEFAULT )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* port D*/

	PORT_START	/* port E*/
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_SERVICE2 )
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_SERVICE, "Test", KEYCODE_F2, IP_JOY_NONE )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* port F*/
	PORT_DIPNAME( 0x03, 0x01, "Monitors" )
	PORT_DIPSETTING(    0x01, "A only" )
	PORT_DIPSETTING(    0x03, "A and B" )
	PORT_DIPSETTING(    0x02, "B only" )
INPUT_PORTS_END



ROM_START( orunners )
	ROM_REGION( 0x200000, REGION_CPU1, 0 ) /* v60 code */
	ROM_LOAD32_WORD( "epr15618.bin", 0x000000, 0x020000, CRC(25647f76) SHA1(9f882921ebb2f078350295c322b263f75812c053) )
	ROM_RELOAD(                      0x040000, 0x020000 )
	ROM_RELOAD(                      0x080000, 0x020000 )
	ROM_RELOAD(                      0x0c0000, 0x020000 )
	ROM_LOAD32_WORD( "epr15619.bin", 0x000002, 0x020000, CRC(2a558f95) SHA1(616ec0a7b251da61a49b933c58895b1a4d39417a) )
	ROM_RELOAD(                      0x040002, 0x020000 )
	ROM_RELOAD(                      0x080002, 0x020000 )
	ROM_RELOAD(                      0x0c0002, 0x020000 )

	/* v60 data */
	ROM_LOAD32_WORD( "mpr15538.bin", 0x100000, 0x080000, CRC(93958820) SHA1(e19b6f18a5707dbb64ae009d63c05eac5bac4a81) )
	ROM_LOAD32_WORD( "mpr15539.bin", 0x100002, 0x080000, CRC(219760fa) SHA1(bd62a83de9c9542f6da454a87dc4947492f65c52) )

	ROM_REGION( 0x90000, REGION_CPU2, 0 ) /* sound CPU */
	ROM_LOAD("epr15550.bin", 0x00000, 0x80000, CRC(0205d2ed) SHA1(3475479e1a45fe96eefbe53842758898db7accbf) )
	ROM_RELOAD(              0x10000, 0x80000             )

	ROM_REGION( 0x400000, REGION_GFX1, ROMREGION_DISPOSE ) /* tiles */
	ROM_LOAD16_BYTE( "mpr15548.bin", 0x000000, 0x200000, CRC(b6470a66) SHA1(e1544590c02d41f62f82a4d771b893fb0f2734c7) )
	ROM_LOAD16_BYTE( "mpr15549.bin", 0x000001, 0x200000, CRC(81d12520) SHA1(1555893941e832f00ad3d0b3ad0c34a0d3a1c58a) )

	ROM_REGION( 0x1000000, REGION_GFX2, 0 ) /* sprites */
	ROMX_LOAD( "mpr15540.bin", 0x000000, 0x200000, CRC(a10d72b4) SHA1(6d9d5e20be6721b53ce49df4d5a1bbd91f5b3aed) , ROM_SKIP(6)|ROM_GROUPWORD )
	ROMX_LOAD( "mpr15542.bin", 0x000002, 0x200000, CRC(40952374) SHA1(c669ef52508bc2f49cf812dc86ac98fb535471fa) , ROM_SKIP(6)|ROM_GROUPWORD )
	ROMX_LOAD( "mpr15544.bin", 0x000004, 0x200000, CRC(39e3df45) SHA1(38a7b21617b45613b05509dda388f8f7770b186c) , ROM_SKIP(6)|ROM_GROUPWORD )
	ROMX_LOAD( "mpr15546.bin", 0x000006, 0x200000, CRC(e3fcc12c) SHA1(1cf7e05c7873f68789a27a91cddf471df40d7907) , ROM_SKIP(6)|ROM_GROUPWORD )
	ROMX_LOAD( "mpr15541.bin", 0x800000, 0x200000, CRC(a2003c2d) SHA1(200a2c7d78d3f5f28909267fdcdbddd58c5f5fa2) , ROM_SKIP(6)|ROM_GROUPWORD )
	ROMX_LOAD( "mpr15543.bin", 0x800002, 0x200000, CRC(933e8e7b) SHA1(0d53286f524f47851a483569dc37e9f6d34cc5f4) , ROM_SKIP(6)|ROM_GROUPWORD )
	ROMX_LOAD( "mpr15545.bin", 0x800004, 0x200000, CRC(53dd0235) SHA1(4aee5ae1820ff933b6bd8a54bdbf989c0bc95c1a) , ROM_SKIP(6)|ROM_GROUPWORD )
	ROMX_LOAD( "mpr15547.bin", 0x800006, 0x200000, CRC(edcb2a43) SHA1(f0bcfcc749ca0267f85bf9838164869912944d00) , ROM_SKIP(6)|ROM_GROUPWORD )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) /* Sega PCM sound data */
	ROM_LOAD("mpr15551.bin", 0x000000, 0x200000, CRC(4894bc73) SHA1(351f5c03fb430fd87df915dfe3a377b5ada622c4) )
	ROM_LOAD("mpr15552.bin", 0x200000, 0x200000, CRC(1c4b5e73) SHA1(50a8e9a200575a3522a51bf094aa0e87b90bb0a3) )

	ROM_REGION( 0x20000, REGION_GFX3, 0 ) /* FG tiles */
ROM_END

ROM_START( harddunk )
	ROM_REGION( 0x200000, REGION_CPU1, 0 ) /* v60 code */
	ROM_LOAD32_WORD( "ep16512.37", 0x000000, 0x40000, CRC(1a7de085) SHA1(2e0dac1f7715089b7f6b1035c859ffe2d674932f) )
	ROM_RELOAD(                      0x080000, 0x040000 )
	/* the following is the same as 16509.40 but with a different name, unusual for Sega */
	ROM_LOAD32_WORD( "ep16513.40", 0x000002, 0x40000, CRC(603dee75) SHA1(32ae964a4b57d470b4900cca6e06329f1a75a6e6) )
	ROM_RELOAD(                      0x080002, 0x040000 )

	ROM_REGION( 0x30000, REGION_CPU2, 0 ) /* sound CPU */
	ROM_LOAD("16505",        0x00000, 0x20000, CRC(eeb90a07) SHA1(d1c2132897994b2e85fd5a97222b9fcd61bc421e) )
	ROM_RELOAD(              0x10000, 0x20000             )

	ROM_REGION( 0x100000, REGION_GFX1, ROMREGION_DISPOSE ) /* tiles */
	ROM_LOAD16_BYTE( "16503", 0x000000, 0x080000, CRC(ac1b6f1a) SHA1(56482931adf7fe551acf796b74cd8af3773d4fef) )
	ROM_LOAD16_BYTE( "16504", 0x000001, 0x080000, CRC(7c61fcd8) SHA1(ca4354f90fada752bf11ee22a7798a8aa22b1c61) )

	ROM_REGION( 0x1000000, REGION_GFX2, 0 ) /* sprites */
	ROMX_LOAD( "16495", 0x000000, 0x200000, CRC(6e5f26be) SHA1(146761072bbed08f4a9df8a474b34fab61afaa4f) , ROM_SKIP(6)|ROM_GROUPWORD )
	ROMX_LOAD( "16497", 0x000002, 0x200000, CRC(42ab5859) SHA1(f50c51eb81186aec5f747ecab4c5c928f8701afc) , ROM_SKIP(6)|ROM_GROUPWORD )
	ROMX_LOAD( "16499", 0x000004, 0x200000, CRC(a290ea36) SHA1(2503b44174f23a9d323caab86553977d1d6d9c94) , ROM_SKIP(6)|ROM_GROUPWORD )
	ROMX_LOAD( "16501", 0x000006, 0x200000, CRC(f1566620) SHA1(bcf31d11ee669d5afc7dc22c42fa59f4e48c1f50) , ROM_SKIP(6)|ROM_GROUPWORD )
	ROMX_LOAD( "16496", 0x800000, 0x200000, CRC(d9d27247) SHA1(d211623478516ed1b89ab16a7fc7969954c5e353) , ROM_SKIP(6)|ROM_GROUPWORD )
	ROMX_LOAD( "16498", 0x800002, 0x200000, CRC(c022a991) SHA1(a660a20692f4d9ba7be73577328f69f109be5e47) , ROM_SKIP(6)|ROM_GROUPWORD )
	ROMX_LOAD( "16500", 0x800004, 0x200000, CRC(452c0be3) SHA1(af87ce4618bae2d791c1baed34ba7f853af664ff) , ROM_SKIP(6)|ROM_GROUPWORD )
	ROMX_LOAD( "16502", 0x800006, 0x200000, CRC(ffc3147e) SHA1(12d882dec3098674d27058a8009e8778555f477a) , ROM_SKIP(6)|ROM_GROUPWORD )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) /* Sega PCM sound data */
	ROM_LOAD("mp16506.1", 0x000000, 0x200000, CRC(e779f5ed) SHA1(462d1bbe8bb12a0c5a6d6c613c720b26ec21cb25) )
	ROM_LOAD("mp16507.2", 0x200000, 0x200000, CRC(31e068d3) SHA1(9ac88b15af441fb3b31ce759c565b60a09039571) )

	ROM_REGION( 0x20000, REGION_GFX3, 0 ) /* FG tiles */
ROM_END

ROM_START( harddunj )
	ROM_REGION( 0x200000, REGION_CPU1, 0 ) /* v60 code */
	ROM_LOAD32_WORD( "16508.37", 0x000000, 0x40000, CRC(b3713be5) SHA1(8123638a838e41fcc0d32e14382421b521eff94f) )
	ROM_RELOAD(                      0x080000, 0x040000 )
	ROM_LOAD32_WORD( "16509.40", 0x000002, 0x40000, CRC(603dee75) SHA1(32ae964a4b57d470b4900cca6e06329f1a75a6e6) )
	ROM_RELOAD(                      0x080002, 0x040000 )

	ROM_REGION( 0x30000, REGION_CPU2, 0 ) /* sound CPU */
	ROM_LOAD("16505",        0x00000, 0x20000, CRC(eeb90a07) SHA1(d1c2132897994b2e85fd5a97222b9fcd61bc421e) )
	ROM_RELOAD(              0x10000, 0x20000             )

	ROM_REGION( 0x100000, REGION_GFX1, ROMREGION_DISPOSE ) /* tiles */
	ROM_LOAD16_BYTE( "16503", 0x000000, 0x080000, CRC(ac1b6f1a) SHA1(56482931adf7fe551acf796b74cd8af3773d4fef) )
	ROM_LOAD16_BYTE( "16504", 0x000001, 0x080000, CRC(7c61fcd8) SHA1(ca4354f90fada752bf11ee22a7798a8aa22b1c61) )

	ROM_REGION( 0x1000000, REGION_GFX2, 0 ) /* sprites */
	ROMX_LOAD( "16495", 0x000000, 0x200000, CRC(6e5f26be) SHA1(146761072bbed08f4a9df8a474b34fab61afaa4f) , ROM_SKIP(6)|ROM_GROUPWORD )
	ROMX_LOAD( "16497", 0x000002, 0x200000, CRC(42ab5859) SHA1(f50c51eb81186aec5f747ecab4c5c928f8701afc) , ROM_SKIP(6)|ROM_GROUPWORD )
	ROMX_LOAD( "16499", 0x000004, 0x200000, CRC(a290ea36) SHA1(2503b44174f23a9d323caab86553977d1d6d9c94) , ROM_SKIP(6)|ROM_GROUPWORD )
	ROMX_LOAD( "16501", 0x000006, 0x200000, CRC(f1566620) SHA1(bcf31d11ee669d5afc7dc22c42fa59f4e48c1f50) , ROM_SKIP(6)|ROM_GROUPWORD )
	ROMX_LOAD( "16496", 0x800000, 0x200000, CRC(d9d27247) SHA1(d211623478516ed1b89ab16a7fc7969954c5e353) , ROM_SKIP(6)|ROM_GROUPWORD )
	ROMX_LOAD( "16498", 0x800002, 0x200000, CRC(c022a991) SHA1(a660a20692f4d9ba7be73577328f69f109be5e47) , ROM_SKIP(6)|ROM_GROUPWORD )
	ROMX_LOAD( "16500", 0x800004, 0x200000, CRC(452c0be3) SHA1(af87ce4618bae2d791c1baed34ba7f853af664ff) , ROM_SKIP(6)|ROM_GROUPWORD )
	ROMX_LOAD( "16502", 0x800006, 0x200000, CRC(ffc3147e) SHA1(12d882dec3098674d27058a8009e8778555f477a) , ROM_SKIP(6)|ROM_GROUPWORD )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) /* Sega PCM sound data */
	ROM_LOAD("mp16506.1", 0x000000, 0x200000, CRC(e779f5ed) SHA1(462d1bbe8bb12a0c5a6d6c613c720b26ec21cb25) )
	ROM_LOAD("mp16507.2", 0x200000, 0x200000, CRC(31e068d3) SHA1(9ac88b15af441fb3b31ce759c565b60a09039571) )

	ROM_REGION( 0x20000, REGION_GFX3, 0 ) /* FG tiles */
ROM_END

ROM_START( scross )
	ROM_REGION( 0x200000, REGION_CPU1, 0 ) /* v60 code */
	ROM_LOAD32_WORD( "epr15093.bin", 0x000000, 0x040000, CRC(2adc7a4b) SHA1(dca71f00d94898c0758394704d819e13482bf120) )
	ROM_RELOAD(                      0x080000, 0x040000 )
	ROM_LOAD32_WORD( "epr15094.bin", 0x000002, 0x040000, CRC(bbb0ae73) SHA1(0d8837706405f301adf8fa85c8d4813d7600af98) )
	ROM_RELOAD(                      0x080002, 0x040000 )

	/* v60 data */
	ROM_LOAD32_WORD( "epr15018.bin", 0x100000, 0x080000, CRC(3a98385e) SHA1(8088d337655030c28e290da4bbf44cb647dab66c) )
	ROM_LOAD32_WORD( "epr15019.bin", 0x100002, 0x080000, CRC(8bf4ac83) SHA1(e594d9d9b42d0765ed8a20a40b7dd92b75124d34) )

	ROM_REGION( 0x90000, REGION_CPU2, 0 ) /* sound CPU */
	ROM_LOAD("epr15192.bin", 0x00000, 0x20000, CRC(7524290b) SHA1(ee58be2c0c4293ee19622b96ca493f4ce4da0038) )
	ROM_RELOAD(              0x10000, 0x20000             )

	ROM_REGION( 0x400000, REGION_GFX1, ROMREGION_DISPOSE ) /* tiles */
	/* 1ST AND 2ND HALF IDENTICAL (all roms) */
	ROM_LOAD16_BYTE( "epr15020.bin", 0x000000, 0x200000, CRC(65afea2f) SHA1(ad573727398bfac8e94f321be84b60e5690bfba6) )
	ROM_LOAD16_BYTE( "epr15021.bin", 0x000001, 0x200000, CRC(27bc6969) SHA1(d6bb446becb2d36b73bca5055357a43b837afc0a) )

	ROM_REGION( 0x1000000, REGION_GFX2, 0 ) /* sprites */
	/* 1ST AND 2ND HALF IDENTICAL (all roms) */
	ROMX_LOAD( "epr15022.bin", 0x000000, 0x200000, CRC(09ca9608) SHA1(cbd0138c1c7811d42b051fed6a7e3526cc4e457f) , ROM_SKIP(6)|ROM_GROUPWORD )
	ROMX_LOAD( "epr15024.bin", 0x000002, 0x200000, CRC(0dc920eb) SHA1(d24d637aa0dcd3bae779ef7e12663df81667dbf7) , ROM_SKIP(6)|ROM_GROUPWORD )
	ROMX_LOAD( "epr15026.bin", 0x000004, 0x200000, CRC(67637c37) SHA1(7c250e7e9dd5c07da4fa35bacdfcecd5e8fa4ec7) , ROM_SKIP(6)|ROM_GROUPWORD )
	ROMX_LOAD( "epr15028.bin", 0x000006, 0x200000, CRC(9929abdc) SHA1(34b6624ddd3a0aedec0a2b433643a37f745ec66d) , ROM_SKIP(6)|ROM_GROUPWORD )
	ROMX_LOAD( "epr15023.bin", 0x800000, 0x200000, CRC(0e42a2bb) SHA1(503214caf5fa9a2324b61e04f378fd1a790322df) , ROM_SKIP(6)|ROM_GROUPWORD )
	ROMX_LOAD( "epr15025.bin", 0x800002, 0x200000, CRC(0c677fc6) SHA1(fc2207008417072e7ee91f722797d827e150ce2d) , ROM_SKIP(6)|ROM_GROUPWORD )
	ROMX_LOAD( "epr15027.bin", 0x800004, 0x200000, CRC(d6d077f9) SHA1(928cefae9ae58239fbffb1dcee282c6ac1e661fe) , ROM_SKIP(6)|ROM_GROUPWORD )
	ROMX_LOAD( "epr15029.bin", 0x800006, 0x200000, CRC(707af749) SHA1(fae5325c983df3cf198878220ad88d47339ac512) , ROM_SKIP(6)|ROM_GROUPWORD )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) /* Sega PCM sound data */
	/* 1ST AND 2ND HALF IDENTICAL (all roms, are these OK?) */
	ROM_LOAD("epr15031.bin", 0x000000, 0x200000, CRC(663a7fd2) SHA1(b4393a687225b075db21960d19a6ddd7a9d7d086) )
	ROM_LOAD("epr15032.bin", 0x200000, 0x200000, CRC(cb709f3d) SHA1(3962c8b5907d1f8f611f58ddac693cc47364a79c) )

	ROM_REGION( 0x20000, REGION_GFX3, 0 ) /* FG tiles */
ROM_END

ROM_START( titlef )
	ROM_REGION( 0x200000, REGION_CPU1, 0 ) /* v60 code */
	ROM_LOAD32_WORD( "epr15388.37", 0x000000, 0x40000, CRC(db1eefbd) SHA1(7059a1d5c9364d836c1d922071a108cbde661e0a) )
	ROM_RELOAD(                      0x080000, 0x040000 )
	ROM_LOAD32_WORD( "epr15389.40", 0x000002, 0x40000, CRC(da9f60a3) SHA1(87a7bea04e51e3c241871e83ff7322c6a07bd106) )
	ROM_RELOAD(                      0x080002, 0x040000 )

	ROM_REGION( 0x30000, REGION_CPU2, 0 ) /* sound CPU */
	ROM_LOAD("epr15384.31", 0x00000, 0x20000, CRC(0f7d208d) SHA1(5425120480f813210fae28951e8bfd5acb08ca53) )
	ROM_RELOAD(              0x10000, 0x20000             )

	ROM_REGION( 0x400000, REGION_GFX1, ROMREGION_DISPOSE ) /* tiles */
	ROM_LOAD16_BYTE( "mpr15381.3", 0x000000, 0x200000, CRC(162cc4d6) SHA1(2369d3d76ab5ef8f033aa45530ab957f0e5ff028) )
	ROM_LOAD16_BYTE( "mpr15382.11", 0x000001, 0x200000, CRC(fd03a130) SHA1(040c36383ef5d8298af714958cd5b0a4c7556ae7) )

	ROM_REGION( 0x1000000, REGION_GFX2, 0 ) /* sprites */
	ROMX_LOAD( "mpr15379.14", 0x000000, 0x200000, CRC(e5c74b11) SHA1(67e4460efe5dcd88ffc12024b255efc843e6a8b5) , ROM_SKIP(6)|ROM_GROUPWORD )
	ROMX_LOAD( "mpr15375.15", 0x000002, 0x200000, CRC(046a9b50) SHA1(2b4c53f2a0264835cb7197daa9b3461c212541e8) , ROM_SKIP(6)|ROM_GROUPWORD )
	ROMX_LOAD( "mpr15371.10", 0x000004, 0x200000, CRC(999046c6) SHA1(37ce4e8aaf537b5366eacabaf36e4477b5624121) , ROM_SKIP(6)|ROM_GROUPWORD )
	ROMX_LOAD( "mpr15373.38", 0x000006, 0x200000, CRC(9b3294d9) SHA1(19542f14ce09753385a44098dfd1aaf331e7af0e) , ROM_SKIP(6)|ROM_GROUPWORD )
	ROMX_LOAD( "mpr15380.22", 0x800000, 0x200000, CRC(6ea0e58d) SHA1(1c4b761522157b0b9d086181ba6f6994879d8fdf) , ROM_SKIP(6)|ROM_GROUPWORD )
	ROMX_LOAD( "mpr15376.23", 0x800002, 0x200000, CRC(de3e05c5) SHA1(cac0d04ecd37e5836d246c0809bcfc11430df591) , ROM_SKIP(6)|ROM_GROUPWORD )
	ROMX_LOAD( "mpr15372.18", 0x800004, 0x200000, CRC(c187c36a) SHA1(bb55c2a768a43ef19a7847a4aa113523fee26c20) , ROM_SKIP(6)|ROM_GROUPWORD )
	ROMX_LOAD( "mpr15374.41", 0x800006, 0x200000, CRC(e026aab0) SHA1(75dfaef6d50c3d1d7f27aa5e44fcbc0ff2173c6f) , ROM_SKIP(6)|ROM_GROUPWORD )

	ROM_REGION( 0x300000, REGION_SOUND1, 0 ) /* Sega PCM sound data */
	ROM_LOAD("mpr15385.1", 0x000000, 0x200000, CRC(5a9b0aa0) SHA1(d208aa165f9eea05e3b8c3f406ff44374e4f6887) )

	ROM_REGION( 0x20000, REGION_GFX3, 0 ) /* FG tiles */
ROM_END

/* boot, and are playable, some gfx problems*/
GAMEX( 1992, orunners,     0,        multi32, orunners, orunners, ROT0, "Sega", "Outrunners (US)", GAME_IMPERFECT_GRAPHICS )
GAMEX( 1994, harddunk,     0,        multi32, harddunk, harddunk, ROT0, "Sega", "Hard Dunk (World)", GAME_IMPERFECT_GRAPHICS )
GAMEX( 1994, harddunj,     harddunk, multi32, harddunk, harddunk, ROT0, "Sega", "Hard Dunk (Japan)", GAME_IMPERFECT_GRAPHICS )
GAMEX( 1992, scross,       0,        scross,  scross,   orunners, ROT0, "Sega", "Stadium Cross (World)", GAME_IMPERFECT_GRAPHICS )
GAMEX( 1992, titlef,       0,        multi32, titlef,   titlef,   ROT0, "Sega", "Title Fight (World)", GAME_IMPERFECT_GRAPHICS )
