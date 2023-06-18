/***************************************************************************

	Atari Star Wars hardware

	driver by Steve Baines (sulaco@ntlworld.com) and Frank Palazzolo

	This file is Copyright 1997, Steve Baines.
	Modified by Frank Palazzolo for sound support

	Games supported:
		* Star Wars
		* The Empire Strikes Back

	Known bugs:
		* none at this time

****************************************************************************

	Memory map (TBA)

***************************************************************************/

#include "driver.h"
#include "cpu/m6809/m6809.h"
/*#include "machine/atari_vg.h"*/
/*#include "vidhrdw/generic.h"*/
#include "vidhrdw/vector.h"
#include "vidhrdw/avgdvg.h"
#include "starwars.h"
#include "slapstic.h"


/* Local variables */
static UINT8 *slapstic_source;
static UINT8 *slapstic_base;
static UINT8 current_bank;
UINT8 starwars_is_esb;



/*************************************
 *
 *	Machine init
 *
 *************************************/

MACHINE_INIT( starwars )
{
	/* ESB-specific */
	if (starwars_is_esb)
	{
		/* reset the slapstic */
		slapstic_reset();
		current_bank = slapstic_bank();
		memcpy(slapstic_base, &slapstic_source[current_bank * 0x2000], 0x2000);

		/* reset all the banks */
		starwars_out_w(4, 0);
	}

	/* reset the mathbox */
	swmathbox_reset();
}



/*************************************
 *
 *	Interrupt generation
 *
 *************************************/

static WRITE_HANDLER( irq_ack_w )
{
	cpu_set_irq_line(0, M6809_IRQ_LINE, CLEAR_LINE);
}



/*************************************
 *
 *	ESB Slapstic handler
 *
 *************************************/

READ_HANDLER( esb_slapstic_r )
{
	int result = slapstic_base[offset];
	int new_bank = slapstic_tweak(offset);

	/* update for the new bank */
	if (new_bank != current_bank)
	{
		current_bank = new_bank;
		memcpy(slapstic_base, &slapstic_source[current_bank * 0x2000], 0x2000);
	}
	return result;
}


WRITE_HANDLER( esb_slapstic_w )
{
	int new_bank = slapstic_tweak(offset);

	/* update for the new bank */
	if (new_bank != current_bank)
	{
		current_bank = new_bank;
		memcpy(slapstic_base, &slapstic_source[current_bank * 0x2000], 0x2000);
	}
}



/*************************************
 *
 *	ESB Opcode base handler
 *
 *************************************/

OPBASE_HANDLER( esb_setopbase )
{
	int prevpc = activecpu_get_previouspc();

	/*
	 *	This is a slightly ugly kludge for Empire Strikes Back because it jumps
	 *	directly to code in the slapstic.
	 */

	/* if we're jumping into the slapstic region, tweak the new PC */
	if ((address & 0xe000) == 0x8000)
	{
		esb_slapstic_r(address & 0x1fff);

		/* make sure we catch the next branch as well */
		catch_nextBranch();
		return -1;
	}

	/* if we're jumping out of the slapstic region, tweak the previous PC */
	else if ((prevpc & 0xe000) == 0x8000)
	{
		if (prevpc != 0x8080 && prevpc != 0x8090 && prevpc != 0x80a0 && prevpc != 0x80b0)
			esb_slapstic_r(prevpc & 0x1fff);
	}

	return address;
}



/*************************************
 *
 *	Main CPU memory handlers
 *
 *************************************/

static MEMORY_READ_START( main_readmem )
	{ 0x0000, 0x2fff, MRA_RAM },			/* vector_ram */
	{ 0x3000, 0x3fff, MRA_ROM },			/* vector_rom */
	{ 0x4300, 0x431f, input_port_0_r },		/* Memory mapped input port 0 */
	{ 0x4320, 0x433f, starwars_input_1_r },	/* Memory mapped input port 1 */
	{ 0x4340, 0x435f, input_port_2_r },		/* DIP switches bank 0 */
	{ 0x4360, 0x437f, input_port_3_r },		/* DIP switches bank 1 */
	{ 0x4380, 0x439f, starwars_adc_r },		/* a-d control result */
	{ 0x4400, 0x4400, starwars_main_read_r },
	{ 0x4401, 0x4401, starwars_main_ready_flag_r },
	{ 0x4500, 0x45ff, MRA_RAM },			/* nov_ram */
	{ 0x4700, 0x4700, swmathbx_reh_r },
	{ 0x4701, 0x4701, swmathbx_rel_r },
	{ 0x4703, 0x4703, swmathbx_prng_r },	/* pseudo random number generator */
	{ 0x4800, 0x5fff, MRA_RAM },			/* CPU and Math RAM */
	{ 0x6000, 0x7fff, MRA_BANK1 },			/* banked ROM */
	{ 0x8000, 0xffff, MRA_ROM },			/* rest of main_rom */
MEMORY_END


static MEMORY_WRITE_START( main_writemem )
	{ 0x0000, 0x2fff, MWA_RAM, &vectorram, &vectorram_size },
	{ 0x3000, 0x3fff, MWA_ROM },								/* vector_rom */
	{ 0x4400, 0x4400, starwars_main_wr_w },
	{ 0x4500, 0x45ff, MWA_RAM, &generic_nvram, &generic_nvram_size },
	{ 0x4600, 0x461f, avgdvg_go_w },
	{ 0x4620, 0x463f, avgdvg_reset_w },
	{ 0x4640, 0x465f, watchdog_reset_w },
	{ 0x4660, 0x467f, irq_ack_w },
	{ 0x4680, 0x4687, starwars_out_w },
	{ 0x46a0, 0x46bf, MWA_NOP },								/* nstore */
	{ 0x46c0, 0x46c2, starwars_adc_select_w },
	{ 0x46e0, 0x46e0, starwars_soundrst_w },
	{ 0x4700, 0x4707, swmathbx_w },
	{ 0x4800, 0x5fff, MWA_RAM },		/* CPU and Math RAM */
	{ 0x6000, 0xffff, MWA_ROM },		/* main_rom */
MEMORY_END



/*************************************
 *
 *	Sound CPU memory handlers
 *
 *************************************/

static MEMORY_READ_START( sound_readmem )
	{ 0x0800, 0x0fff, starwars_sin_r },		/* SIN Read */
	{ 0x1000, 0x107f, MRA_RAM },	/* 6532 RAM */
	{ 0x1080, 0x109f, starwars_m6532_r },
	{ 0x2000, 0x27ff, MRA_RAM },	/* program RAM */
	{ 0x4000, 0xbfff, MRA_ROM },	/* sound roms */
	{ 0xc000, 0xffff, MRA_ROM },	/* load last rom twice */
									/* for proper int vec operation */
MEMORY_END


static MEMORY_WRITE_START( sound_writemem )
	{ 0x0000, 0x07ff, starwars_sout_w },
	{ 0x1000, 0x107f, MWA_RAM }, /* 6532 ram */
	{ 0x1080, 0x109f, starwars_m6532_w },
	{ 0x1800, 0x183f, quad_pokey_w },
	{ 0x2000, 0x27ff, MWA_RAM }, /* program RAM */
	{ 0x4000, 0xbfff, MWA_ROM }, /* sound rom */
	{ 0xc000, 0xffff, MWA_ROM }, /* sound rom again, for intvecs */
MEMORY_END



/*************************************
 *
 *	Port definitions
 *
 *************************************/

INPUT_PORTS_START( starwars )
	PORT_START	/* IN0 */
	PORT_BIT ( 0x01, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT ( 0x02, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT ( 0x04, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT ( 0x08, IP_ACTIVE_LOW, IPT_TILT )
	PORT_SERVICE( 0x10, IP_ACTIVE_LOW )
	PORT_BIT ( 0x20, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT ( 0x40, IP_ACTIVE_LOW, IPT_BUTTON4 )
	PORT_BIT ( 0x80, IP_ACTIVE_LOW, IPT_BUTTON1 )

	PORT_START	/* IN1 */
	PORT_BIT ( 0x01, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT ( 0x02, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BITX( 0x04, IP_ACTIVE_LOW, IPT_SERVICE, "Diagnostic Step", KEYCODE_F1, IP_JOY_NONE )
	PORT_BIT ( 0x08, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT ( 0x10, IP_ACTIVE_LOW, IPT_BUTTON3 )
	PORT_BIT ( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )
	/* Bit 6 is MATH_RUN - see machine/starwars.c */
	PORT_BIT ( 0x40, IP_ACTIVE_HIGH, IPT_UNUSED )
	/* Bit 7 is VG_HALT - see machine/starwars.c */
	PORT_BIT ( 0x80, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START	/* DSW0 */
	PORT_DIPNAME(0x03, 0x00, "Starting Shields" )
	PORT_DIPSETTING (  0x00, "6" )
	PORT_DIPSETTING (  0x01, "7" )
	PORT_DIPSETTING (  0x02, "8" )
	PORT_DIPSETTING (  0x03, "9" )
	PORT_DIPNAME(0x0c, 0x00, DEF_STR( Difficulty ) )
	PORT_DIPSETTING (  0x00, "Easy" )
	PORT_DIPSETTING (  0x04, "Moderate" )
	PORT_DIPSETTING (  0x08, "Hard" )
	PORT_DIPSETTING (  0x0c, "Hardest" )
	PORT_DIPNAME(0x30, 0x00, "Bonus Shields" )
	PORT_DIPSETTING (  0x00, "0" )
	PORT_DIPSETTING (  0x10, "1" )
	PORT_DIPSETTING (  0x20, "2" )
	PORT_DIPSETTING (  0x30, "3" )
	PORT_DIPNAME(0x40, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING (  0x40, DEF_STR( Off ) )
	PORT_DIPSETTING (  0x00, DEF_STR( On ) )
	PORT_DIPNAME(0x80, 0x80, "Freeze" )
	PORT_DIPSETTING (  0x80, DEF_STR( Off ) )
	PORT_DIPSETTING (  0x00, DEF_STR( On ) )

	PORT_START	/* DSW1 */
	PORT_DIPNAME(0x03, 0x02, DEF_STR( Coinage ) )
	PORT_DIPSETTING (  0x03, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING (  0x02, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING (  0x01, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING (  0x00, DEF_STR( Free_Play ) )
	PORT_DIPNAME(0x0c, 0x00, DEF_STR( Coin_B ) )
	PORT_DIPSETTING (  0x00, "*1" )
	PORT_DIPSETTING (  0x04, "*4" )
	PORT_DIPSETTING (  0x08, "*5" )
	PORT_DIPSETTING (  0x0c, "*6" )
	PORT_DIPNAME(0x10, 0x00, DEF_STR( Coin_A ) )
	PORT_DIPSETTING (  0x00, "*1" )
	PORT_DIPSETTING (  0x10, "*2" )
	PORT_DIPNAME(0xe0, 0x00, "Bonus Coinage" )
	PORT_DIPSETTING (  0x20, "2 gives 1" )
	PORT_DIPSETTING (  0x60, "4 gives 2" )
	PORT_DIPSETTING (  0xa0, "3 gives 1" )
	PORT_DIPSETTING (  0x40, "4 gives 1" )
	PORT_DIPSETTING (  0x80, "5 gives 1" )
	PORT_DIPSETTING (  0x00, "None" )
/* 0xc0 and 0xe0 None */

	PORT_START	/* IN4 */
	PORT_ANALOG( 0xff, 0x80, IPT_AD_STICK_Y, 70, 30, 0, 255 )

	PORT_START	/* IN5 */
	PORT_ANALOG( 0xff, 0x80, IPT_AD_STICK_X, 50, 30, 0, 255 )
INPUT_PORTS_END


INPUT_PORTS_START( esb )
	PORT_START	/* IN0 */
	PORT_BIT ( 0x01, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT ( 0x02, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT ( 0x04, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT ( 0x08, IP_ACTIVE_LOW, IPT_TILT )
	PORT_SERVICE( 0x10, IP_ACTIVE_LOW )
	PORT_BIT ( 0x20, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT ( 0x40, IP_ACTIVE_LOW, IPT_BUTTON4 )
	PORT_BIT ( 0x80, IP_ACTIVE_LOW, IPT_BUTTON1 )

	PORT_START	/* IN1 */
	PORT_BIT ( 0x01, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT ( 0x02, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BITX( 0x04, IP_ACTIVE_LOW, IPT_SERVICE, "Diagnostic Step", KEYCODE_F1, IP_JOY_NONE )
	PORT_BIT ( 0x08, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT ( 0x10, IP_ACTIVE_LOW, IPT_BUTTON3 )
	PORT_BIT ( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )
	/* Bit 6 is MATH_RUN - see machine/starwars.c */
	PORT_BIT ( 0x40, IP_ACTIVE_HIGH, IPT_UNUSED )
	/* Bit 7 is VG_HALT - see machine/starwars.c */
	PORT_BIT ( 0x80, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START	/* DSW0 */
	PORT_DIPNAME(0x03, 0x03, "Starting Shields" )
	PORT_DIPSETTING (  0x01, "2" )
	PORT_DIPSETTING (  0x00, "3" )
	PORT_DIPSETTING (  0x03, "4" )
	PORT_DIPSETTING (  0x02, "5" )
	PORT_DIPNAME(0x0c, 0x0c, DEF_STR( Difficulty ) )
	PORT_DIPSETTING (  0x08, "Easy" )
	PORT_DIPSETTING (  0x0c, "Moderate" )
	PORT_DIPSETTING (  0x00, "Hard" )
	PORT_DIPSETTING (  0x04, "Hardest" )
	PORT_DIPNAME(0x30, 0x30, "Jedi-Letter Mode" )
	PORT_DIPSETTING (  0x00, "Level Only" )
	PORT_DIPSETTING (  0x10, "Level" )
	PORT_DIPSETTING (  0x20, "Increment Only" )
	PORT_DIPSETTING (  0x30, "Increment" )
	PORT_DIPNAME(0x40, 0x40, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING (  0x00, DEF_STR( Off ) )
	PORT_DIPSETTING (  0x40, DEF_STR( On ) )
	PORT_DIPNAME(0x80, 0x80, "Freeze" )
	PORT_DIPSETTING (  0x80, DEF_STR( Off ) )
	PORT_DIPSETTING (  0x00, DEF_STR( On ) )

	PORT_START	/* DSW1 */
	PORT_DIPNAME(0x03, 0x02, DEF_STR( Coinage ) )
	PORT_DIPSETTING (  0x03, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING (  0x02, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING (  0x01, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING (  0x00, DEF_STR( Free_Play ) )
	PORT_DIPNAME(0x0c, 0x00, DEF_STR( Coin_B ) )
	PORT_DIPSETTING (  0x00, "*1" )
	PORT_DIPSETTING (  0x04, "*4" )
	PORT_DIPSETTING (  0x08, "*5" )
	PORT_DIPSETTING (  0x0c, "*6" )
	PORT_DIPNAME(0x10, 0x00, DEF_STR( Coin_A ) )
	PORT_DIPSETTING (  0x00, "*1" )
	PORT_DIPSETTING (  0x10, "*2" )
	PORT_DIPNAME(0xe0, 0xe0, "Bonus Coinage" )
	PORT_DIPSETTING (  0x20, "2 gives 1" )
	PORT_DIPSETTING (  0x60, "4 gives 2" )
	PORT_DIPSETTING (  0xa0, "3 gives 1" )
	PORT_DIPSETTING (  0x40, "4 gives 1" )
	PORT_DIPSETTING (  0x80, "5 gives 1" )
	PORT_DIPSETTING (  0xe0, "None" )
/* 0xc0 and 0x00 None */

	PORT_START	/* IN4 */
	PORT_ANALOG( 0xff, 0x80, IPT_AD_STICK_Y, 70, 30, 0, 255 )

	PORT_START	/* IN5 */
	PORT_ANALOG( 0xff, 0x80, IPT_AD_STICK_X, 50, 30, 0, 255 )
INPUT_PORTS_END



/*************************************
 *
 *	Sound definitions
 *
 *************************************/

static struct POKEYinterface pokey_interface =
{
	4,			/* 4 chips */
	1500000,	/* 1.5 MHz? */
	{ 20, 20, 20, 20 },	/* volume */
	/* The 8 pot handlers */
	{ 0, 0, 0, 0 },
	{ 0, 0, 0, 0 },
	{ 0, 0, 0, 0 },
	{ 0, 0, 0, 0 },
	{ 0, 0, 0, 0 },
	{ 0, 0, 0, 0 },
	{ 0, 0, 0, 0 },
	{ 0, 0, 0, 0 },
	/* The allpot handler */
	{ 0, 0, 0, 0 },
};


static struct TMS5220interface tms5220_interface =
{
	640000,     /* clock speed (80*samplerate) */
	50,         /* volume */
	0           /* IRQ handler */
};



/*************************************
 *
 *	Machine driver
 *
 *************************************/

static MACHINE_DRIVER_START( starwars )

	/* basic machine hardware */
	MDRV_CPU_ADD(M6809,1500000)
	MDRV_CPU_MEMORY(main_readmem,main_writemem)
	MDRV_CPU_VBLANK_INT(irq0_line_assert,6)		/* 183Hz ? */

	MDRV_CPU_ADD(M6809,1500000)
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
	MDRV_CPU_MEMORY(sound_readmem,sound_writemem)

	MDRV_FRAMES_PER_SECOND(30)
	MDRV_MACHINE_INIT(starwars)
	MDRV_NVRAM_HANDLER(generic_0fill)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_VECTOR | VIDEO_RGB_DIRECT)
	MDRV_SCREEN_SIZE(400, 300)
	MDRV_VISIBLE_AREA(0, 250, 0, 280)
	MDRV_PALETTE_LENGTH(32768)

	MDRV_PALETTE_INIT(avg_multi)
	MDRV_VIDEO_START(avg_starwars)
	MDRV_VIDEO_UPDATE(vector)

	/* sound hardware */
	MDRV_SOUND_ADD(POKEY, pokey_interface)
	MDRV_SOUND_ADD(TMS5220, tms5220_interface)
MACHINE_DRIVER_END



/*************************************
 *
 *	ROM definitions
 *
 *************************************/

ROM_START( starwar1 )
	ROM_REGION( 0x12000, REGION_CPU1, 0 )     /* 2 64k ROM spaces */
	ROM_LOAD( "136021.105",   0x3000, 0x1000, CRC(538e7d2f) SHA1(032c933fd94a6b0b294beee29159a24494ae969b) ) /* 3000-3fff is 4k vector rom */
	ROM_LOAD( "136021.114",   0x6000, 0x2000, CRC(e75ff867) SHA1(3a40de920c31ffa3c3e67f3edf653b79fcc5ddd7) )   /* ROM 0 bank pages 0 and 1 */
	ROM_CONTINUE(            0x10000, 0x2000 )
	ROM_LOAD( "136021.102",   0x8000, 0x2000, CRC(f725e344) SHA1(f8943b67f2ea032ab9538084756ba86f892be5ca) ) /*  8k ROM 1 bank */
	ROM_LOAD( "136021.203",   0xa000, 0x2000, CRC(f6da0a00) SHA1(dd53b643be856787bbc4da63e5eb132f98f623c3) ) /*  8k ROM 2 bank */
	ROM_LOAD( "136021.104",   0xc000, 0x2000, CRC(7e406703) SHA1(981b505d6e06d7149f8bcb3e81e4d0c790f2fc86) ) /*  8k ROM 3 bank */
	ROM_LOAD( "136021.206",   0xe000, 0x2000, CRC(c7e51237) SHA1(4960f4446271316e3f730eeb2531dbc702947395) ) /*  8k ROM 4 bank */

	/* Sound ROMS */
	ROM_REGION( 0x10000, REGION_CPU2, 0 )     /* Really only 32k, but it looks like 64K */
	ROM_LOAD( "136021.107",   0x4000, 0x2000, CRC(dbf3aea2) SHA1(c38661b2b846fe93487eef09ca3cda19c44f08a0) ) /* Sound ROM 0 */
	ROM_RELOAD(               0xc000, 0x2000 ) /* Copied again for */
	ROM_LOAD( "136021.208",   0x6000, 0x2000, CRC(e38070a8) SHA1(c858ae1702efdd48615453ab46e488848891d139) ) /* Sound ROM 0 */
	ROM_RELOAD(               0xe000, 0x2000 ) /* proper int vecs */

	/* Mathbox PROMs */
	ROM_REGION( 0x1000, REGION_PROMS, ROMREGION_DISPOSE )
	ROM_LOAD( "136021.110",   0x0000, 0x0400, CRC(01061762) SHA1(199549ead9ff9a5d5db75a2d15aef0240feb1aca) ) /* PROM 0 */
	ROM_LOAD( "136021.111",   0x0400, 0x0400, CRC(2e619b70) SHA1(f76132b884ffdf3a4fe58637238c5235aab50408) ) /* PROM 1 */
	ROM_LOAD( "136021.112",   0x0800, 0x0400, CRC(6cfa3544) SHA1(be059bfffe6b5bfcb4af78e9e7a98870ccf21985) ) /* PROM 2 */
	ROM_LOAD( "136021.113",   0x0c00, 0x0400, CRC(03f6acb2) SHA1(1bad540950575581067b97e3a1cadd479c68adf1) ) /* PROM 3 */
ROM_END


ROM_START( starwars )
	ROM_REGION( 0x12000, REGION_CPU1, 0 )     /* 2 64k ROM spaces */
	ROM_LOAD( "136021.105",   0x3000, 0x1000, CRC(538e7d2f) SHA1(032c933fd94a6b0b294beee29159a24494ae969b) ) /* 3000-3fff is 4k vector rom */
	ROM_LOAD( "136021.214",   0x6000, 0x2000, CRC(04f1876e) SHA1(c1d3637cb31ece0890c25f6122d6bcd27e6ffe0c) )   /* ROM 0 bank pages 0 and 1 */
	ROM_CONTINUE(            0x10000, 0x2000 )
	ROM_LOAD( "136021.102",   0x8000, 0x2000, CRC(f725e344) SHA1(f8943b67f2ea032ab9538084756ba86f892be5ca) ) /*  8k ROM 1 bank */
	ROM_LOAD( "136021.203",   0xa000, 0x2000, CRC(f6da0a00) SHA1(dd53b643be856787bbc4da63e5eb132f98f623c3) ) /*  8k ROM 2 bank */
	ROM_LOAD( "136021.104",   0xc000, 0x2000, CRC(7e406703) SHA1(981b505d6e06d7149f8bcb3e81e4d0c790f2fc86) ) /*  8k ROM 3 bank */
	ROM_LOAD( "136021.206",   0xe000, 0x2000, CRC(c7e51237) SHA1(4960f4446271316e3f730eeb2531dbc702947395) ) /*  8k ROM 4 bank */

	/* Sound ROMS */
	ROM_REGION( 0x10000, REGION_CPU2, 0 )     /* Really only 32k, but it looks like 64K */
	ROM_LOAD( "136021.107",   0x4000, 0x2000, CRC(dbf3aea2) SHA1(c38661b2b846fe93487eef09ca3cda19c44f08a0) ) /* Sound ROM 0 */
	ROM_RELOAD(               0xc000, 0x2000 ) /* Copied again for */
	ROM_LOAD( "136021.208",   0x6000, 0x2000, CRC(e38070a8) SHA1(c858ae1702efdd48615453ab46e488848891d139) ) /* Sound ROM 0 */
	ROM_RELOAD(               0xe000, 0x2000 ) /* proper int vecs */

	/* Mathbox PROMs */
	ROM_REGION( 0x1000, REGION_PROMS, ROMREGION_DISPOSE )
	ROM_LOAD( "136021.110",   0x0000, 0x0400, CRC(01061762) SHA1(199549ead9ff9a5d5db75a2d15aef0240feb1aca) ) /* PROM 0 */
	ROM_LOAD( "136021.111",   0x0400, 0x0400, CRC(2e619b70) SHA1(f76132b884ffdf3a4fe58637238c5235aab50408) ) /* PROM 1 */
	ROM_LOAD( "136021.112",   0x0800, 0x0400, CRC(6cfa3544) SHA1(be059bfffe6b5bfcb4af78e9e7a98870ccf21985) ) /* PROM 2 */
	ROM_LOAD( "136021.113",   0x0c00, 0x0400, CRC(03f6acb2) SHA1(1bad540950575581067b97e3a1cadd479c68adf1) ) /* PROM 3 */
ROM_END


ROM_START( esb )
	ROM_REGION( 0x22000, REGION_CPU1, 0 )     /* 64k for code and a buttload for the banked ROMs */
	ROM_LOAD( "136031.111",   0x03000, 0x1000, CRC(b1f9bd12) SHA1(76f15395c9fdcd80dd241307a377031a1f44e150) )    /* 3000-3fff is 4k vector rom */
	ROM_LOAD( "136031.101",   0x06000, 0x2000, CRC(ef1e3ae5) SHA1(d228ff076faa7f9605badeee3b827adb62593e0a) )
	ROM_CONTINUE(             0x10000, 0x2000 )
	/* $8000 - $9fff : slapstic page */
	ROM_LOAD( "136031.102",   0x0a000, 0x2000, CRC(62ce5c12) SHA1(976256acf4499dc396542a117910009a8808f448) )
	ROM_CONTINUE(             0x1c000, 0x2000 )
	ROM_LOAD( "136031.203",   0x0c000, 0x2000, CRC(27b0889b) SHA1(a13074e83f0f57d65096d7f49ae78f33ab00c479) )
	ROM_CONTINUE(             0x1e000, 0x2000 )
	ROM_LOAD( "136031.104",   0x0e000, 0x2000, CRC(fd5c725e) SHA1(541cfd004b1736b6cec13836dfa813f00eedeed0) )
	ROM_CONTINUE(             0x20000, 0x2000 )

	ROM_LOAD( "136031.105",   0x14000, 0x4000, CRC(ea9e4dce) SHA1(9363fd5b1fce62c2306b448a7766eaf7ec97cdf5) ) /* slapstic 0, 1 */
	ROM_LOAD( "136031.106",   0x18000, 0x4000, CRC(76d07f59) SHA1(44dd018b406f95e1512ce92923c2c87f1458844f) ) /* slapstic 2, 3 */

	/* Sound ROMS */
	ROM_REGION( 0x10000, REGION_CPU2, 0 )
	ROM_LOAD( "136031.113",   0x4000, 0x2000, CRC(24ae3815) SHA1(b1a93af76de79b902317eebbc50b400b1f8c1e3c) ) /* Sound ROM 0 */
	ROM_CONTINUE(             0xc000, 0x2000 ) /* Copied again for */
	ROM_LOAD( "136031.112",   0x6000, 0x2000, CRC(ca72d341) SHA1(52de5b82bb85d7c9caad2047e540d0748aa93ba5) ) /* Sound ROM 1 */
	ROM_CONTINUE(             0xe000, 0x2000 ) /* proper int vecs */

	/* Mathbox PROMs */
	ROM_REGION( 0x1000, REGION_PROMS, ROMREGION_DISPOSE )
	ROM_LOAD( "136031.110",   0x0000, 0x0400, CRC(b8d0f69d) SHA1(c196f1a592bd1ac482a81e23efa224d9dfaefc0a) ) /* PROM 0 */
	ROM_LOAD( "136031.109",   0x0400, 0x0400, CRC(6a2a4d98) SHA1(cefca71f025f92a193c5a7d8b5ab8be10db2fd44) ) /* PROM 1 */
	ROM_LOAD( "136031.108",   0x0800, 0x0400, CRC(6a76138f) SHA1(9ef7af898a3e29d03f35045901023615a6a55205) ) /* PROM 2 */
	ROM_LOAD( "136031.107",   0x0c00, 0x0400, CRC(afbf6e01) SHA1(0a6438e6c106d98e5d67a019751e1584324f5e5c) ) /* PROM 3 */
ROM_END



/*************************************
 *
 *	Driver init
 *
 *************************************/

static DRIVER_INIT( starwars )
{
	/* prepare the mathbox */
	starwars_is_esb = 0;
	swmathbox_init();
}


static DRIVER_INIT( esb )
{
	/* init the slapstic */
	slapstic_init(101);
	slapstic_source = &memory_region(REGION_CPU1)[0x14000];
	slapstic_base = &memory_region(REGION_CPU1)[0x08000];

	/* install an opcode base handler */
	memory_set_opbase_handler(0, esb_setopbase);

	/* install read/write handlers for it */
	install_mem_read_handler(0, 0x8000, 0x9fff, esb_slapstic_r);
	install_mem_write_handler(0, 0x8000, 0x9fff, esb_slapstic_w);

	/* install additional banking */
	install_mem_read_handler(0, 0xa000, 0xffff, MRA_BANK2);

	/* prepare the mathbox */
	starwars_is_esb = 1;
	swmathbox_init();
}



/*************************************
 *
 *	Game drivers
 *
 *************************************/

GAME( 1983, starwars, 0,        starwars, starwars, starwars, ROT0, "Atari", "Star Wars (rev 2)" )
GAME( 1983, starwar1, starwars, starwars, starwars, starwars, ROT0, "Atari", "Star Wars (rev 1)" )
GAME( 1985, esb,      0,        starwars, esb,      esb,      ROT0, "Atari Games", "The Empire Strikes Back" )
