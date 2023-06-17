/***************************************************************************

							  -= Metro Games =-

					driver by	Luca Elia (l.elia@tin.it)


Main  CPU    :  MC68000

Video Chips  :  Imagetek 14100 052 9227KK701	Or
                Imagetek 14220 071 9338EK707	Or
				Imagetek 14300 095

Sound CPU    :  NEC78C10 [Optional]

Sound Chips  :	OKIM6295 + YM2413  or
                YRW801-M + YMF278B (YM2610 compatible)

Other        :  Memory Blitter

---------------------------------------------------------------------------
Year + Game						PCB			Video Chip	Issues / Notes
---------------------------------------------------------------------------
92	Last Fortress - Toride		VG420		14100
92	Pang Poms					VG420		14100
92	Sky Alert					VG420		14100
92	The Karate Tournament		VG460-B		14100
93?	Lady Killer / Moeyo Gonta!! VG460-B		14100
93	Poitto!						MTR5260-A	14100
94	Dharma Doujou				?			?
94	Toride II Adauchi Gaiden	MTR5260-A	14220
94	Blazing Tornado				?			14220		Also has Konami 053936 gfx chip
96  Grand Striker 2             HUM-003(A)  14220		Also has Konami 053936 gfx chip
95	Daitoride					MTR5260-A	14220
95	Pururun						MTR5260-A	14220
95	Puzzli 						MTR5260-A	14220
96	Sankokushi					MTR5260-A	14220
96	Bal Cube					?			14220		No sound CPU
96	Bang Bang Ball				?			14220		No sound CPU
95	Mahjong Doukyuhsei			VG330-B		14300		No sound CPU
95	Mahjong Doukyuusei Special	VG340-A		14300		No sound CPU
97	Mahjong Gakuensai			VG340-A		14300		No sound CPU
98	Mahjong Gakuensai 2			VG340-A		14300		No sound CPU
96	Mouja						VG410-B		14300		No sound CPU
---------------------------------------------------------------------------
Not dumped yet:
94	Gun Master
94	Toride II

To Do:

-	1 pixel granularity in the window's placement (8 pixels now, see daitorid title)
-	Coin lockout
-	Some gfx problems in ladykill, 3kokushi, puzzli, gakusai
-	Are the 16x16 tiles used by Mouja a Imagetek 14300-only feature?
-	Flip screen doesn't work correctly in Mouja due to asymmetrical visible area

Notes:

-	To enter service mode in Lady Killer, toggle the dip switch and reset
	keeping  start 2 pressed.
-	Sprite zoom in Mouja at the end of a match looks wrong, but it's been verified
	to be the same on the original board

lastfort info from guru
---
Master clock = 24.00MHz
D7810 clock : 12.00MHz (24 / 2)
M6295 clock: 1.200MHz (24 / 20), sample rate =  M6295 clock /165
YM2413 clock: 3.579545MHz
Vsync: 58Hz
HSync: 15.16kHz

Compared to the real PCB, MAME is too fast, so 60fps needs to be changed to 58fps (i.e 58Hz).
--
driver modified by Eisuke Watanabe
***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "vidhrdw/konamiic.h"
#include "cpu/upd7810/upd7810.h"
#include "machine/eeprom.h"

/* Variables defined in vidhrdw: */

extern data16_t *metro_videoregs;
extern data16_t *metro_screenctrl;
extern data16_t *metro_scroll;
extern data16_t *metro_tiletable;
extern size_t metro_tiletable_size;
extern data16_t *metro_vram_0, *metro_vram_1, *metro_vram_2;
extern data16_t *metro_window;
extern data16_t *metro_K053936_ram;
WRITE16_HANDLER( metro_K053936_w );


/* Functions defined in vidhrdw: */

WRITE16_HANDLER( metro_paletteram_w );

WRITE16_HANDLER( metro_window_w );

WRITE16_HANDLER( metro_vram_0_w );
WRITE16_HANDLER( metro_vram_1_w );
WRITE16_HANDLER( metro_vram_2_w );


VIDEO_START( metro_14100 );
VIDEO_START( metro_14220 );
VIDEO_START( metro_14300 );
VIDEO_START( blzntrnd );
VIDEO_START( gstrik2 );

VIDEO_UPDATE( metro );


/***************************************************************************


								Interrupts


***************************************************************************/

static int irq_line, blitter_bit;

static UINT8 requested_int[8];

static data16_t *metro_irq_levels, *metro_irq_vectors, *metro_irq_enable;

READ16_HANDLER( metro_irq_cause_r )
{
	return	requested_int[0] * 0x01 +	/* vblank*/
			requested_int[1] * 0x02 +
			requested_int[2] * 0x04 +	/* blitter*/
			requested_int[3] * 0x08 +
			requested_int[4] * 0x10 +
			requested_int[5] * 0x20 +
			requested_int[6] * 0x40 +	/* unused*/
			requested_int[7] * 0x80 ;	/* unused*/
}


/* Update the IRQ state based on all possible causes */
static void update_irq_state(void)
{
	/*	Get the pending IRQs (only the enabled ones, e.g. where
		irq_enable is *0*)	*/
	data16_t irq = metro_irq_cause_r(0,0) & ~*metro_irq_enable;

	if (irq_line == -1)	/* mouja, gakusai, gakusai2, dokyusei, dokyusp */
	{
		/*	This is for games that supply an *IRQ Vector* on the data bus
			together with an IRQ level for each possible IRQ source */

		int i = 0;
		while ( i < 8 )
		{
			if (irq & (1 << i))
			{
				cpu_set_irq_line(0, metro_irq_levels[i]&7, ASSERT_LINE);
				return;
			}
			i++;
		}
		cpu_set_irq_line(0, 0, ASSERT_LINE);
	}
	else
	{
		/*	This is for games where every IRQ source generates the same
			IRQ level. The interrupt service routine then reads the actual
			source by peeking a register (metro_irq_cause_r) */

		int state =	(irq ? ASSERT_LINE : CLEAR_LINE);
		cpu_set_irq_line(0, irq_line, state);
	}
}


/* For games that supply an *IRQ Vector* on the data bus */
int metro_irq_callback(int int_level)
{
/*	log_cb(RETRO_LOG_DEBUG, LOGPRE "CPU #0 PC %06X: irq callback returns %04X\n",activecpu_get_pc(),metro_irq_vectors[int_level]);*/
	return metro_irq_vectors[int_level]&0xff;
}

MACHINE_INIT( metro )
{
	if (irq_line == -1)
		cpu_set_irq_callback(0, metro_irq_callback);
}


WRITE16_HANDLER( metro_irq_cause_w )
{
/*if (data & ~0x15)	log_cb(RETRO_LOG_DEBUG, LOGPRE "CPU #0 PC %06X : unknown bits of irqcause written: %04X\n",activecpu_get_pc(),data);*/

	if (ACCESSING_LSB)
	{
		data &= ~*metro_irq_enable;
		if (data & 0x01)	requested_int[0] = 0;
		if (data & 0x02)	requested_int[1] = 0;	/* DAITORIDE, BALCUBE, KARATOUR, MOUJA*/
		if (data & 0x04)	requested_int[2] = 0;
		if (data & 0x08)	requested_int[3] = 0;	/* KARATOUR*/
		if (data & 0x10)	requested_int[4] = 0;
		if (data & 0x20)	requested_int[5] = 0;	/* KARATOUR, BLZNTRND*/
		if (data & 0x40)	requested_int[6] = 0;
		if (data & 0x80)	requested_int[7] = 0;
	}

	update_irq_state();
}


INTERRUPT_GEN( metro_interrupt )
{
	switch ( cpu_getiloops() )
	{
		case 0:
			requested_int[0] = 1;
			update_irq_state();
			break;

		default:
			requested_int[4] = 1;
			update_irq_state();
			break;
	}
}

/* Lev 1. Lev 2 seems sound related */
INTERRUPT_GEN( bangball_interrupt )
{
	requested_int[0] = 1;	/* set scroll regs if a flag is set*/
	requested_int[4] = 1;	/* clear that flag*/
	update_irq_state();
}


static void vblank_end_callback(int param)
{
	requested_int[5] = param;
}

/* lev 2-7 (lev 1 seems sound related) */
INTERRUPT_GEN( karatour_interrupt )
{
	switch ( cpu_getiloops() )
	{
		case 0:
			requested_int[0] = 1;
			requested_int[5] = 1;	/* write the scroll registers*/
			timer_set(TIME_IN_USEC(DEFAULT_REAL_60HZ_VBLANK_DURATION), 0, vblank_end_callback);
			update_irq_state();
			break;

		default:
			requested_int[4] = 1;
			update_irq_state();
			break;
	}
}

static void *mouja_irq_timer;

static void mouja_irq_callback(int param)
{
	requested_int[0] = 1;
	update_irq_state();
}

static WRITE16_HANDLER( mouja_irq_timer_ctrl_w )
{
	double timer;

	timer = 58.0 + (0xff - (data & 0xff)) / 2.2;					/* 0xff=58Hz, 0x80=116Hz? */
	timer_adjust(mouja_irq_timer, TIME_NOW, 0, TIME_IN_HZ(timer));
}

INTERRUPT_GEN( mouja_interrupt )
{
	requested_int[1] = 1;
	update_irq_state();
}


INTERRUPT_GEN( gakusai_interrupt )
{
	switch ( cpu_getiloops() )
	{
		case 0:
			requested_int[1] = 1;
			update_irq_state();
			break;
	}
}

INTERRUPT_GEN( dokyusei_interrupt )
{
	switch ( cpu_getiloops() )
	{
		case 0:
			requested_int[1] = 1;
			update_irq_state();
			break;
		case 1:	/* needed?*/
			requested_int[5] = 1;
			update_irq_state();
			break;
	}
}

static INTERRUPT_GEN( msgogo_interrupt )
{

    switch (cpu_getiloops())
    {
        case 10:
            requested_int[0] = 1;
            update_irq_state();
            break;

        case 224:
            requested_int[4] = 1;
            update_irq_state();
            break;
    }
}


static void ymf278b_interrupt(int active)
{
	cpu_set_irq_line(0, 2, active);
}

/***************************************************************************


							Sound Communication


***************************************************************************/

static data16_t metro_soundstatus;
static int porta, portb, busy_sndcpu;

static int metro_io_callback(int ioline, int state)
{
	data8_t data = 0;

    switch ( ioline )
	{
		case UPD7810_RXD:	/* read the RxD line */
			data = soundlatch_r(0);
			state = data & 1;
			soundlatch_w(0, data >> 1);
			break;
		default:
			log_cb(RETRO_LOG_DEBUG, LOGPRE "upd7810 ioline %d not handled\n", ioline);
    }

	return state;
}

static WRITE16_HANDLER( metro_soundlatch_w )
{
	if (ACCESSING_LSB)
	{
		soundlatch_w(0,data & 0xff);
		cpu_set_nmi_line(1, PULSE_LINE);
		cpu_spinuntil_int();
		busy_sndcpu = 1;
	}
}


static READ16_HANDLER( metro_soundstatus_r )
{
	return (busy_sndcpu ? 0x00 : 0x01);
}

static READ16_HANDLER( daitorid_soundstatus_r )
{
	return readinputport(0) | (busy_sndcpu ? 0x80 : 0x00);
}

static WRITE16_HANDLER( metro_soundstatus_w )
{
	if (ACCESSING_LSB)
		metro_soundstatus = data & 0x01;
}


static WRITE_HANDLER( metro_sound_rombank_w )
{
	int bankaddress;
	data8_t *ROM = memory_region(REGION_CPU2);

	bankaddress = 0x10000-0x4000 + ((data >> 4) & 0x03) * 0x4000;
	if (bankaddress < 0x10000) bankaddress = 0x0000;

	cpu_setbank(1, &ROM[bankaddress]);
}

static WRITE_HANDLER( daitorid_sound_rombank_w )
{
	int bankaddress;
	data8_t *ROM = memory_region(REGION_CPU2);

	bankaddress = 0x10000-0x4000 + ((data >> 4) & 0x07) * 0x4000;
	if (bankaddress < 0x10000) bankaddress = 0x10000;

	cpu_setbank(1, &ROM[bankaddress]);
}


static READ_HANDLER( metro_porta_r )
{
	return porta;
}

static WRITE_HANDLER( metro_porta_w )
{
	porta = data;
}

static WRITE_HANDLER( metro_portb_w )
{
	/* port B layout:
	   7 !clock latch for message to main CPU
       6
	   5 !clock YM2413 I/O
	   4 !clock MSM6295 I/O
	   3
	   2 !enable write to YM2413/6295
	   1 select YM2151 register or data port
	   0
	*/

	if (BIT(portb,7) && !BIT(data,7))	/* clock 1->0 */
	{
		busy_sndcpu = 0;
		portb = data;
		return;
	}

	if (BIT(portb,5) && !BIT(data,5))	/* clock 1->0 */
	{
		if (!BIT(data,2))
		{
			/* write */
			if (BIT(data,1))
				YM2413_data_port_0_w(0,porta);
			else
				YM2413_register_port_0_w(0,porta);
		}
		portb = data;
		return;
	}

	if (BIT(portb,2) && !BIT(data,2))	/* clock 1->0 */
	{
		/* write */
		if (!BIT(data,4))
			OKIM6295_data_0_w(0,porta);
	}
	portb = data;
}


static WRITE_HANDLER( daitorid_portb_w )
{
	/* port B layout:
	   7 !clock latch for message to main CPU
	   6 !clock YM2151 I/O
	   5
	   4 !clock MSM6295 I/O
	   3 !enable read from YM2151/6295
	   2 !enable write to YM2151/6295
	   1 select YM2151 register or data port
	   0
	*/

	if (BIT(portb,7) && !BIT(data,7))	/* clock 1->0 */
	{
		busy_sndcpu = 0;
		portb = data;
		return;
	}

	if (BIT(portb,6) && !BIT(data,6))	/* clock 1->0 */
	{
		if (!BIT(data,2))
		{
			/* write */
			if (BIT(data,1))
				YM2151_data_port_0_w(0,porta);
			else
				YM2151_register_port_0_w(0,porta);
		}
		if (!BIT(data,3))
		{
			/* read */
			if (BIT(data,1))
				porta = YM2151_status_port_0_r(0);
		}
		portb = data;
		return;
	}

	if (BIT(portb,2) && !BIT(data,2))	/* clock 1->0 */
	{
		/* write */
		if (!BIT(data,4))
			OKIM6295_data_0_w(0,porta);
	}
	if (BIT(portb,3) && !BIT(data,3))	/* clock 1->0 */
	{
		/* read */
		if (!BIT(data,4))
			porta = OKIM6295_status_0_r(0);
	}
	portb = data;
}


/*******************/

static int okim6295_command;
static int mouja_m6295_rombank;
static UINT32 volume_table[16];		/* M6295 volume lookup table */


static void mouja_m6295_data_w(int data)
{
	/* if a command is pending, process the second half */
	if (okim6295_command != -1)
	{
		int temp = data >> 4, i, start, stop;
		data8_t *ROM = memory_region(REGION_SOUND1);

		/* determine which voice(s) (voice is set by a 1 bit in the upper 4 bits of the second byte) */
		for (i = 0; i < 4; i++, temp >>= 1)
		{
			if (temp & 1)
			{
				start = ((ROM[okim6295_command*8+0]<<16) + (ROM[okim6295_command*8+1]<<8) + ROM[okim6295_command*8+2]) & 0x3ffff;
				stop  = ((ROM[okim6295_command*8+3]<<16) + (ROM[okim6295_command*8+4]<<8) + ROM[okim6295_command*8+5]) & 0x3ffff;

				if ((start >= 0x20000) && mouja_m6295_rombank)
				{
					start += (mouja_m6295_rombank - 1) * 0x20000;			/* 0x00000-0x1ffff fixed rom  */
					stop += (mouja_m6295_rombank - 1) * 0x20000;			/* 0x20000-0x3ffff banked rom */
				}

				if (start < stop)
				{
					if (!ADPCM_playing(i))
					{
						ADPCM_setvol(i, volume_table[data & 0x0f]);
						ADPCM_play(i, start, 2 * (stop - start + 1));
					}
				}
			}
		}

		/* reset the command */
		okim6295_command = -1;
	}

	/* if this is the start of a command, remember the sample number for next time */
	else if (data & 0x80)
	{
		okim6295_command = data & 0x7f;
	}

	/* otherwise, see if this is a silence command */
	else
	{
		int temp = data >> 3, i;

		/* determine which voice(s) (voice is set by a 1 bit in bits 3-6 of the command */
		for (i = 0; i < 4; i++, temp >>= 1)
		{
			if (temp & 1)
				ADPCM_stop(i);
		}
	}
}

static WRITE16_HANDLER( mouja_sound_rombank_w )
{
	if (ACCESSING_LSB)
		mouja_m6295_rombank = (data >> 3) & 0x07;			/* M6295 special banked rom system */
}

static READ16_HANDLER( mouja_m6295_status_lsb_r )
{
	int i, result;

	result = 0xf0;	/* naname expects bits 4-7 to be 1 */
	/* set the bit to 1 if something is playing on a given channel */
	for (i = 0; i < 4; i++)
	{
		if (ADPCM_playing(i))
			result |= 1 << i;
	}

	return result;
}

static WRITE16_HANDLER( mouja_m6295_data_msb_w )
{
	if (ACCESSING_MSB)
		mouja_m6295_data_w(data >> 8);
}

/*******************/

static void metro_sound_irq_handler(int state)
{
	cpu_set_irq_line(1, UPD7810_INTF2, state ? ASSERT_LINE : CLEAR_LINE);
}

static struct YM2151interface ym2151_interface =
{
	1,
	4000000,			/* 4MHz ? */
	{ YM3012_VOL(80,MIXER_PAN_LEFT,80,MIXER_PAN_RIGHT) },
	{ metro_sound_irq_handler },	/* irq handler */
	{ 0 }							/* port_write */
};


static READ16_HANDLER( ymf278b_r )
{
	return YMF278B_status_port_0_r(0);
}

static WRITE16_HANDLER( ymf278b_w )
{
	if(ACCESSING_LSB)
		switch(offset)
		{
			case 0:
				YMF278B_control_port_0_A_w(0, data);
				break;
			case 1:
				YMF278B_data_port_0_A_w(0, data);
				break;
			case 2:
				YMF278B_control_port_0_B_w(0, data);
				break;
			case 3:
				YMF278B_data_port_0_B_w(0, data);
				break;
			case 4:
				YMF278B_control_port_0_C_w(0, data);
				break;
			case 5:
				YMF278B_data_port_0_C_w(0, data);
				break;
		}
}


static struct OKIM6295interface okim6295_interface =
{
	1,
	{ 1200000/128 },		/* 165=7272Hz? 128=9375Hz? */
	{ REGION_SOUND1 },
	{ 10 }
};

static struct OKIM6295interface okim6295_intf_2151balanced =
{
	1,
	{ 1200000/128 },		/* 165=7272Hz? 128=9375Hz? */
	{ REGION_SOUND1 },
	{ 40 }
};

static struct ADPCMinterface mouja_adpcm_interface =
{
	4,						/* 4 channels (M6295) */
	16000000/1024,			/* 15625Hz */
	REGION_SOUND1,
	{ 35,35,35,35 }
};


static struct OKIM6295interface okim6295_intf_8kHz =
{
	1,
	{ 8000 },
	{ REGION_SOUND1 },
	{ 50 }
};

static struct OKIM6295interface okim6295_intf_16kHz =
{
	1,
	{ 16000 },
	{ REGION_SOUND1 },
	{ 50 }
};


static struct YM2413interface ym2413_interface =
{
	1,
	3579545,
	{ YM2413_VOL(100,MIXER_PAN_CENTER,100,MIXER_PAN_CENTER) }		/* Insufficient gain. */
};

static struct YM2413interface ym2413_intf_8MHz =
{
	1,
	8000000,
	{ YM2413_VOL(100,MIXER_PAN_CENTER,100,MIXER_PAN_CENTER) }
};

static struct YMF278B_interface ymf278b_interface =
{
	1,
	{ YMF278B_STD_CLOCK },
	{ REGION_SOUND1 },
	{ YM3012_VOL(100, MIXER_PAN_CENTER, 100, MIXER_PAN_CENTER) },
	{ ymf278b_interrupt }
};


/***************************************************************************


								Coin Lockout


***************************************************************************/

/* IT DOESN'T WORK PROPERLY */

WRITE16_HANDLER( metro_coin_lockout_1word_w )
{
	if (ACCESSING_LSB)
	{
/*		coin_lockout_w(0, data & 1);*/
/*		coin_lockout_w(1, data & 2);*/
	}
	if (data & ~3)	log_cb(RETRO_LOG_DEBUG, LOGPRE "CPU #0 PC %06X : unknown bits of coin lockout written: %04X\n",activecpu_get_pc(),data);
}


WRITE16_HANDLER( metro_coin_lockout_4words_w )
{
/*	coin_lockout_w( (offset >> 1) & 1, offset & 1 );*/
	if (data & ~1)	log_cb(RETRO_LOG_DEBUG, LOGPRE "CPU #0 PC %06X : unknown bits of coin lockout written: %04X\n",activecpu_get_pc(),data);
}




/***************************************************************************


								Banked ROM access


***************************************************************************/

/*
	The main CPU has access to the ROMs that hold the graphics through
	a banked window of 64k. Those ROMs also usually store the tables for
	the virtual tiles set. The tile codes to be written to the tilemap
	memory to render the backgrounds are also stored here, in a format
	that the blitter can readily use (which is a form of compression)
*/

static data16_t *metro_rombank;

READ16_HANDLER( metro_bankedrom_r )
{
	const int region = REGION_GFX1;

	data8_t *ROM = memory_region( region );
	size_t  len  = memory_region_length( region );

	offset = offset * 2 + 0x10000 * (*metro_rombank);

	if ( offset < len )	return ((ROM[offset+0]<<8)+ROM[offset+1])^0xffff;
	else				return 0xffff;
}




/***************************************************************************


									Blitter

	[ Registers ]

		Offset:		Value:

		0.l			Destination Tilemap      (1,2,3)
		4.l			Blitter Data Address     (byte offset into the gfx ROMs)
		8.l			Destination Address << 7 (byte offset into the tilemap)

		The Blitter reads a byte and looks at the most significative
		bits for the opcode, while the remaining bits define a value
		(usually how many bytes to write). The opcode byte may be
		followed by a number of other bytes:

			76------			Opcode
			--543210			N
			(at most N+1 bytes follow)


		The blitter is designed to write every other byte (e.g. it
		writes a byte and skips the next). Hence 2 blits are needed
		to fill a tilemap (first even, then odd addresses)

	[ Opcodes ]

			0		Copy the following N+1 bytes. If the whole byte
					is $00:	stop and generate an IRQ

			1		Fill N+1 bytes with a sequence, starting with
					the  value in the following byte

			2		Fill N+1 bytes with the value in the following
					byte

			3		Skip N+1 bytes. If the whole byte is $C0:
					skip to the next row of the tilemap (+0x200 bytes)
					but preserve the column passed at the start of the
					blit (destination address % 0x200)


***************************************************************************/

data16_t *metro_blitter_regs;

void metro_blit_done(int param)
{
	requested_int[blitter_bit] = 1;
	update_irq_state();
}

static INLINE int blt_read(const data8_t *ROM, const int offs)
{
	return ROM[offs] ^ 0xff;
}

static INLINE void blt_write(const int tmap, const offs_t offs, const data16_t data, const data16_t mask)
{
	switch( tmap )
	{
		case 1:	metro_vram_0_w(offs,data,mask);	break;
		case 2:	metro_vram_1_w(offs,data,mask);	break;
		case 3:	metro_vram_2_w(offs,data,mask);	break;
	}
/*	log_cb(RETRO_LOG_DEBUG, LOGPRE "CPU #0 PC %06X : Blitter %X] %04X <- %04X & %04X\n",activecpu_get_pc(),tmap,offs,data,mask);*/
}


WRITE16_HANDLER( metro_blitter_w )
{
	COMBINE_DATA( &metro_blitter_regs[offset] );

	if (offset == 0xC/2)
	{
		const int region = REGION_GFX1;

		data8_t *src	=	memory_region(region);
		size_t  src_len	=	memory_region_length(region);

		UINT32 tmap		=	(metro_blitter_regs[ 0x00 / 2 ] << 16 ) +
							 metro_blitter_regs[ 0x02 / 2 ];
		UINT32 src_offs	=	(metro_blitter_regs[ 0x04 / 2 ] << 16 ) +
							 metro_blitter_regs[ 0x06 / 2 ];
		UINT32 dst_offs	=	(metro_blitter_regs[ 0x08 / 2 ] << 16 ) +
							 metro_blitter_regs[ 0x0a / 2 ];

		int shift			=	(dst_offs & 0x80) ? 0 : 8;
		data16_t mask		=	(dst_offs & 0x80) ? 0xff00 : 0x00ff;

/*		log_cb(RETRO_LOG_DEBUG, LOGPRE "CPU #0 PC %06X : Blitter regs %08X, %08X, %08X\n",activecpu_get_pc(),tmap,src_offs,dst_offs);*/

		dst_offs >>= 7+1;
		switch( tmap )
		{
			case 1:
			case 2:
			case 3:
				break;
			default:
				log_cb(RETRO_LOG_DEBUG, LOGPRE "CPU #0 PC %06X : Blitter unknown destination: %08X\n",activecpu_get_pc(),tmap);
				return;
		}

		while (1)
		{
			data16_t b1,b2,count;

			src_offs %= src_len;
			b1 = blt_read(src,src_offs);
/*			log_cb(RETRO_LOG_DEBUG, LOGPRE "CPU #0 PC %06X : Blitter opcode %02X at %06X\n",activecpu_get_pc(),b1,src_offs);*/
			src_offs++;

			count = ((~b1) & 0x3f) + 1;

			switch( (b1 & 0xc0) >> 6 )
			{
				case 0:

					/* Stop and Generate an IRQ. We can't generate it now
					   both because it's unlikely that the blitter is so
					   fast and because some games (e.g. lastfort) need to
					   complete the blitter irq service routine before doing
					   another blit. */
					if (b1 == 0)
					{
						timer_set(TIME_IN_USEC(500),0,metro_blit_done);
						return;
					}

					/* Copy */
					while (count--)
					{
						src_offs %= src_len;
						b2 = blt_read(src,src_offs) << shift;
						src_offs++;

						dst_offs &= 0xffff;
						blt_write(tmap,dst_offs,b2,mask);
						dst_offs = ((dst_offs+1) & (0x100-1)) | (dst_offs & (~(0x100-1)));
					}
					break;


				case 1:

					/* Fill with an increasing value */
					src_offs %= src_len;
					b2 = blt_read(src,src_offs);
					src_offs++;

					while (count--)
					{
						dst_offs &= 0xffff;
						blt_write(tmap,dst_offs,b2<<shift,mask);
						dst_offs = ((dst_offs+1) & (0x100-1)) | (dst_offs & (~(0x100-1)));
						b2++;
					}
					break;


				case 2:

					/* Fill with a fixed value */
					src_offs %= src_len;
					b2 = blt_read(src,src_offs) << shift;
					src_offs++;

					while (count--)
					{
						dst_offs &= 0xffff;
						blt_write(tmap,dst_offs,b2,mask);
						dst_offs = ((dst_offs+1) & (0x100-1)) | (dst_offs & (~(0x100-1)));
					}
					break;


				case 3:

					/* Skip to the next line ?? */
					if (b1 == 0xC0)
					{
						dst_offs +=   0x100;
						dst_offs &= ~(0x100-1);
						dst_offs |=  (0x100-1) & (metro_blitter_regs[ 0x0a / 2 ] >> (7+1));
					}
					else
					{
						dst_offs += count;
					}
					break;


				default:
					log_cb(RETRO_LOG_DEBUG, LOGPRE "CPU #0 PC %06X : Blitter unknown opcode %02X at %06X\n",activecpu_get_pc(),b1,src_offs-1);
					return;
			}

		}
	}

}


/***************************************************************************


								Memory Maps


***************************************************************************/

/*
 Lines starting with an empty comment in the following MemoryReadAddress
 arrays are there for debug (e.g. the game does not read from those ranges
 AFAIK)
*/


static MEMORY_READ_START( metro_snd_readmem )
    { 0x0000, 0x3fff, MRA_ROM   },	/* External ROM */
	{ 0x4000, 0x7fff, MRA_BANK1 },	/* External ROM (Banked) */
	{ 0x8000, 0x87ff, MRA_RAM   },	/* External RAM */
	{ 0xff00, 0xffff, MRA_RAM   },	/* Internal RAM */
MEMORY_END

static MEMORY_WRITE_START( metro_snd_writemem )
    { 0x0000, 0x3fff, MWA_ROM   },	/* External ROM */
	{ 0x4000, 0x7fff, MWA_BANK1 },	/* External ROM (Banked) */
	{ 0x8000, 0x87ff, MWA_RAM   },	/* External RAM */
	{ 0xff00, 0xffff, MWA_RAM   },	/* Internal RAM */
MEMORY_END

static PORT_READ_START( metro_snd_readport )
    { UPD7810_PORTA, UPD7810_PORTA, metro_porta_r },
PORT_END

static PORT_WRITE_START( metro_snd_writeport )
    { UPD7810_PORTA, UPD7810_PORTA, metro_porta_w },
	{ UPD7810_PORTB, UPD7810_PORTB, metro_portb_w },
	{ UPD7810_PORTC, UPD7810_PORTC, metro_sound_rombank_w },
PORT_END

/*****************/

static MEMORY_READ_START( daitorid_snd_readmem )
    { 0x0000, 0x3fff, MRA_ROM   },	/* External ROM */
	{ 0x4000, 0x7fff, MRA_BANK1 },	/* External ROM (Banked) */
	{ 0x8000, 0x87ff, MRA_RAM   },	/* External RAM */
	{ 0xff00, 0xffff, MRA_RAM   },	/* Internal RAM */
MEMORY_END

static MEMORY_WRITE_START( daitorid_snd_writemem )
    { 0x0000, 0x3fff, MWA_ROM   },	/* External ROM */
    { 0x4000, 0x7fff, MWA_BANK1 },	/* External ROM (Banked) */
	{ 0x8000, 0x87ff, MWA_RAM   },	/* External RAM */
	{ 0xff00, 0xffff, MWA_RAM   },	/* Internal RAM */
MEMORY_END

static PORT_READ_START( daitorid_snd_readport )
    { UPD7810_PORTA, UPD7810_PORTA, metro_porta_r },
PORT_END

static PORT_WRITE_START( daitorid_snd_writeport )
    { UPD7810_PORTA, UPD7810_PORTA, metro_porta_w },
	{ UPD7810_PORTB, UPD7810_PORTB, daitorid_portb_w },
	{ UPD7810_PORTC, UPD7810_PORTC, daitorid_sound_rombank_w },
PORT_END

/***************************************************************************
									Bal Cube
***************************************************************************/

/* Really weird way of mapping 3 DSWs */
static READ16_HANDLER( balcube_dsw_r )
{
	data16_t dsw1 = readinputport(2) >> 0;
	data16_t dsw2 = readinputport(2) >> 8;
	data16_t dsw3 = readinputport(3);

	switch (offset*2)
	{
		case 0x1FFFC:	return ((dsw1 & 0x01) ? 0x40 : 0) | ((dsw3 & 0x01) ? 0x80 : 0);
		case 0x1FFFA:	return ((dsw1 & 0x02) ? 0x40 : 0) | ((dsw3 & 0x02) ? 0x80 : 0);
		case 0x1FFF6:	return ((dsw1 & 0x04) ? 0x40 : 0) | ((dsw3 & 0x04) ? 0x80 : 0);
		case 0x1FFEE:	return ((dsw1 & 0x08) ? 0x40 : 0) | ((dsw3 & 0x08) ? 0x80 : 0);
		case 0x1FFDE:	return ((dsw1 & 0x10) ? 0x40 : 0) | ((dsw3 & 0x10) ? 0x80 : 0);
		case 0x1FFBE:	return ((dsw1 & 0x20) ? 0x40 : 0) | ((dsw3 & 0x20) ? 0x80 : 0);
		case 0x1FF7E:	return ((dsw1 & 0x40) ? 0x40 : 0) | ((dsw3 & 0x40) ? 0x80 : 0);
		case 0x1FEFE:	return ((dsw1 & 0x80) ? 0x40 : 0) | ((dsw3 & 0x80) ? 0x80 : 0);

		case 0x1FDFE:	return (dsw2 & 0x01) ? 0x40 : 0;
		case 0x1FBFE:	return (dsw2 & 0x02) ? 0x40 : 0;
		case 0x1F7FE:	return (dsw2 & 0x04) ? 0x40 : 0;
		case 0x1EFFE:	return (dsw2 & 0x08) ? 0x40 : 0;
		case 0x1DFFE:	return (dsw2 & 0x10) ? 0x40 : 0;
		case 0x1BFFE:	return (dsw2 & 0x20) ? 0x40 : 0;
		case 0x17FFE:	return (dsw2 & 0x40) ? 0x40 : 0;
		case 0x0FFFE:	return (dsw2 & 0x80) ? 0x40 : 0;
	}
	log_cb(RETRO_LOG_DEBUG, LOGPRE "CPU #0 PC %06X : unknown dsw address read: %04X\n",activecpu_get_pc(),offset);
	return 0xffff;
}


static MEMORY_READ16_START( balcube_readmem )
	{ 0x000000, 0x07ffff, MRA16_ROM				},	/* ROM*/
	{ 0xf00000, 0xf0ffff, MRA16_RAM				},	/* RAM*/
	{ 0x300000, 0x300001, ymf278b_r				},	/* Sound*/
	{ 0x400000, 0x41ffff, balcube_dsw_r			},	/* DSW x 3*/
	{ 0x600000, 0x61ffff, MRA16_RAM				},	/* Layer 0*/
	{ 0x620000, 0x63ffff, MRA16_RAM				},	/* Layer 1*/
	{ 0x640000, 0x65ffff, MRA16_RAM				},	/* Layer 2*/
	{ 0x660000, 0x66ffff, metro_bankedrom_r		},	/* Banked ROM*/
	{ 0x670000, 0x673fff, MRA16_RAM				},	/* Palette*/
	{ 0x674000, 0x674fff, MRA16_RAM				},	/* Sprites*/
	{ 0x678000, 0x6787ff, MRA16_RAM				},	/* Tiles Set*/
	{ 0x6788a2, 0x6788a3, metro_irq_cause_r		},	/* IRQ Cause*/
	{ 0x500000, 0x500001, input_port_0_word_r	},	/* Inputs*/
	{ 0x500002, 0x500003, input_port_1_word_r	},	/**/
	{ 0x500006, 0x500007, MRA16_NOP				},	/**/
MEMORY_END

static MEMORY_WRITE16_START( balcube_writemem )
	{ 0x000000, 0x07ffff, MWA16_ROM						},	/* ROM*/
	{ 0xf00000, 0xf0ffff, MWA16_RAM						},	/* RAM*/
	{ 0x300000, 0x30000b, ymf278b_w						},	/* Sound*/
	{ 0x500002, 0x500009, metro_coin_lockout_4words_w	},	/* Coin Lockout*/
	{ 0x670000, 0x673fff, metro_paletteram_w, &paletteram16	},	/* Palette*/
	{ 0x674000, 0x674fff, MWA16_RAM, &spriteram16, &spriteram_size				},	/* Sprites*/
	{ 0x600000, 0x61ffff, metro_vram_0_w, &metro_vram_0	},	/* Layer 0*/
	{ 0x620000, 0x63ffff, metro_vram_1_w, &metro_vram_1	},	/* Layer 1*/
	{ 0x640000, 0x65ffff, metro_vram_2_w, &metro_vram_2	},	/* Layer 2*/
	{ 0x678000, 0x6787ff, MWA16_RAM, &metro_tiletable, &metro_tiletable_size		},	/* Tiles Set*/
	{ 0x678840, 0x67884d, metro_blitter_w, &metro_blitter_regs		},	/* Tiles Blitter*/
	{ 0x678860, 0x67886b, metro_window_w, &metro_window				},	/* Tilemap Window*/
	{ 0x678870, 0x67887b, MWA16_RAM, &metro_scroll		},	/* Scroll*/
	{ 0x678880, 0x678881, MWA16_NOP						},	/* ? increasing*/
	{ 0x678890, 0x678891, MWA16_NOP						},	/* ? increasing*/
	{ 0x6788a2, 0x6788a3, metro_irq_cause_w				},	/* IRQ Acknowledge*/
	{ 0x6788a4, 0x6788a5, MWA16_RAM, &metro_irq_enable	},	/* IRQ Enable*/
	{ 0x6788aa, 0x6788ab, MWA16_RAM, &metro_rombank		},	/* Rom Bank*/
	{ 0x6788ac, 0x6788ad, MWA16_RAM, &metro_screenctrl	},	/* Screen Control*/
	{ 0x679700, 0x679713, MWA16_RAM, &metro_videoregs	},	/* Video Registers*/
MEMORY_END


/***************************************************************************
								Bang Bang Ball
***************************************************************************/

static MEMORY_READ16_START( bangball_readmem )
	{ 0x000000, 0x07ffff, MRA16_ROM				},	/* ROM*/
	{ 0xf00000, 0xf0ffff, MRA16_RAM				},	/* RAM*/
	{ 0xf10000, 0xf10fff, MRA16_RAM				},	/* RAM (bug in the ram test routine)*/
	{ 0xb00000, 0xb00001, ymf278b_r				},	/* Sound*/
	{ 0xc00000, 0xc1ffff, balcube_dsw_r			},	/* DSW x 3*/
	{ 0xd00000, 0xd00001, input_port_0_word_r	},	/* Inputs*/
	{ 0xd00002, 0xd00003, input_port_1_word_r	},	/**/
	{ 0xd00006, 0xd00007, MRA16_NOP				},	/**/
	{ 0xe00000, 0xe1ffff, MRA16_RAM				},	/* Layer 0*/
	{ 0xe20000, 0xe3ffff, MRA16_RAM				},	/* Layer 1*/
	{ 0xe40000, 0xe5ffff, MRA16_RAM				},	/* Layer 2*/
	{ 0xe60000, 0xe6ffff, metro_bankedrom_r		},	/* Banked ROM*/
	{ 0xe70000, 0xe73fff, MRA16_RAM				},	/* Palette*/
	{ 0xe74000, 0xe74fff, MRA16_RAM				},	/* Sprites*/
	{ 0xe78000, 0xe787ff, MRA16_RAM				},	/* Tiles Set*/
	{ 0xe788a2, 0xe788a3, metro_irq_cause_r		},	/* IRQ Cause*/
MEMORY_END

static MEMORY_WRITE16_START( bangball_writemem )
	{ 0x000000, 0x07ffff, MWA16_ROM						},	/* ROM*/
	{ 0xf00000, 0xf0ffff, MWA16_RAM						},	/* RAM*/
	{ 0xf10000, 0xf10fff, MWA16_RAM						},	/* RAM*/
	{ 0xb00000, 0xb0000b, ymf278b_w						},	/* Sound*/
	{ 0xd00002, 0xd00009, metro_coin_lockout_4words_w	},	/* Coin Lockout*/
	{ 0xe00000, 0xe1ffff, metro_vram_0_w, &metro_vram_0	},	/* Layer 0*/
	{ 0xe20000, 0xe3ffff, metro_vram_1_w, &metro_vram_1	},	/* Layer 1*/
	{ 0xe40000, 0xe5ffff, metro_vram_2_w, &metro_vram_2	},	/* Layer 2*/
	{ 0xe70000, 0xe73fff, metro_paletteram_w, &paletteram16	},	/* Palette*/
	{ 0xe74000, 0xe74fff, MWA16_RAM, &spriteram16, &spriteram_size				},	/* Sprites*/
	{ 0xe78000, 0xe787ff, MWA16_RAM, &metro_tiletable, &metro_tiletable_size		},	/* Tiles Set*/
	{ 0xe78840, 0xe7884d, metro_blitter_w, &metro_blitter_regs		},	/* Tiles Blitter*/
	{ 0xe78860, 0xe7886b, metro_window_w, &metro_window				},	/* Tilemap Window*/
	{ 0xe78870, 0xe7887b, MWA16_RAM, &metro_scroll		},	/* Scroll*/
	{ 0xe78880, 0xe78881, MWA16_NOP						},	/* ? increasing*/
	{ 0xe78890, 0xe78891, MWA16_NOP						},	/* ? increasing*/
	{ 0xe788a2, 0xe788a3, metro_irq_cause_w				},	/* IRQ Acknowledge*/
	{ 0xe788a4, 0xe788a5, MWA16_RAM, &metro_irq_enable	},	/* IRQ Enable*/
	{ 0xe788aa, 0xe788ab, MWA16_RAM, &metro_rombank		},	/* Rom Bank*/
	{ 0xe788ac, 0xe788ad, MWA16_RAM, &metro_screenctrl	},	/* Screen Control*/
	{ 0xe79700, 0xe79713, MWA16_RAM, &metro_videoregs	},	/* Video Registers*/
MEMORY_END


/***************************************************************************
								Dai Toride
***************************************************************************/

static MEMORY_READ16_START( daitorid_readmem )
	{ 0x000000, 0x03ffff, MRA16_ROM				},	/* ROM*/
	{ 0x800000, 0x80ffff, MRA16_RAM				},	/* RAM*/
	{ 0x400000, 0x41ffff, MRA16_RAM				},	/* Layer 0*/
	{ 0x420000, 0x43ffff, MRA16_RAM				},	/* Layer 1*/
	{ 0x440000, 0x45ffff, MRA16_RAM				},	/* Layer 2*/
	{ 0x460000, 0x46ffff, metro_bankedrom_r		},	/* Banked ROM*/
	{ 0x470000, 0x473fff, MRA16_RAM				},	/* Palette*/
	{ 0x474000, 0x474fff, MRA16_RAM				},	/* Sprites*/
	{ 0x478000, 0x4787ff, MRA16_RAM				},	/* Tiles Set*/
	{ 0x4788a2, 0x4788a3, metro_irq_cause_r		},	/* IRQ Cause*/
	{ 0xc00000, 0xc00001, daitorid_soundstatus_r	},	/* Inputs*/
	{ 0xc00002, 0xc00003, input_port_1_word_r	},	/**/
	{ 0xc00004, 0xc00005, input_port_2_word_r	},	/**/
	{ 0xc00006, 0xc00007, input_port_3_word_r	},	/**/
MEMORY_END

static MEMORY_WRITE16_START( daitorid_writemem )
	{ 0x000000, 0x03ffff, MWA16_ROM						},	/* ROM*/
	{ 0x800000, 0x80ffff, MWA16_RAM						},	/* RAM*/
	{ 0x400000, 0x41ffff, metro_vram_0_w, &metro_vram_0	},	/* Layer 0*/
	{ 0x420000, 0x43ffff, metro_vram_1_w, &metro_vram_1	},	/* Layer 1*/
	{ 0x440000, 0x45ffff, metro_vram_2_w, &metro_vram_2	},	/* Layer 2*/
	{ 0x470000, 0x473fff, metro_paletteram_w, &paletteram16	},	/* Palette*/
	{ 0x474000, 0x474fff, MWA16_RAM, &spriteram16, &spriteram_size				},	/* Sprites*/
	{ 0x478000, 0x4787ff, MWA16_RAM, &metro_tiletable, &metro_tiletable_size	},	/* Tiles Set*/
	{ 0x478840, 0x47884d, metro_blitter_w, &metro_blitter_regs	},	/* Tiles Blitter*/
	{ 0x478860, 0x47886b, metro_window_w, &metro_window			},	/* Tilemap Window*/
	{ 0x478870, 0x47887b, MWA16_RAM, &metro_scroll		},	/* Scroll*/
	{ 0x478880, 0x478881, MWA16_NOP						},	/* ? increasing*/
	{ 0x478890, 0x478891, MWA16_NOP						},	/* ? increasing*/
	{ 0x4788a2, 0x4788a3, metro_irq_cause_w				},	/* IRQ Acknowledge*/
	{ 0x4788a4, 0x4788a5, MWA16_RAM, &metro_irq_enable	},	/* IRQ Enable*/
	{ 0x4788a8, 0x4788a9, metro_soundlatch_w			},	/* To Sound CPU*/
	{ 0x4788aa, 0x4788ab, MWA16_RAM, &metro_rombank		},	/* Rom Bank*/
	{ 0x4788ac, 0x4788ad, MWA16_RAM, &metro_screenctrl	},	/* Screen Control*/
	{ 0x479700, 0x479713, MWA16_RAM, &metro_videoregs	},	/* Video Registers*/
	{ 0xc00000, 0xc00001, metro_soundstatus_w			},	/* To Sound CPU*/
	{ 0xc00002, 0xc00009, metro_coin_lockout_4words_w	},	/* Coin Lockout*/
MEMORY_END


/***************************************************************************
								Dharma Doujou
***************************************************************************/

static MEMORY_READ16_START( dharma_readmem )
	{ 0x000000, 0x03ffff, MRA16_ROM				},	/* ROM*/
	{ 0x400000, 0x40ffff, MRA16_RAM				},	/* RAM*/
	{ 0x800000, 0x81ffff, MRA16_RAM				},	/* Layer 0*/
	{ 0x820000, 0x83ffff, MRA16_RAM				},	/* Layer 1*/
	{ 0x840000, 0x85ffff, MRA16_RAM				},	/* Layer 2*/
	{ 0x860000, 0x86ffff, metro_bankedrom_r		},	/* Banked ROM*/
	{ 0x870000, 0x873fff, MRA16_RAM				},	/* Palette*/
	{ 0x874000, 0x874fff, MRA16_RAM				},	/* Sprites*/
	{ 0x878000, 0x8787ff, MRA16_RAM				},	/* Tiles Set*/
	{ 0x8788a2, 0x8788a3, metro_irq_cause_r		},	/* IRQ Cause*/
	{ 0xc00000, 0xc00001, daitorid_soundstatus_r	},	/* Inputs*/
	{ 0xc00002, 0xc00003, input_port_1_word_r	},	/**/
	{ 0xc00004, 0xc00005, input_port_2_word_r	},	/**/
	{ 0xc00006, 0xc00007, input_port_3_word_r	},	/**/
MEMORY_END

static MEMORY_WRITE16_START( dharma_writemem )
	{ 0x000000, 0x03ffff, MWA16_ROM						},	/* ROM*/
	{ 0x400000, 0x40ffff, MWA16_RAM						},	/* RAM*/
	{ 0x800000, 0x81ffff, metro_vram_0_w, &metro_vram_0	},	/* Layer 0*/
	{ 0x820000, 0x83ffff, metro_vram_1_w, &metro_vram_1	},	/* Layer 1*/
	{ 0x840000, 0x85ffff, metro_vram_2_w, &metro_vram_2	},	/* Layer 2*/
	{ 0x870000, 0x873fff, metro_paletteram_w, &paletteram16	},	/* Palette*/
	{ 0x874000, 0x874fff, MWA16_RAM, &spriteram16, &spriteram_size				},	/* Sprites*/
	{ 0x878000, 0x8787ff, MWA16_RAM, &metro_tiletable, &metro_tiletable_size	},	/* Tiles Set*/
	{ 0x878840, 0x87884d, metro_blitter_w, &metro_blitter_regs	},	/* Tiles Blitter*/
	{ 0x878860, 0x87886b, metro_window_w, &metro_window	},	/* Tilemap Window*/
	{ 0x878870, 0x87887b, MWA16_RAM, &metro_scroll		},	/* Scroll Regs*/
	{ 0x878880, 0x878881, MWA16_NOP						},	/* ? increasing*/
	{ 0x878890, 0x878891, MWA16_NOP						},	/* ? increasing*/
	{ 0x8788a2, 0x8788a3, metro_irq_cause_w				},	/* IRQ Acknowledge*/
	{ 0x8788a4, 0x8788a5, MWA16_RAM, &metro_irq_enable	},	/* IRQ Enable*/
	{ 0x8788a8, 0x8788a9, metro_soundlatch_w			},	/* To Sound CPU*/
	{ 0x8788aa, 0x8788ab, MWA16_RAM, &metro_rombank		},	/* Rom Bank*/
	{ 0x8788ac, 0x8788ad, MWA16_RAM, &metro_screenctrl	},	/* Screen Control*/
	{ 0x879700, 0x879713, MWA16_RAM, &metro_videoregs	},	/* Video Registers*/
	{ 0xc00000, 0xc00001, metro_soundstatus_w			},	/* To Sound CPU*/
	{ 0xc00002, 0xc00009, metro_coin_lockout_4words_w	},	/* Coin Lockout*/
MEMORY_END


/***************************************************************************
								Karate Tournament
***************************************************************************/

/* This game uses almost only the blitter to write to the tilemaps.
   The CPU can only access a "window" of 512x256 pixels in the upper
   left corner of the big tilemap */

#define KARATOUR_OFFS( _x_ ) ((_x_) & (0x3f)) + (((_x_) & ~(0x3f)) * (0x100 / 0x40))

#define KARATOUR_VRAM( _n_ ) \
static READ16_HANDLER( karatour_vram_##_n_##_r ) \
{ \
	return metro_vram_##_n_[KARATOUR_OFFS(offset)]; \
} \
static WRITE16_HANDLER( karatour_vram_##_n_##_w ) \
{ \
	metro_vram_##_n_##_w(KARATOUR_OFFS(offset),data,mem_mask); \
}

KARATOUR_VRAM( 0 )
KARATOUR_VRAM( 1 )
KARATOUR_VRAM( 2 )

static MEMORY_READ16_START( karatour_readmem )
	{ 0x000000, 0x07ffff, MRA16_ROM				},	/* ROM*/
	{ 0xffc000, 0xffffff, MRA16_RAM				},	/* RAM*/
	{ 0x400000, 0x400001, metro_soundstatus_r	},	/* From Sound CPU*/
	{ 0x400002, 0x400003, input_port_0_word_r	},	/* Inputs*/
	{ 0x400004, 0x400005, input_port_1_word_r	},	/**/
	{ 0x400006, 0x400007, input_port_2_word_r	},	/**/
	{ 0x40000a, 0x40000b, input_port_3_word_r	},	/**/
	{ 0x40000c, 0x40000d, input_port_4_word_r	},	/**/
	{ 0x860000, 0x86ffff, metro_bankedrom_r		},	/* Banked ROM*/
	{ 0x870000, 0x873fff, MRA16_RAM				},	/* Palette*/
	{ 0x874000, 0x874fff, MRA16_RAM				},	/* Sprites*/
	{ 0x875000, 0x875fff, karatour_vram_0_r		},	/* Layer 0 (Part of)*/
	{ 0x876000, 0x876fff, karatour_vram_1_r		},	/* Layer 1 (Part of)*/
	{ 0x877000, 0x877fff, karatour_vram_2_r		},	/* Layer 2 (Part of)*/
	{ 0x878000, 0x8787ff, MRA16_RAM				},	/* Tiles Set*/
	{ 0x8788a2, 0x8788a3, metro_irq_cause_r		},	/* IRQ Cause*/
MEMORY_END

static MEMORY_WRITE16_START( karatour_writemem )
	{ 0x000000, 0x07ffff, MWA16_ROM						},	/* ROM*/
	{ 0xffc000, 0xffffff, MWA16_RAM						},	/* RAM*/
	{ 0x400000, 0x400001, metro_soundstatus_w			},	/* To Sound CPU*/
	{ 0x400002, 0x400003, metro_coin_lockout_1word_w	},	/* Coin Lockout*/
	{ 0x870000, 0x873fff, metro_paletteram_w, &paletteram16	},	/* Palette*/
	{ 0x874000, 0x874fff, MWA16_RAM, &spriteram16, &spriteram_size				},	/* Sprites*/
	{ 0x875000, 0x875fff, karatour_vram_0_w				},	/* Layer 0 (Part of)*/
	{ 0x876000, 0x876fff, karatour_vram_1_w				},	/* Layer 1 (Part of)*/
	{ 0x877000, 0x877fff, karatour_vram_2_w				},	/* Layer 2 (Part of)*/
	{ 0x878000, 0x8787ff, MWA16_RAM, &metro_tiletable, &metro_tiletable_size	},	/* Tiles Set*/
	{ 0x878800, 0x878813, MWA16_RAM, &metro_videoregs	},	/* Video Registers*/
	{ 0x878840, 0x87884d, metro_blitter_w, &metro_blitter_regs	},	/* Tiles Blitter*/
	{ 0x878860, 0x87886b, metro_window_w, &metro_window	},	/* Tilemap Window*/
	{ 0x878870, 0x87887b, MWA16_RAM, &metro_scroll		},	/* Scroll*/
	{ 0x878880, 0x878881, MWA16_NOP						},	/* ? increasing*/
	{ 0x878890, 0x878891, MWA16_NOP						},	/* ? increasing*/
	{ 0x8788a2, 0x8788a3, metro_irq_cause_w				},	/* IRQ Acknowledge*/
	{ 0x8788a4, 0x8788a5, MWA16_RAM, &metro_irq_enable	},	/* IRQ Enable*/
	{ 0x8788a8, 0x8788a9, metro_soundlatch_w			},	/* To Sound CPU*/
	{ 0x8788aa, 0x8788ab, MWA16_RAM, &metro_rombank		},	/* Rom Bank*/
	{ 0x8788ac, 0x8788ad, MWA16_RAM, &metro_screenctrl	},	/* Screen Control*/
MEMORY_END


/***************************************************************************
								Sankokushi
***************************************************************************/

/* same limited tilemap access as karatour */

static MEMORY_READ16_START( kokushi_readmem )
	{ 0x000000, 0x07ffff, MRA16_ROM				},	/* ROM*/
	{ 0x7fc000, 0x7fffff, MRA16_RAM				},	/* RAM*/
	{ 0x860000, 0x86ffff, metro_bankedrom_r		},	/* Banked ROM*/
	{ 0x870000, 0x873fff, MRA16_RAM				},	/* Palette*/
	{ 0x874000, 0x874fff, MRA16_RAM				},	/* Sprites*/
	{ 0x875000, 0x875fff, karatour_vram_0_r		},	/* Layer 0 (Part of)*/
	{ 0x876000, 0x876fff, karatour_vram_1_r		},	/* Layer 1 (Part of)*/
	{ 0x877000, 0x877fff, karatour_vram_2_r		},	/* Layer 2 (Part of)*/
	{ 0x878000, 0x8787ff, MRA16_RAM				},	/* Tiles Set*/
	{ 0x8788a2, 0x8788a3, metro_irq_cause_r		},	/* IRQ Cause*/
	{ 0xc00000, 0xc00001, daitorid_soundstatus_r	},	/* From Sound CPU*/
	{ 0xc00002, 0xc00003, input_port_1_word_r	},	/* Inputs*/
	{ 0xc00004, 0xc00005, input_port_2_word_r	},	/**/
MEMORY_END

static MEMORY_WRITE16_START( kokushi_writemem )
	{ 0x000000, 0x07ffff, MWA16_ROM						},	/* ROM*/
	{ 0x7fc000, 0x7fffff, MWA16_RAM						},	/* RAM*/
	{ 0x870000, 0x873fff, metro_paletteram_w, &paletteram16	},	/* Palette*/
	{ 0x874000, 0x874fff, MWA16_RAM, &spriteram16, &spriteram_size				},	/* Sprites*/
	{ 0x875000, 0x875fff, karatour_vram_0_w				},	/* Layer 0 (Part of)*/
	{ 0x876000, 0x876fff, karatour_vram_1_w				},	/* Layer 1 (Part of)*/
	{ 0x877000, 0x877fff, karatour_vram_2_w				},	/* Layer 2 (Part of)*/
	{ 0x878000, 0x8787ff, MWA16_RAM, &metro_tiletable, &metro_tiletable_size	},	/* Tiles Set*/
	{ 0x878840, 0x87884d, metro_blitter_w, &metro_blitter_regs	},	/* Tiles Blitter*/
	{ 0x878860, 0x87886b, metro_window_w, &metro_window	},	/* Tilemap Window*/
	{ 0x878870, 0x87887b, MWA16_RAM, &metro_scroll		},	/* Scroll Regs - WRONG*/
/*	{ 0x878880, 0x878881, MWA16_NOP						},	*/ /* ? increasing*/
	{ 0x878890, 0x878891, MWA16_NOP						},	/* ? increasing*/
	{ 0x8788a2, 0x8788a3, metro_irq_cause_w				},	/* IRQ Acknowledge*/
	{ 0x8788a4, 0x8788a5, MWA16_RAM, &metro_irq_enable	},	/* IRQ Enable*/
	{ 0x8788a8, 0x8788a9, metro_soundlatch_w			},	/* To Sound CPU*/
	{ 0x8788aa, 0x8788ab, MWA16_RAM, &metro_rombank		},	/* Rom Bank*/
	{ 0x8788ac, 0x8788ad, MWA16_RAM, &metro_screenctrl	},	/* Screen Control*/
	{ 0x879700, 0x879713, MWA16_RAM, &metro_videoregs	},	/* Video Registers*/
	{ 0xc00000, 0xc00001, metro_soundstatus_w			},	/* To Sound CPU*/
	{ 0xc00002, 0xc00009, metro_coin_lockout_4words_w	},	/* Coin Lockout*/
MEMORY_END



/***************************************************************************
								Last Fortress
***************************************************************************/

static MEMORY_READ16_START( lastfort_readmem )
	{ 0x000000, 0x03ffff, MRA16_ROM				},	/* ROM*/
	{ 0x400000, 0x40ffff, MRA16_RAM				},	/* RAM*/
	{ 0x800000, 0x81ffff, MRA16_RAM				},	/* Layer 0*/
	{ 0x820000, 0x83ffff, MRA16_RAM				},	/* Layer 1*/
	{ 0x840000, 0x85ffff, MRA16_RAM				},	/* Layer 2*/
	{ 0x860000, 0x86ffff, metro_bankedrom_r		},	/* Banked ROM*/
	{ 0x870000, 0x873fff, MRA16_RAM				},	/* Palette*/
	{ 0x874000, 0x874fff, MRA16_RAM				},	/* Sprites*/
	{ 0x878000, 0x8787ff, MRA16_RAM				},	/* Tiles Set*/
	{ 0x8788a2, 0x8788a3, metro_irq_cause_r		},	/* IRQ Cause*/
	{ 0xc00000, 0xc00001, metro_soundstatus_r	},	/* From Sound CPU*/
	{ 0xc00002, 0xc00003, MRA16_NOP				},	/**/
	{ 0xc00004, 0xc00005, input_port_0_word_r	},	/* Inputs*/
	{ 0xc00006, 0xc00007, input_port_1_word_r	},	/**/
	{ 0xc00008, 0xc00009, input_port_2_word_r	},	/**/
	{ 0xc0000a, 0xc0000b, input_port_3_word_r	},	/**/
	{ 0xc0000c, 0xc0000d, input_port_4_word_r	},	/**/
	{ 0xc0000e, 0xc0000f, input_port_5_word_r	},	/**/
MEMORY_END

static MEMORY_WRITE16_START( lastfort_writemem )
	{ 0x000000, 0x03ffff, MWA16_ROM						},	/* ROM*/
	{ 0x400000, 0x40ffff, MWA16_RAM						},	/* RAM*/
	{ 0x800000, 0x81ffff, metro_vram_0_w, &metro_vram_0	},	/* Layer 0*/
	{ 0x820000, 0x83ffff, metro_vram_1_w, &metro_vram_1	},	/* Layer 1*/
	{ 0x840000, 0x85ffff, metro_vram_2_w, &metro_vram_2	},	/* Layer 2*/
	{ 0x870000, 0x873fff, metro_paletteram_w, &paletteram16	},	/* Palette*/
	{ 0x874000, 0x874fff, MWA16_RAM, &spriteram16, &spriteram_size				},	/* Sprites*/
	{ 0x878000, 0x8787ff, MWA16_RAM, &metro_tiletable, &metro_tiletable_size	},	/* Tiles Set*/
	{ 0x878800, 0x878813, MWA16_RAM, &metro_videoregs	},	/* Video Registers*/
	{ 0x878840, 0x87884d, metro_blitter_w, &metro_blitter_regs	},	/* Tiles Blitter*/
	{ 0x878860, 0x87886b, metro_window_w, &metro_window	},	/* Tilemap Window*/
	{ 0x878870, 0x87887b, MWA16_RAM, &metro_scroll		},	/* Scroll*/
	{ 0x878880, 0x878881, MWA16_NOP						},	/* ? increasing*/
	{ 0x878890, 0x878891, MWA16_NOP						},	/* ? increasing*/
	{ 0x8788a2, 0x8788a3, metro_irq_cause_w				},	/* IRQ Acknowledge*/
	{ 0x8788a4, 0x8788a5, MWA16_RAM, &metro_irq_enable	},	/* IRQ Enable*/
	{ 0x8788a8, 0x8788a9, metro_soundlatch_w			},	/* To Sound CPU*/
	{ 0x8788aa, 0x8788ab, MWA16_RAM, &metro_rombank		},	/* Rom Bank*/
	{ 0x8788ac, 0x8788ad, MWA16_RAM, &metro_screenctrl	},	/* Screen Control*/
	{ 0xc00000, 0xc00001, metro_soundstatus_w			},	/* To Sound CPU*/
	{ 0xc00002, 0xc00003, metro_coin_lockout_1word_w	},	/* Coin Lockout*/
MEMORY_END


/***************************************************************************
								Mahjong Gakuensai
***************************************************************************/

static int gakusai_oki_bank_lo, gakusai_oki_bank_hi;

void gakusai_oki_bank_set(void)
{
	int bank = (gakusai_oki_bank_lo & 7) + (gakusai_oki_bank_hi & 1) * 8;
	OKIM6295_set_bank_base(0, bank * 0x40000);
}

static WRITE16_HANDLER( gakusai_oki_bank_hi_w )
{
	if (ACCESSING_LSB)
	{
		gakusai_oki_bank_hi = data & 0xff;
		gakusai_oki_bank_set();
	}
}

static WRITE16_HANDLER( gakusai_oki_bank_lo_w )
{
	if (ACCESSING_LSB)
	{
		gakusai_oki_bank_lo = data & 0xff;
		gakusai_oki_bank_set();
	}
}

static data16_t *gakusai_input_sel;

static READ16_HANDLER( gakusai_input_r )
{
	data16_t input_sel = (*gakusai_input_sel) ^ 0x3e;
	/* Bit 0 ??*/
	if (input_sel & 0x0002)	return readinputport(0);
	if (input_sel & 0x0004)	return readinputport(1);
	if (input_sel & 0x0008)	return readinputport(2);
	if (input_sel & 0x0010)	return readinputport(3);
	if (input_sel & 0x0020)	return readinputport(4);
	return 0xffff;
}

READ16_HANDLER( gakusai_eeprom_r )
{
	return EEPROM_read_bit() & 1;
}

WRITE16_HANDLER( gakusai_eeprom_w )
{
	if (ACCESSING_LSB)
	{
		/* latch the bit*/
		EEPROM_write_bit(data & 0x01);

		/* reset line asserted: reset.*/
		EEPROM_set_cs_line((data & 0x04) ? CLEAR_LINE : ASSERT_LINE );

		/* clock line asserted: write latch or select next bit to read*/
		EEPROM_set_clock_line((data & 0x02) ? ASSERT_LINE : CLEAR_LINE );
	}
}

static MEMORY_READ16_START( gakusai_readmem )
	{ 0x000000, 0x07ffff, MRA16_ROM					},	/* ROM*/
	{ 0xff0000, 0xffffff, MRA16_RAM					},	/* RAM*/
	{ 0x200000, 0x21ffff, MRA16_RAM					},	/* Layer 0*/
	{ 0x220000, 0x23ffff, MRA16_RAM					},	/* Layer 1*/
	{ 0x240000, 0x25ffff, MRA16_RAM					},	/* Layer 2*/
	{ 0x260000, 0x26ffff, metro_bankedrom_r			},	/* Banked ROM*/
	{ 0x270000, 0x273fff, MRA16_RAM					},	/* Palette*/
	{ 0x274000, 0x274fff, MRA16_RAM					},	/* Sprites*/
	{ 0x278000, 0x2787ff, MRA16_RAM					},	/* Tiles Set*/
	{ 0x278832, 0x278833, metro_irq_cause_r			},	/* IRQ Cause*/
	{ 0x278880, 0x278881, gakusai_input_r			},	/* Inputs*/
	{ 0x278882, 0x278883, input_port_5_word_r		},	/**/
	{ 0x27880e, 0x27880f, MRA16_RAM					},	/* Screen Control*/
	{ 0x700000, 0x700001, OKIM6295_status_0_lsb_r	},	/* Sound*/
	{ 0xc00000, 0xc00001, gakusai_eeprom_r			},	/* EEPROM*/
MEMORY_END

static MEMORY_WRITE16_START( gakusai_writemem )
	{ 0x000000, 0x07ffff, MWA16_ROM						},	/* ROM*/
	{ 0xff0000, 0xffffff, MWA16_RAM						},	/* RAM*/
	{ 0x200000, 0x21ffff, metro_vram_0_w, &metro_vram_0	},	/* Layer 0*/
	{ 0x220000, 0x23ffff, metro_vram_1_w, &metro_vram_1	},	/* Layer 1*/
	{ 0x240000, 0x25ffff, metro_vram_2_w, &metro_vram_2	},	/* Layer 2*/
	{ 0x270000, 0x273fff, metro_paletteram_w, &paletteram16	},	/* Palette*/
	{ 0x274000, 0x274fff, MWA16_RAM, &spriteram16, &spriteram_size				},	/* Sprites*/
	{ 0x278000, 0x2787ff, MWA16_RAM, &metro_tiletable, &metro_tiletable_size	},	/* Tiles Set*/
	{ 0x27880e, 0x27880f, MWA16_RAM, &metro_screenctrl	},	/* Screen Control*/
	{ 0x278810, 0x27881f, MWA16_RAM, &metro_irq_levels	},	/* IRQ Levels*/
	{ 0x278820, 0x27882f, MWA16_RAM, &metro_irq_vectors	},	/* IRQ Vectors*/
	{ 0x278830, 0x278831, MWA16_RAM, &metro_irq_enable	},	/* IRQ Enable*/
	{ 0x278832, 0x278833, metro_irq_cause_w				},	/* IRQ Acknowledge*/
	{ 0x278836, 0x278837, watchdog_reset16_w			},	/* Watchdog*/
	{ 0x278840, 0x27884d, metro_blitter_w, &metro_blitter_regs	},	/* Tiles Blitter*/
	{ 0x278860, 0x27886b, metro_window_w, &metro_window	},	/* Tilemap Window*/
	{ 0x278850, 0x27885b, MWA16_RAM, &metro_scroll		},	/* Scroll Regs*/
	{ 0x278870, 0x278871, MWA16_RAM, &metro_rombank		},	/* Rom Bank*/
	{ 0x278888, 0x278889, MWA16_RAM, &gakusai_input_sel	},	/* Inputs*/
	{ 0x279700, 0x279713, MWA16_RAM, &metro_videoregs	},	/* Video Registers*/
	{ 0x400000, 0x400001, MWA16_NOP						},	/* ? 5*/
	{ 0x500000, 0x500001, gakusai_oki_bank_lo_w			},	/* Sound*/
	{ 0x600000, 0x600001, YM2413_register_port_0_lsb_w	},
	{ 0x600002, 0x600003, YM2413_data_port_0_lsb_w		},
	{ 0x700000, 0x700001, OKIM6295_data_0_lsb_w 		},
	{ 0xc00000, 0xc00001, gakusai_eeprom_w				},	/* EEPROM*/
	{ 0xd00000, 0xd00001, gakusai_oki_bank_hi_w			},
MEMORY_END


/***************************************************************************
								Mahjong Gakuensai 2
***************************************************************************/

static MEMORY_READ16_START( gakusai2_readmem )
	{ 0x000000, 0x07ffff, MRA16_ROM					},	/* ROM*/
	{ 0xff0000, 0xffffff, MRA16_RAM					},	/* RAM*/
	{ 0x600000, 0x61ffff, MRA16_RAM					},	/* Layer 0*/
	{ 0x620000, 0x63ffff, MRA16_RAM					},	/* Layer 1*/
	{ 0x640000, 0x65ffff, MRA16_RAM					},	/* Layer 2*/
	{ 0x660000, 0x66ffff, metro_bankedrom_r			},	/* Banked ROM*/
	{ 0x670000, 0x673fff, MRA16_RAM					},	/* Palette*/
	{ 0x674000, 0x674fff, MRA16_RAM					},	/* Sprites*/
	{ 0x675000, 0x675fff, MRA16_RAM					},	/* Sprites?*/
	{ 0x678000, 0x6787ff, MRA16_RAM					},	/* Tiles Set*/
	{ 0x678832, 0x678833, metro_irq_cause_r			},	/* IRQ Cause*/
	{ 0x678880, 0x678881, gakusai_input_r			},	/* Inputs*/
	{ 0x678882, 0x678883, input_port_5_word_r		},	/**/
	{ 0x67880e, 0x67880f, MRA16_RAM					},	/* Screen Control*/
	{ 0xb00000, 0xb00001, OKIM6295_status_0_lsb_r	},	/* Sound*/
	{ 0xe00000, 0xe00001, gakusai_eeprom_r			},	/* EEPROM*/
MEMORY_END

static MEMORY_WRITE16_START( gakusai2_writemem )
	{ 0x000000, 0x07ffff, MWA16_ROM						},	/* ROM*/
	{ 0xff0000, 0xffffff, MWA16_RAM						},	/* RAM*/
	{ 0x600000, 0x61ffff, metro_vram_0_w, &metro_vram_0	},	/* Layer 0*/
	{ 0x620000, 0x63ffff, metro_vram_1_w, &metro_vram_1	},	/* Layer 1*/
	{ 0x640000, 0x65ffff, metro_vram_2_w, &metro_vram_2	},	/* Layer 2*/
	{ 0x670000, 0x673fff, metro_paletteram_w, &paletteram16	},	/* Palette*/
	{ 0x674000, 0x674fff, MWA16_RAM, &spriteram16, &spriteram_size				},	/* Sprites*/
	{ 0x675000, 0x675fff, MWA16_RAM						},	/* Sprites?*/
	{ 0x678000, 0x6787ff, MWA16_RAM, &metro_tiletable, &metro_tiletable_size	},	/* Tiles Set*/
	{ 0x67880e, 0x67880f, MWA16_RAM, &metro_screenctrl	},	/* Screen Control*/
	{ 0x678810, 0x67881f, MWA16_RAM, &metro_irq_levels	},	/* IRQ Levels*/
	{ 0x678820, 0x67882f, MWA16_RAM, &metro_irq_vectors	},	/* IRQ Vectors*/
	{ 0x678830, 0x678831, MWA16_RAM, &metro_irq_enable	},	/* IRQ Enable*/
	{ 0x678832, 0x678833, metro_irq_cause_w				},	/* IRQ Acknowledge*/
	{ 0x678836, 0x678837, watchdog_reset16_w			},	/* Watchdog*/
	{ 0x678840, 0x67884d, metro_blitter_w, &metro_blitter_regs	},	/* Tiles Blitter*/
	{ 0x678860, 0x67886b, metro_window_w, &metro_window	},	/* Tilemap Window*/
	{ 0x678850, 0x67885b, MWA16_RAM, &metro_scroll		},	/* Scroll Regs*/
	{ 0x678870, 0x678871, MWA16_RAM, &metro_rombank		},	/* Rom Bank*/
	{ 0x678888, 0x678889, MWA16_RAM, &gakusai_input_sel	},	/* Inputs*/
	{ 0x679700, 0x679713, MWA16_RAM, &metro_videoregs	},	/* Video Registers*/
	{ 0x800000, 0x800001, MWA16_NOP						},	/* ? 5*/
	{ 0x900000, 0x900001, gakusai_oki_bank_lo_w			},	/* Sound*/
	{ 0xa00000, 0xa00001, gakusai_oki_bank_hi_w			},
	{ 0xb00000, 0xb00001, OKIM6295_data_0_lsb_w 		},
	{ 0xc00000, 0xc00001, YM2413_register_port_0_lsb_w	},
	{ 0xc00002, 0xc00003, YM2413_data_port_0_lsb_w		},
	{ 0xe00000, 0xe00001, gakusai_eeprom_w				},	/* EEPROM*/
MEMORY_END


/***************************************************************************
						Mahjong Doukyuusei Special
***************************************************************************/

READ16_HANDLER( dokyusp_eeprom_r )
{
	/* clock line asserted: write latch or select next bit to read*/
	EEPROM_set_clock_line(CLEAR_LINE);
	EEPROM_set_clock_line(ASSERT_LINE);

	return EEPROM_read_bit() & 1;
}

WRITE16_HANDLER( dokyusp_eeprom_bit_w )
{
	if (ACCESSING_LSB)
	{
		/* latch the bit*/
		EEPROM_write_bit(data & 0x01);

		/* clock line asserted: write latch or select next bit to read*/
		EEPROM_set_clock_line(CLEAR_LINE);
		EEPROM_set_clock_line(ASSERT_LINE);
	}
}

WRITE16_HANDLER( dokyusp_eeprom_reset_w )
{
	if (ACCESSING_LSB)
	{
		/* reset line asserted: reset.*/
		EEPROM_set_cs_line((data & 0x01) ? CLEAR_LINE : ASSERT_LINE);
	}
}

static MEMORY_READ16_START( dokyusp_readmem )
	{ 0x000000, 0x03ffff, MRA16_ROM					},	/* ROM*/
	{ 0xff0000, 0xffffff, MRA16_RAM					},	/* RAM*/
	{ 0x200000, 0x21ffff, MRA16_RAM					},	/* Layer 0*/
	{ 0x220000, 0x23ffff, MRA16_RAM					},	/* Layer 1*/
	{ 0x240000, 0x25ffff, MRA16_RAM					},	/* Layer 2*/
	{ 0x260000, 0x26ffff, metro_bankedrom_r			},	/* Banked ROM*/
	{ 0x270000, 0x273fff, MRA16_RAM					},	/* Palette*/
	{ 0x274000, 0x274fff, MRA16_RAM					},	/* Sprites*/
	{ 0x278000, 0x2787ff, MRA16_RAM					},	/* Tiles Set*/
	{ 0x278832, 0x278833, metro_irq_cause_r			},	/* IRQ Cause*/
	{ 0x278880, 0x278881, gakusai_input_r			},	/* Inputs*/
	{ 0x278882, 0x278883, input_port_5_word_r		},	/**/
	{ 0x27880e, 0x27880f, MRA16_RAM					},	/* Screen Control*/
	{ 0x700000, 0x700001, OKIM6295_status_0_lsb_r	},	/* Sound*/
	{ 0xd00000, 0xd00001, dokyusp_eeprom_r			},	/* EEPROM*/
MEMORY_END

static MEMORY_WRITE16_START( dokyusp_writemem )
	{ 0x000000, 0x03ffff, MWA16_ROM						},	/* ROM*/
	{ 0xff0000, 0xffffff, MWA16_RAM						},	/* RAM*/
	{ 0x200000, 0x21ffff, metro_vram_0_w, &metro_vram_0	},	/* Layer 0*/
	{ 0x220000, 0x23ffff, metro_vram_1_w, &metro_vram_1	},	/* Layer 1*/
	{ 0x240000, 0x25ffff, metro_vram_2_w, &metro_vram_2	},	/* Layer 2*/
	{ 0x270000, 0x273fff, metro_paletteram_w, &paletteram16	},	/* Palette*/
	{ 0x274000, 0x274fff, MWA16_RAM, &spriteram16, &spriteram_size				},	/* Sprites*/
	{ 0x278000, 0x2787ff, MWA16_RAM, &metro_tiletable, &metro_tiletable_size	},	/* Tiles Set*/
	{ 0x27880e, 0x27880f, MWA16_RAM, &metro_screenctrl	},	/* Screen Control*/
	{ 0x278810, 0x27881f, MWA16_RAM, &metro_irq_levels	},	/* IRQ Levels*/
	{ 0x278820, 0x27882f, MWA16_RAM, &metro_irq_vectors	},	/* IRQ Vectors*/
	{ 0x278830, 0x278831, MWA16_RAM, &metro_irq_enable	},	/* IRQ Enable*/
	{ 0x278832, 0x278833, metro_irq_cause_w				},	/* IRQ Acknowledge*/
	{ 0x278836, 0x278837, watchdog_reset16_w			},	/* Watchdog*/
	{ 0x278840, 0x27884d, metro_blitter_w, &metro_blitter_regs	},	/* Tiles Blitter*/
	{ 0x278860, 0x27886b, metro_window_w, &metro_window	},	/* Tilemap Window*/
	{ 0x278850, 0x27885b, MWA16_RAM, &metro_scroll		},	/* Scroll Regs*/
	{ 0x278870, 0x278871, MWA16_RAM, &metro_rombank		},	/* Rom Bank*/
	{ 0x278888, 0x278889, MWA16_RAM, &gakusai_input_sel	},	/* Inputs*/
	{ 0x279700, 0x279713, MWA16_RAM, &metro_videoregs	},	/* Video Registers*/
	{ 0x400000, 0x400001, MWA16_NOP						},	/* ? 5*/
	{ 0x500000, 0x500001, gakusai_oki_bank_lo_w			},	/* Sound*/
	{ 0x600000, 0x600001, YM2413_register_port_0_lsb_w	},
	{ 0x600002, 0x600003, YM2413_data_port_0_lsb_w		},
	{ 0x700000, 0x700001, OKIM6295_data_0_lsb_w 		},
	{ 0xc00000, 0xc00001, dokyusp_eeprom_reset_w		},	/* EEPROM*/
	{ 0xd00000, 0xd00001, dokyusp_eeprom_bit_w			},	/* EEPROM*/
MEMORY_END


/***************************************************************************
							Mahjong Doukyuusei
***************************************************************************/

static MEMORY_READ16_START( dokyusei_readmem )
	{ 0x000000, 0x03ffff, MRA16_ROM					},	/* ROM*/
	{ 0xff0000, 0xffffff, MRA16_RAM					},	/* RAM*/
	{ 0x400000, 0x41ffff, MRA16_RAM					},	/* Layer 0*/
	{ 0x420000, 0x43ffff, MRA16_RAM					},	/* Layer 1*/
	{ 0x440000, 0x45ffff, MRA16_RAM					},	/* Layer 2*/
	{ 0x460000, 0x46ffff, metro_bankedrom_r			},	/* Banked ROM*/
	{ 0x470000, 0x473fff, MRA16_RAM					},	/* Palette*/
	{ 0x474000, 0x474fff, MRA16_RAM					},	/* Sprites*/
	{ 0x478000, 0x4787ff, MRA16_RAM					},	/* Tiles Set*/
/*	{ 0x478832, 0x478833, metro_irq_cause_r			},	*/ /* IRQ Cause*/
	{ 0x478880, 0x478881, gakusai_input_r			},	/* Inputs*/
	{ 0x478882, 0x478883, input_port_5_word_r		},	/**/
	{ 0x478884, 0x478885, input_port_6_word_r		},	/* 2 x DSW*/
	{ 0x478886, 0x478887, input_port_7_word_r		},	/**/
	{ 0xd00000, 0xd00001, OKIM6295_status_0_lsb_r	},	/* Sound*/
MEMORY_END

static MEMORY_WRITE16_START( dokyusei_writemem )
	{ 0x000000, 0x03ffff, MWA16_ROM						},	/* ROM*/
	{ 0xff0000, 0xffffff, MWA16_RAM						},	/* RAM*/
	{ 0x400000, 0x41ffff, metro_vram_0_w, &metro_vram_0	},	/* Layer 0*/
	{ 0x420000, 0x43ffff, metro_vram_1_w, &metro_vram_1	},	/* Layer 1*/
	{ 0x440000, 0x45ffff, metro_vram_2_w, &metro_vram_2	},	/* Layer 2*/
	{ 0x460000, 0x46ffff, MWA16_NOP						},	/* DSW Selection*/
	{ 0x470000, 0x473fff, metro_paletteram_w, &paletteram16	},	/* Palette*/
	{ 0x474000, 0x474fff, MWA16_RAM, &spriteram16, &spriteram_size				},	/* Sprites*/
	{ 0x478000, 0x4787ff, MWA16_RAM, &metro_tiletable, &metro_tiletable_size	},	/* Tiles Set*/
	{ 0x47880e, 0x47880f, MWA16_RAM, &metro_screenctrl	},	/* Screen Control*/
	{ 0x478810, 0x47881f, MWA16_RAM, &metro_irq_levels	},	/* IRQ Levels*/
	{ 0x478820, 0x47882f, MWA16_RAM, &metro_irq_vectors	},	/* IRQ Vectors*/
	{ 0x478830, 0x478831, MWA16_RAM, &metro_irq_enable	},	/* IRQ Enable*/
	{ 0x478832, 0x478833, metro_irq_cause_w				},	/* IRQ Acknowledge*/
	{ 0x478836, 0x478837, MWA16_NOP						},	/* ? watchdog ?*/
	{ 0x478840, 0x47884d, metro_blitter_w, &metro_blitter_regs	},	/* Tiles Blitter*/
	{ 0x478860, 0x47886b, metro_window_w, &metro_window	},	/* Tilemap Window*/
	{ 0x478850, 0x47885b, MWA16_RAM, &metro_scroll		},	/* Scroll Regs*/
	{ 0x478870, 0x478871, MWA16_RAM, &metro_rombank		},	/* Rom Bank*/
	{ 0x479700, 0x479713, MWA16_RAM, &metro_videoregs	},	/* Video Registers*/
	{ 0x478888, 0x478889, MWA16_RAM, &gakusai_input_sel	},	/* Inputs*/
	{ 0x800000, 0x800001, gakusai_oki_bank_hi_w			},	/* Samples Bank?*/
	{ 0x900000, 0x900001, MWA16_NOP						},	/* ? 4*/
	{ 0xa00000, 0xa00001, gakusai_oki_bank_lo_w			},	/* Samples Bank*/
	{ 0xc00000, 0xc00001, YM2413_register_port_0_lsb_w	},	/* Sound*/
	{ 0xc00002, 0xc00003, YM2413_data_port_0_lsb_w		},	/**/
	{ 0xd00000, 0xd00001, OKIM6295_data_0_lsb_w			},	/**/
MEMORY_END


/***************************************************************************
								Pang Poms
***************************************************************************/

static MEMORY_READ16_START( pangpoms_readmem )
	{ 0x000000, 0x03ffff, MRA16_ROM				},	/* ROM*/
	{ 0xc00000, 0xc0ffff, MRA16_RAM				},	/* RAM*/
	{ 0x400000, 0x41ffff, MRA16_RAM				},	/* Layer 0*/
	{ 0x420000, 0x43ffff, MRA16_RAM				},	/* Layer 1*/
	{ 0x440000, 0x45ffff, MRA16_RAM				},	/* Layer 2*/
	{ 0x460000, 0x46ffff, metro_bankedrom_r		},	/* Banked ROM*/
	{ 0x470000, 0x473fff, MRA16_RAM				},	/* Palette*/
	{ 0x474000, 0x474fff, MRA16_RAM				},	/* Sprites*/
	{ 0x478000, 0x4787ff, MRA16_RAM				},	/* Tiles Set*/
	{ 0x4788a2, 0x4788a3, metro_irq_cause_r		},	/* IRQ Cause*/
	{ 0x800000, 0x800001, metro_soundstatus_r	},	/* From Sound CPU*/
	{ 0x800002, 0x800003, MRA16_NOP				},	/**/
	{ 0x800004, 0x800005, input_port_0_word_r	},	/* Inputs*/
	{ 0x800006, 0x800007, input_port_1_word_r	},	/**/
	{ 0x800008, 0x800009, input_port_2_word_r	},	/**/
	{ 0x80000a, 0x80000b, input_port_3_word_r	},	/**/
	{ 0x80000c, 0x80000d, input_port_4_word_r	},	/**/
	{ 0x80000e, 0x80000f, input_port_5_word_r	},	/**/
MEMORY_END

static MEMORY_WRITE16_START( pangpoms_writemem )
	{ 0x000000, 0x03ffff, MWA16_ROM						},	/* ROM*/
	{ 0xc00000, 0xc0ffff, MWA16_RAM						},	/* RAM*/
	{ 0x400000, 0x41ffff, metro_vram_0_w, &metro_vram_0	},	/* Layer 0*/
	{ 0x420000, 0x43ffff, metro_vram_1_w, &metro_vram_1	},	/* Layer 1*/
	{ 0x440000, 0x45ffff, metro_vram_2_w, &metro_vram_2	},	/* Layer 2*/
	{ 0x470000, 0x473fff, metro_paletteram_w, &paletteram16	},	/* Palette*/
	{ 0x474000, 0x474fff, MWA16_RAM, &spriteram16, &spriteram_size				},	/* Sprites*/
	{ 0x478000, 0x4787ff, MWA16_RAM, &metro_tiletable, &metro_tiletable_size	},	/* Tiles Set*/
	{ 0x478800, 0x478813, MWA16_RAM, &metro_videoregs	},	/* Video Registers*/
	{ 0x478840, 0x47884d, metro_blitter_w, &metro_blitter_regs	},	/* Tiles Blitter*/
	{ 0x478860, 0x47886b, metro_window_w, &metro_window	},	/* Tilemap Window*/
	{ 0x478870, 0x47887b, MWA16_RAM, &metro_scroll		},	/* Scroll Regs*/
	{ 0x478880, 0x478881, MWA16_NOP						},	/* ? increasing*/
	{ 0x478890, 0x478891, MWA16_NOP						},	/* ? increasing*/
	{ 0x4788a2, 0x4788a3, metro_irq_cause_w				},	/* IRQ Acknowledge*/
	{ 0x4788a4, 0x4788a5, MWA16_RAM, &metro_irq_enable	},	/* IRQ Enable*/
	{ 0x4788a8, 0x4788a9, metro_soundlatch_w			},	/* To Sound CPU*/
	{ 0x4788aa, 0x4788ab, MWA16_RAM, &metro_rombank		},	/* Rom Bank*/
	{ 0x4788ac, 0x4788ad, MWA16_RAM, &metro_screenctrl	},	/* Screen Control*/
	{ 0x800000, 0x800001, metro_soundstatus_w			},	/* To Sound CPU*/
	{ 0x800002, 0x800003, metro_coin_lockout_1word_w	},	/* Coin Lockout*/
MEMORY_END


/***************************************************************************
								Poitto!
***************************************************************************/

static MEMORY_READ16_START( poitto_readmem )
	{ 0x000000, 0x03ffff, MRA16_ROM				},	/* ROM*/
	{ 0x400000, 0x40ffff, MRA16_RAM				},	/* RAM*/
	{ 0xc00000, 0xc1ffff, MRA16_RAM				},	/* Layer 0*/
	{ 0xc20000, 0xc3ffff, MRA16_RAM				},	/* Layer 1*/
	{ 0xc40000, 0xc5ffff, MRA16_RAM				},	/* Layer 2*/
	{ 0xc60000, 0xc6ffff, metro_bankedrom_r		},	/* Banked ROM*/
	{ 0xc70000, 0xc73fff, MRA16_RAM				},	/* Palette*/
	{ 0xc74000, 0xc74fff, MRA16_RAM				},	/* Sprites*/
	{ 0xc78000, 0xc787ff, MRA16_RAM				},	/* Tiles Set*/
	{ 0xc788a2, 0xc788a3, metro_irq_cause_r		},	/* IRQ Cause*/
	{ 0x800000, 0x800001, daitorid_soundstatus_r	},	/* Inputs*/
	{ 0x800002, 0x800003, input_port_1_word_r	},	/**/
	{ 0x800004, 0x800005, input_port_2_word_r	},	/**/
	{ 0x800006, 0x800007, input_port_3_word_r	},	/**/
MEMORY_END

static MEMORY_WRITE16_START( poitto_writemem )
	{ 0x000000, 0x03ffff, MWA16_ROM						},	/* ROM*/
	{ 0x400000, 0x40ffff, MWA16_RAM						},	/* RAM*/
	{ 0xc00000, 0xc1ffff, metro_vram_0_w, &metro_vram_0	},	/* Layer 0*/
	{ 0xc20000, 0xc3ffff, metro_vram_1_w, &metro_vram_1	},	/* Layer 1*/
	{ 0xc40000, 0xc5ffff, metro_vram_2_w, &metro_vram_2	},	/* Layer 2*/
	{ 0xc70000, 0xc73fff, metro_paletteram_w, &paletteram16	},	/* Palette*/
	{ 0xc74000, 0xc74fff, MWA16_RAM, &spriteram16, &spriteram_size				},	/* Sprites*/
	{ 0xc78000, 0xc787ff, MWA16_RAM, &metro_tiletable, &metro_tiletable_size	},	/* Tiles Set*/
	{ 0xc78800, 0xc78813, MWA16_RAM, &metro_videoregs	},	/* Video Registers*/
	{ 0xc78840, 0xc7884d, metro_blitter_w, &metro_blitter_regs	},	/* Tiles Blitter*/
	{ 0xc78860, 0xc7886b, metro_window_w, &metro_window	},	/* Tilemap Window*/
	{ 0xc78870, 0xc7887b, MWA16_RAM, &metro_scroll		},	/* Scroll Regs*/
	{ 0xc78880, 0xc78881, MWA16_NOP						},	/* ? increasing*/
	{ 0xc78890, 0xc78891, MWA16_NOP						},	/* ? increasing*/
	{ 0xc788a2, 0xc788a3, metro_irq_cause_w				},	/* IRQ Acknowledge*/
	{ 0xc788a4, 0xc788a5, MWA16_RAM, &metro_irq_enable	},	/* IRQ Enable*/
	{ 0xc788a8, 0xc788a9, metro_soundlatch_w			},	/* To Sound CPU*/
	{ 0xc788aa, 0xc788ab, MWA16_RAM, &metro_rombank		},	/* Rom Bank*/
	{ 0xc788ac, 0xc788ad, MWA16_RAM, &metro_screenctrl	},	/* Screen Control*/
	{ 0x800000, 0x800001, metro_soundstatus_w			},	/* To Sound CPU*/
	{ 0x800002, 0x800009, metro_coin_lockout_4words_w	},	/* Coin Lockout*/
MEMORY_END


/***************************************************************************
								Sky Alert
***************************************************************************/

static MEMORY_READ16_START( skyalert_readmem )
	{ 0x000000, 0x03ffff, MRA16_ROM				},	/* ROM*/
	{ 0xc00000, 0xc0ffff, MRA16_RAM				},	/* RAM*/
	{ 0x800000, 0x81ffff, MRA16_RAM				},	/* Layer 0*/
	{ 0x820000, 0x83ffff, MRA16_RAM				},	/* Layer 1*/
	{ 0x840000, 0x85ffff, MRA16_RAM				},	/* Layer 2*/
	{ 0x860000, 0x86ffff, metro_bankedrom_r		},	/* Banked ROM*/
	{ 0x870000, 0x873fff, MRA16_RAM				},	/* Palette*/
	{ 0x874000, 0x874fff, MRA16_RAM				},	/* Sprites*/
	{ 0x878000, 0x8787ff, MRA16_RAM				},	/* Tiles Set*/
	{ 0x8788a2, 0x8788a3, metro_irq_cause_r		},	/* IRQ Cause*/
	{ 0x400000, 0x400001, metro_soundstatus_r	},	/* From Sound CPU*/
	{ 0x400002, 0x400003, MRA16_NOP				},	/**/
	{ 0x400004, 0x400005, input_port_0_word_r	},	/* Inputs*/
	{ 0x400006, 0x400007, input_port_1_word_r	},	/**/
	{ 0x400008, 0x400009, input_port_2_word_r	},	/**/
	{ 0x40000a, 0x40000b, input_port_3_word_r	},	/**/
	{ 0x40000c, 0x40000d, input_port_4_word_r	},	/**/
	{ 0x40000e, 0x40000f, input_port_5_word_r	},	/**/
MEMORY_END

static MEMORY_WRITE16_START( skyalert_writemem )
	{ 0x000000, 0x03ffff, MWA16_ROM						},	/* ROM*/
	{ 0xc00000, 0xc0ffff, MWA16_RAM						},	/* RAM*/
	{ 0x800000, 0x81ffff, metro_vram_0_w, &metro_vram_0	},	/* Layer 0*/
	{ 0x820000, 0x83ffff, metro_vram_1_w, &metro_vram_1	},	/* Layer 1*/
	{ 0x840000, 0x85ffff, metro_vram_2_w, &metro_vram_2	},	/* Layer 2*/
	{ 0x870000, 0x873fff, metro_paletteram_w, &paletteram16	},	/* Palette*/
	{ 0x874000, 0x874fff, MWA16_RAM, &spriteram16, &spriteram_size				},	/* Sprites*/
	{ 0x878000, 0x8787ff, MWA16_RAM, &metro_tiletable, &metro_tiletable_size	},	/* Tiles Set*/
	{ 0x878800, 0x878813, MWA16_RAM, &metro_videoregs	},	/* Video Registers*/
	{ 0x878840, 0x87884d, metro_blitter_w, &metro_blitter_regs	},	/* Tiles Blitter*/
	{ 0x878860, 0x87886b, metro_window_w, &metro_window	},	/* Tilemap Window*/
	{ 0x878870, 0x87887b, MWA16_RAM, &metro_scroll		},	/* Scroll*/
	{ 0x878880, 0x878881, MWA16_NOP						},	/* ? increasing*/
	{ 0x878890, 0x878891, MWA16_NOP						},	/* ? increasing*/
	{ 0x8788a2, 0x8788a3, metro_irq_cause_w				},	/* IRQ Acknowledge*/
	{ 0x8788a4, 0x8788a5, MWA16_RAM, &metro_irq_enable	},	/* IRQ Enable*/
	{ 0x8788a8, 0x8788a9, metro_soundlatch_w			},	/* To Sound CPU*/
	{ 0x8788aa, 0x8788ab, MWA16_RAM, &metro_rombank		},	/* Rom Bank*/
	{ 0x8788ac, 0x8788ad, MWA16_RAM, &metro_screenctrl	},	/* Screen Control*/
	{ 0x400000, 0x400001, metro_soundstatus_w			},	/* To Sound CPU*/
	{ 0x400002, 0x400003, metro_coin_lockout_1word_w	},	/* Coin Lockout*/
MEMORY_END


/***************************************************************************
								Pururun
***************************************************************************/

static MEMORY_READ16_START( pururun_readmem )
	{ 0x000000, 0x07ffff, MRA16_ROM				},	/* ROM*/
	{ 0x800000, 0x80ffff, MRA16_RAM				},	/* RAM*/
	{ 0xc00000, 0xc1ffff, MRA16_RAM				},	/* Layer 0*/
	{ 0xc20000, 0xc3ffff, MRA16_RAM				},	/* Layer 1*/
	{ 0xc40000, 0xc5ffff, MRA16_RAM				},	/* Layer 2*/
	{ 0xc60000, 0xc6ffff, metro_bankedrom_r		},	/* Banked ROM*/
	{ 0xc70000, 0xc73fff, MRA16_RAM				},	/* Palette*/
	{ 0xc74000, 0xc74fff, MRA16_RAM				},	/* Sprites*/
	{ 0xc78000, 0xc787ff, MRA16_RAM				},	/* Tiles Set*/
	{ 0xc788a2, 0xc788a3, metro_irq_cause_r		},	/* IRQ Cause*/
	{ 0x400000, 0x400001, daitorid_soundstatus_r	},	/* Inputs*/
	{ 0x400002, 0x400003, input_port_1_word_r	},	/**/
	{ 0x400004, 0x400005, input_port_2_word_r	},	/**/
	{ 0x400006, 0x400007, input_port_3_word_r	},	/**/
MEMORY_END

static MEMORY_WRITE16_START( pururun_writemem )
	{ 0x000000, 0x07ffff, MWA16_ROM						},	/* ROM*/
	{ 0x800000, 0x80ffff, MWA16_RAM						},	/* RAM*/
	{ 0xc00000, 0xc1ffff, metro_vram_0_w, &metro_vram_0	},	/* Layer 0*/
	{ 0xc20000, 0xc3ffff, metro_vram_1_w, &metro_vram_1	},	/* Layer 1*/
	{ 0xc40000, 0xc5ffff, metro_vram_2_w, &metro_vram_2	},	/* Layer 2*/
	{ 0xc70000, 0xc73fff, metro_paletteram_w, &paletteram16	},	/* Palette*/
	{ 0xc74000, 0xc74fff, MWA16_RAM, &spriteram16, &spriteram_size				},	/* Sprites*/
	{ 0xc78000, 0xc787ff, MWA16_RAM, &metro_tiletable, &metro_tiletable_size	},	/* Tiles Set*/
	{ 0xc78840, 0xc7884d, metro_blitter_w, &metro_blitter_regs	},	/* Tiles Blitter*/
	{ 0xc78860, 0xc7886b, metro_window_w, &metro_window	},	/* Tilemap Window*/
	{ 0xc78870, 0xc7887b, MWA16_RAM, &metro_scroll		},	/* Scroll Regs*/
	{ 0xc78880, 0xc78881, MWA16_NOP						},	/* ? increasing*/
	{ 0xc78890, 0xc78891, MWA16_NOP						},	/* ? increasing*/
	{ 0xc788a2, 0xc788a3, metro_irq_cause_w				},	/* IRQ Acknowledge*/
	{ 0xc788a4, 0xc788a5, MWA16_RAM, &metro_irq_enable	},	/* IRQ Enable*/
	{ 0xc788a8, 0xc788a9, metro_soundlatch_w			},	/* To Sound CPU*/
	{ 0xc788aa, 0xc788ab, MWA16_RAM, &metro_rombank		},	/* Rom Bank*/
	{ 0xc788ac, 0xc788ad, MWA16_RAM, &metro_screenctrl	},	/* Screen Control*/
	{ 0xc79700, 0xc79713, MWA16_RAM, &metro_videoregs	},	/* Video Registers*/
	{ 0x400000, 0x400001, metro_soundstatus_w			},	/* To Sound CPU*/
	{ 0x400002, 0x400009, metro_coin_lockout_4words_w	},	/* Coin Lockout*/
MEMORY_END


/***************************************************************************
							Toride II Adauchi Gaiden
***************************************************************************/

static MEMORY_READ16_START( toride2g_readmem )
	{ 0x000000, 0x07ffff, MRA16_ROM				},	/* ROM*/
	{ 0x400000, 0x4cffff, MRA16_RAM				},	/* RAM (4xc000-4xffff mirrored?)*/
	{ 0xc00000, 0xc1ffff, MRA16_RAM				},	/* Layer 0*/
	{ 0xc20000, 0xc3ffff, MRA16_RAM				},	/* Layer 1*/
	{ 0xc40000, 0xc5ffff, MRA16_RAM				},	/* Layer 2*/
	{ 0xc60000, 0xc6ffff, metro_bankedrom_r		},	/* Banked ROM*/
	{ 0xc70000, 0xc73fff, MRA16_RAM				},	/* Palette*/
	{ 0xc74000, 0xc74fff, MRA16_RAM				},	/* Sprites*/
	{ 0xc78000, 0xc787ff, MRA16_RAM				},	/* Tiles Set*/
	{ 0xc788a2, 0xc788a3, metro_irq_cause_r		},	/* IRQ Cause*/
	{ 0x800000, 0x800001, daitorid_soundstatus_r	},	/* Inputs*/
	{ 0x800002, 0x800003, input_port_1_word_r	},	/**/
	{ 0x800004, 0x800005, input_port_2_word_r	},	/**/
	{ 0x800006, 0x800007, input_port_3_word_r	},	/**/
MEMORY_END

static MEMORY_WRITE16_START( toride2g_writemem )
	{ 0x000000, 0x07ffff, MWA16_ROM						},	/* ROM*/
	{ 0x400000, 0x4cffff, MWA16_RAM						},	/* RAM (4xc000-4xffff mirrored?)*/
	{ 0xc00000, 0xc1ffff, metro_vram_0_w, &metro_vram_0	},	/* Layer 0*/
	{ 0xc20000, 0xc3ffff, metro_vram_1_w, &metro_vram_1	},	/* Layer 1*/
	{ 0xc40000, 0xc5ffff, metro_vram_2_w, &metro_vram_2	},	/* Layer 2*/
	{ 0xc70000, 0xc73fff, metro_paletteram_w, &paletteram16	},	/* Palette*/
	{ 0xc74000, 0xc74fff, MWA16_RAM, &spriteram16, &spriteram_size				},	/* Sprites*/
	{ 0xc78000, 0xc787ff, MWA16_RAM, &metro_tiletable, &metro_tiletable_size	},	/* Tiles Set*/
	{ 0xc78840, 0xc7884d, metro_blitter_w, &metro_blitter_regs	},	/* Tiles Blitter*/
	{ 0xc78860, 0xc7886b, metro_window_w, &metro_window	},	/* Tilemap Window*/
	{ 0xc78870, 0xc7887b, MWA16_RAM, &metro_scroll		},	/* Scroll Regs*/
	{ 0xc78880, 0xc78881, MWA16_NOP						},	/* ? increasing*/
	{ 0xc78890, 0xc78891, MWA16_NOP						},	/* ? increasing*/
	{ 0xc788a2, 0xc788a3, metro_irq_cause_w				},	/* IRQ Acknowledge*/
	{ 0xc788a4, 0xc788a5, MWA16_RAM, &metro_irq_enable	},	/* IRQ Enable*/
	{ 0xc788a8, 0xc788a9, metro_soundlatch_w			},	/* To Sound CPU*/
	{ 0xc788aa, 0xc788ab, MWA16_RAM, &metro_rombank		},	/* Rom Bank*/
	{ 0xc788ac, 0xc788ad, MWA16_RAM, &metro_screenctrl	},	/* Screen Control*/
	{ 0xc79700, 0xc79713, MWA16_RAM, &metro_videoregs	},	/* Video Registers*/
	{ 0x800000, 0x800001, metro_soundstatus_w			},	/* To Sound CPU*/
	{ 0x800002, 0x800009, metro_coin_lockout_4words_w	},	/* Coin Lockout*/
MEMORY_END


/***************************************************************************
							Blazing Tornado
***************************************************************************/

static WRITE16_HANDLER( blzntrnd_sound_w )
{
	soundlatch_w(offset, data>>8);
	cpu_set_irq_line(1, IRQ_LINE_NMI, PULSE_LINE);
}

static WRITE_HANDLER( blzntrnd_sh_bankswitch_w )
{
	unsigned char *RAM = memory_region(REGION_CPU2);
	int bankaddress;

	bankaddress = 0x10000 + (data & 0x03) * 0x4000;
	cpu_setbank(1, &RAM[bankaddress]);
}

static void blzntrnd_irqhandler(int irq)
{
	cpu_set_irq_line(1, 0, irq ? ASSERT_LINE : CLEAR_LINE);
}

static struct YM2610interface blzntrnd_ym2610_interface =
{
	1,
	8000000,	/* 8 MHz??? */
	{ 25 },
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 },
	{ blzntrnd_irqhandler },
	{ REGION_SOUND1 },
	{ REGION_SOUND2 },
	{ YM3012_VOL(100,MIXER_PAN_LEFT,100,MIXER_PAN_RIGHT) }
};

static MEMORY_READ_START( blzntrnd_sound_readmem )
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0xbfff, MRA_BANK1 },
	{ 0xe000, 0xffff, MRA_RAM },
MEMORY_END

static MEMORY_WRITE_START( blzntrnd_sound_writemem )
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0x8000, 0xbfff, MWA_ROM },
	{ 0xe000, 0xffff, MWA_RAM },
MEMORY_END

static PORT_READ_START( blzntrnd_sound_readport )
	{ 0x40, 0x40, soundlatch_r },
	{ 0x80, 0x80, YM2610_status_port_0_A_r },
	{ 0x82, 0x82, YM2610_status_port_0_B_r },
PORT_END

static PORT_WRITE_START( blzntrnd_sound_writeport )
	{ 0x00, 0x00, blzntrnd_sh_bankswitch_w },
	{ 0x40, 0x40, IOWP_NOP },
	{ 0x80, 0x80, YM2610_control_port_0_A_w },
	{ 0x81, 0x81, YM2610_data_port_0_A_w },
	{ 0x82, 0x82, YM2610_control_port_0_B_w },
	{ 0x83, 0x83, YM2610_data_port_0_B_w },
PORT_END

static MEMORY_READ16_START( blzntrnd_readmem )
	{ 0x000000, 0x1fffff, MRA16_ROM				},	/* ROM*/
	{ 0xff0000, 0xffffff, MRA16_RAM				},	/* RAM*/
/*	{ 0x300000, 0x300001, MRA16_NOP				},	*/ /* Sound*/
	{ 0x200000, 0x21ffff, MRA16_RAM				},	/* Layer 0*/
	{ 0x220000, 0x23ffff, MRA16_RAM				},	/* Layer 1*/
	{ 0x240000, 0x25ffff, MRA16_RAM				},	/* Layer 2*/
	{ 0x260000, 0x26ffff, metro_bankedrom_r		},	/* Banked ROM*/
	{ 0x270000, 0x273fff, MRA16_RAM				},	/* Palette*/
	{ 0x274000, 0x274fff, MRA16_RAM				},	/* Sprites*/
	{ 0x278000, 0x2787ff, MRA16_RAM				},	/* Tiles Set*/
	{ 0x2788a2, 0x2788a3, metro_irq_cause_r		},	/* IRQ Cause*/
	{ 0xe00000, 0xe00001, input_port_0_word_r	},	/* Inputs*/
	{ 0xe00002, 0xe00003, input_port_1_word_r	},	/**/
	{ 0xe00004, 0xe00005, input_port_2_word_r	},	/**/
	{ 0xe00006, 0xe00007, input_port_3_word_r	},	/**/
	{ 0xe00008, 0xe00009, input_port_4_word_r	},	/**/
	{ 0x400000, 0x43ffff, MRA16_RAM				},	/* 053936*/
MEMORY_END

static MEMORY_WRITE16_START( blzntrnd_writemem )
	{ 0x000000, 0x1fffff, MWA16_ROM						},	/* ROM*/
	{ 0x200000, 0x21ffff, metro_vram_0_w, &metro_vram_0	},	/* Layer 0*/
	{ 0x220000, 0x23ffff, metro_vram_1_w, &metro_vram_1	},	/* Layer 1*/
	{ 0x240000, 0x25ffff, metro_vram_2_w, &metro_vram_2	},	/* Layer 2*/
	{ 0x260000, 0x26ffff, MWA16_NOP				},	/* ??????*/
	{ 0x270000, 0x273fff, metro_paletteram_w, &paletteram16	},	/* Palette*/
	{ 0x274000, 0x274fff, MWA16_RAM, &spriteram16, &spriteram_size				},	/* Sprites*/
	{ 0x278000, 0x2787ff, MWA16_RAM, &metro_tiletable, &metro_tiletable_size		},	/* Tiles Set*/
	{ 0x278860, 0x27886b, metro_window_w, &metro_window				},	/* Tilemap Window*/
	{ 0x278870, 0x27887b, MWA16_RAM, &metro_scroll		},	/* Scroll*/
	{ 0x278890, 0x278891, MWA16_NOP						},	/* ? increasing*/
	{ 0x2788a2, 0x2788a3, metro_irq_cause_w				},	/* IRQ Acknowledge*/
	{ 0x2788a4, 0x2788a5, MWA16_RAM, &metro_irq_enable	},	/* IRQ Enable*/
	{ 0x2788aa, 0x2788ab, MWA16_RAM, &metro_rombank		},	/* Rom Bank*/
	{ 0x2788ac, 0x2788ad, MWA16_RAM, &metro_screenctrl	},	/* Screen Control*/
	{ 0x279700, 0x279713, MWA16_RAM, &metro_videoregs	},	/* Video Registers*/
	{ 0x400000, 0x43ffff, metro_K053936_w, &metro_K053936_ram	},	/* 053936*/
	{ 0x500000, 0x500fff, MWA16_RAM, &K053936_0_linectrl },	/* 053936 line control*/
	{ 0x600000, 0x60001f, MWA16_RAM, &K053936_0_ctrl	},	/* 053936 control*/
	{ 0xe00000, 0xe00001, MWA16_NOP },
	{ 0xe00002, 0xe00003, blzntrnd_sound_w },
	{ 0xff0000, 0xffffff, MWA16_RAM						},	/* RAM*/
MEMORY_END


/***************************************************************************
									Mouja
***************************************************************************/

static MEMORY_READ16_START( mouja_readmem )
	{ 0x000000, 0x07ffff, MRA16_ROM				},	/* ROM*/
	{ 0xf00000, 0xf0ffff, MRA16_RAM				},	/* RAM*/
	{ 0x400000, 0x41ffff, MRA16_RAM				},	/* Layer 0*/
	{ 0x420000, 0x43ffff, MRA16_RAM				},	/* Layer 1*/
	{ 0x440000, 0x45ffff, MRA16_RAM				},	/* Layer 2*/
	{ 0x470000, 0x473fff, MRA16_RAM				},	/* Palette*/
	{ 0x474000, 0x474fff, MRA16_RAM				},	/* Sprites*/
	{ 0x478000, 0x4787ff, MRA16_RAM				},	/* Tiles Set*/
	{ 0x478832, 0x478833, metro_irq_cause_r		},	/* IRQ Cause*/
	{ 0x478880, 0x478881, input_port_0_word_r	},	/* Inputs*/
	{ 0x478882, 0x478883, input_port_1_word_r	},	/**/
	{ 0x478884, 0x478885, input_port_2_word_r	},	/**/
	{ 0x478886, 0x478887, input_port_3_word_r	},	/**/
	{ 0xd00000, 0xd00001, OKIM6295_status_0_lsb_r },
#if 0
	{ 0x460000, 0x46ffff, metro_bankedrom_r		},	/* Banked ROM*/
#endif
MEMORY_END

static MEMORY_WRITE16_START( mouja_writemem )
	{ 0x000000, 0x07ffff, MWA16_ROM						},	/* ROM*/
	{ 0xf00000, 0xf0ffff, MWA16_RAM						},	/* RAM*/
	{ 0x400000, 0x41ffff, metro_vram_0_w, &metro_vram_0	},	/* Layer 0*/
	{ 0x420000, 0x43ffff, metro_vram_1_w, &metro_vram_1	},	/* Layer 1*/
	{ 0x440000, 0x45ffff, metro_vram_2_w, &metro_vram_2	},	/* Layer 2*/
	{ 0x470000, 0x473fff, metro_paletteram_w, &paletteram16	},	/* Palette*/
	{ 0x474000, 0x474fff, MWA16_RAM, &spriteram16, &spriteram_size				},	/* Sprites*/
	{ 0x478000, 0x4787ff, MWA16_RAM, &metro_tiletable, &metro_tiletable_size	},	/* Tiles Set*/
	{ 0x47880e, 0x47880f, MWA16_RAM, &metro_screenctrl	},	/* Screen Control*/
	{ 0x478810, 0x47881f, MWA16_RAM, &metro_irq_levels	},	/* IRQ Levels*/
	{ 0x478820, 0x47882f, MWA16_RAM, &metro_irq_vectors	},	/* IRQ Vectors*/
	{ 0x478830, 0x478831, MWA16_RAM, &metro_irq_enable	},	/* IRQ Enable*/
	{ 0x478832, 0x478833, metro_irq_cause_w				},	/* IRQ Acknowledge*/
	{ 0x478836, 0x478837, watchdog_reset16_w			},	/* Watchdog*/
	{ 0x478860, 0x47886b, metro_window_w, &metro_window	},	/* Tilemap Window*/
	{ 0x478850, 0x47885b, MWA16_RAM, &metro_scroll		},	/* Scroll Regs*/
	{ 0x479700, 0x479713, MWA16_RAM, &metro_videoregs	},	/* Video Registers*/
	{ 0xc00000, 0xc00001, YM2413_register_port_0_lsb_w	},
	{ 0xc00002, 0xc00003, YM2413_data_port_0_lsb_w		},
	{ 0xd00000, 0xd00001, OKIM6295_data_0_msb_w },

#if 0
	{ 0x478840, 0x47884d, metro_blitter_w, &metro_blitter_regs	},	/* Tiles Blitter*/
	{ 0x47883a, 0x47883b, MWA16_RAM, &metro_rombank		},	/* Rom Bank*/
	{ 0x800002, 0x800009, metro_coin_lockout_4words_w	},	/* Coin Lockout*/
#endif
MEMORY_END

/***************************************************************************
                             Mouse Shooter GoGo
***************************************************************************/

static MEMORY_READ16_START( msgogo_readmem )
    { 0xf00000, 0xf0ffff, MRA16_RAM			     },
	{ 0x000000, 0x07ffff, MRA16_ROM				 },	
	{ 0x160000, 0x16ffff, metro_bankedrom_r		 },
	{ 0x200000, 0x200001, input_port_0_word_r	 },
	{ 0x200002, 0x200003, input_port_1_word_r	 },
	{ 0x200006, 0x200007, MRA16_NOP				 },	
	{ 0x100000, 0x11ffff, MRA16_RAM              },
	{ 0x120000, 0x13ffff, MRA16_RAM              },	
	{ 0x140000, 0x15ffff, MRA16_RAM              },
	{ 0x400000, 0x400001, ymf278b_r              },
	{ 0x300000, 0x31ffff, balcube_dsw_r			 },
	{ 0x170000, 0x173fff, MRA16_RAM              },
	{ 0x174000, 0x174fff, MRA16_RAM              },
	{ 0x178000, 0x1787ff, MRA16_RAM              },	
	{ 0x1788a2, 0x1788a3, metro_irq_cause_r		 },
MEMORY_END

static MEMORY_WRITE16_START( msgogo_writemem )
    { 0x170000, 0x173fff, metro_paletteram_w, &paletteram16	   },
	{ 0x174000, 0x174fff, MWA16_RAM, &spriteram16, &spriteram_size				},	
	{ 0x178000, 0x1787ff, MWA16_RAM, &metro_tiletable, &metro_tiletable_size	},
	{ 0x178840, 0x17884d, metro_blitter_w, &metro_blitter_regs },
	{ 0x178860, 0x17886b, metro_window_w, &metro_window		   },
	{ 0x178870, 0x17887b, MWA16_RAM, &metro_scroll		       },
	{ 0x178880, 0x178881, MWA16_NOP							   },
	{ 0x178890, 0x178891, MWA16_NOP							   },
	{ 0x1788a2, 0x1788a3, metro_irq_cause_w                    },
	{ 0x1788a4, 0x1788a5, MWA16_RAM, &metro_irq_enable	       },
	{ 0x1788aa, 0x1788ab, MWA16_RAM, &metro_rombank		       },
	{ 0x1788ac, 0x1788ad, MWA16_RAM, &metro_screenctrl	       },
	{ 0x179700, 0x179713, MWA16_RAM, &metro_videoregs	       },
	{ 0x200002, 0x200009, metro_coin_lockout_4words_w          },
	{ 0x400000, 0x40000b, ymf278b_w                            },
	{ 0xf00000, 0xf0ffff, MWA16_RAM						       },
	{ 0x000000, 0x07ffff, MWA16_ROM						       },
	{ 0x100000, 0x11ffff, metro_vram_0_w, &metro_vram_0        },
	{ 0x120000, 0x13ffff, metro_vram_1_w, &metro_vram_1        },
	{ 0x140000, 0x15ffff, metro_vram_2_w, &metro_vram_2        },
MEMORY_END


/***************************************************************************


								Input Ports


***************************************************************************/


#define JOY_LSB(_n_, _b1_, _b2_, _b3_, _b4_) \
	PORT_BIT(  0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER##_n_ ) \
	PORT_BIT(  0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER##_n_ ) \
	PORT_BIT(  0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER##_n_ ) \
	PORT_BIT(  0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER##_n_ ) \
	PORT_BIT(  0x0010, IP_ACTIVE_LOW, IPT_##_b1_         | IPF_PLAYER##_n_ ) \
	PORT_BIT(  0x0020, IP_ACTIVE_LOW, IPT_##_b2_         | IPF_PLAYER##_n_ ) \
	PORT_BIT(  0x0040, IP_ACTIVE_LOW, IPT_##_b3_         | IPF_PLAYER##_n_ ) \
	PORT_BIT(  0x0080, IP_ACTIVE_LOW, IPT_##_b4_         | IPF_PLAYER##_n_ ) \


#define JOY_MSB(_n_, _b1_, _b2_, _b3_, _b4_) \
	PORT_BIT(  0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER##_n_ ) \
	PORT_BIT(  0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER##_n_ ) \
	PORT_BIT(  0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER##_n_ ) \
	PORT_BIT(  0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER##_n_ ) \
	PORT_BIT(  0x1000, IP_ACTIVE_LOW, IPT_##_b1_         | IPF_PLAYER##_n_ ) \
	PORT_BIT(  0x2000, IP_ACTIVE_LOW, IPT_##_b2_         | IPF_PLAYER##_n_ ) \
	PORT_BIT(  0x4000, IP_ACTIVE_LOW, IPT_##_b3_         | IPF_PLAYER##_n_ ) \
	PORT_BIT(  0x8000, IP_ACTIVE_LOW, IPT_##_b4_         | IPF_PLAYER##_n_ ) \


#define COINS \
	PORT_BIT(  0x0001, IP_ACTIVE_LOW,  IPT_SERVICE1 ) \
	PORT_BIT(  0x0002, IP_ACTIVE_LOW,  IPT_TILT     ) \
	PORT_BIT_IMPULSE(  0x0004, IP_ACTIVE_LOW,  IPT_COIN1, 2    ) \
	PORT_BIT_IMPULSE(  0x0008, IP_ACTIVE_LOW,  IPT_COIN2, 2    ) \
	PORT_BIT(  0x0010, IP_ACTIVE_LOW,  IPT_START1   ) \
	PORT_BIT(  0x0020, IP_ACTIVE_LOW,  IPT_START2   ) \
	PORT_BIT(  0x0040, IP_ACTIVE_HIGH, IPT_UNKNOWN  ) \
	PORT_BIT(  0x0080, IP_ACTIVE_HIGH, IPT_SPECIAL  ) /* From Sound CPU in some games */


#define COINAGE_DSW \
	PORT_DIPNAME( 0x0007, 0x0007, DEF_STR( Coin_A ) ) \
	PORT_DIPSETTING(      0x0001, DEF_STR( 4C_1C ) ) \
	PORT_DIPSETTING(      0x0002, DEF_STR( 3C_1C ) ) \
	PORT_DIPSETTING(      0x0003, DEF_STR( 2C_1C ) ) \
	PORT_DIPSETTING(      0x0007, DEF_STR( 1C_1C ) ) \
	PORT_DIPSETTING(      0x0006, DEF_STR( 1C_2C ) ) \
	PORT_DIPSETTING(      0x0005, DEF_STR( 1C_3C ) ) \
	PORT_DIPSETTING(      0x0004, DEF_STR( 1C_4C ) ) \
	PORT_DIPSETTING(      0x0000, DEF_STR( Free_Play ) ) \
	PORT_DIPNAME( 0x0038, 0x0038, DEF_STR( Coin_B ) ) \
	PORT_DIPSETTING(      0x0008, DEF_STR( 4C_1C ) ) \
	PORT_DIPSETTING(      0x0010, DEF_STR( 3C_1C ) ) \
	PORT_DIPSETTING(      0x0018, DEF_STR( 2C_1C ) ) \
	PORT_DIPSETTING(      0x0038, DEF_STR( 1C_1C ) ) \
	PORT_DIPSETTING(      0x0030, DEF_STR( 1C_2C ) ) \
	PORT_DIPSETTING(      0x0028, DEF_STR( 1C_3C ) ) \
	PORT_DIPSETTING(      0x0020, DEF_STR( 1C_4C ) ) \
	PORT_DIPSETTING(      0x0000, DEF_STR( Free_Play ) ) \
	PORT_DIPNAME( 0x0040, 0x0040, DEF_STR( Flip_Screen ) ) \
	PORT_DIPSETTING(      0x0040, DEF_STR( Off ) ) \
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) ) \
	PORT_SERVICE( 0x0080, IP_ACTIVE_LOW )



/***************************************************************************
									Bal Cube
***************************************************************************/

INPUT_PORTS_START( balcube )
	PORT_START	/* IN0 - $500000*/
	COINS

	PORT_START	/* IN1 - $500002*/
	JOY_LSB(1, BUTTON1, UNKNOWN, UNKNOWN, UNKNOWN)
	JOY_MSB(2, BUTTON1, UNKNOWN, UNKNOWN, UNKNOWN)

	PORT_START	/* IN2 - Strangely mapped in the 0x400000-0x41ffff range*/
	COINAGE_DSW

	PORT_DIPNAME( 0x0300, 0x0300, "Difficulty?" )
	PORT_DIPSETTING(      0x0100, "0" )
	PORT_DIPSETTING(      0x0000, "1" )
	PORT_DIPSETTING(      0x0200, "2" )
	PORT_DIPSETTING(      0x0300, "3" )
	PORT_DIPNAME( 0x0400, 0x0400, "2 Players Game" )
	PORT_DIPSETTING(      0x0000, "1 Credit" )
	PORT_DIPSETTING(      0x0400, "2 Credits" )
	PORT_DIPNAME( 0x0800, 0x0800, DEF_STR( Lives ) )
	PORT_DIPSETTING(      0x0800, "2" )
	PORT_DIPSETTING(      0x0000, "3" )
	PORT_DIPNAME( 0x1000, 0x1000, "Allow Continue" )
	PORT_DIPSETTING(      0x0000, DEF_STR( No ) )
	PORT_DIPSETTING(      0x1000, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x2000, 0x2000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x2000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x4000, 0x4000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x4000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x8000, 0x8000, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x8000, DEF_STR( On ) )

	PORT_START	/* IN3 - Strangely mapped in the 0x400000-0x41ffff range*/
	PORT_BIT(  0x00ff, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* unused*/
INPUT_PORTS_END


/***************************************************************************
								Bang Bang Ball
***************************************************************************/

INPUT_PORTS_START( bangball )
	PORT_START	/* IN0 - $d00000*/
	COINS

	PORT_START	/* IN1 - $d00002*/
	JOY_LSB(1, BUTTON1, UNKNOWN, UNKNOWN, UNKNOWN)
	JOY_MSB(2, BUTTON1, UNKNOWN, UNKNOWN, UNKNOWN)

	PORT_START	/* IN2 - Strangely mapped in the 0xc00000-0xc1ffff range*/
	COINAGE_DSW

	PORT_DIPNAME( 0x0300, 0x0300, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(      0x0200, "Easy"    )
	PORT_DIPSETTING(      0x0300, "Normal"  )
	PORT_DIPSETTING(      0x0100, "Hard"    )
	PORT_DIPSETTING(      0x0000, "Hardest" )
	PORT_DIPNAME( 0x0c00, 0x0c00, DEF_STR( Lives ) )
	PORT_DIPSETTING(      0x0800, "2" )
	PORT_DIPSETTING(      0x0400, "3" )
	PORT_DIPSETTING(      0x0c00, "4" )
	PORT_DIPSETTING(      0x0000, "5" )
	PORT_DIPNAME( 0x1000, 0x1000, "Allow Continue" )
	PORT_DIPSETTING(      0x0000, DEF_STR( No ) )
	PORT_DIPSETTING(      0x1000, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x2000, 0x2000, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x2000, DEF_STR( On ) )
	PORT_DIPNAME( 0x4000, 0x4000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x4000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x8000, 0x8000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x8000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )

	
	PORT_START	/* IN3 - Strangely mapped in the 0x400000-0x41ffff range*/
	PORT_BIT(  0x00ff, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* unused*/
INPUT_PORTS_END

/***************************************************************************
                             Mouse Shooter GoGo
***************************************************************************/

INPUT_PORTS_START( msgogo )

	PORT_START
	COINS

	PORT_START
	JOY_LSB(1, BUTTON1, BUTTON2, UNKNOWN, UNKNOWN)
	JOY_MSB(2, BUTTON1, BUTTON2, UNKNOWN, UNKNOWN)

	PORT_START
	COINAGE_DSW
	PORT_DIPNAME( 0x0300, 0x0300, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(      0x0200, "Easy"    )
	PORT_DIPSETTING(      0x0300, "Normal"  )
	PORT_DIPSETTING(      0x0100, "Hard"    )
	PORT_DIPSETTING(      0x0000, "Hardest" )
	PORT_DIPNAME( 0x0400, 0x0400, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0400, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0800, 0x0800, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0800, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
        PORT_DIPNAME( 0x1000, 0x1000, "Allow P2 to Join Game" )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
        PORT_DIPSETTING(      0x1000, DEF_STR( On ) )
	PORT_DIPNAME( 0x2000, 0x2000, DEF_STR( Lives ) )
	PORT_DIPSETTING(      0x2000, "2" )
	PORT_DIPSETTING(      0x0000, "3" )
	PORT_DIPNAME( 0x4000, 0x4000, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x4000, DEF_STR( On ) )
	PORT_DIPNAME( 0x8000, 0x0000, "Language" )
	PORT_DIPSETTING(      0x8000, "Japanese" )
	PORT_DIPSETTING(      0x0000, "English" )

	PORT_START
	PORT_DIPNAME( 0x0001, 0x0001, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0001, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0002, 0x0002, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0004, 0x0004, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0004, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0008, 0x0008, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0008, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0010, 0x0010, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0010, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0020, 0x0020, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0040, 0x0040, "Debug: Offset" )
	PORT_DIPSETTING(      0x0040, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0080, 0x0080, "Debug: Menu" )
	PORT_DIPSETTING(      0x0080, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
INPUT_PORTS_END


/***************************************************************************
							Blazing Tornado
***************************************************************************/

INPUT_PORTS_START( blzntrnd )
	PORT_START
	PORT_DIPNAME( 0x0007, 0x0004, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(      0x0007, "Beginner" )
	PORT_DIPSETTING(      0x0006, "Easiest" )
	PORT_DIPSETTING(      0x0005, "Easy" )
	PORT_DIPSETTING(      0x0004, "Normal" )
	PORT_DIPSETTING(      0x0003, "Hard" )
	PORT_DIPSETTING(      0x0002, "Hardest" )
	PORT_DIPSETTING(      0x0001, "Expert" )
	PORT_DIPSETTING(      0x0000, "Master" )
	PORT_DIPNAME( 0x0008, 0x0008, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(      0x0008, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0010, 0x0000, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(      0x0010, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0020, 0x0000, "Allow Continue" )
	PORT_DIPSETTING(      0x0020, DEF_STR( No ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x00c0, 0x0000, "Control Panel" )
	PORT_DIPSETTING(      0x0000, "4 Players" )
/*	PORT_DIPSETTING(      0x0040, "4 Players" )*/
	PORT_DIPSETTING(      0x0080, "1P & 2P Tag only" )
	PORT_DIPSETTING(      0x00c0, "1P & 2P vs only" )
	PORT_DIPNAME( 0x0300, 0x0300, "Half Continue" )
	PORT_DIPSETTING(      0x0000, "6C to start, 3C to continue" )
	PORT_DIPSETTING(      0x0100, "4C to start, 2C to continue" )
	PORT_DIPSETTING(      0x0200, "2C to start, 1C to continue" )
	PORT_DIPSETTING(      0x0300, "Disabled" )
	PORT_DIPNAME( 0x0400, 0x0400, DEF_STR( Unused ) )
	PORT_DIPSETTING(      0x0400, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0800, 0x0800, DEF_STR( Unused ) )
	PORT_DIPSETTING(      0x0800, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x1000, 0x1000, DEF_STR( Unused ) )
	PORT_DIPSETTING(      0x1000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x2000, 0x2000, DEF_STR( Unused ) )
	PORT_DIPSETTING(      0x2000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x4000, 0x4000, DEF_STR( Unused ) )
	PORT_DIPSETTING(      0x4000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x8000, 0x8000, DEF_STR( Unused ) )
	PORT_DIPSETTING(      0x8000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )

	PORT_START
	PORT_DIPNAME( 0x0007, 0x0007, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(      0x0004, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(      0x0005, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0006, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0007, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0003, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x0001, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 1C_5C ) )
	PORT_DIPNAME( 0x0038, 0x0038, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(      0x0028, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0030, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0038, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0018, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x0010, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x0008, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 1C_5C ) )
	PORT_DIPNAME( 0x0040, 0x0040, DEF_STR( Free_Play ) )
	PORT_DIPSETTING(      0x0040, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_BITX(0x0080, IP_ACTIVE_LOW, IPT_SERVICE, DEF_STR( Service_Mode ), KEYCODE_F2, IP_JOY_NONE )
	PORT_DIPNAME( 0x0300, 0x0300, "CP Single" )
	PORT_DIPSETTING(      0x0300, "2:00" )
	PORT_DIPSETTING(      0x0200, "2:30" )
	PORT_DIPSETTING(      0x0100, "3:00" )
	PORT_DIPSETTING(      0x0000, "3:30" )
	PORT_DIPNAME( 0x0c00, 0x0c00, "CP Tag" )
	PORT_DIPSETTING(      0x0c00, "2:00" )
	PORT_DIPSETTING(      0x0800, "2:30" )
	PORT_DIPSETTING(      0x0400, "3:00" )
	PORT_DIPSETTING(      0x0000, "3:30" )
	PORT_DIPNAME( 0x3000, 0x3000, "Vs Single" )
	PORT_DIPSETTING(      0x3000, "2:30" )
	PORT_DIPSETTING(      0x2000, "3:00" )
	PORT_DIPSETTING(      0x1000, "4:00" )
	PORT_DIPSETTING(      0x0000, "5:00" )
	PORT_DIPNAME( 0xc000, 0xc000, "Vs Tag" )
	PORT_DIPSETTING(      0xc000, "2:30" )
	PORT_DIPSETTING(      0x8000, "3:00" )
	PORT_DIPSETTING(      0x4000, "4:00" )
	PORT_DIPSETTING(      0x0000, "5:00" )

	PORT_START
	JOY_LSB(1, BUTTON1, BUTTON2, BUTTON3, BUTTON4)
	JOY_MSB(2, BUTTON1, BUTTON2, BUTTON3, BUTTON4)

	PORT_START
	JOY_LSB(3, BUTTON1, BUTTON2, BUTTON3, BUTTON4)
	JOY_MSB(4, BUTTON1, BUTTON2, BUTTON3, BUTTON4)

	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BITX(0x0002, IP_ACTIVE_LOW, IPT_SERVICE, DEF_STR( Service_Mode ), KEYCODE_F2, IP_JOY_NONE )
	PORT_BIT_IMPULSE(  0x0004, IP_ACTIVE_LOW, IPT_COIN1, 2    )
	PORT_BIT_IMPULSE(  0x0008, IP_ACTIVE_LOW, IPT_COIN2, 2    )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_START3 )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_START4 )
INPUT_PORTS_END


/***************************************************************************
							Grand Striker 2
***************************************************************************/

INPUT_PORTS_START( gstrik2 )
	PORT_START
	PORT_DIPNAME( 0x0003, 0x0003, "Player Vs Com" )	
	PORT_DIPSETTING(      0x0003, "1:00" )
	PORT_DIPSETTING(      0x0002, "1:30" )
	PORT_DIPSETTING(      0x0001, "2:00" )
	PORT_DIPSETTING(      0x0000, "2:30" )
	PORT_DIPNAME( 0x000c, 0x000c, "1P Vs 2P" )
	PORT_DIPSETTING(      0x000c, "0:45" )
	PORT_DIPSETTING(      0x0008, "1:00" )
	PORT_DIPSETTING(      0x0004, "1:30" )
	PORT_DIPSETTING(      0x0000, "2:00" )
	PORT_DIPNAME( 0x0030, 0x0030, "Extra Time" )
	PORT_DIPSETTING(      0x0030, "0:30" )
	PORT_DIPSETTING(      0x0020, "0:45" )
	PORT_DIPSETTING(      0x0010, "1:00" )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPNAME( 0x0040, 0x0040, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0040, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0080, 0x0080, "Extra Time" )
	PORT_DIPSETTING(      0x0080, "Sudden Death" )
	PORT_DIPSETTING(      0x0000, "Full" )
	PORT_DIPNAME( 0x0700, 0x0400, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(      0x0700, "Very Easy" )
	PORT_DIPSETTING(      0x0600, "Easier" )
	PORT_DIPSETTING(      0x0500, "Easy" )
	PORT_DIPSETTING(      0x0400, "Normal" )
	PORT_DIPSETTING(      0x0300, "Medium" )
	PORT_DIPSETTING(      0x0200, "Hard" )
	PORT_DIPSETTING(      0x0100, "Hardest" )
	PORT_DIPSETTING(      0x0000, "Very Hard" )
	PORT_DIPNAME( 0x0800, 0x0800, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0800, DEF_STR( On ) )
	PORT_DIPNAME( 0x1000, 0x1000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x1000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x2000, 0x2000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x2000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x4000, 0x4000, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(      0x4000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_BITX(0x8000, IP_ACTIVE_LOW, IPT_SERVICE, DEF_STR( Service_Mode ), KEYCODE_F2, IP_JOY_NONE )

	PORT_START
	PORT_DIPNAME( 0x001f, 0x001f, DEF_STR( Coin_A ) )	
	PORT_DIPSETTING(      0x001c, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(      0x001d, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0018, DEF_STR( 4C_2C ) )
	PORT_DIPSETTING(      0x001e, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0019, DEF_STR( 3C_2C ) )
	PORT_DIPSETTING(      0x0014, DEF_STR( 4C_3C ) )
	PORT_DIPSETTING(      0x0010, DEF_STR( 4C_4C ) )
	PORT_DIPSETTING(      0x0015, DEF_STR( 3C_3C ) )
	PORT_DIPSETTING(      0x001a, DEF_STR( 2C_2C ) )
	PORT_DIPSETTING(      0x001f, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x000c, DEF_STR( 4C_5C ) )
	PORT_DIPSETTING(      0x0011, DEF_STR( 3C_4C ) )
	PORT_DIPSETTING(      0x0008, "4 Coins/6 Credits" )
	PORT_DIPSETTING(      0x0016, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(      0x000d, "3 Coins/5 Credits" )
	PORT_DIPSETTING(      0x0004, DEF_STR( 4C_7C ) )
	PORT_DIPSETTING(      0x0000, "4 Coins/8 Credits" )
	PORT_DIPSETTING(      0x0009, "3 Coins/6 Credits" )
	PORT_DIPSETTING(      0x0012, DEF_STR( 2C_4C ) )
	PORT_DIPSETTING(      0x001b, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x0005, "3 Coins/7 Credits" )
	PORT_DIPSETTING(      0x000e, DEF_STR( 2C_5C ) )
	PORT_DIPSETTING(      0x0001, "3 Coins/8 Credits" )
	PORT_DIPSETTING(      0x000a, DEF_STR( 2C_6C ) )
	PORT_DIPSETTING(      0x0017, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x0006, DEF_STR( 2C_7C ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( 2C_8C ) )
	PORT_DIPSETTING(      0x0013, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(      0x000f, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(      0x000b, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(      0x0007, DEF_STR( 1C_7C ) )
	PORT_DIPSETTING(      0x0003, DEF_STR( 1C_8C ) )
	PORT_DIPNAME( 0x00e0, 0x0000, DEF_STR( Coin_B ) )	
	PORT_DIPSETTING(      0x00c0, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x00a0, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x0080, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x0060, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(      0x0040, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 1C_7C ) )
	PORT_DIPSETTING(      0x00e0, "Same as Coin A" )
	PORT_DIPNAME( 0x0300, 0x0300, "Credits to Start" )
	PORT_DIPSETTING(      0x0300, "1" )
	PORT_DIPSETTING(      0x0200, "2" )
	PORT_DIPSETTING(      0x0100, "3" )
	PORT_DIPSETTING(      0x0000, "4" )
	PORT_DIPNAME( 0x0c00, 0x0c00, "Credits to Continue" )
	PORT_DIPSETTING(      0x0c00, "1" )
	PORT_DIPSETTING(      0x0800, "2" )
	PORT_DIPSETTING(      0x0400, "3" )
	PORT_DIPSETTING(      0x0000, "4" )
	PORT_DIPNAME( 0x1000, 0x1000, "Continue" )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x1000, DEF_STR( On ) )
	PORT_DIPNAME( 0x2000, 0x2000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x2000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x4000, 0x4000, "Playmode" )
	PORT_DIPSETTING(      0x4000, "1 Credit for 1 Player" )
	PORT_DIPSETTING(      0x0000, "1 Credit for 2 Players" )
	PORT_DIPNAME( 0x8000, 0x8000, DEF_STR( Free_Play ) )
	PORT_DIPSETTING(      0x8000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )

	PORT_START
	JOY_LSB(1, BUTTON1, BUTTON2, BUTTON3, UNUSED)
	JOY_MSB(2, BUTTON1, BUTTON2, BUTTON3, UNUSED)

	PORT_START
	/* Not Used */

	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BITX(0x0002, IP_ACTIVE_LOW, IPT_SERVICE, DEF_STR( Service_Mode ), KEYCODE_F2, IP_JOY_NONE )
	PORT_BIT_IMPULSE(  0x0004, IP_ACTIVE_LOW, IPT_COIN1, 2    )
	PORT_BIT_IMPULSE(  0x0008, IP_ACTIVE_LOW, IPT_COIN2, 2    )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_START3 )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_START4 )
INPUT_PORTS_END

/***************************************************************************
								Dai Toride
***************************************************************************/

/* If only ONE of the "Coinage" is set to "Free Play", it is in fact "5C_1C".

   IN2 bits 12 and 13 are in fact "merged" :

     12  13    effect
     Off Off   Continue, Retry level
     On  Off   Continue, Ask player for retry
     Off On    No continue
     On  On    Continue, Retry level

*/
INPUT_PORTS_START( daitorid )
	PORT_START	/* IN0 - $c00000*/
	COINS

	PORT_START	/* IN1 - $c00002*/
	JOY_LSB(1, BUTTON1, UNKNOWN, UNKNOWN, UNKNOWN)		/* BUTTON2 and BUTTON3 in "test mode" only*/
	JOY_MSB(2, BUTTON1, UNKNOWN, UNKNOWN, UNKNOWN)		/* BUTTON2 and BUTTON3 in "test mode" only*/

	PORT_START	/* IN2 - $c00004*/
	COINAGE_DSW

	PORT_DIPNAME( 0x0300, 0x0300, DEF_STR( Difficulty ) )		/* Timer speed*/
	PORT_DIPSETTING(      0x0200, "Easy" )				/*   Slow*/
	PORT_DIPSETTING(      0x0300, "Normal" )				/*   Normal*/
	PORT_DIPSETTING(      0x0100, "Hard" )				/*   Fast*/
	PORT_DIPSETTING(      0x0000, "Hardest" )				/*   Fastest*/
	PORT_DIPNAME( 0x0400, 0x0400, DEF_STR( Unused ) )
	PORT_DIPSETTING(      0x0400, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0800, 0x0800, "Winning Rounds (Player VS Player)" )
	PORT_DIPSETTING(      0x0000, "1/1" )
	PORT_DIPSETTING(      0x0800, "2/3" )
	PORT_DIPNAME( 0x1000, 0x0000, "Retry Level On Continue" )
	PORT_DIPSETTING(      0x0000, "Ask Player" )
	PORT_DIPSETTING(      0x1000, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x2000, 0x2000, "Allow Continue" )
	PORT_DIPSETTING(      0x0000, DEF_STR( No ) )
	PORT_DIPSETTING(      0x2000, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x4000, 0x4000, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x4000, DEF_STR( On ) )
	PORT_DIPNAME( 0x8000, 0x8000, DEF_STR( Unused ) )
	PORT_DIPSETTING(      0x8000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )

	PORT_START	/* IN3 - $c00006*/
	PORT_BIT(  0xffff, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END


/***************************************************************************
								Dharma Doujou
***************************************************************************/

/* I don't really know HOW to describe the effect of IN2 bits 8 and 9.
   All I can tell is that in "table 2" the values are smaller for the 2
   first levels (so the game is harder), but they vary less between the
   levels (so there is almost no increasing difficulty).

   Even if there are 4 "tables" the 2 first ones and the 2 last ones
   contains the same values for the timer. */
INPUT_PORTS_START( dharma )
	PORT_START	/* IN0 - $c00000*/
	COINS

	PORT_START	/* IN1 - $c00002*/
	JOY_LSB(1, BUTTON1, UNKNOWN, UNKNOWN, UNKNOWN)		/* BUTTON2 and BUTTON3 in "test mode" only*/
	JOY_MSB(2, BUTTON1, UNKNOWN, UNKNOWN, UNKNOWN)		/* BUTTON2 and BUTTON3 in "test mode" only*/

	PORT_START	/* IN2 - $c00004*/
	COINAGE_DSW

	PORT_DIPNAME( 0x0300, 0x0300, "Time" )				/* Check code at 0x00da0a and see notes*/
	PORT_DIPSETTING(      0x0000, "Table 1" )				/*   Table offset : 0x00e668*/
/*	PORT_DIPSETTING(      0x0100, "Table 1" )				*/ /*   Table offset : 0x00e6c0*/
/*	PORT_DIPSETTING(      0x0200, "Table 2" )				*/ /*   Table offset : 0x00e718*/
	PORT_DIPSETTING(      0x0300, "Table 2" )				/*   Table offset : 0x00e770*/
	PORT_DIPNAME( 0x0c00, 0x0c00, DEF_STR( Difficulty ) )		/* Timer (crab) speed*/
	PORT_DIPSETTING(      0x0800, "Easy" )				/*   Slow*/
	PORT_DIPSETTING(      0x0c00, "Normal" )				/*   Normal*/
	PORT_DIPSETTING(      0x0400, "Hard" )				/*   Fast*/
	PORT_DIPSETTING(      0x0000, "Hardest" )				/*   Fastest*/
	PORT_DIPNAME( 0x1000, 0x1000, "2 Players Game" )
	PORT_DIPSETTING(      0x1000, "2 Credits" )
	PORT_DIPSETTING(      0x0000, "1 Credit" )
	PORT_DIPNAME( 0x2000, 0x2000, "Allow Continue" )
	PORT_DIPSETTING(      0x0000, DEF_STR( No ) )
	PORT_DIPSETTING(      0x2000, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x4000, 0x4000, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x4000, DEF_STR( On ) )
	PORT_BITX(    0x8000, 0x8000, IPT_DIPSWITCH_NAME | IPF_CHEAT, "Freeze", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(      0x8000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )

	PORT_START	/* IN3 - $c00006*/
	PORT_BIT(  0xffff, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END


/***************************************************************************
								Karate Tournament
***************************************************************************/

INPUT_PORTS_START( karatour )
	PORT_START	/* IN0 - $400002*/
	JOY_LSB(2, BUTTON1, BUTTON2, UNKNOWN, UNKNOWN)

	PORT_START	/* IN1 - $400004*/
	COINS

	PORT_START	/* IN2 - $400006*/
	PORT_DIPNAME( 0x0003, 0x0003, DEF_STR( Lives ) )
	PORT_DIPSETTING(      0x0001, "1" )
	PORT_DIPSETTING(      0x0000, "2" )
	PORT_DIPSETTING(      0x0003, "3" )
	PORT_DIPSETTING(      0x0002, "4" )
	PORT_DIPNAME( 0x000c, 0x000c, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x000c, "11 (0)" )
	PORT_DIPSETTING(      0x0008, "10 (1)" )
	PORT_DIPSETTING(      0x0004, "01 (2)" )
	PORT_DIPSETTING(      0x0000, "00 (3)" )
	PORT_DIPNAME( 0x0010, 0x0010, "Time" )
	PORT_DIPSETTING(      0x0010, "60" )
	PORT_DIPSETTING(      0x0000, "40" )
	PORT_DIPNAME( 0x0020, 0x0020, DEF_STR( Free_Play ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0040, 0x0040, "Allow Continue" )
	PORT_DIPSETTING(      0x0000, DEF_STR( No ) )
	PORT_DIPSETTING(      0x0040, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x0080, 0x0000, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(      0x0080, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )

	PORT_START	/* IN3 - $40000a*/
	PORT_DIPNAME( 0x0007, 0x0007, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0001, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0007, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0006, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x0005, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x0004, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(      0x0003, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( 1C_6C ) )
	PORT_DIPNAME( 0x0038, 0x0038, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0008, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0038, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0030, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x0028, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(      0x0018, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(      0x0010, DEF_STR( 1C_6C ) )
	PORT_DIPNAME( 0x0040, 0x0040, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(      0x0040, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_SERVICE( 0x0080, IP_ACTIVE_LOW )

	PORT_START	/* IN4 - $40000c*/
	JOY_LSB(1, BUTTON1, BUTTON2, UNKNOWN, UNKNOWN)
INPUT_PORTS_END


/***************************************************************************
								Lady Killer
***************************************************************************/

INPUT_PORTS_START( ladykill )
	PORT_START	/* IN0 - $400002*/
	JOY_LSB(2, BUTTON1, BUTTON2, UNKNOWN, UNKNOWN)

	PORT_START	/* IN1 - $400004*/
	COINS

	PORT_START	/* IN2 - $400006*/
	PORT_DIPNAME( 0x0003, 0x0003, DEF_STR( Lives ) )
	PORT_DIPSETTING(      0x0001, "1" )
	PORT_DIPSETTING(      0x0000, "2" )
	PORT_DIPSETTING(      0x0003, "3" )
	PORT_DIPSETTING(      0x0002, "4" )
	PORT_DIPNAME( 0x000c, 0x000c, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(      0x0008, "Easy" )
	PORT_DIPSETTING(      0x000c, "Normal" )
	PORT_DIPSETTING(      0x0004, "Hard" )
	PORT_DIPSETTING(      0x0000, "Very Hard" )
	PORT_DIPNAME( 0x0010, 0x0000, "Nudity" )
	PORT_DIPSETTING(      0x0010, "Partial" )
	PORT_DIPSETTING(      0x0000, "Full" )
	PORT_SERVICE( 0x0020, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x0040, 0x0040, "Allow Continue" )
	PORT_DIPSETTING(      0x0000, DEF_STR( No ) )
	PORT_DIPSETTING(      0x0040, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x0080, 0x0000, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(      0x0080, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )

	PORT_START	/* IN3 - $40000a*/
	PORT_DIPNAME( 0x0007, 0x0007, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0001, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0007, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0006, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x0005, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x0004, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(      0x0003, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( 1C_6C ) )
	PORT_DIPNAME( 0x0038, 0x0038, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0008, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0038, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0030, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x0028, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(      0x0018, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(      0x0010, DEF_STR( 1C_6C ) )
	PORT_DIPNAME( 0x0040, 0x0040, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(      0x0040, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0080, 0x0080, DEF_STR( Unused ) )
	PORT_DIPSETTING(      0x0080, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )

	PORT_START	/* IN4 - $40000c*/
	JOY_LSB(1, BUTTON1, BUTTON2, UNKNOWN, UNKNOWN)
INPUT_PORTS_END

/* Same as 'ladykill' but NO "Nudity" Dip Switch */
INPUT_PORTS_START( moegonta )
	PORT_START	/* IN0 - $400002*/
	JOY_LSB(2, BUTTON1, BUTTON2, UNKNOWN, UNKNOWN)

	PORT_START	/* IN1 - $400004*/
	COINS

	PORT_START	/* IN2 - $400006*/
	PORT_DIPNAME( 0x0003, 0x0003, DEF_STR( Lives ) )
	PORT_DIPSETTING(      0x0001, "1" )
	PORT_DIPSETTING(      0x0000, "2" )
	PORT_DIPSETTING(      0x0003, "3" )
	PORT_DIPSETTING(      0x0002, "4" )
	PORT_DIPNAME( 0x000c, 0x000c, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(      0x0008, "Easy" )
	PORT_DIPSETTING(      0x000c, "Normal" )
	PORT_DIPSETTING(      0x0004, "Hard" )
	PORT_DIPSETTING(      0x0000, "Very Hard" )
	PORT_DIPNAME( 0x0010, 0x0010, DEF_STR( Unused ) )
	PORT_DIPSETTING(      0x0010, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_SERVICE( 0x0020, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x0040, 0x0040, "Allow Continue" )
	PORT_DIPSETTING(      0x0000, DEF_STR( No ) )
	PORT_DIPSETTING(      0x0040, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x0080, 0x0000, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(      0x0080, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )

	PORT_START	/* IN3 - $40000a*/
	PORT_DIPNAME( 0x0007, 0x0007, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0001, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0007, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0006, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x0005, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x0004, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(      0x0003, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( 1C_6C ) )
	PORT_DIPNAME( 0x0038, 0x0038, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0008, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0038, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0030, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x0028, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(      0x0018, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(      0x0010, DEF_STR( 1C_6C ) )
	PORT_DIPNAME( 0x0040, 0x0040, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(      0x0040, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0080, 0x0080, DEF_STR( Unused ) )
	PORT_DIPSETTING(      0x0080, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )

	PORT_START	/* IN4 - $40000c*/
	JOY_LSB(1, BUTTON1, BUTTON2, UNKNOWN, UNKNOWN)
INPUT_PORTS_END


/***************************************************************************
								Last Fortress
***************************************************************************/

/* The code which tests IN4 bit 7 is the SAME as the one for 'lastfero'.
   So WHY can't the game display cards instead of mahjong tiles ?
   Is it due to different GFX ROMS or to an emulation bug ?
*/
INPUT_PORTS_START( lastfort )
	PORT_START	/* IN0 - $c00004*/
	COINS

	PORT_START	/* IN1 - $c00006*/
	JOY_LSB(1, BUTTON1, UNKNOWN, UNKNOWN, UNKNOWN)		/* BUTTON2 and BUTTON3 in "test mode" only*/

	PORT_START	/* IN2 - $c00008*/
	JOY_LSB(2, BUTTON1, UNKNOWN, UNKNOWN, UNKNOWN)		/* BUTTON2 and BUTTON3 in "test mode" only*/

	PORT_START	/* IN3 - $c0000a*/
	COINAGE_DSW

	PORT_START	/* IN4 - $c0000c*/
	PORT_DIPNAME( 0x0003, 0x0003, DEF_STR( Difficulty ) )		/* Timer speed*/
	PORT_DIPSETTING(      0x0000, "Easiest" )				/*   Slowest*/
	PORT_DIPSETTING(      0x0001, "Easy" )				/*   Slow*/
	PORT_DIPSETTING(      0x0003, "Normal" )				/*   Normal*/
	PORT_DIPSETTING(      0x0002, "Hard" )				/*   Fast*/
	PORT_DIPNAME( 0x0004, 0x0004, DEF_STR( Unused ) )
	PORT_DIPSETTING(      0x0004, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0008, 0x0008, "Retry Level On Continue" )
	PORT_DIPSETTING(      0x0008, "Ask Player" )
	PORT_DIPSETTING(      0x0000, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x0010, 0x0010, "2 Players Game" )
	PORT_DIPSETTING(      0x0010, "2 Credits" )
	PORT_DIPSETTING(      0x0000, "1 Credit" )
	PORT_DIPNAME( 0x0020, 0x0020, "Allow Continue" )
	PORT_DIPSETTING(      0x0000, DEF_STR( No ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x0040, 0x0040, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0040, DEF_STR( On ) )
	PORT_DIPNAME( 0x0080, 0x0080, "Tiles" )
	PORT_DIPSETTING(      0x0080, "Mahjong" )
/*	PORT_DIPSETTING(      0x0000, "Cards" )				*/ /* Not working - See notes*/

	PORT_START	/* IN5 - $c0000e*/
	PORT_BIT(  0xffff, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END


/***************************************************************************
							Last Fortress (Erotic)
***************************************************************************/

/* Same as 'lastfort' but WORKING "Tiles" Dip Switch */
INPUT_PORTS_START( lastfero )
	PORT_START	/* IN0 - $c00004*/
	COINS

	PORT_START	/* IN1 - $c00006*/
	JOY_LSB(1, BUTTON1, UNKNOWN, UNKNOWN, UNKNOWN)		/* BUTTON2 and BUTTON3 in "test mode" only*/

	PORT_START	/* IN2 - $c00008*/
	JOY_LSB(2, BUTTON1, UNKNOWN, UNKNOWN, UNKNOWN)		/* BUTTON2 and BUTTON3 in "test mode" only*/

	PORT_START	/* IN3 - $c0000a*/
	COINAGE_DSW

	PORT_START	/* IN4 - $c0000c*/
	PORT_DIPNAME( 0x0003, 0x0003, DEF_STR( Difficulty ) )		/* Timer speed*/
	PORT_DIPSETTING(      0x0000, "Easiest" )				/*   Slowest*/
	PORT_DIPSETTING(      0x0001, "Easy" )				/*   Slow*/
	PORT_DIPSETTING(      0x0003, "Normal" )				/*   Normal*/
	PORT_DIPSETTING(      0x0002, "Hard" )				/*   Fast*/
	PORT_DIPNAME( 0x0004, 0x0004, DEF_STR( Unused ) )
	PORT_DIPSETTING(      0x0004, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0008, 0x0008, "Retry Level On Continue" )
	PORT_DIPSETTING(      0x0008, "Ask Player" )
	PORT_DIPSETTING(      0x0000, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x0010, 0x0010, "2 Players Game" )
	PORT_DIPSETTING(      0x0010, "2 Credits" )
	PORT_DIPSETTING(      0x0000, "1 Credit" )
	PORT_DIPNAME( 0x0020, 0x0020, "Allow Continue" )
	PORT_DIPSETTING(      0x0000, DEF_STR( No ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x0040, 0x0040, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0040, DEF_STR( On ) )
	PORT_DIPNAME( 0x0080, 0x0080, "Tiles" )
	PORT_DIPSETTING(      0x0080, "Mahjong" )
	PORT_DIPSETTING(      0x0000, "Cards" )

	PORT_START	/* IN5 - $c0000e*/
	PORT_BIT(  0xffff, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END


/***************************************************************************
							Mahjong Doukyuusei
***************************************************************************/

INPUT_PORTS_START( dokyusei )
	PORT_START	/* IN0 - $478880.w*/
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BITX(0x0002, IP_ACTIVE_LOW, 0, "A",   KEYCODE_A,        IP_JOY_NONE )
	PORT_BITX(0x0004, IP_ACTIVE_LOW, 0, "E",   KEYCODE_E,        IP_JOY_NONE )
	PORT_BITX(0x0008, IP_ACTIVE_LOW, 0, "I",   KEYCODE_I,        IP_JOY_NONE )
	PORT_BITX(0x0010, IP_ACTIVE_LOW, 0, "M",   KEYCODE_M,        IP_JOY_NONE )
	PORT_BITX(0x0020, IP_ACTIVE_LOW, 0, "Kan", KEYCODE_LCONTROL, IP_JOY_NONE )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_START1  )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0xff00, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* IN1 - $478880.w*/
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BITX(0x0002, IP_ACTIVE_LOW, 0, "B",     KEYCODE_B, IP_JOY_NONE )
	PORT_BITX(0x0004, IP_ACTIVE_LOW, 0, "F",     KEYCODE_F, IP_JOY_NONE )
	PORT_BITX(0x0008, IP_ACTIVE_LOW, 0, "J",     KEYCODE_J, IP_JOY_NONE )
	PORT_BITX(0x0010, IP_ACTIVE_LOW, 0, "N",     KEYCODE_N, IP_JOY_NONE )
	PORT_BITX(0x0020, IP_ACTIVE_LOW, 0, "Reach", KEYCODE_LSHIFT, IP_JOY_NONE )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0xff00, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* IN2 - $478880.w*/
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BITX(0x0002, IP_ACTIVE_LOW, 0, "C",   KEYCODE_C,      IP_JOY_NONE )
	PORT_BITX(0x0004, IP_ACTIVE_LOW, 0, "G",   KEYCODE_G,      IP_JOY_NONE )
	PORT_BITX(0x0008, IP_ACTIVE_LOW, 0, "K",   KEYCODE_K,      IP_JOY_NONE )
	PORT_BITX(0x0010, IP_ACTIVE_LOW, 0, "Chi", KEYCODE_SPACE, IP_JOY_NONE )
	PORT_BITX(0x0020, IP_ACTIVE_LOW, 0, "Ron", KEYCODE_Z, IP_JOY_NONE )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0xff00, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* IN3 - $478880.w*/
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BITX(0x0002, IP_ACTIVE_LOW, 0, "D",   KEYCODE_D,     IP_JOY_NONE )
	PORT_BITX(0x0004, IP_ACTIVE_LOW, 0, "H",   KEYCODE_H,     IP_JOY_NONE )
	PORT_BITX(0x0008, IP_ACTIVE_LOW, 0, "L",   KEYCODE_L,     IP_JOY_NONE )
	PORT_BITX(0x0010, IP_ACTIVE_LOW, 0, "Pon", KEYCODE_LALT,   IP_JOY_NONE )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0xff00, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* IN4 - $478880.w*/
	PORT_BIT( 0xffff, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* IN5 - $478882.w*/
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT_IMPULSE(  0x0002, IP_ACTIVE_LOW, IPT_COIN1, 2 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BITX(0x0008, IP_ACTIVE_LOW, IPT_SERVICE, DEF_STR( Service_Mode ), KEYCODE_F2, IP_JOY_NONE )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0xff00, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START /* IN6 - $478884.w*/
	PORT_DIPNAME( 0x0300, 0x0300, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(      0x0300, "Easy" )
	PORT_DIPSETTING(      0x0200, "Normal" )
	PORT_DIPSETTING(      0x0100, "Hard" )
	PORT_DIPSETTING(      0x0000, "Hardest" )
	PORT_DIPNAME( 0x1c00, 0x1c00, DEF_STR( Coinage ) )
	PORT_DIPSETTING(      0x0400, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(      0x0800, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0c00, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x1c00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x1800, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x1400, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x1000, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0x2000, 0x2000, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x2000, DEF_STR( On ) )
	PORT_DIPNAME( 0x4000, 0x4000, "Game Sound" )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x4000, DEF_STR( On ) )
	PORT_DIPNAME( 0x8000, 0x8000, "Auto TSUMO after REACH" )
	PORT_DIPSETTING(      0x8000, DEF_STR( No ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Yes ) )

	PORT_START /* IN7 - $478886.w*/
	PORT_DIPNAME( 0x0100, 0x0100, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(      0x0100, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0200, 0x0200, "CPU wears clothes on RON" )
	PORT_DIPSETTING(      0x0000, DEF_STR( No ) )
	PORT_DIPSETTING(      0x0200, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x0400, 0x0400, "CPU clothes on continue play" )
	PORT_DIPSETTING(      0x0400, "Return to default" )
	PORT_DIPSETTING(      0x0000, "Keep current status" )
	PORT_SERVICE( 0x0800, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x1000, 0x0000, "Self Test" ) /*!*/
	PORT_DIPSETTING(      0x0000, DEF_STR( No ) )
	PORT_DIPSETTING(      0x1000, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x2000, 0x2000, "Unknown 2-5" )
	PORT_DIPSETTING(      0x2000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x4000, 0x4000, "Unknown 2-6" )
	PORT_DIPSETTING(      0x4000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x8000, 0x8000, "Unknown 2-7" )
	PORT_DIPSETTING(      0x8000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
INPUT_PORTS_END


/***************************************************************************
						Mahjong Gakuensai 1 & 2
***************************************************************************/

/* Same as dokyusei, without the DSWs (these games have an eeprom) */

INPUT_PORTS_START( gakusai )
	PORT_START	/* IN0 - $278880.w*/
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BITX(0x0002, IP_ACTIVE_LOW, 0, "A",   KEYCODE_A,        IP_JOY_NONE )
	PORT_BITX(0x0004, IP_ACTIVE_LOW, 0, "E",   KEYCODE_E,        IP_JOY_NONE )
	PORT_BITX(0x0008, IP_ACTIVE_LOW, 0, "I",   KEYCODE_I,        IP_JOY_NONE )
	PORT_BITX(0x0010, IP_ACTIVE_LOW, 0, "M",   KEYCODE_M,        IP_JOY_NONE )
	PORT_BITX(0x0020, IP_ACTIVE_LOW, 0, "Kan", KEYCODE_LCONTROL, IP_JOY_NONE )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_START1  )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0xff00, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* IN1 - $278880.w*/
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BITX(0x0002, IP_ACTIVE_LOW, 0, "B",   KEYCODE_B, IP_JOY_NONE )
	PORT_BITX(0x0004, IP_ACTIVE_LOW, 0, "F",   KEYCODE_F, IP_JOY_NONE )
	PORT_BITX(0x0008, IP_ACTIVE_LOW, 0, "J",   KEYCODE_J, IP_JOY_NONE )
	PORT_BITX(0x0010, IP_ACTIVE_LOW, 0, "N",   KEYCODE_N, IP_JOY_NONE )
	PORT_BITX(0x0020, IP_ACTIVE_LOW, 0, "Reach", KEYCODE_LSHIFT, IP_JOY_NONE )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0xff00, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* IN2 - $278880.w*/
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BITX(0x0002, IP_ACTIVE_LOW, 0, "C",     KEYCODE_C,      IP_JOY_NONE )
	PORT_BITX(0x0004, IP_ACTIVE_LOW, 0, "G",     KEYCODE_G,      IP_JOY_NONE )
	PORT_BITX(0x0008, IP_ACTIVE_LOW, 0, "K",     KEYCODE_K,      IP_JOY_NONE )
	PORT_BITX(0x0010, IP_ACTIVE_LOW, 0, "Chi", KEYCODE_SPACE, IP_JOY_NONE )
	PORT_BITX(0x0020, IP_ACTIVE_LOW, 0, "Ron", KEYCODE_Z, IP_JOY_NONE )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0xff00, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* IN3 - $278880.w*/
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BITX(0x0002, IP_ACTIVE_LOW, 0, "D",   KEYCODE_D,     IP_JOY_NONE )
	PORT_BITX(0x0004, IP_ACTIVE_LOW, 0, "H",   KEYCODE_H,     IP_JOY_NONE )
	PORT_BITX(0x0008, IP_ACTIVE_LOW, 0, "L",   KEYCODE_L,     IP_JOY_NONE )
	PORT_BITX(0x0010, IP_ACTIVE_LOW, 0, "Pon",   KEYCODE_LALT,   IP_JOY_NONE )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0xff00, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* IN4 - $278880.w*/
	PORT_BIT( 0xffff, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* IN5 - $278882.w*/
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT_IMPULSE(  0x0002, IP_ACTIVE_LOW, IPT_COIN1, 2 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BITX(0x0008, IP_ACTIVE_LOW, IPT_SERVICE, DEF_STR( Service_Mode ), KEYCODE_F2, IP_JOY_NONE )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0xff00, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END


/***************************************************************************
									Mouja
***************************************************************************/

INPUT_PORTS_START( mouja )
	PORT_START	/* IN0 - $478880*/
	PORT_BIT(  0x0001, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER1 )
	PORT_BIT(  0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER1 )
	PORT_BIT(  0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER1 )
	PORT_BIT(  0x0010, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER1 )
	PORT_BIT(  0x0020, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT(  0x0040, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT(  0x0080, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER1 )
	PORT_BIT(  0x0100, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER2 )
	PORT_BIT(  0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER2 )
	PORT_BIT(  0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER2 )
	PORT_BIT(  0x1000, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER2 )
	PORT_BIT(  0x2000, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT(  0x4000, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT(  0x8000, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )

	PORT_START	/* IN1 - $478882*/
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT_IMPULSE(  0x0010, IP_ACTIVE_LOW, IPT_COIN1, 2    )
	PORT_BIT_IMPULSE(  0x0020, IP_ACTIVE_LOW, IPT_COIN2, 2    )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BITX(0x0080, IP_ACTIVE_LOW, IPT_SERVICE, DEF_STR( Service_Mode ), KEYCODE_F2, IP_JOY_NONE )

	PORT_START	/* IN2 - $478884*/
	PORT_DIPNAME( 0x0001, 0x0001, DEF_STR( Free_Play ) )
	PORT_DIPSETTING(      0x0001, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0002, 0x0002, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0004, 0x0004, DEF_STR( Service_Mode ) )
	PORT_DIPSETTING(      0x0004, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0008, 0x0000, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(      0x0008, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0030, 0x0030, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0010, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0030, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( 1C_2C ) )
	PORT_DIPNAME( 0x00c0, 0x00c0, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0040, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x00c0, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0080, DEF_STR( 1C_2C ) )
	PORT_DIPNAME( 0x0300, 0x0300, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(      0x0200, "Easy" )
	PORT_DIPSETTING(      0x0300, "Normal" )
	PORT_DIPSETTING(      0x0100, "Hard" )
	PORT_DIPSETTING(      0x0000, "Hardest" )
	PORT_DIPNAME( 0x0400, 0x0400, "Allow Continue" )
	PORT_DIPSETTING(      0x0000, DEF_STR( No ) )
	PORT_DIPSETTING(      0x0400, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x0800, 0x0000, "Winning Rounds (Player VS Computer)" )
	PORT_DIPSETTING(      0x0800, "1/1" )
	PORT_DIPSETTING(      0x0000, "2/3" )
	PORT_DIPNAME( 0x1000, 0x1000, "Winning Rounds (Player VS Player)" )
	PORT_DIPSETTING(      0x1000, "1/1" )
	PORT_DIPSETTING(      0x0000, "2/3" )
	PORT_DIPNAME( 0x2000, 0x2000, DEF_STR( Unused ) )
	PORT_DIPSETTING(      0x2000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x4000, 0x4000, DEF_STR( Unused ) )
	PORT_DIPSETTING(      0x4000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x8000, 0x8000, DEF_STR( Unused ) )
	PORT_DIPSETTING(      0x8000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )

	PORT_START	/* IN3 - $478886*/
	PORT_BIT(  0xffff, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END


/***************************************************************************
								Pang Poms
***************************************************************************/

INPUT_PORTS_START( pangpoms )
	PORT_START	/* IN0 - $800004*/
	COINS

	PORT_START	/* IN1 - $800006*/
	JOY_LSB(1, BUTTON1, BUTTON2, UNKNOWN, UNKNOWN)

	PORT_START	/* IN2 - $800008*/
	JOY_LSB(2, BUTTON1, BUTTON2, UNKNOWN, UNKNOWN)

	PORT_START	/* IN3 - $80000a*/
	COINAGE_DSW

	PORT_START	/* IN4 - $80000c*/
	PORT_DIPNAME( 0x0003, 0x0003, "Time Speed" )
	PORT_DIPSETTING(      0x0000, "Slowest" )	/* 60 (1 game sec. lasts x/60 real sec.)*/
	PORT_DIPSETTING(      0x0001, "Slow"    )	/* 90*/
	PORT_DIPSETTING(      0x0003, "Normal"  )	/* 120*/
	PORT_DIPSETTING(      0x0002, "Fast"    )	/* 150*/
	PORT_DIPNAME( 0x000c, 0x000c, DEF_STR( Lives ) )
	PORT_DIPSETTING(      0x0008, "1" )
	PORT_DIPSETTING(      0x0004, "2" )
	PORT_DIPSETTING(      0x000c, "3" )
	PORT_DIPSETTING(      0x0000, "4" )
	PORT_DIPNAME( 0x0030, 0x0020, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(      0x0020, "400k and 800k" )
	PORT_DIPSETTING(      0x0030, "400k" )
	PORT_DIPSETTING(      0x0010, "800k" )
	PORT_DIPSETTING(      0x0000, "None" )
	PORT_DIPNAME( 0x0040, 0x0040, "Allow Continue" )
	PORT_DIPSETTING(      0x0000, DEF_STR( No ) )
	PORT_DIPSETTING(      0x0040, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x0080, 0x0080, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0080, DEF_STR( On ) )

	PORT_START	/* IN5 - $80000e*/
	PORT_BIT(  0xffff, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END


/***************************************************************************
								Poitto!
***************************************************************************/

INPUT_PORTS_START( poitto )
	PORT_START	/* IN0 - $800000*/
	COINS

	PORT_START	/* IN1 - $800002*/
	JOY_LSB(1, BUTTON1, UNKNOWN, UNKNOWN, UNKNOWN)		/* BUTTON2 and BUTTON3 in "test mode" only*/
	JOY_MSB(2, BUTTON1, UNKNOWN, UNKNOWN, UNKNOWN)		/* BUTTON2 and BUTTON3 in "test mode" only*/

	PORT_START	/* IN2 - $800004*/
	COINAGE_DSW

	PORT_DIPNAME( 0x0300, 0x0300, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(      0x0000, "Easy" )
	PORT_DIPSETTING(      0x0300, "Normal" )
	PORT_DIPSETTING(      0x0200, "Hard" )
	PORT_DIPSETTING(      0x0100, "Hardest" )
	PORT_DIPNAME( 0x0400, 0x0400, DEF_STR( Unused ) )
	PORT_DIPSETTING(      0x0400, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0800, 0x0800, DEF_STR( Unused ) )
	PORT_DIPSETTING(      0x0800, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x1000, 0x1000, DEF_STR( Unused ) )
	PORT_DIPSETTING(      0x1000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x2000, 0x2000, "Allow Continue" )
	PORT_DIPSETTING(      0x0000, DEF_STR( No ) )
	PORT_DIPSETTING(      0x2000, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x4000, 0x4000, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x4000, DEF_STR( On ) )
	PORT_DIPNAME( 0x8000, 0x8000, DEF_STR( Unused ) )
	PORT_DIPSETTING(      0x8000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )

	PORT_START	/* IN3 - $800006*/
	PORT_BIT(  0xffff, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END


/***************************************************************************
								Puzzli
***************************************************************************/

INPUT_PORTS_START( puzzli )
	PORT_START	/* IN0 - $c00000*/
	COINS

	PORT_START	/* IN1 - $c00002*/
	JOY_LSB(1, BUTTON1, BUTTON2, UNKNOWN, UNKNOWN)		/* BUTTON3 in "test mode" only*/
	JOY_MSB(2, BUTTON1, BUTTON2, UNKNOWN, UNKNOWN)		/* BUTTON3 in "test mode" only*/

	PORT_START	/* IN2 - $c00004*/
	COINAGE_DSW

	PORT_DIPNAME( 0x0300, 0x0300, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(      0x0200, "Easy" )
	PORT_DIPSETTING(      0x0300, "Normal" )
/*	PORT_DIPSETTING(      0x0100, "Normal" )			*/ /* Duplicated setting*/
	PORT_DIPSETTING(      0x0000, "Hard" )
	PORT_DIPNAME( 0x0400, 0x0400, "Join In" )
	PORT_DIPSETTING(      0x0000, DEF_STR( No ) )
	PORT_DIPSETTING(      0x0400, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x0800, 0x0800, "2 Players Game" )
	PORT_DIPSETTING(      0x0000, "1 Credit" )
	PORT_DIPSETTING(      0x0800, "2 Credits" )
	PORT_DIPNAME( 0x1000, 0x1000, "Winning Rounds (Player VS Player)" )
	PORT_DIPSETTING(      0x0000, "1/1" )
	PORT_DIPSETTING(      0x1000, "2/3" )
	PORT_DIPNAME( 0x2000, 0x2000, "Allow Continue" )
	PORT_DIPSETTING(      0x0000, DEF_STR( No ) )
	PORT_DIPSETTING(      0x2000, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x4000, 0x4000, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x4000, DEF_STR( On ) )
	PORT_DIPNAME( 0x8000, 0x8000, DEF_STR( Unused ) )
	PORT_DIPSETTING(      0x8000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )

	PORT_START	/* IN3 - $c00006*/
	PORT_BIT(  0xffff, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END


/***************************************************************************
								Sankokushi
***************************************************************************/

INPUT_PORTS_START( 3kokushi )
	PORT_START	/* IN0 - $c00000*/
	COINS

	PORT_START	/* IN1 - $c00002*/
	JOY_LSB(1, BUTTON1, BUTTON2, UNKNOWN, UNKNOWN)
	JOY_MSB(2, BUTTON1, BUTTON2, UNKNOWN, UNKNOWN)

	PORT_START	/* IN2 - $c00004*/
	PORT_DIPNAME( 0x0007, 0x0007, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0001, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0007, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0006, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x0005, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x0004, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(      0x0003, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( 1C_6C ) )
	PORT_DIPNAME( 0x0038, 0x0038, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0008, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0038, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0030, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x0028, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(      0x0018, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(      0x0010, DEF_STR( 1C_6C ) )
	PORT_DIPNAME( 0x0040, 0x0040, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(      0x0040, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0080, 0x0080, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0080, DEF_STR( On ) )
	PORT_DIPNAME( 0x0300, 0x0300, DEF_STR( Difficulty ) )		/* Timer speed*/
	PORT_DIPSETTING(      0x0200, "Easy" )				/*   Slow*/
	PORT_DIPSETTING(      0x0300, "Normal" )				/*   Normal*/
	PORT_DIPSETTING(      0x0100, "Hard" )				/*   Fast*/
	PORT_DIPSETTING(      0x0000, "Hardest" )				/*   Fastest*/
	PORT_DIPNAME( 0x0400, 0x0400, DEF_STR( Unused ) )
	PORT_DIPSETTING(      0x0400, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0800, 0x0800, DEF_STR( Unused ) )
	PORT_DIPSETTING(      0x0800, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x1000, 0x1000, "Allow Continue" )
	PORT_DIPSETTING(      0x0000, DEF_STR( No ) )
	PORT_DIPSETTING(      0x1000, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x2000, 0x2000, DEF_STR( Free_Play ) )
	PORT_DIPSETTING(      0x2000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0xc000, 0xc000, "Helps" )
	PORT_DIPSETTING(      0x0000, "1" )
	PORT_DIPSETTING(      0x4000, "2" )
	PORT_DIPSETTING(      0xc000, "3" )
	PORT_DIPSETTING(      0x8000, "4" )
INPUT_PORTS_END


/***************************************************************************
								Pururun
***************************************************************************/

INPUT_PORTS_START( pururun )
	PORT_START	/* IN0 - $400000*/
	COINS

	PORT_START	/* IN1 - $400002*/
	JOY_LSB(1, BUTTON1, BUTTON2, UNKNOWN, UNKNOWN)		/* BUTTON3 in "test mode" only*/
	JOY_MSB(2, BUTTON1, BUTTON2, UNKNOWN, UNKNOWN)		/* BUTTON3 in "test mode" only*/

	PORT_START	/* IN2 - $400004*/
	COINAGE_DSW

	PORT_DIPNAME( 0x0300, 0x0300, DEF_STR( Difficulty ) )		/* Distance to goal*/
	PORT_DIPSETTING(      0x0200, "Easiest" )
	PORT_DIPSETTING(      0x0100, "Easy" )
	PORT_DIPSETTING(      0x0300, "Normal" )
	PORT_DIPSETTING(      0x0000, "Hard" )
	PORT_DIPNAME( 0x0400, 0x0400, "Join In" )
	PORT_DIPSETTING(      0x0000, DEF_STR( No ) )
	PORT_DIPSETTING(      0x0400, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x0800, 0x0800, "2 Players Game" )
	PORT_DIPSETTING(      0x0000, "1 Credit" )
	PORT_DIPSETTING(      0x0800, "2 Credits" )
	PORT_DIPNAME( 0x1000, 0x1000, "Bombs" )
	PORT_DIPSETTING(      0x1000, "1" )
	PORT_DIPSETTING(      0x0000, "2" )
	PORT_DIPNAME( 0x2000, 0x2000, "Allow Continue" )
	PORT_DIPSETTING(      0x0000, DEF_STR( No ) )
	PORT_DIPSETTING(      0x2000, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x4000, 0x4000, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x4000, DEF_STR( On ) )
	PORT_DIPNAME( 0x8000, 0x8000, DEF_STR( Unused ) )
	PORT_DIPSETTING(      0x8000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )

	PORT_START	/* IN3 - $400006 */
	PORT_BIT(  0xffff, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END


INPUT_PORTS_START( gunmast )
	PORT_START
	COINS

	PORT_START
	JOY_LSB(1, BUTTON1, BUTTON2, BUTTON3, UNKNOWN)
	JOY_MSB(2, BUTTON1, BUTTON2, BUTTON3, UNKNOWN)

	PORT_START
	COINAGE_DSW

	PORT_DIPNAME( 0x0300, 0x0300, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(      0x0300, "Normal" )
	PORT_DIPSETTING(      0x0200, "Hard" )
	PORT_DIPSETTING(      0x0100, "Harder" )
	PORT_DIPSETTING(      0x0000, "Hardest" )
	PORT_DIPNAME( 0x0400, 0x0400, "Allow_Continue" )
	PORT_DIPSETTING(      0x0000, DEF_STR( No ) )
	PORT_DIPSETTING(      0x0400, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x0800, 0x0800, "Allow P2 to Join Game" )
	PORT_DIPSETTING(      0x0000, DEF_STR( No ) )
	PORT_DIPSETTING(      0x0800, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x1000, 0x1000, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x1000, DEF_STR( On ) )
	PORT_DIPNAME( 0x2000, 0x2000, DEF_STR( Lives ) )
	PORT_DIPSETTING(      0x2000, "1" )
	PORT_DIPSETTING(      0x0000, "2" )
	PORT_DIPNAME( 0x4000, 0x4000, DEF_STR( Unused ) ) /* Listed as "Unused" */
	PORT_DIPSETTING(      0x4000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x8000, 0x8000, DEF_STR( Unused ) ) /* Listed as "Unused" */
	PORT_DIPSETTING(      0x8000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )

	PORT_START
	PORT_BIT(  0xffff, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END


/***************************************************************************
								Sky Alert
***************************************************************************/

/* The game shows wrong values on screen for the "Bonus Life" Dip Switch !
   The wrong values are text which is stored at 0x02671a, and to determine
   which text to display, the routine at 0x0022f2 is called.
   The REAL "Bonus Life" table is stored at 0x0097f6, and to determine what
   are the values, the routine at 0x00974e is called.

   Here is the correspondance between real and fake values :

        Real         Fake
     100K, 400K   100K, 400K
     200K, 400K    50K, 300K
     200K         150K, 500K
       "none"       "none"

*/
INPUT_PORTS_START( skyalert )
	PORT_START	/* IN0 - $400004*/
	COINS

	PORT_START	/* IN1 - $400006*/
	JOY_LSB(1, BUTTON1, BUTTON2, UNKNOWN, UNKNOWN)		/* BUTTON3 in "test mode" only*/

	PORT_START	/* IN2 - $400008*/
	JOY_LSB(2, BUTTON1, BUTTON2, UNKNOWN, UNKNOWN)		/* BUTTON3 in "test mode" only*/

	PORT_START	/* IN3 - $40000a*/
	COINAGE_DSW

	PORT_START	/* IN4 - $40000c*/
	PORT_DIPNAME( 0x0003, 0x0003, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(      0x0002, "Easy" )
	PORT_DIPSETTING(      0x0003, "Normal" )
	PORT_DIPSETTING(      0x0001, "Hard" )
	PORT_DIPSETTING(      0x0000, "Hardest" )
	PORT_DIPNAME( 0x000c, 0x000c, DEF_STR( Lives ) )
	PORT_DIPSETTING(      0x0008, "1" )
	PORT_DIPSETTING(      0x0004, "2" )
	PORT_DIPSETTING(      0x000c, "3" )
	PORT_DIPSETTING(      0x0000, "4" )
	PORT_DIPNAME( 0x0030, 0x0030, DEF_STR( Bonus_Life ) )		/* See notes*/
	PORT_DIPSETTING(      0x0030, "100K, every 400K" )
	PORT_DIPSETTING(      0x0020, "200K, every 400K" )
	PORT_DIPSETTING(      0x0010, "200K" )
	PORT_DIPSETTING(      0x0000, "None" )
	PORT_DIPNAME( 0x0040, 0x0040, "Allow Continue" )
	PORT_DIPSETTING(      0x0000, DEF_STR( No ) )
	PORT_DIPSETTING(      0x0040, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x0080, 0x0080, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0080, DEF_STR( On ) )

	PORT_START	/* IN5 - $40000e*/
	PORT_BIT(  0xffff, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END


/***************************************************************************
							Toride II Adauchi Gaiden
***************************************************************************/

/* I don't really know HOW to describe the effect of IN2 bit 10.
   All I can tell is that is that it affects the levels which are
   proposed, but there is no evidence that one "table" is harder
   than another. */
INPUT_PORTS_START( toride2g )
	PORT_START	/* IN0 - $800000*/
	COINS

	PORT_START	/* IN1 - $800002*/
	JOY_LSB(1, BUTTON1, UNKNOWN, UNKNOWN, UNKNOWN)		/* BUTTON2 and BUTTON3 in "test mode" only*/
	JOY_MSB(2, BUTTON1, UNKNOWN, UNKNOWN, UNKNOWN)		/* BUTTON2 and BUTTON3 in "test mode" only*/

	PORT_START	/* IN2 - $800004*/
	COINAGE_DSW

	PORT_DIPNAME( 0x0300, 0x0300, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(      0x0200, "Easy" )
	PORT_DIPSETTING(      0x0300, "Normal" )
	PORT_DIPSETTING(      0x0100, "Hard" )
	PORT_DIPSETTING(      0x0000, "Hardest" )
	PORT_DIPNAME( 0x0400, 0x0400, "Levels" )				/* See notes*/
	PORT_DIPSETTING(      0x0400, "Table 1" )
	PORT_DIPSETTING(      0x0000, "Table 2" )
	PORT_DIPNAME( 0x0800, 0x0000, "Retry Level On Continue" )
	PORT_DIPSETTING(      0x0000, "Ask Player" )
	PORT_DIPSETTING(      0x0800, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x1000, 0x1000, "2 Players Game" )
	PORT_DIPSETTING(      0x1000, "2 Credits" )
	PORT_DIPSETTING(      0x0000, "1 Credit" )
	PORT_DIPNAME( 0x2000, 0x2000, "Allow Continue" )
	PORT_DIPSETTING(      0x0000, DEF_STR( No ) )
	PORT_DIPSETTING(      0x2000, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x4000, 0x4000, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x4000, DEF_STR( On ) )
	PORT_DIPNAME( 0x8000, 0x8000, DEF_STR( Unused ) )
	PORT_DIPSETTING(      0x8000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )

	PORT_START	/* IN3 - $800006*/
	PORT_BIT(  0xffff, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* BIT 6 !?*/
INPUT_PORTS_END



/***************************************************************************


							Graphics Layouts


***************************************************************************/


/* 8x8x4 tiles */
static struct GfxLayout layout_8x8x4 =
{
	8,8,
	RGN_FRAC(1,1),
	4,
	{ GFX_RAW },
	{ 0 },		/* org displacement */
	{ 4*8 },	/* line modulo */
	32*8		/* char modulo */
};

/* 8x8x8 tiles for later games */
static struct GfxLayout layout_8x8x8h =
{
	8,8,
	RGN_FRAC(1,1),
	8,
	{ GFX_RAW },
	{ 0 },		/* org displacement */
	{ 8*8 },	/* line modulo */
	32*8		/* char modulo (half char step) */
};

/* 16x16x4 tiles for later games */
static struct GfxLayout layout_16x16x4q =
{
	16,16,
	RGN_FRAC(1,1),
	4,
	{ GFX_RAW },
	{ 0 },		/* org displacement */
	{ 8*8 },	/* line modulo */
	32*8		/* char modulo (quarter char step) */
};

/* 16x16x8 tiles for later games */
static struct GfxLayout layout_16x16x8o =
{
	16,16,
	RGN_FRAC(1,1),
	8,
	{ GFX_RAW },
	{ 0 },		/* org displacement */
	{ 16*8 },	/* line modulo */
	32*8		/* char modulo (1/8th char step) */
};

static struct GfxLayout layout_053936 =
{
	8,8,
	RGN_FRAC(1,1),
	8,
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	{ 0*8*8, 1*8*8, 2*8*8, 3*8*8, 4*8*8, 5*8*8, 6*8*8, 7*8*8 },
	8*8*8
};

static struct GfxLayout layout_053936_16 =
{
	16,16,
	RGN_FRAC(1,1),
	8,
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
	8*8*8+0*8, 8*8*8+1*8, 8*8*8+2*8, 8*8*8+3*8, 8*8*8+4*8, 8*8*8+5*8, 8*8*8+6*8, 8*8*8+7*8,
	},

	{ 0*8*8, 1*8*8, 2*8*8, 3*8*8, 4*8*8, 5*8*8, 6*8*8, 7*8*8,
	8*8*8*2+0*8*8,	8*8*8*2+1*8*8,	8*8*8*2+2*8*8,	8*8*8*2+3*8*8,	8*8*8*2+4*8*8,	8*8*8*2+5*8*8,	8*8*8*2+6*8*8,	8*8*8*2+7*8*8,
	},
	8*8*8*4
};

static struct GfxDecodeInfo gfxdecodeinfo_14100[] =
{
	{ REGION_GFX1, 0, &layout_8x8x4,    0x0, 0x200 }, /* [0] 4 Bit Tiles*/
	{ -1 }
};

static struct GfxDecodeInfo gfxdecodeinfo_14220[] =
{
	{ REGION_GFX1, 0, &layout_8x8x4,    0x0, 0x200 }, /* [0] 4 Bit Tiles*/
	{ REGION_GFX1, 0, &layout_8x8x8h,   0x0,  0x20 }, /* [1] 8 Bit Tiles*/
	{ -1 }
};

static struct GfxDecodeInfo gfxdecodeinfo_blzntrnd[] =
{
	{ REGION_GFX1, 0, &layout_8x8x4,    0x0, 0x200 }, /* [0] 4 Bit Tiles*/
	{ REGION_GFX1, 0, &layout_8x8x8h,   0x0,  0x20 }, /* [1] 8 Bit Tiles*/
	{ REGION_GFX3, 0, &layout_053936,   0x0,  0x20 }, /* [2] 053936 Tiles*/
	{ -1 }
};

static struct GfxDecodeInfo gfxdecodeinfo_gstrik2[] =
{
	{ REGION_GFX1, 0, &layout_8x8x4,    0x0, 0x200 }, /* [0] 4 Bit Tiles*/
	{ REGION_GFX1, 0, &layout_8x8x8h,   0x0,  0x20 }, /* [1] 8 Bit Tiles*/
	{ REGION_GFX3, 0, &layout_053936_16,0x0,  0x20 }, /* [2] 053936 Tiles*/
	{ -1 }
};

static struct GfxDecodeInfo gfxdecodeinfo_14300[] =
{
	{ REGION_GFX1, 0, &layout_8x8x4,    0x0, 0x200 }, /* [0] 4 Bit Tiles*/
	{ REGION_GFX1, 0, &layout_8x8x8h,   0x0,  0x20 }, /* [1] 8 Bit Tiles*/
	{ REGION_GFX1, 0, &layout_16x16x4q, 0x0, 0x200 }, /* [2] 4 Bit Tiles 16x16*/
	{ REGION_GFX1, 0, &layout_16x16x8o, 0x0, 0x200 }, /* [2] 8 Bit Tiles 16x16*/
	{ -1 }
};


/***************************************************************************


								Machine Drivers


***************************************************************************/

UPD7810_CONFIG metro_cpu_config =
{
    TYPE_7810,
    metro_io_callback
};

static MACHINE_DRIVER_START( balcube )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 16000000)
	MDRV_CPU_MEMORY(balcube_readmem,balcube_writemem)
	MDRV_CPU_VBLANK_INT(metro_interrupt,10)	/* ? */

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_MACHINE_INIT(metro)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(320, 224)
	MDRV_VISIBLE_AREA(0, 320-1, 0, 224-1)
	MDRV_GFXDECODE(gfxdecodeinfo_14220)
	MDRV_PALETTE_LENGTH(8192)

	MDRV_VIDEO_START(metro_14220)
	MDRV_VIDEO_UPDATE(metro)

	/* sound hardware */
	MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
	MDRV_SOUND_ADD(YMF278B, ymf278b_interface)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( bangball )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 16000000)
	MDRV_CPU_MEMORY(bangball_readmem,bangball_writemem)
	MDRV_CPU_VBLANK_INT(bangball_interrupt,1)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_MACHINE_INIT(metro)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(320, 224)
	MDRV_VISIBLE_AREA(0, 320-1, 0, 224-1)
	MDRV_GFXDECODE(gfxdecodeinfo_14220)
	MDRV_PALETTE_LENGTH(8192)

	MDRV_VIDEO_START(metro_14220)
	MDRV_VIDEO_UPDATE(metro)

	/* sound hardware */
	MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
	MDRV_SOUND_ADD(YMF278B, ymf278b_interface)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( msgogo )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 16000000)
	MDRV_CPU_MEMORY(msgogo_readmem,msgogo_writemem)
	MDRV_CPU_VBLANK_INT(msgogo_interrupt,262)    /* ? */

	
	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_MACHINE_INIT(metro)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(320, 224)
	MDRV_VISIBLE_AREA(0, 320-1, 0, 224-1)
	MDRV_GFXDECODE(gfxdecodeinfo_14220)
	MDRV_PALETTE_LENGTH(8192)

	MDRV_VIDEO_START(metro_14220)
	MDRV_VIDEO_UPDATE(metro)

	/* sound hardware */
	MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
	MDRV_SOUND_ADD(YMF278B, ymf278b_interface)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( daitorid )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 16000000)
	MDRV_CPU_MEMORY(daitorid_readmem,daitorid_writemem)
	MDRV_CPU_VBLANK_INT(metro_interrupt,10)	/* ? */

	MDRV_CPU_ADD(UPD7810, 12000000)
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
	MDRV_CPU_CONFIG(metro_cpu_config)
	MDRV_CPU_MEMORY(daitorid_snd_readmem,daitorid_snd_writemem)
	MDRV_CPU_PORTS(daitorid_snd_readport,daitorid_snd_writeport)

	MDRV_FRAMES_PER_SECOND(58)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_MACHINE_INIT(metro)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(320, 224)
	MDRV_VISIBLE_AREA(0, 320-1, 0, 224-1)
	MDRV_GFXDECODE(gfxdecodeinfo_14220)
	MDRV_PALETTE_LENGTH(8192)

	MDRV_VIDEO_START(metro_14220)
	MDRV_VIDEO_UPDATE(metro)

	/* sound hardware */
	MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
	MDRV_SOUND_ADD(YM2151, ym2151_interface)
	MDRV_SOUND_ADD(OKIM6295, okim6295_intf_2151balanced)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( dharma )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 12000000)
	MDRV_CPU_MEMORY(dharma_readmem,dharma_writemem)
	MDRV_CPU_VBLANK_INT(metro_interrupt,10)	/* ? */

	MDRV_CPU_ADD(UPD7810, 12000000)
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
	MDRV_CPU_CONFIG(metro_cpu_config)
	MDRV_CPU_MEMORY(metro_snd_readmem,metro_snd_writemem)
	MDRV_CPU_PORTS(metro_snd_readport,metro_snd_writeport)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_MACHINE_INIT(metro)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(320, 224)
	MDRV_VISIBLE_AREA(0, 320-1, 0, 224-1)
	MDRV_GFXDECODE(gfxdecodeinfo_14100)
	MDRV_PALETTE_LENGTH(8192)

	MDRV_VIDEO_START(metro_14100)
	MDRV_VIDEO_UPDATE(metro)

	/* sound hardware */
	MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
	MDRV_SOUND_ADD(OKIM6295, okim6295_interface)
	MDRV_SOUND_ADD(YM2413, ym2413_interface)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( karatour )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 12000000)
	MDRV_CPU_MEMORY(karatour_readmem,karatour_writemem)
	MDRV_CPU_VBLANK_INT(karatour_interrupt,10)	/* ? */

	MDRV_CPU_ADD(UPD7810, 12000000)
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
	MDRV_CPU_CONFIG(metro_cpu_config)
	MDRV_CPU_MEMORY(metro_snd_readmem,metro_snd_writemem)
	MDRV_CPU_PORTS(metro_snd_readport,metro_snd_writeport)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_MACHINE_INIT(metro)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(320, 240)
	MDRV_VISIBLE_AREA(0, 320-1, 0, 240-1)
	MDRV_GFXDECODE(gfxdecodeinfo_14100)
	MDRV_PALETTE_LENGTH(8192)

	MDRV_VIDEO_START(metro_14100)
	MDRV_VIDEO_UPDATE(metro)

	/* sound hardware */
	MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
	MDRV_SOUND_ADD(OKIM6295, okim6295_interface)
	MDRV_SOUND_ADD(YM2413, ym2413_interface)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( 3kokushi )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 12000000)
	MDRV_CPU_MEMORY(kokushi_readmem,kokushi_writemem)
	MDRV_CPU_VBLANK_INT(karatour_interrupt,10)	/* ? */

	MDRV_CPU_ADD(UPD7810, 12000000)
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
	MDRV_CPU_CONFIG(metro_cpu_config)
	MDRV_CPU_MEMORY(metro_snd_readmem,metro_snd_writemem)
	MDRV_CPU_PORTS(metro_snd_readport,metro_snd_writeport)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_MACHINE_INIT(metro)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(320, 240)
	MDRV_VISIBLE_AREA(0, 320-1, 0, 240-1)
	MDRV_GFXDECODE(gfxdecodeinfo_14220)
	MDRV_PALETTE_LENGTH(8192)

	MDRV_VIDEO_START(metro_14220)
	MDRV_VIDEO_UPDATE(metro)

	/* sound hardware */
	MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
	MDRV_SOUND_ADD(OKIM6295, okim6295_interface)
	MDRV_SOUND_ADD(YM2413, ym2413_interface)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( lastfort )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 12000000)
	MDRV_CPU_MEMORY(lastfort_readmem,lastfort_writemem)
	MDRV_CPU_VBLANK_INT(metro_interrupt,10)	/* ? */

	MDRV_CPU_ADD(UPD7810, 12000000)
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
	MDRV_CPU_CONFIG(metro_cpu_config)
	MDRV_CPU_MEMORY(metro_snd_readmem,metro_snd_writemem)
	MDRV_CPU_PORTS(metro_snd_readport,metro_snd_writeport)

	MDRV_FRAMES_PER_SECOND(58)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_MACHINE_INIT(metro)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(360, 224)
	MDRV_VISIBLE_AREA(0, 360-1, 0, 224-1)
	MDRV_GFXDECODE(gfxdecodeinfo_14100)
	MDRV_PALETTE_LENGTH(8192)

	MDRV_VIDEO_START(metro_14100)
	MDRV_VIDEO_UPDATE(metro)

	/* sound hardware */
	MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
	MDRV_SOUND_ADD(OKIM6295, okim6295_interface)
	MDRV_SOUND_ADD(YM2413, ym2413_interface)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( dokyusei )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 16000000)
	MDRV_CPU_MEMORY(dokyusei_readmem,dokyusei_writemem)
	MDRV_CPU_VBLANK_INT(dokyusei_interrupt,2)	/* ? */

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_MACHINE_INIT(metro)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(320, 256-32)
	MDRV_VISIBLE_AREA(0, 320-1, 0, 256-32-1)
	MDRV_GFXDECODE(gfxdecodeinfo_14300)
	MDRV_PALETTE_LENGTH(8192)

	MDRV_VIDEO_START(metro_14300)
	MDRV_VIDEO_UPDATE(metro)

	/* sound hardware */
	MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
	MDRV_SOUND_ADD(OKIM6295, okim6295_intf_8kHz)
	MDRV_SOUND_ADD(YM2413, ym2413_intf_8MHz)
MACHINE_DRIVER_END

NVRAM_HANDLER( dokyusp )
{
	data8_t def_data[] = {0x00,0xe0};

	if (read_or_write)
		EEPROM_save(file);
	else
	{
		EEPROM_init(&eeprom_interface_93C46);
		if (file)	EEPROM_load(file);
		else		EEPROM_set_data(def_data,sizeof(def_data)/sizeof(def_data[0]));
	}
}

static MACHINE_DRIVER_START( dokyusp )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 16000000)
	MDRV_CPU_MEMORY(dokyusp_readmem,dokyusp_writemem)
	MDRV_CPU_VBLANK_INT(gakusai_interrupt,1)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_MACHINE_INIT(metro)
	MDRV_NVRAM_HANDLER(dokyusp)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(384, 256-32)
	MDRV_VISIBLE_AREA(0, 384-1, 0, 256-32-1)
	MDRV_GFXDECODE(gfxdecodeinfo_14300)
	MDRV_PALETTE_LENGTH(8192)

	MDRV_VIDEO_START(metro_14300)
	MDRV_VIDEO_UPDATE(metro)

	/* sound hardware */
	MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
	MDRV_SOUND_ADD(OKIM6295, okim6295_intf_16kHz)
	MDRV_SOUND_ADD(YM2413, ym2413_intf_8MHz)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( gakusai )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 16000000)
	MDRV_CPU_MEMORY(gakusai_readmem,gakusai_writemem)
	MDRV_CPU_VBLANK_INT(gakusai_interrupt,1)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_MACHINE_INIT(metro)
	MDRV_NVRAM_HANDLER(93C46)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(320, 240)
	MDRV_VISIBLE_AREA(0, 320-1, 0, 240-1)
	MDRV_GFXDECODE(gfxdecodeinfo_14300)
	MDRV_PALETTE_LENGTH(8192)

	MDRV_VIDEO_START(metro_14300)
	MDRV_VIDEO_UPDATE(metro)

	/* sound hardware */
	MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
	MDRV_SOUND_ADD(OKIM6295, okim6295_intf_16kHz)
	MDRV_SOUND_ADD(YM2413, ym2413_intf_8MHz)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( gakusai2 )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 16000000)
	MDRV_CPU_MEMORY(gakusai2_readmem,gakusai2_writemem)
	MDRV_CPU_VBLANK_INT(gakusai_interrupt,1)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_MACHINE_INIT(metro)
	MDRV_NVRAM_HANDLER(93C46)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(320, 240)
	MDRV_VISIBLE_AREA(0, 320-1, 0, 240-1)
	MDRV_GFXDECODE(gfxdecodeinfo_14300)
	MDRV_PALETTE_LENGTH(8192)

	MDRV_VIDEO_START(metro_14300)
	MDRV_VIDEO_UPDATE(metro)

	/* sound hardware */
	MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
	MDRV_SOUND_ADD(OKIM6295, okim6295_intf_16kHz)
	MDRV_SOUND_ADD(YM2413, ym2413_intf_8MHz)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( pangpoms )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 12000000)
	MDRV_CPU_MEMORY(pangpoms_readmem,pangpoms_writemem)
	MDRV_CPU_VBLANK_INT(metro_interrupt,10)	/* ? */

	MDRV_CPU_ADD(UPD7810, 12000000)
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
	MDRV_CPU_CONFIG(metro_cpu_config)
	MDRV_CPU_MEMORY(metro_snd_readmem,metro_snd_writemem)
	MDRV_CPU_PORTS(metro_snd_readport,metro_snd_writeport)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_MACHINE_INIT(metro)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(360, 224)
	MDRV_VISIBLE_AREA(0, 360-1, 0, 224-1)
	MDRV_GFXDECODE(gfxdecodeinfo_14100)
	MDRV_PALETTE_LENGTH(8192)

	MDRV_VIDEO_START(metro_14100)
	MDRV_VIDEO_UPDATE(metro)

	/* sound hardware */
	MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
	MDRV_SOUND_ADD(OKIM6295, okim6295_interface)
	MDRV_SOUND_ADD(YM2413, ym2413_interface)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( poitto )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 12000000)
	MDRV_CPU_MEMORY(poitto_readmem,poitto_writemem)
	MDRV_CPU_VBLANK_INT(metro_interrupt,10)	/* ? */

	MDRV_CPU_ADD(UPD7810, 12000000)
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
	MDRV_CPU_CONFIG(metro_cpu_config)
	MDRV_CPU_MEMORY(metro_snd_readmem,metro_snd_writemem)
	MDRV_CPU_PORTS(metro_snd_readport,metro_snd_writeport)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_MACHINE_INIT(metro)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(360, 224)
	MDRV_VISIBLE_AREA(0, 360-1, 0, 224-1)
	MDRV_GFXDECODE(gfxdecodeinfo_14100)
	MDRV_PALETTE_LENGTH(8192)

	MDRV_VIDEO_START(metro_14100)
	MDRV_VIDEO_UPDATE(metro)

	/* sound hardware */
	MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
	MDRV_SOUND_ADD(OKIM6295, okim6295_interface)
	MDRV_SOUND_ADD(YM2413, ym2413_interface)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( pururun )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 12000000)
	MDRV_CPU_MEMORY(pururun_readmem,pururun_writemem)
	MDRV_CPU_VBLANK_INT(metro_interrupt,10)	/* ? */

	MDRV_CPU_ADD(UPD7810, 12000000)
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
	MDRV_CPU_CONFIG(metro_cpu_config)
	MDRV_CPU_MEMORY(daitorid_snd_readmem,daitorid_snd_writemem)
	MDRV_CPU_PORTS(daitorid_snd_readport,daitorid_snd_writeport)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_MACHINE_INIT(metro)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(320, 224)
	MDRV_VISIBLE_AREA(0, 320-1, 0, 224-1)
	MDRV_GFXDECODE(gfxdecodeinfo_14100)
	MDRV_PALETTE_LENGTH(8192)

	MDRV_VIDEO_START(metro_14100)
	MDRV_VIDEO_UPDATE(metro)

	/* sound hardware */
	MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
	MDRV_SOUND_ADD(YM2151, ym2151_interface)
	MDRV_SOUND_ADD(OKIM6295, okim6295_intf_2151balanced)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( skyalert )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 12000000)
	MDRV_CPU_MEMORY(skyalert_readmem,skyalert_writemem)
	MDRV_CPU_VBLANK_INT(metro_interrupt,10)	/* ? */

	MDRV_CPU_ADD(UPD7810, 12000000)
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
	MDRV_CPU_CONFIG(metro_cpu_config)
	MDRV_CPU_MEMORY(metro_snd_readmem,metro_snd_writemem)
	MDRV_CPU_PORTS(metro_snd_readport,metro_snd_writeport)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_MACHINE_INIT(metro)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(360, 224)
	MDRV_VISIBLE_AREA(0, 360-1, 0, 224-1)
	MDRV_GFXDECODE(gfxdecodeinfo_14100)
	MDRV_PALETTE_LENGTH(8192)

	MDRV_VIDEO_START(metro_14100)
	MDRV_VIDEO_UPDATE(metro)

	/* sound hardware */
	MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
	MDRV_SOUND_ADD(OKIM6295, okim6295_interface)
	MDRV_SOUND_ADD(YM2413, ym2413_interface)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( toride2g )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 12000000)
	MDRV_CPU_MEMORY(toride2g_readmem,toride2g_writemem)
	MDRV_CPU_VBLANK_INT(metro_interrupt,10)	/* ? */

	MDRV_CPU_ADD(UPD7810, 12000000)
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
	MDRV_CPU_CONFIG(metro_cpu_config)
	MDRV_CPU_MEMORY(metro_snd_readmem,metro_snd_writemem)
	MDRV_CPU_PORTS(metro_snd_readport,metro_snd_writeport)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_MACHINE_INIT(metro)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(320, 224)
	MDRV_VISIBLE_AREA(0, 320-1, 0, 224-1)
	MDRV_GFXDECODE(gfxdecodeinfo_14100)
	MDRV_PALETTE_LENGTH(8192)

	MDRV_VIDEO_START(metro_14100)
	MDRV_VIDEO_UPDATE(metro)

	/* sound hardware */
	MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
	MDRV_SOUND_ADD(OKIM6295, okim6295_interface)
	MDRV_SOUND_ADD(YM2413, ym2413_interface)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( mouja )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 12000000)	/* ??? */
	MDRV_CPU_MEMORY(mouja_readmem,mouja_writemem)
	MDRV_CPU_VBLANK_INT(mouja_interrupt,1)

	MDRV_FRAMES_PER_SECOND(58)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_MACHINE_INIT(metro)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(512, 256)
	MDRV_VISIBLE_AREA(0, 320-1, 0, 224-1)
	MDRV_GFXDECODE(gfxdecodeinfo_14300)
	MDRV_PALETTE_LENGTH(8192)

	MDRV_VIDEO_START(metro_14300)
	MDRV_VIDEO_UPDATE(metro)

	/* sound hardware */
	MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
	MDRV_SOUND_ADD(ADPCM, mouja_adpcm_interface)		/* M6295 special bankrom system */
	MDRV_SOUND_ADD(YM2413, ym2413_interface)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( blzntrnd )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 16000000)
	MDRV_CPU_MEMORY(blzntrnd_readmem,blzntrnd_writemem)
	MDRV_CPU_VBLANK_INT(karatour_interrupt,10)	/* ? */

	MDRV_CPU_ADD(Z80, 8000000)
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
	MDRV_CPU_MEMORY(blzntrnd_sound_readmem, blzntrnd_sound_writemem)
	MDRV_CPU_PORTS(blzntrnd_sound_readport, blzntrnd_sound_writeport)

	MDRV_FRAMES_PER_SECOND(58)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_MACHINE_INIT(metro)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(320, 224)
	MDRV_VISIBLE_AREA(8, 320-8-1, 0, 224-1)
	MDRV_GFXDECODE(gfxdecodeinfo_blzntrnd)
	MDRV_PALETTE_LENGTH(8192)

	MDRV_VIDEO_START(blzntrnd)
	MDRV_VIDEO_UPDATE(metro)

	/* sound hardware */
	MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
	MDRV_SOUND_ADD(YM2610, blzntrnd_ym2610_interface)
MACHINE_DRIVER_END

/* like blzntrnd but new vidstart / gfxdecode for the different bg tilemap */
static MACHINE_DRIVER_START( gstrik2 )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 16000000)
	MDRV_CPU_MEMORY(blzntrnd_readmem,blzntrnd_writemem)
	MDRV_CPU_VBLANK_INT(karatour_interrupt,10)	/* ? */

	MDRV_CPU_ADD(Z80, 8000000)
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
	MDRV_CPU_MEMORY(blzntrnd_sound_readmem, blzntrnd_sound_writemem)
	MDRV_CPU_PORTS(blzntrnd_sound_readport, blzntrnd_sound_writeport)

	MDRV_FRAMES_PER_SECOND(58)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_MACHINE_INIT(metro)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(320, 224)
	MDRV_VISIBLE_AREA(8, 320-8-1, 0, 224-1)
	MDRV_GFXDECODE(gfxdecodeinfo_gstrik2)
	MDRV_PALETTE_LENGTH(8192)

	MDRV_VIDEO_START(gstrik2)
	MDRV_VIDEO_UPDATE(metro)

	/* sound hardware */
	MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
	MDRV_SOUND_ADD(YM2610, blzntrnd_ym2610_interface)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( gunmast )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 12000000)
	MDRV_CPU_MEMORY(pururun_readmem,pururun_writemem)
	MDRV_CPU_VBLANK_INT(metro_interrupt,6)	/* ? */

	MDRV_CPU_ADD(UPD7810, 12000000)
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
	MDRV_CPU_CONFIG(metro_cpu_config)
	MDRV_CPU_MEMORY(daitorid_snd_readmem,daitorid_snd_writemem)
	MDRV_CPU_PORTS(daitorid_snd_readport,daitorid_snd_writeport)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_MACHINE_INIT(metro)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(320, 224)
	MDRV_VISIBLE_AREA(0, 320-1, 0, 224-1)
	MDRV_GFXDECODE(gfxdecodeinfo_14100)
	MDRV_PALETTE_LENGTH(8192)

	MDRV_VIDEO_START(metro_14100)
	MDRV_VIDEO_UPDATE(metro)

	/* sound hardware */
	MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
	MDRV_SOUND_ADD(YM2151, ym2151_interface)
	MDRV_SOUND_ADD(OKIM6295, okim6295_intf_2151balanced)
MACHINE_DRIVER_END


/***************************************************************************


								ROMs Loading


***************************************************************************/

static void metro_common(void)
{
	int i;

	/*
	  Tiles can be either 4-bit or 8-bit, and both depths can be used at the same
	  time. The transparent pen is the last one, that is 15 or 255. To make
	  tilemap.c handle that, we invert gfx data so the transparent pen becomes 0
	  for both tile depths.
	*/
	for (i = 0;i < memory_region_length(REGION_GFX1);i++)
		memory_region(REGION_GFX1)[i] ^= 0xff;

	if (memory_region(REGION_GFX3))	/* blzntrnd */
		for (i = 0;i < memory_region_length(REGION_GFX3);i++)
			memory_region(REGION_GFX3)[i] ^= 0xff;

	requested_int[0] = 0;
	requested_int[1] = 0;
	requested_int[2] = 0;
	requested_int[3] = 0;
	requested_int[4] = 0;
	requested_int[5] = 0;
	requested_int[6] = 0;
	requested_int[7] = 0;

	irq_line	=	2;

	blitter_bit	=	2;

	*metro_irq_enable = 0;
}


static DRIVER_INIT( metro )
{
	metro_common();

	porta = 0x00;
	portb = 0x00;
	busy_sndcpu = 0;
	metro_sound_rombank_w(0, 0x00);
}

static DRIVER_INIT( karatour )
{
	data16_t *RAM = (data16_t *) memory_region( REGION_USER1 );
int i;
	metro_vram_0 = RAM + (0x20000/2) * 0;
	metro_vram_1 = RAM + (0x20000/2) * 1;
	metro_vram_2 = RAM + (0x20000/2) * 2;
for (i = 0;i < memory_region_length(REGION_USER1)/2;i++)
	RAM[i] = rand();

	init_metro();
}

static DRIVER_INIT( daitorid )
{
	metro_common();

	porta = 0x00;
	portb = 0x00;
	busy_sndcpu = 0;
	daitorid_sound_rombank_w(0, 0x00);
}


/* Unscramble the GFX ROMs */
static DRIVER_INIT( balcube )
{
	const int region	=	REGION_GFX1;

	const size_t len	=	memory_region_length(region);
	data8_t *src		=	memory_region(region);
	data8_t *end		=	memory_region(region) + len;

	while(src < end)
	{
		const unsigned char scramble[16] =
		 { 0x0,0x8,0x4,0xc,0x2,0xa,0x6,0xe,0x1,0x9,0x5,0xd,0x3,0xb,0x7,0xf };

		unsigned char data;

		data  =  *src;
		*src  =  (scramble[data & 0xF] << 4) | scramble[data >> 4];
		src  +=  2;
	}

	metro_common();
	irq_line = 1;
}

static DRIVER_INIT( blzntrnd )
{
	metro_common();
	irq_line = 1;
}

static DRIVER_INIT( mouja )
{
	int step;

	metro_common();
	irq_line = -1;	/* split interrupt handlers */

	/* generate the OKI6295 volume table */
	for (step = 0; step < 16; step++)
	{
		double out = 256.0;
		int vol = step;

		/* 3dB per step */
		while (vol-- > 0)
			out /= 1.412537545;	/* = 10 ^ (3/20) = 3dB */
		volume_table[step] = (UINT32)out;
	}

	mouja_irq_timer = timer_alloc(mouja_irq_callback);
}

static DRIVER_INIT( gakusai )
{
	metro_common();
	irq_line = -1;
	blitter_bit = 3;
}


/***************************************************************************

Bal Cube
Metro 1996

            7                             1
            YRW801-M                      2
   33.369MHz YMF278B                      3
                                          4



                     16MHz           Imagetek
                6     5              14220
                84256 84256
                68000-16                 52258-20  61C640-20
                             26.666MHz   52258-20

***************************************************************************/

ROM_START( balcube )
	ROM_REGION( 0x080000, REGION_CPU1, 0 )		/* 68000 Code */
	ROM_LOAD16_BYTE( "6", 0x000000, 0x040000, CRC(c400f84d) SHA1(416eb82ec1201d24d9d964191a5a1792c9445923) )
	ROM_LOAD16_BYTE( "5", 0x000001, 0x040000, CRC(15313e3f) SHA1(10a8702016f223194dc91875b4736253fd47dbb8) )

	ROM_REGION( 0x200000, REGION_GFX1, 0 )	/* Gfx + Data (Addressable by CPU & Blitter) */
	ROMX_LOAD( "2", 0x000000, 0x080000, CRC(492ca8f0) SHA1(478336a462a2bfc288cf91262314f5767f8c707d) , ROM_GROUPWORD | ROM_SKIP(6))
	ROMX_LOAD( "4", 0x000002, 0x080000, CRC(d1acda2c) SHA1(f58015302af6c864523d48bdf8f8a4383b69fa9d) , ROM_GROUPWORD | ROM_SKIP(6))
	ROMX_LOAD( "1", 0x000004, 0x080000, CRC(0ea3d161) SHA1(63ae430a19e777ce82b41ab02baef3bb224c7557) , ROM_GROUPWORD | ROM_SKIP(6))
	ROMX_LOAD( "3", 0x000006, 0x080000, CRC(eef1d3b4) SHA1(be535963c00390e34a2305586397a16325f3c3c0) , ROM_GROUPWORD | ROM_SKIP(6))

	ROM_REGION( 0x280000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "yrw801-m", 0x000000, 0x200000, CRC(2a9d8d43) SHA1(32760893ce06dbe3930627755ba065cc3d8ec6ca) )	    /* Yamaha YRW801 2MB ROM with samples for the OPL4.*/
	ROM_LOAD( "7",        0x200000, 0x080000, CRC(f769287d) SHA1(dd0f781b4a1a1fd6bf0a50048b4996f3cf41e155) )	    /* PCM 16 Bit (Signed)*/
ROM_END


/***************************************************************************

Bang Bang Ball
(c)1996 Banpresto/Kunihiko Tashiro/Goodhouse

CPU  : TMP68HC000P-16
Sound: YAMAHA OPL YMF278B-F
OSC  : 16.0000MHz (OSC1) 26.6660MHz (OSC2) 33.869?MHz (OSC3)

ROMs:
rom#005.u19 - Main programs (27c020)
rom#006.u18 /

rom#007.u49 - Sound samples (27c040)
yrw801-m.u52 - Wave data ROM (44pin SOP 16M mask (LH537019))

bp963a.u27 - Graphics (mask, read as 27c800)
bp963a.u28 |
bp963a.u29 |
bp963a.u30 /

PLD:
ALTERA EPM7032LC44-15T D9522

Custom chip:
Imagetek, Inc. I4220 071 9403EK701

***************************************************************************/

ROM_START( bangball )
	ROM_REGION( 0x080000, REGION_CPU1, 0 )		/* 68000 Code */
	ROM_LOAD16_BYTE( "rom#006.u18", 0x000000, 0x040000, CRC(0e4124bc) SHA1(f5cd762df4e822ab5c8dba6f276b3366895235d1) )
	ROM_LOAD16_BYTE( "rom#005.u19", 0x000001, 0x040000, CRC(3fa08587) SHA1(8fdafdde5e77d077b5cd8f94f97b5430fe062936) )

	ROM_REGION( 0x400000, REGION_GFX1, 0 )	/* Gfx + Data (Addressable by CPU & Blitter) */
	ROMX_LOAD( "bp963a.u30", 0x000000, 0x100000, CRC(b0ca8e39) SHA1(f2eb1d07cd10050c234f0b418146c742b496f196) , ROM_GROUPWORD | ROM_SKIP(6))
	ROMX_LOAD( "bp963a.u29", 0x000002, 0x100000, CRC(d934468f) SHA1(b93353bf2302b68a297d71fc9d91dc55c1cccce4) , ROM_GROUPWORD | ROM_SKIP(6))
	ROMX_LOAD( "bp963a.u28", 0x000004, 0x100000, CRC(96d03c6a) SHA1(6257585721291e5a5ce311c2873c9e1e1dac2fc6) , ROM_GROUPWORD | ROM_SKIP(6))
	ROMX_LOAD( "bp963a.u27", 0x000006, 0x100000, CRC(5e3c7732) SHA1(e8c442a8038921ae3de48ce52923d25cb97e36ea) , ROM_GROUPWORD | ROM_SKIP(6))

	ROM_REGION( 0x280000, REGION_SOUND1, ROMREGION_SOUNDONLY )
	ROM_LOAD( "yrw801-m",    0x000000, 0x200000, CRC(2a9d8d43) SHA1(32760893ce06dbe3930627755ba065cc3d8ec6ca) )
	ROM_LOAD( "rom#007.u49", 0x200000, 0x080000, CRC(04cc91a9) SHA1(e5cf6055a0803f4ad44919090cd147702e805d88) )
ROM_END


/***************************************************************************

Blazing Tornado
(c)1994 Human

CPU:	68000-16
Sound:	Z80-8
	YMF286K
OSC:	16.0000MHz
	26.666MHz
Chips:	Imagetek 14220 071
	Konami 053936 (PSAC2)

***************************************************************************/

ROM_START( blzntrnd )
	ROM_REGION( 0x200000, REGION_CPU1, 0 )	/* 68000 */
	ROM_LOAD16_BYTE( "1k.bin", 0x000000, 0x80000, CRC(b007893b) SHA1(609363449c0218b8a38de72d37c66e6f3bb4f8cd) )
	ROM_LOAD16_BYTE( "2k.bin", 0x000001, 0x80000, CRC(ec173252) SHA1(652d70055d2799442beede1ae68e54551931068f) )
	ROM_LOAD16_BYTE( "3k.bin", 0x100000, 0x80000, CRC(1e230ba2) SHA1(ca96c82d57a6b5bacc1bfd2f7965503c2a6e162f) )
	ROM_LOAD16_BYTE( "4k.bin", 0x100001, 0x80000, CRC(e98ca99e) SHA1(9346fc0d419add23eaceb5843c505f3ffa69e495) )

	ROM_REGION( 0x20000, REGION_CPU2, 0 )	/* Z80 */
	ROM_LOAD( "rom5.bin", 0x0000, 0x20000, CRC(7e90b774) SHA1(abd0eda9eababa1f7ab17a2f60534dcebda33c9c) )

	ROM_REGION( 0x1800000, REGION_GFX1, 0 )	/* Gfx + Data (Addressable by CPU & Blitter) */
	ROMX_LOAD( "rom142.bin", 0x0000000, 0x200000, CRC(a7200598) SHA1(f8168a94abc380308901303a69cbd15097019797) , ROM_GROUPWORD | ROM_SKIP(6))
	ROMX_LOAD( "rom186.bin", 0x0000002, 0x200000, CRC(6ee28ea7) SHA1(b33bcbf16423999135d96a62bf25c6ff23031f2a) , ROM_GROUPWORD | ROM_SKIP(6))
	ROMX_LOAD( "rom131.bin", 0x0000004, 0x200000, CRC(c77e75d3) SHA1(8ad716d4e37d6efe478a8e49feb4e68283310890) , ROM_GROUPWORD | ROM_SKIP(6))
	ROMX_LOAD( "rom175.bin", 0x0000006, 0x200000, CRC(04a84f9b) SHA1(83aabbc1c7ab06b351168153335f3c2f91fba0e9) , ROM_GROUPWORD | ROM_SKIP(6))
	ROMX_LOAD( "rom242.bin", 0x0800000, 0x200000, CRC(1182463f) SHA1(6fa2a0b3186a3542b43926e3f37714b78a890542) , ROM_GROUPWORD | ROM_SKIP(6))
	ROMX_LOAD( "rom286.bin", 0x0800002, 0x200000, CRC(384424fc) SHA1(f89d43756bd38515a223fe4ffbed3a44c673ae28) , ROM_GROUPWORD | ROM_SKIP(6))
	ROMX_LOAD( "rom231.bin", 0x0800004, 0x200000, CRC(f0812362) SHA1(9f8be51f60f7baf72f9de8352e4e13d730f85903) , ROM_GROUPWORD | ROM_SKIP(6))
	ROMX_LOAD( "rom275.bin", 0x0800006, 0x200000, CRC(184cb129) SHA1(8ffb3cdc7e0d227b6f0a7962bc6d853c6b84c8d2) , ROM_GROUPWORD | ROM_SKIP(6))
	ROMX_LOAD( "rom342.bin", 0x1000000, 0x200000, CRC(e527fee5) SHA1(e5de1e134d95aa7a48695183189924061482e3a3) , ROM_GROUPWORD | ROM_SKIP(6))
	ROMX_LOAD( "rom386.bin", 0x1000002, 0x200000, CRC(d10b1401) SHA1(0eb75a283000a8b19a14177461b6f335c9d9dec2) , ROM_GROUPWORD | ROM_SKIP(6))
	ROMX_LOAD( "rom331.bin", 0x1000004, 0x200000, CRC(4d909c28) SHA1(fb9bb824e518f67713799ed2c0159a7bd70f35c4) , ROM_GROUPWORD | ROM_SKIP(6))
	ROMX_LOAD( "rom375.bin", 0x1000006, 0x200000, CRC(6eb4f97c) SHA1(c7f006230cbf10e706b0362eeed34655a3aef1a5) , ROM_GROUPWORD | ROM_SKIP(6))

	ROM_REGION( 0x200000, REGION_GFX3, ROMREGION_DISPOSE )	/* 053936 gfx data */
	ROM_LOAD( "rom9.bin", 0x000000, 0x200000, CRC(37ca3570) SHA1(3374c586bf84583fa33f2793c4e8f2f61a0cab1c) )

	ROM_REGION( 0x080000, REGION_SOUND1, ROMREGION_SOUNDONLY )	/* Samples */
	ROM_LOAD( "rom8.bin", 0x000000, 0x080000, CRC(565a4086) SHA1(bd5780acfa5affa8705acbfccb0af16bac8ed298) )

	ROM_REGION( 0x400000, REGION_SOUND2, ROMREGION_SOUNDONLY )	/* ? YRW801-M ? */
	ROM_LOAD( "rom6.bin", 0x000000, 0x200000, CRC(8b8819fc) SHA1(5fd9d2b5088cb676c11d32cac7ba8c5c18e31b64) )
	ROM_LOAD( "rom7.bin", 0x200000, 0x200000, CRC(0089a52b) SHA1(d643ac122d62557de27f06ba1413ef757a45a927) )
ROM_END

/*

Grand Striker 2
Human Entertainment, 1996

PCB Layout
----------

HUM-003(A)
|-----------------------------------------------------------------------|
|           YM3016 ROM8.22  ROM342.88  ROM386.87  ROM331.86  ROM375.85  |
|                                                                       |
| 6264  YM2610         ROM142.80  ROM186.79  ROM131.78  ROM175.77       |
|                                                                       |
|                  ROM7.27  ROM442.92  ROM486.91  ROM431.90  ROM475.89  |
|                                                                       |
|          PAL         ROM242.84  ROM286.83  ROM231.82  ROM275.81       |
|  SPRG.30                                                              |
|  PAL     Z80     ROM6.23                                              |
|                                                                       |
|J                                                                      |
|A                                                                      |
|M                                               |--------|             |
|M                   PRG2  PRG3                  |IMAGETEK|   6264      |
|A                                               |14220   |             |
|                    PRG0  PRG1                  |--------|             |
|     16MHz  68000   62256  62256    26.666MHz                          |
|                                                                       |
|     DSW1                                                              |
|     DSW2   EPM7032         |------|  62256  62256                     |
|     DSW3            6116   |053936|  62256  62256                     |
|     DSW4            6116   |PSAC2 |                   PAL             |
|                            |------|                          ROM9.60  |
|-----------------------------------------------------------------------|

Notes:
       68000 clock: 16.000MHz
         Z80 clock: 8.000MHz
      YM2610 clock: 8.000MHz
             VSync: 58Hz
             HSync: 15.11kHz

*/

ROM_START( gstrik2 )
	ROM_REGION( 0x200000, REGION_CPU1, 0 )	/* 68000 */
	ROM_LOAD16_BYTE( "prg0.107", 0x000000, 0x80000, CRC(e60a8c19) SHA1(19be6cfcb60ede6fd4eb2e14914b174107c4b52d) )
	ROM_LOAD16_BYTE( "prg1.108", 0x000001, 0x80000, CRC(853f6f7c) SHA1(8fb9d7cd0390f620560a1669bb13f2033eed7c81) )
	ROM_LOAD16_BYTE( "prg2.109", 0x100000, 0x80000, CRC(ead86919) SHA1(eb9b68dff4e08d90ac90043c7f3021914caa007d) )
	ROM_LOAD16_BYTE( "prg3.110", 0x100001, 0x80000, CRC(e0b026e3) SHA1(05f75c0432efda3dec0372199382e310bb268fba) )

	ROM_REGION( 0x20000, REGION_CPU2, 0 )	/* Z80 */
	ROM_LOAD( "sprg.30", 0x0000, 0x20000, CRC(aeef6045) SHA1(61b8c89ca495d3aac79e53413a85dd203db816f3) )

	ROM_REGION( 0x1000000, REGION_GFX1, 0 )	/* Gfx + Data (Addressable by CPU & Blitter) */
	ROMX_LOAD( "chr0.80", 0x0000000, 0x200000, CRC(f63a52a9) SHA1(1ad52bb3a051eaffe8fb6ba49d4fc1d0b6144156) , ROM_GROUPWORD | ROM_SKIP(6))
	ROMX_LOAD( "chr1.79", 0x0000002, 0x200000, CRC(4110c184) SHA1(90ccb3d50eff7a655336cfa9c072f7213589e64c) , ROM_GROUPWORD | ROM_SKIP(6))
	ROMX_LOAD( "chr2.78", 0x0000004, 0x200000, CRC(ddb4b9ee) SHA1(0e2c151c3690b9c3d298dda8842e283660d37386) , ROM_GROUPWORD | ROM_SKIP(6))
	ROMX_LOAD( "chr3.77", 0x0000006, 0x200000, CRC(5ab367db) SHA1(adf8749451f4583f8e9e00ab61f3408d804a7265) , ROM_GROUPWORD | ROM_SKIP(6))
	ROMX_LOAD( "chr4.84", 0x0800000, 0x200000, CRC(77d7ef99) SHA1(8f5cf72f5919fe9363e7549e0bb1b3ee633cec3b) , ROM_GROUPWORD | ROM_SKIP(6))
	ROMX_LOAD( "chr5.83", 0x0800002, 0x200000, CRC(a4d49e95) SHA1(9789bacba7876100e0f0293f54c81def545ed068) , ROM_GROUPWORD | ROM_SKIP(6))
	ROMX_LOAD( "chr6.82", 0x0800004, 0x200000, CRC(32eb33b0) SHA1(2ea06484ca326b44a35ee470343147a9d91d5626) , ROM_GROUPWORD | ROM_SKIP(6))
	ROMX_LOAD( "chr7.81", 0x0800006, 0x200000, CRC(2d30a21e) SHA1(749e86b7935ef71556eaee4caf6f954634e9bcbf) , ROM_GROUPWORD | ROM_SKIP(6))
	/* not populated */
/*	ROMX_LOAD( "chr8.88", 0x1000000, 0x200000, CRC() SHA1() , ROM_GROUPWORD | ROM_SKIP(6))*/
/*	ROMX_LOAD( "chr9.87", 0x1000002, 0x200000, CRC() SHA1() , ROM_GROUPWORD | ROM_SKIP(6))*/
/*	ROMX_LOAD( "chr10.86", 0x1000004, 0x200000, CRC() SHA1() , ROM_GROUPWORD | ROM_SKIP(6))*/
/*	ROMX_LOAD( "chr11.85", 0x1000006, 0x200000, CRC() SHA1() , ROM_GROUPWORD | ROM_SKIP(6))*/
/*	ROMX_LOAD( "chr12.92", 0x1800000, 0x200000, CRC() SHA1() , ROM_GROUPWORD | ROM_SKIP(6))*/
/*	ROMX_LOAD( "chr13.91", 0x1800002, 0x200000, CRC() SHA1() , ROM_GROUPWORD | ROM_SKIP(6))*/
/*	ROMX_LOAD( "chr14.90", 0x1800004, 0x200000, CRC() SHA1() , ROM_GROUPWORD | ROM_SKIP(6))*/
/*	ROMX_LOAD( "chr15.89", 0x1800006, 0x200000, CRC() SHA1() , ROM_GROUPWORD | ROM_SKIP(6))*/

	ROM_REGION( 0x200000, REGION_GFX3, ROMREGION_DISPOSE )	/* 053936 gfx data */
	ROM_LOAD( "psacrom.60", 0x000000, 0x200000,  CRC(73f1f279) SHA1(1135b2b1eb4c52249bc12ee178340bbb202a94c8) )

	ROM_REGION( 0x200000, REGION_SOUND1, ROMREGION_SOUNDONLY )	/* Samples */
	ROM_LOAD( "sndpcm-b.22", 0x000000, 0x200000, CRC(a5d844d2) SHA1(18d644545f0844e66aa53775b67b0a29c7b7c31b) )

	ROM_REGION( 0x400000, REGION_SOUND2, ROMREGION_SOUNDONLY )	/* ? YRW801-M ? */
	ROM_LOAD( "sndpcm-a.23", 0x000000, 0x200000, CRC(e6d32373) SHA1(8a79d4ea8b27d785fffd80e38d5ae73b7cea7304) )
	/* ROM7.27 not populated?  */
ROM_END

/***************************************************************************

Daitoride
Metro 1995

MTR5260-A

                                 12MHz  6116
                   YM2151          DT7  DT8
                            M6295
     7C199                             78C10
     7C199       Imagetek14220
     61C64

                  68000-16             DT1
                  32MHz    52258       DT2
   SW1                     52258       DT3
   SW2            DT6  DT5             DT4

***************************************************************************/

ROM_START( daitorid )
	ROM_REGION( 0x040000, REGION_CPU1, 0 )		/* 68000 Code */
	ROM_LOAD16_BYTE( "dt-ja-5.19e", 0x000000, 0x020000, CRC(441efd77) SHA1(18b255f42ba7a180535f0897aaeebe5d2a33df46) )
	ROM_LOAD16_BYTE( "dt-ja-6.19c", 0x000001, 0x020000, CRC(494f9cc3) SHA1(b88af581fee9e2d94a12a5c1fed0797614bb738e) )

	ROM_REGION( 0x02c000, REGION_CPU2, 0 )		/* NEC78C10 Code */
	ROM_LOAD( "dt-ja-8.3h", 0x000000, 0x004000, CRC(0351ad5b) SHA1(942c1cbb52bf2933aea4209335c1bc4cdd1cc3dd) )
	ROM_CONTINUE(           0x010000, 0x01c000 )

	ROM_REGION( 0x200000, REGION_GFX1, 0 )	/* Gfx + Data (Addressable by CPU & Blitter) */
	ROMX_LOAD( "dt-ja-2.14h", 0x000000, 0x080000, CRC(56881062) SHA1(150a8f043e61b28c22d0f898aea61853d1accddc) , ROM_GROUPWORD | ROM_SKIP(6))
	ROMX_LOAD( "dt-ja-4.18h", 0x000002, 0x080000, CRC(85522e3b) SHA1(2c6e7c8ad01d39843669ef1afe7a0843ea6c107c) , ROM_GROUPWORD | ROM_SKIP(6))
	ROMX_LOAD( "dt-ja-1.12h", 0x000004, 0x080000, CRC(2a220bf2) SHA1(553dea2ab42d845b2e91930219fe8df026748642) , ROM_GROUPWORD | ROM_SKIP(6))
	ROMX_LOAD( "dt-ja-3.16h", 0x000006, 0x080000, CRC(fd1f58e0) SHA1(b4bbe94127ae59d4c899d09862703c374c8f4746) , ROM_GROUPWORD | ROM_SKIP(6))

	ROM_REGION( 0x040000, REGION_SOUND1, ROMREGION_SOUNDONLY )	/* Samples */
	ROM_LOAD( "dt-ja-7.3f", 0x000000, 0x040000, CRC(0d888cde) SHA1(fa871fc34f8b8ff0eebe47f338733e4f9fe65b76) )
ROM_END


/***************************************************************************

Dharma Doujou
Metro 1994


                  M6395  JA-7 JA-8

     26.666MHz          NEC78C10
      7C199
      7C199
      7C199               JB-1
                          JB-2
                          JB-3
           68000-12       JB-4

           24MHz
                  6264
                  6264
           JC-5 JC-6

***************************************************************************/

ROM_START( dharma )
	ROM_REGION( 0x040000, REGION_CPU1, 0 )		/* 68000 Code */
	ROM_LOAD16_BYTE( "jc-5", 0x000000, 0x020000, CRC(b5d44426) SHA1(d68aaf6b9976ccf5cb665d7ec0afa44e2453094d) )
	ROM_LOAD16_BYTE( "jc-6", 0x000001, 0x020000, CRC(bc5a202e) SHA1(c2b6d2e44e3605e0525bde4030c5162badad4d4b) )

	ROM_REGION( 0x02c000, REGION_CPU2, 0 )		/* NEC78C10 Code */
	ROM_LOAD( "ja-8", 0x000000, 0x004000, CRC(af7ebc4c) SHA1(6abf0036346da10be56932f9674f8c250a3ea592) )
	ROM_CONTINUE(     0x010000, 0x01c000 )

	ROM_REGION( 0x200000, REGION_GFX1, 0 )	/* Gfx + Data (Addressable by CPU & Blitter) */
	ROMX_LOAD( "jb-2", 0x000000, 0x080000, CRC(2c07c29b) SHA1(26244145139df1ffe2b6ec25a32e5009da6a5aba) , ROM_GROUPWORD | ROM_SKIP(6))
	ROMX_LOAD( "jb-4", 0x000002, 0x080000, CRC(fe15538e) SHA1(a52ac04656783611ec5d5af01b18e22254decc0c) , ROM_GROUPWORD | ROM_SKIP(6))
	ROMX_LOAD( "jb-1", 0x000004, 0x080000, CRC(e6ca9bf6) SHA1(0379250303eb6895a4dda080da8bf031d055ce8e) , ROM_GROUPWORD | ROM_SKIP(6))
	ROMX_LOAD( "jb-3", 0x000006, 0x080000, CRC(6ecbe193) SHA1(33b799699d5d17705df36591cdc40032278388d1) , ROM_GROUPWORD | ROM_SKIP(6))

	ROM_REGION( 0x040000, REGION_SOUND1, ROMREGION_SOUNDONLY )	/* Samples */
	ROM_LOAD( "ja-7", 0x000000, 0x040000, CRC(7ce817eb) SHA1(9dfb79021a552877fbc26049cca853c0b93735b5) )
ROM_END


/***************************************************************************

Karate Tournament

68000-12
NEC D78C10ACN
OKI6295
YM2413
OSC:  24.000MHz,  20.000MHz,   XTAL 3579545

On board, location for but unused things...
Unused DIP#3
Unused BAT1

I can see a large square surface-mounted chip with
these markings...

ImageTek Inc.
14100
052
9227KK702

Filename	Type		Location
KT001.BIN	27C010	 	1I
KT002.BIN	27C2001		8G
KT003.BIN	27C2001		10G
KT008.BIN	27C2001		1D

Filename	Chip Markings	Location
KTMASK1.BIN	361A04 9241D	15F
KTMASK2.BIN	361A05 9239D	17F
KTMASK3.BIN	361A06 9239D	15D
KTMASK4.BIN	361A07 9239D	17D

***************************************************************************/

ROM_START( karatour )
	ROM_REGION( 0x080000, REGION_CPU1, 0 )		/* 68000 Code */
	ROM_LOAD16_BYTE( "kt002.8g",  0x000000, 0x040000, CRC(316a97ec) SHA1(4b099d2fa91822c9c85d647aab3d6779fc400250) )
	ROM_LOAD16_BYTE( "kt003.10g", 0x000001, 0x040000, CRC(abe1b991) SHA1(9b6327169d66717dd9dd74816bc33eb208c3763c) )

	ROM_REGION( 0x02c000, REGION_CPU2, 0 )		/* NEC78C10 Code */
	ROM_LOAD( "kt001.1i", 0x000000, 0x004000, CRC(1dd2008c) SHA1(488b6f5d15bdbc069ee2cd6d7a0980a228d2f790) )
	ROM_CONTINUE(         0x010000, 0x01c000 )

	ROM_REGION( 0x400000, REGION_GFX1, 0 )	/* Gfx + Data (Addressable by CPU & Blitter) */
	ROMX_LOAD( "ktmask.15f", 0x000000, 0x100000, CRC(f6bf20a5) SHA1(cb4cb249eb1c106fe7ef0ace735c0cc3106f1ab7) , ROM_GROUPWORD | ROM_SKIP(6))
	ROMX_LOAD( "ktmask.17d", 0x000002, 0x100000, CRC(794cc1c0) SHA1(ecfdec5874a95846c0fb7966fdd1da625d85531f) , ROM_GROUPWORD | ROM_SKIP(6))
	ROMX_LOAD( "ktmask.17f", 0x000004, 0x100000, CRC(ea9c11fc) SHA1(176c4419cfe13ff019654a93cd7b0befa238bbc3) , ROM_GROUPWORD | ROM_SKIP(6))
	ROMX_LOAD( "ktmask.15d", 0x000006, 0x100000, CRC(7e15f058) SHA1(267f0a5acb874d4fff3556ffa405e24724174667) , ROM_GROUPWORD | ROM_SKIP(6))

	ROM_REGION( 0x040000, REGION_SOUND1, ROMREGION_SOUNDONLY )	/* Samples */
	ROM_LOAD( "kt008.1d", 0x000000, 0x040000, CRC(47cf9fa1) SHA1(88923ace550154c58c066f859cadfa7864c5344c) )

	/* Additional memory for the layers' ram */
	ROM_REGION( 0x20000*3, REGION_USER1, 0 )
ROM_END


/***************************************************************************

Moeyo Gonta!! (Lady Killer)
(c)1993 Yanyaka
VG460-(B)

CPU  : TMP68HC000P-16
Sound: D78C10ACW YM2413 M6295
OSC  : 3.579545MHz(XTAL1) 20.0000MHz(XTAL2) 24.0000MHz(XTAL3)

ROMs:
e1.1i - Sound program (27c010)

j2.8g  - Main programs (27c020)
j3.10g /

ladyj-4.15f - Graphics (mask, read as 27c800)
ladyj-5.17f |
ladyj-6.15d |
ladyj-7.17d /

e8j.1d - Samples (27c020)

Others:
Imagetek I4100 052 9330EK712

***************************************************************************/

ROM_START( ladykill )
	ROM_REGION( 0x080000, REGION_CPU1, 0 )		/* 68000 Code */
	ROM_LOAD16_BYTE( "e2.bin",    0x000000, 0x040000, CRC(211a4865) SHA1(4315c0a708383d357d8dd89a1820fe6cf7652adb) )
	ROM_LOAD16_BYTE( "e3.bin",    0x000001, 0x040000, CRC(581a55ea) SHA1(41bfcaae84e583bf185948ab53ec39c05180a7a4) )

	ROM_REGION( 0x02c000, REGION_CPU2, 0 )		/* NEC78C10 Code */
	ROM_LOAD( "e1.1i",    0x000000, 0x004000, CRC(a4d95cfb) SHA1(2fd8a5cbb0dc289bd5294519dbd5369bfb4c2d4d) )
	ROM_CONTINUE(         0x010000, 0x01c000 )

	ROM_REGION( 0x400000, REGION_GFX1, 0 )	/* Gfx + Data (Addressable by CPU & Blitter) */
	ROMX_LOAD( "ladyj-4.15f", 0x000000, 0x100000, CRC(65e5906c) SHA1(cc3918c2094ca819ec4043055564e1dbff4a4750) , ROM_GROUPWORD | ROM_SKIP(6))
	ROMX_LOAD( "ladyj-7.17d", 0x000002, 0x100000, CRC(56bd64a5) SHA1(911272078b0fd375111f5d1463945c2075c19e40) , ROM_GROUPWORD | ROM_SKIP(6))
	ROMX_LOAD( "ladyj-5.17f", 0x000004, 0x100000, CRC(a81ffaa3) SHA1(5c161b0ef33f1bab077e9a2eb2d3432825729e83) , ROM_GROUPWORD | ROM_SKIP(6))
	ROMX_LOAD( "ladyj-6.15d", 0x000006, 0x100000, CRC(3a34913a) SHA1(a55624ede7c368e61555ca7b9cd9e6948265b784) , ROM_GROUPWORD | ROM_SKIP(6))

	ROM_REGION( 0x040000, REGION_SOUND1, ROMREGION_SOUNDONLY )	/* Samples */
	ROM_LOAD( "e8.bin",   0x000000, 0x040000, CRC(da88244d) SHA1(90c0cc275b69afffd9a0126985fd3fe16d44dced) )

	/* Additional memory for the layers' ram */
	ROM_REGION( 0x20000*3, REGION_USER1, 0 )
ROM_END

ROM_START( moegonta )
	ROM_REGION( 0x080000, REGION_CPU1, 0 )		/* 68000 Code */
	ROM_LOAD16_BYTE( "j2.8g",     0x000000, 0x040000, CRC(aa18d130) SHA1(6e0fd3b95d8589665b418bcae4fe64b288289c78) )
	ROM_LOAD16_BYTE( "j3.10g",    0x000001, 0x040000, CRC(b555e6ab) SHA1(adfc6eafec612c8770b9f832a0a2574c53c3d047) )

	ROM_REGION( 0x02c000, REGION_CPU2, 0 )		/* NEC78C10 Code */
	ROM_LOAD( "e1.1i",    0x000000, 0x004000, CRC(a4d95cfb) SHA1(2fd8a5cbb0dc289bd5294519dbd5369bfb4c2d4d) )
	ROM_CONTINUE(         0x010000, 0x01c000 )

	ROM_REGION( 0x400000, REGION_GFX1, 0 )	/* Gfx + Data (Addressable by CPU & Blitter) */
	ROMX_LOAD( "ladyj-4.15f", 0x000000, 0x100000, CRC(65e5906c) SHA1(cc3918c2094ca819ec4043055564e1dbff4a4750) , ROM_GROUPWORD | ROM_SKIP(6))
	ROMX_LOAD( "ladyj-7.17d", 0x000002, 0x100000, CRC(56bd64a5) SHA1(911272078b0fd375111f5d1463945c2075c19e40) , ROM_GROUPWORD | ROM_SKIP(6))
	ROMX_LOAD( "ladyj-5.17f", 0x000004, 0x100000, CRC(a81ffaa3) SHA1(5c161b0ef33f1bab077e9a2eb2d3432825729e83) , ROM_GROUPWORD | ROM_SKIP(6))
	ROMX_LOAD( "ladyj-6.15d", 0x000006, 0x100000, CRC(3a34913a) SHA1(a55624ede7c368e61555ca7b9cd9e6948265b784) , ROM_GROUPWORD | ROM_SKIP(6))

	ROM_REGION( 0x040000, REGION_SOUND1, ROMREGION_SOUNDONLY )	/* Samples */
	ROM_LOAD( "e8j.1d",   0x000000, 0x040000, CRC(f66c2a80) SHA1(d95ddc8fe4144a6ad4a92385ff962d0b9391d53b) )

	/* Additional memory for the layers' ram */
	ROM_REGION( 0x20000*3, REGION_USER1, 0 )
ROM_END


/***************************************************************************

Last Fortress - Toride
Metro 1992
VG420

                                     TR_JB12 5216
                     SW2 SW1           NEC78C10   3.579MHz

                                                          6269
                                                          TR_JB11
  55328 55328 55328       24MHz

                           4064   4064   TR_   TR_          68000-12
       Imagetek                          JC10  JC09
       14100

    TR_  TR_  TR_  TR_  TR_  TR_  TR_  TR_
    JC08 JC07 JC06 JC05 JC04 JC03 JC02 JC01

***************************************************************************/

ROM_START( lastfort )
	ROM_REGION( 0x040000, REGION_CPU1, 0 )		/* 68000 Code */
	ROM_LOAD16_BYTE( "tr_jc09", 0x000000, 0x020000, CRC(8b98a49a) SHA1(15adca78d54973820d04f8b308dc58d0784eb900) )
	ROM_LOAD16_BYTE( "tr_jc10", 0x000001, 0x020000, CRC(8d04da04) SHA1(5c7e65a39929e94d1fa99aeb5fed7030b110451f) )

	ROM_REGION( 0x02c000, REGION_CPU2, 0 )		/* NEC78C10 Code */
	ROM_LOAD( "tr_jb12", 0x000000, 0x004000, CRC(8a8f5fef) SHA1(530b4966ec058cd80a2fc5f9e961239ce59d0b89) )	
	ROM_CONTINUE(        0x010000, 0x01c000 )

	ROM_REGION( 0x100000, REGION_GFX1, 0 )	/* Gfx + Data (Addressable by CPU & Blitter) */
	ROMX_LOAD( "tr_jc02", 0x000000, 0x020000, CRC(db3c5b79) SHA1(337f4c547a6267f317415cbc78cdac41574b1024) , ROM_SKIP(7))
	ROMX_LOAD( "tr_jc04", 0x000001, 0x020000, CRC(f8ab2f9b) SHA1(bfbbd5ec2bc039b8eaef92467c2e7fd3b425b477) , ROM_SKIP(7))
	ROMX_LOAD( "tr_jc06", 0x000002, 0x020000, CRC(47a7f397) SHA1(1d2b11b95ce81ca66713457283464d6d85753e4b) , ROM_SKIP(7))
	ROMX_LOAD( "tr_jc08", 0x000003, 0x020000, CRC(d7ba5e26) SHA1(294fd9b68eebd28ca64627f0d6e64b325cab18a0) , ROM_SKIP(7))
	ROMX_LOAD( "tr_jc01", 0x000004, 0x020000, CRC(3e3dab03) SHA1(e3c6eb73467f0ed207657084e51ee87d85152c3f) , ROM_SKIP(7))
	ROMX_LOAD( "tr_jc03", 0x000005, 0x020000, CRC(87ac046f) SHA1(6555a55642383990bc7a8282ab5ea8fc0ba6cd14) , ROM_SKIP(7))
	ROMX_LOAD( "tr_jc05", 0x000006, 0x020000, CRC(3fbbe49c) SHA1(642631e69d78898403013884cf0fb711ea000541) , ROM_SKIP(7))
	ROMX_LOAD( "tr_jc07", 0x000007, 0x020000, CRC(05e1456b) SHA1(51cd3ad2aa9c0adc7b9d63a337b247b4b65701ca) , ROM_SKIP(7))

	ROM_REGION( 0x020000, REGION_SOUND1, ROMREGION_SOUNDONLY )	/* Samples */
	ROM_LOAD( "tr_jb11", 0x000000, 0x020000, CRC(83786a09) SHA1(910cf0ccf4493f2a80062149f6364dbb6a1c2a5d) )
ROM_END


/***************************************************************************

Last Fortress - Toride (Erotic)
Metro Corporation.

Board number VG420

CPU: MC68000P12
SND: OKI M6295+ YM2413 + NEC D78C10ACW + NEC D4016 (ram?)
DSW: see manual (scanned in sub-directory Manual)
OSC: 24.000 MHz

***************************************************************************/

ROM_START( lastfero )
	ROM_REGION( 0x040000, REGION_CPU1, 0 )		/* 68000 Code */
	ROM_LOAD16_BYTE( "tre_jc09", 0x000000, 0x020000, CRC(32f43390) SHA1(b5bad9d80f2155f277265fe487a59f0f4ec6575d) )
	ROM_LOAD16_BYTE( "tre_jc10", 0x000001, 0x020000, CRC(9536369c) SHA1(39291e92c107be35d130ff29533b42581efc308b) )

	ROM_REGION( 0x02c000, REGION_CPU2, 0 )		/* NEC78C10 Code */
	ROM_LOAD( "tr_jb12", 0x000000, 0x004000, CRC(8a8f5fef) SHA1(530b4966ec058cd80a2fc5f9e961239ce59d0b89) )	
	ROM_CONTINUE(        0x010000, 0x01c000 )

	ROM_REGION( 0x100000, REGION_GFX1, 0 )	/* Gfx + Data (Addressable by CPU & Blitter) */
	ROMX_LOAD( "tre_jc02", 0x000000, 0x020000, CRC(11cfbc84) SHA1(fb7005be7678564713b5480569f2cdab6c36f029) , ROM_SKIP(7))
	ROMX_LOAD( "tre_jc04", 0x000001, 0x020000, CRC(32bf9c26) SHA1(9d16eca8810d1823726dc9c047504bd24f2a55f7) , ROM_SKIP(7))
	ROMX_LOAD( "tre_jc06", 0x000002, 0x020000, CRC(16937977) SHA1(768bb6b1c9b90b2eedc9dbb19c8e9fa8f4265f17) , ROM_SKIP(7))
	ROMX_LOAD( "tre_jc08", 0x000003, 0x020000, CRC(6dd96a9b) SHA1(fe8214d57dc83157eff53f2d83bd3a4e2da91555) , ROM_SKIP(7))
	ROMX_LOAD( "tre_jc01", 0x000004, 0x020000, CRC(aceb44b3) SHA1(9a236eddbc916c206bfa694b576d971d788e8eb1) , ROM_SKIP(7))
	ROMX_LOAD( "tre_jc03", 0x000005, 0x020000, CRC(f18f1248) SHA1(30e39d904368c61a46719a0f21a6acb7fa55593f) , ROM_SKIP(7))
	ROMX_LOAD( "tre_jc05", 0x000006, 0x020000, CRC(79f769dd) SHA1(7a9ff8e961ae09fdf36a0a751befc141f47c9fd8) , ROM_SKIP(7))
	ROMX_LOAD( "tre_jc07", 0x000007, 0x020000, CRC(b6feacb2) SHA1(85df28d5ff6601753a435e31bcaf45702c7489ea) , ROM_SKIP(7))

	ROM_REGION( 0x020000, REGION_SOUND1, ROMREGION_SOUNDONLY )	/* Samples */
	ROM_LOAD( "tr_jb11", 0x000000, 0x020000, CRC(83786a09) SHA1(910cf0ccf4493f2a80062149f6364dbb6a1c2a5d) )
ROM_END


/***************************************************************************

Mahjong Doukyuhsei (JPN Ver.)

(c)1995 make software/elf/media trading corp.

CPU   :68000 16MHz
Sound :YM2413 custom
OSC   :16.0000MHz 3.579545MHz 26.666MHz

Board Name?:VG330-B

***************************************************************************/

ROM_START( dokyusei )
	ROM_REGION( 0x040000, REGION_CPU1, 0 )		/* 68000 Code */
	ROM_LOAD16_BYTE( "330_a06.bin", 0x000000, 0x020000, CRC(36157c2e) SHA1(f855175143caf476dcbee5a8aaec802a8fdb64fa) )
	ROM_LOAD16_BYTE( "330_a05.bin", 0x000001, 0x020000, CRC(177f50d2) SHA1(2298411152553041b907d9243aaa7983ca21c946) )

	ROM_REGION( 0x800000, REGION_GFX1, 0 )	/* Gfx + Data (Addressable by CPU & Blitter) */
	ROMX_LOAD( "2.bin", 0x000000, 0x200000, CRC(075bface) SHA1(7f0e47ebdc37a1fc09b072cb8e0f38258a702a3d) , ROM_GROUPWORD | ROM_SKIP(6))
	ROMX_LOAD( "4.bin", 0x000002, 0x200000, CRC(bc631438) SHA1(da3ef24d94e69197e3c69e4fd2b716162c275278) , ROM_GROUPWORD | ROM_SKIP(6))
	ROMX_LOAD( "1.bin", 0x000004, 0x200000, CRC(4566c29b) SHA1(3216e21d898855cbb0ad328e6d45f3726d95b099) , ROM_GROUPWORD | ROM_SKIP(6))
	ROMX_LOAD( "3.bin", 0x000006, 0x200000, CRC(5f6d7969) SHA1(bcb48c5808f268ca35a28f162d4e9da9df65b843) , ROM_GROUPWORD | ROM_SKIP(6))

	ROM_REGION( 0x100000, REGION_SOUND1, ROMREGION_SOUNDONLY )	/* Samples */
	ROM_LOAD( "7.bin", 0x000000, 0x100000, CRC(c572aee1) SHA1(2a3baf962617577f8ac3f9e58fb4e5a0dae4f0e8) )	/* 4 x 0x40000*/
ROM_END


/***************************************************************************

Mahjong Doukyuusei Special
(c)1995 Make Software / Elf / Media Trading

Board:	VG340-A

CPU:	68000-16
Sound:	M6295
		YM2413
OSC:	32.0000MHz
		3.579545MHz
EEPROM:	93C46
Custom:	14300 095

***************************************************************************/

ROM_START( dokyusp )
	ROM_REGION( 0x040000, REGION_CPU1, 0 )		/* 68000 Code */
	ROM_LOAD16_BYTE( "6.bin", 0x000000, 0x020000, CRC(01729b7f) SHA1(42a60f034ee5d5c2a42856b97d0d4c499b24627b) )
	ROM_LOAD16_BYTE( "5.bin", 0x000001, 0x020000, CRC(57770776) SHA1(15093886f2fe49443e8d7541903714de0a14aa0b) )

	ROM_REGION( 0x1000000, REGION_GFX1, 0 )	/* Gfx + Data (Addressable by CPU & Blitter) */
	ROMX_LOAD( "2l.bin", 0x0000000, 0x400000, CRC(4bed184d) SHA1(12bdb00030d19c2c9fb2120ed6b267a7982c213a) , ROM_GROUPWORD | ROM_SKIP(6))
	ROMX_LOAD( "4l.bin", 0x0000002, 0x400000, CRC(2ee468e6) SHA1(ced58fdd8b5c99ce3f09cece2e05d7fcf4c7f786) , ROM_GROUPWORD | ROM_SKIP(6))
	ROMX_LOAD( "1l.bin", 0x0000004, 0x400000, CRC(510ace14) SHA1(f5f1f46f4d8d150dd9e17083f32e9b45938c1dad) , ROM_GROUPWORD | ROM_SKIP(6))
	ROMX_LOAD( "3l.bin", 0x0000006, 0x400000, CRC(82ea562e) SHA1(42839de9f346ccd0736bdbd3eead61ad66fcb666) , ROM_GROUPWORD | ROM_SKIP(6))

	ROM_REGION( 0x200000, REGION_SOUND1, ROMREGION_SOUNDONLY )	/* Samples */
	ROM_LOAD( "7.bin", 0x000000, 0x200000, CRC(763985e1) SHA1(395d925b79922de5060a3f59de99fbcc9bd40fad) )
ROM_END


/***************************************************************************

Mahjong Gakuensai (JPN Ver.)
(c)1997 Make Software

Board:	VG340-A

CPU:	68000-16
Sound:	M6295
		YM2413
OSC:	26.6660MHz
		3.5795MHz

Custom:	14300 095

***************************************************************************/

ROM_START( gakusai )
	ROM_REGION( 0x080000, REGION_CPU1, 0 )		/* 68000 Code */
	ROM_LOAD16_BYTE( "6.bin", 0x000000, 0x040000, CRC(6f8ab082) SHA1(18caf49a0c65f831d375f089f27b8570b094f029) )
	ROM_LOAD16_BYTE( "5.bin", 0x000001, 0x040000, CRC(010176c4) SHA1(48fcea18c02c1426a699a636f44b21cf7625e8a0) )

	ROM_REGION( 0x2000000, REGION_GFX1, 0 )	/* Gfx + Data (Addressable by CPU & Blitter) */
	ROMX_LOAD( "2l.bin", 0x0000000, 0x400000, CRC(45dfb5c7) SHA1(04338d695bd6973fd7d7286a8da563250ae4f71b) , ROM_GROUPWORD | ROM_SKIP(6))
	ROMX_LOAD( "4l.bin", 0x0000002, 0x400000, CRC(7ab64f49) SHA1(e4d9a7bf97635b41fe632b3542eee1f609db080a) , ROM_GROUPWORD | ROM_SKIP(6))
	ROMX_LOAD( "1l.bin", 0x0000004, 0x400000, CRC(75093421) SHA1(cfe549e24abfedd740ead30cab235df494e9f45d) , ROM_GROUPWORD | ROM_SKIP(6))
	ROMX_LOAD( "3l.bin", 0x0000006, 0x400000, CRC(4dcfcd98) SHA1(bfb882d99c854e68e86f4e8f8aa7d02dcf5e9cfc) , ROM_GROUPWORD | ROM_SKIP(6))
	ROMX_LOAD( "2u.bin", 0x1000000, 0x400000, CRC(8d4f912b) SHA1(1fcf1dd50fd678cc908ab47bcccaa4ed7b2b6938) , ROM_GROUPWORD | ROM_SKIP(6))
	ROMX_LOAD( "4u.bin", 0x1000002, 0x400000, CRC(1f83e98a) SHA1(10b2d3ceb4bda6a2ecf795b865c948563c2fb84d) , ROM_GROUPWORD | ROM_SKIP(6))
	ROMX_LOAD( "1u.bin", 0x1000004, 0x400000, CRC(28b386d9) SHA1(d1e151fa112c86d2cb97b7a5439a1e549359055d) , ROM_GROUPWORD | ROM_SKIP(6))
	ROMX_LOAD( "3u.bin", 0x1000006, 0x400000, CRC(87f3c5e6) SHA1(097c0a53b040399d928f17fe3e9f42755b1d72f3) , ROM_GROUPWORD | ROM_SKIP(6))

	ROM_REGION( 0x400000, REGION_SOUND1, ROMREGION_SOUNDONLY )	/* Samples */
	ROM_LOAD( "7.bin", 0x000000, 0x400000, CRC(34575a14) SHA1(53d458513f208f07844e1727d5889e85dcd4f0ed) )
ROM_END


/***************************************************************************

Mahjong Gakuensai 2 (JPN Ver.)
(c)1998 Make Software

Board:	VG340-A

CPU:	68000-16
Sound:	M6295
		YM2413
OSC:	26.6660MHz
		3.579545MHz

Custom:	14300 095

***************************************************************************/

ROM_START( gakusai2 )
	ROM_REGION( 0x040000, REGION_CPU1, 0 )		/* 68000 Code */
	ROM_LOAD16_BYTE( "mg2a06.bin", 0x000000, 0x020000, CRC(8b006dd4) SHA1(893ec0e7c367d79bc99e65ab8abd0d290f2ede58) )
	ROM_LOAD16_BYTE( "mg2a05.bin", 0x000001, 0x020000, CRC(7702b9ac) SHA1(09d0c11fa2c9ed9cde365cb1ff215d55e39b7734) )

	ROM_REGION( 0x2000000, REGION_GFX1, 0 )	/* Gfx + Data (Addressable by CPU & Blitter) */
	ROMX_LOAD( "mg22l.bin", 0x0000000, 0x400000, CRC(28366708) SHA1(56fccee126916cc301678a205dfe629efefb79db) , ROM_GROUPWORD | ROM_SKIP(6))
	ROMX_LOAD( "mg24l.bin", 0x0000002, 0x400000, CRC(9e003bb0) SHA1(aa73cc0e79732fd6826c89671b179cb3189571e0) , ROM_GROUPWORD | ROM_SKIP(6))
	ROMX_LOAD( "mg21l.bin", 0x0000004, 0x400000, CRC(3827098d) SHA1(dda9fb6c56c4408802d54c5975fb9470ca2e1d34) , ROM_GROUPWORD | ROM_SKIP(6))
	ROMX_LOAD( "mg23l.bin", 0x0000006, 0x400000, CRC(a6f96961) SHA1(dd2578da5d091991580a2c7a979ba8dbfa0cceb3) , ROM_GROUPWORD | ROM_SKIP(6))
	ROMX_LOAD( "mg22u.bin", 0x1000000, 0x400000, CRC(53ffa68a) SHA1(3d8d69c2063c78bd79cdbd7457bca1af9700bf3c) , ROM_GROUPWORD | ROM_SKIP(6))
	ROMX_LOAD( "mg24u.bin", 0x1000002, 0x400000, CRC(c218e9ab) SHA1(3b6ee4cc828198b284ac9020e2da911efc90725a) , ROM_GROUPWORD | ROM_SKIP(6))
	ROMX_LOAD( "mg21u.bin", 0x1000004, 0x400000, CRC(385495e5) SHA1(5181e279fef23780d07ab5a124618e4d0e5cb821) , ROM_GROUPWORD | ROM_SKIP(6))
	ROMX_LOAD( "mg23u.bin", 0x1000006, 0x400000, CRC(d8315923) SHA1(6bb5cad317f7efa6a384f6c257c5faeb789a8eed) , ROM_GROUPWORD | ROM_SKIP(6))

	ROM_REGION( 0x400000, REGION_SOUND1, ROMREGION_SOUNDONLY )	/* Samples */
	ROM_LOAD( "mg2-7.bin", 0x000000, 0x400000, CRC(2f1c041e) SHA1(a72720b3d7f816e23452775f2fd4223cf2d02985) )
ROM_END


/***************************************************************************

Mouja (JPN Ver.)
(c)1996 Etona / (c)1995 FPS/FWS
VG410-B

CPU 	:TMP68H000P-12
Sound	:YM2413,OKI M6295
OSC 	:16000.00KHz,3.579545MHz,26.666MHz
other	:Imagetek Inc 14300 095

***************************************************************************/

ROM_START( mouja )
	ROM_REGION( 0x080000, REGION_CPU1, 0 )		/* 68000 Code */
	ROM_LOAD16_BYTE( "10.bin",      0x000000, 0x040000, CRC(f9742b69) SHA1(f8c6508b227403a82413ceeb0651922759d7e0f4) )
	ROM_LOAD16_BYTE( "9.bin",       0x000001, 0x040000, CRC(dc77216f) SHA1(3b73d29f4e8e385f45f2abfb38eaffc2d8406948) )

	ROM_REGION( 0x400000, REGION_GFX1, 0 )	/* Gfx + Data (Addressable by CPU & Blitter) */
	ROMX_LOAD( "42.bin",      0x000000, 0x100000, CRC(c4dd3194) SHA1(c9c88a8d2046224957b35de14763aa4bdf0d407f) , ROM_GROUPWORD | ROM_SKIP(6))
	ROMX_LOAD( "86.bin",      0x000002, 0x100000, CRC(09530f9d) SHA1(03f2ec5ea694266808d245abe7f688de0ef6d853) , ROM_GROUPWORD | ROM_SKIP(6))
	ROMX_LOAD( "31.bin",      0x000004, 0x100000, CRC(5dd7a7b2) SHA1(b0347e8951b29356a7d945b906d93c40b9abc19c) , ROM_GROUPWORD | ROM_SKIP(6))
	ROMX_LOAD( "75.bin",      0x000006, 0x100000, CRC(430c3925) SHA1(41e5bd02a665eee87ef8f4ae9f4bee374c25e00b) , ROM_GROUPWORD | ROM_SKIP(6))

	ROM_REGION( 0x100000, REGION_SOUND1, ROMREGION_SOUNDONLY )	/* Samples */
	ROM_LOAD( "11.bin",     0x000000, 0x100000, CRC(fe3df432) SHA1(4fb7ad997ca6e91468d7516e5c4a94cde6e07104) )
ROM_END


/***************************************************************************

Pang Poms (c) 1992 Metro

Pcb code:  VG420 (Same as Toride)

Cpus:  M68000, Z80
Clocks: 24 MHz, 3.579 MHz
Sound: M6295, YM2413, _unused_ slot for a YM2151

Custom graphics chip - Imagetek 14100 052 9227KK701 (same as Karate Tournament)

***************************************************************************/

ROM_START( pangpoms )
	ROM_REGION( 0x040000, REGION_CPU1, 0 )		/* 68000 Code */
	ROM_LOAD16_BYTE( "ppoms09.bin", 0x000000, 0x020000, CRC(0c292dbc) SHA1(8b09de2a560e804e0dea514c95b317c2e2b6501d) )
	ROM_LOAD16_BYTE( "ppoms10.bin", 0x000001, 0x020000, CRC(0bc18853) SHA1(68d50ad50caad34e72d32e7b9fea1d85af74b879) )

	ROM_REGION( 0x02c000, REGION_CPU2, 0 )		/* NEC78C10 Code */
	ROM_LOAD( "ppoms12.bin", 0x000000, 0x004000, CRC(a749357b) SHA1(1555f565c301c5be7c49fc44a004b5c0cb3777c6) )
	ROM_CONTINUE(            0x010000, 0x01c000 )

	ROM_REGION( 0x100000, REGION_GFX1, 0 )	/* Gfx + Data (Addressable by CPU & Blitter) */
	ROMX_LOAD( "ppoms02.bin", 0x000000, 0x020000, CRC(88f902f7) SHA1(12ea58d7c000b629ccdceec3dedc2747a63b84be) , ROM_SKIP(7))
	ROMX_LOAD( "ppoms04.bin", 0x000001, 0x020000, CRC(9190c2a0) SHA1(a7399cc2dea5a963e7c930e426915e8eb3552213) , ROM_SKIP(7))
	ROMX_LOAD( "ppoms06.bin", 0x000002, 0x020000, CRC(ed15c93d) SHA1(95072e7d1def0d8e97946a612b90ce078c64aed2) , ROM_SKIP(7))
	ROMX_LOAD( "ppoms08.bin", 0x000003, 0x020000, CRC(9a3408b9) SHA1(924b184d3a47bbe8aa5d41761ea5e94ba7e4f2e9) , ROM_SKIP(7))
	ROMX_LOAD( "ppoms01.bin", 0x000004, 0x020000, CRC(11ac3810) SHA1(6ada82a73d4383f99f5be67369b810a692d27ef9) , ROM_SKIP(7))
	ROMX_LOAD( "ppoms03.bin", 0x000005, 0x020000, CRC(e595529e) SHA1(91b4bd1f029ce09d7689815099b38916fe0d2686) , ROM_SKIP(7))
	ROMX_LOAD( "ppoms05.bin", 0x000006, 0x020000, CRC(02226214) SHA1(82302e7f1e7269c45e11dfba45ec7bbf522b47f1) , ROM_SKIP(7))
	ROMX_LOAD( "ppoms07.bin", 0x000007, 0x020000, CRC(48471c87) SHA1(025fa79993788a0091c4edb83423725abd3a47a2) , ROM_SKIP(7))

	ROM_REGION( 0x020000, REGION_SOUND1, ROMREGION_SOUNDONLY )	/* Samples */
	ROM_LOAD( "ppoms11.bin", 0x000000, 0x020000, CRC(e89bd565) SHA1(6c7c1ad67ba708dbbe9654c1d290af290207d2be) )
ROM_END

ROM_START( pangpomm )
	ROM_REGION( 0x040000, REGION_CPU1, 0 )		/* 68000 Code */
	ROM_LOAD16_BYTE( "pa.c09", 0x000000, 0x020000, CRC(e01a7a08) SHA1(1890b290dfb1521ab73b2392409aaf44b99d63bb) )
	ROM_LOAD16_BYTE( "pa.c10", 0x000001, 0x020000, CRC(5e509cee) SHA1(821cfbf5f65cc3091eb8008310266f9f2c838072) )

	ROM_REGION( 0x02c000, REGION_CPU2, 0 )		/* NEC78C10 Code */
	ROM_LOAD( "ppoms12.bin", 0x000000, 0x004000, CRC(a749357b) SHA1(1555f565c301c5be7c49fc44a004b5c0cb3777c6) )
	ROM_CONTINUE(            0x010000, 0x01c000 )

	ROM_REGION( 0x100000, REGION_GFX1, 0 )	/* Gfx + Data (Addressable by CPU & Blitter) */
	ROMX_LOAD( "ppoms02.bin", 0x000000, 0x020000, CRC(88f902f7) SHA1(12ea58d7c000b629ccdceec3dedc2747a63b84be) , ROM_SKIP(7))
	ROMX_LOAD( "pj.e04",      0x000001, 0x020000, CRC(54bf2f10) SHA1(2f0f18984e336f226457295d375a73bcf86cef31) , ROM_SKIP(7))
	ROMX_LOAD( "pj.e06",      0x000002, 0x020000, CRC(c8b6347d) SHA1(7090e44dc7032432795b6fb6bc166bf4de159685) , ROM_SKIP(7))
	ROMX_LOAD( "ppoms08.bin", 0x000003, 0x020000, CRC(9a3408b9) SHA1(924b184d3a47bbe8aa5d41761ea5e94ba7e4f2e9) , ROM_SKIP(7))
	ROMX_LOAD( "ppoms01.bin", 0x000004, 0x020000, CRC(11ac3810) SHA1(6ada82a73d4383f99f5be67369b810a692d27ef9) , ROM_SKIP(7))
	ROMX_LOAD( "pj.e03",      0x000005, 0x020000, CRC(d126e774) SHA1(f782d1e1277956f088dc91dec8f338f85b9af13a) , ROM_SKIP(7))
	ROMX_LOAD( "pj.e05",      0x000006, 0x020000, CRC(79c0ec1e) SHA1(b15582e89d859dda4f82908c62e9e07cb45229b9) , ROM_SKIP(7))
	ROMX_LOAD( "ppoms07.bin", 0x000007, 0x020000, CRC(48471c87) SHA1(025fa79993788a0091c4edb83423725abd3a47a2) , ROM_SKIP(7))

	ROM_REGION( 0x020000, REGION_SOUND1, ROMREGION_SOUNDONLY )	/* Samples */
	ROM_LOAD( "ppoms11.bin", 0x000000, 0x020000, CRC(e89bd565) SHA1(6c7c1ad67ba708dbbe9654c1d290af290207d2be) )
ROM_END


/***************************************************************************

Poitto! (c)1993 Metro corp
MTR5260-A

CPU  : TMP68HC000P-16
Sound: D78C10ACW M6295 YM2413
OSC  : 24.0000MHz  (OSC1)
                   (OSC2)
                   (OSC3)
       3.579545MHz (OSC4)
                   (OSC5)

ROMs:
pt-1.13i - Graphics (23c4000)
pt-2.15i |
pt-3.17i |
pt-4.19i /

pt-jd05.20e - Main programs (27c010)
pt-jd06.20c /

pt-jc07.3g - Sound data (27c020)
pt-jc08.3i - Sound program (27c010)

Others:
Imagetek 14100 052 9309EK701 (208pin PQFP)
AMD MACH110-20 (CPLD)

***************************************************************************/

ROM_START( poitto )
	ROM_REGION( 0x040000, REGION_CPU1, 0 )		/* 68000 Code */
	ROM_LOAD16_BYTE( "pt-jd05.20e", 0x000000, 0x020000, CRC(6b1be034) SHA1(270c94f6017c5ce77f562bfe17273c79d4455053) )
	ROM_LOAD16_BYTE( "pt-jd06.20c", 0x000001, 0x020000, CRC(3092d9d4) SHA1(4ff95355fdf94eaa55c0ad46e6ce3b505e3ef790) )

	ROM_REGION( 0x02c000, REGION_CPU2, 0 )		/* NEC78C10 Code */
	ROM_LOAD( "pt-jc08.3i", 0x000000, 0x004000, CRC(f32d386a) SHA1(655c561aec1112d88c1b94725e932059e5d1d5a8) )
	ROM_CONTINUE(           0x010000, 0x01c000 )

	ROM_REGION( 0x200000, REGION_GFX1, 0 )	/* Gfx + Data (Addressable by CPU & Blitter) */
	ROMX_LOAD( "pt-2.15i", 0x000000, 0x080000, CRC(05d15d01) SHA1(24405908fb8207228cd3419657e0be49e413f152) , ROM_GROUPWORD | ROM_SKIP(6))
	ROMX_LOAD( "pt-4.19i", 0x000002, 0x080000, CRC(8a39edb5) SHA1(1d860e0a1b975a93907d5bb0704e3bad383bbda7) , ROM_GROUPWORD | ROM_SKIP(6))
	ROMX_LOAD( "pt-1.13i", 0x000004, 0x080000, CRC(ea6e2289) SHA1(2c939b32d2bf155bb5c8bd979dadcf4f75e178b0) , ROM_GROUPWORD | ROM_SKIP(6))
	ROMX_LOAD( "pt-3.17i", 0x000006, 0x080000, CRC(522917c1) SHA1(cc2f5b574d31b0b93fe52c690f450b20b233dcad) , ROM_GROUPWORD | ROM_SKIP(6))

	ROM_REGION( 0x040000, REGION_SOUND1, ROMREGION_SOUNDONLY )	/* Samples */
	ROM_LOAD( "pt-jc07.3g", 0x000000, 0x040000, CRC(5ae28b8d) SHA1(5e5f80ebbc4e3726ac8dbbfbefb9217f2e3e3563) )
ROM_END


/***************************************************************************

Puzzli
Metro/Banpresto 1995

MTR5260-A                3.5759MHz  12MHz
               YM2151                         6116
   26.666MHz           M6295    PZ.JB7  PZ.JB8
                                     78C10
      7C199         Imagetek
      7C199           14220
      61C64

                                          PZ.JB1
           68000-16                       PZ.JB2
               32MHz   6164               PZ.JB3
                       6164               PZ.JB4
    SW      PZ.JB6 PZ.JB5
    SW

***************************************************************************/

ROM_START( puzzli )
	ROM_REGION( 0x040000, REGION_CPU1, 0 )		/* 68000 Code */
	ROM_LOAD16_BYTE( "pz.jb5",       0x000000, 0x020000, CRC(33bbbd28) SHA1(41a98cfbdd60a638e4aa08f15f1730a2436106f9) )
	ROM_LOAD16_BYTE( "pz.jb6",       0x000001, 0x020000, CRC(e0bdea18) SHA1(9941a2cd88d7a3c1a640f837d9f34c39ba643ee5) )

	ROM_REGION( 0x02c000, REGION_CPU2, 0 )		/* NEC78C10 Code */
	ROM_LOAD( "pz.jb8",      0x000000, 0x004000, CRC(c652da32) SHA1(907eba5103373ca6204f9d62c426ccdeef0a3791) )
	ROM_CONTINUE(            0x010000, 0x01c000 )

	ROM_REGION( 0x200000, REGION_GFX1, 0 )	/* Gfx + Data (Addressable by CPU & Blitter) */
	ROMX_LOAD( "pz.jb2",       0x000000, 0x080000, CRC(0c0997d4) SHA1(922d8553ef505f65238e5cc77b45861a80022d75) , ROM_GROUPWORD | ROM_SKIP(6))
	ROMX_LOAD( "pz.jb4",       0x000002, 0x080000, CRC(576bc5c2) SHA1(08c10e0a3356ee1f79b78eff92395d8b18e43485) , ROM_GROUPWORD | ROM_SKIP(6))
	ROMX_LOAD( "pz.jb1",       0x000004, 0x080000, CRC(29f01eb3) SHA1(1a56f0b8efb599ae4f3cd0a4f0b6a6152ea6b117) , ROM_GROUPWORD | ROM_SKIP(6))
	ROMX_LOAD( "pz.jb3",       0x000006, 0x080000, CRC(6753e282) SHA1(49d092543db34f2cb54697897790df12ca3eda74) , ROM_GROUPWORD | ROM_SKIP(6))

	ROM_REGION( 0x040000, REGION_SOUND1, ROMREGION_SOUNDONLY )	/* Samples */
	ROM_LOAD( "pz.jb7",      0x000000, 0x040000, CRC(b3aab610) SHA1(9bcf1f98e19a7e26b22e152313dfbd43c882f008) )
ROM_END


/***************************************************************************

Sankokushi (JPN Ver.)
(c)1996 Mitchell

Board:  MTR5260-A

sound: YM2413 + M6295

***************************************************************************/

ROM_START( 3kokushi )
	ROM_REGION( 0x080000, REGION_CPU1, 0 )		/* 68000 Code */
	ROM_LOAD16_BYTE( "5.bin",        0x000000, 0x040000, CRC(6104ea35) SHA1(efb4a9a98577894fac720028f18cb9877a00239a) )
	ROM_LOAD16_BYTE( "6.bin",        0x000001, 0x040000, CRC(aac25540) SHA1(811de761bb1b3cc47d811b00f4b5c960c8f061d0) )

	ROM_REGION( 0x02c000, REGION_CPU2, 0 )		/* NEC78C10 Code */
	ROM_LOAD( "8.bin",       0x000000, 0x004000, CRC(f56cca45) SHA1(4739b83b0b3a4235fac10def3d26b0bd190eb12a) )	
	ROM_CONTINUE(            0x010000, 0x01c000 )

	ROM_REGION( 0x200000, REGION_GFX1, 0 )	/* Gfx + Data (Addressable by CPU & Blitter) */
	ROMX_LOAD( "2.bin",        0x000000, 0x080000, CRC(291f8149) SHA1(82f460517543ef544c21a81e51987fb2f5c6273d) , ROM_GROUPWORD | ROM_SKIP(6))
	ROMX_LOAD( "4.bin",        0x000002, 0x080000, CRC(9317c359) SHA1(9756757fb5d2b298a2b1917a131f391ef0e31fb9) , ROM_GROUPWORD | ROM_SKIP(6))
	ROMX_LOAD( "1.bin",        0x000004, 0x080000, CRC(d5495759) SHA1(9cbcb48915ec44a8026d88d96ab391e118e89df5) , ROM_GROUPWORD | ROM_SKIP(6))
	ROMX_LOAD( "3.bin",        0x000006, 0x080000, CRC(3d76bdf3) SHA1(f621fcc8e6bde58077216b534c2e876ea9311e15) , ROM_GROUPWORD | ROM_SKIP(6))

	ROM_REGION( 0x040000, REGION_SOUND1, ROMREGION_SOUNDONLY )	/* Samples */
	ROM_LOAD( "7.bin",       0x000000, 0x040000, CRC(78fe9d44) SHA1(365a2d51daa24741957fa619bbbbf96e8f370701) )

	/* Additional memory for the layers' ram */
	ROM_REGION( 0x20000*3, REGION_USER1, 0 )
ROM_END


/***************************************************************************

Pururun (c)1995 Metro/Banpresto
MTR5260-A

CPU  : TMP68HC000P-16
Sound: D78C10ACW M6295 YM2151 Y3012
OSC  : 24.000MHz   (OSC1)
                   (OSC2)
       26.6660MHz  (OSC3)
                   (OSC4)
       3.579545MHz (OSC5)

ROMs:
pu9-19-1.12i - Graphics (27c4096)
pu9-19-2.14i |
pu9-19-3.16i |
pu9-19-4.18i /

pu9-19-5.20e - Main programs (27c010)
pu9-19-6.20c /

pu9-19-7.3g - Sound data (27c020)
pu9-19-8.3i - Sound program (27c010)

Others:
Imagetek 14220 071 9338EK707 (208pin PQFP)
AMD MACH110-20 (CPLD)

***************************************************************************/

ROM_START( pururun )
	ROM_REGION( 0x040000, REGION_CPU1, 0 )		/* 68000 Code */
	ROM_LOAD16_BYTE( "pu9-19-5.20e", 0x000000, 0x020000, CRC(5a466a1b) SHA1(032eeaf66ce1b601385a8e76d2efd9ea6fd34680) )
	ROM_LOAD16_BYTE( "pu9-19-6.20c", 0x000001, 0x020000, CRC(d155a53c) SHA1(6916a1bad82c624b8757f5124416dac50a8dd7f5) )

	ROM_REGION( 0x02c000, REGION_CPU2, 0 )		/* NEC78C10 Code */
	ROM_LOAD( "pu9-19-8.3i", 0x000000, 0x004000, CRC(edc3830b) SHA1(13ee759d10711218465f6d7155e9c443a82b323c) )
	ROM_CONTINUE(            0x010000, 0x01c000 )

	ROM_REGION( 0x200000, REGION_GFX1, 0 )	/* Gfx + Data (Addressable by CPU & Blitter) */
	ROMX_LOAD( "pu9-19-2.14i", 0x000000, 0x080000, CRC(21550b26) SHA1(cb2a2f672cdca84def2fac8d325b7a80a1e9bfc0) , ROM_GROUPWORD | ROM_SKIP(6))
	ROMX_LOAD( "pu9-19-4.18i", 0x000002, 0x080000, CRC(3f3e216d) SHA1(9881e07d5ee237b7134e2ddcf9a9887a1d7f3b4c) , ROM_GROUPWORD | ROM_SKIP(6))
	ROMX_LOAD( "pu9-19-1.12i", 0x000004, 0x080000, CRC(7e83a75f) SHA1(9f516bbfc4ca8a8e857ebf7a19c37d7f026695a6) , ROM_GROUPWORD | ROM_SKIP(6))
	ROMX_LOAD( "pu9-19-3.16i", 0x000006, 0x080000, CRC(d15485c5) SHA1(d37670b0d696f4ee9da7b8199da114fb4e45cd20) , ROM_GROUPWORD | ROM_SKIP(6))

	ROM_REGION( 0x040000, REGION_SOUND1, ROMREGION_SOUNDONLY )	/* Samples */
	ROM_LOAD( "pu9-19-7.3g", 0x000000, 0x040000, CRC(51ae4926) SHA1(1a69a00e960bda399aaf051b3dcc9e0a108c8047) )
ROM_END


/***************************************************************************

Sky Alert (JPN Ver.)
(c)1992 Metro
VG420

CPU 	:MC68000P12
Sound	:YM2413,OKI M6295
OSC 	:24.0000MHz,3.579545MHz
other	:D78C10ACW,Imagetek Inc 14100 052

***************************************************************************/

ROM_START( skyalert )
	ROM_REGION( 0x040000, REGION_CPU1, 0 )		/* 68000 Code */
	ROM_LOAD16_BYTE( "sa_c_09.bin", 0x000000, 0x020000, CRC(6f14d9ae) SHA1(37e134af3d8461280dab971bc3ee9112f25de335) )
	ROM_LOAD16_BYTE( "sa_c_10.bin", 0x000001, 0x020000, CRC(f10bb216) SHA1(d904030fbb838d906ca69a77cffe286e903b273d) )

	ROM_REGION( 0x02c000, REGION_CPU2, 0 )		/* NEC78C10 Code */
	ROM_LOAD( "sa_b_12.bin", 0x000000, 0x004000, CRC(f358175d) SHA1(781d0f846217aa71e3c6d73c1d63bd87d1fa6b48) )	
	ROM_CONTINUE(            0x010000, 0x01c000 )

	ROM_REGION( 0x200000, REGION_GFX1, 0 )	/* Gfx + Data (Addressable by CPU & Blitter) */
	ROMX_LOAD( "sa_a_02.bin", 0x000000, 0x040000, CRC(f4f81d41) SHA1(85e587b4fda71fa5b944b0ac158d36c00e290f5f) , ROM_SKIP(7))
	ROMX_LOAD( "sa_a_04.bin", 0x000001, 0x040000, CRC(7d071e7e) SHA1(24b9b0cb7e9f719259b0444ee896bdc1ad79a28d) , ROM_SKIP(7))
	ROMX_LOAD( "sa_a_06.bin", 0x000002, 0x040000, CRC(77e4d5e1) SHA1(420e5aaf187e297b371830ebd5787675cff6177b) , ROM_SKIP(7))
	ROMX_LOAD( "sa_a_08.bin", 0x000003, 0x040000, CRC(f2a5a093) SHA1(66d482cc3f45ff7bf1363cf3c88e2dabc902a299) , ROM_SKIP(7))
	ROMX_LOAD( "sa_a_01.bin", 0x000004, 0x040000, CRC(41ec6491) SHA1(c0bd66409bc6ea969f4c45cc006fde891ba8b4d7) , ROM_SKIP(7))
	ROMX_LOAD( "sa_a_03.bin", 0x000005, 0x040000, CRC(e0dff10d) SHA1(3aa18b05f06b4b0a88ba4df86dfc0ca650c2684e) , ROM_SKIP(7))
	ROMX_LOAD( "sa_a_05.bin", 0x000006, 0x040000, CRC(62169d31) SHA1(294887b6ce0d56e053e7f7583b8a160afeef4ce5) , ROM_SKIP(7))
	ROMX_LOAD( "sa_a_07.bin", 0x000007, 0x040000, CRC(a6f5966f) SHA1(00319b96dacc4dcfd70935e1626da0ae6aa63e5a) , ROM_SKIP(7))

	ROM_REGION( 0x020000, REGION_SOUND1, ROMREGION_SOUNDONLY )	/* Samples */
	ROM_LOAD( "sa_a_11.bin", 0x000000, 0x020000, CRC(04842a60) SHA1(ade016c85867dee7ac27efe3910b01f5f8e730a0) )
ROM_END


/***************************************************************************

Toride II Adauchi Gaiden (c)1994 Metro corp
MTR5260-A

CPU  : TMP68HC000P-16
Sound: D78C10ACW M6295 YM2413
OSC  : 24.0000MHz  (OSC1)
                   (OSC2)
       26.6660MHz  (OSC3)
       3.579545MHz (OSC4)
                   (OSC5)

ROMs:
tr2aja-1.12i - Graphics (27c4096)
tr2aja-2.14i |
tr2aja-3.16i |
tr2aja-4.18i /

tr2aja-5.20e - Main programs (27c020)
tr2aja-6.20c /

tr2aja-7.3g - Sound data (27c010)
tr2aja-8.3i - Sound program (27c010)

Others:
Imagetek 14220 071 9338EK700 (208pin PQFP)
AMD MACH110-20 (CPLD)

***************************************************************************/

ROM_START( toride2g )
	ROM_REGION( 0x080000, REGION_CPU1, 0 )		/* 68000 Code */
	ROM_LOAD16_BYTE( "tr2aja-5.20e", 0x000000, 0x040000, CRC(b96a52f6) SHA1(353b5599d50d96b96bdd6352c046ad669cf8da44) )
	ROM_LOAD16_BYTE( "tr2aja-6.20c", 0x000001, 0x040000, CRC(2918b6b4) SHA1(86ebb884759dc9a8a701784d19845467aa1ce11b) )

	ROM_REGION( 0x02c000, REGION_CPU2, 0 )		/* NEC78C10 Code */
	ROM_LOAD( "tr2aja-8.3i", 0x000000, 0x004000, CRC(fdd29146) SHA1(8e996e1afd33f16d35ebf5a40829feb3e92f781f) )
	ROM_CONTINUE(            0x010000, 0x01c000 )

	ROM_REGION( 0x200000, REGION_GFX1, 0 )	/* Gfx + Data (Addressable by CPU & Blitter) */
	ROMX_LOAD( "tr2aja-2.14i", 0x000000, 0x080000, CRC(5c73f629) SHA1(b38b7ee213bcc0dd5e4c339a8f9f2fdd81ede6ad) , ROM_GROUPWORD | ROM_SKIP(6))
	ROMX_LOAD( "tr2aja-4.18i", 0x000002, 0x080000, CRC(67ebaf1b) SHA1(a0c5f253cc33620251fb58ef6f1647453d778462) , ROM_GROUPWORD | ROM_SKIP(6))
	ROMX_LOAD( "tr2aja-1.12i", 0x000004, 0x080000, CRC(96245a5c) SHA1(524990c88a08648de6f330652fc5c02a27e1325c) , ROM_GROUPWORD | ROM_SKIP(6))
	ROMX_LOAD( "tr2aja-3.16i", 0x000006, 0x080000, CRC(49013f5d) SHA1(8f29bd2606b30260e9b21886f2b257f7ae8fb2bf) , ROM_GROUPWORD | ROM_SKIP(6))

	ROM_REGION( 0x020000, REGION_SOUND1, ROMREGION_SOUNDONLY )	/* Samples */
	ROM_LOAD( "tr2aja-7.3g", 0x000000, 0x020000, CRC(630c6193) SHA1(ddb63724e0b0f7264cb02904e49b24b87beb35a9) )
ROM_END

ROM_START( gunmast )
	ROM_REGION( 0x080000, REGION_CPU1, 0 )		/* 68000 Code */
	ROM_LOAD16_BYTE( "gmja-5.20e", 0x000000, 0x040000, CRC(7334b2a3) SHA1(23f0a00b7539329f23eb564bc2823383997f83a9) )
	ROM_LOAD16_BYTE( "gmja-6.20c", 0x000001, 0x040000, CRC(c38d185e) SHA1(fdbc16a6ffc791778cb7ac2dafd15f4eb72c4cf9) )

	ROM_REGION( 0x02c000, REGION_CPU2, 0 )		/* NEC78C10 Code */
	ROM_LOAD( "gmja-8.3i", 0x000000, 0x004000, CRC(ab4bcc56) SHA1(9ef91e14d0974f30c874a12370ddd04ee8ab6d5d) )
	ROM_CONTINUE(          0x010000, 0x01c000 )

	ROM_REGION( 0x200000, REGION_GFX1, 0 )	/* Gfx + Data (Addressable by CPU & Blitter) */
	ROMX_LOAD( "gmja-2.14i", 0x000000, 0x080000, CRC(bc9acd54) SHA1(e6154cc5e8e33b38f56a0055dd0a51aa6adc4f9c) , ROM_GROUPWORD | ROM_SKIP(6))
	ROMX_LOAD( "gmja-4.18i", 0x000002, 0x080000, CRC(f2d72d90) SHA1(575a01999e4608d1503904ba22310413b680b2b9) , ROM_GROUPWORD | ROM_SKIP(6))
	ROMX_LOAD( "gmja-1.12i", 0x000004, 0x080000, CRC(336d0a90) SHA1(39ff59ba13e21f2a8488e5dc2d44cf2c50f7c4fb) , ROM_GROUPWORD | ROM_SKIP(6))
	ROMX_LOAD( "gmja-3.16i", 0x000006, 0x080000, CRC(a6651297) SHA1(cdfb8a176cced552a9e72d39980c7fb005edf4f9) , ROM_GROUPWORD | ROM_SKIP(6))

	ROM_REGION( 0x040000, REGION_SOUND1, ROMREGION_SOUNDONLY )	/* Samples */
	ROM_LOAD( "gmja-7.3g", 0x000000, 0x040000, CRC(3a342312) SHA1(5c31bc9ec5159e1a0c9a931c7b702a31d3a1af10) )
ROM_END

ROM_START( msgogo )
	ROM_REGION( 0x080000, REGION_CPU1, 0 )		/* 68000 Code */
	ROM_LOAD16_BYTE( "ms_wa-6.6", 0x000000, 0x040000, CRC(986acac8) SHA1(97c24f5b730aa811951db4c7e9c894c0701c58fd) )
	ROM_LOAD16_BYTE( "ms_wa-5.5", 0x000001, 0x040000, CRC(746d9f99) SHA1(6e3e34dfb67fecc93213fe040465eccd88575822) )

	ROM_REGION( 0x200000, REGION_GFX1, 0 )	/* Gfx + Data (Addressable by CPU & Blitter) */
	ROMX_LOAD( "ms_wa-2.2", 0x000000, 0x080000, CRC(0d36c2b9) SHA1(3fd6631ad657c73e7e6bfdff9d9caf5ab044bdeb), ROM_GROUPWORD | ROM_SKIP(6))
	ROMX_LOAD( "ms_wa-4.4", 0x000002, 0x080000, CRC(fd387126) SHA1(a2f82a66b098a97d8f245e3c2f96c31c63642fec), ROM_GROUPWORD | ROM_SKIP(6))
	ROMX_LOAD( "ms_ja-1.1", 0x000004, 0x080000, CRC(8ec4e81d) SHA1(46947ad2941af154f91e47acee281302a12e3aa5), ROM_GROUPWORD | ROM_SKIP(6))
	ROMX_LOAD( "ms_wa-3.3", 0x000006, 0x080000, CRC(06cb6807) SHA1(d7303b4047983117cd33e057b1f4b98ed3f7dd32), ROM_GROUPWORD | ROM_SKIP(6))

	ROM_REGION( 0x280000, REGION_SOUND1, ROMREGION_SOUNDONLY )	/* Samples */
	ROM_LOAD( "yrw801-m",  0x000000, 0x200000, CRC(2a9d8d43) SHA1(32760893ce06dbe3930627755ba065cc3d8ec6ca) )
	ROM_LOAD( "ms_wa-7.7", 0x200000, 0x080000, CRC(e19941cb) SHA1(93777c9cd22ddd33d9584b6edad33b95c1e28bde) )
ROM_END


/***************************************************************************


								Game Drivers


***************************************************************************/

GAME ( 1992, karatour, 0,        karatour, karatour, karatour, ROT0,   "Mitchell",                   "The Karate Tournament"                          )
GAME ( 1992, pangpoms, 0,        pangpoms, pangpoms, metro,    ROT0,   "Metro",                      "Pang Poms"                                      )
GAME ( 1992, pangpomm, pangpoms, pangpoms, pangpoms, metro,    ROT0,   "Metro (Mitchell license)",   "Pang Poms (Mitchell)"                           )
GAME ( 1992, skyalert, 0,        skyalert, skyalert, metro,    ROT270, "Metro",                      "Sky Alert"                                      )
GAMEX( 1993?,ladykill, 0,        karatour, ladykill, karatour, ROT90,  "Yanyaka (Mitchell license)", "Lady Killer",                     GAME_IMPERFECT_GRAPHICS )
GAMEX( 1993?,moegonta, ladykill, karatour, moegonta, karatour, ROT90,  "Yanyaka",                    "Moeyo Gonta!! (Japan)",           GAME_IMPERFECT_GRAPHICS )
GAME ( 1993, poitto,   0,        poitto,   poitto,   metro,    ROT0,   "Metro / Able Corp.",         "Poitto!"                                        )
GAME ( 1994, dharma,   0,        dharma,   dharma,   metro,    ROT0,   "Metro",                      "Dharma Doujou"                                  )
GAME ( 1994, lastfort, 0,        lastfort, lastfort, metro,    ROT0,   "Metro",                      "Last Fortress - Toride"                         )
GAME ( 1994, lastfero, lastfort, lastfort, lastfero, metro,    ROT0,   "Metro",                      "Last Fortress - Toride (Erotic)"                )
GAME ( 1994, toride2g, 0,        toride2g, toride2g, metro,    ROT0,   "Metro",                      "Toride II Adauchi Gaiden"                       )
GAME ( 1995, daitorid, 0,        daitorid, daitorid, metro,    ROT0,   "Metro",                      "Daitoride"                                      )
GAME ( 1995, dokyusei, 0,        dokyusei, dokyusei, gakusai,  ROT0,   "Make Software / Elf / Media Trading", "Mahjong Doukyuusei"                    )
GAME ( 1995, dokyusp,  0,        dokyusp,  gakusai,  gakusai,  ROT0,   "Make Software / Elf / Media Trading", "Mahjong Doukyuusei Special"            )
GAME ( 1995, pururun,  0,        pururun,  pururun,  metro,    ROT0,   "Metro / Banpresto",          "Pururun"                                        )
GAMEX( 1995, puzzli,   0,        daitorid, puzzli,   metro,    ROT0,   "Metro / Banpresto",          "Puzzli",                          GAME_IMPERFECT_GRAPHICS )
GAMEX( 1996, 3kokushi, 0,        3kokushi, 3kokushi, karatour, ROT0,   "Mitchell",                   "Sankokushi (Japan)",              GAME_IMPERFECT_GRAPHICS )
GAME ( 1996, balcube,  0,        balcube,  balcube,  balcube,  ROT0,   "Metro",                      "Bal Cube"                                       )
GAME ( 1996, bangball, 0,        bangball, bangball, balcube,  ROT0,   "Banpresto / Kunihiko Tashiro+Goodhouse", "Bang Bang Ball (v1.05)"             )
GAMEX( 1996, mouja,    0,        mouja,    mouja,    mouja,    ROT0,   "Etona",                      "Mouja (Japan)",                   GAME_NO_COCKTAIL )
GAMEX( 1997, gakusai,  0,        gakusai,  gakusai,  gakusai,  ROT0,   "MakeSoft",                   "Mahjong Gakuensai (Japan)",       GAME_IMPERFECT_GRAPHICS )
GAME ( 1998, gakusai2, 0,        gakusai2, gakusai,  gakusai,  ROT0,   "MakeSoft",                   "Mahjong Gakuensai 2 (Japan)"                    )
GAME ( 1994, gunmast,  0,        gunmast,  gunmast,  daitorid, ROT0,   "Metro",                      "Gun Master"                                     )
GAME ( 1995, msgogo,   0,        msgogo,   msgogo,   balcube,  ROT0,   "Metro",                      "Mouse Shooter GoGo"                             ) 

GAMEX( 1994, blzntrnd, 0,        blzntrnd, blzntrnd, blzntrnd, ROT0,   "Human Amusement",            "Blazing Tornado",                 GAME_IMPERFECT_GRAPHICS )
GAMEX( 1996, gstrik2,  0,        gstrik2,  gstrik2,  blzntrnd, ROT0,   "Human Amusement",            "Grand Striker 2 (Japan)",			GAME_IMPERFECT_GRAPHICS ) /* priority between rounds*/
