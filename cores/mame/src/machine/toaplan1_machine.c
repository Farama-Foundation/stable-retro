/***************************************************************************
				ToaPlan game hardware from 1988-1991
				------------------------------------
 ***************************************************************************/

#include "driver.h"
#include "state.h"
#include "cpu/m68000/m68000.h"
#include "cpu/tms32010/tms32010.h"
#include "toaplan1.h"

#define CLEAR 0
#define ASSERT 1


static int toaplan1_coin_count; /* coin count increments on startup ? , so dont count it */
static int toaplan1_intenable;
static int demonwld_dsp_BIO;
static int vimana_credits;
static int vimana_latch;

static int dsp_execute;							/* Demon world */
static unsigned int dsp_addr_w, main_ram_seg;	/* Demon world */

int toaplan1_unk_reset_port;

data8_t *toaplan1_sharedram;



INTERRUPT_GEN( toaplan1_interrupt )
{
	if (toaplan1_intenable)
		cpu_set_irq_line(0, 4, HOLD_LINE);
}

WRITE16_HANDLER( toaplan1_intenable_w )
{
	if (ACCESSING_LSB)
	{
		toaplan1_intenable = data & 0xff;
	}
}


READ16_HANDLER( demonwld_dsp_r )
{
	/* DSP can read data from main CPU RAM via DSP IO port 1 */

	unsigned int input_data = 0;

	switch (main_ram_seg) {
		case 0xc00000:	input_data = *((data16_t *)&(cpu_bankbase[1][(dsp_addr_w)])); break;

		default:		log_cb(RETRO_LOG_DEBUG, LOGPRE "DSP PC:%04x Warning !!! IO reading from %08x (port 1)\n",activecpu_get_previouspc(),main_ram_seg + dsp_addr_w);
	}
	log_cb(RETRO_LOG_DEBUG, LOGPRE "DSP PC:%04x IO read %04x at %08x (port 1)\n",activecpu_get_previouspc(),input_data,main_ram_seg + dsp_addr_w);
	return input_data;
}

WRITE16_HANDLER( demonwld_dsp_w )
{
	if (offset == 0) {
		/* This sets the main CPU RAM address the DSP should */
		/*		read/write, via the DSP IO port 0 */
		/* Top three bits of data need to be shifted left 9 places */
		/*		to select which memory bank from main CPU address */
		/*		space to use */
		/* Lower thirteen bits of this data is shifted left one position */
		/*		to move it to an even address word boundary */

		dsp_addr_w = ((data & 0x1fff) << 1);
		main_ram_seg = ((data & 0xe000) << 9);
		log_cb(RETRO_LOG_DEBUG, LOGPRE "DSP PC:%04x IO write %04x (%08x) at port 0\n",activecpu_get_previouspc(),data,main_ram_seg + dsp_addr_w);
	}
	if (offset == 1) {
		/* Data written to main CPU RAM via DSP IO port 1*/

		dsp_execute = 0;
		switch (main_ram_seg) {
			case 0xc00000:	*((data16_t *)&(cpu_bankbase[1][(dsp_addr_w)])) = data;
							if ((dsp_addr_w < 3) && (data == 0)) dsp_execute = 1; break;
			default:		log_cb(RETRO_LOG_DEBUG, LOGPRE "DSP PC:%04x Warning !!! IO writing to %08x (port 1)\n",activecpu_get_previouspc(),main_ram_seg + dsp_addr_w);
		}
		log_cb(RETRO_LOG_DEBUG, LOGPRE "DSP PC:%04x IO write %04x at %08x (port 1)\n",activecpu_get_previouspc(),data,main_ram_seg + dsp_addr_w);
	}
	if (offset == 3) {
		/* data 0xffff	means inhibit BIO line to DSP and enable  */
		/*				communication to main processor */
		/*				Actually only DSP data bit 15 controls this */
		/* data 0x0000	means set DSP BIO line active and disable */
		/*				communication to main processor*/
		log_cb(RETRO_LOG_DEBUG, LOGPRE "DSP PC:%04x IO write %04x at port 3\n",activecpu_get_previouspc(),data);
		if (data & 0x8000) {
			demonwld_dsp_BIO = CLEAR_LINE;
		}
		if (data == 0) {
			if (dsp_execute) {
				log_cb(RETRO_LOG_DEBUG, LOGPRE "Turning 68000 on\n");
				timer_suspendcpu(0, CLEAR, SUSPEND_REASON_HALT);
				dsp_execute = 0;
			}
			demonwld_dsp_BIO = ASSERT_LINE;
		}
	}
}

WRITE16_HANDLER( demonwld_dsp_ctrl_w )
{
#if 0
	log_cb(RETRO_LOG_DEBUG, LOGPRE "68000:%08x  Writing %08x to %08x.\n",activecpu_get_pc() ,data ,0xe0000a + offset);
#endif

	if (ACCESSING_LSB)
	{
		switch (data)
		{
			case 0x00: 	/* This means assert the INT line to the DSP */
						log_cb(RETRO_LOG_DEBUG, LOGPRE "Turning DSP on and 68000 off\n");
						timer_suspendcpu(2, CLEAR, SUSPEND_REASON_HALT);
						cpu_set_irq_line(2, 0, ASSERT_LINE); /* TMS32020 INT */
						timer_suspendcpu(0, ASSERT, SUSPEND_REASON_HALT);
						break;
			case 0x01:	/* This means inhibit the INT line to the DSP */
						log_cb(RETRO_LOG_DEBUG, LOGPRE "Turning DSP off\n");
						cpu_set_irq_line(2, 0, CLEAR_LINE); /* TMS32020 INT */
						timer_suspendcpu(2, ASSERT, SUSPEND_REASON_HALT);
						break;
			default:	log_cb(RETRO_LOG_DEBUG, LOGPRE "68000:%04x  writing unknown command %08x to %08x\n",activecpu_get_previouspc() ,data ,0xe0000a + offset);
		}
	}
	else
	{
		log_cb(RETRO_LOG_DEBUG, LOGPRE "68000:%04x  writing unknown command %08x to %08x\n",activecpu_get_previouspc() ,data ,0xe0000a + offset);
	}
}


READ16_HANDLER ( demonwld_BIO_r )
{
	return demonwld_dsp_BIO;
}


READ16_HANDLER( samesame_port_6_word_r )
{
	/* Bit 0x80 is secondary CPU (HD647180) ready signal */
	log_cb(RETRO_LOG_DEBUG, LOGPRE "PC:%04x Warning !!! IO reading from $14000a\n",activecpu_get_previouspc());
	return (0x80 | input_port_6_word_r(0,0)) & 0xff;
}

READ16_HANDLER( toaplan1_shared_r )
{
	return toaplan1_sharedram[offset] & 0xff;
}

WRITE16_HANDLER( toaplan1_shared_w )
{
	if (ACCESSING_LSB)
	{
		toaplan1_sharedram[offset] = data & 0xff;
	}
}

WRITE16_HANDLER( toaplan1_reset_sound )
{
	/* Reset the secondary CPU and sound chip during soft resets */

	if (ACCESSING_LSB && (data == 0))
	{
		log_cb(RETRO_LOG_DEBUG, LOGPRE "PC:%04x  Resetting Sound CPU and Sound chip (%08x)\n",activecpu_get_previouspc(),data);
		if (Machine->drv->sound[0].sound_type == SOUND_YM3812)
			YM3812_sh_reset();
		if (Machine->drv->cpu[1].cpu_type == CPU_Z80)
			cpu_set_reset_line(1,PULSE_LINE);
	}
}

MACHINE_INIT( toaplan1 )
{
	toaplan1_intenable = 0;
	toaplan1_coin_count = 0;
	toaplan1_unk_reset_port = 0;
	coin_lockout_global_w(0);
	state_save_register_INT32("toaplan1", 0, "Int_enable", &toaplan1_intenable, 1);
	state_save_register_INT32("toaplan1", 0, "Coin_counter", &toaplan1_coin_count, 1);
}

MACHINE_INIT( zerozone )	/* Hack for ZeroWing and OutZone. See the video driver */
{
	machine_init_toaplan1();
	toaplan1_unk_reset_port = 1;
}

MACHINE_INIT( demonwld )
{
	dsp_addr_w = 0;
	dsp_execute = 0;
	main_ram_seg = 0;
	state_save_register_INT32("demonwld", 0, "DSP_execute", &dsp_execute, 1);
	state_save_register_UINT32("demonwld", 0, "DSP_out_addr", &dsp_addr_w, 1);
	state_save_register_UINT32("demonwld", 0, "DSP_to_68K_RAM_bank", &main_ram_seg, 1);
	machine_init_toaplan1();
}

MACHINE_INIT( vimana )
{
	machine_init_toaplan1();
}

WRITE_HANDLER( rallybik_coin_w )
{
	switch (data) {
		case 0x08: if (toaplan1_coin_count) { coin_counter_w(0,1); coin_counter_w(0,0); } break;
		case 0x09: if (toaplan1_coin_count) { coin_counter_w(2,1); coin_counter_w(2,0); } break;
		case 0x0a: if (toaplan1_coin_count) { coin_counter_w(1,1); coin_counter_w(1,0); } break;
		case 0x0b: if (toaplan1_coin_count) { coin_counter_w(3,1); coin_counter_w(3,0); } break;
		case 0x0c: coin_lockout_w(0,1); coin_lockout_w(2,1); break;
		case 0x0d: coin_lockout_w(0,0); coin_lockout_w(2,0); break;
		case 0x0e: coin_lockout_w(1,1); coin_lockout_w(3,1); break;
		case 0x0f: coin_lockout_w(1,0); coin_lockout_w(3,0); toaplan1_coin_count=1; break;
		default:   log_cb(RETRO_LOG_DEBUG, LOGPRE "PC:%04x  Writing unknown data (%04x) to coin count/lockout port\n",activecpu_get_previouspc(),data); break;
	}
}

WRITE_HANDLER( toaplan1_coin_w )
{
	log_cb(RETRO_LOG_DEBUG, LOGPRE "Z80 writing %02x to coin control\n",data);
	/* This still isnt too clear yet. */
	/* Coin C has no coin lock ? */
	/* Are some outputs for lights ? (no space on JAMMA for it though) */

	switch (data) {
		case 0xee: coin_counter_w(1,1); coin_counter_w(1,0); break; /* Count slot B */
		case 0xed: coin_counter_w(0,1); coin_counter_w(0,0); break; /* Count slot A */
	/* The following are coin counts after coin-lock active (faulty coin-lock ?) */
		case 0xe2: coin_counter_w(1,1); coin_counter_w(1,0); coin_lockout_w(1,1); break;
		case 0xe1: coin_counter_w(0,1); coin_counter_w(0,0); coin_lockout_w(0,1); break;

		case 0xec: coin_lockout_global_w(0); break;	/* ??? count games played */
		case 0xe8: break;	/* ??? Maximum credits reached with coin/credit ratio */
		case 0xe4: break;	/* ??? Reset coin system */

		case 0x0c: coin_lockout_global_w(0); break;	/* Unlock all coin slots */
		case 0x08: coin_lockout_w(2,0); break;	/* Unlock coin slot C */
		case 0x09: coin_lockout_w(0,0); break;	/* Unlock coin slot A */
		case 0x0a: coin_lockout_w(1,0); break;	/* Unlock coin slot B */

		case 0x02: coin_lockout_w(1,1); break;	/* Lock coin slot B */
		case 0x01: coin_lockout_w(0,1); break;	/* Lock coin slot A */
		case 0x00: coin_lockout_global_w(1); break;	/* Lock all coin slots */
		default:   log_cb(RETRO_LOG_DEBUG, LOGPRE "PC:%04x  Writing unknown data (%04x) to coin count/lockout port\n",activecpu_get_previouspc(),data); break;
	}
}

WRITE16_HANDLER( samesame_coin_w )
{
	if (ACCESSING_LSB)
	{
		toaplan1_coin_w(offset, data & 0xff);
	}
	if (ACCESSING_MSB && (data&0xff00))
	{
		log_cb(RETRO_LOG_DEBUG, LOGPRE "PC:%04x  Writing unknown MSB data (%04x) to coin count/lockout port\n",activecpu_get_previouspc(),data);
	}
}
