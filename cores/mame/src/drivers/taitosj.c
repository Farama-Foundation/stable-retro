/***************************************************************************

Taito SJ system memory map

MAIN CPU:

0000-7fff ROM (6000-7fff banked in two banks, controlled by bit 7 of d50e)
8000-87ff RAM
9000-bfff Character generator RAM
c400-c7ff Video RAM: front playfield
c800-cbff Video RAM: middle playfield
cc00-cfff Video RAM: back playfield
d100-d17f Sprites (d140-d15f are NOT sprites)
d200-d27f Palette (64 pairs: xxxxxxxR RRGGGBBB. bits are inverted, i.e. 0x01ff = black)
e000-efff ROM (on the protection board with the 68705)

read:

8800      68705 data read
8801      68705 status read
	    bit 0 = the 68705 has read data from the Z80
	    bit 1 = the 68705 has written data for the Z80
d400-d403 hardware collision detection registers
	  d400 bit0-7 = sprite 0x00-0x07 collided
	  d401 bit0-7 = sprite 0x08-0x0f collided
	  d402 bit0-7 = sprite 0x18-0x1f collided
	  d403 bit0 = obj/pf1
	       bit1 = obj/pf2
	       bit2 = obj/pf3
	       bit3 = pf1/pf2
	       bit4 = pf1/pf3
	       bit5 = pf2/pf3
	       bit6 = nc
	       bit7 = nc
d404      returns contents of graphic ROM, pointed by d509-d50a
d408      IN0
d409      IN1
d40a      DSW1
d40b      IN2 - can come from a ROM or PAL chip
d40c      COIN
d40d      another input port (used for player 2 dial)
d40f      8910 #0 read
	    port A DSW2
	    port B DSW3

write
8800      68705 data write
d000-d01f front playfield column scroll
d020-d03f middle playfield column scroll
d040-d05f back playfield column scroll
d300      playfield priority control
	  bits 0-3 go to A4-A7 of a 256x4 PROM
		  bit 4 selects D0/D1 or D2/D3 of the PROM
		  bit 5-7 n.c.
	  A0-A3 of the PROM is fed with a mask of the inactive planes
		    (i.e. all-zero) in the order sprites-front-middle-back
	  the 2-bit code which comes out from the PROM selects the plane
		  to display.
d40e      8910 #0 control
d40f      8910 #0 write
d500      front playfield horizontal scroll
d501      front playfield vertical scroll
d502      middle playfield horizontal scroll
d503      middle playfield vertical scroll
d504      back playfield horizontal scroll
d505      back playfield vertical scroll
d506      bits 0-2 = front playfield color code
	  bit 3 = front playfield character bank
	  bits 4-6 = middle playfield color code
	  bit 7 = middle playfield character bank
d507      bits 0-2 = back playfield color code
	  bit 3 = back playfield character bank
	  bits 4-5 = sprite color bank (1 bank = 2 color codes)
d508      clear hardware collision detection registers
d509-d50a pointer to graphic ROM to read from d404
d50b      command for the audio CPU
d50d      watchdog reset
d50e      bit 7 = ROM bank selector
		  bit 0-4 = protection write (Alpine Ski); result is read from d40b bits 0-3
d50f      can go to a ROM or PAL; the result is read from d40b
		  ==> used in Alpine Ski (Set 1) for protection
d600      bit 0 horizontal screen flip
	  bit 1 vertical screen flip
	  bit 2 ? sprite related, called OBJEX. It looks like there are 256
		  bytes of sprite RAM, but only 128 can be acessed by the video
		  hardware at a time. This select the high or low bank. The CPU
		  can access all the memory linearly. I don't know if this is
		  ever used.
	  bit 3 n.c.
		  bit 4 front playfield enable
		  bit 5 middle playfield enable
		  bit 6 back playfield enable
		  bit 7 sprites enable


SOUND CPU:
0000-3fff ROM (none of the games have this fully populated)
4000-43ff RAM
e000-efff space for diagnostics ROM?

read:
5000      command from CPU board
8101      ?

write:
4800      8910 #1  control
4801      8910 #1  write
	    PORT A  digital sound out
4802      8910 #2  control
4803      8910 #2  write
4804      8910 #3  control
4805      8910 #3  write
	    port B bit 0 SOUND CPU NMI disable


Kickstart Wheelie King :
- additional ram @ $d800-$dfff (scroll ram  + ??)
- taitosj_colorbank_w @ $d000-$d001
- taitosj_scroll @ $d002-$d007
- strange controls :
	 - 'revolve type' - 3 pos switch (gears) +  button/pedal (accel)
	 - two buttons for gear change, auto acceleration
***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "cpu/z80/z80.h"



MACHINE_INIT( taitosj );
WRITE_HANDLER( taitosj_bankswitch_w );
READ_HANDLER( taitosj_fake_data_r );
READ_HANDLER( taitosj_fake_status_r );
WRITE_HANDLER( taitosj_fake_data_w );
READ_HANDLER( taitosj_mcu_data_r );
READ_HANDLER( taitosj_mcu_status_r );
WRITE_HANDLER( taitosj_mcu_data_w );
READ_HANDLER( taitosj_68705_portA_r );
READ_HANDLER( taitosj_68705_portB_r );
READ_HANDLER( taitosj_68705_portC_r );
WRITE_HANDLER( taitosj_68705_portA_w );
WRITE_HANDLER( taitosj_68705_portB_w );

WRITE_HANDLER( alpine_protection_w );
WRITE_HANDLER( alpinea_bankswitch_w );
READ_HANDLER( alpine_port_2_r );
READ_HANDLER( spacecr_prot_r );

extern unsigned char *taitosj_videoram2,*taitosj_videoram3;
extern unsigned char *taitosj_characterram;
extern unsigned char *taitosj_scroll;
extern unsigned char *taitosj_colscrolly;
extern unsigned char *taitosj_gfxpointer;
extern unsigned char *taitosj_colorbank,*taitosj_video_priority;
extern unsigned char *kikstart_scrollram;
PALETTE_INIT( taitosj );
READ_HANDLER( taitosj_gfxrom_r );
WRITE_HANDLER( taitosj_videoram2_w );
WRITE_HANDLER( taitosj_videoram3_w );
WRITE_HANDLER( taitosj_paletteram_w );
WRITE_HANDLER( taitosj_colorbank_w );
WRITE_HANDLER( taitosj_videoenable_w );
WRITE_HANDLER( taitosj_characterram_w );
WRITE_HANDLER( junglhbr_characterram_w );
READ_HANDLER( taitosj_collision_reg_r );
WRITE_HANDLER( taitosj_collision_reg_clear_w );
VIDEO_START( taitosj );
VIDEO_UPDATE( taitosj );


static int sndnmi_disable = 1;

static WRITE_HANDLER( taitosj_sndnmi_msk_w )
{
	sndnmi_disable = data & 0x01;
}

static WRITE_HANDLER( taitosj_soundcommand_w )
{
	soundlatch_w(offset,data);
	if (!sndnmi_disable) cpu_set_irq_line(1,IRQ_LINE_NMI,PULSE_LINE);
}


static MEMORY_READ_START( readmem )
	{ 0x0000, 0x5fff, MRA_ROM },
	{ 0x6000, 0x7fff, MRA_BANK1 },
	{ 0x8000, 0x87ff, MRA_RAM },
	{ 0x8800, 0x8800, taitosj_fake_data_r },
	{ 0x8801, 0x8801, taitosj_fake_status_r },
	{ 0xc000, 0xcfff, MRA_RAM },
	{ 0xd000, 0xd05f, MRA_RAM },
	{ 0xd100, 0xd1ff, MRA_RAM },
	{ 0xd400, 0xd403, taitosj_collision_reg_r },
	{ 0xd404, 0xd404, taitosj_gfxrom_r },
	{ 0xd408, 0xd408, input_port_0_r },     /* IN0 */
	{ 0xd409, 0xd409, input_port_1_r },     /* IN1 */
	{ 0xd40a, 0xd40a, input_port_5_r },     /* DSW1 */
	{ 0xd40b, 0xd40b, input_port_2_r },     /* IN2 */
	{ 0xd40c, 0xd40c, input_port_3_r },     /* Service */
	{ 0xd40d, 0xd40d, input_port_4_r },
	{ 0xd40f, 0xd40f, AY8910_read_port_0_r },       /* DSW2 and DSW3 */
	{ 0xe000, 0xefff, MRA_ROM },
MEMORY_END

static MEMORY_WRITE_START( writemem )
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0x8000, 0x87ff, MWA_RAM },
	{ 0x8800, 0x8800, taitosj_fake_data_w },
	{ 0x9000, 0xbfff, taitosj_characterram_w, &taitosj_characterram },
	{ 0xc000, 0xc3ff, MWA_RAM },
	{ 0xc400, 0xc7ff, videoram_w, &videoram, &videoram_size },
	{ 0xc800, 0xcbff, taitosj_videoram2_w, &taitosj_videoram2 },
	{ 0xcc00, 0xcfff, taitosj_videoram3_w, &taitosj_videoram3 },
	{ 0xd000, 0xd05f, MWA_RAM, &taitosj_colscrolly },
	{ 0xd100, 0xd17f, MWA_RAM, &spriteram, &spriteram_size },
	{ 0xd180, 0xd1ff, MWA_RAM, &spriteram_2, &spriteram_2_size },
	{ 0xd200, 0xd27f, taitosj_paletteram_w, &paletteram },
	{ 0xd300, 0xd300, MWA_RAM, &taitosj_video_priority },
	{ 0xd40e, 0xd40e, AY8910_control_port_0_w },
	{ 0xd40f, 0xd40f, AY8910_write_port_0_w },
	{ 0xd500, 0xd505, MWA_RAM, &taitosj_scroll },
	{ 0xd506, 0xd507, taitosj_colorbank_w, &taitosj_colorbank },
	{ 0xd508, 0xd508, taitosj_collision_reg_clear_w },
	{ 0xd509, 0xd50a, MWA_RAM, &taitosj_gfxpointer },
	{ 0xd50b, 0xd50b, taitosj_soundcommand_w },
	{ 0xd50d, 0xd50d, MWA_RAM, /*watchdog_reset_w*/ },  /* Bio Attack reset sometimes after you die */
	{ 0xd50e, 0xd50e, taitosj_bankswitch_w },
	{ 0xd50f, 0xd50f, MWA_NOP },
	{ 0xd600, 0xd600, taitosj_videoenable_w },
	{ 0xe000, 0xefff, MWA_ROM },
MEMORY_END

/* only difference is taitosj_fake_ replaced with taitosj_mcu_ */
static MEMORY_READ_START( mcu_readmem )
	{ 0x0000, 0x5fff, MRA_ROM },
	{ 0x6000, 0x7fff, MRA_BANK1 },
	{ 0x8000, 0x87ff, MRA_RAM },
	{ 0x8800, 0x8800, taitosj_mcu_data_r },
	{ 0x8801, 0x8801, taitosj_mcu_status_r },
	{ 0xc000, 0xcfff, MRA_RAM },
	{ 0xd000, 0xd05f, MRA_RAM },
	{ 0xd100, 0xd1ff, MRA_RAM },
	{ 0xd400, 0xd403, taitosj_collision_reg_r },
	{ 0xd404, 0xd404, taitosj_gfxrom_r },
	{ 0xd408, 0xd408, input_port_0_r },     /* IN0 */
	{ 0xd409, 0xd409, input_port_1_r },     /* IN1 */
	{ 0xd40a, 0xd40a, input_port_5_r },     /* DSW1 */
	{ 0xd40b, 0xd40b, input_port_2_r },     /* IN2 */
	{ 0xd40c, 0xd40c, input_port_3_r },     /* Service */
	{ 0xd40d, 0xd40d, input_port_4_r },
	{ 0xd40f, 0xd40f, AY8910_read_port_0_r },       /* DSW2 and DSW3 */
	{ 0xe000, 0xefff, MRA_ROM },
MEMORY_END

static MEMORY_WRITE_START( mcu_writemem )
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0x8000, 0x87ff, MWA_RAM },
	{ 0x8800, 0x8800, taitosj_mcu_data_w },
	{ 0x9000, 0xbfff, taitosj_characterram_w, &taitosj_characterram },
	{ 0xc400, 0xc7ff, videoram_w, &videoram, &videoram_size },
	{ 0xc800, 0xcbff, taitosj_videoram2_w, &taitosj_videoram2 },
	{ 0xc000, 0xc3ff, MWA_RAM },
	{ 0xcc00, 0xcfff, taitosj_videoram3_w, &taitosj_videoram3 },
	{ 0xd000, 0xd05f, MWA_RAM, &taitosj_colscrolly },
	{ 0xd100, 0xd17f, MWA_RAM, &spriteram, &spriteram_size },
	{ 0xd180, 0xd1ff, MWA_RAM, &spriteram_2, &spriteram_2_size },
	{ 0xd200, 0xd27f, taitosj_paletteram_w, &paletteram },
	{ 0xd300, 0xd300, MWA_RAM, &taitosj_video_priority },
	{ 0xd40e, 0xd40e, AY8910_control_port_0_w },
	{ 0xd40f, 0xd40f, AY8910_write_port_0_w },
	{ 0xd500, 0xd505, MWA_RAM, &taitosj_scroll },
	{ 0xd506, 0xd507, taitosj_colorbank_w, &taitosj_colorbank },
	{ 0xd508, 0xd508, taitosj_collision_reg_clear_w },
	{ 0xd509, 0xd50a, MWA_RAM, &taitosj_gfxpointer },
	{ 0xd50b, 0xd50b, taitosj_soundcommand_w },
	{ 0xd50d, 0xd50d, watchdog_reset_w },
	{ 0xd50e, 0xd50e, taitosj_bankswitch_w },
	{ 0xd600, 0xd600, taitosj_videoenable_w },
	{ 0xe000, 0xefff, MWA_ROM },
MEMORY_END

/* seems the most logical way to do the gears */
static UINT8 kikstart_gearP1;
static UINT8 kikstart_gearP2;

static READ_HANDLER ( kikstart_gearsP1_read )
{
	/* gear MUST be 1, 2 or 3 */

	int portreturn = readinputport(3) & 0xf4;

	if (readinputport(8) & 0x01) kikstart_gearP1 = 1;
	if (readinputport(8) & 0x02) kikstart_gearP1 = 2;
	if (readinputport(8) & 0x04) kikstart_gearP1 = 3;

	if (kikstart_gearP1 == 1) portreturn |= (0x02);
	if (kikstart_gearP1 == 2) portreturn |= (0x03);
	if (kikstart_gearP1 == 3) portreturn |= (0x01);

/*usrintf_showmessage	("Kikstart gear %02x",  kikstart_gear); */

	return portreturn;
}

static READ_HANDLER ( kikstart_gearsP2_read )
{
	/* gear MUST be 1, 2 or 3 */

	int portreturn = readinputport(4) & 0xf4; /* correct i think */

	if (readinputport(9) & 0x08) kikstart_gearP2 = 1; /* port 9 for P2 correct.?? */
	if (readinputport(9) & 0x10) kikstart_gearP2 = 2;
	if (readinputport(9) & 0x20) kikstart_gearP2 = 3;

	if (kikstart_gearP2 == 1) portreturn |= (0x02);
	if (kikstart_gearP2 == 2) portreturn |= (0x03);
	if (kikstart_gearP2 == 3) portreturn |= (0x01);

/* usrintf_showmessage	("Kikstart gear %02x",  kikstart_gear); */

	return portreturn;
}

static MEMORY_READ_START( kikstart_readmem )
	{ 0x0000, 0x5fff, MRA_ROM },
	{ 0x6000, 0x7fff, MRA_BANK1 },
	{ 0x8000, 0x87ff, MRA_RAM },
	{ 0x8800, 0x8800, taitosj_mcu_data_r },
	{ 0x8801, 0x8801, taitosj_mcu_status_r },
	{ 0x8802, 0x8802, MRA_NOP },
	{ 0xc000, 0xd05f, MRA_RAM },
	{ 0xd100, 0xd1ff, MRA_RAM },
	{ 0xd400, 0xd403, taitosj_collision_reg_r },
	{ 0xd404, 0xd404, taitosj_gfxrom_r },
	{ 0xd408, 0xd408, input_port_0_r },     /* IN0 */
	{ 0xd409, 0xd409, input_port_1_r },     /* IN1 */
	{ 0xd40a, 0xd40a, input_port_5_r },     /* DSW1 */
	{ 0xd40b, 0xd40b, input_port_2_r },     /* IN2 */
	{ 0xd40c, 0xd40c, kikstart_gearsP1_read },      /* IN3 */
    { 0xd40d, 0xd40d, kikstart_gearsP2_read },		/* IN4 */
	{ 0xd40f, 0xd40f, AY8910_read_port_0_r },       /* DSW2 and DSW3 */
	{ 0xd800, 0xdfff, MRA_RAM },
	{ 0xe000, 0xefff, MRA_ROM },
MEMORY_END

static MEMORY_WRITE_START( kikstart_writemem )
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0x8000, 0x87ff, MWA_RAM },
	{ 0x8800, 0x8800, taitosj_mcu_data_w },
	{ 0x8802, 0x8802, MWA_NOP },
	{ 0x9000, 0xbfff, taitosj_characterram_w, &taitosj_characterram },
	{ 0xc000, 0xc3ff, MWA_RAM },
	{ 0xc400, 0xc7ff, videoram_w, &videoram, &videoram_size },
	{ 0xc800, 0xcbff, taitosj_videoram2_w, &taitosj_videoram2 },
	{ 0xcc00, 0xcfff, taitosj_videoram3_w, &taitosj_videoram3 },
	{ 0x8a00, 0x8a5f, MWA_RAM, &taitosj_colscrolly },
	{ 0xd100, 0xd17f, MWA_RAM, &spriteram, &spriteram_size },
	{ 0xd180, 0xd1ff, MWA_RAM, &spriteram_2, &spriteram_2_size },
	{ 0xd200, 0xd27f, taitosj_paletteram_w, &paletteram },
	{ 0xd300, 0xd300, MWA_RAM, &taitosj_video_priority },
	{ 0xd40e, 0xd40e, AY8910_control_port_0_w },
	{ 0xd40f, 0xd40f, AY8910_write_port_0_w },
	{ 0xd000, 0xd001, taitosj_colorbank_w, &taitosj_colorbank },
	{ 0xd002, 0xd007, MWA_RAM, &taitosj_scroll },
	{ 0xd508, 0xd508, taitosj_collision_reg_clear_w },
	{ 0xd509, 0xd50a, MWA_RAM, &taitosj_gfxpointer },
	{ 0xd50b, 0xd50b, taitosj_soundcommand_w },
	{ 0xd50d, 0xd50d, watchdog_reset_w },
	{ 0xd50e, 0xd50e, taitosj_bankswitch_w },
	{ 0xd600, 0xd600, taitosj_videoenable_w },
	{ 0xd800, 0xdfff, MWA_RAM,&kikstart_scrollram },/* scroll ram + ???*/
	{ 0xe000, 0xefff, MWA_ROM },
MEMORY_END

static MEMORY_READ_START( sound_readmem )
	{ 0x0000, 0x3fff, MRA_ROM },
	{ 0x4000, 0x43ff, MRA_RAM },
	{ 0x4801, 0x4801, AY8910_read_port_1_r },
	{ 0x4803, 0x4803, AY8910_read_port_2_r },
	{ 0x4805, 0x4805, AY8910_read_port_3_r },
	{ 0x5000, 0x5000, soundlatch_r },
	{ 0xe000, 0xefff, MRA_ROM },	/* space for diagnostic ROM */
MEMORY_END

static MEMORY_WRITE_START( sound_writemem )
	{ 0x0000, 0x3fff, MWA_ROM },
	{ 0x4000, 0x43ff, MWA_RAM },
	{ 0x4800, 0x4800, AY8910_control_port_1_w },
	{ 0x4801, 0x4801, AY8910_write_port_1_w },
	{ 0x4802, 0x4802, AY8910_control_port_2_w },
	{ 0x4803, 0x4803, AY8910_write_port_2_w },
	{ 0x4804, 0x4804, AY8910_control_port_3_w },
	{ 0x4805, 0x4805, AY8910_write_port_3_w },
	{ 0xe000, 0xefff, MWA_ROM },
MEMORY_END


static MEMORY_READ_START( m68705_readmem )
	{ 0x0000, 0x0000, taitosj_68705_portA_r },
	{ 0x0001, 0x0001, taitosj_68705_portB_r },
	{ 0x0002, 0x0002, taitosj_68705_portC_r },
	{ 0x0003, 0x007f, MRA_RAM },
	{ 0x0080, 0x07ff, MRA_ROM },
MEMORY_END

static MEMORY_WRITE_START( m68705_writemem )
	{ 0x0000, 0x0000, taitosj_68705_portA_w },
	{ 0x0001, 0x0001, taitosj_68705_portB_w },
	{ 0x0003, 0x007f, MWA_RAM },
	{ 0x0080, 0x07ff, MWA_ROM },
MEMORY_END



#define DSW2_PORT \
	PORT_DIPNAME( 0x0f, 0x00, DEF_STR( Coin_A ) ) \
	PORT_DIPSETTING(    0x0f, DEF_STR( 9C_1C ) ) \
	PORT_DIPSETTING(    0x0e, DEF_STR( 8C_1C ) ) \
	PORT_DIPSETTING(    0x0d, DEF_STR( 7C_1C ) ) \
	PORT_DIPSETTING(    0x0c, DEF_STR( 6C_1C ) ) \
	PORT_DIPSETTING(    0x0b, DEF_STR( 5C_1C ) ) \
	PORT_DIPSETTING(    0x0a, DEF_STR( 4C_1C ) ) \
	PORT_DIPSETTING(    0x09, DEF_STR( 3C_1C ) ) \
	PORT_DIPSETTING(    0x08, DEF_STR( 2C_1C ) ) \
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) ) \
	PORT_DIPSETTING(    0x01, DEF_STR( 1C_2C ) ) \
	PORT_DIPSETTING(    0x02, DEF_STR( 1C_3C ) ) \
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_4C ) ) \
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_5C ) ) \
	PORT_DIPSETTING(    0x05, DEF_STR( 1C_6C ) ) \
	PORT_DIPSETTING(    0x06, DEF_STR( 1C_7C ) ) \
	PORT_DIPSETTING(    0x07, DEF_STR( 1C_8C ) ) \
	PORT_DIPNAME( 0xf0, 0x00, DEF_STR( Coin_B ) ) \
	PORT_DIPSETTING(    0xf0, DEF_STR( 9C_1C ) ) \
	PORT_DIPSETTING(    0xe0, DEF_STR( 8C_1C ) ) \
	PORT_DIPSETTING(    0xd0, DEF_STR( 7C_1C ) ) \
	PORT_DIPSETTING(    0xc0, DEF_STR( 6C_1C ) ) \
	PORT_DIPSETTING(    0xb0, DEF_STR( 5C_1C ) ) \
	PORT_DIPSETTING(    0xa0, DEF_STR( 4C_1C ) ) \
	PORT_DIPSETTING(    0x90, DEF_STR( 3C_1C ) ) \
	PORT_DIPSETTING(    0x80, DEF_STR( 2C_1C ) ) \
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) ) \
	PORT_DIPSETTING(    0x10, DEF_STR( 1C_2C ) ) \
	PORT_DIPSETTING(    0x20, DEF_STR( 1C_3C ) ) \
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_4C ) ) \
	PORT_DIPSETTING(    0x40, DEF_STR( 1C_5C ) ) \
	PORT_DIPSETTING(    0x50, DEF_STR( 1C_6C ) ) \
	PORT_DIPSETTING(    0x60, DEF_STR( 1C_7C ) ) \
	PORT_DIPSETTING(    0x70, DEF_STR( 1C_8C ) )


INPUT_PORTS_START( spaceskr )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START2 )

	PORT_START      /* Service */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_COIN3 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_TILT )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )


	PORT_START
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* DSW1 */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x18, 0x18, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x08, "4" )
	PORT_DIPSETTING(    0x10, "5" )
	PORT_DIPSETTING(    0x18, "6" )
	PORT_SERVICE( 0x20, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Cocktail ) )

	PORT_START      /* DSW2 Coinage */
	DSW2_PORT

	PORT_START      /* DSW3 */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, "Year Display" )
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Yes ) )
	PORT_BITX(    0x40, 0x40, IPT_DIPSWITCH_NAME | IPF_CHEAT, "Invulnerability", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x80, "A and B" )
	PORT_DIPSETTING(    0x00, "A only" )
INPUT_PORTS_END

INPUT_PORTS_START( spacecr )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_2WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_2WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 ) /* continue */
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_2WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_2WAY | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL ) /* continue */
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START2 )

	PORT_START      /* Service */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_TILT )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )


	PORT_START

	PORT_START      /* DSW1 */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x18, 0x18, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "6" )
	PORT_DIPSETTING(    0x08, "5" )
	PORT_DIPSETTING(    0x10, "4" )
	PORT_DIPSETTING(    0x18, "3" )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x80, DEF_STR( On ) )

	PORT_START      /* DSW2 Coinage */
	DSW2_PORT

	PORT_START      /* DSW3 */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, "Year Display" )
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Yes ) )
	PORT_BITX(    0x40, 0x40, IPT_DIPSWITCH_NAME | IPF_CHEAT, "Invulnerability", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x80, "A and B" )
	PORT_DIPSETTING(    0x00, "A only" )
INPUT_PORTS_END


INPUT_PORTS_START( junglek )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START2 )

	PORT_START      /* Service */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_COIN3 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_TILT )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* DSW1 */
	PORT_DIPNAME( 0x03, 0x03, "Finish Bonus" )
	PORT_DIPSETTING(    0x03, "None" )
	PORT_DIPSETTING(    0x02, "Timer x1" )
	PORT_DIPSETTING(    0x01, "Timer x2" )
	PORT_DIPSETTING(    0x00, "Timer x3" )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x18, 0x18, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x18, "3" )
	PORT_DIPSETTING(    0x10, "4" )
	PORT_DIPSETTING(    0x08, "5" )
	PORT_DIPSETTING(    0x00, "6" )
	PORT_SERVICE( 0x20, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Cocktail ) )

	PORT_START      /* DSW2 Coinage */
	DSW2_PORT

	PORT_START      /* DSW3 */
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x02, "10000" )
	PORT_DIPSETTING(    0x01, "20000" )
	PORT_DIPSETTING(    0x00, "30000" )
	PORT_DIPSETTING(    0x03, "None" )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, "Year Display" )
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Yes ) )
	PORT_BITX(    0x40, 0x40, IPT_DIPSWITCH_NAME | IPF_CHEAT, "Infinite Lives", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(    0x40, DEF_STR( No ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x80, "A and B" )
	PORT_DIPSETTING(    0x00, "A only" )
INPUT_PORTS_END

INPUT_PORTS_START( piratpet )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )			/* Skip level when "Debug Mode" is ON*/
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL )	/* Skip level when "Debug Mode" is ON*/
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START2 )

	PORT_START      /* Service */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_COIN3 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_TILT )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* DSW1 */
	PORT_DIPNAME( 0x03, 0x03, "Finish Bonus" )
	PORT_DIPSETTING(    0x03, "None" )
	PORT_DIPSETTING(    0x02, "Timer x1" )
	PORT_DIPSETTING(    0x01, "Timer x2" )
	PORT_DIPSETTING(    0x00, "Timer x3" )
	PORT_DIPNAME( 0x04, 0x04, "Debug Mode" )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x18, 0x18, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x18, "3" )
	PORT_DIPSETTING(    0x10, "4" )
	PORT_DIPSETTING(    0x08, "5" )
	PORT_DIPSETTING(    0x00, "6" )
	PORT_SERVICE( 0x20, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Cocktail ) )

	PORT_START      /* DSW2 Coinage */
	DSW2_PORT

	PORT_START      /* DSW3 */
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x02, "10000" )
	PORT_DIPSETTING(    0x01, "20000" )
	PORT_DIPSETTING(    0x00, "50000" )
	PORT_DIPSETTING(    0x03, "None" )
	PORT_DIPNAME( 0x1c, 0x1c, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x1c, "Easiest" )
	PORT_DIPSETTING(    0x18, "Easier" )
	PORT_DIPSETTING(    0x14, "Easy" )
	PORT_DIPSETTING(    0x10, "Normal" )
	PORT_DIPSETTING(    0x0c, "Medium" )
	PORT_DIPSETTING(    0x08, "Hard" )
	PORT_DIPSETTING(    0x04, "Harder" )
	PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_DIPNAME( 0x20, 0x20, "Year Display" )
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Yes ) )
	PORT_BITX(    0x40, 0x40, IPT_DIPSWITCH_NAME | IPF_CHEAT, "Free Game", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(    0x40, DEF_STR( No ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x80, "A and B" )
	PORT_DIPSETTING(    0x00, "A only" )
INPUT_PORTS_END

INPUT_PORTS_START( alpine )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_2WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_2WAY | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )		/* "Fast"*/
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_2WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_2WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )				/* "Fast"*/
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT( 0x1e, 0x00, IPT_UNUSED )						/* protection read */
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START2 )

	PORT_START      /* Service */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_TILT )					/* flips screen */
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* DSW1 */
	PORT_DIPNAME( 0x03, 0x03, "Jump Bonus" )
	PORT_DIPSETTING(    0x00, "500-1500" )
	PORT_DIPSETTING(    0x01, "800-2000" )
	PORT_DIPSETTING(    0x02, "1000-2500" )
	PORT_DIPSETTING(    0x03, "2000-4000" )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_DIPNAME( 0x18, 0x18, "Time" )
	PORT_DIPSETTING(    0x00, "1:00" )
	PORT_DIPSETTING(    0x08, "1:30" )
	PORT_DIPSETTING(    0x10, "2:00" )
	PORT_DIPSETTING(    0x18, "2:30" )
	PORT_DIPNAME( 0x20, 0x00, "End of Race Time Bonus" )
	PORT_DIPSETTING(    0x20, "0:10" )
	PORT_DIPSETTING(    0x00, "0:20" )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Cocktail ) )

	PORT_START      /* DSW2 Coinage */
	DSW2_PORT

	PORT_START      /* DSW3 */
	PORT_DIPNAME( 0x03, 0x03, "1st Extended Time" )
	PORT_DIPSETTING(    0x00, "10k" )
	PORT_DIPSETTING(    0x01, "15k" )
	PORT_DIPSETTING(    0x02, "20k" )
	PORT_DIPSETTING(    0x03, "25k" )
	PORT_DIPNAME( 0x1c, 0x1c, "Extended Time Every" )
	PORT_DIPSETTING(    0x00, "5k" )
	PORT_DIPSETTING(    0x04, "6k" )
	PORT_DIPSETTING(    0x08, "7k" )
	PORT_DIPSETTING(    0x0c, "8k" )
	PORT_DIPSETTING(    0x10, "9k" )
	PORT_DIPSETTING(    0x14, "10k" )
	PORT_DIPSETTING(    0x18, "11k" )
	PORT_DIPSETTING(    0x1c, "12k" )
	PORT_DIPNAME( 0x20, 0x20, "Year Display" )
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Yes ) )
	PORT_BITX(    0x40, 0x40, IPT_DIPSWITCH_NAME | IPF_CHEAT, "Invulnerability", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x80, "A and B" )
	PORT_DIPSETTING(    0x00, "A only" )
INPUT_PORTS_END

INPUT_PORTS_START( alpinea )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_2WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_2WAY | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )		/* "Fast"*/
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_2WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_2WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )				/* "Fast"*/
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* IN2 */
	PORT_BIT( 0x0f, 0x00, IPT_UNUSED )						/* protection read */
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START2 )

	PORT_START      /* Service */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_TILT )               		/* flips screen */
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* DSW1 */
	PORT_DIPNAME( 0x03, 0x03, "Jump Bonus" )
	PORT_DIPSETTING(    0x00, "500-1500" )
	PORT_DIPSETTING(    0x01, "800-2000" )
	PORT_DIPSETTING(    0x02, "1000-2500" )
	PORT_DIPSETTING(    0x03, "2000-4000" )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_DIPNAME( 0x18, 0x18, "Time" )
	PORT_DIPSETTING(    0x00, "1:00" )
	PORT_DIPSETTING(    0x08, "1:30" )
	PORT_DIPSETTING(    0x10, "2:00" )
	PORT_DIPSETTING(    0x18, "2:30" )
	PORT_SERVICE( 0x20, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Cocktail ) )

	PORT_START      /* DSW2 Coinage */
	DSW2_PORT

	PORT_START      /* DSW3 */
	PORT_DIPNAME( 0x03, 0x03, "1st Extended Time" )
	PORT_DIPSETTING(    0x00, "10k" )
	PORT_DIPSETTING(    0x01, "15k" )
	PORT_DIPSETTING(    0x02, "20k" )
	PORT_DIPSETTING(    0x03, "25k" )
	PORT_DIPNAME( 0x1c, 0x1c, "Extended Time Every" )
	PORT_DIPSETTING(    0x00, "5k" )
	PORT_DIPSETTING(    0x04, "6k" )
	PORT_DIPSETTING(    0x08, "7k" )
	PORT_DIPSETTING(    0x0c, "8k" )
	PORT_DIPSETTING(    0x10, "9k" )
	PORT_DIPSETTING(    0x14, "10k" )
	PORT_DIPSETTING(    0x18, "11k" )
	PORT_DIPSETTING(    0x1c, "12k" )
	PORT_DIPNAME( 0x20, 0x20, "Year Display" )
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Yes ) )
	PORT_BITX(    0x40, 0x40, IPT_DIPSWITCH_NAME | IPF_CHEAT, "Invulnerability", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x80, "A and B" )
	PORT_DIPSETTING(    0x00, "A only" )
INPUT_PORTS_END

INPUT_PORTS_START( timetunl )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP   | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP   | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START2 )

	PORT_START      /* Service */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_TILT )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* DSW1 */
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Free_Play ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x18, 0x18, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x18, "3" )
	PORT_DIPSETTING(    0x10, "4" )
	PORT_DIPSETTING(    0x08, "5" )
	PORT_DIPSETTING(    0x00, "6" )
	PORT_DIPNAME( 0x20, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Cocktail ) )

	PORT_START      /* DSW2 Coinage */
	DSW2_PORT

	PORT_START      /* DSW3 */
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, "Coinage Display" )
	PORT_DIPSETTING(    0x10, "Coins/Credits" )
	PORT_DIPSETTING(    0x00, "Insert Coin" )
	PORT_DIPNAME( 0x20, 0x20, "Year Display" )
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Yes ) )
	PORT_BITX(    0x40, 0x40, IPT_DIPSWITCH_NAME | IPF_CHEAT, "Invulnerability", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x80, "A and B" )
	PORT_DIPSETTING(    0x00, "A only" )
INPUT_PORTS_END

INPUT_PORTS_START( wwestern )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICKRIGHT_LEFT  | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICKRIGHT_RIGHT | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICKRIGHT_DOWN  | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICKRIGHT_UP    | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICKRIGHT_LEFT  | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICKRIGHT_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICKRIGHT_DOWN  | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICKRIGHT_UP    | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x1c, 0x18, IPT_UNUSED )					/* protection read, the game resets after a while without it */
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START2 )

	PORT_START      /* Service */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_LEFT  | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_RIGHT | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_DOWN  | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_UP    | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_COIN3 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_TILT )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_LEFT  | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_DOWN  | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_UP    | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0xf0, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* DSW1 */
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x03, "10000" )
	PORT_DIPSETTING(    0x02, "30000" )
	PORT_DIPSETTING(    0x01, "50000" )
	PORT_DIPSETTING(    0x00, "70000" )
	PORT_DIPNAME( 0x04, 0x00, "High Score Table" )
	PORT_DIPSETTING(    0x04, DEF_STR( No ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x18, 0x18, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x18, "3" )
	PORT_DIPSETTING(    0x10, "4" )
	PORT_DIPSETTING(    0x08, "5" )
	PORT_DIPSETTING(    0x00, "6" )
	PORT_SERVICE( 0x20, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Cocktail ) )

	PORT_START      /* DSW2 Coinage */
	DSW2_PORT

	PORT_START      /* DSW3 */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x10, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_BITX(    0x40, 0x40, IPT_DIPSWITCH_NAME | IPF_CHEAT, "Invulnerability", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x80, "A and B" )
	PORT_DIPSETTING(    0x00, "A only" )
INPUT_PORTS_END

INPUT_PORTS_START( frontlin )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICKRIGHT_LEFT  | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICKRIGHT_RIGHT | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICKRIGHT_DOWN  | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICKRIGHT_UP    | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICKRIGHT_LEFT  | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICKRIGHT_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICKRIGHT_DOWN  | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICKRIGHT_UP    | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START2 )

	PORT_START      /* Service */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_LEFT  | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_RIGHT | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_DOWN  | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_UP    | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_TILT )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_LEFT  | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_DOWN  | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_UP    | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0xf0, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* DSW1 */
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x03, "10000" )
	PORT_DIPSETTING(    0x02, "20000" )
	PORT_DIPSETTING(    0x01, "30000" )
	PORT_DIPSETTING(    0x00, "50000" )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Free_Play ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x18, 0x18, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x18, "3" )
	PORT_DIPSETTING(    0x10, "4" )
	PORT_DIPSETTING(    0x08, "5" )
	PORT_DIPSETTING(    0x00, "6" )
	PORT_SERVICE( 0x20, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Cocktail ) )

	PORT_START      /* DSW2 Coinage */
	DSW2_PORT

	PORT_START      /* DSW3 */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, "Coinage Display" )
	PORT_DIPSETTING(    0x10, "Coins/Credits" )
	PORT_DIPSETTING(    0x00, "Insert Coin" )
	PORT_DIPNAME( 0x20, 0x20, "Year Display" )
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Yes ) )
	PORT_BITX(    0x40, 0x40, IPT_DIPSWITCH_NAME | IPF_CHEAT, "Invulnerability", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x80, "A and B" )
	PORT_DIPSETTING(    0x00, "A only" )
INPUT_PORTS_END

INPUT_PORTS_START( elevator )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_4WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_4WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_4WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START2 )

	PORT_START      /* Service */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_COIN3 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_TILT )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* DSW1 */
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x03, "10000" )
	PORT_DIPSETTING(    0x02, "15000" )
	PORT_DIPSETTING(    0x01, "20000" )
	PORT_DIPSETTING(    0x00, "25000" )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Free_Play ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x18, 0x18, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x18, "3" )
	PORT_DIPSETTING(    0x10, "4" )
	PORT_DIPSETTING(    0x08, "5" )
	PORT_DIPSETTING(    0x00, "6" )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Cocktail ) )

	PORT_START      /* DSW2 Coinage */
	DSW2_PORT

	PORT_START      /* DSW3 */
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x03, "Easiest" )
	PORT_DIPSETTING(    0x02, "Easy" )
	PORT_DIPSETTING(    0x01, "Normal" )
	PORT_DIPSETTING(    0x00, "Hard" )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, "Coinage Display" )
	PORT_DIPSETTING(    0x10, "Coins/Credits" )
	PORT_DIPSETTING(    0x00, "Insert Coin" )
	PORT_DIPNAME( 0x20, 0x20, "Year Display" )
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Yes ) )
	PORT_BITX(    0x40, 0x40, IPT_DIPSWITCH_NAME | IPF_CHEAT, "Invulnerability", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x80, "A and B" )
	PORT_DIPSETTING(    0x00, "A only" )
INPUT_PORTS_END

INPUT_PORTS_START( tinstar )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICKRIGHT_LEFT  | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICKRIGHT_RIGHT | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICKRIGHT_DOWN  | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICKRIGHT_UP    | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICKRIGHT_LEFT  | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICKRIGHT_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICKRIGHT_DOWN  | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICKRIGHT_UP    | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START2 )

	PORT_START      /* Service */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_LEFT  | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_RIGHT | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_DOWN  | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_UP    | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_TILT )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_LEFT  | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_DOWN  | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_UP    | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x30, 0x00, IPT_UNUSED )					/* protection read, the game hangs without it */
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* DSW1 */
	PORT_DIPNAME( 0x03, 0x03, "Bonus Life?" )
	PORT_DIPSETTING(    0x03, "10000?" )
	PORT_DIPSETTING(    0x02, "20000?" )
	PORT_DIPSETTING(    0x01, "30000?" )
	PORT_DIPSETTING(    0x00, "50000?" )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Free_Play ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x18, 0x10, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x18, "2" )
	PORT_DIPSETTING(    0x10, "3" )
	PORT_DIPSETTING(    0x08, "4" )
	PORT_DIPSETTING(    0x00, "5" )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Cocktail ) )

	PORT_START      /* DSW2 Coinage */
	DSW2_PORT

	PORT_START      /* DSW3 */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x00, "Star Sound" )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, "Coinage Display" )
	PORT_DIPSETTING(    0x10, "Coins/Credits" )
	PORT_DIPSETTING(    0x00, "Insert Coin" )
	PORT_DIPNAME( 0x20, 0x20, "Year Display" )
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Yes ) )
	PORT_BITX(    0x40, 0x40, IPT_DIPSWITCH_NAME | IPF_CHEAT, "Invulnerability", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x80, "A and B" )
	PORT_DIPSETTING(    0x00, "A only" )
INPUT_PORTS_END

INPUT_PORTS_START( waterski )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_2WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_2WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )				/* "Slow"*/
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )				/* "Jump"*/
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_2WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_2WAY | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )		/* "Slow"*/
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL )		/* "Jump"*/
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START2 )

	PORT_START      /* Service */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_COIN3 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_TILT )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* DSW1 */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Free_Play ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x18, 0x18, "Time" )
	PORT_DIPSETTING(    0x00, "2:00" )
	PORT_DIPSETTING(    0x08, "2:10" )
	PORT_DIPSETTING(    0x10, "2:20" )
	PORT_DIPSETTING(    0x18, "2:30" )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Cocktail ) )

	PORT_START      /* DSW2 Coinage */
	DSW2_PORT

	PORT_START      /* DSW3 */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, "Coinage Display" )
	PORT_DIPSETTING(    0x10, "Coins/Credits" )
	PORT_DIPSETTING(    0x00, "Insert Coin" )
	PORT_DIPNAME( 0x20, 0x20, "Year Display" )
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Yes ) )
	PORT_BITX(    0x40, 0x40, IPT_DIPSWITCH_NAME | IPF_CHEAT, "Invulnerability", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x80, "A and B" )
	PORT_DIPSETTING(    0x00, "A only" )
INPUT_PORTS_END

INPUT_PORTS_START( bioatack )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* IN2 */
	PORT_BIT( 0x0f, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START2 )

	PORT_START      /* Service */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_TILT )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* DSW1 d50a */
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x03, "5000" )
	PORT_DIPSETTING(    0x02, "10000" )
	PORT_DIPSETTING(    0x01, "15000" )
	PORT_DIPSETTING(    0x00, "20000" )
	PORT_DIPNAME( 0x04, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x18, 0x18, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x18, "3" )
	PORT_DIPSETTING(    0x10, "4" )
	PORT_DIPSETTING(    0x08, "5" )
	PORT_DIPSETTING(    0x00, "6" )
	PORT_DIPNAME( 0x20, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Cocktail ) )

	PORT_START      /* DSW2 Coinage */
	DSW2_PORT

	PORT_START      /* DSW3 */
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, "Year Display" )
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Yes ) )
	PORT_BITX(    0x40, 0x40, IPT_DIPSWITCH_NAME | IPF_CHEAT, "Invulnerability", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x80, "A and B" )
	PORT_DIPSETTING(    0x00, "A only" )
INPUT_PORTS_END

INPUT_PORTS_START( sfposeid )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START2 )

	PORT_START      /* Service */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_COIN3 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_TILT )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* DSW1 */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x18, 0x08, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "2" )
	PORT_DIPSETTING(    0x08, "3" )
	PORT_DIPSETTING(    0x10, "4" )
	PORT_DIPSETTING(    0x18, "5" )
	PORT_DIPNAME( 0x20, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Cocktail ) )

	PORT_START      /* DSW2 Coinage */
	DSW2_PORT

	PORT_START      /* DSW3 */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, "Coinage Display" )
	PORT_DIPSETTING(    0x10, "Coins/Credits" )
	PORT_DIPSETTING(    0x00, "Insert Coin" )
	PORT_DIPNAME( 0x20, 0x20, "Year Display" )
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Yes ) )
	PORT_BITX(    0x40, 0x40, IPT_DIPSWITCH_NAME | IPF_CHEAT, "Invulnerability", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x80, "A and B" )
	PORT_DIPSETTING(    0x00, "A only" )
INPUT_PORTS_END

INPUT_PORTS_START( hwrace )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START2 )

	PORT_START      /* Service */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_COIN3 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_TILT )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* DSW1 */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Free_Play ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_SERVICE( 0x20, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Cocktail ) )

	PORT_START      /* DSW2 Coinage */
	DSW2_PORT

	PORT_START      /* DSW3 */
	PORT_DIPNAME( 0x01, 0x03, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, "0" )
	PORT_DIPSETTING(    0x01, "1" )
	PORT_DIPSETTING(    0x02, "2" )
	PORT_DIPSETTING(    0x03, "3" )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, "Coinage Display" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x10, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, "Year Display" )
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Yes ) )
	PORT_BITX(    0x40, 0x40, IPT_DIPSWITCH_NAME | IPF_CHEAT, "Invulnerability", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x80, "A and B" )
	PORT_DIPSETTING(    0x00, "A only" )
INPUT_PORTS_END

INPUT_PORTS_START( kikstart )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_2WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_2WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_2WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_2WAY | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )

	PORT_START      /* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START2 )

	PORT_START      /* Service / IN3*/
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_SPECIAL ) /* gear */
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_SPECIAL ) /* gear */
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_SPECIAL ) /* gear */
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_SERVICE1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_TILT )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START    /* IN4.?? */ /* P2 gears here.?? */
    PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_SPECIAL ) /* P2 gear */
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_SPECIAL ) /* P2 gear */
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_SPECIAL ) /* P2 gear */

	PORT_START      /* DSW1 */
	PORT_DIPNAME(0x03, 0x01, "Gate Bonus" )
	PORT_DIPSETTING(   0x00, "5k Points" )
	PORT_DIPSETTING(   0x01, "10k Points" )
	PORT_DIPSETTING(   0x02, "15k Points" )
	PORT_DIPSETTING(   0x03, "20k Points" )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Free_Play ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME(0x18, 0x10, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(   0x18, "Easy" )		/* 3:00 */
	PORT_DIPSETTING(   0x10, "Normal" ) 		/* 2:30 */
	PORT_DIPSETTING(   0x08, "Difficult" )		/* 2:00 */
	PORT_DIPSETTING(   0x00, "Very Difficult" ) 	/* 1:30 */
	PORT_SERVICE( 0x20, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Cocktail ) )

	PORT_START      /* DSW2 Coinage */
	DSW2_PORT

	PORT_START      /* DSW3 */

	PORT_DIPNAME( 0x08, 0x08, "Control Type" )
	PORT_DIPSETTING(    0x08, "Revolve" )
	PORT_DIPSETTING(    0x00, "Buttons" )
	PORT_DIPNAME( 0x10, 0x10, "Coinage Display" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x10, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, "Year Display" )
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Yes ) )
	PORT_BITX(    0x40, 0x40, IPT_DIPSWITCH_NAME | IPF_CHEAT, "No Hit", IP_KEY_NONE, IP_JOY_NONE )/*?*/
	PORT_DIPSETTING(    0x40, DEF_STR( No ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x80, "A and B" )
	PORT_DIPSETTING(    0x00, "A only" )

	/* fake for handling the gears */
	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON2 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_BUTTON3 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_BUTTON4 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_BUTTON2 | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON3 | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON4 | IPF_COCKTAIL )
INPUT_PORTS_END



static struct GfxLayout charlayout =
{
	8,8,    /* 8*8 characters */
	256,    /* 256 characters */
	3,      /* 3 bits per pixel */
	{ 512*8*8, 256*8*8, 0 },        /* the bitplanes are separated */
	{ 7, 6, 5, 4, 3, 2, 1, 0 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8     /* every char takes 8 consecutive bytes */
};
static struct GfxLayout spritelayout =
{
	16,16,  /* 16*16 sprites */
	64,             /* 64 sprites */
	3,      /* 3 bits per pixel */
	{ 128*16*16, 64*16*16, 0 },     /* the bitplanes are separated */
	{ 7, 6, 5, 4, 3, 2, 1, 0,
		8*8+7, 8*8+6, 8*8+5, 8*8+4, 8*8+3, 8*8+2, 8*8+1, 8*8+0 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
			16*8, 17*8, 18*8, 19*8, 20*8, 21*8, 22*8, 23*8 },
	32*8    /* every sprite takes 32 consecutive bytes */
};



static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ 0, 0x9000, &charlayout,   0, 16 },    /* the game dynamically modifies this */
	{ 0, 0x9000, &spritelayout, 0, 16 },    /* the game dynamically modifies this */
	{ 0, 0xa800, &charlayout,   0, 16 },    /* the game dynamically modifies this */
	{ 0, 0xa800, &spritelayout, 0, 16 },    /* the game dynamically modifies this */
	{ -1 } /* end of array */
};



static struct AY8910interface ay8910_interface =
{
	4,      /* 4 chips */
	6000000/4,      /* 1.5 MHz */
	{ 15, 15, 15, MIXERG(15,MIXER_GAIN_2x,MIXER_PAN_CENTER) },
	{ input_port_6_r, 0, 0, 0 },            /* port Aread */
	{ input_port_7_r, 0, 0, 0 },            /* port Bread */
	{ 0, DAC_0_data_w, 0, 0 },              /* port Awrite */
	{ 0, 0, 0, taitosj_sndnmi_msk_w }       /* port Bwrite */
};

static struct DACinterface dac_interface =
{
	1,
	{ 15 }
};



static MACHINE_DRIVER_START( nomcu )

	/* basic machine hardware */
	MDRV_CPU_ADD_TAG("main",Z80,8000000/2)      /* 4 MHz */
	MDRV_CPU_MEMORY(readmem,writemem)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,1)

	MDRV_CPU_ADD(Z80,6000000/2)
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)      /* 3 MHz */
	MDRV_CPU_MEMORY(sound_readmem,sound_writemem)
			/* interrupts: */
			/* - no interrupts synced with vblank */
			/* - NMI triggered by the main CPU */
			/* - periodic IRQ, with frequency 6000000/(4*16*16*10*16) = 36.621 Hz, */
			/*   that is a period of 27306666.6666 ns */
	MDRV_CPU_PERIODIC_INT(irq0_line_hold,27306667)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)

	MDRV_MACHINE_INIT(taitosj)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 2*8, 30*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(64)
	MDRV_COLORTABLE_LENGTH(16*8)

	MDRV_PALETTE_INIT(taitosj)
	MDRV_VIDEO_START(taitosj)
	MDRV_VIDEO_UPDATE(taitosj)

	/* sound hardware */
	MDRV_SOUND_ADD(AY8910, ay8910_interface)
	MDRV_SOUND_ADD(DAC, dac_interface)
MACHINE_DRIVER_END


/* same as above, but with additional 68705 MCU */
static MACHINE_DRIVER_START( mcu )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(nomcu)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_MEMORY(mcu_readmem,mcu_writemem)

	MDRV_CPU_ADD(M68705,3000000/4)      /* xtal is 3MHz, divided by 4 internally */
	MDRV_CPU_MEMORY(m68705_readmem,m68705_writemem)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( kikstart )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(nomcu)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_MEMORY(kikstart_readmem,kikstart_writemem)

	MDRV_CPU_ADD(M68705,3000000/4)      /* xtal is 3MHz, divided by 4 internally */
	MDRV_CPU_MEMORY(m68705_readmem,m68705_writemem)
MACHINE_DRIVER_END


/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( spaceskr )
	ROM_REGION( 0x12000, REGION_CPU1, 0 )      /* 64k for code */
	ROM_LOAD( "eb01",         0x0000, 0x1000, CRC(92345b05) SHA1(c4e211c89185a9f9a0eeae87af0bc4eb4e0653e7) )
	ROM_LOAD( "eb02",         0x1000, 0x1000, CRC(a3e21420) SHA1(02b6a6a7626b89be9cc9ee6f4b7f0b94ad328c68) )
	ROM_LOAD( "eb03",         0x2000, 0x1000, CRC(a077c52f) SHA1(cb50e3c1082be54e2239efff4e9fc9160ad6aad8) )
	ROM_LOAD( "eb04",         0x3000, 0x1000, CRC(440030cf) SHA1(3e6a512137d81ca0400a6311961df36546f6f6e3) )
	ROM_LOAD( "eb05",         0x4000, 0x1000, CRC(b0d396ab) SHA1(42d3ad74d6065947f1d25c615fd3af171d952a80) )
	ROM_LOAD( "eb06",         0x5000, 0x1000, CRC(371d2f7a) SHA1(bd323063cc9cad30d4cdafa1a5be29c3153f6c7b) )
	ROM_LOAD( "eb07",         0x6000, 0x1000, CRC(13e667c4) SHA1(0b52eee7f8ed688c497bc60482b02a03f67807ce) )
	ROM_LOAD( "eb08",         0x7000, 0x1000, CRC(f2e84015) SHA1(e51450e3b173d3d6b60026e4d32307781db33c13) )
	/* 10000-11fff space for banked ROMs (not used) */

	ROM_REGION( 0x10000, REGION_CPU2, 0 )      /* 64k for the audio CPU */
	ROM_LOAD( "eb13",         0x0000, 0x1000, CRC(192f6536) SHA1(343d66a44568684ad44b7077fa8f378490fc9df4) )
	ROM_LOAD( "eb14",         0x1000, 0x1000, CRC(d04d0a21) SHA1(7fb66e6b4923329df4f28342e2923fc9c1d0bcc3) )
	ROM_LOAD( "eb15",         0x2000, 0x1000, CRC(88194305) SHA1(18d3e1b72a1eb64594bc9b89b8acb1dbafeb811b) )

	ROM_REGION( 0x8000, REGION_GFX1, 0 )       /* graphic ROMs used at runtime */
	ROM_LOAD( "eb09",         0x0000, 0x1000, CRC(77af540e) SHA1(819463bcc8d808806e4294f72e20ac528f9691b3) )
	ROM_LOAD( "eb10",         0x1000, 0x1000, CRC(b10073de) SHA1(1839f49a11b3afa7b1d73f53e8b706490d687f78) )
	ROM_LOAD( "eb11",         0x2000, 0x1000, CRC(c7954bd1) SHA1(c4bcaeb4b9d3aa50e4b41a8d1913b248bae2bd02) )
	ROM_LOAD( "eb12",         0x3000, 0x1000, CRC(cd6c087b) SHA1(92aaa277381937f5d5a5708e15b71023d8e9c545) )

	ROM_REGION( 0x0100, REGION_PROMS, 0 )      /* layer PROM */
	ROM_LOAD( "eb16.22",      0x0000, 0x0100, CRC(b833b5ea) SHA1(d233f1bf8a3e6cd876853ffd721b9b64c61c9047) )
ROM_END

ROM_START( spacecr )
	ROM_REGION( 0x12000, REGION_CPU1, 0 )      /* 64k for code */
	ROM_LOAD( "cg01.69",         0x0000, 0x1000, CRC(2fe28b71) SHA1(868b109c16fb5ebee576b90392c6ebfec37d4139) )
	ROM_LOAD( "cg02.68",         0x1000, 0x1000, CRC(88f4f856) SHA1(f077c42e7ac865875355bcf76483fbab3d66eb38) )
	ROM_LOAD( "cg03.67",         0x2000, 0x1000, CRC(2223319c) SHA1(4a5147473a11cb8da56de9e835dacb5b3ce9b084) )
	ROM_LOAD( "cg04.66",         0x3000, 0x1000, CRC(4daeb8b5) SHA1(646e2d819d2727395d13a38a3560e5a71db700fa) )
	ROM_LOAD( "cg05.65",         0x4000, 0x1000, CRC(cdc40ca0) SHA1(f79aa1a778190ee6e30d0b78643286cbf64dca45) )
	ROM_LOAD( "cg06.64",         0x5000, 0x1000, CRC(2cc6b4c0) SHA1(e3f74fb480c265f75d0e49cd60d7cfc6e1e37eb4) )
	ROM_LOAD( "cg07.55",         0x6000, 0x1000, CRC(e4c8780a) SHA1(49d6f3d875a83584514bea8a0f6cff175f5030f5) )
	ROM_LOAD( "cg08.54",         0x7000, 0x1000, CRC(2c23ff4d) SHA1(8dcbd394e241587401db4199ac58138b1142a07a) )
	ROM_LOAD( "cg09.53",         0x10000, 0x1000, CRC(3c8bb95e) SHA1(0bd2bedb7ce2176943fdb0cd640549f09b8807fa) ) /* banked at 6000 */
	ROM_LOAD( "cg10.52",         0x11000, 0x1000, CRC(0ff17fce) SHA1(e567754b2b55489ab63ebafc5ad0cc3853d9c8a1) ) /* banked at 7000 */

	ROM_REGION( 0x10000, REGION_CPU2, 0 )      /* 64k for the audio CPU */
	ROM_LOAD( "cg17.70",         0x0000, 0x1000, CRC(53486204) SHA1(36d373a5bfc5cf4fda84eb8011177737ee04acdd) )
	ROM_LOAD( "cg18.71",         0x1000, 0x1000, CRC(d1acf96c) SHA1(117a6ed2b5039bf072fb1ee59c5307ec9a2883b3) )
	ROM_LOAD( "cg19.72",         0x2000, 0x1000, CRC(ffd27215) SHA1(c0fb2dcfcc62e694ed11bd116b992859e10de55a) )

	ROM_REGION( 0x8000, REGION_GFX1, 0 )       /* graphic ROMs used at runtime */
	ROM_LOAD( "cg11.1",         0x0000, 0x1000, CRC(1e4ae527) SHA1(cac5a16a86c53e5e85a4ce323554bb0d6173622d) )
	ROM_LOAD( "cg12.2",         0x1000, 0x1000, CRC(aa57b616) SHA1(a004df9631dd13f083886cc76652166ee8c6da2c) )
	ROM_LOAD( "cg13.3",         0x2000, 0x1000, CRC(945a1b69) SHA1(6e87748733ee95c2ec360e7fb3c24059ddd72468) )
	ROM_LOAD( "cg14.4",         0x3000, 0x1000, CRC(1a29d06b) SHA1(76e866cf160bcbc353dec1d30d636c3f2f1b0ffe) )
	ROM_LOAD( "cg15.5",         0x4000, 0x1000, CRC(656f9713) SHA1(0f1bf28d9dfa50a3098820cfbd5271bc1cdc987b) )
	ROM_LOAD( "cg16.6",         0x5000, 0x1000, CRC(e2c0d585) SHA1(9ff848758b5b41b59eb4a48191c37d018810017e) )

	ROM_REGION( 0x0100, REGION_PROMS, 0 )      /* layer PROM */
	ROM_LOAD( "eb16.22",      0x0000, 0x0100, CRC(b833b5ea) SHA1(d233f1bf8a3e6cd876853ffd721b9b64c61c9047) )
ROM_END


ROM_START( junglek )
    ROM_REGION( 0x12000, REGION_CPU1, 0 ) /* 64k for code */
    ROM_LOAD( "kn21-1.bin",   0x00000, 0x1000, CRC(45f55d30) SHA1(bb9518d7728938f673a663801e47ae0438cdbea1) )
    ROM_LOAD( "kn22-1.bin",   0x01000, 0x1000, CRC(07cc9a21) SHA1(3fe35935e0a430ab0edc6a762623972fa37ea926) )
    ROM_LOAD( "kn43.bin",     0x02000, 0x1000, CRC(a20e5a48) SHA1(af961b671dc4c865d0181d08a70b902bb96f29d0) )
    ROM_LOAD( "kn24.bin",     0x03000, 0x1000, CRC(19ea7f83) SHA1(2399cc89f73811575c3f644d5c04ef13ceec6838) )
    ROM_LOAD( "kn25.bin",     0x04000, 0x1000, CRC(844365ea) SHA1(af34712620e4b784a5014283d3111048c5f81a56) )
    ROM_LOAD( "kn46.bin",     0x05000, 0x1000, CRC(27a95fd5) SHA1(160ee5d11126ac4155b479e43ec1bd6a4e9e21e7) )
    ROM_LOAD( "kn47.bin",     0x06000, 0x1000, CRC(5c3199e0) SHA1(c57dec92998b971d76aecd23674c25cf7b8be667) )
    ROM_LOAD( "kn28.bin",     0x07000, 0x1000, CRC(194a2d09) SHA1(88999493e470acdcf932efff71cd6155387a63d0) )
    /* 10000-10fff space for another banked ROM (not used) */
    ROM_LOAD( "kn60.bin",     0x11000, 0x1000, CRC(1a9c0a26) SHA1(82f4cebeba90419e83a00427b671985824babd7a) ) /* banked at 7000 */

    ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* 64k for the audio CPU */
    ROM_LOAD( "kn37.bin",     0x0000, 0x1000, CRC(dee7f5d4) SHA1(cd8179a17ccd054fb470c4eee97192c2dd226397) )
    ROM_LOAD( "kn38.bin",     0x1000, 0x1000, CRC(bffd3d21) SHA1(a2b3393e9694d6979d39ab0f1ab82b7ef892b3da) )
    ROM_LOAD( "kn59-1.bin",   0x2000, 0x1000, CRC(cee485fc) SHA1(1e0c52ec6b1d3cfd47247db71bcf3fe476c32039) )

    ROM_REGION( 0x8000, REGION_GFX1, 0 )   /* graphic ROMs used at runtime */
	ROM_LOAD( "kn29.bin",     0x0000, 0x1000, CRC(8f83c290) SHA1(aa95ed2d2e15f573e092e8eed7d80479512d9409) )
	ROM_LOAD( "kn30.bin",     0x1000, 0x1000, CRC(89fd19f1) SHA1(fc7dfe3a1d78ac37a036fa9d8ebf3a33a2f4cbe8) )
	ROM_LOAD( "kn51.bin",     0x2000, 0x1000, CRC(70e8fc12) SHA1(505c90c662d372d28cb38201433054b8e3d723d1) )
	ROM_LOAD( "kn52.bin",     0x3000, 0x1000, CRC(bcbac1a3) SHA1(bcd5fc9b3791ab67e0ad9f9ced7226853e9a2a00) )
	ROM_LOAD( "kn53.bin",     0x4000, 0x1000, CRC(b946c87d) SHA1(d16cb6bf38e00ae11c204cbf8f400f8a85c807c2) )
	ROM_LOAD( "kn34.bin",     0x5000, 0x1000, CRC(320db2e1) SHA1(ca8722010712302b491eb5f51d73043bcb2ddc8f) )
	ROM_LOAD( "kn55.bin",     0x6000, 0x1000, CRC(70aef58f) SHA1(df7454a1c3676181eca698bb3b2ef3253a45ca0f) )
	ROM_LOAD( "kn56.bin",     0x7000, 0x1000, CRC(932eb667) SHA1(4bf7c01ab212b616931a21a43a453521aa01ff36) )

	ROM_REGION( 0x0100, REGION_PROMS, 0 )      /* layer PROM */
    ROM_LOAD( "eb16.22",      0x0000, 0x0100, CRC(b833b5ea) SHA1(d233f1bf8a3e6cd876853ffd721b9b64c61c9047) )
ROM_END

ROM_START( junglkj2 )
	ROM_REGION( 0x12000, REGION_CPU1, 0 )      /* 64k for code */
	ROM_LOAD( "kn41.bin",     0x00000, 0x1000, CRC(7e4cd631) SHA1(512c08795d7946500b22d6f63a482c5156e6764b) )
	ROM_LOAD( "kn42.bin",     0x01000, 0x1000, CRC(bade53af) SHA1(c3d2cf776598cb2d8684fa0b3ea7af90af9e8dae) )
	ROM_LOAD( "kn43.bin",     0x02000, 0x1000, CRC(a20e5a48) SHA1(af961b671dc4c865d0181d08a70b902bb96f29d0) )
	ROM_LOAD( "kn44.bin",     0x03000, 0x1000, CRC(44c770d3) SHA1(57a1ddc07009f0dbd423cbe111b886e919a8bb0a) )
	ROM_LOAD( "kn45.bin",     0x04000, 0x1000, CRC(f60a3d06) SHA1(7c387f0aeb9497b026d8838ee6ea7ff11dea506a) )
	ROM_LOAD( "kn46.bin",     0x05000, 0x1000, CRC(27a95fd5) SHA1(160ee5d11126ac4155b479e43ec1bd6a4e9e21e7) )
	ROM_LOAD( "kn47.bin",     0x06000, 0x1000, CRC(5c3199e0) SHA1(c57dec92998b971d76aecd23674c25cf7b8be667) )
	ROM_LOAD( "kn48.bin",     0x07000, 0x1000, CRC(e690b36e) SHA1(25a6c06d6c2bf0082cc776255448c329cb2e74e0) )
	/* 10000-10fff space for another banked ROM (not used) */
	ROM_LOAD( "kn60.bin",     0x11000, 0x1000, CRC(1a9c0a26) SHA1(82f4cebeba90419e83a00427b671985824babd7a) ) /* banked at 7000 */

	ROM_REGION( 0x10000, REGION_CPU2, 0 )      /* 64k for the audio CPU */
	ROM_LOAD( "kn57-1.bin",   0x0000, 0x1000, CRC(62f6763a) SHA1(84eadbc5c6a37c53c104e4ac1fd273b6b2a335e5) )
	ROM_LOAD( "kn58-1.bin",   0x1000, 0x1000, CRC(9ef46c7f) SHA1(867d9352cde4d6496f59e790cbbf15302a55364e) )
	ROM_LOAD( "kn59-1.bin",   0x2000, 0x1000, CRC(cee485fc) SHA1(1e0c52ec6b1d3cfd47247db71bcf3fe476c32039) )

	ROM_REGION( 0x8000, REGION_GFX1, 0 )       /* graphic ROMs used at runtime */
	ROM_LOAD( "kn49.bin",     0x0000, 0x1000, CRC(fe275213) SHA1(5fcbe2db9371ae46610e7ce261498f3a9b4116ec) )
	ROM_LOAD( "kn50.bin",     0x1000, 0x1000, CRC(d9f93c55) SHA1(de04845a42b8214eceda1c9aa92af631f3236ee9) )
	ROM_LOAD( "kn51.bin",     0x2000, 0x1000, CRC(70e8fc12) SHA1(505c90c662d372d28cb38201433054b8e3d723d1) )
	ROM_LOAD( "kn52.bin",     0x3000, 0x1000, CRC(bcbac1a3) SHA1(bcd5fc9b3791ab67e0ad9f9ced7226853e9a2a00) )
	ROM_LOAD( "kn53.bin",     0x4000, 0x1000, CRC(b946c87d) SHA1(d16cb6bf38e00ae11c204cbf8f400f8a85c807c2) )
	ROM_LOAD( "kn54.bin",     0x5000, 0x1000, CRC(f757d8f0) SHA1(896118d990e3733aeb45842c0dc2103cbf2ba1a2) )
	ROM_LOAD( "kn55.bin",     0x6000, 0x1000, CRC(70aef58f) SHA1(df7454a1c3676181eca698bb3b2ef3253a45ca0f) )
	ROM_LOAD( "kn56.bin",     0x7000, 0x1000, CRC(932eb667) SHA1(4bf7c01ab212b616931a21a43a453521aa01ff36) )

	ROM_REGION( 0x0100, REGION_PROMS, 0 )      /* layer PROM */
	ROM_LOAD( "eb16.22",      0x0000, 0x0100, CRC(b833b5ea) SHA1(d233f1bf8a3e6cd876853ffd721b9b64c61c9047) )
ROM_END

ROM_START( jungleh )
	ROM_REGION( 0x12000, REGION_CPU1, 0 )      /* 64k for code */
	ROM_LOAD( "kn41a",        0x00000, 0x1000, CRC(6bf118d8) SHA1(d6de28766aab90b5dbca7f74612ec8eafd144348) )
	ROM_LOAD( "kn42.bin",     0x01000, 0x1000, CRC(bade53af) SHA1(c3d2cf776598cb2d8684fa0b3ea7af90af9e8dae) )
	ROM_LOAD( "kn43.bin",     0x02000, 0x1000, CRC(a20e5a48) SHA1(af961b671dc4c865d0181d08a70b902bb96f29d0) )
	ROM_LOAD( "kn44.bin",     0x03000, 0x1000, CRC(44c770d3) SHA1(57a1ddc07009f0dbd423cbe111b886e919a8bb0a) )
	ROM_LOAD( "kn45.bin",     0x04000, 0x1000, CRC(f60a3d06) SHA1(7c387f0aeb9497b026d8838ee6ea7ff11dea506a) )
	ROM_LOAD( "kn46a",        0x05000, 0x1000, CRC(ac89c155) SHA1(bac17c9828002b644f15933149a205a008a561d3) )
	ROM_LOAD( "kn47.bin",     0x06000, 0x1000, CRC(5c3199e0) SHA1(c57dec92998b971d76aecd23674c25cf7b8be667) )
	ROM_LOAD( "kn48a",        0x07000, 0x1000, CRC(ef80e931) SHA1(b3ddcc37860a2693d45a85970926662cbb96bd0e) )
	/* 10000-10fff space for another banked ROM (not used) */
	ROM_LOAD( "kn60.bin",     0x11000, 0x1000, CRC(1a9c0a26) SHA1(82f4cebeba90419e83a00427b671985824babd7a) ) /* banked at 7000 */

	ROM_REGION( 0x10000, REGION_CPU2, 0 )      /* 64k for the audio CPU */
	ROM_LOAD( "kn57-1.bin",   0x0000, 0x1000, CRC(62f6763a) SHA1(84eadbc5c6a37c53c104e4ac1fd273b6b2a335e5) )
	ROM_LOAD( "kn58-1.bin",   0x1000, 0x1000, CRC(9ef46c7f) SHA1(867d9352cde4d6496f59e790cbbf15302a55364e) )
	ROM_LOAD( "kn59-1.bin",   0x2000, 0x1000, CRC(cee485fc) SHA1(1e0c52ec6b1d3cfd47247db71bcf3fe476c32039) )

	ROM_REGION( 0x8000, REGION_GFX1, 0 )       /* graphic ROMs used at runtime */
	ROM_LOAD( "kn49a",        0x0000, 0x1000, CRC(b139e792) SHA1(10c39abc49786154396c00bd35a51b826e5f6bd0) )
	ROM_LOAD( "kn50a",        0x1000, 0x1000, CRC(1046019f) SHA1(b2d3ab8a53ef3ca55165a5bda9be0829f53be6c9) )
	ROM_LOAD( "kn51a",        0x2000, 0x1000, CRC(da50c8a4) SHA1(de5f9b953f277986679ab958772571d8417a0ce2) )
	ROM_LOAD( "kn52a",        0x3000, 0x1000, CRC(0444f06c) SHA1(80569807ae36b4c5ad90e9e736ce9d0d0ea486ec) )
	ROM_LOAD( "kn53a",        0x4000, 0x1000, CRC(6a17803e) SHA1(d7ab6a240bb1ac80d3903cb694e55fd6d3670faa) )
	ROM_LOAD( "kn54a",        0x5000, 0x1000, CRC(d41428c7) SHA1(8c926db731073313daced31a168da6ac07a6d5cb) )
	ROM_LOAD( "kn55.bin",     0x6000, 0x1000, CRC(70aef58f) SHA1(df7454a1c3676181eca698bb3b2ef3253a45ca0f) )
	ROM_LOAD( "kn56a",        0x7000, 0x1000, CRC(679c1101) SHA1(218cd75f77c858c3714a8f03aea2c7ee88a212dd) )

	ROM_REGION( 0x0100, REGION_PROMS, 0 )      /* layer PROM */
	ROM_LOAD( "eb16.22",      0x0000, 0x0100, CRC(b833b5ea) SHA1(d233f1bf8a3e6cd876853ffd721b9b64c61c9047) )
ROM_END

ROM_START( junglhbr )
	ROM_REGION( 0x12000, REGION_CPU1, 0 ) /* 64k for code */
	ROM_LOAD( "ic1.bin",      0x00000, 0x2000, CRC(3255a10e) SHA1(846448151e7db84b66ab6778c86c0b1bf8c3fec7) )
	ROM_LOAD( "ic2.bin",      0x02000, 0x2000, CRC(8482bc63) SHA1(56ddfc4df4867d81ad78c23fc80f53ff711dffd6) )
	ROM_LOAD( "ic3.bin",      0x04000, 0x2000, CRC(1abc661d) SHA1(58e63ac49de004e960e66fab261f405c96f1e758) )
	ROM_LOAD( "kn47.bin",     0x06000, 0x1000, CRC(5c3199e0) SHA1(c57dec92998b971d76aecd23674c25cf7b8be667) )
	ROM_LOAD( "kn48a",        0x07000, 0x1000, CRC(ef80e931) SHA1(b3ddcc37860a2693d45a85970926662cbb96bd0e) )
	/* 10000-10fff space for another banked ROM (not used) */
	ROM_LOAD( "kn60.bin",     0x11000, 0x1000, CRC(1a9c0a26) SHA1(82f4cebeba90419e83a00427b671985824babd7a) ) /* banked at 7000 */

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* 64k for the audio CPU */
	ROM_LOAD( "kn37.bin",     0x0000, 0x1000, CRC(dee7f5d4) SHA1(cd8179a17ccd054fb470c4eee97192c2dd226397) )
	ROM_LOAD( "kn38.bin",     0x1000, 0x1000, CRC(bffd3d21) SHA1(a2b3393e9694d6979d39ab0f1ab82b7ef892b3da) )
	ROM_LOAD( "kn59-1.bin",   0x2000, 0x1000, CRC(cee485fc) SHA1(1e0c52ec6b1d3cfd47247db71bcf3fe476c32039) )

	ROM_REGION( 0x8000, REGION_GFX1, 0 )   /* graphic ROMs used at runtime */
	ROM_LOAD( "kn29.bin",     0x0000, 0x1000, CRC(8f83c290) SHA1(aa95ed2d2e15f573e092e8eed7d80479512d9409) )
	ROM_LOAD( "kn30.bin",     0x1000, 0x1000, CRC(89fd19f1) SHA1(fc7dfe3a1d78ac37a036fa9d8ebf3a33a2f4cbe8) )
	ROM_LOAD( "kn51.bin",     0x2000, 0x1000, CRC(70e8fc12) SHA1(505c90c662d372d28cb38201433054b8e3d723d1) )
	ROM_LOAD( "kn52.bin",     0x3000, 0x1000, CRC(bcbac1a3) SHA1(bcd5fc9b3791ab67e0ad9f9ced7226853e9a2a00) )
	ROM_LOAD( "kn53.bin",     0x4000, 0x1000, CRC(b946c87d) SHA1(d16cb6bf38e00ae11c204cbf8f400f8a85c807c2) )
	ROM_LOAD( "kn34.bin",     0x5000, 0x1000, CRC(320db2e1) SHA1(ca8722010712302b491eb5f51d73043bcb2ddc8f) )
	ROM_LOAD( "kn55.bin",     0x6000, 0x1000, CRC(70aef58f) SHA1(df7454a1c3676181eca698bb3b2ef3253a45ca0f) )
	ROM_LOAD( "kn56.bin",     0x7000, 0x1000, CRC(932eb667) SHA1(4bf7c01ab212b616931a21a43a453521aa01ff36) )

	ROM_REGION( 0x0100, REGION_PROMS, 0 )      /* layer PROM */
	ROM_LOAD( "eb16.22",      0x0000, 0x0100, CRC(b833b5ea) SHA1(d233f1bf8a3e6cd876853ffd721b9b64c61c9047) )
ROM_END

ROM_START( piratpet )
	ROM_REGION( 0x12000, REGION_CPU1, 0 ) /* 64k for code */
	ROM_LOAD( "pp0p_ic.69", 0x00000, 0x1000, CRC(8287dbc2) SHA1(bdaf6b875b91739d730675bd140288697dc13dc8) )
	ROM_LOAD( "pp1p_ic.68", 0x01000, 0x1000, CRC(27a90850) SHA1(8ba69ed4ebbb513ff8fc0c3e8f0835debb62f7ba) )
	ROM_LOAD( "pp2p_ic.67", 0x02000, 0x1000, CRC(d224fa85) SHA1(8867ee33c22e432632b7a709b721f7c07e26e001) )
	ROM_LOAD( "pp3p_ic.66", 0x03000, 0x1000, CRC(2c900874) SHA1(9505c7a7a2607144de5918525b06b36caa248f91) )
	ROM_LOAD( "pp4p_ic.65", 0x04000, 0x1000, CRC(1aed98d9) SHA1(0158952fa75d3c3c65d6efd2b9854802687d9377) )
	ROM_LOAD( "pp5p_ic.64", 0x05000, 0x1000, CRC(09c3aacd) SHA1(4dd1e4cad13b03f87fca041d41c8a4700560dfb8) )
	ROM_LOAD( "pp6p_ic.55", 0x06000, 0x1000, CRC(bdeed702) SHA1(65f82021ef15b3ba2e80321d688b5a50cce9e8d5) )
	ROM_LOAD( "pp7p_ic.54", 0x07000, 0x1000, CRC(5f36d082) SHA1(f9930197bcd36de69b7c99d50d1a0c4914ca3090) )
	/* 10000-10fff space for another banked ROM (not used) */
	ROM_LOAD( "pp7b_ic.52", 0x11000, 0x1000, CRC(bbc38b03) SHA1(1fac52ae6eb1f9874d11dcfaf69fc5cf3964979c) ) /* banked at 7000 */

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* 64k for the audio CPU */
	ROM_LOAD( "pp05_ic.70", 0x0000, 0x1000, CRC(dcb5eb9d) SHA1(79e01c12475ea3326efa446e7d2a64070f7d268e) )
	ROM_LOAD( "pp15_ic.71", 0x1000, 0x1000, CRC(3123dbe1) SHA1(0581be775d29fdbdbd8535a632dfa3f7d49c9d7d) )

	ROM_REGION( 0x8000, REGION_GFX1, 0 ) /* graphic ROMs used at runtime */
	ROM_LOAD( "pp0e_ic.1", 0x0000, 0x1000, CRC(aceaf79b) SHA1(ef4c626a8d4e884e7d1600d69f36000316b1f213) )
	ROM_LOAD( "pp1e_ic.2", 0x1000, 0x1000, CRC(ac148214) SHA1(947e5795d9abd490b6519da33648a1f7385dc080) )
	ROM_LOAD( "pp2e_ic.3", 0x2000, 0x1000, CRC(108194d2) SHA1(49ddcaa83da7ae5658ce253a1cec9da864c5070c) )
	ROM_LOAD( "pp3e_ic.4", 0x3000, 0x1000, CRC(621b0da1) SHA1(eaec9f57031708c89e8f0ebe98973f793b7636fd) )
	ROM_LOAD( "pp4e_ic.5", 0x4000, 0x1000, CRC(e9826d90) SHA1(644ba76bb2860e4d5b20d2df2b9e5364b05db362) )
	ROM_LOAD( "pp5e_ic.6", 0x5000, 0x1000, CRC(fe0d38c6) SHA1(7ed0a8d800c4e079807cc0744832a4032f129e22) )
	ROM_LOAD( "pp6e_ic.7", 0x6000, 0x1000, CRC(2cfd127b) SHA1(bc353ac84342a11148d46a20281d9f17c0ee8903) )
	ROM_LOAD( "pp7e_ic.8", 0x7000, 0x1000, CRC(9857533f) SHA1(7682672fe651c2d54157f60a55d17bb79953d7ae) )

	ROM_REGION( 0x0100, REGION_PROMS, 0 ) /* layer PROM */
	ROM_LOAD( "eb16.22", 0x0000, 0x0100, CRC(b833b5ea) SHA1(d233f1bf8a3e6cd876853ffd721b9b64c61c9047) )
ROM_END

ROM_START( alpine )
	ROM_REGION( 0x12000, REGION_CPU1, 0 )      /* 64k for code */
	ROM_LOAD( "rh16.069",     0x0000, 0x1000, CRC(6b2a69b7) SHA1(d1904eac06f2ee0c491c2da04ec6191eb1ddca69) )
	ROM_LOAD( "rh17.068",     0x1000, 0x1000, CRC(e344b0b7) SHA1(a4f9c2b61d0d73c30f7e3a440b9c879c19809303) )
	ROM_LOAD( "rh18.067",     0x2000, 0x1000, CRC(753bdd87) SHA1(37b97dd4a5d53df9a86593fd0a53c95475fa09d0) )
	ROM_LOAD( "rh19.066",     0x3000, 0x1000, CRC(3efb3fcd) SHA1(29fb6405ced78662c4d98deeac5593d7bc42d954) )
	ROM_LOAD( "rh20.065",     0x4000, 0x1000, CRC(c2cd4e79) SHA1(0849aa0aa64c87b7f5c10f8a78caae8219059cfa) )
	ROM_LOAD( "rh21.064",     0x5000, 0x1000, CRC(74109145) SHA1(728714ec24962da1c54fc35dc3688d555a4ad101) )
	ROM_LOAD( "rh22.055",     0x6000, 0x1000, CRC(efa82a57) SHA1(b9b275014572c4c67558516d0c3c36d01e84e9ef) )
	ROM_LOAD( "rh23.054",     0x7000, 0x1000, CRC(77c25acf) SHA1(a48bf7044afa7388f68e05fdb2e63c2b04945462) )
	/* 10000-11fff space for banked ROMs (not used) */

	ROM_REGION( 0x10000, REGION_CPU2, 0 )      /* 64k for the audio CPU */
	ROM_LOAD( "rh13.070",     0x0000, 0x1000, CRC(dcad1794) SHA1(1d5479f10cdcc437241bb17c22204fb3ee60f8cb) )

	ROM_REGION( 0x8000, REGION_GFX1, 0 )       /* graphic ROMs used at runtime */
	ROM_LOAD( "rh24.001",     0x0000, 0x1000, CRC(4b1d9455) SHA1(df68a1bae7f06dff9469f8ef3095a802af3cb354) )
	ROM_LOAD( "rh25.002",     0x1000, 0x1000, CRC(bf71e278) SHA1(0016e9fff506e3d1f6a9bd8ebb23b62af00902ca) )
	ROM_LOAD( "rh26.003",     0x2000, 0x1000, CRC(13da2a9b) SHA1(e3dd30a1036ec81b3867dc1c0d20449422d50c31) )
	ROM_LOAD( "rh27.004",     0x3000, 0x1000, CRC(425b52b0) SHA1(1a3046a7d12ad8107750abfb8a801cf9cd372d0f) )

	ROM_REGION( 0x0100, REGION_PROMS, 0 )      /* layer PROM */
	ROM_LOAD( "eb16.22",      0x0000, 0x0100, CRC(b833b5ea) SHA1(d233f1bf8a3e6cd876853ffd721b9b64c61c9047) )
ROM_END

ROM_START( alpinea )
	ROM_REGION( 0x12000, REGION_CPU1, 0 )      /* 64k for code */
	ROM_LOAD( "rh01-1.69",    0x0000, 0x1000, CRC(7fbcb635) SHA1(f1d7f21d98f3b899efbca446006c1a5979f2b94c) )
	ROM_LOAD( "rh02.68",      0x1000, 0x1000, CRC(c83f95af) SHA1(2bb538582d810e44c3093d4e4f73a527ca27d2f0) )
	ROM_LOAD( "rh03.67",      0x2000, 0x1000, CRC(211102bc) SHA1(4ed21b8ff90a501bf29f3d0842857db70703d990) )
	ROM_LOAD( "rh04-1.66",    0x3000, 0x1000, CRC(494a91b0) SHA1(f3a07a2a9091bb1fe9eeba62f2ecb9e2f9c8c033) )
	ROM_LOAD( "rh05.65",      0x4000, 0x1000, CRC(d85588be) SHA1(2c2fa519ea90c80984ab58645bcba148edf6d014) )
	ROM_LOAD( "rh06.64",      0x5000, 0x1000, CRC(521fddb9) SHA1(73540c8f4c15a990ee81e6cfeace94938afbad72) )
	ROM_LOAD( "rh07.55",      0x6000, 0x1000, CRC(51f369a4) SHA1(1bbc92955875794006f31dd63fbcc3b2e5e0de54) )
	ROM_LOAD( "rh08.54",      0x7000, 0x1000, CRC(e0af9cb2) SHA1(d657a8de11f202351571b822065a37ba911723c2) )
	/* 10000-11fff space for banked ROMs (not used) */

	ROM_REGION( 0x10000, REGION_CPU2, 0 )      /* 64k for the audio CPU */
	ROM_LOAD( "rh13.070",     0x0000, 0x1000, CRC(dcad1794) SHA1(1d5479f10cdcc437241bb17c22204fb3ee60f8cb) )

	ROM_REGION( 0x8000, REGION_GFX1, 0 )       /* graphic ROMs used at runtime */
	ROM_LOAD( "rh24.001",     0x0000, 0x1000, CRC(4b1d9455) SHA1(df68a1bae7f06dff9469f8ef3095a802af3cb354) )
	ROM_LOAD( "rh25.002",     0x1000, 0x1000, CRC(bf71e278) SHA1(0016e9fff506e3d1f6a9bd8ebb23b62af00902ca) )
	ROM_LOAD( "rh26.003",     0x2000, 0x1000, CRC(13da2a9b) SHA1(e3dd30a1036ec81b3867dc1c0d20449422d50c31) )
	ROM_LOAD( "rh12.4",       0x3000, 0x1000, CRC(0ff0d1fe) SHA1(584b21b9114321439a722e35c6973e0513d696c0) )

	ROM_REGION( 0x0100, REGION_PROMS, 0 )      /* layer PROM */
	ROM_LOAD( "eb16.22",      0x0000, 0x0100, CRC(b833b5ea) SHA1(d233f1bf8a3e6cd876853ffd721b9b64c61c9047) )
ROM_END

ROM_START( timetunl )
	ROM_REGION( 0x12000, REGION_CPU1, 0 )      /* 64k for code */
	ROM_LOAD( "un01.69",      0x00000, 0x1000, CRC(2e56d946) SHA1(22b344b4467701d94bbd1ed7946a678893c92a75) )
	ROM_LOAD( "un02.68",      0x01000, 0x1000, CRC(f611d852) SHA1(c8709736c586b4288b19d0fbfc56ac9b988e7cdb) )
	ROM_LOAD( "un03.67",      0x02000, 0x1000, CRC(144b5e7f) SHA1(5920fd324253028bcca347b0279f24c665bcc7de) )
	ROM_LOAD( "un04.66",      0x03000, 0x1000, CRC(b6767eba) SHA1(76a005bf2984af3862ab46044dc124bc663b457e) )
	ROM_LOAD( "un05.65",      0x04000, 0x1000, CRC(91e3c558) SHA1(d7b402c58a70a99479accdfc3dd66c1bf6d2cdd6) )
	ROM_LOAD( "un06.64",      0x05000, 0x1000, CRC(af5a7d2a) SHA1(a214df8d29346f729502bb31e46b1c4e897ac5a1) )
	ROM_LOAD( "un07.55",      0x06000, 0x1000, CRC(4ee50999) SHA1(ea3f726c42c52aefcb3fb3cacffef32dfa86a9db) )
	ROM_LOAD( "un08.54",      0x07000, 0x1000, CRC(97259b57) SHA1(6c8c21e99fc4c59cd884a58b1995ecf7dca72206) )
	ROM_LOAD( "un09.53",      0x10000, 0x1000, CRC(771d0fb0) SHA1(5716df303a2b1119558d7061e262cbd39219a37e) ) /* banked at 6000 */
	ROM_LOAD( "un10.52",      0x11000, 0x1000, CRC(8b6afad2) SHA1(ff3e38182944c51e8e3b116eef304faa944910ee) ) /* banked at 7000 */

	ROM_REGION( 0x10000, REGION_CPU2, 0 )      /* 64k for the audio CPU */
	ROM_LOAD( "un19.70",      0x0000, 0x1000, CRC(dbf726c6) SHA1(0c38cf641e23f98a885c20bf4e371b1af9660175) )

	ROM_REGION( 0x8000, REGION_GFX1, 0 )       /* graphic ROMs used at runtime */
	ROM_LOAD( "un11.1",       0x0000, 0x1000, CRC(3be4fed6) SHA1(fe5055d1f451d6779736e6cd7d6bd89dea6b11be) )
	ROM_LOAD( "un12.2",       0x1000, 0x1000, CRC(2dee1cf3) SHA1(d9f68f44e4f3fd4d5e95c3d87368319d1a7dd2d4) )
	ROM_LOAD( "un13.3",       0x2000, 0x1000, CRC(72b491a8) SHA1(c66e55aa7bb874053d57e6cc1c39a410ea8ee713) )
	ROM_LOAD( "un14.4",       0x3000, 0x1000, CRC(5f695369) SHA1(4b35df4475d94bdd37b62b59de85c4eb61b4f519) )
	ROM_LOAD( "un15.5",       0x4000, 0x1000, CRC(001df94b) SHA1(d6168179f7e7c4e747411381c214c9211ac6ef9f) )
	ROM_LOAD( "un16.6",       0x5000, 0x1000, CRC(e33b9019) SHA1(f9f55ec878ea7e1edac05694fe438e9826159674) )
	ROM_LOAD( "un17.7",       0x6000, 0x1000, CRC(d66025b8) SHA1(b0ca7176b38b2cf729816d796be9d50a39a5d7ee) )
	ROM_LOAD( "un18.8",       0x7000, 0x1000, CRC(e67ff377) SHA1(3dd14f55a0959684a3fb61997d78945b7326c7eb) )

	ROM_REGION( 0x0100, REGION_PROMS, 0 )      /* layer PROM */
	ROM_LOAD( "eb16.22",      0x0000, 0x0100, CRC(b833b5ea) SHA1(d233f1bf8a3e6cd876853ffd721b9b64c61c9047) )
ROM_END

ROM_START( wwestern )
	ROM_REGION( 0x12000, REGION_CPU1, 0 )      /* 64k for code */
	ROM_LOAD( "ww01.bin",     0x0000, 0x1000, CRC(bfe10753) SHA1(468cf5a2f7232b5faf4a371aee2c83196fc486a5) )
	ROM_LOAD( "ww02d.bin",    0x1000, 0x1000, CRC(20579e90) SHA1(be250b4b89d1742b4cffd108c32497a0db428335) )
	ROM_LOAD( "ww03d.bin",    0x2000, 0x1000, CRC(0e65be37) SHA1(5e672c6a33d9b68752c27465fd6172304de55d2d) )
	ROM_LOAD( "ww04d.bin",    0x3000, 0x1000, CRC(b3565a31) SHA1(a42a7989530108aa198d77e1369d5b2ca6c69907) )
	ROM_LOAD( "ww05d.bin",    0x4000, 0x1000, CRC(089f3d89) SHA1(aa8c9868ebc593eb1cd9af4d41e1aa0cbb8fb316) )
	ROM_LOAD( "ww06d.bin",    0x5000, 0x1000, CRC(c81c9736) SHA1(43dfdcd93f30a99ffb22d4d5fc9bc886276ab69c) )
	ROM_LOAD( "ww07.bin",     0x6000, 0x1000, CRC(1937cc17) SHA1(fcec4b6dafd631dd2b33db5852b5ae9412910527) )
	/* 10000-11fff space for banked ROMs (not used) */

	ROM_REGION( 0x10000, REGION_CPU2, 0 )      /* 64k for the audio CPU */
	ROM_LOAD( "ww14.bin",     0x0000, 0x1000, CRC(23776870) SHA1(b15fe4f0fabd0939d87fe8c04c7edf74bbd6a23b) )

	ROM_REGION( 0x8000, REGION_GFX1, 0 )       /* graphic ROMs used at runtime */
	ROM_LOAD( "ww08.bin",     0x0000, 0x1000, CRC(041a5a1c) SHA1(c4006dc4915c6107a3f2e41534521385ba6c306c) )
	ROM_LOAD( "ww09.bin",     0x1000, 0x1000, CRC(07982ac5) SHA1(4ff76a53cb7af9ccf084513ee66c73bf16826959) )
	ROM_LOAD( "ww10.bin",     0x2000, 0x1000, CRC(f32ae203) SHA1(3455f99403246ce4483c3b6e954ebe6d93725ec3) )
	ROM_LOAD( "ww11.bin",     0x3000, 0x1000, CRC(7ff1431f) SHA1(945d7f58bd8855f046693eea7791c164bd1d7a3d) )
	ROM_LOAD( "ww12.bin",     0x4000, 0x1000, CRC(be1b563a) SHA1(447e1c982eea6198b99095e26aa5fec4e9ae1e54) )
	ROM_LOAD( "ww13.bin",     0x5000, 0x1000, CRC(092cd9e5) SHA1(56059bf41bb4ccf0f88cea679fdffc061e19f76e) )

	ROM_REGION( 0x0100, REGION_PROMS, 0 )      /* layer PROM */
	ROM_LOAD( "ww17",         0x0000, 0x0100, CRC(93447d2b) SHA1(d29f4a56a06ac809b4b9efa8aa9d1f246250e3a2) )
ROM_END

ROM_START( wwester1 )
	ROM_REGION( 0x12000, REGION_CPU1, 0 )      /* 64k for code */
	ROM_LOAD( "ww01.bin",     0x0000, 0x1000, CRC(bfe10753) SHA1(468cf5a2f7232b5faf4a371aee2c83196fc486a5) )
	ROM_LOAD( "ww02",         0x1000, 0x1000, CRC(f011103a) SHA1(4d94140fb121abb366933bb56d216bdcf2b0a9f4) )
	ROM_LOAD( "ww03d.bin",    0x2000, 0x1000, CRC(0e65be37) SHA1(5e672c6a33d9b68752c27465fd6172304de55d2d) )
	ROM_LOAD( "ww04a",        0x3000, 0x1000, CRC(68b31a6e) SHA1(580e2b560275880ab8d75670f90d314341491953) )
	ROM_LOAD( "ww05",         0x4000, 0x1000, CRC(78293f81) SHA1(311d4200d3f9c1760f31a6c695b31f789ce4aadd) )
	ROM_LOAD( "ww06",         0x5000, 0x1000, CRC(d015e435) SHA1(2b0933348245f4359f5d054289c82647bab1e13d) )
	ROM_LOAD( "ww07.bin",     0x6000, 0x1000, CRC(1937cc17) SHA1(fcec4b6dafd631dd2b33db5852b5ae9412910527) )
	/* 10000-11fff space for banked ROMs (not used) */

	ROM_REGION( 0x10000, REGION_CPU2, 0 )      /* 64k for the audio CPU */
	ROM_LOAD( "ww14.bin",     0x0000, 0x1000, CRC(23776870) SHA1(b15fe4f0fabd0939d87fe8c04c7edf74bbd6a23b) )

	ROM_REGION( 0x8000, REGION_GFX1, 0 )       /* graphic ROMs used at runtime */
	ROM_LOAD( "ww08.bin",     0x0000, 0x1000, CRC(041a5a1c) SHA1(c4006dc4915c6107a3f2e41534521385ba6c306c) )
	ROM_LOAD( "ww09.bin",     0x1000, 0x1000, CRC(07982ac5) SHA1(4ff76a53cb7af9ccf084513ee66c73bf16826959) )
	ROM_LOAD( "ww10.bin",     0x2000, 0x1000, CRC(f32ae203) SHA1(3455f99403246ce4483c3b6e954ebe6d93725ec3) )
	ROM_LOAD( "ww11.bin",     0x3000, 0x1000, CRC(7ff1431f) SHA1(945d7f58bd8855f046693eea7791c164bd1d7a3d) )
	ROM_LOAD( "ww12.bin",     0x4000, 0x1000, CRC(be1b563a) SHA1(447e1c982eea6198b99095e26aa5fec4e9ae1e54) )
	ROM_LOAD( "ww13.bin",     0x5000, 0x1000, CRC(092cd9e5) SHA1(56059bf41bb4ccf0f88cea679fdffc061e19f76e) )

	ROM_REGION( 0x0100, REGION_PROMS, 0 )      /* layer PROM */
	ROM_LOAD( "ww17",         0x0000, 0x0100, CRC(93447d2b) SHA1(d29f4a56a06ac809b4b9efa8aa9d1f246250e3a2) )
ROM_END

ROM_START( frontlin )
	ROM_REGION( 0x12000, REGION_CPU1, 0 )      /* 64k for code */
	ROM_LOAD( "fl69.u69",     0x00000, 0x1000, CRC(93b64599) SHA1(1d4e135d5105d5e2949dbd791eb87c30e8505f1d) )
	ROM_LOAD( "fl68.u68",     0x01000, 0x1000, CRC(82dccdfb) SHA1(0c88feff88b61dc0ae5487aa0a83f665a308658f) )
	ROM_LOAD( "fl67.u67",     0x02000, 0x1000, CRC(3fa1ba12) SHA1(06eaccc75a4a950ed509c0dd203eeb7120849e74) )
	ROM_LOAD( "fl66.u66",     0x03000, 0x1000, CRC(4a3db285) SHA1(0b486523d4ae302962dcb4ca042754fd96208259) )
	ROM_LOAD( "fl65.u65",     0x04000, 0x1000, CRC(da00ec70) SHA1(652eb07c1e98ed04042a334ad8e27fd3da8dd6a2) )
	ROM_LOAD( "fl64.u64",     0x05000, 0x1000, CRC(9fc90a20) SHA1(2d1bc248ed68dbb1993c360a9f2e2dbe26c216fb) )
	ROM_LOAD( "fl55.u55",     0x06000, 0x1000, CRC(359242c2) SHA1(63bd845b2d881946a7904e4df1db3d78a60b57ad) )
	ROM_LOAD( "fl54.u54",     0x07000, 0x1000, CRC(d234c60f) SHA1(b45bf432a64b7aaf3762d72a762b5eca198d5b3d) )
	ROM_LOAD( "aa1_10.8",     0x0e000, 0x1000, CRC(2704aa4c) SHA1(d8dbad5deeef2c7b032b741ab3014a8402c334eb) )
	ROM_LOAD( "fl53.u53",     0x10000, 0x1000, CRC(67429975) SHA1(b84254b2d04b034c2602f95587523a77dfdbae71) ) /* banked at 6000 */
	ROM_LOAD( "fl52.u52",     0x11000, 0x1000, CRC(cb223d34) SHA1(a1a4530ed25064c6cabe34c52bb239e3656e4ced) ) /* banked at 7000 */

	ROM_REGION( 0x10000, REGION_CPU2, 0 )      /* 64k for the audio CPU */
	ROM_LOAD( "fl70.u70",     0x0000, 0x1000, CRC(15f4ed8c) SHA1(ec096234e4e594100180eb99c8c57eb97b9f57e2) )
	ROM_LOAD( "fl71.u71",     0x1000, 0x1000, CRC(c3eb38e7) SHA1(427e5deb6a6e22d8c34923209a818f79d50e59d4) )

	ROM_REGION( 0x0800, REGION_CPU3, 0 )       /* 2k for the microcontroller */
	ROM_LOAD( "aa1.13",       0x0000, 0x0800, CRC(7e78bdd3) SHA1(9eeb0e969fd013b9db074a15b0463216453e9364) )

	ROM_REGION( 0x8000, REGION_GFX1, 0 )       /* graphic ROMs used at runtime */
	ROM_LOAD( "fl1.u1",       0x0000, 0x1000, CRC(e82c9f46) SHA1(eaab468bb5e46e9c714e6f84e65f954331fdbc56) )
	ROM_LOAD( "fl2.u2",       0x1000, 0x1000, CRC(123055d3) SHA1(6aaddd8ebb418c7c8584eb74ad13cd5accd5a196) )
	ROM_LOAD( "fl3.u3",       0x2000, 0x1000, CRC(7ea46347) SHA1(b924a614abe01f7ca6a31463864d6cc55a47946e) )
	ROM_LOAD( "fl4.u4",       0x3000, 0x1000, CRC(9e2cff10) SHA1(0932c15eacccab5a3a931dd40c1a35b5a4ca1cd5) )
	ROM_LOAD( "fl5.u5",       0x4000, 0x1000, CRC(630b4be1) SHA1(780f75fdea68917a08f5f00da3831eaa26fd4405) )
	ROM_LOAD( "fl6.u6",       0x5000, 0x1000, CRC(9e092d58) SHA1(8388870bb40c7a2e3b4ede74c37c71c3a3d1a607) )
	ROM_LOAD( "fl7.u7",       0x6000, 0x1000, CRC(613682a3) SHA1(b681f3a4e70f207ce140adfac1388900d5013317) )
	ROM_LOAD( "fl8.u8",       0x7000, 0x1000, CRC(f73b0d5e) SHA1(3f4ae070e39fac3c64c6c438168d131bffc580e2) )

	ROM_REGION( 0x0100, REGION_PROMS, 0 )      /* layer PROM */
	ROM_LOAD( "eb16.22",      0x0000, 0x0100, CRC(b833b5ea) SHA1(d233f1bf8a3e6cd876853ffd721b9b64c61c9047) )
ROM_END

ROM_START( elevator )
	ROM_REGION( 0x12000, REGION_CPU1, 0 )      /* 64k for code */
	ROM_LOAD( "ea-ic69.bin",  0x0000, 0x1000, CRC(24e277ef) SHA1(764e3b3a34bf0ec849d58023f710e5b0a0d0ccb5) )
	ROM_LOAD( "ea-ic68.bin",  0x1000, 0x1000, CRC(13702e39) SHA1(b72fea84f8322463ff224e3b06698a1ed7e305b7) )
	ROM_LOAD( "ea-ic67.bin",  0x2000, 0x1000, CRC(46f52646) SHA1(11b68b89ab0f580bfe88047e59bd9bba237a2eb4) )
	ROM_LOAD( "ea-ic66.bin",  0x3000, 0x1000, CRC(e22fe57e) SHA1(50888975e698c4d2a124e5731d0922df43eb01ef) )
	ROM_LOAD( "ea-ic65.bin",  0x4000, 0x1000, CRC(c10691d7) SHA1(a7657d3d661421d1fca3b04e4025725272b77203) )
	ROM_LOAD( "ea-ic64.bin",  0x5000, 0x1000, CRC(8913b293) SHA1(163daa07b6d45469f18e4f4a1904b60a890c8699) )
	ROM_LOAD( "ea-ic55.bin",  0x6000, 0x1000, CRC(1cabda08) SHA1(8fff75a354ee7589bd0ffe8b0271fd9111b2b241) )
	ROM_LOAD( "ea-ic54.bin",  0x7000, 0x1000, CRC(f4647b4f) SHA1(711a9447d30b35bc38e149e0cf6e835ff06efd54) )
	/* 10000-11fff space for banked ROMs (not used) */

	ROM_REGION( 0x10000, REGION_CPU2, 0 )      /* 64k for the audio CPU */
	ROM_LOAD( "ea-ic70.bin",  0x0000, 0x1000, CRC(6d5f57cb) SHA1(abb916d675ee85032697d656121d4f525202cab3) )
	ROM_LOAD( "ea-ic71.bin",  0x1000, 0x1000, CRC(f0a769a1) SHA1(9970fba3afeaaaa7fd217f0704fb9df9cf13cf65) )

	ROM_REGION( 0x0800, REGION_CPU3, 0 )       /* 2k for the microcontroller */
	ROM_LOAD( "ba3.11",       0x0000, 0x0800, CRC(9ce75afc) SHA1(4c8f5d926ae2bec8fcb70692125b9e1c863166c6) )

	ROM_REGION( 0x8000, REGION_GFX1, 0 )       /* graphic ROMs used at runtime */
	ROM_LOAD( "ea-ic1.bin",   0x0000, 0x1000, CRC(bbbb3fba) SHA1(a8e3a0886ea5dc8e70aa280b4cef5fb26ca0e125) )
	ROM_LOAD( "ea-ic2.bin",   0x1000, 0x1000, CRC(639cc2fd) SHA1(0ba292ac34dbf779a929db6358cd842d38077b3d) )
	ROM_LOAD( "ea-ic3.bin",   0x2000, 0x1000, CRC(61317eea) SHA1(f1a18c09e31edb4ec3ad7ab853f425383ca22314) )
	ROM_LOAD( "ea-ic4.bin",   0x3000, 0x1000, CRC(55446482) SHA1(0767701213920d30d5a3a226b25cfbbd3f24437a) )
	ROM_LOAD( "ea-ic5.bin",   0x4000, 0x1000, CRC(77895c0f) SHA1(fe116c53a7e8ac523a17249a56df9f40b503b30d) )
	ROM_LOAD( "ea-ic6.bin",   0x5000, 0x1000, CRC(9a1b6901) SHA1(646491c1d28904d9e662b1bff554bb74ec47708d) )
	ROM_LOAD( "ea-ic7.bin",   0x6000, 0x1000, CRC(839112ec) SHA1(30bca7f5214bf424aa10184094947496f054ddf4) )
	ROM_LOAD( "ea-ic8.bin",   0x7000, 0x1000, CRC(db7ff692) SHA1(4d0d9ab0c9d8d758e121f2bcfc6422ffadf2d760) )

	ROM_REGION( 0x0100, REGION_PROMS, 0 )      /* layer PROM */
	ROM_LOAD( "eb16.22",      0x0000, 0x0100, CRC(b833b5ea) SHA1(d233f1bf8a3e6cd876853ffd721b9b64c61c9047) )
ROM_END

ROM_START( elevatob )
	ROM_REGION( 0x12000, REGION_CPU1, 0 )      /* 64k for code */
	ROM_LOAD( "ea69.bin",     0x0000, 0x1000, CRC(66baa214) SHA1(fad660d2983daad478085be3b1a951b0864485dd) )
	ROM_LOAD( "ea-ic68.bin",  0x1000, 0x1000, CRC(13702e39) SHA1(b72fea84f8322463ff224e3b06698a1ed7e305b7) )
	ROM_LOAD( "ea-ic67.bin",  0x2000, 0x1000, CRC(46f52646) SHA1(11b68b89ab0f580bfe88047e59bd9bba237a2eb4) )
	ROM_LOAD( "ea66.bin",     0x3000, 0x1000, CRC(b88f3383) SHA1(99f23d82d7866e4bca8f5a508e0913673d12489b) )
	ROM_LOAD( "ea-ic65.bin",  0x4000, 0x1000, CRC(c10691d7) SHA1(a7657d3d661421d1fca3b04e4025725272b77203) )
	ROM_LOAD( "ea-ic64.bin",  0x5000, 0x1000, CRC(8913b293) SHA1(163daa07b6d45469f18e4f4a1904b60a890c8699) )
	ROM_LOAD( "ea55.bin",     0x6000, 0x1000, CRC(d546923e) SHA1(1de128cf96e6d976f6f09e4ac2b4d2507935bfe9) )
	ROM_LOAD( "ea54.bin",     0x7000, 0x1000, CRC(963ec5a5) SHA1(b2f684b61feb31e3d1856c16edcef33262f68581) )
	/* 10000-10fff space for another banked ROM (not used) */
	ROM_LOAD( "ea52.bin",     0x11000, 0x1000, CRC(44b1314a) SHA1(e8980536f787fcb603943d4b2b3a64f475e51a16) ) /* protection crack, bank switched at 7000 */

	ROM_REGION( 0x10000, REGION_CPU2, 0 )      /* 64k for the audio CPU */
	ROM_LOAD( "ea-ic70.bin",  0x0000, 0x1000, CRC(6d5f57cb) SHA1(abb916d675ee85032697d656121d4f525202cab3) )
	ROM_LOAD( "ea-ic71.bin",  0x1000, 0x1000, CRC(f0a769a1) SHA1(9970fba3afeaaaa7fd217f0704fb9df9cf13cf65) )

	ROM_REGION( 0x8000, REGION_GFX1, 0 )       /* graphic ROMs used at runtime */
	ROM_LOAD( "ea-ic1.bin",   0x0000, 0x1000, CRC(bbbb3fba) SHA1(a8e3a0886ea5dc8e70aa280b4cef5fb26ca0e125) )
	ROM_LOAD( "ea-ic2.bin",   0x1000, 0x1000, CRC(639cc2fd) SHA1(0ba292ac34dbf779a929db6358cd842d38077b3d) )
	ROM_LOAD( "ea-ic3.bin",   0x2000, 0x1000, CRC(61317eea) SHA1(f1a18c09e31edb4ec3ad7ab853f425383ca22314) )
	ROM_LOAD( "ea-ic4.bin",   0x3000, 0x1000, CRC(55446482) SHA1(0767701213920d30d5a3a226b25cfbbd3f24437a) )
	ROM_LOAD( "ea-ic5.bin",   0x4000, 0x1000, CRC(77895c0f) SHA1(fe116c53a7e8ac523a17249a56df9f40b503b30d) )
	ROM_LOAD( "ea-ic6.bin",   0x5000, 0x1000, CRC(9a1b6901) SHA1(646491c1d28904d9e662b1bff554bb74ec47708d) )
	ROM_LOAD( "ea-ic7.bin",   0x6000, 0x1000, CRC(839112ec) SHA1(30bca7f5214bf424aa10184094947496f054ddf4) )
	ROM_LOAD( "ea08.bin",     0x7000, 0x1000, CRC(67ebf7c1) SHA1(ec4db1392967f3959574bc4ca03e95938a6e5173) )

	ROM_REGION( 0x0100, REGION_PROMS, 0 )      /* layer PROM */
	ROM_LOAD( "eb16.22",      0x0000, 0x0100, CRC(b833b5ea) SHA1(d233f1bf8a3e6cd876853ffd721b9b64c61c9047) )
ROM_END

ROM_START( tinstar )
	ROM_REGION( 0x12000, REGION_CPU1, 0 )      /* 64k for code */
	ROM_LOAD( "ts.69",        0x0000, 0x1000, CRC(a930af60) SHA1(1644fcf3460a1dfceaa39ccc54c9506289965f4c) )
	ROM_LOAD( "ts.68",        0x1000, 0x1000, CRC(7f2714ca) SHA1(55e6c83336e4db1142cc6f867e84359fadc39d5a) )
	ROM_LOAD( "ts.67",        0x2000, 0x1000, CRC(49170786) SHA1(9749c60a225b9698bb19b3ab99ea91dd4571197d) )
	ROM_LOAD( "ts.66",        0x3000, 0x1000, CRC(3766f130) SHA1(939a2ad34c8fb3f7f99790d3e492f13be11dadf6) )
	ROM_LOAD( "ts.65",        0x4000, 0x1000, CRC(41251246) SHA1(da8323afb4967eb530f52008e49bb974b30e7e66) )
	ROM_LOAD( "ts.64",        0x5000, 0x1000, CRC(812285d5) SHA1(e57adc29567379603570316d37b8abde05c1e690) )
	ROM_LOAD( "ts.55",        0x6000, 0x1000, CRC(6b80ac51) SHA1(5b3b848273763af5629bad8b5f3a8518d37a6316) )
	ROM_LOAD( "ts.54",        0x7000, 0x1000, CRC(b352360f) SHA1(85dedb98a9d604bd816f626fb39fc49a7f1e73d2) )
	/* 10000-11fff space for banked ROMs (not used) */

	ROM_REGION( 0x10000, REGION_CPU2, 0 )      /* 64k for the audio CPU */
	ROM_LOAD( "ts.70",        0x0000, 0x1000, CRC(4771838d) SHA1(f84f1367f8a86e6c070da29419c8de5c302d1469) )
	ROM_LOAD( "ts.71",        0x1000, 0x1000, CRC(03c91332) SHA1(3903e876ae02e9aea7ee6854bb4c6407dd7108d6) )
	ROM_LOAD( "ts.72",        0x2000, 0x1000, CRC(beeed8f3) SHA1(2a18edecabbfd10b3238338cb5554edc8c18d93c) )

	ROM_REGION( 0x0800, REGION_CPU3, 0 )       /* 2k for the microcontroller */
	ROM_LOAD( "a10-12",       0x0000, 0x0800, CRC(889eefc9) SHA1(1a31aa21c02215410eea27ed52fad67f007ee810) )

	ROM_REGION( 0x8000, REGION_GFX1, 0 )       /* graphic ROMs used at runtime */
	ROM_LOAD( "ts.1",         0x0000, 0x1000, CRC(f1160718) SHA1(d35ebb96867299137ba1ce19da998002a6b9898d) )
	ROM_LOAD( "ts.2",         0x1000, 0x1000, CRC(39dc6dbb) SHA1(223c3430ea9acaeed921f31e6c1e5153dc48874c) )
	ROM_LOAD( "ts.3",         0x2000, 0x1000, CRC(079df429) SHA1(7f1fb64dfecf7b20a7e8357393084eb9c709a898) )
	ROM_LOAD( "ts.4",         0x3000, 0x1000, CRC(e61105d4) SHA1(44dbe3be78b4ba0c1e17c46498b71158ebcfd6d2) )
	ROM_LOAD( "ts.5",         0x4000, 0x1000, CRC(ffab5d15) SHA1(63b71775646f8dc838f0fdb8abb0fe63d4f2f6ae) )
	ROM_LOAD( "ts.6",         0x5000, 0x1000, CRC(f1d8ca36) SHA1(09c5e4885edb2906a929735347f04d06cc6dfeda) )
	ROM_LOAD( "ts.7",         0x6000, 0x1000, CRC(894f6332) SHA1(9b4a0e35ec08f2d6c5340435c69a05ad0333627c) )
	ROM_LOAD( "ts.8",         0x7000, 0x1000, CRC(519aed19) SHA1(8618a794bdd091ed7c2300a2dabf99455c9e2ebd) )

	ROM_REGION( 0x0100, REGION_PROMS, 0 )      /* layer PROM */
	ROM_LOAD( "eb16.22",      0x0000, 0x0100, CRC(b833b5ea) SHA1(d233f1bf8a3e6cd876853ffd721b9b64c61c9047) )
ROM_END

ROM_START( waterski )
	ROM_REGION( 0x12000, REGION_CPU1, 0 )      /* 64k for code */
	ROM_LOAD( "a03-01",       0x0000, 0x1000, CRC(322c4c2c) SHA1(8f25acd50fdda4cae756849f3961c79d6138866e) )
	ROM_LOAD( "a03-02",       0x1000, 0x1000, CRC(8df176d1) SHA1(db6e7a82320dc478306b0b1a06c284ed4faf5103) )
	ROM_LOAD( "a03-03",       0x2000, 0x1000, CRC(420bd04f) SHA1(1efce6d384cde94c0fd9e7c3a43feaa18b6c3d73) )
	ROM_LOAD( "a03-04",       0x3000, 0x1000, CRC(5c081a94) SHA1(eae8b84869b1bc754550c8427c510831348418fa) )
	ROM_LOAD( "a03-05",       0x4000, 0x1000, CRC(1fae90d2) SHA1(f5f2e022794593a5a6a06223e7a1ac19ef23e140) )
	ROM_LOAD( "a03-06",       0x5000, 0x1000, CRC(55b7c151) SHA1(ad05dcd07a7907d08c5d8827e323c207a7c8ace6) )
	ROM_LOAD( "a03-07",       0x6000, 0x1000, CRC(8abc7522) SHA1(4a3d8ea006630020722d385f4d7b50ece323cad2) )
	/* 10000-11fff space for banked ROMs (not used) */

	ROM_REGION( 0x10000, REGION_CPU2, 0 )      /* 64k for the audio CPU */
	ROM_LOAD( "a03-13",       0x0000, 0x1000, CRC(78c7d37f) SHA1(460bff7c78a2fa78a7c6f95c7f2847c1c5267626) )
	ROM_LOAD( "a03-14",       0x1000, 0x1000, CRC(31f991ca) SHA1(82cbaa618ac3de6fce12e9dcbb89ab064773b2bd) )

	ROM_REGION( 0x8000, REGION_GFX1, 0 )       /* graphic ROMs used at runtime */
	ROM_LOAD( "a03-08",       0x0000, 0x1000, CRC(c206d870) SHA1(0be09b7da28d60bf23a0b87cff28957bb165bec5) )
	ROM_LOAD( "a03-09",       0x1000, 0x1000, CRC(48ac912a) SHA1(09d57b5b76a4416f1ee5eb2077bc969d2afbbf11) )
	ROM_LOAD( "a03-10",       0x2000, 0x1000, CRC(a056defb) SHA1(15b4202b19f7190bfc953d0958c189db2db928cc) )
	ROM_LOAD( "a03-11",       0x3000, 0x1000, CRC(f06cddd6) SHA1(483a25880a29552f4b70cd5d41de5cfe4ed64475) )
	ROM_LOAD( "a03-12",       0x4000, 0x1000, CRC(27dfd8c2) SHA1(cf4d2c4b52598cff7a934c816514de1471e4af8d) )

	ROM_REGION( 0x0100, REGION_PROMS, 0 )      /* layer PROM */
	ROM_LOAD( "eb16.22",      0x0000, 0x0100, CRC(b833b5ea) SHA1(d233f1bf8a3e6cd876853ffd721b9b64c61c9047) )
ROM_END

ROM_START( bioatack )
	ROM_REGION( 0x12000, REGION_CPU1, 0 )      /* 64k for code */
	ROM_LOAD( "aa8-01.69",    0x0000, 0x1000, CRC(e5abc211) SHA1(bdec8160a99a1869bd004e8177ac075bd52ee3c9) )
	ROM_LOAD( "aa8-02.68",    0x1000, 0x1000, CRC(b5bfde00) SHA1(403fa7133d7571b53c19f9175ee844349a43ae4e) )
	ROM_LOAD( "aa8-03.67",    0x2000, 0x1000, CRC(e4e46e69) SHA1(6b0de7dbfcc2a56b6c3fce4b94687174002a7217) )
	ROM_LOAD( "aa8-04.66",    0x3000, 0x1000, CRC(86e0af8c) SHA1(672de6978a187c2014401090c1d7fd79f7dd7c28) )
	ROM_LOAD( "aa8-05.65",    0x4000, 0x1000, CRC(c6248608) SHA1(5536c37e9b6ac290fc8a86ab194d15f63a1d40b5) )
	ROM_LOAD( "aa8-06.64",    0x5000, 0x1000, CRC(685a0383) SHA1(abc00554e27cfb9db5c60e7c9127e33d1f4d5a5f) )
	ROM_LOAD( "aa8-07.55",    0x6000, 0x1000, CRC(9d58e2b7) SHA1(ed974fdd4e1d69f08eb8214cb91493086f7ae6ed) )
	ROM_LOAD( "aa8-08.54",    0x7000, 0x1000, CRC(dec5271f) SHA1(27fc30e1c82ade6a7adda1891b78ca3e44643fd1) )
	/* 10000-11fff space for banked ROMs (not used) */

	ROM_REGION( 0x10000, REGION_CPU2, 0 )      /* 64k for the audio CPU */
	ROM_LOAD( "aa8-17.70",    0x0000, 0x1000, CRC(36eb95b5) SHA1(ae31aff6cb34ffa8ce8a3574697b89f6d9dd273c) )

	ROM_REGION( 0x8000, REGION_GFX1, 0 )       /* graphic ROMs used at runtime */
	ROM_LOAD( "aa8-09.1",     0x0000, 0x1000, CRC(1fee5fd6) SHA1(04de2d285f98275ca5809261d264504f9638d2b0) )
	ROM_LOAD( "aa8-10.2",     0x1000, 0x1000, CRC(e0133423) SHA1(50f3aee729dfda34b1025541f1ee59394e4c386d) )
	ROM_LOAD( "aa8-11.3",     0x2000, 0x1000, CRC(0f5715c6) SHA1(a6893a86c3f6e9e17eb31381eea36bd17edf1e62) )
	ROM_LOAD( "aa8-12.4",     0x3000, 0x1000, CRC(71126dd0) SHA1(9d343bc2294a74df6fc2234e250bf4cdd4970d7c) )
	ROM_LOAD( "aa8-13.5",     0x4000, 0x1000, CRC(adcdd2f0) SHA1(b7806e5a8f639795b43553689675861f043ec702) )
	ROM_LOAD( "aa8-14.6",     0x5000, 0x1000, CRC(2fe18680) SHA1(b47095fe48572b9354dd80a9dda9dc45842f0369) )
	ROM_LOAD( "aa8-15.7",     0x6000, 0x1000, CRC(ff5aad4b) SHA1(9ce15d3e5bfbab84913a6fa0830d438c63a86de7) )
	ROM_LOAD( "aa8-16.8",     0x7000, 0x1000, CRC(ceba4036) SHA1(f059ed8482a06c5495c6b3a6e55dbda6b77a894c) )

	ROM_REGION( 0x0100, REGION_PROMS, 0 )      /* layer PROM */
	ROM_LOAD( "eb16.22",      0x0000, 0x0100, CRC(b833b5ea) SHA1(d233f1bf8a3e6cd876853ffd721b9b64c61c9047) )
ROM_END

ROM_START( sfposeid )
	ROM_REGION( 0x12000, REGION_CPU1, 0 )      /* 64k for code */
	ROM_LOAD( "a14-01.1",     0x00000, 0x2000, CRC(aa779fbb) SHA1(d38f16c6f3d3769f82f37ee3bcf0e7b17756dc53) )
	ROM_LOAD( "a14-02.2",     0x02000, 0x2000, CRC(ecec9dc3) SHA1(f9548a7b2bd18e8bd4bc58dd520b0926b5abafac) )
	ROM_LOAD( "a14-03.3",     0x04000, 0x2000, CRC(469498c1) SHA1(3c0a052ba3beac583fc41e55d90f13f3fa3160f1) )
	ROM_LOAD( "a14-04.6",     0x06000, 0x2000, CRC(1db4bc02) SHA1(47f469ae40fcd476edcbcb42fd1f9aa39b6aea01) )
	ROM_LOAD( "a14-05.7",     0x10000, 0x2000, CRC(95e2f903) SHA1(d872bbc2c75f7a87f721465b18881f844651dfd8) ) /* banked at 6000 */

	ROM_REGION( 0x10000, REGION_CPU2, 0 )      /* 64k for the audio CPU */
	ROM_LOAD( "a14-10.70",    0x0000, 0x1000, CRC(f1365f35) SHA1(34b3ea03eb9fbf5454858fa6e07ec49a7b3be8b4) )
	ROM_LOAD( "a14-11.71",    0x1000, 0x1000, CRC(74a12fe2) SHA1(8678ea68bd283b7a63915717cdbbedef0b699198) )

	ROM_REGION( 0x0800, REGION_CPU3, 0 )       /* 2k for the microcontroller */
	ROM_LOAD( "a14-12",       0x0000, 0x0800, CRC(091beed8) SHA1(263806aef01bbc258f5cfa92de8a9e355491fb3a) )

	ROM_REGION( 0x8000, REGION_GFX1, 0 )       /* graphic ROMs used at runtime */
	ROM_LOAD( "a14-06.4",     0x0000, 0x2000, CRC(9740493b) SHA1(7ce56a8a1f2923fe932dbe98fb8457f5f7ba3bb7) )
	ROM_LOAD( "a14-07.5",     0x2000, 0x2000, CRC(1c93de97) SHA1(a345150b1afd1352fe9103b6b5d4fbf24c7f1948) )
	ROM_LOAD( "a14-08.9",     0x4000, 0x2000, CRC(4367e65a) SHA1(d0de218377a876cd584bd80b6adfbe32e1cead99) )
	ROM_LOAD( "a14-09.10",    0x6000, 0x2000, CRC(677cffd5) SHA1(004f3df1b0f5eed0995d7ac507f4672f9e279853) )

	ROM_REGION( 0x0100, REGION_PROMS, 0 )      /* layer PROM */
	ROM_LOAD( "eb16.22",      0x0000, 0x0100, CRC(b833b5ea) SHA1(d233f1bf8a3e6cd876853ffd721b9b64c61c9047) )
ROM_END

ROM_START( hwrace )
	ROM_REGION( 0x12000, REGION_CPU1, 0 )      /* 64k for code */
	ROM_LOAD( "hw_race.01",   0x0000, 0x1000, CRC(8beec11f) SHA1(716b7ca0a49151e74a3c0289d0a7f80db8f76452) )
	ROM_LOAD( "hw_race.02",   0x1000, 0x1000, CRC(72ad099d) SHA1(2b3cc312203a062c01c90127369413297bd25d67) )
	ROM_LOAD( "hw_race.03",   0x2000, 0x1000, CRC(d0c221d7) SHA1(f940099b32524a01cb1ad035e0064d6038c039d2) )
	ROM_LOAD( "hw_race.04",   0x3000, 0x1000, CRC(eb97015b) SHA1(ada2832154077965ed1ef7aebc353fe232852d27) )
	ROM_LOAD( "hw_race.05",   0x4000, 0x1000, CRC(777c8007) SHA1(5241cd43716b6f1b6b29cee3c16cd517ee8a8b8f) )
	ROM_LOAD( "hw_race.06",   0x5000, 0x1000, CRC(165f46a3) SHA1(ba7a95a843ae2e749140fae5f0ee8a4c8e3e0721) )
	ROM_LOAD( "hw_race.07",   0x6000, 0x1000, CRC(53d7e323) SHA1(e59aba28947de6573290c1ff3923963596a5ffa8) )
	ROM_LOAD( "hw_race.08",   0x7000, 0x1000, CRC(bdbc1208) SHA1(9d2da1c987095fdb9443defea109f918d813bec6) )
	/* 10000-11fff space for banked ROMs (not used) */

	ROM_REGION( 0x10000, REGION_CPU2, 0 )      /* 64k for the audio CPU */
	ROM_LOAD( "hw_race.17",   0x0000, 0x1000, CRC(afe24f3e) SHA1(11a794bad711057728463c4db5daadac84016bf0) )
	ROM_LOAD( "hw_race.18",   0x1000, 0x1000, CRC(dbec897d) SHA1(d69a965207eac61f0ad9ea9fa42561449c5bfd11) )

	ROM_REGION( 0x8000, REGION_GFX1, 0 )       /* graphic ROMs used at runtime */
	ROM_LOAD( "hw_race.09",   0x0000, 0x1000, CRC(345b9b88) SHA1(32be98bc127059449a52ebca153cc3614933b126) )
	ROM_LOAD( "hw_race.10",   0x1000, 0x1000, CRC(598a3c3e) SHA1(48088ab7e7daeb2cf9584ed7acf5b138c0c4a027) )
	ROM_LOAD( "hw_race.11",   0x2000, 0x1000, CRC(3f436a7d) SHA1(107dc98eea1e81da667aeca89bb5ebb904727644) )
	ROM_LOAD( "hw_race.12",   0x3000, 0x1000, CRC(8694b2c6) SHA1(e91592019e05065dae2c634b48f58cca2b90acec) )
	ROM_LOAD( "hw_race.13",   0x4000, 0x1000, CRC(a0af7711) SHA1(f148ff70f9bd13f089134079dcca985ca97c63bd) )
	ROM_LOAD( "hw_race.14",   0x5000, 0x1000, CRC(9be0f556) SHA1(5e277003a454bf9f1cfc544195c5da74202cf844) )
	ROM_LOAD( "hw_race.15",   0x6000, 0x1000, CRC(e1057eb7) SHA1(65cf73d5ba6e2a65ed32414840947dcb87ddba5e) )
	ROM_LOAD( "hw_race.16",   0x7000, 0x1000, CRC(f7104668) SHA1(240ee2fad6ac82b5d3bf7af6d3621bc86236e95e) )

	ROM_REGION( 0x0100, REGION_PROMS, 0 )      /* layer PROM */
	ROM_LOAD( "eb16.22",      0x0000, 0x0100, CRC(b833b5ea) SHA1(d233f1bf8a3e6cd876853ffd721b9b64c61c9047) )
ROM_END

ROM_START( kikstart )
	ROM_REGION( 0x12000, REGION_CPU1, 0 )      /* 64k for code */
	ROM_LOAD( "a20-01",       0x00000, 0x2000, CRC(5810be97) SHA1(da3507b8274a1f5c0d5b10ad7259a0f02bfb6eda) )
	ROM_LOAD( "a20-02",       0x02000, 0x2000, CRC(13e9565d) SHA1(bb73b965262bc1bd90266b460d95fe217938a33c) )
	ROM_LOAD( "a20-03",       0x04000, 0x2000, CRC(93d7a9e1) SHA1(ab9289172a8f52e1a191efd91b52e8cf762d1f7f) )
	ROM_LOAD( "a20-04",       0x06000, 0x2000, CRC(1f23c5d6) SHA1(104312f09916e9938f71c3dffb94aaa335afb084) )
	ROM_LOAD( "a20-05",       0x10000, 0x2000, CRC(66e100aa) SHA1(a3b78b1b0250491c5e9bd39e761612a1f70640dd) ) /* banked at 6000 */

	ROM_REGION( 0x10000, REGION_CPU2, 0 )      /* 64k for the audio CPU */
	ROM_LOAD( "a20-10",       0x0000, 0x1000, CRC(de4352a4) SHA1(1548f8d7ac9e79ccaf1a503ded4b868bd350fd7e) )
	ROM_LOAD( "a20-11",       0x1000, 0x1000, CRC(8db12dd9) SHA1(3b291d478b3f3f1bf93d95a78506d99a71f36d05) )
	ROM_LOAD( "a20-12",       0x2000, 0x1000, CRC(e7eeb933) SHA1(26f3904f6d4dc814318221f1c9cd5dcc671fe05a) )

	ROM_REGION( 0x0800, REGION_CPU3, 0 )       /* 2k for the microcontroller */
	ROM_LOAD( "a20-13",       0x0000, 0x0800, CRC(11e23c5c) SHA1(cf2fcdc70d616508dd7ae630bad6a5e4ff91cdad) )

	ROM_REGION( 0x8000, REGION_GFX1, 0 )       /* graphic ROMs used at runtime */
	ROM_LOAD( "a20-06",       0x0000, 0x2000, CRC(6582fc89) SHA1(a902f4f2d3c9e352ac2f49f33b9f30be931f2be1) )
	ROM_LOAD( "a20-07",       0x2000, 0x2000, CRC(8c0b76d2) SHA1(243ef5cdd5bbf504dd040a01b8e126dbbc65f9a9) )
	ROM_LOAD( "a20-08",       0x4000, 0x2000, CRC(0cca7a9d) SHA1(bf83ad5791a2277c51b1a2644a078be0ca8a4047) )
	ROM_LOAD( "a20-09",       0x6000, 0x2000, CRC(da625ccf) SHA1(092c72b5a23d7a5683889f7597096938ba879602) )

	ROM_REGION( 0x0100, REGION_PROMS, 0 )      /* layer PROM */
	ROM_LOAD( "eb16.22",      0x0000, 0x0100, CRC(b833b5ea) SHA1(d233f1bf8a3e6cd876853ffd721b9b64c61c9047) )
ROM_END

static DRIVER_INIT( spacecr )
{
	/* install protection handler */
	install_mem_read_handler(0, 0xd48b, 0xd48b, spacecr_prot_r);
}

static DRIVER_INIT( alpine )
{
	/* install protection handlers */
	install_mem_read_handler (0, 0xd40b, 0xd40b, alpine_port_2_r);
	install_mem_write_handler(0, 0xd50f, 0xd50f, alpine_protection_w);
}

static DRIVER_INIT( alpinea )
{
	/* install protection handlers */
	install_mem_read_handler (0, 0xd40b, 0xd40b, alpine_port_2_r);
	install_mem_write_handler(0, 0xd50e, 0xd50e, alpinea_bankswitch_w);
}

static DRIVER_INIT( junglhbr )
{
	/* inverter on bits 0 and 1 */
	install_mem_write_handler (0, 0x9000, 0xbfff, junglhbr_characterram_w);
}

static DRIVER_INIT( kikstart )
{
	kikstart_gearP1 = 1;
	kikstart_gearP2 = 1;
}

GAME( 1981, spaceskr, 0,        nomcu,    spaceskr,   0,       ROT180, "Taito Corporation", "Space Seeker" )
GAME( 1981, spacecr,  0,        nomcu,    spacecr,    spacecr, ROT90,  "Taito Corporation", "Space Cruiser" )
GAME( 1982, junglek,  0,        nomcu,    junglek,    0,       ROT0,   "Taito Corporation", "Jungle King (Japan)" )
GAME( 1982, junglkj2, junglek,  nomcu,    junglek,    0,       ROT0,   "Taito Corporation", "Jungle King (Japan, earlier)" )
GAME( 1982, jungleh,  junglek,  nomcu,    junglek,    0,       ROT0,   "Taito America Corporation", "Jungle Hunt (US)" )
GAME( 1983, junglhbr, junglek,  nomcu,    junglek,    junglhbr,ROT0,   "Taito do Brasil",   "Jungle Hunt (Brazil)" )
GAME( 1982, piratpet, junglek,  nomcu,    piratpet,   0,       ROT0,   "Taito America Corporation", "Pirate Pete" )
GAME( 1982, alpine,   0,        nomcu,    alpine,     alpine,  ROT270, "Taito Corporation", "Alpine Ski (set 1)" )
GAME( 1982, alpinea,  alpine,   nomcu,    alpinea,    alpinea, ROT270, "Taito Corporation", "Alpine Ski (set 2)" )
GAME( 1982, timetunl, 0,        nomcu,    timetunl,   0,       ROT0,   "Taito Corporation", "Time Tunnel" )
GAME( 1982, wwestern, 0,        nomcu,    wwestern,   0,       ROT270, "Taito Corporation", "Wild Western (set 1)" )
GAME( 1982, wwester1, wwestern, nomcu,    wwestern,   0,       ROT270, "Taito Corporation", "Wild Western (set 2)" )
GAME( 1982, frontlin, 0,        mcu,      frontlin,   0,       ROT270, "Taito Corporation", "Front Line" )
GAME( 1983, elevator, 0,        mcu,      elevator,   0,       ROT0,   "Taito Corporation", "Elevator Action" )
GAME( 1983, elevatob, elevator, nomcu,    elevator,   0,       ROT0,   "bootleg", "Elevator Action (bootleg)" )
GAME( 1983, tinstar,  0,        mcu,      tinstar,    0,       ROT0,   "Taito Corporation", "The Tin Star" )
GAME( 1983, waterski, 0,        nomcu,    waterski,   0,       ROT270, "Taito Corporation", "Water Ski" )
GAME( 1983, bioatack, 0,        nomcu,    bioatack,   0,       ROT270, "Taito Corporation (Fox Video Games license)", "Bio Attack" )
GAME( 1984, sfposeid, 0,        mcu,      sfposeid,   0,       ROT0,   "Taito Corporation", "Sea Fighter Poseidon" )
GAME( 1983, hwrace,   0,        nomcu,    hwrace,     0,       ROT270, "Taito Corporation", "High Way Race" )
GAME( 1984, kikstart, 0,        kikstart, kikstart,   kikstart,ROT0,   "Taito Corporation", "Kick Start Wheelie King" )
