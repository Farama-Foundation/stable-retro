/***************************************************************************

Crazy Balloon memory map (preliminary)

0000-2fff ROM
4000-43ff RAM
4800-4bff Video RAM
5000-53ff Color RAM

I/O:

read:
00        dsw
01        joystick
02        bit 0-3 from chip PC3259 (bit 3 is the sprite/char collision detection)
          bit 4-7 dsw
03        bit 0 dsw
          bit 1 high score name reset
		  bit 2 service
		  bit 3 tilt
		  bit 4-7 from chip PC3092; coin inputs & start buttons
06-0a-0e  mirror addresses for 02; address lines 2 and 3 go to the PC3256 chip
          so they probably alter its output, while the dsw bits (4-7) stay the same.

write:
01        ?
02        bit 0-3 sprite code bit 4-7 sprite color
03        sprite X pos
04        sprite Y pos
05        music?? to a counter?
06        sound
          bit 0 IRQ enable/acknowledge
          bit 1 sound enable
          bit 2 sound related (to amplifier)
          bit 3 explosion (to 76477)
          bit 4 breath (to 76477)
          bit 5 appear (to 76477)
          bit 6 sound related (to 555)
          bit 7 to chip PC3259
07        to chip PC3092 (bits 0-3)
08        to chip PC3092 (bits 0-3)
          bit 0 seems to be flip screen
          bit 1 might enable coin input
09        to chip PC3092 (bits 0-3)
0a        to chip PC3092 (bits 0-3)
0b        to chip PC3092 (bits 0-3)
0c        MSK (to chip PC3259)
0d        CC (not used)

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"


extern WRITE_HANDLER( crbaloon_videoram_w );
extern WRITE_HANDLER( crbaloon_colorram_w );
extern WRITE_HANDLER( crbaloon_spritectrl_w );
extern WRITE_HANDLER( crbaloon_flipscreen_w );

extern PALETTE_INIT( crbaloon );
extern VIDEO_START( crbaloon );
extern VIDEO_UPDATE( crbaloon );


int val06,val08,val0a;
static int crbaloon_tone_stream;
static UINT32 crbaloon_tone_step;
static UINT32 crbaloon_tone_pos;

static MACHINE_INIT( crbaloon )
{
	/* MIXER A = 0, MIXER C = 1 */
	SN76477_mixer_a_w(0, 0);
	SN76477_mixer_c_w(0, 1);
	/* ENVELOPE is constant: pin1 = hi, pin 28 = lo */
	SN76477_envelope_w(0, 1);
    /* fake: pulse the enable line to get rid of the constant noise */
    SN76477_enable_w(0, 1);
    SN76477_enable_w(0, 0);
}

WRITE_HANDLER( crbaloon_06_w )
{
	val06 = data;

	interrupt_enable_w(offset,data & 1);

	/* SOUND STOP high? */
    if( data & 0x02 )
	{

		if( data & 0x08 )
		{
			/* enable is connected to EXPLOSION */
			SN76477_enable_w(0, 1);
		}
		else
		{
			SN76477_enable_w(0, 0);
		}
		if( data & 0x10 )
		{
			/* BREATH changes slf_res to 10k (middle of two 10k resistors) */
			SN76477_set_slf_res(0, RES_K(10));
			/* it also puts a tantal capacitor agains GND on the output,
			   but this section of the schematics is not readable. */
		}
		else
		{
			SN76477_set_slf_res(0, RES_K(20));
		}

		if( data & 0x20 )
		{
			/* APPEAR is connected to MIXER B */
			SN76477_mixer_b_w(0, 1);
		}
		else
		{
			SN76477_mixer_b_w(0, 4);
		}

		/* constant: pin1 = hi, pin 28 = lo */
		SN76477_envelope_w(0, 1);
	}
}

WRITE_HANDLER( crbaloon_08_w )
{
	val08 = data;

	crbaloon_flipscreen_w(offset,data & 1);
}

WRITE_HANDLER( crbaloon_0a_w )
{
	val0a = data;
}

READ_HANDLER( crbaloon_IN2_r )
{
	extern int crbaloon_collision;

	if (crbaloon_collision != 0)
	{
		return (input_port_2_r(0) & 0xf0) | 0x08;
    }

	/* the following is needed for the game to boot up */
	if (val06 & 0x80)
	{
log_cb(RETRO_LOG_DEBUG, LOGPRE "PC %04x: %02x high\n",activecpu_get_pc(),offset);
		return (input_port_2_r(0) & 0xf0) | 0x07;
	}
	else
	{
log_cb(RETRO_LOG_DEBUG, LOGPRE "PC %04x: %02x low\n",activecpu_get_pc(),offset);
		return (input_port_2_r(0) & 0xf0) | 0x07;
	}
}

READ_HANDLER( crbaloon_IN3_r )
{
	if (val08 & 0x02)
		/* enable coin & start input? Wild guess!!! */
		return input_port_3_r(0);

	/* the following is needed for the game to boot up */
	if (val0a & 0x01)
	{
log_cb(RETRO_LOG_DEBUG, LOGPRE "PC %04x: 03 high\n",activecpu_get_pc());
		return (input_port_3_r(0) & 0x0f) | 0x00;
	}
	else
	{
log_cb(RETRO_LOG_DEBUG, LOGPRE "PC %04x: 03 low\n",activecpu_get_pc());
		return (input_port_3_r(0) & 0x0f) | 0x00;
	}
}


READ_HANDLER( crbaloon_IN_r )
{
	switch (offset & 0x03)
	{
		case 0:
			return input_port_0_r(offset);

		case 1:
			return input_port_1_r(offset);

		case 2:
			return crbaloon_IN2_r(offset);

		case 3:
			return crbaloon_IN3_r(offset);
	}

	return 0;
}

WRITE_HANDLER( crbaloon_tone_w )
{
	crbaloon_tone_step = 0;
	if (data && data != 0xff)
	{
		double freq = (13630.0 / (256 - data) + (data>=0xea ? 13 : 0))*0.5;
		crbaloon_tone_step = (UINT32)(freq * 65536.0 * 65536.0 / (double)Machine->sample_rate);
	}
}

static MEMORY_READ_START( readmem )
	{ 0x0000, 0x2fff, MRA_ROM },
	{ 0x4000, 0x43ff, MRA_RAM },
	{ 0x4800, 0x4bff, MRA_RAM },
	{ 0x5000, 0x53ff, MRA_RAM },
MEMORY_END

static MEMORY_WRITE_START( writemem )
	{ 0x0000, 0x2fff, MWA_ROM },
	{ 0x4000, 0x43ff, MWA_RAM },
	{ 0x4800, 0x4bff, crbaloon_videoram_w, &videoram },
	{ 0x5000, 0x53ff, crbaloon_colorram_w, &colorram },
MEMORY_END

static PORT_READ_START( readport )
	{ 0x00, 0x0f, crbaloon_IN_r },
PORT_END

static PORT_WRITE_START( writeport )
	{ 0x02, 0x04, crbaloon_spritectrl_w },
	{ 0x05, 0x05, crbaloon_tone_w },
	{ 0x06, 0x06, crbaloon_06_w },
	{ 0x08, 0x08, crbaloon_08_w },
	{ 0x0a, 0x0a, crbaloon_0a_w },
PORT_END



INPUT_PORTS_START( crbaloon )
	PORT_START
	PORT_DIPNAME( 0x01, 0x01, "Test?" )
	PORT_DIPSETTING(    0x01, "I/O Check?" )
	PORT_DIPSETTING(    0x00, "RAM Check?" )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x0c, 0x04, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "2" )
	PORT_DIPSETTING(    0x04, "3" )
	PORT_DIPSETTING(    0x08, "4" )
	PORT_DIPSETTING(    0x0c, "5" )
	PORT_DIPNAME( 0x10, 0x00, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x00, "5000" )
	PORT_DIPSETTING(    0x10, "10000" )
	PORT_DIPNAME( 0xe0, 0x80, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x60, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0xa0, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0xe0, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x00, "Disable" )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )

	PORT_START
	PORT_BIT( 0x0f, IP_ACTIVE_HIGH, IPT_UNKNOWN )	/* from chip PC3259 */
	PORT_BITX(    0x10, 0x10, IPT_DIPSWITCH_NAME | IPF_CHEAT, "Invulnerability", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_BUTTON1, "High Score Name Reset", IP_KEY_DEFAULT, IP_JOY_DEFAULT )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN2 )	/* should be COIN2 */
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_TILT )
	/* the following four bits come from chip PC3092 */
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )
INPUT_PORTS_END



static struct GfxLayout charlayout =
{
	8,8,	/* 8*8 characters */
	256,	/* 256 characters */
	1,	/* 1 bit per pixel */
	{ 0 },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 7*8, 6*8, 5*8, 4*8, 3*8, 2*8, 1*8, 0*8 },
	8*8	/* every char takes 8 consecutive bytes */
};
static struct GfxLayout spritelayout =
{
	32,32,	/* 32*32 sprites */
	16,	/* 16 sprites */
	1,	/* 1 bit per pixel */
	{ 0 },
	{ 3*32*8+0, 3*32*8+1, 3*32*8+2, 3*32*8+3, 3*32*8+4, 3*32*8+5, 3*32*8+6, 3*32*8+7,
			2*32*8+0, 2*32*8+1, 2*32*8+2, 2*32*8+3, 2*32*8+4, 2*32*8+5, 2*32*8+6, 2*32*8+7,
			1*32*8+0, 1*32*8+1, 1*32*8+2, 1*32*8+3, 1*32*8+4, 1*32*8+5, 1*32*8+6, 1*32*8+7,
			0*32*8+0, 0*32*8+1, 0*32*8+2, 0*32*8+3, 0*32*8+4, 0*32*8+5, 0*32*8+6, 0*32*8+7 },
	{ 31*8, 30*8, 29*8, 28*8, 27*8, 26*8, 25*8, 24*8,
			23*8, 22*8, 21*8, 20*8, 19*8, 18*8, 17*8, 16*8,
			15*8, 14*8, 13*8, 12*8, 11*8, 10*8, 9*8, 8*8,
			7*8, 6*8, 5*8, 4*8, 3*8, 2*8, 1*8, 0*8 },
	32*4*8  /* every sprite takes 128 consecutive bytes */
};


static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &charlayout,   0, 16 },
	{ REGION_GFX2, 0, &spritelayout, 0, 16 },
	{ -1 } /* end of array */
};


static struct SN76477interface sn76477_interface =
{
	1,	/* 1 chip */
	{ 100 }, /* mixing level   pin description		 */
	{ RES_K( 47)   },		/*	4  noise_res		 */
	{ RES_K(330)   },		/*	5  filter_res		 */
	{ CAP_P(470)   },		/*	6  filter_cap		 */
	{ RES_K(220)   },		/*	7  decay_res		 */
	{ CAP_U(1.0)   },		/*	8  attack_decay_cap  */
	{ RES_K(4.7)   },		/* 10  attack_res		 */
	{ RES_M(  1)   },		/* 11  amplitude_res	 */
	{ RES_K(200)   },		/* 12  feedback_res 	 */
	{ 5.0		   },		/* 16  vco_voltage		 */
	{ CAP_P(470)   },		/* 17  vco_cap			 */
	{ RES_K(330)   },		/* 18  vco_res			 */
	{ 5.0		   },		/* 19  pitch_voltage	 */
	{ RES_K( 20)   },		/* 20  slf_res			 */
	{ CAP_P(420)   },		/* 21  slf_cap			 */
	{ CAP_U(1.0)   },		/* 23  oneshot_cap		 */
	{ RES_K( 47)   }		/* 24  oneshot_res		 */
};

static void crbaloon_tone_update(int ch, INT16 *buffer, int len)
{
	memset(buffer, 0, len * sizeof(INT16));
	if (crbaloon_tone_step)
	{
		while (len-- > 0)
		{
			*buffer++ = crbaloon_tone_pos & 0x80000000 ? 0x7fff : 0x8000;
			crbaloon_tone_pos += crbaloon_tone_step;
		}
	}
}
static int crbaloon_sh_start(const struct MachineSound *msound)
{
	crbaloon_tone_step = 0;
	crbaloon_tone_pos = 0;
	crbaloon_tone_stream = stream_init(sound_name(msound), 10, Machine->sample_rate, 0, crbaloon_tone_update);
	return 0;
}
struct CustomSound_interface crbaloon_custom_interface =
{
	crbaloon_sh_start,
	0,
	0
};

static MACHINE_DRIVER_START( crbaloon )

	/* basic machine hardware */
	MDRV_CPU_ADD(Z80, 3072000)	/* 3.072 MHz ????? */
	MDRV_CPU_MEMORY(readmem,writemem)
	MDRV_CPU_PORTS(readport,writeport)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,1)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)
	
	MDRV_MACHINE_INIT(crbaloon)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 0*8, 28*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(16)
	MDRV_COLORTABLE_LENGTH(16*2)

	MDRV_PALETTE_INIT(crbaloon)
	MDRV_VIDEO_START(crbaloon)
	MDRV_VIDEO_UPDATE(crbaloon)

	/* sound hardware */
	MDRV_SOUND_ADD(SN76477, sn76477_interface)
	MDRV_SOUND_ADD(CUSTOM, crbaloon_custom_interface)
MACHINE_DRIVER_END



/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( crbaloon )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )     /* 64k for code */
	ROM_LOAD( "cl01.bin",     0x0000, 0x0800, CRC(9d4eef0b) SHA1(a8dd814ac2612073982123c91fa62deaf5bee242) )
	ROM_LOAD( "cl02.bin",     0x0800, 0x0800, CRC(10f7a6f7) SHA1(e672a7dcdaae08b202cfc2e19033846ebb267e1b) )
	ROM_LOAD( "cl03.bin",     0x1000, 0x0800, CRC(44ed6030) SHA1(8bbf5d9e893710138be15e56682037f128c83527) )
	ROM_LOAD( "cl04.bin",     0x1800, 0x0800, CRC(62f66f6c) SHA1(d173b12d6b5e0719d7b25ff0cafebbe64ec6b134) )
	ROM_LOAD( "cl05.bin",     0x2000, 0x0800, CRC(c8f1e2be) SHA1(a4603ce0268fa987f7f780702b6ca04e28759674) )
	ROM_LOAD( "cl06.bin",     0x2800, 0x0800, CRC(7d465691) SHA1(f5dc7abe8db232f702419d126cee6607ea6a5168) )

	ROM_REGION( 0x0800, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "cl07.bin",     0x0000, 0x0800, CRC(2c1fbea8) SHA1(41cf2aef74d56173057886512d989f6fa3682056) )

	ROM_REGION( 0x0800, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "cl08.bin",     0x0000, 0x0800, CRC(ba898659) SHA1(4291059b113ff91896f1f61a4c14956716edfe1e) )
ROM_END

ROM_START( crbalon2 )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )     /* 64k for code */
	ROM_LOAD( "cl01.bin",     0x0000, 0x0800, CRC(9d4eef0b) SHA1(a8dd814ac2612073982123c91fa62deaf5bee242) )
	ROM_LOAD( "crazybal.ep2", 0x0800, 0x0800, CRC(87572086) SHA1(dba842c7c4cb16154ae0da43d71f8f03a56441c3) )
	ROM_LOAD( "crazybal.ep3", 0x1000, 0x0800, CRC(575fe995) SHA1(829db1da27cc9b706db6d9563bd271ffcd42be4a) )
	ROM_LOAD( "cl04.bin",     0x1800, 0x0800, CRC(62f66f6c) SHA1(d173b12d6b5e0719d7b25ff0cafebbe64ec6b134) )
	ROM_LOAD( "cl05.bin",     0x2000, 0x0800, CRC(c8f1e2be) SHA1(a4603ce0268fa987f7f780702b6ca04e28759674) )
	ROM_LOAD( "crazybal.ep6", 0x2800, 0x0800, CRC(fed6ff5c) SHA1(e6ed276949fd1511c6abe97026793193fda36e92) )

	ROM_REGION( 0x0800, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "cl07.bin",     0x0000, 0x0800, CRC(2c1fbea8) SHA1(41cf2aef74d56173057886512d989f6fa3682056) )

	ROM_REGION( 0x0800, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "cl08.bin",     0x0000, 0x0800, CRC(ba898659) SHA1(4291059b113ff91896f1f61a4c14956716edfe1e) )
ROM_END



GAMEX( 1980, crbaloon, 0,        crbaloon, crbaloon, 0, ROT90, "Taito Corporation", "Crazy Balloon (set 1)", GAME_IMPERFECT_SOUND )
GAMEX( 1980, crbalon2, crbaloon, crbaloon, crbaloon, 0, ROT90, "Taito Corporation", "Crazy Balloon (set 2)", GAME_IMPERFECT_SOUND )
