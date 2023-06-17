/***************************************************************************

  Namco System 11 - Arcade PSX Hardware
  =====================================
  Driver by smf & Ryan Holtz
  Board notes by The Guru & Brian A. Troha
  Thanks to R Belmont & The Zinc Team.

  Issues:
   There is no sound as the Namco C76 (Mitsubishi M37702) & Namco C352 are not emulated.
   Random draw list corruption in soul edge v2 & dunkmania.
   missing memcard support in soul edge, dunk mania & prime goal ex 
   Most games try to run too fast, lack of root counters and sound timers are likely to blame.

Known Dumps
-----------

Game       Description                             CPU board           Mother board             Daughter board          Keycus   ROM0L

tekken     Tekken (TE4/VER.C)                      COH 100             SYSTEM11 MOTHER PCB      SYSTEM11 ROM8 PCB       none     5
tekkena    Tekken (TE2/VER.B)                      COH 100             "                        "                       "        "
tekkenb    Tekken (TE1/VER.B)                      COH 100             "                        "                       "        "
tekken2    Tekken 2 Ver.B (TES3/VER.B)             COH 100             SYSTEM11 MOTHER PCB      SYSTEM11 ROM8 PCB       C406     6
tekken2a   Tekken 2 Ver.B (TES2/VER.B)             COH 100             "                        "                       "        "
tekken2b   Tekken 2 (TES2/VER.A)                   COH 100             "                        "                       "        "
souledge   Soul Edge Ver. II (SO4/VER.C)           COH 100 / COH 110   SYSTEM11 MOTHER PCB      SYSTEM11 ROM8 PCB       C409     6
souledga   Soul Edge (SO3/VER.A)                   COH 100 / COH 110   "                        "                       "        "
souledgb   Soul Edge (SO1/VER.A)                   COH 100 / COH 110   "                        "                       "        "
dunkmnia   Dunk Mania (DM1/VER.C)                  COH 100 / COH 110   SYSTEM11 MOTHER PCB      SYSTEM11 ROM8 PCB       C410     5
xevi3dg    Xevious 3D/G (XV31/VER.A)               COH 100 / COH 110   SYSTEM11 MOTHER PCB      SYSTEM11 ROM8 PCB       C430     5
primglex   Prime Goal EX (PG1/VER.A)               COH 100 / COH 110   SYSTEM11 MOTHER PCB      SYSTEM11 ROM8 PCB       C411     6
danceyes   Dancing Eyes (DC1/VER.A)                COH 100 / COH 110   SYSTEM11 MOTHER PCB      SYSTEM11 ROM8 PCB       C431     5
starswep   Star Sweep (STP1/VER.A)                 COH 100 / COH 110   SYSTEM11 MOTHER(B) PCB                           C442     -
myangel3   Kosodate Quiz My Angel 3 (KQT1/VER.A)   COH 110             SYSTEM11 MOTHER(B) PCB   SYSTEM11 ROM8(64) PCB   C443     ?

PCB Layout
----------


SYSTEM11 MOTHER PCB 8645960103 (8645970103)
|------------------------------------------|
|                         (CONN1)          |
|               C384     KEYCUS   S11MOT3A |
|                        AT28C16           |
|                       S11MOT2A           |
|                                   PRG.2L |
|J     WAVE.8K                      PRG.2K |
|A                                  PRG.2J |
|M                                  PRG.2F |
|M                                         |
|A                                         |
|                                          |
|       C76     SPROG.6D    C195  61C256   |
|                                 61C256   |
|               S11MOT1A                   |
|                                          |
|     DSW(2)                               |
|                                          |
|             (CONN2)                      |
|                                          |
|------------------------------------------|


Notes:
      S11MOT* = Standard System 11 PALs
      WAVE ROM is 42 pin DIP either 16MBit or if 32MBit it's Byte Mode.
      SPROG ROM type is Intel PA28F200BX 2MBit Flash
      PRG ROM type is Intel E28F008SA 8MBit Flash
      CONN1 is for connection of the ROM Board
      CONN2 is for connection of the CPU board
      There is a REV B Mother board (SYSTEM11 MOTHER(B) PCB) with slightly different
      component placings. It's not known if it has different components also.


ROM Board
---------

SYSTEM11 ROM8 PCB 8645960202 (8645970202)
|----------------------------------------|
|               (CONN1)                  |
|ROM3L.9               74F139    ROM3U.1 |
|                                        |
|ROM1L.8     ROM0L.6       ROM2U.4       |
|      ROM2L.7      ROM0U.5       ROM1U.3|
|                                        |
|----------------------------------------|


Notes:
      This ROM board is wired to accept 8Bit x 32MBit SOP44 MASK ROMs.
      There is another ROM board (ROM8(64) PCB) which is wired to accept 64MBit SOP44 MASK ROMs
      and the IC locations are slightly different also.


CPU Board
---------

GP-11  COH-100  S-L1B 1-655-543-12
|-------------------------------------|
|         SONY           KM48V514BJ-6 |
|         CXD8530AQ      KM48V514BJ-6 |
|              67.737MHz KM48V514BJ-6 |
|                        KM48V514BJ-6 |
|                        KM48V514BJ-6 |
|                        KM48V514BJ-6 |
|         SONY           KM48V514BJ-6 |
|(CONN2)  CXD8538Q       KM48V514BJ-6 |
|              53.69MHz               |
|                                     |
|                                     |
|                       D482445LGW-A70|
|  SONY     D482445LGW-A70            |
|  CXD2923AR            D482445LGW-A70|
|           D482445LGW-A70            |
|-------------------------------------|

CXD8530BQ may be used instead of CXD8530AQ


GP-13  COH-110  S-XMB 1-660-276-11
|-------------------------------------|
|         SONY           KM48V514BJ-6 |
|         CXD8530CQ      KM48V514BJ-6 |
|              67.737MHz KM48V514BJ-6 |
|                        KM48V514BJ-6 |
|                        KM48V514BJ-6 |
|                        KM48V514BJ-6 |
|         SONY           KM48V514BJ-6 |
|(CONN2)  CXD8561Q       KM48V514BJ-6 |
|              53.69MHz               |
|                                     |
|                         Motorola    |
|                         XC44200FUD  |
|    32Meg  32Meg                     |
|                                     |
|                                     |
|-------------------------------------|

32Meg = KM4132G271Q-12


***************************************************************************/

#include "driver.h"
#include "state.h"
#include "cpu/mips/psx.h"
#include "includes/psx.h"

#define VERBOSE_LEVEL ( 0 )

static INLINE void verboselog( int n_level, const char *s_fmt, ... )
{
	if( VERBOSE_LEVEL >= n_level )
	{
		va_list v;
		char buf[ 32768 ];
		va_start( v, s_fmt );
		vsprintf( buf, s_fmt, v );
		va_end( v );
		log_cb(RETRO_LOG_DEBUG, LOGPRE  "%08x: %s", activecpu_get_pc(), buf );
	}
}

static data32_t *namcos11_sharedram;
static data32_t *namcos11_keycus;
static size_t namcos11_keycus_size;

static WRITE32_HANDLER( keycus_w )
{
	verboselog( 1, "keycus_w( %08x, %08x, %08x )\n", offset, data, mem_mask );
	COMBINE_DATA( &namcos11_keycus[ offset ] );
}

/* tekken 2 */
static READ32_HANDLER( keycus_c406_r )
{
	/* todo: verify behaviour */
	data32_t data;

	data = namcos11_keycus[ offset ];
	switch( offset )
	{
	case 0:
		data = ( data & 0xffff0000 ) |
			( ( namcos11_keycus[ 0 ] >> 8 ) & 0xf000 ) |
			( ( namcos11_keycus[ 0 ] >> 16 ) & 0xf00 ) |
			( ( namcos11_keycus[ 1 ] >> 8 ) & 0xf0 ) |
			( ( namcos11_keycus[ 1 ] >> 8 ) & 0xf );
		break;
	}
	verboselog( 1, "keycus_c406_r( %08x, %08x, %08x )\n", offset, data, mem_mask );
	return data;
}

/* soul edge */
static READ32_HANDLER( keycus_c409_r )
{
	/* todo: verify behaviour */
	data32_t data;

	data = namcos11_keycus[ offset ];
	switch( offset )
	{
	case 3:
		data = ( data & 0x0000ffff ) | 0x000f0000;
		break;
	}
	verboselog( 1, "keycus_c409_r( %08x, %08x, %08x )\n", offset, data, mem_mask );
	return data;
}

/* dunk mania */
static READ32_HANDLER( keycus_c410_r )
{
	data32_t data;
	data32_t n_value;

	if( ( namcos11_keycus[ 0 ] & 0x0000ffff ) != 0xfffe )
	{
		n_value = namcos11_keycus[ 0 ] & 0x0000ffff;
	}
	else
	{
		n_value = 410;
	}

	data = namcos11_keycus[ offset ];
	switch( offset )
	{
	case 0:
		data = ( data & 0x0000ffff ) |
			( ( n_value / 1 ) % 10 );
		break;
	case 1:
		data =
			( ( ( n_value / 10 ) % 10 ) << 24 ) |
			( ( ( n_value / 100 ) % 10 ) << 0 ) |
			( ( ( n_value / 1000 ) % 10 ) << 8 ) |
			( ( ( n_value / 10000 ) % 10 ) << 16 );
		break;
	}
	verboselog( 1, "keycus_c410_r( %08x, %08x, %08x )\n", offset, data, mem_mask );
	return data;
}

/* prime goal ex */
static READ32_HANDLER( keycus_c411_r )
{
	data32_t data;
	data32_t n_value;

	data = namcos11_keycus[ offset ];
	if( ( namcos11_keycus[ 1 ] & 0x0000ffff ) == 0x00007256 )
	{
		n_value = namcos11_keycus[ 5 ] & 0x0000ffff;
	}
	else
	{
		n_value = 411;
	}

	switch( offset )
	{
	case 0:
		data = ( data & 0xffff0000 ) | ( ( ( n_value / 10 ) % 10 ) << 8 ) | ( ( n_value / 1 ) % 10 );
		break;
	case 1:
		data = ( data & 0xffff0000 ) | ( ( ( n_value / 1000 ) % 10 ) << 8 ) | ( ( n_value / 100 ) % 10 );
		break;
	case 4:
		data = ( data & 0xffff0000 ) | ( ( n_value / 10000 ) % 10 );
		break;
	}

	verboselog( 1, "keycus_c411_r( %08x, %08x, %08x )\n", offset, data, mem_mask );
	return data;
}

/* xevious 3d/g */
static READ32_HANDLER( keycus_c430_r )
{
	data32_t data;
	UINT16 n_value;

	if( ( namcos11_keycus[ 2 ] & 0x0000ffff ) == 0x0000e296 )
	{
		n_value = namcos11_keycus[ 0 ] & 0x0000ffff;
	}
	else
	{
		n_value = 430;
	}

	data = namcos11_keycus[ offset ];
	switch( offset )
	{
	case 0:
		data = ( data & 0x0000ffff ) |
			( ( ( n_value / 10000 ) % 10 ) << 16 );
		break;
	case 2:
		data =
			( ( ( n_value / 1000 ) % 10 ) << 8 ) |
			( ( ( n_value / 100 ) % 10 ) << 0 ) |
			( ( ( n_value / 10 ) % 10 ) << 24 ) |
			( ( ( n_value / 1 ) % 10 ) << 16 );
		break;
	}
	verboselog( 1, "keycus_c430_r( %08x, %08x, %08x )\n", offset, data, mem_mask );
	return data;
}

/* dancing eyes */
static READ32_HANDLER( keycus_c431_r )
{
	data32_t data;
	UINT16 n_value;

	if( ( namcos11_keycus[ 0 ] & 0x0000ffff ) == 0x00009e61 )
	{
		n_value = namcos11_keycus[ 6 ] & 0x0000ffff;
	}
	else
	{
		n_value = 431;
	}

	data = namcos11_keycus[ offset ];
	switch( offset )
	{
	case 0:
		data = ( data & 0xffff0000 ) | ( ( ( n_value / 10 ) % 10 ) << 8 ) | ( ( n_value / 1 ) % 10 );
		break;
	case 2:
		data = ( data & 0xffff0000 ) | ( ( ( n_value / 1000 ) % 10 ) << 8 ) | ( ( n_value / 100 ) % 10 );
		break;
	case 4:
		data = ( data & 0xffff0000 ) | ( ( n_value / 10000 ) % 10 );
		break;
	}
	verboselog( 1, "keycus_c431_r( %08x, %08x, %08x )\n", offset, data, mem_mask );
	return data;
}

/* star sweep */
static READ32_HANDLER( keycus_c442_r )
{
	/* todo: verify behaviour */
	UINT32 data;

	data = namcos11_keycus[ offset ];

	switch( offset )
	{
	case 0:
		if( ( data & 0xffff0000 ) == 0x00210000 )
		{
			data = ( data & 0x0000ffff ) | 0xc4420000;
		}
		break;
	}
	verboselog( 1, "keycus_c442_r( %08x, %08x, %08x )\n", offset, data, mem_mask );
	return data;
}

/* kosodate quiz my angel 3 */
static READ32_HANDLER( keycus_c443_r )
{
	/* todo: verify behaviour */
	UINT32 data;

	data = namcos11_keycus[ offset ];

	switch( offset )
	{
	case 0:
		if( ( data & 0x0000ffff ) == 0x00000020 )
		{
			data = ( data & 0xffff0000 ) | 0x00005678;
		}
		if( ( data & 0xffff0000 ) == 0xa9870000 )
		{
			data = ( data & 0x0000ffff ) | 0x56580000;
		}
		if( ( data & 0xffff0000 ) == 0xffff0000 )
		{
			data = ( data & 0x0000ffff ) | 0xc4430000;
		}
		break;
	}
	verboselog( 1, "keycus_c443_r( %08x, %08x, %08x )\n", offset, data, mem_mask );
	return data;
}

static WRITE32_HANDLER( sharedram_w )
{
	verboselog( 1, "sharedram_w( %08x, %08x, %08x )\n", ( offset * 4 ) + 0x4000, data, mem_mask );
	COMBINE_DATA( &namcos11_sharedram[ offset ] );
}

static READ32_HANDLER( sharedram_r )
{
	verboselog( 1, "sharedram_r( %08x, %08x ) %08x\n", ( offset * 4 ) + 0x4000, mem_mask, namcos11_sharedram[ offset ] );
	return namcos11_sharedram[ offset ];
}

#define SHRAM( x ) namcos11_sharedram[ ( x - 0x4000 ) / 4 ]

static INTERRUPT_GEN( namcos11_vblank )
{
	UINT16 n_coin;
	UINT32 n_input;
	static UINT16 n_oldcoin = 0;

	n_input = readinputport( 0 ) | ( readinputport( 1 ) << 16 );
	SHRAM( 0xbd00 ) = n_input | ( ( n_input & ~SHRAM( 0xbd00 ) ) >> 8 );

	n_input = readinputport( 2 ) | ( readinputport( 3 ) << 16 );
	SHRAM( 0xbd04 ) = n_input | ( ( n_input & ~SHRAM( 0xbd04 ) ) >> 8 );

	n_input = readinputport( 4 );
	SHRAM( 0xbd08 ) = n_input | ( ( n_input & ~SHRAM( 0xbd08 ) ) >> 8 );

	n_coin = readinputport( 5 );

	if( ( n_coin & n_oldcoin & 0x08 ) != 0 )
	{
		SHRAM( 0xbd1c ) = ( SHRAM( 0xbd1c ) & 0x0000ffff ) | ( ( SHRAM( 0xbd1c ) + 0x00010000 ) & 0xffff0000 );
	}
	if( ( n_coin & n_oldcoin & 0x04 ) != 0 )
	{
		SHRAM( 0xbd20 ) = ( SHRAM( 0xbd20 ) & 0xffff0000 ) | ( ( SHRAM( 0xbd20 ) + 0x00000001 ) & 0x0000ffff );
	}
	if( ( n_coin & n_oldcoin & 0x02 ) != 0 )
	{
		SHRAM( 0xbd20 ) = ( SHRAM( 0xbd20 ) & 0x0000ffff ) | ( ( SHRAM( 0xbd20 ) + 0x00010000 ) & 0xffff0000 );
	}
	if( ( n_coin & n_oldcoin & 0x01 ) != 0 )
	{
		SHRAM( 0xbd24 ) = ( SHRAM( 0xbd24 ) & 0xffff0000 ) | ( ( SHRAM( 0xbd24 ) + 0x00000001 ) & 0x0000ffff );
	}
	n_oldcoin = ~n_coin;

	psx_vblank();
}

static UINT32 m_n_bankoffset;
static UINT32 m_p_n_bankoffset[ 8 ];

static INLINE void bankswitch_update( int n_bank )
{
	verboselog( 1, "bankswitch_update( %d ) = %08x\n", n_bank, m_p_n_bankoffset[ n_bank ] );
	cpu_setbank( 7 + n_bank, memory_region( REGION_USER3 ) + m_p_n_bankoffset[ n_bank ] );
}

static void bankswitch_update_all( void )
{
	int n_bank;

	for( n_bank = 0; n_bank < 8; n_bank++ )
	{
		bankswitch_update( n_bank );
	}
}

static INLINE void bankswitch_rom8( int n_bank, int n_data )
{
	m_p_n_bankoffset[ n_bank ] = ( ( ( n_data & 0xc0 ) >> 4 ) + ( n_data & 0x03 ) ) * 1024 * 1024;
	bankswitch_update( n_bank );
}

static WRITE32_HANDLER( bankswitch_rom32_w )
{
	verboselog( 2, "bankswitch_rom32_w( %08x, %08x, %08x )\n", offset, data, mem_mask );

	if( ACCESSING_LSW32 )
	{
		bankswitch_rom8( ( offset * 2 ), data & 0xffff );
	}
	if( ACCESSING_MSW32 )
	{
		bankswitch_rom8( ( offset * 2 ) + 1, data >> 16 );
	}
}

static WRITE32_HANDLER( bankswitch_rom64_upper_w )
{
	verboselog( 2, "bankswitch_rom64_upper_w( %08x, %08x, %08x )\n", offset, data, mem_mask );

	if( ACCESSING_LSW32 )
	{
		m_n_bankoffset = 0;
	}
	if( ACCESSING_MSW32 )
	{
		m_n_bankoffset = 16;
	}
}

static INLINE void bankswitch_rom64( int n_bank, int n_data )
{
	/* todo: verify behaviour */
	m_p_n_bankoffset[ n_bank ] = ( ( ( ( n_data & 0xc0 ) >> 3 ) + ( n_data & 0x07 ) ) ^ m_n_bankoffset ) * 1024 * 1024;
	bankswitch_update( n_bank );
}

static WRITE32_HANDLER( bankswitch_rom64_w )
{
	verboselog( 2, "bankswitch_rom64_w( %08x, %08x, %08x )\n", offset, data, mem_mask );

	if( ACCESSING_LSW32 )
	{
		bankswitch_rom64( ( offset * 2 ), data & 0xffff );
	}
	if( ACCESSING_MSW32 )
	{
		bankswitch_rom64( ( offset * 2 ) + 1, data >> 16 );
	}
}

static MEMORY_WRITE32_START( namcos11_writemem )
	{ 0x00000000, 0x003fffff, MWA32_RAM },    /* ram */
	{ 0x1f800000, 0x1f8003ff, MWA32_BANK1 },  /* scratchpad */
	{ 0x1f801000, 0x1f801007, MWA32_NOP },
	{ 0x1f801008, 0x1f80100b, MWA32_RAM },    /* ?? */
	{ 0x1f80100c, 0x1f80102f, MWA32_NOP },
	{ 0x1f801040, 0x1f80105f, psx_sio_w },
	{ 0x1f801060, 0x1f80106f, MWA32_NOP },
	{ 0x1f801070, 0x1f801077, psx_irq_w },
	{ 0x1f801080, 0x1f8010ff, psx_dma_w },
	{ 0x1f801100, 0x1f80113f, psx_counter_w },
	{ 0x1f801810, 0x1f801817, psx_gpu_w },
	{ 0x1f801820, 0x1f801827, psx_mdec_w },
	{ 0x1f801c00, 0x1f801dff, MWA32_NOP },
	{ 0x1f802020, 0x1f802033, MWA32_RAM },
	{ 0x1f802040, 0x1f802043, MWA32_NOP },
	{ 0x1fa04000, 0x1fa0ffff, sharedram_w, &namcos11_sharedram }, /* shared ram */
	{ 0x1fa20000, 0x1fa2ffff, keycus_w, &namcos11_keycus, &namcos11_keycus_size }, /* keycus */
	{ 0x1fa30000, 0x1fa30fff, MWA32_RAM, (data32_t **)&generic_nvram, &generic_nvram_size }, /* flash */
	{ 0x1fb00000, 0x1fb00003, MWA32_NOP },    /* ?? */
	{ 0x1fbf6000, 0x1fbf6003, MWA32_NOP },    /* ?? */
	{ 0x1fc00000, 0x1fffffff, MWA32_ROM },    /* bios */
	{ 0x80000000, 0x803fffff, MWA32_BANK3 },  /* ram mirror */
	{ 0x9fc00000, 0x9fffffff, MWA32_ROM },    /* bios */
	{ 0xa0000000, 0xa03fffff, MWA32_BANK5 },  /* ram mirror */
	{ 0xbfc00000, 0xbfffffff, MWA32_ROM },    /* bios */
	{ 0xfffe0130, 0xfffe0133, MWA32_NOP },
MEMORY_END

static MEMORY_READ32_START( namcos11_readmem )
	{ 0x00000000, 0x003fffff, MRA32_RAM },    /* ram */
	{ 0x1f000000, 0x1f0fffff, MRA32_BANK7 },  /* banked roms */
	{ 0x1f100000, 0x1f1fffff, MRA32_BANK8 },
	{ 0x1f200000, 0x1f2fffff, MRA32_BANK9 },
	{ 0x1f300000, 0x1f3fffff, MRA32_BANK10 },
	{ 0x1f400000, 0x1f4fffff, MRA32_BANK11 },
	{ 0x1f500000, 0x1f5fffff, MRA32_BANK12 },
	{ 0x1f600000, 0x1f6fffff, MRA32_BANK13 },
	{ 0x1f700000, 0x1f7fffff, MRA32_BANK14 },
	{ 0x1f800000, 0x1f8003ff, MRA32_BANK1 },  /* scratchpad */
	{ 0x1f801008, 0x1f80100b, MRA32_RAM },    /* ?? */
	{ 0x1f801010, 0x1f801013, MRA32_NOP },
	{ 0x1f801014, 0x1f801017, MRA32_NOP },
	{ 0x1f801040, 0x1f80105f, psx_sio_r },
	{ 0x1f801070, 0x1f801077, psx_irq_r },
	{ 0x1f801080, 0x1f8010ff, psx_dma_r },
	{ 0x1f801100, 0x1f80113f, psx_counter_r },
	{ 0x1f801810, 0x1f801817, psx_gpu_r },
	{ 0x1f801820, 0x1f801827, psx_mdec_r },
	{ 0x1f801c00, 0x1f801dff, MRA32_NOP },
	{ 0x1f802020, 0x1f802033, MRA32_RAM },
	{ 0x1fa04000, 0x1fa0ffff, sharedram_r },  /* shared ram */
	{ 0x1fa30000, 0x1fa30fff, MRA32_RAM },    /* flash */
	{ 0x1fc00000, 0x1fffffff, MRA32_BANK2 },  /* bios mirror */
	{ 0x80000000, 0x803fffff, MRA32_BANK3 },  /* ram mirror */
	{ 0x9fc00000, 0x9fffffff, MRA32_BANK4 },  /* bios mirror */
	{ 0xa0000000, 0xa03fffff, MRA32_BANK5 },  /* ram mirror */
	{ 0xbfc00000, 0xbfffffff, MRA32_BANK6 },  /* bios */
MEMORY_END

static struct
{
	const char *s_name;
	mem_read32_handler keycus_r;
	int n_daughterboard;
} namcos11_config_table[] =
{
	{ "tekken", NULL, 32 },
	{ "tekkena", NULL, 32 },
	{ "tekkenb", NULL, 32 },
	{ "tekken2", keycus_c406_r, 32 },
	{ "tekken2a", keycus_c406_r, 32 },
	{ "tekken2b", keycus_c406_r, 32 },
	{ "souledge", keycus_c409_r, 32 },
	{ "souledga", keycus_c409_r, 32 },
	{ "souledgb", keycus_c409_r, 32 },
	{ "dunkmnia", keycus_c410_r, 32 },
	{ "xevi3dg",  keycus_c430_r, 32 },
	{ "primglex", keycus_c411_r, 32 },
	{ "danceyes", keycus_c431_r, 32 },
	{ "starswep", keycus_c442_r, 0 },
	{ "myangel3", keycus_c443_r, 64 },
	{ NULL, NULL }
};

static DRIVER_INIT( namcos11 )
{
	int n_game;

	cpu_setbank( 1, memory_region( REGION_USER1 ) );
	cpu_setbank( 2, memory_region( REGION_USER2 ) );
	cpu_setbank( 3, memory_region( REGION_CPU1 ) );
	cpu_setbank( 4, memory_region( REGION_USER2 ) );
	cpu_setbank( 5, memory_region( REGION_CPU1 ) );
	cpu_setbank( 6, memory_region( REGION_USER2 ) );

	n_game = 0;
	while( namcos11_config_table[ n_game ].s_name != NULL )
	{
		if( strcmp( Machine->gamedrv->name, namcos11_config_table[ n_game ].s_name ) == 0 )
		{
			if( namcos11_config_table[ n_game ].keycus_r != NULL )
			{
				install_mem_read32_handler( 0, 0x1fa20000, 0x1fa2ffff, namcos11_config_table[ n_game ].keycus_r );
			}
			if( namcos11_config_table[ n_game ].n_daughterboard != 0 )
			{
				int n_bank;

				m_n_bankoffset = 0;
				for( n_bank = 0; n_bank < 8; n_bank++ )
				{
					m_p_n_bankoffset[ n_bank ] = 0;
				}
				bankswitch_update_all();

				if( namcos11_config_table[ n_game ].n_daughterboard == 32 )
				{
					install_mem_write32_handler( 0, 0x1fa10020, 0x1fa1002f, bankswitch_rom32_w );
				}
				if( namcos11_config_table[ n_game ].n_daughterboard == 64 )
				{
					install_mem_write32_handler( 0, 0x1f080000, 0x1f080003, bankswitch_rom64_upper_w );
					install_mem_write32_handler( 0, 0x1fa10020, 0x1fa1002f, bankswitch_rom64_w );
					install_mem_read32_handler( 0, 0x1fa10020, 0x1fa1002f, MRA32_NOP );
				}
				state_save_register_UINT32( "namcos11", 0, "m_n_bankoffset", &m_n_bankoffset, 1 );
				state_save_register_UINT32( "namcos11", 0, "m_p_n_bankoffset", &m_p_n_bankoffset[ 0 ], 8 );
				state_save_register_func_postload( bankswitch_update_all );
			}
			else
			{
				install_mem_write32_handler( 0, 0x1fa10020, 0x1fa1002f, MWA32_NOP );
			}
			break;
		}
		n_game++;
	}

	psx_driver_init();
}

MACHINE_INIT( namcos11 )
{
	memset( namcos11_keycus, 0, namcos11_keycus_size );
	psx_machine_init();
}

static MACHINE_DRIVER_START( coh100 )
	/* basic machine hardware */
	MDRV_CPU_ADD( PSXCPU, 33868800 / 2 ) /* 33MHz ?? */
	MDRV_CPU_MEMORY( namcos11_readmem, namcos11_writemem )
	MDRV_CPU_VBLANK_INT( namcos11_vblank, 1 )

	MDRV_FRAMES_PER_SECOND( 60 )
	MDRV_VBLANK_DURATION( 0 )

	MDRV_MACHINE_INIT( namcos11 )
	MDRV_NVRAM_HANDLER( generic_0fill )

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES( VIDEO_TYPE_RASTER )
#if defined( MAME_DEBUG )
	MDRV_SCREEN_SIZE( 1024, 1024 )
	MDRV_VISIBLE_AREA( 0, 1023, 0, 1023 )
#else
	MDRV_SCREEN_SIZE( 640, 480 )
	MDRV_VISIBLE_AREA( 0, 639, 0, 479 )
#endif
	MDRV_PALETTE_LENGTH( 65536 )

	MDRV_PALETTE_INIT( psx )
	MDRV_VIDEO_START( psx_type1_1024x1024 )
	MDRV_VIDEO_UPDATE( psx )
	MDRV_VIDEO_STOP( psx )

	/* sound hardware */
	MDRV_SOUND_ATTRIBUTES( SOUND_SUPPORTS_STEREO )
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( coh100_tekken )
	/* basic machine hardware */
	MDRV_CPU_ADD( PSXCPU, 33868800 / 2 ) /* 33MHz ?? */
	MDRV_CPU_MEMORY( namcos11_readmem, namcos11_writemem )
	MDRV_CPU_VBLANK_INT( namcos11_vblank, 1 )

	MDRV_FRAMES_PER_SECOND( 60 )
	MDRV_VBLANK_DURATION( 0 )

	MDRV_MACHINE_INIT( namcos11 )
	MDRV_NVRAM_HANDLER( generic_0fill )

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES( VIDEO_TYPE_RASTER )
#if defined( MAME_DEBUG )
	MDRV_SCREEN_SIZE( 1024, 1024 )
	MDRV_VISIBLE_AREA( 0, 1023, 0, 1023 )
#else
	MDRV_SCREEN_SIZE( 640, 240 )
	MDRV_VISIBLE_AREA( 0, 639, 0, 239 )
#endif
	MDRV_PALETTE_LENGTH( 65536 )

	MDRV_PALETTE_INIT( psx )
	MDRV_VIDEO_START( psx_type1_1024x1024 )
	MDRV_VIDEO_UPDATE( psx )
	MDRV_VIDEO_STOP( psx )

	/* sound hardware */
	MDRV_SOUND_ATTRIBUTES( SOUND_SUPPORTS_STEREO )
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( coh100_danceyes )
	/* basic machine hardware */
	MDRV_CPU_ADD( PSXCPU, 33868800 / 2 ) /* 33MHz ?? */
	MDRV_CPU_MEMORY( namcos11_readmem, namcos11_writemem )
	MDRV_CPU_VBLANK_INT( namcos11_vblank, 1 )

	MDRV_FRAMES_PER_SECOND( 60 )
	MDRV_VBLANK_DURATION( 0 )

	MDRV_MACHINE_INIT( namcos11 )
	MDRV_NVRAM_HANDLER( generic_0fill )

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES( VIDEO_TYPE_RASTER )
#if defined( MAME_DEBUG )
	MDRV_SCREEN_SIZE( 1024, 1024 )
	MDRV_VISIBLE_AREA( 0, 1023, 0, 1023 )
#else
	MDRV_SCREEN_SIZE( 640, 240 )
	MDRV_VISIBLE_AREA( 0, 639, 0, 239 )
#endif
	MDRV_PALETTE_LENGTH( 65536 )

	MDRV_PALETTE_INIT( psx )
	MDRV_VIDEO_START( psx_type1_1024x1024 )
	MDRV_VIDEO_UPDATE( psx )
	MDRV_VIDEO_STOP( psx )

	/* sound hardware */
	MDRV_SOUND_ATTRIBUTES( SOUND_SUPPORTS_STEREO )
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( coh100_primglex )
	/* basic machine hardware */
	MDRV_CPU_ADD( PSXCPU, 33868800 / 2 ) /* 33MHz ?? */
	MDRV_CPU_MEMORY( namcos11_readmem, namcos11_writemem )
	MDRV_CPU_VBLANK_INT( namcos11_vblank, 1 )

	MDRV_FRAMES_PER_SECOND( 60 )
	MDRV_VBLANK_DURATION( 0 )

	MDRV_MACHINE_INIT( namcos11 )
	MDRV_NVRAM_HANDLER( generic_0fill )

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES( VIDEO_TYPE_RASTER )
#if defined( MAME_DEBUG )
	MDRV_SCREEN_SIZE( 1024, 1024 )
	MDRV_VISIBLE_AREA( 0, 1023, 0, 1023 )
#else
	MDRV_SCREEN_SIZE( 640, 240 )
	MDRV_VISIBLE_AREA( 0, 639, 0, 239 )
#endif
	MDRV_PALETTE_LENGTH( 65536 )

	MDRV_PALETTE_INIT( psx )
	MDRV_VIDEO_START( psx_type1_1024x1024 )
	MDRV_VIDEO_UPDATE( psx )
	MDRV_VIDEO_STOP( psx )

	/* sound hardware */
	MDRV_SOUND_ATTRIBUTES( SOUND_SUPPORTS_STEREO )
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( coh110 )
	MDRV_IMPORT_FROM( coh100 )
	MDRV_VIDEO_START( psx_type2_1024x1024 )
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( coh110_danceyes )
	MDRV_IMPORT_FROM( coh100_danceyes )
	MDRV_VIDEO_START( psx_type2_1024x1024 )
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( coh110_primglex )
	MDRV_IMPORT_FROM( coh100_primglex )
	MDRV_VIDEO_START( psx_type2_1024x1024 )
MACHINE_DRIVER_END

INPUT_PORTS_START( namcos11 )
	/* IN 0 */
	PORT_START
	PORT_BIT( 0xf0, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_SERVICE1 )
	PORT_BITX( 0x04, IP_ACTIVE_HIGH, 0, "Test Switch", KEYCODE_F2, IP_JOY_NONE )
	PORT_DIPNAME( 0x02, 0x00, "DIP1 (Test)" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x02, DEF_STR( On ) )
	PORT_DIPNAME( 0x01, 0x00, "DIP2 (Freeze)" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x01, DEF_STR( On ) )

	/* IN 1 */
	PORT_START
	PORT_BIT( 0x0100, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x0200, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_8WAY )
	PORT_BIT( 0x0400, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN | IPF_8WAY )
	PORT_BIT( 0x0800, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP | IPF_8WAY )
	PORT_BIT( 0x1000, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x2000, IP_ACTIVE_HIGH, IPT_BUTTON2 )
	PORT_BIT( 0x4000, IP_ACTIVE_HIGH, IPT_BUTTON3 )
	PORT_BIT( 0x8000, IP_ACTIVE_HIGH, IPT_START1 )

	/* IN 2 */
	PORT_START
	PORT_BIT( 0x0100, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0200, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0400, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0800, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x1000, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x2000, IP_ACTIVE_HIGH, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x4000, IP_ACTIVE_HIGH, IPT_BUTTON3 | IPF_PLAYER2 )
	PORT_BIT( 0x8000, IP_ACTIVE_HIGH, IPT_START2 )

	/* IN 3 */
	PORT_START
	PORT_BIT( 0x0100, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER3 )
	PORT_BIT( 0x0200, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_PLAYER3 )
	PORT_BIT( 0x0400, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_PLAYER3 )
	PORT_BIT( 0x0800, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP | IPF_8WAY | IPF_PLAYER3 )
	PORT_BIT( 0x1000, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_PLAYER3 )
	PORT_BIT( 0x2000, IP_ACTIVE_HIGH, IPT_BUTTON2 | IPF_PLAYER3 )
	PORT_BIT( 0x4000, IP_ACTIVE_HIGH, IPT_BUTTON3 | IPF_PLAYER3 )
	PORT_BIT( 0x8000, IP_ACTIVE_HIGH, IPT_START3 )

	/* IN 4 */
	PORT_START
	PORT_BIT( 0x0100, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER4 )
	PORT_BIT( 0x0200, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_PLAYER4 )
	PORT_BIT( 0x0400, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_PLAYER4 )
	PORT_BIT( 0x0800, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP | IPF_8WAY | IPF_PLAYER4 )
	PORT_BIT( 0x1000, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_PLAYER4 )
	PORT_BIT( 0x2000, IP_ACTIVE_HIGH, IPT_BUTTON2 | IPF_PLAYER4 )
	PORT_BIT( 0x4000, IP_ACTIVE_HIGH, IPT_BUTTON3 | IPF_PLAYER4 )
	PORT_BIT( 0x8000, IP_ACTIVE_HIGH, IPT_START4 )

	/* IN 5 */
	PORT_START
	PORT_BIT( 0xf0, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_COIN3 )
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN4 )
INPUT_PORTS_END

INPUT_PORTS_START( tekken )
	/* IN 0 */
	PORT_START
	PORT_BIT( 0xf0, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_SERVICE1 )
	PORT_BITX( 0x04, IP_ACTIVE_HIGH, 0, "Test Switch", KEYCODE_F2, IP_JOY_NONE )
	PORT_DIPNAME( 0x02, 0x00, "DIP1 (Test)" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x02, DEF_STR( On ) )
	PORT_DIPNAME( 0x01, 0x00, "DIP2 (Freeze)" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x01, DEF_STR( On ) )

	/* IN 1 */
	PORT_START
	PORT_BIT( 0x0100, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x0200, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_8WAY )
	PORT_BIT( 0x0400, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN | IPF_8WAY )
	PORT_BIT( 0x0800, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP | IPF_8WAY )
	PORT_BIT( 0x1000, IP_ACTIVE_HIGH, IPT_BUTTON2 )
	PORT_BIT( 0x2000, IP_ACTIVE_HIGH, IPT_BUTTON3 )
	PORT_BIT( 0x4000, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x8000, IP_ACTIVE_HIGH, IPT_START1 )

	/* IN 2 */
	PORT_START
	PORT_BIT( 0x0100, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0200, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0400, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0800, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x1000, IP_ACTIVE_HIGH, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x2000, IP_ACTIVE_HIGH, IPT_BUTTON3 | IPF_PLAYER2 )
	PORT_BIT( 0x4000, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x8000, IP_ACTIVE_HIGH, IPT_START2 )

	/* IN 3 */
	PORT_START
	PORT_BIT( 0x0100, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x0200, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x0400, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x0800, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x1000, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x2000, IP_ACTIVE_HIGH, IPT_BUTTON4 )
	PORT_BIT( 0x4000, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x8000, IP_ACTIVE_HIGH, IPT_UNUSED )

	/* IN 4 */
	PORT_START
	PORT_BIT( 0x0100, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x0200, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x0400, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x0800, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x1000, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x2000, IP_ACTIVE_HIGH, IPT_BUTTON4 | IPF_PLAYER2 )
	PORT_BIT( 0x4000, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x8000, IP_ACTIVE_HIGH, IPT_UNUSED )

	/* IN 5 */
	PORT_START
	PORT_BIT( 0xf0, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_COIN3 )
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN4 )
INPUT_PORTS_END

INPUT_PORTS_START( souledge )
	/* IN 0 */
	PORT_START
	PORT_BIT( 0xf0, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_SERVICE1 )
	PORT_BITX( 0x04, IP_ACTIVE_HIGH, 0, "Test Switch", KEYCODE_F2, IP_JOY_NONE )
	PORT_DIPNAME( 0x02, 0x00, "DIP1 (Test)" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x02, DEF_STR( On ) )
	PORT_DIPNAME( 0x01, 0x00, "DIP2 (Freeze)" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x01, DEF_STR( On ) )

	/* IN 1 */
	PORT_START
	PORT_BIT( 0x0100, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x0200, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_8WAY )
	PORT_BIT( 0x0400, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN | IPF_8WAY )
	PORT_BIT( 0x0800, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP | IPF_8WAY )
	PORT_BIT( 0x1000, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x2000, IP_ACTIVE_HIGH, IPT_BUTTON2 )
	PORT_BIT( 0x4000, IP_ACTIVE_HIGH, IPT_BUTTON3 )
	PORT_BIT( 0x8000, IP_ACTIVE_HIGH, IPT_START1 )

	/* IN 2 */
	PORT_START
	PORT_BIT( 0x0100, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0200, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0400, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0800, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x1000, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x2000, IP_ACTIVE_HIGH, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x4000, IP_ACTIVE_HIGH, IPT_BUTTON3 | IPF_PLAYER2 )
	PORT_BIT( 0x8000, IP_ACTIVE_HIGH, IPT_START2 )

	/* IN 3 */
	PORT_START
	PORT_BIT( 0x0100, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x0200, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x0400, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x0800, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x1000, IP_ACTIVE_HIGH, IPT_BUTTON4 )
	PORT_BIT( 0x2000, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x4000, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x8000, IP_ACTIVE_HIGH, IPT_UNUSED )

	/* IN 4 */
	PORT_START
	PORT_BIT( 0x0100, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x0200, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x0400, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x0800, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x1000, IP_ACTIVE_HIGH, IPT_BUTTON4 | IPF_PLAYER2 )
	PORT_BIT( 0x2000, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x4000, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x8000, IP_ACTIVE_HIGH, IPT_UNUSED )

	/* IN 5 */
	PORT_START
	PORT_BIT( 0xf0, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_COIN3 )
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN4 )
INPUT_PORTS_END

INPUT_PORTS_START( myangel3 )
	/* IN 0 */
	PORT_START
	PORT_BIT( 0xf0, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_SERVICE1 )
	PORT_BITX( 0x04, IP_ACTIVE_HIGH, 0, "Test Switch", KEYCODE_F2, IP_JOY_NONE )
	PORT_DIPNAME( 0x02, 0x00, "DIP1 (Test)" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x02, DEF_STR( On ) )
	PORT_DIPNAME( 0x01, 0x00, "DIP2 (Freeze)" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x01, DEF_STR( On ) )

	/* IN 1 */
	PORT_START
	PORT_BIT( 0x8000, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x4000, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x2000, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x1000, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x0800, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x0400, IP_ACTIVE_HIGH, IPT_BUTTON2 )
	PORT_BIT( 0x0200, IP_ACTIVE_HIGH, IPT_BUTTON3 )
	PORT_BIT( 0x0100, IP_ACTIVE_HIGH, IPT_BUTTON4 )

	/* IN 2 */
	PORT_START
	PORT_BIT( 0x8000, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x4000, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x2000, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x1000, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x0800, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x0400, IP_ACTIVE_HIGH, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x0200, IP_ACTIVE_HIGH, IPT_BUTTON3 | IPF_PLAYER2 )
	PORT_BIT( 0x0100, IP_ACTIVE_HIGH, IPT_BUTTON4 | IPF_PLAYER2 )

	/* IN 3 */
	PORT_START
	PORT_BIT( 0x8000, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x4000, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x2000, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x1000, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x0800, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x0400, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x0200, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x0100, IP_ACTIVE_HIGH, IPT_UNUSED )

	/* IN 4 */
	PORT_START
	PORT_BIT( 0x8000, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x4000, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x2000, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x1000, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x0800, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x0400, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x0200, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x0100, IP_ACTIVE_HIGH, IPT_UNUSED )

	/* IN 5 */
	PORT_START
	PORT_BIT( 0xf0, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_UNUSED )
INPUT_PORTS_END


ROM_START( danceyes )
	ROM_REGION( 0x0400000, REGION_CPU1, 0 ) /* main ram */
	ROM_REGION( 0x0000400, REGION_USER1, 0 ) /* scratchpad */

	ROM_REGION32_LE( 0x0400000, REGION_USER2, 0 ) /* main prg */
	ROM_LOAD16_BYTE( "dc1vera.2l",   0x0000000, 0x100000, CRC(b164ad67) SHA1(62a7f9bc7fb9c218e5031598512dbd1e013283b3) )
	ROM_LOAD16_BYTE( "dc1vera.2j",   0x0000001, 0x100000, CRC(28e4cb3d) SHA1(17923c66725da1f8e77b7c08d8017160bba53eb9) )
	ROM_LOAD16_BYTE( "dc1vera.2k",   0x0200000, 0x100000, CRC(bdd9484e) SHA1(2f85e0ac4b12b2cf4c9717fad745d2d68c27d39a) )
	ROM_LOAD16_BYTE( "dc1vera.2f",   0x0200001, 0x100000, CRC(25a2f06f) SHA1(400806a262681cf908ac16e039686b5a0d5fd58e) )

	ROM_REGION32_LE( 0x1000000, REGION_USER3, 0 ) /* main data */
	ROM_LOAD16_BYTE( "dc1rm0l.5",    0x0000000, 0x200000, CRC(8b5b4b13) SHA1(9681e29ad95a2cc555d0fbb558a0865a87a77268) )
	ROM_LOAD16_BYTE( "dc1rm0u.6",    0x0000001, 0x200000, CRC(93ca9bd0) SHA1(c7d2ecbeb451bd57097fb39f21dc347b86a2b838) )
	ROM_LOAD16_BYTE( "dc1rm1l.3",    0x0400000, 0x200000, CRC(380e0282) SHA1(33389e3b15b93fb939991b8d7dc8b182ba88e78a) )
	ROM_LOAD16_BYTE( "dc1rm1u.8",    0x0400001, 0x200000, CRC(47d966a7) SHA1(3b6f004136bd9d75dbef846f417ca34a56893d61) )
	ROM_LOAD16_BYTE( "dc1rm2l.4",    0x0800000, 0x200000, CRC(8f130220) SHA1(7f086d502178c27511c1142254381a9a2a999e8d) )
	ROM_LOAD16_BYTE( "dc1rm2u.7",    0x0800001, 0x200000, CRC(24514dc6) SHA1(f9f7d6a45cbd51513cc038f6321a30c1a72b7a58) )
	ROM_LOAD16_BYTE( "dc1rm3l.1",    0x0c00000, 0x200000, CRC(a76bcd4c) SHA1(817abdc43158b7aaac329c3ea17782277acb36a4) )
	ROM_LOAD16_BYTE( "dc1rm3u.9",    0x0c00001, 0x200000, CRC(1405d123) SHA1(3d7be5558358740f5a0a3a3022543cf5aca4cf24) )

	ROM_REGION( 0x0040000, REGION_CPU2, 0 ) /* sound prg */
	ROM_LOAD( "dc1sprg.6d",   0x0000000, 0x040000, CRC(96cd7788) SHA1(68a5a53a5fc50e2b6b684c99d27d81e3a8c56287) )

	ROM_REGION( 0x0400000, REGION_SOUND1, 0 ) /* samples */
	ROM_LOAD( "dc1wave.8k",   0x0000000, 0x400000, CRC(8ba0f6a7) SHA1(e9868debd808e92b196d1baeeeae9c4855356a01) )
ROM_END

ROM_START( dunkmnia )
	ROM_REGION( 0x0400000, REGION_CPU1, 0 ) /* main ram */
	ROM_REGION( 0x0000400, REGION_USER1, 0 ) /* scratchpad */

	ROM_REGION32_LE( 0x0400000, REGION_USER2, 0 ) /* main prg */
	ROM_LOAD16_BYTE( "dm1verc.2l",   0x0000000, 0x100000, CRC(6c81654a) SHA1(00e84cc41b3dd49a8f0d3a364ea4a5d2662c45ff) )
	ROM_LOAD16_BYTE( "dm1verc.2j",   0x0000001, 0x100000, CRC(10329b7e) SHA1(d214764e90c7d79abea01580e79092e34a58b695) )
	ROM_LOAD16_BYTE( "dm1verc.2k",   0x0200000, 0x100000, CRC(c8d72f78) SHA1(30341301f0d1cdcb14f3e0672ec1165c0583fbc8) )
	ROM_LOAD16_BYTE( "dm1verc.2f",   0x0200001, 0x100000, CRC(d379dfa9) SHA1(142cb70b5ea060c961c5bc60a624643b5ec390df) )

	ROM_REGION32_LE( 0x1000000, REGION_USER3, 0 ) /* main data */
	ROM_LOAD16_BYTE( "dm1rm0l.5",    0x0000000, 0x200000, CRC(4bb5d71d) SHA1(7d61211c7a6d1f6593604390fe99206a6a8cc7b3) )
	ROM_CONTINUE( 0x0000000, 0x200000 ) /* first & second half identical */
	ROM_LOAD16_BYTE( "dm1rm0u.6",    0x0000001, 0x200000, CRC(c16b47c5) SHA1(0fb2c5bc4674b3366762127c6333fb3a837b4de2) )
	ROM_CONTINUE( 0x0000001, 0x200000 ) /* first & second half identical */
	ROM_LOAD16_BYTE( "dm1rm1l.3",    0x0400000, 0x200000, CRC(20dd3294) SHA1(b2fd5075b6281ac7bfc2681fc282f9ebaa089af5) )
	ROM_CONTINUE( 0x0400000, 0x200000 ) /* first & second half identical */
	ROM_LOAD16_BYTE( "dm1rm1u.8",    0x0400001, 0x200000, CRC(01e905d3) SHA1(430b2ae0c67265b6acc8aa4dd50f6144929993f8) )
	ROM_CONTINUE( 0x0400001, 0x200000 ) /* first & second half identical */

	ROM_REGION( 0x0040000, REGION_CPU2, 0 ) /* sound prg */
	ROM_LOAD( "dm1sprg.6d",   0x0000000, 0x040000, CRC(de1cbc78) SHA1(855ebece1841f50ae324d7d6b8b18ab6f657d28e) )

	ROM_REGION( 0x0400000, REGION_SOUND1, 0 ) /* samples */
	ROM_LOAD( "dm1wave.8k",   0x0000000, 0x400000, CRC(4891d53e) SHA1(a1fee060e94d3219174b5974517f4fd3be32aaa5) )
ROM_END

ROM_START( myangel3 )
	ROM_REGION( 0x0400000, REGION_CPU1, 0 ) /* main ram */
	ROM_REGION( 0x0000400, REGION_USER1, 0 ) /* scratchpad */

	ROM_REGION32_LE( 0x0400000, REGION_USER2, 0 ) /* main prg */
	ROM_LOAD( "kqt1vera.1j",         0x0000000, 0x200000, CRC(df7aef8a) SHA1(d4ff144bcdecc1d4a3b834d0b9c182609ad9b260) )
	ROM_LOAD( "kqt1vera.1l",         0x0200000, 0x200000, CRC(ffc51c01) SHA1(bba2c2c1ad31039c7dc7413e51e7fc317451e1e3) )

	ROM_REGION32_LE( 0x2000000, REGION_USER3, 0 ) /* main data */
	ROM_LOAD16_BYTE( "kqt1pr0l.bin", 0x0000000, 0x800000, CRC(d67eee66) SHA1(1842e987ae406ac2cf4c173aaaa73b5f67f4fd3d) )
	ROM_LOAD16_BYTE( "kqt1pr0u.bin", 0x0000001, 0x800000, CRC(4d1c7bf3) SHA1(6bf9712264a82920f7898896643d05a6b425e74a) )
	ROM_LOAD16_BYTE( "kqt1pr1l.bin", 0x1000000, 0x800000, CRC(298d8eeb) SHA1(c421b1bdd5fd46c026a41e2cec47cafd1a69d33d) )
	ROM_LOAD16_BYTE( "kqt1pr1u.bin", 0x1000001, 0x800000, CRC(911783db) SHA1(1005fc9b38e212844e397150a6f98f43ad88d4b9) )

	ROM_REGION( 0x0040000, REGION_CPU2, 0 ) /* sound prg */
	ROM_LOAD( "kqt1sprg.7e",  0x0000000, 0x040000, CRC(bb1888a6) SHA1(4db07738079725413cdba7eb75252ee71ae50a66) )

	ROM_REGION( 0x0400000, REGION_SOUND1, 0 ) /* samples */
	ROM_LOAD( "kqt1wave.bin", 0x0000000, 0x400000, CRC(92ca8e4f) SHA1(48d6bdfcc5de1c280afa36c3f0dd6d4177771355) )
ROM_END

ROM_START( primglex )
	ROM_REGION( 0x0400000, REGION_CPU1, 0 ) /* main ram */
	ROM_REGION( 0x0000400, REGION_USER1, 0 ) /* scratchpad */

	ROM_REGION32_LE( 0x0400000, REGION_USER2, 0 ) /* main prg */
	ROM_LOAD16_BYTE( "pg1vera.2l",   0x0000000, 0x100000, CRC(fc15fd1a) SHA1(6ca5ebdc096cab3296dc7c1f675d78dfc7c69a05) )
	ROM_LOAD16_BYTE( "pg1vera.2j",   0x0000001, 0x100000, CRC(79955553) SHA1(ad2dca38b06a835f8241fae0a5fa18d5874cebe4) )

	ROM_REGION32_LE( 0x1000000, REGION_USER3, 0 ) /* main data */
	ROM_LOAD16_BYTE( "pg1rm0u.5",    0x0000000, 0x200000, CRC(2a503f2f) SHA1(206b9c9204be22241d2a3e017b96c3a103f5a976) )
	ROM_CONTINUE( 0x0000000, 0x200000 ) /* first & second half identical */
	ROM_LOAD16_BYTE( "pg1rm0l.6",    0x0000001, 0x200000, CRC(54cef992) SHA1(5ba81353b1dddc1c6640fc4c15df81535e7a6ae8) )
	ROM_CONTINUE( 0x0000001, 0x200000 ) /* first & second half identical */
	ROM_LOAD16_BYTE( "pg1rm1u.3",    0x0400000, 0x200000, CRC(1ee41152) SHA1(d240e6ba820aa2aa4f12380c255f624f91aed564) )
	ROM_CONTINUE( 0x0400000, 0x200000 ) /* first & second half identical */
	ROM_LOAD16_BYTE( "pg1rm1l.8",    0x0400001, 0x200000, CRC(59b5a71c) SHA1(ddc1f0a5488466166c21fd0c84ab2b4cf04316bf) )
	ROM_CONTINUE( 0x0400001, 0x200000 ) /* first & second half identical */

	ROM_REGION( 0x0040000, REGION_CPU2, 0 ) /* sound prg */
	ROM_LOAD( "pg1sprg.6d",   0x0000000, 0x040000, CRC(e7c3396d) SHA1(12bbb8ebcaab1b40462a12917dd9b58bd9ab8663) )

	ROM_REGION( 0x0400000, REGION_SOUND1, 0 ) /* samples */
	ROM_LOAD( "pg1wave.8k",   0x0000000, 0x400000, CRC(fc9ad9eb) SHA1(ce5bb2288ed8cf1348825c39423cbb99d9324b9c) )
ROM_END

ROM_START( souledge )
	ROM_REGION( 0x0400000, REGION_CPU1, 0 ) /* main ram */
	ROM_REGION( 0x0000400, REGION_USER1, 0 ) /* scratchpad */

	ROM_REGION32_LE( 0x0400000, REGION_USER2, 0 ) /* main prg */
	ROM_LOAD16_BYTE( "so4verc.2l",   0x0000000, 0x100000, CRC(12b8ae0d) SHA1(31571023d5b77ebcd4103b8cac5ba710a3d570a0) )
	ROM_LOAD16_BYTE( "so4verc.2j",   0x0000001, 0x100000, CRC(938262b0) SHA1(e806883e32c473a3c2bb07849126631f6d66fa66) )
	ROM_LOAD16_BYTE( "so1verc.2k",   0x0200000, 0x100000, CRC(1789e399) SHA1(2e11869124cdfa7a66107b0e642bdc72ee0c306a) )
	ROM_LOAD16_BYTE( "so1verc.2f",   0x0200001, 0x100000, CRC(8cffe1c3) SHA1(d54a0b1d55f33db2890bfa70c411cca3e446fccf) )

	ROM_REGION32_LE( 0x1000000, REGION_USER3, 0 ) /* main data */
	ROM_LOAD16_BYTE( "so1rm0u.5",    0x0000000, 0x200000, CRC(e364d673) SHA1(70fd58fb912939c57e3a5fadd01c1467df08d4ac) )
	ROM_LOAD16_BYTE( "so1rm0l.6",    0x0000001, 0x200000, CRC(9c5b0858) SHA1(f3ac726f1167551beea7edc46e43b826b7baaf62) )
	ROM_LOAD16_BYTE( "so1rm1u.3",    0x0400000, 0x200000, CRC(8f9d8c5b) SHA1(ac1da70854eee344a645749f564366ceac571767) )
	ROM_LOAD16_BYTE( "so1rm1l.8",    0x0400001, 0x200000, CRC(4406ef16) SHA1(c86f199fdb1db23e5944ca51e6518b9cd0dafb71) )
	ROM_LOAD16_BYTE( "so1rm2u.4",    0x0800000, 0x200000, CRC(b4baa886) SHA1(0432692a4d71a3f1b47707efb6858927744940e4) )
	ROM_LOAD16_BYTE( "so1rm2l.7",    0x0800001, 0x200000, CRC(37c1f66e) SHA1(13a8a73fce142ea5ebe3f0c1050e44a027ab42a6) )
	ROM_LOAD16_BYTE( "so1rm3u.1",    0x0c00000, 0x200000, CRC(f11bd521) SHA1(baf936dec58cebfeef1c74f95e455b2fe74eb982) )
	ROM_LOAD16_BYTE( "so1rm3l.9",    0x0c00001, 0x200000, CRC(84465bcc) SHA1(d8be888d41cfe194c3a1853d9146d3a74ef7bab1) )

	ROM_REGION( 0x0040000, REGION_CPU2, 0 ) /* sound prg */
	ROM_LOAD( "so1sprc.6d",   0x0000000, 0x040000, CRC(2bbc118c) SHA1(4168a9aa525f1f0ce6cf6e14cfe4c118c4c0d773) )

	ROM_REGION( 0x0400000, REGION_SOUND1, 0 ) /* samples */
	ROM_LOAD( "so1wave.8k",   0x0000000, 0x400000, CRC(0e68836b) SHA1(c392b370a807803c7ab060105861253e1b407f49) )
ROM_END

ROM_START( souledga )
	ROM_REGION( 0x0400000, REGION_CPU1, 0 ) /* main ram */
	ROM_REGION( 0x0000400, REGION_USER1, 0 ) /* scratchpad */

	ROM_REGION32_LE( 0x0400000, REGION_USER2, 0 ) /* main prg */
	ROM_LOAD16_BYTE( "so3vera.2l",   0x0000000, 0x100000, CRC(19b39096) SHA1(9188cd1fd9b15e5545eb41ea2768a8bd42113379) )
	ROM_LOAD16_BYTE( "so3vera.2j",   0x0000001, 0x100000, CRC(09eda46f) SHA1(24d04d2ba51af508ddc0656e8bb5e1335b08cc8a) )
	ROM_LOAD16_BYTE( "so1vera.2k",   0x0200000, 0x100000, CRC(29bdc6bb) SHA1(9047792c1a21a0001024939f1c6a8c3d86948973) )
	ROM_LOAD16_BYTE( "so1vera.2f",   0x0200001, 0x100000, CRC(c035b71b) SHA1(38719a75193774b124d845460c0c03d36849719d) )

	ROM_REGION32_LE( 0x1000000, REGION_USER3, 0 ) /* main data */
	ROM_LOAD16_BYTE( "so1rm0u.5",    0x0000000, 0x200000, CRC(e364d673) SHA1(70fd58fb912939c57e3a5fadd01c1467df08d4ac) )
	ROM_LOAD16_BYTE( "so1rm0l.6",    0x0000001, 0x200000, CRC(9c5b0858) SHA1(f3ac726f1167551beea7edc46e43b826b7baaf62) )
	ROM_LOAD16_BYTE( "so1rm1u.3",    0x0400000, 0x200000, CRC(8f9d8c5b) SHA1(ac1da70854eee344a645749f564366ceac571767) )
	ROM_LOAD16_BYTE( "so1rm1l.8",    0x0400001, 0x200000, CRC(4406ef16) SHA1(c86f199fdb1db23e5944ca51e6518b9cd0dafb71) )
	ROM_LOAD16_BYTE( "so1rm2u.4",    0x0800000, 0x200000, CRC(b4baa886) SHA1(0432692a4d71a3f1b47707efb6858927744940e4) )
	ROM_LOAD16_BYTE( "so1rm2l.7",    0x0800001, 0x200000, CRC(37c1f66e) SHA1(13a8a73fce142ea5ebe3f0c1050e44a027ab42a6) )
	ROM_LOAD16_BYTE( "so1rm3u.1",    0x0c00000, 0x200000, CRC(f11bd521) SHA1(baf936dec58cebfeef1c74f95e455b2fe74eb982) )
	ROM_LOAD16_BYTE( "so1rm3l.9",    0x0c00001, 0x200000, CRC(84465bcc) SHA1(d8be888d41cfe194c3a1853d9146d3a74ef7bab1) )

	ROM_REGION( 0x0040000, REGION_CPU2, 0 ) /* sound prg */
	ROM_LOAD( "so1sprg.6d",   0x0000000, 0x040000, CRC(f6f682b7) SHA1(a64e19be3f6e630b8c34f34b46b95aadfabd3f63) )

	ROM_REGION( 0x0400000, REGION_SOUND1, 0 ) /* samples */
	ROM_LOAD( "so1wave.8k",   0x0000000, 0x400000, CRC(0e68836b) SHA1(c392b370a807803c7ab060105861253e1b407f49) )
ROM_END

ROM_START( souledgb )
	ROM_REGION( 0x0400000, REGION_CPU1, 0 ) /* main ram */
	ROM_REGION( 0x0000400, REGION_USER1, 0 ) /* scratchpad */

	ROM_REGION32_LE( 0x0400000, REGION_USER2, 0 ) /* main prg */
	ROM_LOAD16_BYTE( "so1vera.2l",   0x0000000, 0x100000, CRC(bafb94c8) SHA1(92461ce74a537a9801a024280e18fc3d0a5e6e5c) )
	ROM_LOAD16_BYTE( "so1vera.2j",   0x0000001, 0x100000, CRC(abe2d28e) SHA1(e9d858c8f8651b04bc72eb6de423da4925e94250) )
	ROM_LOAD16_BYTE( "so1vera.2k",   0x0200000, 0x100000, CRC(29bdc6bb) SHA1(9047792c1a21a0001024939f1c6a8c3d86948973) )
	ROM_LOAD16_BYTE( "so1vera.2f",   0x0200001, 0x100000, CRC(c035b71b) SHA1(38719a75193774b124d845460c0c03d36849719d) )

	ROM_REGION32_LE( 0x1000000, REGION_USER3, 0 ) /* main data */
	ROM_LOAD16_BYTE( "so1rm0u.5",    0x0000000, 0x200000, CRC(e364d673) SHA1(70fd58fb912939c57e3a5fadd01c1467df08d4ac) )
	ROM_LOAD16_BYTE( "so1rm0l.6",    0x0000001, 0x200000, CRC(9c5b0858) SHA1(f3ac726f1167551beea7edc46e43b826b7baaf62) )
	ROM_LOAD16_BYTE( "so1rm1u.3",    0x0400000, 0x200000, CRC(8f9d8c5b) SHA1(ac1da70854eee344a645749f564366ceac571767) )
	ROM_LOAD16_BYTE( "so1rm1l.8",    0x0400001, 0x200000, CRC(4406ef16) SHA1(c86f199fdb1db23e5944ca51e6518b9cd0dafb71) )
	ROM_LOAD16_BYTE( "so1rm2u.4",    0x0800000, 0x200000, CRC(b4baa886) SHA1(0432692a4d71a3f1b47707efb6858927744940e4) )
	ROM_LOAD16_BYTE( "so1rm2l.7",    0x0800001, 0x200000, CRC(37c1f66e) SHA1(13a8a73fce142ea5ebe3f0c1050e44a027ab42a6) )
	ROM_LOAD16_BYTE( "so1rm3u.1",    0x0c00000, 0x200000, CRC(f11bd521) SHA1(baf936dec58cebfeef1c74f95e455b2fe74eb982) )
	ROM_LOAD16_BYTE( "so1rm3l.9",    0x0c00001, 0x200000, CRC(84465bcc) SHA1(d8be888d41cfe194c3a1853d9146d3a74ef7bab1) )

	ROM_REGION( 0x0040000, REGION_CPU2, 0 ) /* sound prg */
	ROM_LOAD( "so1sprg.6d",   0x0000000, 0x040000, CRC(f6f682b7) SHA1(a64e19be3f6e630b8c34f34b46b95aadfabd3f63) )

	ROM_REGION( 0x0400000, REGION_SOUND1, 0 ) /* samples */
	ROM_LOAD( "so1wave.8k",   0x0000000, 0x400000, CRC(0e68836b) SHA1(c392b370a807803c7ab060105861253e1b407f49) )
ROM_END

ROM_START( starswep )
	ROM_REGION( 0x0400000, REGION_CPU1, 0 ) /* main ram */
	ROM_REGION( 0x0000400, REGION_USER1, 0 ) /* scratchpad */

	ROM_REGION32_LE( 0x0400000, REGION_USER2, 0 ) /* main prg */
	ROM_LOAD( "stp1vera.1j",         0x0000000, 0x200000, CRC(ef83e126) SHA1(f721b43358cedad0f28af5d2b292b44043fd47a0) )
	ROM_LOAD( "stp1vera.1l",         0x0200000, 0x200000, CRC(0ee7fe1e) SHA1(8c2f5b0e7b49dbe0e8105bf55c493acd46a4f59d) )

	ROM_REGION32_LE( 0x0100000, REGION_USER3, 0 ) /* main data */

	ROM_REGION( 0x0040000, REGION_CPU2, 0 ) /* sound prg */
	ROM_LOAD( "stp1sprg.7e",  0x0000000, 0x040000, CRC(08aaaf6a) SHA1(51c913a39ff7c154aef8bb10139cc8b92eb4756a) )

	ROM_REGION( 0x0400000, REGION_SOUND1, 0 ) /* samples */
	ROM_LOAD( "stp1wave.bin", 0x0000000, 0x400000, CRC(18f30e92) SHA1(b3819455856298527a7224495f541145aecf23dd) )
ROM_END

ROM_START( tekken )
	ROM_REGION( 0x0400000, REGION_CPU1, 0 ) /* main ram */
	ROM_REGION( 0x0000400, REGION_USER1, 0 ) /* scratchpad */

	ROM_REGION32_LE( 0x0400000, REGION_USER2, 0 ) /* main prg */
	ROM_LOAD16_BYTE( "te4verc.2l",   0x0000000, 0x100000, CRC(7ecb7892) SHA1(7837f2b3dbfc6b4a153ea652e8a3fd89c4daa26e) )
	ROM_LOAD16_BYTE( "te4verc.2j",   0x0000001, 0x100000, CRC(eea3365d) SHA1(d13df90833aac48f9d9d20cddefb81f90ebab249) )
	ROM_LOAD16_BYTE( "te1verb.2k",   0x0200000, 0x100000, CRC(b9860b29) SHA1(678889fc5c70bf66f0bd9864a20636ffb620ed0d) )
	ROM_LOAD16_BYTE( "te1verb.2f",   0x0200001, 0x100000, CRC(3dc01aad) SHA1(266f346fa575c42b635bc469798f5aade9821e20) )

	ROM_REGION32_LE( 0x1000000, REGION_USER3, 0 ) /* main data */
	ROM_LOAD16_BYTE( "te1rm0l.5",    0x0000000, 0x200000, CRC(03786e09) SHA1(de2b9e19ace43c424d100dc5d3207217f66d6479) )
	ROM_LOAD16_BYTE( "te1rm0u.6",    0x0000001, 0x200000, CRC(75d91051) SHA1(1c7958162315576c3881dcc684b85710f7f19cd6) )
	ROM_LOAD16_BYTE( "te1rm1l.3",    0x0400000, 0x200000, CRC(81416f8e) SHA1(b42ff08ee84491c57a7c87bb767db7e2ec7a26c2) )
	ROM_LOAD16_BYTE( "te1rm1u.8",    0x0400001, 0x200000, CRC(fa7ba433) SHA1(e222c4b1631c09e58546446a1e82c16fca936f1d) )
	ROM_LOAD16_BYTE( "te1rm2l.4",    0x0800000, 0x200000, CRC(41d77846) SHA1(eeab049135c02a255899fe37e225c1111b2fbb7d) )
	ROM_LOAD16_BYTE( "te1rm2u.7",    0x0800001, 0x200000, CRC(a678987e) SHA1(c62c00ce5cf4d001723c999b2bc3dbb90283def1) )

	ROM_REGION( 0x0040000, REGION_CPU2, 0 ) /* sound prg */
	ROM_LOAD( "te1sprg.6d",   0x0000000, 0x040000, CRC(849587e9) SHA1(94c6a757b24758a866a41bd8acd46aa46844f74b) )

	ROM_REGION( 0x0400000, REGION_SOUND1, 0 ) /* samples */
	ROM_LOAD( "te1wave.8k",   0x0000000, 0x200000, CRC(fce6c57a) SHA1(7fb8c69452c92c59a940a2b69d0d73ef7aefcb82) )
ROM_END

ROM_START( tekkena )
	ROM_REGION( 0x0400000, REGION_CPU1, 0 ) /* main ram */
	ROM_REGION( 0x0000400, REGION_USER1, 0 ) /* scratchpad */

	ROM_REGION32_LE( 0x0400000, REGION_USER2, 0 ) /* main prg */
	ROM_LOAD16_BYTE( "te2verb.2l",   0x0000000, 0x100000, CRC(246cfbdd) SHA1(c1bffe3c463e9eff6dcaf9937da72bff8ff33e4e) )
	ROM_LOAD16_BYTE( "te2verb.2j",   0x0000001, 0x100000, CRC(dfa83e47) SHA1(a29d852c1b6a52c043248d7e5af04067dfa3eb40) )
	ROM_LOAD16_BYTE( "te1verb.2k",   0x0200000, 0x100000, CRC(b9860b29) SHA1(678889fc5c70bf66f0bd9864a20636ffb620ed0d) )
	ROM_LOAD16_BYTE( "te1verb.2f",   0x0200001, 0x100000, CRC(3dc01aad) SHA1(266f346fa575c42b635bc469798f5aade9821e20) )

	ROM_REGION32_LE( 0x1000000, REGION_USER3, 0 ) /* main data */
	ROM_LOAD16_BYTE( "te1rm0l.5",    0x0000000, 0x200000, CRC(03786e09) SHA1(de2b9e19ace43c424d100dc5d3207217f66d6479) )
	ROM_LOAD16_BYTE( "te1rm0u.6",    0x0000001, 0x200000, CRC(75d91051) SHA1(1c7958162315576c3881dcc684b85710f7f19cd6) )
	ROM_LOAD16_BYTE( "te1rm1l.3",    0x0400000, 0x200000, CRC(81416f8e) SHA1(b42ff08ee84491c57a7c87bb767db7e2ec7a26c2) )
	ROM_LOAD16_BYTE( "te1rm1u.8",    0x0400001, 0x200000, CRC(fa7ba433) SHA1(e222c4b1631c09e58546446a1e82c16fca936f1d) )
	ROM_LOAD16_BYTE( "te1rm2l.4",    0x0800000, 0x200000, CRC(41d77846) SHA1(eeab049135c02a255899fe37e225c1111b2fbb7d) )
	ROM_LOAD16_BYTE( "te1rm2u.7",    0x0800001, 0x200000, CRC(a678987e) SHA1(c62c00ce5cf4d001723c999b2bc3dbb90283def1) )

	ROM_REGION( 0x0040000, REGION_CPU2, 0 ) /* sound prg */
	ROM_LOAD( "te1sprg.6d",   0x0000000, 0x040000, CRC(849587e9) SHA1(94c6a757b24758a866a41bd8acd46aa46844f74b) )

	ROM_REGION( 0x0400000, REGION_SOUND1, 0 ) /* samples */
	ROM_LOAD( "te1wave.8k",   0x0000000, 0x200000, CRC(fce6c57a) SHA1(7fb8c69452c92c59a940a2b69d0d73ef7aefcb82) )
ROM_END

ROM_START( tekkenb )
	ROM_REGION( 0x0400000, REGION_CPU1, 0 ) /* main ram */
	ROM_REGION( 0x0000400, REGION_USER1, 0 ) /* scratchpad */

	ROM_REGION32_LE( 0x0400000, REGION_USER2, 0 ) /* main prg */
	ROM_LOAD16_BYTE( "te1verb.2l",   0x0000000, 0x100000, CRC(4291afee) SHA1(2e04a6c786544176e2b7e22b5f469e3548896b19) )
	ROM_LOAD16_BYTE( "te1verb.2j",   0x0000001, 0x100000, CRC(5c534705) SHA1(2430b5c36de419822de0283c006c5af2e7cd95ef) )
	ROM_LOAD16_BYTE( "te1verb.2k",   0x0200000, 0x100000, CRC(b9860b29) SHA1(678889fc5c70bf66f0bd9864a20636ffb620ed0d) )
	ROM_LOAD16_BYTE( "te1verb.2f",   0x0200001, 0x100000, CRC(3dc01aad) SHA1(266f346fa575c42b635bc469798f5aade9821e20) )

	ROM_REGION32_LE( 0x1000000, REGION_USER3, 0 ) /* main data */
	ROM_LOAD16_BYTE( "te1rm0l.5",    0x0000000, 0x200000, CRC(03786e09) SHA1(de2b9e19ace43c424d100dc5d3207217f66d6479) )
	ROM_LOAD16_BYTE( "te1rm0u.6",    0x0000001, 0x200000, CRC(75d91051) SHA1(1c7958162315576c3881dcc684b85710f7f19cd6) )
	ROM_LOAD16_BYTE( "te1rm1l.3",    0x0400000, 0x200000, CRC(81416f8e) SHA1(b42ff08ee84491c57a7c87bb767db7e2ec7a26c2) )
	ROM_LOAD16_BYTE( "te1rm1u.8",    0x0400001, 0x200000, CRC(fa7ba433) SHA1(e222c4b1631c09e58546446a1e82c16fca936f1d) )
	ROM_LOAD16_BYTE( "te1rm2l.4",    0x0800000, 0x200000, CRC(41d77846) SHA1(eeab049135c02a255899fe37e225c1111b2fbb7d) )
	ROM_LOAD16_BYTE( "te1rm2u.7",    0x0800001, 0x200000, CRC(a678987e) SHA1(c62c00ce5cf4d001723c999b2bc3dbb90283def1) )

	ROM_REGION( 0x0040000, REGION_CPU2, 0 ) /* sound prg */
	ROM_LOAD( "te1sprg.6d",   0x0000000, 0x040000, CRC(849587e9) SHA1(94c6a757b24758a866a41bd8acd46aa46844f74b) )

	ROM_REGION( 0x0400000, REGION_SOUND1, 0 ) /* samples */
	ROM_LOAD( "te1wave.8k",   0x0000000, 0x200000, CRC(fce6c57a) SHA1(7fb8c69452c92c59a940a2b69d0d73ef7aefcb82) )
ROM_END

ROM_START( tekken2 )
	ROM_REGION( 0x0400000, REGION_CPU1, 0 ) /* main ram */
	ROM_REGION( 0x0000400, REGION_USER1, 0 ) /* scratchpad */

	ROM_REGION32_LE( 0x0400000, REGION_USER2, 0 ) /* main prg */
	ROM_LOAD16_BYTE( "tes3verb.2l",  0x0000000, 0x100000, CRC(4692075f) SHA1(d048a92040ceb57ef7462bebc2c1112b964570ec) )
	ROM_LOAD16_BYTE( "tes3verb.2j",  0x0000001, 0x100000, CRC(db3ec640) SHA1(fc9f475232ea77abd2eb7e2e09314281264e9d38) )
	ROM_LOAD16_BYTE( "tes1verb.2k",  0x0200000, 0x100000, CRC(668ca712) SHA1(67100db4c6a3ca62d3f62f6fcef974ce017e2c9e) )
	ROM_LOAD16_BYTE( "tes1verb.2f",  0x0200001, 0x100000, CRC(c4f66a0a) SHA1(1b3dd33d7e6d9122826bf8be0dbbc088e4cc41e8) )

	ROM_REGION32_LE( 0x1000000, REGION_USER3, 0 ) /* main data */
	ROM_LOAD16_BYTE( "tes1rm0l.6",   0x0000000, 0x200000, CRC(fc904ede) SHA1(cea378ba86f94beadb3d67685f1b8c141f478abe) )
	ROM_LOAD16_BYTE( "tes1rm0u.5",   0x0000001, 0x200000, CRC(57b38f5d) SHA1(edb4beab47b1339a5b1bc8071086abfcba57722e) )
	ROM_LOAD16_BYTE( "tes1rm1l.8",   0x0400000, 0x200000, CRC(aa48f04b) SHA1(f7383d2b3a84c4e649a27c0ad1e6af4702ec0a17) )
	ROM_LOAD16_BYTE( "tes1rm1u.3",   0x0400001, 0x200000, CRC(b147c543) SHA1(c4b18c218999ec73d04c92e06fb3e6165ceebf2b) )
	ROM_LOAD16_BYTE( "tes1rm2l.7",   0x0800000, 0x200000, CRC(b08da52c) SHA1(31fe2021d0fe37c16555650dd10d26ed80d9b493) )
	ROM_LOAD16_BYTE( "tes1rm2u.4",   0x0800001, 0x200000, CRC(8a1561b8) SHA1(ebc02c9e7033d54aefb5034c97a3c8cd749b5600) )
	ROM_LOAD16_BYTE( "tes1rm3l.9",   0x0c00000, 0x200000, CRC(d5ac0f18) SHA1(342d063f7974bd1f90b5ca4832dfa4fbc9605453) )
	ROM_LOAD16_BYTE( "tes1rm3u.1",   0x0c00001, 0x200000, CRC(44ed509d) SHA1(27e26aaf5ce72ab686f3f05743b1d91b5334b4e0) )

	ROM_REGION( 0x0200000, REGION_CPU2, 0 ) /* sound prg */
	ROM_LOAD( "tes1sprb.6d",  0x0000000, 0x200000, BAD_DUMP CRC(8d89877e) SHA1(7d76d48d64d7ac5411d714a4bb83f37e3e5b8df6) )

	ROM_REGION( 0x0400000, REGION_SOUND1, 0 ) /* samples */
	ROM_LOAD( "tes1wavb.8k",  0x0000000, 0x400000, BAD_DUMP CRC(bee9a7e6) SHA1(c017e4e2ef7bc8193444e2e685a4d8a89ff15ca9) )
ROM_END

ROM_START( tekken2a )
	ROM_REGION( 0x0400000, REGION_CPU1, 0 ) /* main ram */
	ROM_REGION( 0x0000400, REGION_USER1, 0 ) /* scratchpad */

	ROM_REGION32_LE( 0x0400000, REGION_USER2, 0 ) /* main prg */
	ROM_LOAD16_BYTE( "tes2verb.2l",   0x0000000, 0x100000, CRC(aa9a4503) SHA1(e6fdd34216591510593bbda0232ddc0fcd80e80b) )
	ROM_LOAD16_BYTE( "tes2verb.2j",   0x0000001, 0x100000, CRC(63706d8c) SHA1(740cfa8b422fb663bcb412b3389da33c7f9f13be) )
	ROM_LOAD16_BYTE( "tes1verb.2k",   0x0200000, 0x100000, CRC(668ca712) SHA1(67100db4c6a3ca62d3f62f6fcef974ce017e2c9e) )
	ROM_LOAD16_BYTE( "tes1verb.2f",   0x0200001, 0x100000, CRC(c4f66a0a) SHA1(1b3dd33d7e6d9122826bf8be0dbbc088e4cc41e8) )

	ROM_REGION32_LE( 0x1000000, REGION_USER3, 0 ) /* main data */
	ROM_LOAD16_BYTE( "tes1rm0l.6",   0x0000000, 0x200000, CRC(fc904ede) SHA1(cea378ba86f94beadb3d67685f1b8c141f478abe) )
	ROM_LOAD16_BYTE( "tes1rm0u.5",   0x0000001, 0x200000, CRC(57b38f5d) SHA1(edb4beab47b1339a5b1bc8071086abfcba57722e) )
	ROM_LOAD16_BYTE( "tes1rm1l.8",   0x0400000, 0x200000, CRC(aa48f04b) SHA1(f7383d2b3a84c4e649a27c0ad1e6af4702ec0a17) )
	ROM_LOAD16_BYTE( "tes1rm1u.3",   0x0400001, 0x200000, CRC(b147c543) SHA1(c4b18c218999ec73d04c92e06fb3e6165ceebf2b) )
	ROM_LOAD16_BYTE( "tes1rm2l.7",   0x0800000, 0x200000, CRC(b08da52c) SHA1(31fe2021d0fe37c16555650dd10d26ed80d9b493) )
	ROM_LOAD16_BYTE( "tes1rm2u.4",   0x0800001, 0x200000, CRC(8a1561b8) SHA1(ebc02c9e7033d54aefb5034c97a3c8cd749b5600) )
	ROM_LOAD16_BYTE( "tes1rm3l.9",   0x0c00000, 0x200000, CRC(d5ac0f18) SHA1(342d063f7974bd1f90b5ca4832dfa4fbc9605453) )
	ROM_LOAD16_BYTE( "tes1rm3u.1",   0x0c00001, 0x200000, CRC(44ed509d) SHA1(27e26aaf5ce72ab686f3f05743b1d91b5334b4e0) )

	ROM_REGION( 0x0200000, REGION_CPU2, 0 ) /* sound prg */
	ROM_LOAD( "tes1sprb.6d",  0x0000000, 0x200000, BAD_DUMP CRC(8d89877e) SHA1(7d76d48d64d7ac5411d714a4bb83f37e3e5b8df6) )

	ROM_REGION( 0x0400000, REGION_SOUND1, 0 ) /* samples */
	ROM_LOAD( "tes1wavb.8k",  0x0000000, 0x400000, BAD_DUMP CRC(bee9a7e6) SHA1(c017e4e2ef7bc8193444e2e685a4d8a89ff15ca9) )
ROM_END

ROM_START( tekken2b )
	ROM_REGION( 0x0400000, REGION_CPU1, 0 ) /* main ram */
	ROM_REGION( 0x0000400, REGION_USER1, 0 ) /* scratchpad */

	ROM_REGION32_LE( 0x0400000, REGION_USER2, 0 ) /* main prg */
	ROM_LOAD16_BYTE( "tes2vera.2l",  0x0000000, 0x100000, CRC(8bb82bf0) SHA1(ac4e0077dff4c46ea2435903c410590f91cafe7d) )
	ROM_LOAD16_BYTE( "tes2vera.2j",  0x0000001, 0x100000, CRC(4e02f921) SHA1(15339c2626033912947d33e5f59a109e607be0bf) )
	ROM_LOAD16_BYTE( "tes1vera.2k",  0x0200000, 0x100000, CRC(78e2ce1a) SHA1(fb242725dc72fa234bd7df81cec57fe010cf58f3) )
	ROM_LOAD16_BYTE( "tes1vera.2f",  0x0200001, 0x100000, CRC(fbb0b146) SHA1(08b11ac0fbfeed62910c5cb5ff7b5939ecbca142) )

	ROM_REGION32_LE( 0x1000000, REGION_USER3, 0 ) /* main data */
	ROM_LOAD16_BYTE( "tes1rm0l.6",   0x0000000, 0x200000, CRC(fc904ede) SHA1(cea378ba86f94beadb3d67685f1b8c141f478abe) )
	ROM_LOAD16_BYTE( "tes1rm0u.5",   0x0000001, 0x200000, CRC(57b38f5d) SHA1(edb4beab47b1339a5b1bc8071086abfcba57722e) )
	ROM_LOAD16_BYTE( "tes1rm1l.8",   0x0400000, 0x200000, CRC(aa48f04b) SHA1(f7383d2b3a84c4e649a27c0ad1e6af4702ec0a17) )
	ROM_LOAD16_BYTE( "tes1rm1u.3",   0x0400001, 0x200000, CRC(b147c543) SHA1(c4b18c218999ec73d04c92e06fb3e6165ceebf2b) )
	ROM_LOAD16_BYTE( "tes1rm2l.7",   0x0800000, 0x200000, CRC(b08da52c) SHA1(31fe2021d0fe37c16555650dd10d26ed80d9b493) )
	ROM_LOAD16_BYTE( "tes1rm2u.4",   0x0800001, 0x200000, CRC(8a1561b8) SHA1(ebc02c9e7033d54aefb5034c97a3c8cd749b5600) )
	ROM_LOAD16_BYTE( "tes1rm3l.9",   0x0c00000, 0x200000, CRC(d5ac0f18) SHA1(342d063f7974bd1f90b5ca4832dfa4fbc9605453) )
	ROM_LOAD16_BYTE( "tes1rm3u.1",   0x0c00001, 0x200000, CRC(44ed509d) SHA1(27e26aaf5ce72ab686f3f05743b1d91b5334b4e0) )

	ROM_REGION( 0x0040000, REGION_CPU2, 0 ) /* sound prg */
	ROM_LOAD( "tes1sprg.6d",  0x0000000, 0x040000, CRC(af18759f) SHA1(aabd7d1384925781d37f860605a5d4622e0fc2e4) )

	ROM_REGION( 0x0400000, REGION_SOUND1, 0 ) /* samples */
	ROM_LOAD( "tes1wave.8k",  0x0000000, 0x400000, CRC(34a34eab) SHA1(8e83a579abdcd419dc5cff8aa4c1d7e6c3add773) )
	ROM_END

ROM_START( xevi3dg )
	ROM_REGION( 0x0400000, REGION_CPU1, 0 ) /* main ram */
	ROM_REGION( 0x0000400, REGION_USER1, 0 ) /* scratchpad */

	ROM_REGION32_LE( 0x0400000, REGION_USER2, 0 ) /* main prg */
	ROM_LOAD16_BYTE( "xv31vera.2l",  0x0000000, 0x100000, CRC(419e0f13) SHA1(876ae81c9fde18cdec792f3dfee682761ac99730) )
	ROM_LOAD16_BYTE( "xv31vera.2j",  0x0000001, 0x100000, CRC(df95373a) SHA1(65cf85ad4dcbc8f0376ac46ee0c1037d79099843) )
	ROM_LOAD16_BYTE( "xv31vera.2k",  0x0200000, 0x100000, CRC(3d58138e) SHA1(9203d6bdc2d968de818d5f465523cc030217dcf8) )
	ROM_LOAD16_BYTE( "xv31vera.2f",  0x0200001, 0x100000, CRC(9e8780a2) SHA1(83148d55456b2b92969f7ac2bdb2d492bf969895) )

	ROM_REGION32_LE( 0x1000000, REGION_USER3, 0 ) /* main data */
	ROM_LOAD16_BYTE( "xv31rm0l.5",   0x0000000, 0x200000, CRC(24e1e262) SHA1(84df49b22a8a36284da771944a8390672a0c32bf) )
	ROM_LOAD16_BYTE( "xv31rm0u.6",   0x0000001, 0x200000, CRC(cae38ef3) SHA1(2dfe0b31969091975e8d8c8188ce7dd007e4a0f3) )
	ROM_LOAD16_BYTE( "xv31rm1l.3",   0x0400000, 0x200000, CRC(46b4cb72) SHA1(c3360c4fdb71ffcbccec3f4ad8d7963b08822e26) )
	ROM_LOAD16_BYTE( "xv31rm1u.8",   0x0400001, 0x200000, CRC(be0eb5d1) SHA1(f1a0adcb7c65bbba723fe09b07280b0b924d6b19) )
	ROM_LOAD16_BYTE( "xv31rm2l.4",   0x0800000, 0x200000, CRC(8403a277) SHA1(35193211351494a086d8422e3b0b71a8d3a262a6) )
	ROM_LOAD16_BYTE( "xv31rm2u.7",   0x0800001, 0x200000, CRC(ecf70432) SHA1(bec128a215e0aef66e9a8707bb0d1eb7b098a356) )

	ROM_REGION( 0x0040000, REGION_CPU2, 0 ) /* sound prg */
	ROM_LOAD( "xv31sprg.6d",  0x0000000, 0x040000, CRC(e50b856a) SHA1(631da4f60c9ce08387fca26a70481a2fdacf9765) )

	ROM_REGION( 0x0400000, REGION_SOUND1, 0 ) /* samples */
	ROM_LOAD( "xv31wave.8k",  0x0000000, 0x400000, CRC(14f25ddd) SHA1(4981cf1017432ff85b768ec88c36f535df30b783) )
ROM_END

GAMEX( 1994, tekken,    0,        coh100_tekken, tekken,   namcos11, ROT0, "Namco", "Tekken (TE4-VER.C)", GAME_IMPERFECT_GRAPHICS | GAME_NO_SOUND )
GAMEX( 1994, tekkena,   tekken,   coh100_tekken, tekken,   namcos11, ROT0, "Namco", "Tekken (TE2-VER.B)", GAME_IMPERFECT_GRAPHICS | GAME_NO_SOUND )
GAMEX( 1994, tekkenb,   tekken,   coh100_tekken, tekken,   namcos11, ROT0, "Namco", "Tekken (TE1-VER.B)", GAME_IMPERFECT_GRAPHICS | GAME_NO_SOUND )
GAMEX( 1995, tekken2,   0,        coh100_tekken, tekken,   namcos11, ROT0, "Namco", "Tekken 2 Ver.B (TES3-VER.B)", GAME_IMPERFECT_GRAPHICS | GAME_NO_SOUND )
GAMEX( 1995, tekken2a,  tekken2,  coh100_tekken, tekken,   namcos11, ROT0, "Namco", "Tekken 2 Ver.B (TES2-VER.B)", GAME_IMPERFECT_GRAPHICS | GAME_NO_SOUND )
GAMEX( 1995, tekken2b,  tekken2,  coh100_tekken, tekken,   namcos11, ROT0, "Namco", "Tekken 2 (TES2-VER.A)", GAME_IMPERFECT_GRAPHICS | GAME_NO_SOUND )
GAMEX( 1996, souledge,  0,        coh110_danceyes, souledge, namcos11, ROT0, "Namco", "Soul Edge Ver. II (SO4-VER.C)", GAME_IMPERFECT_GRAPHICS | GAME_NO_SOUND )
GAMEX( 1995, souledga,  souledge, coh110_danceyes, souledge, namcos11, ROT0, "Namco", "Soul Edge (SO3-VER.A)", GAME_IMPERFECT_GRAPHICS | GAME_NO_SOUND )
GAMEX( 1995, souledgb,  souledge, coh110_danceyes, souledge, namcos11, ROT0, "Namco", "Soul Edge (SO1-VER.A)", GAME_IMPERFECT_GRAPHICS | GAME_NO_SOUND )
GAMEX( 1995, dunkmnia,  0,        coh110_danceyes, namcos11, namcos11, ROT0, "Namco", "Dunk Mania (DM1-VER.C)", GAME_NO_SOUND )
GAMEX( 1995, xevi3dg,   0,        coh110_danceyes, namcos11, namcos11, ROT0, "Namco", "Xevious 3D-G (XV31-VER.A)", GAME_IMPERFECT_GRAPHICS | GAME_NO_SOUND )
GAMEX( 1996, primglex,  0,        coh110_primglex, tekken,   namcos11, ROT0, "Namco", "Prime Goal EX (PG1-VER.A)", GAME_IMPERFECT_GRAPHICS | GAME_NO_SOUND )
GAMEX( 1996, danceyes,  0,        coh110_danceyes, namcos11, namcos11, ROT0, "Namco", "Dancing Eyes (DC1-VER.A)", GAME_IMPERFECT_GRAPHICS | GAME_NO_SOUND )
GAMEX( 1997, starswep,  0,        coh110_primglex, namcos11, namcos11, ROT0, "Axela/Namco", "Star Sweep (STP1-VER.A)", GAME_IMPERFECT_GRAPHICS | GAME_NO_SOUND )
GAMEX( 1998, myangel3,  0,        coh110_primglex, myangel3, namcos11, ROT0, "Namco", "Kosodate Quiz My Angel 3 (KQT1-VER.A)", GAME_IMPERFECT_GRAPHICS | GAME_NO_SOUND )
