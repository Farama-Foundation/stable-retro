/******************************************************************************
 Prebillian  1986 Taito WORKING
 Hot Smash   1987 Taito NOT WORKING (protection? see notes)

 Driver by Tomasz Slanina  dox@space.pl

 There are some similarities with Super Qix (superqix.c) it might be possible
 to merge the drivers at some point.

Prebillian :
------------

PCB Layout (Prebillian, from The Guru ( http://unemulated.emuunlim.com/ )
 
 M6100211A
 -------------------------------------------------------------------
 |                    HM50464                                       |
 |  6                 HM50464                                       |
 |  5                 HM50464                               6116    |
 |  4                 HM50464                                       |
 |                                                                  |
 |                                                                  |
 |                                                               J  |
 |                                            68705P5 SW1(8)        |
 |               6264                                            A  |
 |                                              3     SW2(8)        |
 |                                                               M  |
 |                                                                  |
 |                                                               M  |
 |                                                                  |
 |                                   2                           A  |
 |                                                                  |
 |                                   1                              |
 |                                                                  |
 |                                   Z80B            AY-3-8910      |
 | 12MHz                                                            |
 --------------------------------------------------------------------

Notes:
       Vertical Sync: 60Hz
         Horiz. Sync: 15.67kHz
         Z80B Clock : 5.995MHz
     AY-3-8910 Clock: 1.499MHz

Controls :

Info from flyer (japaneese) :
- button for 'Shot'
- pullout plunger for shot power (there's no on-screen power indicator in the game)
- dial for aiming

There's also additional button(s)
used for entering initials(hiscore).

Sound:

Ay 8910 + samples.

It's total guess at the moment. 
(see notes in /sndhrdw/prebillian.c)

Gfx:
Sprites are made of 4 tiles (including bg tiles (mostly chars))

Additional info about i/o port $410 :
 
 -------0  ? [not used]
 ------1-  coin counter 1
 -----2--  coin counter 2
 ----3---  rom 2 HI (reserved for ROM banking , not used)
 ---4----  nmi enable/disable
 765-----  flip screen (bit 5 = flip screen ,  6,7 =?)
 ---
 110 cocktail + flip player 2
 001 cocktail + flip player 1 
 001 cocktail player 2 
 000 cocktail player 1
 001 flip screen   

Hot (Vs) Smash :
----------------

Protected ?

There's no reaction for collision between
ball and screen edge (see notes in /vidhrdw/prebillian.c, draw_sprites() )
(additional notes in data_r08_r)
Dips (not verified):

DSW1 stored @ $f236
76------ coin a 
--54---- coin b
----3--- stored @ $f295 , tested @ $2a3b
------1- code @ $03ed, stored @ $f253 (flip screen)
		
DSW2 stored @ $f237 
---4---- code @ $03b4, stored @ $f290  (3/4 lives ?)
----32-- code @ $03d8, stored @ $f293 (3600/5400/2400/1200  -> bonus  ?)
------10 code @ $03be, stored @ $f291/92 (8,8/0,12/16,6/24,4 -> difficulty ? )

******************************************************************************/
#include "driver.h"
#include "vidhrdw/generic.h"
WRITE_HANDLER( pb_videoram_w );
WRITE_HANDLER(data_41a_w);

int pbillian_sh_start(const struct MachineSound*);

VIDEO_START(pbillian);
VIDEO_UPDATE(pbillian);

data8_t select_403,select_407,select_408,is_pbillian;
data8_t *pb_videoram;

static WRITE_HANDLER(select_408_w)
{
	select_408=data;
}  

static WRITE_HANDLER(data_410_w)
{
	coin_counter_w(0,data&2);
	coin_counter_w(1,data&4);
	interrupt_enable_w(0,data&0x10);
	flip_screen_set(data&0x20);
}

static READ_HANDLER(data_408_r)
{
	/* 
		Hot Smash
		select_408=1 && bit 7==1 -> protection related ?
 		Setting this bit to high cause win/lose/game over etc
 		(see below)
  */		
	
	switch(select_408)
	{
		case    0: return 0; /*pb?*/
		case    1: return is_pbillian?input_port_3_r(0):((spriteram[0x20]&1)?0x8c:input_port_3_r(0));
	
							/* 
								written by mcu ? (bit 7=1) (should be sequence of writes , 0x88+0x8c for example)
										
								0x82 = no ball
								0x83 = time over		
								0x84 = P1 score++
								0x86 = 0-1
								0x87 = 1-0
								0x88 = P1 WIN
								0x89 = Game Over
								0x8a = restart P1 side
								0x8b = restart P2 side
								0x8c = next level + restart
								
							*/
							 
		case    2: return input_port_4_r(0);	
		case    4: return input_port_0_r(0);
		case    8: return input_port_1_r(0);
		case 0x20: return 0; /*pb ? */
		case 0x80: return 0; /*pb?*/
		case 0xf0: return 0; /*hs? */
		
	}
	log_cb(RETRO_LOG_DEBUG, LOGPRE "408[%x] r at %x\n",select_408,activecpu_get_previouspc());
	return 0;
}

static READ_HANDLER(ay_port_a_r)
{
	 /* bits 76------  latches ?  0x40 should be ok for prebillian but not for hot smash*/
	 return (rand()&0xc0)|input_port_5_r(0);
}


static struct CustomSound_interface custom_interface =
{
	pbillian_sh_start,
	0,
	0
};

static struct AY8910interface ay8910_interface =
{
	1, 
	1500000, 
	{ 30 },
	{ay_port_a_r},
	{input_port_2_r},
	{ 0 },
	{ 0 }
};



static MEMORY_READ_START( readmem )
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0xbfff, MRA_BANK1 },
	{ 0xe000, 0xffff, MRA_RAM },
MEMORY_END


static MEMORY_WRITE_START( writemem )
	{ 0x0000, 0xbfff, MWA_ROM },
	{ 0xe000, 0xe0ff, MWA_RAM, &spriteram, &spriteram_size },
	{ 0xe100, 0xe7ff, MWA_RAM },
	{ 0xe800, 0xefff, pb_videoram_w,&pb_videoram },
	{ 0xf000, 0xffff, MWA_RAM },
MEMORY_END



static PORT_READ_START( readport )
	{ 0x0000, 0x01ff, paletteram_r },
	{ 0x0401, 0x0401, AY8910_read_port_0_r },
	{ 0x0408, 0x0408, data_408_r},
	{ 0x0418, 0x0418, MRA_NOP },  /*?*/
	{ 0x041b, 0x041b, MRA_NOP },  /*?*/
PORT_END

static PORT_WRITE_START( writeport )
	{ 0x0000, 0x01ff, paletteram_BBGGRRII_w },
	{	0x0200, 0x03ff, MWA_NOP},
	{ 0x0402, 0x0402, AY8910_write_port_0_w },
	{ 0x0403, 0x0403, AY8910_control_port_0_w },
	{ 0x408, 0x408, select_408_w},
	{ 0x410, 0x410, data_410_w },
	{ 0x41a, 0x41a, data_41a_w},
	{ 0x419, 0x419, MWA_NOP },  /*? watchdog ?*/
PORT_END

INPUT_PORTS_START( pbillian )

	PORT_START	
	PORT_DIPNAME( 0x07, 0x07, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x05, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x07, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 1C_3C ) )
	PORT_DIPNAME( 0x38, 0x38, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x18, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x28, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x38, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 1C_3C ) )
	PORT_DIPNAME( 0x40, 0x00, "Allow Continue" )
	PORT_DIPSETTING(    0x40, DEF_STR( No ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x80, 0x80, "Freeze" )
	PORT_DIPSETTING(    0x80, DEF_STR( No ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Yes ) )

	PORT_START 
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "5" )
	PORT_DIPSETTING(    0x01, "4" )
	PORT_DIPSETTING(    0x02, "3" )
	PORT_DIPSETTING(    0x03, "2" )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x0c, "10/20/300K Points" )
	PORT_DIPSETTING(    0x08, "20/30/400K Points" )
	PORT_DIPSETTING(    0x04, "30/40/500K Points" )
	PORT_DIPSETTING(    0x00, "10/30/500K Points" )
	PORT_DIPNAME( 0x30, 0x10, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x00, "Easy" )
	PORT_DIPSETTING(    0x10, "Normal" )
	PORT_DIPSETTING(    0x20, "Hard" )
	PORT_DIPSETTING(    0x30, "Very Hard" )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	
	PORT_START 
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_SERVICE1)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START
	PORT_ANALOG( 0x3f, 0x00, IPT_PADDLE_V| IPF_REVERSE , 30, 3, 0, 0x3f )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	
	PORT_START
	PORT_ANALOG( 0xff, 0x00, IPT_DIAL, 20,10, 0, 0 )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_BUTTON2| IPF_COCKTAIL )
	
INPUT_PORTS_END



INPUT_PORTS_START( hotsmash )

	PORT_START	
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x00, "Unk2" )
	PORT_DIPSETTING(    0x08, DEF_STR( No ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 1C_2C ) )
	PORT_DIPNAME( 0xc0, 0xc0, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 1C_2C ) )
	
	PORT_START 
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "4" )
	PORT_DIPSETTING(    0x10, "3" )
	PORT_DIPNAME( 0x0c, 0x0c, "Unk3" )
	PORT_DIPSETTING(    0x00, "1" )
	PORT_DIPSETTING(    0x04, "2" )
	PORT_DIPSETTING(    0x08, "3" )
	PORT_DIPSETTING(    0x0c, "4" )
	
	PORT_START 

	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN1 )/*$42d*/
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x2, IP_ACTIVE_LOW, IPT_SERVICE1)

	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNUSED )/*$49c , coin ?? game crashes*/
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNUSED )
		
	PORT_START
	PORT_ANALOG( 0x7f, 0x38, IPT_PADDLE|IPF_REVERSE, 30, 5, 0x10, 0x7f )
	
	PORT_START
	
	PORT_START

INPUT_PORTS_END

static struct GfxLayout gfxlayout =
{
   8,8,
   RGN_FRAC(1,1),
   4,
   { 0,1,2,3 },
   { 0*4, 1*4, 2*4, 3*4, 4*4, 5*4, 6*4, 7*4},
   { 0*4*8, 1*4*8, 2*4*8, 3*4*8, 4*4*8, 5*4*8, 6*4*8, 7*4*8},
   8*8*4
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &gfxlayout,   0, 32 }, 
	{ -1 }
};

static MACHINE_DRIVER_START( pbillian )
	MDRV_CPU_ADD(Z80,6000000)		 /* 6 MHz */
	MDRV_CPU_PORTS(readport,writeport)
	MDRV_CPU_MEMORY(readmem,writemem)
	MDRV_CPU_FLAGS(CPU_16BIT_PORT)
	MDRV_CPU_VBLANK_INT(nmi_line_pulse,1)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER )
	MDRV_SCREEN_SIZE(256, 256)
	MDRV_VISIBLE_AREA(0, 256-1, 16, 256-16-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(512)

	MDRV_VIDEO_START(pbillian)
	MDRV_VIDEO_UPDATE(pbillian)

	MDRV_SOUND_ADD(AY8910, ay8910_interface)
	MDRV_SOUND_ADD(CUSTOM, custom_interface)

MACHINE_DRIVER_END


ROM_START( pbillian )
	ROM_REGION( 0x018000, REGION_CPU1, 0 )
	ROM_LOAD( "1.6c",  0x00000, 0x08000, CRC(d379fe23) SHA1(e147a9151b1cdeacb126d9713687bd0aa92980ac) ) 
	ROM_LOAD( "2.6d",  0x08000, 0x04000, CRC(1af522bc) SHA1(83e002dc831bfcedbd7096b350c9b34418b79674) ) 

	ROM_REGION( 0x0800, REGION_CPU2, 0 )
	ROM_LOAD( "pbillian.mcu", 0x00000, 0x0800, NO_DUMP ) 
	
	ROM_REGION( 0x8000, REGION_SOUND1, 0 )
	ROM_LOAD( "3.7j",  0x0000, 0x08000, CRC(3f9bc7f1) SHA1(0b0c2ec3bea6a7f3fc6c0c8b750318f3f9ec3d1f) )
	
	ROM_REGION( 0x018000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "4.1n",  0x00000, 0x08000, CRC(9c08a072) SHA1(25f31fcf72216cf42528b07ad8c09113aa69861a) ) 
	ROM_LOAD( "5.1r",  0x08000, 0x08000, CRC(2dd5b83f) SHA1(b05e3a008050359d0207757b9cbd8cee87abc697) ) 
	ROM_LOAD( "6.1t",  0x10000, 0x08000, CRC(33b855b0) SHA1(5a1df4f82fc0d6f78883b759fd61f395942645eb) ) 
ROM_END


ROM_START( hotsmash )
	ROM_REGION( 0x018000, REGION_CPU1, 0 )
	ROM_LOAD( "b18-04",  0x00000, 0x08000, CRC(981bde2c) SHA1(ebcc901a036cde16b33d534d423500d74523b781) )
	
	ROM_REGION( 0x0800, REGION_CPU2, 0 )
	ROM_LOAD( "b18-06.mcu", 0x00000, 0x0800, NO_DUMP ) 
	
	ROM_REGION( 0x8000, REGION_SOUND1, 0 )
	ROM_LOAD( "b18-05",  0x0000, 0x08000, CRC(dab5e718) SHA1(6cf6486f283f5177dfdc657b1627fbfa3f0743e8) )

	ROM_REGION( 0x018000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "b18-01",  0x00000, 0x08000, CRC(870a4c04) SHA1(a029108bcda40755c8320d2ee297f42d816aa7c0) )
	ROM_LOAD( "b18-02",  0x08000, 0x08000, CRC(4e625cac) SHA1(2c21b32240eaada9a5f909a2ec5b335372c8c994) )
	ROM_LOAD( "b18-03",  0x14000, 0x04000, CRC(1c82717d) SHA1(6942c8877e24ac51ed71036e771a1655d82f3491) )
ROM_END	

static DRIVER_INIT( pbillian ){	is_pbillian=1;}
static DRIVER_INIT( hotsmash ){	is_pbillian=0;}

GAMEX( 1986, pbillian, 0, pbillian, pbillian, pbillian, ROT0, "Taito", "Prebillian",GAME_IMPERFECT_SOUND)
GAMEX( 1987, hotsmash, 0, pbillian, hotsmash, hotsmash, ROT90, "Taito", "Hot Smash",GAME_NOT_WORKING|GAME_UNEMULATED_PROTECTION )
