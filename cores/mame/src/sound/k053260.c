/*********************************************************

	Konami 053260 PCM Sound Chip

*********************************************************/

#include "driver.h"
#include "k053260.h"

/* 2004-02-28: Fixed ppcm decoding. Games sound much better now.*/

#define LOG 0

#define BASE_SHIFT	16

struct K053260_channel_def {
	unsigned long		rate;
	unsigned long		size;
	unsigned long		start;
	unsigned long		bank;
	unsigned long		volume;
	int					play;
	unsigned long		pan;
	unsigned long		pos;
	int					loop;
	int					ppcm; /* packed PCM ( 4 bit signed ) */
	int					ppcm_data;
};

struct K053260_chip_def {
	int								channel;
	int								mode;
	int								regs[0x30];
	unsigned char					*rom;
	int								rom_size;
	unsigned long					*delta_table;
	struct K053260_channel_def		channels[4];
};

static struct K053260_chip_def *K053260_chip;

/* local copy of the interface */
const struct K053260_interface	*intf;

static void InitDeltaTable( int chip ) {
	int		i;
	double	base = ( double )Machine->sample_rate;
	double	max = (double)( intf->clock[chip] ); /* Hz */
	unsigned long val;

	for( i = 0; i < 0x1000; i++ ) {
		double v = ( double )( 0x1000 - i );
		double target = (max) / v;
		double fixed = ( double )( 1 << BASE_SHIFT );

		if ( target && base ) {
			target = fixed / ( base / target );
			val = ( unsigned long )target;
			if ( val == 0 )
				val = 1;
		} else
			val = 1;

		K053260_chip[chip].delta_table[i] = val;
	}
}

static void K053260_reset( int chip ) {
	int i;

	for( i = 0; i < 4; i++ ) {
		K053260_chip[chip].channels[i].rate = 0;
		K053260_chip[chip].channels[i].size = 0;
		K053260_chip[chip].channels[i].start = 0;
		K053260_chip[chip].channels[i].bank = 0;
		K053260_chip[chip].channels[i].volume = 0;
		K053260_chip[chip].channels[i].play = 0;
		K053260_chip[chip].channels[i].pan = 0;
		K053260_chip[chip].channels[i].pos = 0;
		K053260_chip[chip].channels[i].loop = 0;
		K053260_chip[chip].channels[i].ppcm = 0;
		K053260_chip[chip].channels[i].ppcm_data = 0;
	}
}

#define MAXOUT 0x7fff
#define MINOUT -0x8000

void K053260_update( int param, INT16 **buffer, int length ) {
	static long dpcmcnv[] = { 0,1,2,4,8,16,32,64, -128, -64, -32, -16, -8, -4, -2, -1};

	int i, j, lvol[4], rvol[4], play[4], loop[4], ppcm_data[4], ppcm[4];
	unsigned char *rom[4];
	unsigned long delta[4], end[4], pos[4];
	int dataL, dataR;
	signed char d;
	struct K053260_chip_def *ic = &K053260_chip[param];

	/* precache some values */
	for ( i = 0; i < 4; i++ ) {
		rom[i]= &ic->rom[ic->channels[i].start + ( ic->channels[i].bank << 16 )];
		delta[i] = ic->delta_table[ic->channels[i].rate];
		lvol[i] = ic->channels[i].volume * ic->channels[i].pan;
		rvol[i] = ic->channels[i].volume * ( 8 - ic->channels[i].pan );
		end[i] = ic->channels[i].size;
		pos[i] = ic->channels[i].pos;
		play[i] = ic->channels[i].play;
		loop[i] = ic->channels[i].loop;
		ppcm[i] = ic->channels[i].ppcm;
		ppcm_data[i] = ic->channels[i].ppcm_data;
		if ( ppcm[i] )
			delta[i] /= 2;
	}

		for ( j = 0; j < length; j++ ) {

			dataL = dataR = 0;

			for ( i = 0; i < 4; i++ ) {
				/* see if the voice is on */
				if ( play[i] ) {
					/* see if we're done */
					if ( ( pos[i] >> BASE_SHIFT ) >= end[i] ) {

						ppcm_data[i] = 0;
						if ( loop[i] )
							pos[i] = 0;
						else {
							play[i] = 0;
							continue;
						}
					}

					if ( ppcm[i] ) { /* Packed PCM */
						/* we only update the signal if we're starting or a real sound sample has gone by */
						/* this is all due to the dynamic sample rate convertion */
						if ( pos[i] == 0 || ( ( pos[i] ^ ( pos[i] - delta[i] ) ) & 0x8000 ) == 0x8000 )
								
						 {
							int newdata;
							if ( pos[i] & 0x8000 ){
													
								newdata = ((rom[i][pos[i] >> BASE_SHIFT]) >> 4) & 0x0f; /*high nybble*/
							}
							else{
								newdata = ( ( rom[i][pos[i] >> BASE_SHIFT] ) ) & 0x0f; /*low nybble*/
							}

							ppcm_data[i] = (( ( ppcm_data[i] * 62 ) >> 6 ) + dpcmcnv[newdata]);
							
							if ( ppcm_data[i] > 127 )
								ppcm_data[i] = 127;
							else
								if ( ppcm_data[i] < -128 )
									ppcm_data[i] = -128;
						}
                                                                                   
                                               
                                           		
						d = ppcm_data[i];

						pos[i] += delta[i];
					} else { /* PCM */
						d = rom[i][pos[i] >> BASE_SHIFT];

						pos[i] += delta[i];
					}

					if ( ic->mode & 2 ) {
						dataL += ( d * lvol[i] ) >> 2;
						dataR += ( d * rvol[i] ) >> 2;
					}
				}
			}

         MAME_CLAMP_SAMPLE(dataL);
         MAME_CLAMP_SAMPLE(dataR);
			buffer[1][j] = dataL;
			buffer[0][j] = dataR;
		}

	/* update the regs now */
	for ( i = 0; i < 4; i++ ) {
		ic->channels[i].pos = pos[i];
		ic->channels[i].play = play[i];
		ic->channels[i].ppcm_data = ppcm_data[i];
	}
}

int K053260_sh_start(const struct MachineSound *msound) {
	const char *names[2];
	char ch_names[2][40];
	int i, ics;

	/* Initialize our chip structure */
	intf = msound->sound_interface;

	if ( intf->num > MAX_053260 )
		return -1;

	K053260_chip = ( struct K053260_chip_def * )malloc( sizeof( struct K053260_chip_def ) * intf->num );

	if ( K053260_chip == 0 )
		return -1;

	for( ics = 0; ics < intf->num; ics++ ) {
		struct K053260_chip_def *ic = &K053260_chip[ics];

		ic->mode = 0;
		ic->rom = memory_region(intf->region[ics]);
		ic->rom_size = memory_region_length(intf->region[ics]) - 1;

		K053260_reset( ics );

		for ( i = 0; i < 0x30; i++ )
			ic->regs[i] = 0;

		ic->delta_table = ( unsigned long * )malloc( 0x1000 * sizeof( unsigned long ) );

		if ( ic->delta_table == 0 )
			return -1;

		for ( i = 0; i < 2; i++ ) {
			names[i] = ch_names[i];
			sprintf(ch_names[i],"%s #%d Ch %d",sound_name(msound),ics,i);
		}

		ic->channel = stream_init_multi( 2, names, intf->mixing_level[ics], Machine->sample_rate, ics, K053260_update );

		InitDeltaTable( ics );

		/* setup SH1 timer if necessary */
		if ( intf->irq[ics] )
			timer_pulse( TIME_IN_HZ( ( intf->clock[ics] / 32 ) ), 0, intf->irq[ics] );
	}

    return 0;
}

void K053260_sh_stop( void ) {
	int ics;

	if ( K053260_chip ) {
		for( ics = 0; ics < intf->num; ics++ ) {
			struct K053260_chip_def *ic = &K053260_chip[ics];

			if ( ic->delta_table )
				free( ic->delta_table );

			ic->delta_table = 0;
		}

		free( K053260_chip );

		K053260_chip = 0;
	}
}

static INLINE void check_bounds( int chip, int channel ) {
	struct K053260_chip_def *ic = &K053260_chip[chip];

	int channel_start = ( ic->channels[channel].bank << 16 ) + ic->channels[channel].start;
	int channel_end = channel_start + ic->channels[channel].size - 1;

	if ( channel_start > ic->rom_size ) {
		log_cb(RETRO_LOG_DEBUG, LOGPRE "K53260: Attempting to start playing past the end of the rom ( start = %06x, end = %06x ).\n", channel_start, channel_end );

		ic->channels[channel].play = 0;

		return;
	}

	if ( channel_end > ic->rom_size ) {
		log_cb(RETRO_LOG_DEBUG, LOGPRE "K53260: Attempting to play past the end of the rom ( start = %06x, end = %06x ).\n", channel_start, channel_end );

		ic->channels[channel].size = ic->rom_size - channel_start;
	}
	log_cb(RETRO_LOG_DEBUG, LOGPRE "K053260: Sample Start = %06x, Sample End = %06x, Sample rate = %04lx, PPCM = %s\n", channel_start, channel_end, ic->channels[channel].rate, ic->channels[channel].ppcm ? "yes" : "no" );
}

void K053260_write( int chip, offs_t offset, data8_t data )
{
	int i, t;
	int r = offset;
	int v = data;

	struct K053260_chip_def *ic = &K053260_chip[chip];

	if ( r > 0x2f ) {
		log_cb(RETRO_LOG_DEBUG, LOGPRE "K053260: Writing past registers\n" );
		return;
	}

	if ( Machine->sample_rate != 0 )
		stream_update( ic->channel, 0 );

	/* before we update the regs, we need to check for a latched reg */
	if ( r == 0x28 ) {
		t = ic->regs[r] ^ v;

		for ( i = 0; i < 4; i++ ) {
			if ( t & ( 1 << i ) ) {
				if ( v & ( 1 << i ) ) {
					ic->channels[i].play = 1;
					ic->channels[i].pos = 0;
					ic->channels[i].ppcm_data = 0;
					check_bounds( chip, i );
				} else
					ic->channels[i].play = 0;
			}
		}

		ic->regs[r] = v;
		return;
	}

	/* update regs */
	ic->regs[r] = v;

	/* communication registers */
	if ( r < 8 )
		return;

	/* channel setup */
	if ( r < 0x28 ) {
		int channel = ( r - 8 ) / 8;

		switch ( ( r - 8 ) & 0x07 ) {
			case 0: /* sample rate low */
				ic->channels[channel].rate &= 0x0f00;
				ic->channels[channel].rate |= v;
			break;

			case 1: /* sample rate high */
				ic->channels[channel].rate &= 0x00ff;
				ic->channels[channel].rate |= ( v & 0x0f ) << 8;
			break;

			case 2: /* size low */
				ic->channels[channel].size &= 0xff00;
				ic->channels[channel].size |= v;
			break;

			case 3: /* size high */
				ic->channels[channel].size &= 0x00ff;
				ic->channels[channel].size |= v << 8;
			break;

			case 4: /* start low */
				ic->channels[channel].start &= 0xff00;
				ic->channels[channel].start |= v;
			break;

			case 5: /* start high */
				ic->channels[channel].start &= 0x00ff;
				ic->channels[channel].start |= v << 8;
			break;

			case 6: /* bank */
				ic->channels[channel].bank = v & 0xff;
			break;

			case 7: /* volume is 7 bits. Convert to 8 bits now. */
				ic->channels[channel].volume = ( ( v & 0x7f ) << 1 ) | ( v & 1 );
			break;
		}

		return;
	}

	switch( r ) {
		case 0x2a: /* loop, ppcm */
			for ( i = 0; i < 4; i++ )
				ic->channels[i].loop = ( v & ( 1 << i ) ) != 0;

			for ( i = 4; i < 8; i++ )
				ic->channels[i-4].ppcm = ( v & ( 1 << i ) ) != 0;
		break;

		case 0x2c: /* pan */
			ic->channels[0].pan = v & 7;
			ic->channels[1].pan = ( v >> 3 ) & 7;
		break;

		case 0x2d: /* more pan */
			ic->channels[2].pan = v & 7;
			ic->channels[3].pan = ( v >> 3 ) & 7;
		break;

		case 0x2f: /* control */
			ic->mode = v & 7;
			/* bit 0 = read ROM */
			/* bit 1 = enable sound output */
			/* bit 2 = unknown */
		break;
	}
}

data8_t K053260_read( int chip, offs_t offset )
{
	struct K053260_chip_def *ic = &K053260_chip[chip];

	switch ( offset ) {
		case 0x29: /* channel status */
			{
				int i, status = 0;

				for ( i = 0; i < 4; i++ )
					status |= ic->channels[i].play << i;

				return status;
			}
		break;

		case 0x2e: /* read rom */
			if ( ic->mode & 1 ) {
				unsigned int offs = ic->channels[0].start + ( ic->channels[0].pos >> BASE_SHIFT ) + ( ic->channels[0].bank << 16 );

				ic->channels[0].pos += ( 1 << 16 );

				if ( offs > ic->rom_size ) {
					log_cb(RETRO_LOG_DEBUG, LOGPRE "%06x: K53260: Attempting to read past rom size in rom Read Mode (offs = %06x, size = %06x).\n",activecpu_get_pc(),offs,ic->rom_size );

					return 0;
				}

				return ic->rom[offs];
			}
		break;
	}

	return ic->regs[offset];
}

/**************************************************************************************************/
/* Accesors */

READ_HANDLER( K053260_0_r )
{
	return K053260_read( 0, offset );
}

WRITE_HANDLER( K053260_0_w )
{
	K053260_write( 0, offset, data );
}

READ_HANDLER( K053260_1_r )
{
	return K053260_read( 1, offset );
}

WRITE_HANDLER( K053260_1_w )
{
	K053260_write( 1, offset, data );
}

WRITE16_HANDLER( K053260_0_lsb_w )
{
	if (ACCESSING_LSB)
		K053260_0_w (offset, data & 0xff);
}

READ16_HANDLER( K053260_0_lsb_r )
{
	return K053260_0_r(offset);
}

WRITE16_HANDLER( K053260_1_lsb_w )
{
	if (ACCESSING_LSB)
		K053260_1_w (offset, data & 0xff);
}

READ16_HANDLER( K053260_1_lsb_r )
{
	return K053260_1_r(offset);
}
