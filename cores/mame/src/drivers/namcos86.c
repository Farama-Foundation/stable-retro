/*******************************************************************
Rolling Thunder
(C) 1986 Namco

To Do:
-----
Remove sprite lag (watch the "bullets" signs on the walls during scrolling).
  Increasing vblank_duration does it but some sprites flicker.

Add correct dipswitches and potentially fix controls in Wonder Momo.

Notes:
-----
PCM roms sample tables:
At the beggining of each PCM sound ROM you can find a 2 byte
offset to the beggining of each sample in the rom. Since the
table is not in sequential order, it is possible that the order
of the table is actually the sound number. Each sample ends in
a 0xff mark.

*******************************************************************/

#include "driver.h"
#include "cpu/m6809/m6809.h"
#include "cpu/m6800/m6800.h"

extern unsigned char *rthunder_videoram1, *rthunder_videoram2, *spriteram, *dirtybuffer;

/*******************************************************************/

PALETTE_INIT( namcos86 );
VIDEO_START( namcos86 );
VIDEO_UPDATE( namcos86 );
READ_HANDLER( rthunder_videoram1_r );
WRITE_HANDLER( rthunder_videoram1_w );
READ_HANDLER( rthunder_videoram2_r );
WRITE_HANDLER( rthunder_videoram2_w );
WRITE_HANDLER( rthunder_scroll0_w );
WRITE_HANDLER( rthunder_scroll1_w );
WRITE_HANDLER( rthunder_scroll2_w );
WRITE_HANDLER( rthunder_scroll3_w );
WRITE_HANDLER( rthunder_backcolor_w );
WRITE_HANDLER( rthunder_tilebank_select_0_w );
WRITE_HANDLER( rthunder_tilebank_select_1_w );



/*******************************************************************/

/* Sampled voices (Modified and Added by Takahiro Nogi. 1999/09/26) */

/* signed/unsigned 8-bit conversion macros */
#define AUDIO_CONV(A) ((A)^0x80)

static int rt_totalsamples[7];
static int rt_decode_mode;


static int rt_decode_sample(const struct MachineSound *msound)
{
	struct GameSamples *samples;
	unsigned char *src, *scan, *dest, last=0;
	int size, n = 0, j;
	int decode_mode;

	j = memory_region_length(REGION_SOUND1);
	if (j == 0) return 0;	/* no samples in this game */
	else if (j == 0x80000)	/* genpeitd */
		rt_decode_mode = 1;
	else
		rt_decode_mode = 0;

	log_cb(RETRO_LOG_DEBUG, LOGPRE "pcm decode mode:%d\n", rt_decode_mode );
	if (rt_decode_mode != 0) {
		decode_mode = 6;
	} else {
		decode_mode = 4;
	}

	/* get amount of samples */
	for ( j = 0; j < decode_mode; j++ ) {
		src = memory_region(REGION_SOUND1)+ ( j * 0x10000 );
		rt_totalsamples[j] = ( ( src[0] << 8 ) + src[1] ) / 2;
		n += rt_totalsamples[j];
		log_cb(RETRO_LOG_DEBUG, LOGPRE "rt_totalsamples[%d]:%d\n", j, rt_totalsamples[j] );
	}

	/* calculate the amount of headers needed */
	size = sizeof( struct GameSamples ) + n * sizeof( struct GameSamples * );

	/* allocate */
	if ( ( Machine->samples = auto_malloc( size ) ) == NULL )
		return 1;

	samples = Machine->samples;
	samples->total = n;

	for ( n = 0; n < samples->total; n++ ) {
		int indx, start, offs;

		if ( n < rt_totalsamples[0] ) {
			src = memory_region(REGION_SOUND1);
			indx = n;
		} else
			if ( ( n - rt_totalsamples[0] ) < rt_totalsamples[1] ) {
				src = memory_region(REGION_SOUND1)+0x10000;
				indx = n - rt_totalsamples[0];
			} else
				if ( ( n - ( rt_totalsamples[0] + rt_totalsamples[1] ) ) < rt_totalsamples[2] ) {
					src = memory_region(REGION_SOUND1)+0x20000;
					indx = n - ( rt_totalsamples[0] + rt_totalsamples[1] );
				} else
					if ( ( n - ( rt_totalsamples[0] + rt_totalsamples[1] + rt_totalsamples[2] ) ) < rt_totalsamples[3] ) {
						src = memory_region(REGION_SOUND1)+0x30000;
						indx = n - ( rt_totalsamples[0] + rt_totalsamples[1] + rt_totalsamples[2] );
					} else
						if ( ( n - ( rt_totalsamples[0] + rt_totalsamples[1] + rt_totalsamples[2] + rt_totalsamples[3] ) ) < rt_totalsamples[4] ) {
							src = memory_region(REGION_SOUND1)+0x40000;
							indx = n - ( rt_totalsamples[0] + rt_totalsamples[1] + rt_totalsamples[2] + rt_totalsamples[3] );
						} else
							if ( ( n - ( rt_totalsamples[0] + rt_totalsamples[1] + rt_totalsamples[2] + rt_totalsamples[3] + rt_totalsamples[4] ) ) < rt_totalsamples[5] ) {
								src = memory_region(REGION_SOUND1)+0x50000;
								indx = n - ( rt_totalsamples[0] + rt_totalsamples[1] + rt_totalsamples[2] + rt_totalsamples[3] + rt_totalsamples[4] );
							} else {
								src = memory_region(REGION_SOUND1)+0x60000;
								indx = n - ( rt_totalsamples[0] + rt_totalsamples[1] + rt_totalsamples[2] + rt_totalsamples[3] + rt_totalsamples[4] + rt_totalsamples[5] );
							}

		/* calculate header offset */
		offs = indx * 2;

		/* get sample start offset */
		start = ( src[offs] << 8 ) + src[offs+1];

		/* calculate the sample size */
		scan = &src[start];
		size = 0;

		while ( *scan != 0xff ) {
			if ( *scan == 0x00 ) { /* run length encoded data start tag */
				/* get RLE size */
				size += scan[1] + 1;
				scan += 2;
			} else {
				size++;
				scan++;
			}
		}

		/* allocate sample */
		if ( ( samples->sample[n] = auto_malloc( sizeof( struct GameSample ) + size * sizeof( unsigned char ) ) ) == NULL )
			return 1;

		/* fill up the sample info */
		samples->sample[n]->length = size;
		samples->sample[n]->smpfreq = 6000;	/* 6 kHz */
		samples->sample[n]->resolution = 8;	/* 8 bit */

		/* unpack sample */
		dest = (unsigned char *)samples->sample[n]->data;
		scan = &src[start];

		while ( *scan != 0xff ) {
			if ( *scan == 0x00 ) { /* run length encoded data start tag */
				int i;
				for ( i = 0; i <= scan[1]; i++ ) /* unpack RLE */
					*dest++ = last;

				scan += 2;
			} else {
				last = AUDIO_CONV( scan[0] );
				*dest++ = last;
				scan++;
			}
		}
	}

	return 0; /* no errors */
}


/* play voice sample (Modified and Added by Takahiro Nogi. 1999/09/26) */
static int voice[2];

static void namco_voice_play( int offset, int data, int ch ) {

	if ( voice[ch] == -1 )
		sample_stop( ch );
	else
		sample_start( ch, voice[ch], 0 );
}

static WRITE_HANDLER( namco_voice0_play_w ) {

	namco_voice_play(offset, data, 0);
}

static WRITE_HANDLER( namco_voice1_play_w ) {

	namco_voice_play(offset, data, 1);
}

/* select voice sample (Modified and Added by Takahiro Nogi. 1999/09/26) */
static void namco_voice_select( int offset, int data, int ch ) {

	log_cb(RETRO_LOG_DEBUG, LOGPRE "Voice %d mode: %d select: %02x\n", ch, rt_decode_mode, data );

	if ( data == 0 )
		sample_stop( ch );

	if (rt_decode_mode != 0) {
		switch ( data & 0xe0 ) {
			case 0x00:
			break;

			case 0x20:
				data &= 0x1f;
				data += rt_totalsamples[0];
			break;

			case 0x40:
				data &= 0x1f;
				data += rt_totalsamples[0] + rt_totalsamples[1];
			break;

			case 0x60:
				data &= 0x1f;
				data += rt_totalsamples[0] + rt_totalsamples[1] + rt_totalsamples[2];
			break;

			case 0x80:
				data &= 0x1f;
				data += rt_totalsamples[0] + rt_totalsamples[1] + rt_totalsamples[2] + rt_totalsamples[3];
			break;

			case 0xa0:
				data &= 0x1f;
				data += rt_totalsamples[0] + rt_totalsamples[1] + rt_totalsamples[2] + rt_totalsamples[3] + rt_totalsamples[4];
			break;

			case 0xc0:
				data &= 0x1f;
				data += rt_totalsamples[0] + rt_totalsamples[1] + rt_totalsamples[2] + rt_totalsamples[3] + rt_totalsamples[4] + rt_totalsamples[5];
			break;

			case 0xe0:
				data &= 0x1f;
				data += rt_totalsamples[0] + rt_totalsamples[1] + rt_totalsamples[2] + rt_totalsamples[3] + rt_totalsamples[4] + rt_totalsamples[5] + rt_totalsamples[6];
			break;
		}
	} else {
		switch ( data & 0xc0 ) {
			case 0x00:
			break;

			case 0x40:
				data &= 0x3f;
				data += rt_totalsamples[0];
			break;

			case 0x80:
				data &= 0x3f;
				data += rt_totalsamples[0] + rt_totalsamples[1];
			break;

			case 0xc0:
				data &= 0x3f;
				data += rt_totalsamples[0] + rt_totalsamples[1] + rt_totalsamples[2];
			break;
		}
	}

	voice[ch] = data - 1;
}

static WRITE_HANDLER( namco_voice0_select_w ) {

	namco_voice_select(offset, data, 0);
}

static WRITE_HANDLER( namco_voice1_select_w ) {

	namco_voice_select(offset, data, 1);
}
/*******************************************************************/

/* shared memory area with the mcu */
static unsigned char *shared1;
static READ_HANDLER( shared1_r ) { return shared1[offset]; }
static WRITE_HANDLER( shared1_w ) { shared1[offset] = data; }



static WRITE_HANDLER( spriteram_w )
{
	spriteram[offset] = data;
}
static READ_HANDLER( spriteram_r )
{
	return spriteram[offset];
}

static WRITE_HANDLER( bankswitch1_w )
{
	unsigned char *base = memory_region(REGION_CPU1) + 0x10000;

	/* if the ROM expansion module is available, don't do anything. This avoids conflict */
	/* with bankswitch1_ext_w() in wndrmomo */
	if (memory_region(REGION_USER1)) return;

	cpu_setbank(1,base + ((data & 0x03) * 0x2000));
}

static WRITE_HANDLER( bankswitch1_ext_w )
{
	unsigned char *base = memory_region(REGION_USER1);

	if (base == 0) return;

	cpu_setbank(1,base + ((data & 0x1f) * 0x2000));
}

static WRITE_HANDLER( bankswitch2_w )
{
	unsigned char *base = memory_region(REGION_CPU2) + 0x10000;

	cpu_setbank(2,base + ((data & 0x03) * 0x2000));
}

/* Stubs to pass the correct Dip Switch setup to the MCU */
static READ_HANDLER( dsw0_r )
{
	int rhi, rlo;

	rhi = ( readinputport( 2 ) & 0x01 ) << 4;
	rhi |= ( readinputport( 2 ) & 0x04 ) << 3;
	rhi |= ( readinputport( 2 ) & 0x10 ) << 2;
	rhi |= ( readinputport( 2 ) & 0x40 ) << 1;

	rlo = ( readinputport( 3 ) & 0x01 );
	rlo |= ( readinputport( 3 ) & 0x04 ) >> 1;
	rlo |= ( readinputport( 3 ) & 0x10 ) >> 2;
	rlo |= ( readinputport( 3 ) & 0x40 ) >> 3;

	return ~( rhi | rlo ) & 0xff; /* Active Low */
}

static READ_HANDLER( dsw1_r )
{
	int rhi, rlo;

	rhi = ( readinputport( 2 ) & 0x02 ) << 3;
	rhi |= ( readinputport( 2 ) & 0x08 ) << 2;
	rhi |= ( readinputport( 2 ) & 0x20 ) << 1;
	rhi |= ( readinputport( 2 ) & 0x80 );

	rlo = ( readinputport( 3 ) & 0x02 ) >> 1;
	rlo |= ( readinputport( 3 ) & 0x08 ) >> 2;
	rlo |= ( readinputport( 3 ) & 0x20 ) >> 3;
	rlo |= ( readinputport( 3 ) & 0x80 ) >> 4;

	return ~( rhi | rlo ) & 0xff; /* Active Low */
}

static int int_enabled[2];

static WRITE_HANDLER( int_ack1_w )
{
	int_enabled[0] = 1;
}

static WRITE_HANDLER( int_ack2_w )
{
	int_enabled[1] = 1;
}

static INTERRUPT_GEN( namco86_interrupt1 )
{
	if (int_enabled[0])
	{
		int_enabled[0] = 0;
		cpu_set_irq_line(0, 0, HOLD_LINE);
	}
}

static INTERRUPT_GEN( namco86_interrupt2 )
{
	if (int_enabled[1])
	{
		int_enabled[1] = 0;
		cpu_set_irq_line(1, 0, HOLD_LINE);
	}
}

static WRITE_HANDLER( namcos86_coin_w )
{
	coin_lockout_global_w(data & 1);
	coin_counter_w(0,~data & 2);
	coin_counter_w(1,~data & 4);
}

static WRITE_HANDLER( namcos86_led_w )
{
	set_led_status(0,data & 0x08);
	set_led_status(1,data & 0x10);
}


/*******************************************************************/

static MEMORY_READ_START( readmem1 )
	{ 0x0000, 0x1fff, rthunder_videoram1_r },
	{ 0x2000, 0x3fff, rthunder_videoram2_r },
	{ 0x4000, 0x40ff, namcos1_wavedata_r }, /* PSG device, shared RAM */
	{ 0x4100, 0x413f, namcos1_sound_r }, /* PSG device, shared RAM */
	{ 0x4000, 0x43ff, shared1_r },
	{ 0x4400, 0x5fff, spriteram_r },
	{ 0x6000, 0x7fff, MRA_BANK1 },
	{ 0x8000, 0xffff, MRA_ROM },
MEMORY_END

static MEMORY_WRITE_START( writemem1 )
	{ 0x0000, 0x1fff, rthunder_videoram1_w, &rthunder_videoram1 },
	{ 0x2000, 0x3fff, rthunder_videoram2_w, &rthunder_videoram2 },

	{ 0x4000, 0x40ff, namcos1_wavedata_w, &namco_wavedata }, /* PSG device, shared RAM */
	{ 0x4100, 0x413f, namcos1_sound_w, &namco_soundregs }, /* PSG device, shared RAM */
	{ 0x4000, 0x43ff, shared1_w, &shared1 },

	{ 0x4400, 0x5fff, spriteram_w, &spriteram },

	{ 0x6000, 0x6000, namco_voice0_play_w },
	{ 0x6200, 0x6200, namco_voice0_select_w },
	{ 0x6400, 0x6400, namco_voice1_play_w },
	{ 0x6600, 0x6600, namco_voice1_select_w },
	{ 0x6800, 0x6800, bankswitch1_ext_w },
/*	{ 0x6c00, 0x6c00, MWA_NOP },  // ??? /*/
/*	{ 0x6e00, 0x6e00, MWA_NOP },  // ??? /*/

	{ 0x8000, 0x8000, watchdog_reset_w },
	{ 0x8400, 0x8400, int_ack1_w }, /* IRQ acknowledge */
	{ 0x8800, 0x8800, rthunder_tilebank_select_0_w },
	{ 0x8c00, 0x8c00, rthunder_tilebank_select_1_w },

	{ 0x9000, 0x9002, rthunder_scroll0_w },	/* scroll + priority */
	{ 0x9003, 0x9003, bankswitch1_w },
	{ 0x9004, 0x9006, rthunder_scroll1_w },	/* scroll + priority */

	{ 0x9400, 0x9402, rthunder_scroll2_w },	/* scroll + priority */
/*	{ 0x9403, 0x9403 } sub CPU rom bank select would be here*/
	{ 0x9404, 0x9406, rthunder_scroll3_w },	/* scroll + priority */

	{ 0xa000, 0xa000, rthunder_backcolor_w },

	{ 0x8000, 0xffff, MWA_ROM },
MEMORY_END


#define CPU2_MEMORY(NAME,ADDR_SPRITE,ADDR_VIDEO1,ADDR_VIDEO2,ADDR_ROM,ADDR_BANK,ADDR_WDOG,ADDR_INT)	\
static MEMORY_READ_START( NAME##_readmem2 )											\
	{ ADDR_SPRITE+0x0000, ADDR_SPRITE+0x03ff, MRA_RAM },							\
	{ ADDR_SPRITE+0x0400, ADDR_SPRITE+0x1fff, spriteram_r },						\
	{ ADDR_VIDEO1+0x0000, ADDR_VIDEO1+0x1fff, rthunder_videoram1_r },				\
	{ ADDR_VIDEO2+0x0000, ADDR_VIDEO2+0x1fff, rthunder_videoram2_r },				\
	{ ADDR_ROM+0x0000, ADDR_ROM+0x1fff, MRA_BANK2 },								\
	{ 0x8000, 0xffff, MRA_ROM },													\
MEMORY_END																			\
																					\
static MEMORY_WRITE_START( NAME##_writemem2 )										\
	{ ADDR_SPRITE+0x0000, ADDR_SPRITE+0x03ff, MWA_RAM },							\
	{ ADDR_SPRITE+0x0400, ADDR_SPRITE+0x1fff, spriteram_w },						\
	{ ADDR_VIDEO1+0x0000, ADDR_VIDEO1+0x1fff, rthunder_videoram1_w },				\
	{ ADDR_VIDEO2+0x0000, ADDR_VIDEO2+0x1fff, rthunder_videoram2_w },				\
/*	{ ADDR_BANK+0x00, ADDR_BANK+0x02 } layer 2 scroll registers would be here */	\
	{ ADDR_BANK+0x03, ADDR_BANK+0x03, bankswitch2_w },								\
/*	{ ADDR_BANK+0x04, ADDR_BANK+0x06 } layer 3 scroll registers would be here */	\
	{ ADDR_WDOG, ADDR_WDOG, watchdog_reset_w },										\
	{ ADDR_INT, ADDR_INT, int_ack2_w },	/* IRQ acknowledge */						\
	{ ADDR_ROM+0x0000, ADDR_ROM+0x1fff, MWA_ROM },									\
	{ 0x8000, 0xffff, MWA_ROM },													\
MEMORY_END

#define UNUSED 0x4000
/*                     SPRITE  VIDEO1  VIDEO2  ROM     BANK    WDOG    IRQACK */
CPU2_MEMORY( hopmappy, UNUSED, UNUSED, UNUSED, UNUSED, UNUSED, 0x9000, UNUSED )
CPU2_MEMORY( skykiddx, UNUSED, UNUSED, UNUSED, UNUSED, UNUSED, 0x9000, 0x9400 )
CPU2_MEMORY( roishtar, 0x0000, 0x6000, 0x4000, UNUSED, UNUSED, 0xa000, 0xb000 )
CPU2_MEMORY( genpeitd, 0x4000, 0x0000, 0x2000, UNUSED, UNUSED, 0xb000, 0x8800 )
CPU2_MEMORY( rthunder, 0x0000, 0x2000, 0x4000, 0x6000, 0xd800, 0x8000, 0x8800 )
CPU2_MEMORY( wndrmomo, 0x2000, 0x4000, 0x6000, UNUSED, UNUSED, 0xc000, 0xc800 )
#undef UNUSED


#define MCU_MEMORY(NAME,ADDR_LOWROM,ADDR_INPUT,ADDR_UNK1,ADDR_UNK2)			\
static MEMORY_READ_START( NAME##_mcu_readmem )								\
	{ 0x0000, 0x001f, hd63701_internal_registers_r },						\
	{ 0x0080, 0x00ff, MRA_RAM },											\
	{ 0x1000, 0x10ff, namcos1_wavedata_r }, /* PSG device, shared RAM */	\
	{ 0x1100, 0x113f, namcos1_sound_r }, /* PSG device, shared RAM */		\
	{ 0x1000, 0x13ff, shared1_r },											\
	{ 0x1400, 0x1fff, MRA_RAM },											\
	{ ADDR_INPUT+0x00, ADDR_INPUT+0x01, YM2151_status_port_0_r },			\
	{ ADDR_INPUT+0x20, ADDR_INPUT+0x20, input_port_0_r },					\
	{ ADDR_INPUT+0x21, ADDR_INPUT+0x21, input_port_1_r },					\
	{ ADDR_INPUT+0x30, ADDR_INPUT+0x30, dsw0_r },							\
	{ ADDR_INPUT+0x31, ADDR_INPUT+0x31, dsw1_r },							\
	{ ADDR_LOWROM, ADDR_LOWROM+0x3fff, MRA_ROM },							\
	{ 0x8000, 0xbfff, MRA_ROM },											\
	{ 0xf000, 0xffff, MRA_ROM },											\
MEMORY_END																	\
																			\
static MEMORY_WRITE_START( NAME##_mcu_writemem )							\
	{ 0x0000, 0x001f, hd63701_internal_registers_w },						\
	{ 0x0080, 0x00ff, MWA_RAM },											\
	{ 0x1000, 0x10ff, namcos1_wavedata_w }, /* PSG device, shared RAM */	\
	{ 0x1100, 0x113f, namcos1_sound_w }, /* PSG device, shared RAM */		\
	{ 0x1000, 0x13ff, shared1_w },											\
	{ 0x1400, 0x1fff, MWA_RAM },											\
	{ ADDR_INPUT+0x00, ADDR_INPUT+0x00, YM2151_register_port_0_w },			\
	{ ADDR_INPUT+0x01, ADDR_INPUT+0x01, YM2151_data_port_0_w },				\
	{ ADDR_UNK1, ADDR_UNK1, MWA_NOP }, /* ??? written (not always) at end of interrupt */	\
	{ ADDR_UNK2, ADDR_UNK2, MWA_NOP }, /* ??? written (not always) at end of interrupt */	\
	{ ADDR_LOWROM, ADDR_LOWROM+0x3fff, MWA_ROM },							\
	{ 0x8000, 0xbfff, MWA_ROM },											\
	{ 0xf000, 0xffff, MWA_ROM },											\
MEMORY_END

#define UNUSED 0x4000
/*                    LOWROM   INPUT    UNK1    UNK2 */
MCU_MEMORY( hopmappy, UNUSED, 0x2000, 0x8000, 0x8800 )
MCU_MEMORY( skykiddx, UNUSED, 0x2000, 0x8000, 0x8800 )
MCU_MEMORY( roishtar, 0x0000, 0x6000, 0x8000, 0x9800 )
MCU_MEMORY( genpeitd, 0x4000, 0x2800, 0xa000, 0xa800 )
MCU_MEMORY( rthunder, 0x4000, 0x2000, 0xb000, 0xb800 )
MCU_MEMORY( wndrmomo, 0x4000, 0x3800, 0xc000, 0xc800 )
#undef UNUSED


static READ_HANDLER( readFF )
{
	return 0xff;
}

static PORT_READ_START( mcu_readport )
	{ HD63701_PORT1, HD63701_PORT1, input_port_4_r },
	{ HD63701_PORT2, HD63701_PORT2, readFF },	/* leds won't work otherwise */
PORT_END

static PORT_WRITE_START( mcu_writeport )
	{ HD63701_PORT1, HD63701_PORT1, namcos86_coin_w },
	{ HD63701_PORT2, HD63701_PORT2, namcos86_led_w },
PORT_END


/*******************************************************************/

INPUT_PORTS_START( hopmappy )
	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* button 3 player 2 */
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* button 2 player 1 */
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BITX( 0x80, 0x80, IPT_SERVICE, "Service Switch", KEYCODE_F1, IP_JOY_NONE )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* button 3 player 1 */
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* button 2 player 2 */
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START      /* DSWA */
	PORT_DIPNAME( 0x03, 0x00, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 1C_2C ) )
	PORT_DIPNAME( 0x04, 0x00, "Allow Continue" )
	PORT_DIPSETTING(    0x04, DEF_STR( No ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x18, 0x00, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x08, "1" )
	PORT_DIPSETTING(    0x10, "2" )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x18, "5" )
	PORT_DIPNAME( 0x60, 0x00, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x60, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 1C_2C ) )
	PORT_SERVICE( 0x80, IP_ACTIVE_HIGH )

	PORT_START      /* DSWB */
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x02, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x04, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	PORT_BITX(    0x10, 0x00, IPT_DIPSWITCH_NAME | IPF_CHEAT, "Level Select", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x10, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x00, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x20, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x00, "Easy" )
	PORT_DIPSETTING(    0x80, "Hard" )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_SPECIAL )	/* OUT:coin lockout */
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_SPECIAL )	/* OUT:coin counter 1 */
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SPECIAL )	/* OUT:coin counter 2 */
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_COCKTAIL )
INPUT_PORTS_END

INPUT_PORTS_START( skykiddx )
	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* button 3 player 2 */
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BITX( 0x80, 0x80, IPT_SERVICE, "Service Switch", KEYCODE_F1, IP_JOY_NONE )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* button 3 player 1 */
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START      /* DSWA */
	PORT_DIPNAME( 0x03, 0x00, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 1C_2C ) )
	PORT_DIPNAME( 0x04, 0x00, "Freeze" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x04, DEF_STR( On ) )
	PORT_BITX(    0x08, 0x00, IPT_DIPSWITCH_NAME | IPF_CHEAT, "Level Select", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x60, 0x00, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x60, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 1C_2C ) )
	PORT_SERVICE( 0x80, IP_ACTIVE_HIGH )

	PORT_START      /* DSWB */
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x01, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x02, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x04, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x10, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x20, "20000 80000" )
	PORT_DIPSETTING(    0x00, "30000 90000" )
	PORT_DIPNAME( 0xc0, 0x00, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x40, "1" )
	PORT_DIPSETTING(    0x80, "2" )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0xc0, "5" )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_SPECIAL )	/* OUT:coin lockout */
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_SPECIAL )	/* OUT:coin counter 1 */
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SPECIAL )	/* OUT:coin counter 2 */
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_COCKTAIL )
INPUT_PORTS_END

INPUT_PORTS_START( roishtar )
	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* button 3 player 2 */
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_DOWN   | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICKRIGHT_DOWN  | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICKRIGHT_RIGHT | IPF_8WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* button 3 player 1 */
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_UP  | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICKRIGHT_UP | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START      /* DSWA */
	PORT_DIPNAME( 0x07, 0x00, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x07, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x05, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_6C ) )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x10, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x20, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_SERVICE( 0x80, IP_ACTIVE_HIGH )

	PORT_START      /* DSWB */
	PORT_DIPNAME( 0x07, 0x00, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x07, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x05, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_6C ) )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x10, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x20, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x00, "Freeze" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x80, DEF_STR( On ) )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_SPECIAL )	/* OUT:coin lockout */
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_SPECIAL )	/* OUT:coin counter 1 */
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SPECIAL )	/* OUT:coin counter 2 */
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_LEFT  | IPF_8WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_RIGHT | IPF_8WAY )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICKRIGHT_LEFT | IPF_8WAY )
INPUT_PORTS_END

INPUT_PORTS_START( genpeitd )
	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* button 3 player 2 */
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BITX( 0x80, 0x80, IPT_SERVICE, "Service Switch", KEYCODE_F1, IP_JOY_NONE )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* button 3 player 1 */
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START      /* DSWA */
	PORT_DIPNAME( 0x03, 0x00, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 1C_2C ) )
	PORT_DIPNAME( 0x04, 0x00, "Freeze" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x04, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x00, "Allow Continue" )
	PORT_DIPSETTING(    0x08, DEF_STR( No ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x10, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x60, 0x00, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x60, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 1C_2C ) )
	PORT_SERVICE( 0x80, IP_ACTIVE_HIGH )

	PORT_START      /* DSWB */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x04, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x04, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	PORT_DIPNAME( 0x30, 0x00, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x10, "Easy" )
	PORT_DIPSETTING(    0x00, "Normal" )
	PORT_DIPSETTING(    0x20, "Hard" )
	PORT_DIPSETTING(    0x30, "Hardest" )
	PORT_DIPNAME( 0xc0, 0x00, "Candle" )
	PORT_DIPSETTING(    0x40, "40" )
	PORT_DIPSETTING(    0x00, "50" )
	PORT_DIPSETTING(    0x80, "60" )
	PORT_DIPSETTING(    0xc0, "70" )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_SPECIAL )	/* OUT:coin lockout */
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_SPECIAL )	/* OUT:coin counter 1 */
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SPECIAL )	/* OUT:coin counter 2 */
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_PLAYER2 )
INPUT_PORTS_END

INPUT_PORTS_START( rthunder )
	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* button 3 player 2 */
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_4WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_4WAY | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BITX( 0x80, 0x80, IPT_SERVICE, "Service Switch", KEYCODE_F1, IP_JOY_NONE )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* button 3 player 1 */
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_4WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_4WAY | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START      /* DSWA */
	PORT_DIPNAME( 0x03, 0x00, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 1C_2C ) )
	PORT_DIPNAME( 0x04, 0x00, "Freeze" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x04, DEF_STR( On ) )
	PORT_BITX(    0x08, 0x00, IPT_DIPSWITCH_NAME | IPF_CHEAT, "Invulnerability", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x60, 0x00, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x60, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 1C_2C ) )
	PORT_SERVICE( 0x80, IP_ACTIVE_HIGH )

	PORT_START      /* DSWB */
	PORT_DIPNAME( 0x01, 0x00, "Continues" )
	PORT_DIPSETTING(    0x01, "3" )
	PORT_DIPSETTING(    0x00, "6" )
	PORT_DIPNAME( 0x06, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, "Upright 1 Player" )
/*	PORT_DIPSETTING(    0x04, "Upright 1 Player" ) */
	PORT_DIPSETTING(    0x02, "Upright 2 Players" )
	PORT_DIPSETTING(    0x06, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x08, 0x08, "Level Select" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x00, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x00, "Normal" )
	PORT_DIPSETTING(    0x10, "Easy" )
	PORT_DIPNAME( 0x20, 0x20, "Timer value" )
	PORT_DIPSETTING(    0x00, "120 secs" )
	PORT_DIPSETTING(    0x20, "150 secs" )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x00, "70k, 200k" )
	PORT_DIPSETTING(    0x40, "100k, 300k" )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x80, "5" )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_SPECIAL )	/* OUT:coin lockout */
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_SPECIAL )	/* OUT:coin counter 1 */
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SPECIAL )	/* OUT:coin counter 2 */
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_4WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_4WAY | IPF_PLAYER2 )
INPUT_PORTS_END

INPUT_PORTS_START( rthundro )
	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* button 3 player 2 */
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_4WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_4WAY | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BITX( 0x80, 0x80, IPT_SERVICE, "Service Switch", KEYCODE_F1, IP_JOY_NONE )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* button 3 player 1 */
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_4WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_4WAY | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START      /* DSWA */
	PORT_DIPNAME( 0x03, 0x00, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 1C_2C ) )
	PORT_DIPNAME( 0x04, 0x00, "Freeze" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x04, DEF_STR( On ) )
	PORT_BITX(    0x08, 0x00, IPT_DIPSWITCH_NAME | IPF_CHEAT, "Invulnerability", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x60, 0x00, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x60, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 1C_2C ) )
	PORT_SERVICE( 0x80, IP_ACTIVE_HIGH )

	PORT_START      /* DSWB */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x04, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x04, DEF_STR( On ) )
	PORT_BITX(    0x08, 0x00, IPT_DIPSWITCH_NAME | IPF_CHEAT, "Level Select", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x10, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x20, DEF_STR( On ) )
	PORT_DIPNAME( 0xc0, 0x00, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x40, "1" )
	PORT_DIPSETTING(    0x80, "2" )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0xc0, "5" )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_SPECIAL )	/* OUT:coin lockout */
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_SPECIAL )	/* OUT:coin counter 1 */
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SPECIAL )	/* OUT:coin counter 2 */
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_4WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_4WAY | IPF_PLAYER2 )
INPUT_PORTS_END

INPUT_PORTS_START( wndrmomo )
	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* button 3 player 2 */
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_4WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_4WAY | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BITX( 0x80, 0x80, IPT_SERVICE, "Service Switch", KEYCODE_F1, IP_JOY_NONE )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* button 3 player 1 */
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_4WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_4WAY | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START      /* DSWA */
	PORT_DIPNAME( 0x03, 0x00, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 1C_2C ) )
	PORT_DIPNAME( 0x04, 0x00, "Freeze" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x04, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, "Level Select" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x60, 0x00, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x60, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 1C_2C ) )
	PORT_SERVICE( 0x80, IP_ACTIVE_HIGH )

	PORT_START      /* DSWB */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x06, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, "Type A" )
	PORT_DIPSETTING(    0x02, "Type B" )
	PORT_DIPSETTING(    0x04, "Type C" )
/*	PORT_DIPSETTING(    0x06, "Type A" )*/
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x10, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x20, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x80, DEF_STR( On ) )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_SPECIAL )	/* OUT:coin lockout */
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_SPECIAL )	/* OUT:coin counter 1 */
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SPECIAL )	/* OUT:coin counter 2 */
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_4WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_4WAY | IPF_PLAYER2 )
INPUT_PORTS_END


/*******************************************************************/

static struct GfxLayout tilelayout =
{
	8,8,
	RGN_FRAC(1,3),
	3,
	{ RGN_FRAC(2,3), RGN_FRAC(1,3), RGN_FRAC(0,3) },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8
};

#define SPRITELAYOUT(NUM) static struct GfxLayout spritelayout_##NUM =         \
{																			   \
	16,16,	/* 16*16 sprites */												   \
	NUM,	/* NUM sprites */												   \
	4,	/* 4 bits per pixel */												   \
	{ 0, 1, 2, 3 },															   \
	{ 0*4, 1*4, 2*4, 3*4, 4*4, 5*4, 6*4, 7*4,								   \
			8*4, 9*4, 10*4, 11*4, 12*4, 13*4, 14*4, 15*4 },					   \
	{ 0*64, 1*64, 2*64, 3*64, 4*64, 5*64, 6*64, 7*64,						   \
			8*64, 9*64, 10*64, 11*64, 12*64, 13*64, 14*64, 15*64 },			   \
	16*64																	   \
}

SPRITELAYOUT(256);
SPRITELAYOUT(512);
SPRITELAYOUT(1024);


#define GFXDECODE(SPRITE)													\
static struct GfxDecodeInfo gfxdecodeinfo_##SPRITE[] =						\
{																			\
	{ REGION_GFX1, 0x00000,      &tilelayout,            2048*0, 256 },		\
	{ REGION_GFX2, 0x00000,      &tilelayout,            2048*0, 256 },		\
	{ REGION_GFX3, 0*128*SPRITE, &spritelayout_##SPRITE, 2048*1, 128 },		\
	{ REGION_GFX3, 1*128*SPRITE, &spritelayout_##SPRITE, 2048*1, 128 },		\
	{ REGION_GFX3, 2*128*SPRITE, &spritelayout_##SPRITE, 2048*1, 128 },		\
	{ REGION_GFX3, 3*128*SPRITE, &spritelayout_##SPRITE, 2048*1, 128 },		\
	{ REGION_GFX3, 4*128*SPRITE, &spritelayout_##SPRITE, 2048*1, 128 },		\
	{ REGION_GFX3, 5*128*SPRITE, &spritelayout_##SPRITE, 2048*1, 128 },		\
	{ REGION_GFX3, 6*128*SPRITE, &spritelayout_##SPRITE, 2048*1, 128 },		\
	{ REGION_GFX3, 7*128*SPRITE, &spritelayout_##SPRITE, 2048*1, 128 },		\
	{ -1 }																	\
};

GFXDECODE( 256)
GFXDECODE( 512)
GFXDECODE(1024)

/*******************************************************************/

static struct YM2151interface ym2151_interface =
{
	1,                      /* 1 chip */
	3579580,                /* 3.579580 MHz ? */
	{ YM3012_VOL(0,MIXER_PAN_CENTER,60,MIXER_PAN_CENTER) },	/* only right channel is connected */
	{ 0 },
	{ 0 }
};

static struct namco_interface namco_interface =
{
	49152000/2048, 		/* 24000Hz */
	8,		/* number of voices */
	50,     /* playback volume */
	-1,		/* memory region */
	0		/* stereo */
};

static struct Samplesinterface samples_interface =
{
	2,	/* 2 channels for voice effects */
	40	/* volume */
};

static struct CustomSound_interface custom_interface =
{
	rt_decode_sample,
	0,
	0
};


static MACHINE_INIT( namco86 )
{
	unsigned char *base = memory_region(REGION_CPU1) + 0x10000;

	cpu_setbank(1,base);

	int_enabled[0] = int_enabled[1] = 1;
}


static MACHINE_DRIVER_START( hopmappy )

	/* basic machine hardware */
	MDRV_CPU_ADD_TAG("cpu1", M6809, 6000000/4)	/*49152000/32, rthunder doesn't work with this */
	MDRV_CPU_MEMORY(readmem1,writemem1)
	MDRV_CPU_VBLANK_INT(namco86_interrupt1,1)

	MDRV_CPU_ADD_TAG("cpu2", M6809, 49152000/32)
	MDRV_CPU_MEMORY(hopmappy_readmem2,hopmappy_writemem2)
	MDRV_CPU_VBLANK_INT(namco86_interrupt2,1)

	MDRV_CPU_ADD_TAG("mcu", HD63701, 49152000/32)	/* or compatible 6808 with extra instructions */
	MDRV_CPU_MEMORY(hopmappy_mcu_readmem,hopmappy_mcu_writemem)
	MDRV_CPU_PORTS(mcu_readport,mcu_writeport)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,1)	/* ??? */

	MDRV_FRAMES_PER_SECOND(60.606060)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(100)

	MDRV_MACHINE_INIT(namco86)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(36*8, 28*8)
	MDRV_VISIBLE_AREA(0*8, 36*8-1, 0*8, 28*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo_256)
	MDRV_PALETTE_LENGTH(512)
	MDRV_COLORTABLE_LENGTH(4096)

	MDRV_PALETTE_INIT(namcos86)
	MDRV_VIDEO_START(namcos86)
	MDRV_VIDEO_UPDATE(namcos86)

	/* sound hardware */
	MDRV_SOUND_ADD(YM2151, ym2151_interface)
	MDRV_SOUND_ADD(NAMCO_15XX, namco_interface)
	MDRV_SOUND_ADD(SAMPLES, samples_interface)
	MDRV_SOUND_ADD(CUSTOM, custom_interface)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( skykiddx )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(hopmappy)
	MDRV_CPU_MODIFY("cpu2")
	MDRV_CPU_MEMORY(skykiddx_readmem2,skykiddx_writemem2)

	MDRV_CPU_MODIFY("mcu")
	MDRV_CPU_MEMORY(skykiddx_mcu_readmem,skykiddx_mcu_writemem)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( roishtar )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(hopmappy)
	MDRV_CPU_MODIFY("cpu2")
	MDRV_CPU_MEMORY(roishtar_readmem2,roishtar_writemem2)

	MDRV_CPU_MODIFY("mcu")
	MDRV_CPU_MEMORY(roishtar_mcu_readmem,roishtar_mcu_writemem)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( genpeitd )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(hopmappy)
	MDRV_CPU_MODIFY("cpu2")
	MDRV_CPU_MEMORY(genpeitd_readmem2,genpeitd_writemem2)

	MDRV_CPU_MODIFY("mcu")
	MDRV_CPU_MEMORY(genpeitd_mcu_readmem,genpeitd_mcu_writemem)

	/* video hardware */
	MDRV_GFXDECODE(gfxdecodeinfo_1024)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( rthunder )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(hopmappy)
	MDRV_CPU_MODIFY("cpu2")
	MDRV_CPU_MEMORY(rthunder_readmem2,rthunder_writemem2)

	MDRV_CPU_MODIFY("mcu")
	MDRV_CPU_MEMORY(rthunder_mcu_readmem,rthunder_mcu_writemem)

	/* video hardware */
	MDRV_GFXDECODE(gfxdecodeinfo_512)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( wndrmomo )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(hopmappy)
	MDRV_CPU_MODIFY("cpu2")
	MDRV_CPU_MEMORY(wndrmomo_readmem2,wndrmomo_writemem2)

	MDRV_CPU_MODIFY("mcu")
	MDRV_CPU_MEMORY(wndrmomo_mcu_readmem,wndrmomo_mcu_writemem)

	/* video hardware */
	MDRV_GFXDECODE(gfxdecodeinfo_512)
MACHINE_DRIVER_END



/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( hopmappy )
	ROM_REGION( 0x18000, REGION_CPU1, 0 )
	ROM_LOAD( "hm1",         0x08000, 0x8000, CRC(1a83914e) SHA1(6cb96b2518f4b867e20bd5d31ac6913d09c95f06) )
	/* 9d empty */

	/* the CPU1 ROM expansion board is not present in this game */

	ROM_REGION( 0x18000, REGION_CPU2, 0 )
	ROM_LOAD( "hm2",         0xc000, 0x4000, CRC(c46cda65) SHA1(1131b4aa0a446569e1eb9f59964548058c7993e2) )
	/* 12d empty */

	ROM_REGION( 0x06000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "hm6",         0x00000, 0x04000, CRC(fd0e8887) SHA1(b76737d22bb1c1ae4d700ea6796e8d91f6ffa275) )	/* plane 1,2 */
	ROM_FILL(                0x04000, 0x02000, 0 )			/* no plane 3 */

	ROM_REGION( 0x06000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "hm5",         0x00000, 0x04000, CRC(9c4f31ae) SHA1(1c7072355d6f98b8e8554da19eab0512fdd9e2e1) )	/* plane 1,2 */
	ROM_FILL(                0x04000, 0x02000, 0 )			/* no plane 3 */

	ROM_REGION( 0x40000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "hm4",         0x00000, 0x8000, CRC(78719c52) SHA1(06d7bb9f29ccdbf563b3bf13c0290510b26e186f) )
	/* 12k/l/m/p/r/t/u empty */

	ROM_REGION( 0x1420, REGION_PROMS, 0 )
	ROM_LOAD( "hm11.bpr",    0x0000, 0x0200, CRC(cc801088) SHA1(d2c39ac1694d9b8c426e253702ecd096e68c6db9) )	/* red & green components */
	ROM_LOAD( "hm12.bpr",    0x0200, 0x0200, CRC(a1cb71c5) SHA1(d8c33c2e52d64ebf4a07d8a26453e7b872cae413) )	/* blue component */
	ROM_LOAD( "hm13.bpr",    0x0400, 0x0800, CRC(e362d613) SHA1(16d87711c1ac4ac2b649a32a5627cbd62cc5031f) )	/* tiles colortable */
	ROM_LOAD( "hm14.bpr",    0x0c00, 0x0800, CRC(678252b4) SHA1(9e2f7328532be3ac4b48bd5d52cd993108558452) )	/* sprites colortable */
	ROM_LOAD( "hm15.bpr",    0x1400, 0x0020, CRC(475bf500) SHA1(7e6a91e57d3709a5c70786c8e3ed545ee6026d03) )	/* tile address decoder (used at runtime) */

	ROM_REGION( 0x10000, REGION_CPU3, 0 )
	ROM_LOAD( "hm3",         0x08000, 0x2000, CRC(6496e1db) SHA1(f990fb3b2f93295282e8dee4488a4c3fc5ef83d1) )
	ROM_LOAD( "pl1-mcu.bin", 0x0f000, 0x1000, CRC(6ef08fb3) SHA1(4842590d60035a0059b0899eb2d5f58ae72c2529) )

	/* the PCM expansion board is not present in this game */
ROM_END

ROM_START( skykiddx )
	ROM_REGION( 0x18000, REGION_CPU1, 0 )
	ROM_LOAD( "sk3_1b.9c", 0x08000, 0x8000, CRC(767b3514) SHA1(7b85e520e56924235d1f4987333f183c914fafc1) )
	ROM_LOAD( "sk3_2.9d",  0x10000, 0x8000, CRC(74b8f8e2) SHA1(0c9f0a283c764d5db59abea17a7f3285718b4501) )

	/* the CPU1 ROM expansion board is not present in this game */

	ROM_REGION( 0x18000, REGION_CPU2, 0 )
	ROM_LOAD( "sk3_3.12c", 0x8000, 0x8000, CRC(6d1084c4) SHA1(0045e01cbeb750c50a561420f1577de8cd881894) )
	/* 12d empty */

	ROM_REGION( 0x0c000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "sk3_9.7r",  0x00000, 0x08000, CRC(48675b17) SHA1(434babcf5454364a17e529daae16e6f623ca75dd) )	/* plane 1,2 */
	ROM_LOAD( "sk3_10.7s", 0x08000, 0x04000, CRC(7418465a) SHA1(e8236c3d077af147a7d5f8f9cd519d030c073aaf) )	/* plane 3 */

	ROM_REGION( 0x0c000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "sk3_7.4r",  0x00000, 0x08000, CRC(4036b735) SHA1(4177f3f37feb83fab63a1160a939c8d566bbe16c) )	/* plane 1,2 */
	ROM_LOAD( "sk3_8.4s",  0x08000, 0x04000, CRC(044bfd21) SHA1(4fbb72fbf041cb256377952d860147376fc1d05b) )	/* plane 3 */

	ROM_REGION( 0x40000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "sk3_5.12h",  0x00000, 0x8000, CRC(5c7d4399) SHA1(9c57e2510b1a01f618364ddaa9b9fa0ce9ae7340) )
	ROM_LOAD( "sk3_6.12k",  0x08000, 0x8000, CRC(c908a3b2) SHA1(5fd5304c314443fb3351e7a2d50a72a0fede7e6d) )
	/* 12l/m/p/r/t/u empty */

	ROM_REGION( 0x1420, REGION_PROMS, 0 )
	ROM_LOAD( "sk3-1.3r", 0x0000, 0x0200, CRC(9e81dedd) SHA1(9d2ddf51788d22ed65db9070684e586b2f64f99e) )	/* red & green components */
	ROM_LOAD( "sk3-2.3s", 0x0200, 0x0200, CRC(cbfec4dd) SHA1(98adf5db270a853ab2a2e1cdd9edfd5657287a96) )	/* blue component */
	ROM_LOAD( "sk3-3.4v", 0x0400, 0x0800, CRC(81714109) SHA1(577e513369a4368b7dd29dff80904eb0ac2004ff) )	/* tiles colortable */
	ROM_LOAD( "sk3-4.5v", 0x0c00, 0x0800, CRC(1bf25acc) SHA1(a8db254ba4cbb85efc232a5bf9b268534455ad4a) )	/* sprites colortable */
	ROM_LOAD( "sk3-5.6u", 0x1400, 0x0020, CRC(e4130804) SHA1(e1a3e1383186d036fba6dc8a8681f48f24f59281) )	/* tile address decoder (used at runtime) */

	ROM_REGION( 0x10000, REGION_CPU3, 0 )
	ROM_LOAD( "sk3_4.6b",    0x08000, 0x4000, CRC(e6cae2d6) SHA1(b6598aaee0136b0980e13326cb2835aadadd9543) )
	ROM_LOAD( "rt1-mcu.bin", 0x0f000, 0x1000, CRC(6ef08fb3) SHA1(4842590d60035a0059b0899eb2d5f58ae72c2529) )

	/* the PCM expansion board is not present in this game */
ROM_END

ROM_START( skykiddo )
	ROM_REGION( 0x18000, REGION_CPU1, 0 )
	ROM_LOAD( "sk3-1.9c",  0x08000, 0x8000, CRC(5722a291) SHA1(0b3ca2585bf5c18214c1337dce8f92027e9d78c2) )
	ROM_LOAD( "sk3_2.9d",  0x10000, 0x8000, CRC(74b8f8e2) SHA1(0c9f0a283c764d5db59abea17a7f3285718b4501) )

	/* the CPU1 ROM expansion board is not present in this game */

	ROM_REGION( 0x18000, REGION_CPU2, 0 )
	ROM_LOAD( "sk3_3.12c", 0x8000, 0x8000, CRC(6d1084c4) SHA1(0045e01cbeb750c50a561420f1577de8cd881894) )
	/* 12d empty */

	ROM_REGION( 0x0c000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "sk3_9.7r",  0x00000, 0x08000, CRC(48675b17) SHA1(434babcf5454364a17e529daae16e6f623ca75dd) )	/* plane 1,2 */
	ROM_LOAD( "sk3_10.7s", 0x08000, 0x04000, CRC(7418465a) SHA1(e8236c3d077af147a7d5f8f9cd519d030c073aaf) )	/* plane 3 */

	ROM_REGION( 0x0c000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "sk3_7.4r",  0x00000, 0x08000, CRC(4036b735) SHA1(4177f3f37feb83fab63a1160a939c8d566bbe16c) )	/* plane 1,2 */
	ROM_LOAD( "sk3_8.4s",  0x08000, 0x04000, CRC(044bfd21) SHA1(4fbb72fbf041cb256377952d860147376fc1d05b) )	/* plane 3 */

	ROM_REGION( 0x40000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "sk3_5.12h",  0x00000, 0x8000, CRC(5c7d4399) SHA1(9c57e2510b1a01f618364ddaa9b9fa0ce9ae7340) )
	ROM_LOAD( "sk3_6.12k",  0x08000, 0x8000, CRC(c908a3b2) SHA1(5fd5304c314443fb3351e7a2d50a72a0fede7e6d) )
	/* 12l/m/p/r/t/u empty */

	ROM_REGION( 0x1420, REGION_PROMS, 0 )
	ROM_LOAD( "sk3-1.3r", 0x0000, 0x0200, CRC(9e81dedd) SHA1(9d2ddf51788d22ed65db9070684e586b2f64f99e) )	/* red & green components */
	ROM_LOAD( "sk3-2.3s", 0x0200, 0x0200, CRC(cbfec4dd) SHA1(98adf5db270a853ab2a2e1cdd9edfd5657287a96) )	/* blue component */
	ROM_LOAD( "sk3-3.4v", 0x0400, 0x0800, CRC(81714109) SHA1(577e513369a4368b7dd29dff80904eb0ac2004ff) )	/* tiles colortable */
	ROM_LOAD( "sk3-4.5v", 0x0c00, 0x0800, CRC(1bf25acc) SHA1(a8db254ba4cbb85efc232a5bf9b268534455ad4a) )	/* sprites colortable */
	ROM_LOAD( "sk3-5.6u", 0x1400, 0x0020, CRC(e4130804) SHA1(e1a3e1383186d036fba6dc8a8681f48f24f59281) )	/* tile address decoder (used at runtime) */

	ROM_REGION( 0x10000, REGION_CPU3, 0 )
	ROM_LOAD( "sk3_4.6b",    0x08000, 0x4000, CRC(e6cae2d6) SHA1(b6598aaee0136b0980e13326cb2835aadadd9543) )
	ROM_LOAD( "rt1-mcu.bin", 0x0f000, 0x1000, CRC(6ef08fb3) SHA1(4842590d60035a0059b0899eb2d5f58ae72c2529) )

	/* the PCM expansion board is not present in this game */
ROM_END

ROM_START( roishtar )
	ROM_REGION( 0x18000, REGION_CPU1, 0 )
	ROM_LOAD( "ri1-1c.9c", 0x08000, 0x8000, CRC(14acbacb) SHA1(3c6130f9e5a4ba84be0cc3547c1086707ee3b8e9) )
	ROM_LOAD( "ri1-2.9d",  0x14000, 0x2000, CRC(fcd58d91) SHA1(e7b6d7afd7cf6c374ee90d6499ea0f205e742b21) )

	/* the CPU1 ROM expansion board is not present in this game */

	ROM_REGION( 0x18000, REGION_CPU2, 0 )
	ROM_LOAD( "ri1-3.12c", 0x8000, 0x8000, CRC(a39829f7) SHA1(e08114d5154367a3cc36f1485253f18044a1888d) )
	/* 12d empty */

	ROM_REGION( 0x06000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "ri1-14.7r", 0x00000, 0x04000, CRC(de8154b4) SHA1(70a65e4656cf9fcf5c54e84c628ec95393e856fb) )	/* plane 1,2 */
	ROM_LOAD( "ri1-15.7s", 0x04000, 0x02000, CRC(4298822b) SHA1(5aad41fd719c2f310ae485caaacda129c9f2ac94) )	/* plane 3 */

	ROM_REGION( 0x06000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "ri1-12.4r", 0x00000, 0x04000, CRC(557e54d3) SHA1(d22969deefcb3c3443d08a215f1ec2e874650b19) )	/* plane 1,2 */
	ROM_LOAD( "ri1-13.4s", 0x04000, 0x02000, CRC(9ebe8e32) SHA1(5990a86bfbf2669e512e8ca875c69b4c60c4d108) )	/* plane 3 */

	ROM_REGION( 0x40000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "ri1-5.12h",  0x00000, 0x8000, CRC(46b59239) SHA1(bb08e57cd5864f41e27a07dcf449896570d2203d) )
	ROM_LOAD( "ri1-6.12k",  0x08000, 0x8000, CRC(94d9ef48) SHA1(a13b345b8fe30dea8e85698782674859c385e79a) )
	ROM_LOAD( "ri1-7.12l",  0x10000, 0x8000, CRC(da802b59) SHA1(b6551db5cd9c3d674cdf1dc59f581ee435a7eeb7) )
	ROM_LOAD( "ri1-8.12m",  0x18000, 0x8000, CRC(16b88b74) SHA1(9ef3ebf686a539e911bb7a3a4c02d0e2dca616d0) )
	ROM_LOAD( "ri1-9.12p",  0x20000, 0x8000, CRC(f3de3c2a) SHA1(02705bfd37f8996c5fc9c5bf2a99e859083a75e6) )
	ROM_LOAD( "ri1-10.12r", 0x28000, 0x8000, CRC(6dacc70d) SHA1(c7db40a0e90c9717f8a2f1507daff997856a3b91) )
	ROM_LOAD( "ri1-11.12t", 0x30000, 0x8000, CRC(fb6bc533) SHA1(a840af58d6db66518520bc7d88867a09a2e502c2) )
	/* 12u empty */

	ROM_REGION( 0x1420, REGION_PROMS, 0 )
	ROM_LOAD( "ri1-1.3r", 0x0000, 0x0200, CRC(29cd0400) SHA1(a9b0d09492710e72e34155cd6a7b7c1a34c56b20) )	/* red & green components */
	ROM_LOAD( "ri1-2.3s", 0x0200, 0x0200, CRC(02fd278d) SHA1(db104fc7acf2739def902180981eb7ba10ec3dda) )	/* blue component */
	ROM_LOAD( "ri1-3.4v", 0x0400, 0x0800, CRC(cbd7e53f) SHA1(77ef70be4e8a21948d697649352a5e3527086cf2) )	/* tiles colortable */
	ROM_LOAD( "ri1-4.5v", 0x0c00, 0x0800, CRC(22921617) SHA1(7304cb5a86f524f912feb8b58801393cce5d3b09) )	/* sprites colortable */
	ROM_LOAD( "ri1-5.6u", 0x1400, 0x0020, CRC(e2188075) SHA1(be079ace2070433d4d90c757aef3e415b4e21455) )	/* tile address decoder (used at runtime) */

	ROM_REGION( 0x10000, REGION_CPU3, 0 )
	ROM_LOAD( "ri1-4.6b",    0x00000, 0x4000, CRC(552172b8) SHA1(18b35cb116baba362831fc046241895198b07a53) )
	ROM_CONTINUE(            0x08000, 0x4000 )
	ROM_LOAD( "rt1-mcu.bin", 0x0f000, 0x1000, CRC(6ef08fb3) SHA1(4842590d60035a0059b0899eb2d5f58ae72c2529) )

	/* the PCM expansion board is not present in this game */
ROM_END

ROM_START( genpeitd )
	ROM_REGION( 0x18000, REGION_CPU1, 0 )
	ROM_LOAD( "gt1-1b.9c", 0x08000, 0x8000, CRC(75396194) SHA1(2a526064fb91b2796c913f3050867352ac63e643) )
	/* 9d empty */

	ROM_REGION( 0x40000, REGION_USER1, 0 ) /* bank switched data for CPU1 */
	ROM_LOAD( "gt1-10b.f1",  0x00000, 0x10000, CRC(5721ad0d) SHA1(f16afb3f468957a9de270366605592e14837b8c2) )
	/* h1 empty */
	/* k1 empty */
	/* m1 empty */

	ROM_REGION( 0x18000, REGION_CPU2, 0 )
	ROM_LOAD( "gt1-2.12c", 0xc000, 0x4000, CRC(302f2cb6) SHA1(19c39afb7d49d80aeaaf67a837cd02bfd3d64fbd) )
	/* 12d empty */

	ROM_REGION( 0x18000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "gt1-7.7r", 0x00000, 0x10000, CRC(ea77a211) SHA1(32b8ae11723b6223b42225805acd0dcab65516a5) )	/* plane 1,2 */
	ROM_LOAD( "gt1-6.7s", 0x10000, 0x08000, CRC(1b128a2e) SHA1(6d7b95326919420538b509a119c26e9109e5539e) )	/* plane 3 */

	ROM_REGION( 0x0c000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "gt1-5.4r", 0x00000, 0x08000, CRC(44d58b06) SHA1(9663f026092484a4041e486bad23e8e58a4dbf95) )	/* plane 1,2 */
	ROM_LOAD( "gt1-4.4s", 0x08000, 0x04000, CRC(db8d45b0) SHA1(fd4ebdf442e8b9ccc026079c29a975b1fa6e8dd6) )	/* plane 3 */

	ROM_REGION( 0x100000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "gt1-11.12h",  0x00000, 0x20000, CRC(3181a5fe) SHA1(a98b8609afe3a41ed7b1432b3c2850e8de2c428b) )
	ROM_LOAD( "gt1-12.12k",  0x20000, 0x20000, CRC(76b729ab) SHA1(d75aeca1ddbb690ff7442dee3b1d44331d220758) )
	ROM_LOAD( "gt1-13.12l",  0x40000, 0x20000, CRC(e332a36e) SHA1(fa06da1e4f7ef3adf8e87d8d4d95aa7e0eb2d7b2) )
	ROM_LOAD( "gt1-14.12m",  0x60000, 0x20000, CRC(e5ffaef5) SHA1(0db1fd0b7553f69a480fcf2d312c6a8cd99ed777) )
	ROM_LOAD( "gt1-15.12p",  0x80000, 0x20000, CRC(198b6878) SHA1(3f42a80199192412171445dd4fe5c4a3a19a5672) )
	ROM_LOAD( "gt1-16.12r",  0xa0000, 0x20000, CRC(801e29c7) SHA1(4f1dd17f04f56153cd090887e841e3baa1287755) )
	ROM_LOAD( "gt1-8.12t",   0xc0000, 0x10000, CRC(ad7bc770) SHA1(babce324189b9380e1a71b63499362e276c99ea8) )
	ROM_LOAD( "gt1-9.12u",   0xe0000, 0x10000, CRC(d95a5fd7) SHA1(819ac376ac0eb6ffa69153d579a9c11ae5feb6a4) )

	ROM_REGION( 0x1420, REGION_PROMS, 0 )
	ROM_LOAD( "gt1-1.3r", 0x0000, 0x0200, CRC(2f0ddddb) SHA1(27fa45c0baf9a48002db11be9b3c0472ecfd986c) )	/* red & green components */
	ROM_LOAD( "gt1-2.3s", 0x0200, 0x0200, CRC(87d27025) SHA1(a50f969d48a99c6d29141458fb3e34b23cf5e67c) )	/* blue component */
	ROM_LOAD( "gt1-3.4v", 0x0400, 0x0800, CRC(c178de99) SHA1(67289ef9e5068636023316560f9f1690a8384bfb) )	/* tiles colortable */
	ROM_LOAD( "gt1-4.5v", 0x0c00, 0x0800, CRC(9f48ef17) SHA1(78c813dd57326f3f5ab785005ef89ba96303adeb) )	/* sprites colortable */
	ROM_LOAD( "gt1-5.6u", 0x1400, 0x0020, CRC(e4130804) SHA1(e1a3e1383186d036fba6dc8a8681f48f24f59281) )	/* tile address decoder (used at runtime) */

	ROM_REGION( 0x10000, REGION_CPU3, 0 )
	ROM_LOAD( "gt1-3.6b",    0x04000, 0x8000, CRC(315cd988) SHA1(87b1a90b2a53571f7d8f9a475125f3f31ed3cb5d) )
	ROM_LOAD( "rt1-mcu.bin", 0x0f000, 0x1000, CRC(6ef08fb3) SHA1(4842590d60035a0059b0899eb2d5f58ae72c2529) )

	ROM_REGION( 0x80000, REGION_SOUND1, 0 ) /* PCM samples for Hitachi CPU */
	ROM_LOAD( "gt1-17.f3",  0x00000, 0x20000, CRC(26181ff8) SHA1(c97a0e6282b2af88d960c71e3af5283608493d31) )
	ROM_LOAD( "gt1-18.h3",  0x20000, 0x20000, CRC(7ef9e5ea) SHA1(0464d43b39903ce373d70854bbcd5da05896ecae) )
	ROM_LOAD( "gt1-19.k3",  0x40000, 0x20000, CRC(38e11f6c) SHA1(52dea9d444d5a0421db4a8bf5c79a9d901b6f005) )
	/* m3 empty */
ROM_END

ROM_START( rthunder )
	ROM_REGION( 0x18000, REGION_CPU1, 0 )
	ROM_LOAD( "rt3-1b.9c",  0x8000, 0x8000, CRC(7d252a1b) SHA1(cb92709e94eb273b3ce44c55cd252170ad1017f4) )
	/* 9d empty */

	ROM_REGION( 0x40000, REGION_USER1, 0 ) /* bank switched data for CPU1 */
	ROM_LOAD( "rt1-17.f1",  0x00000, 0x10000, CRC(766af455) SHA1(8c71772795e783d6c4b88af9a311d55e363c298a) )
	ROM_LOAD( "rt1-18.h1",  0x10000, 0x10000, CRC(3f9f2f5d) SHA1(541b8f80800cb55e4b81ac48771d00fe10c90743) )
	ROM_LOAD( "rt1-19.k1",  0x20000, 0x10000, CRC(c16675e9) SHA1(e31c28cb95ffa85392c74e1d81bfa89acbaefeb9) )
	ROM_LOAD( "rt1-20.m1",  0x30000, 0x10000, CRC(c470681b) SHA1(87f8d8509c3e8207f34c6001aaf4d0afdad82d0d) )

	ROM_REGION( 0x18000, REGION_CPU2, 0 )
	ROM_LOAD( "rt3-2b.12c", 0x08000, 0x8000, CRC(a7ea46ee) SHA1(52e8757aacb4e01f8432125729e2323c48ebc4f5) )
	ROM_LOAD( "rt3-3.12d",  0x10000, 0x8000, CRC(a13f601c) SHA1(8987174e364d20eeab706c3e0d4e0d3c2b96723c) )

	ROM_REGION( 0x18000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "rt1-7.7r",  0x00000, 0x10000, CRC(a85efa39) SHA1(1ed63b421a93960668cb4558c1ca1b3c86b1f6be) )	/* plane 1,2 */
	ROM_LOAD( "rt1-8.7s",  0x10000, 0x08000, CRC(f7a95820) SHA1(82fe0adf6c5b3abef19031646e1eca1585dcc481) )	/* plane 3 */

	ROM_REGION( 0x0c000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "rt1-5.4r",  0x00000, 0x08000, CRC(d0fc470b) SHA1(70f7f1e29527044eae405f58af08bad3097990bd) )	/* plane 1,2 */
	ROM_LOAD( "rt1-6.4s",  0x08000, 0x04000, CRC(6b57edb2) SHA1(4a8f1e024e5be4d76f2c99d506ae7da86af3d1f5) )	/* plane 3 */

	ROM_REGION( 0x80000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "rt1-9.12h",  0x00000, 0x10000, CRC(8e070561) SHA1(483b4de79f2429236f45c32ec56b97a9a90574a3) )
	ROM_LOAD( "rt1-10.12k", 0x10000, 0x10000, CRC(cb8fb607) SHA1(ba9400fb19d29a285897cc3a2d4d739ce845f897) )
	ROM_LOAD( "rt1-11.12l", 0x20000, 0x10000, CRC(2bdf5ed9) SHA1(a771e922ad868ca1e008d08a8ff5fdf28aa315fc) )
	ROM_LOAD( "rt1-12.12m", 0x30000, 0x10000, CRC(e6c6c7dc) SHA1(ead143c2730a77911839a25734550188533c7b96) )
	ROM_LOAD( "rt1-13.12p", 0x40000, 0x10000, CRC(489686d7) SHA1(a04b57424acbf2584f736b55740d613a1aae2b8b) )
	ROM_LOAD( "rt1-14.12r", 0x50000, 0x10000, CRC(689e56a8) SHA1(b4d6de4eec47856a62f396f55d531fbf345cf12a) )
	ROM_LOAD( "rt1-15.12t", 0x60000, 0x10000, CRC(1d8bf2ca) SHA1(949ae8b00b94bfa5bc2d07888aafbaaaea559b06) )
	ROM_LOAD( "rt1-16.12u", 0x70000, 0x10000, CRC(1bbcf37b) SHA1(8d27c49b36d5e23dd446c150ada3853eec75e4c1) )

	ROM_REGION( 0x1420, REGION_PROMS, 0 )
	ROM_LOAD( "mb7124e.3r", 0x0000, 0x0200, CRC(8ef3bb9d) SHA1(4636d6b8ba7611b11d4863fab02475dc4a619eaf) )	/* red & green components */
	ROM_LOAD( "mb7116e.3s", 0x0200, 0x0200, CRC(6510a8f2) SHA1(935f140bfa7e6f8cebafa7f1b0de99dd319273d4) )	/* blue component */
	ROM_LOAD( "mb7138h.4v", 0x0400, 0x0800, CRC(95c7d944) SHA1(ca5fea028674882a61507ac7c89ada96f5b2674d) )	/* tiles colortable */
	ROM_LOAD( "mb7138h.6v", 0x0c00, 0x0800, CRC(1391fec9) SHA1(8ca94e22110b20d2ecdf03610bcc89ff4245920f) )	/* sprites colortable */
	ROM_LOAD( "mb7112e.6u", 0x1400, 0x0020, CRC(e4130804) SHA1(e1a3e1383186d036fba6dc8a8681f48f24f59281) )	/* tile address decoder (used at runtime) */

	ROM_REGION( 0x10000, REGION_CPU3, 0 )
	ROM_LOAD( "rt1-4.6b",    0x04000, 0x8000, CRC(00cf293f) SHA1(bc441d21bb4c54a01d2393fbe99201714cd4439d) )
	ROM_LOAD( "rt1-mcu.bin", 0x0f000, 0x1000, CRC(6ef08fb3) SHA1(4842590d60035a0059b0899eb2d5f58ae72c2529) )

	ROM_REGION( 0x40000, REGION_SOUND1, 0 ) /* PCM samples for Hitachi CPU */
	ROM_LOAD( "rt1-21.f3",  0x00000, 0x10000, CRC(454968f3) SHA1(e0a679353491190b6d4f0355324456a1bd7c8a7a) )
	ROM_LOAD( "rt1-22.h3",  0x10000, 0x10000, CRC(fe963e72) SHA1(4c9ce4e4c8e756a743c541f670a6741b520125e3) )
	/* k3 empty */
	/* m3 empty */
ROM_END

ROM_START( rthundro )
	ROM_REGION( 0x18000, REGION_CPU1, 0 )
	ROM_LOAD( "r1",         0x8000, 0x8000, CRC(6f8c1252) SHA1(586f2e33dd16f31131e4ae9423d639fdc6555c9c) )
	/* 9d empty */

	ROM_REGION( 0x40000, REGION_USER1, 0 ) /* bank switched data for CPU1 */
	ROM_LOAD( "rt1-17.f1",  0x00000, 0x10000, CRC(766af455) SHA1(8c71772795e783d6c4b88af9a311d55e363c298a) )
	ROM_LOAD( "rt1-18.h1",  0x10000, 0x10000, CRC(3f9f2f5d) SHA1(541b8f80800cb55e4b81ac48771d00fe10c90743) )
	ROM_LOAD( "r19",        0x20000, 0x10000, CRC(fe9343b0) SHA1(ae8e5ee11eaf7dc1e8f814b0a0beef97731f042b) )
	ROM_LOAD( "r20",        0x30000, 0x10000, CRC(f8518d4f) SHA1(3a8551d46ffdf82844b2eb1b2c01cf2d8423a49e) )

	ROM_REGION( 0x18000, REGION_CPU2, 0 )
	ROM_LOAD( "r2",        0x08000, 0x8000, CRC(f22a03d8) SHA1(5b81fc82813978d5cb69402be72b9ccc585fa1d0) )
	ROM_LOAD( "r3",        0x10000, 0x8000, CRC(aaa82885) SHA1(fc2bec3cf7e2de5f90174a2ed3bacfa94b6819f4) )

	ROM_REGION( 0x18000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "rt1-7.7r",  0x00000, 0x10000, CRC(a85efa39) SHA1(1ed63b421a93960668cb4558c1ca1b3c86b1f6be) )	/* plane 1,2 */
	ROM_LOAD( "rt1-8.7s",  0x10000, 0x08000, CRC(f7a95820) SHA1(82fe0adf6c5b3abef19031646e1eca1585dcc481) )	/* plane 3 */

	ROM_REGION( 0x0c000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "rt1-5.4r",  0x00000, 0x08000, CRC(d0fc470b) SHA1(70f7f1e29527044eae405f58af08bad3097990bd) )	/* plane 1,2 */
	ROM_LOAD( "rt1-6.4s",  0x08000, 0x04000, CRC(6b57edb2) SHA1(4a8f1e024e5be4d76f2c99d506ae7da86af3d1f5) )	/* plane 3 */

	ROM_REGION( 0x80000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "rt1-9.12h",  0x00000, 0x10000, CRC(8e070561) SHA1(483b4de79f2429236f45c32ec56b97a9a90574a3) )
	ROM_LOAD( "rt1-10.12k", 0x10000, 0x10000, CRC(cb8fb607) SHA1(ba9400fb19d29a285897cc3a2d4d739ce845f897) )
	ROM_LOAD( "rt1-11.12l", 0x20000, 0x10000, CRC(2bdf5ed9) SHA1(a771e922ad868ca1e008d08a8ff5fdf28aa315fc) )
	ROM_LOAD( "rt1-12.12m", 0x30000, 0x10000, CRC(e6c6c7dc) SHA1(ead143c2730a77911839a25734550188533c7b96) )
	ROM_LOAD( "rt1-13.12p", 0x40000, 0x10000, CRC(489686d7) SHA1(a04b57424acbf2584f736b55740d613a1aae2b8b) )
	ROM_LOAD( "rt1-14.12r", 0x50000, 0x10000, CRC(689e56a8) SHA1(b4d6de4eec47856a62f396f55d531fbf345cf12a) )
	ROM_LOAD( "rt1-15.12t", 0x60000, 0x10000, CRC(1d8bf2ca) SHA1(949ae8b00b94bfa5bc2d07888aafbaaaea559b06) )
	ROM_LOAD( "rt1-16.12u", 0x70000, 0x10000, CRC(1bbcf37b) SHA1(8d27c49b36d5e23dd446c150ada3853eec75e4c1) )

	ROM_REGION( 0x1420, REGION_PROMS, 0 )
	ROM_LOAD( "mb7124e.3r", 0x0000, 0x0200, CRC(8ef3bb9d) SHA1(4636d6b8ba7611b11d4863fab02475dc4a619eaf) )	/* red & green components */
	ROM_LOAD( "mb7116e.3s", 0x0200, 0x0200, CRC(6510a8f2) SHA1(935f140bfa7e6f8cebafa7f1b0de99dd319273d4) )	/* blue component */
	ROM_LOAD( "mb7138h.4v", 0x0400, 0x0800, CRC(95c7d944) SHA1(ca5fea028674882a61507ac7c89ada96f5b2674d) )	/* tiles colortable */
	ROM_LOAD( "mb7138h.6v", 0x0c00, 0x0800, CRC(1391fec9) SHA1(8ca94e22110b20d2ecdf03610bcc89ff4245920f) )	/* sprites colortable */
	ROM_LOAD( "mb7112e.6u", 0x1400, 0x0020, CRC(e4130804) SHA1(e1a3e1383186d036fba6dc8a8681f48f24f59281) )	/* tile address decoder (used at runtime) */

	ROM_REGION( 0x10000, REGION_CPU3, 0 )
	ROM_LOAD( "r4",          0x04000, 0x8000, CRC(0387464f) SHA1(ce7f521bc2ecc6525880da2551daf595a394a275) )
	ROM_LOAD( "rt1-mcu.bin", 0x0f000, 0x1000, CRC(6ef08fb3) SHA1(4842590d60035a0059b0899eb2d5f58ae72c2529) )

	ROM_REGION( 0x40000, REGION_SOUND1, 0 ) /* PCM samples for Hitachi CPU */
	ROM_LOAD( "rt1-21.f3",  0x00000, 0x10000, CRC(454968f3) SHA1(e0a679353491190b6d4f0355324456a1bd7c8a7a) )
	ROM_LOAD( "rt1-22.h3",  0x10000, 0x10000, CRC(fe963e72) SHA1(4c9ce4e4c8e756a743c541f670a6741b520125e3) )
	/* k3 empty */
	/* m3 empty */
ROM_END

ROM_START( wndrmomo )
	ROM_REGION( 0x18000, REGION_CPU1, 0 )
	ROM_LOAD( "wm1-1.9c", 0x8000, 0x8000, CRC(34b50bf0) SHA1(112c8c8a0a16382008cacd2e484f91fa9338d10a) )
	/* 9d empty */

	ROM_REGION( 0x40000, REGION_USER1, 0 ) /* bank switched data for CPU1 */
	ROM_LOAD( "wm1-16.f1", 0x00000, 0x10000, CRC(e565f8f3) SHA1(e1f417003ef9f700f9d5ed091484463c704c8b9f) )
	/* h1 empty */
	/* k1 empty */
	/* m1 empty */

	ROM_REGION( 0x18000, REGION_CPU2, 0 )
	ROM_LOAD( "wm1-2.12c", 0x8000, 0x8000, CRC(3181efd0) SHA1(01a2e0e4c8ced6f48b6e70393a3c4152b079e9b0) )
	/* 12d empty */

	ROM_REGION( 0x0c000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "wm1-6.7r", 0x00000, 0x08000, CRC(93955fbb) SHA1(cffd457886c40bf709b573237165ae8fa9784e32) )	/* plane 1,2 */
	ROM_LOAD( "wm1-7.7s", 0x08000, 0x04000, CRC(7d662527) SHA1(09d1dc46a402c67dddcdd4cc90f32948c7a28795) )	/* plane 3 */

	ROM_REGION( 0x0c000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "wm1-4.4r", 0x00000, 0x08000, CRC(bbe67836) SHA1(bc998c2ddc2664db614e7c487f77073a5be69e89) )	/* plane 1,2 */
	ROM_LOAD( "wm1-5.4s", 0x08000, 0x04000, CRC(a81b481f) SHA1(b5a029e432b29e157505b975ea57cd4b5da361a7) )	/* plane 3 */

	ROM_REGION( 0x80000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "wm1-8.12h",  0x00000, 0x10000, CRC(14f52e72) SHA1(0f8f58cd13e3393a113817593816f53a218f3ce4) )
	ROM_LOAD( "wm1-9.12k",  0x10000, 0x10000, CRC(16f8cdae) SHA1(8281b4c66157580f34aec7c035d06f721f77b3d5) )
	ROM_LOAD( "wm1-10.12l", 0x20000, 0x10000, CRC(bfbc1896) SHA1(0308cf907c77417ad3f84326b074567a00245998) )
	ROM_LOAD( "wm1-11.12m", 0x30000, 0x10000, CRC(d775ddb2) SHA1(8f1a6efbdaeec9049624be56078e843b3094a277) )
	ROM_LOAD( "wm1-12.12p", 0x40000, 0x10000, CRC(de64c12f) SHA1(c867e03d6b249ce0c9b3554797ccebaeb2778f73) )
	ROM_LOAD( "wm1-13.12r", 0x50000, 0x10000, CRC(cfe589ad) SHA1(3289cd1571fefe5266ca1e8bd75069d386919ae5) )
	ROM_LOAD( "wm1-14.12t", 0x60000, 0x10000, CRC(2ae21a53) SHA1(ef3e5e4fa8580f7891d9e1825d2eda4c387db732) )
	ROM_LOAD( "wm1-15.12u", 0x70000, 0x10000, CRC(b5c98be0) SHA1(bdd6e0e02632866eea60a6bdeff3af8b6cd08d68) )

	ROM_REGION( 0x1420, REGION_PROMS, 0 )
	ROM_LOAD( "wm1-1.3r", 0x0000, 0x0200, CRC(1af8ade8) SHA1(1aa0d314c34abc4154092d4b588214afb0b21e22) )	/* red & green components */
	ROM_LOAD( "wm1-2.3s", 0x0200, 0x0200, CRC(8694e213) SHA1(f00d692e587c3706e71b6eeef21e1ea87c9dd921) )	/* blue component */
	ROM_LOAD( "wm1-3.4v", 0x0400, 0x0800, CRC(2ffaf9a4) SHA1(2002df3cc38e05f3e127d05c244cb101d1f1d85f) )	/* tiles colortable */
	ROM_LOAD( "wm1-4.5v", 0x0c00, 0x0800, CRC(f4e83e0b) SHA1(b000d884c6e0373b0403bc9d63eb0452c1197491) )	/* sprites colortable */
	ROM_LOAD( "wm1-5.6u", 0x1400, 0x0020, CRC(e4130804) SHA1(e1a3e1383186d036fba6dc8a8681f48f24f59281) )	/* tile address decoder (used at runtime) */

	ROM_REGION( 0x10000, REGION_CPU3, 0 )
	ROM_LOAD( "wm1-3.6b",    0x04000, 0x8000, CRC(55f01df7) SHA1(c11574a8b51bf965790b97895452e9fa9ab6b752) )
	ROM_LOAD( "rt1-mcu.bin", 0x0f000, 0x1000, CRC(6ef08fb3) SHA1(4842590d60035a0059b0899eb2d5f58ae72c2529) )

	ROM_REGION( 0x40000, REGION_SOUND1, 0 ) /* PCM samples for Hitachi CPU */
	ROM_LOAD( "wm1-17.f3", 0x00000, 0x10000, CRC(bea3c318) SHA1(50b6e4c546ce056d68fbb2e52cb88397daa615aa) )
	ROM_LOAD( "wm1-18.h3", 0x10000, 0x10000, CRC(6d73bcc5) SHA1(fccd83bb3a872b4cd8bd9f11f5cdf5926cb9b74e) )
	ROM_LOAD( "wm1-19.k3", 0x20000, 0x10000, CRC(d288e912) SHA1(d0db58d7ed3d8cf895e031901f91f810f0e18709) )
	ROM_LOAD( "wm1-20.m3", 0x30000, 0x10000, CRC(076a72cb) SHA1(0d0d74a6b11b07a2d768567e7bc06cf08bbcd68f) )
ROM_END



static DRIVER_INIT( namco86 )
{
	int size;
	unsigned char *gfx;
	unsigned char *buffer;

	/* shuffle tile ROMs so regular gfx unpack routines can be used */
	gfx = memory_region(REGION_GFX1);
	size = memory_region_length(REGION_GFX1) * 2 / 3;
	buffer = malloc( size );

	if ( buffer )
	{
		unsigned char *dest1 = gfx;
		unsigned char *dest2 = gfx + ( size / 2 );
		unsigned char *mono = gfx + size;
		int i;

		memcpy( buffer, gfx, size );

		for ( i = 0; i < size; i += 2 )
		{
			unsigned char data1 = buffer[i];
			unsigned char data2 = buffer[i+1];
			*dest1++ = ( data1 << 4 ) | ( data2 & 0xf );
			*dest2++ = ( data1 & 0xf0 ) | ( data2 >> 4 );

			*mono ^= 0xff; mono++;
		}

		free( buffer );
	}

	gfx = memory_region(REGION_GFX2);
	size = memory_region_length(REGION_GFX2) * 2 / 3;
	buffer = malloc( size );

	if ( buffer )
	{
		unsigned char *dest1 = gfx;
		unsigned char *dest2 = gfx + ( size / 2 );
		unsigned char *mono = gfx + size;
		int i;

		memcpy( buffer, gfx, size );

		for ( i = 0; i < size; i += 2 )
		{
			unsigned char data1 = buffer[i];
			unsigned char data2 = buffer[i+1];
			*dest1++ = ( data1 << 4 ) | ( data2 & 0xf );
			*dest2++ = ( data1 & 0xf0 ) | ( data2 >> 4 );

			*mono ^= 0xff; mono++;
		}

		free( buffer );
	}
}



WRITE_HANDLER( roishtar_semaphore_w )
{
    rthunder_videoram1_w(0x7e24-0x6000+offset,data);

    if (data == 0x02)
	    cpu_spinuntil_int();
}

static DRIVER_INIT( roishtar )
{
	/* install hook to avoid hang at game over */
    install_mem_write_handler(1, 0x7e24, 0x7e24, roishtar_semaphore_w);

	init_namco86();
}



GAME( 1986, hopmappy, 0,        hopmappy, hopmappy, namco86,  ROT0,   "Namco", "Hopping Mappy" )
GAME( 1986, skykiddx, 0,        skykiddx, skykiddx, namco86,  ROT180, "Namco", "Sky Kid Deluxe (set 1)" )
GAME( 1986, skykiddo, skykiddx, skykiddx, skykiddx, namco86,  ROT180, "Namco", "Sky Kid Deluxe (set 2)" )
GAME( 1986, roishtar, 0,        roishtar, roishtar, roishtar, ROT0,   "Namco", "The Return of Ishtar" )
GAME( 1986, genpeitd, 0,        genpeitd, genpeitd, namco86,  ROT0,   "Namco", "Genpei ToumaDen" )
GAME( 1986, rthunder, 0,        rthunder, rthunder, namco86,  ROT0,   "Namco", "Rolling Thunder (new version)" )
GAME( 1986, rthundro, rthunder, rthunder, rthundro, namco86,  ROT0,   "Namco", "Rolling Thunder (old version)" )
GAME( 1987, wndrmomo, 0,        wndrmomo, wndrmomo, namco86,  ROT0,   "Namco", "Wonder Momo" )
