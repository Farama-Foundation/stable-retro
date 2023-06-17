/***************************************************************************

							-= Run Deep / The Deep =-

					driver by	Luca Elia (l.elia@tin.it)

Main CPU	:	Z80 (LH0080B @ 6MHz) + i8751 (Intel C8751H-88, protection)

Sound CPU	:	65C02 (R65C02P2 @ 2MHz)

Sound Chips	:	YM2203C

Video Chips	:	L7B0073 DATA EAST MXC 06 8746
				L7A0072 DATA EAST BAC 06 VAE8713

Board		:	DE-0298-1

Notes:

- The MCU handles coins and the bank switching of the roms for the main cpu.
  It additionally provides some z80 code that is copied to ram.

- One ROM (FI-1) is not used.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "thedeep.h"

/***************************************************************************

							Main CPU + MCU simulation

***************************************************************************/

static int nmi_enable;

WRITE_HANDLER( thedeep_nmi_w )
{
	nmi_enable = data;
}

static WRITE_HANDLER( thedeep_sound_w )
{
	soundlatch_w(0,data);
	cpu_set_nmi_line(1, PULSE_LINE);
}

static data8_t protection_command, protection_data;
static int protection_index, protection_irq;
static int rombank;

static MACHINE_INIT( thedeep )
{
	cpu_setbank(1, memory_region(REGION_CPU1) + 0x10000 + 0 * 0x4000);
	thedeep_scroll[0] = 0;
	thedeep_scroll[1] = 0;
	thedeep_scroll[2] = 0;
	thedeep_scroll[3] = 0;
	protection_command = 0;
	protection_index = -1;
	protection_irq = 0;
	rombank = -1;
}

static WRITE_HANDLER( thedeep_protection_w )
{
	protection_command = data;
	switch (protection_command)
	{
		case 0x11:
			flip_screen_set(1);
		break;

		case 0x20:
			flip_screen_set(0);
		break;

		case 0x30:
		case 0x31:
		case 0x32:
		case 0x33:
		{
			int new_rombank = protection_command & 3;
			if (rombank == new_rombank)	break;
			rombank = new_rombank;
			cpu_setbank(1, memory_region(REGION_CPU1) + 0x10000 + rombank * 0x4000);
			/* there's code which falls through from the fixed ROM to bank #1, I have to */
			/* copy it there otherwise the CPU bank switching support will not catch it. */
			memcpy(memory_region(REGION_CPU1) + 0x08000,memory_region(REGION_CPU1) + 0x10000 + rombank * 0x4000, 0x4000);
		}
		break;

		case 0x59:
		{
			if (protection_index < 0)
				protection_index = 0;

			if ( protection_index < 0x19b )
/* d000-d00c:	hl += a * b*/
/* d00d-d029:	input a (e.g. $39) output hl (e.g. h=$03 l=$09).*/
/* 				Replace trainling 0's with space ($10). 00 -> '  '*/
/* d02a-d039:	input a (e.g. $39) output hl (e.g. h=$03 l=$09).*/
/* 				Replace trainling 0's with space ($10). 00 -> ' 0'*/
/* d03a-d046:	input a (e.g. $39) output hl (e.g. h=$03 l=$09). 00 -> '00'*/
/* d047-d086:	a /= e (e can be 0!)*/
/* d087-d0a4:	print ASCII string from HL to IX (sub $30 to every char)*/
/* d0a4-d0be:	print any string from HL to IX*/
/* d0bf-d109:	print ASCII string from HL to IX. Color is in c. (e.g. "game over")*/
/* d10a-d11f:	print 2 digit decimal number in hl to ix, color c. change ix*/
/* d120-d157:	update score: add 3 BCD bytes at ix to those at iy, then clear those at ix*/
/* d158-d165:	print digit: (IX+0) <- H; (IX+1) <-L. ix+=40*/
/* d166-d174:	hl = (hl + 2*a)*/
/* d175-d181:	hl *= e (e must be non zero)*/
/* d182-d19a:	hl /= de*/
				protection_data = memory_region(REGION_CPU3)[0x185+protection_index++];
			else
				protection_data = 0xc9;

			protection_irq  = 1;
		}
		break;

		default:
			log_cb(RETRO_LOG_DEBUG, LOGPRE  "pc %04x: protection_command %02x\n", activecpu_get_pc(),protection_command);
	}
}

static READ_HANDLER( thedeep_e004_r )
{
	return protection_irq ? 1 : 0;
}

static READ_HANDLER( thedeep_protection_r )
{
	protection_irq = 0;
	return protection_data;
}

static WRITE_HANDLER( thedeep_e100_w )
{
	if (data != 1)
		log_cb(RETRO_LOG_DEBUG, LOGPRE "pc %04x: e100 = %02x\n", activecpu_get_pc(),data);
}

static MEMORY_READ_START( thedeep_readmem )
	{ 0x0000, 0x7fff, MRA_ROM				},	/* ROM*/
	{ 0x8000, 0xbfff, MRA_BANK1				},	/* ROM (banked)*/
	{ 0xc000, 0xcfff, MRA_RAM				},	/* RAM*/
	{ 0xd000, 0xdfff, MRA_RAM				},	/* RAM (MCU data copied here)*/
	{ 0xe000, 0xe000, thedeep_protection_r	},	/* From MCU*/
	{ 0xe004, 0xe004, thedeep_e004_r		},	/* ?*/
	{ 0xe008, 0xe008, input_port_0_r		},	/* P1 (Inputs)*/
	{ 0xe009, 0xe009, input_port_1_r		},	/* P2*/
	{ 0xe00a, 0xe00a, input_port_2_r		},	/* DSW1*/
	{ 0xe00b, 0xe00b, input_port_3_r		},	/* DSW2*/
	{ 0xe400, 0xe7ff, MRA_RAM				},	/* Sprites*/
	{ 0xe800, 0xefff, MRA_RAM				},	/* Text Layer*/
	{ 0xf000, 0xf7ff, MRA_RAM				},	/* Background Layer*/
	{ 0xf800, 0xf83f, MRA_RAM				},	/* Column Scroll*/
	{ 0xf840, 0xffff, MRA_RAM				},	/**/
MEMORY_END

static MEMORY_WRITE_START( thedeep_writemem )
	{ 0x0000, 0x7fff, MWA_ROM				},	/* ROM*/
	{ 0x8000, 0xbfff, MWA_ROM				},	/* ROM (banked)*/
	{ 0xc000, 0xcfff, MWA_RAM				},	/* RAM*/
	{ 0xd000, 0xdfff, MWA_RAM				},	/* RAM (MCU data copied here)*/
	{ 0xe000, 0xe000, thedeep_protection_w	},	/* To MCU*/
	{ 0xe004, 0xe004, thedeep_nmi_w			},	/**/
	{ 0xe00c, 0xe00c, thedeep_sound_w		},	/* To Sound CPU*/
	{ 0xe100, 0xe100, thedeep_e100_w		},	/* ?*/
	{ 0xe210, 0xe213, MWA_RAM, &thedeep_scroll				},	/* Scroll*/
	{ 0xe400, 0xe7ff, MWA_RAM, &spriteram, &spriteram_size	},	/* Sprites*/
	{ 0xe800, 0xefff, thedeep_vram_1_w, &thedeep_vram_1		},	/* Text Layer*/
	{ 0xf000, 0xf7ff, thedeep_vram_0_w, &thedeep_vram_0		},	/* Background Layer*/
	{ 0xf800, 0xf83f, MWA_RAM, &thedeep_scroll2				},	/* Column Scroll*/
	{ 0xf840, 0xffff, MWA_RAM								},	/**/
MEMORY_END

/***************************************************************************

									Sound CPU

***************************************************************************/

static MEMORY_READ_START( thedeep_sound_readmem )
	{ 0x0000, 0x07ff, MRA_RAM					},	/* RAM*/
	{ 0x3000, 0x3000, soundlatch_r				},	/* From Main CPU*/
	{ 0x8000, 0xffff, MRA_ROM					},	/* ROM*/
MEMORY_END

static MEMORY_WRITE_START( thedeep_sound_writemem )
	{ 0x0000, 0x07ff, MWA_RAM					},	/* RAM*/
	{ 0x0800, 0x0800, YM2203_control_port_0_w	},	/* YM2203*/
	{ 0x0801, 0x0801, YM2203_write_port_0_w		},	/**/
	{ 0x8000, 0xffff, MWA_ROM					},	/* ROM*/
MEMORY_END


/***************************************************************************

								Input Ports

***************************************************************************/

INPUT_PORTS_START( thedeep )
	PORT_START	/* IN0 - e008*/
	PORT_BIT(  0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    )	/* Up / down shown in service mode*/
	PORT_BIT(  0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  )
	PORT_BIT(  0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  )
	PORT_BIT(  0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT )
	PORT_BIT(  0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT(  0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT(  0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x80, IP_ACTIVE_LOW, IPT_START1  )

	PORT_START	/* IN1 - e009*/
	PORT_BIT(  0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER2 )
	PORT_BIT(  0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER2 )
	PORT_BIT(  0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER2 )
	PORT_BIT(  0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER2 )
	PORT_BIT(  0x10, IP_ACTIVE_LOW, IPT_BUTTON1        | IPF_PLAYER2 )
	PORT_BIT(  0x20, IP_ACTIVE_LOW, IPT_BUTTON2        | IPF_PLAYER2 )
	PORT_BIT(  0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x80, IP_ACTIVE_LOW, IPT_START2  )

	PORT_START	/* IN2 - e00a*/
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_4C ) )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( 1C_1C ) )
	PORT_BITX(    0x10, 0x10, IPT_DIPSWITCH_NAME | IPF_CHEAT, "Invulnerability", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, "Start Stage" )
	PORT_DIPSETTING(    0x20, "1" )
	PORT_DIPSETTING(    0x00, "4" )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START	/* IN3 - e00b*/
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x02, "Easy" )
	PORT_DIPSETTING(    0x03, "Normal" )
	PORT_DIPSETTING(    0x01, "Harder" )
	PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x0c, "3" )
	PORT_DIPSETTING(    0x08, "4" )
	PORT_DIPSETTING(    0x04, "5" )
	PORT_DIPSETTING(    0x00, "6" )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x00, "50k" )
	PORT_DIPSETTING(    0x30, "50k 70k" )
	PORT_DIPSETTING(    0x20, "60k 80k" )
	PORT_DIPSETTING(    0x10, "80k 100k" )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_SERVICE( 0x80, IP_ACTIVE_LOW )

	PORT_START	/* IN4 - read by the mcu*/
	PORT_BIT_IMPULSE( 0x01, IP_ACTIVE_HIGH, IPT_COIN1,    1 )
	PORT_BIT_IMPULSE( 0x02, IP_ACTIVE_HIGH, IPT_COIN2,    1 )
	PORT_BIT_IMPULSE( 0x04, IP_ACTIVE_HIGH, IPT_SERVICE1, 1 )
INPUT_PORTS_END


/***************************************************************************

								Graphics Layouts

***************************************************************************/

static struct GfxLayout layout_8x8x2 =
{
	8,8,
	RGN_FRAC(1,2),
	2,
	{ 0, 4 },
	{ STEP4(RGN_FRAC(1,2),1), STEP4(RGN_FRAC(0,2),1) },
	{ STEP8(0,8) },
	8*8
};

static struct GfxLayout layout_16x16x4 =
{
	16,16,
	RGN_FRAC(1,4),
	4,
	{ RGN_FRAC(0,4),RGN_FRAC(1,4),RGN_FRAC(2,4),RGN_FRAC(3,4) },
	{ STEP8(8*8*2,1), STEP8(0,1) },
	{ STEP16(0,8) },
	16*16
};

static struct GfxDecodeInfo thedeep_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &layout_16x16x4,	0x080,  8 }, /* [0] Sprites*/
	{ REGION_GFX2, 0, &layout_16x16x4,	0x100, 16 }, /* [1] Background Layer*/
	{ REGION_GFX3, 0, &layout_8x8x2,	0x000, 16 }, /* [2] Text Layer*/
	{ -1 }
};



/***************************************************************************

								Machine Drivers

***************************************************************************/

static void irqhandler(int irq)
{
	cpu_set_irq_line(1,0,irq ? ASSERT_LINE : CLEAR_LINE);
}

static struct YM2203interface thedeep_ym2203_intf =
{
	1,
	3000000,	/* ? */
	{ YM2203_VOL(100,100) },
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 },
	{ irqhandler }
};

static INTERRUPT_GEN( thedeep_interrupt )
{
	if (cpu_getiloops())
	{
		if (protection_command != 0x59)
		{
			int coins = readinputport(4);
			if		(coins & 1)	protection_data = 1;
			else if	(coins & 2)	protection_data = 2;
			else if	(coins & 4)	protection_data = 3;
			else				protection_data = 0;

			if (protection_data)
				protection_irq = 1;
		}
		if (protection_irq)
			cpu_set_irq_line(0, 0, HOLD_LINE);
	}
	else
	{
		if (nmi_enable)
		{
			cpu_set_nmi_line(0, ASSERT_LINE);
			cpu_set_nmi_line(0, CLEAR_LINE);
		}
	}
}

static MACHINE_DRIVER_START( thedeep )

	/* basic machine hardware */
	MDRV_CPU_ADD(Z80, 6000000)		/* 6MHz */
	MDRV_CPU_MEMORY(thedeep_readmem,thedeep_writemem)
	MDRV_CPU_VBLANK_INT(thedeep_interrupt,2)	/* IRQ by MCU, NMI by vblank (maskable) */

 	MDRV_CPU_ADD(M65C02, 2000000)	/* 2MHz */
	MDRV_CPU_MEMORY(thedeep_sound_readmem,thedeep_sound_writemem)
	/* IRQ by YM2203, NMI by when sound latch written by main cpu */

	/* CPU3 is a i8751 */

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_MACHINE_INIT(thedeep)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(0x100, 0xf8)
	MDRV_VISIBLE_AREA(0, 0x100-1, 0, 0xf8-1)
	MDRV_GFXDECODE(thedeep_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(512)

	MDRV_PALETTE_INIT(thedeep)
	MDRV_VIDEO_START(thedeep)
	MDRV_VIDEO_UPDATE(thedeep)

	/* sound hardware */
	MDRV_SOUND_ADD(YM2203, thedeep_ym2203_intf)
MACHINE_DRIVER_END



/***************************************************************************

								ROMs Loading

***************************************************************************/

/***************************************************************************

Here are the proms for The Deep!
NOTE: This game is Vertical.
I couldn't test this board so I don't know the manufakturer, sorry.
1 Z80
1 R6502
1 YM 2203
1 OSC 12 Mhz
1 OSC 8 Mhz
1 MPU 8751 (which is read-protected)



If you need more info or if this package doesn't
Work, mail me.

Visit The ROMLiST Homepage: Http://hem.passagen.se/ogg/

..............CaBBe!...................................

***************************************************************************/

ROM_START( thedeep )
	ROM_REGION( 0x20000, REGION_CPU1, 0 )		/* Z80 Code */
	ROM_LOAD( "dp-10.rom", 0x00000, 0x08000, CRC(7480b7a5) SHA1(ac6f121873a70c8077576322c201b7089c7b8a91) )
	ROM_LOAD( "dp-09.rom", 0x10000, 0x10000, CRC(c630aece) SHA1(809916a1ba1c8e0af4c228b0f26ac638e2abf81e) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )		/* 65C02 Code */
	ROM_LOAD( "dp-12.rom", 0x8000, 0x8000, CRC(c4e848c4) SHA1(d2dec5c8d7d59703f5485cab9124bf4f835fe728) )

	ROM_REGION( 0x1000, REGION_CPU3, 0 )		/* i8751 Code */
	ROM_LOAD( "dp-14", 0x0000, 0x1000, CRC(0b886dad) SHA1(487192764342f8b0a320d20a378bf94f84592da9) )	/* 1xxxxxxxxxxx = 0xFF*/

	ROM_REGION( 0x40000, REGION_GFX1, ROMREGION_DISPOSE )	/* Sprites */
	ROM_LOAD( "dp-08.rom", 0x00000, 0x10000, CRC(c5624f6b) SHA1(a3c0b13cddae760f30c7344d718cd69cad990054) )
	ROM_LOAD( "dp-07.rom", 0x10000, 0x10000, CRC(c76768c1) SHA1(e41ace1cb06ebe7f676b3b179b7dd01d00cf4d6a) )
	ROM_LOAD( "dp-06.rom", 0x20000, 0x10000, CRC(98adea78) SHA1(6a1af70de995a0a5e42fd395dd9454b7e2d9cb82) )
	ROM_LOAD( "dp-05.rom", 0x30000, 0x10000, CRC(76ea7dd1) SHA1(c29abb44a1182b47da749eeeb2db025ae3f28ea7) )

	ROM_REGION( 0x40000, REGION_GFX2, ROMREGION_DISPOSE )	/* 16x16x4 Background Layer */
	ROM_LOAD( "dp-03.rom", 0x00000, 0x10000, CRC(6bf5d819) SHA1(74079632d7c88ec22010c1a5bece0e36847fdab9) )
	ROM_LOAD( "dp-01.rom", 0x10000, 0x10000, CRC(e56be2fe) SHA1(25acc0f6d9cb5a727c9bac3e80aeb85a4727ddb0) )
	ROM_LOAD( "dp-04.rom", 0x20000, 0x10000, CRC(4db02c3c) SHA1(6284541372dec1113570cef31ca3c1a202fb4add) )
	ROM_LOAD( "dp-02.rom", 0x30000, 0x10000, CRC(1add423b) SHA1(b565340d719044ba2c428aab74f43f5a7cf7e2a3) )

	ROM_REGION( 0x4000, REGION_GFX3, ROMREGION_DISPOSE )	/* 8x8x2 Text Layer */
	ROM_LOAD( "dp-11.rom", 0x0000, 0x4000, CRC(196e23d1) SHA1(ed14e63fccb3e5dce462d9b8155e78749eaf9b3b) )

	ROM_REGION( 0x600, REGION_PROMS, ROMREGION_DISPOSE )	/* Colors */
	ROM_LOAD( "fi-1", 0x000, 0x200, CRC(f31efe09) SHA1(808c90fe02ed7b4000967c331b8773c4168b8a97) )	/* FIXED BITS (xxxxxx0xxxxxx0x0)*/
	ROM_LOAD( "fi-2", 0x200, 0x200, CRC(f305c8d5) SHA1(f82c709dc75a3c681d6f0ebf2702cb90110b1f0c) )	/* FIXED BITS (0000xxxx)*/
	ROM_LOAD( "fi-3", 0x400, 0x200, CRC(f61a9686) SHA1(24082f60b72268d240ceca6999bdf18872625cd2) )
ROM_END

ROM_START( rundeep )
	ROM_REGION( 0x20000, REGION_CPU1, 0 )		/* Z80 Code */
	ROM_LOAD( "3", 0x00000, 0x08000, CRC(c9c9e194) SHA1(e9552c3321585f0902f29b55a7de8e2316885713) )
	ROM_LOAD( "9", 0x10000, 0x10000, CRC(931f4e67) SHA1(f4942c5f0fdbcd6cdb96ddbbf2015f938b56b466) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )		/* 65C02 Code */
	ROM_LOAD( "dp-12.rom", 0x8000, 0x8000, CRC(c4e848c4) SHA1(d2dec5c8d7d59703f5485cab9124bf4f835fe728) )

	ROM_REGION( 0x1000, REGION_CPU3, 0 )		/* i8751 Code */
	ROM_LOAD( "dp-14", 0x0000, 0x1000, CRC(0b886dad) SHA1(487192764342f8b0a320d20a378bf94f84592da9) )	/* 1xxxxxxxxxxx = 0xFF*/

	ROM_REGION( 0x40000, REGION_GFX1, ROMREGION_DISPOSE )	/* Sprites */
	ROM_LOAD( "dp-08.rom", 0x00000, 0x10000, CRC(c5624f6b) SHA1(a3c0b13cddae760f30c7344d718cd69cad990054) )
	ROM_LOAD( "dp-07.rom", 0x10000, 0x10000, CRC(c76768c1) SHA1(e41ace1cb06ebe7f676b3b179b7dd01d00cf4d6a) )
	ROM_LOAD( "dp-06.rom", 0x20000, 0x10000, CRC(98adea78) SHA1(6a1af70de995a0a5e42fd395dd9454b7e2d9cb82) )
	ROM_LOAD( "dp-05.rom", 0x30000, 0x10000, CRC(76ea7dd1) SHA1(c29abb44a1182b47da749eeeb2db025ae3f28ea7) )

	ROM_REGION( 0x40000, REGION_GFX2, ROMREGION_DISPOSE )	/* 16x16x4 Background Layer */
	ROM_LOAD( "dp-03.rom", 0x00000, 0x10000, CRC(6bf5d819) SHA1(74079632d7c88ec22010c1a5bece0e36847fdab9) )
	ROM_LOAD( "dp-01.rom", 0x10000, 0x10000, CRC(e56be2fe) SHA1(25acc0f6d9cb5a727c9bac3e80aeb85a4727ddb0) )
	ROM_LOAD( "dp-04.rom", 0x20000, 0x10000, CRC(4db02c3c) SHA1(6284541372dec1113570cef31ca3c1a202fb4add) )
	ROM_LOAD( "dp-02.rom", 0x30000, 0x10000, CRC(1add423b) SHA1(b565340d719044ba2c428aab74f43f5a7cf7e2a3) )

	ROM_REGION( 0x4000, REGION_GFX3, ROMREGION_DISPOSE )	/* 8x8x2 Text Layer */
	ROM_LOAD( "11", 0x0000, 0x4000, CRC(5d29e4b9) SHA1(608345291062e9ce329ebe9a8c1e65d52e358785) )

	ROM_REGION( 0x600, REGION_PROMS, ROMREGION_DISPOSE )	/* Colors */
	ROM_LOAD( "fi-1", 0x000, 0x200, CRC(f31efe09) SHA1(808c90fe02ed7b4000967c331b8773c4168b8a97) )	/* FIXED BITS (xxxxxx0xxxxxx0x0)*/
	ROM_LOAD( "fi-2", 0x200, 0x200, CRC(f305c8d5) SHA1(f82c709dc75a3c681d6f0ebf2702cb90110b1f0c) )	/* FIXED BITS (0000xxxx)*/
	ROM_LOAD( "fi-3", 0x400, 0x200, CRC(f61a9686) SHA1(24082f60b72268d240ceca6999bdf18872625cd2) )
ROM_END

GAME( 1987, thedeep, 0,      thedeep, thedeep, 0, ROT270, "Woodplace Inc.", "The Deep (Japan)" )
GAME( 1988, rundeep, thedeep,thedeep, thedeep, 0, ROT270, "Cream",     "Run Deep" )
