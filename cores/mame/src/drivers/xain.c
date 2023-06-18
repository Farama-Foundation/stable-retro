/***************************************************************************
Xain'd Sleena (TECHNOS), Solar Warrior (TAITO).
By Carlos A. Lozano & Rob Rosenbrock & Phil Stroffolino

	- MC68B09EP (2)
	- 6809EP (1)
	- 68705 (dump not available; patched out in the bootleg)
	- ym2203 (2)

TODO:
	- understand how the vblank bit really works
	- understand who triggers NMI and FIRQ on the first CPU
	- 68705 protection (currently patched out, causes partial missing sprites)

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "cpu/m6809/m6809.h"

static unsigned char *xain_sharedram;

VIDEO_UPDATE( xain );
VIDEO_START( xain );
WRITE_HANDLER( xain_scrollxP0_w );
WRITE_HANDLER( xain_scrollyP0_w );
WRITE_HANDLER( xain_scrollxP1_w );
WRITE_HANDLER( xain_scrollyP1_w );
WRITE_HANDLER( xain_charram_w );
WRITE_HANDLER( xain_bgram0_w );
WRITE_HANDLER( xain_bgram1_w );
WRITE_HANDLER( xain_flipscreen_w );

extern unsigned char *xain_charram, *xain_bgram0, *xain_bgram1;


static READ_HANDLER( xain_sharedram_r )
{
	return xain_sharedram[offset];
}

static WRITE_HANDLER( xain_sharedram_w )
{
	/* locations 003d and 003e are used as a semaphores between CPU A and B, */
	/* so let's resync every time they are changed to avoid deadlocks */
	if ((offset == 0x003d || offset == 0x003e)
			&& xain_sharedram[offset] != data)
		cpu_boost_interleave(0, TIME_IN_USEC(20));
	xain_sharedram[offset] = data;
}

static WRITE_HANDLER( xainCPUA_bankswitch_w )
{
	unsigned char *RAM = memory_region(REGION_CPU1);

	if (data & 0x08) {cpu_setbank(1,&RAM[0x10000]);}
	else {cpu_setbank(1,&RAM[0x4000]);}
}

static WRITE_HANDLER( xainCPUB_bankswitch_w )
{
	unsigned char *RAM = memory_region(REGION_CPU2);

	if (data & 0x01) {cpu_setbank(2,&RAM[0x10000]);}
	else {cpu_setbank(2,&RAM[0x4000]);}
}

static WRITE_HANDLER( xain_sound_command_w )
{
	soundlatch_w(offset,data);
	cpu_set_irq_line(2,M6809_IRQ_LINE,HOLD_LINE);
}

static WRITE_HANDLER( xain_irqA_assert_w )
{
	cpu_set_irq_line(0,M6809_IRQ_LINE,ASSERT_LINE);
}

static WRITE_HANDLER( xain_irqA_clear_w )
{
	cpu_set_irq_line(0,M6809_IRQ_LINE,CLEAR_LINE);
}

static WRITE_HANDLER( xain_firqA_clear_w )
{
	cpu_set_irq_line(0,M6809_FIRQ_LINE,CLEAR_LINE);
}

static WRITE_HANDLER( xain_irqB_assert_w )
{
	cpu_set_irq_line(1,M6809_IRQ_LINE,ASSERT_LINE);
}

static WRITE_HANDLER( xain_irqB_clear_w )
{
	cpu_set_irq_line(1,M6809_IRQ_LINE,CLEAR_LINE);
}

static READ_HANDLER( xain_68705_r )
{
/*	log_cb(RETRO_LOG_DEBUG, LOGPRE "read 68705\n");*/
	return 0x4d;	/* fake P5 checksum test pass */
}

static WRITE_HANDLER( xain_68705_w )
{
/*	log_cb(RETRO_LOG_DEBUG, LOGPRE "write %02x to 68705\n",data);*/
}

static INTERRUPT_GEN( xainA_interrupt )
{
	/* returning nmi on iloops() == 0 will cause lockups because the nmi handler */
	/* waits for the vblank bit to be clear and there are other places in the code */
	/* that wait for it to be set */
	if (cpu_getiloops() == 2)
		cpu_set_nmi_line(0,PULSE_LINE);
	else
		cpu_set_irq_line(0,M6809_FIRQ_LINE,ASSERT_LINE);
}



static MEMORY_READ_START( readmem )
	{ 0x0000, 0x1fff, xain_sharedram_r },
	{ 0x2000, 0x37ff, MRA_RAM },
	{ 0x3a00, 0x3a00, input_port_0_r },
	{ 0x3a01, 0x3a01, input_port_1_r },
	{ 0x3a02, 0x3a02, input_port_2_r },
	{ 0x3a03, 0x3a03, input_port_3_r },
	{ 0x3a04, 0x3a04, xain_68705_r },	/* from the 68705 */
	{ 0x3a05, 0x3a05, input_port_4_r },
/*	{ 0x3a06, 0x3a06, MRA_NOP },	 // ?? read (and discarded) on startup. Maybe reset the 68705 /*/
	{ 0x4000, 0x7fff, MRA_BANK1 },
	{ 0x8000, 0xffff, MRA_ROM },
MEMORY_END

static MEMORY_WRITE_START( writemem )
	{ 0x0000, 0x1fff, xain_sharedram_w, &xain_sharedram },
	{ 0x2000, 0x27ff, xain_charram_w, &xain_charram },
	{ 0x2800, 0x2fff, xain_bgram1_w, &xain_bgram1 },
	{ 0x3000, 0x37ff, xain_bgram0_w, &xain_bgram0 },
	{ 0x3800, 0x397f, MWA_RAM, &spriteram, &spriteram_size },
	{ 0x3a00, 0x3a01, xain_scrollxP1_w },
	{ 0x3a02, 0x3a03, xain_scrollyP1_w },
	{ 0x3a04, 0x3a05, xain_scrollxP0_w },
	{ 0x3a06, 0x3a07, xain_scrollyP0_w },
	{ 0x3a08, 0x3a08, xain_sound_command_w },
	{ 0x3a09, 0x3a09, MWA_NOP },	/* NMI acknowledge */
	{ 0x3a0a, 0x3a0a, xain_firqA_clear_w },
	{ 0x3a0b, 0x3a0b, xain_irqA_clear_w },
	{ 0x3a0c, 0x3a0c, xain_irqB_assert_w },
	{ 0x3a0d, 0x3a0d, xain_flipscreen_w },
	{ 0x3a0e, 0x3a0e, xain_68705_w },	/* to 68705 */
	{ 0x3a0f, 0x3a0f, xainCPUA_bankswitch_w },
	{ 0x3c00, 0x3dff, paletteram_xxxxBBBBGGGGRRRR_split1_w, &paletteram },
	{ 0x3e00, 0x3fff, paletteram_xxxxBBBBGGGGRRRR_split2_w, &paletteram_2 },
	{ 0x4000, 0xffff, MWA_ROM },
MEMORY_END

static MEMORY_READ_START( readmemB )
	{ 0x0000, 0x1fff, xain_sharedram_r },
	{ 0x4000, 0x7fff, MRA_BANK2 },
	{ 0x8000, 0xffff, MRA_ROM },
MEMORY_END

static MEMORY_WRITE_START( writememB )
	{ 0x0000, 0x1fff, xain_sharedram_w },
	{ 0x2000, 0x2000, xain_irqA_assert_w },
	{ 0x2800, 0x2800, xain_irqB_clear_w },
	{ 0x3000, 0x3000, xainCPUB_bankswitch_w },
	{ 0x4000, 0xffff, MWA_ROM },
MEMORY_END

static MEMORY_READ_START( sound_readmem )
	{ 0x0000, 0x07ff, MRA_RAM },
	{ 0x1000, 0x1000, soundlatch_r },
	{ 0x8000, 0xffff, MRA_ROM },
MEMORY_END

static MEMORY_WRITE_START( sound_writemem )
	{ 0x0000, 0x07ff, MWA_RAM },
	{ 0x2800, 0x2800, YM2203_control_port_0_w },
	{ 0x2801, 0x2801, YM2203_write_port_0_w },
	{ 0x3000, 0x3000, YM2203_control_port_1_w },
	{ 0x3001, 0x3001, YM2203_write_port_1_w },
	{ 0x4000, 0xffff, MWA_ROM },
MEMORY_END



INPUT_PORTS_START( xsleena )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START2 )

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN2 )

	PORT_START	/* DSW0 */
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 1C_3C ) )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_3C ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x10, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, "Allow Continue" )
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x80, DEF_STR( On ) )

	PORT_START	/* DSW1 */
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x03, "Easy" )
	PORT_DIPSETTING(    0x02, "Normal" )
	PORT_DIPSETTING(    0x01, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_DIPNAME( 0x0c, 0x0c, "Game Time" )
	PORT_DIPSETTING(    0x0c, "Slow" )
	PORT_DIPSETTING(    0x08, "Normal" )
	PORT_DIPSETTING(    0x04, "Fast" )
	PORT_DIPSETTING(    0x00, "Very Fast" )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x30, "20k 70k and every 70k" )
	PORT_DIPSETTING(    0x20, "30k 80k and every 80k" )
	PORT_DIPSETTING(    0x10, "20k and 80k" )
	PORT_DIPSETTING(    0x00, "30k and 80k" )
	PORT_DIPNAME( 0xc0, 0xc0, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0xc0, "3")
	PORT_DIPSETTING(    0x80, "4")
	PORT_DIPSETTING(    0x40, "6")
	PORT_BITX( 0,       0x00, IPT_DIPSWITCH_SETTING | IPF_CHEAT, "Infinite", IP_KEY_NONE, IP_JOY_NONE )

	PORT_START	/* IN2 */
	PORT_BIT( 0x03, IP_ACTIVE_LOW,  IPT_UNUSED )
	PORT_BIT( 0x04, IP_ACTIVE_LOW,  IPT_COIN3 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNKNOWN )	/* when 0, 68705 is ready to send data */
	PORT_BIT( 0x10, IP_ACTIVE_LOW,  IPT_UNKNOWN )	/* when 1, 68705 is ready to receive data */
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_VBLANK )
	PORT_BIT( 0xc0, IP_ACTIVE_LOW,  IPT_UNUSED )
INPUT_PORTS_END



static struct GfxLayout charlayout =
{
	8,8,	/* 8*8 chars */
	1024,	/* 1024 characters */
	4,	/* 4 bits per pixel */
	{ 0, 2, 4, 6 },	/* plane offset */
	{ 1, 0, 8*8+1, 8*8+0, 16*8+1, 16*8+0, 24*8+1, 24*8+0 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	32*8	/* every char takes 32 consecutive bytes */
};

static struct GfxLayout tilelayout =
{
	16,16,	/* 8*8 chars */
	4*512,	/* 512 characters */
	4,	/* 4 bits per pixel */
	{ 0x8000*4*8+0, 0x8000*4*8+4, 0, 4 },	/* plane offset */
	{ 3, 2, 1, 0, 16*8+3, 16*8+2, 16*8+1, 16*8+0,
	  32*8+3,32*8+2 ,32*8+1 ,32*8+0 ,48*8+3 ,48*8+2 ,48*8+1 ,48*8+0 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
	  8*8, 9*8, 10*8, 11*8, 12*8, 13*8, 14*8, 15*8 },
	64*8	/* every char takes 64 consecutive bytes */
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &charlayout,   0, 8 },	/* 8x8 text */
	{ REGION_GFX2, 0, &tilelayout, 256, 8 },	/* 16x16 Background */
	{ REGION_GFX3, 0, &tilelayout, 384, 8 },	/* 16x16 Background */
	{ REGION_GFX4, 0, &tilelayout, 128, 8 },	/* Sprites */
	{ -1 } /* end of array */
};



/* handler called by the 2203 emulator when the internal timers cause an IRQ */
static void irqhandler(int irq)
{
	cpu_set_irq_line(2,M6809_FIRQ_LINE,irq ? ASSERT_LINE : CLEAR_LINE);
}

static struct YM2203interface ym2203_interface =
{
	2,			/* 2 chips */
	3000000,	/* 3 MHz ??? */
	{ YM2203_VOL(40,50), YM2203_VOL(40,50) },
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 },
	{ irqhandler }
};



static MACHINE_DRIVER_START( xsleena )

	/* basic machine hardware */
	MDRV_CPU_ADD(M6809, 2000000)	/* 2 MHz ??? */
	MDRV_CPU_MEMORY(readmem,writemem)
	MDRV_CPU_VBLANK_INT(xainA_interrupt,4)	/* wrong, this is just a hack */
								/* IRQs are caused by CPU B */
								/* FIRQs are caused by ? */
								/* NMIs are caused by... vblank it seems, but it checks */
								/* the vblank bit before RTI, and there are other places in */
								/* the code that check that bit, so it would cause lockups */
	MDRV_CPU_ADD(M6809, 2000000)	/* 2 MHz ??? */
	MDRV_CPU_MEMORY(readmemB,writememB)

	MDRV_CPU_ADD(M6809, 2000000)
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)	/* 2 MHz ??? */
	MDRV_CPU_MEMORY(sound_readmem,sound_writemem)
								/* FIRQs are caused by the YM2203 */
	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(100)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 2*8, 30*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(512)

	MDRV_VIDEO_START(xain)
	MDRV_VIDEO_UPDATE(xain)

	/* sound hardware */
	MDRV_SOUND_ADD(YM2203, ym2203_interface)
MACHINE_DRIVER_END



/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( xsleena )
	ROM_REGION( 0x14000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "s-10.7d",      0x08000, 0x8000, CRC(370164be) SHA1(65c9951cac7dc3943fa4d5f9919ebb4c4f29b3ae) )
	ROM_LOAD( "s-11.7c",      0x04000, 0x4000, CRC(d22bf859) SHA1(9edb159bef2eba2c5d93c03c15fbcb87eea52236) )
	ROM_CONTINUE(             0x10000, 0x4000 )

	ROM_REGION( 0x14000, REGION_CPU2, 0 )	/* 64k for code */
	ROM_LOAD( "s-2.3b",       0x08000, 0x8000, CRC(a1a860e2) SHA1(fb2b152bfafc44608039774436ddf3b17eed979c) )
	ROM_LOAD( "s-1.2b",       0x04000, 0x4000, CRC(948b9757) SHA1(3ea840cc47ae6a66f3e5f6a2f3e88475dcfe1840) )
	ROM_CONTINUE(             0x10000, 0x4000 )

	ROM_REGION( 0x10000, REGION_CPU3, 0 )	/* 64k for code */
	ROM_LOAD( "s-3.4s",       0x8000, 0x8000, CRC(a5318cb8) SHA1(35fb28c5598e39f22552bb036ae356b78422f080) )

	ROM_REGION( 0x08000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "s-12.8b",      0x00000, 0x8000, CRC(83c00dd8) SHA1(8e9b19281039b63072270c7a63d9fb30cda570fd) ) /* chars */

	ROM_REGION( 0x40000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "s-21.16i",     0x00000, 0x8000, CRC(11eb4247) SHA1(5d2f1fa07b8fb1c6bebfdb02c39282d29813791b) ) /* tiles */
	ROM_LOAD( "s-22.15i",     0x08000, 0x8000, CRC(422b536e) SHA1(d5985c0bd1c840cb6f0da6b177a2caaff6db5a04) )
	ROM_LOAD( "s-23.14i",     0x10000, 0x8000, CRC(828c1b0c) SHA1(cb9b64073b0ade3885f61545191db4c445e3066b) )
	ROM_LOAD( "s-24.13i",     0x18000, 0x8000, CRC(d37939e0) SHA1(301d9f6720857c64a4e070444a07a38138ddd4ef) )
	ROM_LOAD( "s-13.16g",     0x20000, 0x8000, CRC(8f0aa1a7) SHA1(be3fdb6204b77dba28b14c5b880d65d7c1d6a161) )
	ROM_LOAD( "s-14.15g",     0x28000, 0x8000, CRC(45681910) SHA1(60c3eb4bc08bf11bf09bcd27549c6427fafbb1fb) )
	ROM_LOAD( "s-15.14g",     0x30000, 0x8000, CRC(a8eeabc8) SHA1(e5dc31df0b223b65144af3602be5bcb2ff9eebbd) )
	ROM_LOAD( "s-16.13g",     0x38000, 0x8000, CRC(e59a2f27) SHA1(4643cea85f8613c36b416f46f9d1753fa9839237) )

	ROM_REGION( 0x40000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "s-6.4h",       0x00000, 0x8000, CRC(5c6c453c) SHA1(68c0028d15da8f5e53f09e3d154d18cd9f219601) ) /* tiles */
	ROM_LOAD( "s-5.4l",       0x08000, 0x8000, CRC(59d87a9a) SHA1(f23cb9a9d6c6249a8a1f8e2acbc235086b008c7b) )
	ROM_LOAD( "s-4.4m",       0x10000, 0x8000, CRC(84884a2e) SHA1(5087010a72226e91a084a61b5089c110dba7e933) )
	/* 0x60000-0x67fff empty */
	ROM_LOAD( "s-7.4f",       0x20000, 0x8000, CRC(8d637639) SHA1(301a7893de8f1bb526f5075e2af8203b8af4b0d3) )
	ROM_LOAD( "s-8.4d",       0x28000, 0x8000, CRC(71eec4e6) SHA1(3417c52a39a6fc43c51ad707168180f54153177a) )
	ROM_LOAD( "s-9.4c",       0x30000, 0x8000, CRC(7fc9704f) SHA1(b6f353fb7fec58f68b9e28be2aa29146ac64ffd4) )
	/* 0x80000-0x87fff empty */

	ROM_REGION( 0x40000, REGION_GFX4, ROMREGION_DISPOSE )
	ROM_LOAD( "s-25.10i",     0x00000, 0x8000, CRC(252976ae) SHA1(534c9148d33e453f3541543a8c0eb4afc59c7de8) ) /* sprites */
	ROM_LOAD( "s-26.9i",      0x08000, 0x8000, CRC(e6f1e8d5) SHA1(2ee0227361d1f1358f5b5964dab7e691243cd9ae) )
	ROM_LOAD( "s-27.8i",      0x10000, 0x8000, CRC(785381ed) SHA1(95bf4eb29830c589a9793a4138e645e5b77f0c06) )
	ROM_LOAD( "s-28.7i",      0x18000, 0x8000, CRC(59754e3d) SHA1(d1781dbc83965fc84492f7282d6813507ba1e81b) )
	ROM_LOAD( "s-17.10g",     0x20000, 0x8000, CRC(4d977f33) SHA1(30b446ddb2f32354334ea780c435f2407d128808) )
	ROM_LOAD( "s-18.9g",      0x28000, 0x8000, CRC(3f3b62a0) SHA1(ab7e8f0ff707771401e679b6151ad0ea85cfc792) )
	ROM_LOAD( "s-19.8g",      0x30000, 0x8000, CRC(76641ee3) SHA1(8fba0fa6639e7bdfb3f7be5e945a55b64411d242) )
	ROM_LOAD( "s-20.7g",      0x38000, 0x8000, CRC(37671f36) SHA1(1494eec4ecde9ae1f1101aa13eb301b3f3d06602) )

	ROM_REGION( 0x0100, REGION_PROMS, 0 )
	ROM_LOAD( "mb7114e.59",   0x0000, 0x0100, CRC(fed32888) SHA1(4e9330456b20f7198c1e27ca1ae7200f25595599) )	/* timing? (not used) */
ROM_END

ROM_START( xsleenab )
	ROM_REGION( 0x14000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "1.rom",        0x08000, 0x8000, CRC(79f515a7) SHA1(e61f18e3639dd9afe16c7bcb90fa7be31905e2c6) )
	ROM_LOAD( "s-11.7c",      0x04000, 0x4000, CRC(d22bf859) SHA1(9edb159bef2eba2c5d93c03c15fbcb87eea52236) )
	ROM_CONTINUE(             0x10000, 0x4000 )

	ROM_REGION( 0x14000, REGION_CPU2, 0 )	/* 64k for code */
	ROM_LOAD( "s-2.3b",       0x08000, 0x8000, CRC(a1a860e2) SHA1(fb2b152bfafc44608039774436ddf3b17eed979c) )
	ROM_LOAD( "s-1.2b",       0x04000, 0x4000, CRC(948b9757) SHA1(3ea840cc47ae6a66f3e5f6a2f3e88475dcfe1840) )
	ROM_CONTINUE(             0x10000, 0x4000 )

	ROM_REGION( 0x10000, REGION_CPU3, 0 )	/* 64k for code */
	ROM_LOAD( "s-3.4s",       0x8000, 0x8000, CRC(a5318cb8) SHA1(35fb28c5598e39f22552bb036ae356b78422f080) )

	ROM_REGION( 0x08000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "s-12.8b",      0x00000, 0x8000, CRC(83c00dd8) SHA1(8e9b19281039b63072270c7a63d9fb30cda570fd) ) /* chars */

	ROM_REGION( 0x40000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "s-21.16i",     0x00000, 0x8000, CRC(11eb4247) SHA1(5d2f1fa07b8fb1c6bebfdb02c39282d29813791b) ) /* tiles */
	ROM_LOAD( "s-22.15i",     0x08000, 0x8000, CRC(422b536e) SHA1(d5985c0bd1c840cb6f0da6b177a2caaff6db5a04) )
	ROM_LOAD( "s-23.14i",     0x10000, 0x8000, CRC(828c1b0c) SHA1(cb9b64073b0ade3885f61545191db4c445e3066b) )
	ROM_LOAD( "s-24.13i",     0x18000, 0x8000, CRC(d37939e0) SHA1(301d9f6720857c64a4e070444a07a38138ddd4ef) )
	ROM_LOAD( "s-13.16g",     0x20000, 0x8000, CRC(8f0aa1a7) SHA1(be3fdb6204b77dba28b14c5b880d65d7c1d6a161) )
	ROM_LOAD( "s-14.15g",     0x28000, 0x8000, CRC(45681910) SHA1(60c3eb4bc08bf11bf09bcd27549c6427fafbb1fb) )
	ROM_LOAD( "s-15.14g",     0x30000, 0x8000, CRC(a8eeabc8) SHA1(e5dc31df0b223b65144af3602be5bcb2ff9eebbd) )
	ROM_LOAD( "s-16.13g",     0x38000, 0x8000, CRC(e59a2f27) SHA1(4643cea85f8613c36b416f46f9d1753fa9839237) )

	ROM_REGION( 0x40000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "s-6.4h",       0x00000, 0x8000, CRC(5c6c453c) SHA1(68c0028d15da8f5e53f09e3d154d18cd9f219601) ) /* tiles */
	ROM_LOAD( "s-5.4l",       0x08000, 0x8000, CRC(59d87a9a) SHA1(f23cb9a9d6c6249a8a1f8e2acbc235086b008c7b) )
	ROM_LOAD( "s-4.4m",       0x10000, 0x8000, CRC(84884a2e) SHA1(5087010a72226e91a084a61b5089c110dba7e933) )
	/* 0x60000-0x67fff empty */
	ROM_LOAD( "s-7.4f",       0x20000, 0x8000, CRC(8d637639) SHA1(301a7893de8f1bb526f5075e2af8203b8af4b0d3) )
	ROM_LOAD( "s-8.4d",       0x28000, 0x8000, CRC(71eec4e6) SHA1(3417c52a39a6fc43c51ad707168180f54153177a) )
	ROM_LOAD( "s-9.4c",       0x30000, 0x8000, CRC(7fc9704f) SHA1(b6f353fb7fec58f68b9e28be2aa29146ac64ffd4) )
	/* 0x80000-0x87fff empty */

	ROM_REGION( 0x40000, REGION_GFX4, ROMREGION_DISPOSE )
	ROM_LOAD( "s-25.10i",     0x00000, 0x8000, CRC(252976ae) SHA1(534c9148d33e453f3541543a8c0eb4afc59c7de8) ) /* sprites */
	ROM_LOAD( "s-26.9i",      0x08000, 0x8000, CRC(e6f1e8d5) SHA1(2ee0227361d1f1358f5b5964dab7e691243cd9ae) )
	ROM_LOAD( "s-27.8i",      0x10000, 0x8000, CRC(785381ed) SHA1(95bf4eb29830c589a9793a4138e645e5b77f0c06) )
	ROM_LOAD( "s-28.7i",      0x18000, 0x8000, CRC(59754e3d) SHA1(d1781dbc83965fc84492f7282d6813507ba1e81b) )
	ROM_LOAD( "s-17.10g",     0x20000, 0x8000, CRC(4d977f33) SHA1(30b446ddb2f32354334ea780c435f2407d128808) )
	ROM_LOAD( "s-18.9g",      0x28000, 0x8000, CRC(3f3b62a0) SHA1(ab7e8f0ff707771401e679b6151ad0ea85cfc792) )
	ROM_LOAD( "s-19.8g",      0x30000, 0x8000, CRC(76641ee3) SHA1(8fba0fa6639e7bdfb3f7be5e945a55b64411d242) )
	ROM_LOAD( "s-20.7g",      0x38000, 0x8000, CRC(37671f36) SHA1(1494eec4ecde9ae1f1101aa13eb301b3f3d06602) )

	ROM_REGION( 0x0100, REGION_PROMS, 0 )
	ROM_LOAD( "mb7114e.59",   0x0000, 0x0100, CRC(fed32888) SHA1(4e9330456b20f7198c1e27ca1ae7200f25595599) )	/* timing? (not used) */
ROM_END

ROM_START( solarwar )
	ROM_REGION( 0x14000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "p9-0.bin",     0x08000, 0x8000, CRC(8ff372a8) SHA1(0fc396e662419fb9cb5bea11748aa8e0e8d072e6) )
	ROM_LOAD( "pa-0.bin",     0x04000, 0x4000, CRC(154f946f) SHA1(25b776eb9c494e5302795ae79e494cbfc7c104b1) )
	ROM_CONTINUE(             0x10000, 0x4000 )

	ROM_REGION( 0x14000, REGION_CPU2, 0 )	/* 64k for code */
	ROM_LOAD( "p1-0.bin",     0x08000, 0x8000, CRC(f5f235a3) SHA1(9f57dd7c5e514afa750edc6da6d263bf1e913c14) )
	ROM_LOAD( "p0-0.bin",     0x04000, 0x4000, CRC(51ae95ae) SHA1(e03f7ccb0b33b05547577c60a7f92dc75e24b4d6) )
	ROM_CONTINUE(             0x10000, 0x4000 )

	ROM_REGION( 0x10000, REGION_CPU3, 0 )	/* 64k for code */
	ROM_LOAD( "s-3.4s",       0x8000, 0x8000, CRC(a5318cb8) SHA1(35fb28c5598e39f22552bb036ae356b78422f080) )

	ROM_REGION( 0x08000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "s-12.8b",      0x00000, 0x8000, CRC(83c00dd8) SHA1(8e9b19281039b63072270c7a63d9fb30cda570fd) ) /* chars */

	ROM_REGION( 0x40000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "s-21.16i",     0x00000, 0x8000, CRC(11eb4247) SHA1(5d2f1fa07b8fb1c6bebfdb02c39282d29813791b) ) /* tiles */
	ROM_LOAD( "s-22.15i",     0x08000, 0x8000, CRC(422b536e) SHA1(d5985c0bd1c840cb6f0da6b177a2caaff6db5a04) )
	ROM_LOAD( "s-23.14i",     0x10000, 0x8000, CRC(828c1b0c) SHA1(cb9b64073b0ade3885f61545191db4c445e3066b) )
	ROM_LOAD( "pn-0.bin",     0x18000, 0x8000, CRC(d2ed6f94) SHA1(155a0d1d978f07517400d0c602fc40657f8569dc) )
	ROM_LOAD( "s-13.16g",     0x20000, 0x8000, CRC(8f0aa1a7) SHA1(be3fdb6204b77dba28b14c5b880d65d7c1d6a161) )
	ROM_LOAD( "s-14.15g",     0x28000, 0x8000, CRC(45681910) SHA1(60c3eb4bc08bf11bf09bcd27549c6427fafbb1fb) )
	ROM_LOAD( "s-15.14g",     0x30000, 0x8000, CRC(a8eeabc8) SHA1(e5dc31df0b223b65144af3602be5bcb2ff9eebbd) )
	ROM_LOAD( "pf-0.bin",     0x38000, 0x8000, CRC(6e627a77) SHA1(1d16031acd53c9e691ae7eac8a6f1ae3954fac8c) )

	ROM_REGION( 0x40000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "s-6.4h",       0x00000, 0x8000, CRC(5c6c453c) SHA1(68c0028d15da8f5e53f09e3d154d18cd9f219601) ) /* tiles */
	ROM_LOAD( "s-5.4l",       0x08000, 0x8000, CRC(59d87a9a) SHA1(f23cb9a9d6c6249a8a1f8e2acbc235086b008c7b) )
	ROM_LOAD( "s-4.4m",       0x10000, 0x8000, CRC(84884a2e) SHA1(5087010a72226e91a084a61b5089c110dba7e933) )
	/* 0x60000-0x67fff empty */
	ROM_LOAD( "s-7.4f",       0x20000, 0x8000, CRC(8d637639) SHA1(301a7893de8f1bb526f5075e2af8203b8af4b0d3) )
	ROM_LOAD( "s-8.4d",       0x28000, 0x8000, CRC(71eec4e6) SHA1(3417c52a39a6fc43c51ad707168180f54153177a) )
	ROM_LOAD( "s-9.4c",       0x30000, 0x8000, CRC(7fc9704f) SHA1(b6f353fb7fec58f68b9e28be2aa29146ac64ffd4) )
	/* 0x80000-0x87fff empty */

	ROM_REGION( 0x40000, REGION_GFX4, ROMREGION_DISPOSE )
	ROM_LOAD( "s-25.10i",     0x00000, 0x8000, CRC(252976ae) SHA1(534c9148d33e453f3541543a8c0eb4afc59c7de8) )	/* sprites */
	ROM_LOAD( "s-26.9i",      0x08000, 0x8000, CRC(e6f1e8d5) SHA1(2ee0227361d1f1358f5b5964dab7e691243cd9ae) )
	ROM_LOAD( "s-27.8i",      0x10000, 0x8000, CRC(785381ed) SHA1(95bf4eb29830c589a9793a4138e645e5b77f0c06) )
	ROM_LOAD( "s-28.7i",      0x18000, 0x8000, CRC(59754e3d) SHA1(d1781dbc83965fc84492f7282d6813507ba1e81b) )
	ROM_LOAD( "s-17.10g",     0x20000, 0x8000, CRC(4d977f33) SHA1(30b446ddb2f32354334ea780c435f2407d128808) )
	ROM_LOAD( "s-18.9g",      0x28000, 0x8000, CRC(3f3b62a0) SHA1(ab7e8f0ff707771401e679b6151ad0ea85cfc792) )
	ROM_LOAD( "s-19.8g",      0x30000, 0x8000, CRC(76641ee3) SHA1(8fba0fa6639e7bdfb3f7be5e945a55b64411d242) )
	ROM_LOAD( "s-20.7g",      0x38000, 0x8000, CRC(37671f36) SHA1(1494eec4ecde9ae1f1101aa13eb301b3f3d06602) )

	ROM_REGION( 0x0100, REGION_PROMS, 0 )
	ROM_LOAD( "mb7114e.59",   0x0000, 0x0100, CRC(fed32888) SHA1(4e9330456b20f7198c1e27ca1ae7200f25595599) )	/* timing? (not used) */
ROM_END



DRIVER_INIT( xsleena )
{
	unsigned char *RAM = memory_region(REGION_CPU1);

	/* do the same patch as the bootleg xsleena */
	RAM[0xd488] = 0x12;
	RAM[0xd489] = 0x12;
	RAM[0xd48a] = 0x12;
	RAM[0xd48b] = 0x12;
	RAM[0xd48c] = 0x12;
	RAM[0xd48d] = 0x12;
}

DRIVER_INIT( solarwar )
{
	unsigned char *RAM = memory_region(REGION_CPU1);

	/* do the same patch as the bootleg xsleena */
	RAM[0xd47e] = 0x12;
	RAM[0xd47f] = 0x12;
	RAM[0xd480] = 0x12;
	RAM[0xd481] = 0x12;
	RAM[0xd482] = 0x12;
	RAM[0xd483] = 0x12;
}



GAME( 1986, xsleena,  0,       xsleena, xsleena, xsleena,  ROT0, "Technos", "Xain'd Sleena" )
GAME( 1986, xsleenab, xsleena, xsleena, xsleena, 0,        ROT0, "bootleg", "Xain'd Sleena (bootleg)" )
GAME( 1986, solarwar, xsleena, xsleena, xsleena, solarwar, ROT0, "[Technos] Taito (Memetron license)", "Solar-Warrior" )
