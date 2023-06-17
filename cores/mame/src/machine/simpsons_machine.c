#include "driver.h"
#include "vidhrdw/generic.h"
#include "vidhrdw/konamiic.h"
#include "cpu/konami/konami.h"
#include "machine/eeprom.h"

/* from vidhrdw */
extern void simpsons_video_banking( int select );
extern unsigned char *simpsons_xtraram;

int simpsons_firq_enabled;

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

NVRAM_HANDLER( simpsons )
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

READ_HANDLER( simpsons_eeprom_r )
{
	int res;

	res = (EEPROM_read_bit() << 4);

	res |= 0x20;/*konami_eeprom_ack() << 5;  // add the ack /*/

	res |= readinputport( 5 ) & 1; /* test switch */

	if (init_eeprom_count)
	{
		init_eeprom_count--;
		res &= 0xfe;
	}
	return res;
}

WRITE_HANDLER( simpsons_eeprom_w )
{
	if ( data == 0xff )
		return;

	EEPROM_write_bit(data & 0x80);
	EEPROM_set_cs_line((data & 0x08) ? CLEAR_LINE : ASSERT_LINE);
	EEPROM_set_clock_line((data & 0x10) ? ASSERT_LINE : CLEAR_LINE);

	simpsons_video_banking( data & 3 );

	simpsons_firq_enabled = data & 0x04;
}

/***************************************************************************

  Coin Counters, Sound Interface

***************************************************************************/

WRITE_HANDLER( simpsons_coin_counter_w )
{
	/* bit 0,1 coin counters */
	coin_counter_w(0,data & 0x01);
	coin_counter_w(1,data & 0x02);
	/* bit 2 selects mono or stereo sound */
	/* bit 3 = enable char ROM reading through the video RAM */
	K052109_set_RMRD_line((data & 0x08) ? ASSERT_LINE : CLEAR_LINE);
	/* bit 4 = INIT (unknown) */
	/* bit 5 = enable sprite ROM reading */
	K053246_set_OBJCHA_line((~data & 0x20) ? ASSERT_LINE : CLEAR_LINE);
}

READ_HANDLER( simpsons_sound_interrupt_r )
{
	cpu_set_irq_line_and_vector( 1, 0, HOLD_LINE, 0xff );
	return 0x00;
}

READ_HANDLER( simpsons_sound_r )
{
	/* If the sound CPU is running, read the status, otherwise
	   just make it pass the test */
	if (Machine->sample_rate != 0) 	return K053260_0_r(2 + offset);
	else
	{
		static int res = 0x80;

		res = (res & 0xfc) | ((res + 1) & 0x03);
		return offset ? res : 0x00;
	}
}

/***************************************************************************

  Speed up memory handlers

***************************************************************************/

READ_HANDLER( simpsons_speedup1_r )
{
	unsigned char *RAM = memory_region(REGION_CPU1);

	int data1 = RAM[0x486a];

	if ( data1 == 0 )
	{
		int data2 = ( RAM[0x4942] << 8 ) | RAM[0x4943];

		if ( data2 < memory_region_length(REGION_CPU1) )
		{
			data2 = ( RAM[data2] << 8 ) | RAM[data2 + 1];

			if ( data2 == 0xffff )
				cpu_spinuntil_int();

			return RAM[0x4942];
		}

		return RAM[0x4942];
	}

	if ( data1 == 1 )
		RAM[0x486a]--;

	return RAM[0x4942];
}

READ_HANDLER( simpsons_speedup2_r )
{
	int data = memory_region(REGION_CPU1)[0x4856];

	if ( data == 1 )
		cpu_spinuntil_int();

	return data;
}

/***************************************************************************

  Banking, initialization

***************************************************************************/

static void simpsons_banking( int lines )
{
	unsigned char *RAM = memory_region(REGION_CPU1);
	int offs = 0;

	switch ( lines & 0xf0 )
	{
		case 0x00: /* simp_g02.rom */
			offs = 0x10000 + ( ( lines & 0x0f ) * 0x2000 );
		break;

		case 0x10: /* simp_p01.rom */
			offs = 0x30000 + ( ( lines & 0x0f ) * 0x2000 );
		break;

		case 0x20: /* simp_013.rom */
			offs = 0x50000 + ( ( lines & 0x0f ) * 0x2000 );
		break;

		case 0x30: /* simp_012.rom ( lines goes from 0x00 to 0x0c ) */
			offs = 0x70000 + ( ( lines & 0x0f ) * 0x2000 );
		break;

		default:
			log_cb(RETRO_LOG_DEBUG, LOGPRE "PC = %04x : Unknown bank selected (%02x)\n", activecpu_get_pc(), lines );
		break;
	}

	cpu_setbank( 1, &RAM[offs] );
}

MACHINE_INIT( simpsons )
{
	unsigned char *RAM = memory_region(REGION_CPU1);

	konami_cpu_setlines_callback = simpsons_banking;

	paletteram = &RAM[0x88000];
	simpsons_xtraram = &RAM[0x89000];
	spriteram16 = (data16_t *)&RAM[0x8a000];

	simpsons_firq_enabled = 0;

	/* init the default banks */
	cpu_setbank( 1, &RAM[0x10000] );

	RAM = memory_region(REGION_CPU2);

	cpu_setbank( 2, &RAM[0x10000] );

	simpsons_video_banking( 0 );
}
