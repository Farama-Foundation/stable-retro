/***************************************************************************

Target Hits (c) 1994 Gaelco (Designed & Developed by Zigurat. Produced by Gaelco)

Driver by Manuel Abadia <manu@teleline.es>

The DS5002FP has 32KB undumped gameplay code making the game unplayable :_(

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "cpu/m68000/m68000.h"

extern data16_t *targeth_vregs;
extern data16_t *targeth_videoram;
extern data16_t *targeth_spriteram;

/* from vidhrdw/targeth.c */
WRITE16_HANDLER( targeth_vram_w );
VIDEO_START( targeth );
VIDEO_UPDATE( targeth );


static struct GfxLayout tilelayout16_0x080000 =
{
	16,16,														/* 16x16 tiles */
	0x080000/32,												/* number of tiles */
	4,															/* bitplanes */
	{ 3*0x080000*8, 2*0x080000*8, 1*0x080000*8, 0*0x080000*8 }, /* plane offsets */
	{ 0,1,2,3,4,5,6,7, 16*8+0,16*8+1,16*8+2,16*8+3,16*8+4,16*8+5,16*8+6,16*8+7 },
	{ 0*8,1*8,2*8,3*8,4*8,5*8,6*8,7*8, 8*8,9*8,10*8,11*8,12*8,13*8,14*8,15*8 },
	32*8
};

static struct GfxDecodeInfo gfxdecodeinfo_0x080000[] =
{
	{ REGION_GFX1, 0x000000, &tilelayout16_0x080000, 0, 64 },
	{ -1 }
};


static INTERRUPT_GEN(targeth_interrupt )
{
	switch(cpu_getiloops()){
		case 0: /* IRQ 2: drives the game */
			cpu_set_irq_line(0, 2, HOLD_LINE);
			break;
		case 1: /* IRQ 4: Read 1P Gun */
			cpu_set_irq_line(0, 4, HOLD_LINE);
			break;
		case 2:	/* IRQ 6: Read 2P Gun */
			cpu_set_irq_line(0, 6, HOLD_LINE);
			break;
	}
}


static MEMORY_READ16_START( targeth_readmem )
	{ 0x000000, 0x0fffff, MRA16_ROM },			/* ROM */
	{ 0x100000, 0x103fff, MRA16_RAM },			/* Video RAM */
	{ 0x108000, 0x108001, input_port_0_word_r },/* Gun 1P X */
	{ 0x108002, 0x108003, input_port_1_word_r },/* Gun 1P Y */
	{ 0x108004, 0x108005, input_port_2_word_r },/* Gun 2P X */
	{ 0x108006, 0x108007, input_port_3_word_r },/* Gun 2P Y */
	{ 0x200000, 0x2007ff, MRA16_RAM },			/* Palette */
	{ 0x440000, 0x440fff, MRA16_RAM },			/* Sprite RAM */
	{ 0x700000, 0x700001, input_port_4_word_r },/* DIPSW #2 */
	{ 0x700002, 0x700003, input_port_5_word_r },/* DIPSW #1 */
	{ 0x700006, 0x700007, input_port_6_word_r },/* Coins, Start & Fire buttons */
	{ 0x700008, 0x700009, input_port_7_word_r },/* Service & Guns Reload? */
	{ 0x70000e, 0x70000f, OKIM6295_status_0_lsb_r },/* OKI6295 status register */
	{ 0xfe0000, 0xfeffff, MRA16_RAM },			/* Work RAM (partially shared with DS5002FP) */
MEMORY_END

static WRITE16_HANDLER( OKIM6295_bankswitch_w )
{
	unsigned char *RAM = memory_region(REGION_SOUND1);

	if (ACCESSING_LSB){
		memcpy(&RAM[0x30000], &RAM[0x40000 + (data & 0x0f)*0x10000], 0x10000);
	}
}

static WRITE16_HANDLER( targeth_coin_counter_w )
{
	coin_counter_w( (offset >> 3) & 0x01, data & 0x01);
}

static MEMORY_WRITE16_START( targeth_writemem )
	{ 0x000000, 0x0fffff, MWA16_ROM },								/* ROM */
	{ 0x100000, 0x103fff, targeth_vram_w, &targeth_videoram },		/* Video RAM */
	{ 0x108000, 0x108007, MWA16_RAM, &targeth_vregs },				/* Video Registers */
	{ 0x10800c, 0x10800d, MWA16_NOP },								/* CLR Video INT */
	{ 0x200000, 0x2007ff, paletteram16_xBBBBBGGGGGRRRRR_word_w, &paletteram16 },/* Palette */
	{ 0x440000, 0x440fff, MWA16_RAM, &targeth_spriteram },			/* Sprite RAM */
	{ 0x70000c, 0x70000d, OKIM6295_bankswitch_w },					/* OKI6295 bankswitch */
	{ 0x70000e, 0x70000f, OKIM6295_data_0_lsb_w },					/* OKI6295 data register */
	{ 0x70000a, 0x70001b, MWA16_NOP },								/* ??? Guns reload related? */
	{ 0x70002a, 0x70003b, targeth_coin_counter_w },					/* Coin counters */
	{ 0xfe0000, 0xfeffff, MWA16_RAM },								/* Work RAM (partially shared with DS5002FP) */
MEMORY_END


INPUT_PORTS_START( targeth )
PORT_START	/* Gun 1 X */
	PORT_ANALOG( 0x01ff, 200, IPT_LIGHTGUN_X | IPF_PLAYER1, 100, 20, 0, 400 + 4)
	PORT_BIT( 0xfe00, IP_ACTIVE_HIGH, IPT_UNKNOWN )

PORT_START	/* Gun 1 Y */
	PORT_ANALOG( 0x01ff, 128, IPT_LIGHTGUN_Y | IPF_PLAYER1, 100, 20, 4, 255)
	PORT_BIT( 0xfe00, IP_ACTIVE_HIGH, IPT_UNKNOWN )

PORT_START	/* Gun 2 X */
	PORT_ANALOG( 0x01ff, 400 + 4, IPT_LIGHTGUN_X | IPF_PLAYER2, 100, 20, 0, 400 + 4)
	PORT_BIT( 0xfe00, IP_ACTIVE_HIGH, IPT_UNKNOWN )

PORT_START	/* Gun 2 Y */
	PORT_ANALOG( 0x01ff, 255, IPT_LIGHTGUN_Y | IPF_PLAYER2, 100, 20, 4, 255)
	PORT_BIT( 0xfe00, IP_ACTIVE_HIGH, IPT_UNKNOWN )

PORT_START	/* DSW #2 */
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x01, "Easy" )
	PORT_DIPSETTING(    0x03, "Normal" )
	PORT_DIPSETTING(    0x02, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x20, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, "Gun alarm" )	/* This doesn't work. What's supposed to do? */
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_SERVICE( 0x80, IP_ACTIVE_LOW )

PORT_START	/* DSW #1 */
	PORT_DIPNAME( 0x07, 0x07, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 6C_1C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x05, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 3C_2C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 4C_3C ) )
	PORT_DIPSETTING(    0x07, DEF_STR( 1C_1C ) )
	PORT_DIPNAME( 0x38, 0x38, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x38, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 3C_4C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x28, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x18, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 1C_6C ) )
	PORT_DIPNAME( 0x40, 0x40, "Credit configuration" )
	PORT_DIPSETTING(    0x40, "Start 1C/Continue 1C" )
	PORT_DIPSETTING(    0x00, "Start 2C/Continue 1C" )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Free_Play ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

PORT_START	/* Button 1, COINSW & STARTSW */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START2 )

PORT_START	/* Service & Button 2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* this MUST be low or the game doesn't boot */
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )	/* Reload 1P? */
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )	/* Reload 2P? */
	PORT_BIT( 0xf0, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END


static struct OKIM6295interface targeth_okim6295_interface =
{
	1,                  /* 1 chip */
	{ 8000 },			/* 8000 KHz? */
	{ REGION_SOUND1 },  /* memory region */
	{ 100 }				/* volume */
};

static MACHINE_DRIVER_START( targeth )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000,24000000/2)			/* 12 MHz */
	MDRV_CPU_MEMORY(targeth_readmem,targeth_writemem)
	MDRV_CPU_VBLANK_INT(targeth_interrupt,3)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(64*16, 32*16)				/* 1024x512 */
	MDRV_VISIBLE_AREA(0, 24*16-1, 16, 16*16-1)	/* 400x240 */
	MDRV_GFXDECODE(gfxdecodeinfo_0x080000)
	MDRV_PALETTE_LENGTH(1024)

	MDRV_VIDEO_START(targeth)
	MDRV_VIDEO_UPDATE(targeth)

	/* sound hardware */
	MDRV_SOUND_ADD(OKIM6295, targeth_okim6295_interface)
MACHINE_DRIVER_END

ROM_START( targeth )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )	/* 68000 code */
	ROM_LOAD16_BYTE(	"targeth.c23",	0x000000, 0x040000, CRC(840887d6) SHA1(9a36b346608d531a62a2e0704ea44f12e07f9d91) )
	ROM_LOAD16_BYTE(	"targeth.c22",	0x000001, 0x040000, CRC(d2435eb8) SHA1(ce75a115dad8019c8e66a1c3b3e15f54781f65ae) )

	ROM_REGION( 0x200000, REGION_GFX1, ROMREGION_DISPOSE )	/* Graphics */
	ROM_LOAD( "targeth.i13",	0x000000, 0x080000, CRC(b892be24) SHA1(9cccaaacf20e77c7358f0ceac60b8a1012f1216c) )
	ROM_LOAD( "targeth.i11",	0x080000, 0x080000, CRC(6797faf9) SHA1(112cffe72f91cb46c262e19a47b0cab3237dd60f) )
	ROM_LOAD( "targeth.i9",		0x100000, 0x080000, CRC(0e922c1c) SHA1(6920e345c82e76f7e0af6101f39eb65ac1f112b9) )
	ROM_LOAD( "targeth.i7",		0x180000, 0x080000, CRC(d8b41000) SHA1(cbe91eb91bdc7a60b2333c6bea37d08a57902669) )

	ROM_REGION( 0x140000, REGION_SOUND1, 0 )	/* ADPCM samples - sound chip is OKIM6295 */
	ROM_LOAD( "targeth.c1",		0x000000, 0x080000, CRC(d6c9dfbc) SHA1(3ec70dea94fc89df933074012a52de6034571e87) )
	/* 0x00000-0x2ffff is fixed, 0x30000-0x3ffff is bank switched from all the ROMs */
	ROM_RELOAD(					0x040000, 0x080000 )
	ROM_LOAD( "targeth.c3",		0x0c0000, 0x080000, CRC(d4c771df) SHA1(7cc0a86ef6aa3d26ab8f19d198f62112bf012870) )
ROM_END

GAMEX( 1994, targeth, 0, targeth,targeth, 0, ROT0, "Gaelco", "Target Hits", GAME_UNEMULATED_PROTECTION )
