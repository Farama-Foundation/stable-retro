/***************************************************************************

  Namco System 12 - Arcade PSX Hardware
  =====================================
  Driver by smf

  Issues:
    sound cpu is only simulated enough for inputs to work, credit handling is not perfect.
    not all games work due to either banking, dma or protection issues.
    graphics are glitchy in some games.

Known Dumps
-----------

Game       Description                             CPU board           Mother board             Daughter board          Keycus

tekken3    Tekken 3 (TET1/VER.E)                   COH 700             ?                        SYSTEM12 M8F2           ?
soulclbr   Soul Calibur (SOC1/VER.A)               COH 700             SYSTEM12 MOTHER(B)       SYSTEM12 M5F2           KC020
ehrgeiz    Ehrgeiz (EG3/VER.A)                     COH 700             SYSTEM12 MOTHER(B)       SYSTEM12 M4F6           KC021
mdhorse    Derby Quiz My Dream Horse (MDH1/VER.A)  COH 700             SYSTEM12 MOTHER(B)       SYSTEM12 M10X64         KC035
fgtlayer   Fighting Layer (FTL1/VER.A)             COH 700             SYSTEM12 MOTHER(B)       SYSTEM12 M5F4           KC037
pacapp     Paca Paca Passion (PPP1/VER.A)          COH 700             SYSTEM12 MOTHER(B)       SYSTEM12 M8F2           KC038
sws99      Super World Stadium '99 (SS91/VER.A)    COH 700             SYSTEM12 MOTHER(B)       SYSTEM12 M5F4           KC043
tekkentt   Tekken Tag Tournament (TEG3/VER.B)      COH 700             SYSTEM12 MOTHER(B)       SYSTEM12 M8F4           KC044
mrdrillr   Mr Driller (DRI1/VER.A)                 COH 700             SYSTEM12 MOTHER(C)       SYSTEM12 M8F2           KC048
aquarush   Aqua Rush (AQ1/VER.A)                   COH 700             SYSTEM12 MOTHER(C)       SYSTEM12 M5F2           KC053
golgo13    Golgo 13 (GLG1/VER.A)                   COH 700             ?                        SYSTEM12 M8F6           ?

COH 700

SONY CXD8661R L9A0088 WE19960 NNM 9738
SONY CXD8654Q
MOTOROLA MC44200FT (Triple 8-Bit Video DAC)
RAM: SEC KM4132G271Q-10 (x2), SEC KM416V1204BT-L5 (x2, note there are 2 additional unpopulated posions for this RAM also)
X1  : V100.00 KDS 745 (near CXD8661R)
X101: M53.693 KDS 745 (near CXD8654Q)

*/

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

static data32_t *namcos12_sharedram;

static WRITE32_HANDLER( sharedram_w )
{
	verboselog( 1, "sharedram_w( %08x, %08x, %08x )\n", ( offset * 4 ), data, mem_mask );
	COMBINE_DATA( &namcos12_sharedram[ offset ] );
}

static READ32_HANDLER( sharedram_r )
{
	verboselog( 1, "sharedram_r( %08x, %08x ) %08x\n", ( offset * 4 ), mem_mask, namcos12_sharedram[ offset ] );
	return namcos12_sharedram[ offset ];
}

#define SHRAM( x ) namcos12_sharedram[ ( x ) / 4 ]

static INTERRUPT_GEN( namcos12_vblank )
{
	UINT16 n_coin;
	static UINT16 n_oldcoin = 0;

	SHRAM( 0x3000 ) = ( SHRAM( 0x3000 ) & 0x0000ffff ) | 0x76010000;
	SHRAM( 0x30f0 ) = ( SHRAM( 0x30f0 ) & 0xffff0000 ) | 0x00000000;
	SHRAM( 0x305c ) = ( SHRAM( 0x305c ) & 0x0000ffff ) | 0x00000000;
	SHRAM( 0x3068 ) = ( SHRAM( 0x3068 ) & 0x0000ffff ) | 0x00000000;
	SHRAM( 0x3078 ) = ( SHRAM( 0x3078 ) & 0xffff0000 ) | 0x00000000;
	SHRAM( 0x3240 ) = ( SHRAM( 0x3240 ) & 0xffff0000 ) | 0x00000000;
	SHRAM( 0x3940 ) = ( SHRAM( 0x3940 ) & 0xffff0000 ) | 0x00000000;

	SHRAM( 0x3380 ) = readinputport( 0 );

	SHRAM( 0x3180 ) = SHRAM( 0x3140 );
	SHRAM( 0x3140 ) = readinputport( 1 ) | ( readinputport( 2 ) << 16 );
	n_coin = readinputport( 3 );
	if( ( n_coin & n_oldcoin & 0x01 ) != 0 )
	{
		SHRAM( 0x32c0 ) = ( SHRAM( 0x32c0 ) & 0xffff0000 ) | ( ( SHRAM( 0x32c0 ) + 0x00000001 ) & 0x0000ffff );
	}
	if( ( n_coin & n_oldcoin & 0x02 ) != 0 )
	{
		SHRAM( 0x32c0 ) = ( SHRAM( 0x32c0 ) & 0x0000ffff ) | ( ( SHRAM( 0x32c0 ) + 0x00010000 ) & 0xffff0000 );
	}
	if( ( SHRAM( 0x3140 ) & ~SHRAM( 0x3180 ) & 0x00004000 ) != 0 )
	{
		SHRAM( 0x3200 ) = ( SHRAM( 0x3200 ) & 0xffff0000 ) | ( ( SHRAM( 0x3200 ) + 0x00000001 ) & 0x0000ffff );
	}
	if( ( SHRAM( 0x3140 ) & ~SHRAM( 0x3180 ) & 0x40000000 ) != 0 )
	{
		SHRAM( 0x3200 ) = ( SHRAM( 0x3200 ) & 0x0000ffff ) | ( ( SHRAM( 0x3200 ) + 0x00010000 ) & 0xffff0000 );
	}
	SHRAM( 0x3900 ) = SHRAM( 0x3200 ) + SHRAM( 0x32c0 );

	n_oldcoin = ~n_coin;

	psx_vblank();

	/* kludge: protection hacks */
	if( strcmp( Machine->gamedrv->name, "fgtlayer" ) == 0 )
	{
		data8_t *RAM = memory_region( REGION_CPU1 );
		if( *( (data32_t *)&RAM[ 0x2ac494 ] ) == 0x080ab125 )
		{
			*( (data32_t *)&RAM[ 0x2ac494 ] ) = 0;
		}
	}
	else if( strcmp( Machine->gamedrv->name, "pacapp" ) == 0 )
	{
		data8_t *RAM = memory_region( REGION_CPU1 );
		if( *( (data32_t *)&RAM[ 0x16d50 ] ) == 0x08005b54 )
		{
			*( (data32_t *)&RAM[ 0x16d50 ] ) = 0;
		}
	}
}

static data32_t m_n_bankoffset;

static WRITE32_HANDLER( bankoffset_w )
{
	m_n_bankoffset = data;

	cpu_setbank( 7, memory_region( REGION_USER3 ) + ( m_n_bankoffset * 0x200000 ) );

	verboselog( 1, "bankoffset_w( %08x, %08x, %08x ) %08x\n", offset, data, mem_mask, m_n_bankoffset );
}

static data32_t m_n_dmaoffset;

static WRITE32_HANDLER( dmaoffset_w )
{
	if( ACCESSING_LSW32 )
	{
		m_n_dmaoffset = ( offset * 4 ) + ( data << 16 );
	}
	verboselog( 1, "dmaoffset_w( %08x, %08x, %08x ) %08x\n", offset, data, mem_mask, m_n_dmaoffset );
}

static void namcos12_rom_read( UINT32 n_address, INT32 n_size )
{
	INT32 n_left;
	UINT32 *p_n_src;
	UINT32 *p_n_dst;

	verboselog( 1, "namcos12_rom_read( %08x, %08x )\n", n_address, n_size );

	if( m_n_dmaoffset >= 0x80000000 )
	{
		p_n_src = (data32_t *)( memory_region( REGION_USER2 ) + ( m_n_dmaoffset & 0x003fffff ) );
	}
	else
	{
		p_n_src = (data32_t *)( memory_region( REGION_USER3 ) + ( m_n_dmaoffset & 0x7fffffff ) );
	}

	p_n_dst = (data32_t *)( memory_region( REGION_CPU1 ) + n_address );

	n_left = ( memory_region_length( REGION_CPU1 ) - n_address ) / 4;
	if( n_size > n_left )
	{
		n_size = n_left;
	}
	while( n_size > 0 )
	{
		*( p_n_dst++ ) = *( p_n_src++ );
		n_size--;
	}
}

static MEMORY_WRITE32_START( namcos12_writemem )
	{ 0x00000000, 0x003fffff, MWA32_RAM },    /* ram */
	{ 0x1f000000, 0x1f000003, bankoffset_w }, /* banking */
	{ 0x1f008000, 0x1f008003, MWA32_NOP },    /* ?? */
	{ 0x1f010000, 0x1f010003, MWA32_NOP },    /* ?? */
	{ 0x1f018000, 0x1f018003, MWA32_NOP },    /* ?? */
	{ 0x1f080000, 0x1f083fff, sharedram_w, &namcos12_sharedram }, /* shared ram?? */
	{ 0x1f140000, 0x1f141fff, MWA32_RAM, (data32_t **)&generic_nvram, &generic_nvram_size }, /* flash */
	{ 0x1f1bff08, 0x1f1bff0f, MWA32_NOP },    /* ?? */
	{ 0x1f700000, 0x1f70ffff, dmaoffset_w },  /* dma */
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
	{ 0x1fc00000, 0x1fffffff, MWA32_ROM },    /* bios */
	{ 0x80000000, 0x803fffff, MWA32_BANK3 },  /* ram mirror */
	{ 0x9fc00000, 0x9fffffff, MWA32_ROM },    /* bios */
	{ 0xa0000000, 0xa03fffff, MWA32_BANK5 },  /* ram mirror */
	{ 0xbfc00000, 0xbfffffff, MWA32_ROM },    /* bios */
	{ 0xfffe0130, 0xfffe0133, MWA32_NOP },
MEMORY_END

static MEMORY_READ32_START( namcos12_readmem )
	{ 0x00000000, 0x003fffff, MRA32_RAM },    /* ram */
	{ 0x1f000000, 0x1f000003, MRA32_NOP },    /* banking */
	{ 0x1f080000, 0x1f083fff, sharedram_r },  /* shared ram */
	{ 0x1f140000, 0x1f141fff, MRA32_RAM },    /* flash */
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
	{ 0x1fa00000, 0x1fbfffff, MRA32_BANK7 },  /* banked roms */
	{ 0x1fc00000, 0x1fffffff, MRA32_BANK2 },  /* bios mirror */
	{ 0x80000000, 0x803fffff, MRA32_BANK3 },  /* ram mirror */
	{ 0x9fc00000, 0x9fffffff, MRA32_BANK4 },  /* bios mirror */
	{ 0xa0000000, 0xa03fffff, MRA32_BANK5 },  /* ram mirror */
	{ 0xbfc00000, 0xbfffffff, MRA32_BANK6 },  /* bios */
MEMORY_END

static DRIVER_INIT( namcos12 )
{
	cpu_setbank( 1, memory_region( REGION_USER1 ) );
	cpu_setbank( 2, memory_region( REGION_USER2 ) );
	cpu_setbank( 3, memory_region( REGION_CPU1 ) );
	cpu_setbank( 4, memory_region( REGION_USER2 ) );
	cpu_setbank( 5, memory_region( REGION_CPU1 ) );
	cpu_setbank( 6, memory_region( REGION_USER2 ) );

	psx_driver_init();

	psx_dma_install_read_handler( 5, namcos12_rom_read );

	state_save_register_UINT32( "namcos12", 0, "m_n_dmaoffset", &m_n_dmaoffset, 1 );
	state_save_register_UINT32( "namcos12", 0, "m_n_bankoffset", &m_n_bankoffset, 1 );

	/* kludge: some kind of protection? */
	if( strcmp( Machine->gamedrv->name, "tekkentt" ) == 0 ||
		strcmp( Machine->gamedrv->name, "fgtlayer" ) == 0 ||
		strcmp( Machine->gamedrv->name, "golgo13" ) == 0 ||
		strcmp( Machine->gamedrv->name, "mrdrillr" ) == 0 ||
		strcmp( Machine->gamedrv->name, "pacapp" ) == 0 )
	{
		data8_t *RAM = memory_region( REGION_USER2 );

		*( (data32_t *)&RAM[ 0x20280 ] ) = 0;
		*( (data32_t *)&RAM[ 0x20284 ] ) = 0;
		*( (data32_t *)&RAM[ 0x20288 ] ) = 0;
	}
}

MACHINE_INIT( namcos12 )
{
	psx_machine_init();
}

static MACHINE_DRIVER_START( coh700 )
	/* basic machine hardware */
	MDRV_CPU_ADD( PSXCPU, 33868800 / 2 ) /* 33MHz ?? */
	MDRV_CPU_MEMORY( namcos12_readmem, namcos12_writemem )
	MDRV_CPU_VBLANK_INT( namcos12_vblank, 1 )

	MDRV_FRAMES_PER_SECOND( 60 )
	MDRV_VBLANK_DURATION( 0 )

	MDRV_MACHINE_INIT( namcos12 )
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
	MDRV_VIDEO_START( psx_type2_1024x1024 )
	MDRV_VIDEO_UPDATE( psx )
	MDRV_VIDEO_STOP( psx )

	/* sound hardware */
	MDRV_SOUND_ATTRIBUTES( SOUND_SUPPORTS_STEREO )
MACHINE_DRIVER_END

INPUT_PORTS_START( namcos12 )
	/* IN 0 */
	PORT_START
	PORT_BITX( 0x8000, IP_ACTIVE_HIGH, 0, "Test Switch", KEYCODE_F2, IP_JOY_NONE )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_DIPNAME( 0x0040, 0x0000, "DIP1 (Test)" )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0040, DEF_STR( On ) )
	PORT_DIPNAME( 0x0020, 0x0000, "DIP2 (Freeze)" )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( On ) )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_UNKNOWN )

	/* IN 1 */
	PORT_START
	PORT_BIT( 0x8000, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x8000, DEF_STR( On ) )
	PORT_BIT( 0x4000, IP_ACTIVE_HIGH, IPT_SERVICE1 )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x4000, DEF_STR( On ) )
	PORT_BIT( 0x2000, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP | IPF_8WAY )
	PORT_BIT( 0x1000, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN | IPF_8WAY )
	PORT_BIT( 0x0800, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_8WAY )
	PORT_BIT( 0x0400, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x0200, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x0100, IP_ACTIVE_HIGH, IPT_BUTTON2 )
	PORT_BIT( 0x0080, IP_ACTIVE_HIGH, IPT_BUTTON3 )
	PORT_BIT( 0x0040, IP_ACTIVE_HIGH, IPT_BUTTON4 )
	PORT_BIT( 0x0020, IP_ACTIVE_HIGH, IPT_BUTTON5 )
	PORT_BIT( 0x0010, IP_ACTIVE_HIGH, IPT_BUTTON6 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_UNKNOWN )

	/* IN 2 */
	PORT_START
	PORT_BIT( 0x8000, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x8000, DEF_STR( On ) )
	PORT_BIT( 0x4000, IP_ACTIVE_HIGH, IPT_SERVICE2 )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x4000, DEF_STR( On ) )
	PORT_BIT( 0x2000, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x1000, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0800, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0400, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0200, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x0100, IP_ACTIVE_HIGH, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x0080, IP_ACTIVE_HIGH, IPT_BUTTON3 | IPF_PLAYER2 )
	PORT_BIT( 0x0040, IP_ACTIVE_HIGH, IPT_BUTTON4 | IPF_PLAYER2 )
	PORT_BIT( 0x0020, IP_ACTIVE_HIGH, IPT_BUTTON5 | IPF_PLAYER2 )
	PORT_BIT( 0x0010, IP_ACTIVE_HIGH, IPT_BUTTON6 | IPF_PLAYER2 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_UNKNOWN )

	/* IN 3 */
	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x0002, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT( 0xfffc, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END

ROM_START( aquarush )
	ROM_REGION( 0x0400000, REGION_CPU1, 0 ) /* main ram */
	ROM_REGION( 0x0000400, REGION_USER1, 0 ) /* scratchpad */

	ROM_REGION32_LE( 0x00400000, REGION_USER2, 0 ) /* main prg */
	ROM_LOAD16_BYTE( "aq1vera.2l",  0x0000000, 0x200000, CRC(91eb9258) SHA1(30e225eb551bfe1bed6b342dd6d597345d64b677) )
	ROM_LOAD16_BYTE( "aq1vera.2p",  0x0000001, 0x200000, CRC(a92f21aa) SHA1(bde33f1f66aaa55031c6b2972b042eef87047cce) )

	ROM_REGION32_LE( 0x1000000, REGION_USER3, 0 ) /* main data */
	ROM_LOAD(        "aq1rm0.7",    0x000000, 0x800000, CRC(3e64dd33) SHA1(dce3bb84c3069bc202d04f24d7702158dd3194d4) )
	ROM_LOAD(        "aq1rm1.8",    0x800000, 0x800000, CRC(e4d415cf) SHA1(bbd244adaf704d7daf7341ff3b0a92162927a59b) )

	ROM_REGION( 0x0080000, REGION_CPU2, 0 ) /* sound prg */
	ROM_LOAD( "aq1vera.11s",  0x000000, 0x080000, CRC(78277e02) SHA1(577ebb6d7ab5e304fb1dc1e7fd5649762e0d1786) )

	ROM_REGION( 0x0800000, REGION_SOUND1, 0 ) /* samples */
	ROM_LOAD( "aq1wav0.2",    0x000000, 0x800000, CRC(0cf7278d) SHA1(aee31e4d9b3522f42325071768803c542aa6de09) )
ROM_END

ROM_START( ehrgeiz )
	ROM_REGION( 0x0400000, REGION_CPU1, 0 ) /* main ram */
	ROM_REGION( 0x0000400, REGION_USER1, 0 ) /* scratchpad */

	ROM_REGION32_LE( 0x00400000, REGION_USER2, 0 ) /* main prg */
	ROM_LOAD16_BYTE( "eg3vera.2l",  0x0000000, 0x200000, CRC(64c00ff0) SHA1(fc7980bc8d98c810aed2eb6b3265d150784dfc15) )
	ROM_LOAD16_BYTE( "eg3vera.2p",  0x0000001, 0x200000, CRC(e722c030) SHA1(4669a7861c14d97048728989708a0fa3733f83a8) )

	ROM_REGION32_LE( 0x1c00000, REGION_USER3, 0 ) /* main data */
	ROM_LOAD16_BYTE( "eg1rm0l.12",  0x0000000, 0x800000, CRC(fe11a48e) SHA1(bdb2af5949bb4a324dab01489e9488aa5b9bd129) )
	ROM_LOAD16_BYTE( "eg1rm0u.11",  0x0000001, 0x800000, CRC(7cb90c25) SHA1(0ff6f7bf8b7eb2e0706bd235fcb9a95ea639fae6) )
	ROM_LOAD16_BYTE( "eg1fl1l.9",   0x1000000, 0x200000, CRC(dd4cac2b) SHA1(f49d0055303b3d8639e94f93bce6bf160cc99913) )
	ROM_LOAD16_BYTE( "eg1fl1u.10",  0x1000001, 0x200000, CRC(e3348407) SHA1(829bf1be0f74fd385e325d774f1449e28aba1e4d) )
	ROM_LOAD16_BYTE( "eg1fl2l.7",   0x1400000, 0x200000, CRC(34493262) SHA1(9dd5b1b25e7232460bf5ee5b339d9f536ec26979) )
	ROM_LOAD16_BYTE( "eg1fl2u.8",   0x1400001, 0x200000, CRC(4fb84f1d) SHA1(714e0ef56871d7b4568bc6dda08bbb1c01dbba37) )
	ROM_LOAD16_BYTE( "eg1fl3l.5",   0x1800000, 0x200000, CRC(f9441f87) SHA1(70944160620ba2fc80b4fc3a7b61c33622298a8b) )
	ROM_LOAD16_BYTE( "eg1fl3u.6",   0x1800001, 0x200000, CRC(cea397de) SHA1(3791dbadb5699c805c27930e59c61af4d62f77f7) )

	ROM_REGION( 0x0080000, REGION_CPU2, 0 ) /* sound prg */
	ROM_LOAD( "eg3vera.11s",  0x0000000, 0x080000, CRC(9e44645e) SHA1(374eb4a4c09d6b5b7af5ff0efec16b4d2aacbe2b) )

	ROM_REGION( 0x0800000, REGION_SOUND1, 0 ) /* samples */
	ROM_LOAD( "eg1wav0.2",    0x0000000, 0x800000, CRC(961fe69f) SHA1(0189a061959a8d94b9d2db627911264faf9f28fd) )
ROM_END

ROM_START( fgtlayer )
	ROM_REGION( 0x0400000, REGION_CPU1, 0 ) /* main ram */
	ROM_REGION( 0x0000400, REGION_USER1, 0 ) /* scratchpad */

	ROM_REGION32_LE( 0x00400000, REGION_USER2, 0 ) /* main prg */
	ROM_LOAD16_BYTE( "ftl1vera.2e", 0x0000000, 0x200000, CRC(f4156e79) SHA1(cedb917940be8c74fa4ddb48213ce6917444e306) )
	ROM_LOAD16_BYTE( "ftl1vera.2j", 0x0000001, 0x200000, CRC(c65b57c0) SHA1(0051aa46d09fbe9d896ae5f534e21955373f1d46) )

	ROM_REGION32_LE( 0x2000000, REGION_USER3, 0 ) /* main data */
	ROM_LOAD(        "ftl1rm0.9",   0x0000000, 0x800000, CRC(e33ce365) SHA1(f7e5b98d2e8e5f233265909477c84331f016ebfb) )
	ROM_LOAD(        "ftl1rm1.10",  0x0800000, 0x800000, CRC(a1ec7d08) SHA1(e693a0b16235c44d4165b2f53dc25d5288297c27) )
	ROM_LOAD(        "ftl1rm2.11",  0x1000000, 0x800000, CRC(204be858) SHA1(95b22b678b7d11b0ec907698c18cebd84437c656) )
	ROM_LOAD16_BYTE( "ftl1fl3l.7",  0x1800000, 0x200000, CRC(e4fb01e1) SHA1(c7883e86afb58412281a317bfdf62a21488421be) )
	ROM_LOAD16_BYTE( "ftl1fl3h.8",  0x1800001, 0x200000, CRC(67a5c56f) SHA1(6f0a4f93b4975b24efb26c3dd47b791c92c55fbf) )
	ROM_LOAD16_BYTE( "ftl1fl4l.5",  0x1c00000, 0x200000, CRC(8039242c) SHA1(9c3f9637d7cd0c004c0c39aee815eed228ebad20) )
	ROM_LOAD16_BYTE( "ftl1fl4h.6",  0x1c00001, 0x200000, CRC(5ad59726) SHA1(b3a68f7ba2052b99407a5423223202001f2a4f67) )

	ROM_REGION( 0x0080000, REGION_CPU2, 0 ) /* sound prg */
	ROM_LOAD( "ftl1vera.11s", 0x0000000, 0x080000, CRC(e3f957cd) SHA1(1c7f2033025fb9c40654cff26d78697baf697c59) )

	ROM_REGION( 0x1000000, REGION_SOUND1, 0 ) /* samples */
	ROM_LOAD( "ftl1wav0.2",   0x0000000, 0x800000, CRC(ee009a2b) SHA1(c332bf59917b2673d7acb864bf92d25d74a350b6) )
	ROM_LOAD( "ftl1wav1.1",   0x0800000, 0x800000, CRC(a54a89cd) SHA1(543b47c6442f7a78e26992b041235a91d719bb89) )
ROM_END

ROM_START( golgo13 )
	ROM_REGION( 0x0400000, REGION_CPU1, 0 ) /* main ram */
	ROM_REGION( 0x0000400, REGION_USER1, 0 ) /* scratchpad */

	ROM_REGION32_LE( 0x00400000, REGION_USER2, 0 ) /* main prg */
	ROM_LOAD16_BYTE( "glg1vera.2l", 0x0000000, 0x200000, CRC(aa15abfe) SHA1(e82b408746e01c50c5cb0dcef804974d1e97078a) )
	ROM_LOAD16_BYTE( "glg1vera.2p", 0x0000001, 0x200000, CRC(37a4cf90) SHA1(b5470d44036e9de8220b669f71b50bcec42d9a18) )

	ROM_REGION32_LE( 0x3c00000, REGION_USER3, 0 ) /* main data */
	ROM_LOAD16_BYTE( "glg1rm0l.10", 0x0000000, 0x800000, CRC(2837524e) SHA1(81d811c2cf8121854ed6046dc421cda7fcd44014) )
	ROM_LOAD16_BYTE( "glg1rm0u.14", 0x0000001, 0x800000, CRC(4482deff) SHA1(03dcd12633b167d9af664ae826fc2c2ff096c0a6) )
	ROM_LOAD16_BYTE( "glg1rm1l.11", 0x1000000, 0x800000, CRC(080f3550) SHA1(40a5257baeb366e32ff51f571efb12a556f93430) )
	ROM_LOAD16_BYTE( "glg1rm1u.15", 0x1000001, 0x800000, CRC(b49e8613) SHA1(d25ede8046ce309d5a5515a2e44b6773fdd56333) )
	ROM_LOAD16_BYTE( "glg1rm2l.12", 0x2000000, 0x800000, CRC(e1a79a87) SHA1(c3f31b1bc6a3b51361df0d89d3ff0717875fd800) )
	ROM_LOAD16_BYTE( "glg1rm2u.16", 0x2000001, 0x800000, CRC(cda22852) SHA1(e609a55e0ec91b532469c3607dcc96456eec6a07) )
	ROM_LOAD16_BYTE( "glg1fl3l.4",  0x3000000, 0x200000, CRC(16ddc2cb) SHA1(71460bd4ad35b3488dc1326fe2f600df40988e06) )
	ROM_LOAD16_BYTE( "glg1fl3u.5",  0x3000001, 0x200000, CRC(d90d72d1) SHA1(0027389344862486c53cd0e1554b36eed65aa5b0) )
	ROM_LOAD16_BYTE( "glg1fl4l.6",  0x3400000, 0x200000, CRC(820d8133) SHA1(2d780612055a0fa5de756cfa46ce7f134139e550) )
	ROM_LOAD16_BYTE( "glg1fl4u.7",  0x3400001, 0x200000, CRC(02d78160) SHA1(2eafcbd3ba550acf7816074e6c7eee65f7d64859) )
	ROM_LOAD16_BYTE( "glg1fl5l.8",  0x3800000, 0x200000, CRC(090b2508) SHA1(c3587aeab71f6a9dcc90bf2af496303e9d20eac8) )
	ROM_LOAD16_BYTE( "glg1fl5u.9",  0x3800001, 0x200000, CRC(2231a07c) SHA1(480e17219101a0dae5cd64507e31cd7e711c95fa) )

	ROM_REGION( 0x0080000, REGION_CPU2, 0 ) /* sound prg */
	ROM_LOAD( "glg1vera.11s", 0x0000000, 0x080000, CRC(5c33f240) SHA1(ec8fc8d83466b28dfa35b93e16d8164883513b19) )

	ROM_REGION( 0x1000000, REGION_SOUND1, 0 ) /* samples */
	ROM_LOAD( "glg1wav0.1",   0x0000000, 0x800000, CRC(672d4f7c) SHA1(16a7564a2c68840a438a33ac2381df4e70e1bb45) )
	ROM_LOAD( "glg1wav1.13",  0x0800000, 0x800000, CRC(480b0a1a) SHA1(341d5ec8ad0f3c0a121eeeec9466aaeec2bd1c74) )
ROM_END

ROM_START( mdhorse )
	ROM_REGION( 0x0400000, REGION_CPU1, 0 ) /* main ram */
	ROM_REGION( 0x0000400, REGION_USER1, 0 ) /* scratchpad */

	ROM_REGION32_LE( 0x00400000, REGION_USER2, 0 ) /* main prg */
	ROM_LOAD16_BYTE( "mdh1vera.2l", 0x0000000, 0x200000, CRC(fbb567b2) SHA1(899dccdfbc8dcbcdaf9b5df93e249a36f8cbf999) )
	ROM_LOAD16_BYTE( "mdh1vera.2p", 0x0000001, 0x200000, CRC(a0f182ab) SHA1(70c789ea88248c1f810f9fdb3feaf808acbaa8cd) )

	ROM_REGION32_LE( 0x3000000, REGION_USER3, 0 ) /* main data */
	ROM_LOAD16_BYTE( "mdh1rm0l",    0x0000000, 0x800000, CRC(ca5bf806) SHA1(8c88b1f29c96a0696ac9428c411cde10faacce35) )
	ROM_LOAD16_BYTE( "mdh1rm0u",    0x0000001, 0x800000, CRC(315e9539) SHA1(340fcd196f53f64b3f56ef73101eddc9e142d907) )
	ROM_LOAD16_BYTE( "mdh1rm1l",    0x1000000, 0x800000, CRC(9f610211) SHA1(8459733c52d1c62033a4aeb9985b4a6e863a62d0) )
	ROM_LOAD16_BYTE( "mdh1rm1u",    0x1000001, 0x800000, CRC(a2e43560) SHA1(b0657a22701c6f2098f210d59e4c9bc88593a750) )
	ROM_LOAD16_BYTE( "mdh1rm2l",    0x2000000, 0x800000, CRC(84840fa9) SHA1(0f051db55b30f7dd473f4df9bb5e7d38c39dc785) )
	ROM_LOAD16_BYTE( "mdh1rm2u",    0x2000001, 0x800000, CRC(9490dafe) SHA1(c1bd9343535876eac5369a86105013b4a7d731b3) )

	ROM_REGION( 0x0080000, REGION_CPU2, 0 ) /* sound prg */
	ROM_LOAD( "mdh1vera.11s", 0x0000000, 0x080000, CRC(20d7ba29) SHA1(95a056d1f1ac70dda8ced832b506076485348a33) )

	ROM_REGION( 0x0800000, REGION_SOUND1, 0 ) /* samples */
	ROM_LOAD( "mdh1wav0",     0x0000000, 0x800000, CRC(7b031123) SHA1(7cbc1f71d259405f9f1ef26026d51abcb255b057) )
ROM_END

ROM_START( mrdrillr )
	ROM_REGION( 0x0400000, REGION_CPU1, 0 ) /* main ram */
	ROM_REGION( 0x0000400, REGION_USER1, 0 ) /* scratchpad */

	ROM_REGION32_LE( 0x00400000, REGION_USER2, 0 ) /* main prg */
	ROM_LOAD16_BYTE( "dri1vera.2l", 0x0000000, 0x200000, CRC(751ca21d) SHA1(1c271bba83d387c797ce8daa43885bcb6e1a51a6) )
	ROM_LOAD16_BYTE( "dri1vera.2p", 0x0000001, 0x200000, CRC(2a2b0704) SHA1(5a8b40c6cf0adc43ca2ee0c576ec82f314aacd2c) )

	ROM_REGION32_LE( 0x0800000, REGION_USER3, 0 ) /* main data */
	ROM_LOAD16_BYTE( "dri1rm0l.6",  0x0000000, 0x400000, CRC(021bb2fa) SHA1(bfe3e46e9728d5b5a692f432515267ff8b8255e7) )
	ROM_LOAD16_BYTE( "dri1rm0u.9",  0x0000001, 0x400000, CRC(5aae85ea) SHA1(a54dcc050c12ed3d77efc328e366e99c392eb139) )

	ROM_REGION( 0x0080000, REGION_CPU2, 0 ) /* sound prg */
	ROM_LOAD( "dri1vera.11s", 0x0000000, 0x080000, CRC(33ea9c0e) SHA1(5018d7a1a45ec3133cd928435db8804f66321924) )

	ROM_REGION( 0x0800000, REGION_SOUND1, 0 ) /* samples */
	ROM_LOAD( "dri1wav0.5",   0x0000000, 0x800000, CRC(32928df1) SHA1(79af92a2d24a0e3d5bfe1785776b0f86a93882ce) )
ROM_END

ROM_START( pacapp )
	ROM_REGION( 0x0400000, REGION_CPU1, 0 ) /* main ram */
	ROM_REGION( 0x0000400, REGION_USER1, 0 ) /* scratchpad */

	ROM_REGION32_LE( 0x00400000, REGION_USER2, 0 ) /* main prg */
	ROM_LOAD16_BYTE( "ppp1vera.2l", 0x0000000, 0x200000, CRC(6e74bd05) SHA1(41a2e06538cea3bced2992f5858a3f0cd1c0b4aa) )
	ROM_LOAD16_BYTE( "ppp1vera.2p", 0x0000001, 0x200000, CRC(b7a2f724) SHA1(820ae04ec416b8394a1d919279748bde3460cb96) )

	ROM_REGION32_LE( 0x1800000, REGION_USER3, 0 ) /* main data */
	ROM_LOAD16_BYTE( "ppp1rm0l.6",  0x0000000, 0x400000, CRC(b152fdd8) SHA1(23c5c07680a62e941a7b1a28897f986dd9399801) )
	ROM_LOAD16_BYTE( "ppp1rm0u.9",  0x0000001, 0x400000, CRC(c615c26e) SHA1(db1aae37ebee2a74636415e4b1b0b17790a6a67e) )
	ROM_LOAD16_BYTE( "ppp1rm1l.7",  0x0800000, 0x400000, CRC(46eaedbd) SHA1(afe3c966fcf083d89b45d44d871bed1b8caa3014) )
	ROM_LOAD16_BYTE( "ppp1rm1u.10", 0x0800001, 0x400000, CRC(32f27dce) SHA1(06de870b83c972403d96b8b9a8ee5a192a99451d) )
	ROM_LOAD16_BYTE( "ppp1rm2l.8",  0x1000000, 0x400000, CRC(dca7e5ed) SHA1(b43b6e086912009295ceae11d2d540733353d7b6) )
	ROM_LOAD16_BYTE( "ppp1rm2u.11", 0x1000001, 0x400000, CRC(cada7a0d) SHA1(e8d927a4680911b77de1d906b5e0140697e9c67b) )

	ROM_REGION( 0x0080000, REGION_CPU2, 0 ) /* sound prg */
	ROM_LOAD( "ppp1vera.11s", 0x0000000, 0x080000, CRC(22242317) SHA1(e46bf3c594136168faedebbd59f53ff9a6ecf3c1) )

	ROM_REGION( 0x1000000, REGION_SOUND1, 0 ) /* samples */
	ROM_LOAD( "ppp1wav0.5",   0x0000000, 0x800000, CRC(184ccc7d) SHA1(b74638cebef209638c625aac7e8c5b924f56a8bb) )
	ROM_LOAD( "ppp1wav1.4",   0x0800000, 0x800000, CRC(cbcf74c5) SHA1(a089277c9befc87b5bbe0d4e5b8187a4ad5ef143) )
ROM_END

ROM_START( soulclbr )
	ROM_REGION( 0x0400000, REGION_CPU1, 0 ) /* main ram */
	ROM_REGION( 0x0000400, REGION_USER1, 0 ) /* scratchpad */

	ROM_REGION32_LE( 0x00400000, REGION_USER2, 0 ) /* main prg */
	ROM_LOAD16_BYTE( "soc1vera.2l", 0x0000000, 0x200000, CRC(37e0a203) SHA1(3915b5e530c8e70a07aa8ccedeb66633ae5f670e) )
	ROM_LOAD16_BYTE( "soc1vera.2p", 0x0000001, 0x200000, CRC(7cd87a35) SHA1(5a4837b6f6a49c88126a0ddbb8059a4da77127bc) )

	ROM_REGION32_LE( 0x2000000, REGION_USER3, 0 ) /* main data */
	ROM_LOAD(        "soc1rm0.7",   0x0000000, 0x800000, CRC(cdc47b55) SHA1(315ea6b819de5c4883aa400f1b9f4172637757bf) )
	ROM_LOAD(        "soc1rm1.8",   0x0800000, 0x800000, CRC(30d2dd5a) SHA1(1c0c467ba339e0241efb8d5c3b025a046b2ca676) )
	ROM_LOAD(        "soc1rm2.9",   0x1000000, 0x800000, CRC(dbb93955) SHA1(624cd8ad94e8ae53206f798bff81784afe95e5f1) )
	ROM_LOAD(        "soc1fl3.6",   0x1800000, 0x400000, CRC(24d94c38) SHA1(0f9b9ab11dd4e02086d7b9104ce2f5d4e93cd696) )
	ROM_LOAD(        "soc1fl4.5",   0x1c00000, 0x400000, CRC(6212090e) SHA1(ed5e50771180935a0c2e760e7369673098722201) )

	ROM_REGION( 0x0080000, REGION_CPU2, 0 ) /* sound prg */
	ROM_LOAD( "soc1vera.11s", 0x0000000, 0x080000, CRC(52aa206a) SHA1(5abe9d6f800fa1b9623aa08b16e9b959b840e50b) )

	ROM_REGION( 0x0800000, REGION_SOUND1, 0 ) /* samples */
	ROM_LOAD( "soc1wav0.2",   0x0000000, 0x800000, CRC(c100618d) SHA1(b87f88ee42ad9c5affa674e5f816d902143fed99) )
ROM_END

ROM_START( sws99 )
	ROM_REGION( 0x0400000, REGION_CPU1, 0 ) /* main ram */
	ROM_REGION( 0x0000400, REGION_USER1, 0 ) /* scratchpad */

	ROM_REGION32_LE( 0x00400000, REGION_USER2, 0 ) /* main prg */
	ROM_LOAD16_BYTE( "ss91vera.2e", 0x0000000, 0x200000, CRC(4dd928d7) SHA1(d76c0f52d1a2cd101a6879e6ff57ed1c52b5e228) )
	ROM_LOAD16_BYTE( "ss91vera.2j", 0x0000001, 0x200000, CRC(40777a48) SHA1(6e3052ddbe3943eb2418cd50102cead88b850240) )

	ROM_REGION32_LE( 0x2000000, REGION_USER3, 0 ) /* main data */
	ROM_LOAD(        "ss91rm0.9",   0x0000000, 0x800000, CRC(db5bc50d) SHA1(b8b59f6db3a374277871c39b3657cb193525e558) )
	ROM_LOAD(        "ss91rm1.10",  0x0800000, 0x800000, CRC(4d71f29f) SHA1(3ccc8410d383bfd9fde44574ebb9c24a235cc734) )
	ROM_FILL(                       0x1000000, 0x800000, 0 )
	ROM_LOAD16_BYTE( "ss91fl3l.7",  0x1800000, 0x200000, CRC(61efd65b) SHA1(1e97d5cd51bf778995c3e568ac7e1d3514264d48) )
	ROM_LOAD16_BYTE( "ss91fl3h.8",  0x1800001, 0x200000, CRC(7f3c8c54) SHA1(b473a503cc5d532922780139ab3cab974d3df65b) )
	ROM_LOAD16_BYTE( "ss91fl4l.5",  0x1c00000, 0x200000, CRC(a6af9511) SHA1(63f14f4fc2b210348e41cdfa552a7a7c86fb1b99) )
	ROM_LOAD16_BYTE( "ss91fl4h.6",  0x1c00001, 0x200000, CRC(be3730a4) SHA1(48802229d31c1eef7f4173eb05060a328b702336) )

	ROM_REGION( 0x0080000, REGION_CPU2, 0 ) /* sound prg */
	ROM_LOAD( "ss91vera.11s", 0x0000000, 0x080000, CRC(c6bc5c31) SHA1(c6ef46c3fa8a7749618126d360e614ea6c8d9c54) )

	ROM_REGION( 0x1000000, REGION_SOUND1, 0 ) /* samples */
	ROM_LOAD( "ss91wav0.2",   0x0000000, 0x800000, CRC(1c5e2ff1) SHA1(4f29dfd49f6b5c3ca3b758823d368051354bd673) )
	ROM_LOAD( "ss91wav1.1",   0x0800000, 0x800000, CRC(5f4c8861) SHA1(baee7182c32bc064d805de5a16948faf78941ac4) )
ROM_END

ROM_START( tekken3 )
	ROM_REGION( 0x0400000, REGION_CPU1, 0 ) /* main ram */
	ROM_REGION( 0x0000400, REGION_USER1, 0 ) /* scratchpad */

	ROM_REGION32_LE( 0x00400000, REGION_USER2, 0 ) /* main prg */
	ROM_LOAD16_BYTE( "tet1vere.2e", 0x0000000, 0x200000, CRC(8b01113b) SHA1(45fdfd58293641ed16bc59c633a85a9cf64ccbaf) )
	ROM_LOAD16_BYTE( "tet1vere.2j", 0x0000001, 0x200000, CRC(df4c96fb) SHA1(2e223045bf5b80ccf615106e869760c5b7aa8d44) )

	ROM_REGION32_LE( 0x1c00000, REGION_USER3, 0 ) /* main data */
	ROM_LOAD16_BYTE( "tet1rm0l.6",  0x0000000, 0x400000, CRC(2886bb32) SHA1(08ad9da2df25ad8c933a812ac238c81135072929) )
	ROM_LOAD16_BYTE( "tet1rm0u.9",  0x0000001, 0x400000, CRC(c5705b92) SHA1(20df20c8d18eb4712d565a3df9a8d9270dee6aaa) )
	ROM_LOAD16_BYTE( "tet1rm1l.7",  0x0800000, 0x400000, CRC(0397d283) SHA1(ebafcd14cdb2601214129a84fc6830846f5cd274) )
	ROM_LOAD16_BYTE( "tet1rm1u.10", 0x0800001, 0x400000, CRC(502ba5cd) SHA1(19c1282245c6dbfc945c0bd0f3918968c3e5c3ed) )
	ROM_LOAD16_BYTE( "tet1rm2l.8",  0x1000000, 0x400000, CRC(e03b1c24) SHA1(8579b95a8fd06b7d2893ff88b228fd794162dff1) )
	ROM_LOAD16_BYTE( "tet1rm2u.11", 0x1000001, 0x400000, CRC(75eb2ab3) SHA1(dee43884e542391903f6aaae2c166e7921a86fb4) )
	ROM_LOAD16_BYTE( "tet1fl3l.12", 0x1800000, 0x200000, CRC(45513073) SHA1(8a36f58ee2d292b50e00c6bf275f9def358032d8) )
	ROM_LOAD16_BYTE( "tet1fl3u.13", 0x1800001, 0x200000, CRC(1917d993) SHA1(cabc44514a3e62a18a7f8f883603241447d6948b) )

	ROM_REGION( 0x0080000, REGION_CPU2, 0 ) /* sound prg */
	ROM_LOAD( "tet1vere.11s", 0x0000000, 0x080000, CRC(c92b98d1) SHA1(8ae6fba8c5b6b9a2ab9541eac8553b282f35750d) )

	ROM_REGION( 0x0800000, REGION_SOUND1, 0 ) /* samples */
	ROM_LOAD( "tet1wav0.5",   0x0000000, 0x400000, CRC(77ba7975) SHA1(fe9434dcf0fb232c85efaaae1b4b13d36099620a) )
	ROM_LOAD( "tet1wav1.4",   0x0400000, 0x400000, CRC(ffeba79f) SHA1(941412bbe9d0305d9a23c224c1bb774c4321f6df) )
ROM_END

ROM_START( tekkentt )
	ROM_REGION( 0x0400000, REGION_CPU1, 0 ) /* main ram */
	ROM_REGION( 0x0000400, REGION_USER1, 0 ) /* scratchpad */

	ROM_REGION32_LE( 0x00400000, REGION_USER2, 0 ) /* main prg */
	ROM_LOAD16_BYTE( "teg3verb.2l", 0x0000000, 0x200000, CRC(1efb7b85) SHA1(0623bb6571caf046ff7b4f83f11ee84a92c4b462) )
	ROM_LOAD16_BYTE( "teg3verb.2p", 0x0000001, 0x200000, CRC(7caef9b2) SHA1(5c56d69ba2f723d0a4fbe4902196efc6ba9d5094) )

	ROM_REGION32_LE( 0x3800000, REGION_USER3, 0 ) /* main data */
	ROM_LOAD32_WORD( "teg3rm0e.9",  0x0000000, 0x800000, CRC(c962a373) SHA1(d662dbd89ef62c5ac3150a018fc2d35ef2ee94ac) )
	ROM_LOAD32_WORD( "teg3rm0o.13", 0x0000002, 0x800000, CRC(badb7dcf) SHA1(8c0bf7f6351c5a2a0996df371a901cf90c68cd8c) )
	ROM_LOAD32_WORD( "teg3rm1e.10", 0x1000000, 0x800000, CRC(b3d56124) SHA1(4df20c74ba63f7362caf15e9b8949fab655704fb) )
	ROM_LOAD32_WORD( "teg3rm1o.14", 0x1000002, 0x800000, CRC(2434ceb6) SHA1(f19f1599acbd6fd48793a2ee5a500ca817d9df56) )
	ROM_LOAD32_WORD( "teg3rm2e.11", 0x2000000, 0x800000, CRC(6e5c3428) SHA1(e3cdb60a4445406877b2e273385f34bfb0974220) )
	ROM_LOAD32_WORD( "teg3rm2o.15", 0x2000002, 0x800000, CRC(21ce9dfa) SHA1(f27e8210ee236c327aa3e1ce4dd408abc6580a1b) )
	ROM_LOAD16_BYTE( "teg3flel.4",  0x3000000, 0x200000, CRC(88b3823c) SHA1(6f31acb642c57daccbfdb87b790037e261c8c73c) )
	ROM_LOAD16_BYTE( "teg3fleu.5",  0x3000001, 0x200000, CRC(36df0867) SHA1(6bec8560ad4c122dc909daa83aa9089ba5b281f7) )
	ROM_LOAD16_BYTE( "teg3flol.6",  0x3400000, 0x200000, CRC(03a76765) SHA1(ae35ae28375f2a3e52d72b77ec09750c326cc269) )
	ROM_LOAD16_BYTE( "teg3flou.7",  0x3400001, 0x200000, CRC(6d6947d1) SHA1(2f307bc4070fadb510c0473bc91d917b2d845ca5) )

	ROM_REGION( 0x0080000, REGION_CPU2, 0 ) /* sound prg */
	ROM_LOAD( "teg3verb.11s", 0x0000000, 0x080000, CRC(67d0c469) SHA1(da164702fc21b9f46a9e32c89e7b1d36070ddf79) )

	ROM_REGION( 0x1000000, REGION_SOUND1, 0 ) /* samples */
	ROM_LOAD( "teg3wav0.1",   0x0000000, 0x800000, CRC(4bd99104) SHA1(f76b0576cc28fe49d3c1c402988b933933e52e15) )
	ROM_LOAD( "teg3wav1.12",  0x0800000, 0x800000, CRC(dbc74fff) SHA1(601b7e7361ea744b34e3fa1fc39d88641de7f4c6) )
ROM_END

GAMEX( 1996, tekken3,   0,        coh700, namcos12, namcos12, ROT0, "Namco",         "Tekken 3 (TET1-VER.E)", GAME_IMPERFECT_GRAPHICS | GAME_NO_SOUND )
GAMEX( 1998, soulclbr,  0,        coh700, namcos12, namcos12, ROT0, "Namco",         "Soul Calibur (SOC1-VER.A)", GAME_NOT_WORKING | GAME_NO_SOUND )
GAMEX( 1998, ehrgeiz,   0,        coh700, namcos12, namcos12, ROT0, "Square/Namco",  "Ehrgeiz (EG3-VER.A)", GAME_IMPERFECT_GRAPHICS | GAME_NO_SOUND )
GAMEX( 1998, mdhorse,   0,        coh700, namcos12, namcos12, ROT0, "Namco",         "Derby Quiz My Dream Horse (MDH1-VER.A)", GAME_NOT_WORKING | GAME_NO_SOUND )
GAMEX( 1998, fgtlayer,  0,        coh700, namcos12, namcos12, ROT0, "Arika/Namco",   "Fighting Layer (FTL1-VER.A)", GAME_IMPERFECT_GRAPHICS | GAME_NO_SOUND )
GAMEX( 1999, pacapp,    0,        coh700, namcos12, namcos12, ROT0, "Produce/Namco", "Paca Paca Passion (PPP1-VERA)", GAME_NOT_WORKING | GAME_NO_SOUND )
GAMEX( 1999, sws99,     0,        coh700, namcos12, namcos12, ROT0, "Namco",         "Super World Stadium '99 (SS91-VER.A)", GAME_NOT_WORKING | GAME_NO_SOUND )
GAMEX( 1999, tekkentt,  0,        coh700, namcos12, namcos12, ROT0, "Namco",         "Tekken Tag Tournament (TEG3-VER.B)", GAME_NOT_WORKING | GAME_NO_SOUND )
GAMEX( 1999, mrdrillr,  0,        coh700, namcos12, namcos12, ROT0, "Namco",         "Mr Driller (DRI1-VER.A)", GAME_IMPERFECT_GRAPHICS | GAME_NO_SOUND )
GAMEX( 1999, aquarush,  0,        coh700, namcos12, namcos12, ROT0, "Namco",         "Aqua Rush (AQ1-VER.A)", GAME_IMPERFECT_GRAPHICS | GAME_NO_SOUND )
GAMEX( 1999, golgo13,   0,        coh700, namcos12, namcos12, ROT0, "Raizing/Namco", "Golgo 13 (GLG1-VER.A)", GAME_NOT_WORKING | GAME_NO_SOUND )
