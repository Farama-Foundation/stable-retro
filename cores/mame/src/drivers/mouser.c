/*******************************************************************************

     Mouser

     Driver by Frank Palazzolo (palazzol@comcast.net)

	- This driver was done with only flyer shots to go by.
	- Colors are a good guess (might be perfect)
	- Clock and interrupt speeds for the sound CPU is a guess, but seem
	  reasonable, especially because the graphics seem to be synched
	- Sprite priorities are unknown

*******************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

unsigned char mouser_sound_byte;
unsigned char mouser_nmi_enable;

/* From "vidhrdw/mouser.c" */
PALETTE_INIT( mouser );
WRITE_HANDLER( mouser_flip_screen_x_w );
WRITE_HANDLER( mouser_flip_screen_y_w );
WRITE_HANDLER( mouser_spriteram_w );
WRITE_HANDLER( mouser_colorram_w );
VIDEO_UPDATE( mouser );

/* Mouser has external masking circuitry around
 * the NMI input on the main CPU */

WRITE_HANDLER( mouser_nmi_enable_w )
{
	mouser_nmi_enable = data;
}

INTERRUPT_GEN( mouser_nmi_interrupt )
{
	if ((mouser_nmi_enable & 1) == 1)
		nmi_line_pulse();
}

/* Sound CPU interrupted on write */

WRITE_HANDLER( mouser_sound_interrupt_w )
{
	mouser_sound_byte = data;
	cpu_set_irq_line(1, 0, PULSE_LINE);
}

READ_HANDLER( mouser_sound_byte_r )
{
	return mouser_sound_byte;
}

static MEMORY_READ_START( readmem )
	{ 0x0000, 0x5fff, MRA_ROM },
	{ 0x6000, 0x6bff, MRA_RAM },
	{ 0x9000, 0x93ff, videoram_r },
	{ 0xa000, 0xa000, input_port_0_r },
	{ 0xa800, 0xa800, input_port_1_r },
	{ 0xb000, 0xb000, input_port_2_r },
	{ 0xb800, 0xb800, input_port_3_r },
MEMORY_END


static MEMORY_WRITE_START( writemem )
	{ 0x0000, 0x5fff, MWA_ROM },
	{ 0x6000, 0x6bff, MWA_RAM },
	{ 0x8800, 0x88ff, MWA_NOP }, /* unknown */
	{ 0x9000, 0x93ff, videoram_w, &videoram, &videoram_size },
	{ 0x9800, 0x9cff, mouser_spriteram_w, &spriteram, &spriteram_size },
	{ 0x9c00, 0x9fff, mouser_colorram_w, &colorram },
	{ 0xa000, 0xa000, mouser_nmi_enable_w }, /* bit 0 = NMI Enable */
	{ 0xa001, 0xa001, mouser_flip_screen_x_w },
	{ 0xa002, 0xa002, mouser_flip_screen_y_w },
	{ 0xb800, 0xb800, mouser_sound_interrupt_w }, /* byte to sound cpu */

MEMORY_END


static MEMORY_READ_START( readmem2 )
	{ 0x0000, 0x1fff, MRA_ROM },
	{ 0x2000, 0x23ff, MRA_RAM },
	{ 0x3000, 0x3000, mouser_sound_byte_r },
MEMORY_END

static MEMORY_WRITE_START( writemem2 )
	{ 0x0000, 0x1fff, MWA_ROM },
	{ 0x2000, 0x23ff, MWA_RAM },
	{ 0x4000, 0x4000, MWA_NOP },	/* watchdog? */
MEMORY_END

static PORT_READ_START( readport2 )
PORT_END

static PORT_WRITE_START( writeport2 )
	{ 0x00, 0x00, AY8910_write_port_0_w },
	{ 0x01, 0x01, AY8910_control_port_0_w },
	{ 0x80, 0x80, AY8910_write_port_1_w },
	{ 0x81, 0x81, AY8910_control_port_1_w },
PORT_END

INPUT_PORTS_START( mouser )
    PORT_START
    PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_UNUSED )
    PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_UNUSED )
    PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNUSED )
    PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_BUTTON1 )
    PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP    | IPF_8WAY )
    PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN  | IPF_8WAY )
    PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_8WAY )
    PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY )

    PORT_START
    PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
    PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
    PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_START1 )
    PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START2 )
    PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNUSED )
    PORT_DIPNAME( 0x20, 0x00, DEF_STR( Difficulty ) )		/* guess ! - check code at 0x29a1 */
    PORT_DIPSETTING(    0x00, "Normal" )
    PORT_DIPSETTING(    0x20, "Hard" )
    PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED )
    PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )

    PORT_START
    PORT_DIPNAME( 0x03, 0x00, DEF_STR( Lives ) )
    PORT_DIPSETTING(	0x00, "3" )
    PORT_DIPSETTING(	0x01, "4" )
    PORT_DIPSETTING(	0x02, "5" )
    PORT_DIPSETTING(	0x03, "6" )
    PORT_DIPNAME( 0x0c, 0x00, DEF_STR( Bonus_Life ) )
    PORT_DIPSETTING(    0x00, "20000" )
    PORT_DIPSETTING(    0x04, "40000" )
    PORT_DIPSETTING(    0x08, "60000" )
    PORT_DIPSETTING(    0x0c, "80000" )
    PORT_DIPNAME( 0x70, 0x00, DEF_STR( Coinage ) )
    PORT_DIPSETTING(    0x70, DEF_STR( 5C_1C ) )
    PORT_DIPSETTING(    0x50, DEF_STR( 4C_1C ) )
    PORT_DIPSETTING(    0x30, DEF_STR( 3C_1C ) )
    PORT_DIPSETTING(    0x10, DEF_STR( 2C_1C ) )
    PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
    PORT_DIPSETTING(    0x20, DEF_STR( 1C_2C ) )
    PORT_DIPSETTING(    0x40, DEF_STR( 1C_3C ) )
    PORT_DIPSETTING(    0x60, DEF_STR( 1C_4C ) )
    PORT_DIPNAME( 0x80, 0x80, DEF_STR( Cabinet ) )
    PORT_DIPSETTING(    0x80, DEF_STR( Upright ) )
    PORT_DIPSETTING(    0x00, DEF_STR( Cocktail ) )

    PORT_START
    PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_UNUSED )
    PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_UNUSED )
    PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNUSED )
    PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_COCKTAIL )
    PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_COCKTAIL )
    PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_COCKTAIL )
    PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_COCKTAIL )
    PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
INPUT_PORTS_END



static struct GfxLayout charlayout =
{
	8,8,     /* 8*8 characters */
	1024,    /* 1024 characters */
	2,       /* 2 bits per pixel */
	{ 8192*8, 0 },
	{0, 1, 2, 3, 4, 5, 6, 7},
	{8*0, 8*1, 8*2, 8*3, 8*4, 8*5, 8*6, 8*7},
	8*8
};


static struct GfxLayout spritelayout =
{
	16,16,   /* 16*16 characters */
	64,      /* 64 sprites (2 banks) */
	2,       /* 2 bits per pixel */
	{ 8192*8, 0 },
	{0,  1,  2,  3,  4,  5,  6,  7,
	 64+0, 64+1, 64+2, 64+3, 64+4, 64+5, 64+6, 64+7},
	{8*0, 8*1, 8*2, 8*3, 8*4, 8*5, 8*6, 8*7,
	 128+8*0, 128+8*1, 128+8*2, 128+8*3, 128+8*4, 128+8*5, 128+8*6, 128+8*7},
	16*16
};


static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0x0000, &charlayout,       0, 16 },
	{ REGION_GFX1, 0x1000, &spritelayout,     0, 16 },
	{ REGION_GFX1, 0x1800, &spritelayout,     0, 16 },
	{ -1 } /* end of array */
};


static struct AY8910interface ay8910_interface =
{
	2,	/* 2 chips */
	4000000/2,	/* 2 MHz ? */
	{ 50, 50 },
	{ 0, 0 },
	{ 0, 0 },
	{ 0 },
	{ 0 },
	{ 0 }
};

static MACHINE_DRIVER_START( mouser )

	/* basic machine hardware */
	MDRV_CPU_ADD(Z80, 4000000)	/* 4 MHz ? */
	MDRV_CPU_MEMORY(readmem,writemem)
	MDRV_CPU_VBLANK_INT(mouser_nmi_interrupt,1) /* NMI is masked externally */

	MDRV_CPU_ADD(Z80, 4000000)	/* ??? */
	MDRV_CPU_MEMORY(readmem2,writemem2)
	MDRV_CPU_PORTS(readport2,writeport2)
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
	MDRV_CPU_VBLANK_INT(nmi_line_pulse,4) /* ??? This controls the sound tempo */

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 2*8, 30*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(64)

	MDRV_PALETTE_INIT(mouser)
	MDRV_VIDEO_START(generic)
	MDRV_VIDEO_UPDATE(mouser)

	/* sound hardware */
	MDRV_SOUND_ADD(AY8910, ay8910_interface)
MACHINE_DRIVER_END


ROM_START( mouser )
	ROM_REGION( 0x20000, REGION_CPU1, 0 ) /* 64K for data, 64K for encrypted opcodes */
	ROM_LOAD( "m0.5e",         0x0000, 0x2000, CRC(b56e00bc) SHA1(f3b23212590d91f1d19b1c7a98c560fbe5943185) )
	ROM_LOAD( "m1.5f",         0x2000, 0x2000, CRC(ae375d49) SHA1(8422f5a4d8560425f0c8612cf6f76029fcfe267c) )
	ROM_LOAD( "m2.5j",         0x4000, 0x2000, CRC(ef5817e4) SHA1(5cadc19f20fadf97c95852b280305fe4c75f1d19) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )
	ROM_LOAD( "m5.3v",         0x0000, 0x1000, CRC(50705eec) SHA1(252cea3498722318638f0c98ae929463ffd7d0d6) )

	ROM_REGION( 0x4000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "m3.11h",        0x0000, 0x2000, CRC(aca2834e) SHA1(c4f457fd8ea46386431ef8dffe54a232631870be) )
	ROM_LOAD( "m4.11k",        0x2000, 0x2000, CRC(943ab2e2) SHA1(ef9fc31dc8fe7a62f7bc6c817ce0d65091cb9a03) )

	/* Opcode Decryption PROMS */
	ROM_REGION( 0x0100, REGION_USER1, ROMREGION_DISPOSE )
	ROM_LOAD_NIB_HIGH( "bprom.4b",0x0000,0x0100,CRC(dd233851) SHA1(25eab1ec2227910c6fcd2803986f1cf206624da7) )
	ROM_LOAD_NIB_LOW(  "bprom.4c",0x0000,0x0100,CRC(60aaa686) SHA1(bb2ad555da51f6b30ab8b55833fe8d461a1e67f4) )

	ROM_REGION( 0x0040, REGION_PROMS, 0 )
	ROM_LOAD( "bprom.5v", 0x0000, 0x0020, CRC(7f8930b2) SHA1(8d0fe14b770fcf7088696d7b80d64507c6ee7364) )
	ROM_LOAD( "bprom.5u", 0x0020, 0x0020, CRC(0086feed) SHA1(b0b368e5fb7380cf09abd60c0933b405daf1c36a) )
ROM_END


ROM_START( mouserc )
	ROM_REGION( 0x20000, REGION_CPU1, 0 ) /* 64K for data, 64K for encrypted opcodes */
	ROM_LOAD( "83001.0",       0x0000, 0x2000, CRC(e20f9601) SHA1(f559a470784bda0bee9cab257a548238365acaa6) )
	ROM_LOAD( "m1.5f",         0x2000, 0x2000, CRC(ae375d49) SHA1(8422f5a4d8560425f0c8612cf6f76029fcfe267c) )	/* 83001.1*/
	ROM_LOAD( "m2.5j",         0x4000, 0x2000, CRC(ef5817e4) SHA1(5cadc19f20fadf97c95852b280305fe4c75f1d19) )	/* 83001.2*/

	ROM_REGION( 0x10000, REGION_CPU2, 0 )
	ROM_LOAD( "m5.3v",         0x0000, 0x1000, CRC(50705eec) SHA1(252cea3498722318638f0c98ae929463ffd7d0d6) )	/* 83001.5*/

	ROM_REGION( 0x4000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "m3.11h",        0x0000, 0x2000, CRC(aca2834e) SHA1(c4f457fd8ea46386431ef8dffe54a232631870be) )	/* 83001.3*/
	ROM_LOAD( "m4.11k",        0x2000, 0x2000, CRC(943ab2e2) SHA1(ef9fc31dc8fe7a62f7bc6c817ce0d65091cb9a03) )	/* 83001.4*/

	/* Opcode Decryption PROMS (originally from the UPL romset!) */
	ROM_REGION( 0x0100, REGION_USER1, ROMREGION_DISPOSE )
	ROM_LOAD_NIB_HIGH( "bprom.4b",0x0000,0x0100,CRC(dd233851) SHA1(25eab1ec2227910c6fcd2803986f1cf206624da7) )
	ROM_LOAD_NIB_LOW(  "bprom.4c",0x0000,0x0100,CRC(60aaa686) SHA1(bb2ad555da51f6b30ab8b55833fe8d461a1e67f4) )

	ROM_REGION( 0x0040, REGION_PROMS, 0 )
	ROM_LOAD( "bprom.5v", 0x0000, 0x0020, CRC(7f8930b2) SHA1(8d0fe14b770fcf7088696d7b80d64507c6ee7364) )	/* clr.5v*/
	ROM_LOAD( "bprom.5u", 0x0020, 0x0020, CRC(0086feed) SHA1(b0b368e5fb7380cf09abd60c0933b405daf1c36a) )	/* clr.5u*/
ROM_END


DRIVER_INIT( mouser )
{
	/* Decode the opcodes */

	offs_t i;
	data8_t *rom = memory_region(REGION_CPU1);
	offs_t diff = memory_region_length(REGION_CPU1) / 2;
	data8_t *table = memory_region(REGION_USER1);

	memory_set_opcode_base(0,rom+diff);

	for (i = 0;i < diff;i++)
	{
		rom[i + diff] = table[rom[i]];
	}
}


GAME( 1983, mouser,   0,      mouser, mouser, mouser, ROT90, "UPL", "Mouser" )
GAME( 1983, mouserc,  mouser, mouser, mouser, mouser, ROT90, "[UPL] (Cosmos license)", "Mouser (Cosmos)" )
