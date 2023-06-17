/********************************************************************************************

Yumefuda (c) 199? Alba

driver by Angelo Salese

Code disassembling:
0: start with a jump at 6b
6b: initialize some stuff(SP $0000).
initialize ports $00,$01 for unknown purposes,I suspect these aren't AY8910 regs...
4d9: initialize paletteram.
c55: initialize videoram.
7d: writes to port $c0
4c3: initialize ram location a7fc.
82/2603: initialize custom ram(?).port $c0 is also written here...
117: Initialize the videoram.The program used is largely unoptimized though. ;)
a0: Check for the Video RAM.Print "Video RAM N/G!!!" otherwise.
13b: Write to port $c0,then initialze the colorram.
a6: Check for the Color RAM.Print "Video RAM N/G!!!" otherwise.
197: Another Custom RAM calculation.
	1ac: print "Back Up N/G !!!" if the Custom Ram value is bigger than $0a & 0x0f.
	1ae: print "Back Up N/G !!!" if the Custom Ram value is bigger than $a0 & 0xf0.
	^-Note: These two checks for BCD coded numbers...
1c5: HL = af83 DE = e279
2344: Moves the contents from CusRAM to WorkRam.
********************************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

static struct tilemap *bg_tilemap;

static void y_get_bg_tile_info(int tile_index)
{
	int code = videoram[tile_index];
	int color = colorram[tile_index];

	SET_TILE_INFO(
			0,
			code,
			color & 0xf,
			0)
}


VIDEO_START( yumefuda )
{
	bg_tilemap = tilemap_create(y_get_bg_tile_info,tilemap_scan_rows,TILEMAP_OPAQUE,8,8,32,32);

	if(!bg_tilemap)
		return 1;

	return 0;
}

VIDEO_UPDATE( yumefuda )
{
	fillbitmap(bitmap, get_black_pen(), &Machine->visible_area);

	tilemap_draw(bitmap,cliprect,bg_tilemap,0,0);
}

/***************************************************************************************/



static struct GfxLayout charlayout =
{
	8,8,
	RGN_FRAC(1,4),
	4,
	{ RGN_FRAC(0,4), RGN_FRAC(1,4), RGN_FRAC(2,4), RGN_FRAC(3,4) },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8
};

/*
static struct GfxLayout spritelayout =
{
	16,16,
	RGN_FRAC(1,2),
	2,
	{ RGN_FRAC(0,2), RGN_FRAC(1,2) },
	{ 0, 1, 2, 3, 4, 5, 6, 7,
			16*8+0, 16*8+1, 16*8+2, 16*8+3, 16*8+4, 16*8+5, 16*8+6, 16*8+7 },
	{ 0*16, 2*16, 4*16, 6*16, 8*16, 10*16, 12*16, 14*16,
			16*16, 18*16, 20*16, 22*16, 24*16, 26*16, 28*16, 30*16 },
	32*8
};
*/
struct GfxDecodeInfo yumefuda_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0x0000, &charlayout,   0, 16 },
/*	{ REGION_GFX1, 0x0000, &spritelayout, 0, 8 },*/
	{ -1 } /* end of array */
};


static WRITE_HANDLER( yumefuda_vram_w )
{
	if(videoram[offset] != data)
	{
		videoram[offset] = data;
		tilemap_mark_tile_dirty(bg_tilemap,offset);
	}
}

static UINT8 *cus_ram;
static UINT8 prot_lock,nvram_lock;
/*Custom RAM (Protection)*/
static READ_HANDLER( custom_ram_r )
{
	log_cb(RETRO_LOG_DEBUG, LOGPRE "Custom RAM read at %02x PC = %x\n",offset+0xaf80,activecpu_get_pc());
	return cus_ram[offset];/* ^ 0x55;*/
}

static WRITE_HANDLER( custom_ram_w )
{
/*	log_cb(RETRO_LOG_DEBUG, LOGPRE "Custom RAM write at %02x : %02x PC = %x\n",offset+0xaf80,data,activecpu_get_pc());*/
	if(prot_lock)	{ cus_ram[offset] = data; }
}

/*this might be used as NVRAM commands btw*/
static WRITE_HANDLER( prot_lock_w )
{
/*	log_cb(RETRO_LOG_DEBUG, LOGPRE "PC %04x Prot lock value written %02x\n",activecpu_get_pc(),data);*/
	prot_lock = data;
}

static WRITE_HANDLER( nvram_lock_w )
{
	log_cb(RETRO_LOG_DEBUG, LOGPRE "PC %04x Nvram lock value written %02x\n",activecpu_get_pc(),data);
	nvram_lock = data;
}

static WRITE_HANDLER( port_c0_w )
{
}

/***************************************************************************************/

static MEMORY_READ_START( readmem )
	{	0x0000,	0x7fff,	MRA_ROM	},
	{   0x8000, 0x9fff, MRA_BANK1 },
	{	0xaf80, 0xafff, custom_ram_r },
	{	0xb000, 0xb0ff, paletteram_r },
	{	0xc000, 0xc3ff, MRA_RAM },
	{	0xd000, 0xd3ff, MRA_RAM },
	{   0xe000, 0xffff, MRA_RAM },
MEMORY_END

static MEMORY_WRITE_START( writemem )
	{ 	0x0000, 0x7fff, MWA_ROM },
	{   0x8000, 0x9fff, MWA_ROM },
	{	0xa7fc, 0xa7fc, prot_lock_w },
	{   0xa7ff, 0xa7ff, nvram_lock_w },
	{	0xaf80, 0xafff, custom_ram_w ,&cus_ram }, /*260d - 2626*/
	{	0xb000, 0xb0ff, paletteram_BBGGGRRR_w , &paletteram },/*Wrong format?*/
	{	0xc000, 0xc3ff, yumefuda_vram_w	, &videoram },
	{	0xd000, 0xd3ff, MWA_RAM,&colorram },
	{   0xe000, 0xffff, MWA_RAM },/*work ram*/
MEMORY_END

static PORT_READ_START( readport )
	{	0x00, 0x00, AY8910_read_port_0_r },
	{   0x81, 0x81, input_port_2_r },
	{   0x82, 0x82, input_port_3_r },
PORT_END

static PORT_WRITE_START( writeport )
	{ 	0x00, 0x00, AY8910_control_port_0_w },
	{ 	0x01, 0x01, AY8910_write_port_0_w },
	{	0xc0, 0xc0,	port_c0_w }, /*watchdog write?*/
PORT_END

static struct AY8910interface ay8910_interface =
{
	1,	/* 1 chip */
	1500000,	/* 1.5 MHz ???? */
	{ 50 },
	{ input_port_0_r },
	{ input_port_1_r },
	{ 0 },
	{ 0 }
};

static MACHINE_DRIVER_START( yumefuda )

	/* basic machine hardware */
	MDRV_CPU_ADD(Z80 , 6000000)/*???*/
	MDRV_CPU_MEMORY(readmem,writemem)
	MDRV_CPU_PORTS(readport,writeport)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,1)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(0, 32*8-1, 0, 32*8-1)
	MDRV_GFXDECODE( yumefuda_gfxdecodeinfo )
	MDRV_PALETTE_LENGTH(0x100)

	MDRV_VIDEO_START( yumefuda )
	MDRV_VIDEO_UPDATE( yumefuda )

	/* sound hardware */
	MDRV_SOUND_ADD(AY8910, ay8910_interface)
MACHINE_DRIVER_END

/***************************************************************************************/

INPUT_PORTS_START( yumefuda )
	PORT_START	/* DSW1 (00 - Port A) */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START	/* DSW2	(0x00 - Port B) */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START	/* DSW1 (00 - Port A) */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START	/* DSW2	(0x00 - Port B) */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
INPUT_PORTS_END

/***************************************************************************************/


ROM_START( yumefuda )
	ROM_REGION( 0x18000, REGION_CPU1, 0 ) /* code */
	ROM_LOAD("zg004y02.u43", 0x00000, 0x8000, CRC(974c543c) SHA1(56aeb318cb00445f133246dfddc8c24bb0c23f2d))
	ROM_LOAD("zg004y01.u42", 0x10000, 0x8000, CRC(ae99126b) SHA1(4ae2c1c804bbc505a013f5e3d98c0bfbb51b747a))

	ROM_REGION( 0x10000, REGION_GFX1, 0 )
	ROM_LOAD("zg001006.u6", 0x0000, 0x4000, CRC(a5df443c) SHA1(a6c088a463c05e43a7b559c5d0afceddc88ef476))
	ROM_LOAD("zg001005.u5", 0x4000, 0x4000, CRC(158b6cde) SHA1(3e335b7dc1bbae2edb02722025180f32ab91f69f))
	ROM_LOAD("zg001004.u4", 0x8000, 0x4000, CRC(d8676435) SHA1(9b6df5378948f492717e1a4d9c833ddc5a9e8225))
	ROM_LOAD("zg001003.u3", 0xc000, 0x4000, CRC(5822ff27) SHA1(d40fa0790de3c912f770ef8f610bd8c42bc3500f))
ROM_END

DRIVER_INIT( yumefuda )
{
	unsigned char *ROM = memory_region(REGION_CPU1);
	UINT32 bankaddress;

	/*Temp until the bankswitch memory address is found...*/
	bankaddress = 0x10000;
	cpu_setbank(1, &ROM[bankaddress]);

}


GAMEX( 199?, yumefuda, 0, 		yumefuda, yumefuda, yumefuda,	ROT0, "Alba", "Yumefuda [BET]", GAME_NO_SOUND | GAME_NOT_WORKING )
