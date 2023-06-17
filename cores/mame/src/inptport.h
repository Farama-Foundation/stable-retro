#ifndef INPTPORT_H
#define INPTPORT_H

#include "memory.h"
#include "input.h"

/* input ports handling */

/* Don't confuse this with the I/O ports in memory.h. This is used to handle game */
/* inputs (joystick, coin slots, etc). Typically, you will read them using */
/* input_port_[n]_r(), which you will associate to the appropriate memory */
/* address or I/O port. */

/***************************************************************************/
#ifdef __cplusplus
extern "C" {
#endif

struct InputPortTiny
{
	UINT16 mask;			/* bits affected */
	UINT16 default_value;	/* default value for the bits affected */
							/* you can also use one of the IP_ACTIVE defines below */
	UINT32 type;			/* see defines below */
	const char *name;		/* name to display */
};

struct InputPort
{
	UINT16 mask;			/* bits affected */
	UINT16 default_value;	/* default value for the bits affected */
							/* you can also use one of the IP_ACTIVE defines below */
	UINT32 type;			/* see defines below */
	const char *name;		/* name to display */
	InputSeq seq;                  	/* input sequence affecting the input bits */
};

#define IP_ACTIVE_HIGH 0x0000
#define IP_ACTIVE_LOW 0xffff

enum { IPT_END=1,IPT_PORT,
	/* use IPT_JOYSTICK for panels where the player has one single joystick */
	IPT_JOYSTICK_UP, IPT_JOYSTICK_DOWN, IPT_JOYSTICK_LEFT, IPT_JOYSTICK_RIGHT,
	/* use IPT_JOYSTICKLEFT and IPT_JOYSTICKRIGHT for dual joystick panels */
	IPT_JOYSTICKRIGHT_UP, IPT_JOYSTICKRIGHT_DOWN, IPT_JOYSTICKRIGHT_LEFT, IPT_JOYSTICKRIGHT_RIGHT,
	IPT_JOYSTICKLEFT_UP, IPT_JOYSTICKLEFT_DOWN, IPT_JOYSTICKLEFT_LEFT, IPT_JOYSTICKLEFT_RIGHT,
	IPT_BUTTON1, IPT_BUTTON2, IPT_BUTTON3, IPT_BUTTON4,	/* action buttons */
	IPT_BUTTON5, IPT_BUTTON6, IPT_BUTTON7, IPT_BUTTON8, IPT_BUTTON9, IPT_BUTTON10,

	/* analog inputs */
	/* the "arg" field contains the default sensitivity expressed as a percentage */
	/* (100 = default, 50 = half, 200 = twice) */
	IPT_ANALOG_START,
	IPT_PADDLE, IPT_PADDLE_V,
	IPT_DIAL, IPT_DIAL_V,
	IPT_TRACKBALL_X, IPT_TRACKBALL_Y,
	IPT_AD_STICK_X, IPT_AD_STICK_Y, IPT_AD_STICK_Z,
	IPT_LIGHTGUN_X, IPT_LIGHTGUN_Y,
	IPT_PEDAL, IPT_PEDAL2,
	IPT_ANALOG_END,

	IPT_START1, IPT_START2, IPT_START3, IPT_START4,	/* start buttons */
	IPT_COIN1, IPT_COIN2, IPT_COIN3, IPT_COIN4,	/* coin slots */
	IPT_SERVICE1, IPT_SERVICE2, IPT_SERVICE3, IPT_SERVICE4,	/* service coin */
	IPT_SERVICE, IPT_TILT,
	IPT_DIPSWITCH_NAME, IPT_DIPSWITCH_SETTING,

/* Many games poll an input bit to check for vertical blanks instead of using */
/* interrupts. This special value allows you to handle that. If you set one of the */
/* input bits to this, the bit will be inverted while a vertical blank is happening. */
	IPT_VBLANK,
	IPT_UNKNOWN,
	IPT_OSD_DESCRIPTION,
	IPT_OSD_1,
	IPT_OSD_2,
	IPT_OSD_3,
	IPT_OSD_4,
	IPT_EXTENSION,	/* this is an extension on the previous InputPort, not a real inputport. */
					/* It is used to store additional parameters for analog inputs */

	/* the following are special codes for user interface handling - not to be used by drivers! */
	IPT_UI_CONFIGURE,
	IPT_UI_ON_SCREEN_DISPLAY,
	IPT_UI_RESET_MACHINE,
	IPT_UI_SHOW_GFX,
	IPT_UI_TOGGLE_CHEAT,
	IPT_UI_UP,
	IPT_UI_DOWN,
	IPT_UI_LEFT,
	IPT_UI_RIGHT,
	IPT_UI_SELECT,
	IPT_UI_CANCEL,
	IPT_UI_TOGGLE_UI,
	IPT_UI_ADD_CHEAT,
	IPT_UI_DELETE_CHEAT,
	IPT_UI_SAVE_CHEAT,
	IPT_UI_WATCH_VALUE,
	IPT_UI_EDIT_CHEAT,

	/* 8 player support */
	IPT_START5, IPT_START6, IPT_START7, IPT_START8,
	IPT_COIN5, IPT_COIN6, IPT_COIN7, IPT_COIN8,
	__ipt_max
};

#define IPT_UNUSED     IPF_UNUSED
#define IPT_SPECIAL    IPT_UNUSED	/* special meaning handled by custom functions */

#define IPF_MASK       0xffffff00
#define IPF_UNUSED     0x80000000 /* The bit is not used by this game, but is used */
                                  /* by other games running on the same hardware. */
                                  /* This is different from IPT_UNUSED, which marks */
                                  /* bits not connected to anything. */
#define IPF_COCKTAIL   IPF_PLAYER2	/* the bit is used in cocktail mode only */

#define IPF_CHEAT      0x40000000	/* Indicates that the input bit is a "cheat" key */
									/* (providing invulnerabilty, level advance, and */
									/* so on). MAME will not recognize it when the */
									/* -nocheat command line option is specified. */

#define IPF_PLAYERMASK 0x00070000	/* use IPF_PLAYERn if more than one person can */
#define IPF_PLAYER1    0         	/* play at the same time. The IPT_ should be the same */
#define IPF_PLAYER2    0x00010000	/* for all players (e.g. IPT_BUTTON1 | IPF_PLAYER2) */
#define IPF_PLAYER3    0x00020000	/* IPF_PLAYER1 is the default and can be left out to */
#define IPF_PLAYER4    0x00030000	/* increase readability. */
#define IPF_PLAYER5    0x00040000
#define IPF_PLAYER6    0x00050000
#define IPF_PLAYER7    0x00060000
#define IPF_PLAYER8    0x00070000

#define IPF_8WAY       0          /* Joystick modes of operation. 8WAY is the default, */
#define IPF_4WAY       0x00080000 /* it prevents left/right or up/down to be pressed at */
#define IPF_2WAY       0          /* the same time. 4WAY prevents diagonal directions. */
                                  /* 2WAY should be used for joysticks wich move only */
                                  /* on one axis (e.g. Battle Zone) */

#define IPF_IMPULSE    0x00100000	/* When this is set, when the key corrisponding to */
									/* the input bit is pressed it will be reported as */
									/* pressed for a certain number of video frames and */
									/* then released, regardless of the real status of */
									/* the key. This is useful e.g. for some coin inputs. */
									/* The number of frames the signal should stay active */
									/* is specified in the "arg" field. */
#define IPF_TOGGLE     0x00200000	/* When this is set, the key acts as a toggle - press */
									/* it once and it goes on, press it again and it goes off. */
									/* useful e.g. for sone Test Mode dip switches. */
#define IPF_REVERSE    0x00400000	/* By default, analog inputs like IPT_TRACKBALL increase */
									/* when going right/up. This flag inverts them. */

#define IPF_CENTER     0x00800000	/* always preload in->default, autocentering the STICK/TRACKBALL */

#define IPF_CUSTOM_UPDATE 0x01000000 /* normally, analog ports are updated when they are accessed. */
									/* When this flag is set, they are never updated automatically, */
									/* it is the responsibility of the driver to call */
									/* update_analog_port(int port). */

#define IPF_RESETCPU   0x02000000	/* when the key is pressed, reset the first CPU */


/* The "arg" field contains 4 bytes fields */
#define IPF_SENSITIVITY(percent)	((percent & 0xff) << 8)
#define IPF_DELTA(val)				((val & 0xff) << 16)

#define IP_GET_PLAYER(port) (((port)->type >> 16) & 7)
#define IP_GET_IMPULSE(port) (((port)->type >> 8) & 0xff)
#define IP_GET_SENSITIVITY(port) ((((port)+1)->type >> 8) & 0xff)
#define IP_SET_SENSITIVITY(port,val) ((port)+1)->type = ((port+1)->type & 0xffff00ff)|((val&0xff)<<8)
#define IP_GET_DELTA(port) ((((port)+1)->type >> 16) & 0xff)
#define IP_SET_DELTA(port,val) ((port)+1)->type = ((port+1)->type & 0xff00ffff)|((val&0xff)<<16)
#define IP_GET_MIN(port) (((port)+1)->mask)
#define IP_GET_MAX(port) (((port)+1)->default_value)
#define IP_GET_CODE_OR1(port) ((port)->mask)
#define IP_GET_CODE_OR2(port) ((port)->default_value)

#define IP_NAME_DEFAULT ((const char *)-1)

/* Wrapper for compatibility */
#define IP_KEY_DEFAULT CODE_DEFAULT
#define IP_JOY_DEFAULT CODE_DEFAULT
#define IP_KEY_PREVIOUS CODE_PREVIOUS
#define IP_JOY_PREVIOUS CODE_PREVIOUS
#define IP_KEY_NONE CODE_NONE
#define IP_JOY_NONE CODE_NONE

/* start of table */
#define INPUT_PORTS_START(name) \
	static const struct InputPortTiny input_ports_##name[] = {

/* end of table */
#define INPUT_PORTS_END \
	{ 0, 0, IPT_END, 0  } \
	};
/* start of a new input port */
#define PORT_START \
	{ 0, 0, IPT_PORT, 0 },

/* input bit definition */
#define PORT_BIT_NAME(mask,default,type,name) \
	{ mask, default, type, name },
#define PORT_BIT(mask,default,type) \
	PORT_BIT_NAME(mask, default, type, IP_NAME_DEFAULT)

/* impulse input bit definition */
#define PORT_BIT_IMPULSE_NAME(mask,default,type,duration,name) \
	PORT_BIT_NAME(mask, default, type | IPF_IMPULSE | ((duration & 0xff) << 8), name)
#define PORT_BIT_IMPULSE(mask,default,type,duration) \
	PORT_BIT_IMPULSE_NAME(mask, default, type, duration, IP_NAME_DEFAULT)

/* key/joy code specification */
#define PORT_CODE(key,joy) \
	{ key, joy, IPT_EXTENSION, 0 },

/* input bit definition with extended fields */
#define PORT_BITX(mask,default,type,name,key,joy) \
	PORT_BIT_NAME(mask, default, type, name) \
	PORT_CODE(key,joy)

/* analog input */
#define PORT_ANALOG(mask,default,type,sensitivity,delta,min,max) \
	PORT_BIT(mask, default, type) \
	{ min, max, IPT_EXTENSION | IPF_SENSITIVITY(sensitivity) | IPF_DELTA(delta), IP_NAME_DEFAULT },

#define PORT_ANALOGX(mask,default,type,sensitivity,delta,min,max,keydec,keyinc,joydec,joyinc) \
	PORT_BIT(mask, default, type) \
	{ min, max, IPT_EXTENSION | IPF_SENSITIVITY(sensitivity) | IPF_DELTA(delta), IP_NAME_DEFAULT }, \
	PORT_CODE(keydec,joydec) \
	PORT_CODE(keyinc,joyinc)

/* dip switch definition */
#define PORT_DIPNAME(mask,default,name) \
	PORT_BIT_NAME(mask, default, IPT_DIPSWITCH_NAME, name)

#define PORT_DIPSETTING(default,name) \
	PORT_BIT_NAME(0, default, IPT_DIPSWITCH_SETTING, name)


#define PORT_SERVICE(mask,default)	\
	PORT_BITX(    mask, mask & default, IPT_DIPSWITCH_NAME | IPF_TOGGLE, DEF_STR( Service_Mode ), KEYCODE_F2, IP_JOY_NONE )	\
	PORT_DIPSETTING(    mask & default, DEF_STR( Off ) )	\
	PORT_DIPSETTING(    mask &~default, DEF_STR( On ) )

#define PORT_SERVICE_NO_TOGGLE(mask,default)	\
	PORT_BITX(    mask, mask & default, IPT_SERVICE, DEF_STR( Service_Mode ), KEYCODE_F2, IP_JOY_NONE )

#define MAX_DEFSTR_LEN 20
extern const char ipdn_defaultstrings[][MAX_DEFSTR_LEN];
#define PORT_ADJUSTER(default,name) 

/* this must match the ipdn_defaultstrings list in inptport.c */
enum {
	STR_Off,
	STR_On,
	STR_No,
	STR_Yes,
	STR_Lives,
	STR_Bonus_Life,
	STR_Difficulty,
	STR_Demo_Sounds,
	STR_Coinage,
	STR_Coin_A,
	STR_Coin_B,
	STR_9C_1C,
	STR_8C_1C,
	STR_7C_1C,
	STR_6C_1C,
	STR_5C_1C,
	STR_4C_1C,
	STR_3C_1C,
	STR_8C_3C,
	STR_4C_2C,
	STR_2C_1C,
	STR_5C_3C,
	STR_3C_2C,
	STR_4C_3C,
	STR_4C_4C,
	STR_3C_3C,
	STR_2C_2C,
	STR_1C_1C,
	STR_4C_5C,
	STR_3C_4C,
	STR_2C_3C,
	STR_4C_7C,
	STR_2C_4C,
	STR_1C_2C,
	STR_2C_5C,
	STR_2C_6C,
	STR_1C_3C,
	STR_2C_7C,
	STR_2C_8C,
	STR_1C_4C,
	STR_1C_5C,
	STR_1C_6C,
	STR_1C_7C,
	STR_1C_8C,
	STR_1C_9C,
	STR_Free_Play,
	STR_Cabinet,
	STR_Upright,
	STR_Cocktail,
	STR_Flip_Screen,
	STR_Service_Mode,
	STR_Unused,
	STR_Unknown,
	STR_Hardest,
	STR_Hard,
	STR_Normal,
	STR_Easy,
	STR_Allow_Continue,
	STR_None,
	STR_TOTAL
};

enum { IKT_STD, IKT_IPT, IKT_IPT_EXT, IKT_OSD_KEY, IKT_OSD_JOY };

#define DEF_STR(str_num) (ipdn_defaultstrings[STR_##str_num])

#define MAX_INPUT_PORTS 30


int load_input_port_settings(void);
void save_input_port_settings(void);

const char *input_port_name(const struct InputPort *in);
InputSeq* input_port_type_seq(int type);
InputSeq* input_port_seq(const struct InputPort *in);

struct InputPort* input_port_allocate(const struct InputPortTiny *src);
void input_port_free(struct InputPort* dst);

void init_analog_seq(void);

void update_analog_port(int port);
void update_input_ports(void);	/* called by cpuintrf.c - not for external use */
void inputport_vblank_end(void);	/* called by cpuintrf.c - not for external use */

int readinputport(int port);
READ_HANDLER( input_port_0_r );
READ_HANDLER( input_port_1_r );
READ_HANDLER( input_port_2_r );
READ_HANDLER( input_port_3_r );
READ_HANDLER( input_port_4_r );
READ_HANDLER( input_port_5_r );
READ_HANDLER( input_port_6_r );
READ_HANDLER( input_port_7_r );
READ_HANDLER( input_port_8_r );
READ_HANDLER( input_port_9_r );
READ_HANDLER( input_port_10_r );
READ_HANDLER( input_port_11_r );
READ_HANDLER( input_port_12_r );
READ_HANDLER( input_port_13_r );
READ_HANDLER( input_port_14_r );
READ_HANDLER( input_port_15_r );
READ_HANDLER( input_port_16_r );
READ_HANDLER( input_port_17_r );
READ_HANDLER( input_port_18_r );
READ_HANDLER( input_port_19_r );
READ_HANDLER( input_port_20_r );
READ_HANDLER( input_port_21_r );
READ_HANDLER( input_port_22_r );
READ_HANDLER( input_port_23_r );
READ_HANDLER( input_port_24_r );
READ_HANDLER( input_port_25_r );
READ_HANDLER( input_port_26_r );
READ_HANDLER( input_port_27_r );
READ_HANDLER( input_port_28_r );
READ_HANDLER( input_port_29_r );

READ16_HANDLER( input_port_0_word_r );
READ16_HANDLER( input_port_1_word_r );
READ16_HANDLER( input_port_2_word_r );
READ16_HANDLER( input_port_3_word_r );
READ16_HANDLER( input_port_4_word_r );
READ16_HANDLER( input_port_5_word_r );
READ16_HANDLER( input_port_6_word_r );
READ16_HANDLER( input_port_7_word_r );
READ16_HANDLER( input_port_8_word_r );
READ16_HANDLER( input_port_9_word_r );
READ16_HANDLER( input_port_10_word_r );
READ16_HANDLER( input_port_11_word_r );
READ16_HANDLER( input_port_12_word_r );
READ16_HANDLER( input_port_13_word_r );
READ16_HANDLER( input_port_14_word_r );
READ16_HANDLER( input_port_15_word_r );
READ16_HANDLER( input_port_16_word_r );
READ16_HANDLER( input_port_17_word_r );
READ16_HANDLER( input_port_18_word_r );
READ16_HANDLER( input_port_19_word_r );
READ16_HANDLER( input_port_20_word_r );
READ16_HANDLER( input_port_21_word_r );
READ16_HANDLER( input_port_22_word_r );
READ16_HANDLER( input_port_23_word_r );
READ16_HANDLER( input_port_24_word_r );
READ16_HANDLER( input_port_25_word_r );
READ16_HANDLER( input_port_26_word_r );
READ16_HANDLER( input_port_27_word_r );
READ16_HANDLER( input_port_28_word_r );
READ16_HANDLER( input_port_29_word_r );

struct ipd
{
	UINT32 type;
	const char *name;
	InputSeq seq;
};

struct ik
{
	const char *name;
	UINT32 type;
	UINT32 val;
};

const char *generic_ctrl_label(int input);

/* 
 * void reset_default_inputs(void)
 * repopulate mappings from the defaults specified in the inptport source 
 */
void reset_default_inputs(void);

/* 
 * void reset_default_keys(void)
 * repopulate mappings from the defaults specified in the driver source 
 */
void reset_driver_inputs(void);

#ifdef __cplusplus
}
#endif


#endif
