/*
---------------------------
Marine Date by TAITO (1981)
---------------------------

Location     Device      File ID     Checksum
---------------------------------------------
LB 3D         2716        MG01         BB4B
LB 4D         2716        MG02         89B3
LB 5D         2716        MG03         A5CE
LB 6D         2716        MG04         CE20
LB 7D         2716        MG05         16B9
LB 9D         2716        MG06         39A9
LB 10D        2716        MG07         B7F1
LB 1F         2716        MG09         9934
LB 3F         2716        MG10         F185
LB 4F         2716        MG11         1603
MB 6C         2532        MG12         66C3
MB 6H         2532        MG13         23E2
MB 2A       82S123        MG14.BPR     1CB1
MB 1A       82S123        MG15.BPR     1471
MB 4E       82S123        MG16.BPR     0570
TB 5F       82S123        MG17.BPR     129B


Notes:     TB - Top PCB        MG070001  MGN00001
           MB - Middle PCB     MG070002  MGN00002
           LB - Lower PCB      AA017779  MGN00002


Brief Hardware Overview
-----------------------

Main processor    -  Z80  2.5MHz

Sound             - Discrete audio, like Space Invaders

-------------------------------------------------------------------------

a static underwater scence with obstacles in it, like seaweed,
crabs and other stuff.  You have a limited number of "strokes"
per screen as well as a timer to work against.  Your goal is
to *bounce* yourself around the screen using *Strokes* on the
trackball to try to reach a *female* octopus before your run out
of strokes or time.  You sort of bounce yourself around the screen
like a billiard ball would bounce, but once in a while bubbles
and other stuff will come up from underneath you and carry you
away from where you are trying to get.  When you reach your goal
you get another more difficult screen, etc.

I think it was manufactured by Taito, I'm not sure but I seem to
recall that it was a full blown Japanese machine.


todo:
in cocktail mopde p1 is flipped
after inking the shark on the far right octi was moved to goal?
for the colours, goal has to be black otherwise it would register
	qas a hit, is goal pen 0 or 6?
rom writes when finishing a game
	worth looking at before the collision is correct?
playing dot hit when eaten by a shark?
dont use any ints, s/b UINT8?
enemy sprite not disabled at end of game
tilemap
palette may only be around 4 colours
	is 14 the palette?
p2 ink doesn't always light up in test mode
how do you know if you've got an ink left?
prom 14 is the top bits? 4 bpp? or so?
why is level 37 chosen?
should it be 30fps?
	check other taito games of the time
look at other taito 1981 games for ideas on the ports
	bking
	jhunt?
"Marine Deto" or "Marine Date"
	look in the roms for all the text
simplify gfx decode
why does the player sprite need 4 colours?
	check if more than 1 are used
check service test ram wipes for confirmation of ram spots
	anything after trackball test?
obj1 to obj2 draw order
2nd trackball
flip/cocktail issues

done:
timer?
	you get 200 for each shot, don't think it's actually a timer
have i been using x/y consistently, ie non rotated or rotated origin?
	yes, seems to be best using xy raw (ie non-rotated)
*/

#include "vidhrdw/generic.h"

static int marinedt_obj1_a, marinedt_obj1_x, marinedt_obj1_y, marinedt_music, marinedt_sound, marinedt_obj2_a, marinedt_obj2_x, marinedt_obj2_y, marinedt_pd, marinedt_pf;
static	int coll,cx,cyr,cyq;
static	int collh,cxh,cyrh,cyqh;

static MEMORY_READ_START( marinedt_readmem )
	{ 0x0000, 0x37ff, MRA_ROM },
	{ 0x4000, 0x43ff, MRA_RAM },
	{ 0x4400, 0x47ff, MRA_RAM },	/*unused, vram mirror?*/
	{ 0x4000, 0x4bff, MRA_RAM },
MEMORY_END

static MEMORY_WRITE_START( marinedt_writemem )
	{ 0x0000, 0x37ff, MWA_ROM },
	{ 0x4000, 0x47ff, MWA_RAM },
	{ 0x4800, 0x4bff, videoram_w, &videoram, &videoram_size },
	{ 0x4c00, 0x4c00, MWA_NOP },	/*?? maybe off by one error*/
MEMORY_END

static READ_HANDLER( marinedt_port1_r )
{
/*might need to be reversed for cocktail stuff*/

	/* x/y multiplexed */
	return readinputport(3 + ((marinedt_pf&0x08)>>3));
}

static READ_HANDLER( marinedt_coll_r )
{
	/*76543210*/
	/*x------- obj1 to obj2 collision*/
	/*-xxx---- unused*/
    /*----x--- obj1 to playfield collision*/
	/*-----xxx unused*/

	if (keyboard_pressed(KEYCODE_X)) return 0x80;
	if (keyboard_pressed(KEYCODE_Z)) return 0x08;

	return coll | collh;
}

/*are these returning only during a collision?*/
/*id imagine they are returning the pf char where the collission took place?*/
/*what about where there is lots of colls?*/
/*maybe the first on a scanline basis*/
static READ_HANDLER( marinedt_obj1_x_r )
{
	/*76543210*/
	/*xxxx---- unknown*/
	/*----xxxx x pos in video ram*/

	unsigned char *RAM = memory_region(REGION_CPU1);
if(RAM[0x430e]) --cx; else ++cx;
/*figure out why inc/dec based on 430e?*/
	return cx | (cxh<<4);
}

static READ_HANDLER( marinedt_obj1_yr_r )
{
	/*76543210*/
	/*xxxx---- unknown*/
	/*----xxxx row in current screen quarter*/
/*has to be +1 if cx went over?*/
if (cx==0x10) cyr++;

	return cyr | (cyrh<<4);
}

static READ_HANDLER( marinedt_obj1_yq_r )
{
	/*76543210*/
	/*xx------ unknown*/
	/*--xx---- screen quarter when flipped?*/
    /*----xx-- unknown*/
	/*------xx screen quarter*/

	return cyq | (cyqh<<4);
}

static PORT_READ_START( marinedt_readport )
	{ 0x00, 0x00, input_port_0_r },		/*dips coinage*/
	{ 0x01, 0x01, marinedt_port1_r },	/*trackball xy muxed*/
	{ 0x02, 0x02, marinedt_obj1_x_r },
	{ 0x03, 0x03, input_port_1_r },		/*buttons*/
	{ 0x04, 0x04, input_port_2_r },		/*dips*/
	{ 0x06, 0x06, marinedt_obj1_yr_r },
	{ 0x0a, 0x0a, marinedt_obj1_yq_r },
	{ 0x0e, 0x0e, marinedt_coll_r },
PORT_END

static WRITE_HANDLER( marinedt_obj1_a_w ) {	marinedt_obj1_a = data; }
static WRITE_HANDLER( marinedt_obj1_x_w ) {	marinedt_obj1_x = data; }
static WRITE_HANDLER( marinedt_obj1_y_w ) {	marinedt_obj1_y = data; }
static WRITE_HANDLER( marinedt_obj2_a_w ) {	marinedt_obj2_a = data; }
static WRITE_HANDLER( marinedt_obj2_x_w ) {	marinedt_obj2_x = data; }
static WRITE_HANDLER( marinedt_obj2_y_w ) {	marinedt_obj2_y = data; }

static WRITE_HANDLER( marinedt_music_w ){	marinedt_music = data; }

static WRITE_HANDLER( marinedt_sound_w )
{
	/*76543210*/
	/*xx------ ??*/
	/*--x----- jet sound*/
	/*---x---- foam*/
    /*----x--- ink*/
	/*-----x-- collision*/
    /*------x- dots hit*/
	/*-------x ??*/

	marinedt_sound = data;
}

static WRITE_HANDLER( marinedt_pd_w )
{
	/*76543210*/
	/*xxx----- ?? unused*/
	/*---x---- ?? the rest should be used based on the table*/
    /*----x--- ??*/
	/*-----x-- ??*/
    /*------x- obj2 enable*/
	/*-------x obj1 enable*/

	marinedt_pd = data;
}

static WRITE_HANDLER( marinedt_pf_w )
{
	/*76543210*/
	/*xxxx---- ?? unused (will need to understand table of written values)*/
    /*----x--- xy trackball select*/
	/*-----x-- ?? flip screen/controls*/
    /*------x- ?? upright/cocktail*/
	/*-------x ?? service mode*/

	marinedt_pf = data;

/*if(data&0xf0)*/
/*	log_cb(RETRO_LOG_DEBUG, LOGPRE "pf:%02x %d\n",marinedt_pf);*/
/*logerror("pd:%02x %d\n",marinedt_pd, cpu_getcurrentframe());*/

}

static PORT_WRITE_START( marinedt_writeport )
	{ 0x02, 0x02, marinedt_obj1_a_w },
	{ 0x03, 0x03, marinedt_obj1_x_w },
	{ 0x04, 0x04, marinedt_obj1_y_w },
	{ 0x05, 0x05, marinedt_music_w },
	{ 0x06, 0x06, marinedt_sound_w },
	{ 0x08, 0x08, marinedt_obj2_a_w },
	{ 0x09, 0x09, marinedt_obj2_x_w },
	{ 0x0a, 0x0a, marinedt_obj2_y_w },
	{ 0x0d, 0x0d, marinedt_pd_w },
	{ 0x0e, 0x0e, watchdog_reset_w },
	{ 0x0f, 0x0f, marinedt_pf_w },
PORT_END

INPUT_PORTS_START( marinedt )
	PORT_START	/* IN0 */
	PORT_DIPNAME( 0x0f, 0x00, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x0f, DEF_STR( 9C_1C ) )
	PORT_DIPSETTING(    0x0e, DEF_STR( 8C_1C ) )
	PORT_DIPSETTING(    0x0d, DEF_STR( 7C_1C ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( 6C_1C ) )
	PORT_DIPSETTING(    0x0b, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(    0x0a, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x09, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0x05, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 1C_7C ) )
	PORT_DIPSETTING(    0x07, DEF_STR( 1C_8C ) )
	PORT_DIPNAME( 0xf0, 0x00, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0xf0, DEF_STR( 9C_1C ) )
	PORT_DIPSETTING(    0xe0, DEF_STR( 8C_1C ) )
	PORT_DIPSETTING(    0xd0, DEF_STR( 7C_1C ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 6C_1C ) )
	PORT_DIPSETTING(    0xb0, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(    0xa0, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x90, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0x50, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(    0x60, DEF_STR( 1C_7C ) )
	PORT_DIPSETTING(    0x70, DEF_STR( 1C_8C ) )

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_SERVICE1 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_TILT )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_START1 )

	PORT_START	/* IN2 */
    PORT_DIPNAME( 0x01, 0x00, DEF_STR( Bonus_Life ) )
    PORT_DIPSETTING(    0x01, "5000" )
    PORT_DIPSETTING(    0x00, "10000" )
/*cheat?*/
    PORT_DIPNAME( 0x02, 0x00, "ignore internal bounce?" )	/*maybe die/bounce of rocks/coral?*/
    PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
    PORT_DIPSETTING(    0x02, DEF_STR( On ) )
/*freezes the game before the reset*/
/*doesn't seem to be done as a dip, but what about mixing with diops like this?*/
	PORT_BITX(0x04, IP_ACTIVE_HIGH, IPT_SERVICE | IPF_TOGGLE, DEF_STR( Service_Mode ), KEYCODE_F2, IP_JOY_NONE )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x10, 0x00, "Coin Chutes" )
	PORT_DIPSETTING(    0x00, "Common" )
	PORT_DIPSETTING(    0x10, "Individual" )
    PORT_DIPNAME( 0x20, 0x00, "Year Display" )
    PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
    PORT_DIPSETTING(    0x00, DEF_STR( On ) )
    PORT_DIPNAME( 0xc0, 0x00, DEF_STR( Lives ) )
    PORT_DIPSETTING(    0x00, "3" )
    PORT_DIPSETTING(    0x40, "4" )
    PORT_DIPSETTING(    0x80, "5" )
    PORT_DIPSETTING(    0xc0, "6" )

    PORT_START  /* IN3 - FAKE MUXED */
/*check all bits are used*/
    PORT_ANALOG( 0xff, 0x00, IPT_TRACKBALL_X | IPF_REVERSE, 25, 10, 0, 0 )

    PORT_START  /* IN4 - FAKE MUXED */
    PORT_ANALOG( 0xff, 0x00, IPT_TRACKBALL_Y, 25, 10, 0, 0 )
INPUT_PORTS_END

static struct GfxLayout marinedt_charlayout =
{
	8,8,
	RGN_FRAC(1,3),
	3,
	{ RGN_FRAC(2,3), RGN_FRAC(1,3), RGN_FRAC(0,3) },	/*maybe 120*/
	{ STEP8(0,1) },
	{ STEP8(0,8) },
	8*8
};

static struct GfxLayout marinedt_objlayout =
{
	32,32,
	RGN_FRAC(1,1),
	2,
	{ 0, 4 },
	{ STEP4(32*8*7,1), STEP4(32*8*6,1), STEP4(32*8*5,1), STEP4(32*8*4,1), STEP4(32*8*3,1), STEP4(32*8*2,1), STEP4(32*8*1,1), STEP4(32*8*0,1) },
	{ STEP16(0,8), STEP16(16*8,8) },
	32*32*2
};

static struct GfxDecodeInfo marinedt_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &marinedt_charlayout, 0,  4 },	/*really only 1 colour set?*/
	{ REGION_GFX2, 0, &marinedt_objlayout,  48, 4 },
	{ REGION_GFX3, 0, &marinedt_objlayout,  32, 4 },
	{ -1 }
};

PALETTE_INIT( marinedt )
{
	int i,r,b,g;

	for (i = 0;i < Machine->drv->total_colors; i++)
	{
		int bit0,bit1,bit2;

		/* red component */
		bit0 = (~color_prom[i] >> 0) & 0x01;
		bit1 = (~color_prom[i] >> 1) & 0x01;
		bit2 = (~color_prom[i] >> 2) & 0x01;
/*		*(palette++) = 0x92 * bit0 + 0x46 * bit1 + 0x27 * bit2;*/
		r = 0x27 * bit0 + 0x46 * bit1 + 0x92 * bit2;
		/* green component */
		bit0 = (~color_prom[i] >> 3) & 0x01;
		bit1 = (~color_prom[i] >> 4) & 0x01;
		bit2 = (~color_prom[i] >> 5) & 0x01;
/*		*(palette++) = 0x92 * bit0 + 0x46 * bit1 + 0x27 * bit2;*/
		g = 0x27 * bit0 + 0x46 * bit1 + 0x92 * bit2;
		/* blue component */
		bit0 = (~color_prom[i] >> 5) & 0x01;
		bit1 = (~color_prom[i] >> 6) & 0x01;
		bit2 = (~color_prom[i] >> 7) & 0x01;
bit0=0;
/*		*(palette++) = 0x92 * bit0 + 0x46 * bit1 + 0x27 * bit2;*/
		b = 0x27 * bit0 + 0x46 * bit1 + 0x92 * bit2;

		palette_set_color(i,r,g,b);
	}
}

VIDEO_UPDATE( marinedt )
{
	int offs,sx,sy,flipx,flipy;
#if 0
	int i=0,j=0;
#endif
fillbitmap(bitmap, Machine->pens[0], cliprect);
	for (offs = 0;offs < videoram_size;offs++)
	{
		sx = offs%32;
		sy = offs/32;
		flipx = 1;
		flipy = 0;

/*logerror("%x\n",videoram[offs]);*/

		drawgfx(bitmap,Machine->gfx[0],
				videoram[offs],
				0,
				flipx,flipy,
				8*sx,8*sy,
				cliprect,TRANSPARENCY_NONE,0);
	}
/*{*/
/*int x,yr,yq;*/

/*x=((256-20-marinedt_obj1_x)%128)/8;*/
/*yr=(((256+2-marinedt_obj1_y+10)%64)/8)*2+((256-20-marinedt_obj1_x)>127?1:0);*/
/*yr=yr<<4;*/
/*yq=(256+2-marinedt_obj1_y+10)/64;*/
/*yq=yq<<8;*/
/*	{*/
/*		sx = (x+yr+yq)%32;*/
/*		sy = (x+yr+yq)/32;*/
/*		flipx = 1;*/
/*		flipy = 0;*/

/*logerror("%x\n",videoram[offs]);*/

/*		drawgfx(bitmap,Machine->gfx[0],*/
/*				0,*/
/*				0,*/
/*				flipx,flipy,*/
/*				8*sx,8*sy,*/
/*				cliprect,TRANSPARENCY_NONE,0);*/
/*	}*/
/*}*/

{
/*	char buf[40];*/
/*	sprintf(buf,"%02X %02X %02X %02X %02X %02X %02X %02X %02X %02X", marinedt_p2, marinedt_p3, marinedt_p4, marinedt_p5, marinedt_p6, marinedt_p8, marinedt_p9, marinedt_pa, marinedt_pd, marinedt_pf);*/
/*	usrintf_showmessage(buf);*/
/*	ui_drawbox(bitmap, 16, 230, 200,25);*/
/* 	ui_text(bitmap, "02 03 04 05 06 08 09 0a 0d 0f", 20, 234);*/
/* 	ui_text(bitmap, buf, 20, 244);*/

/*  	if (marinedt_sound&0x02) ui_text(bitmap, "dot", 10, 4+36);*/
/*  	if (marinedt_sound&0x04) ui_text(bitmap, "col", 50, 4+36);*/
/*  	if (marinedt_sound&0x08) ui_text(bitmap, "ink", 90, 4+36);*/
/*  	if (marinedt_sound&0x10) ui_text(bitmap, "foam", 130, 4+36);*/
/*  	if (marinedt_sound&0x20) ui_text(bitmap, "jet", 180, 4+36);*/

/*obj to obj coll*/
collh=0;cxh=0;cyrh=0;cyqh=0;

#if 0
if (marinedt_pd&0x03)
{
	int o1x=256-32-marinedt_obj1_x;
	int o1y=256-marinedt_obj1_y-1;
	int o2x=256-32-marinedt_obj2_x;
	int o2y=256-marinedt_obj2_y-1;

	if (o2x+10<o1x+22 && o2x+22>o1x+10)
		if (o2y+10<o1y+22 && o2y+22>o1y+10)
		{
			collh=0x80;

			cxh=((256-32-marinedt_obj1_x+16)%128)/8;cxh&=0x0f;
		/*-1 fudge?*/
			cyrh=(((256-marinedt_obj1_y-1+16)%64)/8)*2+((256-32-marinedt_obj1_x+16)>127?1:0);cyrh&=0x0f;
			cyqh=(256-marinedt_obj1_y-1+16)/64;cyqh&=0x0f;
		}
}
#endif

/*pf collision?*/

#if 0
if (marinedt_pd&0x01)
{

	drawgfx(bitmap,Machine->gfx[1],
			((marinedt_obj1_a&0x04)<<1)+((marinedt_obj1_a&0x38)>>3),
			marinedt_obj1_a&0x03,
			1,marinedt_obj1_a&0x80,
			256,32,
			cliprect,TRANSPARENCY_PEN,0);

	coll=0;cx=0;cyr=0;cyq=0;
	for (i=0; (i<32) & (!coll); ++i)
		for (j=0; (j<32) & (!coll); ++j)
			if ((read_pixel(bitmap, 256+i, 32+j) != Machine->pens[0]) &&
			    (read_pixel(bitmap, 256-32-marinedt_obj1_x+i, 256-marinedt_obj1_y+j-1) != Machine->pens[0]))
				coll=0x08;

if(coll)
{
--i;--j;
/*	plot_pixel(bitmap, 256-32-marinedt_obj1_x+i, 256-marinedt_obj1_y+j-1,Machine->pens[2]);*/

/*determine collision registers*/
{

	cx=((256-32-marinedt_obj1_x+i)%128)/8;cx&=0x0f;
/*-1 fudge?*/
	cyr=(((256-marinedt_obj1_y+j-1)%64)/8)*2+((256-32-marinedt_obj1_x+i)>127?1:0);cyr&=0x0f;
	cyq=(256-marinedt_obj1_y+j-1)/64;cyq&=0x0f;

	{
		sx = (cx+(cyr<<4)+(cyq<<8))%32;
		sy = (cx+(cyr<<4)+(cyq<<8))/32;
		flipx = 1;
		flipy = 0;

/*logerror("%x\n",videoram[offs]);*/

		drawgfx(bitmap,Machine->gfx[0],
				0,
				0,
				flipx,flipy,
				8*sx,8*sy,
				cliprect,TRANSPARENCY_NONE,0);
	}
}


}
/*{*/
/*	plot_pixel(bitmap, 256-28-marinedt_obj1_x+i, 256-2-marinedt_obj1_y+j,Machine->pens[1]);*/
/*			plot_pixel(bitmap, 256+i, 32+j,Machine->pens[1]);*/
/*}*/

/*	drawgfx(bitmap,Machine->gfx[1],*/
/*			((marinedt_obj1_a&0x04)<<1)+((marinedt_obj1_a&0x38)>>3),*/
/*			marinedt_obj1_a&0x03,*/
/*			1,marinedt_obj1_a&0x80,*/
/*			256,32,*/
/*			cliprect,TRANSPARENCY_PEN,0);*/

/*if(coll)*/
/*	drawgfx(bitmap,Machine->gfx[1],*/
/*			1,*/
/*			marinedt_obj1_a&0x03,*/
/*			1,0,*/
/*			256,96,*/
/*			cliprect,TRANSPARENCY_NONE,0);*/
/*else*/
/*	drawgfx(bitmap,Machine->gfx[2],*/
/*			1,*/
/*			marinedt_obj1_a&0x03,*/
/*			1,0,*/
/*			256,96,*/
/*			cliprect,TRANSPARENCY_NONE,0);*/

}
#endif
/* x------- flipx (really y cause of rotation?!)*/
/* -x------ flipy, unused ??*/
/* --xxx--- sprite*/
/* -----x-- bank*/
/* ------xx colour*/

/*note the 22 mod in the code, possibly for flipped*/
if (marinedt_pd&0x02)
		drawgfx(bitmap,Machine->gfx[2],
				((marinedt_obj2_a&0x04)<<1)+((marinedt_obj2_a&0x38)>>3),
				marinedt_obj2_a&0x03,
				1,marinedt_obj2_a&0x80,
				256-32-marinedt_obj2_x,256-marinedt_obj2_y-1,
				cliprect,TRANSPARENCY_PEN,0);

/*at x+6 flush with the bottom, x-6 one off flush with the top*/
/*at y+6 2 off flush with the left, y-6 flush with the right*/
/*at y-1 overlap is equal at 5 pixels on left and right*/
/*at x overlap is 6 on left and 5 on right, x+1 may be correct*/
if (marinedt_pd&0x01)
		drawgfx(bitmap,Machine->gfx[1],
				((marinedt_obj1_a&0x04)<<1)+((marinedt_obj1_a&0x38)>>3),
				marinedt_obj1_a&0x03,
				1,marinedt_obj1_a&0x80,
				256-32-marinedt_obj1_x,256-marinedt_obj1_y-1,
				cliprect,TRANSPARENCY_PEN,0);
}


/*if(coll)*/
/*	plot_pixel(bitmap, 256-32-marinedt_obj1_x+i, 256-marinedt_obj1_y+j-1,Machine->pens[1]);*/
}

static MACHINE_DRIVER_START( marinedt )

	/* basic machine hardware */
	MDRV_CPU_ADD(Z80,10000000/4)
	MDRV_CPU_MEMORY(marinedt_readmem,marinedt_writemem)
	MDRV_CPU_PORTS(marinedt_readport,marinedt_writeport)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,1)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(4*8+32*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 4*8+32*8-1, 4*8, 32*8-1)
	MDRV_GFXDECODE(marinedt_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(64)

	MDRV_PALETTE_INIT(marinedt)
	MDRV_VIDEO_START(generic)
	MDRV_VIDEO_UPDATE(marinedt)

	/* sound hardware */
	/*discrete sound*/
MACHINE_DRIVER_END

/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( marinedt )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "mg01",     0x0000, 0x0800, CRC(ad09f04d) SHA1(932fc973b4a2fbbebd7e6437ed30c8444e3d4afb))
	ROM_LOAD( "mg02",     0x0800, 0x0800, CRC(555a2b0f) SHA1(143a8953ce5070c31dc4c1f623833b2a5a2cf657))
	ROM_LOAD( "mg03",     0x1000, 0x0800, CRC(2abc79b3) SHA1(1afb331a2c0e320b6d026bc5cb47a53ac3356c2a))
	ROM_LOAD( "mg04",     0x1800, 0x0800, CRC(be928364) SHA1(8d9ae71e2751c009187e41d84fbad9519ab551e1) )
	ROM_LOAD( "mg05",     0x2000, 0x0800, CRC(44cd114a) SHA1(833165c5c00c6e505acf29fef4a3ae3f9647b443) )
	ROM_LOAD( "mg06",     0x2800, 0x0800, CRC(a7e2c69b) SHA1(614fc479d13c1726382fe7b4b0379c1dd4915af0) )
	ROM_LOAD( "mg07",     0x3000, 0x0800, CRC(b85d1f9a) SHA1(4fd3e76b1816912df84477dba4655d395f5e7072) )

	ROM_REGION( 0x1800, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "mg09",     0x0000, 0x0800, CRC(f4c349ca) SHA1(077f65eeac616a778d6c42bb95677fa2892ab697) )
	ROM_LOAD( "mg10",     0x0800, 0x0800, CRC(b41251e3) SHA1(e125a971b401c78efeb4b03d0fab43e392d3fc14) )
	ROM_LOAD( "mg11",     0x1000, 0x0800, CRC(50d66dd7) SHA1(858d1d2a75e091b0e382d964c5e4ddcd8e6f07dd))

	ROM_REGION( 0x1000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "mg12",     0x0000, 0x1000, CRC(7c6486d5) SHA1(a7f17a803937937f05fc90621883a0fd44b297a0) )

	ROM_REGION( 0x1000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "mg13",     0x0000, 0x1000, CRC(17817044) SHA1(8c9b96620e3c414952e6d85c6e81b0df85c88e7a) )

	ROM_REGION( 0x0080, REGION_PROMS, 0 )
	ROM_LOAD( "mg14.bpr", 0x0000, 0x0020, CRC(f75f4e3a) SHA1(36e665987f475c57435fa8c224a2a3ce0c5e672b) )	/*char clr*/
	ROM_LOAD( "mg15.bpr", 0x0020, 0x0020, CRC(cd3ab489) SHA1(a77478fb94d0cf8f4317f89cc9579def7c294b4f) )	/*obj clr*/
	ROM_LOAD( "mg16.bpr", 0x0040, 0x0020, CRC(92c868bc) SHA1(483ae6f47845ddacb701528e82bd388d7d66a0fb) )	/*?? collisions*/
	ROM_LOAD( "mg17.bpr", 0x0060, 0x0020, CRC(13261a02) SHA1(050edd18e4f79d19d5206f55f329340432fd4099) )	/*?? table of increasing values*/
ROM_END

GAMEX( 1981, marinedt, 0, marinedt, marinedt, 0, ROT270, "Taito", "Marine Date", GAME_NO_SOUND | GAME_NOT_WORKING )
