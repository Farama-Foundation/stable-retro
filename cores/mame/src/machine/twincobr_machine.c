/****************************************************************************
 *	Twin Cobra																*
 *	Communications and memory functions between shared CPU memory spaces	*
 ****************************************************************************/

#include "driver.h"
#include "state.h"
#include "cpu/m68000/m68000.h"
#include "cpu/tms32010/tms32010.h"
#include "vidhrdw/generic.h"

#define LOG_DSP_CALLS 0
#define CLEAR  0
#define ASSERT 1



data16_t *twincobr_68k_dsp_ram;
data8_t  *twincobr_sharedram;
data8_t  *wardner_mainram;


extern int twincobr_fg_rom_bank;
extern int twincobr_bg_ram_bank;
extern int twincobr_display_on;
extern int twincobr_flip_screen;
extern int twincobr_flip_x_base;
extern int twincobr_flip_y_base;
extern int wardner_sprite_hack;

static int dsp_execute;
static unsigned int dsp_addr_w, main_ram_seg;
int toaplan_main_cpu;	/* Main CPU type.  0 = 68000, 1 = Z80 */

#if LOG_DSP_CALLS
static char *toaplan_cpu_type[2] = { "68K"   , "Z80" };
static int  toaplan_port_type[2] = { 0x7800c , 0x5c  };
#endif

int twincobr_intenable;
int wardner_membank;
static int twincobr_dsp_BIO;
static int fsharkbt_8741;


MACHINE_INIT( fsharkbt_reset_8741_mcu )	/* machine_init_fsharkbt_reset_8741_mcu */
{
	toaplan_main_cpu = 0;		/* 68000 */
	twincobr_display_on = 0;
	twincobr_intenable = 0;
	dsp_addr_w = dsp_execute = 0;
	main_ram_seg = 0;
	fsharkbt_8741 = -1;
	twincobr_dsp_BIO = CLEAR_LINE;

	state_save_register_UINT32("toaplan0", 0, "DSP_to_68K_RAM_bank", &main_ram_seg, 1);
	state_save_register_UINT32("toaplan0", 0, "DSP_out_addr", &dsp_addr_w, 1);
	state_save_register_int("toaplan0", 0, "Int_enable", &twincobr_intenable);
	state_save_register_int("toaplan0", 0, "DSP_BIO_pin", &twincobr_dsp_BIO);
	state_save_register_int("toaplan0", 0, "DSP_execute", &dsp_execute);
	state_save_register_int("toaplan0", 0, "CPU#0_type", &toaplan_main_cpu);
	state_save_register_int("fsharkbt", 0, "MCU_Output", &fsharkbt_8741);
}

MACHINE_INIT( wardner )
{
	toaplan_main_cpu = 1;		/* Z80 */
	twincobr_display_on = 1;
	twincobr_intenable = 0;
	dsp_addr_w = dsp_execute = 0;
	main_ram_seg = 0;
	twincobr_dsp_BIO = CLEAR_LINE;
	wardner_membank = 0;

	state_save_register_UINT32("wardner", 0, "DSP_to_Z80_RAM_bank", &main_ram_seg, 1);
	state_save_register_UINT32("wardner", 0, "DSP_out_addr", &dsp_addr_w, 1);
	state_save_register_int("wardner", 0, "Int_enable", &twincobr_intenable);
	state_save_register_int("wardner", 0, "DSP_BIO_pin", &twincobr_dsp_BIO);
	state_save_register_int("wardner", 0, "DSP_execute", &dsp_execute);
	state_save_register_int("wardner", 0, "CPU#0_type", &toaplan_main_cpu);
	state_save_register_int("wardner", 0, "Wardner_MemBank", &wardner_membank);
}



INTERRUPT_GEN( twincobr_interrupt )
{
	if (twincobr_intenable) {
		twincobr_intenable = 0;
		cpu_set_irq_line(0, MC68000_IRQ_4, HOLD_LINE);
	}
}




READ16_HANDLER( twincobr_dsp_r )
{
	/* DSP can read data from main CPU RAM via DSP IO port 1 */

	unsigned int input_data = 0;
	switch (main_ram_seg) {
		case 0x30000:	input_data = twincobr_68k_dsp_ram[dsp_addr_w]; break;
		case 0x40000:	input_data = spriteram16[dsp_addr_w]; break;
		case 0x50000:	input_data = paletteram16[dsp_addr_w]; break;
		case 0x7000:	input_data = wardner_mainram[dsp_addr_w*2] + (wardner_mainram[dsp_addr_w*2+1]<<8); break;
		case 0x8000:	input_data = spriteram16[dsp_addr_w]; break;
		case 0xa000:	input_data = paletteram[dsp_addr_w*2] + (paletteram[dsp_addr_w*2+1]<<8); break;
		default:		log_cb(RETRO_LOG_DEBUG, LOGPRE "DSP PC:%04x Warning !!! IO reading from %08x (port 1)\n",activecpu_get_previouspc(),main_ram_seg + dsp_addr_w);
	}
#if LOG_DSP_CALLS
	log_cb(RETRO_LOG_DEBUG, LOGPRE "DSP PC:%04x IO read %04x at %08x (port 1)\n",activecpu_get_previouspc(),input_data,main_ram_seg + dsp_addr_w);
#endif
	return input_data;
}

READ16_HANDLER( fsharkbt_dsp_r )
{
	/* IO Port 2 used by Flying Shark bootleg */
	/* DSP reads data from an extra MCU (8741) at IO port 2 */
	/* Port is read three times during startup. First and last data */
	/*	 read must equal, but second data read must be different */
	fsharkbt_8741 += 1;
#if LOG_DSP_CALLS
	log_cb(RETRO_LOG_DEBUG, LOGPRE "DSP PC:%04x IO read %04x from 8741 MCU (port 2)\n",activecpu_get_previouspc(),(fsharkbt_8741 & 0x08));
#endif
	return (fsharkbt_8741 & 1);
}

WRITE16_HANDLER( twincobr_dsp_w )
{
	if (offset == 0) {
		/* This sets the main CPU RAM address the DSP should */
		/*		read/write, via the DSP IO port 0 */
		/* Top three bits of data need to be shifted left 3 places */
		/*		to select which memory bank from main CPU address */
		/*		space to use */
		/* Lower thirteen bits of this data is shifted left one position */
		/*		to move it to an even address word boundary */

		dsp_addr_w = data & 0x1fff;
		main_ram_seg = ((data & 0xe000) << 3);
		if (toaplan_main_cpu == 1) {		/* Z80 */
			dsp_addr_w &= 0x7ff;
			if (main_ram_seg == 0x30000) main_ram_seg = 0x7000;
			if (main_ram_seg == 0x40000) main_ram_seg = 0x8000;
			if (main_ram_seg == 0x50000) main_ram_seg = 0xa000;
		}
#if LOG_DSP_CALLS
		log_cb(RETRO_LOG_DEBUG, LOGPRE "DSP PC:%04x IO write %04x (%08x) at port 0\n",activecpu_get_previouspc(),data,main_ram_seg + dsp_addr_w);
#endif
	}
	if (offset == 1) {
		/* Data written to main CPU RAM via DSP IO port 1*/
		dsp_execute = 0;
		switch (main_ram_seg) {
			case 0x30000:	twincobr_68k_dsp_ram[dsp_addr_w]=data;
							if ((dsp_addr_w < 2) && (data == 0)) dsp_execute = 1; break;
			case 0x40000:	spriteram16[dsp_addr_w]=data; break;
			case 0x50000:	paletteram16[dsp_addr_w]=data; break;
			case 0x7000:	wardner_mainram[dsp_addr_w*2] = data;
							wardner_mainram[dsp_addr_w*2 + 1] = data >> 8;
							if ((dsp_addr_w < 2) && (data == 0)) dsp_execute = 1; break;
			case 0x8000:	spriteram16[dsp_addr_w]=data; break;
			case 0xa000:	paletteram[dsp_addr_w*2] = data & 0xff;
							paletteram[dsp_addr_w*2 + 1] = (data >> 8) & 0xff; break;
			default:		log_cb(RETRO_LOG_DEBUG, LOGPRE "DSP PC:%04x Warning !!! IO writing to %08x (port 1)\n",activecpu_get_previouspc(),main_ram_seg + dsp_addr_w);
		}
#if LOG_DSP_CALLS
		log_cb(RETRO_LOG_DEBUG, LOGPRE "DSP PC:%04x IO write %04x at %08x (port 1)\n",activecpu_get_previouspc(),data,main_ram_seg + dsp_addr_w);
#endif
	}
	if (offset == 2) {
		/* Flying Shark bootleg DSP writes data to an extra MCU (8741) at IO port 2 */
#if 0
		log_cb(RETRO_LOG_DEBUG, LOGPRE "DSP PC:%04x IO write from DSP RAM:%04x to 8741 MCU (port 2)\n",activecpu_get_previouspc(),fsharkbt_8741);
#endif
	}
	if (offset == 3) {
		/* data 0xffff	means inhibit BIO line to DSP and enable  */
		/*				communication to main processor */
		/*				Actually only DSP data bit 15 controls this */
		/* data 0x0000	means set DSP BIO line active and disable */
		/*				communication to main processor*/
#if LOG_DSP_CALLS
		log_cb(RETRO_LOG_DEBUG, LOGPRE "DSP PC:%04x IO write %04x at port 3\n",activecpu_get_previouspc(),data);
#endif
		if (data & 0x8000) {
			twincobr_dsp_BIO = CLEAR_LINE;
		}
		if (data == 0) {
			if (dsp_execute) {
#if LOG_DSP_CALLS
				log_cb(RETRO_LOG_DEBUG, LOGPRE "Turning %s on\n",toaplan_cpu_type[toaplan_main_cpu]);
#endif
				timer_suspendcpu(0, CLEAR, SUSPEND_REASON_HALT);
				dsp_execute = 0;
			}
			twincobr_dsp_BIO = ASSERT_LINE;
		}
	}
}

READ16_HANDLER( twincobr_68k_dsp_r )
{
	return twincobr_68k_dsp_ram[offset];
}

WRITE16_HANDLER( twincobr_68k_dsp_w )
{
#if LOG_DSP_CALLS
	if (offset < 5) log_cb(RETRO_LOG_DEBUG, LOGPRE "%s:%08x write %08x at %08x\n",toaplan_cpu_type[toaplan_main_cpu],activecpu_get_pc(),data,0x30000+offset);
#endif
	COMBINE_DATA(&twincobr_68k_dsp_ram[offset]);
}


READ16_HANDLER ( twincobr_BIO_r )
{
	return twincobr_dsp_BIO;
}


WRITE_HANDLER( wardner_mainram_w )
{
#if 0
	if ((offset == 4) && (data != 4)) log_cb(RETRO_LOG_DEBUG, LOGPRE "CPU #0:%04x  Writing %02x to %04x of main RAM (DSP command number)\n",activecpu_get_pc(),data, offset + 0x7000);
#endif
	wardner_mainram[offset] = data;

}
READ_HANDLER( wardner_mainram_r )
{
	return wardner_mainram[offset];
}


static void toaplan0_control_w(int offset, int data)
{
#if LOG_DSP_CALLS
	log_cb(RETRO_LOG_DEBUG, LOGPRE "%s:%08x  Writing %08x to %08x.\n",toaplan_cpu_type[toaplan_main_cpu],activecpu_get_pc(),data,toaplan_port_type[toaplan_main_cpu] - offset);
#endif

	if (toaplan_main_cpu == 1) {
		if (data == 0x0c) { data = 0x1c; wardner_sprite_hack=0; }	/* Z80 ? */
		if (data == 0x0d) { data = 0x1d; wardner_sprite_hack=1; }	/* Z80 ? */
	}

	switch (data) {
		case 0x0004: twincobr_intenable = 0; break;
		case 0x0005: twincobr_intenable = 1; break;
		case 0x0006: twincobr_flip_screen = 0; twincobr_flip_x_base=0x037; twincobr_flip_y_base=0x01e; break;
		case 0x0007: twincobr_flip_screen = 1; twincobr_flip_x_base=0x085; twincobr_flip_y_base=0x0f2; break;
		case 0x0008: twincobr_bg_ram_bank = 0x0000; break;
		case 0x0009: twincobr_bg_ram_bank = 0x1000; break;
		case 0x000a: twincobr_fg_rom_bank = 0x0000; break;
		case 0x000b: twincobr_fg_rom_bank = 0x1000; break;
		case 0x000e: twincobr_display_on  = 0x0000; break; /* Turn display off */
		case 0x000f: twincobr_display_on  = 0x0001; break; /* Turn display on */
		case 0x000c: if (twincobr_display_on) {
						/* This means assert the INT line to the DSP */
#if LOG_DSP_CALLS
						log_cb(RETRO_LOG_DEBUG, LOGPRE "Turning DSP on and %s off\n",toaplan_cpu_type[toaplan_main_cpu]);
#endif
						timer_suspendcpu(2, CLEAR, SUSPEND_REASON_HALT);
						cpu_set_irq_line(2, 0, ASSERT_LINE); /* TMS32010 INT */
						timer_suspendcpu(0, ASSERT, SUSPEND_REASON_HALT);
					} break;
		case 0x000d: if (twincobr_display_on) {
						/* This means inhibit the INT line to the DSP */
#if LOG_DSP_CALLS
						log_cb(RETRO_LOG_DEBUG, LOGPRE "Turning DSP off\n");
#endif
						cpu_set_irq_line(2, 0, CLEAR_LINE); /* TMS32010 INT */
						timer_suspendcpu(2, ASSERT, SUSPEND_REASON_HALT);
					} break;
	}
}
WRITE16_HANDLER( twincobr_control_w )
{
	if (ACCESSING_LSB)
	{
		toaplan0_control_w(offset, data & 0xff);
	}
}
WRITE_HANDLER( wardner_control_w )
{
	toaplan0_control_w(offset, data);
}


READ16_HANDLER( twincobr_sharedram_r )
{
	return twincobr_sharedram[offset];
}

WRITE16_HANDLER( twincobr_sharedram_w )
{
	if (ACCESSING_LSB)
	{
		twincobr_sharedram[offset] = data & 0xff;
	}
}

static void toaplan0_coin_dsp_w(int offset, int data)
{
#if LOG_DSP_CALLS
	if (data > 1)
		log_cb(RETRO_LOG_DEBUG, LOGPRE "%s:%08x  Writing %08x to %08x.\n",toaplan_cpu_type[toaplan_main_cpu],activecpu_get_pc(),data,toaplan_port_type[toaplan_main_cpu] - offset);
#endif
	switch (data) {
		case 0x08: coin_counter_w(0,0); break;
		case 0x09: coin_counter_w(0,1); break;
		case 0x0a: coin_counter_w(1,0); break;
		case 0x0b: coin_counter_w(1,1); break;
		case 0x0c: coin_lockout_w(0,1); break;
		case 0x0d: coin_lockout_w(0,0); break;
		case 0x0e: coin_lockout_w(1,1); break;
		case 0x0f: coin_lockout_w(1,0); break;
		/****** The following apply to Flying Shark/Wardner only ******/
		case 0x00:	/* This means assert the INT line to the DSP */
#if LOG_DSP_CALLS
					log_cb(RETRO_LOG_DEBUG, LOGPRE "Turning DSP on and %s off\n",toaplan_cpu_type[toaplan_main_cpu]);
#endif
					timer_suspendcpu(2, CLEAR, SUSPEND_REASON_HALT);
					cpu_set_irq_line(2, 0, ASSERT_LINE); /* TMS32010 INT */
					timer_suspendcpu(0, ASSERT, SUSPEND_REASON_HALT);
					break;
		case 0x01:	/* This means inhibit the INT line to the DSP */
#if LOG_DSP_CALLS
					log_cb(RETRO_LOG_DEBUG, LOGPRE "Turning DSP off\n");
#endif
					cpu_set_irq_line(2, 0, CLEAR_LINE); /* TMS32010 INT */
					timer_suspendcpu(2, ASSERT, SUSPEND_REASON_HALT);
					break;
	}
}
WRITE16_HANDLER( fshark_coin_dsp_w )
{
	if (ACCESSING_LSB)
	{
		toaplan0_coin_dsp_w(offset, data & 0xff);
	}
}
WRITE_HANDLER( twincobr_coin_w )
{
	toaplan0_coin_dsp_w(offset, data);
}
WRITE_HANDLER( wardner_coin_dsp_w )
{
	toaplan0_coin_dsp_w(offset, data);
}
