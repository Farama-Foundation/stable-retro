/***************************************************************************

  Konami GQ System - Arcade PSX Hardware
  ======================================
  Driver by R. Belmont & smf

  Crypt Killer
  Konami, 1995
  
  PCB Layout
  ----------
  
  GQ420  PWB354905B
  |----------------------------------------------------------|
  |CN14     420A01.2G  420A02.3M           CN6  CN7   CN8    |
  |056602    6264                                            |
  |LA4705    6264         058141  424800                     |
  |          68000                                           |
  |  18.432MHz      PAL   056800  424800                     |
  |  32MHz    48MHz PAL           424800                     |
  |  RESET_SW             TMS57002                           |
  |                               M514256                    |
  |                               M514256                    |
  |J  000180                                                 |
  |A                              MACH110                    |
  |M         53.693175MHz         (000619A)                  |
  |M         KM4216V256                                      |
  |A         KM4216V256     CXD8538Q        PAL (000613)     |
  |          KM4216V256                     PAL (000612)     |
  | TEST_SW  KM4216V256                     PAL (000618)     |
  |   CXD2923AR   67.7376MHz                                 |
  | DIPSW(8)                   93C46       NCR 53CF96-2      |
  | KM48V514  KM48V514         420B03.27P    HD_LED          |
  | KM48V514  KM48V514                                       |
  | CN3                CXD8530BQ      TOSHIBA MK1924FBV      |
  | KM48V514  KM48V514                (U420UAA04)            |
  | KM48V514  KM48V514                                       |
  |----------------------------------------------------------|
  
  Notes:
        CN6, CN7, CN8: For connection of guns.
        CN3 : For connection of extra controls/buttons.
        CN14: For connection of additional speaker for stereo output.
        68000 clock: 8.000MHz
        Vsync: 60Hz
*/

#include "driver.h"
#include "state.h"
#include "cpu/mips/psx.h"
#include "includes/psx.h"
#include "machine/konamigx.h"
#include "machine/eeprom.h"
#include "machine/am53cf96.h"
#include "harddisk.h"

/* Sound */

static data8_t sndto000[ 16 ];
static data8_t sndtor3k[ 16 ];
static data8_t sector_buffer[512];
static UINT8 *m_p_n_ram;

static INLINE void psxwritebyte( UINT32 n_address, UINT8 n_data )
{
	m_p_n_ram[ BYTE_XOR_LE( n_address ) ] = n_data;
}

static WRITE32_HANDLER( soundr3k_w )
{
	if( ACCESSING_MSW32 )
	{
		sndto000[ ( offset << 1 ) + 1 ] = data >> 16;
		if( offset == 3 )
		{
			cpu_set_irq_line( 1, 1, HOLD_LINE );
		}
	}
	if( ACCESSING_LSW32 )
	{
		sndto000[ offset << 1 ] = data;
	}
}

static READ32_HANDLER( soundr3k_r )
{
	data32_t data;

	data = ( sndtor3k[ ( offset << 1 ) + 1 ] << 16 ) | sndtor3k[ offset << 1 ];

	/* hack to help the main program start up */
	if( offset == 1 )
	{
		data = 0;
	}

	return data;
}

/* UART */

static WRITE32_HANDLER( mb89371_w )
{
}

static READ32_HANDLER( mb89371_r )
{
	return 0xffffffff;
}

/* Inputs */

#define GUNX( a ) ( ( readinputport( a ) * 319 ) / 0xff )
#define GUNY( a ) ( ( readinputport( a ) * 239 ) / 0xff )

static READ32_HANDLER( gun1_x_r )
{
	return GUNX( 5 ) + 125;
}

static READ32_HANDLER( gun1_y_r )
{
	return GUNY( 6 );
}

static READ32_HANDLER( gun2_x_r )
{
	return GUNX( 7 ) + 125;
}

static READ32_HANDLER( gun2_y_r )
{
	return GUNY( 8 );
}

static READ32_HANDLER( gun3_x_r )
{
	return GUNX( 9 ) + 125;
}

static READ32_HANDLER( gun3_y_r )
{
	return GUNY( 10 );
}

static READ32_HANDLER( read_inputs_0 )
{
	return ( readinputport( 2 ) << 16 ) | readinputport( 1 );
}

static READ32_HANDLER( read_inputs_1 )
{
	return ( readinputport( 0 ) << 16 ) | readinputport( 3 );
}

/* EEPROM */

static NVRAM_HANDLER( konamigq_93C46 )
{
	if( read_or_write )
	{
		EEPROM_save( file );
	}
	else
	{
		EEPROM_init( &eeprom_interface_93C46 );
		if( file )
		{
			EEPROM_load( file );
		}
		else
		{
			static data8_t def_eeprom[ 128 ] =
			{
				0x29, 0x2b, 0x52, 0x56, 0x20, 0x94, 0x41, 0x55, 0x00, 0x41, 0x14, 0x14, 0x00, 0x03, 0x01, 0x01,
				0x01, 0x03, 0x00, 0x00, 0x07, 0x07, 0x00, 0x01, 0xaa, 0x00, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa,
				0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa,
				0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa,
				0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa,
				0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa,
				0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa,
				0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa,
			};

			EEPROM_set_data( def_eeprom, 128 );
		}
	}
}

static WRITE32_HANDLER( eeprom_w )
{
	EEPROM_write_bit( ( data & 0x01 ) ? 1 : 0 );
	EEPROM_set_clock_line( ( data & 0x04 ) ? ASSERT_LINE : CLEAR_LINE );
	EEPROM_set_cs_line( ( data & 0x02 ) ? CLEAR_LINE : ASSERT_LINE );
	cpu_set_reset_line( 1, ( data & 0x40 ) ? CLEAR_LINE : ASSERT_LINE );
}

static READ32_HANDLER( eeprom_r )
{
	return ( EEPROM_read_bit() << 16 ) | readinputport( 4 );
}

/* PCM RAM */

static data8_t *m_p_n_pcmram;

static WRITE32_HANDLER( pcmram_w )
{
	if( ACCESSING_LSB32 )
	{
		m_p_n_pcmram[ offset << 1 ] = data;
	}
	if( ( mem_mask & 0x00ff0000 ) == 0 )
	{
		m_p_n_pcmram[ ( offset << 1 ) + 1 ] = data >> 16;
	}
}

static READ32_HANDLER( pcmram_r )
{
	return ( m_p_n_pcmram[ ( offset << 1 ) + 1 ] << 16 ) | m_p_n_pcmram[ offset << 1 ];
}

/* Video */

static VIDEO_UPDATE( konamigq )
{
	video_update_psx( bitmap, cliprect );

	draw_crosshair( 1, bitmap, GUNX( 5 ), GUNY( 6 ), cliprect );
	draw_crosshair( 2, bitmap, GUNX( 7 ), GUNY( 8 ), cliprect );
	draw_crosshair( 3, bitmap, GUNX( 9 ), GUNY( 10 ), cliprect );
}

static MEMORY_WRITE32_START( konamigq_writemem )
	{ 0x00000000, 0x003fffff, MWA32_RAM },    /* ram */
	{ 0x1f000000, 0x1f00001f, am53cf96_w },
	{ 0x1f100000, 0x1f10000f, soundr3k_w },
	{ 0x1f180000, 0x1f180003, eeprom_w },
	{ 0x1f198000, 0x1f198003, MWA32_NOP },    /* cabinet lamps? */
	{ 0x1f1a0000, 0x1f1a0003, MWA32_NOP },    /* indicates gun trigger */
	{ 0x1f300000, 0x1f5fffff, pcmram_w },
	{ 0x1f680000, 0x1f68001f, mb89371_w },
	{ 0x1f780000, 0x1f780003, MWA32_NOP },    /* watchdog? */
	{ 0x1f800000, 0x1f8003ff, MWA32_BANK1 },  /* scratchpad */
	{ 0x1f801000, 0x1f801007, MWA32_NOP },
	{ 0x1f801008, 0x1f80100b, MWA32_RAM },    /* ?? */
	{ 0x1f80100c, 0x1f80102f, MWA32_NOP },
	{ 0x1f801040, 0x1f80104f, psx_sio_w },
	{ 0x1f801060, 0x1f80106f, MWA32_NOP },
	{ 0x1f801070, 0x1f801077, psx_irq_w },
	{ 0x1f801080, 0x1f8010ff, psx_dma_w },
	{ 0x1f801100, 0x1f80113f, psx_counter_w },
	{ 0x1f801810, 0x1f801817, psx_gpu_w },
	{ 0x1f801820, 0x1f801827, psx_mdec_w },
	{ 0x1f801c00, 0x1f801dff, MWA32_NOP },
	{ 0x1f802020, 0x1f802033, MWA32_RAM },
	{ 0x1f802040, 0x1f802043, MWA32_NOP },
	{ 0x1fc00000, 0x1fffffff, MWA32_ROM },    /* bios mirror */
	{ 0x80000000, 0x803fffff, MWA32_BANK3 },  /* ram mirror */
	{ 0x9fc00000, 0x9fc7ffff, MWA32_ROM },    /* bios mirror */
	{ 0xa0000000, 0xa03fffff, MWA32_BANK5 },  /* ram mirror */
	{ 0xbfc00000, 0xbfc7ffff, MWA32_ROM },    /* bios */
	{ 0xfffe0130, 0xfffe0133, MWA32_NOP },    /* ?? */
MEMORY_END

static MEMORY_READ32_START( konamigq_readmem )
	{ 0x00000000, 0x003fffff, MRA32_RAM },    /* ram */
	{ 0x1f000000, 0x1f00001f, am53cf96_r },
	{ 0x1f100010, 0x1f10001f, soundr3k_r },
	{ 0x1f200000, 0x1f200003, gun1_x_r },
	{ 0x1f208000, 0x1f208003, gun1_y_r },
	{ 0x1f210000, 0x1f210003, gun2_x_r },
	{ 0x1f218000, 0x1f218003, gun2_y_r },
	{ 0x1f220000, 0x1f220003, gun3_x_r },
	{ 0x1f228000, 0x1f228003, gun3_y_r },
	{ 0x1f230000, 0x1f230003, read_inputs_0 },
	{ 0x1f230004, 0x1f230007, read_inputs_1 },
	{ 0x1f238000, 0x1f238003, eeprom_r },
	{ 0x1f300000, 0x1f5fffff, pcmram_r },
	{ 0x1f680000, 0x1f68001f, mb89371_r },
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
	{ 0x1fc00000, 0x1fffffff, MRA32_BANK2 },  /* bios mirror */
	{ 0x80000000, 0x803fffff, MRA32_BANK3 },  /* ram mirror */
	{ 0x9fc00000, 0x9fc7ffff, MRA32_BANK4 },  /* bios mirror */
	{ 0xa0000000, 0xa03fffff, MRA32_BANK5 },  /* ram mirror */
	{ 0xbfc00000, 0xbfc7ffff, MRA32_BANK6 },  /* bios */
MEMORY_END

/* SOUND CPU */

static READ16_HANDLER( dual539_r )
{
	data16_t data;

	data = 0;
	if( ACCESSING_LSB16 )
	{
		data |= K054539_1_r( offset );
	}
	if( ACCESSING_MSB16 )
	{
		data |= K054539_0_r( offset ) << 8;
	}
	return data;
}

static WRITE16_HANDLER( dual539_w )
{
	if( ACCESSING_LSB16 )
	{
		K054539_1_w( offset, data );
	}
	if( ACCESSING_MSB16 )
	{
		K054539_0_w( offset, data >> 8 );
	}
}

static READ16_HANDLER( sndcomm68k_r )
{
	return sndto000[ offset ];
}

static WRITE16_HANDLER( sndcomm68k_w )
{
	sndtor3k[ offset ] = data;
}

/* 68000 memory handling */
static MEMORY_READ16_START( sndreadmem )
	{ 0x000000, 0x07ffff, MRA16_ROM },
	{ 0x100000, 0x10ffff, MRA16_RAM },
	{ 0x200000, 0x2004ff, dual539_r },
	{ 0x300000, 0x300001, tms57002_data_word_r },
	{ 0x400010, 0x40001f, sndcomm68k_r },
	{ 0x500000, 0x500001, tms57002_status_word_r },
MEMORY_END

static MEMORY_WRITE16_START( sndwritemem )
	{ 0x100000, 0x10ffff, MWA16_RAM },
	{ 0x200000, 0x2004ff, dual539_w },
	{ 0x300000, 0x300001, tms57002_data_word_w },
	{ 0x400000, 0x40000f, sndcomm68k_w },
	{ 0x500000, 0x500001, tms57002_control_word_w },
	{ 0x580000, 0x580001, MWA16_NOP }, /* ?? */
MEMORY_END

static struct K054539interface k054539_interface =
{
	2,			/* 2 chips */
	48000,
	{ REGION_SOUND1, REGION_SOUND1 },
	{ { 100, 100 }, { 100, 100 } },
	{ NULL }
};

static void scsi_dma_read( UINT32 n_address, INT32 n_size )
{
	int i;
	UINT32 dest = n_address;

	/* dma size is in 32-bit words */
	n_size <<= 2;

	while (n_size > 0)
	{
		if (n_size < 512)	/* non-READ commands */
		{
			am53cf96_read_data(n_size, &sector_buffer[0]);
			for (i = 0; i < n_size; i++)
			{
				psxwritebyte(dest+i, sector_buffer[i]);
			}
			n_size = 0;
		}
		else
		{
			am53cf96_read_data(512, &sector_buffer[0]);
			for (i = 0; i < 512; i++)
			{
				psxwritebyte(dest+i, sector_buffer[i]);
			}
			dest += 512;
			n_size -= 512;
		}
	}
}

static void scsi_dma_write( UINT32 n_address, INT32 n_size )
{
}

static void scsi_irq(void)
{
	psx_irq_set(0x400);
}

static struct AM53CF96interface scsi_intf =
{
	AM53CF96_DEVICE_HDD,	/* hard disk */
	&scsi_irq,		/* command completion IRQ */
};

static DRIVER_INIT( konamigq )
{
	cpu_setbank( 1, memory_region( REGION_USER1 ) );
	cpu_setbank( 2, memory_region( REGION_USER2 ) );
	cpu_setbank( 3, memory_region( REGION_CPU1 ) );
	cpu_setbank( 4, memory_region( REGION_USER2 ) );
	cpu_setbank( 5, memory_region( REGION_CPU1 ) );
	cpu_setbank( 6, memory_region( REGION_USER2 ) );

	psx_driver_init();

	m_p_n_pcmram = memory_region( REGION_SOUND1 ) + 0x80000;
	m_p_n_ram = memory_region( REGION_CPU1 );

	/* init the scsi controller and hook up it's DMA */
	am53cf96_init(&scsi_intf);
	psx_dma_install_read_handler(5, scsi_dma_read);
	psx_dma_install_write_handler(5, scsi_dma_write);

	state_save_register_UINT8( "konamigq", 0, "PCM RAM", m_p_n_pcmram, 0x380000 );
	state_save_register_UINT8( "konamigq", 0, "sndto000", sndto000, 16 );
	state_save_register_UINT8( "konamigq", 0, "sndtor3k", sndtor3k, 16 );
	state_save_register_UINT8( "konamigq", 0, "sector buffer", sector_buffer, 512);
}

static MACHINE_INIT( konamigq )
{
	psx_machine_init();
	tms57002_init();
}

static MACHINE_DRIVER_START( konamigq )
	/* basic machine hardware */
	MDRV_CPU_ADD( PSXCPU, 33868800 / 2 ) /* 33MHz ?? */
	MDRV_CPU_MEMORY( konamigq_readmem, konamigq_writemem )
	MDRV_CPU_VBLANK_INT( psx_vblank, 1 )

	MDRV_CPU_ADD_TAG( "sound", M68000, 8000000 )
	MDRV_CPU_FLAGS( CPU_AUDIO_CPU )
	MDRV_CPU_MEMORY( sndreadmem, sndwritemem )
	MDRV_CPU_PERIODIC_INT( irq2_line_hold, 480 )

	MDRV_FRAMES_PER_SECOND( 60 )
	MDRV_VBLANK_DURATION( 0 )

	MDRV_MACHINE_INIT( konamigq )
	MDRV_NVRAM_HANDLER( konamigq_93C46 )

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES( VIDEO_TYPE_RASTER )
	MDRV_SCREEN_SIZE( 1024, 1024 )
	MDRV_VISIBLE_AREA( 0, 639, 0, 479 )
	MDRV_PALETTE_LENGTH( 65536 )

	MDRV_PALETTE_INIT( psx )
	MDRV_VIDEO_START( psx_type1_1024x1024 )
	MDRV_VIDEO_UPDATE( konamigq )
	MDRV_VIDEO_STOP( psx )

	/* sound hardware */
	MDRV_SOUND_ATTRIBUTES( SOUND_SUPPORTS_STEREO )
	MDRV_SOUND_ADD( K054539, k054539_interface )
MACHINE_DRIVER_END

INPUT_PORTS_START( konamigq )
	/* IN 0 */
	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BITX( 0x20, IP_ACTIVE_LOW, 0, "Test Switch", KEYCODE_F2, IP_JOY_NONE )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	/* IN 1 */
	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 ) /* trigger */
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 ) /* reload */
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	/* IN 2 */
	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 ) /* trigger */
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 ) /* reload */
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_SERVICE2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	/* IN 3 */
	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START3 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER3 ) /* trigger */
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER3 ) /* reload */
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_SERVICE3 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	/* IN 4 */
	PORT_START
	PORT_DIPNAME( 0x01, 0x01, "Stereo")
	PORT_DIPSETTING(    0x01, "Stereo")
	PORT_DIPSETTING(    0x00, "Mono")
	PORT_DIPNAME( 0x02, 0x00, "Stage Set" )
	PORT_DIPSETTING(    0x02, "Endless" )
	PORT_DIPSETTING(    0x00, "6st End" )
	PORT_DIPNAME( 0x04, 0x04, "Mirror" )
	PORT_DIPSETTING(    0x04, DEF_STR( No ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x08, 0x08, "Woofer" )
	PORT_DIPSETTING(    0x08, DEF_STR( No ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x10, 0x10, "Number of Players" )
	PORT_DIPSETTING(    0x00, "2" )
	PORT_DIPSETTING(    0x10, "3" )
	PORT_DIPNAME( 0x20, 0x20, "Coin Mechanism (2p only)" )
	PORT_DIPSETTING(    0x20, "Common" )
	PORT_DIPSETTING(    0x00, "Independent" )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	/* IN 5 */
	PORT_START /* mask default type                     sens delta min max */
	PORT_ANALOG( 0xff, 0x80, IPT_LIGHTGUN_X | IPF_PLAYER1, 25, 15, 0, 0xff )

	/* IN 6 */
	PORT_START
	PORT_ANALOG( 0xff, 0x80, IPT_LIGHTGUN_Y | IPF_PLAYER1, 25, 15, 0, 0xff )

	/* IN 7 */
	PORT_START
	PORT_ANALOG( 0xff, 0x80, IPT_LIGHTGUN_X | IPF_PLAYER2, 25, 15, 0, 0xff )

	/* IN 8 */
	PORT_START
	PORT_ANALOG( 0xff, 0x80, IPT_LIGHTGUN_Y | IPF_PLAYER2, 25, 15, 0, 0xff )

	/* IN 9 */
	PORT_START
	PORT_ANALOG( 0xff, 0x80, IPT_LIGHTGUN_X | IPF_PLAYER3, 25, 15, 0, 0xff )

	/* IN 10 */
	PORT_START
	PORT_ANALOG( 0xff, 0x80, IPT_LIGHTGUN_Y | IPF_PLAYER3, 25, 15, 0, 0xff )
INPUT_PORTS_END

ROM_START( cryptklr )
	ROM_REGION( 0x0400000, REGION_CPU1, 0 ) /* main ram */
	ROM_REGION( 0x0000400, REGION_USER1, 0 ) /* scratchpad */

	ROM_REGION( 0x80000, REGION_CPU2, 0 ) /* 68000 sound program */
	ROM_LOAD16_WORD_SWAP( "420a01.2g", 0x000000, 0x080000, CRC(84fc2613) SHA1(e06f4284614d33c76529eb43b168d095200a9eac) )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 )
	ROM_LOAD( "420a02.3m",    0x000000, 0x080000, CRC(2169c3c4) SHA1(6d525f10385791e19eb1897d18f0bab319640162) )

	ROM_REGION32_LE( 0x080000, REGION_USER2, 0 ) /* bios */
	ROM_LOAD( "420b03.27p",   0x0000000, 0x080000, CRC(aab391b1) SHA1(bf9dc7c0c8168c22a4be266fe6a66d3738df916b) )

	DISK_REGION( REGION_DISKS )
	DISK_IMAGE( "420uaa04", 0, MD5(179464886f58a2e14b284e3813227a86) SHA1(18fe867c44982bacf0d3ff8453487cd06425a6b7) )
ROM_END

GAMEX( 1995, cryptklr, 0, konamigq, konamigq, konamigq, ROT0, "Konami", "Crypt Killer (ver. UAA)", GAME_IMPERFECT_GRAPHICS )
