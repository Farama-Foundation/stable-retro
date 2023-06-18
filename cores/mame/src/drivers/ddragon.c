/***************************************************************************

Double Dragon     (c) 1987 Technos Japan
Double Dragon II  (c) 1988 Technos Japan

Driver by Carlos A. Lozano, Rob Rosenbrock, Phil Stroffolino, Ernesto Corvi
Toffy / Super Toffy added by David Haywood
Thanks to Bryan McPhail for spotting the Toffy program rom encryption
Toffy / Super Toffy sound hooked up by R. Belmont.

BM, 8/1/2006:

Double Dragon has a crash which sometimes occurs at the very end of the game
(right before the final animation sequence).  It occurs because of a jump look up
table:

    BAD3: LDY   #$BADD
    BAD7: JSR   [A,Y]

At the point of the crash A is 0x3e which causes a jump to 0x3401 (background tile
ram) which obviously doesn't contain proper code and causes a crash.  The jump
table has 32 entries, and only the last contains an invalid jump vector.  A is set
to 0x3e as a result of code at 0x625f - it reads from the shared spriteram (0x2049
in main cpu memory space), copies the value to 0x523 (main ram) where it is later
fetched and shifted to make 0x3e.

So..  it's not clear where the error is - the 0x1f value is actually written to
shared RAM by the main CPU - perhaps the MCU should modify it before the main CPU
reads it back?  Perhaps 0x1f should never be written at all?  If you want to trace
this further please submit a proper fix!  In the meantime I have patched the error
by making sure the invalid jump is never taken - this fixes the crash (see
ddragon_spriteram_r).



Modifications by Bryan McPhail, June-November 2003:

Correct video & interrupt timing derived from Xain schematics and confirmed on real DD board.
Corrected interrupt handling, epecially to MCU (but one semi-hack remains).
TStrike now boots but sprites don't appear (I had them working at one point, can't remember what broke them again).
Dangerous Dungeons fixed.
World version of Double Dragon added (actually same roms as the bootleg, but confirmed from real board)
Removed stereo audio flag (still on Toffy - does it have it?)

todo:

banking in Toffy / Super toffy

-- Read Me --

Super Toffy - Unico 1994

Main cpu: 	MC6809EP
Sound cpu: 	MC6809P
Sound: 		YM2151
Clocks:		12 MHz, 3.579MHz

Graphics custom: MDE-2001

-- --

Does this make Super Toffy the sequel to a rip-off / bootleg of a
conversion kit which could be applied to a bootleg double dragon :-p?

***************************************************************************/

#include "driver.h"
#include "state.h"
#include "cpu/m6800/m6800.h"
#include "cpu/m6809/m6809.h"
#include "cpu/z80/z80.h"
#include "vidhrdw/generic.h"
#include "ost_samples.h"

static void ddragon_restore_state(void);
/* from vidhrdw */
extern unsigned char *ddragon_bgvideoram,*ddragon_fgvideoram;
extern int ddragon_scrollx_hi, ddragon_scrolly_hi;
extern unsigned char *ddragon_scrollx_lo;
extern unsigned char *ddragon_scrolly_lo;
VIDEO_START( ddragon );
VIDEO_UPDATE( ddragon );
WRITE_HANDLER( ddragon_bgvideoram_w );
WRITE_HANDLER( ddragon_fgvideoram_w );
extern unsigned char *ddragon_spriteram;
extern int technos_video_hw;
/* end of extern code & data */


/* private globals */
static int dd_sub_cpu_busy;
static int m_sprite_irq, m_sound_irq, ym_irq, snd_cpu;
static int adpcm_pos[2],adpcm_end[2],adpcm_idle[2];
static int adpcm_data[2] = { -1, -1 };
static UINT8* darktowr_mcu_ports, *darktowr_ram;
static int VBLK;
static UINT8 m_ddragon_sub_port;
/* end of private globals */


static MACHINE_INIT( ddragon )
{
	m_sprite_irq = IRQ_LINE_NMI;
	m_sound_irq = M6809_IRQ_LINE;
	ym_irq = M6809_FIRQ_LINE;
	technos_video_hw = 0;
	dd_sub_cpu_busy = 0x10;
	snd_cpu = 2;
	adpcm_pos[0] = adpcm_pos[1] = 0;
	adpcm_end[0] = adpcm_end[1] = 0;
	adpcm_idle[0] = adpcm_idle[1] = 1;
	adpcm_data[0] = adpcm_data[1] = -1;

	state_save_register_int("ddragon", 0, "dd_sub_cpu_busy", &dd_sub_cpu_busy);
	state_save_register_int("ddragon", 0, "adpcm_idle[0]", &adpcm_idle[0]);
	state_save_register_int("ddragon", 0, "adpcm_idle[1]", &adpcm_idle[1]);
	state_save_register_int("ddragon", 0, "adpcm_pos[0]", &adpcm_pos[0]);
	state_save_register_int("ddragon", 0, "adpcm_pos[1]", &adpcm_pos[1]);
	state_save_register_int("ddragon", 0, "adpcm_end[0]", &adpcm_end[0]);
	state_save_register_int("ddragon", 0, "adpcm_end[1]", &adpcm_end[1]);
	state_save_register_int("ddragon", 0, "adpcm_data[0]", &adpcm_data[0]);
	state_save_register_int("ddragon", 0, "adpcm_data[1]", &adpcm_data[1]);
	state_save_register_int("ddragon", 0, "ddragon_scrollx_hi", &ddragon_scrollx_hi);
	state_save_register_int("ddragon", 0, "ddragon_scrolly_hi", &ddragon_scrolly_hi);
	state_save_register_UINT8("ddragon", 0, "m_ddragon_sub_port", &m_ddragon_sub_port, 1);
	state_save_register_func_postload(ddragon_restore_state);
/*msm5205 soundcore needs savestates added */
}


static MACHINE_INIT( toffy )
{
	m_sound_irq = M6809_IRQ_LINE;
	ym_irq = M6809_FIRQ_LINE;
	technos_video_hw = 0;
	dd_sub_cpu_busy = 0x10;
	adpcm_idle[0] = adpcm_idle[1] = 1;
	snd_cpu = 1;
}

static MACHINE_INIT( ddragonb )
{
	m_sprite_irq = IRQ_LINE_NMI;
	m_sound_irq = M6809_IRQ_LINE;
	ym_irq = M6809_FIRQ_LINE;
	technos_video_hw = 0;
	dd_sub_cpu_busy = 0x10;
	adpcm_idle[0] = adpcm_idle[1] = 1;
	snd_cpu = 2;
}

static MACHINE_INIT( ddragon2 )
{
	m_sprite_irq = IRQ_LINE_NMI;
	m_sound_irq = IRQ_LINE_NMI;
	ym_irq = 0;
	technos_video_hw = 2;
	dd_sub_cpu_busy = 0x10;
	snd_cpu = 2;
}

/*****************************************************************************/

static WRITE_HANDLER( ddragon_bankswitch_w )
{
	UINT8 *RAM = memory_region(REGION_CPU1);

	ddragon_scrolly_hi = ( ( data & 0x02 ) << 7 );
	ddragon_scrollx_hi = ( ( data & 0x01 ) << 8 );

	flip_screen_set(~data & 0x04);

	/* bit 3 unknown */

	if (data & 0x10)
		dd_sub_cpu_busy = 0x00;
	else if (dd_sub_cpu_busy == 0x00)
		cpu_set_irq_line( 1, m_sprite_irq, (m_sprite_irq == IRQ_LINE_NMI) ? PULSE_LINE : HOLD_LINE );

	cpu_setbank( 1,&RAM[ 0x10000 + ( 0x4000 * ( ( data & 0xe0) >> 5 ) ) ] );

	m_ddragon_sub_port=data;
}

static WRITE_HANDLER( toffy_bankswitch_w )
{
	UINT8 *RAM = memory_region(REGION_CPU1);

	ddragon_scrolly_hi = ( ( data & 0x02 ) << 7 );
	ddragon_scrollx_hi = ( ( data & 0x01 ) << 8 );

/*	flip_screen_set(~data & 0x04);*/

	/* bit 3 unknown */

	/* I don't know ... */
	cpu_setbank( 1,&RAM[ 0x10000 + ( 0x4000 * ( ( data & 0x20) >> 5 ) ) ] );
}

/*****************************************************************************/

static int darktowr_bank=0;

static WRITE_HANDLER( darktowr_bankswitch_w )
{
	ddragon_scrolly_hi = ( ( data & 0x02 ) << 7 );
	ddragon_scrollx_hi = ( ( data & 0x01 ) << 8 );

/*	flip_screen_set(~data & 0x04);*/

	/* bit 3 unknown */

	if (data & 0x10)
		dd_sub_cpu_busy = 0x00;
	else if (dd_sub_cpu_busy == 0x00)
		cpu_set_irq_line( 1, m_sprite_irq, (m_sprite_irq == IRQ_LINE_NMI) ? PULSE_LINE : HOLD_LINE );

	darktowr_bank=(data & 0xe0) >> 5;
/*	cpu_setbank( 1,&RAM[ 0x10000 + ( 0x4000 * ( ( data & 0xe0) >> 5 ) ) ] );*/
/*	log_cb(RETRO_LOG_DEBUG, LOGPRE "Bank %05x %02x %02x\n",activecpu_get_pc(),darktowr_bank,data);*/
}

static READ_HANDLER( darktowr_bank_r )
{
	UINT8 *RAM = memory_region(REGION_CPU1);

	/* MCU is mapped into main cpu memory as a bank */
	if (darktowr_bank==4) {
		/* log_cb(RETRO_LOG_DEBUG, LOGPRE "BankRead %05x %08x\n",activecpu_get_pc(),offset); */

		/* Horrible hack - the alternate TStrike set is mismatched against the MCU,
        so just hack around the protection here.  (The hacks are 'right' as I have
        the original source code & notes to this version of TStrike to examine).
        */
		if(!strcmp(Machine->gamedrv->name, "tstrike"))
		{
			/* Static protection checks at boot-up */
			if (activecpu_get_pc()==0x9ace)
				return 0;
			if (activecpu_get_pc()==0x9ae4)
				return 0x63;

			/* Just return whatever the code is expecting */
			return darktowr_ram[0xbe1];
		}

		if (offset==0x1401 || offset==1) {
			return darktowr_mcu_ports[0];
		}

		log_cb(RETRO_LOG_DEBUG, LOGPRE "Unmapped mcu bank read %04x\n",offset);
		return 0xff;
	}

	return RAM[offset + 0x10000 + (0x4000*darktowr_bank)];
}

static WRITE_HANDLER( darktowr_bank_w )
{
	if (darktowr_bank==4) {
		log_cb(RETRO_LOG_DEBUG, LOGPRE "BankWrite %05x %08x %08x\n",activecpu_get_pc(),offset,data);

		if (offset==0x1400 || offset==0) {
			int bitSwappedData=BITSWAP8(data,0,1,2,3,4,5,6,7);

			darktowr_mcu_ports[1]=bitSwappedData;

			log_cb(RETRO_LOG_DEBUG, LOGPRE "MCU PORT 1 -> %04x (from %04x)\n",bitSwappedData,data);
			return;
		}
		return;
	}

	log_cb(RETRO_LOG_DEBUG, LOGPRE "ROM write! %04x %02x\n",offset,data);
}

static READ_HANDLER( darktowr_mcu_r )
{
	return darktowr_mcu_ports[offset];
}

static WRITE_HANDLER( darktowr_mcu_w )
{
	log_cb(RETRO_LOG_DEBUG, LOGPRE "McuWrite %05x %08x %08x\n",activecpu_get_pc(),offset,data);
	darktowr_mcu_ports[offset]=data;
}

/**************************************************************************/

static WRITE_HANDLER( ddragon_interrupt_w )
{
	switch (offset) {
	case 0: /* 380b - NMI ack */
		cpu_set_nmi_line(0,CLEAR_LINE);
		break;
	case 1: /* 380c - FIRQ ack */
		cpu_set_irq_line(0,M6809_FIRQ_LINE,CLEAR_LINE);
		break;
	case 2: /* 380d - IRQ ack */
		cpu_set_irq_line(0,M6809_IRQ_LINE,CLEAR_LINE);
		break;
	case 3: /* 380e - SND irq */
		if( ost_support_enabled(OST_SUPPORT_DDRAGON) ) {
			if(generate_ost_sound( data )) {
				soundlatch_w( 0, data );
				cpu_set_irq_line( snd_cpu, m_sound_irq, (m_sound_irq == IRQ_LINE_NMI) ? PULSE_LINE : HOLD_LINE );
			}
		}
		else {
			soundlatch_w( 0, data );
			cpu_set_irq_line( snd_cpu, m_sound_irq, (m_sound_irq == IRQ_LINE_NMI) ? PULSE_LINE : HOLD_LINE );
		}
		break;
	case 4: /* 380f - ? */
		/* Not sure what this is - almost certainly related to the sprite mcu */
		break;
	};
}

static READ_HANDLER( ddragon_hd63701_internal_registers_r )
{
	log_cb(RETRO_LOG_DEBUG, LOGPRE "%04x: read %d\n",activecpu_get_pc(),offset);
	return 0;
}


static WRITE_HANDLER( ddragon_hd63701_internal_registers_w )
{

/* Port 6 */
	if (offset == 0x17)
	{
		if ((data & 0x1) == 0)
			cpu_set_irq_line(0,M6809_IRQ_LINE,ASSERT_LINE);

		if (!(m_ddragon_sub_port & 0x2) && (data & 0x2))
			cpu_set_irq_line(0,M6809_IRQ_LINE,ASSERT_LINE);

		m_ddragon_sub_port = data;
	}
}

static WRITE_HANDLER( ddragon2_sub_irq_ack_w )
{
	cpu_set_irq_line(1,m_sprite_irq, CLEAR_LINE );
}

static WRITE_HANDLER( ddragon2_sub_irq_w )
{
	cpu_set_irq_line(0,M6809_IRQ_LINE,ASSERT_LINE);
}

static READ_HANDLER( port4_r )
{
	int port = readinputport( 4 );

	return port | dd_sub_cpu_busy | VBLK;
}

static READ_HANDLER( ddragon_spriteram_r )
{
	return ddragon_spriteram[offset];
}

static WRITE_HANDLER( ddragon_spriteram_w )
{
	if ( cpu_getactivecpu() == 1 && offset == 0 )
		dd_sub_cpu_busy = 0x10;

	ddragon_spriteram[offset] = data;
}

/*****************************************************************************/

#if 0
static WRITE_HANDLER( cpu_sound_command_w )
{
	soundlatch_w( offset, data );
	cpu_set_irq_line( snd_cpu, m_sound_irq, (m_sound_irq == IRQ_LINE_NMI) ? PULSE_LINE : HOLD_LINE );
}
#endif

static WRITE_HANDLER( dd_adpcm_w )
{
	int chip = offset & 1;

	switch (offset/2)
	{
		case 3:
			adpcm_idle[chip] = 1;
			MSM5205_reset_w(chip,1);
			break;

		case 2:
			adpcm_pos[chip] = (data & 0x7f) * 0x200;
			break;

		case 1:
			adpcm_end[chip] = (data & 0x7f) * 0x200;
			break;

		case 0:
			adpcm_idle[chip] = 0;
			MSM5205_reset_w(chip,0);
			break;
	}
}

static void dd_adpcm_int(int chip)
{
	if (adpcm_pos[chip] >= adpcm_end[chip] || adpcm_pos[chip] >= 0x10000)
	{
		adpcm_idle[chip] = 1;
		MSM5205_reset_w(chip,1);
	}
	else if (adpcm_data[chip] != -1)
	{
		MSM5205_data_w(chip,adpcm_data[chip] & 0x0f);
		adpcm_data[chip] = -1;
	}
	else
	{
		UINT8 *ROM = memory_region(REGION_SOUND1) + 0x10000 * chip;

		adpcm_data[chip] = ROM[adpcm_pos[chip]++];
		MSM5205_data_w(chip,adpcm_data[chip] >> 4);
	}
}

static READ_HANDLER( dd_adpcm_status_r )
{
	return adpcm_idle[0] + (adpcm_idle[1] << 1);
}

/*****************************************************************************/

static MEMORY_READ_START( readmem )
	{ 0x0000, 0x1fff, MRA_RAM },
	{ 0x2000, 0x2fff, ddragon_spriteram_r },
	{ 0x3000, 0x37ff, MRA_RAM },
	{ 0x3800, 0x3800, input_port_0_r },
	{ 0x3801, 0x3801, input_port_1_r },
	{ 0x3802, 0x3802, port4_r },
	{ 0x3803, 0x3803, input_port_2_r },
	{ 0x3804, 0x3804, input_port_3_r },
	{ 0x4000, 0x7fff, MRA_BANK1 },
	{ 0x8000, 0xffff, MRA_ROM },
MEMORY_END

static MEMORY_WRITE_START( writemem )
	{ 0x0000, 0x0fff, MWA_RAM },
	{ 0x1000, 0x11ff, paletteram_xxxxBBBBGGGGRRRR_split1_w, &paletteram },
	{ 0x1200, 0x13ff, paletteram_xxxxBBBBGGGGRRRR_split2_w, &paletteram_2 },
	{ 0x1400, 0x17ff, MWA_RAM },
	{ 0x1800, 0x1fff, ddragon_fgvideoram_w, &ddragon_fgvideoram },
	{ 0x2000, 0x2fff, ddragon_spriteram_w, &ddragon_spriteram },
	{ 0x3000, 0x37ff, ddragon_bgvideoram_w, &ddragon_bgvideoram },
	{ 0x3808, 0x3808, ddragon_bankswitch_w },
	{ 0x3809, 0x3809, MWA_RAM, &ddragon_scrollx_lo },
	{ 0x380a, 0x380a, MWA_RAM, &ddragon_scrolly_lo },
	{ 0x380b, 0x380f, ddragon_interrupt_w },
	{ 0x4000, 0xffff, MWA_ROM },
MEMORY_END

static MEMORY_READ_START( darktowr_readmem )
	{ 0x0000, 0x1fff, MRA_RAM },
	{ 0x2000, 0x2fff, ddragon_spriteram_r },
	{ 0x3000, 0x37ff, MRA_RAM },
	{ 0x3800, 0x3800, input_port_0_r },
	{ 0x3801, 0x3801, input_port_1_r },
	{ 0x3802, 0x3802, port4_r },
	{ 0x3803, 0x3803, input_port_2_r },
	{ 0x3804, 0x3804, input_port_3_r },
	{ 0x4000, 0x7fff, darktowr_bank_r },
	{ 0x8000, 0xffff, MRA_ROM },
MEMORY_END

static MEMORY_WRITE_START( darktowr_writemem )
	{ 0x0000, 0x0fff, MWA_RAM, &darktowr_ram },
	{ 0x1000, 0x11ff, paletteram_xxxxBBBBGGGGRRRR_split1_w, &paletteram },
	{ 0x1200, 0x13ff, paletteram_xxxxBBBBGGGGRRRR_split2_w, &paletteram_2 },
	{ 0x1400, 0x17ff, MWA_RAM },
	{ 0x1800, 0x1fff, ddragon_fgvideoram_w, &ddragon_fgvideoram },
	{ 0x2000, 0x2fff, ddragon_spriteram_w, &ddragon_spriteram },
	{ 0x3000, 0x37ff, ddragon_bgvideoram_w, &ddragon_bgvideoram },
	{ 0x3808, 0x3808, darktowr_bankswitch_w },
	{ 0x3809, 0x3809, MWA_RAM, &ddragon_scrollx_lo },
	{ 0x380a, 0x380a, MWA_RAM, &ddragon_scrolly_lo },
	{ 0x380b, 0x380f, ddragon_interrupt_w },
	{ 0x4000, 0x7fff, darktowr_bank_w },
	{ 0x8000, 0xffff, MWA_ROM },
MEMORY_END

static MEMORY_READ_START( dd2_readmem )
	{ 0x0000, 0x1fff, MRA_RAM },
	{ 0x2000, 0x2fff, ddragon_spriteram_r },
	{ 0x3000, 0x37ff, MRA_RAM },
	{ 0x3800, 0x3800, input_port_0_r },
	{ 0x3801, 0x3801, input_port_1_r },
	{ 0x3802, 0x3802, port4_r },
	{ 0x3803, 0x3803, input_port_2_r },
	{ 0x3804, 0x3804, input_port_3_r },
	{ 0x3c00, 0x3fff, MRA_RAM },
	{ 0x4000, 0x7fff, MRA_BANK1 },
	{ 0x8000, 0xffff, MRA_ROM },
MEMORY_END

static MEMORY_WRITE_START( dd2_writemem )
	{ 0x0000, 0x17ff, MWA_RAM },
	{ 0x1800, 0x1fff, ddragon_fgvideoram_w, &ddragon_fgvideoram },
	{ 0x2000, 0x2fff, ddragon_spriteram_w, &ddragon_spriteram },
	{ 0x3000, 0x37ff, ddragon_bgvideoram_w, &ddragon_bgvideoram },
	{ 0x3808, 0x3808, ddragon_bankswitch_w },
	{ 0x3809, 0x3809, MWA_RAM, &ddragon_scrollx_lo },
	{ 0x380a, 0x380a, MWA_RAM, &ddragon_scrolly_lo },
	{ 0x380b, 0x380f, ddragon_interrupt_w },
	{ 0x3c00, 0x3dff, paletteram_xxxxBBBBGGGGRRRR_split1_w, &paletteram },
	{ 0x3e00, 0x3fff, paletteram_xxxxBBBBGGGGRRRR_split2_w, &paletteram_2 },
	{ 0x4000, 0xffff, MWA_ROM },
MEMORY_END

static MEMORY_WRITE_START( toffy_writemem )
	{ 0x0000, 0x0fff, MWA_RAM },
	{ 0x1000, 0x11ff, paletteram_xxxxBBBBGGGGRRRR_split1_w, &paletteram },
	{ 0x1200, 0x13ff, paletteram_xxxxBBBBGGGGRRRR_split2_w, &paletteram_2 },
	{ 0x1400, 0x17ff, MWA_RAM },
	{ 0x1800, 0x1fff, ddragon_fgvideoram_w, &ddragon_fgvideoram },
	{ 0x2000, 0x2fff, ddragon_spriteram_w, &ddragon_spriteram },
	{ 0x3000, 0x37ff, ddragon_bgvideoram_w, &ddragon_bgvideoram },
	{ 0x3808, 0x3808, toffy_bankswitch_w },
	{ 0x3809, 0x3809, MWA_RAM, &ddragon_scrollx_lo },
	{ 0x380a, 0x380a, MWA_RAM, &ddragon_scrolly_lo },
	{ 0x380b, 0x380f, ddragon_interrupt_w },
	{ 0x4000, 0xffff, MWA_ROM },
MEMORY_END

static MEMORY_READ_START( sub_readmem )
	{ 0x0000, 0x001f, ddragon_hd63701_internal_registers_r },
	{ 0x001f, 0x0fff, MRA_RAM },
	{ 0x8000, 0x8fff, ddragon_spriteram_r },
	{ 0xc000, 0xffff, MRA_ROM },
MEMORY_END

static MEMORY_WRITE_START( sub_writemem )
	{ 0x0000, 0x001f, ddragon_hd63701_internal_registers_w },
	{ 0x001f, 0x0fff, MWA_RAM },
	{ 0x8000, 0x8fff, ddragon_spriteram_w },
	{ 0xc000, 0xffff, MWA_ROM },
MEMORY_END

static MEMORY_READ_START( sound_readmem )
	{ 0x0000, 0x0fff, MRA_RAM },
	{ 0x1000, 0x1000, soundlatch_r },
	{ 0x1800, 0x1800, dd_adpcm_status_r },
	{ 0x2800, 0x2801, YM2151_status_port_0_r },
	{ 0x8000, 0xffff, MRA_ROM },
MEMORY_END

static MEMORY_WRITE_START( sound_writemem )
	{ 0x0000, 0x0fff, MWA_RAM },
	{ 0x2800, 0x2800, YM2151_register_port_0_w },
	{ 0x2801, 0x2801, YM2151_data_port_0_w },
	{ 0x3800, 0x3807, dd_adpcm_w },
	{ 0x8000, 0xffff, MWA_ROM },
MEMORY_END

static MEMORY_READ_START( dd2_sub_readmem )
	{ 0x0000, 0xbfff, MRA_ROM },
	{ 0xc000, 0xc3ff, ddragon_spriteram_r },
MEMORY_END

static MEMORY_WRITE_START( dd2_sub_writemem )
	{ 0x0000, 0xbfff, MWA_ROM },
	{ 0xc000, 0xc3ff, ddragon_spriteram_w },
	{ 0xd000, 0xd000, ddragon2_sub_irq_ack_w },
	{ 0xe000, 0xe000, ddragon2_sub_irq_w },
MEMORY_END

static MEMORY_READ_START( dd2_sound_readmem )
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0x87ff, MRA_RAM },
	{ 0x8801, 0x8801, YM2151_status_port_0_r },
	{ 0x9800, 0x9800, OKIM6295_status_0_r },
	{ 0xA000, 0xA000, soundlatch_r },
MEMORY_END

static MEMORY_WRITE_START( dd2_sound_writemem )
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0x8000, 0x87ff, MWA_RAM },
	{ 0x8800, 0x8800, YM2151_register_port_0_w },
	{ 0x8801, 0x8801, YM2151_data_port_0_w },
	{ 0x9800, 0x9800, OKIM6295_data_0_w },
MEMORY_END

static MEMORY_READ_START( mcu_readmem )
	{ 0x0000, 0x0007, darktowr_mcu_r },
	{ 0x0008, 0x007f, MRA_RAM },
	{ 0x0080, 0x07ff, MRA_ROM },
MEMORY_END

static MEMORY_WRITE_START( mcu_writemem )
	{ 0x0000, 0x0007, darktowr_mcu_w, &darktowr_mcu_ports },
	{ 0x0008, 0x007f, MWA_RAM },
	{ 0x0080, 0x07ff, MWA_ROM },
MEMORY_END

/*****************************************************************************/

#define COMMON_PORT4	PORT_START \
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_SERVICE1 ) \
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON3 ) \
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 ) \
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_SPECIAL ) /* Vblank verified to be active high (palette fades in ddragon2) */ \
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_SPECIAL ) /* sub cpu busy */ \
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN ) \
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN ) \
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

#define COMMON_INPUT_DIP1 PORT_START      /* DSW0 */ \
	PORT_DIPNAME( 0x07, 0x07, DEF_STR( Coin_A ) ) \
	PORT_DIPSETTING(    0x00, DEF_STR( 4C_1C ) ) \
	PORT_DIPSETTING(    0x01, DEF_STR( 3C_1C ) ) \
	PORT_DIPSETTING(    0x02, DEF_STR( 2C_1C ) ) \
	PORT_DIPSETTING(    0x07, DEF_STR( 1C_1C ) ) \
	PORT_DIPSETTING(    0x06, DEF_STR( 1C_2C ) ) \
	PORT_DIPSETTING(    0x05, DEF_STR( 1C_3C ) ) \
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_4C ) ) \
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_5C ) ) \
	PORT_DIPNAME( 0x38, 0x38, DEF_STR( Coin_B ) ) \
	PORT_DIPSETTING(    0x00, DEF_STR( 4C_1C ) ) \
	PORT_DIPSETTING(    0x08, DEF_STR( 3C_1C ) ) \
	PORT_DIPSETTING(    0x10, DEF_STR( 2C_1C ) ) \
	PORT_DIPSETTING(    0x38, DEF_STR( 1C_1C ) ) \
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_2C ) ) \
	PORT_DIPSETTING(    0x28, DEF_STR( 1C_3C ) ) \
	PORT_DIPSETTING(    0x20, DEF_STR( 1C_4C ) ) \
	PORT_DIPSETTING(    0x18, DEF_STR( 1C_5C ) ) \
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Cabinet ) ) \
	PORT_DIPSETTING(    0x40, DEF_STR( Upright ) ) \
	PORT_DIPSETTING(    0x00, DEF_STR( Cocktail ) ) \
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Flip_Screen ) ) \
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) ) \
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

#define COMMON_INPUT_DIP2 PORT_START      /* DSW0 */ \
	PORT_DIPNAME( 0x0f, 0x00, DEF_STR( Coin_A ) ) \
	PORT_DIPSETTING(    0x03, DEF_STR( 4C_1C ) ) \
	PORT_DIPSETTING(    0x02, DEF_STR( 3C_1C ) ) \
	PORT_DIPSETTING(    0x07, DEF_STR( 4C_2C ) ) \
	PORT_DIPSETTING(    0x01, DEF_STR( 2C_1C ) ) \
	PORT_DIPSETTING(    0x06, DEF_STR( 3C_2C ) ) \
	PORT_DIPSETTING(    0x0b, DEF_STR( 4C_3C ) ) \
	PORT_DIPSETTING(    0x0f, DEF_STR( 4C_4C ) ) \
	PORT_DIPSETTING(    0x0a, DEF_STR( 3C_3C ) ) \
	PORT_DIPSETTING(    0x05, DEF_STR( 2C_2C ) ) \
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) ) \
	PORT_DIPSETTING(    0x0e, DEF_STR( 3C_4C ) ) \
	PORT_DIPSETTING(    0x09, DEF_STR( 2C_3C ) ) \
	PORT_DIPSETTING(    0x0d, DEF_STR( 2C_4C ) ) \
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_2C ) ) \
	PORT_DIPSETTING(    0x08, DEF_STR( 1C_3C ) ) \
	PORT_DIPSETTING(    0x0c, DEF_STR( 1C_4C ) ) \
	PORT_DIPNAME( 0xf0, 0x00, DEF_STR( Coin_B ) ) \
	PORT_DIPSETTING(    0x30, DEF_STR( 4C_1C ) ) \
	PORT_DIPSETTING(    0x20, DEF_STR( 3C_1C ) ) \
	PORT_DIPSETTING(    0x70, DEF_STR( 4C_2C ) ) \
	PORT_DIPSETTING(    0x10, DEF_STR( 2C_1C ) ) \
	PORT_DIPSETTING(    0x60, DEF_STR( 3C_2C ) ) \
	PORT_DIPSETTING(    0xb0, DEF_STR( 4C_3C ) ) \
	PORT_DIPSETTING(    0xf0, DEF_STR( 4C_4C ) ) \
	PORT_DIPSETTING(    0xa0, DEF_STR( 3C_3C ) ) \
	PORT_DIPSETTING(    0x50, DEF_STR( 2C_2C ) ) \
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) ) \
	PORT_DIPSETTING(    0xe0, DEF_STR( 3C_4C ) ) \
	PORT_DIPSETTING(    0x90, DEF_STR( 2C_3C ) ) \
	PORT_DIPSETTING(    0xd0, DEF_STR( 2C_4C ) ) \
	PORT_DIPSETTING(    0x40, DEF_STR( 1C_2C ) ) \
	PORT_DIPSETTING(    0x80, DEF_STR( 1C_3C ) ) \
	PORT_DIPSETTING(    0xc0, DEF_STR( 1C_4C ) )

#define COMMON_INPUT_PORTS PORT_START \
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY ) \
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY ) \
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY ) \
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY ) \
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 ) \
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 ) \
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START1 ) \
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START2 ) \
	PORT_START \
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 ) \
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 ) \
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 ) \
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 ) \
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 ) \
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 ) \
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN1 ) \
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN2 )


INPUT_PORTS_START( darktowr )
	COMMON_INPUT_PORTS

	COMMON_INPUT_DIP2

	PORT_START      /* DSW1 */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
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

	COMMON_PORT4
INPUT_PORTS_END

INPUT_PORTS_START( tstrike )
	COMMON_INPUT_PORTS

	COMMON_INPUT_DIP1

	PORT_START /*DSW1 */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x0c, "Easy" )
	PORT_DIPSETTING(    0x08, "Normal" )
	PORT_DIPSETTING(    0x04, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x30, "1" )
	PORT_DIPSETTING(    0x20, "2" )
	PORT_DIPSETTING(    0x10, "3" )
	PORT_DIPSETTING(    0x00, "4" )
	PORT_DIPNAME( 0xc0, 0xc0, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0xc0, "100k and 200k" )
	PORT_DIPSETTING(    0x80, "200k and 300k" )
	PORT_DIPSETTING(    0x40, "300k and 400k" )
	PORT_DIPSETTING(    0x00, "400k and 500k" )

	COMMON_PORT4
INPUT_PORTS_END

INPUT_PORTS_START( ddragon )
	COMMON_INPUT_PORTS

	COMMON_INPUT_DIP1

	PORT_START      /* DSW1 */
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x01, "Easy" )
	PORT_DIPSETTING(    0x03, "Medium" )
	PORT_DIPSETTING(    0x02, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x04, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x10, "20k" )
	PORT_DIPSETTING(    0x00, "40k" )
	PORT_DIPSETTING(    0x30, "30k and every 60k" )
	PORT_DIPSETTING(    0x20, "40k and every 80k" )
	PORT_DIPNAME( 0xc0, 0xc0, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0xc0, "2" )
	PORT_DIPSETTING(    0x80, "3" )
	PORT_DIPSETTING(    0x40, "4" )
	PORT_BITX( 0,       0x00, IPT_DIPSWITCH_SETTING | IPF_CHEAT, "Infinite", IP_KEY_NONE, IP_JOY_NONE )

	COMMON_PORT4
INPUT_PORTS_END

INPUT_PORTS_START( ddragon2 )
	COMMON_INPUT_PORTS

	COMMON_INPUT_DIP1

	PORT_START      /* DSW1 */
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x01, "Easy" )
	PORT_DIPSETTING(    0x03, "Medium" )
	PORT_DIPSETTING(    0x02, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x04, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, "Hurricane Kick" )
	PORT_DIPSETTING(    0x00, "Easy" )
	PORT_DIPSETTING(    0x08, "Normal" )
	PORT_DIPNAME( 0x30, 0x30, "Timer" )
	PORT_DIPSETTING(    0x00, "60" )
	PORT_DIPSETTING(    0x10, "65" )
	PORT_DIPSETTING(    0x30, "70" )
	PORT_DIPSETTING(    0x20, "80" )
	PORT_DIPNAME( 0xc0, 0xc0, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0xc0, "1" )
	PORT_DIPSETTING(    0x80, "2" )
	PORT_DIPSETTING(    0x40, "3" )
	PORT_DIPSETTING(    0x00, "4" )

	COMMON_PORT4
INPUT_PORTS_END

INPUT_PORTS_START( ddungeon )
	COMMON_INPUT_PORTS

	COMMON_INPUT_DIP2

	PORT_START      /* DSW1 */
	PORT_DIPNAME( 0x03, 0x01, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "1" )
	PORT_DIPSETTING(    0x01, "2" )
	PORT_DIPSETTING(    0x02, "3" )
	PORT_DIPSETTING(    0x03, "4" )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0xf0, 0x90, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0xf0, "Easy" )
	PORT_DIPSETTING(    0x90, "Medium" )
	PORT_DIPSETTING(    0x70, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )

	COMMON_PORT4
INPUT_PORTS_END

INPUT_PORTS_START( toffy )
	COMMON_INPUT_PORTS

	PORT_START
	PORT_DIPNAME( 0x0f, 0x00, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x07, DEF_STR( 4C_2C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 3C_2C ) )
	PORT_DIPSETTING(    0x05, DEF_STR( 2C_2C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x0b, DEF_STR( 4C_5C ) )
	PORT_DIPSETTING(    0x0f, "4 Coin/6 Credits" )
	PORT_DIPSETTING(    0x0a, "3 Coin/5 Credits" )
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x0e, "3 Coin/6 Credits" )
	PORT_DIPSETTING(    0x09, DEF_STR( 2C_5C ) )
	PORT_DIPSETTING(    0x0d, DEF_STR( 2C_6C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( 1C_6C ) )
	PORT_DIPNAME( 0xf0, 0x00, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x70, DEF_STR( 4C_2C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x60, DEF_STR( 3C_2C ) )
	PORT_DIPSETTING(    0x50, DEF_STR( 2C_2C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0xb0, DEF_STR( 4C_5C ) )
	PORT_DIPSETTING(    0xf0, "4 Coin/6 Credits" )
	PORT_DIPSETTING(    0xa0, "3 Coin/5 Credits" )
	PORT_DIPSETTING(    0x40, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0xe0, "3 Coin/6 Credits" )
	PORT_DIPSETTING(    0x90, DEF_STR( 2C_5C ) )
	PORT_DIPSETTING(    0xd0, DEF_STR( 2C_6C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 1C_6C ) )

	PORT_START
	PORT_DIPNAME( 0x03, 0x01, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "2" )
	PORT_DIPSETTING(    0x01, "3" )
	PORT_DIPSETTING(    0x02, "4" )
	PORT_DIPSETTING(    0x03, "5" )
	PORT_DIPNAME( 0x04, 0x00, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x04, DEF_STR( On ) )
	PORT_DIPNAME( 0x18, 0x08, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x10, "30k, 50k and 100k" )
	PORT_DIPSETTING(    0x08, "50k and 100k" )
	PORT_DIPSETTING(    0x18, "100k and 200k" )
	PORT_DIPSETTING(    0x00, "None" )
	PORT_DIPNAME( 0x20, 0x00, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x20, DEF_STR( On ) )
	PORT_DIPNAME( 0xc0, 0x80, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0xc0, "Easy" )
	PORT_DIPSETTING(    0x80, "Normal" )
	PORT_DIPSETTING(    0x40, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )

	COMMON_PORT4
INPUT_PORTS_END

#undef COMMON_INPUT_PORTS
#undef COMMON_INPUT_DIP2
#undef COMMON_INPUT_DIP1
#undef COMMON_PORT4

/*****************************************************************************/

static struct GfxLayout char_layout =
{
	8,8,
	RGN_FRAC(1,1),
	4,
	{ 0, 2, 4, 6 },
	{ 1, 0, 8*8+1, 8*8+0, 16*8+1, 16*8+0, 24*8+1, 24*8+0 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	32*8
};

static struct GfxLayout tile_layout =
{
	16,16,
	RGN_FRAC(1,2),
	4,
	{ RGN_FRAC(1,2)+0, RGN_FRAC(1,2)+4, 0, 4 },
	{ 3, 2, 1, 0, 16*8+3, 16*8+2, 16*8+1, 16*8+0,
		  32*8+3, 32*8+2, 32*8+1, 32*8+0, 48*8+3, 48*8+2, 48*8+1, 48*8+0 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
		  8*8, 9*8, 10*8, 11*8, 12*8, 13*8, 14*8, 15*8 },
	64*8
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &char_layout,   0, 8 },	/* colors   0-127 */
	{ REGION_GFX2, 0, &tile_layout, 128, 8 },	/* colors 128-255 */
	{ REGION_GFX3, 0, &tile_layout, 256, 8 },	/* colors 256-383 */
	{ -1 }
};

/*****************************************************************************/

static void irq_handler(int irq)
{
	cpu_set_irq_line( snd_cpu, ym_irq , irq ? ASSERT_LINE : CLEAR_LINE );
}

static struct YM2151interface ym2151_interface =
{
	1,			/* 1 chip */
	3579545,	/* ??? */
	{ YM3012_VOL(60,MIXER_PAN_LEFT,60,MIXER_PAN_RIGHT) },
	{ irq_handler }
};

static struct MSM5205interface msm5205_interface =
{
	2,					/* 2 chips             */
	384000,				/* 384KHz             */
	{ dd_adpcm_int, dd_adpcm_int },/* interrupt function */
	{ MSM5205_S48_4B, MSM5205_S48_4B },	/* 8kHz */
	{ 40, 40 }				/* volume */
};

static struct OKIM6295interface okim6295_interface =
{
	1,              /* 1 chip */
	{ 8000 },           /* frequency (Hz) */
	{ REGION_SOUND1 },  /* memory region */
	{ 15 }
};

static INTERRUPT_GEN( ddragon_interrupt )
{
	int scanline=271 - cpu_getiloops();

	/* VBLK is lowered on scanline 0 */
	if (scanline==0) {
		VBLK=0;
	}

	/* VBLK is raised on scanline 240 and NMI line is pulled high */
	if (scanline==240) {
		force_partial_update(scanline);
		cpu_set_nmi_line(0,ASSERT_LINE);
		VBLK=0x8;
	}

	/* IMS is triggered every time VPOS line 3 is raised, as VPOS counter starts at 16, effectively every 16 scanlines */
	if ((scanline%16)==7)
	{

		force_partial_update(scanline);
		cpu_set_irq_line(0,M6809_FIRQ_LINE,ASSERT_LINE);
	}
}

#define MAIN_CLOCK 12000000

#define MAIN_CPU MAIN_CLOCK / 4  
#define MCU_CPU  MAIN_CLOCK / 8  
#define SND_CPU  MAIN_CLOCK / 2 

static MACHINE_DRIVER_START( ddragon )

	/* basic machine hardware */
	MDRV_CPU_ADD(HD6309, MAIN_CPU * 2)
	MDRV_CPU_MEMORY(readmem,writemem)
	MDRV_CPU_VBLANK_INT(ddragon_interrupt,272)

	MDRV_CPU_ADD(HD63701, MCU_CPU ) /* This divider seems correct by comparison to real board */
	MDRV_CPU_MEMORY(sub_readmem,sub_writemem)

	MDRV_CPU_ADD(HD6309, SND_CPU)
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
	MDRV_CPU_MEMORY(sound_readmem,sound_writemem)

	MDRV_FRAMES_PER_SECOND(((12000000.0 / 256.0) / 3.0) / 272.0)
	MDRV_VBLANK_DURATION(0)
	MDRV_INTERLEAVE(100) /* heavy interleaving to sync up sprite<->main cpu's */

	MDRV_MACHINE_INIT(ddragon)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(1*8, 31*8-1, 2*8, 30*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(512)

	MDRV_VIDEO_START(ddragon)
	MDRV_VIDEO_UPDATE(ddragon)

	/* sound hardware */
	MDRV_SOUND_ADD(YM2151, ym2151_interface)
	MDRV_SOUND_ADD(MSM5205, msm5205_interface)

	MDRV_INSTALL_OST_SUPPORT(OST_SUPPORT_DDRAGON)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( darktowr )

	/* basic machine hardware */
 	MDRV_CPU_ADD(HD6309, 3579545)	/* 3.579545 MHz */
	MDRV_CPU_MEMORY(darktowr_readmem,darktowr_writemem)
	MDRV_CPU_VBLANK_INT(ddragon_interrupt,272)

 	MDRV_CPU_ADD(HD63701, 3579545 / 3)
	MDRV_CPU_MEMORY(sub_readmem,sub_writemem)

 	MDRV_CPU_ADD(HD6309, 3579545)
 	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)	/* ? */
	MDRV_CPU_MEMORY(sound_readmem,sound_writemem)

	MDRV_CPU_ADD(M68705,8000000/2)  /* ? MHz */
	MDRV_CPU_MEMORY(mcu_readmem,mcu_writemem)

	MDRV_FRAMES_PER_SECOND(((12000000.0 / 256.0) / 3.0) / 272.0)
	MDRV_VBLANK_DURATION(0)
	MDRV_INTERLEAVE(1000) /* heavy interleaving to sync up sprite<->main cpu's */

	MDRV_MACHINE_INIT(ddragon)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 1*8, 31*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(512)

	MDRV_VIDEO_START(ddragon)
	MDRV_VIDEO_UPDATE(ddragon)

	/* sound hardware */
	MDRV_SOUND_ADD(YM2151, ym2151_interface)
	MDRV_SOUND_ADD(MSM5205, msm5205_interface)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( ddragonb )

	/* basic machine hardware */
 	MDRV_CPU_ADD(HD6309, 3579545)	/* 3.579545 MHz */
	MDRV_CPU_MEMORY(readmem,writemem)
	MDRV_CPU_VBLANK_INT(ddragon_interrupt,272)

 	MDRV_CPU_ADD(HD6309, 12000000 / 3) /* 4 MHz */
	MDRV_CPU_MEMORY(sub_readmem,sub_writemem)

 	MDRV_CPU_ADD(HD6309, 3579545)
 	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)	/* ? */
	MDRV_CPU_MEMORY(sound_readmem,sound_writemem)

	MDRV_FRAMES_PER_SECOND(((12000000.0 / 256.0) / 3.0) / 272.0)
	MDRV_VBLANK_DURATION(0)
	MDRV_INTERLEAVE(100) /* heavy interleaving to sync up sprite<->main cpu's */

	MDRV_MACHINE_INIT(ddragonb)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(1*8, 31*8-1, 2*8, 30*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(384)

	MDRV_VIDEO_START(ddragon)
	MDRV_VIDEO_UPDATE(ddragon)

	/* sound hardware */
	MDRV_SOUND_ADD(YM2151, ym2151_interface)
	MDRV_SOUND_ADD(MSM5205, msm5205_interface)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( ddragon2 )

	/* basic machine hardware */
 	MDRV_CPU_ADD(HD6309, 3579545)	/* 3.579545 MHz */
	MDRV_CPU_MEMORY(dd2_readmem,dd2_writemem)
	MDRV_CPU_VBLANK_INT(ddragon_interrupt,272)

	MDRV_CPU_ADD(Z80, 12000000 / 3) /* 4 MHz */
	MDRV_CPU_MEMORY(dd2_sub_readmem,dd2_sub_writemem)

	MDRV_CPU_ADD(Z80, 3579545)
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)	/* 3.579545 MHz */
	MDRV_CPU_MEMORY(dd2_sound_readmem,dd2_sound_writemem)

	MDRV_FRAMES_PER_SECOND(((12000000.0 / 256.0) / 3.0) / 272.0)
	MDRV_VBLANK_DURATION(0)
	MDRV_INTERLEAVE(100) /* heavy interleaving to sync up sprite<->main cpu's */

	MDRV_MACHINE_INIT(ddragon2)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(1*8, 31*8-1, 2*8, 30*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(384)

	MDRV_VIDEO_START(ddragon)
	MDRV_VIDEO_UPDATE(ddragon)

	/* sound hardware */
	MDRV_SOUND_ADD(YM2151, ym2151_interface)
	MDRV_SOUND_ADD(OKIM6295, okim6295_interface)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( toffy )

	/* basic machine hardware */
	MDRV_CPU_ADD(M6809,3579545) /* 12 MHz / 2 or 3.579545 ?*/
	MDRV_CPU_MEMORY(readmem,toffy_writemem)
	MDRV_CPU_VBLANK_INT(ddragon_interrupt,272)

	MDRV_CPU_ADD(M6809, 3579545)
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
	MDRV_CPU_MEMORY(sound_readmem,sound_writemem)

	MDRV_FRAMES_PER_SECOND(((12000000.0 / 256.0) / 3.0) / 272.0)
	MDRV_VBLANK_DURATION(0)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER )
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(1*8, 31*8-1, 2*8, 30*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(384)

	MDRV_VIDEO_START(ddragon)
	MDRV_VIDEO_UPDATE(ddragon)

	MDRV_MACHINE_INIT(toffy)

	/* sound hardware */
	MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
	MDRV_SOUND_ADD(YM2151, ym2151_interface)
MACHINE_DRIVER_END

/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( ddragon )
	ROM_REGION( 0x28000, REGION_CPU1, 0 )	/* 64k for code + bankswitched memory */
	ROM_LOAD( "21j-1-5",      0x08000, 0x08000, CRC(42045dfd) SHA1(0983705ea3bb87c4c239692f400e02f15c243479) )
	ROM_LOAD( "21j-2-3",      0x10000, 0x08000, CRC(5779705e) SHA1(4b8f22225d10f5414253ce0383bbebd6f720f3af) ) /* banked at 0x4000-0x8000 */
	ROM_LOAD( "21j-3",        0x18000, 0x08000, CRC(3bdea613) SHA1(d9038c80646a6ce3ea61da222873237b0383680e) ) /* banked at 0x4000-0x8000 */
	ROM_LOAD( "21j-4-1",      0x20000, 0x08000, CRC(728f87b9) SHA1(d7442be24d41bb9fc021587ef44ae5b830e4503d) ) /* banked at 0x4000-0x8000 */

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* sprite cpu */
	ROM_LOAD( "63701.bin",    0xc000, 0x4000, CRC(f5232d03) SHA1(e2a194e38633592fd6587690b3cb2669d93985c7) )

	ROM_REGION( 0x10000, REGION_CPU3, 0 ) /* audio cpu */
	ROM_LOAD( "21j-0-1",      0x08000, 0x08000, CRC(9efa95bb) SHA1(da997d9cc7b9e7b2c70a4b6d30db693086a6f7d8) )

	ROM_REGION( 0x08000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "21j-5",        0x00000, 0x08000, CRC(7a8b8db4) SHA1(8368182234f9d4d763d4714fd7567a9e31b7ebeb) )	/* chars */

	ROM_REGION( 0x80000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "21j-a",        0x00000, 0x10000, CRC(574face3) SHA1(481fe574cb79d0159a65ff7486cbc945d50538c5) )	/* sprites */
	ROM_LOAD( "21j-b",        0x10000, 0x10000, CRC(40507a76) SHA1(74581a4b6f48100bddf20f319903af2fe36f39fa) )
	ROM_LOAD( "21j-c",        0x20000, 0x10000, CRC(bb0bc76f) SHA1(37b2225e0593335f636c1e5fded9b21fdeab2f5a) )
	ROM_LOAD( "21j-d",        0x30000, 0x10000, CRC(cb4f231b) SHA1(9f2270f9ceedfe51c5e9a9bbb00d6f43dbc4a3ea) )
	ROM_LOAD( "21j-e",        0x40000, 0x10000, CRC(a0a0c261) SHA1(25c534d82bd237386d447d72feee8d9541a5ded4) )
	ROM_LOAD( "21j-f",        0x50000, 0x10000, CRC(6ba152f6) SHA1(a301ff809be0e1471f4ff8305b30c2fa4aa57fae) )
	ROM_LOAD( "21j-g",        0x60000, 0x10000, CRC(3220a0b6) SHA1(24a16ea509e9aff82b9ddd14935d61bb71acff84) )
	ROM_LOAD( "21j-h",        0x70000, 0x10000, CRC(65c7517d) SHA1(f177ba9c1c7cc75ff04d5591b9865ee364788f94) )

	ROM_REGION( 0x40000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "21j-8",        0x00000, 0x10000, CRC(7c435887) SHA1(ecb76f2148fa9773426f05aac208eb3ac02747db) )	/* tiles */
	ROM_LOAD( "21j-9",        0x10000, 0x10000, CRC(c6640aed) SHA1(f156c337f48dfe4f7e9caee9a72c7ea3d53e3098) )
	ROM_LOAD( "21j-i",        0x20000, 0x10000, CRC(5effb0a0) SHA1(1f21acb15dad824e831ed9a42b3fde096bb31141) )
	ROM_LOAD( "21j-j",        0x30000, 0x10000, CRC(5fb42e7c) SHA1(7953316712c56c6f8ca6bba127319e24b618b646) )

	ROM_REGION( 0x20000, REGION_SOUND1, 0 ) /* adpcm samples */
	ROM_LOAD( "21j-6",        0x00000, 0x10000, CRC(34755de3) SHA1(57c06d6ce9497901072fa50a92b6ed0d2d4d6528) )
	ROM_LOAD( "21j-7",        0x10000, 0x10000, CRC(904de6f8) SHA1(3623e5ea05fd7c455992b7ed87e605b87c3850aa) )

	ROM_REGION( 0x0300, REGION_PROMS, 0 )
	ROM_LOAD( "21j-k-0",      0x0000, 0x0100, CRC(fdb130a9) SHA1(4c4f214229b9fab2b5d69c745ec5428787b89e1f) )	/* unknown */
	ROM_LOAD( "21j-l-0",      0x0100, 0x0200, CRC(46339529) SHA1(64f4c42a826d67b7cbaa8a23a45ebc4eb6248891) )	/* unknown */
ROM_END

ROM_START( ddragonw )
	ROM_REGION( 0x28000, REGION_CPU1, 0 )	/* 64k for code + bankswitched memory */
	ROM_LOAD( "21j-1",        0x08000, 0x08000, CRC(ae714964) SHA1(072522b97ca4edd099c6b48d7634354dc7088c53) )
	ROM_LOAD( "21j-2-3",      0x10000, 0x08000, CRC(5779705e) SHA1(4b8f22225d10f5414253ce0383bbebd6f720f3af) ) /* banked at 0x4000-0x8000 */
	ROM_LOAD( "21a-3",        0x18000, 0x08000, CRC(dbf24897) SHA1(1504faaf07c541330cd43b72dc6846911dfd85a3) ) /* banked at 0x4000-0x8000 */
	ROM_LOAD( "21j-4",        0x20000, 0x08000, CRC(6c9f46fa) SHA1(df251a4aea69b2328f7a543bf085b9c35933e2c1) ) /* banked at 0x4000-0x8000 */

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* sprite cpu */
	ROM_LOAD( "63701.bin",    0xc000, 0x4000, CRC(f5232d03) SHA1(e2a194e38633592fd6587690b3cb2669d93985c7) )

	ROM_REGION( 0x10000, REGION_CPU3, 0 ) /* audio cpu */
	ROM_LOAD( "21j-0-1",      0x08000, 0x08000, CRC(9efa95bb) SHA1(da997d9cc7b9e7b2c70a4b6d30db693086a6f7d8) )

	ROM_REGION( 0x08000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "21j-5",        0x00000, 0x08000, CRC(7a8b8db4) SHA1(8368182234f9d4d763d4714fd7567a9e31b7ebeb) )	/* chars */

	ROM_REGION( 0x80000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "21j-a",        0x00000, 0x10000, CRC(574face3) SHA1(481fe574cb79d0159a65ff7486cbc945d50538c5) )	/* sprites */
	ROM_LOAD( "21j-b",        0x10000, 0x10000, CRC(40507a76) SHA1(74581a4b6f48100bddf20f319903af2fe36f39fa) )
	ROM_LOAD( "21j-c",        0x20000, 0x10000, CRC(bb0bc76f) SHA1(37b2225e0593335f636c1e5fded9b21fdeab2f5a) )
	ROM_LOAD( "21j-d",        0x30000, 0x10000, CRC(cb4f231b) SHA1(9f2270f9ceedfe51c5e9a9bbb00d6f43dbc4a3ea) )
	ROM_LOAD( "21j-e",        0x40000, 0x10000, CRC(a0a0c261) SHA1(25c534d82bd237386d447d72feee8d9541a5ded4) )
	ROM_LOAD( "21j-f",        0x50000, 0x10000, CRC(6ba152f6) SHA1(a301ff809be0e1471f4ff8305b30c2fa4aa57fae) )
	ROM_LOAD( "21j-g",        0x60000, 0x10000, CRC(3220a0b6) SHA1(24a16ea509e9aff82b9ddd14935d61bb71acff84) )
	ROM_LOAD( "21j-h",        0x70000, 0x10000, CRC(65c7517d) SHA1(f177ba9c1c7cc75ff04d5591b9865ee364788f94) )

	ROM_REGION( 0x40000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "21j-8",        0x00000, 0x10000, CRC(7c435887) SHA1(ecb76f2148fa9773426f05aac208eb3ac02747db) )	/* tiles */
	ROM_LOAD( "21j-9",        0x10000, 0x10000, CRC(c6640aed) SHA1(f156c337f48dfe4f7e9caee9a72c7ea3d53e3098) )
	ROM_LOAD( "21j-i",        0x20000, 0x10000, CRC(5effb0a0) SHA1(1f21acb15dad824e831ed9a42b3fde096bb31141) )
	ROM_LOAD( "21j-j",        0x30000, 0x10000, CRC(5fb42e7c) SHA1(7953316712c56c6f8ca6bba127319e24b618b646) )

	ROM_REGION( 0x20000, REGION_SOUND1, 0 ) /* adpcm samples */
	ROM_LOAD( "21j-6",        0x00000, 0x10000, CRC(34755de3) SHA1(57c06d6ce9497901072fa50a92b6ed0d2d4d6528) )
	ROM_LOAD( "21j-7",        0x10000, 0x10000, CRC(904de6f8) SHA1(3623e5ea05fd7c455992b7ed87e605b87c3850aa) )

	ROM_REGION( 0x0300, REGION_PROMS, 0 )
	ROM_LOAD( "21j-k-0",      0x0000, 0x0100, CRC(fdb130a9) SHA1(4c4f214229b9fab2b5d69c745ec5428787b89e1f) )	/* unknown */
	ROM_LOAD( "21j-l-0",      0x0100, 0x0200, CRC(46339529) SHA1(64f4c42a826d67b7cbaa8a23a45ebc4eb6248891) )	/* unknown */
ROM_END

ROM_START( ddragonu )
	ROM_REGION( 0x28000, REGION_CPU1, 0 )	/* 64k for code + bankswitched memory */
	ROM_LOAD( "21a-1-5",      0x08000, 0x08000, CRC(e24a6e11) SHA1(9dd97dd712d5c896f91fd80df58be9b8a2b198ee) )
	ROM_LOAD( "21j-2-3",      0x10000, 0x08000, CRC(5779705e) SHA1(4b8f22225d10f5414253ce0383bbebd6f720f3af) ) /* banked at 0x4000-0x8000 */
	ROM_LOAD( "21a-3",        0x18000, 0x08000, CRC(dbf24897) SHA1(1504faaf07c541330cd43b72dc6846911dfd85a3) ) /* banked at 0x4000-0x8000 */
	ROM_LOAD( "21a-4",        0x20000, 0x08000, CRC(6ea16072) SHA1(0b3b84a0d54f7a3aba411586009babbfee653f9a) ) /* banked at 0x4000-0x8000 */

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* sprite cpu */
	ROM_LOAD( "63701.bin",    0xc000, 0x4000, CRC(f5232d03) SHA1(e2a194e38633592fd6587690b3cb2669d93985c7) )

	ROM_REGION( 0x10000, REGION_CPU3, 0 ) /* audio cpu */
	ROM_LOAD( "21j-0-1",      0x08000, 0x08000, CRC(9efa95bb) SHA1(da997d9cc7b9e7b2c70a4b6d30db693086a6f7d8) )

	ROM_REGION( 0x08000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "21j-5",        0x00000, 0x08000, CRC(7a8b8db4) SHA1(8368182234f9d4d763d4714fd7567a9e31b7ebeb) )	/* chars */

	ROM_REGION( 0x80000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "21j-a",        0x00000, 0x10000, CRC(574face3) SHA1(481fe574cb79d0159a65ff7486cbc945d50538c5) )	/* sprites */
	ROM_LOAD( "21j-b",        0x10000, 0x10000, CRC(40507a76) SHA1(74581a4b6f48100bddf20f319903af2fe36f39fa) )
	ROM_LOAD( "21j-c",        0x20000, 0x10000, CRC(bb0bc76f) SHA1(37b2225e0593335f636c1e5fded9b21fdeab2f5a) )
	ROM_LOAD( "21j-d",        0x30000, 0x10000, CRC(cb4f231b) SHA1(9f2270f9ceedfe51c5e9a9bbb00d6f43dbc4a3ea) )
	ROM_LOAD( "21j-e",        0x40000, 0x10000, CRC(a0a0c261) SHA1(25c534d82bd237386d447d72feee8d9541a5ded4) )
	ROM_LOAD( "21j-f",        0x50000, 0x10000, CRC(6ba152f6) SHA1(a301ff809be0e1471f4ff8305b30c2fa4aa57fae) )
	ROM_LOAD( "21j-g",        0x60000, 0x10000, CRC(3220a0b6) SHA1(24a16ea509e9aff82b9ddd14935d61bb71acff84) )
	ROM_LOAD( "21j-h",        0x70000, 0x10000, CRC(65c7517d) SHA1(f177ba9c1c7cc75ff04d5591b9865ee364788f94) )

	ROM_REGION( 0x40000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "21j-8",        0x00000, 0x10000, CRC(7c435887) SHA1(ecb76f2148fa9773426f05aac208eb3ac02747db) )	/* tiles */
	ROM_LOAD( "21j-9",        0x10000, 0x10000, CRC(c6640aed) SHA1(f156c337f48dfe4f7e9caee9a72c7ea3d53e3098) )
	ROM_LOAD( "21j-i",        0x20000, 0x10000, CRC(5effb0a0) SHA1(1f21acb15dad824e831ed9a42b3fde096bb31141) )
	ROM_LOAD( "21j-j",        0x30000, 0x10000, CRC(5fb42e7c) SHA1(7953316712c56c6f8ca6bba127319e24b618b646) )

	ROM_REGION( 0x20000, REGION_SOUND1, 0 ) /* adpcm samples */
	ROM_LOAD( "21j-6",        0x00000, 0x10000, CRC(34755de3) SHA1(57c06d6ce9497901072fa50a92b6ed0d2d4d6528) )
	ROM_LOAD( "21j-7",        0x10000, 0x10000, CRC(904de6f8) SHA1(3623e5ea05fd7c455992b7ed87e605b87c3850aa) )

	ROM_REGION( 0x0300, REGION_PROMS, 0 )
	ROM_LOAD( "21j-k-0",      0x0000, 0x0100, CRC(fdb130a9) SHA1(4c4f214229b9fab2b5d69c745ec5428787b89e1f) )	/* unknown */
	ROM_LOAD( "21j-l-0",      0x0100, 0x0200, CRC(46339529) SHA1(64f4c42a826d67b7cbaa8a23a45ebc4eb6248891) )	/* unknown */
ROM_END

ROM_START( ddragonb )
	ROM_REGION( 0x28000, REGION_CPU1, 0 )	/* 64k for code + bankswitched memory */
	ROM_LOAD( "ic26",         0x08000, 0x08000, CRC(ae714964) SHA1(072522b97ca4edd099c6b48d7634354dc7088c53) )
	ROM_LOAD( "21j-2-3",      0x10000, 0x08000, CRC(5779705e) SHA1(4b8f22225d10f5414253ce0383bbebd6f720f3af) ) /* banked at 0x4000-0x8000 */
	ROM_LOAD( "21a-3",        0x18000, 0x08000, CRC(dbf24897) SHA1(1504faaf07c541330cd43b72dc6846911dfd85a3) ) /* banked at 0x4000-0x8000 */
	ROM_LOAD( "ic23",         0x20000, 0x08000, CRC(6c9f46fa) SHA1(df251a4aea69b2328f7a543bf085b9c35933e2c1) ) /* banked at 0x4000-0x8000 */

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* sprite cpu */
	ROM_LOAD( "ic38",         0x0c000, 0x04000, CRC(6a6a0325) SHA1(98a940a9f23ce9154ff94f7f2ce29efe9a92f71b) )

	ROM_REGION( 0x10000, REGION_CPU3, 0 ) /* audio cpu */
	ROM_LOAD( "21j-0-1",      0x08000, 0x08000, CRC(9efa95bb) SHA1(da997d9cc7b9e7b2c70a4b6d30db693086a6f7d8) )

	ROM_REGION( 0x08000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "21j-5",        0x00000, 0x08000, CRC(7a8b8db4) SHA1(8368182234f9d4d763d4714fd7567a9e31b7ebeb) )	/* chars */

	ROM_REGION( 0x80000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "21j-a",        0x00000, 0x10000, CRC(574face3) SHA1(481fe574cb79d0159a65ff7486cbc945d50538c5) )	/* sprites */
	ROM_LOAD( "21j-b",        0x10000, 0x10000, CRC(40507a76) SHA1(74581a4b6f48100bddf20f319903af2fe36f39fa) )
	ROM_LOAD( "21j-c",        0x20000, 0x10000, CRC(bb0bc76f) SHA1(37b2225e0593335f636c1e5fded9b21fdeab2f5a) )
	ROM_LOAD( "21j-d",        0x30000, 0x10000, CRC(cb4f231b) SHA1(9f2270f9ceedfe51c5e9a9bbb00d6f43dbc4a3ea) )
	ROM_LOAD( "21j-e",        0x40000, 0x10000, CRC(a0a0c261) SHA1(25c534d82bd237386d447d72feee8d9541a5ded4) )
	ROM_LOAD( "21j-f",        0x50000, 0x10000, CRC(6ba152f6) SHA1(a301ff809be0e1471f4ff8305b30c2fa4aa57fae) )
	ROM_LOAD( "21j-g",        0x60000, 0x10000, CRC(3220a0b6) SHA1(24a16ea509e9aff82b9ddd14935d61bb71acff84) )
	ROM_LOAD( "21j-h",        0x70000, 0x10000, CRC(65c7517d) SHA1(f177ba9c1c7cc75ff04d5591b9865ee364788f94) )

	ROM_REGION( 0x40000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "21j-8",        0x00000, 0x10000, CRC(7c435887) SHA1(ecb76f2148fa9773426f05aac208eb3ac02747db) )	/* tiles */
	ROM_LOAD( "21j-9",        0x10000, 0x10000, CRC(c6640aed) SHA1(f156c337f48dfe4f7e9caee9a72c7ea3d53e3098) )
	ROM_LOAD( "21j-i",        0x20000, 0x10000, CRC(5effb0a0) SHA1(1f21acb15dad824e831ed9a42b3fde096bb31141) )
	ROM_LOAD( "21j-j",        0x30000, 0x10000, CRC(5fb42e7c) SHA1(7953316712c56c6f8ca6bba127319e24b618b646) )

	ROM_REGION( 0x20000, REGION_SOUND1, 0 ) /* adpcm samples */
	ROM_LOAD( "21j-6",        0x00000, 0x10000, CRC(34755de3) SHA1(57c06d6ce9497901072fa50a92b6ed0d2d4d6528) )
	ROM_LOAD( "21j-7",        0x10000, 0x10000, CRC(904de6f8) SHA1(3623e5ea05fd7c455992b7ed87e605b87c3850aa) )

	ROM_REGION( 0x0300, REGION_PROMS, 0 )
	ROM_LOAD( "21j-k-0",      0x0000, 0x0100, CRC(fdb130a9) SHA1(4c4f214229b9fab2b5d69c745ec5428787b89e1f) )	/* unknown */
	ROM_LOAD( "21j-l-0",      0x0100, 0x0200, CRC(46339529) SHA1(64f4c42a826d67b7cbaa8a23a45ebc4eb6248891) )	/* unknown */
ROM_END

ROM_START( ddragon2 )
	ROM_REGION( 0x28000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "26a9-04.bin",  0x08000, 0x8000, CRC(f2cfc649) SHA1(d3f1e0bae02472914a940222e4f600170a91736d) )
	ROM_LOAD( "26aa-03.bin",  0x10000, 0x8000, CRC(44dd5d4b) SHA1(427c4e419668b41545928cfc96435c010ecdc88b) )
	ROM_LOAD( "26ab-0.bin",   0x18000, 0x8000, CRC(49ddddcd) SHA1(91dc53718d04718b313f23d86e241027c89d1a03) )
	ROM_LOAD( "26ac-0e.63",   0x20000, 0x8000, CRC(57acad2c) SHA1(938e2a78af38ecd7e9e08fb10acc1940f7585f5e) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* sprite CPU 64kb (Upper 16kb = 0) */
	ROM_LOAD( "26ae-0.bin",   0x00000, 0x10000, CRC(ea437867) SHA1(cd910203af0565f981b9bdef51ea6e9c33ee82d3) )

	ROM_REGION( 0x10000, REGION_CPU3, 0 ) /* music CPU, 64kb */
	ROM_LOAD( "26ad-0.bin",   0x00000, 0x8000, CRC(75e36cd6) SHA1(f24805f4f6925b3ac508e66a6fc25c275b05f3b9) )

	ROM_REGION( 0x10000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "26a8-0e.19",   0x00000, 0x10000, CRC(4e80cd36) SHA1(dcae0709f27f32effb359f6b943f61b102749f2a) )	/* chars */

	ROM_REGION( 0xc0000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "26j0-0.bin",   0x00000, 0x20000, CRC(db309c84) SHA1(ee095e4a3bc86737539784945decb1f63da47b9b) )	/* sprites */
	ROM_LOAD( "26j1-0.bin",   0x20000, 0x20000, CRC(c3081e0c) SHA1(c4a9ae151aae21073a2c79c5ac088c72d4f3d9db) )
	ROM_LOAD( "26af-0.bin",   0x40000, 0x20000, CRC(3a615aad) SHA1(ec90a35224a177d00327de6fd1a299df38abd790) )
	ROM_LOAD( "26j2-0.bin",   0x60000, 0x20000, CRC(589564ae) SHA1(1e6e0ef623545615e8409b6d3ba586a71e2612b6) )
	ROM_LOAD( "26j3-0.bin",   0x80000, 0x20000, CRC(daf040d6) SHA1(ab0fd5482625dbe64f0f0b0baff5dcde05309b81) )
	ROM_LOAD( "26a10-0.bin",  0xa0000, 0x20000, CRC(6d16d889) SHA1(3bc62b3e7f4ddc3200a9cf8469239662da80c854) )

	ROM_REGION( 0x40000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "26j4-0.bin",   0x00000, 0x20000, CRC(a8c93e76) SHA1(54d64f052971e7fa0d21c5ce12f87b0fa2b648d6) )	/* tiles */
	ROM_LOAD( "26j5-0.bin",   0x20000, 0x20000, CRC(ee555237) SHA1(f9698f3e57f933a43e508f60667c860dee034d05) )

	ROM_REGION( 0x40000, REGION_SOUND1, 0 ) /* adpcm samples */
	ROM_LOAD( "26j6-0.bin",   0x00000, 0x20000, CRC(a84b2a29) SHA1(9cb529e4939c16a0a42f45dd5547c76c2f86f07b) )
	ROM_LOAD( "26j7-0.bin",   0x20000, 0x20000, CRC(bc6a48d5) SHA1(04c434f8cd42a8f82a263548183569396f9b684d) )

	ROM_REGION( 0x0200, REGION_PROMS, 0 )
	ROM_LOAD( "prom.16",      0x0000, 0x0200, CRC(46339529) SHA1(64f4c42a826d67b7cbaa8a23a45ebc4eb6248891) )	/* unknown (same as ddragon) */
ROM_END

ROM_START( ddragn2u )
	ROM_REGION( 0x28000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "26a9-04.bin",  0x08000, 0x8000, CRC(f2cfc649) SHA1(d3f1e0bae02472914a940222e4f600170a91736d) )
	ROM_LOAD( "26aa-03.bin",  0x10000, 0x8000, CRC(44dd5d4b) SHA1(427c4e419668b41545928cfc96435c010ecdc88b) )
	ROM_LOAD( "26ab-0.bin",   0x18000, 0x8000, CRC(49ddddcd) SHA1(91dc53718d04718b313f23d86e241027c89d1a03) )
	ROM_LOAD( "26ac-02.bin",  0x20000, 0x8000, CRC(097eaf26) SHA1(60504abd30fec44c45197cdf3832c87d05ef577d) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* sprite CPU 64kb (Upper 16kb = 0) */
	ROM_LOAD( "26ae-0.bin",   0x00000, 0x10000, CRC(ea437867) SHA1(cd910203af0565f981b9bdef51ea6e9c33ee82d3) )

	ROM_REGION( 0x10000, REGION_CPU3, 0 ) /* music CPU, 64kb */
	ROM_LOAD( "26ad-0.bin",   0x00000, 0x8000, CRC(75e36cd6) SHA1(f24805f4f6925b3ac508e66a6fc25c275b05f3b9) )

	ROM_REGION( 0x10000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "26a8-0.bin",   0x00000, 0x10000, CRC(3ad1049c) SHA1(11d9544a56f8e6a84beb307a5c8a9ff8afc55c66) )	/* chars */

	ROM_REGION( 0xc0000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "26j0-0.bin",   0x00000, 0x20000, CRC(db309c84) SHA1(ee095e4a3bc86737539784945decb1f63da47b9b) )	/* sprites */
	ROM_LOAD( "26j1-0.bin",   0x20000, 0x20000, CRC(c3081e0c) SHA1(c4a9ae151aae21073a2c79c5ac088c72d4f3d9db) )
	ROM_LOAD( "26af-0.bin",   0x40000, 0x20000, CRC(3a615aad) SHA1(ec90a35224a177d00327de6fd1a299df38abd790) )
	ROM_LOAD( "26j2-0.bin",   0x60000, 0x20000, CRC(589564ae) SHA1(1e6e0ef623545615e8409b6d3ba586a71e2612b6) )
	ROM_LOAD( "26j3-0.bin",   0x80000, 0x20000, CRC(daf040d6) SHA1(ab0fd5482625dbe64f0f0b0baff5dcde05309b81) )
	ROM_LOAD( "26a10-0.bin",  0xa0000, 0x20000, CRC(6d16d889) SHA1(3bc62b3e7f4ddc3200a9cf8469239662da80c854) )

	ROM_REGION( 0x40000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "26j4-0.bin",   0x00000, 0x20000, CRC(a8c93e76) SHA1(54d64f052971e7fa0d21c5ce12f87b0fa2b648d6) )	/* tiles */
	ROM_LOAD( "26j5-0.bin",   0x20000, 0x20000, CRC(ee555237) SHA1(f9698f3e57f933a43e508f60667c860dee034d05) )

	ROM_REGION( 0x40000, REGION_SOUND1, 0 ) /* adpcm samples */
	ROM_LOAD( "26j6-0.bin",   0x00000, 0x20000, CRC(a84b2a29) SHA1(9cb529e4939c16a0a42f45dd5547c76c2f86f07b) )
	ROM_LOAD( "26j7-0.bin",   0x20000, 0x20000, CRC(bc6a48d5) SHA1(04c434f8cd42a8f82a263548183569396f9b684d) )

	ROM_REGION( 0x0200, REGION_PROMS, 0 )
	ROM_LOAD( "prom.16",      0x0000, 0x0200, CRC(46339529) SHA1(64f4c42a826d67b7cbaa8a23a45ebc4eb6248891) )	/* unknown (same as ddragon) */
ROM_END

ROM_START( toffy )
	ROM_REGION( 0x28000, REGION_CPU1, 0 ) /* Main CPU? */
	ROM_LOAD( "2-27512.rom", 0x00000, 0x10000, CRC(244709dd) SHA1(b2db51b910f1a031b94fb50e684351f657a465dc) )
	ROM_RELOAD( 0x10000, 0x10000 )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* Sound CPU? */
	ROM_LOAD( "u142.1", 0x00000, 0x10000, CRC(541bd7f0) SHA1(3f0097f5877eae50651f94d46d7dd9127037eb6e) )

	ROM_REGION( 0x10000, REGION_GFX1, 0 ) /* GFX? */
	ROM_LOAD( "7-27512.rom", 0x000, 0x10000, CRC(f9e8ec64) SHA1(36891cd8f28800e03fe0eac84b2484a70011eabb) )

	ROM_REGION( 0x20000, REGION_GFX3, 0 ) /* GFX */
	/* the same as 'Dangerous Dungeons' once decrypted */
	ROM_LOAD( "4-27512.rom", 0x00000, 0x10000, CRC(94b5ef6f) SHA1(32967f6cfc6a077c31923318891ed508f83e67f6) )
	ROM_LOAD( "3-27512.rom", 0x10000, 0x10000, CRC(a7a053a3) SHA1(98625fe73a409c8d51136931a5f707a0bf75b66a) )

	ROM_REGION( 0x20000, REGION_GFX2, 0 ) /* GFX */
	ROM_LOAD( "6-27512.rom", 0x00000, 0x10000, CRC(2ba7ca47) SHA1(ad709fc871f1f1a7d4b0fdf0f516c53fd4c8b685) )
	ROM_LOAD( "5-27512.rom", 0x10000, 0x10000, CRC(4f91eec6) SHA1(18a5f98dfba33837b73d032a6153eeb03263684b) )
ROM_END

ROM_START( stoffy )
	ROM_REGION( 0x28000, REGION_CPU1, 0 ) /* Main CPU? */
	ROM_LOAD( "u70.2", 0x00000, 0x10000, CRC(3c156610) SHA1(d7fdbc595bdc77c452da39da8b20774db0952e33) )
	ROM_RELOAD( 0x10000, 0x10000 )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* Sound CPU? */
	ROM_LOAD( "u142.1", 0x00000, 0x10000, CRC(541bd7f0) SHA1(3f0097f5877eae50651f94d46d7dd9127037eb6e) ) /* same as 'toffy'*/

	ROM_REGION( 0x10000, REGION_GFX1, 0 ) /* GFX? */
	ROM_LOAD( "u35.7", 0x00000, 0x10000, CRC(83735d25) SHA1(d82c046db0112d7d2877339652b2111f12513a4f) )

	ROM_REGION( 0x20000, REGION_GFX3, 0 ) /* GFX */
	ROM_LOAD( "u78.4", 0x00000, 0x10000, CRC(9743a74d) SHA1(876696c5e88e58e6e44671c33a4c140be02a941e) ) /* 0*/
	ROM_LOAD( "u77.3", 0x10000, 0x10000, CRC(f267109a) SHA1(679d2147c79636796dda850345c04ad8a9daa6af) ) /* 0*/

	ROM_REGION( 0x20000, REGION_GFX2, 0 ) /* GFX */
	ROM_LOAD( "u80.5", 0x00000, 0x10000, CRC(ff190865) SHA1(245e69651d0161fcb416bba8f743602b4ee83139) ) /* 1 | should be u80.6 ?*/
	ROM_LOAD( "u79.5", 0x10000, 0x10000, CRC(333d5b8a) SHA1(d3573db87e2318c144ee9ace6c975a70fc96f4c4) ) /* 1*/
ROM_END

ROM_START( ddungeon )
	ROM_REGION( 0x28000, REGION_CPU1, 0 ) /* Main CPU? */
	ROM_LOAD( "dd3.bin", 0x10000, 0x8000, CRC(922e719c) SHA1(d1c73f56913cd368158abc613d7bbab669509742) )
	ROM_LOAD( "dd2.bin", 0x08000, 0x8000, CRC(a6e7f608) SHA1(83b9301c39bfdc1e50a37f2bdc4d4f65a1111bee) )
	/* IC23 is replaced with a daughterboard containing a 68705 MCU */

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* sprite cpu */
	ROM_LOAD( "63701.bin",    0xc000, 0x4000, CRC(f5232d03) SHA1(e2a194e38633592fd6587690b3cb2669d93985c7) )

	ROM_REGION( 0x10000, REGION_CPU3, 0 ) /* audio cpu */
	ROM_LOAD( "21j-0-1",      0x08000, 0x08000, CRC(9efa95bb) SHA1(da997d9cc7b9e7b2c70a4b6d30db693086a6f7d8) ) /* from ddragon */

	ROM_REGION( 0x0800, REGION_CPU4, 0 )	/* 8k for the microcontroller */
	ROM_LOAD( "dd_mcu.bin",   0x00000, 0x0800, CRC(34cbb2d3) SHA1(8e0c3b13c636012d88753d547c639b1a8af85680) )

	ROM_REGION( 0x10000, REGION_GFX1, 0 ) /* GFX? */
	ROM_LOAD( "dd6.bin", 0x00000, 0x08000, CRC(057588ca) SHA1(d4a5dd3ea8cf455b54657473d4d52ab5e838ae15) )

	ROM_REGION( 0x20000, REGION_GFX2, 0 ) /* GFX */
	ROM_LOAD( "dd-7r.bin", 0x00000, 0x08000, CRC(50d6ab5d) SHA1(4c9cbd72d38b631ea2ca231045ef3f3e11cc7c07) ) /* 1*/
	ROM_LOAD( "dd-7k.bin", 0x10000, 0x08000, CRC(43264ad8) SHA1(74f031d6179390bc4fa99f4929a6886db8c2b510) ) /* 1*/

	ROM_REGION( 0x20000, REGION_GFX3, 0 ) /* GFX */
	ROM_LOAD( "dd-6b.bin", 0x00000, 0x08000, CRC(3deacae9) SHA1(6663f054ed3eed50c5cacfa5d22d465dfb179964) ) /* 0*/
	ROM_LOAD( "dd-7c.bin", 0x10000, 0x08000, CRC(5a2f31eb) SHA1(1b85533443e148adb2a9c2c09c43cbf2c35c86bc) ) /* 0*/

	ROM_REGION( 0x20000, REGION_SOUND1, 0 ) /* adpcm samples */
	ROM_LOAD( "21j-6",        0x00000, 0x10000, CRC(34755de3) SHA1(57c06d6ce9497901072fa50a92b6ed0d2d4d6528) )
	ROM_LOAD( "21j-7",        0x10000, 0x10000, CRC(904de6f8) SHA1(3623e5ea05fd7c455992b7ed87e605b87c3850aa) )

	ROM_REGION( 0x0300, REGION_PROMS, 0 )
	ROM_LOAD( "21j-k-0",      0x0000, 0x0100, CRC(fdb130a9) SHA1(4c4f214229b9fab2b5d69c745ec5428787b89e1f) )	/* unknown */
	ROM_LOAD( "21j-l-0",      0x0100, 0x0200, CRC(46339529) SHA1(64f4c42a826d67b7cbaa8a23a45ebc4eb6248891) )	/* unknown */
ROM_END

ROM_START( darktowr )
	ROM_REGION( 0x28000, REGION_CPU1, 0 )	/* 64k for code + bankswitched memory */
	ROM_LOAD( "dt.26",         0x08000, 0x08000, CRC(8134a472) SHA1(7d42d2ed8d09855241d98ed94bce140a314c2f66) )
	ROM_LOAD( "21j-2-3.25",    0x10000, 0x08000, CRC(5779705e) SHA1(4b8f22225d10f5414253ce0383bbebd6f720f3af) ) /* from ddragon */
	ROM_LOAD( "dt.24",         0x18000, 0x08000, CRC(523a5413) SHA1(71c04287e4f2e792c98abdeb97fe70abd0d5e918) ) /* banked at 0x4000-0x8000 */
	/* IC23 is replaced with a daughterboard containing a 68705 MCU */

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* sprite cpu */
	ROM_LOAD( "63701.bin",    0xc000, 0x4000, CRC(f5232d03) SHA1(e2a194e38633592fd6587690b3cb2669d93985c7) )

	ROM_REGION( 0x10000, REGION_CPU3, 0 ) /* audio cpu */
	ROM_LOAD( "21j-0-1",      0x08000, 0x08000, CRC(9efa95bb) SHA1(da997d9cc7b9e7b2c70a4b6d30db693086a6f7d8) ) /* from ddragon */

	ROM_REGION( 0x0800, REGION_CPU4, 0 )	/* 8k for the microcontroller */
	ROM_LOAD( "68705prt.mcu",   0x00000, 0x0800, CRC(34cbb2d3) SHA1(8e0c3b13c636012d88753d547c639b1a8af85680) )

	ROM_REGION( 0x08000, REGION_GFX1, ROMREGION_DISPOSE ) /* chars */
	ROM_LOAD( "dt.20",        0x00000, 0x08000, CRC(860b0298) SHA1(087e4e6511c5bed74ffbfd077ece55a756b13253) )

	ROM_REGION( 0x80000, REGION_GFX2, ROMREGION_DISPOSE ) /* sprites */
	ROM_LOAD( "dt.117",       0x00000, 0x10000, CRC(750dd0fa) SHA1(d95b95a54c7ed87a27edb8660810dd89efa10c9f) )
	ROM_LOAD( "dt.116",       0x10000, 0x10000, CRC(22cfa87b) SHA1(0008a41f307be96be91f491bdeaa1fa450dd0fdf) )
	ROM_LOAD( "dt.115",       0x20000, 0x10000, CRC(8a9f1c34) SHA1(1f07f424b2ab14a051f2c84b3d89fc5d35c5f20b) )
	ROM_LOAD( "21j-d",        0x30000, 0x10000, CRC(cb4f231b) SHA1(9f2270f9ceedfe51c5e9a9bbb00d6f43dbc4a3ea) ) /* from ddragon */
	ROM_LOAD( "dt.113",       0x40000, 0x10000, CRC(7b4bbf9c) SHA1(d0caa3c38e059d3ee48e3e801da36f67457ed542) )
	ROM_LOAD( "dt.112",       0x50000, 0x10000, CRC(df3709d4) SHA1(9cca44be97260e730786db8244a0d655c86537aa) )
	ROM_LOAD( "dt.111",       0x60000, 0x10000, CRC(59032154) SHA1(637372e4619472a958f4971b50a6fe0985bffc8b) )
	ROM_LOAD( "21j-h",        0x70000, 0x10000, CRC(65c7517d) SHA1(f177ba9c1c7cc75ff04d5591b9865ee364788f94) ) /* from ddragon */

	ROM_REGION( 0x40000, REGION_GFX3, ROMREGION_DISPOSE ) /* tiles */
	ROM_LOAD( "dt.78",        0x00000, 0x10000, CRC(72c15604) SHA1(202b46a2445eea5877e986a871bb0a6b76b88a6f) )
	ROM_LOAD( "21j-9",        0x10000, 0x10000, CRC(c6640aed) SHA1(f156c337f48dfe4f7e9caee9a72c7ea3d53e3098) ) /* from ddragon */
	ROM_LOAD( "dt.109",       0x20000, 0x10000, CRC(15bdcb62) SHA1(75382a3805dc333b196e119d28b5c3f320bd9f2a) )
	ROM_LOAD( "21j-j",        0x30000, 0x10000, CRC(5fb42e7c) SHA1(7953316712c56c6f8ca6bba127319e24b618b646) ) /* from ddragon */

	ROM_REGION( 0x20000, REGION_SOUND1, 0 ) /* adpcm samples */
	ROM_LOAD( "21j-6",        0x00000, 0x10000, CRC(34755de3) SHA1(57c06d6ce9497901072fa50a92b6ed0d2d4d6528) ) /* from ddragon */
	ROM_LOAD( "21j-7",        0x10000, 0x10000, CRC(904de6f8) SHA1(3623e5ea05fd7c455992b7ed87e605b87c3850aa) ) /* from ddragon */

	ROM_REGION( 0x0300, REGION_PROMS, 0 )
	ROM_LOAD( "21j-k-0",      0x0000, 0x0100, CRC(fdb130a9) SHA1(4c4f214229b9fab2b5d69c745ec5428787b89e1f) )	/* unknown */ /* from ddragon */
	ROM_LOAD( "21j-l-0",      0x0100, 0x0200, CRC(46339529) SHA1(64f4c42a826d67b7cbaa8a23a45ebc4eb6248891) )	/* unknown */ /* from ddragon */
ROM_END

ROM_START( tstrike )
	ROM_REGION( 0x28000, REGION_CPU1, 0 )	/* 64k for code + bankswitched memory */
	ROM_LOAD( "tstrike.26",      0x08000, 0x08000, CRC(871b10bc) SHA1(c824775cf72c039612fda76c4a518cd89e4c8657) )
	ROM_LOAD( "tstrike.25",      0x10000, 0x08000, CRC(b6a0c2f3) SHA1(3434689ca217f5af268058ad34c277db672d389c) ) /* banked at 0x4000-0x8000 */
	ROM_LOAD( "tstrike.24",      0x18000, 0x08000, CRC(363816fa) SHA1(65c1ccbb950e09230196b49dc7312a13a34f3f79) ) /* banked at 0x4000-0x8000 */
	/* IC23 is replaced with a daughterboard containing a 68705 MCU */

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* sprite cpu */
	ROM_LOAD( "63701.bin",    0xc000, 0x4000, CRC(f5232d03) SHA1(e2a194e38633592fd6587690b3cb2669d93985c7) )

	ROM_REGION( 0x10000, REGION_CPU3, 0 ) /* audio cpu */
	ROM_LOAD( "tstrike.30",      0x08000, 0x08000, CRC(3f3f04a1) SHA1(45d2b4542ec783c1c4122616606be6c160f76c06) )

	ROM_REGION( 0x0800, REGION_CPU4, 0 )	/* 8k for the microcontroller */
	ROM_LOAD( "68705prt.mcu",   0x00000, 0x0800, CRC(34cbb2d3) SHA1(8e0c3b13c636012d88753d547c639b1a8af85680) )

	ROM_REGION( 0x08000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "tstrike.20",        0x00000, 0x08000, CRC(b6b8bfa0) SHA1(ce50f8eb1a84873ef3df621d971a6b087473d6c2) )	/* chars */

	ROM_REGION( 0x80000, REGION_GFX2, ROMREGION_DISPOSE ) 	/* sprites */
	ROM_LOAD( "tstrike.117",  0x00000, 0x10000, CRC(f7122c0d) SHA1(2b6b359585d9df966c1fc0041fb972aac9b1ab93) )
	ROM_LOAD( "21j-b",        0x10000, 0x10000, CRC(40507a76) SHA1(74581a4b6f48100bddf20f319903af2fe36f39fa) ) /* from ddragon (116) */
	ROM_LOAD( "tstrike.115",  0x20000, 0x10000, CRC(a13c7b62) SHA1(d929d8db7eb2b949cd3bd77238611ecc54b2e885) )
	ROM_LOAD( "21j-d",        0x30000, 0x10000, CRC(cb4f231b) SHA1(9f2270f9ceedfe51c5e9a9bbb00d6f43dbc4a3ea) ) /* from ddragon (114) */
	ROM_LOAD( "tstrike.113",  0x40000, 0x10000, CRC(5ad60938) SHA1(a0af9b227157d87fa6d4ea88b34227a97baff20e) )
	ROM_LOAD( "21j-f",        0x50000, 0x10000, CRC(6ba152f6) SHA1(a301ff809be0e1471f4ff8305b30c2fa4aa57fae) ) /* from ddragon (112) */
	ROM_LOAD( "tstrike.111",  0x60000, 0x10000, CRC(7b9c87ad) SHA1(429049f84b2084bb074e380dca63b75150e7e69f) )
	ROM_LOAD( "21j-h",        0x70000, 0x10000, CRC(65c7517d) SHA1(f177ba9c1c7cc75ff04d5591b9865ee364788f94) ) /* from ddragon (110) */

	ROM_REGION( 0x40000, REGION_GFX3, ROMREGION_DISPOSE ) 	/* tiles */
	ROM_LOAD( "tstrike.78",   0x00000, 0x10000, CRC(88284aec) SHA1(f07bc5f84f2b2f976c911541c8f1ff2558f569ca) )
	ROM_LOAD( "21j-9",        0x10000, 0x10000, CRC(c6640aed) SHA1(f156c337f48dfe4f7e9caee9a72c7ea3d53e3098) ) /* from ddragon (77) */
	ROM_LOAD( "tstrike.109",  0x20000, 0x10000, CRC(8c2cd0bb) SHA1(364a708484c7750f38162d463104216bbd555b86) )
	ROM_LOAD( "21j-j",        0x30000, 0x10000, CRC(5fb42e7c) SHA1(7953316712c56c6f8ca6bba127319e24b618b646) ) /* from ddragon (108) */

	ROM_REGION( 0x20000, REGION_SOUND1, 0 ) /* adpcm samples */
	ROM_LOAD( "tstrike.94",        0x00000, 0x10000, CRC(8a2c09fc) SHA1(f59a43c3fa814b169a51744f9604d36ae63c190f) ) /* first+second half identical */
	ROM_LOAD( "tstrike.95",        0x10000, 0x08000, CRC(1812eecb) SHA1(9b7d526f30a86682cdf088600b25ea5a56b112ef) )

	ROM_REGION( 0x0300, REGION_PROMS, 0 )
	ROM_LOAD( "21j-k-0",      0x0000, 0x0100, CRC(fdb130a9) SHA1(4c4f214229b9fab2b5d69c745ec5428787b89e1f) )	/* unknown */
	ROM_LOAD( "21j-l-0",      0x0100, 0x0200, CRC(46339529) SHA1(64f4c42a826d67b7cbaa8a23a45ebc4eb6248891) )	/* unknown */
ROM_END

/** INITS **
toffy / stoffy are 'encrytped

*/

static void ddragon_restore_state(void)
{
	ddragon_bankswitch_w(0, m_ddragon_sub_port);
}

static DRIVER_INIT( toffy )
{
	/* the program rom has a simple bitswap encryption */
	UINT8 *rom=memory_region(REGION_CPU1);
	int i;

	for (i = 0;i < 0x20000;i++)
		rom[i] = BITSWAP8(rom[i] , 6,7,5,4,3,2,1,0);

	/* and the fg gfx ... */
	rom=memory_region(REGION_GFX1);

	for (i = 0;i < 0x10000;i++)
		rom[i] = BITSWAP8(rom[i] , 7,6,5,3,4,2,1,0);

	/* and the bg gfx */
	rom=memory_region(REGION_GFX3);

	for (i = 0;i < 0x10000;i++)
	{
		rom[i] = BITSWAP8(rom[i] , 7,6,1,4,3,2,5,0);
		rom[i+0x10000] = BITSWAP8(rom[i+0x10000] , 7,6,2,4,3,5,1,0);
	}

	/* and the sprites gfx */
	rom=memory_region(REGION_GFX2);

	for (i = 0;i < 0x20000;i++)
		rom[i] = BITSWAP8(rom[i] , 7,6,5,4,3,2,0,1);

	/* should the sound rom be bitswapped too? */

}

GAMEC( 1987, ddragon,  0,        ddragon,  ddragon,  0, ROT0, "Technos", "Double Dragon (Japan)", &ddragon_ctrl, NULL )
GAMEC( 1987, ddragonw, ddragon,  ddragon,  ddragon,  0, ROT0, "[Technos] (Taito license)", "Double Dragon (World)", &ddragon_ctrl, NULL )
GAMEC( 1987, ddragonu, ddragon,  ddragon,  ddragon,  0, ROT0, "[Technos] (Taito America license)", "Double Dragon (US)", &ddragon_ctrl, NULL )
GAMEC( 1987, ddragonb, ddragon,  ddragonb, ddragon,  0, ROT0, "bootleg", "Double Dragon (bootleg)", &ddragon_ctrl, NULL )
GAMEC( 1988, ddragon2, 0,        ddragon2, ddragon2, 0, ROT0, "Technos", "Double Dragon II - The Revenge (World)", &ddragon2_ctrl, NULL )
GAMEC( 1988, ddragn2u, ddragon2, ddragon2, ddragon2, 0, ROT0, "Technos", "Double Dragon II - The Revenge (US)", &ddragon2_ctrl, NULL )

/* these were conversions of double dragon */
GAME( 1991, tstrike,   0,        darktowr,  tstrike,   0, ROT0, "East Coast Coin Company (Melbourne)", "Thunder Strike")
GAME( 1992, ddungeon,  0,        darktowr,  ddungeon,  0, ROT0, "East Coast Coin Company (Melbourne)", "Dangerous Dungeons" )
GAME( 1992, darktowr,  0,        darktowr,  darktowr,  0, ROT0, "Game Room", "Dark Tower" )


/* these run on their own board, but are basically the same game. Toffy even has 'dangerous dungeons' text in it */
GAME( 1993, toffy,  0, toffy, toffy, toffy, ROT0, "Midas",                 "Toffy" )
GAME( 1994, stoffy, 0, toffy, toffy, toffy, ROT0, "Midas (Unico license)", "Super Toffy" )
