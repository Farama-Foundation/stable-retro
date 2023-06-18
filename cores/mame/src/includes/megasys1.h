/***************************************************************************

							-= Jaleco Mega System 1 =-

					driver by	Luca Elia (l.elia@tin.it)


	This file contains definitions used across multiple megasys1
	and non megasys1 Jaleco games:

	* Input ports
	* Scrolling layers handling
	* Code decryption handling

***************************************************************************/


/***************************************************************************

								Input Ports

***************************************************************************/


/* IN0 - COINS */
#define COINS \
	PORT_START\
	PORT_BIT(  0x01, IP_ACTIVE_LOW, IPT_START1 )\
	PORT_BIT(  0x02, IP_ACTIVE_LOW, IPT_START2 )\
	PORT_BIT(  0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )\
	PORT_BIT(  0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )\
	PORT_BIT(  0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )\
	PORT_BIT(  0x20, IP_ACTIVE_LOW, IPT_SERVICE1 )\
	PORT_BIT(  0x40, IP_ACTIVE_LOW, IPT_COIN1 )\
	PORT_BIT(  0x80, IP_ACTIVE_LOW, IPT_COIN2 )

/* IN1/3 - PLAYER 1/2 */
#define JOY_4BUTTONS(_flag_) \
	PORT_START\
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | _flag_ )\
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | _flag_ )\
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | _flag_ )\
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | _flag_ )\
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | _flag_ )\
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | _flag_ )\
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 | _flag_ )\
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON4 | _flag_ )

#define JOY_3BUTTONS(_flag_) \
	PORT_START\
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | _flag_ )\
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | _flag_ )\
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | _flag_ )\
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | _flag_ )\
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | _flag_ )\
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | _flag_ )\
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 | _flag_ )\
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

#define JOY_2BUTTONS(_flag_) \
	PORT_START\
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | _flag_ )\
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | _flag_ )\
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | _flag_ )\
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | _flag_ )\
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | _flag_ )\
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | _flag_ )\
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )\
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

/* IN2 - RESERVE */
#define RESERVE \
	PORT_START\
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN ) /* Reserve 1P */\
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )\
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )\
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )\
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN ) /* Reserve 2P */\
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )\
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )\
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

/* IN4 - Coinage DSWs */
/*	1]	01-41 02-31 03-21 07-11 06-12 05-13 04-14 00-FC	* 2*/
/*	2]	04-31 02-21 07-11 03-12 05-13 01-14 06-15 00-FC*/
/*		00-41 20-31 10-21 38-11 18-12 28-13 08-14 30-15*/


#define COINAGE_6BITS \
	PORT_DIPNAME( 0x07, 0x07, DEF_STR( Coin_A ) )\
	PORT_DIPSETTING(    0x04, DEF_STR( 3C_1C ) )\
	PORT_DIPSETTING(    0x02, DEF_STR( 2C_1C ) )\
	PORT_DIPSETTING(    0x07, DEF_STR( 1C_1C ) )\
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_2C ) )\
	PORT_DIPSETTING(    0x05, DEF_STR( 1C_3C ) )\
	PORT_DIPSETTING(    0x01, DEF_STR( 1C_4C ) )\
	PORT_DIPSETTING(    0x06, DEF_STR( 1C_5C ) )\
	PORT_DIPSETTING(    0x00, DEF_STR( Free_Play ) )\
	PORT_DIPNAME( 0x38, 0x38, DEF_STR( Coin_B ) )\
	PORT_DIPSETTING(    0x00, DEF_STR( 4C_1C ) )\
	PORT_DIPSETTING(    0x20, DEF_STR( 3C_1C ) )\
	PORT_DIPSETTING(    0x10, DEF_STR( 2C_1C ) )\
	PORT_DIPSETTING(    0x38, DEF_STR( 1C_1C ) )\
	PORT_DIPSETTING(    0x18, DEF_STR( 1C_2C ) )\
	PORT_DIPSETTING(    0x28, DEF_STR( 1C_3C ) )\
	PORT_DIPSETTING(    0x08, DEF_STR( 1C_4C ) )\
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_5C ) )\

#define COINAGE_6BITS_2 \
	PORT_DIPNAME( 0x07, 0x07, DEF_STR( Coin_A ) )\
	PORT_DIPSETTING(    0x01, DEF_STR( 4C_1C ) )\
	PORT_DIPSETTING(    0x02, DEF_STR( 3C_1C ) )\
	PORT_DIPSETTING(    0x03, DEF_STR( 2C_1C ) )\
	PORT_DIPSETTING(    0x07, DEF_STR( 1C_1C ) )\
	PORT_DIPSETTING(    0x06, DEF_STR( 1C_2C ) )\
	PORT_DIPSETTING(    0x05, DEF_STR( 1C_3C ) )\
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_4C ) )\
	PORT_DIPSETTING(    0x00, DEF_STR( Free_Play ) )\
	PORT_DIPNAME( 0x38, 0x38, DEF_STR( Coin_B ) )\
	PORT_DIPSETTING(    0x08, DEF_STR( 4C_1C ) )\
	PORT_DIPSETTING(    0x10, DEF_STR( 3C_1C ) )\
	PORT_DIPSETTING(    0x18, DEF_STR( 2C_1C ) )\
	PORT_DIPSETTING(    0x38, DEF_STR( 1C_1C ) )\
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_2C ) )\
	PORT_DIPSETTING(    0x28, DEF_STR( 1C_3C ) )\
	PORT_DIPSETTING(    0x20, DEF_STR( 1C_4C ) )\
	PORT_DIPSETTING(    0x00, DEF_STR( Free_Play ) )\

#define COINAGE_8BITS \
	PORT_DIPNAME( 0x0f, 0x0f, DEF_STR( Coin_A ) )\
	PORT_DIPSETTING(    0x07, DEF_STR( 4C_1C ) )\
	PORT_DIPSETTING(    0x08, DEF_STR( 3C_1C ) )\
	PORT_DIPSETTING(    0x09, DEF_STR( 2C_1C ) )\
	PORT_DIPSETTING(    0x0f, DEF_STR( 1C_1C ) )\
/*	PORT_DIPSETTING(    0x05, DEF_STR( 1C_1C ) )*/	\
/*	PORT_DIPSETTING(    0x04, DEF_STR( 1C_1C ) )*/	\
/*	PORT_DIPSETTING(    0x03, DEF_STR( 1C_1C ) )*/	\
/*	PORT_DIPSETTING(    0x02, DEF_STR( 1C_1C ) )*/	\
/*	PORT_DIPSETTING(    0x01, DEF_STR( 1C_1C ) )*/	\
	PORT_DIPSETTING(    0x06, DEF_STR( 2C_3C ) )\
	PORT_DIPSETTING(    0x0e, DEF_STR( 1C_2C ) )\
	PORT_DIPSETTING(    0x0d, DEF_STR( 1C_3C ) )\
	PORT_DIPSETTING(    0x0c, DEF_STR( 1C_4C ) )\
	PORT_DIPSETTING(    0x0b, DEF_STR( 1C_5C ) )\
	PORT_DIPSETTING(    0x0a, DEF_STR( 1C_6C ) )\
	PORT_DIPSETTING(    0x00, DEF_STR( Free_Play ) )\
	PORT_DIPNAME( 0xf0, 0xf0, DEF_STR( Coin_B ) )\
	PORT_DIPSETTING(    0x70, DEF_STR( 4C_1C ) )\
	PORT_DIPSETTING(    0x80, DEF_STR( 3C_1C ) )\
	PORT_DIPSETTING(    0x90, DEF_STR( 2C_1C ) )\
	PORT_DIPSETTING(    0xf0, DEF_STR( 1C_1C ) )\
/*	PORT_DIPSETTING(    0x50, DEF_STR( 1C_1C ) )*/	\
/*	PORT_DIPSETTING(    0x40, DEF_STR( 1C_1C ) )*/	\
/*	PORT_DIPSETTING(    0x30, DEF_STR( 1C_1C ) )*/	\
/*	PORT_DIPSETTING(    0x20, DEF_STR( 1C_1C ) )*/	\
/*	PORT_DIPSETTING(    0x10, DEF_STR( 1C_1C ) )*/	\
	PORT_DIPSETTING(    0x60, DEF_STR( 2C_3C ) )\
	PORT_DIPSETTING(    0xe0, DEF_STR( 1C_2C ) )\
	PORT_DIPSETTING(    0xd0, DEF_STR( 1C_3C ) )\
	PORT_DIPSETTING(    0xc0, DEF_STR( 1C_4C ) )\
	PORT_DIPSETTING(    0xb0, DEF_STR( 1C_5C ) )\
	PORT_DIPSETTING(    0xa0, DEF_STR( 1C_6C ) )\
	PORT_DIPSETTING(    0x00, DEF_STR( Free_Play ) )


/***************************************************************************

							Scrolling Layers Handling

***************************************************************************/

/* Variables */
extern struct tilemap *megasys1_tmap[3];

extern data16_t *megasys1_scrollram_0, *megasys1_scrollram_1, *megasys1_scrollram_2;
extern data16_t *megasys1_objectram, *megasys1_vregs, *megasys1_ram;

extern int megasys1_scroll_flag[3], megasys1_scrollx[3], megasys1_scrolly[3], megasys1_pages_per_tmap_x[3], megasys1_pages_per_tmap_y[3];
extern int megasys1_active_layers, megasys1_sprite_bank;
extern int megasys1_screen_flag, megasys1_sprite_flag;
extern int megasys1_bits_per_color_code;
extern int megasys1_8x8_scroll_0_factor, megasys1_16x16_scroll_0_factor;
extern int megasys1_8x8_scroll_1_factor, megasys1_16x16_scroll_1_factor;
extern int megasys1_8x8_scroll_2_factor, megasys1_16x16_scroll_2_factor;


/* Functions */
VIDEO_START( megasys1 );
VIDEO_EOF( megasys1 );

WRITE16_HANDLER( megasys1_scrollram_0_w );
WRITE16_HANDLER( megasys1_scrollram_1_w );
WRITE16_HANDLER( megasys1_scrollram_2_w );

void megasys1_scroll_0_flag_w(int data);
void megasys1_scroll_1_flag_w(int data);
void megasys1_scroll_2_flag_w(int data);

void astyanax_rom_decode(int cpu);
void phantasm_rom_decode(int cpu);
void rodland_rom_decode (int cpu);
