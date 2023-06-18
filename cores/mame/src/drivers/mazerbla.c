/****************************************************************************

Mazer Blazer by Stern (c) 1983
Great Guns by Stern (c) 1983


Driver by Jarek Burczynski
2003.03.19


Issues:
======
Sprites leave trails in both ganes
Sprites should be transparent (color 0x0f)
Screen flickers heavily in Great Guns (double buffer issue?).


TO DO:
=====
- handle page flipping

- figure out the VCU modes used in "clr_r":
  0x13 -? sprites related
  0x03 -? could be collision detection (if there is such a thing)

- figure out how the palette is handled (partially done)

- find out if there are any CLUTs (might be the other unknown cases in mode 7)

- figure out what really should happen during VCU test in Great Guns (patched
  out at the moment) (btw. Mazer Blazer doesn't test VCU)

- add sound to Mazer Blazer - Speech processor is unknown chip


****************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

static data8_t *cfb_ram;

static UINT32 VCU_gfx_addr = 0;
static UINT32 VCU_gfx_param_addr = 0;
static UINT32 xpos=0, ypos=0, pix_xsize=0, pix_ysize=0;
static UINT8 color=0, color2=0, mode=0, plane=0;
static UINT8 vbank = 0; /* video page select signal, likely for double buffering ?*/

static UINT32 gfx_rom_bank = 0xff;	/* graphics ROMs are banked */




#define MAZERBLA 0x01
#define GREATGUN 0x02
static UINT8 game_id = 0; /* hacks per game */



static UINT8 lookup_RAM[0x100*4];



#include "vidhrdw/res_net.h"
/***************************************************************************

  Convert the color PROMs into a more useable format.


  bit 0 -- 10.0Kohm resistor--\
  bit 1 -- 4.7 Kohm resistor --+-- 3.6 Kohm pulldown resistor -- BLUE
  bit 2 -- 2.2 Kohm resistor --/

  bit 3 -- 10.0Kohm resistor--\
  bit 4 -- 4.7 Kohm resistor --+-- 3.6 Kohm pulldown resistor -- GREEN
  bit 5 -- 2.2 Kohm resistor --/

  bit 6 -- 4.7 Kohm resistor --+-- 3.6 Kohm pulldown resistor -- RED
  bit 7 -- 2.2 Kohm resistor --/

***************************************************************************/

static double weights_r[2], weights_g[3], weights_b[3];

static PALETTE_INIT( mazerbla )
{

	const int resistances_r[2]  = { 4700, 2200 };
	const int resistances_gb[3] = { 10000, 4700, 2200 };


/* just to calculate coefficients for later use */

	compute_resistor_weights(0,	255,	-1.0,
			3,	resistances_gb,	weights_g,	3600,	0,
			3,	resistances_gb,	weights_b,	3600,	0,
			2,	resistances_r,	weights_r,	3600,	0);

}


static struct mame_bitmap * tmpbitmaps[4];

VIDEO_START( mazerbla )
{
	tmpbitmaps[0] = auto_bitmap_alloc(Machine->drv->screen_width,Machine->drv->screen_height);
	tmpbitmaps[1] = auto_bitmap_alloc(Machine->drv->screen_width,Machine->drv->screen_height);
	tmpbitmaps[2] = auto_bitmap_alloc(Machine->drv->screen_width,Machine->drv->screen_height);
	tmpbitmaps[3] = auto_bitmap_alloc(Machine->drv->screen_width,Machine->drv->screen_height);

	if ((tmpbitmaps[0]==0) || (tmpbitmaps[1]==0) || (tmpbitmaps[2]==0) || (tmpbitmaps[3]==0))
	{
		log_cb(RETRO_LOG_DEBUG, LOGPRE "too bad - run out of memory in video_start() ");
		return 1;
	}

	return 0;
}

#if 0
static int dbg_info = 1;
static int dbg_gfx_e = 1;
static int dbg_clr_e = 0;
static int dbg_vbank = 1;
static int dbg_lookup = 4;	/*4= off*/

static int planes_enabled[4] = {1,1,1,1}; /*all enabled*/
#endif

#if 0
VIDEO_UPDATE( test_vcu )
{
	int j,trueorientation;
	char buf[128];

	UINT32 color_base=0;

	if (game_id==MAZERBLA)
		color_base = 0x80;	/* 0x80 constant: matches Mazer Blazer movie */

	if (game_id==GREATGUN)
		color_base = 0x0;


	fillbitmap(bitmap,0,NULL);
/*logerror("-->frame\n");*/


	if (planes_enabled[3])
		copybitmap(bitmap,tmpbitmaps[3],0,0,0,0,&Machine->visible_area,TRANSPARENCY_NONE, 0 );


	if (planes_enabled[2])
		copybitmap(bitmap,tmpbitmaps[2],0,0,0,0,&Machine->visible_area,TRANSPARENCY_PEN, Machine->pens[color_base] );
	fillbitmap(tmpbitmaps[2],Machine->pens[color_base],NULL);


	if (planes_enabled[1])
		copybitmap(bitmap,tmpbitmaps[1],0,0,0,0,&Machine->visible_area,TRANSPARENCY_PEN, Machine->pens[color_base] );
	fillbitmap(tmpbitmaps[1],Machine->pens[color_base],NULL);


	if (planes_enabled[0])
		copybitmap(bitmap,tmpbitmaps[0],0,0,0,0,&Machine->visible_area,TRANSPARENCY_PEN, Machine->pens[color_base] );
	fillbitmap(tmpbitmaps[0],Machine->pens[color_base],NULL);

	if (keyboard_pressed_memory(KEYCODE_1))	/* plane 1 */
	{
		planes_enabled[0] ^= 1;
	}
	if (keyboard_pressed_memory(KEYCODE_2))	/* plane 2 */
	{
		planes_enabled[1] ^= 1;
	}
	if (keyboard_pressed_memory(KEYCODE_3))	/* plane 3 */
	{
		planes_enabled[2] ^= 1;
	}
	if (keyboard_pressed_memory(KEYCODE_4))	/* plane 4 */
	{
		planes_enabled[3] ^= 1;
	}

	if (keyboard_pressed_memory(KEYCODE_I))	/* show/hide debug info */
	{
		dbg_info = !dbg_info;
	}

	if (keyboard_pressed_memory(KEYCODE_G))	/* enable gfx area handling */
	{
		dbg_gfx_e = !dbg_gfx_e;
	}

	if (keyboard_pressed_memory(KEYCODE_C))	/* enable color area handling */
	{
		dbg_clr_e = !dbg_clr_e;
	}

	if (keyboard_pressed_memory(KEYCODE_V))	/* draw only when vbank==dbg_vbank */
	{
		dbg_vbank ^= 1;
	}

	if (keyboard_pressed_memory(KEYCODE_L))	/* showlookup ram */
	{
		dbg_lookup = (dbg_lookup+1)%5;/*0,1,2,3, 4-off*/
	}


	if (dbg_info)
	{
		trueorientation = Machine->orientation;
		Machine->orientation = ROT0;

		sprintf(buf,"I-info, G-gfx, C-color, V-vbank, 1-4 enable planes");
		for (j = 0;j < 52;j++)
			drawgfx(bitmap,Machine->uifont,buf[j],0,0,0,10+6*j,4,0,TRANSPARENCY_NONE,0);

		sprintf(buf,"g:%1i c:%1i v:%1i vbk=%1i  planes=%1i%1i%1i%1i  ", dbg_gfx_e&1, dbg_clr_e&1, dbg_vbank, vbank&1,
			planes_enabled[0],
			planes_enabled[1],
			planes_enabled[2],
			planes_enabled[3] );

		for (j = 0;j < 32;j++)
			drawgfx(bitmap,Machine->uifont,buf[j],0,0,0,10+6*j,16,0,TRANSPARENCY_NONE,0);

		if (dbg_lookup!=4)
		{
			int lookup_offs = (dbg_lookup)*256; /*=0,1,2,3*256*/
			int y,x;

			for (y=0; y<16; y++)
			{
				memset(buf,0,128);
				sprintf( buf+strlen(buf), "%04x ", lookup_offs+y*16 );
				for (x=0; x<16; x++)
				{
					sprintf( buf+strlen(buf), "%02x ", lookup_RAM[ lookup_offs+x+y*16 ] );
				}
				for (j = 0;j < 55;j++)
					drawgfx(bitmap,Machine->uifont,buf[j],0,0,0,6*j,30+y*8,0,TRANSPARENCY_NONE,0);
			}
		}

		Machine->orientation = trueorientation;
	}

}
#endif


/* these two VIDEO_UPDATE()s will be joined one day */
VIDEO_UPDATE( greatgun )
{

	UINT32 color_base=0;

	if (game_id==MAZERBLA)
		color_base = 0x80;	/* 0x80 constant: matches Mazer Blazer movie */

	if (game_id==GREATGUN)
		color_base = 0x0;

/*fillbitmap(bitmap,0,NULL);*/

	copybitmap(bitmap,tmpbitmaps[3],0,0,0,0,&Machine->visible_area,TRANSPARENCY_NONE, 0 );
	copybitmap(bitmap,tmpbitmaps[2],0,0,0,0,&Machine->visible_area,TRANSPARENCY_PEN, Machine->pens[color_base] );
	copybitmap(bitmap,tmpbitmaps[1],0,0,0,0,&Machine->visible_area,TRANSPARENCY_PEN, Machine->pens[color_base] );
	copybitmap(bitmap,tmpbitmaps[0],0,0,0,0,&Machine->visible_area,TRANSPARENCY_PEN, Machine->pens[color_base] );
}

VIDEO_UPDATE( mazerbla )
{

	UINT32 color_base=0;

	if (game_id==MAZERBLA)
		color_base = 0x80;	/* 0x80 constant: matches Mazer Blazer movie */

	if (game_id==GREATGUN)
		color_base = 0x0;

/*fillbitmap(bitmap,0,NULL);*/

	copybitmap(bitmap,tmpbitmaps[3],0,0,0,0,&Machine->visible_area,TRANSPARENCY_NONE, 0 ); /*text*/
	copybitmap(bitmap,tmpbitmaps[2],0,0,0,0,&Machine->visible_area,TRANSPARENCY_PEN, Machine->pens[0] );
	copybitmap(bitmap,tmpbitmaps[1],0,0,0,0,&Machine->visible_area,TRANSPARENCY_PEN, Machine->pens[0] ); /*haircross*/
	copybitmap(bitmap,tmpbitmaps[0],0,0,0,0,&Machine->visible_area,TRANSPARENCY_PEN, Machine->pens[0] ); /*sprites*/
}


static UINT8 zpu_int_vector;

static WRITE_HANDLER( cfb_zpu_int_req_set_w )
{
	zpu_int_vector &= ~2;	/* clear D1 on INTA (interrupt acknowledge) */

	cpu_set_irq_line(0, 0, ASSERT_LINE);	/* main cpu interrupt (comes from CFB (generated at the start of INT routine on CFB) - vblank?) */
}

static READ_HANDLER( cfb_zpu_int_req_clr )
{
	zpu_int_vector |= 2;

	/* clear the INT line when there are no more interrupt requests */
	if (zpu_int_vector==0xff)
		cpu_set_irq_line(0, 0, CLEAR_LINE);

	return 0;
}


static int irq_callback(int irqline)
{
/* all data lines are tied to +5V via 10K resistors */
/* D1 is set to GND when INT comes from CFB */
/* D2 is set to GND when INT comes from ZPU board - from 6850 on schematics (RS232 controller) */

/* resulting vectors:
1111 11000 (0xf8)
1111 11010 (0xfa)
1111 11100 (0xfc)

note:
1111 11110 (0xfe) - cannot happen and is not handled by game */

	return (zpu_int_vector & ~1);	/* D0->GND is performed on CFB board */
}



static data8_t *cfb_zpu_sharedram;
static WRITE_HANDLER ( sharedram_CFB_ZPU_w )
{
	cfb_zpu_sharedram[offset] = data;
}
static READ_HANDLER ( sharedram_CFB_ZPU_r )
{
	return cfb_zpu_sharedram[offset];
}



static UINT8 ls670_0[4];
static UINT8 ls670_1[4];

static READ_HANDLER( ls670_0_r )
{
	/* set a timer to force synchronization after the read */
	timer_set(TIME_NOW, 0, NULL);

	return ls670_0[offset];
}

static void deferred_ls670_0_w(int param )
{
	int offset = (param>>8) & 255;
	int data = param & 255;

	ls670_0[offset] = data;
}

static WRITE_HANDLER( ls670_0_w )
{
	/* do this on a timer to let the CPUs synchronize */
	timer_set(TIME_NOW, (offset<<8) | data, deferred_ls670_0_w);
}



static READ_HANDLER( ls670_1_r )
{
	/* set a timer to force synchronization after the read */
	timer_set(TIME_NOW, 0, NULL);

	return ls670_1[offset];
}

static void deferred_ls670_1_w(int param )
{
	int offset = (param>>8) & 255;
	int data = param & 255;

	ls670_1[offset] = data;
}

static WRITE_HANDLER( ls670_1_w )
{
	/* do this on a timer to let the CPUs synchronize */
	timer_set(TIME_NOW, (offset<<8) | data, deferred_ls670_1_w);
}


/* bcd decoder used a input select (a mux) for reads from port 0x62 */
static UINT8 bcd_7445 = 0;

static WRITE_HANDLER(zpu_bcd_decoder_w)
{

/*
name:			Strobe(bcd_value)	BIT
---------------------------------------
ZPU switch 1	0					6
ZPU switch 2	0					7

dipsw 35		1					7
dipsw 34		1					6
dipsw 33		1					5
dipsw 32		1					4
dipsw 31		1					3
dipsw 30		1					2
dipsw 29		1					1
dipsw 28		1					0
dipsw 27		2					7
dipsw 26		2					6
...
dipsw 8			4					4
dipsw 7			4					3
dipsw 6			4					2
dipsw 5			4					1
dipsw 4			4					0

Right Coin Sw.	5					0
Left Coin Sw.	5					1
Player One		5					2
Player Two		5					3
Fire Button		5					4

Horizontal movement of gun is Strobe 6, Bits 0-7.
	Movement is from 0000 0000 to 1111 1111

Vertical movement of gun is Strobe 7, Bits 0-7.
	Movement is from 0000 0000 to 1111 1111


Great Guns has two guns and here is necessary support for second gun:

Horizontal movement of gun is Strobe 8, Bits 0-7.
	Movement is from 0000 0000 to 1111 1111

Vertical movement of gun is Strobe 9, Bits 0-7.
	Movement is from 0000 0000 to 1111 1111

*/
	bcd_7445 = data & 15;
}

static READ_HANDLER( zpu_inputs_r )
{
	UINT8 ret = 0;

	if (bcd_7445<10)
	{
		ret = readinputport( bcd_7445 );
	}
	return ret;
}





static WRITE_HANDLER(zpu_led_w)
{
	/* 0x6e - reset (offset = 0)*/
	/* 0x6f - set */
	set_led_status(0, offset&1 );
}
static WRITE_HANDLER(zpu_lamps_w)
{
	/* bit 4 = /LAMP0 */
	/* bit 5 = /LAMP1 */

	/*set_led_status(0, (data&0x10)>>4 );*/
	/*set_led_status(1, (data&0x20)>>4 );*/
}

static WRITE_HANDLER(zpu_coin_counter_w)
{
	/* bit 6 = coin counter */
	coin_counter_w(offset, (data&0x40)>>6 );
}

static PORT_READ_START( readport )
	{ 0x4c, 0x4f, ls670_1_r },
	{ 0x62, 0x62, zpu_inputs_r },
PORT_END

static PORT_WRITE_START( writeport )
	{ 0x4c, 0x4f, ls670_0_w },
	{ 0x60, 0x60, zpu_bcd_decoder_w },
	{ 0x68, 0x68, zpu_coin_counter_w },
	{ 0x6a, 0x6a, zpu_lamps_w },
	{ 0x6e, 0x6f, zpu_led_w },
PORT_END

static MEMORY_READ_START( readmem )
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0xc000, 0xc7ff, sharedram_CFB_ZPU_r },
	{ 0xd800, 0xd800, cfb_zpu_int_req_clr },
	{ 0xe000, 0xe7ff, MRA_RAM },
	{ 0xe800, 0xefff, MRA_RAM },
MEMORY_END

static MEMORY_WRITE_START( writemem )
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0xc000, 0xc7ff, sharedram_CFB_ZPU_w },
	{ 0xe000, 0xe7ff, MWA_RAM, &videoram, &videoram_size },
	{ 0xe800, 0xefff, MWA_RAM },
MEMORY_END



static UINT8 vsb_ls273;
static WRITE_HANDLER( vsb_ls273_audio_control_w )
{
	vsb_ls273 = data;

	/* bit 5 - led on */
	set_led_status(1,(data&0x20)>>5);
}


static PORT_READ_START( readport_cpu2 )
	{ 0x80, 0x83, ls670_0_r },
PORT_END

static PORT_WRITE_START( writeport_cpu2 )
	{ 0x00, 0x00, vsb_ls273_audio_control_w },
	{ 0x80, 0x83, ls670_1_w },
PORT_END

static MEMORY_READ_START( readmem_cpu2 )
	{ 0x0000, 0x1fff, MRA_ROM },
	{ 0x4000, 0x43ff, MRA_RAM },
	{ 0x8000, 0x83ff, MRA_RAM },
MEMORY_END

static MEMORY_WRITE_START( writemem_cpu2 )
	{ 0x0000, 0x1fff, MWA_ROM },
	{ 0x4000, 0x43ff, MWA_RAM }, /* main RAM (stack) */
	{ 0x8000, 0x83ff, MWA_RAM }, /* waveform ???*/
MEMORY_END





/* Color Frame Buffer PCB */
static WRITE_HANDLER ( cfb_ram_w )
{
	cfb_ram[offset] = data;
}
static READ_HANDLER ( cfb_ram_r )
{
	return cfb_ram[offset];
}


static WRITE_HANDLER(cfb_led_w)
{
	/* bit 7 - led on */
	set_led_status(2,(data&0x80)>>7);
}


static UINT8 bknd_col = 0xaa;
static WRITE_HANDLER(cfb_backgnd_color_w)
{

	if (bknd_col != data)
	{
		int r,g,b, bit0, bit1, bit2;

		bknd_col = data;

		/* red component */
		bit1 = (data >> 7) & 0x01;
		bit0 = (data >> 6) & 0x01;
		r = combine_2_weights(weights_r, bit0, bit1);

		/* green component */
		bit2 = (data >> 5) & 0x01;
		bit1 = (data >> 4) & 0x01;
		bit0 = (data >> 3) & 0x01;
		g = combine_3_weights(weights_g, bit0, bit1, bit2);

		/* blue component */
		bit2 = (data >> 2) & 0x01;
		bit1 = (data >> 1) & 0x01;
		bit0 = (data >> 0) & 0x01;
		b = combine_3_weights(weights_b, bit0, bit1, bit2);

		palette_set_color(255, r, g, b);
		/*logerror("background color (port 01) write=%02x\n",data);*/
	}
}


static WRITE_HANDLER(cfb_vbank_w)
{
	data = (data & 0x40)>>6;	/* only bit 6 connected */
	if (vbank != data)
	{
		vbank = data;
		/*logerror("vbank=%1x\n",vbank);*/
	}
}


static WRITE_HANDLER(cfb_rom_bank_sel_w)	/* mazer blazer */
{
	gfx_rom_bank = data;

	cpu_setbank( 1, memory_region(REGION_CPU3) + (gfx_rom_bank * 0x2000) + 0x10000 );
}
static WRITE_HANDLER(cfb_rom_bank_sel_w_gg)	/* great guns */
{
	gfx_rom_bank = data>>1;

	cpu_setbank( 1, memory_region(REGION_CPU3) + (gfx_rom_bank * 0x2000) + 0x10000 );
}


/* ????????????? */
static UINT8 port02_status = 0;
static READ_HANDLER( cfb_port_02_r )
{
	port02_status ^= 0xff;
	return (port02_status);
}

static PORT_READ_START( readport_cpu3 )
	{ 0x02, 0x02, cfb_port_02_r },	/* VCU status ? */
PORT_END

static PORT_WRITE_START( writeport_cpu3_mb )
	{ 0x01, 0x01, cfb_backgnd_color_w },
	{ 0x02, 0x02, cfb_led_w },
	{ 0x03, 0x03, cfb_zpu_int_req_set_w },
	{ 0x04, 0x04, cfb_rom_bank_sel_w },
	{ 0x05, 0x05, cfb_vbank_w },	/*visible/writable videopage select?*/
PORT_END

/* Great Guns has a little different banking layout */
static PORT_WRITE_START( writeport_cpu3_gg )
	{ 0x00, 0x00, IOWP_NOP },
	{ 0x01, 0x01, cfb_backgnd_color_w },
	{ 0x02, 0x02, cfb_led_w },
	{ 0x03, 0x03, cfb_zpu_int_req_set_w },
	{ 0x04, 0x04, cfb_rom_bank_sel_w_gg },
	{ 0x05, 0x05, cfb_vbank_w },	/*visible/writable videopage select?*/
PORT_END



static UINT8 VCU_video_reg[4];
static WRITE_HANDLER( VCU_video_reg_w )
{
	if (VCU_video_reg[offset] != data)
	{
		VCU_video_reg[offset] = data;
		/*usrintf_showmessage("video_reg= %02x %02x %02x %02x", VCU_video_reg[0], VCU_video_reg[1], VCU_video_reg[2], VCU_video_reg[3] );*/
		/*logerror("video_reg= %02x %02x %02x %02x\n", VCU_video_reg[0], VCU_video_reg[1], VCU_video_reg[2], VCU_video_reg[3] );*/
	}
}

static READ_HANDLER( VCU_set_cmd_param_r )
{
	VCU_gfx_param_addr = offset;

								/* offset  = 0 is not known */
	xpos      = cfb_ram[VCU_gfx_param_addr + 1] | (cfb_ram[VCU_gfx_param_addr + 2]<<8);
	ypos      = cfb_ram[VCU_gfx_param_addr + 3] | (cfb_ram[VCU_gfx_param_addr + 4]<<8);
	color     = cfb_ram[VCU_gfx_param_addr + 5];
	color2    = cfb_ram[VCU_gfx_param_addr + 6];
	mode      = cfb_ram[VCU_gfx_param_addr + 7];
	pix_xsize = cfb_ram[VCU_gfx_param_addr + 8];
	pix_ysize = cfb_ram[VCU_gfx_param_addr + 9];

	plane = mode & 3;

	return 0;
}


static READ_HANDLER( VCU_set_gfx_addr_r )
{
int offs;
int x,y;
int bits = 0;

UINT8 color_base=0;

unsigned char * rom = memory_region(REGION_CPU3) + (gfx_rom_bank * 0x2000) + 0x10000;

/*
	if ((mode<=0x07) || (mode>=0x10))
	{
		log_cb(RETRO_LOG_DEBUG, LOGPRE "paradr=");
		log_cb(RETRO_LOG_DEBUG, LOGPRE "%3x ",VCU_gfx_param_addr );

		log_cb(RETRO_LOG_DEBUG, LOGPRE "%02x ", cfb_ram[VCU_gfx_param_addr + 0] );
		log_cb(RETRO_LOG_DEBUG, LOGPRE "x=%04x ", xpos );					//1,2
		log_cb(RETRO_LOG_DEBUG, LOGPRE "y=%04x ", ypos );					//3,4
		log_cb(RETRO_LOG_DEBUG, LOGPRE "color=%02x ", color);				//5
		log_cb(RETRO_LOG_DEBUG, LOGPRE "color2=%02x ", color2);			//6
		log_cb(RETRO_LOG_DEBUG, LOGPRE "mode=%02x ", mode );				//7
		log_cb(RETRO_LOG_DEBUG, LOGPRE "xpix=%02x ", pix_xsize );			//8
		log_cb(RETRO_LOG_DEBUG, LOGPRE "ypix=%02x ", pix_ysize );			//9

		log_cb(RETRO_LOG_DEBUG, LOGPRE "addr=%4i bank=%1i\n", offset, gfx_rom_bank);
	}
*/

	VCU_gfx_addr = offset;


/* draw */
	offs = VCU_gfx_addr;

	switch(mode)
	{
	/* 2 bits per pixel */
	case 0x0f:
	case 0x0e:
	case 0x0d:
	case 0x0c:
/*if (dbg_gfx_e)*/
/*{*/
	/*if (vbank==dbg_vbank)*/
	{
		if (game_id==MAZERBLA)
			color_base = 0x80;	/* 0x80 constant: matches Mazer Blazer movie */

		if (game_id==GREATGUN)
			color_base = 0x0;

		for (y = 0; y <= pix_ysize; y++)
		{
			for (x = 0; x <= pix_xsize; x++)
			{
				UINT8 pixeldata = rom[offs + (bits>>3)];
				UINT8 data = (pixeldata>>(6-(bits&7))) & 3;
				UINT8 col = 0;

				switch(data)
				{
				case 0:
					col = color_base | ((color &0x0f));		/*background PEN*/
					break;
				case 1:
					col = color_base | ((color &0xf0)>>4);	/*foreground PEN*/
					break;
				case 2:
					col = color_base | ((color2 &0x0f));	/*background PEN2*/
					break;
				case 3:
					col = color_base | ((color2 &0xf0)>>4);	/*foreground PEN2*/
					break;
				}

				if ( ((xpos+x)<256) && ((ypos+y)<256) )
				{
					plot_pixel(tmpbitmaps[plane], xpos+x, ypos+y, col );
				}

				bits+=2;
			}
		}
	}
/*}*/
	break;

	/* 1 bit per pixel */
	case 0x0b:/* verified - 1bpp ; used for 'cleaning' using color 0xff */
	case 0x0a:/* verified - 1bpp */
	case 0x09:/* verified - 1bpp: gun crosshair */
	case 0x08:/* */
/*if (dbg_gfx_e)*/
/*{*/
	/*if (vbank==dbg_vbank)*/
	{
		if (game_id==MAZERBLA)
			color_base = 0x80;	/* 0x80 - good for Mazer Blazer: (only in game, CRT test mode is bad) */

		if (game_id==GREATGUN)
			color_base = 0x0;	/* 0x00 - good for Great Guns: (both in game and CRT test mode) */
		for (y = 0; y <= pix_ysize; y++)
		{
			for (x = 0; x <= pix_xsize; x++)
			{
				UINT8 pixeldata = rom[offs + (bits>>3)];
				UINT8 data = (pixeldata>>(7-(bits&7))) & 1;

				/* color = 4 MSB = front PEN, 4 LSB = background PEN */

				if ( ((xpos+x)<256) && ((ypos+y)<256) )
				{
					plot_pixel(tmpbitmaps[plane], xpos+x, ypos+y, data? color_base | ((color&0xf0)>>4): color_base | ((color&0x0f)) );
				}

				bits+=1;
			}
		}
	}
/*}*/
	break;

	/* 4 bits per pixel */
	case 0x03:
	case 0x01:
	case 0x00:
/*if (dbg_gfx_e)*/
/*{*/
	/*if (vbank==dbg_vbank)*/
	{
		if (game_id==MAZERBLA)
			color_base = 0x80;	/* 0x80 - good for Mazer Blazer: (only in game, CRT test mode is bad) */

		if (game_id==GREATGUN)
			color_base = 0x0;	/* 0x00 - good for Great Guns: (both in game and CRT test mode) */

		for (y = 0; y <= pix_ysize; y++)
		{
			for (x = 0; x <= pix_xsize; x++)
			{
				UINT8 pixeldata = rom[offs + (bits>>3)];
				UINT8 data = (pixeldata>>(4-(bits&7))) & 15;
				UINT8 col = 0;

				col = color_base | data;

				if ( ((xpos+x)<256) && ((ypos+y)<256) )
				{
					plot_pixel(tmpbitmaps[plane], xpos+x, ypos+y, col );
				}

				bits+=4;
			}
		}
	}
/*}*/
	break;
	default:
		usrintf_showmessage("not supported VCU drawing mode=%2x", mode);
	break;
	}
	return 0;
}

static READ_HANDLER( VCU_set_clr_addr_r )
{
int offs;
int x,y;
int bits = 0;

UINT8 color_base=0;

unsigned char * rom = memory_region(REGION_CPU3) + (gfx_rom_bank * 0x2000) + 0x10000;

/*
	if (0) */ /*(mode != 0x07)
	{
		log_cb(RETRO_LOG_DEBUG, LOGPRE "paladr=");
		log_cb(RETRO_LOG_DEBUG, LOGPRE "%3x ",VCU_gfx_param_addr );

		log_cb(RETRO_LOG_DEBUG, LOGPRE "%02x ", cfb_ram[VCU_gfx_param_addr + 0] );
		log_cb(RETRO_LOG_DEBUG, LOGPRE "x=%04x ", xpos );					//1,2
		log_cb(RETRO_LOG_DEBUG, LOGPRE "y=%04x ", ypos );					//3,4
		log_cb(RETRO_LOG_DEBUG, LOGPRE "color=%02x ", color);				//5
		log_cb(RETRO_LOG_DEBUG, LOGPRE "color2=%02x ", color2 );			//6
		log_cb(RETRO_LOG_DEBUG, LOGPRE "mode=%02x ", mode );				//7
		log_cb(RETRO_LOG_DEBUG, LOGPRE "xpix=%02x ", pix_xsize );			//8
		log_cb(RETRO_LOG_DEBUG, LOGPRE "ypix=%02x ", pix_ysize );			//9

		log_cb(RETRO_LOG_DEBUG, LOGPRE "addr=%4i bank=%1i\n", offset, gfx_rom_bank);

		for (y=0; y<16; y++)
		{
			log_cb(RETRO_LOG_DEBUG, LOGPRE "%04x: ",offset+y*16);
			for (x=0; x<16; x++)
			{
				log_cb(RETRO_LOG_DEBUG, LOGPRE "%02x ",cfb_ram[offset+x+y*16]);
			}
			log_cb(RETRO_LOG_DEBUG, LOGPRE "\n");
		}
	}

*/

/* copy palette / CLUT(???) */


	switch(mode)
	{
	case 0x13: /* draws sprites?? in mazer blazer and ... wrong sprite in place of targeting-cross and UFO laser */
	case 0x03:
/* ... this may proove that there is really only one area and that
 the draw command/palette selector is done via the 'mode' only ... */
	/*if (dbg_clr_e)*/
	{
		offs = VCU_gfx_addr;

		if (game_id==MAZERBLA)
			color_base = 0x80;	/* 0x80 constant: matches Mazer Blazer movie */

		if (game_id==GREATGUN)
			color_base = 0x0;

		for (y = 0; y <= pix_ysize; y++)
		{
			for (x = 0; x <= pix_xsize; x++)
			{
				UINT8 pixeldata = rom[offs + (bits>>3)];
				UINT8 data = (pixeldata>>(6-(bits&7))) & 3;
				UINT8 col = 0;

				switch(data)
				{
				case 0:
					col = color_base | ((color &0x0f));		/*background PEN*/
					break;
				case 1:
					col = color_base | ((color &0xf0)>>4);	/*foreground PEN*/
					break;
				case 2:
					col = color_base | ((color2 &0x0f));	/*background PEN2*/
					break;
				case 3:
					col = color_base | ((color2 &0xf0)>>4);	/*foreground PEN2*/
					break;
				}

				if ( ((xpos+x)<256) && ((ypos+y)<256) )
				{
					plot_pixel(tmpbitmaps[plane], xpos+x, ypos+y, col );
				}

				bits+=2;
			}
		}
	}
	break;

/* Palette / "something else" write mode */
	case 0x07:

		offs = offset;

		switch(ypos)
		{
		case 6: /*seems to encode palete write*/
			{
				int r,g,b, bit0, bit1, bit2;

/*pix_xsize and pix_ysize seem to be related to palette length ? (divide by 2)*/
				int lookup_offs = (ypos>>1)*256; /*=3*256*/

				for (y=0; y<16; y++)
				{
					for (x=0; x<16; x++)
					{
						UINT8 colour = cfb_ram[ offs + x + y*16 ];

						/* red component */
						bit1 = (colour >> 7) & 0x01;
						bit0 = (colour >> 6) & 0x01;
						r = combine_2_weights(weights_r, bit0, bit1);

						/* green component */
						bit2 = (colour >> 5) & 0x01;
						bit1 = (colour >> 4) & 0x01;
						bit0 = (colour >> 3) & 0x01;
						g = combine_3_weights(weights_g, bit0, bit1, bit2);

						/* blue component */
						bit2 = (colour >> 2) & 0x01;
						bit1 = (colour >> 1) & 0x01;
						bit0 = (colour >> 0) & 0x01;
						b = combine_3_weights(weights_b, bit0, bit1, bit2);

						if ((x+y*16)<255)/*keep color 255 free for use as background color*/
							palette_set_color(x+y*16, r, g, b);

						lookup_RAM[ lookup_offs + x + y*16 ] = colour;
					}
				}
			}
		break;
		case 4: /*seems to encode lookup???? table write*/
			{
				int lookup_offs = (ypos>>1)*256; /*=2*256*/

				for (y=0; y<16; y++)
				{
					for (x=0; x<16; x++)
					{
						UINT8 dat = cfb_ram[ offs + x + y*16 ];
						lookup_RAM[ lookup_offs + x + y*16 ] = dat;
					}
				}
			}
		break;
		case 2: /*seems to encode lookup???? table write*/
			{
				int lookup_offs = (ypos>>1)*256; /*=1*256*/

				for (y=0; y<16; y++)
				{
					for (x=0; x<16; x++)
					{
						UINT8 dat = cfb_ram[ offs + x + y*16 ];
						lookup_RAM[ lookup_offs + x + y*16 ] = dat;
					}
				}
			}
		break;
		case 0: /*seems to encode lookup???? table write*/
			{
				int lookup_offs = (ypos>>1)*256; /*=0*256*/

				for (y=0; y<16; y++)
				{
					for (x=0; x<16; x++)
					{
						UINT8 dat = cfb_ram[ offs + x + y*16 ];
						lookup_RAM[ lookup_offs + x + y*16 ] = dat;
					}
				}
			}
		break;

		default:
			usrintf_showmessage("not supported lookup/color write mode=%2x", ypos);
		break;
	}
	break;

	default:
		usrintf_showmessage("not supported VCU color mode=%2x", mode);
	break;

	}

	return 0;
}

static MEMORY_READ_START( readmem_cpu3 )
	{ 0x0000, 0x37ff, MRA_ROM },
	{ 0x3800, 0x3fff, sharedram_CFB_ZPU_r },
	{ 0x4000, 0x5fff, MRA_BANK1 },				/* GFX roms */
	{ 0x6000, 0x67ff, cfb_ram_r },				/* RAM for VCU commands and parameters */

	{ 0xa000, 0xa7ff, VCU_set_cmd_param_r },	/* VCU command and parameters LOAD */
	{ 0xc000, 0xdfff, VCU_set_gfx_addr_r },		/* gfx LOAD (blit) */
	{ 0xe000, 0xffff, VCU_set_clr_addr_r },		/* palette? LOAD */
MEMORY_END

static MEMORY_WRITE_START( writemem_cpu3 )
	{ 0x0000, 0x37ff, MWA_ROM },
	{ 0x3800, 0x3fff, sharedram_CFB_ZPU_w, &cfb_zpu_sharedram },
	{ 0x4000, 0x4003, VCU_video_reg_w },
	{ 0x6000, 0x67ff, cfb_ram_w, &cfb_ram },
MEMORY_END






/* Great Guns */

static UINT8 soundlatch;

static READ_HANDLER( soundcommand_r )
{
	return soundlatch;
}

static void delayed_sound_w(int param)
{
	soundlatch = param;

	/* cause NMI on sound CPU */
	cpu_set_nmi_line(1, ASSERT_LINE);
}


static WRITE_HANDLER( main_sound_w )
{
	timer_set(TIME_NOW, data & 0xff, delayed_sound_w);
}


static PORT_READ_START( gg_readport )
	{ 0x62, 0x62, zpu_inputs_r },
PORT_END
static PORT_WRITE_START( gg_writeport )
	{ 0x4c, 0x4c, main_sound_w },
	{ 0x60, 0x60, zpu_bcd_decoder_w },
	{ 0x66, 0x66, IOWP_NOP },
	{ 0x68, 0x68, IOWP_NOP },

	{ 0x6e, 0x6f, zpu_led_w },
PORT_END




/* frequency is 14.318 MHz/16/16/16/16 */
static INTERRUPT_GEN( sound_interrupt )
{
	cpu_set_irq_line(1, 0, ASSERT_LINE);
}

static WRITE_HANDLER( sound_int_clear_w )
{
	cpu_set_irq_line(1, 0, CLEAR_LINE);
}
static WRITE_HANDLER( sound_nmi_clear_w )
{
	cpu_set_nmi_line(1, CLEAR_LINE);
}

static WRITE_HANDLER( gg_led_ctrl_w )
{
	/* bit 0, bit 1 - led on */
	set_led_status(1,data&0x01);
}

static MEMORY_READ_START( sound_readmem )
	{ 0x0000, 0x1fff, MRA_ROM },
	{ 0x2000, 0x27ff, MRA_RAM },
	{ 0x4000, 0x4000, AY8910_read_port_0_r },
MEMORY_END

static MEMORY_WRITE_START( sound_writemem )
	{ 0x0000, 0x1fff, MWA_ROM },
	{ 0x2000, 0x27ff, MWA_RAM }, /* main RAM (stack) */

	{ 0x4000, 0x4000, AY8910_control_port_0_w },
	{ 0x4001, 0x4001, AY8910_write_port_0_w },
	{ 0x6000, 0x6000, AY8910_control_port_1_w },
	{ 0x6001, 0x6001, AY8910_write_port_1_w },

	{ 0x8000, 0x8000, sound_int_clear_w },
	{ 0xa000, 0xa000, sound_nmi_clear_w },
MEMORY_END






INPUT_PORTS_START( mazerbla )
	PORT_START	/* Strobe 0: ZPU Switches */
	PORT_DIPNAME( 0x40, 0x40, "ZPU Switch 1" )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, "ZPU Switch 2" )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START	/* Strobe 1: Dip Switches 28-35*/
	PORT_DIPNAME( 0x03, 0x00, DEF_STR( Lives ) )
	PORT_DIPSETTING(	0x03, "6" )
	PORT_DIPSETTING(	0x02, "5" )
	PORT_DIPSETTING(	0x01, "4" )
	PORT_DIPSETTING(	0x00, "3" )
	PORT_DIPNAME( 0x0c, 0x00, "Freeze Time" )
	PORT_DIPSETTING(	0x0c, "1.5 seconds" )
	PORT_DIPSETTING(	0x08, "2.0 seconds" )
	PORT_DIPSETTING(	0x04, "2.5 seconds" )
	PORT_DIPSETTING(	0x00, "3.0 seconds" )
	PORT_DIPNAME( 0x30, 0x00, "Number of points for extra frezze & first life" )
	PORT_DIPSETTING(	0x30, "20000" )
	PORT_DIPSETTING(	0x20, "25000" )
	PORT_DIPSETTING(	0x10, "30000" )
	PORT_DIPSETTING(	0x00, "35000" )
	PORT_DIPNAME( 0xc0, 0x00, "Number of points for extra life other than first" )
	PORT_DIPSETTING(	0xc0, "40000" )
	PORT_DIPSETTING(	0x80, "50000" )
	PORT_DIPSETTING(	0x40, "60000" )
	PORT_DIPSETTING(	0x00, "70000" )

	PORT_START	/* Strobe 2: Dip Switches 20-27*/
	PORT_DIPNAME( 0x0f, 0x0f, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 4C_3C ) )
	PORT_DIPSETTING(    0x0f, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 4C_5C ) )
	PORT_DIPSETTING(    0x05, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 4C_7C ) )
	PORT_DIPSETTING(    0x0e, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 2C_5C ) )
	PORT_DIPSETTING(    0x0d, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 2C_7C ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x0b, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0x0a, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(    0x09, DEF_STR( 1C_7C ) )
	PORT_DIPSETTING(    0x08, "1 Coin/10 Credits" )
	PORT_DIPSETTING(    0x07, "1 Coin/14 Credits" )

	PORT_DIPNAME( 0xf0, 0xf0, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x60, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 4C_3C ) )
	PORT_DIPSETTING(    0xf0, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 4C_5C ) )
	PORT_DIPSETTING(    0x50, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 4C_7C ) )
	PORT_DIPSETTING(    0xe0, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 2C_5C ) )
	PORT_DIPSETTING(    0xd0, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 2C_7C ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0xb0, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0xa0, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(    0x90, DEF_STR( 1C_7C ) )
	PORT_DIPSETTING(    0x80, "1 Coin/10 Credits" )
	PORT_DIPSETTING(    0x70, "1 Coin/14 Credits" )

	PORT_START	/* Strobe 3: Dip Switches 12-19*/
	PORT_DIPNAME( 0x01, 0x01, "Service Index" )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, "Switch Test" )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Free_Play ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, "Player Immortality" )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, "Super Shot" )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )	/*probably unused*/
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )	/*probably unused*/
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START	/* Strobe 4: Dip Switches 4-11 */
	PORT_DIPNAME( 0x03, 0x02, "Number of Freezes" )
	PORT_DIPSETTING(	0x03, "4" )
	PORT_DIPSETTING(	0x02, "3" )
	PORT_DIPSETTING(	0x01, "2" )
	PORT_DIPSETTING(	0x00, "1" )
	PORT_DIPNAME( 0x04, 0x04, "Gun Knocker" )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	/*dips 7-11 - not listed in manual*/
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )	/*probably unused*/
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )	/*probably unused*/
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )	/*probably unused*/
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )	/*probably unused*/
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )	/*probably unused*/
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START	/* Strobe 5: coin1&2, start1&2, fire */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* Strobe 6: horizontal movement of gun */
	PORT_ANALOG( 0xff, 0x80, IPT_LIGHTGUN_X | IPF_PLAYER1 | IPF_REVERSE, 25, 7, 0, 255)
	PORT_START	/* Strobe 7: vertical movement of gun */
	PORT_ANALOG( 0xff, 0x80, IPT_LIGHTGUN_Y | IPF_PLAYER1, 25, 7, 0, 255)

/* Mazer Blazer cabinet has only one gun, really */
	PORT_START	/* Strobe 8: horizontal movement of gun */
	PORT_ANALOG( 0xff, 0x80, IPT_LIGHTGUN_X | IPF_PLAYER2 | IPF_REVERSE, 25, 7, 0, 255)
	PORT_START	/* Strobe 9: vertical movement of gun */
	PORT_ANALOG( 0xff, 0x80, IPT_LIGHTGUN_Y | IPF_PLAYER2, 25, 7, 0, 255)
INPUT_PORTS_END

INPUT_PORTS_START( greatgun )
	PORT_START	/* Strobe 0: ZPU Switches */
	PORT_DIPNAME( 0x40, 0x40, "ZPU Switch 1" )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, "ZPU Switch 2" )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START	/* Strobe 1: Dip Switches 28-35*/
	PORT_DIPNAME( 0x03, 0x00, "Starting Number of Bullets/Credit" )
	PORT_DIPSETTING(	0x03, "60" )
	PORT_DIPSETTING(	0x02, "70" )
	PORT_DIPSETTING(	0x01, "80" )
	PORT_DIPSETTING(	0x00, "90" )
	PORT_DIPNAME( 0x0c, 0x00, "Target Size" )
	PORT_DIPSETTING(	0x0c, "7 x 7" )
	PORT_DIPSETTING(	0x08, "9 x 9" )
	PORT_DIPSETTING(	0x04, "11x11" )
	PORT_DIPSETTING(	0x00, "7 x 7" )
	PORT_DIPNAME( 0x70, 0x00, "Number of points for extra bullet" )
	PORT_DIPSETTING(	0x70, "1000" )
	PORT_DIPSETTING(	0x60, "2000" )
	PORT_DIPSETTING(	0x50, "3000" )
	PORT_DIPSETTING(	0x40, "4000" )
	PORT_DIPSETTING(	0x30, "5000" )
	PORT_DIPSETTING(	0x20, "6000" )
	PORT_DIPSETTING(	0x10, "7000" )
	PORT_DIPSETTING(	0x00, "8000" )
	/* from manual:
		"This switch is used when an optional coin return or ticket dispenser is used"
	*/
	PORT_DIPNAME( 0x80, 0x00, "Number of coins or tickets returned" )
	PORT_DIPSETTING(	0x80, "1" )
	PORT_DIPSETTING(	0x00, "2" )

	PORT_START	/* Strobe 2: Dip Switches 20-27*/
	PORT_DIPNAME( 0x0f, 0x0f, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 4C_3C ) )
	PORT_DIPSETTING(    0x0f, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 4C_5C ) )
	PORT_DIPSETTING(    0x05, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 4C_7C ) )
	PORT_DIPSETTING(    0x0e, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 2C_5C ) )
	PORT_DIPSETTING(    0x0d, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 2C_7C ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x0b, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0x0a, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(    0x09, DEF_STR( 1C_7C ) )
	PORT_DIPSETTING(    0x08, "1 Coin/10 Credits" )
	PORT_DIPSETTING(    0x07, "1 Coin/14 Credits" )

	PORT_DIPNAME( 0xf0, 0xf0, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x60, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 4C_3C ) )
	PORT_DIPSETTING(    0xf0, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 4C_5C ) )
	PORT_DIPSETTING(    0x50, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 4C_7C ) )
	PORT_DIPSETTING(    0xe0, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 2C_5C ) )
	PORT_DIPSETTING(    0xd0, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 2C_7C ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0xb0, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0xa0, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(    0x90, DEF_STR( 1C_7C ) )
	PORT_DIPSETTING(    0x80, "1 Coin/10 Credits" )
	PORT_DIPSETTING(    0x70, "1 Coin/14 Credits" )

	PORT_START	/* Strobe 3: Dip Switches 12-19*/
	PORT_DIPNAME( 0x01, 0x01, "Service Index" )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, "Switch Test" )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Free_Play ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, "Player Immortality" )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, "Rack Advance" )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )	/*probably unused*/
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )	/*probably unused*/
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START	/* Strobe 4: Dip Switches 4-11 */
	PORT_DIPNAME( 0x01, 0x01, "Free game/coin return" )
	PORT_DIPSETTING(	0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	/*dips 5-11 - not listed in manual*/
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING(	0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START	/* Strobe 5: coin1&2, start1&2, fire */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON4 )

	PORT_START	/* Strobe 6: horizontal movement of gun */
	PORT_ANALOG( 0xff, 0x80, IPT_LIGHTGUN_X | IPF_PLAYER1, 25, 7, 0, 255)
	PORT_START	/* Strobe 7: vertical movement of gun */
	PORT_ANALOG( 0xff, 0x80, IPT_LIGHTGUN_Y | IPF_PLAYER1, 25, 7, 0, 255)

	PORT_START	/* Strobe 8: horizontal movement of gun */
	PORT_ANALOG( 0xff, 0x80, IPT_LIGHTGUN_X | IPF_PLAYER2, 25, 7, 0, 255)
	PORT_START	/* Strobe 9: vertical movement of gun */
	PORT_ANALOG( 0xff, 0x80, IPT_LIGHTGUN_Y | IPF_PLAYER2, 25, 7, 0, 255)
INPUT_PORTS_END




static MACHINE_INIT( mazerbla )
{
	game_id = MAZERBLA;
	zpu_int_vector = 0xff;
	cpu_set_irq_callback(0, irq_callback);
}


static MACHINE_INIT( greatgun )
{
	game_id = GREATGUN;
	zpu_int_vector = 0xff;
	cpu_set_irq_callback(0, irq_callback);


/*patch VCU test*/
/*VCU test starts at PC=0x56f*/
	memory_region(REGION_CPU3)[0x05b6] = 0;
	memory_region(REGION_CPU3)[0x05b7] = 0;
/*so we also need to patch ROM checksum test*/
	memory_region(REGION_CPU3)[0x037f] = 0;
	memory_region(REGION_CPU3)[0x0380] = 0;
}


/* only Great Guns */
static struct AY8910interface ay8912_interface =
{
	2,	/* 2 chips */
	14318000 / 8,
	{ 30, 100 },
	{ 0, 0 },	/* port Aread */
	{ soundcommand_r, 0 },	/* port Bread */
	{ /*ay0_output_ctrl_w*/0,/*ay1_output_ctrl_w*/ 0 },	/* port Awrite */
	{ 0, gg_led_ctrl_w }	/* port Bwrite */
};


static MACHINE_DRIVER_START( mazerbla )
	/* basic machine hardware */
	MDRV_CPU_ADD(Z80, 4000000)	/* 4 MHz, no NMI, IM2 - vectors at 0xf8, 0xfa, 0xfc */
	MDRV_CPU_MEMORY(readmem,writemem)
	MDRV_CPU_PORTS(readport,writeport)

	MDRV_CPU_ADD(Z80, 4000000)	/* 4 MHz, NMI, IM1 INT */
	MDRV_CPU_MEMORY(readmem_cpu2,writemem_cpu2)
	MDRV_CPU_PORTS(readport_cpu2,writeport_cpu2)
/*MDRV_CPU_PERIODIC_INT(irq0_line_hold, 400 )  // frequency in Hz /*/

	MDRV_CPU_ADD(Z80, 4000000)	/* 4 MHz, no  NMI, IM1 INT */
	MDRV_CPU_MEMORY(readmem_cpu3,writemem_cpu3)
	MDRV_CPU_PORTS(readport_cpu3,writeport_cpu3_mb)
/* (vblank related ??) int generated by a custom video processor
and cleared on ANY port access.
but handled differently for now
*/
	MDRV_CPU_VBLANK_INT(irq0_line_hold,1)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)

	/* synchronization forced on the fly */

	MDRV_MACHINE_INIT(mazerbla)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(40*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 0*8, 32*8-1)
	MDRV_PALETTE_LENGTH(256)

	MDRV_PALETTE_INIT(mazerbla)
	MDRV_VIDEO_START(mazerbla)
	MDRV_VIDEO_UPDATE(mazerbla)

	/* sound hardware */
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( greatgun )

	/* basic machine hardware */
	MDRV_CPU_ADD(Z80, 4000000)	/* 4 MHz, no NMI, IM2 - vectors at 0xf8, 0xfa, 0xfc */
	MDRV_CPU_MEMORY(readmem,writemem)
	MDRV_CPU_PORTS(gg_readport,gg_writeport)

	MDRV_CPU_ADD(Z80, 14318000 / 4)	/* 3.579500 MHz, NMI - caused by sound command write, periodic INT */
	MDRV_CPU_MEMORY(sound_readmem,sound_writemem)
	/* IRQ frequency is: 14318000.0 Hz/16/16/16/16 = 218.475341796875 Hz */
	/*   that is a period of 1000000000.0 / 218.475341796875 = 4577175.5831 ns */
	MDRV_CPU_PERIODIC_INT(sound_interrupt, 4577176 ) /* period in nanoseconds */

	MDRV_CPU_ADD(Z80, 4000000)	/* 4 MHz, no  NMI, IM1 INT */
	MDRV_CPU_MEMORY(readmem_cpu3,writemem_cpu3)
	MDRV_CPU_PORTS(readport_cpu3,writeport_cpu3_gg)
/* (vblank related ??) int generated by a custom video processor
and cleared on ANY port access.
but handled differently for now
*/
	MDRV_CPU_VBLANK_INT(irq0_line_hold,1)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)

	MDRV_MACHINE_INIT(greatgun)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(40*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 0*8, 32*8-1)
	MDRV_PALETTE_LENGTH(256)

	MDRV_PALETTE_INIT(mazerbla)
	MDRV_VIDEO_START(mazerbla)
	MDRV_VIDEO_UPDATE(greatgun)

	/* sound hardware */
	MDRV_SOUND_ADD(AY8910, ay8912_interface)
MACHINE_DRIVER_END


/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( mazerbla )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )     /* 64k for main CPU (ZPU board) */
	ROM_LOAD( "mblzpu0.bin",0x0000, 0x2000, CRC(82766187) SHA1(cfc425c87cccb84180f1091998eafeaede126d9d) )
	ROM_LOAD( "mblzpu1.bin",0x2000, 0x2000, CRC(8ba2b3f9) SHA1(1d203332e434d1d9821f98c6ac959ae65dcc51ef) )
	ROM_LOAD( "mblzpu2.bin",0x4000, 0x2000, CRC(48e5306c) SHA1(d27cc85d24c7b6c23c5c96be4dad5cae6e8069be) )
	ROM_LOAD( "mblzpu3.bin",0x6000, 0x2000, CRC(eba91546) SHA1(8c1da4e0d9b562dbbf7c7583dbf567c804eb670f) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )     /* 64k for sound CPU (VSB board) */
	ROM_LOAD( "mblvsb0.bin",0x0000, 0x1000, CRC(0cf7a1c3) SHA1(af27e3a3b51d03d46c62c2797268744d0577d075) )
	ROM_LOAD( "mblvsb1.bin",0x1000, 0x1000, CRC(0b8d0e43) SHA1(b3ddb7561e715a58ca512fe76e53cda39402a8e4) )

	ROM_REGION( 0x18000, REGION_CPU3, 0 )     /* 64k for video CPU (CFB board) */
	ROM_LOAD( "mblrom0.bin",0x0000, 0x2000, CRC(948a2c5e) SHA1(d693f1b96caf31649f600c5038bb79b0d1d16133) )

	ROM_LOAD( "mblrom2.bin",0x10000,0x2000, CRC(36237058) SHA1(9db8fced37a3d40c4ea5b87ea18ac8e75d71e586) )/*banked at 0x4000 (select=0)*/
	ROM_LOAD( "mblrom3.bin",0x12000,0x2000, CRC(18d75d7f) SHA1(51c35ea4a2127439a1299863eb74e57be833e2e4) )/*banked at 0x4000 (select=1)*/
	/* empty socket??? (the *name* of next rom seems good ?) or wrong schematics ?*/
	ROM_LOAD( "mblrom4.bin",0x16000,0x2000, CRC(1805acdc) SHA1(40b8e70e6ba69ac864af0b276e81218e63e48deb) )/*banked at 0x4000 (select=3)*/
ROM_END


ROM_START( greatgun )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )     /* 64k for main CPU (ZPU board) */
	ROM_LOAD( "zpu0",0x0000, 0x2000, CRC(80cf2cbf) SHA1(ea24b844ea6d8fc54adb2e28be68e1f3e1184b8b) )
	ROM_LOAD( "zpu1",0x2000, 0x2000, CRC(fc12af94) SHA1(65f5bca2853271c232bd02dfc3467e6a4f7f0a6f) )
	ROM_LOAD( "zpu2",0x4000, 0x2000, CRC(b34cfa26) SHA1(903adc6de0d34e5bc8fb0f8d3e74ff53204d8c68) )
	ROM_LOAD( "zpu3",0x6000, 0x2000, CRC(c142ebdf) SHA1(0b87740d26b19a05f65b811225ee0053ddb27d22) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )     /* 64k for sound CPU (PSB board) */
	ROM_LOAD( "psba4",0x0000, 0x2000, CRC(172a793e) SHA1(3618a778af1f4a6267bf7e0786529be731ac9b76) )

	ROM_REGION( 0x38000, REGION_CPU3, 0 )     /* 64k for video CPU (CFB board) */
	ROM_LOAD( "cfb0",0x0000, 0x2000, CRC(ee372b1f) SHA1(b630fd659d59eb8c2540f18d91ae0d72e859fc4f) )
	ROM_LOAD( "cfb1",0x2000, 0x2000, CRC(b76d9527) SHA1(8f16b850bd67d553aaaf7e176754e36aba581445) )

	ROM_LOAD( "psb00",0x10000,0x2000, CRC(b4956100) SHA1(98baf5c27c76dc5c4eafc44f42705239504637fe) )/*banked at 0x4000*/
	ROM_LOAD( "psb01",0x12000,0x2000, CRC(acdce2ee) SHA1(96b8961afbd0006b10cfdc825aefe27ec18121ff) )
	ROM_LOAD( "psb02",0x14000,0x2000, CRC(cb840fc6) SHA1(c30c72d355e1957f3715e9fab701f65b9d7d632a) )
	ROM_LOAD( "psb03",0x16000,0x2000, CRC(86ea6f99) SHA1(ce5d42557d0a62eebe3d0cee28587d60707573e4) )
	ROM_LOAD( "psb04",0x18000,0x2000, CRC(65379893) SHA1(84bb755e23d5ce13b1c82e59f24f3890c50697cc) )
	ROM_LOAD( "psb05",0x1a000,0x2000, CRC(f82245cb) SHA1(fa1cab94a03ce7b8e45ea6eec572b21f268f7547) )
	ROM_LOAD( "psb06",0x1c000,0x2000, CRC(6b86794f) SHA1(72cf67ecf5a9198ecb44dd846de968e6cdd6458d) )
	ROM_LOAD( "psb07",0x1e000,0x2000, CRC(60a7abf3) SHA1(44b932d8af29ec706c29d6b71a8bac6318d92315) )
	ROM_LOAD( "psb08",0x20000,0x2000, CRC(854be14e) SHA1(ae9b1fe2443c87bb4334bc776f7bc7e5fa874f38) )
	ROM_LOAD( "psb09",0x22000,0x2000, CRC(b2e8afa3) SHA1(30a3d83bf1ec7885549b47f9569e9ae0d05b948d) )
	ROM_LOAD( "psb10",0x24000,0x2000, CRC(fbfb0aab) SHA1(2eb666a5eff31019b4ffdfc82e242ff47cd59527) )
	ROM_LOAD( "psb11",0x26000,0x2000, CRC(ddcd3cec) SHA1(7d0c3b4160b11ebe9b097664190d8ae605413baa) )
	ROM_LOAD( "psb12",0x28000,0x2000, CRC(c6617377) SHA1(29a6fc52e06c41f06ee333aad707c3a1952dff4d) )
	ROM_LOAD( "psb13",0x2a000,0x2000, CRC(aeab8555) SHA1(c398cac5210022e3c9e25a9f2ef1017b27c21e62) )
	ROM_LOAD( "psb14",0x2c000,0x2000, CRC(ef35e314) SHA1(2e20517ff89b153fd888cf4eb0404a802e16b1b7) )
	ROM_LOAD( "psb15",0x2e000,0x2000, CRC(1fafe83d) SHA1(d1d406275f50d87547aabe1295795099f341433d) )
	ROM_LOAD( "psb16",0x30000,0x2000, CRC(ec49864f) SHA1(7a3b295972b52682406f75c4fe12c29632452491) )
	ROM_LOAD( "psb17",0x32000,0x2000, CRC(d9778e85) SHA1(2998f0a08cdba8a75e687a54cb9a03edeb4b22cd) )
	ROM_LOAD( "psb18",0x34000,0x2000, CRC(ef61b6c0) SHA1(7e8a82beefb9fd8e219fc4d7d25a3a43ab8aadf7) )
	ROM_LOAD( "psb19",0x36000,0x2000, CRC(68752e0d) SHA1(58a4921e4f774af5e1ef7af67f06e9b43643ffab) )

ROM_END


GAMEX( 1983, mazerbla, 0, mazerbla,  mazerbla, 0, ROT0, "Stern", "Mazer Blazer", GAME_IMPERFECT_GRAPHICS |GAME_NO_SOUND | GAME_NOT_WORKING )
GAMEX( 1983, greatgun, 0, greatgun,  greatgun, 0, ROT0, "Stern", "Great Guns", GAME_IMPERFECT_GRAPHICS )
