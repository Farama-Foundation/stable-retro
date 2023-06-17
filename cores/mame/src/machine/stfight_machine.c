/***************************************************************************

  machine.c

  Functions to emulate general aspects of the machine (RAM, ROM, interrupts,
  I/O ports)

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "cpu/z80/z80.h"

/* this prototype will move to the driver*/
WRITE_HANDLER( stfight_bank_w );


/*

Encryption PAL 16R4 on CPU board

          +---U---+
     CP --|       |-- VCC
 ROM D1 --|       |-- ROM D0          M1 = 0                M1 = 1
 ROM D3 --|       |-- (NC)
 ROM D4 --|       |-- D6         D6 = D1 ^^ D3          D6 = / ( D1 ^^ D0 )
 ROM D6 --|       |-- D4         D4 = / ( D6 ^^ A7 )    D4 = D3 ^^ A0
     A0 --|       |-- D3         D3 = / ( D0 ^^ A1 )    D3 = D4 ^^ A4
     A1 --|       |-- D0         D0 = D1 ^^ D4          D0 = / ( D6 ^^ A0 )
     A4 --|       |-- (NC)
     A7 --|       |-- /M1
    GND --|       |-- /OE
          +-------+

*/

DRIVER_INIT( empcity )
{
	unsigned char *rom = memory_region(REGION_CPU1);
	int diff = memory_region_length(REGION_CPU1) / 2;
	int A;


	memory_set_opcode_base(0,rom+diff);

	for (A = 0;A < 0x8000;A++)
	{
		unsigned char src = rom[A];

		/* decode opcode*/
		rom[A+diff] =
				( src & 0xA6 ) |
				( ( ( ( src << 2 ) ^ src ) << 3 ) & 0x40 ) |
				( ~( ( src ^ ( A >> 1 ) ) >> 2 ) & 0x10 ) |
				( ~( ( ( src << 1 ) ^ A ) << 2 ) & 0x08 ) |
				( ( ( src ^ ( src >> 3 ) ) >> 1 ) & 0x01 );

		/* decode operand*/
		rom[A] =
				( src & 0xA6 ) |
				( ~( ( src ^ ( src << 1 ) ) << 5 ) & 0x40 ) |
				( ( ( src ^ ( A << 3 ) ) << 1 ) & 0x10 ) |
				( ( ( src ^ A ) >> 1 ) & 0x08 ) |
				( ~( ( src >> 6 ) ^ A ) & 0x01 );
	}
}

DRIVER_INIT( stfight )
{
	unsigned char *rom = memory_region(REGION_CPU1);
	int diff = memory_region_length(REGION_CPU1) / 2;


	init_empcity();

	/* patch out a tight loop during startup - is the code waiting */
	/* for NMI to wake it up? */
	rom[0xb1 + diff] = 0x00;
	rom[0xb2 + diff] = 0x00;
	rom[0xb3 + diff] = 0x00;
	rom[0xb4 + diff] = 0x00;
	rom[0xb5 + diff] = 0x00;
}

MACHINE_INIT( stfight )
{
    /* initialise rom bank*/
    stfight_bank_w( 0, 0 );
}

/* It's entirely possible that this bank is never switched out*/
/* - in fact I don't even know how/where it's switched in!*/
WRITE_HANDLER( stfight_bank_w )
{
	unsigned char   *ROM2 = memory_region(REGION_CPU1) + 0x10000;

	cpu_setbank( 1, &ROM2[data<<14] );
}

INTERRUPT_GEN( stfight_vb_interrupt )
{
    /* Do a RST10*/
    cpu_set_irq_line_and_vector(0,0,HOLD_LINE,0xd7);
}

/*
 *      CPU 1 timed interrupt - 30Hz???
 */

INTERRUPT_GEN( stfight_interrupt_1 )
{
    /* Do a RST08*/
    cpu_set_irq_line_and_vector(0,0,HOLD_LINE,0xcf);
}

/*
 *      Hardware handlers
 */

/* Perhaps define dipswitches as active low?*/
READ_HANDLER( stfight_dsw_r )
{
    return( ~readinputport( 3+offset ) );
}

static int stfight_coin_mech_query_active = 0;
static int stfight_coin_mech_query;

READ_HANDLER( stfight_coin_r )
{
    static int coin_mech_latch[2] = { 0x02, 0x01 };

    int coin_mech_data;
    int i;

    /* Was the coin mech queried by software?*/
    if( stfight_coin_mech_query_active )
    {
        stfight_coin_mech_query_active = 0;
        return( (~stfight_coin_mech_query) & 0x03 );
    }

    /*
     *      Is this really necessary?
     *      - we can control impulse length so that the port is
     *        never strobed twice within the impulse period
     *        since it's read by the 30Hz interrupt ISR
     */

    coin_mech_data = readinputport( 5 );

    for( i=0; i<2; i++ )
    {
        /* Only valid on signal edge */
        if( ( coin_mech_data & (1<<i) ) != coin_mech_latch[i] )
            coin_mech_latch[i] = coin_mech_data & (1<<i);
        else
            coin_mech_data |= coin_mech_data & (1<<i);
    }

    return( coin_mech_data );
}

WRITE_HANDLER( stfight_coin_w )
{
    /* interrogate coin mech*/
    stfight_coin_mech_query_active = 1;
    stfight_coin_mech_query = data;
}

/*
 *      Machine hardware for MSM5205 ADPCM sound control
 */

static int sampleLimits[] =
{
    0x0000,     /* machine gun fire?*/
    0x1000,     /* player getting shot*/
    0x2C00,     /* player shooting*/
    0x3C00,     /* girl screaming*/
    0x5400,     /* girl getting shot*/
    0x7200      /* (end of samples)*/
};
static int adpcm_data_offs;
static int adpcm_data_end;

void stfight_adpcm_int( int data )
{
	static int toggle;
	unsigned char *SAMPLES = memory_region(REGION_SOUND1);
	int adpcm_data = SAMPLES[adpcm_data_offs & 0x7fff];

    /* finished playing sample?*/
    if( adpcm_data_offs == adpcm_data_end )
    {
        MSM5205_reset_w( 0, 1 );
        return;
    }

	if( toggle == 0 )
		MSM5205_data_w( 0, ( adpcm_data >> 4 ) & 0x0f );
	else
	{
		MSM5205_data_w( 0, adpcm_data & 0x0f );
		adpcm_data_offs++;
	}

	toggle ^= 1;
}

WRITE_HANDLER( stfight_adpcm_control_w )
{
    if( data < 0x08 )
    {
        adpcm_data_offs = sampleLimits[data];
        adpcm_data_end = sampleLimits[data+1];
    }

    MSM5205_reset_w( 0, data & 0x08 ? 1 : 0 );
}

WRITE_HANDLER( stfight_e800_w )
{
}

/*
 *      Machine hardware for YM2303 fm sound control
 */

static unsigned char fm_data;

WRITE_HANDLER( stfight_fm_w )
{
    /* the sound cpu ignores any fm data without bit 7 set*/
    fm_data = 0x80 | data;
}

READ_HANDLER( stfight_fm_r )
{
    int data = fm_data;

    /* clear the latch?!?*/
    fm_data &= 0x7f;

    return( data );
}
