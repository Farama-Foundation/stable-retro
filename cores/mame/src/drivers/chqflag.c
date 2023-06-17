/***************************************************************************

Chequered Flag / Checkered Flag (GX717) (c) Konami 1988

Notes:

- 007232 volume & panning control is almost certainly wrong.
- Needs HW tests or side-by-side tests to determine if the protection
  is 100% ok now;
- I've modified the YM2151 clock with an xtal of 2,on what I recall the
  music at the title screen should end when the words "Chequered Flag"
  flashes.Needs a comparison with a real PCB however. -AS

***************************************************************************/

#include "driver.h"
#include "cpu/z80/z80.h"
#include "cpu/konami/konami.h"
#include "vidhrdw/konamiic.h"

static int K051316_readroms;

static WRITE_HANDLER( k007232_extvolume_w );

/* from vidhrdw/chqflag.c */
VIDEO_START( chqflag );
VIDEO_UPDATE( chqflag );


static INTERRUPT_GEN( chqflag_interrupt )
{
	if (cpu_getiloops() == 0)
	{
		if (K051960_is_IRQ_enabled())
			cpu_set_irq_line(0, KONAMI_IRQ_LINE, HOLD_LINE);
	}
	else if (cpu_getiloops() % 2)
	{
		if (K051960_is_NMI_enabled())
			cpu_set_irq_line(0, IRQ_LINE_NMI, PULSE_LINE);
	}
}

static WRITE_HANDLER( chqflag_bankswitch_w )
{
	int bankaddress;
	unsigned char *RAM = memory_region(REGION_CPU1);

	/* bits 0-4 = ROM bank # (0x00-0x11) */
	bankaddress = 0x10000 + (data & 0x1f)*0x4000;
	cpu_setbank(4,&RAM[bankaddress]);

	/* bit 5 = memory bank select */
	if (data & 0x20){
		install_mem_read_handler(0, 0x1800, 0x1fff, paletteram_r);							/* palette */
	    install_mem_write_handler(0, 0x1800, 0x1fff, paletteram_xBBBBBGGGGGRRRRR_swap_w);	/* palette */
		if (K051316_readroms){
			install_mem_read_handler(0, 0x1000, 0x17ff, K051316_rom_0_r);	/* 051316 #1 (ROM test) */
			install_mem_write_handler(0, 0x1000, 0x17ff, K051316_0_w);		/* 051316 #1 */
		}
		else{
			install_mem_read_handler(0, 0x1000, 0x17ff, K051316_0_r);		/* 051316 #1 */
			install_mem_write_handler(0, 0x1000, 0x17ff, K051316_0_w);		/* 051316 #1 */
		}
	}
	else{
	    install_mem_read_handler(0, 0x1000, 0x17ff, MRA_BANK1);				/* RAM */
	    install_mem_write_handler(0, 0x1000, 0x17ff, MWA_BANK1);				/* RAM */
	    install_mem_read_handler(0, 0x1800, 0x1fff, MRA_BANK2);				/* RAM */
		install_mem_write_handler(0, 0x1800, 0x1fff, MWA_BANK2);				/* RAM */
	}

	/* other bits unknown/unused */
}

static WRITE_HANDLER( chqflag_vreg_w )
{
	static int last;

	/* bits 0 & 1 = coin counters */
	coin_counter_w(1,data & 0x01);
	coin_counter_w(0,data & 0x02);

	/* bit 4 = enable rom reading thru K051316 #1 & #2 */
	if ((K051316_readroms = (data & 0x10))){
	    install_mem_read_handler(0, 0x2800, 0x2fff, K051316_rom_1_r);	/* 051316 (ROM test) */
	}
	else{
		install_mem_read_handler(0, 0x2800, 0x2fff, K051316_1_r);		/* 051316 */
	}

	/* Bits 3-7 probably control palette dimming in a similar way to TMNT2/Sunset Riders, */
	/* however I don't have enough evidence to determine the exact behaviour. */
	/* Bits 3 and 7 are set in night stages, where the background should get darker and */
	/* the headlight (which have the shadow bit set) become highlights */
	/* Maybe one of the bits inverts the SHAD line while the other darkens the background. */
	if (data & 0x08)
		palette_set_shadow_factor(1/PALETTE_DEFAULT_SHADOW_FACTOR);
	else
		palette_set_shadow_factor(PALETTE_DEFAULT_SHADOW_FACTOR);

	if ((data & 0x80) != last)
	{
		double brt = (data & 0x80) ? PALETTE_DEFAULT_SHADOW_FACTOR : 1.0;
		int i;

		last = data & 0x80;

		/* only affect the background */
		for (i = 512;i < 1024;i++)
			palette_set_brightness(i,brt);
	}

/*if ((data & 0xf8) && (data & 0xf8) != 0x88)*/
/*	usrintf_showmessage("chqflag_vreg_w %02x",data);*/


	/* other bits unknown. bit 5 is used. */
}

static int analog_ctrl;

static WRITE_HANDLER( select_analog_ctrl_w )
{
	analog_ctrl = data;
}

static READ_HANDLER( analog_read_r )
{
	static int accel, wheel;

	switch (analog_ctrl & 0x03){
		case 0x00: return (accel = readinputport(5));	/* accelerator */
		case 0x01: return (wheel = readinputport(6));	/* steering */
		case 0x02: return accel;						/* accelerator (previous?) */
		case 0x03: return wheel;						/* steering (previous?) */
	}

	return 0xff;
}

WRITE_HANDLER( chqflag_sh_irqtrigger_w )
{
    soundlatch2_w(0, data);
	cpu_set_irq_line(1,0,HOLD_LINE);
}


/****************************************************************************/

static MEMORY_READ_START( chqflag_readmem )
	{ 0x0000, 0x0fff, MRA_RAM },					/* RAM */
	{ 0x1000, 0x17ff, MRA_BANK1 },					/* banked RAM (RAM/051316 (chip 1)) */
	{ 0x1800, 0x1fff, MRA_BANK2 },					/* palette + RAM */
	{ 0x2000, 0x2007, K051937_r },					/* Sprite control registers */
	{ 0x2400, 0x27ff, K051960_r },					/* Sprite RAM */
	{ 0x2800, 0x2fff, MRA_BANK3 },					/* 051316 zoom/rotation (chip 2) */
	{ 0x3100, 0x3100, input_port_0_r },				/* DIPSW #1  */
	{ 0x3200, 0x3200, input_port_3_r },				/* COINSW, STARTSW, test mode */
	{ 0x3201, 0x3201, input_port_2_r },				/* DIPSW #3, SW 4 */
	{ 0x3203, 0x3203, input_port_1_r },				/* DIPSW #2 */
	{ 0x3400, 0x341f, K051733_r },					/* 051733 (protection) */
	{ 0x3701, 0x3701, input_port_4_r },				/* Brake + Shift + ? */
	{ 0x3702, 0x3702, analog_read_r },				/* accelerator/wheel */
	{ 0x4000, 0x7fff, MRA_BANK4 },					/* banked ROM */
	{ 0x8000, 0xffff, MRA_ROM },					/* ROM */
MEMORY_END

static MEMORY_WRITE_START( chqflag_writemem )
	{ 0x0000, 0x0fff, MWA_RAM },					/* RAM */
	{ 0x1000, 0x17ff, MWA_BANK1 },					/* banked RAM (RAM/051316 (chip 1)) */
	{ 0x1800, 0x1fff, MWA_BANK2 },					/* palette + RAM */
	{ 0x2000, 0x2007, K051937_w },					/* Sprite control registers */
	{ 0x2400, 0x27ff, K051960_w },					/* Sprite RAM */
	{ 0x2800, 0x2fff, K051316_1_w },				/* 051316 zoom/rotation (chip 2) */
	{ 0x3000, 0x3000, soundlatch_w },				/* sound code # */
	{ 0x3001, 0x3001, chqflag_sh_irqtrigger_w },	/* cause interrupt on audio CPU */
	{ 0x3002, 0x3002, chqflag_bankswitch_w },		/* bankswitch control */
	{ 0x3003, 0x3003, chqflag_vreg_w },				/* enable K051316 ROM reading */
	{ 0x3300, 0x3300, watchdog_reset_w },			/* watchdog timer */
	{ 0x3400, 0x341f, K051733_w },					/* 051733 (protection) */
	{ 0x3500, 0x350f, K051316_ctrl_0_w },			/* 051316 control registers (chip 1) */
	{ 0x3600, 0x360f, K051316_ctrl_1_w },			/* 051316 control registers (chip 2) */
	{ 0x3700, 0x3700, select_analog_ctrl_w },		/* select accelerator/wheel */
	{ 0x3702, 0x3702, select_analog_ctrl_w },		/* select accelerator/wheel (mirror?) */
	{ 0x4000, 0x7fff, MWA_ROM },					/* banked ROM */
	{ 0x8000, 0xffff, MWA_ROM },					/* ROM */
MEMORY_END

static MEMORY_READ_START( chqflag_readmem_sound )
	{ 0x0000, 0x7fff, MRA_ROM },				/* ROM */
	{ 0x8000, 0x87ff, MRA_RAM },				/* RAM */
	{ 0xa000, 0xa00d, K007232_read_port_0_r },	/* 007232 (chip 1) */
	{ 0xb000, 0xb00d, K007232_read_port_1_r },	/* 007232 (chip 2) */
	{ 0xc001, 0xc001, YM2151_status_port_0_r },	/* YM2151 */
	{ 0xd000, 0xd000, soundlatch_r },			/* soundlatch_r */
	{ 0xe000, 0xe000, soundlatch2_r },		    /* engine sound volume */
MEMORY_END

static WRITE_HANDLER( k007232_bankswitch_w )
{
	int bank_A, bank_B;

	/* banks # for the 007232 (chip 1) */
	bank_A = ((data >> 4) & 0x03);
	bank_B = ((data >> 6) & 0x03);
	K007232_set_bank( 0, bank_A, bank_B );

	/* banks # for the 007232 (chip 2) */
	bank_A = ((data >> 0) & 0x03);
	bank_B = ((data >> 2) & 0x03);
	K007232_set_bank( 1, bank_A, bank_B );
}

static MEMORY_WRITE_START( chqflag_writemem_sound )
	{ 0x0000, 0x7fff, MWA_ROM },					/* ROM */
	{ 0x8000, 0x87ff, MWA_RAM },					/* RAM */
	{ 0x9000, 0x9000, k007232_bankswitch_w },		/* 007232 bankswitch */
	{ 0xa000, 0xa00d, K007232_write_port_0_w },		/* 007232 (chip 1) */
	{ 0xa01c, 0xa01c, k007232_extvolume_w },/* extra volume, goes to the 007232 w/ A11 */
											/* selecting a different latch for the external port */
	{ 0xb000, 0xb00d, K007232_write_port_1_w },		/* 007232 (chip 2) */
	{ 0xc000, 0xc000, YM2151_register_port_0_w },	/* YM2151 */
	{ 0xc001, 0xc001, YM2151_data_port_0_w },		/* YM2151 */
	{ 0xf000, 0xf000, MWA_NOP },					/* ??? */
MEMORY_END


INPUT_PORTS_START( chqflag )
	PORT_START	/* DSW #1 */
	PORT_DIPNAME( 0x0f, 0x0f, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x05, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 3C_2C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 4C_3C ) )
	PORT_DIPSETTING(    0x0f, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 3C_4C ) )
	PORT_DIPSETTING(    0x07, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x0e, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 2C_5C ) )
	PORT_DIPSETTING(    0x0d, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x0b, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0x0a, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(    0x09, DEF_STR( 1C_7C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0xf0, 0xf0, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x50, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 3C_2C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 4C_3C ) )
	PORT_DIPSETTING(    0xf0, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 3C_4C ) )
	PORT_DIPSETTING(    0x70, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0xe0, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x60, DEF_STR( 2C_5C ) )
	PORT_DIPSETTING(    0xd0, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0xb0, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0xa0, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(    0x90, DEF_STR( 1C_7C ) )
/*	PORT_DIPSETTING(    0x00, "Coin Slot 2 Invalidity" )*/

	PORT_START	/* DSW #2 (according to the manual SW1 thru SW5 are not used) */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unused ) )
	PORT_DIPSETTING(	0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unused ) )
	PORT_DIPSETTING(	0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unused ) )
	PORT_DIPSETTING(	0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unused ) )
	PORT_DIPSETTING(	0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unused ) )
	PORT_DIPSETTING(	0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x60, 0x40, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(	0x60, "Easy" )
	PORT_DIPSETTING(	0x40, "Normal" )
	PORT_DIPSETTING(	0x20, "Difficult" )
	PORT_DIPSETTING(	0x00, "Very difficult" )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(	0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )

	PORT_START
	PORT_BIT( 0x7f, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unused ) )	/* DIPSW #3 - SW4 */
	PORT_DIPSETTING(	0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )

	PORT_START
	/* COINSW + STARTSW */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	/* DIPSW #3 */
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unused ) )
	PORT_DIPSETTING(	0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, "Title" )
	PORT_DIPSETTING(	0x40, "Chequered Flag" )
	PORT_DIPSETTING(	0x00, "Checkered Flag" )
	PORT_SERVICE( 0x80, IP_ACTIVE_LOW )

	PORT_START	/* Brake, Shift + ??? */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_TOGGLE )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x0c, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* if this is set, it goes directly to test mode */
	PORT_BIT( 0xf0, IP_ACTIVE_HIGH, IPT_UNKNOWN )	/* if bit 7 == 0, the game resets */

	PORT_START	/* Accelerator */
	PORT_ANALOG( 0xff, 0x00, IPT_PEDAL, 50, 5, 0, 0xff )

	PORT_START	/* Driving wheel */
	PORT_ANALOG( 0xff, 0x80, IPT_AD_STICK_X | IPF_CENTER, 80, 8, 0x10, 0xef)
INPUT_PORTS_END



static void chqflag_ym2151_irq_w(int data)
{
    cpu_set_irq_line(1,IRQ_LINE_NMI, data ? ASSERT_LINE : CLEAR_LINE);
}


static struct YM2151interface ym2151_interface =
{
	1,
	3579545,	/* 3.579545 MHz */
	{ YM3012_VOL(80,MIXER_PAN_LEFT,80,MIXER_PAN_RIGHT) },
	{ chqflag_ym2151_irq_w },
	{ 0 }
};

static void volume_callback0(int v)
{
	K007232_set_volume(0,0,(v & 0x0f)*0x11,0);
	K007232_set_volume(0,1,0,(v >> 4)*0x11);
}

static WRITE_HANDLER( k007232_extvolume_w )
{
	K007232_set_volume(1,1,(data & 0x0f)*0x11/2,(data >> 4)*0x11/2);
}

static void volume_callback1(int v)
{
	K007232_set_volume(1,0,(v & 0x0f)*0x11/2,(v >> 4)*0x11/2);
}

static struct K007232_interface k007232_interface =
{
	2,															/* number of chips */
	3579545,	/* clock */
	{ REGION_SOUND1, REGION_SOUND2 },							/* memory regions */
	{ K007232_VOL(20,MIXER_PAN_CENTER,20,MIXER_PAN_CENTER),		/* volume */
		K007232_VOL(20,MIXER_PAN_LEFT,20,MIXER_PAN_RIGHT) },
	{ volume_callback0,  volume_callback1 }						/* external port callback */
};

static MACHINE_DRIVER_START( chqflag )

	/* basic machine hardware */
	MDRV_CPU_ADD(KONAMI,3000000)	/* 052001 */
	MDRV_CPU_MEMORY(chqflag_readmem,chqflag_writemem)
	MDRV_CPU_VBLANK_INT(chqflag_interrupt,16)	/* ? */

	MDRV_CPU_ADD(Z80, 3579545)
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)	/* ? */
	MDRV_CPU_MEMORY(chqflag_readmem_sound,chqflag_writemem_sound)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(10)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_HAS_SHADOWS)
	MDRV_SCREEN_SIZE(64*8, 32*8)
	MDRV_VISIBLE_AREA(12*8, (64-14)*8-1, 2*8, 30*8-1 )
	MDRV_PALETTE_LENGTH(1024)

	MDRV_VIDEO_START(chqflag)
	MDRV_VIDEO_UPDATE(chqflag)

	/* sound hardware */
	MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
	MDRV_SOUND_ADD(YM2151, ym2151_interface)
	MDRV_SOUND_ADD(K007232, k007232_interface)
MACHINE_DRIVER_END



ROM_START( chqflag )
	ROM_REGION( 0x58800, REGION_CPU1, 0 )	/* 052001 code */
	ROM_LOAD( "717h02",		0x050000, 0x008000, CRC(f5bd4e78) SHA1(7bab02152d055a6c3a322c88e7ee0b85a39d8ef2) )	/* banked ROM */
	ROM_CONTINUE(			0x008000, 0x008000 )				/* fixed ROM */
	ROM_LOAD( "717e10",		0x010000, 0x040000, CRC(72fc56f6) SHA1(433ea9a33f0230e046c731c70060f6a38db14ac7) )	/* banked ROM */
	/* extra memory for banked RAM */

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for the SOUND CPU */
	ROM_LOAD( "717e01",		0x000000, 0x008000, CRC(966b8ba8) SHA1(ab7448cb61fa5922b1d8ae5f0d0f42d734ed4f93) )

    ROM_REGION( 0x100000, REGION_GFX1, 0 )	/* graphics (addressable by the main CPU) */
	ROM_LOAD( "717e04",		0x000000, 0x080000, CRC(1a50a1cc) SHA1(bc16fab84c637ed124e37b115ddc0149560b727d) )	/* sprites */
	ROM_LOAD( "717e05",		0x080000, 0x080000, CRC(46ccb506) SHA1(3ed1f54744fc5cdc0f48e42f250c366267a8199a) )	/* sprites */

	ROM_REGION( 0x020000, REGION_GFX2, 0 )	/* graphics (addressable by the main CPU) */
	ROM_LOAD( "717e06",		0x000000, 0x020000, CRC(1ec26c7a) SHA1(05b5b522c5ebf5d0a71a7fc39ec9382008ef33c8) )	/* zoom/rotate (N16) */

	ROM_REGION( 0x100000, REGION_GFX3, 0 )	/* graphics (addressable by the main CPU) */
	ROM_LOAD( "717e07",		0x000000, 0x040000, CRC(b9a565a8) SHA1(a11782f7336e5ad58a4c6ea81f2eeac35d5e7d0a) )	/* zoom/rotate (L20) */
	ROM_LOAD( "717e08",		0x040000, 0x040000, CRC(b68a212e) SHA1(b2bd121a43552c3ade528ac763a0df40c3e648e0) )	/* zoom/rotate (L22) */
	ROM_LOAD( "717e11",		0x080000, 0x040000, CRC(ebb171ec) SHA1(d65d4a6b169ce03e4427b2a397484634f938236b) )	/* zoom/rotate (N20) */
	ROM_LOAD( "717e12",		0x0c0000, 0x040000, CRC(9269335d) SHA1(af298c8cff50d707d6abc806065f8e931f975dc0) )	/* zoom/rotate (N22) */

	ROM_REGION( 0x080000, REGION_SOUND1, 0 )	/* 007232 data (chip 1) */
	ROM_LOAD( "717e03",		0x000000, 0x080000, CRC(ebe73c22) SHA1(fad3334e5e91bf8d11b74ffdbbfd57567e6f6f8c) )

	ROM_REGION( 0x080000, REGION_SOUND2, 0 )	/* 007232 data (chip 2) */
	ROM_LOAD( "717e09",		0x000000, 0x080000, CRC(d74e857d) SHA1(00c851c857650d67fc4caccea4461d99be4acb3c) )
ROM_END

ROM_START( chqflagj )
	ROM_REGION( 0x58800, REGION_CPU1, 0 )	/* 052001 code */
	ROM_LOAD( "717j02.bin",	0x050000, 0x008000, CRC(05355daa) SHA1(130ddbc289c077565e44f33c63a63963e6417e19) )	/* banked ROM */
	ROM_CONTINUE(			0x008000, 0x008000 )				/* fixed ROM */
	ROM_LOAD( "717e10",		0x010000, 0x040000, CRC(72fc56f6) SHA1(433ea9a33f0230e046c731c70060f6a38db14ac7) )	/* banked ROM */
	/* extra memory for banked RAM */

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for the SOUND CPU */
	ROM_LOAD( "717e01",		0x000000, 0x008000, CRC(966b8ba8) SHA1(ab7448cb61fa5922b1d8ae5f0d0f42d734ed4f93) )

    ROM_REGION( 0x100000, REGION_GFX1, 0 )	/* graphics (addressable by the main CPU) */
	ROM_LOAD( "717e04",		0x000000, 0x080000, CRC(1a50a1cc) SHA1(bc16fab84c637ed124e37b115ddc0149560b727d) )	/* sprites */
	ROM_LOAD( "717e05",		0x080000, 0x080000, CRC(46ccb506) SHA1(3ed1f54744fc5cdc0f48e42f250c366267a8199a) )	/* sprites */

	ROM_REGION( 0x020000, REGION_GFX2, 0 )	/* graphics (addressable by the main CPU) */
	ROM_LOAD( "717e06",		0x000000, 0x020000, CRC(1ec26c7a) SHA1(05b5b522c5ebf5d0a71a7fc39ec9382008ef33c8) )	/* zoom/rotate (N16) */

	ROM_REGION( 0x100000, REGION_GFX3, 0 )	/* graphics (addressable by the main CPU) */
	ROM_LOAD( "717e07",		0x000000, 0x040000, CRC(b9a565a8) SHA1(a11782f7336e5ad58a4c6ea81f2eeac35d5e7d0a) )	/* zoom/rotate (L20) */
	ROM_LOAD( "717e08",		0x040000, 0x040000, CRC(b68a212e) SHA1(b2bd121a43552c3ade528ac763a0df40c3e648e0) )	/* zoom/rotate (L22) */
	ROM_LOAD( "717e11",		0x080000, 0x040000, CRC(ebb171ec) SHA1(d65d4a6b169ce03e4427b2a397484634f938236b) )	/* zoom/rotate (N20) */
	ROM_LOAD( "717e12",		0x0c0000, 0x040000, CRC(9269335d) SHA1(af298c8cff50d707d6abc806065f8e931f975dc0) )	/* zoom/rotate (N22) */

	ROM_REGION( 0x080000, REGION_SOUND1, 0 )	/* 007232 data (chip 1) */
	ROM_LOAD( "717e03",		0x000000, 0x080000, CRC(ebe73c22) SHA1(fad3334e5e91bf8d11b74ffdbbfd57567e6f6f8c) )

	ROM_REGION( 0x080000, REGION_SOUND2, 0 )	/* 007232 data (chip 2) */
	ROM_LOAD( "717e09",		0x000000, 0x080000, CRC(d74e857d) SHA1(00c851c857650d67fc4caccea4461d99be4acb3c) )
ROM_END



static DRIVER_INIT( chqflag )
{
	unsigned char *RAM = memory_region(REGION_CPU1);

	konami_rom_deinterleave_2(REGION_GFX1);
	paletteram = &RAM[0x58000];
}

GAMEX( 1988, chqflag,        0, chqflag, chqflag, chqflag, ROT90, "Konami", "Chequered Flag", GAME_UNEMULATED_PROTECTION | GAME_IMPERFECT_SOUND )
GAMEX( 1988, chqflagj, chqflag, chqflag, chqflag, chqflag, ROT90, "Konami", "Chequered Flag (Japan)", GAME_UNEMULATED_PROTECTION | GAME_IMPERFECT_SOUND )
