/***************************************************************************

	Hard Drivin' sound hardware

****************************************************************************/

#include "cpu/tms32010/tms32010.h"
#include "machine/atarigen.h"
#include "harddriv.h"


#define BIO_FREQUENCY		(1000000 / 50)
#define CYCLES_PER_BIO		(20000000 / BIO_FREQUENCY)


data16_t *hdsnddsp_ram;


/*************************************
 *
 *	Static globals
 *
 *************************************/

static UINT8 soundflag;
static UINT8 mainflag;
static UINT16 sounddata;
static UINT16 maindata;

static UINT8 dacmute;
static UINT8 cramen;
static UINT8 irq68k;

static offs_t sound_rom_offs;

static UINT8 *rombase;
static UINT32 romsize;
static UINT16 *comram;

static UINT32 last_bio_cycles;



/*************************************
 *
 *	Driver init
 *
 *************************************/

void hdsnd_init(void)
{
	rombase = (UINT8 *)memory_region(REGION_SOUND1);
	romsize = memory_region_length(REGION_SOUND1);
	comram = (UINT16 *)(memory_region(REGION_CPU1 + hdcpu_sounddsp) + 0x1000);
	last_bio_cycles = 0;
}



/*************************************
 *
 *	Update flags
 *
 *************************************/

static void update_68k_interrupts(void)
{
	int irqline = 0;

	if (mainflag) irqline = 1;
	if (irq68k) irqline = 3;

	if (irqline)
		cpu_set_irq_line(hdcpu_sound, irqline, ASSERT_LINE);
	else
		cpu_set_irq_line(hdcpu_sound, 7, CLEAR_LINE);
}



/*************************************
 *
 *	I/O from main CPU side
 *
 *************************************/

READ16_HANDLER( hd68k_snd_data_r )
{
	soundflag = 0;
	log_cb(RETRO_LOG_DEBUG, LOGPRE "%06X:main read from sound=%04X\n", activecpu_get_previouspc(), sounddata);
	return sounddata;
}


READ16_HANDLER( hd68k_snd_status_r )
{
	return (mainflag << 15) | (soundflag << 14) | 0x1fff;
}


static void delayed_68k_w(int data)
{
	maindata = data;
	mainflag = 1;
	update_68k_interrupts();
}


WRITE16_HANDLER( hd68k_snd_data_w )
{
	timer_set(TIME_NOW, data, delayed_68k_w);
	log_cb(RETRO_LOG_DEBUG, LOGPRE "%06X:main write to sound=%04X\n", activecpu_get_previouspc(), data);
}


WRITE16_HANDLER( hd68k_snd_reset_w )
{
	cpu_set_reset_line(hdcpu_sound, ASSERT_LINE);
	cpu_set_reset_line(hdcpu_sound, CLEAR_LINE);
	mainflag = soundflag = 0;
	update_68k_interrupts();
	log_cb(RETRO_LOG_DEBUG, LOGPRE "%06X:Reset sound\n", activecpu_get_previouspc());
}



/*************************************
 *
 *	I/O from sound CPU side
 *
 *************************************/

READ16_HANDLER( hdsnd68k_data_r )
{
	mainflag = 0;
	update_68k_interrupts();
	log_cb(RETRO_LOG_DEBUG, LOGPRE "%06X:sound read from main=%04X\n", activecpu_get_previouspc(), maindata);
	return maindata;
}


WRITE16_HANDLER( hdsnd68k_data_w )
{
	COMBINE_DATA(&sounddata);
	soundflag = 1;
	log_cb(RETRO_LOG_DEBUG, LOGPRE "%06X:sound write to main=%04X\n", activecpu_get_previouspc(), data);
}



/*************************************
 *
 *	Misc. 68k inputs
 *
 *************************************/

READ16_HANDLER( hdsnd68k_switches_r )
{
	log_cb(RETRO_LOG_DEBUG, LOGPRE "%06X:hdsnd68k_switches_r(%04X)\n", activecpu_get_previouspc(), offset);
	return 0;
}


READ16_HANDLER( hdsnd68k_320port_r )
{
	log_cb(RETRO_LOG_DEBUG, LOGPRE "%06X:hdsnd68k_320port_r(%04X)\n", activecpu_get_previouspc(), offset);
	return 0;
}


READ16_HANDLER( hdsnd68k_status_r )
{
/*FFFF 3000	R	READSTAT	Read Status*/
/*			  D15 = 'Main Flag'*/
/*			  D14 = 'Sound Flag'*/
/*			  D13 = Test Switch*/
/*			  D12 = 5220 Ready Flag (0=Ready)*/
	log_cb(RETRO_LOG_DEBUG, LOGPRE "%06X:hdsnd68k_status_r(%04X)\n", activecpu_get_previouspc(), offset);
	return (mainflag << 15) | (soundflag << 14) | 0x2000 | 0;/*((readinputport(0) & 0x0020) << 8) | 0;*/
}



/*************************************
 *
 *	Misc. 68k outputs
 *
 *************************************/

WRITE16_HANDLER( hdsnd68k_latches_w )
{
	/* bit 3 selects the value; data is ignored */
	data = (offset >> 3) & 1;

	/* low 3 bits select the function */
	offset &= 7;
	switch (offset)
	{
		case 0:	/* SPWR - 5220 write strobe */
			/* data == 0 means high, 1 means low */
			log_cb(RETRO_LOG_DEBUG, LOGPRE "%06X:SPWR=%d\n", activecpu_get_previouspc(), data);
			break;

		case 1:	/* SPRES - 5220 hard reset */
			/* data == 0 means low, 1 means high */
			log_cb(RETRO_LOG_DEBUG, LOGPRE "%06X:SPRES=%d\n", activecpu_get_previouspc(), data);
			break;

		case 2:	/* SPRATE */
			/* data == 0 means 8kHz, 1 means 10kHz */
			log_cb(RETRO_LOG_DEBUG, LOGPRE "%06X:SPRATE=%d\n", activecpu_get_previouspc(), data);
			break;

		case 3:	/* CRAMEN */
			/* data == 0 means disable 68k access to COM320, 1 means enable */
			cramen = data;
			break;

		case 4:	/* RES320 */
			log_cb(RETRO_LOG_DEBUG, LOGPRE "%06X:RES320=%d\n", activecpu_get_previouspc(), data);
			if (hdcpu_sounddsp != -1)
				cpu_set_halt_line(hdcpu_sounddsp, data ? CLEAR_LINE : ASSERT_LINE);
			break;

		case 7:	/* LED */
			break;
	}
}


WRITE16_HANDLER( hdsnd68k_speech_w )
{
	log_cb(RETRO_LOG_DEBUG, LOGPRE "%06X:hdsnd68k_speech_w(%04X)=%04X\n", activecpu_get_previouspc(), offset, data);
}


WRITE16_HANDLER( hdsnd68k_irqclr_w )
{
	irq68k = 0;
	update_68k_interrupts();
}



/*************************************
 *
 *	TMS32010 access
 *
 *************************************/

READ16_HANDLER( hdsnd68k_320ram_r )
{
	return hdsnddsp_ram[offset & 0xfff];
}


WRITE16_HANDLER( hdsnd68k_320ram_w )
{
	COMBINE_DATA(&hdsnddsp_ram[offset & 0xfff]);
}


READ16_HANDLER( hdsnd68k_320ports_r )
{
	data16_t result;
	cpuintrf_push_context(hdcpu_sounddsp);
	result = TMS32010_In(offset & 7);
	cpuintrf_pop_context();
	return result;
}


WRITE16_HANDLER( hdsnd68k_320ports_w )
{
	cpuintrf_push_context(hdcpu_sounddsp);
	TMS32010_Out(offset & 7, data);
	cpuintrf_pop_context();
}


READ16_HANDLER( hdsnd68k_320com_r )
{
	if (cramen)
		return comram[offset & 0x1ff];

	log_cb(RETRO_LOG_DEBUG, LOGPRE "%06X:hdsnd68k_320com_r(%04X) -- not allowed\n", activecpu_get_previouspc(), offset);
	return 0xffff;
}


WRITE16_HANDLER( hdsnd68k_320com_w )
{
	if (cramen)
		COMBINE_DATA(&comram[offset & 0x1ff]);
	else
		log_cb(RETRO_LOG_DEBUG, LOGPRE "%06X:hdsnd68k_320com_w(%04X)=%04X -- not allowed\n", activecpu_get_previouspc(), offset, data);
}



/*************************************
 *
 *	TMS32010 interrupts
 *
 *************************************/

READ16_HANDLER( hdsnddsp_get_bio )
{
	UINT32 cycles_since_last_bio = activecpu_gettotalcycles() - last_bio_cycles;
	INT32 cycles_until_bio = CYCLES_PER_BIO - cycles_since_last_bio;

	/* if we're not at the next BIO yet, advance us there */
	if (cycles_until_bio > 0)
	{
		activecpu_adjust_icount(-cycles_until_bio);
		last_bio_cycles += CYCLES_PER_BIO;
	}
	else
		last_bio_cycles = activecpu_gettotalcycles();
	return ASSERT_LINE;
}



/*************************************
 *
 *	TMS32010 ports
 *
 *************************************/

WRITE16_HANDLER( hdsnddsp_dac_w )
{
	/* DAC L */
	if (!dacmute)
		DAC_signed_data_16_w(offset, data ^ 0x8000);
}


WRITE16_HANDLER( hdsnddsp_comport_w )
{
	/* COM port TD0-7 */
	log_cb(RETRO_LOG_DEBUG, LOGPRE "%06X:hdsnddsp_comport_w=%d\n", activecpu_get_previouspc(), data);
}


WRITE16_HANDLER( hdsnddsp_mute_w )
{
	/* mute DAC audio, D0=1 */
/*	dacmute = data & 1;		-- NOT STUFFED */
	log_cb(RETRO_LOG_DEBUG, LOGPRE "%06X:mute DAC=%d\n", activecpu_get_previouspc(), data);
}


WRITE16_HANDLER( hdsnddsp_gen68kirq_w )
{
	/* generate 68k IRQ */
	irq68k = 1;
	update_68k_interrupts();
}


WRITE16_HANDLER( hdsnddsp_soundaddr_w )
{
	if (offset == 0)
	{
		/* select sound ROM block */
		sound_rom_offs = (sound_rom_offs & 0xffff) | ((data & 15) << 16);
	}
	else
	{
		/* sound memory address */
		sound_rom_offs = (sound_rom_offs & ~0xffff) | (data & 0xffff);
	}
}


READ16_HANDLER( hdsnddsp_rom_r )
{
	if (sound_rom_offs < romsize)
		return rombase[sound_rom_offs++] << 7;
	sound_rom_offs++;
	return 0;
}


READ16_HANDLER( hdsnddsp_comram_r )
{
	return comram[sound_rom_offs++ & 0x1ff];
}


READ16_HANDLER( hdsnddsp_compare_r )
{
	log_cb(RETRO_LOG_DEBUG, LOGPRE "%06X:hdsnddsp_compare_r(%04X)\n", activecpu_get_previouspc(), offset);
	return 0;
}
