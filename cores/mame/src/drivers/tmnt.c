/***************************************************************************

This driver contains several Konami 68000 based games. For the most part they
run on incompatible boards, but since 90% of the work is done by the custom
ICs emulated in vidhrdw/konamiic.c, we can just as well keep them all
together.

driver by Nicola Salmoria

Notes:
- Golfing Greats has a peculiar way to know where the ball is laying: the
  hardware latches the color of roz pixel at the center (more or less) of the
  screen, and uses that to determine if it's water, fairway etc.

TODO:

- glfgretj is in worse shape than glfgreat, the latter is at least playable,
  the former hangs.
- glfgretj uses a special controller.
  1 "shot controller (with stance selection button on the top of it)" and 3
  buttons for shot direction (right/left) and club selection.
  Twist the "shot controller" to adjust shot power, then release it.
  The controller returns to its default position by internal spring.
- prmrsocr: when the field rotates before the penalty kicks, parts of the
  053936 tilemap that shouldn't be seen are visible. Maybe the tilemap ROM is
  banked, or there are controls to clip the visible region (registers 0x06 and
  0x07 of the 053936) or both.
- is IPT_VBLANK really vblank or something else? Investigate.
- some slowdowns in lgtnfght when there are many sprites on screen - vblank issue?

Updates:

- detatwin: sprites are left on screen during attract mode(fixed)
  Sprite buffer should be cleared at vblank start. On the GX OBJDMA
  automatically occurs 32.0-42.7us after clearing but on older boards
  using the K053245, DMA must be triggered manually. The game uses a
  trick to disable sprites by simply not triggering OBJDMA.
- a garbage sprite is STILL sticking on screen in ssriders.(fixed)
- sprite colors / zoomed placement in tmnt2(improved MCU sim)
- I don't think I'm handling the palette dim control in tmnt2/ssriders
  correctly. TMNT2 stays dimmed most of the time.(fixed)
- sprite lag, quite evident in lgtnfght and mia but also in the others.
  Also see the left corner of the wall in punkshot DownTown level(should be better)
- ssriders: Billy no longer goes berserk at stage 4's boss.

* uncertain bugs:
- Detana!! Twin Bee's remaining sprite lag does not appear to be
  emulation related. While these common one-pixel lags are very obvious
  on VGA-class displays they're virtually invisible on TV and older
  15kHz arcade monitors.
- TMNT2: "BIG APPLE, 3 AM" is not played in the first loop. I believe
  it's a design decision because it'll cut into the last intro dialogue.
  The game looks up sound codes of these prologue lines from two different
  tables. One for the first loop and the other for any loop thereafter.
  The SNES port plays it everytime probably because it has no intro speech.
  If the real board turns out otherwise I have no idea how to fix it except
  patching the first table.
- TMNT: In the sewer level when purple foot soldiers jump out of the water
  they turn white for a short moment. The color index is copied straight
  form ROM and nothing unusual is found in that part the code. There is no
  known protection in this game so this bug may need reconfirmation.
(081003AT)

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "vidhrdw/konamiic.h"
#include "machine/eeprom.h"
#include "cpu/m68000/m68000.h"
#include "cpu/z80/z80.h"
#include "math.h"

WRITE16_HANDLER( tmnt_paletteram_word_w );
WRITE16_HANDLER( tmnt_0a0000_w );
WRITE16_HANDLER( punkshot_0a0020_w );
WRITE16_HANDLER( lgtnfght_0a0018_w );
WRITE16_HANDLER( detatwin_700300_w );
READ16_HANDLER( glfgreat_rom_r );
WRITE16_HANDLER( glfgreat_122000_w );
WRITE16_HANDLER( ssriders_1c0300_w );
WRITE16_HANDLER( prmrsocr_122000_w );
WRITE16_HANDLER( tmnt_priority_w );
READ16_HANDLER( glfgreat_ball_r );
VIDEO_START( sunsetbl );
VIDEO_START( cuebrckj );
VIDEO_START( mia );
VIDEO_START( tmnt );
VIDEO_START( punkshot );
VIDEO_START( lgtnfght );
VIDEO_START( detatwin );
VIDEO_START( glfgreat );
VIDEO_START( thndrx2 );
VIDEO_START( prmrsocr );
VIDEO_UPDATE( mia );
VIDEO_UPDATE( tmnt );
VIDEO_UPDATE( punkshot );
VIDEO_UPDATE( lgtnfght );
VIDEO_UPDATE( glfgreat );
VIDEO_UPDATE( tmnt2 );
VIDEO_UPDATE( thndrx2 );
VIDEO_EOF( detatwin );

static int tmnt_soundlatch;
static int cbj_snd_irqlatch, cbj_nvram_bank;
static data16_t cbj_nvram[0x400*0x20];	/* 32k paged in a 1k window*/

static READ16_HANDLER( K052109_word_noA12_r )
{
	/* some games have the A12 line not connected, so the chip spans */
	/* twice the memory range, with mirroring */
	offset = ((offset & 0x3000) >> 1) | (offset & 0x07ff);
	return K052109_word_r(offset,mem_mask);
}

static WRITE16_HANDLER( K052109_word_noA12_w )
{
	/* some games have the A12 line not connected, so the chip spans */
	/* twice the memory range, with mirroring */
	offset = ((offset & 0x3000) >> 1) | (offset & 0x07ff);
	K052109_word_w(offset,data,mem_mask);
}

WRITE16_HANDLER( punkshot_K052109_word_w )
{
	/* it seems that a word write is supposed to affect only the MSB. The */
	/* "ROUND 1" text in punkshtj goes lost otherwise. */
	if (ACCESSING_MSB)
		K052109_w(offset,(data >> 8) & 0xff);
	else if (ACCESSING_LSB)
		K052109_w(offset + 0x2000,data & 0xff);
}

static WRITE16_HANDLER( punkshot_K052109_word_noA12_w )
{
	/* some games have the A12 line not connected, so the chip spans */
	/* twice the memory range, with mirroring */
	offset = ((offset & 0x3000) >> 1) | (offset & 0x07ff);
	punkshot_K052109_word_w(offset,data,mem_mask);
}


/* the interface with the 053245 is weird. The chip can address only 0x800 bytes */
/* of RAM, but they put 0x4000 there. The CPU can access them all. Address lines */
/* A1, A5 and A6 don't go to the 053245. */
static READ16_HANDLER( K053245_scattered_word_r )
{
	if (offset & 0x0031)
		return spriteram16[offset];
	else
	{
		offset = ((offset & 0x000e) >> 1) | ((offset & 0x1fc0) >> 3);
		return K053245_word_r(offset,mem_mask);
	}
}

static WRITE16_HANDLER( K053245_scattered_word_w ) /***/
{
	COMBINE_DATA(spriteram16 + offset);

	if (!(offset & 0x0031))
	{
		offset = ((offset & 0x000e) >> 1) | ((offset & 0x1fc0) >> 3);
		K053245_word_w(offset,data,mem_mask);
	}
}

static READ16_HANDLER( K053244_word_noA1_r )
{
	offset &= ~1;	/* handle mirror address */

	return K053244_r(offset + 1) | (K053244_r(offset) << 8);
}

static WRITE16_HANDLER( K053244_word_noA1_w )
{
	offset &= ~1;	/* handle mirror address */

	if (ACCESSING_MSB)
		K053244_w(offset,(data >> 8) & 0xff);
	if (ACCESSING_LSB)
		K053244_w(offset + 1,data & 0xff);
}

static INTERRUPT_GEN(cbj_interrupt)
{
	/* cheap IRQ multiplexing to avoid losing sound IRQs*/
	switch (cpu_getiloops())
	{
		case 0:
			cpu_set_irq_line(0, MC68000_IRQ_5, HOLD_LINE);
			break;

		default:
			if (cbj_snd_irqlatch)
				cpu_set_irq_line(0, MC68000_IRQ_6, HOLD_LINE);
			break;
	}
}

static INTERRUPT_GEN( punkshot_interrupt )
{
	if (K052109_is_IRQ_enabled()) irq4_line_hold();

}

static INTERRUPT_GEN( lgtnfght_interrupt )
{
	if (K052109_is_IRQ_enabled()) irq5_line_hold();

}



static WRITE16_HANDLER( tmnt_sound_command_w )
{
	if (ACCESSING_LSB)
		soundlatch_w(0,data & 0xff);
}

static READ16_HANDLER( punkshot_sound_r )
{
	/* If the sound CPU is running, read the status, otherwise
	   just make it pass the test */
	if (Machine->sample_rate != 0) 	return K053260_0_r(2 + offset);
	else return 0x80;
}

static READ16_HANDLER( detatwin_sound_r )
{
	/* If the sound CPU is running, read the status, otherwise
	   just make it pass the test */
	if (Machine->sample_rate != 0) 	return K053260_0_r(2 + offset);
	else return offset ? 0xfe : 0x00;
}

static READ16_HANDLER( glfgreat_sound_r )
{
	/* If the sound CPU is running, read the status, otherwise
	   just make it pass the test */
	if (Machine->sample_rate != 0) 	return K053260_0_r(2 + offset) << 8;
	else return 0;
}

static WRITE16_HANDLER( glfgreat_sound_w )
{
	if (ACCESSING_MSB)
		K053260_0_w(offset, (data >> 8) & 0xff);

	if (offset)
		cpu_set_irq_line_and_vector(1,0,HOLD_LINE,0xff);
}

static READ16_HANDLER( prmrsocr_sound_r )
{
	return soundlatch3_r(0);
}

static WRITE16_HANDLER( prmrsocr_sound_cmd_w )
{
	if (ACCESSING_LSB)
	{
		data &= 0xff;
		if (offset == 0) soundlatch_w(0,data);
		else soundlatch2_w(0,data);

		/* If the sound CPU is not running, make the tests pass anyway */
		if (offset == 0 && !Machine->sample_rate)
		{
			if (data == 0xfe)	/* ROM & RAM test */
				soundlatch3_w(0,0x0f);
			if (data == 0xfc)	/* sample ROM test */
				soundlatch3_w(0,0x10);
		}
	}
}

static WRITE16_HANDLER( prmrsocr_sound_irq_w )
{
	cpu_set_irq_line_and_vector(1,0,HOLD_LINE,0xff);
}

static WRITE_HANDLER( prmrsocr_s_bankswitch_w )
{
	data8_t *rom = memory_region(REGION_CPU2) + 0x10000;

	cpu_setbank(1,rom + (data & 7) * 0x4000);
}


static READ16_HANDLER( tmnt2_sound_r )
{
	/* If the sound CPU is running, read the status, otherwise
	   just make it pass the test */
	if (Machine->sample_rate != 0) 	return K053260_0_r(2 + offset);
	else return offset ? 0x00 : 0x80;
}

READ_HANDLER( tmnt_sres_r )
{
	return tmnt_soundlatch;
}

WRITE_HANDLER( tmnt_sres_w )
{
	/* bit 1 resets the UPD7795C sound chip */
	upd7759_reset_w(0, data & 2);

	/* bit 2 plays the title music */
	if (data & 0x04)
	{
		if (!sample_playing(0))	sample_start(0,0,0);
	}
	else sample_stop(0);
	tmnt_soundlatch = data;
}


static int tmnt_decode_sample(const struct MachineSound *msound)
{
	int i;
	signed short *dest;
	unsigned char *source = memory_region(REGION_SOUND3);
	struct GameSamples *samples;


	if ((Machine->samples = auto_malloc(sizeof(struct GameSamples))) == NULL)
		return 1;

	samples = Machine->samples;

	if ((samples->sample[0] = auto_malloc(sizeof(struct GameSample) + (0x40000)*sizeof(short))) == NULL)
		return 1;

	samples->sample[0]->length = 0x40000*2;
	samples->sample[0]->smpfreq = 20000;	/* 20 kHz */
	samples->sample[0]->resolution = 16;
	dest = (signed short *)samples->sample[0]->data;
	samples->total = 1;

	/*	Sound sample for TMNT.D05 is stored in the following mode (ym3012 format):
	 *
	 *	Bit 15-13:	Exponent (2 ^ x)
	 *	Bit 12-3 :	Sound data (10 bit)
	 *
	 *	(Sound info courtesy of Dave <dave@finalburn.com>)
	 */

	for (i = 0; i < 0x40000; i++)
	{
		int val = source[2 * i] + source[2 * i + 1] * 256;
		int expo = val >> 13;
		val = (val >> 3) & (0x3ff); /* 10 bit, Max Amplitude 0x400 */
		val -= 0x200;                   /* Centralize value */

		val = (val << expo) >> 3;

		dest[i] = val;
	}

	/*	The sample is now ready to be used.  It's a 16 bit, 22kHz sample.
	 */

	return 0;
}

#if 0
static int sound_nmi_enabled;

static void sound_nmi_callback( int param )
{
	cpu_set_nmi_line( 1, ( sound_nmi_enabled ) ? CLEAR_LINE : ASSERT_LINE );

	sound_nmi_enabled = 0;
}
#endif

static void nmi_callback(int param)
{
	cpu_set_nmi_line(1,ASSERT_LINE);
}

static WRITE_HANDLER( sound_arm_nmi_w )
{
/*	sound_nmi_enabled = 1;*/
	cpu_set_nmi_line(1,CLEAR_LINE);
	timer_set(TIME_IN_USEC(50),0,nmi_callback);	/* kludge until the K053260 is emulated correctly */
}





static READ16_HANDLER( punkshot_kludge_r )
{
	/* I don't know what's going on here; at one point, the code reads location */
	/* 0xffffff, and returning 0 causes the game to mess up - locking up in a */
	/* loop where the ball is continuously bouncing from the basket. Returning */
	/* a random number seems to prevent that. */
	return rand();
}


/* protection simulation derived from a bootleg */
static READ16_HANDLER( ssriders_protection_r )
{
    int data = cpu_readmem24bew_word(0x105a0a);
    int cmd = cpu_readmem24bew_word(0x1058fc);

	switch (cmd)
	{
		case 0x100b:
			/* read twice in a row, first result discarded? */
			/* data is always == 0x75c */
			return 0x0064;

		case 0x6003:
			/* start of level */
			return data & 0x000f;

		case 0x6004:
			return data & 0x001f;

		case 0x6000:
			return data & 0x0001;

		case 0x0000:
			return data & 0x00ff;

		case 0x6007:
			return data & 0x00ff;

		case 0x8abc:
			/* collision table */
			data = -cpu_readmem24bew_word(0x105818);
			data = ((data / 8 - 4) & 0x1f) * 0x40;
			/* 0x1040c8 is the x scroll buffer, avoids stutter on slopes + scrolling (and it's actually more logical as HW pov)*/
			data += ((cpu_readmem24bew_word(0x105cb0) + cpu_readmem24bew_word(0x1040c8) - 6) / 8 + 12) & 0x3f;
 			return data;

		default:
			usrintf_showmessage("%06x: unknown protection read",activecpu_get_pc());
			log_cb(RETRO_LOG_DEBUG, LOGPRE "%06x: read 1c0800 (D7=%02x 1058fc=%02x 105a0a=%02x)\n",activecpu_get_pc(),activecpu_get_reg(M68K_D7),cmd,data);
			return 0xffff;
    }
}

static WRITE16_HANDLER( ssriders_protection_w )
{
	if (offset == 1)
	{
		int logical_pri,hardware_pri;

		/* create sprite priority attributes */
		hardware_pri = 1;
		for (logical_pri = 1;logical_pri < 0x100;logical_pri <<= 1)
		{
			int i;

			for (i = 0;i < 128;i++)
			{
				if ((cpu_readmem24bew_word(0x180006 + 128*i) >> 8) == logical_pri)
				{
					K053245_word_w(8*i,hardware_pri,0xff00);
					hardware_pri++;
				}
			}
		}
	}
}



/***************************************************************************

  EEPROM

***************************************************************************/

static int init_eeprom_count;


static struct EEPROM_interface eeprom_interface =
{
	7,				/* address bits */
	8,				/* data bits */
	"011000",		/*  read command */
	"011100",		/* write command */
	0,				/* erase command */
	"0100000000000",/* lock command */
	"0100110000000" /* unlock command */
};

static NVRAM_HANDLER( eeprom )
{
	if (read_or_write)
		EEPROM_save(file);
	else
	{
		EEPROM_init(&eeprom_interface);

		if (file)
		{
			init_eeprom_count = 0;
			EEPROM_load(file);
		}
		else
			init_eeprom_count = 10;
	}
}

static READ16_HANDLER( detatwin_coin_r )
{
	int res;
	static int toggle;

	/* bit 3 is service button */
	/* bit 6 is ??? VBLANK? OBJMPX? */
	res = input_port_2_word_r(0,0);
	if (init_eeprom_count)
	{
		init_eeprom_count--;
		res &= 0xf7;
	}
	toggle ^= 0x40;
	return res ^ toggle;
}

static READ16_HANDLER( detatwin_eeprom_r )
{
	int res;

	/* bit 0 is EEPROM data */
	/* bit 1 is EEPROM ready */
	res = EEPROM_read_bit() | input_port_3_word_r(0,0);
	return res;
}

static READ16_HANDLER( ssriders_eeprom_r )
{
	int res;
	static int toggle;

	/* bit 0 is EEPROM data */
	/* bit 1 is EEPROM ready */
	/* bit 2 is VBLANK (???) */
	/* bit 7 is service button */
	res = EEPROM_read_bit() | input_port_3_word_r(0,0);
	if (init_eeprom_count)
	{
		init_eeprom_count--;
		res &= 0x7f;
	}
	toggle ^= 0x04;
	return res ^ toggle;
}

static READ16_HANDLER( ssridersbl_eeprom_r )
{
	int res;
	static int toggle;

	/* bit 0 is EEPROM data */
	/* bit 1 is EEPROM ready */
	/* bit 2 is VBLANK (???) */
	/* bit 3 is service button */
	res = EEPROM_read_bit() | input_port_3_word_r(0,0);
	if (init_eeprom_count)
	{
		init_eeprom_count--;
		res &= 0xf7;
	}
	toggle ^= 0x04;
	return res ^ toggle;
}

static WRITE16_HANDLER( detatwin_eeprom_w )
{
	if (ACCESSING_LSB)
	{
		/* bit 0 is data */
		/* bit 1 is cs (active low) */
		/* bit 2 is clock (active high) */
		EEPROM_write_bit(data & 0x01);
		EEPROM_set_cs_line((data & 0x02) ? CLEAR_LINE : ASSERT_LINE);
		EEPROM_set_clock_line((data & 0x04) ? ASSERT_LINE : CLEAR_LINE);
	}
}

WRITE16_HANDLER( ssriders_eeprom_w );	/* in vidhrdw/tmnt.c */


static struct EEPROM_interface thndrx2_eeprom_interface =
{
	7,				/* address bits */
	8,				/* data bits */
	"011000",		/*  read command */
	"010100",		/* write command */
	0,				/* erase command */
	"0100000000000",/* lock command */
	"0100110000000" /* unlock command */
};

static NVRAM_HANDLER( thndrx2 )
{
	if (read_or_write)
		EEPROM_save(file);
	else
	{
		EEPROM_init(&thndrx2_eeprom_interface);

		if (file)
		{
			init_eeprom_count = 0;
			EEPROM_load(file);
		}
		else
			init_eeprom_count = 10;
	}
}

static READ16_HANDLER( thndrx2_in0_r )
{
	int res;

	res = input_port_0_word_r(0,0);
	if (init_eeprom_count)
	{
		init_eeprom_count--;
		res &= 0xf7ff;
	}
	return res;
}

static READ16_HANDLER( thndrx2_eeprom_r )
{
	int res;
	static int toggle;

	/* bit 0 is EEPROM data */
	/* bit 1 is EEPROM ready */
	/* bit 3 is VBLANK (???) */
	/* bit 7 is service button */
	res = (EEPROM_read_bit() << 8) | input_port_1_word_r(0,0);
	toggle ^= 0x0800;
	return (res ^ toggle);
}

static WRITE16_HANDLER( thndrx2_eeprom_w )
{
	static int last;

	if (ACCESSING_LSB)
	{
		/* bit 0 is data */
		/* bit 1 is cs (active low) */
		/* bit 2 is clock (active high) */
		EEPROM_write_bit(data & 0x01);
		EEPROM_set_cs_line((data & 0x02) ? CLEAR_LINE : ASSERT_LINE);
		EEPROM_set_clock_line((data & 0x04) ? ASSERT_LINE : CLEAR_LINE);

		/* bit 5 triggers IRQ on sound cpu */
		if (last == 0 && (data & 0x20) != 0)
			cpu_set_irq_line_and_vector(1,0,HOLD_LINE,0xff);
		last = data & 0x20;

		/* bit 6 = enable char ROM reading through the video RAM */
		K052109_set_RMRD_line((data & 0x40) ? ASSERT_LINE : CLEAR_LINE);
	}
}


static READ16_HANDLER( prmrsocr_IN0_r )
{
	/* bit 9 is service button */
	int res;

	res = input_port_0_word_r(0,0);
	if (init_eeprom_count)
	{
		init_eeprom_count--;
		res &= 0xfdff;
	}
	return res;
}

static READ16_HANDLER( prmrsocr_eeprom_r )
{
	/* bit 8 is EEPROM data */
	/* bit 9 is EEPROM ready */
	return (EEPROM_read_bit() << 8) | input_port_1_word_r(0,0);
}

static WRITE16_HANDLER( prmrsocr_eeprom_w )
{
	if (ACCESSING_LSB)
	{
		prmrsocr_122000_w(offset,data,mem_mask);
	}

	if (ACCESSING_MSB)
	{
		/* bit 8 is data */
		/* bit 9 is cs (active low) */
		/* bit 10 is clock (active high) */
		EEPROM_write_bit(data & 0x0100);
		EEPROM_set_cs_line((data & 0x0200) ? CLEAR_LINE : ASSERT_LINE);
		EEPROM_set_clock_line((data & 0x0400) ? ASSERT_LINE : CLEAR_LINE);
	}
}

static READ16_HANDLER( cbj_snd_r )
{
	return YM2151_status_port_0_r(0)<<8;
}

static WRITE16_HANDLER( cbj_snd_w )
{
	if (offset)
	{
		YM2151_data_port_0_w(0, data>>8);
	}
	else
	{
		YM2151_register_port_0_w(0, data>>8);
	}
}

static READ16_HANDLER( cbj_nv_r )
{
	return cbj_nvram[offset + (cbj_nvram_bank*0x400/2)];
}

static WRITE16_HANDLER( cbj_nv_w )
{
       COMBINE_DATA(&cbj_nvram[offset + (cbj_nvram_bank*0x400/2)]);
}

static WRITE16_HANDLER( cbj_nvbank_w )
{
	cbj_nvram_bank = (data>>8);
}

static MEMORY_READ16_START( cuebrckj_readmem )
	{ 0x000000, 0x01ffff, MRA16_ROM },
	{ 0x040000, 0x043fff, MRA16_RAM },	/* main RAM */
	{ 0x060000, 0x063fff, MRA16_RAM },	/* main RAM */
	{ 0x080000, 0x080fff, MRA16_RAM },
	{ 0x0a0000, 0x0a0001, input_port_0_word_r },
	{ 0x0a0002, 0x0a0003, input_port_1_word_r },
	{ 0x0a0004, 0x0a0005, input_port_2_word_r },
	{ 0x0a0010, 0x0a0011, input_port_3_word_r },
	{ 0x0a0012, 0x0a0013, input_port_4_word_r },
	{ 0x0a0018, 0x0a0019, input_port_5_word_r },
	{ 0x0b0000, 0x0b03ff, cbj_nv_r },
	{ 0x0c0000, 0x0c0003, cbj_snd_r },
	{ 0x100000, 0x107fff, K052109_word_noA12_r },
	{ 0x140000, 0x140007, K051937_word_r },
	{ 0x140400, 0x1407ff, K051960_word_r },
MEMORY_END

static MEMORY_WRITE16_START( cuebrckj_writemem )
	{ 0x000000, 0x01ffff, MWA16_ROM },
	{ 0x040000, 0x043fff, MWA16_RAM },	/* main RAM */
	{ 0x060000, 0x063fff, MWA16_RAM },	/* main RAM */
	{ 0x080000, 0x080fff, tmnt_paletteram_word_w, &paletteram16 },
	{ 0x0a0000, 0x0a0001, tmnt_0a0000_w },
	{ 0x0a0008, 0x0a0009, tmnt_sound_command_w },
	{ 0x0a0010, 0x0a0011, watchdog_reset16_w },
	{ 0x0b0000, 0x0b03ff, cbj_nv_w },
	{ 0x0b0400, 0x0b0401, cbj_nvbank_w },
	{ 0x0c0000, 0x0c0003, cbj_snd_w },
	{ 0x100000, 0x107fff, K052109_word_noA12_w },
	{ 0x140000, 0x140007, K051937_word_w },
	{ 0x140400, 0x1407ff, K051960_word_w },
MEMORY_END


static MEMORY_READ16_START( mia_readmem )
	{ 0x000000, 0x03ffff, MRA16_ROM },
	{ 0x040000, 0x043fff, MRA16_RAM },	/* main RAM */
	{ 0x060000, 0x063fff, MRA16_RAM },	/* main RAM */
	{ 0x080000, 0x080fff, MRA16_RAM },
	{ 0x0a0000, 0x0a0001, input_port_0_word_r },
	{ 0x0a0002, 0x0a0003, input_port_1_word_r },
	{ 0x0a0004, 0x0a0005, input_port_2_word_r },
	{ 0x0a0010, 0x0a0011, input_port_3_word_r },
	{ 0x0a0012, 0x0a0013, input_port_4_word_r },
	{ 0x0a0018, 0x0a0019, input_port_5_word_r },
	{ 0x100000, 0x107fff, K052109_word_noA12_r },
	{ 0x140000, 0x140007, K051937_word_r },
	{ 0x140400, 0x1407ff, K051960_word_r },
MEMORY_END

static MEMORY_WRITE16_START( mia_writemem )
	{ 0x000000, 0x03ffff, MWA16_ROM },
	{ 0x040000, 0x043fff, MWA16_RAM },	/* main RAM */
	{ 0x060000, 0x063fff, MWA16_RAM },	/* main RAM */
	{ 0x080000, 0x080fff, tmnt_paletteram_word_w, &paletteram16 },
	{ 0x0a0000, 0x0a0001, tmnt_0a0000_w },
	{ 0x0a0008, 0x0a0009, tmnt_sound_command_w },
	{ 0x0a0010, 0x0a0011, watchdog_reset16_w },
	{ 0x100000, 0x107fff, K052109_word_noA12_w },
	{ 0x140000, 0x140007, K051937_word_w },
	{ 0x140400, 0x1407ff, K051960_word_w },
/*	{ 0x10e800, 0x10e801, MWA16_NOP }, ???*/
#if 0
	{ 0x0c0000, 0x0c0001, tmnt_priority_w },
#endif
MEMORY_END

static MEMORY_READ16_START( tmnt_readmem )
	{ 0x000000, 0x05ffff, MRA16_ROM },
	{ 0x060000, 0x063fff, MRA16_RAM },	/* main RAM */
	{ 0x080000, 0x080fff, MRA16_RAM },
	{ 0x0a0000, 0x0a0001, input_port_0_word_r },
	{ 0x0a0002, 0x0a0003, input_port_1_word_r },
	{ 0x0a0004, 0x0a0005, input_port_2_word_r },
	{ 0x0a0006, 0x0a0007, input_port_3_word_r },
	{ 0x0a0010, 0x0a0011, input_port_4_word_r },
	{ 0x0a0012, 0x0a0013, input_port_5_word_r },
	{ 0x0a0014, 0x0a0015, input_port_6_word_r },
	{ 0x0a0018, 0x0a0019, input_port_7_word_r },
	{ 0x100000, 0x107fff, K052109_word_noA12_r },
	{ 0x140000, 0x140007, K051937_word_r },
	{ 0x140400, 0x1407ff, K051960_word_r },
MEMORY_END

static MEMORY_WRITE16_START( tmnt_writemem )
	{ 0x000000, 0x05ffff, MWA16_ROM },
	{ 0x060000, 0x063fff, MWA16_RAM },	/* main RAM */
	{ 0x080000, 0x080fff, tmnt_paletteram_word_w, &paletteram16 },
	{ 0x0a0000, 0x0a0001, tmnt_0a0000_w },
	{ 0x0a0008, 0x0a0009, tmnt_sound_command_w },
	{ 0x0a0010, 0x0a0011, watchdog_reset16_w },
	{ 0x0c0000, 0x0c0001, tmnt_priority_w },
	{ 0x100000, 0x107fff, K052109_word_noA12_w },
/*	{ 0x10e800, 0x10e801, MWA16_NOP }, ???*/
	{ 0x140000, 0x140007, K051937_word_w },
	{ 0x140400, 0x1407ff, K051960_word_w },
MEMORY_END

static MEMORY_READ16_START( punkshot_readmem )
	{ 0x000000, 0x03ffff, MRA16_ROM },
	{ 0x080000, 0x083fff, MRA16_RAM },	/* main RAM */
	{ 0x090000, 0x090fff, MRA16_RAM },
	{ 0x0a0000, 0x0a0001, input_port_0_word_r },
	{ 0x0a0002, 0x0a0003, input_port_1_word_r },
	{ 0x0a0004, 0x0a0005, input_port_3_word_r },
	{ 0x0a0006, 0x0a0007, input_port_2_word_r },
	{ 0x0a0040, 0x0a0043, punkshot_sound_r },	/* K053260 */
	{ 0x100000, 0x107fff, K052109_word_noA12_r },
	{ 0x110000, 0x110007, K051937_word_r },
	{ 0x110400, 0x1107ff, K051960_word_r },
	{ 0xfffffc, 0xffffff, punkshot_kludge_r },
MEMORY_END

static MEMORY_WRITE16_START( punkshot_writemem )
	{ 0x000000, 0x03ffff, MWA16_ROM },
	{ 0x080000, 0x083fff, MWA16_RAM },	/* main RAM */
	{ 0x090000, 0x090fff, paletteram16_xBBBBBGGGGGRRRRR_word_w, &paletteram16 },
	{ 0x0a0020, 0x0a0021, punkshot_0a0020_w },
	{ 0x0a0040, 0x0a0041, K053260_0_lsb_w },
	{ 0x0a0060, 0x0a007f, K053251_lsb_w },
	{ 0x0a0080, 0x0a0081, watchdog_reset16_w },
	{ 0x100000, 0x107fff, punkshot_K052109_word_noA12_w },
	{ 0x110000, 0x110007, K051937_word_w },
	{ 0x110400, 0x1107ff, K051960_word_w },
MEMORY_END

static MEMORY_READ16_START( lgtnfght_readmem )
	{ 0x000000, 0x03ffff, MRA16_ROM },
	{ 0x080000, 0x080fff, MRA16_RAM },
	{ 0x090000, 0x093fff, MRA16_RAM },	/* main RAM */
	{ 0x0a0000, 0x0a0001, input_port_0_word_r },
	{ 0x0a0002, 0x0a0003, input_port_1_word_r },
	{ 0x0a0004, 0x0a0005, input_port_2_word_r },
	{ 0x0a0006, 0x0a0007, input_port_3_word_r },
	{ 0x0a0008, 0x0a0009, input_port_4_word_r },
	{ 0x0a0010, 0x0a0011, input_port_5_word_r },
	{ 0x0a0020, 0x0a0023, punkshot_sound_r },	/* K053260 */
	{ 0x0b0000, 0x0b3fff, K053245_scattered_word_r },
	{ 0x0c0000, 0x0c001f, K053244_word_noA1_r },
	{ 0x100000, 0x107fff, K052109_word_noA12_r },
MEMORY_END

static MEMORY_WRITE16_START( lgtnfght_writemem )
	{ 0x000000, 0x03ffff, MWA16_ROM },
	{ 0x080000, 0x080fff, paletteram16_xBBBBBGGGGGRRRRR_word_w, &paletteram16 },
	{ 0x090000, 0x093fff, MWA16_RAM },	/* main RAM */
	{ 0x0a0018, 0x0a0019, lgtnfght_0a0018_w },
	{ 0x0a0020, 0x0a0021, K053260_0_lsb_w },
	{ 0x0a0028, 0x0a0029, watchdog_reset16_w },
	{ 0x0b0000, 0x0b3fff, K053245_scattered_word_w, &spriteram16 },
	{ 0x0c0000, 0x0c001f, K053244_word_noA1_w },
	{ 0x0e0000, 0x0e001f, K053251_lsb_w },
	{ 0x100000, 0x107fff, K052109_word_noA12_w },
MEMORY_END


static WRITE16_HANDLER( ssriders_soundkludge_w )
{
	/* I think this is more than just a trigger */
	cpu_set_irq_line_and_vector(1,0,HOLD_LINE,0xff);
}

static MEMORY_READ16_START( detatwin_readmem )
	{ 0x000000, 0x07ffff, MRA16_ROM },
	{ 0x180000, 0x183fff, K052109_word_r },
	{ 0x204000, 0x207fff, MRA16_RAM },	/* main RAM */
	{ 0x300000, 0x303fff, K053245_scattered_word_r },
	{ 0x400000, 0x400fff, MRA16_RAM },
	{ 0x500000, 0x50003f, K054000_lsb_r },
	{ 0x680000, 0x68001f, K053244_word_noA1_r },
	{ 0x700000, 0x700001, input_port_0_word_r },
	{ 0x700002, 0x700003, input_port_1_word_r },
	{ 0x700004, 0x700005, detatwin_coin_r },
	{ 0x700006, 0x700007, detatwin_eeprom_r },
	{ 0x780600, 0x780603, detatwin_sound_r },	/* K053260 */
MEMORY_END

static MEMORY_WRITE16_START( detatwin_writemem )
	{ 0x000000, 0x07ffff, MWA16_ROM },
	{ 0x180000, 0x183fff, K052109_word_w },
	{ 0x204000, 0x207fff, MWA16_RAM },	/* main RAM */
	{ 0x300000, 0x303fff, K053245_scattered_word_w, &spriteram16 },
	{ 0x500000, 0x50003f, K054000_lsb_w },
	{ 0x400000, 0x400fff, paletteram16_xBBBBBGGGGGRRRRR_word_w, &paletteram16 },
	{ 0x680000, 0x68001f, K053244_word_noA1_w },
	{ 0x700200, 0x700201, detatwin_eeprom_w },
	{ 0x700400, 0x700401, watchdog_reset16_w },
	{ 0x700300, 0x700301, detatwin_700300_w },
	{ 0x780600, 0x780601, K053260_0_lsb_w },
	{ 0x780604, 0x780605, ssriders_soundkludge_w },
	{ 0x780700, 0x78071f, K053251_lsb_w },
MEMORY_END

static MEMORY_READ16_START( glfgreat_readmem )
	{ 0x000000, 0x03ffff, MRA16_ROM },
	{ 0x100000, 0x103fff, MRA16_RAM },	/* main RAM */
	{ 0x104000, 0x107fff, K053245_scattered_word_r },
	{ 0x108000, 0x108fff, MRA16_RAM },
	{ 0x10c000, 0x10cfff, MRA16_RAM },	/* 053936? */
	{ 0x114000, 0x11401f, K053244_lsb_r },
	{ 0x120000, 0x120001, input_port_0_word_r },
	{ 0x120002, 0x120003, input_port_1_word_r },
	{ 0x120004, 0x120005, input_port_3_word_r },
	{ 0x120006, 0x120007, input_port_2_word_r },
	{ 0x121000, 0x121001, glfgreat_ball_r },	/* returns the color of the center pixel of the roz layer */
	{ 0x125000, 0x125003, glfgreat_sound_r },	/* K053260 */
	{ 0x200000, 0x207fff, K052109_word_noA12_r },
	{ 0x300000, 0x3fffff, glfgreat_rom_r },
MEMORY_END

static MEMORY_WRITE16_START( glfgreat_writemem )
	{ 0x000000, 0x03ffff, MWA16_ROM },
	{ 0x100000, 0x103fff, MWA16_RAM },	/* main RAM */
	{ 0x104000, 0x107fff, K053245_scattered_word_w, &spriteram16 },
	{ 0x108000, 0x108fff, paletteram16_xBBBBBGGGGGRRRRR_word_w, &paletteram16 },
	{ 0x10c000, 0x10cfff, MWA16_RAM, &K053936_0_linectrl },
	{ 0x110000, 0x11001f, K053244_word_noA1_w },	/* duplicate! */
	{ 0x114000, 0x11401f, K053244_lsb_w },			/* duplicate! */
	{ 0x118000, 0x11801f, MWA16_RAM, &K053936_0_ctrl },
	{ 0x11c000, 0x11c01f, K053251_msb_w },
	{ 0x122000, 0x122001, glfgreat_122000_w },
	{ 0x124000, 0x124001, watchdog_reset16_w },
	{ 0x125000, 0x125003, glfgreat_sound_w },	/* K053260 */
	{ 0x200000, 0x207fff, K052109_word_noA12_w },
MEMORY_END


static MEMORY_READ16_START( prmrsocr_readmem )
	{ 0x000000, 0x07ffff, MRA16_ROM },
	{ 0x100000, 0x103fff, MRA16_RAM },	/* main RAM */
	{ 0x104000, 0x107fff, K053245_scattered_word_r },
	{ 0x108000, 0x108fff, MRA16_RAM },
	{ 0x10c000, 0x10cfff, MRA16_RAM },
	{ 0x114000, 0x11401f, K053244_lsb_r },
	{ 0x120000, 0x120001, prmrsocr_IN0_r },
	{ 0x120002, 0x120003, prmrsocr_eeprom_r },
	{ 0x121014, 0x121015, prmrsocr_sound_r },
	{ 0x200000, 0x207fff, K052109_word_noA12_r },
MEMORY_END

static MEMORY_WRITE16_START( prmrsocr_writemem )
	{ 0x000000, 0x07ffff, MWA16_ROM },
	{ 0x100000, 0x103fff, MWA16_RAM },	/* main RAM */
	{ 0x104000, 0x107fff, K053245_scattered_word_w, &spriteram16 },
	{ 0x108000, 0x108fff, paletteram16_xBBBBBGGGGGRRRRR_word_w, &paletteram16 },
	{ 0x10c000, 0x10cfff, MWA16_RAM, &K053936_0_linectrl },
	{ 0x110000, 0x11001f, K053244_word_noA1_w },	/* duplicate! */
	{ 0x114000, 0x11401f, K053244_lsb_w },			/* duplicate! */
	{ 0x118000, 0x11801f, MWA16_RAM, &K053936_0_ctrl },
	{ 0x11c000, 0x11c01f, K053251_msb_w },
	{ 0x122000, 0x122001, prmrsocr_eeprom_w },	/* EEPROM + video control */
	{ 0x12100c, 0x12100f, prmrsocr_sound_cmd_w },
	{ 0x123000, 0x123001, prmrsocr_sound_irq_w },
	{ 0x200000, 0x207fff, K052109_word_noA12_w },
	{ 0x280000, 0x280001, watchdog_reset16_w },
MEMORY_END


static MEMORY_READ16_START( tmnt2_readmem ) /***/
	{ 0x000000, 0x0fffff, MRA16_ROM },
	{ 0x104000, 0x107fff, MRA16_RAM },	/* main RAM */
	{ 0x140000, 0x140fff, MRA16_RAM },
	{ 0x180000, 0x183fff, MRA16_RAM },	/* K053245_scattered_word_r*/
	{ 0x1c0000, 0x1c0001, input_port_0_word_r },
	{ 0x1c0002, 0x1c0003, input_port_1_word_r },
	{ 0x1c0004, 0x1c0005, input_port_4_word_r },
	{ 0x1c0006, 0x1c0007, input_port_5_word_r },
	{ 0x1c0100, 0x1c0101, input_port_2_word_r },
	{ 0x1c0102, 0x1c0103, ssriders_eeprom_r },
	{ 0x1c0400, 0x1c0401, watchdog_reset16_r },
	{ 0x1c0500, 0x1c057f, MRA16_RAM },	/* TMNT2 only (1J) unknown, mostly MCU blit offsets */
/*	{ 0x1c0800, 0x1c0801, ssriders_protection_r },  protection device */
	{ 0x5a0000, 0x5a001f, K053244_word_noA1_r },
	{ 0x5c0600, 0x5c0603, tmnt2_sound_r },	/* K053260 */
	{ 0x600000, 0x603fff, K052109_word_r },
MEMORY_END

static data16_t *tmnt2_1c0800,*sunset_104000;
static data16_t *tmnt2_rom;

#if 1 /***/
static INLINE UINT32 tmnt2_get_word(UINT32 addr)
{
	if (addr <= 0x07ffff/2) return(tmnt2_rom[addr]); else
	if (addr >= 0x104000/2 && addr <= 0x107fff/2) return(sunset_104000[addr-0x104000/2]); else
	if (addr >= 0x180000/2 && addr <= 0x183fff/2) return(spriteram16[addr-0x180000/2]);
	return(0);
}

static void tmnt2_put_word(UINT32 addr, UINT16 data)
{
	UINT32 offs;
	if (addr >= 0x180000/2 && addr <= 0x183fff/2)
	{
		spriteram16[addr-0x180000/2] = data;
		offs = addr-0x180000/2;
		if (!(offs & 0x0031))
		{
			offs = ((offs & 0x000e) >> 1) | ((offs & 0x1fc0) >> 3);
			K053245_word_w(offs, data, 0);
		}
	}
	else if (addr >= 0x104000/2 && addr <= 0x107fff/2) sunset_104000[addr-0x104000/2] = data;
}

WRITE16_HANDLER( tmnt2_1c0800_w )
{
	UINT32 src_addr, dst_addr, mod_addr, attr1, code, attr2, cbase, cmod, color;
	int xoffs, yoffs, xmod, ymod, zmod, xzoom, yzoom, i;
	UINT16 *mcu;
	UINT16 src[4], mod[24];
	UINT8 keepaspect, xlock, ylock, zlock;

	COMBINE_DATA(tmnt2_1c0800 + offset);

	if (offset != 0x18/2 || !ACCESSING_MSB) return;

	mcu = tmnt2_1c0800;
	if ((mcu[8] & 0xff00) != 0x8200) return;

	src_addr = (mcu[0] | (mcu[1]&0xff)<<16) >> 1;
	dst_addr = (mcu[2] | (mcu[3]&0xff)<<16) >> 1;
	mod_addr = (mcu[4] | (mcu[5]&0xff)<<16) >> 1;
	zlock    = (mcu[8] & 0xff) == 0x0001;

	for (i=0; i< 4; i++) src[i] = tmnt2_get_word(src_addr + i);
	for (i=0; i<24; i++) mod[i] = tmnt2_get_word(mod_addr + i);

	code = src[0];			/* code*/

	i= src[1];
	attr1 = i>>2 & 0x3f00;	/* flip y, flip x and sprite size*/
	attr2 = i & 0x380;		/* mirror y, mirror x, shadow*/
	cbase = i & 0x01f;		/* base color*/
	cmod  = mod[0x2a/2]>>8;
	color = (cbase != 0x0f && cmod <= 0x1f && !zlock) ? cmod : cbase;

	xoffs = (INT16)src[2];	/* local x*/
	yoffs = (INT16)src[3];	/* local y*/

	i = mod[0];
	attr2 |= i & 0x0060;	/* priority*/
	keepaspect = (i & 0x0014) == 0x0014;
	if (i & 0x8000) { attr1 |= 0x8000; }	/* active*/
	if (keepaspect)	{ attr1 |= 0x4000; }	/* keep aspect*/
/*	if (i & 0x????) { attr1 ^= 0x2000; yoffs = -yoffs; }	*/ /* flip y (not used?)*/
	if (i & 0x4000) { attr1 ^= 0x1000; xoffs = -xoffs; }	/* flip x*/

	xmod = (INT16)mod[6];	/* global x*/
	ymod = (INT16)mod[7];	/* global y*/
	zmod = (INT16)mod[8];	/* global z*/
	xzoom = mod[0x1c/2];
	yzoom = (keepaspect) ? xzoom : mod[0x1e/2];

	ylock = xlock = (i & 0x0020 && (!xzoom || xzoom == 0x100));

	/*
		Scale factor is non-linear. The zoom vales are looked-up from
		two to three nested tables and passed through a series of math
		operations. The MCU is suspected to have its own tables for
		translating zoom values to final scale factors or it knows where
		to fetch them in ROM. There is no access to its internal code so
		the scale curve is only approximated.

		The most accurate method is to trace how MCU zoom is transformed
		from ROM data, reverse the maths, plug the result into the sprite
		zoom code and derive the scale factor from there; but zooming
		would still suffer from precision loss in K053245_sprites_draw()
		and drawgfx() producing gaps in logical sprite groups.

		A few sample points on the real curve:

		 Zoom | Scale factor
		------+--------------
		 0    | 0.0
		 0x2c | 0x40/0x8d
		 0x2f | 0x40/0x80
		 0x4f | 1.0
		 0x60 | 0x40/0x2f
		 0x7b | 0x40/0x14
	*/
	if (!xlock)
	{
		i = xzoom - 0x4f00;
		if (i > 0)
		{
			i >>= 8;
			xoffs += (int)(pow(i, /*1.898461*/1.891292) * xoffs / 599.250121);
		}
		else if (i < 0)
		{
			i = (i>>3) + (i>>4) + (i>>5) + (i>>6) + xzoom;
			xoffs = (i > 0) ? (xoffs * i / 0x4f00) : 0;
		}
	}
	if (!ylock)
	{
		i = yzoom - 0x4f00;
		if (i > 0)
		{
			i >>= 8;
			yoffs += (int)(pow(i, /*1.898461*/1.891292) * yoffs / 599.250121);
		}
		else if (i < 0)
		{
			i = (i>>3) + (i>>4) + (i>>5) + (i>>6) + yzoom;
			yoffs = (i > 0) ? (yoffs * i / 0x4f00) : 0;
		}

	}
	if (!zlock) yoffs += zmod;
	xoffs += xmod;
	yoffs += ymod;

	tmnt2_put_word(dst_addr +  0, attr1);
	tmnt2_put_word(dst_addr +  2, code);
	tmnt2_put_word(dst_addr +  4, (UINT32)yoffs);
	tmnt2_put_word(dst_addr +  6, (UINT32)xoffs);
	tmnt2_put_word(dst_addr + 12, attr2 | color);
}
#else /* for reference; do not remove*/
WRITE16_HANDLER( tmnt2_1c0800_w )
{
    COMBINE_DATA( tmnt2_1c0800 + offset);
    if ( offset == 0x0008 && ( tmnt2_1c0800[0x8] & 0xff00 ) == 0x8200 )
	{
		unsigned int CellSrc;
		unsigned int CellVar;
		data16_t *src;
		int dst;
		int x,y;

		CellVar = tmnt2_1c0800[0x04] | (tmnt2_1c0800[0x05] << 16 );
		dst = tmnt2_1c0800[0x02] | (tmnt2_1c0800[0x03] << 16 );
		CellSrc = tmnt2_1c0800[0x00] | (tmnt2_1c0800[0x01] << 16 );
/*        if ( CellDest >= 0x180000 && CellDest < 0x183fe0 ) {*/
        CellVar -= 0x104000;
		src = (data16_t *)(memory_region(REGION_CPU1) + CellSrc);

		CellVar >>= 1;

		cpu_writemem24bew_word(dst+0x00, 0x8000 | ((src[1] & 0xfc00) >> 2));	/* size, flip xy */
        cpu_writemem24bew_word(dst+0x04, src[0]);	/* code */
        cpu_writemem24bew_word(dst+0x18, (src[1] & 0x3ff) ^		/* color, mirror, priority */
				(sunset_104000[CellVar + 0x00] & 0x0060));

		/* base color modifier */
		/* TODO: this is wrong, e.g. it breaks the explosions when you kill an */
		/* enemy, or surfs in the sewer level (must be blue for all enemies). */
		/* It fixes the enemies, though, they are not all purple when you throw them around. */
		/* Also, the bosses don't blink when they are about to die - don't know */
		/* if this is correct or not. */
/*		if (sunset_104000[CellVar + 0x15] & 0x001f)*/
/*			cpu_writemem24bew_word(dst+0x18,(cpu_readmem24bew_word(dst+0x18) & 0xffe0) |*/
/*					(sunset_104000[CellVar + 0x15] & 0x001f));*/

		x = src[2];
		if (sunset_104000[CellVar + 0x00] & 0x4000)
		{
			/* flip x */
			cpu_writemem24bew_word(dst+0x00,cpu_readmem24bew_word(dst+0x00) ^ 0x1000);
			x = -x;
		}
		x += sunset_104000[CellVar + 0x06];
		cpu_writemem24bew_word(dst+0x0c,x);
		y = src[3];
		y += sunset_104000[CellVar + 0x07];
		/* don't do second offset for shadows */
		if ((tmnt2_1c0800[0x08] & 0x00ff) != 0x01)
			y += sunset_104000[CellVar + 0x08];
		cpu_writemem24bew_word(dst+0x08,y);
#if 0
log_cb(RETRO_LOG_DEBUG, LOGPRE "copy command %04x sprite %08x data %08x: %04x%04x %04x%04x  modifiers %08x:%04x%04x %04x%04x %04x%04x %04x%04x %04x%04x %04x%04x %04x%04x %04x%04x %04x%04x %04x%04x %04x%04x %04x%04x\n",
	tmnt2_1c0800[0x05],
	CellDest,CellSrc,
	src[0], src[1], src[2], src[3],
	CellVar*2,
	sunset_104000[CellVar + 0x00],
	sunset_104000[CellVar + 0x01],
	sunset_104000[CellVar + 0x02],
	sunset_104000[CellVar + 0x03],
	sunset_104000[CellVar + 0x04],
	sunset_104000[CellVar + 0x05],
	sunset_104000[CellVar + 0x06],
	sunset_104000[CellVar + 0x07],
	sunset_104000[CellVar + 0x08],
	sunset_104000[CellVar + 0x09],
	sunset_104000[CellVar + 0x0a],
	sunset_104000[CellVar + 0x0b],
	sunset_104000[CellVar + 0x0c],
	sunset_104000[CellVar + 0x0d],
	sunset_104000[CellVar + 0x0e],
	sunset_104000[CellVar + 0x0f],
	sunset_104000[CellVar + 0x10],
	sunset_104000[CellVar + 0x11],
	sunset_104000[CellVar + 0x12],
	sunset_104000[CellVar + 0x13],
	sunset_104000[CellVar + 0x14],
	sunset_104000[CellVar + 0x15],
	sunset_104000[CellVar + 0x16],
	sunset_104000[CellVar + 0x17]
	);
#endif
/*        }*/
    }
}
#endif

static MEMORY_WRITE16_START( tmnt2_writemem ) /***/
	{ 0x000000, 0x0fffff, MWA16_ROM, &tmnt2_rom },
	{ 0x104000, 0x107fff, MWA16_RAM, &sunset_104000 },	/* main RAM */
	{ 0x140000, 0x140fff, paletteram16_xBBBBBGGGGGRRRRR_word_w, &paletteram16 },
	{ 0x180000, 0x183fff, K053245_scattered_word_w, &spriteram16 },
	{ 0x1c0200, 0x1c0201, ssriders_eeprom_w },	/* EEPROM and gfx control */
	{ 0x1c0300, 0x1c0301, ssriders_1c0300_w },
	{ 0x1c0400, 0x1c0401, watchdog_reset16_w },
	{ 0x1c0500, 0x1c057f, MWA16_RAM },	/* unknown: TMNT2 only (1J), mostly MCU blit offsets */
	{ 0x1c0800, 0x1c081f, tmnt2_1c0800_w, &tmnt2_1c0800 },	/* protection device */
	{ 0x5a0000, 0x5a001f, K053244_word_noA1_w },
	{ 0x5c0600, 0x5c0601, K053260_0_lsb_w },
	{ 0x5c0604, 0x5c0605, ssriders_soundkludge_w },
	{ 0x5c0700, 0x5c071f, K053251_lsb_w },
	{ 0x600000, 0x603fff, K052109_word_w },
MEMORY_END


static MEMORY_READ16_START( ssriders_readmem )
	{ 0x000000, 0x0bffff, MRA16_ROM },
	{ 0x104000, 0x107fff, MRA16_RAM },	/* main RAM */
	{ 0x140000, 0x140fff, MRA16_RAM },
	{ 0x180000, 0x183fff, K053245_scattered_word_r },
	{ 0x1c0000, 0x1c0001, input_port_0_word_r },
	{ 0x1c0002, 0x1c0003, input_port_1_word_r },
	{ 0x1c0004, 0x1c0005, input_port_4_word_r },
	{ 0x1c0006, 0x1c0007, input_port_5_word_r },
	{ 0x1c0100, 0x1c0101, input_port_2_word_r },
	{ 0x1c0102, 0x1c0103, ssriders_eeprom_r },
	{ 0x1c0400, 0x1c0401, watchdog_reset16_r },
	{ 0x1c0500, 0x1c057f, MRA16_RAM },	/* TMNT2 only (1J) unknown */
	{ 0x1c0800, 0x1c0801, ssriders_protection_r },	/* protection device */
	{ 0x5a0000, 0x5a001f, K053244_word_noA1_r },
	{ 0x5c0600, 0x5c0603, punkshot_sound_r },	/* K053260 */
	{ 0x600000, 0x603fff, K052109_word_r },
MEMORY_END

static MEMORY_WRITE16_START( ssriders_writemem )
	{ 0x000000, 0x0bffff, MWA16_ROM },
	{ 0x104000, 0x107fff, MWA16_RAM },	/* main RAM */
	{ 0x140000, 0x140fff, paletteram16_xBBBBBGGGGGRRRRR_word_w, &paletteram16 },
	{ 0x180000, 0x183fff, K053245_scattered_word_w, &spriteram16 },
	{ 0x1c0200, 0x1c0201, ssriders_eeprom_w },	/* EEPROM and gfx control */
	{ 0x1c0300, 0x1c0301, ssriders_1c0300_w },
	{ 0x1c0400, 0x1c0401, watchdog_reset16_w },
	{ 0x1c0500, 0x1c057f, MWA16_RAM },	/* TMNT2 only (1J) unknown */
	{ 0x1c0800, 0x1c0803, ssriders_protection_w },	/* protection device */
	{ 0x5a0000, 0x5a001f, K053244_word_noA1_w },
	{ 0x5c0600, 0x5c0601, K053260_0_lsb_w },
	{ 0x5c0604, 0x5c0605, ssriders_soundkludge_w },
	{ 0x5c0700, 0x5c071f, K053251_lsb_w },
	{ 0x600000, 0x603fff, K052109_word_w },
MEMORY_END

static MEMORY_READ16_START( ssridersbl_readmem )
	{ 0x000000, 0x0bffff, MRA16_ROM },
	{ 0x104000, 0x107fff, MRA16_RAM },	/* main RAM */
	{ 0x14c000, 0x14cfff, MRA16_RAM },
	{ 0x180000, 0x183fff, K053245_scattered_word_r },
	{ 0x184000, 0x18ffff, MRA16_RAM },
	{ 0x5a0000, 0x5a001f, K053244_word_noA1_r },
	{ 0x600000, 0x603fff, K052109_word_r },
	{ 0xc00000, 0xc00001, input_port_0_word_r },
	{ 0xc00002, 0xc00003, input_port_1_word_r },
	{ 0xc00004, 0xc00005, input_port_4_word_r },
	{ 0xc00006, 0xc00007, input_port_5_word_r },
	{ 0xc00404, 0xc00405, input_port_2_word_r },
	{ 0xc00406, 0xc00407, ssridersbl_eeprom_r },
	{ 0xc00600, 0xc00601, OKIM6295_status_0_lsb_r },
	{ 0x75d288, 0x75d289, MRA16_NOP },	/* read repeatedly in some test menus (PC=181f2)*/
MEMORY_END

static MEMORY_WRITE16_START( ssridersbl_writemem )
	{ 0x000000, 0x0bffff, MWA16_ROM },
	{ 0x104000, 0x107fff, MWA16_RAM },	/* main RAM */
	{ 0x14c000, 0x14cfff, paletteram16_xBBBBBGGGGGRRRRR_word_w, &paletteram16 },
	{ 0x14e700, 0x14e71f, K053251_lsb_w },
	{ 0x180000, 0x183fff, K053245_scattered_word_w, &spriteram16 },
	{ 0x184000, 0x18ffff, MWA16_RAM },
	{ 0x1c0300, 0x1c0301, ssriders_1c0300_w },
	{ 0x1c0400, 0x1c0401, MWA16_NOP },
	{ 0x5a0000, 0x5a001f, K053244_word_noA1_w },
	{ 0x600000, 0x603fff, K052109_word_w },
	{ 0x604020, 0x60402f, MWA16_NOP },	/* written every frame */
	{ 0x604200, 0x604201, MWA16_NOP },	/* watchdog */
	{ 0x6119e2, 0x6119e3, MWA16_NOP },	/* written a lot in some test menus (PC=18204) */
	{ 0xc00200, 0xc00201, ssriders_eeprom_w },	/* EEPROM and gfx control */
	{ 0xc00600, 0xc00601, OKIM6295_data_0_lsb_w },
MEMORY_END

static MEMORY_READ16_START( thndrx2_readmem )
	{ 0x000000, 0x03ffff, MRA16_ROM },
	{ 0x100000, 0x103fff, MRA16_RAM },	/* main RAM */
	{ 0x200000, 0x200fff, MRA16_RAM },
	{ 0x400000, 0x400003, punkshot_sound_r },	/* K053260 */
	{ 0x500000, 0x50003f, K054000_lsb_r },
	{ 0x500200, 0x500201, thndrx2_in0_r },
	{ 0x500202, 0x500203, thndrx2_eeprom_r },
	{ 0x600000, 0x607fff, K052109_word_noA12_r },
	{ 0x700000, 0x700007, K051937_word_r },
	{ 0x700400, 0x7007ff, K051960_word_r },
MEMORY_END

static MEMORY_WRITE16_START( thndrx2_writemem )
	{ 0x000000, 0x03ffff, MWA16_ROM },
	{ 0x100000, 0x103fff, MWA16_RAM },	/* main RAM */
	{ 0x200000, 0x200fff, paletteram16_xBBBBBGGGGGRRRRR_word_w, &paletteram16 },
	{ 0x300000, 0x30001f, K053251_lsb_w },
	{ 0x400000, 0x400001, K053260_0_lsb_w },
	{ 0x500000, 0x50003f, K054000_lsb_w },
	{ 0x500100, 0x500101, thndrx2_eeprom_w },
	{ 0x500300, 0x500301, MWA16_NOP },	/* watchdog reset? irq enable? */
	{ 0x600000, 0x607fff, K052109_word_noA12_w },
	{ 0x700000, 0x700007, K051937_word_w },
	{ 0x700400, 0x7007ff, K051960_word_w },
MEMORY_END



static MEMORY_READ_START( mia_s_readmem )
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0x87ff, MRA_RAM },
	{ 0xa000, 0xa000, soundlatch_r },
	{ 0xb000, 0xb00d, K007232_read_port_0_r },
	{ 0xc001, 0xc001, YM2151_status_port_0_r },
MEMORY_END

static MEMORY_WRITE_START( mia_s_writemem )
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0x8000, 0x87ff, MWA_RAM },
	{ 0xb000, 0xb00d, K007232_write_port_0_w },
	{ 0xc000, 0xc000, YM2151_register_port_0_w },
	{ 0xc001, 0xc001, YM2151_data_port_0_w },
MEMORY_END

static MEMORY_READ_START( tmnt_s_readmem )
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0x87ff, MRA_RAM },
	{ 0x9000, 0x9000, tmnt_sres_r },	/* title music & UPD7759C reset */
	{ 0xa000, 0xa000, soundlatch_r },
	{ 0xb000, 0xb00d, K007232_read_port_0_r },
	{ 0xc001, 0xc001, YM2151_status_port_0_r },
	{ 0xf000, 0xf000, upd7759_0_busy_r },
MEMORY_END

static MEMORY_WRITE_START( tmnt_s_writemem )
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0x8000, 0x87ff, MWA_RAM },
	{ 0x9000, 0x9000, tmnt_sres_w },	/* title music & UPD7759C reset */
	{ 0xb000, 0xb00d, K007232_write_port_0_w  },
	{ 0xc000, 0xc000, YM2151_register_port_0_w },
	{ 0xc001, 0xc001, YM2151_data_port_0_w },
	{ 0xd000, 0xd000, upd7759_0_port_w },
	{ 0xe000, 0xe000, upd7759_0_start_w },
MEMORY_END

static MEMORY_READ_START( punkshot_s_readmem )
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0xf000, 0xf7ff, MRA_RAM },
	{ 0xf801, 0xf801, YM2151_status_port_0_r },
	{ 0xfc00, 0xfc2f, K053260_0_r },
MEMORY_END

static MEMORY_WRITE_START( punkshot_s_writemem )
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0xf000, 0xf7ff, MWA_RAM },
	{ 0xf800, 0xf800, YM2151_register_port_0_w },
	{ 0xf801, 0xf801, YM2151_data_port_0_w },
	{ 0xfa00, 0xfa00, sound_arm_nmi_w },
	{ 0xfc00, 0xfc2f, K053260_0_w },
MEMORY_END

static MEMORY_READ_START( lgtnfght_s_readmem )
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0x87ff, MRA_RAM },
	{ 0xa001, 0xa001, YM2151_status_port_0_r },
	{ 0xc000, 0xc02f, K053260_0_r },
MEMORY_END

static MEMORY_WRITE_START( lgtnfght_s_writemem )
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0x8000, 0x87ff, MWA_RAM },
	{ 0xa000, 0xa000, YM2151_register_port_0_w },
	{ 0xa001, 0xa001, YM2151_data_port_0_w },
	{ 0xc000, 0xc02f, K053260_0_w },
MEMORY_END

static MEMORY_READ_START( glfgreat_s_readmem )
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0xf000, 0xf7ff, MRA_RAM },
	{ 0xf800, 0xf82f, K053260_0_r },
MEMORY_END

static MEMORY_WRITE_START( glfgreat_s_writemem )
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0xf000, 0xf7ff, MWA_RAM },
	{ 0xf800, 0xf82f, K053260_0_w },
	{ 0xfa00, 0xfa00, sound_arm_nmi_w },
MEMORY_END

static MEMORY_READ_START( ssriders_s_readmem )
	{ 0x0000, 0xefff, MRA_ROM },
	{ 0xf000, 0xf7ff, MRA_RAM },
	{ 0xf801, 0xf801, YM2151_status_port_0_r },
	{ 0xfa00, 0xfa2f, K053260_0_r },
MEMORY_END

static MEMORY_WRITE_START( ssriders_s_writemem )
	{ 0x0000, 0xefff, MWA_ROM },
	{ 0xf000, 0xf7ff, MWA_RAM },
	{ 0xf800, 0xf800, YM2151_register_port_0_w },
	{ 0xf801, 0xf801, YM2151_data_port_0_w },
	{ 0xfa00, 0xfa2f, K053260_0_w },
	{ 0xfc00, 0xfc00, sound_arm_nmi_w },
MEMORY_END

static MEMORY_READ_START( thndrx2_s_readmem )
	{ 0x0000, 0xefff, MRA_ROM },
	{ 0xf000, 0xf7ff, MRA_RAM },
	{ 0xf801, 0xf801, YM2151_status_port_0_r },
	{ 0xfc00, 0xfc2f, K053260_0_r },
MEMORY_END

static MEMORY_WRITE_START( thndrx2_s_writemem )
	{ 0x0000, 0xefff, MWA_ROM },
	{ 0xf000, 0xf7ff, MWA_RAM },
	{ 0xf800, 0xf800, YM2151_register_port_0_w },
	{ 0xf801, 0xf801, YM2151_data_port_0_w },
	{ 0xf811, 0xf811, YM2151_data_port_0_w },	/* mirror */
	{ 0xfa00, 0xfa00, sound_arm_nmi_w },
	{ 0xfc00, 0xfc2f, K053260_0_w },
MEMORY_END

static READ_HANDLER( K054539_0_ctrl_r )
{
	return K054539_0_r(0x200+offset);
}
static WRITE_HANDLER( K054539_0_ctrl_w )
{
	K054539_0_w(0x200+offset,data);
}

static MEMORY_READ_START( prmrsocr_s_readmem )
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0xbfff, MRA_BANK1 },
	{ 0xc000, 0xdfff, MRA_RAM },
	{ 0xe000, 0xe0ff, K054539_0_r },
	{ 0xe100, 0xe12f, K054539_0_ctrl_r },
	{ 0xf002, 0xf002, soundlatch_r },
	{ 0xf003, 0xf003, soundlatch2_r },
MEMORY_END

static MEMORY_WRITE_START( prmrsocr_s_writemem )
	{ 0x0000, 0xbfff, MWA_ROM },
	{ 0xc000, 0xdfff, MWA_RAM },
	{ 0xe000, 0xe0ff, K054539_0_w },
	{ 0xe100, 0xe12f, K054539_0_ctrl_w },
	{ 0xf000, 0xf000, soundlatch3_w },
	{ 0xf800, 0xf800, prmrsocr_s_bankswitch_w },
MEMORY_END



#define KONAMI_PLAYERS_INPUT_LSB( player, button3, start ) \
	PORT_BIT(  0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | player ) \
	PORT_BIT(  0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | player ) \
	PORT_BIT(  0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | player ) \
	PORT_BIT(  0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | player ) \
	PORT_BIT(  0x0010, IP_ACTIVE_LOW, IPT_BUTTON1        | player ) \
	PORT_BIT(  0x0020, IP_ACTIVE_LOW, IPT_BUTTON2        | player ) \
	PORT_BIT(  0x0040, IP_ACTIVE_LOW, button3            | player ) \
	PORT_BIT(  0x0080, IP_ACTIVE_LOW, start )

#define KONAMI_PLAYERS_INPUT_MSB( player, button3, start ) \
	PORT_BIT(  0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | player ) \
	PORT_BIT(  0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | player ) \
	PORT_BIT(  0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | player ) \
	PORT_BIT(  0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | player ) \
	PORT_BIT(  0x1000, IP_ACTIVE_LOW, IPT_BUTTON1        | player ) \
	PORT_BIT(  0x2000, IP_ACTIVE_LOW, IPT_BUTTON2        | player ) \
	PORT_BIT(  0x4000, IP_ACTIVE_LOW, button3            | player ) \
	PORT_BIT(  0x8000, IP_ACTIVE_LOW, start )


INPUT_PORTS_START( mia )
	PORT_START      /* COINS */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START      /* PLAYER 1 */
	KONAMI_PLAYERS_INPUT_LSB( IPF_PLAYER1, IPT_BUTTON3, IPT_UNUSED )

	PORT_START      /* PLAYER 2 */
	KONAMI_PLAYERS_INPUT_LSB( IPF_PLAYER2, IPT_BUTTON3, IPT_UNUSED )

	PORT_START	/* DSW1 */
	PORT_DIPNAME( 0x0f, 0x0f, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x05, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 3C_2C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 4C_3C ) )
	PORT_DIPSETTING(    0x0f, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 3C_4C ) )
	PORT_DIPSETTING(    0x07, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x0e, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 2C_5C ) )
	PORT_DIPSETTING(    0x0d, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x0b, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0x0a, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(    0x09, DEF_STR( 1C_7C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0xf0, 0xf0, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x50, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 3C_2C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 4C_3C ) )
	PORT_DIPSETTING(    0xf0, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 3C_4C ) )
	PORT_DIPSETTING(    0x70, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0xe0, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x60, DEF_STR( 2C_5C ) )
	PORT_DIPSETTING(    0xd0, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0xb0, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0xa0, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(    0x90, DEF_STR( 1C_7C ) )
/*	PORT_DIPSETTING(    0x00, "Invalid" )*/

	PORT_START	/* DSW2 */
	PORT_DIPNAME( 0x03, 0x02, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x03, "2" )
	PORT_DIPSETTING(    0x02, "3" )
	PORT_DIPSETTING(    0x01, "5" )
	PORT_DIPSETTING(    0x00, "7" )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_DIPNAME( 0x18, 0x10, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x18, "30K 80K" )
	PORT_DIPSETTING(    0x10, "50K 100K" )
	PORT_DIPSETTING(    0x08, "50K" )
	PORT_DIPSETTING(    0x00, "100K" )
	PORT_DIPNAME( 0x60, 0x40, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x60, "Easy" )
	PORT_DIPSETTING(    0x40, "Normal" )
	PORT_DIPSETTING(    0x20, "Difficult" )
	PORT_DIPSETTING(    0x00, "Very Difficult" )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START	/* DSW3 */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, "VRAM Character Check" )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_SERVICE( 0x04, IP_ACTIVE_LOW )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNUSED )
INPUT_PORTS_END

INPUT_PORTS_START( tmnt )
	PORT_START      /* COINS */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN4 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_SERVICE2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_SERVICE3 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_SERVICE4 )

	PORT_START      /* PLAYER 1 */
	KONAMI_PLAYERS_INPUT_LSB( IPF_PLAYER1, IPT_UNKNOWN, IPT_UNKNOWN )

	PORT_START      /* PLAYER 2 */
	KONAMI_PLAYERS_INPUT_LSB( IPF_PLAYER2, IPT_UNKNOWN, IPT_UNKNOWN )

	PORT_START      /* PLAYER 3 */
	KONAMI_PLAYERS_INPUT_LSB( IPF_PLAYER3, IPT_UNKNOWN, IPT_UNKNOWN )

	PORT_START	/* DSW1 */
	PORT_DIPNAME( 0x0f, 0x0f, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x05, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 3C_2C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 4C_3C ) )
	PORT_DIPSETTING(    0x0f, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 3C_4C ) )
	PORT_DIPSETTING(    0x07, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x0e, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 2C_5C ) )
	PORT_DIPSETTING(    0x0d, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x0b, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0x0a, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(    0x09, DEF_STR( 1C_7C ) )

	PORT_START	/* DSW2 */
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x03, "1" )
	PORT_DIPSETTING(    0x02, "2" )
	PORT_DIPSETTING(    0x01, "3" )
	PORT_DIPSETTING(    0x00, "5" )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x60, 0x40, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x60, "Easy" )
	PORT_DIPSETTING(    0x40, "Normal" )
	PORT_DIPSETTING(    0x20, "Difficult" )
	PORT_DIPSETTING(    0x00, "Very Difficult" )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START      /* PLAYER 4 */
	KONAMI_PLAYERS_INPUT_LSB( IPF_PLAYER4, IPT_UNKNOWN, IPT_UNKNOWN )

	PORT_START	/* DSW3 */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_SERVICE( 0x04, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_BIT( 0xf0, IP_ACTIVE_LOW, IPT_UNUSED )
INPUT_PORTS_END

INPUT_PORTS_START( tmnt2p )
	PORT_START      /* COINS */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_SERVICE2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* PLAYER 1 */
	KONAMI_PLAYERS_INPUT_LSB( IPF_PLAYER1, IPT_UNKNOWN, IPT_START1 )

	PORT_START      /* PLAYER 2 */
	KONAMI_PLAYERS_INPUT_LSB( IPF_PLAYER2, IPT_UNKNOWN, IPT_START2 )

	PORT_START      /* PLAYER 3 */
/*	KONAMI_PLAYERS_INPUT_LSB( IPF_PLAYER3, IPT_UNKNOWN, IPT_START3 )*/

	PORT_START	/* DSW1 */
	PORT_DIPNAME( 0x0f, 0x0f, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x05, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 3C_2C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 4C_3C ) )
	PORT_DIPSETTING(    0x0f, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 3C_4C ) )
	PORT_DIPSETTING(    0x07, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x0e, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 2C_5C ) )
	PORT_DIPSETTING(    0x0d, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x0b, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0x0a, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(    0x09, DEF_STR( 1C_7C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0xf0, 0xf0, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x50, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 3C_2C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 4C_3C ) )
	PORT_DIPSETTING(    0xf0, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 3C_4C ) )
	PORT_DIPSETTING(    0x70, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0xe0, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x60, DEF_STR( 2C_5C ) )
	PORT_DIPSETTING(    0xd0, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0xb0, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0xa0, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(    0x90, DEF_STR( 1C_7C ) )
/*	PORT_DIPSETTING(    0x00, "Invalid" )*/

	PORT_START	/* DSW2 */
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x03, "1" )
	PORT_DIPSETTING(    0x02, "2" )
	PORT_DIPSETTING(    0x01, "3" )
	PORT_DIPSETTING(    0x00, "5" )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x60, 0x40, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x60, "Easy" )
	PORT_DIPSETTING(    0x40, "Normal" )
	PORT_DIPSETTING(    0x20, "Difficult" )
	PORT_DIPSETTING(    0x00, "Very Difficult" )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START      /* PLAYER 4 */
/*	KONAMI_PLAYERS_INPUT_LSB( IPF_PLAYER4, IPT_UNKNOWN, IPT_START4 )*/

	PORT_START	/* DSW3 */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_SERVICE( 0x04, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_BIT( 0xf0, IP_ACTIVE_LOW, IPT_UNUSED )
INPUT_PORTS_END

INPUT_PORTS_START( punkshot )
	PORT_START	/* DSW1/DSW2 */
	PORT_DIPNAME( 0x000f, 0x000f, DEF_STR( Coinage ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(      0x0005, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0008, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0004, DEF_STR( 3C_2C ) )
	PORT_DIPSETTING(      0x0001, DEF_STR( 4C_3C ) )
	PORT_DIPSETTING(      0x000f, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0003, DEF_STR( 3C_4C ) )
	PORT_DIPSETTING(      0x0007, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(      0x000e, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x0006, DEF_STR( 2C_5C ) )
	PORT_DIPSETTING(      0x000d, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x000c, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(      0x000b, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(      0x000a, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(      0x0009, DEF_STR( 1C_7C ) )
	PORT_DIPNAME( 0x0010, 0x0010, "Continue" )
	PORT_DIPSETTING(      0x0010, "Normal" )
	PORT_DIPSETTING(      0x0000, "1 Coin" )
	PORT_DIPNAME( 0x0020, 0x0020, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0040, 0x0040, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0040, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0080, 0x0080, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0080, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0300, 0x0300, "Energy" )
	PORT_DIPSETTING(      0x0300, "30" )
	PORT_DIPSETTING(      0x0200, "40" )
	PORT_DIPSETTING(      0x0100, "50" )
	PORT_DIPSETTING(      0x0000, "60" )
	PORT_DIPNAME( 0x0c00, 0x0c00, "Period Length" )
	PORT_DIPSETTING(      0x0c00, "2 Minutes" )
	PORT_DIPSETTING(      0x0800, "3 Minutes" )
	PORT_DIPSETTING(      0x0400, "4 Minutes" )
	PORT_DIPSETTING(      0x0000, "5 Minutes" )
	PORT_DIPNAME( 0x1000, 0x1000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x1000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x6000, 0x6000, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(      0x6000, "Easy" )
	PORT_DIPSETTING(      0x4000, "Medium" )
	PORT_DIPSETTING(      0x2000, "Hard" )
	PORT_DIPSETTING(      0x0000, "Hardest" )
	PORT_DIPNAME( 0x8000, 0x0000, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(      0x8000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )

	PORT_START	/* COIN/DSW3 */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_COIN4 )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_SERVICE2 )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_SERVICE3 )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_SERVICE4 )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_DIPNAME( 0x1000, 0x1000, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(      0x1000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x2000, 0x2000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x2000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_SERVICE( 0x4000, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x8000, 0x8000, "Freeze" )
	PORT_DIPSETTING(      0x8000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )

	PORT_START	/* IN0/IN1 */
	KONAMI_PLAYERS_INPUT_LSB( IPF_PLAYER1, IPT_UNKNOWN, IPT_UNKNOWN )
	KONAMI_PLAYERS_INPUT_MSB( IPF_PLAYER2, IPT_UNKNOWN, IPT_UNKNOWN )

	PORT_START	/* IN2/IN3 */
	KONAMI_PLAYERS_INPUT_LSB( IPF_PLAYER3, IPT_UNKNOWN, IPT_UNKNOWN )
	KONAMI_PLAYERS_INPUT_MSB( IPF_PLAYER4, IPT_UNKNOWN, IPT_UNKNOWN )
INPUT_PORTS_END

INPUT_PORTS_START( punksht2 )
	PORT_START	/* DSW1/DSW2 */
	PORT_DIPNAME( 0x000f, 0x000f, DEF_STR( Coinage ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(      0x0005, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0008, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0004, DEF_STR( 3C_2C ) )
	PORT_DIPSETTING(      0x0001, DEF_STR( 4C_3C ) )
	PORT_DIPSETTING(      0x000f, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0003, DEF_STR( 3C_4C ) )
	PORT_DIPSETTING(      0x0007, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(      0x000e, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x0006, DEF_STR( 2C_5C ) )
	PORT_DIPSETTING(      0x000d, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x000c, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(      0x000b, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(      0x000a, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(      0x0009, DEF_STR( 1C_7C ) )
	PORT_DIPNAME( 0x0010, 0x0010, "Continue" )
	PORT_DIPSETTING(      0x0010, "Normal" )
	PORT_DIPSETTING(      0x0000, "1 Coin" )
	PORT_DIPNAME( 0x0020, 0x0020, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0040, 0x0040, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0040, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0080, 0x0080, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0080, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0300, 0x0300, "Energy" )
	PORT_DIPSETTING(      0x0300, "40" )
	PORT_DIPSETTING(      0x0200, "50" )
	PORT_DIPSETTING(      0x0100, "60" )
	PORT_DIPSETTING(      0x0000, "70" )
	PORT_DIPNAME( 0x0c00, 0x0c00, "Period Length" )
	PORT_DIPSETTING(      0x0c00, "3 Minutes" )
	PORT_DIPSETTING(      0x0800, "4 Minutes" )
	PORT_DIPSETTING(      0x0400, "5 Minutes" )
	PORT_DIPSETTING(      0x0000, "6 Minutes" )
	PORT_DIPNAME( 0x1000, 0x1000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x1000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x6000, 0x6000, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(      0x6000, "Easy" )
	PORT_DIPSETTING(      0x4000, "Medium" )
	PORT_DIPSETTING(      0x2000, "Hard" )
	PORT_DIPSETTING(      0x0000, "Hardest" )
	PORT_DIPNAME( 0x8000, 0x0000, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(      0x8000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )

	PORT_START	/* COIN/DSW3 */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_DIPNAME( 0x1000, 0x1000, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(      0x1000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x2000, 0x2000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x2000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_SERVICE( 0x4000, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x8000, 0x8000, "Freeze" )
	PORT_DIPSETTING(      0x8000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )

	PORT_START	/* IN0/IN1 */
	KONAMI_PLAYERS_INPUT_LSB( IPF_PLAYER1, IPT_UNKNOWN, IPT_UNKNOWN )
	KONAMI_PLAYERS_INPUT_MSB( IPF_PLAYER2, IPT_UNKNOWN, IPT_UNKNOWN )
INPUT_PORTS_END

INPUT_PORTS_START( lgtnfght )
	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNKNOWN )	/* vblank? checked during boot */
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	KONAMI_PLAYERS_INPUT_LSB( IPF_PLAYER1, IPT_BUTTON3, IPT_UNKNOWN )

	PORT_START
	KONAMI_PLAYERS_INPUT_LSB( IPF_PLAYER2, IPT_BUTTON3, IPT_UNKNOWN )

	PORT_START	/* DSW1 */
	PORT_DIPNAME( 0x03, 0x02, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x03, "2" )
	PORT_DIPSETTING(    0x02, "3" )
	PORT_DIPSETTING(    0x01, "5" )
	PORT_DIPSETTING(    0x00, "7" )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x18, 0x18, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x18, "100000 400000" )
	PORT_DIPSETTING(    0x10, "150000 500000" )
	PORT_DIPSETTING(    0x08, "200000" )
	PORT_DIPSETTING(    0x00, "None" )
	PORT_DIPNAME( 0x60, 0x40, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x60, "Easy" )
	PORT_DIPSETTING(    0x40, "Medium" )
	PORT_DIPSETTING(    0x20, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START	/* DSW2 */
	PORT_DIPNAME( 0x0f, 0x0f, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x05, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 3C_2C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 4C_3C ) )
	PORT_DIPSETTING(    0x0f, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 3C_4C ) )
	PORT_DIPSETTING(    0x07, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x0e, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 2C_5C ) )
	PORT_DIPSETTING(    0x0d, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x0b, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0x0a, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(    0x09, DEF_STR( 1C_7C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0xf0, 0xf0, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x50, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 3C_2C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 4C_3C ) )
	PORT_DIPSETTING(    0xf0, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 3C_4C ) )
	PORT_DIPSETTING(    0x70, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0xe0, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x60, DEF_STR( 2C_5C ) )
	PORT_DIPSETTING(    0xd0, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0xb0, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0xa0, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(    0x90, DEF_STR( 1C_7C ) )
/*	PORT_DIPSETTING(    0x00, "Invalid" )*/

	PORT_START	/* DSW3 */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x00, "Sound" )
	PORT_DIPSETTING(    0x02, "Mono" )
	PORT_DIPSETTING(    0x00, "Stereo" )
	PORT_SERVICE( 0x04, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_BIT( 0xf0, IP_ACTIVE_LOW, IPT_UNUSED )
INPUT_PORTS_END

INPUT_PORTS_START( detatwin )
	PORT_START	/* IN0 */
	KONAMI_PLAYERS_INPUT_LSB( IPF_PLAYER1, IPT_UNKNOWN, IPT_UNKNOWN )

	PORT_START	/* IN1 */
	KONAMI_PLAYERS_INPUT_LSB( IPF_PLAYER2, IPT_UNKNOWN, IPT_UNKNOWN )

	PORT_START	/* COIN */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_SERVICE, DEF_STR( Service_Mode ), KEYCODE_F2, IP_JOY_NONE )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* VBLANK? OBJMPX? */
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* EEPROM */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_SPECIAL )	/* EEPROM data */
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_SPECIAL )	/* EEPROM status? - always 1 */
	PORT_BIT( 0xfc, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END

INPUT_PORTS_START( glfgreat )
	PORT_START	/* IN0 */
	KONAMI_PLAYERS_INPUT_LSB( IPF_PLAYER1, IPT_BUTTON3, IPT_BUTTON4 | IPF_PLAYER1 )
	KONAMI_PLAYERS_INPUT_MSB( IPF_PLAYER2, IPT_BUTTON3, IPT_BUTTON4 | IPF_PLAYER2 )

	PORT_START	/* IN1 */
	KONAMI_PLAYERS_INPUT_LSB( IPF_PLAYER3, IPT_BUTTON3, IPT_BUTTON4 | IPF_PLAYER3 )
	KONAMI_PLAYERS_INPUT_MSB( IPF_PLAYER4, IPT_BUTTON3, IPT_BUTTON4 | IPF_PLAYER4 )

	PORT_START
	PORT_DIPNAME( 0x000f, 0x000f, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(      0x0005, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0008, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0004, DEF_STR( 3C_2C ) )
	PORT_DIPSETTING(      0x0001, DEF_STR( 4C_3C ) )
	PORT_DIPSETTING(      0x000f, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0003, DEF_STR( 3C_4C ) )
	PORT_DIPSETTING(      0x0007, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(      0x000e, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x0006, DEF_STR( 2C_5C ) )
	PORT_DIPSETTING(      0x000d, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x000c, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(      0x000b, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(      0x000a, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(      0x0009, DEF_STR( 1C_7C ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0x00f0, 0x00f0, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(      0x0050, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0080, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0040, DEF_STR( 3C_2C ) )
	PORT_DIPSETTING(      0x0010, DEF_STR( 4C_3C ) )
	PORT_DIPSETTING(      0x00f0, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0030, DEF_STR( 3C_4C ) )
	PORT_DIPSETTING(      0x0070, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(      0x00e0, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x0060, DEF_STR( 2C_5C ) )
	PORT_DIPSETTING(      0x00d0, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x00c0, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(      0x00b0, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(      0x00a0, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(      0x0090, DEF_STR( 1C_7C ) )
/*	PORT_DIPSETTING(      0x0000, "Invalid" )*/
	PORT_DIPNAME( 0x0300, 0x0100, "Players/Controllers" )
	PORT_DIPSETTING(      0x0300, "4/1" )
	PORT_DIPSETTING(      0x0200, "4/2" )
	PORT_DIPSETTING(      0x0100, "4/4" )
	PORT_DIPSETTING(      0x0000, "3/3" )
	PORT_DIPNAME( 0x0400, 0x0000, "Sound" )
	PORT_DIPSETTING(      0x0400, "Mono" )
	PORT_DIPSETTING(      0x0000, "Stereo" )
	PORT_DIPNAME( 0x1800, 0x1800, "Initial/Maximum Credit" )
	PORT_DIPSETTING(      0x1800, "2/3" )
	PORT_DIPSETTING(      0x1000, "2/4" )
	PORT_DIPSETTING(      0x0800, "2/5" )
	PORT_DIPSETTING(      0x0000, "3/5" )
	PORT_DIPNAME( 0x6000, 0x4000, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(      0x6000, "Easy" )
	PORT_DIPSETTING(      0x4000, "Normal" )
	PORT_DIPSETTING(      0x2000, "Hard" )
	PORT_DIPSETTING(      0x0000, "Hardest" )
	PORT_DIPNAME( 0x8000, 0x0000, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(      0x8000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )

	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_COIN4 )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_SERVICE1 )	/* service coin */
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BITX(0x0400, IP_ACTIVE_LOW, IPT_SERVICE, DEF_STR( Service_Mode ), KEYCODE_F2, IP_JOY_NONE )
	PORT_DIPNAME( 0x8000, 0x0000, "Freeze" )	/* ?? VBLANK ?? */
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x8000, DEF_STR( On ) )
	PORT_DIPNAME( 0x1000, 0x1000, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(      0x1000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x2000, 0x2000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x2000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
/*	PORT_SERVICE( 0x4000, IP_ACTIVE_LOW )*/
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_SERVICE )
	PORT_DIPNAME( 0x8000, 0x8000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x8000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
INPUT_PORTS_END

INPUT_PORTS_START( ssriders )
	PORT_START	/* IN0 */
	KONAMI_PLAYERS_INPUT_LSB( IPF_PLAYER1, IPT_UNKNOWN, IPT_START1 )

	PORT_START	/* IN1 */
	KONAMI_PLAYERS_INPUT_LSB( IPF_PLAYER2, IPT_UNKNOWN, IPT_START2 )

	PORT_START	/* COIN */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_SERVICE2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* EEPROM and service */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_SPECIAL )	/* EEPROM data */
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_SPECIAL )	/* EEPROM status? - always 1 */
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* ?? TMNT2: OBJMPX */
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_VBLANK )	/* ?? TMNT2: NVBLK */
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* ?? TMNT2: IPL0 */
	PORT_BIT( 0x60, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* unused? */
	PORT_BITX(0x80, IP_ACTIVE_LOW, IPT_SERVICE, DEF_STR( Service_Mode ), KEYCODE_F2, IP_JOY_NONE )
INPUT_PORTS_END

INPUT_PORTS_START( ssridr4p )
	PORT_START	/* IN0 */
	KONAMI_PLAYERS_INPUT_LSB( IPF_PLAYER1, IPT_UNKNOWN, IPT_UNKNOWN )

	PORT_START	/* IN1 */
	KONAMI_PLAYERS_INPUT_LSB( IPF_PLAYER2, IPT_UNKNOWN, IPT_UNKNOWN )

	PORT_START	/* COIN */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN4 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_SERVICE2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_SERVICE3 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_SERVICE4 )

	PORT_START	/* EEPROM and service */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_SPECIAL )	/* EEPROM data */
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_SPECIAL )	/* EEPROM status? - always 1 */
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* ?? TMNT2: OBJMPX */
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_VBLANK )	/* ?? TMNT2: NVBLK */
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* ?? TMNT2: IPL0 */
	PORT_BIT( 0x60, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* unused? */
	PORT_BITX(0x80, IP_ACTIVE_LOW, IPT_SERVICE, DEF_STR( Service_Mode ), KEYCODE_F2, IP_JOY_NONE )

	PORT_START	/* IN2 */
	KONAMI_PLAYERS_INPUT_LSB( IPF_PLAYER3, IPT_UNKNOWN, IPT_UNKNOWN )

	PORT_START	/* IN3 */
	KONAMI_PLAYERS_INPUT_LSB( IPF_PLAYER4, IPT_UNKNOWN, IPT_UNKNOWN )
INPUT_PORTS_END

/* Same as 'ssridr4p', but additional Start button for each player.
   COIN3, COIN4, SERVICE3 and SERVICE4 only have an effect in the "test mode". */
INPUT_PORTS_START( ssrid4ps )
	PORT_START	/* IN0 */
	KONAMI_PLAYERS_INPUT_LSB( IPF_PLAYER1, IPT_UNKNOWN, IPT_START1 )

	PORT_START	/* IN1 */
	KONAMI_PLAYERS_INPUT_LSB( IPF_PLAYER2, IPT_UNKNOWN, IPT_START2 )

	PORT_START	/* COIN */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN4 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_SERVICE2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_SERVICE3 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_SERVICE4 )

	PORT_START	/* EEPROM and service */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_SPECIAL )	/* EEPROM data */
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_SPECIAL )	/* EEPROM status? - always 1 */
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* ?? TMNT2: OBJMPX */
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_VBLANK )	/* ?? TMNT2: NVBLK */
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* ?? TMNT2: IPL0 */
	PORT_BIT( 0x60, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* unused? */
	PORT_BITX(0x80, IP_ACTIVE_LOW, IPT_SERVICE, DEF_STR( Service_Mode ), KEYCODE_F2, IP_JOY_NONE )

	PORT_START	/* IN2 */
	KONAMI_PLAYERS_INPUT_LSB( IPF_PLAYER3, IPT_UNKNOWN, IPT_START3 )

	PORT_START	/* IN3 */
	KONAMI_PLAYERS_INPUT_LSB( IPF_PLAYER4, IPT_UNKNOWN, IPT_START4 )
INPUT_PORTS_END

/* Version for the bootleg, which has the service switch a little different */
INPUT_PORTS_START( ssridbl )
	PORT_START	/* IN0 */
	KONAMI_PLAYERS_INPUT_LSB( IPF_PLAYER1, IPT_UNKNOWN, IPT_START1 )

	PORT_START	/* IN1 */
	KONAMI_PLAYERS_INPUT_LSB( IPF_PLAYER2, IPT_UNKNOWN, IPT_START2 )

	PORT_START	/* COIN */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN4 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_SERVICE2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_SERVICE3 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_SERVICE4 )

	PORT_START	/* EEPROM and service */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_SPECIAL )	/* EEPROM data */
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_SPECIAL )	/* EEPROM status? - always 1 */
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_SERVICE, DEF_STR( Service_Mode ), KEYCODE_F2, IP_JOY_NONE )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x60, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* unused? */
	PORT_BITX(0x80, IP_ACTIVE_LOW, IPT_SERVICE, DEF_STR( Service_Mode ), KEYCODE_F2, IP_JOY_NONE )

	PORT_START	/* IN2 */
	KONAMI_PLAYERS_INPUT_LSB( IPF_PLAYER3, IPT_UNKNOWN, IPT_START3 )

	PORT_START	/* IN3 */
	KONAMI_PLAYERS_INPUT_LSB( IPF_PLAYER4, IPT_UNKNOWN, IPT_START4 )
INPUT_PORTS_END

INPUT_PORTS_START( qgakumon )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER1 )	/* Joystick control : Left*/
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER1 )	/* Joystick control : Right*/
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )	/* Joystick control : Up*/
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )	/* Joystick control : Down*/
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON5 | IPF_PLAYER1 )	/* Joystick control : Button*/
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )	/* Joystick control : Left*/
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER2 )	/* Joystick control : Right*/
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )	/* Joystick control : Up*/
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )	/* Joystick control : Down*/
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON5 | IPF_PLAYER2 )	/* Joystick control : Button*/
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START2 )

	PORT_START	/* COIN */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* EEPROM and service */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_SPECIAL )	/* EEPROM data */
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_SPECIAL )	/* EEPROM status? - always 1 */
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* ?? TMNT2: OBJMPX */
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_VBLANK )	/* ?? TMNT2: NVBLK (needs to be ACTIVE_HIGH to avoid problems) */
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* ?? TMNT2: IPL0 */
	PORT_BIT( 0x60, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* unused? */
	PORT_BITX(0x80, IP_ACTIVE_LOW, IPT_SERVICE, DEF_STR( Service_Mode ), KEYCODE_F2, IP_JOY_NONE )
INPUT_PORTS_END

INPUT_PORTS_START( thndrx2 )
	PORT_START
	KONAMI_PLAYERS_INPUT_LSB( IPF_PLAYER1, IPT_UNKNOWN, IPT_START1 )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BITX(0x0800, IP_ACTIVE_LOW, IPT_SERVICE, DEF_STR( Service_Mode ), KEYCODE_F2, IP_JOY_NONE )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* EEPROM and service */
	KONAMI_PLAYERS_INPUT_LSB( IPF_PLAYER2, IPT_UNKNOWN, IPT_START2 )
	PORT_BIT( 0x0100, IP_ACTIVE_HIGH, IPT_SPECIAL )	/* EEPROM data */
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_SPECIAL )	/* EEPROM status? - always 1 */
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* VBLK?? */
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END

INPUT_PORTS_START( prmrsocr )
	PORT_START
	KONAMI_PLAYERS_INPUT_LSB( IPF_PLAYER1, IPT_UNKNOWN, IPT_START1 )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BITX(0x0200, IP_ACTIVE_LOW, IPT_SERVICE, DEF_STR( Service_Mode ), KEYCODE_F2, IP_JOY_NONE )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_DIPNAME( 0x1000, 0x0000, "Sound" )
	PORT_DIPSETTING(      0x1000, "Mono" )
	PORT_DIPSETTING(      0x0000, "Stereo" )
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
	KONAMI_PLAYERS_INPUT_LSB( IPF_PLAYER2, IPT_UNKNOWN, IPT_START2 )
	PORT_BIT( 0x0100, IP_ACTIVE_HIGH, IPT_SPECIAL )	/* EEPROM data */
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_SPECIAL )	/* EEPROM status? - always 1 */
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END


static void cbj_irq_handler(int state)
{
	cbj_snd_irqlatch = state;
}

static struct YM2151interface ym2151_interface_cbj =
{
	1,			/* 1 chip */
	3579545,	/* 3.579545 MHz */
	{ YM3012_VOL(100,MIXER_PAN_LEFT,100,MIXER_PAN_RIGHT) },
	{ cbj_irq_handler }
};

static struct YM2151interface ym2151_interface =
{
	1,			/* 1 chip */
	3579545,	/* 3.579545 MHz */
	{ YM3012_VOL(60,MIXER_PAN_LEFT,60,MIXER_PAN_RIGHT) },
	{ 0 }
};

static void volume_callback(int v)
{
	K007232_set_volume(0,0,(v >> 4) * 0x11,0);
	K007232_set_volume(0,1,0,(v & 0x0f) * 0x11);
}

static struct K007232_interface k007232_interface =
{
	1,		/* number of chips */
	3579545,	/* clock */
	{ REGION_SOUND1 },	/* memory regions */
	{ K007232_VOL(30,MIXER_PAN_CENTER,30,MIXER_PAN_CENTER) },	/* volume */
	{ volume_callback }	/* external port callback */
};

static struct upd7759_interface upd7759_interface =
{
	1,		/* number of chips */
	{ UPD7759_STANDARD_CLOCK },
	{ 60 }, /* volume */
	{ REGION_SOUND2 },		/* memory region */
	{0}
};

static struct Samplesinterface samples_interface =
{
	1,	/* 1 channel for the title music */
	100	/* volume */
};

static struct CustomSound_interface custom_interface =
{
	tmnt_decode_sample,
	0,
	0
};

static struct K053260_interface k053260_interface_nmi =
{
	1,
	{ 3579545 },
	{ REGION_SOUND1 }, /* memory region */
	{ { MIXER(70,MIXER_PAN_LEFT), MIXER(70,MIXER_PAN_RIGHT) } },
/*	{ sound_nmi_callback }, */
};

static struct K053260_interface dtk053260_interface_nmi =
{
	1,
	{ 3579545 },
	{ REGION_SOUND1 }, /* memory region */
	{ { MIXER(75,MIXER_PAN_LEFT), MIXER(75,MIXER_PAN_RIGHT) } },
/*	{ sound_nmi_callback }, */
};

static struct K053260_interface k053260_interface =
{
	1,
	{ 3579545 },
	{ REGION_SOUND1 }, /* memory region */
	{ { MIXER(70,MIXER_PAN_LEFT), MIXER(70,MIXER_PAN_RIGHT) } },
	{ 0 }
};

static struct K053260_interface glfgreat_k053260_interface =
{
	1,
	{ 3579545 },
	{ REGION_SOUND1 }, /* memory region */
	{ { MIXER(100,MIXER_PAN_LEFT), MIXER(100,MIXER_PAN_RIGHT) } },
/*	{ sound_nmi_callback },*/
};

static struct OKIM6295interface okim6295_interface =
{
	1,			/* 1 chip */
	{ 8000 },
	{ REGION_SOUND1 },
	{ 100 }
};

static MACHINE_DRIVER_START( cuebrckj )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 8000000)	/* 8 MHz */
	MDRV_CPU_MEMORY(cuebrckj_readmem,cuebrckj_writemem)
	MDRV_CPU_VBLANK_INT(cbj_interrupt,10)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_HAS_SHADOWS | VIDEO_HAS_HIGHLIGHTS | VIDEO_UPDATE_AFTER_VBLANK)
	MDRV_SCREEN_SIZE(64*8, 32*8)
	MDRV_VISIBLE_AREA(13*8, (64-13)*8-1, 2*8, 30*8-1 )
	MDRV_PALETTE_LENGTH(1024)
	MDRV_NVRAM_HANDLER(generic_0fill)

	MDRV_VIDEO_START(cuebrckj)
	MDRV_VIDEO_UPDATE(mia)

	/* sound hardware */
	MDRV_SOUND_ADD(YM2151, ym2151_interface_cbj)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( mia )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 8000000)	/* 8 MHz */
	MDRV_CPU_MEMORY(mia_readmem,mia_writemem)
	MDRV_CPU_VBLANK_INT(irq5_line_hold,1)

	MDRV_CPU_ADD(Z80, 3579545)
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)	/* 3.579545 MHz */
	MDRV_CPU_MEMORY(mia_s_readmem,mia_s_writemem)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_HAS_SHADOWS | VIDEO_HAS_HIGHLIGHTS | VIDEO_UPDATE_AFTER_VBLANK)
	MDRV_SCREEN_SIZE(64*8, 32*8)
	MDRV_VISIBLE_AREA(13*8, (64-13)*8-1, 2*8, 30*8-1 )
	MDRV_PALETTE_LENGTH(1024)

	MDRV_VIDEO_START(mia)
	MDRV_VIDEO_UPDATE(mia)

	/* sound hardware */
	MDRV_SOUND_ADD(YM2151, ym2151_interface)
	MDRV_SOUND_ADD(K007232, k007232_interface)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( tmnt )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 8000000)	/* 8 MHz */
	MDRV_CPU_MEMORY(tmnt_readmem,tmnt_writemem)
	MDRV_CPU_VBLANK_INT(irq5_line_hold,1)

	MDRV_CPU_ADD(Z80, 3579545)
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)	/* 3.579545 MHz */
	MDRV_CPU_MEMORY(tmnt_s_readmem,tmnt_s_writemem)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_HAS_SHADOWS | VIDEO_HAS_HIGHLIGHTS | VIDEO_UPDATE_AFTER_VBLANK)
	MDRV_SCREEN_SIZE(64*8, 32*8)
	MDRV_VISIBLE_AREA(13*8, (64-13)*8-1, 2*8, 30*8-1 )
	MDRV_PALETTE_LENGTH(1024)

	MDRV_VIDEO_START(tmnt)
	MDRV_VIDEO_UPDATE(tmnt)

	/* sound hardware */
	MDRV_SOUND_ADD(YM2151, ym2151_interface)
	MDRV_SOUND_ADD(K007232, k007232_interface)
	MDRV_SOUND_ADD(UPD7759, upd7759_interface)
	MDRV_SOUND_ADD(SAMPLES, samples_interface)
	MDRV_SOUND_ADD(CUSTOM, custom_interface)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( punkshot )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 12000000)	/* CPU is 68000/12, but this doesn't necessarily mean it's */
									/* running at 12MHz. TMNT uses 8MHz */
	MDRV_CPU_MEMORY(punkshot_readmem,punkshot_writemem)
	MDRV_CPU_VBLANK_INT(punkshot_interrupt,1)

	MDRV_CPU_ADD(Z80, 3579545)
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)	/* 3.579545 MHz */
	MDRV_CPU_MEMORY(punkshot_s_readmem,punkshot_s_writemem)
								/* NMIs are generated by the 053260 */
	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_HAS_SHADOWS | VIDEO_HAS_HIGHLIGHTS | VIDEO_UPDATE_AFTER_VBLANK)
	MDRV_SCREEN_SIZE(64*8, 32*8)
	MDRV_VISIBLE_AREA(14*8, (64-14)*8-1, 2*8, 30*8-1 )
	MDRV_PALETTE_LENGTH(2048)

	MDRV_VIDEO_START(punkshot)
	MDRV_VIDEO_UPDATE(punkshot)

	/* sound hardware */
	MDRV_SOUND_ADD(YM2151, ym2151_interface)
	MDRV_SOUND_ADD(K053260, k053260_interface_nmi)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( lgtnfght )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 12000000)	/* 12 MHz */
	MDRV_CPU_MEMORY(lgtnfght_readmem,lgtnfght_writemem)
	MDRV_CPU_VBLANK_INT(lgtnfght_interrupt,1)

	MDRV_CPU_ADD(Z80, 3579545)
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)	/* 3.579545 MHz */
	MDRV_CPU_MEMORY(lgtnfght_s_readmem,lgtnfght_s_writemem)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_HAS_SHADOWS | VIDEO_HAS_HIGHLIGHTS | VIDEO_UPDATE_AFTER_VBLANK)
	MDRV_SCREEN_SIZE(64*8, 32*8)
	MDRV_VISIBLE_AREA(14*8, (64-14)*8-1, 2*8, 30*8-1 )
	MDRV_PALETTE_LENGTH(2048)

	MDRV_VIDEO_START(lgtnfght)
	MDRV_VIDEO_UPDATE(lgtnfght)

	/* sound hardware */
	MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
	MDRV_SOUND_ADD(YM2151, ym2151_interface)
	MDRV_SOUND_ADD(K053260, k053260_interface)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( detatwin )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 16000000)	/* 16 MHz */
	MDRV_CPU_MEMORY(detatwin_readmem,detatwin_writemem)
	MDRV_CPU_VBLANK_INT(punkshot_interrupt,1)

	MDRV_CPU_ADD(Z80, 3579545)
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)	/* ????? */
	MDRV_CPU_MEMORY(ssriders_s_readmem,ssriders_s_writemem)
								/* NMIs are generated by the 053260 */
	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)

	MDRV_NVRAM_HANDLER(eeprom)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_HAS_SHADOWS | VIDEO_HAS_HIGHLIGHTS | VIDEO_UPDATE_AFTER_VBLANK)
	MDRV_SCREEN_SIZE(64*8, 32*8)
	MDRV_VISIBLE_AREA(14*8, (64-14)*8-1, 2*8, 30*8-1 )
	MDRV_PALETTE_LENGTH(2048)

	MDRV_VIDEO_START(detatwin)
	MDRV_VIDEO_UPDATE(lgtnfght)
	MDRV_VIDEO_EOF( detatwin ) /***/

	/* sound hardware */
	MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
	MDRV_SOUND_ADD(YM2151, ym2151_interface)
	MDRV_SOUND_ADD(K053260, dtk053260_interface_nmi)
MACHINE_DRIVER_END



static struct GfxLayout zoomlayout =
{
	16,16,
	RGN_FRAC(1,1),
	4,
	{ 0, 1, 2, 3 },
	{ 1*4, 0*4, 3*4, 2*4, 5*4, 4*4, 7*4, 6*4,
			9*4, 8*4, 11*4, 10*4, 13*4, 12*4, 15*4, 14*4 },
	{ 0*64, 1*64, 2*64, 3*64, 4*64, 5*64, 6*64, 7*64,
			8*64, 9*64, 10*64, 11*64, 12*64, 13*64, 14*64, 15*64, },
	16*64
};
static struct GfxDecodeInfo glfgreat_gfxdecodeinfo[] =
{
	{ REGION_GFX3, 0, &zoomlayout, 0x400, 16 },
	{ -1 }
};

static MACHINE_DRIVER_START( glfgreat )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 12000000)	/* ? */
	MDRV_CPU_MEMORY(glfgreat_readmem,glfgreat_writemem)
	MDRV_CPU_VBLANK_INT(lgtnfght_interrupt,1)

	MDRV_CPU_ADD(Z80, 3579545)
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)	/* ? */
	MDRV_CPU_MEMORY(glfgreat_s_readmem,glfgreat_s_writemem)
								/* NMIs are generated by the 053260 */
	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_HAS_SHADOWS | VIDEO_HAS_HIGHLIGHTS | VIDEO_UPDATE_AFTER_VBLANK)
	MDRV_SCREEN_SIZE(64*8, 32*8)
	MDRV_VISIBLE_AREA(14*8, (64-14)*8-1, 2*8, 30*8-1 )
	MDRV_GFXDECODE(glfgreat_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(2048)

	MDRV_VIDEO_START(glfgreat)
	MDRV_VIDEO_UPDATE(glfgreat)

	/* sound hardware */
	MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
	MDRV_SOUND_ADD(K053260, glfgreat_k053260_interface)
MACHINE_DRIVER_END


static void sound_nmi(void)
{
	cpu_set_nmi_line(1, PULSE_LINE);
}

static struct K054539interface k054539_interface =
{
	1,			/* 1 chip */
	48000,
	{ REGION_SOUND1 },
	{ { 100, 100 } },
	{ 0 },
	{ sound_nmi }
};

static MACHINE_DRIVER_START( prmrsocr )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 12000000)	/* ? */
	MDRV_CPU_MEMORY(prmrsocr_readmem,prmrsocr_writemem)
	MDRV_CPU_VBLANK_INT(lgtnfght_interrupt,1)

	MDRV_CPU_ADD(Z80, 8000000)
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)	/* ? */
	MDRV_CPU_MEMORY(prmrsocr_s_readmem,prmrsocr_s_writemem)
								/* NMIs are generated by the 054539 */
	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)

	MDRV_NVRAM_HANDLER(thndrx2)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_HAS_SHADOWS | VIDEO_HAS_HIGHLIGHTS | VIDEO_UPDATE_AFTER_VBLANK)
	MDRV_SCREEN_SIZE(64*8, 32*8)
	MDRV_VISIBLE_AREA(14*8, (64-14)*8-1, 2*8, 30*8-1 )
	MDRV_GFXDECODE(glfgreat_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(2048)

	MDRV_VIDEO_START(prmrsocr)
	MDRV_VIDEO_UPDATE(glfgreat)

	/* sound hardware */
	MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
	MDRV_SOUND_ADD(K054539, k054539_interface)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( tmnt2 ) /***/

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 16000000)	/* 16 MHz */
	MDRV_CPU_MEMORY(tmnt2_readmem,tmnt2_writemem)
	MDRV_CPU_VBLANK_INT(punkshot_interrupt,1)

	MDRV_CPU_ADD(Z80, 8000000)
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)	/* 8 MHz; clock is correct, but there's 1 cycle wait for ROM/RAM */
						/* access. Access speed of ROM/RAM used on the machine is 150ns, */
						/* without the wait, they cannot run on 8MHz.                    */
						/* We are not emulating the wait state, so the ROM test ends at  */
						/* 02 instead of 00. */
	MDRV_CPU_MEMORY(ssriders_s_readmem,ssriders_s_writemem)
								/* NMIs are generated by the 053260 */
	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)

	MDRV_NVRAM_HANDLER(eeprom)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_HAS_SHADOWS | VIDEO_HAS_HIGHLIGHTS | VIDEO_UPDATE_AFTER_VBLANK)
	MDRV_SCREEN_SIZE(64*8, 32*8)
	MDRV_VISIBLE_AREA(13*8, (64-13)*8-1, 2*8, 30*8-1 )
	MDRV_PALETTE_LENGTH(2048)

	MDRV_VIDEO_START(lgtnfght)
	MDRV_VIDEO_UPDATE(tmnt2)

	/* sound hardware */
	MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
	MDRV_SOUND_ADD(YM2151, ym2151_interface)
	MDRV_SOUND_ADD(K053260, k053260_interface_nmi)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( ssriders )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 16000000)	/* 16 MHz */
	MDRV_CPU_MEMORY(ssriders_readmem,ssriders_writemem)
	MDRV_CPU_VBLANK_INT(punkshot_interrupt,1)

	MDRV_CPU_ADD(Z80, 4000000)
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)	/* ????? makes the ROM test sync */
	MDRV_CPU_MEMORY(ssriders_s_readmem,ssriders_s_writemem)
								/* NMIs are generated by the 053260 */
	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)

	MDRV_NVRAM_HANDLER(eeprom)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_HAS_SHADOWS | VIDEO_HAS_HIGHLIGHTS | VIDEO_UPDATE_AFTER_VBLANK)
	MDRV_SCREEN_SIZE(64*8, 32*8)
	MDRV_VISIBLE_AREA(14*8, (64-14)*8-1, 2*8, 30*8-1 )
	MDRV_PALETTE_LENGTH(2048)

	MDRV_VIDEO_START(lgtnfght)
	MDRV_VIDEO_UPDATE(tmnt2)

	/* sound hardware */
	MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
	MDRV_SOUND_ADD(YM2151, ym2151_interface)
	MDRV_SOUND_ADD(K053260, dtk053260_interface_nmi)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( ssridersbl )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 16000000)	/* 16 MHz */
	MDRV_CPU_MEMORY(ssridersbl_readmem,ssridersbl_writemem)
	MDRV_CPU_VBLANK_INT(irq4_line_hold,1)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)

	MDRV_NVRAM_HANDLER(eeprom)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_HAS_SHADOWS | VIDEO_HAS_HIGHLIGHTS | VIDEO_UPDATE_AFTER_VBLANK)
	MDRV_SCREEN_SIZE(64*8, 32*8)
	MDRV_VISIBLE_AREA(14*8, (64-14)*8-1, 2*8, 30*8-1 )
	MDRV_PALETTE_LENGTH(2048)

	MDRV_VIDEO_START(sunsetbl)
	MDRV_VIDEO_UPDATE(tmnt2)

	/* sound hardware */
	MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
	MDRV_SOUND_ADD(OKIM6295, okim6295_interface)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( thndrx2 )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 12000000)	/* 12 MHz */
	MDRV_CPU_MEMORY(thndrx2_readmem,thndrx2_writemem)
	MDRV_CPU_VBLANK_INT(punkshot_interrupt,1)

	MDRV_CPU_ADD(Z80, 3579545)
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)	/* ????? */
	MDRV_CPU_MEMORY(thndrx2_s_readmem,thndrx2_s_writemem)
								/* NMIs are generated by the 053260 */
	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)

	MDRV_NVRAM_HANDLER(thndrx2)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_HAS_SHADOWS | VIDEO_HAS_HIGHLIGHTS)
	MDRV_SCREEN_SIZE(64*8, 32*8)
	MDRV_VISIBLE_AREA(14*8, (64-14)*8-1, 2*8, 30*8-1 )
	MDRV_PALETTE_LENGTH(2048)

	MDRV_VIDEO_START(thndrx2)
	MDRV_VIDEO_UPDATE(thndrx2)

	/* sound hardware */
	MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
	MDRV_SOUND_ADD(YM2151, ym2151_interface)
	MDRV_SOUND_ADD(K053260, k053260_interface_nmi)
MACHINE_DRIVER_END



/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( cuebrckj )
	ROM_REGION( 0x40000, REGION_CPU1, 0 )	/* 2*128k and 2*64k for 68000 code */
	ROM_LOAD16_BYTE( "903d25.g12",   0x00000, 0x10000, CRC(8d575663) SHA1(0e308e04936efa80351bf808ac304d3fcc82f19a) )
	ROM_LOAD16_BYTE( "903d24.f12",   0x00001, 0x10000, CRC(2973625d) SHA1(e2496704390930761204624d4bf6b0b68d3133ab) )

	ROM_REGION( 0x40000, REGION_GFX1, 0 ) /* graphics (addressable by the main CPU) */
	ROM_LOAD16_BYTE( "903c29.k21",  0x000000, 0x10000, CRC(fada986d) SHA1(79d13dcee5433457c25a8cca0093bddd55165a72) )
	ROM_LOAD16_BYTE( "903c27.k17",  0x000001, 0x10000, CRC(5bd4b8e1) SHA1(0bc5e508af20e479c7913fab1ef158165fe67079) )
	ROM_LOAD16_BYTE( "903c28.k19",  0x020000, 0x10000, CRC(80d2bfaf) SHA1(3b38558d4f17309154457e9e7780a25577d1858d) )
	ROM_LOAD16_BYTE( "903c26.k15",  0x020001, 0x10000, CRC(f808fa3d) SHA1(2b0fa1581acc5c4f7055e6faad97664ef16cc082) )

	ROM_REGION( 0x40000, REGION_GFX2, 0 )	/* graphics (addressable by the main CPU) */
	ROM_LOAD16_BYTE( "903d23.k12",  0x000000, 0x10000, CRC(c39fc9fd) SHA1(fe5a63e5d898f985f9ab9be5b701af4a8e2a9049) )        /* 8x8 tiles */
	ROM_LOAD16_BYTE( "903d21.k8",   0x000001, 0x10000, CRC(3c7bf8cd) SHA1(c487e0109f56b3b0e2aa2c4db2dfb30ad74fb0ab) )        /* 8x8 tiles */
	ROM_LOAD16_BYTE( "903d22.k10",  0x020000, 0x10000, CRC(95ad8591) SHA1(4e3c8c794be1cd78044eb0eebfa3c755e2aaf54f) )        /* 8x8 tiles */
	ROM_LOAD16_BYTE( "903d20.k6",   0x020001, 0x10000, CRC(2872a1bb) SHA1(da7c7a41860283eac49facaa3beb712d3be7db56) )        /* 8x8 tiles */
ROM_END

ROM_START( mia )
	ROM_REGION( 0x40000, REGION_CPU1, 0 )	/* 2*128k and 2*64k for 68000 code */
	ROM_LOAD16_BYTE( "808t20.h17",   0x00000, 0x20000, CRC(6f0acb1d) SHA1(af3447fd4645cb03b1660df2ae076fa53ff81945) )
	ROM_LOAD16_BYTE( "808t21.j17",   0x00001, 0x20000, CRC(42a30416) SHA1(8d9d27de96e79cae5230705beecadff0180cc479) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for the audio CPU */
	ROM_LOAD( "808e03.f4",    0x00000, 0x08000, CRC(3d93a7cd) SHA1(dcdd327e78f32436b276d0666f62a5b733b296e8) )

	ROM_REGION( 0x40000, REGION_GFX1, 0 )	/* graphics (addressable by the main CPU) */
	ROM_LOAD16_BYTE( "808e12.f28",   0x000000, 0x10000, CRC(d62f1fde) SHA1(1e55084f1294b6ac7c152fcd1800511fcab5d360) )        /* 8x8 tiles */
	ROM_LOAD16_BYTE( "808e13.h28",   0x000001, 0x10000, CRC(1fa708f4) SHA1(9511a19f50fb61571c2986c72d1a85e87b8d0495) )        /* 8x8 tiles */
	ROM_LOAD16_BYTE( "808e22.i28",   0x020000, 0x10000, CRC(73d758f6) SHA1(69e7079c3178f6f5acae533dae4854808c45bc29) )        /* 8x8 tiles */
	ROM_LOAD16_BYTE( "808e23.k28",   0x020001, 0x10000, CRC(8ff08b21) SHA1(9a8a03a960967f6f1d982b490f1724427538ecac) )        /* 8x8 tiles */

	ROM_REGION( 0x100000, REGION_GFX2, 0 )	/* graphics (addressable by the main CPU) */
	ROM_LOAD( "808d17.j4",    0x00000, 0x80000, CRC(d1299082) SHA1(c3c07b0517e7428ccd1cdf9e15aaf16d98e7c4cd) )	/* sprites */
	ROM_LOAD( "808d15.h4",    0x80000, 0x80000, CRC(2b22a6b6) SHA1(8e1af0627a4eac045128c4096e2cfb59c3d2f5ef) )

	ROM_REGION( 0x0100, REGION_PROMS, 0 )
	ROM_LOAD( "808a18.f16",   0x0000, 0x0100, CRC(eb95aede) SHA1(8153eb516ae9753910c6d6a2143e91e079586836) )	/* priority encoder (not used) */

	ROM_REGION( 0x20000, REGION_SOUND1, 0 )	/* 128k for the samples */
	ROM_LOAD( "808d01.d4",    0x00000, 0x20000, CRC(fd4d37c0) SHA1(ef91c6e7bb57c27a9a51729fffd1bfe3e806fb61) ) /* samples for 007232 */
ROM_END

ROM_START( mia2 )
	ROM_REGION( 0x40000, REGION_CPU1, 0 )	/* 2*128k and 2*64k for 68000 code */
	ROM_LOAD16_BYTE( "808s20.h17",   0x00000, 0x20000, CRC(caa2897f) SHA1(58f69586d1cd49acf64cf34a69a9ba88dba0923c) )
	ROM_LOAD16_BYTE( "808s21.j17",   0x00001, 0x20000, CRC(3d892ffb) SHA1(f6c0f8aa83f5688c8b57c5a66a481f65a5d4f530) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for the audio CPU */
	ROM_LOAD( "808e03.f4",    0x00000, 0x08000, CRC(3d93a7cd) SHA1(dcdd327e78f32436b276d0666f62a5b733b296e8) )

	ROM_REGION( 0x40000, REGION_GFX1, 0 )	/* graphics (addressable by the main CPU) */
	ROM_LOAD16_BYTE( "808e12.f28",   0x000000, 0x10000, CRC(d62f1fde) SHA1(1e55084f1294b6ac7c152fcd1800511fcab5d360) )        /* 8x8 tiles */
	ROM_LOAD16_BYTE( "808e13.h28",   0x000001, 0x10000, CRC(1fa708f4) SHA1(9511a19f50fb61571c2986c72d1a85e87b8d0495) )        /* 8x8 tiles */
	ROM_LOAD16_BYTE( "808e22.i28",   0x020000, 0x10000, CRC(73d758f6) SHA1(69e7079c3178f6f5acae533dae4854808c45bc29) )        /* 8x8 tiles */
	ROM_LOAD16_BYTE( "808e23.k28",   0x020001, 0x10000, CRC(8ff08b21) SHA1(9a8a03a960967f6f1d982b490f1724427538ecac) )        /* 8x8 tiles */

	ROM_REGION( 0x100000, REGION_GFX2, 0 )	/* graphics (addressable by the main CPU) */
	ROM_LOAD( "808d17.j4",    0x00000, 0x80000, CRC(d1299082) SHA1(c3c07b0517e7428ccd1cdf9e15aaf16d98e7c4cd) )	/* sprites */
	ROM_LOAD( "808d15.h4",    0x80000, 0x80000, CRC(2b22a6b6) SHA1(8e1af0627a4eac045128c4096e2cfb59c3d2f5ef) )

	ROM_REGION( 0x0100, REGION_PROMS, 0 )
	ROM_LOAD( "808a18.f16",   0x0000, 0x0100, CRC(eb95aede) SHA1(8153eb516ae9753910c6d6a2143e91e079586836) )	/* priority encoder (not used) */

	ROM_REGION( 0x20000, REGION_SOUND1, 0 )	/* 128k for the samples */
	ROM_LOAD( "808d01.d4",    0x00000, 0x20000, CRC(fd4d37c0) SHA1(ef91c6e7bb57c27a9a51729fffd1bfe3e806fb61) ) /* samples for 007232 */
ROM_END

ROM_START( tmnt )
	ROM_REGION( 0x60000, REGION_CPU1, 0 )	/* 2*128k and 2*64k for 68000 code */
	ROM_LOAD16_BYTE( "963-x23",      0x00000, 0x20000, CRC(a9549004) SHA1(bf9be5983af2282f627fb8408c069415c9b90229) )
	ROM_LOAD16_BYTE( "963-x24",      0x00001, 0x20000, CRC(e5cc9067) SHA1(649db4a09864eb8aba44cb77b580f1f28cfd80ed) )
	ROM_LOAD16_BYTE( "963-x21",      0x40000, 0x10000, CRC(5789cf92) SHA1(c1d1c958813062e5df5ac62e90ee4ce11f7e4a24) )
	ROM_LOAD16_BYTE( "963-x22",      0x40001, 0x10000, CRC(0a74e277) SHA1(c349d3c25eb05cc30ec1fd823475d971f3649f8b) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for the audio CPU */
	ROM_LOAD( "963-e20",      0x00000, 0x08000, CRC(1692a6d6) SHA1(68c3419012b2863e91a7d7e479fce5ceabb10b88) )

	ROM_REGION( 0x100000, REGION_GFX1, 0 )	/* graphics (addressable by the main CPU) */
	ROM_LOAD( "963-a28",      0x000000, 0x80000, CRC(db4769a8) SHA1(810811914f9c1fbf2320d5a9030cbf124f6d78cf) )        /* 8x8 tiles */
	ROM_LOAD( "963-a29",      0x080000, 0x80000, CRC(8069cd2e) SHA1(54095d3546119ccd1e8814d692aceb1327c9369f) )        /* 8x8 tiles */

	ROM_REGION( 0x200000, REGION_GFX2, 0 )	/* graphics (addressable by the main CPU) */
	ROM_LOAD( "963-a17",      0x000000, 0x80000, CRC(b5239a44) SHA1(84e94807e7c51aa652b4e4b827b36be59a53d0d6) )        /* sprites */
	ROM_LOAD( "963-a18",      0x080000, 0x80000, CRC(dd51adef) SHA1(5010c0911b0b9e4f23a785e8a751a0bde5be5be0) )        /* sprites */
	ROM_LOAD( "963-a15",      0x100000, 0x80000, CRC(1f324eed) SHA1(971a675578518fffa341a943d0cc4fdea005fde0) )        /* sprites */
	ROM_LOAD( "963-a16",      0x180000, 0x80000, CRC(d4bd9984) SHA1(d780ae7f72e16767c3a492544f02f0f1a332ab22) )        /* sprites */

	ROM_REGION( 0x0200, REGION_PROMS, 0 )
	ROM_LOAD( "963-a30",      0x0000, 0x0100, CRC(abd82680) SHA1(945a71e6ec65202f13209b45d45b616372d6c0f5) )	/* sprite address decoder */
	ROM_LOAD( "963-a31",      0x0100, 0x0100, CRC(f8004a1c) SHA1(ed6694b8eebfe0238b50ebd05007d519f6e57b1b) )	/* priority encoder (not used) */

	ROM_REGION( 0x20000, REGION_SOUND1, 0 )	/* 128k for the samples */
	ROM_LOAD( "963-a26",      0x00000, 0x20000, CRC(e2ac3063) SHA1(5bb294c46fb5eaba9935a18c0aa5d3931168f474) ) /* samples for 007232 */

	ROM_REGION( 0x20000, REGION_SOUND2, 0 )	/* 128k for the samples */
	ROM_LOAD( "963-a27",      0x00000, 0x20000, CRC(2dfd674b) SHA1(bbec5896c70056964fbc972a84bd5b0dfc6af257) ) /* samples for UPD7759C */

	ROM_REGION( 0x80000, REGION_SOUND3, 0 )	/* 512k for the title music sample */
	ROM_LOAD( "963-a25",      0x00000, 0x80000, CRC(fca078c7) SHA1(3e1124d72c9db4cb11d8de6c44b7aeca967f44e1) )
ROM_END

ROM_START( tmntu )
	ROM_REGION( 0x60000, REGION_CPU1, 0 )	/* 2*128k and 2*64k for 68000 code */
	ROM_LOAD16_BYTE( "963-r23",      0x00000, 0x20000, CRC(a7f61195) SHA1(db231ffb045f512040793b6815bcb998cee04c3d) )
	ROM_LOAD16_BYTE( "963-r24",      0x00001, 0x20000, CRC(661e056a) SHA1(4773883a66540c07dbc969881689184697355537) )
	ROM_LOAD16_BYTE( "963-r21",      0x40000, 0x10000, CRC(de047bb6) SHA1(d41d11f1b7dfd3824308f7fff43a5a7ced432ec2) )
	ROM_LOAD16_BYTE( "963-r22",      0x40001, 0x10000, CRC(d86a0888) SHA1(c761b3e8acc45a36ae691758c639eb826a8ab5b2) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for the audio CPU */
	ROM_LOAD( "963-e20",      0x00000, 0x08000, CRC(1692a6d6) SHA1(68c3419012b2863e91a7d7e479fce5ceabb10b88) )

	ROM_REGION( 0x100000, REGION_GFX1, 0 )	/* graphics (addressable by the main CPU) */
	ROM_LOAD( "963-a28",      0x000000, 0x80000, CRC(db4769a8) SHA1(810811914f9c1fbf2320d5a9030cbf124f6d78cf) )        /* 8x8 tiles */
	ROM_LOAD( "963-a29",      0x080000, 0x80000, CRC(8069cd2e) SHA1(54095d3546119ccd1e8814d692aceb1327c9369f) )        /* 8x8 tiles */

	ROM_REGION( 0x200000, REGION_GFX2, 0 )	/* graphics (addressable by the main CPU) */
	ROM_LOAD( "963-a17",      0x000000, 0x80000, CRC(b5239a44) SHA1(84e94807e7c51aa652b4e4b827b36be59a53d0d6) )        /* sprites */
	ROM_LOAD( "963-a18",      0x080000, 0x80000, CRC(dd51adef) SHA1(5010c0911b0b9e4f23a785e8a751a0bde5be5be0) )        /* sprites */
	ROM_LOAD( "963-a15",      0x100000, 0x80000, CRC(1f324eed) SHA1(971a675578518fffa341a943d0cc4fdea005fde0) )        /* sprites */
	ROM_LOAD( "963-a16",      0x180000, 0x80000, CRC(d4bd9984) SHA1(d780ae7f72e16767c3a492544f02f0f1a332ab22) )        /* sprites */

	ROM_REGION( 0x0200, REGION_PROMS, 0 )
	ROM_LOAD( "963-a30",      0x0000, 0x0100, CRC(abd82680) SHA1(945a71e6ec65202f13209b45d45b616372d6c0f5) )	/* sprite address decoder */
	ROM_LOAD( "963-a31",      0x0100, 0x0100, CRC(f8004a1c) SHA1(ed6694b8eebfe0238b50ebd05007d519f6e57b1b) )	/* priority encoder (not used) */

	ROM_REGION( 0x20000, REGION_SOUND1, 0 )	/* 128k for the samples */
	ROM_LOAD( "963-a26",      0x00000, 0x20000, CRC(e2ac3063) SHA1(5bb294c46fb5eaba9935a18c0aa5d3931168f474) ) /* samples for 007232 */

	ROM_REGION( 0x20000, REGION_SOUND2, 0 )	/* 128k for the samples */
	ROM_LOAD( "963-a27",      0x00000, 0x20000, CRC(2dfd674b) SHA1(bbec5896c70056964fbc972a84bd5b0dfc6af257) ) /* samples for UPD7759C */

	ROM_REGION( 0x80000, REGION_SOUND3, 0 )	/* 512k for the title music sample */
	ROM_LOAD( "963-a25",      0x00000, 0x80000, CRC(fca078c7) SHA1(3e1124d72c9db4cb11d8de6c44b7aeca967f44e1) )
ROM_END

ROM_START( tmht )
	ROM_REGION( 0x60000, REGION_CPU1, 0 )	/* 2*128k and 2*64k for 68000 code */
	ROM_LOAD16_BYTE( "963f23.j17",   0x00000, 0x20000, CRC(9cb5e461) SHA1(b693e61070d6ce7ac59ff3f0a824cfefb37b33eb) )
	ROM_LOAD16_BYTE( "963f24.k17",   0x00001, 0x20000, CRC(2d902fab) SHA1(5a9a3bb0b6c2824eb971a8c0aa8d3069d3c63d06) )
	ROM_LOAD16_BYTE( "963f21.j15",   0x40000, 0x10000, CRC(9fa25378) SHA1(9ed0bba148e7c5e78224c5168053eeafc2e4b663) )
	ROM_LOAD16_BYTE( "963f22.k15",   0x40001, 0x10000, CRC(2127ee53) SHA1(e614260883872fd27cd641e6b4787672b2a44139) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for the audio CPU */
	ROM_LOAD( "963-e20",      0x00000, 0x08000, CRC(1692a6d6) SHA1(68c3419012b2863e91a7d7e479fce5ceabb10b88) )

	ROM_REGION( 0x100000, REGION_GFX1, 0 )	/* graphics (addressable by the main CPU) */
	ROM_LOAD( "963-a28",      0x000000, 0x80000, CRC(db4769a8) SHA1(810811914f9c1fbf2320d5a9030cbf124f6d78cf) )        /* 8x8 tiles */
	ROM_LOAD( "963-a29",      0x080000, 0x80000, CRC(8069cd2e) SHA1(54095d3546119ccd1e8814d692aceb1327c9369f) )        /* 8x8 tiles */

	ROM_REGION( 0x200000, REGION_GFX2, 0 )	/* graphics (addressable by the main CPU) */
	ROM_LOAD( "963-a17",      0x000000, 0x80000, CRC(b5239a44) SHA1(84e94807e7c51aa652b4e4b827b36be59a53d0d6) )        /* sprites */
	ROM_LOAD( "963-a18",      0x080000, 0x80000, CRC(dd51adef) SHA1(5010c0911b0b9e4f23a785e8a751a0bde5be5be0) )        /* sprites */
	ROM_LOAD( "963-a15",      0x100000, 0x80000, CRC(1f324eed) SHA1(971a675578518fffa341a943d0cc4fdea005fde0) )        /* sprites */
	ROM_LOAD( "963-a16",      0x180000, 0x80000, CRC(d4bd9984) SHA1(d780ae7f72e16767c3a492544f02f0f1a332ab22) )        /* sprites */

	ROM_REGION( 0x0200, REGION_PROMS, 0 )
	ROM_LOAD( "963-a30",      0x0000, 0x0100, CRC(abd82680) SHA1(945a71e6ec65202f13209b45d45b616372d6c0f5) )	/* sprite address decoder */
	ROM_LOAD( "963-a31",      0x0100, 0x0100, CRC(f8004a1c) SHA1(ed6694b8eebfe0238b50ebd05007d519f6e57b1b) )	/* priority encoder (not used) */

	ROM_REGION( 0x20000, REGION_SOUND1, 0 )	/* 128k for the samples */
	ROM_LOAD( "963-a26",      0x00000, 0x20000, CRC(e2ac3063) SHA1(5bb294c46fb5eaba9935a18c0aa5d3931168f474) ) /* samples for 007232 */

	ROM_REGION( 0x20000, REGION_SOUND2, 0 )	/* 128k for the samples */
	ROM_LOAD( "963-a27",      0x00000, 0x20000, CRC(2dfd674b) SHA1(bbec5896c70056964fbc972a84bd5b0dfc6af257) ) /* samples for UPD7759C */

	ROM_REGION( 0x80000, REGION_SOUND3, 0 )	/* 512k for the title music sample */
	ROM_LOAD( "963-a25",      0x00000, 0x80000, CRC(fca078c7) SHA1(3e1124d72c9db4cb11d8de6c44b7aeca967f44e1) )
ROM_END

ROM_START( tmntj )
	ROM_REGION( 0x60000, REGION_CPU1, 0 )	/* 2*128k and 2*64k for 68000 code */
	ROM_LOAD16_BYTE( "963_223.j17",  0x00000, 0x20000, CRC(0d34a5ff) SHA1(a387f3e7c727dc66ebb0e1f40e4ab8dc83f647e5) )
	ROM_LOAD16_BYTE( "963_224.k17",  0x00001, 0x20000, CRC(2fd453f2) SHA1(8eb68cba3b5f5baf2c00172942a3d2bf578d0196) )
	ROM_LOAD16_BYTE( "963_221.j15",  0x40000, 0x10000, CRC(fa8e25fd) SHA1(129cb9498508cdabdda3cf4fc86ff716fe1da940) )
	ROM_LOAD16_BYTE( "963_222.k15",  0x40001, 0x10000, CRC(ca437a4f) SHA1(96922d2dcd0d84dc0d09a3ba9800b1154b5e2486) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for the audio CPU */
	ROM_LOAD( "963-e20",      0x00000, 0x08000, CRC(1692a6d6) SHA1(68c3419012b2863e91a7d7e479fce5ceabb10b88) )

	ROM_REGION( 0x100000, REGION_GFX1, 0 )	/* graphics (addressable by the main CPU) */
	ROM_LOAD( "963-a28",      0x000000, 0x80000, CRC(db4769a8) SHA1(810811914f9c1fbf2320d5a9030cbf124f6d78cf) )        /* 8x8 tiles */
	ROM_LOAD( "963-a29",      0x080000, 0x80000, CRC(8069cd2e) SHA1(54095d3546119ccd1e8814d692aceb1327c9369f) )        /* 8x8 tiles */

	ROM_REGION( 0x200000, REGION_GFX2, 0 )	/* graphics (addressable by the main CPU) */
	ROM_LOAD( "963-a17",      0x000000, 0x80000, CRC(b5239a44) SHA1(84e94807e7c51aa652b4e4b827b36be59a53d0d6) )        /* sprites */
	ROM_LOAD( "963-a18",      0x080000, 0x80000, CRC(dd51adef) SHA1(5010c0911b0b9e4f23a785e8a751a0bde5be5be0) )        /* sprites */
	ROM_LOAD( "963-a15",      0x100000, 0x80000, CRC(1f324eed) SHA1(971a675578518fffa341a943d0cc4fdea005fde0) )        /* sprites */
	ROM_LOAD( "963-a16",      0x180000, 0x80000, CRC(d4bd9984) SHA1(d780ae7f72e16767c3a492544f02f0f1a332ab22) )        /* sprites */

	ROM_REGION( 0x0200, REGION_PROMS, 0 )
	ROM_LOAD( "963-a30",      0x0000, 0x0100, CRC(abd82680) SHA1(945a71e6ec65202f13209b45d45b616372d6c0f5) )	/* sprite address decoder */
	ROM_LOAD( "963-a31",      0x0100, 0x0100, CRC(f8004a1c) SHA1(ed6694b8eebfe0238b50ebd05007d519f6e57b1b) )	/* priority encoder (not used) */

	ROM_REGION( 0x20000, REGION_SOUND1, 0 )	/* 128k for the samples */
	ROM_LOAD( "963-a26",      0x00000, 0x20000, CRC(e2ac3063) SHA1(5bb294c46fb5eaba9935a18c0aa5d3931168f474) ) /* samples for 007232 */

	ROM_REGION( 0x20000, REGION_SOUND2, 0 )	/* 128k for the samples */
	ROM_LOAD( "963-a27",      0x00000, 0x20000, CRC(2dfd674b) SHA1(bbec5896c70056964fbc972a84bd5b0dfc6af257) ) /* samples for UPD7759C */

	ROM_REGION( 0x80000, REGION_SOUND3, 0 )	/* 512k for the title music sample */
	ROM_LOAD( "963-a25",      0x00000, 0x80000, CRC(fca078c7) SHA1(3e1124d72c9db4cb11d8de6c44b7aeca967f44e1) )
ROM_END

ROM_START( tmht2p )
	ROM_REGION( 0x60000, REGION_CPU1, 0 )	/* 2*128k and 2*64k for 68000 code */
	ROM_LOAD16_BYTE( "963-u23",      0x00000, 0x20000, CRC(58bec748) SHA1(6cf146d6de8ef01c0705394d135abebc3aeaae16) )
	ROM_LOAD16_BYTE( "963-u24",      0x00001, 0x20000, CRC(dce87c8d) SHA1(b85018ffc226ec7dfc97f9cd0f4454951c6e5918) )
	ROM_LOAD16_BYTE( "963-u21",      0x40000, 0x10000, CRC(abce5ead) SHA1(2b3674497bb4f688c5f0e1cc9a078b3feb01475d) )
	ROM_LOAD16_BYTE( "963-u22",      0x40001, 0x10000, CRC(4ecc8d6b) SHA1(ce29aecbd98c0a07f48766564de173facb310371) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for the audio CPU */
	ROM_LOAD( "963-e20",      0x00000, 0x08000, CRC(1692a6d6) SHA1(68c3419012b2863e91a7d7e479fce5ceabb10b88) )

	ROM_REGION( 0x100000, REGION_GFX1, 0 )	/* graphics (addressable by the main CPU) */
	ROM_LOAD( "963-a28",      0x000000, 0x80000, CRC(db4769a8) SHA1(810811914f9c1fbf2320d5a9030cbf124f6d78cf) )        /* 8x8 tiles */
	ROM_LOAD( "963-a29",      0x080000, 0x80000, CRC(8069cd2e) SHA1(54095d3546119ccd1e8814d692aceb1327c9369f) )        /* 8x8 tiles */

	ROM_REGION( 0x200000, REGION_GFX2, 0 )	/* graphics (addressable by the main CPU) */
	ROM_LOAD( "963-a17",      0x000000, 0x80000, CRC(b5239a44) SHA1(84e94807e7c51aa652b4e4b827b36be59a53d0d6) )        /* sprites */
	ROM_LOAD( "963-a18",      0x080000, 0x80000, CRC(dd51adef) SHA1(5010c0911b0b9e4f23a785e8a751a0bde5be5be0) )        /* sprites */
	ROM_LOAD( "963-a15",      0x100000, 0x80000, CRC(1f324eed) SHA1(971a675578518fffa341a943d0cc4fdea005fde0) )        /* sprites */
	ROM_LOAD( "963-a16",      0x180000, 0x80000, CRC(d4bd9984) SHA1(d780ae7f72e16767c3a492544f02f0f1a332ab22) )        /* sprites */

	ROM_REGION( 0x0200, REGION_PROMS, 0 )
	ROM_LOAD( "963-a30",      0x0000, 0x0100, CRC(abd82680) SHA1(945a71e6ec65202f13209b45d45b616372d6c0f5) )	/* sprite address decoder */
	ROM_LOAD( "963-a31",      0x0100, 0x0100, CRC(f8004a1c) SHA1(ed6694b8eebfe0238b50ebd05007d519f6e57b1b) )	/* priority encoder (not used) */

	ROM_REGION( 0x20000, REGION_SOUND1, 0 )	/* 128k for the samples */
	ROM_LOAD( "963-a26",      0x00000, 0x20000, CRC(e2ac3063) SHA1(5bb294c46fb5eaba9935a18c0aa5d3931168f474) ) /* samples for 007232 */

	ROM_REGION( 0x20000, REGION_SOUND2, 0 )	/* 128k for the samples */
	ROM_LOAD( "963-a27",      0x00000, 0x20000, CRC(2dfd674b) SHA1(bbec5896c70056964fbc972a84bd5b0dfc6af257) ) /* samples for UPD7759C */

	ROM_REGION( 0x80000, REGION_SOUND3, 0 )	/* 512k for the title music sample */
	ROM_LOAD( "963-a25",      0x00000, 0x80000, CRC(fca078c7) SHA1(3e1124d72c9db4cb11d8de6c44b7aeca967f44e1) )
ROM_END

ROM_START( tmnt2pj )
	ROM_REGION( 0x60000, REGION_CPU1, 0 )	/* 2*128k and 2*64k for 68000 code */
	ROM_LOAD16_BYTE( "963-123",      0x00000, 0x20000, CRC(6a3527c9) SHA1(a5a8cbec3fae3f37d4d82a7700cec3c96c6a362f) )
	ROM_LOAD16_BYTE( "963-124",      0x00001, 0x20000, CRC(2c4bfa15) SHA1(0264ef6f15806d52d6f7869034f5a3024ba1cea2) )
	ROM_LOAD16_BYTE( "963-121",      0x40000, 0x10000, CRC(4181b733) SHA1(306601597102a1bc79880e557889a6fce7b30b7b) )
	ROM_LOAD16_BYTE( "963-122",      0x40001, 0x10000, CRC(c64eb5ff) SHA1(e546f1cb81e98a38833cd0affe73e2bc1d95d017) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for the audio CPU */
	ROM_LOAD( "963-e20",      0x00000, 0x08000, CRC(1692a6d6) SHA1(68c3419012b2863e91a7d7e479fce5ceabb10b88) )

	ROM_REGION( 0x100000, REGION_GFX1, 0 )	/* graphics (addressable by the main CPU) */
	ROM_LOAD( "963-a28",      0x000000, 0x80000, CRC(db4769a8) SHA1(810811914f9c1fbf2320d5a9030cbf124f6d78cf) )        /* 8x8 tiles */
	ROM_LOAD( "963-a29",      0x080000, 0x80000, CRC(8069cd2e) SHA1(54095d3546119ccd1e8814d692aceb1327c9369f) )        /* 8x8 tiles */

	ROM_REGION( 0x200000, REGION_GFX2, 0 )	/* graphics (addressable by the main CPU) */
	ROM_LOAD( "963-a17",      0x000000, 0x80000, CRC(b5239a44) SHA1(84e94807e7c51aa652b4e4b827b36be59a53d0d6) )        /* sprites */
	ROM_LOAD( "963-a18",      0x080000, 0x80000, CRC(dd51adef) SHA1(5010c0911b0b9e4f23a785e8a751a0bde5be5be0) )        /* sprites */
	ROM_LOAD( "963-a15",      0x100000, 0x80000, CRC(1f324eed) SHA1(971a675578518fffa341a943d0cc4fdea005fde0) )        /* sprites */
	ROM_LOAD( "963-a16",      0x180000, 0x80000, CRC(d4bd9984) SHA1(d780ae7f72e16767c3a492544f02f0f1a332ab22) )        /* sprites */

	ROM_REGION( 0x0200, REGION_PROMS, 0 )
	ROM_LOAD( "963-a30",      0x0000, 0x0100, CRC(abd82680) SHA1(945a71e6ec65202f13209b45d45b616372d6c0f5) )	/* sprite address decoder */
	ROM_LOAD( "963-a31",      0x0100, 0x0100, CRC(f8004a1c) SHA1(ed6694b8eebfe0238b50ebd05007d519f6e57b1b) )	/* priority encoder (not used) */

	ROM_REGION( 0x20000, REGION_SOUND1, 0 )	/* 128k for the samples */
	ROM_LOAD( "963-a26",      0x00000, 0x20000, CRC(e2ac3063) SHA1(5bb294c46fb5eaba9935a18c0aa5d3931168f474) ) /* samples for 007232 */

	ROM_REGION( 0x20000, REGION_SOUND2, 0 )	/* 128k for the samples */
	ROM_LOAD( "963-a27",      0x00000, 0x20000, CRC(2dfd674b) SHA1(bbec5896c70056964fbc972a84bd5b0dfc6af257) ) /* samples for UPD7759C */

	ROM_REGION( 0x80000, REGION_SOUND3, 0 )	/* 512k for the title music sample */
	ROM_LOAD( "963-a25",      0x00000, 0x80000, CRC(fca078c7) SHA1(3e1124d72c9db4cb11d8de6c44b7aeca967f44e1) )
ROM_END

ROM_START( tmnt2po )
	ROM_REGION( 0x60000, REGION_CPU1, 0 )	/* 2*128k and 2*64k for 68000 code */
	ROM_LOAD16_BYTE( "tmnt123.bin",  0x00000, 0x20000, CRC(2d905183) SHA1(38c77a08733f9da1dc6f1c510a2c8dac34848787) )
	ROM_LOAD16_BYTE( "tmnt124.bin",  0x00001, 0x20000, CRC(e0125352) SHA1(e2a297bf96d0fa1d19ce767786453c489d49d693) )
	ROM_LOAD16_BYTE( "tmnt21.bin",   0x40000, 0x10000, CRC(12deeafb) SHA1(1f70a326f8f4a896da297b4f66ca467894d22159) )
	ROM_LOAD16_BYTE( "tmnt22.bin",   0x40001, 0x10000, CRC(aec4f1c3) SHA1(189ed93bc9ee4a1ff1c0ca7b80f4e817e5484e69) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for the audio CPU */
	ROM_LOAD( "963-e20",      0x00000, 0x08000, CRC(1692a6d6) SHA1(68c3419012b2863e91a7d7e479fce5ceabb10b88) )

	ROM_REGION( 0x100000, REGION_GFX1, 0 )	/* graphics (addressable by the main CPU) */
	ROM_LOAD( "963-a28",      0x000000, 0x80000, CRC(db4769a8) SHA1(810811914f9c1fbf2320d5a9030cbf124f6d78cf) )        /* 8x8 tiles */
	ROM_LOAD( "963-a29",      0x080000, 0x80000, CRC(8069cd2e) SHA1(54095d3546119ccd1e8814d692aceb1327c9369f) )        /* 8x8 tiles */

	ROM_REGION( 0x200000, REGION_GFX2, 0 )	/* graphics (addressable by the main CPU) */
	ROM_LOAD( "963-a17",      0x000000, 0x80000, CRC(b5239a44) SHA1(84e94807e7c51aa652b4e4b827b36be59a53d0d6) )        /* sprites */
	ROM_LOAD( "963-a18",      0x080000, 0x80000, CRC(dd51adef) SHA1(5010c0911b0b9e4f23a785e8a751a0bde5be5be0) )        /* sprites */
	ROM_LOAD( "963-a15",      0x100000, 0x80000, CRC(1f324eed) SHA1(971a675578518fffa341a943d0cc4fdea005fde0) )        /* sprites */
	ROM_LOAD( "963-a16",      0x180000, 0x80000, CRC(d4bd9984) SHA1(d780ae7f72e16767c3a492544f02f0f1a332ab22) )        /* sprites */

	ROM_REGION( 0x0200, REGION_PROMS, 0 )
	ROM_LOAD( "963-a30",      0x0000, 0x0100, CRC(abd82680) SHA1(945a71e6ec65202f13209b45d45b616372d6c0f5) )	/* sprite address decoder */
	ROM_LOAD( "963-a31",      0x0100, 0x0100, CRC(f8004a1c) SHA1(ed6694b8eebfe0238b50ebd05007d519f6e57b1b) )	/* priority encoder (not used) */

	ROM_REGION( 0x20000, REGION_SOUND1, 0 )	/* 128k for the samples */
	ROM_LOAD( "963-a26",      0x00000, 0x20000, CRC(e2ac3063) SHA1(5bb294c46fb5eaba9935a18c0aa5d3931168f474) ) /* samples for 007232 */

	ROM_REGION( 0x20000, REGION_SOUND2, 0 )	/* 128k for the samples */
	ROM_LOAD( "963-a27",      0x00000, 0x20000, CRC(2dfd674b) SHA1(bbec5896c70056964fbc972a84bd5b0dfc6af257) ) /* samples for UPD7759C */

	ROM_REGION( 0x80000, REGION_SOUND3, 0 )	/* 512k for the title music sample */
	ROM_LOAD( "963-a25",      0x00000, 0x80000, CRC(fca078c7) SHA1(3e1124d72c9db4cb11d8de6c44b7aeca967f44e1) )
ROM_END

ROM_START( punkshot )
	ROM_REGION( 0x40000, REGION_CPU1, 0 )	/* 4*64k for 68000 code */
	ROM_LOAD16_BYTE( "907-j02.i7",   0x00000, 0x20000, CRC(dbb3a23b) SHA1(78f999f4e5b12641195a7f9f7fedf696e32ff0c0) )
	ROM_LOAD16_BYTE( "907-j03.i10",  0x00001, 0x20000, CRC(2151d1ab) SHA1(e71768142b903825f8104ffc90906b0d471599e0) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for the audio CPU */
	ROM_LOAD( "907f01.e8",    0x0000, 0x8000, CRC(f040c484) SHA1(f76a739cacc0aba98a5bf85a48c81cef0d9bbfb4) )

	ROM_REGION( 0x80000, REGION_GFX1, 0 )	/* graphics (addressable by the main CPU) */
	ROM_LOAD( "907d06.e23",   0x000000, 0x40000, CRC(f5cc38f4) SHA1(e6dc9994582a08740dc2fcb30a38771053627d5f) )
	ROM_LOAD( "907d05.e22",   0x040000, 0x40000, CRC(e25774c1) SHA1(74fda3b418b4b0064b5e660a93122b07f6d41416) )

	ROM_REGION( 0x200000, REGION_GFX2, 0 )	/* graphics (addressable by the main CPU) */
	ROM_LOAD( "907d07.k2",    0x000000, 0x100000, CRC(b0fe4543) SHA1(3be1caef29084063dd8754c1eecc34a2ec842415) )
	ROM_LOAD( "907d08.k7",    0x100000, 0x100000, CRC(d5ac8d9d) SHA1(cb330be1c5c016465ef7048b3b29c65a741ee45b) )

	ROM_REGION( 0x80000, REGION_SOUND1, 0 )	/* samples for 053260 */
	ROM_LOAD( "907d04.d3",    0x0000, 0x80000, CRC(090feb5e) SHA1(2394907b62ff0724c277642caf6375239249e2d7) )
ROM_END

ROM_START( punksht2 )
	ROM_REGION( 0x40000, REGION_CPU1, 0 )	/* 4*64k for 68000 code */
	ROM_LOAD16_BYTE( "907m02.i7",    0x00000, 0x20000, CRC(59e14575) SHA1(249fc98a2d5fa3e4779438c37d22c0256be8d3fa) )
	ROM_LOAD16_BYTE( "907m03.i10",   0x00001, 0x20000, CRC(adb14b1e) SHA1(c5db1c3b70ab3e53cd6a600b82bdccda4db05f90) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for the audio CPU */
	ROM_LOAD( "907f01.e8",    0x0000, 0x8000, CRC(f040c484) SHA1(f76a739cacc0aba98a5bf85a48c81cef0d9bbfb4) )

	ROM_REGION( 0x80000, REGION_GFX1, 0 )	/* graphics (addressable by the main CPU) */
	ROM_LOAD( "907d06.e23",   0x000000, 0x40000, CRC(f5cc38f4) SHA1(e6dc9994582a08740dc2fcb30a38771053627d5f) )
	ROM_LOAD( "907d05.e22",   0x040000, 0x40000, CRC(e25774c1) SHA1(74fda3b418b4b0064b5e660a93122b07f6d41416) )

	ROM_REGION( 0x200000, REGION_GFX2, 0 )	/* graphics (addressable by the main CPU) */
	ROM_LOAD( "907d07.k2",    0x000000, 0x100000, CRC(b0fe4543) SHA1(3be1caef29084063dd8754c1eecc34a2ec842415) )
	ROM_LOAD( "907d08.k7",    0x100000, 0x100000, CRC(d5ac8d9d) SHA1(cb330be1c5c016465ef7048b3b29c65a741ee45b) )

	ROM_REGION( 0x80000, REGION_SOUND1, 0 )	/* samples for the 053260 */
	ROM_LOAD( "907d04.d3",    0x0000, 0x80000, CRC(090feb5e) SHA1(2394907b62ff0724c277642caf6375239249e2d7) )
ROM_END

ROM_START( punkshtj )
	ROM_REGION( 0x40000, REGION_CPU1, 0 )	/* 4*64k for 68000 code */
	ROM_LOAD16_BYTE( "907z02.i7",    0x00000, 0x20000, CRC(7a3a5c89) SHA1(240967b911df8939b048bbcdfcac668455fc82e9) )
	ROM_LOAD16_BYTE( "907z03.i10",   0x00001, 0x20000, CRC(22a3d9d6) SHA1(76f016435956088aa680297ee9ba0abda446a7bb) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for the audio CPU */
	ROM_LOAD( "907f01.e8",    0x0000, 0x8000, CRC(f040c484) SHA1(f76a739cacc0aba98a5bf85a48c81cef0d9bbfb4) )

	ROM_REGION( 0x80000, REGION_GFX1, 0 )	/* graphics (addressable by the main CPU) */
	ROM_LOAD( "907d06.e23",   0x000000, 0x40000, CRC(f5cc38f4) SHA1(e6dc9994582a08740dc2fcb30a38771053627d5f) )
	ROM_LOAD( "907d05.e22",   0x040000, 0x40000, CRC(e25774c1) SHA1(74fda3b418b4b0064b5e660a93122b07f6d41416) )

	ROM_REGION( 0x200000, REGION_GFX2, 0 )	/* graphics (addressable by the main CPU) */
	ROM_LOAD( "907d07.k2",    0x000000, 0x100000, CRC(b0fe4543) SHA1(3be1caef29084063dd8754c1eecc34a2ec842415) )
	ROM_LOAD( "907d08.k7",    0x100000, 0x100000, CRC(d5ac8d9d) SHA1(cb330be1c5c016465ef7048b3b29c65a741ee45b) )

	ROM_REGION( 0x80000, REGION_SOUND1, 0 )	/* samples for the 053260 */
	ROM_LOAD( "907d04.d3",    0x0000, 0x80000, CRC(090feb5e) SHA1(2394907b62ff0724c277642caf6375239249e2d7) )
ROM_END

ROM_START( lgtnfght )
	ROM_REGION( 0x40000, REGION_CPU1, 0 )	/* 4*64k for 68000 code */
	ROM_LOAD16_BYTE( "939m02.e11",   0x00000, 0x20000, CRC(61a12184) SHA1(f6d82aa0a444f885fd1e5d3d1464798b639a1710) )
	ROM_LOAD16_BYTE( "939m03.e15",   0x00001, 0x20000, CRC(6db6659d) SHA1(def943b906eab68a0b86f9a28fb0b9a1f3b65e4c) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for the audio CPU */
	ROM_LOAD( "939e01.d7",    0x0000, 0x8000, CRC(4a5fc848) SHA1(878825e07c2718b7c923ad7c77daddf18cb28beb) )

	ROM_REGION( 0x100000, REGION_GFX1, 0 )	/* graphics (addressable by the main CPU) */
	ROM_LOAD( "939a07.k14",   0x000000, 0x80000, CRC(7955dfcf) SHA1(012644c1bfbe2e5d1c7ba25f29ebfde7dbfd1c0d) )
	ROM_LOAD( "939a08.k19",   0x080000, 0x80000, CRC(ed95b385) SHA1(5aa5291cf1a8935b0a65ae10aa20b9cf9a138b03) )

	ROM_REGION( 0x100000, REGION_GFX2, 0 )	/* graphics (addressable by the main CPU) */
	ROM_LOAD( "939a06.k8",    0x000000, 0x80000, CRC(e393c206) SHA1(9b35fc6dba1f15c3d9d69ff5a4e1673c539aa533) )
	ROM_LOAD( "939a05.k2",    0x080000, 0x80000, CRC(3662d47a) SHA1(789c3f07ce812902050970f48be5115b8e95bea0) )

	ROM_REGION( 0x80000, REGION_SOUND1, 0 )	/* samples for the 053260 */
	ROM_LOAD( "939a04.c5",    0x0000, 0x80000, CRC(c24e2b6e) SHA1(affc142883c2383afd08dcf156e48709ceca49fd) )
ROM_END

ROM_START( trigon )
	ROM_REGION( 0x40000, REGION_CPU1, 0 )	/* 4*64k for 68000 code */
	ROM_LOAD16_BYTE( "939j02.bin",   0x00000, 0x20000, CRC(38381d1b) SHA1(d4ddf883f61e5d48143cf467ba3c9c5b37f7e790) )
	ROM_LOAD16_BYTE( "939j03.bin",   0x00001, 0x20000, CRC(b5beddcd) SHA1(dc5d79793d5453f284bf7fd198ba7c4ab1fc09c3) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for the audio CPU */
	ROM_LOAD( "939e01.d7",    0x0000, 0x8000, CRC(4a5fc848) SHA1(878825e07c2718b7c923ad7c77daddf18cb28beb) )

	ROM_REGION( 0x100000, REGION_GFX1, 0 )	/* graphics (addressable by the main CPU) */
	ROM_LOAD( "939a07.k14",   0x000000, 0x80000, CRC(7955dfcf) SHA1(012644c1bfbe2e5d1c7ba25f29ebfde7dbfd1c0d) )
	ROM_LOAD( "939a08.k19",   0x080000, 0x80000, CRC(ed95b385) SHA1(5aa5291cf1a8935b0a65ae10aa20b9cf9a138b03) )

	ROM_REGION( 0x100000, REGION_GFX2, 0 )	/* graphics (addressable by the main CPU) */
	ROM_LOAD( "939a06.k8",    0x000000, 0x80000, CRC(e393c206) SHA1(9b35fc6dba1f15c3d9d69ff5a4e1673c539aa533) )
	ROM_LOAD( "939a05.k2",    0x080000, 0x80000, CRC(3662d47a) SHA1(789c3f07ce812902050970f48be5115b8e95bea0) )

	ROM_REGION( 0x80000, REGION_SOUND1, 0 )	/* samples for the 053260 */
	ROM_LOAD( "939a04.c5",    0x0000, 0x80000, CRC(c24e2b6e) SHA1(affc142883c2383afd08dcf156e48709ceca49fd) )
ROM_END

ROM_START( blswhstl )
	ROM_REGION( 0x80000, REGION_CPU1, 0 )
	ROM_LOAD16_BYTE( "e09.bin",     0x000000, 0x20000, CRC(e8b7b234) SHA1(65ae9faf34ed8ab71013acdc84e9429e5f5fb7a2) )
	ROM_LOAD16_BYTE( "g09.bin",     0x000001, 0x20000, CRC(3c26d281) SHA1(d348305ecd4457e023bcdbc39842096d23c455fb) )
	ROM_LOAD16_BYTE( "e11.bin",     0x040000, 0x20000, CRC(14628736) SHA1(87f7a65cffb87085b3e21043bd46fbb7db9266dd) )
	ROM_LOAD16_BYTE( "g11.bin",     0x040001, 0x20000, CRC(f738ad4a) SHA1(5aea4afa4bf935d3e92856eff745f61ed4d98165) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* 64k for the audio CPU */
	ROM_LOAD( "060_j01.rom",  0x0000, 0x10000, CRC(f9d9a673) SHA1(8e5631c20dc37913cc7fa84f7ef786ff1ef85f09) )

    ROM_REGION( 0x100000, REGION_GFX1, 0 )	/* graphics (addressable by the main CPU) */
	ROM_LOAD16_WORD_SWAP( "060_e07.r16",  0x000000, 0x080000, CRC(c400edf3) SHA1(3f507df8804c1774e2e213f5eb8be0aa7e818d65) )	/* tiles */
	ROM_LOAD16_WORD_SWAP( "060_e08.r16",  0x080000, 0x080000, CRC(70dddba1) SHA1(2acb94f249cf89b3d53798a6ee1c960f84a04d2e) )

	ROM_REGION( 0x100000, REGION_GFX2, 0 )	/* graphics (addressable by the main CPU) */
	ROM_LOAD16_WORD_SWAP( "060_e06.r16",  0x000000, 0x080000, CRC(09381492) SHA1(5a3008dec99a8e0043405e9c4f5145794b8606e0) )	/* sprites */
	ROM_LOAD16_WORD_SWAP( "060_e05.r16",  0x080000, 0x080000, CRC(32454241) SHA1(7a246b255ff30118c4f8e07e6ba03a22fd5ddc8a) )

	ROM_REGION( 0x100000, REGION_SOUND1, 0 )	/* samples for the 053260 */
	ROM_LOAD( "060_e04.r16",  0x0000, 0x100000, CRC(c680395d) SHA1(acde593a5ec501e89c8aaca6c4fbacf707a727e1) )
ROM_END

ROM_START( detatwin )
	ROM_REGION( 0x80000, REGION_CPU1, 0 )
	ROM_LOAD16_BYTE( "060_j02.rom", 0x000000, 0x20000, CRC(11b761ac) SHA1(1a143b0a43da48bdcfe085a2a9d1a2de0329fafd) )
	ROM_LOAD16_BYTE( "060_j03.rom", 0x000001, 0x20000, CRC(8d0b588c) SHA1(a444493557cc19c7828b40a54dac9165c1f5b541) )
	ROM_LOAD16_BYTE( "060_j09.rom", 0x040000, 0x20000, CRC(f2a5f15f) SHA1(4b8786e5ce0b895e6358e16e2a0a926325d0afcc) )
	ROM_LOAD16_BYTE( "060_j10.rom", 0x040001, 0x20000, CRC(36eefdbc) SHA1(a3ec5078779b4ab33edf32e04db3e221e52b36c7) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* 64k for the audio CPU */
	ROM_LOAD( "060_j01.rom",  0x0000, 0x10000, CRC(f9d9a673) SHA1(8e5631c20dc37913cc7fa84f7ef786ff1ef85f09) )

    ROM_REGION( 0x100000, REGION_GFX1, 0 )	/* graphics (addressable by the main CPU) */
	ROM_LOAD16_WORD_SWAP( "060_e07.r16",  0x000000, 0x080000, CRC(c400edf3) SHA1(3f507df8804c1774e2e213f5eb8be0aa7e818d65) )	/* tiles */
	ROM_LOAD16_WORD_SWAP( "060_e08.r16",  0x080000, 0x080000, CRC(70dddba1) SHA1(2acb94f249cf89b3d53798a6ee1c960f84a04d2e) )

	ROM_REGION( 0x100000, REGION_GFX2, 0 )	/* graphics (addressable by the main CPU) */
	ROM_LOAD16_WORD_SWAP( "060_e06.r16",  0x000000, 0x080000, CRC(09381492) SHA1(5a3008dec99a8e0043405e9c4f5145794b8606e0) )	/* sprites */
	ROM_LOAD16_WORD_SWAP( "060_e05.r16",  0x080000, 0x080000, CRC(32454241) SHA1(7a246b255ff30118c4f8e07e6ba03a22fd5ddc8a) )

	ROM_REGION( 0x100000, REGION_SOUND1, 0 )	/* samples for the 053260 */
	ROM_LOAD( "060_e04.r16",  0x0000, 0x100000, CRC(c680395d) SHA1(acde593a5ec501e89c8aaca6c4fbacf707a727e1) )
ROM_END

ROM_START( glfgreat )
	ROM_REGION( 0x40000, REGION_CPU1, 0 )
	ROM_LOAD16_BYTE( "061l02.1h",   0x000000, 0x20000, CRC(ac7399f4) SHA1(27f95bd41cb550ea0395a93138066896b834551e) )
	ROM_LOAD16_BYTE( "061l03.4h",   0x000001, 0x20000, CRC(77b0ff5c) SHA1(e47701402a9a6f69cfbc72de0fee4cbdd79fbc6e) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* 64k for the audio CPU */
	ROM_LOAD( "061f01.4e",    0x0000, 0x8000, CRC(ab9a2a57) SHA1(c92738b4d3754c2378cd1e6ae786faa0c5a65808) )

    ROM_REGION( 0x100000, REGION_GFX1, 0 )	/* graphics (addressable by the main CPU) */
	ROM_LOAD( "061d14.12l",   0x000000, 0x080000, CRC(b9440924) SHA1(d12763f1c999cfa4f2d6f685a73c8c20204f9cbb) )	/* tiles */
	ROM_LOAD( "061d13.12k",   0x080000, 0x080000, CRC(9f999f0b) SHA1(f83e3e9e44d7d5ba4c72f72db1ab9f98a0e80fe2) )

	ROM_REGION( 0x200000, REGION_GFX2, 0 )	/* graphics (addressable by the main CPU) */
	ROM_LOAD( "061d11.3k",    0x000000, 0x100000, CRC(c45b66a3) SHA1(bfb7f9a39d195857893d3f04c28d5c89442c3ac7) )	/* sprites */
	ROM_LOAD( "061d12.8k",    0x100000, 0x100000, CRC(d305ecd1) SHA1(28cba6b5eb56b6f5c01e9da341a5c0e2ed3cb407) )

	ROM_REGION( 0x180000, REGION_GFX3, 0 )	/* 053936 tiles */
	ROM_LOAD( "061b08.14g",   0x000000, 0x080000, CRC(6ab739c3) SHA1(37ed6c9b224189c183895517d6a72738fe92ecc4) )
	ROM_LOAD( "061b09.15g",   0x080000, 0x080000, CRC(42c7a603) SHA1(e98e484ca817ed65c7fb80a87d732e70d120676f) )
	ROM_LOAD( "061b10.17g",   0x100000, 0x080000, CRC(10f89ce7) SHA1(cf6a16ed0174db640780da4d11076efeb48a6119) )

	ROM_REGION( 0x120000, REGION_USER1, 0 )	/* 053936 tilemaps */
	ROM_LOAD( "061b07.18d",   0x000000, 0x080000, CRC(517887e2) SHA1(ff7aa0df2cda3c745a195879c71727352696ef3a) )
	ROM_LOAD( "061b06.16d",   0x080000, 0x080000, CRC(41ada2ad) SHA1(7b200e44e040e3d79f2603a02c9991b4655407d4) )
	ROM_LOAD( "061b05.15d",   0x100000, 0x020000, CRC(2456fb11) SHA1(e1bdb9f5983751d28addad6977a44df3d9899a14) )

	ROM_REGION( 0x100000, REGION_SOUND1, 0 )	/* samples for the 053260 */
	ROM_LOAD( "061e04.1d",    0x0000, 0x100000, CRC(7921d8df) SHA1(19ca4850ec489cca245e90a41bfc22493cd52263) )
ROM_END

ROM_START( glfgretj )
	ROM_REGION( 0x40000, REGION_CPU1, 0 )
	ROM_LOAD16_BYTE( "061j02.1h",   0x000000, 0x20000, CRC(7f0d95f4) SHA1(20b66cb07ca350dcc11d781511d04988bcff9019) )
	ROM_LOAD16_BYTE( "061j03.4h",   0x000001, 0x20000, CRC(06caa38b) SHA1(95a08133f6b025db5f50f528aad480af579ebe3d) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* 64k for the audio CPU */
	ROM_LOAD( "061f01.4e",    0x0000, 0x8000, CRC(ab9a2a57) SHA1(c92738b4d3754c2378cd1e6ae786faa0c5a65808) )

    ROM_REGION( 0x100000, REGION_GFX1, 0 )	/* graphics (addressable by the main CPU) */
	ROM_LOAD( "061d14.12l",   0x000000, 0x080000, CRC(b9440924) SHA1(d12763f1c999cfa4f2d6f685a73c8c20204f9cbb) )	/* tiles */
	ROM_LOAD( "061d13.12k",   0x080000, 0x080000, CRC(9f999f0b) SHA1(f83e3e9e44d7d5ba4c72f72db1ab9f98a0e80fe2) )

	ROM_REGION( 0x200000, REGION_GFX2, 0 )	/* graphics (addressable by the main CPU) */
	ROM_LOAD( "061d11.3k",    0x000000, 0x100000, CRC(c45b66a3) SHA1(bfb7f9a39d195857893d3f04c28d5c89442c3ac7) )	/* sprites */
	ROM_LOAD( "061d12.8k",    0x100000, 0x100000, CRC(d305ecd1) SHA1(28cba6b5eb56b6f5c01e9da341a5c0e2ed3cb407) )

	ROM_REGION( 0x180000, REGION_GFX3, 0 )	/* 053936 tiles */
	ROM_LOAD( "061b08.14g",   0x000000, 0x080000, CRC(6ab739c3) SHA1(37ed6c9b224189c183895517d6a72738fe92ecc4) )
	ROM_LOAD( "061b09.15g",   0x080000, 0x080000, CRC(42c7a603) SHA1(e98e484ca817ed65c7fb80a87d732e70d120676f) )
	ROM_LOAD( "061b10.17g",   0x100000, 0x080000, CRC(10f89ce7) SHA1(cf6a16ed0174db640780da4d11076efeb48a6119) )

	ROM_REGION( 0x120000, REGION_USER1, 0 )	/* 053936 tilemaps */
	ROM_LOAD( "061b07.18d",   0x000000, 0x080000, CRC(517887e2) SHA1(ff7aa0df2cda3c745a195879c71727352696ef3a) )
	ROM_LOAD( "061b06.16d",   0x080000, 0x080000, CRC(41ada2ad) SHA1(7b200e44e040e3d79f2603a02c9991b4655407d4) )
	ROM_LOAD( "061b05.15d",   0x100000, 0x020000, CRC(2456fb11) SHA1(e1bdb9f5983751d28addad6977a44df3d9899a14) )

	ROM_REGION( 0x100000, REGION_SOUND1, 0 )	/* samples for the 053260 */
	ROM_LOAD( "061e04.1d",    0x0000, 0x100000, CRC(7921d8df) SHA1(19ca4850ec489cca245e90a41bfc22493cd52263) )
ROM_END

ROM_START( tmnt2 )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_BYTE( "uaa02", 0x000000, 0x20000, CRC(58d5c93d) SHA1(6618678ec2da33d2ee6335cca7c9d49e9148b799) )
	ROM_LOAD16_BYTE( "uaa03", 0x000001, 0x20000, CRC(0541fec9) SHA1(985364616a95e7dd008b5be02c0f0bf5eef54b3d) )
	ROM_LOAD16_BYTE( "uaa04", 0x040000, 0x20000, CRC(1d441a7d) SHA1(97ce51eaf1c7560c19d8453f93ce01b0f71fe36d) )
	ROM_LOAD16_BYTE( "uaa05", 0x040001, 0x20000, CRC(9c428273) SHA1(92202b6061313e464c2d9760926852b833994d28) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* 64k for the audio CPU */
	ROM_LOAD( "b01",          0x0000, 0x10000, CRC(364f548a) SHA1(e0636e27d4fc48b2ccb1417b63d2b68d9e272c06) )

    ROM_REGION( 0x100000, REGION_GFX1, 0 )	/* graphics (addressable by the main CPU) */
	ROM_LOAD( "b12",          0x000000, 0x080000, CRC(d3283d19) SHA1(49e4daa9cbe4d99bf71fcee6237cb434a0d55312) )	/* tiles */
	ROM_LOAD( "b11",          0x080000, 0x080000, CRC(6ebc0c15) SHA1(e6848405076937fbf8ec6d318293a0ff922725f4) )

	ROM_REGION( 0x400000, REGION_GFX2, 0 )	/* graphics (addressable by the main CPU) */
	ROM_LOAD( "b09",          0x000000, 0x100000, CRC(2d7a9d2a) SHA1(a26f9c1a07152bc8c7bcd797d4485bf848f5e2a0) )	/* sprites */
	ROM_LOAD( "b10",          0x100000, 0x080000, CRC(f2dd296e) SHA1(a2aad10bfb0904dd73c2ee11049648c94de7f4d5) )
	/* second half empty */
	ROM_LOAD( "b07",          0x200000, 0x100000, CRC(d9bee7bf) SHA1(7bbb65138fbd216b80412783e6f0072742101440) )
	ROM_LOAD( "b08",          0x300000, 0x080000, CRC(3b1ae36f) SHA1(9e69cae8b517497ac77c4d148f56f2bb6a23de89) )
	/* second half empty */

	ROM_REGION( 0x200000, REGION_SOUND1, 0 )	/* samples for the 053260 */
	ROM_LOAD( "063b06",       0x0000, 0x200000, CRC(1e510aa5) SHA1(02b9bd6bb6b098026a620e4d671c40a31ad9e318) )
ROM_END

ROM_START( tmnt22p )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_BYTE( "a02",   0x000000, 0x20000, CRC(aadffe3a) SHA1(f20eaef64f81b91726675006aa45807b0841f046) )
	ROM_LOAD16_BYTE( "a03",   0x000001, 0x20000, CRC(125687a8) SHA1(ab8eb954a56cbb18a26af3431aa8d60406ef23b5) )
	ROM_LOAD16_BYTE( "a04",   0x040000, 0x20000, CRC(fb5c7ded) SHA1(322ec2a4a6a2ecea0865bc72b6c1d23e52da33da) )
	ROM_LOAD16_BYTE( "a05",   0x040001, 0x20000, CRC(3c40fe66) SHA1(d2d1f24bf8ab44d24478f021f0b651095f623860) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* 64k for the audio CPU */
	ROM_LOAD( "b01",          0x0000, 0x10000, CRC(364f548a) SHA1(e0636e27d4fc48b2ccb1417b63d2b68d9e272c06) )

    ROM_REGION( 0x100000, REGION_GFX1, 0 )	/* graphics (addressable by the main CPU) */
	ROM_LOAD( "b12",          0x000000, 0x080000, CRC(d3283d19) SHA1(49e4daa9cbe4d99bf71fcee6237cb434a0d55312) )	/* tiles */
	ROM_LOAD( "b11",          0x080000, 0x080000, CRC(6ebc0c15) SHA1(e6848405076937fbf8ec6d318293a0ff922725f4) )

	ROM_REGION( 0x400000, REGION_GFX2, 0 )	/* graphics (addressable by the main CPU) */
	ROM_LOAD( "b09",          0x000000, 0x100000, CRC(2d7a9d2a) SHA1(a26f9c1a07152bc8c7bcd797d4485bf848f5e2a0) )	/* sprites */
	ROM_LOAD( "b10",          0x100000, 0x080000, CRC(f2dd296e) SHA1(a2aad10bfb0904dd73c2ee11049648c94de7f4d5) )
	/* second half empty */
	ROM_LOAD( "b07",          0x200000, 0x100000, CRC(d9bee7bf) SHA1(7bbb65138fbd216b80412783e6f0072742101440) )
	ROM_LOAD( "b08",          0x300000, 0x080000, CRC(3b1ae36f) SHA1(9e69cae8b517497ac77c4d148f56f2bb6a23de89) )
	/* second half empty */

	ROM_REGION( 0x200000, REGION_SOUND1, 0 )	/* samples for the 053260 */
	ROM_LOAD( "063b06",       0x0000, 0x200000, CRC(1e510aa5) SHA1(02b9bd6bb6b098026a620e4d671c40a31ad9e318) )
ROM_END

ROM_START( tmnt2a )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_BYTE( "ada02", 0x000000, 0x20000, CRC(4f11b587) SHA1(111051da23ce7035405b4d12c0f18dcc1d6c8ddc) )
	ROM_LOAD16_BYTE( "ada03", 0x000001, 0x20000, CRC(82a1b9ac) SHA1(161e8fd33e0e5c9349fec98b02225ed37578e488) )
	ROM_LOAD16_BYTE( "ada04", 0x040000, 0x20000, CRC(05ad187a) SHA1(27a36a02ef792d87ffa2364537c42b6c50d6e4f0) )
	ROM_LOAD16_BYTE( "ada05", 0x040001, 0x20000, CRC(d4826547) SHA1(ffee07be64469fa386a0979352b4fe20c352fee4) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* 64k for the audio CPU */
	ROM_LOAD( "b01",          0x0000, 0x10000, CRC(364f548a) SHA1(e0636e27d4fc48b2ccb1417b63d2b68d9e272c06) )

    ROM_REGION( 0x100000, REGION_GFX1, 0 )	/* graphics (addressable by the main CPU) */
	ROM_LOAD( "b12",          0x000000, 0x080000, CRC(d3283d19) SHA1(49e4daa9cbe4d99bf71fcee6237cb434a0d55312) )	/* tiles */
	ROM_LOAD( "b11",          0x080000, 0x080000, CRC(6ebc0c15) SHA1(e6848405076937fbf8ec6d318293a0ff922725f4) )

	ROM_REGION( 0x400000, REGION_GFX2, 0 )	/* graphics (addressable by the main CPU) */
	ROM_LOAD( "b09",          0x000000, 0x100000, CRC(2d7a9d2a) SHA1(a26f9c1a07152bc8c7bcd797d4485bf848f5e2a0) )	/* sprites */
	ROM_LOAD( "b10",          0x100000, 0x080000, CRC(f2dd296e) SHA1(a2aad10bfb0904dd73c2ee11049648c94de7f4d5) )
	/* second half empty */
	ROM_LOAD( "b07",          0x200000, 0x100000, CRC(d9bee7bf) SHA1(7bbb65138fbd216b80412783e6f0072742101440) )
	ROM_LOAD( "b08",          0x300000, 0x080000, CRC(3b1ae36f) SHA1(9e69cae8b517497ac77c4d148f56f2bb6a23de89) )
	/* second half empty */

	ROM_REGION( 0x200000, REGION_SOUND1, 0 )	/* samples for the 053260 */
	ROM_LOAD( "063b06",       0x0000, 0x200000, CRC(1e510aa5) SHA1(02b9bd6bb6b098026a620e4d671c40a31ad9e318) )
ROM_END

ROM_START( qgakumon )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_BYTE( "248jaa02.8e",  0x000000, 0x40000, CRC(fab79410) SHA1(8b1a8946ee65505608cf026c9fca87365ccef089) )
	ROM_LOAD16_BYTE( "248jaa03.8g",  0x000001, 0x40000, CRC(8d888ef3) SHA1(1ef2636620abff8e3fe0258c90c5c8c0bf33f2d5) )
	ROM_LOAD16_BYTE( "248jaa04.10e", 0x080000, 0x40000, CRC(56cb16cb) SHA1(a659229b43fba59c055e1da061fbfb19ecbb5c24) )
	ROM_LOAD16_BYTE( "248jaa05.10g", 0x080001, 0x40000, CRC(27614fcd) SHA1(c44d1dd3f16914f9616d6370098eaf6fa8a44542) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* 64k for the audio CPU */
	ROM_LOAD( "248a01.2f",          0x0000, 0x10000, CRC(a8a41cc6) SHA1(ad0d73bbdaacb8d5d0c7971ec4357eec665ee7cf) )

    ROM_REGION( 0x100000, REGION_GFX1, 0 )	/* graphics (addressable by the main CPU) */
	ROM_LOAD( "248a12.16k",          0x000000, 0x080000, CRC(62870987) SHA1(f502c44781a077590038dcca9bf76c8a047169be) )	/* tiles */
	ROM_LOAD( "248a11.12k",          0x080000, 0x080000, CRC(fad2dbfd) SHA1(a6cc9a612467c43ae4194f71b43a442272f0fde1) )

	ROM_REGION( 0x400000, REGION_GFX2, 0 )	/* graphics (addressable by the main CPU) */
	ROM_LOAD( "248a09.7l",          0x000000, 0x100000, CRC(a176e205) SHA1(e0b2176a1525711c6e692f88a913f57b9bdd0046) )	/* sprites */
	ROM_LOAD( "248a07.3l",          0x200000, 0x100000, CRC(9595589f) SHA1(3e48f66448577a8fa39b6707e89c2267152b6f0b) )

	ROM_REGION( 0x200000, REGION_SOUND1, 0 )	/* samples for the 053260 */
	ROM_LOAD( "248a06.1d",       0x0000, 0x200000, CRC(0fba1def) SHA1(f2ba23213effd06f14c7a179acea974c78c2198f) )
ROM_END

ROM_START( ssriders )
	ROM_REGION( 0xc0000, REGION_CPU1, 0 )
	ROM_LOAD16_BYTE( "064eac02",    0x000000, 0x40000, CRC(5a5425f4) SHA1(213226558d772f3ae573ee851b881536ce2faa2a) )
	ROM_LOAD16_BYTE( "064eac03",    0x000001, 0x40000, CRC(093c00fb) SHA1(208a3688504bad3bc23135ceb0f15226dd98558e) )
	ROM_LOAD16_BYTE( "sr_b04.rom",  0x080000, 0x20000, CRC(ef2315bd) SHA1(2c8b11321cb5fdb78d760fabca666c0d8cc5b298) )
	ROM_LOAD16_BYTE( "sr_b05.rom",  0x080001, 0x20000, CRC(51d6fbc4) SHA1(e80de7d155b7f263c48ef4ae2702059be3c18e76) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* 64k for the audio CPU */
	ROM_LOAD( "sr_e01.rom",   0x0000, 0x10000, CRC(44b9bc52) SHA1(4654d6e14c6956c40a19cb41155accb63f0da338) )

    ROM_REGION( 0x100000, REGION_GFX1, 0 )	/* graphics (addressable by the main CPU) */
	ROM_LOAD( "sr_16k.rom",   0x000000, 0x080000, CRC(e2bdc619) SHA1(04449deb267b0beacfa33640b593eb16194aa0d9) )	/* tiles */
	ROM_LOAD( "sr_12k.rom",   0x080000, 0x080000, CRC(2d8ca8b0) SHA1(7c882f79c2402cf75979c681071007d76e4db9ae) )

	ROM_REGION( 0x200000, REGION_GFX2, 0 )	/* graphics (addressable by the main CPU) */
	ROM_LOAD( "sr_7l.rom",    0x000000, 0x100000, CRC(4160c372) SHA1(0b36181e5ccd785c7fb89b9f41e458066a42c3b0) )	/* sprites */
	ROM_LOAD( "sr_3l.rom",    0x100000, 0x100000, CRC(64dd673c) SHA1(bea4d17a71dd21c635866ee69b4892dc9d0ab455) )

	ROM_REGION( 0x100000, REGION_SOUND1, 0 )	/* samples for the 053260 */
	ROM_LOAD( "sr_1d.rom",    0x0000, 0x100000, CRC(59810df9) SHA1(a0affc6330bdbfab1447dc0cf13c20ff708c2c71) )
ROM_END

ROM_START( ssrdrebd )
	ROM_REGION( 0xc0000, REGION_CPU1, 0 )
	ROM_LOAD16_BYTE( "064ebd02",    0x000000, 0x40000, CRC(8deef9ac) SHA1(406ef2b022a59ed958674f432ed50f6ed37fd3c4) )
	ROM_LOAD16_BYTE( "064ebd03",    0x000001, 0x40000, CRC(2370c107) SHA1(85d2bd8dde928f647a5d34ac98d2df2ed559f7a2) )
	ROM_LOAD16_BYTE( "sr_b04.rom",  0x080000, 0x20000, CRC(ef2315bd) SHA1(2c8b11321cb5fdb78d760fabca666c0d8cc5b298) )
	ROM_LOAD16_BYTE( "sr_b05.rom",  0x080001, 0x20000, CRC(51d6fbc4) SHA1(e80de7d155b7f263c48ef4ae2702059be3c18e76) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* 64k for the audio CPU */
	ROM_LOAD( "sr_e01.rom",   0x0000, 0x10000, CRC(44b9bc52) SHA1(4654d6e14c6956c40a19cb41155accb63f0da338) )

    ROM_REGION( 0x100000, REGION_GFX1, 0 )	/* graphics (addressable by the main CPU) */
	ROM_LOAD( "sr_16k.rom",   0x000000, 0x080000, CRC(e2bdc619) SHA1(04449deb267b0beacfa33640b593eb16194aa0d9) )	/* tiles */
	ROM_LOAD( "sr_12k.rom",   0x080000, 0x080000, CRC(2d8ca8b0) SHA1(7c882f79c2402cf75979c681071007d76e4db9ae) )

	ROM_REGION( 0x200000, REGION_GFX2, 0 )	/* graphics (addressable by the main CPU) */
	ROM_LOAD( "sr_7l.rom",    0x000000, 0x100000, CRC(4160c372) SHA1(0b36181e5ccd785c7fb89b9f41e458066a42c3b0) )	/* sprites */
	ROM_LOAD( "sr_3l.rom",    0x100000, 0x100000, CRC(64dd673c) SHA1(bea4d17a71dd21c635866ee69b4892dc9d0ab455) )

	ROM_REGION( 0x100000, REGION_SOUND1, 0 )	/* samples for the 053260 */
	ROM_LOAD( "sr_1d.rom",    0x0000, 0x100000, CRC(59810df9) SHA1(a0affc6330bdbfab1447dc0cf13c20ff708c2c71) )
ROM_END

ROM_START( ssrdrebc )
	ROM_REGION( 0xc0000, REGION_CPU1, 0 )
	ROM_LOAD16_BYTE( "sr_c02.rom",  0x000000, 0x40000, CRC(9bd7d164) SHA1(492abdaf62fe7cb72b7e53076a05c987503c738a) )
	ROM_LOAD16_BYTE( "sr_c03.rom",  0x000001, 0x40000, CRC(40fd4165) SHA1(c30d7560aae6e9f0bebe2d6d3e0e11b56634de0c) )
	ROM_LOAD16_BYTE( "sr_b04.rom",  0x080000, 0x20000, CRC(ef2315bd) SHA1(2c8b11321cb5fdb78d760fabca666c0d8cc5b298) )
	ROM_LOAD16_BYTE( "sr_b05.rom",  0x080001, 0x20000, CRC(51d6fbc4) SHA1(e80de7d155b7f263c48ef4ae2702059be3c18e76) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* 64k for the audio CPU */
	ROM_LOAD( "sr_e01.rom",   0x0000, 0x10000, CRC(44b9bc52) SHA1(4654d6e14c6956c40a19cb41155accb63f0da338) )

    ROM_REGION( 0x100000, REGION_GFX1, 0 )	/* graphics (addressable by the main CPU) */
	ROM_LOAD( "sr_16k.rom",   0x000000, 0x080000, CRC(e2bdc619) SHA1(04449deb267b0beacfa33640b593eb16194aa0d9) )	/* tiles */
	ROM_LOAD( "sr_12k.rom",   0x080000, 0x080000, CRC(2d8ca8b0) SHA1(7c882f79c2402cf75979c681071007d76e4db9ae) )

	ROM_REGION( 0x200000, REGION_GFX2, 0 )	/* graphics (addressable by the main CPU) */
	ROM_LOAD( "sr_7l.rom",    0x000000, 0x100000, CRC(4160c372) SHA1(0b36181e5ccd785c7fb89b9f41e458066a42c3b0) )	/* sprites */
	ROM_LOAD( "sr_3l.rom",    0x100000, 0x100000, CRC(64dd673c) SHA1(bea4d17a71dd21c635866ee69b4892dc9d0ab455) )

	ROM_REGION( 0x100000, REGION_SOUND1, 0 )	/* samples for the 053260 */
	ROM_LOAD( "sr_1d.rom",    0x0000, 0x100000, CRC(59810df9) SHA1(a0affc6330bdbfab1447dc0cf13c20ff708c2c71) )
ROM_END

ROM_START( ssrdruda )
	ROM_REGION( 0xc0000, REGION_CPU1, 0 )
	ROM_LOAD16_BYTE( "064uda02",    0x000000, 0x40000, CRC(5129a6b7) SHA1(8892d7043e7b0aee9eaffde9fa9bfd9bbfb7f15f) )
	ROM_LOAD16_BYTE( "064uda03",    0x000001, 0x40000, CRC(9f887214) SHA1(f5e22230b7dca42242f3f244e45e67a4bbbdb65f) )
	ROM_LOAD16_BYTE( "sr_b04.rom",  0x080000, 0x20000, CRC(ef2315bd) SHA1(2c8b11321cb5fdb78d760fabca666c0d8cc5b298) )
	ROM_LOAD16_BYTE( "sr_b05.rom",  0x080001, 0x20000, CRC(51d6fbc4) SHA1(e80de7d155b7f263c48ef4ae2702059be3c18e76) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* 64k for the audio CPU */
	ROM_LOAD( "sr_e01.rom",   0x0000, 0x10000, CRC(44b9bc52) SHA1(4654d6e14c6956c40a19cb41155accb63f0da338) )

    ROM_REGION( 0x100000, REGION_GFX1, 0 )	/* graphics (addressable by the main CPU) */
	ROM_LOAD( "sr_16k.rom",   0x000000, 0x080000, CRC(e2bdc619) SHA1(04449deb267b0beacfa33640b593eb16194aa0d9) )	/* tiles */
	ROM_LOAD( "sr_12k.rom",   0x080000, 0x080000, CRC(2d8ca8b0) SHA1(7c882f79c2402cf75979c681071007d76e4db9ae) )

	ROM_REGION( 0x200000, REGION_GFX2, 0 )	/* graphics (addressable by the main CPU) */
	ROM_LOAD( "sr_7l.rom",    0x000000, 0x100000, CRC(4160c372) SHA1(0b36181e5ccd785c7fb89b9f41e458066a42c3b0) )	/* sprites */
	ROM_LOAD( "sr_3l.rom",    0x100000, 0x100000, CRC(64dd673c) SHA1(bea4d17a71dd21c635866ee69b4892dc9d0ab455) )

	ROM_REGION( 0x100000, REGION_SOUND1, 0 )	/* samples for the 053260 */
	ROM_LOAD( "sr_1d.rom",    0x0000, 0x100000, CRC(59810df9) SHA1(a0affc6330bdbfab1447dc0cf13c20ff708c2c71) )
ROM_END

ROM_START( ssrdruac )
	ROM_REGION( 0xc0000, REGION_CPU1, 0 )
	ROM_LOAD16_BYTE( "064uac02",    0x000000, 0x40000, CRC(870473b6) SHA1(2e2fd5c6df3fa8da6655699043e08a8f918ef63c) )
	ROM_LOAD16_BYTE( "064uac03",    0x000001, 0x40000, CRC(eadf289a) SHA1(824230714ae0c1d065e83719bb344e76a5ca1fba) )
	ROM_LOAD16_BYTE( "sr_b04.rom",  0x080000, 0x20000, CRC(ef2315bd) SHA1(2c8b11321cb5fdb78d760fabca666c0d8cc5b298) )
	ROM_LOAD16_BYTE( "sr_b05.rom",  0x080001, 0x20000, CRC(51d6fbc4) SHA1(e80de7d155b7f263c48ef4ae2702059be3c18e76) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* 64k for the audio CPU */
	ROM_LOAD( "sr_e01.rom",   0x0000, 0x10000, CRC(44b9bc52) SHA1(4654d6e14c6956c40a19cb41155accb63f0da338) )

    ROM_REGION( 0x100000, REGION_GFX1, 0 )	/* graphics (addressable by the main CPU) */
	ROM_LOAD( "sr_16k.rom",   0x000000, 0x080000, CRC(e2bdc619) SHA1(04449deb267b0beacfa33640b593eb16194aa0d9) )	/* tiles */
	ROM_LOAD( "sr_12k.rom",   0x080000, 0x080000, CRC(2d8ca8b0) SHA1(7c882f79c2402cf75979c681071007d76e4db9ae) )

	ROM_REGION( 0x200000, REGION_GFX2, 0 )	/* graphics (addressable by the main CPU) */
	ROM_LOAD( "sr_7l.rom",    0x000000, 0x100000, CRC(4160c372) SHA1(0b36181e5ccd785c7fb89b9f41e458066a42c3b0) )	/* sprites */
	ROM_LOAD( "sr_3l.rom",    0x100000, 0x100000, CRC(64dd673c) SHA1(bea4d17a71dd21c635866ee69b4892dc9d0ab455) )

	ROM_REGION( 0x100000, REGION_SOUND1, 0 )	/* samples for the 053260 */
	ROM_LOAD( "sr_1d.rom",    0x0000, 0x100000, CRC(59810df9) SHA1(a0affc6330bdbfab1447dc0cf13c20ff708c2c71) )
ROM_END

ROM_START( ssrdrubc )
	ROM_REGION( 0xc0000, REGION_CPU1, 0 )
	ROM_LOAD16_BYTE( "2pl.8e",      0x000000, 0x40000, CRC(aca7fda5) SHA1(318fdefbea70724e95f2537b1915bc3a7abbb644) )
	ROM_LOAD16_BYTE( "2pl.8g",      0x000001, 0x40000, CRC(bb1fdeff) SHA1(1b74954258e3e8fdc80dd3c27785c945e57d36f8) )
	ROM_LOAD16_BYTE( "sr_b04.rom",  0x080000, 0x20000, CRC(ef2315bd) SHA1(2c8b11321cb5fdb78d760fabca666c0d8cc5b298) )
	ROM_LOAD16_BYTE( "sr_b05.rom",  0x080001, 0x20000, CRC(51d6fbc4) SHA1(e80de7d155b7f263c48ef4ae2702059be3c18e76) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* 64k for the audio CPU */
	ROM_LOAD( "sr_e01.rom",   0x0000, 0x10000, CRC(44b9bc52) SHA1(4654d6e14c6956c40a19cb41155accb63f0da338) )

    ROM_REGION( 0x100000, REGION_GFX1, 0 )	/* graphics (addressable by the main CPU) */
	ROM_LOAD( "sr_16k.rom",   0x000000, 0x080000, CRC(e2bdc619) SHA1(04449deb267b0beacfa33640b593eb16194aa0d9) )	/* tiles */
	ROM_LOAD( "sr_12k.rom",   0x080000, 0x080000, CRC(2d8ca8b0) SHA1(7c882f79c2402cf75979c681071007d76e4db9ae) )

	ROM_REGION( 0x200000, REGION_GFX2, 0 )	/* graphics (addressable by the main CPU) */
	ROM_LOAD( "sr_7l.rom",    0x000000, 0x100000, CRC(4160c372) SHA1(0b36181e5ccd785c7fb89b9f41e458066a42c3b0) )	/* sprites */
	ROM_LOAD( "sr_3l.rom",    0x100000, 0x100000, CRC(64dd673c) SHA1(bea4d17a71dd21c635866ee69b4892dc9d0ab455) )

	ROM_REGION( 0x100000, REGION_SOUND1, 0 )	/* samples for the 053260 */
	ROM_LOAD( "sr_1d.rom",    0x0000, 0x100000, CRC(59810df9) SHA1(a0affc6330bdbfab1447dc0cf13c20ff708c2c71) )
ROM_END

ROM_START( ssrdrabd )
	ROM_REGION( 0xc0000, REGION_CPU1, 0 )
	ROM_LOAD16_BYTE( "064abd02.8e", 0x000000, 0x40000, CRC(713406cb) SHA1(23769413bfce6cf7039437d0fa25a7b4b9c86387) )
	ROM_LOAD16_BYTE( "064abd03.8g", 0x000001, 0x40000, CRC(680feb3c) SHA1(379082cccdbc579a88afcf771f6deb64e4baf4d6) )
	ROM_LOAD16_BYTE( "sr_b04.rom",  0x080000, 0x20000, CRC(ef2315bd) SHA1(2c8b11321cb5fdb78d760fabca666c0d8cc5b298) )
	ROM_LOAD16_BYTE( "sr_b05.rom",  0x080001, 0x20000, CRC(51d6fbc4) SHA1(e80de7d155b7f263c48ef4ae2702059be3c18e76) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* 64k for the audio CPU */
	ROM_LOAD( "sr_e01.rom",   0x0000, 0x10000, CRC(44b9bc52) SHA1(4654d6e14c6956c40a19cb41155accb63f0da338) )

    ROM_REGION( 0x100000, REGION_GFX1, 0 )	/* graphics (addressable by the main CPU) */
	ROM_LOAD( "sr_16k.rom",   0x000000, 0x080000, CRC(e2bdc619) SHA1(04449deb267b0beacfa33640b593eb16194aa0d9) )	/* tiles */
	ROM_LOAD( "sr_12k.rom",   0x080000, 0x080000, CRC(2d8ca8b0) SHA1(7c882f79c2402cf75979c681071007d76e4db9ae) )

	ROM_REGION( 0x200000, REGION_GFX2, 0 )	/* graphics (addressable by the main CPU) */
	ROM_LOAD( "sr_7l.rom",    0x000000, 0x100000, CRC(4160c372) SHA1(0b36181e5ccd785c7fb89b9f41e458066a42c3b0) )	/* sprites */
	ROM_LOAD( "sr_3l.rom",    0x100000, 0x100000, CRC(64dd673c) SHA1(bea4d17a71dd21c635866ee69b4892dc9d0ab455) )

	ROM_REGION( 0x100000, REGION_SOUND1, 0 )	/* samples for the 053260 */
	ROM_LOAD( "sr_1d.rom",    0x0000, 0x100000, CRC(59810df9) SHA1(a0affc6330bdbfab1447dc0cf13c20ff708c2c71) )
ROM_END

ROM_START( ssrdrjbd )
	ROM_REGION( 0xc0000, REGION_CPU1, 0 )
	ROM_LOAD16_BYTE( "064jbd02.8e", 0x000000, 0x40000, CRC(7acdc1e3) SHA1(09679403abe695758d01fb0161168bc93888f915) )
	ROM_LOAD16_BYTE( "064jbd03.8g", 0x000001, 0x40000, CRC(6a424918) SHA1(3e7a66adc934b1ed4ecd75a36d5a1c133916ac66) )
	ROM_LOAD16_BYTE( "sr_b04.rom",  0x080000, 0x20000, CRC(ef2315bd) SHA1(2c8b11321cb5fdb78d760fabca666c0d8cc5b298) )
	ROM_LOAD16_BYTE( "sr_b05.rom",  0x080001, 0x20000, CRC(51d6fbc4) SHA1(e80de7d155b7f263c48ef4ae2702059be3c18e76) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* 64k for the audio CPU */
	ROM_LOAD( "sr_e01.rom",   0x0000, 0x10000, CRC(44b9bc52) SHA1(4654d6e14c6956c40a19cb41155accb63f0da338) )

    ROM_REGION( 0x100000, REGION_GFX1, 0 )	/* graphics (addressable by the main CPU) */
	ROM_LOAD( "sr_16k.rom",   0x000000, 0x080000, CRC(e2bdc619) SHA1(04449deb267b0beacfa33640b593eb16194aa0d9) )	/* tiles */
	ROM_LOAD( "sr_12k.rom",   0x080000, 0x080000, CRC(2d8ca8b0) SHA1(7c882f79c2402cf75979c681071007d76e4db9ae) )

	ROM_REGION( 0x200000, REGION_GFX2, 0 )	/* graphics (addressable by the main CPU) */
	ROM_LOAD( "sr_7l.rom",    0x000000, 0x100000, CRC(4160c372) SHA1(0b36181e5ccd785c7fb89b9f41e458066a42c3b0) )	/* sprites */
	ROM_LOAD( "sr_3l.rom",    0x100000, 0x100000, CRC(64dd673c) SHA1(bea4d17a71dd21c635866ee69b4892dc9d0ab455) )

	ROM_REGION( 0x100000, REGION_SOUND1, 0 )	/* samples for the 053260 */
	ROM_LOAD( "sr_1d.rom",    0x0000, 0x100000, CRC(59810df9) SHA1(a0affc6330bdbfab1447dc0cf13c20ff708c2c71) )
ROM_END

ROM_START( sunsetbl )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_WORD_SWAP( "sunsetb.03",   0x000000, 0x080000, CRC(37ffe90b) SHA1(3f8542243f2a0c0718056672a906b70af5894a86) )
	ROM_LOAD16_WORD_SWAP( "sunsetb.04",   0x080000, 0x080000, CRC(8ff647b7) SHA1(75144ce928fc4e7d24d9dd50a93e11ea41903bc4) )

	ROM_REGION( 0x100000, REGION_GFX1, 0 )	/* graphics (addressable by the main CPU) */
	/* should be sunsetb.09 and .10 from the bootleg, but .09 is a bad dump and .10 matches the parent's sr_12k, so we just use the parent's roms*/
	ROM_LOAD( "sr_16k.rom",   0x000000, 0x080000, CRC(e2bdc619) SHA1(04449deb267b0beacfa33640b593eb16194aa0d9) )	/* tiles */
	ROM_LOAD( "sr_12k.rom",   0x080000, 0x080000, CRC(2d8ca8b0) SHA1(7c882f79c2402cf75979c681071007d76e4db9ae) )

	ROM_REGION( 0x200000, REGION_GFX2, 0 )	/* graphics (addressable by the main CPU) */
	ROM_LOAD( "sunsetb.05",   0x000000, 0x080000, CRC(8a0ff31a) SHA1(fee21d787d1cddd04713e10b1622f3fa231ebc4e) )
	ROM_LOAD( "sunsetb.06",   0x080000, 0x080000, CRC(fdf2c887) SHA1(a165c7e6495d870324f59262ad4175a039e199a5) )
	ROM_LOAD( "sunsetb.07",   0x100000, 0x080000, CRC(a545b1ed) SHA1(249f1f1a992f05c0dc23bd52785a355a402a0d10) )
	ROM_LOAD( "sunsetb.08",   0x180000, 0x080000, CRC(f867cd38) SHA1(633703474010364dc47176965daa873d548da074) )

	ROM_REGION( 0x100000, REGION_SOUND1, 0 )	/* samples */
	ROM_LOAD( "sunsetb.01",   0x000000, 0x080000, CRC(1a8b5ca2) SHA1(4101686c7bf3243273a52fca046b252fc3c78721) )
	ROM_LOAD( "sunsetb.02",   0x080000, 0x080000, CRC(5d485523) SHA1(478119cb6273d870ca04a66e9b964ca0424f6fbd) )
ROM_END

ROM_START( thndrx2 )
	ROM_REGION( 0x40000, REGION_CPU1, 0 )
	ROM_LOAD16_BYTE( "073-k02.11c", 0x000000, 0x20000, CRC(0c8b2d3f) SHA1(44ca5d96d8f85ae2760df4e1c339916e0a76143f) )
	ROM_LOAD16_BYTE( "073-k03.12c", 0x000001, 0x20000, CRC(3803b427) SHA1(95b755c70ac55af604c6b44bc41b761efce19f48) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* 64k for the audio CPU */
	ROM_LOAD( "073-c01.4f",   0x0000, 0x10000, CRC(44ebe83c) SHA1(9274df6affa4f0456d273ff3aa1bda7d2a20416e) )

    ROM_REGION( 0x100000, REGION_GFX1, 0 )	/* graphics (addressable by the main CPU) */
	ROM_LOAD( "073-c06.16k",  0x000000, 0x080000, CRC(24e22b42) SHA1(7e5e14495bd4adbe5d1cbec75262c9c4c83f5793) )	/* tiles */
	ROM_LOAD( "073-c05.12k",  0x080000, 0x080000, CRC(952a935f) SHA1(87ed81616a243d679f7501db7acdd8b6617f85a3) )

	ROM_REGION( 0x100000, REGION_GFX2, 0 )	/* graphics (addressable by the main CPU) */
	ROM_LOAD( "073-c07.7k",   0x000000, 0x080000, CRC(14e93f38) SHA1(bf111b68be722c9c2f0f9c7700b3af6cd8fd28be) )	/* sprites */
	ROM_LOAD( "073-c08.3k",   0x080000, 0x080000, CRC(09fab3ab) SHA1(af54c7bfe8edc5b5ea2c4fba4d5c637cfcbbeff5) )

	ROM_REGION( 0x80000, REGION_SOUND1, 0 )	/* samples for the 053260 */
	ROM_LOAD( "073-b04.2d",   0x0000, 0x80000, CRC(05287a0b) SHA1(10784b8be6a93a5ebf22a884f99c116e51ae8743) )
ROM_END

ROM_START( thndrx2a )
	ROM_REGION( 0x40000, REGION_CPU1, 0 )
	ROM_LOAD16_BYTE( "073-m02.11c", 0x000000, 0x20000, CRC(5b5b4cc0) SHA1(9f67169fba4523e2893e5ecf17b1be8cdedba83e) )
	ROM_LOAD16_BYTE( "073-m03.12c", 0x000001, 0x20000, CRC(320435a8) SHA1(5f656867049b614b0834ef6d8e36fe86118ea1cf) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* 64k for the audio CPU */
	ROM_LOAD( "073-c01.4f",   0x0000, 0x10000, CRC(44ebe83c) SHA1(9274df6affa4f0456d273ff3aa1bda7d2a20416e) )

    ROM_REGION( 0x100000, REGION_GFX1, 0 )	/* graphics (addressable by the main CPU) */
	ROM_LOAD( "073-c06.16k",  0x000000, 0x080000, CRC(24e22b42) SHA1(7e5e14495bd4adbe5d1cbec75262c9c4c83f5793) )	/* tiles */
	ROM_LOAD( "073-c05.12k",  0x080000, 0x080000, CRC(952a935f) SHA1(87ed81616a243d679f7501db7acdd8b6617f85a3) )

	ROM_REGION( 0x100000, REGION_GFX2, 0 )	/* graphics (addressable by the main CPU) */
	ROM_LOAD( "073-c07.7k",   0x000000, 0x080000, CRC(14e93f38) SHA1(bf111b68be722c9c2f0f9c7700b3af6cd8fd28be) )	/* sprites */
	ROM_LOAD( "073-c08.3k",   0x080000, 0x080000, CRC(09fab3ab) SHA1(af54c7bfe8edc5b5ea2c4fba4d5c637cfcbbeff5) )

	ROM_REGION( 0x80000, REGION_SOUND1, 0 )	/* samples for the 053260 */
	ROM_LOAD( "073-b04.2d",   0x0000, 0x80000, CRC(05287a0b) SHA1(10784b8be6a93a5ebf22a884f99c116e51ae8743) )
ROM_END


ROM_START( prmrsocr )
	ROM_REGION( 0x80000, REGION_CPU1, 0 )
	ROM_LOAD16_BYTE( "101eab08.1h", 0x000000, 0x40000, CRC(47208de6) SHA1(fe4ef56688d4a50f67a604357e7beea785106cd1) ) /* 3.bin*/
	ROM_LOAD16_BYTE( "101eab07.4h", 0x000001, 0x40000, CRC(5f408eca) SHA1(f2f6e126bfdcf884b477f49cb95f5e673357e9e0) ) /* 3.bin*/

	ROM_REGION( 0x30000, REGION_CPU2, 0 ) /* 64k for the audio CPU */
	ROM_LOAD( "101_c05.5e",   0x00000, 0x20000, CRC(02c3679f) SHA1(e6d878185e73baca24ac98891c647856be9353c4) ) /* 1.bin*/
	ROM_RELOAD(               0x10000, 0x20000 )

    ROM_REGION( 0x080000, REGION_GFX1, 0 )	/* graphics (addressable by the main CPU) */
	ROM_LOAD( "101a12.12l",   0x000000, 0x040000, CRC(33530d7f) SHA1(87859ad058fb79e357101675706373f83a3f23d4) )	/* tiles */
	ROM_LOAD( "101a11.12k",   0x040000, 0x040000, CRC(7f773271) SHA1(0c6a62c6eb1897e88e893576f751e3d4fc788036) )

	ROM_REGION( 0x400000, REGION_GFX2, 0 )	/* graphics (addressable by the main CPU) */
	ROM_LOAD( "101a09.3l",    0x000000, 0x200000, CRC(b6a1b424) SHA1(4cf7bb4b8176977dea10fb80fcd9d6e24cc6d1b9) )	/* sprites */
	ROM_LOAD( "101a10.8l",    0x200000, 0x200000, CRC(bbd58adc) SHA1(ad9bd4df995de6e6290f27c58c7892c7191802e4) )

	ROM_REGION( 0x080000, REGION_GFX3, ROMREGION_DISPOSE )	/* 053936 tiles */
	ROM_LOAD( "101a03.18f",   0x000000, 0x080000, CRC(59a1a91c) SHA1(f596a40784a671e97116df6561682eb6c5c44e08) )

	ROM_REGION( 0x040000, REGION_USER1, 0 )	/* 053936 tilemaps */
	ROM_LOAD( "101a01.18d",   0x000000, 0x020000, CRC(716f910f) SHA1(fbe69cac266084ea1efb094a7f863dca39f12500) )
	ROM_LOAD( "101a02.16d",   0x020000, 0x020000, CRC(222869c7) SHA1(0a9bea294ff3281f316dd4beecc4c94d75d52b49) )

	ROM_REGION( 0x200000, REGION_SOUND1, 0 )	/* samples for the 054539 */
	ROM_LOAD( "101a06.1d",    0x0000, 0x200000, CRC(4f48e043) SHA1(f50e8642d9d3a028c243777640e7cd13da1abf86) )
ROM_END

ROM_START( prmrsocj )
	ROM_REGION( 0x80000, REGION_CPU1, 0 )
	ROM_LOAD16_BYTE( "101jab08.1h", 0x000000, 0x40000, CRC(c22b528c) SHA1(6c96ba573f7bb5be1d52d9352f57d7a402bc96b4) )
	ROM_LOAD16_BYTE( "101jab07.4h", 0x000001, 0x40000, CRC(06e7acaf) SHA1(d7197bb1c3b28cbe82dd4e25302e00f7c1838208) )

	ROM_REGION( 0x30000, REGION_CPU2, 0 ) /* 64k for the audio CPU */
	ROM_LOAD( "101_c05.5e",   0x00000, 0x20000, CRC(02c3679f) SHA1(e6d878185e73baca24ac98891c647856be9353c4) )
	ROM_RELOAD(               0x10000, 0x20000 )

    ROM_REGION( 0x080000, REGION_GFX1, 0 )	/* graphics (addressable by the main CPU) */
	ROM_LOAD( "101a12.12l",   0x000000, 0x040000, CRC(33530d7f) SHA1(87859ad058fb79e357101675706373f83a3f23d4) )	/* tiles */
	ROM_LOAD( "101a11.12k",   0x040000, 0x040000, CRC(7f773271) SHA1(0c6a62c6eb1897e88e893576f751e3d4fc788036) )

	ROM_REGION( 0x400000, REGION_GFX2, 0 )	/* graphics (addressable by the main CPU) */
	ROM_LOAD( "101a09.3l",    0x000000, 0x200000, CRC(b6a1b424) SHA1(4cf7bb4b8176977dea10fb80fcd9d6e24cc6d1b9) )	/* sprites */
	ROM_LOAD( "101a10.8l",    0x200000, 0x200000, CRC(bbd58adc) SHA1(ad9bd4df995de6e6290f27c58c7892c7191802e4) )

	ROM_REGION( 0x080000, REGION_GFX3, ROMREGION_DISPOSE )	/* 053936 tiles */
	ROM_LOAD( "101a03.18f",   0x000000, 0x080000, CRC(59a1a91c) SHA1(f596a40784a671e97116df6561682eb6c5c44e08) )

	ROM_REGION( 0x040000, REGION_USER1, 0 )	/* 053936 tilemaps */
	ROM_LOAD( "101a01.18d",   0x000000, 0x020000, CRC(716f910f) SHA1(fbe69cac266084ea1efb094a7f863dca39f12500) )
	ROM_LOAD( "101a02.16d",   0x020000, 0x020000, CRC(222869c7) SHA1(0a9bea294ff3281f316dd4beecc4c94d75d52b49) )

	ROM_REGION( 0x200000, REGION_SOUND1, 0 )	/* samples for the 054539 */
	ROM_LOAD( "101a06.1d",    0x0000, 0x200000, CRC(4f48e043) SHA1(f50e8642d9d3a028c243777640e7cd13da1abf86) )
ROM_END

static DRIVER_INIT( gfx )
{
	konami_rom_deinterleave_2(REGION_GFX1);
	konami_rom_deinterleave_2(REGION_GFX2);
}

static DRIVER_INIT( mia )
{
	unsigned char *gfxdata;
	int len;
	int i,j,k,A,B;
	int bits[32];
	unsigned char *temp;


	init_gfx();

	/*
		along with the normal byte reordering, TMNT also needs the bits to
		be shuffled around because the ROMs are connected differently to the
		051962 custom IC.
	*/
	gfxdata = memory_region(REGION_GFX1);
	len = memory_region_length(REGION_GFX1);
	for (i = 0;i < len;i += 4)
	{
		for (j = 0;j < 4;j++)
			for (k = 0;k < 8;k++)
				bits[8*j + k] = (gfxdata[i + j] >> k) & 1;

		for (j = 0;j < 4;j++)
		{
			gfxdata[i + j] = 0;
			for (k = 0;k < 8;k++)
				gfxdata[i + j] |= bits[j + 4*k] << k;
		}
	}

	/*
		along with the normal byte reordering, MIA also needs the bits to
		be shuffled around because the ROMs are connected differently to the
		051937 custom IC.
	*/
	gfxdata = memory_region(REGION_GFX2);
	len = memory_region_length(REGION_GFX2);
	for (i = 0;i < len;i += 4)
	{
		for (j = 0;j < 4;j++)
			for (k = 0;k < 8;k++)
				bits[8*j + k] = (gfxdata[i + j] >> k) & 1;

		for (j = 0;j < 4;j++)
		{
			gfxdata[i + j] = 0;
			for (k = 0;k < 8;k++)
				gfxdata[i + j] |= bits[j + 4*k] << k;
		}
	}

	temp = malloc(len);
	if (!temp) return;	/* bad thing! */
	memcpy(temp,gfxdata,len);
	for (A = 0;A < len/4;A++)
	{
		/* the bits to scramble are the low 8 ones */
		for (i = 0;i < 8;i++)
			bits[i] = (A >> i) & 0x01;

		B = A & 0x3ff00;

		if ((A & 0x3c000) == 0x3c000)
		{
			B |= bits[3] << 0;
			B |= bits[5] << 1;
			B |= bits[0] << 2;
			B |= bits[1] << 3;
			B |= bits[2] << 4;
			B |= bits[4] << 5;
			B |= bits[6] << 6;
			B |= bits[7] << 7;
		}
		else
		{
			B |= bits[3] << 0;
			B |= bits[5] << 1;
			B |= bits[7] << 2;
			B |= bits[0] << 3;
			B |= bits[1] << 4;
			B |= bits[2] << 5;
			B |= bits[4] << 6;
			B |= bits[6] << 7;
		}

		gfxdata[4*A+0] = temp[4*B+0];
		gfxdata[4*A+1] = temp[4*B+1];
		gfxdata[4*A+2] = temp[4*B+2];
		gfxdata[4*A+3] = temp[4*B+3];
	}
	free(temp);
}


static DRIVER_INIT( tmnt )
{
	unsigned char *gfxdata;
	int len;
	int i,j,k,A,B,entry;
	int bits[32];
	unsigned char *temp;


	init_gfx();

	/*
		along with the normal byte reordering, TMNT also needs the bits to
		be shuffled around because the ROMs are connected differently to the
		051962 custom IC.
	*/
	gfxdata = memory_region(REGION_GFX1);
	len = memory_region_length(REGION_GFX1);
	for (i = 0;i < len;i += 4)
	{
		for (j = 0;j < 4;j++)
			for (k = 0;k < 8;k++)
				bits[8*j + k] = (gfxdata[i + j] >> k) & 1;

		for (j = 0;j < 4;j++)
		{
			gfxdata[i + j] = 0;
			for (k = 0;k < 8;k++)
				gfxdata[i + j] |= bits[j + 4*k] << k;
		}
	}

	/*
		along with the normal byte reordering, TMNT also needs the bits to
		be shuffled around because the ROMs are connected differently to the
		051937 custom IC.
	*/
	gfxdata = memory_region(REGION_GFX2);
	len = memory_region_length(REGION_GFX2);
	for (i = 0;i < len;i += 4)
	{
		for (j = 0;j < 4;j++)
			for (k = 0;k < 8;k++)
				bits[8*j + k] = (gfxdata[i + j] >> k) & 1;

		for (j = 0;j < 4;j++)
		{
			gfxdata[i + j] = 0;
			for (k = 0;k < 8;k++)
				gfxdata[i + j] |= bits[j + 4*k] << k;
		}
	}

	temp = malloc(len);
	if (!temp) return;	/* bad thing! */
	memcpy(temp,gfxdata,len);
	for (A = 0;A < len/4;A++)
	{
		unsigned char *code_conv_table = &memory_region(REGION_PROMS)[0x0000];
#define CA0 0
#define CA1 1
#define CA2 2
#define CA3 3
#define CA4 4
#define CA5 5
#define CA6 6
#define CA7 7
#define CA8 8
#define CA9 9

		/* following table derived from the schematics. It indicates, for each of the */
		/* 9 low bits of the sprite line address, which bit to pick it from. */
		/* For example, when the PROM contains 4, which applies to 4x2 sprites, */
		/* bit OA1 comes from CA5, OA2 from CA0, and so on. */
		static unsigned char bit_pick_table[10][8] =
		{
			/*0(1x1) 1(2x1) 2(1x2) 3(2x2) 4(4x2) 5(2x4) 6(4x4) 7(8x8) */
			{ CA3,   CA3,   CA3,   CA3,   CA3,   CA3,   CA3,   CA3 },	/* CA3 */
			{ CA0,   CA0,   CA5,   CA5,   CA5,   CA5,   CA5,   CA5 },	/* OA1 */
			{ CA1,   CA1,   CA0,   CA0,   CA0,   CA7,   CA7,   CA7 },	/* OA2 */
			{ CA2,   CA2,   CA1,   CA1,   CA1,   CA0,   CA0,   CA9 },	/* OA3 */
			{ CA4,   CA4,   CA2,   CA2,   CA2,   CA1,   CA1,   CA0 },	/* OA4 */
			{ CA5,   CA6,   CA4,   CA4,   CA4,   CA2,   CA2,   CA1 },	/* OA5 */
			{ CA6,   CA5,   CA6,   CA6,   CA6,   CA4,   CA4,   CA2 },	/* OA6 */
			{ CA7,   CA7,   CA7,   CA7,   CA8,   CA6,   CA6,   CA4 },	/* OA7 */
			{ CA8,   CA8,   CA8,   CA8,   CA7,   CA8,   CA8,   CA6 },	/* OA8 */
			{ CA9,   CA9,   CA9,   CA9,   CA9,   CA9,   CA9,   CA8 }	/* OA9 */
		};

		/* pick the correct entry in the PROM (top 8 bits of the address) */
		entry = code_conv_table[(A & 0x7f800) >> 11] & 7;

		/* the bits to scramble are the low 10 ones */
		for (i = 0;i < 10;i++)
			bits[i] = (A >> i) & 0x01;

		B = A & 0x7fc00;

		for (i = 0;i < 10;i++)
			B |= bits[bit_pick_table[i][entry]] << i;

		gfxdata[4*A+0] = temp[4*B+0];
		gfxdata[4*A+1] = temp[4*B+1];
		gfxdata[4*A+2] = temp[4*B+2];
		gfxdata[4*A+3] = temp[4*B+3];
	}
	free(temp);
}

static void shuffle(UINT8 *buf,int len)
{
	int i;
	UINT8 t;

	if (len == 2) return;

	if (len % 4) exit(1);	/* must not happen */

	len /= 2;

	for (i = 0;i < len/2;i++)
	{
		t = buf[len/2 + i];
		buf[len/2 + i] = buf[len + i];
		buf[len + i] = t;
	}

	shuffle(buf,len);
	shuffle(buf + len,len);
}

static DRIVER_INIT( glfgreat )
{
	/* ROMs are interleaved at byte level */
	shuffle(memory_region(REGION_GFX1),memory_region_length(REGION_GFX1));
	shuffle(memory_region(REGION_GFX2),memory_region_length(REGION_GFX2));
}

static DRIVER_INIT( cuebrckj )
{
	generic_nvram = (data8_t *)cbj_nvram;
	generic_nvram_size = 0x400*0x20;

	/* ROMs are interleaved at byte level */
	shuffle(memory_region(REGION_GFX1),memory_region_length(REGION_GFX1));
	shuffle(memory_region(REGION_GFX2),memory_region_length(REGION_GFX2));
}

GAME( 1989, cuebrckj, cuebrick, cuebrckj, mia,      cuebrckj, ROT0,  "Konami", "Cue Brick (World version D)" )

GAME( 1989, mia,      0,        mia,      mia,      mia,      ROT0,  "Konami", "M.I.A. - Missing in Action (version T)" )
GAME( 1989, mia2,     mia,      mia,      mia,      mia,      ROT0,  "Konami", "M.I.A. - Missing in Action (version S)" )

GAME( 1989, tmnt,     0,        tmnt,     tmnt,     tmnt,     ROT0,  "Konami", "Teenage Mutant Ninja Turtles (World 4 Players)" )
GAME( 1989, tmntu,    tmnt,     tmnt,     tmnt,     tmnt,     ROT0,  "Konami", "Teenage Mutant Ninja Turtles (US 4 Players)" )
GAME( 1989, tmht,     tmnt,     tmnt,     tmnt,     tmnt,     ROT0,  "Konami", "Teenage Mutant Hero Turtles (UK 4 Players)" )
GAME( 1990, tmntj,    tmnt,     tmnt,     tmnt,     tmnt,     ROT0,  "Konami", "Teenage Mutant Ninja Turtles (Japan 4 Players)" )
GAME( 1989, tmht2p,   tmnt,     tmnt,     tmnt2p,   tmnt,     ROT0,  "Konami", "Teenage Mutant Hero Turtles (UK 2 Players)" )
GAME( 1990, tmnt2pj,  tmnt,     tmnt,     tmnt2p,   tmnt,     ROT0,  "Konami", "Teenage Mutant Ninja Turtles (Japan 2 Players)" )
GAME( 1989, tmnt2po,  tmnt,     tmnt,     tmnt2p,   tmnt,     ROT0,  "Konami", "Teenage Mutant Ninja Turtles (Oceania 2 Players)" )

GAME( 1990, punkshot, 0,        punkshot, punkshot, gfx,      ROT0,  "Konami", "Punk Shot (US 4 Players)" )
GAME( 1990, punksht2, punkshot, punkshot, punksht2, gfx,      ROT0,  "Konami", "Punk Shot (US 2 Players)" )
GAME( 1990, punkshtj, punkshot, punkshot, punksht2, gfx,      ROT0,  "Konami", "Punk Shot (Japan 2 Players)" )

GAME( 1990, lgtnfght, 0,        lgtnfght, lgtnfght, gfx,      ROT90, "Konami", "Lightning Fighters (US)" )
GAME( 1990, trigon,   lgtnfght, lgtnfght, lgtnfght, gfx,      ROT90, "Konami", "Trigon (Japan)" )

GAME( 1991, blswhstl, 0,        detatwin, detatwin, gfx,      ROT90, "Konami", "Bells and Whistles (Version L)" )		/* version L*/
GAME( 1991, detatwin, blswhstl, detatwin, detatwin, gfx,      ROT90, "Konami", "Detana!! Twin Bee (Japan ver. J)" )	/* version J*/

GAMEX(1991, glfgreat, 0,        glfgreat, glfgreat, glfgreat, ROT0,  "Konami", "Golfing Greats", GAME_UNEMULATED_PROTECTION | GAME_IMPERFECT_GRAPHICS | GAME_IMPERFECT_SOUND )
GAMEX(1991, glfgretj, glfgreat, glfgreat, glfgreat, glfgreat, ROT0,  "Konami", "Golfing Greats (Japan)", GAME_UNEMULATED_PROTECTION | GAME_IMPERFECT_GRAPHICS | GAME_IMPERFECT_SOUND )

GAMEX(1991, tmnt2,    0,        tmnt2,    ssridr4p, gfx,      ROT0,  "Konami", "Teenage Mutant Ninja Turtles - Turtles in Time (US 4 Players ver. UAA)", GAME_UNEMULATED_PROTECTION )		/* ver. UAA*/
GAMEX(1991, tmnt22p,  tmnt2,    tmnt2,    ssriders, gfx,      ROT0,  "Konami", "Teenage Mutant Ninja Turtles - Turtles in Time (US 2 Players ver. UDA)", GAME_UNEMULATED_PROTECTION )		/* ver. UDA*/
GAMEX(1991, tmnt2a,   tmnt2,    tmnt2,    ssrid4ps, gfx,      ROT0,  "Konami", "Teenage Mutant Ninja Turtles - Turtles in Time (Asia 4 Players ver. ADA)", GAME_UNEMULATED_PROTECTION )	/* ver. ADA*/

GAME( 1993, qgakumon, 0,        tmnt2,    qgakumon, gfx,      ROT0,  "Konami", "Quiz Gakumon no Susume (Japan ver. JA2 Type L)" )

GAMEX(1991, ssriders, 0,        ssriders, ssridr4p, gfx,      ROT0,  "Konami", "Sunset Riders (World 4 Players ver. EAC)", GAME_IMPERFECT_GRAPHICS )
GAMEX(1991, ssrdrebd, ssriders, ssriders, ssriders, gfx,      ROT0,  "Konami", "Sunset Riders (World 2 Players ver. EBD)", GAME_IMPERFECT_GRAPHICS )
GAMEX(1991, ssrdrebc, ssriders, ssriders, ssriders, gfx,      ROT0,  "Konami", "Sunset Riders (World 2 Players ver. EBC)", GAME_IMPERFECT_GRAPHICS )
GAMEX(1991, ssrdruda, ssriders, ssriders, ssrid4ps, gfx,      ROT0,  "Konami", "Sunset Riders (US 4 Players ver. UDA)", GAME_IMPERFECT_GRAPHICS )
GAMEX(1991, ssrdruac, ssriders, ssriders, ssridr4p, gfx,      ROT0,  "Konami", "Sunset Riders (US 4 Players ver. UAC)", GAME_IMPERFECT_GRAPHICS )
GAMEX(1991, ssrdrubc, ssriders, ssriders, ssriders, gfx,      ROT0,  "Konami", "Sunset Riders (US 2 Players ver. UBC)", GAME_IMPERFECT_GRAPHICS )
GAMEX(1991, ssrdrabd, ssriders, ssriders, ssriders, gfx,      ROT0,  "Konami", "Sunset Riders (Asia 2 Players ver. ABD)", GAME_IMPERFECT_GRAPHICS )
GAMEX(1991, ssrdrjbd, ssriders, ssriders, ssriders, gfx,      ROT0,  "Konami", "Sunset Riders (Japan 2 Players ver. JBD)", GAME_IMPERFECT_GRAPHICS )
GAMEX(1991, sunsetbl, ssriders, ssridersbl, ssridbl, gfx,     ROT0,  "Konami", "Sunset Riders (bootleg 4 Players ver. ADD)", GAME_NOT_WORKING | GAME_IMPERFECT_GRAPHICS )

GAME( 1991, thndrx2,  0,        thndrx2,  thndrx2,  gfx,      ROT0,  "Konami", "Thunder Cross II (Japan)" )
GAME( 1991, thndrx2a, thndrx2,  thndrx2,  thndrx2,  gfx,      ROT0,  "Konami", "Thunder Cross II (Asia)" )

GAME( 1993, prmrsocr, 0,        prmrsocr, prmrsocr, glfgreat, ROT0,  "Konami", "Premier Soccer (Europe ver. EAB)" )
GAME( 1993, prmrsocj, prmrsocr, prmrsocr, prmrsocr, glfgreat, ROT0,  "Konami", "Premier Soccer (Japan ver. JAB)" )
