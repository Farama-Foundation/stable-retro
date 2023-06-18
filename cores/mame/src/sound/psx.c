/***************************************************************************

	PSX SPU

	CXD2922BQ/CXD2925Q

	preliminary version by smf.

***************************************************************************/

#include "driver.h"
#include "includes/psx.h"
#include "state.h"


static UINT8 *m_p_n_ram;
static size_t m_n_ramsize;

static data16_t m_n_mainvolumeleft;
static data16_t m_n_mainvolumeright;
static data16_t m_n_reverberationdepthleft;
static data16_t m_n_reverberationdepthright;
static data32_t m_n_voiceon;
static data32_t m_n_voiceoff;
static data32_t m_n_modulationmode;
static data32_t m_n_noisemode;
static data32_t m_n_reverbmode;
static data32_t m_n_channelonoff;
static data16_t m_n_reverbworkareastart;
static data16_t m_n_irqaddress;
static data16_t m_n_spudata;
static data16_t m_n_spucontrol;
static data16_t m_n_spustatus;
static data16_t m_n_cdvolumeleft;
static data16_t m_n_cdvolumeright;
static data16_t m_n_externalvolumeleft;
static data16_t m_n_externalvolumeright;
static data16_t m_p_n_volumeleft[ 24 ];
static data16_t m_p_n_volumeright[ 24 ];
static data16_t m_p_n_pitch[ 24 ];
static data16_t m_p_n_address[ 24 ];
static data16_t m_p_n_attackdecaysustain[ 24 ];
static data16_t m_p_n_sustainrelease[ 24 ];
static data16_t m_p_n_adsrvolume[ 24 ];
static data16_t m_p_n_repeataddress[ 24 ];
static data32_t m_p_n_effect[ 16 ];
static data32_t *m_p_n_spuram;

#define SPU_REG( a ) ( ( a - 0xc00 ) / 4 )
#define SPU_CHANNEL_REG( a ) ( a / 4 )

static INLINE data32_t psxreadlong( data32_t n_address )
{
	return *( (data32_t *)&m_p_n_ram[ n_address ] );
}

static INLINE void psxwritelong( data32_t n_address, data32_t n_data )
{
	*( (data32_t *)&m_p_n_ram[ n_address ] ) = n_data;
}

static void spu_read( UINT32 n_address, INT32 n_size )
{
	data32_t n_spuoffset;

	log_cb(RETRO_LOG_DEBUG, "spu_read( %08x, %08x )\n", n_address, n_size );

	n_spuoffset = m_n_irqaddress * 2;
	while( n_size > 0 )
	{
		psxwritelong( n_address, m_p_n_spuram[ n_spuoffset ] );
		n_spuoffset++;
		n_address += 4;
		n_size--;
	}
}

static void spu_write( UINT32 n_address, INT32 n_size )
{
	data32_t n_spuoffset;

	log_cb(RETRO_LOG_DEBUG, "spu_write( %08x, %08x )\n", n_address, n_size );

	n_spuoffset = m_n_irqaddress * 2;
	while( n_size > 0 )
	{
		m_p_n_spuram[ n_spuoffset ] = psxreadlong( n_address );
		n_spuoffset++;
		n_address += 4;
		n_size--;
	}
}

int PSX_sh_start( const struct MachineSound *msound )
{
	int n_effect;
	int n_channel;

	m_p_n_ram = memory_region( REGION_CPU1 );
	m_n_ramsize = memory_region_length( REGION_CPU1 ) - 1;

	m_n_mainvolumeleft = 0;
	m_n_mainvolumeright = 0;
	m_n_reverberationdepthleft = 0;
	m_n_reverberationdepthright = 0;
	m_n_voiceon = 0;
	m_n_voiceoff = 0;
	m_n_modulationmode = 0;
	m_n_noisemode = 0;
	m_n_reverbmode = 0;
	m_n_channelonoff = 0;
	m_n_reverbworkareastart = 0;
	m_n_irqaddress = 0;
	m_n_spudata = 0;
	m_n_spucontrol = 0;
	m_n_spustatus = 0;
	m_n_cdvolumeleft = 0;
	m_n_cdvolumeright = 0;
	m_n_externalvolumeleft = 0;
	m_n_externalvolumeright = 0;

	for( n_channel = 0; n_channel < 24; n_channel++ )
	{
		m_p_n_volumeleft[ n_channel ] = 0;
		m_p_n_volumeright[ n_channel ] = 0;
		m_p_n_pitch[ n_channel ] = 0;
		m_p_n_address[ n_channel ] = 0;
		m_p_n_attackdecaysustain[ n_channel ] = 0;
		m_p_n_sustainrelease[ n_channel ] = 0;
		m_p_n_adsrvolume[ n_channel ] = 0;
		m_p_n_repeataddress[ n_channel ] = 0;
	}

	for( n_effect = 0; n_effect < 16; n_effect++ )
	{
		m_p_n_effect[ n_effect ] = 0;
	}

	m_p_n_spuram = malloc( 0x80000 );
	if( m_p_n_spuram == NULL )
	{
		return 1;
	}

	state_save_register_UINT16( "psx", 0, "m_n_mainvolumeleft", &m_n_mainvolumeleft, 1 );
	state_save_register_UINT16( "psx", 0, "m_n_mainvolumeright", &m_n_mainvolumeright, 1 );
	state_save_register_UINT16( "psx", 0, "m_n_reverberationdepthleft", &m_n_reverberationdepthleft, 1 );
	state_save_register_UINT16( "psx", 0, "m_n_reverberationdepthright", &m_n_reverberationdepthright, 1 );
	state_save_register_UINT16( "psx", 0, "m_n_reverberationdepthleft", &m_n_reverberationdepthleft, 1 );
	state_save_register_UINT16( "psx", 0, "m_n_reverberationdepthright", &m_n_reverberationdepthright, 1 );
	state_save_register_UINT32( "psx", 0, "m_n_voiceon", &m_n_voiceon, 1 );
	state_save_register_UINT32( "psx", 0, "m_n_voiceoff", &m_n_voiceoff, 1 );
	state_save_register_UINT32( "psx", 0, "m_n_modulationmode", &m_n_modulationmode, 1 );
	state_save_register_UINT32( "psx", 0, "m_n_noisemode", &m_n_noisemode, 1 );
	state_save_register_UINT32( "psx", 0, "m_n_reverbmode", &m_n_reverbmode, 1 );
	state_save_register_UINT32( "psx", 0, "m_n_channelonoff", &m_n_channelonoff, 1 );
	state_save_register_UINT16( "psx", 0, "m_n_reverbworkareastart", &m_n_reverbworkareastart, 1 );
	state_save_register_UINT16( "psx", 0, "m_n_irqaddress", &m_n_irqaddress, 1 );
	state_save_register_UINT16( "psx", 0, "m_n_spudata", &m_n_spudata, 1 );
	state_save_register_UINT16( "psx", 0, "m_n_spucontrol", &m_n_spucontrol, 1 );
	state_save_register_UINT16( "psx", 0, "m_n_spustatus", &m_n_spustatus, 1 );
	state_save_register_UINT16( "psx", 0, "m_n_cdvolumeleft", &m_n_cdvolumeleft, 1 );
	state_save_register_UINT16( "psx", 0, "m_n_cdvolumeright", &m_n_cdvolumeright, 1 );
	state_save_register_UINT16( "psx", 0, "m_n_externalvolumeleft", &m_n_externalvolumeleft, 1 );
	state_save_register_UINT16( "psx", 0, "m_n_externalvolumeright", &m_n_externalvolumeright, 1 );
	state_save_register_UINT16( "psx", 0, "m_p_n_volumeleft", m_p_n_volumeleft, 24 );
	state_save_register_UINT16( "psx", 0, "m_p_n_volumeright", m_p_n_volumeright, 24 );
	state_save_register_UINT16( "psx", 0, "m_p_n_pitch", m_p_n_pitch, 24 );
	state_save_register_UINT16( "psx", 0, "m_p_n_address", m_p_n_address, 24 );
	state_save_register_UINT16( "psx", 0, "m_p_n_attackdecaysustain", m_p_n_attackdecaysustain, 24 );
	state_save_register_UINT16( "psx", 0, "m_p_n_sustainrelease", m_p_n_sustainrelease, 24 );
	state_save_register_UINT16( "psx", 0, "m_p_n_adsrvolume", m_p_n_adsrvolume, 24 );
	state_save_register_UINT16( "psx", 0, "m_p_n_repeataddress", m_p_n_repeataddress, 24 );
	state_save_register_UINT32( "psx", 0, "m_p_n_effect", m_p_n_effect, 16 );
	state_save_register_UINT32( "psx", 0, "m_p_n_spuram", m_p_n_spuram, 0x80000 / 4 );

	psx_dma_install_read_handler( 4, spu_read );
	psx_dma_install_write_handler( 4, spu_write );

	return 0;
}

void PSX_sh_stop( void )
{
	free( m_p_n_spuram );
	m_p_n_spuram = NULL;
}

void PSX_sh_update( void )
{
}

void PSX_sh_reset( void )
{
}

READ32_HANDLER( psx_spu_delay_r )
{
	log_cb(RETRO_LOG_DEBUG, "psx_spu_delay_r()\n", m_n_voiceon );
	return 0;
}

READ32_HANDLER( psx_spu_r )
{
	int n_channel;
	n_channel = offset / 4;
	if( n_channel < 24 )
	{
		switch( offset % 4 )
		{
		case SPU_CHANNEL_REG( 0xc ):
			if( ACCESSING_LSW32 )
			{
				log_cb(RETRO_LOG_DEBUG, "psx_spu_r() channel %d adsr volume = %04x\n", n_channel, m_p_n_adsrvolume[ n_channel ] );
			}
			if( ACCESSING_MSW32 )
			{
				log_cb(RETRO_LOG_DEBUG, "psx_spu_r() channel %d repeat address = %04x\n", n_channel, m_p_n_repeataddress[ n_channel ] );
			}
			return ( m_p_n_repeataddress[ n_channel ] << 16 ) | m_p_n_adsrvolume[ n_channel ];
		default:
			log_cb(RETRO_LOG_DEBUG, "psx_spu_r( %08x, %08x ) channel %d reg %d\n", offset, mem_mask, n_channel, offset % 4 ); 
			return 0;
		}
	}
	else
	{
		switch( offset )
		{
		case SPU_REG( 0xd88 ):
			log_cb(RETRO_LOG_DEBUG, "psx_spu_r() voice on = %08x\n", m_n_voiceon );
			return m_n_voiceon;
		case SPU_REG( 0xd8c ):
			log_cb(RETRO_LOG_DEBUG, "psx_spu_r() voice off = %08x\n", m_n_voiceoff );
			return m_n_voiceoff;
		case SPU_REG( 0xd98 ):
			log_cb( RETRO_LOG_DEBUG, "psx_spu_r() reverb mode = %08x\n", m_n_reverbmode );
			return m_n_reverbmode;
		case SPU_REG( 0xda4 ):
			log_cb(RETRO_LOG_DEBUG, "psx_spu_r() irq address = %08x\n", m_n_irqaddress << 16 );
			return m_n_irqaddress << 16;
		case SPU_REG( 0xda8 ):
			log_cb(RETRO_LOG_DEBUG, "psx_spu_r() spu data/control = %08x\n", m_n_spudata | ( m_n_spucontrol << 16 ) );
			return m_n_spudata | ( m_n_spucontrol << 16 );
		case SPU_REG( 0xdac ):
			log_cb(RETRO_LOG_DEBUG, "psx_spu_r() spu status = %08x\n", m_n_spustatus );
			return m_n_spustatus;
		default:
			log_cb(RETRO_LOG_DEBUG, "psx_spu_r( %08x, %08x ) %08x\n", offset, mem_mask, 0xc00 + ( offset * 4 ) );
			return 0;
		}
	}
}

WRITE32_HANDLER( psx_spu_w )
{
	int n_channel;
	n_channel = offset / 4;
	if( n_channel < 24 )
	{
		switch( offset % 4 )
		{
		case SPU_CHANNEL_REG( 0x0 ):
			if( ACCESSING_LSW32 )
			{
				m_p_n_volumeleft[ n_channel ] = offset & 0xffff;
				log_cb( RETRO_LOG_DEBUG, "psx_spu_w() channel %d volume left = %04x\n", n_channel, m_p_n_volumeleft[ n_channel ] );
			}
			if( ACCESSING_MSW32 )
			{
				m_p_n_volumeright[ n_channel ] = offset >> 16;
				log_cb( RETRO_LOG_DEBUG, "psx_spu_w() channel %d volume right = %04x\n", n_channel, m_p_n_volumeright[ n_channel ] );
			}
			break;
		case SPU_CHANNEL_REG( 0x4 ):
			if( ACCESSING_LSW32 )
			{
				m_p_n_pitch[ n_channel ] = offset & 0xffff;
				log_cb( RETRO_LOG_DEBUG, "psx_spu_w() channel %d pitch = %04x\n", n_channel, m_p_n_pitch[ n_channel ] );
			}
			if( ACCESSING_MSW32 )
			{
				m_p_n_address[ n_channel ] = offset >> 16;
				log_cb( RETRO_LOG_DEBUG, "psx_spu_w() channel %d address = %04x\n", n_channel, m_p_n_address[ n_channel ] );
			}
			break;
		case SPU_CHANNEL_REG( 0x8 ):
			if( ACCESSING_LSW32 )
			{
				m_p_n_attackdecaysustain[ n_channel ] = offset & 0xffff;
				log_cb( RETRO_LOG_DEBUG, "psx_spu_w() channel %d attack/decay/sustain = %04x\n", n_channel, m_p_n_attackdecaysustain[ n_channel ] );
			}
			if( ACCESSING_MSW32 )
			{
				m_p_n_sustainrelease[ n_channel ] = offset >> 16;
				log_cb( RETRO_LOG_DEBUG, "psx_spu_w() channel %d sustain/release = %04x\n", n_channel, m_p_n_sustainrelease[ n_channel ] );
			}
			break;
		case SPU_CHANNEL_REG( 0xc ):
			if( ACCESSING_LSW32 )
			{
				m_p_n_adsrvolume[ n_channel ] = offset & 0xffff;
				log_cb( RETRO_LOG_DEBUG, "psx_spu_w() channel %d adsr volume = %04x\n", n_channel, m_p_n_adsrvolume[ n_channel ] );
			}
			if( ACCESSING_MSW32 )
			{
				m_p_n_repeataddress[ n_channel ] = offset >> 16;
				log_cb( RETRO_LOG_DEBUG, "psx_spu_w() channel %d repeat address = %04x\n", n_channel, m_p_n_repeataddress[ n_channel ] );
			}
			break;
		default:
			log_cb( RETRO_LOG_DEBUG, "psx_spu_w( %08x, %08x, %08x ) channel %d reg %d\n", offset, mem_mask, data, n_channel, offset % 4 ); 
			break;
		}
	}
	else
	{
		switch( offset )
		{
		case SPU_REG( 0xd80 ):
			if( ACCESSING_LSW32 )
			{
				m_n_mainvolumeleft = data & 0xffff;
				log_cb( RETRO_LOG_DEBUG, "psx_spu_w() main volume left = %04x\n", m_n_mainvolumeleft );
			}
			if( ACCESSING_MSW32 )
			{
				m_n_mainvolumeright = data >> 16;
				log_cb( RETRO_LOG_DEBUG, "psx_spu_w() main volume right = %04x\n", m_n_mainvolumeright );
			}
			break;
		case SPU_REG( 0xd84 ):
			if( ACCESSING_LSW32 )
			{
				m_n_reverberationdepthleft = data & 0xffff;
				log_cb( RETRO_LOG_DEBUG, "psx_spu_w() reverberation depth left = %04x\n", m_n_reverberationdepthleft );
			}
			if( ACCESSING_MSW32 )
			{
				m_n_reverberationdepthright = data >> 16;
				log_cb( RETRO_LOG_DEBUG, "psx_spu_w() reverberation depth right = %04x\n", m_n_reverberationdepthright );
			}
			break;
		case SPU_REG( 0xd88 ):
			COMBINE_DATA( &m_n_voiceon );
			log_cb( RETRO_LOG_DEBUG, "psx_spu_w() voice on = %08x\n", m_n_voiceon );
			break;
		case SPU_REG( 0xd8c ):
			COMBINE_DATA( &m_n_voiceoff );
			log_cb( RETRO_LOG_DEBUG, "psx_spu_w() voice off = %08x\n", m_n_voiceoff );
			break;
		case SPU_REG( 0xd90 ):
			COMBINE_DATA( &m_n_modulationmode );
			log_cb( RETRO_LOG_DEBUG, "psx_spu_w() modulation mode = %08x\n", m_n_modulationmode );
			break;
		case SPU_REG( 0xd94 ):
			COMBINE_DATA( &m_n_noisemode );
			log_cb( RETRO_LOG_DEBUG, "psx_spu_w() noise mode = %08x\n", m_n_noisemode );
			break;
		case SPU_REG( 0xd98 ):
			COMBINE_DATA( &m_n_reverbmode );
			log_cb( RETRO_LOG_DEBUG, "psx_spu_w() reverb mode = %08x\n", m_n_reverbmode );
			break;
		case SPU_REG( 0xd9c ):
			COMBINE_DATA( &m_n_channelonoff );
			log_cb( RETRO_LOG_DEBUG, "psx_spu_w() channel on/off = %08x\n", m_n_channelonoff );
			break;
		case SPU_REG( 0xda0 ):
			if( ACCESSING_LSW32 )
			{
				log_cb( RETRO_LOG_DEBUG, "psx_spu_w( %08x, %08x, %08x ) %08x\n", offset, mem_mask, data, 0xc00 + ( offset * 4 ) ); 
			}
			if( ACCESSING_MSW32 )
			{
				m_n_reverbworkareastart = data >> 16;
				log_cb( RETRO_LOG_DEBUG, "psx_spu_w() reverb work area start = %04x\n", m_n_reverbworkareastart );
			}
			break;
		case SPU_REG( 0xda4 ):
			if( ACCESSING_LSW32 )
			{
				log_cb( RETRO_LOG_DEBUG, "psx_spu_w( %08x, %08x, %08x ) %08x\n", offset, mem_mask, data, 0xc00 + ( offset * 4 ) ); 
			}
			if( ACCESSING_MSW32 )
			{
				m_n_irqaddress = data >> 16;
				log_cb( RETRO_LOG_DEBUG, "psx_spu_w() irq address = %04x\n", m_n_irqaddress );
			}
			break;
		case SPU_REG( 0xda8 ):
			if( ACCESSING_LSW32 )
			{
				m_n_spudata = data & 0xffff;
				log_cb( RETRO_LOG_DEBUG, "psx_spu_w() spu data = %04x\n", m_n_spudata );
			}
			if( ACCESSING_MSW32 )
			{
				m_n_spucontrol = data >> 16;
				log_cb( RETRO_LOG_DEBUG, "psx_spu_w() spu control = %04x\n", m_n_spucontrol );
			}
			break;
		case SPU_REG( 0xdac ):
			if( ACCESSING_LSW32 )
			{
				m_n_spustatus = data & 0xffff;
				log_cb( RETRO_LOG_DEBUG, "psx_spu_w() spu status = %04x\n", m_n_spustatus );
			}
			if( ACCESSING_MSW32 )
			{
				log_cb( RETRO_LOG_DEBUG, "psx_spu_w( %08x, %08x, %08x ) %08x\n", offset, mem_mask, data, 0xc00 + ( offset * 4 ) ); 
			}
			break;
		case SPU_REG( 0xdb0 ):
			if( ACCESSING_LSW32 )
			{
				m_n_cdvolumeleft = data & 0xffff;
				log_cb( RETRO_LOG_DEBUG, "psx_spu_w() cd volume left = %04x\n", m_n_cdvolumeleft );
			}
			if( ACCESSING_MSW32 )
			{
				m_n_cdvolumeright = data >> 16;
				log_cb( RETRO_LOG_DEBUG, "psx_spu_w() cd volume right = %04x\n", m_n_cdvolumeright );
			}
			break;
		case SPU_REG( 0xdb4 ):
			if( ACCESSING_LSW32 )
			{
				m_n_externalvolumeleft = data & 0xffff;
				log_cb( RETRO_LOG_DEBUG, "psx_spu_w() external volume left = %04x\n", m_n_externalvolumeleft );
			}
			if( ACCESSING_MSW32 )
			{
				m_n_externalvolumeright = data >> 16;
				log_cb( RETRO_LOG_DEBUG, "psx_spu_w() external volume right = %04x\n", m_n_externalvolumeright );
			}
			break;
		case SPU_REG( 0xdc0 ):
		case SPU_REG( 0xdc4 ):
		case SPU_REG( 0xdc8 ):
		case SPU_REG( 0xdcc ):
		case SPU_REG( 0xdd0 ):
		case SPU_REG( 0xdd4 ):
		case SPU_REG( 0xdd8 ):
		case SPU_REG( 0xddc ):
		case SPU_REG( 0xde0 ):
		case SPU_REG( 0xde4 ):
		case SPU_REG( 0xde8 ):
		case SPU_REG( 0xdec ):
		case SPU_REG( 0xdf0 ):
		case SPU_REG( 0xdf4 ):
		case SPU_REG( 0xdf8 ):
		case SPU_REG( 0xdfc ):
			COMBINE_DATA( &m_p_n_effect[ offset & 0x0f ] );
			log_cb( RETRO_LOG_DEBUG, "psx_spu_w() effect %d = %04x\n", offset & 0x0f, m_p_n_effect[ offset & 0x0f ] );
			break;
		default:
			log_cb( RETRO_LOG_DEBUG, "psx_spu_w( %08x, %08x, %08x ) %08x\n", offset, mem_mask, data, 0xc00 + ( offset * 4 ) ); 
			break;
		}
	}
}
