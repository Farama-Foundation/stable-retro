/*	Space Harrier Hardware
**
**	2xMC68000 + Z80
**	YM2151 or YM2203 + Custom PCM
**
**	Enduro Racer
**	Space Harrier
*/

/*
03/11/04  Charles MacDonald
Various Hang-On fixes:
- Fixed sprite RAM size.
- Fixed tile RAM size.
- Fixed 2nd 68000 work RAM size, passes RAM test.
- Fixed visibility of 2nd 68000 ROM to 1st 68000, passes ROM test.
- Fixed access to road RAM and shared RAM by both CPUs.
- Cleaned up input management, now entering test mode does not crash
  MAME, there are no specific control hacks for the name entry screen,
  and the ROM patches are no longer needed.
  
To do:
- Missing color bars in CRT tests
- Proper Enduro Racer and Space Harrier inputs
*/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "cpu/z80/z80.h"
#include "cpu/i8039/i8039.h"
#include "system16.h"

/*
	Hang-On I/O hardware
*/

static int latched_analog_input;	/* Selected input to read with ADC */
static int ppi_reg[2][4];			/* PPI registers */

static WRITE16_HANDLER( hangon_io_w )
{
	if( ACCESSING_LSB )
	{                        
		switch( offset & 0x003020/2 )
		{
			case 0x0000: /* PPI @ 4B */
				switch( offset & 0x07)
				{
					case 0x00: /* Port A : Z80 sound command */
						ppi_reg[0][0] = data;
						soundlatch_w(0, data & 0xff);
						cpu_set_nmi_line(1, PULSE_LINE);
						return;

					case 0x01: /* Port B : Miscellaneous outputs */
						ppi_reg[0][1] = data;

						/* D7 : FLIPC (1= flip screen, 0= normal orientation) */
						/* D6 : SHADE0 (1= highlight, 0= shadow) */
						/* D4 : /KILL (1= screen on, 0= screen off) */
						sys16_refreshenable = data & 0x10;

						/* D3 : LAMP2 */
						set_led_status(1, data & 0x08);
						
						/* D2 : LAMP1 */
						set_led_status(0, data & 0x04);
						
						/* D1 : COIN2 */
						coin_counter_w(1, data & 0x02);
						
						/* D0 : COIN1 */
						coin_counter_w(0, data & 0x01);						
						return;

					case 0x02: /* Port C : Tilemap origin and audio mute */
						ppi_reg[0][2] = data;

						/* D2 : SCONT1 - Tilemap origin bit 1 */
						/* D1 : SCONT0 - Tilemap origin bit 0 */
						/* D0 : MUTE (1= audio on, 0= audio off) */					
						
  						/* Not used */
						return;

					case 0x03: /* PPI control register */
						ppi_reg[0][3] = data;
						return;
				}
				break;

		case 0x3000/2: /* PPI @ 4C */
			switch( offset & 0x07)
			{
				case 0x00: /* Port A : S.CPU control and ADC channel select */
					ppi_reg[1][0] = data;
				
#if 0 /* Not sure this is correct */

					/* To S.RES of second CPU */
					if(data & 0x20)
					cpu_set_reset_line(2, CLEAR_LINE);
					else
					cpu_set_reset_line(2, ASSERT_LINE);
					
					/* To S.INT of second CPU */
					if(data & 0x10)
					cpu_set_irq_line(2, 1, HOLD_LINE);
					else
					cpu_set_irq_line(2, 1, CLEAR_LINE);
#endif
					return;
				
				case 0x01: /* Port B : High-current outputs */
					ppi_reg[1][1] = data;		
					/* Not used */
					return;
				
				case 0x02: /* Port C : LED driver control (?) */
					ppi_reg[1][2] = data;				
					/* Not used */
					return;
					
				case 0x03: /* PPI control register */
					ppi_reg[1][3] = data;
					return;
			}
		break;

		case 0x3020/2: /* ADC0804 */
			switch(ppi_reg[1][0] & 0x0C)
			{
				case 0x00: /* "ANGLE" */
					latched_analog_input = readinputport(0);
					return;
				
				case 0x04: /* "ACCEL" */
					latched_analog_input = readinputport(1);
					return;
			
				case 0x08: /* "BRAKE" */
					latched_analog_input = readinputport(5);
					return;
				
				case 0x0C: /* Not used */
					latched_analog_input = 0;
					return;
			}
			break;
		}
	}
}

static READ16_HANDLER( hangon_io_r )
{
	switch( offset & 0x003020/2 )
	{
		case 0x0000: /* PPI @ 4B */
			switch( offset & 0x07)
			{
				case 0x00: /* Port A : Z80 sound command */
				/*
				Bidirectional port, but Z80 only ever reads data written
				by the main 68000.
				*/
				return 0xFF;
			
				case 0x01: /* Port B */
					return ppi_reg[0][1];
				
				case 0x02: /* Port C */
					return ppi_reg[0][2];
				
				case 0x03: /* PPI control register */
					return ppi_reg[0][3];
			}
			break;
			
			case 0x1000/2: /* Input ports and DIP switches */
				switch( offset & 0x0F )
				{
					case 0x00: /* Input port #0 */
						return readinputport(2);
			
					case 0x01: /* Input port #1 */	
						/* Not used */
						return 0xFF;
			
					case 0x04: /* DIP switch A */
						return readinputport(3);
			
					case 0x06: /* DIP switch B */
						return readinputport(4);
				}
				break;
			
			case 0x3000/2: /* PPI @ 4C */
				switch( offset & 0x07)
				{
					case 0x00: /* Port A */
						return ppi_reg[1][0];
					
					case 0x01: /* Port B */
						return ppi_reg[1][1];
					
					case 0x02: /* Port C : ADC status */
						/*
						D7 = 0 (left open)
						D6 = /INTR of ADC0804
						D5 = 0 (left open)
						D4 = 0 (left open)
						
						We leave /INTR low to indicate converted data is
						always ready to be read.
						*/
						return (ppi_reg[1][2] & 0x0F);
					
					case 0x03: /* PPI control register */
						return ppi_reg[1][3];
				}
				break;
				
			case 0x3020/2: /* ADC0804 data output */
				return latched_analog_input;
	}	
	
	return -1;
}

/***************************************************************************/

static void generate_gr_screen(
	int w, /* 512*/
	int bitmap_width, /* 1024*/
	int skip, /* 8*/
	int start_color,int end_color, /* 0..4*/
	int source_size )
{
	/* preprocess road data, expanding it into a form more easily rendered */
	UINT8 *buf = malloc( source_size );
	if( buf ){
		UINT8 *buf0 = buf; /* remember so we can free and not leak memory */
		UINT8 *gr = memory_region(REGION_GFX3); /* road gfx data */
		UINT8 *grr = NULL;
	    int row_offset,byte_offset,bit_offset;
	    int center_offset=0;
		sys16_gr_bitmap_width = bitmap_width;

/*log_cb(RETRO_LOG_DEBUG, LOGPRE  "generating road gfx; bitmap_width = %d\n", bitmap_width );*/

		memcpy( buf,gr,source_size ); /* copy from ROM to temp buffer */
		memset( gr,0,256*bitmap_width ); /* erase */

		if( w!=sys16_gr_bitmap_width ){
			if( skip>0 ) /* needs mirrored RHS*/
				grr=gr;
			else {
				center_offset= bitmap_width-w;
				gr+=center_offset/2;
			}
		}

		for( row_offset=0; row_offset<256; row_offset++ ){ /* build gr_bitmap*/
			UINT8 last_bit;
			UINT8 color_data[4];

			color_data[0]=start_color;
			color_data[1]=start_color+1;
			color_data[2]=start_color+2;
			color_data[3]=start_color+3;

			last_bit = ((buf[0]&0x80)==0)|(((buf[0x4000]&0x80)==0)<<1);
			for( byte_offset=0; byte_offset<w/8; byte_offset++ ){
				for( bit_offset=0; bit_offset<8; bit_offset++ ){
					UINT8 bit=((buf[0]&0x80)==0)|(((buf[0x4000]&0x80)==0)<<1);
					if( bit!=last_bit && bit==0 && row_offset>1 ){
						/* color flipped to 0? advance color[0]*/
						if (color_data[0]+end_color <= end_color){
							color_data[0]+=end_color;
						}
						else{
							color_data[0]-=end_color;
						}
					}
					*gr++ = color_data[bit];
/*					log_cb(RETRO_LOG_DEBUG, LOGPRE  "%01x", color_data[bit] );*/
					last_bit=bit;
					buf[0] <<= 1; buf[0x4000] <<= 1;
				}
				buf++;
			}
/*			log_cb(RETRO_LOG_DEBUG, LOGPRE  "\n" );*/

			if( grr!=NULL ){ /* need mirrored RHS*/
				const UINT8 *temp = gr-1-skip;
				for( byte_offset=0; byte_offset<w-skip; byte_offset++){
					*gr++ = *temp--;
				}
				for( byte_offset=0; byte_offset<skip; byte_offset++){
					*gr++ = 0;
				}
			}
			else {
				gr += center_offset;
			}
		}
		{
			int i=1;
			while ( (1<<i) < sys16_gr_bitmap_width ) i++;
			sys16_gr_bitmap_width=i; /* power of 2*/
		}
/*		log_cb(RETRO_LOG_DEBUG, LOGPRE  "width = %d\n", sys16_gr_bitmap_width );*/
		free( buf0 );
	}
}

#if 0
static void set_tile_bank( int data ){
	sys16_tile_bank1 = data&0xf;
	sys16_tile_bank0 = (data>>4)&0xf;
}

static void set_tile_bank18( int data ){
	sys16_tile_bank0 = data&0xf;
	sys16_tile_bank1 = (data>>4)&0xf;
}
#endif

static void set_page( int page[4], data16_t data ){
	page[1] = data>>12;
	page[0] = (data>>8)&0xf;
	page[3] = (data>>4)&0xf;
	page[2] = data&0xf;
}

static INTERRUPT_GEN( sys16_interrupt ){
	if(sys16_custom_irq) sys16_custom_irq();
	cpu_set_irq_line(cpu_getactivecpu(), 4, HOLD_LINE); /* Interrupt vector 4, used by VBlank */
}

static WRITE16_HANDLER( sound_command_nmi_w ){
	if( ACCESSING_LSB ){
		soundlatch_w( 0,data&0xff );
		cpu_set_nmi_line(1, PULSE_LINE);
	}
}

static data16_t coinctrl;

static WRITE16_HANDLER( sys16_3d_coinctrl_w )
{
	if( ACCESSING_LSB ){
		coinctrl = data&0xff;
		sys16_refreshenable = coinctrl & 0x10;
		coin_counter_w(0,coinctrl & 0x01);
		/* bit 6 is also used (0 in fantzone) */

		/* Hang-On, Super Hang-On, Space Harrier, Enduro Racer */
		set_led_status(0,coinctrl & 0x04);

		/* Space Harrier */
		set_led_status(1,coinctrl & 0x08);
	}
}

static READ16_HANDLER( sys16_coinctrl_r ){
	return coinctrl;
}

#if 0
static WRITE16_HANDLER( sys16_coinctrl_w )
{
	if( ACCESSING_LSB ){
		coinctrl = data&0xff;
		sys16_refreshenable = coinctrl & 0x20;
		coin_counter_w(0,coinctrl & 0x01);
		set_led_status(0,coinctrl & 0x04);
		set_led_status(1,coinctrl & 0x08);
		/* bit 6 is also used (1 most of the time; 0 in dduxbl, sdi, wb3;
		   tturf has it normally 1 but 0 after coin insertion) */
		/* eswat sets bit 4 */
	}
}
#endif

/*
	Hang-On shared road RAM and 68000 #2 work RAM
*/

data16_t *hangon_roadram;
data16_t *hangon_sharedram;

static READ16_HANDLER( hangon_sharedram_r ) {
	return hangon_sharedram[offset];
}

static WRITE16_HANDLER( hangon_sharedram_w ) {
	COMBINE_DATA( hangon_sharedram + offset );
}

static READ16_HANDLER( hangon_roadram_r ) {
	return hangon_roadram[offset];
}

static WRITE16_HANDLER( hangon_roadram_w ) {
	COMBINE_DATA( hangon_roadram + offset );
}


static READ16_HANDLER( hangon1_skip_r ){
	if (activecpu_get_pc()==0x17e6) {cpu_spinuntil_int(); return 0xffff;}
	return sys16_extraram[0x0400/2];
}

static MEMORY_READ16_START( hangon_readmem )
    { 0x000000, 0x03ffff, MRA16_ROM },
	{ 0x20c400, 0x20c401, hangon1_skip_r },
	{ 0x20c000, 0x20ffff, SYS16_MRA16_EXTRAM },
	{ 0x400000, 0x403fff, SYS16_MRA16_TILERAM },
	{ 0x410000, 0x410fff, SYS16_MRA16_TEXTRAM },
	{ 0x600000, 0x6007ff, SYS16_MRA16_SPRITERAM },
	{ 0xa00000, 0xa00fff, SYS16_MRA16_PALETTERAM },
	{ 0xc00000, 0xc0ffff, SYS16_CPU3ROM16_r },
	{ 0xc68000, 0xc68fff, hangon_roadram_r },
	{ 0xc7c000, 0xc7ffff, hangon_sharedram_r },
	{ 0xe00000, 0xffffff, hangon_io_r },
MEMORY_END

static MEMORY_WRITE16_START( hangon_writemem )
    { 0x000000, 0x03ffff, MWA16_ROM },
	{ 0x20c000, 0x20ffff, SYS16_MWA16_EXTRAM, &sys16_extraram },
	{ 0x400000, 0x403fff, SYS16_MWA16_TILERAM, &sys16_tileram },
	{ 0x410000, 0x410fff, SYS16_MWA16_TEXTRAM, &sys16_textram },
	{ 0x600000, 0x6007ff, SYS16_MWA16_SPRITERAM, &sys16_spriteram },
	{ 0xa00000, 0xa00fff, SYS16_MWA16_PALETTERAM, &paletteram16 },
	{ 0xc00000, 0xc3ffff, MWA16_NOP },
	{ 0xc68000, 0xc68fff, hangon_roadram_w, &hangon_roadram },
	{ 0xc7c000, 0xc7ffff, hangon_sharedram_w, &hangon_sharedram },
	{ 0xe00000, 0xffffff, hangon_io_w },
MEMORY_END

static READ16_HANDLER( hangon2_skip_r ){
	if (activecpu_get_pc()==0xf66) {cpu_spinuntil_int(); return 0xffff;}
	return hangon_sharedram[0x01000/2];
}

static MEMORY_READ16_START( hangon_readmem2 )
    { 0x000000, 0x03ffff, MRA16_ROM },
	{ 0xc7f000, 0xc7f001, hangon2_skip_r },
	{ 0xc68000, 0xc68fff, hangon_roadram_r },
	{ 0xc7c000, 0xc7ffff, hangon_sharedram_r },
MEMORY_END

static MEMORY_WRITE16_START( hangon_writemem2 )
    { 0x000000, 0x03ffff, MWA16_ROM },
	{ 0xc68000, 0xc68fff, hangon_roadram_w },
	{ 0xc7c000, 0xc7ffff, hangon_sharedram_w },
MEMORY_END

static MEMORY_READ_START( hangon_sound_readmem )
    { 0x0000, 0x7fff, MRA_ROM },
	{ 0xc000, 0xc7ff, MRA_RAM },
	{ 0xd000, 0xd000, YM2203_status_port_0_r },
	{ 0xe000, 0xe7ff, SegaPCM_r },
	{ 0xf800, 0xffff, MRA_RAM },
MEMORY_END

static MEMORY_WRITE_START( hangon_sound_writemem )
    { 0x0000, 0x7fff, MWA_ROM },
	{ 0xc000, 0xc7ff, MWA_RAM },
	{ 0xd000, 0xd000, YM2203_control_port_0_w },
	{ 0xd001, 0xd001, YM2203_write_port_0_w },
	{ 0xe000, 0xe7ff, SegaPCM_w },
	{ 0xf800, 0xffff, MWA_RAM },
MEMORY_END

static PORT_READ_START( hangon_sound_readport )
    { 0x40, 0x40, soundlatch_r },
PORT_END

static PORT_WRITE_START( hangon_sound_writeport )
PORT_END

/***************************************************************************/

static void hangon_update_proc( void ){
	set_page( sys16_bg_page, sys16_textram[0x74e] & 0x3333 );
	set_page( sys16_fg_page, sys16_textram[0x74f] & 0x3333 );
	sys16_fg_scrollx = sys16_textram[0x7fc] & 0x01ff;
	sys16_bg_scrollx = sys16_textram[0x7fd] & 0x01ff;
	sys16_fg_scrolly = sys16_textram[0x792] & 0x01ff;
	sys16_bg_scrolly = sys16_textram[0x793] & 0x01ff;
}

static MACHINE_INIT( hangon ){
	sys16_textmode=1;
	sys16_spritesystem = sys16_sprite_hangon;
	sys16_sprxoffset = -0xc0;
	sys16_fgxoffset = 8;
	sys16_textlayer_lo_min=0;
	sys16_textlayer_lo_max=0;
	sys16_textlayer_hi_min=0;
	sys16_textlayer_hi_max=0xff;

/*  
	The following patches modified the input code to read the first three
	analog inputs from unique addresses rather than the single address
	the ADC is mapped to, so the input selection behavior didn't have to be
	emulated. Not needed anymore, but left in for reference.
	
	sys16_patch_code( 0x83bd, 0x29); // $E03021 -> $E03029
	sys16_patch_code( 0x8495, 0x2a); // $E03021 -> $E0302A
	sys16_patch_code( 0x84f9, 0x2b); // $E03021 -> $E0302B
*/

	sys16_update_proc = hangon_update_proc;

	sys16_gr_ver = &hangon_roadram[0x0];
	sys16_gr_hor = sys16_gr_ver+0x200/2;
	sys16_gr_pal = sys16_gr_ver+0x400/2;
	sys16_gr_flip= sys16_gr_ver+0x600/2;

	sys16_gr_palette= 0xf80 / 2;
	sys16_gr_palette_default = 0x70 /2;
	sys16_gr_colorflip[0][0]=0x08 / 2;
	sys16_gr_colorflip[0][1]=0x04 / 2;
	sys16_gr_colorflip[0][2]=0x00 / 2;
	sys16_gr_colorflip[0][3]=0x06 / 2;
	sys16_gr_colorflip[1][0]=0x0a / 2;
	sys16_gr_colorflip[1][1]=0x04 / 2;
	sys16_gr_colorflip[1][2]=0x02 / 2;
	sys16_gr_colorflip[1][3]=0x02 / 2;
}

static DRIVER_INIT( hangon ){
	generate_gr_screen(512,1024,8,0,4,0x8000);
}

/***************************************************************************/

static MACHINE_DRIVER_START( hangon )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 10000000)
	MDRV_CPU_MEMORY(hangon_readmem,hangon_writemem)
	MDRV_CPU_VBLANK_INT(sys16_interrupt,1)
	
	MDRV_CPU_ADD(Z80, 4096000)
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
	MDRV_CPU_MEMORY(hangon_sound_readmem,hangon_sound_writemem)
	MDRV_CPU_PORTS(hangon_sound_readport,hangon_sound_writeport)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,4)
	
	MDRV_CPU_ADD(M68000, 10000000)
	MDRV_CPU_MEMORY(hangon_readmem2,hangon_writemem2)
	MDRV_CPU_VBLANK_INT(sys16_interrupt,1)
	
	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(100)

	MDRV_MACHINE_INIT(hangon)
	
	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(40*8, 28*8)
	MDRV_VISIBLE_AREA(0*8, 40*8-1, 0*8, 28*8-1)
	MDRV_GFXDECODE(sys16_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(2048*ShadowColorsMultiplier)

	/* initilize system16 variables prior to driver_init and video_start */
	machine_init_sys16_onetime();

	MDRV_VIDEO_START(hangon)
	MDRV_VIDEO_UPDATE(hangon)
	
	/* sound hardware */
	MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
	MDRV_SOUND_ADD(YM2203, sys16_ym2203_interface)
	MDRV_SOUND_ADD(SEGAPCM, sys16_segapcm_interface_32k)
MACHINE_DRIVER_END




static READ16_HANDLER( sh_io_joy_r ){ return (input_port_5_r( offset ) << 8) + input_port_6_r( offset ); }

static data16_t *shared_ram;
static READ16_HANDLER( shared_ram_r ){
	return shared_ram[offset];
}
static WRITE16_HANDLER( shared_ram_w ){
	COMBINE_DATA( &shared_ram[offset] );
}

static READ16_HANDLER( sh_motor_status_r ) { return 0x0; }


static MEMORY_READ16_START( harrier_readmem )
    { 0x000000, 0x03ffff, MRA16_ROM },
	{ 0x040000, 0x043fff, SYS16_MRA16_EXTRAM },
	{ 0x100000, 0x107fff, SYS16_MRA16_TILERAM },
	{ 0x108000, 0x108fff, SYS16_MRA16_TEXTRAM },
	{ 0x110000, 0x110fff, SYS16_MRA16_PALETTERAM },
	{ 0x124000, 0x127fff, shared_ram_r },
	{ 0x130000, 0x130fff, SYS16_MRA16_SPRITERAM },
	{ 0x140002, 0x140003, sys16_coinctrl_r },
	{ 0x140010, 0x140011, input_port_2_word_r }, /* service*/
	{ 0x140014, 0x140015, input_port_3_word_r }, /* dip1*/
	{ 0x140016, 0x140017, input_port_4_word_r }, /* dip2*/
	{ 0x140024, 0x140027, sh_motor_status_r },
	{ 0xc68000, 0xc68fff, SYS16_MRA16_EXTRAM2 },
MEMORY_END

static MEMORY_WRITE16_START( harrier_writemem )
    { 0x000000, 0x03ffff, MWA16_ROM },
	{ 0x040000, 0x043fff, SYS16_MWA16_EXTRAM, &sys16_extraram },
	{ 0x100000, 0x107fff, SYS16_MWA16_TILERAM, &sys16_tileram },
	{ 0x108000, 0x108fff, SYS16_MWA16_TEXTRAM, &sys16_textram },
	{ 0x110000, 0x110fff, SYS16_MWA16_PALETTERAM, &paletteram16 },
	{ 0x124000, 0x127fff, shared_ram_w, &shared_ram },
	{ 0x130000, 0x130fff, SYS16_MWA16_SPRITERAM, &sys16_spriteram },
	{ 0x140000, 0x140001, sound_command_nmi_w },
	{ 0x140002, 0x140003, sys16_3d_coinctrl_w },
	{ 0xc68000, 0xc68fff, SYS16_MWA16_EXTRAM2, &sys16_extraram2 },
MEMORY_END

static MEMORY_READ16_START( harrier_readmem2 )
    { 0x000000, 0x03ffff, MRA16_ROM },
	{ 0xc68000, 0xc68fff, SYS16_MRA16_EXTRAM2 },
	{ 0xc7c000, 0xc7ffff, shared_ram_r },
MEMORY_END

static MEMORY_WRITE16_START( harrier_writemem2 )
    { 0x000000, 0x03ffff, MWA16_ROM },
	{ 0xc68000, 0xc68fff, SYS16_MWA16_EXTRAM2, &sys16_extraram2 },
	{ 0xc7c000, 0xc7ffff, shared_ram_w, &shared_ram },
MEMORY_END

static MEMORY_READ_START( harrier_sound_readmem )
    { 0x0000, 0x7fff, MRA_ROM },
	{ 0xd000, 0xd000, YM2203_status_port_0_r },
	{ 0xe000, 0xe0ff, SegaPCM_r },
	{ 0x8000, 0xffff, MRA_RAM },
MEMORY_END

static MEMORY_WRITE_START( harrier_sound_writemem )
    { 0x0000, 0x7fff, MWA_ROM },
	{ 0xd000, 0xd000, YM2203_control_port_0_w },
	{ 0xd001, 0xd001, YM2203_write_port_0_w },
	{ 0xe000, 0xe0ff, SegaPCM_w },
	{ 0x8000, 0xffff, MWA_RAM },
MEMORY_END

static PORT_READ_START( harrier_sound_readport )
    { 0x40, 0x40, soundlatch_r },
PORT_END

static PORT_WRITE_START( harrier_sound_writeport )
PORT_END

/***************************************************************************/

static void harrier_update_proc( void ){
	int data;
	sys16_fg_scrollx = sys16_textram[0x7fc] & 0x01ff;
	sys16_bg_scrollx = sys16_textram[0x7fd] & 0x01ff;
	sys16_fg_scrolly = sys16_textram[0x792] & 0x01ff;
	sys16_bg_scrolly = sys16_textram[0x793] & 0x01ff;

	data = sys16_textram[0x74f];
	sys16_fg_page[0] = data>>12;
	sys16_fg_page[1] = (data>>8)&0xf;
	sys16_fg_page[3] = (data>>4)&0xf;
	sys16_fg_page[2] = data&0xf;

	data = sys16_textram[0x74e];
	sys16_bg_page[0] = data>>12;
	sys16_bg_page[1] = (data>>8)&0xf;
	sys16_bg_page[3] = (data>>4)&0xf;
	sys16_bg_page[2] = data&0xf;

	sys16_extraram[0x492/2] = sh_io_joy_r(0,0);
}

static MACHINE_INIT( harrier ){
	sys16_textmode=1;
	sys16_spritesystem = sys16_sprite_sharrier;
	sys16_sprxoffset = -0xc0;
	sys16_fgxoffset = 8;
	sys16_textlayer_lo_min=0;
	sys16_textlayer_lo_max=0;
	sys16_textlayer_hi_min=0;
	sys16_textlayer_hi_max=0xff;

/**disable illegal rom writes*/
	sys16_patch_code( 0x8112, 0x4a);
	sys16_patch_code( 0x83d2, 0x4a);
	sys16_patch_code( 0x83d6, 0x4a);
	sys16_patch_code( 0x82c4, 0x4a);
	sys16_patch_code( 0x82c8, 0x4a);
	sys16_patch_code( 0x84d0, 0x4a);
	sys16_patch_code( 0x84d4, 0x4a);
	sys16_patch_code( 0x85de, 0x4a);
	sys16_patch_code( 0x85e2, 0x4a);

	sys16_update_proc = harrier_update_proc;

	sys16_gr_ver = sys16_extraram2;
	sys16_gr_hor = sys16_gr_ver+0x200/2;
	sys16_gr_pal = sys16_gr_ver+0x400/2;
	sys16_gr_flip= sys16_gr_ver+0x600/2;

	sys16_gr_palette= 0xf80 / 2;
	sys16_gr_palette_default = 0x70 /2;
	sys16_gr_colorflip[0][0]=0x00 / 2;
	sys16_gr_colorflip[0][1]=0x02 / 2;
	sys16_gr_colorflip[0][2]=0x04 / 2;
	sys16_gr_colorflip[0][3]=0x00 / 2;
	sys16_gr_colorflip[1][0]=0x00 / 2;
	sys16_gr_colorflip[1][1]=0x00 / 2;
	sys16_gr_colorflip[1][2]=0x06 / 2;
	sys16_gr_colorflip[1][3]=0x00 / 2;

	sys16_sh_shadowpal=0;
}

static DRIVER_INIT( sharrier )
{
	sys16_MaxShadowColors=NumOfShadowColors / 2;
	sys16_interleave_sprite_data( 0x100000 );
	generate_gr_screen(512,512,0,0,4,0x8000);
}
/***************************************************************************/

static MACHINE_DRIVER_START( sharrier )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 10000000)
	MDRV_CPU_MEMORY(harrier_readmem,harrier_writemem)
	MDRV_CPU_VBLANK_INT(sys16_interrupt,1)
	
	MDRV_CPU_ADD(Z80, 4096000)
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
	MDRV_CPU_MEMORY(harrier_sound_readmem,harrier_sound_writemem)
	MDRV_CPU_PORTS(harrier_sound_readport,harrier_sound_writeport)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,4)
	
	MDRV_CPU_ADD(M68000, 10000000)
	MDRV_CPU_MEMORY(harrier_readmem2,harrier_writemem2)
	MDRV_CPU_VBLANK_INT(sys16_interrupt,1)
	
	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(100)

	MDRV_MACHINE_INIT(harrier)
	
	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(40*8, 28*8)
	MDRV_VISIBLE_AREA(0*8, 40*8-1, 0*8, 28*8-1)
	MDRV_GFXDECODE(sys16_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(2048*ShadowColorsMultiplier)

	/* initilize system16 variables prior to driver_init and video_start */
	machine_init_sys16_onetime();

	MDRV_VIDEO_START(hangon)
	MDRV_VIDEO_UPDATE(hangon)
	
	/* sound hardware */
	MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
	MDRV_SOUND_ADD(YM2203, sys16_ym2203_interface)
	MDRV_SOUND_ADD(SEGAPCM, sys16_segapcm_interface_32k)
MACHINE_DRIVER_END



/***************************************************************************/

data16_t er_io_analog_sel;

static READ16_HANDLER( er_io_analog_r )
{
	switch( er_io_analog_sel )
	{
		case 0:		/* accel*/
			if(input_port_1_r( offset ) & 1)
				return 0xff;
			else
				return 0;
		case 4:		/* brake*/
			if(input_port_1_r( offset ) & 2)
				return 0xff;
			else
				return 0;
		case 8:		/* bank up down?*/
			if(input_port_1_r( offset ) & 4)
				return 0xff;
			else
				return 0;
		case 12:	/* handle*/
			return input_port_0_r( offset );

	}
	return 0;
}

static WRITE16_HANDLER( er_io_analog_w )
{
	COMBINE_DATA( &er_io_analog_sel );
}

static READ16_HANDLER( er_reset2_r )
{
	cpu_set_reset_line(2,PULSE_LINE);
	return 0;
}

static MEMORY_READ16_START( enduror_readmem )
    { 0x000000, 0x03ffff, MRA16_ROM },
	{ 0x040000, 0x043fff, SYS16_MRA16_EXTRAM },
	{ 0x100000, 0x107fff, SYS16_MRA16_TILERAM },
	{ 0x108000, 0x108fff, SYS16_MRA16_TEXTRAM },
	{ 0x110000, 0x110fff, SYS16_MRA16_PALETTERAM },
	{ 0x124000, 0x127fff, shared_ram_r },
	{ 0x130000, 0x130fff, SYS16_MRA16_SPRITERAM },
	{ 0x140002, 0x140003, sys16_coinctrl_r },
	{ 0x140010, 0x140011, input_port_2_word_r }, /* service*/
	{ 0x140014, 0x140015, input_port_3_word_r }, /* dip1*/
	{ 0x140016, 0x140017, input_port_4_word_r }, /* dip2*/
	{ 0x140030, 0x140031, er_io_analog_r },
	{ 0xe00000, 0xe00001, er_reset2_r },
MEMORY_END

static MEMORY_WRITE16_START( enduror_writemem )
    { 0x000000, 0x03ffff, MWA16_ROM },
	{ 0x040000, 0x043fff, SYS16_MWA16_EXTRAM, &sys16_extraram },
	{ 0x100000, 0x107fff, SYS16_MWA16_TILERAM, &sys16_tileram },
	{ 0x108000, 0x108fff, SYS16_MWA16_TEXTRAM, &sys16_textram },
	{ 0x110000, 0x110fff, SYS16_MWA16_PALETTERAM, &paletteram16 },
	{ 0x124000, 0x127fff, shared_ram_w, &shared_ram },
	{ 0x130000, 0x130fff, SYS16_MWA16_SPRITERAM, &sys16_spriteram },
	{ 0x140000, 0x140001, sound_command_nmi_w },
	{ 0x140002, 0x140003, sys16_3d_coinctrl_w },
	{ 0x140030, 0x140031, er_io_analog_w },
MEMORY_END

static READ16_HANDLER( enduro_p2_skip_r ){
	if (activecpu_get_pc()==0x4ba) {cpu_spinuntil_int(); return 0xffff;}
	return shared_ram[0x2000/2];
}

static MEMORY_READ16_START( enduror_readmem2 )
    { 0x000000, 0x03ffff, MRA16_ROM },
	{ 0xc68000, 0xc68fff, SYS16_MRA16_EXTRAM2 },
	{ 0xc7e000, 0xc7e001, enduro_p2_skip_r },
	{ 0xc7c000, 0xc7ffff, shared_ram_r },
MEMORY_END

static MEMORY_WRITE16_START( enduror_writemem2 )
    { 0x000000, 0x03ffff, MWA16_ROM },
	{ 0xc68000, 0xc68fff, SYS16_MWA16_EXTRAM2, &sys16_extraram2 },
	{ 0xc7c000, 0xc7ffff, shared_ram_w, &shared_ram },
MEMORY_END

static MEMORY_READ_START( enduror_sound_readmem )
    { 0x0000, 0x7fff, MRA_ROM },
	{ 0xc000, 0xc7ff, MRA_RAM },
	{ 0xd000, 0xd000, YM2203_status_port_0_r },
	{ 0xe000, 0xe7ff, SegaPCM_r },
MEMORY_END

static MEMORY_WRITE_START( enduror_sound_writemem )
    { 0x0000, 0x7fff, MWA_ROM },
	{ 0xc000, 0xc7ff, MWA_RAM },
	{ 0xd000, 0xd000, YM2203_control_port_0_w },
	{ 0xd001, 0xd001, YM2203_write_port_0_w },
	{ 0xe000, 0xe7ff, SegaPCM_w },
MEMORY_END


static PORT_READ_START( enduror_sound_readport )
    { 0x40, 0x40, soundlatch_r },
PORT_END

static PORT_WRITE_START( enduror_sound_writeport )
PORT_END


static MEMORY_READ_START( enduror_b2_sound_readmem )
    { 0x0000, 0x7fff, MRA_ROM },
/*	{ 0xc000, 0xc7ff, MRA_RAM },*/
	{ 0xf000, 0xf7ff, SegaPCM_r },
	{ 0xf800, 0xffff, MRA_RAM },
MEMORY_END

static MEMORY_WRITE_START( enduror_b2_sound_writemem )
    { 0x0000, 0x7fff, MWA_ROM },
/*	{ 0xc000, 0xc7ff, MWA_RAM },*/
	{ 0xf000, 0xf7ff, SegaPCM_w },
	{ 0xf800, 0xffff, MWA_RAM },
MEMORY_END

static PORT_READ_START( enduror_b2_sound_readport )
    { 0x00, 0x00, YM2203_status_port_0_r },
	{ 0x80, 0x80, YM2203_status_port_1_r },
	{ 0xc0, 0xc0, YM2203_status_port_2_r },
	{ 0x40, 0x40, soundlatch_r },
PORT_END

static PORT_WRITE_START( enduror_b2_sound_writeport )
    { 0x00, 0x00, YM2203_control_port_0_w },
	{ 0x01, 0x01, YM2203_write_port_0_w },
	{ 0x80, 0x80, YM2203_control_port_1_w },
	{ 0x81, 0x81, YM2203_write_port_1_w },
	{ 0xc0, 0xc0, YM2203_control_port_2_w },
	{ 0xc1, 0xc1, YM2203_write_port_2_w },
PORT_END

/***************************************************************************/

static void enduror_update_proc( void ){
	int data;
	sys16_fg_scrollx = sys16_textram[0x7fc] & 0x01ff;
	sys16_bg_scrollx = sys16_textram[0x7fd] & 0x01ff;
	sys16_fg_scrolly = sys16_textram[0x792] & 0x01ff;
	sys16_bg_scrolly = sys16_textram[0x793] & 0x01ff;

	data = sys16_textram[0x74f];
	sys16_fg_page[0] = data>>12;
	sys16_fg_page[1] = (data>>8)&0xf;
	sys16_fg_page[3] = (data>>4)&0xf;
	sys16_fg_page[2] = data&0xf;

	data = sys16_textram[0x74e];
	sys16_bg_page[0] = data>>12;
	sys16_bg_page[1] = (data>>8)&0xf;
	sys16_bg_page[3] = (data>>4)&0xf;
	sys16_bg_page[2] = data&0xf;
}

static MACHINE_INIT( enduror ){
	sys16_textmode=1;
	sys16_spritesystem = sys16_sprite_sharrier;
	sys16_sprxoffset = -0xc0;
	sys16_fgxoffset = 13;
/*	sys16_sprxoffset = -0xbb;*/
/*	sys16_fgxoffset = 8;*/
	sys16_textlayer_lo_min=0;
	sys16_textlayer_lo_max=0;
	sys16_textlayer_hi_min=0;
	sys16_textlayer_hi_max=0xff;

	sys16_update_proc = enduror_update_proc;

	sys16_gr_ver = &sys16_extraram2[0x0];
	sys16_gr_hor = sys16_gr_ver+0x200/2;
	sys16_gr_pal = sys16_gr_ver+0x400/2;
	sys16_gr_flip= sys16_gr_ver+0x600/2;

	sys16_gr_palette= 0xf80 / 2;
	sys16_gr_palette_default = 0x70 /2;
	sys16_gr_colorflip[0][0]=0x00 / 2;
	sys16_gr_colorflip[0][1]=0x02 / 2;
	sys16_gr_colorflip[0][2]=0x04 / 2;
	sys16_gr_colorflip[0][3]=0x00 / 2;
	sys16_gr_colorflip[1][0]=0x00 / 2;
	sys16_gr_colorflip[1][1]=0x00 / 2;
	sys16_gr_colorflip[1][2]=0x06 / 2;
	sys16_gr_colorflip[1][3]=0x00 / 2;

	sys16_sh_shadowpal=0xff;
}

static void enduror_sprite_decode( void ){
	data16_t *rom = (data16_t *)memory_region(REGION_CPU1);
	sys16_interleave_sprite_data( 8*0x20000 );
	generate_gr_screen(512,1024,8,0,4,0x8000);

/*	enduror_decode_data (rom,rom,0x10000);	*/ /* no decrypt info.*/
	enduror_decode_data (rom+0x10000/2,rom+0x10000/2,0x10000);
	enduror_decode_data2(rom+0x20000/2,rom+0x20000/2,0x10000);
}

static void endurob_sprite_decode( void ){
	sys16_interleave_sprite_data( 8*0x20000 );
	generate_gr_screen(512,1024,8,0,4,0x8000);
}

static void endurora_opcode_decode( void )
{
	data16_t *rom = (data16_t *)memory_region(REGION_CPU1);
	int diff = 0x50000;	/* place decrypted opcodes in a hole after RAM */


	memory_set_opcode_base(0,rom+diff/2);

	memcpy(rom+(diff+0x10000)/2,rom+0x10000/2,0x20000);
	memcpy(rom+diff/2,rom+0x30000/2,0x10000);

	/* patch code to force a reset on cpu2 when starting a new game.*/
	/* Undoubtly wrong, but something like it is needed for the game to work*/
	rom[(0x1866 + diff)/2] = 0x4a79;
	rom[(0x1868 + diff)/2] = 0x00e0;
	rom[(0x186a + diff)/2] = 0x0000;
}

static void endurob2_opcode_decode( void )
{
	data16_t *rom = (data16_t *)memory_region(REGION_CPU1);
	int diff = 0x50000;	/* place decrypted opcodes in a hole after RAM */


	memory_set_opcode_base(0,rom+diff/2);

	memcpy(rom+diff/2,rom,0x30000);

	endurob2_decode_data (rom,rom+diff/2,0x10000);
	endurob2_decode_data2(rom+0x10000/2,rom+(diff+0x10000)/2,0x10000);

	/* patch code to force a reset on cpu2 when starting a new game.*/
	/* Undoubtly wrong, but something like it is needed for the game to work*/
	rom[(0x1866 + diff)/2] = 0x4a79;
	rom[(0x1868 + diff)/2] = 0x00e0;
	rom[(0x186a + diff)/2] = 0x0000;
}

static DRIVER_INIT( enduror )
{
	sys16_MaxShadowColors=NumOfShadowColors / 2;
/*	sys16_MaxShadowColors=0;*/

	enduror_sprite_decode();
}

static DRIVER_INIT( endurobl )
{
	sys16_MaxShadowColors=NumOfShadowColors / 2;
/*	sys16_MaxShadowColors=0;*/

	endurob_sprite_decode();
	endurora_opcode_decode();
}

static DRIVER_INIT( endurob2 )
{
	sys16_MaxShadowColors=NumOfShadowColors / 2;
/*	sys16_MaxShadowColors=0;*/

	endurob_sprite_decode();
	endurob2_opcode_decode();
}

/***************************************************************************/

static MACHINE_DRIVER_START( enduror )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 10000000)
	MDRV_CPU_MEMORY(enduror_readmem,enduror_writemem)
	MDRV_CPU_VBLANK_INT(sys16_interrupt,1)
	
	MDRV_CPU_ADD(Z80, 4096000)
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
	MDRV_CPU_MEMORY(enduror_sound_readmem,enduror_sound_writemem)
	MDRV_CPU_PORTS(enduror_sound_readport,enduror_sound_writeport)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,4)
	
	MDRV_CPU_ADD(M68000, 10000000)
	MDRV_CPU_MEMORY(enduror_readmem2,enduror_writemem2)
	MDRV_CPU_VBLANK_INT(sys16_interrupt,1)
	
	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(100)

	MDRV_MACHINE_INIT(enduror)
	
	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(40*8, 28*8)
	MDRV_VISIBLE_AREA(0*8, 40*8-1, 0*8, 28*8-1)
	MDRV_GFXDECODE(sys16_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(2048*ShadowColorsMultiplier)

	/* initilize system16 variables prior to driver_init and video_start */
	machine_init_sys16_onetime();

	MDRV_VIDEO_START(hangon)
	MDRV_VIDEO_UPDATE(hangon)
	
	/* sound hardware */
	MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
	MDRV_SOUND_ADD(YM2203, sys16_ym2203_interface)
	MDRV_SOUND_ADD(SEGAPCM, sys16_segapcm_interface_32k)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( endurob2 )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 10000000)
	MDRV_CPU_MEMORY(enduror_readmem,enduror_writemem)
	MDRV_CPU_VBLANK_INT(sys16_interrupt,1)
	
	MDRV_CPU_ADD(Z80, 4096000)
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
	MDRV_CPU_MEMORY(enduror_b2_sound_readmem,enduror_b2_sound_writemem)
	MDRV_CPU_PORTS(enduror_b2_sound_readport,enduror_b2_sound_writeport)
	
	MDRV_CPU_ADD(M68000, 10000000)
	MDRV_CPU_MEMORY(enduror_readmem2,enduror_writemem2)
	MDRV_CPU_VBLANK_INT(sys16_interrupt,1)
	
	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(100)

	MDRV_MACHINE_INIT(enduror)
	
	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(40*8, 28*8)
	MDRV_VISIBLE_AREA(0*8, 40*8-1, 0*8, 28*8-1)
	MDRV_GFXDECODE(sys16_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(2048*ShadowColorsMultiplier)

	/* initilize system16 variables prior to driver_init and video_start */
	machine_init_sys16_onetime();

	MDRV_VIDEO_START(hangon)
	MDRV_VIDEO_UPDATE(hangon)
	
	/* sound hardware */
	MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
	MDRV_SOUND_ADD(YM2203, sys16_3xym2203_interface)
	MDRV_SOUND_ADD(SEGAPCM, sys16_segapcm_interface_15k)
MACHINE_DRIVER_END

/*****************************************************************************/

ROM_START( hangon )
	ROM_REGION( 0x20000, REGION_CPU1, 0 ) /* 68000 code */
	ROM_LOAD16_BYTE( "6918.rom", 0x000000, 0x8000, CRC(20b1c2b0) SHA1(01b4f5105e2bbeb6ec6dbd18bfb728e3a973e0ca) )
	ROM_LOAD16_BYTE( "6916.rom", 0x000001, 0x8000, CRC(7d9db1bf) SHA1(952ee3e7a0d57ec1bb3385e0e6675890b8378d31) )
	ROM_LOAD16_BYTE( "6917.rom", 0x010000, 0x8000, CRC(fea12367) SHA1(9a1ce5863c562160b657ad948812b43f42d7d0cc) )
	ROM_LOAD16_BYTE( "6915.rom", 0x010001, 0x8000, CRC(ac883240) SHA1(f943341ae13e062f3d12c6221180086ce8bdb8c4) )

	ROM_REGION( 0x18000, REGION_GFX1, ROMREGION_DISPOSE ) /* tiles */
	ROM_LOAD( "6841.rom", 0x00000, 0x08000, CRC(54d295dc) SHA1(ad8cdb281032a2f931c2abbeb966998944683dc3) )
	ROM_LOAD( "6842.rom", 0x08000, 0x08000, CRC(f677b568) SHA1(636ca60bd4be9b5c2be09de8ae49db1063aa6c79) )
	ROM_LOAD( "6843.rom", 0x10000, 0x08000, CRC(a257f0da) SHA1(9828f8ce4ef245ffb8dbad347f9ca74ed81aa998) )

	ROM_REGION( 0x80000, REGION_GFX2, 0 ) /* sprites */
	ROM_LOAD16_BYTE( "6819.rom", 0x000001, 0x8000, CRC(469dad07) SHA1(6d01c0b3506e28832928ad74d518577ff5be323b) )
	ROM_LOAD16_BYTE( "6820.rom", 0x000000, 0x8000, CRC(87cbc6de) SHA1(b64652e062e1b88c6f6ae8dd2ffe4533bb27ba45) )
	ROM_LOAD16_BYTE( "6821.rom", 0x010001, 0x8000, CRC(15792969) SHA1(b061dbf24e8b511116446794753c8b0cc49e2149) )
	ROM_LOAD16_BYTE( "6822.rom", 0x010000, 0x8000, CRC(e9718de5) SHA1(30e3a7d5b33504da03c5780b4a946b977e46098a) )
	ROM_LOAD16_BYTE( "6823.rom", 0x020001, 0x8000, CRC(49422691) SHA1(caee2a4a3f4587ae27dec330214edaa1229012af) )
	ROM_LOAD16_BYTE( "6824.rom", 0x020000, 0x8000, CRC(701deaa4) SHA1(053032ef886b85a4cb4753d17b3c27d228695157) )
	ROM_LOAD16_BYTE( "6825.rom", 0x030001, 0x8000, CRC(6e23c8b4) SHA1(b17fd7d590ed4e6616b7b4d91a47a2820248d8c7) )
	ROM_LOAD16_BYTE( "6826.rom", 0x030000, 0x8000, CRC(77d0de2c) SHA1(83b126ed1d463504b2702391816e6e20dcd04ffc) )
	ROM_LOAD16_BYTE( "6827.rom", 0x040001, 0x8000, CRC(7fa1bfb6) SHA1(a27b54c93613372f59050f0b2182d2984a8d2efe) )
	ROM_LOAD16_BYTE( "6828.rom", 0x040000, 0x8000, CRC(8e880c93) SHA1(8c55deec065daf09a5d1c1c1f3f3f7bc1aeaf563) )
	ROM_LOAD16_BYTE( "6829.rom", 0x050001, 0x8000, CRC(7ca0952d) SHA1(617d73591158ed3fea5174f7dabf0413d28de9b3) )
	ROM_LOAD16_BYTE( "6830.rom", 0x050000, 0x8000, CRC(b1a63aef) SHA1(5db0a1cc2d13c6cfc77044f5d7f6f99d198531ed) )
	ROM_LOAD16_BYTE( "6845.rom", 0x060001, 0x8000, CRC(ba08c9b8) SHA1(65ceaefa18999c468b38576c29101674d1f63e5f) )
	ROM_LOAD16_BYTE( "6846.rom", 0x060000, 0x8000, CRC(f21e57a3) SHA1(92ce0723e722f446c0cef9e23080a008aa9752e7) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* sound CPU */
	ROM_LOAD( "6833.rom", 0x00000, 0x4000, CRC(3b942f5f) SHA1(4384b5c090954e69de561dde0ef32104aa11399a) )

	ROM_REGION( 0x10000, REGION_SOUND1, 0 ) /* Sega PCM sound data */
	ROM_LOAD( "6831.rom", 0x00000, 0x8000, CRC(cfef5481) SHA1(c04b302fee58f0e59a097b2be2b61e5d03df7c91) )
	ROM_LOAD( "6832.rom", 0x08000, 0x8000, CRC(4165aea5) SHA1(be05c6d295807af2f396a1ff72d5a3d2a1e6054d) )

	ROM_REGION( 0x10000, REGION_CPU3, 0 ) /* second 68000 CPU */
	ROM_LOAD16_BYTE( "6920.rom", 0x0000, 0x8000, CRC(1c95013e) SHA1(8344ac953477279c2c701f984d98292a21dd2f7d) )
	ROM_LOAD16_BYTE( "6919.rom", 0x0001, 0x8000, CRC(6ca30d69) SHA1(ed933351883ebf6d9ef9428a81d09749b609cd60) )

	ROM_REGION( 0x40000, REGION_GFX3, 0 ) /* Road Graphics  (region size should be gr_bitmapwidth*256, 0 )*/
	ROM_LOAD( "6840.rom", 0x0000, 0x8000, CRC(581230e3) SHA1(954eab35059322a12a197bba04bf85f816132f20) )
ROM_END

ROM_START( enduror )
	ROM_REGION( 0x040000, REGION_CPU1, 0 ) /* 68000 code */
	ROM_LOAD16_BYTE( "7640a.rom",0x00000, 0x8000, CRC(1d1dc5d4) SHA1(8e7ae5abd23e949de5d5e1772f90e53d05c866ec) )
	ROM_LOAD16_BYTE( "7636a.rom",0x00001, 0x8000, CRC(84131639) SHA1(04981464577d2604eec36c14c5de9c91604ae501) )

	ROM_LOAD16_BYTE( "7641.rom", 0x10000, 0x8000, CRC(2503ae7c) SHA1(27009d5b47dc207145048edfcc1ac8ffda5f0b78) )
	ROM_LOAD16_BYTE( "7637.rom", 0x10001, 0x8000, CRC(82a27a8c) SHA1(4b182d8c23454aed7d786c9824932957319b6eff) )
	ROM_LOAD16_BYTE( "7642.rom", 0x20000, 0x8000, CRC(1c453bea) SHA1(c6e606cdcb1690de05ef5283b48a8a61b2e0ad51) )	/* enduro.a06 / .a09*/
	ROM_LOAD16_BYTE( "7638.rom", 0x20001, 0x8000, CRC(70544779) SHA1(e6403edd7fc0ad5d447c25be5d7f10889aa109ff) )	/* looks like encrypted versions of*/

	ROM_REGION( 0x18000, REGION_GFX1, ROMREGION_DISPOSE ) /* tiles */
	ROM_LOAD( "7644.rom", 0x00000, 0x08000, CRC(e7a4ff90) SHA1(06d18470019041e32be9a969870cd995de626cd6) )
	ROM_LOAD( "7645.rom", 0x08000, 0x08000, CRC(4caa0095) SHA1(a24c741cdca0542e462f17ff94f132c62710e198) )
	ROM_LOAD( "7646.rom", 0x10000, 0x08000, CRC(7e432683) SHA1(c8249b23fce77eb456166161c2d9aa34309efe31) )

	ROM_REGION( 0x100000, REGION_GFX2, 0 ) /* sprites */
	ROM_LOAD( "7678.rom", 0x00000, 0x8000, CRC(9fb5e656) SHA1(264b0ad017eb0fc7e0b542e6dd160ba964c100fd) )
	ROM_LOAD( "7677.rom", 0x08000, 0x8000, CRC(7764765b) SHA1(62543130816c084d292f229a15b3ce1305c99bb3) )
	ROM_LOAD( "7676.rom", 0x10000, 0x8000, CRC(2e42e0d4) SHA1(508f6f89e681b59272884ba129a5c6ffa1b6ba05) )
	ROM_LOAD( "7675.rom", 0x18000, 0x8000, CRC(05cd2d61) SHA1(51688a5a9bc4da3f88ce162ff30affe8c6d3d0c8) )
	ROM_LOAD( "7674.rom", 0x20000, 0x8000, CRC(1a129acf) SHA1(ebaa60ccedc95c58af3ce99105b924b303827f6e) )
	ROM_LOAD( "7673.rom", 0x28000, 0x8000, CRC(82602394) SHA1(d714f397f33a52429f394fc4c403d39be7911ccf) )
	ROM_LOAD( "7672.rom", 0x30000, 0x8000, CRC(d11452f7) SHA1(f68183053005a26c0014050592bad6d63325895e) )
	ROM_LOAD( "7671.rom", 0x38000, 0x8000, CRC(b0c7fdc6) SHA1(c9e0993fed36526e0e46ab2da9413af24b96cae8) )
	ROM_LOAD( "7670.rom", 0x40000, 0x8000, CRC(dbbe2f6e) SHA1(310797a61f91d6866e728e0da3b30828e06d1b52) )
	ROM_LOAD( "7669.rom", 0x48000, 0x8000, CRC(f9525faa) SHA1(fbe2f67a9baee069dbca26a669d0a263bcca0d09) )
	ROM_LOAD( "7668.rom", 0x50000, 0x8000, CRC(e115ce33) SHA1(1af591bc1567b89d0de399e4a02d896fba938bab) )
	ROM_LOAD( "7667.rom", 0x58000, 0x8000, CRC(923bde9d) SHA1(7722a7fdbf45f862f1011d1afae8dedd5885bf52) )
	ROM_LOAD( "7666.rom", 0x60000, 0x8000, CRC(23697257) SHA1(19453b14e8e6789e4c48a80d1b83dbaf37fbdceb) )
	ROM_LOAD( "7665.rom", 0x68000, 0x8000, CRC(12d77607) SHA1(5b5d25646336a8ceae449d5b7a6b70372d81dd8b) )
	ROM_LOAD( "7664.rom", 0x70000, 0x8000, CRC(0df2cfad) SHA1(d62d12922be921967da37fbc624aaed72c4a2a98) )
	ROM_LOAD( "7663.rom", 0x78000, 0x8000, CRC(2b0b8f08) SHA1(14aa1e6866f1c23c9ff271e8f216f6ecc21601ab) )
	ROM_LOAD( "7662.rom", 0x80000, 0x8000, CRC(cb0c13c5) SHA1(856d1234fd8f8146e20fe6c65c0a535b7b7512cd) )
	ROM_LOAD( "7661.rom", 0x88000, 0x8000, CRC(fe93a79b) SHA1(591025a371a451c9cddc8c7480c9841a18bb9a7f) )
	ROM_LOAD( "7660.rom", 0x90000, 0x8000, CRC(86dfbb68) SHA1(a05ac16fbe3aaf34dd46229d4b71fc1f72a3a556) )
	ROM_LOAD( "7659.rom", 0x98000, 0x8000, CRC(629dc8ce) SHA1(4af2a53678890b02922dee54f7cd3c5550001572) )
	ROM_LOAD( "7658.rom", 0xa0000, 0x8000, CRC(1677f24f) SHA1(4786996cc8a04344e82ec9be7c4e7c8a005914a3) )
	ROM_LOAD( "7657.rom", 0xa8000, 0x8000, CRC(8158839c) SHA1(f22081caf11d6b57488c969b5935cd4686e11197) )
	ROM_LOAD( "7656.rom", 0xb0000, 0x8000, CRC(6c741272) SHA1(ccaedda1436ddc339377e610d51e13726bb6c7eb) )
	ROM_LOAD( "7655.rom", 0xb8000, 0x8000, CRC(3433fe7b) SHA1(636449a0707d6629bf6ea503cfb52ad24af1c017) )
	ROM_LOAD( "7654.rom", 0xc0000, 0x8000, CRC(2db6520d) SHA1(d16739e84316b4bd26963b729208169bbf01f499) )
	ROM_LOAD( "7653.rom", 0xc8000, 0x8000, CRC(46a52114) SHA1(d646ab03c1985953401619457d03072833edc6c7) )
	ROM_LOAD( "7652.rom", 0xd0000, 0x8000, CRC(2880cfdb) SHA1(94b78d78d82c324ca108970d8689f1d6b2ca8a24) )
	ROM_LOAD( "7651.rom", 0xd8000, 0x8000, CRC(d7902bad) SHA1(f4872d1a42dcf7d5dbdbc1233606a706b39478d7) )
	ROM_LOAD( "7650.rom", 0xe0000, 0x8000, CRC(642635ec) SHA1(e42bbae178e9a139325633e8c85a606c91e39e36) )
	ROM_LOAD( "7649.rom", 0xe8000, 0x8000, CRC(4edba14c) SHA1(db0aab94de50f8f9501b7afd2fff70fb0a6b2b36) )
	ROM_LOAD( "7648.rom", 0xf0000, 0x8000, CRC(983ea830) SHA1(9629476a300ba711893775ca94dce81a00afd246) )
	ROM_LOAD( "7647.rom", 0xf8000, 0x8000, CRC(2e7fbec0) SHA1(a59ec5fc3341833671fb948cd21b47f3a49db538) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* sound CPU */
	ROM_LOAD( "7682.rom", 0x00000, 0x8000, CRC(c4efbf48) SHA1(2bcbc4757d98f291fcaec467abc36158b3f59be3) )

	ROM_REGION( 0x10000, REGION_SOUND1, 0 ) /* Sega PCM sound data */
	ROM_LOAD( "7681.rom", 0x00000, 0x8000, CRC(bc0c4d12) SHA1(3de71bde4c23e3c31984f20fc4bc7e221354c56f) )
	ROM_LOAD( "7680.rom", 0x08000, 0x8000, CRC(627b3c8c) SHA1(806fe7dce619ad19c09178061be4607d2beba14d) )

	ROM_REGION( 0x10000, REGION_CPU3, 0 ) /* second 68000 CPU */
	ROM_LOAD16_BYTE("7634.rom", 0x0000, 0x8000, CRC(3e07fd32) SHA1(7acb9e9712ecfe928c421c84dece783e75077746) )
	ROM_LOAD16_BYTE("7635.rom", 0x0001, 0x8000, CRC(22f762ab) SHA1(70fa87da76c714db7213c42128a0b6a27644a1d4) )
	/* alternate version?? */
/*	ROM_LOAD16_BYTE("7634a.rom", 0x0000, 0x8000, CRC(aec83731) ) */
/*	ROM_LOAD16_BYTE("7635a.rom", 0x0001, 0x8000, CRC(b2fce96f) ) */

	ROM_REGION( 0x40000, REGION_GFX3, 0 ) /* Road Graphics  (region size should be gr_bitmapwidth*256, 0 )*/
	ROM_LOAD( "7633.rom", 0x0000, 0x8000, CRC(6f146210) SHA1(2f58f0c3563b434ed02700b9ca1545a696a5716e) )
ROM_END

ROM_START( endurobl )
	ROM_REGION( 0x040000+0x010000+0x040000, REGION_CPU1, 0 ) /* 68000 code + space for RAM + space for decrypted opcodes */
	ROM_LOAD16_BYTE( "7.13j", 0x030000, 0x08000, CRC(f1d6b4b7) SHA1(32bd966191cbb36d1e60ed1a06d4caa023dd6b88) )
	ROM_CONTINUE (            0x000000, 0x08000 )
	ROM_LOAD16_BYTE( "4.13h", 0x030001, 0x08000, CRC(43bff873) SHA1(04e906c1965a6211fb8e13987db52f1f99cc0203) )				/* rom de-coded*/
	ROM_CONTINUE (            0x000001, 0x08000 )		/* data de-coded*/

	ROM_LOAD16_BYTE( "8.14j", 0x010000, 0x08000, CRC(2153154a) SHA1(145d8ed59812d26ca412a01ae77cd7872adaba5a) )
	ROM_LOAD16_BYTE( "5.14h", 0x010001, 0x08000, CRC(0a97992c) SHA1(7a6fc8c575637107ed07a30f6f0f8cb8877cbb43) )
	ROM_LOAD16_BYTE( "9.15j", 0x020000, 0x08000, CRC(db3bff1c) SHA1(343ed27a690800683cdd5128dcdb28c7b45288a3) )	/* one byte difference from*/
	ROM_LOAD16_BYTE( "6.15h", 0x020001, 0x08000, CRC(54b1885a) SHA1(f53d906390e5414e73c4cdcbc102d3cb3e719e67) )	/* enduro.a06 / enduro.a09*/

	ROM_REGION( 0x18000, REGION_GFX1, ROMREGION_DISPOSE ) /* tiles */
	ROM_LOAD( "7644.rom", 0x00000, 0x08000, CRC(e7a4ff90) SHA1(06d18470019041e32be9a969870cd995de626cd6) )
	ROM_LOAD( "7645.rom", 0x08000, 0x08000, CRC(4caa0095) SHA1(a24c741cdca0542e462f17ff94f132c62710e198) )
	ROM_LOAD( "7646.rom", 0x10000, 0x08000, CRC(7e432683) SHA1(c8249b23fce77eb456166161c2d9aa34309efe31) )

	ROM_REGION( 0x100000, REGION_GFX2, 0 ) /* sprites */
	ROM_LOAD( "7678.rom", 0x00000, 0x8000, CRC(9fb5e656) SHA1(264b0ad017eb0fc7e0b542e6dd160ba964c100fd) )
	ROM_LOAD( "7677.rom", 0x08000, 0x8000, CRC(7764765b) SHA1(62543130816c084d292f229a15b3ce1305c99bb3) )
	ROM_LOAD( "7676.rom", 0x10000, 0x8000, CRC(2e42e0d4) SHA1(508f6f89e681b59272884ba129a5c6ffa1b6ba05) )
	ROM_LOAD( "7675.rom", 0x18000, 0x8000, CRC(05cd2d61) SHA1(51688a5a9bc4da3f88ce162ff30affe8c6d3d0c8) )
	ROM_LOAD( "7674.rom", 0x20000, 0x8000, CRC(1a129acf) SHA1(ebaa60ccedc95c58af3ce99105b924b303827f6e) )
	ROM_LOAD( "7673.rom", 0x28000, 0x8000, CRC(82602394) SHA1(d714f397f33a52429f394fc4c403d39be7911ccf) )
	ROM_LOAD( "7672.rom", 0x30000, 0x8000, CRC(d11452f7) SHA1(f68183053005a26c0014050592bad6d63325895e) )
	ROM_LOAD( "7671.rom", 0x38000, 0x8000, CRC(b0c7fdc6) SHA1(c9e0993fed36526e0e46ab2da9413af24b96cae8) )
	ROM_LOAD( "7670.rom", 0x40000, 0x8000, CRC(dbbe2f6e) SHA1(310797a61f91d6866e728e0da3b30828e06d1b52) )
	ROM_LOAD( "7669.rom", 0x48000, 0x8000, CRC(f9525faa) SHA1(fbe2f67a9baee069dbca26a669d0a263bcca0d09) )
	ROM_LOAD( "7668.rom", 0x50000, 0x8000, CRC(e115ce33) SHA1(1af591bc1567b89d0de399e4a02d896fba938bab) )
	ROM_LOAD( "7667.rom", 0x58000, 0x8000, CRC(923bde9d) SHA1(7722a7fdbf45f862f1011d1afae8dedd5885bf52) )
	ROM_LOAD( "7666.rom", 0x60000, 0x8000, CRC(23697257) SHA1(19453b14e8e6789e4c48a80d1b83dbaf37fbdceb) )
	ROM_LOAD( "7665.rom", 0x68000, 0x8000, CRC(12d77607) SHA1(5b5d25646336a8ceae449d5b7a6b70372d81dd8b) )
	ROM_LOAD( "7664.rom", 0x70000, 0x8000, CRC(0df2cfad) SHA1(d62d12922be921967da37fbc624aaed72c4a2a98) )
	ROM_LOAD( "7663.rom", 0x78000, 0x8000, CRC(2b0b8f08) SHA1(14aa1e6866f1c23c9ff271e8f216f6ecc21601ab) )
	ROM_LOAD( "7662.rom", 0x80000, 0x8000, CRC(cb0c13c5) SHA1(856d1234fd8f8146e20fe6c65c0a535b7b7512cd) )
	ROM_LOAD( "7661.rom", 0x88000, 0x8000, CRC(fe93a79b) SHA1(591025a371a451c9cddc8c7480c9841a18bb9a7f) )
	ROM_LOAD( "7660.rom", 0x90000, 0x8000, CRC(86dfbb68) SHA1(a05ac16fbe3aaf34dd46229d4b71fc1f72a3a556) )
	ROM_LOAD( "7659.rom", 0x98000, 0x8000, CRC(629dc8ce) SHA1(4af2a53678890b02922dee54f7cd3c5550001572) )
	ROM_LOAD( "7658.rom", 0xa0000, 0x8000, CRC(1677f24f) SHA1(4786996cc8a04344e82ec9be7c4e7c8a005914a3) )
	ROM_LOAD( "7657.rom", 0xa8000, 0x8000, CRC(8158839c) SHA1(f22081caf11d6b57488c969b5935cd4686e11197) )
	ROM_LOAD( "7656.rom", 0xb0000, 0x8000, CRC(6c741272) SHA1(ccaedda1436ddc339377e610d51e13726bb6c7eb) )
	ROM_LOAD( "7655.rom", 0xb8000, 0x8000, CRC(3433fe7b) SHA1(636449a0707d6629bf6ea503cfb52ad24af1c017) )
	ROM_LOAD( "7654.rom", 0xc0000, 0x8000, CRC(2db6520d) SHA1(d16739e84316b4bd26963b729208169bbf01f499) )
	ROM_LOAD( "7653.rom", 0xc8000, 0x8000, CRC(46a52114) SHA1(d646ab03c1985953401619457d03072833edc6c7) )
	ROM_LOAD( "7652.rom", 0xd0000, 0x8000, CRC(2880cfdb) SHA1(94b78d78d82c324ca108970d8689f1d6b2ca8a24) )
	ROM_LOAD( "7651.rom", 0xd8000, 0x8000, CRC(d7902bad) SHA1(f4872d1a42dcf7d5dbdbc1233606a706b39478d7) )
	ROM_LOAD( "7650.rom", 0xe0000, 0x8000, CRC(642635ec) SHA1(e42bbae178e9a139325633e8c85a606c91e39e36) )
	ROM_LOAD( "7649.rom", 0xe8000, 0x8000, CRC(4edba14c) SHA1(db0aab94de50f8f9501b7afd2fff70fb0a6b2b36) )
	ROM_LOAD( "7648.rom", 0xf0000, 0x8000, CRC(983ea830) SHA1(9629476a300ba711893775ca94dce81a00afd246) )
	ROM_LOAD( "7647.rom", 0xf8000, 0x8000, CRC(2e7fbec0) SHA1(a59ec5fc3341833671fb948cd21b47f3a49db538) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* sound CPU */
	ROM_LOAD( "13.16d", 0x00000, 0x004000, CRC(81c82fc9) SHA1(99eae7edc62d719993c46a703f9daaf332e236e9) )
	ROM_LOAD( "12.16e", 0x04000, 0x004000, CRC(755bfdad) SHA1(2942f3da5a45a3ac7bba6a73142663fd975f4379) )

	ROM_REGION( 0x10000, REGION_SOUND1, 0 ) /* Sega PCM sound data */
	ROM_LOAD( "7681.rom", 0x00000, 0x8000, CRC(bc0c4d12) SHA1(3de71bde4c23e3c31984f20fc4bc7e221354c56f) )
	ROM_LOAD( "7680.rom", 0x08000, 0x8000, CRC(627b3c8c) SHA1(806fe7dce619ad19c09178061be4607d2beba14d) )

	ROM_REGION( 0x10000, REGION_CPU3, 0 ) /* second 68000 CPU */
	ROM_LOAD16_BYTE("7634.rom", 0x0000, 0x8000, CRC(3e07fd32) SHA1(7acb9e9712ecfe928c421c84dece783e75077746) )
	ROM_LOAD16_BYTE("7635.rom", 0x0001, 0x8000, CRC(22f762ab) SHA1(70fa87da76c714db7213c42128a0b6a27644a1d4) )

	ROM_REGION( 0x40000, REGION_GFX3, 0 ) /* Road Graphics  (region size should be gr_bitmapwidth*256, 0 )*/
	ROM_LOAD( "7633.rom", 0x0000, 0x8000, CRC(6f146210) SHA1(2f58f0c3563b434ed02700b9ca1545a696a5716e) )
ROM_END

ROM_START( endurob2 )
	ROM_REGION( 0x040000+0x010000+0x040000, REGION_CPU1, 0 ) /* 68000 code + space for RAM + space for decrypted opcodes */
	ROM_LOAD16_BYTE( "enduro.a07", 0x000000, 0x08000, CRC(259069bc) SHA1(42fa47ce4a29294f9eff3eddbba6c305d750aaa5) )
	ROM_LOAD16_BYTE( "enduro.a04", 0x000001, 0x08000, CRC(f584fbd9) SHA1(6c9ddcd1d9cf95c6250b705b27865644da45d197) )
	ROM_LOAD16_BYTE( "enduro.a08", 0x010000, 0x08000, CRC(d234918c) SHA1(ce2493a4ceff48331551e915fdbe19107865436e) )
	ROM_LOAD16_BYTE( "enduro.a05", 0x010001, 0x08000, CRC(a525dd57) SHA1(587f449ea317dc9eae06e755e7c63a652effbe15) )
	ROM_LOAD16_BYTE( "enduro.a09", 0x020000, 0x08000, CRC(f6391091) SHA1(3160b342b6447cccf67c932c7c1a42354cdfb058) )
	ROM_LOAD16_BYTE( "enduro.a06", 0x020001, 0x08000, CRC(79b367d7) SHA1(e901036b1b9fac460415d513837c8f852f7750b0) )

	ROM_REGION( 0x18000, REGION_GFX1, ROMREGION_DISPOSE ) /* tiles */
	ROM_LOAD( "7644.rom", 0x00000, 0x08000, CRC(e7a4ff90) SHA1(06d18470019041e32be9a969870cd995de626cd6) )
	ROM_LOAD( "7645.rom", 0x08000, 0x08000, CRC(4caa0095) SHA1(a24c741cdca0542e462f17ff94f132c62710e198) )
	ROM_LOAD( "7646.rom", 0x10000, 0x08000, CRC(7e432683) SHA1(c8249b23fce77eb456166161c2d9aa34309efe31) )

	ROM_REGION( 0x100000, REGION_GFX2, 0 ) /* sprites */
	ROM_LOAD( "7678.rom",		0x00000, 0x8000, CRC(9fb5e656) SHA1(264b0ad017eb0fc7e0b542e6dd160ba964c100fd) )
	ROM_LOAD( "7677.rom", 		0x08000, 0x8000, CRC(7764765b) SHA1(62543130816c084d292f229a15b3ce1305c99bb3) )
	ROM_LOAD( "7676.rom", 		0x10000, 0x8000, CRC(2e42e0d4) SHA1(508f6f89e681b59272884ba129a5c6ffa1b6ba05) )
	ROM_LOAD( "enduro.a20", 	0x18000, 0x8000, CRC(7c280bc8) SHA1(ad8bb0204a53ea1415f088819748d40c47d96509) )
	ROM_LOAD( "7674.rom", 		0x20000, 0x8000, CRC(1a129acf) SHA1(ebaa60ccedc95c58af3ce99105b924b303827f6e) )
	ROM_LOAD( "7673.rom",		0x28000, 0x8000, CRC(82602394) SHA1(d714f397f33a52429f394fc4c403d39be7911ccf) )
	ROM_LOAD( "7672.rom", 		0x30000, 0x8000, CRC(d11452f7) SHA1(f68183053005a26c0014050592bad6d63325895e) )
	ROM_LOAD( "7671.rom", 		0x38000, 0x8000, CRC(b0c7fdc6) SHA1(c9e0993fed36526e0e46ab2da9413af24b96cae8) )
	ROM_LOAD( "7670.rom", 		0x40000, 0x8000, CRC(dbbe2f6e) SHA1(310797a61f91d6866e728e0da3b30828e06d1b52) )
	ROM_LOAD( "7669.rom", 		0x48000, 0x8000, CRC(f9525faa) SHA1(fbe2f67a9baee069dbca26a669d0a263bcca0d09) )
	ROM_LOAD( "7668.rom", 		0x50000, 0x8000, CRC(e115ce33) SHA1(1af591bc1567b89d0de399e4a02d896fba938bab) )
	ROM_LOAD( "enduro.a28", 	0x58000, 0x8000, CRC(321f034b) SHA1(e30f541d0f17a75ac02a49bd5d621c75fdd89298) )
	ROM_LOAD( "7666.rom", 		0x60000, 0x8000, CRC(23697257) SHA1(19453b14e8e6789e4c48a80d1b83dbaf37fbdceb) )
	ROM_LOAD( "7665.rom", 		0x68000, 0x8000, CRC(12d77607) SHA1(5b5d25646336a8ceae449d5b7a6b70372d81dd8b) )
	ROM_LOAD( "7664.rom", 		0x70000, 0x8000, CRC(0df2cfad) SHA1(d62d12922be921967da37fbc624aaed72c4a2a98) )
	ROM_LOAD( "7663.rom", 		0x78000, 0x8000, CRC(2b0b8f08) SHA1(14aa1e6866f1c23c9ff271e8f216f6ecc21601ab) )
	ROM_LOAD( "7662.rom", 		0x80000, 0x8000, CRC(cb0c13c5) SHA1(856d1234fd8f8146e20fe6c65c0a535b7b7512cd) )
	ROM_LOAD( "enduro.a34", 	0x88000, 0x8000, CRC(296454d8) SHA1(17e829a08606837d36006849edffe54c76c384d5) )
	ROM_LOAD( "enduro.a35", 	0x90000, 0x8000, CRC(1ebe76df) SHA1(c68257d92b79cd346ca9f5639e6b3dffc6e21a5d) )
	ROM_LOAD( "enduro.a36",		0x98000, 0x8000, CRC(243e34e5) SHA1(4117435e97841ac2e0233089343f14b4a27dcaed) )
	ROM_LOAD( "7658.rom", 		0xa0000, 0x8000, CRC(1677f24f) SHA1(4786996cc8a04344e82ec9be7c4e7c8a005914a3) )
	ROM_LOAD( "7657.rom", 		0xa8000, 0x8000, CRC(8158839c) SHA1(f22081caf11d6b57488c969b5935cd4686e11197) )
	ROM_LOAD( "enduro.a39",		0xb0000, 0x8000, CRC(1ff3a5e2) SHA1(b4672ed06f6f1ed28538e6dc63efa6eed5c34587) )
	ROM_LOAD( "7655.rom", 		0xb8000, 0x8000, CRC(3433fe7b) SHA1(636449a0707d6629bf6ea503cfb52ad24af1c017) )
	ROM_LOAD( "7654.rom", 		0xc0000, 0x8000, CRC(2db6520d) SHA1(d16739e84316b4bd26963b729208169bbf01f499) )
	ROM_LOAD( "7653.rom", 		0xc8000, 0x8000, CRC(46a52114) SHA1(d646ab03c1985953401619457d03072833edc6c7) )
	ROM_LOAD( "7652.rom", 		0xd0000, 0x8000, CRC(2880cfdb) SHA1(94b78d78d82c324ca108970d8689f1d6b2ca8a24) )
	ROM_LOAD( "enduro.a44", 	0xd8000, 0x8000, CRC(84bb12a1) SHA1(340de454cee9d78f8b64e12b74450b7a152b8726) )
	ROM_LOAD( "7650.rom", 		0xe0000, 0x8000, CRC(642635ec) SHA1(e42bbae178e9a139325633e8c85a606c91e39e36) )
	ROM_LOAD( "7649.rom", 		0xe8000, 0x8000, CRC(4edba14c) SHA1(db0aab94de50f8f9501b7afd2fff70fb0a6b2b36) )
	ROM_LOAD( "7648.rom", 		0xf0000, 0x8000, CRC(983ea830) SHA1(9629476a300ba711893775ca94dce81a00afd246) )
	ROM_LOAD( "7647.rom", 		0xf8000, 0x8000, CRC(2e7fbec0) SHA1(a59ec5fc3341833671fb948cd21b47f3a49db538) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* sound CPU */
	ROM_LOAD( "enduro.a16", 0x00000, 0x8000, CRC(d2cb6eb5) SHA1(80c5fab16ec4ddfa67fae94808026b2e6285b7f1) )

	ROM_REGION( 0x10000, REGION_SOUND1, 0 ) /* Sega PCM sound data */
	ROM_LOAD( "7681.rom", 0x00000, 0x8000, CRC(bc0c4d12) SHA1(3de71bde4c23e3c31984f20fc4bc7e221354c56f) )
	ROM_LOAD( "7680.rom", 0x08000, 0x8000, CRC(627b3c8c) SHA1(806fe7dce619ad19c09178061be4607d2beba14d) )

	ROM_REGION( 0x10000, REGION_CPU3, 0 ) /* second 68000 CPU */
	ROM_LOAD16_BYTE("7634.rom", 0x0000, 0x8000, CRC(3e07fd32) SHA1(7acb9e9712ecfe928c421c84dece783e75077746) )
	ROM_LOAD16_BYTE("7635.rom", 0x0001, 0x8000, CRC(22f762ab) SHA1(70fa87da76c714db7213c42128a0b6a27644a1d4) )

	ROM_REGION( 0x40000, REGION_GFX3, 0 ) /* Road Graphics  (region size should be gr_bitmapwidth*256, 0 )*/
	ROM_LOAD( "7633.rom", 0x0000, 0x8000, CRC(6f146210) SHA1(2f58f0c3563b434ed02700b9ca1545a696a5716e) )
ROM_END

ROM_START( sharrier )
	ROM_REGION( 0x040000, REGION_CPU1, 0 ) /* 68000 code */
	ROM_LOAD16_BYTE( "ic97.bin", 0x000000, 0x8000, CRC(7c30a036) SHA1(d3902342be714b4e181c87ad2bad7102e3eeec20) )
	ROM_LOAD16_BYTE( "ic84.bin", 0x000001, 0x8000, CRC(16deaeb1) SHA1(bdf85b924a914865bf876eda7fc2b20131a4cf2d) )
	ROM_LOAD16_BYTE( "ic98.bin", 0x010000, 0x8000, CRC(40b1309f) SHA1(9b050983f043a88f414745d02c912b59bbf1b121) )
	ROM_LOAD16_BYTE( "ic85.bin", 0x010001, 0x8000, CRC(ce78045c) SHA1(ce640f05bed64ff5b47f1064b5fc13e58bc422a4) )
	ROM_LOAD16_BYTE( "ic99.bin", 0x020000, 0x8000, CRC(f6391091) SHA1(3160b342b6447cccf67c932c7c1a42354cdfb058) )
	ROM_LOAD16_BYTE( "ic86.bin", 0x020001, 0x8000, CRC(79b367d7) SHA1(e901036b1b9fac460415d513837c8f852f7750b0) )
	ROM_LOAD16_BYTE( "ic100.bin", 0x030000, 0x8000, CRC(6171e9d3) SHA1(72f8736f421dc93139859fd47f0c8c3c32b6ff0b) )
	ROM_LOAD16_BYTE( "ic87.bin", 0x030001, 0x8000, CRC(70cb72ef) SHA1(d1d89bd133b6905f81c25513d852b7e3a05a7312) )

	ROM_REGION( 0x18000, REGION_GFX1, ROMREGION_DISPOSE ) /* tiles */
	ROM_LOAD( "sic31.bin", 0x00000, 0x08000, CRC(347fa325) SHA1(9076b16de9598b52a75e5084651ee5a220b0e88b) )
	ROM_LOAD( "sic46.bin", 0x08000, 0x08000, CRC(39d98bd1) SHA1(5aab91bdd08b0f1ea537cd43ccc2e82fd01dd031) )
	ROM_LOAD( "sic60.bin", 0x10000, 0x08000, CRC(3da3ea6b) SHA1(9a6ce304a14e6ef0be41d867284a63b941f960fb) )

	ROM_REGION( 0x100000, REGION_GFX2, 0 ) /* sprites */
	ROM_LOAD( "ic36.bin", 0x00000, 0x8000, CRC(93e2d264) SHA1(ca56de13756ab77408506d88f291da1da8134435) )
	ROM_LOAD( "ic35.bin", 0x08000, 0x8000, CRC(cd6e7500) SHA1(6f0e42eb28ad3f5df93d8bb39dc41aba66eee144) )
	ROM_LOAD( "ic34.bin", 0x10000, 0x8000, CRC(d5e15e66) SHA1(2bd81c5c736d725577adf85532de7802bef057f2) )
	ROM_LOAD( "ic33.bin", 0x18000, 0x8000, CRC(60d7c1bb) SHA1(19ddc1d8f269b50343266d508ad04d4c0fff69d1) )
	ROM_LOAD( "ic32.bin", 0x20000, 0x8000, CRC(6d7b5c97) SHA1(94c27e4ef1e197ee136f9399b07520cae00a366f) )
	ROM_LOAD( "ic31.bin", 0x28000, 0x8000, CRC(5e784271) SHA1(063bd83b7f42dec556a7bdf736cb51456ba7184b) )
	ROM_LOAD( "ic30.bin", 0x30000, 0x8000, CRC(ec42c9ef) SHA1(313d908f3a964529b18e09825552879817a2e159) )
	ROM_LOAD( "ic29.bin", 0x38000, 0x8000, CRC(ed51fdc4) SHA1(a2696b15a0911ac3b6b330b7d8df58ca51d629de) )
	ROM_LOAD( "ic28.bin", 0x40000, 0x8000, CRC(edbf5fc3) SHA1(a93f8c431075741c181eb422b24c9303487ca16c) )
	ROM_LOAD( "ic27.bin", 0x48000, 0x8000, CRC(41f25a9c) SHA1(bebbd4c4600028205aeff54190b32b664e4710d0) )
	ROM_LOAD( "ic26.bin", 0x50000, 0x8000, CRC(ac62ae2e) SHA1(d472dcc1d9b7889d04870920d5c6e5578597b8dc) )
	ROM_LOAD( "ic25.bin", 0x58000, 0x8000, CRC(f6330038) SHA1(805d4ed664972c0773c837d62f094858c1804148) )
	ROM_LOAD( "ic24.bin", 0x60000, 0x8000, CRC(cebf797c) SHA1(d3d5aeba1a0e70a322ec86b930814fa8bc744829) )
	ROM_LOAD( "ic23.bin", 0x68000, 0x8000, CRC(510e5e10) SHA1(47b9f1bc8df0690c37d1d045bd361f8599e8a903) )
	ROM_LOAD( "ic22.bin", 0x70000, 0x8000, CRC(6d4a7d7a) SHA1(997ac38e47d84f0166ca3ece50ac5f2d3435d8d3) )
	ROM_LOAD( "ic21.bin", 0x78000, 0x8000, CRC(dfe75f3d) SHA1(ff908419066494bc28cbd6afe72bd30350b20c4b) )
	ROM_LOAD( "ic118.bin",0x80000, 0x8000, CRC(e8c537d8) SHA1(c9b3c0f33272c47d32e6aa349d72f7e355468e0e) )
	ROM_LOAD( "ic17.bin", 0x88000, 0x8000, CRC(5bb09a67) SHA1(8bedefd2fa29a1a5e36f1d81c4e9067e3c7f28e9) )
	ROM_LOAD( "ic16.bin", 0x90000, 0x8000, CRC(9c782295) SHA1(c1627f3d849d2fdb02a590502bafbe133212b943) )
	ROM_LOAD( "ic15.bin", 0x98000, 0x8000, CRC(60737b98) SHA1(5e91498bc473f099a1b2887baf980486e922af97) )
	ROM_LOAD( "ic14.bin", 0xa0000, 0x8000, CRC(24596a8b) SHA1(f580022c4f7dcaefb7209058c310a329479b5fcd) )
	ROM_LOAD( "ic13.bin", 0xa8000, 0x8000, CRC(7a2dad15) SHA1(227865447027f8669aa0d06d059f3d61da6d59dd) )
	ROM_LOAD( "ic12.bin", 0xb0000, 0x8000, CRC(0f732717) SHA1(6a19c88d5d52f4ec4adb32c511fed4caae81c65f) )
	ROM_LOAD( "ic11.bin", 0xb8000, 0x8000, CRC(a2c07741) SHA1(747c029ab399c4110dbe360b8913f5c2e57c87cc) )
	ROM_LOAD( "ic8.bin",  0xc0000, 0x8000, CRC(22844fa4) SHA1(6d0f177082084c8c92085cd53e1d7ddc62d09a4c) )
	ROM_LOAD( "ic7.bin",  0xc8000, 0x8000, CRC(dcaa2ebf) SHA1(9cf77bb966859febc5e5f1447cb719db64aa4db4) )
	ROM_LOAD( "ic6.bin",  0xd0000, 0x8000, CRC(3711105c) SHA1(075197f614786f89bee28ed944371223bc75c9be) )
	ROM_LOAD( "ic5.bin",  0xd8000, 0x8000, CRC(70fb5ebb) SHA1(38755a9be92865d2c5da8112d8d9c0fe8e778cff) )
	ROM_LOAD( "ic4.bin",  0xe0000, 0x8000, CRC(b537d082) SHA1(f87a644d9af8477c9eb94e5d3aeb5f6376264418) )
	ROM_LOAD( "ic3.bin",  0xe8000, 0x8000, CRC(f5ba4e08) SHA1(443b07c996cbb213fe57dfdd3879c1d4da27c001) )
	ROM_LOAD( "ic2.bin",  0xf0000, 0x8000, CRC(fc3bf8f3) SHA1(d9168b9bce110bfd531410179a107895c641e105) )
	ROM_LOAD( "ic1.bin",  0xf8000, 0x8000, CRC(b191e22f) SHA1(406c7f4eed0b8fe93fa0bef370e496894f4d46a4) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* sound CPU */
	ROM_LOAD( "ic73.bin", 0x00000, 0x004000, CRC(d6397933) SHA1(b85bb47efb6c113b3676b10ab86f1798a89d45b4) )
	ROM_LOAD( "ic72.bin", 0x04000, 0x004000, CRC(504e76d9) SHA1(302af9101da01c97ca4be6acd21fb5b8e8f0b7ef) )

	ROM_REGION( 0x10000, REGION_SOUND1, 0 ) /* Sega PCM sound data */
	ROM_LOAD( "snd7231.256", 0x00000, 0x8000, CRC(871c6b14) SHA1(6d04ddc32fdf1db409cb519890821bd10fc9e58b) )
	ROM_LOAD( "snd7232.256", 0x08000, 0x8000, CRC(4b59340c) SHA1(a01ba8580b65dd17bfd92560265e502d95d3ff16) )

	ROM_REGION( 0x10000, REGION_CPU3, 0 ) /* second 68000 CPU */
	ROM_LOAD16_BYTE( "ic54.bin", 0x0000, 0x8000, CRC(d7c535b6) SHA1(c0659a678c0c3776387a4a675016e9a2e9c67ee3) )
	ROM_LOAD16_BYTE( "ic67.bin", 0x0001, 0x8000, CRC(a6153af8) SHA1(b56ba472e4afb474c7a3f7dc11d7428ebbe1a9c7) )

	ROM_REGION( 0x40000, REGION_GFX3, 0 ) /* Road Graphics  (region size should be gr_bitmapwidth*256, 0 )*/
	ROM_LOAD( "pic2.bin", 0x0000, 0x8000, CRC(b4740419) SHA1(8ece2dc85692e32d0ba0b427c260c3d10ac0b7cc) )
ROM_END

/***************************************************************************/

INPUT_PORTS_START( hangon )
	PORT_START	/* Steering */
		PORT_ANALOG( 0xff, 0x7f, IPT_AD_STICK_X | IPF_REVERSE | IPF_CENTER , 100, 3, 0x48, 0xb7 )
	PORT_START	/* Accel / Decel */
		PORT_ANALOG( 0xff, 0x1, IPT_AD_STICK_Y | IPF_CENTER | IPF_REVERSE, 100, 16, 0, 0xa2 )
	SYS16_SERVICE
	SYS16_COINAGE
	PORT_START	/* DSW1 */
		PORT_DIPNAME( 0x01, 0x00, DEF_STR( Demo_Sounds ) )
		PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
		PORT_DIPSETTING(    0x00, DEF_STR( On ) )
		PORT_DIPNAME( 0x06, 0x06, DEF_STR( Difficulty ) )
		PORT_DIPSETTING(    0x04, "Easy" )
		PORT_DIPSETTING(    0x06, "Normal" )
		PORT_DIPSETTING(    0x02, "Hard" )
		PORT_DIPSETTING(    0x00, "Hardest" )
		PORT_DIPNAME( 0x18, 0x18, "Time Adj." )
		PORT_DIPSETTING(    0x18, "Normal" )
		PORT_DIPSETTING(    0x10, "Medium" )
		PORT_DIPSETTING(    0x08, "Hard" )
		PORT_DIPSETTING(    0x00, "Hardest" )
		PORT_DIPNAME( 0x20, 0x20, "Play Music" )
		PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
		PORT_DIPSETTING(    0x20, DEF_STR( On ) )
	PORT_START	/* Brake */
		PORT_ANALOG( 0xff, 0x1, IPT_AD_STICK_Z | IPF_CENTER | IPF_REVERSE, 100, 16, 0, 0xa2 )
INPUT_PORTS_END

INPUT_PORTS_START( enduror )
	PORT_START	/* handle right left */
		PORT_ANALOG( 0xff, 0x7f, IPT_AD_STICK_X | IPF_REVERSE | IPF_CENTER, 100, 4, 0x0, 0xff )
	PORT_START	/* Fake Buttons */
		PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON1 )	/* accel*/
		PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_BUTTON2 )	/* brake*/
		PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN )	/* wheelie*/
	PORT_START
		PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
		PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
		PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_SERVICE, DEF_STR( Service_Mode ), KEYCODE_F2, IP_JOY_NONE )
		PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_SERVICE1 )
		PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
		PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
		PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START1 )
		PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )
	SYS16_COINAGE
	PORT_START	/* DSW1 */
		PORT_DIPNAME( 0x01, 0x00, DEF_STR( Cabinet ) )
		PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
		PORT_DIPSETTING(    0x01, "Moving" )
		PORT_DIPNAME( 0x06, 0x06, DEF_STR( Difficulty ) )
		PORT_DIPSETTING(    0x04, "Easy" )
		PORT_DIPSETTING(    0x06, "Normal" )
		PORT_DIPSETTING(    0x02, "Hard" )
		PORT_DIPSETTING(    0x00, "Hardest" )
		PORT_DIPNAME( 0x18, 0x18, "Time Adjust" )
		PORT_DIPSETTING(    0x10, "Easy" )
		PORT_DIPSETTING(    0x18, "Normal" )
		PORT_DIPSETTING(    0x08, "Hard" )
		PORT_DIPSETTING(    0x00, "Hardest" )
		PORT_DIPNAME( 0x60, 0x60, "Time Control" )
		PORT_DIPSETTING(    0x40, "Easy" )
		PORT_DIPSETTING(    0x60, "Normal" )
		PORT_DIPSETTING(    0x20, "Hard" )
		PORT_DIPSETTING(    0x00, "Hardest" )
		PORT_DIPNAME( 0x80, 0x00, DEF_STR( Demo_Sounds ) )
		PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
		PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	/*PORT_START	 /* Y */
	/*PORT_ANALOG( 0xff, 0x0, IPT_AD_STICK_Y | IPF_CENTER , 100, 8, 0x0, 0xff )*/
INPUT_PORTS_END

INPUT_PORTS_START( sharrier )
	SYS16_JOY1
	SYS16_JOY2
	PORT_START
		PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
		PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
		PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_SERVICE, DEF_STR( Service_Mode ), KEYCODE_F2, IP_JOY_NONE )
		PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_SERVICE1 )
		PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START1 )
		PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 )
		PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
		PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )
	SYS16_COINAGE
	PORT_START	/* DSW1 */
		PORT_DIPNAME( 0x01, 0x00, DEF_STR( Cabinet ) )
		PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
		PORT_DIPSETTING(    0x01, "Moving" )
		PORT_DIPNAME( 0x02, 0x00, DEF_STR( Demo_Sounds ) )
		PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
		PORT_DIPSETTING(    0x00, DEF_STR( On ) )
		PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Lives ) )
		PORT_DIPSETTING(    0x08, "2" )
		PORT_DIPSETTING(    0x0c, "3" )
		PORT_DIPSETTING(    0x04, "4" )
		PORT_DIPSETTING(    0x00, "5" )
		PORT_DIPNAME( 0x10, 0x10, "Add Player Score" )
		PORT_DIPSETTING(    0x10, "5000000" )
		PORT_DIPSETTING(    0x00, "7000000" )
		PORT_DIPNAME( 0x20, 0x20, "Trial Time" )
		PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
		PORT_DIPSETTING(    0x00, DEF_STR( On ) )
		PORT_DIPNAME( 0xc0, 0xc0, DEF_STR( Difficulty ) )
		PORT_DIPSETTING(    0x80, "Easy" )
		PORT_DIPSETTING(    0xc0, "Normal" )
		PORT_DIPSETTING(    0x40, "Hard" )
		PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_START	/* X */
		PORT_ANALOG( 0xff, 0x7f, IPT_AD_STICK_X |  IPF_REVERSE, 100, 4, 0x20, 0xdf )
	PORT_START	/* Y */
		PORT_ANALOG( 0xff, 0x7f, IPT_AD_STICK_Y |  IPF_REVERSE, 100, 4, 0x60, 0x9f )
INPUT_PORTS_END

/***************************************************************************/

GAME( 1985, hangon,   0,        hangon,   hangon,   hangon,   ROT0, "Sega",    "Hang-On" )
GAME( 1985, sharrier, 0,        sharrier, sharrier, sharrier, ROT0, "Sega",    "Space Harrier" )
GAMEX(1986, enduror,  0,        enduror,  enduror,  enduror,  ROT0, "Sega",    "Enduro Racer", GAME_NOT_WORKING )
GAME( 1986, endurobl, enduror,  enduror,  enduror,  endurobl, ROT0, "bootleg", "Enduro Racer (bootleg set 1)" )
GAME( 1986, endurob2, enduror,  endurob2, enduror,  endurob2, ROT0, "bootleg", "Enduro Racer (bootleg set 2)" )
