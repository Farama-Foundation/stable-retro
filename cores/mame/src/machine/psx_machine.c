/***************************************************************************

  machine/psx.c

  Thanks to Olivier Galibert for IDCT information

***************************************************************************/

#include "driver.h"
#include "state.h"
#include "includes/psx.h"

#define VERBOSE_LEVEL ( 1 )

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

static UINT8 *m_p_n_ram;
static size_t m_n_ramsize;

static INLINE UINT8 psxreadbyte( UINT32 n_address )
{
	return m_p_n_ram[ BYTE_XOR_LE( n_address ) ];
}

static INLINE UINT16 psxreadword( UINT32 n_address )
{
	return *( (UINT16 *)&m_p_n_ram[ WORD_XOR_LE( n_address ) ] );
}

/* IRQ */

static UINT32 m_n_irqdata;
static UINT32 m_n_irqmask;

static void psx_irq_update( void )
{
	if( ( m_n_irqdata & m_n_irqmask ) != 0 )
	{
		verboselog( 2, "psx irq assert\n" );
		cpu_set_irq_line( 0, 0, ASSERT_LINE );
	}
	else
	{
		verboselog( 2, "psx irq clear\n" );
		cpu_set_irq_line( 0, 0, CLEAR_LINE );
	}
}

WRITE32_HANDLER( psx_irq_w )
{
	switch( offset )
	{
	case 0x00:
		m_n_irqdata = ( m_n_irqdata & mem_mask ) | ( m_n_irqdata & m_n_irqmask & data );
		psx_irq_update();
		break;
	case 0x01:
		m_n_irqmask = ( m_n_irqmask & mem_mask ) | data;
		if( ( m_n_irqmask & ~( 0x1 | 0x08 | 0x10 | 0x20 | 0x40 | 0x400 ) ) != 0 )
		{
			verboselog( 0, "psx_irq_w( %08x, %08x, %08x ) unknown irq\n", offset, data, mem_mask );
		}
		psx_irq_update();
		break;
	default:
		verboselog( 0, "psx_irq_w( %08x, %08x, %08x ) unknown register\n", offset, data, mem_mask );
		break;
	}
}

READ32_HANDLER( psx_irq_r )
{
	switch( offset )
	{
	case 0x00:
		verboselog( 1, "psx_irq_r irq data %08x\n", m_n_irqdata );
		return m_n_irqdata;
	case 0x01:
		verboselog( 1, "psx_irq_r irq mask %08x\n", m_n_irqmask );
		return m_n_irqmask;
	default:
		verboselog( 0, "psx_irq_r unknown register %d\n", offset );
		break;
	}
	return 0;
}

void psx_irq_set( UINT32 data )
{
	m_n_irqdata |= data;
	psx_irq_update();
}

/* DMA */

static UINT32 m_p_n_dmabase[ 7 ];
static UINT32 m_p_n_dmablockcontrol[ 7 ];
static UINT32 m_p_n_dmachannelcontrol[ 7 ];
static void *m_p_timer_dma[ 7 ];
static psx_dma_read_handler m_p_fn_dma_read[ 7 ];
static psx_dma_write_handler m_p_fn_dma_write[ 7 ];
static UINT32 m_p_n_dma_lastscanline[ 7 ];
static UINT32 m_n_dpcp;
static UINT32 m_n_dicr;

static void dma_timer( int n_channel, UINT32 n_scanline )
{
	if( n_scanline != 0xffffffff )
	{
		timer_adjust( m_p_timer_dma[ n_channel ], cpu_getscanlinetime( n_scanline ), n_channel, 0 );
	}
	else
	{
		timer_adjust( m_p_timer_dma[ n_channel ], TIME_NEVER, 0, 0 );
	}
	m_p_n_dma_lastscanline[ n_channel ] = n_scanline;
}

void dma_finished( int n_channel )
{
	m_p_n_dmachannelcontrol[ n_channel ] &= ~( ( 1L << 0x18 ) | ( 1L << 0x1c ) );

	if( ( m_n_dicr & ( 1 << ( 16 + n_channel ) ) ) != 0 )
	{
		m_n_dicr |= 0x80000000 | ( 1 << ( 24 + n_channel ) );
		psx_irq_set( 0x0008 );
		verboselog( 2, "dma_finished( %d ) interrupt triggered\n", n_channel );
	}
	else
	{
		verboselog( 2, "dma_finished( %d ) interrupt not enabled\n", n_channel );
	}
	dma_timer( n_channel, 0xffffffff );
}

void psx_dma_install_read_handler( int n_channel, psx_dma_read_handler p_fn_dma_read )
{
	m_p_fn_dma_read[ n_channel ] = p_fn_dma_read;
}

void psx_dma_install_write_handler( int n_channel, psx_dma_read_handler p_fn_dma_write )
{
	m_p_fn_dma_write[ n_channel ] = p_fn_dma_write;
}

WRITE32_HANDLER( psx_dma_w )
{
	static int n_channel;
	n_channel = offset / 4;
	if( n_channel < 7 )
	{
		switch( offset % 4 )
		{
		case 0:
			verboselog( 2, "dmabase( %d ) = %08x\n", n_channel, data );
			m_p_n_dmabase[ n_channel ] = data;
			break;
		case 1:
			verboselog( 2, "dmablockcontrol( %d ) = %08x\n", n_channel, data );
			m_p_n_dmablockcontrol[ n_channel ] = data;
			break;
		case 2:
			verboselog( 2, "dmachannelcontrol( %d ) = %08x\n", n_channel, data );
			m_p_n_dmachannelcontrol[ n_channel ] = data;
			if( ( m_p_n_dmachannelcontrol[ n_channel ] & ( 1L << 0x18 ) ) != 0 && ( m_n_dpcp & ( 1 << ( 3 + ( n_channel * 4 ) ) ) ) != 0 )
			{
				INT32 n_size;
				UINT32 n_address;
				UINT32 n_nextaddress;

				n_address = ( m_p_n_dmabase[ n_channel ] & m_n_ramsize );
				n_size = m_p_n_dmablockcontrol[ n_channel ];
				if( ( m_p_n_dmachannelcontrol[ n_channel ] & 0x200 ) != 0 )
				{
					n_size = ( n_size & 0xffff ) * ( n_size >> 16 );
				}

				if( m_p_n_dmachannelcontrol[ n_channel ] == 0x01000000 && 
					m_p_fn_dma_read[ n_channel ] != NULL )
				{
					verboselog( 1, "dma %d read block %08x %08x\n",
						n_channel, m_p_n_dmabase[ n_channel ], m_p_n_dmablockcontrol[ n_channel ] );
					m_p_fn_dma_read[ n_channel ]( n_address, n_size );
					dma_finished( n_channel );
				}
				else if( m_p_n_dmachannelcontrol[ n_channel ] == 0x01000200 &&
					m_p_fn_dma_read[ n_channel ] != NULL )
				{
					verboselog( 1, "dma %d read block %08x %08x\n",
						n_channel, m_p_n_dmabase[ n_channel ], m_p_n_dmablockcontrol[ n_channel ] );
					m_p_fn_dma_read[ n_channel ]( n_address, n_size );
					if( n_channel == 1 )
					{
						dma_timer( n_channel, cpu_getscanline() + 16 );
					}
					else
					{
						dma_finished( n_channel );
					}
				}
				else if( m_p_n_dmachannelcontrol[ n_channel ] == 0x01000201 &&
					m_p_fn_dma_write[ n_channel ] != NULL )
				{
					verboselog( 1, "dma %d write block %08x %08x\n",
						n_channel, m_p_n_dmabase[ n_channel ], m_p_n_dmablockcontrol[ n_channel ] );
					m_p_fn_dma_write[ n_channel ]( n_address, n_size );
					dma_finished( n_channel );
				}
				else if( m_p_n_dmachannelcontrol[ n_channel ] == 0x01000401 &&
					n_channel == 2 &&
					m_p_fn_dma_write[ n_channel ] != NULL )
				{
					UINT32 n_segments = 0;

					verboselog( 1, "dma %d write linked list %08x\n",
						n_channel, m_p_n_dmabase[ n_channel ] );
					do
					{
						n_address &= m_n_ramsize;
						n_nextaddress = *( (UINT32 *)&m_p_n_ram[ n_address ] );
						m_p_fn_dma_write[ n_channel ]( n_address + 4, n_nextaddress >> 24 );
						n_address = ( n_nextaddress & 0xffffff );

						n_segments++;
						if( n_segments == 10000 )
						{
							/* todo: find a better way of detecting this */
							verboselog( 1, "dma looped\n" );
							break;
						}
					} while( n_address != 0xffffff );
					dma_finished( n_channel );
				}
				else if( m_p_n_dmachannelcontrol[ n_channel ] == 0x11000002 &&
					n_channel == 6 )
				{
					verboselog( 1, "dma 6 reverse clear %08x %08x\n",
						m_p_n_dmabase[ n_channel ], m_p_n_dmablockcontrol[ n_channel ] );
					if( n_size > 0 )
					{
						n_size--;
						while( n_size > 0 )
						{
							n_nextaddress = ( n_address - 4 ) & 0xffffff;
							*( (UINT32 *)&m_p_n_ram[ n_address ] ) = n_nextaddress;
							n_address = n_nextaddress;
							n_size--;
						}
						*( (UINT32 *)&m_p_n_ram[ n_address ] ) = 0xffffff;
					}
					dma_finished( n_channel );
				}
				else
				{
					verboselog( 0, "dma %d unknown mode %08x\n", n_channel, m_p_n_dmachannelcontrol[ n_channel ] );
				}
			}
			else if( m_p_n_dmachannelcontrol[ n_channel ] != 0 )
			{
				verboselog( 1, "psx_dma_w( %04x, %08x, %08x ) channel not enabled\n", offset, m_p_n_dmachannelcontrol[ n_channel ], mem_mask );
			}
			break;
		default:
			verboselog( 1, "psx_dma_w( %04x, %08x, %08x ) Unknown dma channel register\n", offset, data, mem_mask );
			break;
		}
	}
	else
	{
		switch( offset % 4 )
		{
		case 0x0:
			verboselog( 1, "psx_dma_w( %04x, %08x, %08x ) dpcp\n", offset, data, mem_mask );
			m_n_dpcp = ( m_n_dpcp & mem_mask ) | data;
			break;
		case 0x1:
			verboselog( 1, "psx_dma_w( %04x, %08x, %08x ) dicr\n", offset, data, mem_mask );
			m_n_dicr = ( m_n_dicr & mem_mask ) | ( data & 0xffffff );
			break;
		default:
			verboselog( 0, "psx_dma_w( %04x, %08x, %08x ) Unknown dma control register\n", offset, data, mem_mask );
			break;
		}
	}
}

READ32_HANDLER( psx_dma_r )
{
	static int n_channel;
	n_channel = offset / 4;
	if( n_channel < 7 )
	{
		switch( offset % 4 )
		{
		case 0:
			verboselog( 1, "psx_dma_r dmabase[ %d ] ( %08x )\n", n_channel, m_p_n_dmabase[ n_channel ] );
			return m_p_n_dmabase[ n_channel ];
		case 1:
			verboselog( 1, "psx_dma_r dmablockcontrol[ %d ] ( %08x )\n", n_channel, m_p_n_dmablockcontrol[ n_channel ] );
			return m_p_n_dmablockcontrol[ n_channel ];
		case 2:
			verboselog( 1, "psx_dma_r dmachannelcontrol[ %d ] ( %08x )\n", n_channel, m_p_n_dmachannelcontrol[ n_channel ] );
			return m_p_n_dmachannelcontrol[ n_channel ];
		default:
			verboselog( 0, "psx_dma_r( %08x, %08x ) Unknown dma channel register\n", offset, mem_mask );
			break;
		}
	}
	else
	{
		switch( offset % 4 )
		{
		case 0x0:
			verboselog( 1, "psx_dma_r dpcp ( %08x )\n", m_n_dpcp );
			return m_n_dpcp;
		case 0x1:
			verboselog( 1, "psx_dma_r dicr ( %08x )\n", m_n_dicr );
			return m_n_dicr;
		default:
			verboselog( 0, "psx_dma_r( %08x, %08x ) Unknown dma control register\n", offset, mem_mask );
			break;
		}
	}
	return 0;
}

/* Root Counters */

static void *m_p_timer_root[ 7 ];
static data16_t m_p_n_root_count[ 3 ];
static data16_t m_p_n_root_mode[ 3 ];
static data16_t m_p_n_root_target[ 3 ];

static void root_finished( int n_counter )
{
	if( ( m_p_n_root_mode[ n_counter ] & 0x50 ) != 0 )
	{
		psx_irq_set( 0x10 << n_counter );
	}
}

static void root_timer( int n_counter )
{
	int n_duration;

	n_duration = m_p_n_root_target[ n_counter ] - m_p_n_root_count[ n_counter ];
	if( n_duration < 1 )
	{
		n_duration += 0x10000;
	}

	if( n_counter == 0 )
	{
		n_duration *= 1200;
	}
	else if( n_counter == 1 && ( m_p_n_root_mode[ n_counter ] & ( 1 << 0x08 ) ) != 0 )
	{
		n_duration *= 4800;
	}
	else if( n_counter == 2 && ( m_p_n_root_mode[ n_counter ] & ( 1 << 0x09 ) ) != 0 )
	{
		n_duration *= 480;
	}

	timer_adjust( m_p_timer_root[ n_counter ], (double)n_duration / 33868800, n_counter, 0 );
}

WRITE32_HANDLER( psx_counter_w )
{
	int n_counter;

	verboselog( 1, "psx_counter_w ( %08x, %08x, %08x )\n", offset, data, mem_mask );

	n_counter = offset / 4;

	switch( offset % 4 )
	{
	case 0:
		m_p_n_root_count[ n_counter ] = data;
		break;
	case 1:
		m_p_n_root_mode[ n_counter ] = data;
		break;
	case 2:
		m_p_n_root_target[ n_counter ] = data;
		break;
	}

	root_timer( n_counter );
}

READ32_HANDLER( psx_counter_r )
{
	int n_counter;
	data32_t data;

	n_counter = offset / 4;

	switch( offset % 4 )
	{
	case 0:
		if( n_counter == 0 )
		{
			data = ( activecpu_gettotalcycles() / 1200 ) & 0xffff;
		}
		else if( n_counter == 1 && ( m_p_n_root_mode[ n_counter ] & ( 1 << 0x08 ) ) != 0 )
		{
			data = ( activecpu_gettotalcycles() / 4800 ) & 0xffff;
		}
		else if( n_counter == 2 && ( m_p_n_root_mode[ n_counter ] & ( 1 << 0x09 ) ) != 0 )
		{
			data = ( activecpu_gettotalcycles() / 480 ) & 0xffff;
		}
		else
		{
			data = activecpu_gettotalcycles() & 0xffff;
		}
		m_p_n_root_count[ n_counter ] = data;
		break;
	case 1:
		data = m_p_n_root_mode[ n_counter ];
		break;
	case 2:
		data = m_p_n_root_target[ n_counter ];
		break;
	default:
		data = 0;
		break;
	}
	verboselog( 1, "psx_counter_r ( %08x, %08x ) %08x\n", offset, mem_mask, data );
	return data;
}

/* SIO */

static data16_t m_p_n_sio_status[ 2 ];
static data16_t m_p_n_sio_mode[ 2 ];
static data16_t m_p_n_sio_control[ 2 ];
static data16_t m_p_n_sio_baud[ 2 ];
static data8_t *m_p_p_n_sio_buf[ 2 ];
static data16_t m_p_n_sio_rx[ 2 ];
static data16_t m_p_n_sio_read[ 2 ];
static psx_sio_write_handler m_p_f_sio_write[ 2 ];

static void sio_interrupt( int n_port )
{
	verboselog( 1, "sio_interrupt( %d )\n", n_port );
	m_p_n_sio_status[ n_port ] |= ( 1 << 9 );
	psx_irq_set( 0x80 );
}

void psx_sio_send( int n_port, data8_t n_data )
{
	if( m_p_n_sio_rx[ n_port ] < 256 )
	{
		verboselog( 1, "psx_sio_send( %d, %u )\n", n_port, n_data );
		m_p_n_sio_status[ n_port ] |= ( 1 << 1 );
		m_p_p_n_sio_buf[ n_port ][ m_p_n_sio_rx[ n_port ]++ ] = n_data;
		if( ( m_p_n_sio_control[ n_port ] & ( 1 << 11 ) ) != 0 )
		{
			sio_interrupt( n_port );
		}
	}
	else
	{
		verboselog( 0, "psx_sio_send( %d, %u ) buffer overrun\n", n_port, n_data );
	}
}

WRITE32_HANDLER( psx_sio_w )
{
	int n_port;

	n_port = offset / 4;

	switch( offset % 4 )
	{
	case 0:
		verboselog( 1, "psx_sio_w %d data %08x, %08x\n", n_port, data, mem_mask );
		if( ( m_p_n_sio_control[ n_port ] & ( 1 << 10 ) ) != 0 )
		{
			sio_interrupt( n_port );
		}
		if( m_p_f_sio_write[ n_port ] != NULL )
		{
			m_p_f_sio_write[ n_port ]( data );
		}
		else
		{
			psx_sio_send( n_port, 0 ); /* kludge */
		}
		break;
	case 1:
		verboselog( 0, "psx_sio_w( %08x, %08x, %08x )\n", offset, data, mem_mask );
		break;
	case 2:
		if( ACCESSING_LSW32 )
		{
			m_p_n_sio_mode[ n_port ] = data & 0xffff;
			verboselog( 1, "psx_sio_w %d mode %04x\n", n_port, data & 0xffff );
		}
		if( ACCESSING_MSW32 )
		{
			m_p_n_sio_control[ n_port ] = data >> 16;
			verboselog( 1, "psx_sio_w %d control %04x\n", n_port, data >> 16 );
			if( ( m_p_n_sio_control[ n_port ] & ( 1 << 4 ) ) != 0 )
			{
				m_p_n_sio_status[ n_port ] &= ~( 1 << 9 );
				m_p_n_sio_control[ n_port ] &= ~( 1 << 4 );
			}
		}
		break;
	case 3:
		if( ACCESSING_LSW32 )
		{
			verboselog( 0, "psx_sio_w( %08x, %08x, %08x )\n", offset, data, mem_mask );
		}
		if( ACCESSING_MSW32 )
		{
			m_p_n_sio_baud[ n_port ] = data >> 16;
			verboselog( 1, "psx_sio_w %d baud %04x\n", n_port, data >> 16 );
		}
		break;
	default:
		verboselog( 0, "psx_sio_w( %08x, %08x, %08x )\n", offset, data, mem_mask );
		break;
	}
}

READ32_HANDLER( psx_sio_r )
{
	data32_t data;
	int n_port;

	n_port = offset / 4;

	switch( offset % 4 )
	{
	case 0:
		if( m_p_n_sio_rx[ n_port ] != 0 )
		{
			data = m_p_p_n_sio_buf[ n_port ][ m_p_n_sio_read[ n_port ]++ ];
			if( m_p_n_sio_read[ n_port ] == m_p_n_sio_rx[ n_port ] )
			{
				m_p_n_sio_read[ n_port ] = 0;
				m_p_n_sio_rx[ n_port ] = 0;
				m_p_n_sio_status[ n_port ] &= ~( 1 << 1 );
			}
		}
		else
		{
			data = 0;
		}
		verboselog( 1, "psx_sio_r %d data %02x\n", n_port, data );
		break;
	case 1:
		data = m_p_n_sio_status[ n_port ];
		if( ACCESSING_LSW32 )
		{
			verboselog( 1, "psx_sio_r %d status %04x\n", n_port, data & 0xffff );
		}
		if( ACCESSING_MSW32 )
		{
			verboselog( 1, "psx_sio_r %d mode %04x\n", n_port, data >> 16 );
		}
		break;
	case 2:
		data = ( m_p_n_sio_control[ n_port ] << 16 ) | m_p_n_sio_mode[ n_port ];
		if( ACCESSING_LSW32 )
		{
			verboselog( 1, "psx_sio_r %d mode %04x\n", n_port, data & 0xffff );
		}
		if( ACCESSING_MSW32 )
		{
			verboselog( 1, "psx_sio_r %d control %04x\n", n_port, data >> 16 );
		}
		break;
	case 3:
		data = m_p_n_sio_baud[ n_port ] << 16;
		if( ACCESSING_LSW32 )
		{
			verboselog( 0, "psx_sio_r( %08x, %08x ) %08x\n", offset, mem_mask, data );
		}
		if( ACCESSING_MSW32 )
		{
			verboselog( 1, "psx_sio_r %d baud %04x\n", n_port, data >> 16 );
		}
		break;
	default:
		data = 0;
		verboselog( 0, "psx_sio_r( %08x, %08x ) %08x\n", offset, mem_mask, data );
		break;
	}
	return data;
}

void psx_sio_install_write_handler( int n_port, psx_sio_write_handler p_f_write )
{
	m_p_f_sio_write[ n_port ] = p_f_write;
}

/* MDEC */

#define	DCTSIZE ( 8 )
#define	DCTSIZE2 ( DCTSIZE * DCTSIZE )

static INT32 m_p_n_mdec_quantize_y[ DCTSIZE2 ];
static INT32 m_p_n_mdec_quantize_uv[ DCTSIZE2 ];
static INT32 m_p_n_mdec_cos[ DCTSIZE2 ];
static INT32 m_p_n_mdec_cos_precalc[ DCTSIZE2 * DCTSIZE2 ];

static UINT32 m_n_mdec0_command;
static UINT32 m_n_mdec0_address;
static UINT32 m_n_mdec0_size;
static UINT32 m_n_mdec1_command;
static UINT32 m_n_mdec1_status;

static UINT16 m_p_n_mdec_r15[ 256 * 3 ];
static UINT16 m_p_n_mdec_g15[ 256 * 3 ];
static UINT16 m_p_n_mdec_b15[ 256 * 3 ];

static UINT32 m_p_n_mdec_zigzag[ DCTSIZE2 ] =
{
	 0,  1,  8, 16,  9,  2,  3, 10,
	17, 24, 32, 25, 18, 11,  4,  5,
	12, 19, 26, 33, 40, 48, 41, 34,
	27, 20, 13,  6,  7, 14, 21, 28,
	35, 42, 49, 56, 57, 50, 43, 36,
	29, 22, 15, 23, 30, 37, 44, 51,
	58, 59, 52, 45, 38, 31, 39, 46,
	53, 60, 61, 54, 47, 55, 62, 63
};

static INT32 m_p_n_mdec_unpacked[ DCTSIZE2 * 6 * 2 ];

#define MDEC_COS_PRECALC_BITS ( 21 )

static void mdec_cos_precalc( void )
{
	UINT32 n_x;
	UINT32 n_y;
	UINT32 n_u;
	UINT32 n_v;
	INT32 *p_n_precalc;

	p_n_precalc = m_p_n_mdec_cos_precalc;

	for( n_y = 0; n_y < 8; n_y++ )
	{
		for( n_x = 0; n_x < 8; n_x++ )
		{
			for( n_v = 0; n_v < 8; n_v++ )
			{
				for( n_u = 0; n_u < 8; n_u++ )
				{
					*( p_n_precalc++ ) =
						( ( m_p_n_mdec_cos[ ( n_u * 8 ) + n_x ] *
						m_p_n_mdec_cos[ ( n_v * 8 ) + n_y ] ) >> ( 30 - MDEC_COS_PRECALC_BITS ) );
				}
			}
		}
	}
}

static void mdec_idct( INT32 *p_n_src, INT32 *p_n_dst )
{
	UINT32 n_yx;
	UINT32 n_vu;
	INT32 p_n_z[ 8 ];
	INT32 *p_n_data;
	INT32 *p_n_precalc;

	p_n_precalc = m_p_n_mdec_cos_precalc;

	for( n_yx = 0; n_yx < DCTSIZE2; n_yx++ )
	{
		p_n_data = p_n_src;

		memset( p_n_z, 0, sizeof( p_n_z ) );

		for( n_vu = 0; n_vu < DCTSIZE2 / 8; n_vu++ )
		{
			p_n_z[ 0 ] += p_n_data[ 0 ] * p_n_precalc[ 0 ];
			p_n_z[ 1 ] += p_n_data[ 1 ] * p_n_precalc[ 1 ];
			p_n_z[ 2 ] += p_n_data[ 2 ] * p_n_precalc[ 2 ];
			p_n_z[ 3 ] += p_n_data[ 3 ] * p_n_precalc[ 3 ];
			p_n_z[ 4 ] += p_n_data[ 4 ] * p_n_precalc[ 4 ];
			p_n_z[ 5 ] += p_n_data[ 5 ] * p_n_precalc[ 5 ];
			p_n_z[ 6 ] += p_n_data[ 6 ] * p_n_precalc[ 6 ];
			p_n_z[ 7 ] += p_n_data[ 7 ] * p_n_precalc[ 7 ];
			p_n_data += 8;
			p_n_precalc += 8;
		}

		*( p_n_dst++ ) = ( p_n_z[ 0 ] + p_n_z[ 1 ] + p_n_z[ 2 ] + p_n_z[ 3 ] +
			p_n_z[ 4 ] + p_n_z[ 5 ] + p_n_z[ 6 ] + p_n_z[ 7 ] ) >> ( MDEC_COS_PRECALC_BITS + 2 );
	}
}

static INLINE UINT16 mdec_unpack_run( UINT16 n_packed )
{
	return n_packed >> 10;
}

static INLINE INT32 mdec_unpack_val( UINT16 n_packed )
{
	return ( ( (INT32)n_packed ) << 22 ) >> 22;
}

static UINT32 mdec_unpack( UINT32 n_address )
{
	UINT8 n_z;
	INT32 n_qscale;
	UINT16 n_packed;
	UINT32 n_block;
	INT32 *p_n_block;
	INT32 p_n_unpacked[ 64 ];
	INT32 *p_n_q;

	p_n_q = m_p_n_mdec_quantize_uv;
	p_n_block = m_p_n_mdec_unpacked;

	for( n_block = 0; n_block < 6; n_block++ )
	{
		memset( p_n_unpacked, 0, sizeof( p_n_unpacked ) );

		if( n_block == 2 )
		{
			p_n_q = m_p_n_mdec_quantize_y;
		}
		n_packed = psxreadword( n_address );
		n_address += 2;

		n_qscale = mdec_unpack_run( n_packed );

		p_n_unpacked[ 0 ] = mdec_unpack_val( n_packed ) * p_n_q[ 0 ];

		n_z = 0;
		for( ;; )
		{
			n_packed = psxreadword( n_address );
			n_address += 2;

			if( n_packed == 0xfe00 )
			{
				break;
			}
			n_z += mdec_unpack_run( n_packed ) + 1;
			if( n_z > 63 )
			{
				break;
			}
			p_n_unpacked[ m_p_n_mdec_zigzag[ n_z ] ] = ( mdec_unpack_val( n_packed ) * p_n_q[ n_z ] * n_qscale ) / 8;
		}
		mdec_idct( p_n_unpacked, p_n_block );
		p_n_block += DCTSIZE2;
	}
	return n_address;
}

static INLINE INT32 mdec_cr_to_r( INT32 n_cr )
{
	return ( 1435 * n_cr ) >> 10;
}

static INLINE INT32 mdec_cr_to_g( INT32 n_cr )
{
	return ( -731 * n_cr ) >> 10;
}

static INLINE INT32 mdec_cb_to_g( INT32 n_cb )
{
	return ( -351 * n_cb ) >> 10;
}

static INLINE INT32 mdec_cb_to_b( INT32 n_cb )
{
	return ( 1814 * n_cb ) >> 10;
}

static INLINE UINT16 mdec_clamp_r15( INT32 n_r )
{
	return m_p_n_mdec_r15[ n_r + 128 + 256 ];
}

static INLINE UINT16 mdec_clamp_g15( INT32 n_g )
{
	return m_p_n_mdec_g15[ n_g + 128 + 256 ];
}

static INLINE UINT16 mdec_clamp_b15( INT32 n_b )
{
	return m_p_n_mdec_b15[ n_b + 128 + 256 ];
}

static INLINE UINT32 mdec_makergb15( INT32 n_r, INT32 n_g, INT32 n_b, INT32 *p_n_y )
{
	return mdec_clamp_r15( p_n_y[ BYTE_XOR_LE( 0 ) ] + n_r ) | mdec_clamp_g15( p_n_y[ BYTE_XOR_LE( 0 ) ] + n_g ) | mdec_clamp_b15( p_n_y[ BYTE_XOR_LE( 0 ) ] + n_b ) |
		( mdec_clamp_r15( p_n_y[ BYTE_XOR_LE( 1 ) ] + n_r ) | mdec_clamp_g15( p_n_y[ BYTE_XOR_LE( 1 ) ] + n_g ) | mdec_clamp_b15( p_n_y[ BYTE_XOR_LE( 1 ) ] + n_b ) ) << 16;
}

static void mdec_yuv2_to_rgb15( UINT32 n_address )
{
	INT32 n_r;
	INT32 n_g;
	INT32 n_b;
	INT32 n_cb;
	INT32 n_cr;
	INT32 *p_n_cb;
	INT32 *p_n_cr;
	INT32 *p_n_y;
	UINT32 n_x;
	UINT32 n_y;
	UINT32 n_z;
	UINT32 n_stp;

	if( ( m_n_mdec0_command & ( 1L << 25 ) ) != 0 )
	{
		n_stp = 0x80008000;
	}
	else
	{
		n_stp = 0x00000000;
	}

	p_n_cb = &m_p_n_mdec_unpacked[ 0 ];
	p_n_cr = &m_p_n_mdec_unpacked[ DCTSIZE2 ];
	p_n_y = &m_p_n_mdec_unpacked[ DCTSIZE2 * 2 ];

	for( n_z = 0; n_z < 2; n_z++ )
	{
		for( n_y = 0; n_y < 4; n_y++ )
		{
			for( n_x = 0; n_x < 4; n_x++ )
			{
				n_cr = *( p_n_cr );
				n_cb = *( p_n_cb );
				n_r = mdec_cr_to_r( n_cr );
				n_g = mdec_cr_to_g( n_cr ) + mdec_cb_to_g( n_cb );
				n_b = mdec_cb_to_b( n_cb );

				*( (UINT32 *)&m_p_n_ram[ n_address ] ) = mdec_makergb15( n_r, n_g, n_b, p_n_y ) | n_stp;
				*( (UINT32 *)&m_p_n_ram[ n_address + 32 ] ) = mdec_makergb15( n_r, n_g, n_b, p_n_y + 8 ) | n_stp;

				n_cr = *( p_n_cr + 4 );
				n_cb = *( p_n_cb + 4 );
				n_r = mdec_cr_to_r( n_cr );
				n_g = mdec_cr_to_g( n_cr ) + mdec_cb_to_g( n_cb );
				n_b = mdec_cb_to_b( n_cb );

				*( (UINT32 *)&m_p_n_ram[ n_address + 16 ] ) = mdec_makergb15( n_r, n_g, n_b, p_n_y + DCTSIZE2 ) | n_stp;
				*( (UINT32 *)&m_p_n_ram[ n_address + 48 ] ) = mdec_makergb15( n_r, n_g, n_b, p_n_y + DCTSIZE2 + 8 ) | n_stp;

				p_n_cr++;
				p_n_cb++;
				p_n_y += 2;
				n_address += 4;
			}
			p_n_cr += 4;
			p_n_cb += 4;
			p_n_y += 8;
			n_address += 48;
		}
		p_n_y += DCTSIZE2;
	}
}

static void mdec0_write( UINT32 n_address, INT32 n_size )
{
	int n_index;

	switch( m_n_mdec0_command >> 28 )
	{
	case 0x3:
		verboselog( 1, "mdec decode %08x %08x %08x\n", m_n_mdec0_command, n_address, n_size );
		m_n_mdec0_address = n_address;
		m_n_mdec0_size = n_size;
		m_n_mdec1_status |= ( 1L << 29 );
		break;
	case 0x4:
		verboselog( 1, "mdec quantize table %08x %08x %08x\n", m_n_mdec0_command, n_address, n_size );
		for( n_index = 0; n_index < DCTSIZE2; n_index++ )
		{
			m_p_n_mdec_quantize_y[ n_index ] = psxreadbyte( n_address + n_index );
			m_p_n_mdec_quantize_uv[ n_index ] = psxreadbyte( n_address + n_index + DCTSIZE2 );
		}
		break;
	case 0x6:
		verboselog( 1, "mdec cosine table %08x %08x %08x\n", m_n_mdec0_command, n_address, n_size );
		for( n_index = 0; n_index < DCTSIZE2; n_index++ )
		{
			m_p_n_mdec_cos[ n_index ] = (INT16)psxreadword( n_address + ( n_index * 2 ) );
		}
		mdec_cos_precalc();
		break;
	default:
		verboselog( 0, "mdec unknown command %08x %08x %08x\n", m_n_mdec0_command, n_address, n_size );
		break;
	}
}

static void mdec1_read( UINT32 n_address, INT32 n_size )
{
	if( ( m_n_mdec0_command & ( 1L << 29 ) ) != 0 )
	{
		while( n_size > 0 )
		{
			m_n_mdec0_address = mdec_unpack( m_n_mdec0_address );
			mdec_yuv2_to_rgb15( n_address );
			n_address += ( 16 * 16 ) * 2;
			n_size -= ( 16 * 16 ) / 2;
		}
	}
	else
	{
		verboselog( 0, "mdec 24bit not supported\n" );
	}
	m_n_mdec1_status &= ~( 1L << 29 );
}

WRITE32_HANDLER( psx_mdec_w )
{
	switch( offset )
	{
	case 0:
		verboselog( 2, "mdec 0 command %08x\n", data );
		m_n_mdec0_command = data;
		break;
	case 1:
		verboselog( 2, "mdec 1 command %08x\n", data );
		m_n_mdec1_command = data;
		break;
	}
}

READ32_HANDLER( psx_mdec_r )
{
	switch( offset )
	{
	case 0:
		return 0;
	case 1:
		return m_n_mdec1_status;
	}
	return 0;
}

static void gpu_read( UINT32 n_address, INT32 n_size )
{
	psx_gpu_read( (UINT32 *)&m_p_n_ram[ n_address ], n_size );
}

static void gpu_write( UINT32 n_address, INT32 n_size )
{
	psx_gpu_write( (UINT32 *)&m_p_n_ram[ n_address ], n_size );
}

void psx_machine_init( void )
{
	int n_channel;

	/* irq */
	m_n_irqdata = 0;
	m_n_irqmask = 0;

	/* dma */
	m_n_dpcp = 0;
	m_n_dicr = 0;

	m_n_mdec0_command = 0;
	m_n_mdec0_address = 0;
	m_n_mdec0_size = 0;
	m_n_mdec1_command = 0;
	m_n_mdec1_status = 0;

	for( n_channel = 0; n_channel < 7; n_channel++ )
	{
		m_p_n_dma_lastscanline[ n_channel ] = 0xffffffff;
	}

	psx_gpu_reset();
}

static void psx_postload( void )
{
	int n;

	psx_irq_update();

	for( n = 0; n < 7; n++ )
	{
		dma_timer( n, m_p_n_dma_lastscanline[ n ] );
	}

	for( n = 0; n < 3; n++ )
	{
		root_timer( n );
	}

	mdec_cos_precalc();
}

void psx_driver_init( void )
{
	int n;

	for( n = 0; n < 7; n++ )
	{
		m_p_timer_dma[ n ] = timer_alloc( dma_finished );
		m_p_fn_dma_read[ n ] = NULL;
		m_p_fn_dma_write[ n ] = NULL;
	}

	for( n = 0; n < 3; n++ )
	{
		m_p_timer_root[ n ] = timer_alloc( root_finished );
	}

	for( n = 0; n < 256; n++ )
	{
		m_p_n_mdec_r15[ n ] = 0;
		m_p_n_mdec_r15[ n + 256 ] = ( n >> 3 ) << 10;
		m_p_n_mdec_r15[ n + 512 ] = ( 255 >> 3 ) << 10;

		m_p_n_mdec_g15[ n ] = 0;
		m_p_n_mdec_g15[ n + 256 ] = ( n >> 3 ) << 5;
		m_p_n_mdec_g15[ n + 512 ] = ( 255 >> 3 ) << 5;

		m_p_n_mdec_b15[ n ] = 0;
		m_p_n_mdec_b15[ n + 256 ] = ( n >> 3 );
		m_p_n_mdec_b15[ n + 512 ] = ( 255 >> 3 );
	}

	for( n = 0; n < 2; n++ )
	{
		m_p_n_sio_status[ n ] = ( 1 << 2 ) | ( 1 << 0 );
		m_p_n_sio_mode[ n ] = 0;
		m_p_n_sio_control[ n ] = 0;
		m_p_n_sio_baud[ n ] = 0;
		m_p_p_n_sio_buf[ n ] = malloc( 256 );
		m_p_n_sio_rx[ n ] = 0;
		m_p_n_sio_read[ n ] = 0;
		m_p_f_sio_write[ n ] = NULL;
	}

	psx_dma_install_read_handler( 1, mdec1_read );
	psx_dma_install_read_handler( 2, gpu_read );

	psx_dma_install_write_handler( 0, mdec0_write );
	psx_dma_install_write_handler( 2, gpu_write );

	m_p_n_ram = memory_region( REGION_CPU1 );
	m_n_ramsize = memory_region_length( REGION_CPU1 ) - 1;

	state_save_register_UINT32( "psx", 0, "m_n_irqdata", &m_n_irqdata, 1 );
	state_save_register_UINT32( "psx", 0, "m_n_irqmask", &m_n_irqmask, 1 );
	state_save_register_UINT32( "psx", 0, "m_p_n_dmabase", m_p_n_dmabase, 7 );
	state_save_register_UINT32( "psx", 0, "m_p_n_dmablockcontrol", m_p_n_dmablockcontrol, 7 );
	state_save_register_UINT32( "psx", 0, "m_p_n_dmachannelcontrol", m_p_n_dmachannelcontrol, 7 );
	state_save_register_UINT32( "psx", 0, "m_p_n_dma_lastscanline", m_p_n_dma_lastscanline, 7 );
	state_save_register_UINT32( "psx", 0, "m_n_dpcp", &m_n_dpcp, 1 );
	state_save_register_UINT32( "psx", 0, "m_n_dicr", &m_n_dicr, 1 );
	state_save_register_UINT16( "psx", 0, "m_p_n_root_count", m_p_n_root_count, 3 );
	state_save_register_UINT16( "psx", 0, "m_p_n_root_mode", m_p_n_root_mode, 3 );
	state_save_register_UINT16( "psx", 0, "m_p_n_root_target", m_p_n_root_target, 3 );
	state_save_register_UINT16( "psx", 0, "m_p_n_sio_status", m_p_n_sio_status, 2 );
	state_save_register_UINT16( "psx", 0, "m_p_n_sio_mode", m_p_n_sio_mode, 2 );
	state_save_register_UINT16( "psx", 0, "m_p_n_sio_control", m_p_n_sio_control, 2 );
	state_save_register_UINT16( "psx", 0, "m_p_n_sio_baud", m_p_n_sio_baud, 2 );
	state_save_register_UINT8( "psx", 0, "m_p_p_n_sio_buf0", m_p_p_n_sio_buf[ 0 ], 256 );
	state_save_register_UINT8( "psx", 0, "m_p_p_n_sio_buf1", m_p_p_n_sio_buf[ 1 ], 256 );
	state_save_register_UINT16( "psx", 0, "m_p_n_sio_rx", m_p_n_sio_rx, 2 );
	state_save_register_UINT16( "psx", 0, "m_p_n_sio_read", m_p_n_sio_read, 2 );
	state_save_register_UINT32( "psx", 0, "m_n_mdec0_command", &m_n_mdec0_command, 1 );
	state_save_register_UINT32( "psx", 0, "m_n_mdec0_address", &m_n_mdec0_address, 1 );
	state_save_register_UINT32( "psx", 0, "m_n_mdec0_size", &m_n_mdec0_size, 1 );
	state_save_register_UINT32( "psx", 0, "m_n_mdec1_command", &m_n_mdec1_command, 1 );
	state_save_register_UINT32( "psx", 0, "m_n_mdec1_status", &m_n_mdec1_status, 1 );
	state_save_register_INT32( "psx", 0, "m_p_n_mdec_quantize_y", m_p_n_mdec_quantize_y, DCTSIZE2 );
	state_save_register_INT32( "psx", 0, "m_p_n_mdec_quantize_uv", m_p_n_mdec_quantize_uv, DCTSIZE2 );
	state_save_register_INT32( "psx", 0, "m_p_n_mdec_cos", m_p_n_mdec_cos, DCTSIZE2 );

	state_save_register_func_postload( psx_postload );
}
