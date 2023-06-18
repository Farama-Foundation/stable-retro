/****************************************************************\
* Status Trivia 2 driver by David Haywood, MooglyGuy, and Stephh *
* Super Trivia driver by MooglyGuy                               *
* Triv Quiz driver by MooglyGuy                                  *
*                                                                *
******************************************************************
*                                                                *
* All notes by David Haywood unless otherwise noted              *
*                                                                *
* Thanks to MooglyGuy for working out why statriv2 was crashing  *
* in attract and fixing the questions so it actually asked more  *
* than one per category.                                         *
*                                                                *
* Game Speed too fast?                                           *
*                                                                *
* MG: Dave seems to think that the AY is hooked up wrong since   *
*     it generates lots of errors in error.log, even though the  *
*     sound seems to make sense. Can someone with a PCB stomach  *
*     the game long enough to verify one way or the other?       *
*                                                                *
\****************************************************************/

/****************************************************************\
*                                                                *
*                       1982 status game corp                    *
*                         8085  12.4 MHz                         *
* 8910 crt5037 8255                                              *
*                                                                *
*         u36            u7 u8 u9                                *
*                                 5101    2114                   *
*                                 5101    2114                   *
*           2114 2114 2114 2114                                  *
*                                u12(pb)                         *
*                                                                *
* triv-quiz II (pb plugs into u12)                               *
*                                                                *
* 74244  74161 74161 74161 74161  74138 7402  74139              *
* u1 u2 u3 u4 u5 u6 u7 u8                                        *
*                                                                *
*                                                                *
\****************************************************************/


#include "driver.h"
#include "cpu/i8085/i8085.h"

/* Default NVram, we seem to need one or statriv2 crashes during attract
   attempting to display an unterminated message */

unsigned char statriv2_default_eeprom[256] = {
	0x24,0x00,0x02,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x02,0x00,0x00,0x00,0x00,0x00,0x00,0x06,0x00,0x00,0x11,0x00,
	0x00,0x00,0x00,0x00,0x01,0x02,0x01,0x05,0x00,0x00,0x11,0x49,0x41,0x41,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x11,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x02,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x02,0x8D,0x03,0x00,0x50,
	0x17,0x00,0x00,0x01,0xB5,0xAC,0x00,0x00,0x00,0x00,0x20,0x20,0x20,0x20,0x20,0x20,
	0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x80,0x20,0x20,0x20,0x20,0x20,0x20,0x20,
	0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x80,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,
	0x20,0x20,0x20,0x20,0x20,0x20,0x80,0xFF,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
};


data8_t *statriv2_videoram;
static struct tilemap* statriv2_tilemap;

/* Video Related, move to vidhrdw later */

static void get_statriv2_tile_info(int tile_index)
{
	int code = statriv2_videoram[0x400+tile_index];
	int attr = statriv2_videoram[tile_index];

	SET_TILE_INFO(
			0,
			code,
			attr,
			0)
}


WRITE_HANDLER( statriv2_videoram_w )
{
	statriv2_videoram[offset] = data;
	tilemap_mark_tile_dirty(statriv2_tilemap,offset & 0x3ff);
}


VIDEO_START (statriv2)
{
	statriv2_tilemap = tilemap_create(get_statriv2_tile_info,tilemap_scan_rows,TILEMAP_OPAQUE,8,16,64, 16);
	if(!statriv2_tilemap)
		return 1;
	return 0;
}

VIDEO_UPDATE (statriv2)
{
	tilemap_draw(bitmap,cliprect,statriv2_tilemap,0,0);
}

PALETTE_INIT(statriv2)
{
	int i;

	for (i = 0; i < 8; i++)
	{
		palette_set_color(i,
			0xff * ((i >> 2) & 1),
			0xff * ((i >> 0) & 1),
			0xff * ((i >> 1) & 1));
	}

	for (i = 0; i < 64; i++)
	{
		colortable[2*i+0] = i % 8;
		colortable[2*i+1] = i / 8;
	}
}

/* end video related */

static NVRAM_HANDLER (statriv2)
{
	if (read_or_write)
		mame_fwrite(file, generic_nvram, generic_nvram_size);
	else if (file)
		mame_fread(file, generic_nvram, generic_nvram_size);
	else
		memcpy ( generic_nvram, statriv2_default_eeprom, 0x100 );
}


static data8_t  question_offset_low;
static data8_t  question_offset_med;
static data8_t  question_offset_high;

static WRITE_HANDLER ( question_offset_low_w )
{
	question_offset_low = data;
}

static WRITE_HANDLER ( question_offset_med_w )
{
	question_offset_med = data;
}

static WRITE_HANDLER ( question_offset_high_w )
{
	question_offset_high = data;
}

static READ_HANDLER (statriv2_questions_read)
{
	data8_t *question_data    = memory_region       ( REGION_USER1 );
	int offs;

	question_offset_low++;
	offs = (question_offset_high << 8) | question_offset_low;
	return question_data[offs];
}

/***************************************************\
*                                                   *
* Super Trivia has some really weird protection on  *
* its question data. For some odd reason, the data  *
* itself is stored normally. Just load the ROMs up  *
* in a hex editor and OR everything with 0x40 to    *
* get normal text. However, the game itself expects *
* different data than what the question ROMs        *
* contain. Here is some pseudocode for what the     *
* game does for each character:                     *
*                                                   *
*     GetCharacter:                                 *
*     In A,($28)             // Read character in   *
*     Invert A               // Invert the bits     *
*     AND A,$1F              // Put low 5 bits of   *
*     B = Low 8 bits of addy // addy into high 8    *
*     C = 0                  // bits of BC pair     *
*     Call ArcaneFormula(BC) // Get XOR value       *
*     XOR A,C                // Apply it            *
*     Return                                        *
*                                                   *
*     ArcaneFormula(BC):                            *
*     ShiftR BC,1                                   *
*     DblShiftR BC,1                                *
*     DblShiftR BC,1                                *
*     DblShiftR BC,1                                *
*     ShiftR BC,1                                   *
*     Return                                        *
*                                                   *
* Essentially what ArcaneFormula does is to "fill   *
* out" an entire 8 bit number from only five bits.  *
* The way it does this is by putting bit 0 of the 5 *
* bits into bit 0 of the 8 bits, putting bit 1 into *
* bits 1 and 2, bit 2 into bits 3 and 4, bit 3 into *
* bits 5 and 6, and finally, bit 4 into bit         *
* position 7 of the 8-bit number. For example, for  *
* a value of FA, these would be the steps to get    *
* the XOR value:                                    *
*                                                   *
*                                 Address  XOR val  *
*     1: Take original number     11111010 00000000 *
*     2: XOR with 0x1F            00011010 00000000 *
*     3: Put bit 0 in bit 0       0001101- 00000000 *
*     4: Double bit 1 in bits 1,2 000110-0 00000110 *
*     5: Double bit 2 in bits 3,4 00011-10 00000110 *
*     6: Double bit 3 in bits 5,6 0001-010 01100110 *
*     7: Put bit 4 in bit 7       000-1010 11100110 *
*                                                   *
* Since XOR operations are symmetrical, to make the *
* game end up receiving the correct value one only  *
* needs to invert the value and XOR it with the     *
* value derived from its address. The game will     *
* then de-invert the value when it tries to invert  *
* it, re-OR the value when it tries to XOR it, and  *
* we wind up with nice, working questions. If       *
* anyone can figure out a way to simplify the       *
* formula I'm using, PLEASE DO SO!                  *
*                                                   *
*                                       - MooglyGuy *
*                                                   *
\***************************************************/

static READ_HANDLER (supertr2_questions_read)
{
	data8_t *question_data = memory_region( REGION_USER1 );
	int offs;
	int XORval;

	XORval = question_offset_low & 0x01;
	XORval |= (question_offset_low & 0x02) * 3;
	XORval |= ((question_offset_low & 0x04) * 3) << 1;
	XORval |= ((question_offset_low & 0x08) * 3) << 2;
	XORval |= (question_offset_low & 0x10) << 3;

	offs = (question_offset_high << 16) | (question_offset_med << 8) | question_offset_low;

	return (question_data[offs] ^ 0xFF) ^ XORval;
}

static MEMORY_READ_START( statriv2_readmem )
	{ 0x0000, 0x2fff, MRA_ROM },
	{ 0x4000, 0x43ff, MRA_RAM },
	{ 0x4800, 0x48ff, MRA_RAM },
	{ 0xc800, 0xcfff, MRA_RAM },
MEMORY_END

static MEMORY_WRITE_START( statriv2_writemem )
	{ 0x0000, 0x2fff, MWA_ROM },
	{ 0x4000, 0x43ff, MWA_RAM },
	{ 0x4800, 0x48ff, MWA_RAM, &generic_nvram, &generic_nvram_size },    /* backup ram?*/
	{ 0xc800, 0xcfff, statriv2_videoram_w, &statriv2_videoram },
MEMORY_END

static MEMORY_READ_START( supertr2_readmem )
	{ 0x0000, 0x3fff, MRA_ROM },
	{ 0x4000, 0x43ff, MRA_RAM },
	{ 0x4800, 0x48ff, MRA_RAM },
	{ 0xc800, 0xcfff, MRA_RAM },
MEMORY_END

static MEMORY_WRITE_START( supertr2_writemem )
	{ 0x0000, 0x3fff, MWA_ROM },
	{ 0x4000, 0x43ff, MWA_RAM },
	{ 0x4800, 0x48ff, MWA_RAM, &generic_nvram, &generic_nvram_size },    /* backup ram?*/
	{ 0xc800, 0xcfff, statriv2_videoram_w, &statriv2_videoram },
MEMORY_END

static PORT_READ_START( statriv2_readport )
	{ 0x20, 0x20, input_port_0_r },
	{ 0x21, 0x21, input_port_1_r },
	{ 0x2b, 0x2b, statriv2_questions_read },		/* question data*/
	{ 0xb1, 0xb1, AY8910_read_port_0_r },		/* ???*/
	{ 0xce, 0xce, IORP_NOP },				/* ???*/
PORT_END

static PORT_WRITE_START( statriv2_writeport )
	{ 0x22, 0x22, IOWP_NOP },				/* ???*/
	{ 0x23, 0x23, IOWP_NOP },				/* ???*/
	{ 0x29, 0x29, question_offset_low_w },
	{ 0x2a, 0x2a, question_offset_high_w },
	{ 0xb0, 0xb0, AY8910_control_port_0_w },
	{ 0xb1, 0xb1, AY8910_write_port_0_w },
	{ 0xc0, 0xcf, IOWP_NOP },				/* ???*/
PORT_END

static PORT_READ_START( supertr2_readport )
	{ 0x20, 0x20, input_port_0_r },
	{ 0x21, 0x21, input_port_1_r },
	{ 0x28, 0x28, supertr2_questions_read },                /* question data*/
	{ 0xb1, 0xb1, AY8910_read_port_0_r },		/* ???*/
	{ 0xce, 0xce, IORP_NOP },				/* ???*/
PORT_END

static PORT_WRITE_START( supertr2_writeport )
	{ 0x22, 0x22, IOWP_NOP },				/* ???*/
	{ 0x23, 0x23, IOWP_NOP },				/* ???*/
	{ 0x28, 0x28, question_offset_low_w },
	{ 0x29, 0x29, question_offset_med_w },
	{ 0x2a, 0x2a, question_offset_high_w },
	{ 0xb0, 0xb0, AY8910_control_port_0_w },
	{ 0xb1, 0xb1, AY8910_write_port_0_w },
	{ 0xc0, 0xcf, IOWP_NOP },				/* ???*/
PORT_END

static PORT_WRITE_START( trivquiz_writeport )
        { 0x22, 0x22, IOWP_NOP },                               /* ???*/
        { 0x23, 0x23, IOWP_NOP },                               /* ???*/
        { 0x28, 0x28, question_offset_low_w },
        { 0x29, 0x29, question_offset_high_w },
	{ 0xb0, 0xb0, AY8910_control_port_0_w },
	{ 0xb1, 0xb1, AY8910_write_port_0_w },
	{ 0xc0, 0xcf, IOWP_NOP },				/* ???*/
PORT_END

static PORT_READ_START( trivquiz_readport )
	{ 0x20, 0x20, input_port_0_r },
	{ 0x21, 0x21, input_port_1_r },
	{ 0x2a, 0x2a, statriv2_questions_read },                /* question data*/
	{ 0xb1, 0xb1, AY8910_read_port_0_r },		/* ???*/
	{ 0xce, 0xce, IORP_NOP },				/* ???*/
PORT_END

INPUT_PORTS_START( statriv2 )
	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_SERVICE3, "Play All",   IP_KEY_DEFAULT, IP_JOY_DEFAULT )
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_SERVICE2, "Play 10000", IP_KEY_DEFAULT, IP_JOY_DEFAULT )
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_BUTTON1,  "Button A",   IP_KEY_DEFAULT, IP_JOY_DEFAULT )
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_BUTTON2,  "Button B",   IP_KEY_DEFAULT, IP_JOY_DEFAULT )
	PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_BUTTON3,  "Button C",   IP_KEY_DEFAULT, IP_JOY_DEFAULT )
	PORT_BITX(0x40, IP_ACTIVE_LOW, IPT_BUTTON4,  "Button D",   IP_KEY_DEFAULT, IP_JOY_DEFAULT )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

 	PORT_START
	PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_SERVICE1, "Play 1000",  IP_KEY_DEFAULT, IP_JOY_DEFAULT )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_SERVICE( 0x10, IP_ACTIVE_HIGH )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_DIPNAME( 0x80, 0x00, "Freeze" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x80, DEF_STR( On ) )
INPUT_PORTS_END

INPUT_PORTS_START( supertr2 )
	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_SERVICE3, "Play All",   IP_KEY_DEFAULT, IP_JOY_DEFAULT )
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_SERVICE2, "Play 10000", IP_KEY_DEFAULT, IP_JOY_DEFAULT )
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_BUTTON1,  "Button A",   IP_KEY_DEFAULT, IP_JOY_DEFAULT )
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_BUTTON2,  "Button B",   IP_KEY_DEFAULT, IP_JOY_DEFAULT )
	PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_BUTTON3,  "Button C",   IP_KEY_DEFAULT, IP_JOY_DEFAULT )
	PORT_BITX(0x40, IP_ACTIVE_LOW, IPT_BUTTON4,  "Button D",   IP_KEY_DEFAULT, IP_JOY_DEFAULT )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_SERVICE1, "Play 1000",  IP_KEY_DEFAULT, IP_JOY_DEFAULT )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_SERVICE( 0x10, IP_ACTIVE_LOW )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END

static struct GfxLayout statriv2_tiles8x16_layout =
{
	8,16,
	RGN_FRAC(1,1),
	1,
	{ 0 },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
	  8*8, 9*8, 10*8, 11*8, 12*8, 13*8, 14*8, 15*8 },
	16*8
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &statriv2_tiles8x16_layout, 0, 16 },
	{ -1 }
};


static struct AY8910interface ay8910_interface =
{
	1,	/* 1 chip */
	12440000/8,	/* 1.555 MHz */
	{ 100 },
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 }
};

static INTERRUPT_GEN( statriv2_interrupt )
{
	cpu_set_irq_line(0, I8085_RST75_LINE, HOLD_LINE);
}

static MACHINE_DRIVER_START( statriv2 )
	/* basic machine hardware */
	MDRV_CPU_ADD(8085A,12440000)              /* 12.44MHz / 4? */
	MDRV_CPU_MEMORY(statriv2_readmem,statriv2_writemem)
	MDRV_CPU_PORTS(statriv2_readport,statriv2_writeport)
	MDRV_CPU_VBLANK_INT(statriv2_interrupt,1)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_NVRAM_HANDLER(statriv2)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER )
	MDRV_SCREEN_SIZE(64*8, 32*8)
	MDRV_VISIBLE_AREA(4*8, 38*8-1, 0, 32*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(8)
	MDRV_COLORTABLE_LENGTH(2*64)

	MDRV_PALETTE_INIT(statriv2)
	MDRV_VIDEO_START(statriv2)
	MDRV_VIDEO_UPDATE(statriv2)

	/* sound hardware */
	MDRV_SOUND_ADD(AY8910, ay8910_interface)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( supertr2 )
	/* basic machine hardware */
	MDRV_CPU_ADD(8085A,12440000)              /* 12.44MHz / 4? */
	MDRV_CPU_MEMORY(supertr2_readmem,supertr2_writemem)
	MDRV_CPU_PORTS(supertr2_readport,supertr2_writeport)
	MDRV_CPU_VBLANK_INT(statriv2_interrupt,1)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_NVRAM_HANDLER(generic_0fill)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER )
	MDRV_SCREEN_SIZE(64*8, 32*8)
	MDRV_VISIBLE_AREA(2*8, 36*8-1, 0, 32*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(8)
	MDRV_COLORTABLE_LENGTH(2*64)

	MDRV_PALETTE_INIT(statriv2)
	MDRV_VIDEO_START(statriv2)
	MDRV_VIDEO_UPDATE(statriv2)

	/* sound hardware */
	MDRV_SOUND_ADD(AY8910, ay8910_interface)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( trivquiz )
	/* basic machine hardware */
	MDRV_CPU_ADD(8085A,12440000)              /* 12.44MHz / 4? */
	MDRV_CPU_MEMORY(supertr2_readmem,supertr2_writemem)
	MDRV_CPU_PORTS(trivquiz_readport,trivquiz_writeport)
	MDRV_CPU_VBLANK_INT(statriv2_interrupt,1)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_NVRAM_HANDLER(statriv2)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER )
	MDRV_SCREEN_SIZE(64*8, 32*8)
	MDRV_VISIBLE_AREA(4*8, 38*8-1, 0, 32*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(8)
	MDRV_COLORTABLE_LENGTH(2*64)

	MDRV_PALETTE_INIT(statriv2)
	MDRV_VIDEO_START(statriv2)
	MDRV_VIDEO_UPDATE(statriv2)

	/* sound hardware */
	MDRV_SOUND_ADD(AY8910, ay8910_interface)
MACHINE_DRIVER_END

ROM_START( statriv2 )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "trivii1c.u7", 0x00000, 0x01000, CRC(89326d7b) SHA1(4878a3aabe51a4de5ff5927a0707b2d121ff87fa) )
	ROM_LOAD( "trivii2c.u8", 0x01000, 0x01000, CRC(6fd255f6) SHA1(13c75effda1db8eb3635d955ae11f37388f159aa) )
	ROM_LOAD( "trivii3c.u9", 0x02000, 0x01000, CRC(f666dc54) SHA1(757e0e621400d266771ea6db835305208457702f) )

	ROM_REGION( 0x1000, REGION_GFX1, ROMREGION_INVERT )
	ROM_LOAD( "trivii0c.u36", 0x00000, 0x01000, CRC(af5f434a) SHA1(1e7ae7ad7ea697007a30f5ba89127802a835eddc) )

	ROM_REGION( 0x10000, REGION_USER1, 0 ) /* question data */
	ROM_LOAD( "statuspb.u1", 0x00000, 0x02000, CRC(a50c0313) SHA1(f9bf84613e2ebb952a81a10ee1da49a37423b717) )
	ROM_LOAD( "statuspb.u2", 0x02000, 0x02000, CRC(0bc03294) SHA1(c4873cd065c9eb237b03a4195332b7629abac327) )
	ROM_LOAD( "statuspb.u3", 0x04000, 0x02000, CRC(d1732f3b) SHA1(c4e862bd98f237e1d2ecad430226cba6aba4ebb8) )
	ROM_LOAD( "statuspb.u4", 0x06000, 0x02000, CRC(e51d45b8) SHA1(7cd0ced0245dbd55a225182e43b89d55d8d33197) )
	ROM_LOAD( "statuspb.u5", 0x08000, 0x02000, CRC(b3e49c5d) SHA1(bc42ba21bb0d411c2f484076499484b104f0b4ea) )
	ROM_LOAD( "statuspb.u6", 0x0a000, 0x02000, CRC(7ee1cea0) SHA1(00ef768524e54890ebd1fdb3dd52d0080a18fc03) )
	ROM_LOAD( "statuspb.u7", 0x0c000, 0x02000, CRC(121d6976) SHA1(2e4da8f2c3620c8f46fd4951551b0747b3c38caf) )
	ROM_LOAD( "statuspb.u8", 0x0e000, 0x02000, CRC(5080df10) SHA1(b5cb0868d844bbb598159177fd5ce65ff3f18eda) )
ROM_END

ROM_START( trivquiz )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "triv1-1f.u8",  0x00000, 0x01000, CRC(da9a763a) SHA1(d7a60718a1aeadb247330c939e0ac487015b55b2) )
	ROM_LOAD( "triv1-2f.u9",  0x01000, 0x01000, CRC(270459fe) SHA1(1507a477fe7170d24788c880d43b0a3b08f35748) )
	ROM_LOAD( "triv1-3f.u10", 0x02000, 0x01000, CRC(103f4160) SHA1(487afaf243d144aaee8a2ea76105fba09181dfdb) )

	ROM_REGION( 0x1000,  REGION_GFX1, ROMREGION_INVERT )
	ROM_LOAD( "triv1-0f.u7",  0x00000, 0x01000, CRC(af5f434a) SHA1(1e7ae7ad7ea697007a30f5ba89127802a835eddc) )

	ROM_REGION( 0x10000, REGION_USER1, 0 ) /* question data */
	ROM_LOAD( "qmt11.rom",    0x00000, 0x02000, CRC(82107565) SHA1(28d71340873330df7d15f1bc55cee78a9c7c31a6) )
	ROM_LOAD( "qmt12.rom",    0x02000, 0x02000, CRC(68667637) SHA1(df6ad3e624dcad57ce176912931660c6c1780369) )
	ROM_LOAD( "qmt13.rom",    0x04000, 0x02000, CRC(e0d01a68) SHA1(22bb2a8628a3764d733748e4f5f3bad881371a29) )
	ROM_LOAD( "qmt14.rom",    0x06000, 0x02000, CRC(68262b46) SHA1(faba97e5f6475e088554117e4b772e1631d740b2) )
	ROM_LOAD( "qmt15.rom",    0x08000, 0x02000, CRC(d1f39185) SHA1(e46120496e84e224bd15da0652e218cea85c170d) )
	ROM_LOAD( "qmt16.rom",    0x0a000, 0x02000, CRC(1d2ecf1d) SHA1(1d833b57bf4b3ccb3dc60307641ef9476289fe07) )
	ROM_LOAD( "qmt17.rom",    0x0c000, 0x02000, CRC(01840f9c) SHA1(d9b4f7f931657d4e16cf981d887508fd1db5e4c0) )
	ROM_LOAD( "qmt18.rom",    0x0e000, 0x02000, CRC(004a9480) SHA1(7adff194a1549fa42577f969706aab6bb6a58851) )
ROM_END

ROM_START( supertr2 )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "ast2-1d.rom", 0x00000, 0x01000, CRC(e9f0e271) SHA1(c2bae7d5ef04aed3ce14c403c70d2acc1831b763) )
	ROM_LOAD( "ast2-2d.rom", 0x01000, 0x01000, CRC(542ba813) SHA1(1ac063f3678d1295aa728ab7ac43165284b66836) )
	ROM_LOAD( "ast2-3d.rom", 0x02000, 0x01000, CRC(46c467b7) SHA1(2556ce6436112646d8ec3bcff7c32212c5296463) )
	ROM_LOAD( "ast2-4d.rom", 0x03000, 0x01000, CRC(11382c44) SHA1(6b611ad9e591b27d5cb239388e4d27e646be3028) )

	ROM_REGION( 0x1000, REGION_GFX1, ROMREGION_INVERT )
	ROM_LOAD( "ast2-0d.rom", 0x00000, 0x01000, CRC(a40f9201) SHA1(a87cfc3dbe5cff82926f5f8486c37fd3f4449135) )

	ROM_REGION( 0x40000, REGION_USER1, 0 ) /* question data */
	ROM_LOAD( "astq2-1.rom", 0x00000, 0x08000, CRC(4af390cb) SHA1(563c6210f2fcc8ee9b5112e2d6f522ddfca2ddea) )
	ROM_LOAD( "astq2-2.rom", 0x08000, 0x08000, CRC(91a7b4f6) SHA1(c8ff2e8475ae889be14086a04275df94efd66156) )
	ROM_LOAD( "astq2-3.rom", 0x10000, 0x08000, CRC(e6a50944) SHA1(e3fad344d4bedfd14f307445334903c35e745d9b) )
	ROM_LOAD( "astq2-4.rom", 0x18000, 0x08000, CRC(6f9f9cef) SHA1(b43d1a2a714764f46f038f85a8233bf811a877ae) )
	ROM_LOAD( "astq2-5.rom", 0x20000, 0x08000, CRC(a0c0f51e) SHA1(c61518ef53d5bec334062b6853663424853892b9) )
	ROM_LOAD( "astq2-6.rom", 0x28000, 0x08000, CRC(c0f61b5f) SHA1(65398f9d22fce95c4146a2cb8174edd6b336b9e4) )
	ROM_LOAD( "astq2-7.rom", 0x30000, 0x08000, CRC(72461937) SHA1(2f95a708b24f56d9b1293a88aa53eb4a32f89869) )
	ROM_LOAD( "astq2-8.rom", 0x38000, 0x08000, CRC(cd2674d5) SHA1(7fb6513172ffe8e3b9e0f4dc9ecdb42d954b1ff0) )
ROM_END

GAME( 1984, trivquiz, 0, trivquiz, statriv2, 0, ROT0, "Status Games", "Triv Quiz" )
GAME( 1984, statriv2, 0, statriv2, statriv2, 0, ROT0, "Status Games", "Triv Two" )
GAME( 1986, supertr2, 0, supertr2, supertr2, 0, ROT0, "Status Games", "Super Triv II" )
